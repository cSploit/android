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
 * @file StatusCallback.cpp
 * @brief Implementation of the class StatusCallback
 */

#include "StatusCallback.h"
#include "CreateJ.h"
#include "EnumMapper.h"
#include "JNIUtil.h"
#include "svn_time.h"

/**
 * Create a StatusCallback object
 * @param jcallback the Java callback object.
 */
StatusCallback::StatusCallback(jobject jcallback)
{
  m_callback = jcallback;
}

/**
 * Destroy a StatusCallback object
 */
StatusCallback::~StatusCallback()
{
  // the m_callback does not need to be destroyed, because it is the passed
  // in parameter to the Java SVNClient.status method.
}

svn_error_t *
StatusCallback::callback(void *baton,
                         const char *local_abspath,
                         const svn_client_status_t *status,
                         apr_pool_t *pool)
{
  if (baton)
    return ((StatusCallback *)baton)->doStatus(local_abspath, status, pool);

  return SVN_NO_ERROR;
}

/**
 * Callback called for a single status item.
 */
svn_error_t *
StatusCallback::doStatus(const char *local_abspath,
                         const svn_client_status_t *status,
                         apr_pool_t *pool)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  static jmethodID mid = 0; // the method id will not change during
  // the time this library is loaded, so
  // it can be cached.
  if (mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/StatusCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);

      mid = env->GetMethodID(clazz, "doStatus",
                             "(Ljava/lang/String;"
                             "L"JAVA_PACKAGE"/types/Status;)V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  jstring jPath = JNIUtil::makeJString(local_abspath);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jobject jStatus = CreateJ::Status(wc_ctx, status, pool);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  env->CallVoidMethod(m_callback, mid, jPath, jStatus);
  // We return here regardless of whether an exception is thrown or not,
  // so we do not need to explicitly check for one.

  env->PopLocalFrame(NULL);
  return SVN_NO_ERROR;
}

void
StatusCallback::setWcCtx(svn_wc_context_t *wc_ctx_in)
{
  this->wc_ctx = wc_ctx_in;
}
