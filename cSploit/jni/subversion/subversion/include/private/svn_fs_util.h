/*
 * svn_fs_util.h: Declarations for the APIs of libsvn_fs_util to be
 * consumed by only fs_* libs.
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

#ifndef SVN_FS_UTIL_H
#define SVN_FS_UTIL_H

#include <apr_pools.h>

#include "svn_types.h"
#include "svn_error.h"
#include "svn_fs.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Return a canonicalized version of a filesystem PATH, allocated in
   POOL.  While the filesystem API is pretty flexible about the
   incoming paths (they must be UTF-8 with '/' as separators, but they
   don't have to begin with '/', and multiple contiguous '/'s are
   ignored) we want any paths that are physically stored in the
   underlying database to look consistent.  Specifically, absolute
   filesystem paths should begin with '/', and all redundant and trailing '/'
   characters be removed.  */
const char *
svn_fs__canonicalize_abspath(const char *path, apr_pool_t *pool);

/* If EXPECT_OPEN, verify that FS refers to an open database;
   otherwise, verify that FS refers to an unopened database.  Return
   an appropriate error if the expectation fails to match the
   reality.  */
svn_error_t *
svn_fs__check_fs(svn_fs_t *fs, svn_boolean_t expect_open);

/* Constructing nice error messages for roots.  */

/* Build an SVN_ERR_FS_NOT_FOUND error, with a detailed error text,
   for PATH in ROOT. ROOT is of type svn_fs_root_t *. */
#define SVN_FS__NOT_FOUND(root, path) (                        \
  root->is_txn_root ?                                          \
    svn_error_createf                                          \
      (SVN_ERR_FS_NOT_FOUND, 0,                                \
       _("File not found: transaction '%s', path '%s'"),       \
       root->txn, path)                                        \
  :                                                            \
    svn_error_createf                                          \
      (SVN_ERR_FS_NOT_FOUND, 0,                                \
       _("File not found: revision %ld, path '%s'"),           \
       root->rev, path)                                        \
  )


/* Build a detailed `file already exists' message for PATH in ROOT.
   ROOT is of type svn_fs_root_t *. */
#define SVN_FS__ALREADY_EXISTS(root, path_str, pool) (                         \
  root->is_txn_root ?                                                          \
    svn_error_createf                                                          \
      (SVN_ERR_FS_ALREADY_EXISTS, 0,                                           \
       _("File already exists: filesystem '%s', transaction '%s', path '%s'"), \
       svn_dirent_local_style(root->fs->path, pool), root->txn, path_str)      \
  :                                                                            \
    svn_error_createf                                                          \
      (SVN_ERR_FS_ALREADY_EXISTS, 0,                                           \
       _("File already exists: filesystem '%s', revision %ld, path '%s'"),     \
       svn_dirent_local_style(root->fs->path, pool), root->rev, path_str)      \
  )

/* ROOT is of type svn_fs_root_t *. */
#define SVN_FS__NOT_TXN(root)                         \
  svn_error_create                                    \
    (SVN_ERR_FS_NOT_TXN_ROOT, NULL,                   \
     _("Root object must be a transaction root"))

/* SVN_FS__ERR_NOT_MUTABLE: the caller attempted to change a node
   outside of a transaction. FS is of type "svn_fs_t *". */
#define SVN_FS__ERR_NOT_MUTABLE(fs, rev, path_in_repo, scratch_pool)     \
  svn_error_createf(                                                     \
     SVN_ERR_FS_NOT_MUTABLE, 0,                                          \
     _("File is not mutable: filesystem '%s', revision %ld, path '%s'"), \
     svn_dirent_local_style(fs->path, scratch_pool), rev, path_in_repo)

/* FS is of type "svn fs_t *".*/
#define SVN_FS__ERR_NOT_DIRECTORY(fs, path_in_repo, scratch_pool) \
  svn_error_createf(                                              \
     SVN_ERR_FS_NOT_DIRECTORY, 0,                                 \
     _("'%s' is not a directory in filesystem '%s'"),             \
     path_in_repo, svn_dirent_local_style(fs->path, scratch_pool))

/* FS is of type "svn fs_t *".   */
#define SVN_FS__ERR_NOT_FILE(fs, path_in_repo, scratch_pool)      \
  svn_error_createf(                                              \
     SVN_ERR_FS_NOT_FILE, 0,                                      \
     _("'%s' is not a file in filesystem '%s'"),                  \
     path_in_repo, svn_dirent_local_style(fs->path, scratch_pool))


/* FS is of type "svn fs_t *", LOCK is of type "svn_lock_t *".   */
#define SVN_FS__ERR_PATH_ALREADY_LOCKED(fs, lock, scratch_pool)             \
  svn_error_createf(                                                        \
     SVN_ERR_FS_PATH_ALREADY_LOCKED, 0,                                     \
     _("Path '%s' is already locked by user '%s' in filesystem '%s'"),      \
     lock->path, lock->owner, svn_dirent_local_style(fs->path, scratch_pool))

/* FS is of type "svn fs_t *". */
#define SVN_FS__ERR_NO_SUCH_LOCK(fs, path_in_repo, scratch_pool)  \
  svn_error_createf(                                              \
     SVN_ERR_FS_NO_SUCH_LOCK, 0,                                  \
     _("No lock on path '%s' in filesystem '%s'"),                \
     path_in_repo, svn_dirent_local_style(fs->path, scratch_pool))

/* FS is of type "svn fs_t *". */
#define SVN_FS__ERR_LOCK_EXPIRED(fs, token, scratch_pool)        \
  svn_error_createf(                                             \
     SVN_ERR_FS_LOCK_EXPIRED, 0,                                 \
     _("Lock has expired: lock-token '%s' in filesystem '%s'"),  \
     token, svn_dirent_local_style(fs->path, scratch_pool))

/* FS is of type "svn fs_t *". */
#define SVN_FS__ERR_NO_USER(fs, scratch_pool)                       \
  svn_error_createf(                                                \
     SVN_ERR_FS_NO_USER, 0,                                         \
     _("No username is currently associated with filesystem '%s'"), \
     svn_dirent_local_style(fs->path, scratch_pool))

/* SVN_FS__ERR_LOCK_OWNER_MISMATCH: trying to use a lock whose
   LOCK_OWNER doesn't match the USERNAME associated with FS.
   FS is of type "svn fs_t *". */
#define SVN_FS__ERR_LOCK_OWNER_MISMATCH(fs, username, lock_owner, pool) \
  svn_error_createf(                                                    \
     SVN_ERR_FS_LOCK_OWNER_MISMATCH, 0,                                 \
     _("User '%s' is trying to use a lock owned by '%s' in "            \
       "filesystem '%s'"),                                              \
     username, lock_owner, svn_dirent_local_style(fs->path, pool))

/* Return a NULL-terminated copy of the first component of PATH,
   allocated in POOL.  If path is empty, or consists entirely of
   slashes, return the empty string.

   If the component is followed by one or more slashes, we set *NEXT_P
   to point after the slashes.  If the component ends PATH, we set
   *NEXT_P to zero.  This means:
   - If *NEXT_P is zero, then the component ends the PATH, and there
     are no trailing slashes in the path.
   - If *NEXT_P points at PATH's terminating NULL character, then
     the component returned was the last, and PATH ends with one or more
     slash characters.
   - Otherwise, *NEXT_P points to the beginning of the next component
     of PATH.  You can pass this value to next_entry_name to extract
     the next component. */
char *
svn_fs__next_entry_name(const char **next_p,
                        const char *path,
                        apr_pool_t *pool);

/* Allocate an svn_fs_path_change2_t structure in POOL, initialize and
   return it.

   Set the node_rev_id field of the created struct to NODE_REV_ID, and
   change_kind to CHANGE_KIND.  Set all other fields to their _unknown,
   NULL or invalid value, respectively. */
svn_fs_path_change2_t *
svn_fs__path_change_create_internal(const svn_fs_id_t *node_rev_id,
                                    svn_fs_path_change_kind_t change_kind,
                                    apr_pool_t *pool);

/* Append REL_PATH (which may contain slashes) to each path that exists in
   the mergeinfo INPUT, and return a new mergeinfo in *OUTPUT.  Deep
   copies the values.  Perform all allocations in POOL. */
svn_error_t *
svn_fs__append_to_merged_froms(svn_mergeinfo_t *output,
                               svn_mergeinfo_t input,
                               const char *rel_path,
                               apr_pool_t *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_FS_UTIL_H */
