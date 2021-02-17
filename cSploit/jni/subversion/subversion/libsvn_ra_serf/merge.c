/*
 * merge.c :  MERGE response parsing functions for ra_serf
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

#include "svn_pools.h"
#include "svn_ra.h"
#include "svn_dav.h"
#include "svn_xml.h"
#include "svn_config.h"
#include "svn_dirent_uri.h"
#include "svn_props.h"

#include "private/svn_dav_protocol.h"
#include "private/svn_fspath.h"
#include "svn_private_config.h"

#include "ra_serf.h"
#include "../libsvn_ra/ra_loader.h"


/*
 * This enum represents the current state of our XML parsing for a MERGE.
 */
typedef enum merge_state_e {
  NONE = 0,
  MERGE_RESPONSE,
  UPDATED_SET,
  RESPONSE,
  HREF,
  PROPSTAT,
  PROP,
  RESOURCE_TYPE,
  AUTHOR,
  NAME,
  DATE,
  IGNORE_PROP_NAME,
  NEED_PROP_NAME,
  PROP_VAL
} merge_state_e;

typedef enum resource_type_e {
  UNSET,
  BASELINE,
  COLLECTION,
  CHECKED_IN
} resource_type_e;

typedef struct merge_info_t {
  /* Temporary allocations here please */
  apr_pool_t *pool;

  resource_type_e type;

  apr_hash_t *props;

  const char *prop_ns;
  const char *prop_name;
  const char *prop_val;
  apr_size_t prop_val_len;
} merge_info_t;

/* Structure associated with a MERGE request. */
struct svn_ra_serf__merge_context_t
{
  apr_pool_t *pool;

  svn_ra_serf__session_t *session;

  apr_hash_t *lock_tokens;
  svn_boolean_t keep_locks;

  const char *merge_resource_url; /* URL of resource to be merged. */
  const char *merge_url; /* URL at which the MERGE request is aimed. */

  int status;

  svn_boolean_t done;

  svn_commit_info_t *commit_info;
};


static merge_info_t *
push_state(svn_ra_serf__xml_parser_t *parser,
           svn_ra_serf__merge_context_t *ctx,
           merge_state_e state)
{
  merge_info_t *info;

  svn_ra_serf__xml_push_state(parser, state);

  if (state == RESPONSE)
    {
      info = apr_palloc(parser->state->pool, sizeof(*info));
      info->pool = parser->state->pool;
      info->props = apr_hash_make(info->pool);

      parser->state->private = info;
    }

  return parser->state->private;
}

static svn_error_t *
start_merge(svn_ra_serf__xml_parser_t *parser,
            void *userData,
            svn_ra_serf__dav_props_t name,
            const char **attrs)
{
  svn_ra_serf__merge_context_t *ctx = userData;
  merge_state_e state;
  merge_info_t *info;

  state = parser->state->current_state;

  if (state == NONE &&
      strcmp(name.name, "merge-response") == 0)
    {
      push_state(parser, ctx, MERGE_RESPONSE);
    }
  else if (state == NONE)
    {
      /* do nothing as we haven't seen our valid start tag yet. */
    }
  else if (state == MERGE_RESPONSE &&
           strcmp(name.name, "updated-set") == 0)
    {
      push_state(parser, ctx, UPDATED_SET);
    }
  else if (state == UPDATED_SET &&
           strcmp(name.name, "response") == 0)
    {
      push_state(parser, ctx, RESPONSE);
    }
  else if (state == RESPONSE &&
           strcmp(name.name, "href") == 0)
    {
      info = push_state(parser, ctx, PROP_VAL);

      info->prop_ns = name.namespace;
      info->prop_name = apr_pstrdup(info->pool, name.name);
      info->prop_val = NULL;
      info->prop_val_len = 0;
    }
  else if (state == RESPONSE &&
           strcmp(name.name, "propstat") == 0)
    {
      push_state(parser, ctx, PROPSTAT);
    }
  else if (state == PROPSTAT &&
           strcmp(name.name, "prop") == 0)
    {
      push_state(parser, ctx, PROP);
    }
  else if (state == PROPSTAT &&
           strcmp(name.name, "status") == 0)
    {
      /* Do nothing for now. */
    }
  else if (state == PROP &&
           strcmp(name.name, "resourcetype") == 0)
    {
      info = push_state(parser, ctx, RESOURCE_TYPE);
      info->type = UNSET;
    }
  else if (state == RESOURCE_TYPE &&
           strcmp(name.name, "baseline") == 0)
    {
      info = parser->state->private;

      info->type = BASELINE;
    }
  else if (state == RESOURCE_TYPE &&
           strcmp(name.name, "collection") == 0)
    {
      info = parser->state->private;

      info->type = COLLECTION;
    }
  else if (state == PROP &&
           strcmp(name.name, "checked-in") == 0)
    {
      info = push_state(parser, ctx, IGNORE_PROP_NAME);

      info->prop_ns = name.namespace;
      info->prop_name = apr_pstrdup(info->pool, name.name);
      info->prop_val = NULL;
      info->prop_val_len = 0;
    }
  else if (state == PROP)
    {
      push_state(parser, ctx, PROP_VAL);
    }
  else if (state == IGNORE_PROP_NAME)
    {
      push_state(parser, ctx, PROP_VAL);
    }
  else if (state == NEED_PROP_NAME)
    {
      info = push_state(parser, ctx, PROP_VAL);
      info->prop_ns = name.namespace;
      info->prop_name = apr_pstrdup(info->pool, name.name);
      info->prop_val = NULL;
      info->prop_val_len = 0;
    }
  else
    {
      SVN_ERR_MALFUNCTION();
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
end_merge(svn_ra_serf__xml_parser_t *parser,
          void *userData,
          svn_ra_serf__dav_props_t name)
{
  svn_ra_serf__merge_context_t *ctx = userData;
  merge_state_e state;
  merge_info_t *info;

  state = parser->state->current_state;
  info = parser->state->private;

  if (state == NONE)
    {
      /* nothing to close yet. */
      return SVN_NO_ERROR;
    }

  if (state == RESPONSE &&
      strcmp(name.name, "response") == 0)
    {
      if (info->type == BASELINE)
        {
          const char *str;

          str = apr_hash_get(info->props, SVN_DAV__VERSION_NAME,
                             APR_HASH_KEY_STRING);
          if (str)
            {
              ctx->commit_info->revision = SVN_STR_TO_REV(str);
            }
          else
            {
              ctx->commit_info->revision = SVN_INVALID_REVNUM;
            }

          ctx->commit_info->date =
              apr_pstrdup(ctx->pool,
                          apr_hash_get(info->props, SVN_DAV__CREATIONDATE,
                                       APR_HASH_KEY_STRING));

          ctx->commit_info->author =
              apr_pstrdup(ctx->pool,
                          apr_hash_get(info->props, "creator-displayname",
                                       APR_HASH_KEY_STRING));

          ctx->commit_info->post_commit_err =
             apr_pstrdup(ctx->pool,
                         apr_hash_get(info->props,
                                      "post-commit-err", APR_HASH_KEY_STRING));
        }
      else
        {
          const char *href;

          href = apr_hash_get(info->props, "href", APR_HASH_KEY_STRING);
          if (! svn_urlpath__is_ancestor(ctx->merge_url, href))
            {
              return svn_error_createf(SVN_ERR_RA_DAV_REQUEST_FAILED, NULL,
                                       _("A MERGE response for '%s' is not "
                                         "a child of the destination ('%s')"),
                                       href, ctx->merge_url);
            }

          /* We now need to dive all the way into the WC to update the
           * base VCC url.
           */
          if ((! SVN_RA_SERF__HAVE_HTTPV2_SUPPORT(ctx->session))
              && ctx->session->wc_callbacks->push_wc_prop)
            {
              svn_string_t checked_in_str;
              const char *checked_in;

              /* From the above check, we know that CTX->MERGE_URL is
                 an ancestor of HREF.  All that remains is to
                 determine of HREF is the same as CTX->MERGE_URL, or --
                 if not -- is relative value as a child thereof. */
              href = svn_urlpath__is_child(ctx->merge_url, href, NULL);
              if (! href)
                href = "";

              checked_in = apr_hash_get(info->props, "checked-in",
                                        APR_HASH_KEY_STRING);
              checked_in_str.data = checked_in;
              checked_in_str.len = strlen(checked_in);

              SVN_ERR(ctx->session->wc_callbacks->push_wc_prop(
                ctx->session->wc_callback_baton, href,
                SVN_RA_SERF__WC_CHECKED_IN_URL, &checked_in_str, info->pool));
            }
        }

      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == PROPSTAT &&
           strcmp(name.name, "propstat") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == PROP &&
           strcmp(name.name, "prop") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == RESOURCE_TYPE &&
           strcmp(name.name, "resourcetype") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == IGNORE_PROP_NAME || state == NEED_PROP_NAME)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == PROP_VAL)
    {
      if (!info->prop_name)
        {
          info->prop_name = apr_pstrdup(info->pool, name.name);
        }
      info->prop_val = apr_pstrmemdup(info->pool, info->prop_val,
                                      info->prop_val_len);
      if (strcmp(info->prop_name, "href") == 0)
        info->prop_val = svn_urlpath__canonicalize(info->prop_val,
                                                       info->pool);

      /* Set our property. */
      apr_hash_set(info->props, info->prop_name, APR_HASH_KEY_STRING,
                   info->prop_val);

      info->prop_ns = NULL;
      info->prop_name = NULL;
      info->prop_val = NULL;
      info->prop_val_len = 0;

      svn_ra_serf__xml_pop_state(parser);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
cdata_merge(svn_ra_serf__xml_parser_t *parser,
            void *userData,
            const char *data,
            apr_size_t len)
{
  svn_ra_serf__merge_context_t *ctx = userData;
  merge_state_e state;
  merge_info_t *info;

  UNUSED_CTX(ctx);

  state = parser->state->current_state;
  info = parser->state->private;

  if (state == PROP_VAL)
    {
      svn_ra_serf__expand_string(&info->prop_val, &info->prop_val_len,
                                 data, len, parser->state->pool);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
setup_merge_headers(serf_bucket_t *headers,
                    void *baton,
                    apr_pool_t *pool)
{
  svn_ra_serf__merge_context_t *ctx = baton;

  if (!ctx->keep_locks)
    {
      serf_bucket_headers_set(headers, SVN_DAV_OPTIONS_HEADER,
                              SVN_DAV_OPTION_RELEASE_LOCKS);
    }

  return SVN_NO_ERROR;
}

void
svn_ra_serf__merge_lock_token_list(apr_hash_t *lock_tokens,
                                   const char *parent,
                                   serf_bucket_t *body,
                                   serf_bucket_alloc_t *alloc,
                                   apr_pool_t *pool)
{
  apr_hash_index_t *hi;

  if (!lock_tokens || apr_hash_count(lock_tokens) == 0)
    return;

  svn_ra_serf__add_open_tag_buckets(body, alloc,
                                    "S:lock-token-list",
                                    "xmlns:S", SVN_XML_NAMESPACE,
                                    NULL);

  for (hi = apr_hash_first(pool, lock_tokens);
       hi;
       hi = apr_hash_next(hi))
    {
      const void *key;
      apr_ssize_t klen;
      void *val;
      svn_string_t path;

      apr_hash_this(hi, &key, &klen, &val);

      path.data = key;
      path.len = klen;

      if (parent && !svn_relpath__is_ancestor(parent, key))
        continue;

      svn_ra_serf__add_open_tag_buckets(body, alloc, "S:lock", NULL);

      svn_ra_serf__add_open_tag_buckets(body, alloc, "lock-path", NULL);
      svn_ra_serf__add_cdata_len_buckets(body, alloc, path.data, path.len);
      svn_ra_serf__add_close_tag_buckets(body, alloc, "lock-path");

      svn_ra_serf__add_tag_buckets(body, "lock-token", val, alloc);

      svn_ra_serf__add_close_tag_buckets(body, alloc, "S:lock");
    }

  svn_ra_serf__add_close_tag_buckets(body, alloc, "S:lock-token-list");
}

static svn_error_t*
create_merge_body(serf_bucket_t **bkt,
                  void *baton,
                  serf_bucket_alloc_t *alloc,
                  apr_pool_t *pool)
{
  svn_ra_serf__merge_context_t *ctx = baton;
  serf_bucket_t *body_bkt;

  body_bkt = serf_bucket_aggregate_create(alloc);

  svn_ra_serf__add_xml_header_buckets(body_bkt, alloc);
  svn_ra_serf__add_open_tag_buckets(body_bkt, alloc, "D:merge",
                                    "xmlns:D", "DAV:",
                                    NULL);
  svn_ra_serf__add_open_tag_buckets(body_bkt, alloc, "D:source", NULL);
  svn_ra_serf__add_open_tag_buckets(body_bkt, alloc, "D:href", NULL);

  svn_ra_serf__add_cdata_len_buckets(body_bkt, alloc,
                                     ctx->merge_resource_url,
                                     strlen(ctx->merge_resource_url));

  svn_ra_serf__add_close_tag_buckets(body_bkt, alloc, "D:href");
  svn_ra_serf__add_close_tag_buckets(body_bkt, alloc, "D:source");

  svn_ra_serf__add_tag_buckets(body_bkt, "D:no-auto-merge", NULL, alloc);
  svn_ra_serf__add_tag_buckets(body_bkt, "D:no-checkout", NULL, alloc);

  svn_ra_serf__add_open_tag_buckets(body_bkt, alloc, "D:prop", NULL);
  svn_ra_serf__add_tag_buckets(body_bkt, "D:checked-in", NULL, alloc);
  svn_ra_serf__add_tag_buckets(body_bkt, "D:" SVN_DAV__VERSION_NAME, NULL, alloc);
  svn_ra_serf__add_tag_buckets(body_bkt, "D:resourcetype", NULL, alloc);
  svn_ra_serf__add_tag_buckets(body_bkt, "D:" SVN_DAV__CREATIONDATE, NULL, alloc);
  svn_ra_serf__add_tag_buckets(body_bkt, "D:creator-displayname", NULL, alloc);
  svn_ra_serf__add_close_tag_buckets(body_bkt, alloc, "D:prop");

  svn_ra_serf__merge_lock_token_list(ctx->lock_tokens, NULL, body_bkt, alloc,
                                     pool);

  svn_ra_serf__add_close_tag_buckets(body_bkt, alloc, "D:merge");

  *bkt = body_bkt;

  return SVN_NO_ERROR;
}


svn_error_t *
svn_ra_serf__merge_create_req(svn_ra_serf__merge_context_t **ret_ctx,
                              svn_ra_serf__session_t *session,
                              svn_ra_serf__connection_t *conn,
                              const char *merge_resource_url,
                              apr_hash_t *lock_tokens,
                              svn_boolean_t keep_locks,
                              apr_pool_t *pool)
{
  svn_ra_serf__merge_context_t *merge_ctx;
  svn_ra_serf__handler_t *handler;
  svn_ra_serf__xml_parser_t *parser_ctx;

  merge_ctx = apr_pcalloc(pool, sizeof(*merge_ctx));

  merge_ctx->pool = pool;
  merge_ctx->session = session;

  merge_ctx->merge_resource_url = merge_resource_url;

  merge_ctx->lock_tokens = lock_tokens;
  merge_ctx->keep_locks = keep_locks;

  merge_ctx->commit_info = svn_create_commit_info(pool);

  merge_ctx->merge_url = session->session_url.path;

  handler = apr_pcalloc(pool, sizeof(*handler));

  handler->method = "MERGE";
  handler->path = merge_ctx->merge_url;
  handler->body_delegate = create_merge_body;
  handler->body_delegate_baton = merge_ctx;
  handler->conn = conn;
  handler->session = session;

  parser_ctx = apr_pcalloc(pool, sizeof(*parser_ctx));

  parser_ctx->pool = pool;
  parser_ctx->user_data = merge_ctx;
  parser_ctx->start = start_merge;
  parser_ctx->end = end_merge;
  parser_ctx->cdata = cdata_merge;
  parser_ctx->done = &merge_ctx->done;
  parser_ctx->status_code = &merge_ctx->status;

  handler->header_delegate = setup_merge_headers;
  handler->header_delegate_baton = merge_ctx;

  handler->response_handler = svn_ra_serf__handle_xml_parser;
  handler->response_baton = parser_ctx;

  svn_ra_serf__request_create(handler);

  *ret_ctx = merge_ctx;

  return SVN_NO_ERROR;
}

svn_boolean_t*
svn_ra_serf__merge_get_done_ptr(svn_ra_serf__merge_context_t *ctx)
{
  return &ctx->done;
}

svn_commit_info_t*
svn_ra_serf__merge_get_commit_info(svn_ra_serf__merge_context_t *ctx)
{
  return ctx->commit_info;
}

int
svn_ra_serf__merge_get_status(svn_ra_serf__merge_context_t *ctx)
{
  return ctx->status;
}
