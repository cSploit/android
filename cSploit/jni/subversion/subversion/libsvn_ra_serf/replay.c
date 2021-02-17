/*
 * replay.c :  entry point for replay RA functions for ra_serf
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
#include "../libsvn_ra/ra_loader.h"
#include "svn_config.h"
#include "svn_delta.h"
#include "svn_base64.h"
#include "svn_path.h"
#include "svn_private_config.h"

#include "ra_serf.h"


/*
 * This enum represents the current state of our XML parsing.
 */
typedef enum replay_state_e {
  NONE = 0,
  REPORT,
  OPEN_DIR,
  ADD_DIR,
  OPEN_FILE,
  ADD_FILE,
  DELETE_ENTRY,
  APPLY_TEXTDELTA,
  CHANGE_PROP
} replay_state_e;

typedef struct replay_info_t replay_info_t;

struct replay_info_t {
  apr_pool_t *pool;

  void *baton;
  svn_stream_t *stream;

  replay_info_t *parent;
};

typedef svn_error_t *
(*change_prop_t)(void *baton,
                 const char *name,
                 const svn_string_t *value,
                 apr_pool_t *pool);

typedef struct prop_info_t {
  apr_pool_t *pool;

  change_prop_t change;

  const char *name;
  svn_boolean_t del_prop;

  const char *data;
  apr_size_t len;

  replay_info_t *parent;
} prop_info_t;

typedef struct replay_context_t {
  apr_pool_t *src_rev_pool;
  apr_pool_t *dst_rev_pool;
  /*file_pool is cleared after completion of each file. */
  apr_pool_t *file_pool;

  /* Are we done fetching this file? */
  svn_boolean_t done;
  svn_ra_serf__list_t **done_list;
  svn_ra_serf__list_t done_item;

  /* callback to get an editor */
  svn_ra_replay_revstart_callback_t revstart_func;
  svn_ra_replay_revfinish_callback_t revfinish_func;
  void *replay_baton;

  /* replay receiver function and baton */
  const svn_delta_editor_t *editor;
  void *editor_baton;

  /* current revision */
  svn_revnum_t revision;

  /* Information needed to create the replay report body */
  svn_revnum_t low_water_mark;
  svn_boolean_t send_deltas;

  /* Cached report target url */
  const char *report_target;

  /* Target and revision to fetch revision properties on */
  const char *revprop_target;
  svn_revnum_t revprop_rev;

  /* Revision properties for this revision. */
  apr_hash_t *revs_props;
  apr_hash_t *props;

  /* Keep a reference to the XML parser ctx to report any errors. */
  svn_ra_serf__xml_parser_t *parser_ctx;

  /* The propfind for the revision properties of the current revision */
  svn_ra_serf__propfind_context_t *prop_ctx;

} replay_context_t;


static void *
push_state(svn_ra_serf__xml_parser_t *parser,
           replay_context_t *replay_ctx,
           replay_state_e state)
{
  svn_ra_serf__xml_push_state(parser, state);

  if (state == OPEN_DIR || state == ADD_DIR ||
      state == OPEN_FILE || state == ADD_FILE)
    {
      replay_info_t *info;

      info = apr_palloc(replay_ctx->dst_rev_pool, sizeof(*info));

      info->pool = replay_ctx->dst_rev_pool;
      info->parent = parser->state->private;
      info->baton = NULL;
      info->stream = NULL;

      parser->state->private = info;
    }
  else if (state == CHANGE_PROP)
    {
      prop_info_t *info;

      info = apr_pcalloc(replay_ctx->dst_rev_pool, sizeof(*info));

      info->pool = replay_ctx->dst_rev_pool;
      info->parent = parser->state->private;

      parser->state->private = info;
    }

  return parser->state->private;
}

static svn_error_t *
start_replay(svn_ra_serf__xml_parser_t *parser,
             void *userData,
             svn_ra_serf__dav_props_t name,
             const char **attrs)
{
  replay_context_t *ctx = userData;
  replay_state_e state;

  state = parser->state->current_state;

  if (state == NONE &&
      strcmp(name.name, "editor-report") == 0)
    {
      push_state(parser, ctx, REPORT);

      /* Before we can continue, we need the revision properties. */
      SVN_ERR_ASSERT(!ctx->prop_ctx
                     || svn_ra_serf__propfind_is_done(ctx->prop_ctx));

      /* Create a pool for the commit editor. */
      ctx->dst_rev_pool = svn_pool_create(ctx->src_rev_pool);
      ctx->file_pool = svn_pool_create(ctx->dst_rev_pool);

      /* ### it would be nice to have a proper scratch_pool.  */
      SVN_ERR(svn_ra_serf__select_revprops(&ctx->props,
                                           ctx->revprop_target,
                                           ctx->revprop_rev,
                                           ctx->revs_props,
                                           ctx->dst_rev_pool,
                                           ctx->dst_rev_pool));

      if (ctx->revstart_func)
        {
          SVN_ERR(ctx->revstart_func(ctx->revision, ctx->replay_baton,
                                     &ctx->editor, &ctx->editor_baton,
                                     ctx->props,
                                     ctx->dst_rev_pool));
        }
    }
  else if (state == REPORT &&
           strcmp(name.name, "target-revision") == 0)
    {
      const char *rev;

      rev = svn_xml_get_attr_value("rev", attrs);
      if (!rev)
        {
          return svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                    _("Missing revision attr in target-revision element"));
        }

      SVN_ERR(ctx->editor->set_target_revision(ctx->editor_baton,
                                               SVN_STR_TO_REV(rev),
                                               ctx->dst_rev_pool));
    }
  else if (state == REPORT &&
           strcmp(name.name, "open-root") == 0)
    {
      const char *rev;
      replay_info_t *info;

      rev = svn_xml_get_attr_value("rev", attrs);

      if (!rev)
        {
          return svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                    _("Missing revision attr in open-root element"));
        }

      info = push_state(parser, ctx, OPEN_DIR);

      SVN_ERR(ctx->editor->open_root(ctx->editor_baton,
                                     SVN_STR_TO_REV(rev),
                                     ctx->dst_rev_pool,
                                     &info->baton));
    }
  else if ((state == OPEN_DIR || state == ADD_DIR) &&
           strcmp(name.name, "delete-entry") == 0)
    {
      const char *file_name, *rev;
      replay_info_t *info;

      file_name = svn_xml_get_attr_value("name", attrs);
      if (!file_name)
        {
          return svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                    _("Missing name attr in delete-entry element"));
        }
      rev = svn_xml_get_attr_value("rev", attrs);
      if (!rev)
        {
          return svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                    _("Missing revision attr in delete-entry element"));
        }

      info = push_state(parser, ctx, DELETE_ENTRY);

      SVN_ERR(ctx->editor->delete_entry(file_name, SVN_STR_TO_REV(rev),
                                        info->baton, ctx->dst_rev_pool));

      svn_ra_serf__xml_pop_state(parser);
    }
  else if ((state == OPEN_DIR || state == ADD_DIR) &&
           strcmp(name.name, "open-directory") == 0)
    {
      const char *rev, *dir_name;
      replay_info_t *info;

      dir_name = svn_xml_get_attr_value("name", attrs);
      if (!dir_name)
        {
          return svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                    _("Missing name attr in open-directory element"));
        }
      rev = svn_xml_get_attr_value("rev", attrs);
      if (!rev)
        {
          return svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                    _("Missing revision attr in open-directory element"));
        }

      info = push_state(parser, ctx, OPEN_DIR);

      SVN_ERR(ctx->editor->open_directory(dir_name, info->parent->baton,
                                          SVN_STR_TO_REV(rev),
                                          ctx->dst_rev_pool, &info->baton));
    }
  else if ((state == OPEN_DIR || state == ADD_DIR) &&
           strcmp(name.name, "add-directory") == 0)
    {
      const char *dir_name, *copyfrom, *copyrev;
      svn_revnum_t rev;
      replay_info_t *info;

      dir_name = svn_xml_get_attr_value("name", attrs);
      if (!dir_name)
        {
          return svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                    _("Missing name attr in add-directory element"));
        }
      copyfrom = svn_xml_get_attr_value("copyfrom-path", attrs);
      copyrev = svn_xml_get_attr_value("copyfrom-rev", attrs);

      if (copyrev)
        rev = SVN_STR_TO_REV(copyrev);
      else
        rev = SVN_INVALID_REVNUM;

      info = push_state(parser, ctx, ADD_DIR);

      SVN_ERR(ctx->editor->add_directory(dir_name, info->parent->baton,
                                         copyfrom, rev,
                                         ctx->dst_rev_pool, &info->baton));
    }
  else if ((state == OPEN_DIR || state == ADD_DIR) &&
           strcmp(name.name, "close-directory") == 0)
    {
      replay_info_t *info = parser->state->private;

      SVN_ERR(ctx->editor->close_directory(info->baton, ctx->dst_rev_pool));

      svn_ra_serf__xml_pop_state(parser);
    }
  else if ((state == OPEN_DIR || state == ADD_DIR) &&
           strcmp(name.name, "open-file") == 0)
    {
      const char *file_name, *rev;
      replay_info_t *info;

      svn_pool_clear(ctx->file_pool);
      file_name = svn_xml_get_attr_value("name", attrs);
      if (!file_name)
        {
          return svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                    _("Missing name attr in open-file element"));
        }
      rev = svn_xml_get_attr_value("rev", attrs);
      if (!rev)
        {
          return svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                    _("Missing revision attr in open-file element"));
        }

      info = push_state(parser, ctx, OPEN_FILE);

      SVN_ERR(ctx->editor->open_file(file_name, info->parent->baton,
                                     SVN_STR_TO_REV(rev),
                                     ctx->file_pool, &info->baton));
    }
  else if ((state == OPEN_DIR || state == ADD_DIR) &&
           strcmp(name.name, "add-file") == 0)
    {
      const char *file_name, *copyfrom, *copyrev;
      svn_revnum_t rev;
      replay_info_t *info;

      svn_pool_clear(ctx->file_pool);
      file_name = svn_xml_get_attr_value("name", attrs);
      if (!file_name)
        {
          return svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                    _("Missing name attr in add-file element"));
        }
      copyfrom = svn_xml_get_attr_value("copyfrom-path", attrs);
      copyrev = svn_xml_get_attr_value("copyfrom-rev", attrs);

      info = push_state(parser, ctx, ADD_FILE);

      if (copyrev)
        rev = SVN_STR_TO_REV(copyrev);
      else
        rev = SVN_INVALID_REVNUM;

      SVN_ERR(ctx->editor->add_file(file_name, info->parent->baton,
                                    copyfrom, rev,
                                    ctx->file_pool, &info->baton));
    }
  else if ((state == OPEN_FILE || state == ADD_FILE) &&
           strcmp(name.name, "apply-textdelta") == 0)
    {
      const char *checksum;
      replay_info_t *info;
      svn_txdelta_window_handler_t textdelta;
      void *textdelta_baton;
      svn_stream_t *delta_stream;

      info = push_state(parser, ctx, APPLY_TEXTDELTA);

      checksum = svn_xml_get_attr_value("checksum", attrs);
      if (checksum)
        {
          checksum = apr_pstrdup(info->pool, checksum);
        }

      SVN_ERR(ctx->editor->apply_textdelta(info->baton, checksum,
                                           ctx->file_pool,
                                           &textdelta,
                                           &textdelta_baton));

      delta_stream = svn_txdelta_parse_svndiff(textdelta, textdelta_baton,
                                               TRUE, info->pool);
      info->stream = svn_base64_decode(delta_stream, info->pool);
    }
  else if ((state == OPEN_FILE || state == ADD_FILE) &&
           strcmp(name.name, "close-file") == 0)
    {
      replay_info_t *info = parser->state->private;
      const char *checksum;

      checksum = svn_xml_get_attr_value("checksum", attrs);

      SVN_ERR(ctx->editor->close_file(info->baton, checksum,
                                      ctx->file_pool));

      svn_ra_serf__xml_pop_state(parser);
    }
  else if (((state == OPEN_FILE || state == ADD_FILE) &&
            strcmp(name.name, "change-file-prop") == 0) ||
           ((state == OPEN_DIR || state == ADD_DIR) &&
            strcmp(name.name, "change-dir-prop") == 0))
    {
      const char *prop_name;
      prop_info_t *info;

      prop_name = svn_xml_get_attr_value("name", attrs);
      if (!prop_name)
        {
          return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                   _("Missing name attr in %s element"),
                                   name.name);
        }

      info = push_state(parser, ctx, CHANGE_PROP);


      if (svn_xml_get_attr_value("del", attrs))
        info->del_prop = TRUE;
      else
        info->del_prop = FALSE;

      if (state == OPEN_FILE || state == ADD_FILE)
        {
          info->name = apr_pstrdup(ctx->file_pool, prop_name);
          info->change = ctx->editor->change_file_prop;
        }
      else
        {
          info->name = apr_pstrdup(ctx->dst_rev_pool, prop_name);
          info->change = ctx->editor->change_dir_prop;
        }

    }

  return SVN_NO_ERROR;
}

static svn_error_t *
end_replay(svn_ra_serf__xml_parser_t *parser,
           void *userData,
           svn_ra_serf__dav_props_t name)
{
  replay_context_t *ctx = userData;
  replay_state_e state;

  UNUSED_CTX(ctx);

  state = parser->state->current_state;

  if (state == REPORT &&
      strcmp(name.name, "editor-report") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
      if (ctx->revfinish_func)
        {
          SVN_ERR(ctx->revfinish_func(ctx->revision, ctx->replay_baton,
                                      ctx->editor, ctx->editor_baton,
                                      ctx->props,
                                      ctx->dst_rev_pool));
        }
      svn_pool_destroy(ctx->dst_rev_pool);
    }
  else if (state == OPEN_DIR && strcmp(name.name, "open-directory") == 0)
    {
      /* Don't do anything. */
    }
  else if (state == ADD_DIR && strcmp(name.name, "add-directory") == 0)
    {
      /* Don't do anything. */
    }
  else if (state == OPEN_FILE && strcmp(name.name, "open-file") == 0)
    {
      /* Don't do anything. */
    }
  else if (state == ADD_FILE && strcmp(name.name, "add-file") == 0)
    {
      /* Don't do anything. */
    }
  else if ((state == OPEN_FILE || state == ADD_FILE) &&
           strcmp(name.name, "close-file") == 0)
    {
      /* Don't do anything. */
    }
  else if ((state == APPLY_TEXTDELTA) &&
           strcmp(name.name, "apply-textdelta") == 0)
    {
      replay_info_t *info = parser->state->private;
      SVN_ERR(svn_stream_close(info->stream));
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == CHANGE_PROP &&
           (strcmp(name.name, "change-file-prop") == 0 ||
            strcmp(name.name, "change-dir-prop") == 0))
    {
      prop_info_t *info = parser->state->private;
      const svn_string_t *prop_val;

      if (info->del_prop)
        {
          prop_val = NULL;
        }
      else
        {
          svn_string_t tmp_prop;

          tmp_prop.data = info->data;
          tmp_prop.len = info->len;

          if (strcmp(name.name, "change-file-prop") == 0)
            prop_val = svn_base64_decode_string(&tmp_prop, ctx->file_pool);
          else
            prop_val = svn_base64_decode_string(&tmp_prop, ctx->dst_rev_pool);
        }

      SVN_ERR(info->change(info->parent->baton, info->name, prop_val,
                           info->parent->pool));
      svn_ra_serf__xml_pop_state(parser);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
cdata_replay(svn_ra_serf__xml_parser_t *parser,
             void *userData,
             const char *data,
             apr_size_t len)
{
  replay_context_t *replay_ctx = userData;
  replay_state_e state;

  UNUSED_CTX(replay_ctx);

  state = parser->state->current_state;

  if (state == APPLY_TEXTDELTA)
    {
      replay_info_t *info = parser->state->private;
      apr_size_t written;

      written = len;

      SVN_ERR(svn_stream_write(info->stream, data, &written));

      if (written != len)
        return svn_error_create(SVN_ERR_STREAM_UNEXPECTED_EOF, NULL,
                                _("Error writing stream: unexpected EOF"));
    }
  else if (state == CHANGE_PROP)
    {
      prop_info_t *info = parser->state->private;

      svn_ra_serf__expand_string(&info->data, &info->len,
                                 data, len, parser->state->pool);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
create_replay_body(serf_bucket_t **bkt,
                   void *baton,
                   serf_bucket_alloc_t *alloc,
                   apr_pool_t *pool)
{
  replay_context_t *ctx = baton;
  serf_bucket_t *body_bkt;

  body_bkt = serf_bucket_aggregate_create(alloc);

  svn_ra_serf__add_open_tag_buckets(body_bkt, alloc,
                                    "S:replay-report",
                                    "xmlns:S", SVN_XML_NAMESPACE,
                                    NULL);

  svn_ra_serf__add_tag_buckets(body_bkt,
                               "S:revision",
                               apr_ltoa(ctx->src_rev_pool, ctx->revision),
                               alloc);
  svn_ra_serf__add_tag_buckets(body_bkt,
                               "S:low-water-mark",
                               apr_ltoa(ctx->src_rev_pool, ctx->low_water_mark),
                               alloc);

  svn_ra_serf__add_tag_buckets(body_bkt,
                               "S:send-deltas",
                               apr_ltoa(ctx->src_rev_pool, ctx->send_deltas),
                               alloc);

  svn_ra_serf__add_close_tag_buckets(body_bkt, alloc, "S:replay-report");

  *bkt = body_bkt;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_ra_serf__replay(svn_ra_session_t *ra_session,
                    svn_revnum_t revision,
                    svn_revnum_t low_water_mark,
                    svn_boolean_t send_deltas,
                    const svn_delta_editor_t *editor,
                    void *edit_baton,
                    apr_pool_t *pool)
{
  replay_context_t *replay_ctx;
  svn_ra_serf__session_t *session = ra_session->priv;
  svn_ra_serf__handler_t *handler;
  svn_ra_serf__xml_parser_t *parser_ctx;
  svn_error_t *err;
  const char *report_target;
  /* We're not really interested in the status code here in replay, but
     the XML parsing code will abort on error if it doesn't have a place
     to store the response status code. */
  int status_code;

  SVN_ERR(svn_ra_serf__report_resource(&report_target, session, NULL, pool));

  replay_ctx = apr_pcalloc(pool, sizeof(*replay_ctx));
  replay_ctx->src_rev_pool = pool;
  replay_ctx->editor = editor;
  replay_ctx->editor_baton = edit_baton;
  replay_ctx->done = FALSE;
  replay_ctx->revision = revision;
  replay_ctx->low_water_mark = low_water_mark;
  replay_ctx->send_deltas = send_deltas;
  replay_ctx->report_target = report_target;
  replay_ctx->revs_props = apr_hash_make(replay_ctx->src_rev_pool);

  handler = apr_pcalloc(pool, sizeof(*handler));

  handler->method = "REPORT";
  handler->path = session->session_url_str;
  handler->body_delegate = create_replay_body;
  handler->body_delegate_baton = replay_ctx;
  handler->body_type = "text/xml";
  handler->conn = session->conns[0];
  handler->session = session;

  parser_ctx = apr_pcalloc(pool, sizeof(*parser_ctx));

  parser_ctx->pool = pool;
  parser_ctx->user_data = replay_ctx;
  parser_ctx->start = start_replay;
  parser_ctx->end = end_replay;
  parser_ctx->cdata = cdata_replay;
  parser_ctx->status_code = &status_code;
  parser_ctx->done = &replay_ctx->done;

  handler->response_handler = svn_ra_serf__handle_xml_parser;
  handler->response_baton = parser_ctx;

  /* This is only needed to handle errors during XML parsing. */
  replay_ctx->parser_ctx = parser_ctx;

  svn_ra_serf__request_create(handler);

  err = svn_ra_serf__context_run_wait(&replay_ctx->done, session, pool);

  SVN_ERR(err);

  return SVN_NO_ERROR;
}

/* The maximum number of outstanding requests at any time. When this
 * number is reached, ra_serf will stop sending requests until
 * responses on the previous requests are received and handled.
 *
 * Some observations about serf which lead us to the current value.
 * ----------------------------------------------------------------
 *
 * We aim to keep serf's outgoing queue filled with enough requests so
 * the network bandwidth and server capacity is used
 * optimally. Originally we used 5 as the max. number of outstanding
 * requests, but this turned out to be too low.
 *
 * Serf doesn't exit out of the serf_context_run loop as long as it
 * has data to send or receive. With small responses (revs of a few
 * kB), serf doesn't come out of this loop at all. So with
 * MAX_OUTSTANDING_REQUESTS set to a low number, there's a big chance
 * that serf handles those requests completely in its internal loop,
 * and only then gives us a chance to create new requests. This
 * results in hiccups, slowing down the whole process.
 *
 * With a larger MAX_OUTSTANDING_REQUESTS, like 100 or more, there's
 * more chance that serf can come out of its internal loop so we can
 * replenish the outgoing request queue.  There's no real disadvantage
 * of using a large number here, besides the memory used to store the
 * message, parser and handler objects (approx. 250 bytes).
 *
 * In my test setup peak performance was reached at max. 30-35
 * requests. So I added a small margin and chose 50.
 */
#define MAX_OUTSTANDING_REQUESTS 50

svn_error_t *
svn_ra_serf__replay_range(svn_ra_session_t *ra_session,
                          svn_revnum_t start_revision,
                          svn_revnum_t end_revision,
                          svn_revnum_t low_water_mark,
                          svn_boolean_t send_deltas,
                          svn_ra_replay_revstart_callback_t revstart_func,
                          svn_ra_replay_revfinish_callback_t revfinish_func,
                          void *replay_baton,
                          apr_pool_t *pool)
{
  svn_ra_serf__session_t *session = ra_session->priv;
  svn_revnum_t rev = start_revision;
  const char *report_target;
  int active_reports = 0;

  SVN_ERR(svn_ra_serf__report_resource(&report_target, session, NULL, pool));

  while (active_reports || rev <= end_revision)
    {
      apr_status_t status;
      svn_error_t *err;
      svn_ra_serf__list_t *done_list;
      svn_ra_serf__list_t *done_reports = NULL;
      replay_context_t *replay_ctx;
      /* We're not really interested in the status code here in replay, but
         the XML parsing code will abort on error if it doesn't have a place
         to store the response status code. */
      int status_code;

      if (session->cancel_func)
        SVN_ERR(session->cancel_func(session->cancel_baton));

      /* Send pending requests, if any. Limit the number of outstanding
         requests to MAX_OUTSTANDING_REQUESTS. */
      if (rev <= end_revision  && active_reports < MAX_OUTSTANDING_REQUESTS)
        {
          svn_ra_serf__handler_t *handler;
          svn_ra_serf__xml_parser_t *parser_ctx;
          apr_pool_t *ctx_pool = svn_pool_create(pool);

          replay_ctx = apr_pcalloc(ctx_pool, sizeof(*replay_ctx));
          replay_ctx->src_rev_pool = ctx_pool;
          replay_ctx->revstart_func = revstart_func;
          replay_ctx->revfinish_func = revfinish_func;
          replay_ctx->replay_baton = replay_baton;
          replay_ctx->done = FALSE;
          replay_ctx->revision = rev;
          replay_ctx->low_water_mark = low_water_mark;
          replay_ctx->send_deltas = send_deltas;
          replay_ctx->done_item.data = replay_ctx;
          /* Request all properties of a certain revision. */
          replay_ctx->report_target = report_target;
          replay_ctx->revs_props = apr_hash_make(replay_ctx->src_rev_pool);

          if (SVN_RA_SERF__HAVE_HTTPV2_SUPPORT(session))
           {
             replay_ctx->revprop_target = apr_psprintf(pool, "%s/%ld",
                                                       session->rev_stub, rev);
             replay_ctx->revprop_rev = SVN_INVALID_REVNUM;
            }
          else
            {
              replay_ctx->revprop_target = report_target;
              replay_ctx->revprop_rev = rev;
            }

          SVN_ERR(svn_ra_serf__deliver_props(&replay_ctx->prop_ctx,
                                             replay_ctx->revs_props, session,
                                             session->conns[0],
                                             replay_ctx->revprop_target,
                                             replay_ctx->revprop_rev,
                                             "0", all_props,
                                             NULL,
                                             replay_ctx->src_rev_pool));

          /* Send the replay report request. */
          handler = apr_pcalloc(replay_ctx->src_rev_pool, sizeof(*handler));

          handler->method = "REPORT";
          handler->path = session->session_url_str;
          handler->body_delegate = create_replay_body;
          handler->body_delegate_baton = replay_ctx;
          handler->conn = session->conns[0];
          handler->session = session;

          parser_ctx = apr_pcalloc(replay_ctx->src_rev_pool,
                                   sizeof(*parser_ctx));

          /* Setup the XML parser context.
             Because we have not one but a list of requests, the 'done' property
             on the replay_ctx is not of much use. Instead, use 'done_list'.
             On each handled response (succesfully or not), the parser will add
             done_item to done_list, so by keeping track of the state of
             done_list we know how many requests have been handled completely.
          */
          parser_ctx->pool = replay_ctx->src_rev_pool;
          parser_ctx->user_data = replay_ctx;
          parser_ctx->start = start_replay;
          parser_ctx->end = end_replay;
          parser_ctx->cdata = cdata_replay;
          parser_ctx->status_code = &status_code;
          parser_ctx->done = &replay_ctx->done;
          parser_ctx->done_list = &done_reports;
          parser_ctx->done_item = &replay_ctx->done_item;
          handler->response_handler = svn_ra_serf__handle_xml_parser;
          handler->response_baton = parser_ctx;

          /* This is only needed to handle errors during XML parsing. */
          replay_ctx->parser_ctx = parser_ctx;

          svn_ra_serf__request_create(handler);

          rev++;
          active_reports++;
        }

      /* Run the serf loop, send outgoing and process incoming requests.
         This request will block when there are no more requests to send or
         responses to receive, so we have to be careful on our bookkeeping. */
      status = serf_context_run(session->context, session->timeout,
                                pool);

      err = session->pending_error;
      session->pending_error = NULL;

      if (APR_STATUS_IS_TIMEUP(status))
        {
          svn_error_clear(err);
          return svn_error_create(SVN_ERR_RA_DAV_CONN_TIMEOUT,
                                  NULL,
                                  _("Connection timed out"));
        }

      /* Substract the number of completely handled responses from our
         total nr. of open requests', so we'll know when to stop this loop.
         Since the message is completely handled, we can destroy its pool. */
      done_list = done_reports;
      while (done_list)
        {
          replay_context_t *ctx = (replay_context_t *)done_list->data;

          done_list = done_list->next;
          svn_pool_destroy(ctx->src_rev_pool);
          active_reports--;
        }

      SVN_ERR(err);
      if (status)
        {
          return svn_error_wrap_apr(status,
                                    _("Error retrieving replay REPORT (%d)"),
                                    status);
        }
      done_reports = NULL;
    }

  return SVN_NO_ERROR;
}
#undef MAX_OUTSTANDING_REQUESTS
