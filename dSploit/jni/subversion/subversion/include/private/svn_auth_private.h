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
 * @file svn_auth_private.h
 * @brief Subversion's authentication system - Internal routines
 */

#ifndef SVN_AUTH_PRIVATE_H
#define SVN_AUTH_PRIVATE_H

#include <apr_pools.h>
#include <apr_hash.h>

#include "svn_types.h"
#include "svn_error.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* If you add a password type for a provider which stores
 * passwords on disk in encrypted form, remember to update
 * svn_auth__simple_save_creds_helper. Otherwise it will be
 * assumed that your provider stores passwords in plaintext. */
#define SVN_AUTH__SIMPLE_PASSWORD_TYPE             "simple"
#define SVN_AUTH__WINCRYPT_PASSWORD_TYPE           "wincrypt"
#define SVN_AUTH__KEYCHAIN_PASSWORD_TYPE           "keychain"
#define SVN_AUTH__KWALLET_PASSWORD_TYPE            "kwallet"
#define SVN_AUTH__GNOME_KEYRING_PASSWORD_TYPE      "gnome-keyring"

/* A function that stores in *PASSWORD (potentially after decrypting it)
   the user's password.  It might be obtained directly from CREDS, or
   from an external store, using REALMSTRING and USERNAME as keys.
   (The behavior is undefined if REALMSTRING or USERNAME are NULL.)
   If NON_INTERACTIVE is set, the user must not be involved in the
   retrieval process.  Set *DONE to TRUE if a password was stored
   in *PASSWORD, to FALSE otherwise. POOL is used for any necessary
   allocation. */
typedef svn_error_t * (*svn_auth__password_get_t)
  (svn_boolean_t *done,
   const char **password,
   apr_hash_t *creds,
   const char *realmstring,
   const char *username,
   apr_hash_t *parameters,
   svn_boolean_t non_interactive,
   apr_pool_t *pool);

/* A function that stores PASSWORD (or some encrypted version thereof)
   either directly in CREDS, or externally using REALMSTRING and USERNAME
   as keys into the external store.  If NON_INTERACTIVE is set, the user
   must not be involved in the storage process. Set *DONE to TRUE if the
   password was store, to FALSE otherwise. POOL is used for any necessary
   allocation. */
typedef svn_error_t * (*svn_auth__password_set_t)
  (svn_boolean_t *done,
   apr_hash_t *creds,
   const char *realmstring,
   const char *username,
   const char *password,
   apr_hash_t *parameters,
   svn_boolean_t non_interactive,
   apr_pool_t *pool);

/* Common implementation for simple_first_creds and
   windows_simple_first_creds. Uses PARAMETERS, REALMSTRING and the
   simple auth provider's username and password cache to fill a set of
   CREDENTIALS. PASSWORD_GET is used to obtain the password value.
   PASSTYPE identifies the type of the cached password. CREDENTIALS are
   allocated from POOL. */
svn_error_t *
svn_auth__simple_first_creds_helper(void **credentials,
                                    void **iter_baton,
                                    void *provider_baton,
                                    apr_hash_t *parameters,
                                    const char *realmstring,
                                    svn_auth__password_get_t password_get,
                                    const char *passtype,
                                    apr_pool_t *pool);

/* Common implementation for simple_save_creds and
   windows_simple_save_creds. Uses PARAMETERS and REALMSTRING to save
   a set of CREDENTIALS to the simple auth provider's username and
   password cache. PASSWORD_SET is used to store the password.
   PASSTYPE identifies the type of the cached password. Allocates from POOL. */
svn_error_t *
svn_auth__simple_save_creds_helper(svn_boolean_t *saved,
                                   void *credentials,
                                   void *provider_baton,
                                   apr_hash_t *parameters,
                                   const char *realmstring,
                                   svn_auth__password_set_t password_set,
                                   const char *passtype,
                                   apr_pool_t *pool);

/* Implementation of svn_auth__password_get_t that retrieves
   the plaintext password from CREDS when USERNAME matches the stored
   credentials. */
svn_error_t *
svn_auth__simple_password_get(svn_boolean_t *done,
                              const char **password,
                              apr_hash_t *creds,
                              const char *realmstring,
                              const char *username,
                              apr_hash_t *parameters,
                              svn_boolean_t non_interactive,
                              apr_pool_t *pool);

/* Implementation of svn_auth__password_set_t that stores
   the plaintext password in CREDS. */
svn_error_t *
svn_auth__simple_password_set(svn_boolean_t *done,
                              apr_hash_t *creds,
                              const char *realmstring,
                              const char *username,
                              const char *password,
                              apr_hash_t *parameters,
                              svn_boolean_t non_interactive,
                              apr_pool_t *pool);


/* Common implementation for ssl_client_cert_pw_file_first_credentials.
   Uses PARAMETERS, REALMSTRING and the ssl client passphrase auth provider's
   passphrase cache to fill the CREDENTIALS. PASSPHRASE_GET is used to obtain
   the passphrase value. PASSTYPE identifies the type of the cached passphrase.
   CREDENTIALS are allocated from POOL. */
svn_error_t *
svn_auth__ssl_client_cert_pw_file_first_creds_helper
  (void **credentials,
   void **iter_baton,
   void *provider_baton,
   apr_hash_t *parameters,
   const char *realmstring,
   svn_auth__password_get_t passphrase_get,
   const char *passtype,
   apr_pool_t *pool);

/* Common implementation for ssl_client_cert_pw_file_save_credentials and
   windows_ssl_client_cert_pw_file_save_credentials. Uses PARAMETERS and
   REALMSTRING to save a set of CREDENTIALS to the ssl client cert auth
   provider's passphrase cache. PASSPHRASE_SET is used to store the
   passphrase. PASSTYPE identifies the type of the cached passphrase.
   Allocates from POOL. */
svn_error_t *
svn_auth__ssl_client_cert_pw_file_save_creds_helper
  (svn_boolean_t *saved,
   void *credentials,
   void *provider_baton,
   apr_hash_t *parameters,
   const char *realmstring,
   svn_auth__password_set_t passphrase_set,
   const char *passtype,
   apr_pool_t *pool);


/* This implements the svn_auth__password_get_t interface.
   Set **PASSPHRASE to the plaintext passphrase retrieved from CREDS;
   ignore other parameters. */
svn_error_t *
svn_auth__ssl_client_cert_pw_get(svn_boolean_t *done,
                                 const char **passphrase,
                                 apr_hash_t *creds,
                                 const char *realmstring,
                                 const char *username,
                                 apr_hash_t *parameters,
                                 svn_boolean_t non_interactive,
                                 apr_pool_t *pool);

/* This implements the svn_auth__password_set_t interface.
   Store PASSPHRASE in CREDS; ignore other parameters. */
svn_error_t *
svn_auth__ssl_client_cert_pw_set(svn_boolean_t *done,
                                 apr_hash_t *creds,
                                 const char *realmstring,
                                 const char *username,
                                 const char *passphrase,
                                 apr_hash_t *parameters,
                                 svn_boolean_t non_interactive,
                                 apr_pool_t *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_AUTH_PRIVATE_H */
