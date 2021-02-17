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
 * @file ClientContext.cpp
 * @brief Implementation of the class ClientContext
 */

#include "svn_client.h"
#include "private/svn_wc_private.h"
#include "svn_private_config.h"

#include "ClientContext.h"
#include "JNIUtil.h"
#include "JNICriticalSection.h"

#include "Prompter.h"
#include "CreateJ.h"
#include "EnumMapper.h"
#include "CommitMessage.h"


ClientContext::ClientContext(jobject jsvnclient, SVN::Pool &pool)
    : m_prompter(NULL),
      m_cancelOperation(false)
{
    JNIEnv *env = JNIUtil::getEnv();

    /* Grab a global reference to the Java object embedded in the parent Java
       object. */
    static jfieldID ctxFieldID = 0;
    if (ctxFieldID == 0)
    {
        jclass clazz = env->GetObjectClass(jsvnclient);
        if (JNIUtil::isJavaExceptionThrown())
            return;

        ctxFieldID = env->GetFieldID(clazz, "clientContext",
                                "L"JAVA_PACKAGE"/SVNClient$ClientContext;");
        if (JNIUtil::isJavaExceptionThrown() || ctxFieldID == 0)
            return;

        env->DeleteLocalRef(clazz);
    }

    jobject jctx = env->GetObjectField(jsvnclient, ctxFieldID);
    if (JNIUtil::isJavaExceptionThrown())
        return;

    m_jctx = env->NewGlobalRef(jctx);
    if (JNIUtil::isJavaExceptionThrown())
        return;

    env->DeleteLocalRef(jctx);

    SVN_JNI_ERR(svn_client_create_context(&m_context, pool.getPool()),
                );

    /* Clear the wc_ctx as we don't want to maintain this unconditionally
       for compatibility reasons */
    SVN_JNI_ERR(svn_wc_context_destroy(m_context->wc_ctx),
                );
    m_context->wc_ctx = NULL;

    /* None of the following members change during the lifetime of
       this object. */
    m_context->notify_func = NULL;
    m_context->notify_baton = NULL;
    m_context->log_msg_func3 = CommitMessage::callback;
    m_context->log_msg_baton3 = NULL;
    m_context->cancel_func = checkCancel;
    m_context->cancel_baton = this;
    m_context->notify_func2= notify;
    m_context->notify_baton2 = m_jctx;
    m_context->progress_func = progress;
    m_context->progress_baton = m_jctx;
    m_context->conflict_func2 = resolve;
    m_context->conflict_baton2 = m_jctx;

    m_context->client_name = "javahl";
    m_pool = &pool;
}

ClientContext::~ClientContext()
{
    delete m_prompter;

    JNIEnv *env = JNIUtil::getEnv();
    env->DeleteGlobalRef(m_jctx);
}


/* Helper function to make sure that we don't keep dangling pointers in ctx.
   Note that this function might be called multiple times if getContext()
   is called on the same pool.
   
   The use of this function assumes a proper subpool behavior by its user,
   (read: SVNClient) usually per request.
 */
extern "C" {

struct clearctx_baton_t
{
  svn_client_ctx_t *ctx;
  svn_client_ctx_t *backup;
};

static apr_status_t clear_ctx_ptrs(void *ptr)
{
    clearctx_baton_t *bt = (clearctx_baton_t*)ptr;

    /* Reset all values to those before overwriting by getContext. */
    *bt->ctx = *bt->backup;

    return APR_SUCCESS;
}

};

svn_client_ctx_t *
ClientContext::getContext(CommitMessage *message, SVN::Pool &in_pool)
{
    apr_pool_t *pool = in_pool.getPool();
    svn_auth_baton_t *ab;
    svn_client_ctx_t *ctx = m_context;

    /* Make a temporary copy of ctx to restore at pool cleanup to avoid
       leaving references to dangling pointers.

       Note that this allows creating a stack of context changes if
       the function is invoked multiple times with different pools.
     */
    clearctx_baton_t *bt = (clearctx_baton_t *)apr_pcalloc(pool, sizeof(*bt));
    bt->ctx = ctx;
    bt->backup = (svn_client_ctx_t*)apr_pmemdup(pool, ctx, sizeof(*ctx));
    apr_pool_cleanup_register(in_pool.getPool(), bt, clear_ctx_ptrs,
                              clear_ctx_ptrs);


    if (!ctx->config)
      {
        const char *configDir = m_configDir.c_str();
        if (m_configDir.empty())
            configDir = NULL;
        SVN_JNI_ERR(svn_config_get_config(&(ctx->config), configDir,
                                          m_pool->getPool()),
                    NULL);

        bt->backup->config = ctx->config;
      }
    svn_config_t *config = (svn_config_t *) apr_hash_get(ctx->config,
                                                         SVN_CONFIG_CATEGORY_CONFIG,
                                                         APR_HASH_KEY_STRING);


    /* The whole list of registered providers */
    apr_array_header_t *providers;

    /* Populate the registered providers with the platform-specific providers */
    SVN_JNI_ERR(svn_auth_get_platform_specific_client_providers(&providers,
                                                                config,
                                                                pool),
                NULL);

    /* Use the prompter (if available) to prompt for password and cert
     * caching. */
    svn_auth_plaintext_prompt_func_t plaintext_prompt_func = NULL;
    void *plaintext_prompt_baton = NULL;
    svn_auth_plaintext_passphrase_prompt_func_t plaintext_passphrase_prompt_func;
    void *plaintext_passphrase_prompt_baton = NULL;

    if (m_prompter != NULL)
    {
        plaintext_prompt_func = Prompter::plaintext_prompt;
        plaintext_prompt_baton = m_prompter;
        plaintext_passphrase_prompt_func = Prompter::plaintext_passphrase_prompt;
        plaintext_passphrase_prompt_baton = m_prompter;
    }

    /* The main disk-caching auth providers, for both
     * 'username/password' creds and 'username' creds.  */
    svn_auth_provider_object_t *provider;

    svn_auth_get_simple_provider2(&provider, plaintext_prompt_func,
                                  plaintext_prompt_baton, pool);
    APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

    svn_auth_get_username_provider(&provider, pool);
    APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

    /* The server-cert, client-cert, and client-cert-password providers. */
    SVN_JNI_ERR(svn_auth_get_platform_specific_provider(&provider,
                                                        "windows",
                                                        "ssl_server_trust",
                                                        pool),
                NULL);

    if (provider)
        APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

    svn_auth_get_ssl_server_trust_file_provider(&provider, pool);
    APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;
    svn_auth_get_ssl_client_cert_file_provider(&provider, pool);
    APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;
    svn_auth_get_ssl_client_cert_pw_file_provider2(&provider,
                        plaintext_passphrase_prompt_func,
                        plaintext_passphrase_prompt_baton, pool);
    APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

    if (m_prompter != NULL)
    {
        /* Two basic prompt providers: username/password, and just username.*/
        provider = m_prompter->getProviderSimple(in_pool);

        APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

        provider = m_prompter->getProviderUsername(in_pool);
        APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

        /* Three ssl prompt providers, for server-certs, client-certs,
         * and client-cert-passphrases.  */
        provider = m_prompter->getProviderServerSSLTrust(in_pool);
        APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

        provider = m_prompter->getProviderClientSSL(in_pool);
        APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

        provider = m_prompter->getProviderClientSSLPassword(in_pool);
        APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;
    }

    /* Build an authentication baton to give to libsvn_client. */
    svn_auth_open(&ab, providers, pool);

    /* Place any default --username or --password credentials into the
     * auth_baton's run-time parameter hash.  ### Same with --no-auth-cache? */
    if (!m_userName.empty())
        svn_auth_set_parameter(ab, SVN_AUTH_PARAM_DEFAULT_USERNAME,
                               apr_pstrdup(in_pool.getPool(),
                                           m_userName.c_str()));
    if (!m_passWord.empty())
        svn_auth_set_parameter(ab, SVN_AUTH_PARAM_DEFAULT_PASSWORD,
                               apr_pstrdup(in_pool.getPool(),
                                           m_passWord.c_str()));
    /* Store where to retrieve authentication data? */
    if (!m_configDir.empty())
        svn_auth_set_parameter(ab, SVN_AUTH_PARAM_CONFIG_DIR,
                               apr_pstrdup(in_pool.getPool(),
                                           m_configDir.c_str()));

    ctx->auth_baton = ab;
    ctx->log_msg_baton3 = message;
    m_cancelOperation = false;

    SVN_JNI_ERR(svn_wc_context_create(&ctx->wc_ctx, NULL,
                                      in_pool.getPool(), in_pool.getPool()),
                NULL);

    return ctx;
}

void
ClientContext::username(const char *pi_username)
{
    m_userName = (pi_username == NULL ? "" : pi_username);
}

void
ClientContext::password(const char *pi_password)
{
    m_passWord = (pi_password == NULL ? "" : pi_password);
}

void
ClientContext::setPrompt(Prompter *prompter)
{
    delete m_prompter;
    m_prompter = prompter;
}

void
ClientContext::setConfigDirectory(const char *configDir)
{
    // A change to the config directory may necessitate creation of
    // the config templates.
    SVN::Pool requestPool;
    SVN_JNI_ERR(svn_config_ensure(configDir, requestPool.getPool()), );

    m_configDir = (configDir == NULL ? "" : configDir);
    m_context->config = NULL;
}

const char *
ClientContext::getConfigDirectory() const
{
    return m_configDir.c_str();
}

void
ClientContext::cancelOperation()
{
    m_cancelOperation = true;
}

svn_error_t *
ClientContext::checkCancel(void *cancelBaton)
{
    ClientContext *that = (ClientContext *)cancelBaton;
    if (that->m_cancelOperation)
        return svn_error_create(SVN_ERR_CANCELLED, NULL,
                                _("Operation cancelled"));
    else
        return SVN_NO_ERROR;
}

void
ClientContext::notify(void *baton,
                      const svn_wc_notify_t *notify,
                      apr_pool_t *pool)
{
  jobject jctx = (jobject) baton;
  JNIEnv *env = JNIUtil::getEnv();

  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->GetObjectClass(jctx);
      if (JNIUtil::isJavaExceptionThrown())
        return;

      mid = env->GetMethodID(clazz, "onNotify",
                             "(L"JAVA_PACKAGE"/ClientNotifyInformation;)V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        return;

      env->DeleteLocalRef(clazz);
    }

  jobject jInfo = CreateJ::ClientNotifyInformation(notify);
  if (JNIUtil::isJavaExceptionThrown())
    return;

  env->CallVoidMethod(jctx, mid, jInfo);
  if (JNIUtil::isJavaExceptionThrown())
    return;

  env->DeleteLocalRef(jInfo);
}

void
ClientContext::progress(apr_off_t progressVal, apr_off_t total,
                        void *baton, apr_pool_t *pool)
{
  jobject jctx = (jobject) baton;
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return;

  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->GetObjectClass(jctx);
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN_NOTHING();

      mid = env->GetMethodID(clazz, "onProgress",
                             "(L"JAVA_PACKAGE"/ProgressEvent;)V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN_NOTHING();
    }

  static jmethodID midCT = 0;
  jclass clazz = env->FindClass(JAVA_PACKAGE"/ProgressEvent");
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();

  if (midCT == 0)
    {
      midCT = env->GetMethodID(clazz, "<init>", "(JJ)V");
      if (JNIUtil::isJavaExceptionThrown() || midCT == 0)
        POP_AND_RETURN_NOTHING();
    }

  // Call the Java method.
  jobject jevent = env->NewObject(clazz, midCT,
                                  (jlong) progressVal, (jlong) total);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NOTHING();

  env->CallVoidMethod(jctx, mid, jevent);

  POP_AND_RETURN_NOTHING();
}

svn_error_t *
ClientContext::resolve(svn_wc_conflict_result_t **result,
                       const svn_wc_conflict_description2_t *desc,
                       void *baton,
                       apr_pool_t *result_pool,
                       apr_pool_t *scratch_pool)
{
  jobject jctx = (jobject) baton;
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->GetObjectClass(jctx);
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);

      mid = env->GetMethodID(clazz, "resolve",
                             "(L"JAVA_PACKAGE"/ConflictDescriptor;)"
                             "L"JAVA_PACKAGE"/ConflictResult;");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  // Create an instance of the conflict descriptor.
  jobject jdesc = CreateJ::ConflictDescriptor(desc);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  // Invoke the Java conflict resolver callback method using the descriptor.
  jobject jresult = env->CallObjectMethod(jctx, mid, jdesc);
  if (JNIUtil::isJavaExceptionThrown())
    {
      // If an exception is thrown by our conflict resolver, remove it
      // from the JNI env, and convert it into a Subversion error.
      SVN::Pool tmpPool(scratch_pool);
      const char *msg = JNIUtil::thrownExceptionToCString(tmpPool);
      svn_error_t *err = svn_error_create(SVN_ERR_WC_CONFLICT_RESOLVER_FAILURE,
                                          NULL, msg);
      env->PopLocalFrame(NULL);
      return err;
    }

  *result = javaResultToC(jresult, result_pool);
  if (*result == NULL)
    {
      // Unable to convert the result into a C representation.
      env->PopLocalFrame(NULL);
      return svn_error_create(SVN_ERR_WC_CONFLICT_RESOLVER_FAILURE, NULL, NULL);
    }

  env->PopLocalFrame(NULL);
  return SVN_NO_ERROR;
}

svn_wc_conflict_result_t *
ClientContext::javaResultToC(jobject jresult, apr_pool_t *pool)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  static jmethodID getChoice = 0;
  static jmethodID getMergedPath = 0;

  jclass clazz = NULL;
  if (getChoice == 0 || getMergedPath == 0)
    {
      clazz = env->FindClass(JAVA_PACKAGE "/ConflictResult");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN_NULL;
    }

  if (getChoice == 0)
    {
      getChoice = env->GetMethodID(clazz, "getChoice",
                                   "()L"JAVA_PACKAGE"/ConflictResult$Choice;");
      if (JNIUtil::isJavaExceptionThrown() || getChoice == 0)
        POP_AND_RETURN_NULL;
    }
  if (getMergedPath == 0)
    {
      getMergedPath = env->GetMethodID(clazz, "getMergedPath",
                                       "()Ljava/lang/String;");
      if (JNIUtil::isJavaExceptionThrown() || getMergedPath == 0)
        POP_AND_RETURN_NULL;
    }

  jobject jchoice = env->CallObjectMethod(jresult, getChoice);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jstring jmergedPath = (jstring) env->CallObjectMethod(jresult, getMergedPath);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  JNIStringHolder mergedPath(jmergedPath);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  svn_wc_conflict_result_t *result =
         svn_wc_create_conflict_result(EnumMapper::toConflictChoice(jchoice),
                                       mergedPath.pstrdup(pool),
                                       pool);

  env->PopLocalFrame(NULL);
  return result;
}
