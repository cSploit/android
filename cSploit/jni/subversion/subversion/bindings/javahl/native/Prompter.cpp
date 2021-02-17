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
 * @file Prompter.cpp
 * @brief Implementation of the class Prompter
 */

#include "Prompter.h"
#include "Pool.h"
#include "JNIUtil.h"
#include "JNIStringHolder.h"
#include "../include/org_apache_subversion_javahl_callback_UserPasswordCallback.h"
#include <apr_strings.h>
#include "svn_auth.h"
#include "svn_error.h"
#include "svn_error_codes.h"
#include "svn_private_config.h"

/**
 * Constructor
 * @param jprompter     a global reference to the Java callback object
 */
Prompter::Prompter(jobject jprompter)
{
  m_prompter = jprompter;
}

Prompter::~Prompter()
{
  if (m_prompter!= NULL)
    {
      // Since the reference to the Java object is a global one, it
      // has to be deleted.
      JNIEnv *env = JNIUtil::getEnv();
      env->DeleteGlobalRef(m_prompter);
    }
}

/**
 * Create a C++ peer object for the Java callback object
 *
 * @param jprompter     Java callback object
 * @return              C++ peer object
 */
Prompter *Prompter::makeCPrompter(jobject jprompter)
{
  // If we have no Java object, we need no C++ object.
  if (jprompter == NULL)
    return NULL;

  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  // Sanity check that the Java object implements UserPasswordCallback.
  jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/UserPasswordCallback");
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  if (!env->IsInstanceOf(jprompter, clazz))
    POP_AND_RETURN_NULL;

  // Create a new global ref for the Java object, because it is
  // longer used that this call.
  jobject myPrompt = env->NewGlobalRef(jprompter);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  env->PopLocalFrame(NULL);

  // Create the C++ peer.
  return new Prompter(myPrompt);
}

/**
 * Retrieve the username from the Java object
 * @return Java string for the username or NULL
 */
jstring Prompter::username()
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;

  if (mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/UserPasswordCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN_NULL;

      mid = env->GetMethodID(clazz, "getUsername", "()Ljava/lang/String;");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN_NULL;
    }

  jstring ret = static_cast<jstring>(env->CallObjectMethod(m_prompter, mid));
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  return (jstring) env->PopLocalFrame(ret);
}

/**
 * Retrieve the password from the Java object
 * @return Java string for the password or NULL
 */
jstring Prompter::password()
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;

  if (mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/UserPasswordCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN_NULL;

      mid = env->GetMethodID(clazz, "getPassword", "()Ljava/lang/String;");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN_NULL;
    }

  jstring ret = static_cast<jstring>(env->CallObjectMethod(m_prompter, mid));
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  return (jstring) env->PopLocalFrame(ret);
}
/**
 * Ask the user a question, which can be answered by yes/no.
 * @param realm         the server realm, for which this question is asked
 * @param question      the question to ask the user
 * @param yesIsDefault  flag if the yes-button should be the default button
 * @return flag who the user answered the question
 */
bool Prompter::askYesNo(const char *realm, const char *question,
                        bool yesIsDefault)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return false;

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;

  if (mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/UserPasswordCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(false);

      mid = env->GetMethodID(clazz, "askYesNo",
                             "(Ljava/lang/String;Ljava/lang/String;Z)Z");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN(false);
    }

  // convert the texts to Java strings
  jstring jrealm = JNIUtil::makeJString(realm);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(false);

  jstring jquestion = JNIUtil::makeJString(question);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(false);

  // execute the callback
  jboolean ret = env->CallBooleanMethod(m_prompter, mid, jrealm, jquestion,
                                        yesIsDefault ? JNI_TRUE : JNI_FALSE);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(false);

  env->PopLocalFrame(NULL);
  return ret ? true:false;
}

const char *Prompter::askQuestion(const char *realm, const char *question,
                                  bool showAnswer, bool maySave)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  static jmethodID mid = 0;
  static jmethodID mid2 = 0;
  if (mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/UserPasswordCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN_NULL;

      mid = env->GetMethodID(clazz, "askQuestion",
                             "(Ljava/lang/String;Ljava/lang/String;"
                             "ZZ)Ljava/lang/String;");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN_NULL;

      mid2 = env->GetMethodID(clazz, "userAllowedSave", "()Z");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN_NULL;
    }

  jstring jrealm = JNIUtil::makeJString(realm);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jstring jquestion = JNIUtil::makeJString(question);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jstring janswer = static_cast<jstring>(
                       env->CallObjectMethod(m_prompter, mid, jrealm,
                                    jquestion,
                                    showAnswer ? JNI_TRUE : JNI_FALSE,
                                    maySave ? JNI_TRUE : JNI_FALSE));
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  JNIStringHolder answer(janswer);
  if (answer != NULL)
    {
      m_answer = answer;
      m_maySave = env->CallBooleanMethod(m_prompter, mid2) ? true: false;
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN_NULL;
    }
  else
    {
      m_answer = "";
      m_maySave = false;
    }

  env->PopLocalFrame(NULL);
  return m_answer.c_str();
}

int Prompter::askTrust(const char *question, bool maySave)
{
   static jmethodID mid = 0;
   JNIEnv *env = JNIUtil::getEnv();

   // Create a local frame for our references
   env->PushLocalFrame(LOCAL_FRAME_SIZE);
   if (JNIUtil::isJavaExceptionThrown())
     return -1;

   if (mid == 0)
     {
       jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/UserPasswordCallback");
       if (JNIUtil::isJavaExceptionThrown())
         POP_AND_RETURN(-1);

       mid = env->GetMethodID(clazz, "askTrustSSLServer",
                              "(Ljava/lang/String;Z)I");
       if (JNIUtil::isJavaExceptionThrown() || mid == 0)
         POP_AND_RETURN(-1);
     }
   jstring jquestion = JNIUtil::makeJString(question);
   if (JNIUtil::isJavaExceptionThrown())
     POP_AND_RETURN(-1);

   jint ret = env->CallIntMethod(m_prompter, mid, jquestion,
                                 maySave ? JNI_TRUE : JNI_FALSE);
   if (JNIUtil::isJavaExceptionThrown())
     POP_AND_RETURN(-1);

   env->PopLocalFrame(NULL);
   return ret;
}

bool Prompter::prompt(const char *realm, const char *pi_username, bool maySave)
{
  JNIEnv *env = JNIUtil::getEnv();
  jboolean ret;

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return false;

  static jmethodID mid = 0;
  static jmethodID mid2 = 0;
  if (mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/UserPasswordCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(false);

      mid = env->GetMethodID(clazz, "prompt",
                             "(Ljava/lang/String;Ljava/lang/String;Z)Z");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN(false);

      mid2 = env->GetMethodID(clazz, "userAllowedSave", "()Z");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN(false);
    }

  jstring jrealm = JNIUtil::makeJString(realm);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(false);

  jstring jusername = JNIUtil::makeJString(pi_username);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(false);

  ret = env->CallBooleanMethod(m_prompter, mid, jrealm, jusername,
                               maySave ? JNI_TRUE: JNI_FALSE);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(false);

  m_maySave = env->CallBooleanMethod(m_prompter, mid2) ? true : false;
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(false);

  env->PopLocalFrame(NULL);
  return ret ? true:false;
}

svn_auth_provider_object_t *Prompter::getProviderSimple(SVN::Pool &in_pool)
{
  apr_pool_t *pool = in_pool.getPool();
  svn_auth_provider_object_t *provider;
  svn_auth_get_simple_prompt_provider(&provider,
                                      simple_prompt,
                                      this,
                                      2, /* retry limit */
                                      pool);

  return provider;
}

svn_auth_provider_object_t *Prompter::getProviderUsername(SVN::Pool &in_pool)
{
  apr_pool_t *pool = in_pool.getPool();
  svn_auth_provider_object_t *provider;
  svn_auth_get_username_prompt_provider(&provider,
                                        username_prompt,
                                        this,
                                        2, /* retry limit */
                                        pool);

  return provider;
}

svn_auth_provider_object_t *Prompter::getProviderServerSSLTrust(SVN::Pool &in_pool)
{
  apr_pool_t *pool = in_pool.getPool();
  svn_auth_provider_object_t *provider;
  svn_auth_get_ssl_server_trust_prompt_provider
    (&provider, ssl_server_trust_prompt, this, pool);

  return provider;
}

svn_auth_provider_object_t *Prompter::getProviderClientSSL(SVN::Pool &in_pool)
{
  apr_pool_t *pool = in_pool.getPool();
  svn_auth_provider_object_t *provider;
  svn_auth_get_ssl_client_cert_prompt_provider(&provider,
                                               ssl_client_cert_prompt,
                                               this,
                                               2 /* retry limit */,
                                               pool);

  return provider;
}

svn_auth_provider_object_t *Prompter::getProviderClientSSLPassword(SVN::Pool &in_pool)
{
  apr_pool_t *pool = in_pool.getPool();
  svn_auth_provider_object_t *provider;
  svn_auth_get_ssl_client_cert_pw_prompt_provider
    (&provider, ssl_client_cert_pw_prompt, this, 2 /* retry limit */,
     pool);

  return provider;
}

svn_error_t *Prompter::simple_prompt(svn_auth_cred_simple_t **cred_p,
                                     void *baton,
                                     const char *realm, const char *username,
                                     svn_boolean_t may_save,
                                     apr_pool_t *pool)
{
  Prompter *that = (Prompter*)baton;
  svn_auth_cred_simple_t *ret =
    (svn_auth_cred_simple_t*)apr_pcalloc(pool, sizeof(*ret));
  if (!that->prompt(realm, username, may_save ? true : false))
    return svn_error_create(SVN_ERR_RA_NOT_AUTHORIZED, NULL,
                            _("User canceled dialog"));
  jstring juser = that->username();
  JNIStringHolder user(juser);
  if (user == NULL)
    return svn_error_create(SVN_ERR_RA_NOT_AUTHORIZED, NULL,
                            _("User canceled dialog"));
  ret->username = apr_pstrdup(pool,user);
  jstring jpass = that->password();
  JNIStringHolder pass(jpass);
  if (pass == NULL)
    return svn_error_create(SVN_ERR_RA_NOT_AUTHORIZED, NULL,
                            _("User canceled dialog"));
  else
    {
      ret->password  = apr_pstrdup(pool, pass);
      ret->may_save = that->m_maySave;
    }
  *cred_p = ret;

  return SVN_NO_ERROR;
}

svn_error_t *Prompter::username_prompt(svn_auth_cred_username_t **cred_p,
                                       void *baton,
                                       const char *realm,
                                       svn_boolean_t may_save,
                                       apr_pool_t *pool)
{
  Prompter *that = (Prompter*)baton;
  svn_auth_cred_username_t *ret =
    (svn_auth_cred_username_t*)apr_pcalloc(pool, sizeof(*ret));
  const char *user = that->askQuestion(realm, _("Username: "), true,
                                       may_save ? true : false);
  if (user == NULL)
    return svn_error_create(SVN_ERR_RA_NOT_AUTHORIZED, NULL,
                            _("User canceled dialog"));
  ret->username = apr_pstrdup(pool,user);
  ret->may_save = that->m_maySave;
  *cred_p = ret;

  return SVN_NO_ERROR;
}

svn_error_t *
Prompter::ssl_server_trust_prompt(svn_auth_cred_ssl_server_trust_t **cred_p,
                                  void *baton,
                                  const char *realm,
                                  apr_uint32_t failures,
                                  const svn_auth_ssl_server_cert_info_t *cert_info,
                                  svn_boolean_t may_save,
                                  apr_pool_t *pool)
{
  Prompter *that = (Prompter*)baton;
  svn_auth_cred_ssl_server_trust_t *ret =
    (svn_auth_cred_ssl_server_trust_t*)apr_pcalloc(pool, sizeof(*ret));

  std::string question = _("Error validating server certificate for ");
  question += realm;
  question += ":\n";

  if (failures & SVN_AUTH_SSL_UNKNOWNCA)
    {
      question += _(" - Unknown certificate issuer\n");
      question += _("   Fingerprint: ");
      question += cert_info->fingerprint;
      question += "\n";
      question += _("   Distinguished name: ");
      question += cert_info->issuer_dname;
      question += "\n";
    }

  if (failures & SVN_AUTH_SSL_CNMISMATCH)
    {
      question += _(" - Hostname mismatch (");
      question += cert_info->hostname;
      question += _(")\n");
    }

  if (failures & SVN_AUTH_SSL_NOTYETVALID)
    {
      question += _(" - Certificate is not yet valid\n");
      question += _("   Valid from ");
      question += cert_info->valid_from;
      question += "\n";
    }

  if (failures & SVN_AUTH_SSL_EXPIRED)
    {
      question += _(" - Certificate is expired\n");
      question += _("   Valid until ");
      question += cert_info->valid_until;
      question += "\n";
    }

  switch(that->askTrust(question.c_str(), may_save ? true : false))
    {
    case org_apache_subversion_javahl_callback_UserPasswordCallback_AcceptTemporary:
      *cred_p = ret;
      ret->may_save = FALSE;
      break;
    case org_apache_subversion_javahl_callback_UserPasswordCallback_AcceptPermanently:
      *cred_p = ret;
      ret->may_save = TRUE;
      ret->accepted_failures = failures;
      break;
    default:
      *cred_p = NULL;
    }
  return SVN_NO_ERROR;
}

svn_error_t *
Prompter::ssl_client_cert_prompt(svn_auth_cred_ssl_client_cert_t **cred_p,
                                 void *baton,
                                 const char *realm,
                                 svn_boolean_t may_save,
                                 apr_pool_t *pool)
{
  Prompter *that = (Prompter*)baton;
  svn_auth_cred_ssl_client_cert_t *ret =
    (svn_auth_cred_ssl_client_cert_t*)apr_pcalloc(pool, sizeof(*ret));
  const char *cert_file =
    that->askQuestion(realm, _("client certificate filename: "), true,
                      may_save ? true : false);
  if (cert_file == NULL)
    return svn_error_create(SVN_ERR_RA_NOT_AUTHORIZED, NULL,
                            _("User canceled dialog"));
  ret->cert_file = apr_pstrdup(pool, cert_file);
  ret->may_save = that->m_maySave;
  *cred_p = ret;
  return SVN_NO_ERROR;
}

svn_error_t *
Prompter::ssl_client_cert_pw_prompt(svn_auth_cred_ssl_client_cert_pw_t **cred_p,
                                    void *baton,
                                    const char *realm,
                                    svn_boolean_t may_save,
                                    apr_pool_t *pool)
{
  Prompter *that = (Prompter*)baton;
  svn_auth_cred_ssl_client_cert_pw_t *ret =
    (svn_auth_cred_ssl_client_cert_pw_t*)apr_pcalloc(pool, sizeof(*ret));
  const char *info = that->askQuestion(realm,
                                       _("client certificate passphrase: "),
                                       false, may_save ? true : false);
  if (info == NULL)
    return svn_error_create(SVN_ERR_RA_NOT_AUTHORIZED, NULL,
                            _("User canceled dialog"));
  ret->password = apr_pstrdup(pool, info);
  ret->may_save = that->m_maySave;
  *cred_p = ret;
  return SVN_NO_ERROR;
}

svn_error_t *
Prompter::plaintext_prompt(svn_boolean_t *may_save_plaintext,
                           const char *realmstring,
                           void *baton,
                           apr_pool_t *pool)
{
  Prompter *that = (Prompter *) baton;

  bool result = that->askYesNo(realmstring,
                               _("Store password unencrypted?"),
                               false);

  *may_save_plaintext = (result ? TRUE : FALSE);

  return SVN_NO_ERROR;
}

svn_error_t *
Prompter::plaintext_passphrase_prompt(svn_boolean_t *may_save_plaintext,
                                      const char *realmstring,
                                      void *baton,
                                      apr_pool_t *pool)
{
  Prompter *that = (Prompter *) baton;

  bool result = that->askYesNo(realmstring,
                               _("Store passphrase unencrypted?"),
                               false);

  *may_save_plaintext = (result ? TRUE : FALSE);

  return SVN_NO_ERROR;
}
