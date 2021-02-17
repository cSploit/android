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
 * @file ClientContext.h
 * @brief Interface of the class ClientContext
 */

#ifndef CLIENTCONTEXT_H
#define CLIENTCONTEXT_H

#include <string>

#include "svn_types.h"
#include "svn_client.h"

#include <jni.h>
#include "Pool.h"
#include "JNIStringHolder.h"

class Prompter;
class CommitMessage;

/**
 * This class contains a Java objects implementing the interface ClientContext
 * and implements the functions read & close of svn_stream_t.
 *
 */
class ClientContext
{
 private:
  svn_client_ctx_t *m_context;
  const SVN::Pool *m_pool;
  jobject m_jctx;

  std::string m_userName;
  std::string m_passWord;
  std::string m_configDir;

  Prompter *m_prompter;
  bool m_cancelOperation;

 protected:
  static void notify(void *baton, const svn_wc_notify_t *notify,
                     apr_pool_t *pool);
  static void progress(apr_off_t progressVal, apr_off_t total,
                       void *baton, apr_pool_t *pool);
  static svn_error_t *resolve(svn_wc_conflict_result_t **result,
                              const svn_wc_conflict_description2_t *desc,
                              void *baton,
                              apr_pool_t *result_pool,
                              apr_pool_t *scratch_pool);
  static svn_wc_conflict_result_t *javaResultToC(jobject result,
                                                 apr_pool_t *pool);

 public:
  ClientContext(jobject jsvnclient, SVN::Pool &pool);
  ~ClientContext();

  static svn_error_t *checkCancel(void *cancelBaton);

  svn_client_ctx_t *getContext(CommitMessage *message, SVN::Pool &in_pool);

  void username(const char *pi_username);
  void password(const char *pi_password);
  void setPrompt(Prompter *prompter);
  void cancelOperation();
  const char *getConfigDirectory() const;

  /**
   * Set the configuration directory, taking the usual steps to
   * ensure that Subversion's config file templates exist in the
   * specified location.
   */
  void setConfigDirectory(const char *configDir);
};

#endif // CLIENTCONTEXT_H
