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
 * @file CommitCallback.cpp
 * @brief Implementation of the class CommitCallback
 */

#include "CommitCallback.h"
#include "CreateJ.h"
#include "EnumMapper.h"
#include "JNIUtil.h"
#include "svn_time.h"
#include "svn_sorts.h"
#include "svn_compat.h"

/**
 * Create a CommitCallback object
 * @param jcallback the Java callback object.
 */
CommitCallback::CommitCallback(jobject jcallback)
{
  m_callback = jcallback;
}

/**
 * Destroy a CommitCallback object
 */
CommitCallback::~CommitCallback()
{
  // The m_callback does not need to be destroyed because it is the
  // passed in parameter to the Java SVNClientInterface.logMessages
  // method.
}

svn_error_t *
CommitCallback::callback(const svn_commit_info_t *commit_info,
                         void *baton,
                         apr_pool_t *pool)
{
  if (baton)
    return ((CommitCallback *)baton)->commitInfo(commit_info, pool);

  return SVN_NO_ERROR;
}

/**
 * Callback called for a single log message
 */
svn_error_t *
CommitCallback::commitInfo(const svn_commit_info_t *commit_info,
                           apr_pool_t *pool)
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
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/CommitCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);

      sm_mid = env->GetMethodID(clazz,
                                "commitInfo",
                                "(L"JAVA_PACKAGE"/CommitInfo;)V");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  jobject jcommitInfo = CreateJ::CommitInfo(commit_info);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  env->CallVoidMethod(m_callback, sm_mid, jcommitInfo);
  // No need to check for an exception here, because we return anyway.

  env->PopLocalFrame(NULL);
  return SVN_NO_ERROR;
}
