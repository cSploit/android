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
 * @file ReposNotifyCallback.cpp
 * @brief Implementation of the class ReposNotifyCallback
 */

#include "ReposNotifyCallback.h"
#include "JNIUtil.h"
#include "CreateJ.h"
#include "EnumMapper.h"
#include "RevisionRange.h"

/**
 * Create a new object and store the Java object.
 * @param notify    global reference to the Java object
 */
ReposNotifyCallback::ReposNotifyCallback(jobject p_notify)
{
  m_notify = p_notify;
}

ReposNotifyCallback::~ReposNotifyCallback()
{
  // Don't need to destroy the reference, since it was given us by Java
}

void
ReposNotifyCallback::notify(void *baton, const svn_repos_notify_t *notify,
                            apr_pool_t *pool)
{
  if (baton)
    ((ReposNotifyCallback *)baton)->onNotify(notify, pool);
}

/**
 * Handler for Subversion notifications.
 *
 * @param notify all the information about the event
 * @param pool an apr pool to allocated memory
 */
void
ReposNotifyCallback::onNotify(const svn_repos_notify_t *wcNotify,
                              apr_pool_t *pool)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Java method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/ReposNotifyCallback");
      if (JNIUtil::isJavaExceptionThrown())
        return;

      mid = env->GetMethodID(clazz, "onNotify",
                             "(L"JAVA_PACKAGE"/ReposNotifyInformation;)V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        return;

      env->DeleteLocalRef(clazz);
    }

  jobject jInfo = CreateJ::ReposNotifyInformation(wcNotify);
  if (JNIUtil::isJavaExceptionThrown())
    return;

  env->CallVoidMethod(m_notify, mid, jInfo);
  if (JNIUtil::isJavaExceptionThrown())
    return;

  env->DeleteLocalRef(jInfo);
}
