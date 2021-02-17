/*
 * blame.c :  entry point for blame RA functions for ra_serf
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

#include <expat.h>

#include <serf.h>

#include "svn_pools.h"
#include "svn_ra.h"
#include "svn_dav.h"
#include "svn_xml.h"
#include "svn_config.h"
#include "svn_delta.h"
#include "svn_path.h"
#include "svn_base64.h"
#include "svn_props.h"

#include "svn_private_config.h"

#include "ra_serf.h"
#include "../libsvn_ra/ra_loader.h"


/*
 * This enum represents the current state of our XML parsing for a REPORT.
 */
typedef enum blame_state_e {
  NONE = 0,
  FILE_REVS_REPORT,
  FILE_REV,
  REV_PROP,
  SET_PROP,
  REMOVE_PROP,
  MERGED_REVISION,
  TXDELTA
} blame_state_e;

typedef struct blame_info_t {
  /* Current pool. */
  apr_pool_t *pool;

  /* our suspicious file */
  const char *path;

  /* the intended suspect */
  svn_revnum_t rev;

  /* Hashtable of revision properties */
  apr_hash_t *rev_props;

  /* Added and removed properties (svn_prop_t*'s) */
  apr_array_header_t *prop_diffs;

  /* txdelta */
  svn_txdelta_window_handler_t txdelta;
  void *txdelta_baton;

  /* returned txdelta stream */
  svn_stream_t *stream;

  /* Is this property base64-encoded? */
  svn_boolean_t prop_base64;

  /* The currently collected value as we build it up */
  const char *prop_name;
  const char *prop_attr;
  apr_size_t prop_attr_len;

  svn_string_t *prop_string;

  /* Merged revision flag */
  svn_boolean_t merged_revision;

} blame_info_t;

typedef struct blame_context_t {
  /* pool passed to get_file_revs */
  apr_pool_t *pool;

  /* parameters set by our caller */
  const char *path;
  svn_revnum_t start;
  svn_revnum_t end;
  svn_boolean_t include_merged_revisions;

  /* are we done? */
  svn_boolean_t done;

  /* blame handler and baton */
  svn_file_rev_handler_t file_rev;
  void *file_rev_baton;
} blame_context_t;


static blame_info_t *
push_state(svn_ra_serf__xml_parser_t *parser,
           blame_context_t *blame_ctx,
           blame_state_e state)
{
  svn_ra_serf__xml_push_state(parser, state);

  if (state == FILE_REV)
    {
      blame_info_t *info;

      info = apr_palloc(parser->state->pool, sizeof(*info));

      info->pool = parser->state->pool;

      info->rev = SVN_INVALID_REVNUM;
      info->path = NULL;

      info->rev_props = apr_hash_make(info->pool);
      info->prop_diffs = apr_array_make(info->pool, 0, sizeof(svn_prop_t));

      info->stream = NULL;
      info->merged_revision = FALSE;

      parser->state->private = info;
    }

  return parser->state->private;
}

static const svn_string_t *
create_propval(blame_info_t *info)
{
  const svn_string_t *s;

  if (!info->prop_attr)
    {
      return svn_string_create("", info->pool);
    }
  else
    {
      info->prop_attr = apr_pmemdup(info->pool, info->prop_attr,
                                    info->prop_attr_len + 1);
    }

  s = svn_string_ncreate(info->prop_attr, info->prop_attr_len, info->pool);
  if (info->prop_base64)
    {
      s = svn_base64_decode_string(s, info->pool);
    }
  return s;
}

static svn_error_t *
start_blame(svn_ra_serf__xml_parser_t *parser,
            void *userData,
            svn_ra_serf__dav_props_t name,
            const char **attrs)
{
  blame_context_t *blame_ctx = userData;
  blame_state_e state;

  state = parser->state->current_state;

  if (state == NONE && strcmp(name.name, "file-revs-report") == 0)
    {
      push_state(parser, blame_ctx, FILE_REVS_REPORT);
    }
  else if (state == FILE_REVS_REPORT &&
           strcmp(name.name, "file-rev") == 0)
    {
      blame_info_t *info;

      info = push_state(parser, blame_ctx, FILE_REV);

      info->path = apr_pstrdup(info->pool,
                               svn_xml_get_attr_value("path", attrs));
      info->rev = SVN_STR_TO_REV(svn_xml_get_attr_value("rev", attrs));
    }
  else if (state == FILE_REV)
    {
      blame_info_t *info;
      const char *enc;

      info = parser->state->private;

      if (strcmp(name.name, "rev-prop") == 0)
        {
          push_state(parser, blame_ctx, REV_PROP);
        }
      else if (strcmp(name.name, "set-prop") == 0)
        {
          push_state(parser, blame_ctx, SET_PROP);
        }
      if (strcmp(name.name, "remove-prop") == 0)
        {
          push_state(parser, blame_ctx, REMOVE_PROP);
        }
      else if (strcmp(name.name, "merged-revision") == 0)
        {
          push_state(parser, blame_ctx, MERGED_REVISION);
        }
      else if (strcmp(name.name, "txdelta") == 0)
        {
          SVN_ERR(blame_ctx->file_rev(blame_ctx->file_rev_baton,
                                      info->path, info->rev,
                                      info->rev_props, info->merged_revision,
                                      &info->txdelta, &info->txdelta_baton,
                                      info->prop_diffs, info->pool));

          info->stream = svn_base64_decode
              (svn_txdelta_parse_svndiff(info->txdelta, info->txdelta_baton,
                                         TRUE, info->pool), info->pool);

          push_state(parser, blame_ctx, TXDELTA);
        }

      state = parser->state->current_state;

      switch (state)
        {
        case REV_PROP:
        case SET_PROP:
        case REMOVE_PROP:
          info->prop_name = apr_pstrdup(info->pool,
                                        svn_xml_get_attr_value("name", attrs));
          info->prop_attr = NULL;
          info->prop_attr_len = 0;

          enc = svn_xml_get_attr_value("encoding", attrs);
          if (enc && strcmp(enc, "base64") == 0)
            {
              info->prop_base64 = TRUE;
            }
          else
            {
              info->prop_base64 = FALSE;
            }
          break;
        case MERGED_REVISION:
            info->merged_revision = TRUE;
          break;
        default:
          break;
        }
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
end_blame(svn_ra_serf__xml_parser_t *parser,
          void *userData,
          svn_ra_serf__dav_props_t name)
{
  blame_context_t *blame_ctx = userData;
  blame_state_e state;
  blame_info_t *info;

  state = parser->state->current_state;
  info = parser->state->private;

  if (state == NONE)
    {
      return SVN_NO_ERROR;
    }

  if (state == FILE_REVS_REPORT &&
      strcmp(name.name, "file-revs-report") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == FILE_REV &&
           strcmp(name.name, "file-rev") == 0)
    {
      /* no file changes. */
      if (!info->stream)
        {
          SVN_ERR(blame_ctx->file_rev(blame_ctx->file_rev_baton,
                                      info->path, info->rev,
                                      info->rev_props, FALSE,
                                      NULL, NULL,
                                      info->prop_diffs, info->pool));
        }
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == REV_PROP &&
           strcmp(name.name, "rev-prop") == 0)
    {
      apr_hash_set(info->rev_props,
                   info->prop_name, APR_HASH_KEY_STRING,
                   create_propval(info));

      svn_ra_serf__xml_pop_state(parser);
    }
  else if ((state == SET_PROP &&
            strcmp(name.name, "set-prop") == 0) ||
           (state == REMOVE_PROP &&
            strcmp(name.name, "remove-prop") == 0))
    {
      svn_prop_t *prop = apr_array_push(info->prop_diffs);
      prop->name = info->prop_name;
      prop->value = create_propval(info);

      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == MERGED_REVISION &&
           strcmp(name.name, "merged-revision") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == TXDELTA &&
           strcmp(name.name, "txdelta") == 0)
    {
      SVN_ERR(svn_stream_close(info->stream));

      svn_ra_serf__xml_pop_state(parser);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
cdata_blame(svn_ra_serf__xml_parser_t *parser,
            void *userData,
            const char *data,
            apr_size_t len)
{
  blame_context_t *blame_ctx = userData;
  blame_state_e state;
  blame_info_t *info;

  UNUSED_CTX(blame_ctx);

  state = parser->state->current_state;
  info = parser->state->private;

  if (state == NONE)
    {
      return SVN_NO_ERROR;
    }

  switch (state)
    {
      case REV_PROP:
      case SET_PROP:
        svn_ra_serf__expand_string(&info->prop_attr, &info->prop_attr_len,
                                   data, len, parser->state->pool);
        break;
      case TXDELTA:
        if (info->stream)
          {
            apr_size_t ret_len;

            ret_len = len;

            SVN_ERR(svn_stream_write(info->stream, data, &ret_len));
          }
        break;
      default:
        break;
    }

  return SVN_NO_ERROR;
}

/* Implements svn_ra_serf__request_body_delegate_t */
static svn_error_t *
create_file_revs_body(serf_bucket_t **body_bkt,
                      void *baton,
                      serf_bucket_alloc_t *alloc,
                      apr_pool_t *pool)
{
  serf_bucket_t *buckets;
  blame_context_t *blame_ctx = baton;

  buckets = serf_bucket_aggregate_create(alloc);

  svn_ra_serf__add_open_tag_buckets(buckets, alloc,
                                    "S:file-revs-report",
                                    "xmlns:S", SVN_XML_NAMESPACE,
                                    NULL);

  svn_ra_serf__add_tag_buckets(buckets,
                               "S:start-revision", apr_ltoa(pool, blame_ctx->start),
                               alloc);

  svn_ra_serf__add_tag_buckets(buckets,
                               "S:end-revision", apr_ltoa(pool, blame_ctx->end),
                               alloc);

  if (blame_ctx->include_merged_revisions)
    {
      svn_ra_serf__add_tag_buckets(buckets,
                                   "S:include-merged-revisions", NULL,
                                   alloc);
    }

  svn_ra_serf__add_tag_buckets(buckets,
                               "S:path", blame_ctx->path,
                               alloc);

  svn_ra_serf__add_close_tag_buckets(buckets, alloc,
                                     "S:file-revs-report");

  *body_bkt = buckets;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_ra_serf__get_file_revs(svn_ra_session_t *ra_session,
                           const char *path,
                           svn_revnum_t start,
                           svn_revnum_t end,
                           svn_boolean_t include_merged_revisions,
                           svn_file_rev_handler_t rev_handler,
                           void *rev_handler_baton,
                           apr_pool_t *pool)
{
  blame_context_t *blame_ctx;
  svn_ra_serf__session_t *session = ra_session->priv;
  svn_ra_serf__handler_t *handler;
  svn_ra_serf__xml_parser_t *parser_ctx;
  const char *relative_url, *basecoll_url, *req_url;
  int status_code;
  svn_error_t *err;

  blame_ctx = apr_pcalloc(pool, sizeof(*blame_ctx));
  blame_ctx->pool = pool;
  blame_ctx->path = path;
  blame_ctx->file_rev = rev_handler;
  blame_ctx->file_rev_baton = rev_handler_baton;
  blame_ctx->start = start;
  blame_ctx->end = end;
  blame_ctx->include_merged_revisions = include_merged_revisions;
  blame_ctx->done = FALSE;

  SVN_ERR(svn_ra_serf__get_baseline_info(&basecoll_url, &relative_url, session,
                                         NULL, session->session_url.path,
                                         end, NULL, pool));
  req_url = svn_path_url_add_component2(basecoll_url, relative_url, pool);

  handler = apr_pcalloc(pool, sizeof(*handler));

  handler->method = "REPORT";
  handler->path = req_url;
  handler->body_type = "text/xml";
  handler->body_delegate = create_file_revs_body;
  handler->body_delegate_baton = blame_ctx;
  handler->conn = session->conns[0];
  handler->session = session;

  parser_ctx = apr_pcalloc(pool, sizeof(*parser_ctx));

  parser_ctx->pool = pool;
  parser_ctx->user_data = blame_ctx;
  parser_ctx->start = start_blame;
  parser_ctx->end = end_blame;
  parser_ctx->cdata = cdata_blame;
  parser_ctx->done = &blame_ctx->done;
  parser_ctx->status_code = &status_code;

  handler->response_handler = svn_ra_serf__handle_xml_parser;
  handler->response_baton = parser_ctx;

  svn_ra_serf__request_create(handler);

  err = svn_ra_serf__context_run_wait(&blame_ctx->done, session, pool);

  err = svn_error_compose_create(
            svn_ra_serf__error_on_status(status_code,
                                         handler->path,
                                         parser_ctx->location),
            err);

  return svn_error_trace(err);
}
