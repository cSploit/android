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
 * @file CommitMessage.h
 * @brief Interface of the class CommitMessage
 */

#ifndef COMMITMESSAGE_H
#define COMMITMESSAGE_H

#include <jni.h>

#include "svn_client.h"

/**
 * This class stores a Java object implementing the CommitMessage
 * interface.
 */
class CommitMessage
{
 public:
  CommitMessage(jobject jcommitMessage);
  ~CommitMessage();

  static svn_error_t *callback(const char **log_msg,
                               const char **tmp_file,
                               const apr_array_header_t *commit_items,
                               void *baton,
                               apr_pool_t *pool);

 protected:
  svn_error_t *getCommitMessage(const char **log_msg,
                                const char **tmp_file,
                                const apr_array_header_t *commit_items,
                                apr_pool_t *pool);

 private:
  /* A local reference. */
  jobject m_jcommitMessage;

};

#endif  // COMMITMESSAGE_H
