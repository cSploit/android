/*
 * adm_ops.c: routines for affecting working copy administrative
 *            information.  NOTE: this code doesn't know where the adm
 *            info is actually stored.  Instead, generic handles to
 *            adm data are requested via a reference to some PATH
 *            (PATH being a regular, non-administrative directory or
 *            file in the working copy).
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
#include <stdlib.h>

#include <apr_pools.h>
#include <apr_tables.h>
#include <apr_hash.h>
#include <apr_file_io.h>
#include <apr_time.h>
#include <apr_errno.h>

#include "svn_types.h"
#include "svn_pools.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_hash.h"
#include "svn_wc.h"
#include "svn_io.h"
#include "svn_time.h"
#include "svn_sorts.h"

#include "wc.h"
#include "adm_files.h"
#include "props.h"
#include "translate.h"
#include "workqueue.h"

#include "svn_private_config.h"
#include "private/svn_io_private.h"
#include "private/svn_wc_private.h"



struct svn_wc_committed_queue_t
{
  /* The pool in which ->queue is allocated. */
  apr_pool_t *pool;
  /* Mapping (const char *) local_abspath to (committed_queue_item_t *). */
  apr_hash_t *queue;
  /* Is any item in the queue marked as 'recursive'? */
  svn_boolean_t have_recursive;
};

typedef struct committed_queue_item_t
{
  const char *local_abspath;
  svn_boolean_t recurse;
  svn_boolean_t no_unlock;
  svn_boolean_t keep_changelist;

  /* The pristine text checksum. */
  const svn_checksum_t *sha1_checksum;

  apr_hash_t *new_dav_cache;
} committed_queue_item_t;


apr_pool_t *
svn_wc__get_committed_queue_pool(const struct svn_wc_committed_queue_t *queue)
{
  return queue->pool;
}



/*** Finishing updates and commits. ***/

/* Queue work items that will finish a commit of the file or directory
 * LOCAL_ABSPATH in DB:
 *   - queue the removal of any "revert-base" props and text files;
 *   - queue an update of the DB entry for this node
 *
 * ### The Pristine Store equivalent should be:
 *   - remember the old BASE_NODE and WORKING_NODE pristine text c'sums;
 *   - queue an update of the DB entry for this node (incl. updating the
 *       BASE_NODE c'sum and setting the WORKING_NODE c'sum to NULL);
 *   - queue deletion of the old pristine texts by the remembered checksums.
 *
 * CHECKSUM is the checksum of the new text base for LOCAL_ABSPATH, and must
 * be provided if there is one, else NULL. */
static svn_error_t *
process_committed_leaf(svn_wc__db_t *db,
                       const char *local_abspath,
                       svn_boolean_t via_recurse,
                       svn_revnum_t new_revnum,
                       apr_time_t new_changed_date,
                       const char *new_changed_author,
                       apr_hash_t *new_dav_cache,
                       svn_boolean_t no_unlock,
                       svn_boolean_t keep_changelist,
                       const svn_checksum_t *checksum,
                       apr_pool_t *scratch_pool)
{
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;
  const svn_checksum_t *copied_checksum;
  svn_revnum_t new_changed_rev = new_revnum;
  svn_boolean_t have_base;
  svn_boolean_t have_work;
  svn_boolean_t had_props;
  svn_boolean_t prop_mods;
  svn_skel_t *work_item = NULL;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  SVN_ERR(svn_wc__db_read_info(&status, &kind, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, &copied_checksum,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, &had_props, &prop_mods,
                               &have_base, NULL, &have_work,
                               db, local_abspath,
                               scratch_pool, scratch_pool));

  {
    const char *adm_abspath;

    if (kind == svn_wc__db_kind_dir)
      adm_abspath = local_abspath;
    else
      adm_abspath = svn_dirent_dirname(local_abspath, scratch_pool);
    SVN_ERR(svn_wc__write_check(db, adm_abspath, scratch_pool));
  }

  if (status == svn_wc__db_status_deleted)
    {
      return svn_error_trace(
                svn_wc__db_op_remove_node(
                                db, local_abspath,
                                (have_base && !via_recurse)
                                    ? new_revnum : SVN_INVALID_REVNUM,
                                kind,
                                scratch_pool));
    }
  else if (status == svn_wc__db_status_not_present)
    {
      /* We are committing the leaf of a copy operation.
         We leave the not-present marker to allow pulling in excluded
         children of a copy.

         The next update will remove the not-present marker. */

      return SVN_NO_ERROR;
    }

  SVN_ERR_ASSERT(status == svn_wc__db_status_normal
                 || status == svn_wc__db_status_incomplete
                 || status == svn_wc__db_status_added);

  if (kind != svn_wc__db_kind_dir)
    {
      /* If we sent a delta (meaning: post-copy modification),
         then this file will appear in the queue and so we should have
         its checksum already. */
      if (checksum == NULL)
        {
          /* It was copied and not modified. We must have a text
             base for it. And the node should have a checksum. */
          SVN_ERR_ASSERT(copied_checksum != NULL);

          checksum = copied_checksum;

          /* Is the node completely unmodified and are we recursing? */
          if (via_recurse && !prop_mods)
            {
              /* If a copied node itself is not modified, but the op_root of
                 the copy is committed we have to make sure that changed_rev,
                 changed_date and changed_author don't change or the working
                 copy used for committing will show different last modified
                 information then a clean checkout of exactly the same
                 revisions. (Issue #3676) */

              SVN_ERR(svn_wc__db_read_info(NULL, NULL, NULL, NULL, NULL,
                                           NULL, &new_changed_rev,
                                           &new_changed_date,
                                           &new_changed_author, NULL, NULL,
                                           NULL, NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL, NULL,
                                           NULL, NULL, NULL,
                                           db, local_abspath,
                                           scratch_pool, scratch_pool));
            }
        }

      SVN_ERR(svn_wc__wq_build_file_commit(&work_item,
                                           db, local_abspath,
                                           prop_mods,
                                           scratch_pool, scratch_pool));
    }

  /* The new text base will be found in the pristine store by its checksum. */
  SVN_ERR(svn_wc__db_global_commit(db, local_abspath,
                                   new_revnum, new_changed_rev,
                                   new_changed_date, new_changed_author,
                                   checksum,
                                   NULL /* new_children */,
                                   new_dav_cache,
                                   keep_changelist,
                                   no_unlock,
                                   work_item,
                                   scratch_pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc__process_committed_internal(svn_wc__db_t *db,
                                   const char *local_abspath,
                                   svn_boolean_t recurse,
                                   svn_boolean_t top_of_recurse,
                                   svn_revnum_t new_revnum,
                                   apr_time_t new_date,
                                   const char *rev_author,
                                   apr_hash_t *new_dav_cache,
                                   svn_boolean_t no_unlock,
                                   svn_boolean_t keep_changelist,
                                   const svn_checksum_t *sha1_checksum,
                                   const svn_wc_committed_queue_t *queue,
                                   apr_pool_t *scratch_pool)
{
  svn_wc__db_kind_t kind;

  /* NOTE: be wary of making crazy semantic changes in this function, since
     svn_wc_process_committed4() calls this.  */

  SVN_ERR(process_committed_leaf(db, local_abspath, !top_of_recurse,
                                 new_revnum, new_date, rev_author,
                                 new_dav_cache,
                                 no_unlock, keep_changelist,
                                 sha1_checksum,
                                 scratch_pool));

  /* Only check kind after processing the node itself. The node might
     have been deleted */
  SVN_ERR(svn_wc__db_read_kind(&kind, db, local_abspath, TRUE, scratch_pool));

  if (recurse && kind == svn_wc__db_kind_dir)
    {
      const apr_array_header_t *children;
      apr_pool_t *iterpool = svn_pool_create(scratch_pool);
      int i;

      /* Read PATH's entries;  this is the absolute path. */
      SVN_ERR(svn_wc__db_read_children(&children, db, local_abspath,
                                       scratch_pool, iterpool));

      /* Recursively loop over all children. */
      for (i = 0; i < children->nelts; i++)
        {
          const char *name = APR_ARRAY_IDX(children, i, const char *);
          const char *this_abspath;
          svn_wc__db_status_t status;

          svn_pool_clear(iterpool);

          this_abspath = svn_dirent_join(local_abspath, name, iterpool);

          SVN_ERR(svn_wc__db_read_info(&status, &kind, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       db, this_abspath,
                                       iterpool, iterpool));

          /* We come to this branch since we have committed a copied tree.
             svn_depth_exclude is possible in this situation. So check and
             skip */
          if (status == svn_wc__db_status_excluded)
            continue;

          sha1_checksum = NULL;
          if (kind != svn_wc__db_kind_dir && queue != NULL)
            {
              const committed_queue_item_t *cqi;

              cqi = apr_hash_get(queue->queue, this_abspath,
                                 APR_HASH_KEY_STRING);
              if (cqi != NULL)
                {
                  sha1_checksum = cqi->sha1_checksum;
                }
            }

          /* Recurse.  Pass NULL for NEW_DAV_CACHE, because the
             ones present in the current call are only applicable to
             this one committed item. */
          SVN_ERR(svn_wc__process_committed_internal(
                    db, this_abspath,
                    TRUE /* recurse */,
                    FALSE /* top_of_recurse */,
                    new_revnum, new_date,
                    rev_author,
                    NULL /* new_dav_cache */,
                    TRUE /* no_unlock */,
                    keep_changelist,
                    sha1_checksum,
                    queue,
                    iterpool));
        }

      svn_pool_destroy(iterpool);
    }

  return SVN_NO_ERROR;
}


apr_hash_t *
svn_wc__prop_array_to_hash(const apr_array_header_t *props,
                           apr_pool_t *result_pool)
{
  int i;
  apr_hash_t *prophash;

  if (props == NULL || props->nelts == 0)
    return NULL;

  prophash = apr_hash_make(result_pool);

  for (i = 0; i < props->nelts; i++)
    {
      const svn_prop_t *prop = APR_ARRAY_IDX(props, i, const svn_prop_t *);
      if (prop->value != NULL)
        apr_hash_set(prophash, prop->name, APR_HASH_KEY_STRING, prop->value);
    }

  return prophash;
}


svn_wc_committed_queue_t *
svn_wc_committed_queue_create(apr_pool_t *pool)
{
  svn_wc_committed_queue_t *q;

  q = apr_palloc(pool, sizeof(*q));
  q->pool = pool;
  q->queue = apr_hash_make(pool);
  q->have_recursive = FALSE;

  return q;
}


svn_error_t *
svn_wc_queue_committed3(svn_wc_committed_queue_t *queue,
                        svn_wc_context_t *wc_ctx,
                        const char *local_abspath,
                        svn_boolean_t recurse,
                        const apr_array_header_t *wcprop_changes,
                        svn_boolean_t remove_lock,
                        svn_boolean_t remove_changelist,
                        const svn_checksum_t *sha1_checksum,
                        apr_pool_t *scratch_pool)
{
  committed_queue_item_t *cqi;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  queue->have_recursive |= recurse;

  /* Use the same pool as the one QUEUE was allocated in,
     to prevent lifetime issues.  Intermediate operations
     should use SCRATCH_POOL. */

  /* Add to the array with paths and options */
  cqi = apr_palloc(queue->pool, sizeof(*cqi));
  cqi->local_abspath = local_abspath;
  cqi->recurse = recurse;
  cqi->no_unlock = !remove_lock;
  cqi->keep_changelist = !remove_changelist;
  cqi->sha1_checksum = sha1_checksum;
  cqi->new_dav_cache = svn_wc__prop_array_to_hash(wcprop_changes, queue->pool);

  apr_hash_set(queue->queue, local_abspath, APR_HASH_KEY_STRING, cqi);

  return SVN_NO_ERROR;
}


/* Return TRUE if any item of QUEUE is a parent of ITEM and will be
   processed recursively, return FALSE otherwise.

   The algorithmic complexity of this search implementation is O(queue
   length), but it's quite quick.
*/
static svn_boolean_t
have_recursive_parent(apr_hash_t *queue,
                      const committed_queue_item_t *item,
                      apr_pool_t *scratch_pool)
{
  apr_hash_index_t *hi;
  const char *local_abspath = item->local_abspath;

  for (hi = apr_hash_first(scratch_pool, queue); hi; hi = apr_hash_next(hi))
    {
      const committed_queue_item_t *qi = svn__apr_hash_index_val(hi);

      if (qi == item)
        continue;

      if (qi->recurse && svn_dirent_is_child(qi->local_abspath, local_abspath,
                                             NULL))
        return TRUE;
    }

  return FALSE;
}


svn_error_t *
svn_wc_process_committed_queue2(svn_wc_committed_queue_t *queue,
                                svn_wc_context_t *wc_ctx,
                                svn_revnum_t new_revnum,
                                const char *rev_date,
                                const char *rev_author,
                                svn_cancel_func_t cancel_func,
                                void *cancel_baton,
                                apr_pool_t *scratch_pool)
{
  apr_array_header_t *sorted_queue;
  int i;
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  apr_time_t new_date;
  apr_hash_t *run_wqs = apr_hash_make(scratch_pool);
  apr_hash_index_t *hi;

  if (rev_date)
    SVN_ERR(svn_time_from_cstring(&new_date, rev_date, iterpool));
  else
    new_date = 0;

  /* Process the queued items in order of their paths.  (The requirement is
   * probably just that a directory must be processed before its children.) */
  sorted_queue = svn_sort__hash(queue->queue, svn_sort_compare_items_as_paths,
                                scratch_pool);
  for (i = 0; i < sorted_queue->nelts; i++)
    {
      const svn_sort__item_t *sort_item
        = &APR_ARRAY_IDX(sorted_queue, i, svn_sort__item_t);
      const committed_queue_item_t *cqi = sort_item->value;
      const char *wcroot_abspath;

      svn_pool_clear(iterpool);

      /* Skip this item if it is a child of a recursive item, because it has
         been (or will be) accounted for when that recursive item was (or
         will be) processed. */
      if (queue->have_recursive && have_recursive_parent(queue->queue, cqi,
                                                         iterpool))
        continue;

      SVN_ERR(svn_wc__process_committed_internal(
                wc_ctx->db, cqi->local_abspath,
                cqi->recurse,
                TRUE /* top_of_recurse */,
                new_revnum, new_date, rev_author,
                cqi->new_dav_cache,
                cqi->no_unlock,
                cqi->keep_changelist,
                cqi->sha1_checksum, queue,
                iterpool));

      /* Don't run the wq now, but remember that we must call it for this
         working copy */
      SVN_ERR(svn_wc__db_get_wcroot(&wcroot_abspath,
                                    wc_ctx->db, cqi->local_abspath,
                                    iterpool, iterpool));

      if (! apr_hash_get(run_wqs, wcroot_abspath, APR_HASH_KEY_STRING))
        {
          wcroot_abspath = apr_pstrdup(scratch_pool, wcroot_abspath);
          apr_hash_set(run_wqs, wcroot_abspath, APR_HASH_KEY_STRING,
                       wcroot_abspath);
        }
    }

  /* Make sure nothing happens if this function is called again.  */
  SVN_ERR(svn_hash__clear(queue->queue, iterpool));

  /* Ok; everything is committed now. Now we can start calling callbacks */

  if (cancel_func)
    SVN_ERR(cancel_func(cancel_baton));

  for (hi = apr_hash_first(scratch_pool, run_wqs);
       hi;
       hi = apr_hash_next(hi))
    {
      const char *wcroot_abspath = svn__apr_hash_index_key(hi);

      svn_pool_clear(iterpool);

      SVN_ERR(svn_wc__wq_run(wc_ctx->db, wcroot_abspath,
                             cancel_func, cancel_baton,
                             iterpool));
    }

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}


/* Remove/erase PATH from the working copy. This involves deleting PATH
 * from the physical filesystem. PATH is assumed to be an unversioned file
 * or directory.
 *
 * If ignore_enoent is TRUE, ignore missing targets.
 *
 * If CANCEL_FUNC is non-null, invoke it with CANCEL_BATON at various
 * points, return any error immediately.
 */
static svn_error_t *
erase_unversioned_from_wc(const char *path,
                          svn_boolean_t ignore_enoent,
                          svn_cancel_func_t cancel_func,
                          void *cancel_baton,
                          apr_pool_t *scratch_pool)
{
  svn_error_t *err;

  /* Optimize the common case: try to delete the file */
  err = svn_io_remove_file2(path, ignore_enoent, scratch_pool);
  if (err)
    {
      /* Then maybe it was a directory? */
      svn_error_clear(err);

      err = svn_io_remove_dir2(path, ignore_enoent, cancel_func, cancel_baton,
                               scratch_pool);

      if (err)
        {
          /* We're unlikely to end up here. But we need this fallback
             to make sure we report the right error *and* try the
             correct deletion at least once. */
          svn_node_kind_t kind;

          svn_error_clear(err);
          SVN_ERR(svn_io_check_path(path, &kind, scratch_pool));
          if (kind == svn_node_file)
            SVN_ERR(svn_io_remove_file2(path, ignore_enoent, scratch_pool));
          else if (kind == svn_node_dir)
            SVN_ERR(svn_io_remove_dir2(path, ignore_enoent,
                                       cancel_func, cancel_baton,
                                       scratch_pool));
          else if (kind == svn_node_none)
            return svn_error_createf(SVN_ERR_BAD_FILENAME, NULL,
                                     _("'%s' does not exist"),
                                     svn_dirent_local_style(path,
                                                            scratch_pool));
          else
            return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                                     _("Unsupported node kind for path '%s'"),
                                     svn_dirent_local_style(path,
                                                            scratch_pool));

        }
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_delete4(svn_wc_context_t *wc_ctx,
               const char *local_abspath,
               svn_boolean_t keep_local,
               svn_boolean_t delete_unversioned_target,
               svn_cancel_func_t cancel_func,
               void *cancel_baton,
               svn_wc_notify_func2_t notify_func,
               void *notify_baton,
               apr_pool_t *scratch_pool)
{
  apr_pool_t *pool = scratch_pool;
  svn_wc__db_t *db = wc_ctx->db;
  svn_error_t *err;
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;
  svn_boolean_t conflicted;
  const apr_array_header_t *conflicts;

  err = svn_wc__db_read_info(&status, &kind, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, &conflicted,
                             NULL, NULL, NULL, NULL, NULL, NULL,
                             db, local_abspath, pool, pool);

  if (delete_unversioned_target &&
      err != NULL && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
    {
      svn_error_clear(err);

      if (!keep_local)
        SVN_ERR(erase_unversioned_from_wc(local_abspath, FALSE,
                                          cancel_func, cancel_baton,
                                          pool));
      return SVN_NO_ERROR;
    }
  else
    SVN_ERR(err);

  switch (status)
    {
      /* svn_wc__db_status_server_excluded handled by svn_wc__db_op_delete */
      case svn_wc__db_status_excluded:
      case svn_wc__db_status_not_present:
        return svn_error_createf(SVN_ERR_WC_PATH_NOT_FOUND, NULL,
                                 _("'%s' cannot be deleted"),
                                 svn_dirent_local_style(local_abspath, pool));

      /* Explicitly ignore other statii */
      default:
        break;
    }

  if (status == svn_wc__db_status_normal
      && kind == svn_wc__db_kind_dir)
    {
      svn_boolean_t is_wcroot;
      SVN_ERR(svn_wc__db_is_wcroot(&is_wcroot, db, local_abspath, pool));

      if (is_wcroot)
        return svn_error_createf(SVN_ERR_WC_PATH_UNEXPECTED_STATUS, NULL,
                                 _("'%s' is the root of a working copy and "
                                   "cannot be deleted"),
                                 svn_dirent_local_style(local_abspath, pool));
    }

  /* Verify if we have a write lock on the parent of this node as we might
     be changing the childlist of that directory. */
  SVN_ERR(svn_wc__write_check(db, svn_dirent_dirname(local_abspath, pool),
                              pool));

  /* Read conflicts, to allow deleting the markers after updating the DB */
  if (!keep_local && conflicted)
    SVN_ERR(svn_wc__db_read_conflicts(&conflicts, db, local_abspath,
                                      scratch_pool, scratch_pool));

  SVN_ERR(svn_wc__db_op_delete(db, local_abspath,
                               notify_func, notify_baton,
                               cancel_func, cancel_baton,
                               pool));

  if (!keep_local && conflicted && conflicts != NULL)
    {
      int i;

      /* Do we have conflict markers that should be removed? */
      for (i = 0; i < conflicts->nelts; i++)
        {
          const svn_wc_conflict_description2_t *desc;

          desc = APR_ARRAY_IDX(conflicts, i,
                               const svn_wc_conflict_description2_t*);

          if (desc->kind == svn_wc_conflict_kind_text)
            {
              if (desc->base_abspath != NULL)
                {
                  SVN_ERR(svn_io_remove_file2(desc->base_abspath, TRUE,
                                              scratch_pool));
                }
              if (desc->their_abspath != NULL)
                {
                  SVN_ERR(svn_io_remove_file2(desc->their_abspath, TRUE,
                                              scratch_pool));
                }
              if (desc->my_abspath != NULL)
                {
                  SVN_ERR(svn_io_remove_file2(desc->my_abspath, TRUE,
                                              scratch_pool));
                }
            }
          else if (desc->kind == svn_wc_conflict_kind_property
                   && desc->their_abspath != NULL)
            {
              SVN_ERR(svn_io_remove_file2(desc->their_abspath, TRUE,
                                          scratch_pool));
            }
        }
    }

  /* By the time we get here, the db knows that everything that is still at
     LOCAL_ABSPATH is unversioned. */
  if (!keep_local)
    {
        SVN_ERR(erase_unversioned_from_wc(local_abspath, TRUE,
                                          cancel_func, cancel_baton,
                                          pool));
    }

  return SVN_NO_ERROR;
}


/* Schedule the single node at LOCAL_ABSPATH, of kind KIND, for addition in
 * its parent directory in the WC.  It will have no properties. */
static svn_error_t *
add_from_disk(svn_wc__db_t *db,
              const char *local_abspath,
              svn_node_kind_t kind,
              svn_wc_notify_func2_t notify_func,
              void *notify_baton,
              apr_pool_t *scratch_pool)
{
  if (kind == svn_node_file)
    {
      SVN_ERR(svn_wc__db_op_add_file(db, local_abspath, NULL, scratch_pool));
    }
  else
    {
      SVN_ERR(svn_wc__db_op_add_directory(db, local_abspath, NULL,
                                          scratch_pool));
    }

  return SVN_NO_ERROR;
}


/* Set *REPOS_ROOT_URL and *REPOS_UUID to the repository of the parent of
   LOCAL_ABSPATH.  REPOS_ROOT_URL and/or REPOS_UUID may be NULL if not
   wanted.  Check that the parent of LOCAL_ABSPATH is a versioned directory
   in a state in which a new child node can be scheduled for addition;
   return an error if not. */
static svn_error_t *
check_can_add_to_parent(const char **repos_root_url,
                        const char **repos_uuid,
                        svn_wc__db_t *db,
                        const char *local_abspath,
                        apr_pool_t *result_pool,
                        apr_pool_t *scratch_pool)
{
  const char *parent_abspath = svn_dirent_dirname(local_abspath, scratch_pool);
  svn_wc__db_status_t parent_status;
  svn_wc__db_kind_t parent_kind;
  svn_error_t *err;

  SVN_ERR(svn_wc__write_check(db, parent_abspath, scratch_pool));

  err = svn_wc__db_read_info(&parent_status, &parent_kind, NULL,
                             NULL, repos_root_url, repos_uuid, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL,
                             db, parent_abspath, result_pool, scratch_pool);

  if (err
      || parent_status == svn_wc__db_status_not_present
      || parent_status == svn_wc__db_status_excluded
      || parent_status == svn_wc__db_status_server_excluded)
    {
      return
        svn_error_createf(SVN_ERR_ENTRY_NOT_FOUND, err,
                          _("Can't find parent directory's node while"
                            " trying to add '%s'"),
                          svn_dirent_local_style(local_abspath,
                                                 scratch_pool));
    }
  else if (parent_status == svn_wc__db_status_deleted)
    {
      return
        svn_error_createf(SVN_ERR_WC_SCHEDULE_CONFLICT, NULL,
                          _("Can't add '%s' to a parent directory"
                            " scheduled for deletion"),
                          svn_dirent_local_style(local_abspath,
                                                 scratch_pool));
    }
  else if (parent_kind != svn_wc__db_kind_dir)
    return svn_error_createf(SVN_ERR_NODE_UNEXPECTED_KIND, NULL,
                             _("Can't schedule an addition of '%s'"
                               " below a not-directory node"),
                             svn_dirent_local_style(local_abspath,
                                                    scratch_pool));

  /* If we haven't found the repository info yet, find it now. */
  if ((repos_root_url && ! *repos_root_url)
      || (repos_uuid && ! *repos_uuid))
    {
      if (parent_status == svn_wc__db_status_added)
        SVN_ERR(svn_wc__db_scan_addition(NULL, NULL, NULL,
                                         repos_root_url, repos_uuid, NULL,
                                         NULL, NULL, NULL,
                                         db, parent_abspath,
                                         result_pool, scratch_pool));
      else
        SVN_ERR(svn_wc__db_scan_base_repos(NULL,
                                           repos_root_url, repos_uuid,
                                           db, parent_abspath,
                                           result_pool, scratch_pool));
    }

  return SVN_NO_ERROR;
}


/* Check that the on-disk item at LOCAL_ABSPATH can be scheduled for
 * addition to its WC parent directory.
 *
 * Set *KIND_P to the kind of node to be added, *DB_ROW_EXISTS_P to whether
 * it is already a versioned path, and if so, *IS_WC_ROOT_P to whether it's
 * a WC root.
 *
 * ### The checks here, and the outputs, are geared towards svn_wc_add4().
 */
static svn_error_t *
check_can_add_node(svn_node_kind_t *kind_p,
                   svn_boolean_t *db_row_exists_p,
                   svn_boolean_t *is_wc_root_p,
                   svn_wc__db_t *db,
                   const char *local_abspath,
                   const char *copyfrom_url,
                   svn_revnum_t copyfrom_rev,
                   apr_pool_t *scratch_pool)
{
  const char *base_name = svn_dirent_basename(local_abspath, scratch_pool);
  svn_boolean_t is_wc_root;
  svn_node_kind_t kind;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));
  SVN_ERR_ASSERT(!copyfrom_url || (svn_uri_is_canonical(copyfrom_url,
                                                        scratch_pool)
                                   && SVN_IS_VALID_REVNUM(copyfrom_rev)));

  /* Check that the proposed node has an acceptable name. */
  if (svn_wc_is_adm_dir(base_name, scratch_pool))
    return svn_error_createf
      (SVN_ERR_ENTRY_FORBIDDEN, NULL,
       _("Can't create an entry with a reserved name while trying to add '%s'"),
       svn_dirent_local_style(local_abspath, scratch_pool));

  SVN_ERR(svn_path_check_valid(local_abspath, scratch_pool));

  /* Make sure something's there; set KIND and *KIND_P. */
  SVN_ERR(svn_io_check_path(local_abspath, &kind, scratch_pool));
  if (kind == svn_node_none)
    return svn_error_createf(SVN_ERR_WC_PATH_NOT_FOUND, NULL,
                             _("'%s' not found"),
                             svn_dirent_local_style(local_abspath,
                                                    scratch_pool));
  if (kind == svn_node_unknown)
    return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                             _("Unsupported node kind for path '%s'"),
                             svn_dirent_local_style(local_abspath,
                                                    scratch_pool));
  if (kind_p)
    *kind_p = kind;

  /* Determine whether a DB row for this node EXISTS, and whether it
     IS_WC_ROOT.  If it exists, check that it is in an acceptable state for
     adding the new node; if not, return an error. */
  {
    svn_wc__db_status_t status;
    svn_boolean_t conflicted;
    svn_boolean_t exists;
    svn_error_t *err
      = svn_wc__db_read_info(&status, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL,
                             &conflicted,
                             NULL, NULL, NULL, NULL, NULL, NULL,
                             db, local_abspath,
                             scratch_pool, scratch_pool);

    if (err)
      {
        if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
          return svn_error_trace(err);

        svn_error_clear(err);
        exists = FALSE;
        is_wc_root = FALSE;
      }
    else
      {
        is_wc_root = FALSE;
        exists = TRUE;

        /* Note that the node may be in conflict even if it does not
         * exist on disk (certain tree conflict scenarios). */
        if (conflicted)
          return svn_error_createf(SVN_ERR_WC_FOUND_CONFLICT, NULL,
                                   _("'%s' is an existing item in conflict; "
                                   "please mark the conflict as resolved "
                                   "before adding a new item here"),
                                   svn_dirent_local_style(local_abspath,
                                                          scratch_pool));
        switch (status)
          {
            case svn_wc__db_status_not_present:
              break;
            case svn_wc__db_status_deleted:
              /* A working copy root should never have a WORKING_NODE */
              SVN_ERR_ASSERT(!is_wc_root);
              break;
            case svn_wc__db_status_normal:
              if (copyfrom_url)
                {
                  SVN_ERR(svn_wc__check_wc_root(&is_wc_root, NULL, NULL,
                                                db, local_abspath,
                                                scratch_pool));

                  if (is_wc_root)
                    break;
                }
              /* else: Fall through in default error */

            default:
              return svn_error_createf(
                               SVN_ERR_ENTRY_EXISTS, NULL,
                               _("'%s' is already under version control"),
                               svn_dirent_local_style(local_abspath,
                                                      scratch_pool));
          }
      } /* err */

    if (db_row_exists_p)
      *db_row_exists_p = exists;
    if (is_wc_root_p)
      *is_wc_root_p = is_wc_root;
  }

  return SVN_NO_ERROR;
}


/* Convert the nested pristine working copy rooted at LOCAL_ABSPATH into
 * a copied subtree in the outer working copy.
 *
 * LOCAL_ABSPATH must be the root of a nested working copy that has no
 * local modifications.  The parent directory of LOCAL_ABSPATH must be a
 * versioned directory in the outer WC, and must belong to the same
 * repository as the nested WC.  The nested WC will be integrated into the
 * parent's WC, and will no longer be a separate WC. */
static svn_error_t *
integrate_nested_wc_as_copy(svn_wc_context_t *wc_ctx,
                            const char *local_abspath,
                            apr_pool_t *scratch_pool)
{
  svn_wc__db_t *db = wc_ctx->db;
  const char *moved_abspath;

  /* Drop any references to the wc that is to be rewritten */
  SVN_ERR(svn_wc__db_drop_root(db, local_abspath, scratch_pool));

  /* Move the admin dir from the wc to a temporary location: MOVED_ABSPATH */
  {
    const char *tmpdir_abspath;
    const char *moved_adm_abspath;
    const char *adm_abspath;

    SVN_ERR(svn_wc__db_temp_wcroot_tempdir(&tmpdir_abspath, db,
                                           svn_dirent_dirname(local_abspath,
                                                              scratch_pool),
                                           scratch_pool, scratch_pool));
    SVN_ERR(svn_io_open_unique_file3(NULL, &moved_abspath, tmpdir_abspath,
                                     svn_io_file_del_on_close,
                                     scratch_pool, scratch_pool));
    SVN_ERR(svn_io_dir_make(moved_abspath, APR_OS_DEFAULT, scratch_pool));

    adm_abspath = svn_wc__adm_child(local_abspath, "", scratch_pool);
    moved_adm_abspath = svn_wc__adm_child(moved_abspath, "", scratch_pool);
    SVN_ERR(svn_io_file_move(adm_abspath, moved_adm_abspath, scratch_pool));
  }

  /* Copy entries from temporary location into the main db */
  SVN_ERR(svn_wc_copy3(wc_ctx, moved_abspath, local_abspath,
                       TRUE /* metadata_only */,
                       NULL, NULL, NULL, NULL, scratch_pool));

  /* Cleanup the temporary admin dir */
  SVN_ERR(svn_wc__db_drop_root(db, moved_abspath, scratch_pool));
  SVN_ERR(svn_io_remove_dir2(moved_abspath, FALSE, NULL, NULL,
                             scratch_pool));

  /* The subdir is now part of our parent working copy. Our caller assumes
     that we return the new node locked, so obtain a lock if we didn't
     receive the lock via our depth infinity lock */
  {
    svn_boolean_t owns_lock;

    SVN_ERR(svn_wc__db_wclock_owns_lock(&owns_lock, db, local_abspath,
                                        FALSE, scratch_pool));
    if (!owns_lock)
      SVN_ERR(svn_wc__db_wclock_obtain(db, local_abspath, 0, FALSE,
                                       scratch_pool));
  }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_add4(svn_wc_context_t *wc_ctx,
            const char *local_abspath,
            svn_depth_t depth,
            const char *copyfrom_url,
            svn_revnum_t copyfrom_rev,
            svn_cancel_func_t cancel_func,
            void *cancel_baton,
            svn_wc_notify_func2_t notify_func,
            void *notify_baton,
            apr_pool_t *scratch_pool)
{
  svn_wc__db_t *db = wc_ctx->db;
  svn_node_kind_t kind;
  svn_boolean_t db_row_exists;
  svn_boolean_t is_wc_root;
  const char *repos_root_url;
  const char *repos_uuid;

  SVN_ERR(check_can_add_node(&kind, &db_row_exists, &is_wc_root,
                             db, local_abspath, copyfrom_url, copyfrom_rev,
                             scratch_pool));

  /* Get REPOS_ROOT_URL and REPOS_UUID.  Check that the
     parent is a versioned directory in an acceptable state. */
  SVN_ERR(check_can_add_to_parent(&repos_root_url, &repos_uuid,
                                  db, local_abspath, scratch_pool,
                                  scratch_pool));

  /* If we're performing a repos-to-WC copy, check that the copyfrom
     repository is the same as the parent dir's repository. */
  if (copyfrom_url && !svn_uri__is_ancestor(repos_root_url, copyfrom_url))
    return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                             _("The URL '%s' has a different repository "
                               "root than its parent"), copyfrom_url);

  /* Verify that we can actually integrate the inner working copy */
  if (is_wc_root)
    {
      const char *repos_relpath, *inner_repos_root_url, *inner_repos_uuid;
      const char *inner_url;

      SVN_ERR(svn_wc__db_scan_base_repos(&repos_relpath,
                                         &inner_repos_root_url,
                                         &inner_repos_uuid,
                                         db, local_abspath,
                                         scratch_pool, scratch_pool));

      if (strcmp(inner_repos_uuid, repos_uuid)
          || strcmp(repos_root_url, inner_repos_root_url))
        return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                                 _("Can't schedule the working copy at '%s' "
                                   "from repository '%s' with uuid '%s' "
                                   "for addition under a working copy from "
                                   "repository '%s' with uuid '%s'."),
                                 svn_dirent_local_style(local_abspath,
                                                        scratch_pool),
                                 inner_repos_root_url, inner_repos_uuid,
                                 repos_root_url, repos_uuid);

      inner_url = svn_path_url_add_component2(repos_root_url, repos_relpath,
                                              scratch_pool);

      if (strcmp(copyfrom_url, inner_url))
        return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
                                 _("Can't add '%s' with URL '%s', but with "
                                   "the data from '%s'"),
                                 svn_dirent_local_style(local_abspath,
                                                        scratch_pool),
                                 copyfrom_url, inner_url);
    }

  if (!copyfrom_url)  /* Case 2a: It's a simple add */
    {
      SVN_ERR(add_from_disk(db, local_abspath, kind, notify_func, notify_baton,
                            scratch_pool));
      if (kind == svn_node_dir && !db_row_exists)
        {
          /* If using the legacy 1.6 interface the parent lock may not
             be recursive and add is expected to lock the new dir.

             ### Perhaps the lock should be created in the same
             transaction that adds the node? */
          svn_boolean_t owns_lock;

          SVN_ERR(svn_wc__db_wclock_owns_lock(&owns_lock, db, local_abspath,
                                              FALSE, scratch_pool));
          if (!owns_lock)
            SVN_ERR(svn_wc__db_wclock_obtain(db, local_abspath, 0, FALSE,
                                             scratch_pool));
        }
    }
  else if (!is_wc_root)  /* Case 2b: It's a copy from the repository */
    {
      if (kind == svn_node_file)
        {
          /* This code should never be used, as it doesn't install proper
             pristine and/or properties. But it was not an error in the old
             version of this function.

             ===> Use svn_wc_add_repos_file4() directly! */
          svn_stream_t *content = svn_stream_empty(scratch_pool);

          SVN_ERR(svn_wc_add_repos_file4(wc_ctx, local_abspath,
                                         content, NULL, NULL, NULL,
                                         copyfrom_url, copyfrom_rev,
                                         cancel_func, cancel_baton,
                                         scratch_pool));
        }
      else
        {
          const char *repos_relpath =
            svn_uri_skip_ancestor(repos_root_url, copyfrom_url, scratch_pool);

          SVN_ERR(svn_wc__db_op_copy_dir(db, local_abspath,
                                         apr_hash_make(scratch_pool),
                                         copyfrom_rev, 0, NULL,
                                         repos_relpath,
                                         repos_root_url, repos_uuid,
                                         copyfrom_rev,
                                         NULL /* children */, depth,
                                         NULL /* conflicts */,
                                         NULL /* work items */,
                                         scratch_pool));
        }
    }
  else  /* Case 1: Integrating a separate WC into this one, in place */
    {
      SVN_ERR(integrate_nested_wc_as_copy(wc_ctx, local_abspath,
                                          scratch_pool));
    }

  /* Report the addition to the caller. */
  if (notify_func != NULL)
    {
      svn_wc_notify_t *notify = svn_wc_create_notify(local_abspath,
                                                     svn_wc_notify_add,
                                                     scratch_pool);
      notify->kind = kind;
      (*notify_func)(notify_baton, notify, scratch_pool);
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_add_from_disk(svn_wc_context_t *wc_ctx,
                     const char *local_abspath,
                     svn_wc_notify_func2_t notify_func,
                     void *notify_baton,
                     apr_pool_t *scratch_pool)
{
  svn_node_kind_t kind;

  SVN_ERR(check_can_add_node(&kind, NULL, NULL, wc_ctx->db, local_abspath,
                             NULL, SVN_INVALID_REVNUM, scratch_pool));
  SVN_ERR(check_can_add_to_parent(NULL, NULL, wc_ctx->db, local_abspath,
                                  scratch_pool, scratch_pool));
  SVN_ERR(add_from_disk(wc_ctx->db, local_abspath, kind,
                        notify_func, notify_baton,
                        scratch_pool));

  /* Report the addition to the caller. */
  if (notify_func != NULL)
    {
      svn_wc_notify_t *notify = svn_wc_create_notify(local_abspath,
                                                     svn_wc_notify_add,
                                                     scratch_pool);
      notify->kind = kind;
      (*notify_func)(notify_baton, notify, scratch_pool);
    }

  return SVN_NO_ERROR;
}

/* Thoughts on Reversion.

    What does is mean to revert a given PATH in a tree?  We'll
    consider things by their modifications.

    Adds

    - For files, svn_wc_remove_from_revision_control(), baby.

    - Added directories may contain nothing but added children, and
      reverting the addition of a directory necessarily means reverting
      the addition of all the directory's children.  Again,
      svn_wc_remove_from_revision_control() should do the trick.

    Deletes

    - Restore properties to their unmodified state.

    - For files, restore the pristine contents, and reset the schedule
      to 'normal'.

    - For directories, reset the schedule to 'normal'.  All children
      of a directory marked for deletion must also be marked for
      deletion, but it's okay for those children to remain deleted even
      if their parent directory is restored.  That's what the
      recursive flag is for.

    Replaces

    - Restore properties to their unmodified state.

    - For files, restore the pristine contents, and reset the schedule
      to 'normal'.

    - For directories, reset the schedule to normal.  A replaced
      directory can have deleted children (left over from the initial
      deletion), replaced children (children of the initial deletion
      now re-added), and added children (new entries under the
      replaced directory).  Since this is technically an addition, it
      necessitates recursion.

    Modifications

    - Restore properties and, for files, contents to their unmodified
      state.

*/


/* Remove conflict file CONFLICT_ABSPATH, which may not exist, and set
 * *NOTIFY_REQUIRED to TRUE if the file was present and removed. */
static svn_error_t *
remove_conflict_file(svn_boolean_t *notify_required,
                     const char *conflict_abspath,
                     const char *local_abspath,
                     apr_pool_t *scratch_pool)
{
  if (conflict_abspath)
    {
      svn_error_t *err = svn_io_remove_file2(conflict_abspath, FALSE,
                                             scratch_pool);
      if (err)
        svn_error_clear(err);
      else
        *notify_required = TRUE;
    }

  return SVN_NO_ERROR;
}


/* Sort copied children obtained from the revert list based on
 * their paths in descending order (longest paths first). */
static int
compare_revert_list_copied_children(const void *a, const void *b)
{
  const svn_wc__db_revert_list_copied_child_info_t * const *ca = a;
  const svn_wc__db_revert_list_copied_child_info_t * const *cb = b;
  int i;

  i = svn_path_compare_paths(ca[0]->abspath, cb[0]->abspath);

  /* Reverse the result of svn_path_compare_paths() to achieve
   * descending order. */
  return -i;
}


/* Remove all reverted copied children from the directory at LOCAL_ABSPATH.
 * If REMOVE_SELF is TRUE, try to remove LOCAL_ABSPATH itself (REMOVE_SELF
 * should be set if LOCAL_ABSPATH is itself a reverted copy).
 *
 * If REMOVED_SELF is not NULL, indicate in *REMOVED_SELF whether
 * LOCAL_ABSPATH itself was removed.
 *
 * All reverted copied file children are removed from disk. Reverted copied
 * directories left empty as a result are also removed from disk.
 */
static svn_error_t *
revert_restore_handle_copied_dirs(svn_boolean_t *removed_self,
                                  svn_wc__db_t *db,
                                  const char *local_abspath,
                                  svn_boolean_t remove_self,
                                  svn_cancel_func_t cancel_func,
                                  void *cancel_baton,
                                  apr_pool_t *scratch_pool)
{
  const apr_array_header_t *copied_children;
  svn_wc__db_revert_list_copied_child_info_t *child_info;
  int i;
  svn_node_kind_t on_disk;
  apr_pool_t *iterpool;
  svn_error_t *err;

  if (removed_self)
    *removed_self = FALSE;

  SVN_ERR(svn_wc__db_revert_list_read_copied_children(&copied_children,
                                                      db, local_abspath,
                                                      scratch_pool,
                                                      scratch_pool));
  iterpool = svn_pool_create(scratch_pool);

  /* Remove all copied file children. */
  for (i = 0; i < copied_children->nelts; i++)
    {
      child_info = APR_ARRAY_IDX(
                     copied_children, i,
                     svn_wc__db_revert_list_copied_child_info_t *);

      if (cancel_func)
        SVN_ERR(cancel_func(cancel_baton));

      if (child_info->kind != svn_wc__db_kind_file)
        continue;

      svn_pool_clear(iterpool);

      /* Make sure what we delete from disk is really a file. */
      SVN_ERR(svn_io_check_path(child_info->abspath, &on_disk, iterpool));
      if (on_disk != svn_node_file)
        continue;

      SVN_ERR(svn_io_remove_file2(child_info->abspath, TRUE, iterpool));
    }

  /* Delete every empty child directory.
   * We cannot delete children recursively since we want to keep any files
   * that still exist on disk (e.g. unversioned files within the copied tree).
   * So sort the children list such that longest paths come first and try to
   * remove each child directory in order. */
  qsort(copied_children->elts, copied_children->nelts,
        sizeof(svn_wc__db_revert_list_copied_child_info_t *),
        compare_revert_list_copied_children);
  for (i = 0; i < copied_children->nelts; i++)
    {
      child_info = APR_ARRAY_IDX(
                     copied_children, i,
                     svn_wc__db_revert_list_copied_child_info_t *);

      if (cancel_func)
        SVN_ERR(cancel_func(cancel_baton));

      if (child_info->kind != svn_wc__db_kind_dir)
        continue;

      svn_pool_clear(iterpool);

      err = svn_io_dir_remove_nonrecursive(child_info->abspath, iterpool);
      if (err)
        {
          if (APR_STATUS_IS_ENOENT(err->apr_err) ||
              SVN__APR_STATUS_IS_ENOTDIR(err->apr_err) ||
              APR_STATUS_IS_ENOTEMPTY(err->apr_err))
            svn_error_clear(err);
          else
            return svn_error_trace(err);
        }
    }

  if (remove_self)
    {
      /* Delete LOCAL_ABSPATH itself if no children are left. */
      err = svn_io_dir_remove_nonrecursive(local_abspath, iterpool);
      if (err)
       {
          if (APR_STATUS_IS_ENOTEMPTY(err->apr_err))
            svn_error_clear(err);
          else
            return svn_error_trace(err);
        }
      else if (removed_self)
        *removed_self = TRUE;
    }

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}


/* Make the working tree under LOCAL_ABSPATH to depth DEPTH match the
   versioned tree.  This function is called after svn_wc__db_op_revert
   has done the database revert and created the revert list.  Notifies
   for all paths equal to or below LOCAL_ABSPATH that are reverted.

   REVERT_ROOT is true for explicit revert targets and FALSE for targets
   reached via recursion.
 */
static svn_error_t *
revert_restore(svn_wc__db_t *db,
               const char *local_abspath,
               svn_depth_t depth,
               svn_boolean_t use_commit_times,
               svn_boolean_t revert_root,
               svn_cancel_func_t cancel_func,
               void *cancel_baton,
               svn_wc_notify_func2_t notify_func,
               void *notify_baton,
               apr_pool_t *scratch_pool)
{
  svn_error_t *err;
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;
  svn_node_kind_t on_disk;
  svn_boolean_t notify_required;
  const char *conflict_old;
  const char *conflict_new;
  const char *conflict_working;
  const char *prop_reject;
  svn_filesize_t recorded_size;
  apr_time_t recorded_mod_time;
  apr_finfo_t finfo;
#ifdef HAVE_SYMLINK
  svn_boolean_t special;
#endif
  svn_boolean_t copied_here;
  svn_wc__db_kind_t reverted_kind;
  svn_boolean_t is_wcroot;

  if (cancel_func)
    SVN_ERR(cancel_func(cancel_baton));

  SVN_ERR(svn_wc__db_is_wcroot(&is_wcroot, db, local_abspath, scratch_pool));
  if (is_wcroot && !revert_root)
    {
      /* Issue #4162: Obstructing working copy. We can't access the working
         copy data from the parent working copy for this node by just using
         local_abspath */

      if (notify_func)
        {
          svn_wc_notify_t *notify = svn_wc_create_notify(
                                        local_abspath,
                                        svn_wc_notify_update_skip_obstruction,
                                        scratch_pool);

          notify_func(notify_baton, notify, scratch_pool);
        }

      return SVN_NO_ERROR; /* We don't revert obstructing working copies */
    }

  SVN_ERR(svn_wc__db_revert_list_read(&notify_required,
                                      &conflict_old, &conflict_new,
                                      &conflict_working, &prop_reject,
                                      &copied_here, &reverted_kind,
                                      db, local_abspath,
                                      scratch_pool, scratch_pool));

  err = svn_wc__db_read_info(&status, &kind,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             &recorded_size, &recorded_mod_time, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             db, local_abspath, scratch_pool, scratch_pool);

  if (err && err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
    {
      svn_error_clear(err);

      if (!copied_here)
        {
          if (notify_func && notify_required)
            notify_func(notify_baton,
                        svn_wc_create_notify(local_abspath,
                                             svn_wc_notify_revert,
                                             scratch_pool),
                        scratch_pool);

          if (notify_func)
            SVN_ERR(svn_wc__db_revert_list_notify(notify_func, notify_baton,
                                                  db, local_abspath,
                                                  scratch_pool));
          return SVN_NO_ERROR;
        }
      else
        {
          /* ### Initialise to values which prevent the code below from
           * ### trying to restore anything to disk.
           * ### 'status' should be status_unknown but that doesn't exist. */
          status = svn_wc__db_status_normal;
          kind = svn_wc__db_kind_unknown;
          recorded_size = SVN_INVALID_FILESIZE;
          recorded_mod_time = 0;
        }
    }
  else if (err)
    return svn_error_trace(err);

  err = svn_io_stat(&finfo, local_abspath,
                    APR_FINFO_TYPE | APR_FINFO_LINK
                    | APR_FINFO_SIZE | APR_FINFO_MTIME
                    | SVN__APR_FINFO_EXECUTABLE
                    | SVN__APR_FINFO_READONLY,
                    scratch_pool);

  if (err && (APR_STATUS_IS_ENOENT(err->apr_err)
              || SVN__APR_STATUS_IS_ENOTDIR(err->apr_err)))
    {
      svn_error_clear(err);
      on_disk = svn_node_none;
#ifdef HAVE_SYMLINK
      special = FALSE;
#endif
    }
  else
    {
      if (finfo.filetype == APR_REG || finfo.filetype == APR_LNK)
        on_disk = svn_node_file;
      else if (finfo.filetype == APR_DIR)
        on_disk = svn_node_dir;
      else
        on_disk = svn_node_unknown;

#ifdef HAVE_SYMLINK
      special = (finfo.filetype == APR_LNK);
#endif
    }

  if (copied_here)
    {
      /* The revert target itself is the op-root of a copy. */
      if (reverted_kind == svn_wc__db_kind_file && on_disk == svn_node_file)
        {
          SVN_ERR(svn_io_remove_file2(local_abspath, TRUE, scratch_pool));
          on_disk = svn_node_none;
        }
      else if (reverted_kind == svn_wc__db_kind_dir && on_disk == svn_node_dir)
        {
          svn_boolean_t removed;

          SVN_ERR(revert_restore_handle_copied_dirs(&removed, db,
                                                    local_abspath, TRUE, 
                                                    cancel_func, cancel_baton,
                                                    scratch_pool));
          if (removed)
            on_disk = svn_node_none;
        }
    }

  /* If we expect a versioned item to be present then check that any
     item on disk matches the versioned item, if it doesn't match then
     fix it or delete it.  */
  if (on_disk != svn_node_none
      && status != svn_wc__db_status_server_excluded
      && status != svn_wc__db_status_deleted
      && status != svn_wc__db_status_excluded
      && status != svn_wc__db_status_not_present)
    {
      if (on_disk == svn_node_dir && kind != svn_wc__db_kind_dir)
        {
          SVN_ERR(svn_io_remove_dir2(local_abspath, FALSE,
                                     cancel_func, cancel_baton, scratch_pool));
          on_disk = svn_node_none;
        }
      else if (on_disk == svn_node_file && kind != svn_wc__db_kind_file)
        {
#ifdef HAVE_SYMLINK
          /* Preserve symlinks pointing at directories. Changes on the
           * directory node have been reverted. The symlink should remain. */
          if (!(special && kind == svn_wc__db_kind_dir))
#endif
            {
              SVN_ERR(svn_io_remove_file2(local_abspath, FALSE, scratch_pool));
              on_disk = svn_node_none;
            }
        }
      else if (on_disk == svn_node_file)
        {
          svn_boolean_t modified;
          apr_hash_t *props;
#ifdef HAVE_SYMLINK
          svn_string_t *special_prop;
#endif

          SVN_ERR(svn_wc__db_read_pristine_props(&props, db, local_abspath,
                                                 scratch_pool, scratch_pool));

#ifdef HAVE_SYMLINK
          special_prop = apr_hash_get(props, SVN_PROP_SPECIAL,
                                      APR_HASH_KEY_STRING);

          if ((special_prop != NULL) != special)
            {
              /* File/symlink mismatch. */
              SVN_ERR(svn_io_remove_file2(local_abspath, FALSE, scratch_pool));
              on_disk = svn_node_none;
            }
          else
#endif
            {
              /* Issue #1663 asserts that we should compare a file in its
                 working copy format here, but before r1101473 we would only
                 do that if the file was already unequal to its recorded
                 information.

                 r1101473 removes the option of asking for a working format
                 compare but *also* check the recorded information first, as
                 that combination doesn't guarantee a stable behavior.
                 (See the revert_test.py: revert_reexpand_keyword)

                 But to have the same issue #1663 behavior for revert as we
                 had in <=1.6 we only have to check the recorded information
                 ourselves. And we already have everything we need, because
                 we called stat ourselves. */
              if (recorded_size != SVN_INVALID_FILESIZE
                  && recorded_mod_time != 0
                  && recorded_size == finfo.size
                  && recorded_mod_time == finfo.mtime)
                {
                  modified = FALSE;
                }
              else
                SVN_ERR(svn_wc__internal_file_modified_p(&modified,
                                                         db, local_abspath,
                                                         TRUE, scratch_pool));

              if (modified)
                {
                  SVN_ERR(svn_io_remove_file2(local_abspath, FALSE,
                                              scratch_pool));
                  on_disk = svn_node_none;
                }
              else
                {
                  svn_boolean_t read_only;
                  svn_string_t *needs_lock_prop;

                  SVN_ERR(svn_io__is_finfo_read_only(&read_only, &finfo,
                                                     scratch_pool));

                  needs_lock_prop = apr_hash_get(props, SVN_PROP_NEEDS_LOCK,
                                                 APR_HASH_KEY_STRING);
                  if (needs_lock_prop && !read_only)
                    {
                      SVN_ERR(svn_io_set_file_read_only(local_abspath,
                                                        FALSE, scratch_pool));
                      notify_required = TRUE;
                    }
                  else if (!needs_lock_prop && read_only)
                    {
                      SVN_ERR(svn_io_set_file_read_write(local_abspath,
                                                         FALSE, scratch_pool));
                      notify_required = TRUE;
                    }

#if !defined(WIN32) && !defined(__OS2__)
#ifdef HAVE_SYMLINK
                  if (!special)
#endif
                    {
                      svn_boolean_t executable;
                      svn_string_t *executable_prop;

                      SVN_ERR(svn_io__is_finfo_executable(&executable, &finfo,
                                                          scratch_pool));
                      executable_prop = apr_hash_get(props, SVN_PROP_EXECUTABLE,
                                                     APR_HASH_KEY_STRING);
                      if (executable_prop && !executable)
                        {
                          SVN_ERR(svn_io_set_file_executable(local_abspath,
                                                             TRUE, FALSE,
                                                             scratch_pool));
                          notify_required = TRUE;
                        }
                      else if (!executable_prop && executable)
                        {
                          SVN_ERR(svn_io_set_file_executable(local_abspath,
                                                             FALSE, FALSE,
                                                             scratch_pool));
                          notify_required = TRUE;
                        }
                    }
#endif
                }
            }
        }
    }

  /* If we expect a versioned item to be present and there is nothing
     on disk then recreate it. */
  if (on_disk == svn_node_none
      && status != svn_wc__db_status_server_excluded
      && status != svn_wc__db_status_deleted
      && status != svn_wc__db_status_excluded
      && status != svn_wc__db_status_not_present)
    {
      if (kind == svn_wc__db_kind_dir)
        SVN_ERR(svn_io_dir_make(local_abspath, APR_OS_DEFAULT, scratch_pool));

      if (kind == svn_wc__db_kind_file)
        {
          svn_skel_t *work_item;

          /* ### Get the checksum from read_info above and pass in here? */
          SVN_ERR(svn_wc__wq_build_file_install(&work_item, db, local_abspath,
                                                NULL, use_commit_times, TRUE,
                                                scratch_pool, scratch_pool));
          SVN_ERR(svn_wc__db_wq_add(db, local_abspath, work_item,
                                    scratch_pool));
          SVN_ERR(svn_wc__wq_run(db, local_abspath, cancel_func, cancel_baton,
                                 scratch_pool));
        }
      notify_required = TRUE;
    }

  SVN_ERR(remove_conflict_file(&notify_required, conflict_old,
                               local_abspath, scratch_pool));
  SVN_ERR(remove_conflict_file(&notify_required, conflict_new,
                               local_abspath, scratch_pool));
  SVN_ERR(remove_conflict_file(&notify_required, conflict_working,
                               local_abspath, scratch_pool));
  SVN_ERR(remove_conflict_file(&notify_required, prop_reject,
                               local_abspath, scratch_pool));

  if (notify_func && notify_required)
    notify_func(notify_baton,
                svn_wc_create_notify(local_abspath, svn_wc_notify_revert,
                                     scratch_pool),
                scratch_pool);

  if (depth == svn_depth_infinity && kind == svn_wc__db_kind_dir)
    {
      apr_pool_t *iterpool = svn_pool_create(scratch_pool);
      const apr_array_header_t *children;
      int i;

      SVN_ERR(revert_restore_handle_copied_dirs(NULL, db, local_abspath, FALSE,
                                                cancel_func, cancel_baton,
                                                iterpool));

      SVN_ERR(svn_wc__db_read_children_of_working_node(&children, db,
                                                       local_abspath,
                                                       scratch_pool,
                                                       iterpool));
      for (i = 0; i < children->nelts; ++i)
        {
          const char *child_abspath;

          svn_pool_clear(iterpool);

          child_abspath = svn_dirent_join(local_abspath,
                                          APR_ARRAY_IDX(children, i,
                                                        const char *),
                                          iterpool);

          SVN_ERR(revert_restore(db, child_abspath, depth,
                                 use_commit_times, FALSE /* revert root */,
                                 cancel_func, cancel_baton,
                                 notify_func, notify_baton,
                                 iterpool));
        }

      svn_pool_destroy(iterpool);
    }

  if (notify_func)
    SVN_ERR(svn_wc__db_revert_list_notify(notify_func, notify_baton,
                                          db, local_abspath, scratch_pool));
  return SVN_NO_ERROR;
}


/* Revert tree LOCAL_ABSPATH to depth DEPTH and notify for all
   reverts. */
static svn_error_t *
new_revert_internal(svn_wc__db_t *db,
                    const char *local_abspath,
                    svn_depth_t depth,
                    svn_boolean_t use_commit_times,
                    svn_cancel_func_t cancel_func,
                    void *cancel_baton,
                    svn_wc_notify_func2_t notify_func,
                    void *notify_baton,
                    apr_pool_t *scratch_pool)
{
  svn_error_t *err;

  SVN_ERR_ASSERT(depth == svn_depth_empty || depth == svn_depth_infinity);

  /* We should have a write lock on the parent of local_abspath, except
     when local_abspath is the working copy root. */
  {
    const char *dir_abspath;

    SVN_ERR(svn_wc__db_get_wcroot(&dir_abspath, db, local_abspath,
                                  scratch_pool, scratch_pool));

    if (svn_dirent_is_child(dir_abspath, local_abspath, NULL))
      dir_abspath = svn_dirent_dirname(local_abspath, scratch_pool);

    SVN_ERR(svn_wc__write_check(db, dir_abspath, scratch_pool));
  }

  err = svn_wc__db_op_revert(db, local_abspath, depth,
                             scratch_pool, scratch_pool);

  if (!err)
    err = revert_restore(db, local_abspath, depth,
                         use_commit_times, TRUE /* revert root */,
                         cancel_func, cancel_baton,
                         notify_func, notify_baton,
                         scratch_pool);

  err = svn_error_compose_create(err,
                                 svn_wc__db_revert_list_done(db,
                                                             local_abspath,
                                                             scratch_pool));

  return err;
}


/* Revert files in LOCAL_ABSPATH to depth DEPTH that match
   CHANGELIST_HASH and notify for all reverts. */
static svn_error_t *
new_revert_changelist(svn_wc__db_t *db,
                      const char *local_abspath,
                      svn_depth_t depth,
                      svn_boolean_t use_commit_times,
                      apr_hash_t *changelist_hash,
                      svn_cancel_func_t cancel_func,
                      void *cancel_baton,
                      svn_wc_notify_func2_t notify_func,
                      void *notify_baton,
                      apr_pool_t *scratch_pool)
{
  apr_pool_t *iterpool;
  const apr_array_header_t *children;
  int i;

  if (cancel_func)
    SVN_ERR(cancel_func(cancel_baton));

  /* Revert this node (depth=empty) if it matches one of the changelists.  */
  if (svn_wc__internal_changelist_match(db, local_abspath, changelist_hash,
                                        scratch_pool))
    SVN_ERR(new_revert_internal(db, local_abspath,
                                svn_depth_empty, use_commit_times,
                                cancel_func, cancel_baton,
                                notify_func, notify_baton,
                                scratch_pool));

  if (depth == svn_depth_empty)
    return SVN_NO_ERROR;

  iterpool = svn_pool_create(scratch_pool);

  /* We can handle both depth=files and depth=immediates by setting
     depth=empty here.  We don't need to distinguish files and
     directories when making the recursive call because directories
     can never match a changelist, so making the recursive call for
     directories when asked for depth=files is a no-op. */
  if (depth == svn_depth_files || depth == svn_depth_immediates)
    depth = svn_depth_empty;

  SVN_ERR(svn_wc__db_read_children_of_working_node(&children, db,
                                                   local_abspath,
                                                   scratch_pool,
                                                   iterpool));
  for (i = 0; i < children->nelts; ++i)
    {
      const char *child_abspath;

      svn_pool_clear(iterpool);

      child_abspath = svn_dirent_join(local_abspath,
                                      APR_ARRAY_IDX(children, i,
                                                    const char *),
                                      iterpool);

      SVN_ERR(new_revert_changelist(db, child_abspath, depth,
                                    use_commit_times, changelist_hash,
                                    cancel_func, cancel_baton,
                                    notify_func, notify_baton,
                                    iterpool));
    }

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}


/* Does a partially recursive revert of LOCAL_ABSPATH to depth DEPTH
   (which must be either svn_depth_files or svn_depth_immediates) by
   doing a non-recursive revert on each permissible path.  Notifies
   all reverted paths.

   ### This won't revert a copied dir with one level of children since
   ### the non-recursive revert on the dir will fail.  Not sure how a
   ### partially recursive revert should handle actual-only nodes. */
static svn_error_t *
new_revert_partial(svn_wc__db_t *db,
                   const char *local_abspath,
                   svn_depth_t depth,
                   svn_boolean_t use_commit_times,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   svn_wc_notify_func2_t notify_func,
                   void *notify_baton,
                   apr_pool_t *scratch_pool)
{
  apr_pool_t *iterpool;
  const apr_array_header_t *children;
  int i;

  SVN_ERR_ASSERT(depth == svn_depth_files || depth == svn_depth_immediates);

  if (cancel_func)
    SVN_ERR(cancel_func(cancel_baton));

  iterpool = svn_pool_create(scratch_pool);

  /* Revert the root node itself (depth=empty), then move on to the
     children.  */
  SVN_ERR(new_revert_internal(db, local_abspath, svn_depth_empty,
                              use_commit_times, cancel_func, cancel_baton,
                              notify_func, notify_baton, iterpool));

  SVN_ERR(svn_wc__db_read_children_of_working_node(&children, db,
                                                   local_abspath,
                                                   scratch_pool,
                                                   iterpool));
  for (i = 0; i < children->nelts; ++i)
    {
      const char *child_abspath;

      svn_pool_clear(iterpool);

      child_abspath = svn_dirent_join(local_abspath,
                                      APR_ARRAY_IDX(children, i, const char *),
                                      iterpool);

      /* For svn_depth_files: don't revert non-files.  */
      if (depth == svn_depth_files)
        {
          svn_wc__db_kind_t kind;

          SVN_ERR(svn_wc__db_read_kind(&kind, db, child_abspath, TRUE,
                                       iterpool));
          if (kind != svn_wc__db_kind_file)
            continue;
        }

      /* Revert just this node (depth=empty).  */
      SVN_ERR(new_revert_internal(db, child_abspath,
                                  svn_depth_empty, use_commit_times,
                                  cancel_func, cancel_baton,
                                  notify_func, notify_baton,
                                  iterpool));
    }

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_revert4(svn_wc_context_t *wc_ctx,
               const char *local_abspath,
               svn_depth_t depth,
               svn_boolean_t use_commit_times,
               const apr_array_header_t *changelist_filter,
               svn_cancel_func_t cancel_func,
               void *cancel_baton,
               svn_wc_notify_func2_t notify_func,
               void *notify_baton,
               apr_pool_t *scratch_pool)
{
  if (changelist_filter && changelist_filter->nelts)
    {
      apr_hash_t *changelist_hash;

      SVN_ERR(svn_hash_from_cstring_keys(&changelist_hash, changelist_filter,
                                         scratch_pool));
      return svn_error_trace(new_revert_changelist(wc_ctx->db, local_abspath,
                                                   depth, use_commit_times,
                                                   changelist_hash,
                                                   cancel_func, cancel_baton,
                                                   notify_func, notify_baton,
                                                   scratch_pool));
    }

  if (depth == svn_depth_empty || depth == svn_depth_infinity)
    return svn_error_trace(new_revert_internal(wc_ctx->db, local_abspath,
                                               depth, use_commit_times,
                                               cancel_func, cancel_baton,
                                               notify_func, notify_baton,
                                               scratch_pool));

  /* The user may expect svn_depth_files/svn_depth_immediates to work
     on copied dirs with one level of children.  It doesn't, the user
     will get an error and will need to invoke an infinite revert.  If
     we identified those cases where svn_depth_infinity would not
     revert too much we could invoke the recursive call above. */

  if (depth == svn_depth_files || depth == svn_depth_immediates)
    return svn_error_trace(new_revert_partial(wc_ctx->db, local_abspath,
                                              depth, use_commit_times,
                                              cancel_func, cancel_baton,
                                              notify_func, notify_baton,
                                              scratch_pool));

  /* Bogus depth. Tell the caller.  */
  return svn_error_create(SVN_ERR_WC_INVALID_OPERATION_DEPTH, NULL, NULL);
}


/* Return a path where nothing exists on disk, within the admin directory
   belonging to the WCROOT_ABSPATH directory.  */
static const char *
nonexistent_path(const char *wcroot_abspath, apr_pool_t *scratch_pool)
{
  return svn_wc__adm_child(wcroot_abspath, SVN_WC__ADM_NONEXISTENT_PATH,
                           scratch_pool);
}


svn_error_t *
svn_wc_get_pristine_copy_path(const char *path,
                              const char **pristine_path,
                              apr_pool_t *pool)
{
  svn_wc__db_t *db;
  const char *local_abspath;
  svn_error_t *err;

  SVN_ERR(svn_dirent_get_absolute(&local_abspath, path, pool));

  SVN_ERR(svn_wc__db_open(&db, NULL, TRUE, TRUE, pool, pool));
  /* DB is now open. This is seemingly a "light" function that a caller
     may use repeatedly despite error return values. The rest of this
     function should aggressively close DB, even in the error case.  */

  err = svn_wc__text_base_path_to_read(pristine_path, db, local_abspath,
                                       pool, pool);
  if (err && err->apr_err == SVN_ERR_WC_PATH_UNEXPECTED_STATUS)
    {
      /* The node doesn't exist, so return a non-existent path located
         in WCROOT/.svn/  */
      const char *wcroot_abspath;

      svn_error_clear(err);

      err = svn_wc__db_get_wcroot(&wcroot_abspath, db, local_abspath,
                                  pool, pool);
      if (err == NULL)
        *pristine_path = nonexistent_path(wcroot_abspath, pool);
    }

   return svn_error_compose_create(err, svn_wc__db_close(db));
}


svn_error_t *
svn_wc_get_pristine_contents2(svn_stream_t **contents,
                              svn_wc_context_t *wc_ctx,
                              const char *local_abspath,
                              apr_pool_t *result_pool,
                              apr_pool_t *scratch_pool)
{
  return svn_error_trace(svn_wc__get_pristine_contents(contents, NULL,
                                                       wc_ctx->db,
                                                       local_abspath,
                                                       result_pool,
                                                       scratch_pool));
}


svn_error_t *
svn_wc__internal_remove_from_revision_control(svn_wc__db_t *db,
                                              const char *local_abspath,
                                              svn_boolean_t destroy_wf,
                                              svn_boolean_t instant_error,
                                              svn_cancel_func_t cancel_func,
                                              void *cancel_baton,
                                              apr_pool_t *scratch_pool)
{
  svn_error_t *err;
  svn_boolean_t left_something = FALSE;
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;

  /* ### This whole function should be rewritten to run inside a transaction,
     ### to allow a stable cancel behavior.
     ###
     ### Subversion < 1.7 marked the directory as incomplete to allow updating
     ### it from a canceled state. But this would not work because update
     ### doesn't retrieve deleted items.
     ###
     ### WC-NG doesn't support a delete+incomplete state, but we can't build
     ### transactions over multiple databases yet. */

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  /* Check cancellation here, so recursive calls get checked early. */
  if (cancel_func)
    SVN_ERR(cancel_func(cancel_baton));

  SVN_ERR(svn_wc__db_read_info(&status, &kind, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL,
                               db, local_abspath, scratch_pool, scratch_pool));

  if (kind == svn_wc__db_kind_file || kind == svn_wc__db_kind_symlink)
    {
      svn_boolean_t text_modified_p = FALSE;

      if (instant_error || destroy_wf)
        {
          svn_node_kind_t on_disk;
          SVN_ERR(svn_io_check_path(local_abspath, &on_disk, scratch_pool));
          if (on_disk == svn_node_file)
            {
              /* Check for local mods. before removing entry */
              SVN_ERR(svn_wc__internal_file_modified_p(&text_modified_p, db,
                                                       local_abspath, FALSE,
                                                       scratch_pool));
              if (text_modified_p && instant_error)
                return svn_error_createf(SVN_ERR_WC_LEFT_LOCAL_MOD, NULL,
                       _("File '%s' has local modifications"),
                       svn_dirent_local_style(local_abspath, scratch_pool));
            }
        }

      /* Remove NAME from DB */
      SVN_ERR(svn_wc__db_op_remove_node(db, local_abspath,
                                        SVN_INVALID_REVNUM,
                                        svn_wc__db_kind_unknown,
                                        scratch_pool));

      /* If we were asked to destroy the working file, do so unless
         it has local mods. */
      if (destroy_wf)
        {
          /* Don't kill local mods. */
          if (text_modified_p)
            return svn_error_create(SVN_ERR_WC_LEFT_LOCAL_MOD, NULL, NULL);
          else  /* The working file is still present; remove it. */
            SVN_ERR(svn_io_remove_file2(local_abspath, TRUE, scratch_pool));
        }

    }  /* done with file case */
  else /* looking at THIS_DIR */
    {
      apr_pool_t *iterpool = svn_pool_create(scratch_pool);
      const apr_array_header_t *children;
      int i;

      /* ### sanity check:  check 2 places for DELETED flag? */

      /* Walk over every entry. */
      SVN_ERR(svn_wc__db_read_children(&children, db, local_abspath,
                                       scratch_pool, iterpool));

      for (i = 0; i < children->nelts; i++)
        {
          const char *node_name = APR_ARRAY_IDX(children, i, const char*);
          const char *node_abspath;
          svn_wc__db_status_t node_status;
          svn_wc__db_kind_t node_kind;

          svn_pool_clear(iterpool);

          node_abspath = svn_dirent_join(local_abspath, node_name, iterpool);

          SVN_ERR(svn_wc__db_read_info(&node_status, &node_kind, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL,
                                       db, node_abspath,
                                       iterpool, iterpool));

          if (node_status == svn_wc__db_status_normal
              && node_kind == svn_wc__db_kind_dir)
            {
              svn_boolean_t is_root;

              SVN_ERR(svn_wc__check_wc_root(&is_root, NULL, NULL,
                                            db, node_abspath, iterpool));

              if (is_root)
                continue; /* Just skip working copies as obstruction */
            }

          if (node_status != svn_wc__db_status_normal
              && node_status != svn_wc__db_status_added
              && node_status != svn_wc__db_status_incomplete)
            {
              /* The node is already 'deleted', so nothing to do on
                 versioned nodes */
              SVN_ERR(svn_wc__db_op_remove_node(db, node_abspath,
                                                SVN_INVALID_REVNUM,
                                                svn_wc__db_kind_unknown,
                                                iterpool));

              continue;
            }

          err = svn_wc__internal_remove_from_revision_control(
                            db, node_abspath,
                            destroy_wf, instant_error,
                            cancel_func, cancel_baton,
                            iterpool);

          if (err && (err->apr_err == SVN_ERR_WC_LEFT_LOCAL_MOD))
            {
              if (instant_error)
                return svn_error_trace(err);
              else
                {
                  svn_error_clear(err);
                  left_something = TRUE;
                }
            }
          else if (err)
            return svn_error_trace(err);
        }

      /* At this point, every directory below this one has been
         removed from revision control. */

      /* Remove self from parent's entries file, but only if parent is
         a working copy.  If it's not, that's fine, we just move on. */
      {
        svn_boolean_t is_root;

        SVN_ERR(svn_wc__check_wc_root(&is_root, NULL, NULL,
                                      db, local_abspath, iterpool));

        /* If full_path is not the top of a wc, then its parent
           directory is also a working copy and has an entry for
           full_path.  We need to remove that entry: */
        if (! is_root)
          {
            SVN_ERR(svn_wc__db_op_remove_node(db, local_abspath,
                                              SVN_INVALID_REVNUM,
                                              svn_wc__db_kind_unknown,
                                              iterpool));
          }
        else
          {
            /* Remove the entire administrative .svn area, thereby removing
               _this_ dir from revision control too.  */
            SVN_ERR(svn_wc__adm_destroy(db, local_abspath,
                                        cancel_func, cancel_baton, iterpool));
          }
      }

      /* If caller wants us to recursively nuke everything on disk, go
         ahead, provided that there are no dangling local-mod files
         below */
      if (destroy_wf && (! left_something))
        {
          /* If the dir is *truly* empty (i.e. has no unversioned
             resources, all versioned files are gone, all .svn dirs are
             gone, and contains nothing but empty dirs), then a
             *non*-recursive dir_remove should work.  If it doesn't,
             no big deal.  Just assume there are unversioned items in
             there and set "left_something" */
          err = svn_io_dir_remove_nonrecursive(local_abspath, iterpool);
          if (err)
            {
              if (!APR_STATUS_IS_ENOENT(err->apr_err))
                left_something = TRUE;
              svn_error_clear(err);
            }
        }

      svn_pool_destroy(iterpool);

    }  /* end of directory case */

  if (left_something)
    return svn_error_create(SVN_ERR_WC_LEFT_LOCAL_MOD, NULL, NULL);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_remove_from_revision_control2(svn_wc_context_t *wc_ctx,
                                    const char *local_abspath,
                                    svn_boolean_t destroy_wf,
                                    svn_boolean_t instant_error,
                                    svn_cancel_func_t cancel_func,
                                    void *cancel_baton,
                                    apr_pool_t *scratch_pool)
{
  return svn_error_trace(
      svn_wc__internal_remove_from_revision_control(wc_ctx->db,
                                                    local_abspath,
                                                    destroy_wf,
                                                    instant_error,
                                                    cancel_func,
                                                    cancel_baton,
                                                    scratch_pool));
}


svn_error_t *
svn_wc_add_lock2(svn_wc_context_t *wc_ctx,
                 const char *local_abspath,
                 const svn_lock_t *lock,
                 apr_pool_t *scratch_pool)
{
  svn_wc__db_lock_t db_lock;
  svn_error_t *err;
  const svn_string_t *needs_lock;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  db_lock.token = lock->token;
  db_lock.owner = lock->owner;
  db_lock.comment = lock->comment;
  db_lock.date = lock->creation_date;
  err = svn_wc__db_lock_add(wc_ctx->db, local_abspath, &db_lock, scratch_pool);
  if (err)
    {
      if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
        return svn_error_trace(err);

      /* Remap the error.  */
      svn_error_clear(err);
      return svn_error_createf(SVN_ERR_ENTRY_NOT_FOUND, NULL,
                               _("'%s' is not under version control"),
                               svn_dirent_local_style(local_abspath,
                                                      scratch_pool));
    }

  /* if svn:needs-lock is present, then make the file read-write. */
  SVN_ERR(svn_wc__internal_propget(&needs_lock, wc_ctx->db, local_abspath,
                                   SVN_PROP_NEEDS_LOCK, scratch_pool,
                                   scratch_pool));
  if (needs_lock)
    SVN_ERR(svn_io_set_file_read_write(local_abspath, FALSE, scratch_pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_remove_lock2(svn_wc_context_t *wc_ctx,
                    const char *local_abspath,
                    apr_pool_t *scratch_pool)
{
  svn_error_t *err;
  const svn_string_t *needs_lock;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  err = svn_wc__db_lock_remove(wc_ctx->db, local_abspath, scratch_pool);
  if (err)
    {
      if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND)
        return svn_error_trace(err);

      /* Remap the error.  */
      svn_error_clear(err);
      return svn_error_createf(SVN_ERR_ENTRY_NOT_FOUND, NULL,
                               _("'%s' is not under version control"),
                               svn_dirent_local_style(local_abspath,
                                                      scratch_pool));
    }

  /* if svn:needs-lock is present, then make the file read-only. */
  SVN_ERR(svn_wc__internal_propget(&needs_lock, wc_ctx->db, local_abspath,
                                   SVN_PROP_NEEDS_LOCK, scratch_pool,
                                   scratch_pool));
  if (needs_lock)
    SVN_ERR(svn_io_set_file_read_only(local_abspath, FALSE, scratch_pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_set_changelist2(svn_wc_context_t *wc_ctx,
                       const char *local_abspath,
                       const char *new_changelist,
                       svn_depth_t depth,
                       const apr_array_header_t *changelist_filter,
                       svn_cancel_func_t cancel_func,
                       void *cancel_baton,
                       svn_wc_notify_func2_t notify_func,
                       void *notify_baton,
                       apr_pool_t *scratch_pool)
{
  /* Assert that we aren't being asked to set an empty changelist. */
  SVN_ERR_ASSERT(! (new_changelist && new_changelist[0] == '\0'));

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  SVN_ERR(svn_wc__db_op_set_changelist(wc_ctx->db, local_abspath,
                                       new_changelist, changelist_filter,
                                       depth, notify_func, notify_baton,
                                       cancel_func, cancel_baton,
                                       scratch_pool));

  return SVN_NO_ERROR;
}

struct get_cl_fn_baton
{
  svn_wc__db_t *db;
  apr_hash_t *clhash;
  svn_changelist_receiver_t callback_func;
  void *callback_baton;
};


static svn_error_t *
get_node_changelist(const char *local_abspath,
                    svn_node_kind_t kind,
                    void *baton,
                    apr_pool_t *scratch_pool)
{
  struct get_cl_fn_baton *b = baton;
  const char *changelist;

  SVN_ERR(svn_wc__db_read_info(NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, &changelist,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               b->db, local_abspath,
                               scratch_pool, scratch_pool));

  if (svn_wc__internal_changelist_match(b->db, local_abspath, b->clhash,
                                        scratch_pool))
    SVN_ERR(b->callback_func(b->callback_baton, local_abspath,
                             changelist, scratch_pool));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc_get_changelists(svn_wc_context_t *wc_ctx,
                       const char *local_abspath,
                       svn_depth_t depth,
                       const apr_array_header_t *changelist_filter,
                       svn_changelist_receiver_t callback_func,
                       void *callback_baton,
                       svn_cancel_func_t cancel_func,
                       void *cancel_baton,
                       apr_pool_t *scratch_pool)
{
  struct get_cl_fn_baton gnb;
  gnb.db = wc_ctx->db;
  gnb.clhash = NULL;
  gnb.callback_func = callback_func;
  gnb.callback_baton = callback_baton;

  if (changelist_filter)
    SVN_ERR(svn_hash_from_cstring_keys(&gnb.clhash, changelist_filter,
                                       scratch_pool));

  return svn_error_trace(
    svn_wc__internal_walk_children(wc_ctx->db, local_abspath, FALSE,
                                   changelist_filter, get_node_changelist,
                                   &gnb, depth,
                                   cancel_func, cancel_baton,
                                   scratch_pool));

}


svn_boolean_t
svn_wc__internal_changelist_match(svn_wc__db_t *db,
                                  const char *local_abspath,
                                  const apr_hash_t *clhash,
                                  apr_pool_t *scratch_pool)
{
  svn_error_t *err;
  const char *changelist;

  if (clhash == NULL)
    return TRUE;

  err = svn_wc__db_read_info(NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL, &changelist,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             db, local_abspath, scratch_pool, scratch_pool);
  if (err)
    {
      svn_error_clear(err);
      return FALSE;
    }

  return (changelist
            && apr_hash_get((apr_hash_t *)clhash, changelist,
                            APR_HASH_KEY_STRING) != NULL);
}


svn_boolean_t
svn_wc__changelist_match(svn_wc_context_t *wc_ctx,
                         const char *local_abspath,
                         const apr_hash_t *clhash,
                         apr_pool_t *scratch_pool)
{
  return svn_wc__internal_changelist_match(wc_ctx->db, local_abspath, clhash,
                                           scratch_pool);
}
