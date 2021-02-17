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
 * @file InfoCallback.cpp
 * @brief Implementation of the class InfoCallback
 */

#include "InfoCallback.h"
#include "CreateJ.h"
#include "EnumMapper.h"
#include "JNIUtil.h"
#include "svn_time.h"
#include "svn_path.h"

/**
 * Create a InfoCallback object.  jcallback is the Java callback object.
 */
InfoCallback::InfoCallback(jobject jcallback)
{
  m_callback = jcallback;
}

InfoCallback::~InfoCallback()
{
  // The m_callback does not need to be destroyed, because it is the
  // passed in parameter to the Java SVNClient.info method.
}

svn_error_t *
InfoCallback::callback(void *baton,
                       const char *path,
                       const svn_client_info2_t *info,
                       apr_pool_t *pool)
{
  if (baton)
    return ((InfoCallback *)baton)->singleInfo(path, info, pool);

  return SVN_NO_ERROR;
}

/**
 * Callback called for a single path.
 * @param path is the path name
 * @param info the information for the path
 * @param pool to use for memory allocation.
 */
svn_error_t *
InfoCallback::singleInfo(const char *path,
                         const svn_client_info2_t *info,
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
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/InfoCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);

      mid = env->GetMethodID(clazz, "singleInfo",
                             "(L"JAVA_PACKAGE"/types/Info;)V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  jobject jinfo2 = CreateJ::Info(path, info);
  if (jinfo2 == NULL || JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  env->CallVoidMethod(m_callback, mid, jinfo2);
  // Return SVN_NO_ERROR here regardless of an exception or not.

  env->PopLocalFrame(NULL);
  return SVN_NO_ERROR;
}
