/*
 * options.c :  entry point for OPTIONS RA functions for ra_serf
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



#include <apr_uri.h>

#include <serf.h>

#include "svn_dirent_uri.h"
#include "svn_pools.h"
#include "svn_ra.h"
#include "svn_dav.h"
#include "svn_xml.h"

#include "../libsvn_ra/ra_loader.h"
#include "svn_private_config.h"
#include "private/svn_fspath.h"

#include "ra_serf.h"


/* In a debug build, setting this environment variable to "yes" will force
   the client to speak v1, even if the server is capable of speaking v2. */
#define SVN_IGNORE_V2_ENV_VAR "SVN_I_LIKE_LATENCY_SO_IGNORE_HTTPV2"


/*
 * This enum represents the current state of our XML parsing for an OPTIONS.
 */
typedef enum options_state_e {
  OPTIONS,
  ACTIVITY_COLLECTION,
  HREF
} options_state_e;

typedef struct options_state_list_t {
  /* The current state that we are in now. */
  options_state_e state;

  /* The previous state we were in. */
  struct options_state_list_t *prev;
} options_state_list_t;

struct svn_ra_serf__options_context_t {
  /* pool to allocate memory from */
  apr_pool_t *pool;

  const char *attr_val;
  apr_size_t attr_val_len;
  svn_boolean_t collect_cdata;

  /* Current state we're in */
  options_state_list_t *state;
  options_state_list_t *free_state;

  /* HTTP Status code */
  int status_code;

  /* are we done? */
  svn_boolean_t done;

  svn_ra_serf__session_t *session;
  svn_ra_serf__connection_t *conn;

  const char *path;

  const char *activity_collection;
  svn_revnum_t youngest_rev;

  serf_response_acceptor_t acceptor;
  serf_response_handler_t handler;
  svn_ra_serf__xml_parser_t *parser_ctx;

};

static void
push_state(svn_ra_serf__options_context_t *options_ctx, options_state_e state)
{
  options_state_list_t *new_state;

  if (!options_ctx->free_state)
    {
      new_state = apr_palloc(options_ctx->pool, sizeof(*options_ctx->state));
    }
  else
    {
      new_state = options_ctx->free_state;
      options_ctx->free_state = options_ctx->free_state->prev;
    }
  new_state->state = state;

  /* Add it to the state chain. */
  new_state->prev = options_ctx->state;
  options_ctx->state = new_state;
}

static void pop_state(svn_ra_serf__options_context_t *options_ctx)
{
  options_state_list_t *free_state;
  free_state = options_ctx->state;
  /* advance the current state */
  options_ctx->state = options_ctx->state->prev;
  free_state->prev = options_ctx->free_state;
  options_ctx->free_state = free_state;
}

static svn_error_t *
start_options(svn_ra_serf__xml_parser_t *parser,
              void *userData,
              svn_ra_serf__dav_props_t name,
              const char **attrs)
{
  svn_ra_serf__options_context_t *options_ctx = userData;

  if (!options_ctx->state && strcmp(name.name, "options-response") == 0)
    {
      push_state(options_ctx, OPTIONS);
    }
  else if (!options_ctx->state)
    {
      /* Nothing to do. */
      return SVN_NO_ERROR;
    }
  else if (options_ctx->state->state == OPTIONS &&
           strcmp(name.name, "activity-collection-set") == 0)
    {
      push_state(options_ctx, ACTIVITY_COLLECTION);
    }
  else if (options_ctx->state->state == ACTIVITY_COLLECTION &&
           strcmp(name.name, "href") == 0)
    {
      options_ctx->collect_cdata = TRUE;
      push_state(options_ctx, HREF);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
end_options(svn_ra_serf__xml_parser_t *parser,
            void *userData,
            svn_ra_serf__dav_props_t name)
{
  svn_ra_serf__options_context_t *options_ctx = userData;
  options_state_list_t *cur_state;

  if (!options_ctx->state)
    {
      return SVN_NO_ERROR;
    }

  cur_state = options_ctx->state;

  if (cur_state->state == OPTIONS &&
      strcmp(name.name, "options-response") == 0)
    {
      pop_state(options_ctx);
    }
  else if (cur_state->state == ACTIVITY_COLLECTION &&
           strcmp(name.name, "activity-collection-set") == 0)
    {
      pop_state(options_ctx);
    }
  else if (cur_state->state == HREF &&
           strcmp(name.name, "href") == 0)
    {
      options_ctx->collect_cdata = FALSE;
      options_ctx->activity_collection =
        svn_urlpath__canonicalize(options_ctx->attr_val, options_ctx->pool);
      pop_state(options_ctx);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
cdata_options(svn_ra_serf__xml_parser_t *parser,
              void *userData,
              const char *data,
              apr_size_t len)
{
  svn_ra_serf__options_context_t *ctx = userData;
  if (ctx->collect_cdata)
    {
      svn_ra_serf__expand_string(&ctx->attr_val, &ctx->attr_val_len,
                                 data, len, ctx->pool);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
create_options_body(serf_bucket_t **body_bkt,
                    void *baton,
                    serf_bucket_alloc_t *alloc,
                    apr_pool_t *pool)
{
  serf_bucket_t *body;
  body = serf_bucket_aggregate_create(alloc);
  svn_ra_serf__add_xml_header_buckets(body, alloc);
  svn_ra_serf__add_open_tag_buckets(body, alloc, "D:options",
                                    "xmlns:D", "DAV:",
                                    NULL);
  svn_ra_serf__add_tag_buckets(body, "D:activity-collection-set", NULL, alloc);
  svn_ra_serf__add_close_tag_buckets(body, alloc, "D:options");

  *body_bkt = body;
  return SVN_NO_ERROR;
}

svn_boolean_t*
svn_ra_serf__get_options_done_ptr(svn_ra_serf__options_context_t *ctx)
{
  return &ctx->done;
}

const char *
svn_ra_serf__options_get_activity_collection(svn_ra_serf__options_context_t *ctx)
{
  return ctx->activity_collection;
}

svn_revnum_t
svn_ra_serf__options_get_youngest_rev(svn_ra_serf__options_context_t *ctx)
{
  return ctx->youngest_rev;
}

/* Context for both options_response_handler() and capabilities callback. */
struct options_response_ctx_t {
  /* Baton for __handle_xml_parser() */
  svn_ra_serf__xml_parser_t *parser_ctx;

  /* Session into which we'll store server capabilities */
  svn_ra_serf__session_t *session;

  /* For temporary work only. */
  apr_pool_t *pool;
};


/* We use these static pointers so we can employ pointer comparison
 * of our capabilities hash members instead of strcmp()ing all over
 * the place.
 */
/* Both server and repository support the capability. */
static const char *capability_yes = "yes";
/* Either server or repository does not support the capability. */
static const char *capability_no = "no";
/* Server supports the capability, but don't yet know if repository does. */
static const char *capability_server_yes = "server-yes";


/* This implements serf_bucket_headers_do_callback_fn_t.
 */
static int
capabilities_headers_iterator_callback(void *baton,
                                       const char *key,
                                       const char *val)
{
  struct options_response_ctx_t *orc = baton;

  if (svn_cstring_casecmp(key, "dav") == 0)
    {
      /* Each header may contain multiple values, separated by commas, e.g.:
           DAV: version-control,checkout,working-resource
           DAV: merge,baseline,activity,version-controlled-collection
           DAV: http://subversion.tigris.org/xmlns/dav/svn/depth */
      apr_array_header_t *vals = svn_cstring_split(val, ",", TRUE, orc->pool);

      /* Right now we only have a few capabilities to detect, so just
         seek for them directly.  This could be written slightly more
         efficiently, but that wouldn't be worth it until we have many
         more capabilities. */

      if (svn_cstring_match_list(SVN_DAV_NS_DAV_SVN_DEPTH, vals))
        {
          apr_hash_set(orc->session->capabilities, SVN_RA_CAPABILITY_DEPTH,
                       APR_HASH_KEY_STRING, capability_yes);
        }
      if (svn_cstring_match_list(SVN_DAV_NS_DAV_SVN_MERGEINFO, vals))
        {
          /* The server doesn't know what repository we're referring
             to, so it can't just say capability_yes. */
          apr_hash_set(orc->session->capabilities, SVN_RA_CAPABILITY_MERGEINFO,
                       APR_HASH_KEY_STRING, capability_server_yes);
        }
      if (svn_cstring_match_list(SVN_DAV_NS_DAV_SVN_LOG_REVPROPS, vals))
        {
          apr_hash_set(orc->session->capabilities,
                       SVN_RA_CAPABILITY_LOG_REVPROPS,
                       APR_HASH_KEY_STRING, capability_yes);
        }
      if (svn_cstring_match_list(SVN_DAV_NS_DAV_SVN_ATOMIC_REVPROPS, vals))
        {
          apr_hash_set(orc->session->capabilities,
                       SVN_RA_CAPABILITY_ATOMIC_REVPROPS,
                       APR_HASH_KEY_STRING, capability_yes);
        }
      if (svn_cstring_match_list(SVN_DAV_NS_DAV_SVN_PARTIAL_REPLAY, vals))
        {
          apr_hash_set(orc->session->capabilities,
                       SVN_RA_CAPABILITY_PARTIAL_REPLAY,
                       APR_HASH_KEY_STRING, capability_yes);
        }
    }

  /* SVN-specific headers -- if present, server supports HTTP protocol v2 */
  else if (strncmp(key, "SVN", 3) == 0)
    {
      if (svn_cstring_casecmp(key, SVN_DAV_ROOT_URI_HEADER) == 0)
        {
          orc->session->repos_root = orc->session->session_url;
          orc->session->repos_root.path =
            (char *)svn_fspath__canonicalize(val, orc->session->pool);
          orc->session->repos_root_str =
            svn_urlpath__canonicalize(
                apr_uri_unparse(orc->session->pool,
                                &orc->session->repos_root,
                                0),
                orc->session->pool);
        }
      else if (svn_cstring_casecmp(key, SVN_DAV_ME_RESOURCE_HEADER) == 0)
        {
#ifdef SVN_DEBUG
          char *ignore_v2_env_var = getenv(SVN_IGNORE_V2_ENV_VAR);

          if (!(ignore_v2_env_var
                && apr_strnatcasecmp(ignore_v2_env_var, "yes") == 0))
            orc->session->me_resource = apr_pstrdup(orc->session->pool, val);
#else
          orc->session->me_resource = apr_pstrdup(orc->session->pool, val);
#endif
        }
      else if (svn_cstring_casecmp(key, SVN_DAV_REV_STUB_HEADER) == 0)
        {
          orc->session->rev_stub = apr_pstrdup(orc->session->pool, val);
        }
      else if (svn_cstring_casecmp(key, SVN_DAV_REV_ROOT_STUB_HEADER) == 0)
        {
          orc->session->rev_root_stub = apr_pstrdup(orc->session->pool, val);
        }
      else if (svn_cstring_casecmp(key, SVN_DAV_TXN_STUB_HEADER) == 0)
        {
          orc->session->txn_stub = apr_pstrdup(orc->session->pool, val);
        }
      else if (svn_cstring_casecmp(key, SVN_DAV_TXN_ROOT_STUB_HEADER) == 0)
        {
          orc->session->txn_root_stub = apr_pstrdup(orc->session->pool, val);
        }
      else if (svn_cstring_casecmp(key, SVN_DAV_VTXN_STUB_HEADER) == 0)
        {
          orc->session->vtxn_stub = apr_pstrdup(orc->session->pool, val);
        }
      else if (svn_cstring_casecmp(key, SVN_DAV_VTXN_ROOT_STUB_HEADER) == 0)
        {
          orc->session->vtxn_root_stub = apr_pstrdup(orc->session->pool, val);
        }
      else if (svn_cstring_casecmp(key, SVN_DAV_REPOS_UUID_HEADER) == 0)
        {
          orc->session->uuid = apr_pstrdup(orc->session->pool, val);
        }
      else if (svn_cstring_casecmp(key, SVN_DAV_YOUNGEST_REV_HEADER) == 0)
        {
          struct svn_ra_serf__options_context_t *user_data =
            orc->parser_ctx->user_data;
          user_data->youngest_rev = SVN_STR_TO_REV(val);
        }
    }

  return 0;
}


/* A custom serf_response_handler_t which is mostly a wrapper around
   svn_ra_serf__handle_xml_parser -- it just notices OPTIONS response
   headers first, before handing off to the xml parser.
   Implements svn_ra_serf__response_handler_t */
static svn_error_t *
options_response_handler(serf_request_t *request,
                         serf_bucket_t *response,
                         void *baton,
                         apr_pool_t *pool)
{
  struct options_response_ctx_t *orc = baton;
  serf_bucket_t *hdrs = serf_bucket_response_get_headers(response);

  /* Start out assuming all capabilities are unsupported. */
  apr_hash_set(orc->session->capabilities, SVN_RA_CAPABILITY_PARTIAL_REPLAY,
               APR_HASH_KEY_STRING, capability_no);
  apr_hash_set(orc->session->capabilities, SVN_RA_CAPABILITY_DEPTH,
               APR_HASH_KEY_STRING, capability_no);
  apr_hash_set(orc->session->capabilities, SVN_RA_CAPABILITY_MERGEINFO,
               APR_HASH_KEY_STRING, capability_no);
  apr_hash_set(orc->session->capabilities, SVN_RA_CAPABILITY_LOG_REVPROPS,
               APR_HASH_KEY_STRING, capability_no);
  apr_hash_set(orc->session->capabilities, SVN_RA_CAPABILITY_ATOMIC_REVPROPS,
               APR_HASH_KEY_STRING, capability_no);

  /* Then see which ones we can discover. */
  serf_bucket_headers_do(hdrs, capabilities_headers_iterator_callback, orc);

  /* Execute the 'real' response handler to XML-parse the repsonse body. */
  return svn_ra_serf__handle_xml_parser(request, response,
                                        orc->parser_ctx, pool);
}


svn_error_t *
svn_ra_serf__create_options_req(svn_ra_serf__options_context_t **opt_ctx,
                                svn_ra_serf__session_t *session,
                                svn_ra_serf__connection_t *conn,
                                const char *path,
                                apr_pool_t *pool)
{
  svn_ra_serf__options_context_t *new_ctx;
  svn_ra_serf__handler_t *handler;
  svn_ra_serf__xml_parser_t *parser_ctx;
  struct options_response_ctx_t *options_response_ctx;

  new_ctx = apr_pcalloc(pool, sizeof(*new_ctx));

  new_ctx->pool = pool;

  new_ctx->path = path;
  new_ctx->youngest_rev = SVN_INVALID_REVNUM;

  new_ctx->session = session;
  new_ctx->conn = conn;

  handler = apr_pcalloc(pool, sizeof(*handler));

  handler->method = "OPTIONS";
  handler->path = path;
  handler->body_delegate = create_options_body;
  handler->body_type = "text/xml";
  handler->conn = conn;
  handler->session = session;

  parser_ctx = apr_pcalloc(pool, sizeof(*parser_ctx));

  parser_ctx->pool = pool;
  parser_ctx->user_data = new_ctx;
  parser_ctx->start = start_options;
  parser_ctx->end = end_options;
  parser_ctx->cdata = cdata_options;
  parser_ctx->done = &new_ctx->done;
  parser_ctx->status_code = &new_ctx->status_code;

  options_response_ctx = apr_pcalloc(pool, sizeof(*options_response_ctx));
  options_response_ctx->parser_ctx = parser_ctx;
  options_response_ctx->session = session;
  options_response_ctx->pool = pool;

  handler->response_handler = options_response_handler;
  handler->response_baton = options_response_ctx;

  svn_ra_serf__request_create(handler);

  new_ctx->parser_ctx = parser_ctx;

  *opt_ctx = new_ctx;

  return SVN_NO_ERROR;
}



/** Capabilities exchange. */

svn_error_t *
svn_ra_serf__exchange_capabilities(svn_ra_serf__session_t *serf_sess,
                                   const char **corrected_url,
                                   apr_pool_t *pool)
{
  svn_ra_serf__options_context_t *opt_ctx;
  svn_error_t *err;

  /* This routine automatically fills in serf_sess->capabilities */
  SVN_ERR(svn_ra_serf__create_options_req(&opt_ctx, serf_sess,
                                          serf_sess->conns[0],
                                          serf_sess->session_url.path, pool));

  err = svn_ra_serf__context_run_wait(
            svn_ra_serf__get_options_done_ptr(opt_ctx), serf_sess, pool);

  /* If our caller cares about server redirections, and our response
     carries such a thing, report as much.  We'll disregard ERR --
     it's most likely just a complaint about the response body not
     successfully parsing as XML or somesuch. */
  if (corrected_url && (opt_ctx->status_code == 301))
    {
      svn_error_clear(err);
      *corrected_url = opt_ctx->parser_ctx->location;
      return SVN_NO_ERROR;
    }

  SVN_ERR(svn_error_compose_create(
              svn_ra_serf__error_on_status(opt_ctx->status_code,
                                           serf_sess->session_url.path,
                                           opt_ctx->parser_ctx->location),
              err));

  /* Opportunistically cache any reported activity URL.  (We don't
     want to have to ask for this again later, potentially against an
     unreadable commit anchor URL.)  */
  if (opt_ctx->activity_collection)
    {
      serf_sess->activity_collection_url =
        apr_pstrdup(serf_sess->pool, opt_ctx->activity_collection);
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_ra_serf__has_capability(svn_ra_session_t *ra_session,
                            svn_boolean_t *has,
                            const char *capability,
                            apr_pool_t *pool)
{
  svn_ra_serf__session_t *serf_sess = ra_session->priv;
  const char *cap_result;

  /* This capability doesn't rely on anything server side. */
  if (strcmp(capability, SVN_RA_CAPABILITY_COMMIT_REVPROPS) == 0)
    {
      *has = TRUE;
      return SVN_NO_ERROR;
    }

  cap_result = apr_hash_get(serf_sess->capabilities,
                            capability,
                            APR_HASH_KEY_STRING);

  /* If any capability is unknown, they're all unknown, so ask. */
  if (cap_result == NULL)
    SVN_ERR(svn_ra_serf__exchange_capabilities(serf_sess, NULL, pool));

  /* Try again, now that we've fetched the capabilities. */
  cap_result = apr_hash_get(serf_sess->capabilities,
                            capability, APR_HASH_KEY_STRING);

  /* Some capabilities depend on the repository as well as the server.
     NOTE: svn_ra_neon__has_capability() has a very similar code block.  If
     you change something here, check there as well. */
  if (cap_result == capability_server_yes)
    {
      if (strcmp(capability, SVN_RA_CAPABILITY_MERGEINFO) == 0)
        {
          /* Handle mergeinfo specially.  Mergeinfo depends on the
             repository as well as the server, but the server routine
             that answered our svn_ra_serf__exchange_capabilities() call above
             didn't even know which repository we were interested in
             -- it just told us whether the server supports mergeinfo.
             If the answer was 'no', there's no point checking the
             particular repository; but if it was 'yes', we still must
             change it to 'no' iff the repository itself doesn't
             support mergeinfo. */
          svn_mergeinfo_catalog_t ignored;
          svn_error_t *err;
          apr_array_header_t *paths = apr_array_make(pool, 1,
                                                     sizeof(char *));
          APR_ARRAY_PUSH(paths, const char *) = "";

          err = svn_ra_serf__get_mergeinfo(ra_session, &ignored, paths, 0,
                                           FALSE, FALSE, pool);

          if (err)
            {
              if (err->apr_err == SVN_ERR_UNSUPPORTED_FEATURE)
                {
                  svn_error_clear(err);
                  cap_result = capability_no;
                }
              else if (err->apr_err == SVN_ERR_FS_NOT_FOUND)
                {
                  /* Mergeinfo requests use relative paths, and
                     anyway we're in r0, so this is a likely error,
                     but it means the repository supports mergeinfo! */
                  svn_error_clear(err);
                  cap_result = capability_yes;
                }
              else
                return err;
            }
          else
            cap_result = capability_yes;

            apr_hash_set(serf_sess->capabilities,
                         SVN_RA_CAPABILITY_MERGEINFO, APR_HASH_KEY_STRING,
                         cap_result);
        }
      else
        {
          return svn_error_createf
            (SVN_ERR_UNKNOWN_CAPABILITY, NULL,
             _("Don't know how to handle '%s' for capability '%s'"),
             capability_server_yes, capability);
        }
    }

  if (cap_result == capability_yes)
    {
      *has = TRUE;
    }
  else if (cap_result == capability_no)
    {
      *has = FALSE;
    }
  else if (cap_result == NULL)
    {
      return svn_error_createf
        (SVN_ERR_UNKNOWN_CAPABILITY, NULL,
         _("Don't know anything about capability '%s'"), capability);
    }
  else  /* "can't happen" */
    {
      /* Well, let's hope it's a string. */
      return svn_error_createf
        (SVN_ERR_RA_DAV_OPTIONS_REQ_FAILED, NULL,
         _("Attempt to fetch capability '%s' resulted in '%s'"),
         capability, cap_result);
    }

  return SVN_NO_ERROR;
}
