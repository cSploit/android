/*
 * get_locks.c :  RA get-locks API implementation
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
#include <apr_want.h> /* for strcmp() */

#include <apr_pools.h>
#include <apr_tables.h>
#include <apr_strings.h>
#include <apr_xml.h>

#include <ne_basic.h>

#include "svn_error.h"
#include "svn_pools.h"
#include "svn_base64.h"
#include "svn_ra.h"
#include "../libsvn_ra/ra_loader.h"
#include "svn_path.h"
#include "svn_xml.h"
#include "svn_dav.h"
#include "svn_time.h"

#include "private/svn_dav_protocol.h"
#include "private/svn_fspath.h"
#include "svn_private_config.h"

#include "ra_neon.h"

/* -------------------------------------------------------------------------
**
** GET-LOCKS REPORT HANDLING
**
** DeltaV provides a mechanism for fetching a list of locks below a
** path, but it's often unscalable.  It requires doing a PROPFIND of
** depth infinity, looking for the 'DAV:lockdiscovery' prop on every
** resource.  But depth-infinity propfinds can sometimes behave like a
** DoS attack, and mod_dav even disables them by default!
**
** So we send a custom 'get-locks' REPORT on a public URI... which is
** fine, since all lock queries are always against HEAD anyway.  The
** response is a just a list of svn_lock_t's.  (Generic DAV clients,
** of course, are free to do infinite PROPFINDs as they wish, assuming
** the server allows it.)
*/

/* Elements used in a get-locks-report response */
static const svn_ra_neon__xml_elm_t getlocks_report_elements[] =
{
  { SVN_XML_NAMESPACE, "get-locks-report", ELEM_get_locks_report, 0 },
  { SVN_XML_NAMESPACE, "lock", ELEM_lock, 0},
  { SVN_XML_NAMESPACE, "path", ELEM_lock_path, SVN_RA_NEON__XML_CDATA },
  { SVN_XML_NAMESPACE, "token", ELEM_lock_token, SVN_RA_NEON__XML_CDATA },
  { SVN_XML_NAMESPACE, "owner", ELEM_lock_owner, SVN_RA_NEON__XML_CDATA },
  { SVN_XML_NAMESPACE, "comment", ELEM_lock_comment, SVN_RA_NEON__XML_CDATA },
  { SVN_XML_NAMESPACE, SVN_DAV__CREATIONDATE,
    ELEM_lock_creationdate, SVN_RA_NEON__XML_CDATA },
  { SVN_XML_NAMESPACE, "expirationdate",
    ELEM_lock_expirationdate, SVN_RA_NEON__XML_CDATA },
  { NULL }
};

/*
 * The get-locks-report xml request body is super-simple.
 * The server doesn't need anything but the URI in the REPORT request line.
 *
 *    <S:get-locks-report [depth=DEPTH] xmlns...>
 *    </S:get-locks-report>
 *
 * The get-locks-report xml response is just a list of svn_lock_t's
 * that exist at or "below" the request URI.  (The server runs
 * svn_repos_fs_get_locks()).
 *
 *    <S:get-locks-report xmlns...>
 *        <S:lock>
 *           <S:path>/foo/bar/baz</S:path>
 *           <S:token>opaquelocktoken:706689a6-8cef-0310-9809-fb7545cbd44e
 *                </S:token>
 *           <S:owner>fred</S:owner>
 *           <S:comment encoding="base64">ET39IGCB93LL4M</S:comment>
 *           <S:creationdate>2005-02-07T14:17:08Z</S:creationdate>
 *           <S:expirationdate>2005-02-08T14:17:08Z</S:expirationdate>
 *        </S:lock>
 *        ...
 *    </S:get-locks-report>
 *
 *
 * The <path> and <token> and date-element cdata is xml-escaped by mod_dav_svn.
 *
 * The <owner> and <comment> cdata is always xml-escaped, but
 * possibly also base64-encoded if necessary, as indicated by the
 * encoding attribute.
 *
 * The absence of <expirationdate> means that there's no expiration.
 *
 * If there are no locks to return, then the response will look just
 * like the request.
 */


/* Context for parsing server's response. */
typedef struct get_locks_baton_t {
  const char *path;                /* fspath target of the report */
  svn_depth_t requested_depth;     /* requested depth of the report */
  svn_lock_t *current_lock;        /* the lock being constructed */
  svn_stringbuf_t *cdata_accum;    /* a place to accumulate cdata */
  const char *encoding;            /* normally NULL, else the value of
                                      'encoding' attribute on cdata's tag.*/
  apr_hash_t *lock_hash;           /* the final hash returned */

  apr_pool_t *scratchpool;         /* temporary stuff goes in here */
  apr_pool_t *pool;                /* permanent stuff goes in here */

} get_locks_baton_t;



/* This implements the `svn_ra_neon__startelm_cb_t' prototype. */
static svn_error_t *
getlocks_start_element(int *elem, void *userdata, int parent_state,
                       const char *ns, const char *ln, const char **atts)
{
  get_locks_baton_t *baton = userdata;
  const svn_ra_neon__xml_elm_t *elm;

  elm = svn_ra_neon__lookup_xml_elem(getlocks_report_elements, ns, ln);

  /* Just skip unknown elements. */
  if (!elm)
    {
      *elem = NE_XML_DECLINE;
      return SVN_NO_ERROR;
    }

  if (elm->id == ELEM_lock)
    {
      if (parent_state != ELEM_get_locks_report)
        return UNEXPECTED_ELEMENT(ns, ln);
      else
        /* allocate a new svn_lock_t in the permanent pool */
        baton->current_lock = svn_lock_create(baton->pool);
    }

  else if (elm->id == ELEM_lock_path
           || elm->id == ELEM_lock_token
           || elm->id == ELEM_lock_owner
           || elm->id == ELEM_lock_comment
           || elm->id == ELEM_lock_creationdate
           || elm->id == ELEM_lock_expirationdate)
    {
      const char *encoding;

      if (parent_state != ELEM_lock)
        return UNEXPECTED_ELEMENT(ns, ln);

      /* look for any incoming encodings on these elements. */
      encoding = svn_xml_get_attr_value("encoding", atts);
      if (encoding)
        baton->encoding = apr_pstrdup(baton->scratchpool, encoding);
    }

  *elem = elm->id;

  return SVN_NO_ERROR;
}


/* This implements the `svn_ra_svn__cdata_cb_t' prototype. */
static svn_error_t *
getlocks_cdata_handler(void *userdata, int state,
                       const char *cdata, size_t len)
{
  get_locks_baton_t *baton = userdata;

  switch(state)
    {
    case ELEM_lock_path:
    case ELEM_lock_token:
    case ELEM_lock_owner:
    case ELEM_lock_comment:
    case ELEM_lock_creationdate:
    case ELEM_lock_expirationdate:
      /* accumulate cdata in the scratchpool. */
      svn_stringbuf_appendbytes(baton->cdata_accum, cdata, len);
      break;
    }

  return SVN_NO_ERROR;
}



/* This implements the `svn_ra_neon__endelm_cb_t' prototype. */
static svn_error_t *
getlocks_end_element(void *userdata, int state,
                     const char *ns, const char *ln)
{
  get_locks_baton_t *baton = userdata;
  const svn_ra_neon__xml_elm_t *elm;

  elm = svn_ra_neon__lookup_xml_elem(getlocks_report_elements, ns, ln);

  /* Just skip unknown elements. */
  if (elm == NULL)
    return SVN_NO_ERROR;

  switch (elm->id)
    {
    case ELEM_lock:
      /* is the final svn_lock_t valid?  all fields must be present
         except for 'comment' and 'expiration_date'. */
      if ((! baton->current_lock->path)
          || (! baton->current_lock->token)
          || (! baton->current_lock->owner)
          || (! baton->current_lock->creation_date))
        SVN_ERR(svn_error_create(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                                 _("Incomplete lock data returned")));

      /* Filter out unwanted paths.  Since Subversion only allows
         locks on files, we can treat depth=immediates the same as
         depth=files for filtering purposes.  Meaning, we'll keep
         this lock if:

         a) its path is the very path we queried, or
         b) we've asked for a fully recursive answer, or
         c) we've asked for depth=files or depth=immediates, and this
            lock is on an immediate child of our query path.
      */
      if ((strcmp(baton->path, baton->current_lock->path) == 0)
          || (baton->requested_depth == svn_depth_infinity))
        {
          apr_hash_set(baton->lock_hash, baton->current_lock->path,
                       APR_HASH_KEY_STRING, baton->current_lock);
        }
      else if ((baton->requested_depth == svn_depth_files) ||
               (baton->requested_depth == svn_depth_immediates))
        {
          const char *rel_uri = svn_fspath__is_child(baton->path,
                                                     baton->current_lock->path,
                                                     baton->scratchpool);
          if (rel_uri && (svn_path_component_count(rel_uri) == 1))
            apr_hash_set(baton->lock_hash, baton->current_lock->path,
                         APR_HASH_KEY_STRING, baton->current_lock);
          svn_pool_clear(baton->scratchpool);
        }
      break;

    case ELEM_lock_path:
      /* neon has already xml-unescaped the cdata for us. */
      baton->current_lock->path =
        svn_fspath__canonicalize(apr_pstrmemdup(baton->scratchpool,
                                                baton->cdata_accum->data,
                                                baton->cdata_accum->len),
                                 baton->pool);

      /* clean up the accumulator. */
      svn_stringbuf_setempty(baton->cdata_accum);
      svn_pool_clear(baton->scratchpool);
      break;

    case ELEM_lock_token:
      /* neon has already xml-unescaped the cdata for us. */
      baton->current_lock->token = apr_pstrmemdup(baton->pool,
                                                  baton->cdata_accum->data,
                                                  baton->cdata_accum->len);
      /* clean up the accumulator. */
      svn_stringbuf_setempty(baton->cdata_accum);
      svn_pool_clear(baton->scratchpool);
      break;

    case ELEM_lock_creationdate:
      SVN_ERR(svn_time_from_cstring(&(baton->current_lock->creation_date),
                                    baton->cdata_accum->data,
                                    baton->scratchpool));
      /* clean up the accumulator. */
      svn_stringbuf_setempty(baton->cdata_accum);
      svn_pool_clear(baton->scratchpool);
      break;

    case ELEM_lock_expirationdate:
      SVN_ERR(svn_time_from_cstring(&(baton->current_lock->expiration_date),
                                    baton->cdata_accum->data,
                                    baton->scratchpool));
      /* clean up the accumulator. */
      svn_stringbuf_setempty(baton->cdata_accum);
      svn_pool_clear(baton->scratchpool);
      break;

    case ELEM_lock_owner:
    case ELEM_lock_comment:
      {
        const char *final_val;

        if (baton->encoding)
          {
            /* Possibly recognize other encodings someday. */
            if (strcmp(baton->encoding, "base64") == 0)
              {
                svn_string_t *encoded_val;
                const svn_string_t *decoded_val;

                encoded_val = svn_string_create_from_buf(baton->cdata_accum,
                                                         baton->scratchpool);
                decoded_val = svn_base64_decode_string(encoded_val,
                                                       baton->scratchpool);
                final_val = decoded_val->data;
              }
            else
              return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA,
                                       NULL,
                                       _("Got unrecognized encoding '%s'"),
                                       baton->encoding);

            baton->encoding = NULL;
          }
        else
          {
            /* neon has already xml-unescaped the cdata for us. */
            final_val = baton->cdata_accum->data;
          }

        if (elm->id == ELEM_lock_owner)
          baton->current_lock->owner = apr_pstrdup(baton->pool, final_val);
        if (elm->id == ELEM_lock_comment)
          baton->current_lock->comment = apr_pstrdup(baton->pool, final_val);

        /* clean up the accumulator. */
        svn_stringbuf_setempty(baton->cdata_accum);
        svn_pool_clear(baton->scratchpool);
        break;
      }


    default:
      break;
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_ra_neon__get_locks(svn_ra_session_t *session,
                       apr_hash_t **locks,
                       const char *path,
                       svn_depth_t depth,
                       apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  const char *body, *url, *rel_path;
  svn_error_t *err;
  int status_code = 0;
  get_locks_baton_t baton;

  /* We always run the report on the 'public' URL, which represents
     HEAD anyway.  If the path doesn't exist in HEAD, then there can't
     possibly be a lock, so we just return no locks. */
  url = svn_path_url_add_component2(ras->url->data, path, pool);

  SVN_ERR(svn_ra_neon__get_path_relative_to_root(session, &rel_path,
                                                 url, pool));

  baton.lock_hash = apr_hash_make(pool);
  baton.path = svn_fspath__canonicalize(rel_path, pool);
  baton.requested_depth = depth;
  baton.pool = pool;
  baton.scratchpool = svn_pool_create(pool);
  baton.current_lock = NULL;
  baton.encoding = NULL;
  baton.cdata_accum = svn_stringbuf_create("", pool);

  body = apr_psprintf(pool,
                      "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                      "<S:get-locks-report xmlns:S=\"" SVN_XML_NAMESPACE "\" "
                      "xmlns:D=\"DAV:\" depth=\"%s\">"
                      "</S:get-locks-report>",
                      svn_depth_to_word(depth));

  err = svn_ra_neon__parsed_request(ras, "REPORT", url,
                                    body, NULL, NULL,
                                    getlocks_start_element,
                                    getlocks_cdata_handler,
                                    getlocks_end_element,
                                    &baton,
                                    NULL, /* extra headers */
                                    &status_code,
                                    FALSE,
                                    pool);

  svn_pool_destroy(baton.scratchpool);

  if (err && err->apr_err == SVN_ERR_FS_NOT_FOUND)
    {
      svn_error_clear(err);
      *locks = baton.lock_hash;
      return SVN_NO_ERROR;
    }

  /* ### Should svn_ra_neon__parsed_request() take care of storing auth
     ### info itself? */
  err = svn_ra_neon__maybe_store_auth_info_after_result(err, ras, pool);

  /* Map status 501: Method Not Implemented to our not implemented error.
     1.0.x servers and older don't support this report. */
  if (status_code == 501)
    return svn_error_create(SVN_ERR_RA_NOT_IMPLEMENTED, err,
                            _("Server does not support locking features"));

  if (err && err->apr_err == SVN_ERR_UNSUPPORTED_FEATURE)
    return svn_error_create(SVN_ERR_RA_NOT_IMPLEMENTED, err,
                            _("Server does not support locking features"));

  else if (err)
    return err;

  *locks = baton.lock_hash;
  return SVN_NO_ERROR;
}
