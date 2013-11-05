/*
 * diff_pristine.c -- A simple diff walker which compares local files against
 *                    their pristine versions.
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
 *
 * This is the simple working copy diff algorithm which is used when you
 * just use 'svn diff PATH'. It shows what is modified in your working copy
 * since a node was checked out or copied but doesn't show most kinds of
 * restructuring operations.
 *
 * You can look at this as another form of the status walker.
 */

#include <apr_hash.h>

#include "svn_error.h"
#include "svn_pools.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_hash.h"

#include "private/svn_wc_private.h"

#include "wc.h"
#include "props.h"
#include "translate.h"

#include "svn_private_config.h"

/*-------------------------------------------------------------------------*/


/* The diff baton */
struct diff_baton
{
  /* A wc db. */
  svn_wc__db_t *db;

  /* Report editor paths relative from this directory */
  const char *anchor_abspath;

  /* The callbacks and callback argument that implement the file comparison
     functions */
  const svn_wc_diff_callbacks4_t *callbacks;
  void *callback_baton;

  /* Should this diff ignore node ancestry? */
  svn_boolean_t ignore_ancestry;

  /* Should this diff not compare copied files with their source? */
  svn_boolean_t show_copies_as_adds;

  /* Are we producing a git-style diff? */
  svn_boolean_t use_git_diff_format;

  /* Empty file used to diff adds / deletes */
  const char *empty_file;

  /* Hash whose keys are const char * changelist names. */
  apr_hash_t *changelist_hash;

  /* Cancel function/baton */
  svn_cancel_func_t cancel_func;
  void *cancel_baton;

  apr_pool_t *pool;
};

/* Get the empty file associated with the edit baton. This is cached so
 * that it can be reused, all empty files are the same.
 */
static svn_error_t *
get_empty_file(struct diff_baton *eb,
               const char **empty_file,
               apr_pool_t *scratch_pool)
{
  /* Create the file if it does not exist */
  /* Note that we tried to use /dev/null in r857294, but
     that won't work on Windows: it's impossible to stat NUL */
  if (!eb->empty_file)
    {
      SVN_ERR(svn_io_open_unique_file3(NULL, &eb->empty_file, NULL,
                                       svn_io_file_del_on_pool_cleanup,
                                       eb->pool, scratch_pool));
    }

  *empty_file = eb->empty_file;

  return SVN_NO_ERROR;
}


/* Return the value of the svn:mime-type property held in PROPS, or NULL
   if no such property exists. */
static const char *
get_prop_mimetype(apr_hash_t *props)
{
  return svn_prop_get_value(props, SVN_PROP_MIME_TYPE);
}


/* Diff the file PATH against its text base.  At this
 * stage we are dealing with a file that does exist in the working copy.
 *
 * DIR_BATON is the parent directory baton, PATH is the path to the file to
 * be compared.
 *
 * Do all allocation in POOL.
 *
 * ### TODO: Need to work on replace if the new filename used to be a
 * directory.
 */
static svn_error_t *
file_diff(struct diff_baton *eb,
          const char *local_abspath,
          const char *path,
          apr_pool_t *scratch_pool)
{
  svn_wc__db_t *db = eb->db;
  const char *empty_file;
  const char *original_repos_relpath;
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;
  svn_revnum_t revision;
  const svn_checksum_t *checksum;
  svn_boolean_t op_root;
  svn_boolean_t had_props, props_mod;
  svn_boolean_t have_base, have_more_work;
  svn_boolean_t replaced = FALSE;
  svn_boolean_t base_replace = FALSE;
  svn_wc__db_status_t base_status;
  svn_revnum_t base_revision = SVN_INVALID_REVNUM;
  const svn_checksum_t *base_checksum;
  const char *pristine_abspath;

  SVN_ERR(svn_wc__db_read_info(&status, &kind, &revision, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, &checksum, NULL,
                               &original_repos_relpath, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL,
                               &op_root, &had_props, &props_mod,
                               &have_base, &have_more_work, NULL,
                               db, local_abspath, scratch_pool, scratch_pool));

  if ((status == svn_wc__db_status_added) && (have_base || have_more_work))
    {
      SVN_ERR(svn_wc__db_node_check_replace(&replaced, &base_replace,
                                            NULL, db, local_abspath,
                                            scratch_pool));

      if (replaced && base_replace /* && !have_more_work */)
        {
          svn_wc__db_kind_t base_kind;
          SVN_ERR(svn_wc__db_base_get_info(&base_status, &base_kind,
                                           &base_revision,
                                           NULL, NULL, NULL, NULL, NULL, NULL,
                                           NULL, &base_checksum, NULL,
                                           NULL, NULL, NULL,
                                           db, local_abspath,
                                           scratch_pool, scratch_pool));

          if (base_status != svn_wc__db_status_normal
              || base_kind != kind)
            {
              /* We can't show a replacement here */
              replaced = FALSE;
              base_replace = FALSE;
            }
        }
      else
        {
          /* We can't look in this middle working layer (yet).
             We just report the change itself.

             And if we could look at it, how would we report the addition
             of this middle layer (and maybe different layers below that)?

             For 1.7 we just do what we did before: Ignore this layering
             problem and just show how the current file got in your wc.
           */
          replaced = FALSE;
          base_replace = FALSE;
        }
    }

  /* Now refine ADDED to one of: ADDED, COPIED, MOVED_HERE. Note that only
     the latter two have corresponding pristine info to diff against.  */
  if (status == svn_wc__db_status_added)
    SVN_ERR(svn_wc__db_scan_addition(&status, NULL, NULL, NULL, NULL,
                                     NULL, NULL, NULL,
                                     NULL, db, local_abspath,
                                     scratch_pool, scratch_pool));

  SVN_ERR(get_empty_file(eb, &empty_file, scratch_pool));

  /* When we show a delete, we show a diff of the original pristine against
   * an empty file.
   * A base-replace is treated like a delete plus an add.
   *
   * For this kind of diff we prefer to show the deletion of what was checked
   * out over showing what was actually deleted (if that was defined by
   * a higher layer). */
  if (status == svn_wc__db_status_deleted ||
      (base_replace && ! eb->ignore_ancestry))
    {
      apr_hash_t *del_props;
      const svn_checksum_t *del_checksum;
      const char *del_text_abspath;
      const char *del_mimetype;

      if (base_replace && ! eb->ignore_ancestry)
        {
          /* We show a deletion of the information in the BASE layer */
          SVN_ERR(svn_wc__db_base_get_props(&del_props, db, local_abspath,
                                            scratch_pool, scratch_pool));

          del_checksum = base_checksum;
        }
      else
        {
          /* We show a deletion of what was actually deleted */
          SVN_ERR_ASSERT(status == svn_wc__db_status_deleted);

          SVN_ERR(svn_wc__get_pristine_props(&del_props, db, local_abspath,
                                             scratch_pool, scratch_pool));

          SVN_ERR(svn_wc__db_read_pristine_info(NULL, NULL, NULL, NULL, NULL,
                                                NULL, &del_checksum, NULL,
                                                NULL, db, local_abspath,
                                                scratch_pool, scratch_pool));
        }

      SVN_ERR_ASSERT(del_checksum != NULL);

      SVN_ERR(svn_wc__db_pristine_get_path(&del_text_abspath, db,
                                           local_abspath, del_checksum,
                                           scratch_pool, scratch_pool));

      if (del_props == NULL)
        del_props = apr_hash_make(scratch_pool);

      del_mimetype = get_prop_mimetype(del_props);

      SVN_ERR(eb->callbacks->file_deleted(NULL, NULL, path,
                                          del_text_abspath,
                                          empty_file,
                                          del_mimetype,
                                          NULL,
                                          del_props,
                                          eb->callback_baton,
                                          scratch_pool));

      if (status == svn_wc__db_status_deleted)
        {
          /* We're here only for showing a delete, so we're done. */
          return SVN_NO_ERROR;
        }
    }

  if (checksum != NULL)
    SVN_ERR(svn_wc__db_pristine_get_path(&pristine_abspath, db, local_abspath,
                                         checksum,
                                         scratch_pool, scratch_pool));
  else if (base_replace && eb->ignore_ancestry)
    SVN_ERR(svn_wc__db_pristine_get_path(&pristine_abspath, db, local_abspath,
                                         base_checksum,
                                         scratch_pool, scratch_pool));
  else
    pristine_abspath = empty_file;

 /* Now deal with showing additions, or the add-half of replacements.
  * If the item is schedule-add *with history*, then we usually want
  * to see the usual working vs. text-base comparison, which will show changes
  * made since the file was copied.  But in case we're showing copies as adds,
  * we need to compare the copied file to the empty file. If we're doing a git
  * diff, and the file was copied, we need to report the file as added and
  * diff it against the text base, so that a "copied" git diff header, and
  * possibly a diff against the copy source, will be generated for it. */
  if ((! base_replace && status == svn_wc__db_status_added) ||
     (base_replace && ! eb->ignore_ancestry) ||
     ((status == svn_wc__db_status_copied ||
       status == svn_wc__db_status_moved_here) &&
         (eb->show_copies_as_adds || eb->use_git_diff_format)))
    {
      const char *translated = NULL;
      apr_hash_t *pristine_props;
      apr_hash_t *actual_props;
      const char *actual_mimetype;
      apr_array_header_t *propchanges;


      /* Get svn:mime-type from ACTUAL props of PATH. */
      SVN_ERR(svn_wc__get_actual_props(&actual_props, db, local_abspath,
                                       scratch_pool, scratch_pool));
      actual_mimetype = get_prop_mimetype(actual_props);

      /* Set the original properties to empty, then compute "changes" from
         that. Essentially, all ACTUAL props will be "added".  */
      pristine_props = apr_hash_make(scratch_pool);
      SVN_ERR(svn_prop_diffs(&propchanges, actual_props, pristine_props,
                             scratch_pool));

      SVN_ERR(svn_wc__internal_translated_file(
              &translated, local_abspath, db, local_abspath,
              SVN_WC_TRANSLATE_TO_NF | SVN_WC_TRANSLATE_USE_GLOBAL_TMP,
              eb->cancel_func, eb->cancel_baton,
              scratch_pool, scratch_pool));

      SVN_ERR(eb->callbacks->file_added(NULL, NULL, NULL, path,
                                        (! eb->show_copies_as_adds &&
                                         eb->use_git_diff_format &&
                                         status != svn_wc__db_status_added) ?
                                          pristine_abspath : empty_file,
                                        translated,
                                        0, revision,
                                        NULL,
                                        actual_mimetype,
                                        original_repos_relpath,
                                        SVN_INVALID_REVNUM, propchanges,
                                        pristine_props, eb->callback_baton,
                                        scratch_pool));
    }
  else
    {
      const char *translated = NULL;
      apr_hash_t *pristine_props;
      const char *pristine_mimetype;
      const char *actual_mimetype;
      apr_hash_t *actual_props;
      apr_array_header_t *propchanges;
      svn_boolean_t modified;

      /* Here we deal with showing pure modifications. */
      SVN_ERR(svn_wc__internal_file_modified_p(&modified, db, local_abspath,
                                               FALSE, scratch_pool));
      if (modified)
        {
          /* Note that this might be the _second_ time we translate
             the file, as svn_wc__text_modified_internal_p() might have used a
             tmp translated copy too.  But what the heck, diff is
             already expensive, translating twice for the sake of code
             modularity is liveable. */
          SVN_ERR(svn_wc__internal_translated_file(
                    &translated, local_abspath, db, local_abspath,
                    SVN_WC_TRANSLATE_TO_NF | SVN_WC_TRANSLATE_USE_GLOBAL_TMP,
                    eb->cancel_func, eb->cancel_baton,
                    scratch_pool, scratch_pool));
        }

      /* Get the properties, the svn:mime-type values, and compute the
         differences between the two.  */
      if (base_replace
          && eb->ignore_ancestry)
        {
          /* We don't want the normal pristine properties (which are
             from the WORKING tree). We want the pristines associated
             with the BASE tree, which are saved as "revert" props.  */
          SVN_ERR(svn_wc__db_base_get_props(&pristine_props,
                                            db, local_abspath,
                                            scratch_pool, scratch_pool));
        }
      else
        {
          /* We can only fetch the pristine props (from BASE or WORKING) if
             the node has not been replaced, or it was copied/moved here.  */
          SVN_ERR_ASSERT(!replaced
                         || status == svn_wc__db_status_copied
                         || status == svn_wc__db_status_moved_here);

          SVN_ERR(svn_wc__db_read_pristine_props(&pristine_props, db,
                                                 local_abspath,
                                                 scratch_pool, scratch_pool));

          /* baseprops will be NULL for added nodes */
          if (!pristine_props)
            pristine_props = apr_hash_make(scratch_pool);
        }
      pristine_mimetype = get_prop_mimetype(pristine_props);

      SVN_ERR(svn_wc__db_read_props(&actual_props, db, local_abspath,
                                    scratch_pool, scratch_pool));
      actual_mimetype = get_prop_mimetype(actual_props);

      SVN_ERR(svn_prop_diffs(&propchanges, actual_props, pristine_props,
                             scratch_pool));

      if (modified || propchanges->nelts > 0)
        {
          SVN_ERR(eb->callbacks->file_changed(NULL, NULL, NULL,
                                              path,
                                              modified ? pristine_abspath
                                                       : NULL,
                                              translated,
                                              revision,
                                              SVN_INVALID_REVNUM,
                                              pristine_mimetype,
                                              actual_mimetype,
                                              propchanges,
                                              pristine_props,
                                              eb->callback_baton,
                                              scratch_pool));
        }
    }

  return SVN_NO_ERROR;
}

/* Implements svn_wc_status_func3_t */
static svn_error_t *
diff_status_callback(void *baton,
                     const char *local_abspath,
                     const svn_wc_status3_t *status,
                     apr_pool_t *scratch_pool)
{
  struct diff_baton *eb = baton;
  switch (status->node_status)
    {
      case svn_wc_status_unversioned:
      case svn_wc_status_ignored:
        return SVN_NO_ERROR; /* No diff */

      case svn_wc_status_obstructed:
      case svn_wc_status_missing:
        return SVN_NO_ERROR; /* ### What should we do here? */

      default:
        break; /* Go check other conditions */
    }

  if (eb->changelist_hash != NULL
      && (!status->changelist
          || ! apr_hash_get(eb->changelist_hash, status->changelist,
                            APR_HASH_KEY_STRING)))
    return SVN_NO_ERROR; /* Filtered via changelist */

  /* ### The following checks should probably be reversed as it should decide
         when *not* to show a diff, because generally all changed nodes should
         have a diff. */
  if (status->kind == svn_node_file)
    {
      /* Show a diff when
       *   - The text is modified
       *   - Or the properties are modified
       *   - Or when the node has been replaced
       *   - Or (if in copies as adds or git mode) when a node is copied */
      if (status->text_status == svn_wc_status_modified
          || status->prop_status == svn_wc_status_modified
          || status->node_status == svn_wc_status_deleted
          || status->node_status == svn_wc_status_replaced
          || ((eb->show_copies_as_adds || eb->use_git_diff_format)
              && status->copied))
        {
          const char *path = svn_dirent_skip_ancestor(eb->anchor_abspath,
                                                      local_abspath);

          SVN_ERR(file_diff(eb, local_abspath, path, scratch_pool));
        }
    }
  else
    {
      /* ### This case should probably be extended for git-diff, but this
             is what the old diff code provided */
      if (status->node_status == svn_wc_status_deleted
          || status->node_status == svn_wc_status_replaced
          || status->prop_status == svn_wc_status_modified)
        {
          apr_array_header_t *propchanges;
          apr_hash_t *baseprops;
          const char *path = svn_dirent_skip_ancestor(eb->anchor_abspath,
                                                      local_abspath);


          SVN_ERR(svn_wc__internal_propdiff(&propchanges, &baseprops,
                                            eb->db, local_abspath,
                                            scratch_pool, scratch_pool));

          SVN_ERR(eb->callbacks->dir_props_changed(NULL, NULL,
                                                   path, FALSE /* ### ? */,
                                                   propchanges, baseprops,
                                                   eb->callback_baton,
                                                   scratch_pool));
        }
    }
  return SVN_NO_ERROR;
}


/* Public Interface */
svn_error_t *
svn_wc_diff6(svn_wc_context_t *wc_ctx,
             const char *local_abspath,
             const svn_wc_diff_callbacks4_t *callbacks,
             void *callback_baton,
             svn_depth_t depth,
             svn_boolean_t ignore_ancestry,
             svn_boolean_t show_copies_as_adds,
             svn_boolean_t use_git_diff_format,
             const apr_array_header_t *changelist_filter,
             svn_cancel_func_t cancel_func,
             void *cancel_baton,
             apr_pool_t *scratch_pool)
{
  struct diff_baton eb = { 0 };
  svn_wc__db_kind_t kind;
  svn_boolean_t get_all;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));
  SVN_ERR(svn_wc__db_read_kind(&kind, wc_ctx->db, local_abspath, FALSE,
                               scratch_pool));

  if (kind == svn_wc__db_kind_dir)
      eb.anchor_abspath = local_abspath;
  else
    eb.anchor_abspath = svn_dirent_dirname(local_abspath, scratch_pool);

  eb.db = wc_ctx->db;
  eb.callbacks = callbacks;
  eb.callback_baton = callback_baton;
  eb.ignore_ancestry = ignore_ancestry;
  eb.show_copies_as_adds = show_copies_as_adds;
  eb.use_git_diff_format = use_git_diff_format;
  eb.empty_file = NULL;
  eb.pool = scratch_pool;

  if (changelist_filter && changelist_filter->nelts)
    SVN_ERR(svn_hash_from_cstring_keys(&eb.changelist_hash, changelist_filter,
                                       scratch_pool));

  if (show_copies_as_adds || use_git_diff_format)
    get_all = TRUE; /* We need unmodified descendants of copies */
  else
    get_all = FALSE;

  /* Walk status handles files and directories */
  SVN_ERR(svn_wc__internal_walk_status(wc_ctx->db, local_abspath, depth,
                                       get_all,
                                       TRUE /* no_ignore */,
                                       FALSE /* ignore_text_mods */,
                                       NULL /* ignore_patterns */,
                                       diff_status_callback, &eb,
                                       cancel_func, cancel_baton,
                                       scratch_pool));

  return SVN_NO_ERROR;
}
