/**
 * @copyright
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
 * @endcopyright
 */

#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_pools.h"
#include "svn_wc.h"

#include "wc.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"



svn_wc_info_t *
svn_wc_info_dup(const svn_wc_info_t *info,
                apr_pool_t *pool)
{
  svn_wc_info_t *new_info = apr_pmemdup(pool, info, sizeof(*new_info));

  if (info->changelist)
    new_info->changelist = apr_pstrdup(pool, info->changelist);
  if (info->checksum)
    new_info->checksum = svn_checksum_dup(info->checksum, pool);
  if (info->conflicts)
    {
      int i;

      apr_array_header_t *new_conflicts
        = apr_array_make(pool, info->conflicts->nelts, info->conflicts->elt_size);
      for (i = 0; i < info->conflicts->nelts; i++)
        {
          APR_ARRAY_PUSH(new_conflicts, svn_wc_conflict_description2_t *)
            = svn_wc__conflict_description2_dup(
                APR_ARRAY_IDX(info->conflicts, i,
                              const svn_wc_conflict_description2_t *),
                pool);
        }
      new_info->conflicts = new_conflicts;
    }
  if (info->copyfrom_url)
    new_info->copyfrom_url = apr_pstrdup(pool, info->copyfrom_url);
  if (info->wcroot_abspath)
    new_info->wcroot_abspath = apr_pstrdup(pool, info->wcroot_abspath);
  return new_info;
}


/* Set *INFO to a new struct, allocated in RESULT_POOL, built from the WC
   metadata of LOCAL_ABSPATH.  Pointer fields are copied by reference, not
   dup'd. */
static svn_error_t *
build_info_for_node(svn_wc__info2_t **info,
                     svn_wc__db_t *db,
                     const char *local_abspath,
                     svn_node_kind_t kind,
                     apr_pool_t *result_pool,
                     apr_pool_t *scratch_pool)
{
  svn_wc__info2_t *tmpinfo;
  const char *repos_relpath;
  svn_wc__db_status_t status;
  svn_wc__db_kind_t db_kind;
  const char *original_repos_relpath;
  const char *original_repos_root_url;
  const char *original_uuid;
  svn_revnum_t original_revision;
  svn_wc__db_lock_t *lock;
  svn_boolean_t conflicted;
  svn_boolean_t op_root;
  svn_boolean_t have_base;
  svn_wc_info_t *wc_info;

  tmpinfo = apr_pcalloc(result_pool, sizeof(*tmpinfo));
  tmpinfo->kind = kind;

  wc_info = apr_pcalloc(result_pool, sizeof(*wc_info));
  tmpinfo->wc_info = wc_info;

  wc_info->copyfrom_rev = SVN_INVALID_REVNUM;

  SVN_ERR(svn_wc__db_read_info(&status, &db_kind, &tmpinfo->rev,
                               &repos_relpath,
                               &tmpinfo->repos_root_URL, &tmpinfo->repos_UUID,
                               &tmpinfo->last_changed_rev,
                               &tmpinfo->last_changed_date,
                               &tmpinfo->last_changed_author,
                               &wc_info->depth, &wc_info->checksum, NULL,
                               &original_repos_relpath,
                               &original_repos_root_url, &original_uuid,
                               &original_revision, &lock,
                               &wc_info->recorded_size,
                               &wc_info->recorded_time,
                               &wc_info->changelist,
                               &conflicted, &op_root, NULL, NULL,
                               &have_base, NULL, NULL,
                               db, local_abspath,
                               result_pool, scratch_pool));

  if (original_repos_root_url != NULL)
    {
      tmpinfo->repos_root_URL = original_repos_root_url;
      tmpinfo->repos_UUID = original_uuid;
    }

  if (status == svn_wc__db_status_added)
    {
      /* ### We should also just be fetching the true BASE revision
         ### here, which means copied items would also not have a
         ### revision to display.  But WC-1 wants to show the revision of
         ### copy targets as the copyfrom-rev.  *sigh* */

      if (original_repos_relpath)
        {
          /* Root or child of copy */
          tmpinfo->rev = original_revision;
          repos_relpath = original_repos_relpath;

          if (op_root)
            {
              wc_info->copyfrom_url =
                    svn_path_url_add_component2(tmpinfo->repos_root_URL,
                                                original_repos_relpath,
                                                result_pool);

              wc_info->copyfrom_rev = original_revision;
            }
        }
      else if (op_root)
        {
          /* Local addition */
          SVN_ERR(svn_wc__db_scan_addition(NULL, NULL, &repos_relpath,
                                           &tmpinfo->repos_root_URL,
                                           &tmpinfo->repos_UUID,
                                           NULL, NULL, NULL, NULL,
                                           db, local_abspath,
                                           result_pool, scratch_pool));

          if (have_base)
            SVN_ERR(svn_wc__db_base_get_info(NULL, NULL, &tmpinfo->rev, NULL,
                                             NULL, NULL, NULL, NULL, NULL,
                                             NULL, NULL, NULL, NULL, NULL,
                                             NULL,
                                             db, local_abspath,
                                             scratch_pool, scratch_pool));
        }
      else
        {
          /* Child of copy. ### Not WC-NG like */
          SVN_ERR(svn_wc__internal_get_origin(NULL, &tmpinfo->rev,
                                              &repos_relpath,
                                              &tmpinfo->repos_root_URL,
                                              &tmpinfo->repos_UUID, NULL,
                                              db, local_abspath, TRUE,
                                              result_pool, scratch_pool));
        }

      /* ### We should be able to avoid both these calls with the information
         from read_info() in most cases */
      SVN_ERR(svn_wc__internal_node_get_schedule(&wc_info->schedule, NULL,
                                                 db, local_abspath,
                                                 scratch_pool));
      SVN_ERR(svn_wc__db_read_url(&tmpinfo->URL, db, local_abspath,
                                result_pool, scratch_pool));
    }
  else if (status == svn_wc__db_status_deleted)
    {
      const char *work_del_abspath;

      SVN_ERR(svn_wc__db_read_pristine_info(NULL, NULL,
                                            &tmpinfo->last_changed_rev,
                                            &tmpinfo->last_changed_date,
                                            &tmpinfo->last_changed_author,
                                            &wc_info->depth,
                                            &wc_info->checksum,
                                            NULL, NULL,
                                            db, local_abspath,
                                            result_pool, scratch_pool));

      /* And now fetch the url and revision of what will be deleted */
      SVN_ERR(svn_wc__db_scan_deletion(NULL, NULL,
                                       &work_del_abspath,
                                       db, local_abspath,
                                       scratch_pool, scratch_pool));
      if (work_del_abspath != NULL)
        {
          /* This is a deletion within a copied subtree. Get the copied-from
           * revision. */
          const char *added_abspath = svn_dirent_dirname(work_del_abspath,
                                                         scratch_pool);

          SVN_ERR(svn_wc__db_scan_addition(NULL, NULL, &repos_relpath,
                                           &tmpinfo->repos_root_URL,
                                           &tmpinfo->repos_UUID,
                                           NULL, NULL, NULL,
                                           &tmpinfo->rev,
                                           db, added_abspath,
                                           result_pool, scratch_pool));

          tmpinfo->URL = svn_path_url_add_component2(
                              tmpinfo->repos_root_URL,
                              svn_relpath_join(repos_relpath,
                                    svn_dirent_skip_ancestor(added_abspath,
                                                             local_abspath),
                                    scratch_pool),
                              result_pool);
        }
      else
        {
          SVN_ERR(svn_wc__db_base_get_info(NULL, NULL, &tmpinfo->rev,
                                           &repos_relpath,
                                           &tmpinfo->repos_root_URL,
                                           &tmpinfo->repos_UUID, NULL, NULL,
                                           NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL,
                                           db, local_abspath,
                                           result_pool, scratch_pool));

          tmpinfo->URL = svn_path_url_add_component2(tmpinfo->repos_root_URL,
                                                     repos_relpath,
                                                     result_pool);
        }

      wc_info->schedule = svn_wc_schedule_delete;
    }
  else if (status == svn_wc__db_status_not_present
           || status == svn_wc__db_status_server_excluded)
    {
      *info = NULL;
      return SVN_NO_ERROR;
    }
  else
    {
      /* Just a BASE node. We have all the info we need */
      tmpinfo->URL = svn_path_url_add_component2(tmpinfo->repos_root_URL,
                                                 repos_relpath,
                                                 result_pool);
      wc_info->schedule = svn_wc_schedule_normal;
    }

  if (status == svn_wc__db_status_excluded)
    tmpinfo->wc_info->depth = svn_depth_exclude;

  /* A default */
  tmpinfo->size = SVN_INVALID_FILESIZE;

  SVN_ERR(svn_wc__db_get_wcroot(&tmpinfo->wc_info->wcroot_abspath, db,
                                local_abspath, result_pool, scratch_pool));

  if (conflicted)
    SVN_ERR(svn_wc__db_read_conflicts(&wc_info->conflicts, db,
                                      local_abspath,
                                      result_pool, scratch_pool));
  else
    wc_info->conflicts = NULL;

  /* lock stuff */
  if (lock != NULL)
    {
      tmpinfo->lock = apr_pcalloc(result_pool, sizeof(*(tmpinfo->lock)));
      tmpinfo->lock->token         = lock->token;
      tmpinfo->lock->owner         = lock->owner;
      tmpinfo->lock->comment       = lock->comment;
      tmpinfo->lock->creation_date = lock->date;
    }

  /* ### Temporary hacks to keep our test suite happy: */

  *info = tmpinfo;
  return SVN_NO_ERROR;
}


/* Set *INFO to a new struct with minimal content, to be
   used in reporting info for unversioned tree conflict victims. */
/* ### Some fields we could fill out based on the parent dir's entry
       or by looking at an obstructing item. */
static svn_error_t *
build_info_for_unversioned(svn_wc__info2_t **info,
                           apr_pool_t *pool)
{
  svn_wc__info2_t *tmpinfo = apr_pcalloc(pool, sizeof(*tmpinfo));
  svn_wc_info_t *wc_info = apr_pcalloc(pool, sizeof (*wc_info));

  tmpinfo->URL                  = NULL;
  tmpinfo->repos_UUID           = NULL;
  tmpinfo->repos_root_URL       = NULL;
  tmpinfo->rev                  = SVN_INVALID_REVNUM;
  tmpinfo->kind                 = svn_node_none;
  tmpinfo->size                 = SVN_INVALID_FILESIZE;
  tmpinfo->last_changed_rev     = SVN_INVALID_REVNUM;
  tmpinfo->last_changed_date    = 0;
  tmpinfo->last_changed_author  = NULL;
  tmpinfo->lock                 = NULL;

  tmpinfo->wc_info = wc_info;

  wc_info->copyfrom_rev = SVN_INVALID_REVNUM;
  wc_info->depth = svn_depth_unknown;
  wc_info->recorded_size = SVN_INVALID_FILESIZE;

  *info = tmpinfo;
  return SVN_NO_ERROR;
}

/* Callback and baton for crawl_entries() walk over entries files. */
struct found_entry_baton
{
  svn_wc__info_receiver2_t receiver;
  void *receiver_baton;
  svn_wc__db_t *db;
  svn_boolean_t actual_only;
  svn_boolean_t first;
  /* The set of tree conflicts that have been found but not (yet) visited by
   * the tree walker.  Map of abspath -> svn_wc_conflict_description2_t. */
  apr_hash_t *tree_conflicts;
  apr_pool_t *pool;
};

/* Call WALK_BATON->receiver with WALK_BATON->receiver_baton, passing to it
 * info about the path LOCAL_ABSPATH.
 * An svn_wc__node_found_func_t callback function. */
static svn_error_t *
info_found_node_callback(const char *local_abspath,
                         svn_node_kind_t kind,
                         void *walk_baton,
                         apr_pool_t *scratch_pool)
{
  struct found_entry_baton *fe_baton = walk_baton;
  svn_wc__info2_t *info;

  SVN_ERR(build_info_for_node(&info, fe_baton->db, local_abspath,
                               kind, scratch_pool, scratch_pool));

  if (info == NULL)
    {
      if (!fe_baton->first)
        return SVN_NO_ERROR; /* not present or server excluded descendant */

      /* If the info root is not found, that is an error */
      return svn_error_createf(SVN_ERR_WC_PATH_NOT_FOUND, NULL,
                               _("The node '%s' was not found."),
                               svn_dirent_local_style(local_abspath,
                                                      scratch_pool));
    }

  fe_baton->first = FALSE;

  SVN_ERR_ASSERT(info->wc_info != NULL);
  SVN_ERR(fe_baton->receiver(fe_baton->receiver_baton, local_abspath,
                             info, scratch_pool));

  /* If this node is a versioned directory, make a note of any tree conflicts
   * on all immediate children.  Some of these may be visited later in this
   * walk, at which point they will be removed from the list, while any that
   * are not visited will remain in the list. */
  if (fe_baton->actual_only && kind == svn_node_dir)
    {
      apr_hash_t *conflicts;
      apr_hash_index_t *hi;

      SVN_ERR(svn_wc__db_op_read_all_tree_conflicts(
                &conflicts, fe_baton->db, local_abspath,
                fe_baton->pool, scratch_pool));
      for (hi = apr_hash_first(scratch_pool, conflicts); hi;
           hi = apr_hash_next(hi))
        {
          const char *this_basename = svn__apr_hash_index_key(hi);

          apr_hash_set(fe_baton->tree_conflicts,
                       svn_dirent_join(local_abspath, this_basename,
                                       fe_baton->pool),
                       APR_HASH_KEY_STRING, svn__apr_hash_index_val(hi));
        }
    }

  /* Delete this path which we are currently visiting from the list of tree
   * conflicts.  This relies on the walker visiting a directory before visiting
   * its children. */
  apr_hash_set(fe_baton->tree_conflicts, local_abspath, APR_HASH_KEY_STRING,
               NULL);

  return SVN_NO_ERROR;
}


/* Return TRUE iff the subtree at ROOT_ABSPATH, restricted to depth DEPTH,
 * would include the path CHILD_ABSPATH of kind CHILD_KIND. */
static svn_boolean_t
depth_includes(const char *root_abspath,
               svn_depth_t depth,
               const char *child_abspath,
               svn_node_kind_t child_kind,
               apr_pool_t *scratch_pool)
{
  const char *parent_abspath = svn_dirent_dirname(child_abspath, scratch_pool);

  return (depth == svn_depth_infinity
          || ((depth == svn_depth_immediates
               || (depth == svn_depth_files && child_kind == svn_node_file))
              && strcmp(root_abspath, parent_abspath) == 0)
          || strcmp(root_abspath, child_abspath) == 0);
}


svn_error_t *
svn_wc__get_info(svn_wc_context_t *wc_ctx,
                 const char *local_abspath,
                 svn_depth_t depth,
                 svn_boolean_t fetch_excluded,
                 svn_boolean_t fetch_actual_only,
                 const apr_array_header_t *changelist_filter,
                 svn_wc__info_receiver2_t receiver,
                 void *receiver_baton,
                 svn_cancel_func_t cancel_func,
                 void *cancel_baton,
                 apr_pool_t *scratch_pool)
{
  struct found_entry_baton fe_baton;
  svn_error_t *err;
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  apr_hash_index_t *hi;

  fe_baton.receiver = receiver;
  fe_baton.receiver_baton = receiver_baton;
  fe_baton.db = wc_ctx->db;
  fe_baton.actual_only = fetch_actual_only;
  fe_baton.first = TRUE;
  fe_baton.tree_conflicts = apr_hash_make(scratch_pool);
  fe_baton.pool = scratch_pool;

  err = svn_wc__internal_walk_children(wc_ctx->db, local_abspath,
                                       fetch_excluded,
                                       changelist_filter,
                                       info_found_node_callback,
                                       &fe_baton, depth,
                                       cancel_func, cancel_baton,
                                       iterpool);

  /* If the target root node is not present, svn_wc__internal_walk_children()
     returns a PATH_NOT_FOUND error and doesn't call the callback.  If there
     is a tree conflict on this node, that is not an error. */
  if (fe_baton.first /* not visited by walk_children */
      && fetch_actual_only
      && err && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
    {
      const svn_wc_conflict_description2_t *root_tree_conflict;
      svn_error_t *err2;

      err2 = svn_wc__db_op_read_tree_conflict(&root_tree_conflict,
                                              wc_ctx->db, local_abspath,
                                              scratch_pool, iterpool);

      if ((err2 && err2->apr_err == SVN_ERR_WC_PATH_NOT_FOUND))
        {
          svn_error_clear(err2);
          return svn_error_trace(err);
        }
      else if (err2 || !root_tree_conflict)
        return svn_error_compose_create(err, err2);

      svn_error_clear(err);

      apr_hash_set(fe_baton.tree_conflicts, local_abspath,
                   APR_HASH_KEY_STRING, root_tree_conflict);
    }
  else
    SVN_ERR(err);

  /* If there are any tree conflicts that we have found but have not reported,
   * send a minimal info struct for each one now. */
  for (hi = apr_hash_first(scratch_pool, fe_baton.tree_conflicts); hi;
       hi = apr_hash_next(hi))
    {
      const char *this_abspath = svn__apr_hash_index_key(hi);
      const svn_wc_conflict_description2_t *tree_conflict
        = svn__apr_hash_index_val(hi);

      svn_pool_clear(iterpool);

      if (depth_includes(local_abspath, depth, tree_conflict->local_abspath,
                         tree_conflict->node_kind, iterpool))
        {
          apr_array_header_t *conflicts = apr_array_make(iterpool,
            1, sizeof(const svn_wc_conflict_description2_t *));
          svn_wc__info2_t *info;

          SVN_ERR(build_info_for_unversioned(&info, iterpool));
          SVN_ERR(svn_wc__internal_get_repos_info(&info->repos_root_URL,
                                                  &info->repos_UUID,
                                                  fe_baton.db,
                                                  local_abspath,
                                                  iterpool, iterpool));
          APR_ARRAY_PUSH(conflicts, const svn_wc_conflict_description2_t *)
            = tree_conflict;
          info->wc_info->conflicts = conflicts;

          SVN_ERR(receiver(receiver_baton, this_abspath, info, iterpool));
        }
    }
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}
