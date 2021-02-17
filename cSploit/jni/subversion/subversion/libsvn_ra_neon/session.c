/*
 * session.c :  routines for maintaining sessions state (to the DAV server)
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



#include <ctype.h>

#define APR_WANT_STRFUNC
#include <apr_want.h>
#include <apr_general.h>
#include <apr_xml.h>

#include <ne_auth.h>

#include "svn_error.h"
#include "svn_pools.h"
#include "svn_ra.h"
#include "../libsvn_ra/ra_loader.h"
#include "svn_config.h"
#include "svn_delta.h"
#include "svn_version.h"
#include "svn_path.h"
#include "svn_time.h"
#include "svn_xml.h"
#include "svn_private_config.h"
#include "private/svn_atomic.h"

#ifdef SVN_NEON_0_28
#include <ne_pkcs11.h>
#endif

#include "ra_neon.h"

#define DEFAULT_HTTP_TIMEOUT 3600

static svn_atomic_t neon_initialized = 0;


/* a cleanup routine attached to the pool that contains the RA session
   baton. */
static apr_status_t cleanup_session(void *sess)
{
  ne_session_destroy(sess);
  return APR_SUCCESS;
}

/* a cleanup routine attached to the pool that contains the RA session
   root URI. */
static apr_status_t cleanup_uri(void *uri)
{
  ne_uri_free(uri);
  return APR_SUCCESS;
}

#ifdef SVN_NEON_0_28
/* a cleanup routine attached to the pool that contains the PKCS#11
   provider object. */
static apr_status_t cleanup_p11provider(void *provider)
{
  ne_ssl_pkcs11_provider *prov = provider;
  ne_ssl_pkcs11_provider_destroy(prov);
  return APR_SUCCESS;
}
#endif

/* A neon-session callback to 'pull' authentication data when
   challenged.  In turn, this routine 'pulls' the data from the client
   callbacks if needed.  */
static int request_auth(void *userdata, const char *realm, int attempt,
                        char *username, char *password)
{
  svn_error_t *err;
  svn_ra_neon__session_t *ras = userdata;
  void *creds;
  svn_auth_cred_simple_t *simple_creds;

  /* Start by marking the current credentials invalid. */
  ras->auth_used = FALSE;

  /* No auth_baton?  Give up. */
  if (! ras->callbacks->auth_baton)
    return -1;

  /* Neon automatically tries some auth protocols and bumps the attempt
     count without using Subversion's callbacks, so we can't depend
     on attempt == 0 the first time we are called -- we need to check
     if the auth state has been initted as well.  */
  if (attempt == 0 || ras->auth_iterstate == NULL)
    {
      const char *realmstring;

      /* <https://svn.collab.net:80> Subversion repository */
      realmstring = apr_psprintf(ras->pool, "<%s://%s:%d> %s",
                                 ras->root.scheme, ras->root.host,
                                 ras->root.port, realm);

      err = svn_auth_first_credentials(&creds,
                                       &(ras->auth_iterstate),
                                       SVN_AUTH_CRED_SIMPLE,
                                       realmstring,
                                       ras->callbacks->auth_baton,
                                       ras->pool);
    }

  else /* attempt > 0 */
    /* ### TODO:  if the http realm changed this time around, we
       should be calling first_creds(), not next_creds(). */
    err = svn_auth_next_credentials(&creds,
                                    ras->auth_iterstate,
                                    ras->pool);
  if (err || ! creds)
    {
      svn_error_clear(err);
      return -1;
    }
  simple_creds = creds;

  /* Make svn_ra_neon__request_dispatch store the credentials after it
     sees a succesful response */
  ras->auth_used = TRUE;

  /* ### silently truncates username/password to 256 chars. */
  apr_cpystrn(username, simple_creds->username, NE_ABUFSIZ);
  apr_cpystrn(password, simple_creds->password, NE_ABUFSIZ);

  return 0;
}


static const apr_uint32_t neon_failure_map[][2] =
{
  { NE_SSL_NOTYETVALID,        SVN_AUTH_SSL_NOTYETVALID },
  { NE_SSL_EXPIRED,            SVN_AUTH_SSL_EXPIRED },
  { NE_SSL_IDMISMATCH,         SVN_AUTH_SSL_CNMISMATCH },
  { NE_SSL_UNTRUSTED,          SVN_AUTH_SSL_UNKNOWNCA }
};

/* Convert neon's SSL failure mask to our own failure mask. */
static apr_uint32_t
convert_neon_failures(int neon_failures)
{
  apr_uint32_t svn_failures = 0;
  apr_size_t i;

  for (i = 0; i < sizeof(neon_failure_map) / (2 * sizeof(int)); ++i)
    {
      if (neon_failures & neon_failure_map[i][0])
        {
          svn_failures |= neon_failure_map[i][1];
          neon_failures &= ~neon_failure_map[i][0];
        }
    }

  /* Map any remaining neon failure bits to our OTHER bit. */
  if (neon_failures)
    {
      svn_failures |= SVN_AUTH_SSL_OTHER;
    }

  return svn_failures;
}

/* A neon-session callback to validate the SSL certificate when the CA
   is unknown (e.g. a self-signed cert), or there are other SSL
   certificate problems. */
static int
server_ssl_callback(void *userdata,
                    int failures,
                    const ne_ssl_certificate *cert)
{
  svn_ra_neon__session_t *ras = userdata;
  svn_auth_cred_ssl_server_trust_t *server_creds = NULL;
  void *creds;
  svn_auth_iterstate_t *state;
  apr_pool_t *pool;
  svn_error_t *error;
  char *ascii_cert = ne_ssl_cert_export(cert);
  char *issuer_dname = ne_ssl_readable_dname(ne_ssl_cert_issuer(cert));
  svn_auth_ssl_server_cert_info_t cert_info;
  char fingerprint[NE_SSL_DIGESTLEN];
  char valid_from[NE_SSL_VDATELEN], valid_until[NE_SSL_VDATELEN];
  const char *realmstring;
  apr_uint32_t *svn_failures = apr_palloc(ras->pool, sizeof(*svn_failures));

  /* Construct the realmstring, e.g. https://svn.collab.net:80 */
  realmstring = apr_psprintf(ras->pool, "%s://%s:%d", ras->root.scheme,
                             ras->root.host, ras->root.port);

  *svn_failures = convert_neon_failures(failures);
  svn_auth_set_parameter(ras->callbacks->auth_baton,
                         SVN_AUTH_PARAM_SSL_SERVER_FAILURES,
                         svn_failures);

  /* Extract the info from the certificate */
  cert_info.hostname = ne_ssl_cert_identity(cert);
  if (ne_ssl_cert_digest(cert, fingerprint) != 0)
    {
      strcpy(fingerprint, "<unknown>");
    }
  cert_info.fingerprint = fingerprint;
  ne_ssl_cert_validity(cert, valid_from, valid_until);
  cert_info.valid_from = valid_from;
  cert_info.valid_until = valid_until;
  cert_info.issuer_dname = issuer_dname;
  cert_info.ascii_cert = ascii_cert;

  svn_auth_set_parameter(ras->callbacks->auth_baton,
                         SVN_AUTH_PARAM_SSL_SERVER_CERT_INFO,
                         &cert_info);

  apr_pool_create(&pool, ras->pool);
  error = svn_auth_first_credentials(&creds, &state,
                                     SVN_AUTH_CRED_SSL_SERVER_TRUST,
                                     realmstring,
                                     ras->callbacks->auth_baton,
                                     pool);
  if (error || ! creds)
    {
      svn_error_clear(error);
    }
  else
    {
      server_creds = creds;
      error = svn_auth_save_credentials(state, pool);
      if (error)
        {
          /* It would be nice to show the error to the user somehow... */
          svn_error_clear(error);
        }
    }

  free(issuer_dname);
  free(ascii_cert);
  svn_auth_set_parameter(ras->callbacks->auth_baton,
                         SVN_AUTH_PARAM_SSL_SERVER_CERT_INFO, NULL);

  svn_pool_destroy(pool);
  return ! server_creds;
}

static svn_boolean_t
client_ssl_decrypt_cert(svn_ra_neon__session_t *ras,
                        const char *cert_file,
                        ne_ssl_client_cert *clicert)
{
  svn_auth_iterstate_t *state;
  svn_error_t *error;
  apr_pool_t *pool;
  svn_boolean_t ok = FALSE;
  void *creds;
  int try;

  apr_pool_create(&pool, ras->pool);
  for (try = 0; TRUE; ++try)
    {
      if (try == 0)
        {
          error = svn_auth_first_credentials(&creds, &state,
                                             SVN_AUTH_CRED_SSL_CLIENT_CERT_PW,
                                             cert_file,
                                             ras->callbacks->auth_baton,
                                             pool);
        }
      else
        {
          error = svn_auth_next_credentials(&creds, state, pool);
        }

      if (error || ! creds)
        {
          /* Failure or too many attempts */
          svn_error_clear(error);
          break;
        }
      else
        {
          svn_auth_cred_ssl_client_cert_pw_t *pw_creds = creds;

          if (ne_ssl_clicert_decrypt(clicert, pw_creds->password) == 0)
            {
              error = svn_auth_save_credentials(state, pool);
              if (error)
                svn_error_clear(error);

              /* Success */
              ok = TRUE;
              break;
            }
        }
    }
  svn_pool_destroy(pool);

  return ok;
}

#ifdef SVN_NEON_0_28
/* Callback invoked to enter PKCS#11 PIN code. */
static int
client_ssl_pkcs11_pin_entry(void *userdata,
                            int attempt,
                            const char *slot_descr,
                            const char *token_label,
                            unsigned int flags,
                            char *pin)
{
  svn_ra_neon__session_t *ras = userdata;
  svn_error_t *err;
  void *creds;
  svn_auth_cred_ssl_client_cert_pw_t *pw_creds;

  /* Always prevent PIN caching. */
  svn_auth_set_parameter(ras->callbacks->auth_baton,
                         SVN_AUTH_PARAM_NO_AUTH_CACHE, "");

  if (attempt == 0)
    {
      const char *realmstring;

      realmstring = apr_psprintf(ras->pool,
                                 _("PIN for token \"%s\" in slot \"%s\""),
                                 token_label, slot_descr);

      err = svn_auth_first_credentials(&creds,
                                       &(ras->auth_iterstate),
                                       SVN_AUTH_CRED_SSL_CLIENT_CERT_PW,
                                       realmstring,
                                       ras->callbacks->auth_baton,
                                       ras->pool);
    }
  else
    {
      err = svn_auth_next_credentials(&creds, ras->auth_iterstate, ras->pool);
    }

  if (err || ! creds)
    {
      svn_error_clear(err);
      return -1;
    }

  pw_creds = creds;

  apr_cpystrn(pin, pw_creds->password, NE_SSL_P11PINLEN);

  return 0;
}
#endif

static void
client_ssl_callback(void *userdata, ne_session *sess,
                    const ne_ssl_dname *const *dnames,
                    int dncount)
{
  svn_ra_neon__session_t *ras = userdata;
  ne_ssl_client_cert *clicert = NULL;
  void *creds;
  svn_auth_iterstate_t *state;
  const char *realmstring;
  apr_pool_t *pool;
  svn_error_t *error;
  int try;

  apr_pool_create(&pool, ras->pool);

  realmstring = apr_psprintf(pool, "%s://%s:%d", ras->root.scheme,
                             ras->root.host, ras->root.port);

  for (try = 0; TRUE; ++try)
    {
      if (try == 0)
        {
          error = svn_auth_first_credentials(&creds, &state,
                                             SVN_AUTH_CRED_SSL_CLIENT_CERT,
                                             realmstring,
                                             ras->callbacks->auth_baton,
                                             pool);
        }
      else
        {
          error = svn_auth_next_credentials(&creds, state, pool);
        }

      if (error || ! creds)
        {
          /* Failure or too many attempts */
          svn_error_clear(error);
          break;
        }
      else
        {
          svn_auth_cred_ssl_client_cert_t *client_creds = creds;

          clicert = ne_ssl_clicert_read(client_creds->cert_file);
          if (clicert)
            {
              if (! ne_ssl_clicert_encrypted(clicert) ||
                  client_ssl_decrypt_cert(ras, client_creds->cert_file,
                                          clicert))
                {
                  ne_ssl_set_clicert(sess, clicert);
                }
              break;
            }
        }
    }

  svn_pool_destroy(pool);
}

/* Set *PROXY_HOST, *PROXY_PORT, *PROXY_USERNAME, *PROXY_PASSWORD,
 * *TIMEOUT_SECONDS, *NEON_DEBUG, *COMPRESSION, *NEON_AUTH_TYPES, and
 * *PK11_PROVIDER to the information for REQUESTED_HOST, allocated in
 * POOL, if there is any applicable information.  If there is no
 * applicable information or if there is an error, then set
 * *PROXY_PORT to (unsigned int) -1, *TIMEOUT_SECONDS and *NEON_DEBUG
 * to zero, *COMPRESSION to TRUE, *NEON_AUTH_TYPES is left untouched,
 * and the rest are set to NULL.  This function can return an error,
 * so before examining any values, check the error return value.
 */
static svn_error_t *get_server_settings(const char **proxy_host,
                                        unsigned int *proxy_port,
                                        const char **proxy_username,
                                        const char **proxy_password,
                                        int *timeout_seconds,
                                        int *neon_debug,
                                        svn_boolean_t *compression,
                                        unsigned int *neon_auth_types,
                                        const char **pk11_provider,
                                        svn_config_t *cfg,
                                        const char *requested_host,
                                        apr_pool_t *pool)
{
  const char *exceptions, *port_str, *timeout_str, *server_group;
  const char *debug_str;
  svn_boolean_t is_exception = FALSE;
#ifdef SVN_NEON_0_26
  const char *http_auth_types = NULL;
#endif

  /* If we find nothing, default to nulls. */
  *proxy_host     = NULL;
  *proxy_port     = (unsigned int) -1;
  *proxy_username = NULL;
  *proxy_password = NULL;
  port_str        = NULL;
  timeout_str     = NULL;
  debug_str       = NULL;
  *pk11_provider  = NULL;

  /* Use the default proxy-specific settings if and only if
     "http-proxy-exceptions" is not set to exclude this host. */
  svn_config_get(cfg, &exceptions, SVN_CONFIG_SECTION_GLOBAL,
                 SVN_CONFIG_OPTION_HTTP_PROXY_EXCEPTIONS, NULL);
  if (exceptions)
    {
      apr_array_header_t *l = svn_cstring_split(exceptions, ",", TRUE, pool);
      is_exception = svn_cstring_match_glob_list(requested_host, l);
    }
  if (! is_exception)
    {
      svn_config_get(cfg, proxy_host, SVN_CONFIG_SECTION_GLOBAL,
                     SVN_CONFIG_OPTION_HTTP_PROXY_HOST, NULL);
      svn_config_get(cfg, &port_str, SVN_CONFIG_SECTION_GLOBAL,
                     SVN_CONFIG_OPTION_HTTP_PROXY_PORT, NULL);
      svn_config_get(cfg, proxy_username, SVN_CONFIG_SECTION_GLOBAL,
                     SVN_CONFIG_OPTION_HTTP_PROXY_USERNAME, NULL);
      svn_config_get(cfg, proxy_password, SVN_CONFIG_SECTION_GLOBAL,
                     SVN_CONFIG_OPTION_HTTP_PROXY_PASSWORD, NULL);
    }

  /* Apply non-proxy-specific settings regardless of exceptions: */
  svn_config_get(cfg, &timeout_str, SVN_CONFIG_SECTION_GLOBAL,
                 SVN_CONFIG_OPTION_HTTP_TIMEOUT, NULL);
  SVN_ERR(svn_config_get_bool(cfg, compression, SVN_CONFIG_SECTION_GLOBAL,
                              SVN_CONFIG_OPTION_HTTP_COMPRESSION, TRUE));
  svn_config_get(cfg, &debug_str, SVN_CONFIG_SECTION_GLOBAL,
                 SVN_CONFIG_OPTION_NEON_DEBUG_MASK, NULL);
#ifdef SVN_NEON_0_26
  svn_config_get(cfg, &http_auth_types, SVN_CONFIG_SECTION_GLOBAL,
                 SVN_CONFIG_OPTION_HTTP_AUTH_TYPES, NULL);
#endif
  svn_config_get(cfg, pk11_provider, SVN_CONFIG_SECTION_GLOBAL,
                 SVN_CONFIG_OPTION_SSL_PKCS11_PROVIDER, NULL);

  if (cfg)
    server_group = svn_config_find_group(cfg, requested_host,
                                         SVN_CONFIG_SECTION_GROUPS, pool);
  else
    server_group = NULL;

  if (server_group)
    {
      svn_config_get(cfg, proxy_host, server_group,
                     SVN_CONFIG_OPTION_HTTP_PROXY_HOST, *proxy_host);
      svn_config_get(cfg, &port_str, server_group,
                     SVN_CONFIG_OPTION_HTTP_PROXY_PORT, port_str);
      svn_config_get(cfg, proxy_username, server_group,
                     SVN_CONFIG_OPTION_HTTP_PROXY_USERNAME, *proxy_username);
      svn_config_get(cfg, proxy_password, server_group,
                     SVN_CONFIG_OPTION_HTTP_PROXY_PASSWORD, *proxy_password);
      svn_config_get(cfg, &timeout_str, server_group,
                     SVN_CONFIG_OPTION_HTTP_TIMEOUT, timeout_str);
      SVN_ERR(svn_config_get_bool(cfg, compression, server_group,
                                  SVN_CONFIG_OPTION_HTTP_COMPRESSION,
                                  *compression));
      svn_config_get(cfg, &debug_str, server_group,
                     SVN_CONFIG_OPTION_NEON_DEBUG_MASK, debug_str);
#ifdef SVN_NEON_0_26
      svn_config_get(cfg, &http_auth_types, server_group,
                     SVN_CONFIG_OPTION_HTTP_AUTH_TYPES, http_auth_types);
#endif
      svn_config_get(cfg, pk11_provider, server_group,
                     SVN_CONFIG_OPTION_SSL_PKCS11_PROVIDER, *pk11_provider);
    }

  /* Special case: convert the port value, if any. */
  if (port_str)
    {
      char *endstr;
      const long int port = strtol(port_str, &endstr, 10);

      if (*endstr)
        return svn_error_create(SVN_ERR_RA_ILLEGAL_URL, NULL,
                                _("Invalid URL: illegal character in proxy "
                                  "port number"));
      if (port < 0)
        return svn_error_create(SVN_ERR_RA_ILLEGAL_URL, NULL,
                                _("Invalid URL: negative proxy port number"));
      if (port > 65535)
        return svn_error_create(SVN_ERR_RA_ILLEGAL_URL, NULL,
                                _("Invalid URL: proxy port number greater "
                                  "than maximum TCP port number 65535"));
      *proxy_port = port;
    }
  else
    *proxy_port = 80;

  if (timeout_str)
    {
      char *endstr;
      const long int timeout = strtol(timeout_str, &endstr, 10);

      if (*endstr)
        return svn_error_create(SVN_ERR_BAD_CONFIG_VALUE, NULL,
                                _("Invalid config: illegal character in "
                                  "timeout value"));
      if (timeout < 0)
        return svn_error_create(SVN_ERR_BAD_CONFIG_VALUE, NULL,
                                _("Invalid config: negative timeout value"));
      *timeout_seconds = timeout;
    }
  else
    *timeout_seconds = 0;

  if (debug_str)
    {
      char *endstr;
      const long int debug = strtol(debug_str, &endstr, 10);

      if (*endstr)
        return svn_error_create(SVN_ERR_BAD_CONFIG_VALUE, NULL,
                                _("Invalid config: illegal character in "
                                  "debug mask value"));

      *neon_debug = debug;
    }
  else
    *neon_debug = 0;

#ifdef SVN_NEON_0_26
  if (http_auth_types)
    {
      char *token, *last;
      char *auth_types_list = apr_palloc(pool, strlen(http_auth_types) + 1);
      apr_collapse_spaces(auth_types_list, http_auth_types);
      while ((token = apr_strtok(auth_types_list, ";", &last)) != NULL)
        {
          auth_types_list = NULL;
          if (svn_cstring_casecmp("basic", token) == 0)
            *neon_auth_types |= NE_AUTH_BASIC;
          else if (svn_cstring_casecmp("digest", token) == 0)
            *neon_auth_types |= NE_AUTH_DIGEST;
          else if (svn_cstring_casecmp("negotiate", token) == 0)
            *neon_auth_types |= NE_AUTH_NEGOTIATE;
          else
            return svn_error_createf(SVN_ERR_BAD_CONFIG_VALUE, NULL,
                                     _("Invalid config: unknown http auth"
                                       "type '%s'"), token);
      }
    }
#endif /* SVN_NEON_0_26 */

  return SVN_NO_ERROR;
}


/* Userdata for the `proxy_auth' function. */
struct proxy_auth_baton
{
  const char *username;  /* Cannot be NULL, but "" is okay. */
  const char *password;  /* Cannot be NULL, but "" is okay. */
};


/* An `ne_request_auth' callback, see ne_auth.h.  USERDATA is a
 * `struct proxy_auth_baton *'.
 *
 * If ATTEMPT < 10, copy USERDATA->username and USERDATA->password
 * into USERNAME and PASSWORD respectively (but do not copy more than
 * NE_ABUFSIZ bytes of either), and return zero to indicate to Neon
 * that authentication should be attempted.
 *
 * If ATTEMPT >= 10, copy nothing into USERNAME and PASSWORD and
 * return 1, to cancel further authentication attempts.
 *
 * Ignore REALM.
 *
 * ### Note: There is no particularly good reason for the 10-attempt
 * limit.  Perhaps there should only be one attempt, and if it fails,
 * we just cancel any further attempts.  I used 10 just in case the
 * proxy tries various times with various realms, since we ignore
 * REALM.  And why do we ignore REALM?  Because we currently don't
 * have any way to specify different auth information for different
 * realms.  (I'm assuming that REALM would be a realm on the proxy
 * server, not on the Subversion repository server that is the real
 * destination.)  Do we have any need to support proxy realms?
 */
static int proxy_auth(void *userdata,
                      const char *realm,
                      int attempt,
                      char *username,
                      char *password)
{
  struct proxy_auth_baton *pab = userdata;

  if (attempt >= 10)
    return 1;

  /* Else. */

  apr_cpystrn(username, pab->username, NE_ABUFSIZ);
  apr_cpystrn(password, pab->password, NE_ABUFSIZ);

  return 0;
}

#define RA_NEON_DESCRIPTION \
  N_("Module for accessing a repository via WebDAV protocol using Neon.")

static const char *
ra_neon_get_description(void)
{
  return _(RA_NEON_DESCRIPTION);
}

static const char * const *
ra_neon_get_schemes(apr_pool_t *pool)
{
  static const char *schemes_no_ssl[] = { "http", NULL };
  static const char *schemes_ssl[] = { "http", "https", NULL };

  return ne_has_support(NE_FEATURE_SSL) ? schemes_ssl : schemes_no_ssl;
}

typedef struct neonprogress_baton_t
{
  svn_ra_neon__session_t *ras;
  apr_off_t last_progress;
  apr_pool_t *pool;
} neonprogress_baton_t;

static void
#ifdef SVN_NEON_0_27
ra_neon_neonprogress(void *baton, ne_off_t progress, ne_off_t total)
#else
ra_neon_neonprogress(void *baton, off_t progress, off_t total)
#endif /* SVN_NEON_0_27 */
{
  neonprogress_baton_t *pb = baton;
  svn_ra_neon__session_t *ras = pb->ras;

 if (ras->progress_func)
    {
      if (total < 0)
        {
          /* Neon sends the total number of bytes sent for this specific
             session and there are two sessions active at once.

             For this case we combine the totals to allow clients to provide
             a better progress indicator. */

          if (progress >= pb->last_progress)
            ras->total_progress += (progress - pb->last_progress);
          else
            /* Session total has been reset. A new stream started */
            ras->total_progress += pb->last_progress;

          pb->last_progress = progress;

          ras->progress_func(ras->total_progress, -1, ras->progress_baton,
                             pb->pool);
        }
      else
        {
          /* Neon provides total bytes to receive information. Pass literaly
             to allow providing a percentage. */
          ras->progress_func(progress, total, ras->progress_baton, pb->pool);
        }
    }
}



/* ### need an ne_session_dup to avoid the second gethostbyname
 * call and make this halfway sane. */


/* Parse URL into *URI, doing some sanity checking and initializing the port
   to a default value if it wasn't specified in URL.  */
static svn_error_t *
parse_url(ne_uri *uri, const char *url)
{
  if (ne_uri_parse(url, uri)
      || uri->host == NULL || uri->path == NULL || uri->scheme == NULL)
    {
      ne_uri_free(uri);
      return svn_error_createf(SVN_ERR_RA_ILLEGAL_URL, NULL,
                               _("URL '%s' is malformed or the "
                                 "scheme or host or path is missing"), url);
    }
  if (uri->port == 0)
    uri->port = ne_uri_defaultport(uri->scheme);

  return SVN_NO_ERROR;
}

/* Initializer function matching the prototype accepted by
   svn_atomic__init_once(). */
static svn_error_t *
initialize_neon(void *baton, apr_pool_t *ignored_pool)
{
  if (ne_sock_init() != 0)
    return svn_error_create(SVN_ERR_RA_DAV_SOCK_INIT, NULL,
                            _("Network socket initialization failed"));

  return SVN_NO_ERROR;
}

/* Initialize neon when not initialized before. */
static svn_error_t *
ensure_neon_initialized(void)
{
  return svn_atomic__init_once(&neon_initialized, initialize_neon, NULL, NULL);
}

static svn_error_t *
svn_ra_neon__open(svn_ra_session_t *session,
                  const char **corrected_url,
                  const char *repos_URL,
                  const svn_ra_callbacks2_t *callbacks,
                  void *callback_baton,
                  apr_hash_t *config,
                  apr_pool_t *pool)
{
  apr_size_t len;
  ne_session *sess, *sess2;
  ne_uri *uri = apr_pcalloc(pool, sizeof(*uri));
  svn_ra_neon__session_t *ras;
  int is_ssl_session;
  svn_boolean_t compression;
  svn_config_t *cfg, *cfg_client;
  const char *server_group;
  unsigned int neon_auth_types = 0;
  const char *pkcs11_provider;
  const char *useragent = NULL;
  const char *client_string = NULL;

  SVN_ERR_ASSERT(svn_uri_is_canonical(repos_URL, pool));

  if (callbacks->get_client_string)
    callbacks->get_client_string(callback_baton, &client_string, pool);

  if (client_string)
    useragent = apr_pstrcat(pool, "SVN/" SVN_VER_NUMBER "/", client_string,
                            (char *)NULL);
  else
    useragent = "SVN/" SVN_VER_NUMBER;

  /* Sanity check the URI */
  SVN_ERR(parse_url(uri, repos_URL));

  /* make sure we eventually destroy the uri */
  apr_pool_cleanup_register(pool, uri, cleanup_uri, apr_pool_cleanup_null);


  /* Initialize neon if required */
  SVN_ERR(ensure_neon_initialized());

  /* we want to know if the repository is actually somewhere else */
  /* ### not yet: http_redirect_register(sess, ... ); */

  is_ssl_session = (strcmp(uri->scheme, "https") == 0);
  if (is_ssl_session)
    {
      if (ne_has_support(NE_FEATURE_SSL) == 0)
        return svn_error_create(SVN_ERR_RA_DAV_SOCK_INIT, NULL,
                                _("SSL is not supported"));
    }
  /* Create two neon session objects, and set their properties... */
  sess = ne_session_create(uri->scheme, uri->host, uri->port);
  sess2 = ne_session_create(uri->scheme, uri->host, uri->port);
  /* make sure we will eventually destroy the session */
  apr_pool_cleanup_register(pool, sess, cleanup_session, apr_pool_cleanup_null);
  apr_pool_cleanup_register(pool, sess2, cleanup_session,
                            apr_pool_cleanup_null);

  cfg = config ? apr_hash_get(config,
                              SVN_CONFIG_CATEGORY_SERVERS,
                              APR_HASH_KEY_STRING) : NULL;
  cfg_client = config ? apr_hash_get(config,
                                     SVN_CONFIG_CATEGORY_CONFIG,
                                     APR_HASH_KEY_STRING) : NULL;
  if (cfg)
    server_group = svn_config_find_group(cfg, uri->host,
                                         SVN_CONFIG_SECTION_GROUPS, pool);
  else
    server_group = NULL;

  /* If there's a timeout or proxy for this URL, use it. */
  {
    const char *proxy_host;
    unsigned int proxy_port;
    const char *proxy_username;
    const char *proxy_password;
    int timeout;
    int debug;

    SVN_ERR(get_server_settings(&proxy_host,
                                &proxy_port,
                                &proxy_username,
                                &proxy_password,
                                &timeout,
                                &debug,
                                &compression,
                                &neon_auth_types,
                                &pkcs11_provider,
                                cfg,
                                uri->host,
                                pool));

#ifdef SVN_NEON_0_26
    if (neon_auth_types == 0)
      {
        /* If there were no auth types specified in the configuration
           file, provide the appropriate defaults. */
        neon_auth_types = NE_AUTH_BASIC | NE_AUTH_DIGEST;
        if (is_ssl_session)
          neon_auth_types |= NE_AUTH_NEGOTIATE;
      }
#endif

    if (debug)
      ne_debug_init(stderr, debug);

    if (proxy_host)
      {
        ne_session_proxy(sess, proxy_host, proxy_port);
        ne_session_proxy(sess2, proxy_host, proxy_port);

        if (proxy_username)
          {
            /* Allocate the baton in pool, not on stack, so it will
               last till whenever Neon needs it. */
            struct proxy_auth_baton *pab = apr_palloc(pool, sizeof(*pab));

            pab->username = proxy_username;
            pab->password = proxy_password ? proxy_password : "";

            ne_set_proxy_auth(sess, proxy_auth, pab);
            ne_set_proxy_auth(sess2, proxy_auth, pab);
          }
#ifdef SVN_NEON_0_26
        else
          {
            /* Enable (only) the Negotiate scheme for proxy
               authentication, if no username/password is
               configured. */
            ne_add_proxy_auth(sess, NE_AUTH_NEGOTIATE, NULL, NULL);
            ne_add_proxy_auth(sess2, NE_AUTH_NEGOTIATE, NULL, NULL);
          }
#endif
      }

    if (!timeout)
      timeout = DEFAULT_HTTP_TIMEOUT;
    ne_set_read_timeout(sess, timeout);
    ne_set_read_timeout(sess2, timeout);

#ifdef SVN_NEON_0_27
    ne_set_connect_timeout(sess, timeout);
    ne_set_connect_timeout(sess2, timeout);
#endif
  }

  if (useragent)
    {
      ne_set_useragent(sess, useragent);
      ne_set_useragent(sess2, useragent);
    }
  else
    {
      ne_set_useragent(sess, "SVN/" SVN_VER_NUMBER);
      ne_set_useragent(sess2, "SVN/" SVN_VER_NUMBER);
    }

  /* clean up trailing slashes from the URL */
  len = strlen(uri->path);
  if (len > 1 && (uri->path)[len - 1] == '/')
    (uri->path)[len - 1] = '\0';

  /* Create and fill a session_baton. */
  ras = apr_pcalloc(pool, sizeof(*ras));
  ras->pool = pool;
  ras->url = svn_stringbuf_create(repos_URL, pool);
  /* copies uri pointer members, they get free'd in __close. */
  ras->root = *uri;
  ras->ne_sess = sess;
  ras->ne_sess2 = sess2;
  ras->callbacks = callbacks;
  ras->callback_baton = callback_baton;
  ras->compression = compression;
  ras->progress_baton = callbacks->progress_baton;
  ras->progress_func = callbacks->progress_func;
  ras->capabilities = apr_hash_make(ras->pool);
  ras->supports_deadprop_count = svn_tristate_unknown;
  ras->vcc = NULL;
  ras->uuid = NULL;
  /* save config and server group in the auth parameter hash */
  svn_auth_set_parameter(ras->callbacks->auth_baton,
                         SVN_AUTH_PARAM_CONFIG_CATEGORY_CONFIG, cfg_client);
  svn_auth_set_parameter(ras->callbacks->auth_baton,
                         SVN_AUTH_PARAM_CONFIG_CATEGORY_SERVERS, cfg);
  svn_auth_set_parameter(ras->callbacks->auth_baton,
                         SVN_AUTH_PARAM_SERVER_GROUP, server_group);

  /* note that ras->username and ras->password are still NULL at this
     point. */

  /* Register an authentication 'pull' callback with the neon sessions */
#ifdef SVN_NEON_0_26
  ne_add_server_auth(sess, neon_auth_types, request_auth, ras);
  ne_add_server_auth(sess2, neon_auth_types, request_auth, ras);
#else
  ne_set_server_auth(sess, request_auth, ras);
  ne_set_server_auth(sess2, request_auth, ras);
#endif

  if (is_ssl_session)
    {
      svn_boolean_t trust_default_ca;
      const char *authorities
        = svn_config_get_server_setting(cfg, server_group,
                                        SVN_CONFIG_OPTION_SSL_AUTHORITY_FILES,
                                        NULL);

      if (authorities != NULL)
        {
          char *files, *file, *last;
          files = apr_pstrdup(pool, authorities);

          while ((file = apr_strtok(files, ";", &last)) != NULL)
            {
              ne_ssl_certificate *ca_cert;
              files = NULL;
              ca_cert = ne_ssl_cert_read(file);
              if (ca_cert == NULL)
                {
                  return svn_error_createf(
                    SVN_ERR_BAD_CONFIG_VALUE, NULL,
                    _("Invalid config: unable to load certificate file '%s'"),
                    svn_dirent_local_style(file, pool));
                }
              ne_ssl_trust_cert(sess, ca_cert);
              ne_ssl_trust_cert(sess2, ca_cert);
            }
        }

      /* When the CA certificate or server certificate has
         verification problems, neon will call our verify function before
         outright rejection of the connection.*/
      ne_ssl_set_verify(sess, server_ssl_callback, ras);
      ne_ssl_set_verify(sess2, server_ssl_callback, ras);
      /* For client connections, we register a callback for if the server
         wants to authenticate the client via client certificate. */

#ifdef SVN_NEON_0_28
      if (pkcs11_provider)
        {
          ne_ssl_pkcs11_provider *provider;
          int rv;

          /* Initialize the PKCS#11 provider. */
          rv = ne_ssl_pkcs11_provider_init(&provider, pkcs11_provider);
          if (rv != NE_PK11_OK)
            {
              return svn_error_createf
                (SVN_ERR_BAD_CONFIG_VALUE, NULL,
                 _("Invalid config: unable to load PKCS#11 provider '%s'"),
                 pkcs11_provider);
            }

          /* Share the provider between the two sessions. */
          ne_ssl_set_pkcs11_provider(sess, provider);
          ne_ssl_set_pkcs11_provider(sess2, provider);

          ne_ssl_pkcs11_provider_pin(provider, client_ssl_pkcs11_pin_entry,
                                     ras);

          apr_pool_cleanup_register(pool, provider, cleanup_p11provider,
                                    apr_pool_cleanup_null);
        }
      /* Note the "else"; if a PKCS#11 provider is set up, a client
         cert callback is already configured, so don't displace it
         with the normal one here.  */
      else
#endif
        {
          ne_ssl_provide_clicert(sess, client_ssl_callback, ras);
          ne_ssl_provide_clicert(sess2, client_ssl_callback, ras);
        }

      /* See if the user wants us to trust "default" openssl CAs. */
      SVN_ERR(svn_config_get_server_setting_bool(
               cfg, &trust_default_ca, server_group,
               SVN_CONFIG_OPTION_SSL_TRUST_DEFAULT_CA, TRUE));

      if (trust_default_ca)
        {
          ne_ssl_trust_default_ca(sess);
          ne_ssl_trust_default_ca(sess2);
        }
    }

  if (ras->progress_func)
    {
      neonprogress_baton_t *progress1 = apr_pcalloc(pool, sizeof(*progress1));
      neonprogress_baton_t *progress2 = apr_pcalloc(pool, sizeof(*progress2));

      progress1->pool = pool;
      progress1->ras = ras;
      progress1->last_progress = 0;

      *progress2 = *progress1;

      ne_set_progress(sess, ra_neon_neonprogress, progress1);
      ne_set_progress(sess2, ra_neon_neonprogress, progress2);
    }

  session->priv = ras;

  return svn_ra_neon__exchange_capabilities(ras, corrected_url, NULL, pool);
}


static svn_error_t *svn_ra_neon__reparent(svn_ra_session_t *session,
                                          const char *url,
                                          apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  ne_uri *uri = apr_pcalloc(session->pool, sizeof(*uri));

  SVN_ERR(parse_url(uri, url));
  apr_pool_cleanup_register(session->pool, uri, cleanup_uri,
                            apr_pool_cleanup_null);

  ras->root = *uri;
  svn_stringbuf_set(ras->url, url);
  return SVN_NO_ERROR;
}

static svn_error_t *svn_ra_neon__get_session_url(svn_ra_session_t *session,
                                                 const char **url,
                                                 apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  *url = apr_pstrmemdup(pool, ras->url->data, ras->url->len);
  return SVN_NO_ERROR;
}

static svn_error_t *svn_ra_neon__get_repos_root(svn_ra_session_t *session,
                                                const char **url,
                                                apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;

  if (! ras->repos_root)
    {
      const char *bc_relative;
      svn_stringbuf_t *url_buf;

      SVN_ERR(svn_ra_neon__get_baseline_info(NULL, &bc_relative, NULL,
                                             ras, ras->url->data,
                                             SVN_INVALID_REVNUM, pool));

      /* Remove as many path components from the URL as there are components
         in bc_relative. */
      url_buf = svn_stringbuf_dup(ras->url, pool);
      svn_path_remove_components
        (url_buf, svn_path_component_count(bc_relative));
      ras->repos_root = apr_pstrdup(ras->pool, url_buf->data);
    }

  *url = ras->repos_root;
  return SVN_NO_ERROR;
}

/* Copied from svn_ra_get_path_relative_to_root() and de-vtable-ized
   to prevent a dependency cycle. */
svn_error_t *
svn_ra_neon__get_path_relative_to_root(svn_ra_session_t *session,
                                       const char **rel_path,
                                       const char *url,
                                       apr_pool_t *pool)
{
  const char *root_url;

  SVN_ERR(svn_ra_neon__get_repos_root(session, &root_url, pool));
  if (strcmp(root_url, url) == 0)
    {
      *rel_path = "";
    }
  else
    {
      *rel_path = svn_uri__is_child(root_url, url, pool);
      if (! *rel_path)
        return svn_error_createf(SVN_ERR_RA_ILLEGAL_URL, NULL,
                                 _("'%s' isn't a child of repository root "
                                   "URL '%s'"),
                                 url, root_url);
    }
  return SVN_NO_ERROR;
}

static svn_error_t *svn_ra_neon__do_get_uuid(svn_ra_session_t *session,
                                             const char **uuid,
                                             apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;

  if (! ras->uuid)
    {
      svn_ra_neon__resource_t *rsrc;
      const char *lopped_path;

      SVN_ERR(svn_ra_neon__search_for_starting_props(&rsrc, &lopped_path,
                                                     ras, ras->url->data,
                                                     pool));

      if (! ras->uuid)
        {
          /* ### better error reporting... */
          return svn_error_create(APR_EGENERAL, NULL,
                                  _("The UUID property was not found on the "
                                    "resource or any of its parents"));
        }
    }

  /* search_for_starting_props() filled it. */
  *uuid = ras->uuid;

  return SVN_NO_ERROR;
}


static const svn_version_t *
ra_neon_version(void)
{
  SVN_VERSION_BODY;
}

static const svn_ra__vtable_t neon_vtable = {
  ra_neon_version,
  ra_neon_get_description,
  ra_neon_get_schemes,
  svn_ra_neon__open,
  svn_ra_neon__reparent,
  svn_ra_neon__get_session_url,
  svn_ra_neon__get_latest_revnum,
  svn_ra_neon__get_dated_revision,
  svn_ra_neon__change_rev_prop,
  svn_ra_neon__rev_proplist,
  svn_ra_neon__rev_prop,
  svn_ra_neon__get_commit_editor,
  svn_ra_neon__get_file,
  svn_ra_neon__get_dir,
  svn_ra_neon__get_mergeinfo,
  svn_ra_neon__do_update,
  svn_ra_neon__do_switch,
  svn_ra_neon__do_status,
  svn_ra_neon__do_diff,
  svn_ra_neon__get_log,
  svn_ra_neon__do_check_path,
  svn_ra_neon__do_stat,
  svn_ra_neon__do_get_uuid,
  svn_ra_neon__get_repos_root,
  svn_ra_neon__get_locations,
  svn_ra_neon__get_location_segments,
  svn_ra_neon__get_file_revs,
  svn_ra_neon__lock,
  svn_ra_neon__unlock,
  svn_ra_neon__get_lock,
  svn_ra_neon__get_locks,
  svn_ra_neon__replay,
  svn_ra_neon__has_capability,
  svn_ra_neon__replay_range,
  svn_ra_neon__get_deleted_rev
};

svn_error_t *
svn_ra_neon__init(const svn_version_t *loader_version,
                  const svn_ra__vtable_t **vtable,
                  apr_pool_t *pool)
{
  static const svn_version_checklist_t checklist[] =
    {
      { "svn_subr",  svn_subr_version },
      { "svn_delta", svn_delta_version },
      { NULL, NULL }
    };

  SVN_ERR(svn_ver_check_list(ra_neon_version(), checklist));

  /* Simplified version check to make sure we can safely use the
     VTABLE parameter. The RA loader does a more exhaustive check. */
  if (loader_version->major != SVN_VER_MAJOR)
    {
      return svn_error_createf
        (SVN_ERR_VERSION_MISMATCH, NULL,
         _("Unsupported RA loader version (%d) for ra_neon"),
         loader_version->major);
    }

  *vtable = &neon_vtable;

  return SVN_NO_ERROR;
}

/* Compatibility wrapper for the 1.1 and before API. */
#define NAME "ra_neon"
#define DESCRIPTION RA_NEON_DESCRIPTION
#define VTBL neon_vtable
#define INITFUNC svn_ra_neon__init
#define COMPAT_INITFUNC svn_ra_dav_init
#include "../libsvn_ra/wrapper_template.h"
