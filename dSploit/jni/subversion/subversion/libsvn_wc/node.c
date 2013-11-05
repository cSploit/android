/*
 * node.c:  routines for getting information about nodes in the working copy.
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

/* A note about these functions:

   We aren't really sure yet which bits of data libsvn_client needs about
   nodes.  In wc-1, we just grab the entry, and then use whatever we want
   from it.  Such a pattern is Bad.

   This file is intended to hold functions which retrieve specific bits of
   information about a node, and will hopefully give us a better idea about
   what data libsvn_client needs, and how to best provide that data in 1.7
   final.  As such, these functions should only be called from outside
   libsvn_wc; any internal callers are encouraged to use the appropriate
   information fetching function, such as svn_wc__db_read_info().
*/

#include <apr_pools.h>
#include <apr_time.h>

#include "svn_pools.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_hash.h"
#include "svn_types.h"

#include "wc.h"
#include "props.h"
#include "entries.h"
#include "wc_db.h"

#include "svn_private_config.h"
#include "private/svn_wc_private.h"


/* Set *CHILDREN_ABSPATHS to a new array of the full paths formed by joining
 * each name in REL_CHILDREN onto DIR_ABSPATH.  If SHOW_HIDDEN is false then
 * omit any paths that are reported as 'hidden' by svn_wc__db_node_hidden().
 *
 * Allocate the output array and its elements in RESULT_POOL. */
static svn_error_t *
filter_and_make_absolute(const apr_array_header_t **children_abspaths,
                         svn_wc_context_t *wc_ctx,
                         const char *dir_abspath,
                         const apr_array_header_t *rel_children,
                         svn_boolean_t show_hidden,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool)
{
  apr_array_header_t *children;
  int i;

  children = apr_array_make(result_pool, rel_children->nelts,
                            sizeof(const char *));
  for (i = 0; i < rel_children->nelts; i++)
    {
      const char *child_abspath = svn_dirent_join(dir_abspath,
                                                  APR_ARRAY_IDX(rel_children,
                                                                i,
                                                                const char *),
                                                  result_pool);

      /* Don't add hidden nodes to *CHILDREN if we don't want them. */
      if (!show_hidden)
        {
          svn_boolean_t child_is_hidden;

          SVN_ERR(svn_wc__db_node_hidden(&child_is_hidden, wc_ctx->db,
                                         child_abspath, scratch_pool));
          if (child_is_hidden)
            continue;
        }

      APR_ARRAY_PUSH(children, const char *) = child_abspath;
    }

  *children_abspaths = children;

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc__node_get_children_of_working_node(const apr_array_header_t **children,
                                          svn_wc_context_t *wc_ctx,
                                          const char *dir_abspath,
                                          svn_boolean_t show_hidden,
                                          apr_pool_t *result_pool,
                                          apr_pool_t *scratch_pool)
{
  const apr_array_header_t *rel_children;

  SVN_ERR(svn_wc__db_read_children_of_working_node(&rel_children,
                                                   wc_ctx->db, dir_abspath,
                                                   scratch_pool, scratch_pool));
  SVN_ERR(filter_and_make_absolute(children, wc_ctx, dir_abspath,
                                   rel_children, show_hidden,
                                   result_pool, scratch_pool));
  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_get_children(const apr_array_header_t **children,
                          svn_wc_context_t *wc_ctx,
                          const char *dir_abspath,
                          svn_boolean_t show_hidden,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool)
{
  const apr_array_header_t *rel_children;

  SVN_ERR(svn_wc__db_read_children(&rel_children, wc_ctx->db, dir_abspath,
                                   scratch_pool, scratch_pool));
  SVN_ERR(filter_and_make_absolute(children, wc_ctx, dir_abspath,
                                   rel_children, show_hidden,
                                   result_pool, scratch_pool));
  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc__internal_get_repos_info(const char **repos_root_url,
                                const char **repos_uuid,
                                svn_wc__db_t *db,
                                const char *local_abspath,
                                apr_pool_t *result_pool,
                                apr_pool_t *scratch_pool)
{
  svn_error_t *err;
  svn_wc__db_status_t status;

  err = svn_wc__db_read_info(&status, NULL, NULL, NULL,
                             repos_root_url, repos_uuid,
                             NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL,
                             db, local_abspath,
                             result_pool, scratch_pool);
  if (err)
    {
      if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND
          && err->apr_err != SVN_ERR_WC_NOT_WORKING_COPY)
        return svn_error_trace(err);

      /* This node is not versioned. Return NULL repos info.  */
      svn_error_clear(err);

      if (repos_root_url)
        *repos_root_url = NULL;
      if (repos_uuid)
        *repos_uuid = NULL;
      return SVN_NO_ERROR;
    }

  if (((repos_root_url && *repos_root_url) || !repos_root_url)
      && ((repos_uuid && *repos_uuid) || !repos_uuid))
    return SVN_NO_ERROR;

  if (status == svn_wc__db_status_deleted)
    {
      const char *base_del_abspath, *wrk_del_abspath;

      SVN_ERR(svn_wc__db_scan_deletion(&base_del_abspath, NULL,
                                       &wrk_del_abspath,
                                       db, local_abspath,
                                       scratch_pool, scratch_pool));

      if (base_del_abspath)
        SVN_ERR(svn_wc__db_scan_base_repos(NULL,repos_root_url,
                                           repos_uuid, db, base_del_abspath,
                                           result_pool, scratch_pool));
      else if (wrk_del_abspath)
        SVN_ERR(svn_wc__db_scan_addition(NULL, NULL, NULL,
                                         repos_root_url, repos_uuid,
                                         NULL, NULL, NULL, NULL,
                                         db, svn_dirent_dirname(
                                                   wrk_del_abspath,
                                                   scratch_pool),
                                         result_pool, scratch_pool));
    }
  else if (status == svn_wc__db_status_added)
    {
      /* We have an addition. scan_addition() will find the intended
         repository location by scanning up the tree.  */
      SVN_ERR(svn_wc__db_scan_addition(NULL, NULL, NULL,
                                       repos_root_url, repos_uuid,
                                       NULL, NULL, NULL, NULL,
                                       db, local_abspath,
                                       result_pool, scratch_pool));
    }
  else
    SVN_ERR(svn_wc__db_scan_base_repos(NULL, repos_root_url, repos_uuid,
                                       db, local_abspath,
                                       result_pool, scratch_pool));

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_get_repos_info(const char **repos_root_url,
                            const char **repos_uuid,
                            svn_wc_context_t *wc_ctx,
                            const char *local_abspath,
                            apr_pool_t *result_pool,
                            apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__internal_get_repos_info(
            repos_root_url, repos_uuid, wc_ctx->db, local_abspath,
            result_pool, scratch_pool));
}

/* Convert DB_KIND into the appropriate NODE_KIND value.
 * If SHOW_HIDDEN is TRUE, report the node kind as found in the DB
 * even if DB_STATUS indicates that the node is hidden.
 * Else, return svn_kind_none for such nodes.
 *
 * ### This is a bit ugly. We should consider promoting svn_wc__db_kind_t
 * ### to the de-facto node kind type instead of converting between them
 * ### in non-backwards compat code.
 * ### See also comments at the definition of svn_wc__db_kind_t. */
static svn_error_t *
convert_db_kind_to_node_kind(svn_node_kind_t *node_kind,
                             svn_wc__db_kind_t db_kind,
                             svn_wc__db_status_t db_status,
                             svn_boolean_t show_hidden)
{
  switch (db_kind)
    {
      case svn_wc__db_kind_file:
        *node_kind = svn_node_file;
        break;
      case svn_wc__db_kind_dir:
        *node_kind = svn_node_dir;
        break;
      case svn_wc__db_kind_symlink:
        *node_kind = svn_node_file;
        break;
      case svn_wc__db_kind_unknown:
        *node_kind = svn_node_unknown;
        break;
      default:
        SVN_ERR_MALFUNCTION();
    }

  /* Make sure hidden nodes return svn_node_none. */
  if (! show_hidden)
    switch (db_status)
      {
        case svn_wc__db_status_not_present:
        case svn_wc__db_status_server_excluded:
        case svn_wc__db_status_excluded:
          *node_kind = svn_node_none;

        default:
          break;
      }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc_read_kind(svn_node_kind_t *kind,
                 svn_wc_context_t *wc_ctx,
                 const char *local_abspath,
                 svn_boolean_t show_hidden,
                 apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t db_status;
  svn_wc__db_kind_t db_kind;
  svn_error_t *err;

  err = svn_wc__db_read_info(&db_status, &db_kind, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL,
                             wc_ctx->db, local_abspath,
                             scratch_pool, scratch_pool);

  if (err && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
    {
      svn_error_clear(err);
      *kind = svn_node_none;
      return SVN_NO_ERROR;
    }
  else
    SVN_ERR(err);

  SVN_ERR(convert_db_kind_to_node_kind(kind, db_kind, db_status, show_hidden));

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_get_depth(svn_depth_t *depth,
                       svn_wc_context_t *wc_ctx,
                       const char *local_abspath,
                       apr_pool_t *scratch_pool)
{
  return svn_error_trace(
    svn_wc__db_read_info(NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, depth, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, NULL, NULL,
                         wc_ctx->db, local_abspath, scratch_pool,
                         scratch_pool));
}

svn_error_t *
svn_wc__node_get_changed_info(svn_revnum_t *changed_rev,
                              apr_time_t *changed_date,
                              const char **changed_author,
                              svn_wc_context_t *wc_ctx,
                              const char *local_abspath,
                              apr_pool_t *result_pool,
                              apr_pool_t *scratch_pool)
{
  return svn_error_trace(
    svn_wc__db_read_info(NULL, NULL, NULL, NULL, NULL, NULL, changed_rev,
                         changed_date, changed_author, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                         wc_ctx->db, local_abspath, result_pool,
                         scratch_pool));
}

svn_error_t *
svn_wc__node_get_url(const char **url,
                     svn_wc_context_t *wc_ctx,
                     const char *local_abspath,
                     apr_pool_t *result_pool,
                     apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__db_read_url(url, wc_ctx->db, local_abspath,
                                             result_pool, scratch_pool));
}

/* ### This is essentially a copy-paste of svn_wc__internal_get_url().
 * ### If we decide to keep this one, then it should be rewritten to avoid
 * ### code duplication.*/
svn_error_t *
svn_wc__node_get_repos_relpath(const char **repos_relpath,
                               svn_wc_context_t *wc_ctx,
                               const char *local_abspath,
                               apr_pool_t *result_pool,
                               apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;
  svn_boolean_t have_base;

  SVN_ERR(svn_wc__db_read_info(&status, NULL, NULL, repos_relpath,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL,
                               &have_base, NULL, NULL,
                               wc_ctx->db, local_abspath,
                               result_pool, scratch_pool));
  if (*repos_relpath == NULL)
    {
      if (status == svn_wc__db_status_added)
        {
          SVN_ERR(svn_wc__db_scan_addition(NULL, NULL, repos_relpath,
                                           NULL, NULL, NULL, NULL,
                                           NULL, NULL,
                                           wc_ctx->db, local_abspath,
                                           result_pool, scratch_pool));
        }
      else if (have_base)
        {
          SVN_ERR(svn_wc__db_scan_base_repos(repos_relpath, NULL,
                                             NULL,
                                             wc_ctx->db, local_abspath,
                                             result_pool, scratch_pool));
        }
      else if (status == svn_wc__db_status_excluded
               || (!have_base && (status == svn_wc__db_status_deleted)))
        {
          const char *parent_abspath, *name, *parent_relpath;

          svn_dirent_split(&parent_abspath, &name, local_abspath,
                           scratch_pool);
          SVN_ERR(svn_wc__node_get_repos_relpath(&parent_relpath, wc_ctx,
                                                 parent_abspath,
                                                 scratch_pool,
                                                 scratch_pool));

          if (parent_relpath)
            *repos_relpath = svn_relpath_join(parent_relpath, name,
                                              result_pool);
        }
      else
        {
          /* Status: obstructed, obstructed_add */
          *repos_relpath = NULL;
          return SVN_NO_ERROR;
        }
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__internal_get_copyfrom_info(const char **copyfrom_root_url,
                                   const char **copyfrom_repos_relpath,
                                   const char **copyfrom_url,
                                   svn_revnum_t *copyfrom_rev,
                                   svn_boolean_t *is_copy_target,
                                   svn_wc__db_t *db,
                                   const char *local_abspath,
                                   apr_pool_t *result_pool,
                                   apr_pool_t *scratch_pool)
{
  const char *original_root_url;
  const char *original_repos_relpath;
  svn_revnum_t original_revision;
  svn_wc__db_status_t status;

  if (copyfrom_root_url)
    *copyfrom_root_url = NULL;
  if (copyfrom_repos_relpath)
    *copyfrom_repos_relpath = NULL;
  if (copyfrom_url)
    *copyfrom_url = NULL;
  if (copyfrom_rev)
    *copyfrom_rev = SVN_INVALID_REVNUM;
  if (is_copy_target)
    *is_copy_target = FALSE;

  SVN_ERR(svn_wc__db_read_info(&status, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL,
                               &original_repos_relpath,
                               &original_root_url, NULL,
                               &original_revision,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL,
                               db, local_abspath, result_pool, scratch_pool));
  if (original_root_url && original_repos_relpath)
    {
      /* If this was the root of the copy then the URL is immediately
         available... */
      const char *my_copyfrom_url;

      if (copyfrom_url || is_copy_target)
        my_copyfrom_url = svn_path_url_add_component2(original_root_url,
                                                      original_repos_relpath,
                                                      result_pool);

      if (copyfrom_root_url)
        *copyfrom_root_url = original_root_url;
      if (copyfrom_repos_relpath)
        *copyfrom_repos_relpath = original_repos_relpath;
      if (copyfrom_url)
        *copyfrom_url = my_copyfrom_url;

      if (copyfrom_rev)
        *copyfrom_rev = original_revision;

      if (is_copy_target)
        {
          /* ### At this point we'd just set is_copy_target to TRUE, *but* we
           * currently want to model wc-1 behaviour.  Particularly, this
           * affects mixed-revision copies (e.g. wc-wc copy):
           * - Wc-1 saw only the root of a mixed-revision copy as the copy's
           *   root.
           * - Wc-ng returns an explicit original_root_url,
           *   original_repos_relpath pair for each subtree with mismatching
           *   revision.
           * We need to compensate for that: Find out if the parent of
           * this node is also copied and has a matching copy_from URL. If so,
           * nevermind the revision, just like wc-1 did, and say this was not
           * a separate copy target. */
          const char *parent_abspath;
          const char *base_name;
          const char *parent_copyfrom_url;

          svn_dirent_split(&parent_abspath, &base_name, local_abspath,
                           scratch_pool);

          /* This is a copied node, so we should never fall off the top of a
           * working copy here. */
          SVN_ERR(svn_wc__internal_get_copyfrom_info(NULL, NULL,
                                                     &parent_copyfrom_url,
                                                     NULL, NULL,
                                                     db, parent_abspath,
                                                     scratch_pool,
                                                     scratch_pool));

          /* So, count this as a separate copy target only if the URLs
           * don't match up, or if the parent isn't copied at all. */
          if (parent_copyfrom_url == NULL
              || strcmp(my_copyfrom_url,
                        svn_path_url_add_component2(parent_copyfrom_url,
                                                    base_name,
                                                    scratch_pool)) != 0)
            *is_copy_target = TRUE;
        }
    }
  else if ((status == svn_wc__db_status_added)
           && (copyfrom_rev || copyfrom_url || copyfrom_root_url
               || copyfrom_repos_relpath))
    {
      /* ...But if this is merely the descendant of an explicitly
         copied/moved directory, we need to do a bit more work to
         determine copyfrom_url and copyfrom_rev. */
      const char *op_root_abspath;

      SVN_ERR(svn_wc__db_scan_addition(&status, &op_root_abspath, NULL, NULL,
                                       NULL, &original_repos_relpath,
                                       &original_root_url, NULL,
                                       &original_revision, db, local_abspath,
                                       result_pool, scratch_pool));
      if (status == svn_wc__db_status_copied ||
          status == svn_wc__db_status_moved_here)
        {
          const char *src_parent_url;
          const char *src_relpath;

          src_parent_url = svn_path_url_add_component2(original_root_url,
                                                       original_repos_relpath,
                                                       scratch_pool);
          src_relpath = svn_dirent_is_child(op_root_abspath, local_abspath,
                                            scratch_pool);
          if (src_relpath)
            {
              if (copyfrom_root_url)
                *copyfrom_root_url = original_root_url;
              if (copyfrom_repos_relpath)
                *copyfrom_repos_relpath = svn_relpath_join(
                                            original_repos_relpath,
                                            src_relpath, result_pool);
              if (copyfrom_url)
                *copyfrom_url = svn_path_url_add_component2(src_parent_url,
                                                            src_relpath,
                                                            result_pool);
              if (copyfrom_rev)
                *copyfrom_rev = original_revision;
            }
        }
    }

  return SVN_NO_ERROR;
}


/* A recursive node-walker, helper for svn_wc__internal_walk_children().
 *
 * Call WALK_CALLBACK with WALK_BATON on all children (recursively) of
 * DIR_ABSPATH in DB, but not on DIR_ABSPATH itself. DIR_ABSPATH must be a
 * versioned directory. If SHOW_HIDDEN is true, visit hidden nodes, else
 * ignore them. Restrict the depth of the walk to DEPTH.
 *
 * ### Is it possible for a subdirectory to be hidden and known to be a
 *     directory?  If so, and if show_hidden is true, this will try to
 *     recurse into it.  */
static svn_error_t *
walker_helper(svn_wc__db_t *db,
              const char *dir_abspath,
              svn_boolean_t show_hidden,
              const apr_hash_t *changelist_filter,
              svn_wc__node_found_func_t walk_callback,
              void *walk_baton,
              svn_depth_t depth,
              svn_cancel_func_t cancel_func,
              void *cancel_baton,
              apr_pool_t *scratch_pool)
{
  apr_hash_t *rel_children_info;
  apr_hash_index_t *hi;
  apr_pool_t *iterpool;

  if (depth == svn_depth_empty)
    return SVN_NO_ERROR;

  SVN_ERR(svn_wc__db_read_children_walker_info(&rel_children_info, db,
                                               dir_abspath, scratch_pool,
                                               scratch_pool));


  iterpool = svn_pool_create(scratch_pool);
  for (hi = apr_hash_first(scratch_pool, rel_children_info);
       hi;
       hi = apr_hash_next(hi))
    {
      const char *child_name = svn__apr_hash_index_key(hi);
      struct svn_wc__db_walker_info_t *wi = svn__apr_hash_index_val(hi);
      svn_wc__db_kind_t child_kind = wi->kind;
      svn_wc__db_status_t child_status = wi->status;
      const char *child_abspath;

      svn_pool_clear(iterpool);

      /* See if someone wants to cancel this operation. */
      if (cancel_func)
        SVN_ERR(cancel_func(cancel_baton));

      child_abspath = svn_dirent_join(dir_abspath, child_name, iterpool);

      if (!show_hidden)
        switch (child_status)
          {
            case svn_wc__db_status_not_present:
            case svn_wc__db_status_server_excluded:
            case svn_wc__db_status_excluded:
              continue;
            default:
              break;
          }

      /* Return the child, if appropriate. */
      if ( (child_kind == svn_wc__db_kind_file
             || depth >= svn_depth_immediates)
           && svn_wc__internal_changelist_match(db, child_abspath,
                                                changelist_filter,
                                                scratch_pool) )
        {
          svn_node_kind_t kind;

          SVN_ERR(convert_db_kind_to_node_kind(&kind, child_kind,
                                               child_status, show_hidden));
          /* ### We might want to pass child_status as well because at least
           * ### one callee is asking for it.
           * ### But is it OK to use an svn_wc__db type in this API?
           * ###    Not yet, we need to get the node walker
           * ###    libsvn_wc-internal first. -hkw */
          SVN_ERR(walk_callback(child_abspath, kind, walk_baton, iterpool));
        }

      /* Recurse into this directory, if appropriate. */
      if (child_kind == svn_wc__db_kind_dir
            && depth >= svn_depth_immediates)
        {
          svn_depth_t depth_below_here = depth;

          if (depth == svn_depth_immediates)
            depth_below_here = svn_depth_empty;

          SVN_ERR(walker_helper(db, child_abspath, show_hidden,
                                changelist_filter,
                                walk_callback, walk_baton,
                                depth_below_here,
                                cancel_func, cancel_baton,
                                iterpool));
        }
    }

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc__internal_walk_children(svn_wc__db_t *db,
                               const char *local_abspath,
                               svn_boolean_t show_hidden,
                               const apr_array_header_t *changelist_filter,
                               svn_wc__node_found_func_t walk_callback,
                               void *walk_baton,
                               svn_depth_t walk_depth,
                               svn_cancel_func_t cancel_func,
                               void *cancel_baton,
                               apr_pool_t *scratch_pool)
{
  svn_wc__db_kind_t db_kind;
  svn_node_kind_t kind;
  svn_wc__db_status_t status;
  apr_hash_t *changelist_hash = NULL;

  SVN_ERR_ASSERT(walk_depth >= svn_depth_empty
                 && walk_depth <= svn_depth_infinity);

  if (changelist_filter && changelist_filter->nelts)
    SVN_ERR(svn_hash_from_cstring_keys(&changelist_hash, changelist_filter,
                                       scratch_pool));

  /* Check if the node exists before the first callback */
  SVN_ERR(svn_wc__db_read_info(&status, &db_kind, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               db, local_abspath, scratch_pool, scratch_pool));

  SVN_ERR(convert_db_kind_to_node_kind(&kind, db_kind, status, show_hidden));

  if (svn_wc__internal_changelist_match(db, local_abspath,
                                        changelist_hash, scratch_pool))
    SVN_ERR(walk_callback(local_abspath, kind, walk_baton, scratch_pool));

  if (db_kind == svn_wc__db_kind_file
      || status == svn_wc__db_status_not_present
      || status == svn_wc__db_status_excluded
      || status == svn_wc__db_status_server_excluded)
    return SVN_NO_ERROR;

  if (db_kind == svn_wc__db_kind_dir)
    {
      return svn_error_trace(
        walker_helper(db, local_abspath, show_hidden, changelist_hash,
                      walk_callback, walk_baton,
                      walk_depth, cancel_func, cancel_baton, scratch_pool));
    }

  return svn_error_createf(SVN_ERR_NODE_UNKNOWN_KIND, NULL,
                           _("'%s' has an unrecognized node kind"),
                           svn_dirent_local_style(local_abspath,
                                                  scratch_pool));
}

svn_error_t *
svn_wc__node_is_status_deleted(svn_boolean_t *is_deleted,
                               svn_wc_context_t *wc_ctx,
                               const char *local_abspath,
                               apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;

  SVN_ERR(svn_wc__db_read_info(&status,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL,
                               wc_ctx->db, local_abspath,
                               scratch_pool, scratch_pool));

  *is_deleted = (status == svn_wc__db_status_deleted);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_is_status_server_excluded(svn_boolean_t *is_server_excluded,
                                       svn_wc_context_t *wc_ctx,
                                       const char *local_abspath,
                                       apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;

  SVN_ERR(svn_wc__db_read_info(&status,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL,
                               wc_ctx->db, local_abspath,
                               scratch_pool, scratch_pool));
  *is_server_excluded = (status == svn_wc__db_status_server_excluded);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_is_status_not_present(svn_boolean_t *is_not_present,
                                   svn_wc_context_t *wc_ctx,
                                   const char *local_abspath,
                                   apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;

  SVN_ERR(svn_wc__db_read_info(&status,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL,
                               wc_ctx->db, local_abspath,
                               scratch_pool, scratch_pool));
  *is_not_present = (status == svn_wc__db_status_not_present);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_is_status_excluded(svn_boolean_t *is_excluded,
                                   svn_wc_context_t *wc_ctx,
                                   const char *local_abspath,
                                   apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;

  SVN_ERR(svn_wc__db_read_info(&status,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL,
                               wc_ctx->db, local_abspath,
                               scratch_pool, scratch_pool));
  *is_excluded = (status == svn_wc__db_status_excluded);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_is_added(svn_boolean_t *is_added,
                      svn_wc_context_t *wc_ctx,
                      const char *local_abspath,
                      apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;

  SVN_ERR(svn_wc__db_read_info(&status,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL,
                               wc_ctx->db, local_abspath,
                               scratch_pool, scratch_pool));
  *is_added = (status == svn_wc__db_status_added);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_has_working(svn_boolean_t *has_working,
                         svn_wc_context_t *wc_ctx,
                         const char *local_abspath,
                         apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;

  SVN_ERR(svn_wc__db_read_info(&status,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, has_working,
                               wc_ctx->db, local_abspath,
                               scratch_pool, scratch_pool));

  return SVN_NO_ERROR;
}


static svn_error_t *
get_base_rev(svn_revnum_t *base_revision,
             svn_wc__db_t *db,
             const char *local_abspath,
             apr_pool_t *scratch_pool)
{
  svn_boolean_t have_base;
  svn_error_t *err;

  err = svn_wc__db_base_get_info(NULL, NULL, base_revision, NULL,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 db, local_abspath,
                                 scratch_pool, scratch_pool);

  if (!err || err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
    return svn_error_trace(err);

  svn_error_clear(err);

  SVN_ERR(svn_wc__db_read_info(NULL, NULL, base_revision,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, &have_base, NULL,
                               NULL, NULL, NULL, NULL, NULL,
                               db, local_abspath,
                               scratch_pool, scratch_pool));

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_get_base_rev(svn_revnum_t *base_revision,
                          svn_wc_context_t *wc_ctx,
                          const char *local_abspath,
                          apr_pool_t *scratch_pool)
{
  return svn_error_trace(get_base_rev(base_revision, wc_ctx->db,
                                      local_abspath, scratch_pool));
}

svn_error_t *
svn_wc__node_get_pre_ng_status_data(svn_revnum_t *revision,
                                   svn_revnum_t *changed_rev,
                                   apr_time_t *changed_date,
                                   const char **changed_author,
                                   svn_wc_context_t *wc_ctx,
                                   const char *local_abspath,
                                   apr_pool_t *result_pool,
                                   apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;
  svn_boolean_t have_base, have_more_work, have_work;

  SVN_ERR(svn_wc__db_read_info(&status, NULL, revision, NULL, NULL, NULL,
                               changed_rev, changed_date, changed_author,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               &have_base, &have_more_work, &have_work,
                               wc_ctx->db, local_abspath,
                               result_pool, scratch_pool));

  if (!have_work
      || ((!changed_rev || SVN_IS_VALID_REVNUM(*changed_rev))
          && (!revision || SVN_IS_VALID_REVNUM(*revision)))
      || ((status != svn_wc__db_status_added)
          && (status != svn_wc__db_status_deleted)))
    {
      return SVN_NO_ERROR; /* We got everything we need */
    }

  if (have_base && !have_more_work)
    SVN_ERR(svn_wc__db_base_get_info(NULL, NULL, revision, NULL, NULL, NULL,
                                     changed_rev, changed_date, changed_author,
                                     NULL, NULL, NULL,
                                     NULL, NULL, NULL,
                                     wc_ctx->db, local_abspath,
                                     result_pool, scratch_pool));
  else
    {
      /* Sorry, we need a function to peek below the current working and
         the BASE layer. And we don't have one yet.

         ### Better to report nothing, than the wrong information */
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc__internal_get_commit_base_rev(svn_revnum_t *commit_base_revision,
                                     svn_wc__db_t *db,
                                     const char *local_abspath,
                                     apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;
  svn_boolean_t have_base;
  svn_boolean_t have_more_work;
  svn_revnum_t revision;
  svn_revnum_t original_revision;

  *commit_base_revision = SVN_INVALID_REVNUM;

  SVN_ERR(svn_wc__db_read_info(&status, NULL, &revision, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, &original_revision, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL,
                               &have_base, &have_more_work, NULL,
                               db, local_abspath, scratch_pool, scratch_pool));

  if (SVN_IS_VALID_REVNUM(revision))
    {
      /* We are looking directly at BASE */
      *commit_base_revision = revision;
      return SVN_NO_ERROR;
    }
  else if (SVN_IS_VALID_REVNUM(original_revision))
    {
      /* We are looking at a copied node */
      *commit_base_revision = original_revision;
      return SVN_NO_ERROR;
    }

  if (status == svn_wc__db_status_added)
    {
      /* If the node was copied/moved-here, return the copy/move source
         revision (not this node's base revision). */
      SVN_ERR(svn_wc__db_scan_addition(NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, commit_base_revision,
                                       db, local_abspath,
                                       scratch_pool, scratch_pool));


      if (SVN_IS_VALID_REVNUM(*commit_base_revision))
        return SVN_NO_ERROR;
      /* Fall through to handle simple replacements */
    }
  else if (status == svn_wc__db_status_deleted)
    {
      const char *work_del_abspath;

      SVN_ERR(svn_wc__db_scan_deletion(NULL, NULL,
                                       &work_del_abspath,
                                       db, local_abspath,
                                       scratch_pool, scratch_pool));
      if (work_del_abspath != NULL)
        {
          /* This is a deletion within a copied subtree. Get the copied-from
           * revision. */
          SVN_ERR(svn_wc__db_scan_addition(NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL,
                                           commit_base_revision,
                                           db,
                                           svn_dirent_dirname(work_del_abspath,
                                                              scratch_pool),
                                           scratch_pool, scratch_pool));

          SVN_ERR_ASSERT(SVN_IS_VALID_REVNUM(*commit_base_revision));

          return SVN_NO_ERROR;
        }
      /* else deletion of BASE node, fall through */
    }

  /* Catch replacement by local addition and deleted BASE nodes. */
  if (have_base && !have_more_work)
    {
      SVN_ERR(svn_wc__db_base_get_info(&status, NULL, commit_base_revision,
                                       NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL,
                                       db, local_abspath,
                                       scratch_pool, scratch_pool));

      if (status == svn_wc__db_status_not_present)
        *commit_base_revision = SVN_INVALID_REVNUM; /* No replacement */
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_get_commit_base_rev(svn_revnum_t *commit_base_revision,
                                 svn_wc_context_t *wc_ctx,
                                 const char *local_abspath,
                                 apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__internal_get_commit_base_rev(
                           commit_base_revision, wc_ctx->db, local_abspath,
                           scratch_pool));
}

svn_error_t *
svn_wc__node_get_lock_info(const char **lock_token,
                           const char **lock_owner,
                           const char **lock_comment,
                           apr_time_t *lock_date,
                           svn_wc_context_t *wc_ctx,
                           const char *local_abspath,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  svn_wc__db_lock_t *lock;
  svn_error_t *err;

  err = svn_wc__db_base_get_info(NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL, &lock, NULL,
                                 NULL,
                                 wc_ctx->db, local_abspath,
                                 result_pool, scratch_pool);

  if (err)
    {
      if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
        return svn_error_trace(err);

      svn_error_clear(err);
      lock = NULL;
    }
  if (lock_token)
    *lock_token = lock ? lock->token : NULL;
  if (lock_owner)
    *lock_owner = lock ? lock->owner : NULL;
  if (lock_comment)
    *lock_comment = lock ? lock->comment : NULL;
  if (lock_date)
    *lock_date = lock ? lock->date : 0;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__internal_node_get_schedule(svn_wc_schedule_t *schedule,
                                   svn_boolean_t *copied,
                                   svn_wc__db_t *db,
                                   const char *local_abspath,
                                   apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;
  svn_boolean_t op_root;
  svn_boolean_t have_base;
  svn_boolean_t have_work;
  svn_boolean_t have_more_work;
  const char *copyfrom_relpath;

  if (schedule)
    *schedule = svn_wc_schedule_normal;
  if (copied)
    *copied = FALSE;

  SVN_ERR(svn_wc__db_read_info(&status, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, &copyfrom_relpath,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               &op_root, NULL, NULL,
                               &have_base, &have_more_work, &have_work,
                               db, local_abspath, scratch_pool, scratch_pool));

  switch (status)
    {
      case svn_wc__db_status_not_present:
      case svn_wc__db_status_server_excluded:
      case svn_wc__db_status_excluded:
        /* We used status normal in the entries world. */
        if (schedule)
          *schedule = svn_wc_schedule_normal;
        break;
      case svn_wc__db_status_normal:
      case svn_wc__db_status_incomplete:
        break;

      case svn_wc__db_status_deleted:
        {
          if (schedule)
            *schedule = svn_wc_schedule_delete;

          if (!copied)
            break;

          if (have_more_work || !have_base)
            *copied = TRUE;
          else
            {
              const char *work_del_abspath;

              /* Find out details of our deletion.  */
              SVN_ERR(svn_wc__db_scan_deletion(NULL, NULL,
                                               &work_del_abspath,
                                               db, local_abspath,
                                               scratch_pool, scratch_pool));

              if (work_del_abspath)
                *copied = TRUE; /* Working deletion */
            }
          break;
        }
      case svn_wc__db_status_added:
        {
          if (!op_root)
            {
              if (copied)
                *copied = TRUE;

              if (schedule)
                *schedule = svn_wc_schedule_normal;

              break;
            }

          if (copied)
            *copied = (copyfrom_relpath != NULL);

          if (schedule)
            *schedule = svn_wc_schedule_add;
          else
            break;

          /* Check for replaced */
          if (have_base || have_more_work)
            {
              svn_wc__db_status_t below_working;
              SVN_ERR(svn_wc__db_info_below_working(&have_base, &have_work,
                                                    &below_working,
                                                    db, local_abspath,
                                                    scratch_pool));

              /* If the node is not present or deleted (read: not present
                 in working), then the node is not a replacement */
              if (below_working != svn_wc__db_status_not_present
                  && below_working != svn_wc__db_status_deleted)
                {
                  *schedule = svn_wc_schedule_replace;
                  break;
                }
            }
          break;
        }
      default:
        SVN_ERR_MALFUNCTION();
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_get_schedule(svn_wc_schedule_t *schedule,
                          svn_boolean_t *copied,
                          svn_wc_context_t *wc_ctx,
                          const char *local_abspath,
                          apr_pool_t *scratch_pool)
{
  return svn_error_trace(
           svn_wc__internal_node_get_schedule(schedule,
                                              copied,
                                              wc_ctx->db,
                                              local_abspath,
                                              scratch_pool));
}

svn_error_t *
svn_wc__node_get_conflict_info(const char **conflict_old,
                               const char **conflict_new,
                               const char **conflict_wrk,
                               const char **prejfile,
                               svn_wc_context_t *wc_ctx,
                               const char *local_abspath,
                               apr_pool_t *result_pool,
                               apr_pool_t *scratch_pool)
{
  svn_boolean_t conflicted;

  SVN_ERR(svn_wc__db_read_info(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, &conflicted,
                               NULL, NULL, NULL, NULL, NULL, NULL,
                               wc_ctx->db, local_abspath,
                               scratch_pool, scratch_pool));

  if (conflict_old)
    *conflict_old = NULL;
  if (conflict_new)
    *conflict_new = NULL;
  if (conflict_wrk)
    *conflict_wrk = NULL;
  if (prejfile)
    *prejfile = NULL;

  if (conflicted
      && (conflict_old || conflict_new || conflict_wrk || prejfile))
    {
      const apr_array_header_t *conflicts;
      int j;
      SVN_ERR(svn_wc__db_read_conflicts(&conflicts, wc_ctx->db, local_abspath,
                                        scratch_pool, scratch_pool));

      for (j = 0; j < conflicts->nelts; j++)
        {
          const svn_wc_conflict_description2_t *cd;
          cd = APR_ARRAY_IDX(conflicts, j,
                             const svn_wc_conflict_description2_t *);

          switch (cd->kind)
            {
            case svn_wc_conflict_kind_text:
              if (conflict_old)
                *conflict_old = svn_dirent_basename(cd->base_abspath,
                                                    result_pool);

              if (conflict_new)
                *conflict_new = svn_dirent_basename(cd->their_abspath,
                                                    result_pool);

              if (conflict_wrk)
                *conflict_wrk = svn_dirent_basename(cd->my_abspath,
                                                    result_pool);
              break;

            case svn_wc_conflict_kind_property:
              if (prejfile)
                *prejfile = svn_dirent_basename(cd->their_abspath,
                                                result_pool);
              break;
            case svn_wc_conflict_kind_tree:
              break;
            }
        }
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_clear_dav_cache_recursive(svn_wc_context_t *wc_ctx,
                                       const char *local_abspath,
                                       apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__db_base_clear_dav_cache_recursive(
                              wc_ctx->db, local_abspath, scratch_pool));
}


svn_error_t *
svn_wc__node_get_lock_tokens_recursive(apr_hash_t **lock_tokens,
                                       svn_wc_context_t *wc_ctx,
                                       const char *local_abspath,
                                       apr_pool_t *result_pool,
                                       apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__db_base_get_lock_tokens_recursive(
                              lock_tokens, wc_ctx->db, local_abspath,
                              result_pool, scratch_pool));
}

svn_error_t *
svn_wc__get_server_excluded_subtrees(apr_hash_t **server_excluded_subtrees,
                                     svn_wc_context_t *wc_ctx,
                                     const char *local_abspath,
                                     apr_pool_t *result_pool,
                                     apr_pool_t *scratch_pool)
{
  return svn_error_trace(
           svn_wc__db_get_server_excluded_subtrees(server_excluded_subtrees,
                                                   wc_ctx->db,
                                                   local_abspath,
                                                   result_pool,
                                                   scratch_pool));
}

svn_error_t *
svn_wc__internal_get_origin(svn_boolean_t *is_copy,
                            svn_revnum_t *revision,
                            const char **repos_relpath,
                            const char **repos_root_url,
                            const char **repos_uuid,
                            const char **copy_root_abspath,
                            svn_wc__db_t *db,
                            const char *local_abspath,
                            svn_boolean_t scan_deleted,
                            apr_pool_t *result_pool,
                            apr_pool_t *scratch_pool)
{
  const char *original_repos_relpath;
  const char *original_repos_root_url;
  const char *original_repos_uuid;
  svn_revnum_t original_revision;
  svn_wc__db_status_t status;

  const char *tmp_repos_relpath;

  if (!repos_relpath)
    repos_relpath = &tmp_repos_relpath;

  SVN_ERR(svn_wc__db_read_info(&status, NULL, revision, repos_relpath,
                               repos_root_url, repos_uuid, NULL, NULL, NULL,
                               NULL, NULL, NULL,
                               &original_repos_relpath,
                               &original_repos_root_url,
                               &original_repos_uuid, &original_revision,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, is_copy,
                               db, local_abspath, result_pool, scratch_pool));

  if (*repos_relpath)
    {
      return SVN_NO_ERROR; /* Returned BASE information */
    }

  if (status == svn_wc__db_status_deleted && !scan_deleted)
    {
      if (is_copy)
        *is_copy = FALSE; /* Deletes are stored in working; default to FALSE */

      return SVN_NO_ERROR; /* No info */
    }

  if (original_repos_relpath)
    {
      *repos_relpath = original_repos_relpath;
      if (revision)
        *revision = original_revision;
      if (repos_root_url)
        *repos_root_url = original_repos_root_url;
      if (repos_uuid)
        *repos_uuid = original_repos_uuid;

      if (copy_root_abspath == NULL)
        return SVN_NO_ERROR;
    }

  {
    svn_boolean_t scan_working = FALSE;

    if (status == svn_wc__db_status_added)
      scan_working = TRUE;
    else if (status == svn_wc__db_status_deleted)
      {
        svn_boolean_t have_base;
        /* Is this a BASE or a WORKING delete? */
        SVN_ERR(svn_wc__db_info_below_working(&have_base, &scan_working,
                                              &status, db, local_abspath,
                                              scratch_pool));
      }

    if (scan_working)
      {
        const char *op_root_abspath;

        SVN_ERR(svn_wc__db_scan_addition(&status, &op_root_abspath, NULL,
                                         NULL, NULL, &original_repos_relpath,
                                         repos_root_url,
                                         repos_uuid,
                                         revision, db, local_abspath,
                                         result_pool, scratch_pool));

        if (status == svn_wc__db_status_added)
          {
            if (is_copy)
              *is_copy = FALSE;
            return SVN_NO_ERROR; /* Local addition */
          }

        /* We don't know how the following error condition can be fulfilled
         * but we have seen that happening in the wild.  Better to create
         * an error than a SEGFAULT. */
        if (status == svn_wc__db_status_incomplete && !original_repos_relpath)
          return svn_error_createf(SVN_ERR_WC_PATH_UNEXPECTED_STATUS, NULL,
                               _("Incomplete copy information on path '%s'."),
                                   svn_dirent_local_style(local_abspath,
                                                          scratch_pool));

        *repos_relpath = svn_relpath_join(
                                original_repos_relpath,
                                svn_dirent_skip_ancestor(op_root_abspath,
                                                         local_abspath),
                                result_pool);
        if (copy_root_abspath)
          *copy_root_abspath = op_root_abspath;
      }
    else /* Deleted, excluded, not-present, server-excluded, ... */
      {
        if (is_copy)
          *is_copy = FALSE;

        SVN_ERR(svn_wc__db_base_get_info(NULL, NULL, revision, repos_relpath,
                                         repos_root_url, repos_uuid, NULL,
                                         NULL, NULL, NULL, NULL, NULL, NULL,
                                         NULL, NULL,
                                         db, local_abspath,
                                         result_pool, scratch_pool));
      }

    return SVN_NO_ERROR;
  }
}

svn_error_t *
svn_wc__node_get_origin(svn_boolean_t *is_copy,
                        svn_revnum_t *revision,
                        const char **repos_relpath,
                        const char **repos_root_url,
                        const char **repos_uuid,
                        const char **copy_root_abspath,
                        svn_wc_context_t *wc_ctx,
                        const char *local_abspath,
                        svn_boolean_t scan_deleted,
                        apr_pool_t *result_pool,
                        apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__internal_get_origin(is_copy, revision,
                           repos_relpath, repos_root_url, repos_uuid,
                           copy_root_abspath,
                           wc_ctx->db, local_abspath, scan_deleted,
                           result_pool, scratch_pool));
}

svn_error_t *
svn_wc__node_get_commit_status(svn_node_kind_t *kind,
                               svn_boolean_t *added,
                               svn_boolean_t *deleted,
                               svn_boolean_t *is_replace_root,
                               svn_boolean_t *not_present,
                               svn_boolean_t *excluded,
                               svn_boolean_t *is_op_root,
                               svn_boolean_t *symlink,
                               svn_revnum_t *revision,
                               const char **repos_relpath,
                               svn_revnum_t *original_revision,
                               const char **original_repos_relpath,
                               svn_boolean_t *conflicted,
                               const char **changelist,
                               svn_boolean_t *props_mod,
                               svn_boolean_t *update_root,
                               const char **lock_token,
                               svn_wc_context_t *wc_ctx,
                               const char *local_abspath,
                               apr_pool_t *result_pool,
                               apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;
  svn_wc__db_kind_t db_kind;
  svn_wc__db_lock_t *lock;
  svn_boolean_t had_props;
  svn_boolean_t props_mod_tmp;
  svn_boolean_t have_base;
  svn_boolean_t have_more_work;
  svn_boolean_t op_root;

  if (!props_mod)
    props_mod = &props_mod_tmp;

  /* ### All of this should be handled inside a single read transaction */
  SVN_ERR(svn_wc__db_read_info(&status, &db_kind, revision, repos_relpath,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               original_repos_relpath, NULL, NULL,
                               original_revision, &lock, NULL, NULL,
                               changelist, conflicted,
                               &op_root, &had_props, props_mod,
                               &have_base, &have_more_work, NULL,
                               wc_ctx->db, local_abspath,
                               result_pool, scratch_pool));

  if (kind)
    {
      if (db_kind == svn_wc__db_kind_file)
        *kind = svn_node_file;
      else if (db_kind == svn_wc__db_kind_dir)
        *kind = svn_node_dir;
      else
        *kind = svn_node_unknown;
    }
  if (added)
    *added = (status == svn_wc__db_status_added);
  if (deleted)
    *deleted = (status == svn_wc__db_status_deleted);
  if (not_present)
    *not_present = (status == svn_wc__db_status_not_present);
  if (excluded)
    *excluded = (status == svn_wc__db_status_excluded);
  if (is_op_root)
    *is_op_root = op_root;

  if (is_replace_root)
    {
      if (status == svn_wc__db_status_added
          && op_root
          && (have_base || have_more_work))
        SVN_ERR(svn_wc__db_node_check_replace(is_replace_root, NULL, NULL,
                                              wc_ctx->db, local_abspath,
                                              scratch_pool));
      else
        *is_replace_root = FALSE;
    }

  if (symlink)
    {
      apr_hash_t *props;
      *symlink = FALSE;

      if (db_kind == svn_wc__db_kind_file
          && (had_props || *props_mod))
        {
          SVN_ERR(svn_wc__db_read_props(&props, wc_ctx->db, local_abspath,
                                        scratch_pool, scratch_pool));

          *symlink = ((props != NULL)
                      && (apr_hash_get(props, SVN_PROP_SPECIAL,
                                       APR_HASH_KEY_STRING) != NULL));
        }
    }

  /* Retrieve some information from BASE which is needed for replacing
     and/or deleting BASE nodes. (We don't need lock here) */
  if (have_base
      && ((revision && !SVN_IS_VALID_REVNUM(*revision))
          || (update_root && status == svn_wc__db_status_normal)))
    {
      SVN_ERR(svn_wc__db_base_get_info(NULL, NULL, revision, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, update_root,
                                       wc_ctx->db, local_abspath,
                                       scratch_pool, scratch_pool));
    }
  else if (update_root)
    *update_root = FALSE;

  if (lock_token)
    *lock_token = lock ? lock->token : NULL;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__node_get_md5_from_sha1(const svn_checksum_t **md5_checksum,
                               svn_wc_context_t *wc_ctx,
                               const char *wri_abspath,
                               const svn_checksum_t *sha1_checksum,
                               apr_pool_t *result_pool,
                               apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__db_pristine_get_md5(md5_checksum,
                                                     wc_ctx->db,
                                                     wri_abspath,
                                                     sha1_checksum,
                                                     result_pool,
                                                     scratch_pool));
}

svn_error_t *
svn_wc__get_not_present_descendants(const apr_array_header_t **descendants,
                                    svn_wc_context_t *wc_ctx,
                                    const char *local_abspath,
                                    apr_pool_t *result_pool,
                                    apr_pool_t *scratch_pool)
{
  return svn_error_trace(
                svn_wc__db_get_not_present_descendants(descendants,
                                                       wc_ctx->db,
                                                       local_abspath,
                                                       result_pool,
                                                       scratch_pool));
}

svn_error_t *
svn_wc__rename_wc(svn_wc_context_t *wc_ctx,
                  const char *from_abspath,
                  const char *dst_abspath,
                  apr_pool_t *scratch_pool)
{
  const char *wcroot_abspath;
  SVN_ERR(svn_wc__db_get_wcroot(&wcroot_abspath, wc_ctx->db, from_abspath,
                                scratch_pool, scratch_pool));

  if (! strcmp(from_abspath, wcroot_abspath))
    {
      SVN_ERR(svn_wc__db_drop_root(wc_ctx->db, wcroot_abspath, scratch_pool));

      SVN_ERR(svn_io_file_rename(from_abspath, dst_abspath, scratch_pool));
    }
  else
    return svn_error_createf(
                    SVN_ERR_WC_PATH_UNEXPECTED_STATUS, NULL,
                    _("'%s' is not the root of the working copy '%s'"),
                    svn_dirent_local_style(from_abspath, scratch_pool),
                    svn_dirent_local_style(wcroot_abspath, scratch_pool));

  return SVN_NO_ERROR;
}

svn_error_t *
svn_wc__check_for_obstructions(svn_wc_notify_state_t *obstruction_state,
                               svn_node_kind_t *kind,
                               svn_boolean_t *added,
                               svn_boolean_t *deleted,
                               svn_boolean_t *conflicted,
                               svn_wc_context_t *wc_ctx,
                               const char *local_abspath,
                               svn_boolean_t no_wcroot_check,
                               apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;
  svn_wc__db_kind_t db_kind;
  svn_node_kind_t disk_kind;
  svn_boolean_t conflicted_p;
  svn_error_t *err;

  *obstruction_state = svn_wc_notify_state_inapplicable;
  if (kind)
    *kind = svn_node_none;
  if (added)
    *added = FALSE;
  if (deleted)
    *deleted = FALSE;
  if (conflicted)
    *conflicted = FALSE;

  SVN_ERR(svn_io_check_path(local_abspath, &disk_kind, scratch_pool));

  err = svn_wc__db_read_info(&status, &db_kind, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, &conflicted_p, NULL,
                             NULL, NULL, NULL, NULL, NULL,
                             wc_ctx->db, local_abspath,
                             scratch_pool, scratch_pool);

  if (err && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
    {
      svn_error_clear(err);

      if (disk_kind != svn_node_none)
        {
          /* Nothing in the DB, but something on disk */
          *obstruction_state = svn_wc_notify_state_obstructed;
          return SVN_NO_ERROR;
        }

      err = svn_wc__db_read_info(&status, &db_kind, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 wc_ctx->db, svn_dirent_dirname(local_abspath,
                                                                scratch_pool),
                                 scratch_pool, scratch_pool);

      if (err && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
        {
          svn_error_clear(err);
          /* No versioned parent; we can't add a node here */
          *obstruction_state = svn_wc_notify_state_obstructed;
          return SVN_NO_ERROR;
        }
      else
        SVN_ERR(err);

      if (db_kind != svn_wc__db_kind_dir
          || (status != svn_wc__db_status_normal
              && status != svn_wc__db_status_added))
        {
          /* The parent doesn't allow nodes to be added below it */
          *obstruction_state = svn_wc_notify_state_obstructed;
        }

      return SVN_NO_ERROR;
    }
  else
    SVN_ERR(err);

  /* Check for obstructing working copies */
  if (!no_wcroot_check
      && db_kind == svn_wc__db_kind_dir
      && status == svn_wc__db_status_normal)
    {
      svn_boolean_t is_root;
      SVN_ERR(svn_wc__db_is_wcroot(&is_root, wc_ctx->db, local_abspath,
                                   scratch_pool));

      if (is_root)
        {
          /* Callers should handle this as unversioned */
          *obstruction_state = svn_wc_notify_state_obstructed;
          return SVN_NO_ERROR;
        }
    }

  if (kind)
    SVN_ERR(convert_db_kind_to_node_kind(kind, db_kind, status, FALSE));

  switch (status)
    {
      case svn_wc__db_status_deleted:
        if (deleted)
          *deleted = TRUE;
        /* Fall through to svn_wc__db_status_not_present */
      case svn_wc__db_status_not_present:
        if (disk_kind != svn_node_none)
          *obstruction_state = svn_wc_notify_state_obstructed;
        break;

      case svn_wc__db_status_excluded:
      case svn_wc__db_status_server_excluded:
      case svn_wc__db_status_incomplete:
        *obstruction_state = svn_wc_notify_state_missing;
        break;

      case svn_wc__db_status_added:
        if (added)
          *added = TRUE;
        /* Fall through to svn_wc__db_status_normal */
      case svn_wc__db_status_normal:
        if (disk_kind == svn_node_none)
          *obstruction_state = svn_wc_notify_state_missing;
        else
          {
            svn_node_kind_t expected_kind;

            SVN_ERR(convert_db_kind_to_node_kind(&expected_kind, db_kind,
                                                 status, FALSE));

            if (disk_kind != expected_kind)
              *obstruction_state = svn_wc_notify_state_obstructed;
          }
        break;
      default:
        SVN_ERR_MALFUNCTION();
    }

  if (conflicted_p && (conflicted != NULL))
    {
      svn_boolean_t text_c, prop_c, tree_c;

      SVN_ERR(svn_wc__internal_conflicted_p(&text_c, &prop_c, &tree_c,
                                            wc_ctx->db, local_abspath,
                                            scratch_pool));

      *conflicted = (text_c || prop_c || tree_c);
    }

  return SVN_NO_ERROR;
}

