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
 * @file svn_wc_db.h
 * @brief The Subversion Working Copy Library - Metadata/Base-Text Support
 *
 * Requires:
 *            - A working copy
 *
 * Provides:
 *            - Ability to manipulate working copy's administrative files.
 *
 * Used By:
 *            - The main working copy library
 */

#ifndef SVN_WC_DB_H
#define SVN_WC_DB_H

#include "svn_wc.h"

#include "svn_types.h"
#include "svn_error.h"
#include "svn_config.h"
#include "svn_io.h"

#include "private/svn_skel.h"
#include "private/svn_sqlite.h"
#include "private/svn_wc_private.h"

#include "svn_private_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* INTERFACE CONVENTIONS

   "OUT" PARAMETERS

   There are numerous functions within this API which take a (large) number
   of "out" parameters. These are listed individually, rather than combined
   into a struct, so that a caller can be fine-grained about the which
   pieces of information are being requested. In many cases, only a subset
   is required, so the implementation can perform various optimizations
   to fulfill the limited request for information.


   POOLS

   wc_db uses the dual-pool paradigm for all of its functions. Any OUT
   parameter will be allocated within the result pool, and all temporary
   allocations will be performed within the scratch pool.

   The pool that DB is allocated within (the "state" pool) is only used
   for a few, limited allocations to track each of the working copy roots
   that the DB is asked to operate upon. The memory usage on this pool
   os O(# wcroots), which should normally be one or a few. Custom clients
   which hold open structures over a significant period of time should
   pay particular attention to the number of roots touched, and the
   resulting impact on memory consumption (which should still be minimal).


   PARAMETER CONVENTIONS

   * Parameter Order
     - any output arguments
     - DB
     - LOCAL_ABSPATH
     - any other input arguments
     - RESULT_POOL
     - SCRATCH_POOL

   * DB
     This parameter is the primary context for all operations on the
     metadata for working copies. This parameter is passed to almost every
     function, and maintains information and state about every working
     copy "touched" by any of the APIs in this interface.

   * *_ABSPATH
     All *_ABSPATH parameters in this API are absolute paths in the local
     filesystem, represented in Subversion internal canonical form.

   * LOCAL_ABSPATH
     This parameter specifies a particular *versioned* node in the local
     filesystem. From this node, a working copy root is implied, and will
     be used for the given API operation.

   * LOCAL_DIR_ABSPATH
     This parameter is similar to LOCAL_ABSPATH, but the semantics of the
     parameter and operation require the node to be a directory within
     the working copy.

   * WRI_ABSPATH
     This is a "Working copy Root Indicator" path. This refers to a location
     in the local filesystem that is anywhere inside a working copy. The given
     operation will be performed within the context of the root of that
     working copy. This does not necessarily need to refer to a specific
     versioned node or the root of a working copy (although it can) -- any
     location, existing or not, is sufficient, as long as it is inside a
     working copy.
     ### TODO: Define behaviour for switches and externals.
     ### Preference has been stated that WRI_ABSPATH should imply the root
     ### of the parent WC of all switches and externals, but that may
     ### not play out well, especially with multiple repositories involved.
*/

/* Context data structure for interacting with the administrative data. */
typedef struct svn_wc__db_t svn_wc__db_t;


/* Enum indicating what kind of versioned object we're talking about.

   ### KFF: That is, my understanding is that this is *not* an enum
   ### indicating what kind of storage the DB is using, even though
   ### one might think that from its name.  Rather, the "svn_wc__db_"
   ### is a generic prefix, and this "_kind_t" type indicates the kind
   ### of something that's being stored in the DB.

   ### KFF: Does this overlap too much with what svn_node_kind_t does?

   ### gjs: possibly. but that doesn't have a symlink kind. and that
   ###   cannot simply be added. it would surprise too much code.
   ###   (we could probably create svn_node_kind2_t though)
*/
typedef enum svn_wc__db_kind_t {
    /* The node is a directory. */
    svn_wc__db_kind_dir,

    /* The node is a file. */
    svn_wc__db_kind_file,

    /* The node is a symbolic link. */
    svn_wc__db_kind_symlink,

    /* The type of the node is not known, due to its absence, exclusion,
       deletion, or incomplete status. */
    svn_wc__db_kind_unknown

} svn_wc__db_kind_t;


/* Enumerated values describing the state of a node. */
typedef enum svn_wc__db_status_t {
    /* The node is present and has no known modifications applied to it. */
    svn_wc__db_status_normal,

    /* The node has been added (potentially obscuring a delete or move of
       the BASE node; see HAVE_BASE param). The text will be marked as
       modified, and if properties exist, they will be marked as modified.

       In many cases svn_wc__db_status_added means any of added, moved-here
       or copied-here. See individual functions for clarification and
       svn_wc__db_scan_addition() to get more details. */
    svn_wc__db_status_added,

    /* This node has been added with history, based on the move source.
       Text and property modifications are based on whether changes have
       been made against their pristine versions. */
    svn_wc__db_status_moved_here,

    /* This node has been added with history, based on the copy source.
       Text and property modifications are based on whether changes have
       been made against their pristine versions. */
    svn_wc__db_status_copied,

    /* This node has been deleted. No text or property modifications
       will be present. */
    svn_wc__db_status_deleted,

    /* This node was named by the server, but no information was provided. */
    svn_wc__db_status_server_excluded,

    /* This node has been administratively excluded. */
    svn_wc__db_status_excluded,

    /* This node is not present in this revision. This typically happens
       when a node is deleted and committed without updating its parent.
       The parent revision indicates it should be present, but this node's
       revision states otherwise. */
    svn_wc__db_status_not_present,

    /* This node is known, but its information is incomplete. Generally,
       it should be treated similar to the other missing status values
       until some (later) process updates the node with its data.

       When the incomplete status applies to a directory, the list of
       children and the list of its base properties as recorded in the
       working copy do not match their working copy versions.
       The update editor can complete a directory by using a different
       update algorithm. */
    svn_wc__db_status_incomplete,

    /* The BASE node has been marked as deleted. Only used as an internal
       status in wc_db.c and entries.c.  */
    svn_wc__db_status_base_deleted

} svn_wc__db_status_t;

/* Lock information.  We write/read it all as one, so let's use a struct
   for convenience.  */
typedef struct svn_wc__db_lock_t {
  /* The lock token */
  const char *token;

  /* The owner of the lock, possibly NULL */
  const char *owner;

  /* A comment about the lock, possibly NULL */
  const char *comment;

  /* The date the lock was created */
  apr_time_t date;
} svn_wc__db_lock_t;


/* ### NOTE: I have not provided docstrings for most of this file at this
   ### point in time. The shape and extent of this API is still in massive
   ### flux. I'm iterating in public, but do not want to doc until it feels
   ### like it is "Right".
*/

/* ### where/how to handle: text_time, locks, working_size */


/*
  @defgroup svn_wc__db_admin  General administractive functions
  @{
*/

/* Open a working copy administrative database context.

   This context is (initially) not associated with any particular working
   copy directory or working copy root (wcroot). As operations are performed,
   this context will load the appropriate wcroot information.

   The context is returned in DB.

   CONFIG should hold the various configuration options that may apply to
   the administrative operation. It should live at least as long as the
   RESULT_POOL parameter.

   When AUTO_UPGRADE is TRUE, then the working copy databases will be
   upgraded when possible (when an old database is found/detected during
   the operation of a wc_db API). If it is detected that a manual upgrade is
   required, then SVN_ERR_WC_UPGRADE_REQUIRED will be returned from that API.
   Passing FALSE will allow a bare minimum of APIs to function (most notably,
   the temp_get_format() function will always return a value) since most of
   these APIs expect a current-format database to be present.

   If ENFORCE_EMPTY_WQ is TRUE, then any databases with stale work items in
   their work queue will raise an error when they are opened. The operation
   will raise SVN_ERR_WC_CLEANUP_REQUIRED. Passing FALSE for this routine
   means that the work queue is being processed (via 'svn cleanup') and all
   operations should be allowed.

   The DB will be closed when RESULT_POOL is cleared. It may also be closed
   manually using svn_wc__db_close(). In particular, this will close any
   SQLite databases that have been opened and cached.

   The context is allocated in RESULT_POOL. This pool is *retained* and used
   for future allocations within the DB. Be forewarned about unbounded
   memory growth if this DB is used across an unbounded number of wcroots
   and versioned directories.

   Temporary allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_open(svn_wc__db_t **db,
                const svn_config_t *config,
                svn_boolean_t auto_upgrade,
                svn_boolean_t enforce_empty_wq,
                apr_pool_t *result_pool,
                apr_pool_t *scratch_pool);


/* Close DB.  */
svn_error_t *
svn_wc__db_close(svn_wc__db_t *db);


/* Initialize the SDB for LOCAL_ABSPATH, which should be a working copy path.

   A REPOSITORY row will be constructed for the repository identified by
   REPOS_ROOT_URL and REPOS_UUID. Neither of these may be NULL.

   A BASE_NODE row will be created for the directory at REPOS_RELPATH at
   revision INITIAL_REV.
   If INITIAL_REV is greater than zero, then the node will be marked as
   "incomplete" because we don't know its children. Contrary, if the
   INITIAL_REV is zero, then this directory should represent the root and
   we know it has no children, so the node is complete.

   ### Is there any benefit to marking it 'complete' if rev==0?  Seems like
   ### an unnecessary special case.

   DEPTH is the initial depth of the working copy; it must be a definite
   depth, not svn_depth_unknown.

   Use SCRATCH_POOL for temporary allocations.
*/
svn_error_t *
svn_wc__db_init(svn_wc__db_t *db,
                const char *local_abspath,
                const char *repos_relpath,
                const char *repos_root_url,
                const char *repos_uuid,
                svn_revnum_t initial_rev,
                svn_depth_t depth,
                apr_pool_t *scratch_pool);


/* Compute the LOCAL_RELPATH for the given LOCAL_ABSPATH, relative
   from wri_abspath.

   The LOCAL_RELPATH is a relative path to the working copy's root. That
   root will be located by this function, and the path will be relative to
   that location. If LOCAL_ABSPATH is the wcroot directory, then "" will
   be returned.

   The LOCAL_RELPATH should ONLY be used for persisting paths to disk.
   Those patsh should not be an abspath, otherwise the working copy cannot
   be moved. The working copy library should not make these paths visible
   in its API (which should all be abspaths), and it should not be using
   relpaths for other processing.

   LOCAL_RELPATH will be allocated in RESULT_POOL. All other (temporary)
   allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_to_relpath(const char **local_relpath,
                      svn_wc__db_t *db,
                      const char *wri_abspath,
                      const char *local_abspath,
                      apr_pool_t *result_pool,
                      apr_pool_t *scratch_pool);


/* Compute the LOCAL_ABSPATH for a LOCAL_RELPATH located within the working
   copy identified by WRI_ABSPATH.

   This is the reverse of svn_wc__db_to_relpath. It should be used for
   returning a persisted relpath back into an abspath.

   LOCAL_ABSPATH will be allocated in RESULT_POOL. All other (temporary)
   allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_from_relpath(const char **local_abspath,
                        svn_wc__db_t *db,
                        const char *wri_abspath,
                        const char *local_relpath,
                        apr_pool_t *result_pool,
                        apr_pool_t *scratch_pool);

/* Compute the working copy root WCROOT_ABSPATH for WRI_ABSPATH using DB.
 */
svn_error_t *
svn_wc__db_get_wcroot(const char **wcroot_abspath,
                      svn_wc__db_t *db,
                      const char *wri_abspath,
                      apr_pool_t *result_pool,
                      apr_pool_t *scratch_pool);


/* @} */

/* Different kinds of trees

   The design doc mentions three different kinds of trees, BASE, WORKING and
   ACTUAL: http://svn.apache.org/repos/asf/subversion/trunk/notes/wc-ng-design
   We have different APIs to handle each tree, enumerated below, along with
   a blurb to explain what that tree represents.
*/

/* @defgroup svn_wc__db_base  BASE tree management

   BASE is what we get from the server.  It is the *absolute* pristine copy.
   You need to use checkout, update, switch, or commit to alter your view of
   the repository.

   In the BASE tree, each node corresponds to a particular node-rev in the
   repository.  It can be a mixed-revision tree.  Each node holds either a
   copy of the node-rev as it exists in the repository (if presence =
   'normal'), or a place-holder (if presence = 'absent' or 'excluded' or
   'not-present').

   @{
*/

/* Add or replace a directory in the BASE tree.

   The directory is located at LOCAL_ABSPATH on the local filesystem, and
   corresponds to <REPOS_RELPATH, REPOS_ROOT_URL, REPOS_UUID> in the
   repository, at revision REVISION.

   The directory properties are given by the PROPS hash (which is
   const char *name => const svn_string_t *).

   The last-change information is given by <CHANGED_REV, CHANGED_DATE,
   CHANGED_AUTHOR>.

   The directory's children are listed in CHILDREN, as an array of
   const char *. The child nodes do NOT have to exist when this API
   is called. For each child node which does not exists, an "incomplete"
   node will be added. These child nodes will be added regardless of
   the DEPTH value. The caller must sort out which must be recorded,
   and which must be omitted.

   This subsystem does not use DEPTH, but it can be recorded here in
   the BASE tree for higher-level code to use.

   If DAV_CACHE is not NULL, sets LOCAL_ABSPATH's dav cache to the specified
   data.

   If CONFLICT is not NULL, then it describes a conflict for this node. The
   node will be record as conflicted (in ACTUAL).

   If UPDATE_ACTUAL_PROPS is TRUE, set the properties store NEW_ACTUAL_PROPS
   as the new set of properties in ACTUAL. If NEW_ACTUAL_PROPS is NULL or
   when the value of NEW_ACTUAL_PROPS matches NEW_PROPS, store NULL in
   ACTUAL, to mark the properties unmodified.

   Any work items that are necessary as part of this node construction may
   be passed in WORK_ITEMS.

   All temporary allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_base_add_directory(svn_wc__db_t *db,
                              const char *local_abspath,
                              const char *wri_abspath,
                              const char *repos_relpath,
                              const char *repos_root_url,
                              const char *repos_uuid,
                              svn_revnum_t revision,
                              const apr_hash_t *props,
                              svn_revnum_t changed_rev,
                              apr_time_t changed_date,
                              const char *changed_author,
                              const apr_array_header_t *children,
                              svn_depth_t depth,
                              apr_hash_t *dav_cache,
                              const svn_skel_t *conflict,
                              svn_boolean_t update_actual_props,
                              apr_hash_t *new_actual_props,
                              const svn_skel_t *work_items,
                              apr_pool_t *scratch_pool);


/* Add or replace a file in the BASE tree.

   The file is located at LOCAL_ABSPATH on the local filesystem, and
   corresponds to <REPOS_RELPATH, REPOS_ROOT_URL, REPOS_UUID> in the
   repository, at revision REVISION.

   The file properties are given by the PROPS hash (which is
   const char *name => const svn_string_t *).

   The last-change information is given by <CHANGED_REV, CHANGED_DATE,
   CHANGED_AUTHOR>.

   The checksum of the file contents is given in CHECKSUM. An entry in
   the pristine text base is NOT required when this API is called.

   If DAV_CACHE is not NULL, sets LOCAL_ABSPATH's dav cache to the specified
   data.

   If CONFLICT is not NULL, then it describes a conflict for this node. The
   node will be record as conflicted (in ACTUAL).

   If UPDATE_ACTUAL_PROPS is TRUE, set the properties store NEW_ACTUAL_PROPS
   as the new set of properties in ACTUAL. If NEW_ACTUAL_PROPS is NULL or
   when the value of NEW_ACTUAL_PROPS matches NEW_PROPS, store NULL in
   ACTUAL, to mark the properties unmodified.

   Any work items that are necessary as part of this node construction may
   be passed in WORK_ITEMS.

   Unless KEEP_RECORDED_INFO is set to TRUE, recorded size and timestamp values
   will be cleared.

   All temporary allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_base_add_file(svn_wc__db_t *db,
                         const char *local_abspath,
                         const char *wri_abspath,
                         const char *repos_relpath,
                         const char *repos_root_url,
                         const char *repos_uuid,
                         svn_revnum_t revision,
                         const apr_hash_t *props,
                         svn_revnum_t changed_rev,
                         apr_time_t changed_date,
                         const char *changed_author,
                         const svn_checksum_t *checksum,
                         apr_hash_t *dav_cache,
                         const svn_skel_t *conflict,
                         svn_boolean_t update_actual_props,
                         apr_hash_t *new_actual_props,
                         svn_boolean_t keep_recorded_info,
                         svn_boolean_t insert_base_deleted,
                         const svn_skel_t *work_items,
                         apr_pool_t *scratch_pool);


/* Add or replace a symlink in the BASE tree.

   The symlink is located at LOCAL_ABSPATH on the local filesystem, and
   corresponds to <REPOS_RELPATH, REPOS_ROOT_URL, REPOS_UUID> in the
   repository, at revision REVISION.

   The symlink's properties are given by the PROPS hash (which is
   const char *name => const svn_string_t *).

   The last-change information is given by <CHANGED_REV, CHANGED_DATE,
   CHANGED_AUTHOR>.

   The target of the symlink is specified by TARGET.

   If DAV_CACHE is not NULL, sets LOCAL_ABSPATH's dav cache to the specified
   data.

   If CONFLICT is not NULL, then it describes a conflict for this node. The
   node will be record as conflicted (in ACTUAL).

   If UPDATE_ACTUAL_PROPS is TRUE, set the properties store NEW_ACTUAL_PROPS
   as the new set of properties in ACTUAL. If NEW_ACTUAL_PROPS is NULL or
   when the value of NEW_ACTUAL_PROPS matches NEW_PROPS, store NULL in
   ACTUAL, to mark the properties unmodified.

   Any work items that are necessary as part of this node construction may
   be passed in WORK_ITEMS.

   All temporary allocations will be made in SCRATCH_POOL.
*/
/* ### KFF: This is an interesting question, because currently
   ### symlinks are versioned as regular files with the svn:special
   ### property; then the file's text contents indicate that it is a
   ### symlink and where that symlink points.  That's for portability:
   ### you can check 'em out onto a platform that doesn't support
   ### symlinks, and even modify the link and check it back in.  It's
   ### a great solution; but then the question for wc-ng is:
   ###
   ### Suppose you check out a symlink on platform X and platform Y.
   ### X supports symlinks; Y does not.  Should the wc-ng storage for
   ### those two be the same?  I mean, on platform Y, the file is just
   ### going to look and behave like a regular file.  It would be sort
   ### of odd for the wc-ng storage for that file to be of a different
   ### type from all the other files.  (On the other hand, maybe it's
   ### weird today that the wc-1 storage for a working symlink is to
   ### be like a regular file (i.e., regular text-base and whatnot).
   ###
   ### I'm still feeling my way around this problem; just pointing out
   ### the issues.

   ### gjs: symlinks are stored in the database as first-class objects,
   ###   rather than in the filesystem as "special" regular files. thus,
   ###   all portability concerns are moot. higher-levels can figure out
   ###   how to represent the link in ACTUAL. higher-levels can also
   ###   deal with translating to/from the svn:special property and
   ###   the plain-text file contents.
   ### dlr: What about hard links? At minimum, mention in doc string.
*/
svn_error_t *
svn_wc__db_base_add_symlink(svn_wc__db_t *db,
                            const char *local_abspath,
                            const char *wri_abspath,
                            const char *repos_relpath,
                            const char *repos_root_url,
                            const char *repos_uuid,
                            svn_revnum_t revision,
                            const apr_hash_t *props,
                            svn_revnum_t changed_rev,
                            apr_time_t changed_date,
                            const char *changed_author,
                            const char *target,
                            apr_hash_t *dav_cache,
                            const svn_skel_t *conflict,
                            svn_boolean_t update_actual_props,
                            apr_hash_t *new_actual_props,
                            const svn_skel_t *work_items,
                            apr_pool_t *scratch_pool);


/* Create a node in the BASE tree that is present in name only.

   The new node will be located at LOCAL_ABSPATH, and correspond to the
   repository node described by <REPOS_RELPATH, REPOS_ROOT_URL, REPOS_UUID>
   at revision REVISION.

   The node's kind is described by KIND, and the reason for its absence
   is specified by STATUS. Only these values are allowed for STATUS:

     svn_wc__db_status_server_excluded
     svn_wc__db_status_excluded

   If CONFLICT is not NULL, then it describes a conflict for this node. The
   node will be record as conflicted (in ACTUAL).

   Any work items that are necessary as part of this node construction may
   be passed in WORK_ITEMS.

   All temporary allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_base_add_excluded_node(svn_wc__db_t *db,
                                  const char *local_abspath,
                                  const char *repos_relpath,
                                  const char *repos_root_url,
                                  const char *repos_uuid,
                                  svn_revnum_t revision,
                                  svn_wc__db_kind_t kind,
                                  svn_wc__db_status_t status,
                                  const svn_skel_t *conflict,
                                  const svn_skel_t *work_items,
                                  apr_pool_t *scratch_pool);


/* Create a node in the BASE tree that is present in name only.

   The new node will be located at LOCAL_ABSPATH, and correspond to the
   repository node described by <REPOS_RELPATH, REPOS_ROOT_URL, REPOS_UUID>
   at revision REVISION.

   The node's kind is described by KIND, and the reason for its absence
   is 'svn_wc__db_status_not_present'.

   If CONFLICT is not NULL, then it describes a conflict for this node. The
   node will be record as conflicted (in ACTUAL).

   Any work items that are necessary as part of this node construction may
   be passed in WORK_ITEMS.

   All temporary allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_base_add_not_present_node(svn_wc__db_t *db,
                                     const char *local_abspath,
                                     const char *repos_relpath,
                                     const char *repos_root_url,
                                     const char *repos_uuid,
                                     svn_revnum_t revision,
                                     svn_wc__db_kind_t kind,
                                     const svn_skel_t *conflict,
                                     const svn_skel_t *work_items,
                                     apr_pool_t *scratch_pool);


/* Remove a node from the BASE tree.

   The node to remove is indicated by LOCAL_ABSPATH from the local
   filesystem.

   Note that no changes are made to the local filesystem; LOCAL_ABSPATH
   is merely the key to figure out which BASE node to remove.

   To maintain a consistent database this function will also remove
   any working node that marks LOCAL_ABSPATH as base-deleted.  If this
   results in there being no working node for LOCAL_ABSPATH then any
   actual node will be removed if the actual node does not mark a
   conflict.

   Note the caller is responsible for removing base node
   children before calling this function (this may change).
*/
svn_error_t *
svn_wc__db_base_remove(svn_wc__db_t *db,
                       const char *local_abspath,
                       apr_pool_t *scratch_pool);


/* Retrieve information about a node in the BASE tree.

   For the BASE node implied by LOCAL_ABSPATH from the local filesystem,
   return information in the provided OUT parameters. Each OUT parameter
   may be NULL, indicating that specific item is not requested.

   If there is no information about this node, then SVN_ERR_WC_PATH_NOT_FOUND
   will be returned.

   The OUT parameters, and their "not available" values are:
     STATUS             n/a (always available)
     KIND               n/a (always available)
     REVISION           SVN_INVALID_REVNUM
     REPOS_RELPATH      NULL (caller should scan up)
     REPOS_ROOT_URL     NULL (caller should scan up)
     REPOS_UUID         NULL (caller should scan up)
     CHANGED_REV        SVN_INVALID_REVNUM
     CHANGED_DATE       0
     CHANGED_AUTHOR     NULL
     DEPTH              svn_depth_unknown
     CHECKSUM           NULL
     TARGET             NULL
     LOCK               NULL

     HAD_PROPS          FALSE

     UPDATE_ROOT        FALSE

   If the STATUS is normal, and the REPOS_* values are NULL, then the
   caller should use svn_wc__db_scan_base_repos() to scan up the BASE
   tree for the repository information.

   If DEPTH is requested, and the node is NOT a directory, then the
   value will be set to svn_depth_unknown. If LOCAL_ABSPATH is a link,
   it's up to the caller to resolve depth for the link's target.

   If CHECKSUM is requested, and the node is NOT a file, then it will
   be set to NULL.

   If TARGET is requested, and the node is NOT a symlink, then it will
   be set to NULL.

   If UPDATE_ROOT is requested, set it to TRUE if the node should only
   be updated when it is the root of an update (e.g. file externals).

   All returned data will be allocated in RESULT_POOL. All temporary
   allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_base_get_info(svn_wc__db_status_t *status,
                         svn_wc__db_kind_t *kind,
                         svn_revnum_t *revision,
                         const char **repos_relpath,
                         const char **repos_root_url,
                         const char **repos_uuid,
                         svn_revnum_t *changed_rev,
                         apr_time_t *changed_date,
                         const char **changed_author,
                         svn_depth_t *depth,
                         const svn_checksum_t **checksum,
                         const char **target,
                         svn_wc__db_lock_t **lock,
                         svn_boolean_t *had_props,
                         svn_boolean_t *update_root,
                         svn_wc__db_t *db,
                         const char *local_abspath,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool);

/* Structure returned by svn_wc__db_base_get_children_info.  Only has the
   fields needed by the adm crawler. */
struct svn_wc__db_base_info_t {
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;
  svn_revnum_t revnum;
  const char *repos_relpath;
  const char *repos_root_url;
  svn_depth_t depth;
  svn_boolean_t update_root;
  svn_wc__db_lock_t *lock;
};

/* Return in *NODES a hash mapping name->struct svn_wc__db_base_info_t for
   the children of DIR_ABSPATH at op_depth 0.
 */
svn_error_t *
svn_wc__db_base_get_children_info(apr_hash_t **nodes,
                                  svn_wc__db_t *db,
                                  const char *dir_abspath,
                                  apr_pool_t *result_pool,
                                  apr_pool_t *scratch_pool);


/* Set *PROPS to the properties of the node LOCAL_ABSPATH in the BASE tree.

   *PROPS maps "const char *" names to "const svn_string_t *" values.
   If the node has no properties, set *PROPS to an empty hash.
   *PROPS will never be set to NULL.
   If the node is not present in the BASE tree, return an error.
   Allocate *PROPS and its keys and values in RESULT_POOL.
*/
svn_error_t *
svn_wc__db_base_get_props(apr_hash_t **props,
                          svn_wc__db_t *db,
                          const char *local_abspath,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool);


/* Return a list of the BASE tree node's children's names.

   For the node indicated by LOCAL_ABSPATH, this function will return
   the names of all of its children in the array CHILDREN. The array
   elements are const char * values.

   If the node is not a directory, then SVN_ERR_WC_NOT_WORKING_COPY will
   be returned.

   All returned data will be allocated in RESULT_POOL. All temporary
   allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_base_get_children(const apr_array_header_t **children,
                             svn_wc__db_t *db,
                             const char *local_abspath,
                             apr_pool_t *result_pool,
                             apr_pool_t *scratch_pool);


/* Set the dav cache for LOCAL_ABSPATH to PROPS.  Use SCRATCH_POOL for
   temporary allocations. */
svn_error_t *
svn_wc__db_base_set_dav_cache(svn_wc__db_t *db,
                              const char *local_abspath,
                              const apr_hash_t *props,
                              apr_pool_t *scratch_pool);


/* Retrieve the dav cache for LOCAL_ABSPATH into *PROPS, allocated in
   RESULT_POOL.  Use SCRATCH_POOL for temporary allocations.  Return
   SVN_ERR_WC_PATH_NOT_FOUND if no dav cache can be located for
   LOCAL_ABSPATH in DB.  */
svn_error_t *
svn_wc__db_base_get_dav_cache(apr_hash_t **props,
                              svn_wc__db_t *db,
                              const char *local_abspath,
                              apr_pool_t *result_pool,
                              apr_pool_t *scratch_pool);

/* Recursively clear the dav cache for LOCAL_ABSPATH.  Use
   SCRATCH_POOL for temporary allocations. */
svn_error_t *
svn_wc__db_base_clear_dav_cache_recursive(svn_wc__db_t *db,
                                          const char *local_abspath,
                                          apr_pool_t *scratch_pool);

/* Set LOCK_TOKENS to a hash mapping const char * full URLs to const char *
 * lock tokens for every base node at or under LOCAL_ABSPATH in DB which has
 * such a lock token set on it.
 * Allocate the hash and all items therein from RESULT_POOL.  */
svn_error_t *
svn_wc__db_base_get_lock_tokens_recursive(apr_hash_t **lock_tokens,
                                          svn_wc__db_t *db,
                                          const char *local_abspath,
                                          apr_pool_t *result_pool,
                                          apr_pool_t *scratch_pool);

/* ### anything else needed for maintaining the BASE tree? */


/* @} */

/* @defgroup svn_wc__db_pristine  Pristine ("text base") management
   @{
*/

/* Set *PRISTINE_ABSPATH to the path to the pristine text file
   identified by SHA1_CHECKSUM.  Error if it does not exist.

   ### This is temporary - callers should not be looking at the file
   directly.

   Allocate the path in RESULT_POOL. */
svn_error_t *
svn_wc__db_pristine_get_path(const char **pristine_abspath,
                             svn_wc__db_t *db,
                             const char *wri_abspath,
                             const svn_checksum_t *checksum,
                             apr_pool_t *result_pool,
                             apr_pool_t *scratch_pool);

/* Set *PRISTINE_ABSPATH to the path under WCROOT_ABSPATH that will be
   used by the pristine text identified by SHA1_CHECKSUM.  The file
   need not exist.
 */
svn_error_t *
svn_wc__db_pristine_get_future_path(const char **pristine_abspath,
                                    const char *wcroot_abspath,
                                    const svn_checksum_t *sha1_checksum,
                                    apr_pool_t *result_pool,
                                    apr_pool_t *scratch_pool);


/* If requested set *CONTENTS to a readable stream that will yield the pristine
   text identified by SHA1_CHECKSUM (must be a SHA-1 checksum) within the WC
   identified by WRI_ABSPATH in DB.

   If requested set *SIZE to the size of the pristine stream in bytes,

   Even if the pristine text is removed from the store while it is being
   read, the stream will remain valid and readable until it is closed.

   Allocate the stream in RESULT_POOL. */
svn_error_t *
svn_wc__db_pristine_read(svn_stream_t **contents,
                         svn_filesize_t *size,
                         svn_wc__db_t *db,
                         const char *wri_abspath,
                         const svn_checksum_t *sha1_checksum,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool);


/* Set *TEMP_DIR_ABSPATH to a directory in which the caller should create
   a uniquely named file for later installation as a pristine text file.

   The directory is guaranteed to be one that svn_wc__db_pristine_install()
   can use: specifically, one from which it can atomically move the file.

   Allocate *TEMP_DIR_ABSPATH in RESULT_POOL. */
svn_error_t *
svn_wc__db_pristine_get_tempdir(const char **temp_dir_abspath,
                                svn_wc__db_t *db,
                                const char *wri_abspath,
                                apr_pool_t *result_pool,
                                apr_pool_t *scratch_pool);


/* Install the file TEMPFILE_ABSPATH (which is sitting in a directory given by
   svn_wc__db_pristine_get_tempdir()) into the pristine data store, to be
   identified by the SHA-1 checksum of its contents, SHA1_CHECKSUM, and whose
   MD-5 checksum is MD5_CHECKSUM. */
svn_error_t *
svn_wc__db_pristine_install(svn_wc__db_t *db,
                            const char *tempfile_abspath,
                            const svn_checksum_t *sha1_checksum,
                            const svn_checksum_t *md5_checksum,
                            apr_pool_t *scratch_pool);


/* Set *MD5_CHECKSUM to the MD-5 checksum of a pristine text
   identified by its SHA-1 checksum SHA1_CHECKSUM. Return an error
   if the pristine text does not exist or its MD5 checksum is not found.

   Allocate *MD5_CHECKSUM in RESULT_POOL. */
svn_error_t *
svn_wc__db_pristine_get_md5(const svn_checksum_t **md5_checksum,
                            svn_wc__db_t *db,
                            const char *wri_abspath,
                            const svn_checksum_t *sha1_checksum,
                            apr_pool_t *result_pool,
                            apr_pool_t *scratch_pool);


/* Set *SHA1_CHECKSUM to the SHA-1 checksum of a pristine text
   identified by its MD-5 checksum MD5_CHECKSUM. Return an error
   if the pristine text does not exist or its SHA-1 checksum is not found.

   Note: The MD-5 checksum is not strictly guaranteed to be unique in the
   database table, although duplicates are expected to be extremely rare.
   ### TODO: The behaviour is currently unspecified if the MD-5 checksum is
   not unique. Need to see whether this function is going to stay in use,
   and, if so, address this somehow.

   Allocate *SHA1_CHECKSUM in RESULT_POOL. */
svn_error_t *
svn_wc__db_pristine_get_sha1(const svn_checksum_t **sha1_checksum,
                             svn_wc__db_t *db,
                             const char *wri_abspath,
                             const svn_checksum_t *md5_checksum,
                             apr_pool_t *result_pool,
                             apr_pool_t *scratch_pool);


/* Remove the pristine text with SHA-1 checksum SHA1_CHECKSUM from the
 * pristine store, iff it is not referenced by any of the (other) WC DB
 * tables. */
svn_error_t *
svn_wc__db_pristine_remove(svn_wc__db_t *db,
                           const char *wri_abspath,
                           const svn_checksum_t *sha1_checksum,
                           apr_pool_t *scratch_pool);


/* Remove all unreferenced pristines in the WC of WRI_ABSPATH in DB. */
svn_error_t *
svn_wc__db_pristine_cleanup(svn_wc__db_t *db,
                            const char *wri_abspath,
                            apr_pool_t *scratch_pool);


/* Set *PRESENT to true if the pristine store for WRI_ABSPATH in DB contains
   a pristine text with SHA-1 checksum SHA1_CHECKSUM, and to false otherwise.
*/
svn_error_t *
svn_wc__db_pristine_check(svn_boolean_t *present,
                          svn_wc__db_t *db,
                          const char *wri_abspath,
                          const svn_checksum_t *sha1_checksum,
                          apr_pool_t *scratch_pool);

/* @defgroup svn_wc__db_external  External management
   @{ */

/* Adds (or overwrites) a file external LOCAL_ABSPATH to the working copy
   identified by WRI_ABSPATH.

   It updates both EXTERNALS and NODES in one atomic step.
 */
svn_error_t *
svn_wc__db_external_add_file(svn_wc__db_t *db,
                             const char *local_abspath,
                             const char *wri_abspath,

                             const char *repos_relpath,
                             const char *repos_root_url,
                             const char *repos_uuid,
                             svn_revnum_t revision,

                             const apr_hash_t *props,

                             svn_revnum_t changed_rev,
                             apr_time_t changed_date,
                             const char *changed_author,

                             const svn_checksum_t *checksum,

                             const apr_hash_t *dav_cache,

                             const char *record_ancestor_abspath,
                             const char *recorded_repos_relpath,
                             svn_revnum_t recorded_peg_revision,
                             svn_revnum_t recorded_revision,

                             svn_boolean_t update_actual_props,
                             apr_hash_t *new_actual_props,

                             svn_boolean_t keep_recorded_info,
                             const svn_skel_t *work_items,
                             apr_pool_t *scratch_pool);

/* Adds (or overwrites) a symlink external LOCAL_ABSPATH to the working copy
   identified by WRI_ABSPATH.
 */
svn_error_t *
svn_wc__db_external_add_symlink(svn_wc__db_t *db,
                                const char *local_abspath,
                                const char *wri_abspath,

                                const char *repos_relpath,
                                const char *repos_root_url,
                                const char *repos_uuid,
                                svn_revnum_t revision,

                                const apr_hash_t *props,

                                svn_revnum_t changed_rev,
                                apr_time_t changed_date,
                                const char *changed_author,

                                const char *target,

                                const apr_hash_t *dav_cache,

                                const char *record_ancestor_abspath,
                                const char *recorded_repos_relpath,
                                svn_revnum_t recorded_peg_revision,
                                svn_revnum_t recorded_revision,

                                svn_boolean_t update_actual_props,
                                apr_hash_t *new_actual_props,

                                svn_boolean_t keep_recorded_info,
                                const svn_skel_t *work_items,
                                apr_pool_t *scratch_pool);

/* Adds (or overwrites) a directory external LOCAL_ABSPATH to the working copy
   identified by WRI_ABSPATH.

  Directory externals are stored in their own working copy, so one should use
  the normal svn_wc__db functions to access the normal working copy
  information.
 */
svn_error_t *
svn_wc__db_external_add_dir(svn_wc__db_t *db,
                            const char *local_abspath,
                            const char *wri_abspath,

                            const char *repos_root_url,
                            const char *repos_uuid,

                            const char *record_ancestor_abspath,
                            const char *recorded_repos_relpath,
                            svn_revnum_t recorded_peg_revision,
                            svn_revnum_t recorded_revision,

                            const svn_skel_t *work_items,
                            apr_pool_t *scratch_pool);

/* Remove a registered external LOCAL_ABSPATH from the working copy identified
   by WRI_ABSPATH.
 */
svn_error_t *
svn_wc__db_external_remove(svn_wc__db_t *db,
                           const char *local_abspath,
                           const char *wri_abspath,

                           const svn_skel_t *work_items,
                           apr_pool_t *scratch_pool);


/* Reads information on the external LOCAL_ABSPATH as stored in the working
   copy identified with WRI_ABSPATH (If NULL the parent directory of
   LOCAL_ABSPATH is taken as WRI_ABSPATH).

   Return SVN_ERR_WC_PATH_NOT_FOUND if LOCAL_ABSPATH is not an external in
   this working copy.

   When STATUS is requested it has one of these values
      svn_wc__db_status_normal           The external is available
      svn_wc__db_status_excluded         The external is user excluded

   When KIND is requested then the value will be set to the kind of external.

   If DEFININING_ABSPATH is requested, then the value will be set to the
   absolute path of the directory which originally defined the external.
   (The path with the svn:externals property)

   If REPOS_ROOT_URL is requested, then the value will be set to the
   repository root of the external.

   If REPOS_UUID is requested, then the value will be set to the
   repository uuid of the external.

   If RECORDED_REPOS_RELPATH is requested, then the value will be set to the
   original repository relative path inside REPOS_ROOT_URL of the external.

   If RECORDED_PEG_REVISION is requested, then the value will be set to the
   original recorded operational (peg) revision of the external.

   If RECORDED_REVISION is requested, then the value will be set to the
   original recorded revision of the external.

   Allocate the result in RESULT_POOL and perform temporary allocations in
   SCRATCH_POOL.
 */
svn_error_t *
svn_wc__db_external_read(svn_wc__db_status_t *status,
                         svn_wc__db_kind_t *kind,
                         const char **defining_abspath,

                         const char **repos_root_url,
                         const char **repos_uuid,

                         const char **recorded_repos_relpath,
                         svn_revnum_t *recorded_peg_revision,
                         svn_revnum_t *recorded_revision,

                         svn_wc__db_t *db,
                         const char *local_abspath,
                         const char *wri_abspath,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool);

/* Gets a mapping from const char * local abspaths of externals to the const
   char * local abspath of where they are defined for all externals defined
   at or below LOCAL_ABSPATH.

   ### Returns NULL in *EXTERNALS until we bumped to format 29.

   Allocate the result in RESULT_POOL and perform temporary allocations in
   SCRATCH_POOL. */
svn_error_t *
svn_wc__db_externals_defined_below(apr_hash_t **externals,
                                   svn_wc__db_t *db,
                                   const char *local_abspath,
                                   apr_pool_t *result_pool,
                                   apr_pool_t *scratch_pool);

/* Gather all svn:externals property values from the actual properties on
   directories below LOCAL_ABSPATH as a mapping of const char *local_abspath
   to const char * property values.

   If DEPTHS is not NULL, set *depths to an apr_hash_t* mapping the same
   local_abspaths to the const char * ambient depth of the node.

   Allocate the result in RESULT_POOL and perform temporary allocations in
   SCRATCH_POOL. */
svn_error_t *
svn_wc__db_externals_gather_definitions(apr_hash_t **externals,
                                        apr_hash_t **depths,
                                        svn_wc__db_t *db,
                                        const char *local_abspath,
                                        apr_pool_t *result_pool,
                                        apr_pool_t *scratch_pool);

/* @} */

/* @defgroup svn_wc__db_op  Operations on WORKING tree
   @{
*/

/* Copy the node at SRC_ABSPATH (in NODES and ACTUAL_NODE tables) to
 * DST_ABSPATH, both in DB but not necessarily in the same WC.  The parent
 * of DST_ABSPATH must be a versioned directory.
 *
 * This copy is NOT recursive. It simply establishes this one node, plus
 * incomplete nodes for the children.
 *
 * Add WORK_ITEMS to the work queue. */
svn_error_t *
svn_wc__db_op_copy(svn_wc__db_t *db,
                   const char *src_abspath,
                   const char *dst_abspath,
                   const char *dst_op_root_abspath,
                   const svn_skel_t *work_items,
                   apr_pool_t *scratch_pool);

/* Copy the leaves of the op_depth layer directly shadowed by the operation
 * of SRC_ABSPATH (so SRC_ABSPATH must be an op_root) to dst_abspaths
 * parents layer.
 *
 * This operation is recursive. It copies all the descandants at the lower
 * layer and adds base-deleted nodes on dst_abspath layer to mark these nodes
 * properly deleted.
 *
 * Usually this operation is directly followed by a call to svn_wc__db_op_copy
 * which performs the real copy from src_abspath to dst_abspath.
 */
svn_error_t *
svn_wc__db_op_copy_shadowed_layer(svn_wc__db_t *db,
                                  const char *src_abspath,
                                  const char *dst_abspath,
                                  apr_pool_t *scratch_pool);


/* Record a copy at LOCAL_ABSPATH from a repository directory.

   This copy is NOT recursive. It simply establishes this one node.
   CHILDREN must be provided, and incomplete nodes will be constructed
   for them.

   ### arguments docco.  */
svn_error_t *
svn_wc__db_op_copy_dir(svn_wc__db_t *db,
                       const char *local_abspath,
                       const apr_hash_t *props,
                       svn_revnum_t changed_rev,
                       apr_time_t changed_date,
                       const char *changed_author,
                       const char *original_repos_relpath,
                       const char *original_root_url,
                       const char *original_uuid,
                       svn_revnum_t original_revision,
                       const apr_array_header_t *children,
                       svn_depth_t depth,
                       const svn_skel_t *conflict,
                       const svn_skel_t *work_items,
                       apr_pool_t *scratch_pool);


/* Record a copy at LOCAL_ABSPATH from a repository file.

   ### arguments docco.  */
svn_error_t *
svn_wc__db_op_copy_file(svn_wc__db_t *db,
                        const char *local_abspath,
                        const apr_hash_t *props,
                        svn_revnum_t changed_rev,
                        apr_time_t changed_date,
                        const char *changed_author,
                        const char *original_repos_relpath,
                        const char *original_root_url,
                        const char *original_uuid,
                        svn_revnum_t original_revision,
                        const svn_checksum_t *checksum,
                        const svn_skel_t *conflict,
                        const svn_skel_t *work_items,
                        apr_pool_t *scratch_pool);


svn_error_t *
svn_wc__db_op_copy_symlink(svn_wc__db_t *db,
                           const char *local_abspath,
                           const apr_hash_t *props,
                           svn_revnum_t changed_rev,
                           apr_time_t changed_date,
                           const char *changed_author,
                           const char *original_repos_relpath,
                           const char *original_root_url,
                           const char *original_uuid,
                           svn_revnum_t original_revision,
                           const char *target,
                           const svn_skel_t *conflict,
                           const svn_skel_t *work_items,
                           apr_pool_t *scratch_pool);


/* ### do we need svn_wc__db_op_copy_absent() ??  */


/* ### add a new versioned directory. a list of children is NOT passed
   ### since they are added in future, distinct calls to db_op_add_*.
   ### this is freshly added, so it has no properties.  */
/* ### do we need a CONFLICTS param?  */
svn_error_t *
svn_wc__db_op_add_directory(svn_wc__db_t *db,
                            const char *local_abspath,
                            const svn_skel_t *work_items,
                            apr_pool_t *scratch_pool);


/* ### as a new file, there are no properties. this file has no "pristine"
   ### contents, so a checksum [reference] is not required.  */
/* ### do we need a CONFLICTS param?  */
svn_error_t *
svn_wc__db_op_add_file(svn_wc__db_t *db,
                       const char *local_abspath,
                       const svn_skel_t *work_items,
                       apr_pool_t *scratch_pool);


/* ### newly added symlinks have no properties.  */
/* ### do we need a CONFLICTS param?  */
svn_error_t *
svn_wc__db_op_add_symlink(svn_wc__db_t *db,
                          const char *local_abspath,
                          const char *target,
                          const svn_skel_t *work_items,
                          apr_pool_t *scratch_pool);


/* Set the properties of the node LOCAL_ABSPATH in the ACTUAL tree to
   PROPS.

   PROPS maps "const char *" names to "const svn_string_t *" values.
   To specify no properties, PROPS must be an empty hash, not NULL.
   If the node is not present, return an error.

   If PROPS is NULL, set the properties to be the same as the pristine
   properties.

   CONFLICT is used to register a conflict on this node at the same time
   the properties are changed.

   WORK_ITEMS are inserted into the work queue, as additional things that
   need to be completed before the working copy is stable.


   If CLEAR_RECORDED_INFO is true, the recorded information for the node
   is cleared. (commonly used when updating svn:* magic properties).

   NOTE: This will overwrite ALL working properties the node currently
   has. There is no db_op_set_prop() function. Callers must read all the
   properties, change one, and write all the properties.
   ### ugh. this has poor transaction semantics...


   NOTE: This will create an entry in the ACTUAL table for the node if it
   does not yet have one.
*/
svn_error_t *
svn_wc__db_op_set_props(svn_wc__db_t *db,
                        const char *local_abspath,
                        apr_hash_t *props,
                        svn_boolean_t clear_recorded_info,
                        const svn_skel_t *conflict,
                        const svn_skel_t *work_items,
                        apr_pool_t *scratch_pool);

/* See props.h  */
#ifdef SVN__SUPPORT_BASE_MERGE
/* ### Set the properties of the node LOCAL_ABSPATH in the BASE tree to PROPS.
   ###
   ### This function should not exist because properties should be stored
   ### onto the BASE node at construction time, in a single atomic operation.
   ###
   ### PROPS maps "const char *" names to "const svn_string_t *" values.
   ### To specify no properties, PROPS must be an empty hash, not NULL.
   ### If the node is not present, SVN_ERR_WC_PATH_NOT_FOUND is returned.
*/
svn_error_t *
svn_wc__db_temp_base_set_props(svn_wc__db_t *db,
                               const char *local_abspath,
                               const apr_hash_t *props,
                               apr_pool_t *scratch_pool);


/* ### Set the properties of the node LOCAL_ABSPATH in the WORKING tree
   ### to PROPS.
   ###
   ### This function should not exist because properties should be stored
   ### onto the WORKING node at construction time, in a single atomic
   ### operation.
   ###
   ### PROPS maps "const char *" names to "const svn_string_t *" values.
   ### To specify no properties, PROPS must be an empty hash, not NULL.
   ### If the node is not present, SVN_ERR_WC_PATH_NOT_FOUND is returned.
*/
svn_error_t *
svn_wc__db_temp_working_set_props(svn_wc__db_t *db,
                                  const char *local_abspath,
                                  const apr_hash_t *props,
                                  apr_pool_t *scratch_pool);
#endif

/* Mark LOCAL_ABSPATH, and all children, for deletion.
 *
 * If NOTIFY_FUNC is not NULL, then it will be called (with NOTIFY_BATON)
 * for each node deleted. While this processing occurs, if CANCEL_FUNC is
 * not NULL, then it will be called (with CANCEL_BATON) to detect cancellation
 * during the processing.
 *
 * Note: the notification (and cancellation) occur outside of a SQLite
 * transaction.
 */
svn_error_t *
svn_wc__db_op_delete(svn_wc__db_t *db,
                     const char *local_abspath,
                     /* ### flip to CANCEL, then NOTIFY. precedent.  */
                     svn_wc_notify_func2_t notify_func,
                     void *notify_baton,
                     svn_cancel_func_t cancel_func,
                     void *cancel_baton,
                     apr_pool_t *scratch_pool);


/* ### KFF: Would like to know behavior when dst_path already exists
   ### and is a) a dir or b) a non-dir. */
svn_error_t *
svn_wc__db_op_move(svn_wc__db_t *db,
                   const char *src_abspath,
                   const char *dst_abspath,
                   apr_pool_t *scratch_pool);


/* ### mark PATH as (possibly) modified. "svn edit" ... right API here? */
svn_error_t *
svn_wc__db_op_modified(svn_wc__db_t *db,
                       const char *local_abspath,
                       apr_pool_t *scratch_pool);


/* ### use NULL to remove from a changelist.

   ### NOTE: only depth=svn_depth_empty is supported right now.
 */
svn_error_t *
svn_wc__db_op_set_changelist(svn_wc__db_t *db,
                             const char *local_abspath,
                             const char *new_changelist,
                             const apr_array_header_t *changelist_filter,
                             svn_depth_t depth,
                             /* ### flip to CANCEL, then NOTIFY. precedent.  */
                             svn_wc_notify_func2_t notify_func,
                             void *notify_baton,
                             svn_cancel_func_t cancel_func,
                             void *cancel_baton,
                             apr_pool_t *scratch_pool);


/* ### caller maintains ACTUAL. we're just recording state. */
/* ### we probably need to record details of the conflict. how? */
svn_error_t *
svn_wc__db_op_mark_conflict(svn_wc__db_t *db,
                            const char *local_abspath,
                            apr_pool_t *scratch_pool);


/* ### caller maintains ACTUAL, and how the resolution occurred. we're just
   ### recording state.
   ###
   ### I'm not sure that these three values are the best way to do this,
   ### but they're handy for now.  */
svn_error_t *
svn_wc__db_op_mark_resolved(svn_wc__db_t *db,
                            const char *local_abspath,
                            svn_boolean_t resolved_text,
                            svn_boolean_t resolved_props,
                            svn_boolean_t resolved_tree,
                            apr_pool_t *scratch_pool);


/* Revert all local changes which are being maintained in the database,
 * including conflict storage, properties and text modification status.
 *
 * Returns SVN_ERR_WC_INVALID_OPERATION_DEPTH if the revert is not
 * possible, e.g. copy/delete but not a root, or a copy root with
 * children.
 *
 * At present only depth=empty and depth=infinity are supported.
 *
 * This function populates the revert list that can be queried to
 * determine what was reverted.
 */
svn_error_t *
svn_wc__db_op_revert(svn_wc__db_t *db,
                     const char *local_abspath,
                     svn_depth_t depth,
                     apr_pool_t *result_pool,
                     apr_pool_t *scratch_pool);

/* Query the revert list for LOCAL_ABSPATH and set *REVERTED if the
 * path was reverted.  Set *CONFLICT_OLD, *CONFLICT_NEW,
 * *CONFLICT_WORKING and *PROP_REJECT to the names of the conflict
 * files, or NULL if the names are not stored.
 * 
 * Set *COPIED_HERE if the reverted node was copied here and is the
 * operation root of the copy.
 * Set *KIND to the node kind of the reverted node.
 *
 * Removes the row for LOCAL_ABSPATH from the revert list.
 */
svn_error_t *
svn_wc__db_revert_list_read(svn_boolean_t *reverted,
                            const char **conflict_old,
                            const char **conflict_new,
                            const char **conflict_working,
                            const char **prop_reject,
                            svn_boolean_t *copied_here,
                            svn_wc__db_kind_t *kind,
                            svn_wc__db_t *db,
                            const char *local_abspath,
                            apr_pool_t *result_pool,
                            apr_pool_t *scratch_pool);

/* The type of elements in the array returned by
 * svn_wc__db_revert_list_read_copied_children(). */
typedef struct svn_wc__db_revert_list_copied_child_info_t {
  const char *abspath;
  svn_wc__db_kind_t kind;
} svn_wc__db_revert_list_copied_child_info_t ;

/* Return in *CHILDREN a list of reverted copied nodes at or within
 * LOCAL_ABSPATH (which is a reverted file or a reverted directory).
 * Allocate *COPIED_CHILDREN and its elements in RESULT_POOL.
 * The elements are of type svn_wc__db_revert_list_copied_child_info_t. */
svn_error_t *
svn_wc__db_revert_list_read_copied_children(const apr_array_header_t **children,
                                            svn_wc__db_t *db,
                                            const char *local_abspath,
                                            apr_pool_t *result_pool,
                                            apr_pool_t *scratch_pool);


/* Make revert notifications for all paths in the revert list that are
 * equal to LOCAL_ABSPATH or below LOCAL_ABSPATH.
 *
 * Removes all the corresponding rows from the revert list.
 *
 * ### Pass in cancel_func?
 */
svn_error_t *
svn_wc__db_revert_list_notify(svn_wc_notify_func2_t notify_func,
                              void *notify_baton,
                              svn_wc__db_t *db,
                              const char *local_abspath,
                              apr_pool_t *scratch_pool);

/* Clean up after svn_wc__db_op_revert by removing the revert list.
 */
svn_error_t *
svn_wc__db_revert_list_done(svn_wc__db_t *db,
                            const char *local_abspath,
                            apr_pool_t *scratch_pool);


/* Return a hash @a *tree_conflicts of all the children of @a
 * local_abspath that are in tree conflicts.  The hash maps local
 * basenames to pointers to svn_wc_conflict_description2_t, all
 * allocated in result pool.
 */
/* ### this is not an OPERATION. remove the _op_.  */
svn_error_t *
svn_wc__db_op_read_all_tree_conflicts(apr_hash_t **tree_conflicts,
                                      svn_wc__db_t *db,
                                      const char *local_abspath,
                                      apr_pool_t *result_pool,
                                      apr_pool_t *scratch_pool);

/* Get any tree conflict associated with LOCAL_ABSPATH in DB, and put it
   in *TREE_CONFLICT, allocated in RESULT_POOL.

   Use SCRATCH_POOL for any temporary allocations.
*/
/* ### this is not an OPERATION. remove the _op_.  */
svn_error_t *
svn_wc__db_op_read_tree_conflict(
                     const svn_wc_conflict_description2_t **tree_conflict,
                     svn_wc__db_t *db,
                     const char *local_abspath,
                     apr_pool_t *result_pool,
                     apr_pool_t *scratch_pool);


/* Set the tree conflict on LOCAL_ABSPATH in DB to TREE_CONFLICT.  Use
   NULL to remove a tree conflict.

   Use SCRATCH_POOL for any temporary allocations.
*/
/* ### can this also record text/prop conflicts? drop "tree"? */
/* ### dunno if it can, but it definitely should be able to. */
/* ### gjs: also ref: db_op_mark_conflict()  */
svn_error_t *
svn_wc__db_op_set_tree_conflict(svn_wc__db_t *db,
                                const char *local_abspath,
                                const svn_wc_conflict_description2_t *tree_conflict,
                                apr_pool_t *scratch_pool);


/* ### status */


/* @} */

/* @defgroup svn_wc__db_read  Read operations on the BASE/WORKING tree
   @{

   These functions query information about nodes in ACTUAL, and returns
   the requested information from the appropriate ACTUAL, WORKING, or
   BASE tree.

   For example, asking for the checksum of the pristine version will
   return the one recorded in WORKING, or if no WORKING node exists, then
   the checksum comes from BASE.
*/

/* Retrieve information about a node.

   For the node implied by LOCAL_ABSPATH from the local filesystem, return
   information in the provided OUT parameters. Each OUT parameter may be
   NULL, indicating that specific item is not requested.

   The information returned comes from the BASE tree, as possibly modified
   by the WORKING and ACTUAL trees.

   If there is no information about the node, then SVN_ERR_WC_PATH_NOT_FOUND
   will be returned.

   The OUT parameters, and their "not available" values are:
     STATUS                  n/a (always available)
     KIND                    svn_wc__db_kind_unknown   (For ACTUAL only nodes)
     REVISION                SVN_INVALID_REVNUM
     REPOS_RELPATH           NULL
     REPOS_ROOT_URL          NULL
     REPOS_UUID              NULL
     CHANGED_REV             SVN_INVALID_REVNUM
     CHANGED_DATE            0
     CHANGED_AUTHOR          NULL
     DEPTH                   svn_depth_unknown
     CHECKSUM                NULL
     TARGET                  NULL

     ORIGINAL_REPOS_RELPATH  NULL
     ORIGINAL_ROOT_URL       NULL
     ORIGINAL_UUID           NULL
     ORIGINAL_REVISION       SVN_INVALID_REVNUM

     LOCK                    NULL

     RECORDED_SIZE           SVN_INVALID_FILESIZE
     RECORDED_MOD_TIME       0

     CHANGELIST              NULL
     CONFLICTED              FALSE

     OP_ROOT                 FALSE
     HAD_PROPS               FALSE
     PROPS_MOD               FALSE

     HAVE_BASE               FALSE
     HAVE_MORE_WORK          FALSE
     HAVE_WORK               FALSE

   When STATUS is requested, then it will be one of these values:

     svn_wc__db_status_normal
       A plain BASE node, with no local changes.

     svn_wc__db_status_added
       A node has been added/copied/moved to here. See HAVE_BASE to see
       if this change overwrites a BASE node. Use scan_addition() to resolve
       whether this has been added, copied, or moved, and the details of the
       operation (this function only looks at LOCAL_ABSPATH, but resolving
       the details requires scanning one or more ancestor nodes).

     svn_wc__db_status_deleted
       This node has been deleted or moved away. It may be a delete/move of
       a BASE node, or a child node of a subtree that was copied/moved to
       an ancestor location. Call scan_deletion() to determine the full
       details of the operations upon this node.

     svn_wc__db_status_server_excluded
       The node is versioned/known by the server, but the server has
       decided not to provide further information about the node. This
       is a BASE node (since changes are not allowed to this node).

     svn_wc__db_status_excluded
       The node has been excluded from the working copy tree. This may
       be an exclusion from the BASE tree, or an exclusion in the
       WORKING tree for a child node of a copied/moved parent.

     svn_wc__db_status_not_present
       This is a node from the BASE tree, has been marked as "not-present"
       within this mixed-revision working copy. This node is at a revision
       that is not in the tree, contrary to its inclusion in the parent
       node's revision.

     svn_wc__db_status_incomplete
       The BASE is incomplete due to an interrupted operation.  An
       incomplete WORKING node will be svn_wc__db_status_added.

   If REVISION is requested, it will be set to the revision of the
   unmodified (BASE) node, or to SVN_INVALID_REVNUM if any structural
   changes have been made to that node (that is, if the node has a row in
   the WORKING table).

   If DEPTH is requested, and the node is NOT a directory, then
   the value will be set to svn_depth_unknown.

   If CHECKSUM is requested, and the node is NOT a file, then it will
   be set to NULL.

   If TARGET is requested, and the node is NOT a symlink, then it will
   be set to NULL.

   If TRANSLATED_SIZE is requested, and the node is NOT a file, then
   it will be set to SVN_INVALID_FILESIZE.

   If HAVE_WORK is TRUE, the returned information is from the highest WORKING
   layer. In that case HAVE_MORE_WORK and HAVE_BASE provide information about
   what other layers exist for this node.

   If HAVE_WORK is FALSE and HAVE_BASE is TRUE then the information is from
   the BASE tree.

   If HAVE_WORK and HAVE_BASE are both FALSE and when retrieving CONFLICTED,
   then the node doesn't exist at all.

   If OP_ROOT is requested and the node has a WORKING layer, OP_ROOT will be
   set to true if this node is the op_root for this layer.

   If HAD_PROPS is requested and the node has pristine props, the value will
   be set to TRUE.

   If PROP_MODS is requested and the node has property modification the value
   will be set to TRUE.

   ### add information about the need to scan upwards to get a complete
   ### picture of the state of this node.

   ### add some documentation about OUT parameter values based on STATUS ??

   ### the TEXT_MOD may become an enumerated value at some point to
   ### indicate different states of knowledge about text modifications.
   ### for example, an "svn edit" command in the future might set a
   ### flag indicating adminstratively-defined modification. and/or we
   ### might have a status indicating that we saw it was modified while
   ### performing a filesystem traversal.

   All returned data will be allocated in RESULT_POOL. All temporary
   allocations will be made in SCRATCH_POOL.
*/
/* ### old docco. needs to be incorporated as appropriate. there is
   ### some pending, potential changes to the definition of this API,
   ### so not worrying about it just yet.

   ### if the node has not been committed (after adding):
   ###   revision will be SVN_INVALID_REVNUM
   ###   repos_* will be NULL
   ###   changed_rev will be SVN_INVALID_REVNUM
   ###   changed_date will be 0
   ###   changed_author will be NULL
   ###   status will be svn_wc__db_status_added
   ###   text_mod will be TRUE
   ###   prop_mod will be TRUE if any props have been set
   ###   base_shadowed will be FALSE

   ### if the node is not a copy, or a move destination:
   ###   original_repos_path will be NULL
   ###   original_root_url will be NULL
   ###   original_uuid will be NULL
   ###   original_revision will be SVN_INVALID_REVNUM

   ### note that @a base_shadowed can be derived. if the status specifies
   ### an add/copy/move *and* there is a corresponding node in BASE, then
   ### the BASE has been deleted to open the way for this node.
*/
svn_error_t *
svn_wc__db_read_info(svn_wc__db_status_t *status,  /* ### derived */
                     svn_wc__db_kind_t *kind,
                     svn_revnum_t *revision,
                     const char **repos_relpath,
                     const char **repos_root_url,
                     const char **repos_uuid,
                     svn_revnum_t *changed_rev,
                     apr_time_t *changed_date,
                     const char **changed_author,
                     svn_depth_t *depth,  /* dirs only */
                     const svn_checksum_t **checksum, /* files only */
                     const char **target, /* symlinks only */

                     /* ### the following fields if copied/moved (history) */
                     const char **original_repos_relpath,
                     const char **original_root_url,
                     const char **original_uuid,
                     svn_revnum_t *original_revision,

                     /* For BASE nodes */
                     svn_wc__db_lock_t **lock,

                     /* Recorded for files present in the working copy */
                     svn_filesize_t *recorded_size,
                     apr_time_t *recorded_mod_time,

                     /* From ACTUAL */
                     const char **changelist,
                     svn_boolean_t *conflicted,

                     /* ### the followed are derived fields */
                     svn_boolean_t *op_root,

                     svn_boolean_t *had_props,
                     svn_boolean_t *props_mod,

                     svn_boolean_t *have_base,
                     svn_boolean_t *have_more_work,
                     svn_boolean_t *have_work,

                     svn_wc__db_t *db,
                     const char *local_abspath,
                     apr_pool_t *result_pool,
                     apr_pool_t *scratch_pool);

/* Structure returned by svn_wc__db_read_children_info.  Only has the
   fields needed by status. */
struct svn_wc__db_info_t {
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;
  svn_revnum_t revnum;
  const char *repos_relpath;
  const char *repos_root_url;
  const char *repos_uuid;
  svn_revnum_t changed_rev;
  const char *changed_author;
  apr_time_t changed_date;
  svn_depth_t depth;

  svn_filesize_t recorded_size;
  apr_time_t recorded_mod_time;

  const char *changelist;
  svn_boolean_t conflicted;
#ifdef HAVE_SYMLINK
  svn_boolean_t special;
#endif
  svn_boolean_t op_root;

  svn_boolean_t has_checksum;
  svn_boolean_t had_props;
  svn_boolean_t props_mod;

  svn_boolean_t have_base;
  svn_boolean_t have_more_work;

  svn_boolean_t locked;     /* WC directory lock */
  svn_wc__db_lock_t *lock;  /* Repository file lock */
  svn_boolean_t incomplete; /* TRUE if a working node is incomplete */
};

/* Return in *NODES a hash mapping name->struct svn_wc__db_info_t for
   the children of DIR_ABSPATH, and in *CONFLICTS a hash of names in
   conflict.

   The results include any path that was a child of a deleted directory that
   existed at LOCAL_ABSPATH, even if that directory is now scheduled to be
   replaced by the working node at LOCAL_ABSPATH.
 */
svn_error_t *
svn_wc__db_read_children_info(apr_hash_t **nodes,
                              apr_hash_t **conflicts,
                              svn_wc__db_t *db,
                              const char *dir_abspath,
                              apr_pool_t *result_pool,
                              apr_pool_t *scratch_pool);


/* Structure returned by svn_wc__db_read_walker_info.  Only has the
   fields needed by svn_wc__internal_walk_children(). */
struct svn_wc__db_walker_info_t {
  svn_wc__db_status_t status;
  svn_wc__db_kind_t kind;
};

/* When a node is deleted in WORKING, some of its information is no longer
   available. But in some cases it might still be relevant to obtain this
   information even when the information isn't stored in the BASE tree.

   This function allows access to that specific information.

   When a node is not deleted, this node returns the same information
   as svn_wc__db_read_info().

   All output arguments are optional and behave in the same way as when
   calling svn_wc__db_read_info().

   (All other information (like original_*) can be obtained via other apis).
 */
svn_error_t *
svn_wc__db_read_pristine_info(svn_wc__db_status_t *status,
                              svn_wc__db_kind_t *kind,
                              svn_revnum_t *changed_rev,
                              apr_time_t *changed_date,
                              const char **changed_author,
                              svn_depth_t *depth,  /* dirs only */
                              const svn_checksum_t **checksum, /* files only */
                              const char **target, /* symlinks only */
                              svn_boolean_t *had_props,
                              svn_wc__db_t *db,
                              const char *local_abspath,
                              apr_pool_t *result_pool,
                              apr_pool_t *scratch_pool);

/* Gets the information required to install a pristine file to the working copy

   Set WCROOT_ABSPATH to the working copy root, SHA1_CHECKSUM to the
   checksum of the node (a valid reference into the pristine store)
   and PRISTINE_PROPS to the node's pristine properties (to use for
   installing the file).

   If WRI_ABSPATH is not NULL, check for information in the working copy
   identified by WRI_ABSPATH.
   */
svn_error_t *
svn_wc__db_read_node_install_info(const char **wcroot_abspath,
                                  const svn_checksum_t **sha1_checksum,
                                  apr_hash_t **pristine_props,
                                  apr_time_t *changed_date,
                                  svn_wc__db_t *db,
                                  const char *local_abspath,
                                  const char *wri_abspath,
                                  apr_pool_t *result_pool,
                                  apr_pool_t *scratch_pool);

/* Return in *NODES a hash mapping name->struct svn_wc__db_walker_info_t for
   the children of DIR_ABSPATH. "name" is the child's name relatve to
   DIR_ABSPATH, not an absolute path. */
svn_error_t *
svn_wc__db_read_children_walker_info(apr_hash_t **nodes,
                                     svn_wc__db_t *db,
                                     const char *dir_abspath,
                                     apr_pool_t *result_pool,
                                     apr_pool_t *scratch_pool);


/**
 * Set *URL to the corresponding url for LOCAL_ABSPATH.
 * If the node is added, return the url it will have in the repository.
 */
svn_error_t *
svn_wc__db_read_url(const char **url,
                    svn_wc__db_t *db,
                    const char *local_abspath,
                    apr_pool_t *result_pool,
                    apr_pool_t *scratch_pool);


/* Set *PROPS to the properties of the node LOCAL_ABSPATH in the ACTUAL
   tree (looking through to the WORKING or BASE tree as required).

   ### *PROPS will be set to NULL in the following situations:
   ### ... tbd

   PROPS maps "const char *" names to "const svn_string_t *" values.
   If the node has no properties, set *PROPS to an empty hash.
   If the node is not present, return an error.
   Allocate *PROPS and its keys and values in RESULT_POOL.
*/
svn_error_t *
svn_wc__db_read_props(apr_hash_t **props,
                      svn_wc__db_t *db,
                      const char *local_abspath,
                      apr_pool_t *result_pool,
                      apr_pool_t *scratch_pool);

/* Call RECEIVER_FUNC, passing RECEIVER_BATON, an absolute path, and
 * a hash table mapping <tt>char *</tt> names onto svn_string_t *
 * values for any properties of child nodes of LOCAL_ABSPATH (up to DEPTH).
 *
 * If BASE_PROPS is TRUE, read the properties from the BASE layer (op_depth=0),
 * without local modifications.
 *
 * If BASE_PROPS is FALSE, read the properties from the WORKING layer (highest
 * op_depth).
 *
 * If BASE_PROPS is FALSE and, PRISTINE is TRUE, the local modifications will
 * be suppressed. If PRISTINE is FALSE, local modifications will be visible.
 */
svn_error_t *
svn_wc__db_read_props_streamily(svn_wc__db_t *db,
                                const char *local_abspath,
                                svn_depth_t depth,
                                svn_boolean_t base_props,
                                svn_boolean_t pristine,
                                const apr_array_header_t *changelists,
                                svn_wc__proplist_receiver_t receiver_func,
                                void *receiver_baton,
                                svn_cancel_func_t cancel_func,
                                void *cancel_baton,
                                apr_pool_t *scratch_pool);


/* Set *PROPS to the properties of the node LOCAL_ABSPATH in the WORKING
   tree (looking through to the BASE tree as required).

   ### *PROPS will set set to NULL in the following situations:
   ### ... tbd.  see props.c:svn_wc__get_pristine_props()

   *PROPS maps "const char *" names to "const svn_string_t *" values.
   If the node has no properties, set *PROPS to an empty hash.
   If the node is not present, return an error.
   Allocate *PROPS and its keys and values in RESULT_POOL.
*/
svn_error_t *
svn_wc__db_read_pristine_props(apr_hash_t **props,
                               svn_wc__db_t *db,
                               const char *local_abspath,
                               apr_pool_t *result_pool,
                               apr_pool_t *scratch_pool);

/* Set *CHILDREN to a new array of the (const char *) basenames of the
   immediate children of the working node at LOCAL_ABSPATH in DB.

   Return every path that refers to a child of the working node at
   LOCAL_ABSPATH.  Do not include a path just because it was a child of a
   deleted directory that existed at LOCAL_ABSPATH if that directory is now
   sheduled to be replaced by the working node at LOCAL_ABSPATH.

   Allocate *CHILDREN in RESULT_POOL and do temporary allocations in
   SCRATCH_POOL.

   ### return some basic info for each child? e.g. kind.
   ### maybe the data in _read_get_info should be a structure, and this
   ### can return a struct for each one.
   ### however: _read_get_info can say "not interested", which isn't the
   ###   case with a struct. thus, a struct requires fetching and/or
   ###   computing all info.
*/
svn_error_t *
svn_wc__db_read_children_of_working_node(const apr_array_header_t **children,
                                         svn_wc__db_t *db,
                                         const char *local_abspath,
                                         apr_pool_t *result_pool,
                                         apr_pool_t *scratch_pool);

/* Like svn_wc__db_read_children_of_working_node(), except also include any
   path that was a child of a deleted directory that existed at
   LOCAL_ABSPATH, even if that directory is now scheduled to be replaced by
   the working node at LOCAL_ABSPATH.
*/
svn_error_t *
svn_wc__db_read_children(const apr_array_header_t **children,
                         svn_wc__db_t *db,
                         const char *local_abspath,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool);

/* Read into *VICTIMS the basenames of the immediate children of
   LOCAL_ABSPATH in DB that are conflicted.

   In case of tree conflicts a victim doesn't have to be in the
   working copy.

   Allocate *VICTIMS in RESULT_POOL and do temporary allocations in
   SCRATCH_POOL */
/* ### This function will probably be removed. */
svn_error_t *
svn_wc__db_read_conflict_victims(const apr_array_header_t **victims,
                                 svn_wc__db_t *db,
                                 const char *local_abspath,
                                 apr_pool_t *result_pool,
                                 apr_pool_t *scratch_pool);

/* Read into *MARKER_FILES the basenames of the immediate children of
   LOCAL_ABSPATH in DB that are unversioned marker files for text or
   property conflicts.  The files may have been deleted by the user.

   Allocate *MARKER_FILES in RESULT_POOL and do temporary allocations
   in SCRATCH_POOL */
/* ### This function will probably be removed. */
svn_error_t *
svn_wc__db_get_conflict_marker_files(apr_hash_t **markers,
                                     svn_wc__db_t *db,
                                     const char *local_abspath,
                                     apr_pool_t *result_pool,
                                     apr_pool_t *scratch_pool);

/* Read into CONFLICTS svn_wc_conflict_description2_t* structs
   for all conflicts that have LOCAL_ABSPATH as victim.

   Victim must be versioned or be part of a tree conflict.

   Allocate *CONFLICTS in RESULT_POOL and do temporary allocations in
   SCRATCH_POOL */
/* ### Currently there can be just one property conflict recorded
       per victim */
/*  ### This function will probably be removed. */
svn_error_t *
svn_wc__db_read_conflicts(const apr_array_header_t **conflicts,
                          svn_wc__db_t *db,
                          const char *local_abspath,
                          apr_pool_t *result_pool,
                          apr_pool_t *scratch_pool);


/* Return the kind of the node in DB at LOCAL_ABSPATH. The WORKING tree will
   be examined first, then the BASE tree. If the node is not present in either
   tree and ALLOW_MISSING is TRUE, then svn_wc__db_kind_unknown is returned.
   If the node is missing and ALLOW_MISSING is FALSE, then it will return
   SVN_ERR_WC_PATH_NOT_FOUND.

   Uses SCRATCH_POOL for temporary allocations.  */
svn_error_t *
svn_wc__db_read_kind(svn_wc__db_kind_t *kind,
                     svn_wc__db_t *db,
                     const char *local_abspath,
                     svn_boolean_t allow_missing,
                     apr_pool_t *scratch_pool);


/* An analog to svn_wc__entry_is_hidden().  Set *HIDDEN to TRUE if
   LOCAL_ABSPATH in DB "is not present, and I haven't scheduled something
   over the top of it." */
svn_error_t *
svn_wc__db_node_hidden(svn_boolean_t *hidden,
                       svn_wc__db_t *db,
                       const char *local_abspath,
                       apr_pool_t *scratch_pool);

/* Checks if a node replaces a node in a different layer. Also check if it
   replaces a BASE (op_depth 0) node or just a node in a higher layer (a copy).
   Finally check if this is the root of the replacement, or if the replacement
   is initiated by the parent node.

   IS_REPLACE_ROOT (if not NULL) is set to TRUE if the node is the root of a
   replacement; otherwise to FALSE.

   BASE_REPLACE (if not NULL) is set to TRUE if the node directly or indirectly
   replaces a node in the BASE tree; otherwise to FALSE.

   IS_REPLACE (if not NULL) is set to TRUE if the node directly replaces a node
   in a lower layer; otherwise to FALSE.
 */
svn_error_t *
svn_wc__db_node_check_replace(svn_boolean_t *is_replace_root,
                              svn_boolean_t *base_replace,
                              svn_boolean_t *is_replace,
                              svn_wc__db_t *db,
                              const char *local_abspath,
                              apr_pool_t *scratch_pool);

/* ### changelists. return an array, or an iterator interface? how big
   ### are these things? are we okay with an in-memory array? examine other
   ### changelist usage -- we may already assume the list fits in memory.
*/

/* Checks if LOCAL_ABSPATH has a parent directory that knows about its
 * existance. Set *IS_ROOT to FALSE if a parent is found, and to TRUE
 * if there is no such parent.
 */
svn_error_t *
svn_wc__db_is_wcroot(svn_boolean_t *is_root,
                     svn_wc__db_t *db,
                     const char *local_abspath,
                     apr_pool_t *scratch_pool);



/* @} */


/* @defgroup svn_wc__db_global  Operations that alter multiple trees
   @{
*/

/* Associate LOCAL_DIR_ABSPATH, and all its children with the repository at
   at REPOS_ROOT_URL.  The relative path to the repos root will not change,
   just the repository root.  The repos uuid will also remain the same.
   This also updates any locks which may exist for the node, as well as any
   copyfrom repository information.  Finally, the DAV cache (aka
   "wcprops") will be reset for affected entries.

   Use SCRATCH_POOL for any temporary allocations.

   ### local_dir_abspath "should be" the wcroot or a switch root. all URLs
   ### under this directory (depth=infinity) will be rewritten.

   ### This API had a depth parameter, which was removed, should it be
   ### resurrected?  What's the purpose if we claim relocate is infinitely
   ### recursive?

   ### Assuming the future ability to copy across repositories, should we
   ### refrain from resetting the copyfrom information in this operation?
*/
svn_error_t *
svn_wc__db_global_relocate(svn_wc__db_t *db,
                           const char *local_dir_abspath,
                           const char *repos_root_url,
                           apr_pool_t *scratch_pool);


/* ### docco

   ### collapse the WORKING and ACTUAL tree changes down into BASE, called
       for each committed node.

   NEW_REVISION must be the revision number of the revision created by
   the commit. It will become the BASE node's 'revnum' and 'changed_rev'
   values in the BASE_NODE table.

   CHANGED_REVISION is the the new 'last changed' revision. If the node is
   modified its value is equivalent to NEW_REVISION, but in case of a
   decendant of a copy/move it can be an older revision.

   CHANGED_DATE is the (server-side) date of CHANGED_REVISION. It may be 0 if
   the revprop is missing on the revision.

   CHANGED_AUTHOR is the (server-side) author of CHANGED_REVISION. It may be
   NULL if the revprop is missing on the revision.

   One or both of NEW_CHECKSUM and NEW_CHILDREN should be NULL. For new:
     files: NEW_CHILDREN should be NULL
     dirs: NEW_CHECKSUM should be NULL
     symlinks: both should be NULL

   WORK_ITEMS will be place into the work queue.
*/
svn_error_t *
svn_wc__db_global_commit(svn_wc__db_t *db,
                         const char *local_abspath,
                         svn_revnum_t new_revision,
                         svn_revnum_t changed_revision,
                         apr_time_t changed_date,
                         const char *changed_author,
                         const svn_checksum_t *new_checksum,
                         const apr_array_header_t *new_children,
                         apr_hash_t *new_dav_cache,
                         svn_boolean_t keep_changelist,
                         svn_boolean_t no_unlock,
                         const svn_skel_t *work_items,
                         apr_pool_t *scratch_pool);


/* ### docco

   Perform an "update" operation at this node. It will create/modify a BASE
   node, and possibly update the ACTUAL tree's node (e.g put the node into
   a conflicted state).

   ### there may be cases where we need to tweak an existing WORKING node

   ### this operations on a single node, but may affect children

   ### the repository cannot be changed with this function, but a "switch"
   ### (aka changing repos_relpath) is possible

   ### one of NEW_CHILDREN, NEW_CHECKSUM, or NEW_TARGET must be provided.
   ### the other two values must be NULL.
   ### should this be broken out into an update_(directory|file|symlink) ?

   ### how does this differ from base_add_*? just the CONFLICT param.
   ### the WORK_ITEMS param is new here, but the base_add_* functions
   ### should probably grow that. should we instead just (re)use base_add
   ### rather than grow a new function?

   ### this does not allow a change of depth

   ### we do not update a file's TRANSLATED_SIZE here. at some future point,
   ### when the file is installed, then a TRANSLATED_SIZE will be set.
*/
svn_error_t *
svn_wc__db_global_update(svn_wc__db_t *db,
                         const char *local_abspath,
                         svn_wc__db_kind_t new_kind,
                         const char *new_repos_relpath,
                         svn_revnum_t new_revision,
                         const apr_hash_t *new_props,
                         svn_revnum_t new_changed_rev,
                         apr_time_t new_changed_date,
                         const char *new_changed_author,
                         const apr_array_header_t *new_children,
                         const svn_checksum_t *new_checksum,
                         const char *new_target,
                         const apr_hash_t *new_dav_cache,
                         const svn_skel_t *conflict,
                         const svn_skel_t *work_items,
                         apr_pool_t *scratch_pool);


/* Modify the entry of working copy LOCAL_ABSPATH, presumably after an update
   of depth DEPTH completes.  If LOCAL_ABSPATH doesn't exist, this routine
   does nothing.

   Set the node's repository relpath, repository root, repository uuid and
   revision to NEW_REPOS_RELPATH, NEW_REPOS_ROOT and NEW_REPOS_UUID.  If
   NEW_REPOS_RELPATH is null, the repository location is untouched; if
   NEW_REVISION in invalid, the working revision field is untouched.
   The modifications are mutually exclusive.  If NEW_REPOS_ROOT is non-NULL,
   set the repository root of the entry to NEW_REPOS_ROOT.

   If LOCAL_ABSPATH is a directory, then, walk entries below LOCAL_ABSPATH
   according to DEPTH thusly:

   If DEPTH is svn_depth_infinity, perform the following actions on
   every entry below PATH; if svn_depth_immediates, svn_depth_files,
   or svn_depth_empty, perform them only on LOCAL_ABSPATH.

   If NEW_REVISION is valid, then tweak every entry to have this new
   working revision (excluding files that are scheduled for addition
   or replacement).  Likewise, if BASE_URL is non-null, then rewrite
   all urls to be "telescoping" children of the base_url.

   EXCLUDE_RELPATHS is a hash containing const char *local_relpath.  Nodes
   for pathnames contained in EXCLUDE_RELPATHS are not touched by this
   function.  These pathnames should be paths relative to the wcroot.
*/
svn_error_t *
svn_wc__db_op_bump_revisions_post_update(svn_wc__db_t *db,
                                         const char *local_abspath,
                                         svn_depth_t depth,
                                         const char *new_repos_relpath,
                                         const char *new_repos_root_url,
                                         const char *new_repos_uuid,
                                         svn_revnum_t new_revision,
                                         apr_hash_t *exclude_relpaths,
                                         apr_pool_t *scratch_pool);


/* Record the TRANSLATED_SIZE and LAST_MOD_TIME for a versioned node.

   This function will record the information within the WORKING node,
   if present, or within the BASE tree. If neither node is present, then
   SVN_ERR_WC_PATH_NOT_FOUND will be returned.

   TRANSLATED_SIZE may be SVN_INVALID_FILESIZE, which will be recorded
   as such, implying "unknown size".

   LAST_MOD_TIME may be 0, which will be recorded as such, implying
   "unknown last mod time".
*/
svn_error_t *
svn_wc__db_global_record_fileinfo(svn_wc__db_t *db,
                                  const char *local_abspath,
                                  svn_filesize_t translated_size,
                                  apr_time_t last_mod_time,
                                  apr_pool_t *scratch_pool);


/* ### post-commit handling.
   ### maybe multiple phases?
   ### 1) mark a changelist as being-committed
   ### 2) collect ACTUAL content, store for future use as TEXTBASE
   ### 3) caller performs commit
   ### 4) post-commit, integrate changelist into BASE
*/


/* @} */


/* @defgroup svn_wc__db_lock  Function to manage the LOCKS table.
   @{
*/

/* Add or replace LOCK for LOCAL_ABSPATH to DB.  */
svn_error_t *
svn_wc__db_lock_add(svn_wc__db_t *db,
                    const char *local_abspath,
                    const svn_wc__db_lock_t *lock,
                    apr_pool_t *scratch_pool);


/* Remove any lock for LOCAL_ABSPATH in DB.  */
svn_error_t *
svn_wc__db_lock_remove(svn_wc__db_t *db,
                       const char *local_abspath,
                       apr_pool_t *scratch_pool);


/* @} */


/* @defgroup svn_wc__db_scan  Functions to scan up a tree for further data.
   @{
*/

/* Read a BASE node's repository information.

   For the BASE node implied by LOCAL_ABSPATH, its location in the repository
   returned in *REPOS_ROOT_URL and *REPOS_UUID will be returned in
   *REPOS_RELPATH. Any of the OUT parameters may be NULL, indicating no
   interest in that piece of information.

   All returned data will be allocated in RESULT_POOL. All temporary
   allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_scan_base_repos(const char **repos_relpath,
                           const char **repos_root_url,
                           const char **repos_uuid,
                           svn_wc__db_t *db,
                           const char *local_abspath,
                           apr_pool_t *result_pool,
                           apr_pool_t *scratch_pool);


/* Scan upwards for information about a known addition to the WORKING tree.

   IFF a node's status as returned by svn_wc__db_read_info() is
   svn_wc__db_status_added (NOT obstructed_add!), then this function
   returns a refined status in *STATUS, which is one of:

     svn_wc__db_status_added -- this NODE is a simple add without history.
       OP_ROOT_ABSPATH will be set to the topmost node in the added subtree
       (implying its parent will be an unshadowed BASE node). The REPOS_*
       values will be implied by that ancestor BASE node and this node's
       position in the added subtree. ORIGINAL_* will be set to their
       NULL values (and SVN_INVALID_REVNUM for ORIGINAL_REVISION).

     svn_wc__db_status_copied -- this NODE is the root or child of a copy.
       The root of the copy will be stored in OP_ROOT_ABSPATH. Note that
       the parent of the operation root could be another WORKING node (from
       an add, copy, or move). The REPOS_* values will be implied by the
       ancestor unshadowed BASE node. ORIGINAL_* will indicate the source
       of the copy.

     svn_wc__db_status_incomplete -- this NODE is copied but incomplete.

     svn_wc__db_status_moved_here -- this NODE arrived as a result of a move.
       The root of the moved nodes will be stored in OP_ROOT_ABSPATH.
       Similar to the copied state, its parent may be a WORKING node or a
       BASE node. And again, the REPOS_* values are implied by this node's
       position in the subtree under the ancestor unshadowed BASE node.
       ORIGINAL_* will indicate the source of the move.

   All OUT parameters may be NULL to indicate a lack of interest in
   that piece of information.

   STATUS, OP_ROOT_ABSPATH, and REPOS_* will always be assigned a value
   if that information is requested (and assuming a successful return).

   ORIGINAL_REPOS_RELPATH will refer to the *root* of the operation. It
   does *not* correspond to the node given by LOCAL_ABSPATH. The caller
   can use the suffix on LOCAL_ABSPATH (relative to OP_ROOT_ABSPATH) in
   order to compute the source node which corresponds to LOCAL_ABSPATH.

   If the node given by LOCAL_ABSPATH does not have changes recorded in
   the WORKING tree, then SVN_ERR_WC_PATH_NOT_FOUND is returned. If it
   doesn't have an "added" status, then SVN_ERR_WC_PATH_UNEXPECTED_STATUS
   will be returned.

   All returned data will be allocated in RESULT_POOL. All temporary
   allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_scan_addition(svn_wc__db_status_t *status,
                         const char **op_root_abspath,
                         const char **repos_relpath,
                         const char **repos_root_url,
                         const char **repos_uuid,
                         const char **original_repos_relpath,
                         const char **original_root_url,
                         const char **original_uuid,
                         svn_revnum_t *original_revision,
                         svn_wc__db_t *db,
                         const char *local_abspath,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool);


/* Scan upwards for additional information about a deleted node.

   When a deleted node is discovered in the WORKING tree, the situation
   may be quite complex. This function will provide the information to
   resolve the circumstances of the deletion.

   For discussion purposes, we will start with the most complex example
   and then demonstrate simplified examples. Consider node B/W/D/N has been
   found as deleted. B is an unmodified directory (thus, only in BASE). W is
   "replacement" content that exists in WORKING, shadowing a similar B/W
   directory in BASE. D is a deleted subtree in the WORKING tree, and N is
   the deleted node.

   In this example, BASE_DEL_ABSPATH will bet set to B/W. That is the root of
   the BASE tree (implicitly) deleted by the replacement. WORK_DEL_ABSPATH
   will be set to the subtree deleted within the replacement; in this case,
   B/W/D. No move-away took place, so MOVED_TO_ABSPATH is set to NULL.

   In another scenario, B/W was moved-away before W was put into the WORKING
   tree through an add/copy/move-here. MOVED_TO_ABSPATH will indicate where
   B/W was moved to. Note that further operations may have been performed
   post-move, but that is not known or reported by this function.

   If BASE does not have a B/W, then the WORKING B/W is not a replacement,
   but a simple add/copy/move-here. BASE_DEL_ABSPATH will be set to NULL.

   If B/W/D does not exist in the WORKING tree (we're only talking about a
   deletion of nodes of the BASE tree), then deleting B/W/D would have marked
   the subtree for deletion. BASE_DEL_ABSPATH will refer to B/W/D,
   MOVED_TO_ABSPATH will be NULL, and WORK_DEL_ABSPATH will be NULL.

   If the BASE node B/W/D was moved instead of deleted, then MOVED_TO_ABSPATH
   would indicate the target location (and other OUT values as above).

   When the user deletes B/W/D from the WORKING tree, there are a few
   additional considerations. If B/W is a simple addition (not a copy or
   a move-here), then the deletion will simply remove the nodes from WORKING
   and possibly leave behind "base-delete" markers in the WORKING tree.
   If the source is a copy/moved-here, then the nodes are replaced with
   deletion markers.

   If the user moves-away B/W/D from the WORKING tree, then behavior is
   again dependent upon the origination of B/W. For a plain add, the nodes
   simply move to the destination. For a copy, a deletion is made at B/W/D,
   and a new copy (of a subtree of the original source) is made at the
   destination. For a move-here, a deletion is made, and a copy is made at
   the destination (we do not track multiple moves; the source is moved to
   B/W, then B/W/D is deleted; then a copy is made at the destination;
   however, note the double-move could have been performed by moving the
   subtree first, then moving the source to B/W).

   There are three further considerations when resolving a deleted node:

     If the BASE B/W/D was moved-away, then BASE_DEL_ABSPATH will specify
     B/W/D as the root of the BASE deletion (not necessarily B/W as an
     implicit delete caused by a replacement; only the closest ancestor is
     reported). The other parameters will operate as normal, based on what
     is happening in the WORKING tree. Also note that ancestors of B/W/D
     may report additional, explicit moved-away status.

     If the BASE B/W/D was deleted explicitly *and* B/W is a replacement,
     then the explicit deletion is subsumed by the implicit deletion that
     occurred with the B/W replacement. Thus, BASE_DEL_ABSPATH will point
     to B/W as the root of the BASE deletion. IOW, we can detect the
     explicit move-away, but not an explicit deletion.

     If B/W/D/N refers to a node present in the BASE tree, and B/W was
     replaced by a shallow subtree, then it is possible for N to be
     reported as deleted (from BASE) yet no deletions occurred in the
     WORKING tree above N. Thus, WORK_DEL_ABSPATH will be set to NULL.


   Summary of OUT parameters:

   BASE_DEL_ABSPATH will specify the nearest ancestor of the explicit or
   implicit deletion (if any) that applies to the BASE tree.

   MOVED_TO_ABSPATH will specify the nearest ancestor that has moved-away,
   if any. If no ancestors have been moved-away, then this is set to NULL.

   WORK_DEL_ABSPATH will specify the root of a deleted subtree within
   the WORKING tree (note there is no concept of layered delete operations
   in WORKING, so there is only one deletion root in the ancestry).

   All OUT parameters may be set to NULL to indicate a lack of interest in
   that piece of information.

   If the node given by LOCAL_ABSPATH does not exist, then
   SVN_ERR_WC_PATH_NOT_FOUND is returned. If it doesn't have a "deleted"
   status, then SVN_ERR_WC_PATH_UNEXPECTED_STATUS will be returned.

   All returned data will be allocated in RESULT_POOL. All temporary
   allocations will be made in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__db_scan_deletion(const char **base_del_abspath,
                         const char **moved_to_abspath,
                         const char **work_del_abspath,
                         svn_wc__db_t *db,
                         const char *local_abspath,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool);


/* @} */


/* @defgroup svn_wc__db_upgrade  Functions for upgrading a working copy.
   @{
*/

/* Create a new wc.db file for LOCAL_DIR_ABSPATH, which is going to be a
   working copy for the repository REPOS_ROOT_URL with uuid REPOS_UUID.
   Return the raw sqlite handle, repository id and working copy id
   and store the database in WC_DB.

   Perform temporary allocations in SCRATCH_POOL. */
svn_error_t *
svn_wc__db_upgrade_begin(svn_sqlite__db_t **sdb,
                         apr_int64_t *repos_id,
                         apr_int64_t *wc_id,
                         svn_wc__db_t *wc_db,
                         const char *local_dir_abspath,
                         const char *repos_root_url,
                         const char *repos_uuid,
                         apr_pool_t *scratch_pool);


svn_error_t *
svn_wc__db_upgrade_apply_dav_cache(svn_sqlite__db_t *sdb,
                                   const char *dir_relpath,
                                   apr_hash_t *cache_values,
                                   apr_pool_t *scratch_pool);


/* ### need much more docco

   ### this function should be called within a sqlite transaction. it makes
   ### assumptions around this fact.

   Apply the various sets of properties to the database nodes based on
   their existence/presence, the current state of the node, and the original
   format of the working copy which provided these property sets.
*/
svn_error_t *
svn_wc__db_upgrade_apply_props(svn_sqlite__db_t *sdb,
                               const char *dir_abspath,
                               const char *local_relpath,
                               apr_hash_t *base_props,
                               apr_hash_t *revert_props,
                               apr_hash_t *working_props,
                               int original_format,
                               apr_int64_t wc_id,
                               apr_pool_t *scratch_pool);

/* Simply insert (or replace) one row in the EXTERNALS table. */
svn_error_t *
svn_wc__db_upgrade_insert_external(svn_wc__db_t *db,
                                   const char *local_abspath,
                                   svn_wc__db_kind_t kind,
                                   const char *parent_abspath,
                                   const char *def_local_abspath,
                                   const char *repos_relpath,
                                   const char *repos_root_url,
                                   const char *repos_uuid,
                                   svn_revnum_t def_peg_revision,
                                   svn_revnum_t def_revision,
                                   apr_pool_t *scratch_pool);

/* Get the repository identifier corresponding to REPOS_ROOT_URL from the
   database in SDB. The value is returned in *REPOS_ID. All allocations
   are allocated in SCRATCH_POOL.

   NOTE: the row in REPOSITORY must exist. If not, then SVN_ERR_WC_DB_ERROR
   is returned.

   ### unclear on whether/how this interface will stay/evolve.  */
svn_error_t *
svn_wc__db_upgrade_get_repos_id(apr_int64_t *repos_id,
                                svn_sqlite__db_t *sdb,
                                const char *repos_root_url,
                                apr_pool_t *scratch_pool);

/* @} */


/* @defgroup svn_wc__db_wq  Work queue manipulation. see workqueue.h
   @{
*/

/* In the WCROOT associated with DB and WRI_ABSPATH, add WORK_ITEM to the
   wcroot's work queue. Use SCRATCH_POOL for all temporary allocations.  */
svn_error_t *
svn_wc__db_wq_add(svn_wc__db_t *db,
                  const char *wri_abspath,
                  const svn_skel_t *work_item,
                  apr_pool_t *scratch_pool);


/* In the WCROOT associated with DB and WRI_ABSPATH, fetch a work item that
   needs to be completed. Its identifier is returned in ID, and the data in
   WORK_ITEM.

   Items are returned in the same order they were queued. This allows for
   (say) queueing work on a parent node to be handled before that of its
   children.

   If there are no work items to be completed, then ID will be set to zero,
   and WORK_ITEM to NULL.

   If COMPLETED_ID is not 0, the wq item COMPLETED_ID will be marked as
   completed before returning the next item.

   RESULT_POOL will be used to allocate WORK_ITEM, and SCRATCH_POOL
   will be used for all temporary allocations.  */
svn_error_t *
svn_wc__db_wq_fetch_next(apr_uint64_t *id,
                         svn_skel_t **work_item,
                         svn_wc__db_t *db,
                         const char *wri_abspath,
                         apr_uint64_t completed_id,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool);

/* @} */


/* Note: LEVELS_TO_LOCK is here strictly for backward compat.  The access
   batons still have the notion of 'levels to lock' and we need to ensure
   that they still function correctly, even in the new world.  'levels to
   lock' should not be exposed through the wc-ng APIs at all: users either
   get to lock the entire tree (rooted at some subdir, of course), or none.

   An infinite depth lock is obtained with LEVELS_TO_LOCK set to -1, but until
   we move to a single DB only depth 0 is supported.
*/
svn_error_t *
svn_wc__db_wclock_obtain(svn_wc__db_t *db,
                         const char *local_abspath,
                         int levels_to_lock,
                         svn_boolean_t steal_lock,
                         apr_pool_t *scratch_pool);

/* Check if somebody has a wclock on LOCAL_ABSPATH */
svn_error_t *
svn_wc__db_wclocked(svn_boolean_t *locked,
                    svn_wc__db_t *db,
                    const char *local_abspath,
                    apr_pool_t *scratch_pool);

/* Release the previously obtained lock on LOCAL_ABSPATH */
svn_error_t *
svn_wc__db_wclock_release(svn_wc__db_t *db,
                          const char *local_abspath,
                          apr_pool_t *scratch_pool);

/* Checks whether DB currently owns a lock to operate on LOCAL_ABSPATH.
   If EXACT is TRUE only lock roots are checked. */
svn_error_t *
svn_wc__db_wclock_owns_lock(svn_boolean_t *own_lock,
                            svn_wc__db_t *db,
                            const char *local_abspath,
                            svn_boolean_t exact,
                            apr_pool_t *scratch_pool);



/* @defgroup svn_wc__db_temp Various temporary functions during transition

  ### These functions SHOULD be completely removed before 1.7

  @{
*/

/* Removes all references to LOCAL_ABSPATH from DB, while optionally leaving
   tree conflicts and/or a not present node.

   This operation always recursively removes all nodes at and below
   LOCAL_ABSPATH from NODES and ACTUAL.

   If NOT_PRESENT_REVISION specifies a valid revision, leave a not_present
   BASE node at local_abspath. (Requires an existing BASE node before removing)
 */
svn_error_t *
svn_wc__db_op_remove_node(svn_wc__db_t *db,
                          const char *local_abspath,
                          svn_revnum_t not_present_revision,
                          svn_wc__db_kind_t not_present_kind,
                          apr_pool_t *scratch_pool);

/* Remove the WORKING_NODE row of LOCAL_ABSPATH in DB. */
svn_error_t *
svn_wc__db_temp_op_remove_working(svn_wc__db_t *db,
                                  const char *local_abspath,
                                  apr_pool_t *scratch_pool);

/* Sets the depth of LOCAL_ABSPATH in its working copy to DEPTH using DB.

   Returns SVN_ERR_WC_PATH_NOT_FOUND if LOCAL_ABSPATH is not a BASE directory
 */
svn_error_t *
svn_wc__db_op_set_base_depth(svn_wc__db_t *db,
                             const char *local_abspath,
                             svn_depth_t depth,
                             apr_pool_t *scratch_pool);

/* ### temp function. return the FORMAT for the directory LOCAL_ABSPATH.  */
svn_error_t *
svn_wc__db_temp_get_format(int *format,
                           svn_wc__db_t *db,
                           const char *local_dir_abspath,
                           apr_pool_t *scratch_pool);

/* ### temp functions to manage/store access batons within the DB.  */
svn_wc_adm_access_t *
svn_wc__db_temp_get_access(svn_wc__db_t *db,
                           const char *local_dir_abspath,
                           apr_pool_t *scratch_pool);
void
svn_wc__db_temp_set_access(svn_wc__db_t *db,
                           const char *local_dir_abspath,
                           svn_wc_adm_access_t *adm_access,
                           apr_pool_t *scratch_pool);
svn_error_t *
svn_wc__db_temp_close_access(svn_wc__db_t *db,
                             const char *local_dir_abspath,
                             svn_wc_adm_access_t *adm_access,
                             apr_pool_t *scratch_pool);
void
svn_wc__db_temp_clear_access(svn_wc__db_t *db,
                             const char *local_dir_abspath,
                             apr_pool_t *scratch_pool);

/* ### shallow hash: abspath -> svn_wc_adm_access_t *  */
apr_hash_t *
svn_wc__db_temp_get_all_access(svn_wc__db_t *db,
                               apr_pool_t *result_pool);

/* ### temp function to open the sqlite database to the appropriate location,
   ### then borrow it for a bit.
   ### The *only* reason for this function is because entries.c still
   ### manually hacks the sqlite database.

   ### No matter how tempted you may be DO NOT USE THIS FUNCTION!
   ### (if you do, gstein will hunt you down and burn your knee caps off
   ### in the middle of the night)
   ### "Bet on it." --gstein
*/
svn_error_t *
svn_wc__db_temp_borrow_sdb(svn_sqlite__db_t **sdb,
                           svn_wc__db_t *db,
                           const char *local_dir_abspath,
                           apr_pool_t *scratch_pool);


/* Return a directory in *TEMP_DIR_ABSPATH that is suitable for temporary
   files which may need to be moved (atomically and same-device) into the
   working copy indicated by WRI_ABSPATH.  */
svn_error_t *
svn_wc__db_temp_wcroot_tempdir(const char **temp_dir_abspath,
                               svn_wc__db_t *db,
                               const char *wri_abspath,
                               apr_pool_t *result_pool,
                               apr_pool_t *scratch_pool);

/* Update the BASE_NODE of directory LOCAL_ABSPATH to be NEW_REPOS_RELPATH
   at revision NEW_REV with status incomplete. */
svn_error_t *
svn_wc__db_temp_op_start_directory_update(svn_wc__db_t *db,
                                          const char *local_abspath,
                                          const char *new_repos_relpath,
                                          svn_revnum_t new_rev,
                                          apr_pool_t *scratch_pool);

/* Marks a directory update started with
   svn_wc__db_temp_op_start_directory_update as completed, by removing
   the incomplete status */
svn_error_t *
svn_wc__db_temp_op_end_directory_update(svn_wc__db_t *db,
                                        const char *local_dir_abspath,
                                        apr_pool_t *scratch_pool);


/* Copy the base tree at LOCAL_ABSPATH into the working tree as copy,
   leaving any subtree additions and copies as-is.  This allows the
   base node tree to be removed. */
svn_error_t *
svn_wc__db_temp_op_make_copy(svn_wc__db_t *db,
                             const char *local_abspath,
                             apr_pool_t *scratch_pool);


/* Set the conflict marker information on LOCAL_ABSPATH to the specified
   values */
svn_error_t *
svn_wc__db_temp_op_set_text_conflict_marker_files(svn_wc__db_t *db,
                                                  const char *local_abspath,
                                                  const char *old_abspath,
                                                  const char *new_abspath,
                                                  const char *wrk_abspath,
                                                  apr_pool_t *scratch_pool);

/* Set the conflict marker information on LOCAL_ABSPATH to the specified
   values */
svn_error_t *
svn_wc__db_temp_op_set_property_conflict_marker_file(svn_wc__db_t *db,
                                                     const char *local_abspath,
                                                     const char *prej_abspath,
                                                     apr_pool_t *scratch_pool);

/* Add a new directory in BASE, whether WORKING nodes exist or not. Mark it
   as incomplete and with revision REVISION. If REPOS_RELPATH is not NULL,
   apply REPOS_RELPATH, REPOS_ROOT_URL and REPOS_UUID.
   Perform all temporary allocations in SCRATCH_POOL.
   */
svn_error_t *
svn_wc__db_temp_op_set_new_dir_to_incomplete(svn_wc__db_t *db,
                                             const char *local_abspath,
                                             const char *repos_relpath,
                                             const char *repos_root_url,
                                             const char *repos_uuid,
                                             svn_revnum_t revision,
                                             svn_depth_t depth,
                                             apr_pool_t *scratch_pool);

/* Close the wc root LOCAL_ABSPATH and remove any per-directory
   handles associated with it. */
svn_error_t *
svn_wc__db_drop_root(svn_wc__db_t *db,
                     const char *local_abspath,
                     apr_pool_t *scratch_pool);

/* Return the OP_DEPTH for LOCAL_RELPATH. */
apr_int64_t svn_wc__db_op_depth_for_upgrade(const char *local_relpath);

/* Set *HAVE_WORK TRUE if there is a working layer below the top layer and
   *HAVE_BASE if there is a base layer. Set *STATUS to the status of the
   highest layer below WORKING */
svn_error_t *
svn_wc__db_info_below_working(svn_boolean_t *have_base,
                              svn_boolean_t *have_work,
                              svn_wc__db_status_t *status,
                              svn_wc__db_t *db,
                              const char *local_abspath,
                              apr_pool_t *scratch_pool);


/* Gets an array of const char *local_relpaths of descendants of LOCAL_ABSPATH,
 * which itself must be the op root of an addition, copy or move.
 * The descendants returned are at the same op_depth, but are to be deleted
 * by the commit processing because they are not present in the local copy.
 */
svn_error_t *
svn_wc__db_get_not_present_descendants(const apr_array_header_t **descendants,
                                       svn_wc__db_t *db,
                                       const char *local_abspath,
                                       apr_pool_t *result_pool,
                                       apr_pool_t *scratch_pool);

/* Gather revision status information about a working copy using DB.
 *
 * Set *MIN_REVISION and *MAX_REVISION to the lowest and highest revision
 * numbers found within LOCAL_ABSPATH.
 * Only nodes with op_depth zero and presence 'normal' or 'incomplete'
 * are considered, so that added, deleted or excluded nodes do not affect
 * the result.  If COMMITTED is TRUE, set *MIN_REVISION and *MAX_REVISION
 * to the lowest and highest comitted (i.e. "last changed") revision numbers,
 * respectively.
 *
 * Indicate in *IS_SPARSE_CHECKOUT whether any of the nodes within
 * LOCAL_ABSPATH is sparse.
 * Indicate in *IS_MODIFIED whether the working copy has local modifications.
 *
 * Indicate in *IS_SWITCHED whether any node beneath LOCAL_ABSPATH
 * is switched. If TRAIL_URL is non-NULL, use it to determine if LOCAL_ABSPATH
 * itself is switched.  It should be any trailing portion of LOCAL_ABSPATH's
 * expected URL, long enough to include any parts that the caller considers
 * might be changed by a switch.  If it does not match the end of WC_PATH's
 * actual URL, then report a "switched" status.
 *
 * See also the functions below which provide a subset of this functionality.
 */
svn_error_t *
svn_wc__db_revision_status(svn_revnum_t *min_revision,
                           svn_revnum_t *max_revision,
                           svn_boolean_t *is_sparse_checkout,
                           svn_boolean_t *is_modified,
                           svn_boolean_t *is_switched,
                           svn_wc__db_t *db,
                           const char *local_abspath,
                           const char *trail_url,
                           svn_boolean_t committed,
                           svn_cancel_func_t cancel_func,
                           void *cancel_baton,
                           apr_pool_t *scratch_pool);

/* Set *MIN_REVISION and *MAX_REVISION to the lowest and highest revision
 * numbers found within LOCAL_ABSPATH in the working copy using DB.
 * Only nodes with op_depth zero and presence 'normal' or 'incomplete'
 * are considered, so that added, deleted or excluded nodes do not affect
 * the result.  If COMMITTED is TRUE, set *MIN_REVISION and *MAX_REVISION
 * to the lowest and highest comitted (i.e. "last changed") revision numbers,
 * respectively. Use SCRATCH_POOL for temporary allocations.
 *
 * Either of MIN_REVISION and MAX_REVISION may be passed as NULL if
 * the caller doesn't care about that return value.
 *
 * This function provides a subset of the functionality of
 * svn_wc__db_revision_status() and is more efficient if the caller
 * doesn't need all information returned by svn_wc__db_revision_status(). */
svn_error_t *
svn_wc__db_min_max_revisions(svn_revnum_t *min_revision,
                             svn_revnum_t *max_revision,
                             svn_wc__db_t *db,
                             const char *local_abspath,
                             svn_boolean_t committed,
                             apr_pool_t *scratch_pool);

/* Indicate in *IS_SPARSE_CHECKOUT whether any of the nodes within
 * LOCAL_ABSPATH is sparse, using DB.
 * Use SCRATCH_POOL for temporary allocations.
 *
 * This function provides a subset of the functionality of
 * svn_wc__db_revision_status() and is more efficient if the caller
 * doesn't need all information returned by svn_wc__db_revision_status(). */
svn_error_t *
svn_wc__db_is_sparse_checkout(svn_boolean_t *is_sparse_checkout,
                              svn_wc__db_t *db,
                              const char *local_abspath,
                              apr_pool_t *scratch_pool);

/* Indicate in *IS_SWITCHED whether any node beneath LOCAL_ABSPATH
 * is switched, using DB. Use SCRATCH_POOL for temporary allocations.
 *
 * If TRAIL_URL is non-NULL, use it to determine if LOCAL_ABSPATH itself
 * is switched.  It should be any trailing portion of LOCAL_ABSPATH's
 * expected URL, long enough to include any parts that the caller considers
 * might be changed by a switch.  If it does not match the end of WC_PATH's
 * actual URL, then report a "switched" status.
 *
 * This function provides a subset of the functionality of
 * svn_wc__db_revision_status() and is more efficient if the caller
 * doesn't need all information returned by svn_wc__db_revision_status(). */
svn_error_t *
svn_wc__db_has_switched_subtrees(svn_boolean_t *is_switched,
                                 svn_wc__db_t *db,
                                 const char *local_abspath,
                                 const char *trail_url,
                                 apr_pool_t *scratch_pool);

/* Set @a *server_excluded_subtrees to a hash mapping <tt>const char *</tt>
 * local absolute paths to <tt>const char *</tt> local absolute paths for
 * every path at or under @a local_abspath in @a db which are excluded by
 * the server (e.g. due to authz).  If no such paths are found then
 * @a *server_excluded_subtrees is set to @c NULL.
 * Allocate the hash and all items therein from @a result_pool.
 */
svn_error_t *
svn_wc__db_get_server_excluded_subtrees(apr_hash_t **server_excluded_subtrees,
                                        svn_wc__db_t *db,
                                        const char *local_abspath,
                                        apr_pool_t *result_pool,
                                        apr_pool_t *scratch_pool);

/* Indicate in *IS_MODIFIED whether the working copy has local modifications,
 * using DB. Use SCRATCH_POOL for temporary allocations.
 *
 * This function provides a subset of the functionality of
 * svn_wc__db_revision_status() and is more efficient if the caller
 * doesn't need all information returned by svn_wc__db_revision_status(). */
svn_error_t *
svn_wc__db_has_local_mods(svn_boolean_t *is_modified,
                          svn_wc__db_t *db,
                          const char *local_abspath,
                          svn_cancel_func_t cancel_func,
                          void *cancel_baton,
                          apr_pool_t *scratch_pool);


/* Verify the consistency of metadata concerning the WC that contains
 * WRI_ABSPATH, in DB.  Return an error if any problem is found. */
svn_error_t *
svn_wc__db_verify(svn_wc__db_t *db,
                  const char *wri_abspath,
                  apr_pool_t *scratch_pool);


/* @} */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_WC_DB_H */
