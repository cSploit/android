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
 * @file svn_repos.h
 * @brief Tools built on top of the filesystem.
 */

#ifndef SVN_REPOS_H
#define SVN_REPOS_H

#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_tables.h>
#include <apr_time.h>

#include "svn_types.h"
#include "svn_string.h"
#include "svn_delta.h"
#include "svn_fs.h"
#include "svn_io.h"
#include "svn_mergeinfo.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ---------------------------------------------------------------*/

/**
 * Get libsvn_repos version information.
 *
 * @since New in 1.1.
 */
const svn_version_t *
svn_repos_version(void);


/* Some useful enums.  They need to be declared here for the notification
   system to pick them up. */
/** The different "actions" attached to nodes in the dumpfile. */
enum svn_node_action
{
  svn_node_action_change,
  svn_node_action_add,
  svn_node_action_delete,
  svn_node_action_replace
};

/** The different policies for processing the UUID in the dumpfile. */
enum svn_repos_load_uuid
{
  svn_repos_load_uuid_default,
  svn_repos_load_uuid_ignore,
  svn_repos_load_uuid_force
};


/** Callback type for checking authorization on paths produced by (at
 * least) svn_repos_dir_delta2().
 *
 * Set @a *allowed to TRUE to indicate that some operation is
 * authorized for @a path in @a root, or set it to FALSE to indicate
 * unauthorized (presumably according to state stored in @a baton).
 *
 * Do not assume @a pool has any lifetime beyond this call.
 *
 * The exact operation being authorized depends on the callback
 * implementation.  For read authorization, for example, the caller
 * would implement an instance that does read checking, and pass it as
 * a parameter named [perhaps] 'authz_read_func'.  The receiver of
 * that parameter might also take another parameter named
 * 'authz_write_func', which although sharing this type, would be a
 * different implementation.
 *
 * @note If someday we want more sophisticated authorization states
 * than just yes/no, @a allowed can become an enum type.
 */
typedef svn_error_t *(*svn_repos_authz_func_t)(svn_boolean_t *allowed,
                                               svn_fs_root_t *root,
                                               const char *path,
                                               void *baton,
                                               apr_pool_t *pool);


/** An enum defining the kinds of access authz looks up.
 *
 * @since New in 1.3.
 */
typedef enum svn_repos_authz_access_t
{
  /** No access. */
  svn_authz_none = 0,

  /** Path can be read. */
  svn_authz_read = 1,

  /** Path can be altered. */
  svn_authz_write = 2,

  /** The other access credentials are recursive. */
  svn_authz_recursive = 4
} svn_repos_authz_access_t;


/** Callback type for checking authorization on paths produced by
 * the repository commit editor.
 *
 * Set @a *allowed to TRUE to indicate that the @a required access on
 * @a path in @a root is authorized, or set it to FALSE to indicate
 * unauthorized (presumable according to state stored in @a baton).
 *
 * If @a path is NULL, the callback should perform a global authz
 * lookup for the @a required access.  That is, the lookup should
 * check if the @a required access is granted for at least one path of
 * the repository, and set @a *allowed to TRUE if so.  @a root may
 * also be NULL if @a path is NULL.
 *
 * This callback is very similar to svn_repos_authz_func_t, with the
 * exception of the addition of the @a required parameter.
 * This is due to historical reasons: when authz was first implemented
 * for svn_repos_dir_delta2(), it seemed there would need only checks
 * for read and write operations, hence the svn_repos_authz_func_t
 * callback prototype and usage scenario.  But it was then realized
 * that lookups due to copying needed to be recursive, and that
 * brute-force recursive lookups didn't square with the O(1)
 * performances a copy operation should have.
 *
 * So a special way to ask for a recursive lookup was introduced.  The
 * commit editor needs this capability to retain acceptable
 * performance.  Instead of revving the existing callback, causing
 * unnecessary revving of functions that don't actually need the
 * extended functionality, this second, more complete callback was
 * introduced, for use by the commit editor.
 *
 * Some day, it would be nice to reunite these two callbacks and do
 * the necessary revving anyway, but for the time being, this dual
 * callback mechanism will do.
 */
typedef svn_error_t *(*svn_repos_authz_callback_t)
  (svn_repos_authz_access_t required,
   svn_boolean_t *allowed,
   svn_fs_root_t *root,
   const char *path,
   void *baton,
   apr_pool_t *pool);

/**
 * Similar to #svn_file_rev_handler_t, but without the @a
 * result_of_merge parameter.
 *
 * @deprecated Provided for backward compatibility with 1.4 API.
 * @since New in 1.1.
 */
typedef svn_error_t *(*svn_repos_file_rev_handler_t)
  (void *baton,
   const char *path,
   svn_revnum_t rev,
   apr_hash_t *rev_props,
   svn_txdelta_window_handler_t *delta_handler,
   void **delta_baton,
   apr_array_header_t *prop_diffs,
   apr_pool_t *pool);


/* Notification system. */

/** The type of action occurring.
 *
 * @since New in 1.7.
 */
typedef enum svn_repos_notify_action_t
{
  /** A warning message is waiting. */
  svn_repos_notify_warning = 0,

  /** A revision has finished being dumped. */
  svn_repos_notify_dump_rev_end,

  /** A revision has finished being verified. */
  svn_repos_notify_verify_rev_end,

  /** All revisions have finished being dumped. */
  svn_repos_notify_dump_end,

  /** All revisions have finished being verified. */
  svn_repos_notify_verify_end,

  /** packing of an FSFS shard has commenced */
  svn_repos_notify_pack_shard_start,

  /** packing of an FSFS shard is completed */
  svn_repos_notify_pack_shard_end,

  /** packing of the shard revprops has commenced */
  svn_repos_notify_pack_shard_start_revprop,

  /** packing of the shard revprops has completed */
  svn_repos_notify_pack_shard_end_revprop,

  /** A revision has begun loading */
  svn_repos_notify_load_txn_start,

  /** A revision has finished loading */
  svn_repos_notify_load_txn_committed,

  /** A node has begun loading */
  svn_repos_notify_load_node_start,

  /** A node has finished loading */
  svn_repos_notify_load_node_done,

  /** A copied node has been encountered */
  svn_repos_notify_load_copied_node,

  /** Mergeinfo has been normalized */
  svn_repos_notify_load_normalized_mergeinfo,

  /** The operation has acquired a mutex for the repo. */
  svn_repos_notify_mutex_acquired,

  /** Recover has started. */
  svn_repos_notify_recover_start,

  /** Upgrade has started. */
  svn_repos_notify_upgrade_start

} svn_repos_notify_action_t;

/** The type of error occurring.
 *
 * @since New in 1.7.
 */
typedef enum svn_repos_notify_warning_t
{
  /** Referencing copy source data from a revision earlier than the
   * first revision dumped. */
  svn_repos_notify_warning_found_old_reference,

  /** An #SVN_PROP_MERGEINFO property's encoded mergeinfo references a
   * revision earlier than the first revision dumped. */
  svn_repos_notify_warning_found_old_mergeinfo,

  /** Found an invalid path in the filesystem.
   * @see svn_fs.h:"Directory entry names and directory paths" */
  /* ### TODO(doxygen): make that a proper doxygen link */
  /* See svn_fs__path_valid(). */
  svn_repos_notify_warning_invalid_fspath

} svn_repos_notify_warning_t;

/**
 * Structure used by #svn_repos_notify_func_t.
 *
 * The only field guaranteed to be populated is @c action.  Other fields are
 * dependent upon the @c action.  (See individual fields for more information.)
 *
 * @note Callers of notification functions should use
 * svn_repos_notify_create() to create structures of this type to allow for
 * future extensibility.
 *
 * @since New in 1.7.
 */
typedef struct svn_repos_notify_t
{
  /** Action that describes what happened in the repository. */
  svn_repos_notify_action_t action;

  /** For #svn_repos_notify_dump_rev_end and #svn_repos_notify_verify_rev_end,
   * the revision which just completed. */
  svn_revnum_t revision;

  /** For #svn_repos_notify_warning, the warning object. Must be cleared
      by the consumer of the notification. */
  const char *warning_str;
  svn_repos_notify_warning_t warning;

  /** For #svn_repos_notify_pack_shard_start,
      #svn_repos_notify_pack_shard_end,
      #svn_repos_notify_pack_shard_start_revprop, and
      #svn_repos_notify_pack_shard_end_revprop, the shard processed. */
  apr_int64_t shard;

  /** For #svn_repos_notify_load_committed_rev, the revision committed. */
  svn_revnum_t new_revision;

  /** For #svn_repos_notify_load_committed_rev, the source revision, if
      different from @a new_revision, otherwise #SVN_INVALID_REVNUM.
      For #svn_repos_notify_load_txn_start, the source revision. */
  svn_revnum_t old_revision;

  /** For #svn_repos_notify_load_node_start, the action being taken on the
      node. */
  enum svn_node_action node_action;

  /** For #svn_repos_notify_load_node_start, the path of the node. */
  const char *path;

  /* NOTE: Add new fields at the end to preserve binary compatibility.
     Also, if you add fields here, you have to update
     svn_repos_notify_create(). */
} svn_repos_notify_t;

/** Callback for providing notification from the repository.
 * Returns @c void.  Justification: success of an operation is not dependent
 * upon successful notification of that operation.
 *
 * @since New in 1.7. */
typedef void (*svn_repos_notify_func_t)(void *baton,
                                        const svn_repos_notify_t *notify,
                                        apr_pool_t *scratch_pool);

/**
 * Allocate an #svn_repos_notify_t structure in @a result_pool, initialize
 * and return it.
 *
 * @since New in 1.7.
 */
svn_repos_notify_t *
svn_repos_notify_create(svn_repos_notify_action_t action,
                        apr_pool_t *result_pool);


/** The repository object. */
typedef struct svn_repos_t svn_repos_t;

/* Opening and creating repositories. */


/** Find the root path of the repository that contains @a path.
 *
 * If a repository was found, the path to the root of the repository
 * is returned, else @c NULL. The pointer to the returned path may be
 * equal to @a path.
 */
const char *
svn_repos_find_root_path(const char *path,
                         apr_pool_t *pool);

/** Set @a *repos_p to a repository object for the repository at @a path.
 *
 * Allocate @a *repos_p in @a pool.
 *
 * Acquires a shared lock on the repository, and attaches a cleanup
 * function to @a pool to remove the lock.  If no lock can be acquired,
 * returns error, with undefined effect on @a *repos_p.  If an exclusive
 * lock is present, this blocks until it's gone.  @a fs_config will be
 * passed to the filesystem initialization function and may be @c NULL.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_repos_open2(svn_repos_t **repos_p,
                const char *path,
                apr_hash_t *fs_config,
                apr_pool_t *pool);

/** Similar to svn_repos_open2() with @a fs_config set to NULL.
 *
 * @deprecated Provided for backward compatibility with 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_open(svn_repos_t **repos_p,
               const char *path,
               apr_pool_t *pool);

/** Create a new Subversion repository at @a path, building the necessary
 * directory structure, creating the filesystem, and so on.
 * Return the repository object in @a *repos_p, allocated in @a pool.
 *
 * @a config is a client configuration hash of #svn_config_t * items
 * keyed on config category names, and may be NULL.
 *
 * @a fs_config is passed to the filesystem, and may be NULL.
 *
 * @a unused_1 and @a unused_2 are not used and should be NULL.
 */
svn_error_t *
svn_repos_create(svn_repos_t **repos_p,
                 const char *path,
                 const char *unused_1,
                 const char *unused_2,
                 apr_hash_t *config,
                 apr_hash_t *fs_config,
                 apr_pool_t *pool);

/**
 * Upgrade the Subversion repository (and its underlying versioned
 * filesystem) located in the directory @a path to the latest version
 * supported by this library.  If the requested upgrade is not
 * supported due to the current state of the repository or it
 * underlying filesystem, return #SVN_ERR_REPOS_UNSUPPORTED_UPGRADE
 * or #SVN_ERR_FS_UNSUPPORTED_UPGRADE (respectively) and make no
 * changes to the repository or filesystem.
 *
 * Acquires an exclusive lock on the repository, upgrades the
 * repository, and releases the lock.  If an exclusive lock can't be
 * acquired, returns error.
 *
 * If @a nonblocking is TRUE, an error of type EWOULDBLOCK is
 * returned if the lock is not immediately available.
 *
 * If @a start_callback is not NULL, it will be called with @a
 * start_callback_baton as argument before the upgrade starts, but
 * after the exclusive lock has been acquired.
 *
 * Use @a pool for necessary allocations.
 *
 * @note This functionality is provided as a convenience for
 * administrators wishing to make use of new Subversion functionality
 * without a potentially costly full repository dump/load.  As such,
 * the operation performs only the minimum amount of work needed to
 * accomplish this while maintaining the integrity of the repository.
 * It does *not* guarantee the most optimized repository state as a
 * dump and subsequent load would.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_repos_upgrade2(const char *path,
                   svn_boolean_t nonblocking,
                   svn_repos_notify_func_t notify_func,
                   void *notify_baton,
                   apr_pool_t *pool);

/**
 * Similar to svn_repos_upgrade2(), but with @a start_callback and baton,
 * rather than a notify_callback / baton
 *
 * @since New in 1.5.
 * @deprecated Provided for backward compatibility with the 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_upgrade(const char *path,
                  svn_boolean_t nonblocking,
                  svn_error_t *(*start_callback)(void *baton),
                  void *start_callback_baton,
                  apr_pool_t *pool);

/** Destroy the Subversion repository found at @a path, using @a pool for any
 * necessary allocations.
 */
svn_error_t *
svn_repos_delete(const char *path,
                 apr_pool_t *pool);

/**
 * Set @a *has to TRUE if @a repos has @a capability (one of the
 * capabilities beginning with @c "SVN_REPOS_CAPABILITY_"), else set
 * @a *has to FALSE.
 *
 * If @a capability isn't recognized, throw #SVN_ERR_UNKNOWN_CAPABILITY,
 * with the effect on @a *has undefined.
 *
 * Use @a pool for all allocation.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_has_capability(svn_repos_t *repos,
                         svn_boolean_t *has,
                         const char *capability,
                         apr_pool_t *pool);

/**
 * The capability of doing the right thing with merge-tracking
 * information, both storing it and responding to queries about it.
 *
 * @since New in 1.5.
 */
#define SVN_REPOS_CAPABILITY_MERGEINFO "mergeinfo"
/*       *** PLEASE READ THIS IF YOU ADD A NEW CAPABILITY ***
 *
 * @c SVN_REPOS_CAPABILITY_foo strings should not include colons, to
 * be consistent with @c SVN_RA_CAPABILITY_foo strings, which forbid
 * colons for their own reasons.  While this RA limitation has no
 * direct impact on repository capabilities, there's no reason to be
 * gratuitously different either.
 */


/** Return the filesystem associated with repository object @a repos. */
svn_fs_t *
svn_repos_fs(svn_repos_t *repos);


/** Make a hot copy of the Subversion repository found at @a src_path
 * to @a dst_path.
 *
 * Copy a possibly live Subversion repository from @a src_path to
 * @a dst_path.  If @a clean_logs is @c TRUE, perform cleanup on the
 * source filesystem as part of the copy operation; currently, this
 * means deleting copied, unused logfiles for a Berkeley DB source
 * repository.
 */
svn_error_t *
svn_repos_hotcopy(const char *src_path,
                  const char *dst_path,
                  svn_boolean_t clean_logs,
                  apr_pool_t *pool);


/**
 * Possibly update the repository, @a repos, to use a more efficient
 * filesystem representation.  Use @a pool for allocations.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_repos_fs_pack2(svn_repos_t *repos,
                   svn_repos_notify_func_t notify_func,
                   void *notify_baton,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   apr_pool_t *pool);

/**
 * Similar to svn_repos_fs_pack2(), but with a #svn_fs_pack_notify_t instead
 * of a #svn_repos_notify_t.
 *
 * @since New in 1.6.
 * @deprecated Provided for backward compatibility with the 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_fs_pack(svn_repos_t *repos,
                  svn_fs_pack_notify_t notify_func,
                  void *notify_baton,
                  svn_cancel_func_t cancel_func,
                  void *cancel_baton,
                  apr_pool_t *pool);

/**
 * Run database recovery procedures on the repository at @a path,
 * returning the database to a consistent state.  Use @a pool for all
 * allocation.
 *
 * Acquires an exclusive lock on the repository, recovers the
 * database, and releases the lock.  If an exclusive lock can't be
 * acquired, returns error.
 *
 * If @a nonblocking is TRUE, an error of type EWOULDBLOCK is
 * returned if the lock is not immediately available.
 *
 * If @a notify_func is not NULL, it will be called with @a
 * notify_baton as argument before the recovery starts, but
 * after the exclusive lock has been acquired.
 *
 * If @a cancel_func is not @c NULL, it is called periodically with
 * @a cancel_baton as argument to see if the client wishes to cancel
 * the recovery.
 *
 * @note On some platforms the exclusive lock does not exclude other
 * threads in the same process so this function should only be called
 * by a single threaded process, or by a multi-threaded process when
 * no other threads are accessing the repository.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_repos_recover4(const char *path,
                   svn_boolean_t nonblocking,
                   svn_repos_notify_func_t notify_func,
                   void *notify_baton,
                   svn_cancel_func_t cancel_func,
                   void * cancel_baton,
                   apr_pool_t *pool);

/**
 * Similar to svn_repos_recover4(), but with @a start callback in place of
 * the notify_func / baton.
 *
 * @since New in 1.5.
 * @deprecated Provided for backward compatibility with the 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_recover3(const char *path,
                   svn_boolean_t nonblocking,
                   svn_error_t *(*start_callback)(void *baton),
                   void *start_callback_baton,
                   svn_cancel_func_t cancel_func,
                   void * cancel_baton,
                   apr_pool_t *pool);

/**
 * Similar to svn_repos_recover3(), but without cancellation support.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_recover2(const char *path,
                   svn_boolean_t nonblocking,
                   svn_error_t *(*start_callback)(void *baton),
                   void *start_callback_baton,
                   apr_pool_t *pool);

/**
 * Similar to svn_repos_recover2(), but with nonblocking set to FALSE, and
 * with no callbacks provided.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_recover(const char *path,
                  apr_pool_t *pool);

/** This function is a wrapper around svn_fs_berkeley_logfiles(),
 * returning log file paths relative to the root of the repository.
 *
 * @copydoc svn_fs_berkeley_logfiles()
 */
svn_error_t *
svn_repos_db_logfiles(apr_array_header_t **logfiles,
                      const char *path,
                      svn_boolean_t only_unused,
                      apr_pool_t *pool);



/* Repository Paths */

/** Return the top-level repository path allocated in @a pool. */
const char *
svn_repos_path(svn_repos_t *repos,
               apr_pool_t *pool);

/** Return the path to @a repos's filesystem directory, allocated in
 * @a pool.
 */
const char *
svn_repos_db_env(svn_repos_t *repos,
                 apr_pool_t *pool);

/** Return path to @a repos's config directory, allocated in @a pool. */
const char *
svn_repos_conf_dir(svn_repos_t *repos,
                   apr_pool_t *pool);

/** Return path to @a repos's svnserve.conf, allocated in @a pool. */
const char *
svn_repos_svnserve_conf(svn_repos_t *repos,
                        apr_pool_t *pool);

/** Return path to @a repos's lock directory, allocated in @a pool. */
const char *
svn_repos_lock_dir(svn_repos_t *repos,
                   apr_pool_t *pool);

/** Return path to @a repos's db lockfile, allocated in @a pool. */
const char *
svn_repos_db_lockfile(svn_repos_t *repos,
                      apr_pool_t *pool);

/** Return path to @a repos's db logs lockfile, allocated in @a pool. */
const char *
svn_repos_db_logs_lockfile(svn_repos_t *repos,
                           apr_pool_t *pool);

/** Return the path to @a repos's hook directory, allocated in @a pool. */
const char *
svn_repos_hook_dir(svn_repos_t *repos,
                   apr_pool_t *pool);

/** Return the path to @a repos's start-commit hook, allocated in @a pool. */
const char *
svn_repos_start_commit_hook(svn_repos_t *repos,
                            apr_pool_t *pool);

/** Return the path to @a repos's pre-commit hook, allocated in @a pool. */
const char *
svn_repos_pre_commit_hook(svn_repos_t *repos,
                          apr_pool_t *pool);

/** Return the path to @a repos's post-commit hook, allocated in @a pool. */
const char *
svn_repos_post_commit_hook(svn_repos_t *repos,
                           apr_pool_t *pool);

/** Return the path to @a repos's pre-revprop-change hook, allocated in
 * @a pool.
 */
const char *
svn_repos_pre_revprop_change_hook(svn_repos_t *repos,
                                  apr_pool_t *pool);

/** Return the path to @a repos's post-revprop-change hook, allocated in
 * @a pool.
 */
const char *
svn_repos_post_revprop_change_hook(svn_repos_t *repos,
                                   apr_pool_t *pool);


/** @defgroup svn_repos_lock_hooks Paths to lock hooks
 * @{
 * @since New in 1.2. */

/** Return the path to @a repos's pre-lock hook, allocated in @a pool. */
const char *
svn_repos_pre_lock_hook(svn_repos_t *repos,
                        apr_pool_t *pool);

/** Return the path to @a repos's post-lock hook, allocated in @a pool. */
const char *
svn_repos_post_lock_hook(svn_repos_t *repos,
                         apr_pool_t *pool);

/** Return the path to @a repos's pre-unlock hook, allocated in @a pool. */
const char *
svn_repos_pre_unlock_hook(svn_repos_t *repos,
                          apr_pool_t *pool);

/** Return the path to @a repos's post-unlock hook, allocated in @a pool. */
const char *
svn_repos_post_unlock_hook(svn_repos_t *repos,
                           apr_pool_t *pool);

/** @} */

/* ---------------------------------------------------------------*/

/* Reporting the state of a working copy, for updates. */


/**
 * Construct and return a @a report_baton that will be passed to the
 * other functions in this section to describe the state of a pre-existing
 * tree (typically, a working copy).  When the report is finished,
 * @a editor/@a edit_baton will be driven in such a way as to transform the
 * existing tree to @a revnum and, if @a tgt_path is non-NULL, switch the
 * reported hierarchy to @a tgt_path.
 *
 * @a fs_base is the absolute path of the node in the filesystem at which
 * the comparison should be rooted.  @a target is a single path component,
 * used to limit the scope of the report to a single entry of @a fs_base,
 * or "" if all of @a fs_base itself is the main subject of the report.
 *
 * @a tgt_path and @a revnum is the fs path/revision pair that is the
 * "target" of the delta.  @a tgt_path should be provided only when
 * the source and target paths of the report differ.  That is, @a tgt_path
 * should *only* be specified when specifying that the resultant editor
 * drive be one that transforms the reported hierarchy into a pristine tree
 * of @a tgt_path at revision @a revnum.  A @c NULL value for @a tgt_path
 * will indicate that the editor should be driven in such a way as to
 * transform the reported hierarchy to revision @a revnum, preserving the
 * reported hierarchy.
 *
 * @a text_deltas instructs the driver of the @a editor to enable
 * the generation of text deltas.
 *
 * @a ignore_ancestry instructs the driver to ignore node ancestry
 * when determining how to transmit differences.
 *
 * @a send_copyfrom_args instructs the driver to send 'copyfrom'
 * arguments to the editor's add_file() and add_directory() methods,
 * whenever it deems feasible.
 *
 * Use @a authz_read_func and @a authz_read_baton (if not @c NULL) to
 * avoid sending data through @a editor/@a edit_baton which is not
 * authorized for transmission.
 *
 * All allocation for the context and collected state will occur in
 * @a pool.
 *
 * @a depth is the requested depth of the editor drive.
 *
 * If @a depth is #svn_depth_unknown, the editor will affect only the
 * paths reported by the individual calls to svn_repos_set_path3() and
 * svn_repos_link_path3().
 *
 * For example, if the reported tree is the @c A subdir of the Greek Tree
 * (see Subversion's test suite), at depth #svn_depth_empty, but the
 * @c A/B subdir is reported at depth #svn_depth_infinity, then
 * repository-side changes to @c A/mu, or underneath @c A/C and @c
 * A/D, would not be reflected in the editor drive, but changes
 * underneath @c A/B would be.
 *
 * Additionally, the editor driver will call @c add_directory and
 * and @c add_file for directories with an appropriate depth.  For
 * example, a directory reported at #svn_depth_files will receive
 * file (but not directory) additions.  A directory at #svn_depth_empty
 * will receive neither.
 *
 * If @a depth is #svn_depth_files, #svn_depth_immediates or
 * #svn_depth_infinity and @a depth is greater than the reported depth
 * of the working copy, then the editor driver will emit editor
 * operations so as to upgrade the working copy to this depth.
 *
 * If @a depth is #svn_depth_empty, #svn_depth_files,
 * #svn_depth_immediates and @a depth is lower
 * than or equal to the depth of the working copy, then the editor
 * operations will affect only paths at or above @a depth.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_begin_report2(void **report_baton,
                        svn_revnum_t revnum,
                        svn_repos_t *repos,
                        const char *fs_base,
                        const char *target,
                        const char *tgt_path,
                        svn_boolean_t text_deltas,
                        svn_depth_t depth,
                        svn_boolean_t ignore_ancestry,
                        svn_boolean_t send_copyfrom_args,
                        const svn_delta_editor_t *editor,
                        void *edit_baton,
                        svn_repos_authz_func_t authz_read_func,
                        void *authz_read_baton,
                        apr_pool_t *pool);

/**
 * The same as svn_repos_begin_report2(), but taking a boolean
 * @a recurse flag, and sending FALSE for @a send_copyfrom_args.
 *
 * If @a recurse is TRUE, the editor driver will drive the editor with
 * a depth of #svn_depth_infinity; if FALSE, then with a depth of
 * #svn_depth_files.
 *
 * @note @a username is ignored, and has been removed in a revised
 * version of this API.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_begin_report(void **report_baton,
                       svn_revnum_t revnum,
                       const char *username,
                       svn_repos_t *repos,
                       const char *fs_base,
                       const char *target,
                       const char *tgt_path,
                       svn_boolean_t text_deltas,
                       svn_boolean_t recurse,
                       svn_boolean_t ignore_ancestry,
                       const svn_delta_editor_t *editor,
                       void *edit_baton,
                       svn_repos_authz_func_t authz_read_func,
                       void *authz_read_baton,
                       apr_pool_t *pool);


/**
 * Given a @a report_baton constructed by svn_repos_begin_report2(),
 * record the presence of @a path, at @a revision with depth @a depth,
 * in the current tree.
 *
 * @a path is relative to the anchor/target used in the creation of the
 * @a report_baton.
 *
 * @a revision may be SVN_INVALID_REVNUM if (for example) @a path
 * represents a locally-added path with no revision number, or @a
 * depth is #svn_depth_exclude.
 *
 * @a path may not be underneath a path on which svn_repos_set_path3()
 * was previously called with #svn_depth_exclude in this report.
 *
 * The first call of this in a given report usually passes an empty
 * @a path; this is used to set up the correct root revision for the editor
 * drive.
 *
 * A depth of #svn_depth_unknown is not allowed, and results in an
 * error.
 *
 * If @a start_empty is TRUE and @a path is a directory, then require the
 * caller to explicitly provide all the children of @a path - do not assume
 * that the tree also contains all the children of @a path at @a revision.
 * This is for 'low confidence' client reporting.
 *
 * If the caller has a lock token for @a path, then @a lock_token should
 * be set to that token.  Else, @a lock_token should be NULL.
 *
 * All temporary allocations are done in @a pool.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_set_path3(void *report_baton,
                    const char *path,
                    svn_revnum_t revision,
                    svn_depth_t depth,
                    svn_boolean_t start_empty,
                    const char *lock_token,
                    apr_pool_t *pool);

/**
 * Similar to svn_repos_set_path3(), but with @a depth set to
 * #svn_depth_infinity.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_set_path2(void *report_baton,
                    const char *path,
                    svn_revnum_t revision,
                    svn_boolean_t start_empty,
                    const char *lock_token,
                    apr_pool_t *pool);

/**
 * Similar to svn_repos_set_path2(), but with @a lock_token set to @c NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_set_path(void *report_baton,
                   const char *path,
                   svn_revnum_t revision,
                   svn_boolean_t start_empty,
                   apr_pool_t *pool);

/**
 * Given a @a report_baton constructed by svn_repos_begin_report2(),
 * record the presence of @a path in the current tree, containing the contents
 * of @a link_path at @a revision with depth @a depth.
 *
 * A depth of #svn_depth_unknown is not allowed, and results in an
 * error.
 *
 * @a path may not be underneath a path on which svn_repos_set_path3()
 * was previously called with #svn_depth_exclude in this report.
 *
 * Note that while @a path is relative to the anchor/target used in the
 * creation of the @a report_baton, @a link_path is an absolute filesystem
 * path!
 *
 * If @a start_empty is TRUE and @a path is a directory, then require the
 * caller to explicitly provide all the children of @a path - do not assume
 * that the tree also contains all the children of @a link_path at
 * @a revision.  This is for 'low confidence' client reporting.
 *
 * If the caller has a lock token for @a link_path, then @a lock_token
 * should be set to that token.  Else, @a lock_token should be NULL.
 *
 * All temporary allocations are done in @a pool.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_link_path3(void *report_baton,
                     const char *path,
                     const char *link_path,
                     svn_revnum_t revision,
                     svn_depth_t depth,
                     svn_boolean_t start_empty,
                     const char *lock_token,
                     apr_pool_t *pool);

/**
 * Similar to svn_repos_link_path3(), but with @a depth set to
 * #svn_depth_infinity.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_link_path2(void *report_baton,
                     const char *path,
                     const char *link_path,
                     svn_revnum_t revision,
                     svn_boolean_t start_empty,
                     const char *lock_token,
                     apr_pool_t *pool);

/**
 * Similar to svn_repos_link_path2(), but with @a lock_token set to @c NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_link_path(void *report_baton,
                    const char *path,
                    const char *link_path,
                    svn_revnum_t revision,
                    svn_boolean_t start_empty,
                    apr_pool_t *pool);

/** Given a @a report_baton constructed by svn_repos_begin_report2(),
 * record the non-existence of @a path in the current tree.
 *
 * @a path may not be underneath a path on which svn_repos_set_path3()
 * was previously called with #svn_depth_exclude in this report.
 *
 * (This allows the reporter's driver to describe missing pieces of a
 * working copy, so that 'svn up' can recreate them.)
 *
 * All temporary allocations are done in @a pool.
 */
svn_error_t *
svn_repos_delete_path(void *report_baton,
                      const char *path,
                      apr_pool_t *pool);

/** Given a @a report_baton constructed by svn_repos_begin_report2(),
 * finish the report and drive the editor as specified when the report
 * baton was constructed.
 *
 * If an error occurs during the driving of the editor, do NOT abort the
 * edit; that responsibility belongs to the caller of this function, if
 * it happens at all.
 *
 * After the call to this function, @a report_baton is no longer valid;
 * it should not be passed to any other reporting functions, including
 * svn_repos_abort_report(), even if this function returns an error.
 */
svn_error_t *
svn_repos_finish_report(void *report_baton,
                        apr_pool_t *pool);


/** Given a @a report_baton constructed by svn_repos_begin_report2(),
 * abort the report.  This function can be called anytime before
 * svn_repos_finish_report() is called.
 *
 * After the call to this function, @a report_baton is no longer valid;
 * it should not be passed to any other reporting functions.
 */
svn_error_t *
svn_repos_abort_report(void *report_baton,
                       apr_pool_t *pool);


/* ---------------------------------------------------------------*/

/* The magical dir_delta update routines. */

/** Use the provided @a editor and @a edit_baton to describe the changes
 * necessary for making a given node (and its descendants, if it is a
 * directory) under @a src_root look exactly like @a tgt_path under
 * @a tgt_root.  @a src_entry is the node to update.  If @a src_entry
 * is empty, then compute the difference between the entire tree
 * anchored at @a src_parent_dir under @a src_root and @a tgt_path
 * under @a tgt_root.  Else, describe the changes needed to update
 * only that entry in @a src_parent_dir.  Typically, callers of this
 * function will use a @a tgt_path that is the concatenation of @a
 * src_parent_dir and @a src_entry.
 *
 * @a src_root and @a tgt_root can both be either revision or transaction
 * roots.  If @a tgt_root is a revision, @a editor's set_target_revision()
 * will be called with the @a tgt_root's revision number, else it will
 * not be called at all.
 *
 * If @a authz_read_func is non-NULL, invoke it before any call to
 *
 *    @a editor->open_root
 *    @a editor->add_directory
 *    @a editor->open_directory
 *    @a editor->add_file
 *    @a editor->open_file
 *
 * passing @a tgt_root, the same path that would be passed to the
 * editor function in question, and @a authz_read_baton.  If the
 * @a *allowed parameter comes back TRUE, then proceed with the planned
 * editor call; else if FALSE, then invoke @a editor->absent_file or
 * @a editor->absent_directory as appropriate, except if the planned
 * editor call was open_root, throw SVN_ERR_AUTHZ_ROOT_UNREADABLE.
 *
 * If @a text_deltas is @c FALSE, send a single @c NULL txdelta window to
 * the window handler returned by @a editor->apply_textdelta().
 *
 * If @a depth is #svn_depth_empty, invoke @a editor calls only on
 * @a src_entry (or @a src_parent_dir, if @a src_entry is empty).
 * If @a depth is #svn_depth_files, also invoke the editor on file
 * children, if any; if #svn_depth_immediates, invoke it on
 * immediate subdirectories as well as files; if #svn_depth_infinity,
 * recurse fully.
 *
 * If @a entry_props is @c TRUE, accompany each opened/added entry with
 * propchange editor calls that relay special "entry props" (this
 * is typically used only for working copy updates).
 *
 * @a ignore_ancestry instructs the function to ignore node ancestry
 * when determining how to transmit differences.
 *
 * Before completing successfully, this function calls @a editor's
 * close_edit(), so the caller should expect its @a edit_baton to be
 * invalid after its use with this function.
 *
 * Do any allocation necessary for the delta computation in @a pool.
 * This function's maximum memory consumption is at most roughly
 * proportional to the greatest depth of the tree under @a tgt_root, not
 * the total size of the delta.
 *
 * ### svn_repos_dir_delta2 is mostly superseded by the reporter
 * ### functionality (svn_repos_begin_report2 and friends).
 * ### svn_repos_dir_delta2 does allow the roots to be transaction
 * ### roots rather than just revision roots, and it has the
 * ### entry_props flag.  Almost all of Subversion's own code uses the
 * ### reporter instead; there are some stray references to the
 * ### svn_repos_dir_delta[2] in comments which should probably
 * ### actually refer to the reporter.
 */
svn_error_t *
svn_repos_dir_delta2(svn_fs_root_t *src_root,
                     const char *src_parent_dir,
                     const char *src_entry,
                     svn_fs_root_t *tgt_root,
                     const char *tgt_path,
                     const svn_delta_editor_t *editor,
                     void *edit_baton,
                     svn_repos_authz_func_t authz_read_func,
                     void *authz_read_baton,
                     svn_boolean_t text_deltas,
                     svn_depth_t depth,
                     svn_boolean_t entry_props,
                     svn_boolean_t ignore_ancestry,
                     apr_pool_t *pool);

/**
 * Similar to svn_repos_dir_delta2(), but if @a recurse is TRUE, pass
 * #svn_depth_infinity for @a depth, and if @a recurse is FALSE,
 * pass #svn_depth_files for @a depth.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_dir_delta(svn_fs_root_t *src_root,
                    const char *src_parent_dir,
                    const char *src_entry,
                    svn_fs_root_t *tgt_root,
                    const char *tgt_path,
                    const svn_delta_editor_t *editor,
                    void *edit_baton,
                    svn_repos_authz_func_t authz_read_func,
                    void *authz_read_baton,
                    svn_boolean_t text_deltas,
                    svn_boolean_t recurse,
                    svn_boolean_t entry_props,
                    svn_boolean_t ignore_ancestry,
                    apr_pool_t *pool);


/** Use the provided @a editor and @a edit_baton to describe the
 * skeletal changes made in a particular filesystem @a root
 * (revision or transaction).
 *
 * Changes will be limited to those within @a base_dir, and if
 * @a low_water_mark is set to something other than #SVN_INVALID_REVNUM
 * it is assumed that the client has no knowledge of revisions prior to
 * @a low_water_mark.  Together, these two arguments define the portion of
 * the tree that the client is assumed to have knowledge of, and thus any
 * copies of data from outside that part of the tree will be sent in their
 * entirety, not as simple copies or deltas against a previous version.
 *
 * The @a editor passed to this function should be aware of the fact
 * that, if @a send_deltas is FALSE, calls to its change_dir_prop(),
 * change_file_prop(), and apply_textdelta() functions will not
 * contain meaningful data, and merely serve as indications that
 * properties or textual contents were changed.
 *
 * If @a send_deltas is @c TRUE, the text and property deltas for changes
 * will be sent, otherwise NULL text deltas and empty prop changes will be
 * used.
 *
 * If @a authz_read_func is non-NULL, it will be used to determine if the
 * user has read access to the data being accessed.  Data that the user
 * cannot access will be skipped.
 *
 * @note This editor driver passes SVN_INVALID_REVNUM for all
 * revision parameters in the editor interface except the copyfrom
 * parameter of the add_file() and add_directory() editor functions.
 *
 * @since New in 1.4.
 */
svn_error_t *
svn_repos_replay2(svn_fs_root_t *root,
                  const char *base_dir,
                  svn_revnum_t low_water_mark,
                  svn_boolean_t send_deltas,
                  const svn_delta_editor_t *editor,
                  void *edit_baton,
                  svn_repos_authz_func_t authz_read_func,
                  void *authz_read_baton,
                  apr_pool_t *pool);

/**
 * Similar to svn_repos_replay2(), but with @a base_dir set to @c "",
 * @a low_water_mark set to #SVN_INVALID_REVNUM, @a send_deltas
 * set to @c FALSE, and @a authz_read_func and @a authz_read_baton
 * set to @c NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_replay(svn_fs_root_t *root,
                 const svn_delta_editor_t *editor,
                 void *edit_baton,
                 apr_pool_t *pool);

/* ---------------------------------------------------------------*/

/* Making commits. */

/**
 * Return an @a editor and @a edit_baton to commit changes to the
 * filesystem of @a repos, beginning at location 'rev:@a base_path',
 * where "rev" is the argument given to open_root().
 *
 * @a repos is a previously opened repository.  @a repos_url is the
 * decoded URL to the base of the repository, and is used to check
 * copyfrom paths.  @a txn is a filesystem transaction object to use
 * during the commit, or @c NULL to indicate that this function should
 * create (and fully manage) a new transaction.
 *
 * Store the contents of @a revprop_table, a hash mapping <tt>const
 * char *</tt> property names to #svn_string_t values, as properties
 * of the commit transaction, including author and log message if
 * present.
 *
 * @note #SVN_PROP_REVISION_DATE may be present in @a revprop_table, but
 * it will be overwritten when the transaction is committed.
 *
 * Iff @a authz_callback is provided, check read/write authorizations
 * on paths accessed by editor operations.  An operation which fails
 * due to authz will return SVN_ERR_AUTHZ_UNREADABLE or
 * SVN_ERR_AUTHZ_UNWRITABLE.
 *
 * Calling @a (*editor)->close_edit completes the commit.
 *
 * If @a callback is non-NULL, then before @c close_edit returns (but
 * after the commit has succeeded) @c close_edit will invoke
 * @a callback with a filled-in #svn_commit_info_t *, @a callback_baton,
 * and @a pool or some subpool thereof as arguments.  If @a callback
 * returns an error, that error will be returned from @c close_edit,
 * otherwise if there was a post-commit hook failure, then that error
 * will be returned with code SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED.
 * (Note that prior to Subversion 1.6, @a callback cannot be NULL; if
 * you don't need a callback, pass a dummy function.)
 *
 * Calling @a (*editor)->abort_edit aborts the commit, and will also
 * abort the commit transaction unless @a txn was supplied (not @c
 * NULL).  Callers who supply their own transactions are responsible
 * for cleaning them up (either by committing them, or aborting them).
 *
 * @since New in 1.5.
 *
 * @note Yes, @a repos_url is a <em>decoded</em> URL.  We realize
 * that's sorta wonky.  Sorry about that.
 */
svn_error_t *
svn_repos_get_commit_editor5(const svn_delta_editor_t **editor,
                             void **edit_baton,
                             svn_repos_t *repos,
                             svn_fs_txn_t *txn,
                             const char *repos_url,
                             const char *base_path,
                             apr_hash_t *revprop_table,
                             svn_commit_callback2_t callback,
                             void *callback_baton,
                             svn_repos_authz_callback_t authz_callback,
                             void *authz_baton,
                             apr_pool_t *pool);

/**
 * Similar to svn_repos_get_commit_editor5(), but with @a revprop_table
 * set to a hash containing @a user and @a log_msg as the
 * #SVN_PROP_REVISION_AUTHOR and #SVN_PROP_REVISION_LOG properties,
 * respectively.  @a user and @a log_msg may both be @c NULL.
 *
 * @since New in 1.4.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_get_commit_editor4(const svn_delta_editor_t **editor,
                             void **edit_baton,
                             svn_repos_t *repos,
                             svn_fs_txn_t *txn,
                             const char *repos_url,
                             const char *base_path,
                             const char *user,
                             const char *log_msg,
                             svn_commit_callback2_t callback,
                             void *callback_baton,
                             svn_repos_authz_callback_t authz_callback,
                             void *authz_baton,
                             apr_pool_t *pool);

/**
 * Similar to svn_repos_get_commit_editor4(), but
 * uses the svn_commit_callback_t type.
 *
 * @since New in 1.3.
 *
 * @deprecated Provided for backward compatibility with the 1.3 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_get_commit_editor3(const svn_delta_editor_t **editor,
                             void **edit_baton,
                             svn_repos_t *repos,
                             svn_fs_txn_t *txn,
                             const char *repos_url,
                             const char *base_path,
                             const char *user,
                             const char *log_msg,
                             svn_commit_callback_t callback,
                             void *callback_baton,
                             svn_repos_authz_callback_t authz_callback,
                             void *authz_baton,
                             apr_pool_t *pool);

/**
 * Similar to svn_repos_get_commit_editor3(), but with @a
 * authz_callback and @a authz_baton set to @c NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_get_commit_editor2(const svn_delta_editor_t **editor,
                             void **edit_baton,
                             svn_repos_t *repos,
                             svn_fs_txn_t *txn,
                             const char *repos_url,
                             const char *base_path,
                             const char *user,
                             const char *log_msg,
                             svn_commit_callback_t callback,
                             void *callback_baton,
                             apr_pool_t *pool);


/**
 * Similar to svn_repos_get_commit_editor2(), but with @a txn always
 * set to @c NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_get_commit_editor(const svn_delta_editor_t **editor,
                            void **edit_baton,
                            svn_repos_t *repos,
                            const char *repos_url,
                            const char *base_path,
                            const char *user,
                            const char *log_msg,
                            svn_commit_callback_t callback,
                            void *callback_baton,
                            apr_pool_t *pool);

/* ---------------------------------------------------------------*/

/* Finding particular revisions. */

/** Set @a *revision to the revision number in @a repos's filesystem that was
 * youngest at time @a tm.
 */
svn_error_t *
svn_repos_dated_revision(svn_revnum_t *revision,
                         svn_repos_t *repos,
                         apr_time_t tm,
                         apr_pool_t *pool);


/** Given a @a root/@a path within some filesystem, return three pieces of
 * information allocated in @a pool:
 *
 *    - set @a *committed_rev to the revision in which the object was
 *      last modified.  (In fs parlance, this is the revision in which
 *      the particular node-rev-id was 'created'.)
 *
 *    - set @a *committed_date to the date of said revision, or @c NULL
 *      if not available.
 *
 *    - set @a *last_author to the author of said revision, or @c NULL
 *      if not available.
 */
svn_error_t *
svn_repos_get_committed_info(svn_revnum_t *committed_rev,
                             const char **committed_date,
                             const char **last_author,
                             svn_fs_root_t *root,
                             const char *path,
                             apr_pool_t *pool);


/**
 * Set @a *dirent to an #svn_dirent_t associated with @a path in @a
 * root.  If @a path does not exist in @a root, set @a *dirent to
 * NULL.  Use @a pool for memory allocation.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_repos_stat(svn_dirent_t **dirent,
               svn_fs_root_t *root,
               const char *path,
               apr_pool_t *pool);


/**
 * Given @a path which exists at revision @a start in @a fs, set
 * @a *deleted to the revision @a path was first deleted, within the
 * inclusive revision range bounded by @a start and @a end.  If @a path
 * does not exist at revision @a start or was not deleted within the
 * specified range, then set @a *deleted to SVN_INVALID_REVNUM.
 * Use @a pool for memory allocation.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_deleted_rev(svn_fs_t *fs,
                      const char *path,
                      svn_revnum_t start,
                      svn_revnum_t end,
                      svn_revnum_t *deleted,
                      apr_pool_t *pool);


/** Callback type for use with svn_repos_history().  @a path and @a
 * revision represent interesting history locations in the lifetime
 * of the path passed to svn_repos_history().  @a baton is the same
 * baton given to svn_repos_history().  @a pool is provided for the
 * convenience of the implementor, who should not expect it to live
 * longer than a single callback call.
 *
 * Signal to callback driver to stop processing/invoking this callback
 * by returning the #SVN_ERR_CEASE_INVOCATION error code.
 *
 * @note SVN_ERR_CEASE_INVOCATION is new in 1.5.
 */
typedef svn_error_t *(*svn_repos_history_func_t)(void *baton,
                                                 const char *path,
                                                 svn_revnum_t revision,
                                                 apr_pool_t *pool);

/**
 * Call @a history_func (with @a history_baton) for each interesting
 * history location in the lifetime of @a path in @a fs, from the
 * youngest of @a end and @a start to the oldest.  Stop processing if
 * @a history_func returns #SVN_ERR_CEASE_INVOCATION.  Only cross
 * filesystem copy history if @a cross_copies is @c TRUE.  And do all
 * of this in @a pool.
 *
 * If @a authz_read_func is non-NULL, then use it (and @a
 * authz_read_baton) to verify that @a path in @a end is readable; if
 * not, return SVN_ERR_AUTHZ_UNREADABLE.  Also verify the readability
 * of every ancestral path/revision pair before pushing them at @a
 * history_func.  If a pair is deemed unreadable, then do not send
 * them; instead, immediately stop traversing history and return
 * SVN_NO_ERROR.
 *
 * @since New in 1.1.
 *
 * @note SVN_ERR_CEASE_INVOCATION is new in 1.5.
 */
svn_error_t *
svn_repos_history2(svn_fs_t *fs,
                   const char *path,
                   svn_repos_history_func_t history_func,
                   void *history_baton,
                   svn_repos_authz_func_t authz_read_func,
                   void *authz_read_baton,
                   svn_revnum_t start,
                   svn_revnum_t end,
                   svn_boolean_t cross_copies,
                   apr_pool_t *pool);

/**
 * Similar to svn_repos_history2(), but with @a authz_read_func
 * and @a authz_read_baton always set to NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_history(svn_fs_t *fs,
                  const char *path,
                  svn_repos_history_func_t history_func,
                  void *history_baton,
                  svn_revnum_t start,
                  svn_revnum_t end,
                  svn_boolean_t cross_copies,
                  apr_pool_t *pool);


/**
 * Set @a *locations to be a mapping of the revisions to the paths of
 * the file @a fs_path present at the repository in revision
 * @a peg_revision, where the revisions are taken out of the array
 * @a location_revisions.
 *
 * @a location_revisions is an array of svn_revnum_t's and @a *locations
 * maps 'svn_revnum_t *' to 'const char *'.
 *
 * If optional @a authz_read_func is non-NULL, then use it (and @a
 * authz_read_baton) to verify that the peg-object is readable.  If not,
 * return SVN_ERR_AUTHZ_UNREADABLE.  Also use the @a authz_read_func
 * to check that every path returned in the hash is readable.  If an
 * unreadable path is encountered, stop tracing and return
 * SVN_NO_ERROR.
 *
 * @a pool is used for all allocations.
 *
 * @since New in 1.1.
 */
svn_error_t *
svn_repos_trace_node_locations(svn_fs_t *fs,
                               apr_hash_t **locations,
                               const char *fs_path,
                               svn_revnum_t peg_revision,
                               const apr_array_header_t *location_revisions,
                               svn_repos_authz_func_t authz_read_func,
                               void *authz_read_baton,
                               apr_pool_t *pool);


/**
 * Call @a receiver and @a receiver_baton to report successive
 * location segments in revisions between @a start_rev and @a end_rev
 * (inclusive) for the line of history identified by the peg-object @a
 * path in @a peg_revision (and in @a repos).
 *
 * @a end_rev may be #SVN_INVALID_REVNUM to indicate that you want
 * to trace the history of the object to its origin.
 *
 * @a start_rev may be #SVN_INVALID_REVNUM to indicate "the HEAD
 * revision".  Otherwise, @a start_rev must be younger than @a end_rev
 * (unless @a end_rev is #SVN_INVALID_REVNUM).
 *
 * @a peg_revision may be #SVN_INVALID_REVNUM to indicate "the HEAD
 * revision", and must evaluate to be at least as young as @a start_rev.
 *
 * If optional @a authz_read_func is not @c NULL, then use it (and @a
 * authz_read_baton) to verify that the peg-object is readable.  If
 * not, return #SVN_ERR_AUTHZ_UNREADABLE.  Also use the @a
 * authz_read_func to check that every path reported in a location
 * segment is readable.  If an unreadable path is encountered, report
 * a final (possibly truncated) location segment (if any), stop
 * tracing history, and return #SVN_NO_ERROR.
 *
 * @a pool is used for all allocations.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_node_location_segments(svn_repos_t *repos,
                                 const char *path,
                                 svn_revnum_t peg_revision,
                                 svn_revnum_t start_rev,
                                 svn_revnum_t end_rev,
                                 svn_location_segment_receiver_t receiver,
                                 void *receiver_baton,
                                 svn_repos_authz_func_t authz_read_func,
                                 void *authz_read_baton,
                                 apr_pool_t *pool);


/* ### other queries we can do someday --

     * fetch the last revision created by <user>
         (once usernames become revision properties!)
     * fetch the last revision where <path> was modified

*/



/* ---------------------------------------------------------------*/

/* Retrieving log messages. */


/**
 * Invoke @a receiver with @a receiver_baton on each log message from
 * @a start to @a end in @a repos's filesystem.  @a start may be greater
 * or less than @a end; this just controls whether the log messages are
 * processed in descending or ascending revision number order.
 *
 * If @a start or @a end is #SVN_INVALID_REVNUM, it defaults to youngest.
 *
 * If @a paths is non-NULL and has one or more elements, then only show
 * revisions in which at least one of @a paths was changed (i.e., if
 * file, text or props changed; if dir, props or entries changed or any node
 * changed below it).  Each path is a <tt>const char *</tt> representing
 * an absolute path in the repository.  If @a paths is NULL or empty,
 * show all revisions regardless of what paths were changed in those
 * revisions.
 *
 * If @a limit is non-zero then only invoke @a receiver on the first
 * @a limit logs.
 *
 * If @a discover_changed_paths, then each call to @a receiver passes a
 * hash mapping paths committed in that revision to information about them
 * as the receiver's @a changed_paths argument.
 * Otherwise, each call to @a receiver passes NULL for @a changed_paths.
 *
 * If @a strict_node_history is set, copy history (if any exists) will
 * not be traversed while harvesting revision logs for each path.
 *
 * If @a include_merged_revisions is set, log information for revisions
 * which have been merged to @a paths will also be returned, unless these
 * revisions are already part of @a start to @a end in @a repos's
 * filesystem, as limited by @a paths. In the latter case those revisions
 * are skipped and @a receiver is not invoked.
 *
 * If @a revprops is NULL, retrieve all revprops; else, retrieve only the
 * revprops named in the array (i.e. retrieve none if the array is empty).
 *
 * If any invocation of @a receiver returns error, return that error
 * immediately and without wrapping it.
 *
 * If @a start or @a end is a non-existent revision, return the error
 * #SVN_ERR_FS_NO_SUCH_REVISION, without ever invoking @a receiver.
 *
 * If optional @a authz_read_func is non-NULL, then use this function
 * (along with optional @a authz_read_baton) to check the readability
 * of each changed-path in each revision about to be "pushed" at
 * @a receiver.  If a revision has some changed-paths readable and
 * others unreadable, unreadable paths are omitted from the
 * changed_paths field and only svn:author and svn:date will be
 * available in the revprops field.  If a revision has no
 * changed-paths readable at all, then all paths are omitted and no
 * revprops are available.
 *
 * See also the documentation for #svn_log_entry_receiver_t.
 *
 * Use @a pool for temporary allocations.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_get_logs4(svn_repos_t *repos,
                    const apr_array_header_t *paths,
                    svn_revnum_t start,
                    svn_revnum_t end,
                    int limit,
                    svn_boolean_t discover_changed_paths,
                    svn_boolean_t strict_node_history,
                    svn_boolean_t include_merged_revisions,
                    const apr_array_header_t *revprops,
                    svn_repos_authz_func_t authz_read_func,
                    void *authz_read_baton,
                    svn_log_entry_receiver_t receiver,
                    void *receiver_baton,
                    apr_pool_t *pool);

/**
 * Same as svn_repos_get_logs4(), but with @a receiver being
 * #svn_log_message_receiver_t instead of #svn_log_entry_receiver_t.
 * Also, @a include_merged_revisions is set to @c FALSE and @a revprops is
 * svn:author, svn:date, and svn:log.  If @a paths is empty, nothing
 * is returned.
 *
 * @since New in 1.2.
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_get_logs3(svn_repos_t *repos,
                    const apr_array_header_t *paths,
                    svn_revnum_t start,
                    svn_revnum_t end,
                    int limit,
                    svn_boolean_t discover_changed_paths,
                    svn_boolean_t strict_node_history,
                    svn_repos_authz_func_t authz_read_func,
                    void *authz_read_baton,
                    svn_log_message_receiver_t receiver,
                    void *receiver_baton,
                    apr_pool_t *pool);


/**
 * Same as svn_repos_get_logs3(), but with @a limit always set to 0.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_get_logs2(svn_repos_t *repos,
                    const apr_array_header_t *paths,
                    svn_revnum_t start,
                    svn_revnum_t end,
                    svn_boolean_t discover_changed_paths,
                    svn_boolean_t strict_node_history,
                    svn_repos_authz_func_t authz_read_func,
                    void *authz_read_baton,
                    svn_log_message_receiver_t receiver,
                    void *receiver_baton,
                    apr_pool_t *pool);

/**
 * Same as svn_repos_get_logs2(), but with @a authz_read_func and
 * @a authz_read_baton always set to NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_get_logs(svn_repos_t *repos,
                   const apr_array_header_t *paths,
                   svn_revnum_t start,
                   svn_revnum_t end,
                   svn_boolean_t discover_changed_paths,
                   svn_boolean_t strict_node_history,
                   svn_log_message_receiver_t receiver,
                   void *receiver_baton,
                   apr_pool_t *pool);



/* ---------------------------------------------------------------*/

/* Retrieving mergeinfo. */

/**
 * Fetch the mergeinfo for @a paths at @a revision in @a repos, and
 * set @a *catalog to a catalog of this mergeinfo.  @a *catalog will
 * never be @c NULL but may be empty.
 *
 * @a inherit indicates whether explicit, explicit or inherited, or
 * only inherited mergeinfo for @a paths is fetched.
 *
 * If @a revision is #SVN_INVALID_REVNUM, it defaults to youngest.
 *
 * If @a include_descendants is TRUE, then additionally return the
 * mergeinfo for any descendant of any element of @a paths which has
 * the #SVN_PROP_MERGEINFO property explicitly set on it.  (Note
 * that inheritance is only taken into account for the elements in @a
 * paths; descendants of the elements in @a paths which get their
 * mergeinfo via inheritance are not included in @a *catalog.)
 *
 * If optional @a authz_read_func is non-NULL, then use this function
 * (along with optional @a authz_read_baton) to check the readability
 * of each path which mergeinfo was requested for (from @a paths).
 * Silently omit unreadable paths from the request for mergeinfo.
 *
 * Use @a pool for all allocations.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_fs_get_mergeinfo(svn_mergeinfo_catalog_t *catalog,
                           svn_repos_t *repos,
                           const apr_array_header_t *paths,
                           svn_revnum_t revision,
                           svn_mergeinfo_inheritance_t inherit,
                           svn_boolean_t include_descendants,
                           svn_repos_authz_func_t authz_read_func,
                           void *authz_read_baton,
                           apr_pool_t *pool);


/* ---------------------------------------------------------------*/

/* Retrieving multiple revisions of a file. */

/**
 * Retrieve a subset of the interesting revisions of a file @a path in
 * @a repos as seen in revision @a end.  Invoke @a handler with
 * @a handler_baton as its first argument for each such revision.
 * @a pool is used for all allocations.  See svn_fs_history_prev() for
 * a discussion of interesting revisions.
 *
 * If optional @a authz_read_func is non-NULL, then use this function
 * (along with optional @a authz_read_baton) to check the readability
 * of the rev-path in each interesting revision encountered.
 *
 * Revision discovery happens from @a end to @a start, and if an
 * unreadable revision is encountered before @a start is reached, then
 * revision discovery stops and only the revisions from @a end to the
 * oldest readable revision are returned (So it will appear that @a
 * path was added without history in the latter revision).
 *
 * If there is an interesting revision of the file that is less than or
 * equal to start, the iteration will start at that revision.  Else, the
 * iteration will start at the first revision of the file in the repository,
 * which has to be less than or equal to end.  Note that if the function
 * succeeds, @a handler will have been called at least once.
 *
 * In a series of calls, the file contents for the first interesting revision
 * will be provided as a text delta against the empty file.  In the following
 * calls, the delta will be against the contents for the previous call.
 *
 * If @a include_merged_revisions is TRUE, revisions which a included as a
 * result of a merge between @a start and @a end will be included.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_get_file_revs2(svn_repos_t *repos,
                         const char *path,
                         svn_revnum_t start,
                         svn_revnum_t end,
                         svn_boolean_t include_merged_revisions,
                         svn_repos_authz_func_t authz_read_func,
                         void *authz_read_baton,
                         svn_file_rev_handler_t handler,
                         void *handler_baton,
                         apr_pool_t *pool);

/**
 * Similar to svn_repos_get_file_revs2(), with @a include_merged_revisions
 * set to FALSE.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 * @since New in 1.1.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_get_file_revs(svn_repos_t *repos,
                        const char *path,
                        svn_revnum_t start,
                        svn_revnum_t end,
                        svn_repos_authz_func_t authz_read_func,
                        void *authz_read_baton,
                        svn_repos_file_rev_handler_t handler,
                        void *handler_baton,
                        apr_pool_t *pool);


/* ---------------------------------------------------------------*/

/**
 * @defgroup svn_repos_hook_wrappers Hook-sensitive wrappers for libsvn_fs \
 * routines.
 * @{
 */

/** Like svn_fs_commit_txn(), but invoke the @a repos' pre- and
 * post-commit hooks around the commit.  Use @a pool for any necessary
 * allocations.
 *
 * If the pre-commit hook fails, do not attempt to commit the
 * transaction and throw the original error to the caller.
 *
 * A successful commit is indicated by a valid revision value in @a
 * *new_rev, not if svn_fs_commit_txn() returns an error, which can
 * occur during its post commit FS processing.  If the transaction was
 * not committed, then return the associated error and do not execute
 * the post-commit hook.
 *
 * If the commit succeeds the post-commit hook is executed.  If the
 * post-commit hook returns an error, always wrap it with
 * SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED; this allows the caller to
 * find the post-commit hook error in the returned error chain.  If
 * both svn_fs_commit_txn() and the post-commit hook return errors,
 * then svn_fs_commit_txn()'s error is the parent error and the
 * SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED wrapped error is the child
 * error.
 *
 * @a conflict_p, @a new_rev, and @a txn are as in svn_fs_commit_txn().
 */
svn_error_t *
svn_repos_fs_commit_txn(const char **conflict_p,
                        svn_repos_t *repos,
                        svn_revnum_t *new_rev,
                        svn_fs_txn_t *txn,
                        apr_pool_t *pool);

/** Like svn_fs_begin_txn(), but use @a revprop_table, a hash mapping
 * <tt>const char *</tt> property names to #svn_string_t values, to
 * set the properties on transaction @a *txn_p.  @a repos is the
 * repository object which contains the filesystem.  @a rev, @a
 * *txn_p, and @a pool are as in svn_fs_begin_txn().
 *
 * Before a txn is created, the repository's start-commit hooks are
 * run; if any of them fail, no txn is created, @a *txn_p is unaffected,
 * and #SVN_ERR_REPOS_HOOK_FAILURE is returned.
 *
 * @note @a revprop_table may contain an #SVN_PROP_REVISION_DATE property,
 * which will be set on the transaction, but that will be overwritten
 * when the transaction is committed.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_fs_begin_txn_for_commit2(svn_fs_txn_t **txn_p,
                                   svn_repos_t *repos,
                                   svn_revnum_t rev,
                                   apr_hash_t *revprop_table,
                                   apr_pool_t *pool);


/**
 * Same as svn_repos_fs_begin_txn_for_commit2(), but with @a revprop_table
 * set to a hash containing @a author and @a log_msg as the
 * #SVN_PROP_REVISION_AUTHOR and #SVN_PROP_REVISION_LOG properties,
 * respectively.  @a author and @a log_msg may both be @c NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_fs_begin_txn_for_commit(svn_fs_txn_t **txn_p,
                                  svn_repos_t *repos,
                                  svn_revnum_t rev,
                                  const char *author,
                                  const char *log_msg,
                                  apr_pool_t *pool);


/** Like svn_fs_begin_txn(), but use @a author to set the corresponding
 * property on transaction @a *txn_p.  @a repos is the repository object
 * which contains the filesystem.  @a rev, @a *txn_p, and @a pool are as in
 * svn_fs_begin_txn().
 *
 * ### Someday: before a txn is created, some kind of read-hook could
 *              be called here.
 */
svn_error_t *
svn_repos_fs_begin_txn_for_update(svn_fs_txn_t **txn_p,
                                  svn_repos_t *repos,
                                  svn_revnum_t rev,
                                  const char *author,
                                  apr_pool_t *pool);


/** @defgroup svn_repos_fs_locks Repository lock wrappers
 * @{
 */

/** Like svn_fs_lock(), but invoke the @a repos's pre- and
 * post-lock hooks before and after the locking action.  Use @a pool
 * for any necessary allocations.
 *
 * If the pre-lock hook or svn_fs_lock() fails, throw the original
 * error to caller.  If an error occurs when running the post-lock
 * hook, return the original error wrapped with
 * SVN_ERR_REPOS_POST_LOCK_HOOK_FAILED.  If the caller sees this
 * error, it knows that the lock succeeded anyway.
 *
 * The pre-lock hook may cause a different token to be used for the
 * lock, instead of @a token; see the pre-lock-hook documentation for
 * more.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_repos_fs_lock(svn_lock_t **lock,
                  svn_repos_t *repos,
                  const char *path,
                  const char *token,
                  const char *comment,
                  svn_boolean_t is_dav_comment,
                  apr_time_t expiration_date,
                  svn_revnum_t current_rev,
                  svn_boolean_t steal_lock,
                  apr_pool_t *pool);


/** Like svn_fs_unlock(), but invoke the @a repos's pre- and
 * post-unlock hooks before and after the unlocking action.  Use @a
 * pool for any necessary allocations.
 *
 * If the pre-unlock hook or svn_fs_unlock() fails, throw the original
 * error to caller.  If an error occurs when running the post-unlock
 * hook, return the original error wrapped with
 * SVN_ERR_REPOS_POST_UNLOCK_HOOK_FAILED.  If the caller sees this
 * error, it knows that the unlock succeeded anyway.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_repos_fs_unlock(svn_repos_t *repos,
                    const char *path,
                    const char *token,
                    svn_boolean_t break_lock,
                    apr_pool_t *pool);



/** Look up all the locks in and under @a path in @a repos, setting @a
 * *locks to a hash which maps <tt>const char *</tt> paths to the
 * #svn_lock_t locks associated with those paths.  Use @a
 * authz_read_func and @a authz_read_baton to "screen" all returned
 * locks.  That is: do not return any locks on any paths that are
 * unreadable in HEAD, just silently omit them.
 *
 * @a depth limits the returned locks to those associated with paths
 * within the specified depth of @a path, and must be one of the
 * following values:  #svn_depth_empty, #svn_depth_files,
 * #svn_depth_immediates, or #svn_depth_infinity.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_repos_fs_get_locks2(apr_hash_t **locks,
                        svn_repos_t *repos,
                        const char *path,
                        svn_depth_t depth,
                        svn_repos_authz_func_t authz_read_func,
                        void *authz_read_baton,
                        apr_pool_t *pool);

/**
 * Similar to svn_repos_fs_get_locks2(), but with @a depth always
 * passed as svn_depth_infinity.
 *
 * @since New in 1.2.
 * @deprecated Provided for backward compatibility with the 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_fs_get_locks(apr_hash_t **locks,
                       svn_repos_t *repos,
                       const char *path,
                       svn_repos_authz_func_t authz_read_func,
                       void *authz_read_baton,
                       apr_pool_t *pool);

/** @} */

/**
 * Like svn_fs_change_rev_prop2(), but validate the name and value of the
 * property and invoke the @a repos's pre- and post-revprop-change hooks
 * around the change as specified by @a use_pre_revprop_change_hook and
 * @a use_post_revprop_change_hook (respectively).
 *
 * @a rev is the revision whose property to change, @a name is the
 * name of the property, and @a new_value is the new value of the
 * property.   If @a old_value_p is not @c NULL, then @a *old_value_p
 * is the expected current (preexisting) value of the property (or @c NULL
 * for "unset").  @a author is the authenticated username of the person
 * changing the property value, or NULL if not available.
 *
 * If @a authz_read_func is non-NULL, then use it (with @a
 * authz_read_baton) to validate the changed-paths associated with @a
 * rev.  If the revision contains any unreadable changed paths, then
 * return #SVN_ERR_AUTHZ_UNREADABLE.
 *
 * Validate @a name and @a new_value like the same way
 * svn_repos_fs_change_node_prop() does.
 *
 * Use @a pool for temporary allocations.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_repos_fs_change_rev_prop4(svn_repos_t *repos,
                              svn_revnum_t rev,
                              const char *author,
                              const char *name,
                              const svn_string_t *const *old_value_p,
                              const svn_string_t *new_value,
                              svn_boolean_t
                              use_pre_revprop_change_hook,
                              svn_boolean_t
                              use_post_revprop_change_hook,
                              svn_repos_authz_func_t
                              authz_read_func,
                              void *authz_read_baton,
                              apr_pool_t *pool);

/**
 * Similar to svn_repos_fs_change_rev_prop4(), but with @a old_value_p always
 * set to @c NULL.  (In other words, it is similar to
 * svn_fs_change_rev_prop().)
 *
 * @deprecated Provided for backward compatibility with the 1.6 API.
 * @since New in 1.5.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_fs_change_rev_prop3(svn_repos_t *repos,
                              svn_revnum_t rev,
                              const char *author,
                              const char *name,
                              const svn_string_t *new_value,
                              svn_boolean_t
                              use_pre_revprop_change_hook,
                              svn_boolean_t
                              use_post_revprop_change_hook,
                              svn_repos_authz_func_t
                              authz_read_func,
                              void *authz_read_baton,
                              apr_pool_t *pool);

/**
 * Similar to svn_repos_fs_change_rev_prop3(), but with the @a
 * use_pre_revprop_change_hook and @a use_post_revprop_change_hook
 * always set to @c TRUE.
 *
 * @deprecated Provided for backward compatibility with the 1.4 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_fs_change_rev_prop2(svn_repos_t *repos,
                              svn_revnum_t rev,
                              const char *author,
                              const char *name,
                              const svn_string_t *new_value,
                              svn_repos_authz_func_t
                              authz_read_func,
                              void *authz_read_baton,
                              apr_pool_t *pool);

/**
 * Similar to svn_repos_fs_change_rev_prop2(), but with the
 * @a authz_read_func parameter always NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_fs_change_rev_prop(svn_repos_t *repos,
                             svn_revnum_t rev,
                             const char *author,
                             const char *name,
                             const svn_string_t *new_value,
                             apr_pool_t *pool);



/**
 * Set @a *value_p to the value of the property named @a propname on
 * revision @a rev in the filesystem opened in @a repos.  If @a rev
 * has no property by that name, set @a *value_p to zero.  Allocate
 * the result in @a pool.
 *
 * If @a authz_read_func is non-NULL, then use it (with @a
 * authz_read_baton) to validate the changed-paths associated with @a
 * rev.  If the changed-paths are all unreadable, then set @a *value_p
 * to zero unconditionally.  If only some of the changed-paths are
 * unreadable, then allow 'svn:author' and 'svn:date' propvalues to be
 * fetched, but return 0 for any other property.
 *
 * @since New in 1.1.
 */
svn_error_t *
svn_repos_fs_revision_prop(svn_string_t **value_p,
                           svn_repos_t *repos,
                           svn_revnum_t rev,
                           const char *propname,
                           svn_repos_authz_func_t
                           authz_read_func,
                           void *authz_read_baton,
                           apr_pool_t *pool);


/**
 * Set @a *table_p to the entire property list of revision @a rev in
 * filesystem opened in @a repos, as a hash table allocated in @a
 * pool.  The table maps <tt>char *</tt> property names to
 * #svn_string_t * values; the names and values are allocated in @a
 * pool.
 *
 * If @a authz_read_func is non-NULL, then use it (with @a
 * authz_read_baton) to validate the changed-paths associated with @a
 * rev.  If the changed-paths are all unreadable, then return an empty
 * hash. If only some of the changed-paths are unreadable, then return
 * an empty hash, except for 'svn:author' and 'svn:date' properties
 * (assuming those properties exist).
 *
 * @since New in 1.1.
 */
svn_error_t *
svn_repos_fs_revision_proplist(apr_hash_t **table_p,
                               svn_repos_t *repos,
                               svn_revnum_t rev,
                               svn_repos_authz_func_t
                               authz_read_func,
                               void *authz_read_baton,
                               apr_pool_t *pool);



/* ---------------------------------------------------------------*/

/* Prop-changing wrappers for libsvn_fs routines. */

/* NOTE: svn_repos_fs_change_rev_prop() also exists, but is located
   above with the hook-related functions. */


/** Validating wrapper for svn_fs_change_node_prop() (which see for
 * argument descriptions).
 *
 * If @a name's kind is not #svn_prop_regular_kind, return
 * #SVN_ERR_REPOS_BAD_ARGS.  If @a name is an "svn:" property, validate its
 * @a value and return SVN_ERR_BAD_PROPERTY_VALUE if it is invalid for the
 * property.
 *
 * @note Currently, the only properties validated are the "svn:" properties
 * #SVN_PROP_REVISION_LOG and #SVN_PROP_REVISION_DATE. This may change
 * in future releases.
 */
svn_error_t *
svn_repos_fs_change_node_prop(svn_fs_root_t *root,
                              const char *path,
                              const char *name,
                              const svn_string_t *value,
                              apr_pool_t *pool);

/** Validating wrapper for svn_fs_change_txn_prop() (which see for
 * argument descriptions).  See svn_repos_fs_change_txn_props() for more
 * information.
 */
svn_error_t *
svn_repos_fs_change_txn_prop(svn_fs_txn_t *txn,
                             const char *name,
                             const svn_string_t *value,
                             apr_pool_t *pool);

/** Validating wrapper for svn_fs_change_txn_props() (which see for
 * argument descriptions).  Validate properties and their values the
 * same way svn_repos_fs_change_node_prop() does.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_fs_change_txn_props(svn_fs_txn_t *txn,
                              const apr_array_header_t *props,
                              apr_pool_t *pool);

/** @} */

/* ---------------------------------------------------------------*/

/**
 * @defgroup svn_repos_inspection Data structures and editor things for \
 * repository inspection.
 * @{
 *
 * As it turns out, the svn_repos_replay2(), svn_repos_dir_delta2() and
 * svn_repos_begin_report2() interfaces can be extremely useful for
 * examining the repository, or more exactly, changes to the repository.
 * These drivers allows for differences between two trees to be
 * described using an editor.
 *
 * By using the editor obtained from svn_repos_node_editor() with one of
 * the drivers mentioned above, the description of how to transform one
 * tree into another can be used to build an in-memory linked-list tree,
 * which each node representing a repository node that was changed.
 */

/** A node in the repository. */
typedef struct svn_repos_node_t
{
  /** Node type (file, dir, etc.) */
  svn_node_kind_t kind;

  /** How this node entered the node tree: 'A'dd, 'D'elete, 'R'eplace */
  char action;

  /** Were there any textual mods? (files only) */
  svn_boolean_t text_mod;

  /** Where there any property mods? */
  svn_boolean_t prop_mod;

  /** The name of this node as it appears in its parent's entries list */
  const char *name;

  /** The filesystem revision where this was copied from (if any) */
  svn_revnum_t copyfrom_rev;

  /** The filesystem path where this was copied from (if any) */
  const char *copyfrom_path;

  /** Pointer to the next sibling of this node */
  struct svn_repos_node_t *sibling;

  /** Pointer to the first child of this node */
  struct svn_repos_node_t *child;

  /** Pointer to the parent of this node */
  struct svn_repos_node_t *parent;

} svn_repos_node_t;


/** Set @a *editor and @a *edit_baton to an editor that, when driven by
 * a driver such as svn_repos_replay2(), builds an <tt>svn_repos_node_t *</tt>
 * tree representing the delta from @a base_root to @a root in @a
 * repos's filesystem.
 *
 * The editor can also be driven by svn_repos_dir_delta2() or
 * svn_repos_begin_report2(), but unless you have special needs,
 * svn_repos_replay2() is preferred.
 *
 * Invoke svn_repos_node_from_baton() on @a edit_baton to obtain the root
 * node afterwards.
 *
 * Note that the delta includes "bubbled-up" directories; that is,
 * many of the directory nodes will have no prop_mods.
 *
 * Allocate the tree and its contents in @a node_pool; do all other
 * allocation in @a pool.
 */
svn_error_t *
svn_repos_node_editor(const svn_delta_editor_t **editor,
                      void **edit_baton,
                      svn_repos_t *repos,
                      svn_fs_root_t *base_root,
                      svn_fs_root_t *root,
                      apr_pool_t *node_pool,
                      apr_pool_t *pool);

/** Return the root node of the linked-list tree generated by driving the
 * editor (associated with @a edit_baton) created by svn_repos_node_editor().
 * This is only really useful if used *after* the editor drive is completed.
 */
svn_repos_node_t *
svn_repos_node_from_baton(void *edit_baton);

/** @} */

/* ---------------------------------------------------------------*/

/**
 * @defgroup svn_repos_dump_load Dumping and loading filesystem data
 * @{
 *
 * The filesystem 'dump' format contains nothing but the abstract
 * structure of the filesystem -- independent of any internal node-id
 * schema or database back-end.  All of the data in the dumpfile is
 * acquired by public function calls into svn_fs.h.  Similarly, the
 * parser which reads the dumpfile is able to reconstruct the
 * filesystem using only public svn_fs.h routines.
 *
 * Thus the dump/load feature's main purpose is for *migrating* data
 * from one svn filesystem to another -- presumably two filesystems
 * which have different internal implementations.
 *
 * If you simply want to backup your filesystem, you're probably
 * better off using the built-in facilities of the DB backend (using
 * Berkeley DB's hot-backup feature, for example.)
 *
 * For a description of the dumpfile format, see
 * /trunk/notes/fs_dumprestore.txt.
 */

/* The RFC822-style headers in our dumpfile format. */
#define SVN_REPOS_DUMPFILE_MAGIC_HEADER            "SVN-fs-dump-format-version"
#define SVN_REPOS_DUMPFILE_FORMAT_VERSION           3
#define SVN_REPOS_DUMPFILE_UUID                      "UUID"
#define SVN_REPOS_DUMPFILE_CONTENT_LENGTH            "Content-length"

#define SVN_REPOS_DUMPFILE_REVISION_NUMBER           "Revision-number"

#define SVN_REPOS_DUMPFILE_NODE_PATH                 "Node-path"
#define SVN_REPOS_DUMPFILE_NODE_KIND                 "Node-kind"
#define SVN_REPOS_DUMPFILE_NODE_ACTION               "Node-action"
#define SVN_REPOS_DUMPFILE_NODE_COPYFROM_PATH        "Node-copyfrom-path"
#define SVN_REPOS_DUMPFILE_NODE_COPYFROM_REV         "Node-copyfrom-rev"
/** @since New in 1.6. */
#define SVN_REPOS_DUMPFILE_TEXT_COPY_SOURCE_MD5      "Text-copy-source-md5"
/** @since New in 1.6. */
#define SVN_REPOS_DUMPFILE_TEXT_COPY_SOURCE_SHA1     "Text-copy-source-sha1"
#define SVN_REPOS_DUMPFILE_TEXT_COPY_SOURCE_CHECKSUM \
                                        SVN_REPOS_DUMPFILE_TEXT_COPY_SOURCE_MD5
/** @since New in 1.6. */
#define SVN_REPOS_DUMPFILE_TEXT_CONTENT_MD5          "Text-content-md5"
/** @since New in 1.6. */
#define SVN_REPOS_DUMPFILE_TEXT_CONTENT_SHA1         "Text-content-sha1"
#define SVN_REPOS_DUMPFILE_TEXT_CONTENT_CHECKSUM     \
                                        SVN_REPOS_DUMPFILE_TEXT_CONTENT_MD5

#define SVN_REPOS_DUMPFILE_PROP_CONTENT_LENGTH       "Prop-content-length"
#define SVN_REPOS_DUMPFILE_TEXT_CONTENT_LENGTH       "Text-content-length"

/** @since New in 1.1. */
#define SVN_REPOS_DUMPFILE_PROP_DELTA                "Prop-delta"
/** @since New in 1.1. */
#define SVN_REPOS_DUMPFILE_TEXT_DELTA                "Text-delta"
/** @since New in 1.6. */
#define SVN_REPOS_DUMPFILE_TEXT_DELTA_BASE_MD5       "Text-delta-base-md5"
/** @since New in 1.6. */
#define SVN_REPOS_DUMPFILE_TEXT_DELTA_BASE_SHA1      "Text-delta-base-sha1"
/** @since New in 1.5. */
#define SVN_REPOS_DUMPFILE_TEXT_DELTA_BASE_CHECKSUM  \
                                        SVN_REPOS_DUMPFILE_TEXT_DELTA_BASE_MD5

/**
 * Verify the contents of the file system in @a repos.
 *
 * If @a feedback_stream is not @c NULL, write feedback to it (lines of
 * the form "* Verified revision %ld\n").
 *
 * If @a start_rev is #SVN_INVALID_REVNUM, then start verifying at
 * revision 0.  If @a end_rev is #SVN_INVALID_REVNUM, then verify
 * through the @c HEAD revision.
 *
 * For every verified revision call @a notify_func with @a rev set to
 * the verified revision and @a warning_text @c NULL. For warnings call @a
 * notify_func with @a warning_text set.
 *
 * If @a cancel_func is not @c NULL, call it periodically with @a
 * cancel_baton as argument to see if the caller wishes to cancel the
 * verification.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_repos_verify_fs2(svn_repos_t *repos,
                     svn_revnum_t start_rev,
                     svn_revnum_t end_rev,
                     svn_repos_notify_func_t notify_func,
                     void *notify_baton,
                     svn_cancel_func_t cancel,
                     void *cancel_baton,
                     apr_pool_t *scratch_pool);

/**
 * Similar to svn_repos_verify_fs2(), but with a feedback_stream instead of
 * handling feedback via the notify_func handler
 *
 * @since New in 1.5.
 * @deprecated Provided for backward compatibility with the 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_verify_fs(svn_repos_t *repos,
                    svn_stream_t *feedback_stream,
                    svn_revnum_t start_rev,
                    svn_revnum_t end_rev,
                    svn_cancel_func_t cancel_func,
                    void *cancel_baton,
                    apr_pool_t *pool);

/**
 * Dump the contents of the filesystem within already-open @a repos into
 * writable @a dumpstream.  Begin at revision @a start_rev, and dump every
 * revision up through @a end_rev.  Use @a pool for all allocation.  If
 * non-@c NULL, send feedback to @a feedback_stream.  If @a dumpstream is
 * @c NULL, this is effectively a primitive verify.  It is not complete,
 * however; see svn_fs_verify instead.
 *
 * If @a start_rev is #SVN_INVALID_REVNUM, then start dumping at revision
 * 0.  If @a end_rev is #SVN_INVALID_REVNUM, then dump through the @c HEAD
 * revision.
 *
 * If @a incremental is @c TRUE, the first revision dumped will be a diff
 * against the previous revision (usually it looks like a full dump of
 * the tree).
 *
 * If @a use_deltas is @c TRUE, output only node properties which have
 * changed relative to the previous contents, and output text contents
 * as svndiff data against the previous contents.  Regardless of how
 * this flag is set, the first revision of a non-incremental dump will
 * be done with full plain text.  A dump with @a use_deltas set cannot
 * be loaded by Subversion 1.0.x.
 *
 * If @a notify_func is not @c NULL, then for every dumped revision call
 * @a notify_func with @a rev set to the dumped revision and @a warning_text
 * @c NULL. For warnings call @a notify_func with @a warning_text.
 *
 * If @a cancel_func is not @c NULL, it is called periodically with
 * @a cancel_baton as argument to see if the client wishes to cancel
 * the dump.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_repos_dump_fs3(svn_repos_t *repos,
                   svn_stream_t *dumpstream,
                   svn_revnum_t start_rev,
                   svn_revnum_t end_rev,
                   svn_boolean_t incremental,
                   svn_boolean_t use_deltas,
                   svn_repos_notify_func_t notify_func,
                   void *notify_baton,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   apr_pool_t *scratch_pool);

/**
 * Similar to svn_repos_dump_fs3(), but with a feedback_stream instead of
 * handling feedback via the notify_func handler
 *
 * @since New in 1.1.
 * @deprecated Provided for backward compatibility with the 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_dump_fs2(svn_repos_t *repos,
                   svn_stream_t *dumpstream,
                   svn_stream_t *feedback_stream,
                   svn_revnum_t start_rev,
                   svn_revnum_t end_rev,
                   svn_boolean_t incremental,
                   svn_boolean_t use_deltas,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   apr_pool_t *pool);

/**
 * Similar to svn_repos_dump_fs2(), but with the @a use_deltas
 * parameter always set to @c FALSE.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_dump_fs(svn_repos_t *repos,
                  svn_stream_t *dumpstream,
                  svn_stream_t *feedback_stream,
                  svn_revnum_t start_rev,
                  svn_revnum_t end_rev,
                  svn_boolean_t incremental,
                  svn_cancel_func_t cancel_func,
                  void *cancel_baton,
                  apr_pool_t *pool);


/**
 * Read and parse dumpfile-formatted @a dumpstream, reconstructing
 * filesystem revisions in already-open @a repos, handling uuids in
 * accordance with @a uuid_action.  Use @a pool for all allocation.
 *
 * If the dumpstream contains copy history that is unavailable in the
 * repository, an error will be thrown.
 *
 * The repository's UUID will be updated iff
 *   the dumpstream contains a UUID and
 *   @a uuid_action is not equal to #svn_repos_load_uuid_ignore and
 *   either the repository contains no revisions or
 *          @a uuid_action is equal to #svn_repos_load_uuid_force.
 *
 * If the dumpstream contains no UUID, then @a uuid_action is
 * ignored and the repository UUID is not touched.
 *
 * If @a parent_dir is not NULL, then the parser will reparent all the
 * loaded nodes, from root to @a parent_dir.  The directory @a parent_dir
 * must be an existing directory in the repository.
 *
 * If @a use_pre_commit_hook is set, call the repository's pre-commit
 * hook before committing each loaded revision.
 *
 * If @a use_post_commit_hook is set, call the repository's
 * post-commit hook after committing each loaded revision.
 *
 * If @a validate_props is set, then validate Subversion revision and
 * node properties (those in the svn: namespace) against established
 * rules for those things.
 *
 * If non-NULL, use @a notify_func and @a notify_baton to send notification
 * of events to the caller.
 *
 * If @a cancel_func is not @c NULL, it is called periodically with
 * @a cancel_baton as argument to see if the client wishes to cancel
 * the load.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_repos_load_fs3(svn_repos_t *repos,
                   svn_stream_t *dumpstream,
                   enum svn_repos_load_uuid uuid_action,
                   const char *parent_dir,
                   svn_boolean_t use_pre_commit_hook,
                   svn_boolean_t use_post_commit_hook,
                   svn_boolean_t validate_props,
                   svn_repos_notify_func_t notify_func,
                   void *notify_baton,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   apr_pool_t *pool);

/**
 * Similar to svn_repos_load_fs3(), but with @a feedback_stream in
 * place of the #svn_repos_notify_func_t and baton and with
 * @a validate_props always FALSE.
 *
 * @since New in 1.2.
 * @deprecated Provided for backward compatibility with the 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_load_fs2(svn_repos_t *repos,
                   svn_stream_t *dumpstream,
                   svn_stream_t *feedback_stream,
                   enum svn_repos_load_uuid uuid_action,
                   const char *parent_dir,
                   svn_boolean_t use_pre_commit_hook,
                   svn_boolean_t use_post_commit_hook,
                   svn_cancel_func_t cancel_func,
                   void *cancel_baton,
                   apr_pool_t *pool);

/**
 * Similar to svn_repos_load_fs2(), but with @a use_pre_commit_hook and
 * @a use_post_commit_hook always @c FALSE.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_load_fs(svn_repos_t *repos,
                  svn_stream_t *dumpstream,
                  svn_stream_t *feedback_stream,
                  enum svn_repos_load_uuid uuid_action,
                  const char *parent_dir,
                  svn_cancel_func_t cancel_func,
                  void *cancel_baton,
                  apr_pool_t *pool);


/**
 * A vtable that is driven by svn_repos_parse_dumpstream2().
 *
 * @since New in 1.1.
 */
typedef struct svn_repos_parse_fns2_t
{
  /** The parser has discovered a new revision record within the
   * parsing session represented by @a parse_baton.  All the headers are
   * placed in @a headers (allocated in @a pool), which maps <tt>const
   * char *</tt> header-name ==> <tt>const char *</tt> header-value.
   * The @a revision_baton received back (also allocated in @a pool)
   * represents the revision.
   */
  svn_error_t *(*new_revision_record)(void **revision_baton,
                                      apr_hash_t *headers,
                                      void *parse_baton,
                                      apr_pool_t *pool);

  /** The parser has discovered a new uuid record within the parsing
   * session represented by @a parse_baton.  The uuid's value is
   * @a uuid, and it is allocated in @a pool.
   */
  svn_error_t *(*uuid_record)(const char *uuid,
                              void *parse_baton,
                              apr_pool_t *pool);

  /** The parser has discovered a new node record within the current
   * revision represented by @a revision_baton.  All the headers are
   * placed in @a headers (as with @c new_revision_record), allocated in
   * @a pool.  The @a node_baton received back is allocated in @a pool
   * and represents the node.
   */
  svn_error_t *(*new_node_record)(void **node_baton,
                                  apr_hash_t *headers,
                                  void *revision_baton,
                                  apr_pool_t *pool);

  /** For a given @a revision_baton, set a property @a name to @a value. */
  svn_error_t *(*set_revision_property)(void *revision_baton,
                                        const char *name,
                                        const svn_string_t *value);

  /** For a given @a node_baton, set a property @a name to @a value. */
  svn_error_t *(*set_node_property)(void *node_baton,
                                    const char *name,
                                    const svn_string_t *value);

  /** For a given @a node_baton, delete property @a name. */
  svn_error_t *(*delete_node_property)(void *node_baton, const char *name);

  /** For a given @a node_baton, remove all properties. */
  svn_error_t *(*remove_node_props)(void *node_baton);

  /** For a given @a node_baton, receive a writable @a stream capable of
   * receiving the node's fulltext.  After writing the fulltext, call
   * the stream's close() function.
   *
   * If a @c NULL is returned instead of a stream, the vtable is
   * indicating that no text is desired, and the parser will not
   * attempt to send it.
   */
  svn_error_t *(*set_fulltext)(svn_stream_t **stream,
                               void *node_baton);

  /** For a given @a node_baton, set @a handler and @a handler_baton
   * to a window handler and baton capable of receiving a delta
   * against the node's previous contents.  A NULL window will be
   * sent to the handler after all the windows are sent.
   *
   * If a @c NULL is returned instead of a handler, the vtable is
   * indicating that no delta is desired, and the parser will not
   * attempt to send it.
   */
  svn_error_t *(*apply_textdelta)(svn_txdelta_window_handler_t *handler,
                                  void **handler_baton,
                                  void *node_baton);

  /** The parser has reached the end of the current node represented by
   * @a node_baton, it can be freed.
   */
  svn_error_t *(*close_node)(void *node_baton);

  /** The parser has reached the end of the current revision
   * represented by @a revision_baton.  In other words, there are no more
   * changed nodes within the revision.  The baton can be freed.
   */
  svn_error_t *(*close_revision)(void *revision_baton);

} svn_repos_parse_fns2_t;

/** @deprecated Provided for backward compatibility with the 1.2 API. */
typedef svn_repos_parse_fns2_t svn_repos_parser_fns2_t;


/**
 * Read and parse dumpfile-formatted @a stream, calling callbacks in
 * @a parse_fns/@a parse_baton, and using @a pool for allocations.
 *
 * If @a cancel_func is not @c NULL, it is called periodically with
 * @a cancel_baton as argument to see if the client wishes to cancel
 * the dump.
 *
 * This parser has built-in knowledge of the dumpfile format, but only
 * in a general sense:
 *
 *    * it recognizes revision and node records by looking for either
 *      a REVISION_NUMBER or NODE_PATH headers.
 *
 *    * it recognizes the CONTENT-LENGTH headers, so it knows if and
 *      how to suck up the content body.
 *
 *    * it knows how to parse a content body into two parts:  props
 *      and text, and pass the pieces to the vtable.
 *
 * This is enough knowledge to make it easy on vtable implementors,
 * but still allow expansion of the format:  most headers are ignored.
 *
 * @since New in 1.1.
 */
svn_error_t *
svn_repos_parse_dumpstream2(svn_stream_t *stream,
                            const svn_repos_parse_fns2_t *parse_fns,
                            void *parse_baton,
                            svn_cancel_func_t cancel_func,
                            void *cancel_baton,
                            apr_pool_t *pool);


/**
 * Set @a *parser and @a *parse_baton to a vtable parser which commits new
 * revisions to the fs in @a repos.  The constructed parser will treat
 * UUID records in a manner consistent with @a uuid_action.  Use @a pool
 * to operate on the fs.
 *
 * If @a use_history is set, then the parser will require relative
 * 'copyfrom' history to exist in the repository when it encounters
 * nodes that are added-with-history.
 *
 * If @a validate_props is set, then validate Subversion revision and
 * node properties (those in the svn: namespace) against established
 * rules for those things.
 *
 * If @a parent_dir is not NULL, then the parser will reparent all the
 * loaded nodes, from root to @a parent_dir.  The directory @a parent_dir
 * must be an existing directory in the repository.
 *
 * Print all parsing feedback to @a outstream (if non-@c NULL).
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_repos_get_fs_build_parser3(const svn_repos_parse_fns2_t **parser,
                               void **parse_baton,
                               svn_repos_t *repos,
                               svn_boolean_t use_history,
                               svn_boolean_t validate_props,
                               enum svn_repos_load_uuid uuid_action,
                               const char *parent_dir,
                               svn_repos_notify_func_t notify_func,
                               void *notify_baton,
                               apr_pool_t *pool);

/**
 * Similar to svn_repos_get_fs_build_parser3(), but with @a outstream
 * in place if a #svn_repos_notify_func_t and baton and with
 * @a validate_props always FALSE.
 *
 * @since New in 1.1.
 * @deprecated Provided for backward compatibility with the 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_get_fs_build_parser2(const svn_repos_parse_fns2_t **parser,
                               void **parse_baton,
                               svn_repos_t *repos,
                               svn_boolean_t use_history,
                               enum svn_repos_load_uuid uuid_action,
                               svn_stream_t *outstream,
                               const char *parent_dir,
                               apr_pool_t *pool);

/**
 * A vtable that is driven by svn_repos_parse_dumpstream().
 * Similar to #svn_repos_parse_fns2_t except that it lacks
 * the delete_node_property and apply_textdelta callbacks.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
typedef struct svn_repos_parse_fns_t
{
  /** Same as #svn_repos_parse_fns2_t.new_revision_record. */
  svn_error_t *(*new_revision_record)(void **revision_baton,
                                      apr_hash_t *headers,
                                      void *parse_baton,
                                      apr_pool_t *pool);
  /** Same as #svn_repos_parse_fns2_t.uuid_record. */
  svn_error_t *(*uuid_record)(const char *uuid,
                              void *parse_baton,
                              apr_pool_t *pool);
  /** Same as #svn_repos_parse_fns2_t.new_node_record. */
  svn_error_t *(*new_node_record)(void **node_baton,
                                  apr_hash_t *headers,
                                  void *revision_baton,
                                  apr_pool_t *pool);
  /** Same as #svn_repos_parse_fns2_t.set_revision_property. */
  svn_error_t *(*set_revision_property)(void *revision_baton,
                                        const char *name,
                                        const svn_string_t *value);
  /** Same as #svn_repos_parse_fns2_t.set_node_property. */
  svn_error_t *(*set_node_property)(void *node_baton,
                                    const char *name,
                                    const svn_string_t *value);
  /** Same as #svn_repos_parse_fns2_t.remove_node_props. */
  svn_error_t *(*remove_node_props)(void *node_baton);
  /** Same as #svn_repos_parse_fns2_t.set_fulltext. */
  svn_error_t *(*set_fulltext)(svn_stream_t **stream,
                               void *node_baton);
  /** Same as #svn_repos_parse_fns2_t.close_node. */
  svn_error_t *(*close_node)(void *node_baton);
  /** Same as #svn_repos_parse_fns2_t.close_revision. */
  svn_error_t *(*close_revision)(void *revision_baton);
} svn_repos_parser_fns_t;


/**
 * Similar to svn_repos_parse_dumpstream2(), but uses the more limited
 * #svn_repos_parser_fns_t vtable type.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_parse_dumpstream(svn_stream_t *stream,
                           const svn_repos_parser_fns_t *parse_fns,
                           void *parse_baton,
                           svn_cancel_func_t cancel_func,
                           void *cancel_baton,
                           apr_pool_t *pool);


/**
 * Similar to svn_repos_get_fs_build_parser2(), but yields the more
 * limited svn_repos_parser_fns_t vtable type.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_repos_get_fs_build_parser(const svn_repos_parser_fns_t **parser,
                              void **parse_baton,
                              svn_repos_t *repos,
                              svn_boolean_t use_history,
                              enum svn_repos_load_uuid uuid_action,
                              svn_stream_t *outstream,
                              const char *parent_dir,
                              apr_pool_t *pool);


/** @} */

/** A data type which stores the authz information.
 *
 * @since New in 1.3.
 */
typedef struct svn_authz_t svn_authz_t;

/** Read authz configuration data from @a file (a file or registry
 * path) into @a *authz_p, allocated in @a pool.
 *
 * If @a file is not a valid authz rule file, then return
 * SVN_AUTHZ_INVALID_CONFIG.  The contents of @a *authz_p is then
 * undefined.  If @a must_exist is TRUE, a missing authz file is also
 * an error.
 *
 * @since New in 1.3.
 */
svn_error_t *
svn_repos_authz_read(svn_authz_t **authz_p,
                     const char *file,
                     svn_boolean_t must_exist,
                     apr_pool_t *pool);

/**
 * Check whether @a user can access @a path in the repository @a
 * repos_name with the @a required_access.  @a authz lists the ACLs to
 * check against.  Set @a *access_granted to indicate if the requested
 * access is granted.
 *
 * If @a path is NULL, then check whether @a user has the @a
 * required_access anywhere in the repository.  Set @a *access_granted
 * to TRUE if at least one path is accessible with the @a
 * required_access.
 *
 * For compatibility with 1.6, and earlier, @a repos_name can be NULL
 * in which case it is equivalent to a @a repos_name of "".
 *
 * @since New in 1.3.
 */
svn_error_t *
svn_repos_authz_check_access(svn_authz_t *authz,
                             const char *repos_name,
                             const char *path,
                             const char *user,
                             svn_repos_authz_access_t required_access,
                             svn_boolean_t *access_granted,
                             apr_pool_t *pool);



/** Revision Access Levels
 *
 * Like most version control systems, access to versioned objects in
 * Subversion is determined on primarily path-based system.  Users either
 * do or don't have the ability to read a given path.
 *
 * However, unlike many version control systems where versioned objects
 * maintain their own distinct version information (revision numbers,
 * authors, log messages, change timestamps, etc.), Subversion binds
 * multiple paths changed as part of a single commit operation into a
 * set, calls the whole thing a revision, and hangs commit metadata
 * (author, date, log message, etc.) off of that revision.  So, commit
 * metadata is shared across all the paths changed as part of a given
 * commit operation.
 *
 * It is common (or, at least, we hope it is) for log messages to give
 * detailed information about changes made in the commit to which the log
 * message is attached.  Such information might include a mention of all
 * the files changed, what was changed in them, and so on.  But this
 * causes a problem when presenting information to readers who aren't
 * authorized to read every path in the repository.  Simply knowing that
 * a given path exists may be a security leak, even if the user can't see
 * the contents of the data located at that path.
 *
 * So Subversion does what it reasonably can to prevent the leak of this
 * information, and does so via a staged revision access policy.  A
 * reader can be said to have one of three levels of access to a given
 * revision's metadata, based solely on the reader's access rights to the
 * paths changed or copied in that revision:
 *
 *   'full access' -- Granted when the reader has access to all paths
 *      changed or copied in the revision, or when no paths were
 *      changed in the revision at all, this access level permits
 *      full visibility of all revision property names and values,
 *      and the full changed-paths information.
 *
 *   'no access' -- Granted when the reader does not have access to any
 *      paths changed or copied in the revision, this access level
 *      denies the reader access to all revision properties and all
 *      changed-paths information.
 *
 *   'partial access' -- Granted when the reader has access to at least
 *      one, but not all, of the paths changed or copied in the revision,
 *      this access level permits visibility of the svn:date and
 *      svn:author revision properties and only the paths of the
 *      changed-paths information to which the reader has access.
 *
 */


/** An enum defining levels of revision access.
 *
 * @since New in 1.5.
 */
typedef enum svn_repos_revision_access_level_t
{
  svn_repos_revision_access_none,
  svn_repos_revision_access_partial,
  svn_repos_revision_access_full
}
svn_repos_revision_access_level_t;


/**
 * Set @a access to the access level granted for @a revision in @a
 * repos, as determined by consulting the @a authz_read_func callback
 * function and its associated @a authz_read_baton.
 *
 * @a authz_read_func may be @c NULL, in which case @a access will be
 * set to #svn_repos_revision_access_full.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_check_revision_access(svn_repos_revision_access_level_t *access_level,
                                svn_repos_t *repos,
                                svn_revnum_t revision,
                                svn_repos_authz_func_t authz_read_func,
                                void *authz_read_baton,
                                apr_pool_t *pool);



/** Capabilities **/

/**
 * Store in @a repos the client-reported capabilities @a capabilities,
 * which must be allocated in memory at least as long-lived as @a repos.
 *
 * The elements of @a capabilities are 'const char *', a subset of
 * the constants beginning with @c SVN_RA_CAPABILITY_.
 * @a capabilities is not copied, so changing it later will affect
 * what is remembered by @a repos.
 *
 * @note The capabilities are passed along to the start-commit hook;
 * see that hook's template for details.
 *
 * @note As of Subversion 1.5, there are no error conditions defined,
 * so this always returns SVN_NO_ERROR.  In future releases it may
 * return error, however, so callers should check.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_repos_remember_client_capabilities(svn_repos_t *repos,
                                       const apr_array_header_t *capabilities);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_REPOS_H */
