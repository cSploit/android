/*
 * merge.c: merging
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



/*** Includes ***/

#include <apr_strings.h>
#include <apr_tables.h>
#include <apr_hash.h>
#include "svn_types.h"
#include "svn_hash.h"
#include "svn_wc.h"
#include "svn_delta.h"
#include "svn_diff.h"
#include "svn_mergeinfo.h"
#include "svn_client.h"
#include "svn_string.h"
#include "svn_error.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_io.h"
#include "svn_utf.h"
#include "svn_pools.h"
#include "svn_config.h"
#include "svn_props.h"
#include "svn_time.h"
#include "svn_sorts.h"
#include "svn_subst.h"
#include "svn_ra.h"
#include "client.h"
#include "mergeinfo.h"

#include "private/svn_wc_private.h"
#include "private/svn_mergeinfo_private.h"
#include "private/svn_fspath.h"

#include "svn_private_config.h"

/*-----------------------------------------------------------------------*/

/* MERGEINFO MERGE SOURCE NORMALIZATION
 *
 * Nearly any helper function herein that accepts two URL/revision
 * pairs expects one of two things to be true:
 *
 *    1.  that mergeinfo is not being recorded at all for this
 *        operation, or
 *
 *    2.  that the pairs represent two locations along a single line
 *        of version history such that there are no copies in the
 *        history of the object between the locations when treating
 *        the oldest of the two locations as non-inclusive.  In other
 *        words, if there is a copy at all between them, there is only
 *        one copy and its source was the oldest of the two locations.
 *
 * We use svn_ra_get_location_segments() to split a given range of
 * revisions across an object's history into several which obey these
 * rules.  For example, a merge between r19500 and r27567 of
 * Subversion's own /tags/1.4.5 directory gets split into sequential
 * merges of the following location pairs:
 *
 *    [/trunk:19549, /trunk:19523]
 *    (recorded in svn:mergeinfo as /trunk:19500-19523)
 *
 *    [/trunk:19523, /branches/1.4.x:25188]
 *    (recorded in svn:mergeinfo as /branches/1.4.x:19524-25188)
 *
 *    [/branches/1.4.x:25188, /tags/1.4.4@26345]
 *    (recorded in svn:mergeinfo as /tags/1.4.4:25189-26345)
 *
 *    [/tags/1.4.4@26345, /branches/1.4.5@26350]
 *    (recorded in svn:mergeinfo as /branches/1.4.5:26346-26350)
 *
 *    [/branches/1.4.5@26350, /tags/1.4.5@27567]
 *    (recorded in svn:mergeinfo as /tags/1.4.5:26351-27567)
 *
 * Our helper functions would then operate on one of these location
 * pairs at a time.
 */

/* WHICH SVN_CLIENT_MERGE* API DO I WANT?
 *
 * libsvn_client has three public merge APIs; they are all wrappers
 * around the do_merge engine.  Which one to use depends on the number
 * of URLs passed as arguments and whether or not specific merge
 * ranges (-c/-r) are specified.
 *
 *                 1 URL                        2 URLs
 * +----+--------------------------------+---------------------+
 * | -c |       mergeinfo-driven         |                     |
 * | or |        cherrypicking           |                     |
 * | -r |    (svn_client_merge_peg)      |                     |
 * |----+--------------------------------+                     |
 * |    |       mergeinfo-driven         |     unsupported     |
 * |    |  'cherry harvest', i.e. merge  |                     |
 * |    |  all revisions from URL that   |                     |
 * | no |  have not already been merged  |                     |
 * | -c |    (svn_client_merge_peg)      |                     |
 * | or +--------------------------------+---------------------+
 * | -r |      mergeinfo-driven          |   mergeinfo-writing |
 * |    |        whole-branch            |    diff-and-apply   |
 * |    |       heuristic merge          |  (svn_client_merge) |
 * |    | (svn_client_merge_reintegrate) |                     |
 * +----+--------------------------------+---------------------+
 *
 *
 */

/* THE CHILDREN_WITH_MERGEINFO ARRAY
 *
 * Many of the helper functions in this file pass around an
 * apr_array_header_t *CHILDREN_WITH_MERGEINFO.  This is a depth first
 * sorted array filled with svn_client__merge_path_t * describing the
 * merge target and any of its subtrees which have explicit mergeinfo
 * or otherwise need special attention during a merge.
 *
 * During mergeinfo unaware merges, CHILDREN_WITH_MERGEINFO is created by
 * do_mergeinfo_unaware_dir_merge and contains only one element describing
 * a contiguous range to be merged to the WC merge target.
 *
 * During mergeinfo aware merges CHILDREN_WITH_MERGEINFO is created
 * by get_mergeinfo_paths() and outside of that function and its helpers
 * should always meet the criteria dictated in get_mergeinfo_paths()'s doc
 * string.  The elements of CHILDREN_WITH_MERGINFO should never be NULL.
 *
 * For clarification on mergeinfo aware vs. mergeinfo unaware merges, see
 * the doc string for HONOR_MERGEINFO().
 */


/*-----------------------------------------------------------------------*/

/*** Repos-Diff Editor Callbacks ***/

typedef struct merge_source_t
{
  /* "left" side URL and revision (inclusive iff youngest) */
  const char *url1;
  svn_revnum_t rev1;

  /* "right" side URL and revision (inclusive iff youngest) */
  const char *url2;
  svn_revnum_t rev2;

} merge_source_t;

typedef struct merge_cmd_baton_t {
  svn_boolean_t force;
  svn_boolean_t dry_run;
  svn_boolean_t record_only;          /* Whether to merge only mergeinfo
                                         differences. */
  svn_boolean_t sources_ancestral;    /* Whether the left-side merge source is
                                         an ancestor of the right-side, or
                                         vice-versa (history-wise). */
  svn_boolean_t same_repos;           /* Whether the merge source repository
                                         is the same repository as the
                                         target.  Defaults to FALSE if DRY_RUN
                                         is TRUE.*/
  svn_boolean_t mergeinfo_capable;    /* Whether the merge source server
                                         is capable of Merge Tracking. */
  svn_boolean_t ignore_ancestry;      /* Are we ignoring ancestry (and by
                                         extension, mergeinfo)?  FALSE if
                                         SOURCES_ANCESTRAL is FALSE. */
  svn_boolean_t target_missing_child; /* Whether working copy target of the
                                         merge is missing any immediate
                                         children. */
  svn_boolean_t reintegrate_merge;    /* Whether this is a --reintegrate
                                         merge or not. */
  const char *added_path;             /* Set to the dir path whenever the
                                         dir is added as a child of a
                                         versioned dir (dry-run only) */
  const char *target_abspath;         /* Absolute working copy target of
                                         the merge. */
  const char *repos_root_url;         /* The repository root of the repository
                                         containing TARGET_ABSPATH */

  /* The left and right URLs and revs.  The value of this field changes to
     reflect the merge_source_t *currently* being merged by do_merge(). */
  merge_source_t merge_source;

  /* Rangelist containing single range which describes the gap, if any,
     in the natural history of the merge source currently being processed.
     See http://subversion.tigris.org/issues/show_bug.cgi?id=3432.
     Updated during each call to do_directory_merge().  May be NULL if there
     is no gap. */
  apr_array_header_t *implicit_src_gap;

  svn_client_ctx_t *ctx;              /* Client context for callbacks, etc. */

  /* Whether invocation of the merge_file_added() callback required
     delegation to the merge_file_changed() function for the file
     currently being merged.  This info is used to detect whether a
     file on the left side of a 3-way merge actually exists (important
     because it's created as an empty temp file on disk regardless).*/
  svn_boolean_t add_necessitated_merge;

  /* The list of paths for entries we've deleted, used only when in
     dry_run mode. */
  apr_hash_t *dry_run_deletions;

  /* The list of paths for entries we've added, used only when in
     dry_run mode. */
  apr_hash_t *dry_run_added;

  /* The list of any paths which remained in conflict after a
     resolution attempt was made.  We track this in-memory, rather
     than just using WC entry state, since the latter doesn't help us
     when in dry_run mode. */
  apr_hash_t *conflicted_paths;

  /* A list of absolute paths which had no explicit mergeinfo prior to the
     merge but got explicit mergeinfo added by the merge.  This is populated
     by merge_change_props() and is allocated in POOL so it is subject to the
     lifetime limitations of POOL.  Is NULL if no paths are found which
     meet the criteria or DRY_RUN is true. */
  apr_hash_t *paths_with_new_mergeinfo;

  /* A list of absolute paths which had explicit mergeinfo prior to the merge
     but had this mergeinfo deleted by the merge.  This is populated by
     merge_change_props() and is allocated in POOL so it is subject to the
     lifetime limitations of POOL.  Is NULL if no paths are found which
     meet the criteria or DRY_RUN is true. */
  apr_hash_t *paths_with_deleted_mergeinfo;

  /* The diff3_cmd in ctx->config, if any, else null.  We could just
     extract this as needed, but since more than one caller uses it,
     we just set it up when this baton is created. */
  const char *diff3_cmd;
  const apr_array_header_t *merge_options;

  /* RA sessions used throughout a merge operation.  Opened/re-parented
     as needed.

     NOTE: During the actual merge editor drive, RA_SESSION1 is used
     for the primary editing and RA_SESSION2 for fetching additional
     information -- as necessary -- from the repository.  So during
     this phase of the merge, you *must not* reparent RA_SESSION1; use
     (temporarily reparenting if you must) RA_SESSION2 instead.  */
  svn_ra_session_t *ra_session1;
  svn_ra_session_t *ra_session2;

  /* During the merge, *USE_SLEEP is set to TRUE if a sleep will be required
     afterwards to ensure timestamp integrity, or unchanged if not. */
  svn_boolean_t *use_sleep;

  /* Pool which has a lifetime limited to one iteration over a given
     merge source, i.e. it is cleared on every call to do_directory_merge()
     or do_file_merge() in do_merge(). */
  apr_pool_t *pool;
} merge_cmd_baton_t;


/* If the merge source server is is capable of merge tracking, the left-side
   merge source is an ancestor of the right-side (or vice-versa), the merge
   source repository is the same repository as the MERGE_B->TARGET_ABSPATH, and
   ancestry is being considered then return TRUE.  */
#define HONOR_MERGEINFO(merge_b) ((merge_b)->mergeinfo_capable      \
                                  && (merge_b)->sources_ancestral   \
                                  && (merge_b)->same_repos          \
                                  && (! (merge_b)->ignore_ancestry))


/* If HONOR_MERGEINFO is TRUE and the merge is not a dry run
   then return TRUE.  */
#define RECORD_MERGEINFO(merge_b) (HONOR_MERGEINFO(merge_b) \
                                   && !(merge_b)->dry_run)


/*-----------------------------------------------------------------------*/

/*** Utilities ***/

/* Return SVN_ERR_UNSUPPORTED_FEATURE if URL is not inside the repository
   of LOCAL_ABSPAT.  Use SCRATCH_POOL for temporary allocations. */
static svn_error_t *
check_repos_match(merge_cmd_baton_t *merge_b,
                  const char *local_abspath,
                  const char *url,
                  apr_pool_t *scratch_pool)
{
  if (!svn_uri__is_ancestor(merge_b->repos_root_url, url))
    return svn_error_createf(
        SVN_ERR_UNSUPPORTED_FEATURE, NULL,
         _("Url '%s' of '%s' is not in repository '%s'"),
         url, svn_dirent_local_style(local_abspath, scratch_pool),
         merge_b->repos_root_url);

  return SVN_NO_ERROR;
}
/* Return true iff we're in dry-run mode and WCPATH would have been
   deleted by now if we weren't in dry-run mode.
   Used to avoid spurious notifications (e.g. conflicts) from a merge
   attempt into an existing target which would have been deleted if we
   weren't in dry_run mode (issue #2584).  Assumes that WCPATH is
   still versioned (e.g. has an associated entry). */
static APR_INLINE svn_boolean_t
dry_run_deleted_p(const merge_cmd_baton_t *merge_b, const char *wcpath)
{
  return (merge_b->dry_run &&
          apr_hash_get(merge_b->dry_run_deletions, wcpath,
                       APR_HASH_KEY_STRING) != NULL);
}

/* Return true iff we're in dry-run mode and WCPATH would have been
   added by now if we weren't in dry-run mode.
   Used to avoid spurious notifications (e.g. conflicts) from a merge
   attempt into an existing target which would have been deleted if we
   weren't in dry_run mode (issue #2584).  Assumes that WCPATH is
   still versioned (e.g. has an associated entry). */
static APR_INLINE svn_boolean_t
dry_run_added_p(const merge_cmd_baton_t *merge_b, const char *wcpath)
{
  return (merge_b->dry_run &&
          apr_hash_get(merge_b->dry_run_added, wcpath,
                       APR_HASH_KEY_STRING) != NULL);
}

/* Return whether any WC path was put in conflict by the merge
   operation corresponding to MERGE_B. */
static APR_INLINE svn_boolean_t
is_path_conflicted_by_merge(merge_cmd_baton_t *merge_b)
{
  return (merge_b->conflicted_paths &&
          apr_hash_count(merge_b->conflicted_paths) > 0);
}

/* Return a state indicating whether the WC metadata matches the
 * node kind on disk of the local path LOCAL_ABSPATH.
 * Use MERGE_B to determine the dry-run details; particularly, if a dry run
 * noted that it deleted this path, assume matching node kinds (as if both
 * kinds were svn_node_none).
 *
 *   - Return svn_wc_notify_state_inapplicable if the node kind matches.
 *   - Return 'obstructed' if there is a node on disk where none or a
 *     different kind is expected, or if the disk node cannot be read.
 *   - Return 'missing' if there is no node on disk but one is expected.
 *     Also return 'missing' for server-excluded nodes (not here due to
 *     authz or other reasons determined by the server).
 *
 * Optionally return a bit more info for interested users.
 **/
static svn_error_t *
perform_obstruction_check(svn_wc_notify_state_t *obstruction_state,
                          svn_boolean_t *added,
                          svn_boolean_t *deleted,
                          svn_node_kind_t *kind,
                          const merge_cmd_baton_t *merge_b,
                          const char *local_abspath,
                          svn_node_kind_t expected_kind,
                          apr_pool_t *scratch_pool)
{
  svn_wc_context_t *wc_ctx = merge_b->ctx->wc_ctx;
  svn_node_kind_t wc_kind;
  svn_boolean_t check_root;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  *obstruction_state = svn_wc_notify_state_inapplicable;

  if (added)
    *added = FALSE;
  if (deleted)
    *deleted = FALSE;
  if (kind)
    *kind = svn_node_none;

  /* In a dry run, make as if nodes "deleted" by the dry run appear so. */
  if (merge_b->dry_run)
    {
      if (dry_run_deleted_p(merge_b, local_abspath))
        {
          *obstruction_state = svn_wc_notify_state_inapplicable;

          if (deleted)
            *deleted = TRUE;

          if (expected_kind != svn_node_unknown
              && expected_kind != svn_node_none)
            *obstruction_state = svn_wc_notify_state_obstructed;
          return SVN_NO_ERROR;
        }
      else if (dry_run_added_p(merge_b, local_abspath))
        {
          *obstruction_state = svn_wc_notify_state_inapplicable;

          if (added)
            *added = TRUE;
          if (kind)
            *kind = svn_node_dir; /* Currently only used for dirs */

          return SVN_NO_ERROR;
        }
     }

  if (kind == NULL)
    kind = &wc_kind;

  check_root = ! strcmp(local_abspath, merge_b->target_abspath);

  SVN_ERR(svn_wc__check_for_obstructions(obstruction_state,
                                         kind,
                                         added,
                                         deleted,
                                         NULL,
                                         wc_ctx, local_abspath,
                                         check_root,
                                         scratch_pool));

  if (*obstruction_state == svn_wc_notify_state_inapplicable
      && expected_kind != svn_node_unknown
      && *kind != expected_kind)
    {
      *obstruction_state = svn_wc_notify_state_obstructed;
    }

  return SVN_NO_ERROR;
}

/* Create *LEFT and *RIGHT conflict versions for conflict victim
 * at VICTIM_ABSPATH, with kind NODE_KIND, using information obtained
 * from MERGE_B.
 * Allocate returned conflict versions in MERGE_B->POOL. */
static svn_error_t *
make_conflict_versions(const svn_wc_conflict_version_t **left,
                       const svn_wc_conflict_version_t **right,
                       const char *victim_abspath,
                       svn_node_kind_t node_kind,
                       merge_cmd_baton_t *merge_b)
{
  const char *src_repos_url;  /* root URL of source repository */
  const char *left_url;
  const char *right_url;

  SVN_ERR(svn_ra_get_repos_root2(merge_b->ra_session1, &src_repos_url,
                                 merge_b->pool));

  /* Construct the source URLs of the victim. */
  {
    const char *child = svn_dirent_is_child(merge_b->target_abspath,
                                            victim_abspath,
                                            merge_b->pool);
    if (child != NULL)
      {
        left_url = svn_path_url_add_component2(merge_b->merge_source.url1,
                                               child, merge_b->pool);
        right_url = svn_path_url_add_component2(merge_b->merge_source.url2,
                                                child, merge_b->pool);
      }
    else
      {
        left_url = merge_b->merge_source.url1;
        right_url = merge_b->merge_source.url2;
      }
  }

  *left = svn_wc_conflict_version_create(
            src_repos_url,
            svn_uri__is_child(src_repos_url, left_url, merge_b->pool),
            merge_b->merge_source.rev1, node_kind, merge_b->pool);

  *right = svn_wc_conflict_version_create(
             src_repos_url,
             svn_uri__is_child(src_repos_url, right_url, merge_b->pool),
             merge_b->merge_source.rev2, node_kind, merge_b->pool);

  return SVN_NO_ERROR;
}

/* Set *CONFLICT to a new tree-conflict description allocated in MERGE_B->pool,
 * populated with information from MERGE_B and the other parameters.
 * See tree_conflict() for the other parameters.
 */
static svn_error_t*
make_tree_conflict(svn_wc_conflict_description2_t **conflict,
                   merge_cmd_baton_t *merge_b,
                   const char *victim_abspath,
                   svn_node_kind_t node_kind,
                   svn_wc_conflict_action_t action,
                   svn_wc_conflict_reason_t reason)
{
  const svn_wc_conflict_version_t *left;
  const svn_wc_conflict_version_t *right;

  SVN_ERR(make_conflict_versions(&left, &right, victim_abspath, node_kind,
                                 merge_b));

  *conflict = svn_wc_conflict_description_create_tree2(
                    victim_abspath, node_kind, svn_wc_operation_merge,
                    left, right, merge_b->pool);

  (*conflict)->action = action;
  (*conflict)->reason = reason;

  return SVN_NO_ERROR;
}

/* Record a tree conflict in the WC, unless this is a dry run or a record-
 * only merge, or if a tree conflict is already flagged for the VICTIM_PATH.
 * (The latter can happen if a merge-tracking-aware merge is doing multiple
 * editor drives because of a gap in the range of eligible revisions.)
 *
 * The tree conflict, with its victim specified by VICTIM_PATH, is
 * assumed to have happened during a merge using merge baton MERGE_B.
 *
 * NODE_KIND must be the node kind of "old" and "theirs" and "mine";
 * this function cannot cope with node kind clashes.
 * ACTION and REASON correspond to the fields
 * of the same names in svn_wc_tree_conflict_description_t.
 */
static svn_error_t*
tree_conflict(merge_cmd_baton_t *merge_b,
              const char *victim_abspath,
              svn_node_kind_t node_kind,
              svn_wc_conflict_action_t action,
              svn_wc_conflict_reason_t reason)
{
  const svn_wc_conflict_description2_t *existing_conflict;
  svn_wc_conflict_description2_t *conflict;

  if (merge_b->record_only || merge_b->dry_run)
    return SVN_NO_ERROR;

  SVN_ERR(svn_wc__get_tree_conflict(&existing_conflict, merge_b->ctx->wc_ctx,
                                    victim_abspath, merge_b->pool,
                                    merge_b->pool));
  if (existing_conflict == NULL)
    {
      /* There is no existing tree conflict so it is safe to add one. */
      SVN_ERR(make_tree_conflict(&conflict, merge_b, victim_abspath,
                                 node_kind, action, reason));
      SVN_ERR(svn_wc__add_tree_conflict(merge_b->ctx->wc_ctx, conflict,
                                        merge_b->pool));

      if (merge_b->conflicted_paths == NULL)
        merge_b->conflicted_paths = apr_hash_make(merge_b->pool);
      victim_abspath = apr_pstrdup(merge_b->pool, victim_abspath);

      apr_hash_set(merge_b->conflicted_paths, victim_abspath,
                   APR_HASH_KEY_STRING, victim_abspath);
    }

  return SVN_NO_ERROR;
}

/* Similar to tree_conflict(), but if this is an "add" action and there
   is an existing tree conflict on the victim with a "delete" action, then
   combine the two conflicts into a single conflict with a "replace" action. */
static svn_error_t*
tree_conflict_on_add(merge_cmd_baton_t *merge_b,
                     const char *victim_abspath,
                     svn_node_kind_t node_kind,
                     svn_wc_conflict_action_t action,
                     svn_wc_conflict_reason_t reason)
{
  const svn_wc_conflict_description2_t *existing_conflict;
  svn_wc_conflict_description2_t *conflict;

  if (merge_b->record_only || merge_b->dry_run)
    return SVN_NO_ERROR;

  /* Construct the new conflict first  compare the new conflict with
     a possibly existing one. */
  SVN_ERR(make_tree_conflict(&conflict, merge_b, victim_abspath,
                             node_kind, action, reason));

  SVN_ERR(svn_wc__get_tree_conflict(&existing_conflict, merge_b->ctx->wc_ctx,
                                    victim_abspath, merge_b->pool,
                                    merge_b->pool));

  if (existing_conflict == NULL)
    {
      /* There is no existing tree conflict so it is safe to add one. */
      SVN_ERR(svn_wc__add_tree_conflict(merge_b->ctx->wc_ctx, conflict,
                                        merge_b->pool));

      if (merge_b->conflicted_paths == NULL)
        merge_b->conflicted_paths = apr_hash_make(merge_b->pool);
      victim_abspath = apr_pstrdup(merge_b->pool, victim_abspath);

      apr_hash_set(merge_b->conflicted_paths, victim_abspath,
                   APR_HASH_KEY_STRING, victim_abspath);
    }
  else if (existing_conflict->action == svn_wc_conflict_action_delete &&
           conflict->action == svn_wc_conflict_action_add)
    {
      /* There is already a tree conflict raised by a previous incoming
       * change that attempted to delete the item (whether in this same
       * merge operation or not). Change the existing conflict to note
       * that the incoming change is replacement. */

      /* Remove the existing tree-conflict so we can add a new one.*/
      SVN_ERR(svn_wc__del_tree_conflict(merge_b->ctx->wc_ctx,
                                        victim_abspath, merge_b->pool));

      /* Preserve the reason which caused the first conflict,
       * re-label the incoming change as 'replacement', and update
       * version info for the left version of the conflict. */
      conflict->reason = existing_conflict->reason;
      conflict->action = svn_wc_conflict_action_replace;
      conflict->src_left_version = svn_wc_conflict_version_dup(
                                     existing_conflict->src_left_version,
                                     merge_b->pool);

      SVN_ERR(svn_wc__add_tree_conflict(merge_b->ctx->wc_ctx, conflict,
                                        merge_b->pool));

      if (merge_b->conflicted_paths == NULL)
        merge_b->conflicted_paths = apr_hash_make(merge_b->pool);
      victim_abspath = apr_pstrdup(merge_b->pool, victim_abspath);

      apr_hash_set(merge_b->conflicted_paths, victim_abspath,
                   APR_HASH_KEY_STRING, victim_abspath);
    }

  /* In any other cases, we don't touch the existing conflict. */
  return SVN_NO_ERROR;
}


/* Helper for filter_self_referential_mergeinfo()

   *MERGEINFO is a non-empty, non-null collection of mergeinfo.

   Remove all mergeinfo from *MERGEINFO that describes revision ranges
   greater than REVISION.  Put a copy of any removed mergeinfo, allocated
   in POOL, into *YOUNGER_MERGEINFO.

   If no mergeinfo is removed from *MERGEINFO then *YOUNGER_MERGEINFO is set
   to NULL.  If all mergeinfo is removed from *MERGEINFO then *MERGEINFO is
   set to NULL.
   */
static svn_error_t*
split_mergeinfo_on_revision(svn_mergeinfo_t *younger_mergeinfo,
                            svn_mergeinfo_t *mergeinfo,
                            svn_revnum_t revision,
                            apr_pool_t *pool)
{
  apr_hash_index_t *hi;
  apr_pool_t *iterpool = svn_pool_create(pool);

  *younger_mergeinfo = NULL;
  for (hi = apr_hash_first(pool, *mergeinfo); hi; hi = apr_hash_next(hi))
    {
      int i;
      const char *merge_source_path = svn__apr_hash_index_key(hi);
      apr_array_header_t *rangelist = svn__apr_hash_index_val(hi);

      svn_pool_clear(iterpool);

      for (i = 0; i < rangelist->nelts; i++)
        {
          svn_merge_range_t *range =
            APR_ARRAY_IDX(rangelist, i, svn_merge_range_t *);
          if (range->end <= revision)
            {
              /* This entirely of this range is as old or older than
                 REVISION, so leave it in *MERGEINFO. */
              continue;
            }
          else
            {
              /* Since the rangelists in svn_mergeinfo_t's are sorted in
                 increasing order we know that part or all of *this* range
                 and *all* of the remaining ranges in *RANGELIST are younger
                 than REVISION.  Remove the younger rangelists from
                 *MERGEINFO and put them in *YOUNGER_MERGEINFO. */
              int j;
              apr_array_header_t *younger_rangelist =
                apr_array_make(pool, 1, sizeof(svn_merge_range_t *));

              for (j = i; j < rangelist->nelts; j++)
                {
                  svn_merge_range_t *younger_range = svn_merge_range_dup(
                    APR_ARRAY_IDX(rangelist, j, svn_merge_range_t *), pool);

                  /* REVISION might intersect with the first range where
                     range->end > REVISION.  If that is the case then split
                     the current range into two, putting the younger half
                     into *YOUNGER_MERGEINFO and leaving the older half in
                     *MERGEINFO. */
                  if (j == i && range->start + 1 <= revision)
                    younger_range->start = range->end = revision;

                  APR_ARRAY_PUSH(younger_rangelist, svn_merge_range_t *) =
                    younger_range;
                }

              /* So far we've only been manipulating rangelists, now we
                 actually create *YOUNGER_MERGEINFO and then remove the older
                 ranges from *MERGEINFO */
              if (!(*younger_mergeinfo))
                *younger_mergeinfo = apr_hash_make(pool);
              apr_hash_set(*younger_mergeinfo,
                           (const char *)merge_source_path,
                           APR_HASH_KEY_STRING, younger_rangelist);
              SVN_ERR(svn_mergeinfo_remove2(mergeinfo, *younger_mergeinfo,
                                            *mergeinfo, TRUE, pool, iterpool));
              break; /* ...out of for (i = 0; i < rangelist->nelts; i++) */
            }
        }
    }

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}


/* Make a copy of PROPCHANGES (array of svn_prop_t) into *TRIMMED_PROPCHANGES,
   omitting any svn:mergeinfo changes.  */
static svn_error_t *
omit_mergeinfo_changes(apr_array_header_t **trimmed_propchanges,
                       const apr_array_header_t *propchanges,
                       apr_pool_t *result_pool)
{
  int i;

  *trimmed_propchanges = apr_array_make(result_pool,
                                        propchanges->nelts,
                                        sizeof(svn_prop_t));

  for (i = 0; i < propchanges->nelts; ++i)
    {
      const svn_prop_t *change = &APR_ARRAY_IDX(propchanges, i, svn_prop_t);

      /* If this property is not svn:mergeinfo, then copy it.  */
      if (strcmp(change->name, SVN_PROP_MERGEINFO) != 0)
        APR_ARRAY_PUSH(*trimmed_propchanges, svn_prop_t) = *change;
    }

  return SVN_NO_ERROR;
}


/* Helper for merge_props_changed().

   *PROPS is an array of svn_prop_t structures representing regular properties
   to be added to the working copy LOCAL_ABSPATH.

   HONOR_MERGEINFO determines whether mergeinfo will be honored by this
   function (when applicable).

   If mergeinfo is not being honored, SAME_REPOS is true, and
   REINTEGRATE_MERGE is FALSE do nothing.  Otherwise, if
   SAME_REPOS is false, then filter out all mergeinfo
   property additions (Issue #3383) from *PROPS.  If SAME_REPOS is
   true then filter out mergeinfo property additions to LOCAL_ABSPATH when
   those additions refer to the same line of history as LOCAL_ABSPATH as
   described below.

   If mergeinfo is being honored and SAME_REPOS is true
   then examine the added mergeinfo, looking at each range (or single rev)
   of each source path.  If a source_path/range refers to the same line of
   history as LOCAL_ABSPATH (pegged at its base revision), then filter out
   that range.  If the entire rangelist for a given path is filtered then
   filter out the path as well.

   Use RA_SESSION for any communication to the repository, and CTX for any
   further client operations.

   If any filtering occurs, set outgoing *PROPS to a shallow copy (allocated
   in POOL) of incoming *PROPS minus the filtered mergeinfo. */
static svn_error_t*
filter_self_referential_mergeinfo(apr_array_header_t **props,
                                  const char *local_abspath,
                                  svn_boolean_t honor_mergeinfo,
                                  svn_boolean_t same_repos,
                                  svn_boolean_t reintegrate_merge,
                                  svn_ra_session_t *ra_session,
                                  svn_client_ctx_t *ctx,
                                  apr_pool_t *pool)
{
  apr_array_header_t *adjusted_props;
  int i;
  apr_pool_t *iterpool;
  svn_boolean_t is_added;
  svn_revnum_t base_revision;

  /* Issue #3383: We don't want mergeinfo from a foreign repos.

     If this is a merge from a foreign repository we must strip all
     incoming mergeinfo (including mergeinfo deletions).  Otherwise if
     this property isn't mergeinfo or is NULL valued (i.e. prop removal)
     or empty mergeinfo it does not require any special handling.  There
     is nothing to filter out of empty mergeinfo and the concept of
     filtering doesn't apply if we are trying to remove mergeinfo
     entirely.  */
  if (! same_repos)
    return svn_error_trace(omit_mergeinfo_changes(props, *props, pool));

  /* If we aren't honoring mergeinfo and this is a merge from the
     same repository, then get outta here.  If this is a reintegrate
     merge or a merge from a foreign repository we still need to
     filter regardless of whether we are honoring mergeinfo or not. */
  if (! honor_mergeinfo
      && ! reintegrate_merge)
    return SVN_NO_ERROR;

  /* If this is a merge from the same repository and PATH itself has been
     added there is no need to filter. */
  SVN_ERR(svn_wc__node_is_added(&is_added, ctx->wc_ctx, local_abspath, pool));
  if (is_added)
    return SVN_NO_ERROR;

  SVN_ERR(svn_wc__node_get_base_rev(&base_revision, ctx->wc_ctx,
                                    local_abspath, pool));

  adjusted_props = apr_array_make(pool, (*props)->nelts, sizeof(svn_prop_t));
  iterpool = svn_pool_create(pool);
  for (i = 0; i < (*props)->nelts; ++i)
    {
      svn_prop_t *prop = &APR_ARRAY_IDX((*props), i, svn_prop_t);

      svn_mergeinfo_t mergeinfo, younger_mergeinfo;
      svn_mergeinfo_t filtered_mergeinfo = NULL;
      svn_mergeinfo_t filtered_younger_mergeinfo = NULL;
      const char *target_url;
      const char *old_url = NULL;
      svn_error_t *err;

      if ((strcmp(prop->name, SVN_PROP_MERGEINFO) != 0)
          || (! prop->value)       /* Removal of mergeinfo */
          || (! prop->value->len)) /* Empty mergeinfo */
        {
          APR_ARRAY_PUSH(adjusted_props, svn_prop_t) = *prop;
          continue;
        }

      svn_pool_clear(iterpool);

      /* Non-empty mergeinfo; filter self-referential mergeinfo out. */
      /* Temporarily reparent our RA session to the merge
         target's URL. */
      SVN_ERR(svn_client_url_from_path2(&target_url, local_abspath,
                                        ctx, iterpool, iterpool));
      SVN_ERR(svn_client__ensure_ra_session_url(&old_url,
                                                ra_session,
                                                target_url, iterpool));

      /* Parse the incoming mergeinfo to allow easier manipulation. */
      err = svn_mergeinfo_parse(&mergeinfo, prop->value->data, iterpool);

      if (err)
        {
          /* Issue #3896: If we can't parse it, we certainly can't
             filter it. */
          if (err->apr_err == SVN_ERR_MERGEINFO_PARSE_ERROR)
            {
              svn_error_clear(err);
              APR_ARRAY_PUSH(adjusted_props, svn_prop_t) = *prop;

              /* If we reparented RA_SESSION above, put it back
                 to the original URL. */
              if (old_url)
                SVN_ERR(svn_ra_reparent(ra_session, old_url, iterpool));

              continue;
            }
          else
            {
              return svn_error_trace(err);
            }
        }

      /* The working copy target PATH is at BASE_REVISION.  Divide the
         incoming mergeinfo into two groups.  One where all revision ranges
         are as old or older than BASE_REVISION and one where all revision
         ranges are younger.

         Note: You may be wondering why we do this.

         For the incoming mergeinfo "older" than target's base revision we
         can filter out self-referential mergeinfo efficiently using
         svn_client__get_history_as_mergeinfo().  We simply look at PATH's
         natural history as mergeinfo and remove that from any incoming
         mergeinfo.

         For mergeinfo "younger" than the base revision we can't use
         svn_ra_get_location_segments() to look into PATH's future
         history.  Instead we must use svn_client__repos_locations() and
         look at each incoming source/range individually and see if PATH
         at its base revision and PATH at the start of the incoming range
         exist on the same line of history.  If they do then we can filter
         out the incoming range.  But since we have to do this for each
         range there is a substantial performance penalty to pay if the
         incoming ranges are not contiguous, i.e. we call
         svn_client__repos_locations for each discrete range and incur
         the cost of a roundtrip communication with the repository. */
      SVN_ERR(split_mergeinfo_on_revision(&younger_mergeinfo,
                                          &mergeinfo,
                                          base_revision,
                                          iterpool));

      /* Filter self-referential mergeinfo from younger_mergeinfo. */
      if (younger_mergeinfo)
        {
          apr_hash_index_t *hi;
          const char *merge_source_root_url;

          SVN_ERR(svn_ra_get_repos_root2(ra_session,
                                         &merge_source_root_url, iterpool));

          for (hi = apr_hash_first(iterpool, younger_mergeinfo);
               hi; hi = apr_hash_next(hi))
            {
              int j;
              const char *source_path = svn__apr_hash_index_key(hi);
              apr_array_header_t *rangelist = svn__apr_hash_index_val(hi);
              const char *merge_source_url;
              apr_array_header_t *adjusted_rangelist =
                apr_array_make(iterpool, 0, sizeof(svn_merge_range_t *));

              merge_source_url =
                    svn_path_url_add_component2(merge_source_root_url,
                                                source_path + 1, iterpool);

              for (j = 0; j < rangelist->nelts; j++)
                {
                  svn_error_t *err2;
                  svn_opt_revision_t *start_revision;
                  const char *start_url;
                  svn_opt_revision_t peg_rev, rev1_opt, rev2_opt;
                  svn_merge_range_t *range =
                    APR_ARRAY_IDX(rangelist, j, svn_merge_range_t *);

                  peg_rev.kind = svn_opt_revision_number;
                  peg_rev.value.number = base_revision;
                  rev1_opt.kind = svn_opt_revision_number;
                  /* SVN_PROP_MERGEINFO only stores forward merges, so
                     the start range of svn_merge_range_t RANGE is not
                     inclusive. */
                  rev1_opt.value.number = range->start + 1;

                  /* Because the merge source normalization code
                     ensures mergeinfo refers to real locations on
                     the same line of history, there's no need to
                     look at the whole range, just the start. */
                   rev2_opt.kind = svn_opt_revision_unspecified;

                  /* Check if PATH@BASE_REVISION exists at
                     RANGE->START on the same line of history. */
                  err2 = svn_client__repos_locations(&start_url,
                                                     &start_revision,
                                                     NULL,
                                                     NULL,
                                                     ra_session,
                                                     target_url,
                                                     &peg_rev,
                                                     &rev1_opt,
                                                     &rev2_opt,
                                                     ctx,
                                                     iterpool);
                  if (err2)
                    {
                      if (err2->apr_err == SVN_ERR_CLIENT_UNRELATED_RESOURCES
                          || err2->apr_err == SVN_ERR_FS_NOT_FOUND
                          || err2->apr_err == SVN_ERR_FS_NO_SUCH_REVISION)
                        {
                          /* PATH@BASE_REVISION didn't exist at
                             RANGE->START + 1 or is unrelated to the
                             resource PATH@RANGE->START.  Some of the
                             requested revisions may not even exist in
                             the repository; a real possibility since
                             mergeinfo is hand editable.  In all of these
                             cases clear and ignore the error and don't
                             do any filtering.

                             Note: In this last case it is possible that
                             we will allow self-referential mergeinfo to
                             be applied, but fixing it here is potentially
                             very costly in terms of finding what part of
                             a range is actually valid.  Simply allowing
                             the merge to proceed without filtering the
                             offending range seems the least worst
                             option. */
                          svn_error_clear(err2);
                          err2 = NULL;
                          APR_ARRAY_PUSH(adjusted_rangelist,
                                         svn_merge_range_t *) = range;
                        }
                      else
                        {
                          return svn_error_trace(err2);
                        }
                     }
                  else
                    {
                      /* PATH@BASE_REVISION exists on the same
                         line of history at RANGE->START and RANGE->END.
                         Now check that PATH@BASE_REVISION's path
                         names at RANGE->START and RANGE->END are the same.
                         If the names are not the same then the mergeinfo
                         describing PATH@RANGE->START through
                         PATH@RANGE->END actually belong to some other
                         line of history and we want to record this
                         mergeinfo, not filter it. */
                      if (strcmp(start_url, merge_source_url) != 0)
                        {
                          APR_ARRAY_PUSH(adjusted_rangelist,
                                         svn_merge_range_t *) = range;
                        }
                    }
                    /* else no need to add, this mergeinfo is
                       all on the same line of history. */
                } /* for (j = 0; j < rangelist->nelts; j++) */

              /* Add any rangelists for source_path that are not
                 self-referential. */
              if (adjusted_rangelist->nelts)
                {
                  if (!filtered_younger_mergeinfo)
                    filtered_younger_mergeinfo = apr_hash_make(iterpool);
                  apr_hash_set(filtered_younger_mergeinfo, source_path,
                               APR_HASH_KEY_STRING, adjusted_rangelist);
                }

            } /* Iteration over each merge source in younger_mergeinfo. */
        } /* if (younger_mergeinfo) */

      /* Filter self-referential mergeinfo from "older" mergeinfo. */
      if (mergeinfo)
        {
          svn_mergeinfo_t implicit_mergeinfo;
          svn_opt_revision_t peg_rev;

          peg_rev.kind = svn_opt_revision_number;
          peg_rev.value.number = base_revision;
          SVN_ERR(svn_client__get_history_as_mergeinfo(
            &implicit_mergeinfo, NULL,
            local_abspath, &peg_rev,
            base_revision,
            SVN_INVALID_REVNUM,
            ra_session,
            ctx,
            iterpool));

          /* Remove PATH's implicit mergeinfo from the incoming mergeinfo. */
          SVN_ERR(svn_mergeinfo_remove2(&filtered_mergeinfo,
                                        implicit_mergeinfo,
                                        mergeinfo, TRUE, iterpool, iterpool));
        }

      /* If we reparented RA_SESSION above, put it back
         to the original URL. */
      if (old_url)
        SVN_ERR(svn_ra_reparent(ra_session, old_url, iterpool));

      /* Combine whatever older and younger filtered mergeinfo exists
         into filtered_mergeinfo. */
      if (filtered_mergeinfo && filtered_younger_mergeinfo)
        SVN_ERR(svn_mergeinfo_merge(filtered_mergeinfo,
                                    filtered_younger_mergeinfo, iterpool));
      else if (filtered_younger_mergeinfo)
        filtered_mergeinfo = filtered_younger_mergeinfo;

      /* If there is any incoming mergeinfo remaining after filtering
         then put it in adjusted_props. */
      if (filtered_mergeinfo && apr_hash_count(filtered_mergeinfo))
        {
          /* Convert filtered_mergeinfo to a svn_prop_t and put it
             back in the array. */
          svn_string_t *filtered_mergeinfo_str;
          svn_prop_t *adjusted_prop = apr_pcalloc(pool,
                                                  sizeof(*adjusted_prop));
          SVN_ERR(svn_mergeinfo_to_string(&filtered_mergeinfo_str,
                                          filtered_mergeinfo,
                                          pool));
          adjusted_prop->name = SVN_PROP_MERGEINFO;
          adjusted_prop->value = filtered_mergeinfo_str;
          APR_ARRAY_PUSH(adjusted_props, svn_prop_t) = *adjusted_prop;
        }
    }
  svn_pool_destroy(iterpool);

  *props = adjusted_props;
  return SVN_NO_ERROR;
}

/* Used for both file and directory property merges. */
static svn_error_t *
merge_props_changed(svn_wc_notify_state_t *state,
                    svn_boolean_t *tree_conflicted,
                    const char *local_abspath,
                    const apr_array_header_t *propchanges,
                    apr_hash_t *original_props,
                    void *baton,
                    apr_pool_t *scratch_pool)
{
  apr_array_header_t *props;
  merge_cmd_baton_t *merge_b = baton;
  svn_client_ctx_t *ctx = merge_b->ctx;
  svn_error_t *err;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  SVN_ERR(svn_categorize_props(propchanges, NULL, NULL, &props,
                               scratch_pool));

  /* If we are only applying mergeinfo changes then we need to do
     additional filtering of PROPS so it contains only mergeinfo changes. */
  if (merge_b->record_only && props->nelts)
    {
      apr_array_header_t *mergeinfo_props =
        apr_array_make(scratch_pool, 1, sizeof(svn_prop_t));
      int i;

      for (i = 0; i < props->nelts; i++)
        {
          svn_prop_t *prop = &APR_ARRAY_IDX(props, i, svn_prop_t);

          if (strcmp(prop->name, SVN_PROP_MERGEINFO) == 0)
            {
              APR_ARRAY_PUSH(mergeinfo_props, svn_prop_t) = *prop;
              break;
            }
        }
      props = mergeinfo_props;
    }

  /* We only want to merge "regular" version properties:  by
     definition, 'svn merge' shouldn't touch any data within .svn/  */
  if (props->nelts)
    {
      /* If this is a forward merge then don't add new mergeinfo to
         PATH that is already part of PATH's own history, see
         http://svn.haxx.se/dev/archive-2008-09/0006.shtml.  If the
         merge sources are not ancestral then there is no concept of a
         'forward' or 'reverse' merge and we filter unconditionally. */
      if (merge_b->merge_source.rev1 < merge_b->merge_source.rev2
          || !merge_b->sources_ancestral)
        SVN_ERR(filter_self_referential_mergeinfo(&props,
                                                  local_abspath,
                                                  HONOR_MERGEINFO(merge_b),
                                                  merge_b->same_repos,
                                                  merge_b->reintegrate_merge,
                                                  merge_b->ra_session2,
                                                  merge_b->ctx,
                                                  scratch_pool));

      err = svn_wc_merge_props3(state, ctx->wc_ctx, local_abspath, NULL, NULL,
                                original_props, props, merge_b->dry_run,
                                ctx->conflict_func2, ctx->conflict_baton2,
                                ctx->cancel_func, ctx->cancel_baton,
                                scratch_pool);

      /* If this is not a dry run then make a record in BATON if we find a
         PATH where mergeinfo is added where none existed previously or PATH
         is having its existing mergeinfo deleted. */
      if (!merge_b->dry_run)
        {
          int i;

          for (i = 0; i < props->nelts; ++i)
            {
              svn_prop_t *prop = &APR_ARRAY_IDX(props, i, svn_prop_t);

              if (strcmp(prop->name, SVN_PROP_MERGEINFO) == 0)
                {
                  /* Does LOCAL_ABSPATH have any pristine mergeinfo? */
                  svn_boolean_t has_pristine_mergeinfo = FALSE;
                  apr_hash_t *pristine_props;

                  SVN_ERR(svn_wc_get_pristine_props(&pristine_props,
                                                    ctx->wc_ctx,
                                                    local_abspath,
                                                    scratch_pool,
                                                    scratch_pool));

                  if (pristine_props
                      && apr_hash_get(pristine_props, SVN_PROP_MERGEINFO,
                                      APR_HASH_KEY_STRING))
                    has_pristine_mergeinfo = TRUE;

                  if (!has_pristine_mergeinfo && prop->value)
                    {
                      /* If BATON->PATHS_WITH_NEW_MERGEINFO needs to be
                         allocated do so in BATON->POOL so it has a
                         sufficient lifetime. */
                      if (!merge_b->paths_with_new_mergeinfo)
                        merge_b->paths_with_new_mergeinfo =
                          apr_hash_make(merge_b->pool);

                      apr_hash_set(merge_b->paths_with_new_mergeinfo,
                                   apr_pstrdup(merge_b->pool, local_abspath),
                                   APR_HASH_KEY_STRING, local_abspath);
                    }
                  else if (has_pristine_mergeinfo && !prop->value)
                    {
                      /* If BATON->PATHS_WITH_DELETED_MERGEINFO needs to be
                         allocated do so in BATON->POOL so it has a
                         sufficient lifetime. */
                      if (!merge_b->paths_with_deleted_mergeinfo)
                        merge_b->paths_with_deleted_mergeinfo =
                          apr_hash_make(merge_b->pool);

                      apr_hash_set(merge_b->paths_with_deleted_mergeinfo,
                                   apr_pstrdup(merge_b->pool, local_abspath),
                                   APR_HASH_KEY_STRING, local_abspath);
                    }
                }
            }
        }

      if (err && (err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND
                  || err->apr_err == SVN_ERR_WC_PATH_UNEXPECTED_STATUS))
        {
          /* If the entry doesn't exist in the wc, this is a tree-conflict. */
          if (state)
            *state = svn_wc_notify_state_missing;
          if (tree_conflicted)
            *tree_conflicted = TRUE;
          svn_error_clear(err);
          return SVN_NO_ERROR;
        }
      else if (err)
        return svn_error_trace(err);
    }

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
merge_dir_props_changed(svn_wc_notify_state_t *state,
                        svn_boolean_t *tree_conflicted,
                        const char *local_abspath,
                        svn_boolean_t dir_was_added,
                        const apr_array_header_t *propchanges,
                        apr_hash_t *original_props,
                        void *diff_baton,
                        apr_pool_t *scratch_pool)
{
  merge_cmd_baton_t *merge_b = diff_baton;
  svn_wc_notify_state_t obstr_state;

  SVN_ERR(perform_obstruction_check(&obstr_state, NULL, NULL,
                                    NULL,
                                    merge_b, local_abspath, svn_node_dir,
                                    scratch_pool));

  if (obstr_state != svn_wc_notify_state_inapplicable)
    {
      if (state)
        *state = obstr_state;
      return SVN_NO_ERROR;
    }

  if (dir_was_added
      && merge_b->dry_run
      && dry_run_added_p(merge_b, local_abspath))
    {
      return SVN_NO_ERROR; /* We can't do a real prop merge for added dirs */
    }

  return svn_error_trace(merge_props_changed(state,
                                             tree_conflicted,
                                             local_abspath,
                                             propchanges,
                                             original_props,
                                             diff_baton,
                                             scratch_pool));
}

/* Contains any state collected while resolving conflicts. */
typedef struct conflict_resolver_baton_t
{
  /* The wrapped callback and baton. */
  svn_wc_conflict_resolver_func2_t wrapped_func;
  void *wrapped_baton;

  /* The list of any paths which remained in conflict after a
     resolution attempt was made. */
  apr_hash_t **conflicted_paths;

  /* Pool used in notification_receiver() to avoid the iteration
     sub-pool which is passed in, then subsequently destroyed. */
  apr_pool_t *pool;
} conflict_resolver_baton_t;

/* An implementation of the svn_wc_conflict_resolver_func_t interface.
   We keep a record of paths which remain in conflict after any
   resolution attempt from BATON->wrapped_func. */
static svn_error_t *
conflict_resolver(svn_wc_conflict_result_t **result,
                  const svn_wc_conflict_description2_t *description,
                  void *baton,
                  apr_pool_t *result_pool,
                  apr_pool_t *scratch_pool)
{
  svn_error_t *err;
  conflict_resolver_baton_t *conflict_b = baton;

  if (conflict_b->wrapped_func)
    err = (*conflict_b->wrapped_func)(result, description,
                                      conflict_b->wrapped_baton,
                                      result_pool,
                                      scratch_pool);
  else
    {
      /* If we have no wrapped callback to invoke, then we still need
         to behave like a proper conflict-callback ourselves.  */
      *result = svn_wc_create_conflict_result(svn_wc_conflict_choose_postpone,
                                              NULL, result_pool);
      err = SVN_NO_ERROR;
    }

  /* Keep a record of paths still in conflict after the resolution attempt. */
  if ((! conflict_b->wrapped_func)
      || (*result && ((*result)->choice == svn_wc_conflict_choose_postpone)))
    {
      const char *conflicted_path = apr_pstrdup(conflict_b->pool,
                                                description->local_abspath);

      if (*conflict_b->conflicted_paths == NULL)
        *conflict_b->conflicted_paths = apr_hash_make(conflict_b->pool);

      apr_hash_set(*conflict_b->conflicted_paths, conflicted_path,
                   APR_HASH_KEY_STRING, conflicted_path);
    }

  return svn_error_trace(err);
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
merge_file_opened(svn_boolean_t *tree_conflicted,
                  svn_boolean_t *skip,
                  const char *path,
                  svn_revnum_t rev,
                  void *diff_baton,
                  apr_pool_t *scratch_pool)
{
  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
merge_file_changed(svn_wc_notify_state_t *content_state,
                   svn_wc_notify_state_t *prop_state,
                   svn_boolean_t *tree_conflicted,
                   const char *mine_abspath,
                   const char *older_abspath,
                   const char *yours_abspath,
                   svn_revnum_t older_rev,
                   svn_revnum_t yours_rev,
                   const char *mimetype1,
                   const char *mimetype2,
                   const apr_array_header_t *prop_changes,
                   apr_hash_t *original_props,
                   void *baton,
                   apr_pool_t *scratch_pool)
{
  merge_cmd_baton_t *merge_b = baton;
  enum svn_wc_merge_outcome_t merge_outcome;
  svn_node_kind_t wc_kind;
  svn_boolean_t is_deleted;

  SVN_ERR_ASSERT(mine_abspath && svn_dirent_is_absolute(mine_abspath));
  SVN_ERR_ASSERT(!older_abspath || svn_dirent_is_absolute(older_abspath));
  SVN_ERR_ASSERT(!yours_abspath || svn_dirent_is_absolute(yours_abspath));

  /* Check for an obstructed or missing node on disk. */
  {
    svn_wc_notify_state_t obstr_state;

    SVN_ERR(perform_obstruction_check(&obstr_state, NULL,
                                      &is_deleted, &wc_kind,
                                      merge_b, mine_abspath, svn_node_unknown,
                                      scratch_pool));
    if (obstr_state != svn_wc_notify_state_inapplicable)
      {
        /* ### a future thought:  if the file is under version control,
         * but the working file is missing, maybe we can 'restore' the
         * working file from the text-base, and then allow the merge to run? */

        if (content_state)
          *content_state = obstr_state;
        if (prop_state && obstr_state == svn_wc_notify_state_missing)
          *prop_state = svn_wc_notify_state_missing;
        return SVN_NO_ERROR;
      }
  }

  /* Other easy outs:  if the merge target isn't under version
     control, or is just missing from disk, fogettaboutit.  There's no
     way svn_wc_merge4() can do the merge. */
  if (wc_kind != svn_node_file || is_deleted)
    {
      /* Maybe the node is excluded via depth filtering? */

      if (wc_kind == svn_node_none)
        {
          svn_depth_t parent_depth;

          /* If the file isn't there due to depth restrictions, do not flag
           * a conflict. Non-inheritable mergeinfo will be recorded, allowing
           * future merges into non-shallow working copies to merge changes
           * we missed this time around. */
          SVN_ERR(svn_wc__node_get_depth(&parent_depth, merge_b->ctx->wc_ctx,
                                         svn_dirent_dirname(mine_abspath,
                                                            scratch_pool),
                                         scratch_pool));
          if (parent_depth < svn_depth_files
              && parent_depth != svn_depth_unknown)
            {
              if (content_state)
                *content_state = svn_wc_notify_state_missing;
              if (prop_state)
                *prop_state = svn_wc_notify_state_missing;
              return SVN_NO_ERROR;
            }
        }

      /* This is use case 4 described in the paper attached to issue
       * #2282.  See also notes/tree-conflicts/detection.txt
       */
      SVN_ERR(tree_conflict(merge_b, mine_abspath, svn_node_file,
                            svn_wc_conflict_action_edit,
                            svn_wc_conflict_reason_missing));
      if (tree_conflicted)
        *tree_conflicted = TRUE;
      if (content_state)
        *content_state = svn_wc_notify_state_missing;
      if (prop_state)
        *prop_state = svn_wc_notify_state_missing;
      return SVN_NO_ERROR;
    }

  /* ### TODO: Thwart attempts to merge into a path that has
     ### unresolved conflicts.  This needs to be smart enough to deal
     ### with tree conflicts!
  if (is_path_conflicted_by_merge(merge_b, mine))
    {
      *content_state = svn_wc_notify_state_conflicted;
      return svn_error_createf(SVN_ERR_WC_FOUND_CONFLICT, NULL,
                               _("Path '%s' is in conflict, and must be "
                                 "resolved before the remainder of the "
                                 "requested merge can be applied"), mine);
    }
  */

  /* This callback is essentially no more than a wrapper around
     svn_wc_merge4().  Thank goodness that all the
     diff-editor-mechanisms are doing the hard work of getting the
     fulltexts! */

  /* Do property merge before text merge so that keyword expansion takes
     into account the new property values. */
  if (prop_changes->nelts > 0)
    {
      svn_boolean_t tree_conflicted2 = FALSE;

      SVN_ERR(merge_props_changed(prop_state, &tree_conflicted2,
                                  mine_abspath, prop_changes, original_props,
                                  baton, scratch_pool));

      /* If the prop change caused a tree-conflict, just bail. */
      if (tree_conflicted2)
        {
          if (tree_conflicted != NULL)
            *tree_conflicted = TRUE;

          return SVN_NO_ERROR;
        }
    }
  else if (prop_state)
    *prop_state = svn_wc_notify_state_unchanged;

  /* Easy out: We are only applying mergeinfo differences. */
  if (merge_b->record_only)
    {
      if (content_state)
        *content_state = svn_wc_notify_state_unchanged;
      return SVN_NO_ERROR;
    }

  if (older_abspath)
    {
      svn_boolean_t has_local_mods;

      /* xgettext: the '.working', '.merge-left.r%ld' and
         '.merge-right.r%ld' strings are used to tag onto a file
         name in case of a merge conflict */
      const char *target_label = _(".working");
      const char *left_label = apr_psprintf(scratch_pool,
                                            _(".merge-left.r%ld"),
                                            older_rev);
      const char *right_label = apr_psprintf(scratch_pool,
                                             _(".merge-right.r%ld"),
                                             yours_rev);
      conflict_resolver_baton_t conflict_baton = { 0 };
      const svn_wc_conflict_version_t *left;
      const svn_wc_conflict_version_t *right;

      SVN_ERR(svn_wc_text_modified_p2(&has_local_mods, merge_b->ctx->wc_ctx,
                                      mine_abspath, FALSE, scratch_pool));

      conflict_baton.wrapped_func = merge_b->ctx->conflict_func2;
      conflict_baton.wrapped_baton = merge_b->ctx->conflict_baton2;
      conflict_baton.conflicted_paths = &merge_b->conflicted_paths;
      conflict_baton.pool = merge_b->pool;

      SVN_ERR(make_conflict_versions(&left, &right, mine_abspath,
                                     svn_node_file, merge_b));
      SVN_ERR(svn_wc_merge4(&merge_outcome, merge_b->ctx->wc_ctx,
                            older_abspath, yours_abspath, mine_abspath,
                            left_label, right_label, target_label,
                            left, right,
                            merge_b->dry_run, merge_b->diff3_cmd,
                            merge_b->merge_options, prop_changes,
                            conflict_resolver, &conflict_baton,
                            merge_b->ctx->cancel_func,
                            merge_b->ctx->cancel_baton,
                            scratch_pool));

      if (content_state)
        {
          if (merge_outcome == svn_wc_merge_conflict)
            *content_state = svn_wc_notify_state_conflicted;
          else if (has_local_mods
                   && merge_outcome != svn_wc_merge_unchanged)
            *content_state = svn_wc_notify_state_merged;
          else if (merge_outcome == svn_wc_merge_merged)
            *content_state = svn_wc_notify_state_changed;
          else if (merge_outcome == svn_wc_merge_no_merge)
            *content_state = svn_wc_notify_state_missing;
          else /* merge_outcome == svn_wc_merge_unchanged */
            *content_state = svn_wc_notify_state_unchanged;
        }
    }

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
merge_file_added(svn_wc_notify_state_t *content_state,
                 svn_wc_notify_state_t *prop_state,
                 svn_boolean_t *tree_conflicted,
                 const char *mine_abspath,
                 const char *older_abspath,
                 const char *yours_abspath,
                 svn_revnum_t rev1,
                 svn_revnum_t rev2,
                 const char *mimetype1,
                 const char *mimetype2,
                 const char *copyfrom_path,
                 svn_revnum_t copyfrom_revision,
                 const apr_array_header_t *prop_changes,
                 apr_hash_t *original_props,
                 void *baton,
                 apr_pool_t *scratch_pool)
{
  merge_cmd_baton_t *merge_b = baton;
  svn_node_kind_t kind;
  int i;
  apr_hash_t *file_props;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(mine_abspath));

  /* Easy out: We are only applying mergeinfo differences. */
  if (merge_b->record_only)
    {
      if (content_state)
        *content_state = svn_wc_notify_state_unchanged;
      if (prop_state)
        *prop_state = svn_wc_notify_state_unchanged;
      return SVN_NO_ERROR;
    }

  /* In most cases, we just leave prop_state as unknown, and let the
     content_state what happened, so we set prop_state here to avoid that
     below. */
  if (prop_state)
    *prop_state = svn_wc_notify_state_unknown;

  /* Apply the prop changes to a new hash table. */
  file_props = apr_hash_copy(scratch_pool, original_props);
  for (i = 0; i < prop_changes->nelts; ++i)
    {
      const svn_prop_t *prop = &APR_ARRAY_IDX(prop_changes, i, svn_prop_t);

      /* We don't want any DAV wcprops related to this file because
         they'll point to the wrong repository (in the
         merge-from-foreign-repository scenario) or wrong place in the
         right repository (in the same-repos scenario).  So we'll
         strip them.  (Is this a layering violation?)  */
      if (svn_property_kind(NULL, prop->name) == svn_prop_wc_kind)
        continue;

      /* And in the foreign repository merge case, we only want
         regular properties. */
      if ((! merge_b->same_repos)
          && (svn_property_kind(NULL, prop->name) != svn_prop_regular_kind))
        continue;

      /* Issue #3383: We don't want mergeinfo from a foreign repository. */
      if ((! merge_b->same_repos)
          && strcmp(prop->name, SVN_PROP_MERGEINFO) == 0)
        continue;

      apr_hash_set(file_props, prop->name, APR_HASH_KEY_STRING, prop->value);
    }

  /* Check for an obstructed or missing node on disk. */
  {
    svn_wc_notify_state_t obstr_state;

    SVN_ERR(perform_obstruction_check(&obstr_state, NULL, NULL,
                                      &kind,
                                      merge_b, mine_abspath, svn_node_unknown,
                                      scratch_pool));

    if (obstr_state != svn_wc_notify_state_inapplicable)
      {
        if (merge_b->dry_run && merge_b->added_path
            && svn_dirent_is_child(merge_b->added_path, mine_abspath, NULL))
          {
            if (content_state)
              *content_state = svn_wc_notify_state_changed;
            if (prop_state && apr_hash_count(file_props))
              *prop_state = svn_wc_notify_state_changed;
          }
        else if (content_state)
          *content_state = obstr_state;

        return SVN_NO_ERROR;
      }
  }

  SVN_ERR(svn_io_check_path(mine_abspath, &kind, scratch_pool));
  switch (kind)
    {
    case svn_node_none:
      {
        if (! merge_b->dry_run)
          {
            const char *copyfrom_url;
            svn_revnum_t copyfrom_rev;
            svn_stream_t *new_contents, *new_base_contents;
            apr_hash_t *new_base_props, *new_props;
            const svn_wc_conflict_description2_t *existing_conflict;

            /* If this is a merge from the same repository as our
               working copy, we handle adds as add-with-history.
               Otherwise, we'll use a pure add. */
            if (merge_b->same_repos)
              {
                const char *child =
                  svn_dirent_is_child(merge_b->target_abspath, mine_abspath,
                                      scratch_pool);
                if (child != NULL)
                  copyfrom_url = svn_path_url_add_component2(
                                               merge_b->merge_source.url2,
                                               child, scratch_pool);
                else
                  copyfrom_url = merge_b->merge_source.url2;
                copyfrom_rev = rev2;
                SVN_ERR(check_repos_match(merge_b, mine_abspath, copyfrom_url,
                                          scratch_pool));
                new_base_props = file_props;
                new_props = NULL; /* inherit from new_base_props */
                SVN_ERR(svn_stream_open_readonly(&new_base_contents,
                                                 yours_abspath,
                                                 scratch_pool,
                                                 scratch_pool));
                new_contents = NULL; /* inherit from new_base_contents */
              }
            else
              {
                copyfrom_url = NULL;
                copyfrom_rev = SVN_INVALID_REVNUM;
                new_base_props = apr_hash_make(scratch_pool);
                new_props = file_props;
                new_base_contents = svn_stream_empty(scratch_pool);
                SVN_ERR(svn_stream_open_readonly(&new_contents, yours_abspath,
                                                 scratch_pool, scratch_pool));
              }

            SVN_ERR(svn_wc__get_tree_conflict(&existing_conflict,
                                              merge_b->ctx->wc_ctx,
                                              mine_abspath, merge_b->pool,
                                              merge_b->pool));
            if (existing_conflict)
              {
                /* Possibly collapse the existing conflict into a 'replace'
                 * tree conflict. The conflict reason is 'added' because
                 * the now-deleted tree conflict victim must have been
                 * added in the history of the merge target. */
                SVN_ERR(tree_conflict_on_add(merge_b, mine_abspath,
                                             svn_node_file,
                                             svn_wc_conflict_action_add,
                                             svn_wc_conflict_reason_added));
                if (tree_conflicted)
                  *tree_conflicted = TRUE;
              }
            else
              {
                /* Since 'mine' doesn't exist, and this is
                   'merge_file_added', I hope it's safe to assume that
                   'older' is empty, and 'yours' is the full file.  Merely
                   copying 'yours' to 'mine', isn't enough; we need to get
                   the whole text-base and props installed too, just as if
                   we had called 'svn cp wc wc'. */
                SVN_ERR(svn_wc_add_repos_file4(merge_b->ctx->wc_ctx,
                                               mine_abspath,
                                               new_base_contents,
                                               new_contents,
                                               new_base_props, new_props,
                                               copyfrom_url, copyfrom_rev,
                                               merge_b->ctx->cancel_func,
                                               merge_b->ctx->cancel_baton,
                                               scratch_pool));

                /* ### delete 'yours' ? */
              }
          }
        if (content_state)
          *content_state = svn_wc_notify_state_changed;
        if (prop_state && apr_hash_count(file_props))
          *prop_state = svn_wc_notify_state_changed;
      }
      break;
    case svn_node_dir:
      /* The file add the merge wants to carry out is obstructed by
       * a directory, so the file the merge wants to add is a tree
       * conflict victim.
       * See notes about obstructions in notes/tree-conflicts/detection.txt.
       */
      SVN_ERR(tree_conflict_on_add(merge_b, mine_abspath, svn_node_file,
                                   svn_wc_conflict_action_add,
                                   svn_wc_conflict_reason_obstructed));
      if (tree_conflicted)
        *tree_conflicted = TRUE;
      if (content_state)
        {
          /* directory already exists, is it under version control? */
          svn_node_kind_t wc_kind;
          SVN_ERR(svn_wc_read_kind(&wc_kind, merge_b->ctx->wc_ctx,
                                   mine_abspath, FALSE, scratch_pool));

          if ((wc_kind != svn_node_none)
              && dry_run_deleted_p(merge_b, mine_abspath))
            *content_state = svn_wc_notify_state_changed;
          else
            /* this will make the repos_editor send a 'skipped' message */
            *content_state = svn_wc_notify_state_obstructed;
        }
      break;
    case svn_node_file:
      {
            if (dry_run_deleted_p(merge_b, mine_abspath))
              {
                if (content_state)
                  *content_state = svn_wc_notify_state_changed;
              }
            else
              {
                /* The file add the merge wants to carry out is obstructed by
                 * a versioned file. This file must have been added in the
                 * history of the merge target, hence we flag a tree conflict
                 * with reason 'added'. */
                SVN_ERR(tree_conflict_on_add(
                          merge_b, mine_abspath, svn_node_file,
                          svn_wc_conflict_action_add,
                          svn_wc_conflict_reason_added));

                if (tree_conflicted)
                  *tree_conflicted = TRUE;
              }
        break;
      }
    default:
      if (content_state)
        *content_state = svn_wc_notify_state_unknown;
      break;
    }

  return SVN_NO_ERROR;
}

/* Compare the two sets of properties PROPS1 and PROPS2, ignoring the
 * "svn:mergeinfo" property, and noticing only "normal" props. Set *SAME to
 * true if the rest of the properties are identical or false if they differ.
 */
static svn_error_t *
properties_same_p(svn_boolean_t *same,
                  apr_hash_t *props1,
                  apr_hash_t *props2,
                  apr_pool_t *scratch_pool)
{
  apr_array_header_t *prop_changes;
  int i, diffs;

  /* Examine the properties that differ */
  SVN_ERR(svn_prop_diffs(&prop_changes, props1, props2, scratch_pool));
  diffs = 0;
  for (i = 0; i < prop_changes->nelts; i++)
    {
      const char *pname = APR_ARRAY_IDX(prop_changes, i, svn_prop_t).name;

      /* Count the properties we're interested in; ignore the rest */
      if (svn_wc_is_normal_prop(pname)
          && strcmp(pname, SVN_PROP_MERGEINFO) != 0)
        diffs++;
    }
  *same = (diffs == 0);
  return SVN_NO_ERROR;
}

/* Compare the file OLDER_ABSPATH (together with its normal properties in
 * ORIGINAL_PROPS which may also contain WC props and entry props) with the
 * versioned file MINE_ABSPATH (together with its versioned properties).
 * Set *SAME to true if they are the same or false if they differ, ignoring
 * the "svn:mergeinfo" property, and ignoring differences in keyword
 * expansion and end-of-line style. */
static svn_error_t *
files_same_p(svn_boolean_t *same,
             const char *older_abspath,
             apr_hash_t *original_props,
             const char *mine_abspath,
             svn_wc_context_t *wc_ctx,
             apr_pool_t *scratch_pool)
{
  apr_hash_t *working_props;

  SVN_ERR(svn_wc_prop_list2(&working_props, wc_ctx, mine_abspath,
                            scratch_pool, scratch_pool));

  /* Compare the properties */
  SVN_ERR(properties_same_p(same, original_props, working_props,
                            scratch_pool));
  if (*same)
    {
      svn_stream_t *mine_stream;
      svn_stream_t *older_stream;
      svn_opt_revision_t working_rev;

      working_rev.kind = svn_opt_revision_working;

      /* Compare the file content, translating 'mine' to 'normal' form. */
      if (svn_prop_get_value(working_props, SVN_PROP_SPECIAL) != NULL)
        SVN_ERR(svn_subst_read_specialfile(&mine_stream, mine_abspath,
                                           scratch_pool, scratch_pool));
      else
        SVN_ERR(svn_client__get_normalized_stream(&mine_stream, wc_ctx,
                                                  mine_abspath, &working_rev,
                                                  FALSE, TRUE, NULL, NULL,
                                                  scratch_pool, scratch_pool));

      SVN_ERR(svn_stream_open_readonly(&older_stream, older_abspath,
                                       scratch_pool, scratch_pool));

      SVN_ERR(svn_stream_contents_same2(same, mine_stream, older_stream,
                                        scratch_pool));

    }

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
merge_file_deleted(svn_wc_notify_state_t *state,
                   svn_boolean_t *tree_conflicted,
                   const char *mine_abspath,
                   const char *older_abspath,
                   const char *yours_abspath,
                   const char *mimetype1,
                   const char *mimetype2,
                   apr_hash_t *original_props,
                   void *baton,
                   apr_pool_t *scratch_pool)
{
  merge_cmd_baton_t *merge_b = baton;
  svn_node_kind_t kind;

  if (merge_b->dry_run)
    {
      const char *wcpath = apr_pstrdup(merge_b->pool, mine_abspath);
      apr_hash_set(merge_b->dry_run_deletions, wcpath,
                   APR_HASH_KEY_STRING, wcpath);
    }

  /* Easy out: We are only applying mergeinfo differences. */
  if (merge_b->record_only)
    {
      if (state)
        *state = svn_wc_notify_state_unchanged;
      return SVN_NO_ERROR;
    }

  /* Check for an obstructed or missing node on disk. */
  {
    svn_wc_notify_state_t obstr_state;

    SVN_ERR(perform_obstruction_check(&obstr_state, NULL, NULL,
                                      NULL,
                                      merge_b, mine_abspath, svn_node_unknown,
                                      scratch_pool));

    if (obstr_state != svn_wc_notify_state_inapplicable)
      {
        if (state)
          *state = obstr_state;
        return SVN_NO_ERROR;
      }
  }

  SVN_ERR(svn_io_check_path(mine_abspath, &kind, scratch_pool));
  switch (kind)
    {
    case svn_node_file:
      {
        svn_boolean_t same;

        /* If the files are identical, attempt deletion */
        SVN_ERR(files_same_p(&same, older_abspath, original_props,
                             mine_abspath, merge_b->ctx->wc_ctx,
                             scratch_pool));
        if (same || merge_b->force || merge_b->record_only /* ### why? */)
          {
            /* Passing NULL for the notify_func and notify_baton because
               repos_diff.c:delete_entry() will do it for us. */
            SVN_ERR(svn_client__wc_delete(mine_abspath, TRUE,
                                          merge_b->dry_run, FALSE, NULL, NULL,
                                          merge_b->ctx, scratch_pool));
            if (state)
              *state = svn_wc_notify_state_changed;
          }
        else
          {
            /* The files differ, so raise a conflict instead of deleting */

            /* This might be use case 5 described in the paper attached to issue
             * #2282.  See also notes/tree-conflicts/detection.txt
             */
            SVN_ERR(tree_conflict(merge_b, mine_abspath, svn_node_file,
                                  svn_wc_conflict_action_delete,
                                  svn_wc_conflict_reason_edited));
            if (tree_conflicted)
              *tree_conflicted = TRUE;

            if (state)
              *state = svn_wc_notify_state_obstructed;
          }
      }
      break;
    case svn_node_dir:
      /* The file deletion the merge wants to carry out is obstructed by
       * a directory, so the file the merge wants to delete is a tree
       * conflict victim.
       * See notes about obstructions in notes/tree-conflicts/detection.txt.
       */
      SVN_ERR(tree_conflict(merge_b, mine_abspath, svn_node_file,
                            svn_wc_conflict_action_delete,
                            svn_wc_conflict_reason_obstructed));
      if (tree_conflicted)
        *tree_conflicted = TRUE;
      if (state)
        *state = svn_wc_notify_state_obstructed;
      break;
    case svn_node_none:
      /* The file deleted in the diff does not exist at the current URL.
       *
       * This is use case 6 described in the paper attached to issue
       * #2282.  See also notes/tree-conflicts/detection.txt
       */
      SVN_ERR(tree_conflict(merge_b, mine_abspath, svn_node_file,
                            svn_wc_conflict_action_delete,
                            svn_wc_conflict_reason_deleted));
      if (tree_conflicted)
        *tree_conflicted = TRUE;
      if (state)
        *state = svn_wc_notify_state_missing;
      break;
    default:
      if (state)
        *state = svn_wc_notify_state_unknown;
      break;
    }

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
merge_dir_added(svn_wc_notify_state_t *state,
                svn_boolean_t *tree_conflicted,
                svn_boolean_t *skip,
                svn_boolean_t *skip_children,
                const char *local_abspath,
                svn_revnum_t rev,
                const char *copyfrom_path,
                svn_revnum_t copyfrom_revision,
                void *baton,
                apr_pool_t *scratch_pool)
{
  merge_cmd_baton_t *merge_b = baton;
  svn_node_kind_t kind;
  const char *copyfrom_url = NULL, *child;
  svn_revnum_t copyfrom_rev = SVN_INVALID_REVNUM;
  const char *parent_abspath;
  svn_boolean_t is_versioned;
  svn_boolean_t is_deleted;

  /* Easy out: We are only applying mergeinfo differences. */
  if (merge_b->record_only)
    {
      if (state)
        *state = svn_wc_notify_state_unchanged;
      return SVN_NO_ERROR;
    }

  parent_abspath = svn_dirent_dirname(local_abspath, scratch_pool);

  child = svn_dirent_is_child(merge_b->target_abspath, local_abspath, NULL);
  SVN_ERR_ASSERT(child != NULL);

  /* If this is a merge from the same repository as our working copy,
     we handle adds as add-with-history.  Otherwise, we'll use a pure
     add. */
  if (merge_b->same_repos)
    {
      copyfrom_url = svn_path_url_add_component2(merge_b->merge_source.url2,
                                                 child, scratch_pool);
      copyfrom_rev = rev;

      SVN_ERR(check_repos_match(merge_b, parent_abspath, copyfrom_url,
                                scratch_pool));
    }

  /* Check for an obstructed or missing node on disk. */
  {
    svn_wc_notify_state_t obstr_state;

    SVN_ERR(perform_obstruction_check(&obstr_state, NULL,
                                      &is_deleted, &kind,
                                      merge_b, local_abspath, svn_node_unknown,
                                      scratch_pool));

    is_versioned = (kind == svn_node_dir) || (kind == svn_node_file);

    /* In this case of adding a directory, we have an exception to the usual
     * "skip if it's inconsistent" rule. If the directory exists on disk
     * unexpectedly, we simply make it versioned, because we can do so without
     * risk of destroying data. Only skip if it is versioned but unexpectedly
     * missing from disk, or is unversioned but obstructed by a node of the
     * wrong kind. */
    if (obstr_state == svn_wc_notify_state_obstructed
        && (is_deleted || kind == svn_node_none))
      {
        svn_node_kind_t disk_kind;

        SVN_ERR(svn_io_check_path(local_abspath, &disk_kind, scratch_pool));

        if (disk_kind == svn_node_dir)
          {
            obstr_state = svn_wc_notify_state_inapplicable;
            kind = svn_node_dir; /* Take over existing directory */
          }
      }

    if (obstr_state != svn_wc_notify_state_inapplicable)
      {
        if (state && merge_b->dry_run && merge_b->added_path
            && svn_dirent_is_child(merge_b->added_path, local_abspath, NULL))
          {
            *state = svn_wc_notify_state_changed;
          }
        else if (state)
          *state = obstr_state;
        return SVN_NO_ERROR;
      }

    if (is_deleted)
      kind = svn_node_none;
  }

  /* Switch on the on-disk state of this path */
  switch (kind)
    {
    case svn_node_none:
      /* Unversioned or schedule-delete */
      if (merge_b->dry_run)
        {
          merge_b->added_path = apr_pstrdup(merge_b->pool, local_abspath);
          apr_hash_set(merge_b->dry_run_added, merge_b->added_path,
                       APR_HASH_KEY_STRING, merge_b->added_path);
        }
      else
        {
          SVN_ERR(svn_io_dir_make(local_abspath, APR_OS_DEFAULT,
                                  scratch_pool));
          SVN_ERR(svn_wc_add4(merge_b->ctx->wc_ctx, local_abspath,
                              svn_depth_infinity,
                              copyfrom_url, copyfrom_rev,
                              merge_b->ctx->cancel_func,
                              merge_b->ctx->cancel_baton,
                              NULL, NULL, /* don't pass notification func! */
                              scratch_pool));

        }
      if (state)
        *state = svn_wc_notify_state_changed;
      break;
    case svn_node_dir:
      /* Adding an unversioned directory doesn't destroy data */
      if (! is_versioned || is_deleted)
        {
          /* The dir is not known to Subversion, or is schedule-delete.
           * We will make it schedule-add. */
          if (!merge_b->dry_run)
            SVN_ERR(svn_wc_add4(merge_b->ctx->wc_ctx, local_abspath,
                                svn_depth_infinity,
                                copyfrom_url, copyfrom_rev,
                                merge_b->ctx->cancel_func,
                                merge_b->ctx->cancel_baton,
                                NULL, NULL, /* no notification func! */
                                scratch_pool));
          else
            merge_b->added_path = apr_pstrdup(merge_b->pool, local_abspath);
          if (state)
            *state = svn_wc_notify_state_changed;
        }
      else
        {
          /* The dir is known to Subversion as already existing. */
          if (dry_run_deleted_p(merge_b, local_abspath))
            {
              if (state)
                *state = svn_wc_notify_state_changed;
            }
          else
            {
              /* This is a tree conflict. */
              SVN_ERR(tree_conflict_on_add(merge_b, local_abspath,
                                           svn_node_dir,
                                           svn_wc_conflict_action_add,
                                           svn_wc_conflict_reason_added));
              if (tree_conflicted)
                *tree_conflicted = TRUE;
              if (state)
                *state = svn_wc_notify_state_obstructed;
            }
        }
      break;
    case svn_node_file:
      if (merge_b->dry_run)
        merge_b->added_path = NULL;

      if (is_versioned && dry_run_deleted_p(merge_b, local_abspath))
        {
          /* ### TODO: Retain record of this dir being added to
             ### avoid problems from subsequent edits which try to
             ### add children. */
          if (state)
            *state = svn_wc_notify_state_changed;
        }
      else
        {
          /* Obstructed: we can't add a dir because there's a file here. */
          SVN_ERR(tree_conflict_on_add(merge_b, local_abspath, svn_node_dir,
                                       svn_wc_conflict_action_add,
                                       svn_wc_conflict_reason_obstructed));
          if (tree_conflicted)
            *tree_conflicted = TRUE;
          if (state)
            *state = svn_wc_notify_state_obstructed;
        }
      break;
    default:
      if (merge_b->dry_run)
        merge_b->added_path = NULL;
      if (state)
        *state = svn_wc_notify_state_unknown;
      break;
    }

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
merge_dir_deleted(svn_wc_notify_state_t *state,
                  svn_boolean_t *tree_conflicted,
                  const char *local_abspath,
                  void *baton,
                  apr_pool_t *scratch_pool)
{
  merge_cmd_baton_t *merge_b = baton;
  svn_node_kind_t kind;
  svn_error_t *err;
  svn_boolean_t is_versioned;
  svn_boolean_t is_deleted;

  /* Easy out: We are only applying mergeinfo differences. */
  if (merge_b->record_only)
    {
      if (state)
        *state = svn_wc_notify_state_unchanged;
      return SVN_NO_ERROR;
    }

  /* Check for an obstructed or missing node on disk. */
  {
    svn_wc_notify_state_t obstr_state;

    SVN_ERR(perform_obstruction_check(&obstr_state, NULL,
                                      &is_deleted, &kind,
                                      merge_b, local_abspath, svn_node_unknown,
                                      scratch_pool));

    is_versioned = (kind == svn_node_dir) || (kind == svn_node_file);

    if (obstr_state != svn_wc_notify_state_inapplicable)
      {
        if (state)
          *state = obstr_state;
        return SVN_NO_ERROR;
      }

    if (is_deleted)
      kind = svn_node_none;
  }

  if (merge_b->dry_run)
    {
      const char *wcpath = apr_pstrdup(merge_b->pool, local_abspath);
      apr_hash_set(merge_b->dry_run_deletions, wcpath,
                   APR_HASH_KEY_STRING, wcpath);
    }


  /* Switch on the wc state of this path */
  switch (kind)
    {
    case svn_node_dir:
      {
        if (is_versioned && !is_deleted)
          {
            /* ### TODO: Before deleting, we should ensure that this dir
               tree is equal to the one we're being asked to delete.
               If not, mark this directory as a tree conflict victim,
               because this could be use case 5 as described in
               notes/tree-conflicts/detection.txt.
             */

            /* Passing NULL for the notify_func and notify_baton because
               repos_diff.c:delete_entry() will do it for us. */
            err = svn_client__wc_delete(local_abspath, merge_b->force,
                                        merge_b->dry_run, FALSE,
                                        NULL, NULL,
                                        merge_b->ctx, scratch_pool);
            if (err)
              {
                svn_error_clear(err);

                /* If the attempt to delete an existing directory failed,
                 * the directory has local modifications (e.g. locally added
                 * files, or property changes). Flag a tree conflict. */
                SVN_ERR(tree_conflict(merge_b, local_abspath, svn_node_dir,
                                      svn_wc_conflict_action_delete,
                                      svn_wc_conflict_reason_edited));
                if (tree_conflicted)
                  *tree_conflicted = TRUE;
                if (state)
                  *state = svn_wc_notify_state_conflicted;
              }
            else
              {
                if (state)
                  *state = svn_wc_notify_state_changed;
              }
          }
        else
          {
            /* Dir is already not under version control at this path. */
            /* Raise a tree conflict. */
            SVN_ERR(tree_conflict(merge_b, local_abspath, svn_node_dir,
                                  svn_wc_conflict_action_delete,
                                  svn_wc_conflict_reason_deleted));
            if (tree_conflicted)
              *tree_conflicted = TRUE;
          }
      }
      break;
    case svn_node_file:
      if (state)
        *state = svn_wc_notify_state_obstructed;
      break;
    case svn_node_none:
      /* Dir is already non-existent. This is use case 6 as described in
       * notes/tree-conflicts/detection.txt.
       * This case was formerly treated as no-op. */
      SVN_ERR(tree_conflict(merge_b, local_abspath, svn_node_dir,
                            svn_wc_conflict_action_delete,
                            svn_wc_conflict_reason_deleted));
      if (tree_conflicted)
        *tree_conflicted = TRUE;
      if (state)
        *state = svn_wc_notify_state_missing;
      break;
    default:
      if (state)
        *state = svn_wc_notify_state_unknown;
      break;
    }

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
merge_dir_opened(svn_boolean_t *tree_conflicted,
                 svn_boolean_t *skip,
                 svn_boolean_t *skip_children,
                 const char *local_abspath,
                 svn_revnum_t rev,
                 void *baton,
                 apr_pool_t *scratch_pool)
{
  merge_cmd_baton_t *merge_b = baton;
  svn_depth_t parent_depth;
  svn_node_kind_t wc_kind;
  svn_wc_notify_state_t obstr_state;
  svn_boolean_t is_deleted;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  /* Check for an obstructed or missing node on disk. */
  SVN_ERR(perform_obstruction_check(&obstr_state, NULL,
                                    &is_deleted, &wc_kind,
                                    merge_b, local_abspath, svn_node_unknown,
                                    scratch_pool));

  if (obstr_state != svn_wc_notify_state_inapplicable)
    {
      if (skip_children)
        *skip_children = TRUE;
      /* But don't skip THIS, to allow a skip notification */
      return SVN_NO_ERROR;
    }

  if (wc_kind != svn_node_dir || is_deleted)
    {
      if (wc_kind == svn_node_none)
        {
          /* If the parent is too shallow to contain this directory,
           * and the directory is not present on disk, skip it.
           * Non-inheritable mergeinfo will be recorded, allowing
           * future merges into non-shallow working copies to merge
           * changes we missed this time around. */
          SVN_ERR(svn_wc__node_get_depth(&parent_depth, merge_b->ctx->wc_ctx,
                                         svn_dirent_dirname(local_abspath,
                                                            scratch_pool),
                                         scratch_pool));
          if (parent_depth != svn_depth_unknown &&
              parent_depth < svn_depth_immediates)
            {
              if (skip_children)
                *skip_children = TRUE;
              return SVN_NO_ERROR;
            }
        }

      /* Check for tree conflicts, if any. */

      /* If we're trying to open a file, the reason for the conflict is
       * 'replaced'. Because the merge is trying to open the directory,
       * rather than adding it, the directory must have existed in the
       * history of the target branch and has been replaced with a file. */
      if (wc_kind == svn_node_file)
        {
          SVN_ERR(tree_conflict(merge_b, local_abspath, svn_node_dir,
                                svn_wc_conflict_action_edit,
                                svn_wc_conflict_reason_replaced));
          if (tree_conflicted)
            *tree_conflicted = TRUE;
        }

      /* If we're trying to open a directory that's locally deleted,
       * or not present because it was deleted in the history of the
       * target branch, the reason for the conflict is 'deleted'.
       *
       * If the DB says something should be here, but there is
       * nothing on disk, we're probably in a mixed-revision
       * working copy and the parent has an outdated idea about
       * the state of its child. Flag a tree conflict in this case
       * forcing the user to sanity-check the merge result. */
      else if (is_deleted || wc_kind == svn_node_none)
        {
          SVN_ERR(tree_conflict(merge_b, local_abspath, svn_node_dir,
                                svn_wc_conflict_action_edit,
                                svn_wc_conflict_reason_deleted));
          if (tree_conflicted)
            *tree_conflicted = TRUE;
        }
    }

  return SVN_NO_ERROR;
}

/* An svn_wc_diff_callbacks4_t function. */
static svn_error_t *
merge_dir_closed(svn_wc_notify_state_t *contentstate,
                 svn_wc_notify_state_t *propstate,
                 svn_boolean_t *tree_conflicted,
                 const char *path,
                 svn_boolean_t dir_was_added,
                 void *baton,
                 apr_pool_t *scratch_pool)
{
  merge_cmd_baton_t *merge_b = baton;

  if (merge_b->dry_run)
    svn_hash__clear(merge_b->dry_run_deletions, scratch_pool);

  return SVN_NO_ERROR;
}

/* The main callback table for 'svn merge'.  */
static const svn_wc_diff_callbacks4_t
merge_callbacks =
  {
    merge_file_opened,
    merge_file_changed,
    merge_file_added,
    merge_file_deleted,
    merge_dir_deleted,
    merge_dir_opened,
    merge_dir_added,
    merge_dir_props_changed,
    merge_dir_closed
  };


/*-----------------------------------------------------------------------*/

/*** Merge Notification ***/


/* Contains any state collected while receiving path notifications. */
typedef struct notification_receiver_baton_t
{
  /* The wrapped callback and baton. */
  svn_wc_notify_func2_t wrapped_func;
  void *wrapped_baton;

  /* The number of notifications received. */
  apr_uint32_t nbr_notifications;

  /* The number of operative notifications received. */
  apr_uint32_t nbr_operative_notifications;

  /* The list of absolute merged paths.  Is NULL if MERGE_B->SOURCES_ANCESTRAL
     and MERGE_B->REINTEGRATE_MERGE are both false. */
  apr_hash_t *merged_abspaths;

  /* The list of absolute skipped paths, which should be examined and
     cleared after each invocation of the callback.  The paths
     are absolute.  Is NULL if MERGE_B->SOURCES_ANCESTRAL and
     MERGE_B->REINTEGRATE_MERGE are both false. */
  apr_hash_t *skipped_abspaths;

  /* A list of the absolute root paths of any added subtrees which might
     require their own explicit mergeinfo.  Is NULL if
     MERGE_B->SOURCES_ANCESTRAL and MERGE_B->REINTEGRATE_MERGE are both
     false. */
  apr_hash_t *added_abspaths;

  /* A list of tree conflict victim absolute paths which may be NULL.  Is NULL
     if MERGE_B->SOURCES_ANCESTRAL and MERGE_B->REINTEGRATE_MERGE are both
     false. */
  apr_hash_t *tree_conflicted_abspaths;

  /* Flag indicating whether it is a single file merge or not. */
  svn_boolean_t is_single_file_merge;

  /* Depth first ordered list of paths that needs special care while merging.
     This defaults to NULL. For 'same_url' merge alone we set it to
     proper array. This is used by notification_receiver to put a
     merge notification begin lines. */
  apr_array_header_t *children_with_mergeinfo;

  /* The index in CHILDREN_WITH_MERGEINFO where we found the nearest ancestor
     for merged path. Default value is '-1'.*/
  int cur_ancestor_index;

  /* We use this to make a decision on merge begin line notifications. */
  merge_cmd_baton_t *merge_b;

  /* Pool used in notification_receiver() to avoid the iteration
     sub-pool which is passed in, then subsequently destroyed. */
  apr_pool_t *pool;

} notification_receiver_baton_t;


/* Finds a nearest ancestor in CHILDREN_WITH_MERGEINFO for PATH. If
   PATH_IS_OWN_ANCESTOR is TRUE then a child in CHILDREN_WITH_MERGEINFO
   where child->abspath == PATH is considered PATH's ancestor.  If FALSE,
   then child->abspath must be a proper ancestor of PATH.

   CHILDREN_WITH_MERGEINFO is expected to be sorted in Depth first
   order of path. Nearest ancestor's index from
   CHILDREN_WITH_MERGEINFO is returned. */
static int
find_nearest_ancestor(const apr_array_header_t *children_with_mergeinfo,
                      svn_boolean_t path_is_own_ancestor,
                      const char *path)
{
  int i;
  int ancestor_index = 0;

  /* This if condition is not needed as this function should be used
     from the context of same_url merge where CHILDREN_WITH_MERGEINFO
     will not be NULL and of size atleast 1. We have this if condition
     just to protect the wrong caller. */
  if (!children_with_mergeinfo)
    return 0;
  for (i = 0; i < children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
        APR_ARRAY_IDX(children_with_mergeinfo, i, svn_client__merge_path_t *);
      if (svn_dirent_is_ancestor(child->abspath, path)
          && (path_is_own_ancestor
              || svn_path_compare_paths(child->abspath, path) != 0))
        ancestor_index = i;
    }
  return ancestor_index;
}


#define IS_OPERATIVE_NOTIFICATION(notify)  \
                    (notify->content_state == svn_wc_notify_state_conflicted \
                     || notify->content_state == svn_wc_notify_state_merged  \
                     || notify->content_state == svn_wc_notify_state_changed \
                     || notify->prop_state == svn_wc_notify_state_conflicted \
                     || notify->prop_state == svn_wc_notify_state_merged     \
                     || notify->prop_state == svn_wc_notify_state_changed    \
                     || notify->action == svn_wc_notify_update_add \
                     || notify->action == svn_wc_notify_tree_conflict)

/* Our svn_wc_notify_func2_t wrapper.*/
static void
notification_receiver(void *baton, const svn_wc_notify_t *notify,
                      apr_pool_t *pool)
{
  notification_receiver_baton_t *notify_b = baton;
  svn_boolean_t is_operative_notification = FALSE;

  /* Skip notifications if this is a --record-only merge that is adding
     or deleting NOTIFY->PATH, allow only mergeinfo changes and headers.
     We will already have skipped the actual addition or deletion, but will
     still get a notification callback for it. */
  if (notify_b->merge_b->record_only
      && (notify->action != svn_wc_notify_update_update
          && notify->action != svn_wc_notify_merge_record_info_begin))
    return;

  /* Is the notification the result of a real operative merge? */
  if (IS_OPERATIVE_NOTIFICATION(notify))
    {
      notify_b->nbr_operative_notifications++;
      is_operative_notification = TRUE;
    }

  if (notify_b->merge_b->sources_ancestral
      || notify_b->merge_b->reintegrate_merge)
    {
      if (notify->content_state == svn_wc_notify_state_merged
          || notify->content_state == svn_wc_notify_state_changed
          || notify->prop_state == svn_wc_notify_state_merged
          || notify->prop_state == svn_wc_notify_state_changed
          || notify->action == svn_wc_notify_update_add)
        {
          const char *merged_path = apr_pstrdup(notify_b->pool, notify->path);

          if (notify_b->merged_abspaths == NULL)
            notify_b->merged_abspaths = apr_hash_make(notify_b->pool);

          apr_hash_set(notify_b->merged_abspaths, merged_path,
                       APR_HASH_KEY_STRING, merged_path);
        }

      if (notify->action == svn_wc_notify_skip)
        {
          const char *skipped_path = apr_pstrdup(notify_b->pool, notify->path);

          if (notify_b->skipped_abspaths == NULL)
            notify_b->skipped_abspaths = apr_hash_make(notify_b->pool);

          apr_hash_set(notify_b->skipped_abspaths, skipped_path,
                       APR_HASH_KEY_STRING, skipped_path);
        }

      if (notify->action == svn_wc_notify_tree_conflict)
        {
          const char *tree_conflicted_path = apr_pstrdup(notify_b->pool,
                                                         notify->path);

          if (notify_b->tree_conflicted_abspaths == NULL)
            notify_b->tree_conflicted_abspaths =
              apr_hash_make(notify_b->pool);

          apr_hash_set(notify_b->tree_conflicted_abspaths,
                       tree_conflicted_path, APR_HASH_KEY_STRING,
                       tree_conflicted_path);
        }

      if (notify->action == svn_wc_notify_update_add)
        {
          svn_boolean_t root_of_added_subtree = TRUE;

          /* Stash the root path of any added subtrees. */
          if (notify_b->added_abspaths == NULL)
            {
              /* The first added path is always a root. */
              notify_b->added_abspaths = apr_hash_make(notify_b->pool);
            }
          else
            {
              const char *added_path_parent =
                svn_dirent_dirname(notify->path, pool);
              apr_pool_t *subpool = svn_pool_create(pool);

              /* Is NOTIFY->PATH the root of an added subtree? */
              while (strcmp(notify_b->merge_b->target_abspath,
                            added_path_parent))
                {
                  if (apr_hash_get(notify_b->added_abspaths,
                                   added_path_parent,
                                   APR_HASH_KEY_STRING))
                    {
                      root_of_added_subtree = FALSE;
                      break;
                    }

                  added_path_parent = svn_dirent_dirname(
                    added_path_parent, subpool);
                }

              svn_pool_destroy(subpool);
            }

          if (root_of_added_subtree)
            {
              const char *added_root_path = apr_pstrdup(notify_b->pool,
                                                        notify->path);
              apr_hash_set(notify_b->added_abspaths, added_root_path,
                           APR_HASH_KEY_STRING, added_root_path);
            }
        }

      if (notify->action == svn_wc_notify_update_delete
          && notify_b->added_abspaths)
        {
          /* Issue #4166: If a previous merge added NOTIFY_ABSPATH, but we
             are now deleting it, then remove it from the list of added
             paths. */
          apr_hash_set(notify_b->added_abspaths, notify->path,
                       APR_HASH_KEY_STRING, NULL);
        }
    }

  /* If our merge sources are ancestors of one another... */
  if (notify_b->merge_b->sources_ancestral)
    {
      notify_b->nbr_notifications++;

      /* See if this is an operative directory merge. */
      if (!(notify_b->is_single_file_merge) && is_operative_notification)
        {
          /* Find NOTIFY->PATH's nearest ancestor in
             NOTIFY->CHILDREN_WITH_MERGEINFO.  Normally we consider a child in
             NOTIFY->CHILDREN_WITH_MERGEINFO representing PATH to be an
             ancestor of PATH, but if this is a deletion of PATH then the
             notification must be for a proper ancestor of PATH.  This ensures
             we don't get notifications like:

               --- Merging rX into 'PARENT/CHILD'
               D    PARENT/CHILD

             But rather:

               --- Merging rX into 'PARENT'
               D    PARENT/CHILD
          */
          int new_nearest_ancestor_index =
            find_nearest_ancestor(
              notify_b->children_with_mergeinfo,
              notify->action != svn_wc_notify_update_delete,
              notify->path);

          if (new_nearest_ancestor_index != notify_b->cur_ancestor_index)
            {
              svn_client__merge_path_t *child =
                APR_ARRAY_IDX(notify_b->children_with_mergeinfo,
                              new_nearest_ancestor_index,
                              svn_client__merge_path_t *);
              notify_b->cur_ancestor_index = new_nearest_ancestor_index;
              if (!child->absent && child->remaining_ranges->nelts > 0
                  && !(new_nearest_ancestor_index == 0
                       && child->remaining_ranges == 0))
                {
                  svn_wc_notify_t *notify_merge_begin;
                  notify_merge_begin =
                    svn_wc_create_notify(child->abspath,
                                         notify_b->merge_b->same_repos
                                           ? svn_wc_notify_merge_begin
                                           : svn_wc_notify_foreign_merge_begin,
                                         pool);
                  notify_merge_begin->merge_range =
                    APR_ARRAY_IDX(child->remaining_ranges, 0,
                                  svn_merge_range_t *);
                  if (notify_b->wrapped_func)
                    (*notify_b->wrapped_func)(notify_b->wrapped_baton,
                                              notify_merge_begin, pool);
                }
            }
        }
    }
  /* Otherwise, our merge sources aren't ancestors of one another. */
  else if (!(notify_b->is_single_file_merge)
           && notify_b->nbr_operative_notifications == 1
           && is_operative_notification)
    {
      svn_wc_notify_t *notify_merge_begin;
      notify_merge_begin =
        svn_wc_create_notify(notify_b->merge_b->target_abspath,
                             notify_b->merge_b->same_repos
                               ? svn_wc_notify_merge_begin
                               : svn_wc_notify_foreign_merge_begin,
                             pool);
      if (notify_b->wrapped_func)
        (*notify_b->wrapped_func)(notify_b->wrapped_baton, notify_merge_begin,
                                  pool);
    }

  if (notify_b->wrapped_func)
    (*notify_b->wrapped_func)(notify_b->wrapped_baton, notify, pool);
}

/* Set *OUT_RANGELIST to the intersection of IN_RANGELIST with the simple
 * (inheritable) revision range REV1:REV2, according to CONSIDER_INHERITANCE.
 * If REV1 is equal to REV2, the result is an empty rangelist, otherwise
 * REV1 must be less than REV2.
 *
 * Note: If CONSIDER_INHERITANCE is FALSE, the effect is to treat any non-
 * inheritable input ranges as if they were inheritable.  If it is TRUE, the
 * effect is to discard any non-inheritable input ranges.  Therefore the
 * ranges in *OUT_RANGELIST will always be inheritable. */
static svn_error_t *
rangelist_intersect_range(apr_array_header_t **out_rangelist,
                          const apr_array_header_t *in_rangelist,
                          svn_revnum_t rev1,
                          svn_revnum_t rev2,
                          svn_boolean_t consider_inheritance,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool)
{
  SVN_ERR_ASSERT(rev1 <= rev2);

  if (rev1 < rev2)
    {
      apr_array_header_t *simple_rangelist =
        svn_rangelist__initialize(rev1, rev2, TRUE, scratch_pool);

      SVN_ERR(svn_rangelist_intersect(out_rangelist,
                                      simple_rangelist, in_rangelist,
                                      consider_inheritance, result_pool));
    }
  else
    {
      *out_rangelist = apr_array_make(result_pool, 0,
                                      sizeof(svn_merge_range_t *));
    }
  return SVN_NO_ERROR;
}

/* Helper for fix_deleted_subtree_ranges().  Like fix_deleted_subtree_ranges()
   this function should only be called when honoring mergeinfo.

   CHILD, PARENT, REVISION1, REVISION2, and RA_SESSION are all cascaded from
   fix_deleted_subtree_ranges() -- see that function for more information on
   each.

   If PARENT is not the merge target then PARENT must have already have been
   processed by this function as a child.  Specifically, this means that
   PARENT->REMAINING_RANGES must already be populated -- it can be an empty
   rangelist but cannot be NULL.

   PRIMARY_URL is the merge source url of CHILD at the younger of REVISION1
   and REVISION2.  MERGEINFO_PATH is the absolute repository path of
   PRIMARY_URL (i.e. the path value of mergeinfo from PRIMARY_URL).

   Since this function is only invoked for subtrees of the merge target, the
   guarantees afforded by normalize_merge_sources() don't apply - see the
   'MERGEINFO MERGE SOURCE NORMALIZATION' comment at the top of this file.
   Therefore it is possible that PRIMARY_URL@REVISION1 and
   PRIMARY_URL@REVISION2 don't describe the endpoints of an unbroken line of
   history.  The purpose of this helper is to identify these cases of broken
   history and adjust CHILD->REMAINING_RANGES in such a way we don't later try
   to describe nonexistent path/revisions to the merge report editor -- see
   drive_merge_report_editor().

   If PRIMARY_URL@REVISION1 and PRIMARY_URL@REVISION2 describe an unbroken
   line of history then do nothing and leave CHILD->REMAINING_RANGES as-is.

   If neither PRIMARY_URL@REVISION1 nor PRIMARY_URL@REVISION2 exist then
   there is nothing to merge to CHILD->ABSPATH so set CHILD->REMAINING_RANGES
   equal to PARENT->REMAINING_RANGES.  This will cause the subtree to
   effectively ignore CHILD -- see 'Note: If the first svn_merge_range_t...'
   in drive_merge_report_editor()'s doc string.

   If PRIMARY_URL@REVISION1 *xor* PRIMARY_URL@REVISION2 exist then we take the
   subset of REVISION1:REVISION2 in CHILD->REMAINING_RANGES at which
   PRIMARY_URL doesn't exist and set that subset equal to
   PARENT->REMAINING_RANGES' intersection with that non-existent range.  Why?
   Because this causes CHILD->REMAINING_RANGES to be identical to
   PARENT->REMAINING_RANGES for revisions between REVISION1 and REVISION2 at
   which PRIMARY_URL doesn't exist.  As mentioned above this means that
   drive_merge_report_editor() won't attempt to describe these non-existent
   subtree path/ranges to the reporter (which would break the merge).

   If the preceeding paragraph wasn't terribly clear then what follows spells
   out this function's behavior a bit more explicitly:

   For forward merges (REVISION1 < REVISION2)

     If PRIMARY_URL@REVISION1 exists but PRIMARY_URL@REVISION2 doesn't, then
     find the revision 'N' in which PRIMARY_URL@REVISION1 was deleted.  Leave
     the subset of CHILD->REMAINING_RANGES that intersects with
     REVISION1:(N - 1) as-is and set the subset of CHILD->REMAINING_RANGES
     that intersects with (N - 1):REVISION2 equal to PARENT->REMAINING_RANGES'
     intersection with (N - 1):REVISION2.

     If PRIMARY_URL@REVISION1 doesn't exist but PRIMARY_URL@REVISION2 does,
     then find the revision 'M' in which PRIMARY_URL@REVISION2 came into
     existence.  Leave the subset of CHILD->REMAINING_RANGES that intersects with
     (M - 1):REVISION2 as-is and set the subset of CHILD->REMAINING_RANGES
     that intersects with REVISION1:(M - 1) equal to PARENT->REMAINING_RANGES'
     intersection with REVISION1:(M - 1).

   For reverse merges (REVISION1 > REVISION2)

     If PRIMARY_URL@REVISION1 exists but PRIMARY_URL@REVISION2 doesn't, then
     find the revision 'N' in which PRIMARY_URL@REVISION1 came into existence.
     Leave the subset of CHILD->REMAINING_RANGES that intersects with
     REVISION2:(N - 1) as-is and set the subset of CHILD->REMAINING_RANGES
     that intersects with (N - 1):REVISION1 equal to PARENT->REMAINING_RANGES'
     intersection with (N - 1):REVISION1.

     If PRIMARY_URL@REVISION1 doesn't exist but PRIMARY_URL@REVISION2 does,
     then find the revision 'M' in which PRIMARY_URL@REVISION2 came into
     existence.  Leave the subset of CHILD->REMAINING_RANGES that intersects with
     REVISION2:(M - 1) as-is and set the subset of CHILD->REMAINING_RANGES
     that intersects with (M - 1):REVISION1 equal to PARENT->REMAINING_RANGES'
     intersection with REVISION1:(M - 1).

   SCRATCH_POOL is used for all temporary allocations.  Changes to CHILD are
   allocated in RESULT_POOL. */
static svn_error_t *
adjust_deleted_subtree_ranges(svn_client__merge_path_t *child,
                              svn_client__merge_path_t *parent,
                              const char *mergeinfo_path,
                              svn_revnum_t revision1,
                              svn_revnum_t revision2,
                              const char *primary_url,
                              svn_ra_session_t *ra_session,
                              svn_client_ctx_t *ctx,
                              apr_pool_t *result_pool,
                              apr_pool_t *scratch_pool)
{
  svn_boolean_t is_rollback = revision2 < revision1;
  svn_revnum_t younger_rev = is_rollback ? revision1 : revision2;
  svn_revnum_t peg_rev = younger_rev;
  svn_revnum_t older_rev = is_rollback ? revision2 : revision1;
  apr_array_header_t *segments;
  const char *rel_source_path;
  const char *session_url;
  svn_error_t *err;

  SVN_ERR_ASSERT(parent->remaining_ranges);

  /* We want to know about PRIMARY_URL@peg_rev, but we need PRIMARY_URL's
     path relative to RA_SESSION's URL. */
  SVN_ERR(svn_ra_get_session_url(ra_session, &session_url, scratch_pool));
  SVN_ERR(svn_client__path_relative_to_root(&rel_source_path,
                                            ctx->wc_ctx,
                                            primary_url,
                                            session_url,
                                            FALSE,
                                            ra_session,
                                            scratch_pool,
                                            scratch_pool));
  err = svn_client__repos_location_segments(&segments, ra_session,
                                            rel_source_path, peg_rev,
                                            younger_rev, older_rev, ctx,
                                            scratch_pool);

  /* If PRIMARY_URL@peg_rev doesn't exist then
      svn_client__repos_location_segments() typically returns an
      SVN_ERR_FS_NOT_FOUND error, but if it doesn't exist for a
      forward merge over ra_neon then we get SVN_ERR_RA_DAV_REQUEST_FAILED.
      http://subversion.tigris.org/issues/show_bug.cgi?id=3137 fixed some of
      the cases where different RA layers returned different error codes to
      signal the "path not found"...but it looks like there is more to do. */
  if (err)
    {
      if (err->apr_err == SVN_ERR_FS_NOT_FOUND
          || err->apr_err == SVN_ERR_RA_DAV_REQUEST_FAILED)
        {
          /* PRIMARY_URL@peg_rev doesn't exist.  Check if PRIMARY_URL@older_rev
             exists, if neither exist then the editor can simply ignore this
             subtree. */
          svn_node_kind_t kind;

          svn_error_clear(err);
          err = NULL;
          SVN_ERR(svn_ra_check_path(ra_session, rel_source_path,
                                    older_rev, &kind, scratch_pool));
          if (kind == svn_node_none)
            {
              /* Neither PRIMARY_URL@peg_rev nor PRIMARY_URL@older_rev exist,
                 so there is nothing to merge.  Set CHILD->REMAINING_RANGES
                 identical to PARENT's. */
              child->remaining_ranges =
                svn_rangelist_dup(parent->remaining_ranges, scratch_pool);
            }
          else
            {
              apr_array_header_t *deleted_rangelist;
              svn_revnum_t rev_primary_url_deleted;

              /* PRIMARY_URL@older_rev exists, so it was deleted at some
                 revision prior to peg_rev, find that revision. */
              SVN_ERR(svn_ra_get_deleted_rev(ra_session, rel_source_path,
                                             older_rev, younger_rev,
                                             &rev_primary_url_deleted,
                                             scratch_pool));

              /* PRIMARY_URL@older_rev exists and PRIMARY_URL@peg_rev doesn't,
                 so svn_ra_get_deleted_rev() should always find the revision
                 PRIMARY_URL@older_rev was deleted. */
              SVN_ERR_ASSERT(SVN_IS_VALID_REVNUM(rev_primary_url_deleted));

              /* If this is a reverse merge reorder CHILD->REMAINING_RANGES and
                 PARENT->REMAINING_RANGES so both will work with the
                 svn_rangelist_* APIs below. */
              if (is_rollback)
                {
                  /* svn_rangelist_reverse operates in place so it's safe
                     to use our scratch_pool. */
                  SVN_ERR(svn_rangelist_reverse(child->remaining_ranges,
                                                scratch_pool));
                  SVN_ERR(svn_rangelist_reverse(parent->remaining_ranges,
                                                scratch_pool));
                }

              /* Find the intersection of CHILD->REMAINING_RANGES with the
                 range over which PRIMARY_URL@older_rev exists (ending at
                 the youngest revision at which it still exists). */
              SVN_ERR(rangelist_intersect_range(&child->remaining_ranges,
                                                child->remaining_ranges,
                                                older_rev,
                                                rev_primary_url_deleted - 1,
                                                FALSE,
                                                scratch_pool, scratch_pool));

              /* Merge into CHILD->REMANING_RANGES the intersection of
                 PARENT->REMAINING_RANGES with the range beginning when
                 PRIMARY_URL@older_rev was deleted until younger_rev. */
              SVN_ERR(rangelist_intersect_range(&deleted_rangelist,
                                                parent->remaining_ranges,
                                                rev_primary_url_deleted - 1,
                                                peg_rev,
                                                FALSE,
                                                scratch_pool, scratch_pool));
              SVN_ERR(svn_rangelist_merge(&(child->remaining_ranges),
                                          deleted_rangelist, scratch_pool));

              /* Return CHILD->REMAINING_RANGES and PARENT->REMAINING_RANGES
                 to reverse order if necessary. */
              if (is_rollback)
                {
                  SVN_ERR(svn_rangelist_reverse(child->remaining_ranges,
                                                scratch_pool));
                  SVN_ERR(svn_rangelist_reverse(parent->remaining_ranges,
                                                scratch_pool));
                }
            }
        }
      else
        {
          return svn_error_trace(err);
        }
    }
  else /* PRIMARY_URL@peg_rev exists. */
    {
      apr_array_header_t *non_existent_rangelist;
      svn_location_segment_t *segment =
        APR_ARRAY_IDX(segments, (segments->nelts - 1),
                      svn_location_segment_t *);

      /* We know PRIMARY_URL@peg_rev exists as the call to
         svn_client__repos_location_segments() succeeded.  If there is only
         one segment that starts at oldest_rev then we know that
         PRIMARY_URL@oldest_rev:PRIMARY_URL@peg_rev describes an unbroken
         line of history, so there is nothing more to adjust in
         CHILD->REMAINING_RANGES. */
      if (segment->range_start == older_rev)
        {
          return SVN_NO_ERROR;
        }

      /* If this is a reverse merge reorder CHILD->REMAINING_RANGES and
         PARENT->REMAINING_RANGES so both will work with the
         svn_rangelist_* APIs below. */
      if (is_rollback)
        {
          SVN_ERR(svn_rangelist_reverse(child->remaining_ranges,
                                        scratch_pool));
          SVN_ERR(svn_rangelist_reverse(parent->remaining_ranges,
                                        scratch_pool));
        }

      /* Intersect CHILD->REMAINING_RANGES with the range where PRIMARY_URL
         exists.  Since segment doesn't span older_rev:peg_rev we know
         PRIMARY_URL@peg_rev didn't come into existence until
         segment->range_start + 1. */
      SVN_ERR(rangelist_intersect_range(&child->remaining_ranges,
                                        child->remaining_ranges,
                                        segment->range_start, peg_rev,
                                        FALSE, scratch_pool, scratch_pool));

      /* Merge into CHILD->REMANING_RANGES the intersection of
         PARENT->REMAINING_RANGES with the range before PRIMARY_URL@peg_rev
         came into existence. */
      SVN_ERR(rangelist_intersect_range(&non_existent_rangelist,
                                        parent->remaining_ranges,
                                        older_rev, segment->range_start,
                                        FALSE, scratch_pool, scratch_pool));
      SVN_ERR(svn_rangelist_merge(&(child->remaining_ranges),
                                  non_existent_rangelist, scratch_pool));

      /* Return CHILD->REMAINING_RANGES and PARENT->REMAINING_RANGES
         to reverse order if necessary. */
      if (is_rollback)
        {
          SVN_ERR(svn_rangelist_reverse(child->remaining_ranges,
                                        scratch_pool));
          SVN_ERR(svn_rangelist_reverse(parent->remaining_ranges,
                                        scratch_pool));
        }
    }

  /* Make a lasting copy of CHILD->REMAINING_RANGES using POOL. */
  child->remaining_ranges = svn_rangelist_dup(child->remaining_ranges,
                                              result_pool);
  return SVN_NO_ERROR;
}

/* Helper for do_directory_merge().

   URL1, REVISION1, URL2, REVISION2, NOTIFY_B, and MERGE_B are
   cascaded from the arguments of the same name in do_directory_merge().
   RA_SESSION is the session for the younger of URL1@REVISION1 and
   URL2@REVISION2.

   Adjust the subtrees in NOTIFY_B->CHILDREN_WITH_MERGEINFO so that we don't
   later try to describe invalid paths in drive_merge_report_editor().
   This function is just a thin wrapper around
   adjust_deleted_subtree_ranges(), which see for further details.

   SCRATCH_POOL is used for all temporary allocations.  Changes to
   NOTIFY_B->CHILDREN_WITH_MERGEINFO are allocated in RESULT_POOL.
*/
static svn_error_t *
fix_deleted_subtree_ranges(const char *url1,
                           svn_revnum_t revision1,
                           const char *url2,
                           svn_revnum_t revision2,
                           svn_ra_session_t *ra_session,
                           notification_receiver_baton_t *notify_b,
                           merge_cmd_baton_t *merge_b,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  int i;
  const char *source_root_url;
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  svn_boolean_t is_rollback = revision2 < revision1;

  SVN_ERR(svn_ra_get_repos_root2(ra_session, &source_root_url, scratch_pool));

  /* NOTIFY_B->CHILDREN_WITH_MERGEINFO is sorted in depth-first order, so
     start at index 1 to examine only subtrees. */
  for (i = 1; i < notify_b->children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
        APR_ARRAY_IDX(notify_b->children_with_mergeinfo, i,
                      svn_client__merge_path_t *);
      svn_client__merge_path_t *parent;
      apr_array_header_t *deleted_rangelist, *added_rangelist;
      int parent_index;

      SVN_ERR_ASSERT(child);
      if (child->absent)
        continue;

      svn_pool_clear(iterpool);

      /* Find CHILD's parent. */
      parent_index = find_nearest_ancestor(notify_b->children_with_mergeinfo,
                                           FALSE, child->abspath);
      parent = APR_ARRAY_IDX(notify_b->children_with_mergeinfo, parent_index,
                             svn_client__merge_path_t *);

      /* Since CHILD is a subtree then its parent must be in
         CHILDREN_WITH_MERGEINFO, see the global comment
         'THE CHILDREN_WITH_MERGEINFO ARRAY'. */
      SVN_ERR_ASSERT(parent);

      /* If this is a reverse merge reorder CHILD->REMAINING_RANGES
         so it will work with the svn_rangelist_diff API. */
      if (is_rollback)
        {
          SVN_ERR(svn_rangelist_reverse(child->remaining_ranges, iterpool));
          SVN_ERR(svn_rangelist_reverse(parent->remaining_ranges, iterpool));
        }

      SVN_ERR(svn_rangelist_diff(&deleted_rangelist, &added_rangelist,
                                 child->remaining_ranges,
                                 parent->remaining_ranges,
                                 TRUE, iterpool));

      if (is_rollback)
        {
          SVN_ERR(svn_rangelist_reverse(child->remaining_ranges, iterpool));
          SVN_ERR(svn_rangelist_reverse(parent->remaining_ranges, iterpool));
        }

      /* If CHILD is the merge target we then know that URL1, URL2,
         REVISION1, and REVISION2 are provided by normalize_merge_sources()
         -- see 'MERGEINFO MERGE SOURCE NORMALIZATION'.  Due to this
         normalization we know that URL1@REVISION1 and URL2@REVISION2
         describe an unbroken line of history such that the entire range
         described by REVISION1:REVISION2 can potentially be merged to CHILD.

         But if CHILD is a subtree we don't have the same guarantees about
         URL1, URL2, REVISION1, and REVISION2 as we do for the merge target.
         URL1@REVSION1 and/or URL2@REVSION2 might not exist.

         If one or both doesn't exist, then adjust CHILD->REMAINING_RANGES
         such that we don't later try to describe invalid subtrees in
         drive_merge_report_editor(), as that will break the merge.
         If CHILD has the same remaining ranges as PARENT however, then
         there is no need to make these adjustments, since
         drive_merge_report_editor() won't attempt to describe CHILD in this
         case, see the 'Note' in drive_merge_report_editor's docstring. */
      if (deleted_rangelist->nelts || added_rangelist->nelts)
        {
          const char *child_mergeinfo_path;
          const char *child_primary_source_url;
          const char *child_repos_src_path =
            svn_dirent_is_child(merge_b->target_abspath, child->abspath,
                                iterpool);

          /* This loop is only processing subtrees, so CHILD->ABSPATH
             better be a proper child of the merge target. */
          SVN_ERR_ASSERT(child_repos_src_path);

          child_primary_source_url =
            svn_path_url_add_component2((revision1 < revision2) ? url2 : url1,
                                        child_repos_src_path, iterpool);

          SVN_ERR(svn_client__path_relative_to_root(&child_mergeinfo_path,
                                                    merge_b->ctx->wc_ctx,
                                                    child_primary_source_url,
                                                    source_root_url,
                                                    TRUE, ra_session,
                                                    iterpool, iterpool));
          SVN_ERR(adjust_deleted_subtree_ranges(child, parent,
                                                child_mergeinfo_path,
                                                revision1, revision2,
                                                child_primary_source_url,
                                                ra_session,
                                                merge_b->ctx, result_pool,
                                                iterpool));
        }
    }

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

/*-----------------------------------------------------------------------*/

/*** Determining What Remains To Be Merged ***/

/* Get explicit and/or implicit mergeinfo for the working copy path
   TARGET_ABSPATH.

   If RECORDED_MERGEINFO is not NULL then set *RECORDED_MERGEINFO
   to TARGET_ABSPATH's explicit or inherited mergeinfo as dictated by
   INHERIT.

   If IMPLICIT_MERGEINFO is not NULL then set *IMPLICIT_MERGEINFO
   to TARGET_ABSPATH's implicit mergeinfo (a.k.a. natural history).

   If both RECORDED_MERGEINFO and IMPLICIT_MERGEINFO are not NULL and
   *RECORDED_MERGEINFO is inherited, then *IMPLICIT_MERGEINFO will be
   removed from *RECORDED_MERGEINFO.

   If INHERITED is not NULL set *INHERITED to TRUE if *RECORDED_MERGEINFO
   is inherited rather than explicit.  If RECORDED_MERGEINFO is NULL then
   INHERITED is ignored.


   If IMPLICIT_MERGEINFO is not NULL then START and END are limits on the
   the natural history sought, must both be valid revision numbers, and
   START must be greater than END.  If TARGET_ABSPATH's base revision
   is older than START, then the base revision is used as the younger
   bound in place of START.

   RA_SESSION is an open session that may be temporarily reparented as
   needed by this function.

   Allocate *RECORDED_MERGEINFO and *IMPLICIT_MERGEINFO in RESULT_POOL.
   Use SCRATCH_POOL for any temporary allocations. */
static svn_error_t *
get_full_mergeinfo(svn_mergeinfo_t *recorded_mergeinfo,
                   svn_mergeinfo_t *implicit_mergeinfo,
                   svn_boolean_t *inherited,
                   svn_mergeinfo_inheritance_t inherit,
                   svn_ra_session_t *ra_session,
                   const char *target_abspath,
                   svn_revnum_t start,
                   svn_revnum_t end,
                   svn_client_ctx_t *ctx,
                   apr_pool_t *result_pool,
                   apr_pool_t *scratch_pool)
{
  svn_boolean_t inherited_mergeinfo = FALSE;

  /* First, we get the real mergeinfo. */
  if (recorded_mergeinfo)
    {
      svn_boolean_t inherited_from_repos;

      SVN_ERR(svn_client__get_wc_or_repos_mergeinfo(recorded_mergeinfo,
                                                    &inherited_mergeinfo,
                                                    &inherited_from_repos,
                                                    FALSE,
                                                    inherit, ra_session,
                                                    target_abspath,
                                                    ctx, result_pool));
      if (inherited)
        *inherited = inherited_mergeinfo;
    }

  if (implicit_mergeinfo)
    {
      const char *repos_root;
      const char *repos_relpath;
      const char *session_url = NULL;
      svn_revnum_t target_rev;

      /* Assert that we have sane input. */
      SVN_ERR_ASSERT(SVN_IS_VALID_REVNUM(start)
                 && SVN_IS_VALID_REVNUM(end)
                 && (start > end));

      /* Retrieve the origin (original_*) of the node, or just the
         url if the node was not copied. */
      SVN_ERR(svn_wc__node_get_origin(FALSE, &target_rev, &repos_relpath,
                                      &repos_root, NULL, NULL,
                                      ctx->wc_ctx, target_abspath, FALSE,
                                      scratch_pool, scratch_pool));

      if (! repos_relpath)
        {
          /* We've been asked to operate on a target which has no location
           * in the repository. Either it's unversioned (but attempts to
           * merge into unversioned targets should not get as far as here),
           * or it is locally added, in which case the target's implicit
           * mergeinfo is empty. */
          *implicit_mergeinfo = apr_hash_make(result_pool);
        }
      else if (target_rev <= end)
        {
          /* We're asking about a range outside our natural history
             altogether.  That means our implicit mergeinfo is empty. */
          *implicit_mergeinfo = apr_hash_make(result_pool);
        }
      else
        {
          svn_opt_revision_t peg_revision;
          const char *url;

          url = svn_path_url_add_component2(repos_root, repos_relpath,
                                            scratch_pool);

          /* Temporarily point our RA_SESSION at our target URL so we can
             fetch so-called "implicit mergeinfo" (that is, natural
             history). */
          SVN_ERR(svn_client__ensure_ra_session_url(&session_url,
                                                    ra_session, url,
                                                    scratch_pool));

          /* Do not ask for implicit mergeinfo from TARGET_ABSPATH's future.
             TARGET_ABSPATH might not even exist, and even if it does the
             working copy is *at* TARGET_REV so its implicit history ends
             at TARGET_REV! */
          if (target_rev < start)
            start = target_rev;

          /* Fetch the implicit mergeinfo. */
          peg_revision.kind = svn_opt_revision_number;
          peg_revision.value.number = target_rev;
          SVN_ERR(svn_client__get_history_as_mergeinfo(implicit_mergeinfo,
                                                       NULL,
                                                       url, &peg_revision,
                                                       start, end,
                                                       ra_session, ctx,
                                                       result_pool));

          /* Return RA_SESSION back to where it was when we were called. */
         SVN_ERR(svn_client__ensure_ra_session_url(&session_url,
                                                   ra_session, session_url,
                                                   scratch_pool));
        }
    } /*if (implicit_mergeinfo) */

  return SVN_NO_ERROR;
}

/* Helper for ensure_implicit_mergeinfo().

   PARENT, CHILD, REVISION1, REVISION2, RA_SESSION, and CTX
   are all cascaded from the arguments of the same names in
   ensure_implicit_mergeinfo().  PARENT and CHILD must both exist, i.e.
   this function should never be called where CHILD is the merge target.

   If PARENT->IMPLICIT_MERGEINFO is NULL, obtain it from the server.

   Set CHILD->IMPLICIT_MERGEINFO to the mergeinfo inherited from
   PARENT->IMPLICIT_MERGEINFO.  CHILD->IMPLICIT_MERGEINFO is allocated
   in POOL.
   */
static svn_error_t *
inherit_implicit_mergeinfo_from_parent(svn_client__merge_path_t *parent,
                                       svn_client__merge_path_t *child,
                                       svn_revnum_t revision1,
                                       svn_revnum_t revision2,
                                       svn_ra_session_t *ra_session,
                                       svn_client_ctx_t *ctx,
                                       apr_pool_t *result_pool,
                                       apr_pool_t *scratch_pool)
{
  const char *path_diff;

  /* This only works on subtrees! */
  SVN_ERR_ASSERT(parent);
  SVN_ERR_ASSERT(child);

  /* While PARENT must exist, it is possible we've deferred
     getting its implicit mergeinfo.  If so get it now. */
  if (!parent->implicit_mergeinfo)
    SVN_ERR(get_full_mergeinfo(NULL, &(parent->implicit_mergeinfo),
                               NULL, svn_mergeinfo_inherited,
                               ra_session, child->abspath,
                               MAX(revision1, revision2),
                               MIN(revision1, revision2),
                               ctx, result_pool, scratch_pool));

  /* Let CHILD inherit PARENT's implicit mergeinfo. */
  child->implicit_mergeinfo = apr_hash_make(result_pool);

  path_diff = svn_dirent_is_child(parent->abspath, child->abspath,
                                  scratch_pool);

  /* PARENT->PATH better be an ancestor of CHILD->ABSPATH! */
  SVN_ERR_ASSERT(path_diff);

  SVN_ERR(svn_client__adjust_mergeinfo_source_paths(
    child->implicit_mergeinfo, path_diff,
    parent->implicit_mergeinfo, result_pool));
  return SVN_NO_ERROR;
}

/* Helper of filter_merged_revisions().

   If we have deferred obtaining CHILD->IMPLICIT_MERGEINFO, then get
   it now, allocating it in RESULT_POOL.  If CHILD_INHERITS_PARENT is true
   then set CHILD->IMPLICIT_MERGEINFO to the mergeinfo inherited from
   PARNET->IMPLICIT_MERGEINFO, otherwise contact the repository.  Use
   SCRATCH_POOL for all temporary allocations.

   PARENT, CHILD, REVISION1, REVISION2, RA_SESSION, and
   CTX are all cascased from the arguments of the same name in
   filter_merged_revisions() and the same conditions for that function
   hold here. */
static svn_error_t *
ensure_implicit_mergeinfo(svn_client__merge_path_t *parent,
                          svn_client__merge_path_t *child,
                          svn_boolean_t child_inherits_parent,
                          svn_revnum_t revision1,
                          svn_revnum_t revision2,
                          svn_ra_session_t *ra_session,
                          svn_client_ctx_t *ctx,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool)
{
  /* If we haven't already found CHILD->IMPLICIT_MERGEINFO then
     contact the server to get it. */

  if (child->implicit_mergeinfo)
    return SVN_NO_ERROR;

  if (child_inherits_parent)
    SVN_ERR(inherit_implicit_mergeinfo_from_parent(parent,
                                                   child,
                                                   revision1,
                                                   revision2,
                                                   ra_session,
                                                   ctx,
                                                   result_pool,
                                                   scratch_pool));
  else
    SVN_ERR(get_full_mergeinfo(NULL,
                               &(child->implicit_mergeinfo),
                               NULL, svn_mergeinfo_inherited,
                               ra_session, child->abspath,
                               MAX(revision1, revision2),
                               MIN(revision1, revision2),
                               ctx, result_pool, scratch_pool));

  return SVN_NO_ERROR;
}

/* Helper for calculate_remaining_ranges().

   Initialize CHILD->REMAINING_RANGES to a rangelist representing the
   requested merge of REVISION1:REVISION2 from MERGEINFO_PATH to
   CHILD->ABSPATH.

   For forward merges remove any ranges from CHILD->REMAINING_RANGES that
   have already been merged to CHILD->ABSPATH per TARGET_MERGEINFO or
   CHILD->IMPLICIT_MERGEINFO.  For reverse merges remove any ranges from
   CHILD->REMAINING_RANGES that have not already been merged to CHILD->ABSPATH
   per TARGET_MERGEINFO or CHILD->IMPLICIT_MERGEINFO.  If we have deferred
   obtaining CHILD->IMPLICIT_MERGEINFO and it is necessary to use it for
   these calculations, then get it from the server, allocating it in
   RESULT_POOL.

   CHILD represents a working copy path which is the merge target or one of
   the target's subtrees.  If not NULL, PARENT is CHILD's nearest path-wise
   ancestor - see 'THE CHILDREN_WITH_MERGEINFO ARRAY'.

   If the function needs to consider CHILD->IMPLICIT_MERGEINFO and
   CHILD_INHERITS_IMPLICIT is true, then set CHILD->IMPLICIT_MERGEINFO to the
   mergeinfo inherited from PARENT->IMPLICIT_MERGEINFO.  Otherwise contact
   the repository for CHILD->IMPLICIT_MERGEINFO.

   NOTE: If PARENT is present then this function must have previously been
   called for PARENT, i.e. if populate_remaining_ranges() is calling this
   function for a set of svn_client__merge_path_t* the calls must be made
   in depth-first order.

   MERGEINFO_PATH is the merge source relative to the repository root.

   REVISION1 and REVISION2 describe the merge range requested from
   MERGEINFO_PATH.

   TARGET_MERGEINFO is the CHILD->ABSPATH's explicit or inherited mergeinfo.
   TARGET_MERGEINFO should be NULL if there is no explicit or inherited
   mergeinfo on CHILD->ABSPATH or an empty hash if CHILD->ABSPATH has empty
   mergeinfo.

   SCRATCH_POOL is used for all temporary allocations.

   NOTE: This should only be called when honoring mergeinfo.

   NOTE: Like calculate_remaining_ranges() if PARENT is present then this
   function must have previously been called for PARENT.
*/
static svn_error_t *
filter_merged_revisions(svn_client__merge_path_t *parent,
                        svn_client__merge_path_t *child,
                        const char *mergeinfo_path,
                        svn_mergeinfo_t target_mergeinfo,
                        svn_revnum_t revision1,
                        svn_revnum_t revision2,
                        svn_boolean_t child_inherits_implicit,
                        svn_ra_session_t *ra_session,
                        svn_client_ctx_t *ctx,
                        apr_pool_t *result_pool,
                        apr_pool_t *scratch_pool)
{
  apr_array_header_t *requested_rangelist, *target_rangelist,
    *target_implicit_rangelist, *explicit_rangelist;

  /* Convert REVISION1 and REVISION2 to a rangelist.

     Note: Talking about a requested merge range's inheritability
     doesn't make much sense, but as we are using svn_merge_range_t
     to describe it we need to pick *something*.  Since all the
     rangelist manipulations in this function either don't consider
     inheritance by default or we are requesting that they don't (i.e.
     svn_rangelist_remove and svn_rangelist_intersect) then we could
     set the inheritability as FALSE, it won't matter either way. */
  requested_rangelist = svn_rangelist__initialize(revision1, revision2,
                                                  TRUE, scratch_pool);

  /* Now filter out revisions that have already been merged to CHILD. */

  if (revision1 > revision2) /* This is a reverse merge. */
    {
      apr_array_header_t *added_rangelist, *deleted_rangelist;

      /* The revert range and will need to be reversed for
         our svn_rangelist_* APIs to work properly. */
      SVN_ERR(svn_rangelist_reverse(requested_rangelist, scratch_pool));

      if (target_mergeinfo)
        target_rangelist = apr_hash_get(target_mergeinfo,
                                        mergeinfo_path, APR_HASH_KEY_STRING);
      else
        target_rangelist = NULL;

      if (target_rangelist)
        {
          /* Return the intersection of the revs which are both already
             represented by CHILD's explicit or inherited mergeinfo.

             We don't consider inheritance when determining intersecting
             ranges.  If we *did* consider inheritance, then our calculation
             would be wrong.  For example, if the CHILD->REMAINING_RANGES is
             5:3 and TARGET_RANGELIST is r5* (non-inheritable) then the
             intersection would be r4.  And that would be wrong as we clearly
             want to reverse merge both r4 and r5 in this case.  Ignoring the
             ranges' inheritance results in an intersection of r4-5.

             You might be wondering about CHILD's children, doesn't the above
             imply that we will reverse merge r4-5 from them?  Nope, this is
             safe to do because any path whose parent has non-inheritable
             ranges is always considered a subtree with differing mergeinfo
             even if that path has no explicit mergeinfo prior to the
             merge -- See condition 3 in the doc string for
             merge.c:get_mergeinfo_paths(). */
          SVN_ERR(svn_rangelist_intersect(&explicit_rangelist,
                                          target_rangelist,
                                          requested_rangelist,
                                          FALSE, scratch_pool));
        }
      else
        {
          explicit_rangelist =
            apr_array_make(result_pool, 0, sizeof(svn_merge_range_t *));
        }

      /* Was any part of the requested reverse merge not accounted for in
         CHILD's explicit or inherited mergeinfo? */
      SVN_ERR(svn_rangelist_diff(&deleted_rangelist, &added_rangelist,
                                 requested_rangelist, explicit_rangelist,
                                 FALSE, scratch_pool));

      if (deleted_rangelist->nelts == 0)
        {
          /* The whole of REVISION1:REVISION2 was represented in CHILD's
             explicit/inherited mergeinfo, allocate CHILD's remaining
             ranges in POOL and then we are done. */
          SVN_ERR(svn_rangelist_reverse(requested_rangelist, scratch_pool));
          child->remaining_ranges = svn_rangelist_dup(requested_rangelist,
                                                      result_pool);
        }
      else /* We need to check CHILD's implicit mergeinfo. */
        {
          apr_array_header_t *implicit_rangelist;

          SVN_ERR(ensure_implicit_mergeinfo(parent,
                                            child,
                                            child_inherits_implicit,
                                            revision1,
                                            revision2,
                                            ra_session,
                                            ctx,
                                            result_pool,
                                            scratch_pool));

          target_implicit_rangelist = apr_hash_get(child->implicit_mergeinfo,
                                                   mergeinfo_path,
                                                   APR_HASH_KEY_STRING);

          if (target_implicit_rangelist)
            SVN_ERR(svn_rangelist_intersect(&implicit_rangelist,
                                            target_implicit_rangelist,
                                            requested_rangelist,
                                            FALSE, scratch_pool));
          else
            implicit_rangelist = apr_array_make(scratch_pool, 0,
                                                sizeof(svn_merge_range_t *));

          SVN_ERR(svn_rangelist_merge(&implicit_rangelist,
                                      explicit_rangelist, scratch_pool));
          SVN_ERR(svn_rangelist_reverse(implicit_rangelist, scratch_pool));
          child->remaining_ranges = svn_rangelist_dup(implicit_rangelist,
                                                      result_pool);
        }
    }
  else /* This is a forward merge */
    {
      if (target_mergeinfo)
        target_rangelist = apr_hash_get(target_mergeinfo, mergeinfo_path,
                                        APR_HASH_KEY_STRING);
      else
        target_rangelist = NULL;

      /* See earlier comment preceeding svn_rangelist_intersect() for
         why we don't consider inheritance here. */
      if (target_rangelist)
        {
          SVN_ERR(svn_rangelist_remove(&explicit_rangelist,
                                       target_rangelist,
                                       requested_rangelist, FALSE,
                                       scratch_pool));
        }
      else
        {
          explicit_rangelist = svn_rangelist_dup(requested_rangelist,
                                                 scratch_pool);
        }

      if (explicit_rangelist->nelts == 0)
        {
          child->remaining_ranges =
            apr_array_make(result_pool, 0, sizeof(svn_merge_range_t *));
        }
      else
/* ### TODO:  Which evil shall we choose?
   ###
   ### If we allow all forward-merges not already found in recorded
   ### mergeinfo, we destroy the ability to, say, merge the whole of a
   ### branch to the trunk while automatically ignoring the revisions
   ### common to both.  That's bad.
   ###
   ### If we allow only forward-merges not found in either recorded
   ### mergeinfo or implicit mergeinfo (natural history), then the
   ### previous scenario works great, but we can't reverse-merge a
   ### previous change made to our line of history and then remake it
   ### (because the reverse-merge will leave no mergeinfo trace, and
   ### the remake-it attempt will still find the original change in
   ### natural mergeinfo.  But you know, that we happen to use 'merge'
   ### for revision undoing is somewhat unnatural anyway, so I'm
   ### finding myself having little interest in caring too much about
   ### this.  That said, if we had a way of storing reverse merge
   ### ranges, we'd be in good shape either way.
*/
#ifdef SVN_MERGE__ALLOW_ALL_FORWARD_MERGES_FROM_SELF
        {
          /* ### Don't consider implicit mergeinfo. */
          child->remaining_ranges = svn_rangelist_dup(explicit_rangelist,
                                                      pool);
        }
#else
        {
          /* Based on CHILD's TARGET_MERGEINFO there are ranges to merge.
             Check CHILD's implicit mergeinfo to see if these remaining
             ranges are represented there. */
          SVN_ERR(ensure_implicit_mergeinfo(parent,
                                            child,
                                            child_inherits_implicit,
                                            revision1,
                                            revision2,
                                            ra_session,
                                            ctx,
                                            result_pool,
                                            scratch_pool));

          target_implicit_rangelist = apr_hash_get(child->implicit_mergeinfo,
                                                   mergeinfo_path,
                                                   APR_HASH_KEY_STRING);
          if (target_implicit_rangelist)
            SVN_ERR(svn_rangelist_remove(&(child->remaining_ranges),
                                         target_implicit_rangelist,
                                         explicit_rangelist,
                                         FALSE, result_pool));
          else
            child->remaining_ranges = svn_rangelist_dup(explicit_rangelist,
                                                        result_pool);
        }
#endif
    }

  return SVN_NO_ERROR;
}

/* Helper for do_file_merge and do_directory_merge (by way of
   populate_remaining_ranges() for the latter).

   Determine what portions of URL1@REVISION1 -> URL2@REVISION2 have already
   been merged to CHILD->ABSPATH and populate CHILD->REMAINING_RANGES with
   the ranges that still need merging.

   URL1, REVISION1, URL2, REVISION2, and CTX are all cascaded
   from the caller's arguments of the same names.  Note that this means URL1,
   REVISION1, URL2, and REVISION2 adhere to the requirements noted in
   `MERGEINFO MERGE SOURCE NORMALIZATION'.

   CHILD represents a working copy path which is the merge target or one of
   the target's subtrees.  If not NULL, PARENT is CHILD's nearest path-wise
   ancestor - see 'THE CHILDREN_WITH_MERGEINFO ARRAY'.  TARGET_MERGEINFO is
   the working mergeinfo on CHILD.

   RA_SESSION is the session for, and SOURCE_ROOT_URL is the repository root
   for, the younger of URL1@REVISION1 and URL2@REVISION2.

   If the function needs to consider CHILD->IMPLICIT_MERGEINFO and
   CHILD_INHERITS_IMPLICIT is true, then set CHILD->IMPLICIT_MERGEINFO to the
   mergeinfo inherited from PARENT->IMPLICIT_MERGEINFO.  Otherwise contact
   the repository for CHILD->IMPLICIT_MERGEINFO.

   If not null, IMPLICIT_SRC_GAP is the gap, if any, in the natural history
   of URL1@REVISION1:URL2@REVISION2, see merge_cmd_baton_t.implicit_src_gap.

   SCRATCH_POOL is used for all temporary allocations.  Changes to CHILD and
   PARENT are made in RESULT_POOL.

   NOTE: This should only be called when honoring mergeinfo.

   NOTE: If PARENT is present then this function must have previously been
   called for PARENT, i.e. if populate_remaining_ranges() is calling this
   function for a set of svn_client__merge_path_t* the calls must be made
   in depth-first order.

   NOTE: When performing reverse merges, return
   SVN_ERR_CLIENT_NOT_READY_TO_MERGE if URL1@REVISION1, URL2@REVISION2, and
   CHILD->ABSPATH are all on the same line of history but CHILD->ABSPATH's
   base revision is older than the REVISION1-REVISION2 range, see comment re
   issue #2973 below.
*/
static svn_error_t *
calculate_remaining_ranges(svn_client__merge_path_t *parent,
                           svn_client__merge_path_t *child,
                           const char *source_root_url,
                           const char *url1,
                           svn_revnum_t revision1,
                           const char *url2,
                           svn_revnum_t revision2,
                           svn_mergeinfo_t target_mergeinfo,
                           const apr_array_header_t *implicit_src_gap,
                           svn_boolean_t child_inherits_implicit,
                           svn_ra_session_t *ra_session,
                           svn_client_ctx_t *ctx,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  const char *mergeinfo_path;
  const char *primary_url = (revision1 < revision2) ? url2 : url1;
  svn_mergeinfo_t adjusted_target_mergeinfo = NULL;
  svn_revnum_t child_base_revision;

  /* Determine which of the requested ranges to consider merging... */
  SVN_ERR(svn_client__path_relative_to_root(&mergeinfo_path, ctx->wc_ctx,
                                            primary_url, source_root_url, TRUE,
                                            ra_session, result_pool,
                                            scratch_pool));

  /* Consider: CHILD might have explicit mergeinfo '/MERGEINFO_PATH:M-N'
     where M-N fall into the gap in URL1@REVISION1:URL2@REVISION2's natural
     history allowed by 'MERGEINFO MERGE SOURCE NORMALIZATION'.  If this is
     the case, then '/MERGEINFO_PATH:N' actually refers to a completely
     different line of history than URL1@REVISION1:URL2@REVISION2 and we
     *don't* want to consider those revisions merged already. */
  if (implicit_src_gap && child->pre_merge_mergeinfo)
    {
      apr_array_header_t *explicit_mergeinfo_gap_ranges =
        apr_hash_get(child->pre_merge_mergeinfo, mergeinfo_path,
                     APR_HASH_KEY_STRING);

      if (explicit_mergeinfo_gap_ranges)
        {
          svn_mergeinfo_t gap_mergeinfo = apr_hash_make(scratch_pool);

          apr_hash_set(gap_mergeinfo, mergeinfo_path, APR_HASH_KEY_STRING,
                       implicit_src_gap);
          SVN_ERR(svn_mergeinfo_remove2(&adjusted_target_mergeinfo,
                                        gap_mergeinfo, target_mergeinfo,
                                        FALSE, result_pool, scratch_pool));
        }
    }
  else
    {
      adjusted_target_mergeinfo = target_mergeinfo;
    }

  /* Initialize CHILD->REMAINING_RANGES and filter out revisions already
     merged (or, in the case of reverse merges, ranges not yet merged). */
  SVN_ERR(filter_merged_revisions(parent, child, mergeinfo_path,
                                  adjusted_target_mergeinfo,
                                  revision1, revision2,
                                  child_inherits_implicit,
                                  ra_session, ctx, result_pool,
                                  scratch_pool));

  /* Issue #2973 -- from the continuing series of "Why, since the advent of
     merge tracking, allowing merges into mixed rev and locally modified
     working copies isn't simple and could be considered downright evil".

     If reverse merging a range to the WC path represented by CHILD, from
     that path's own history, where the path inherits no locally modified
     mergeinfo from its WC parents (i.e. there is no uncommitted merge to
     the WC), and the path's base revision is older than the range, then
     the merge will always be a no-op.  This is because we only allow reverse
     merges of ranges in the path's explicit or natural mergeinfo and a
     reverse merge from the path's future history obviously isn't going to be
     in either, hence the no-op.

     The problem is two-fold.  First, in a mixed rev WC, the change we
     want to revert might actually be to some child of the target path
     which is at a younger base revision.  Sure, we can merge directly
     to that child or update the WC or even use --ignore-ancestry and then
     successfully run the reverse merge, but that gets to the second
     problem: Those courses of action are not very obvious.  Before 1.5 if
     a user committed a change that didn't touch the commit target, then
     immediately decided to revert that change via a reverse merge it would
     just DTRT.  But with the advent of merge tracking the user gets a no-op.

     So in the name of user friendliness, return an error suggesting a helpful
     course of action.
  */
  SVN_ERR(svn_wc__node_get_base_rev(&child_base_revision, ctx->wc_ctx,
                                     child->abspath, scratch_pool));
  /* If CHILD has no base revision then it hasn't been committed yet, so it
     can't have any "future" history. */
  if (SVN_IS_VALID_REVNUM(child_base_revision)
      && ((child->remaining_ranges)->nelts == 0) /* Inoperative merge */
      && (revision2 < revision1)                 /* Reverse merge */
      && (child_base_revision <= revision2))     /* From CHILD's future */
    {
      /* Hmmm, an inoperative reverse merge from the "future".  If it is
         from our own future return a helpful error. */
      svn_error_t *err;
      const char *start_url;
      svn_opt_revision_t requested, unspec, pegrev, *start_revision;
      unspec.kind = svn_opt_revision_unspecified;
      requested.kind = svn_opt_revision_number;
      requested.value.number = child_base_revision;
      pegrev.kind = svn_opt_revision_number;
      pegrev.value.number = revision1;

      err = svn_client__repos_locations(&start_url, &start_revision,
                                        NULL, NULL, ra_session, url1,
                                        &pegrev, &requested,
                                        &unspec, ctx, scratch_pool);
      if (err)
        {
          if (err->apr_err == SVN_ERR_FS_NOT_FOUND
              || err->apr_err == SVN_ERR_CLIENT_UNRELATED_RESOURCES)
            svn_error_clear(err);
          else
            return svn_error_trace(err);
        }
      else
        {
          const char *url;

          SVN_ERR(svn_wc__node_get_url(&url, ctx->wc_ctx, child->abspath,
                                       scratch_pool, scratch_pool));
          if (strcmp(start_url, url) == 0)
            return svn_error_create(SVN_ERR_CLIENT_MERGE_UPDATE_REQUIRED, NULL,
                                    _("Cannot reverse-merge a range from a "
                                      "path's own future history; try "
                                      "updating first"));
        }
    }

  return SVN_NO_ERROR;
}

/* Helper for populate_remaining_ranges().

   URL1, REVISION1, URL2, REVISION2, RA_SESSION, MERGE_SRC_CANON_PATH,
   and MERGE_B are all cascaded from the arguments of the same name in
   populate_remaining_ranges().  MERGE_SRC_CANON_PATH is the absolute
   repository path of URL2.

   Note: The following comments assume a forward merge, i.e.
   REVISION1 < REVISION2.  If this is a reverse merge then all the following
   comments still apply, but with URL1 switched with URL2 and REVISION1
   switched with REVISION2.

   Like populate_remaining_ranges(), URL1@REVISION1:URL2@REVISION2 must adhere
   to the restrictions documented in 'MERGEINFO MERGE SOURCE NORMALIZATION'.
   These restrictions allow for a *single* gap, URL@GAP_REV1:URL2@GAP_REV2,
   (where REVISION1 < GAP_REV1 <= GAP_REV2 < REVISION2) in
   URL1@REVISION1:URL2@REVISION2 if URL2@REVISION2 was copied from
   URL1@REVISION1.  If such a gap exists, set *GAP_START and *GAP_END to the
   starting and ending revisions of the gap.  Otherwise set both to
   SVN_INVALID_REVNUM.

   For example, if the natural history of URL@2:URL@9 is 'trunk/:2,7-9' this
   would indicate that trunk@7 was copied from trunk@2.  This function would
   return GAP_START:GAP_END of 2:6 in this case.  Note that a path 'trunk'
   might exist at r3-6, but it would not be on the same line of history as
   trunk@9. */
static svn_error_t *
find_gaps_in_merge_source_history(svn_revnum_t *gap_start,
                                  svn_revnum_t *gap_end,
                                  const char *merge_src_canon_path,
                                  const char *url1,
                                  svn_revnum_t revision1,
                                  const char *url2,
                                  svn_revnum_t revision2,
                                  svn_ra_session_t *ra_session,
                                  merge_cmd_baton_t *merge_b,
                                  apr_pool_t *scratch_pool)
{
  svn_mergeinfo_t implicit_src_mergeinfo;
  svn_opt_revision_t peg_rev;
  svn_revnum_t young_rev = MAX(revision1, revision2);
  svn_revnum_t old_rev = MIN(revision1, revision2);
  apr_array_header_t *rangelist;
  const char *url = (revision2 < revision1) ? url1 : url2;

  /* Start by assuming there is no gap. */
  *gap_start = *gap_end = SVN_INVALID_REVNUM;

  /* Get URL1@REVISION1:URL2@REVISION2 as mergeinfo. */
  peg_rev.kind = svn_opt_revision_number;
  peg_rev.value.number = young_rev;
  SVN_ERR(svn_client__get_history_as_mergeinfo(&implicit_src_mergeinfo, NULL,
                                               url, &peg_rev, young_rev,
                                               old_rev, ra_session,
                                               merge_b->ctx, scratch_pool));

  rangelist = apr_hash_get(implicit_src_mergeinfo,
                           merge_src_canon_path,
                           APR_HASH_KEY_STRING);

  if (!rangelist) /* ### Can we ever not find a rangelist? */
    return SVN_NO_ERROR;

  /* A gap in natural history can result from either a copy or
     a rename.  If from a copy then history as mergeinfo will look
     something like this:

       '/trunk:X,Y-Z'

     If from a rename it will look like this:

       '/trunk_old_name:X'
       '/trunk_new_name:Y-Z'

    In both cases the gap, if it exists, is M-N, where M = X + 1 and
    N = Y - 1.

    Note that per the rules of 'MERGEINFO MERGE SOURCE NORMALIZATION' we
    should never have multiple gaps, e.g. if we see anything like the
    following then something is quite wrong:

        '/trunk_old_name:A,B-C'
        '/trunk_new_name:D-E'
  */

  if (rangelist->nelts > 1) /* Copy */
    {
      /* As mentioned above, multiple gaps *shouldn't* be possible. */
      SVN_ERR_ASSERT(apr_hash_count(implicit_src_mergeinfo) == 1);

      *gap_start = MIN(revision1, revision2);
      *gap_end = (APR_ARRAY_IDX(rangelist,
                                rangelist->nelts - 1,
                                svn_merge_range_t *))->start;
    }
  else if (apr_hash_count(implicit_src_mergeinfo) > 1) /* Rename */
    {
      apr_array_header_t *requested_rangelist =
        svn_rangelist__initialize(MIN(revision1, revision2),
                                  MAX(revision1, revision2),
                                  TRUE, scratch_pool);
      apr_array_header_t *implicit_rangelist =
        apr_array_make(scratch_pool, 2, sizeof(svn_merge_range_t *));
      apr_array_header_t *gap_rangelist;
      apr_hash_index_t *hi;

      for (hi = apr_hash_first(scratch_pool, implicit_src_mergeinfo);
           hi;
           hi = apr_hash_next(hi))
        {
          apr_array_header_t *value = svn__apr_hash_index_val(hi);

          SVN_ERR(svn_rangelist_merge(&implicit_rangelist, value,
                                      scratch_pool));
        }
      SVN_ERR(svn_rangelist_remove(&gap_rangelist, implicit_rangelist,
                                   requested_rangelist, FALSE,
                                   scratch_pool));

      /* If there is anything left it is the gap. */
      if (gap_rangelist->nelts)
        {
          svn_merge_range_t *gap_range =
            APR_ARRAY_IDX(gap_rangelist, 0, svn_merge_range_t *);

          *gap_start = gap_range->start;
          *gap_end = gap_range->end;
        }
    }

  return SVN_NO_ERROR;
}

/* Helper for do_directory_merge().

   For each child in CHILDREN_WITH_MERGEINFO, populate that
   child's remaining_ranges list.  CHILDREN_WITH_MERGEINFO is expected
   to be sorted in depth first order and each child must be processed in
   that order.  The inheritability of all calculated ranges is TRUE.

   If HONOR_MERGEINFO is set, this function will actually try to be
   intelligent about populating remaining_ranges list.  Otherwise, it
   will claim that each child has a single remaining range, from
   revision1, to revision2.

   SCRATCH_POOL is used for all temporary allocations.  Changes to
   CHILDREN_WITH_MERGEINFO are made in RESULT_POOL.

   Note that if REVISION1 > REVISION2, then each child's remaining_ranges
   member does not adhere to the API rules for rangelists described in
   svn_mergeinfo.h -- See svn_client__merge_path_t.

   See `MERGEINFO MERGE SOURCE NORMALIZATION' for more requirements
   around the values of URL1, REVISION1, URL2, and REVISION2.
*/
static svn_error_t *
populate_remaining_ranges(apr_array_header_t *children_with_mergeinfo,
                          const char *source_root_url,
                          const char *url1,
                          svn_revnum_t revision1,
                          const char *url2,
                          svn_revnum_t revision2,
                          svn_boolean_t honor_mergeinfo,
                          svn_ra_session_t *ra_session,
                          const char *parent_merge_src_canon_path,
                          merge_cmd_baton_t *merge_b,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool)
{
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  int i;
  svn_revnum_t gap_start, gap_end;
  svn_boolean_t child_inherits_implicit;
  svn_client__merge_path_t *parent;
  int parent_index;

  /* If we aren't honoring mergeinfo or this is a --record-only merge,
     we'll make quick work of this by simply adding dummy REVISION1:REVISION2
     ranges for all children. */
  if (! honor_mergeinfo || merge_b->record_only)
    {
      for (i = 0; i < children_with_mergeinfo->nelts; i++)
        {
          svn_client__merge_path_t *child =
            APR_ARRAY_IDX(children_with_mergeinfo, i,
                          svn_client__merge_path_t *);

          parent = NULL;
          svn_pool_clear(iterpool);

          /* Issue #3646 'record-only merges create self-referential
             mergeinfo'.  Get the merge target's implicit mergeinfo (natural
             history).  We'll use it later to avoid setting self-referential
             mergeinfo -- see filter_natural_history_from_mergeinfo(). */
          if (i == 0) /* First item is always the merge target. */
            {
              SVN_ERR(get_full_mergeinfo(NULL, /* child->pre_merge_mergeinfo */
                                         &(child->implicit_mergeinfo),
                                         NULL, /* child->inherited_mergeinfo */
                                         svn_mergeinfo_inherited, ra_session,
                                         child->abspath,
                                         MAX(revision1, revision2),
                                         MIN(revision1, revision2),
                                         merge_b->ctx, result_pool,
                                         iterpool));
            }
          else
            {
              /* Issue #3443 - Subtrees of the merge target can inherit
                 their parent's implicit mergeinfo in most cases. */
              parent_index = find_nearest_ancestor(children_with_mergeinfo,
                                                   FALSE, child->abspath);
              parent = APR_ARRAY_IDX(children_with_mergeinfo, parent_index,
                                     svn_client__merge_path_t *);
              /* If CHILD is a subtree then its parent must be in
                 CHILDREN_WITH_MERGEINFO, see the global comment
                 'THE CHILDREN_WITH_MERGEINFO ARRAY'. */
              SVN_ERR_ASSERT(parent);

              child_inherits_implicit = (parent && !child->switched);
              SVN_ERR(ensure_implicit_mergeinfo(parent, child,
                child_inherits_implicit,
                revision1, revision2,
                ra_session, merge_b->ctx,
                result_pool, iterpool));
            }

          child->remaining_ranges = svn_rangelist__initialize(revision1,
                                                              revision2,
                                                              TRUE,
                                                              result_pool);
        }
      svn_pool_destroy(iterpool);
      return SVN_NO_ERROR;
    }

  /* If, in the merge source's history, there was a copy from a older
     revision, then URL2 won't exist at some range M:N, where
     REVISION1 < M < N < REVISION2. The rules of 'MERGEINFO MERGE SOURCE
     NORMALIZATION' allow this, but we must ignore these gaps when
     calculating what ranges remain to be merged from
     URL1@REVISION1:URL2@REVISION2.  If we don't and try to merge any part
     of URL2@M:URL2@N we would break the editor since no part of that
     actually exists.  See http://svn.haxx.se/dev/archive-2008-11/0618.shtml.

     Find the gaps in the merge target's history, if any.  Eventually
     we will adjust CHILD->REMAINING_RANGES such that we don't describe
     non-existent paths to the editor. */
  SVN_ERR(find_gaps_in_merge_source_history(&gap_start, &gap_end,
                                            parent_merge_src_canon_path,
                                            url1, revision1,
                                            url2, revision2,
                                            ra_session, merge_b,
                                            iterpool));

  /* Stash any gap in the merge command baton, we'll need it later when
     recording mergeinfo describing this merge. */
  if (SVN_IS_VALID_REVNUM(gap_start) && SVN_IS_VALID_REVNUM(gap_end))
    merge_b->implicit_src_gap = svn_rangelist__initialize(gap_start, gap_end,
                                                          TRUE, result_pool);

  for (i = 0; i < children_with_mergeinfo->nelts; i++)
    {
      const char *child_repos_path;
      const char *child_url1, *child_url2;
      svn_client__merge_path_t *child =
        APR_ARRAY_IDX(children_with_mergeinfo, i, svn_client__merge_path_t *);

      parent = NULL;

      /* If the path is absent don't do subtree merge either. */
      SVN_ERR_ASSERT(child);
      if (child->absent)
        continue;

      svn_pool_clear(iterpool);

      child_repos_path = svn_dirent_is_child(merge_b->target_abspath,
                                             child->abspath, iterpool);
      if (!child_repos_path)
        child_repos_path = "";

      child_url1 = svn_path_url_add_component2(url1, child_repos_path,
                                               iterpool);
      child_url2 = svn_path_url_add_component2(url2, child_repos_path,
                                               iterpool);

      /* Get the explicit/inherited mergeinfo for CHILD.  If CHILD is the
         merge target then also get its implicit mergeinfo.  Otherwise defer
         this until we know it is absolutely necessary, since it requires an
         expensive round trip communication with the server. */
      SVN_ERR(get_full_mergeinfo(
        child->pre_merge_mergeinfo ? NULL : &(child->pre_merge_mergeinfo),
        /* Get implicit only for merge target. */
        (i == 0) ? &(child->implicit_mergeinfo) : NULL,
        &(child->inherited_mergeinfo),
        svn_mergeinfo_inherited, ra_session,
        child->abspath,
        MAX(revision1, revision2),
        MIN(revision1, revision2),
        merge_b->ctx, result_pool, iterpool));

      /* If CHILD isn't the merge target find its parent. */
      if (i > 0)
        {
          parent_index = find_nearest_ancestor(children_with_mergeinfo,
                                               FALSE, child->abspath);
          parent = APR_ARRAY_IDX(children_with_mergeinfo, parent_index,
                                 svn_client__merge_path_t *);
          /* If CHILD is a subtree then its parent must be in
             CHILDREN_WITH_MERGEINFO, see the global comment
             'THE CHILDREN_WITH_MERGEINFO ARRAY'. */
          SVN_ERR_ASSERT(parent);
        }

      /* Issue #3443 - Can CHILD inherit PARENT's implicit mergeinfo, saving
         us from having to ask the repos?  The only time we can't do this is if
         CHILD is the merge target and so there is no PARENT to inherit from
         or if CHILD is the root of a switched subtree, in which case PARENT
         exists but is not CHILD's repository parent. */
      child_inherits_implicit = (parent && !child->switched);

      SVN_ERR(calculate_remaining_ranges(parent, child,
        source_root_url,
        child_url1, revision1,
        child_url2, revision2,
        child->pre_merge_mergeinfo,
        merge_b->implicit_src_gap,
        child_inherits_implicit,
        ra_session,
        merge_b->ctx, result_pool,
        iterpool));

      /* Deal with any gap in URL1@REVISION1:URL2@REVISION2's natural history.

         If the gap is a proper subset of CHILD->REMAINING_RANGES then we can
         safely ignore it since we won't describe this path/rev pair.

         If the gap exactly matches or is a superset of a range in
         CHILD->REMAINING_RANGES then we must remove that range so we don't
         attempt to describe non-existent paths via the reporter, this will
         break the editor and our merge.

         If the gap adjoins or overlaps a range in CHILD->REMAINING_RANGES
         then we must *add* the gap so we span the missing revisions. */
      if (child->remaining_ranges->nelts
          && merge_b->implicit_src_gap)
        {
          int j;
          svn_revnum_t start, end;
          svn_boolean_t proper_subset = FALSE;
          svn_boolean_t overlaps_or_adjoins = FALSE;

          /* If this is a reverse merge reorder CHILD->REMAINING_RANGES
              so it will work with the svn_rangelist_* APIs below. */
          if (revision1 > revision2)
            SVN_ERR(svn_rangelist_reverse(child->remaining_ranges, iterpool));

          for (j = 0; j < child->remaining_ranges->nelts; j++)
            {
              start = (APR_ARRAY_IDX(child->remaining_ranges, j,
                                     svn_merge_range_t *))->start;
              end = (APR_ARRAY_IDX(child->remaining_ranges, j,
                                   svn_merge_range_t *))->end;
              if ((start <= gap_start && gap_end < end)
                  || (start < gap_start && gap_end <= end))
                {
                  proper_subset = TRUE;
                  break;
                }
              else if ((gap_start == start) && (end == gap_end))
                {
                  break;
                }
              else if (gap_start <= end && start <= gap_end)  /* intersect */
                {
                  overlaps_or_adjoins = TRUE;
                  break;
                }
            }

          if (!proper_subset)
            {
              /* We need to make adjustements.  Remove from, or add the gap
                 to, CHILD->REMAINING_RANGES as appropriate. */

              if (overlaps_or_adjoins)
                SVN_ERR(svn_rangelist_merge(&(child->remaining_ranges),
                                            merge_b->implicit_src_gap,
                                            result_pool));
              else /* equals == TRUE */
                SVN_ERR(svn_rangelist_remove(&(child->remaining_ranges),
                                             merge_b->implicit_src_gap,
                                             child->remaining_ranges, FALSE,
                                             result_pool));
            }

          if (revision1 > revision2) /* Reverse merge */
            SVN_ERR(svn_rangelist_reverse(child->remaining_ranges, iterpool));
        }
    }

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}


/*-----------------------------------------------------------------------*/

/*** Other Helper Functions ***/

/* Helper for record_mergeinfo_for_dir_merge().

   Adjust, in place, the inheritability of the ranges in RANGELIST to
   describe a merge of RANGELIST into WC_WCPATH at depth DEPTH.  Set
   *RANGELIST_INHERITANCE to the inheritability set.

   WC_PATH_IS_MERGE_TARGET is true if WC_PATH is the target of the merge,
   otherwise WC_PATH is a subtree.

   WC_PATH_HAS_MISSING_CHILD is true if WC_PATH is missing an immediate child
   because the child is switched or absent from the WC, or due to a sparse
   checkout -- see get_mergeinfo_paths().

   Perform any temporary allocations in SCRATCH_POOL. */
static svn_error_t *
calculate_merge_inheritance(apr_array_header_t *rangelist,
                            svn_boolean_t *rangelist_inheritance,
                            const char *local_abspath,
                            svn_boolean_t wc_path_is_merge_target,
                            svn_boolean_t wc_path_has_missing_child,
                            svn_depth_t depth,
                            svn_wc_context_t *wc_ctx,
                            apr_pool_t * scratch_pool)
{
  svn_node_kind_t path_kind;

  SVN_ERR(svn_wc_read_kind(&path_kind, wc_ctx, local_abspath, FALSE,
                           scratch_pool));

  /* Starting assumption. */
  *rangelist_inheritance = TRUE;

  if (path_kind == svn_node_file)
    {
      /* Files *never* have non-inheritable mergeinfo. */
      svn_rangelist__set_inheritance(rangelist, TRUE);
    }
  else if (path_kind == svn_node_dir)
    {
      if (wc_path_is_merge_target)
        {
          if (wc_path_has_missing_child
              || depth == svn_depth_files
              || depth == svn_depth_empty)
            {
              svn_rangelist__set_inheritance(rangelist, FALSE);
              *rangelist_inheritance = FALSE;
            }
          else /* depth == svn_depth_files || depth == svn_depth_empty */
            {
              svn_rangelist__set_inheritance(rangelist, TRUE);
            }
        }
      else /* WC_PATH is a directory subtree of the target. */
        {
          if (wc_path_has_missing_child
              || depth == svn_depth_immediates)
            {
              svn_rangelist__set_inheritance(rangelist, FALSE);
              *rangelist_inheritance = FALSE;
            }
          else /* depth == infinity */
            {
              svn_rangelist__set_inheritance(rangelist, TRUE);
            }
        }
    }
  return SVN_NO_ERROR;
}

/* Calculate the new mergeinfo for the target tree rooted at TARGET_ABSPATH
   based on MERGES (a mapping of absolute WC paths to rangelists representing
   a merge from the source REPOS_REL_PATH).

   If RESULT_CATALOG is NULL, then record the new mergeinfo in the WC (at,
   and possibly below, TARGET_ABSPATH).

   If RESULT_CATALOG is not NULL, then don't record the new mergeinfo on the
   WC, but instead record it in RESULT_CATALOG, where the keys are absolute
   working copy paths and the values are the new mergeinfos for each.
   Allocate additions to RESULT_CATALOG in pool which RESULT_CATALOG was
   created in. */
static svn_error_t *
update_wc_mergeinfo(svn_mergeinfo_catalog_t result_catalog,
                    const char *target_abspath,
                    const char *repos_rel_path,
                    apr_hash_t *merges,
                    svn_boolean_t is_rollback,
                    svn_client_ctx_t *ctx,
                    apr_pool_t *scratch_pool)
{
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  const char *rel_path;
  svn_mergeinfo_t mergeinfo;
  apr_hash_index_t *hi;

  /* Combine the mergeinfo for the revision range just merged into
     the WC with its on-disk mergeinfo. */
  for (hi = apr_hash_first(scratch_pool, merges); hi; hi = apr_hash_next(hi))
    {
      const char *local_abspath = svn__apr_hash_index_key(hi);
      apr_array_header_t *ranges = svn__apr_hash_index_val(hi);
      apr_array_header_t *rangelist;
      svn_error_t *err;
      const char *local_abspath_rel_to_target;

      svn_pool_clear(iterpool);

      /* As some of the merges may've changed the WC's mergeinfo, get
         a fresh copy before using it to update the WC's mergeinfo. */
      err = svn_client__parse_mergeinfo(&mergeinfo, ctx->wc_ctx,
                                        local_abspath, iterpool, iterpool);

      /* If a directory PATH was skipped because it is missing or was
         obstructed by an unversioned item then there's nothing we can
         do with that, so skip it. */
      if (err)
        {
          if (err->apr_err == SVN_ERR_WC_NOT_LOCKED
              || err->apr_err == SVN_ERR_WC_PATH_NOT_FOUND)
            {
              svn_error_clear(err);
              continue;
            }
          else
            {
              return svn_error_trace(err);
            }
        }

      /* If we are attempting to set empty revision range override mergeinfo
         on a path with no explicit mergeinfo, we first need the
         mergeinfo that path inherits. */
      if (mergeinfo == NULL && ranges->nelts == 0)
        {
          svn_boolean_t inherited;
          SVN_ERR(svn_client__get_wc_mergeinfo(&mergeinfo, &inherited,
                                               svn_mergeinfo_nearest_ancestor,
                                               local_abspath, NULL, NULL,
                                               FALSE, ctx, iterpool, iterpool));
        }

      if (mergeinfo == NULL)
        mergeinfo = apr_hash_make(iterpool);

      local_abspath_rel_to_target = svn_dirent_is_child(target_abspath,
                                                        local_abspath,
                                                        iterpool);
      if (local_abspath_rel_to_target)
        rel_path = svn_dirent_join(repos_rel_path,
                                   local_abspath_rel_to_target,
                                   iterpool);
      else
          rel_path = repos_rel_path;
      rangelist = apr_hash_get(mergeinfo, rel_path, APR_HASH_KEY_STRING);
      if (rangelist == NULL)
        rangelist = apr_array_make(iterpool, 0, sizeof(svn_merge_range_t *));

      if (is_rollback)
        {
          ranges = svn_rangelist_dup(ranges, iterpool);
          SVN_ERR(svn_rangelist_reverse(ranges, iterpool));
          SVN_ERR(svn_rangelist_remove(&rangelist, ranges, rangelist,
                                       FALSE,
                                       iterpool));
        }
      else
        {
          SVN_ERR(svn_rangelist_merge(&rangelist, ranges,
                                      iterpool));
        }
      /* Update the mergeinfo by adjusting the path's rangelist. */
      apr_hash_set(mergeinfo, rel_path, APR_HASH_KEY_STRING, rangelist);

      if (is_rollback && apr_hash_count(mergeinfo) == 0)
        mergeinfo = NULL;

      svn_mergeinfo__remove_empty_rangelists(mergeinfo, scratch_pool);

      if (result_catalog)
        {
          svn_mergeinfo_t existing_mergeinfo =
            apr_hash_get(result_catalog, local_abspath, APR_HASH_KEY_STRING);
          apr_pool_t *result_catalog_pool = apr_hash_pool_get(result_catalog);

          if (existing_mergeinfo)
            SVN_ERR(svn_mergeinfo_merge(mergeinfo, existing_mergeinfo,
                                        result_catalog_pool));
          apr_hash_set(result_catalog,
                       apr_pstrdup(result_catalog_pool, local_abspath),
                       APR_HASH_KEY_STRING,
                       svn_mergeinfo_dup(mergeinfo, result_catalog_pool));
        }
      else
        {
          err = svn_client__record_wc_mergeinfo(local_abspath, mergeinfo,
                                                TRUE, ctx, iterpool);

          if (err && err->apr_err == SVN_ERR_ENTRY_NOT_FOUND)
            {
              /* PATH isn't just missing, it's not even versioned as far
                 as this working copy knows.  But it was included in
                 MERGES, which means that the server knows about it.
                 Likely we don't have access to the source due to authz
                 restrictions.  For now just clear the error and
                 continue...

                 ### TODO:  Set non-inheritable mergeinfo on PATH's immediate
                 ### parent and normal mergeinfo on PATH's siblings which we
                 ### do have access to. */
              svn_error_clear(err);
            }
          else
            SVN_ERR(err);
        }
    }

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

/* Helper for record_mergeinfo_for_dir_merge().

   Record override mergeinfo on any paths skipped during a merge.

   Set empty mergeinfo on each path in NOTIFY_B->SKIPPED_PATHS so the path
   does not incorrectly inherit mergeinfo that will later be describing
   the merge.

   MERGEINFO_PATH, NOTIFY_B, and MERGE_B are all cascased from
   arguments of the same name in the caller.

   IS_ROLLBACK is true if the caller is recording a reverse merge and false
   otherwise.  RANGELIST is the set of revisions being merged from
   MERGEINFO_PATH to MERGE_B->TARGET_ABSPATH. */
static svn_error_t *
record_skips(const char *mergeinfo_path,
             const apr_array_header_t *rangelist,
             svn_boolean_t is_rollback,
             notification_receiver_baton_t *notify_b,
             merge_cmd_baton_t *merge_b,
             apr_pool_t *scratch_pool)
{
  apr_hash_index_t *hi;
  apr_hash_t *merges;
  apr_size_t nbr_skips = (notify_b->skipped_abspaths != NULL ?
                          apr_hash_count(notify_b->skipped_abspaths) : 0);
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);

  if (nbr_skips == 0)
    return SVN_NO_ERROR;

  merges = apr_hash_make(scratch_pool);

  /* Override the mergeinfo for child paths which weren't actually merged. */
  for (hi = apr_hash_first(scratch_pool, notify_b->skipped_abspaths); hi;
       hi = apr_hash_next(hi))
    {
      const char *skipped_abspath = svn__apr_hash_index_key(hi);
      svn_wc_notify_state_t obstruction_state;

      svn_pool_clear(iterpool);

      /* Before we override, make sure this is a versioned path, it might
         be an external or missing from disk due to authz restrictions. */
      SVN_ERR(perform_obstruction_check(&obstruction_state,
                                        NULL, NULL, NULL,
                                        merge_b, skipped_abspath,
                                        svn_node_unknown, iterpool));
      if (obstruction_state == svn_wc_notify_state_obstructed
          || obstruction_state == svn_wc_notify_state_missing)
        continue;

      /* Add an empty range list for this path.

         ### TODO: This works fine for a file path skipped because it is
         ### missing as long as the file's parent directory is present.
         ### But missing directory paths skipped are not handled yet,
         ### see issue #2915.

         ### TODO: An empty range is fine if the skipped path doesn't
         ### inherit any mergeinfo from a parent, but if it does
         ### we need to account for that.  See issue #3440
         ### http://subversion.tigris.org/issues/show_bug.cgi?id=3440. */
      apr_hash_set(merges, skipped_abspath,
                   APR_HASH_KEY_STRING,
                   apr_array_make(scratch_pool, 0,
                                  sizeof(svn_merge_range_t *)));

      if (nbr_skips < notify_b->nbr_notifications)
        /* ### Use RANGELIST as the mergeinfo for all children of
           ### this path which were not also explicitly
           ### skipped? */
        ;
    }
  SVN_ERR(update_wc_mergeinfo(NULL, merge_b->target_abspath,
                              mergeinfo_path, merges,
                              is_rollback, merge_b->ctx, iterpool));
  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

/* Create and return an error structure appropriate for the unmerged
   revisions range(s). */
static APR_INLINE svn_error_t *
make_merge_conflict_error(const char *target_wcpath,
                          svn_merge_range_t *r,
                          apr_pool_t *scratch_pool)
{
  return svn_error_createf
    (SVN_ERR_WC_FOUND_CONFLICT, NULL,
     _("One or more conflicts were produced while merging r%ld:%ld into\n"
       "'%s' --\n"
       "resolve all conflicts and rerun the merge to apply the remaining\n"
       "unmerged revisions"),
     r->start, r->end, svn_dirent_local_style(target_wcpath, scratch_pool));
}

/* Helper for do_directory_merge().

   TARGET_WCPATH is a directory and CHILDREN_WITH_MERGEINFO is filled
   with paths (svn_client__merge_path_t *) arranged in depth first order,
   which have mergeinfo set on them or meet one of the other criteria
   defined in get_mergeinfo_paths().  Remove any paths absent from disk
   or scheduled for deletion from CHILDREN_WITH_MERGEINFO which are equal to
   or are descendants of TARGET_WCPATH by setting those children to NULL. */
static void
remove_absent_children(const char *target_wcpath,
                       apr_array_header_t *children_with_mergeinfo,
                       notification_receiver_baton_t *notify_b)
{
  /* Before we try to override mergeinfo for skipped paths, make sure
     the path isn't absent due to authz restrictions, because there's
     nothing we can do about those. */
  int i;
  for (i = 0; i < children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
        APR_ARRAY_IDX(children_with_mergeinfo,
                      i, svn_client__merge_path_t *);
      if ((child->absent || child->scheduled_for_deletion)
          && svn_dirent_is_ancestor(target_wcpath, child->abspath))
        {
          svn_sort__array_delete(children_with_mergeinfo, i--, 1);
        }
    }
}

/* Helper for do_directory_merge() to handle the case were a merge editor
   drive removes explicit mergeinfo from a subtree of the merge target.

   MERGE_B, NOTIFY_B are cascaded from the arguments of the same name in
   do_directory_merge().  If MERGE_B->DRY_RUN is true do nothing, if it is
   false then for each path (if any) in MERGE_B->PATHS_WITH_DELETED_MERGEINFO
   remove that path from NOTIFY_B->CHILDREN_WITH_MERGEINFO by setting that
   child to NULL.  The one exception is for the merge target itself,
   MERGE_B->TARGET_ABSPATH, this must always be present in
   NOTIFY_B->CHILDREN_WITH_MERGEINFO so this is never removed by this
   function. */
static void
remove_children_with_deleted_mergeinfo(merge_cmd_baton_t *merge_b,
                                       notification_receiver_baton_t *notify_b)
{
  int i;

  if (merge_b->dry_run || !merge_b->paths_with_deleted_mergeinfo)
    return;

  /* NOTIFY_B->CHILDREN_WITH_MERGEINFO[0] is the always the merge target
     so start at the first child. */
  for (i = 1; i < notify_b->children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
        APR_ARRAY_IDX(notify_b->children_with_mergeinfo,
                      i, svn_client__merge_path_t *);
      if (apr_hash_get(merge_b->paths_with_deleted_mergeinfo,
                       child->abspath,
                       APR_HASH_KEY_STRING))
        {
          svn_sort__array_delete(notify_b->children_with_mergeinfo, i--, 1);
        }
    }
}

/* Helper for do_directory_merge().

   Set up the diff editor report to merge URL1@REVISION1 to URL2@REVISION2
   into TARGET_ABSPATH and drive it.

   If mergeinfo is not being honored based on MERGE_B, see the doc string for
   HONOR_MERGEINFO() for how this is determined, then ignore
   CHILDREN_WITH_MERGEINFO and merge the diff between URL1@REVISION1 and
   URL2@REVISION2 to TARGET_ABSPATH.

   If mergeinfo is being honored then perform a history-aware merge,
   describing TARGET_ABSPATH and its subtrees to the reporter in such as way
   as to avoid repeating merges already performed per the mergeinfo and
   natural history of TARGET_ABSPATH and its subtrees.

   The ranges that still need to be merged to the TARGET_ABSPATH and its
   subtrees are described in CHILDREN_WITH_MERGEINFO, an array of
   svn_client__merge_path_t * -- see 'THE CHILDREN_WITH_MERGEINFO ARRAY'
   comment at the top of this file for more info.  Note that it is possible
   TARGET_ABSPATH and/or some of its subtrees need only a subset, or no part,
   of REVISION1:REVISION2 to be merged.  Though there is little point to
   calling this function if TARGET_ABSPATH and all its subtrees have already
   had URL1@REVISION1 to URL2@REVISION2 merged, this will work but is a no-op.

   REVISION1 and REVISION2 must be bound by the set of remaining_ranges
   fields in CHILDREN_WITH_MERGEINFO's elements, specifically:

   For forward merges (REVISION1 < REVISION2):

     1) The first svn_merge_range_t * element of each child's remaining_ranges
        array must meet one of the following conditions:

        a) The range's start field is greater than or equal to REVISION2.

        b) The range's end field is REVISION2.

     2) Among all the ranges that meet condition 'b' the oldest start
        revision must equal REVISION1.

   For reverse merges (REVISION1 > REVISION2):

     1) The first svn_merge_range_t * element of each child's remaining_ranges
        array must meet one of the following conditions:

        a) The range's start field is less than or equal to REVISION2.

        b) The range's end field is REVISION2.

     2) Among all the ranges that meet condition 'b' the youngest start
        revision must equal REVISION1.

   Note: If the first svn_merge_range_t * element of some subtree child's
   remaining_ranges array is the same as the first range of that child's
   nearest path-wise ancestor, then the subtree child *will not* be described
   to the reporter.

   DEPTH, NOTIFY_B, and MERGE_B are cascasded from do_directory_merge(), see
   that function for more info.

   If MERGE_B->sources_ancestral is set, then URL1@REVISION1 must be a
   historical ancestor of URL2@REVISION2, or vice-versa (see
   `MERGEINFO MERGE SOURCE NORMALIZATION' for more requirements around
   the values of URL1, REVISION1, URL2, and REVISION2 in this case).
*/
static svn_error_t *
drive_merge_report_editor(const char *target_abspath,
                          const char *url1,
                          svn_revnum_t revision1,
                          const char *url2,
                          svn_revnum_t revision2,
                          const apr_array_header_t *children_with_mergeinfo,
                          svn_depth_t depth,
                          notification_receiver_baton_t *notify_b,
                          merge_cmd_baton_t *merge_b,
                          apr_pool_t *scratch_pool)
{
  const svn_ra_reporter3_t *reporter;
  const svn_delta_editor_t *diff_editor;
  void *diff_edit_baton;
  void *report_baton;
  svn_revnum_t target_start;
  svn_boolean_t honor_mergeinfo;
  const char *old_sess2_url;
  svn_boolean_t is_rollback = revision1 > revision2;

  honor_mergeinfo = HONOR_MERGEINFO(merge_b);

  /* Start with a safe default starting revision for the editor and the
     merge target. */
  target_start = revision1;

  /* If we are honoring mergeinfo the starting revision for the merge target
     might not be REVISION1, in fact the merge target might not need *any*
     part of REVISION1:REVISION2 merged -- Instead some subtree of the target
     needs REVISION1:REVISION2 -- So get the right starting revision for the
     target. */
  if (honor_mergeinfo)
    {
      svn_client__merge_path_t *child;

      /* CHILDREN_WITH_MERGEINFO must always exist if we are honoring
         mergeinfo and must have at least one element (describing the
         merge target). */
      SVN_ERR_ASSERT(children_with_mergeinfo);
      SVN_ERR_ASSERT(children_with_mergeinfo->nelts);

      /* Get the merge target's svn_client__merge_path_t, which is always
         the first in the array due to depth first sorting requirement,
         see 'THE CHILDREN_WITH_MERGEINFO ARRAY'. */
      child = APR_ARRAY_IDX(children_with_mergeinfo, 0,
                            svn_client__merge_path_t *);
      SVN_ERR_ASSERT(child);
      if (child->remaining_ranges->nelts == 0)
        {
          /* The merge target doesn't need anything merged. */
          target_start = revision2;
        }
      else
        {
          /* The merge target has remaining revisions to merge.  These
             ranges may fully or partially overlap the range described
             by REVISION1:REVISION2 or may not intersect that range at
             all. */
          svn_merge_range_t *range =
            APR_ARRAY_IDX(child->remaining_ranges, 0,
                          svn_merge_range_t *);
          if ((!is_rollback && range->start > revision2)
              || (is_rollback && range->start < revision2))
            {
              /* Merge target's first remaining range doesn't intersect. */
              target_start = revision2;
            }
          else
            {
              /* Merge target's first remaining range partially or
                 fully overlaps. */
              target_start = range->start;
            }
        }
    }

  /* Temporarily point our second RA session to URL1, too.  We use
     this to request individual file contents. */
  SVN_ERR(svn_client__ensure_ra_session_url(&old_sess2_url,
                                            merge_b->ra_session2,
                                            url1, scratch_pool));

  /* Get the diff editor and a reporter with which to, ultimately,
     drive it. */
  SVN_ERR(svn_client__get_diff_editor(&diff_editor, &diff_edit_baton,
                                      merge_b->ctx->wc_ctx, target_abspath,
                                      depth,
                                      merge_b->ra_session2, revision1,
                                      FALSE, merge_b->dry_run,
                                      &merge_callbacks, merge_b,
                                      merge_b->ctx->cancel_func,
                                      merge_b->ctx->cancel_baton,
                                      notification_receiver, notify_b,
                                      scratch_pool, scratch_pool));
  SVN_ERR(svn_ra_do_diff3(merge_b->ra_session1,
                          &reporter, &report_baton, revision2,
                          "", depth, merge_b->ignore_ancestry,
                          TRUE,  /* text_deltas */
                          url2, diff_editor, diff_edit_baton, scratch_pool));

  /* Drive the reporter. */
  SVN_ERR(reporter->set_path(report_baton, "", target_start, depth,
                             FALSE, NULL, scratch_pool));
  if (honor_mergeinfo && children_with_mergeinfo)
    {
      /* Describe children with mergeinfo overlapping this merge
         operation such that no repeated diff is retrieved for them from
         the repository. */
      int i;
      apr_pool_t *iterpool = svn_pool_create(scratch_pool);

      /* Start with CHILDREN_WITH_MERGEINFO[1], CHILDREN_WITH_MERGEINFO[0]
         is always the merge target (TARGET_ABSPATH). */
      for (i = 1; i < children_with_mergeinfo->nelts; i++)
        {
          svn_merge_range_t *range;
          const char *child_repos_path;
          svn_client__merge_path_t *parent;
          svn_client__merge_path_t *child =
            APR_ARRAY_IDX(children_with_mergeinfo, i,
                          svn_client__merge_path_t *);
          int parent_index;

          SVN_ERR_ASSERT(child);
          if (child->absent)
            continue;

          svn_pool_clear(iterpool);

          /* Find this child's nearest wc ancestor with mergeinfo. */
          parent_index = find_nearest_ancestor(children_with_mergeinfo,
                                               FALSE, child->abspath);
          parent = APR_ARRAY_IDX(children_with_mergeinfo, parent_index,
                                 svn_client__merge_path_t *);

          /* If a subtree needs the same range applied as its nearest parent
             with mergeinfo or neither the subtree nor this parent need
             REVISION1:REVISION2 merged, then we don't need to describe the
             subtree separately.  In the latter case this could break the
             editor if child->abspath didn't exist at REVISION2 and we
             attempt to describe it via a reporter set_path call. */
          if (child->remaining_ranges->nelts)
            {
              range = APR_ARRAY_IDX(child->remaining_ranges, 0,
                                    svn_merge_range_t *);
              if ((!is_rollback && range->start > revision2)
                  || (is_rollback && range->start < revision2))
                {
                  /* This child's first remaining range comes after the range
                     we are currently merging, so skip it. We expect to get
                     to it in a subsequent call to this function. */
                  continue;
                }
              else if (parent->remaining_ranges->nelts)
                {
                   svn_merge_range_t *parent_range =
                    APR_ARRAY_IDX(parent->remaining_ranges, 0,
                                  svn_merge_range_t *);
                   svn_merge_range_t *child_range =
                    APR_ARRAY_IDX(child->remaining_ranges, 0,
                                  svn_merge_range_t *);
                  if (parent_range->start == child_range->start)
                    continue; /* Subtree needs same range as parent. */
                }
            }
          else /* child->remaining_ranges->nelts == 0*/
            {
              /* If both the subtree and its parent need no ranges applied
                 consider that as the "same ranges" and don't describe
                 the subtree. */
              if (parent->remaining_ranges->nelts == 0)
                continue;
            }

          /* Ok, we really need to describe this subtree as it needs different
             ranges applied than its nearest working copy parent. */
          child_repos_path = svn_dirent_is_child(target_abspath,
                                                 child->abspath,
                                                 iterpool);
          /* This loop is only processing subtrees, so CHILD->ABSPATH
             better be a proper child of the merge target. */
          SVN_ERR_ASSERT(child_repos_path);

          if ((child->remaining_ranges->nelts == 0)
              || (is_rollback && (range->start < revision2))
              || (!is_rollback && (range->start > revision2)))
            {
              /* Nothing to merge to this child.  We'll claim we have
                 it up to date so the server doesn't send us
                 anything. */
              SVN_ERR(reporter->set_path(report_baton, child_repos_path,
                                         revision2, depth, FALSE,
                                         NULL, iterpool));
            }
          else
            {
              SVN_ERR(reporter->set_path(report_baton, child_repos_path,
                                         range->start, depth, FALSE,
                                         NULL, iterpool));
            }
        }
      svn_pool_destroy(iterpool);
    }
  SVN_ERR(reporter->finish_report(report_baton, scratch_pool));

  /* Point the merge baton's second session back where it was. */
  if (old_sess2_url)
    SVN_ERR(svn_ra_reparent(merge_b->ra_session2, old_sess2_url, scratch_pool));

  /* Caller must call svn_sleep_for_timestamps() */
  *(merge_b->use_sleep) = TRUE;

  return SVN_NO_ERROR;
}

/* Iterate over each svn_client__merge_path_t * element in
   CHILDREN_WITH_MERGEINFO and find the most inclusive start revision
   among those element's first remaining_ranges element.

   If IS_ROLLBACK is true the youngest revision is considered the "most
   inclusive" otherwise the oldest revision is.

   If none of CHILDREN_WITH_MERGEINFO's elements have any remaining ranges
   return SVN_INVALID_REVNUM. */
static svn_revnum_t
get_most_inclusive_start_rev(const apr_array_header_t *children_with_mergeinfo,
                             svn_boolean_t is_rollback)
{
  int i;
  svn_revnum_t start_rev = SVN_INVALID_REVNUM;

  for (i = 0; i < children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
        APR_ARRAY_IDX(children_with_mergeinfo, i, svn_client__merge_path_t *);
      svn_merge_range_t *range;

      if ((! child) || child->absent)
        continue;
      if (! child->remaining_ranges->nelts)
        continue;
      range = APR_ARRAY_IDX(child->remaining_ranges, 0, svn_merge_range_t *);
      if ((i == 0) && (range->start == range->end))
        continue;
      if ((start_rev == SVN_INVALID_REVNUM)
          || (is_rollback && (range->start > start_rev))
          || ((! is_rollback) && (range->start < start_rev)))
        start_rev = range->start;
    }
  return start_rev;
}

/* Iterate over each svn_client__merge_path_t * element in
   CHILDREN_WITH_MERGEINFO and find the most inclusive end revision
   among those element's first remaining_ranges element.

   If IS_ROLLBACK is true the oldest revision is considered the "most
   inclusive" otherwise the youngest revision is.

   If none of CHILDREN_WITH_MERGEINFO's elements have any remaining ranges
   return SVN_INVALID_REVNUM. */
static svn_revnum_t
get_most_inclusive_end_rev(const apr_array_header_t *children_with_mergeinfo,
                           svn_boolean_t is_rollback)
{
  int i;
  svn_revnum_t end_rev = SVN_INVALID_REVNUM;

  for (i = 0; i < children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
        APR_ARRAY_IDX(children_with_mergeinfo, i, svn_client__merge_path_t *);
      if (!child || child->absent)
        continue;
      if (child->remaining_ranges->nelts > 0)
        {
          svn_merge_range_t *range = APR_ARRAY_IDX(child->remaining_ranges, 0,
                                                   svn_merge_range_t *);
          if ((end_rev == SVN_INVALID_REVNUM)
              || (is_rollback && (range->end > end_rev))
              || ((! is_rollback) && (range->end < end_rev)))
            end_rev = range->end;
        }
    }
  return end_rev;
}

/* If first item in each child of CHILDREN_WITH_MERGEINFO's
   remaining_ranges is inclusive of END_REV, Slice the first range in
   to two at END_REV. All the allocations are persistent and allocated
   from POOL. */
static void
slice_remaining_ranges(apr_array_header_t *children_with_mergeinfo,
                       svn_boolean_t is_rollback, svn_revnum_t end_rev,
                       apr_pool_t *pool)
{
  int i;
  for (i = 0; i < children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
                                     APR_ARRAY_IDX(children_with_mergeinfo, i,
                                                   svn_client__merge_path_t *);
      if (!child || child->absent)
        continue;
      if (child->remaining_ranges->nelts > 0)
        {
          svn_merge_range_t *range = APR_ARRAY_IDX(child->remaining_ranges, 0,
                                                   svn_merge_range_t *);
          if ((is_rollback && (range->start > end_rev)
               && (range->end < end_rev))
              || (!is_rollback && (range->start < end_rev)
                  && (range->end > end_rev)))
            {
              int j;
              svn_merge_range_t *split_range1, *split_range2;
              apr_array_header_t *orig_remaining_ranges =
                                                     child->remaining_ranges;
              split_range1 = svn_merge_range_dup(range, pool);
              split_range2 = svn_merge_range_dup(range, pool);
              split_range1->end = end_rev;
              split_range2->start = end_rev;
              child->remaining_ranges =
                     apr_array_make(pool, (child->remaining_ranges->nelts + 1),
                                    sizeof(svn_merge_range_t *));
              APR_ARRAY_PUSH(child->remaining_ranges,
                             svn_merge_range_t *) = split_range1;
              APR_ARRAY_PUSH(child->remaining_ranges,
                             svn_merge_range_t *) = split_range2;
              for (j = 1; j < orig_remaining_ranges->nelts; j++)
                {
                  svn_merge_range_t *orig_range =
                                     APR_ARRAY_IDX(orig_remaining_ranges, j,
                                                   svn_merge_range_t *);
                  APR_ARRAY_PUSH(child->remaining_ranges,
                                 svn_merge_range_t *) = orig_range;
                }
            }
        }
    }
}

/* Helper for do_directory_merge().

   For each child in CHILDREN_WITH_MERGEINFO remove the first remaining_ranges
   svn_merge_range_t *element of the child if that range has an end revision
   equal to REVISION.

   If a range is removed from a child's remaining_ranges array, allocate the
   new remaining_ranges array in POOL.

   ### TODO: We should have remaining_ranges in reverse order to avoid
   ### recreating and reallocationg the remaining_ranges every time we want
   ### to remove the first range.  If the ranges were reversed we could simply
   ### pop the last element in the array. */
static void
remove_first_range_from_remaining_ranges(svn_revnum_t revision,
                                         apr_array_header_t
                                           *children_with_mergeinfo,
                                         apr_pool_t *pool)
{
  int i, j;
  for (i = 0; i < children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
                                APR_ARRAY_IDX(children_with_mergeinfo, i,
                                              svn_client__merge_path_t *);
      if (!child || child->absent)
        continue;
      if (child->remaining_ranges->nelts > 0)
        {
          svn_merge_range_t *first_range =
            APR_ARRAY_IDX(child->remaining_ranges, 0, svn_merge_range_t *);
          if (first_range->end == revision)
            {
              apr_array_header_t *orig_remaining_ranges =
                child->remaining_ranges;
              child->remaining_ranges =
                apr_array_make(pool, (child->remaining_ranges->nelts - 1),
                               sizeof(svn_merge_range_t *));
              for (j = 1; j < orig_remaining_ranges->nelts; j++)
                {
                  svn_merge_range_t *range =
                    APR_ARRAY_IDX(orig_remaining_ranges,
                                  j,
                                  svn_merge_range_t *);
                  APR_ARRAY_PUSH(child->remaining_ranges,
                                 svn_merge_range_t *) = range;
                }
            }
        }
    }
}

/* Get a file's content and properties from the repository.
   Set *FILENAME to the local path to a new temporary file holding its text,
   and set *PROPS to a new hash of its properties.

   RA_SESSION is a session whose current root is the URL of the file itself,
   and REV is the revision to get.

   The new temporary file will be created as a sibling of WC_TARGET.
   WC_TARGET should be the local path to the working copy of the file, but
   it does not matter whether anything exists on disk at this path as long
   as WC_TARGET's parent directory exists.

   All allocation occurs in POOL.

   ### TODO: Create the temporary file under .svn/tmp/ instead of next to
   the working file.
*/
static svn_error_t *
single_file_merge_get_file(const char **filename,
                           svn_ra_session_t *ra_session,
                           apr_hash_t **props,
                           svn_revnum_t rev,
                           const char *wc_target,
                           apr_pool_t *pool)
{
  svn_stream_t *stream;

  SVN_ERR(svn_stream_open_unique(&stream, filename,
                                 svn_dirent_dirname(wc_target, pool),
                                 svn_io_file_del_none, pool, pool));
  SVN_ERR(svn_ra_get_file(ra_session, "", rev,
                          stream, NULL, props, pool));
  return svn_stream_close(stream);
}


/* Send a notification specific to a single-file merge if the states
   indicate there's something worth reporting.

   If *HEADER_SENT is not set and HEADER_NOTIFICATION is not NULL, then
   send the header notification before sending the state notification,
   and set *HEADER_SENT to TRUE. */
static APR_INLINE void
single_file_merge_notify(void *notify_baton,
                         const char *local_abspath,
                         svn_wc_notify_action_t action,
                         svn_wc_notify_state_t text_state,
                         svn_wc_notify_state_t prop_state,
                         svn_wc_notify_t *header_notification,
                         svn_boolean_t *header_sent,
                         apr_pool_t *pool)
{
  svn_wc_notify_t *notify = svn_wc_create_notify(local_abspath, action, pool);
  notify->kind = svn_node_file;
  notify->content_state = text_state;
  notify->prop_state = prop_state;
  if (notify->content_state == svn_wc_notify_state_missing)
    notify->action = svn_wc_notify_skip;

  if (IS_OPERATIVE_NOTIFICATION(notify)
      && header_notification
      && (! *header_sent))
    {
      notification_receiver(notify_baton, header_notification, pool);
      *header_sent = TRUE;
    }
  notification_receiver(notify_baton, notify, pool);
}

/* Compare two svn_client__merge_path_t elements **A and **B, given the
   addresses of pointers to them. Return an integer less than, equal to, or
   greater than zero if A sorts before, the same as, or after B, respectively.
   This is a helper for qsort() and bsearch() on an array of such elements. */
static int
compare_merge_path_t_as_paths(const void *a,
                              const void *b)
{
  const svn_client__merge_path_t *child1
    = *((const svn_client__merge_path_t * const *) a);
  const svn_client__merge_path_t *child2
    = *((const svn_client__merge_path_t * const *) b);

  return svn_path_compare_paths(child1->abspath, child2->abspath);
}

/* Return a pointer to the element of CHILDREN_WITH_MERGEINFO whose path
 * is PATH, or return NULL if there is no such element. */
static svn_client__merge_path_t *
get_child_with_mergeinfo(const apr_array_header_t *children_with_mergeinfo,
                         const char *abspath)
{
  svn_client__merge_path_t merge_path;
  svn_client__merge_path_t *key;
  svn_client__merge_path_t **pchild;

  merge_path.abspath = abspath;
  key = &merge_path;
  pchild = bsearch(&key, children_with_mergeinfo->elts,
                   children_with_mergeinfo->nelts,
                   children_with_mergeinfo->elt_size,
                   compare_merge_path_t_as_paths);
  return pchild ? *pchild : NULL;
}

/* Insert a deep copy of INSERT_ELEMENT into the CHILDREN_WITH_MERGEINFO
   array at its correct position.  Allocate the new storage in POOL.
   CHILDREN_WITH_MERGEINFO is a depth first sorted array of
   (svn_client__merge_path_t *). */
static void
insert_child_to_merge(apr_array_header_t *children_with_mergeinfo,
                      const svn_client__merge_path_t *insert_element,
                      apr_pool_t *pool)
{
  int insert_index;
  const svn_client__merge_path_t *new_element;

  /* Find where to insert the new element */
  insert_index =
    svn_sort__bsearch_lower_bound(&insert_element, children_with_mergeinfo,
                                  compare_merge_path_t_as_paths);

  new_element = svn_client__merge_path_dup(insert_element, pool);
  svn_sort__array_insert(&new_element, children_with_mergeinfo, insert_index);
}

/* Helper for get_mergeinfo_paths().

   CHILDREN_WITH_MERGEINFO, MERGE_CMD_BATON, DEPTH, and POOL are
   all cascaded from the arguments of the same name to get_mergeinfo_paths().

   *CHILD is the element in in CHILDREN_WITH_MERGEINFO that
   get_mergeinfo_paths() is iterating over and *CURR_INDEX is index for
   *CHILD.

   If CHILD->ABSPATH is equal to MERGE_CMD_BATON->TARGET_ABSPATH do nothing.
   Else if CHILD->ABSPATH is switched or absent then make sure its immediate
   (as opposed to nearest) parent in CHILDREN_WITH_MERGEINFO is marked as
   missing a child.  If the immediate parent does not exist in
   CHILDREN_WITH_MERGEINFO then create it (and increment *CURR_INDEX so that
   caller doesn't process the inserted element).  Also ensure that
   CHILD->ABSPATH's siblings which are not already present in
   CHILDREN_WITH_MERGEINFO are also added to the array, limited by DEPTH
   (e.g. don't add directory siblings of a switched file).
   Use POOL for temporary allocations only, any new CHILDREN_WITH_MERGEINFO
   elements are allocated in POOL. */
static svn_error_t *
insert_parent_and_sibs_of_sw_absent_del_subtree(
                                   apr_array_header_t *children_with_mergeinfo,
                                   merge_cmd_baton_t *merge_cmd_baton,
                                   int *curr_index,
                                   svn_client__merge_path_t *child,
                                   svn_depth_t depth,
                                   apr_pool_t *pool)
{
  svn_client__merge_path_t *parent;
  const char *parent_abspath;
  apr_pool_t *iterpool;
  const apr_array_header_t *children;
  int i;

  if (!(child->absent
          || (child->switched
              && strcmp(merge_cmd_baton->target_abspath,
                        child->abspath) != 0)))
    return SVN_NO_ERROR;

  parent_abspath = svn_dirent_dirname(child->abspath, pool);
  parent = get_child_with_mergeinfo(children_with_mergeinfo, parent_abspath);
  if (parent)
    {
      parent->missing_child = TRUE;
    }
  else
    {
      /* Create a new element to insert into CHILDREN_WITH_MERGEINFO. */
      parent = apr_pcalloc(pool, sizeof(*parent));
      parent->abspath = apr_pstrdup(pool, parent_abspath);
      parent->missing_child = TRUE;
      /* Insert PARENT into CHILDREN_WITH_MERGEINFO. */
      insert_child_to_merge(children_with_mergeinfo, parent, pool);
      /* Increment for loop index so we don't process the inserted element. */
      (*curr_index)++;
    } /*(parent == NULL) */

  /* Add all of PARENT's non-missing children that are not already present.*/
  SVN_ERR(svn_wc__node_get_children(&children, merge_cmd_baton->ctx->wc_ctx,
                                    parent_abspath, FALSE, pool, pool));
  iterpool = svn_pool_create(pool);
  for (i = 0; i < children->nelts; i++)
    {
      const char *child_abspath = APR_ARRAY_IDX(children, i, const char *);
      svn_client__merge_path_t *sibling_of_missing;

      svn_pool_clear(iterpool);

      /* Does this child already exist in CHILDREN_WITH_MERGEINFO? */
      sibling_of_missing = get_child_with_mergeinfo(children_with_mergeinfo,
                                                    child_abspath);
      /* Create the missing child and insert it into CHILDREN_WITH_MERGEINFO.*/
      if (!sibling_of_missing)
        {
          /* Don't add directory children if DEPTH is svn_depth_files. */
          if (depth == svn_depth_files)
            {
              svn_node_kind_t child_kind;

              SVN_ERR(svn_wc_read_kind(&child_kind,
                                       merge_cmd_baton->ctx->wc_ctx,
                                       child_abspath, FALSE, iterpool));
              if (child_kind != svn_node_file)
                continue;
            }

          sibling_of_missing = apr_pcalloc(pool,
                                           sizeof(*sibling_of_missing));
          sibling_of_missing->abspath = apr_pstrdup(pool,
                                                    child_abspath);
          insert_child_to_merge(children_with_mergeinfo, sibling_of_missing,
                                pool);
        }
    }

  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

/* pre_merge_status_cb's baton */
struct pre_merge_status_baton_t
{
  svn_wc_context_t *wc_ctx;

  /* const char *absolute_wc_path to svn_depth_t * mapping for depths
     of empty, immediates, and files. */
  apr_hash_t *shallow_subtrees;

  /* const char *absolute_wc_path to the same, for all paths missing
     from the working copy. */
  apr_hash_t *missing_subtrees;

  /* const char *absolute_wc_path const char * repos relative path, describing
     the root of each switched subtree in the working copy and the repository
     relative path it is switched to. */
  apr_hash_t *switched_subtrees;

  /* A pool to allocate additions to the above hashes in. */
  apr_pool_t *pool;
};

/* A svn_client_status_func_t callback used by get_mergeinfo_paths to gather
   all switched, absent, and missing subtrees under a merge target. */
static svn_error_t *
pre_merge_status_cb(void *baton,
                    const char *local_abspath,
                    const svn_wc_status3_t *status,
                    apr_pool_t *pool)
{
  struct pre_merge_status_baton_t *pmsb = baton;
  const char *dup_abspath = NULL;

  /* ### Probably needed: Calculate file external status */
  svn_boolean_t is_file_external = FALSE;

  /* ### This block can go once we bumped to the EXTERNALS store */
  if (status->versioned
      && status->switched
      && status->kind == svn_node_file)
    {
      svn_node_kind_t external_kind;

      SVN_ERR(svn_wc__read_external_info(&external_kind, NULL, NULL, NULL,
                                         NULL,
                                         pmsb->wc_ctx, local_abspath,
                                         local_abspath, TRUE, pool, pool));

      is_file_external = (external_kind == svn_node_file);
    }

  if (status->switched && !is_file_external)
    {
      if (!dup_abspath)
        dup_abspath = apr_pstrdup(pmsb->pool, local_abspath);

      apr_hash_set(pmsb->switched_subtrees,
                   apr_pstrdup(pmsb->pool, local_abspath),
                   APR_HASH_KEY_STRING,
                   dup_abspath);
    }

  if (status->depth == svn_depth_empty
      || status->depth == svn_depth_files)
    {
      svn_depth_t *depth = apr_pcalloc(pmsb->pool, sizeof *depth);

      if (!dup_abspath)
        dup_abspath = apr_pstrdup(pmsb->pool, local_abspath);

      *depth = status->depth;
      apr_hash_set(pmsb->shallow_subtrees,
                   dup_abspath,
                   APR_HASH_KEY_STRING,
                   depth);
    }

  if (status->node_status == svn_wc_status_missing)
    {
      svn_boolean_t new_missing_root = TRUE;
      apr_hash_index_t *hi;

      if (!dup_abspath)
        dup_abspath = apr_pstrdup(pmsb->pool, local_abspath);

      for (hi = apr_hash_first(pool, pmsb->missing_subtrees);
           hi;
           hi = apr_hash_next(hi))
        {
          const char *missing_root_path = svn__apr_hash_index_key(hi);

          if (svn_dirent_is_ancestor(missing_root_path,
                                     dup_abspath))
            {
              new_missing_root = FALSE;
              break;
            }
        }

      if (new_missing_root)
        apr_hash_set(pmsb->missing_subtrees, dup_abspath,
                     APR_HASH_KEY_STRING, dup_abspath);
    }

  return SVN_NO_ERROR;
}

/* Helper for do_directory_merge() when performing merge-tracking aware
   merges.

   Walk of the working copy tree rooted at MERGE_CMD_BATON->TARGET_ABSPATH to
   depth DEPTH.  Create an svn_client__merge_path_t * for any path which meets
   one or more of the following criteria:

     1) Path has working svn:mergeinfo.
     2) Path is switched.
     3) Path is a subtree of the merge target (i.e. is not equal to
        MERGE_CMD_BATON->TARGET_ABSPATH) and has no mergeinfo of its own but
        its parent has mergeinfo with non-inheritable ranges.  If this isn't a
        dry-run and the merge is between differences in the same repository,
        then this function will set working mergeinfo on the path equal to
        the mergeinfo inheritable from its parent.
     4) Path has an immediate child (or children) missing from the WC because
        the child is switched or absent from the WC, or due to a sparse
        checkout.
     5) Path has a sibling (or siblings) missing from the WC because the
        sibling is switched, absent, scheduled for deletion, or missing due to
        a sparse checkout.
     6) Path is absent from disk due to an authz restriction.
     7) Path is equal to MERGE_CMD_BATON->TARGET_ABSPATH.
     8) Path is an immediate *directory* child of
        MERGE_CMD_BATON->TARGET_ABSPATH and DEPTH is svn_depth_immediates.
     9) Path is an immediate *file* child of MERGE_CMD_BATON->TARGET_ABSPATH
        and DEPTH is svn_depth_files.
     10) Path is at a depth of 'empty' or 'files'.
     11) Path is missing from disk (e.g. due to an OS-level deletion).

   If subtrees within the requested DEPTH are unexpectedly missing disk,
   then raise SVN_ERR_CLIENT_NOT_READY_TO_MERGE.

   Store the svn_client__merge_path_t *'s in *CHILDREN_WITH_MERGEINFO in
   depth-first order based on the svn_client__merge_path_t *s path member as
   sorted by svn_path_compare_paths().  Set the remaining_ranges field of each
   element to NULL.

   Note: Since the walk is rooted at MERGE_CMD_BATON->TARGET_ABSPATH, the
   latter is guaranteed to be in *CHILDREN_WITH_MERGEINFO and due to the
   depth-first ordering it is guaranteed to be the first element in
   *CHILDREN_WITH_MERGEINFO.

   MERGE_COMMAND_BATON, URL1, URL2, REVISION1, and REVISION2 are
   cascaded from the arguments of the same name in do_directory_merge().

   RA_SESSION is the session for, and SOURCE_ROOT_URL is the repository root
   for, the younger of URL1@REVISION1 and URL2@REVISION2.

   MERGE_SRC_CANON_PATH is the path of of the younger of URL1@REVISION1 and
   URL2@REVISION2, relative to SOURCE_ROOT_URL (with a leading '/').
*/
static svn_error_t *
get_mergeinfo_paths(apr_array_header_t *children_with_mergeinfo,
                    merge_cmd_baton_t *merge_cmd_baton,
                    const char* merge_src_canon_path,
                    const char *source_root_url,
                    const char *url1,
                    const char *url2,
                    svn_revnum_t revision1,
                    svn_revnum_t revision2,
                    svn_ra_session_t *ra_session,
                    svn_depth_t depth,
                    apr_pool_t *result_pool,
                    apr_pool_t *scratch_pool)
{
  int i;
  apr_pool_t *iterpool = NULL;
  apr_hash_t *subtrees_with_mergeinfo;
  apr_hash_t *server_excluded_subtrees;
  apr_hash_t *switched_subtrees;
  apr_hash_t *shallow_subtrees;
  apr_hash_t *missing_subtrees;
  struct pre_merge_status_baton_t pre_merge_status_baton;
  svn_opt_revision_t working_revision;

  working_revision.kind = svn_opt_revision_working;

  /* Case 1: Subtrees with explicit mergeinfo. */
  SVN_ERR(svn_client_propget4(&subtrees_with_mergeinfo, SVN_PROP_MERGEINFO,
                              merge_cmd_baton->target_abspath,
                              &working_revision,
                              &working_revision, NULL, depth, NULL,
                              merge_cmd_baton->ctx, scratch_pool,
                              scratch_pool));

  if (subtrees_with_mergeinfo)
    {
      apr_hash_index_t *hi;

      if (!iterpool)
          iterpool = svn_pool_create(scratch_pool);

      for (hi = apr_hash_first(scratch_pool, subtrees_with_mergeinfo);
           hi;
           hi = apr_hash_next(hi))
        {
          svn_error_t *err;
          svn_mergeinfo_t child_pre_merge_mergeinfo;
          const char *wc_path = svn__apr_hash_index_key(hi);
          svn_string_t *mergeinfo_string = svn__apr_hash_index_val(hi);
          svn_client__merge_path_t *mergeinfo_child =
            apr_pcalloc(result_pool, sizeof(*mergeinfo_child));

          svn_pool_clear(iterpool);
          mergeinfo_child->abspath = apr_pstrdup(result_pool, wc_path);
          err = svn_mergeinfo_parse(&child_pre_merge_mergeinfo,
                                    mergeinfo_string->data, result_pool);
          if (err)
            {
              if (err->apr_err == SVN_ERR_MERGEINFO_PARSE_ERROR)
                {
                  err = svn_error_createf(
                    SVN_ERR_CLIENT_INVALID_MERGEINFO_NO_MERGETRACKING, err,
                    _("Invalid mergeinfo detected on '%s', "
                      "mergetracking not possible"),
                    svn_dirent_local_style(wc_path, scratch_pool));
                }
              return svn_error_trace(err);
            }
          /* Stash this child's pre-existing mergeinfo. */
          mergeinfo_child->pre_merge_mergeinfo = child_pre_merge_mergeinfo;

          /* Note if this child has non-inheritable mergeinfo */
          SVN_ERR(svn_mergeinfo__string_has_noninheritable(
            &(mergeinfo_child->has_noninheritable), mergeinfo_string->data,
            iterpool));

          insert_child_to_merge(children_with_mergeinfo, mergeinfo_child,
                                result_pool);
        }

      /* Sort CHILDREN_WITH_MERGEINFO by each child's path (i.e. as per
         compare_merge_path_t_as_paths).  Any subsequent insertions of new
         children with insert_child_to_merge() require this ordering. */
      qsort(children_with_mergeinfo->elts,
            children_with_mergeinfo->nelts,
            children_with_mergeinfo->elt_size,
            compare_merge_path_t_as_paths);
    }

  /* Case 2: Switched subtrees
     Case 10: Paths at depths of 'empty' or 'files'
     Case 11: Paths missing from disk */
  pre_merge_status_baton.wc_ctx = merge_cmd_baton->ctx->wc_ctx;
  switched_subtrees = apr_hash_make(scratch_pool);
  pre_merge_status_baton.switched_subtrees = switched_subtrees;
  shallow_subtrees = apr_hash_make(scratch_pool);
  pre_merge_status_baton.shallow_subtrees = shallow_subtrees;
  missing_subtrees = apr_hash_make(scratch_pool);
  pre_merge_status_baton.missing_subtrees = missing_subtrees;
  pre_merge_status_baton.pool = scratch_pool;
  SVN_ERR(svn_wc_walk_status(merge_cmd_baton->ctx->wc_ctx,
                             merge_cmd_baton->target_abspath,
                             depth, TRUE, TRUE, TRUE, NULL,
                             pre_merge_status_cb,
                             &pre_merge_status_baton,
                             merge_cmd_baton->ctx->cancel_func,
                             merge_cmd_baton->ctx->cancel_baton,
                             scratch_pool));

  /* Issue #2915: Raise an error describing the roots of any missing
     subtrees, i.e. those that the WC thinks are on disk but have been
     removed outside of Subversion. */
  if (apr_hash_count(missing_subtrees))
    {
      apr_hash_index_t *hi;
      svn_stringbuf_t *missing_subtree_err_buf =
        svn_stringbuf_create(_("Merge tracking not allowed with missing "
                               "subtrees; try restoring these items "
                               "first:\n"), scratch_pool);

      iterpool = svn_pool_create(scratch_pool);

      for (hi = apr_hash_first(scratch_pool, missing_subtrees);
           hi;
           hi = apr_hash_next(hi))
        {
          svn_pool_clear(iterpool);
          svn_stringbuf_appendcstr(missing_subtree_err_buf,
                                   svn_dirent_local_style(
                                     svn__apr_hash_index_key(hi), iterpool));
          svn_stringbuf_appendcstr(missing_subtree_err_buf, "\n");
        }

      return svn_error_create(SVN_ERR_CLIENT_NOT_READY_TO_MERGE,
                              NULL, missing_subtree_err_buf->data);
    }

  if (apr_hash_count(switched_subtrees))
    {
      apr_hash_index_t *hi;

      for (hi = apr_hash_first(scratch_pool, switched_subtrees);
           hi;
           hi = apr_hash_next(hi))
        {
           const char *wc_path = svn__apr_hash_index_key(hi);
           svn_client__merge_path_t *child = get_child_with_mergeinfo(
             children_with_mergeinfo, wc_path);

           if (child)
             {
               child->switched = TRUE;
             }
           else
             {
               svn_client__merge_path_t *switched_child =
                 apr_pcalloc(result_pool, sizeof(*switched_child));
               switched_child->abspath = apr_pstrdup(result_pool, wc_path);
               switched_child->switched = TRUE;
               insert_child_to_merge(children_with_mergeinfo, switched_child,
                                     result_pool);
             }
        }
    }

  if (apr_hash_count(shallow_subtrees))
    {
      apr_hash_index_t *hi;

      for (hi = apr_hash_first(scratch_pool, shallow_subtrees);
           hi;
           hi = apr_hash_next(hi))
        {
           svn_boolean_t new_shallow_child = FALSE;
           const char *wc_path = svn__apr_hash_index_key(hi);
           svn_depth_t *child_depth = svn__apr_hash_index_val(hi);
           svn_client__merge_path_t *shallow_child = get_child_with_mergeinfo(
             children_with_mergeinfo, wc_path);

           if (shallow_child)
             {
               if (*child_depth == svn_depth_empty
                   || *child_depth == svn_depth_files)
                 shallow_child->missing_child = TRUE;
             }
           else
             {
               shallow_child = apr_pcalloc(result_pool,
                                           sizeof(*shallow_child));
               new_shallow_child = TRUE;
               shallow_child->abspath = apr_pstrdup(result_pool, wc_path);

               if (*child_depth == svn_depth_empty
                   || *child_depth == svn_depth_files)
                 shallow_child->missing_child = TRUE;
             }

          /* A little trickery: If PATH doesn't have any mergeinfo or has
             only inheritable mergeinfo, we still describe it as having
             non-inheritable mergeinfo if it is missing a child due to
             a shallow depth.  Why? Because the mergeinfo we'll add to PATH
             to describe the merge must be non-inheritable, so PATH's missing
             children don't inherit it.  Marking these PATHs as non-
             inheritable allows the logic for case 3 to properly account
             for PATH's children. */
          if (!shallow_child->has_noninheritable
              && (*child_depth == svn_depth_empty
                  || *child_depth == svn_depth_files))
            {
              shallow_child->has_noninheritable = TRUE;
            }

          if (new_shallow_child)
            insert_child_to_merge(children_with_mergeinfo, shallow_child,
                                  result_pool);
       }
    }

  /* Case 6: Paths absent from disk due to server-side exclusion. */
  SVN_ERR(svn_wc__get_server_excluded_subtrees(&server_excluded_subtrees,
                                               merge_cmd_baton->ctx->wc_ctx,
                                               merge_cmd_baton->target_abspath,
                                               result_pool, scratch_pool));
  if (server_excluded_subtrees)
    {
      apr_hash_index_t *hi;

      for (hi = apr_hash_first(scratch_pool, server_excluded_subtrees);
           hi;
           hi = apr_hash_next(hi))
        {
           const char *wc_path = svn__apr_hash_index_key(hi);
           svn_client__merge_path_t *child = get_child_with_mergeinfo(
             children_with_mergeinfo, wc_path);

           if (child)
             {
               child->absent = TRUE;
             }
           else
             {
               svn_client__merge_path_t *absent_child =
                 apr_pcalloc(result_pool, sizeof(*absent_child));
               absent_child->abspath = apr_pstrdup(result_pool, wc_path);
               absent_child->absent = TRUE;
               insert_child_to_merge(children_with_mergeinfo, absent_child,
                                     result_pool);
             }
        }
    }

  /* Case 7: The merge target MERGE_CMD_BATON->TARGET_ABSPATH is always
     present. */
  if (!get_child_with_mergeinfo(children_with_mergeinfo,
                                merge_cmd_baton->target_abspath))
    {
      svn_client__merge_path_t *target_child =
        apr_pcalloc(result_pool, sizeof(*target_child));
      target_child->abspath = apr_pstrdup(result_pool,
                                          merge_cmd_baton->target_abspath);
      insert_child_to_merge(children_with_mergeinfo, target_child,
                            result_pool);
    }

  /* Case 8: Path is an immediate *directory* child of
     MERGE_CMD_BATON->TARGET_ABSPATH and DEPTH is svn_depth_immediates.

     Case 9: Path is an immediate *file* child of
     MERGE_CMD_BATON->TARGET_ABSPATH and DEPTH is svn_depth_files. */
  if (depth == svn_depth_immediates || depth == svn_depth_files)
    {
      int j;
      const apr_array_header_t *immediate_children;

      SVN_ERR(svn_wc__node_get_children_of_working_node(
        &immediate_children, merge_cmd_baton->ctx->wc_ctx,
        merge_cmd_baton->target_abspath, FALSE, scratch_pool, scratch_pool));

      if (!iterpool)
        iterpool = svn_pool_create(scratch_pool);

      for (j = 0; j < immediate_children->nelts; j++)
        {
          const char *immediate_child_abspath =
            APR_ARRAY_IDX(immediate_children, j, const char *);
          svn_node_kind_t immediate_child_kind;

          svn_pool_clear(iterpool);
          SVN_ERR(svn_wc_read_kind(&immediate_child_kind,
                                   merge_cmd_baton->ctx->wc_ctx,
                                   immediate_child_abspath, FALSE,
                                   iterpool));
          if ((immediate_child_kind == svn_node_dir
               && depth == svn_depth_immediates)
              || (immediate_child_kind == svn_node_file
                  && depth == svn_depth_files))
            {
              if (!get_child_with_mergeinfo(children_with_mergeinfo,
                                            immediate_child_abspath))
                {
                  svn_client__merge_path_t *immediate_child =
                    apr_pcalloc(result_pool, sizeof(*immediate_child));

                  immediate_child->abspath =
                    apr_pstrdup(result_pool, immediate_child_abspath);

                  if (immediate_child_kind == svn_node_dir
                      && depth == svn_depth_immediates)
                    immediate_child->immediate_child_dir = TRUE;

                  insert_child_to_merge(children_with_mergeinfo,
                                        immediate_child, result_pool);
                }
            }
        }
    }

  /* If DEPTH isn't empty then cover cases 3), 4), and 5), possibly adding
     elements to CHILDREN_WITH_MERGEINFO. */
  if (depth <= svn_depth_empty)
    return SVN_NO_ERROR;

  iterpool = svn_pool_create(scratch_pool);
  for (i = 0; i < children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
        APR_ARRAY_IDX(children_with_mergeinfo, i,
                      svn_client__merge_path_t *);
      svn_pool_clear(iterpool);

      /* Case 3) Where merging to a path with a switched child the path
         gets non-inheritable mergeinfo for the merge range performed and
         the child gets its own set of mergeinfo.  If the switched child
         later "returns", e.g. a switched path is unswitched, the child
         may not have any explicit mergeinfo.  If the initial merge is
         repeated we don't want to repeat the merge for the path, but we
         do want to repeat it for the previously switched child.  To
         ensure this we check if all of CHILD's non-missing children have
         explicit mergeinfo (they should already be present in
         CHILDREN_WITH_MERGEINFO if they do).  If not,
         add the children without mergeinfo to CHILDREN_WITH_MERGEINFO so
         do_directory_merge() will merge them independently.

         But that's not enough!  Since do_directory_merge() performs
         the merges on the paths in CHILDREN_WITH_MERGEINFO in a depth
         first manner it will merge the previously switched path's parent
         first.  As part of this merge it will update the parent's
         previously non-inheritable mergeinfo and make it inheritable
         (since it notices the path has no missing children), then when
         do_directory_merge() finally merges the previously missing
         child it needs to get mergeinfo from the child's nearest
         ancestor, but since do_directory_merge() already tweaked that
         mergeinfo, removing the non-inheritable flag, it appears that the
         child already has been merged to.  To prevent this we set
         override mergeinfo on the child now, before any merging is done,
         so it has explicit mergeinfo that reflects only CHILD's
         inheritable mergeinfo. */

      if (child->has_noninheritable)
        {
          const apr_array_header_t *children;
          int j;

          SVN_ERR(svn_wc__node_get_children(&children,
                                            merge_cmd_baton->ctx->wc_ctx,
                                            child->abspath, FALSE,
                                            iterpool, iterpool));
          for (j = 0; j < children->nelts; j++)
            {
              svn_client__merge_path_t *child_of_noninheritable;
              const char *child_abspath = APR_ARRAY_IDX(children, j,
                                                        const char*);

              /* Does this child already exist in CHILDREN_WITH_MERGEINFO?
                 If not, create it and insert it into
                 CHILDREN_WITH_MERGEINFO and set override mergeinfo on
                 it. */
              child_of_noninheritable =
                get_child_with_mergeinfo(children_with_mergeinfo,
                                         child_abspath);
              if (!child_of_noninheritable)
                {
                  /* Don't add directory children if DEPTH
                     is svn_depth_files. */
                  if (depth == svn_depth_files)
                    {
                      svn_node_kind_t child_kind;
                      SVN_ERR(svn_wc_read_kind(&child_kind,
                                               merge_cmd_baton->ctx->wc_ctx,
                                               child_abspath, FALSE,
                                               iterpool));
                      if (child_kind != svn_node_file)
                        continue;
                    }
                  /* else DEPTH is infinity or immediates so we want both
                     directory and file children. */

                  child_of_noninheritable =
                    apr_pcalloc(result_pool,
                                sizeof(*child_of_noninheritable));
                  child_of_noninheritable->child_of_noninheritable = TRUE;
                  child_of_noninheritable->abspath =
                    apr_pstrdup(result_pool,
                                child_abspath);
                  insert_child_to_merge(children_with_mergeinfo,
                                        child_of_noninheritable,
                                        result_pool);
                  if (!merge_cmd_baton->dry_run
                      && merge_cmd_baton->same_repos)
                    {
                      svn_boolean_t inherited;
                      svn_mergeinfo_t mergeinfo;

                      SVN_ERR(svn_client__get_wc_mergeinfo(
                        &mergeinfo, &inherited,
                        svn_mergeinfo_nearest_ancestor,
                        child_of_noninheritable->abspath,
                        merge_cmd_baton->target_abspath, NULL, FALSE,
                        merge_cmd_baton->ctx, iterpool, iterpool));

                      SVN_ERR(svn_client__record_wc_mergeinfo(
                        child_of_noninheritable->abspath, mergeinfo,
                        FALSE, merge_cmd_baton->ctx, iterpool));
                    }
                }
            }
        }
      /* Case 4 and 5 are handled by the following function. */
      SVN_ERR(insert_parent_and_sibs_of_sw_absent_del_subtree(
        children_with_mergeinfo, merge_cmd_baton, &i, child,
        depth, result_pool));
    } /* i < children_with_mergeinfo->nelts */
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}


/* Implements the svn_log_entry_receiver_t interface.
 *
 * BATON is an 'apr_array_header_t *'.  Copy LOG_ENTRY->revision (in
 * BATON's pool, not POOL) and push the copy onto BATON.  Thus, a
 * series of invocations of this callback accumulates the
 * corresponding set of revisions into BATON.
 */
static svn_error_t *
log_changed_revs(void *baton,
                  svn_log_entry_t *log_entry,
                  apr_pool_t *pool)
{
  apr_array_header_t *revs = baton;
  svn_revnum_t *revision = apr_palloc(revs->pool, sizeof(*revision));
  *revision = log_entry->revision;
  APR_ARRAY_PUSH(revs, svn_revnum_t *) = revision;
  return SVN_NO_ERROR;
}


/* Set *OPERATIVE_RANGES_P to an array of svn_merge_range_t * merge
   range objects copied wholesale from RANGES which have the property
   that in some revision within that range the object identified by
   RA_SESSION was modified (if by "modified" we mean "'svn log' would
   return that revision).  *OPERATIVE_RANGES_P is allocated from the
   same pool as RANGES, and the ranges within it are shared with
   RANGES, too.

   *OPERATIVE_RANGES_P may be the same as RANGES (that is, the output
   parameter is set only after the input is no longer used).

   Use POOL for temporary allocations.  */
static svn_error_t *
remove_noop_merge_ranges(apr_array_header_t **operative_ranges_p,
                         svn_ra_session_t *ra_session,
                         const apr_array_header_t *ranges,
                         apr_pool_t *pool)
{
  int i;
  svn_revnum_t oldest_rev = SVN_INVALID_REVNUM;
  svn_revnum_t youngest_rev = SVN_INVALID_REVNUM;
  apr_array_header_t *changed_revs =
    apr_array_make(pool, ranges->nelts, sizeof(svn_revnum_t *));
  apr_array_header_t *operative_ranges =
    apr_array_make(ranges->pool, ranges->nelts, ranges->elt_size);
  apr_array_header_t *log_targets =
    apr_array_make(pool, 1, sizeof(const char *));
  APR_ARRAY_PUSH(log_targets, const char *) = "";

  /* Find the revision extremes of the RANGES we have. */
  for (i = 0; i < ranges->nelts; i++)
    {
      svn_merge_range_t *r = APR_ARRAY_IDX(ranges, i, svn_merge_range_t *);
      svn_revnum_t max_rev = MAX(r->start, r->end);
      svn_revnum_t min_rev = MIN(r->start, r->end) + 1;

      if ((! SVN_IS_VALID_REVNUM(youngest_rev)) || (max_rev > youngest_rev))
        youngest_rev = max_rev;
      if ((! SVN_IS_VALID_REVNUM(oldest_rev)) || (min_rev < oldest_rev))
        oldest_rev = min_rev;
    }

  /* Get logs across those ranges, recording which revisions hold
     changes to our object's history. */
  SVN_ERR(svn_ra_get_log2(ra_session, log_targets, youngest_rev,
                          oldest_rev, 0, FALSE, FALSE, FALSE,
                          apr_array_make(pool, 0, sizeof(const char *)),
                          log_changed_revs, changed_revs, pool));

  /* Are there *any* changes? */
  if (changed_revs->nelts)
    {
      svn_revnum_t oldest_changed_rev, youngest_changed_rev;

      /* Our list of changed revisions should be in youngest-to-oldest
         order. */
      youngest_changed_rev = *(APR_ARRAY_IDX(changed_revs,
                                             0, svn_revnum_t *));
      oldest_changed_rev = *(APR_ARRAY_IDX(changed_revs,
                                           changed_revs->nelts - 1,
                                           svn_revnum_t *));

      /* Now, copy from RANGES to *OPERATIVE_RANGES, filtering out ranges
         that aren't operative (by virtue of not having any revisions
         represented in the CHANGED_REVS array). */
      for (i = 0; i < ranges->nelts; i++)
        {
          svn_merge_range_t *range = APR_ARRAY_IDX(ranges, i,
                                                   svn_merge_range_t *);
          svn_revnum_t range_min = MIN(range->start, range->end) + 1;
          svn_revnum_t range_max = MAX(range->start, range->end);
          int j;

          /* If the merge range is entirely outside the range of changed
             revisions, we've no use for it. */
          if ((range_min > youngest_changed_rev)
              || (range_max < oldest_changed_rev))
            continue;

          /* Walk through the changed_revs to see if any of them fall
             inside our current range. */
          for (j = 0; j < changed_revs->nelts; j++)
            {
              svn_revnum_t *changed_rev =
                APR_ARRAY_IDX(changed_revs, j, svn_revnum_t *);
              if ((*changed_rev >= range_min) && (*changed_rev <= range_max))
                {
                  APR_ARRAY_PUSH(operative_ranges, svn_merge_range_t *) =
                    range;
                  break;
                }
            }
        }
    }

  *operative_ranges_p = operative_ranges;
  return SVN_NO_ERROR;
}


/*-----------------------------------------------------------------------*/

/*** Merge Source Normalization ***/

/* qsort-compatible sort routine, rating merge_source_t * objects to
   be in descending (youngest-to-oldest) order based on their ->rev1
   component. */
static int
compare_merge_source_ts(const void *a,
                        const void *b)
{
  svn_revnum_t a_rev = (*(const merge_source_t *const *)a)->rev1;
  svn_revnum_t b_rev = (*(const merge_source_t *const *)b)->rev1;
  if (a_rev == b_rev)
    return 0;
  return a_rev < b_rev ? 1 : -1;
}

/* Set *MERGE_SOURCE_TS_P to a list of merge sources generated by
   slicing history location SEGMENTS with a given requested merge
   RANGE.  Use SOURCE_ROOT_URL for full source URL calculation.

   Order the merge sources in *MERGE_SOURCE_TS_P from oldest to
   youngest. */
static svn_error_t *
combine_range_with_segments(apr_array_header_t **merge_source_ts_p,
                            svn_merge_range_t *range,
                            const apr_array_header_t *segments,
                            const char *source_root_url,
                            apr_pool_t *pool)
{
  apr_array_header_t *merge_source_ts =
    apr_array_make(pool, 1, sizeof(merge_source_t *));
  svn_revnum_t minrev = MIN(range->start, range->end) + 1;
  svn_revnum_t maxrev = MAX(range->start, range->end);
  svn_boolean_t subtractive = (range->start > range->end);
  int i;

  for (i = 0; i < segments->nelts; i++)
    {
      svn_location_segment_t *segment =
        APR_ARRAY_IDX(segments, i, svn_location_segment_t *);
      merge_source_t *merge_source;
      const char *path1 = NULL;
      svn_revnum_t rev1;

      /* If this segment doesn't overlap our range at all, or
         represents a gap, ignore it. */
      if ((segment->range_end < minrev)
          || (segment->range_start > maxrev)
          || (! segment->path))
        continue;

      /* If our range spans a segment boundary, we have to point our
         merge_source_t's path1 to the path of the immediately older
         segment, else it points to the same location as its path2.  */
      rev1 = MAX(segment->range_start, minrev) - 1;
      if (minrev <= segment->range_start)
        {
          if (i > 0)
            {
              path1 = (APR_ARRAY_IDX(segments, i - 1,
                                     svn_location_segment_t *))->path;
            }
          /* If we've backed PATH1 up into a segment gap, let's back
             it up further still to the segment before the gap.  We'll
             have to adjust rev1, too. */
          if ((! path1) && (i > 1))
            {
              path1 = (APR_ARRAY_IDX(segments, i - 2,
                                     svn_location_segment_t *))->path;
              rev1 = (APR_ARRAY_IDX(segments, i - 2,
                                    svn_location_segment_t *))->range_end;
            }
        }
      else
        {
          path1 = apr_pstrdup(pool, segment->path);
        }

      /* If we don't have two valid paths, we won't know what to do
         when merging.  This could happen if someone requested a merge
         where the source didn't exist in a particular revision or
         something.  The merge code would probably bomb out anyway, so
         we'll just *not* create a merge source in this case. */
      if (! (path1 && segment->path))
        continue;

      /* Build our merge source structure. */
      merge_source = apr_pcalloc(pool, sizeof(*merge_source));
      merge_source->url1 = svn_path_url_add_component2(source_root_url,
                                                       path1,
                                                       pool);
      merge_source->url2 = svn_path_url_add_component2(source_root_url,
                                                       segment->path,
                                                       pool);
      merge_source->rev1 = rev1;
      merge_source->rev2 = MIN(segment->range_end, maxrev);

      /* If this is subtractive, reverse the whole calculation. */
      if (subtractive)
        {
          svn_revnum_t tmprev = merge_source->rev1;
          const char *tmpurl = merge_source->url1;
          merge_source->rev1 = merge_source->rev2;
          merge_source->url1 = merge_source->url2;
          merge_source->rev2 = tmprev;
          merge_source->url2 = tmpurl;
        }

      APR_ARRAY_PUSH(merge_source_ts, merge_source_t *) = merge_source;
    }

  /* If this was a subtractive merge, and we created more than one
     merge source, we need to reverse the sort ordering of our sources. */
  if (subtractive && (merge_source_ts->nelts > 1))
    qsort(merge_source_ts->elts, merge_source_ts->nelts,
          merge_source_ts->elt_size, compare_merge_source_ts);

  *merge_source_ts_p = merge_source_ts;
  return SVN_NO_ERROR;
}

/* Set *MERGE_SOURCES_P to an array of merge_source_t * objects, each
   holding the paths and revisions needed to fully describe a range of
   requested merges; order the objects from oldest to youngest.

   Determine the requested merges by examining SOURCE (and its
   associated URL, SOURCE_URL) and PEG_REVISION (which specifies the
   line of history from which merges will be pulled) and
   RANGES_TO_MERGE (a list of svn_opt_revision_range_t's which provide
   revision ranges).  Note that SOURCE may itself be either a working
   copy path or a URL; in the latter case, SOURCE_URL is probably
   identical to SOURCE.

   If PEG_REVISION is unspecified, treat that it as HEAD.

   SOURCE_ROOT_URL is the root URL of the source repository.

   Use RA_SESSION -- whose session URL matches SOURCE_URL -- to answer
   historical questions.

   CTX is a client context baton.

   SCRATCH_POOL is used for all temporary allocations.  MERGE_SOURCES_P and
   its contents are allocated in RESULT_POOL.

   See `MERGEINFO MERGE SOURCE NORMALIZATION' for more on the
   background of this function.
*/
static svn_error_t *
normalize_merge_sources(apr_array_header_t **merge_sources_p,
                        const char *source,
                        const char *source_url,
                        const char *source_root_url,
                        const svn_opt_revision_t *peg_revision,
                        const apr_array_header_t *ranges_to_merge,
                        svn_ra_session_t *ra_session,
                        svn_client_ctx_t *ctx,
                        apr_pool_t *result_pool,
                        apr_pool_t *scratch_pool)
{
  svn_revnum_t youngest_rev = SVN_INVALID_REVNUM;
  svn_revnum_t peg_revnum;
  svn_revnum_t oldest_requested = SVN_INVALID_REVNUM;
  svn_revnum_t youngest_requested = SVN_INVALID_REVNUM;
  svn_revnum_t trim_revision = SVN_INVALID_REVNUM;
  apr_array_header_t *merge_range_ts, *segments;
  const char *source_abspath_or_url;
  int i;
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);

  if(!svn_path_is_url(source))
    SVN_ERR(svn_dirent_get_absolute(&source_abspath_or_url, source,
                                    scratch_pool));
  else
    source_abspath_or_url = source;

  /* Initialize our return variable. */
  *merge_sources_p = apr_array_make(result_pool, 1, sizeof(merge_source_t *));

  /* Resolve our PEG_REVISION to a real number. */
  SVN_ERR(svn_client__get_revision_number(&peg_revnum, &youngest_rev,
                                          ctx->wc_ctx,
                                          source_abspath_or_url,
                                          ra_session, peg_revision,
                                          iterpool));
  if (! SVN_IS_VALID_REVNUM(peg_revnum))
    return svn_error_create(SVN_ERR_CLIENT_BAD_REVISION, NULL, NULL);

  /* Create a list to hold svn_merge_range_t's. */
  merge_range_ts = apr_array_make(scratch_pool, ranges_to_merge->nelts,
                                  sizeof(svn_merge_range_t *));

  for (i = 0; i < ranges_to_merge->nelts; i++)
    {
      svn_revnum_t range_start_rev, range_end_rev;
      svn_opt_revision_t *range_start =
        &((APR_ARRAY_IDX(ranges_to_merge, i,
                         svn_opt_revision_range_t *))->start);
      svn_opt_revision_t *range_end =
        &((APR_ARRAY_IDX(ranges_to_merge, i,
                         svn_opt_revision_range_t *))->end);

      svn_pool_clear(iterpool);

      /* Resolve revisions to real numbers, validating as we go. */
      if ((range_start->kind == svn_opt_revision_unspecified)
          || (range_end->kind == svn_opt_revision_unspecified))
        return svn_error_create(SVN_ERR_CLIENT_BAD_REVISION, NULL,
                                _("Not all required revisions are specified"));

      SVN_ERR(svn_client__get_revision_number(&range_start_rev, &youngest_rev,
                                              ctx->wc_ctx,
                                              source_abspath_or_url,
                                              ra_session, range_start,
                                              iterpool));
      SVN_ERR(svn_client__get_revision_number(&range_end_rev, &youngest_rev,
                                              ctx->wc_ctx,
                                              source_abspath_or_url,
                                              ra_session, range_end,
                                              iterpool));

      /* If this isn't a no-op range... */
      if (range_start_rev != range_end_rev)
        {
          /* ...then create an svn_merge_range_t object for it. */
          svn_merge_range_t *range = apr_pcalloc(scratch_pool,
                                                 sizeof(*range));
          range->start = range_start_rev;
          range->end = range_end_rev;
          range->inheritable = TRUE;

          /* Add our merge range to our list thereof. */
          APR_ARRAY_PUSH(merge_range_ts, svn_merge_range_t *) = range;
        }
    }

  /* No ranges to merge?  No problem. */
  if (merge_range_ts->nelts == 0)
    {
      svn_pool_destroy(iterpool);
      return SVN_NO_ERROR;
    }

  /* Find the extremes of the revisions across our set of ranges. */
  for (i = 0; i < merge_range_ts->nelts; i++)
    {
      svn_merge_range_t *range =
        APR_ARRAY_IDX(merge_range_ts, i, svn_merge_range_t *);
      svn_revnum_t minrev = MIN(range->start, range->end);
      svn_revnum_t maxrev = MAX(range->start, range->end);

      /* Keep a running tally of the oldest and youngest requested
         revisions. */
      if ((! SVN_IS_VALID_REVNUM(oldest_requested))
          || (minrev < oldest_requested))
        oldest_requested = minrev;
      if ((! SVN_IS_VALID_REVNUM(youngest_requested))
          || (maxrev > youngest_requested))
        youngest_requested = maxrev;
    }

  /* ### FIXME:  Our underlying APIs can't yet handle the case where
     the peg revision isn't the youngest of the three revisions.  So
     we'll just verify that the source in the peg revision is related
     to the the source in the youngest requested revision (which is
     all the underlying APIs would do in this case right now anyway). */
  if (peg_revnum < youngest_requested)
    {
      const char *start_url;
      svn_opt_revision_t requested, unspec, pegrev, *start_revision;
      unspec.kind = svn_opt_revision_unspecified;
      requested.kind = svn_opt_revision_number;
      requested.value.number = youngest_requested;
      pegrev.kind = svn_opt_revision_number;
      pegrev.value.number = peg_revnum;

      SVN_ERR(svn_client__repos_locations(&start_url, &start_revision,
                                          NULL, NULL,
                                          ra_session, source_url,
                                          &pegrev, &requested,
                                          &unspec, ctx, iterpool));
      peg_revnum = youngest_requested;
    }

  /* Fetch the locations for our merge range span. */
  SVN_ERR(svn_client__repos_location_segments(&segments,
                                              ra_session, "",
                                              peg_revnum,
                                              youngest_requested,
                                              oldest_requested,
                                              ctx, result_pool));

  /* See if we fetched enough history to do the job.  "Surely we did,"
     you say.  "After all, we covered the entire requested merge
     range."  Yes, that's true, but if our first segment doesn't
     extend back to the oldest request revision, we've got a special
     case to deal with.  Or if the first segment represents a gap,
     that's another special case.  */
  trim_revision = SVN_INVALID_REVNUM;
  if (segments->nelts)
    {
      svn_location_segment_t *segment =
        APR_ARRAY_IDX(segments, 0, svn_location_segment_t *);

      /* If the first segment doesn't start with the OLDEST_REQUESTED
         revision, we'll need to pass a trim revision to our range
         cruncher. */
      if (segment->range_start != oldest_requested)
        {
          trim_revision = segment->range_start;
        }

      /* Else, if the first segment has no path (and therefore is a
         gap), then we'll fetch the copy source revision from the
         second segment (provided there is one, of course) and use it
         to prepend an extra pathful segment to our list.

         ### We could avoid this bit entirely if we'd passed
         ### SVN_INVALID_REVNUM instead of OLDEST_REQUESTED to
         ### svn_client__repos_location_segments(), but that would
         ### really penalize clients hitting pre-1.5 repositories with
         ### the typical small merge range request (because of the
         ### lack of a node-origins cache in the repository).  */
      else if (! segment->path)
        {
          if (segments->nelts > 1)
            {
              svn_location_segment_t *segment2 =
                APR_ARRAY_IDX(segments, 1, svn_location_segment_t *);
              const char *copyfrom_path, *segment_url;
              svn_revnum_t copyfrom_rev;
              svn_opt_revision_t range_start_rev;
              range_start_rev.kind = svn_opt_revision_number;
              range_start_rev.value.number = segment2->range_start;

              segment_url = svn_path_url_add_component2(source_root_url,
                                                        segment2->path,
                                                        iterpool);
              SVN_ERR(svn_client__get_copy_source(segment_url,
                                                  &range_start_rev,
                                                  &copyfrom_path,
                                                  &copyfrom_rev,
                                                  ctx, result_pool));
              /* Got copyfrom data?  Fix up the first segment to cover
                 back to COPYFROM_REV + 1, and then prepend a new
                 segment covering just COPYFROM_REV. */
              if (copyfrom_path && SVN_IS_VALID_REVNUM(copyfrom_rev))
                {
                  svn_location_segment_t *new_segment =
                    apr_pcalloc(result_pool, sizeof(*new_segment));
                  /* Skip the leading '/'. */
                  new_segment->path = (*copyfrom_path == '/')
                    ? copyfrom_path + 1 : copyfrom_path;
                  new_segment->range_start = copyfrom_rev;
                  new_segment->range_end = copyfrom_rev;
                  segment->range_start = copyfrom_rev + 1;
                  APR_ARRAY_PUSH(segments, svn_location_segment_t *) = NULL;
                  memmove(segments->elts + segments->elt_size,
                          segments->elts,
                          segments->elt_size * (segments->nelts - 1));
                  APR_ARRAY_IDX(segments, 0, svn_location_segment_t *) =
                    new_segment;
                }
            }
        }
    }

  /* For each range in our requested range set, try to determine the
     path(s) associated with that range.  */
  for (i = 0; i < merge_range_ts->nelts; i++)
    {
      svn_merge_range_t *range =
        APR_ARRAY_IDX(merge_range_ts, i, svn_merge_range_t *);
      apr_array_header_t *merge_sources;
      int j;

      if (SVN_IS_VALID_REVNUM(trim_revision))
        {
          /* If the youngest of the range revisions predates the trim
             revision, discard the range. */
          if (MAX(range->start, range->end) < trim_revision)
            continue;

          /* Otherwise, if either of oldest of the range revisions predates
             the trim revision, update the range revision to be equal
             to the trim revision. */
          if (range->start < trim_revision)
            range->start = trim_revision;
          if (range->end < trim_revision)
            range->end = trim_revision;
        }

      /* Copy the resulting merge sources into master list thereof. */
      SVN_ERR(combine_range_with_segments(&merge_sources, range,
                                          segments, source_root_url,
                                          result_pool));
      for (j = 0; j < merge_sources->nelts; j++)
        {
          APR_ARRAY_PUSH(*merge_sources_p, merge_source_t *) =
            APR_ARRAY_IDX(merge_sources, j, merge_source_t *);
        }
    }

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}


/*-----------------------------------------------------------------------*/

/*** Merge Workhorse Functions ***/

/* Helper for do_directory_merge() and do_file_merge() which filters out a
   path's own natural history from the mergeinfo describing a merge.

   Given the natural history IMPLICIT_MERGEINFO of some wc merge target path,
   the repository-relative merge source path SOURCE_REL_PATH, and the
   requested merge range REQUESTED_RANGE from SOURCE_REL_PATH, remove any
   portion of REQUESTED_RANGE which is already described in
   IMPLICIT_MERGEINFO.  Store the result in *FILTERED_RANGELIST.

   This function only filters natural history for mergeinfo that will be
   *added* during a forward merge.  Removing natural history from explicit
   mergeinfo is harmless.  If REQUESTED_RANGE describes a reverse merge,
   then *FILTERED_RANGELIST is simply populated with one range described
   by REQUESTED_RANGE.  *FILTERED_RANGELIST is never NULL.

   Allocate *FILTERED_RANGELIST in POOL. */
static svn_error_t *
filter_natural_history_from_mergeinfo(apr_array_header_t **filtered_rangelist,
                                      const char *source_rel_path,
                                      svn_mergeinfo_t implicit_mergeinfo,
                                      svn_merge_range_t *requested_range,
                                      apr_pool_t *pool)
{
  /* Make the REQUESTED_RANGE into a rangelist. */
  apr_array_header_t *requested_rangelist =
    svn_rangelist__initialize(requested_range->start, requested_range->end,
                              requested_range->inheritable, pool);

  *filtered_rangelist = NULL;

  /* For forward merges: If the IMPLICIT_MERGEINFO already describes ranges
     associated with SOURCE_REL_PATH then filter those ranges out. */
  if (implicit_mergeinfo
      && (requested_range->start < requested_range->end))
    {
      apr_array_header_t *implied_rangelist =
        apr_hash_get(implicit_mergeinfo, source_rel_path,
                     APR_HASH_KEY_STRING);

      if (implied_rangelist)
        SVN_ERR(svn_rangelist_remove(filtered_rangelist,
                                     implied_rangelist,
                                     requested_rangelist,
                                     FALSE, pool));
    }

  /* If no filtering was performed the filtered rangelist is
     simply the requested rangelist.*/
  if (! (*filtered_rangelist))
    *filtered_rangelist = requested_rangelist;

  return SVN_NO_ERROR;
}

/* The single-file, simplified version of do_directory_merge(), which see for
   parameter descriptions.

   Additional parameters:

   If SOURCES_RELATED is set, the "left" and "right" sides of the
   merge source are historically related (ancestors, uncles, second
   cousins thrice removed, etc...).  (This is used to simulate the
   history checks that the repository logic does in the directory case.)

   If mergeinfo is being recorded to describe this merge, and RESULT_CATALOG
   is not NULL, then don't record the new mergeinfo on the TARGET_ABSPATH,
   but instead record it in RESULT_CATALOG, where the key is TARGET_ABSPATH
   and the value is the new mergeinfo for that path.  Allocate additions
   to RESULT_CATALOG in pool which RESULT_CATALOG was created in.

   Note: MERGE_B->RA_SESSION1 must be associated with URL1 and
   MERGE_B->RA_SESSION2 with URL2.
*/
static svn_error_t *
do_file_merge(svn_mergeinfo_catalog_t result_catalog,
              const char *url1,
              svn_revnum_t revision1,
              const char *url2,
              svn_revnum_t revision2,
              const char *target_abspath,
              svn_boolean_t sources_related,
              svn_boolean_t squelch_mergeinfo_notifications,
              notification_receiver_baton_t *notify_b,
              svn_boolean_t abort_on_conflicts,
              merge_cmd_baton_t *merge_b,
              apr_pool_t *scratch_pool)
{
  apr_hash_t *props1, *props2;
  const char *tmpfile1, *tmpfile2;
  const char *mimetype1, *mimetype2;
  svn_string_t *pval;
  apr_array_header_t *propchanges, *remaining_ranges;
  svn_wc_notify_state_t prop_state = svn_wc_notify_state_unknown;
  svn_wc_notify_state_t text_state = svn_wc_notify_state_unknown;
  svn_boolean_t tree_conflicted = FALSE;
  svn_client_ctx_t *ctx = merge_b->ctx;
  const char *mergeinfo_path;
  svn_merge_range_t range;
  svn_mergeinfo_t target_mergeinfo;
  svn_merge_range_t *conflicted_range = NULL;
  svn_boolean_t inherited = FALSE;
  svn_boolean_t is_rollback = (revision1 > revision2);
  const char *primary_url = is_rollback ? url1 : url2;
  const char *target_url;
  svn_boolean_t honor_mergeinfo, record_mergeinfo;
  svn_client__merge_path_t *merge_target = NULL;
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);

  SVN_ERR_ASSERT(svn_dirent_is_absolute(target_abspath));

  honor_mergeinfo = HONOR_MERGEINFO(merge_b);
  record_mergeinfo = RECORD_MERGEINFO(merge_b);

  /* Note that this is a single-file merge. */
  notify_b->is_single_file_merge = TRUE;

  SVN_ERR(svn_wc__node_get_url(&target_url, merge_b->ctx->wc_ctx,
                               target_abspath,
                               iterpool, iterpool));

  range.start = revision1;
  range.end = revision2;
  range.inheritable = TRUE;
  if (honor_mergeinfo)
    {
      svn_error_t *err;
      const char *source_root_url;
      merge_target = apr_pcalloc(scratch_pool, sizeof(*merge_target));
      merge_target->abspath = target_abspath;

      SVN_ERR(svn_ra_get_repos_root2(merge_b->ra_session1,
                                     &source_root_url, iterpool));
      SVN_ERR(svn_client__path_relative_to_root(&mergeinfo_path,
                                                merge_b->ctx->wc_ctx,
                                                primary_url,
                                                source_root_url, TRUE, NULL,
                                                scratch_pool, iterpool));

      /* Fetch mergeinfo (temporarily reparenting ra_session1 to
         working copy target URL). */
      SVN_ERR(svn_ra_reparent(merge_b->ra_session1, target_url,
                              iterpool));
      err = get_full_mergeinfo(&target_mergeinfo,
                               &(merge_target->implicit_mergeinfo),
                               &inherited, svn_mergeinfo_inherited,
                               merge_b->ra_session1, target_abspath,
                               MAX(revision1, revision2),
                               MIN(revision1, revision2),
                               ctx, scratch_pool, iterpool);

      if (err)
        {
          if (err->apr_err == SVN_ERR_MERGEINFO_PARSE_ERROR)
            {
              err = svn_error_createf(
                SVN_ERR_CLIENT_INVALID_MERGEINFO_NO_MERGETRACKING, err,
                _("Invalid mergeinfo detected on merge target '%s', "
                  "mergetracking not possible"),
                svn_dirent_local_style(target_abspath, scratch_pool));
            }
          return svn_error_trace(err);
        }

      SVN_ERR(svn_ra_reparent(merge_b->ra_session1, url1, iterpool));

      /* Calculate remaining merges unless this is a record only merge.
         In that case the remaining range is the whole range described
         by REVISION1:REVISION2. */
      if (!merge_b->record_only)
        {
          SVN_ERR(calculate_remaining_ranges(NULL, merge_target,
            source_root_url,
            url1, revision1, url2, revision2,
            target_mergeinfo,
            merge_b->implicit_src_gap, FALSE,
            merge_b->ra_session1,
            ctx, scratch_pool,
            iterpool));
          remaining_ranges = merge_target->remaining_ranges;
        }
    }

  /* The simple cases where our remaining range is REVISION1:REVISION2. */
  if (!honor_mergeinfo || merge_b->record_only)
    {
      remaining_ranges = apr_array_make(scratch_pool, 1, sizeof(&range));
      APR_ARRAY_PUSH(remaining_ranges, svn_merge_range_t *) = &range;
    }

  if (!merge_b->record_only)
    {
      apr_array_header_t *ranges_to_merge = remaining_ranges;
      int i;

      /* If we have ancestrally related sources and more than one
         range to merge, eliminate no-op ranges before going through
         the effort of downloading the many copies of the file
         required to do these merges (two copies per range). */
      if (merge_b->sources_ancestral && (remaining_ranges->nelts > 1))
        {
          const char *old_sess_url = NULL;
          SVN_ERR(svn_client__ensure_ra_session_url(&old_sess_url,
                                                    merge_b->ra_session1,
                                                    primary_url,
                                                    iterpool));
          SVN_ERR(remove_noop_merge_ranges(&ranges_to_merge,
                                           merge_b->ra_session1,
                                           remaining_ranges, scratch_pool));
          if (old_sess_url)
            SVN_ERR(svn_ra_reparent(merge_b->ra_session1, old_sess_url,
                                    iterpool));
        }

      for (i = 0; i < ranges_to_merge->nelts; i++)
        {
          svn_wc_notify_t *n;
          svn_boolean_t header_sent = FALSE;
          svn_ra_session_t *ra_session1, *ra_session2;

          /* When using this merge range, account for the exclusivity of
             its low value (which is indicated by this operation being a
             merge vs. revert). */
          svn_merge_range_t *r = APR_ARRAY_IDX(ranges_to_merge, i,
                                               svn_merge_range_t *);

          svn_pool_clear(iterpool);

          n = svn_wc_create_notify(target_abspath,
                                   merge_b->same_repos
                                     ? svn_wc_notify_merge_begin
                                     : svn_wc_notify_foreign_merge_begin,
                                   iterpool);
          if (merge_b->sources_ancestral)
            n->merge_range = r;

          /* Issue #3174: If we are honoring mergeinfo, then URL1, URL2,
             REVISION1, and REVISION2 meet the conditions described in
             'MERGEINFO MERGE SOURCE NORMALIZATION'.  This means that
             URL1@REVISION1 may be the copy source of URL2@REVISION2.
             If this is the case, then URL1 != URL2.  Since
             MERGE_B->RA_SESSION1 is always opened with URL1, the only time
             we can safely call single_file_merge_get_file() with that RA
             session is for REVISION1 (or REVISION2 if this is a reverse
             merge). */
          ra_session1 = merge_b->ra_session1;
          ra_session2 = merge_b->ra_session2;
          if (honor_mergeinfo && strcmp(url1, url2) != 0)
            {
              if (!is_rollback && r->start != revision1)
                ra_session1 = ra_session2; /* Use URL2's RA session. */
              else if (is_rollback && r->end != revision2)
                ra_session2 = ra_session1; /* Use URL1's RA session. */
            }

          /* While we currently don't allow it, in theory we could be
             fetching two fulltexts from two different repositories here. */
          SVN_ERR(single_file_merge_get_file(&tmpfile1, ra_session1,
                                             &props1, r->start,
                                             target_abspath, iterpool));
          SVN_ERR(single_file_merge_get_file(&tmpfile2, ra_session2,
                                             &props2, r->end, target_abspath,
                                             iterpool));

          /* Discover any svn:mime-type values in the proplists */
          pval = apr_hash_get(props1, SVN_PROP_MIME_TYPE,
                              strlen(SVN_PROP_MIME_TYPE));
          mimetype1 = pval ? pval->data : NULL;

          pval = apr_hash_get(props2, SVN_PROP_MIME_TYPE,
                              strlen(SVN_PROP_MIME_TYPE));
          mimetype2 = pval ? pval->data : NULL;

          /* Deduce property diffs. */
          SVN_ERR(svn_prop_diffs(&propchanges, props2, props1, iterpool));

          /* If we aren't ignoring ancestry, then we've already done
             ancestry relatedness checks.  If we are ignoring ancestry, or
             our sources are known to be related, then we can do
             text-n-props merge; otherwise, we have to do a delete-n-add
             merge.  */
          if (! (merge_b->ignore_ancestry || sources_related))
            {
              /* Delete... */
              SVN_ERR(merge_file_deleted(&text_state,
                                         &tree_conflicted,
                                         target_abspath,
                                         tmpfile1,
                                         tmpfile2,
                                         mimetype1, mimetype2,
                                         props1,
                                         merge_b,
                                         iterpool));
              single_file_merge_notify(notify_b, target_abspath,
                                       tree_conflicted
                                         ? svn_wc_notify_tree_conflict
                                         : svn_wc_notify_update_delete,
                                       text_state,
                                       svn_wc_notify_state_unknown,
                                       n, &header_sent, iterpool);

              /* ...plus add... */
              SVN_ERR(merge_file_added(&text_state, &prop_state,
                                       &tree_conflicted,
                                       target_abspath,
                                       tmpfile1,
                                       tmpfile2,
                                       r->start,
                                       r->end,
                                       mimetype1, mimetype2,
                                       NULL, SVN_INVALID_REVNUM,
                                       propchanges, props1,
                                       merge_b,
                                       iterpool));
              single_file_merge_notify(notify_b, target_abspath,
                                       tree_conflicted
                                         ? svn_wc_notify_tree_conflict
                                         : svn_wc_notify_update_add,
                                       text_state, prop_state, n,
                                       &header_sent, iterpool);
              /* ... equals replace. */
            }
          else
            {
              SVN_ERR(merge_file_changed(&text_state, &prop_state,
                                         &tree_conflicted,
                                         target_abspath,
                                         tmpfile1,
                                         tmpfile2,
                                         r->start,
                                         r->end,
                                         mimetype1, mimetype2,
                                         propchanges, props1,
                                         merge_b,
                                         iterpool));
              single_file_merge_notify(notify_b, target_abspath,
                                       tree_conflicted
                                         ? svn_wc_notify_tree_conflict
                                         : svn_wc_notify_update_update,
                                       text_state, prop_state, n,
                                       &header_sent, iterpool);
            }

          /* Ignore if temporary file not found. It may have been renamed. */
          /* (This is where we complain about missing Lisp, or better yet,
             Python...) */
          SVN_ERR(svn_io_remove_file2(tmpfile1, TRUE, iterpool));
          SVN_ERR(svn_io_remove_file2(tmpfile2, TRUE, iterpool));

          if ((i < (ranges_to_merge->nelts - 1) || abort_on_conflicts)
              && is_path_conflicted_by_merge(merge_b))
            {
              conflicted_range = svn_merge_range_dup(r, scratch_pool);
              /* Only record partial mergeinfo if only a partial merge was
                 performed before a conflict was encountered. */
              range.end = r->end;
              break;
            }
        }
    } /* !merge_b->record_only */

  /* Record updated WC mergeinfo to account for our new merges, minus
     any unresolved conflicts and skips.  We use the original
     REMAINING_RANGES here instead of the possibly-pared-down
     RANGES_TO_MERGE because we want to record all the requested
     merge ranges, include the noop ones.  */
  if (record_mergeinfo && remaining_ranges->nelts)
    {
      apr_array_header_t *filtered_rangelist;

      /* Filter any ranges from TARGET_WCPATH's own history, there is no
         need to record this explicitly in mergeinfo, it is already part
         of TARGET_WCPATH's natural history (implicit mergeinfo). */
      SVN_ERR(filter_natural_history_from_mergeinfo(
        &filtered_rangelist,
        mergeinfo_path,
        merge_target->implicit_mergeinfo,
        &range,
        iterpool));

      /* Only record mergeinfo if there is something other than
         self-referential mergeinfo, but don't record mergeinfo if
         TARGET_WCPATH was skipped. */
      if (filtered_rangelist->nelts
          && (!notify_b->skipped_abspaths
              || (apr_hash_count(notify_b->skipped_abspaths) == 0)))
        {
          apr_hash_t *merges = apr_hash_make(iterpool);

          /* If merge target has inherited mergeinfo set it before
             recording the first merge range. */
          if (inherited)
            SVN_ERR(svn_client__record_wc_mergeinfo(target_abspath,
                                                    target_mergeinfo,
                                                    FALSE, ctx,
                                                    iterpool));

          apr_hash_set(merges, target_abspath, APR_HASH_KEY_STRING,
                       filtered_rangelist);

          if (!squelch_mergeinfo_notifications)
            {
              /* Notify that we are recording mergeinfo describing a merge. */
              svn_wc_notify_t *notify = svn_wc_create_notify(
                target_abspath, svn_wc_notify_merge_record_info_begin,
                iterpool);
              svn_revnum_t youngest_rev;
              svn_revnum_t oldest_rev;
              SVN_ERR(svn_mergeinfo__get_range_endpoints(&youngest_rev,
                                                         &oldest_rev,
                                                         merges,
                                                         iterpool));
              notify->merge_range = apr_pcalloc(iterpool,
                                                sizeof(svn_merge_range_t));
              notify->merge_range->start = oldest_rev;
              notify->merge_range->end = youngest_rev;
              notify->merge_range->inheritable = TRUE;
              merge_b->ctx->notify_func2(merge_b->ctx->notify_baton2,
                                         notify, iterpool);
            }

          SVN_ERR(update_wc_mergeinfo(result_catalog, target_abspath,
                                      mergeinfo_path, merges, is_rollback,
                                      ctx, iterpool));
        }
    }

  /* Caller must call svn_sleep_for_timestamps() */
  *(merge_b->use_sleep) = TRUE;

  svn_pool_destroy(iterpool);

  /* If our multi-pass merge terminated early due to conflicts, return
     that fact as an error. */
  if (conflicted_range)
    return make_merge_conflict_error(target_abspath, conflicted_range,
                                     scratch_pool);

  return SVN_NO_ERROR;
}

/* Helper for do_directory_merge() to handle the case were a merge editor
   drive adds explicit mergeinfo to a path which didn't have any explicit
   mergeinfo previously.

   MERGE_B and NOTIFY_B are cascaded from the arguments of the same
   name in do_directory_merge().  Should be called only after
   do_directory_merge() has called populate_remaining_ranges() and populated
   the remaining_ranges field of each child in
   NOTIFY_B->CHILDREN_WITH_MERGEINFO (i.e. the remaining_ranges fields can be
   empty but never NULL).

   If MERGE_B->DRY_RUN is true do nothing, if it is false then
   for each path (if any) in MERGE_B->PATHS_WITH_NEW_MERGEINFO merge that
   path's inherited mergeinfo (if any) with its working explicit mergeinfo
   and set that as the path's new explicit mergeinfo.  Then add an
   svn_client__merge_path_t * element representing the path to
   NOTIFY_B->CHILDREN_WITH_MERGEINFO if it isn't already present.  All fields
   in any elements added to NOTIFY_B->CHILDREN_WITH_MERGEINFO are initialized
   to FALSE/NULL with the exception of 'path' and 'remaining_ranges'.  The
   latter is set to a rangelist equal to the remaining_ranges of the path's
   nearest path-wise ancestor in NOTIFY_B->CHILDREN_WITH_MERGEINFO.

   Any elements added to NOTIFY_B->CHILDREN_WITH_MERGEINFO are allocated
   in POOL. */
static svn_error_t *
process_children_with_new_mergeinfo(merge_cmd_baton_t *merge_b,
                                    notification_receiver_baton_t *notify_b,
                                    apr_pool_t *pool)
{
  apr_pool_t *iterpool;
  apr_hash_index_t *hi;

  if (!merge_b->paths_with_new_mergeinfo || merge_b->dry_run)
    return SVN_NO_ERROR;

  /* Iterate over each path with explicit mergeinfo added by the merge. */
  iterpool = svn_pool_create(pool);
  for (hi = apr_hash_first(pool, merge_b->paths_with_new_mergeinfo);
       hi;
       hi = apr_hash_next(hi))
    {
      const char *abspath_with_new_mergeinfo = svn__apr_hash_index_key(hi);
      const char *old_session_url = NULL;
      const char *path_url;
      svn_mergeinfo_t path_inherited_mergeinfo;
      svn_mergeinfo_t path_explicit_mergeinfo;
      svn_boolean_t inherited;
      svn_client__merge_path_t *new_child;

      apr_pool_clear(iterpool);
      SVN_ERR(svn_wc__node_get_url(&path_url, merge_b->ctx->wc_ctx,
                                   abspath_with_new_mergeinfo,
                                   pool, pool));

      /* Get the path's new explicit mergeinfo... */
      SVN_ERR(svn_client__get_wc_mergeinfo(&path_explicit_mergeinfo,
                                           &inherited,
                                           svn_mergeinfo_explicit,
                                           abspath_with_new_mergeinfo,
                                           NULL, NULL, FALSE,
                                           merge_b->ctx,
                                           iterpool, iterpool));
      /* ...there *should* always be explicit mergeinfo at this point
         but you can't be too careful. */
      if (path_explicit_mergeinfo)
        {
          /* Temporarily reparent MERGE_B->RA_SESSION2 in case we need to
             contact the repository for inherited mergeinfo. */
          SVN_ERR(svn_client__ensure_ra_session_url(&old_session_url,
                                                    merge_b->ra_session2,
                                                    path_url,
                                                    iterpool));
          /* Get the mergeinfo the path would have inherited before
             the merge. */
          SVN_ERR(svn_client__get_wc_or_repos_mergeinfo(
            &path_inherited_mergeinfo,
            &inherited, NULL,
            FALSE,
            svn_mergeinfo_nearest_ancestor, /* We only want inherited MI */
            merge_b->ra_session2,
            abspath_with_new_mergeinfo,
            merge_b->ctx,
            iterpool));

          /* If the path inherited any mergeinfo then merge that with the
             explicit mergeinfo and record the result as the path's new
             explicit mergeinfo. */
          if (path_inherited_mergeinfo)
            {
              SVN_ERR(svn_mergeinfo_merge(path_explicit_mergeinfo,
                                          path_inherited_mergeinfo,
                                          iterpool));
              SVN_ERR(svn_client__record_wc_mergeinfo(
                                          abspath_with_new_mergeinfo,
                                          path_explicit_mergeinfo,
                                          FALSE, merge_b->ctx, iterpool));
            }

          /* If the path is not in NOTIFY_B->CHILDREN_WITH_MERGEINFO
             then add it. */
          new_child =
            get_child_with_mergeinfo(notify_b->children_with_mergeinfo,
                                     abspath_with_new_mergeinfo);
          if (!new_child)
            {
              int parent_index =
                find_nearest_ancestor(notify_b->children_with_mergeinfo,
                                      FALSE, abspath_with_new_mergeinfo);
              svn_client__merge_path_t *parent =
                APR_ARRAY_IDX(notify_b->children_with_mergeinfo,
                              parent_index,
                              svn_client__merge_path_t *);
              new_child = apr_pcalloc(pool, sizeof(*new_child));
              new_child->abspath = apr_pstrdup(pool,
                                               abspath_with_new_mergeinfo);

              /* If path_with_new_mergeinfo is the merge target itself
                 then it should already be in
                 NOTIFY_B->CHILDREN_WITH_MERGEINFO per the criteria of
                 get_mergeinfo_paths() and we shouldn't be in this block.
                 If path_with_new_mergeinfo is a subtree then it must have
                 a parent in NOTIFY_B->CHILDREN_WITH_MERGEINFO if only
                 the merge target itself...so if we don't find a parent
                 the caller has done something quite wrong. */
              SVN_ERR_ASSERT(parent);
              SVN_ERR_ASSERT(parent->remaining_ranges);

              /* Set the path's remaining_ranges equal to its parent's. */
              new_child->remaining_ranges = svn_rangelist_dup(
                 parent->remaining_ranges, pool);
              insert_child_to_merge(notify_b->children_with_mergeinfo,
                                    new_child, pool);
            }
        }
      /* Return MERGE_B->RA_SESSION2 to its initial state if we
         reparented it. */
      if (old_session_url)
        SVN_ERR(svn_ra_reparent(merge_b->ra_session2, old_session_url,
                                iterpool));
    }
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

/* SUBTREES is one of the following notification_receiver_baton_t members:
   merged_paths, skipped_paths, or added_paths.  Return true if any path in
   SUBTRESS is equal to, or is a subtree of, LOCAL_ABSPATH.  Return false
   otherwise.  If LOCAL_ABSPATH or SUBTREES are NULL return false. */
static svn_boolean_t
path_is_subtree(const char *local_abspath,
                apr_hash_t *subtrees,
                apr_pool_t *pool)
{
  if (local_abspath && subtrees)
    {
      apr_hash_index_t *hi;

      for (hi = apr_hash_first(pool, subtrees);
           hi;
           hi = apr_hash_next(hi))
        {
          const char *path_touched_by_merge = svn__apr_hash_index_key(hi);
          if (svn_dirent_is_ancestor(local_abspath, path_touched_by_merge))
            return TRUE;
        }
    }
  return FALSE;
}

/* Return true if LOCAL_ABDPATH is equal to or a subtree of any of the paths
   in NOTIFY_B->MERGED_PATHS, NOTIFY_B->SKIPPED_PATHS, NOTIFY_B->ADDED_PATHS,
   or NOTIFY_B->CONFLICTED_PATHS.  Return false otherwise. */
static svn_boolean_t
subtree_touched_by_merge(const char *local_abspath,
                         notification_receiver_baton_t *notify_b,
                         apr_pool_t *pool)
{
  return (path_is_subtree(local_abspath, notify_b->merged_abspaths, pool)
          || path_is_subtree(local_abspath, notify_b->skipped_abspaths, pool)
          || path_is_subtree(local_abspath, notify_b->added_abspaths, pool)
          || path_is_subtree(local_abspath,
                             notify_b->tree_conflicted_abspaths,
                             pool));
}

/* Helper for do_directory_merge() when performing mergeinfo unaware merges.

   Merge the diff of URL1@REVISION1:URL2@REVISION2 to TARGET_DIR_WCPATH.

   URL1, REVISION1, URL2, REVISION2, DEPTH, NOTIFY_B, and MERGE_B
   are all cascaded from do_directory_merge's arguments of the same names.

   NOTE: This is a very thin wrapper around drive_merge_report_editor() and
   exists only to populate NOTIFY_B->CHILREN_WITH_MERGEINFO with the single
   element expected during mergeinfo unaware merges..
*/
static svn_error_t *
do_mergeinfo_unaware_dir_merge(const char *url1,
                               svn_revnum_t revision1,
                               const char *url2,
                               svn_revnum_t revision2,
                               const char *target_dir_wcpath,
                               svn_depth_t depth,
                               notification_receiver_baton_t *notify_b,
                               merge_cmd_baton_t *merge_b,
                               apr_pool_t *pool)
{
  /* Initialize NOTIFY_B->CHILDREN_WITH_MERGEINFO and populate it with
     one element describing the merge of REVISION1:REVISION2 to
     TARGET_DIR_WCPATH. */
  svn_client__merge_path_t *item = apr_pcalloc(pool, sizeof(*item));

  item->abspath = apr_pstrdup(pool, target_dir_wcpath);
  item->remaining_ranges = svn_rangelist__initialize(revision1, revision2,
                                                     TRUE, pool);
  APR_ARRAY_PUSH(notify_b->children_with_mergeinfo,
                 svn_client__merge_path_t *) = item;
  return drive_merge_report_editor(target_dir_wcpath,
                                   url1, revision1, url2, revision2,
                                   NULL, depth, notify_b,
                                   merge_b, pool);
}

/* A svn_log_entry_receiver_t callback for
   get_inoperative_immediate_children(). */
static svn_error_t *
log_find_operative_subtree_revs(void *baton,
                                svn_log_entry_t *log_entry,
                                apr_pool_t *pool)
{
  apr_hash_t *immediate_children = baton;
  apr_hash_index_t *hi, *hi2;

  for (hi = apr_hash_first(pool, log_entry->changed_paths2);
       hi;
       hi = apr_hash_next(hi))
    {
      const char *path = svn__apr_hash_index_key(hi);
      for (hi2 = apr_hash_first(pool, immediate_children);
           hi2;
           hi2 = apr_hash_next(hi2))
        {
          const char *immediate_path = svn__apr_hash_index_val(hi2);
          if (svn_dirent_is_ancestor(immediate_path, path))
            {
              apr_hash_set(immediate_children, svn__apr_hash_index_key(hi2),
                           APR_HASH_KEY_STRING, NULL);
              /* A path can't be the child of more than
                 one immediate child of the merge target. */
              break;
            }
      }
    }
  return SVN_NO_ERROR;
}

/* Issue #3642.

   Find inoperative subtrees when record_mergeinfo_for_dir_merge() is
   recording mergeinfo describing a merge at depth=immediates.

   CHILDREN_WITH_MERGEINFO is the standard array of subtrees that are
   interesting from a merge tracking perspective, see the global comment
   'THE CHILDREN_WITH_MERGEINFO ARRAY'.

   MERGE_SOURCE_REPOS_ABSPATH is the absolute repository path of the merge
   source.  OLDEST_REV and YOUNGEST_REV are the revisions merged from
   MERGE_SOURCE_REPOS_ABSPATH to MERGE_TARGET_ABSPATH.

   RA_SESSION points to MERGE_SOURCE_REPOS_ABSPATH.

   Set *IMMEDIATE_CHILDREN to a hash (mapping const char * WC absolute
   paths to const char * repos absolute paths) containing all the
   subtrees which would be inoperative if MERGE_SOURCE_REPOS_PATH
   -r(OLDEST_REV - 1):YOUNGEST_REV were merged to MERGE_TARGET_ABSPATH
   at --depth infinity.  The keys of the hash point to copies of the ABSPATH
   members of the svn_client__merge_path_t * in CHILDREN_WITH_MERGEINFO that
   are inoperative.  The hash values are the key's corresponding repository
   absolute merge source path.

   RESULT_POOL is used to allocate the contents of *IMMEDIATE_CHILDREN.
   SCRATCH_POOL is used for temporary allocations. */
static svn_error_t *
get_inoperative_immediate_children(apr_hash_t **immediate_children,
                                   apr_array_header_t *children_with_mergeinfo,
                                   const char *merge_source_repos_abspath,
                                   svn_revnum_t oldest_rev,
                                   svn_revnum_t youngest_rev,
                                   const char *merge_target_abspath,
                                   svn_ra_session_t *ra_session,
                                   apr_pool_t *result_pool,
                                   apr_pool_t *scratch_pool)
{
  int i;
  apr_pool_t *iterpool;

  SVN_ERR_ASSERT(SVN_IS_VALID_REVNUM(oldest_rev));
  SVN_ERR_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  SVN_ERR_ASSERT(oldest_rev <= youngest_rev);

  *immediate_children = apr_hash_make(result_pool);
  iterpool = svn_pool_create(scratch_pool);

  /* Find all the children in CHILDREN_WITH_MERGEINFO with the
     immediate_child_dir flag set and store them in *IMMEDIATE_CHILDREN. */
  for (i = 0; i < children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
        APR_ARRAY_IDX(children_with_mergeinfo, i, svn_client__merge_path_t *);

      svn_pool_clear(iterpool);

      if (child->immediate_child_dir)
        apr_hash_set(*immediate_children,
                     apr_pstrdup(result_pool, child->abspath),
                     APR_HASH_KEY_STRING,
                     svn_fspath__join(merge_source_repos_abspath,
                                      svn_dirent_is_child(merge_target_abspath,
                                                          child->abspath,
                                                          iterpool),
                                      result_pool));
    }

  svn_pool_destroy(iterpool);

  /* Now remove any paths from *IMMEDIATE_CHILDREN that are inoperative when
     merging MERGE_SOURCE_REPOS_PATH -r(OLDEST_REV - 1):YOUNGEST_REV to
     MERGE_TARGET_ABSPATH at --depth infinity. */
  if (apr_hash_count(*immediate_children))
    {
      apr_array_header_t *log_targets = apr_array_make(scratch_pool, 1,
                                                       sizeof(const char *));
      APR_ARRAY_PUSH(log_targets, const char *) = "";
      SVN_ERR(svn_ra_get_log2(ra_session, log_targets, youngest_rev,
                              oldest_rev, 0, TRUE, FALSE, FALSE,
                              NULL, log_find_operative_subtree_revs,
                              *immediate_children, scratch_pool));
    }
  return SVN_NO_ERROR;
}

/* Helper for do_directory_merge().

   If RESULT_CATALOG is NULL then record mergeinfo describing a merge of
   MERGED_RANGE->START:MERGED_RANGE->END from the repository relative path
   MERGEINFO_PATH to the merge target (and possibly its subtrees) described
   by NOTIFY_B->CHILDREN_WITH_MERGEINFO -- see the global comment
   'THE CHILDREN_WITH_MERGEINFO ARRAY'.  Obviously this should only
   be called if recording mergeinfo -- see doc string for RECORD_MERGEINFO().

   If RESULT_CATALOG is not NULL, then don't record the new mergeinfo on the
   WC, but instead record it in RESULT_CATALOG, where the keys are absolute
   working copy paths and the values are the new mergeinfos for each.
   Allocate additions to RESULT_CATALOG in pool which RESULT_CATALOG was
   created in.

   DEPTH, NOTIFY_B, MERGE_B, and SQUELCH_MERGEINFO_NOTIFICATIONS are all
   cascaded from do_directory_merge's arguments of the same names.
*/
static svn_error_t *
record_mergeinfo_for_dir_merge(svn_mergeinfo_catalog_t result_catalog,
                               const svn_merge_range_t *merged_range,
                               const char *mergeinfo_path,
                               svn_depth_t depth,
                               svn_boolean_t squelch_mergeinfo_notifications,
                               notification_receiver_baton_t *notify_b,
                               merge_cmd_baton_t *merge_b,
                               apr_pool_t *pool)
{
  int i;
  svn_boolean_t is_rollback = (merged_range->start > merged_range->end);
  svn_boolean_t operative_merge = FALSE;
  apr_hash_t *inoperative_immediate_children = NULL;

  /* Update the WC mergeinfo here to account for our new
     merges, minus any unresolved conflicts and skips. */

  /* We need a scratch pool for iterations below. */
  apr_pool_t *iterpool = svn_pool_create(pool);

  svn_merge_range_t range;

  range.start = merged_range->start;
  range.end = merged_range->end;
  range.inheritable = merged_range->inheritable;

  /* Regardless of what subtrees in MERGE_B->TARGET_ABSPATH might be missing
     could this merge have been operative? */
  if ((notify_b->merged_abspaths
       && apr_hash_count(notify_b->merged_abspaths))
      || (notify_b->skipped_abspaths
          && apr_hash_count(notify_b->skipped_abspaths))
      || (notify_b->added_abspaths
          && apr_hash_count(notify_b->added_abspaths))
      || (notify_b->tree_conflicted_abspaths
          && apr_hash_count(notify_b->tree_conflicted_abspaths)))
    operative_merge = TRUE;

  /* If this couldn't be an operative merge then don't bother with
     the added complexity (and user confusion) of non-inheritable ranges.
     There is no harm in subtrees inheriting inoperative mergeinfo. */
  if (!operative_merge)
    range.inheritable = TRUE;

  /* Remove absent children at or under TARGET_WCPATH from
     NOTIFY_B->CHILDREN_WITH_MERGEINFO
     before we calculate the merges performed. */
  remove_absent_children(merge_b->target_abspath,
                         notify_b->children_with_mergeinfo, notify_b);


  /* Find all issue #3642 children (i.e immediate child directories of the
     merge target, with no pre-existing explicit mergeinfo, during a --depth
     immediates merge).  Stash those that are inoperative at any depth in
     INOPERATIVE_IMMEDIATE_CHILDREN. */
  if (!merge_b->record_only && range.start <= range.end
      && depth == svn_depth_immediates)
    SVN_ERR(get_inoperative_immediate_children(
      &inoperative_immediate_children,
      notify_b->children_with_mergeinfo,
      mergeinfo_path, range.start + 1, range.end,
      merge_b->target_abspath, merge_b->ra_session1,
      pool, iterpool));

  /* Record mergeinfo on any subtree affected by the merge or for
     every subtree if this is a --record-only merge.  Always record
     mergeinfo on the merge target CHILDREN_WITH_MERGEINFO[0]. */
  for (i = 0; i < notify_b->children_with_mergeinfo->nelts; i++)
    {
      const char *child_repos_path;
      const char *child_merge_src_canon_path;
      apr_array_header_t *child_merge_rangelist;
      apr_hash_t *child_merges;
      svn_client__merge_path_t *child =
                     APR_ARRAY_IDX(notify_b->children_with_mergeinfo, i,
                                   svn_client__merge_path_t *);
      SVN_ERR_ASSERT(child);

      /* Can't record mereginfo on something that isn't here. */
      if (child->absent)
        continue;

      svn_pool_clear(iterpool);

      /* If CHILD is a subtree, this is not a record only merge, and
         CHILD was not affected by the merge then we don't need to
         record mergeinfo.  If this is a record only merge being done
         as part of a reintegrate merge then we need to check if CHILD
         was affected by the merge. */
      if (i > 0 /* Always record mergeinfo on the merge target. */
          && (!merge_b->record_only || merge_b->reintegrate_merge)
          && (!child->immediate_child_dir || child->pre_merge_mergeinfo)
          && (!operative_merge
              || !subtree_touched_by_merge(child->abspath, notify_b,
                                           iterpool)))
        {
          /* If CHILD is in NOTIFY_B->CHILDREN_WITH_MERGEINFO simply
             because it had no explicit mergeinfo of its own at the
             start of the merge but is the child of of some path with
             non-inheritable mergeinfo, then the explicit mergeinfo it
             has *now* was set by get_mergeinfo_paths() -- see criteria
             3 in that function's doc string.  So since CHILD->ABSPATH
             was not touched by the merge we can remove the
             mergeinfo. */
          if (child->child_of_noninheritable)
            {
              SVN_ERR(svn_client__record_wc_mergeinfo(child->abspath,
                                                      NULL, FALSE,
                                                      merge_b->ctx,
                                                      iterpool));
            }
        }
      else /* Record mergeinfo on CHILD. */
        {
          svn_boolean_t child_is_deleted;
          svn_boolean_t rangelist_inheritance;

          /* If CHILD is deleted we don't need to set mergeinfo on it. */
          SVN_ERR(svn_wc__node_is_status_deleted(&child_is_deleted,
                                                 merge_b->ctx->wc_ctx,
                                                 child->abspath, iterpool));
          if (child_is_deleted)
            continue;

          /* We don't need to record mergeinfo on those issue #3642 children
             that are inoperative at any depth. */
          if (inoperative_immediate_children
              && apr_hash_get(inoperative_immediate_children,
                              child->abspath,
                             APR_HASH_KEY_STRING))
            continue;

          child_repos_path = svn_dirent_is_child(merge_b->target_abspath,
                                                 child->abspath, iterpool);
          if (!child_repos_path)
            child_repos_path = "";

          child_merge_src_canon_path = svn_fspath__join(mergeinfo_path,
                                                        child_repos_path,
                                                        iterpool);
          /* Filter any ranges from each child's natural history before
             setting mergeinfo describing the merge. */
          SVN_ERR(filter_natural_history_from_mergeinfo(
            &child_merge_rangelist, child_merge_src_canon_path,
            child->implicit_mergeinfo, &range, iterpool));

          if (child_merge_rangelist->nelts == 0)
            continue;

          if (!squelch_mergeinfo_notifications)
            {
              svn_wc_notify_t *notify = svn_wc_create_notify(
                child->abspath,
                svn_wc_notify_merge_record_info_begin,
                iterpool);
              notify->merge_range = svn_merge_range_dup(merged_range,
                                                        iterpool);
              merge_b->ctx->notify_func2(merge_b->ctx->notify_baton2, notify,
                                         iterpool);
            }

          /* If we are here we know we will be recording some mergeinfo, but
             before we do set override mergeinfo on skipped paths so they
             don't incorrectly inherit the mergeinfo we are about to set.
             We only need to do this once.  If we are dealing with a subtree
             (i.e. i != 0) that was skipped then don't record mergeinfo on
             it.  The earlier call to record_skips will already have taken
             care of this. */
          if (i == 0)
            SVN_ERR(record_skips(mergeinfo_path,
                                 child_merge_rangelist,
                                 is_rollback, notify_b, merge_b, iterpool));
          else if (notify_b->skipped_abspaths
                   && apr_hash_get(notify_b->skipped_abspaths, child->abspath,
                                   APR_HASH_KEY_STRING))
            continue;

          SVN_ERR(calculate_merge_inheritance(child_merge_rangelist,
                                              &rangelist_inheritance,
                                              child->abspath,
                                              i == 0,
                                              child->missing_child,
                                              depth,
                                              merge_b->ctx->wc_ctx,
                                              iterpool));

          /* If CHILD has inherited mergeinfo set it before
             recording the first merge range. */
          if (child->inherited_mergeinfo)
            SVN_ERR(svn_client__record_wc_mergeinfo(
              child->abspath,
              child->pre_merge_mergeinfo,
              FALSE, merge_b->ctx,
              iterpool));
          if (merge_b->implicit_src_gap)
            {
              /* If this is a reverse merge reorder CHILD->REMAINING_RANGES
                 so it will work with the svn_rangelist_remove API. */
              if (is_rollback)
                SVN_ERR(svn_rangelist_reverse(child_merge_rangelist,
                                              iterpool));

              SVN_ERR(svn_rangelist_remove(&child_merge_rangelist,
                                           merge_b->implicit_src_gap,
                                           child_merge_rangelist, FALSE,
                                           iterpool));
              if (is_rollback)
                SVN_ERR(svn_rangelist_reverse(child_merge_rangelist,
                                              iterpool));
            }

          child_merges = apr_hash_make(iterpool);

          /* The short story:

             If we are describing a forward merge, then the naive mergeinfo
             defined by MERGE_SOURCE_PATH:MERGED_RANGE->START:
             MERGE_SOURCE_PATH:MERGED_RANGE->END may contain non-existent
             path-revs or may describe other lines of history.  We must
             remove these invalid portion(s) before recording mergeinfo
             describing the merge.

             The long story:

             If CHILD is the merge target we know that
             MERGE_SOURCE_PATH:MERGED_RANGE->END exists.  Further, if there
             were no copies in MERGE_SOURCE_PATH's history going back to
             RANGE->START then we know that
             MERGE_SOURCE_PATH:MERGED_RANGE->START exists too and the two
             describe and unbroken line of history and thus
             MERGE_SOURCE_PATH:MERGED_RANGE->START:
             MERGE_SOURCE_PATH:MERGED_RANGE->END is a valid description of
             the merge -- see normalize_merge_sources() and the global comment
             'MERGEINFO MERGE SOURCE NORMALIZATION'.

             However, if there *was* a copy, then
             MERGE_SOURCE_PATH:MERGED_RANGE->START doesn't exist or is
             unrelated to MERGE_SOURCE_PATH:MERGED_RANGE->END.  Also, we
             don't know if (MERGE_SOURCE_PATH:MERGED_RANGE->START)+1 through
             (MERGE_SOURCE_PATH:MERGED_RANGE->END)-1 actually exist.

             If CHILD is a subtree of the merge target, then nothing is
             guaranteed beyond the fact that MERGE_SOURCE_PATH exists at
             MERGED_RANGE->END. */
          if ((!merge_b->record_only || merge_b->reintegrate_merge)
              && (!is_rollback))
            {
              svn_error_t *err;
              svn_opt_revision_t peg_revision;
              svn_mergeinfo_t subtree_history_as_mergeinfo;
              apr_array_header_t *child_merge_src_rangelist;
              const char *old_session_url;
              const char *subtree_mergeinfo_url =
                svn_path_url_add_component2(merge_b->repos_root_url,
                                            child_merge_src_canon_path + 1,
                                            iterpool);

              /* Confirm that the naive mergeinfo we want to set on
                 CHILD->ABSPATH both exists and is part of
                 (MERGE_SOURCE_PATH+CHILD_REPOS_PATH)@MERGED_RANGE->END's
                 history. */
              peg_revision.kind = svn_opt_revision_number;

              /* We know MERGED_RANGE->END is younger than MERGE_RANGE->START
                 because we only do this for forward merges. */
              peg_revision.value.number = merged_range->end;
              SVN_ERR(svn_client__ensure_ra_session_url(&old_session_url,
                                                        merge_b->ra_session2,
                                                        subtree_mergeinfo_url,
                                                        iterpool));
              err = svn_client__get_history_as_mergeinfo(
                &subtree_history_as_mergeinfo, NULL,
                subtree_mergeinfo_url, &peg_revision,
                MAX(merged_range->start, merged_range->end),
                MIN(merged_range->start, merged_range->end),
                merge_b->ra_session2, merge_b->ctx, iterpool);

              /* If CHILD is a subtree it may have been deleted prior to
                 MERGED_RANGE->END so the above call to get its history
                 will fail. */
              if (err)
                {
                  if (err->apr_err != SVN_ERR_FS_NOT_FOUND)
                      return svn_error_trace(err);
                  svn_error_clear(err);
                }
              else
                {
                  child_merge_src_rangelist = apr_hash_get(
                    subtree_history_as_mergeinfo,
                    child_merge_src_canon_path,
                    APR_HASH_KEY_STRING);
                  SVN_ERR(svn_rangelist_intersect(&child_merge_rangelist,
                                                  child_merge_rangelist,
                                                  child_merge_src_rangelist,
                                                  FALSE, iterpool));
                  if (!rangelist_inheritance)
                    svn_rangelist__set_inheritance(child_merge_rangelist,
                                                   FALSE);
                }

              if (old_session_url)
                SVN_ERR(svn_ra_reparent(merge_b->ra_session2,
                                        old_session_url, iterpool));
            }

          apr_hash_set(child_merges, child->abspath, APR_HASH_KEY_STRING,
                       child_merge_rangelist);
          SVN_ERR(update_wc_mergeinfo(result_catalog,
                                      child->abspath,
                                      child_merge_src_canon_path,
                                      child_merges, is_rollback,
                                      merge_b->ctx, iterpool));
        }

      /* Elide explicit subtree mergeinfo whether or not we updated it. */
      if (i > 0)
        {
          svn_boolean_t in_switched_subtree = FALSE;

          if (child->switched)
            in_switched_subtree = TRUE;
          else if (i > 1)
            {
              /* Check if CHILD is part of a switched subtree */
              svn_client__merge_path_t *parent;
              int j = i - 1;
              for (; j > 0; j--)
                {
                  parent = APR_ARRAY_IDX(notify_b->children_with_mergeinfo,
                                         j, svn_client__merge_path_t *);
                  if (parent
                      && parent->switched
                      && svn_dirent_is_ancestor(parent->abspath,
                                                child->abspath))
                    {
                      in_switched_subtree = TRUE;
                      break;
                    }
                }
            }

          /* Allow mergeinfo on switched subtrees to elide to the
             repository. Otherwise limit elision to the merge target
             for now.  do_directory_merge() will eventually try to
             elide that when the merge is complete. */
          SVN_ERR(svn_client__elide_mergeinfo(
            child->abspath,
            in_switched_subtree ? NULL : merge_b->target_abspath,
            merge_b->ctx, iterpool));
        }
    } /* (i = 0; i < notify_b->children_with_mergeinfo->nelts; i++) */

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

/* Helper for do_directory_merge().

   Record mergeinfo describing a merge of
   MERGED_RANGE->START:MERGED_RANGE->END from the repository relative path
   MERGEINFO_PATH to each path in NOTIFY_B->ADDED_ABSPATHS which has explicit
   mergeinfo or is the immediate child of a parent with explicit
   non-inheritable mergeinfo.

   DEPTH, NOTIFY_B, MERGE_B, and SQUELCH_MERGEINFO_NOTIFICATIONS, are
   cascaded from do_directory_merge's arguments of the same names.

   Note: This is intended to support forward merges only, i.e.
   MERGED_RANGE->START must be older than MERGED_RANGE->END.
*/
static svn_error_t *
record_mergeinfo_for_added_subtrees(
  svn_merge_range_t *merged_range,
  const char *mergeinfo_path,
  svn_depth_t depth,
  svn_boolean_t squelch_mergeinfo_notifications,
  notification_receiver_baton_t *notify_b,
  merge_cmd_baton_t *merge_b,
  apr_pool_t *pool)
{
  apr_pool_t *iterpool;
  apr_hash_index_t *hi;

  /* If no paths were added by the merge then we have nothing to do. */
  if (!notify_b->added_abspaths)
    return SVN_NO_ERROR;

  SVN_ERR_ASSERT(merged_range->start < merged_range->end);

  iterpool = svn_pool_create(pool);
  for (hi = apr_hash_first(pool, notify_b->added_abspaths); hi;
       hi = apr_hash_next(hi))
    {
      const char *added_abspath = svn__apr_hash_index_key(hi);
      const char *dir_abspath;
      svn_mergeinfo_t parent_mergeinfo;
      svn_mergeinfo_t added_path_mergeinfo;
      svn_boolean_t inherited; /* used multiple times, but ignored */

      apr_pool_clear(iterpool);
      dir_abspath = svn_dirent_dirname(added_abspath, iterpool);

      /* Grab the added path's explicit mergeinfo. */
      SVN_ERR(svn_client__get_wc_mergeinfo(&added_path_mergeinfo, &inherited,
                                           svn_mergeinfo_explicit,
                                           added_abspath, NULL, NULL, FALSE,
                                           merge_b->ctx, iterpool, iterpool));

      /* If the added path doesn't have explicit mergeinfo, does its immediate
         parent have non-inheritable mergeinfo? */
      if (!added_path_mergeinfo)
        SVN_ERR(svn_client__get_wc_mergeinfo(&parent_mergeinfo, &inherited,
                                             svn_mergeinfo_explicit,
                                             dir_abspath, NULL, NULL, FALSE,
                                             merge_b->ctx,
                                             iterpool, iterpool));

      if (added_path_mergeinfo
          || svn_mergeinfo__is_noninheritable(parent_mergeinfo, iterpool))
        {
          svn_client__merge_path_t *target_merge_path =
            APR_ARRAY_IDX(notify_b->children_with_mergeinfo, 0,
                          svn_client__merge_path_t *);
          svn_merge_range_t *rng;
          svn_node_kind_t added_path_kind;
          svn_mergeinfo_t merge_mergeinfo;
          svn_mergeinfo_t adds_history_as_mergeinfo;
          apr_array_header_t *rangelist;
          const char *rel_added_path;
          const char *added_path_mergeinfo_path;
          const char *old_session_url;
          const char *added_path_mergeinfo_url;
          svn_opt_revision_t peg_revision;

          SVN_ERR(svn_wc_read_kind(&added_path_kind, merge_b->ctx->wc_ctx,
                                   added_abspath, FALSE, iterpool));

          /* Calculate the naive mergeinfo describing the merge. */
          merge_mergeinfo = apr_hash_make(iterpool);
          rangelist = apr_array_make(iterpool, 1, sizeof(svn_merge_range_t *));
          rng = svn_merge_range_dup(merged_range, iterpool);
          if (added_path_kind == svn_node_file)
            rng->inheritable = TRUE;
          else
            rng->inheritable =
              (!(depth == svn_depth_infinity
                 || depth == svn_depth_immediates));
          APR_ARRAY_PUSH(rangelist, svn_merge_range_t *) = rng;

          /* Create the new mergeinfo path for
             added_path's mergeinfo. */

          /* abs_added_path had better be a child of abs_target_path
             or something is *really* wrong. */

          rel_added_path = svn_dirent_is_child(target_merge_path->abspath,
                                               added_abspath, iterpool);
          SVN_ERR_ASSERT(rel_added_path);
          added_path_mergeinfo_path = svn_dirent_join(mergeinfo_path,
                                                      rel_added_path,
                                                      iterpool);
          apr_hash_set(merge_mergeinfo, added_path_mergeinfo_path,
                       APR_HASH_KEY_STRING, rangelist);

          /* Don't add new mergeinfo to describe the merge if that mergeinfo
             contains non-existent merge sources.

             We know that MERGEINFO_PATH/rel_added_path's history does not
             span MERGED_RANGE->START:MERGED_RANGE->END but rather that it
             was added at some revions greater than MERGED_RANGE->START
             (assuming this is a forward merge).  It may have been added,
             deleted, and re-added many times.  The point is that we cannot
             blindly apply the naive mergeinfo calculated above because it
             will describe non-existent merge sources. To avoid this we get
             take the intersection of the naive mergeinfo with
             MERGEINFO_PATH/rel_added_path's history. */
          added_path_mergeinfo_url =
            svn_path_url_add_component2(merge_b->repos_root_url,
                                        added_path_mergeinfo_path + 1,
                                        iterpool);
          peg_revision.kind = svn_opt_revision_number;
          peg_revision.value.number = MAX(merged_range->start,
                                          merged_range->end);
          SVN_ERR(svn_client__ensure_ra_session_url(
            &old_session_url, merge_b->ra_session2,
            added_path_mergeinfo_url, iterpool));
          SVN_ERR(svn_client__get_history_as_mergeinfo(
            &adds_history_as_mergeinfo, NULL,
            added_path_mergeinfo_url, &peg_revision,
            MAX(merged_range->start, merged_range->end),
            MIN(merged_range->start, merged_range->end),
            merge_b->ra_session2, merge_b->ctx, iterpool));

          if (old_session_url)
            SVN_ERR(svn_ra_reparent(merge_b->ra_session2,
                                    old_session_url, iterpool));

          SVN_ERR(svn_mergeinfo_intersect2(&merge_mergeinfo,
                                           merge_mergeinfo,
                                           adds_history_as_mergeinfo,
                                           FALSE, iterpool, iterpool));

          /* Combine the explict mergeinfo on the added path (if any)
             with the mergeinfo describing this merge. */
          if (added_path_mergeinfo)
            SVN_ERR(svn_mergeinfo_merge(merge_mergeinfo, added_path_mergeinfo,
                                        iterpool));
          SVN_ERR(svn_client__record_wc_mergeinfo(
            added_abspath, merge_mergeinfo,
            !squelch_mergeinfo_notifications, merge_b->ctx, iterpool));
        }
    }
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}
/* Baton structure for log_noop_revs. */
typedef struct log_noop_baton_t
{
  merge_cmd_baton_t *merge_b;

  /* See the comment 'THE CHILDREN_WITH_MERGEINFO ARRAY' at the start
     of this file.*/
  apr_array_header_t *children_with_mergeinfo;

  /* Absolute repository path of MERGE_B->TARGET_ABSPATH. */
  const char *target_repos_abs;

  /* Absolute repository path of younger of the two merge sources
     being diffed. */
  const char *source_repos_abs;

  /* Initially empty rangelists allocated in POOL. The rangelists are
   * populated across multiple invocations of the log_noop_revs(). */
  apr_array_header_t *operative_ranges;
  apr_array_header_t *merged_ranges;

  /* Pool to store the rangelists. */
  apr_pool_t *pool;
} log_noop_baton_t;

/* Helper for log_noop_revs: Merge a svn_merge_range_t representation of
   REVISION into RANGELIST. New elements added to rangelist are allocated
   in RESULT_POOL.

   This is *not* a general purpose rangelist merge but a special replacement
   for svn_rangelist_merge when REVISION is guaranteed to be younger than any
   element in RANGELIST.  svn_rangelist_merge is O(n) worst-case (i.e. when
   all the ranges in output rangelist are older than the incoming changes).
   This turns the special case of a single incoming younger range into O(1).
   */
static svn_error_t *
rangelist_merge_revision(apr_array_header_t *rangelist,
                         svn_revnum_t revision,
                         apr_pool_t *result_pool)
{
  svn_merge_range_t *new_range;
  if (rangelist->nelts)
    {
      svn_merge_range_t *range = APR_ARRAY_IDX(rangelist, rangelist->nelts - 1,
                                               svn_merge_range_t *);
      if (range->end == revision - 1)
        {
          /* REVISION is adjacent to the youngest range in RANGELIST
             so we can simply expand that range to encompass REVISION. */
          range->end = revision;
          return SVN_NO_ERROR;
        }
    }
  new_range = apr_palloc(result_pool, sizeof(*new_range));
  new_range->start = revision - 1;
  new_range->end = revision;
  new_range->inheritable = TRUE;

  APR_ARRAY_PUSH(rangelist, svn_merge_range_t *) = new_range;

  return SVN_NO_ERROR;
}

/* Implements the svn_log_entry_receiver_t interface.

   BATON is an log_noop_baton_t *.

   Add LOG_ENTRY->REVISION to BATON->OPERATIVE_RANGES.

   If LOG_ENTRY->REVISION has already been fully merged to
   MERGE_B->TARGET_ABSPATH per the mergeinfo in CHILDREN_WITH_MERGEINFO,
   then add LOG_ENTRY->REVISION to BATON->MERGED_RANGES.

   Use SCRATCH_POOL for temporary allocations.  Allocate additions to
   BATON->MERGED_RANGES and BATON->OPERATIVE_RANGES in BATON->POOL.
*/
static svn_error_t *
log_noop_revs(void *baton,
              svn_log_entry_t *log_entry,
              apr_pool_t *scratch_pool)
{
  log_noop_baton_t *log_gap_baton = baton;
  apr_hash_index_t *hi;
  svn_revnum_t revision;
  svn_boolean_t log_entry_rev_required = FALSE;

  revision = log_entry->revision;

  /* Unconditionally add LOG_ENTRY->REVISION to BATON->OPERATIVE_MERGES. */
  SVN_ERR(rangelist_merge_revision(log_gap_baton->operative_ranges,
                                   revision,
                                   log_gap_baton->pool));

  /* Examine each path affected by LOG_ENTRY->REVISION.  If the explicit or
     inherited mergeinfo for *all* of the corresponding paths under
     MERGE_B->TARGET_ABSPATH reflects that LOG_ENTRY->REVISION has been
     merged, then add LOG_ENTRY->REVISION to BATON->MERGED_RANGES. */
  for (hi = apr_hash_first(scratch_pool, log_entry->changed_paths2);
       hi;
       hi = apr_hash_next(hi))
    {
      const char *path = svn__apr_hash_index_key(hi);
      const char *rel_path;
      const char *cwmi_path;
      apr_array_header_t *paths_explicit_rangelist = NULL;
      svn_boolean_t mergeinfo_inherited = FALSE;

      /* Adjust REL_PATH so it is relative to the merge source then use it to
         calculate what path in the merge target would be affected by this
         revision. */
      rel_path = svn_fspath__skip_ancestor(log_gap_baton->source_repos_abs, path);
      /* Is PATH even within the merge target?  If it isn't we
         can disregard it altogether. */
      if (rel_path == NULL)
        continue;
      cwmi_path = svn_dirent_join(log_gap_baton->merge_b->target_abspath,
                                  rel_path, scratch_pool);

      /* Find any explicit or inherited mergeinfo for PATH. */
      while (!log_entry_rev_required)
        {
          svn_client__merge_path_t *child = get_child_with_mergeinfo(
            log_gap_baton->children_with_mergeinfo, cwmi_path);

          if (child && child->pre_merge_mergeinfo)
            {
              /* Found some explicit mergeinfo, grab any ranges
                 for PATH. */
              paths_explicit_rangelist =
                apr_hash_get(child->pre_merge_mergeinfo, path,
                             APR_HASH_KEY_STRING);
              break;
            }

          if (cwmi_path[0] == '\0'
              || svn_dirent_is_root(cwmi_path, strlen(cwmi_path))
              || svn_path_compare_paths(log_gap_baton->merge_b->target_abspath,
                                        cwmi_path) == 0)
            {
              /* Can't crawl any higher. */
              break;
            }

          /* Didn't find anything so crawl up to the parent. */
          cwmi_path = svn_dirent_dirname(cwmi_path, scratch_pool);
          path = svn_dirent_dirname(path, scratch_pool);

          /* At this point *if* we find mergeinfo it will be inherited. */
          mergeinfo_inherited = TRUE;
        }

      if (paths_explicit_rangelist)
        {
          apr_array_header_t *intersecting_range;
          apr_array_header_t *rangelist;

          rangelist = svn_rangelist__initialize(revision - 1, revision, TRUE,
                                                scratch_pool);

          /* If PATH inherited mergeinfo we must consider inheritance in the
             event the inherited mergeinfo is actually non-inheritable. */
          SVN_ERR(svn_rangelist_intersect(&intersecting_range,
                                          paths_explicit_rangelist,
                                          rangelist,
                                          mergeinfo_inherited, scratch_pool));

          if (intersecting_range->nelts == 0)
            log_entry_rev_required = TRUE;
        }
      else
        {
          log_entry_rev_required = TRUE;
        }
    }

  if (!log_entry_rev_required)
    SVN_ERR(rangelist_merge_revision(log_gap_baton->merged_ranges,
                                     revision,
                                     log_gap_baton->pool));

  return SVN_NO_ERROR;
}

/* Helper for do_directory_merge().

   URL1, REVISION1, URL2, REVISION2, NOTIFY_B, and MERGE_B are
   cascaded from the arguments of the same name in do_directory_merge().
   RA_SESSION is the session for URL2@REVISION2.

   Find all the ranges required by subtrees in
   NOTIFY_B->CHILDREN_WITH_MERGEINFO that are *not* required by
   MERGE_B->TARGET_ABSPATH (i.e. NOTIFY_B->CHILDREN_WITH_MERGEINFO[0]).  If such
   ranges exist, then find any subset of ranges which, if merged, would be
   inoperative.  Finally, if any inoperative ranges are found then remove
   these ranges from all of the subtree's REMAINING_RANGES.

   This function should only be called when honoring mergeinfo during
   forward merges (i.e. REVISION1 < REVSION2).
*/
static svn_error_t *
remove_noop_subtree_ranges(const char *url1,
                           svn_revnum_t revision1,
                           const char *url2,
                           svn_revnum_t revision2,
                           svn_ra_session_t *ra_session,
                           notification_receiver_baton_t *notify_b,
                           merge_cmd_baton_t *merge_b,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool)
{
  /* ### Do we need to check that we are at a uniform working revision? */
  int i;
  svn_client__merge_path_t *root_child =
    APR_ARRAY_IDX(notify_b->children_with_mergeinfo, 0,
                  svn_client__merge_path_t *);
  apr_array_header_t *requested_ranges;
  apr_array_header_t *subtree_gap_ranges;
  apr_array_header_t *subtree_remaining_ranges;
  apr_array_header_t *log_targets;
  apr_array_header_t *merged_ranges;
  apr_array_header_t *operative_ranges;
  log_noop_baton_t log_gap_baton;
  svn_merge_range_t *oldest_gap_rev;
  svn_merge_range_t *youngest_gap_rev;
  apr_array_header_t *inoperative_ranges;
  const char *repos_root_url;


  /* This function is only intended to work with forward merges. */
  if (revision1 > revision2)
    return SVN_NO_ERROR;

  /* Another easy out: There are no subtrees. */
  if (notify_b->children_with_mergeinfo->nelts < 2)
    return SVN_NO_ERROR;

  subtree_remaining_ranges = apr_array_make(scratch_pool, 1,
                                            sizeof(svn_merge_range_t *));
  log_targets = apr_array_make(scratch_pool, 1, sizeof(const char *));
  merged_ranges = apr_array_make(scratch_pool, 0, sizeof(svn_revnum_t *));
  operative_ranges = apr_array_make(scratch_pool, 0, sizeof(svn_revnum_t *));

  /* Given the requested merge of REVISION1:REVISION2 might there be any
     part of this range required for subtrees but not for the target? */
  requested_ranges = svn_rangelist__initialize(MIN(revision1, revision2),
                                               MAX(revision1, revision2),
                                               TRUE, scratch_pool);
  SVN_ERR(svn_rangelist_remove(&subtree_gap_ranges,
                               root_child->remaining_ranges,
                               requested_ranges, FALSE, scratch_pool));

  /* Early out, nothing to operate on */
  if (!subtree_gap_ranges->nelts)
    return SVN_NO_ERROR;

  /* Create a rangelist describing every range required across all subtrees. */
  for (i = 1; i < notify_b->children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
        APR_ARRAY_IDX(notify_b->children_with_mergeinfo, i,
                      svn_client__merge_path_t *);

      /* CHILD->REMAINING_RANGES will be NULL if child is absent. */
      if (child->remaining_ranges && child->remaining_ranges->nelts)
        SVN_ERR(svn_rangelist_merge(&subtree_remaining_ranges,
                                    child->remaining_ranges,
                                    scratch_pool));
    }

  /* It's possible that none of the subtrees had any remaining ranges. */
  if (!subtree_remaining_ranges->nelts)
    return SVN_NO_ERROR;

  /* Ok, *finally* we can answer what part(s) of REVISION1:REVISION2 are
     required for the subtrees but not the target. */
  SVN_ERR(svn_rangelist_intersect(&subtree_gap_ranges,
                                  subtree_gap_ranges,
                                  subtree_remaining_ranges, FALSE,
                                  scratch_pool));

  /* Another early out */
  if (!subtree_gap_ranges->nelts)
    return SVN_NO_ERROR;

  /* One or more subtrees need some revisions that the target doesn't need.
     Use log to determine if any of these revisions are inoperative. */
  oldest_gap_rev = APR_ARRAY_IDX(subtree_gap_ranges, 0, svn_merge_range_t *);
  youngest_gap_rev = APR_ARRAY_IDX(subtree_gap_ranges,
                         subtree_gap_ranges->nelts - 1, svn_merge_range_t *);

  SVN_ERR(svn_wc__node_get_repos_info(&repos_root_url, NULL,
                                      merge_b->ctx->wc_ctx,
                                      merge_b->target_abspath,
                                      scratch_pool, scratch_pool));

  /* Set up the log baton. */
  log_gap_baton.merge_b = merge_b;
  log_gap_baton.children_with_mergeinfo = notify_b->children_with_mergeinfo;
  SVN_ERR(svn_client__path_relative_to_root(
                    &(log_gap_baton.target_repos_abs), merge_b->ctx->wc_ctx,
                    merge_b->target_abspath, repos_root_url, TRUE, NULL,
                    result_pool, scratch_pool));
  SVN_ERR(svn_client__path_relative_to_root(
                    &(log_gap_baton.source_repos_abs), merge_b->ctx->wc_ctx,
                    url2, repos_root_url, TRUE, NULL,
                    result_pool, scratch_pool));
  log_gap_baton.merged_ranges = merged_ranges;
  log_gap_baton.operative_ranges = operative_ranges;
  log_gap_baton.pool = svn_pool_create(scratch_pool);

  APR_ARRAY_PUSH(log_targets, const char *) = "";

  SVN_ERR(svn_ra_get_log2(ra_session, log_targets, oldest_gap_rev->start + 1,
                          youngest_gap_rev->end, 0, TRUE, TRUE, FALSE,
                          apr_array_make(scratch_pool, 0,
                                         sizeof(const char *)),
                          log_noop_revs, &log_gap_baton, scratch_pool));

  inoperative_ranges = svn_rangelist__initialize(oldest_gap_rev->start,
                                                 youngest_gap_rev->end,
                                                 TRUE, scratch_pool);
  SVN_ERR(svn_rangelist_remove(&(inoperative_ranges),
                               log_gap_baton.operative_ranges,
                               inoperative_ranges, FALSE, scratch_pool));

  SVN_ERR(svn_rangelist_merge(&(log_gap_baton.merged_ranges),
                              inoperative_ranges, scratch_pool));

  for (i = 1; i < notify_b->children_with_mergeinfo->nelts; i++)
    {
      svn_client__merge_path_t *child =
        APR_ARRAY_IDX(notify_b->children_with_mergeinfo, i,
                      svn_client__merge_path_t *);

      /* CHILD->REMAINING_RANGES will be NULL if child is absent. */
      if (child->remaining_ranges && child->remaining_ranges->nelts)
        {
          /* Remove inoperative ranges from all children so we don't perform
             inoperative editor drives. */
          SVN_ERR(svn_rangelist_remove(&(child->remaining_ranges),
                                       log_gap_baton.merged_ranges,
                                       child->remaining_ranges,
                                       FALSE, result_pool));
        }
    }

  svn_pool_destroy(log_gap_baton.pool);

  return SVN_NO_ERROR;
}

/* Helper for do_merge() when the merge target is a directory.

   Perform a merge of changes between URL1@REVISION1 and URL2@REVISION2
   to the working copy path TARGET_ABSPATH.
   URL1, URL2, and TARGET_ABSPATH all represent directories -- for the
   single file case, the caller should use do_file_merge().

   MERGE_B is the merge_cmd_baton_t created by do_merge() that describes
   the merge being performed.  If MERGE_B->sources_ancestral is set, then
   URL1@REVISION1 must be a historical ancestor of URL2@REVISION2, or
   vice-versa (see `MERGEINFO MERGE SOURCE NORMALIZATION' for more
   requirements around the values of URL1, REVISION1, URL2, and REVISION2
   in this case).

   If mergeinfo is being recorded, SQUELCH_MERGEINFO_NOTIFICATIONS is FALSE,
   and MERGE_B->CTX->NOTIFY_FUNC2 is not NULL, then call
   MERGE_B->CTX->NOTIFY_FUNC2 with MERGE_B->CTX->NOTIFY_BATON2 and a
   svn_wc_notify_merge_record_info_begin notification before any mergeinfo
   changes are made to describe the merge performed.

   If mergeinfo is being recorded to describe this merge, and RESULT_CATALOG
   is not NULL, then don't record the new mergeinfo on the WC, but instead
   record it in RESULT_CATALOG, where the keys are absolute working copy
   paths and the values are the new mergeinfos for each.  Allocate additions
   to RESULT_CATALOG in pool which RESULT_CATALOG was created in.

   Handle DEPTH as documented for svn_client_merge3().

   If ABORT_ON_CONFLICTS is TRUE raise an SVN_ERR_WC_FOUND_CONFLICT error
   if any merge conflicts occur.

   Perform any temporary allocations in SCRATCH_POOL.

   NOTE: This is a wrapper around drive_merge_report_editor() which
   handles the complexities inherent to situations where a given
   directory's children may have intersecting merges (because they
   meet one or more of the criteria described in get_mergeinfo_paths()).
*/
static svn_error_t *
do_directory_merge(svn_mergeinfo_catalog_t result_catalog,
                   const char *url1,
                   svn_revnum_t revision1,
                   const char *url2,
                   svn_revnum_t revision2,
                   const char *target_abspath,
                   svn_depth_t depth,
                   svn_boolean_t squelch_mergeinfo_notifications,
                   svn_boolean_t abort_on_conflicts,
                   notification_receiver_baton_t *notify_b,
                   merge_cmd_baton_t *merge_b,
                   apr_pool_t *scratch_pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  svn_error_t *merge_conflict_err = SVN_NO_ERROR;

  /* The range defining the mergeinfo we will record to describe the merge
     (assuming we are recording mergeinfo

     Note: This may be a subset of REVISION1:REVISION2 if
     populate_remaining_ranges() determines that some part of
     REVISION1:REVISION2 has already been wholly merged to TARGET_ABSPATH.
     Also, the actual editor drive(s) may be a subset of RANGE, if
     remove_noop_subtree_ranges() and/or fix_deleted_subtree_ranges()
     further tweak things. */
  svn_merge_range_t range;

  svn_ra_session_t *ra_session;
  svn_client__merge_path_t *target_merge_path;
  svn_boolean_t is_rollback = (revision1 > revision2);
  const char *primary_url = is_rollback ? url1 : url2;
  const char *source_root_url, *mergeinfo_path;
  svn_boolean_t honor_mergeinfo, record_mergeinfo;
  svn_boolean_t same_urls = (strcmp(url1, url2) == 0);

  honor_mergeinfo = HONOR_MERGEINFO(merge_b);
  record_mergeinfo = RECORD_MERGEINFO(merge_b);

  /* Initialize NOTIFY_B->CHILDREN_WITH_MERGEINFO. See the comment
     'THE CHILDREN_WITH_MERGEINFO ARRAY' at the start of this file. */
  notify_b->children_with_mergeinfo =
    apr_array_make(scratch_pool, 0, sizeof(svn_client__merge_path_t *));

  /* If we are not honoring mergeinfo we can skip right to the
     business of merging changes! */
  if (!honor_mergeinfo)
    return do_mergeinfo_unaware_dir_merge(url1, revision1, url2, revision2,
                                          target_abspath, depth,
                                          notify_b, merge_b, scratch_pool);

  /*** If we get here, we're dealing with related sources from the
       same repository as the target -- merge tracking might be
       happenin'! ***/

  /* Point our RA_SESSION to the URL of our youngest merge source side. */
  ra_session = is_rollback ? merge_b->ra_session1 : merge_b->ra_session2;

  /* Fill NOTIFY_B->CHILDREN_WITH_MERGEINFO with child paths (const
     svn_client__merge_path_t *) which might have intersecting merges
     because they meet one or more of the criteria described in
     get_mergeinfo_paths(). Here the paths are arranged in a depth
     first order. */
  SVN_ERR(svn_ra_get_repos_root2(ra_session, &source_root_url, scratch_pool));
  SVN_ERR(svn_client__path_relative_to_root(&mergeinfo_path,
                                            merge_b->ctx->wc_ctx, primary_url,
                                            source_root_url, TRUE, NULL,
                                            scratch_pool, scratch_pool));

  SVN_ERR(get_mergeinfo_paths(notify_b->children_with_mergeinfo, merge_b,
                              mergeinfo_path, source_root_url,
                              url1, url2, revision1, revision2,
                              ra_session, depth, scratch_pool, scratch_pool));

  /* The first item from the NOTIFY_B->CHILDREN_WITH_MERGEINFO is always
     the target thanks to depth-first ordering. */
  target_merge_path = APR_ARRAY_IDX(notify_b->children_with_mergeinfo, 0,
                                    svn_client__merge_path_t *);
  merge_b->target_missing_child = target_merge_path->missing_child;

  /* If we are honoring mergeinfo, then for each item in
     NOTIFY_B->CHILDREN_WITH_MERGEINFO, we need to calculate what needs to be
     merged, and then merge it.  Otherwise, we just merge what we were asked
     to merge across the whole tree.  */
  SVN_ERR(populate_remaining_ranges(notify_b->children_with_mergeinfo,
                                    source_root_url,
                                    url1, revision1, url2, revision2,
                                    honor_mergeinfo,
                                    ra_session, mergeinfo_path,
                                    merge_b, scratch_pool, scratch_pool));

  /* Always start with a range which describes the most inclusive merge
     possible, i.e. REVISION1:REVISION2. */
  range.start = revision1;
  range.end = revision2;
  range.inheritable = TRUE;

  if (honor_mergeinfo && !merge_b->reintegrate_merge)
    {
      svn_revnum_t new_range_start, start_rev;
      apr_pool_t *iterpool = svn_pool_create(scratch_pool);

      /* The merge target TARGET_ABSPATH and/or its subtrees may not need all
         of REVISION1:REVISION2 applied.  So examine
         NOTIFY_B->CHILDREN_WITH_MERGEINFO to find the oldest starting
         revision that actually needs to be merged (for reverse merges this is
         the youngest starting revision).

         We'll do this twice, right now for the start of the mergeinfo we will
         ultimately record to describe this merge and then later for the
         start of the actual editor drive. */
      new_range_start = get_most_inclusive_start_rev(
        notify_b->children_with_mergeinfo, is_rollback);
      if (SVN_IS_VALID_REVNUM(new_range_start))
        range.start = new_range_start;

      /* Remove inoperative ranges from any subtrees' remaining_ranges
         to spare the expense of noop editor drives. */
      if (!is_rollback)
        SVN_ERR(remove_noop_subtree_ranges(url1, revision1, url2, revision2,
                                           ra_session,
                                           notify_b, merge_b,
                                           scratch_pool, iterpool));

      /* Adjust subtrees' remaining_ranges to deal with issue #3067 */
      SVN_ERR(fix_deleted_subtree_ranges(url1, revision1, url2, revision2,
                                         ra_session, notify_b, merge_b,
                                         scratch_pool, iterpool));

      /* remove_noop_subtree_ranges() and/or fix_deleted_subtree_range()
         may have further refined the starting revision for our editor
         drive. */
      start_rev =
        get_most_inclusive_start_rev(notify_b->children_with_mergeinfo,
                                     is_rollback);

      /* Is there anything to merge? */
      if (SVN_IS_VALID_REVNUM(start_rev))
        {
          /* Now examine NOTIFY_B->CHILDREN_WITH_MERGEINFO to find the youngest
             ending revision that actually needs to be merged (for reverse
             merges this is the oldest starting revision). */
           svn_revnum_t end_rev =
             get_most_inclusive_end_rev(notify_b->children_with_mergeinfo,
                                        is_rollback);

          /* While END_REV is valid, do the following:

             1. Tweak each NOTIFY_B->CHILDREN_WITH_MERGEINFO element so that
                the element's remaining_ranges member has as its first element
                a range that ends with end_rev.

             2. Starting with start_rev, call drive_merge_report_editor()
                on MERGE_B->target_abspath for start_rev:end_rev.

             3. Remove the first element from each
                NOTIFY_B->CHILDREN_WITH_MERGEINFO element's remaining_ranges
                member.

             4. Again examine NOTIFY_B->CHILDREN_WITH_MERGEINFO to find the most
                inclusive starting revision that actually needs to be merged and
                update start_rev.  This prevents us from needlessly contacting the
                repository and doing a diff where we describe the entire target
                tree as *not* needing any of the requested range.  This can happen
                whenever we have mergeinfo with gaps in it for the merge source.

             5. Again examine NOTIFY_B->CHILDREN_WITH_MERGEINFO to find the most
                inclusive ending revision that actually needs to be merged and
                update end_rev.

             6. Lather, rinse, repeat.
          */

          while (end_rev != SVN_INVALID_REVNUM)
            {
              svn_revnum_t next_end_rev;
              const char *real_url1 = url1, *real_url2 = url2;
              const char *old_sess1_url = NULL, *old_sess2_url = NULL;
              svn_merge_range_t *first_target_range = APR_ARRAY_IDX(
                target_merge_path->remaining_ranges, 0, svn_merge_range_t *);

              /* Issue #3324: Stop editor abuse!  Don't call
                 drive_merge_report_editor() in such a way that we request an
                 editor with svn_client__get_diff_editor() for some rev X,
                 then call svn_ra_do_diff3() for some revision Y, and then
                 call reporter->set_path(PATH=="") to set the root revision
                 for the editor drive to revision Z where
                 (X != Z && X < Z < Y).  This is bogus because the server will
                 send us the diff between X:Y but the client is expecting the
                 diff between Y:Z.  See issue #3324 for full details on the
                 problems this can cause. */
              if (first_target_range
                  && start_rev != first_target_range->start)
                {
                  if (is_rollback)
                    {
                      if (end_rev < first_target_range->start)
                        end_rev = first_target_range->start;
                    }
                  else
                    {
                      if (end_rev > first_target_range->start)
                        end_rev = first_target_range->start;
                    }
                }

              svn_pool_clear(iterpool);

              slice_remaining_ranges(notify_b->children_with_mergeinfo,
                                     is_rollback, end_rev, scratch_pool);
              notify_b->cur_ancestor_index = -1;

              /* URL1@REVISION1 is a real location; URL2@REVISION2 is a
                 real location -- that much we know (thanks to the merge
                 source normalization code).  But for revisions between
                 them, the URLs might differ.  Here are the rules:

                   * If URL1 == URL2, then all URLs between REVISION1 and
                     REVISION2 also match URL1/URL2.

                   * If URL1 != URL2, then:

                       * If REVISION1 < REVISION2, only REVISION1 maps to
                         URL1.  The revisions between REVISION1+1 and
                         REVISION2 (inclusive) map to URL2.

                       * If REVISION1 > REVISION2, Only REVISION2 maps to
                         URL2.  The revisions between REVISION1 and
                         REVISION2+1 (inclusive) map to URL1.

                 We need to adjust our URLs accordingly, here.
              */
              if (! same_urls)
                {
                  if (is_rollback && (end_rev != revision2))
                    {
                      real_url2 = url1;
                      SVN_ERR(svn_client__ensure_ra_session_url
                              (&old_sess2_url, merge_b->ra_session2,
                               real_url2, iterpool));
                    }
                  if ((! is_rollback) && (start_rev != revision1))
                    {
                      real_url1 = url2;
                      SVN_ERR(svn_client__ensure_ra_session_url
                              (&old_sess1_url, merge_b->ra_session1,
                               real_url1, iterpool));
                    }
                }
              SVN_ERR(drive_merge_report_editor(
                merge_b->target_abspath,
                real_url1, start_rev,
                real_url2, end_rev,
                notify_b->children_with_mergeinfo,
                depth, notify_b,
                merge_b,
                iterpool));
              if (old_sess1_url)
                SVN_ERR(svn_ra_reparent(merge_b->ra_session1,
                                        old_sess1_url, iterpool));
              if (old_sess2_url)
                SVN_ERR(svn_ra_reparent(merge_b->ra_session2,
                                        old_sess2_url, iterpool));

              /* If any paths picked up explicit mergeinfo as a result of
                 the merge we need to make sure any mergeinfo those paths
                 inherited is recorded and then add these paths to
                 MERGE_B->CHILDREN_WITH_MERGEINFO.*/
              SVN_ERR(process_children_with_new_mergeinfo(merge_b, notify_b,
                                                          scratch_pool));

              /* If any subtrees had their explicit mergeinfo deleted as a
                 result of the merge then remove these paths from
                 MERGE_B->CHILDREN_WITH_MERGEINFO since there is no need
                 to consider these subtrees for subsequent editor drives
                 nor do we want to record mergeinfo on them describing
                 the merge itself. */
              remove_children_with_deleted_mergeinfo(merge_b, notify_b);

              /* Prepare for the next iteration (if any). */
              remove_first_range_from_remaining_ranges(
                end_rev, notify_b->children_with_mergeinfo, scratch_pool);
              next_end_rev =
                get_most_inclusive_end_rev(notify_b->children_with_mergeinfo,
                                           is_rollback);
              if ((next_end_rev != SVN_INVALID_REVNUM || abort_on_conflicts)
                  && is_path_conflicted_by_merge(merge_b))
                {
                  svn_merge_range_t conflicted_range;
                  conflicted_range.start = start_rev;
                  conflicted_range.end = end_rev;
                  merge_conflict_err = make_merge_conflict_error(
                                         merge_b->target_abspath,
                                         &conflicted_range,
                                         scratch_pool);
                  range.end = end_rev;
                  break;
                }
              start_rev =
                get_most_inclusive_start_rev(notify_b->children_with_mergeinfo,
                                             is_rollback);
              end_rev = next_end_rev;
            }
        }
      svn_pool_destroy(iterpool);
    }
  else
    {
      if (!merge_b->record_only)
        {
          /* Reset cur_ancestor_index to -1 so that subsequent cherry
             picked revision ranges will be notified upon subsequent
             operative merge. */
          notify_b->cur_ancestor_index = -1;

          SVN_ERR(drive_merge_report_editor(merge_b->target_abspath,
                                            url1, revision1, url2, revision2,
                                            NULL,
                                            depth, notify_b,
                                            merge_b,
                                            scratch_pool));
        }
    }

  /* Record mergeinfo where appropriate.*/
  if (record_mergeinfo)
    {
      err = record_mergeinfo_for_dir_merge(result_catalog,
                                           &range,
                                           mergeinfo_path,
                                           depth,
                                           squelch_mergeinfo_notifications,
                                           notify_b,
                                           merge_b,
                                           scratch_pool);

      /* If a path has an immediate parent with non-inheritable mergeinfo at
         this point, then it meets criteria 3 or 5 described in
         get_mergeinfo_paths' doc string.  For paths which exist prior to a
         merge explicit mergeinfo has already been set.  But for paths added
         during the merge this is not the case.  The path might have explicit
         mergeinfo from the merge source, but no mergeinfo yet exists
         describing *this* merge.  So the added path has either incomplete
         explicit mergeinfo or inherits incomplete mergeinfo from its
         immediate parent (if any, the parent might have only non-inheritable
         ranges in which case the path simply inherits empty mergeinfo).

         So here we look at the root path of each subtree added during the
         merge and set explicit mergeinfo on it if it meets the aforementioned
         conditions. */
      if (err == SVN_NO_ERROR
          && (range.start < range.end)) /* Nothing to record on added subtrees
                                           resulting from reverse merges. */
        {
          err = record_mergeinfo_for_added_subtrees(
                  &range, mergeinfo_path, depth,
                  squelch_mergeinfo_notifications,
                  notify_b, merge_b, scratch_pool);
        }
    }

  return svn_error_compose_create(err, merge_conflict_err);
}

/** Ensure that *RA_SESSION is opened to URL, either by reusing
 * *RA_SESSION if it is non-null and already opened to URL's
 * repository, or by allocating a new *RA_SESSION in POOL.
 * (RA_SESSION itself cannot be null, of course.)
 *
 * CTX is used as for svn_client__open_ra_session_internal().
 */
static svn_error_t *
ensure_ra_session_url(svn_ra_session_t **ra_session,
                      const char *url,
                      svn_client_ctx_t *ctx,
                      apr_pool_t *pool)
{
  svn_error_t *err = SVN_NO_ERROR;

  if (*ra_session)
    {
      const char *old_session_url;
      err = svn_client__ensure_ra_session_url(&old_session_url,
                                              *ra_session,
                                              url,
                                              pool);
    }

  /* SVN_ERR_RA_ILLEGAL_URL is raised when url doesn't point to the same
     repository as ra_session. */
  if (! *ra_session || (err && err->apr_err == SVN_ERR_RA_ILLEGAL_URL))
    {
      svn_error_clear(err);
      err = svn_client__open_ra_session_internal(ra_session, NULL, url, NULL,
                                                 NULL, FALSE, TRUE, ctx, pool);
    }
  SVN_ERR(err);

  return SVN_NO_ERROR;
}

/* Drive a merge of MERGE_SOURCES into working copy path TARGET_ABSPATH
   and possibly record mergeinfo describing the merge -- see
   RECORD_MERGEINFO().

   If MODIFIED_SUBTREES is not NULL and SOURCES_ANCESTRAL or
   REINTEGRATE_MERGE is true, then set *MODIFIED_SUBTREES to a hash
   containing every path modified, skipped, added, or tree-conflicted
   by the merge.  Keys and values of the hash are both (const char *)
   absolute paths.  The contents of the hash are allocated in RESULT_POOL.

   If SOURCES_ANCESTRAL is set, then for every merge source in
   MERGE_SOURCES, the "left" and "right" side of the merge source are
   ancestrally related.  (See 'MERGEINFO MERGE SOURCE NORMALIZATION'
   for more on what that means and how it matters.)

   If SOURCES_RELATED is set, the "left" and "right" sides of the
   merge source are historically related (ancestors, uncles, second
   cousins thrice removed, etc...).  (This is passed through to
   do_file_merge() to simulate the history checks that the repository
   logic does in the directory case.)

   SAME_REPOS is TRUE iff the merge sources live in the same
   repository as the one from which the target working copy has been
   checked out.

   If mergeinfo is being recorded, SQUELCH_MERGEINFO_NOTIFICATIONS is FALSE,
   and CTX->NOTIFY_FUNC2 is not NULL, then call CTX->NOTIFY_FUNC2 with
   CTX->NOTIFY_BATON2 and a svn_wc_notify_merge_record_info_begin
   notification before any mergeinfo changes are made to describe the merge
   performed.

   If mergeinfo is being recorded to describe this merge, and RESULT_CATALOG
   is not NULL, then don't record the new mergeinfo on the WC, but instead
   record it in RESULT_CATALOG, where the keys are absolute working copy
   paths and the values are the new mergeinfos for each.  Allocate additions
   to RESULT_CATALOG in pool which RESULT_CATALOG was created in.

   FORCE, DRY_RUN, RECORD_ONLY, IGNORE_ANCESTRY, DEPTH, MERGE_OPTIONS,
   and CTX are as described in the docstring for svn_client_merge_peg3().

   If not NULL, RECORD_ONLY_PATHS is a hash of (const char *) paths mapped
   to the same.  If RECORD_ONLY is true and RECORD_ONLY_PATHS is not NULL,
   then record mergeinfo describing the merge only on subtrees which contain
   items from RECORD_ONLY_PATHS.  If RECORD_ONLY is true and RECORD_ONLY_PATHS
   is NULL, then record mergeinfo on every subtree with mergeinfo in
   TARGET_ABSPATH.

   REINTEGRATE_MERGE is TRUE if this is a reintegrate merge.

   *USE_SLEEP will be set TRUE if a sleep is required to ensure timestamp
   integrity, *USE_SLEEP will be unchanged if no sleep is required.

   SCRATCH_POOL is used for all temporary allocations.
*/
static svn_error_t *
do_merge(apr_hash_t **modified_subtrees,
         svn_mergeinfo_catalog_t result_catalog,
         const apr_array_header_t *merge_sources,
         const char *target_abspath,
         svn_boolean_t sources_ancestral,
         svn_boolean_t sources_related,
         svn_boolean_t same_repos,
         svn_boolean_t ignore_ancestry,
         svn_boolean_t force,
         svn_boolean_t dry_run,
         svn_boolean_t record_only,
         apr_hash_t *record_only_paths,
         svn_boolean_t reintegrate_merge,
         svn_boolean_t squelch_mergeinfo_notifications,
         svn_depth_t depth,
         const apr_array_header_t *merge_options,
         svn_boolean_t *use_sleep,
         svn_client_ctx_t *ctx,
         apr_pool_t *result_pool,
         apr_pool_t *scratch_pool)
{
  merge_cmd_baton_t merge_cmd_baton;
  notification_receiver_baton_t notify_baton;
  svn_config_t *cfg;
  const char *diff3_cmd;
  int i;
  svn_boolean_t checked_mergeinfo_capability = FALSE;
  svn_ra_session_t *ra_session1 = NULL, *ra_session2 = NULL;
  svn_node_kind_t target_kind;
  apr_pool_t *iterpool;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(target_abspath));

  /* Check from some special conditions when in record-only mode
     (which is a merge-tracking thing). */
  if (record_only)
    {
      /* We can't do a record-only merge if the sources aren't related. */
      if (! sources_ancestral)
        return svn_error_create(SVN_ERR_INCORRECT_PARAMS, NULL,
                                _("Use of two URLs is not compatible with "
                                  "mergeinfo modification"));

      /* We can't do a record-only merge if the sources aren't from
         the same repository as the target. */
      if (! same_repos)
        return svn_error_create(SVN_ERR_INCORRECT_PARAMS, NULL,
                                _("Merge from foreign repository is not "
                                  "compatible with mergeinfo modification"));

      /* If this is a dry-run record-only merge, there's nothing to do. */
      if (dry_run)
        return SVN_NO_ERROR;
    }

  iterpool = svn_pool_create(scratch_pool);
  SVN_ERR(svn_wc_read_kind(&target_kind, ctx->wc_ctx, target_abspath, FALSE,
                           iterpool));

  if (target_kind != svn_node_dir && target_kind != svn_node_file)
    return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                             _("Merge target '%s' does not exist in the "
                               "working copy"), target_abspath);

  /* Ensure a known depth. */
  if (depth == svn_depth_unknown)
    depth = svn_depth_infinity;

  /* Set up the diff3 command, so various callers don't have to. */
  cfg = ctx->config ? apr_hash_get(ctx->config, SVN_CONFIG_CATEGORY_CONFIG,
                                   APR_HASH_KEY_STRING) : NULL;
  svn_config_get(cfg, &diff3_cmd, SVN_CONFIG_SECTION_HELPERS,
                 SVN_CONFIG_OPTION_DIFF3_CMD, NULL);

  if (diff3_cmd != NULL)
    SVN_ERR(svn_path_cstring_to_utf8(&diff3_cmd, diff3_cmd, scratch_pool));

  /* Build the merge context baton (or at least the parts of it that
     don't need to be reset for each merge source).  */
  merge_cmd_baton.force = force;
  merge_cmd_baton.dry_run = dry_run;
  merge_cmd_baton.record_only = record_only;
  merge_cmd_baton.ignore_ancestry = ignore_ancestry;
  merge_cmd_baton.same_repos = same_repos;
  merge_cmd_baton.mergeinfo_capable = FALSE;
  merge_cmd_baton.sources_ancestral = sources_ancestral;
  merge_cmd_baton.ctx = ctx;
  merge_cmd_baton.target_missing_child = FALSE;
  merge_cmd_baton.reintegrate_merge = reintegrate_merge;
  merge_cmd_baton.target_abspath = target_abspath;
  SVN_ERR(svn_wc__node_get_repos_info(&merge_cmd_baton.repos_root_url, NULL,
                                      ctx->wc_ctx,
                                      merge_cmd_baton.target_abspath,
                                      result_pool,
                                      iterpool));
  merge_cmd_baton.pool = iterpool;
  merge_cmd_baton.merge_options = merge_options;
  merge_cmd_baton.diff3_cmd = diff3_cmd;
  merge_cmd_baton.use_sleep = use_sleep;

  /* Build the notification receiver baton. */
  notify_baton.wrapped_func = ctx->notify_func2;
  notify_baton.wrapped_baton = ctx->notify_baton2;
  notify_baton.nbr_notifications = 0;
  notify_baton.nbr_operative_notifications = 0;

  /* Do we already know the specific subtrees with mergeinfo we want
     to record-only mergeinfo on? */
  if (record_only && record_only_paths)
    notify_baton.merged_abspaths = record_only_paths;
  else
    notify_baton.merged_abspaths = NULL;

  notify_baton.skipped_abspaths = NULL;
  notify_baton.added_abspaths = NULL;
  notify_baton.tree_conflicted_abspaths = NULL;
  notify_baton.is_single_file_merge = FALSE;
  notify_baton.children_with_mergeinfo = NULL;
  notify_baton.cur_ancestor_index = -1;
  notify_baton.merge_b = &merge_cmd_baton;
  notify_baton.pool = result_pool;

  for (i = 0; i < merge_sources->nelts; i++)
    {
      merge_source_t *merge_source =
        APR_ARRAY_IDX(merge_sources, i, merge_source_t *);
      const char *url1, *url2;
      svn_revnum_t rev1, rev2;
      /* If conflicts occur while merging any but the very last
       * revision range we want an error to be raised that aborts
       * the merge operation. The user will be asked to resolve conflicts
       * before merging subsequent revision ranges. */
      svn_boolean_t abort_on_conflicts = (i < merge_sources->nelts - 1);

      svn_pool_clear(iterpool);

      /* Convenience variables. */
      url1 = merge_source->url1;
      url2 = merge_source->url2;
      rev1 = merge_source->rev1;
      rev2 = merge_source->rev2;

      /* Sanity check:  if our left- and right-side merge sources are
         the same, there's nothing to here. */
      if ((strcmp(url1, url2) == 0) && (rev1 == rev2))
        continue;

      /* Establish RA sessions to our URLs, reuse where possible. */
      SVN_ERR(ensure_ra_session_url(&ra_session1, url1, ctx, scratch_pool));
      SVN_ERR(ensure_ra_session_url(&ra_session2, url2, ctx, scratch_pool));

      /* Populate the portions of the merge context baton that need to
         be reset for each merge source iteration. */
      merge_cmd_baton.merge_source = *merge_source;
      merge_cmd_baton.implicit_src_gap = NULL;
      merge_cmd_baton.added_path = NULL;
      merge_cmd_baton.add_necessitated_merge = FALSE;
      merge_cmd_baton.dry_run_deletions =
        dry_run ? apr_hash_make(iterpool) : NULL;
      merge_cmd_baton.dry_run_added =
        dry_run ? apr_hash_make(iterpool) : NULL;
      merge_cmd_baton.conflicted_paths = NULL;
      merge_cmd_baton.paths_with_new_mergeinfo = NULL;
      merge_cmd_baton.paths_with_deleted_mergeinfo = NULL;
      merge_cmd_baton.ra_session1 = ra_session1;
      merge_cmd_baton.ra_session2 = ra_session2;

      /* Populate the portions of the merge context baton that require
         an RA session to set, but shouldn't be reset for each iteration. */
      if (! checked_mergeinfo_capability)
        {
          SVN_ERR(svn_ra_has_capability(ra_session1,
                                        &merge_cmd_baton.mergeinfo_capable,
                                        SVN_RA_CAPABILITY_MERGEINFO,
                                        iterpool));
          checked_mergeinfo_capability = TRUE;
        }

      /* Call our merge helpers based on TARGET_ABSPATH's kind. */
      if (target_kind == svn_node_file)
        {
          SVN_ERR(do_file_merge(result_catalog,
                                url1, rev1, url2, rev2, target_abspath,
                                sources_related,
                                squelch_mergeinfo_notifications,
                                &notify_baton,
                                abort_on_conflicts,
                                &merge_cmd_baton, iterpool));
        }
      else if (target_kind == svn_node_dir)
        {
          SVN_ERR(do_directory_merge(result_catalog,
                                     url1, rev1, url2, rev2, target_abspath,
                                     depth, squelch_mergeinfo_notifications,
                                     abort_on_conflicts,
                                     &notify_baton, &merge_cmd_baton,
                                     iterpool));

          /* Does the caller want to know what the merge has done? */
          if (modified_subtrees)
            {
              if (notify_baton.merged_abspaths)
                *modified_subtrees =
                  apr_hash_overlay(result_pool, *modified_subtrees,
                                   notify_baton.merged_abspaths);
              if (notify_baton.added_abspaths)
                *modified_subtrees =
                  apr_hash_overlay(result_pool, *modified_subtrees,
                                   notify_baton.added_abspaths);
              if (notify_baton.skipped_abspaths)
                *modified_subtrees =
                  apr_hash_overlay(result_pool, *modified_subtrees,
                                   notify_baton.skipped_abspaths);
              if (notify_baton.tree_conflicted_abspaths)
                *modified_subtrees =
                  apr_hash_overlay(result_pool, *modified_subtrees,
                                   notify_baton.tree_conflicted_abspaths);
            }
        }

      /* The final mergeinfo on TARGET_WCPATH may itself elide. */
      if (! dry_run)
        SVN_ERR(svn_client__elide_mergeinfo(target_abspath, NULL,
                                            ctx, iterpool));
    }

  /* Let everyone know we're finished here. */
  if (ctx->notify_func2)
    {
      svn_wc_notify_t *notify
        = svn_wc_create_notify(target_abspath, svn_wc_notify_merge_completed,
                               iterpool);
      notify->kind = svn_node_none;
      notify->content_state = notify->prop_state
        = svn_wc_notify_state_inapplicable;
      notify->lock_state = svn_wc_notify_lock_state_inapplicable;
      notify->revision = SVN_INVALID_REVNUM;
      (*ctx->notify_func2)(ctx->notify_baton2, notify, iterpool);
    }

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

/* Perform a two-URL merge between URLs which are related, but neither
   is a direct ancestor of the other.  This first does a real two-URL
   merge (unless this is record-only), followed by record-only merges
   to represent the changed mergeinfo.

   The merge is between URL1@REV1 (in URL1_RA_SESSION1) and URL2@REV2 (in
   URL2_RA_SESSION2); YC_REV is their youngest common ancestor.
   SOURCE_REPOS_ROOT and WC_REPOS_ROOT are the repository roots of the
   source URL and the target working copy.  Other arguments are as in
   all of the public merge APIs.

   *USE_SLEEP will be set TRUE if a sleep is required to ensure timestamp
   integrity, *USE_SLEEP will be unchanged if no sleep is required.

   SCRATCH_POOL is used for all temporary allocations.
 */
static svn_error_t *
merge_cousins_and_supplement_mergeinfo(const char *target_abspath,
                                       svn_ra_session_t *URL1_ra_session,
                                       svn_ra_session_t *URL2_ra_session,
                                       const char *URL1,
                                       svn_revnum_t rev1,
                                       const char *URL2,
                                       svn_revnum_t rev2,
                                       svn_revnum_t yc_rev,
                                       const char *source_repos_root,
                                       const char *wc_repos_root,
                                       svn_depth_t depth,
                                       svn_boolean_t ignore_ancestry,
                                       svn_boolean_t force,
                                       svn_boolean_t record_only,
                                       svn_boolean_t dry_run,
                                       const apr_array_header_t *merge_options,
                                       svn_boolean_t *use_sleep,
                                       svn_client_ctx_t *ctx,
                                       apr_pool_t *scratch_pool)
{
  svn_opt_revision_range_t *range;
  apr_array_header_t *remove_sources, *add_sources, *ranges;
  svn_opt_revision_t peg_revision;
  svn_boolean_t same_repos;
  apr_hash_t *modified_subtrees = NULL;

  /* Sure we could use SCRATCH_POOL througout this function, but since this
     is a wrapper around three separate merges we'll create a subpool we can
     clear between each of the three.  If the merge target has a lot of
     subtree mergeinfo, then this will help keep memory use in check. */
  apr_pool_t *subpool = svn_pool_create(scratch_pool);

  SVN_ERR_ASSERT(svn_dirent_is_absolute(target_abspath));

  if (strcmp(wc_repos_root, source_repos_root) != 0)
    {
      const char *source_repos_uuid;
      const char *wc_repos_uuid;

      SVN_ERR(svn_ra_get_uuid2(URL1_ra_session, &source_repos_uuid,
                               subpool));
      SVN_ERR(svn_client_uuid_from_path2(&wc_repos_uuid, target_abspath,
                                         ctx, subpool, subpool));
      same_repos = (strcmp(wc_repos_uuid, source_repos_uuid) == 0);
    }
  else
    same_repos = TRUE;

  peg_revision.kind = svn_opt_revision_number;

  range = apr_pcalloc(scratch_pool, sizeof(*range));
  range->start.kind = svn_opt_revision_number;
  range->start.value.number = rev1;
  range->end.kind = svn_opt_revision_number;
  range->end.value.number = yc_rev;
  ranges = apr_array_make(scratch_pool, 2,
                          sizeof(svn_opt_revision_range_t *));
  APR_ARRAY_PUSH(ranges, svn_opt_revision_range_t *) = range;
  peg_revision.value.number = rev1;
  SVN_ERR(normalize_merge_sources(&remove_sources, URL1, URL1,
                                  source_repos_root, &peg_revision,
                                  ranges, URL1_ra_session, ctx, scratch_pool,
                                  subpool));

  range = apr_pcalloc(scratch_pool, sizeof(*range));
  range->start.kind = svn_opt_revision_number;
  range->start.value.number = yc_rev;
  range->end.kind = svn_opt_revision_number;
  range->end.value.number = rev2;
  ranges = apr_array_make(scratch_pool, 2,
                          sizeof(svn_opt_revision_range_t *));
  APR_ARRAY_PUSH(ranges, svn_opt_revision_range_t *) = range;
  peg_revision.value.number = rev2;
  SVN_ERR(normalize_merge_sources(&add_sources, URL2, URL2,
                                  source_repos_root, &peg_revision,
                                  ranges, URL2_ra_session, ctx,
                                  scratch_pool, subpool));

  /* If this isn't a record-only merge, we'll first do a stupid
     point-to-point merge... */
  if (! record_only)
    {
      merge_source_t *faux_source;
      apr_array_header_t *faux_sources =
        apr_array_make(scratch_pool, 1, sizeof(merge_source_t *));
      modified_subtrees = apr_hash_make(scratch_pool);
      faux_source = apr_pcalloc(scratch_pool, sizeof(*faux_source));
      faux_source->url1 = URL1;
      faux_source->url2 = URL2;
      faux_source->rev1 = rev1;
      faux_source->rev2 = rev2;
      APR_ARRAY_PUSH(faux_sources, merge_source_t *) = faux_source;
      SVN_ERR(do_merge(&modified_subtrees, NULL, faux_sources, target_abspath,
                       FALSE, TRUE, same_repos,
                       ignore_ancestry, force, dry_run, FALSE, NULL, TRUE,
                       FALSE, depth, merge_options, use_sleep, ctx,
                       scratch_pool, subpool));
    }
  else if (! same_repos)
    {
      return svn_error_create(SVN_ERR_INCORRECT_PARAMS, NULL,
                              _("Merge from foreign repository is not "
                                "compatible with mergeinfo modification"));
    }

  /* ... and now, if we're doing the mergeinfo thang, we execute a
     pair of record-only merges using the real sources we've
     calculated.  (We know that each tong in our fork of our merge
     source history tree has an ancestral relationship with the common
     ancestral, so we force ancestral=TRUE here.)

     Issue #3648: We don't actually perform these two record-only merges
     on the WC at first, but rather see what each would do and store that
     in two mergeinfo catalogs.  We then merge the catalogs together and
     then record the result in the WC.  This prevents the second record
     only merge from removing legitimate mergeinfo history, from the same
     source, that was made in prior merges. */
  if (same_repos && !dry_run)
    {
      svn_wc_notify_t *notify = svn_wc_create_notify(
        target_abspath, svn_wc_notify_merge_record_info_begin, scratch_pool);
      svn_mergeinfo_catalog_t add_result_catalog =
        apr_hash_make(scratch_pool);
      svn_mergeinfo_catalog_t remove_result_catalog =
        apr_hash_make(scratch_pool);

      ctx->notify_func2(ctx->notify_baton2, notify, scratch_pool);
      svn_pool_clear(subpool);
      SVN_ERR(do_merge(NULL, add_result_catalog, add_sources, target_abspath,
                       TRUE, TRUE, same_repos,
                       ignore_ancestry, force, dry_run, TRUE,
                       modified_subtrees, TRUE,
                       TRUE, depth, merge_options, use_sleep, ctx,
                       scratch_pool, subpool));
      svn_pool_clear(subpool);
      SVN_ERR(do_merge(NULL, remove_result_catalog, remove_sources,
                       target_abspath, TRUE, TRUE, same_repos,
                       ignore_ancestry, force, dry_run, TRUE,
                       modified_subtrees, TRUE,
                       TRUE, depth, merge_options, use_sleep, ctx,
                       scratch_pool, subpool));
      SVN_ERR(svn_mergeinfo_catalog_merge(add_result_catalog,
                                          remove_result_catalog,
                                          scratch_pool, scratch_pool));

      if (apr_hash_count(add_result_catalog))
        {
          int i;
          apr_array_header_t *sorted_cat =
            svn_sort__hash(add_result_catalog,
                           svn_sort_compare_items_as_paths,
                           scratch_pool);
          for (i = 0; i < sorted_cat->nelts; i++)
            {
              svn_sort__item_t elt = APR_ARRAY_IDX(sorted_cat, i,
                                                   svn_sort__item_t);
              svn_error_t *err;

              svn_pool_clear(subpool);
              err = svn_client__record_wc_mergeinfo(elt.key,
                                                    elt.value,
                                                    TRUE, ctx,
                                                    subpool);

              if (err && err->apr_err == SVN_ERR_ENTRY_NOT_FOUND)
                {
                  /* PATH isn't just missing, it's not even versioned as far
                     as this working copy knows.  But it was included in
                     MERGES, which means that the server knows about it.
                     Likely we don't have access to the source due to authz
                     restrictions.  For now just clear the error and
                     continue... */
                  svn_error_clear(err);
                }
              else
                {
                  SVN_ERR(err);
                }
            }
        }
    }

  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}

/* Perform checks to determine whether of the working copy at TARGET_ABSPATH
 * can safely be used as a merge target. Checks are performed according to
 * the ALLOW_MIXED_REV, ALLOW_LOCAL_MODS, and ALLOW_SWITCHED_SUBTREES
 * parameters. If any checks fail, raise SVN_ERR_CLIENT_NOT_READY_TO_MERGE.
 *
 * E.g. if all the ALLOW_* parameters are FALSE, TARGET_ABSPATH must
 * be a single-revision, pristine, unswitched working copy.
 * In other words, it must reflect a subtree of the repostiory as found
 * at single revision -- although sparse checkouts are permitted. */
static svn_error_t *
ensure_wc_is_suitable_merge_target(const char *target_abspath,
                                   svn_client_ctx_t *ctx,
                                   svn_boolean_t allow_mixed_rev,
                                   svn_boolean_t allow_local_mods,
                                   svn_boolean_t allow_switched_subtrees,
                                   apr_pool_t *scratch_pool)
{
  /* Avoid the following checks if we don't need them. */
  if (allow_mixed_rev && allow_local_mods && allow_switched_subtrees)
    return SVN_NO_ERROR;

  /* Perform the mixed-revision check first because it's the cheapest one. */
  if (! allow_mixed_rev)
    {
      svn_revnum_t min_rev;
      svn_revnum_t max_rev;

      SVN_ERR(svn_client_min_max_revisions(&min_rev, &max_rev, target_abspath,
                                           FALSE, ctx, scratch_pool));

      if (!(SVN_IS_VALID_REVNUM(min_rev) && SVN_IS_VALID_REVNUM(max_rev)))
        {
          svn_boolean_t is_added;

          /* Allow merge into added nodes. */
          SVN_ERR(svn_wc__node_is_added(&is_added, ctx->wc_ctx, target_abspath,
                                        scratch_pool));
          if (is_added)
            return SVN_NO_ERROR;
          else
            return svn_error_create(SVN_ERR_CLIENT_NOT_READY_TO_MERGE, NULL,
                                    _("Cannot determine revision of working "
                                      "copy"));
        }

      if (min_rev != max_rev)
        return svn_error_createf(SVN_ERR_CLIENT_MERGE_UPDATE_REQUIRED, NULL,
                                 _("Cannot merge into mixed-revision working "
                                   "copy [%lu:%lu]; try updating first"),
                                   min_rev, max_rev);
    }

  /* Next, check for switched subtrees. */
  if (! allow_switched_subtrees)
    {
      svn_boolean_t is_switched;

      SVN_ERR(svn_wc__has_switched_subtrees(&is_switched, ctx->wc_ctx,
                                            target_abspath, NULL,
                                            scratch_pool));
      if (is_switched)
        return svn_error_create(SVN_ERR_CLIENT_NOT_READY_TO_MERGE, NULL,
                                _("Cannot merge into a working copy "
                                  "with a switched subtree"));
    }

  /* This is the most expensive check, so it is performed last.*/
  if (! allow_local_mods)
    {
      svn_boolean_t is_modified;

      SVN_ERR(svn_wc__has_local_mods(&is_modified, ctx->wc_ctx,
                                     target_abspath,
                                     ctx->cancel_func,
                                     ctx->cancel_baton,
                                     scratch_pool));
      if (is_modified)
        return svn_error_create(SVN_ERR_CLIENT_NOT_READY_TO_MERGE, NULL,
                                _("Cannot merge into a working copy "
                                  "that has local modifications"));
    }

  return SVN_NO_ERROR;
}


/*-----------------------------------------------------------------------*/

/*** Public APIs ***/

static svn_error_t *
merge_locked(const char *source1,
             const svn_opt_revision_t *revision1,
             const char *source2,
             const svn_opt_revision_t *revision2,
             const char *target_abspath,
             svn_depth_t depth,
             svn_boolean_t ignore_ancestry,
             svn_boolean_t force,
             svn_boolean_t record_only,
             svn_boolean_t dry_run,
             svn_boolean_t allow_mixed_rev,
             const apr_array_header_t *merge_options,
             svn_client_ctx_t *ctx,
             apr_pool_t *scratch_pool)
{
  const char *URL1, *URL2;
  svn_revnum_t rev1, rev2;
  svn_boolean_t related = FALSE, ancestral = FALSE;
  const char *wc_repos_root, *source_repos_root;
  svn_revnum_t youngest_rev = SVN_INVALID_REVNUM;
  svn_ra_session_t *ra_session1, *ra_session2;
  apr_array_header_t *merge_sources;
  merge_source_t *merge_source;
  svn_error_t *err;
  svn_boolean_t use_sleep = FALSE;
  const char *yc_path = NULL;
  svn_revnum_t yc_rev = SVN_INVALID_REVNUM;
  apr_pool_t *sesspool;
  svn_boolean_t same_repos;
  const char *source_repos_uuid1, *source_repos_uuid2;
  svn_node_kind_t target_kind;

  /* Make sure the target is really there. */
  SVN_ERR(svn_io_check_path(target_abspath, &target_kind, scratch_pool));
  if (target_kind == svn_node_none)
    return svn_error_createf(SVN_ERR_WC_PATH_NOT_FOUND, NULL,
                             _("Path '%s' does not exist"),
                             svn_dirent_local_style(target_abspath,
                                                    scratch_pool));

  /* Sanity check our input -- we require specified revisions,
   * and either 2 paths or 2 URLs. */
  if ((revision1->kind == svn_opt_revision_unspecified)
      || (revision2->kind == svn_opt_revision_unspecified))
    return svn_error_create(SVN_ERR_CLIENT_BAD_REVISION, NULL,
                            _("Not all required revisions are specified"));

  if (svn_path_is_url(source1) != svn_path_is_url(source2))
    return svn_error_create(SVN_ERR_ILLEGAL_TARGET, NULL,
                            _("Merge sources must both be "
                              "either paths or URLs"));

  /* ### FIXME: This function really ought to do a history check on
     the left and right sides of the merge source, and -- if one is an
     ancestor of the other -- just call svn_client_merge_peg3() with
     the appropriate args. */

  /* If source1 or source2 are paths, we need to get the underlying
     URL from the wc and save the initial path we were passed so we
     can use it as a path parameter (either in the baton or not).
     otherwise, the path will just be NULL, which means we won't be
     able to figure out some kind of revision specifications, but in
     that case it won't matter, because those ways of specifying a
     revision are meaningless for a url. */
  SVN_ERR(svn_client_url_from_path2(&URL1, source1, ctx,
                                    scratch_pool, scratch_pool));
  if (! URL1)
    return svn_error_createf(SVN_ERR_ENTRY_MISSING_URL, NULL,
                             _("'%s' has no URL"),
                             svn_dirent_local_style(source1, scratch_pool));

  SVN_ERR(svn_client_url_from_path2(&URL2, source2, ctx,
                                    scratch_pool, scratch_pool));
  if (! URL2)
    return svn_error_createf(SVN_ERR_ENTRY_MISSING_URL, NULL,
                             _("'%s' has no URL"),
                             svn_dirent_local_style(source2, scratch_pool));

  SVN_ERR(svn_wc_read_kind(&target_kind, ctx->wc_ctx, target_abspath, FALSE,
                           scratch_pool));

  if (target_kind != svn_node_dir && target_kind != svn_node_file)
    return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                             _("Merge target '%s' does not exist in the "
                               "working copy"), target_abspath);

  /* Do not allow merges into mixed-revision working copies. */
  SVN_ERR(ensure_wc_is_suitable_merge_target(target_abspath, ctx,
                                             allow_mixed_rev, TRUE, TRUE,
                                             scratch_pool));

  /* Determine the working copy target's repository root URL. */
  SVN_ERR(svn_client__get_repos_root(&wc_repos_root, target_abspath,
                                     ctx, scratch_pool, scratch_pool));

  /* Open some RA sessions to our merge source sides. */
  sesspool = svn_pool_create(scratch_pool);
  SVN_ERR(svn_client__open_ra_session_internal(&ra_session1, NULL, URL1,
                                               NULL, NULL, FALSE, TRUE,
                                               ctx, sesspool));
  SVN_ERR(svn_client__open_ra_session_internal(&ra_session2, NULL, URL2,
                                               NULL, NULL, FALSE, TRUE,
                                               ctx, sesspool));

  /* Resolve revisions to real numbers. */
  SVN_ERR(svn_client__get_revision_number(&rev1, &youngest_rev, ctx->wc_ctx,
                                          NULL, ra_session1, revision1,
                                          sesspool));
  SVN_ERR(svn_client__get_revision_number(&rev2, &youngest_rev, ctx->wc_ctx,
                                          NULL, ra_session2, revision2,
                                          sesspool));

  SVN_ERR(svn_ra_get_uuid2(ra_session1, &source_repos_uuid1, scratch_pool));
  SVN_ERR(svn_ra_get_uuid2(ra_session2, &source_repos_uuid2, scratch_pool));

  /* We can't do a diff between different repositories. */
  if (strcmp(source_repos_uuid1, source_repos_uuid2) != 0)
    return svn_error_createf(SVN_ERR_RA_UUID_MISMATCH, NULL,
                             _("'%s' isn't in the same repository as '%s'"),
                             URL1, URL2);

  /* Get the repository root URL from one of our sessions. */
  SVN_ERR(svn_ra_get_repos_root2(ra_session1, &source_repos_root, sesspool));

  /* Do our working copy and sources come from the same repository? */
  if (strcmp(wc_repos_root, source_repos_root) != 0)
    {
      const char *wc_repos_uuid;

      SVN_ERR(svn_client_uuid_from_path2(&wc_repos_uuid, target_abspath,
                                         ctx, scratch_pool, scratch_pool));
      same_repos = (strcmp(wc_repos_uuid, source_repos_uuid1) == 0);
    }
  else
    same_repos = TRUE;

  /* Unless we're ignoring ancestry, see if the two sources are related.  */
  if (! ignore_ancestry)
    SVN_ERR(svn_client__get_youngest_common_ancestor(&yc_path, &yc_rev,
                                                     URL1, rev1,
                                                     URL2, rev2,
                                                     ctx, scratch_pool));

  /* Check for a youngest common ancestor.  If we have one, we'll be
     doing merge tracking.

     So, given a requested merge of the differences between A and
     B, and a common ancestor of C, we will find ourselves in one of
     four positions, and four different approaches:

        A == B == C   there's nothing to merge

        A == C != B   we merge the changes between A (or C) and B

        B == C != A   we merge the changes between B (or C) and A

        A != B != C   we merge the changes between A and B without
                      merge recording, then record-only two merges:
                      from A to C, and from C to B
  */
  if (yc_path && SVN_IS_VALID_REVNUM(yc_rev))
    {
      apr_array_header_t *ranges;
      svn_opt_revision_range_t *range;
      svn_opt_revision_t peg_revision;
      peg_revision.kind = svn_opt_revision_number;

      /* Note that our merge sources are related. */
      related = TRUE;

      /* Make YC_PATH into a full URL. */
      yc_path = svn_path_url_add_component2(source_repos_root, yc_path,
                                            scratch_pool);

      /* If the common ancestor matches the right side of our merge,
         then we only need to reverse-merge the left side. */
      if ((strcmp(yc_path, URL2) == 0) && (yc_rev == rev2))
        {
          ancestral = TRUE;
          range = apr_pcalloc(scratch_pool, sizeof(*range));
          range->start.kind = svn_opt_revision_number;
          range->start.value.number = rev1;
          range->end.kind = svn_opt_revision_number;
          range->end.value.number = yc_rev;
          ranges = apr_array_make(scratch_pool,
                                  2, sizeof(svn_opt_revision_range_t *));
          APR_ARRAY_PUSH(ranges, svn_opt_revision_range_t *) = range;
          peg_revision.value.number = rev1;
          SVN_ERR(normalize_merge_sources(&merge_sources, URL1, URL1,
                                          source_repos_root, &peg_revision,
                                          ranges, ra_session1, ctx,
                                          scratch_pool, scratch_pool));
        }
      /* If the common ancestor matches the left side of our merge,
         then we only need to merge the right side. */
      else if ((strcmp(yc_path, URL1) == 0) && (yc_rev == rev1))
        {
          ancestral = TRUE;
          range = apr_pcalloc(scratch_pool, sizeof(*range));
          range->start.kind = svn_opt_revision_number;
          range->start.value.number = yc_rev;
          range->end.kind = svn_opt_revision_number;
          range->end.value.number = rev2;
          ranges = apr_array_make(scratch_pool,
                                  2, sizeof(svn_opt_revision_range_t *));
          APR_ARRAY_PUSH(ranges, svn_opt_revision_range_t *) = range;
          peg_revision.value.number = rev2;
          SVN_ERR(normalize_merge_sources(&merge_sources, URL2, URL2,
                                          source_repos_root, &peg_revision,
                                          ranges, ra_session2, ctx,
                                          scratch_pool, scratch_pool));
        }
      /* And otherwise, we need to do both: reverse merge the left
         side, and merge the right. */
      else
        {
          err = merge_cousins_and_supplement_mergeinfo(target_abspath,
                                                       ra_session1,
                                                       ra_session2,
                                                       URL1, rev1,
                                                       URL2, rev2,
                                                       yc_rev,
                                                       source_repos_root,
                                                       wc_repos_root,
                                                       depth,
                                                       ignore_ancestry, force,
                                                       record_only, dry_run,
                                                       merge_options,
                                                       &use_sleep, ctx,
                                                       scratch_pool);
          if (err)
            {
              if (use_sleep)
                svn_io_sleep_for_timestamps(target_abspath, scratch_pool);

              return svn_error_trace(err);
            }

          /* Close our temporary RA sessions (this could've happened
             after the second call to normalize_merge_sources() inside
             the merge_cousins_and_supplement_mergeinfo() routine). */
          svn_pool_destroy(sesspool);

          return SVN_NO_ERROR;
        }
    }
  else
    {
      /* Build a single-item merge_source_t array. */
      merge_sources = apr_array_make(scratch_pool, 1, sizeof(merge_source_t *));
      merge_source = apr_pcalloc(scratch_pool, sizeof(*merge_source));
      merge_source->url1 = URL1;
      merge_source->url2 = URL2;
      merge_source->rev1 = rev1;
      merge_source->rev2 = rev2;
      APR_ARRAY_PUSH(merge_sources, merge_source_t *) = merge_source;
    }

  /* Close our temporary RA sessions. */
  svn_pool_destroy(sesspool);

  err = do_merge(NULL, NULL, merge_sources, target_abspath,
                 ancestral, related, same_repos,
                 ignore_ancestry, force, dry_run,
                 record_only, NULL, FALSE, FALSE, depth, merge_options,
                 &use_sleep, ctx, scratch_pool, scratch_pool);

  if (use_sleep)
    svn_io_sleep_for_timestamps(target_abspath, scratch_pool);

  if (err)
    return svn_error_trace(err);

  return SVN_NO_ERROR;
}

struct merge_baton_t {
  const char *source1;
  const svn_opt_revision_t *revision1;
  const char *source2;
  const svn_opt_revision_t *revision2;
  const char *target_abspath;
  svn_depth_t depth;
  svn_boolean_t ignore_ancestry;
  svn_boolean_t force;
  svn_boolean_t record_only;
  svn_boolean_t dry_run;
  svn_boolean_t allow_mixed_rev;
  const apr_array_header_t *merge_options;
  svn_client_ctx_t *ctx;
};

/* Implements svn_wc__with_write_lock_func_t. */
static svn_error_t *
merge_cb(void *baton, apr_pool_t *result_pool, apr_pool_t *scratch_pool)
{
  struct merge_baton_t *b = baton;

  SVN_ERR(merge_locked(b->source1, b->revision1, b->source2, b->revision2,
                       b->target_abspath, b->depth, b->ignore_ancestry,
                       b->force, b->record_only, b->dry_run,
                       b->allow_mixed_rev, b->merge_options, b->ctx,
                       scratch_pool));

  return SVN_NO_ERROR;
}

/* Set *TARGET_ABSPATH to the absolute path of, and *LOCK_ABSPATH to
 the absolute path to lock for, TARGET_WCPATH. */
static svn_error_t *
get_target_and_lock_abspath(const char **target_abspath,
                            const char **lock_abspath,
                            const char *target_wcpath,
                            svn_client_ctx_t *ctx,
                            apr_pool_t *scratch_pool)
{
  svn_node_kind_t kind;
  SVN_ERR(svn_dirent_get_absolute(target_abspath, target_wcpath,
                                  scratch_pool));
  SVN_ERR(svn_wc_read_kind(&kind, ctx->wc_ctx, *target_abspath, FALSE,
                           scratch_pool));
  if (kind == svn_node_dir)
    *lock_abspath = *target_abspath;
  else
    *lock_abspath = svn_dirent_dirname(*target_abspath, scratch_pool);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client_merge4(const char *source1,
                  const svn_opt_revision_t *revision1,
                  const char *source2,
                  const svn_opt_revision_t *revision2,
                  const char *target_wcpath,
                  svn_depth_t depth,
                  svn_boolean_t ignore_ancestry,
                  svn_boolean_t force,
                  svn_boolean_t record_only,
                  svn_boolean_t dry_run,
                  svn_boolean_t allow_mixed_rev,
                  const apr_array_header_t *merge_options,
                  svn_client_ctx_t *ctx,
                  apr_pool_t *pool)
{
  const char *target_abspath, *lock_abspath;
  struct merge_baton_t merge_baton;

  SVN_ERR(get_target_and_lock_abspath(&target_abspath, &lock_abspath,
                                      target_wcpath, ctx, pool));

  merge_baton.source1 = source1;
  merge_baton.revision1 = revision1;
  merge_baton.source2 = source2;
  merge_baton.revision2 = revision2;
  merge_baton.target_abspath = target_abspath;
  merge_baton.depth = depth;
  merge_baton.ignore_ancestry = ignore_ancestry;
  merge_baton.force = force;
  merge_baton.record_only = record_only;
  merge_baton.dry_run = dry_run;
  merge_baton.allow_mixed_rev = allow_mixed_rev;
  merge_baton.merge_options = merge_options;
  merge_baton.ctx = ctx;

  if (!dry_run)
    SVN_ERR(svn_wc__call_with_write_lock(merge_cb, &merge_baton,
                                         ctx->wc_ctx, lock_abspath, FALSE,
                                         pool, pool));
  else
    SVN_ERR(merge_cb(&merge_baton, pool, pool));

  return SVN_NO_ERROR;
}


/* Check if mergeinfo for a given path is described explicitly or via
   inheritance in a mergeinfo catalog.

   If REPOS_REL_PATH exists in CATALOG and has mergeinfo containing
   MERGEINFO, then set *IN_CATALOG to TRUE.  If REPOS_REL_PATH does
   not exist in CATALOG, then find its nearest parent which does exist.
   If the mergeinfo REPOS_REL_PATH would inherit from that parent
   contains MERGEINFO then set *IN_CATALOG to TRUE.  Set *IN_CATALOG
   to FALSE in all other cases.

   Set *CAT_KEY_PATH to the key path in CATALOG for REPOS_REL_PATH's
   explicit or inherited mergeinfo.  If no explicit or inherited mergeinfo
   is found for REPOS_REL_PATH then set *CAT_KEY_PATH to NULL.

   User RESULT_POOL to allocate *CAT_KEY_PATH.  Use SCRATCH_POOL for
   temporary allocations. */
static svn_error_t *
mergeinfo_in_catalog(svn_boolean_t *in_catalog,
                     const char **cat_key_path,
                     const char *repos_rel_path,
                     svn_mergeinfo_t mergeinfo,
                     svn_revnum_t revision,
                     svn_mergeinfo_catalog_t catalog,
                     apr_pool_t *result_pool,
                     apr_pool_t *scratch_pool)
{
  const char *walk_path = NULL;

  *in_catalog = FALSE;
  *cat_key_path = NULL;

  if (mergeinfo && catalog && apr_hash_count(catalog))
    {
      const char *path = repos_rel_path;

      /* Start with the assumption there is no explicit or inherited
         mergeinfo for REPOS_REL_PATH in CATALOG. */
      svn_mergeinfo_t mergeinfo_in_cat = NULL;

      while (1)
        {
          mergeinfo_in_cat = apr_hash_get(catalog, path, APR_HASH_KEY_STRING);

          if (mergeinfo_in_cat) /* Found it! */
            {
              *cat_key_path = apr_pstrdup(result_pool, path);
              break;
            }
          else /* Look for inherited mergeinfo. */
            {
              walk_path = svn_relpath_join(svn_relpath_basename(path,
                                                                scratch_pool),
                                           walk_path ? walk_path : "",
                                           scratch_pool);
              path = svn_relpath_dirname(path, scratch_pool);

              if (path[0] == '\0') /* No mergeinfo to inherit. */
                break;
            }
        }

      if (mergeinfo_in_cat)
        {
          if (walk_path)
            SVN_ERR(svn_mergeinfo__add_suffix_to_mergeinfo(&mergeinfo_in_cat,
                                                           mergeinfo_in_cat,
                                                           walk_path,
                                                           scratch_pool,
                                                           scratch_pool));
          SVN_ERR(svn_mergeinfo_intersect2(&mergeinfo_in_cat,
                                           mergeinfo_in_cat, mergeinfo,
                                           TRUE,
                                           scratch_pool, scratch_pool));
          SVN_ERR(svn_mergeinfo__equals(in_catalog, mergeinfo_in_cat,
                                        mergeinfo, TRUE, scratch_pool));
        }
    }

  return SVN_NO_ERROR;
}

/* A svn_log_entry_receiver_t baton for log_find_operative_revs(). */
typedef struct log_find_operative_baton_t
{
  /* The catalog of explicit mergeinfo on a reintegrate source. */
  svn_mergeinfo_catalog_t merged_catalog;

  /* The catalog of unmerged history from the reintegrate target to
     the source which we will create.  Allocated in RESULT_POOL. */
  svn_mergeinfo_catalog_t unmerged_catalog;

  /* The repository absolute path of the reintegrate target. */
  const char *target_abspath;

  /* The path of the reintegrate source relative to the repository root. */
  const char *source_repos_rel_path;

  apr_pool_t *result_pool;
} log_find_operative_baton_t;

/* A svn_log_entry_receiver_t callback for find_unsynced_ranges(). */
static svn_error_t *
log_find_operative_revs(void *baton,
                        svn_log_entry_t *log_entry,
                        apr_pool_t *pool)
{
  log_find_operative_baton_t *log_baton = baton;
  apr_hash_index_t *hi;
  svn_revnum_t revision;

  revision = log_entry->revision;

  for (hi = apr_hash_first(pool, log_entry->changed_paths2);
       hi;
       hi = apr_hash_next(hi))
    {
      const char *subtree_missing_this_rev;
      const char *path = svn__apr_hash_index_key(hi);
      const char *rel_path;
      const char *source_rel_path;
      svn_boolean_t in_catalog;
      svn_mergeinfo_t log_entry_as_mergeinfo;

      rel_path = svn_fspath__skip_ancestor(log_baton->target_abspath, path);
      /* Easy out: The path is not within the tree of interest. */
      if (rel_path == NULL)
        continue;

      source_rel_path = svn_relpath_join(log_baton->source_repos_rel_path,
                                         rel_path, pool);

      SVN_ERR(svn_mergeinfo_parse(&log_entry_as_mergeinfo,
                                  apr_psprintf(pool, "%s:%ld",
                                               path, revision),
                                  pool));

      SVN_ERR(mergeinfo_in_catalog(&in_catalog, &subtree_missing_this_rev,
                                   source_rel_path, log_entry_as_mergeinfo,
                                   revision, log_baton->merged_catalog,
                                   pool, pool));

      if (!in_catalog)
        {
          svn_mergeinfo_t unmerged_for_key;
          const char *missing_path;

          /* If there is no mergeinfo on the source tree we'll say
             the "subtree" missing this revision is the root of the
             source. */
          if (!subtree_missing_this_rev)
            subtree_missing_this_rev = log_baton->source_repos_rel_path;

          if (subtree_missing_this_rev
              && strcmp(subtree_missing_this_rev, source_rel_path))
            {
              const char *suffix =
                svn_relpath_skip_ancestor(subtree_missing_this_rev,
                                          source_rel_path);
              missing_path = apr_pstrmemdup(pool, path,
                                            strlen(path) - strlen(suffix) - 1);
            }
          else
            {
              missing_path = path;
            }

          SVN_ERR(svn_mergeinfo_parse(&log_entry_as_mergeinfo,
                                      apr_psprintf(pool, "%s:%ld",
                                                   missing_path, revision),
                                      log_baton->result_pool));
          unmerged_for_key = apr_hash_get(log_baton->unmerged_catalog,
                                          subtree_missing_this_rev,
                                          APR_HASH_KEY_STRING);

          if (unmerged_for_key)
            {
              SVN_ERR(svn_mergeinfo_merge(unmerged_for_key,
                                          log_entry_as_mergeinfo,
                                          log_baton->result_pool));
            }
          else
            {
              apr_hash_set(log_baton->unmerged_catalog,
                           apr_pstrdup(log_baton->result_pool,
                                       subtree_missing_this_rev),
                           APR_HASH_KEY_STRING,
                           log_entry_as_mergeinfo);
            }

        }
    }
  return SVN_NO_ERROR;
}

/* Determine if the mergeinfo on a reintegrate source SOURCE_REPOS_REL_PATH,
   reflects that the source is fully synced with the reintegrate target
   TARGET_REPOS_REL_PATH, even if a naive interpretation of the source's
   mergeinfo says otherwise -- See issue #3577.

   UNMERGED_CATALOG represents the history (as mergeinfo) from
   TARGET_REPOS_REL_PATH that is not represented in SOURCE_REPOS_REL_PATH's
   explicit/inherited mergeinfo as represented by MERGED_CATALOG.
   MERGEINFO_CATALOG may be empty if the source has no explicit or inherited
   mergeinfo.

   Using RA_SESSION, which is pointed at TARGET_REPOS_REL_PATH, check that all
   of the unmerged revisions in UNMERGED_CATALOG's mergeinfos are "phantoms",
   that is, one of the following conditions holds:

     1) The revision affects no corresponding paths in SOURCE_REPOS_REL_PATH.

     2) The revision affects corresponding paths in SOURCE_REPOS_REL_PATH,
        but based on the mergeinfo in MERGED_CATALOG, the change was
        previously merged.

   Make a deep copy, allocated in RESULT_POOL, of any portions of
   UNMERGED_CATALOG that are not phantoms, to TRUE_UNMERGED_CATALOG.

   Note: The keys in all mergeinfo catalogs used here are relative to the
   root of the repository.

   Use SCRATCH_POOL for all temporary allocations. */
static svn_error_t *
find_unsynced_ranges(const char *source_repos_rel_path,
                     const char *target_repos_rel_path,
                     svn_mergeinfo_catalog_t unmerged_catalog,
                     svn_mergeinfo_catalog_t merged_catalog,
                     svn_mergeinfo_catalog_t true_unmerged_catalog,
                     svn_ra_session_t *ra_session,
                     apr_pool_t *result_pool,
                     apr_pool_t *scratch_pool)
{
  apr_array_header_t *potentially_unmerged_ranges = NULL;

  /* Convert all the unmerged history to a rangelist. */
  if (apr_hash_count(unmerged_catalog))
    {
      apr_hash_index_t *hi_catalog;

      potentially_unmerged_ranges =
        apr_array_make(scratch_pool, 1, sizeof(svn_merge_range_t *));

      for (hi_catalog = apr_hash_first(scratch_pool, unmerged_catalog);
           hi_catalog;
           hi_catalog = apr_hash_next(hi_catalog))
        {
          svn_mergeinfo_t mergeinfo = svn__apr_hash_index_val(hi_catalog);
          apr_hash_index_t *hi_mergeinfo;

          for (hi_mergeinfo = apr_hash_first(scratch_pool, mergeinfo);
               hi_mergeinfo;
               hi_mergeinfo = apr_hash_next(hi_mergeinfo))
            {
              apr_array_header_t *rangelist =
                svn__apr_hash_index_val(hi_mergeinfo);

              SVN_ERR(svn_rangelist_merge(&potentially_unmerged_ranges,
                                          rangelist, scratch_pool));
            }
        }
    }

  /* Find any unmerged revisions which both affect the source and
     are not yet merged to it. */
  if (potentially_unmerged_ranges)
    {
      svn_revnum_t oldest_rev =
        (APR_ARRAY_IDX(potentially_unmerged_ranges,
                       0,
                       svn_merge_range_t *))->start + 1;
      svn_revnum_t youngest_rev =
        (APR_ARRAY_IDX(potentially_unmerged_ranges,
                       potentially_unmerged_ranges->nelts - 1,
                       svn_merge_range_t *))->end;
      apr_array_header_t *log_targets = apr_array_make(scratch_pool, 1,
                                                       sizeof(const char *));
      log_find_operative_baton_t log_baton;

      log_baton.merged_catalog = merged_catalog;
      log_baton.unmerged_catalog = true_unmerged_catalog;
      log_baton.source_repos_rel_path = source_repos_rel_path;
      log_baton.target_abspath = apr_psprintf(scratch_pool, "/%s",
                                              target_repos_rel_path);
      log_baton.result_pool = result_pool;

      APR_ARRAY_PUSH(log_targets, const char *) = "";

      SVN_ERR(svn_ra_get_log2(ra_session, log_targets, youngest_rev,
                              oldest_rev, 0, TRUE, FALSE, FALSE,
                              NULL, log_find_operative_revs, &log_baton,
                              scratch_pool));
    }

  return SVN_NO_ERROR;
}


/* Helper for calculate_left_hand_side() which produces a mergeinfo catalog
   describing what parts of of the reintegrate target have not previously been
   merged to the reintegrate source.

   SOURCE_CATALOG is the collection of explicit mergeinfo on
   SOURCE_REPOS_REL_PATH@SOURCE_REV and all its children, i.e. the mergeinfo
   catalog for the reintegrate source.

   TARGET_SEGMENTS_HASH is a hash of (const char *) paths mapped to
   (apr_array_header_t *) arrays of svn_location_segment_t *.  Each of these
   path keys represent a path in the reintegrate target, relative to the
   repository root, which has explicit mergeinfo and/or is the reintegrate
   target itself.  The segment arrays contain the natural history of each
   path@TARGET_REV.  Effectively this is the mergeinfo catalog on the
   reintegrate target.

   YC_ANCESTOR_REV is the revision of the youngest common ancestor of the
   reintegrate source and the reintegrate target.

   SOURCE_REPOS_REL_PATH is the path of the reintegrate source relative to
   the root of the repository.  TARGET_REPOS_REL_PATH is the path of the
   reintegrate target relative to the root of the repository.

   TARGET_REV is the working revision the entire WC tree rooted at
   TARGET_REPOS_REL_PATH is at.  SOURCE_REV is the peg revision of the
   reintegrate source.

   SOURCE_RA_SESSION is a session opened to the SOURCE_REPOS_REL_PATH
   and TARGET_RA_SESSION is open to TARGET_REPOS_REL_PATH.

   For each path/segment in TARGET_SEGMENTS_HASH check that the history that
   segment represents is contained in either the explicit mergeinfo for the
   corresponding path in SOURCE_CATALOG, the corresponding path's inherited
   mergeinfo (if no explicit mergeinfo for the path is found in
   SOURCE_CATALOG), or the corresponding path's natural history.  Populate
   *UNMERGED_TO_SOURCE_CATALOG with the corresponding source paths mapped to
   the mergeinfo from the target's natural history which is *not* found.  Also
   include any mergeinfo from SOURCE_CATALOG which explicitly describes the
   target's history but for which *no* path/segment was found in
   TARGET_SEGMENTS_HASH.

   If no part of TARGET_SEGMENTS_HASH is found in SOURCE_CATALOG set
   *NEVER_SYNCHED to TRUE and set *YOUNGEST_MERGED_REV to SVN_INVALID_REVNUM.
   Otherwise set *NEVER_SYNCHED to FALSE, *YOUNGEST_MERGED_REV to the youngest
   revision previously merged from the target to the source, and filter
   *UNMERGED_TO_SOURCE_CATALOG so that it contains no ranges greater than
   *YOUNGEST_MERGED_REV.

   *UNMERGED_TO_SOURCE_CATALOG is (deeply) allocated in RESULT_POOL.
    SCRATCH_POOL is used for all temporary allocations.  */
static svn_error_t *
find_unmerged_mergeinfo(svn_mergeinfo_catalog_t *unmerged_to_source_catalog,
                        svn_boolean_t *never_synched,
                        svn_revnum_t *youngest_merged_rev,
                        svn_revnum_t yc_ancestor_rev,
                        svn_mergeinfo_catalog_t source_catalog,
                        apr_hash_t *target_segments_hash,
                        const char *source_repos_rel_path,
                        const char *target_repos_rel_path,
                        svn_revnum_t target_rev,
                        svn_revnum_t source_rev,
                        svn_ra_session_t *source_ra_session,
                        svn_ra_session_t *target_ra_session,
                        svn_client_ctx_t *ctx,
                        apr_pool_t *result_pool,
                        apr_pool_t *scratch_pool)
{
  apr_hash_index_t *hi;
  svn_mergeinfo_catalog_t new_catalog = apr_hash_make(result_pool);
  svn_mergeinfo_t target_history_as_mergeinfo, source_history_as_mergeinfo;
  svn_mergeinfo_t common_mergeinfo;
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  svn_revnum_t old_rev, young_rev;

  *never_synched = TRUE;
  *youngest_merged_rev = SVN_INVALID_REVNUM;

  /* Examine the natural history of each path in the reintegrate target
     with explicit mergeinfo. */
  for (hi = apr_hash_first(scratch_pool, target_segments_hash);
       hi;
       hi = apr_hash_next(hi))
    {
      const char *path = svn__apr_hash_index_key(hi);
      apr_array_header_t *segments = svn__apr_hash_index_val(hi);
      const char *source_path;
      const char *source_path_rel_to_session;
      svn_mergeinfo_t source_mergeinfo, filtered_mergeinfo;

      svn_pool_clear(iterpool);

      source_path_rel_to_session = path + strlen(target_repos_rel_path);
      if (source_path_rel_to_session[0] == '/')  /* Remove leading '/'. */
        source_path_rel_to_session++;
      source_path = svn_relpath_join(source_repos_rel_path,
                                     source_path_rel_to_session, iterpool);

      /* Convert this target path's natural history into mergeinfo. */
      SVN_ERR(svn_mergeinfo__mergeinfo_from_segments(
        &target_history_as_mergeinfo, segments, iterpool));

      /* Remove any target history that is also part of the source's history,
         i.e. their common ancestry.  By definition this has already been
         "merged" from the target to the source.  If the source has explict
         self referential mergeinfo it would intersect with the target's
         history below, making it appear that some merges had been done from
         the target to the source, when this might not actually be the case. */
      SVN_ERR(svn_mergeinfo__filter_mergeinfo_by_ranges(
        &target_history_as_mergeinfo, target_history_as_mergeinfo,
        source_rev, yc_ancestor_rev, TRUE, iterpool, iterpool));

      /* Look for any explicit mergeinfo on the source path corresponding to
         the target path.  If we find any remove that from SOURCE_CATALOG.
         When this iteration over TARGET_SEGMENTS_HASH is complete all that
         should be left in SOURCE_CATALOG are subtrees that have explicit
         mergeinfo on the reintegrate source where there is no corresponding
         explicit mergeinfo on the reintegrate target. */
      source_mergeinfo = apr_hash_get(source_catalog, source_path,
                                      APR_HASH_KEY_STRING);
      if (source_mergeinfo)
        {
          svn_mergeinfo_t explicit_source_target_history_intersection;

          apr_hash_set(source_catalog, source_path, APR_HASH_KEY_STRING,
                       NULL);
          /* If there is an intersection between the *explicit* mergeinfo on
             this source path and the corresponding target's history then we
             know that at least one merge was done from the target to the
             source. */
          SVN_ERR(svn_mergeinfo_intersect2(
            &explicit_source_target_history_intersection,
            source_mergeinfo, target_history_as_mergeinfo, TRUE,
            iterpool, iterpool));
          if (apr_hash_count(explicit_source_target_history_intersection))
            {
              *never_synched = FALSE;
              /* Keep track of the youngest revision merged from the
                 target to the source. */
              SVN_ERR(svn_mergeinfo__get_range_endpoints(
                &young_rev, &old_rev,
                explicit_source_target_history_intersection,
                iterpool));
              if (!SVN_IS_VALID_REVNUM(*youngest_merged_rev)
                  || (young_rev > *youngest_merged_rev))
                *youngest_merged_rev = young_rev;
            }
        }
      else
        {
          /* There is no mergeinfo on source_path *or* source_path doesn't
             exist at all.  If simply doesn't exist we can ignore it
             altogether. */
          svn_node_kind_t kind;
          svn_mergeinfo_catalog_t subtree_catalog;
          apr_array_header_t *source_repos_rel_path_as_array;

          SVN_ERR(svn_ra_check_path(source_ra_session,
                                    source_path_rel_to_session,
                                    source_rev, &kind, iterpool));
          if (kind == svn_node_none)
              continue;
          /* Else source_path does exist though it has no explicit mergeinfo.
             Find its inherited mergeinfo.  If it doesn't have any then simply
             set source_mergeinfo to an empty hash. */
          source_repos_rel_path_as_array =
            apr_array_make(iterpool, 1, sizeof(const char *));
          APR_ARRAY_PUSH(source_repos_rel_path_as_array, const char *)
            = source_path_rel_to_session;
          SVN_ERR(svn_ra_get_mergeinfo(source_ra_session, &subtree_catalog,
                                        source_repos_rel_path_as_array,
                                        source_rev, svn_mergeinfo_inherited,
                                       FALSE, iterpool));
          if (subtree_catalog)
            source_mergeinfo = apr_hash_get(subtree_catalog,
                                            source_path_rel_to_session,
                                            APR_HASH_KEY_STRING);

          /* A path might not have any inherited mergeinfo either. */
          if (!source_mergeinfo)
            source_mergeinfo = apr_hash_make(iterpool);
        }

      /* Get the source path's natural history and convert it to mergeinfo.
         Then merge that natural history into source path's explicit
         or inherited mergeinfo. */
      SVN_ERR(svn_client__repos_location_segments(&segments,
                                                  source_ra_session,
                                                  source_path_rel_to_session,
                                                  source_rev, source_rev,
                                                  SVN_INVALID_REVNUM,
                                                  ctx, iterpool));
      SVN_ERR(svn_mergeinfo__mergeinfo_from_segments(
        &source_history_as_mergeinfo, segments, iterpool));
      SVN_ERR(svn_mergeinfo_merge(source_mergeinfo,
                                  source_history_as_mergeinfo, iterpool));

      /* Now source_mergeinfo represents everything we know about
         source_path's history.  Now we need to know what part, if any, of the
         corresponding target's history is *not* part of source_path's total
         history; because it is neither shared history nor was it ever merged
         from the target to the source. */
      SVN_ERR(svn_mergeinfo_intersect2(&common_mergeinfo,
                                       source_mergeinfo,
                                       target_history_as_mergeinfo, TRUE,
                                       iterpool, iterpool));

      /* Use scratch_pool rather than iterpool because filtered_mergeinfo is
         going into new_catalog below and needs to last to the end of
         this function. */
      SVN_ERR(svn_mergeinfo_remove2(&filtered_mergeinfo,
                                    common_mergeinfo,
                                    target_history_as_mergeinfo, TRUE,
                                    scratch_pool, iterpool));

      /* As with svn_mergeinfo_intersect above, we need to use scratch_pool
         rather than iterpool. */
      apr_hash_set(new_catalog,
                   apr_pstrdup(scratch_pool, source_path),
                   APR_HASH_KEY_STRING,
                   filtered_mergeinfo);
    }

  /* Are there any subtrees with explicit mergeinfo still left in the merge
     source where there was no explicit mergeinfo for the corresponding path
     in the merge target?  If so, add the intersection of those path's
     mergeinfo and the corresponding target path's mergeinfo to
     new_catalog. */
  if (apr_hash_count(source_catalog))
    {
      for (hi = apr_hash_first(scratch_pool, source_catalog);
           hi;
           hi = apr_hash_next(hi))
        {
          const char *source_path = svn__apr_hash_index_key(hi);
          const char *source_path_rel_to_session =
            svn_relpath_skip_ancestor(source_repos_rel_path, source_path);
          svn_mergeinfo_t source_mergeinfo = svn__apr_hash_index_val(hi);
          svn_mergeinfo_t filtered_mergeinfo;
          const char *target_path;
          apr_array_header_t *segments;
          svn_error_t *err;

          svn_pool_clear(iterpool);

          target_path = source_path + strlen(source_repos_rel_path);
          if (target_path[0] == '/')  /* Remove leading '/'. */
            target_path++;
          err = svn_client__repos_location_segments(&segments,
                                                    target_ra_session,
                                                    target_path,
                                                    target_rev, target_rev,
                                                    SVN_INVALID_REVNUM,
                                                    ctx, iterpool);
          if (err)
            {
              if (err->apr_err == SVN_ERR_FS_NOT_FOUND
                  || err->apr_err == SVN_ERR_RA_DAV_REQUEST_FAILED)
                {
                  /* This path with explicit mergeinfo in the source doesn't
                     exist on the target. */
                  svn_error_clear(err);
                  err = NULL;
                }
              else
                {
                  return svn_error_trace(err);
                }
            }
          else
            {
              svn_mergeinfo_t explicit_source_target_history_intersection;

              SVN_ERR(svn_mergeinfo__mergeinfo_from_segments(
                &target_history_as_mergeinfo, segments, iterpool));

              /* If there is an intersection between the *explicit* mergeinfo
                 on this source path and the corresponding target's history
                 then we know that at least one merge was done from the target
                 to the source. */
              SVN_ERR(svn_mergeinfo_intersect2(
                &explicit_source_target_history_intersection,
                source_mergeinfo, target_history_as_mergeinfo, TRUE,
                iterpool, iterpool));

              if (apr_hash_count(explicit_source_target_history_intersection))
                {
                  *never_synched = FALSE;
                  /* Keep track of the youngest revision merged from the
                     target to the source. */
                  SVN_ERR(svn_mergeinfo__get_range_endpoints(
                    &young_rev, &old_rev,
                    explicit_source_target_history_intersection, iterpool));
                  if (!SVN_IS_VALID_REVNUM(*youngest_merged_rev)
                      || (young_rev > *youngest_merged_rev))
                    *youngest_merged_rev = young_rev;
                }

              /* Get the source path's natural history and convert it to
                 mergeinfo.  Then merge that natural history into source
                 path's explicit or inherited mergeinfo. */
              SVN_ERR(svn_client__repos_location_segments(
                &segments,
                source_ra_session,
                source_path_rel_to_session,
                target_rev,
                target_rev,
                SVN_INVALID_REVNUM,
                ctx, iterpool));
              SVN_ERR(svn_mergeinfo__mergeinfo_from_segments(
                &source_history_as_mergeinfo, segments, iterpool));
              SVN_ERR(svn_mergeinfo_merge(source_mergeinfo,
                                          source_history_as_mergeinfo,
                                          iterpool));
              SVN_ERR(svn_mergeinfo_intersect2(&common_mergeinfo,
                                               source_mergeinfo,
                                               target_history_as_mergeinfo,
                                               TRUE, iterpool, iterpool));
              /* Use subpool rather than iterpool because filtered_mergeinfo
                 is  going into new_catalog below and needs to last to the
                 end of this function. */
              SVN_ERR(svn_mergeinfo_remove2(&filtered_mergeinfo,
                                            common_mergeinfo,
                                            target_history_as_mergeinfo,
                                            TRUE, scratch_pool, iterpool));
              if (apr_hash_count(filtered_mergeinfo))
                apr_hash_set(new_catalog,
                             apr_pstrdup(scratch_pool, source_path),
                             APR_HASH_KEY_STRING,
                             filtered_mergeinfo);
            }
        }
    }

  /* Limit new_catalog to the youngest revisions previously merged from
     the target to the source. */
  if (SVN_IS_VALID_REVNUM(*youngest_merged_rev))
    SVN_ERR(svn_mergeinfo__filter_catalog_by_ranges(&new_catalog,
                                                    new_catalog,
                                                    *youngest_merged_rev,
                                                    0, /* No oldest bound. */
                                                    TRUE,
                                                    scratch_pool,
                                                    scratch_pool));

  /* Make a shiny new copy before blowing away all the temporary pools. */
  *unmerged_to_source_catalog = svn_mergeinfo_catalog_dup(new_catalog,
                                                          result_pool);
  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

/* Helper for svn_client_merge_reintegrate() which calculates the
   'left hand side' of the underlying two-URL merge that a --reintegrate
   merge actually performs.  If no merge should be performed, set
   *URL_LEFT to NULL and *REV_LEFT to SVN_INVALID_REVNUM.

   TARGET_ABSPATH is the absolute working copy path of the reintegrate
   merge.

   TARGET_REPOS_REL_PATH is the path of TARGET_ABSPATH relative to
   the root of the repository.  SOURCE_REPOS_REL_PATH is the path of the
   reintegrate source relative to the root of the repository.

   SUBTREES_WITH_MERGEINFO is a hash of (const char *) absolute paths mapped
   to (svn_string_t *) mergeinfo values for each working copy path with
   explicit mergeinfo in TARGET_ABSPATH.

   TARGET_REV is the working revision the entire WC tree rooted at
   TARGET_REPOS_REL_PATH is at.  SOURCE_REV is the peg revision of the
   reintegrate source.

   Populate *UNMERGED_TO_SOURCE_CATALOG with the mergeinfo describing what
   parts of TARGET_REPOS_REL_PATH@TARGET_REV have not been merged to
   SOURCE_REPOS_REL_PATH@SOURCE_REV, up to the youngest revision ever merged
   from the TARGET_ABSPATH to the source if such exists, see doc string for
   find_unmerged_mergeinfo().

   SOURCE_RA_SESSION is a session opened to the SOURCE_REPOS_REL_PATH
   and TARGET_RA_SESSION is open to TARGET_REPOS_REL_PATH.

   *URL_LEFT, *MERGED_TO_SOURCE_CATALOG , and *UNMERGED_TO_SOURCE_CATALOG are
   allocated in RESULT_POOL.  SCRATCH_POOL is used for all temporary
   allocations. */
static svn_error_t *
calculate_left_hand_side(const char **url_left,
                         svn_revnum_t *rev_left,
                         svn_mergeinfo_t *merged_to_source_catalog,
                         svn_mergeinfo_t *unmerged_to_source_catalog,
                         const char *target_abspath,
                         const char *target_repos_rel_path,
                         apr_hash_t *subtrees_with_mergeinfo,
                         svn_revnum_t target_rev,
                         const char *source_repos_rel_path,
                         const char *source_repos_root,
                         const char *target_repos_root,
                         svn_revnum_t source_rev,
                         svn_ra_session_t *source_ra_session,
                         svn_ra_session_t *target_ra_session,
                         svn_client_ctx_t *ctx,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool)
{
  apr_array_header_t *segments; /* array of (svn_location_segment_t *) */
  svn_mergeinfo_catalog_t mergeinfo_catalog, unmerged_catalog;
  apr_array_header_t *source_repos_rel_path_as_array
    = apr_array_make(scratch_pool, 1, sizeof(const char *));
  apr_pool_t *iterpool = svn_pool_create(scratch_pool);
  apr_hash_index_t *hi;
  /* hash of paths mapped to arrays of svn_location_segment_t *. */
  apr_hash_t *segments_hash = apr_hash_make(scratch_pool);
  svn_boolean_t never_synced;
  svn_revnum_t youngest_merged_rev;
  const char *yc_ancestor_path;
  svn_revnum_t yc_ancestor_rev;
  const char *source_url;
  const char *target_url;

  /* Initialize our return variables. */
  *url_left = NULL;
  *rev_left = SVN_INVALID_REVNUM;

  /* TARGET_ABSPATH may not have explicit mergeinfo and thus may not be
     contained within SUBTREES_WITH_MERGEINFO.  If this is the case then
     add a dummy item for TARGET_ABSPATH so we get its history (i.e. implicit
     mergeinfo) below.  */
  if (!apr_hash_get(subtrees_with_mergeinfo, target_abspath,
                    APR_HASH_KEY_STRING))
    apr_hash_set(subtrees_with_mergeinfo, target_abspath,
                 APR_HASH_KEY_STRING, svn_string_create("", result_pool));

  /* Get the history (segments) for TARGET_ABSPATH and any of its subtrees
     with explicit mergeinfo. */
  for (hi = apr_hash_first(scratch_pool, subtrees_with_mergeinfo);
       hi;
       hi = apr_hash_next(hi))
    {
      const char *absolute_path = svn__apr_hash_index_key(hi);
      svn_string_t *mergeinfo_string = svn__apr_hash_index_val(hi);
      const char *path_rel_to_session;
      const char *path_rel_to_root;
      svn_mergeinfo_t subtree_mergeinfo;
      svn_error_t *err;

      svn_pool_clear(iterpool);

      /* Issue #3896: If invalid mergeinfo in the reintegrate target
         prevents us from proceeding, then raise the best error possible. */
      err = svn_mergeinfo_parse(&subtree_mergeinfo, mergeinfo_string->data,
                                iterpool);
      if (err)
        {
          if (err->apr_err == SVN_ERR_MERGEINFO_PARSE_ERROR)
            {
              err = svn_error_createf(
                SVN_ERR_CLIENT_INVALID_MERGEINFO_NO_MERGETRACKING, err,
                _("Invalid mergeinfo detected on '%s', "
                  "reintegrate merge not possible"),
                svn_dirent_local_style(absolute_path, scratch_pool));
            }
          return svn_error_trace(err);
        }

      /* Convert the absolute path with mergeinfo on it to a path relative
         to the session root. */
      SVN_ERR(svn_client__path_relative_to_root(&path_rel_to_root,
                                                ctx->wc_ctx, absolute_path,
                                                target_repos_root, FALSE,
                                                NULL, scratch_pool,
                                                iterpool));
      path_rel_to_session = svn_relpath_skip_ancestor(target_repos_rel_path,
                                                      path_rel_to_root);
      SVN_ERR(svn_client__repos_location_segments(&segments,
                                                  target_ra_session,
                                                  path_rel_to_session,
                                                  target_rev, target_rev,
                                                  SVN_INVALID_REVNUM,
                                                  ctx, scratch_pool));
      apr_hash_set(segments_hash,
                   apr_pstrdup(scratch_pool, path_rel_to_root),
                   APR_HASH_KEY_STRING, segments);
    }

  /* Check that SOURCE_URL@SOURCE_REV and TARGET_URL@TARGET_REV are
     actually related, we can't reintegrate if they are not.  Also
     get an initial value for YC_ANCESTOR_REV. */
  source_url = svn_path_url_add_component2(source_repos_root,
                                           source_repos_rel_path,
                                           iterpool);
  target_url = svn_path_url_add_component2(source_repos_root,
                                           target_repos_rel_path,
                                           iterpool);
  SVN_ERR(svn_client__get_youngest_common_ancestor(&yc_ancestor_path,
                                                   &yc_ancestor_rev,
                                                   source_url, source_rev,
                                                   target_url, target_rev,
                                                   ctx, iterpool));
  if (!(yc_ancestor_path && SVN_IS_VALID_REVNUM(yc_ancestor_rev)))
    return svn_error_createf(SVN_ERR_CLIENT_NOT_READY_TO_MERGE, NULL,
                             _("'%s@%ld' must be ancestrally related to "
                               "'%s@%ld'"), source_url, source_rev,
                             target_url, target_rev);

  /* If the source revision is the same as the youngest common
     revision, then there can't possibly be any unmerged revisions
     that we need to apply to target. */
  if (source_rev == yc_ancestor_rev)
    {
      svn_pool_destroy(iterpool);
      return SVN_NO_ERROR;
    }

  /* Get the mergeinfo from the source, including its descendants
     with differing explicit mergeinfo. */
  APR_ARRAY_PUSH(source_repos_rel_path_as_array, const char *) = "";
  SVN_ERR(svn_ra_get_mergeinfo(source_ra_session, &mergeinfo_catalog,
                                source_repos_rel_path_as_array, source_rev,
                                svn_mergeinfo_inherited,
                               TRUE, iterpool));

  if (mergeinfo_catalog)
    SVN_ERR(svn_mergeinfo__add_prefix_to_catalog(&mergeinfo_catalog,
                                                 mergeinfo_catalog,
                                                 source_repos_rel_path,
                                                 iterpool, iterpool));

  if (!mergeinfo_catalog)
    mergeinfo_catalog = apr_hash_make(iterpool);

  *merged_to_source_catalog = svn_mergeinfo_catalog_dup(mergeinfo_catalog,
                                                        result_pool);

  /* Filter the source's mergeinfo catalog so that we are left with
     mergeinfo that describes what has *not* previously been merged from
     TARGET_REPOS_REL_PATH@TARGET_REV to SOURCE_REPOS_REL_PATH@SOURCE_REV. */
  SVN_ERR(find_unmerged_mergeinfo(&unmerged_catalog,
                                  &never_synced,
                                  &youngest_merged_rev,
                                  yc_ancestor_rev,
                                  mergeinfo_catalog,
                                  segments_hash,
                                  source_repos_rel_path,
                                  target_repos_rel_path,
                                  target_rev,
                                  source_rev,
                                  source_ra_session,
                                  target_ra_session,
                                  ctx,
                                  iterpool, iterpool));

  /* Simplify unmerged_catalog through elision then make a copy in POOL. */
  SVN_ERR(svn_client__elide_mergeinfo_catalog(unmerged_catalog,
                                              iterpool));
  *unmerged_to_source_catalog = svn_mergeinfo_catalog_dup(unmerged_catalog,
                                                          result_pool);

  if (never_synced)
    {
      /* We never merged to the source.  Just return the branch point. */
      *url_left = svn_path_url_add_component2(source_repos_root,
                                              yc_ancestor_path, result_pool);
      *rev_left = yc_ancestor_rev;
    }
  else
    {
      /* We've previously merged some or all of the target, up to
         youngest_merged_rev, from TARGET_ABSPATH to the source.  Set
         *URL_LEFT and *REV_LEFT to cover the youngest part of this range. */
      svn_opt_revision_t peg_revision, youngest_rev, unspecified_rev;
      svn_opt_revision_t *start_revision;
      const char *youngest_url;

      peg_revision.kind = svn_opt_revision_number;
      peg_revision.value.number = target_rev;

      youngest_rev.kind = svn_opt_revision_number;
      youngest_rev.value.number = youngest_merged_rev;

      unspecified_rev.kind = svn_opt_revision_unspecified;

      *rev_left = youngest_rev.value.number;

      /* Get the URL of the target_url@youngest_merged_rev. */
      SVN_ERR(svn_client__repos_locations(&youngest_url, &start_revision,
                                          NULL, NULL, target_ra_session,
                                          target_url, &peg_revision,
                                          &youngest_rev, &unspecified_rev,
                                          ctx, iterpool));

      *url_left = apr_pstrdup(result_pool, youngest_url);
    }

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

static svn_error_t *
merge_reintegrate_locked(const char *source,
                         const svn_opt_revision_t *peg_revision,
                         const char *target_abspath,
                         svn_boolean_t dry_run,
                         const apr_array_header_t *merge_options,
                         svn_client_ctx_t *ctx,
                         apr_pool_t *scratch_pool)
{
  const char *wc_repos_root, *source_repos_root;
  svn_opt_revision_t working_revision;
  svn_ra_session_t *target_ra_session;
  svn_ra_session_t *source_ra_session;
  const char *source_repos_rel_path, *target_repos_rel_path;
  const char *yc_ancestor_path;
  svn_revnum_t yc_ancestor_rev;
  const char *url1, *url2;
  svn_revnum_t rev1, rev2;
  svn_mergeinfo_t unmerged_to_source_mergeinfo_catalog;
  svn_mergeinfo_t final_unmerged_catalog;
  svn_mergeinfo_t merged_to_source_mergeinfo_catalog;
  svn_boolean_t use_sleep = FALSE;
  svn_error_t *err;
  apr_hash_t *subtrees_with_mergeinfo;
  const char *target_url;
  svn_revnum_t target_base_rev;
  svn_node_kind_t kind;
  svn_opt_revision_t no_rev;

  /* Make sure the target is really there. */
  SVN_ERR(svn_io_check_path(target_abspath, &kind, scratch_pool));
  if (kind == svn_node_none)
    return svn_error_createf(SVN_ERR_WC_PATH_NOT_FOUND, NULL,
                             _("Path '%s' does not exist"),
                             svn_dirent_local_style(target_abspath,
                                                    scratch_pool));

  /* Make sure we're dealing with a real URL. */
  SVN_ERR(svn_client_url_from_path2(&url2, source, ctx,
                                    scratch_pool, scratch_pool));
  if (! url2)
    return svn_error_createf(SVN_ERR_ENTRY_MISSING_URL, NULL,
                             _("'%s' has no URL"),
                             svn_dirent_local_style(source, scratch_pool));

  /* Determine the working copy target's repository root URL. */
  SVN_ERR(svn_client__get_repos_root(&wc_repos_root, target_abspath,
                                     ctx, scratch_pool, scratch_pool));

  /* Determine the source's repository root URL. */
  SVN_ERR(svn_client__get_repos_root(&source_repos_root, url2,
                                     ctx, scratch_pool, scratch_pool));

  /* source_repos_root and wc_repos_root are required to be the same,
     as mergeinfo doesn't come into play for cross-repository merging. */
  if (strcmp(source_repos_root, wc_repos_root) != 0)
    return svn_error_createf(SVN_ERR_CLIENT_UNRELATED_RESOURCES, NULL,
                             _("'%s' must be from the same repository as "
                               "'%s'"), svn_dirent_local_style(source,
                                                               scratch_pool),
                             svn_dirent_local_style(target_abspath,
                                                    scratch_pool));

  /* A reintegrate merge requires the merge target to reflect a subtree
   * of the repository as found at a single revision. */
  SVN_ERR(ensure_wc_is_suitable_merge_target(target_abspath, ctx,
                                             FALSE, FALSE, FALSE,
                                             scratch_pool));
  SVN_ERR(svn_wc__node_get_base_rev(&target_base_rev, ctx->wc_ctx,
                                    target_abspath, scratch_pool));

  /* As the WC tree is "pure", use its last-updated-to revision as
     the default revision for the left side of our merge, since that's
     what the repository sub-tree is required to be up to date with
     (with regard to the WC). */
  rev1 = target_base_rev;

  source_repos_rel_path = svn_uri_skip_ancestor(wc_repos_root, url2,
                                                scratch_pool);
  SVN_ERR(svn_client__path_relative_to_root(&target_repos_rel_path,
                                            ctx->wc_ctx, target_abspath,
                                            wc_repos_root, FALSE, NULL,
                                            scratch_pool, scratch_pool));

  /* Can't reintegrate to or from the root of the repository. */
  if (svn_path_is_empty(source_repos_rel_path)
      || svn_path_is_empty(target_repos_rel_path))
    return svn_error_createf(SVN_ERR_CLIENT_NOT_READY_TO_MERGE, NULL,
                             _("Neither the reintegrate source nor target "
                               "can be the root of the repository"));

  /* Find all the subtree's in TARGET_WCPATH that have explicit mergeinfo. */
  working_revision.kind = svn_opt_revision_working;
  SVN_ERR(svn_client_propget4(&subtrees_with_mergeinfo, SVN_PROP_MERGEINFO,
                              target_abspath, &working_revision,
                              &working_revision, NULL, svn_depth_infinity,
                              NULL, ctx, scratch_pool, scratch_pool));

  if (apr_hash_count(subtrees_with_mergeinfo))
    {
      apr_hash_t *externals;
      apr_hash_index_t *hi;

      SVN_ERR(svn_wc__externals_defined_below(&externals, ctx->wc_ctx,
                                              target_abspath, scratch_pool,
                                              scratch_pool));

      for (hi = apr_hash_first(scratch_pool, subtrees_with_mergeinfo);
           hi;
           hi = apr_hash_next(hi))
        {
          const char *wc_path = svn__apr_hash_index_key(hi);

          /* svn_client_propget4 picks up file externals with
             mergeinfo, but we don't want those. */
          if (apr_hash_get(externals, wc_path, APR_HASH_KEY_STRING))
            {
              apr_hash_set(subtrees_with_mergeinfo, wc_path,
                           APR_HASH_KEY_STRING, NULL);
              continue;
            }
        }
    }

  /* Open two RA sessions, one to our source and one to our target. */
  no_rev.kind = svn_opt_revision_unspecified;
  SVN_ERR(svn_client__ra_session_from_path(&source_ra_session, &rev2, &url2,
                                           url2, NULL, peg_revision, &no_rev,
                                           ctx, scratch_pool));
  SVN_ERR(svn_wc__node_get_url(&target_url, ctx->wc_ctx, target_abspath,
                               scratch_pool, scratch_pool));
  SVN_ERR(svn_client__open_ra_session_internal(&target_ra_session, NULL,
                                               target_url,
                                               NULL, NULL, FALSE, FALSE,
                                               ctx, scratch_pool));

  SVN_ERR(calculate_left_hand_side(&url1, &rev1,
                                   &merged_to_source_mergeinfo_catalog,
                                   &unmerged_to_source_mergeinfo_catalog,
                                   target_abspath,
                                   target_repos_rel_path,
                                   subtrees_with_mergeinfo,
                                   rev1,
                                   source_repos_rel_path,
                                   source_repos_root,
                                   wc_repos_root,
                                   rev2,
                                   source_ra_session,
                                   target_ra_session,
                                   ctx,
                                   scratch_pool, scratch_pool));

  /* Did calculate_left_hand_side() decide that there was no merge to
     be performed here?  */
  if (! url1)
    return SVN_NO_ERROR;

  /* If the target was moved after the source was branched from it,
     it is possible that the left URL differs from the target's current
     URL.  If so, then adjust TARGET_RA_SESSION to point to the old URL. */
  if (strcmp(url1, target_url))
    SVN_ERR(svn_ra_reparent(target_ra_session, url1, scratch_pool));

  SVN_ERR(svn_client__get_youngest_common_ancestor(&yc_ancestor_path,
                                                   &yc_ancestor_rev,
                                                   url2, rev2,
                                                   url1, rev1,
                                                   ctx, scratch_pool));

  if (!(yc_ancestor_path && SVN_IS_VALID_REVNUM(yc_ancestor_rev)))
    return svn_error_createf(SVN_ERR_CLIENT_NOT_READY_TO_MERGE, NULL,
                             _("'%s@%ld' must be ancestrally related to "
                               "'%s@%ld'"), url1, rev1, url2, rev2);

  if (rev1 > yc_ancestor_rev)
    {
      /* Have we actually merged anything to the source from the
         target?  If so, make sure we've merged a contiguous
         prefix. */
      final_unmerged_catalog = apr_hash_make(scratch_pool);

      SVN_ERR(find_unsynced_ranges(source_repos_rel_path,
                                   yc_ancestor_path,
                                   unmerged_to_source_mergeinfo_catalog,
                                   merged_to_source_mergeinfo_catalog,
                                   final_unmerged_catalog,
                                   target_ra_session, scratch_pool,
                                   scratch_pool));

      if (apr_hash_count(final_unmerged_catalog))
        {
          svn_string_t *source_mergeinfo_cat_string;

          SVN_ERR(svn_mergeinfo__catalog_to_formatted_string(
            &source_mergeinfo_cat_string,
            final_unmerged_catalog,
            "  ", "    Missing ranges: ", scratch_pool));
          return svn_error_createf(SVN_ERR_CLIENT_NOT_READY_TO_MERGE,
                                   NULL,
                                   _("Reintegrate can only be used if "
                                     "revisions %ld through %ld were "
                                     "previously merged from %s to the "
                                     "reintegrate source, but this is "
                                     "not the case:\n%s"),
                                   yc_ancestor_rev + 1, rev2,
                                   target_url,
                                   source_mergeinfo_cat_string->data);
        }
    }

  /* Left side: trunk@youngest-trunk-rev-merged-to-branch-at-specified-peg-rev
   * Right side: branch@specified-peg-revision */

  /* Do the real merge! */
  /* ### TODO(reint): Make sure that one isn't the same line ancestor
     ### of the other (what's erroneously referred to as "ancestrally
     ### related" in this source file).  We can merge to trunk without
     ### implementing this. */
  err = merge_cousins_and_supplement_mergeinfo(target_abspath,
                                               target_ra_session,
                                               source_ra_session,
                                               url1, rev1, url2, rev2,
                                               yc_ancestor_rev,
                                               source_repos_root,
                                               wc_repos_root,
                                               svn_depth_infinity,
                                               FALSE, FALSE, FALSE, dry_run,
                                               merge_options, &use_sleep,
                                               ctx, scratch_pool);

  if (use_sleep)
    svn_io_sleep_for_timestamps(target_abspath, scratch_pool);

  if (err)
    return svn_error_trace(err);

  return SVN_NO_ERROR;
}

struct merge_reintegrate_baton_t {
  const char *source;
  const svn_opt_revision_t *peg_revision;
  const char *target_abspath;
  svn_boolean_t dry_run;
  const apr_array_header_t *merge_options;
  svn_client_ctx_t *ctx;
};

/* Implements svn_wc__with_write_lock_func_t. */
static svn_error_t *
merge_reintegrate_cb(void *baton,
                     apr_pool_t *result_pool,
                     apr_pool_t *scratch_pool)
{
  struct merge_reintegrate_baton_t *b = baton;

  SVN_ERR(merge_reintegrate_locked(b->source, b->peg_revision,
                                   b->target_abspath, b->dry_run,
                                   b->merge_options, b->ctx,
                                   scratch_pool));

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client_merge_reintegrate(const char *source,
                             const svn_opt_revision_t *peg_revision,
                             const char *target_wcpath,
                             svn_boolean_t dry_run,
                             const apr_array_header_t *merge_options,
                             svn_client_ctx_t *ctx,
                             apr_pool_t *pool)
{
  const char *target_abspath, *lock_abspath;
  struct merge_reintegrate_baton_t merge_reintegrate_baton;

  SVN_ERR(get_target_and_lock_abspath(&target_abspath, &lock_abspath,
                                      target_wcpath, ctx, pool));

  merge_reintegrate_baton.source = source;
  merge_reintegrate_baton.peg_revision = peg_revision;
  merge_reintegrate_baton.target_abspath = target_abspath;
  merge_reintegrate_baton.dry_run = dry_run;
  merge_reintegrate_baton.merge_options = merge_options;
  merge_reintegrate_baton.ctx = ctx;

  if (!dry_run)
    SVN_ERR(svn_wc__call_with_write_lock(merge_reintegrate_cb,
                                         &merge_reintegrate_baton,
                                         ctx->wc_ctx, lock_abspath, FALSE,
                                         pool, pool));
  else
    SVN_ERR(merge_reintegrate_cb(&merge_reintegrate_baton, pool, pool));

  return SVN_NO_ERROR;
}


static svn_error_t *
merge_peg_locked(const char *source,
                 const apr_array_header_t *ranges_to_merge,
                 const svn_opt_revision_t *peg_revision,
                 const char *target_abspath,
                 svn_depth_t depth,
                 svn_boolean_t ignore_ancestry,
                 svn_boolean_t force,
                 svn_boolean_t record_only,
                 svn_boolean_t dry_run,
                 svn_boolean_t allow_mixed_rev,
                 const apr_array_header_t *merge_options,
                 svn_client_ctx_t *ctx,
                 apr_pool_t *scratch_pool)
{
  const char *URL;
  apr_array_header_t *merge_sources;
  const char *wc_repos_root, *source_repos_root;
  svn_ra_session_t *ra_session;
  apr_pool_t *sesspool;
  svn_boolean_t use_sleep = FALSE;
  svn_error_t *err;
  svn_boolean_t same_repos;
  svn_node_kind_t target_kind;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(target_abspath));

  /* Make sure the target is really there. */
  SVN_ERR(svn_io_check_path(target_abspath, &target_kind, scratch_pool));
  if (target_kind == svn_node_none)
    return svn_error_createf(SVN_ERR_WC_PATH_NOT_FOUND, NULL,
                             _("Path '%s' does not exist"),
                             svn_dirent_local_style(target_abspath,
                                                    scratch_pool));

  /* Make sure we're dealing with a real URL. */
  SVN_ERR(svn_client_url_from_path2(&URL, source, ctx,
                                    scratch_pool, scratch_pool));
  if (! URL)
    return svn_error_createf(SVN_ERR_ENTRY_MISSING_URL, NULL,
                             _("'%s' has no URL"),
                             svn_dirent_local_style(source, scratch_pool));

  SVN_ERR(svn_wc_read_kind(&target_kind, ctx->wc_ctx, target_abspath, FALSE,
                           scratch_pool));
  if (target_kind != svn_node_dir && target_kind != svn_node_file)
    return svn_error_createf(SVN_ERR_ILLEGAL_TARGET, NULL,
                             _("Merge target '%s' does not exist in the "
                               "working copy"), target_abspath);

  SVN_ERR(ensure_wc_is_suitable_merge_target(target_abspath, ctx,
                                             allow_mixed_rev, TRUE, TRUE,
                                             scratch_pool));

  /* Determine the working copy target's repository root URL. */
  SVN_ERR(svn_client__get_repos_root(&wc_repos_root, target_abspath,
                                     ctx, scratch_pool, scratch_pool));

  /* Open an RA session to our source URL, and determine its root URL. */
  sesspool = svn_pool_create(scratch_pool);
  SVN_ERR(svn_client__open_ra_session_internal(&ra_session, NULL, URL, NULL,
                                               NULL, FALSE, TRUE,
                                               ctx, sesspool));
  SVN_ERR(svn_ra_get_repos_root2(ra_session, &source_repos_root, scratch_pool));

  /* Normalize our merge sources. */
  SVN_ERR(normalize_merge_sources(&merge_sources, source, URL,
                                  source_repos_root, peg_revision,
                                  ranges_to_merge, ra_session, ctx,
                                  scratch_pool, scratch_pool));

  /* Check for same_repos. */
  if (strcmp(wc_repos_root, source_repos_root) != 0)
    {
      const char *source_repos_uuid;
      const char *wc_repos_uuid;

      SVN_ERR(svn_ra_get_uuid2(ra_session, &source_repos_uuid, scratch_pool));
      SVN_ERR(svn_client_uuid_from_path2(&wc_repos_uuid, target_abspath,
                                         ctx, scratch_pool, scratch_pool));
      same_repos = (strcmp(wc_repos_uuid, source_repos_uuid) == 0);
    }
  else
    same_repos = TRUE;

  /* We're done with our little RA session. */
  svn_pool_destroy(sesspool);

  /* Do the real merge!  (We say with confidence that our merge
     sources are both ancestral and related.) */
  err = do_merge(NULL, NULL, merge_sources, target_abspath,
                 TRUE, TRUE, same_repos, ignore_ancestry, force, dry_run,
                 record_only, NULL, FALSE, FALSE, depth, merge_options,
                 &use_sleep, ctx, scratch_pool, scratch_pool);

  if (use_sleep)
    svn_io_sleep_for_timestamps(target_abspath, scratch_pool);

  if (err)
    return svn_error_trace(err);

  return SVN_NO_ERROR;
}

struct merge_peg_baton_t {
  const char *source;
  const apr_array_header_t *ranges_to_merge;
  const svn_opt_revision_t *peg_revision;
  const char *target_abspath;
  svn_depth_t depth;
  svn_boolean_t ignore_ancestry;
  svn_boolean_t force;
  svn_boolean_t record_only;
  svn_boolean_t dry_run;
  svn_boolean_t allow_mixed_rev;
  const apr_array_header_t *merge_options;
  svn_client_ctx_t *ctx;
};

/* Implements svn_wc__with_write_lock_func_t. */
static svn_error_t *
merge_peg_cb(void *baton, apr_pool_t *result_pool, apr_pool_t *scratch_pool)
{
  struct merge_peg_baton_t *b = baton;

  SVN_ERR(merge_peg_locked(b->source, b->ranges_to_merge, b->peg_revision,
                           b->target_abspath, b->depth, b->ignore_ancestry,
                           b->force, b->record_only, b->dry_run,
                           b->allow_mixed_rev, b->merge_options, b->ctx,
                           scratch_pool));

  return SVN_NO_ERROR;
}

svn_error_t *
svn_client_merge_peg4(const char *source,
                      const apr_array_header_t *ranges_to_merge,
                      const svn_opt_revision_t *peg_revision,
                      const char *target_wcpath,
                      svn_depth_t depth,
                      svn_boolean_t ignore_ancestry,
                      svn_boolean_t force,
                      svn_boolean_t record_only,
                      svn_boolean_t dry_run,
                      svn_boolean_t allow_mixed_rev,
                      const apr_array_header_t *merge_options,
                      svn_client_ctx_t *ctx,
                      apr_pool_t *pool)
{
  const char *target_abspath, *lock_abspath;
  struct merge_peg_baton_t merge_peg_baton;

  /* No ranges to merge?  No problem. */
  if (ranges_to_merge->nelts == 0)
    return SVN_NO_ERROR;

  SVN_ERR(get_target_and_lock_abspath(&target_abspath, &lock_abspath,
                                      target_wcpath, ctx, pool));

  merge_peg_baton.source = source;
  merge_peg_baton.ranges_to_merge = ranges_to_merge;
  merge_peg_baton.peg_revision = peg_revision;
  merge_peg_baton.target_abspath = target_abspath;
  merge_peg_baton.depth = depth;
  merge_peg_baton.ignore_ancestry = ignore_ancestry;
  merge_peg_baton.force = force;
  merge_peg_baton.record_only = record_only;
  merge_peg_baton.dry_run = dry_run;
  merge_peg_baton.allow_mixed_rev = allow_mixed_rev;
  merge_peg_baton.merge_options = merge_options;
  merge_peg_baton.ctx = ctx;

  if (!dry_run)
    SVN_ERR(svn_wc__call_with_write_lock(merge_peg_cb, &merge_peg_baton,
                                         ctx->wc_ctx, lock_abspath, FALSE,
                                         pool, pool));
  else
    SVN_ERR(merge_peg_cb(&merge_peg_baton, pool, pool));

  return SVN_NO_ERROR;
}
