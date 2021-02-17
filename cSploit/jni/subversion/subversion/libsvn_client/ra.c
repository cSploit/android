/*
 * ra.c :  routines for interacting with the RA layer
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

#include "svn_error.h"
#include "svn_pools.h"
#include "svn_string.h"
#include "svn_sorts.h"
#include "svn_ra.h"
#include "svn_client.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_props.h"
#include "svn_mergeinfo.h"
#include "client.h"
#include "mergeinfo.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"


/* This is the baton that we pass svn_ra_open3(), and is associated with
   the callback table we provide to RA. */
typedef struct callback_baton_t
{
  /* Holds the directory that corresponds to the REPOS_URL at svn_ra_open3()
     time. When callbacks specify a relative path, they are joined with
     this base directory. */
  const char *base_dir_abspath;

  /* When true, makes sure temporary files are created
     outside the working copy. */
  svn_boolean_t read_only_wc;

  /* An array of svn_client_commit_item3_t * structures, present only
     during working copy commits. */
  const apr_array_header_t *commit_items;

  /* A client context. */
  svn_client_ctx_t *ctx;

  /* The pool to use for session-related items. */
  apr_pool_t *pool;

} callback_baton_t;



static svn_error_t *
open_tmp_file(apr_file_t **fp,
              void *callback_baton,
              apr_pool_t *pool)
{
  return svn_error_trace(svn_io_open_unique_file3(fp, NULL, NULL,
                                  svn_io_file_del_on_pool_cleanup,
                                  pool, pool));
}


/* This implements the 'svn_ra_get_wc_prop_func_t' interface. */
static svn_error_t *
get_wc_prop(void *baton,
            const char *relpath,
            const char *name,
            const svn_string_t **value,
            apr_pool_t *pool)
{
  callback_baton_t *cb = baton;
  const char *local_abspath = NULL;
  svn_error_t *err;

  *value = NULL;

  /* If we have a list of commit_items, search through that for a
     match for this relative URL. */
  if (cb->commit_items)
    {
      int i;
      for (i = 0; i < cb->commit_items->nelts; i++)
        {
          svn_client_commit_item3_t *item
            = APR_ARRAY_IDX(cb->commit_items, i, svn_client_commit_item3_t *);

          if (! strcmp(relpath, item->session_relpath))
            {
              SVN_ERR_ASSERT(svn_dirent_is_absolute(item->path));
              local_abspath = item->path;
              break;
            }
        }

      /* Commits can only query relpaths in the commit_items list
         since the commit driver traverses paths as they are, or will
         be, in the repository.  Non-commits query relpaths in the
         working copy. */
      if (! local_abspath)
        return SVN_NO_ERROR;
    }

  /* If we don't have a base directory, then there are no properties. */
  else if (cb->base_dir_abspath == NULL)
    return SVN_NO_ERROR;

  else
    local_abspath = svn_dirent_join(cb->base_dir_abspath, relpath, pool);

  err = svn_wc_prop_get2(value, cb->ctx->wc_ctx, local_abspath, name,
                         pool, pool);
  if (err && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
    {
      svn_error_clear(err);
      err = NULL;
    }
  return svn_error_trace(err);
}

/* This implements the 'svn_ra_push_wc_prop_func_t' interface. */
static svn_error_t *
push_wc_prop(void *baton,
             const char *relpath,
             const char *name,
             const svn_string_t *value,
             apr_pool_t *pool)
{
  callback_baton_t *cb = baton;
  int i;

  /* If we're committing, search through the commit_items list for a
     match for this relative URL. */
  if (! cb->commit_items)
    return svn_error_createf
      (SVN_ERR_UNSUPPORTED_FEATURE, NULL,
       _("Attempt to set wc property '%s' on '%s' in a non-commit operation"),
       name, svn_dirent_local_style(relpath, pool));

  for (i = 0; i < cb->commit_items->nelts; i++)
    {
      svn_client_commit_item3_t *item
        = APR_ARRAY_IDX(cb->commit_items, i, svn_client_commit_item3_t *);

      if (strcmp(relpath, item->session_relpath) == 0)
        {
          apr_pool_t *changes_pool = item->incoming_prop_changes->pool;
          svn_prop_t *prop = apr_palloc(changes_pool, sizeof(*prop));

          prop->name = apr_pstrdup(changes_pool, name);
          if (value)
            prop->value = svn_string_dup(value, changes_pool);
          else
            prop->value = NULL;

          /* Buffer the propchange to take effect during the
             post-commit process. */
          APR_ARRAY_PUSH(item->incoming_prop_changes, svn_prop_t *) = prop;
          return SVN_NO_ERROR;
        }
    }

  return SVN_NO_ERROR;
}


/* This implements the 'svn_ra_set_wc_prop_func_t' interface. */
static svn_error_t *
set_wc_prop(void *baton,
            const char *path,
            const char *name,
            const svn_string_t *value,
            apr_pool_t *pool)
{
  callback_baton_t *cb = baton;
  const char *local_abspath;

  local_abspath = svn_dirent_join(cb->base_dir_abspath, path, pool);

  /* We pass 1 for the 'force' parameter here.  Since the property is
     coming from the repository, we definitely want to accept it.
     Ideally, we'd raise a conflict if, say, the received property is
     svn:eol-style yet the file has a locally added svn:mime-type
     claiming that it's binary.  Probably the repository is still
     right, but the conflict would remind the user to make sure.
     Unfortunately, we don't have a clean mechanism for doing that
     here, so we just set the property and hope for the best. */
  return svn_error_trace(svn_wc_prop_set4(cb->ctx->wc_ctx, local_abspath,
                                          name,
                                          value, svn_depth_empty,
                                          TRUE /* skip_checks */,
                                          NULL /* changelist_filter */,
                                          NULL, NULL /* cancellation */,
                                          NULL, NULL /* notification */,
                                          pool));
}


/* This implements the `svn_ra_invalidate_wc_props_func_t' interface. */
static svn_error_t *
invalidate_wc_props(void *baton,
                    const char *path,
                    const char *prop_name,
                    apr_pool_t *pool)
{
  callback_baton_t *cb = baton;
  const char *local_abspath;

  local_abspath = svn_dirent_join(cb->base_dir_abspath, path, pool);

  /* It's easier just to clear the whole dav_cache than to remove
     individual items from it recursively like this.  And since we
     know that the RA providers that ship with Subversion only
     invalidate the one property they use the most from this cache,
     and that we're intentionally trying to get away from the use of
     the cache altogether anyway, there's little to lose in wiping the
     whole cache.  Is it the most well-behaved approach to take?  Not
     so much.  We choose not to care.  */
  return svn_error_trace(svn_wc__node_clear_dav_cache_recursive(
                              cb->ctx->wc_ctx, local_abspath, pool));
}


static svn_error_t *
cancel_callback(void *baton)
{
  callback_baton_t *b = baton;
  return svn_error_trace((b->ctx->cancel_func)(b->ctx->cancel_baton));
}


static svn_error_t *
get_client_string(void *baton,
                  const char **name,
                  apr_pool_t *pool)
{
  callback_baton_t *b = baton;
  *name = apr_pstrdup(pool, b->ctx->client_name);
  return SVN_NO_ERROR;
}


#define SVN_CLIENT__MAX_REDIRECT_ATTEMPTS 3 /* ### TODO:  Make configurable. */

svn_error_t *
svn_client__open_ra_session_internal(svn_ra_session_t **ra_session,
                                     const char **corrected_url,
                                     const char *base_url,
                                     const char *base_dir_abspath,
                                     const apr_array_header_t *commit_items,
                                     svn_boolean_t use_admin,
                                     svn_boolean_t read_only_wc,
                                     svn_client_ctx_t *ctx,
                                     apr_pool_t *pool)
{
  svn_ra_callbacks2_t *cbtable = apr_pcalloc(pool, sizeof(*cbtable));
  callback_baton_t *cb = apr_pcalloc(pool, sizeof(*cb));
  const char *uuid = NULL;

  SVN_ERR_ASSERT(base_dir_abspath != NULL || ! use_admin);
  SVN_ERR_ASSERT(base_dir_abspath == NULL
                        || svn_dirent_is_absolute(base_dir_abspath));

  cbtable->open_tmp_file = open_tmp_file;
  cbtable->get_wc_prop = use_admin ? get_wc_prop : NULL;
  cbtable->set_wc_prop = read_only_wc ? NULL : set_wc_prop;
  cbtable->push_wc_prop = commit_items ? push_wc_prop : NULL;
  cbtable->invalidate_wc_props = read_only_wc ? NULL : invalidate_wc_props;
  cbtable->auth_baton = ctx->auth_baton; /* new-style */
  cbtable->progress_func = ctx->progress_func;
  cbtable->progress_baton = ctx->progress_baton;
  cbtable->cancel_func = ctx->cancel_func ? cancel_callback : NULL;
  cbtable->get_client_string = get_client_string;

  cb->base_dir_abspath = base_dir_abspath;
  cb->read_only_wc = read_only_wc;
  cb->pool = pool;
  cb->commit_items = commit_items;
  cb->ctx = ctx;

  if (base_dir_abspath)
    {
      svn_error_t *err = svn_wc__node_get_repos_info(NULL, &uuid, ctx->wc_ctx,
                                                     base_dir_abspath,
                                                     pool, pool);

      if (err && (err->apr_err == SVN_ERR_WC_NOT_WORKING_COPY
                  || err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND
                  || err->apr_err == SVN_ERR_WC_UPGRADE_REQUIRED))
        {
          svn_error_clear(err);
          uuid = NULL;
        }
      else
        SVN_ERR(err);
    }

  /* If the caller allows for auto-following redirections, and the
     RA->open() call above reveals a CORRECTED_URL, try the new URL.
     We'll do this in a loop up to some maximum number follow-and-retry
     attempts.  */
  if (corrected_url)
    {
      apr_hash_t *attempted = apr_hash_make(pool);
      int attempts_left = SVN_CLIENT__MAX_REDIRECT_ATTEMPTS;

      *corrected_url = NULL;
      while (attempts_left--)
        {
          const char *corrected = NULL;

          /* Try to open the RA session.  If this is our last attempt,
             don't accept corrected URLs from the RA provider. */
          SVN_ERR(svn_ra_open4(ra_session,
                               attempts_left == 0 ? NULL : &corrected,
                               base_url, uuid, cbtable, cb, ctx->config, pool));

          /* No error and no corrected URL?  We're done here. */
          if (! corrected)
            break;

          /* Notify the user that a redirect is being followed. */
          if (ctx->notify_func2 != NULL)
            {
              svn_wc_notify_t *notify =
                svn_wc_create_notify_url(corrected,
                                         svn_wc_notify_url_redirect, pool);
              (*ctx->notify_func2)(ctx->notify_baton2, notify, pool);
            }

          /* Our caller will want to know what our final corrected URL was. */
          *corrected_url = corrected;

          /* Make sure we've not attempted this URL before. */
          if (apr_hash_get(attempted, corrected, APR_HASH_KEY_STRING))
            return svn_error_createf(SVN_ERR_CLIENT_CYCLE_DETECTED, NULL,
                                     _("Redirect cycle detected for URL '%s'"),
                                     corrected);

          /* Remember this CORRECTED_URL so we don't wind up in a loop. */
          apr_hash_set(attempted, corrected, APR_HASH_KEY_STRING, (void *)1);
          base_url = corrected;
        }
    }
  else
    {
      SVN_ERR(svn_ra_open4(ra_session, NULL, base_url,
                           uuid, cbtable, cb, ctx->config, pool));
    }

  return SVN_NO_ERROR;
 }
#undef SVN_CLIENT__MAX_REDIRECT_ATTEMPTS


svn_error_t *
svn_client_open_ra_session(svn_ra_session_t **session,
                           const char *url,
                           svn_client_ctx_t *ctx,
                           apr_pool_t *pool)
{
  return svn_error_trace(
             svn_client__open_ra_session_internal(session, NULL, url,
                                                  NULL, NULL, FALSE, TRUE,
                                                  ctx, pool));
}


svn_error_t *
svn_client_uuid_from_url(const char **uuid,
                         const char *url,
                         svn_client_ctx_t *ctx,
                         apr_pool_t *pool)
{
  svn_ra_session_t *ra_session;
  apr_pool_t *subpool = svn_pool_create(pool);

  /* use subpool to create a temporary RA session */
  SVN_ERR(svn_client__open_ra_session_internal(&ra_session, NULL, url,
                                               NULL, /* no base dir */
                                               NULL, FALSE, TRUE,
                                               ctx, subpool));

  SVN_ERR(svn_ra_get_uuid2(ra_session, uuid, pool));

  /* destroy the RA session */
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_client_uuid_from_path2(const char **uuid,
                           const char *local_abspath,
                           svn_client_ctx_t *ctx,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  return svn_error_trace(
    svn_wc__node_get_repos_info(NULL, uuid, ctx->wc_ctx, local_abspath,
                                result_pool, scratch_pool));
}




/* Convert a path or URL for display: if it is a local path, convert it to
 * the local path style; if it is a URL, return it unchanged. */
static const char *
path_or_url_local_style(const char *path_or_url,
                        apr_pool_t *pool)
{
  if (svn_path_is_url(path_or_url))
    return path_or_url;
  return svn_dirent_local_style(path_or_url, pool);
}

svn_error_t *
svn_client__ra_session_from_path(svn_ra_session_t **ra_session_p,
                                 svn_revnum_t *rev_p,
                                 const char **url_p,
                                 const char *path_or_url,
                                 const char *base_dir_abspath,
                                 const svn_opt_revision_t *peg_revision_p,
                                 const svn_opt_revision_t *revision,
                                 svn_client_ctx_t *ctx,
                                 apr_pool_t *pool)
{
  svn_ra_session_t *ra_session;
  const char *initial_url, *url;
  svn_opt_revision_t *good_rev;
  svn_opt_revision_t peg_revision, start_rev;
  svn_opt_revision_t dead_end_rev;
  svn_opt_revision_t *ignored_rev;
  svn_revnum_t rev;
  const char *ignored_url, *corrected_url;

  SVN_ERR(svn_client_url_from_path2(&initial_url, path_or_url, ctx, pool,
                                    pool));
  if (! initial_url)
    return svn_error_createf(SVN_ERR_ENTRY_MISSING_URL, NULL,
                             _("'%s' has no URL"), path_or_url);

  start_rev = *revision;
  peg_revision = *peg_revision_p;
  SVN_ERR(svn_opt_resolve_revisions(&peg_revision, &start_rev,
                                    svn_path_is_url(path_or_url),
                                    TRUE,
                                    pool));

  SVN_ERR(svn_client__open_ra_session_internal(&ra_session, &corrected_url,
                                               initial_url,
                                               base_dir_abspath, NULL,
                                               base_dir_abspath != NULL,
                                               FALSE, ctx, pool));

  /* If we got a CORRECTED_URL, we'll want to refer to that as the
     URL-ized form of PATH_OR_URL from now on. */
  if (corrected_url && svn_path_is_url(path_or_url))
    path_or_url = corrected_url;

  dead_end_rev.kind = svn_opt_revision_unspecified;

  /* Run the history function to get the object's (possibly
     different) url in REVISION. */
  SVN_ERR(svn_client__repos_locations(&url, &good_rev,
                                      &ignored_url, &ignored_rev,
                                      ra_session,
                                      path_or_url, &peg_revision,
                                      /* search range: */
                                      &start_rev, &dead_end_rev,
                                      ctx, pool));

  /* Make the session point to the real URL. */
  SVN_ERR(svn_ra_reparent(ra_session, url, pool));

  /* Resolve good_rev into a real revnum. */
  if (good_rev->kind == svn_opt_revision_unspecified)
    good_rev->kind = svn_opt_revision_head;
  SVN_ERR(svn_client__get_revision_number(&rev, NULL, ctx->wc_ctx, url,
                                          ra_session, good_rev, pool));

  *ra_session_p = ra_session;
  *rev_p = rev;
  *url_p = url;

  return SVN_NO_ERROR;
}


svn_error_t *
svn_client__ensure_ra_session_url(const char **old_session_url,
                                  svn_ra_session_t *ra_session,
                                  const char *session_url,
                                  apr_pool_t *pool)
{
  *old_session_url = NULL;
  SVN_ERR(svn_ra_get_session_url(ra_session, old_session_url, pool));
  if (! session_url)
    SVN_ERR(svn_ra_get_repos_root2(ra_session, &session_url, pool));
  if (strcmp(*old_session_url, session_url) != 0)
    SVN_ERR(svn_ra_reparent(ra_session, session_url, pool));
  return SVN_NO_ERROR;
}



/*** Repository Locations ***/

struct gls_receiver_baton_t
{
  apr_array_header_t *segments;
  svn_client_ctx_t *ctx;
  apr_pool_t *pool;
};

static svn_error_t *
gls_receiver(svn_location_segment_t *segment,
             void *baton,
             apr_pool_t *pool)
{
  struct gls_receiver_baton_t *b = baton;
  APR_ARRAY_PUSH(b->segments, svn_location_segment_t *) =
    svn_location_segment_dup(segment, b->pool);
  if (b->ctx->cancel_func)
    SVN_ERR((b->ctx->cancel_func)(b->ctx->cancel_baton));
  return SVN_NO_ERROR;
}

/* A qsort-compatible function which sorts svn_location_segment_t's
   based on their revision range covering, resulting in ascending
   (oldest-to-youngest) ordering. */
static int
compare_segments(const void *a, const void *b)
{
  const svn_location_segment_t *a_seg
    = *((const svn_location_segment_t * const *) a);
  const svn_location_segment_t *b_seg
    = *((const svn_location_segment_t * const *) b);
  if (a_seg->range_start == b_seg->range_start)
    return 0;
  return (a_seg->range_start < b_seg->range_start) ? -1 : 1;
}

svn_error_t *
svn_client__repos_location_segments(apr_array_header_t **segments,
                                    svn_ra_session_t *ra_session,
                                    const char *path,
                                    svn_revnum_t peg_revision,
                                    svn_revnum_t start_revision,
                                    svn_revnum_t end_revision,
                                    svn_client_ctx_t *ctx,
                                    apr_pool_t *pool)
{
  struct gls_receiver_baton_t gls_receiver_baton;
  *segments = apr_array_make(pool, 8, sizeof(svn_location_segment_t *));
  gls_receiver_baton.segments = *segments;
  gls_receiver_baton.ctx = ctx;
  gls_receiver_baton.pool = pool;
  SVN_ERR(svn_ra_get_location_segments(ra_session, path, peg_revision,
                                       start_revision, end_revision,
                                       gls_receiver, &gls_receiver_baton,
                                       pool));
  qsort((*segments)->elts, (*segments)->nelts,
        (*segments)->elt_size, compare_segments);
  return SVN_NO_ERROR;
}


svn_error_t *
svn_client__repos_locations(const char **start_url,
                            svn_opt_revision_t **start_revision,
                            const char **end_url,
                            svn_opt_revision_t **end_revision,
                            svn_ra_session_t *ra_session,
                            const char *path,
                            const svn_opt_revision_t *revision,
                            const svn_opt_revision_t *start,
                            const svn_opt_revision_t *end,
                            svn_client_ctx_t *ctx,
                            apr_pool_t *pool)
{
  const char *repos_url;
  const char *url;
  const char *start_path = NULL;
  const char *end_path = NULL;
  const char *local_abspath_or_url;
  svn_revnum_t peg_revnum = SVN_INVALID_REVNUM;
  svn_revnum_t start_revnum, end_revnum;
  svn_revnum_t youngest_rev = SVN_INVALID_REVNUM;
  apr_array_header_t *revs;
  apr_hash_t *rev_locs;
  apr_pool_t *subpool = svn_pool_create(pool);

  /* Ensure that we are given some real revision data to work with.
     (It's okay if the END is unspecified -- in that case, we'll just
     set it to the same thing as START.)  */
  if (revision->kind == svn_opt_revision_unspecified
      || start->kind == svn_opt_revision_unspecified)
    return svn_error_create(SVN_ERR_CLIENT_BAD_REVISION, NULL, NULL);

  /* Check to see if this is schedule add with history working copy
     path.  If it is, then we need to use the URL and peg revision of
     the copyfrom information. */
  if (! svn_path_is_url(path))
    {
      SVN_ERR(svn_dirent_get_absolute(&local_abspath_or_url, path, subpool));

      if (revision->kind == svn_opt_revision_working)
        {
          const char *repos_root_url;
          const char *repos_relpath;
          svn_boolean_t is_copy;

          SVN_ERR(svn_wc__node_get_origin(&is_copy, &peg_revnum, &repos_relpath,
                                          &repos_root_url, NULL, NULL,
                                          ctx->wc_ctx, local_abspath_or_url,
                                          FALSE, subpool, subpool));

          if (repos_relpath)
            url = svn_path_url_add_component2(repos_root_url, repos_relpath,
                                              pool);
          else
            url = NULL;

          if (url && is_copy && ra_session)
            {
              const char *session_url;
              SVN_ERR(svn_ra_get_session_url(ra_session, &session_url,
                                             subpool));

              if (strcmp(session_url, url) != 0)
                {
                  /* We can't use the caller provided RA session now :( */
                  ra_session = NULL;
                }
            }
        }
      else
        url = NULL;

      if (! url)
        SVN_ERR(svn_wc__node_get_url(&url, ctx->wc_ctx,
                                     local_abspath_or_url, pool, subpool));

      if (!url)
        {
          return svn_error_createf(SVN_ERR_ENTRY_MISSING_URL, NULL,
                                   _("'%s' has no URL"),
                                   svn_dirent_local_style(path, pool));
        }
    }
  else
    {
      local_abspath_or_url = path;
      url = path;
    }

  /* ### We should be smarter here.  If the callers just asks for BASE and
     WORKING revisions, we should already have the correct URLs, so we
     don't need to do anything more here in that case. */

  /* Open a RA session to this URL if we don't have one already. */
  if (! ra_session)
    SVN_ERR(svn_client__open_ra_session_internal(&ra_session, NULL, url, NULL,
                                                 NULL, FALSE, TRUE,
                                                 ctx, subpool));

  /* Resolve the opt_revision_ts. */
  if (peg_revnum == SVN_INVALID_REVNUM)
    SVN_ERR(svn_client__get_revision_number(&peg_revnum, &youngest_rev,
                                            ctx->wc_ctx, local_abspath_or_url,
                                            ra_session, revision, pool));

  SVN_ERR(svn_client__get_revision_number(&start_revnum, &youngest_rev,
                                          ctx->wc_ctx, local_abspath_or_url,
                                          ra_session, start, pool));
  if (end->kind == svn_opt_revision_unspecified)
    end_revnum = start_revnum;
  else
    SVN_ERR(svn_client__get_revision_number(&end_revnum, &youngest_rev,
                                            ctx->wc_ctx, local_abspath_or_url,
                                            ra_session, end, pool));

  /* Set the output revision variables. */
  *start_revision = apr_pcalloc(pool, sizeof(**start_revision));
  (*start_revision)->kind = svn_opt_revision_number;
  (*start_revision)->value.number = start_revnum;
  if (end->kind != svn_opt_revision_unspecified)
    {
      *end_revision = apr_pcalloc(pool, sizeof(**end_revision));
      (*end_revision)->kind = svn_opt_revision_number;
      (*end_revision)->value.number = end_revnum;
    }

  if (start_revnum == peg_revnum && end_revnum == peg_revnum)
    {
      /* Avoid a network request in the common easy case. */
      *start_url = url;
      if (end->kind != svn_opt_revision_unspecified)
        *end_url = url;
      svn_pool_destroy(subpool);
      return SVN_NO_ERROR;
    }

  SVN_ERR(svn_ra_get_repos_root2(ra_session, &repos_url, subpool));

  revs = apr_array_make(subpool, 2, sizeof(svn_revnum_t));
  APR_ARRAY_PUSH(revs, svn_revnum_t) = start_revnum;
  if (end_revnum != start_revnum)
    APR_ARRAY_PUSH(revs, svn_revnum_t) = end_revnum;

  SVN_ERR(svn_ra_get_locations(ra_session, &rev_locs, "", peg_revnum,
                               revs, subpool));

  /* We'd better have all the paths we were looking for! */
  start_path = apr_hash_get(rev_locs, &start_revnum, sizeof(svn_revnum_t));
  if (! start_path)
    return svn_error_createf
      (SVN_ERR_CLIENT_UNRELATED_RESOURCES, NULL,
       _("Unable to find repository location for '%s' in revision %ld"),
       path_or_url_local_style(path, pool), start_revnum);

  end_path = apr_hash_get(rev_locs, &end_revnum, sizeof(svn_revnum_t));
  if (! end_path)
    return svn_error_createf
      (SVN_ERR_CLIENT_UNRELATED_RESOURCES, NULL,
       _("The location for '%s' for revision %ld does not exist in the "
         "repository or refers to an unrelated object"),
       path_or_url_local_style(path, pool), end_revnum);

  /* Set our return variables */
  *start_url = svn_path_url_add_component2(repos_url, start_path + 1, pool);
  if (end->kind != svn_opt_revision_unspecified)
    *end_url = svn_path_url_add_component2(repos_url, end_path + 1, pool);

  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}


svn_error_t *
svn_client__get_youngest_common_ancestor(const char **ancestor_path,
                                         svn_revnum_t *ancestor_revision,
                                         const char *path_or_url1,
                                         svn_revnum_t rev1,
                                         const char *path_or_url2,
                                         svn_revnum_t rev2,
                                         svn_client_ctx_t *ctx,
                                         apr_pool_t *pool)
{
  apr_hash_t *history1, *history2;
  apr_hash_index_t *hi;
  svn_revnum_t yc_revision = SVN_INVALID_REVNUM;
  const char *yc_path = NULL;
  svn_opt_revision_t revision1, revision2;
  svn_boolean_t has_rev_zero_history1;
  svn_boolean_t has_rev_zero_history2;

  revision1.kind = revision2.kind = svn_opt_revision_number;
  revision1.value.number = rev1;
  revision2.value.number = rev2;

  /* We're going to cheat and use history-as-mergeinfo because it
     saves us a bunch of annoying custom data comparisons and such. */
  SVN_ERR(svn_client__get_history_as_mergeinfo(&history1,
                                               &has_rev_zero_history1,
                                               path_or_url1,
                                               &revision1,
                                               SVN_INVALID_REVNUM,
                                               SVN_INVALID_REVNUM,
                                               NULL, ctx, pool));
  SVN_ERR(svn_client__get_history_as_mergeinfo(&history2,
                                               &has_rev_zero_history2,
                                               path_or_url2,
                                               &revision2,
                                               SVN_INVALID_REVNUM,
                                               SVN_INVALID_REVNUM,
                                               NULL, ctx, pool));

  /* Loop through the first location's history, check for overlapping
     paths and ranges in the second location's history, and
     remembering the youngest matching location. */
  for (hi = apr_hash_first(pool, history1); hi; hi = apr_hash_next(hi))
    {
      const char *path = svn__apr_hash_index_key(hi);
      apr_ssize_t path_len = svn__apr_hash_index_klen(hi);
      apr_array_header_t *ranges1 = svn__apr_hash_index_val(hi);
      apr_array_header_t *ranges2, *common;

      ranges2 = apr_hash_get(history2, path, path_len);
      if (ranges2)
        {
          /* We have a path match.  Now, did our two histories share
             any revisions at that path? */
          SVN_ERR(svn_rangelist_intersect(&common, ranges1, ranges2,
                                          TRUE, pool));
          if (common->nelts)
            {
              svn_merge_range_t *yc_range =
                APR_ARRAY_IDX(common, common->nelts - 1, svn_merge_range_t *);
              if ((! SVN_IS_VALID_REVNUM(yc_revision))
                  || (yc_range->end > yc_revision))
                {
                  yc_revision = yc_range->end;
                  yc_path = path + 1;
                }
            }
        }
    }

  /* It's possible that PATH_OR_URL1 and PATH_OR_URL2's only common
     history is revision 0. */
  if (!yc_path && has_rev_zero_history1 && has_rev_zero_history2)
    {
      yc_path = "/";
      yc_revision = 0;
    }

  *ancestor_path = yc_path;
  *ancestor_revision = yc_revision;
  return SVN_NO_ERROR;
}
