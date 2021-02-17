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
 * @file svn_config.h
 * @brief Accessing SVN configuration files.
 */



#ifndef SVN_CONFIG_H
#define SVN_CONFIG_H

#include <apr.h>        /* for apr_int64_t */
#include <apr_pools.h>  /* for apr_pool_t */
#include <apr_hash.h>   /* for apr_hash_t */

#include "svn_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**************************************************************************
 ***                                                                    ***
 ***  For a description of the SVN configuration file syntax, see       ***
 ***  your ~/.subversion/README, which is written out automatically by  ***
 ***  svn_config_ensure().                                              ***
 ***                                                                    ***
 **************************************************************************/


/** Opaque structure describing a set of configuration options. */
typedef struct svn_config_t svn_config_t;


/*** Configuration Defines ***/

/**
 * @name Client configuration files strings
 * Strings for the names of files, sections, and options in the
 * client configuration files.
 * @{
 */
#define SVN_CONFIG_CATEGORY_SERVERS        "servers"
#define SVN_CONFIG_SECTION_GROUPS               "groups"
#define SVN_CONFIG_SECTION_GLOBAL               "global"
#define SVN_CONFIG_OPTION_HTTP_PROXY_HOST           "http-proxy-host"
#define SVN_CONFIG_OPTION_HTTP_PROXY_PORT           "http-proxy-port"
#define SVN_CONFIG_OPTION_HTTP_PROXY_USERNAME       "http-proxy-username"
#define SVN_CONFIG_OPTION_HTTP_PROXY_PASSWORD       "http-proxy-password"
#define SVN_CONFIG_OPTION_HTTP_PROXY_EXCEPTIONS     "http-proxy-exceptions"
#define SVN_CONFIG_OPTION_HTTP_TIMEOUT              "http-timeout"
#define SVN_CONFIG_OPTION_HTTP_COMPRESSION          "http-compression"
#define SVN_CONFIG_OPTION_NEON_DEBUG_MASK           "neon-debug-mask"
#define SVN_CONFIG_OPTION_HTTP_AUTH_TYPES           "http-auth-types"
#define SVN_CONFIG_OPTION_SSL_AUTHORITY_FILES       "ssl-authority-files"
#define SVN_CONFIG_OPTION_SSL_TRUST_DEFAULT_CA      "ssl-trust-default-ca"
#define SVN_CONFIG_OPTION_SSL_CLIENT_CERT_FILE      "ssl-client-cert-file"
#define SVN_CONFIG_OPTION_SSL_CLIENT_CERT_PASSWORD  "ssl-client-cert-password"
#define SVN_CONFIG_OPTION_SSL_PKCS11_PROVIDER       "ssl-pkcs11-provider"
#define SVN_CONFIG_OPTION_HTTP_LIBRARY              "http-library"
#define SVN_CONFIG_OPTION_STORE_PASSWORDS           "store-passwords"
#define SVN_CONFIG_OPTION_STORE_PLAINTEXT_PASSWORDS "store-plaintext-passwords"
#define SVN_CONFIG_OPTION_STORE_AUTH_CREDS          "store-auth-creds"
#define SVN_CONFIG_OPTION_STORE_SSL_CLIENT_CERT_PP  "store-ssl-client-cert-pp"
#define SVN_CONFIG_OPTION_STORE_SSL_CLIENT_CERT_PP_PLAINTEXT \
                                          "store-ssl-client-cert-pp-plaintext"
#define SVN_CONFIG_OPTION_USERNAME                  "username"

#define SVN_CONFIG_CATEGORY_CONFIG          "config"
#define SVN_CONFIG_SECTION_AUTH                 "auth"
#define SVN_CONFIG_OPTION_PASSWORD_STORES           "password-stores"
#define SVN_CONFIG_OPTION_KWALLET_WALLET            "kwallet-wallet"
#define SVN_CONFIG_OPTION_KWALLET_SVN_APPLICATION_NAME_WITH_PID "kwallet-svn-application-name-with-pid"
/* The majority of options of the "auth" section
 * has been moved to SVN_CONFIG_CATEGORY_SERVERS. */
#define SVN_CONFIG_SECTION_HELPERS              "helpers"
#define SVN_CONFIG_OPTION_EDITOR_CMD                "editor-cmd"
#define SVN_CONFIG_OPTION_DIFF_CMD                  "diff-cmd"
/** @since New in 1.7. */
#define SVN_CONFIG_OPTION_DIFF_EXTENSIONS           "diff-extensions"
#define SVN_CONFIG_OPTION_DIFF3_CMD                 "diff3-cmd"
#define SVN_CONFIG_OPTION_DIFF3_HAS_PROGRAM_ARG     "diff3-has-program-arg"
#define SVN_CONFIG_OPTION_MERGE_TOOL_CMD            "merge-tool-cmd"
#define SVN_CONFIG_SECTION_MISCELLANY           "miscellany"
#define SVN_CONFIG_OPTION_GLOBAL_IGNORES            "global-ignores"
#define SVN_CONFIG_OPTION_LOG_ENCODING              "log-encoding"
#define SVN_CONFIG_OPTION_USE_COMMIT_TIMES          "use-commit-times"
#define SVN_CONFIG_OPTION_TEMPLATE_ROOT             "template-root"
#define SVN_CONFIG_OPTION_ENABLE_AUTO_PROPS         "enable-auto-props"
#define SVN_CONFIG_OPTION_NO_UNLOCK                 "no-unlock"
#define SVN_CONFIG_OPTION_MIMETYPES_FILE            "mime-types-file"
#define SVN_CONFIG_OPTION_PRESERVED_CF_EXTS         "preserved-conflict-file-exts"
#define SVN_CONFIG_OPTION_INTERACTIVE_CONFLICTS     "interactive-conflicts"
#define SVN_CONFIG_OPTION_MEMORY_CACHE_SIZE         "memory-cache-size"
#define SVN_CONFIG_SECTION_TUNNELS              "tunnels"
#define SVN_CONFIG_SECTION_AUTO_PROPS           "auto-props"
/** @} */

/** @name Repository conf directory configuration files strings
 * Strings for the names of sections and options in the
 * repository conf directory configuration files.
 * @{
 */
/* For repository svnserve.conf files */
#define SVN_CONFIG_SECTION_GENERAL              "general"
#define SVN_CONFIG_OPTION_ANON_ACCESS               "anon-access"
#define SVN_CONFIG_OPTION_AUTH_ACCESS               "auth-access"
#define SVN_CONFIG_OPTION_PASSWORD_DB               "password-db"
#define SVN_CONFIG_OPTION_REALM                     "realm"
#define SVN_CONFIG_OPTION_AUTHZ_DB                  "authz-db"
/** @since New in 1.7. */
#define SVN_CONFIG_OPTION_FORCE_USERNAME_CASE       "force-username-case"
#define SVN_CONFIG_SECTION_SASL                 "sasl"
#define SVN_CONFIG_OPTION_USE_SASL                  "use-sasl"
#define SVN_CONFIG_OPTION_MIN_SSF                   "min-encryption"
#define SVN_CONFIG_OPTION_MAX_SSF                   "max-encryption"

/* For repository password database */
#define SVN_CONFIG_SECTION_USERS                "users"
/** @} */

/*** Configuration Default Values ***/

/* '*' matches leading dots, e.g. '*.rej' matches '.foo.rej'. */
/* We want this to be printed on two lines in the generated config file,
 * but we don't want the # character to end up in the variable.
 */
#define SVN_CONFIG__DEFAULT_GLOBAL_IGNORES_LINE_1 \
  "*.o *.lo *.la *.al .libs *.so *.so.[0-9]* *.a *.pyc *.pyo"
#define SVN_CONFIG__DEFAULT_GLOBAL_IGNORES_LINE_2 \
  "*.rej *~ #*# .#* .*.swp .DS_Store"

#define SVN_CONFIG_DEFAULT_GLOBAL_IGNORES \
  SVN_CONFIG__DEFAULT_GLOBAL_IGNORES_LINE_1 " " \
  SVN_CONFIG__DEFAULT_GLOBAL_IGNORES_LINE_2

#define SVN_CONFIG_TRUE  "TRUE"
#define SVN_CONFIG_FALSE "FALSE"
#define SVN_CONFIG_ASK   "ASK"

/* Default values for some options. Should be passed as default values
 * to svn_config_get and friends, instead of hard-coding the defaults in
 * multiple places. */
#define SVN_CONFIG_DEFAULT_OPTION_STORE_PASSWORDS            TRUE
#define SVN_CONFIG_DEFAULT_OPTION_STORE_PLAINTEXT_PASSWORDS  SVN_CONFIG_ASK
#define SVN_CONFIG_DEFAULT_OPTION_STORE_AUTH_CREDS           TRUE
#define SVN_CONFIG_DEFAULT_OPTION_STORE_SSL_CLIENT_CERT_PP   TRUE
#define SVN_CONFIG_DEFAULT_OPTION_STORE_SSL_CLIENT_CERT_PP_PLAINTEXT \
                                                             SVN_CONFIG_ASK

/** Read configuration information from the standard sources and merge it
 * into the hash @a *cfg_hash.  If @a config_dir is not NULL it specifies a
 * directory from which to read the configuration files, overriding all
 * other sources.  Otherwise, first read any system-wide configurations
 * (from a file or from the registry), then merge in personal
 * configurations (again from file or registry).  The hash and all its data
 * are allocated in @a pool.
 *
 * @a *cfg_hash is a hash whose keys are @c const char * configuration
 * categories (@c SVN_CONFIG_CATEGORY_SERVERS,
 * @c SVN_CONFIG_CATEGORY_CONFIG, etc.) and whose values are the @c
 * svn_config_t * items representing the configuration values for that
 * category.
 */
svn_error_t *
svn_config_get_config(apr_hash_t **cfg_hash,
                      const char *config_dir,
                      apr_pool_t *pool);


/** Set @a *cfgp to an empty @c svn_config_t structure,
 * allocated in @a result_pool.
 *
 * Pass TRUE to @a section_names_case_sensitive if
 * section names are to be populated case sensitively.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_config_create(svn_config_t **cfgp,
                  svn_boolean_t section_names_case_sensitive,
                  apr_pool_t *result_pool);

/** Read configuration data from @a file (a file or registry path) into
 * @a *cfgp, allocated in @a pool.
 *
 * If @a file does not exist, then if @a must_exist, return an error,
 * otherwise return an empty @c svn_config_t.
 *
 * If @a section_names_case_sensitive is TRUE, populate section name hashes
 * case sensitively, except for the default SVN_CONFIG__DEFAULT_SECTION.
 *
 * @since New in 1.7.
 */

svn_error_t *
svn_config_read2(svn_config_t **cfgp,
                 const char *file,
                 svn_boolean_t must_exist,
                 svn_boolean_t section_names_case_sensitive,
                 apr_pool_t *pool);

/** Similar to svn_config_read2, but always passes FALSE to
 * section_names_case_sensitive.
 *
 * @deprecated Provided for backward compatibility with 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_config_read(svn_config_t **cfgp,
                const char *file,
                svn_boolean_t must_exist,
                apr_pool_t *pool);

/** Like svn_config_read(), but merges the configuration data from @a file
 * (a file or registry path) into @a *cfg, which was previously returned
 * from svn_config_read().  This function invalidates all value
 * expansions in @a cfg, so that the next svn_config_get() takes the
 * modifications into account.
 */
svn_error_t *
svn_config_merge(svn_config_t *cfg,
                 const char *file,
                 svn_boolean_t must_exist);


/** Find the value of a (@a section, @a option) pair in @a cfg, set @a
 * *valuep to the value.
 *
 * If @a cfg is @c NULL, just sets @a *valuep to @a default_value. If
 * the value does not exist, expand and return @a default_value. @a
 * default_value can be NULL.
 *
 * The returned value will be valid at least until the next call to
 * svn_config_get(), or for the lifetime of @a default_value. It is
 * safest to consume the returned value immediately.
 *
 * This function may change @a cfg by expanding option values.
 */
void
svn_config_get(svn_config_t *cfg,
               const char **valuep,
               const char *section,
               const char *option,
               const char *default_value);

/** Add or replace the value of a (@a section, @a option) pair in @a cfg with
 * @a value.
 *
 * This function invalidates all value expansions in @a cfg.
 *
 * To remove an option, pass NULL for the @a value.
 */
void
svn_config_set(svn_config_t *cfg,
               const char *section,
               const char *option,
               const char *value);

/** Like svn_config_get(), but for boolean values.
 *
 * Parses the option as a boolean value. The recognized representations
 * are 'TRUE'/'FALSE', 'yes'/'no', 'on'/'off', '1'/'0'; case does not
 * matter. Returns an error if the option doesn't contain a known string.
 */
svn_error_t *
svn_config_get_bool(svn_config_t *cfg,
                    svn_boolean_t *valuep,
                    const char *section,
                    const char *option,
                    svn_boolean_t default_value);

/** Like svn_config_set(), but for boolean values.
 *
 * Sets the option to 'TRUE'/'FALSE', depending on @a value.
 */
void
svn_config_set_bool(svn_config_t *cfg,
                    const char *section,
                    const char *option,
                    svn_boolean_t value);

/** Like svn_config_get(), but only for yes/no/ask values.
 *
 * Parse @a option in @a section and set @a *valuep to one of
 * SVN_CONFIG_TRUE, SVN_CONFIG_FALSE, or SVN_CONFIG_ASK.  If there is
 * no setting for @a option, then parse @a default_value and set
 * @a *valuep accordingly.  If @a default_value is NULL, the result is
 * undefined, and may be an error; we recommend that you pass one of
 * SVN_CONFIG_TRUE, SVN_CONFIG_FALSE, or SVN_CONFIG_ASK for @a default value.
 *
 * Valid representations are (at least) "true"/"false", "yes"/"no",
 * "on"/"off", "1"/"0", and "ask"; they are case-insensitive.  Return
 * an SVN_ERR_BAD_CONFIG_VALUE error if either @a default_value or
 * @a option's value is not a valid representation.
 *
 * @since New in 1.6.
 */
svn_error_t *
svn_config_get_yes_no_ask(svn_config_t *cfg,
                          const char **valuep,
                          const char *section,
                          const char *option,
                          const char* default_value);

/** Similar to @c svn_config_section_enumerator2_t, but is not
 * provided with a memory pool argument.
 *
 * See svn_config_enumerate_sections() for the details of this type.
 *
 * @deprecated Provided for backwards compatibility with the 1.2 API.
 */
typedef svn_boolean_t (*svn_config_section_enumerator_t)(const char *name,
                                                         void *baton);

/** Similar to svn_config_enumerate_sections2(), but uses a memory pool of
 * @a cfg instead of one that is explicitly provided.
 *
 * @deprecated Provided for backwards compatibility with the 1.2 API.
 */
SVN_DEPRECATED
int
svn_config_enumerate_sections(svn_config_t *cfg,
                              svn_config_section_enumerator_t callback,
                              void *baton);

/** A callback function used in enumerating config sections.
 *
 * See svn_config_enumerate_sections2() for the details of this type.
 *
 * @since New in 1.3.
 */
typedef svn_boolean_t (*svn_config_section_enumerator2_t)(const char *name,
                                                          void *baton,
                                                          apr_pool_t *pool);

/** Enumerate the sections, passing @a baton and the current section's name
 * to @a callback.  Continue the enumeration if @a callback returns @c TRUE.
 * Return the number of times @a callback was called.
 *
 * ### See kff's comment to svn_config_enumerate2().  It applies to this
 * function, too. ###
 *
 * @a callback's @a name parameter is only valid for the duration of the call.
 *
 * @since New in 1.3.
 */
int
svn_config_enumerate_sections2(svn_config_t *cfg,
                               svn_config_section_enumerator2_t callback,
                               void *baton, apr_pool_t *pool);

/** Similar to @c svn_config_enumerator2_t, but is not
 * provided with a memory pool argument.
 * See svn_config_enumerate() for the details of this type.
 *
 * @deprecated Provided for backwards compatibility with the 1.2 API.
 */
typedef svn_boolean_t (*svn_config_enumerator_t)(const char *name,
                                                 const char *value,
                                                 void *baton);

/** Similar to svn_config_enumerate2(), but uses a memory pool of
 * @a cfg instead of one that is explicitly provided.
 *
 * @deprecated Provided for backwards compatibility with the 1.2 API.
 */
SVN_DEPRECATED
int
svn_config_enumerate(svn_config_t *cfg,
                     const char *section,
                     svn_config_enumerator_t callback,
                     void *baton);


/** A callback function used in enumerating config options.
 *
 * See svn_config_enumerate2() for the details of this type.
 *
 * @since New in 1.3.
 */
typedef svn_boolean_t (*svn_config_enumerator2_t)(const char *name,
                                                  const char *value,
                                                  void *baton,
                                                  apr_pool_t *pool);

/** Enumerate the options in @a section, passing @a baton and the current
 * option's name and value to @a callback.  Continue the enumeration if
 * @a callback returns @c TRUE.  Return the number of times @a callback
 * was called.
 *
 * ### kff asks: A more usual interface is to continue enumerating
 *     while @a callback does not return error, and if @a callback does
 *     return error, to return the same error (or a wrapping of it)
 *     from svn_config_enumerate().  What's the use case for
 *     svn_config_enumerate()?  Is it more likely to need to break out
 *     of an enumeration early, with no error, than an invocation of
 *     @a callback is likely to need to return an error? ###
 *
 * @a callback's @a name and @a value parameters are only valid for the
 * duration of the call.
 *
 * @since New in 1.3.
 */
int
svn_config_enumerate2(svn_config_t *cfg,
                      const char *section,
                      svn_config_enumerator2_t callback,
                      void *baton,
                      apr_pool_t *pool);

/**
 * Return @c TRUE if @a section exists in @a cfg, @c FALSE otherwise.
 *
 * @since New in 1.4.
 */
svn_boolean_t
svn_config_has_section(svn_config_t *cfg,
                       const char *section);

/** Enumerate the group @a master_section in @a cfg.  Each variable
 * value is interpreted as a list of glob patterns (separated by comma
 * and optional whitespace).  Return the name of the first variable
 * whose value matches @a key, or @c NULL if no variable matches.
 */
const char *
svn_config_find_group(svn_config_t *cfg,
                      const char *key,
                      const char *master_section,
                      apr_pool_t *pool);

/** Retrieve value corresponding to @a option_name in @a cfg, or
 *  return @a default_value if none is found.
 *
 *  The config will first be checked for a default.
 *  If @a server_group is not @c NULL, the config will also be checked
 *  for an override in a server group,
 *
 */
const char *
svn_config_get_server_setting(svn_config_t *cfg,
                              const char* server_group,
                              const char* option_name,
                              const char* default_value);

/** Retrieve value into @a result_value corresponding to @a option_name for a
 *  given @a server_group in @a cfg, or return @a default_value if none is
 *  found.
 *
 *  The config will first be checked for a default, then will be checked for
 *  an override in a server group. If the value found is not a valid integer,
 *  a @c svn_error_t* will be returned.
 */
svn_error_t *
svn_config_get_server_setting_int(svn_config_t *cfg,
                                  const char *server_group,
                                  const char *option_name,
                                  apr_int64_t default_value,
                                  apr_int64_t *result_value,
                                  apr_pool_t *pool);


/** Set @a *valuep according to @a option_name for a given
 * @a  server_group in @a cfg, or set to @a default_value if no value is
 * specified.
 *
 * Check first a default, then for an override in a server group.  If
 * a value is found but is not a valid boolean, return an
 * SVN_ERR_BAD_CONFIG_VALUE error.
 *
 * @since New in 1.6.
 */
svn_error_t *
svn_config_get_server_setting_bool(svn_config_t *cfg,
                                   svn_boolean_t *valuep,
                                   const char *server_group,
                                   const char *option_name,
                                   svn_boolean_t default_value);



/** Try to ensure that the user's ~/.subversion/ area exists, and create
 * no-op template files for any absent config files.  Use @a pool for any
 * temporary allocation.  If @a config_dir is not @c NULL it specifies a
 * directory from which to read the config overriding all other sources.
 *
 * Don't error if something exists but is the wrong kind (for example,
 * ~/.subversion exists but is a file, or ~/.subversion/servers exists
 * but is a directory).
 *
 * Also don't error if trying to create something and failing -- it's
 * okay for the config area or its contents not to be created.
 * However, if creating a config template file succeeds, return an
 * error if unable to initialize its contents.
 */
svn_error_t *
svn_config_ensure(const char *config_dir,
                  apr_pool_t *pool);




/** Accessing cached authentication data in the user config area.
 *
 * @defgroup cached_authentication_data Cached authentication data
 * @{
 */


/** A hash-key pointing to a realmstring.  Every file containing
 * authentication data should have this key.
 */
#define SVN_CONFIG_REALMSTRING_KEY  "svn:realmstring"

/** Use @a cred_kind and @a realmstring to locate a file within the
 * ~/.subversion/auth/ area.  If the file exists, initialize @a *hash
 * and load the file contents into the hash, using @a pool.  If the
 * file doesn't exist, set @a *hash to NULL.
 *
 * If @a config_dir is not NULL it specifies a directory from which to
 * read the config overriding all other sources.
 *
 * Besides containing the original credential fields, the hash will
 * also contain @c SVN_CONFIG_REALMSTRING_KEY.  The caller can examine
 * this value as a sanity-check that the correct file was loaded.
 *
 * The hashtable will contain <tt>const char *</tt> keys and
 * <tt>svn_string_t *</tt> values.
 */
svn_error_t *
svn_config_read_auth_data(apr_hash_t **hash,
                          const char *cred_kind,
                          const char *realmstring,
                          const char *config_dir,
                          apr_pool_t *pool);

/** Use @a cred_kind and @a realmstring to create or overwrite a file
 * within the ~/.subversion/auth/ area.  Write the contents of @a hash into
 * the file.  If @a config_dir is not NULL it specifies a directory to read
 * the config overriding all other sources.
 *
 * Also, add @a realmstring to the file, with key @c
 * SVN_CONFIG_REALMSTRING_KEY.  This allows programs (or users) to
 * verify exactly which set credentials live within the file.
 *
 * The hashtable must contain <tt>const char *</tt> keys and
 * <tt>svn_string_t *</tt> values.
 */
svn_error_t *
svn_config_write_auth_data(apr_hash_t *hash,
                           const char *cred_kind,
                           const char *realmstring,
                           const char *config_dir,
                           apr_pool_t *pool);

/** Put the absolute path to the user's configuration directory,
 * or to a file within that directory, into @a *path.
 *
 * If @a config_dir is not NULL, it must point to an alternative
 * config directory location. If it is NULL, the default location
 * is used.  If @a fname is not NULL, it must specify the last
 * component of the path to be returned. This can be used to create
 * a path to any file in the configuration directory.
 *
 * Do all allocations in @a pool.
 *
 * Hint:
 * To get the user configuration file, pass @c SVN_CONFIG_CATEGORY_CONFIG
 * for @a fname. To get the servers configuration file, pass
 * @c SVN_CONFIG_CATEGORY_SERVERS for @a fname.
 *
 * @since New in 1.6.
 */
svn_error_t *
svn_config_get_user_config_path(const char **path,
                                const char *config_dir,
                                const char *fname,
                                apr_pool_t *pool);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_CONFIG_H */
