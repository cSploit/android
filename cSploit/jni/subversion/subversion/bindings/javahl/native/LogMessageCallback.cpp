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
 * @file LogMessageCallback.cpp
 * @brief Implementation of the class LogMessageCallback
 */

#include "LogMessageCallback.h"
#include "CreateJ.h"
#include "EnumMapper.h"
#include "JNIUtil.h"
#include "svn_time.h"
#include "svn_sorts.h"
#include "svn_compat.h"

/**
 * Create a LogMessageCallback object
 * @param jcallback the Java callback object.
 */
LogMessageCallback::LogMessageCallback(jobject jcallback)
{
  m_callback = jcallback;
}

/**
 * Destroy a LogMessageCallback object
 */
LogMessageCallback::~LogMessageCallback()
{
  // The m_callback does not need to be destroyed because it is the
  // passed in parameter to the Java SVNClientInterface.logMessages
  // method.
}

svn_error_t *
LogMessageCallback::callback(void *baton,
                             svn_log_entry_t *log_entry,
                             apr_pool_t *pool)
{
  if (baton)
    return ((LogMessageCallback *)baton)->singleMessage(log_entry, pool);

  return SVN_NO_ERROR;
}

/**
 * Callback called for a single log message
 */
svn_error_t *
LogMessageCallback::singleMessage(svn_log_entry_t *log_entry, apr_pool_t *pool)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID sm_mid = 0;
  if (sm_mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/LogMessageCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);

      sm_mid = env->GetMethodID(clazz,
                                "singleMessage",
                                "(Ljava/util/Set;JLjava/util/Map;Z)V");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  jobject jChangedPaths = NULL;
  if (log_entry->changed_paths)
    {
      apr_hash_index_t *hi;
      std::vector<jobject> jcps;

      for (hi = apr_hash_first(pool, log_entry->changed_paths);
           hi;
           hi = apr_hash_next(hi))
        {
          const char *path = (const char *) svn__apr_hash_index_key(hi);
          svn_log_changed_path2_t *log_item =
                    (svn_log_changed_path2_t *) svn__apr_hash_index_val(hi);

          jobject cp = CreateJ::ChangedPath(path, log_item);

          jcps.push_back(cp);
        }

      jChangedPaths = CreateJ::Set(jcps);
    }

  jobject jrevprops = NULL;
  if (log_entry->revprops != NULL && apr_hash_count(log_entry->revprops) > 0)
    jrevprops = CreateJ::PropertyMap(log_entry->revprops);

  env->CallVoidMethod(m_callback,
                      sm_mid,
                      jChangedPaths,
                      (jlong)log_entry->revision,
                      jrevprops,
                      (jboolean)log_entry->has_children);
  // No need to check for an exception here, because we return anyway.

  env->PopLocalFrame(NULL);
  return SVN_NO_ERROR;
}
