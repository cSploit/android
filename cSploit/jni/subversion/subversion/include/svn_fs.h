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
 * @file svn_fs.h
 * @brief Interface to the Subversion filesystem.
 */

#ifndef SVN_FS_H
#define SVN_FS_H

#include <apr.h>
#include <apr_pools.h>
#include <apr_hash.h>
#include <apr_tables.h>
#include <apr_time.h>    /* for apr_time_t */

#include "svn_types.h"
#include "svn_string.h"
#include "svn_delta.h"
#include "svn_io.h"
#include "svn_mergeinfo.h"
#include "svn_checksum.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * Get libsvn_fs version information.
 *
 * @since New in 1.1.
 */
const svn_version_t *
svn_fs_version(void);

/**
 * @defgroup fs_handling Filesystem interaction subsystem
 * @{
 */

/* Opening and creating filesystems.  */


/** An object representing a Subversion filesystem.  */
typedef struct svn_fs_t svn_fs_t;


/**
 * @name Filesystem configuration options
 * @{
 */
#define SVN_FS_CONFIG_BDB_TXN_NOSYNC            "bdb-txn-nosync"
#define SVN_FS_CONFIG_BDB_LOG_AUTOREMOVE        "bdb-log-autoremove"

/** Enable / disable text delta caching for a FSFS repository.
 *
 * @since New in 1.7.
 */
#define SVN_FS_CONFIG_FSFS_CACHE_DELTAS         "fsfs-cache-deltas"

/** Enable / disable full-text caching for a FSFS repository.
 *
 * @since New in 1.7.
 */
#define SVN_FS_CONFIG_FSFS_CACHE_FULLTEXTS      "fsfs-cache-fulltexts"

/* See also svn_fs_type(). */
/** @since New in 1.1. */
#define SVN_FS_CONFIG_FS_TYPE                   "fs-type"
/** @since New in 1.1. */
#define SVN_FS_TYPE_BDB                         "bdb"
/** @since New in 1.1. */
#define SVN_FS_TYPE_FSFS                        "fsfs"

/** Create repository format compatible with Subversion versions
 * earlier than 1.4.
 *
 *  @since New in 1.4.
 */
#define SVN_FS_CONFIG_PRE_1_4_COMPATIBLE        "pre-1.4-compatible"

/** Create repository format compatible with Subversion versions
 * earlier than 1.5.
 *
 * @since New in 1.5.
 */
#define SVN_FS_CONFIG_PRE_1_5_COMPATIBLE        "pre-1.5-compatible"

/** Create repository format compatible with Subversion versions
 * earlier than 1.6.
 *
 * @since New in 1.6.
 */
#define SVN_FS_CONFIG_PRE_1_6_COMPATIBLE        "pre-1.6-compatible"
/** @} */


/**
 * Callers should invoke this function to initialize global state in
 * the FS library before creating FS objects.  If this function is
 * invoked, no FS objects may be created in another thread at the same
 * time as this invocation, and the provided @a pool must last longer
 * than any FS object created subsequently.
 *
 * If this function is not called, the FS library will make a best
 * effort to bootstrap a mutex for protecting data common to FS
 * objects; however, there is a small window of failure.  Also, a
 * small amount of data will be leaked if the Subversion FS library is
 * dynamically unloaded, and using the bdb FS can potentially segfault
 * or invoke other undefined behavior if this function is not called
 * with an appropriate pool (such as the pool the module was loaded into)
 * when loaded dynamically.
 *
 * If this function is called multiple times before the pool passed to
 * the first call is destroyed or cleared, the later calls will have
 * no effect.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_fs_initialize(apr_pool_t *pool);


/** The type of a warning callback function.  @a baton is the value specified
 * in the call to svn_fs_set_warning_func(); the filesystem passes it through
 * to the callback.  @a err contains the warning message.
 *
 * The callback function should not clear the error that is passed to it;
 * its caller should do that.
 */
typedef void (*svn_fs_warning_callback_t)(void *baton, svn_error_t *err);


/** Provide a callback function, @a warning, that @a fs should use to
 * report (non-fatal) errors.  To print an error, the filesystem will call
 * @a warning, passing it @a warning_baton and the error.
 *
 * By default, this is set to a function that will crash the process.
 * Dumping to @c stderr or <tt>/dev/tty</tt> is not acceptable default
 * behavior for server processes, since those may both be equivalent to
 * <tt>/dev/null</tt>.
 */
void
svn_fs_set_warning_func(svn_fs_t *fs,
                        svn_fs_warning_callback_t warning,
                        void *warning_baton);



/**
 * Create a new, empty Subversion filesystem, stored in the directory
 * @a path, and return a pointer to it in @a *fs_p.  @a path must not
 * currently exist, but its parent must exist.  If @a fs_config is not
 * @c NULL, the options it contains modify the behavior of the
 * filesystem.  The interpretation of @a fs_config is specific to the
 * filesystem back-end.  The new filesystem may be closed by
 * destroying @a pool.
 *
 * @note The lifetime of @a fs_config must not be shorter than @a
 * pool's. It's a good idea to allocate @a fs_config from @a pool or
 * one of its ancestors.
 *
 * If @a fs_config contains a value for #SVN_FS_CONFIG_FS_TYPE, that
 * value determines the filesystem type for the new filesystem.
 * Currently defined values are:
 *
 *   SVN_FS_TYPE_BDB   Berkeley-DB implementation
 *   SVN_FS_TYPE_FSFS  Native-filesystem implementation
 *
 * If @a fs_config is @c NULL or does not contain a value for
 * #SVN_FS_CONFIG_FS_TYPE then the default filesystem type will be used.
 * This will typically be BDB for version 1.1 and FSFS for later versions,
 * though the caller should not rely upon any particular default if they
 * wish to ensure that a filesystem of a specific type is created.
 *
 * @since New in 1.1.
 */
svn_error_t *
svn_fs_create(svn_fs_t **fs_p,
              const char *path,
              apr_hash_t *fs_config,
              apr_pool_t *pool);

/**
 * Open a Subversion filesystem located in the directory @a path, and
 * return a pointer to it in @a *fs_p.  If @a fs_config is not @c
 * NULL, the options it contains modify the behavior of the
 * filesystem.  The interpretation of @a fs_config is specific to the
 * filesystem back-end.  The opened filesystem may be closed by
 * destroying @a pool.
 *
 * @note The lifetime of @a fs_config must not be shorter than @a
 * pool's. It's a good idea to allocate @a fs_config from @a pool or
 * one of its ancestors.
 *
 * Only one thread may operate on any given filesystem object at once.
 * Two threads may access the same filesystem simultaneously only if
 * they open separate filesystem objects.
 *
 * @note You probably don't want to use this directly.  Take a look at
 * svn_repos_open2() instead.
 *
 * @since New in 1.1.
 */
svn_error_t *
svn_fs_open(svn_fs_t **fs_p,
            const char *path,
            apr_hash_t *fs_config,
            apr_pool_t *pool);

/**
 * Upgrade the Subversion filesystem located in the directory @a path
 * to the latest version supported by this library.  Return
 * #SVN_ERR_FS_UNSUPPORTED_UPGRADE and make no changes to the
 * filesystem if the requested upgrade is not supported.  Use @a pool
 * for necessary allocations.
 *
 * @note You probably don't want to use this directly.  Take a look at
 * svn_repos_upgrade() instead.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_fs_upgrade(const char *path,
               apr_pool_t *pool);

/**
 * Return, in @a *fs_type, a string identifying the back-end type of
 * the Subversion filesystem located in @a path.  Allocate @a *fs_type
 * in @a pool.
 *
 * The string should be equal to one of the @c SVN_FS_TYPE_* defined
 * constants, unless the filesystem is a new back-end type added in
 * a later version of Subversion.
 *
 * In general, the type should make no difference in the filesystem's
 * semantics, but there are a few situations (such as backups) where
 * it might matter.
 *
 * @since New in 1.3.
 */
svn_error_t *
svn_fs_type(const char **fs_type,
            const char *path,
            apr_pool_t *pool);

/**
 * Return the path to @a fs's repository, allocated in @a pool.
 * @note This is just what was passed to svn_fs_create() or
 * svn_fs_open() -- might be absolute, might not.
 *
 * @since New in 1.1.
 */
const char *
svn_fs_path(svn_fs_t *fs,
            apr_pool_t *pool);

/**
 * Delete the filesystem at @a path.
 *
 * @since New in 1.1.
 */
svn_error_t *
svn_fs_delete_fs(const char *path,
                 apr_pool_t *pool);

/**
 * Copy a possibly live Subversion filesystem from @a src_path to
 * @a dest_path.  If @a clean is @c TRUE, perform cleanup on the
 * source filesystem as part of the copy operation; currently, this
 * means deleting copied, unused logfiles for a Berkeley DB source
 * filesystem.
 *
 * @since New in 1.1.
 */
svn_error_t *
svn_fs_hotcopy(const char *src_path,
               const char *dest_path,
               svn_boolean_t clean,
               apr_pool_t *pool);

/** Perform any necessary non-catastrophic recovery on the Subversion
 * filesystem located at @a path.
 *
 * If @a cancel_func is not @c NULL, it is called periodically with
 * @a cancel_baton as argument to see if the client wishes to cancel
 * recovery.  BDB filesystems do not currently support cancellation.
 *
 * Do any necessary allocation within @a pool.
 *
 * For FSFS filesystems, recovery is currently limited to recreating
 * the db/current file, and does not require exclusive access.
 *
 * For BDB filesystems, recovery requires exclusive access, and is
 * described in detail below.
 *
 * After an unexpected server exit, due to a server crash or a system
 * crash, a Subversion filesystem based on Berkeley DB needs to run
 * recovery procedures to bring the database back into a consistent
 * state and release any locks that were held by the deceased process.
 * The recovery procedures require exclusive access to the database
 * --- while they execute, no other process or thread may access the
 * database.
 *
 * In a server with multiple worker processes, like Apache, if a
 * worker process accessing the filesystem dies, you must stop the
 * other worker processes, and run recovery.  Then, the other worker
 * processes can re-open the database and resume work.
 *
 * If the server exited cleanly, there is no need to run recovery, but
 * there is no harm in it, either, and it take very little time.  So
 * it's a fine idea to run recovery when the server process starts,
 * before it begins handling any requests.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_fs_recover(const char *path,
               svn_cancel_func_t cancel_func,
               void *cancel_baton,
               apr_pool_t *pool);


/** Subversion filesystems based on Berkeley DB.
 *
 * The following functions are specific to Berkeley DB filesystems.
 *
 * @defgroup svn_fs_bdb Berkeley DB filesystems
 * @{
 */

/** Register an error handling function for Berkeley DB error messages.
 *
 * @deprecated Provided for backward compatibility with the 1.2 API.
 *
 * Despite being first declared deprecated in Subversion 1.3, this API
 * is redundant in versions 1.1 and 1.2 as well.
 *
 * Berkeley DB's error codes are seldom sufficiently informative to allow
 * adequate troubleshooting.  Berkeley DB provides extra messages through
 * a callback function - if an error occurs, the @a handler will be called
 * with two strings: an error message prefix, which will be zero, and
 * an error message.  @a handler might print it out, log it somewhere,
 * etc.
 *
 * Subversion 1.1 and later install their own handler internally, and
 * wrap the messages from Berkeley DB into the standard svn_error_t object,
 * making any information gained through this interface redundant.
 *
 * It is only worth using this function if your program will be used
 * with Subversion 1.0.
 *
 * This function connects to the Berkeley DB @c DBENV->set_errcall interface.
 * Since that interface supports only a single callback, Subversion's internal
 * callback is registered with Berkeley DB, and will forward notifications to
 * a user provided callback after performing its own processing.
 */
SVN_DEPRECATED
svn_error_t *
svn_fs_set_berkeley_errcall(svn_fs_t *fs,
                            void (*handler)(const char *errpfx,
                                            char *msg));

/** Set @a *logfiles to an array of <tt>const char *</tt> log file names
 * of Berkeley DB-based Subversion filesystem.
 *
 * If @a only_unused is @c TRUE, set @a *logfiles to an array which
 * contains only the names of Berkeley DB log files no longer in use
 * by the filesystem.  Otherwise, all log files (used and unused) are
 * returned.

 * This function wraps the Berkeley DB 'log_archive' function
 * called by the db_archive binary.  Repository administrators may
 * want to run this function periodically and delete the unused log
 * files, as a way of reclaiming disk space.
 */
svn_error_t *
svn_fs_berkeley_logfiles(apr_array_header_t **logfiles,
                         const char *path,
                         svn_boolean_t only_unused,
                         apr_pool_t *pool);


/**
 * The following functions are similar to their generic counterparts.
 *
 * In Subversion 1.2 and earlier, they only work on Berkeley DB filesystems.
 * In Subversion 1.3 and later, they perform largely as aliases for their
 * generic counterparts (with the exception of recover, which only gained
 * a generic counterpart in 1.5).
 *
 * @defgroup svn_fs_bdb_deprecated Berkeley DB filesystem compatibility
 * @{
 */

/** @deprecated Provided for backward compatibility with the 1.0 API. */
SVN_DEPRECATED
svn_fs_t *
svn_fs_new(apr_hash_t *fs_config,
           apr_pool_t *pool);

/** @deprecated Provided for backward compatibility with the 1.0 API. */
SVN_DEPRECATED
svn_error_t *
svn_fs_create_berkeley(svn_fs_t *fs,
                       const char *path);

/** @deprecated Provided for backward compatibility with the 1.0 API. */
SVN_DEPRECATED
svn_error_t *
svn_fs_open_berkeley(svn_fs_t *fs,
                     const char *path);

/** @deprecated Provided for backward compatibility with the 1.0 API. */
SVN_DEPRECATED
const char *
svn_fs_berkeley_path(svn_fs_t *fs,
                     apr_pool_t *pool);

/** @deprecated Provided for backward compatibility with the 1.0 API. */
SVN_DEPRECATED
svn_error_t *
svn_fs_delete_berkeley(const char *path,
                       apr_pool_t *pool);

/** @deprecated Provided for backward compatibility with the 1.0 API. */
SVN_DEPRECATED
svn_error_t *
svn_fs_hotcopy_berkeley(const char *src_path,
                        const char *dest_path,
                        svn_boolean_t clean_logs,
                        apr_pool_t *pool);

/** @deprecated Provided for backward compatibility with the 1.4 API. */
SVN_DEPRECATED
svn_error_t *
svn_fs_berkeley_recover(const char *path,
                        apr_pool_t *pool);
/** @} */

/** @} */


/** Filesystem Access Contexts.
 *
 * @since New in 1.2.
 *
 * At certain times, filesystem functions need access to temporary
 * user data.  For example, which user is changing a file?  If the
 * file is locked, has an appropriate lock-token been supplied?
 *
 * This temporary user data is stored in an "access context" object,
 * and the access context is then connected to the filesystem object.
 * Whenever a filesystem function requires information, it can pull
 * things out of the context as needed.
 *
 * @defgroup svn_fs_access_ctx Filesystem access contexts
 * @{
 */

/** An opaque object representing temporary user data. */
typedef struct svn_fs_access_t svn_fs_access_t;


/** Set @a *access_ctx to a new #svn_fs_access_t object representing
 *  @a username, allocated in @a pool.  @a username is presumed to
 *  have been authenticated by the caller.
 *
 *  Make a deep copy of @a username.
 */
svn_error_t *
svn_fs_create_access(svn_fs_access_t **access_ctx,
                     const char *username,
                     apr_pool_t *pool);


/** Associate @a access_ctx with an open @a fs.
 *
 * This function can be run multiple times on the same open
 * filesystem, in order to change the filesystem access context for
 * different filesystem operations.  Pass a NULL value for @a
 * access_ctx to disassociate the current access context from the
 * filesystem.
 */
svn_error_t *
svn_fs_set_access(svn_fs_t *fs,
                  svn_fs_access_t *access_ctx);


/** Set @a *access_ctx to the current @a fs access context, or NULL if
 * there is no current fs access context.
 */
svn_error_t *
svn_fs_get_access(svn_fs_access_t **access_ctx,
                  svn_fs_t *fs);


/** Accessors for the access context: */

/** Set @a *username to the name represented by @a access_ctx. */
svn_error_t *
svn_fs_access_get_username(const char **username,
                           svn_fs_access_t *access_ctx);


/** Push a lock-token @a token associated with path @a path into the
 * context @a access_ctx.  The context remembers all tokens it
 * receives, and makes them available to fs functions.  The token and
 * path are not duplicated into @a access_ctx's pool; make sure the
 * token's lifetime is at least as long as @a access_ctx.
 *
 * @since New in 1.6. */
svn_error_t *
svn_fs_access_add_lock_token2(svn_fs_access_t *access_ctx,
                              const char *path,
                              const char *token);

/**
 * Same as svn_fs_access_add_lock_token2(), but with @a path set to value 1.
 *
 * @deprecated Provided for backward compatibility with the 1.5 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_fs_access_add_lock_token(svn_fs_access_t *access_ctx,
                             const char *token);

/** @} */


/** Filesystem Nodes and Node-Revisions.
 *
 * In a Subversion filesystem, a `node' corresponds roughly to an
 * `inode' in a Unix filesystem:
 * - A node is either a file or a directory.
 * - A node's contents change over time.
 * - When you change a node's contents, it's still the same node; it's
 *   just been changed.  So a node's identity isn't bound to a specific
 *   set of contents.
 * - If you rename a node, it's still the same node, just under a
 *   different name.  So a node's identity isn't bound to a particular
 *   filename.
 *
 * A `node revision' refers to one particular version of a node's contents,
 * that existed over a specific period of time (one or more repository
 * revisions).  Changing a node's contents always creates a new revision of
 * that node, which is to say creates a new `node revision'.  Once created,
 * a node revision's contents never change.
 *
 * When we create a node, its initial contents are the initial revision of
 * the node.  As users make changes to the node over time, we create new
 * revisions of that same node.  When a user commits a change that deletes
 * a file from the filesystem, we don't delete the node, or any revision
 * of it --- those stick around to allow us to recreate prior revisions of
 * the filesystem.  Instead, we just remove the reference to the node
 * from the directory.
 *
 * Each node revision is a part of exactly one node, and appears only once
 * in the history of that node.  It is uniquely identified by a node
 * revision id, #svn_fs_id_t.  Its node revision id also identifies which
 * node it is a part of.
 *
 * @note: Often when we talk about `the node' within the context of a single
 * revision (or transaction), we implicitly mean `the node as it appears in
 * this revision (or transaction)', or in other words `the node revision'.
 *
 * @note: Commonly, a node revision will have the same content as some other
 * node revisions in the same node and in different nodes.  The FS libraries
 * allow different node revisions to share the same data without storing a
 * separate copy of the data.
 *
 * @defgroup svn_fs_nodes Filesystem nodes
 * @{
 */

/** An object representing a node-revision id.  */
typedef struct svn_fs_id_t svn_fs_id_t;


/** Return -1, 0, or 1 if node revisions @a a and @a b are respectively
 * unrelated, equivalent, or otherwise related (part of the same node).
 */
int
svn_fs_compare_ids(const svn_fs_id_t *a,
                   const svn_fs_id_t *b);



/** Return TRUE if node revisions @a id1 and @a id2 are related (part of the
 * same node), else return FALSE.
 */
svn_boolean_t
svn_fs_check_related(const svn_fs_id_t *id1,
                     const svn_fs_id_t *id2);


/**
 * @note This function is not guaranteed to work with all filesystem
 * types.  There is currently no un-deprecated equivalent; contact the
 * Subversion developers if you have a need for it.
 *
 * @deprecated Provided for backward compatibility with the 1.0 API.
 */
SVN_DEPRECATED
svn_fs_id_t *
svn_fs_parse_id(const char *data,
                apr_size_t len,
                apr_pool_t *pool);


/** Return a Subversion string containing the unparsed form of the
 * node revision id @a id.  Allocate the string containing the
 * unparsed form in @a pool.
 */
svn_string_t *
svn_fs_unparse_id(const svn_fs_id_t *id,
                  apr_pool_t *pool);

/** @} */


/** Filesystem Transactions.
 *
 * To make a change to a Subversion filesystem:
 * - Create a transaction object, using svn_fs_begin_txn().
 * - Call svn_fs_txn_root(), to get the transaction's root directory.
 * - Make whatever changes you like in that tree.
 * - Commit the transaction, using svn_fs_commit_txn().
 *
 * The filesystem implementation guarantees that your commit will
 * either:
 * - succeed completely, so that all of the changes are committed to
 *   create a new revision of the filesystem, or
 * - fail completely, leaving the filesystem unchanged.
 *
 * Until you commit the transaction, any changes you make are
 * invisible.  Only when your commit succeeds do they become visible
 * to the outside world, as a new revision of the filesystem.
 *
 * If you begin a transaction, and then decide you don't want to make
 * the change after all (say, because your net connection with the
 * client disappeared before the change was complete), you can call
 * svn_fs_abort_txn(), to cancel the entire transaction; this
 * leaves the filesystem unchanged.
 *
 * The only way to change the contents of files or directories, or
 * their properties, is by making a transaction and creating a new
 * revision, as described above.  Once a revision has been committed, it
 * never changes again; the filesystem interface provides no means to
 * go back and edit the contents of an old revision.  Once history has
 * been recorded, it is set in stone.  Clients depend on this property
 * to do updates and commits reliably; proxies depend on this property
 * to cache changes accurately; and so on.
 *
 * There are two kinds of nodes in the filesystem: mutable, and
 * immutable.  Revisions in the filesystem consist entirely of
 * immutable nodes, whose contents never change.  A transaction in
 * progress, which the user is still constructing, uses mutable nodes
 * for those nodes which have been changed so far, and refers to
 * immutable nodes from existing revisions for portions of the tree
 * which haven't been changed yet in that transaction.
 *
 * Immutable nodes, as part of revisions, never refer to mutable
 * nodes, which are part of uncommitted transactions.  Mutable nodes
 * may refer to immutable nodes, or other mutable nodes.
 *
 * Note that the terms "immutable" and "mutable" describe whether or
 * not the nodes have been changed as part of a transaction --- not
 * the permissions on the nodes they refer to.  Even if you aren't
 * authorized to modify the filesystem's root directory, you might be
 * authorized to change some descendant of the root; doing so would
 * create a new mutable copy of the root directory.  Mutability refers
 * to the role of the node: part of an existing revision, or part of a
 * new one.  This is independent of your authorization to make changes
 * to a given node.
 *
 * Transactions are actually persistent objects, stored in the
 * database.  You can open a filesystem, begin a transaction, and
 * close the filesystem, and then a separate process could open the
 * filesystem, pick up the same transaction, and continue work on it.
 * When a transaction is successfully committed, it is removed from
 * the database.
 *
 * Every transaction is assigned a name.  You can open a transaction
 * by name, and resume work on it, or find out the name of a
 * transaction you already have open.  You can also list all the
 * transactions currently present in the database.
 *
 * You may assign properties to transactions; these are name/value
 * pairs.  When you commit a transaction, all of its properties become
 * unversioned revision properties of the new revision.  (There is one
 * exception: the svn:date property will be automatically set on new
 * transactions to the date that the transaction was created, and will
 * be overwritten when the transaction is committed by the current
 * time; changes to a transaction's svn:date property will not affect
 * its committed value.)
 *
 * Transaction names are guaranteed to contain only letters (upper-
 * and lower-case), digits, `-', and `.', from the ASCII character
 * set.
 *
 * The Subversion filesystem will make a best effort to not reuse
 * transaction names.  The Berkeley DB backend generates transaction
 * names using a sequence, or a counter, which is stored in the BDB
 * database.  Each new transaction increments the counter.  The
 * current value of the counter is not serialized into a filesystem
 * dump file, so dumping and restoring the repository will reset the
 * sequence and reuse transaction names.  The FSFS backend generates a
 * transaction name using the hostname, process ID and current time in
 * microseconds since 00:00:00 January 1, 1970 UTC.  So it is
 * extremely unlikely that a transaction name will be reused.
 *
 * @defgroup svn_fs_txns Filesystem transactions
 * @{
 */

/** The type of a Subversion transaction object.  */
typedef struct svn_fs_txn_t svn_fs_txn_t;


/** @defgroup svn_fs_begin_txn2_flags Bitmask flags for svn_fs_begin_txn2()
 * @since New in 1.2.
 * @{ */

/** Do on-the-fly out-of-dateness checks.  That is, an fs routine may
 * throw error if a caller tries to edit an out-of-date item in the
 * transaction.
 *
 * @warning ### Not yet implemented.
 */
#define SVN_FS_TXN_CHECK_OOD                     0x00001

/** Do on-the-fly lock checks.  That is, an fs routine may throw error
 * if a caller tries to edit a locked item without having rights to the lock.
 */
#define SVN_FS_TXN_CHECK_LOCKS                   0x00002
/** @} */

/**
 * Begin a new transaction on the filesystem @a fs, based on existing
 * revision @a rev.  Set @a *txn_p to a pointer to the new transaction.
 * When committed, this transaction will create a new revision.
 *
 * Allocate the new transaction in @a pool; when @a pool is freed, the new
 * transaction will be closed (neither committed nor aborted).
 *
 * @a flags determines transaction enforcement behaviors, and is composed
 * from the constants SVN_FS_TXN_* (#SVN_FS_TXN_CHECK_OOD etc.).
 *
 * @note If you're building a txn for committing, you probably
 * don't want to call this directly.  Instead, call
 * svn_repos_fs_begin_txn_for_commit(), which honors the
 * repository's hook configurations.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_fs_begin_txn2(svn_fs_txn_t **txn_p,
                  svn_fs_t *fs,
                  svn_revnum_t rev,
                  apr_uint32_t flags,
                  apr_pool_t *pool);


/**
 * Same as svn_fs_begin_txn2(), but with @a flags set to 0.
 *
 * @deprecated Provided for backward compatibility with the 1.1 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_fs_begin_txn(svn_fs_txn_t **txn_p,
                 svn_fs_t *fs,
                 svn_revnum_t rev,
                 apr_pool_t *pool);



/** Commit @a txn.
 *
 * @note You usually don't want to call this directly.
 * Instead, call svn_repos_fs_commit_txn(), which honors the
 * repository's hook configurations.
 *
 * If the transaction conflicts with other changes committed to the
 * repository, return an #SVN_ERR_FS_CONFLICT error.  Otherwise, create
 * a new filesystem revision containing the changes made in @a txn,
 * storing that new revision number in @a *new_rev, and return zero.
 *
 * If @a conflict_p is non-zero, use it to provide details on any
 * conflicts encountered merging @a txn with the most recent committed
 * revisions.  If a conflict occurs, set @a *conflict_p to the path of
 * the conflict in @a txn, with the same lifetime as @a txn;
 * otherwise, set @a *conflict_p to NULL.
 *
 * If the commit succeeds, @a txn is invalid.
 *
 * If the commit fails for any reason, @a *new_rev is an invalid
 * revision number, an error other than #SVN_NO_ERROR is returned and
 * @a txn is still valid; you can make more operations to resolve the
 * conflict, or call svn_fs_abort_txn() to abort the transaction.
 *
 * @note Success or failure of the commit of @a txn is determined by
 * examining the value of @a *new_rev upon this function's return.  If
 * the value is a valid revision number, the commit was successful,
 * even though a non-@c NULL function return value may indicate that
 * something else went wrong in post commit FS processing.
 */
svn_error_t *
svn_fs_commit_txn(const char **conflict_p,
                  svn_revnum_t *new_rev,
                  svn_fs_txn_t *txn,
                  apr_pool_t *pool);


/** Abort the transaction @a txn.  Any changes made in @a txn are
 * discarded, and the filesystem is left unchanged.  Use @a pool for
 * any necessary allocations.
 *
 * @note This function first sets the state of @a txn to "dead", and
 * then attempts to purge it and any related data from the filesystem.
 * If some part of the cleanup process fails, @a txn and some portion
 * of its data may remain in the database after this function returns.
 * Use svn_fs_purge_txn() to retry the transaction cleanup.
 */
svn_error_t *
svn_fs_abort_txn(svn_fs_txn_t *txn,
                 apr_pool_t *pool);


/** Cleanup the dead transaction in @a fs whose ID is @a txn_id.  Use
 * @a pool for all allocations.  If the transaction is not yet dead,
 * the error #SVN_ERR_FS_TRANSACTION_NOT_DEAD is returned.  (The
 * caller probably forgot to abort the transaction, or the cleanup
 * step of that abort failed for some reason.)
 */
svn_error_t *
svn_fs_purge_txn(svn_fs_t *fs,
                 const char *txn_id,
                 apr_pool_t *pool);


/** Set @a *name_p to the name of the transaction @a txn, as a
 * NULL-terminated string.  Allocate the name in @a pool.
 */
svn_error_t *
svn_fs_txn_name(const char **name_p,
                svn_fs_txn_t *txn,
                apr_pool_t *pool);

/** Return @a txn's base revision. */
svn_revnum_t
svn_fs_txn_base_revision(svn_fs_txn_t *txn);



/** Open the transaction named @a name in the filesystem @a fs.  Set @a *txn
 * to the transaction.
 *
 * If there is no such transaction, #SVN_ERR_FS_NO_SUCH_TRANSACTION is
 * the error returned.
 *
 * Allocate the new transaction in @a pool; when @a pool is freed, the new
 * transaction will be closed (neither committed nor aborted).
 */
svn_error_t *
svn_fs_open_txn(svn_fs_txn_t **txn,
                svn_fs_t *fs,
                const char *name,
                apr_pool_t *pool);


/** Set @a *names_p to an array of <tt>const char *</tt> ids which are the
 * names of all the currently active transactions in the filesystem @a fs.
 * Allocate the array in @a pool.
 */
svn_error_t *
svn_fs_list_transactions(apr_array_header_t **names_p,
                         svn_fs_t *fs,
                         apr_pool_t *pool);

/* Transaction properties */

/** Set @a *value_p to the value of the property named @a propname on
 * transaction @a txn.  If @a txn has no property by that name, set
 * @a *value_p to zero.  Allocate the result in @a pool.
 */
svn_error_t *
svn_fs_txn_prop(svn_string_t **value_p,
                svn_fs_txn_t *txn,
                const char *propname,
                apr_pool_t *pool);


/** Set @a *table_p to the entire property list of transaction @a txn, as
 * an APR hash table allocated in @a pool.  The resulting table maps property
 * names to pointers to #svn_string_t objects containing the property value.
 */
svn_error_t *
svn_fs_txn_proplist(apr_hash_t **table_p,
                    svn_fs_txn_t *txn,
                    apr_pool_t *pool);


/** Change a transactions @a txn's property's value, or add/delete a
 * property.  @a name is the name of the property to change, and @a value
 * is the new value of the property, or zero if the property should be
 * removed altogether.  Do any necessary temporary allocation in @a pool.
 */
svn_error_t *
svn_fs_change_txn_prop(svn_fs_txn_t *txn,
                       const char *name,
                       const svn_string_t *value,
                       apr_pool_t *pool);


/** Change, add, and/or delete transaction property values in
 * transaction @a txn.  @a props is an array of <tt>svn_prop_t</tt>
 * elements.  This is equivalent to calling svn_fs_change_txn_prop()
 * multiple times with the @c name and @c value fields of each
 * successive <tt>svn_prop_t</tt>, but may be more efficient.
 * (Properties not mentioned are left alone.)  Do any necessary
 * temporary allocation in @a pool.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_fs_change_txn_props(svn_fs_txn_t *txn,
                        const apr_array_header_t *props,
                        apr_pool_t *pool);

/** @} */


/** Roots.
 *
 * An #svn_fs_root_t object represents the root directory of some
 * revision or transaction in a filesystem.  To refer to particular
 * node or node revision, you provide a root, and a directory path
 * relative to that root.
 *
 * @defgroup svn_fs_roots Filesystem roots
 * @{
 */

/** The Filesystem Root object. */
typedef struct svn_fs_root_t svn_fs_root_t;


/** Set @a *root_p to the root directory of revision @a rev in filesystem @a fs.
 * Allocate @a *root_p in a private subpool of @a pool; the root can be
 * destroyed earlier than @a pool by calling #svn_fs_close_root.
 */
svn_error_t *
svn_fs_revision_root(svn_fs_root_t **root_p,
                     svn_fs_t *fs,
                     svn_revnum_t rev,
                     apr_pool_t *pool);


/** Set @a *root_p to the root directory of @a txn.  Allocate @a *root_p in a
 * private subpool of @a pool; the root can be destroyed earlier than @a pool by
 * calling #svn_fs_close_root.
 */
svn_error_t *
svn_fs_txn_root(svn_fs_root_t **root_p,
                svn_fs_txn_t *txn,
                apr_pool_t *pool);


/** Free the root directory @a root; this only needs to be used if you want to
 * free the memory associated with @a root earlier than the time you destroy
 * the pool passed to the function that created it (svn_fs_revision_root() or
 * svn_fs_txn_root()).
 */
void
svn_fs_close_root(svn_fs_root_t *root);


/** Return the filesystem to which @a root belongs.  */
svn_fs_t *
svn_fs_root_fs(svn_fs_root_t *root);


/** Return @c TRUE iff @a root is a transaction root.  */
svn_boolean_t
svn_fs_is_txn_root(svn_fs_root_t *root);

/** Return @c TRUE iff @a root is a revision root.  */
svn_boolean_t
svn_fs_is_revision_root(svn_fs_root_t *root);


/** If @a root is the root of a transaction, return the name of the
 * transaction, allocated in @a pool; otherwise, return NULL.
 */
const char *
svn_fs_txn_root_name(svn_fs_root_t *root,
                     apr_pool_t *pool);

/** If @a root is the root of a transaction, return the number of the
 * revision on which is was based when created.  Otherwise, return
 * #SVN_INVALID_REVNUM.
 *
 * @since New in 1.5.
 */
svn_revnum_t
svn_fs_txn_root_base_revision(svn_fs_root_t *root);

/** If @a root is the root of a revision, return the revision number.
 * Otherwise, return #SVN_INVALID_REVNUM.
 */
svn_revnum_t
svn_fs_revision_root_revision(svn_fs_root_t *root);

/** @} */


/** Directory entry names and directory paths.
 *
 * Here are the rules for directory entry names, and directory paths:
 *
 * A directory entry name is a Unicode string encoded in UTF-8, and
 * may not contain the NULL character (U+0000).  The name should be in
 * Unicode canonical decomposition and ordering.  No directory entry
 * may be named '.', '..', or the empty string.  Given a directory
 * entry name which fails to meet these requirements, a filesystem
 * function returns an SVN_ERR_FS_PATH_SYNTAX error.
 *
 * A directory path is a sequence of zero or more directory entry
 * names, separated by slash characters (U+002f), and possibly ending
 * with slash characters.  Sequences of two or more consecutive slash
 * characters are treated as if they were a single slash.  If a path
 * ends with a slash, it refers to the same node it would without the
 * slash, but that node must be a directory, or else the function
 * returns an SVN_ERR_FS_NOT_DIRECTORY error.
 *
 * A path consisting of the empty string, or a string containing only
 * slashes, refers to the root directory.
 *
 * @defgroup svn_fs_directories Filesystem directories
 * @{
 */



/** The kind of change that occurred on the path. */
typedef enum svn_fs_path_change_kind_t
{
  /** path modified in txn */
  svn_fs_path_change_modify = 0,

  /** path added in txn */
  svn_fs_path_change_add,

  /** path removed in txn */
  svn_fs_path_change_delete,

  /** path removed and re-added in txn */
  svn_fs_path_change_replace,

  /** ignore all previous change items for path (internal-use only) */
  svn_fs_path_change_reset

} svn_fs_path_change_kind_t;

/** Change descriptor.
 *
 * @note Fields may be added to the end of this structure in future
 * versions.  Therefore, to preserve binary compatibility, users
 * should not directly allocate structures of this type.
 *
 * @since New in 1.6. */
typedef struct svn_fs_path_change2_t
{
  /** node revision id of changed path */
  const svn_fs_id_t *node_rev_id;

  /** kind of change */
  svn_fs_path_change_kind_t change_kind;

  /** were there text mods? */
  svn_boolean_t text_mod;

  /** were there property mods? */
  svn_boolean_t prop_mod;

  /** what node kind is the path?
      (Note: it is legal for this to be #svn_node_unknown.) */
  svn_node_kind_t node_kind;

  /** Copyfrom revision and path; this is only valid if copyfrom_known
   * is true. */
  svn_boolean_t copyfrom_known;
  svn_revnum_t copyfrom_rev;
  const char *copyfrom_path;

  /* NOTE! Please update svn_fs_path_change2_create() when adding new
     fields here. */
} svn_fs_path_change2_t;


/** Similar to #svn_fs_path_change2_t, but without kind and copyfrom
 * information.
 *
 * @deprecated Provided for backwards compatibility with the 1.5 API.
 */

typedef struct svn_fs_path_change_t
{
  /** node revision id of changed path */
  const svn_fs_id_t *node_rev_id;

  /** kind of change */
  svn_fs_path_change_kind_t change_kind;

  /** were there text mods? */
  svn_boolean_t text_mod;

  /** were there property mods? */
  svn_boolean_t prop_mod;

} svn_fs_path_change_t;

/**
 * Allocate an #svn_fs_path_change2_t structure in @a pool, initialize and
 * return it.
 *
 * Set the @c node_rev_id field of the created struct to @a node_rev_id, and
 * @c change_kind to @a change_kind.  Set all other fields to their
 * @c _unknown, @c NULL or invalid value, respectively.
 *
 * @since New in 1.6.
 */
svn_fs_path_change2_t *
svn_fs_path_change2_create(const svn_fs_id_t *node_rev_id,
                           svn_fs_path_change_kind_t change_kind,
                           apr_pool_t *pool);

/** Determine what has changed under a @a root.
 *
 * Allocate and return a hash @a *changed_paths2_p containing descriptions
 * of the paths changed under @a root.  The hash is keyed with
 * <tt>const char *</tt> paths, and has #svn_fs_path_change2_t * values.
 *
 * Callers can assume that this function takes time proportional to
 * the amount of data output, and does not need to do tree crawls;
 * however, it is possible that some of the @c node_kind fields in the
 * #svn_fs_path_change2_t * values will be #svn_node_unknown or
 * that and some of the @c copyfrom_known fields will be FALSE.
 *
 * Use @a pool for all allocations, including the hash and its values.
 *
 * @since New in 1.6.
 */
svn_error_t *
svn_fs_paths_changed2(apr_hash_t **changed_paths2_p,
                      svn_fs_root_t *root,
                      apr_pool_t *pool);


/** Same as svn_fs_paths_changed2(), only with #svn_fs_path_change_t * values
 * in the hash (and thus no kind or copyfrom data).
 *
 * @deprecated Provided for backward compatibility with the 1.5 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_fs_paths_changed(apr_hash_t **changed_paths_p,
                     svn_fs_root_t *root,
                     apr_pool_t *pool);

/** @} */


/* Operations appropriate to all kinds of nodes.  */

/** Set @a *kind_p to the type of node present at @a path under @a
 * root.  If @a path does not exist under @a root, set @a *kind_p to
 * #svn_node_none.  Use @a pool for temporary allocation.
 */
svn_error_t *
svn_fs_check_path(svn_node_kind_t *kind_p,
                  svn_fs_root_t *root,
                  const char *path,
                  apr_pool_t *pool);


/** An opaque node history object. */
typedef struct svn_fs_history_t svn_fs_history_t;


/** Set @a *history_p to an opaque node history object which
 * represents @a path under @a root.  @a root must be a revision root.
 * Use @a pool for all allocations.
 */
svn_error_t *
svn_fs_node_history(svn_fs_history_t **history_p,
                    svn_fs_root_t *root,
                    const char *path,
                    apr_pool_t *pool);


/** Set @a *prev_history_p to an opaque node history object which
 * represents the previous (or "next oldest") interesting history
 * location for the filesystem node represented by @a history, or @c
 * NULL if no such previous history exists.  If @a cross_copies is @c
 * FALSE, also return @c NULL if stepping backwards in history to @a
 * *prev_history_p would cross a filesystem copy operation.
 *
 * @note If this is the first call to svn_fs_history_prev() for the @a
 * history object, it could return a history object whose location is
 * the same as the original.  This will happen if the original
 * location was an interesting one (where the node was modified, or
 * took place in a copy event).  This behavior allows looping callers
 * to avoid the calling svn_fs_history_location() on the object
 * returned by svn_fs_node_history(), and instead go ahead and begin
 * calling svn_fs_history_prev().
 *
 * @note This function uses node-id ancestry alone to determine
 * modifiedness, and therefore does NOT claim that in any of the
 * returned revisions file contents changed, properties changed,
 * directory entries lists changed, etc.
 *
 * @note The revisions returned for @a path will be older than or
 * the same age as the revision of that path in @a root.  That is, if
 * @a root is a revision root based on revision X, and @a path was
 * modified in some revision(s) younger than X, those revisions
 * younger than X will not be included for @a path.  */
svn_error_t *
svn_fs_history_prev(svn_fs_history_t **prev_history_p,
                    svn_fs_history_t *history,
                    svn_boolean_t cross_copies,
                    apr_pool_t *pool);


/** Set @a *path and @a *revision to the path and revision,
 * respectively, of the @a history object.  Use @a pool for all
 * allocations.
 */
svn_error_t *
svn_fs_history_location(const char **path,
                        svn_revnum_t *revision,
                        svn_fs_history_t *history,
                        apr_pool_t *pool);


/** Set @a *is_dir to @c TRUE iff @a path in @a root is a directory.
 * Do any necessary temporary allocation in @a pool.
 */
svn_error_t *
svn_fs_is_dir(svn_boolean_t *is_dir,
              svn_fs_root_t *root,
              const char *path,
              apr_pool_t *pool);


/** Set @a *is_file to @c TRUE iff @a path in @a root is a file.
 * Do any necessary temporary allocation in @a pool.
 */
svn_error_t *
svn_fs_is_file(svn_boolean_t *is_file,
               svn_fs_root_t *root,
               const char *path,
               apr_pool_t *pool);


/** Get the id of a node.
 *
 * Set @a *id_p to the node revision ID of @a path in @a root, allocated in
 * @a pool.
 *
 * If @a root is the root of a transaction, keep in mind that other
 * changes to the transaction can change which node @a path refers to,
 * and even whether the path exists at all.
 */
svn_error_t *
svn_fs_node_id(const svn_fs_id_t **id_p,
               svn_fs_root_t *root,
               const char *path,
               apr_pool_t *pool);

/** Set @a *revision to the revision in which @a path under @a root was
 * created.  Use @a pool for any temporary allocations.  @a *revision will
 * be set to #SVN_INVALID_REVNUM for uncommitted nodes (i.e. modified nodes
 * under a transaction root).  Note that the root of an unmodified transaction
 * is not itself considered to be modified; in that case, return the revision
 * upon which the transaction was based.
 */
svn_error_t *
svn_fs_node_created_rev(svn_revnum_t *revision,
                        svn_fs_root_t *root,
                        const char *path,
                        apr_pool_t *pool);

/** Set @a *revision to the revision in which the line of history
 * represented by @a path under @a root originated.  Use @a pool for
 * any temporary allocations.  If @a root is a transaction root, @a
 * *revision will be set to #SVN_INVALID_REVNUM for any nodes newly
 * added in that transaction (brand new files or directories created
 * using #svn_fs_make_dir or #svn_fs_make_file).
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_fs_node_origin_rev(svn_revnum_t *revision,
                       svn_fs_root_t *root,
                       const char *path,
                       apr_pool_t *pool);

/** Set @a *created_path to the path at which @a path under @a root was
 * created.  Use @a pool for all allocations.  Callers may use this
 * function in conjunction with svn_fs_node_created_rev() to perform a
 * reverse lookup of the mapping of (path, revision) -> node-id that
 * svn_fs_node_id() performs.
 */
svn_error_t *
svn_fs_node_created_path(const char **created_path,
                         svn_fs_root_t *root,
                         const char *path,
                         apr_pool_t *pool);


/** Set @a *value_p to the value of the property named @a propname of
 * @a path in @a root.  If the node has no property by that name, set
 * @a *value_p to zero.  Allocate the result in @a pool.
 */
svn_error_t *
svn_fs_node_prop(svn_string_t **value_p,
                 svn_fs_root_t *root,
                 const char *path,
                 const char *propname,
                 apr_pool_t *pool);


/** Set @a *table_p to the entire property list of @a path in @a root,
 * as an APR hash table allocated in @a pool.  The resulting table maps
 * property names to pointers to #svn_string_t objects containing the
 * property value.
 */
svn_error_t *
svn_fs_node_proplist(apr_hash_t **table_p,
                     svn_fs_root_t *root,
                     const char *path,
                     apr_pool_t *pool);


/** Change a node's property's value, or add/delete a property.
 *
 * - @a root and @a path indicate the node whose property should change.
 *   @a root must be the root of a transaction, not the root of a revision.
 * - @a name is the name of the property to change.
 * - @a value is the new value of the property, or zero if the property should
 *   be removed altogether.
 * Do any necessary temporary allocation in @a pool.
 */
svn_error_t *
svn_fs_change_node_prop(svn_fs_root_t *root,
                        const char *path,
                        const char *name,
                        const svn_string_t *value,
                        apr_pool_t *pool);


/** Determine if the properties of two path/root combinations are different.
 *
 * Set @a *changed_p to 1 if the properties at @a path1 under @a root1 differ
 * from those at @a path2 under @a root2, or set it to 0 if they are the
 * same.  Both paths must exist under their respective roots, and both
 * roots must be in the same filesystem.
 */
svn_error_t *
svn_fs_props_changed(svn_boolean_t *changed_p,
                     svn_fs_root_t *root1,
                     const char *path1,
                     svn_fs_root_t *root2,
                     const char *path2,
                     apr_pool_t *pool);


/** Discover a node's copy ancestry, if any.
 *
 * If the node at @a path in @a root was copied from some other node, set
 * @a *rev_p and @a *path_p to the revision and path (expressed as an
 * absolute filesystem path) of the other node, allocating @a *path_p
 * in @a pool.
 *
 * Else if there is no copy ancestry for the node, set @a *rev_p to
 * #SVN_INVALID_REVNUM and @a *path_p to NULL.
 *
 * If an error is returned, the values of @a *rev_p and @a *path_p are
 * undefined, but otherwise, if one of them is set as described above,
 * you may assume the other is set correspondingly.
 *
 * @a root may be a revision root or a transaction root.
 *
 * Notes:
 *    - Copy ancestry does not descend.  After copying directory D to
 *      E, E will have copy ancestry referring to D, but E's children
 *      may not.  See also svn_fs_copy().
 *
 *    - Copy ancestry *under* a copy is preserved.  That is, if you
 *      copy /A/D/G/pi to /A/D/G/pi2, and then copy /A/D/G to /G, then
 *      /G/pi2 will still have copy ancestry pointing to /A/D/G/pi.
 *      We don't know if this is a feature or a bug yet; if it turns
 *      out to be a bug, then the fix is to make svn_fs_copied_from()
 *      observe the following logic, which currently callers may
 *      choose to follow themselves: if node X has copy history, but
 *      its ancestor A also has copy history, then you may ignore X's
 *      history if X's revision-of-origin is earlier than A's --
 *      because that would mean that X's copy history was preserved in
 *      a copy-under-a-copy scenario.  If X's revision-of-origin is
 *      the same as A's, then it was copied under A during the same
 *      transaction that created A.  (X's revision-of-origin cannot be
 *      greater than A's, if X has copy history.)  @todo See how
 *      people like this, it can always be hidden behind the curtain
 *      if necessary.
 *
 *    - Copy ancestry is not stored as a regular subversion property
 *      because it is not inherited.  Copying foo to bar results in a
 *      revision of bar with copy ancestry; but committing a text
 *      change to bar right after that results in a new revision of
 *      bar without copy ancestry.
 */
svn_error_t *
svn_fs_copied_from(svn_revnum_t *rev_p,
                   const char **path_p,
                   svn_fs_root_t *root,
                   const char *path,
                   apr_pool_t *pool);


/** Set @a *root_p and @a *path_p to the revision root and path of the
 * destination of the most recent copy event that caused @a path to
 * exist where it does in @a root, or to NULL if no such copy exists.
 * When non-NULL, allocate @a *root_p and @a *path_p in @a pool.
 *
 * @a *path_p might be a parent of @a path, rather than @a path
 * itself.  However, it will always be the deepest relevant path.
 * That is, if a copy occurs underneath another copy in the same txn,
 * this function makes sure to set @a *path_p to the longest copy
 * destination path that is still a parent of or equal to @a path.
 *
 * @since New in 1.3.
 */
svn_error_t *
svn_fs_closest_copy(svn_fs_root_t **root_p,
                    const char **path_p,
                    svn_fs_root_t *root,
                    const char *path,
                    apr_pool_t *pool);


/** Retrieve mergeinfo for multiple nodes.
 *
 * @a *catalog is a catalog for @a paths.  It will never be @c NULL,
 * but may be empty.
 *
 * @a root is revision root to use when looking up paths.
 *
 * @a paths are the paths you are requesting information for.
 *
 * @a inherit indicates whether to retrieve explicit,
 * explicit-or-inherited, or only inherited mergeinfo.
 *
 * If @a include_descendants is TRUE, then additionally return the
 * mergeinfo for any descendant of any element of @a paths which has
 * the #SVN_PROP_MERGEINFO property explicitly set on it.  (Note
 * that inheritance is only taken into account for the elements in @a
 * paths; descendants of the elements in @a paths which get their
 * mergeinfo via inheritance are not included in @a *catalog.)
 *
 * Do any necessary temporary allocation in @a pool.
 *
 * @since New in 1.5.
 */
svn_error_t *
svn_fs_get_mergeinfo(svn_mergeinfo_catalog_t *catalog,
                     svn_fs_root_t *root,
                     const apr_array_header_t *paths,
                     svn_mergeinfo_inheritance_t inherit,
                     svn_boolean_t include_descendants,
                     apr_pool_t *pool);

/** Merge changes between two nodes into a third node.
 *
 * Given nodes @a source and @a target, and a common ancestor @a ancestor,
 * modify @a target to contain all the changes made between @a ancestor and
 * @a source, as well as the changes made between @a ancestor and @a target.
 * @a target_root must be the root of a transaction, not a revision.
 *
 * @a source, @a target, and @a ancestor are generally directories; this
 * function recursively merges the directories' contents.  If they are
 * files, this function simply returns an error whenever @a source,
 * @a target, and @a ancestor are all distinct node revisions.
 *
 * If there are differences between @a ancestor and @a source that conflict
 * with changes between @a ancestor and @a target, this function returns an
 * #SVN_ERR_FS_CONFLICT error.
 *
 * If the merge is successful, @a target is left in the merged state, and
 * the base root of @a target's txn is set to the root node of @a source.
 * If an error is returned (whether for conflict or otherwise), @a target
 * is left unaffected.
 *
 * If @a conflict_p is non-NULL, then: a conflict error sets @a *conflict_p
 * to the name of the node in @a target which couldn't be merged,
 * otherwise, success sets @a *conflict_p to NULL.
 *
 * Do any necessary temporary allocation in @a pool.
 */
svn_error_t *
svn_fs_merge(const char **conflict_p,
             svn_fs_root_t *source_root,
             const char *source_path,
             svn_fs_root_t *target_root,
             const char *target_path,
             svn_fs_root_t *ancestor_root,
             const char *ancestor_path,
             apr_pool_t *pool);



/* Directories.  */


/** The type of a Subversion directory entry.  */
typedef struct svn_fs_dirent_t
{

  /** The name of this directory entry.  */
  const char *name;

  /** The node revision ID it names.  */
  const svn_fs_id_t *id;

  /** The node kind. */
  svn_node_kind_t kind;

} svn_fs_dirent_t;


/** Set @a *entries_p to a newly allocated APR hash table containing the
 * entries of the directory at @a path in @a root.  The keys of the table
 * are entry names, as byte strings, excluding the final NULL
 * character; the table's values are pointers to #svn_fs_dirent_t
 * structures.  Allocate the table and its contents in @a pool.
 */
svn_error_t *
svn_fs_dir_entries(apr_hash_t **entries_p,
                   svn_fs_root_t *root,
                   const char *path,
                   apr_pool_t *pool);


/** Create a new directory named @a path in @a root.  The new directory has
 * no entries, and no properties.  @a root must be the root of a transaction,
 * not a revision.
 *
 * Do any necessary temporary allocation in @a pool.
 */
svn_error_t *
svn_fs_make_dir(svn_fs_root_t *root,
                const char *path,
                apr_pool_t *pool);


/** Delete the node named @a path in @a root.  If the node being deleted is
 * a directory, its contents will be deleted recursively.  @a root must be
 * the root of a transaction, not of a revision.  Use @a pool for
 * temporary allocation.
 *
 * If return #SVN_ERR_FS_NO_SUCH_ENTRY, then the basename of @a path is
 * missing from its parent, that is, the final target of the deletion
 * is missing.
 *
 * Attempting to remove the root dir also results in an error,
 * #SVN_ERR_FS_ROOT_DIR, even if the dir is empty.
 */
svn_error_t *
svn_fs_delete(svn_fs_root_t *root,
              const char *path,
              apr_pool_t *pool);


/** Create a copy of @a from_path in @a from_root named @a to_path in
 * @a to_root.  If @a from_path in @a from_root is a directory, copy the
 * tree it refers to recursively.
 *
 * The copy will remember its source; use svn_fs_copied_from() to
 * access this information.
 *
 * @a to_root must be the root of a transaction; @a from_root must be the
 * root of a revision.  (Requiring @a from_root to be the root of a
 * revision makes the implementation trivial: there is no detectable
 * difference (modulo node revision ID's) between copying @a from and
 * simply adding a reference to it.  So the operation takes place in
 * constant time.  However, there's no reason not to extend this to
 * mutable nodes --- it's just more code.)  Further, @a to_root and @a
 * from_root must represent the same filesystem.
 *
 * @note To do a copy without preserving copy history, use
 * svn_fs_revision_link().
 *
 * Do any necessary temporary allocation in @a pool.
 */
svn_error_t *
svn_fs_copy(svn_fs_root_t *from_root,
            const char *from_path,
            svn_fs_root_t *to_root,
            const char *to_path,
            apr_pool_t *pool);


/** Like svn_fs_copy(), but doesn't record copy history, and preserves
 * the PATH.  You cannot use svn_fs_copied_from() later to find out
 * where this copy came from.
 *
 * Use svn_fs_revision_link() in situations where you don't care
 * about the copy history, and where @a to_path and @a from_path are
 * the same, because it is cheaper than svn_fs_copy().
 */
svn_error_t *
svn_fs_revision_link(svn_fs_root_t *from_root,
                     svn_fs_root_t *to_root,
                     const char *path,
                     apr_pool_t *pool);

/* Files.  */

/** Set @a *length_p to the length of the file @a path in @a root, in bytes.
 * Do any necessary temporary allocation in @a pool.
 */
svn_error_t *
svn_fs_file_length(svn_filesize_t *length_p,
                   svn_fs_root_t *root,
                   const char *path,
                   apr_pool_t *pool);


/** Set @a *checksum to the checksum of type @a kind for the file @a path.
 * @a *checksum will be allocated out of @a pool, which will also be used
 * for temporary allocations.
 *
 * If the filesystem does not have a prerecorded checksum of @a kind for
 * @a path, and @a force is not TRUE, do not calculate a checksum
 * dynamically, just put NULL into @a checksum.  (By convention, the NULL
 * checksum is considered to match any checksum.)
 *
 * Notes:
 *
 * You might wonder, why do we only provide this interface for file
 * contents, and not for properties or directories?
 *
 * The answer is that property lists and directory entry lists are
 * essentially data structures, not text.  We serialize them for
 * transmission, but there is no guarantee that the consumer will
 * parse them into the same form, or even the same order, as the
 * producer.  It's difficult to find a checksumming method that
 * reaches the same result given such variation in input.  (I suppose
 * we could calculate an independent MD5 sum for each propname and
 * value, and XOR them together; same with directory entry names.
 * Maybe that's the solution?)  Anyway, for now we punt.  The most
 * important data, and the only data that goes through svndiff
 * processing, is file contents, so that's what we provide
 * checksumming for.
 *
 * Internally, of course, the filesystem checksums everything, because
 * it has access to the lowest level storage forms: strings behind
 * representations.
 *
 * @since New in 1.6.
 */
svn_error_t *
svn_fs_file_checksum(svn_checksum_t **checksum,
                     svn_checksum_kind_t kind,
                     svn_fs_root_t *root,
                     const char *path,
                     svn_boolean_t force,
                     apr_pool_t *pool);

/**
 * Same as svn_fs_file_checksum(), only always put the MD5 checksum of file
 * @a path into @a digest, which should point to @c APR_MD5_DIGESTSIZE bytes
 * of storage.  If the checksum doesn't exist, put all 0's into @a digest.
 *
 * @deprecated Provided for backward compatibility with the 1.5 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_fs_file_md5_checksum(unsigned char digest[],
                         svn_fs_root_t *root,
                         const char *path,
                         apr_pool_t *pool);


/** Set @a *contents to a readable generic stream that will yield the
 * contents of the file @a path in @a root.  Allocate the stream in
 * @a pool.  You can only use @a *contents for as long as the underlying
 * filesystem is open.  If @a path is not a file, return
 * #SVN_ERR_FS_NOT_FILE.
 *
 * If @a root is the root of a transaction, it is possible that the
 * contents of the file @a path will change between calls to
 * svn_fs_file_contents().  In that case, the result of reading from
 * @a *contents is undefined.
 *
 * ### @todo kff: I am worried about lifetime issues with this pool vs
 * the trail created farther down the call stack.  Trace this function
 * to investigate...
 */
svn_error_t *
svn_fs_file_contents(svn_stream_t **contents,
                     svn_fs_root_t *root,
                     const char *path,
                     apr_pool_t *pool);


/** Create a new file named @a path in @a root.  The file's initial contents
 * are the empty string, and it has no properties.  @a root must be the
 * root of a transaction, not a revision.
 *
 * Do any necessary temporary allocation in @a pool.
 */
svn_error_t *
svn_fs_make_file(svn_fs_root_t *root,
                 const char *path,
                 apr_pool_t *pool);


/** Apply a text delta to the file @a path in @a root.  @a root must be the
 * root of a transaction, not a revision.
 *
 * Set @a *contents_p to a function ready to receive text delta windows
 * describing how to change the file's contents, relative to its
 * current contents.  Set @a *contents_baton_p to a baton to pass to
 * @a *contents_p.
 *
 * If @a path does not exist in @a root, return an error.  (You cannot use
 * this routine to create new files;  use svn_fs_make_file() to create
 * an empty file first.)
 *
 * @a base_checksum is the hex MD5 digest for the base text against
 * which the delta is to be applied; it is ignored if NULL, and may be
 * ignored even if not NULL.  If it is not ignored, it must match the
 * checksum of the base text against which svndiff data is being
 * applied; if not, svn_fs_apply_textdelta() or the @a *contents_p call
 * which detects the mismatch will return the error
 * #SVN_ERR_CHECKSUM_MISMATCH (if there is no base text, there may
 * still be an error if @a base_checksum is neither NULL nor the
 * checksum of the empty string).
 *
 * @a result_checksum is the hex MD5 digest for the fulltext that
 * results from this delta application.  It is ignored if NULL, but if
 * not NULL, it must match the checksum of the result; if it does not,
 * then the @a *contents_p call which detects the mismatch will return
 * the error #SVN_ERR_CHECKSUM_MISMATCH.
 *
 * The caller must send all delta windows including the terminating
 * NULL window to @a *contents_p before making further changes to the
 * transaction.
 *
 * Do temporary allocation in @a pool.
 */
svn_error_t *
svn_fs_apply_textdelta(svn_txdelta_window_handler_t *contents_p,
                       void **contents_baton_p,
                       svn_fs_root_t *root,
                       const char *path,
                       const char *base_checksum,
                       const char *result_checksum,
                       apr_pool_t *pool);


/** Write data directly to the file @a path in @a root.  @a root must be the
 * root of a transaction, not a revision.
 *
 * Set @a *contents_p to a stream ready to receive full textual data.
 * When the caller closes this stream, the data replaces the previous
 * contents of the file.  The caller must write all file data and close
 * the stream before making further changes to the transaction.
 *
 * If @a path does not exist in @a root, return an error.  (You cannot use
 * this routine to create new files;  use svn_fs_make_file() to create
 * an empty file first.)
 *
 * @a result_checksum is the hex MD5 digest for the final fulltext
 * written to the stream.  It is ignored if NULL, but if not null, it
 * must match the checksum of the result; if it does not, then the @a
 * *contents_p call which detects the mismatch will return the error
 * #SVN_ERR_CHECKSUM_MISMATCH.
 *
 * Do any necessary temporary allocation in @a pool.
 *
 * ### This is like svn_fs_apply_textdelta(), but takes the text
 * straight.  It is currently used only by the loader, see
 * libsvn_repos/load.c.  It should accept a checksum, of course, which
 * would come from an (optional) header in the dump file.  See
 * http://subversion.tigris.org/issues/show_bug.cgi?id=1102 for more.
 */
svn_error_t *
svn_fs_apply_text(svn_stream_t **contents_p,
                  svn_fs_root_t *root,
                  const char *path,
                  const char *result_checksum,
                  apr_pool_t *pool);


/** Check if the contents of two root/path combos have changed.
 *
 * Set @a *changed_p to 1 if the contents at @a path1 under @a root1 differ
 * from those at @a path2 under @a root2, or set it to 0 if they are the
 * same.  Both paths must exist under their respective roots, and both
 * roots must be in the same filesystem.
 */
svn_error_t *
svn_fs_contents_changed(svn_boolean_t *changed_p,
                        svn_fs_root_t *root1,
                        const char *path1,
                        svn_fs_root_t *root2,
                        const char *path2,
                        apr_pool_t *pool);



/* Filesystem revisions.  */


/** Set @a *youngest_p to the number of the youngest revision in filesystem
 * @a fs.  Use @a pool for all temporary allocation.
 *
 * The oldest revision in any filesystem is numbered zero.
 */
svn_error_t *
svn_fs_youngest_rev(svn_revnum_t *youngest_p,
                    svn_fs_t *fs,
                    apr_pool_t *pool);


/** Provide filesystem @a fs the opportunity to compress storage relating to
 * associated with  @a revision in filesystem @a fs.  Use @a pool for all
 * allocations.
 *
 * @note This can be a time-consuming process, depending the breadth
 * of the changes made in @a revision, and the depth of the history of
 * those changed paths.  This may also be a no op.
 */
svn_error_t *
svn_fs_deltify_revision(svn_fs_t *fs,
                        svn_revnum_t revision,
                        apr_pool_t *pool);


/** Set @a *value_p to the value of the property named @a propname on
 * revision @a rev in the filesystem @a fs.  If @a rev has no property by
 * that name, set @a *value_p to zero.  Allocate the result in @a pool.
 */
svn_error_t *
svn_fs_revision_prop(svn_string_t **value_p,
                     svn_fs_t *fs,
                     svn_revnum_t rev,
                     const char *propname,
                     apr_pool_t *pool);


/** Set @a *table_p to the entire property list of revision @a rev in
 * filesystem @a fs, as an APR hash table allocated in @a pool.  The table
 * maps <tt>char *</tt> property names to #svn_string_t * values; the names
 * and values are allocated in @a pool.
 */
svn_error_t *
svn_fs_revision_proplist(apr_hash_t **table_p,
                         svn_fs_t *fs,
                         svn_revnum_t rev,
                         apr_pool_t *pool);


/** Change a revision's property's value, or add/delete a property.
 *
 * - @a fs is a filesystem, and @a rev is the revision in that filesystem
 *   whose property should change.
 * - @a name is the name of the property to change.
 * - if @a old_value_p is not @c NULL, then changing the property will fail with
 *   error #SVN_ERR_FS_PROP_BASEVALUE_MISMATCH if the present value of the
 *   property is not @a *old_value_p.  (This is an atomic test-and-set).
 *   @a *old_value_p may be @c NULL, representing that the property must be not
 *   already set.
 * - @a value is the new value of the property, or zero if the property should
 *   be removed altogether.
 *
 * Note that revision properties are non-historied --- you can change
 * them after the revision has been committed.  They are not protected
 * via transactions.
 *
 * Do any necessary temporary allocation in @a pool.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_fs_change_rev_prop2(svn_fs_t *fs,
                        svn_revnum_t rev,
                        const char *name,
                        const svn_string_t *const *old_value_p,
                        const svn_string_t *value,
                        apr_pool_t *pool);


/**
 * Similar to svn_fs_change_rev_prop2(), but with @a old_value_p passed as
 * @c NULL.
 *
 * @deprecated Provided for backward compatibility with the 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_fs_change_rev_prop(svn_fs_t *fs,
                       svn_revnum_t rev,
                       const char *name,
                       const svn_string_t *value,
                       apr_pool_t *pool);



/* Computing deltas.  */


/** Set @a *stream_p to a pointer to a delta stream that will turn the
 * contents of the file @a source into the contents of the file @a target.
 * If @a source_root is zero, use a file with zero length as the source.
 *
 * This function does not compare the two files' properties.
 *
 * Allocate @a *stream_p, and do any necessary temporary allocation, in
 * @a pool.
 */
svn_error_t *
svn_fs_get_file_delta_stream(svn_txdelta_stream_t **stream_p,
                             svn_fs_root_t *source_root,
                             const char *source_path,
                             svn_fs_root_t *target_root,
                             const char *target_path,
                             apr_pool_t *pool);



/* UUID manipulation. */

/** Populate @a *uuid with the UUID associated with @a fs.  Allocate
    @a *uuid in @a pool.  */
svn_error_t *
svn_fs_get_uuid(svn_fs_t *fs,
                const char **uuid,
                apr_pool_t *pool);


/** If not @c NULL, associate @a *uuid with @a fs.  Otherwise (if @a
 * uuid is @c NULL), generate a new UUID for @a fs.  Use @a pool for
 * any scratch work.
 */
svn_error_t *
svn_fs_set_uuid(svn_fs_t *fs,
                const char *uuid,
                apr_pool_t *pool);


/* Non-historical properties.  */

/* [[Yes, do tell.]] */



/** @defgroup svn_fs_locks Filesystem locks
 * @{
 * @since New in 1.2. */

/** A lock represents one user's exclusive right to modify a path in a
 * filesystem.  In order to create or destroy a lock, a username must
 * be associated with the filesystem's access context (see
 * #svn_fs_access_t).
 *
 * When a lock is created, a 'lock-token' is returned.  The lock-token
 * is a unique URI that represents the lock (treated as an opaque
 * string by the client), and is required to make further use of the
 * lock (including removal of the lock.)  A lock-token can also be
 * queried to return a svn_lock_t structure that describes the details
 * of the lock.  lock-tokens must not contain any newline character,
 * mainly due to the serialization for tokens for pre-commit hook.
 *
 * Locks are not secret; anyone can view existing locks in a
 * filesystem.  Locks are not omnipotent: they can broken and stolen
 * by people who don't "own" the lock.  (Though admins can tailor a
 * custom break/steal policy via libsvn_repos pre-lock hook script.)
 *
 * Locks can be created with an optional expiration date.  If a lock
 * has an expiration date, then the act of fetching/reading it might
 * cause it to automatically expire, returning either nothing or an
 * expiration error (depending on the API).
 */


/** Lock @a path in @a fs, and set @a *lock to a lock
 * representing the new lock, allocated in @a pool.
 *
 * @warning You may prefer to use svn_repos_fs_lock() instead,
 * which see.
 *
 * @a fs must have a username associated with it (see
 * #svn_fs_access_t), else return #SVN_ERR_FS_NO_USER.  Set the
 * 'owner' field in the new lock to the fs username.
 *
 * @a comment is optional: it's either an xml-escapable UTF8 string
 * which describes the lock, or it is @c NULL.
 *
 * @a is_dav_comment describes whether the comment was created by a
 * generic DAV client; only mod_dav_svn's autoversioning feature needs
 * to use it.  If in doubt, pass 0.
 *
 * If path is already locked, then return #SVN_ERR_FS_PATH_ALREADY_LOCKED,
 * unless @a steal_lock is TRUE, in which case "steal" the existing
 * lock, even if the FS access-context's username does not match the
 * current lock's owner: delete the existing lock on @a path, and
 * create a new one.
 *
 * @a token is a lock token such as can be generated using
 * svn_fs_generate_lock_token() (indicating that the caller wants to
 * dictate the lock token used), or it is @c NULL (indicating that the
 * caller wishes to have a new token generated by this function).  If
 * @a token is not @c NULL, and represents an existing lock, then @a
 * path must match the path associated with that existing lock.
 *
 * If @a expiration_date is zero, then create a non-expiring lock.
 * Else, the lock will expire at @a expiration_date.
 *
 * If @a current_rev is a valid revnum, then do an out-of-dateness
 * check.  If the revnum is less than the last-changed-revision of @a
 * path (or if @a path doesn't exist in HEAD), return
 * #SVN_ERR_FS_OUT_OF_DATE.
 *
 * @note At this time, only files can be locked.
 */
svn_error_t *
svn_fs_lock(svn_lock_t **lock,
            svn_fs_t *fs,
            const char *path,
            const char *token,
            const char *comment,
            svn_boolean_t is_dav_comment,
            apr_time_t expiration_date,
            svn_revnum_t current_rev,
            svn_boolean_t steal_lock,
            apr_pool_t *pool);


/** Generate a unique lock-token using @a fs. Return in @a *token,
 * allocated in @a pool.
 *
 * This can be used in to populate lock->token before calling
 * svn_fs_attach_lock().
 */
svn_error_t *
svn_fs_generate_lock_token(const char **token,
                           svn_fs_t *fs,
                           apr_pool_t *pool);


/** Remove the lock on @a path represented by @a token in @a fs.
 *
 * If @a token doesn't point to a lock, return #SVN_ERR_FS_BAD_LOCK_TOKEN.
 * If @a token points to an expired lock, return #SVN_ERR_FS_LOCK_EXPIRED.
 * If @a fs has no username associated with it, return #SVN_ERR_FS_NO_USER
 * unless @a break_lock is specified.
 *
 * If @a token points to a lock, but the username of @a fs's access
 * context doesn't match the lock's owner, return
 * #SVN_ERR_FS_LOCK_OWNER_MISMATCH.  If @a break_lock is TRUE, however, don't
 * return error;  allow the lock to be "broken" in any case.  In the latter
 * case, @a token shall be @c NULL.
 *
 * Use @a pool for temporary allocations.
 */
svn_error_t *
svn_fs_unlock(svn_fs_t *fs,
              const char *path,
              const char *token,
              svn_boolean_t break_lock,
              apr_pool_t *pool);


/** If @a path is locked in @a fs, set @a *lock to an svn_lock_t which
 *  represents the lock, allocated in @a pool.
 *
 * If @a path is not locked, set @a *lock to NULL.
 */
svn_error_t *
svn_fs_get_lock(svn_lock_t **lock,
                svn_fs_t *fs,
                const char *path,
                apr_pool_t *pool);


/** The type of a lock discovery callback function.  @a baton is the
 * value specified in the call to svn_fs_get_locks(); the filesystem
 * passes it through to the callback.  @a lock is a lock structure.
 * @a pool is a temporary subpool for use by the callback
 * implementation -- it is cleared after invocation of the callback.
 */
typedef svn_error_t *(*svn_fs_get_locks_callback_t)(void *baton,
                                                    svn_lock_t *lock,
                                                    apr_pool_t *pool);


/** Report locks on or below @a path in @a fs using the @a
 * get_locks_func / @a get_locks_baton.  Use @a pool for necessary
 * allocations.
 *
 * @a depth limits the reported locks to those associated with paths
 * within the specified depth of @a path, and must be one of the
 * following values:  #svn_depth_empty, #svn_depth_files,
 * #svn_depth_immediates, or #svn_depth_infinity.
 *
 * If the @a get_locks_func callback implementation returns an error,
 * lock iteration will terminate and that error will be returned by
 * this function.
 *
 * @note Over the course of this function's invocation, locks might be
 * added, removed, or modified by concurrent processes.  Callers need
 * to anticipate and gracefully handle the transience of this
 * information.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_fs_get_locks2(svn_fs_t *fs,
                  const char *path,
                  svn_depth_t depth,
                  svn_fs_get_locks_callback_t get_locks_func,
                  void *get_locks_baton,
                  apr_pool_t *pool);

/** Similar to svn_fs_get_locks2(), but with @a depth always passed as
 * svn_depth_infinity, and with the following known problem (which is
 * not present in svn_fs_get_locks2()):
 *
 * @note On Berkeley-DB-backed filesystems in Subversion 1.6 and
 * prior, the @a get_locks_func callback will be invoked from within a
 * Berkeley-DB transaction trail.  Implementors of the callback are,
 * as a result, forbidden from calling any svn_fs API functions which
 * might themselves attempt to start a new Berkeley DB transaction
 * (which is most of this svn_fs API).  Yes, this is a nasty
 * implementation detail to have to be aware of.
 *
 * @deprecated Provided for backward compatibility with the 1.6 API.
 */
SVN_DEPRECATED
svn_error_t *
svn_fs_get_locks(svn_fs_t *fs,
                 const char *path,
                 svn_fs_get_locks_callback_t get_locks_func,
                 void *get_locks_baton,
                 apr_pool_t *pool);

/** @} */

/**
 * Append a textual list of all available FS modules to the stringbuf
 * @a output.
 *
 * @since New in 1.2.
 */
svn_error_t *
svn_fs_print_modules(svn_stringbuf_t *output,
                     apr_pool_t *pool);


/** The kind of action being taken by 'pack'. */
typedef enum svn_fs_pack_notify_action_t
{
  /** packing of the shard has commenced */
  svn_fs_pack_notify_start = 0,

  /** packing of the shard is completed */
  svn_fs_pack_notify_end,

  /** packing of the shard revprops has commenced
      @since New in 1.7. */
  svn_fs_pack_notify_start_revprop,

  /** packing of the shard revprops has completed
      @since New in 1.7. */
  svn_fs_pack_notify_end_revprop

} svn_fs_pack_notify_action_t;

/** The type of a pack notification function.  @a shard is the shard being
 * acted upon; @a action is the type of action being performed.  @a baton is
 * the corresponding baton for the notification function, and @a pool can
 * be used for temporary allocations, but will be cleared between invocations.
 */
typedef svn_error_t *(*svn_fs_pack_notify_t)(void *baton,
                                             apr_int64_t shard,
                                             svn_fs_pack_notify_action_t action,
                                             apr_pool_t *pool);

/**
 * Possibly update the filesystem located in the directory @a path
 * to use disk space more efficiently.
 *
 * @since New in 1.6.
 */
svn_error_t *
svn_fs_pack(const char *db_path,
            svn_fs_pack_notify_t notify_func,
            void *notify_baton,
            svn_cancel_func_t cancel_func,
            void *cancel_baton,
            apr_pool_t *pool);


/** @} */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_FS_H */
