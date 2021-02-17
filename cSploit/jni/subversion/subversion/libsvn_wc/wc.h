/*
 * wc.h :  shared stuff internal to the svn_wc library.
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


#ifndef SVN_LIBSVN_WC_H
#define SVN_LIBSVN_WC_H

#include <apr_pools.h>
#include <apr_hash.h>

#include "svn_types.h"
#include "svn_error.h"
#include "svn_wc.h"

#include "private/svn_sqlite.h"
#include "private/svn_wc_private.h"
#include "private/svn_skel.h"

#include "wc_db.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define SVN_WC__PROP_REJ_EXT  ".prej"

/* We can handle this format or anything lower, and we (should) error
 * on anything higher.
 *
 * There is no format version 0; we started with 1.
 *
 * The bump to 2 introduced the ".svn-work" extension. For example,
 *   ".svn/props/foo" became ".svn/props/foo.svn-work".
 *
 * The bump to 3 introduced the entry attribute
 *   old-and-busted.c::ENTRIES_ATTR_ABSENT.
 *
 * The bump to 4 renamed the magic "svn:this_dir" entry name to "".
 *
 * == 1.0.x shipped with format 4
 * == 1.1.x shipped with format 4
 * == 1.2.x shipped with format 4
 * == 1.3.x shipped with format 4
 *
 * The bump to 5 added support for replacing files with history (the
 *   "revert base"). This was introduced in 1.4.0, but buggy until 1.4.6.
 *
 * The bump to 6 introduced caching of property modification state and
 *   certain properties in the entries file.
 *
 * The bump to 7 changed the entries file format from XML to a custom
 *   text-based format.
 *
 * The bump to 8 placed wcprops in one file per directory (named
 *   upgrade.c::WCPROPS_ALL_DATA)
 *
 * == 1.4.x shipped with format 8
 *
 * The bump to 9 added changelists, keep-local, and sticky depth (for
 *   selective/sparse checkouts) to each entry.
 *
 * == 1.5.x shipped with format 9
 *
 * The bump to 10 added tree-conflicts, file externals and a different
 *   canonicalization of urls.
 *
 * == 1.6.x shipped with format 10
 *
 * The bump to 11 cleared the has_props, has_prop_mods, cachable_props,
 *   and present_props values in the entries file. Older clients expect
 *   proper values for these fields.
 *
 * The bump to 12 switched from 'entries' to the SQLite database 'wc.db'.
 *
 * The bump to 13 added the WORK_QUEUE table into 'wc.db', moved the
 *   wcprops into the 'dav_cache' column in BASE_NODE, and stopped using
 *   the 'incomplete_children' column of BASE_NODE.
 *
 * The bump to 14 added the WCLOCKS table (and migrated locks from the
 *   filesystem into wc.db), and some columns to ACTUAL_NODE for future
 *   use.
 *
 * The bump to 15 switched from depth='exclude' on directories to using
 *   presence='exclude' within the BASE_NODE and WORKING_NODE tables.
 *   This change also enabled exclude support on files and symlinks.
 *
 * The bump to 16 added 'locked_levels' to WC_LOCK, setting any existing
 *   locks to a level of 0. The 'md5_checksum' column was added to PRISTINE
 *   for future use.
 *
 * The bump to 17 added a '.svn/pristine' dir and moved the text bases into
 *   the Pristine Store (the PRISTINE table and '.svn/pristine' dir), and
 *   removed the '/.svn/text-base' dir.
 *
 * The bump to 18 moved the properties from separate files in the props and
 *   prop-base directory (and .svn for the dir itself) into the wc.db file,
 *   and then removed the props and prop-base dir.
 *
 * The bump to 19 introduced the 'single DB' per working copy. All metadata
 *   is held in a single '.svn/wc.db' in the root directory of the working
 *   copy. Bumped in r991236.
 *
 * The bump to 20 introduced NODES and drops BASE_NODE and WORKING_NODE,
 *   op_depth is always 0 or 2. Bumped in r1005388.
 *
 * The bump to 21 moved tree conflict storage from the parent to the
 *   conflicted node. Bumped in r1034436.
 *
 * The bump to 22 moved tree conflict storage from conflict_data column
 *   to the tree_conflict_data column. Bumped in r1040255.
 *
 * The bump to 23 introduced multi-layer op_depth processing for NODES.
 *   Bumped in r1044384.
 *
 * The bump to 24 started using the 'refcount' column of the PRISTINE table
 *   correctly, instead of always setting it to '1'. Bumped in r1058523.
 *
 * The bump to 25 introduced the NODES_CURRENT view. Bumped in r1071283.
 *
 * The bump to 26 introduced the NODES_BASE view. Bumped in r1076617.
 *
 * The bump to 27 stored conflict files as relpaths rather than basenames.
 *   Bumped in r1089593.
 *
 * The bump to 28 converted any remaining references to MD5 checksums
 *   to SHA1 checksums. Bumped in r1095214.
 *
 * The bump to 29 renamed the pristine files from '<SHA1>' to '<SHA1>.svn-base'
 * and introduced the EXTERNALS store. Bumped in r1129286.
 *
 * == 1.7.x shipped with format ???
 *
 * Please document any further format changes here.
 */

#define SVN_WC__VERSION 29


/* Formats <= this have no concept of "revert text-base/props".  */
#define SVN_WC__NO_REVERT_FILES 4

/* A version <= this has wcprops stored in one file per entry. */
#define SVN_WC__WCPROPS_MANY_FILES_VERSION 7

/* A version < this can have urls that aren't canonical according to the new
   rules. See issue #2475. */
#define SVN_WC__CHANGED_CANONICAL_URLS 10

/* The format number written to wc-ng working copies so that old clients
   can recognize them as "newer Subversion"'s working copies. */
#define SVN_WC__NON_ENTRIES 12
#define SVN_WC__NON_ENTRIES_STRING "12\n"

/* A version < this uses the old 'entries' file mechanism.  */
#define SVN_WC__WC_NG_VERSION 12

/* In this version, the wcprops are "lost" between files and wc.db. We want
   to ignore them in upgrades.  */
#define SVN_WC__WCPROPS_LOST 12

/* A version < this has no work queue (see workqueue.h).  */
#define SVN_WC__HAS_WORK_QUEUE 13

/* Return true iff error E indicates an "is not a working copy" type
   of error, either because something wasn't a working copy at all, or
   because it's a working copy from a previous version (in need of
   upgrade). */
#define SVN_WC__ERR_IS_NOT_CURRENT_WC(e) \
            ((e->apr_err == SVN_ERR_WC_NOT_WORKING_COPY) || \
             (e->apr_err == SVN_ERR_WC_UPGRADE_REQUIRED))



/*** Context handling ***/
struct svn_wc_context_t
{
  /* The wc_db handle for this working copy. */
  svn_wc__db_t *db;

  /* Close the DB when we destroy this context?
     (This is used inside backward compat wrappers, and should only be
      modified by the proper create() functions. */
  svn_boolean_t close_db_on_destroy;

  /* The state pool for this context. */
  apr_pool_t *state_pool;
};

/**
 * Just like svn_wc_context_create(), only use the provided DB to construct
 * the context.
 *
 * Even though DB is not allocated from the same pool at *WC_CTX, it is
 * expected to remain open throughout the life of *WC_CTX.
 */
svn_error_t *
svn_wc__context_create_with_db(svn_wc_context_t **wc_ctx,
                               svn_config_t *config,
                               svn_wc__db_t *db,
                               apr_pool_t *result_pool);


/*** Committed Queue ***/

/**
 * Return the pool associated with QUEUE.  (This so we can keep some
 * deprecated functions that need to peek inside the QUEUE struct in
 * deprecated.c).
 */
apr_pool_t *
svn_wc__get_committed_queue_pool(const struct svn_wc_committed_queue_t *queue);


/** Internal helper for svn_wc_process_committed_queue2().
 *
 * ### If @a queue is NULL, then ...?
 * ### else:
 * Bump an item from @a queue (the one associated with @a
 * local_abspath) to @a new_revnum after a commit succeeds, recursing
 * if @a recurse is set.
 *
 * @a new_date is the (server-side) date of the new revision, or 0.
 *
 * @a rev_author is the (server-side) author of the new
 * revision; it may be @c NULL.
 *
 * @a new_dav_cache is a hash of dav property changes to be made to
 * the @a local_abspath.
 *   ### [JAF]  Is it? See svn_wc_queue_committed3(). It ends up being
 *   ### assigned as a whole to wc.db:BASE_NODE:dav_cache.
 *
 * If @a no_unlock is set, don't release any user locks on @a
 * local_abspath; otherwise release them as part of this processing.
 *
 * If @a keep_changelist is set, don't remove any changeset assignments
 * from @a local_abspath; otherwise, clear it of such assignments.
 *
 * If @a sha1_checksum is non-NULL, use it to identify the node's pristine
 * text.
 *
 * Set TOP_OF_RECURSE to TRUE to show that this the top of a possibly
 * recursive commit operation.
 */
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
                                   apr_pool_t *scratch_pool);


/*** Update traversals. ***/

struct svn_wc_traversal_info_t
{
  /* The pool in which this structure and everything inside it is
     allocated. */
  apr_pool_t *pool;

  /* The before and after values of the SVN_PROP_EXTERNALS property,
   * for each directory on which that property changed.  These have
   * the same layout as those returned by svn_wc_edited_externals().
   *
   * The hashes, their keys, and their values are allocated in the
   * above pool.
   */
  apr_hash_t *externals_old;
  apr_hash_t *externals_new;

  /* The ambient depths of the working copy directories.  The keys are
     working copy paths (as for svn_wc_edited_externals()), the values
     are the result of svn_depth_to_word(depth_of_each_dir). */
  apr_hash_t *depths;
};


/*** Names and file/dir operations in the administrative area. ***/

/** The files within the administrative subdir. **/
#define SVN_WC__ADM_FORMAT              "format"
#define SVN_WC__ADM_ENTRIES             "entries"
#define SVN_WC__ADM_TMP                 "tmp"
#define SVN_WC__ADM_PRISTINE            "pristine"
#define SVN_WC__ADM_NONEXISTENT_PATH    "nonexistent-path"

/* The basename of the ".prej" file, if a directory ever has property
   conflicts.  This .prej file will appear *within* the conflicted
   directory.  */
#define SVN_WC__THIS_DIR_PREJ           "dir_conflicts"


/* A few declarations for stuff in util.c.
 * If this section gets big, move it all out into a new util.h file. */

/* Ensure that DIR exists. */
svn_error_t *svn_wc__ensure_directory(const char *path, apr_pool_t *pool);


/* Return a hash keyed by 'const char *' property names and with
   'svn_string_t *' values built from PROPS (which is an array of
   pointers to svn_prop_t's) or to NULL if PROPS is NULL or empty.
   PROPS items which lack a value will be ignored.  If PROPS contains
   multiple properties with the same name, each successive such item
   reached in a walk from the beginning to the end of the array will
   overwrite the previous in the returned hash.

   NOTE: While the returned hash will be allocated in RESULT_POOL, the
   items it holds will share storage with those in PROPS.

   ### This is rather the reverse of svn_prop_hash_to_array(), except
   ### that function's arrays contains svn_prop_t's, whereas this
   ### one's contains *pointers* to svn_prop_t's.  So much for
   ### consistency.  */
apr_hash_t *
svn_wc__prop_array_to_hash(const apr_array_header_t *props,
                           apr_pool_t *result_pool);


/* Set *MODIFIED_P to non-zero if LOCAL_ABSPATH's text is modified with
 * regard to the base revision, else set *MODIFIED_P to zero.
 *
 * If EXACT_COMPARISON is FALSE, translate LOCAL_ABSPATH's EOL
 * style and keywords to repository-normal form according to its properties,
 * and compare the result with the text base.  If COMPARE_TEXTBASES is
 * TRUE, translate the text base's EOL style and keywords to working-copy
 * form according to LOCAL_ABSPATH's properties, and compare the
 * result with LOCAL_ABSPATH.  Usually, EXACT_COMPARISON should be FALSE.
 *
 * If LOCAL_ABSPATH does not exist, consider it unmodified.  If it exists
 * but is not under revision control (not even scheduled for
 * addition), return the error SVN_WC_PATH_NOT_FOUND.
 *
 * If the text is unmodified and a write-lock is held this function
 * will ensure that the last-known-unmodified timestamp and
 * filesize of the file as recorded in DB matches the corresponding
 * attributes of the actual file.  (This is often referred to as
 * "timestamp repair", and serves to help future unforced is-modified
 * checks return quickly if the file remains untouched.)
 */
svn_error_t *
svn_wc__internal_file_modified_p(svn_boolean_t *modified_p,
                                 svn_wc__db_t *db,
                                 const char *local_abspath,
                                 svn_boolean_t exact_comparison,
                                 apr_pool_t *scratch_pool);


/* Merge the difference between LEFT_ABSPATH and RIGHT_ABSPATH into
   TARGET_ABSPATH, return the appropriate work queue operations in
   *WORK_ITEMS.

   Note that, in the case of updating, the update can have sent new
   properties, which could affect the way the wc target is
   detranslated and compared with LEFT and RIGHT for merging.

   The merge result is stored in *MERGE_OUTCOME and merge conflicts
   are marked in MERGE_RESULT using LEFT_LABEL, RIGHT_LABEL and
   TARGET_LABEL.

   When DRY_RUN is true, no actual changes are made to the working copy.

   If DIFF3_CMD is specified, the given external diff3 tool will
   be used instead of our built in diff3 routines.

   When MERGE_OPTIONS are specified, they are used by the internal
   diff3 routines, or passed to the external diff3 tool.

   If CONFLICT_FUNC is non-NULL, then call it with CONFLICT_BATON if a
   conflict is encountered, giving the callback a chance to resolve
   the conflict (before marking the file 'conflicted').

   When LEFT_VERSION and RIGHT_VERSION are non-NULL, pass them to the
   conflict resolver as older_version and their_version.

   ## TODO: We should store the information in LEFT_VERSION and RIGHT_VERSION
            in the workingcopy for future retrieval via svn info.

   WRI_ABSPATH describes in which working copy information should be
   retrieved. (Interesting for merging file externals).

   ACTUAL_PROPS is the set of actual properties before merging; used for
   detranslating the file before merging.

   Property changes sent by the update are provided in PROP_DIFF.

   For a complete description, see svn_wc_merge3() for which this is
   the (loggy) implementation.

   *WORK_ITEMS will be allocated in RESULT_POOL. All temporary allocations
   will be performed in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__internal_merge(svn_skel_t **work_items,
                       enum svn_wc_merge_outcome_t *merge_outcome,
                       svn_wc__db_t *db,
                       const char *left_abspath,
                       const svn_wc_conflict_version_t *left_version,
                       const char *right_abspath,
                       const svn_wc_conflict_version_t *right_version,
                       const char *target_abspath,
                       const char *wri_abspath,
                       const char *left_label,
                       const char *right_label,
                       const char *target_label,
                       apr_hash_t *actual_props,
                       svn_boolean_t dry_run,
                       const char *diff3_cmd,
                       const apr_array_header_t *merge_options,
                       const apr_array_header_t *prop_diff,
                       svn_wc_conflict_resolver_func2_t conflict_func,
                       void *conflict_baton,
                       svn_cancel_func_t cancel_func,
                       void *cancel_baton,
                       apr_pool_t *result_pool,
                       apr_pool_t *scratch_pool);

/* A default error handler for svn_wc_walk_entries3().  Returns ERR in
   all cases. */
svn_error_t *
svn_wc__walker_default_error_handler(const char *path,
                                     svn_error_t *err,
                                     void *walk_baton,
                                     apr_pool_t *pool);

/* Set *EDITOR and *EDIT_BATON to an ambient-depth-based filtering
 * editor that wraps WRAPPED_EDITOR and WRAPPED_BATON.  This is only
 * required for operations where the requested depth is @c
 * svn_depth_unknown and the server's editor driver doesn't understand
 * depth.  It is safe for *EDITOR and *EDIT_BATON to start as
 * WRAPPED_EDITOR and WRAPPED_BATON.
 *
 * ANCHOR, TARGET, and DB are as in svn_wc_get_update_editor3.
 *
 * @a requested_depth must be one of the following depth values:
 * @c svn_depth_infinity, @c svn_depth_empty, @c svn_depth_files,
 * @c svn_depth_immediates, or @c svn_depth_unknown.
 *
 * Allocations are done in POOL.
 */
svn_error_t *
svn_wc__ambient_depth_filter_editor(const svn_delta_editor_t **editor,
                                    void **edit_baton,
                                    svn_wc__db_t *db,
                                    const char *anchor_abspath,
                                    const char *target,
                                    const svn_delta_editor_t *wrapped_editor,
                                    void *wrapped_edit_baton,
                                    apr_pool_t *result_pool);


/* Similar to svn_wc_conflicted_p3(), but with a wc_db parameter in place of
 * a wc_context. */
svn_error_t *
svn_wc__internal_conflicted_p(svn_boolean_t *text_conflicted_p,
                              svn_boolean_t *prop_conflicted_p,
                              svn_boolean_t *tree_conflicted_p,
                              svn_wc__db_t *db,
                              const char *local_abspath,
                              apr_pool_t *scratch_pool);


/* Internal version of svn_wc_transmit_text_deltas3(). */
svn_error_t *
svn_wc__internal_transmit_text_deltas(const char **tempfile,
                                      const svn_checksum_t **new_text_base_md5_checksum,
                                      const svn_checksum_t **new_text_base_sha1_checksum,
                                      svn_wc__db_t *db,
                                      const char *local_abspath,
                                      svn_boolean_t fulltext,
                                      const svn_delta_editor_t *editor,
                                      void *file_baton,
                                      apr_pool_t *result_pool,
                                      apr_pool_t *scratch_pool);

/* Internal version of svn_wc_transmit_prop_deltas2(). */
svn_error_t *
svn_wc__internal_transmit_prop_deltas(svn_wc__db_t *db,
                                     const char *local_abspath,
                                     const svn_delta_editor_t *editor,
                                     void *baton,
                                     apr_pool_t *scratch_pool);

/* Library-internal version of svn_wc_ensure_adm4(). */
svn_error_t *
svn_wc__internal_ensure_adm(svn_wc__db_t *db,
                            const char *local_abspath,
                            const char *url,
                            const char *repos_root_url,
                            const char *repos_uuid,
                            svn_revnum_t revision,
                            svn_depth_t depth,
                            apr_pool_t *scratch_pool);


/* Library-internal version of svn_wc__changelist_match(). */
svn_boolean_t
svn_wc__internal_changelist_match(svn_wc__db_t *db,
                                  const char *local_abspath,
                                  const apr_hash_t *clhash,
                                  apr_pool_t *scratch_pool);

/* Library-internal version of svn_wc_walk_status(), which see. */
svn_error_t *
svn_wc__internal_walk_status(svn_wc__db_t *db,
                             const char *local_abspath,
                             svn_depth_t depth,
                             svn_boolean_t get_all,
                             svn_boolean_t no_ignore,
                             svn_boolean_t ignore_text_mods,
                             const apr_array_header_t *ignore_patterns,
                             svn_wc_status_func4_t status_func,
                             void *status_baton,
                             svn_cancel_func_t cancel_func,
                             void *cancel_baton,
                             apr_pool_t *scratch_pool);

/** A callback invoked by the generic node-walker function.  */
typedef svn_error_t *(*svn_wc__node_found_func_t)(const char *local_abspath,
                                                  svn_node_kind_t kind,
                                                  void *walk_baton,
                                                  apr_pool_t *scratch_pool);

/* Call @a walk_callback with @a walk_baton for @a local_abspath and all
   nodes underneath it, restricted by @a walk_depth, and possibly
   @a changelists.

   If @a show_hidden is true, include hidden nodes, else ignore them.
   If CHANGELISTS is non-NULL and non-empty, filter thereon. */
svn_error_t *
svn_wc__internal_walk_children(svn_wc__db_t *db,
                               const char *local_abspath,
                               svn_boolean_t show_hidden,
                               const apr_array_header_t *changelists,
                               svn_wc__node_found_func_t walk_callback,
                               void *walk_baton,
                               svn_depth_t walk_depth,
                               svn_cancel_func_t cancel_func,
                               void *cancel_baton,
                               apr_pool_t *scratch_pool);

/* Library-internal version of svn_wc_remove_from_revision_control2,
   which see.*/
svn_error_t *
svn_wc__internal_remove_from_revision_control(svn_wc__db_t *db,
                                              const char *local_abspath,
                                              svn_boolean_t destroy_wf,
                                              svn_boolean_t instant_error,
                                              svn_cancel_func_t cancel_func,
                                              void *cancel_baton,
                                              apr_pool_t *scratch_pool);

/* Library-internal version of svn_wc__node_get_schedule(). */
svn_error_t *
svn_wc__internal_node_get_schedule(svn_wc_schedule_t *schedule,
                                   svn_boolean_t *copied,
                                   svn_wc__db_t *db,
                                   const char *local_abspath,
                                   apr_pool_t *scratch_pool);

/**
 * Set @a *copyfrom_url to the corresponding copy-from URL (allocated
 * from @a result_pool), and @a copyfrom_rev to the corresponding
 * copy-from revision, of @a local_abspath, using @a db.  Set @a
 * is_copy_target to TRUE iff @a local_abspath was the target of a
 * copy information (versus being a member of the subtree beneath such
 * a copy target).
 *
 * @a copyfrom_root_url and @a copyfrom_repos_relpath return the exact same
 * information as @a copyfrom_url, just still separated as root and relpath.
 *
 * If @a local_abspath is not copied, set @a *copyfrom_root_url,
 * @a *copyfrom_repos_relpath and @a copyfrom_url to NULL and
 * @a *copyfrom_rev to @c SVN_INVALID_REVNUM.
 *
 * Any out parameters may be NULL if the caller doesn't care about those
 * values.
 */
svn_error_t *
svn_wc__internal_get_copyfrom_info(const char **copyfrom_root_url,
                                   const char **copyfrom_repos_relpath,
                                   const char **copyfrom_url,
                                   svn_revnum_t *copyfrom_rev,
                                   svn_boolean_t *is_copy_target,
                                   svn_wc__db_t *db,
                                   const char *local_abspath,
                                   apr_pool_t *result_pool,
                                   apr_pool_t *scratch_pool);

/* Internal version of svn_wc__node_get_origin() */
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
                            apr_pool_t *scratch_pool);

/* Internal version of svn_wc__node_get_commit_base_rev */
svn_error_t *
svn_wc__internal_get_commit_base_rev(svn_revnum_t *commit_base_revision,
                                     svn_wc__db_t *db,
                                     const char *local_abspath,
                                     apr_pool_t *scratch_pool);


/* Internal version of svn_wc__node_get_repos_info() */
svn_error_t *
svn_wc__internal_get_repos_info(const char **repos_root_url,
                                const char **repos_uuid,
                                svn_wc__db_t *db,
                                const char *local_abspath,
                                apr_pool_t *result_pool,
                                apr_pool_t *scratch_pool);

/* Upgrade the wc sqlite database given in SDB for the wc located at
   WCROOT_ABSPATH. It's current/starting format is given by START_FORMAT.
   After the upgrade is complete (to as far as the automatic upgrade will
   perform), the resulting format is RESULT_FORMAT. All allocations are
   performed in SCRATCH_POOL.  */
svn_error_t *
svn_wc__upgrade_sdb(int *result_format,
                    const char *wcroot_abspath,
                    svn_sqlite__db_t *sdb,
                    int start_format,
                    apr_pool_t *scratch_pool);


svn_error_t *
svn_wc__wipe_postupgrade(const char *dir_abspath,
                         svn_boolean_t whole_admin,
                         svn_cancel_func_t cancel_func,
                         void *cancel_baton,
                         apr_pool_t *scratch_pool);

/* Check whether a node is a working copy root or switched.
 *
 * If LOCAL_ABSPATH is the root of a working copy, set *WC_ROOT to TRUE,
 * otherwise to FALSE.
 *
 * If KIND is not null, set *KIND to the node type of LOCAL_ABSPATH.
 *
 * If LOCAL_ABSPATH is switched against its parent in the same working copy
 * set *SWITCHED to TRUE, otherwise to FALSE.  SWITCHED can be NULL
 * if the result is not important.
 *
 * Use SCRATCH_POOL for temporary allocations.
 */
svn_error_t *
svn_wc__check_wc_root(svn_boolean_t *wc_root,
                      svn_wc__db_kind_t *kind,
                      svn_boolean_t *switched,
                      svn_wc__db_t *db,
                      const char *local_abspath,
                      apr_pool_t *scratch_pool);

/* Ensure LOCAL_ABSPATH is still locked in DB.  Returns the error
 * SVN_ERR_WC_NOT_LOCKED if this is not the case.
 */
svn_error_t *
svn_wc__write_check(svn_wc__db_t *db,
                    const char *local_abspath,
                    apr_pool_t *scratch_pool);

/* Perform the actual merge of file changes between an original file,
   identified by ORIGINAL_CHECKSUM (an empty file if NULL) to a new file
   identified by NEW_CHECKSUM in the working copy identified by WRI_ABSPATH.

   Merge the result into LOCAL_ABSPATH, which is part of the working copy
   identified by WRI_ABSPATH. Use OLD_REVISION and TARGET_REVISION for naming
   the intermediate files.

   The rest of the arguments are passed to svn_wc__internal_merge.
 */
svn_error_t *
svn_wc__perform_file_merge(svn_skel_t **work_items,
                           enum svn_wc_merge_outcome_t *merge_outcome,
                           svn_wc__db_t *db,
                           const char *local_abspath,
                           const char *wri_abspath,
                           const svn_checksum_t *new_checksum,
                           const svn_checksum_t *original_checksum,
                           apr_hash_t *actual_props,
                           const apr_array_header_t *ext_patterns,
                           svn_revnum_t old_revision,
                           svn_revnum_t target_revision,
                           const apr_array_header_t *propchanges,
                           const char *diff3_cmd,
                           svn_wc_conflict_resolver_func2_t conflict_func,
                           void *conflict_baton,
                           svn_cancel_func_t cancel_func,
                           void *cancel_baton,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_LIBSVN_WC_H */
