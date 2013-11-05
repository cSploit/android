/*
 * mod_dav_svn.c: an Apache mod_dav sub-module to provide a Subversion
 *                repository.
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

#include <apr_strings.h>

#include <httpd.h>
#include <http_config.h>
#include <http_request.h>
#include <http_log.h>
#include <ap_provider.h>
#include <mod_dav.h>

#include "svn_version.h"
#include "svn_cache_config.h"
#include "svn_utf.h"
#include "svn_ctype.h"
#include "svn_dso.h"
#include "mod_dav_svn.h"

#include "private/svn_fspath.h"

#include "dav_svn.h"
#include "mod_authz_svn.h"


/* This is the default "special uri" used for SVN's special resources
   (e.g. working resources, activities) */
#define SVN_DEFAULT_SPECIAL_URI "!svn"

/* This is the value to be given to SVNPathAuthz to bypass the apache
 * subreq mechanism and make a call directly to mod_authz_svn. */
#define PATHAUTHZ_BYPASS_ARG "short_circuit"

/* per-server configuration */
typedef struct server_conf_t {
  const char *special_uri;
} server_conf_t;


/* A tri-state enum used for per directory on/off flags.  Note that
   it's important that CONF_FLAG_DEFAULT is 0 to make
   dav_svn_merge_dir_config do the right thing. */
enum conf_flag {
  CONF_FLAG_DEFAULT,
  CONF_FLAG_ON,
  CONF_FLAG_OFF
};

/* An enum used for the per directory configuration path_authz_method. */
enum path_authz_conf {
  CONF_PATHAUTHZ_DEFAULT,
  CONF_PATHAUTHZ_ON,
  CONF_PATHAUTHZ_OFF,
  CONF_PATHAUTHZ_BYPASS
};

/* per-dir configuration */
typedef struct dir_conf_t {
  const char *fs_path;               /* path to the SVN FS */
  const char *repo_name;             /* repository name */
  const char *xslt_uri;              /* XSL transform URI */
  const char *fs_parent_path;        /* path to parent of SVN FS'es  */
  enum conf_flag autoversioning;     /* whether autoversioning is active */
  enum conf_flag bulk_updates;       /* whether bulk updates are allowed */
  enum conf_flag v2_protocol;        /* whether HTTP v2 is advertised */
  enum path_authz_conf path_authz_method; /* how GET subrequests are handled */
  enum conf_flag list_parentpath;    /* whether to allow GET of parentpath */
  const char *root_dir;              /* our top-level directory */
  const char *master_uri;            /* URI to the master SVN repos */
  const char *activities_db;         /* path to activities database(s) */
  enum conf_flag txdelta_cache;      /* whether to enable txdelta caching */
  enum conf_flag fulltext_cache;     /* whether to enable fulltext caching */
} dir_conf_t;


#define INHERIT_VALUE(parent, child, field) \
                ((child)->field ? (child)->field : (parent)->field)


extern module AP_MODULE_DECLARE_DATA dav_svn_module;

/* The authz_svn provider for bypassing path authz. */
static authz_svn__subreq_bypass_func_t pathauthz_bypass_func = NULL;

/* The compression level we will pass to svn_txdelta_to_svndiff3()
 * for wire-compression */
static int svn__compression_level = SVN_DELTA_COMPRESSION_LEVEL_DEFAULT;

static int
init(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s)
{
  svn_error_t *serr;
  ap_add_version_component(p, "SVN/" SVN_VER_NUMBER);

  serr = svn_fs_initialize(p);
  if (serr)
    {
      ap_log_perror(APLOG_MARK, APLOG_ERR, serr->apr_err, p,
                    "mod_dav_svn: error calling svn_fs_initialize: '%s'",
                    serr->message ? serr->message : "(no more info)");
      return HTTP_INTERNAL_SERVER_ERROR;
    }

  /* This returns void, so we can't check for error. */
  svn_utf_initialize(p);

  return OK;
}

static int
init_dso(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t *ptemp)
{
  /* This isn't ideal, we're not actually being called before any
     pool is created, but we are being called before the server or
     request pools are created, which is probably good enough for
     98% of cases. */

  svn_error_t *serr = svn_dso_initialize2();

  if (serr)
    {
      ap_log_perror(APLOG_MARK, APLOG_ERR, serr->apr_err, plog,
                    "mod_dav_svn: error calling svn_dso_initialize2: '%s'",
                    serr->message ? serr->message : "(no more info)");
      svn_error_clear(serr);
      return HTTP_INTERNAL_SERVER_ERROR;
    }

  return OK;
}

/* Implements the #create_server_config method of Apache's #module vtable. */
static void *
create_server_config(apr_pool_t *p, server_rec *s)
{
  return apr_pcalloc(p, sizeof(server_conf_t));
}


/* Implements the #merge_server_config method of Apache's #module vtable. */
static void *
merge_server_config(apr_pool_t *p, void *base, void *overrides)
{
  server_conf_t *parent = base;
  server_conf_t *child = overrides;
  server_conf_t *newconf;

  newconf = apr_pcalloc(p, sizeof(*newconf));

  newconf->special_uri = INHERIT_VALUE(parent, child, special_uri);

  return newconf;
}


/* Implements the #create_dir_config method of Apache's #module vtable. */
static void *
create_dir_config(apr_pool_t *p, char *dir)
{
  /* NOTE: dir==NULL creates the default per-dir config */
  dir_conf_t *conf = apr_pcalloc(p, sizeof(*conf));

  /* In subversion context dir is always considered to be coming from
     <Location /blah> directive. So we treat it as a urlpath. */
  if (dir)
    conf->root_dir = svn_urlpath__canonicalize(dir, p);
  conf->bulk_updates = CONF_FLAG_ON;
  conf->v2_protocol = CONF_FLAG_ON;

  return conf;
}


/* Implements the #merge_dir_config method of Apache's #module vtable. */
static void *
merge_dir_config(apr_pool_t *p, void *base, void *overrides)
{
  dir_conf_t *parent = base;
  dir_conf_t *child = overrides;
  dir_conf_t *newconf;

  newconf = apr_pcalloc(p, sizeof(*newconf));

  newconf->fs_path = INHERIT_VALUE(parent, child, fs_path);
  newconf->master_uri = INHERIT_VALUE(parent, child, master_uri);
  newconf->activities_db = INHERIT_VALUE(parent, child, activities_db);
  newconf->repo_name = INHERIT_VALUE(parent, child, repo_name);
  newconf->xslt_uri = INHERIT_VALUE(parent, child, xslt_uri);
  newconf->fs_parent_path = INHERIT_VALUE(parent, child, fs_parent_path);
  newconf->autoversioning = INHERIT_VALUE(parent, child, autoversioning);
  newconf->bulk_updates = INHERIT_VALUE(parent, child, bulk_updates);
  newconf->v2_protocol = INHERIT_VALUE(parent, child, v2_protocol);
  newconf->path_authz_method = INHERIT_VALUE(parent, child, path_authz_method);
  newconf->list_parentpath = INHERIT_VALUE(parent, child, list_parentpath);
  newconf->txdelta_cache = INHERIT_VALUE(parent, child, txdelta_cache);
  newconf->fulltext_cache = INHERIT_VALUE(parent, child, fulltext_cache);
  newconf->root_dir = INHERIT_VALUE(parent, child, root_dir);

  if (parent->fs_path)
    ap_log_error(APLOG_MARK, APLOG_WARNING, 0, NULL,
                 "mod_dav_svn: nested Location '%s' hinders access to '%s' "
                 "in SVNPath Location '%s'",
                 child->root_dir,
                 svn_fspath__skip_ancestor(parent->root_dir, child->root_dir),
                 parent->root_dir);

  return newconf;
}


static const char *
SVNReposName_cmd(cmd_parms *cmd, void *config, const char *arg1)
{
  dir_conf_t *conf = config;

  conf->repo_name = apr_pstrdup(cmd->pool, arg1);

  return NULL;
}


static const char *
SVNMasterURI_cmd(cmd_parms *cmd, void *config, const char *arg1)
{
  dir_conf_t *conf = config;
  apr_uri_t parsed_uri;
  const char *uri_base_name = "";

  /* SVNMasterURI requires mod_proxy and mod_proxy_http
   * (r->handler = "proxy-server" in mirror.c), make sure
   * they are present. */
  if (ap_find_linked_module("mod_proxy.c") == NULL)
    return "module mod_proxy not loaded, required for SVNMasterURI";
  if (ap_find_linked_module("mod_proxy_http.c") == NULL)
    return "module mod_proxy_http not loaded, required for SVNMasterURI";
  if (APR_SUCCESS != apr_uri_parse(cmd->pool, arg1, &parsed_uri))
    return "unable to parse SVNMasterURI value";
  if (parsed_uri.path)
    uri_base_name = svn_urlpath__basename(
                        svn_urlpath__canonicalize(parsed_uri.path, cmd->pool),
                        cmd->pool);
  if (! *uri_base_name)
    return "SVNMasterURI value must not be a server root";

  conf->master_uri = apr_pstrdup(cmd->pool, arg1);

  return NULL;
}


static const char *
SVNActivitiesDB_cmd(cmd_parms *cmd, void *config, const char *arg1)
{
  dir_conf_t *conf = config;

  conf->activities_db = apr_pstrdup(cmd->pool, arg1);

  return NULL;
}


static const char *
SVNIndexXSLT_cmd(cmd_parms *cmd, void *config, const char *arg1)
{
  dir_conf_t *conf = config;

  conf->xslt_uri = apr_pstrdup(cmd->pool, arg1);

  return NULL;
}


static const char *
SVNAutoversioning_cmd(cmd_parms *cmd, void *config, int arg)
{
  dir_conf_t *conf = config;

  if (arg)
    conf->autoversioning = CONF_FLAG_ON;
  else
    conf->autoversioning = CONF_FLAG_OFF;

  return NULL;
}


static const char *
SVNAllowBulkUpdates_cmd(cmd_parms *cmd, void *config, int arg)
{
  dir_conf_t *conf = config;

  if (arg)
    conf->bulk_updates = CONF_FLAG_ON;
  else
    conf->bulk_updates = CONF_FLAG_OFF;

  return NULL;
}


static const char *
SVNAdvertiseV2Protocol_cmd(cmd_parms *cmd, void *config, int arg)
{
  dir_conf_t *conf = config;

  if (arg)
    conf->v2_protocol = CONF_FLAG_ON;
  else
    conf->v2_protocol = CONF_FLAG_OFF;

  return NULL;
}


static const char *
SVNPathAuthz_cmd(cmd_parms *cmd, void *config, const char *arg1)
{
  dir_conf_t *conf = config;

  if (apr_strnatcasecmp("off", arg1) == 0)
    {
      conf->path_authz_method = CONF_PATHAUTHZ_OFF;
    }
  else if (apr_strnatcasecmp(PATHAUTHZ_BYPASS_ARG, arg1) == 0)
    {
      conf->path_authz_method = CONF_PATHAUTHZ_BYPASS;
      if (pathauthz_bypass_func == NULL)
        {
          pathauthz_bypass_func =
            ap_lookup_provider(AUTHZ_SVN__SUBREQ_BYPASS_PROV_GRP,
                               AUTHZ_SVN__SUBREQ_BYPASS_PROV_NAME,
                               AUTHZ_SVN__SUBREQ_BYPASS_PROV_VER);
        }
    }
  else if (apr_strnatcasecmp("on", arg1) == 0)
    {
      conf->path_authz_method = CONF_PATHAUTHZ_ON;
    }
  else
    {
      return "Unrecognized value for SVNPathAuthz directive";
    }

  return NULL;
}


static const char *
SVNListParentPath_cmd(cmd_parms *cmd, void *config, int arg)
{
  dir_conf_t *conf = config;

  if (arg)
    conf->list_parentpath = CONF_FLAG_ON;
  else
    conf->list_parentpath = CONF_FLAG_OFF;

  return NULL;
}


static const char *
SVNPath_cmd(cmd_parms *cmd, void *config, const char *arg1)
{
  dir_conf_t *conf = config;

  if (conf->fs_parent_path != NULL)
    return "SVNPath cannot be defined at same time as SVNParentPath.";

  conf->fs_path = svn_dirent_internal_style(arg1, cmd->pool);

  return NULL;
}


static const char *
SVNParentPath_cmd(cmd_parms *cmd, void *config, const char *arg1)
{
  dir_conf_t *conf = config;

  if (conf->fs_path != NULL)
    return "SVNParentPath cannot be defined at same time as SVNPath.";

  conf->fs_parent_path = svn_dirent_internal_style(arg1, cmd->pool);

  return NULL;
}


static const char *
SVNSpecialURI_cmd(cmd_parms *cmd, void *config, const char *arg1)
{
  server_conf_t *conf;
  char *uri;
  apr_size_t len;

  uri = apr_pstrdup(cmd->pool, arg1);

  /* apply a bit of processing to the thing:
     - eliminate .. and . components
     - eliminate double slashes
     - eliminate leading and trailing slashes
     */
  ap_getparents(uri);
  ap_no2slash(uri);
  if (*uri == '/')
    ++uri;
  len = strlen(uri);
  if (len > 0 && uri[len - 1] == '/')
    uri[--len] = '\0';
  if (len == 0)
    return "The special URI path must have at least one component.";

  conf = ap_get_module_config(cmd->server->module_config,
                              &dav_svn_module);
  conf->special_uri = uri;

  return NULL;
}

static const char *
SVNCacheTextDeltas_cmd(cmd_parms *cmd, void *config, int arg)
{
  dir_conf_t *conf = config;

  if (arg)
    conf->txdelta_cache = CONF_FLAG_ON;
  else
    conf->txdelta_cache = CONF_FLAG_OFF;

  return NULL;
}

static const char *
SVNCacheFullTexts_cmd(cmd_parms *cmd, void *config, int arg)
{
  dir_conf_t *conf = config;

  if (arg)
    conf->fulltext_cache = CONF_FLAG_ON;
  else
    conf->fulltext_cache = CONF_FLAG_OFF;

  return NULL;
}

static const char *
SVNInMemoryCacheSize_cmd(cmd_parms *cmd, void *config, const char *arg1)
{
  svn_cache_config_t settings = *svn_cache_config_get();

  apr_uint64_t value = 0;
  svn_error_t *err = svn_cstring_atoui64(&value, arg1);
  if (err)
    {
      svn_error_clear(err);
      return "Invalid decimal number for the SVN cache size.";
    }

  settings.cache_size = value * 0x400;

  svn_cache_config_set(&settings);

  return NULL;
}

static const char *
SVNCompressionLevel_cmd(cmd_parms *cmd, void *config, const char *arg1)
{
  int value = 0;
  svn_error_t *err = svn_cstring_atoi(&value, arg1);
  if (err)
    {
      svn_error_clear(err);
      return "Invalid decimal number for the SVN compression level.";
    }

  if ((value < SVN_DELTA_COMPRESSION_LEVEL_NONE)
      || (value > SVN_DELTA_COMPRESSION_LEVEL_MAX))
    return apr_psprintf(cmd->pool,
                        "%d is not a valid compression level. "
                        "The valid range is %d .. %d.",
                        value,
                        (int)SVN_DELTA_COMPRESSION_LEVEL_NONE,
                        (int)SVN_DELTA_COMPRESSION_LEVEL_MAX);

  svn__compression_level = value;

  return NULL;
}


/** Accessor functions for the module's configuration state **/

const char *
dav_svn__get_fs_path(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->fs_path;
}


const char *
dav_svn__get_fs_parent_path(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->fs_parent_path;
}


AP_MODULE_DECLARE(dav_error *)
dav_svn_get_repos_path(request_rec *r,
                       const char *root_path,
                       const char **repos_path)
{

  const char *fs_path;
  const char *fs_parent_path;
  const char *repos_name;
  const char *ignored_path_in_repos;
  const char *ignored_cleaned_uri;
  const char *ignored_relative;
  int ignored_had_slash;
  dav_error *derr;

  /* Handle the SVNPath case. */
  fs_path = dav_svn__get_fs_path(r);

  if (fs_path != NULL)
    {
      *repos_path = fs_path;
      return NULL;
    }

  /* Handle the SVNParentPath case.  If neither directive was used,
     dav_svn_split_uri will throw a suitable error for us - we do
     not need to check that here. */
  fs_parent_path = dav_svn__get_fs_parent_path(r);

  /* Split the svn URI to get the name of the repository below
     the parent path. */
  derr = dav_svn_split_uri(r, r->uri, root_path,
                           &ignored_cleaned_uri, &ignored_had_slash,
                           &repos_name,
                           &ignored_relative, &ignored_path_in_repos);
  if (derr)
    return derr;

  /* Construct the full path from the parent path base directory
     and the repository name. */
  *repos_path = svn_dirent_join(fs_parent_path, repos_name, r->pool);
  return NULL;
}


const char *
dav_svn__get_repo_name(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->repo_name;
}


const char *
dav_svn__get_root_dir(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->root_dir;
}


const char *
dav_svn__get_master_uri(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->master_uri;
}


const char *
dav_svn__get_xslt_uri(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->xslt_uri;
}


const char *
dav_svn__get_special_uri(request_rec *r)
{
  server_conf_t *conf;

  conf = ap_get_module_config(r->server->module_config,
                              &dav_svn_module);
  return conf->special_uri ? conf->special_uri : SVN_DEFAULT_SPECIAL_URI;
}


const char *
dav_svn__get_me_resource_uri(request_rec *r)
{
  return apr_pstrcat(r->pool, dav_svn__get_special_uri(r), "/me",
                     (char *)NULL);
}


const char *
dav_svn__get_rev_stub(request_rec *r)
{
  return apr_pstrcat(r->pool, dav_svn__get_special_uri(r), "/rev",
                     (char *)NULL);
}


const char *
dav_svn__get_rev_root_stub(request_rec *r)
{
  return apr_pstrcat(r->pool, dav_svn__get_special_uri(r), "/rvr",
                     (char *)NULL);
}


const char *
dav_svn__get_txn_stub(request_rec *r)
{
  return apr_pstrcat(r->pool, dav_svn__get_special_uri(r), "/txn",
                     (char *)NULL);
}


const char *
dav_svn__get_txn_root_stub(request_rec *r)
{
  return apr_pstrcat(r->pool, dav_svn__get_special_uri(r), "/txr", (char *)NULL);
}


const char *
dav_svn__get_vtxn_stub(request_rec *r)
{
  return apr_pstrcat(r->pool, dav_svn__get_special_uri(r), "/vtxn",
                     (char *)NULL);
}


const char *
dav_svn__get_vtxn_root_stub(request_rec *r)
{
  return apr_pstrcat(r->pool, dav_svn__get_special_uri(r), "/vtxr",
                     (char *)NULL);
}


svn_boolean_t
dav_svn__get_autoversioning_flag(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->autoversioning == CONF_FLAG_ON;
}


svn_boolean_t
dav_svn__get_bulk_updates_flag(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->bulk_updates == CONF_FLAG_ON;
}


svn_boolean_t
dav_svn__get_v2_protocol_flag(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->v2_protocol == CONF_FLAG_ON;
}


/* FALSE if path authorization should be skipped.
 * TRUE if either the bypass or the apache subrequest methods should be used.
 */
svn_boolean_t
dav_svn__get_pathauthz_flag(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->path_authz_method != CONF_PATHAUTHZ_OFF;
}

/* Function pointer if we should use the bypass directly to mod_authz_svn.
 * NULL otherwise. */
authz_svn__subreq_bypass_func_t
dav_svn__get_pathauthz_bypass(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);

  if (conf->path_authz_method == CONF_PATHAUTHZ_BYPASS)
    return pathauthz_bypass_func;
  return NULL;
}


svn_boolean_t
dav_svn__get_list_parentpath_flag(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->list_parentpath == CONF_FLAG_ON;
}


const char *
dav_svn__get_activities_db(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->activities_db;
}


svn_boolean_t
dav_svn__get_txdelta_cache_flag(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->txdelta_cache == CONF_FLAG_ON;
}


svn_boolean_t
dav_svn__get_fulltext_cache_flag(request_rec *r)
{
  dir_conf_t *conf;

  conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);
  return conf->fulltext_cache == CONF_FLAG_ON;
}


int
dav_svn__get_compression_level(void)
{
  return svn__compression_level;
}

static void
merge_xml_filter_insert(request_rec *r)
{
  /* We only care about MERGE and DELETE requests. */
  if ((r->method_number == M_MERGE)
      || (r->method_number == M_DELETE))
    {
      dir_conf_t *conf;
      conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);

      /* We only care if we are configured. */
      if (conf->fs_path || conf->fs_parent_path)
        {
          ap_add_input_filter("SVN-MERGE", NULL, r, r->connection);
        }
    }
}


typedef struct merge_ctx_t {
  apr_bucket_brigade *bb;
  apr_xml_parser *parser;
  apr_pool_t *pool;
} merge_ctx_t;


static apr_status_t
merge_xml_in_filter(ap_filter_t *f,
                    apr_bucket_brigade *bb,
                    ap_input_mode_t mode,
                    apr_read_type_e block,
                    apr_off_t readbytes)
{
  apr_status_t rv;
  request_rec *r = f->r;
  merge_ctx_t *ctx = f->ctx;
  apr_bucket *bucket;
  int seen_eos = 0;

  /* We shouldn't be added if we're not a MERGE/DELETE, but double check. */
  if ((r->method_number != M_MERGE)
      && (r->method_number != M_DELETE))
    {
      ap_remove_input_filter(f);
      return ap_get_brigade(f->next, bb, mode, block, readbytes);
    }

  if (!ctx)
    {
      f->ctx = ctx = apr_palloc(r->pool, sizeof(*ctx));
      ctx->parser = apr_xml_parser_create(r->pool);
      ctx->bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
      apr_pool_create(&ctx->pool, r->pool);
    }

  rv = ap_get_brigade(f->next, ctx->bb, mode, block, readbytes);

  if (rv != APR_SUCCESS)
    return rv;

  for (bucket = APR_BRIGADE_FIRST(ctx->bb);
       bucket != APR_BRIGADE_SENTINEL(ctx->bb);
       bucket = APR_BUCKET_NEXT(bucket))
    {
      const char *data;
      apr_size_t len;

      if (APR_BUCKET_IS_EOS(bucket))
        {
          seen_eos = 1;
          break;
        }

      if (APR_BUCKET_IS_METADATA(bucket))
        continue;

      rv = apr_bucket_read(bucket, &data, &len, APR_BLOCK_READ);
      if (rv != APR_SUCCESS)
        return rv;

      rv = apr_xml_parser_feed(ctx->parser, data, len);
      if (rv != APR_SUCCESS)
        {
          /* Clean up the parser. */
          (void) apr_xml_parser_done(ctx->parser, NULL);
          break;
        }
    }

  /* This will clear-out the ctx->bb as well. */
  APR_BRIGADE_CONCAT(bb, ctx->bb);

  if (seen_eos)
    {
      apr_xml_doc *pdoc;

      /* Remove ourselves now. */
      ap_remove_input_filter(f);

      /* tell the parser that we're done */
      rv = apr_xml_parser_done(ctx->parser, &pdoc);
      if (rv == APR_SUCCESS)
        {
#if APR_CHARSET_EBCDIC
          apr_xml_parser_convert_doc(r->pool, pdoc, ap_hdrs_from_ascii);
#endif
          /* stash the doc away for mod_dav_svn's later use. */
          rv = apr_pool_userdata_set(pdoc, "svn-request-body",
                                     NULL, r->pool);
          if (rv != APR_SUCCESS)
            return rv;

        }
    }

  return APR_SUCCESS;
}


/* Response handler for POST requests (protocol-v2 commits).  */
static int dav_svn__handler(request_rec *r)
{
  dir_conf_t *conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);

  if (conf->fs_path || conf->fs_parent_path)
    {
      /* HTTP-defined Methods we handle */
      r->allowed = 0
        | (AP_METHOD_BIT << M_POST);

      if (r->method_number == M_POST)
        return dav_svn__method_post(r);
    }

  return DECLINED;
}

#define NO_MAP_TO_STORAGE_NOTE "dav_svn-no-map-to-storage"

/* Prevent filename on the request from being set since we aren't serving a
 * file off the disk.  This means that <Directory> blocks will not match and
 * that * %f in logging formats will show as "-". */
static int dav_svn__translate_name(request_rec *r)
{
  dir_conf_t *conf = ap_get_module_config(r->per_dir_config, &dav_svn_module);

  /* module is not configured, bail out early */
  if (!conf->fs_path && !conf->fs_parent_path)
    return DECLINED;

  /* Be paranoid and set it to NULL just in case some other module set it
   * before we got called. */ 
  r->filename = NULL;

  /* Leave a note to ourselves so that we know not to decline in the 
   * map_to_storage hook. */
  apr_table_setn(r->notes, NO_MAP_TO_STORAGE_NOTE, (const char*)1); 
  return OK;
}

/* Prevent core_map_to_storage from running if we prevented the r->filename
 * from being set since core_map_to_storage doesn't like r->filename being
 * NULL. */
static int dav_svn__map_to_storage(request_rec *r)
{
  /* Check a note we left in translate_name since map_to_storage doesn't
   * have access to our configuration. */
  if (apr_table_get(r->notes, NO_MAP_TO_STORAGE_NOTE))
    return OK;

  return DECLINED;
}



/** Module framework stuff **/

/* Implements the #cmds member of Apache's #module vtable. */
static const command_rec cmds[] =
{
  /* per directory/location */
  AP_INIT_TAKE1("SVNPath", SVNPath_cmd, NULL, ACCESS_CONF,
                "specifies the location in the filesystem for a Subversion "
                "repository's files."),

  /* per server */
  AP_INIT_TAKE1("SVNSpecialURI", SVNSpecialURI_cmd, NULL, RSRC_CONF,
                "specify the URI component for special Subversion "
                "resources"),

  /* per directory/location */
  AP_INIT_TAKE1("SVNReposName", SVNReposName_cmd, NULL, ACCESS_CONF,
                "specify the name of a Subversion repository"),

  /* per directory/location */
  AP_INIT_TAKE1("SVNIndexXSLT", SVNIndexXSLT_cmd, NULL, ACCESS_CONF,
                "specify the URI of an XSL transformation for "
                "directory indexes"),

  /* per directory/location */
  AP_INIT_TAKE1("SVNParentPath", SVNParentPath_cmd, NULL, ACCESS_CONF,
                "specifies the location in the filesystem whose "
                "subdirectories are assumed to be Subversion repositories."),

  /* per directory/location */
  AP_INIT_FLAG("SVNAutoversioning", SVNAutoversioning_cmd, NULL,
               ACCESS_CONF|RSRC_CONF, "turn on deltaV autoversioning."),

  /* per directory/location */
  AP_INIT_TAKE1("SVNPathAuthz", SVNPathAuthz_cmd, NULL,
               ACCESS_CONF|RSRC_CONF,
               "control path-based authz by enabling subrequests(On,default), "
               "disabling subrequests(Off), or"
               "querying mod_authz_svn directly(" PATHAUTHZ_BYPASS_ARG ")"),

  /* per directory/location */
  AP_INIT_FLAG("SVNListParentPath", SVNListParentPath_cmd, NULL,
               ACCESS_CONF|RSRC_CONF, "allow GET of SVNParentPath."),

  /* per directory/location */
  AP_INIT_TAKE1("SVNMasterURI", SVNMasterURI_cmd, NULL, ACCESS_CONF,
                "specifies a URI to access a master Subversion repository"),

  /* per directory/location */
  AP_INIT_TAKE1("SVNActivitiesDB", SVNActivitiesDB_cmd, NULL, ACCESS_CONF,
                "specifies the location in the filesystem in which the "
                "activities database(s) should be stored"),

  /* per directory/location */
  AP_INIT_FLAG("SVNAllowBulkUpdates", SVNAllowBulkUpdates_cmd, NULL,
               ACCESS_CONF|RSRC_CONF,
               "enables support for bulk update-style requests (as opposed to "
               "only skeletal reports that require additional per-file "
               "downloads."),

  /* per directory/location */
  AP_INIT_FLAG("SVNAdvertiseV2Protocol", SVNAdvertiseV2Protocol_cmd, NULL,
               ACCESS_CONF|RSRC_CONF,
               "enables server advertising of support for version 2 of "
               "Subversion's HTTP protocol (default values is On)."),

  /* per directory/location */
  AP_INIT_FLAG("SVNCacheTextDeltas", SVNCacheTextDeltas_cmd, NULL,
               ACCESS_CONF|RSRC_CONF,
               "speeds up data access to older revisions by caching "
               "delta information if sufficient in-memory cache is "
               "available (default is Off)."),

  /* per directory/location */
  AP_INIT_FLAG("SVNCacheFullTexts", SVNCacheFullTexts_cmd, NULL,
               ACCESS_CONF|RSRC_CONF,
               "speeds up data access by caching full file content "
               "if sufficient in-memory cache is available "
               "(default is Off)."),

  /* per server */
  AP_INIT_TAKE1("SVNInMemoryCacheSize", SVNInMemoryCacheSize_cmd, NULL,
                RSRC_CONF,
                "specifies the maximum size in kB per process of Subversion's "
                "in-memory object cache (default value is 16384; 0 deactivates "
                "the cache)."),
  /* per server */
  AP_INIT_TAKE1("SVNCompressionLevel", SVNCompressionLevel_cmd, NULL,
                RSRC_CONF,
                "specifies the compression level used before sending file "
                "content over the network (0 for no compression, 9 for "
                "maximum, 5 is default)."),

  { NULL }
};


static dav_provider provider =
{
  &dav_svn__hooks_repository,
  &dav_svn__hooks_propdb,
  &dav_svn__hooks_locks,
  &dav_svn__hooks_vsn,
  NULL,                       /* binding */
  NULL                        /* search */
};


/* Implements the #register_hooks method of Apache's #module vtable. */
static void
register_hooks(apr_pool_t *pconf)
{
  ap_hook_pre_config(init_dso, NULL, NULL, APR_HOOK_REALLY_FIRST);
  ap_hook_post_config(init, NULL, NULL, APR_HOOK_MIDDLE);

  /* our provider */
  dav_register_provider(pconf, "svn", &provider);

  /* input filter to read MERGE bodies. */
  ap_register_input_filter("SVN-MERGE", merge_xml_in_filter, NULL,
                           AP_FTYPE_RESOURCE);
  ap_hook_insert_filter(merge_xml_filter_insert, NULL, NULL,
                        APR_HOOK_MIDDLE);

  /* general request handler for methods which mod_dav DECLINEs. */
  ap_hook_handler(dav_svn__handler, NULL, NULL, APR_HOOK_LAST);

  /* live property handling */
  dav_hook_gather_propsets(dav_svn__gather_propsets, NULL, NULL,
                           APR_HOOK_MIDDLE);
  dav_hook_find_liveprop(dav_svn__find_liveprop, NULL, NULL, APR_HOOK_MIDDLE);
  dav_hook_insert_all_liveprops(dav_svn__insert_all_liveprops, NULL, NULL,
                                APR_HOOK_MIDDLE);
  dav_register_liveprop_group(pconf, &dav_svn__liveprop_group);

  /* Proxy / mirroring filters and fixups */
  ap_register_output_filter("LocationRewrite", dav_svn__location_header_filter,
                            NULL, AP_FTYPE_CONTENT_SET);
  ap_register_output_filter("ReposRewrite", dav_svn__location_body_filter,
                            NULL, AP_FTYPE_CONTENT_SET);
  ap_register_input_filter("IncomingRewrite", dav_svn__location_in_filter,
                           NULL, AP_FTYPE_CONTENT_SET);
  ap_hook_fixups(dav_svn__proxy_request_fixup, NULL, NULL, APR_HOOK_MIDDLE);
  /* translate_name hook is LAST so that it doesn't interfere with modules
   * like mod_alias that are MIDDLE. */
  ap_hook_translate_name(dav_svn__translate_name, NULL, NULL, APR_HOOK_LAST);
  /* map_to_storage hook is LAST to avoid interferring with mod_http's
   * handling of OPTIONS and TRACE. */
  ap_hook_map_to_storage(dav_svn__map_to_storage, NULL, NULL, APR_HOOK_LAST);
}


module AP_MODULE_DECLARE_DATA dav_svn_module =
{
  STANDARD20_MODULE_STUFF,
  create_dir_config,    /* dir config creater */
  merge_dir_config,     /* dir merger --- default is to override */
  create_server_config, /* server config */
  merge_server_config,  /* merge server config */
  cmds,                 /* command table */
  register_hooks,       /* register hooks */
};
