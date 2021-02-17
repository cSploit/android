/*
 * svn_server.h :  declarations for the svn server
 *
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
 */



#ifndef SERVER_H
#define SERVER_H

#include <apr_network_io.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "svn_config.h"
#include "svn_repos.h"
#include "svn_ra_svn.h"

enum username_case_type { CASE_FORCE_UPPER, CASE_FORCE_LOWER, CASE_ASIS };

typedef struct server_baton_t {
  svn_repos_t *repos;
  const char *repos_name;  /* URI-encoded name of repository (not for authz) */
  svn_fs_t *fs;            /* For convenience; same as svn_repos_fs(repos) */
  svn_config_t *cfg;       /* Parsed repository svnserve.conf */
  svn_config_t *pwdb;      /* Parsed password database */
  svn_authz_t *authzdb;    /* Parsed authz rules */
  const char *authz_repos_name; /* The name of the repository for authz */
  const char *realm;       /* Authentication realm */
  const char *repos_url;   /* URL to base of repository */
  svn_stringbuf_t *fs_path;/* Decoded base in-repos path (w/ leading slash) */
  apr_hash_t *fs_config;   /* Additional FS configuration parameters */
  const char *user;        /* Authenticated username of the user */
  enum username_case_type username_case; /* Case-normalize the username? */
  const char *authz_user;  /* Username for authz ('user' + 'username_case') */
  svn_boolean_t tunnel;    /* Tunneled through login agent */
  const char *tunnel_user; /* Allow EXTERNAL to authenticate as this */
  svn_boolean_t read_only; /* Disallow write access (global flag) */
  svn_boolean_t use_sasl;  /* Use Cyrus SASL for authentication;
                              always false if SVN_HAVE_SASL not defined */
  apr_file_t *log_file;    /* Log filehandle. */
  apr_pool_t *pool;
} server_baton_t;

enum authn_type { UNAUTHENTICATED, AUTHENTICATED };
enum access_type { NO_ACCESS, READ_ACCESS, WRITE_ACCESS };

enum access_type get_access(server_baton_t *b, enum authn_type auth);

typedef struct serve_params_t {
  /* The virtual root of the repositories to serve.  The client URL
     path is interpreted relative to this root and is not allowed to
     escape it. */
  const char *root;

  /* True if the connection is tunneled over an ssh-like transport,
     such that the client may use EXTERNAL to authenticate as the
     current uid's username. */
  svn_boolean_t tunnel;

  /* If tunnel is true, overrides the current uid's username as the
     identity EXTERNAL authenticates as. */
  const char *tunnel_user;

  /* True if the read-only flag was specified on the command-line,
     which forces all connections to be read-only. */
  svn_boolean_t read_only;

  /* A parsed repository svnserve configuration file, ala
     svnserve.conf.  If this is NULL, then no configuration file was
     specified on the command line.  If this is non-NULL, then
     per-repository svnserve.conf are not read. */
  svn_config_t *cfg;

  /* A parsed repository password database.  If this is NULL, then
     either no svnserve configuration file was specified on the
     command line, or it was specified and it did not refer to a
     password database. */
  svn_config_t *pwdb;

  /* A parsed repository authorization database.  If this is NULL,
     then either no svnserve configuration file was specified on the
     command line, or it was specified and it did not refer to a
     authorization database. */
  svn_authz_t *authzdb;

  /* A filehandle open for writing logs to; possibly NULL. */
  apr_file_t *log_file;

  /* Username case normalization style. */
  enum username_case_type username_case;

  /* Enable text delta caching for all FSFS repositories. */
  svn_boolean_t cache_txdeltas;

  /* Enable full-text caching for all FSFS repositories. */
  svn_boolean_t cache_fulltexts;

  /* Size of the in-memory cache (used by FSFS only). */
  apr_uint64_t memory_cache_size;

  /* Data compression level to reduce for network traffic. If this
     is 0, no compression should be applied and the protocol may
     fall back to svndiff "version 0" bypassing zlib entirely.
     Defaults to SVN_DELTA_COMPRESSION_LEVEL_DEFAULT. */
  int compression_level;

} serve_params_t;

/* Serve the connection CONN according to the parameters PARAMS. */
svn_error_t *serve(svn_ra_svn_conn_t *conn, serve_params_t *params,
                   apr_pool_t *pool);

/* Load a svnserve configuration file located at FILENAME into CFG,
   and if such as found, then:

    - set *PWDB to any referenced password database,
    - set *AUTHZDB to any referenced authorization database, and
    - set *USERNAME_CASE to the enumerated value of the
      'force-username-case' configuration value (or its default).

   If MUST_EXIST is true and FILENAME does not exist, then return an
   error.  BASE may be specified as the base path to any referenced
   password and authorization files found in FILENAME.

   If SERVER is not NULL, log the real errors with SERVER and CONN but
   return generic errors to the client.  CONN must not be NULL if SERVER
   is not NULL. */
svn_error_t *load_configs(svn_config_t **cfg,
                          svn_config_t **pwdb,
                          svn_authz_t **authzdb,
                          enum username_case_type *username_case,
                          const char *filename,
                          svn_boolean_t must_exist,
                          const char *base,
                          server_baton_t *server,
                          svn_ra_svn_conn_t *conn,
                          apr_pool_t *pool);

/* Initialize the Cyrus SASL library. POOL is used for allocations. */
svn_error_t *cyrus_init(apr_pool_t *pool);

/* Authenticate using Cyrus SASL. */
svn_error_t *cyrus_auth_request(svn_ra_svn_conn_t *conn,
                                apr_pool_t *pool,
                                server_baton_t *b,
                                enum access_type required,
                                svn_boolean_t needs_username);

/* Escape SOURCE into DEST where SOURCE is null-terminated and DEST is
   size BUFLEN DEST will be null-terminated.  Returns number of bytes
   written, including terminating null byte. */
apr_size_t escape_errorlog_item(char *dest, const char *source,
                                apr_size_t buflen);

/* Log ERR to LOG_FILE if LOG_FILE is not NULL.  Include REMOTE_HOST,
   USER, and REPOS in the log if they are not NULL.  Allocate temporary
   char buffers in POOL (which caller can then clear or dispose of). */
void
log_error(svn_error_t *err, apr_file_t *log_file, const char *remote_host,
          const char *user, const char *repos, apr_pool_t *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* SERVER_H */
