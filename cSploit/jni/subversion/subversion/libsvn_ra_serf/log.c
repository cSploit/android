/*
 * log.c :  entry point for log RA functions for ra_serf
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
#include "svn_base64.h"
#include "svn_xml.h"
#include "svn_config.h"
#include "svn_path.h"
#include "svn_props.h"

#include "private/svn_dav_protocol.h"
#include "svn_private_config.h"

#include "ra_serf.h"
#include "../libsvn_ra/ra_loader.h"


/*
 * This enum represents the current state of our XML parsing for a REPORT.
 */
typedef enum log_state_e {
  NONE = 0,
  REPORT,
  ITEM,
  VERSION,
  CREATOR,
  DATE,
  COMMENT,
  REVPROP,
  HAS_CHILDREN,
  ADDED_PATH,
  REPLACED_PATH,
  DELETED_PATH,
  MODIFIED_PATH,
  SUBTRACTIVE_MERGE
} log_state_e;

typedef struct log_info_t {
  apr_pool_t *pool;

  /* The currently collected value as we build it up, and its wire
   * encoding (if any).
   */
  const char *tmp;
  apr_size_t tmp_len;
  const char *tmp_encoding;

  /* Temporary change path - ultimately inserted into changed_paths hash. */
  svn_log_changed_path2_t *tmp_path;

  /* Log information */
  svn_log_entry_t *log_entry;

  /* Place to hold revprop name. */
  const char *revprop_name;
} log_info_t;

typedef struct log_context_t {
  apr_pool_t *pool;

  /* parameters set by our caller */
  const apr_array_header_t *paths;
  svn_revnum_t start;
  svn_revnum_t end;
  int limit;
  svn_boolean_t changed_paths;
  svn_boolean_t strict_node_history;
  svn_boolean_t include_merged_revisions;
  const apr_array_header_t *revprops;
  int nest_level; /* used to track mergeinfo nesting levels */
  int count; /* only incremented when nest_level == 0 */

  /* are we done? */
  svn_boolean_t done;
  int status_code;

  /* log receiver function and baton */
  svn_log_entry_receiver_t receiver;
  void *receiver_baton;

  /* pre-1.5 compatibility */
  svn_boolean_t want_author;
  svn_boolean_t want_date;
  svn_boolean_t want_message;
} log_context_t;


static log_info_t *
push_state(svn_ra_serf__xml_parser_t *parser,
           log_context_t *log_ctx,
           log_state_e state,
           const char **attrs)
{
  svn_ra_serf__xml_push_state(parser, state);

  if (state == ITEM)
    {
      log_info_t *info;
      apr_pool_t *info_pool = svn_pool_create(parser->state->pool);

      info = apr_pcalloc(info_pool, sizeof(*info));
      info->pool = info_pool;
      info->log_entry = svn_log_entry_create(info_pool);

      info->log_entry->revision = SVN_INVALID_REVNUM;

      parser->state->private = info;
    }

  if (state == ADDED_PATH || state == REPLACED_PATH ||
      state == DELETED_PATH || state == MODIFIED_PATH)
    {
      log_info_t *info = parser->state->private;

      if (!info->log_entry->changed_paths2)
        {
          info->log_entry->changed_paths2 = apr_hash_make(info->pool);
          info->log_entry->changed_paths = info->log_entry->changed_paths2;
        }

      info->tmp_path = svn_log_changed_path2_create(info->pool);
      info->tmp_path->copyfrom_rev = SVN_INVALID_REVNUM;
    }

  if (state == CREATOR || state == DATE || state == COMMENT
      || state == REVPROP)
    {
      log_info_t *info = parser->state->private;

      info->tmp_encoding = svn_xml_get_attr_value("encoding", attrs);
      if (info->tmp_encoding)
        info->tmp_encoding = apr_pstrdup(info->pool, info->tmp_encoding);

      if (!info->log_entry->revprops)
        {
          info->log_entry->revprops = apr_hash_make(info->pool);
        }
    }

  return parser->state->private;
}

/* Helper function to parse the common arguments availabe in ATTRS into CHANGE. */
static svn_error_t *
read_changed_path_attributes(svn_log_changed_path2_t *change, const char **attrs)
{
  /* All these arguments are optional. The *_from_word() functions can handle
     them for us */

  change->node_kind = svn_node_kind_from_word(
                           svn_xml_get_attr_value("node-kind", attrs));
  change->text_modified = svn_tristate__from_word(
                           svn_xml_get_attr_value("text-mods", attrs));
  change->props_modified = svn_tristate__from_word(
                           svn_xml_get_attr_value("prop-mods", attrs));

  return SVN_NO_ERROR;
}

static svn_error_t *
start_log(svn_ra_serf__xml_parser_t *parser,
          void *userData,
          svn_ra_serf__dav_props_t name,
          const char **attrs)
{
  log_context_t *log_ctx = userData;
  log_state_e state;

  state = parser->state->current_state;

  if (state == NONE &&
      strcmp(name.name, "log-report") == 0)
    {
      push_state(parser, log_ctx, REPORT, attrs);
    }
  else if (state == REPORT &&
           strcmp(name.name, "log-item") == 0)
    {
      push_state(parser, log_ctx, ITEM, attrs);
    }
  else if (state == ITEM)
    {
      log_info_t *info;

      if (strcmp(name.name, SVN_DAV__VERSION_NAME) == 0)
        {
          push_state(parser, log_ctx, VERSION, attrs);
        }
      else if (strcmp(name.name, "creator-displayname") == 0)
        {
          info = push_state(parser, log_ctx, CREATOR, attrs);
        }
      else if (strcmp(name.name, "date") == 0)
        {
          info = push_state(parser, log_ctx, DATE, attrs);
        }
      else if (strcmp(name.name, "comment") == 0)
        {
          info = push_state(parser, log_ctx, COMMENT, attrs);
        }
      else if (strcmp(name.name, "revprop") == 0)
        {
          const char *revprop_name =
            svn_xml_get_attr_value("name", attrs);
          if (revprop_name == NULL)
            return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                     _("Missing name attr in revprop element"));
          info = push_state(parser, log_ctx, REVPROP, attrs);
          info->revprop_name = apr_pstrdup(info->pool, revprop_name);
        }
      else if (strcmp(name.name, "has-children") == 0)
        {
          push_state(parser, log_ctx, HAS_CHILDREN, attrs);
        }
      else if (strcmp(name.name, "subtractive-merge") == 0)
        {
          push_state(parser, log_ctx, SUBTRACTIVE_MERGE, attrs);
        }
      else if (strcmp(name.name, "added-path") == 0)
        {
          const char *copy_path, *copy_rev_str;

          info = push_state(parser, log_ctx, ADDED_PATH, attrs);
          info->tmp_path->action = 'A';

          copy_path = svn_xml_get_attr_value("copyfrom-path", attrs);
          copy_rev_str = svn_xml_get_attr_value("copyfrom-rev", attrs);
          if (copy_path && copy_rev_str)
            {
              svn_revnum_t copy_rev;

              copy_rev = SVN_STR_TO_REV(copy_rev_str);
              if (SVN_IS_VALID_REVNUM(copy_rev))
                {
                  info->tmp_path->copyfrom_path = apr_pstrdup(info->pool,
                                                              copy_path);
                  info->tmp_path->copyfrom_rev = copy_rev;
                }
            }

          SVN_ERR(read_changed_path_attributes(info->tmp_path, attrs));
        }
      else if (strcmp(name.name, "replaced-path") == 0)
        {
          const char *copy_path, *copy_rev_str;

          info = push_state(parser, log_ctx, REPLACED_PATH, attrs);
          info->tmp_path->action = 'R';

          copy_path = svn_xml_get_attr_value("copyfrom-path", attrs);
          copy_rev_str = svn_xml_get_attr_value("copyfrom-rev", attrs);
          if (copy_path && copy_rev_str)
            {
              svn_revnum_t copy_rev;

              copy_rev = SVN_STR_TO_REV(copy_rev_str);
              if (SVN_IS_VALID_REVNUM(copy_rev))
                {
                  info->tmp_path->copyfrom_path = apr_pstrdup(info->pool,
                                                              copy_path);
                  info->tmp_path->copyfrom_rev = copy_rev;
                }
            }

          SVN_ERR(read_changed_path_attributes(info->tmp_path, attrs));
        }
      else if (strcmp(name.name, "deleted-path") == 0)
        {
          info = push_state(parser, log_ctx, DELETED_PATH, attrs);
          info->tmp_path->action = 'D';

          SVN_ERR(read_changed_path_attributes(info->tmp_path, attrs));
        }
      else if (strcmp(name.name, "modified-path") == 0)
        {
          info = push_state(parser, log_ctx, MODIFIED_PATH, attrs);
          info->tmp_path->action = 'M';

          SVN_ERR(read_changed_path_attributes(info->tmp_path, attrs));
        }
    }

  return SVN_NO_ERROR;
}

/*
 * Set *DECODED_CDATA to a copy of current CDATA being tracked in INFO,
 * decoded as necessary, and allocated from INFO->pool..
 */
static svn_error_t *
maybe_decode_log_cdata(const svn_string_t **decoded_cdata,
                       log_info_t *info)
{
  if (info->tmp_encoding)
    {
      svn_string_t in;
      in.data = info->tmp;
      in.len = info->tmp_len;

      /* Check for a known encoding type.  This is easy -- there's
         only one.  */
      if (strcmp(info->tmp_encoding, "base64") != 0)
        {
          return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                   _("Unsupported encoding '%s'"),
                                   info->tmp_encoding);
        }

      *decoded_cdata = svn_base64_decode_string(&in, info->pool);
    }
  else
    {
      *decoded_cdata = svn_string_ncreate(info->tmp, info->tmp_len,
                                          info->pool);
    }
  return SVN_NO_ERROR;
}

static svn_error_t *
end_log(svn_ra_serf__xml_parser_t *parser,
        void *userData,
        svn_ra_serf__dav_props_t name)
{
  log_context_t *log_ctx = userData;
  log_state_e state;
  log_info_t *info;

  state = parser->state->current_state;
  info = parser->state->private;

  if (state == REPORT &&
      strcmp(name.name, "log-report") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == ITEM &&
           strcmp(name.name, "log-item") == 0)
    {
      if (log_ctx->limit && (log_ctx->nest_level == 0)
          && (++log_ctx->count > log_ctx->limit))
        {
          return SVN_NO_ERROR;
        }

      /* Give the info to the reporter */
      SVN_ERR(log_ctx->receiver(log_ctx->receiver_baton,
                                info->log_entry,
                                info->pool));

      if (info->log_entry->has_children)
        {
          log_ctx->nest_level++;
        }
      if (! SVN_IS_VALID_REVNUM(info->log_entry->revision))
        {
          SVN_ERR_ASSERT(log_ctx->nest_level);
          log_ctx->nest_level--;
        }

      svn_pool_destroy(info->pool);
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == VERSION &&
           strcmp(name.name, SVN_DAV__VERSION_NAME) == 0)
    {
      info->log_entry->revision = SVN_STR_TO_REV(info->tmp);
      info->tmp_len = 0;
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == CREATOR &&
           strcmp(name.name, "creator-displayname") == 0)
    {
      if (log_ctx->want_author)
        {
          const svn_string_t *decoded_cdata;
          SVN_ERR(maybe_decode_log_cdata(&decoded_cdata, info));
          apr_hash_set(info->log_entry->revprops, SVN_PROP_REVISION_AUTHOR,
                       APR_HASH_KEY_STRING, decoded_cdata);
        }
      info->tmp_len = 0;
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == DATE &&
           strcmp(name.name, "date") == 0)
    {
      if (log_ctx->want_date)
        {
          const svn_string_t *decoded_cdata;
          SVN_ERR(maybe_decode_log_cdata(&decoded_cdata, info));
          apr_hash_set(info->log_entry->revprops, SVN_PROP_REVISION_DATE,
                       APR_HASH_KEY_STRING, decoded_cdata);
        }
      info->tmp_len = 0;
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == COMMENT &&
           strcmp(name.name, "comment") == 0)
    {
      if (log_ctx->want_message)
        {
          const svn_string_t *decoded_cdata;
          SVN_ERR(maybe_decode_log_cdata(&decoded_cdata, info));
          apr_hash_set(info->log_entry->revprops, SVN_PROP_REVISION_LOG,
                       APR_HASH_KEY_STRING, decoded_cdata);
        }
      info->tmp_len = 0;
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == REVPROP)
    {
      const svn_string_t *decoded_cdata;
      SVN_ERR(maybe_decode_log_cdata(&decoded_cdata, info));
      apr_hash_set(info->log_entry->revprops, info->revprop_name,
                   APR_HASH_KEY_STRING, decoded_cdata);
      info->tmp_len = 0;
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == HAS_CHILDREN &&
           strcmp(name.name, "has-children") == 0)
    {
      info->log_entry->has_children = TRUE;
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == SUBTRACTIVE_MERGE &&
           strcmp(name.name, "subtractive-merge") == 0)
    {
      info->log_entry->subtractive_merge = TRUE;
      svn_ra_serf__xml_pop_state(parser);
    }
  else if ((state == ADDED_PATH &&
            strcmp(name.name, "added-path") == 0) ||
           (state == DELETED_PATH &&
            strcmp(name.name, "deleted-path") == 0) ||
           (state == MODIFIED_PATH &&
            strcmp(name.name, "modified-path") == 0) ||
           (state == REPLACED_PATH &&
            strcmp(name.name, "replaced-path") == 0))
    {
      char *path;

      path = apr_pstrmemdup(info->pool, info->tmp, info->tmp_len);
      info->tmp_len = 0;

      apr_hash_set(info->log_entry->changed_paths2, path, APR_HASH_KEY_STRING,
                   info->tmp_path);
      svn_ra_serf__xml_pop_state(parser);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
cdata_log(svn_ra_serf__xml_parser_t *parser,
          void *userData,
          const char *data,
          apr_size_t len)
{
  log_context_t *log_ctx = userData;
  log_state_e state;
  log_info_t *info;

  UNUSED_CTX(log_ctx);

  state = parser->state->current_state;
  info = parser->state->private;

  switch (state)
    {
      case VERSION:
      case CREATOR:
      case DATE:
      case COMMENT:
      case REVPROP:
      case ADDED_PATH:
      case REPLACED_PATH:
      case DELETED_PATH:
      case MODIFIED_PATH:
        svn_ra_serf__expand_string(&info->tmp, &info->tmp_len,
                                   data, len, info->pool);
        break;
      default:
        break;
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
create_log_body(serf_bucket_t **body_bkt,
                void *baton,
                serf_bucket_alloc_t *alloc,
                apr_pool_t *pool)
{
  serf_bucket_t *buckets;
  log_context_t *log_ctx = baton;

  buckets = serf_bucket_aggregate_create(alloc);

  svn_ra_serf__add_open_tag_buckets(buckets, alloc,
                                    "S:log-report",
                                    "xmlns:S", SVN_XML_NAMESPACE,
                                    NULL);

  svn_ra_serf__add_tag_buckets(buckets,
                               "S:start-revision",
                               apr_ltoa(pool, log_ctx->start),
                               alloc);
  svn_ra_serf__add_tag_buckets(buckets,
                               "S:end-revision",
                               apr_ltoa(pool, log_ctx->end),
                               alloc);

  if (log_ctx->limit)
    {
      svn_ra_serf__add_tag_buckets(buckets,
                                   "S:limit", apr_ltoa(pool, log_ctx->limit),
                                   alloc);
    }

  if (log_ctx->changed_paths)
    {
      svn_ra_serf__add_tag_buckets(buckets,
                                   "S:discover-changed-paths", NULL,
                                   alloc);
    }

  if (log_ctx->strict_node_history)
    {
      svn_ra_serf__add_tag_buckets(buckets,
                                   "S:strict-node-history", NULL,
                                   alloc);
    }

  if (log_ctx->include_merged_revisions)
    {
      svn_ra_serf__add_tag_buckets(buckets,
                                   "S:include-merged-revisions", NULL,
                                   alloc);
    }

  if (log_ctx->revprops)
    {
      int i;
      for (i = 0; i < log_ctx->revprops->nelts; i++)
        {
          char *name = APR_ARRAY_IDX(log_ctx->revprops, i, char *);
          svn_ra_serf__add_tag_buckets(buckets,
                                       "S:revprop", name,
                                       alloc);
        }
      if (log_ctx->revprops->nelts == 0)
        {
          svn_ra_serf__add_tag_buckets(buckets,
                                       "S:no-revprops", NULL,
                                       alloc);
        }
    }
  else
    {
      svn_ra_serf__add_tag_buckets(buckets,
                                   "S:all-revprops", NULL,
                                   alloc);
    }

  if (log_ctx->paths)
    {
      int i;
      for (i = 0; i < log_ctx->paths->nelts; i++)
        {
          svn_ra_serf__add_tag_buckets(buckets,
                                       "S:path", APR_ARRAY_IDX(log_ctx->paths, i,
                                                               const char*),
                                       alloc);
        }
    }

  svn_ra_serf__add_tag_buckets(buckets,
                               "S:encode-binary-props", NULL,
                               alloc);

  svn_ra_serf__add_close_tag_buckets(buckets, alloc,
                                     "S:log-report");

  *body_bkt = buckets;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_ra_serf__get_log(svn_ra_session_t *ra_session,
                     const apr_array_header_t *paths,
                     svn_revnum_t start,
                     svn_revnum_t end,
                     int limit,
                     svn_boolean_t discover_changed_paths,
                     svn_boolean_t strict_node_history,
                     svn_boolean_t include_merged_revisions,
                     const apr_array_header_t *revprops,
                     svn_log_entry_receiver_t receiver,
                     void *receiver_baton,
                     apr_pool_t *pool)
{
  log_context_t *log_ctx;
  svn_ra_serf__session_t *session = ra_session->priv;
  svn_ra_serf__handler_t *handler;
  svn_ra_serf__xml_parser_t *parser_ctx;
  svn_boolean_t want_custom_revprops;
  svn_revnum_t peg_rev;
  svn_error_t *err;
  const char *relative_url, *basecoll_url, *req_url;

  log_ctx = apr_pcalloc(pool, sizeof(*log_ctx));
  log_ctx->pool = pool;
  log_ctx->receiver = receiver;
  log_ctx->receiver_baton = receiver_baton;
  log_ctx->paths = paths;
  log_ctx->start = start;
  log_ctx->end = end;
  log_ctx->limit = limit;
  log_ctx->changed_paths = discover_changed_paths;
  log_ctx->strict_node_history = strict_node_history;
  log_ctx->include_merged_revisions = include_merged_revisions;
  log_ctx->revprops = revprops;
  log_ctx->nest_level = 0;
  log_ctx->done = FALSE;

  want_custom_revprops = FALSE;
  if (revprops)
    {
      int i;
      for (i = 0; i < revprops->nelts; i++)
        {
          char *name = APR_ARRAY_IDX(revprops, i, char *);
          if (strcmp(name, SVN_PROP_REVISION_AUTHOR) == 0)
            log_ctx->want_author = TRUE;
          else if (strcmp(name, SVN_PROP_REVISION_DATE) == 0)
            log_ctx->want_date = TRUE;
          else if (strcmp(name, SVN_PROP_REVISION_LOG) == 0)
            log_ctx->want_message = TRUE;
          else
            want_custom_revprops = TRUE;
        }
    }
  else
    {
      log_ctx->want_author = log_ctx->want_date = log_ctx->want_message = TRUE;
      want_custom_revprops = TRUE;
    }

  if (want_custom_revprops)
    {
      svn_boolean_t has_log_revprops;
      SVN_ERR(svn_ra_serf__has_capability(ra_session, &has_log_revprops,
                                          SVN_RA_CAPABILITY_LOG_REVPROPS, pool));
      if (!has_log_revprops)
        return svn_error_create(SVN_ERR_RA_NOT_IMPLEMENTED, NULL,
                                _("Server does not support custom revprops"
                                  " via log"));
    }
  /* At this point, we may have a deleted file.  So, we'll match ra_neon's
   * behavior and use the larger of start or end as our 'peg' rev.
   */
  peg_rev = (start > end) ? start : end;

  SVN_ERR(svn_ra_serf__get_baseline_info(&basecoll_url, &relative_url, session,
                                         NULL, NULL, peg_rev, NULL, pool));

  req_url = svn_path_url_add_component2(basecoll_url, relative_url, pool);

  handler = apr_pcalloc(pool, sizeof(*handler));

  handler->method = "REPORT";
  handler->path = req_url;
  handler->body_delegate = create_log_body;
  handler->body_delegate_baton = log_ctx;
  handler->body_type = "text/xml";
  handler->conn = session->conns[0];
  handler->session = session;

  parser_ctx = apr_pcalloc(pool, sizeof(*parser_ctx));

  parser_ctx->pool = pool;
  parser_ctx->user_data = log_ctx;
  parser_ctx->start = start_log;
  parser_ctx->end = end_log;
  parser_ctx->cdata = cdata_log;
  parser_ctx->done = &log_ctx->done;
  parser_ctx->status_code = &log_ctx->status_code;

  handler->response_handler = svn_ra_serf__handle_xml_parser;
  handler->response_baton = parser_ctx;

  svn_ra_serf__request_create(handler);

  err = svn_ra_serf__context_run_wait(&log_ctx->done, session, pool);

  SVN_ERR(svn_error_compose_create(
              svn_ra_serf__error_on_status(log_ctx->status_code,
                                           req_url,
                                           parser_ctx->location),
              err));

  return SVN_NO_ERROR;
}
