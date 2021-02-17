/*
 * mergeinfo.c : entry point for mergeinfo RA functions for ra_serf
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

#include <apr_tables.h>
#include <apr_xml.h>

#include "svn_mergeinfo.h"
#include "svn_path.h"
#include "svn_ra.h"
#include "svn_string.h"
#include "svn_xml.h"

#include "private/svn_dav_protocol.h"
#include "../libsvn_ra/ra_loader.h"
#include "svn_private_config.h"
#include "ra_serf.h"




/* The current state of our XML parsing. */
typedef enum mergeinfo_state_e {
  NONE = 0,
  MERGEINFO_REPORT,
  MERGEINFO_ITEM,
  MERGEINFO_PATH,
  MERGEINFO_INFO
} mergeinfo_state_e;

/* Baton for accumulating mergeinfo.  RESULT_CATALOG stores the final
   mergeinfo catalog result we are going to hand back to the caller of
   get_mergeinfo.  curr_path and curr_info contain the value of the
   CDATA from the mergeinfo items as we get them from the server.  */

typedef struct mergeinfo_context_t {
  apr_pool_t *pool;
  svn_stringbuf_t *curr_path;
  svn_stringbuf_t *curr_info;
  svn_mergeinfo_t result_catalog;
  svn_boolean_t done;
  const apr_array_header_t *paths;
  svn_revnum_t revision;
  svn_mergeinfo_inheritance_t inherit;
  svn_boolean_t include_descendants;
} mergeinfo_context_t;

static svn_error_t *
start_element(svn_ra_serf__xml_parser_t *parser,
              void *userData,
              svn_ra_serf__dav_props_t name,
              const char **attrs)
{
  mergeinfo_context_t *mergeinfo_ctx = userData;
  mergeinfo_state_e state;

  state = parser->state->current_state;
  if (state == NONE && strcmp(name.name, SVN_DAV__MERGEINFO_REPORT) == 0)
    {
      svn_ra_serf__xml_push_state(parser, MERGEINFO_REPORT);
    }
  else if (state == MERGEINFO_REPORT &&
           strcmp(name.name, SVN_DAV__MERGEINFO_ITEM) == 0)
    {
      svn_ra_serf__xml_push_state(parser, MERGEINFO_ITEM);
      svn_stringbuf_setempty(mergeinfo_ctx->curr_path);
      svn_stringbuf_setempty(mergeinfo_ctx->curr_info);
    }
  else if (state == MERGEINFO_ITEM &&
           strcmp(name.name, SVN_DAV__MERGEINFO_PATH) == 0)
    {
      svn_ra_serf__xml_push_state(parser, MERGEINFO_PATH);
    }
  else if (state == MERGEINFO_ITEM &&
           strcmp(name.name, SVN_DAV__MERGEINFO_INFO) == 0)
    {
      svn_ra_serf__xml_push_state(parser, MERGEINFO_INFO);
    }
  return SVN_NO_ERROR;
}

static svn_error_t *
end_element(svn_ra_serf__xml_parser_t *parser, void *userData,
            svn_ra_serf__dav_props_t name)
{
  mergeinfo_context_t *mergeinfo_ctx = userData;
  mergeinfo_state_e state;

  state = parser->state->current_state;

  if (state == MERGEINFO_REPORT &&
      strcmp(name.name, SVN_DAV__MERGEINFO_REPORT) == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == MERGEINFO_ITEM
           && strcmp(name.name, SVN_DAV__MERGEINFO_ITEM) == 0)
    {
      if (mergeinfo_ctx->curr_info && mergeinfo_ctx->curr_path)
        {
          svn_mergeinfo_t path_mergeinfo;
          const char *path;

          SVN_ERR_ASSERT(mergeinfo_ctx->curr_path->data);
          path = apr_pstrdup(mergeinfo_ctx->pool,
                             mergeinfo_ctx->curr_path->data);
          SVN_ERR(svn_mergeinfo_parse(&path_mergeinfo,
                                      mergeinfo_ctx->curr_info->data,
                                      mergeinfo_ctx->pool));
          /* Correct for naughty servers that send "relative" paths
             with leading slashes! */
          apr_hash_set(mergeinfo_ctx->result_catalog,
                       path[0] == '/' ? path + 1 : path,
                       APR_HASH_KEY_STRING, path_mergeinfo);
        }
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == MERGEINFO_PATH
           && strcmp(name.name, SVN_DAV__MERGEINFO_PATH) == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == MERGEINFO_INFO
           && strcmp(name.name, SVN_DAV__MERGEINFO_INFO) == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  return SVN_NO_ERROR;
}


static svn_error_t *
cdata_handler(svn_ra_serf__xml_parser_t *parser, void *userData,
              const char *data, apr_size_t len)
{
  mergeinfo_context_t *mergeinfo_ctx = userData;
  mergeinfo_state_e state;

  state = parser->state->current_state;
  switch (state)
    {
    case MERGEINFO_PATH:
      if (mergeinfo_ctx->curr_path)
        svn_stringbuf_appendbytes(mergeinfo_ctx->curr_path, data, len);
      break;

    case MERGEINFO_INFO:
      if (mergeinfo_ctx->curr_info)
        svn_stringbuf_appendbytes(mergeinfo_ctx->curr_info, data, len);
      break;

    default:
      break;
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
create_mergeinfo_body(serf_bucket_t **bkt,
                      void *baton,
                      serf_bucket_alloc_t *alloc,
                      apr_pool_t *pool)
{
  mergeinfo_context_t *mergeinfo_ctx = baton;
  serf_bucket_t *body_bkt;

  body_bkt = serf_bucket_aggregate_create(alloc);

  svn_ra_serf__add_open_tag_buckets(body_bkt, alloc,
                                    "S:" SVN_DAV__MERGEINFO_REPORT,
                                    "xmlns:S", SVN_XML_NAMESPACE,
                                    NULL);

  svn_ra_serf__add_tag_buckets(body_bkt,
                               "S:" SVN_DAV__REVISION,
                               apr_ltoa(pool, mergeinfo_ctx->revision),
                               alloc);
  svn_ra_serf__add_tag_buckets(body_bkt, "S:" SVN_DAV__INHERIT,
                               svn_inheritance_to_word(mergeinfo_ctx->inherit),
                               alloc);
  if (mergeinfo_ctx->include_descendants)
    {
      svn_ra_serf__add_tag_buckets(body_bkt, "S:"
                                   SVN_DAV__INCLUDE_DESCENDANTS,
                                   "yes", alloc);
    }

  if (mergeinfo_ctx->paths)
    {
      int i;

      for (i = 0; i < mergeinfo_ctx->paths->nelts; i++)
        {
          const char *this_path = APR_ARRAY_IDX(mergeinfo_ctx->paths,
                                                i, const char *);

          svn_ra_serf__add_tag_buckets(body_bkt, "S:" SVN_DAV__PATH,
                                       this_path, alloc);
        }
    }

  svn_ra_serf__add_close_tag_buckets(body_bkt, alloc,
                                     "S:" SVN_DAV__MERGEINFO_REPORT);

  *bkt = body_bkt;
  return SVN_NO_ERROR;
}

/* Request a mergeinfo-report from the URL attached to SESSION,
   and fill in the MERGEINFO hash with the results.  */
svn_error_t *
svn_ra_serf__get_mergeinfo(svn_ra_session_t *ra_session,
                           svn_mergeinfo_catalog_t *catalog,
                           const apr_array_header_t *paths,
                           svn_revnum_t revision,
                           svn_mergeinfo_inheritance_t inherit,
                           svn_boolean_t include_descendants,
                           apr_pool_t *pool)
{
  svn_error_t *err, *err2;
  int status_code;

  mergeinfo_context_t *mergeinfo_ctx;
  svn_ra_serf__session_t *session = ra_session->priv;
  svn_ra_serf__handler_t *handler;
  svn_ra_serf__xml_parser_t *parser_ctx;
  const char *relative_url, *basecoll_url;
  const char *path;

  *catalog = NULL;

  SVN_ERR(svn_ra_serf__get_baseline_info(&basecoll_url, &relative_url, session,
                                         NULL, NULL, revision, NULL, pool));

  path = svn_path_url_add_component2(basecoll_url, relative_url, pool);

  mergeinfo_ctx = apr_pcalloc(pool, sizeof(*mergeinfo_ctx));
  mergeinfo_ctx->pool = pool;
  mergeinfo_ctx->curr_path = svn_stringbuf_create("", pool);
  mergeinfo_ctx->curr_info = svn_stringbuf_create("", pool);
  mergeinfo_ctx->done = FALSE;
  mergeinfo_ctx->result_catalog = apr_hash_make(pool);
  mergeinfo_ctx->paths = paths;
  mergeinfo_ctx->revision = revision;
  mergeinfo_ctx->inherit = inherit;
  mergeinfo_ctx->include_descendants = include_descendants;

  handler = apr_pcalloc(pool, sizeof(*handler));

  handler->method = "REPORT";
  handler->path = path;
  handler->conn = session->conns[0];
  handler->session = session;
  handler->body_delegate = create_mergeinfo_body;
  handler->body_delegate_baton = mergeinfo_ctx;
  handler->body_type = "text/xml";

  parser_ctx = apr_pcalloc(pool, sizeof(*parser_ctx));

  parser_ctx->pool = pool;
  parser_ctx->user_data = mergeinfo_ctx;
  parser_ctx->start = start_element;
  parser_ctx->end = end_element;
  parser_ctx->cdata = cdata_handler;
  parser_ctx->done = &mergeinfo_ctx->done;
  parser_ctx->status_code = &status_code;

  handler->response_handler = svn_ra_serf__handle_xml_parser;
  handler->response_baton = parser_ctx;

  svn_ra_serf__request_create(handler);

  err = svn_ra_serf__context_run_wait(&mergeinfo_ctx->done, session, pool);

  err2 = svn_ra_serf__error_on_status(status_code, handler->path,
                                      parser_ctx->location);
  if (err2)
    {
      svn_error_clear(err);
      return err2;
    }

  SVN_ERR(err);

  if (mergeinfo_ctx->done && apr_hash_count(mergeinfo_ctx->result_catalog))
    *catalog = mergeinfo_ctx->result_catalog;

  return SVN_NO_ERROR;
}
