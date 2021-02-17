/*
 * getlocations.c :  entry point for get_locations RA functions for ra_serf
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

#include "svn_path.h"
#include "svn_pools.h"
#include "svn_ra.h"
#include "svn_xml.h"
#include "svn_private_config.h"

#include "../libsvn_ra/ra_loader.h"

#include "ra_serf.h"


/*
 * This enum represents the current state of our XML parsing for a REPORT.
 */
typedef enum loc_state_e {
  REPORT,
  LOCATION
} loc_state_e;

typedef struct loc_state_list_t {
  /* The current state that we are in now. */
  loc_state_e state;

  /* The previous state we were in. */
  struct loc_state_list_t *prev;
} loc_state_list_t;

typedef struct loc_context_t {
  /* pool to allocate memory from */
  apr_pool_t *pool;

  /* parameters set by our caller */
  const char *path;
  const apr_array_header_t *location_revisions;
  svn_revnum_t peg_revision;

  /* Returned location hash */
  apr_hash_t *paths;

  /* Current state we're in */
  loc_state_list_t *state;
  loc_state_list_t *free_state;

  int status_code;

  svn_boolean_t done;
} loc_context_t;


static void
push_state(loc_context_t *loc_ctx, loc_state_e state)
{
  loc_state_list_t *new_state;

  if (!loc_ctx->free_state)
    {
      new_state = apr_palloc(loc_ctx->pool, sizeof(*loc_ctx->state));
    }
  else
    {
      new_state = loc_ctx->free_state;
      loc_ctx->free_state = loc_ctx->free_state->prev;
    }
  new_state->state = state;

  /* Add it to the state chain. */
  new_state->prev = loc_ctx->state;
  loc_ctx->state = new_state;
}

static void pop_state(loc_context_t *loc_ctx)
{
  loc_state_list_t *free_state;
  free_state = loc_ctx->state;
  /* advance the current state */
  loc_ctx->state = loc_ctx->state->prev;
  free_state->prev = loc_ctx->free_state;
  loc_ctx->free_state = free_state;
}

static svn_error_t *
start_getloc(svn_ra_serf__xml_parser_t *parser,
             void *userData,
             svn_ra_serf__dav_props_t name,
             const char **attrs)
{
  loc_context_t *loc_ctx = userData;

  if (!loc_ctx->state && strcmp(name.name, "get-locations-report") == 0)
    {
      push_state(loc_ctx, REPORT);
    }
  else if (loc_ctx->state &&
           loc_ctx->state->state == REPORT &&
           strcmp(name.name, "location") == 0)
    {
      svn_revnum_t rev = SVN_INVALID_REVNUM;
      const char *revstr, *path;

      revstr = svn_xml_get_attr_value("rev", attrs);
      if (revstr)
        {
          rev = SVN_STR_TO_REV(revstr);
        }

      path = svn_xml_get_attr_value("path", attrs);

      if (SVN_IS_VALID_REVNUM(rev) && path)
        {
          apr_hash_set(loc_ctx->paths,
                       apr_pmemdup(loc_ctx->pool, &rev, sizeof(rev)),
                       sizeof(rev),
                       apr_pstrdup(loc_ctx->pool, path));
        }
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
end_getloc(svn_ra_serf__xml_parser_t *parser,
           void *userData,
           svn_ra_serf__dav_props_t name)
{
  loc_context_t *loc_ctx = userData;
  loc_state_list_t *cur_state;

  if (!loc_ctx->state)
    {
      return SVN_NO_ERROR;
    }

  cur_state = loc_ctx->state;

  if (cur_state->state == REPORT &&
      strcmp(name.name, "get-locations-report") == 0)
    {
      pop_state(loc_ctx);
    }
  else if (cur_state->state == LOCATION &&
           strcmp(name.name, "location") == 0)
    {
      pop_state(loc_ctx);
    }

  return SVN_NO_ERROR;
}

/* Implements svn_ra_serf__request_body_delegate_t */
static svn_error_t *
create_get_locations_body(serf_bucket_t **body_bkt,
                          void *baton,
                          serf_bucket_alloc_t *alloc,
                          apr_pool_t *pool)
{
  serf_bucket_t *buckets;
  loc_context_t *loc_ctx = baton;
  int i;

  buckets = serf_bucket_aggregate_create(alloc);

  svn_ra_serf__add_open_tag_buckets(buckets, alloc,
                                    "S:get-locations",
                                    "xmlns:S", SVN_XML_NAMESPACE,
                                    "xmlns:D", "DAV:",
                                    NULL);

  svn_ra_serf__add_tag_buckets(buckets,
                               "S:path", loc_ctx->path,
                               alloc);

  svn_ra_serf__add_tag_buckets(buckets,
                               "S:peg-revision", apr_ltoa(pool, loc_ctx->peg_revision),
                               alloc);

  for (i = 0; i < loc_ctx->location_revisions->nelts; i++)
    {
      svn_revnum_t rev = APR_ARRAY_IDX(loc_ctx->location_revisions, i, svn_revnum_t);
      svn_ra_serf__add_tag_buckets(buckets,
                                   "S:location-revision", apr_ltoa(pool, rev),
                                   alloc);
    }

  svn_ra_serf__add_close_tag_buckets(buckets, alloc,
                                     "S:get-locations");

  *body_bkt = buckets;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_ra_serf__get_locations(svn_ra_session_t *ra_session,
                           apr_hash_t **locations,
                           const char *path,
                           svn_revnum_t peg_revision,
                           const apr_array_header_t *location_revisions,
                           apr_pool_t *pool)
{
  loc_context_t *loc_ctx;
  svn_ra_serf__session_t *session = ra_session->priv;
  svn_ra_serf__handler_t *handler;
  svn_ra_serf__xml_parser_t *parser_ctx;
  const char *relative_url, *basecoll_url, *req_url;
  svn_error_t *err;

  loc_ctx = apr_pcalloc(pool, sizeof(*loc_ctx));
  loc_ctx->pool = pool;
  loc_ctx->path = path;
  loc_ctx->peg_revision = peg_revision;
  loc_ctx->location_revisions = location_revisions;
  loc_ctx->done = FALSE;
  loc_ctx->paths = apr_hash_make(loc_ctx->pool);

  *locations = loc_ctx->paths;

  SVN_ERR(svn_ra_serf__get_baseline_info(&basecoll_url, &relative_url, session,
                                         NULL, NULL, peg_revision, NULL,
                                         pool));

  req_url = svn_path_url_add_component2(basecoll_url, relative_url, pool);

  handler = apr_pcalloc(pool, sizeof(*handler));

  handler->method = "REPORT";
  handler->path = req_url;
  handler->body_delegate = create_get_locations_body;
  handler->body_delegate_baton = loc_ctx;
  handler->body_type = "text/xml";
  handler->conn = session->conns[0];
  handler->session = session;

  parser_ctx = apr_pcalloc(pool, sizeof(*parser_ctx));

  parser_ctx->pool = pool;
  parser_ctx->user_data = loc_ctx;
  parser_ctx->start = start_getloc;
  parser_ctx->end = end_getloc;
  parser_ctx->status_code = &loc_ctx->status_code;
  parser_ctx->done = &loc_ctx->done;

  handler->response_handler = svn_ra_serf__handle_xml_parser;
  handler->response_baton = parser_ctx;

  svn_ra_serf__request_create(handler);

  err = svn_ra_serf__context_run_wait(&loc_ctx->done, session, pool);

  SVN_ERR(svn_error_compose_create(
              svn_ra_serf__error_on_status(loc_ctx->status_code,
                                           req_url,
                                           parser_ctx->location),
              err));

  return SVN_NO_ERROR;
}
