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
 * @file DiffSummaryReceiver.cpp
 * @brief Implementation of the class DiffSummaryReceiver
 */

#include "JNIUtil.h"
#include "EnumMapper.h"
#include "DiffSummaryReceiver.h"

DiffSummaryReceiver::DiffSummaryReceiver(jobject jreceiver)
{
  m_receiver = jreceiver;
}

DiffSummaryReceiver::~DiffSummaryReceiver()
{
  // m_receiver does not need to be destroyed, because it is a
  // parameter to the Java SVNClient.diffSummarize() method, and
  // thus not explicitly managed.
}

svn_error_t *
DiffSummaryReceiver::summarize(const svn_client_diff_summarize_t *diff,
                               void *baton,
                               apr_pool_t *pool)
{
  if (baton)
    return ((DiffSummaryReceiver *) baton)->onSummary(diff, pool);

  return SVN_NO_ERROR;
}

svn_error_t *
DiffSummaryReceiver::onSummary(const svn_client_diff_summarize_t *diff,
                               apr_pool_t *pool)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  // As Java method IDs will not change during the time this library
  // is loaded, they can be cached.
  static jmethodID callback = 0;
  jclass clazz;
  if (callback == 0)
    {
      // Initialize the method ID.
      clazz = env->FindClass(JAVA_PACKAGE "/callback/DiffSummaryCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);

      callback = env->GetMethodID(clazz, "onSummary",
                                  "(L"JAVA_PACKAGE"/DiffSummary;)V");
      if (JNIUtil::isJavaExceptionThrown() || callback == 0)
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  // Do some prep work for tranforming the DIFF parameter into a
  // Java equivalent.
  static jmethodID ctor = 0;
  clazz = env->FindClass(JAVA_PACKAGE "/DiffSummary");
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  if (ctor == 0)
    {
      ctor = env->GetMethodID(clazz, "<init>",
                              "(Ljava/lang/String;"
                              "L"JAVA_PACKAGE"/DiffSummary$DiffKind;Z"
                              "L"JAVA_PACKAGE"/types/NodeKind;)V");
      if (JNIUtil::isJavaExceptionThrown() || ctor == 0)
        POP_AND_RETURN(SVN_NO_ERROR);
    }
  // Convert the arguments into their Java equivalent,
  jstring jPath = JNIUtil::makeJString(diff->path);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  jobject jNodeKind = EnumMapper::mapNodeKind(diff->node_kind);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  jobject jSummarizeKind = EnumMapper::mapSummarizeKind(diff->summarize_kind);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  // Actually tranform the DIFF parameter into a Java equivalent.
  jobject jDiffSummary = env->NewObject(clazz, ctor, jPath, jSummarizeKind,
                                        (jboolean) diff->prop_changed,
                                        jNodeKind);
  if (JNIUtil::isJavaExceptionThrown() || jDiffSummary == NULL)
    POP_AND_RETURN(SVN_NO_ERROR);

  // Invoke the Java DiffSummaryReceiver callback.
  env->CallVoidMethod(m_receiver, callback, jDiffSummary);
  // We return whether an exception was thrown or not.

  env->PopLocalFrame(NULL);
  return SVN_NO_ERROR;
}
