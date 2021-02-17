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
 * @file CommitMessage.cpp
 * @brief Implementation of the class CommitMessage
 */

#include "CommitMessage.h"
#include "CreateJ.h"
#include "EnumMapper.h"
#include "JNIUtil.h"
#include "JNIStringHolder.h"

#include <apr_tables.h>
#include "svn_client.h"

CommitMessage::CommitMessage(jobject jcommitMessage)
{
  m_jcommitMessage = jcommitMessage;
}

CommitMessage::~CommitMessage()
{
  // No need to delete the local reference
}

svn_error_t *
CommitMessage::callback(const char **log_msg,
                        const char **tmp_file,
                        const apr_array_header_t *commit_items,
                        void *baton,
                        apr_pool_t *pool)
{
  if (baton && ((CommitMessage *)baton)->m_jcommitMessage)
    return ((CommitMessage *)baton)->getCommitMessage(log_msg, tmp_file,
                                                      commit_items, pool);

  *log_msg = NULL;
  *tmp_file = NULL;
  return SVN_NO_ERROR;
}

svn_error_t *
CommitMessage::getCommitMessage(const char **log_msg,
                                const char **tmp_file,
                                const apr_array_header_t *commit_items,
                                apr_pool_t *pool)
{
  *tmp_file = NULL;
  JNIEnv *env = JNIUtil::getEnv();

  // get the method if for the CommitMessage callback method
  static jmethodID midCallback = 0;
  if (midCallback == 0)
    {
      jclass clazz2 = env->FindClass(JAVA_PACKAGE"/callback/CommitMessageCallback");
      if (JNIUtil::isJavaExceptionThrown())
        return SVN_NO_ERROR;

      midCallback = env->GetMethodID(clazz2, "getLogMessage",
                                     "(Ljava/util/Set;)Ljava/lang/String;");
      if (JNIUtil::isJavaExceptionThrown())
        return SVN_NO_ERROR;

      env->DeleteLocalRef(clazz2);
    }

  // create a Java CommitItem for each of the passed in commit items
  std::vector<jobject> jitems;
  for (int i = 0; i < commit_items->nelts; ++i)
    {
      svn_client_commit_item3_t *item =
        APR_ARRAY_IDX(commit_items, i, svn_client_commit_item3_t *);

      jobject jitem = CreateJ::CommitItem(item);

      // store the Java object into the array
      jitems.push_back(jitem);
    }

  // call the Java callback method
  jstring jmessage = (jstring)env->CallObjectMethod(m_jcommitMessage,
                                                    midCallback,
                                                    CreateJ::Set(jitems));
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  if (jmessage != NULL)
    {
      JNIStringHolder msg(jmessage);
      *log_msg = apr_pstrdup(pool, msg);
    }
  else
    *log_msg = NULL;

  return SVN_NO_ERROR;
}
