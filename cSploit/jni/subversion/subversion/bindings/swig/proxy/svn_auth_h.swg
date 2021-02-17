/* Proxy classes for svn_auth.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
%nodefault svn_auth_baton_t;
%nodefault svn_auth_iterstate_t;
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_auth_simple_provider_func_t,
  svn_auth_ssl_client_cert_pw_provider_func_t,
  svn_auth_simple_prompt_func_t,
  svn_auth_username_prompt_func_t,
  svn_auth_ssl_server_trust_prompt_func_t,
  svn_auth_ssl_client_cert_prompt_func_t,
  svn_auth_ssl_client_cert_pw_prompt_func_t,
  svn_auth_plaintext_prompt_func_t,
  svn_auth_plaintext_passphrase_prompt_func_t,
  svn_auth_gnome_keyring_unlock_prompt_func_t
};
%apply CALLABLE_CALLBACK * {
  svn_auth_simple_provider_func_t *,
  svn_auth_ssl_client_cert_pw_provider_func_t *,
  svn_auth_simple_prompt_func_t *,
  svn_auth_username_prompt_func_t *,
  svn_auth_ssl_server_trust_prompt_func_t *,
  svn_auth_ssl_client_cert_prompt_func_t *,
  svn_auth_ssl_client_cert_pw_prompt_func_t *,
  svn_auth_plaintext_prompt_func_t *,
  svn_auth_plaintext_passphrase_prompt_func_t *,
  svn_auth_gnome_keyring_unlock_prompt_func_t *
};
#endif

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *provider_baton,
  void *iter_baton,
  void *baton,
  void *prompt_baton
};
#endif

/* Includes */
%{
#include "svn_auth.h"
%}
%include svn_auth.h

/* Structure definitions */
#ifdef SWIGPYTHON
%opaque_proxy(svn_auth_baton_t);
%opaque_proxy(svn_auth_iterstate_t);
%proxy(svn_auth_provider_t);
%proxy(svn_auth_provider_object_t);
%proxy(svn_auth_cred_simple_t);
%proxy(svn_auth_cred_username_t);
%proxy(svn_auth_cred_ssl_client_cert_t);
%proxy(svn_auth_cred_ssl_client_cert_pw_t);
%proxy(svn_auth_ssl_server_cert_info_t);
%proxy(svn_auth_cred_ssl_server_trust_t);
#endif

/* Callbacks */

%inline %{
static svn_error_t * svn_auth_provider_invoke_first_credentials(
  svn_auth_provider_t * _obj, void **credentials, void **iter_baton, void *provider_baton, apr_hash_t *parameters, const char *realmstring, apr_pool_t *pool) {
  return (_obj->first_credentials)(credentials, iter_baton, provider_baton, parameters, realmstring, pool);
}

static svn_error_t * svn_auth_provider_invoke_next_credentials(
  svn_auth_provider_t * _obj, void **credentials, void *iter_baton, void *provider_baton, apr_hash_t *parameters, const char *realmstring, apr_pool_t *pool) {
  return (_obj->next_credentials)(credentials, iter_baton, provider_baton, parameters, realmstring, pool);
}

static svn_error_t * svn_auth_provider_invoke_save_credentials(
  svn_auth_provider_t * _obj, svn_boolean_t *saved, void *credentials, void *provider_baton, apr_hash_t *parameters, const char *realmstring, apr_pool_t *pool) {
  return (_obj->save_credentials)(saved, credentials, provider_baton, parameters, realmstring, pool);
}

static void svn_auth_invoke_simple_provider_func(
  svn_auth_simple_provider_func_t _obj, svn_auth_provider_object_t **provider, apr_pool_t *pool) {
  _obj(provider, pool);
}

static void svn_auth_invoke_ssl_client_cert_pw_provider_func(
  svn_auth_ssl_client_cert_pw_provider_func_t _obj, svn_auth_provider_object_t **provider, apr_pool_t *pool) {
  _obj(provider, pool);
}

static svn_error_t * svn_auth_invoke_simple_prompt_func(
  svn_auth_simple_prompt_func_t _obj, svn_auth_cred_simple_t **cred, void *baton, const char *realm, const char *username, svn_boolean_t may_save, apr_pool_t *pool) {
  return _obj(cred, baton, realm, username, may_save, pool);
}

static svn_error_t * svn_auth_invoke_username_prompt_func(
  svn_auth_username_prompt_func_t _obj, svn_auth_cred_username_t **cred, void *baton, const char *realm, svn_boolean_t may_save, apr_pool_t *pool) {
  return _obj(cred, baton, realm, may_save, pool);
}

static svn_error_t * svn_auth_invoke_ssl_server_trust_prompt_func(
  svn_auth_ssl_server_trust_prompt_func_t _obj, svn_auth_cred_ssl_server_trust_t **cred, void *baton, const char *realm, apr_uint32_t failures, const svn_auth_ssl_server_cert_info_t *cert_info, svn_boolean_t may_save, apr_pool_t *pool) {
  return _obj(cred, baton, realm, failures, cert_info, may_save, pool);
}

static svn_error_t * svn_auth_invoke_ssl_client_cert_prompt_func(
  svn_auth_ssl_client_cert_prompt_func_t _obj, svn_auth_cred_ssl_client_cert_t **cred, void *baton, const char *realm, svn_boolean_t may_save, apr_pool_t *pool) {
  return _obj(cred, baton, realm, may_save, pool);
}

static svn_error_t * svn_auth_invoke_ssl_client_cert_pw_prompt_func(
  svn_auth_ssl_client_cert_pw_prompt_func_t _obj, svn_auth_cred_ssl_client_cert_pw_t **cred, void *baton, const char *realm, svn_boolean_t may_save, apr_pool_t *pool) {
  return _obj(cred, baton, realm, may_save, pool);
}

static svn_error_t * svn_auth_invoke_plaintext_prompt_func(
  svn_auth_plaintext_prompt_func_t _obj, svn_boolean_t *may_save_plaintext, const char *realmstring, void *baton, apr_pool_t *pool) {
  return _obj(may_save_plaintext, realmstring, baton, pool);
}

static svn_error_t * svn_auth_invoke_plaintext_passphrase_prompt_func(
  svn_auth_plaintext_passphrase_prompt_func_t _obj, svn_boolean_t *may_save_plaintext, const char *realmstring, void *baton, apr_pool_t *pool) {
  return _obj(may_save_plaintext, realmstring, baton, pool);
}

%}

#ifdef SWIGPYTHON
%funcptr_member_proxy(svn_auth_provider_t, first_credentials, svn_auth_provider_invoke_first_credentials);
%funcptr_member_proxy(svn_auth_provider_t, next_credentials, svn_auth_provider_invoke_next_credentials);
%funcptr_member_proxy(svn_auth_provider_t, save_credentials, svn_auth_provider_invoke_save_credentials);
%funcptr_proxy(svn_auth_simple_provider_func_t, svn_auth_invoke_simple_provider_func);
%funcptr_proxy(svn_auth_ssl_client_cert_pw_provider_func_t, svn_auth_invoke_ssl_client_cert_pw_provider_func);
%funcptr_proxy(svn_auth_simple_prompt_func_t, svn_auth_invoke_simple_prompt_func);
%funcptr_proxy(svn_auth_username_prompt_func_t, svn_auth_invoke_username_prompt_func);
%funcptr_proxy(svn_auth_ssl_server_trust_prompt_func_t, svn_auth_invoke_ssl_server_trust_prompt_func);
%funcptr_proxy(svn_auth_ssl_client_cert_prompt_func_t, svn_auth_invoke_ssl_client_cert_prompt_func);
%funcptr_proxy(svn_auth_ssl_client_cert_pw_prompt_func_t, svn_auth_invoke_ssl_client_cert_pw_prompt_func);
%funcptr_proxy(svn_auth_plaintext_prompt_func_t, svn_auth_invoke_plaintext_prompt_func);
%funcptr_proxy(svn_auth_plaintext_passphrase_prompt_func_t, svn_auth_invoke_plaintext_passphrase_prompt_func);
%funcptr_proxy(svn_auth_gnome_keyring_unlock_prompt_func_t, svn_auth_invoke_gnome_keyring_unlock_prompt_func);

#endif
