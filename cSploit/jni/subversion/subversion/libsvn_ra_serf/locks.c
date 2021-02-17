/*
 * locks.c :  entry point for locking RA functions for ra_serf
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

#include "svn_dav.h"
#include "svn_pools.h"
#include "svn_ra.h"

#include "../libsvn_ra/ra_loader.h"
#include "svn_config.h"
#include "svn_path.h"
#include "svn_time.h"
#include "svn_private_config.h"

#include "ra_serf.h"


/*
 * This enum represents the current state of our XML parsing for a REPORT.
 */
typedef enum lock_state_e {
  NONE = 0,
  PROP,
  LOCK_DISCOVERY,
  ACTIVE_LOCK,
  LOCK_TYPE,
  LOCK_SCOPE,
  DEPTH,
  TIMEOUT,
  LOCK_TOKEN,
  COMMENT
} lock_state_e;

typedef struct lock_prop_info_t {
  const char *data;
  apr_size_t len;
} lock_prop_info_t;

typedef struct lock_info_t {
  apr_pool_t *pool;

  const char *path;

  svn_lock_t *lock;

  svn_boolean_t force;
  svn_revnum_t revision;

  svn_boolean_t read_headers;

  /* Our HTTP status code and reason. */
  int status_code;
  const char *reason;

  /* The currently collected value as we build it up */
  const char *tmp;
  apr_size_t tmp_len;

  /* are we done? */
  svn_boolean_t done;
} lock_info_t;


static lock_prop_info_t*
push_state(svn_ra_serf__xml_parser_t *parser,
           lock_info_t *lock_ctx,
           lock_state_e state)
{
  svn_ra_serf__xml_push_state(parser, state);
  switch (state)
    {
    case LOCK_TYPE:
    case LOCK_SCOPE:
    case DEPTH:
    case TIMEOUT:
    case LOCK_TOKEN:
    case COMMENT:
        parser->state->private = apr_pcalloc(parser->state->pool,
                                             sizeof(lock_prop_info_t));
        break;
      default:
        break;
    }

  return parser->state->private;
}

/*
 * Expat callback invoked on a start element tag for a PROPFIND response.
 */
static svn_error_t *
start_lock(svn_ra_serf__xml_parser_t *parser,
           void *userData,
           svn_ra_serf__dav_props_t name,
           const char **attrs)
{
  lock_info_t *ctx = userData;
  lock_state_e state;

  state = parser->state->current_state;

  if (state == NONE && strcmp(name.name, "prop") == 0)
    {
      svn_ra_serf__xml_push_state(parser, PROP);
    }
  else if (state == PROP &&
           strcmp(name.name, "lockdiscovery") == 0)
    {
      push_state(parser, ctx, LOCK_DISCOVERY);
    }
  else if (state == LOCK_DISCOVERY &&
           strcmp(name.name, "activelock") == 0)
    {
      push_state(parser, ctx, ACTIVE_LOCK);
    }
  else if (state == ACTIVE_LOCK)
    {
      if (strcmp(name.name, "locktype") == 0)
        {
          push_state(parser, ctx, LOCK_TYPE);
        }
      else if (strcmp(name.name, "lockscope") == 0)
        {
          push_state(parser, ctx, LOCK_SCOPE);
        }
      else if (strcmp(name.name, "depth") == 0)
        {
          push_state(parser, ctx, DEPTH);
        }
      else if (strcmp(name.name, "timeout") == 0)
        {
          push_state(parser, ctx, TIMEOUT);
        }
      else if (strcmp(name.name, "locktoken") == 0)
        {
          push_state(parser, ctx, LOCK_TOKEN);
        }
      else if (strcmp(name.name, "owner") == 0)
        {
          push_state(parser, ctx, COMMENT);
        }
    }
  else if (state == LOCK_TYPE)
    {
      if (strcmp(name.name, "write") == 0)
        {
          /* Do nothing. */
        }
      else
        {
          SVN_ERR_MALFUNCTION();
        }
    }
  else if (state == LOCK_SCOPE)
    {
      if (strcmp(name.name, "exclusive") == 0)
        {
          /* Do nothing. */
        }
      else
        {
          SVN_ERR_MALFUNCTION();
        }
    }

  return SVN_NO_ERROR;
}

/*
 * Expat callback invoked on an end element tag for a PROPFIND response.
 */
static svn_error_t *
end_lock(svn_ra_serf__xml_parser_t *parser,
         void *userData,
         svn_ra_serf__dav_props_t name)
{
  lock_info_t *ctx = userData;
  lock_state_e state;

  state = parser->state->current_state;

  if (state == PROP &&
      strcmp(name.name, "prop") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == LOCK_DISCOVERY &&
           strcmp(name.name, "lockdiscovery") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == ACTIVE_LOCK &&
           strcmp(name.name, "activelock") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == LOCK_TYPE &&
           strcmp(name.name, "locktype") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == LOCK_SCOPE &&
           strcmp(name.name, "lockscope") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == DEPTH &&
           strcmp(name.name, "depth") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == TIMEOUT &&
           strcmp(name.name, "timeout") == 0)
    {
      lock_prop_info_t *info = parser->state->private;

      if (strcmp(info->data, "Infinite") == 0)
        {
          ctx->lock->expiration_date = 0;
        }
      else
        {
          SVN_ERR(svn_time_from_cstring(&ctx->lock->creation_date,
                                        info->data, ctx->pool));
        }
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == LOCK_TOKEN &&
           strcmp(name.name, "locktoken") == 0)
    {
      lock_prop_info_t *info = parser->state->private;

      if (!ctx->lock->token && info->len)
        {
          apr_collapse_spaces((char*)info->data, info->data);
          ctx->lock->token = apr_pstrndup(ctx->pool, info->data, info->len);
        }
      /* We don't actually need the lock token. */
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == COMMENT &&
           strcmp(name.name, "owner") == 0)
    {
      lock_prop_info_t *info = parser->state->private;

      if (info->len)
        {
          ctx->lock->comment = apr_pstrndup(ctx->pool, info->data, info->len);
        }
      svn_ra_serf__xml_pop_state(parser);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
cdata_lock(svn_ra_serf__xml_parser_t *parser,
           void *userData,
           const char *data,
           apr_size_t len)
{
  lock_info_t *lock_ctx = userData;
  lock_state_e state;
  lock_prop_info_t *info;

  UNUSED_CTX(lock_ctx);

  state = parser->state->current_state;
  info = parser->state->private;

  switch (state)
    {
    case LOCK_TYPE:
    case LOCK_SCOPE:
    case DEPTH:
    case TIMEOUT:
    case LOCK_TOKEN:
    case COMMENT:
        svn_ra_serf__expand_string(&info->data, &info->len,
                                   data, len, parser->state->pool);
        break;
      default:
        break;
    }

  return SVN_NO_ERROR;
}

static const svn_ra_serf__dav_props_t lock_props[] =
{
  { "DAV:", "lockdiscovery" },
  { NULL }
};

static svn_error_t *
set_lock_headers(serf_bucket_t *headers,
                 void *baton,
                 apr_pool_t *pool)
{
  lock_info_t *lock_ctx = baton;

  if (lock_ctx->force)
    {
      serf_bucket_headers_set(headers, SVN_DAV_OPTIONS_HEADER,
                              SVN_DAV_OPTION_LOCK_STEAL);
    }

  if (SVN_IS_VALID_REVNUM(lock_ctx->revision))
    {
      serf_bucket_headers_set(headers, SVN_DAV_VERSION_NAME_HEADER,
                              apr_ltoa(pool, lock_ctx->revision));
    }

  return APR_SUCCESS;
}

/* Implements svn_ra_serf__response_handler_t */
static svn_error_t *
handle_lock(serf_request_t *request,
            serf_bucket_t *response,
            void *handler_baton,
            apr_pool_t *pool)
{
  svn_ra_serf__xml_parser_t *xml_ctx = handler_baton;
  lock_info_t *ctx = xml_ctx->user_data;
  svn_error_t *err;

  if (ctx->read_headers == FALSE)
    {
      serf_bucket_t *headers;
      const char *val;

      serf_status_line sl;
      apr_status_t status;

      status = serf_bucket_response_status(response, &sl);
      if (SERF_BUCKET_READ_ERROR(status))
        {
          return svn_error_wrap_apr(status, NULL);
        }

      ctx->status_code = sl.code;
      ctx->reason = sl.reason;

      /* 423 == Locked */
      if (sl.code == 423)
        {
          /* Older servers may not give a descriptive error, so we'll
             make one of our own if we can't find one in the response. */
          err = svn_ra_serf__handle_server_error(request, response, pool);
          if (!err)
            {
              err = svn_error_createf(SVN_ERR_FS_PATH_ALREADY_LOCKED,
                                      NULL,
                                      _("Lock request failed: %d %s"),
                                      ctx->status_code, ctx->reason);
            }
          return err;
        }

      headers = serf_bucket_response_get_headers(response);

      val = serf_bucket_headers_get(headers, SVN_DAV_LOCK_OWNER_HEADER);
      if (val)
        {
          ctx->lock->owner = apr_pstrdup(ctx->pool, val);
        }

      val = serf_bucket_headers_get(headers, SVN_DAV_CREATIONDATE_HEADER);
      if (val)
        {
          SVN_ERR(svn_time_from_cstring(&ctx->lock->creation_date, val,
                                        ctx->pool));
        }

      ctx->read_headers = TRUE;
    }

  /* Forbidden when a lock doesn't exist. */
  if (ctx->status_code == 403)
    {
      /* If we get an "unexpected EOF" error, we'll wrap it with
         generic request failure error. */
      err = svn_ra_serf__handle_discard_body(request, response, NULL, pool);
      if (err && APR_STATUS_IS_EOF(err->apr_err))
        {
          ctx->done = TRUE;
          err = svn_error_createf(SVN_ERR_RA_DAV_REQUEST_FAILED,
                                  err,
                                  _("Lock request failed: %d %s"),
                                  ctx->status_code, ctx->reason);
        }
      return err;
    }

  return svn_ra_serf__handle_xml_parser(request, response,
                                        handler_baton, pool);
}

/* Implements svn_ra_serf__request_body_delegate_t */
static svn_error_t *
create_getlock_body(serf_bucket_t **body_bkt,
                    void *baton,
                    serf_bucket_alloc_t *alloc,
                    apr_pool_t *pool)
{
  serf_bucket_t *buckets;

  buckets = serf_bucket_aggregate_create(alloc);

  svn_ra_serf__add_xml_header_buckets(buckets, alloc);
  svn_ra_serf__add_open_tag_buckets(buckets, alloc, "propfind",
                                    "xmlns", "DAV:",
                                    NULL);
  svn_ra_serf__add_open_tag_buckets(buckets, alloc, "prop", NULL);
  svn_ra_serf__add_tag_buckets(buckets, "lockdiscovery", NULL, alloc);
  svn_ra_serf__add_close_tag_buckets(buckets, alloc, "prop");
  svn_ra_serf__add_close_tag_buckets(buckets, alloc, "propfind");

  *body_bkt = buckets;
  return SVN_NO_ERROR;
}

static svn_error_t*
setup_getlock_headers(serf_bucket_t *headers,
                      void *baton,
                      apr_pool_t *pool)
{
  serf_bucket_headers_set(headers, "Depth", "0");

  return SVN_NO_ERROR;
}

/* Implements svn_ra_serf__request_body_delegate_t */
static svn_error_t *
create_lock_body(serf_bucket_t **body_bkt,
                 void *baton,
                 serf_bucket_alloc_t *alloc,
                 apr_pool_t *pool)
{
  lock_info_t *ctx = baton;
  serf_bucket_t *buckets;

  buckets = serf_bucket_aggregate_create(alloc);

  svn_ra_serf__add_xml_header_buckets(buckets, alloc);
  svn_ra_serf__add_open_tag_buckets(buckets, alloc, "lockinfo",
                                    "xmlns", "DAV:",
                                    NULL);

  svn_ra_serf__add_open_tag_buckets(buckets, alloc, "lockscope", NULL);
  svn_ra_serf__add_tag_buckets(buckets, "exclusive", NULL, alloc);
  svn_ra_serf__add_close_tag_buckets(buckets, alloc, "lockscope");

  svn_ra_serf__add_open_tag_buckets(buckets, alloc, "locktype", NULL);
  svn_ra_serf__add_tag_buckets(buckets, "write", NULL, alloc);
  svn_ra_serf__add_close_tag_buckets(buckets, alloc, "locktype");

  if (ctx->lock->comment)
    {
      svn_ra_serf__add_tag_buckets(buckets, "owner", ctx->lock->comment,
                                   alloc);
    }

  svn_ra_serf__add_close_tag_buckets(buckets, alloc, "lockinfo");

  *body_bkt = buckets;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_ra_serf__get_lock(svn_ra_session_t *ra_session,
                      svn_lock_t **lock,
                      const char *path,
                      apr_pool_t *pool)
{
  svn_ra_serf__session_t *session = ra_session->priv;
  svn_ra_serf__handler_t *handler;
  svn_ra_serf__xml_parser_t *parser_ctx;
  lock_info_t *lock_ctx;
  const char *req_url;
  svn_error_t *err;
  int status_code;

  req_url = svn_path_url_add_component2(session->session_url.path, path, pool);

  lock_ctx = apr_pcalloc(pool, sizeof(*lock_ctx));

  lock_ctx->pool = pool;
  lock_ctx->path = req_url;
  lock_ctx->lock = svn_lock_create(pool);
  lock_ctx->lock->path = path;

  handler = apr_pcalloc(pool, sizeof(*handler));

  handler->method = "PROPFIND";
  handler->path = req_url;
  handler->body_type = "text/xml";
  handler->conn = session->conns[0];
  handler->session = session;

  parser_ctx = apr_pcalloc(pool, sizeof(*parser_ctx));

  parser_ctx->pool = pool;
  parser_ctx->user_data = lock_ctx;
  parser_ctx->start = start_lock;
  parser_ctx->end = end_lock;
  parser_ctx->cdata = cdata_lock;
  parser_ctx->done = &lock_ctx->done;
  parser_ctx->status_code = &status_code;

  handler->body_delegate = create_getlock_body;
  handler->body_delegate_baton = lock_ctx;

  handler->header_delegate = setup_getlock_headers;
  handler->header_delegate_baton = lock_ctx;

  handler->response_handler = handle_lock;
  handler->response_baton = parser_ctx;

  svn_ra_serf__request_create(handler);
  err = svn_ra_serf__context_run_wait(&lock_ctx->done, session, pool);

  if (status_code == 404)
    {
      return svn_error_create(SVN_ERR_RA_ILLEGAL_URL, err,
                              _("Malformed URL for repository"));
    }
  if (err)
    {
      /* TODO Shh.  We're telling a white lie for now. */
      return svn_error_create(SVN_ERR_RA_NOT_IMPLEMENTED, err,
                              _("Server does not support locking features"));
    }

  *lock = lock_ctx->lock;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_ra_serf__lock(svn_ra_session_t *ra_session,
                  apr_hash_t *path_revs,
                  const char *comment,
                  svn_boolean_t force,
                  svn_ra_lock_callback_t lock_func,
                  void *lock_baton,
                  apr_pool_t *pool)
{
  svn_ra_serf__session_t *session = ra_session->priv;
  apr_hash_index_t *hi;
  apr_pool_t *subpool;

  subpool = svn_pool_create(pool);

  for (hi = apr_hash_first(pool, path_revs); hi; hi = apr_hash_next(hi))
    {
      svn_ra_serf__handler_t *handler;
      svn_ra_serf__xml_parser_t *parser_ctx;
      const char *req_url;
      lock_info_t *lock_ctx;
      const void *key;
      void *val;
      svn_error_t *err;
      svn_error_t *new_err = NULL;

      svn_pool_clear(subpool);

      lock_ctx = apr_pcalloc(subpool, sizeof(*lock_ctx));

      apr_hash_this(hi, &key, NULL, &val);
      lock_ctx->pool = subpool;
      lock_ctx->path = key;
      lock_ctx->revision = *((svn_revnum_t*)val);
      lock_ctx->lock = svn_lock_create(subpool);
      lock_ctx->lock->path = key;
      lock_ctx->lock->comment = comment;

      lock_ctx->force = force;
      req_url = svn_path_url_add_component2(session->session_url.path,
                                            lock_ctx->path, subpool);

      handler = apr_pcalloc(subpool, sizeof(*handler));

      handler->method = "LOCK";
      handler->path = req_url;
      handler->body_type = "text/xml";
      handler->conn = session->conns[0];
      handler->session = session;

      parser_ctx = apr_pcalloc(subpool, sizeof(*parser_ctx));

      parser_ctx->pool = subpool;
      parser_ctx->user_data = lock_ctx;
      parser_ctx->start = start_lock;
      parser_ctx->end = end_lock;
      parser_ctx->cdata = cdata_lock;
      parser_ctx->done = &lock_ctx->done;

      handler->header_delegate = set_lock_headers;
      handler->header_delegate_baton = lock_ctx;

      handler->body_delegate = create_lock_body;
      handler->body_delegate_baton = lock_ctx;

      handler->response_handler = handle_lock;
      handler->response_baton = parser_ctx;

      svn_ra_serf__request_create(handler);
      err = svn_ra_serf__context_run_wait(&lock_ctx->done, session, subpool);

      if (lock_func)
        new_err = lock_func(lock_baton, lock_ctx->path, TRUE, lock_ctx->lock,
                            err, subpool);
      svn_error_clear(err);

      SVN_ERR(new_err);
    }

  return SVN_NO_ERROR;
}

struct unlock_context_t {
  const char *token;
  svn_boolean_t force;
};

static svn_error_t *
set_unlock_headers(serf_bucket_t *headers,
                   void *baton,
                   apr_pool_t *pool)
{
  struct unlock_context_t *ctx = baton;

  serf_bucket_headers_set(headers, "Lock-Token", ctx->token);
  if (ctx->force)
    {
      serf_bucket_headers_set(headers, SVN_DAV_OPTIONS_HEADER,
                              SVN_DAV_OPTION_LOCK_BREAK);
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_ra_serf__unlock(svn_ra_session_t *ra_session,
                    apr_hash_t *path_tokens,
                    svn_boolean_t force,
                    svn_ra_lock_callback_t lock_func,
                    void *lock_baton,
                    apr_pool_t *pool)
{
  svn_ra_serf__session_t *session = ra_session->priv;
  apr_hash_index_t *hi;
  apr_pool_t *subpool;

  subpool = svn_pool_create(pool);

  for (hi = apr_hash_first(pool, path_tokens); hi; hi = apr_hash_next(hi))
    {
      svn_ra_serf__handler_t *handler;
      svn_ra_serf__simple_request_context_t *ctx;
      const char *req_url, *path, *token;
      const void *key;
      void *val;
      svn_lock_t *existing_lock = NULL;
      struct unlock_context_t unlock_ctx;
      svn_error_t *lock_err = NULL;

      svn_pool_clear(subpool);

      ctx = apr_pcalloc(subpool, sizeof(*ctx));
      ctx->pool = subpool;

      apr_hash_this(hi, &key, NULL, &val);
      path = key;
      token = val;

      if (force && (!token || token[0] == '\0'))
        {
          SVN_ERR(svn_ra_serf__get_lock(ra_session, &existing_lock, path,
                                        subpool));
          token = existing_lock->token;
          if (!token)
            {
              svn_error_t *err;

              err = svn_error_createf(SVN_ERR_RA_NOT_LOCKED, NULL,
                                      _("'%s' is not locked in the repository"),
                                      path);

              if (lock_func)
                {
                  svn_error_t *err2;
                  err2 = lock_func(lock_baton, path, FALSE, NULL, err, subpool);
                  svn_error_clear(err);
                  if (err2)
                    return err2;
                }
              continue;
            }
        }

      unlock_ctx.force = force;
      unlock_ctx.token = apr_pstrcat(subpool, "<", token, ">", (char *)NULL);

      req_url = svn_path_url_add_component2(session->session_url.path, path,
                                            subpool);

      handler = apr_pcalloc(subpool, sizeof(*handler));

      handler->method = "UNLOCK";
      handler->path = req_url;
      handler->conn = session->conns[0];
      handler->session = session;

      handler->header_delegate = set_unlock_headers;
      handler->header_delegate_baton = &unlock_ctx;

      handler->response_handler = svn_ra_serf__handle_status_only;
      handler->response_baton = ctx;

      svn_ra_serf__request_create(handler);
      SVN_ERR(svn_ra_serf__context_run_wait(&ctx->done, session, subpool));

      switch (ctx->status)
        {
          case 204:
            break; /* OK */
          case 403:
            /* Api users expect this specific error code to detect failures */
            lock_err = svn_error_createf(SVN_ERR_FS_LOCK_OWNER_MISMATCH, NULL,
                                         _("Unlock request failed: %d %s"),
                                         ctx->status, ctx->reason);
            break;
          default:
            lock_err = svn_error_createf(SVN_ERR_RA_DAV_REQUEST_FAILED, NULL,
                                   _("Unlock request failed: %d %s"),
                                   ctx->status, ctx->reason);
        }

      if (lock_func)
        {
          SVN_ERR(lock_func(lock_baton, path, FALSE, existing_lock,
                            lock_err, subpool));
          svn_error_clear(lock_err);
        }
    }

  return SVN_NO_ERROR;
}
