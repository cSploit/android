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
 * @file ListCallback.cpp
 * @brief Implementation of the class ListCallback
 */

#include "ListCallback.h"
#include "EnumMapper.h"
#include "CreateJ.h"
#include "JNIUtil.h"
#include "svn_time.h"

/**
 * Create a ListCallback object
 * @param jcallback the Java callback object.
 */
ListCallback::ListCallback(jobject jcallback)
{
  m_callback = jcallback;
}

/**
 * Destroy a ListCallback object
 */
ListCallback::~ListCallback()
{
  // The m_callback does not need to be destroyed, because it is the passed
  // in parameter to the Java SVNClient.list method.
}

svn_error_t *
ListCallback::callback(void *baton,
                       const char *path,
                       const svn_dirent_t *dirent,
                       const svn_lock_t *lock,
                       const char *abs_path,
                       apr_pool_t *pool)
{
  if (baton)
    return ((ListCallback *)baton)->doList(path, dirent, lock, abs_path,
                                           pool);

  return SVN_NO_ERROR;
}

/**
 * Callback called for each directory entry.
 */
svn_error_t *
ListCallback::doList(const char *path,
                     const svn_dirent_t *dirent,
                     const svn_lock_t *lock,
                     const char *abs_path,
                     apr_pool_t *pool)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/ListCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);

      mid = env->GetMethodID(clazz, "doEntry",
                             "(L"JAVA_PACKAGE"/types/DirEntry;"
                             "L"JAVA_PACKAGE"/types/Lock;)V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  // convert the parameters to their Java relatives
  jobject jdirentry = createJavaDirEntry(path, abs_path, dirent);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  jobject jlock = NULL;
  if (lock != NULL)
    {
      jlock = CreateJ::Lock(lock);
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  // call the Java method
  env->CallVoidMethod(m_callback, mid, jdirentry, jlock);
  // No need to check for exception here, because we'll just return anyway

  env->PopLocalFrame(NULL);
  return SVN_NO_ERROR;
}

/**
 * Create a DirEntry Java object from the svn_dirent_t structure.
 */
jobject
ListCallback::createJavaDirEntry(const char *path, const char *absPath,
                                 const svn_dirent_t *dirent)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  jclass clazz = env->FindClass(JAVA_PACKAGE"/types/DirEntry");
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  static jmethodID mid = 0;
  if (mid == 0)
    {
      mid = env->GetMethodID(clazz, "<init>",
                             "(Ljava/lang/String;Ljava/lang/String;"
                             "L"JAVA_PACKAGE"/types/NodeKind;"
                             "JZJJLjava/lang/String;)V");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN_NULL;
    }

  jstring jPath = JNIUtil::makeJString(path);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jstring jAbsPath = JNIUtil::makeJString(absPath);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jobject jNodeKind = EnumMapper::mapNodeKind(dirent->kind);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jlong jSize = dirent->size;
  jboolean jHasProps = (dirent->has_props? JNI_TRUE : JNI_FALSE);
  jlong jLastChangedRevision = dirent->created_rev;
  jlong jLastChanged = dirent->time;
  jstring jLastAuthor = JNIUtil::makeJString(dirent->last_author);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jobject ret = env->NewObject(clazz, mid, jPath, jAbsPath, jNodeKind,
                               jSize, jHasProps, jLastChangedRevision,
                               jLastChanged, jLastAuthor);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  return env->PopLocalFrame(ret);
}
