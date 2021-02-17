/*
 * property.c : property routines for ra_serf
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



#include <serf.h>

#include "svn_path.h"
#include "svn_base64.h"
#include "svn_xml.h"
#include "svn_props.h"
#include "svn_dirent_uri.h"

#include "private/svn_dav_protocol.h"
#include "private/svn_fspath.h"
#include "svn_private_config.h"

#include "ra_serf.h"


/* Our current parsing state we're in for the PROPFIND response. */
typedef enum prop_state_e {
  NONE = 0,
  RESPONSE,
  PROP,
  PROPVAL
} prop_state_e;

typedef struct prop_info_t {
  apr_pool_t *pool;

  /* Current ns, attribute name, and value of the property we're parsing */
  const char *ns;

  const char *name;

  const char *val;
  apr_size_t val_len;

  const char *encoding;

} prop_info_t;

/*
 * This structure represents a pending PROPFIND response.
 */
struct svn_ra_serf__propfind_context_t {
  /* pool to issue allocations from */
  apr_pool_t *pool;

  svn_ra_serf__handler_t *handler;

  /* associated serf session */
  svn_ra_serf__session_t *sess;
  svn_ra_serf__connection_t *conn;

  /* the requested path */
  const char *path;

  /* the requested version (number and string form) */
  svn_revnum_t rev;
  const char *label;

  /* the request depth */
  const char *depth;

  /* the list of requested properties */
  const svn_ra_serf__dav_props_t *find_props;

  /* hash table that will be updated with the properties
   *
   * This can be shared between multiple svn_ra_serf__propfind_context_t
   * structures
   */
  apr_hash_t *ret_props;

  /* If we're dealing with a Depth: 1 response,
   * we may be dealing with multiple paths.
   */
  const char *current_path;

  /* Returned status code. */
  int status_code;

  /* Are we done issuing the PROPFIND? */
  svn_boolean_t done;

  /* Context from XML stream */
  svn_ra_serf__xml_parser_t *parser_ctx;

  /* If not-NULL, add us to this list when we're done. */
  svn_ra_serf__list_t **done_list;

  svn_ra_serf__list_t done_item;
};

const svn_string_t *
svn_ra_serf__get_ver_prop_string(apr_hash_t *props,
                                 const char *path,
                                 svn_revnum_t rev,
                                 const char *ns,
                                 const char *name)
{
  apr_hash_t *ver_props, *path_props, *ns_props;
  void *val = NULL;

  ver_props = apr_hash_get(props, &rev, sizeof(rev));
  if (ver_props)
    {
      path_props = apr_hash_get(ver_props, path, APR_HASH_KEY_STRING);

      if (path_props)
        {
          ns_props = apr_hash_get(path_props, ns, APR_HASH_KEY_STRING);
          if (ns_props)
            {
              val = apr_hash_get(ns_props, name, APR_HASH_KEY_STRING);
            }
        }
    }

  return val;
}

const char *
svn_ra_serf__get_ver_prop(apr_hash_t *props,
                          const char *path,
                          svn_revnum_t rev,
                          const char *ns,
                          const char *name)
{
  const svn_string_t *val;

  val = svn_ra_serf__get_ver_prop_string(props, path, rev, ns, name);

  if (val)
    {
      return val->data;
    }

  return NULL;
}

const svn_string_t *
svn_ra_serf__get_prop_string(apr_hash_t *props,
                             const char *path,
                             const char *ns,
                             const char *name)
{
  return svn_ra_serf__get_ver_prop_string(props, path, SVN_INVALID_REVNUM,
                                          ns, name);
}

const char *
svn_ra_serf__get_prop(apr_hash_t *props,
                      const char *path,
                      const char *ns,
                      const char *name)
{
  return svn_ra_serf__get_ver_prop(props, path, SVN_INVALID_REVNUM, ns, name);
}

void
svn_ra_serf__set_ver_prop(apr_hash_t *props,
                          const char *path, svn_revnum_t rev,
                          const char *ns, const char *name,
                          const svn_string_t *val, apr_pool_t *pool)
{
  apr_hash_t *ver_props, *path_props, *ns_props;

  ver_props = apr_hash_get(props, &rev, sizeof(rev));
  if (!ver_props)
    {
      ver_props = apr_hash_make(pool);
      apr_hash_set(props, apr_pmemdup(pool, &rev, sizeof(rev)), sizeof(rev),
                   ver_props);
    }

  path_props = apr_hash_get(ver_props, path, APR_HASH_KEY_STRING);

  if (!path_props)
    {
      path_props = apr_hash_make(pool);
      path = apr_pstrdup(pool, path);
      apr_hash_set(ver_props, path, APR_HASH_KEY_STRING, path_props);

      /* todo: we know that we'll fail the next check, but fall through
       * for now for simplicity's sake.
       */
    }

  ns_props = apr_hash_get(path_props, ns, APR_HASH_KEY_STRING);
  if (!ns_props)
    {
      ns_props = apr_hash_make(pool);
      ns = apr_pstrdup(pool, ns);
      apr_hash_set(path_props, ns, APR_HASH_KEY_STRING, ns_props);
    }

  apr_hash_set(ns_props, name, APR_HASH_KEY_STRING, val);
}

void
svn_ra_serf__set_prop(apr_hash_t *props,
                      const char *path,
                      const char *ns, const char *name,
                      const svn_string_t *val, apr_pool_t *pool)
{
  svn_ra_serf__set_ver_prop(props, path, SVN_INVALID_REVNUM, ns, name,
                            val, pool);
}

static prop_info_t *
push_state(svn_ra_serf__xml_parser_t *parser,
           svn_ra_serf__propfind_context_t *propfind,
           prop_state_e state)
{
  svn_ra_serf__xml_push_state(parser, state);

  if (state == PROPVAL)
    {
      prop_info_t *info;

      info = apr_pcalloc(parser->state->pool, sizeof(*info));
      info->pool = parser->state->pool;

      parser->state->private = info;
    }

  return parser->state->private;
}

/*
 * Expat callback invoked on a start element tag for a PROPFIND response.
 */
static svn_error_t *
start_propfind(svn_ra_serf__xml_parser_t *parser,
               void *userData,
               svn_ra_serf__dav_props_t name,
               const char **attrs)
{
  svn_ra_serf__propfind_context_t *ctx = userData;
  prop_state_e state;
  prop_info_t *info;

  state = parser->state->current_state;

  if (state == NONE && strcmp(name.name, "response") == 0)
    {
      svn_ra_serf__xml_push_state(parser, RESPONSE);
    }
  else if (state == RESPONSE && strcmp(name.name, "href") == 0)
    {
      info = push_state(parser, ctx, PROPVAL);
      info->ns = name.namespace;
      info->name = apr_pstrdup(info->pool, name.name);
    }
  else if (state == RESPONSE && strcmp(name.name, "prop") == 0)
    {
      push_state(parser, ctx, PROP);
    }
  else if (state == PROP)
    {
      info = push_state(parser, ctx, PROPVAL);
      info->ns = name.namespace;
      info->name = apr_pstrdup(info->pool, name.name);
      info->encoding = apr_pstrdup(info->pool,
                                   svn_xml_get_attr_value("V:encoding", attrs));
    }

  return SVN_NO_ERROR;
}

/*
 * Expat callback invoked on an end element tag for a PROPFIND response.
 */
static svn_error_t *
end_propfind(svn_ra_serf__xml_parser_t *parser,
             void *userData,
             svn_ra_serf__dav_props_t name)
{
  svn_ra_serf__propfind_context_t *ctx = userData;
  prop_state_e state;
  prop_info_t *info;

  state = parser->state->current_state;
  info = parser->state->private;

  if (state == RESPONSE && strcmp(name.name, "response") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == PROP && strcmp(name.name, "prop") == 0)
    {
      svn_ra_serf__xml_pop_state(parser);
    }
  else if (state == PROPVAL)
    {
      const char *ns, *pname, *val;
      svn_string_t *val_str;

      /* if we didn't see a CDATA element, we may want the tag name
       * as long as it isn't equivalent to the property name.
       */
      if (!info->val)
        {
          if (strcmp(info->name, name.name) != 0)
            {
              info->val = name.name;
              info->val_len = strlen(info->val);
            }
          else
            {
              info->val = "";
              info->val_len = 0;
            }
        }

      if (parser->state->prev->current_state == RESPONSE &&
          strcmp(name.name, "href") == 0)
        {
          if (strcmp(ctx->depth, "1") == 0)
            {
              ctx->current_path =
                svn_urlpath__canonicalize(info->val, ctx->pool);
            }
          else
            {
              ctx->current_path = ctx->path;
            }
        }
      else if (info->encoding)
        {
          if (strcmp(info->encoding, "base64") == 0)
            {
              svn_string_t encoded;
              const svn_string_t *decoded;

              encoded.data = info->val;
              encoded.len = info->val_len;

              decoded = svn_base64_decode_string(&encoded, parser->state->pool);
              info->val = decoded->data;
              info->val_len = decoded->len;
            }
          else
            {
              return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA,
                                       NULL,
                                       _("Got unrecognized encoding '%s'"),
                                       info->encoding);
            }
        }

      ns = apr_pstrdup(ctx->pool, info->ns);
      pname = apr_pstrdup(ctx->pool, info->name);
      val = apr_pmemdup(ctx->pool, info->val, info->val_len);
      val_str = svn_string_ncreate(val, info->val_len, ctx->pool);

      /* set the return props and update our cache too. */
      svn_ra_serf__set_ver_prop(ctx->ret_props,
                                ctx->current_path, ctx->rev,
                                ns, pname, val_str,
                                ctx->pool);

      svn_ra_serf__xml_pop_state(parser);
    }

  return SVN_NO_ERROR;
}

/*
 * Expat callback invoked on CDATA elements in a PROPFIND response.
 *
 * This callback can be called multiple times.
 */
static svn_error_t *
cdata_propfind(svn_ra_serf__xml_parser_t *parser,
               void *userData,
               const char *data,
               apr_size_t len)
{
  svn_ra_serf__propfind_context_t *ctx = userData;
  prop_state_e state;
  prop_info_t *info;

  UNUSED_CTX(ctx);

  state = parser->state->current_state;
  info = parser->state->private;

  if (state == PROPVAL)
    {
      svn_ra_serf__expand_string(&info->val, &info->val_len, data, len,
                                 info->pool);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
setup_propfind_headers(serf_bucket_t *headers,
                        void *setup_baton,
                        apr_pool_t *pool)
{
  svn_ra_serf__propfind_context_t *ctx = setup_baton;

  if (ctx->conn->using_compression)
    {
      serf_bucket_headers_setn(headers, "Accept-Encoding", "gzip");
    }
  serf_bucket_headers_setn(headers, "Depth", ctx->depth);
  if (ctx->label)
    {
      serf_bucket_headers_setn(headers, "Label", ctx->label);
    }

  return SVN_NO_ERROR;
}

#define PROPFIND_HEADER "<?xml version=\"1.0\" encoding=\"utf-8\"?><propfind xmlns=\"DAV:\">"
#define PROPFIND_TRAILER "</propfind>"

static svn_error_t *
create_propfind_body(serf_bucket_t **bkt,
                     void *setup_baton,
                     serf_bucket_alloc_t *alloc,
                     apr_pool_t *pool)
{
  svn_ra_serf__propfind_context_t *ctx = setup_baton;

  serf_bucket_t *body_bkt, *tmp;
  const svn_ra_serf__dav_props_t *prop;
  svn_boolean_t requested_allprop = FALSE;

  body_bkt = serf_bucket_aggregate_create(alloc);

  prop = ctx->find_props;
  while (prop && prop->namespace)
    {
      /* special case the allprop case. */
      if (strcmp(prop->name, "allprop") == 0)
        {
          requested_allprop = TRUE;
        }

      /* <*propname* xmlns="*propns*" /> */
      tmp = SERF_BUCKET_SIMPLE_STRING_LEN("<", 1, alloc);
      serf_bucket_aggregate_append(body_bkt, tmp);

      tmp = SERF_BUCKET_SIMPLE_STRING(prop->name, alloc);
      serf_bucket_aggregate_append(body_bkt, tmp);

      tmp = SERF_BUCKET_SIMPLE_STRING_LEN(" xmlns=\"",
                                          sizeof(" xmlns=\"")-1,
                                          alloc);
      serf_bucket_aggregate_append(body_bkt, tmp);

      tmp = SERF_BUCKET_SIMPLE_STRING(prop->namespace, alloc);
      serf_bucket_aggregate_append(body_bkt, tmp);

      tmp = SERF_BUCKET_SIMPLE_STRING_LEN("\"/>", sizeof("\"/>")-1,
                                          alloc);
      serf_bucket_aggregate_append(body_bkt, tmp);

      prop++;
    }

  /* If we're not doing an allprop, add <prop> tags. */
  if (requested_allprop == FALSE)
    {
      tmp = SERF_BUCKET_SIMPLE_STRING_LEN("<prop>",
                                          sizeof("<prop>")-1,
                                          alloc);
      serf_bucket_aggregate_prepend(body_bkt, tmp);
    }

  tmp = SERF_BUCKET_SIMPLE_STRING_LEN(PROPFIND_HEADER,
                                      sizeof(PROPFIND_HEADER)-1,
                                      alloc);

  serf_bucket_aggregate_prepend(body_bkt, tmp);

  if (requested_allprop == FALSE)
    {
      tmp = SERF_BUCKET_SIMPLE_STRING_LEN("</prop>",
                                          sizeof("</prop>")-1,
                                          alloc);
      serf_bucket_aggregate_append(body_bkt, tmp);
    }

  tmp = SERF_BUCKET_SIMPLE_STRING_LEN(PROPFIND_TRAILER,
                                      sizeof(PROPFIND_TRAILER)-1,
                                      alloc);
  serf_bucket_aggregate_append(body_bkt, tmp);

  *bkt = body_bkt;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_ra_serf__deliver_props(svn_ra_serf__propfind_context_t **prop_ctx,
                           apr_hash_t *ret_props,
                           svn_ra_serf__session_t *sess,
                           svn_ra_serf__connection_t *conn,
                           const char *path,
                           svn_revnum_t rev,
                           const char *depth,
                           const svn_ra_serf__dav_props_t *find_props,
                           svn_ra_serf__list_t **done_list,
                           apr_pool_t *pool)
{
  svn_ra_serf__propfind_context_t *new_prop_ctx;
  svn_ra_serf__handler_t *handler;
  svn_ra_serf__xml_parser_t *parser_ctx;

  new_prop_ctx = apr_pcalloc(pool, sizeof(*new_prop_ctx));

  new_prop_ctx->pool = apr_hash_pool_get(ret_props);
  new_prop_ctx->path = path;
  new_prop_ctx->find_props = find_props;
  new_prop_ctx->ret_props = ret_props;
  new_prop_ctx->depth = depth;
  new_prop_ctx->done = FALSE;
  new_prop_ctx->sess = sess;
  new_prop_ctx->conn = conn;
  new_prop_ctx->rev = rev;
  new_prop_ctx->done_list = done_list;

  if (SVN_IS_VALID_REVNUM(rev))
    {
      new_prop_ctx->label = apr_ltoa(pool, rev);
    }
  else
    {
      new_prop_ctx->label = NULL;
    }

  handler = apr_pcalloc(pool, sizeof(*handler));

  handler->method = "PROPFIND";
  handler->path = path;
  handler->body_delegate = create_propfind_body;
  handler->body_type = "text/xml";
  handler->body_delegate_baton = new_prop_ctx;
  handler->header_delegate = setup_propfind_headers;
  handler->header_delegate_baton = new_prop_ctx;

  handler->session = new_prop_ctx->sess;
  handler->conn = new_prop_ctx->conn;

  new_prop_ctx->handler = handler;

  parser_ctx = apr_pcalloc(pool, sizeof(*new_prop_ctx->parser_ctx));
  parser_ctx->pool = pool;
  parser_ctx->user_data = new_prop_ctx;
  parser_ctx->start = start_propfind;
  parser_ctx->end = end_propfind;
  parser_ctx->cdata = cdata_propfind;
  parser_ctx->status_code = &new_prop_ctx->status_code;
  parser_ctx->done = &new_prop_ctx->done;
  parser_ctx->done_list = new_prop_ctx->done_list;
  parser_ctx->done_item = &new_prop_ctx->done_item;

  new_prop_ctx->parser_ctx = parser_ctx;

  handler->response_handler = svn_ra_serf__handle_xml_parser;
  handler->response_baton = parser_ctx;

  /* create request */
  svn_ra_serf__request_create(new_prop_ctx->handler);

  *prop_ctx = new_prop_ctx;

  return SVN_NO_ERROR;
}

svn_boolean_t
svn_ra_serf__propfind_is_done(svn_ra_serf__propfind_context_t *ctx)
{
  return ctx->done;
}

int
svn_ra_serf__propfind_status_code(svn_ra_serf__propfind_context_t *ctx)
{
  return ctx->status_code;
}

/*
 * This helper function will block until the PROP_CTX indicates that is done
 * or another error is returned.
 */
svn_error_t *
svn_ra_serf__wait_for_props(svn_ra_serf__propfind_context_t *prop_ctx,
                            svn_ra_serf__session_t *sess,
                            apr_pool_t *pool)
{
  svn_error_t *err, *err2;

  err = svn_ra_serf__context_run_wait(&prop_ctx->done, sess, pool);

  err2 = svn_ra_serf__error_on_status(prop_ctx->status_code,
                                      prop_ctx->path, NULL);
  if (err2)
    {
      svn_error_clear(err);
      return err2;
    }

  return err;
}

/*
 * This is a blocking version of deliver_props.
 */
svn_error_t *
svn_ra_serf__retrieve_props(apr_hash_t **results,
                            svn_ra_serf__session_t *sess,
                            svn_ra_serf__connection_t *conn,
                            const char *url,
                            svn_revnum_t rev,
                            const char *depth,
                            const svn_ra_serf__dav_props_t *props,
                            apr_pool_t *result_pool,
                            apr_pool_t *scratch_pool)
{
  svn_ra_serf__propfind_context_t *prop_ctx;

  *results = apr_hash_make(result_pool);

  SVN_ERR(svn_ra_serf__deliver_props(&prop_ctx, *results, sess, conn, url,
                                     rev, depth, props, NULL, result_pool));
  SVN_ERR(svn_ra_serf__wait_for_props(prop_ctx, sess, result_pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_ra_serf__walk_all_props(apr_hash_t *props,
                            const char *name,
                            svn_revnum_t rev,
                            svn_ra_serf__walker_visitor_t walker,
                            void *baton,
                            apr_pool_t *scratch_pool)
{
  apr_hash_index_t *ns_hi;
  apr_hash_t *ver_props;
  apr_hash_t *path_props;
  apr_pool_t *iterpool;

  ver_props = apr_hash_get(props, &rev, sizeof(rev));
  if (!ver_props)
    {
      return SVN_NO_ERROR;
    }

  path_props = apr_hash_get(ver_props, name, APR_HASH_KEY_STRING);
  if (!path_props)
    {
      return SVN_NO_ERROR;
    }

  iterpool = svn_pool_create(scratch_pool);
  for (ns_hi = apr_hash_first(scratch_pool, path_props); ns_hi;
       ns_hi = apr_hash_next(ns_hi))
    {
      void *ns_val;
      const void *ns_name;
      apr_hash_index_t *name_hi;

      /* NOTE: We do not clear ITERPOOL in this loop. Generally, there are
           very few namespaces, so this loop will not have many iterations.
           Instead, ITERPOOL is used for the inner loop.  */

      apr_hash_this(ns_hi, &ns_name, NULL, &ns_val);

      for (name_hi = apr_hash_first(scratch_pool, ns_val); name_hi;
           name_hi = apr_hash_next(name_hi))
        {
          void *prop_val;
          const void *prop_name;

          /* See note above, regarding clearing of this pool.  */
          svn_pool_clear(iterpool);

          apr_hash_this(name_hi, &prop_name, NULL, &prop_val);

          SVN_ERR(walker(baton, ns_name, prop_name, prop_val, iterpool));
        }
    }
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_ra_serf__walk_all_paths(apr_hash_t *props,
                            svn_revnum_t rev,
                            svn_ra_serf__path_rev_walker_t walker,
                            void *baton,
                            apr_pool_t *pool)
{
  apr_hash_index_t *path_hi;
  apr_hash_t *ver_props;

  ver_props = apr_hash_get(props, &rev, sizeof(rev));

  if (!ver_props)
    {
      return SVN_NO_ERROR;
    }

  for (path_hi = apr_hash_first(pool, ver_props); path_hi;
       path_hi = apr_hash_next(path_hi))
    {
      void *path_props;
      const void *path_name;
      apr_ssize_t path_len;
      apr_hash_index_t *ns_hi;

      apr_hash_this(path_hi, &path_name, &path_len, &path_props);
      for (ns_hi = apr_hash_first(pool, path_props); ns_hi;
           ns_hi = apr_hash_next(ns_hi))
        {
          void *ns_val;
          const void *ns_name;
          apr_ssize_t ns_len;
          apr_hash_index_t *name_hi;
          apr_hash_this(ns_hi, &ns_name, &ns_len, &ns_val);
          for (name_hi = apr_hash_first(pool, ns_val); name_hi;
               name_hi = apr_hash_next(name_hi))
            {
              void *prop_val;
              const void *prop_name;
              apr_ssize_t prop_len;

              apr_hash_this(name_hi, &prop_name, &prop_len, &prop_val);
              /* use a subpool? */
              SVN_ERR(walker(baton, path_name, path_len, ns_name, ns_len,
                             prop_name, prop_len, prop_val, pool));
            }
        }
    }

  return SVN_NO_ERROR;
}


const char *
svn_ra_serf__svnname_from_wirename(const char *ns,
                                   const char *name,
                                   apr_pool_t *result_pool)
{
  if (*ns == '\0' || strcmp(ns, SVN_DAV_PROP_NS_CUSTOM) == 0)
    return apr_pstrdup(result_pool, name);

  if (strcmp(ns, SVN_DAV_PROP_NS_SVN) == 0)
    return apr_pstrcat(result_pool, SVN_PROP_PREFIX, name, (char *)NULL);

  if (strcmp(ns, SVN_PROP_PREFIX) == 0)
    return apr_pstrcat(result_pool, SVN_PROP_PREFIX, name, (char *)NULL);

  if (strcmp(name, SVN_DAV__VERSION_NAME) == 0)
    return SVN_PROP_ENTRY_COMMITTED_REV;

  if (strcmp(name, SVN_DAV__CREATIONDATE) == 0)
    return SVN_PROP_ENTRY_COMMITTED_DATE;

  if (strcmp(name, "creator-displayname") == 0)
    return SVN_PROP_ENTRY_LAST_AUTHOR;

  if (strcmp(name, "repository-uuid") == 0)
    return SVN_PROP_ENTRY_UUID;

  if (strcmp(name, "lock-token") == 0)
    return SVN_PROP_ENTRY_LOCK_TOKEN;

  if (strcmp(name, "checked-in") == 0)
    return SVN_RA_SERF__WC_CHECKED_IN_URL;

  if (strcmp(ns, "DAV:") == 0 || strcmp(ns, SVN_DAV_PROP_NS_DAV) == 0)
    {
      /* Here DAV: properties not yet converted to svn: properties should be
         ignored. */
      return NULL;
    }

  /* An unknown namespace, must be a custom property. */
  return apr_pstrcat(result_pool, ns, name, (char *)NULL);
}


/* Conforms to svn_ra_serf__walker_visitor_t  */
static svn_error_t *
set_flat_props(void *baton,
               const char *ns,
               const char *name,
               const svn_string_t *value,
               apr_pool_t *pool)
{
  apr_hash_t *props = baton;
  apr_pool_t *result_pool = apr_hash_pool_get(props);
  const char *prop_name;

  /* ### is VAL in the proper pool?  */

  prop_name = svn_ra_serf__svnname_from_wirename(ns, name, result_pool);
  if (prop_name != NULL)
    apr_hash_set(props, prop_name, APR_HASH_KEY_STRING, value);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_ra_serf__flatten_props(apr_hash_t **flat_props,
                           apr_hash_t *props,
                           const char *path,
                           svn_revnum_t revision,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  *flat_props = apr_hash_make(result_pool);

  return svn_error_trace(svn_ra_serf__walk_all_props(
                            props, path, revision,
                            set_flat_props,
                            *flat_props /* baton */,
                            scratch_pool));
}


static svn_error_t *
select_revprops(void *baton,
                const char *ns,
                const char *name,
                const svn_string_t *val,
                apr_pool_t *scratch_pool)
{
  apr_hash_t *revprops = baton;
  apr_pool_t *result_pool = apr_hash_pool_get(revprops);
  const char *prop_name;

  /* ### copy NAME into the RESULT_POOL?  */
  /* ### copy VAL into the RESULT_POOL?  */

  if (strcmp(ns, SVN_DAV_PROP_NS_CUSTOM) == 0)
    prop_name = name;
  else if (strcmp(ns, SVN_DAV_PROP_NS_SVN) == 0)
    prop_name = apr_pstrcat(result_pool, SVN_PROP_PREFIX, name, (char *)NULL);
  else if (strcmp(ns, SVN_PROP_PREFIX) == 0)
    prop_name = apr_pstrcat(result_pool, SVN_PROP_PREFIX, name, (char *)NULL);
  else if (strcmp(ns, "") == 0)
    prop_name = name;
  else
    {
      /* do nothing for now? */
      return SVN_NO_ERROR;
    }

  apr_hash_set(revprops, prop_name, APR_HASH_KEY_STRING, val);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_ra_serf__select_revprops(apr_hash_t **revprops,
                             const char *name,
                             svn_revnum_t rev,
                             apr_hash_t *all_revprops,
                             apr_pool_t *result_pool,
                             apr_pool_t *scratch_pool)
{
  *revprops = apr_hash_make(result_pool);

  return svn_error_trace(svn_ra_serf__walk_all_props(
                            all_revprops, name, rev,
                            select_revprops, *revprops,
                            scratch_pool));
}


/*
 * Contact the server (using SESSION and CONN) to calculate baseline
 * information for BASELINE_URL at REVISION (which may be
 * SVN_INVALID_REVNUM to query the HEAD revision).
 *
 * If ACTUAL_REVISION is non-NULL, set *ACTUAL_REVISION to revision
 * retrieved from the server as part of this process (which should
 * match REVISION when REVISION is valid).  Set *BASECOLL_URL_P to the
 * baseline collection URL.
 */
static svn_error_t *
retrieve_baseline_info(svn_revnum_t *actual_revision,
                       const char **basecoll_url_p,
                       svn_ra_serf__session_t *session,
                       svn_ra_serf__connection_t *conn,
                       const char *baseline_url,
                       svn_revnum_t revision,
                       apr_pool_t *pool)
{
  apr_hash_t *props;
  const char *basecoll_url;
  const char *version_name;

  SVN_ERR(svn_ra_serf__retrieve_props(&props, session, conn,
                                      baseline_url, revision, "0",
                                      baseline_props,
                                      pool, pool));

  basecoll_url = svn_ra_serf__get_ver_prop(props, baseline_url, revision,
                                           "DAV:", "baseline-collection");

  if (!basecoll_url)
    {
      return svn_error_create(SVN_ERR_RA_DAV_PROPS_NOT_FOUND, NULL,
                              _("The PROPFIND response did not include "
                                "the requested baseline-collection value"));
    }

  *basecoll_url_p = svn_urlpath__canonicalize(basecoll_url, pool);

  version_name = svn_ra_serf__get_ver_prop(props, baseline_url, revision,
                                           "DAV:", SVN_DAV__VERSION_NAME);
  if (!version_name)
    {
      return svn_error_create(SVN_ERR_RA_DAV_PROPS_NOT_FOUND, NULL,
                              _("The PROPFIND response did not include "
                                "the requested version-name value"));
    }

  if (actual_revision)
    {
      *actual_revision = SVN_STR_TO_REV(version_name);
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_ra_serf__get_baseline_info(const char **bc_url,
                               const char **bc_relative,
                               svn_ra_serf__session_t *session,
                               svn_ra_serf__connection_t *conn,
                               const char *url,
                               svn_revnum_t revision,
                               svn_revnum_t *latest_revnum,
                               apr_pool_t *pool)
{
  const char *vcc_url, *relative_url, *basecoll_url, *baseline_url;

  /* No URL?  No sweat.  We'll use the session URL. */
  if (! url)
    url = session->session_url.path;

  /* If the caller didn't provide a specific connection for us to use,
     we'll use the default one.  */
  if (! conn)
    conn = session->conns[0];

  /* If we detected HTTP v2 support on the server, we can construct
     the baseline collection URL ourselves, and fetch the latest
     revision (if needed) with an OPTIONS request.  */
  if (SVN_RA_SERF__HAVE_HTTPV2_SUPPORT(session))
    {
      svn_revnum_t actual_revision;

      if (SVN_IS_VALID_REVNUM(revision))
        {
          actual_revision = revision;
        }
      else
        {
          svn_ra_serf__options_context_t *opt_ctx;

          SVN_ERR(svn_ra_serf__create_options_req(&opt_ctx, session, conn,
                                                  session->session_url.path,
                                                  pool));
          SVN_ERR(svn_ra_serf__context_run_wait(
                svn_ra_serf__get_options_done_ptr(opt_ctx), session, pool));

          actual_revision = svn_ra_serf__options_get_youngest_rev(opt_ctx);
          if (! SVN_IS_VALID_REVNUM(actual_revision))
            return svn_error_create(SVN_ERR_RA_DAV_OPTIONS_REQ_FAILED, NULL,
                                    _("The OPTIONS response did not include "
                                      "the youngest revision"));
        }

      basecoll_url = apr_psprintf(pool, "%s/%ld",
                                  session->rev_root_stub, actual_revision);
      if (latest_revnum)
        *latest_revnum = actual_revision;
    }

  /* Otherwise, we fall back to the old VCC_URL PROPFIND hunt.  */
  else
    {
      SVN_ERR(svn_ra_serf__discover_vcc(&vcc_url, session, conn, pool));

      if (SVN_IS_VALID_REVNUM(revision))
        {
          /* First check baseline information cache. */
          SVN_ERR(svn_ra_serf__blncache_get_bc_url(&basecoll_url,
                                                   session->blncache,
                                                   revision, pool));

          if (!basecoll_url)
            {
              SVN_ERR(retrieve_baseline_info(NULL, &basecoll_url, session,
                                             conn, vcc_url, revision, pool));
              SVN_ERR(svn_ra_serf__blncache_set(session->blncache, NULL,
                                                revision, basecoll_url, pool));
            }

          if (latest_revnum)
            {
              *latest_revnum = revision;
            }
        }
      else
        {
          apr_hash_t *props;
          svn_revnum_t actual_revision;

          SVN_ERR(svn_ra_serf__retrieve_props(&props, session, conn,
                                              vcc_url, revision, "0",
                                              checked_in_props,
                                              pool, pool));
          baseline_url = svn_ra_serf__get_ver_prop(props, vcc_url, revision,
                                                   "DAV:", "checked-in");
          if (!baseline_url)
            {
              return svn_error_create(SVN_ERR_RA_DAV_OPTIONS_REQ_FAILED, NULL,
                                      _("The OPTIONS response did not include "
                                        "the requested checked-in value"));
            }

          baseline_url = svn_urlpath__canonicalize(baseline_url, pool);

          /* First check baseline information cache. */
          SVN_ERR(svn_ra_serf__blncache_get_baseline_info(&basecoll_url,
                                                          &actual_revision,
                                                          session->blncache,
                                                          baseline_url,
                                                          pool));
          if (!basecoll_url)
            {
              SVN_ERR(retrieve_baseline_info(&actual_revision, &basecoll_url,
                                             session, conn,
                                             baseline_url, revision, pool));
              SVN_ERR(svn_ra_serf__blncache_set(session->blncache,
                                                baseline_url, actual_revision,
                                                basecoll_url, pool));
            }

          if (latest_revnum)
            {
              *latest_revnum = actual_revision;
            }
        }
    }

  /* And let's not forget to calculate our relative path. */
  SVN_ERR(svn_ra_serf__get_relative_path(&relative_url, url, session,
                                         conn, pool));

  *bc_url = basecoll_url;
  *bc_relative = relative_url;
  return SVN_NO_ERROR;
}


svn_error_t *
svn_ra_serf__get_resource_type(svn_node_kind_t *kind,
                               apr_hash_t *props,
                               const char *url,
                               svn_revnum_t revision)
{
  const char *res_type;

  res_type = svn_ra_serf__get_ver_prop(props, url, revision,
                                       "DAV:", "resourcetype");
  if (!res_type)
    {
      /* How did this happen? */
      return svn_error_create(SVN_ERR_RA_DAV_PROPS_NOT_FOUND, NULL,
                              _("The PROPFIND response did not include the "
                              "requested resourcetype value"));
    }

  if (strcmp(res_type, "collection") == 0)
    {
      *kind = svn_node_dir;
    }
  else
    {
      *kind = svn_node_file;
    }

  return SVN_NO_ERROR;
}
