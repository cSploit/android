/*
 * commit.c :  routines for committing changes to the server
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



#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_uuid.h>

#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include <apr_want.h>

#include "svn_pools.h"
#include "svn_error.h"
#include "svn_delta.h"
#include "svn_io.h"
#include "svn_ra.h"
#include "../libsvn_ra/ra_loader.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_xml.h"
#include "svn_dav.h"
#include "svn_props.h"

#include "svn_private_config.h"

#include "ra_neon.h"


#define APPLY_TO_VERSION "<D:apply-to-version/>"

/*
 * version_rsrc_t: identify the relevant pieces of a resource on the server
 *
 * NOTE:  If you tweak this structure, please update dup_resource() to
 * ensure that it continues to create complete deep copies!
 */
typedef struct version_rsrc_t
{
  svn_revnum_t revision;  /* resource's revision, or SVN_INVALID_REVNUM
                             if it's new or is the HEAD */
  const char *url;        /* public/viewable/original resource URL */
  const char *vsn_url;    /* version resource URL that we stored
                             locally; NULL if this is a just-added resource */
  const char *wr_url;     /* working resource URL for this resource;
                             NULL for resources not (yet) checked out */
  const char *name;       /* basename of the resource */
  apr_pool_t *pool;       /* pool in which this resource is allocated */

} version_rsrc_t;


typedef struct commit_ctx_t
{
  /* Pool for the whole of the commit context. */
  apr_pool_t *pool;

  /* Pointer to the RA session baton. */
  svn_ra_neon__session_t *ras;

  /* Commit anchor repository relpath. */
  const char *anchor_relpath;

  /* A hash of revision properties (log messages, etc.) we need to set
     on the commit transaction. */
  apr_hash_t *revprop_table;

  /* A hash mapping svn_string_t * paths (those which are valid as
     target in the MERGE response) to svn_node_kind_t kinds. */
  apr_hash_t *valid_targets;

  /* The (potential) author of this commit. */
  const char *user;

  /* The commit callback and baton */
  svn_commit_callback2_t callback;
  void *callback_baton;

  /* The hash of lock tokens owned by the working copy, and whether
     or not to keep them after this commit is complete. */
  apr_hash_t *lock_tokens;
  svn_boolean_t keep_locks;

  /* HTTP v2 stuff */
  const char *txn_url;           /* txn URL (!svn/txn/TXN_NAME) */
  const char *txn_root_url;      /* commit anchor txn root URL */

  /* HTTP v1 stuff (only valid when 'txn_url' is NULL) */
  const char *activity_url;      /* activity base URL... */

} commit_ctx_t;


/* Are we using HTTPv2 semantics for this commit? */
#define USING_HTTPV2_COMMIT_SUPPORT(commit_ctx) ((commit_ctx)->txn_url != NULL)


typedef struct put_baton_t
{
  apr_file_t *tmpfile;        /* may be NULL for content-less file */
  svn_stringbuf_t *fname;     /* may be NULL for content-less file */
  const char *base_checksum;  /* hex md5 of base text; may be null */
  apr_off_t progress;
  svn_ra_neon__session_t *ras;
  apr_pool_t *pool;
} put_baton_t;

typedef struct resource_baton_t
{
  commit_ctx_t *cc;
  version_rsrc_t *rsrc;
  svn_revnum_t base_revision; /* base revision */
  apr_hash_t *prop_changes; /* name/values pairs of new/changed properties. */
  apr_array_header_t *prop_deletes; /* names of properties to delete. */
  svn_boolean_t created; /* set if this is an add rather than an update */
  svn_boolean_t copied; /* set if this object was copied */
  apr_pool_t *pool; /* the pool from open_foo() / add_foo() */
  put_baton_t *put_baton;  /* baton for this file's PUT request */
  const char *token;       /* file's lock token, if available */
  const char *txn_root_url; /* URL under !svn/txr/ (HTTPv2 only) */
  const char *local_relpath; /* path relative to the root of the commit
                                (used for the get_wc_prop() and push_wc_prop()
                                callbacks). */
} resource_baton_t;

/* this property will be fetched from the server when we don't find it
   cached in the WC property store. */
static const ne_propname fetch_props[] =
{
  { "DAV:", "checked-in" },
  { NULL }
};

static const ne_propname log_message_prop = { SVN_DAV_PROP_NS_SVN, "log" };

/* Return a deep copy of BASE allocated from POOL. */
static version_rsrc_t * dup_resource(version_rsrc_t *base, apr_pool_t *pool)
{
  version_rsrc_t *rsrc = apr_pcalloc(pool, sizeof(*rsrc));
  rsrc->revision = base->revision;
  rsrc->url = base->url ?
    apr_pstrdup(pool, base->url) : NULL;
  rsrc->vsn_url = base->vsn_url ?
    apr_pstrdup(pool, base->vsn_url) : NULL;
  rsrc->wr_url = base->wr_url ?
    apr_pstrdup(pool, base->wr_url) : NULL;
  rsrc->name = base->name ?
    apr_pstrdup(pool, base->name) : NULL;
  rsrc->pool = pool;
  return rsrc;
}

/* Get the version resource URL for RSRC, storing it in
   RSRC->vsn_url.  Use POOL for all temporary allocations. */
static svn_error_t * get_version_url(commit_ctx_t *cc,
                                     const char *local_relpath,
                                     const version_rsrc_t *parent,
                                     version_rsrc_t *rsrc,
                                     svn_boolean_t force,
                                     apr_pool_t *pool)
{
  svn_ra_neon__resource_t *propres;
  const char *url;
  const svn_string_t *url_str;

  if (!force)
    {
      if (cc->ras->callbacks->get_wc_prop != NULL)
        {
          const svn_string_t *vsn_url_value;

          SVN_ERR(cc->ras->callbacks->get_wc_prop(cc->ras->callback_baton,
                                                  local_relpath,
                                                  SVN_RA_NEON__LP_VSN_URL,
                                                  &vsn_url_value,
                                                  pool));
          if (vsn_url_value != NULL)
            {
              rsrc->vsn_url = apr_pstrdup(rsrc->pool, vsn_url_value->data);
              return SVN_NO_ERROR;
            }
        }

      /* If we know the version resource URL of the parent and it is
         the same revision as RSRC, use that as a base to calculate
         the version resource URL of RSRC. */
      if (parent && parent->vsn_url && parent->revision == rsrc->revision)
        {
          rsrc->vsn_url = svn_path_url_add_component2(parent->vsn_url,
                                                      rsrc->name,
                                                      rsrc->pool);
          return SVN_NO_ERROR;
        }

      /* whoops. it wasn't there. go grab it from the server. */
    }

  if (rsrc->revision == SVN_INVALID_REVNUM)
    {
      /* We aren't trying to get a specific version -- use the HEAD. We
         fetch the version URL from the public URL. */
      url = rsrc->url;
    }
  else
    {
      const char *bc_url;
      const char *bc_relative;

      /* The version URL comes from a resource in the Baseline Collection. */
      SVN_ERR(svn_ra_neon__get_baseline_info(&bc_url, &bc_relative, NULL,
                                             cc->ras, rsrc->url,
                                             rsrc->revision, pool));
      url = svn_path_url_add_component2(bc_url, bc_relative, pool);
    }

  /* Get the DAV:checked-in property, which contains the URL of the
     Version Resource */
  SVN_ERR(svn_ra_neon__get_props_resource(&propres, cc->ras, url,
                                          NULL, fetch_props, pool));
  url_str = apr_hash_get(propres->propset,
                         SVN_RA_NEON__PROP_CHECKED_IN,
                         APR_HASH_KEY_STRING);
  if (url_str == NULL)
    {
      /* ### need a proper SVN_ERR here */
      return svn_error_create(APR_EGENERAL, NULL,
                              _("Could not fetch the Version Resource URL "
                                "(needed during an import or when it is "
                                "missing from the local, cached props)"));
    }

  /* ensure we get the proper lifetime for this URL since it is going into
     a resource object. */
  rsrc->vsn_url = apr_pstrdup(rsrc->pool, url_str->data);

  if (cc->ras->callbacks->push_wc_prop != NULL)
    {
      /* Now we can store the new version-url. */
      SVN_ERR(cc->ras->callbacks->push_wc_prop(cc->ras->callback_baton,
                                               local_relpath,
                                               SVN_RA_NEON__LP_VSN_URL,
                                               url_str,
                                               pool));
    }

  return SVN_NO_ERROR;
}

/* When FORCE is true, then we force a query to the server, ignoring any
   cached property. */
static svn_error_t * get_activity_collection(commit_ctx_t *cc,
                                             const svn_string_t **collection,
                                             svn_boolean_t force,
                                             apr_pool_t *pool)
{
  if (!force && cc->ras->callbacks->get_wc_prop != NULL)
    {
      /* with a get_wc_prop, we can just ask for the activity URL from the
         property store. */

      /* get the URL where we should create activities */
      SVN_ERR(cc->ras->callbacks->get_wc_prop(cc->ras->callback_baton,
                                              "",
                                              SVN_RA_NEON__LP_ACTIVITY_COLL,
                                              collection,
                                              pool));

      if (*collection != NULL)
        {
          /* the property was there. return it. */
          return SVN_NO_ERROR;
        }

      /* property not found for some reason. get it from the server. */
    }

  /* use our utility function to fetch the activity URL */
  SVN_ERR(svn_ra_neon__get_activity_collection(collection,
                                               cc->ras,
                                               pool));

  if (cc->ras->callbacks->push_wc_prop != NULL)
    {
      /* save the (new) activity collection URL into the directory */
      SVN_ERR(cc->ras->callbacks->push_wc_prop(cc->ras->callback_baton,
                                               "",
                                               SVN_RA_NEON__LP_ACTIVITY_COLL,
                                               *collection,
                                               pool));
    }

  return SVN_NO_ERROR;
}

static svn_error_t * create_activity(commit_ctx_t *cc,
                                     apr_pool_t *pool)
{
  const svn_string_t * activity_collection;
  const char *uuid_buf = svn_uuid_generate(pool);
  int code;
  const char *url;

  /* get the URL where we'll create activities, construct the URL for
     the activity, and create the activity.  The URL for our activity
     will be ACTIVITY_COLL/UUID */
  SVN_ERR(get_activity_collection(cc, &activity_collection, FALSE, pool));
  url = svn_path_url_add_component2(activity_collection->data,
                                    uuid_buf, pool);
  SVN_ERR(svn_ra_neon__simple_request(&code, cc->ras,
                                      "MKACTIVITY", url, NULL, NULL,
                                      201 /* Created */,
                                      404 /* Not Found */, pool));

  /* if we get a 404, then it generally means that the cached activity
     collection no longer exists. Retry the sequence, but force a query
     to the server for the activity collection. */
  if (code == 404)
    {
      SVN_ERR(get_activity_collection(cc, &activity_collection, TRUE, pool));
      url = svn_path_url_add_component2(activity_collection->data,
                                        uuid_buf, pool);
      SVN_ERR(svn_ra_neon__simple_request(&code, cc->ras,
                                          "MKACTIVITY", url, NULL, NULL,
                                          201, 0, pool));
    }

  cc->activity_url = apr_pstrdup(cc->pool, url);

  return SVN_NO_ERROR;
}

/* Add a child resource.  POOL should be as "temporary" as possible,
   but probably not as far as requiring a new temp pool. */
static svn_error_t * add_child(version_rsrc_t **child,
                               commit_ctx_t *cc,
                               const version_rsrc_t *parent,
                               const char *parent_local_relpath,
                               const char *name,
                               int created,
                               svn_revnum_t revision,
                               apr_pool_t *pool)
{
  version_rsrc_t *rsrc;

  /* ### todo:  This from Yoshiki Hayashi <yoshiki@xemacs.org>:

     Probably created flag in add_child can be removed because
        revision is valid => created is false
        revision is invalid => created is true
  */

  rsrc = apr_pcalloc(pool, sizeof(*rsrc));
  rsrc->pool = pool;
  rsrc->revision = revision;
  rsrc->name = name;
  rsrc->url = svn_path_url_add_component2(parent->url, name, pool);

  /* Case 1:  the resource is truly "new".  Either it was added as a
     completely new object, or implicitly created via a COPY.  Either
     way, it has no VR URL anywhere.  However, we *can* derive its WR
     URL by the rules of deltaV:  "copy structure is preserved below
     the WR you COPY to."  */
  if (created || (parent->vsn_url == NULL))
    {
      rsrc->wr_url = svn_path_url_add_component2(parent->wr_url, name, pool);
    }
  /* Case 2: the resource is already under version-control somewhere.
     This means it has a VR URL already, and the WR URL won't exist
     until it's "checked out". */
  else
    {
      SVN_ERR(get_version_url(cc, svn_relpath_join(parent_local_relpath,
                                                   name, pool),
                              parent, rsrc, FALSE, pool));
    }

  *child = rsrc;
  return SVN_NO_ERROR;
}


static svn_error_t * do_checkout(commit_ctx_t *cc,
                                 const char *vsn_url,
                                 svn_boolean_t allow_404,
                                 const char *token,
                                 svn_boolean_t is_vcc,
                                 int *code,
                                 const char **locn,
                                 apr_pool_t *pool)
{
  svn_ra_neon__request_t *request;
  const char *body;
  apr_hash_t *extra_headers = NULL;
  svn_error_t *err = SVN_NO_ERROR;

  /* assert: vsn_url != NULL */

  /* ### send a CHECKOUT resource on vsn_url; include cc->activity_url;
     ### place result into res->wr_url and return it */

  /* create/prep the request */
  SVN_ERR(svn_ra_neon__request_create(&request, cc->ras, "CHECKOUT", vsn_url,
                                      pool));

  /* ### store this into cc to avoid pool growth */
  body = apr_psprintf(request->pool,
                      "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                      "<D:checkout xmlns:D=\"DAV:\">"
                      "<D:activity-set>"
                      "<D:href>%s</D:href>"
                      "</D:activity-set>%s</D:checkout>",
                      cc->activity_url,
                      is_vcc ? APPLY_TO_VERSION: "");

  if (token)
    {
      extra_headers = apr_hash_make(request->pool);
      svn_ra_neon__set_header(extra_headers, "If",
                              apr_psprintf(request->pool, "(<%s>)", token));
    }

  /* run the request and get the resulting status code (and svn_error_t) */
  err = svn_ra_neon__request_dispatch(code, request, extra_headers, body,
                                      201 /* Created */,
                                      allow_404 ? 404 /* Not Found */ : 0,
                                      pool);
  if (err)
    goto cleanup;

  if (allow_404 && *code == 404 && request->err)
    {
      svn_error_clear(request->err);
      request->err = SVN_NO_ERROR;
    }

  *locn = svn_ra_neon__request_get_location(request, pool);

 cleanup:
  svn_ra_neon__request_destroy(request);

  return err;
}


static svn_error_t * checkout_resource(commit_ctx_t *cc,
                                       const char *local_relpath,
                                       version_rsrc_t *rsrc,
                                       svn_boolean_t allow_404,
                                       const char *token,
                                       svn_boolean_t is_vcc,
                                       apr_pool_t *pool)
{
  int code;
  const char *locn = NULL;
  ne_uri parse;
  svn_error_t *err;

  if (rsrc->wr_url != NULL)
    {
      /* already checked out! */
      return NULL;
    }

  /* check out the Version Resource */
  err = do_checkout(cc, rsrc->vsn_url, allow_404, token,
                    is_vcc, &code, &locn, pool);

  /* possibly run the request again, with a re-fetched Version Resource */
  if (err == NULL && allow_404 && code == 404)
    {
      locn = NULL;

      /* re-fetch, forcing a query to the server */
      SVN_ERR(get_version_url(cc, local_relpath, NULL, rsrc, TRUE, pool));

      /* do it again, but don't allow a 404 this time */
      err = do_checkout(cc, rsrc->vsn_url, FALSE, token,
                        is_vcc, &code, &locn, pool);
    }

  /* special-case when conflicts occur */
  if (err)
    {
      /* ### TODO: it's a shame we don't have the full path from the
         ### root of the drive here, nor the type of the resource.
         ### Because we lack this information, the error message is
         ### overly generic.  See issue #2740. */
      if (err->apr_err == SVN_ERR_FS_CONFLICT)
        return svn_error_createf
          (SVN_ERR_FS_OUT_OF_DATE, err,
           _("File or directory '%s' is out of date; try updating"),
           local_relpath);
      return err;
    }

  /* we got the header, right? */
  if (locn == NULL)
    return svn_error_create(SVN_ERR_RA_DAV_REQUEST_FAILED, NULL,
                            _("The CHECKOUT response did not contain a "
                              "'Location:' header"));

  /* The location is an absolute URI. We want just the path portion. */
  /* ### what to do with the rest? what if it points somewhere other
     ### than the current session? */
  if (ne_uri_parse(locn, &parse) != 0)
    {
      ne_uri_free(&parse);
      return svn_error_createf(SVN_ERR_RA_DAV_MALFORMED_DATA, NULL,
                               _("Unable to parse URL '%s'"), locn);
    }

  rsrc->wr_url = apr_pstrdup(rsrc->pool, parse.path);
  ne_uri_free(&parse);

  return SVN_NO_ERROR;
}

static void record_prop_change(apr_pool_t *pool,
                               resource_baton_t *r,
                               const char *name,
                               const svn_string_t *value)
{
  /* copy the name into the pool so we get the right lifetime (who knows
     what the caller will do with it) */
  name = apr_pstrdup(pool, name);

  if (value)
    {
      /* changed/new property */
      if (r->prop_changes == NULL)
        r->prop_changes = apr_hash_make(pool);
      apr_hash_set(r->prop_changes, name, APR_HASH_KEY_STRING,
                   svn_string_dup(value, pool));
    }
  else
    {
      /* deleted property. */
      if (r->prop_deletes == NULL)
        r->prop_deletes = apr_array_make(pool, 5, sizeof(char *));
      APR_ARRAY_PUSH(r->prop_deletes, const char *) = name;
    }
}

/* Send a Neon COPY request to the location identified by
   COPYFROM_PATH and COPYFROM_REVISION, using COPY_DST_URL as the
   "Destination" of that copy. */
static svn_error_t * copy_resource(svn_ra_neon__session_t *ras,
                                   const char *copyfrom_path,
                                   svn_revnum_t copyfrom_revision,
                                   const char *copy_dst_url,
                                   apr_pool_t *scratch_pool)
{
  const char *bc_url;
  const char *bc_relative;
  const char *copy_src_url;

  SVN_ERR(svn_ra_neon__get_baseline_info(&bc_url, &bc_relative, NULL,
                                         ras, copyfrom_path,
                                         copyfrom_revision, scratch_pool));
  copy_src_url = svn_path_url_add_component2(bc_url, bc_relative,
                                             scratch_pool);
  SVN_ERR(svn_ra_neon__copy(ras, 1 /* overwrite */, SVN_RA_NEON__DEPTH_INFINITE,
                            copy_src_url, copy_dst_url, scratch_pool));

  return SVN_NO_ERROR;
}


static svn_error_t * do_proppatch(resource_baton_t *rb,
                                  apr_pool_t *pool)
{
  apr_hash_t *extra_headers = apr_hash_make(pool);
  const char *proppatch_target =
    USING_HTTPV2_COMMIT_SUPPORT(rb->cc) ? rb->txn_root_url : rb->rsrc->wr_url;

  if (SVN_IS_VALID_REVNUM(rb->base_revision))
    svn_ra_neon__set_header(extra_headers, SVN_DAV_VERSION_NAME_HEADER,
                            apr_psprintf(pool, "%ld", rb->base_revision));

  if (rb->token)
    apr_hash_set(extra_headers, "If", APR_HASH_KEY_STRING,
                 apr_psprintf(pool, "(<%s>)", rb->token));

  return svn_error_trace(svn_ra_neon__do_proppatch(rb->cc->ras,
                                                   proppatch_target,
                                                   rb->prop_changes,
                                                   rb->prop_deletes,
                                                   NULL, extra_headers,
                                                   pool));
}


static void
add_valid_target(commit_ctx_t *cc,
                 const char *path,
                 enum svn_recurse_kind kind)
{
  apr_hash_t *hash = cc->valid_targets;
  svn_string_t *path_str = svn_string_create(path, apr_hash_pool_get(hash));
  apr_hash_set(hash, path_str->data, path_str->len, (void*)kind);
}


/* Helper func for commit_delete_entry.  Find all keys in LOCK_TOKENS
   which are children of DIR.  Returns the keys (and their vals) in
   CHILD_TOKENS.   No keys or values are reallocated or dup'd.  If no
   keys are children, then return an empty hash.  Use POOL to allocate
   new hash. */
static apr_hash_t *get_child_tokens(apr_hash_t *lock_tokens,
                                    const char *dir,
                                    apr_pool_t *pool)
{
  apr_hash_index_t *hi;
  apr_hash_t *tokens = apr_hash_make(pool);
  apr_pool_t *subpool = svn_pool_create(pool);

  for (hi = apr_hash_first(pool, lock_tokens); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      apr_ssize_t klen;
      void *val;

      svn_pool_clear(subpool);
      apr_hash_this(hi, &key, &klen, &val);

      if (svn_relpath__is_child(dir, key, subpool))
        apr_hash_set(tokens, key, klen, val);
    }

  svn_pool_destroy(subpool);
  return tokens;
}


/* PROPPATCH the appropriate resource in order to set the revision
   properties in CC->REVPROP_TABLE on the commit transaction.  Use
   POOL for temporary allocations.  */
static svn_error_t *
apply_revprops(commit_ctx_t *cc,
               apr_pool_t *pool)
{
  const char *proppatch_url;

  if (USING_HTTPV2_COMMIT_SUPPORT(cc))
    {
      proppatch_url = cc->txn_url;
    }
  else
    {
      const char *vcc;
      version_rsrc_t vcc_rsrc = { SVN_INVALID_REVNUM };
      svn_error_t *err = NULL;
      int retry_count = 5;

      /* Fetch the DAV:version-controlled-configuration from the
         session's URL.  */
      SVN_ERR(svn_ra_neon__get_vcc(&vcc, cc->ras, cc->ras->root.path, pool));

      do {
        svn_error_clear(err);

        vcc_rsrc.pool = pool;
        vcc_rsrc.vsn_url = vcc;

        /* To set the revision properties, we must checkout the latest
           baseline and get back a mutable "working" baseline.  */
        err = checkout_resource(cc, "", &vcc_rsrc, FALSE, NULL, TRUE, pool);

        /* There's a small chance of a race condition here, if apache
           is experiencing heavy commit concurrency or if the network
           has long latency.  It's possible that the value of HEAD
           changed between the time we fetched the latest baseline and
           the time we checkout that baseline.  If that happens,
           apache will throw us a BAD_BASELINE error (deltaV says you
           can only checkout the latest baseline).  We just ignore
           that specific error and retry a few times, asking for the
           latest baseline again. */
        if (err && err->apr_err != SVN_ERR_APMOD_BAD_BASELINE)
          return err;

      } while (err && (--retry_count > 0));

      /* Yikes, if we couldn't hold onto HEAD after a few retries, throw a
         real error.*/
      if (err)
        return err;

      proppatch_url = vcc_rsrc.wr_url;
    }

  return svn_error_trace(svn_ra_neon__do_proppatch(cc->ras, proppatch_url,
                                                   cc->revprop_table,
                                                   NULL, NULL, NULL, pool));
}


/*** Commit Editor Functions ***/

static svn_error_t * commit_open_root(void *edit_baton,
                                      svn_revnum_t base_revision,
                                      apr_pool_t *dir_pool,
                                      void **root_baton)
{
  commit_ctx_t *cc = edit_baton;
  resource_baton_t *root;
  version_rsrc_t *rsrc = NULL;

  root = apr_pcalloc(dir_pool, sizeof(*root));
  root->pool = dir_pool;
  root->cc = cc;
  root->base_revision = base_revision;
  root->created = FALSE;
  root->local_relpath = "";

  if (SVN_RA_NEON__HAVE_HTTPV2_SUPPORT(cc->ras))
    {
      /* POST to the 'me' resource to create our server-side
         transaction (and, optionally, a corresponding activity). */
      svn_ra_neon__request_t *req;
      const char *header_val;
      svn_error_t *err;

      SVN_ERR(svn_ra_neon__request_create(&req, cc->ras, "POST",
                                          cc->ras->me_resource, dir_pool));
      ne_add_request_header(req->ne_req, "Content-Type", SVN_SKEL_MIME_TYPE);

#ifdef SVN_DAV_SEND_VTXN_NAME
      /* Enable this to exercise the VTXN-NAME code based on a client
         supplied transaction name. */
      ne_add_request_header(req->ne_req, SVN_DAV_VTXN_NAME_HEADER,
                            svn_uuid_generate(dir_pool));
#endif

      err = svn_ra_neon__request_dispatch(NULL, req, NULL, "( create-txn )",
                                          201, 0, dir_pool);
      if (!err)
        {
          /* Check the response headers for either the virtual transaction
             details, or the real transaction details.  We need to have
             one or the other of those!  */
          if ((header_val = ne_get_response_header(req->ne_req,
                                                   SVN_DAV_VTXN_NAME_HEADER)))
            {
              cc->txn_url = svn_path_url_add_component2(cc->ras->vtxn_stub,
                                                        header_val, cc->pool);
              cc->txn_root_url
                = svn_path_url_add_component2(cc->ras->vtxn_root_stub,
                                              header_val, cc->pool);
            }
          else if ((header_val
                    = ne_get_response_header(req->ne_req,
                                             SVN_DAV_TXN_NAME_HEADER)))
            {
              cc->txn_url = svn_path_url_add_component2(cc->ras->txn_stub,
                                                        header_val, cc->pool);
              cc->txn_root_url
                = svn_path_url_add_component2(cc->ras->txn_root_stub,
                                              header_val, cc->pool);
            }
          else
            err = svn_error_createf(SVN_ERR_RA_DAV_REQUEST_FAILED, NULL,
                                    _("POST request did not return transaction "
                                      "information"));
        }
      svn_ra_neon__request_destroy(req);
      SVN_ERR(err);

      root->rsrc = NULL;
      root->txn_root_url = svn_path_url_add_component2(cc->txn_root_url,
                                                       cc->anchor_relpath,
                                                       dir_pool);
    }
  else
    {
      /* Use MKACTIVITY against a unique child of activity collection
         to create a server-side activity which corresponds directly
         to an FS transaction.  We will check out all further
         resources within the context of this activity. */
      SVN_ERR(create_activity(cc, dir_pool));

      /* Create the root resource.  (We don't yet know the wr_url.) */
      rsrc = apr_pcalloc(dir_pool, sizeof(*rsrc));
      rsrc->pool = dir_pool;
      rsrc->revision = base_revision;
      rsrc->url = cc->ras->root.path;

      SVN_ERR(get_version_url(cc, root->local_relpath, NULL, rsrc,
                              FALSE, dir_pool));

      root->rsrc = rsrc;
      root->txn_root_url = NULL;
    }

  /* Find the latest baseline resource, check it out, and then apply
     the log message onto the thing. */
  SVN_ERR(apply_revprops(cc, dir_pool));


  *root_baton = root;

  return SVN_NO_ERROR;
}


static svn_error_t * commit_delete_entry(const char *path,
                                         svn_revnum_t revision,
                                         void *parent_baton,
                                         apr_pool_t *pool)
{
  resource_baton_t *parent = parent_baton;
  const char *name = svn_relpath_basename(path, NULL);
  apr_hash_t *extra_headers = apr_hash_make(pool);
  int code;
  svn_error_t *serr;
  const char *delete_target;

  if (USING_HTTPV2_COMMIT_SUPPORT(parent->cc))
    {
      delete_target = svn_path_url_add_component2(parent->txn_root_url,
                                                  name, pool);
    }
  else
    {
      /* Get the URL to the working collection.  */
      SVN_ERR(checkout_resource(parent->cc, parent->local_relpath,
                                parent->rsrc, TRUE, NULL, FALSE, pool));

      /* Create the URL for the child resource. */
      delete_target = svn_path_url_add_component2(parent->rsrc->wr_url,
                                                  name, pool);
    }

  /* If we have a base revision for the server to compare against,
     pass that along in a custom header. */
  if (SVN_IS_VALID_REVNUM(revision))
    svn_ra_neon__set_header(extra_headers, SVN_DAV_VERSION_NAME_HEADER,
                            apr_psprintf(pool, "%ld", revision));

  /* We start out assuming that we're deleting a file; try to lookup
     the path itself in the token-hash, and if found, attach it to the
     If: header. */
  if (parent->cc->lock_tokens)
    {
      const char *token =
        apr_hash_get(parent->cc->lock_tokens, path, APR_HASH_KEY_STRING);

      if (token)
        {
          const char *token_header_val;
          const char *token_uri;

          token_uri = svn_path_url_add_component2(parent->cc->ras->url->data,
                                                  path, pool);
          token_header_val = apr_psprintf(pool, "<%s> (<%s>)",
                                          token_uri, token);
          extra_headers = apr_hash_make(pool);
          apr_hash_set(extra_headers, "If", APR_HASH_KEY_STRING,
                       token_header_val);
        }
    }

  /* dav_method_delete() always calls dav_unlock(), but if the svn
     client passed --no-unlock to 'svn commit', then we need to send a
     header which prevents mod_dav_svn from actually doing the unlock. */
  if (parent->cc->keep_locks)
    {
      apr_hash_set(extra_headers, SVN_DAV_OPTIONS_HEADER,
                   APR_HASH_KEY_STRING, SVN_DAV_OPTION_KEEP_LOCKS);
    }

  serr = svn_ra_neon__simple_request(&code, parent->cc->ras,
                                     "DELETE", delete_target,
                                     extra_headers, NULL,
                                     204 /* No Content */,
                                     0, pool);

  /* A locking-related error most likely means we were deleting a
     directory rather than a file, and didn't send all of the
     necessary lock-tokens within the directory. */
  if (serr && ((serr->apr_err == SVN_ERR_FS_BAD_LOCK_TOKEN)
               || (serr->apr_err == SVN_ERR_FS_NO_LOCK_TOKEN)
               || (serr->apr_err == SVN_ERR_FS_LOCK_OWNER_MISMATCH)
               || (serr->apr_err == SVN_ERR_FS_PATH_ALREADY_LOCKED)))
    {
      /* Re-attempt the DELETE request as if the path were a
         directory.  Discover all lock-tokens within the directory,
         and send them in the body of the request (which is normally
         empty).  Of course, if we don't *find* any additional
         lock-tokens, don't bother to retry (it ain't gonna do any
         good).

         Note that we're not sending the locks in the If: header, for
         the same reason we're not sending in MERGE's headers: httpd
         has limits on the amount of data it's willing to receive in
         headers. */

      apr_hash_t *child_tokens = NULL;
      svn_ra_neon__request_t *request;
      const char *body;
      const char *token;
      svn_stringbuf_t *locks_list;
      svn_error_t *err = SVN_NO_ERROR;

      if (parent->cc->lock_tokens)
        child_tokens = get_child_tokens(parent->cc->lock_tokens, path, pool);

      /* No kiddos?  Return the original error.  Else, clear it so it
         doesn't get leaked.  */
      if ((! child_tokens) || (apr_hash_count(child_tokens) == 0))
        return serr;
      else
        svn_error_clear(serr);

      /* In preparation of directory locks, go ahead and add the actual
         target's lock token to those of its children. */
      if ((token = apr_hash_get(parent->cc->lock_tokens, path,
                                APR_HASH_KEY_STRING)))
        {
          /* ### copy PATH into the right pool. which?  */
          apr_hash_set(child_tokens, path, APR_HASH_KEY_STRING, token);
        }

      SVN_ERR(svn_ra_neon__request_create(&request, parent->cc->ras, "DELETE",
                                          delete_target, pool));

      err = svn_ra_neon__assemble_locktoken_body(&locks_list,
                                                 child_tokens, request->pool);
      if (err)
        goto cleanup;

      body = apr_psprintf(request->pool,
                          "<?xml version=\"1.0\" encoding=\"utf-8\"?> %s",
                          locks_list->data);

      err = svn_ra_neon__request_dispatch(&code, request, NULL, body,
                                          204 /* Created */,
                                          404 /* Not Found */,
                                          pool);
    cleanup:
      svn_ra_neon__request_destroy(request);
      SVN_ERR(err);
    }
  else if (serr)
    return serr;

  /* Add this path to the valid targets hash. */
  add_valid_target(parent->cc, path, svn_nonrecursive);

  return SVN_NO_ERROR;
}


static svn_error_t * commit_add_dir(const char *path,
                                    void *parent_baton,
                                    const char *copyfrom_path,
                                    svn_revnum_t copyfrom_revision,
                                    apr_pool_t *dir_pool,
                                    void **child_baton)
{
  resource_baton_t *parent = parent_baton;
  resource_baton_t *child;
  int code;
  const char *name = svn_relpath_basename(path, dir_pool);
  apr_pool_t *workpool = svn_pool_create(dir_pool);
  version_rsrc_t *rsrc = NULL;
  const char *mkcol_target = NULL;

  /* create a child object that contains all the resource urls */
  child = apr_pcalloc(dir_pool, sizeof(*child));
  child->pool = dir_pool;
  child->base_revision = SVN_INVALID_REVNUM;
  child->cc = parent->cc;
  child->created = TRUE;
  child->local_relpath = svn_relpath_join(parent->local_relpath,
                                          name, dir_pool);

  if (USING_HTTPV2_COMMIT_SUPPORT(parent->cc))
    {
      child->rsrc = NULL;
      child->txn_root_url = svn_path_url_add_component2(parent->txn_root_url,
                                                        name, dir_pool);
      mkcol_target = child->txn_root_url;
    }
  else
    {
      /* check out the parent resource so that we can create the new collection
         as one of its children. */
      SVN_ERR(checkout_resource(parent->cc, parent->local_relpath,
                                parent->rsrc, TRUE, NULL, FALSE, dir_pool));
      SVN_ERR(add_child(&rsrc, parent->cc, parent->rsrc, parent->local_relpath,
                        name, 1, SVN_INVALID_REVNUM, workpool));

      child->rsrc = dup_resource(rsrc, dir_pool);
      child->txn_root_url = NULL;
      mkcol_target = child->rsrc->wr_url;
    }

  if (! copyfrom_path)
    {
      /* This a new directory with no history, so just create a new,
         empty collection */
      SVN_ERR(svn_ra_neon__simple_request(&code, parent->cc->ras, "MKCOL",
                                          mkcol_target, NULL, NULL,
                                          201 /* Created */, 0, workpool));
    }
  else
    {
      /* This add has history, so we need to do a COPY. */
      SVN_ERR(copy_resource(parent->cc->ras, copyfrom_path, copyfrom_revision,
                            mkcol_target, workpool));

      /* Remember that this object was copied. */
      child->copied = TRUE;
    }

  /* Add this path to the valid targets hash. */
  add_valid_target(parent->cc, path,
                   copyfrom_path ? svn_recursive : svn_nonrecursive);

  svn_pool_destroy(workpool);
  *child_baton = child;
  return SVN_NO_ERROR;
}

static svn_error_t * commit_open_dir(const char *path,
                                     void *parent_baton,
                                     svn_revnum_t base_revision,
                                     apr_pool_t *dir_pool,
                                     void **child_baton)
{
  resource_baton_t *parent = parent_baton;
  resource_baton_t *child = apr_pcalloc(dir_pool, sizeof(*child));
  const char *name = svn_relpath_basename(path, dir_pool);
  version_rsrc_t *rsrc = NULL;

  child->pool = dir_pool;
  child->base_revision = base_revision;
  child->cc = parent->cc;
  child->created = FALSE;
  child->local_relpath = svn_relpath_join(parent->local_relpath,
                                          name, dir_pool);

  if (USING_HTTPV2_COMMIT_SUPPORT(parent->cc))
    {
      child->rsrc = NULL;
      child->txn_root_url = svn_path_url_add_component2(parent->txn_root_url,
                                                        name, dir_pool);
    }
  else
    {
      apr_pool_t *workpool = svn_pool_create(dir_pool);

      SVN_ERR(add_child(&rsrc, parent->cc, parent->rsrc, parent->local_relpath,
                        name, 0, base_revision, workpool));
      child->rsrc = dup_resource(rsrc, dir_pool);
      child->txn_root_url = NULL;

      svn_pool_destroy(workpool);
    }

  /* We don't do any real work here -- open_dir() just sets up the
     baton for this directory for use later when we operate on one of
     its children. */
  *child_baton = child;
  return SVN_NO_ERROR;
}

static svn_error_t * commit_change_dir_prop(void *dir_baton,
                                            const char *name,
                                            const svn_string_t *value,
                                            apr_pool_t *pool)
{
  resource_baton_t *dir = dir_baton;

  /* record the change. it will be applied at close_dir time. */
  record_prop_change(dir->pool, dir, name, value);

  if (! USING_HTTPV2_COMMIT_SUPPORT(dir->cc))
    {
      /* We might as well CHECKOUT now (if we haven't already).  Why
         wait?  */
      SVN_ERR(checkout_resource(dir->cc, dir->local_relpath, dir->rsrc,
                                TRUE, NULL, FALSE, pool));
    }

    /* Add the path to the valid targets hash. */
  add_valid_target(dir->cc, dir->local_relpath, svn_nonrecursive);

  return SVN_NO_ERROR;
}

static svn_error_t * commit_close_dir(void *dir_baton,
                                      apr_pool_t *pool)
{
  resource_baton_t *dir = dir_baton;

  /* Perform all of the property changes on the directory. Note that we
     checked out the directory when the first prop change was noted. */
  return do_proppatch(dir, pool);
}

static svn_error_t * commit_add_file(const char *path,
                                     void *parent_baton,
                                     const char *copyfrom_path,
                                     svn_revnum_t copyfrom_revision,
                                     apr_pool_t *file_pool,
                                     void **file_baton)
{
  resource_baton_t *parent = parent_baton;
  resource_baton_t *file;
  const char *name = svn_relpath_basename(path, file_pool);
  apr_pool_t *workpool = svn_pool_create(file_pool);
  const char *put_target = NULL;

  /*
  ** To add a new file into the repository, we CHECKOUT the parent
  ** collection, then PUT the file as a member of the resulting working
  ** collection.
  **
  ** If the file was copied from elsewhere, then we will use the COPY
  ** method to copy into the working collection.
  */

  /* Construct a file_baton that contains all the resource urls. */
  file = apr_pcalloc(file_pool, sizeof(*file));
  file->base_revision = SVN_INVALID_REVNUM;
  file->pool = file_pool;
  file->cc = parent->cc;
  file->created = TRUE;
  file->local_relpath = svn_relpath_join(parent->local_relpath,
                                         name, file_pool);

  if (USING_HTTPV2_COMMIT_SUPPORT(parent->cc))
    {
      file->rsrc = NULL;
      file->txn_root_url = svn_path_url_add_component2(parent->txn_root_url,
                                                       name, file_pool);
      put_target = file->txn_root_url;
    }
  else
    {
      version_rsrc_t *rsrc = NULL;

      /* Do the parent CHECKOUT first */
      SVN_ERR(checkout_resource(parent->cc, parent->local_relpath, parent->rsrc,
                                TRUE, NULL, FALSE, workpool));
      SVN_ERR(add_child(&rsrc, parent->cc, parent->rsrc, parent->local_relpath,
                        name, 1, SVN_INVALID_REVNUM, workpool));

      file->rsrc = dup_resource(rsrc, file_pool);
      file->txn_root_url = NULL;
      put_target = file->rsrc->wr_url;
    }

  if (parent->cc->lock_tokens)
    file->token = apr_hash_get(parent->cc->lock_tokens, path,
                               APR_HASH_KEY_STRING);

  /* If the parent directory existed before this commit then there may
     be a file with this URL already. We need to ensure such a file
     does not exist, which we do by attempting a PROPFIND in both
     public URL (the path in HEAD) and the working URL (the path
     within the transaction), since we cannot differentiate between
     deleted items.

     ### For now, we'll assume that if this path has already been
     ### added to the valid targets hash, that addition occurred
     ### during the "delete" phase (if that's not the case, this
     ### editor is being driven incorrectly, as we should never visit
     ### the same path twice except in a delete+add situation). */
  if ((! parent->created)
      && (! apr_hash_get(file->cc->valid_targets, path, APR_HASH_KEY_STRING)))
    {
      static const ne_propname restype_props[] =
      {
        { "DAV:", "resourcetype" },
        { NULL }
      };
      svn_ra_neon__resource_t *res;
      const char *public_url;
      svn_error_t *err1, *err2;

      public_url = svn_path_url_add_component2(file->cc->ras->url->data,
                                               path, workpool);
      err1 = svn_ra_neon__get_props_resource(&res, parent->cc->ras, put_target,
                                             NULL, restype_props, workpool);
      err2 = svn_ra_neon__get_props_resource(&res, parent->cc->ras, public_url,
                                             NULL, restype_props, workpool);
      if (! err1 && ! err2)
        {
          /* If the PROPFINDs succeed the file already exists */
          return svn_error_createf(SVN_ERR_RA_DAV_ALREADY_EXISTS, NULL,
                                   _("File '%s' already exists"), path);
        }
      else if ((err1 && (err1->apr_err == SVN_ERR_FS_NOT_FOUND))
               || (err2 && (err2->apr_err == SVN_ERR_FS_NOT_FOUND)))
        {
          svn_error_clear(err1);
          svn_error_clear(err2);
        }
      else
        {
          /* A real error */
          return svn_error_compose_create(err1, err2);
        }
    }

  if (! copyfrom_path)
    {
      /* This a truly new file. */

      /* Wait for apply_txdelta() before doing a PUT.  It might arrive
         a "long time" from now -- certainly after many other
         operations -- we don't want to start a PUT just yet. */
    }
  else
    {
      /* This add has history, so we need to do a COPY. */
      SVN_ERR(copy_resource(parent->cc->ras, copyfrom_path, copyfrom_revision,
                            put_target, workpool));

      /* Remember that this object was copied. */
      file->copied = TRUE;
    }

  /* Add this path to the valid targets hash. */
  add_valid_target(parent->cc, path, svn_nonrecursive);

  svn_pool_destroy(workpool);

  /* return the file_baton */
  *file_baton = file;
  return SVN_NO_ERROR;
}

static svn_error_t * commit_open_file(const char *path,
                                      void *parent_baton,
                                      svn_revnum_t base_revision,
                                      apr_pool_t *file_pool,
                                      void **file_baton)
{
  resource_baton_t *parent = parent_baton;
  resource_baton_t *file;
  const char *name = svn_relpath_basename(path, file_pool);

  file = apr_pcalloc(file_pool, sizeof(*file));
  file->pool = file_pool;
  file->base_revision = base_revision;
  file->cc = parent->cc;
  file->created = FALSE;
  file->local_relpath = svn_relpath_join(parent->local_relpath,
                                         name, file_pool);

  if (parent->cc->lock_tokens)
    file->token = apr_hash_get(parent->cc->lock_tokens, path,
                               APR_HASH_KEY_STRING);

  if (USING_HTTPV2_COMMIT_SUPPORT(parent->cc))
    {
      file->rsrc = NULL;
      file->txn_root_url = svn_path_url_add_component2(parent->txn_root_url,
                                                       name, file_pool);
    }
  else
    {
      version_rsrc_t *rsrc = NULL;
      apr_pool_t *workpool = svn_pool_create(file_pool);

      SVN_ERR(add_child(&rsrc, parent->cc, parent->rsrc, parent->local_relpath,
                        name, 0, base_revision, workpool));
      file->rsrc = dup_resource(rsrc, file_pool);
      file->txn_root_url = NULL;

      /* Do the CHECKOUT now.  */
      SVN_ERR(checkout_resource(parent->cc, file->local_relpath, file->rsrc,
                                TRUE, file->token, FALSE, workpool));

      svn_pool_destroy(workpool);
    }

  /* Wait for apply_txdelta() before doing a PUT.  It might arrive
     a "long time" from now -- certainly after many other
     operations -- we don't want to start a PUT just yet. */

  *file_baton = file;
  return SVN_NO_ERROR;
}

static svn_error_t * commit_stream_write(void *baton,
                                         const char *data,
                                         apr_size_t *len)
{
  put_baton_t *pb = baton;
  svn_ra_neon__session_t *ras = pb->ras;
  apr_status_t status;

  if (ras->callbacks && ras->callbacks->cancel_func)
    SVN_ERR(ras->callbacks->cancel_func(ras->callback_baton));

  /* drop the data into our temp file */
  status = apr_file_write_full(pb->tmpfile, data, *len, NULL);
  if (status)
    return svn_error_wrap_apr(status,
                              _("Could not write svndiff to temp file"));

  if (ras->progress_func)
    {
      pb->progress += *len;
      ras->progress_func(pb->progress, -1, ras->progress_baton, pb->pool);
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
commit_apply_txdelta(void *file_baton,
                     const char *base_checksum,
                     apr_pool_t *pool,
                     svn_txdelta_window_handler_t *handler,
                     void **handler_baton)
{
  resource_baton_t *file = file_baton;
  put_baton_t *baton;
  svn_stream_t *stream;

  baton = apr_pcalloc(file->pool, sizeof(*baton));
  baton->ras = file->cc->ras;
  baton->pool = file->pool;
  file->put_baton = baton;

  if (base_checksum)
    baton->base_checksum = apr_pstrdup(file->pool, base_checksum);
  else
    baton->base_checksum = NULL;

  /* ### oh, hell. Neon's request body support is either text (a C string),
     ### or a FILE*. since we are getting binary data, we must use a FILE*
     ### for now. isn't that special? */
  SVN_ERR(svn_io_open_unique_file3(&baton->tmpfile, NULL, NULL,
                                   svn_io_file_del_on_pool_cleanup,
                                   file->pool, pool));

  stream = svn_stream_create(baton, pool);
  svn_stream_set_write(stream, commit_stream_write);

  svn_txdelta_to_svndiff2(handler, handler_baton, stream, 0, pool);

  /* Add this path to the valid targets hash. */
  add_valid_target(file->cc, file->local_relpath, svn_nonrecursive);

  return SVN_NO_ERROR;
}

static svn_error_t * commit_change_file_prop(void *file_baton,
                                             const char *name,
                                             const svn_string_t *value,
                                             apr_pool_t *pool)
{
  resource_baton_t *file = file_baton;

  /* Record the change.  It will be applied at close_file() time. */
  record_prop_change(file->pool, file, name, value);

  if (! USING_HTTPV2_COMMIT_SUPPORT(file->cc))
    {
      /* We might as well CHECKOUT now (if we haven't already).  Why
         wait?  */
      SVN_ERR(checkout_resource(file->cc, file->local_relpath, file->rsrc,
                                TRUE, file->token, FALSE, pool));
    }

    /* Add the path to the valid targets hash. */
  add_valid_target(file->cc, file->local_relpath, svn_nonrecursive);

  return SVN_NO_ERROR;
}

static svn_error_t * commit_close_file(void *file_baton,
                                       const char *text_checksum,
                                       apr_pool_t *pool)
{
  resource_baton_t *file = file_baton;
  commit_ctx_t *cc = file->cc;

  /* If this is a newly added file, not copied, and the editor driver
     didn't call apply_textdelta(), then we'll pretend they *did* call
     apply_textdelta() and described a zero-byte empty file. */
  if ((! file->put_baton) && file->created && (! file->copied))
    {
      /* Make a dummy put_baton, with NULL fields to indicate that
         we're dealing with a content-less (zero-byte) file. */
      file->put_baton = apr_pcalloc(file->pool, sizeof(*(file->put_baton)));
    }

  if (file->put_baton)
    {
      svn_error_t *err = SVN_NO_ERROR;
      put_baton_t *pb = file->put_baton;
      apr_hash_t *extra_headers;
      svn_ra_neon__request_t *request;
      int code;
      const char *public_url =
        svn_path_url_add_component2(file->cc->ras->url->data,
                                    file->local_relpath, pool);
      const char *put_target =
        USING_HTTPV2_COMMIT_SUPPORT(cc) ? file->txn_root_url
                                        : file->rsrc->wr_url;

      /* create/prep the request */
      SVN_ERR(svn_ra_neon__request_create(&request, cc->ras, "PUT",
                                          put_target, pool));

      extra_headers = apr_hash_make(request->pool);

      if (file->token)
        svn_ra_neon__set_header(extra_headers, "If",
                                apr_psprintf(pool, "<%s> (<%s>)",
                                             public_url, file->token));

      if (pb->base_checksum)
        svn_ra_neon__set_header(extra_headers,
                                SVN_DAV_BASE_FULLTEXT_MD5_HEADER,
                                pb->base_checksum);

      if (text_checksum)
        svn_ra_neon__set_header(extra_headers,
                                SVN_DAV_RESULT_FULLTEXT_MD5_HEADER,
                                text_checksum);

      if (SVN_IS_VALID_REVNUM(file->base_revision))
        svn_ra_neon__set_header(extra_headers,
                                SVN_DAV_VERSION_NAME_HEADER,
                                apr_psprintf(pool, "%ld", file->base_revision));

      if (pb->tmpfile)
        {
          svn_ra_neon__set_header(extra_headers, "Content-Type",
                                  SVN_SVNDIFF_MIME_TYPE);

          /* Give the file to neon. The provider will rewind the file. */
          err = svn_ra_neon__set_neon_body_provider(request, pb->tmpfile);
          if (err)
            goto cleanup;
        }
      else
        {
          ne_set_request_body_buffer(request->ne_req, "", 0);
        }

      /* run the request and get the resulting status code (and svn_error_t) */
      err = svn_ra_neon__request_dispatch(&code, request, extra_headers, NULL,
                                          201 /* Created */,
                                          204 /* No Content */,
                                          pool);

      if (err && (err->apr_err == SVN_ERR_RA_DAV_REQUEST_FAILED))
        {
          switch (code)
          {
            case 423:
               err = svn_error_createf(SVN_ERR_RA_NOT_LOCKED, err,
                                       _("No lock on path '%s'"
                                         " (Status %d on PUT Request)"),
                                       put_target, code);
            default:
              break;
          }
      }
    cleanup:
      svn_ra_neon__request_destroy(request);
      SVN_ERR(err);

      if (pb->tmpfile)
        {
          /* We're done with the file.  this should delete it. Note: it
             isn't a big deal if this line is never executed -- the pool
             will eventually get it. We're just being proactive here. */
          (void) apr_file_close(pb->tmpfile);
        }
    }

  /* Perform all of the property changes on the file. Note that we
     checked out the file when the first prop change was noted. */
  return do_proppatch(file, pool);
}


static svn_error_t * commit_close_edit(void *edit_baton,
                                       apr_pool_t *pool)
{
  commit_ctx_t *cc = edit_baton;
  svn_commit_info_t *commit_info = svn_create_commit_info(pool);
  const char *merge_resource_url =
    USING_HTTPV2_COMMIT_SUPPORT(cc) ? cc->txn_url : cc->activity_url;

  SVN_ERR(svn_ra_neon__merge_activity(&(commit_info->revision),
                                      &(commit_info->date),
                                      &(commit_info->author),
                                      &(commit_info->post_commit_err),
                                      cc->ras,
                                      cc->ras->root.path,
                                      merge_resource_url,
                                      cc->valid_targets,
                                      cc->lock_tokens,
                                      cc->keep_locks,
                                      cc->ras->callbacks->push_wc_prop == NULL,
                                      pool));

  /* DELETE any activity that might be left on the server. */
  if (cc->activity_url)
    {
      SVN_ERR(svn_ra_neon__simple_request(NULL, cc->ras, "DELETE",
                                          cc->activity_url, NULL, NULL,
                                          204 /* No Content */,
                                          404 /* Not Found */, pool));
    }

  if (cc->callback && commit_info->revision != SVN_INVALID_REVNUM)
    SVN_ERR(cc->callback(commit_info, cc->callback_baton, pool));

  return SVN_NO_ERROR;
}


static svn_error_t * commit_abort_edit(void *edit_baton,
                                       apr_pool_t *pool)
{
  commit_ctx_t *cc = edit_baton;
  const char *delete_target = NULL;

  /* If we started an activity and/or transaction, we need to (try to)
     delete it.  (If it doesn't exist, that's okay, too.) */
  if (USING_HTTPV2_COMMIT_SUPPORT(cc))
    delete_target = cc->txn_url;
  else
    delete_target = cc->activity_url;

  if (delete_target)
    SVN_ERR(svn_ra_neon__simple_request(NULL, cc->ras, "DELETE",
                                        delete_target, NULL, NULL,
                                        204 /* No Content */,
                                        404 /* Not Found */, pool));
  return SVN_NO_ERROR;
}


svn_error_t * svn_ra_neon__get_commit_editor(svn_ra_session_t *session,
                                             const svn_delta_editor_t **editor,
                                             void **edit_baton,
                                             apr_hash_t *revprop_table,
                                             svn_commit_callback2_t callback,
                                             void *callback_baton,
                                             apr_hash_t *lock_tokens,
                                             svn_boolean_t keep_locks,
                                             apr_pool_t *pool)
{
  svn_ra_neon__session_t *ras = session->priv;
  svn_delta_editor_t *commit_editor;
  commit_ctx_t *cc;
  apr_hash_index_t *hi;

  /* Build the main commit editor's baton. */
  cc = apr_pcalloc(pool, sizeof(*cc));
  cc->pool = pool;
  cc->ras = ras;
  cc->valid_targets = apr_hash_make(pool);
  cc->callback = callback;
  cc->callback_baton = callback_baton;
  cc->lock_tokens = lock_tokens;
  cc->keep_locks = keep_locks;

  /* Dup the revprops into POOL, in case the caller clears the pool
     they're in before driving the editor that this function returns. */
  cc->revprop_table = apr_hash_make(pool);
  for (hi = apr_hash_first(pool, revprop_table); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      apr_ssize_t klen;
      void *val;

      apr_hash_this(hi, &key, &klen, &val);
      apr_hash_set(cc->revprop_table, apr_pstrdup(pool, key), klen,
                   svn_string_dup(val, pool));
    }

  /* Calculate the commit anchor's repository relpath. */
  SVN_ERR(svn_ra_neon__get_path_relative_to_root(session,
                                                 &(cc->anchor_relpath),
                                                 ras->url->data, pool));

  /* Set up the editor. */
  commit_editor = svn_delta_default_editor(pool);
  commit_editor->open_root = commit_open_root;
  commit_editor->delete_entry = commit_delete_entry;
  commit_editor->add_directory = commit_add_dir;
  commit_editor->open_directory = commit_open_dir;
  commit_editor->change_dir_prop = commit_change_dir_prop;
  commit_editor->close_directory = commit_close_dir;
  commit_editor->add_file = commit_add_file;
  commit_editor->open_file = commit_open_file;
  commit_editor->apply_textdelta = commit_apply_txdelta;
  commit_editor->change_file_prop = commit_change_file_prop;
  commit_editor->close_file = commit_close_file;
  commit_editor->close_edit = commit_close_edit;
  commit_editor->abort_edit = commit_abort_edit;

  *editor = commit_editor;
  *edit_baton = cc;
  return SVN_NO_ERROR;
}
