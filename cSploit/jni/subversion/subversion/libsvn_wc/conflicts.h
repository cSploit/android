/*
 * conflicts.h: declarations related to conflicts
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

#ifndef SVN_WC_CONFLICTS_H
#define SVN_WC_CONFLICTS_H

#include <apr_pools.h>

#include "svn_types.h"
#include "svn_wc.h"

#include "wc_db.h"
#include "private/svn_skel.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#define SVN_WC__CONFLICT_OP_UPDATE "update"
#define SVN_WC__CONFLICT_OP_SWITCH "switch"
#define SVN_WC__CONFLICT_OP_MERGE "merge"
#define SVN_WC__CONFLICT_OP_PATCH "patch"

#define SVN_WC__CONFLICT_KIND_TEXT "text"
#define SVN_WC__CONFLICT_KIND_PROP "prop"
#define SVN_WC__CONFLICT_KIND_TREE "tree"
#define SVN_WC__CONFLICT_KIND_REJECT "reject"
#define SVN_WC__CONFLICT_KIND_OBSTRUCTED "obstructed"



/* Return a new conflict skel, allocated in RESULT_POOL. */
svn_skel_t *
svn_wc__conflict_skel_new(apr_pool_t *result_pool);


/* Set 'update' as the conflicting operation in CONFLICT_SKEL.
   Allocate data stored in the skel in RESULT_POOL.

   BASE_REVISION is the revision the node was at before the update.
   TARGET_REVISION is the revision being updated to.

   Do temporary allocations in SCRATCH_POOL. */
svn_error_t *
svn_wc__conflict_skel_set_op_update(svn_skel_t *conflict_skel,
                                    svn_revnum_t base_revision,
                                    svn_revnum_t target_revision,
                                    apr_pool_t *result_pool,
                                    apr_pool_t *scratch_pool);


/* Set 'switch' as the conflicting operation in CONFLICT_SKEL.
   Allocate data stored in the skel in RESULT_POOL.

   BASE_REVISION is the revision the node was at before the switch.
   TARGET_REVISION is the revision being switched to.
   REPOS_RELPATH is the path being switched to, relative to the
   repository root.

   Do temporary allocations in SCRATCH_POOL. */
svn_error_t *
svn_wc__conflict_skel_set_op_switch(svn_skel_t *conflict_skel,
                                    svn_revnum_t base_revision,
                                    svn_revnum_t target_revision,
                                    const char *repos_relpath,
                                    apr_pool_t *scratch_pool);


/* Set 'merge' as conflicting operation in CONFLICT_SKEL.
   Allocate data stored in the skel in RESULT_POOL.

   REPOS_UUID is the UUID of the repository accessed via REPOS_ROOT_URL.

   LEFT_REPOS_RELPATH and RIGHT_REPOS_RELPATH paths to the merge-left
   and merge-right merge sources, relative to REPOS_URL

   LEFT_REVISION is the merge-left revision.
   RIGHT_REVISION is the merge-right revision.

   Do temporary allocations in SCRATCH_POOL. */
svn_error_t *
svn_wc__conflict_skel_set_op_merge(svn_skel_t *conflict_skel,
                                   const char *repos_uuid,
                                   const char *repos_root_url,
                                   svn_revnum_t left_revision,
                                   const char *left_repos_relpath,
                                   svn_revnum_t right_revision,
                                   const char *right_repos_relpath,
                                   apr_pool_t *result_pool,
                                   apr_pool_t *scratch_pool);


/* Set 'patch' as the conflicting operation in CONFLICT_SKEL.
   Allocate data stored in the skel in RESULT_POOL.

   PATCH_SOURCE_LABEL is a string identifying the patch source in
   some way, for display purposes. It is usually the absolute path
   to the patch file, or a token such as "<stdin>" if the patch source
   is not a file.

   Do temporary allocations in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__conflict_skel_set_op_patch(svn_skel_t *conflict_skel,
                                   const char *patch_source_label,
                                   apr_pool_t *result_pool,
                                   apr_pool_t *scratch_pool);


/* Add a text conflict to CONFLICT_SKEL.
   Allocate data stored in the skel in RESULT_POOL.

   All checksums passed should be suitable for retreiving conflicted
   versions of the file from the pristine store.

   ORIGINAL_CHECKSUM is the checksum of the BASE version of the conflicted
   file (without local modifications).
   MINE_CHECKSUM is the checksum of the WORKING version of the conflicted
   file as of the time the conflicting operation was run (i.e. including
   local modifications).
   INCOMING_CHECKSUM is the checksum of the incoming file causing the
   conflict. ### is this needed for update? what about merge?

   It is an error (### which one?) if no conflicting operation has been
   set on CONFLICT_SKEL before calling this function.
   It is an error (### which one?) if CONFLICT_SKEL already contains
   a text conflict.

   Do temporary allocations in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__conflict_skel_add_text_conflict(
  svn_skel_t *conflict_skel,
  const svn_checksum_t *original_checksum,
  const svn_checksum_t *mine_checksum,
  const svn_checksum_t *incoming_checksum,
  apr_pool_t *result_pool,
  apr_pool_t *scratch_pool);


/* Add a property conflict to SKEL.

   PROP_NAME is the name of the conflicted property.

   ORIGINAL_VALUE is the property's value at the BASE revision. MINE_VALUE
   is the property's value in WORKING (BASE + local modifications).
   INCOMING_VALUE is the incoming property value brought in by the
   operation. When merging, INCOMING_BASE_VALUE is the base value against
   which INCOMING_VALUE ws being applied. For updates, INCOMING_BASE_VALUE
   should be the same as ORIGINAL_VALUE.

   *_VALUE may be NULL, indicating no value was present.

   It is an error (### which one?) if no conflicting operation has been
   set on CONFLICT_SKEL before calling this function.
   It is an error (### which one?) if CONFLICT_SKEL already cotains
   a propery conflict for PROP_NAME.

   The conflict recorded in SKEL will be allocated from RESULT_POOL. Do
   temporary allocations in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__conflict_skel_add_prop_conflict(
  svn_skel_t *skel,
  const char *prop_name,
  const svn_string_t *original_value,
  const svn_string_t *mine_value,
  const svn_string_t *incoming_value,
  const svn_string_t *incoming_base_value,
  apr_pool_t *result_pool,
  apr_pool_t *scratch_pool);


/* Add a tree conflict to CONFLICT_SKEL.
   Allocate data stored in the skel in RESULT_POOL.

   LOCAL_CHANGE is the local tree change made to the node.
   ORIGINAL_LOCAL_KIND is the kind of the local node in BASE.
   If ORIGINAL_LOCAL_KIND is svn_node_file, ORIGINAL_CHECKSUM is the checksum
   for the BASE of the file, for retrieval from the pristine store.

   MINE_LOCAL_KIND is the kind of the local node in WORKING at the
   time the conflict was flagged.
   If MINE_LOCAL_KIND is svn_node_file, ORIGINAL_CHECKSUM is the checksum
   of the WORKING version of the file at the time the conflict was flagged,
   for retrieval from the pristine store.

   INCOMING_KIND is the kind of the incoming node.
   If INCOMING_KIND is svn_node_file, INCOMING_CHECKSUM is the checksum
   of the INCOMING version of the file, for retrieval from the pristine store.

   It is an error (### which one?) if no conflicting operation has been
   set on CONFLICT_SKEL before calling this function.
   It is an error (### which one?) if CONFLICT_SKEL already contains
   a tree conflict.

   Do temporary allocations in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__conflict_skel_add_tree_conflict(
  svn_skel_t *skel,
  svn_wc_conflict_reason_t local_change,
  svn_wc__db_kind_t original_local_kind,
  const svn_checksum_t *original_checksum,
  svn_wc__db_kind_t mine_local_kind,
  const svn_checksum_t *mine_checksum,
  svn_wc_conflict_action_t incoming_change,
  svn_wc__db_kind_t incoming_kind,
  const svn_checksum_t *incoming_checksum,
  apr_pool_t *result_pool,
  apr_pool_t *scratch_pool);


/* Add a reject conflict to CONFLICT_SKEL.
   Allocate data stored in the skel in RESULT_POOL.

   HUNK_ORIGINAL_OFFSET, HUNK_ORIGINAL_LENGTH, HUNK_MODIFIED_OFFSET,
   and HUNK_MODIFIED_LENGTH is hunk-header data identifying the hunk
   which was rejected.

   REJECT_DIFF_CHECKSUM is the checksum of the text of the rejected
   diff, for retrieval from the pristine store.

   It is an error (### which one?) if no conflicting operation has been
   set on CONFLICT_SKEL before calling this function.
   It is an error (### which one?) if CONFLICT_SKEL already contains
   a reject conflict for the hunk.

   Do temporary allocations in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__conflict_skel_add_reject_conflict(
  svn_skel_t *conflict_skel,
  svn_linenum_t hunk_original_offset,
  svn_linenum_t hunk_original_length,
  svn_linenum_t hunk_modified_offset,
  svn_linenum_t hunk_modified_length,
  const svn_checksum_t *reject_diff_checksum,
  apr_pool_t *result_pool,
  apr_pool_t *scratch_pool);


/* Add an obstruction conflict to CONFLICT_SKEL.
   Allocate data stored in the skel in RESULT_POOL.

   It is an error (### which one?) if no conflicting operation has been
   set on CONFLICT_SKEL before calling this function.
   It is an error (### which one?) if CONFLICT_SKEL already contains
   an obstruction.

   Do temporary allocations in SCRATCH_POOL.
*/
svn_error_t *
svn_wc__conflict_skel_add_obstruction(svn_skel_t *conflict_skel,
                                      apr_pool_t *result_pool,
                                      apr_pool_t *scratch_pool);


/* Resolve text conflicts on the given node.  */
svn_error_t *
svn_wc__resolve_text_conflict(svn_wc__db_t *db,
                              const char *local_abspath,
                              apr_pool_t *scratch_pool);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_WC_CONFLICTS_H */
