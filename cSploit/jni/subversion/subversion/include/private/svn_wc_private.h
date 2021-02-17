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
 *
 * @file svn_wc_private.h
 * @brief The Subversion Working Copy Library - Internal routines
 *
 * Requires:
 *            - A working copy
 *
 * Provides:
 *            - Ability to manipulate working copy's versioned data.
 *            - Ability to manipulate working copy's administrative files.
 *
 * Used By:
 *            - Clients.
 */

#ifndef SVN_WC_PRIVATE_H
#define SVN_WC_PRIVATE_H

#include "svn_types.h"
#include "svn_wc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Return TRUE iff CLHASH (a hash whose keys are const char *
   changelist names) is NULL or if LOCAL_ABSPATH is part of a changelist in
   CLHASH. */
svn_boolean_t
svn_wc__changelist_match(svn_wc_context_t *wc_ctx,
                         const char *local_abspath,
                         const apr_hash_t *clhash,
                         apr_pool_t *scratch_pool);

/* Like svn_wc_get_update_editorX and svn_wc_get_status_editorX, but only
   allows updating a file external LOCAL_ABSPATH */
svn_error_t *
svn_wc__get_file_external_editor(const svn_delta_editor_t **editor,
                                 void **edit_baton,
                                 svn_revnum_t *target_revision,
                                 svn_wc_context_t *wc_ctx,
                                 const char *local_abspath,
                                 const char *wri_abspath,
                                 const char *url,
                                 const char *repos_root_url,
                                 const char *repos_uuid,
                                 svn_boolean_t use_commit_times,
                                 const char *diff3_cmd,
                                 const apr_array_header_t *preserved_exts,
                                 const char *record_ancestor_abspath,
                                 const char *recorded_url,
                                 const svn_opt_revision_t *recorded_peg_rev,
                                 const svn_opt_revision_t *recorded_rev,
                                 svn_wc_conflict_resolver_func2_t conflict_func,
                                 void *conflict_baton,
                                 svn_cancel_func_t cancel_func,
                                 void *cancel_baton,
                                 svn_wc_notify_func2_t notify_func,
                                 void *notify_baton,
                                 apr_pool_t *result_pool,
                                 apr_pool_t *scratch_pool);

/* Like svn_wc_crawl_revisionsX, but only supports updating a file external
   LOCAL_ABSPATH which may or may not exist yet. */
svn_error_t *
svn_wc__crawl_file_external(svn_wc_context_t *wc_ctx,
                            const char *local_abspath,
                            const svn_ra_reporter3_t *reporter,
                            void *report_baton,
                            svn_boolean_t restore_files,
                            svn_boolean_t use_commit_times,
                            svn_cancel_func_t cancel_func,
                            void *cancel_baton,
                            svn_wc_notify_func2_t notify_func,
                            void *notify_baton,
                            apr_pool_t *scratch_pool);

/* Check if LOCAL_ABSPATH is an external in the working copy identified
   by WRI_ABSPATH. If not return SVN_ERR_WC_PATH_NOT_FOUND.

   If it is an external return more information on this external.

   If IGNORE_ENOENT, then set *external_kind to svn_node_none, when
   LOCAL_ABSPATH is not an external instead of returning an error.

   ### While we are not at the SVN_WC__HAS_EXTERNALS_STORE format, roots
   ### of working copies will be identified as directory externals. The
   ### recorded information will be NULL for directory externals.
*/
svn_error_t *
svn_wc__read_external_info(svn_node_kind_t *external_kind,
                           const char **defining_abspath,
                           const char **defining_url,
                           svn_revnum_t *defining_operational_revision,
                           svn_revnum_t *defining_revision,
                           svn_wc_context_t *wc_ctx,
                           const char *wri_abspath,
                           const char *local_abspath,
                           svn_boolean_t ignore_enoent,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool);

/* Gets a mapping from const char * local abspaths of externals to the const
   char * local abspath of where they are defined for all externals defined
   at or below LOCAL_ABSPATH.

   ### Returns NULL in *EXTERNALS until we bumped to format 29.

   Allocate the result in RESULT_POOL and perform temporary allocations in
   SCRATCH_POOL. */
svn_error_t *
svn_wc__externals_defined_below(apr_hash_t **externals,
                                svn_wc_context_t *wc_ctx,
                                const char *local_abspath,
                                apr_pool_t *result_pool,
                                apr_pool_t *scratch_pool);


/* Registers a new external at LOCAL_ABSPATH in the working copy containing
   DEFINING_ABSPATH.

   The node is registered as defined on DEFINING_ABSPATH (must be an ancestor
   of LOCAL_ABSPATH) of kind KIND.

   The external is registered as from repository REPOS_ROOT_URL with uuid
   REPOS_UUID and the defining relative path REPOS_RELPATH.

   If the revision of the node is locked OPERATIONAL_REVISION and REVISION
   are the peg and normal revision; otherwise their value is
   SVN_INVALID_REVNUM.

   ### Only KIND svn_node_dir is supported.

   Perform temporary allocations in SCRATCH_POOL.
 */
svn_error_t *
svn_wc__external_register(svn_wc_context_t *wc_ctx,
                          const char *defining_abspath,
                          const char *local_abspath,
                          svn_node_kind_t kind,
                          const char *repos_root_url,
                          const char *repos_uuid,
                          const char *repos_relpath,
                          svn_revnum_t operational_revision,
                          svn_revnum_t revision,
                          apr_pool_t *scratch_pool);

/* Remove the external at LOCAL_ABSPATH from the working copy identified by
   WRI_ABSPATH using WC_CTX.

   If not NULL, call CANCEL_FUNC with CANCEL_BATON to allow canceling while
   removing the working copy files.

   ### This function wraps svn_wc_remove_from_revision_control2().
 */
svn_error_t *
svn_wc__external_remove(svn_wc_context_t *wc_ctx,
                        const char *wri_abspath,
                        const char *local_abspath,
                        svn_cancel_func_t cancel_func,
                        void *cancel_baton,
                        apr_pool_t *scratch_pool);

/* Gather all svn:externals property values from the actual properties on
   directories below LOCAL_ABSPATH as a mapping of const char *local_abspath
   to const char * values.

   Use DEPTH as how it would be used to limit the externals property results
   on update. (So any depth < infinity will only read svn:externals on
   LOCAL_ABSPATH itself)

   If DEPTHS is not NULL, set *depths to an apr_hash_t* mapping the same
   local_abspaths to the const char * ambient depth of the node.

   Allocate the result in RESULT_POOL and perform temporary allocations in
   SCRATCH_POOL. */
svn_error_t *
svn_wc__externals_gather_definitions(apr_hash_t **externals,
                                     apr_hash_t **ambient_depths,
                                     svn_wc_context_t *wc_ctx,
                                     const char *local_abspath,
                                     svn_depth_t depth,
                                     apr_pool_t *result_pool,
                                     apr_pool_t *scratch_pool);

/* Close the DB for LOCAL_ABSPATH.  Perform temporary allocations in
   SCRATCH_POOL.

   Wraps svn_wc__db_drop_root(). */
svn_error_t *
svn_wc__close_db(const char *external_abspath,
                 svn_wc_context_t *wc_ctx,
                 apr_pool_t *scratch_pool);

/** Set @a *tree_conflict to a newly allocated @c
 * svn_wc_conflict_description_t structure describing the tree
 * conflict state of @a victim_abspath, or to @c NULL if @a victim_abspath
 * is not in a state of tree conflict. @a wc_ctx is a working copy context
 * used to access @a victim_path.  Allocate @a *tree_conflict in @a result_pool,
 * use @a scratch_pool for temporary allocations.
 */
svn_error_t *
svn_wc__get_tree_conflict(const svn_wc_conflict_description2_t **tree_conflict,
                          svn_wc_context_t *wc_ctx,
                          const char *victim_abspath,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool);

/** Record the tree conflict described by @a conflict in the WC for
 * @a conflict->local_abspath.  Use @a scratch_pool for all temporary
 * allocations.
 */
svn_error_t *
svn_wc__add_tree_conflict(svn_wc_context_t *wc_ctx,
                          const svn_wc_conflict_description2_t *conflict,
                          apr_pool_t *scratch_pool);

/* Remove any tree conflict on victim @a victim_abspath using @a wc_ctx.
 * (If there is no such conflict recorded, do nothing and return success.)
 *
 * Do all temporary allocations in @a scratch_pool.
 */
svn_error_t *
svn_wc__del_tree_conflict(svn_wc_context_t *wc_ctx,
                          const char *victim_abspath,
                          apr_pool_t *scratch_pool);


/* Return a hash @a *tree_conflicts of all the children of @a
 * local_abspath that are in tree conflicts.  The hash maps local
 * abspaths to pointers to svn_wc_conflict_description2_t, all
 * allocated in result pool.
 */
svn_error_t *
svn_wc__get_all_tree_conflicts(apr_hash_t **tree_conflicts,
                               svn_wc_context_t *wc_ctx,
                               const char *local_abspath,
                               apr_pool_t *result_pool,
                               apr_pool_t *scratch_pool);


/** Like svn_wc_is_wc_root(), but it doesn't consider switched subdirs or
 * deleted entries as working copy roots.
 */
svn_error_t *
svn_wc__strictly_is_wc_root(svn_boolean_t *wc_root,
                            svn_wc_context_t *wc_ctx,
                            const char *local_abspath,
                            apr_pool_t *scratch_pool);


/** Set @a *wcroot_abspath to the local abspath of the root of the
 * working copy in which @a local_abspath resides.
 */
svn_error_t *
svn_wc__get_wc_root(const char **wcroot_abspath,
                    svn_wc_context_t *wc_ctx,
                    const char *local_abspath,
                    apr_pool_t *result_pool,
                    apr_pool_t *scratch_pool);

/**
 * The following are temporary APIs to aid in the transition from wc-1 to
 * wc-ng.  Use them for new development now, but they may be disappearing
 * before the 1.7 release.
 */


/*
 * Convert from svn_wc_conflict_description2_t to
 * svn_wc_conflict_description_t. This is needed by some backwards-compat
 * code in libsvn_client/ctx.c
 *
 * Allocate the result in RESULT_POOL.
 */
svn_wc_conflict_description_t *
svn_wc__cd2_to_cd(const svn_wc_conflict_description2_t *conflict,
                  apr_pool_t *result_pool);


/*
 * Convert from svn_wc_status3_t to svn_wc_status2_t.
 * Allocate the result in RESULT_POOL.
 */
svn_error_t *
svn_wc__status2_from_3(svn_wc_status2_t **status,
                       const svn_wc_status3_t *old_status,
                       svn_wc_context_t *wc_ctx,
                       const char *local_abspath,
                       apr_pool_t *result_pool,
                       apr_pool_t *scratch_pool);


/**
 * Set @a *children to a new array of the immediate children of the working
 * node at @a dir_abspath.  The elements of @a *children are (const char *)
 * absolute paths.
 *
 * Include children that are scheduled for deletion.  Iff @a show_hidden
 * is true, also include children that are 'excluded' or 'server-excluded' or
 * 'not-present'.
 *
 * Return every path that refers to a child of the working node at
 * @a dir_abspath.  Do not include a path just because it was a child of a
 * deleted directory that existed at @a dir_abspath if that directory is now
 * sheduled to be replaced by the working node at @a dir_abspath.
 *
 * Allocate @a *children in @a result_pool.  Use @a wc_ctx to access the
 * working copy, and @a scratch_pool for all temporary allocations.
 */
svn_error_t *
svn_wc__node_get_children_of_working_node(const apr_array_header_t **children,
                                          svn_wc_context_t *wc_ctx,
                                          const char *dir_abspath,
                                          svn_boolean_t show_hidden,
                                          apr_pool_t *result_pool,
                                          apr_pool_t *scratch_pool);

/**
 * Like svn_wc__node_get_children2(), except also include any path that was
 * a child of a deleted directory that existed at @a dir_abspath, even if
 * that directory is now scheduled to be replaced by the working node at
 * @a dir_abspath.
 */
svn_error_t *
svn_wc__node_get_children(const apr_array_header_t **children,
                          svn_wc_context_t *wc_ctx,
                          const char *dir_abspath,
                          svn_boolean_t show_hidden,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool);


/**
 * Fetch the repository root information for a given @a local_abspath into
 * @a *repos_root_url and @a repos_uuid. Use @a wc_ctx to access the working copy
 * for @a local_abspath, @a scratch_pool for all temporary allocations,
 * @a result_pool for result allocations. Note: the result may be NULL if the
 * given node has no repository root associated with it (e.g. locally added).
 *
 * Either input value may be NULL, indicating no interest.
 */
svn_error_t *
svn_wc__node_get_repos_info(const char **repos_root_url,
                            const char **repos_uuid,
                            svn_wc_context_t *wc_ctx,
                            const char *local_abspath,
                            apr_pool_t *result_pool,
                            apr_pool_t *scratch_pool);



/**
 * Get the depth of @a local_abspath using @a wc_ctx.  If @a local_abspath is
 * not in the working copy, return @c SVN_ERR_WC_PATH_NOT_FOUND.
 */
svn_error_t *
svn_wc__node_get_depth(svn_depth_t *depth,
                       svn_wc_context_t *wc_ctx,
                       const char *local_abspath,
                       apr_pool_t *scratch_pool);

/**
 * Get the changed revision, date and author for @a local_abspath using @a
 * wc_ctx.  Allocate the return values in @a result_pool; use @a scratch_pool
 * for temporary allocations.  Any of the return pointers may be @c NULL, in
 * which case they are not set.
 *
 * If @a local_abspath is not in the working copy, return
 * @c SVN_ERR_WC_PATH_NOT_FOUND.
 */
svn_error_t *
svn_wc__node_get_changed_info(svn_revnum_t *changed_rev,
                              apr_time_t *changed_date,
                              const char **changed_author,
                              svn_wc_context_t *wc_ctx,
                              const char *local_abspath,
                              apr_pool_t *result_pool,
                              apr_pool_t *scratch_pool);


/**
 * Set @a *url to the corresponding url for @a local_abspath, using @a wc_ctx.
 * If the node is added, return the url it will have in the repository.
 *
 * If @a local_abspath is not in the working copy, return
 * @c SVN_ERR_WC_PATH_NOT_FOUND.
 */
svn_error_t *
svn_wc__node_get_url(const char **url,
                     svn_wc_context_t *wc_ctx,
                     const char *local_abspath,
                     apr_pool_t *result_pool,
                     apr_pool_t *scratch_pool);

/**
 * Retrieves the origin of the node as it is known in the repository. For
 * added nodes this retrieves where the node is copied from, and the repository
 * location for other nodes.
 *
 * All output arguments may be NULL.
 *
 * If @a is_copy is not NULL, sets @a *is_copy to TRUE if the origin is a copy
 * of the original node.
 *
 * If not NULL, sets @a revision, @a repos_relpath, @a repos_root_url and
 * @a repos_uuid to the original (if a copy) or their current values.
 *
 * If @a copy_root_abspath is not NULL, and @a *is_copy indicates that the
 * node was copied, set @a *copy_root_abspath to the local absolute path of
 * the root of the copied subtree containing the node. If the copied node is
 * a root by itself, @a *copy_root_abspath will match @a local_abspath (but
 * won't necessarily point to the same string in memory).
 *
 * If @a scan_deleted is TRUE, determine the origin of the deleted node. If
 * @a scan_deleted is FALSE, return NULL, SVN_INVALID_REVNUM or FALSE for
 * deleted nodes.
 *
 * Allocate the result in @a result_pool. Perform temporary allocations in
 * @a scratch_pool */
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
                        apr_pool_t *scratch_pool);


/**
 * Set @a *repos_relpath to the corresponding repos_relpath for @a
 * local_abspath, using @a wc_ctx. If the node is added, return the
 * repos_relpath it will have in the repository.
 *
 * If @a local_abspath is not in the working copy, return @c
 * SVN_ERR_WC_PATH_NOT_FOUND.
 * */
svn_error_t *
svn_wc__node_get_repos_relpath(const char **repos_relpath,
                               svn_wc_context_t *wc_ctx,
                               const char *local_abspath,
                               apr_pool_t *result_pool,
                               apr_pool_t *scratch_pool);

/**
 * Set @a *is_deleted to TRUE if @a local_abspath is deleted, using
 * @a wc_ctx.  If @a local_abspath is not in the working copy, return
 * @c SVN_ERR_WC_PATH_NOT_FOUND.  Use @a scratch_pool for all temporary
 * allocations.
 */
svn_error_t *
svn_wc__node_is_status_deleted(svn_boolean_t *is_deleted,
                               svn_wc_context_t *wc_ctx,
                               const char *local_abspath,
                               apr_pool_t *scratch_pool);

/**
 * Set @a *is_server_excluded to whether @a local_abspath has been
 * excluded by the server, using @a wc_ctx.  If @a local_abspath is not
 * in the working copy, return @c SVN_ERR_WC_PATH_NOT_FOUND.
 * Use @a scratch_pool for all temporary allocations.
 */
svn_error_t *
svn_wc__node_is_status_server_excluded(svn_boolean_t *is_server_excluded,
                                       svn_wc_context_t *wc_ctx,
                                       const char *local_abspath,
                                       apr_pool_t *scratch_pool);

/**
 * Set @a *is_not_present to whether the status of @a local_abspath is
 * #svn_wc__db_status_not_present, using @a wc_ctx.
 * If @a local_abspath is not in the working copy, return
 * @c SVN_ERR_WC_PATH_NOT_FOUND.  Use @a scratch_pool for all temporary
 * allocations.
 */
svn_error_t *
svn_wc__node_is_status_not_present(svn_boolean_t *is_not_present,
                                   svn_wc_context_t *wc_ctx,
                                   const char *local_abspath,
                                   apr_pool_t *scratch_pool);

/**
 * Set @a *is_excluded to whether the status of @a local_abspath is
 * #svn_wc__db_status_excluded, using @a wc_ctx.
 * If @a local_abspath is not in the working copy, return
 * @c SVN_ERR_WC_PATH_NOT_FOUND.  Use @a scratch_pool for all temporary
 * allocations.
 */
svn_error_t *
svn_wc__node_is_status_excluded(svn_boolean_t *is_excluded,
                                   svn_wc_context_t *wc_ctx,
                                   const char *local_abspath,
                                   apr_pool_t *scratch_pool);

/**
 * Set @a *is_added to whether @a local_abspath is added, using
 * @a wc_ctx.  If @a local_abspath is not in the working copy, return
 * @c SVN_ERR_WC_PATH_NOT_FOUND.  Use @a scratch_pool for all temporary
 * allocations.
 *
 * NOTE: "added" in this sense, means it was added, copied-here, or
 *   moved-here. This function provides NO information on whether this
 *   addition has replaced another node.
 *
 *   To be clear, this does NOT correspond to svn_wc_schedule_add.
 */
svn_error_t *
svn_wc__node_is_added(svn_boolean_t *is_added,
                      svn_wc_context_t *wc_ctx,
                      const char *local_abspath,
                      apr_pool_t *scratch_pool);

/**
 * Set @a *has_working to whether @a local_abspath has a working node (which
 * might shadow BASE nodes)
 *
 * This is a check similar to status = added or status = deleted.
 */
svn_error_t *
svn_wc__node_has_working(svn_boolean_t *has_working,
                         svn_wc_context_t *wc_ctx,
                         const char *local_abspath,
                         apr_pool_t *scratch_pool);


/**
 * Get the base revision of @a local_abspath using @a wc_ctx.  If
 * @a local_abspath is not in the working copy, return
 * @c SVN_ERR_WC_PATH_NOT_FOUND.
 *
 * In @a *base_revision, return the revision of the revert-base, i.e. the
 * revision that this node was checked out at or last updated/switched to,
 * regardless of any uncommitted changes (delete, replace and/or
 * copy-here/move-here).  For a locally added/copied/moved-here node that is
 * not part of a replace, return @c SVN_INVALID_REVNUM.
 */
svn_error_t *
svn_wc__node_get_base_rev(svn_revnum_t *base_revision,
                          svn_wc_context_t *wc_ctx,
                          const char *local_abspath,
                          apr_pool_t *scratch_pool);


/* Get the working revision of @a local_abspath using @a wc_ctx. If @a
 * local_abspath is not in the working copy, return @c
 * SVN_ERR_WC_PATH_NOT_FOUND.
 *
 * This function is meant as a temporary solution for using the old-style
 * semantics of entries. It will handle any uncommitted changes (delete,
 * replace and/or copy-here/move-here).
 *
 * For a delete the @a revision is the BASE node of the operation root, e.g
 * the path that was deleted. But if the delete is  below an add, the
 * revision is set to SVN_INVALID_REVNUM. For an add, copy or move we return
 * SVN_INVALID_REVNUM. In case of a replacement, we return the BASE
 * revision.
 *
 * The @a changed_rev is set to the latest committed change to @a
 * local_abspath before or equal to @a revision, unless the node is
 * copied-here or moved-here. Then it is the revision of the latest committed
 * change before or equal to the copyfrom_rev.  NOTE, that we use
 * SVN_INVALID_REVNUM for a scheduled copy or move.
 *
 * The @a changed_date and @a changed_author are the ones associated with @a
 * changed_rev.
 */
svn_error_t *
svn_wc__node_get_pre_ng_status_data(svn_revnum_t *revision,
                                    svn_revnum_t *changed_rev,
                                    apr_time_t *changed_date,
                                    const char **changed_author,
                                    svn_wc_context_t *wc_ctx,
                                    const char *local_abspath,
                                    apr_pool_t *result_pool,
                                    apr_pool_t *scratch_pool);


/** This whole function is for legacy, and it sucks. It does not really
 * make sense to get the copy-from revision number without the copy-from
 * URL, but higher level code currently wants that. This should go away.
 * (This function serves to get away from entry_t->revision without having to
 * change the public API.)
 *
 * Get the base revision of @a local_abspath using @a wc_ctx.  If @a
 * local_abspath is not in the working copy, return @c
 * SVN_ERR_WC_PATH_NOT_FOUND.
 *
 * Return the revision number of the base for this node's next commit,
 * reflecting any local tree modifications affecting this node.
 *
 * If this node has no uncommitted changes, return the same as
 * svn_wc__node_get_base_rev().
 *
 * If this node is moved-here or copied-here (possibly as part of a replace),
 * return the revision of the copy/move source. Do the same even when the node
 * has been removed from a recursive copy (subpath excluded from the copy).
 *
 * Else, if this node is locally added, return SVN_INVALID_REVNUM, or if this
 * node is locally deleted or replaced, return the revert-base revision.
 */
svn_error_t *
svn_wc__node_get_commit_base_rev(svn_revnum_t *base_revision,
                                 svn_wc_context_t *wc_ctx,
                                 const char *local_abspath,
                                 apr_pool_t *scratch_pool);

/**
 * Fetch lock information (if any) for @a local_abspath using @a wc_ctx:
 *
 *   Set @a *lock_token to the lock token (or NULL)
 *   Set @a *lock_owner to the owner of the lock (or NULL)
 *   Set @a *lock_comment to the comment associated with the lock (or NULL)
 *   Set @a *lock_date to the timestamp of the lock (or 0)
 *
 * Any of the aforementioned return values may be NULL to indicate
 * that the caller doesn't care about those values.
 *
 * If @a local_abspath is not in the working copy, return @c
 * SVN_ERR_WC_PATH_NOT_FOUND.
 */
svn_error_t *
svn_wc__node_get_lock_info(const char **lock_token,
                           const char **lock_owner,
                           const char **lock_comment,
                           apr_time_t *lock_date,
                           svn_wc_context_t *wc_ctx,
                           const char *local_abspath,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool);

/**
 * A hack to remove the last entry from libsvn_client.  This simply fetches an
 * some values from WC-NG, and puts the needed bits into the output parameters,
 * allocated in @a result_pool.
 *
 * All output arguments can be NULL to indicate that the
 * caller is not interested in the specific result.
 *
 * @a local_abspath and @a wc_ctx are what you think they are.
 */
svn_error_t *
svn_wc__node_get_conflict_info(const char **conflict_old,
                               const char **conflict_new,
                               const char **conflict_wrk,
                               const char **prejfile,
                               svn_wc_context_t *wc_ctx,
                               const char *local_abspath,
                               apr_pool_t *result_pool,
                               apr_pool_t *scratch_pool);


/**
 * Acquire a recursive write lock for @a local_abspath.  If @a lock_anchor
 * is true, determine if @a local_abspath has an anchor that should be locked
 * instead; otherwise, @a local_abspath must be a versioned directory.
 * Store the obtained lock in @a wc_ctx.
 *
 * If @a lock_root_abspath is not NULL, store the root of the lock in
 * @a *lock_root_abspath. If @a lock_root_abspath is NULL, then @a
 * lock_anchor must be FALSE.
 *
 * Returns @c SVN_ERR_WC_LOCKED if an existing lock is encountered, in
 * which case any locks acquired will have been released.
 *
 * If @a lock_anchor is TRUE and @a lock_root_abspath is not NULL, @a
 * lock_root_abspath will be set even when SVN_ERR_WC_LOCKED is returned.
 */
svn_error_t *
svn_wc__acquire_write_lock(const char **lock_root_abspath,
                           svn_wc_context_t *wc_ctx,
                           const char *local_abspath,
                           svn_boolean_t lock_anchor,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool);


/**
 * Recursively release write locks for @a local_abspath, using @a wc_ctx
 * for working copy access.  Only locks held by @a wc_ctx are released.
 * Locks are not removed if work queue items are present.
 *
 * If @a local_abspath is not the root of an owned SVN_ERR_WC_NOT_LOCKED
 * is returned.
 */
svn_error_t *
svn_wc__release_write_lock(svn_wc_context_t *wc_ctx,
                           const char *local_abspath,
                           apr_pool_t *scratch_pool);

/** A callback invoked by the svn_wc__call_with_write_lock() function.  */
typedef svn_error_t *(*svn_wc__with_write_lock_func_t)(void *baton,
                                                       apr_pool_t *result_pool,
                                                       apr_pool_t *scratch_pool);


/** Call function @a func while holding a write lock on
 * @a local_abspath. The @a baton, and @a result_pool and
 * @a scratch_pool, is passed @a func.
 *
 * If @a lock_anchor is TRUE, determine if @a local_abspath has an anchor
 * that should be locked instead.
 *
 * Use @a wc_ctx for working copy access.
 * The lock is guaranteed to be released after @a func returns.
 */
svn_error_t *
svn_wc__call_with_write_lock(svn_wc__with_write_lock_func_t func,
                             void *baton,
                             svn_wc_context_t *wc_ctx,
                             const char *local_abspath,
                             svn_boolean_t lock_anchor,
                             apr_pool_t *result_pool,
                             apr_pool_t *scratch_pool);

/**
 * Calculates the schedule and copied status of a node as that would
 * have been stored in an svn_wc_entry_t instance.
 *
 * If not @c NULL, @a schedule and @a copied are set to their calculated
 * values.
 */
svn_error_t *
svn_wc__node_get_schedule(svn_wc_schedule_t *schedule,
                          svn_boolean_t *copied,
                          svn_wc_context_t *wc_ctx,
                          const char *local_abspath,
                          apr_pool_t *scratch_pool);

/** A callback invoked by svn_wc__prop_list_recursive().
 * It is equivalent to svn_proplist_receiver_t declared in svn_client.h,
 * but kept private within the svn_wc__ namespace because it is used within
 * the bowels of libsvn_wc which don't include svn_client.h.
 *
 * @since New in 1.7. */
typedef svn_error_t *(*svn_wc__proplist_receiver_t)(void *baton,
                                                    const char *local_abspath,
                                                    apr_hash_t *props,
                                                    apr_pool_t *scratch_pool);

/** Call @a receiver_func, passing @a receiver_baton, an absolute path, and
 * a hash table mapping <tt>const char *</tt> names onto <tt>const
 * svn_string_t *</tt> values for all the regular properties of the node
 * at @a local_abspath and any node beneath @a local_abspath within the
 * specified @a depth. @a receiver_fun must not be NULL.
 *
 * If @a propname is not NULL, the passed hash table will only contain
 * the property @a propname.
 *
 * If @a base_props is @c TRUE, get the unmodified BASE properties
 * from the working copy, instead of getting the current (or "WORKING")
 * properties.
 *
 * If @a pristine is not @c TRUE, and @a base_props is FALSE show local
 * modifications to the properties.
 *
 * If a node has no properties, @a receiver_func is not called for the node.
 *
 * If @a changelists are non-NULL and non-empty, filter by them.
 *
 * Use @a wc_ctx to access the working copy, and @a scratch_pool for
 * temporary allocations.
 *
 * If the node at @a local_abspath does not exist,
 * #SVN_ERR_WC_PATH_NOT_FOUND is returned.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_wc__prop_list_recursive(svn_wc_context_t *wc_ctx,
                            const char *local_abspath,
                            const char *propname,
                            svn_depth_t depth,
                            svn_boolean_t base_props,
                            svn_boolean_t pristine,
                            const apr_array_header_t *changelists,
                            svn_wc__proplist_receiver_t receiver_func,
                            void *receiver_baton,
                            svn_cancel_func_t cancel_func,
                            void *cancel_baton,
                            apr_pool_t *scratch_pool);


/**
 * For use by entries.c and entries-dump.c to read old-format working copies.
 */
svn_error_t *
svn_wc__read_entries_old(apr_hash_t **entries,
                         const char *dir_abspath,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool);

/**
 * Recursively clear the dav cache (wcprops) in @a wc_ctx for the tree
 * rooted at @a local_abspath.
 */
svn_error_t *
svn_wc__node_clear_dav_cache_recursive(svn_wc_context_t *wc_ctx,
                                       const char *local_abspath,
                                       apr_pool_t *scratch_pool);

/**
 * Set @a lock_tokens to a hash mapping <tt>const char *</tt> URL
 * to <tt>const char *</tt> lock tokens for every path at or under
 * @a local_abspath in @a wc_ctx which has such a lock token set on it.
 * Allocate the hash and all items therein from @a result_pool.
 */
svn_error_t *
svn_wc__node_get_lock_tokens_recursive(apr_hash_t **lock_tokens,
                                       svn_wc_context_t *wc_ctx,
                                       const char *local_abspath,
                                       apr_pool_t *result_pool,
                                       apr_pool_t *scratch_pool);

/* Set @a *min_revision and @a *max_revision to the lowest and highest revision
 * numbers found within @a local_abspath, using context @a wc_ctx.
 * If @a committed is TRUE, set @a *min_revision and @a *max_revision
 * to the lowest and highest comitted (i.e. "last changed") revision numbers,
 * respectively. Use @a scratch_pool for temporary allocations.
 *
 * Either of MIN_REVISION and MAX_REVISION may be passed as NULL if
 * the caller doesn't care about that return value.
 *
 * This function provides a subset of the functionality of
 * svn_wc_revision_status2() and is more efficient if the caller
 * doesn't need all information returned by svn_wc_revision_status2(). */
svn_error_t *
svn_wc__min_max_revisions(svn_revnum_t *min_revision,
                          svn_revnum_t *max_revision,
                          svn_wc_context_t *wc_ctx,
                          const char *local_abspath,
                          svn_boolean_t committed,
                          apr_pool_t *scratch_pool);

/* Indicate in @a *is_sparse_checkout whether any of the nodes within
 * @a local_abspath is sparse, using context @a wc_ctx.
 * Use @a scratch_pool for temporary allocations.
 *
 * This function provides a subset of the functionality of
 * svn_wc_revision_status2() and is more efficient if the caller
 * doesn't need all information returned by svn_wc_revision_status2(). */
svn_error_t *
svn_wc__is_sparse_checkout(svn_boolean_t *is_sparse_checkout,
                           svn_wc_context_t *wc_ctx,
                           const char *local_abspath,
                           apr_pool_t *scratch_pool);

/* Indicate in @a is_switched whether any node beneath @a local_abspath
 * is switched, using context @a wc_ctx.
 * Use @a scratch_pool for temporary allocations.
 *
 * If @a trail_url is non-NULL, use it to determine if @a local_abspath itself
 * is switched.  It should be any trailing portion of @a local_abspath's
 * expected URL, long enough to include any parts that the caller considers
 * might be changed by a switch.  If it does not match the end of
 * @a local_abspath's actual URL, then report a "switched" status.
 *
 * This function provides a subset of the functionality of
 * svn_wc_revision_status2() and is more efficient if the caller
 * doesn't need all information returned by svn_wc_revision_status2(). */
svn_error_t *
svn_wc__has_switched_subtrees(svn_boolean_t *is_switched,
                              svn_wc_context_t *wc_ctx,
                              const char *local_abspath,
                              const char *trail_url,
                              apr_pool_t *scratch_pool);

/* Set @a *server_excluded_subtrees to a hash mapping <tt>const char *</tt>
 * local * absolute paths to <tt>const char *</tt> local absolute paths for
 * every path at or under @a local_abspath in @a wc_ctx which are excluded
 * by the server (e.g. because of authz).
 * If no server-excluded paths are found then @a *server_excluded_subtrees
 * is set to @c NULL.
 * Allocate the hash and all items therein from @a result_pool.
 */
svn_error_t *
svn_wc__get_server_excluded_subtrees(apr_hash_t **server_excluded_subtrees,
                                     svn_wc_context_t *wc_ctx,
                                     const char *local_abspath,
                                     apr_pool_t *result_pool,
                                     apr_pool_t *scratch_pool);

/* Indicate in @a *is_modified whether the working copy has local
 * modifications, using context @a wc_ctx.
 * Use @a scratch_pool for temporary allocations.
 *
 * This function provides a subset of the functionality of
 * svn_wc_revision_status2() and is more efficient if the caller
 * doesn't need all information returned by svn_wc_revision_status2(). */
svn_error_t *
svn_wc__has_local_mods(svn_boolean_t *is_modified,
                       svn_wc_context_t *wc_ctx,
                       const char *local_abspath,
                       svn_cancel_func_t cancel_func,
                       void *cancel_baton,
                       apr_pool_t *scratch_pool);

/* Renames a working copy from @a from_abspath to @a dst_abspath and makes sure
   open handles are closed to allow this on all platforms.

   Summary: This avoids a file lock problem on wc.db on Windows, that is
            triggered by libsvn_client'ss copy to working copy code. */
svn_error_t *
svn_wc__rename_wc(svn_wc_context_t *wc_ctx,
                  const char *from_abspath,
                  const char *dst_abspath,
                  apr_pool_t *scratch_pool);

/* Gets information needed by the commit harvester.
 *
 * ### Currently this API is work in progress and is designed for just this
 * ### caller. It is certainly possible (and likely) that this function and
 * ### it's caller will eventually move into a wc and maybe wc_db api.
 */
svn_error_t *
svn_wc__node_get_commit_status(svn_node_kind_t *kind,
                               svn_boolean_t *added,
                               svn_boolean_t *deleted,
                               svn_boolean_t *replaced,
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
                               apr_pool_t *scratch_pool);

/* Gets the md5 checksum for the pristine file identified by a sha1_checksum in the
   working copy identified by wri_abspath.

   Wraps svn_wc__db_pristine_get_md5().
 */
svn_error_t *
svn_wc__node_get_md5_from_sha1(const svn_checksum_t **md5_checksum,
                               svn_wc_context_t *wc_ctx,
                               const char *wri_abspath,
                               const svn_checksum_t *sha1_checksum,
                               apr_pool_t *result_pool,
                               apr_pool_t *scratch_pool);


/* Gets an array of const char *repos_relpaths of descendants of LOCAL_ABSPATH,
 * which must be the op root of an addition, copy or move. The descendants
 * returned are at the same op_depth, but are to be deleted by the commit
 * processing because they are not present in the local copy.
 */
svn_error_t *
svn_wc__get_not_present_descendants(const apr_array_header_t **descendants,
                                    svn_wc_context_t *wc_ctx,
                                    const char *local_abspath,
                                    apr_pool_t *result_pool,
                                    apr_pool_t *scratch_pool);


/* Checks a node LOCAL_ABSPATH in WC_CTX for several kinds of obstructions
 * for tasks like merge processing.
 *
 * If a node is not obstructed it sets *OBSTRUCTION_STATE to
 * svn_wc_notify_state_inapplicable. If a node is obstructed or when its
 * direct parent does not exist or is deleted return _state_obstructed. When
 * a node doesn't exist but should exist return svn_wc_notify_state_missing.
 *
 * A node is also obstructed if it is marked excluded or server-excluded or when
 * an unversioned file or directory exists. And if NO_WCROOT_CHECK is FALSE,
 * the root of a working copy is also obstructed; this to allow detecting
 * obstructing working copies.
 *
 * If KIND is not NULL, set *KIND to the kind of node registered in the working
 * copy, or SVN_NODE_NONE if the node doesn't
 *
 * If ADDED is not NULL, set *ADDED to TRUE if the node is added. (Addition,
 * copy or moved).
 *
 * If DELETED is not NULL, set *DELETED to TRUE if the node is marked as
 * deleted in the working copy.
 *
 * If CONFLICTED is not NULL, set *CONFLICTED to TRUE if the node is somehow
 * conflicted.
 *
 * All output arguments except OBSTRUCTION_STATE can be NULL to ommit the
 * result.
 *
 * This function performs temporary allocations in SCRATCH_POOL.
 */
svn_error_t *
svn_wc__check_for_obstructions(svn_wc_notify_state_t *obstruction_state,
                               svn_node_kind_t *kind,
                               svn_boolean_t *added,
                               svn_boolean_t *deleted,
                               svn_boolean_t *conflicted,
                               svn_wc_context_t *wc_ctx,
                               const char *local_abspath,
                               svn_boolean_t no_wcroot_check,
                               apr_pool_t *scratch_pool);


/**
 * A structure which describes various system-generated metadata about
 * a working-copy path or URL.
 *
 * @note Fields may be added to the end of this structure in future
 * versions.  Therefore, users shouldn't allocate structures of this
 * type, to preserve binary compatibility.
 *
 * @since New in 1.7.
 */
typedef struct svn_wc__info2_t
{
  /** Where the item lives in the repository. */
  const char *URL;

  /** The root URL of the repository. */
  const char *repos_root_URL;

  /** The repository's UUID. */
  const char *repos_UUID;

  /** The revision of the object.  If the target is a working-copy
   * path, then this is its current working revision number.  If the target
   * is a URL, then this is the repository revision that it lives in. */
  svn_revnum_t rev;

  /** The node's kind. */
  svn_node_kind_t kind;

  /** The size of the file in the repository (untranslated,
   * e.g. without adjustment of line endings and keyword
   * expansion). Only applicable for file -- not directory -- URLs.
   * For working copy paths, @a size will be #SVN_INVALID_FILESIZE. */
  svn_filesize_t size;

  /** The last revision in which this object changed. */
  svn_revnum_t last_changed_rev;

  /** The date of the last_changed_rev. */
  apr_time_t last_changed_date;

  /** The author of the last_changed_rev. */
  const char *last_changed_author;

  /** An exclusive lock, if present.  Could be either local or remote. */
  svn_lock_t *lock;

  /* Possible information about the working copy, NULL if not valid. */
  struct svn_wc_info_t *wc_info;

} svn_wc__info2_t;

/** The callback invoked by info retrievers.  Each invocation
 * describes @a local_abspath with the information present in @a info.
 * Use @a scratch_pool for all temporary allocation.
 *
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_wc__info_receiver2_t)(void *baton,
                                                 const char *local_abspath,
                                                 const svn_wc__info2_t *info,
                                                 apr_pool_t *scratch_pool);

/* Walk the children of LOCAL_ABSPATH and push svn_wc__info2_t's through
   RECEIVER/RECEIVER_BATON.  Honor DEPTH while crawling children, and
   filter the pushed items against CHANGELISTS.

   If FETCH_EXCLUDED is TRUE, also fetch excluded nodes.
   If FETCH_ACTUAL_ONLY is TRUE, also fetch actual-only nodes. */
svn_error_t *
svn_wc__get_info(svn_wc_context_t *wc_ctx,
                 const char *local_abspath,
                 svn_depth_t depth,
                 svn_boolean_t fetch_excluded,
                 svn_boolean_t fetch_actual_only,
                 const apr_array_header_t *changelists,
                 svn_wc__info_receiver2_t receiver,
                 void *receiver_baton,
                 svn_cancel_func_t cancel_func,
                 void *cancel_baton,
                 apr_pool_t *scratch_pool);

/* During an upgrade to wc-ng, supply known details about an existing
 * external.  The working copy will suck in and store the information supplied
 * about the existing external at @a local_abspath. */
svn_error_t *
svn_wc__upgrade_add_external_info(svn_wc_context_t *wc_ctx,
                                  const char *local_abspath,
                                  svn_node_kind_t kind,
                                  const char *def_local_abspath,
                                  const char *repos_relpath,
                                  const char *repos_root_url,
                                  const char *repos_uuid,
                                  svn_revnum_t def_peg_revision,
                                  svn_revnum_t def_revision,
                                  apr_pool_t *scratch_pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_WC_PRIVATE_H */
