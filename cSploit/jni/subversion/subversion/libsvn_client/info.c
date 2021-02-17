/*
 * info.c:  return system-generated metadata about paths or URLs.
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

/* ==================================================================== */



#include "client.h"
#include "svn_client.h"
#include "svn_pools.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_hash.h"
#include "svn_wc.h"

#include "svn_private_config.h"
#include "private/svn_fspath.h"
#include "private/svn_wc_private.h"


svn_client_info2_t *
svn_client_info2_dup(const svn_client_info2_t *info,
                     apr_pool_t *pool)
{
  svn_client_info2_t *new_info = apr_pmemdup(pool, info, sizeof(*new_info));

  if (new_info->URL)
    new_info->URL = apr_pstrdup(pool, info->URL);
  if (new_info->repos_root_URL)
    new_info->repos_root_URL = apr_pstrdup(pool, info->repos_root_URL);
  if (new_info->repos_UUID)
    new_info->repos_UUID = apr_pstrdup(pool, info->repos_UUID);
  if (info->last_changed_author)
    new_info->last_changed_author = apr_pstrdup(pool, info->last_changed_author);
  if (new_info->lock)
    new_info->lock = svn_lock_dup(info->lock, pool);
  if (new_info->wc_info)
    new_info->wc_info = svn_wc_info_dup(info->wc_info, pool);
  return new_info;
}

/* Set *INFO to a new info struct built from DIRENT
   and (possibly NULL) svn_lock_t LOCK, all allocated in POOL.
   Pointer fields are copied by reference, not dup'd. */
static svn_error_t *
build_info_from_dirent(svn_client_info2_t **info,
                       const svn_dirent_t *dirent,
                       svn_lock_t *lock,
                       const char *URL,
                       svn_revnum_t revision,
                       const char *repos_UUID,
                       const char *repos_root,
                       apr_pool_t *pool)
{
  svn_client_info2_t *tmpinfo = apr_pcalloc(pool, sizeof(*tmpinfo));

  tmpinfo->URL                  = URL;
  tmpinfo->rev                  = revision;
  tmpinfo->kind                 = dirent->kind;
  tmpinfo->repos_UUID           = repos_UUID;
  tmpinfo->repos_root_URL       = repos_root;
  tmpinfo->last_changed_rev     = dirent->created_rev;
  tmpinfo->last_changed_date    = dirent->time;
  tmpinfo->last_changed_author  = dirent->last_author;
  tmpinfo->lock                 = lock;
  tmpinfo->size                 = dirent->size;

  tmpinfo->wc_info              = NULL;

  *info = tmpinfo;
  return SVN_NO_ERROR;
}


/* The dirent fields we care about for our calls to svn_ra_get_dir2. */
#define DIRENT_FIELDS (SVN_DIRENT_KIND        | \
                       SVN_DIRENT_CREATED_REV | \
                       SVN_DIRENT_TIME        | \
                       SVN_DIRENT_LAST_AUTHOR)


/* Helper func for recursively fetching svn_dirent_t's from a remote
   directory and pushing them at an info-receiver callback.

   DEPTH is the depth starting at DIR, even though RECEIVER is never
   invoked on DIR: if DEPTH is svn_depth_immediates, then invoke
   RECEIVER on all children of DIR, but none of their children; if
   svn_depth_files, then invoke RECEIVER on file children of DIR but
   not on subdirectories; if svn_depth_infinity, recurse fully.
   DIR is a relpath, relative to the root of RA_SESSION.
*/
static svn_error_t *
push_dir_info(svn_ra_session_t *ra_session,
              const char *session_URL,
              const char *dir,
              svn_revnum_t rev,
              const char *repos_UUID,
              const char *repos_root,
              svn_client_info_receiver2_t receiver,
              void *receiver_baton,
              svn_depth_t depth,
              svn_client_ctx_t *ctx,
              apr_hash_t *locks,
              apr_pool_t *pool)
{
  apr_hash_t *tmpdirents;
  apr_hash_index_t *hi;
  apr_pool_t *subpool = svn_pool_create(pool);

  SVN_ERR(svn_ra_get_dir2(ra_session, &tmpdirents, NULL, NULL,
                          dir, rev, DIRENT_FIELDS, pool));

  for (hi = apr_hash_first(pool, tmpdirents); hi; hi = apr_hash_next(hi))
    {
      const char *path, *URL, *fs_path;
      svn_lock_t *lock;
      svn_client_info2_t *info;
      const char *name = svn__apr_hash_index_key(hi);
      svn_dirent_t *the_ent = svn__apr_hash_index_val(hi);

      svn_pool_clear(subpool);

      if (ctx->cancel_func)
        SVN_ERR(ctx->cancel_func(ctx->cancel_baton));

      path = svn_relpath_join(dir, name, subpool);
      URL = svn_path_url_add_component2(session_URL, name, subpool);
      fs_path = svn_fspath__canonicalize(svn_uri__is_child(repos_root, URL,
                                                           subpool), subpool);

      lock = apr_hash_get(locks, fs_path, APR_HASH_KEY_STRING);

      SVN_ERR(build_info_from_dirent(&info, the_ent, lock, URL, rev,
                                     repos_UUID, repos_root, subpool));

      if (depth >= svn_depth_immediates
          || (depth == svn_depth_files && the_ent->kind == svn_node_file))
        {
          SVN_ERR(receiver(receiver_baton, path, info, subpool));
        }

      if (depth == svn_depth_infinity && the_ent->kind == svn_node_dir)
        {
          SVN_ERR(push_dir_info(ra_session, URL, path,
                                rev, repos_UUID, repos_root,
                                receiver, receiver_baton,
                                depth, ctx, locks, subpool));
        }
    }

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


/* Set *SAME_P to TRUE if URL exists in the head of the repository and
   refers to the same resource as it does in REV, using POOL for
   temporary allocations.  RA_SESSION is an open RA session for URL.  */
static svn_error_t *
same_resource_in_head(svn_boolean_t *same_p,
                      const char *url,
                      svn_revnum_t rev,
                      svn_ra_session_t *ra_session,
                      svn_client_ctx_t *ctx,
                      apr_pool_t *pool)
{
  svn_error_t *err;
  svn_opt_revision_t start_rev, end_rev, peg_rev;
  svn_opt_revision_t *ignored_rev;
  const char *head_url, *ignored_url;

  start_rev.kind = svn_opt_revision_head;
  peg_rev.kind = svn_opt_revision_number;
  peg_rev.value.number = rev;
  end_rev.kind = svn_opt_revision_unspecified;

  err = svn_client__repos_locations(&head_url, &ignored_rev,
                                    &ignored_url, &ignored_rev,
                                    ra_session,
                                    url, &peg_rev,
                                    &start_rev, &end_rev,
                                    ctx, pool);
  if (err &&
      ((err->apr_err == SVN_ERR_CLIENT_UNRELATED_RESOURCES) ||
       (err->apr_err == SVN_ERR_FS_NOT_FOUND)))
    {
      svn_error_clear(err);
      *same_p = FALSE;
      return SVN_NO_ERROR;
    }
  else
    SVN_ERR(err);

  /* ### Currently, the URLs should always be equal, since we can't
     ### walk forwards in history. */
  *same_p = (strcmp(url, head_url) == 0);

  return SVN_NO_ERROR;
}

/* A baton for wc_info_receiver(), containing the wrapped receiver. */
typedef struct wc_info_receiver_baton_t
{
  svn_client_info_receiver2_t client_receiver_func;
  void *client_receiver_baton;
} wc_info_receiver_baton_t;

/* A receiver for WC info, implementing svn_client_info_receiver2_t.
 * Convert the WC info to client info and pass it to the client info
 * receiver (BATON->client_receiver_func with BATON->client_receiver_baton). */
static svn_error_t *
wc_info_receiver(void *baton,
                 const char *abspath_or_url,
                 const svn_wc__info2_t *wc_info,
                 apr_pool_t *scratch_pool)
{
  wc_info_receiver_baton_t *b = baton;
  svn_client_info2_t client_info;

  /* Make a shallow copy in CLIENT_INFO of the contents of WC_INFO. */
  client_info.repos_root_URL = wc_info->repos_root_URL;
  client_info.repos_UUID = wc_info->repos_UUID;
  client_info.rev = wc_info->rev;
  client_info.URL = wc_info->URL;

  client_info.kind = wc_info->kind;
  client_info.size = wc_info->size;
  client_info.last_changed_rev = wc_info->last_changed_rev;
  client_info.last_changed_date = wc_info->last_changed_date;
  client_info.last_changed_author = wc_info->last_changed_author;

  client_info.lock = wc_info->lock;

  client_info.wc_info = wc_info->wc_info;

  return b->client_receiver_func(b->client_receiver_baton,
                                 abspath_or_url, &client_info, scratch_pool);
}

svn_error_t *
svn_client_info3(const char *abspath_or_url,
                 const svn_opt_revision_t *peg_revision,
                 const svn_opt_revision_t *revision,
                 svn_depth_t depth,
                 svn_boolean_t fetch_excluded,
                 svn_boolean_t fetch_actual_only,
                 const apr_array_header_t *changelists,
                 svn_client_info_receiver2_t receiver,
                 void *receiver_baton,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *pool)
{
  svn_ra_session_t *ra_session, *parent_ra_session;
  svn_revnum_t rev;
  const char *url;
  svn_node_kind_t url_kind;
  const char *repos_root_URL, *repos_UUID;
  svn_lock_t *lock;
  svn_boolean_t related;
  apr_hash_t *parent_ents;
  const char *parent_url, *base_name;
  svn_dirent_t *the_ent;
  svn_client_info2_t *info;
  svn_error_t *err;

  if (depth == svn_depth_unknown)
    depth = svn_depth_empty;

  if ((revision == NULL
       || revision->kind == svn_opt_revision_unspecified)
      && (peg_revision == NULL
          || peg_revision->kind == svn_opt_revision_unspecified))
    {
      /* Do all digging in the working copy. */
      wc_info_receiver_baton_t b;
      b.client_receiver_func = receiver;
      b.client_receiver_baton = receiver_baton;
      return svn_error_trace(
        svn_wc__get_info(ctx->wc_ctx, abspath_or_url, depth,
                        fetch_excluded, fetch_actual_only, changelists,
                         wc_info_receiver, &b,
                         ctx->cancel_func, ctx->cancel_baton, pool));
    }

  /* Go repository digging instead. */

  /* Trace rename history (starting at path_or_url@peg_revision) and
     return RA session to the possibly-renamed URL as it exists in REVISION.
     The ra_session returned will be anchored on this "final" URL. */
  SVN_ERR(svn_client__ra_session_from_path(&ra_session, &rev,
                                           &url, abspath_or_url, NULL,
                                           peg_revision,
                                           revision, ctx, pool));

  SVN_ERR(svn_ra_get_repos_root2(ra_session, &repos_root_URL, pool));
  SVN_ERR(svn_ra_get_uuid2(ra_session, &repos_UUID, pool));

  svn_uri_split(&parent_url, &base_name, url, pool);

  /* Get the dirent for the URL itself. */
  err = svn_ra_stat(ra_session, "", rev, &the_ent, pool);

  /* svn_ra_stat() will work against old versions of mod_dav_svn, but
     not old versions of svnserve.  In the case of a pre-1.2 svnserve,
     catch the specific error it throws:*/
  if (err && err->apr_err == SVN_ERR_RA_NOT_IMPLEMENTED)
    {
      /* Fall back to pre-1.2 strategy for fetching dirent's URL. */
      svn_error_clear(err);

      if (strcmp(url, repos_root_URL) == 0)
        {
          /* In this universe, there's simply no way to fetch
             information about the repository's root directory!
             If we're recursing, degrade gracefully: rather than
             throw an error, return no information about the
             repos root. */
          if (depth > svn_depth_empty)
            goto pre_1_2_recurse;

          /* Otherwise, we really are stuck.  Better tell the user
             what's going on. */
          return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                                   _("Server does not support retrieving "
                                     "information about the repository root"));
        }

      SVN_ERR(svn_ra_check_path(ra_session, "", rev, &url_kind, pool));
      if (url_kind == svn_node_none)
        return svn_error_createf(SVN_ERR_RA_ILLEGAL_URL, NULL,
                                 _("URL '%s' non-existent in revision %ld"),
                                 url, rev);

      /* Open a new RA session to the item's parent. */
      SVN_ERR(svn_client__open_ra_session_internal(&parent_ra_session, NULL,
                                                   parent_url, NULL,
                                                   NULL, FALSE, TRUE,
                                                   ctx, pool));

      /* Get all parent's entries, and find the item's dirent in the hash. */
      SVN_ERR(svn_ra_get_dir2(parent_ra_session, &parent_ents, NULL, NULL,
                              "", rev, DIRENT_FIELDS, pool));
      the_ent = apr_hash_get(parent_ents, base_name, APR_HASH_KEY_STRING);
      if (the_ent == NULL)
        return svn_error_createf(SVN_ERR_RA_ILLEGAL_URL, NULL,
                                 _("URL '%s' non-existent in revision %ld"),
                                 url, rev);
    }
  else if (err)
    {
      return svn_error_trace(err);
    }

  if (! the_ent)
    return svn_error_createf(SVN_ERR_RA_ILLEGAL_URL, NULL,
                             _("URL '%s' non-existent in revision %ld"),
                             url, rev);

  /* Check if the URL exists in HEAD and refers to the same resource.
     In this case, we check the repository for a lock on this URL.

     ### There is a possible race here, since HEAD might have changed since
     ### we checked it.  A solution to this problem could be to do the below
     ### check in a loop which only terminates if the HEAD revision is the same
     ### before and after this check.  That could, however, lead to a
     ### starvation situation instead.  */
  SVN_ERR(same_resource_in_head(&related, url, rev, ra_session, ctx, pool));
  if (related)
    {
      err = svn_ra_get_lock(ra_session, &lock, "", pool);

      /* An old mod_dav_svn will always work; there's nothing wrong with
         doing a PROPFIND for a property named "DAV:supportedlock". But
         an old svnserve will error. */
      if (err && err->apr_err == SVN_ERR_RA_NOT_IMPLEMENTED)
        {
          svn_error_clear(err);
          lock = NULL;
        }
      else if (err)
        return svn_error_trace(err);
    }
  else
    lock = NULL;

  /* Push the URL's dirent (and lock) at the callback.*/
  SVN_ERR(build_info_from_dirent(&info, the_ent, lock, url, rev,
                                 repos_UUID, repos_root_URL, pool));
  SVN_ERR(receiver(receiver_baton, base_name, info, pool));

  /* Possibly recurse, using the original RA session. */
  if (depth > svn_depth_empty && (the_ent->kind == svn_node_dir))
    {
      apr_hash_t *locks;

pre_1_2_recurse:
      if (peg_revision->kind == svn_opt_revision_head)
        {
          err = svn_ra_get_locks2(ra_session, &locks, "", depth,
                                  pool);

          /* Catch specific errors thrown by old mod_dav_svn or svnserve. */
          if (err &&
              (err->apr_err == SVN_ERR_RA_NOT_IMPLEMENTED
               || err->apr_err == SVN_ERR_UNSUPPORTED_FEATURE))
            {
              svn_error_clear(err);
              locks = apr_hash_make(pool); /* use an empty hash */
            }
          else if (err)
            return svn_error_trace(err);
        }
      else
        locks = apr_hash_make(pool); /* use an empty hash */

      SVN_ERR(push_dir_info(ra_session, url, "", rev,
                            repos_UUID, repos_root_URL,
                            receiver, receiver_baton,
                            depth, ctx, locks, pool));
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_client_get_wc_root(const char **wcroot_abspath,
                       const char *local_abspath,
                       svn_client_ctx_t *ctx,
                       apr_pool_t *result_pool,
                       apr_pool_t *scratch_pool)
{
  return svn_wc__get_wc_root(wcroot_abspath, ctx->wc_ctx, local_abspath,
                             result_pool, scratch_pool);
}


/* NOTE: This function was requested by the TortoiseSVN project.  See
   issue #3927. */
svn_error_t *
svn_client_min_max_revisions(svn_revnum_t *min_revision,
                             svn_revnum_t *max_revision,
                             const char *local_abspath,
                             svn_boolean_t committed,
                             svn_client_ctx_t *ctx,
                             apr_pool_t *scratch_pool)
{
  return svn_wc__min_max_revisions(min_revision, max_revision, ctx->wc_ctx,
                                   local_abspath, committed, scratch_pool);
}
