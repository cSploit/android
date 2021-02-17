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
 * @file Prompter.h
 * @brief Interface of the class Prompter
 */

#ifndef PROMPTER_H
#define PROMPTER_H

#include <jni.h>
#include "svn_auth.h"
#include <string>
#include "Pool.h"
/**
 * This class requests username/password and informations about
 * ssl-certificates from the user.
 */
class Prompter
{
 private:
  /**
   * The Java callback object.
   */
  jobject m_prompter;

  /**
   * Tntermediate storage for an answer.
   */
  std::string m_answer;

  /**
   * Flag is the user allowed, that the last answer is stored in the
   * configuration.
   */
  bool m_maySave;

  Prompter(jobject jprompter);
  bool prompt(const char *realm, const char *pi_username, bool maySave);
  bool askYesNo(const char *realm, const char *question, bool yesIsDefault);
  const char *askQuestion(const char *realm, const char *question,
                          bool showAnswer, bool maySave);
  int askTrust(const char *question, bool maySave);
  jstring password();
  jstring username();
  static svn_error_t *simple_prompt(svn_auth_cred_simple_t **cred_p,
                                    void *baton, const char *realm,
                                    const char *username,
                                    svn_boolean_t may_save,
                                    apr_pool_t *pool);
  static svn_error_t *username_prompt
    (svn_auth_cred_username_t **cred_p,
     void *baton,
     const char *realm,
     svn_boolean_t may_save,
     apr_pool_t *pool);
  static svn_error_t *ssl_server_trust_prompt
    (svn_auth_cred_ssl_server_trust_t **cred_p,
     void *baton,
     const char *realm,
     apr_uint32_t failures,
     const svn_auth_ssl_server_cert_info_t *cert_info,
     svn_boolean_t may_save,
     apr_pool_t *pool);
  static svn_error_t *ssl_client_cert_prompt
    (svn_auth_cred_ssl_client_cert_t **cred_p,
     void *baton,
     const char *realm,
     svn_boolean_t may_save,
     apr_pool_t *pool);
  static svn_error_t *ssl_client_cert_pw_prompt
    (svn_auth_cred_ssl_client_cert_pw_t **cred_p,
     void *baton,
     const char *realm,
     svn_boolean_t may_save,
     apr_pool_t *pool);
 public:
  static Prompter *makeCPrompter(jobject jprompter);
  ~Prompter();
  svn_auth_provider_object_t *getProviderUsername(SVN::Pool &in_pool);
  svn_auth_provider_object_t *getProviderSimple(SVN::Pool &in_pool);
  svn_auth_provider_object_t *getProviderServerSSLTrust(SVN::Pool &in_pool);
  svn_auth_provider_object_t *getProviderClientSSL(SVN::Pool &in_pool);
  svn_auth_provider_object_t *getProviderClientSSLPassword(SVN::Pool &in_pool);

  static svn_error_t *plaintext_prompt(svn_boolean_t *may_save_plaintext,
                                       const char *realmstring,
                                       void *baton,
                                       apr_pool_t *pool);
  static svn_error_t *plaintext_passphrase_prompt(
                                      svn_boolean_t *may_save_plaintext,
                                      const char *realmstring,
                                      void *baton,
                                      apr_pool_t *pool);
};

#endif // PROMPTER_H
