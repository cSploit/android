/*
 * lock.c :  routines for managing lock states in the DAV server
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



#define APR_WANT_STRFUNC
#include <apr_want.h>
#include <apr_xml.h>

#include "svn_error.h"
#include "svn_pools.h"
#include "svn_ra.h"
#include "../libsvn_ra/ra_loader.h"
#include "svn_path.h"
#include "svn_string.h"
#include "svn_time.h"
#include "svn_private_config.h"

#include "ra_neon.h"

static const svn_ra_neon__xml_elm_t lock_elements[] =
{
  /* lockdiscovery elements */
  { "DAV:", "response", ELEM_response, 0 },
  { "DAV:", "propstat", ELEM_propstat, 0 },
  { "DAV:", "status", ELEM_status, SVN_RA_NEON__XML_CDATA },
  /* extend lockdiscovery elements here;
     ### Remember to update do_lock() when you change the number of
     elements here: it contains a hard reference to the next element. */

  /* lock and lockdiscovery elements */
  { "DAV:", "prop", ELEM_prop, 0 },
  { "DAV:", "lockdiscovery", ELEM_lock_discovery, 0 },
  { "DAV:", "activelock", ELEM_lock_activelock, 0 },
  { "DAV:", "locktype", ELEM_lock_type, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "lockscope", ELEM_lock_scope, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "depth", ELEM_lock_depth, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "owner", ELEM_lock_owner, SVN_RA_NEON__XML_COLLECT },
  { "DAV:", "timeout", ELEM_lock_timeout, SVN_RA_NEON__XML_CDATA },
  { "DAV:", "locktoken", ELEM_lock_token, 0 },
  { "DAV:", "href", ELEM_href, SVN_RA_NEON__XML_CDATA },
  { "", "", ELEM_unknown, SVN_RA_NEON__XML_COLLECT },
  /* extend lock elements here */

  { NULL }
};

typedef struct lock_baton_t
{
  svn_stringbuf_t *cdata;
  apr_pool_t *pool;
  const svn_ra_neon__xml_elm_t *xml_table;

  /* lockdiscovery fields */
  svn_stringbuf_t *href;
  svn_stringbuf_t *status_line;

  /* lock and lockdiscovery fields */
  int parent;
  svn_stringbuf_t *owner;
  svn_stringbuf_t *timeout;
  svn_stringbuf_t *depth;
  svn_stringbuf_t *token;
} lock_baton_t;

static svn_error_t *
lock_start_element(int *elem, void *baton, int parent,
                   const char *nspace, const char *name, const char **atts)
{
  lock_baton_t *b = baton;
  const svn_ra_neon__xml_elm_t *elm =
    svn_ra_neon__lookup_xml_elem(b->xml_table, nspace, name);

  if (! elm)
    {
      *elem = NE_XML_DECLINE;
      return SVN_NO_ERROR;
    }

  /* collect interesting element contents */
  /* owner, href inside locktoken, depth, timeout */
  switch (elm->id)
    {
    case ELEM_lock_owner:
    case ELEM_lock_timeout:
    case ELEM_lock_depth:
    case ELEM_status:
      b->cdata = svn_stringbuf_create("", b->pool);
      break;

    case ELEM_href:
      if (parent == ELEM_lock_token
          || parent == ELEM_response)
        b->cdata = svn_stringbuf_create("", b->pool);
      break;

    default:
      b->cdata = NULL;
    }

  b->parent = parent;
  *elem = elm->id;
  return SVN_NO_ERROR;
}

static svn_error_t *
lock_end_element(void *baton, int state, const char *nspace, const char *name)
{
  lock_baton_t *b = baton;

  if (b->cdata)
    switch (state)
      {
      case ELEM_lock_owner:
        b->owner = b->cdata;
        break;

      case ELEM_lock_timeout:
        b->timeout = b->cdata;
        break;

      case ELEM_lock_depth:
        b->depth = b->cdata;
        break;

      case ELEM_href:
        if (b->parent == ELEM_lock_token)
          b->token = b->cdata;
        else
          b->href = b->cdata;
        break;

      case ELEM_status:
        b->status_line = b->cdata;
        break;
      }

  b->cdata = NULL;

  return SVN_NO_ERROR;
}

static svn_error_t *
lock_cdata(void *baton, int state, const char *cdata, size_t len)
{
  lock_baton_t *b = baton;

  if (b->cdata)
    svn_stringbuf_appendbytes(b->cdata, cdata, len);

  return SVN_NO_ERROR;
}


static svn_error_t *
lock_from_baton(svn_lock_t **lock,
                svn_ra_neon__request_t *req,
                const char *path,
                lock_baton_t *lrb, apr_pool_t *pool)
{
  const char *val;
  svn_lock_t *lck = svn_lock_create(pool);

  if (lrb->token)
    lck->token = lrb->token->data;
  else
    {
      /* No lock */
      *lock = NULL;
      return SVN_NO_ERROR;
    }

  val = ne_get_response_header(req->ne_req, SVN_DAV_CREATIONDATE_HEADER);
  if (val)
    SVN_ERR_W(svn_time_from_cstring(&(lck->creation_date), val, pool),
              _("Invalid creation date header value in response."));

  val = ne_get_response_header(req->ne_req, SVN_DAV_LOCK_OWNER_HEADER);
  if (val)
    lck->owner = apr_pstrdup(pool, val);
  if (lrb->owner)
    lck->comment = lrb->owner->data;
  if (path)
    lck->path = path;
  if (lrb->timeout)
    {
      const char *timeout_str = lrb->timeout->data;

      if (strcmp(timeout_str, "Infinite") != 0)
        {
          if (strncmp("Second-", timeout_str, strlen("Second-")) == 0)
            {
              int time_offset;

              SVN_ERR(svn_cstring_atoi(&time_offset, &(timeout_str[7])));
              lck->expiration_date = lck->creation_date
                + apr_time_from_sec(time_offset);
            }
          else
            return svn_error_create(SVN_ERR_RA_DAV_RESPONSE_HEADER_BADNESS,
                                    NULL, _("Invalid timeout value"));
        }
      else
        lck->expiration_date = 0;
    }
  *lock = lck;

  return SVN_NO_ERROR;
}

static svn_error_t *
do_lock(svn_lock_t **lock,
        svn_ra_session_t *session,
        const char *path,
        const char *comment,
        svn_boolean_t force,
        svn_revnum_t current_rev,
        apr_pool_t *pool)
{
  svn_ra_neon__request_t *req;
  svn_stringbuf_t *body;
  ne_uri uri;
  int code;
  const char *url;
  const char *fs_path;
  ne_xml_parser *lck_parser;
  svn_ra_neon__session_t *ras = session->priv;
  lock_baton_t *lrb = apr_pcalloc(pool, sizeof(*lrb));
  apr_hash_t *extra_headers;
  svn_error_t *err = SVN_NO_ERROR;

  /* To begin, we convert the incoming path into an absolute fs-path. */
  url = svn_path_url_add_component2(ras->url->data, path, pool);
  SVN_ERR(svn_ra_neon__get_baseline_info(NULL, &fs_path, NULL, ras,
                                         url, SVN_INVALID_REVNUM, pool));

  if (ne_uri_parse(url, &uri) != 0)
    {
      ne_uri_free(&uri);
      return svn_error_createf(SVN_ERR_RA_DAV_CREATING_REQUEST, NULL,
                               _("Failed to parse URI '%s'"), url);
    }

  SVN_ERR(svn_ra_neon__request_create(&req, ras, "LOCK", uri.path, pool));
  ne_uri_free(&uri);

  lrb->pool = pool;
  lrb->xml_table = &(lock_elements[3]);
  lck_parser = svn_ra_neon__xml_parser_create
    (req, ne_accept_2xx,
     lock_start_element, lock_cdata, lock_end_element, lrb);

  body = svn_stringbuf_createf
    (req->pool,
     "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" DEBUG_CR
     "<D:lockinfo xmlns:D=\"DAV:\">" DEBUG_CR
     " <D:lockscope><D:exclusive /></D:lockscope>" DEBUG_CR
     " <D:locktype><D:write /></D:locktype>" DEBUG_CR
     "%s" /* maybe owner */
     "</D:lockinfo>",
     comment ? apr_pstrcat(pool,
                           "<D:owner>",
                           apr_xml_quote_string(pool, comment, 0),
                           "</D:owner>",
                           (char *)NULL)
             : "");

  extra_headers = apr_hash_make(req->pool);
  svn_ra_neon__set_header(extra_headers, "Depth", "0");
  svn_ra_neon__set_header(extra_headers, "Timeout", "Infinite");
  svn_ra_neon__set_header(extra_headers, "Content-Type",
                         "text/xml; charset=\"utf-8\"");
  if (force)
    svn_ra_neon__set_header(extra_headers, SVN_DAV_OPTIONS_HEADER,
                            SVN_DAV_OPTION_LOCK_STEAL);
  if (SVN_IS_VALID_REVNUM(current_rev))
    svn_ra_neon__set_header(extra_headers, SVN_DAV_VERSION_NAME_HEADER,
                            apr_psprintf(req->pool, "%ld", current_rev));

  err = svn_ra_neon__request_dispatch(&code, req, extra_headers, body->data,
                                      200, 0, pool);
  if (err)
    goto cleanup;

  err = svn_ra_neon__check_parse_error("LOCK", lck_parser, url);
  if (err)
    goto cleanup;

  /*###FIXME: we never verified whether we have received back the type
    of lock we requested: was it shared/exclusive? was it write/otherwise?
    How many did we get back? Only one? */
  err = lock_from_baton(lock, req, fs_path, lrb, pool);

 cleanup:
  /* 405 == Method Not Allowed (Occurs when trying to lock a working
     copy path which no longer exists at HEAD in the repository. */
  if (code == 405)
    {
      svn_error_clear(err);
      err = svn_error_createf(SVN_ERR_FS_OUT_OF_DATE, NULL,
                              _("Lock request failed: %d %s"),
                              code, req->code_desc);
    }
  svn_ra_neon__request_destroy(req);

  return err;
}

svn_error_t *
svn_ra_neon__lock(svn_ra_session_t *session,
                  apr_hash_t *path_revs,
                  const char *comment,
                  svn_boolean_t force,
                  svn_ra_lock_callback_t lock_func,
                  void *lock_baton,
                  apr_pool_t *pool)
{
  apr_hash_index_t *hi;
  apr_pool_t *iterpool = svn_pool_create(pool);
  svn_ra_neon__session_t *ras = session->priv;
  svn_error_t *ret_err = NULL;

  /* ### TODO for issue 2263: Send all the locks over the wire at once.  This
     loop is just a temporary shim. */
  for (hi = apr_hash_first(pool, path_revs); hi; hi = apr_hash_next(hi))
    {
      svn_lock_t *lock;
      const void *key;
      const char *path;
      void *val;
      svn_revnum_t *revnum;
      svn_error_t *err, *callback_err = NULL;

      svn_pool_clear(iterpool);

      apr_hash_this(hi, &key, NULL, &val);
      path = key;
      revnum = val;

      err = do_lock(&lock, session, path, comment, force, *revnum, iterpool);

      if (err && !SVN_ERR_IS_LOCK_ERROR(err))
        {
          ret_err = err;
          goto departure;
        }

      if (lock_func)
        callback_err = lock_func(lock_baton, path, TRUE, err ? NULL : lock,
                                 err, iterpool);

      svn_error_clear(err);

      if (callback_err)
        {
          ret_err = callback_err;
          goto departure;
        }

    }

  svn_pool_destroy(iterpool);

 departure:
  return svn_ra_neon__maybe_store_auth_info_after_result(ret_err, ras, pool);
}


/* ###TODO for issue 2263: Send all lock tokens to the server at once. */
static svn_error_t *
do_unlock(svn_ra_session_t *session,
          const char *path,
          const char *token,
          svn_boolean_t force,
          const svn_lock_t **old_lock,
          apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  const char *url;
  const char *url_path;
  ne_uri uri;
  svn_error_t *err = SVN_NO_ERROR;

  apr_hash_t *extra_headers = apr_hash_make(pool);

  if (old_lock)
    *old_lock = NULL;

  /* Make a neon lock structure containing token and full URL to unlock. */
  url = svn_path_url_add_component2(ras->url->data, path, pool);
  if (ne_uri_parse(url, &uri) != 0)
    {
      ne_uri_free(&uri);
      return svn_error_createf(SVN_ERR_RA_DAV_CREATING_REQUEST, NULL,
                               _("Failed to parse URI '%s'"), url);
    }

  url_path = apr_pstrdup(pool, uri.path);
  ne_uri_free(&uri);
  /* In the case of 'force', we might not have a token at all.
     Unfortunately, mod_dav insists on having a valid token for
     UNLOCK requests.  That means we need to fetch the token. */
  if (! token)
    {
      svn_lock_t *lock;

      SVN_ERR(svn_ra_neon__get_lock(session, &lock, path, pool));
      if (! lock)
        return svn_error_createf(SVN_ERR_RA_NOT_LOCKED, NULL,
                                 _("'%s' is not locked in the repository"),
                                 path);
      token = lock->token;
      if (old_lock)
        *old_lock = lock;
    }

  apr_hash_set(extra_headers, "Lock-Token", APR_HASH_KEY_STRING,
               apr_psprintf(pool, "<%s>", token));
  if (force)
    apr_hash_set(extra_headers, SVN_DAV_OPTIONS_HEADER, APR_HASH_KEY_STRING,
                 SVN_DAV_OPTION_LOCK_BREAK);

  {
    int code = 0;

    err = svn_ra_neon__simple_request(&code, ras, "UNLOCK", url_path,
                                      extra_headers, NULL, 204, 0, pool);

    if (err && ((err->apr_err == SVN_ERR_RA_DAV_REQUEST_FAILED)
                || (err->apr_err == SVN_ERR_RA_DAV_FORBIDDEN)))
      {
        switch (code)
          {
            case 403:
              return svn_error_createf(SVN_ERR_FS_LOCK_OWNER_MISMATCH, err,
                                       _("Unlock failed on '%s'"
                                         " (%d Forbidden)"), path, code);
            case 400:
               return svn_error_createf(SVN_ERR_FS_NO_SUCH_LOCK, err,
                                       _("No lock on path '%s'"
                                         " (%d Bad Request)"), path, code);
            default:
              break;
          }
      }
  }
  return svn_error_trace(err);
}


svn_error_t *
svn_ra_neon__unlock(svn_ra_session_t *session,
                    apr_hash_t *path_tokens,
                    svn_boolean_t force,
                    svn_ra_lock_callback_t lock_func,
                    void *lock_baton,
                    apr_pool_t *pool)
{
  apr_hash_index_t *hi;
  apr_pool_t *iterpool = svn_pool_create(pool);
  svn_ra_neon__session_t *ras = session->priv;
  svn_error_t *ret_err = NULL;

  /* ### TODO for issue 2263: Send all the lock tokens over the wire at once.
        This loop is just a temporary shim. */
  for (hi = apr_hash_first(pool, path_tokens); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      const char *path;
      void *val;
      const char *token;
      svn_error_t *err, *callback_err = NULL;
      const svn_lock_t *old_lock = NULL;

      svn_pool_clear(iterpool);

      apr_hash_this(hi, &key, NULL, &val);
      path = key;
      /* Since we can't store NULL values in a hash, we turn "" to
         NULL here. */
      if (strcmp(val, "") != 0)
        token = val;
      else
        token = NULL;

      err = do_unlock(session, path, token, force, &old_lock, iterpool);

      if (err && !SVN_ERR_IS_UNLOCK_ERROR(err))
        {
          ret_err = err;
          goto departure;
        }

      if (lock_func)
        callback_err = svn_error_trace(
                 lock_func(lock_baton, path, FALSE, old_lock, err, iterpool));

      svn_error_clear(err);

      if (callback_err)
        {
          ret_err = callback_err;
          goto departure;
        }
    }

  svn_pool_destroy(iterpool);

 departure:
  return svn_error_trace(
          svn_ra_neon__maybe_store_auth_info_after_result(ret_err, ras, pool));
}


svn_error_t *
svn_ra_neon__get_lock_internal(svn_ra_neon__session_t *ras,
                               svn_lock_t **lock,
                               const char *path,
                               apr_pool_t *pool)
{
  const char *url;
  const char *fs_path;
  svn_error_t *err;
  ne_uri uri;
  lock_baton_t *lrb = apr_pcalloc(pool, sizeof(*lrb));
  svn_ra_neon__request_t *req;
  ne_xml_parser *lck_parser;
  apr_hash_t *extra_headers;
  static const char *body =
    "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" DEBUG_CR
    "<D:propfind xmlns:D=\"DAV:\">" DEBUG_CR
    " <D:prop>" DEBUG_CR
    "  <D:lockdiscovery />" DEBUG_CR
    " </D:prop>" DEBUG_CR
    "</D:propfind>";

  /* To begin, we convert the incoming path into an absolute fs-path. */
  url = svn_path_url_add_component2(ras->url->data, path, pool);

  err = svn_ra_neon__get_baseline_info(NULL, &fs_path, NULL, ras,
                                       url, SVN_INVALID_REVNUM, pool);
  SVN_ERR(svn_ra_neon__maybe_store_auth_info_after_result(err, ras, pool));

  ne_uri_parse(url, &uri);
  url = apr_pstrdup(pool, uri.path);
  ne_uri_free(&uri);

  SVN_ERR(svn_ra_neon__request_create(&req, ras, "PROPFIND", url, pool));

  lrb->pool = pool;
  lrb->xml_table = lock_elements;
  lck_parser = svn_ra_neon__xml_parser_create
    (req, ne_accept_207, lock_start_element, lock_cdata, lock_end_element, lrb);

  extra_headers = apr_hash_make(req->pool);
  svn_ra_neon__set_header(extra_headers, "Depth", "0");
  svn_ra_neon__set_header(extra_headers, "Content-Type",
                          "text/xml; charset=\"utf-8\"");

  err = svn_ra_neon__request_dispatch(NULL, req, extra_headers, body,
                                      200, 207, pool);
  if (err)
    {
      err = svn_error_quick_wrap(err, _("Failed to fetch lock information"));
      goto cleanup;
    }

  err = svn_ra_neon__check_parse_error("PROPFIND", lck_parser, url);
  if (err)
    goto cleanup;

  /*###FIXME We assume here we only got one lock response. The WebDAV
    spec makes no such guarantees. How to make sure we grab the one we need? */
  err = lock_from_baton(lock, req, fs_path, lrb, pool);

 cleanup:
  svn_ra_neon__request_destroy(req);
  return err;
}


svn_error_t *
svn_ra_neon__get_lock(svn_ra_session_t *session,
                      svn_lock_t **lock,
                      const char *path,
                      apr_pool_t *pool)
{
  return svn_ra_neon__get_lock_internal(session->priv, lock, path, pool);
}
