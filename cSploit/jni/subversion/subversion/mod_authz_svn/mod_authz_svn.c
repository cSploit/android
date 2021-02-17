/*
 * mod_authz_svn.c: an Apache mod_dav_svn sub-module to provide path
 *                  based authorization for a Subversion repository.
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



#include <httpd.h>
#include <http_config.h>
#include <http_core.h>
#include <http_request.h>
#include <http_protocol.h>
#include <http_log.h>
#include <http_config.h>
#include <ap_config.h>
#include <ap_provider.h>
#include <ap_mmn.h>
#include <apr_uri.h>
#include <apr_lib.h>
#include <mod_dav.h>

#include "mod_dav_svn.h"
#include "mod_authz_svn.h"
#include "svn_path.h"
#include "svn_config.h"
#include "svn_string.h"
#include "svn_repos.h"
#include "svn_pools.h"
#include "svn_dirent_uri.h"
#include "private/svn_fspath.h"


extern module AP_MODULE_DECLARE_DATA authz_svn_module;

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(authz_svn);
#endif

typedef struct authz_svn_config_rec {
  int authoritative;
  int anonymous;
  int no_auth_when_anon_ok;
  const char *base_path;
  const char *access_file;
  const char *repo_relative_access_file;
  const char *force_username_case;
} authz_svn_config_rec;

/*
 * Configuration
 */

/* Implements the #create_dir_config method of Apache's #module vtable. */
static void *
create_authz_svn_dir_config(apr_pool_t *p, char *d)
{
  authz_svn_config_rec *conf = apr_pcalloc(p, sizeof(*conf));
  conf->base_path = d;

  if (d)
    conf->base_path = svn_urlpath__canonicalize(d, p);

  /* By default keep the fortress secure */
  conf->authoritative = 1;
  conf->anonymous = 1;

  return conf;
}

static const char *
AuthzSVNAccessFile_cmd(cmd_parms *cmd, void *config, const char *arg1)
{
  authz_svn_config_rec *conf = config;

  if (conf->repo_relative_access_file != NULL)
    return "AuthzSVNAccessFile and AuthzSVNReposRelativeAccessFile "
           "directives are mutually exclusive.";

  conf->access_file = ap_server_root_relative(cmd->pool, arg1);

  return NULL;
}


static const char *
AuthzSVNReposRelativeAccessFile_cmd(cmd_parms *cmd,
                                    void *config,
                                    const char *arg1)
{
  authz_svn_config_rec *conf = config;

  if (conf->access_file != NULL)
    return "AuthzSVNAccessFile and AuthzSVNReposRelativeAccessFile "
           "directives are mutually exclusive.";

  conf->repo_relative_access_file = arg1;

  return NULL;
}

/* Implements the #cmds member of Apache's #module vtable. */
static const command_rec authz_svn_cmds[] =
{
  AP_INIT_FLAG("AuthzSVNAuthoritative", ap_set_flag_slot,
               (void *)APR_OFFSETOF(authz_svn_config_rec, authoritative),
               OR_AUTHCFG,
               "Set to 'Off' to allow access control to be passed along to "
               "lower modules. (default is On.)"),
  AP_INIT_TAKE1("AuthzSVNAccessFile", AuthzSVNAccessFile_cmd,
                NULL,
                OR_AUTHCFG,
                "Path to text file containing permissions of repository "
                "paths."),
  AP_INIT_TAKE1("AuthzSVNReposRelativeAccessFile",
                AuthzSVNReposRelativeAccessFile_cmd,
                NULL,
                OR_AUTHCFG,
                "Path (relative to repository 'conf' directory) to text "
                "file containing permissions of repository paths. "),
  AP_INIT_FLAG("AuthzSVNAnonymous", ap_set_flag_slot,
               (void *)APR_OFFSETOF(authz_svn_config_rec, anonymous),
               OR_AUTHCFG,
               "Set to 'Off' to disable two special-case behaviours of "
               "this module: (1) interaction with the 'Satisfy Any' "
               "directive, and (2) enforcement of the authorization "
               "policy even when no 'Require' directives are present. "
               "(default is On.)"),
  AP_INIT_FLAG("AuthzSVNNoAuthWhenAnonymousAllowed", ap_set_flag_slot,
               (void *)APR_OFFSETOF(authz_svn_config_rec,
                                    no_auth_when_anon_ok),
               OR_AUTHCFG,
               "Set to 'On' to suppress authentication and authorization "
               "for requests which anonymous users are allowed to perform. "
               "(default is Off.)"),
  AP_INIT_TAKE1("AuthzForceUsernameCase", ap_set_string_slot,
                (void *)APR_OFFSETOF(authz_svn_config_rec,
                                     force_username_case),
                OR_AUTHCFG,
                "Set to 'Upper' or 'Lower' to convert the username before "
                "checking for authorization."),
  { NULL }
};

/*
 * Get the, possibly cached, svn_authz_t for this request.
 */
static svn_authz_t *
get_access_conf(request_rec *r, authz_svn_config_rec *conf,
                apr_pool_t *scratch_pool)
{
  const char *cache_key = NULL;
  const char *access_file;
  const char *repos_path;
  void *user_data = NULL;
  svn_authz_t *access_conf = NULL;
  svn_error_t *svn_err;
  dav_error *dav_err;
  char errbuf[256];

  if (conf->repo_relative_access_file)
    {
      dav_err = dav_svn_get_repos_path(r, conf->base_path, &repos_path);
      if (dav_err) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, "%s", dav_err->desc);
        return NULL;
      }
      access_file = svn_dirent_join_many(scratch_pool, repos_path, "conf",
                                         conf->repo_relative_access_file,
                                         NULL);
    }
  else
    {
      access_file = conf->access_file;
    }

  ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r,
                "Path to authz file is %s", access_file);

  cache_key = apr_pstrcat(scratch_pool, "mod_authz_svn:",
                          access_file, (char *)NULL);
  apr_pool_userdata_get(&user_data, cache_key, r->connection->pool);
  access_conf = user_data;
  if (access_conf == NULL)
    {
      svn_err = svn_repos_authz_read(&access_conf, access_file,
                                     TRUE, r->connection->pool);
      if (svn_err)
        {
          ap_log_rerror(APLOG_MARK, APLOG_ERR,
                        /* If it is an error code that APR can make sense
                           of, then show it, otherwise, pass zero to avoid
                           putting "APR does not understand this error code"
                           in the error log. */
                        ((svn_err->apr_err >= APR_OS_START_USERERR &&
                          svn_err->apr_err < APR_OS_START_CANONERR) ?
                         0 : svn_err->apr_err),
                        r, "Failed to load the AuthzSVNAccessFile: %s",
                        svn_err_best_message(svn_err, errbuf, sizeof(errbuf)));
          svn_error_clear(svn_err);
          access_conf = NULL;
        }
      else
        {
          /* Cache the open repos for the next request on this connection */
          apr_pool_userdata_set(access_conf, cache_key,
                                NULL, r->connection->pool);
        }
    }
  return access_conf;
}

/* Convert TEXT to upper case if TO_UPPERCASE is TRUE, else
   converts it to lower case. */
static void
convert_case(char *text, svn_boolean_t to_uppercase)
{
  char *c = text;
  while (*c)
    {
      *c = (to_uppercase ? apr_toupper(*c) : apr_tolower(*c));
      ++c;
    }
}

/* Return the username to authorize, with case-conversion performed if
   CONF->force_username_case is set. */
static char *
get_username_to_authorize(request_rec *r, authz_svn_config_rec *conf,
                          apr_pool_t *pool)
{
  char *username_to_authorize = r->user;
  if (username_to_authorize && conf->force_username_case)
    {
      username_to_authorize = apr_pstrdup(pool, r->user);
      convert_case(username_to_authorize,
                   strcasecmp(conf->force_username_case, "upper") == 0);
    }
  return username_to_authorize;
}

/* Check if the current request R is allowed.  Upon exit *REPOS_PATH_REF
 * will contain the path and repository name that an operation was requested
 * on in the form 'name:path'.  *DEST_REPOS_PATH_REF will contain the
 * destination path if the requested operation was a MOVE or a COPY.
 * Returns OK when access is allowed, DECLINED when it isn't, or an HTTP_
 * error code when an error occurred.
 */
static int
req_check_access(request_rec *r,
                 authz_svn_config_rec *conf,
                 const char **repos_path_ref,
                 const char **dest_repos_path_ref)
{
  const char *dest_uri;
  apr_uri_t parsed_dest_uri;
  const char *cleaned_uri;
  int trailing_slash;
  const char *repos_name;
  const char *dest_repos_name;
  const char *relative_path;
  const char *repos_path;
  const char *dest_repos_path = NULL;
  dav_error *dav_err;
  svn_repos_authz_access_t authz_svn_type = svn_authz_none;
  svn_boolean_t authz_access_granted = FALSE;
  svn_authz_t *access_conf = NULL;
  svn_error_t *svn_err;
  char errbuf[256];
  const char *username_to_authorize = get_username_to_authorize(r, conf,
                                                                r->pool);

  switch (r->method_number)
    {
      /* All methods requiring read access to all subtrees of r->uri */
      case M_COPY:
        authz_svn_type |= svn_authz_recursive;

      /* All methods requiring read access to r->uri */
      case M_OPTIONS:
      case M_GET:
      case M_PROPFIND:
      case M_REPORT:
        authz_svn_type |= svn_authz_read;
        break;

      /* All methods requiring write access to all subtrees of r->uri */
      case M_MOVE:
      case M_DELETE:
        authz_svn_type |= svn_authz_recursive;

      /* All methods requiring write access to r->uri */
      case M_MKCOL:
      case M_PUT:
      case M_PROPPATCH:
      case M_CHECKOUT:
      case M_MERGE:
      case M_MKACTIVITY:
      case M_LOCK:
      case M_UNLOCK:
        authz_svn_type |= svn_authz_write;
        break;

      default:
        /* Require most strict access for unknown methods */
        authz_svn_type |= svn_authz_write | svn_authz_recursive;
        break;
    }

  if (strcmp(svn_urlpath__canonicalize(r->uri, r->pool), conf->base_path) == 0)
    {
      /* Do no access control when conf->base_path(as configured in <Location>)
       * and given uri are same. The reason for such relaxation of access
       * control is "This module is meant to control access inside the
       * repository path, in this case inside PATH is empty and hence
       * dav_svn_split_uri fails saying no repository name present".
       * One may ask it will allow access to '/' inside the repository if
       * repository is served via SVNPath instead of SVNParentPath.
       * It does not, The other methods(PROPFIND, MKACTIVITY) for
       * accomplishing the operation takes care of making a request to
       * proper URL */
      return OK;
    }

  dav_err = dav_svn_split_uri(r,
                              r->uri,
                              conf->base_path,
                              &cleaned_uri,
                              &trailing_slash,
                              &repos_name,
                              &relative_path,
                              &repos_path);
  if (dav_err)
    {
      ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                    "%s  [%d, #%d]",
                    dav_err->desc, dav_err->status, dav_err->error_id);
      /* Ensure that we never allow access by dav_err->status */
      return (dav_err->status != OK && dav_err->status != DECLINED) ?
              dav_err->status : HTTP_INTERNAL_SERVER_ERROR;
    }

  /* Ignore the URI passed to MERGE, like mod_dav_svn does.
   * See issue #1821.
   * XXX: When we start accepting a broader range of DeltaV MERGE
   * XXX: requests, this should be revisited.
   */
  if (r->method_number == M_MERGE)
    repos_path = NULL;

  if (repos_path)
    repos_path = svn_fspath__canonicalize(repos_path, r->pool);

  *repos_path_ref = apr_pstrcat(r->pool, repos_name, ":", repos_path,
                                (char *)NULL);

  if (r->method_number == M_MOVE || r->method_number == M_COPY)
    {
      dest_uri = apr_table_get(r->headers_in, "Destination");

      /* Decline MOVE or COPY when there is no Destination uri, this will
       * cause failure.
       */
      if (!dest_uri)
        return DECLINED;

      apr_uri_parse(r->pool, dest_uri, &parsed_dest_uri);

      ap_unescape_url(parsed_dest_uri.path);
      dest_uri = parsed_dest_uri.path;
      if (strncmp(dest_uri, conf->base_path, strlen(conf->base_path)))
        {
          /* If it is not the same location, then we don't allow it.
           * XXX: Instead we could compare repository uuids, but that
           * XXX: seems a bit over the top.
           */
          return HTTP_BAD_REQUEST;
        }

      dav_err = dav_svn_split_uri(r,
                                  dest_uri,
                                  conf->base_path,
                                  &cleaned_uri,
                                  &trailing_slash,
                                  &dest_repos_name,
                                  &relative_path,
                                  &dest_repos_path);

      if (dav_err)
        {
          ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                        "%s  [%d, #%d]",
                        dav_err->desc, dav_err->status, dav_err->error_id);
          /* Ensure that we never allow access by dav_err->status */
          return (dav_err->status != OK && dav_err->status != DECLINED) ?
                  dav_err->status : HTTP_INTERNAL_SERVER_ERROR;
        }

      if (dest_repos_path)
        dest_repos_path = svn_fspath__canonicalize(dest_repos_path, r->pool);

      *dest_repos_path_ref = apr_pstrcat(r->pool, dest_repos_name, ":",
                                         dest_repos_path, (char *)NULL);
    }

  /* Retrieve/cache authorization file */
  access_conf = get_access_conf(r,conf, r->pool);
  if (access_conf == NULL)
    return DECLINED;

  /* Perform authz access control.
   *
   * First test the special case where repos_path == NULL, and skip
   * calling the authz routines in that case.  This is an oddity of
   * the DAV RA method: some requests have no repos_path, but apache
   * still triggers an authz lookup for the URI.
   *
   * However, if repos_path == NULL and the request requires write
   * access, then perform a global authz lookup.  The request is
   * denied if the user commiting isn't granted any access anywhere
   * in the repository.  This is to avoid operations that involve no
   * paths (commiting an empty revision, leaving a dangling
   * transaction in the FS) being granted by default, letting
   * unauthenticated users write some changes to the repository.
   * This was issue #2388.
   *
   * XXX: For now, requesting access to the entire repository always
   * XXX: succeeds, until we come up with a good way of figuring
   * XXX: this out.
   */
  if (repos_path
      || (!repos_path && (authz_svn_type & svn_authz_write)))
    {
      svn_err = svn_repos_authz_check_access(access_conf, repos_name,
                                             repos_path,
                                             username_to_authorize,
                                             authz_svn_type,
                                             &authz_access_granted,
                                             r->pool);
      if (svn_err)
        {
          ap_log_rerror(APLOG_MARK, APLOG_ERR,
                        /* If it is an error code that APR can make
                           sense of, then show it, otherwise, pass
                           zero to avoid putting "APR does not
                           understand this error code" in the error
                           log. */
                        ((svn_err->apr_err >= APR_OS_START_USERERR &&
                          svn_err->apr_err < APR_OS_START_CANONERR) ?
                          0 : svn_err->apr_err),
                         r, "Failed to perform access control: %s",
                         svn_err_best_message(svn_err, errbuf,
                                              sizeof(errbuf)));
          svn_error_clear(svn_err);

          return DECLINED;
        }
        if (!authz_access_granted)
          return DECLINED;
    }

  /* XXX: MKCOL, MOVE, DELETE
   * XXX: Require write access to the parent dir of repos_path.
   */

  /* XXX: PUT
   * XXX: If the path doesn't exist, require write access to the
   * XXX: parent dir of repos_path.
   */

  /* Only MOVE and COPY have a second uri we have to check access to. */
  if (r->method_number != M_MOVE && r->method_number != M_COPY)
    return OK;

  /* Check access on the destination repos_path.  Again, skip this if
     repos_path == NULL (see above for explanations) */
  if (repos_path)
    {
      svn_err = svn_repos_authz_check_access(access_conf,
                                             dest_repos_name,
                                             dest_repos_path,
                                             username_to_authorize,
                                             svn_authz_write
                                             |svn_authz_recursive,
                                             &authz_access_granted,
                                             r->pool);
      if (svn_err)
        {
          ap_log_rerror(APLOG_MARK, APLOG_ERR,
                        /* If it is an error code that APR can make sense
                           of, then show it, otherwise, pass zero to avoid
                           putting "APR does not understand this error code"
                           in the error log. */
                        ((svn_err->apr_err >= APR_OS_START_USERERR &&
                          svn_err->apr_err < APR_OS_START_CANONERR) ?
                         0 : svn_err->apr_err),
                        r, "Failed to perform access control: %s",
                        svn_err_best_message(svn_err, errbuf, sizeof(errbuf)));
          svn_error_clear(svn_err);

          return DECLINED;
        }
      if (!authz_access_granted)
        return DECLINED;
    }

  /* XXX: MOVE and COPY, if the path doesn't exist yet, also
   * XXX: require write access to the parent dir of dest_repos_path.
   */

  return OK;
}

/* The macros LOG_ARGS_SIGNATURE and LOG_ARGS_CASCADE are expanded as formal
 * and actual parameters to log_access_verdict with respect to HTTPD version.
 */
#if AP_MODULE_MAGIC_AT_LEAST(20100606,0)
#define LOG_ARGS_SIGNATURE const char *file, int line, int module_index
#define LOG_ARGS_CASCADE file, line, module_index
#else
#define LOG_ARGS_SIGNATURE const char *file, int line
#define LOG_ARGS_CASCADE file, line
#endif

/* Log a message indicating the access control decision made about a
 * request.  The macro LOG_ARGS_SIGNATURE expands to FILE, LINE and
 * MODULE_INDEX in HTTPD 2.3 as APLOG_MARK macro has been changed for
 * per-module loglevel configuration.  It expands to FILE and LINE
 * in older server versions.  ALLOWED is boolean.
 * REPOS_PATH and DEST_REPOS_PATH are information
 * about the request.  DEST_REPOS_PATH may be NULL. */
static void
log_access_verdict(LOG_ARGS_SIGNATURE,
                   const request_rec *r, int allowed,
                   const char *repos_path, const char *dest_repos_path)
{
  int level = allowed ? APLOG_INFO : APLOG_ERR;
  const char *verdict = allowed ? "granted" : "denied";

  if (r->user)
    {
      if (dest_repos_path)
        ap_log_rerror(LOG_ARGS_CASCADE, level, 0, r,
                      "Access %s: '%s' %s %s %s", verdict, r->user,
                      r->method, repos_path, dest_repos_path);
      else
        ap_log_rerror(LOG_ARGS_CASCADE, level, 0, r,
                      "Access %s: '%s' %s %s", verdict, r->user,
                      r->method, repos_path);
    }
  else
    {
      if (dest_repos_path)
        ap_log_rerror(LOG_ARGS_CASCADE, level, 0, r,
                      "Access %s: - %s %s %s", verdict,
                      r->method, repos_path, dest_repos_path);
      else
        ap_log_rerror(LOG_ARGS_CASCADE, level, 0, r,
                      "Access %s: - %s %s", verdict,
                      r->method, repos_path);
    }
}

/*
 * Implementation of subreq_bypass with scratch_pool parameter.
 */
static int
subreq_bypass2(request_rec *r,
               const char *repos_path,
               const char *repos_name,
               apr_pool_t *scratch_pool)
{
  svn_error_t *svn_err = NULL;
  svn_authz_t *access_conf = NULL;
  authz_svn_config_rec *conf = NULL;
  svn_boolean_t authz_access_granted = FALSE;
  char errbuf[256];
  const char *username_to_authorize;

  conf = ap_get_module_config(r->per_dir_config,
                              &authz_svn_module);
  username_to_authorize = get_username_to_authorize(r, conf, scratch_pool);

  /* If configured properly, this should never be true, but just in case. */
  if (!conf->anonymous
      || (! (conf->access_file || conf->repo_relative_access_file)))
    {
      log_access_verdict(APLOG_MARK, r, 0, repos_path, NULL);
      return HTTP_FORBIDDEN;
    }

  /* Retrieve authorization file */
  access_conf = get_access_conf(r, conf, scratch_pool);
  if (access_conf == NULL)
    return HTTP_FORBIDDEN;

  /* Perform authz access control.
   * See similarly labeled comment in req_check_access.
   */
  if (repos_path)
    {
      svn_err = svn_repos_authz_check_access(access_conf, repos_name,
                                             repos_path,
                                             username_to_authorize,
                                             svn_authz_none|svn_authz_read,
                                             &authz_access_granted,
                                             scratch_pool);
      if (svn_err)
        {
          ap_log_rerror(APLOG_MARK, APLOG_ERR,
                        /* If it is an error code that APR can make
                           sense of, then show it, otherwise, pass
                           zero to avoid putting "APR does not
                           understand this error code" in the error
                           log. */
                        ((svn_err->apr_err >= APR_OS_START_USERERR &&
                          svn_err->apr_err < APR_OS_START_CANONERR) ?
                         0 : svn_err->apr_err),
                        r, "Failed to perform access control: %s",
                        svn_err_best_message(svn_err, errbuf, sizeof(errbuf)));
          svn_error_clear(svn_err);
          return HTTP_FORBIDDEN;
        }
      if (!authz_access_granted)
        {
          log_access_verdict(APLOG_MARK, r, 0, repos_path, NULL);
          return HTTP_FORBIDDEN;
        }
    }

  log_access_verdict(APLOG_MARK, r, 1, repos_path, NULL);

  return OK;
}

/*
 * This function is used as a provider to allow mod_dav_svn to bypass the
 * generation of an apache request when checking GET access from
 * "mod_dav_svn/authz.c" .
 */
static int
subreq_bypass(request_rec *r,
              const char *repos_path,
              const char *repos_name)
{
  int status;
  apr_pool_t *scratch_pool;

  scratch_pool = svn_pool_create(r->pool);
  status = subreq_bypass2(r, repos_path, repos_name, scratch_pool);
  svn_pool_destroy(scratch_pool);

  return status;
}

/*
 * Hooks
 */

static int
access_checker(request_rec *r)
{
  authz_svn_config_rec *conf = ap_get_module_config(r->per_dir_config,
                                                    &authz_svn_module);
  const char *repos_path = NULL;
  const char *dest_repos_path = NULL;
  int status;

  /* We are not configured to run */
  if (!conf->anonymous
      || (! (conf->access_file || conf->repo_relative_access_file)))
    return DECLINED;

  if (ap_some_auth_required(r))
    {
      /* It makes no sense to check if a location is both accessible
       * anonymous and by an authenticated user (in the same request!).
       */
      if (ap_satisfies(r) != SATISFY_ANY)
        return DECLINED;

      /* If the user is trying to authenticate, let him.  If anonymous
       * access is allowed, so is authenticated access, by definition
       * of the meaning of '*' in the access file.
       */
      if (apr_table_get(r->headers_in,
                        (PROXYREQ_PROXY == r->proxyreq)
                        ? "Proxy-Authorization" : "Authorization"))
        {
          /* Given Satisfy Any is in effect, we have to forbid access
           * to let the auth_checker hook have a go at it.
           */
          return HTTP_FORBIDDEN;
        }
    }

  /* If anon access is allowed, return OK */
  status = req_check_access(r, conf, &repos_path, &dest_repos_path);
  if (status == DECLINED)
    {
      if (!conf->authoritative)
        return DECLINED;

      if (!ap_some_auth_required(r))
        log_access_verdict(APLOG_MARK, r, 0, repos_path, dest_repos_path);

      return HTTP_FORBIDDEN;
    }

  if (status != OK)
    return status;

  log_access_verdict(APLOG_MARK, r, 1, repos_path, dest_repos_path);

  return OK;
}

static int
check_user_id(request_rec *r)
{
  authz_svn_config_rec *conf = ap_get_module_config(r->per_dir_config,
                                                    &authz_svn_module);
  const char *repos_path = NULL;
  const char *dest_repos_path = NULL;
  int status;

  /* We are not configured to run, or, an earlier module has already
   * authenticated this request. */
  if (!conf->no_auth_when_anon_ok || r->user
      || (! (conf->access_file || conf->repo_relative_access_file)))
    return DECLINED;

  /* If anon access is allowed, return OK, preventing later modules
   * from issuing an HTTP_UNAUTHORIZED.  Also pass a note to our
   * auth_checker hook that access has already been checked. */
  status = req_check_access(r, conf, &repos_path, &dest_repos_path);
  if (status == OK)
    {
      apr_table_setn(r->notes, "authz_svn-anon-ok", (const char*)1);
      log_access_verdict(APLOG_MARK, r, 1, repos_path, dest_repos_path);
      return OK;
    }

  return status;
}

static int
auth_checker(request_rec *r)
{
  authz_svn_config_rec *conf = ap_get_module_config(r->per_dir_config,
                                                    &authz_svn_module);
  const char *repos_path = NULL;
  const char *dest_repos_path = NULL;
  int status;

  /* We are not configured to run */
  if (! (conf->access_file || conf->repo_relative_access_file))
    return DECLINED;

  /* Previous hook (check_user_id) already did all the work,
   * and, as a sanity check, r->user hasn't been set since then? */
  if (!r->user && apr_table_get(r->notes, "authz_svn-anon-ok"))
    return OK;

  status = req_check_access(r, conf, &repos_path, &dest_repos_path);
  if (status == DECLINED)
    {
      if (conf->authoritative)
        {
          log_access_verdict(APLOG_MARK, r, 0, repos_path, dest_repos_path);
          ap_note_auth_failure(r);
          return HTTP_FORBIDDEN;
        }
      return DECLINED;
    }

  if (status != OK)
    return status;

  log_access_verdict(APLOG_MARK, r, 1, repos_path, dest_repos_path);

  return OK;
}

/*
 * Module flesh
 */

/* Implements the #register_hooks method of Apache's #module vtable. */
static void
register_hooks(apr_pool_t *p)
{
  static const char * const mod_ssl[] = { "mod_ssl.c", NULL };

  ap_hook_access_checker(access_checker, NULL, NULL, APR_HOOK_LAST);
  /* Our check_user_id hook must be before any module which will return
   * HTTP_UNAUTHORIZED (mod_auth_basic, etc.), but after mod_ssl, to
   * give SSLOptions +FakeBasicAuth a chance to work. */
  ap_hook_check_user_id(check_user_id, mod_ssl, NULL, APR_HOOK_FIRST);
  ap_hook_auth_checker(auth_checker, NULL, NULL, APR_HOOK_FIRST);
  ap_register_provider(p,
                       AUTHZ_SVN__SUBREQ_BYPASS_PROV_GRP,
                       AUTHZ_SVN__SUBREQ_BYPASS_PROV_NAME,
                       AUTHZ_SVN__SUBREQ_BYPASS_PROV_VER,
                       (void*)subreq_bypass);
}

module AP_MODULE_DECLARE_DATA authz_svn_module =
{
  STANDARD20_MODULE_STUFF,
  create_authz_svn_dir_config,     /* dir config creater */
  NULL,                            /* dir merger --- default is to override */
  NULL,                            /* server config */
  NULL,                            /* merge server config */
  authz_svn_cmds,                  /* command apr_table_t */
  register_hooks                   /* register hooks */
};
