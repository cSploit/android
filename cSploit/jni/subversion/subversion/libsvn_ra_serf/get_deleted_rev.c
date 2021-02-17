/*
 * get_deleted_rev.c :  ra_serf get_deleted_rev API implementation.
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


#include "svn_ra.h"
#include "svn_xml.h"
#include "svn_path.h"
#include "svn_private_config.h"

#include "../libsvn_ra/ra_loader.h"

#include "ra_serf.h"


/*
 * This enum represents the current state of our XML parsing for a REPORT.
 */
typedef enum drev_state_e {
  NONE = 0,
  VERSION_NAME
} drev_state_e;

typedef struct drev_context_t {
  apr_pool_t *pool;

  const char *path;
  svn_revnum_t peg_revision;
  svn_revnum_t end_revision;

  /* What revision was PATH@PEG_REVISION first deleted within
     the range PEG_REVISION-END-END_REVISION? */
  svn_revnum_t *revision_deleted;

  /* are we done? */
  svn_boolean_t done;

} drev_context_t;


static void
push_state(svn_ra_serf__xml_parser_t *parser,
           drev_context_t *drev_ctx,
           drev_state_e state)
{
  svn_ra_serf__xml_push_state(parser, state);

  if (state == VERSION_NAME)
    parser->state->private = NULL;
}

static svn_error_t *
start_getdrev(svn_ra_serf__xml_parser_t *parser,
              void *userData,
              svn_ra_serf__dav_props_t name,
              const char **attrs)
{
  drev_context_t *drev_ctx = userData;
  drev_state_e state;

  state = parser->state->current_state;

  if (state == NONE &&
      strcmp(name.name, SVN_DAV__VERSION_NAME) == 0)
    {
      push_state(parser, drev_ctx, VERSION_NAME);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
end_getdrev(svn_ra_serf__xml_parser_t *parser,
            void *userData,
            svn_ra_serf__dav_props_t name)
{
  drev_context_t *drev_ctx = userData;
  drev_state_e state;
  svn_string_t *info;

  state = parser->state->current_state;
  info = parser->state->private;

  if (state == VERSION_NAME &&
      strcmp(name.name, SVN_DAV__VERSION_NAME) == 0 &&
      info)
    {
      *drev_ctx->revision_deleted = SVN_STR_TO_REV(info->data);
      svn_ra_serf__xml_pop_state(parser);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
cdata_getdrev(svn_ra_serf__xml_parser_t *parser,
              void *userData,
              const char *data,
              apr_size_t len)
{
  drev_context_t *drev_ctx = userData;
  drev_state_e state;

  UNUSED_CTX(drev_ctx);

  state = parser->state->current_state;
  switch (state)
    {
    case VERSION_NAME:
        parser->state->private = svn_string_ncreate(data, len,
                                                    parser->state->pool);
        break;
    default:
        break;
    }

  return SVN_NO_ERROR;
}

/* Implements svn_ra_serf__request_body_delegate_t */
static svn_error_t *
create_getdrev_body(serf_bucket_t **body_bkt,
                    void *baton,
                    serf_bucket_alloc_t *alloc,
                    apr_pool_t *pool)
{
  serf_bucket_t *buckets;
  drev_context_t *drev_ctx = baton;

  buckets = serf_bucket_aggregate_create(alloc);

  svn_ra_serf__add_open_tag_buckets(buckets, alloc,
                                    "S:get-deleted-rev-report",
                                    "xmlns:S", SVN_XML_NAMESPACE,
                                    "xmlns:D", "DAV:",
                                    NULL, NULL);

  svn_ra_serf__add_tag_buckets(buckets,
                               "S:path", drev_ctx->path,
                               alloc);

  svn_ra_serf__add_tag_buckets(buckets,
                               "S:peg-revision",
                               apr_ltoa(pool, drev_ctx->peg_revision),
                               alloc);

  svn_ra_serf__add_tag_buckets(buckets,
                               "S:end-revision",
                               apr_ltoa(pool, drev_ctx->end_revision),
                               alloc);

  svn_ra_serf__add_close_tag_buckets(buckets, alloc,
                                     "S:get-deleted-rev-report");

  *body_bkt = buckets;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_ra_serf__get_deleted_rev(svn_ra_session_t *session,
                             const char *path,
                             svn_revnum_t peg_revision,
                             svn_revnum_t end_revision,
                             svn_revnum_t *revision_deleted,
                             apr_pool_t *pool)
{
  drev_context_t *drev_ctx;
  svn_ra_serf__session_t *ras = session->priv;
  svn_ra_serf__handler_t *handler;
  svn_ra_serf__xml_parser_t *parser_ctx;
  const char *relative_url, *basecoll_url, *req_url;
  int status_code = 0;
  svn_error_t *err;

  drev_ctx = apr_pcalloc(pool, sizeof(*drev_ctx));
  drev_ctx->path = path;
  drev_ctx->peg_revision = peg_revision;
  drev_ctx->end_revision = end_revision;
  drev_ctx->pool = pool;
  drev_ctx->revision_deleted = revision_deleted;
  drev_ctx->done = FALSE;

  SVN_ERR(svn_ra_serf__get_baseline_info(&basecoll_url, &relative_url,
                                         ras, NULL, NULL, peg_revision, NULL,
                                         pool));

  req_url = svn_path_url_add_component2(basecoll_url, relative_url, pool);

  parser_ctx = apr_pcalloc(pool, sizeof(*parser_ctx));
  parser_ctx->pool = pool;
  parser_ctx->user_data = drev_ctx;
  parser_ctx->start = start_getdrev;
  parser_ctx->end = end_getdrev;
  parser_ctx->cdata = cdata_getdrev;
  parser_ctx->done = &drev_ctx->done;
  parser_ctx->status_code = &status_code;

  handler = apr_pcalloc(pool, sizeof(*handler));
  handler->method = "REPORT";
  handler->path = req_url;
  handler->body_type = "text/xml";
  handler->response_handler = svn_ra_serf__handle_xml_parser;
  handler->body_delegate = create_getdrev_body;
  handler->body_delegate_baton = drev_ctx;
  handler->conn = ras->conns[0];
  handler->session = ras;
  handler->response_baton = parser_ctx;

  svn_ra_serf__request_create(handler);

  err = svn_ra_serf__context_run_wait(&drev_ctx->done, ras, pool);

  /* Map status 501: Method Not Implemented to our not implemented error.
     1.5.x servers and older don't support this report. */
  if (status_code == 501)
    return svn_error_createf(SVN_ERR_RA_NOT_IMPLEMENTED, err,
                             _("'%s' REPORT not implemented"), "get-deleted-rev");
  SVN_ERR(err);
  return SVN_NO_ERROR;
}
