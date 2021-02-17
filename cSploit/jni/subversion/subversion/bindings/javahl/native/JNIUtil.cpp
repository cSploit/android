/**
 * @copyright
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 * @endcopyright
 *
 * @file JNIUtil.cpp
 * @brief Implementation of the class JNIUtil
 */

#include "JNIUtil.h"
#include "Array.h"

#include <sstream>
#include <vector>
#include <locale.h>

#include <apr_strings.h>
#include <apr_tables.h>
#include <apr_general.h>
#include <apr_lib.h>

#include "svn_pools.h"
#include "svn_wc.h"
#include "svn_dso.h"
#include "svn_path.h"
#include <apr_file_info.h>
#include "svn_private_config.h"
#ifdef WIN32
/* FIXME: We're using an internal APR header here, which means we
   have to build Subversion with APR sources. This being Win32-only,
   that should be fine for now, but a better solution must be found in
   combination with issue #850. */
extern "C" {
#include <arch/win32/apr_arch_utf8.h>
};
#endif

#include "SVNBase.h"
#include "JNIMutex.h"
#include "JNICriticalSection.h"
#include "JNIThreadData.h"
#include "JNIStringHolder.h"
#include "Pool.h"

// Static members of JNIUtil are allocated here.
apr_pool_t *JNIUtil::g_pool = NULL;
std::list<SVNBase*> JNIUtil::g_finalizedObjects;
JNIMutex *JNIUtil::g_finalizedObjectsMutex = NULL;
JNIMutex *JNIUtil::g_logMutex = NULL;
bool JNIUtil::g_initException;
bool JNIUtil::g_inInit;
JNIEnv *JNIUtil::g_initEnv;
char JNIUtil::g_initFormatBuffer[formatBufferSize];
int JNIUtil::g_logLevel = JNIUtil::noLog;
std::ofstream JNIUtil::g_logStream;

/**
 * Initialize the environment for all requests.
 * @param env   the JNI environment for this request
 */
bool JNIUtil::JNIInit(JNIEnv *env)
{
  // Clear all standing exceptions.
  env->ExceptionClear();

  // Remember the env parameter for the remainder of the request.
  setEnv(env);

  // Lock the list of finalized objects.
  JNICriticalSection cs(*g_finalizedObjectsMutex) ;
  if (isExceptionThrown())
    return false;

  // Delete all finalized, but not yet deleted objects.
  for (std::list<SVNBase*>::iterator it = g_finalizedObjects.begin();
       it != g_finalizedObjects.end();
       ++it)
    {
      delete *it;
    }
  g_finalizedObjects.clear();

  return true;
}

/**
 * Initialize the environment for all requests.
 * @param env   the JNI environment for this request
 */
bool JNIUtil::JNIGlobalInit(JNIEnv *env)
{
  // This method has to be run only once during the run a program.
  static bool run = false;
  svn_error_t *err;
  if (run) // already run
    return true;

  run = true;

  // Do not run this part more than one time.  This leaves a small
  // time window when two threads create their first SVNClient and
  // SVNAdmin at the same time, but I do not see a better option
  // without APR already initialized
  if (g_inInit)
    return false;

  g_inInit = true;
  g_initEnv = env;

  apr_status_t status;



  /* Initialize the APR subsystem, and register an atexit() function
   * to Uninitialize that subsystem at program exit. */
  status = apr_initialize();
  if (status)
    {
      if (stderr)
        {
          char buf[1024];
          apr_strerror(status, buf, sizeof(buf) - 1);
          fprintf(stderr,
                  "%s: error: cannot initialize APR: %s\n",
                  "svnjavahl", buf);
        }
      return FALSE;
    }

  /* This has to happen before any pools are created. */
  if ((err = svn_dso_initialize2()))
    {
      if (stderr && err->message)
        fprintf(stderr, "%s", err->message);

      svn_error_clear(err);
      return FALSE;
    }

  if (0 > atexit(apr_terminate))
    {
      if (stderr)
        fprintf(stderr,
                "%s: error: atexit registration failed\n",
                "svnjavahl");
      return FALSE;
    }

  /* Create our top-level pool. */
  g_pool = svn_pool_create(NULL);

  apr_allocator_t* allocator = apr_pool_allocator_get(g_pool);

  if (allocator)
    {
      /* Keep a maximum of 1 free block, to release memory back to the JVM
         (and other modules). */
      apr_allocator_max_free_set(allocator, 1);
    }


#ifdef ENABLE_NLS
#ifdef WIN32
  {
    WCHAR ucs2_path[MAX_PATH];
    char *utf8_path;
    const char *internal_path;
    apr_pool_t *pool;
    apr_status_t apr_err;
    apr_size_t inwords, outbytes;
    unsigned int outlength;

    pool = svn_pool_create(g_pool);
    /* get dll name - our locale info will be in '../share/locale' */
    inwords = sizeof(ucs2_path) / sizeof(ucs2_path[0]);
    HINSTANCE moduleHandle = GetModuleHandle("libsvnjavahl-1");
    GetModuleFileNameW(moduleHandle, ucs2_path, inwords);
    inwords = lstrlenW(ucs2_path);
    outbytes = outlength = 3 * (inwords + 1);
    utf8_path = (char *)apr_palloc(pool, outlength);
    apr_err = apr_conv_ucs2_to_utf8((const apr_wchar_t *) ucs2_path,
                                    &inwords, utf8_path, &outbytes);
    if (!apr_err && (inwords > 0 || outbytes == 0))
      apr_err = APR_INCOMPLETE;
    if (apr_err)
      {
        if (stderr)
          fprintf(stderr, "Can't convert module path to UTF-8");
        return FALSE;
      }
    utf8_path[outlength - outbytes] = '\0';
    internal_path = svn_dirent_internal_style(utf8_path, pool);
    /* get base path name */
    internal_path = svn_dirent_dirname(internal_path, pool);
    internal_path = svn_dirent_join(internal_path, SVN_LOCALE_RELATIVE_PATH,
                                  pool);
    bindtextdomain(PACKAGE_NAME, internal_path);
    svn_pool_destroy(pool);
  }
#else
  bindtextdomain(PACKAGE_NAME, SVN_LOCALE_DIR);
#endif
#endif

#if defined(WIN32) || defined(__CYGWIN__)
  /* See http://svn.apache.org/repos/asf/subversion/trunk/notes/asp-dot-net-hack.txt */
  /* ### This code really only needs to be invoked by consumers of
     ### the libsvn_wc library, which basically means SVNClient. */
  if (getenv ("SVN_ASP_DOT_NET_HACK"))
    {
      err = svn_wc_set_adm_dir("_svn", g_pool);
      if (err)
        {
          if (stderr)
            {
              fprintf(stderr,
                      "%s: error: SVN_ASP_DOT_NET_HACK failed: %s\n",
                      "svnjavahl", err->message);
            }
          svn_error_clear(err);
          return FALSE;
        }
    }
#endif

  // Build all mutexes.
  g_finalizedObjectsMutex = new JNIMutex(g_pool);
  if (isExceptionThrown())
    return false;

  g_logMutex = new JNIMutex(g_pool);
  if (isExceptionThrown())
    return false;

  // initialized the thread local storage
  if (!JNIThreadData::initThreadData())
    return false;

  setEnv(env);
  if (isExceptionThrown())
    return false;

  g_initEnv = NULL;
  g_inInit = false;
  return true;
}

/**
 * Returns the global (not request specific) pool.
 * @return global pool
 */
apr_pool_t *JNIUtil::getPool()
{
  return g_pool;
}

void JNIUtil::raiseThrowable(const char *name, const char *message)
{
  if (getLogLevel() >= errorLog)
    {
      JNICriticalSection cs(*g_logMutex);
      g_logStream << "Throwable raised <" << message << ">" << std::endl;
    }

  JNIEnv *env = getEnv();
  jclass clazz = env->FindClass(name);
  if (isJavaExceptionThrown())
    return;

  env->ThrowNew(clazz, message);
  setExceptionThrown();
  env->DeleteLocalRef(clazz);
}

jstring JNIUtil::makeSVNErrorMessage(svn_error_t *err)
{
  if (err == NULL)
    return NULL;
  std::string buffer;
  assembleErrorMessage(err, 0, APR_SUCCESS, buffer);
  jstring jmessage = makeJString(buffer.c_str());
  return jmessage;
}

void
JNIUtil::throwNativeException(const char *className, const char *msg,
                              const char *source, int aprErr)
{
  JNIEnv *env = getEnv();
  jclass clazz = env->FindClass(className);

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return;

  if (getLogLevel() >= exceptionLog)
    {
      JNICriticalSection cs(*g_logMutex);
      g_logStream << "Subversion JavaHL exception thrown, message:<";
      g_logStream << msg << ">";
      if (source)
        g_logStream << " source:<" << source << ">";
      if (aprErr != -1)
        g_logStream << " apr-err:<" << aprErr << ">";
      g_logStream << std::endl;
    }
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();

  jstring jmessage = makeJString(msg);
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();
  jstring jsource = makeJString(source);
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();

  jmethodID mid = env->GetMethodID(clazz, "<init>",
                                   "(Ljava/lang/String;Ljava/lang/String;I)V");
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();
  jobject nativeException = env->NewObject(clazz, mid, jmessage, jsource,
                                           static_cast<jint>(aprErr));
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();

  env->Throw(static_cast<jthrowable>(env->PopLocalFrame(nativeException)));
}

void
JNIUtil::putErrorsInTrace(svn_error_t *err,
                          std::vector<jobject> &stackTrace)
{
  if (!err)
    return;

  JNIEnv *env = getEnv();

  // First, put all our child errors in the stack trace
  putErrorsInTrace(err->child, stackTrace);

  // Now, put our own error in the stack trace
  jclass stClazz = env->FindClass("java/lang/StackTraceElement");
  if (isJavaExceptionThrown())
    return;

  static jmethodID ctor_mid = 0;
  if (ctor_mid == 0)
    {
      ctor_mid = env->GetMethodID(stClazz, "<init>",
                                  "(Ljava/lang/String;Ljava/lang/String;"
                                  "Ljava/lang/String;I)V");
      if (isJavaExceptionThrown())
        return;
    }

  jstring jdeclClass = makeJString("native");
  if (isJavaExceptionThrown())
    return;

  char *tmp_path;
  char *path = svn_dirent_dirname(err->file, err->pool);
  while (tmp_path = strchr(path, '/'))
    *tmp_path = '.';

  jstring jmethodName = makeJString(path);
  if (isJavaExceptionThrown())
    return;

  jstring jfileName = makeJString(svn_dirent_basename(err->file, err->pool));
  if (isJavaExceptionThrown())
    return;

  jobject jelement = env->NewObject(stClazz, ctor_mid, jdeclClass, jmethodName,
                                    jfileName, (jint) err->line);

  stackTrace.push_back(jelement);

  env->DeleteLocalRef(stClazz);
  env->DeleteLocalRef(jdeclClass);
  env->DeleteLocalRef(jmethodName);
  env->DeleteLocalRef(jfileName);
}

void JNIUtil::handleSVNError(svn_error_t *err)
{
  std::string msg;
  assembleErrorMessage(svn_error_purge_tracing(err), 0, APR_SUCCESS, msg);
  const char *source = NULL;
#ifdef SVN_DEBUG
#ifndef SVN_ERR__TRACING
  if (err->file)
    {
      std::ostringstream buf;
      buf << err->file;
      if (err->line > 0)
        buf << ':' << err->line;
      source = buf.str().c_str();
    }
#endif
#endif

  // Much of the following is stolen from throwNativeException().  As much as
  // we'd like to call that function, we need to do some manual stack
  // unrolling, so it isn't feasible.

  JNIEnv *env = getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return;

  jclass clazz = env->FindClass(JAVA_PACKAGE "/ClientException");
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();

  if (getLogLevel() >= exceptionLog)
    {
      JNICriticalSection cs(*g_logMutex);
      g_logStream << "Subversion JavaHL exception thrown, message:<";
      g_logStream << msg << ">";
      if (source)
        g_logStream << " source:<" << source << ">";
      if (err->apr_err != -1)
        g_logStream << " apr-err:<" << err->apr_err << ">";
      g_logStream << std::endl;
    }
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();

  jstring jmessage = makeJString(msg.c_str());
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();
  jstring jsource = makeJString(source);
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();

  jmethodID mid = env->GetMethodID(clazz, "<init>",
                                   "(Ljava/lang/String;Ljava/lang/String;I)V");
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();
  jobject nativeException = env->NewObject(clazz, mid, jmessage, jsource,
                                           static_cast<jint>(err->apr_err));
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();

#ifdef SVN_ERR__TRACING
  // Add all the C error stack trace information to the Java Exception

  // Get the standard stack trace, and vectorize it using the Array class.
  static jmethodID mid_gst = 0;
  if (mid_gst == 0)
    {
      mid_gst = env->GetMethodID(clazz, "getStackTrace",
                                 "()[Ljava/lang/StackTraceElement;");
      if (isJavaExceptionThrown())
        POP_AND_RETURN_NOTHING();
    }
  Array stackTraceArray((jobjectArray) env->CallObjectMethod(nativeException,
                                                             mid_gst));
  std::vector<jobject> oldStackTrace = stackTraceArray.vector();

  // Build the new stack trace elements from the chained errors.
  std::vector<jobject> newStackTrace;
  putErrorsInTrace(err, newStackTrace);

  // Join the new elements with the old ones
  for (std::vector<jobject>::const_iterator it = oldStackTrace.begin();
            it < oldStackTrace.end(); ++it)
    {
      newStackTrace.push_back(*it);
    }

  jclass stClazz = env->FindClass("java/lang/StackTraceElement");
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();

  jobjectArray jStackTrace = env->NewObjectArray(newStackTrace.size(), stClazz,
                                                 NULL);
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();

  int i = 0;
  for (std::vector<jobject>::const_iterator it = newStackTrace.begin();
       it < newStackTrace.end(); ++it)
    {
      env->SetObjectArrayElement(jStackTrace, i, *it);
      ++i;
    }

  // And put the entire trace back into the exception
  static jmethodID mid_sst = 0;
  if (mid_sst == 0)
    {
      mid_sst = env->GetMethodID(clazz, "setStackTrace",
                                 "([Ljava/lang/StackTraceElement;)V");
      if (isJavaExceptionThrown())
        POP_AND_RETURN_NOTHING();
    }
  env->CallVoidMethod(nativeException, mid_sst, jStackTrace);
  if (isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();
#endif

  env->Throw(static_cast<jthrowable>(env->PopLocalFrame(nativeException)));

  svn_error_clear(err);
}

void JNIUtil::putFinalizedClient(SVNBase *object)
{
  enqueueForDeletion(object);
}

void JNIUtil::enqueueForDeletion(SVNBase *object)
{
  JNICriticalSection cs(*g_finalizedObjectsMutex);
  if (!isExceptionThrown())
    g_finalizedObjects.push_back(object);
}

/**
 * Handle an apr error (those are not expected) by throwing an error.
 * @param error the apr error number
 * @param op the apr function returning the error
 */
void JNIUtil::handleAPRError(int error, const char *op)
{
  char *buffer = getFormatBuffer();
  if (buffer == NULL)
    return;

  apr_snprintf(buffer, formatBufferSize,
               _("an error occurred in function %s with return value %d"),
               op, error);

  throwError(buffer);
}

/**
 * Return if an exception has been detected.
 * @return a exception has been detected
 */
bool JNIUtil::isExceptionThrown()
{
  // During init -> look in the global member.
  if (g_inInit)
    return g_initException;

  // Look in the thread local storage.
  JNIThreadData *data = JNIThreadData::getThreadData();
  return data == NULL || data->m_exceptionThrown;
}

/**
 * Store the JNI environment for this request in the thread local
 * storage.
 * @param env   the JNI environment
 */
void JNIUtil::setEnv(JNIEnv *env)
{
  JNIThreadData::pushNewThreadData();
  JNIThreadData *data = JNIThreadData::getThreadData();
  data->m_env = env;
  data->m_exceptionThrown = false;
}

/**
 * Return the JNI environment to use
 * @return the JNI environment
 */
JNIEnv *JNIUtil::getEnv()
{
  // During init -> look into the global variable.
  if (g_inInit)
    return g_initEnv;

  // Look in the thread local storage.
  JNIThreadData *data = JNIThreadData::getThreadData();
  return data->m_env;
}

/**
 * Check if a Java exception has been thrown.
 * @return is a Java exception has been thrown
 */
bool JNIUtil::isJavaExceptionThrown()
{
  JNIEnv *env = getEnv();
  if (env->ExceptionCheck())
    {
      // Retrieving the exception removes it so we rethrow it here.
      jthrowable exp = env->ExceptionOccurred();
      env->ExceptionDescribe();
      env->Throw(exp);
      env->DeleteLocalRef(exp);
      setExceptionThrown();
      return true;
    }
  return false;
}

const char *
JNIUtil::thrownExceptionToCString(SVN::Pool &in_pool)
{
  const char *msg;
  JNIEnv *env = getEnv();
  if (env->ExceptionCheck())
    {
      jthrowable t = env->ExceptionOccurred();
      static jmethodID getMessage = 0;
      if (getMessage == 0)
        {
          jclass clazz = env->FindClass("java/lang/Throwable");
          getMessage = env->GetMethodID(clazz, "getMessage",
                                        "(V)Ljava/lang/String;");
          env->DeleteLocalRef(clazz);
        }
      jstring jmsg = (jstring) env->CallObjectMethod(t, getMessage);
      JNIStringHolder tmp(jmsg);
      msg = tmp.pstrdup(in_pool.getPool());
      // ### Conditionally add t.printStackTrace() to msg?
    }
  else
    {
      msg = NULL;
    }
  return msg;
}

/**
 * Create a Java string from a native UTF-8 string.
 * @param txt   native UTF-8 string
 * @return the Java string. It is a local reference, which should be deleted
 *         as soon a possible
 */
jstring JNIUtil::makeJString(const char *txt)
{
  if (txt == NULL)
    // A NULL pointer is equates to a null java.lang.String.
    return NULL;

  JNIEnv *env = getEnv();
  return env->NewStringUTF(txt);
}

void
JNIUtil::setExceptionThrown(bool flag)
{
  if (g_inInit)
    {
      // During global initialization, store any errors that occur
      // in a global variable (since thread-local storage may not
      // yet be available).
      g_initException = flag;
    }
  else
    {
      // When global initialization is complete, thread-local
      // storage should be available, so store the error there.
      JNIThreadData *data = JNIThreadData::getThreadData();
      data->m_exceptionThrown = flag;
    }
}

/**
 * Initialite the log file.
 * @param level the log level
 * @param the name of the log file
 */
void JNIUtil::initLogFile(int level, jstring path)
{
  // lock this operation
  JNICriticalSection cs(*g_logMutex);
  if (g_logLevel > noLog) // if the log file has been opened
    g_logStream.close();

  // remember the log level
  g_logLevel = level;
  JNIStringHolder myPath(path);
  if (g_logLevel > noLog) // if a new log file is needed
    {
      // open it
      g_logStream.open(myPath, std::ios::app);
    }
}

/**
 * Returns a buffer to format error messages.
 * @return a buffer for formating error messages
 */
char *JNIUtil::getFormatBuffer()
{
  if (g_inInit) // during init -> use the global buffer
    return g_initFormatBuffer;

  // use the buffer in the thread local storage
  JNIThreadData *data = JNIThreadData::getThreadData();
  if (data == NULL) // if that does not exists -> use the global buffer
    return g_initFormatBuffer;

  return data->m_formatBuffer;
}

/**
 * Returns the current log level.
 * @return the log level
 */
int JNIUtil::getLogLevel()
{
  return g_logLevel;
}

/**
 * Write a message to the log file if needed.
 * @param the log message
 */
void JNIUtil::logMessage(const char *message)
{
  // lock the log file
  JNICriticalSection cs(*g_logMutex);
  g_logStream << message << std::endl;
}

/**
 * Create a java.util.Date object from an apr time.
 * @param time  the apr time
 * @return the java.util.Date. This is a local reference.  Delete as
 *         soon as possible
 */
jobject JNIUtil::createDate(apr_time_t time)
{
  jlong javatime = time /1000;
  JNIEnv *env = getEnv();
  jclass clazz = env->FindClass("java/util/Date");
  if (isJavaExceptionThrown())
    return NULL;

  static jmethodID mid = 0;
  if (mid == 0)
    {
      mid = env->GetMethodID(clazz, "<init>", "(J)V");
      if (isJavaExceptionThrown())
        return NULL;
    }

  jobject ret = env->NewObject(clazz, mid, javatime);
  if (isJavaExceptionThrown())
    return NULL;

  env->DeleteLocalRef(clazz);

  return ret;
}

/**
 * Create a Java byte array from an array of characters.
 * @param data      the character array
 * @param length    the number of characters in the array
 */
jbyteArray JNIUtil::makeJByteArray(const signed char *data, int length)
{
  if (data == NULL)
    {
      // a NULL will create no Java array
      return NULL;
    }

  JNIEnv *env = getEnv();

  // Allocate the Java array.
  jbyteArray ret = env->NewByteArray(length);
  if (isJavaExceptionThrown() || ret == NULL)
      return NULL;

  // Access the bytes.
  jbyte *retdata = env->GetByteArrayElements(ret, NULL);
  if (isJavaExceptionThrown())
    return NULL;

  // Copy the bytes.
  memcpy(retdata, data, length);

  // Release the bytes.
  env->ReleaseByteArrayElements(ret, retdata, 0);
  if (isJavaExceptionThrown())
    return NULL;

  return ret;
}

/**
 * Build the error message from the svn error into buffer.  This
 * method calls itselft recursively for all the chained errors
 *
 * @param err               the subversion error
 * @param depth             the depth of the call, used for formating
 * @param parent_apr_err    the apr of the previous level, used for formating
 * @param buffer            the buffer where the formated error message will
 *                          be stored
 */
void JNIUtil::assembleErrorMessage(svn_error_t *err, int depth,
                                   apr_status_t parent_apr_err,
                                   std::string &buffer)
{
  // buffer for a single error message
  char errbuf[256];

  /* Pretty-print the error */
  /* Note: we can also log errors here someday. */

  /* When we're recursing, don't repeat the top-level message if its
   * the same as before. */
  if (depth == 0 || err->apr_err != parent_apr_err)
    {
      /* Is this a Subversion-specific error code? */
      if ((err->apr_err > APR_OS_START_USEERR)
          && (err->apr_err <= APR_OS_START_CANONERR))
        buffer.append(svn_strerror(err->apr_err, errbuf, sizeof(errbuf)));
      /* Otherwise, this must be an APR error code. */
      else
        buffer.append(apr_strerror(err->apr_err, errbuf, sizeof(errbuf)));
      buffer.append("\n");
    }
  if (err->message)
    buffer.append(_("svn: ")).append(err->message).append("\n");

  if (err->child)
    assembleErrorMessage(err->child, depth + 1, err->apr_err, buffer);

}

/**
 * Throw a Java NullPointerException.  Used when input parameters
 * which should not be null are that.
 *
 * @param message   the name of the parameter that is null
 */
void JNIUtil::throwNullPointerException(const char *message)
{
  if (getLogLevel() >= errorLog)
    logMessage("NullPointerException thrown");

  JNIEnv *env = getEnv();
  jclass clazz = env->FindClass("java/lang/NullPointerException");
  if (isJavaExceptionThrown())
    return;

  env->ThrowNew(clazz, message);
  setExceptionThrown();
  env->DeleteLocalRef(clazz);
}

svn_error_t *JNIUtil::preprocessPath(const char *&path, apr_pool_t *pool)
{
  /* URLs and wc-paths get treated differently. */
  if (svn_path_is_url(path))
    {
      /* No need to canonicalize a URL's case or path separators. */

      /* Convert to URI. */
      path = svn_path_uri_from_iri(path, pool);

      /* Auto-escape some ASCII characters. */
      path = svn_path_uri_autoescape(path, pool);

      /* The above doesn't guarantee a valid URI. */
      if (! svn_path_is_uri_safe(path))
        return svn_error_createf(SVN_ERR_BAD_URL, NULL,
                                 _("URL '%s' is not properly URI-encoded"),
                                 path);

      /* Verify that no backpaths are present in the URL. */
      if (svn_path_is_backpath_present(path))
        return svn_error_createf(SVN_ERR_BAD_URL, NULL,
                                 _("URL '%s' contains a '..' element"),
                                 path);

      /* strip any trailing '/' */
      path = svn_uri_canonicalize(path, pool);
    }
  else  /* not a url, so treat as a path */
    {
      /* Normalize path to subversion internal style */

      /* ### In Subversion < 1.6 this method on Windows actually tried
         to lookup the path on disk to fix possible invalid casings in
         the passed path. (An extremely expensive operation; especially
         on network drives).

         This 'feature'is now removed as it penalizes every correct
         path passed, and also breaks behavior of e.g.
           'svn status .' returns '!' file, because there is only a "File"
             on disk.
            But when you then call 'svn status file', you get '? File'.

         As JavaHL is designed to be platform independent I assume users
         don't want this broken behavior on non round-trippable paths, nor
         the performance penalty.
       */

      path = svn_dirent_internal_style(path, pool);

      /* For kicks and giggles, let's absolutize it. */
      SVN_ERR(svn_dirent_get_absolute(&path, path, pool));
    }

  return NULL;
}
