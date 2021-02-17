/*
 * conflicts.c: routines for managing conflict data.
 *            NOTE: this code doesn't know where the conflict is
 *            actually stored.
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



#include <string.h>

#include <apr_pools.h>
#include <apr_tables.h>
#include <apr_hash.h>
#include <apr_errno.h>

#include "svn_types.h"
#include "svn_pools.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_wc.h"
#include "svn_io.h"
#include "svn_diff.h"

#include "wc.h"
#include "wc_db.h"
#include "conflicts.h"

#include "private/svn_wc_private.h"
#include "private/svn_skel.h"

#include "svn_private_config.h"

svn_skel_t *
svn_wc__conflict_skel_new(apr_pool_t *result_pool)
{
  svn_skel_t *operation = svn_skel__make_empty_list(result_pool);
  svn_skel_t *result = svn_skel__make_empty_list(result_pool);

  svn_skel__prepend(operation, result);
  return result;
}


static void
prepend_prop_value(const svn_string_t *value,
                   svn_skel_t *skel,
                   apr_pool_t *result_pool)
{
  svn_skel_t *value_skel = svn_skel__make_empty_list(result_pool);

  if (value != NULL)
    {
      const void *dup = apr_pmemdup(result_pool, value->data, value->len);

      svn_skel__prepend(svn_skel__mem_atom(dup, value->len, result_pool),
                        value_skel);
    }

  svn_skel__prepend(value_skel, skel);
}


svn_error_t *
svn_wc__conflict_skel_add_prop_conflict(
  svn_skel_t *skel,
  const char *prop_name,
  const svn_string_t *original_value,
  const svn_string_t *mine_value,
  const svn_string_t *incoming_value,
  const svn_string_t *incoming_base_value,
  apr_pool_t *result_pool,
  apr_pool_t *scratch_pool)
{
  svn_skel_t *prop_skel = svn_skel__make_empty_list(result_pool);

  /* ### check that OPERATION has been filled in.  */

  /* See notes/wc-ng/conflict-storage  */
  prepend_prop_value(incoming_base_value, prop_skel, result_pool);
  prepend_prop_value(incoming_value, prop_skel, result_pool);
  prepend_prop_value(mine_value, prop_skel, result_pool);
  prepend_prop_value(original_value, prop_skel, result_pool);
  svn_skel__prepend_str(apr_pstrdup(result_pool, prop_name), prop_skel,
                        result_pool);
  svn_skel__prepend_str(SVN_WC__CONFLICT_KIND_PROP, prop_skel, result_pool);

  /* Now we append PROP_SKEL to the end of the provided conflict SKEL.  */
  svn_skel__append(skel, prop_skel);

  return SVN_NO_ERROR;
}




/*** Resolving a conflict automatically ***/


/* Helper for resolve_conflict_on_entry.  Delete the file FILE_ABSPATH
   in if it exists.  Set WAS_PRESENT to TRUE if the file existed, and
   leave it UNTOUCHED otherwise. */
static svn_error_t *
attempt_deletion(const char *file_abspath,
                 svn_boolean_t *was_present,
                 apr_pool_t *scratch_pool)
{
  svn_error_t *err;

  if (file_abspath == NULL)
    return SVN_NO_ERROR;

  err = svn_io_remove_file2(file_abspath, FALSE, scratch_pool);

  if (err == NULL || !APR_STATUS_IS_ENOENT(err->apr_err))
    {
      *was_present = TRUE;
      return svn_error_trace(err);
    }

  svn_error_clear(err);
  return SVN_NO_ERROR;
}


/* Conflict resolution involves removing the conflict files, if they exist,
   and clearing the conflict filenames from the entry.  The latter needs to
   be done whether or not the conflict files exist.

   Tree conflicts are not resolved here, because the data stored in one
   entry does not refer to that entry but to children of it.

   PATH is the path to the item to be resolved, BASE_NAME is the basename
   of PATH, and CONFLICT_DIR is the access baton for PATH.  ORIG_ENTRY is
   the entry prior to resolution. RESOLVE_TEXT and RESOLVE_PROPS are TRUE
   if text and property conflicts respectively are to be resolved.

   If this call marks any conflict as resolved, set *DID_RESOLVE to true,
   else do not change *DID_RESOLVE.

   See svn_wc_resolved_conflict5() for how CONFLICT_CHOICE behaves.

   ### FIXME: This function should be loggy, otherwise an interruption can
   ### leave, for example, one of the conflict artifact files deleted but
   ### the entry still referring to it and trying to use it for the next
   ### attempt at resolving.

   ### Does this still apply in the world of WC-NG?  -hkw
*/
static svn_error_t *
resolve_conflict_on_node(svn_wc__db_t *db,
                         const char *local_abspath,
                         svn_boolean_t resolve_text,
                         svn_boolean_t resolve_props,
                         svn_wc_conflict_choice_t conflict_choice,
                         svn_boolean_t *did_resolve,
                         apr_pool_t *pool)
{
  svn_boolean_t found_file;
  const char *conflict_old = NULL;
  const char *conflict_new = NULL;
  const char *conflict_working = NULL;
  const char *prop_reject_file = NULL;
  svn_wc__db_kind_t kind;
  int i;
  const apr_array_header_t *conflicts;
  const char *conflict_dir_abspath;

  *did_resolve = FALSE;

  SVN_ERR(svn_wc__db_read_kind(&kind, db, local_abspath, TRUE, pool));
  SVN_ERR(svn_wc__db_read_conflicts(&conflicts, db, local_abspath,
                                    pool, pool));

  for (i = 0; i < conflicts->nelts; i++)
    {
      const svn_wc_conflict_description2_t *desc;

      desc = APR_ARRAY_IDX(conflicts, i,
                           const svn_wc_conflict_description2_t*);

      if (desc->kind == svn_wc_conflict_kind_text)
        {
          conflict_old = desc->base_abspath;
          conflict_new = desc->their_abspath;
          conflict_working = desc->my_abspath;
        }
      else if (desc->kind == svn_wc_conflict_kind_property)
        prop_reject_file = desc->their_abspath;
    }

  if (kind == svn_wc__db_kind_dir)
    conflict_dir_abspath = local_abspath;
  else
    conflict_dir_abspath = svn_dirent_dirname(local_abspath, pool);

  if (resolve_text)
    {
      const char *auto_resolve_src;

      /* Handle automatic conflict resolution before the temporary files are
       * deleted, if necessary. */
      switch (conflict_choice)
        {
        case svn_wc_conflict_choose_base:
          auto_resolve_src = conflict_old;
          break;
        case svn_wc_conflict_choose_mine_full:
          auto_resolve_src = conflict_working;
          break;
        case svn_wc_conflict_choose_theirs_full:
          auto_resolve_src = conflict_new;
          break;
        case svn_wc_conflict_choose_merged:
          auto_resolve_src = NULL;
          break;
        case svn_wc_conflict_choose_theirs_conflict:
        case svn_wc_conflict_choose_mine_conflict:
          {
            if (conflict_old && conflict_working && conflict_new)
              {
                const char *temp_dir;
                svn_stream_t *tmp_stream = NULL;
                svn_diff_t *diff;
                svn_diff_conflict_display_style_t style =
                  conflict_choice == svn_wc_conflict_choose_theirs_conflict
                  ? svn_diff_conflict_display_latest
                  : svn_diff_conflict_display_modified;

                SVN_ERR(svn_wc__db_temp_wcroot_tempdir(&temp_dir, db,
                                                       conflict_dir_abspath,
                                                       pool, pool));
                SVN_ERR(svn_stream_open_unique(&tmp_stream,
                                               &auto_resolve_src,
                                               temp_dir,
                                               svn_io_file_del_on_pool_cleanup,
                                               pool, pool));

                SVN_ERR(svn_diff_file_diff3_2(&diff,
                                              conflict_old,
                                              conflict_working,
                                              conflict_new,
                                              svn_diff_file_options_create(pool),
                                              pool));
                SVN_ERR(svn_diff_file_output_merge2(tmp_stream, diff,
                                                    conflict_old,
                                                    conflict_working,
                                                    conflict_new,
                                                    /* markers ignored */
                                                    NULL, NULL, NULL, NULL,
                                                    style,
                                                    pool));
                SVN_ERR(svn_stream_close(tmp_stream));
              }
            else
              auto_resolve_src = NULL;
            break;
          }
        default:
          return svn_error_create(SVN_ERR_INCORRECT_PARAMS, NULL,
                                  _("Invalid 'conflict_result' argument"));
        }

      if (auto_resolve_src)
        SVN_ERR(svn_io_copy_file(
          svn_dirent_join(conflict_dir_abspath, auto_resolve_src, pool),
          local_abspath, TRUE, pool));
    }

  /* Records whether we found any of the conflict files.  */
  found_file = FALSE;

  if (resolve_text)
    {
      SVN_ERR(attempt_deletion(conflict_old, &found_file, pool));
      SVN_ERR(attempt_deletion(conflict_new, &found_file, pool));
      SVN_ERR(attempt_deletion(conflict_working, &found_file, pool));
      resolve_text = conflict_old || conflict_new || conflict_working;
    }
  if (resolve_props)
    {
      if (prop_reject_file != NULL)
        SVN_ERR(attempt_deletion(prop_reject_file, &found_file, pool));
      else
        resolve_props = FALSE;
    }

  if (resolve_text || resolve_props)
    {
      SVN_ERR(svn_wc__db_op_mark_resolved(db, local_abspath,
                                          resolve_text, resolve_props,
                                          FALSE, pool));

      /* No feedback if no files were deleted and all we did was change the
         entry, such a file did not appear as a conflict */
      if (found_file)
        *did_resolve = TRUE;
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc__resolve_text_conflict(svn_wc__db_t *db,
                              const char *local_abspath,
                              apr_pool_t *scratch_pool)
{
  svn_boolean_t ignored_result;

  return svn_error_trace(resolve_conflict_on_node(
                           db, local_abspath,
                           TRUE /* resolve_text */,
                           FALSE /* resolve_props */,
                           svn_wc_conflict_choose_merged,
                           &ignored_result,
                           scratch_pool));
}


/* */
static svn_error_t *
resolve_one_conflict(svn_wc__db_t *db,
                     const char *local_abspath,
                     svn_boolean_t resolve_text,
                     const char *resolve_prop,
                     svn_boolean_t resolve_tree,
                     svn_wc_conflict_choice_t conflict_choice,
                     svn_wc_notify_func2_t notify_func,
                     void *notify_baton,
                     apr_pool_t *scratch_pool)
{
  const apr_array_header_t *conflicts;
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  int i;
  svn_boolean_t resolved = FALSE;

  SVN_ERR(svn_wc__db_read_conflicts(&conflicts, db, local_abspath,
                                    scratch_pool, iterpool));

  for (i = 0; i < conflicts->nelts; i++)
    {
      const svn_wc_conflict_description2_t *cd;
      svn_boolean_t did_resolve;

      cd = APR_ARRAY_IDX(conflicts, i, const svn_wc_conflict_description2_t *);

      svn_pool_clear(iterpool);

      switch (cd->kind)
        {
          case svn_wc_conflict_kind_tree:
            if (!resolve_tree)
              break;

            /* For now, we only clear tree conflict information and resolve
             * to the working state. There is no way to pick theirs-full
             * or mine-full, etc. Throw an error if the user expects us
             * to be smarter than we really are. */
            if (conflict_choice != svn_wc_conflict_choose_merged)
              {
                return svn_error_createf(SVN_ERR_WC_CONFLICT_RESOLVER_FAILURE,
                                         NULL,
                                         _("Tree conflicts can only be "
                                           "resolved to 'working' state; "
                                           "'%s' not resolved"),
                                         svn_dirent_local_style(local_abspath,
                                                                iterpool));
              }

            SVN_ERR(svn_wc__db_op_set_tree_conflict(db, local_abspath, NULL,
                                                    iterpool));

            resolved = TRUE;
            break;

          case svn_wc_conflict_kind_text:
            if (!resolve_text)
              break;

            SVN_ERR(resolve_conflict_on_node(db,
                                             local_abspath,
                                             TRUE /* resolve_text */,
                                             FALSE /* resolve_props */,
                                             conflict_choice,
                                             &did_resolve,
                                             iterpool));

            if (did_resolve)
              resolved = TRUE;
            break;

          case svn_wc_conflict_kind_property:
            if (!resolve_prop)
              break;

            /* ### this is bogus. resolve_conflict_on_node() does not handle
               ### individual property resolution.  */
            if (*resolve_prop != '\0' &&
                strcmp(resolve_prop, cd->property_name) != 0)
              {
                break; /* Skip this property conflict */
              }


            /* We don't have property name handling here yet :( */
            SVN_ERR(resolve_conflict_on_node(db,
                                             local_abspath,
                                             FALSE /* resolve_text */,
                                             TRUE /* resolve_props */,
                                             conflict_choice,
                                             &did_resolve,
                                             iterpool));

            if (did_resolve)
              resolved = TRUE;
            break;

          default:
            /* We can't resolve other conflict types */
            break;
        }
    }

  /* Notify */
  if (notify_func && resolved)
    notify_func(notify_baton,
                svn_wc_create_notify(local_abspath, svn_wc_notify_resolved,
                                     iterpool),
                iterpool);

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

/* */
static svn_error_t *
recursive_resolve_conflict(svn_wc__db_t *db,
                           const char *local_abspath,
                           svn_boolean_t this_is_conflicted,
                           svn_depth_t depth,
                           svn_boolean_t resolve_text,
                           const char *resolve_prop,
                           svn_boolean_t resolve_tree,
                           svn_wc_conflict_choice_t conflict_choice,
                           svn_cancel_func_t cancel_func,
                           void *cancel_baton,
                           svn_wc_notify_func2_t notify_func,
                           void *notify_baton,
                           apr_pool_t *scratch_pool)
{
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  const apr_array_header_t *children;
  apr_hash_t *visited = apr_hash_make(scratch_pool);
  svn_depth_t child_depth;
  int i;

  if (cancel_func)
    SVN_ERR(cancel_func(cancel_baton));

  if (this_is_conflicted)
    {
      SVN_ERR(resolve_one_conflict(db,
                                   local_abspath,
                                   resolve_text,
                                   resolve_prop,
                                   resolve_tree,
                                   conflict_choice,
                                   notify_func, notify_baton,
                                   iterpool));
    }

  if (depth < svn_depth_files)
    return SVN_NO_ERROR;

  child_depth = (depth < svn_depth_infinity) ? svn_depth_empty : depth;

  SVN_ERR(svn_wc__db_read_children(&children, db, local_abspath,
                                   scratch_pool, iterpool));

  for (i = 0; i < children->nelts; i++)
    {
      const char *name = APR_ARRAY_IDX(children, i, const char *);
      const char *child_abspath;
      svn_wc__db_status_t status;
      svn_wc__db_kind_t kind;
      svn_boolean_t conflicted;

      svn_pool_clear(iterpool);

      if (cancel_func)
        SVN_ERR(cancel_func(cancel_baton));

      child_abspath = svn_dirent_join(local_abspath, name, iterpool);

      SVN_ERR(svn_wc__db_read_info(&status, &kind, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   &conflicted, NULL, NULL, NULL, NULL, NULL,
                                   NULL,
                                   db, child_abspath, iterpool, iterpool));

      if (status == svn_wc__db_status_not_present
          || status == svn_wc__db_status_excluded
          || status == svn_wc__db_status_server_excluded)
        continue;

      apr_hash_set(visited, name, APR_HASH_KEY_STRING, name);
      if (kind == svn_wc__db_kind_dir && depth < svn_depth_immediates)
        continue;

      if (kind == svn_wc__db_kind_dir)
        SVN_ERR(recursive_resolve_conflict(db,
                                           child_abspath,
                                           conflicted,
                                           child_depth,
                                           resolve_text,
                                           resolve_prop,
                                           resolve_tree,
                                           conflict_choice,
                                           cancel_func, cancel_baton,
                                           notify_func, notify_baton,
                                           iterpool));
      else if (conflicted)
        SVN_ERR(resolve_one_conflict(db,
                                     child_abspath,
                                     resolve_text,
                                     resolve_prop,
                                     resolve_tree,
                                     conflict_choice,
                                     notify_func, notify_baton,
                                     iterpool));
    }

    SVN_ERR(svn_wc__db_read_conflict_victims(&children, db, local_abspath,
                                           scratch_pool, iterpool));

  for (i = 0; i < children->nelts; i++)
    {
      const char *name = APR_ARRAY_IDX(children, i, const char *);
      const char *child_abspath;

      svn_pool_clear(iterpool);

      if (apr_hash_get(visited, name, APR_HASH_KEY_STRING) != NULL)
        continue; /* Already visited */

      if (cancel_func)
        SVN_ERR(cancel_func(cancel_baton));

      child_abspath = svn_dirent_join(local_abspath, name, iterpool);

      /* We only have to resolve one level of tree conflicts. All other
         conflicts are resolved in the other loop */
      SVN_ERR(resolve_one_conflict(db,
                                   child_abspath,
                                   FALSE /*resolve_text*/,
                                   FALSE /*resolve_prop*/,
                                   resolve_tree,
                                   conflict_choice,
                                   notify_func, notify_baton,
                                   iterpool));
    }


  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_resolved_conflict5(svn_wc_context_t *wc_ctx,
                          const char *local_abspath,
                          svn_depth_t depth,
                          svn_boolean_t resolve_text,
                          const char *resolve_prop,
                          svn_boolean_t resolve_tree,
                          svn_wc_conflict_choice_t conflict_choice,
                          svn_cancel_func_t cancel_func,
                          void *cancel_baton,
                          svn_wc_notify_func2_t notify_func,
                          void *notify_baton,
                          apr_pool_t *scratch_pool)
{
  svn_wc__db_kind_t kind;
  svn_boolean_t conflicted;
  /* ### the underlying code does NOT support resolving individual
     ### properties. bail out if the caller tries it.  */
  if (resolve_prop != NULL && *resolve_prop != '\0')
    return svn_error_create(SVN_ERR_INCORRECT_PARAMS, NULL,
                            U_("Resolving a single property is not (yet) "
                               "supported."));

  SVN_ERR(svn_wc__db_read_info(NULL, &kind, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, &conflicted,
                               NULL, NULL, NULL, NULL, NULL, NULL,
                               wc_ctx->db, local_abspath,
                               scratch_pool, scratch_pool));

  /* When the implementation still used the entry walker, depth
     unknown was translated to infinity. */
  if (kind != svn_wc__db_kind_dir)
    depth = svn_depth_empty;
  else if (depth == svn_depth_unknown)
    depth = svn_depth_infinity;

  return svn_error_trace(recursive_resolve_conflict(
                           wc_ctx->db,
                           local_abspath,
                           conflicted,
                           depth,
                           resolve_text,
                           resolve_prop,
                           resolve_tree,
                           conflict_choice,
                           cancel_func, cancel_baton,
                           notify_func, notify_baton,
                           scratch_pool));
}
