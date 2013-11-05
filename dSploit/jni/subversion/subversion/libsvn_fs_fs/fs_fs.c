/* fs_fs.c --- filesystem operations specific to fs_fs
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#include <apr_general.h>
#include <apr_pools.h>
#include <apr_file_io.h>
#include <apr_uuid.h>
#include <apr_lib.h>
#include <apr_md5.h>
#include <apr_sha1.h>
#include <apr_strings.h>
#include <apr_thread_mutex.h>

#include "svn_pools.h"
#include "svn_fs.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_hash.h"
#include "svn_props.h"
#include "svn_sorts.h"
#include "svn_string.h"
#include "svn_time.h"
#include "svn_mergeinfo.h"
#include "svn_config.h"
#include "svn_ctype.h"

#include "fs.h"
#include "tree.h"
#include "lock.h"
#include "key-gen.h"
#include "fs_fs.h"
#include "id.h"
#include "rep-cache.h"
#include "temp_serializer.h"

#include "private/svn_fs_util.h"
#include "../libsvn_fs/fs-loader.h"

#include "svn_private_config.h"
#include "temp_serializer.h"

/* An arbitrary maximum path length, so clients can't run us out of memory
 * by giving us arbitrarily large paths. */
#define FSFS_MAX_PATH_LEN 4096

/* The default maximum number of files per directory to store in the
   rev and revprops directory.  The number below is somewhat arbitrary,
   and can be overriden by defining the macro while compiling; the
   figure of 1000 is reasonable for VFAT filesystems, which are by far
   the worst performers in this area. */
#ifndef SVN_FS_FS_DEFAULT_MAX_FILES_PER_DIR
#define SVN_FS_FS_DEFAULT_MAX_FILES_PER_DIR 1000
#endif

/* Following are defines that specify the textual elements of the
   native filesystem directories and revision files. */

/* Headers used to describe node-revision in the revision file. */
#define HEADER_ID          "id"
#define HEADER_TYPE        "type"
#define HEADER_COUNT       "count"
#define HEADER_PROPS       "props"
#define HEADER_TEXT        "text"
#define HEADER_CPATH       "cpath"
#define HEADER_PRED        "pred"
#define HEADER_COPYFROM    "copyfrom"
#define HEADER_COPYROOT    "copyroot"
#define HEADER_FRESHTXNRT  "is-fresh-txn-root"
#define HEADER_MINFO_HERE  "minfo-here"
#define HEADER_MINFO_CNT   "minfo-cnt"

/* Kinds that a change can be. */
#define ACTION_MODIFY      "modify"
#define ACTION_ADD         "add"
#define ACTION_DELETE      "delete"
#define ACTION_REPLACE     "replace"
#define ACTION_RESET       "reset"

/* True and False flags. */
#define FLAG_TRUE          "true"
#define FLAG_FALSE         "false"

/* Kinds that a node-rev can be. */
#define KIND_FILE          "file"
#define KIND_DIR           "dir"

/* Kinds of representation. */
#define REP_PLAIN          "PLAIN"
#define REP_DELTA          "DELTA"

/* Notes:

To avoid opening and closing the rev-files all the time, it would
probably be advantageous to keep each rev-file open for the
lifetime of the transaction object.  I'll leave that as a later
optimization for now.

I didn't keep track of pool lifetimes at all in this code.  There
are likely some errors because of that.

*/

/* The vtable associated with an open transaction object. */
static txn_vtable_t txn_vtable = {
  svn_fs_fs__commit_txn,
  svn_fs_fs__abort_txn,
  svn_fs_fs__txn_prop,
  svn_fs_fs__txn_proplist,
  svn_fs_fs__change_txn_prop,
  svn_fs_fs__txn_root,
  svn_fs_fs__change_txn_props
};

/* Declarations. */

static svn_error_t *
read_min_unpacked_rev(svn_revnum_t *min_unpacked_rev,
                      const char *path,
                      apr_pool_t *pool);

static svn_error_t *
update_min_unpacked_rev(svn_fs_t *fs, apr_pool_t *pool);

/* Pathname helper functions */

/* Return TRUE is REV is packed in FS, FALSE otherwise. */
static svn_boolean_t
is_packed_rev(svn_fs_t *fs, svn_revnum_t rev)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  return (rev < ffd->min_unpacked_rev);
}

/* Return TRUE is REV is packed in FS, FALSE otherwise. */
static svn_boolean_t
is_packed_revprop(svn_fs_t *fs, svn_revnum_t rev)
{
#if 0
  fs_fs_data_t *ffd = fs->fsap_data;

  return (rev < ffd->min_unpacked_revprop);
#else
  return FALSE;
#endif
}

static const char *
path_format(svn_fs_t *fs, apr_pool_t *pool)
{
  return svn_dirent_join(fs->path, PATH_FORMAT, pool);
}

static APR_INLINE const char *
path_uuid(svn_fs_t *fs, apr_pool_t *pool)
{
  return svn_dirent_join(fs->path, PATH_UUID, pool);
}

const char *
svn_fs_fs__path_current(svn_fs_t *fs, apr_pool_t *pool)
{
  return svn_dirent_join(fs->path, PATH_CURRENT, pool);
}

static APR_INLINE const char *
path_txn_current(svn_fs_t *fs, apr_pool_t *pool)
{
  return svn_dirent_join(fs->path, PATH_TXN_CURRENT, pool);
}

static APR_INLINE const char *
path_txn_current_lock(svn_fs_t *fs, apr_pool_t *pool)
{
  return svn_dirent_join(fs->path, PATH_TXN_CURRENT_LOCK, pool);
}

static APR_INLINE const char *
path_lock(svn_fs_t *fs, apr_pool_t *pool)
{
  return svn_dirent_join(fs->path, PATH_LOCK_FILE, pool);
}

static const char *
path_rev_packed(svn_fs_t *fs, svn_revnum_t rev, const char *kind,
                apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  assert(ffd->max_files_per_dir);
  assert(is_packed_rev(fs, rev));

  return svn_dirent_join_many(pool, fs->path, PATH_REVS_DIR,
                              apr_psprintf(pool, "%ld.pack",
                                           rev / ffd->max_files_per_dir),
                              kind, NULL);
}

static const char *
path_rev_shard(svn_fs_t *fs, svn_revnum_t rev, apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  assert(ffd->max_files_per_dir);
  return svn_dirent_join_many(pool, fs->path, PATH_REVS_DIR,
                              apr_psprintf(pool, "%ld",
                                                 rev / ffd->max_files_per_dir),
                              NULL);
}

static const char *
path_rev(svn_fs_t *fs, svn_revnum_t rev, apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  assert(! is_packed_rev(fs, rev));

  if (ffd->max_files_per_dir)
    {
      return svn_dirent_join(path_rev_shard(fs, rev, pool),
                             apr_psprintf(pool, "%ld", rev),
                             pool);
    }

  return svn_dirent_join_many(pool, fs->path, PATH_REVS_DIR,
                              apr_psprintf(pool, "%ld", rev), NULL);
}

svn_error_t *
svn_fs_fs__path_rev_absolute(const char **path,
                             svn_fs_t *fs,
                             svn_revnum_t rev,
                             apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  if (ffd->format < SVN_FS_FS__MIN_PACKED_FORMAT
      || ! is_packed_rev(fs, rev))
    {
      *path = path_rev(fs, rev, pool);
    }
  else
    {
      *path = path_rev_packed(fs, rev, "pack", pool);
    }

  return SVN_NO_ERROR;
}

static const char *
path_revprops_shard(svn_fs_t *fs, svn_revnum_t rev, apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  assert(ffd->max_files_per_dir);
  return svn_dirent_join_many(pool, fs->path, PATH_REVPROPS_DIR,
                              apr_psprintf(pool, "%ld",
                                           rev / ffd->max_files_per_dir),
                              NULL);
}

static const char *
path_revprops(svn_fs_t *fs, svn_revnum_t rev, apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  if (ffd->max_files_per_dir)
    {
      return svn_dirent_join(path_revprops_shard(fs, rev, pool),
                             apr_psprintf(pool, "%ld", rev),
                             pool);
    }

  return svn_dirent_join_many(pool, fs->path, PATH_REVPROPS_DIR,
                              apr_psprintf(pool, "%ld", rev), NULL);
}

static APR_INLINE const char *
path_txn_dir(svn_fs_t *fs, const char *txn_id, apr_pool_t *pool)
{
  SVN_ERR_ASSERT_NO_RETURN(txn_id != NULL);
  return svn_dirent_join_many(pool, fs->path, PATH_TXNS_DIR,
                              apr_pstrcat(pool, txn_id, PATH_EXT_TXN,
                                          (char *)NULL),
                              NULL);
}

static APR_INLINE const char *
path_txn_changes(svn_fs_t *fs, const char *txn_id, apr_pool_t *pool)
{
  return svn_dirent_join(path_txn_dir(fs, txn_id, pool), PATH_CHANGES, pool);
}

static APR_INLINE const char *
path_txn_props(svn_fs_t *fs, const char *txn_id, apr_pool_t *pool)
{
  return svn_dirent_join(path_txn_dir(fs, txn_id, pool), PATH_TXN_PROPS, pool);
}

static APR_INLINE const char *
path_txn_next_ids(svn_fs_t *fs, const char *txn_id, apr_pool_t *pool)
{
  return svn_dirent_join(path_txn_dir(fs, txn_id, pool), PATH_NEXT_IDS, pool);
}

static APR_INLINE const char *
path_min_unpacked_rev(svn_fs_t *fs, apr_pool_t *pool)
{
  return svn_dirent_join(fs->path, PATH_MIN_UNPACKED_REV, pool);
}


static APR_INLINE const char *
path_txn_proto_rev(svn_fs_t *fs, const char *txn_id, apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  if (ffd->format >= SVN_FS_FS__MIN_PROTOREVS_DIR_FORMAT)
    return svn_dirent_join_many(pool, fs->path, PATH_TXN_PROTOS_DIR,
                                apr_pstrcat(pool, txn_id, PATH_EXT_REV,
                                            (char *)NULL),
                                NULL);
  else
    return svn_dirent_join(path_txn_dir(fs, txn_id, pool), PATH_REV, pool);
}

static APR_INLINE const char *
path_txn_proto_rev_lock(svn_fs_t *fs, const char *txn_id, apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  if (ffd->format >= SVN_FS_FS__MIN_PROTOREVS_DIR_FORMAT)
    return svn_dirent_join_many(pool, fs->path, PATH_TXN_PROTOS_DIR,
                                apr_pstrcat(pool, txn_id, PATH_EXT_REV_LOCK,
                                            (char *)NULL),
                                NULL);
  else
    return svn_dirent_join(path_txn_dir(fs, txn_id, pool), PATH_REV_LOCK,
                           pool);
}

static const char *
path_txn_node_rev(svn_fs_t *fs, const svn_fs_id_t *id, apr_pool_t *pool)
{
  const char *txn_id = svn_fs_fs__id_txn_id(id);
  const char *node_id = svn_fs_fs__id_node_id(id);
  const char *copy_id = svn_fs_fs__id_copy_id(id);
  const char *name = apr_psprintf(pool, PATH_PREFIX_NODE "%s.%s",
                                  node_id, copy_id);

  return svn_dirent_join(path_txn_dir(fs, txn_id, pool), name, pool);
}

static APR_INLINE const char *
path_txn_node_props(svn_fs_t *fs, const svn_fs_id_t *id, apr_pool_t *pool)
{
  return apr_pstrcat(pool, path_txn_node_rev(fs, id, pool), PATH_EXT_PROPS,
                     (char *)NULL);
}

static APR_INLINE const char *
path_txn_node_children(svn_fs_t *fs, const svn_fs_id_t *id, apr_pool_t *pool)
{
  return apr_pstrcat(pool, path_txn_node_rev(fs, id, pool),
                     PATH_EXT_CHILDREN, (char *)NULL);
}

static APR_INLINE const char *
path_node_origin(svn_fs_t *fs, const char *node_id, apr_pool_t *pool)
{
  size_t len = strlen(node_id);
  const char *node_id_minus_last_char =
    (len == 1) ? "0" : apr_pstrmemdup(pool, node_id, len - 1);
  return svn_dirent_join_many(pool, fs->path, PATH_NODE_ORIGINS_DIR,
                              node_id_minus_last_char, NULL);
}

static APR_INLINE const char *
path_and_offset_of(apr_file_t *file, apr_pool_t *pool)
{
  const char *path;
  apr_off_t offset = 0;

  if (apr_file_name_get(&path, file) != APR_SUCCESS)
    path = "(unknown)";

  if (apr_file_seek(file, APR_CUR, &offset) != APR_SUCCESS)
    offset = -1;

  return apr_psprintf(pool, "%s:%" APR_OFF_T_FMT, path, offset);
}



/* Functions for working with shared transaction data. */

/* Return the transaction object for transaction TXN_ID from the
   transaction list of filesystem FS (which must already be locked via the
   txn_list_lock mutex).  If the transaction does not exist in the list,
   then create a new transaction object and return it (if CREATE_NEW is
   true) or return NULL (otherwise). */
static fs_fs_shared_txn_data_t *
get_shared_txn(svn_fs_t *fs, const char *txn_id, svn_boolean_t create_new)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  fs_fs_shared_data_t *ffsd = ffd->shared;
  fs_fs_shared_txn_data_t *txn;

  for (txn = ffsd->txns; txn; txn = txn->next)
    if (strcmp(txn->txn_id, txn_id) == 0)
      break;

  if (txn || !create_new)
    return txn;

  /* Use the transaction object from the (single-object) freelist,
     if one is available, or otherwise create a new object. */
  if (ffsd->free_txn)
    {
      txn = ffsd->free_txn;
      ffsd->free_txn = NULL;
    }
  else
    {
      apr_pool_t *subpool = svn_pool_create(ffsd->common_pool);
      txn = apr_palloc(subpool, sizeof(*txn));
      txn->pool = subpool;
    }

  assert(strlen(txn_id) < sizeof(txn->txn_id));
  apr_cpystrn(txn->txn_id, txn_id, sizeof(txn->txn_id));
  txn->being_written = FALSE;

  /* Link this transaction into the head of the list.  We will typically
     be dealing with only one active transaction at a time, so it makes
     sense for searches through the transaction list to look at the
     newest transactions first.  */
  txn->next = ffsd->txns;
  ffsd->txns = txn;

  return txn;
}

/* Free the transaction object for transaction TXN_ID, and remove it
   from the transaction list of filesystem FS (which must already be
   locked via the txn_list_lock mutex).  Do nothing if the transaction
   does not exist. */
static void
free_shared_txn(svn_fs_t *fs, const char *txn_id)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  fs_fs_shared_data_t *ffsd = ffd->shared;
  fs_fs_shared_txn_data_t *txn, *prev = NULL;

  for (txn = ffsd->txns; txn; prev = txn, txn = txn->next)
    if (strcmp(txn->txn_id, txn_id) == 0)
      break;

  if (!txn)
    return;

  if (prev)
    prev->next = txn->next;
  else
    ffsd->txns = txn->next;

  /* As we typically will be dealing with one transaction after another,
     we will maintain a single-object free list so that we can hopefully
     keep reusing the same transaction object. */
  if (!ffsd->free_txn)
    ffsd->free_txn = txn;
  else
    svn_pool_destroy(txn->pool);
}


/* Obtain a lock on the transaction list of filesystem FS, call BODY
   with FS, BATON, and POOL, and then unlock the transaction list.
   Return what BODY returned. */
static svn_error_t *
with_txnlist_lock(svn_fs_t *fs,
                  svn_error_t *(*body)(svn_fs_t *fs,
                                       const void *baton,
                                       apr_pool_t *pool),
                  const void *baton,
                  apr_pool_t *pool)
{
  svn_error_t *err;
#if APR_HAS_THREADS
  fs_fs_data_t *ffd = fs->fsap_data;
  fs_fs_shared_data_t *ffsd = ffd->shared;
  apr_status_t apr_err;

  apr_err = apr_thread_mutex_lock(ffsd->txn_list_lock);
  if (apr_err)
    return svn_error_wrap_apr(apr_err, _("Can't grab FSFS txn list mutex"));
#endif

  err = body(fs, baton, pool);

#if APR_HAS_THREADS
  apr_err = apr_thread_mutex_unlock(ffsd->txn_list_lock);
  if (apr_err && !err)
    return svn_error_wrap_apr(apr_err, _("Can't ungrab FSFS txn list mutex"));
#endif

  return svn_error_trace(err);
}


/* Get a lock on empty file LOCK_FILENAME, creating it in POOL. */
static svn_error_t *
get_lock_on_filesystem(const char *lock_filename,
                       apr_pool_t *pool)
{
  svn_error_t *err = svn_io_file_lock2(lock_filename, TRUE, FALSE, pool);

  if (err && APR_STATUS_IS_ENOENT(err->apr_err))
    {
      /* No lock file?  No big deal; these are just empty files
         anyway.  Create it and try again. */
      svn_error_clear(err);
      err = NULL;

      SVN_ERR(svn_io_file_create(lock_filename, "", pool));
      SVN_ERR(svn_io_file_lock2(lock_filename, TRUE, FALSE, pool));
    }

  return svn_error_trace(err);
}

/* Obtain a write lock on the file LOCK_FILENAME (protecting with
   LOCK_MUTEX if APR is threaded) in a subpool of POOL, call BODY with
   BATON and that subpool, destroy the subpool (releasing the write
   lock) and return what BODY returned. */
static svn_error_t *
with_some_lock(svn_fs_t *fs,
               svn_error_t *(*body)(void *baton,
                                    apr_pool_t *pool),
               void *baton,
               const char *lock_filename,
#if SVN_FS_FS__USE_LOCK_MUTEX
               apr_thread_mutex_t *lock_mutex,
#endif
               apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_error_t *err;

#if SVN_FS_FS__USE_LOCK_MUTEX
  apr_status_t status;

  /* POSIX fcntl locks are per-process, so we need to serialize locks
     within the process. */
  status = apr_thread_mutex_lock(lock_mutex);
  if (status)
    return svn_error_wrap_apr(status,
                              _("Can't grab FSFS mutex for '%s'"),
                              lock_filename);
#endif

  err = get_lock_on_filesystem(lock_filename, subpool);

  if (!err)
    {
      fs_fs_data_t *ffd = fs->fsap_data;
      if (ffd->format >= SVN_FS_FS__MIN_PACKED_FORMAT)
        SVN_ERR(update_min_unpacked_rev(fs, pool));
#if 0 /* Might be a good idea? */
      SVN_ERR(get_youngest(&ffd->youngest_rev_cache, fs->path,
                           pool));
#endif
      err = body(baton, subpool);
    }

  svn_pool_destroy(subpool);

#if SVN_FS_FS__USE_LOCK_MUTEX
  status = apr_thread_mutex_unlock(lock_mutex);
  if (status && !err)
    return svn_error_wrap_apr(status,
                              _("Can't ungrab FSFS mutex for '%s'"),
                              lock_filename);
#endif

  return svn_error_trace(err);
}

svn_error_t *
svn_fs_fs__with_write_lock(svn_fs_t *fs,
                           svn_error_t *(*body)(void *baton,
                                                apr_pool_t *pool),
                           void *baton,
                           apr_pool_t *pool)
{
#if SVN_FS_FS__USE_LOCK_MUTEX
  fs_fs_data_t *ffd = fs->fsap_data;
  fs_fs_shared_data_t *ffsd = ffd->shared;
  apr_thread_mutex_t *mutex = ffsd->fs_write_lock;
#endif

  return with_some_lock(fs, body, baton,
                        path_lock(fs, pool),
#if SVN_FS_FS__USE_LOCK_MUTEX
                        mutex,
#endif
                        pool);
}

/* Run BODY (with BATON and POOL) while the txn-current file
   of FS is locked. */
static svn_error_t *
with_txn_current_lock(svn_fs_t *fs,
                      svn_error_t *(*body)(void *baton,
                                           apr_pool_t *pool),
                      void *baton,
                      apr_pool_t *pool)
{
#if SVN_FS_FS__USE_LOCK_MUTEX
  fs_fs_data_t *ffd = fs->fsap_data;
  fs_fs_shared_data_t *ffsd = ffd->shared;
  apr_thread_mutex_t *mutex = ffsd->txn_current_lock;
#endif

  return with_some_lock(fs, body, baton,
                        path_txn_current_lock(fs, pool),
#if SVN_FS_FS__USE_LOCK_MUTEX
                        mutex,
#endif
                        pool);
}

/* A structure used by unlock_proto_rev() and unlock_proto_rev_body(),
   which see. */
struct unlock_proto_rev_baton
{
  const char *txn_id;
  void *lockcookie;
};

/* Callback used in the implementation of unlock_proto_rev(). */
static svn_error_t *
unlock_proto_rev_body(svn_fs_t *fs, const void *baton, apr_pool_t *pool)
{
  const struct unlock_proto_rev_baton *b = baton;
  const char *txn_id = b->txn_id;
  apr_file_t *lockfile = b->lockcookie;
  fs_fs_shared_txn_data_t *txn = get_shared_txn(fs, txn_id, FALSE);
  apr_status_t apr_err;

  if (!txn)
    return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                             _("Can't unlock unknown transaction '%s'"),
                             txn_id);
  if (!txn->being_written)
    return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                             _("Can't unlock nonlocked transaction '%s'"),
                             txn_id);

  apr_err = apr_file_unlock(lockfile);
  if (apr_err)
    return svn_error_wrap_apr
      (apr_err,
       _("Can't unlock prototype revision lockfile for transaction '%s'"),
       txn_id);
  apr_err = apr_file_close(lockfile);
  if (apr_err)
    return svn_error_wrap_apr
      (apr_err,
       _("Can't close prototype revision lockfile for transaction '%s'"),
       txn_id);

  txn->being_written = FALSE;

  return SVN_NO_ERROR;
}

/* Unlock the prototype revision file for transaction TXN_ID in filesystem
   FS using cookie LOCKCOOKIE.  The original prototype revision file must
   have been closed _before_ calling this function.

   Perform temporary allocations in POOL. */
static svn_error_t *
unlock_proto_rev(svn_fs_t *fs, const char *txn_id, void *lockcookie,
                 apr_pool_t *pool)
{
  struct unlock_proto_rev_baton b;

  b.txn_id = txn_id;
  b.lockcookie = lockcookie;
  return with_txnlist_lock(fs, unlock_proto_rev_body, &b, pool);
}

/* Same as unlock_proto_rev(), but requires that the transaction list
   lock is already held. */
static svn_error_t *
unlock_proto_rev_list_locked(svn_fs_t *fs, const char *txn_id,
                             void *lockcookie,
                             apr_pool_t *pool)
{
  struct unlock_proto_rev_baton b;

  b.txn_id = txn_id;
  b.lockcookie = lockcookie;
  return unlock_proto_rev_body(fs, &b, pool);
}

/* A structure used by get_writable_proto_rev() and
   get_writable_proto_rev_body(), which see. */
struct get_writable_proto_rev_baton
{
  apr_file_t **file;
  void **lockcookie;
  const char *txn_id;
};

/* Callback used in the implementation of get_writable_proto_rev(). */
static svn_error_t *
get_writable_proto_rev_body(svn_fs_t *fs, const void *baton, apr_pool_t *pool)
{
  const struct get_writable_proto_rev_baton *b = baton;
  apr_file_t **file = b->file;
  void **lockcookie = b->lockcookie;
  const char *txn_id = b->txn_id;
  svn_error_t *err;
  fs_fs_shared_txn_data_t *txn = get_shared_txn(fs, txn_id, TRUE);

  /* First, ensure that no thread in this process (including this one)
     is currently writing to this transaction's proto-rev file. */
  if (txn->being_written)
    return svn_error_createf(SVN_ERR_FS_REP_BEING_WRITTEN, NULL,
                             _("Cannot write to the prototype revision file "
                               "of transaction '%s' because a previous "
                               "representation is currently being written by "
                               "this process"),
                             txn_id);


  /* We know that no thread in this process is writing to the proto-rev
     file, and by extension, that no thread in this process is holding a
     lock on the prototype revision lock file.  It is therefore safe
     for us to attempt to lock this file, to see if any other process
     is holding a lock. */

  {
    apr_file_t *lockfile;
    apr_status_t apr_err;
    const char *lockfile_path = path_txn_proto_rev_lock(fs, txn_id, pool);

    /* Open the proto-rev lockfile, creating it if necessary, as it may
       not exist if the transaction dates from before the lockfiles were
       introduced.

       ### We'd also like to use something like svn_io_file_lock2(), but
           that forces us to create a subpool just to be able to unlock
           the file, which seems a waste. */
    SVN_ERR(svn_io_file_open(&lockfile, lockfile_path,
                             APR_WRITE | APR_CREATE, APR_OS_DEFAULT, pool));

    apr_err = apr_file_lock(lockfile,
                            APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK);
    if (apr_err)
      {
        svn_error_clear(svn_io_file_close(lockfile, pool));

        if (APR_STATUS_IS_EAGAIN(apr_err))
          return svn_error_createf(SVN_ERR_FS_REP_BEING_WRITTEN, NULL,
                                   _("Cannot write to the prototype revision "
                                     "file of transaction '%s' because a "
                                     "previous representation is currently "
                                     "being written by another process"),
                                   txn_id);

        return svn_error_wrap_apr(apr_err,
                                  _("Can't get exclusive lock on file '%s'"),
                                  svn_dirent_local_style(lockfile_path, pool));
      }

    *lockcookie = lockfile;
  }

  /* We've successfully locked the transaction; mark it as such. */
  txn->being_written = TRUE;


  /* Now open the prototype revision file and seek to the end. */
  err = svn_io_file_open(file, path_txn_proto_rev(fs, txn_id, pool),
                         APR_WRITE | APR_BUFFERED, APR_OS_DEFAULT, pool);

  /* You might expect that we could dispense with the following seek
     and achieve the same thing by opening the file using APR_APPEND.
     Unfortunately, APR's buffered file implementation unconditionally
     places its initial file pointer at the start of the file (even for
     files opened with APR_APPEND), so we need this seek to reconcile
     the APR file pointer to the OS file pointer (since we need to be
     able to read the current file position later). */
  if (!err)
    {
      apr_off_t offset = 0;
      err = svn_io_file_seek(*file, APR_END, &offset, pool);
    }

  if (err)
    {
      err = svn_error_compose_create(
              err,
              unlock_proto_rev_list_locked(fs, txn_id, *lockcookie, pool));
      
      *lockcookie = NULL;
    }

  return svn_error_trace(err);
}

/* Get a handle to the prototype revision file for transaction TXN_ID in
   filesystem FS, and lock it for writing.  Return FILE, a file handle
   positioned at the end of the file, and LOCKCOOKIE, a cookie that
   should be passed to unlock_proto_rev() to unlock the file once FILE
   has been closed.

   If the prototype revision file is already locked, return error
   SVN_ERR_FS_REP_BEING_WRITTEN.

   Perform all allocations in POOL. */
static svn_error_t *
get_writable_proto_rev(apr_file_t **file,
                       void **lockcookie,
                       svn_fs_t *fs, const char *txn_id,
                       apr_pool_t *pool)
{
  struct get_writable_proto_rev_baton b;

  b.file = file;
  b.lockcookie = lockcookie;
  b.txn_id = txn_id;

  return with_txnlist_lock(fs, get_writable_proto_rev_body, &b, pool);
}

/* Callback used in the implementation of purge_shared_txn(). */
static svn_error_t *
purge_shared_txn_body(svn_fs_t *fs, const void *baton, apr_pool_t *pool)
{
  const char *txn_id = baton;

  free_shared_txn(fs, txn_id);
  svn_fs_fs__reset_txn_caches(fs);

  return SVN_NO_ERROR;
}

/* Purge the shared data for transaction TXN_ID in filesystem FS.
   Perform all allocations in POOL. */
static svn_error_t *
purge_shared_txn(svn_fs_t *fs, const char *txn_id, apr_pool_t *pool)
{
  return with_txnlist_lock(fs, purge_shared_txn_body, txn_id, pool);
}



/* Fetch the current offset of FILE into *OFFSET_P. */
static svn_error_t *
get_file_offset(apr_off_t *offset_p, apr_file_t *file, apr_pool_t *pool)
{
  apr_off_t offset;

  /* Note that, for buffered files, one (possibly surprising) side-effect
     of this call is to flush any unwritten data to disk. */
  offset = 0;
  SVN_ERR(svn_io_file_seek(file, APR_CUR, &offset, pool));
  *offset_p = offset;

  return SVN_NO_ERROR;
}


/* Check that BUF, a nul-terminated buffer of text from format file PATH,
   contains only digits at OFFSET and beyond, raising an error if not.

   Uses POOL for temporary allocation. */
static svn_error_t *
check_format_file_buffer_numeric(const char *buf, apr_off_t offset,
                                 const char *path, apr_pool_t *pool)
{
  const char *p;

  for (p = buf + offset; *p; p++)
    if (!svn_ctype_isdigit(*p))
      return svn_error_createf(SVN_ERR_BAD_VERSION_FILE_FORMAT, NULL,
        _("Format file '%s' contains unexpected non-digit '%c' within '%s'"),
        svn_dirent_local_style(path, pool), *p, buf);

  return SVN_NO_ERROR;
}

/* Read the format number and maximum number of files per directory
   from PATH and return them in *PFORMAT and *MAX_FILES_PER_DIR
   respectively.

   *MAX_FILES_PER_DIR is obtained from the 'layout' format option, and
   will be set to zero if a linear scheme should be used.

   Use POOL for temporary allocation. */
static svn_error_t *
read_format(int *pformat, int *max_files_per_dir,
            const char *path, apr_pool_t *pool)
{
  svn_error_t *err;
  apr_file_t *file;
  char buf[80];
  apr_size_t len;

  err = svn_io_file_open(&file, path, APR_READ | APR_BUFFERED,
                         APR_OS_DEFAULT, pool);
  if (err && APR_STATUS_IS_ENOENT(err->apr_err))
    {
      /* Treat an absent format file as format 1.  Do not try to
         create the format file on the fly, because the repository
         might be read-only for us, or this might be a read-only
         operation, and the spirit of FSFS is to make no changes
         whatseover in read-only operations.  See thread starting at
         http://subversion.tigris.org/servlets/ReadMsg?list=dev&msgNo=97600
         for more. */
      svn_error_clear(err);
      *pformat = 1;
      *max_files_per_dir = 0;

      return SVN_NO_ERROR;
    }
  SVN_ERR(err);

  len = sizeof(buf);
  err = svn_io_read_length_line(file, buf, &len, pool);
  if (err && APR_STATUS_IS_EOF(err->apr_err))
    {
      /* Return a more useful error message. */
      svn_error_clear(err);
      return svn_error_createf(SVN_ERR_BAD_VERSION_FILE_FORMAT, NULL,
                               _("Can't read first line of format file '%s'"),
                               svn_dirent_local_style(path, pool));
    }
  SVN_ERR(err);

  /* Check that the first line contains only digits. */
  SVN_ERR(check_format_file_buffer_numeric(buf, 0, path, pool));
  SVN_ERR(svn_cstring_atoi(pformat, buf));

  /* Set the default values for anything that can be set via an option. */
  *max_files_per_dir = 0;

  /* Read any options. */
  while (1)
    {
      len = sizeof(buf);
      err = svn_io_read_length_line(file, buf, &len, pool);
      if (err && APR_STATUS_IS_EOF(err->apr_err))
        {
          /* No more options; that's okay. */
          svn_error_clear(err);
          break;
        }
      SVN_ERR(err);

      if (*pformat >= SVN_FS_FS__MIN_LAYOUT_FORMAT_OPTION_FORMAT &&
          strncmp(buf, "layout ", 7) == 0)
        {
          if (strcmp(buf+7, "linear") == 0)
            {
              *max_files_per_dir = 0;
              continue;
            }

          if (strncmp(buf+7, "sharded ", 8) == 0)
            {
              /* Check that the argument is numeric. */
              SVN_ERR(check_format_file_buffer_numeric(buf, 15, path, pool));
              SVN_ERR(svn_cstring_atoi(max_files_per_dir, buf + 15));
              continue;
            }
        }

      return svn_error_createf(SVN_ERR_BAD_VERSION_FILE_FORMAT, NULL,
         _("'%s' contains invalid filesystem format option '%s'"),
         svn_dirent_local_style(path, pool), buf);
    }

  return svn_io_file_close(file, pool);
}

/* Write the format number and maximum number of files per directory
   to a new format file in PATH, possibly expecting to overwrite a
   previously existing file.

   Use POOL for temporary allocation. */
static svn_error_t *
write_format(const char *path, int format, int max_files_per_dir,
             svn_boolean_t overwrite, apr_pool_t *pool)
{
  svn_stringbuf_t *sb;

  SVN_ERR_ASSERT(1 <= format && format <= SVN_FS_FS__FORMAT_NUMBER);

  sb = svn_stringbuf_createf(pool, "%d\n", format);

  if (format >= SVN_FS_FS__MIN_LAYOUT_FORMAT_OPTION_FORMAT)
    {
      if (max_files_per_dir)
        svn_stringbuf_appendcstr(sb, apr_psprintf(pool, "layout sharded %d\n",
                                                  max_files_per_dir));
      else
        svn_stringbuf_appendcstr(sb, "layout linear\n");
    }

  /* svn_io_write_version_file() does a load of magic to allow it to
     replace version files that already exist.  We only need to do
     that when we're allowed to overwrite an existing file. */
  if (! overwrite)
    {
      /* Create the file */
      SVN_ERR(svn_io_file_create(path, sb->data, pool));
    }
  else
    {
      const char *path_tmp;

      SVN_ERR(svn_io_write_unique(&path_tmp,
                                  svn_dirent_dirname(path, pool),
                                  sb->data, sb->len,
                                  svn_io_file_del_none, pool));

      /* rename the temp file as the real destination */
      SVN_ERR(svn_io_file_rename(path_tmp, path, pool));
    }

  /* And set the perms to make it read only */
  return svn_io_set_file_read_only(path, FALSE, pool);
}

/* Return the error SVN_ERR_FS_UNSUPPORTED_FORMAT if FS's format
   number is not the same as a format number supported by this
   Subversion. */
static svn_error_t *
check_format(int format)
{
  /* Blacklist.  These formats may be either younger or older than
     SVN_FS_FS__FORMAT_NUMBER, but we don't support them. */
  if (format == SVN_FS_FS__PACKED_REVPROP_SQLITE_DEV_FORMAT)
    return svn_error_createf(SVN_ERR_FS_UNSUPPORTED_FORMAT, NULL,
                             _("Found format '%d', only created by "
                               "unreleased dev builds; see "
                               "http://subversion.apache.org"
                               "/docs/release-notes/1.7#revprop-packing"),
                             format);

  /* We support all formats from 1-current simultaneously */
  if (1 <= format && format <= SVN_FS_FS__FORMAT_NUMBER)
    return SVN_NO_ERROR;

  return svn_error_createf(SVN_ERR_FS_UNSUPPORTED_FORMAT, NULL,
     _("Expected FS format between '1' and '%d'; found format '%d'"),
     SVN_FS_FS__FORMAT_NUMBER, format);
}

svn_boolean_t
svn_fs_fs__fs_supports_mergeinfo(svn_fs_t *fs)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  return ffd->format >= SVN_FS_FS__MIN_MERGEINFO_FORMAT;
}

static svn_error_t *
read_config(svn_fs_t *fs,
            apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  SVN_ERR(svn_config_read2(&ffd->config,
                           svn_dirent_join(fs->path, PATH_CONFIG, pool),
                           FALSE, FALSE, fs->pool));

  /* Initialize ffd->rep_sharing_allowed. */
  if (ffd->format >= SVN_FS_FS__MIN_REP_SHARING_FORMAT)
    SVN_ERR(svn_config_get_bool(ffd->config, &ffd->rep_sharing_allowed,
                                CONFIG_SECTION_REP_SHARING,
                                CONFIG_OPTION_ENABLE_REP_SHARING, TRUE));
  else
    ffd->rep_sharing_allowed = FALSE;

  return SVN_NO_ERROR;
}

static svn_error_t *
write_config(svn_fs_t *fs,
             apr_pool_t *pool)
{
#define NL APR_EOL_STR
  static const char * const fsfs_conf_contents =
"### This file controls the configuration of the FSFS filesystem."           NL
""                                                                           NL
"[" SVN_CACHE_CONFIG_CATEGORY_MEMCACHED_SERVERS "]"                          NL
"### These options name memcached servers used to cache internal FSFS"       NL
"### data.  See http://www.danga.com/memcached/ for more information on"     NL
"### memcached.  To use memcached with FSFS, run one or more memcached"      NL
"### servers, and specify each of them as an option like so:"                NL
"# first-server = 127.0.0.1:11211"                                           NL
"# remote-memcached = mymemcached.corp.example.com:11212"                    NL
"### The option name is ignored; the value is of the form HOST:PORT."        NL
"### memcached servers can be shared between multiple repositories;"         NL
"### however, if you do this, you *must* ensure that repositories have"      NL
"### distinct UUIDs and paths, or else cached data from one repository"      NL
"### might be used by another accidentally.  Note also that memcached has"   NL
"### no authentication for reads or writes, so you must ensure that your"    NL
"### memcached servers are only accessible by trusted users."                NL
""                                                                           NL
"[" CONFIG_SECTION_CACHES "]"                                                NL
"### When a cache-related error occurs, normally Subversion ignores it"      NL
"### and continues, logging an error if the server is appropriately"         NL
"### configured (and ignoring it with file:// access).  To make"             NL
"### Subversion never ignore cache errors, uncomment this line."             NL
"# " CONFIG_OPTION_FAIL_STOP " = true"                                       NL
""                                                                           NL
"[" CONFIG_SECTION_REP_SHARING "]"                                           NL
"### To conserve space, the filesystem can optionally avoid storing"         NL
"### duplicate representations.  This comes at a slight cost in"             NL
"### performance, as maintaining a database of shared representations can"   NL
"### increase commit times.  The space savings are dependent upon the size"  NL
"### of the repository, the number of objects it contains and the amount of" NL
"### duplication between them, usually a function of the branching and"      NL
"### merging process."                                                       NL
"###"                                                                        NL
"### The following parameter enables rep-sharing in the repository.  It can" NL
"### be switched on and off at will, but for best space-saving results"      NL
"### should be enabled consistently over the life of the repository."        NL
"### rep-sharing is enabled by default."                                     NL
"# " CONFIG_OPTION_ENABLE_REP_SHARING " = true"                              NL

;
#undef NL
  return svn_io_file_create(svn_dirent_join(fs->path, PATH_CONFIG, pool),
                            fsfs_conf_contents, pool);
}

static svn_error_t *
read_min_unpacked_rev(svn_revnum_t *min_unpacked_rev,
                      const char *path,
                      apr_pool_t *pool)
{
  char buf[80];
  apr_file_t *file;
  apr_size_t len;

  SVN_ERR(svn_io_file_open(&file, path, APR_READ | APR_BUFFERED,
                           APR_OS_DEFAULT, pool));
  len = sizeof(buf);
  SVN_ERR(svn_io_read_length_line(file, buf, &len, pool));
  SVN_ERR(svn_io_file_close(file, pool));

  *min_unpacked_rev = SVN_STR_TO_REV(buf);
  return SVN_NO_ERROR;
}

static svn_error_t *
update_min_unpacked_rev(svn_fs_t *fs, apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  SVN_ERR_ASSERT(ffd->format >= SVN_FS_FS__MIN_PACKED_FORMAT);

  return read_min_unpacked_rev(&ffd->min_unpacked_rev,
                               path_min_unpacked_rev(fs, pool),
                               pool);
}

static svn_error_t *
get_youngest(svn_revnum_t *youngest_p, const char *fs_path, apr_pool_t *pool);

svn_error_t *
svn_fs_fs__open(svn_fs_t *fs, const char *path, apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  apr_file_t *uuid_file;
  int format, max_files_per_dir;
  char buf[APR_UUID_FORMATTED_LENGTH + 2];
  apr_size_t limit;

  fs->path = apr_pstrdup(fs->pool, path);

  /* Read the FS format number. */
  SVN_ERR(read_format(&format, &max_files_per_dir,
                      path_format(fs, pool), pool));
  SVN_ERR(check_format(format));

  /* Now we've got a format number no matter what. */
  ffd->format = format;
  ffd->max_files_per_dir = max_files_per_dir;

  /* Read in and cache the repository uuid. */
  SVN_ERR(svn_io_file_open(&uuid_file, path_uuid(fs, pool),
                           APR_READ | APR_BUFFERED, APR_OS_DEFAULT, pool));

  limit = sizeof(buf);
  SVN_ERR(svn_io_read_length_line(uuid_file, buf, &limit, pool));
  ffd->uuid = apr_pstrdup(fs->pool, buf);

  SVN_ERR(svn_io_file_close(uuid_file, pool));

  /* Read the min unpacked revision. */
  if (ffd->format >= SVN_FS_FS__MIN_PACKED_FORMAT)
    SVN_ERR(update_min_unpacked_rev(fs, pool));

  /* Read the configuration file. */
  SVN_ERR(read_config(fs, pool));

  return get_youngest(&(ffd->youngest_rev_cache), path, pool);
}

/* Wrapper around svn_io_file_create which ignores EEXIST. */
static svn_error_t *
create_file_ignore_eexist(const char *file,
                          const char *contents,
                          apr_pool_t *pool)
{
  svn_error_t *err = svn_io_file_create(file, contents, pool);
  if (err && APR_STATUS_IS_EEXIST(err->apr_err))
    {
      svn_error_clear(err);
      err = SVN_NO_ERROR;
    }
  return svn_error_trace(err);
}

static svn_error_t *
upgrade_body(void *baton, apr_pool_t *pool)
{
  svn_fs_t *fs = baton;
  int format, max_files_per_dir;
  const char *format_path = path_format(fs, pool);
  svn_node_kind_t kind;

  /* Read the FS format number and max-files-per-dir setting. */
  SVN_ERR(read_format(&format, &max_files_per_dir, format_path, pool));
  SVN_ERR(check_format(format));

  /* If the config file does not exist, create one. */
  SVN_ERR(svn_io_check_path(svn_dirent_join(fs->path, PATH_CONFIG, pool),
                            &kind, pool));
  switch (kind)
    {
    case svn_node_none:
      SVN_ERR(write_config(fs, pool));
      break;
    case svn_node_file:
      break;
    default:
      return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                               _("'%s' is not a regular file."
                                 " Please move it out of "
                                 "the way and try again"),
                               svn_dirent_join(fs->path, PATH_CONFIG, pool));
    }

  /* If we're already up-to-date, there's nothing else to be done here. */
  if (format == SVN_FS_FS__FORMAT_NUMBER)
    return SVN_NO_ERROR;

  /* If our filesystem predates the existance of the 'txn-current
     file', make that file and its corresponding lock file. */
  if (format < SVN_FS_FS__MIN_TXN_CURRENT_FORMAT)
    {
      SVN_ERR(create_file_ignore_eexist(path_txn_current(fs, pool), "0\n",
                                        pool));
      SVN_ERR(create_file_ignore_eexist(path_txn_current_lock(fs, pool), "",
                                        pool));
    }

  /* If our filesystem predates the existance of the 'txn-protorevs'
     dir, make that directory.  */
  if (format < SVN_FS_FS__MIN_PROTOREVS_DIR_FORMAT)
    {
      /* We don't use path_txn_proto_rev() here because it expects
         we've already bumped our format. */
      SVN_ERR(svn_io_make_dir_recursively(
          svn_dirent_join(fs->path, PATH_TXN_PROTOS_DIR, pool), pool));
    }

  /* If our filesystem is new enough, write the min unpacked rev file. */
  if (format < SVN_FS_FS__MIN_PACKED_FORMAT)
    SVN_ERR(svn_io_file_create(path_min_unpacked_rev(fs, pool), "0\n", pool));

  /* Bump the format file. */
  return write_format(format_path, SVN_FS_FS__FORMAT_NUMBER, max_files_per_dir,
                      TRUE, pool);
}


svn_error_t *
svn_fs_fs__upgrade(svn_fs_t *fs, apr_pool_t *pool)
{
  return svn_fs_fs__with_write_lock(fs, upgrade_body, (void *)fs, pool);
}


/* SVN_ERR-like macros for dealing with recoverable errors on mutable files
 *
 * Revprops, current, and txn-current files are mutable; that is, they
 * change as part of normal fsfs operation, in constrat to revs files, or
 * the format file, which are written once at create (or upgrade) time.
 * When more than one host writes to the same repository, we will
 * sometimes see these recoverable errors when accesssing these files.
 *
 * These errors all relate to NFS, and thus we only use this retry code if
 * ESTALE is defined.
 *
 ** ESTALE
 *
 * In NFS v3 and under, the server doesn't track opened files.  If you
 * unlink(2) or rename(2) a file held open by another process *on the
 * same host*, that host's kernel typically renames the file to
 * .nfsXXXX and automatically deletes that when it's no longer open,
 * but this behavior is not required.
 *
 * For obvious reasons, this does not work *across hosts*.  No one
 * knows about the opened file; not the server, and not the deleting
 * client.  So the file vanishes, and the reader gets stale NFS file
 * handle.
 *
 ** EIO, ENOENT
 *
 * Some client implementations (at least the 2.6.18.5 kernel that ships
 * with Ubuntu Dapper) sometimes give spurious ENOENT (only on open) or
 * even EIO errors when trying to read these files that have been renamed
 * over on some other host.
 *
 ** Solution
 *
 * Wrap opens and reads of such files with RETRY_RECOVERABLE and
 * closes with IGNORE_RECOVERABLE.  Call these macros within a loop of
 * RECOVERABLE_RETRY_COUNT iterations (though, realistically, the
 * second try will succeed).  Make sure you put a break statement
 * after the close, at the end of your loop.  Immediately after your
 * loop, return err if err.
 *
 * You must initialize err to SVN_NO_ERROR and filehandle to NULL, as
 * these macros do not.
 */

#define RECOVERABLE_RETRY_COUNT 10

#ifdef ESTALE
/* Do not use do-while due to the embedded 'continue'.  */
#define RETRY_RECOVERABLE(err, filehandle, expr)                \
  if (1) {                                                      \
    svn_error_clear(err);                                       \
    err = (expr);                                               \
    if (err)                                                    \
      {                                                         \
        apr_status_t _e = APR_TO_OS_ERROR(err->apr_err);        \
        if ((_e == ESTALE) || (_e == EIO) || (_e == ENOENT)) {  \
          if (NULL != filehandle)                               \
            (void)apr_file_close(filehandle);                   \
          continue;                                             \
        }                                                       \
        return svn_error_trace(err);                           \
      }                                                         \
  } else
#define IGNORE_RECOVERABLE(err, expr)                           \
  if (1) {                                                      \
    svn_error_clear(err);                                       \
    err = (expr);                                               \
    if (err)                                                    \
      {                                                         \
        apr_status_t _e = APR_TO_OS_ERROR(err->apr_err);        \
        if ((_e != ESTALE) && (_e != EIO))                      \
          return svn_error_trace(err);                         \
      }                                                         \
  } else
#else
#define RETRY_RECOVERABLE(err, filehandle, expr)  SVN_ERR(expr)
#define IGNORE_RECOVERABLE(err, expr) SVN_ERR(expr)
#endif

/* Long enough to hold: "<svn_revnum_t> <node id> <copy id>\0"
 * 19 bytes for svn_revnum_t (room for 32 or 64 bit values)
 * + 2 spaces
 * + 26 bytes for each id (these are actually unbounded, so we just
 *   have to pick something; 2^64 is 13 bytes in base-36)
 * + 1 terminating null
 */
#define CURRENT_BUF_LEN 48

/* Read the 'current' file FNAME and store the contents in *BUF.
   Allocations are performed in POOL. */
static svn_error_t *
read_current(const char *fname, char **buf, apr_pool_t *pool)
{
  apr_file_t *revision_file = NULL;
  apr_size_t len;
  int i;
  svn_error_t *err = SVN_NO_ERROR;
  apr_pool_t *iterpool;

  *buf = apr_palloc(pool, CURRENT_BUF_LEN);
  iterpool = svn_pool_create(pool);
  for (i = 0; i < RECOVERABLE_RETRY_COUNT; i++)
    {
      svn_pool_clear(iterpool);

      RETRY_RECOVERABLE(err, revision_file,
                        svn_io_file_open(&revision_file, fname,
                                         APR_READ | APR_BUFFERED,
                                         APR_OS_DEFAULT, iterpool));

      len = CURRENT_BUF_LEN;
      RETRY_RECOVERABLE(err, revision_file,
                        svn_io_read_length_line(revision_file,
                                                *buf, &len, iterpool));
      IGNORE_RECOVERABLE(err, svn_io_file_close(revision_file, iterpool));

      break;
    }
  svn_pool_destroy(iterpool);

  return svn_error_trace(err);
}

/* Find the youngest revision in a repository at path FS_PATH and
   return it in *YOUNGEST_P.  Perform temporary allocations in
   POOL. */
static svn_error_t *
get_youngest(svn_revnum_t *youngest_p,
             const char *fs_path,
             apr_pool_t *pool)
{
  char *buf;

  SVN_ERR(read_current(svn_dirent_join(fs_path, PATH_CURRENT, pool),
                       &buf, pool));

  *youngest_p = SVN_STR_TO_REV(buf);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__hotcopy(const char *src_path,
                   const char *dst_path,
                   apr_pool_t *pool)
{
  const char *src_subdir, *dst_subdir;
  svn_revnum_t youngest, rev, min_unpacked_rev;
  apr_pool_t *iterpool;
  svn_node_kind_t kind;
  int format, max_files_per_dir;

  /* Check format to be sure we know how to hotcopy this FS. */
  SVN_ERR(read_format(&format, &max_files_per_dir,
                      svn_dirent_join(src_path, PATH_FORMAT, pool),
                      pool));
  SVN_ERR(check_format(format));

  /* Try to copy the config.
   *
   * ### We try copying the config file before doing anything else,
   * ### because higher layers will abort the hotcopy if we throw
   * ### an error from this function, and that renders the hotcopy
   * ### unusable anyway. */
  if (format >= SVN_FS_FS__MIN_CONFIG_FILE)
    {
      svn_error_t *err;

      err = svn_io_dir_file_copy(src_path, dst_path, PATH_CONFIG, pool);
      if (err)
        {
          if (APR_STATUS_IS_ENOENT(err->apr_err))
            {
              /* 1.6.0 to 1.6.11 did not copy the configuration file during
               * hotcopy. So if we're hotcopying a repository which has been
               * created as a hotcopy itself, it's possible that fsfs.conf
               * does not exist. Ask the user to re-create it.
               *
               * ### It would be nice to make this a non-fatal error,
               * ### but this function does not get an svn_fs_t object
               * ### so we have no way of just printing a warning via
               * ### the fs->warning() callback. */

              const char *msg;
              const char *src_abspath;
              const char *dst_abspath;
              const char *config_relpath;
              svn_error_t *err2;

              config_relpath = svn_dirent_join(src_path, PATH_CONFIG, pool);
              err2 = svn_dirent_get_absolute(&src_abspath, src_path, pool);
              if (err2)
                return svn_error_trace(svn_error_compose_create(err, err2));
              err2 = svn_dirent_get_absolute(&dst_abspath, dst_path, pool);
              if (err2)
                return svn_error_trace(svn_error_compose_create(err, err2));

              /* ### hack: strip off the 'db/' directory from paths so
               * ### they make sense to the user */
              src_abspath = svn_dirent_dirname(src_abspath, pool);
              dst_abspath = svn_dirent_dirname(dst_abspath, pool);

              msg = apr_psprintf(pool,
                                 _("Failed to create hotcopy at '%s'. "
                                   "The file '%s' is missing from the source "
                                   "repository. Please create this file, for "
                                   "instance by running 'svnadmin upgrade %s'"),
                                 dst_abspath, config_relpath, src_abspath);
              return svn_error_quick_wrap(err, msg);
            }
          else
            return svn_error_trace(err);
        }
    }

  /* Copy the 'current' file. */
  SVN_ERR(svn_io_dir_file_copy(src_path, dst_path, PATH_CURRENT, pool));

  /* Copy the uuid. */
  SVN_ERR(svn_io_dir_file_copy(src_path, dst_path, PATH_UUID, pool));

  /* Copy the rep cache before copying the rev files to make sure all
     cached references will be present in the copy. */
  src_subdir = svn_dirent_join(src_path, REP_CACHE_DB_NAME, pool);
  dst_subdir = svn_dirent_join(dst_path, REP_CACHE_DB_NAME, pool);
  SVN_ERR(svn_io_check_path(src_subdir, &kind, pool));
  if (kind == svn_node_file)
    SVN_ERR(svn_sqlite__hotcopy(src_subdir, dst_subdir, pool));

  /* Copy the min unpacked rev, and read its value. */
  if (format >= SVN_FS_FS__MIN_PACKED_FORMAT)
    {
      const char *min_unpacked_rev_path;
      min_unpacked_rev_path = svn_dirent_join(src_path, PATH_MIN_UNPACKED_REV,
                                              pool);

      SVN_ERR(svn_io_dir_file_copy(src_path, dst_path, PATH_MIN_UNPACKED_REV,
                                   pool));
      SVN_ERR(read_min_unpacked_rev(&min_unpacked_rev, min_unpacked_rev_path,
                                    pool));
    }
  else
    {
      min_unpacked_rev = 0;
    }

  /* Find the youngest revision from this 'current' file. */
  SVN_ERR(get_youngest(&youngest, dst_path, pool));

  /* Copy the necessary rev files. */
  src_subdir = svn_dirent_join(src_path, PATH_REVS_DIR, pool);
  dst_subdir = svn_dirent_join(dst_path, PATH_REVS_DIR, pool);

  SVN_ERR(svn_io_make_dir_recursively(dst_subdir, pool));

  iterpool = svn_pool_create(pool);
  /* First, copy packed shards. */
  for (rev = 0; rev < min_unpacked_rev; rev += max_files_per_dir)
    {
      const char *packed_shard = apr_psprintf(iterpool, "%ld.pack",
                                              rev / max_files_per_dir);
      const char *src_subdir_packed_shard;
      src_subdir_packed_shard = svn_dirent_join(src_subdir, packed_shard,
                                                iterpool);

      SVN_ERR(svn_io_copy_dir_recursively(src_subdir_packed_shard,
                                          dst_subdir, packed_shard,
                                          TRUE /* copy_perms */,
                                          NULL /* cancel_func */, NULL,
                                          iterpool));
      svn_pool_clear(iterpool);
    }

  /* Then, copy non-packed shards. */
  SVN_ERR_ASSERT(rev == min_unpacked_rev);
  for (; rev <= youngest; rev++)
    {
      const char *src_subdir_shard = src_subdir,
                 *dst_subdir_shard = dst_subdir;

      if (max_files_per_dir)
        {
          const char *shard = apr_psprintf(iterpool, "%ld",
                                           rev / max_files_per_dir);
          src_subdir_shard = svn_dirent_join(src_subdir, shard, iterpool);
          dst_subdir_shard = svn_dirent_join(dst_subdir, shard, iterpool);

          if (rev % max_files_per_dir == 0)
            {
              SVN_ERR(svn_io_dir_make(dst_subdir_shard, APR_OS_DEFAULT,
                                      iterpool));
              SVN_ERR(svn_io_copy_perms(dst_subdir, dst_subdir_shard,
                                        iterpool));
            }
        }

      SVN_ERR(svn_io_dir_file_copy(src_subdir_shard, dst_subdir_shard,
                                   apr_psprintf(iterpool, "%ld", rev),
                                   iterpool));
      svn_pool_clear(iterpool);
    }

  /* Copy the necessary revprop files. */
  src_subdir = svn_dirent_join(src_path, PATH_REVPROPS_DIR, pool);
  dst_subdir = svn_dirent_join(dst_path, PATH_REVPROPS_DIR, pool);

  SVN_ERR(svn_io_make_dir_recursively(dst_subdir, pool));

  for (rev = 0; rev <= youngest; rev++)
    {
      const char *src_subdir_shard = src_subdir,
                 *dst_subdir_shard = dst_subdir;

      svn_pool_clear(iterpool);

      if (max_files_per_dir)
        {
          const char *shard = apr_psprintf(iterpool, "%ld",
                                           rev / max_files_per_dir);
          src_subdir_shard = svn_dirent_join(src_subdir, shard, iterpool);
          dst_subdir_shard = svn_dirent_join(dst_subdir, shard, iterpool);

          if (rev % max_files_per_dir == 0)
            {
              SVN_ERR(svn_io_dir_make(dst_subdir_shard, APR_OS_DEFAULT,
                                      iterpool));
              SVN_ERR(svn_io_copy_perms(dst_subdir, dst_subdir_shard,
                                        iterpool));
            }
        }

      SVN_ERR(svn_io_dir_file_copy(src_subdir_shard, dst_subdir_shard,
                                   apr_psprintf(iterpool, "%ld", rev),
                                   iterpool));
    }

  svn_pool_destroy(iterpool);

  /* Make an empty transactions directory for now.  Eventually some
     method of copying in progress transactions will need to be
     developed.*/
  dst_subdir = svn_dirent_join(dst_path, PATH_TXNS_DIR, pool);
  SVN_ERR(svn_io_make_dir_recursively(dst_subdir, pool));
  if (format >= SVN_FS_FS__MIN_PROTOREVS_DIR_FORMAT)
    {
      dst_subdir = svn_dirent_join(dst_path, PATH_TXN_PROTOS_DIR, pool);
      SVN_ERR(svn_io_make_dir_recursively(dst_subdir, pool));
    }

  /* Now copy the locks tree. */
  src_subdir = svn_dirent_join(src_path, PATH_LOCKS_DIR, pool);
  SVN_ERR(svn_io_check_path(src_subdir, &kind, pool));
  if (kind == svn_node_dir)
    SVN_ERR(svn_io_copy_dir_recursively(src_subdir, dst_path,
                                        PATH_LOCKS_DIR, TRUE, NULL,
                                        NULL, pool));

  /* Now copy the node-origins cache tree. */
  src_subdir = svn_dirent_join(src_path, PATH_NODE_ORIGINS_DIR, pool);
  SVN_ERR(svn_io_check_path(src_subdir, &kind, pool));
  if (kind == svn_node_dir)
    SVN_ERR(svn_io_copy_dir_recursively(src_subdir, dst_path,
                                        PATH_NODE_ORIGINS_DIR, TRUE, NULL,
                                        NULL, pool));

  /* Copy the txn-current file. */
  if (format >= SVN_FS_FS__MIN_TXN_CURRENT_FORMAT)
    SVN_ERR(svn_io_dir_file_copy(src_path, dst_path, PATH_TXN_CURRENT, pool));

  /* Hotcopied FS is complete. Stamp it with a format file. */
  return write_format(svn_dirent_join(dst_path, PATH_FORMAT, pool),
                      format, max_files_per_dir, FALSE, pool);
}

svn_error_t *
svn_fs_fs__youngest_rev(svn_revnum_t *youngest_p,
                        svn_fs_t *fs,
                        apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  SVN_ERR(get_youngest(youngest_p, fs->path, pool));
  ffd->youngest_rev_cache = *youngest_p;

  return SVN_NO_ERROR;
}

/* Given a revision file FILE that has been pre-positioned at the
   beginning of a Node-Rev header block, read in that header block and
   store it in the apr_hash_t HEADERS.  All allocations will be from
   POOL. */
static svn_error_t * read_header_block(apr_hash_t **headers,
                                       svn_stream_t *stream,
                                       apr_pool_t *pool)
{
  *headers = apr_hash_make(pool);

  while (1)
    {
      svn_stringbuf_t *header_str;
      const char *name, *value;
      apr_size_t i = 0;
      svn_boolean_t eof;

      SVN_ERR(svn_stream_readline(stream, &header_str, "\n", &eof, pool));

      if (eof || header_str->len == 0)
        break; /* end of header block */

      while (header_str->data[i] != ':')
        {
          if (header_str->data[i] == '\0')
            return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                     _("Found malformed header '%s' in "
                                       "revision file"),
                                     header_str->data);
          i++;
        }

      /* Create a 'name' string and point to it. */
      header_str->data[i] = '\0';
      name = header_str->data;

      /* Skip over the NULL byte and the space following it. */
      i += 2;

      if (i > header_str->len)
        {
          /* Restore the original line for the error. */
          i -= 2;
          header_str->data[i] = ':';
          return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                   _("Found malformed header '%s' in "
                                     "revision file"),
                                   header_str->data);
        }

      value = header_str->data + i;

      /* header_str is safely in our pool, so we can use bits of it as
         key and value. */
      apr_hash_set(*headers, name, APR_HASH_KEY_STRING, value);
    }

  return SVN_NO_ERROR;
}

/* Return SVN_ERR_FS_NO_SUCH_REVISION if the given revision is newer
   than the current youngest revision or is simply not a valid
   revision number, else return success.

   FSFS is based around the concept that commits only take effect when
   the number in "current" is bumped.  Thus if there happens to be a rev
   or revprops file installed for a revision higher than the one recorded
   in "current" (because a commit failed between installing the rev file
   and bumping "current", or because an administrator rolled back the
   repository by resetting "current" without deleting rev files, etc), it
   ought to be completely ignored.  This function provides the check
   by which callers can make that decision. */
static svn_error_t *
ensure_revision_exists(svn_fs_t *fs,
                       svn_revnum_t rev,
                       apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  if (! SVN_IS_VALID_REVNUM(rev))
    return svn_error_createf(SVN_ERR_FS_NO_SUCH_REVISION, NULL,
                             _("Invalid revision number '%ld'"), rev);


  /* Did the revision exist the last time we checked the current
     file? */
  if (rev <= ffd->youngest_rev_cache)
    return SVN_NO_ERROR;

  SVN_ERR(get_youngest(&(ffd->youngest_rev_cache), fs->path, pool));

  /* Check again. */
  if (rev <= ffd->youngest_rev_cache)
    return SVN_NO_ERROR;

  return svn_error_createf(SVN_ERR_FS_NO_SUCH_REVISION, NULL,
                           _("No such revision %ld"), rev);
}

/* Open the correct revision file for REV.  If the filesystem FS has
   been packed, *FILE will be set to the packed file; otherwise, set *FILE
   to the revision file for REV.  Return SVN_ERR_FS_NO_SUCH_REVISION if the
   file doesn't exist.

   TODO: Consider returning an indication of whether this is a packed rev
         file, so the caller need not rely on is_packed_rev() which in turn
         relies on the cached FFD->min_unpacked_rev value not having changed
         since the rev file was opened.

   Use POOL for allocations. */
static svn_error_t *
open_pack_or_rev_file(apr_file_t **file,
                      svn_fs_t *fs,
                      svn_revnum_t rev,
                      apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  svn_error_t *err;
  const char *path;
  svn_boolean_t retry = FALSE;

  do
    {
      err = svn_fs_fs__path_rev_absolute(&path, fs, rev, pool);

      /* open the revision file in buffered r/o mode */
      if (! err)
        err = svn_io_file_open(file, path,
                              APR_READ | APR_BUFFERED, APR_OS_DEFAULT, pool);

      if (err && APR_STATUS_IS_ENOENT(err->apr_err)
          && ffd->format >= SVN_FS_FS__MIN_PACKED_FORMAT)
        {
          /* Could not open the file. This may happen if the
           * file once existed but got packed later. */
          svn_error_clear(err);

          /* if that was our 2nd attempt, leave it at that. */
          if (retry)
            return svn_error_createf(SVN_ERR_FS_NO_SUCH_REVISION, NULL,
                                    _("No such revision %ld"), rev);

          /* We failed for the first time. Refresh cache & retry. */
          SVN_ERR(update_min_unpacked_rev(fs, pool));

          retry = TRUE;
        }
      else
        {
          retry = FALSE;
        }
    }
  while (retry);

  return svn_error_trace(err);
}

/* Given REV in FS, set *REV_OFFSET to REV's offset in the packed file.
   Use POOL for temporary allocations. */
static svn_error_t *
get_packed_offset(apr_off_t *rev_offset,
                  svn_fs_t *fs,
                  svn_revnum_t rev,
                  apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  svn_stream_t *manifest_stream;
  svn_boolean_t is_cached;
  svn_revnum_t shard;
  apr_int64_t shard_pos;
  apr_array_header_t *manifest;
  apr_pool_t *iterpool;

  shard = rev / ffd->max_files_per_dir;

  /* position of the shard within the manifest */
  shard_pos = rev % ffd->max_files_per_dir;

  /* fetch exactly that element into *rev_offset, if the manifest is found
     in the cache */
  SVN_ERR(svn_cache__get_partial((void **) rev_offset, &is_cached,
                                 ffd->packed_offset_cache, &shard,
                                 svn_fs_fs__get_sharded_offset, &shard_pos,
                                 pool));

  if (is_cached)
      return SVN_NO_ERROR;

  /* Open the manifest file. */
  SVN_ERR(svn_stream_open_readonly(&manifest_stream,
                                   path_rev_packed(fs, rev, "manifest", pool),
                                   pool, pool));

  /* While we're here, let's just read the entire manifest file into an array,
     so we can cache the entire thing. */
  iterpool = svn_pool_create(pool);
  manifest = apr_array_make(pool, ffd->max_files_per_dir, sizeof(apr_off_t));
  while (1)
    {
      svn_stringbuf_t *sb;
      svn_boolean_t eof;
      apr_int64_t val;
      svn_error_t *err;

      svn_pool_clear(iterpool);
      SVN_ERR(svn_stream_readline(manifest_stream, &sb, "\n", &eof, iterpool));
      if (eof)
        break;

      err = svn_cstring_atoi64(&val, sb->data);
      if (err)
        return svn_error_createf(SVN_ERR_FS_CORRUPT, err,
                                 _("Manifest offset '%s' too large"),
                                 sb->data);
      APR_ARRAY_PUSH(manifest, apr_off_t) = (apr_off_t)val;
    }
  svn_pool_destroy(iterpool);

  *rev_offset = APR_ARRAY_IDX(manifest, rev % ffd->max_files_per_dir,
                              apr_off_t);

  /* Close up shop and cache the array. */
  SVN_ERR(svn_stream_close(manifest_stream));
  return svn_cache__set(ffd->packed_offset_cache, &shard, manifest, pool);
}

/* Open the revision file for revision REV in filesystem FS and store
   the newly opened file in FILE.  Seek to location OFFSET before
   returning.  Perform temporary allocations in POOL. */
static svn_error_t *
open_and_seek_revision(apr_file_t **file,
                       svn_fs_t *fs,
                       svn_revnum_t rev,
                       apr_off_t offset,
                       apr_pool_t *pool)
{
  apr_file_t *rev_file;

  SVN_ERR(ensure_revision_exists(fs, rev, pool));

  SVN_ERR(open_pack_or_rev_file(&rev_file, fs, rev, pool));

  if (is_packed_rev(fs, rev))
    {
      apr_off_t rev_offset;

      SVN_ERR(get_packed_offset(&rev_offset, fs, rev, pool));
      offset += rev_offset;
    }

  SVN_ERR(svn_io_file_seek(rev_file, APR_SET, &offset, pool));

  *file = rev_file;

  return SVN_NO_ERROR;
}

/* Open the representation for a node-revision in transaction TXN_ID
   in filesystem FS and store the newly opened file in FILE.  Seek to
   location OFFSET before returning.  Perform temporary allocations in
   POOL.  Only appropriate for file contents, nor props or directory
   contents. */
static svn_error_t *
open_and_seek_transaction(apr_file_t **file,
                          svn_fs_t *fs,
                          const char *txn_id,
                          representation_t *rep,
                          apr_pool_t *pool)
{
  apr_file_t *rev_file;
  apr_off_t offset;

  SVN_ERR(svn_io_file_open(&rev_file, path_txn_proto_rev(fs, txn_id, pool),
                           APR_READ | APR_BUFFERED, APR_OS_DEFAULT, pool));

  offset = rep->offset;
  SVN_ERR(svn_io_file_seek(rev_file, APR_SET, &offset, pool));

  *file = rev_file;

  return SVN_NO_ERROR;
}

/* Given a node-id ID, and a representation REP in filesystem FS, open
   the correct file and seek to the correction location.  Store this
   file in *FILE_P.  Perform any allocations in POOL. */
static svn_error_t *
open_and_seek_representation(apr_file_t **file_p,
                             svn_fs_t *fs,
                             representation_t *rep,
                             apr_pool_t *pool)
{
  if (! rep->txn_id)
    return open_and_seek_revision(file_p, fs, rep->revision, rep->offset,
                                  pool);
  else
    return open_and_seek_transaction(file_p, fs, rep->txn_id, rep, pool);
}

/* Parse the description of a representation from STRING and store it
   into *REP_P.  If the representation is mutable (the revision is
   given as -1), then use TXN_ID for the representation's txn_id
   field.  If MUTABLE_REP_TRUNCATED is true, then this representation
   is for property or directory contents, and no information will be
   expected except the "-1" revision number for a mutable
   representation.  Allocate *REP_P in POOL. */
static svn_error_t *
read_rep_offsets_body(representation_t **rep_p,
                      char *string,
                      const char *txn_id,
                      svn_boolean_t mutable_rep_truncated,
                      apr_pool_t *pool)
{
  representation_t *rep;
  char *str, *last_str;
  apr_int64_t val;

  rep = apr_pcalloc(pool, sizeof(*rep));
  *rep_p = rep;

  str = apr_strtok(string, " ", &last_str);
  if (str == NULL)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Malformed text representation offset line in node-rev"));


  rep->revision = SVN_STR_TO_REV(str);
  if (rep->revision == SVN_INVALID_REVNUM)
    {
      rep->txn_id = txn_id;
      if (mutable_rep_truncated)
        return SVN_NO_ERROR;
    }

  str = apr_strtok(NULL, " ", &last_str);
  if (str == NULL)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Malformed text representation offset line in node-rev"));

  SVN_ERR(svn_cstring_atoi64(&val, str));
  rep->offset = (apr_off_t)val;

  str = apr_strtok(NULL, " ", &last_str);
  if (str == NULL)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Malformed text representation offset line in node-rev"));

  SVN_ERR(svn_cstring_atoi64(&val, str));
  rep->size = (svn_filesize_t)val;

  str = apr_strtok(NULL, " ", &last_str);
  if (str == NULL)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Malformed text representation offset line in node-rev"));

  SVN_ERR(svn_cstring_atoi64(&val, str));
  rep->expanded_size = (svn_filesize_t)val;

  /* Read in the MD5 hash. */
  str = apr_strtok(NULL, " ", &last_str);
  if ((str == NULL) || (strlen(str) != (APR_MD5_DIGESTSIZE * 2)))
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Malformed text representation offset line in node-rev"));

  SVN_ERR(svn_checksum_parse_hex(&rep->md5_checksum, svn_checksum_md5, str,
                                 pool));

  /* The remaining fields are only used for formats >= 4, so check that. */
  str = apr_strtok(NULL, " ", &last_str);
  if (str == NULL)
    return SVN_NO_ERROR;

  /* Read the SHA1 hash. */
  if (strlen(str) != (APR_SHA1_DIGESTSIZE * 2))
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Malformed text representation offset line in node-rev"));

  SVN_ERR(svn_checksum_parse_hex(&rep->sha1_checksum, svn_checksum_sha1, str,
                                 pool));

  /* Read the uniquifier. */
  str = apr_strtok(NULL, " ", &last_str);
  if (str == NULL)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Malformed text representation offset line in node-rev"));

  rep->uniquifier = apr_pstrdup(pool, str);

  return SVN_NO_ERROR;
}

/* Wrap read_rep_offsets_body(), extracting its TXN_ID from our NODEREV_ID,
   and adding an error message. */
static svn_error_t *
read_rep_offsets(representation_t **rep_p,
                 char *string,
                 const svn_fs_id_t *noderev_id,
                 svn_boolean_t mutable_rep_truncated,
                 apr_pool_t *pool)
{
  svn_error_t *err;
  const char *txn_id;

  if (noderev_id)
    txn_id = svn_fs_fs__id_txn_id(noderev_id);
  else
    txn_id = NULL;

  err = read_rep_offsets_body(rep_p, string, txn_id, mutable_rep_truncated,
                              pool);
  if (err)
    {
      const svn_string_t *id_unparsed = svn_fs_fs__id_unparse(noderev_id, pool);
      const char *where;
      where = apr_psprintf(pool,
                           _("While reading representation offsets "
                             "for node-revision '%s':"),
                           noderev_id ? id_unparsed->data : "(null)");

      return svn_error_quick_wrap(err, where);
    }
  else
    return SVN_NO_ERROR;
}

static svn_error_t *
err_dangling_id(svn_fs_t *fs, const svn_fs_id_t *id)
{
  svn_string_t *id_str = svn_fs_fs__id_unparse(id, fs->pool);
  return svn_error_createf
    (SVN_ERR_FS_ID_NOT_FOUND, 0,
     _("Reference to non-existent node '%s' in filesystem '%s'"),
     id_str->data, fs->path);
}

/* Return a string that uniquely identifies the noderev with the
 * given ID, for use as a cache key.
 */
static const char *
get_noderev_cache_key(const svn_fs_id_t *id, apr_pool_t *pool)
{
  const svn_string_t *id_unparsed = svn_fs_fs__id_unparse(id, pool);
  return id_unparsed->data;
}

/* Look up the NODEREV_P for ID in FS' node revsion cache. If noderev
 * caching has been enabled and the data can be found, IS_CACHED will
 * be set to TRUE. The noderev will be allocated from POOL.
 *
 * Non-permanent ids (e.g. ids within a TXN) will not be cached.
 */
static svn_error_t *
get_cached_node_revision_body(node_revision_t **noderev_p,
                              svn_fs_t *fs,
                              const svn_fs_id_t *id,
                              svn_boolean_t *is_cached,
                              apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  if (! ffd->node_revision_cache || svn_fs_fs__id_txn_id(id))
    *is_cached = FALSE;
  else
    SVN_ERR(svn_cache__get((void **) noderev_p,
                           is_cached,
                           ffd->node_revision_cache,
                           get_noderev_cache_key(id, pool),
                           pool));

  return SVN_NO_ERROR;
}

/* If noderev caching has been enabled, store the NODEREV_P for the given ID
 * in FS' node revsion cache. SCRATCH_POOL is used for temporary allcations.
 *
 * Non-permanent ids (e.g. ids within a TXN) will not be cached.
 */
static svn_error_t *
set_cached_node_revision_body(node_revision_t *noderev_p,
                              svn_fs_t *fs,
                              const svn_fs_id_t *id,
                              apr_pool_t *scratch_pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  if (ffd->node_revision_cache && !svn_fs_fs__id_txn_id(id))
    return svn_cache__set(ffd->node_revision_cache,
                          get_noderev_cache_key(id, scratch_pool),
                          noderev_p,
                          scratch_pool);

  return SVN_NO_ERROR;
}

/* Get the node-revision for the node ID in FS.
   Set *NODEREV_P to the new node-revision structure, allocated in POOL.
   See svn_fs_fs__get_node_revision, which wraps this and adds another
   error. */
static svn_error_t *
get_node_revision_body(node_revision_t **noderev_p,
                       svn_fs_t *fs,
                       const svn_fs_id_t *id,
                       apr_pool_t *pool)
{
  apr_file_t *revision_file;
  svn_error_t *err;
  svn_boolean_t is_cached = FALSE;

  /* First, try a cache lookup. If that succeeds, we are done here. */
  SVN_ERR(get_cached_node_revision_body(noderev_p, fs, id, &is_cached, pool));
  if (is_cached)
    return SVN_NO_ERROR;

  if (svn_fs_fs__id_txn_id(id))
    {
      /* This is a transaction node-rev. */
      err = svn_io_file_open(&revision_file, path_txn_node_rev(fs, id, pool),
                             APR_READ | APR_BUFFERED, APR_OS_DEFAULT, pool);
    }
  else
    {
      /* This is a revision node-rev. */
      err = open_and_seek_revision(&revision_file, fs,
                                   svn_fs_fs__id_rev(id),
                                   svn_fs_fs__id_offset(id),
                                   pool);
    }

  if (err)
    {
      if (APR_STATUS_IS_ENOENT(err->apr_err))
        {
          svn_error_clear(err);
          return svn_error_trace(err_dangling_id(fs, id));
        }

      return svn_error_trace(err);
    }

  SVN_ERR(svn_fs_fs__read_noderev(noderev_p,
                                  svn_stream_from_aprfile2(revision_file, FALSE,
                                                           pool),
                                  pool));

  /* The noderev is not in cache, yet. Add it, if caching has been enabled. */
  return set_cached_node_revision_body(*noderev_p, fs, id, pool);
}

svn_error_t *
svn_fs_fs__read_noderev(node_revision_t **noderev_p,
                        svn_stream_t *stream,
                        apr_pool_t *pool)
{
  apr_hash_t *headers;
  node_revision_t *noderev;
  char *value;
  const char *noderev_id;

  SVN_ERR(read_header_block(&headers, stream, pool));

  noderev = apr_pcalloc(pool, sizeof(*noderev));

  /* Read the node-rev id. */
  value = apr_hash_get(headers, HEADER_ID, APR_HASH_KEY_STRING);
  if (value == NULL)
      /* ### More information: filename/offset coordinates */
      return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                              _("Missing id field in node-rev"));

  SVN_ERR(svn_stream_close(stream));

  noderev->id = svn_fs_fs__id_parse(value, strlen(value), pool);
  noderev_id = value; /* for error messages later */

  /* Read the type. */
  value = apr_hash_get(headers, HEADER_TYPE, APR_HASH_KEY_STRING);

  if ((value == NULL) ||
      (strcmp(value, KIND_FILE) != 0 && strcmp(value, KIND_DIR)))
    /* ### s/kind/type/ */
    return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                             _("Missing kind field in node-rev '%s'"),
                             noderev_id);

  noderev->kind = (strcmp(value, KIND_FILE) == 0) ? svn_node_file
    : svn_node_dir;

  /* Read the 'count' field. */
  value = apr_hash_get(headers, HEADER_COUNT, APR_HASH_KEY_STRING);
  if (value)
    SVN_ERR(svn_cstring_atoi(&noderev->predecessor_count, value));
  else
    noderev->predecessor_count = 0;

  /* Get the properties location. */
  value = apr_hash_get(headers, HEADER_PROPS, APR_HASH_KEY_STRING);
  if (value)
    {
      SVN_ERR(read_rep_offsets(&noderev->prop_rep, value,
                               noderev->id, TRUE, pool));
    }

  /* Get the data location. */
  value = apr_hash_get(headers, HEADER_TEXT, APR_HASH_KEY_STRING);
  if (value)
    {
      SVN_ERR(read_rep_offsets(&noderev->data_rep, value,
                               noderev->id,
                               (noderev->kind == svn_node_dir), pool));
    }

  /* Get the created path. */
  value = apr_hash_get(headers, HEADER_CPATH, APR_HASH_KEY_STRING);
  if (value == NULL)
    {
      return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                               _("Missing cpath field in node-rev '%s'"),
                               noderev_id);
    }
  else
    {
      noderev->created_path = apr_pstrdup(pool, value);
    }

  /* Get the predecessor ID. */
  value = apr_hash_get(headers, HEADER_PRED, APR_HASH_KEY_STRING);
  if (value)
    noderev->predecessor_id = svn_fs_fs__id_parse(value, strlen(value),
                                                  pool);

  /* Get the copyroot. */
  value = apr_hash_get(headers, HEADER_COPYROOT, APR_HASH_KEY_STRING);
  if (value == NULL)
    {
      noderev->copyroot_path = apr_pstrdup(pool, noderev->created_path);
      noderev->copyroot_rev = svn_fs_fs__id_rev(noderev->id);
    }
  else
    {
      char *str, *last_str;

      str = apr_strtok(value, " ", &last_str);
      if (str == NULL)
        return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                 _("Malformed copyroot line in node-rev '%s'"),
                                 noderev_id);

      noderev->copyroot_rev = SVN_STR_TO_REV(str);

      if (last_str == NULL)
        return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                 _("Malformed copyroot line in node-rev '%s'"),
                                 noderev_id);
      noderev->copyroot_path = apr_pstrdup(pool, last_str);
    }

  /* Get the copyfrom. */
  value = apr_hash_get(headers, HEADER_COPYFROM, APR_HASH_KEY_STRING);
  if (value == NULL)
    {
      noderev->copyfrom_path = NULL;
      noderev->copyfrom_rev = SVN_INVALID_REVNUM;
    }
  else
    {
      char *str, *last_str;

      str = apr_strtok(value, " ", &last_str);
      if (str == NULL)
        return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                 _("Malformed copyfrom line in node-rev '%s'"),
                                 noderev_id);

      noderev->copyfrom_rev = SVN_STR_TO_REV(str);

      if (last_str == NULL)
        return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                 _("Malformed copyfrom line in node-rev '%s'"),
                                 noderev_id);
      noderev->copyfrom_path = apr_pstrdup(pool, last_str);
    }

  /* Get whether this is a fresh txn root. */
  value = apr_hash_get(headers, HEADER_FRESHTXNRT, APR_HASH_KEY_STRING);
  noderev->is_fresh_txn_root = (value != NULL);

  /* Get the mergeinfo count. */
  value = apr_hash_get(headers, HEADER_MINFO_CNT, APR_HASH_KEY_STRING);
  if (value)
    SVN_ERR(svn_cstring_atoi64(&noderev->mergeinfo_count, value));
  else
    noderev->mergeinfo_count = 0;

  /* Get whether *this* node has mergeinfo. */
  value = apr_hash_get(headers, HEADER_MINFO_HERE, APR_HASH_KEY_STRING);
  noderev->has_mergeinfo = (value != NULL);

  *noderev_p = noderev;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__get_node_revision(node_revision_t **noderev_p,
                             svn_fs_t *fs,
                             const svn_fs_id_t *id,
                             apr_pool_t *pool)
{
  svn_error_t *err = get_node_revision_body(noderev_p, fs, id, pool);
  if (err && err->apr_err == SVN_ERR_FS_CORRUPT)
    {
      svn_string_t *id_string = svn_fs_fs__id_unparse(id, pool);
      return svn_error_createf(SVN_ERR_FS_CORRUPT, err,
                               "Corrupt node-revision '%s'",
                               id_string->data);
    }
  return svn_error_trace(err);
}


/* Return a formatted string, compatible with filesystem format FORMAT,
   that represents the location of representation REP.  If
   MUTABLE_REP_TRUNCATED is given, the rep is for props or dir contents,
   and only a "-1" revision number will be given for a mutable rep.
   Perform the allocation from POOL.  */
static const char *
representation_string(representation_t *rep,
                      int format,
                      svn_boolean_t mutable_rep_truncated,
                      apr_pool_t *pool)
{
  if (rep->txn_id && mutable_rep_truncated)
    return "-1";

  if (format < SVN_FS_FS__MIN_REP_SHARING_FORMAT || rep->sha1_checksum == NULL)
    return apr_psprintf(pool, "%ld %" APR_OFF_T_FMT " %" SVN_FILESIZE_T_FMT
                        " %" SVN_FILESIZE_T_FMT " %s",
                        rep->revision, rep->offset, rep->size,
                        rep->expanded_size,
                        svn_checksum_to_cstring_display(rep->md5_checksum,
                                                        pool));

  return apr_psprintf(pool, "%ld %" APR_OFF_T_FMT " %" SVN_FILESIZE_T_FMT
                      " %" SVN_FILESIZE_T_FMT " %s %s %s",
                      rep->revision, rep->offset, rep->size,
                      rep->expanded_size,
                      svn_checksum_to_cstring_display(rep->md5_checksum,
                                                      pool),
                      svn_checksum_to_cstring_display(rep->sha1_checksum,
                                                      pool),
                      rep->uniquifier);
}


svn_error_t *
svn_fs_fs__write_noderev(svn_stream_t *outfile,
                         node_revision_t *noderev,
                         int format,
                         svn_boolean_t include_mergeinfo,
                         apr_pool_t *pool)
{
  SVN_ERR(svn_stream_printf(outfile, pool, HEADER_ID ": %s\n",
                            svn_fs_fs__id_unparse(noderev->id,
                                                  pool)->data));

  SVN_ERR(svn_stream_printf(outfile, pool, HEADER_TYPE ": %s\n",
                            (noderev->kind == svn_node_file) ?
                            KIND_FILE : KIND_DIR));

  if (noderev->predecessor_id)
    SVN_ERR(svn_stream_printf(outfile, pool, HEADER_PRED ": %s\n",
                              svn_fs_fs__id_unparse(noderev->predecessor_id,
                                                    pool)->data));

  SVN_ERR(svn_stream_printf(outfile, pool, HEADER_COUNT ": %d\n",
                            noderev->predecessor_count));

  if (noderev->data_rep)
    SVN_ERR(svn_stream_printf(outfile, pool, HEADER_TEXT ": %s\n",
                              representation_string(noderev->data_rep,
                                                    format,
                                                    (noderev->kind
                                                     == svn_node_dir),
                                                    pool)));

  if (noderev->prop_rep)
    SVN_ERR(svn_stream_printf(outfile, pool, HEADER_PROPS ": %s\n",
                              representation_string(noderev->prop_rep, format,
                                                    TRUE, pool)));

  SVN_ERR(svn_stream_printf(outfile, pool, HEADER_CPATH ": %s\n",
                            noderev->created_path));

  if (noderev->copyfrom_path)
    SVN_ERR(svn_stream_printf(outfile, pool, HEADER_COPYFROM ": %ld"
                              " %s\n",
                              noderev->copyfrom_rev,
                              noderev->copyfrom_path));

  if ((noderev->copyroot_rev != svn_fs_fs__id_rev(noderev->id)) ||
      (strcmp(noderev->copyroot_path, noderev->created_path) != 0))
    SVN_ERR(svn_stream_printf(outfile, pool, HEADER_COPYROOT ": %ld"
                              " %s\n",
                              noderev->copyroot_rev,
                              noderev->copyroot_path));

  if (noderev->is_fresh_txn_root)
    SVN_ERR(svn_stream_printf(outfile, pool, HEADER_FRESHTXNRT ": y\n"));

  if (include_mergeinfo)
    {
      if (noderev->mergeinfo_count > 0)
        SVN_ERR(svn_stream_printf(outfile, pool, HEADER_MINFO_CNT ": %"
                                  APR_INT64_T_FMT "\n",
                                  noderev->mergeinfo_count));

      if (noderev->has_mergeinfo)
        SVN_ERR(svn_stream_printf(outfile, pool, HEADER_MINFO_HERE ": y\n"));
    }

  return svn_stream_printf(outfile, pool, "\n");
}

svn_error_t *
svn_fs_fs__put_node_revision(svn_fs_t *fs,
                             const svn_fs_id_t *id,
                             node_revision_t *noderev,
                             svn_boolean_t fresh_txn_root,
                             apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  apr_file_t *noderev_file;
  const char *txn_id = svn_fs_fs__id_txn_id(id);

  noderev->is_fresh_txn_root = fresh_txn_root;

  if (! txn_id)
    return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                             _("Attempted to write to non-transaction '%s'"),
                             svn_fs_fs__id_unparse(id, pool)->data);

  SVN_ERR(svn_io_file_open(&noderev_file, path_txn_node_rev(fs, id, pool),
                           APR_WRITE | APR_CREATE | APR_TRUNCATE
                           | APR_BUFFERED, APR_OS_DEFAULT, pool));

  SVN_ERR(svn_fs_fs__write_noderev(svn_stream_from_aprfile2(noderev_file, TRUE,
                                                            pool),
                                   noderev, ffd->format,
                                   svn_fs_fs__fs_supports_mergeinfo(fs),
                                   pool));

  return svn_io_file_close(noderev_file, pool);
}


/* This structure is used to hold the information associated with a
   REP line. */
struct rep_args
{
  svn_boolean_t is_delta;
  svn_boolean_t is_delta_vs_empty;

  svn_revnum_t base_revision;
  apr_off_t base_offset;
  svn_filesize_t base_length;
};

/* Read the next line from file FILE and parse it as a text
   representation entry.  Return the parsed entry in *REP_ARGS_P.
   Perform all allocations in POOL. */
static svn_error_t *
read_rep_line(struct rep_args **rep_args_p,
              apr_file_t *file,
              apr_pool_t *pool)
{
  char buffer[160];
  apr_size_t limit;
  struct rep_args *rep_args;
  char *str, *last_str;
  apr_int64_t val;

  limit = sizeof(buffer);
  SVN_ERR(svn_io_read_length_line(file, buffer, &limit, pool));

  rep_args = apr_pcalloc(pool, sizeof(*rep_args));
  rep_args->is_delta = FALSE;

  if (strcmp(buffer, REP_PLAIN) == 0)
    {
      *rep_args_p = rep_args;
      return SVN_NO_ERROR;
    }

  if (strcmp(buffer, REP_DELTA) == 0)
    {
      /* This is a delta against the empty stream. */
      rep_args->is_delta = TRUE;
      rep_args->is_delta_vs_empty = TRUE;
      *rep_args_p = rep_args;
      return SVN_NO_ERROR;
    }

  rep_args->is_delta = TRUE;
  rep_args->is_delta_vs_empty = FALSE;

  /* We have hopefully a DELTA vs. a non-empty base revision. */
  str = apr_strtok(buffer, " ", &last_str);
  if (! str || (strcmp(str, REP_DELTA) != 0))
    goto error;

  str = apr_strtok(NULL, " ", &last_str);
  if (! str)
    goto error;
  rep_args->base_revision = SVN_STR_TO_REV(str);

  str = apr_strtok(NULL, " ", &last_str);
  if (! str)
    goto error;
  SVN_ERR(svn_cstring_atoi64(&val, str));
  rep_args->base_offset = (apr_off_t)val;

  str = apr_strtok(NULL, " ", &last_str);
  if (! str)
    goto error;
  SVN_ERR(svn_cstring_atoi64(&val, str));
  rep_args->base_length = (svn_filesize_t)val;

  *rep_args_p = rep_args;
  return SVN_NO_ERROR;

 error:
  return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                           _("Malformed representation header at %s"),
                           path_and_offset_of(file, pool));
}

/* Given a revision file REV_FILE, opened to REV in FS, find the Node-ID
   of the header located at OFFSET and store it in *ID_P.  Allocate
   temporary variables from POOL. */
static svn_error_t *
get_fs_id_at_offset(svn_fs_id_t **id_p,
                    apr_file_t *rev_file,
                    svn_fs_t *fs,
                    svn_revnum_t rev,
                    apr_off_t offset,
                    apr_pool_t *pool)
{
  svn_fs_id_t *id;
  apr_hash_t *headers;
  const char *node_id_str;

  SVN_ERR(svn_io_file_seek(rev_file, APR_SET, &offset, pool));

  SVN_ERR(read_header_block(&headers,
                            svn_stream_from_aprfile2(rev_file, TRUE, pool),
                            pool));

  /* In error messages, the offset is relative to the pack file,
     not to the rev file. */

  node_id_str = apr_hash_get(headers, HEADER_ID, APR_HASH_KEY_STRING);

  if (node_id_str == NULL)
    return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                             _("Missing node-id in node-rev at r%ld "
                             "(offset %s)"),
                             rev,
                             apr_psprintf(pool, "%" APR_OFF_T_FMT, offset));

  id = svn_fs_fs__id_parse(node_id_str, strlen(node_id_str), pool);

  if (id == NULL)
    return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                             _("Corrupt node-id '%s' in node-rev at r%ld "
                               "(offset %s)"),
                             node_id_str, rev,
                             apr_psprintf(pool, "%" APR_OFF_T_FMT, offset));

  *id_p = id;

  /* ### assert that the txn_id is REV/OFFSET ? */

  return SVN_NO_ERROR;
}


/* Given an open revision file REV_FILE in FS for REV, locate the trailer that
   specifies the offset to the root node-id and to the changed path
   information.  Store the root node offset in *ROOT_OFFSET and the
   changed path offset in *CHANGES_OFFSET.  If either of these
   pointers is NULL, do nothing with it.

   If PACKED is true, REV_FILE should be a packed shard file.
   ### There is currently no such parameter.  This function assumes that
       is_packed_rev(FS, REV) will indicate whether REV_FILE is a packed
       file.  Therefore FS->fsap_data->min_unpacked_rev must not have been
       refreshed since REV_FILE was opened if there is a possibility that
       revision REV may have become packed since then.
       TODO: Take an IS_PACKED parameter instead, in order to remove this
       requirement.

   Allocate temporary variables from POOL. */
static svn_error_t *
get_root_changes_offset(apr_off_t *root_offset,
                        apr_off_t *changes_offset,
                        apr_file_t *rev_file,
                        svn_fs_t *fs,
                        svn_revnum_t rev,
                        apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  apr_off_t offset;
  apr_off_t rev_offset;
  char buf[64];
  int i, num_bytes;
  const char *str;
  apr_size_t len;
  apr_seek_where_t seek_relative;

  /* Determine where to seek to in the file.

     If we've got a pack file, we want to seek to the end of the desired
     revision.  But we don't track that, so we seek to the beginning of the
     next revision.

     Unless the next revision is in a different file, in which case, we can
     just seek to the end of the pack file -- just like we do in the
     non-packed case. */
  if (is_packed_rev(fs, rev) && ((rev + 1) % ffd->max_files_per_dir != 0))
    {
      SVN_ERR(get_packed_offset(&offset, fs, rev + 1, pool));
      seek_relative = APR_SET;
    }
  else
    {
      seek_relative = APR_END;
      offset = 0;
    }

  /* Offset of the revision from the start of the pack file, if applicable. */
  if (is_packed_rev(fs, rev))
    SVN_ERR(get_packed_offset(&rev_offset, fs, rev, pool));
  else
    rev_offset = 0;

  /* We will assume that the last line containing the two offsets
     will never be longer than 64 characters. */
  SVN_ERR(svn_io_file_seek(rev_file, seek_relative, &offset, pool));

  offset -= sizeof(buf);
  SVN_ERR(svn_io_file_seek(rev_file, APR_SET, &offset, pool));

  /* Read in this last block, from which we will identify the last line. */
  len = sizeof(buf);
  SVN_ERR(svn_io_file_read(rev_file, buf, &len, pool));

  /* This cast should be safe since the maximum amount read, 64, will
     never be bigger than the size of an int. */
  num_bytes = (int) len;

  /* The last byte should be a newline. */
  if (buf[num_bytes - 1] != '\n')
    {
      return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                               _("Revision file (r%ld) lacks trailing newline"),
                               rev);
    }

  /* Look for the next previous newline. */
  for (i = num_bytes - 2; i >= 0; i--)
    {
      if (buf[i] == '\n')
        break;
    }

  if (i < 0)
    {
      return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                               _("Final line in revision file (r%ld) longer "
                                 "than 64 characters"),
                               rev);
    }

  i++;
  str = &buf[i];

  /* find the next space */
  for ( ; i < (num_bytes - 2) ; i++)
    if (buf[i] == ' ')
      break;

  if (i == (num_bytes - 2))
    return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                             _("Final line in revision file r%ld missing space"),
                             rev);

  if (root_offset)
    {
      apr_int64_t val;

      buf[i] = '\0';
      SVN_ERR(svn_cstring_atoi64(&val, str));
      *root_offset = rev_offset + (apr_off_t)val;
    }

  i++;
  str = &buf[i];

  /* find the next newline */
  for ( ; i < num_bytes; i++)
    if (buf[i] == '\n')
      break;

  if (changes_offset)
    {
      apr_int64_t val;

      buf[i] = '\0';
      SVN_ERR(svn_cstring_atoi64(&val, str));
      *changes_offset = rev_offset + (apr_off_t)val;
    }

  return SVN_NO_ERROR;
}

/* Move a file into place from OLD_FILENAME in the transactions
   directory to its final location NEW_FILENAME in the repository.  On
   Unix, match the permissions of the new file to the permissions of
   PERMS_REFERENCE.  Temporary allocations are from POOL.

   This function almost duplicates svn_io_file_move(), but it tries to
   guarantee a flush. */
static svn_error_t *
move_into_place(const char *old_filename,
                const char *new_filename,
                const char *perms_reference,
                apr_pool_t *pool)
{
  svn_error_t *err;

  SVN_ERR(svn_io_copy_perms(perms_reference, old_filename, pool));

  /* Move the file into place. */
  err = svn_io_file_rename(old_filename, new_filename, pool);
  if (err && APR_STATUS_IS_EXDEV(err->apr_err))
    {
      apr_file_t *file;

      /* Can't rename across devices; fall back to copying. */
      svn_error_clear(err);
      err = SVN_NO_ERROR;
      SVN_ERR(svn_io_copy_file(old_filename, new_filename, TRUE, pool));

      /* Flush the target of the copy to disk. */
      SVN_ERR(svn_io_file_open(&file, new_filename, APR_READ,
                               APR_OS_DEFAULT, pool));
      /* ### BH: Does this really guarantee a flush of the data written
         ### via a completely different handle on all operating systems?
         ###
         ### Maybe we should perform the copy ourselves instead of making
         ### apr do that and flush the real handle? */
      SVN_ERR(svn_io_file_flush_to_disk(file, pool));
      SVN_ERR(svn_io_file_close(file, pool));
    }
  if (err)
    return svn_error_trace(err);

#ifdef __linux__
  {
    /* Linux has the unusual feature that fsync() on a file is not
       enough to ensure that a file's directory entries have been
       flushed to disk; you have to fsync the directory as well.
       On other operating systems, we'd only be asking for trouble
       by trying to open and fsync a directory. */
    const char *dirname;
    apr_file_t *file;

    dirname = svn_dirent_dirname(new_filename, pool);
    SVN_ERR(svn_io_file_open(&file, dirname, APR_READ, APR_OS_DEFAULT,
                             pool));
    SVN_ERR(svn_io_file_flush_to_disk(file, pool));
    SVN_ERR(svn_io_file_close(file, pool));
  }
#endif

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__rev_get_root(svn_fs_id_t **root_id_p,
                        svn_fs_t *fs,
                        svn_revnum_t rev,
                        apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  apr_file_t *revision_file;
  apr_off_t root_offset;
  svn_fs_id_t *root_id = NULL;
  svn_boolean_t is_cached;

  SVN_ERR(ensure_revision_exists(fs, rev, pool));

  SVN_ERR(svn_cache__get((void **) root_id_p, &is_cached,
                         ffd->rev_root_id_cache, &rev, pool));
  if (is_cached)
    return SVN_NO_ERROR;

  SVN_ERR(open_pack_or_rev_file(&revision_file, fs, rev, pool));
  SVN_ERR(get_root_changes_offset(&root_offset, NULL, revision_file, fs, rev,
                                  pool));

  SVN_ERR(get_fs_id_at_offset(&root_id, revision_file, fs, rev,
                              root_offset, pool));

  SVN_ERR(svn_io_file_close(revision_file, pool));

  SVN_ERR(svn_cache__set(ffd->rev_root_id_cache, &rev, root_id, pool));

  *root_id_p = root_id;

  return SVN_NO_ERROR;
}

/* Set the revision property list of revision REV in filesystem FS to
   PROPLIST.  Use POOL for temporary allocations. */
static svn_error_t *
set_revision_proplist(svn_fs_t *fs,
                      svn_revnum_t rev,
                      apr_hash_t *proplist,
                      apr_pool_t *pool)
{
  SVN_ERR(ensure_revision_exists(fs, rev, pool));

  if (1)
    {
      const char *final_path = path_revprops(fs, rev, pool);
      const char *tmp_path;
      const char *perms_reference;
      svn_stream_t *stream;

      /* ### do we have a directory sitting around already? we really shouldn't
         ### have to get the dirname here. */
      SVN_ERR(svn_stream_open_unique(&stream, &tmp_path,
                                     svn_dirent_dirname(final_path, pool),
                                     svn_io_file_del_none, pool, pool));
      SVN_ERR(svn_hash_write2(proplist, stream, SVN_HASH_TERMINATOR, pool));
      SVN_ERR(svn_stream_close(stream));

      /* We use the rev file of this revision as the perms reference,
         because when setting revprops for the first time, the revprop
         file won't exist and therefore can't serve as its own reference.
         (Whereas the rev file should already exist at this point.) */
      SVN_ERR(svn_fs_fs__path_rev_absolute(&perms_reference, fs, rev, pool));
      SVN_ERR(move_into_place(tmp_path, final_path, perms_reference, pool));

      return SVN_NO_ERROR;
    }

  return SVN_NO_ERROR;
}

static svn_error_t *
revision_proplist(apr_hash_t **proplist_p,
                  svn_fs_t *fs,
                  svn_revnum_t rev,
                  apr_pool_t *pool)
{
  apr_hash_t *proplist;

  SVN_ERR(ensure_revision_exists(fs, rev, pool));

  if (1)
    {
      apr_file_t *revprop_file = NULL;
      svn_error_t *err = SVN_NO_ERROR;
      int i;
      apr_pool_t *iterpool;

      proplist = apr_hash_make(pool);
      iterpool = svn_pool_create(pool);
      for (i = 0; i < RECOVERABLE_RETRY_COUNT; i++)
        {
          svn_pool_clear(iterpool);

          /* Clear err here rather than after finding a recoverable error so
           * we can return that error on the last iteration of the loop. */
          svn_error_clear(err);
          err = svn_io_file_open(&revprop_file, path_revprops(fs, rev,
                                                              iterpool),
                                 APR_READ | APR_BUFFERED, APR_OS_DEFAULT,
                                 iterpool);
          if (err)
            {
              if (APR_STATUS_IS_ENOENT(err->apr_err))
                {
                  svn_error_clear(err);
                  return svn_error_createf(SVN_ERR_FS_NO_SUCH_REVISION, NULL,
                                           _("No such revision %ld"), rev);
                }
#ifdef ESTALE
              else if (APR_TO_OS_ERROR(err->apr_err) == ESTALE
                       || APR_TO_OS_ERROR(err->apr_err) == EIO
                       || APR_TO_OS_ERROR(err->apr_err) == ENOENT)
                continue;
#endif
              return svn_error_trace(err);
            }

          SVN_ERR(svn_hash__clear(proplist, iterpool));
          RETRY_RECOVERABLE(err, revprop_file,
                            svn_hash_read2(proplist,
                                           svn_stream_from_aprfile2(
                                                revprop_file, TRUE, iterpool),
                                           SVN_HASH_TERMINATOR, pool));

          IGNORE_RECOVERABLE(err, svn_io_file_close(revprop_file, iterpool));

          break;
        }

      if (err)
        return svn_error_trace(err);
      svn_pool_destroy(iterpool);
    }

  *proplist_p = proplist;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__revision_proplist(apr_hash_t **proplist_p,
                             svn_fs_t *fs,
                             svn_revnum_t rev,
                             apr_pool_t *pool)
{
  SVN_ERR(revision_proplist(proplist_p, fs, rev, pool));

  return SVN_NO_ERROR;
}

/* Represents where in the current svndiff data block each
   representation is. */
struct rep_state
{
  apr_file_t *file;
                    /* The txdelta window cache to use or NULL. */
  svn_cache__t *window_cache;
  apr_off_t start;  /* The starting offset for the raw
                       svndiff/plaintext data minus header. */
  apr_off_t off;    /* The current offset into the file. */
  apr_off_t end;    /* The end offset of the raw data. */
  int ver;          /* If a delta, what svndiff version? */
  int chunk_index;
};

/* See create_rep_state, which wraps this and adds another error. */
static svn_error_t *
create_rep_state_body(struct rep_state **rep_state,
                      struct rep_args **rep_args,
                      representation_t *rep,
                      svn_fs_t *fs,
                      apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  struct rep_state *rs = apr_pcalloc(pool, sizeof(*rs));
  struct rep_args *ra;
  unsigned char buf[4];

  SVN_ERR(open_and_seek_representation(&rs->file, fs, rep, pool));
  rs->window_cache = ffd->txdelta_window_cache;

  SVN_ERR(read_rep_line(&ra, rs->file, pool));
  SVN_ERR(get_file_offset(&rs->start, rs->file, pool));
  rs->off = rs->start;
  rs->end = rs->start + rep->size;
  *rep_state = rs;
  *rep_args = ra;

  if (ra->is_delta == FALSE)
    /* This is a plaintext, so just return the current rep_state. */
    return SVN_NO_ERROR;

  /* We are dealing with a delta, find out what version. */
  SVN_ERR(svn_io_file_read_full2(rs->file, buf, sizeof(buf),
                                 NULL, NULL, pool));
  /* ### Layering violation */
  if (! ((buf[0] == 'S') && (buf[1] == 'V') && (buf[2] == 'N')))
    return svn_error_create
      (SVN_ERR_FS_CORRUPT, NULL,
       _("Malformed svndiff data in representation"));
  rs->ver = buf[3];
  rs->chunk_index = 0;
  rs->off += 4;

  return SVN_NO_ERROR;
}

/* Read the rep args for REP in filesystem FS and create a rep_state
   for reading the representation.  Return the rep_state in *REP_STATE
   and the rep args in *REP_ARGS, both allocated in POOL. */
static svn_error_t *
create_rep_state(struct rep_state **rep_state,
                 struct rep_args **rep_args,
                 representation_t *rep,
                 svn_fs_t *fs,
                 apr_pool_t *pool)
{
  svn_error_t *err = create_rep_state_body(rep_state, rep_args, rep, fs, pool);
  if (err && err->apr_err == SVN_ERR_FS_CORRUPT)
    {
      fs_fs_data_t *ffd = fs->fsap_data;

      /* ### This always returns "-1" for transaction reps, because
         ### this particular bit of code doesn't know if the rep is
         ### stored in the protorev or in the mutable area (for props
         ### or dir contents).  It is pretty rare for FSFS to *read*
         ### from the protorev file, though, so this is probably OK.
         ### And anyone going to debug corruption errors is probably
         ### going to jump straight to this comment anyway! */
      return svn_error_createf(SVN_ERR_FS_CORRUPT, err,
                               "Corrupt representation '%s'",
                               rep 
                               ? representation_string(rep, ffd->format, TRUE,
                                                       pool)
                               : "(null)");
    }
  /* ### Call representation_string() ? */
  return svn_error_trace(err);
}

/* Build an array of rep_state structures in *LIST giving the delta
   reps from first_rep to a plain-text or self-compressed rep.  Set
   *SRC_STATE to the plain-text rep we find at the end of the chain,
   or to NULL if the final delta representation is self-compressed.
   The representation to start from is designated by filesystem FS, id
   ID, and representation REP. */
static svn_error_t *
build_rep_list(apr_array_header_t **list,
               struct rep_state **src_state,
               svn_fs_t *fs,
               representation_t *first_rep,
               apr_pool_t *pool)
{
  representation_t rep;
  struct rep_state *rs;
  struct rep_args *rep_args;

  *list = apr_array_make(pool, 1, sizeof(struct rep_state *));
  rep = *first_rep;

  while (1)
    {
      SVN_ERR(create_rep_state(&rs, &rep_args, &rep, fs, pool));
      if (rep_args->is_delta == FALSE)
        {
          /* This is a plaintext, so just return the current rep_state. */
          *src_state = rs;
          return SVN_NO_ERROR;
        }

      /* Push this rep onto the list.  If it's self-compressed, we're done. */
      APR_ARRAY_PUSH(*list, struct rep_state *) = rs;
      if (rep_args->is_delta_vs_empty)
        {
          *src_state = NULL;
          return SVN_NO_ERROR;
        }

      rep.revision = rep_args->base_revision;
      rep.offset = rep_args->base_offset;
      rep.size = rep_args->base_length;
      rep.txn_id = NULL;
    }
}


struct rep_read_baton
{
  /* The FS from which we're reading. */
  svn_fs_t *fs;

  /* The state of all prior delta representations. */
  apr_array_header_t *rs_list;

  /* The plaintext state, if there is a plaintext. */
  struct rep_state *src_state;

  /* The index of the current delta chunk, if we are reading a delta. */
  int chunk_index;

  /* The buffer where we store undeltified data. */
  char *buf;
  apr_size_t buf_pos;
  apr_size_t buf_len;

  /* A checksum context for summing the data read in order to verify it.
     Note: we don't need to use the sha1 checksum because we're only doing
     data verification, for which md5 is perfectly safe.  */
  svn_checksum_ctx_t *md5_checksum_ctx;

  svn_boolean_t checksum_finalized;

  /* The stored checksum of the representation we are reading, its
     length, and the amount we've read so far.  Some of this
     information is redundant with rs_list and src_state, but it's
     convenient for the checksumming code to have it here. */
  svn_checksum_t *md5_checksum;

  svn_filesize_t len;
  svn_filesize_t off;

  /* The key for the fulltext cache for this rep, if there is a
     fulltext cache. */
  const char *fulltext_cache_key;
  /* The text we've been reading, if we're going to cache it. */
  svn_stringbuf_t *current_fulltext;

  /* Used for temporary allocations during the read. */
  apr_pool_t *pool;

  /* Pool used to store file handles and other data that is persistant
     for the entire stream read. */
  apr_pool_t *filehandle_pool;
};

/* Create a rep_read_baton structure for node revision NODEREV in
   filesystem FS and store it in *RB_P.  If FULLTEXT_CACHE_KEY is not
   NULL, it is the rep's key in the fulltext cache, and a stringbuf
   must be allocated to store the text.  Perform all allocations in
   POOL.  If rep is mutable, it must be for file contents. */
static svn_error_t *
rep_read_get_baton(struct rep_read_baton **rb_p,
                   svn_fs_t *fs,
                   representation_t *rep,
                   const char *fulltext_cache_key,
                   apr_pool_t *pool)
{
  struct rep_read_baton *b;

  b = apr_pcalloc(pool, sizeof(*b));
  b->fs = fs;
  b->chunk_index = 0;
  b->buf = NULL;
  b->md5_checksum_ctx = svn_checksum_ctx_create(svn_checksum_md5, pool);
  b->checksum_finalized = FALSE;
  b->md5_checksum = svn_checksum_dup(rep->md5_checksum, pool);
  b->len = rep->expanded_size ? rep->expanded_size : rep->size;
  b->off = 0;
  b->fulltext_cache_key = fulltext_cache_key;
  b->pool = svn_pool_create(pool);
  b->filehandle_pool = svn_pool_create(pool);

  if (fulltext_cache_key)
    b->current_fulltext = svn_stringbuf_create_ensure
                            ((apr_size_t)b->len,
                             b->filehandle_pool);
  else
    b->current_fulltext = NULL;

  SVN_ERR(build_rep_list(&b->rs_list, &b->src_state, fs, rep,
                         b->filehandle_pool));

  /* Save our output baton. */
  *rb_p = b;

  return SVN_NO_ERROR;
}

/* Combine the name of the rev file in RS with the given OFFSET to form
 * a cache lookup key. Allocations will be made from POOL. */
static const char*
get_window_key(struct rep_state *rs, apr_off_t offset, apr_pool_t *pool)
{
  const char *name;
  const char *last_part;
  const char *name_last;

  /* the rev file name containing the txdelta window.
   * If this fails we are in serious trouble anyways.
   * And if nobody else detects the problems, the file content checksum
   * comparison _will_ find them.
   */
  if (apr_file_name_get(&name, rs->file))
    return "";

  /* Handle packed files as well by scanning backwards until we find the
   * revision or pack number. */
  name_last = name + strlen(name) - 1;
  while (! svn_ctype_isdigit(*name_last))
    --name_last;

  last_part = name_last;
  while (svn_ctype_isdigit(*last_part))
    --last_part;

  /* We must differentiate between packed files (as of today, the number
   * is being followed by a dot) and non-packed files (followed by \0).
   * Otherwise, there might be overlaps in the numbering range if the
   * repo gets packed after caching the txdeltas of non-packed revs.
   * => add the first non-digit char to the packed number. */
  if (name_last[1] != '\0')
    ++name_last;

  /* copy one char MORE than the actual number to mark packed files,
   * i.e. packed revision file content uses different key space then
   * non-packed ones: keys for packed rev file content ends with a dot
   * for non-packed rev files they end with a digit. */
  name = apr_pstrndup(pool, last_part + 1, name_last - last_part);
  return svn_fs_fs__combine_number_and_string(offset, name, pool);
}

/* Read the WINDOW_P for the rep state RS from the current FSFS session's
 * cache. This will be a no-op and IS_CACHED will be set to FALSE if no
 * cache has been given. If a cache is available IS_CACHED will inform
 * the caller about the success of the lookup. Allocations (of the window
 * in particualar) will be made from POOL.
 *
 * If the information could be found, put RS and the position within the
 * rev file into the same state as if the data had just been read from it.
 */
static svn_error_t *
get_cached_window(svn_txdelta_window_t **window_p,
                  struct rep_state *rs,
                  svn_boolean_t *is_cached,
                  apr_pool_t *pool)
{
  if (! rs->window_cache)
    {
      /* txdelta window has not been enabled */
      *is_cached = FALSE;
    }
  else
    {
      /* ask the cache for the desired txdelta window */
      svn_fs_fs__txdelta_cached_window_t *cached_window;
      SVN_ERR(svn_cache__get((void **) &cached_window,
                             is_cached,
                             rs->window_cache,
                             get_window_key(rs, rs->off, pool),
                             pool));

      if (*is_cached)
        {
          /* found it. Pass it back to the caller. */
          *window_p = cached_window->window;

          /* manipulate the RS as if we just read the data */
          rs->chunk_index++;
          rs->off = cached_window->end_offset;

          /* manipulate the rev file as if we just read from it */
          SVN_ERR(svn_io_file_seek(rs->file, APR_SET, &rs->off, pool));
        }
    }

  return SVN_NO_ERROR;
}

/* Store the WINDOW read at OFFSET for the rep state RS in the current
 * FSFS session's cache. This will be a no-op if no cache has been given.
 * Temporary allocations will be made from SCRATCH_POOL. */
static svn_error_t *
set_cached_window(svn_txdelta_window_t *window,
                  struct rep_state *rs,
                  apr_off_t offset,
                  apr_pool_t *scratch_pool)
{
  if (rs->window_cache)
    {
      /* store the window and the first offset _past_ it */
      svn_fs_fs__txdelta_cached_window_t cached_window;
      cached_window.window = window;
      cached_window.end_offset = rs->off;

      /* but key it with the start offset because that is the known state
       * when we will look it up */
      return svn_cache__set(rs->window_cache,
                            get_window_key(rs, offset, scratch_pool),
                            &cached_window,
                            scratch_pool);
    }

  return SVN_NO_ERROR;
}

/* Skip forwards to THIS_CHUNK in REP_STATE and then read the next delta
   window into *NWIN. */
static svn_error_t *
read_window(svn_txdelta_window_t **nwin, int this_chunk, struct rep_state *rs,
            apr_pool_t *pool)
{
  svn_stream_t *stream;
  svn_boolean_t is_cached;
  apr_off_t old_offset;

  SVN_ERR_ASSERT(rs->chunk_index <= this_chunk);

  /* Skip windows to reach the current chunk if we aren't there yet. */
  while (rs->chunk_index < this_chunk)
    {
      SVN_ERR(svn_txdelta_skip_svndiff_window(rs->file, rs->ver, pool));
      rs->chunk_index++;
      SVN_ERR(get_file_offset(&rs->off, rs->file, pool));
      if (rs->off >= rs->end)
        return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                _("Reading one svndiff window read "
                                  "beyond the end of the "
                                  "representation"));
    }

  /* Read the next window. But first, try to find it in the cache. */
  SVN_ERR(get_cached_window(nwin, rs, &is_cached, pool));
  if (is_cached)
    return SVN_NO_ERROR;

  /* Actually read the next window. */
  old_offset = rs->off;
  stream = svn_stream_from_aprfile2(rs->file, TRUE, pool);
  SVN_ERR(svn_txdelta_read_svndiff_window(nwin, stream, rs->ver, pool));
  rs->chunk_index++;
  SVN_ERR(get_file_offset(&rs->off, rs->file, pool));

  if (rs->off > rs->end)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Reading one svndiff window read beyond "
                              "the end of the representation"));

  /* the window has not been cached before, thus cache it now
   * (if caching is used for them at all) */
  return set_cached_window(*nwin, rs, old_offset, pool);
}

/* Get one delta window that is a result of combining all but the last deltas
   from the current desired representation identified in *RB, to its
   final base representation.  Store the window in *RESULT. */
static svn_error_t *
get_combined_window(svn_txdelta_window_t **result,
                    struct rep_read_baton *rb)
{
  apr_pool_t *pool, *new_pool;
  int i;
  svn_txdelta_window_t *window, *nwin;
  struct rep_state *rs;

  SVN_ERR_ASSERT(rb->rs_list->nelts >= 2);

  pool = svn_pool_create(rb->pool);

  /* Read the next window from the original rep. */
  rs = APR_ARRAY_IDX(rb->rs_list, 0, struct rep_state *);
  SVN_ERR(read_window(&window, rb->chunk_index, rs, pool));

  /* Combine in the windows from the other delta reps, if needed. */
  for (i = 1; i < rb->rs_list->nelts - 1; i++)
    {
      if (window->src_ops == 0)
        break;

      rs = APR_ARRAY_IDX(rb->rs_list, i, struct rep_state *);

      SVN_ERR(read_window(&nwin, rb->chunk_index, rs, pool));

      /* Combine this window with the current one.  Cycle pools so that we
         only need to hold three windows at a time. */
      new_pool = svn_pool_create(rb->pool);
      window = svn_txdelta_compose_windows(nwin, window, new_pool);
      svn_pool_destroy(pool);
      pool = new_pool;
    }

  *result = window;
  return SVN_NO_ERROR;
}

/* Returns whether or not the expanded fulltext of the file is cachable
 * based on its size SIZE.  The decision depends on the cache used by RB.
 */
static svn_boolean_t
fulltext_size_is_cachable(fs_fs_data_t *ffd, svn_filesize_t size)
{
  return (size < APR_SIZE_MAX)
      && svn_cache__is_cachable(ffd->fulltext_cache, (apr_size_t)size);
}

/* Close method used on streams returned by read_representation().
 */
static svn_error_t *
rep_read_contents_close(void *baton)
{
  struct rep_read_baton *rb = baton;

  svn_pool_destroy(rb->pool);
  svn_pool_destroy(rb->filehandle_pool);

  return SVN_NO_ERROR;
}

/* Return the next *LEN bytes of the rep and store them in *BUF. */
static svn_error_t *
get_contents(struct rep_read_baton *rb,
             char *buf,
             apr_size_t *len)
{
  apr_size_t copy_len, remaining = *len, tlen;
  char *sbuf, *tbuf, *cur = buf;
  struct rep_state *rs;
  svn_txdelta_window_t *cwindow, *lwindow;

  /* Special case for when there are no delta reps, only a plain
     text. */
  if (rb->rs_list->nelts == 0)
    {
      copy_len = remaining;
      rs = rb->src_state;
      if (((apr_off_t) copy_len) > rs->end - rs->off)
        copy_len = (apr_size_t) (rs->end - rs->off);
      SVN_ERR(svn_io_file_read_full2(rs->file, cur, copy_len, NULL,
                                     NULL, rb->pool));
      rs->off += copy_len;
      *len = copy_len;
      return SVN_NO_ERROR;
    }

  while (remaining > 0)
    {
      /* If we have buffered data from a previous chunk, use that. */
      if (rb->buf)
        {
          /* Determine how much to copy from the buffer. */
          copy_len = rb->buf_len - rb->buf_pos;
          if (copy_len > remaining)
            copy_len = remaining;

          /* Actually copy the data. */
          memcpy(cur, rb->buf + rb->buf_pos, copy_len);
          rb->buf_pos += copy_len;
          cur += copy_len;
          remaining -= copy_len;

          /* If the buffer is all used up, clear it and empty the
             local pool. */
          if (rb->buf_pos == rb->buf_len)
            {
              svn_pool_clear(rb->pool);
              rb->buf = NULL;
            }
        }
      else
        {

          rs = APR_ARRAY_IDX(rb->rs_list, 0, struct rep_state *);
          if (rs->off == rs->end)
            break;

          /* Get more buffered data by evaluating a chunk. */
          if (rb->rs_list->nelts > 1)
            SVN_ERR(get_combined_window(&cwindow, rb));
          else
            cwindow = NULL;
          if (!cwindow || cwindow->src_ops > 0)
            {
              rs = APR_ARRAY_IDX(rb->rs_list, rb->rs_list->nelts - 1,
                                 struct rep_state *);
              /* Read window from last representation in list. */
              /* We apply this window directly instead of combining it
                 with the others.  We do this because vdelta used to
                 be used for deltas against the empty stream, which
                 will trigger quadratic behaviour in the delta
                 combiner.  It's still likely that we'll find such
                 deltas in an old repository; it may be worth
                 considering whether or not this special case is still
                 needed in the future, though. */
              SVN_ERR(read_window(&lwindow, rb->chunk_index, rs, rb->pool));

              if (lwindow->src_ops > 0)
                {
                  if (! rb->src_state)
                    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                            _("svndiff data requested "
                                              "non-existent source"));
                  rs = rb->src_state;
                  sbuf = apr_palloc(rb->pool, lwindow->sview_len);
                  if (! ((rs->start + lwindow->sview_offset) < rs->end))
                    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                            _("svndiff requested position "
                                              "beyond end of stream"));
                  if ((rs->start + lwindow->sview_offset) != rs->off)
                    {
                      rs->off = rs->start + lwindow->sview_offset;
                      SVN_ERR(svn_io_file_seek(rs->file, APR_SET, &rs->off,
                                               rb->pool));
                    }
                  SVN_ERR(svn_io_file_read_full2(rs->file, sbuf,
                                                 lwindow->sview_len,
                                                 NULL, NULL, rb->pool));
                  rs->off += lwindow->sview_len;
                }
              else
                sbuf = NULL;

              /* Apply lwindow to source. */
              tlen = lwindow->tview_len;
              tbuf = apr_palloc(rb->pool, tlen);
              svn_txdelta_apply_instructions(lwindow, sbuf, tbuf,
                                             &tlen);
              if (tlen != lwindow->tview_len)
                return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                        _("svndiff window length is "
                                          "corrupt"));
              sbuf = tbuf;
            }
          else
            sbuf = NULL;

          rb->chunk_index++;

          if (cwindow)
            {
              rb->buf_len = cwindow->tview_len;
              rb->buf = apr_palloc(rb->pool, rb->buf_len);
              svn_txdelta_apply_instructions(cwindow, sbuf, rb->buf,
                                             &rb->buf_len);
              if (rb->buf_len != cwindow->tview_len)
                return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                        _("svndiff window length is "
                                          "corrupt"));
            }
          else
            {
              rb->buf_len = lwindow->tview_len;
              rb->buf = sbuf;
            }

          rb->buf_pos = 0;
        }
    }

  *len = cur - buf;

  return SVN_NO_ERROR;
}

/* BATON is of type `rep_read_baton'; read the next *LEN bytes of the
   representation and store them in *BUF.  Sum as we read and verify
   the MD5 sum at the end. */
static svn_error_t *
rep_read_contents(void *baton,
                  char *buf,
                  apr_size_t *len)
{
  struct rep_read_baton *rb = baton;

  /* Get the next block of data. */
  SVN_ERR(get_contents(rb, buf, len));

  if (rb->current_fulltext)
    svn_stringbuf_appendbytes(rb->current_fulltext, buf, *len);

  /* Perform checksumming.  We want to check the checksum as soon as
     the last byte of data is read, in case the caller never performs
     a short read, but we don't want to finalize the MD5 context
     twice. */
  if (!rb->checksum_finalized)
    {
      SVN_ERR(svn_checksum_update(rb->md5_checksum_ctx, buf, *len));
      rb->off += *len;
      if (rb->off == rb->len)
        {
          svn_checksum_t *md5_checksum;

          rb->checksum_finalized = TRUE;
          SVN_ERR(svn_checksum_final(&md5_checksum, rb->md5_checksum_ctx,
                                     rb->pool));
          if (!svn_checksum_match(md5_checksum, rb->md5_checksum))
            return svn_error_create(SVN_ERR_FS_CORRUPT,
                    svn_checksum_mismatch_err(rb->md5_checksum, md5_checksum,
                        rb->pool,
                        _("Checksum mismatch while reading representation")),
                    NULL);
        }
    }

  if (rb->off == rb->len && rb->current_fulltext)
    {
      fs_fs_data_t *ffd = rb->fs->fsap_data;
      SVN_ERR(svn_cache__set(ffd->fulltext_cache, rb->fulltext_cache_key,
                             rb->current_fulltext, rb->pool));
      rb->current_fulltext = NULL;
    }

  return SVN_NO_ERROR;
}


/* Return a stream in *CONTENTS_P that will read the contents of a
   representation stored at the location given by REP.  Appropriate
   for any kind of immutable representation, but only for file
   contents (not props or directory contents) in mutable
   representations.

   If REP is NULL, the representation is assumed to be empty, and the
   empty stream is returned.
*/
static svn_error_t *
read_representation(svn_stream_t **contents_p,
                    svn_fs_t *fs,
                    representation_t *rep,
                    apr_pool_t *pool)
{
  if (! rep)
    {
      *contents_p = svn_stream_empty(pool);
    }
  else
    {
      fs_fs_data_t *ffd = fs->fsap_data;
      const char *fulltext_key = NULL;
      svn_filesize_t len = rep->expanded_size ? rep->expanded_size : rep->size;
      struct rep_read_baton *rb;

      if (ffd->fulltext_cache && SVN_IS_VALID_REVNUM(rep->revision)
          && fulltext_size_is_cachable(ffd, len))
        {
          svn_string_t *fulltext;
          svn_boolean_t is_cached;
          fulltext_key = apr_psprintf(pool, "%ld/%" APR_OFF_T_FMT,
                                      rep->revision, rep->offset);
          SVN_ERR(svn_cache__get((void **) &fulltext, &is_cached,
                                 ffd->fulltext_cache, fulltext_key, pool));
          if (is_cached)
            {
              *contents_p = svn_stream_from_string(fulltext, pool);
              return SVN_NO_ERROR;
            }
        }

      SVN_ERR(rep_read_get_baton(&rb, fs, rep, fulltext_key, pool));

      *contents_p = svn_stream_create(rb, pool);
      svn_stream_set_read(*contents_p, rep_read_contents);
      svn_stream_set_close(*contents_p, rep_read_contents_close);
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__get_contents(svn_stream_t **contents_p,
                        svn_fs_t *fs,
                        node_revision_t *noderev,
                        apr_pool_t *pool)
{
  return read_representation(contents_p, fs, noderev->data_rep, pool);
}

/* Baton used when reading delta windows. */
struct delta_read_baton
{
  struct rep_state *rs;
  svn_checksum_t *checksum;
};

/* This implements the svn_txdelta_next_window_fn_t interface. */
static svn_error_t *
delta_read_next_window(svn_txdelta_window_t **window, void *baton,
                       apr_pool_t *pool)
{
  struct delta_read_baton *drb = baton;

  if (drb->rs->off == drb->rs->end)
    {
      *window = NULL;
      return SVN_NO_ERROR;
    }

  return read_window(window, drb->rs->chunk_index, drb->rs, pool);
}

/* This implements the svn_txdelta_md5_digest_fn_t interface. */
static const unsigned char *
delta_read_md5_digest(void *baton)
{
  struct delta_read_baton *drb = baton;

  if (drb->checksum->kind == svn_checksum_md5)
    return drb->checksum->digest;
  else
    return NULL;
}

svn_error_t *
svn_fs_fs__get_file_delta_stream(svn_txdelta_stream_t **stream_p,
                                 svn_fs_t *fs,
                                 node_revision_t *source,
                                 node_revision_t *target,
                                 apr_pool_t *pool)
{
  svn_stream_t *source_stream, *target_stream;

  /* Try a shortcut: if the target is stored as a delta against the source,
     then just use that delta. */
  if (source && source->data_rep && target->data_rep)
    {
      struct rep_state *rep_state;
      struct rep_args *rep_args;

      /* Read target's base rep if any. */
      SVN_ERR(create_rep_state(&rep_state, &rep_args, target->data_rep,
                               fs, pool));
      /* If that matches source, then use this delta as is. */
      if (rep_args->is_delta
          && (rep_args->is_delta_vs_empty
              || (rep_args->base_revision == source->data_rep->revision
                  && rep_args->base_offset == source->data_rep->offset)))
        {
          /* Create the delta read baton. */
          struct delta_read_baton *drb = apr_pcalloc(pool, sizeof(*drb));
          drb->rs = rep_state;
          drb->checksum = svn_checksum_dup(target->data_rep->md5_checksum,
                                           pool);
          *stream_p = svn_txdelta_stream_create(drb, delta_read_next_window,
                                                delta_read_md5_digest, pool);
          return SVN_NO_ERROR;
        }
      else
        SVN_ERR(svn_io_file_close(rep_state->file, pool));
    }

  /* Read both fulltexts and construct a delta. */
  if (source)
    SVN_ERR(read_representation(&source_stream, fs, source->data_rep, pool));
  else
    source_stream = svn_stream_empty(pool);
  SVN_ERR(read_representation(&target_stream, fs, target->data_rep, pool));
  svn_txdelta(stream_p, source_stream, target_stream, pool);

  return SVN_NO_ERROR;
}


/* Fetch the contents of a directory into ENTRIES.  Values are stored
   as filename to string mappings; further conversion is necessary to
   convert them into svn_fs_dirent_t values. */
static svn_error_t *
get_dir_contents(apr_hash_t *entries,
                 svn_fs_t *fs,
                 node_revision_t *noderev,
                 apr_pool_t *pool)
{
  svn_stream_t *contents;

  if (noderev->data_rep && noderev->data_rep->txn_id)
    {
      const char *filename = path_txn_node_children(fs, noderev->id, pool);

      /* The representation is mutable.  Read the old directory
         contents from the mutable children file, followed by the
         changes we've made in this transaction. */
      SVN_ERR(svn_stream_open_readonly(&contents, filename, pool, pool));
      SVN_ERR(svn_hash_read2(entries, contents, SVN_HASH_TERMINATOR, pool));
      SVN_ERR(svn_hash_read_incremental(entries, contents, NULL, pool));
      SVN_ERR(svn_stream_close(contents));
    }
  else if (noderev->data_rep)
    {
      /* The representation is immutable.  Read it normally. */
      SVN_ERR(read_representation(&contents, fs, noderev->data_rep, pool));
      SVN_ERR(svn_hash_read2(entries, contents, SVN_HASH_TERMINATOR, pool));
      SVN_ERR(svn_stream_close(contents));
    }

  return SVN_NO_ERROR;
}


static const char *
unparse_dir_entry(svn_node_kind_t kind, const svn_fs_id_t *id,
                  apr_pool_t *pool)
{
  return apr_psprintf(pool, "%s %s",
                      (kind == svn_node_file) ? KIND_FILE : KIND_DIR,
                      svn_fs_fs__id_unparse(id, pool)->data);
}

/* Given a hash ENTRIES of dirent structions, return a hash in
   *STR_ENTRIES_P, that has svn_string_t as the values in the format
   specified by the fs_fs directory contents file.  Perform
   allocations in POOL. */
static svn_error_t *
unparse_dir_entries(apr_hash_t **str_entries_p,
                    apr_hash_t *entries,
                    apr_pool_t *pool)
{
  apr_hash_index_t *hi;

  *str_entries_p = apr_hash_make(pool);

  for (hi = apr_hash_first(pool, entries); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      apr_ssize_t klen;
      svn_fs_dirent_t *dirent = svn__apr_hash_index_val(hi);
      const char *new_val;

      apr_hash_this(hi, &key, &klen, NULL);
      new_val = unparse_dir_entry(dirent->kind, dirent->id, pool);
      apr_hash_set(*str_entries_p, key, klen,
                   svn_string_create(new_val, pool));
    }

  return SVN_NO_ERROR;
}


/* Given a hash STR_ENTRIES with values as svn_string_t as specified
   in an FSFS directory contents listing, return a hash of dirents in
   *ENTRIES_P.  Perform allocations in POOL. */
static svn_error_t *
parse_dir_entries(apr_hash_t **entries_p,
                  apr_hash_t *str_entries,
                  const char *unparsed_id,
                  apr_pool_t *pool)
{
  apr_hash_index_t *hi;

  *entries_p = apr_hash_make(pool);

  /* Translate the string dir entries into real entries. */
  for (hi = apr_hash_first(pool, str_entries); hi; hi = apr_hash_next(hi))
    {
      const char *name = svn__apr_hash_index_key(hi);
      svn_string_t *str_val = svn__apr_hash_index_val(hi);
      char *str, *last_str;
      svn_fs_dirent_t *dirent = apr_pcalloc(pool, sizeof(*dirent));

      str = apr_pstrdup(pool, str_val->data);
      dirent->name = apr_pstrdup(pool, name);

      str = apr_strtok(str, " ", &last_str);
      if (str == NULL)
        return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                 _("Directory entry corrupt in '%s'"),
                                 unparsed_id);

      if (strcmp(str, KIND_FILE) == 0)
        {
          dirent->kind = svn_node_file;
        }
      else if (strcmp(str, KIND_DIR) == 0)
        {
          dirent->kind = svn_node_dir;
        }
      else
        {
          return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                   _("Directory entry corrupt in '%s'"),
                                   unparsed_id);
        }

      str = apr_strtok(NULL, " ", &last_str);
      if (str == NULL)
          return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                   _("Directory entry corrupt in '%s'"),
                                   unparsed_id);

      dirent->id = svn_fs_fs__id_parse(str, strlen(str), pool);

      apr_hash_set(*entries_p, dirent->name, APR_HASH_KEY_STRING, dirent);
    }

  return SVN_NO_ERROR;
}

/* Return the cache object in FS responsible to storing the directory
 * the NODEREV. If none exists, return NULL. */
static svn_cache__t *
locate_dir_cache(svn_fs_t *fs,
                 node_revision_t *noderev)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  return svn_fs_fs__id_txn_id(noderev->id)
      ? ffd->txn_dir_cache
      : ffd->dir_cache;
}

svn_error_t *
svn_fs_fs__rep_contents_dir(apr_hash_t **entries_p,
                            svn_fs_t *fs,
                            node_revision_t *noderev,
                            apr_pool_t *pool)
{
  const char *unparsed_id = NULL;
  apr_hash_t *unparsed_entries, *parsed_entries;

  /* find the cache we may use */
  svn_cache__t *cache = locate_dir_cache(fs, noderev);
  if (cache)
    {
      svn_boolean_t found;

      unparsed_id = svn_fs_fs__id_unparse(noderev->id, pool)->data;
      SVN_ERR(svn_cache__get((void **) entries_p, &found, cache,
                             unparsed_id, pool));
      if (found)
        return SVN_NO_ERROR;
    }

  /* Read in the directory hash. */
  unparsed_entries = apr_hash_make(pool);
  SVN_ERR(get_dir_contents(unparsed_entries, fs, noderev, pool));
  SVN_ERR(parse_dir_entries(&parsed_entries, unparsed_entries,
                            unparsed_id, pool));

  /* Update the cache, if we are to use one. */
  if (cache)
    SVN_ERR(svn_cache__set(cache, unparsed_id, parsed_entries, pool));

  *entries_p = parsed_entries;
  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__rep_contents_dir_entry(svn_fs_dirent_t **dirent,
                                  svn_fs_t *fs,
                                  node_revision_t *noderev,
                                  const char *name,
                                  apr_pool_t *pool)
{
  svn_boolean_t found = FALSE;

  /* find the cache we may use */
  svn_cache__t *cache = locate_dir_cache(fs, noderev);
  if (cache)
    {
      const char *unparsed_id =
        svn_fs_fs__id_unparse(noderev->id, pool)->data;

      /* Cache lookup. */
      SVN_ERR(svn_cache__get_partial((void **)dirent,
                                     &found,
                                     cache,
                                     unparsed_id,
                                     svn_fs_fs__extract_dir_entry,
                                     (void*)name,
                                     pool));
    }

  /* fetch data from disk if we did not find it in the cache */
  if (! found)
    {
      apr_hash_t *entries;
      svn_fs_dirent_t *entry;
      svn_fs_dirent_t *entry_copy = NULL;

      /* since we don't need the directory content later on, put it into
         some sub-pool that will be reclaimed immedeately after exiting
         this function successfully. Opon failure, it will live as long
         as pool.
       */
      apr_pool_t *sub_pool = svn_pool_create(pool);

      /* read the dir from the file system. It will probably be put it
         into the cache for faster lookup in future calls. */
      SVN_ERR(svn_fs_fs__rep_contents_dir(&entries, fs, noderev, sub_pool));

      /* find desired entry and return a copy in POOL, if found */
      entry = apr_hash_get(entries, name, APR_HASH_KEY_STRING);
      if (entry != NULL)
        {
          entry_copy = apr_palloc(pool, sizeof(*entry_copy));
          entry_copy->name = apr_pstrdup(pool, entry->name);
          entry_copy->id = svn_fs_fs__id_copy(entry->id, pool);
          entry_copy->kind = entry->kind;
        }

      *dirent = entry_copy;
      apr_pool_destroy(sub_pool);
    }

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__get_proplist(apr_hash_t **proplist_p,
                        svn_fs_t *fs,
                        node_revision_t *noderev,
                        apr_pool_t *pool)
{
  apr_hash_t *proplist;
  svn_stream_t *stream;

  proplist = apr_hash_make(pool);

  if (noderev->prop_rep && noderev->prop_rep->txn_id)
    {
      const char *filename = path_txn_node_props(fs, noderev->id, pool);

      SVN_ERR(svn_stream_open_readonly(&stream, filename, pool, pool));
      SVN_ERR(svn_hash_read2(proplist, stream, SVN_HASH_TERMINATOR, pool));
      SVN_ERR(svn_stream_close(stream));
    }
  else if (noderev->prop_rep)
    {
      SVN_ERR(read_representation(&stream, fs, noderev->prop_rep, pool));
      SVN_ERR(svn_hash_read2(proplist, stream, SVN_HASH_TERMINATOR, pool));
      SVN_ERR(svn_stream_close(stream));
    }

  *proplist_p = proplist;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__file_length(svn_filesize_t *length,
                       node_revision_t *noderev,
                       apr_pool_t *pool)
{
  if (noderev->data_rep)
    *length = noderev->data_rep->expanded_size;
  else
    *length = 0;

  return SVN_NO_ERROR;
}

svn_boolean_t
svn_fs_fs__noderev_same_rep_key(representation_t *a,
                                representation_t *b)
{
  if (a == b)
    return TRUE;

  if (a == NULL || b == NULL)
    return FALSE;

  if (a->offset != b->offset)
    return FALSE;

  if (a->revision != b->revision)
    return FALSE;

  if (a->uniquifier == b->uniquifier)
    return TRUE;

  if (a->uniquifier == NULL || b->uniquifier == NULL)
    return FALSE;

  return strcmp(a->uniquifier, b->uniquifier) == 0;
}

svn_error_t *
svn_fs_fs__file_checksum(svn_checksum_t **checksum,
                         node_revision_t *noderev,
                         svn_checksum_kind_t kind,
                         apr_pool_t *pool)
{
  if (noderev->data_rep)
    {
      switch(kind)
        {
          case svn_checksum_md5:
            *checksum = svn_checksum_dup(noderev->data_rep->md5_checksum,
                                         pool);
            break;
          case svn_checksum_sha1:
            *checksum = svn_checksum_dup(noderev->data_rep->sha1_checksum,
                                         pool);
            break;
          default:
            *checksum = NULL;
        }
    }
  else
    *checksum = NULL;

  return SVN_NO_ERROR;
}

representation_t *
svn_fs_fs__rep_copy(representation_t *rep,
                    apr_pool_t *pool)
{
  representation_t *rep_new;

  if (rep == NULL)
    return NULL;

  rep_new = apr_pcalloc(pool, sizeof(*rep_new));

  memcpy(rep_new, rep, sizeof(*rep_new));
  rep_new->md5_checksum = svn_checksum_dup(rep->md5_checksum, pool);
  rep_new->sha1_checksum = svn_checksum_dup(rep->sha1_checksum, pool);
  rep_new->uniquifier = apr_pstrdup(pool, rep->uniquifier);

  return rep_new;
}

/* Merge the internal-use-only CHANGE into a hash of public-FS
   svn_fs_path_change2_t CHANGES, collapsing multiple changes into a
   single summarical (is that real word?) change per path.  Also keep
   the COPYFROM_CACHE up to date with new adds and replaces.  */
static svn_error_t *
fold_change(apr_hash_t *changes,
            const change_t *change,
            apr_hash_t *copyfrom_cache)
{
  apr_pool_t *pool = apr_hash_pool_get(changes);
  svn_fs_path_change2_t *old_change, *new_change;
  const char *path;

  if ((old_change = apr_hash_get(changes, change->path, APR_HASH_KEY_STRING)))
    {
      /* This path already exists in the hash, so we have to merge
         this change into the already existing one. */

      /* Sanity check:  only allow NULL node revision ID in the
         `reset' case. */
      if ((! change->noderev_id) && (change->kind != svn_fs_path_change_reset))
        return svn_error_create
          (SVN_ERR_FS_CORRUPT, NULL,
           _("Missing required node revision ID"));

      /* Sanity check: we should be talking about the same node
         revision ID as our last change except where the last change
         was a deletion. */
      if (change->noderev_id
          && (! svn_fs_fs__id_eq(old_change->node_rev_id, change->noderev_id))
          && (old_change->change_kind != svn_fs_path_change_delete))
        return svn_error_create
          (SVN_ERR_FS_CORRUPT, NULL,
           _("Invalid change ordering: new node revision ID "
             "without delete"));

      /* Sanity check: an add, replacement, or reset must be the first
         thing to follow a deletion. */
      if ((old_change->change_kind == svn_fs_path_change_delete)
          && (! ((change->kind == svn_fs_path_change_replace)
                 || (change->kind == svn_fs_path_change_reset)
                 || (change->kind == svn_fs_path_change_add))))
        return svn_error_create
          (SVN_ERR_FS_CORRUPT, NULL,
           _("Invalid change ordering: non-add change on deleted path"));

      /* Sanity check: an add can't follow anything except
         a delete or reset.  */
      if ((change->kind == svn_fs_path_change_add)
          && (old_change->change_kind != svn_fs_path_change_delete)
          && (old_change->change_kind != svn_fs_path_change_reset))
        return svn_error_create
          (SVN_ERR_FS_CORRUPT, NULL,
           _("Invalid change ordering: add change on preexisting path"));

      /* Now, merge that change in. */
      switch (change->kind)
        {
        case svn_fs_path_change_reset:
          /* A reset here will simply remove the path change from the
             hash. */
          old_change = NULL;
          break;

        case svn_fs_path_change_delete:
          if (old_change->change_kind == svn_fs_path_change_add)
            {
              /* If the path was introduced in this transaction via an
                 add, and we are deleting it, just remove the path
                 altogether. */
              old_change = NULL;
            }
          else
            {
              /* A deletion overrules all previous changes. */
              old_change->change_kind = svn_fs_path_change_delete;
              old_change->text_mod = change->text_mod;
              old_change->prop_mod = change->prop_mod;
              old_change->copyfrom_rev = SVN_INVALID_REVNUM;
              old_change->copyfrom_path = NULL;
            }
          break;

        case svn_fs_path_change_add:
        case svn_fs_path_change_replace:
          /* An add at this point must be following a previous delete,
             so treat it just like a replace. */
          old_change->change_kind = svn_fs_path_change_replace;
          old_change->node_rev_id = svn_fs_fs__id_copy(change->noderev_id,
                                                       pool);
          old_change->text_mod = change->text_mod;
          old_change->prop_mod = change->prop_mod;
          if (change->copyfrom_rev == SVN_INVALID_REVNUM)
            {
              old_change->copyfrom_rev = SVN_INVALID_REVNUM;
              old_change->copyfrom_path = NULL;
            }
          else
            {
              old_change->copyfrom_rev = change->copyfrom_rev;
              old_change->copyfrom_path = apr_pstrdup(pool,
                                                      change->copyfrom_path);
            }
          break;

        case svn_fs_path_change_modify:
        default:
          if (change->text_mod)
            old_change->text_mod = TRUE;
          if (change->prop_mod)
            old_change->prop_mod = TRUE;
          break;
        }

      /* Point our new_change to our (possibly modified) old_change. */
      new_change = old_change;
    }
  else
    {
      /* This change is new to the hash, so make a new public change
         structure from the internal one (in the hash's pool), and dup
         the path into the hash's pool, too. */
      new_change = apr_pcalloc(pool, sizeof(*new_change));
      new_change->node_rev_id = svn_fs_fs__id_copy(change->noderev_id, pool);
      new_change->change_kind = change->kind;
      new_change->text_mod = change->text_mod;
      new_change->prop_mod = change->prop_mod;
      /* In FSFS, copyfrom_known is *always* true, since we've always
       * stored copyfroms in changed paths lists. */
      new_change->copyfrom_known = TRUE;
      if (change->copyfrom_rev != SVN_INVALID_REVNUM)
        {
          new_change->copyfrom_rev = change->copyfrom_rev;
          new_change->copyfrom_path = apr_pstrdup(pool, change->copyfrom_path);
        }
      else
        {
          new_change->copyfrom_rev = SVN_INVALID_REVNUM;
          new_change->copyfrom_path = NULL;
        }
    }

  if (new_change)
    new_change->node_kind = change->node_kind;

  /* Add (or update) this path.

     Note: this key might already be present, and it would be nice to
     re-use its value, but there is no way to fetch it. The API makes no
     guarantees that this (new) key will not be retained. Thus, we (again)
     copy the key into the target pool to ensure a proper lifetime.  */
  path = apr_pstrdup(pool, change->path);
  apr_hash_set(changes, path, APR_HASH_KEY_STRING, new_change);

  /* Update the copyfrom cache, if any. */
  if (copyfrom_cache)
    {
      apr_pool_t *copyfrom_pool = apr_hash_pool_get(copyfrom_cache);
      const char *copyfrom_string = NULL, *copyfrom_key = path;
      if (new_change)
        {
          if (SVN_IS_VALID_REVNUM(new_change->copyfrom_rev))
            copyfrom_string = apr_psprintf(copyfrom_pool, "%ld %s",
                                           new_change->copyfrom_rev,
                                           new_change->copyfrom_path);
          else
            copyfrom_string = "";
        }
      /* We need to allocate a copy of the key in the copyfrom_pool if
       * we're not doing a deletion and if it isn't already there. */
      if (copyfrom_string && ! apr_hash_get(copyfrom_cache, copyfrom_key,
                                            APR_HASH_KEY_STRING))
        copyfrom_key = apr_pstrdup(copyfrom_pool, copyfrom_key);
      apr_hash_set(copyfrom_cache, copyfrom_key, APR_HASH_KEY_STRING,
                   copyfrom_string);
    }

  return SVN_NO_ERROR;
}

/* The 256 is an arbitrary size large enough to hold the node id and the
 * various flags. */
#define MAX_CHANGE_LINE_LEN FSFS_MAX_PATH_LEN + 256

/* Read the next entry in the changes record from file FILE and store
   the resulting change in *CHANGE_P.  If there is no next record,
   store NULL there.  Perform all allocations from POOL. */
static svn_error_t *
read_change(change_t **change_p,
            apr_file_t *file,
            apr_pool_t *pool)
{
  char buf[MAX_CHANGE_LINE_LEN];
  apr_size_t len = sizeof(buf);
  change_t *change;
  char *str, *last_str, *kind_str;
  svn_error_t *err;

  /* Default return value. */
  *change_p = NULL;

  err = svn_io_read_length_line(file, buf, &len, pool);

  /* Check for a blank line. */
  if (err || (len == 0))
    {
      if (err && APR_STATUS_IS_EOF(err->apr_err))
        {
          svn_error_clear(err);
          return SVN_NO_ERROR;
        }
      if ((len == 0) && (! err))
        return SVN_NO_ERROR;
      return svn_error_trace(err);
    }

  change = apr_pcalloc(pool, sizeof(*change));

  /* Get the node-id of the change. */
  str = apr_strtok(buf, " ", &last_str);
  if (str == NULL)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Invalid changes line in rev-file"));

  change->noderev_id = svn_fs_fs__id_parse(str, strlen(str), pool);
  if (change->noderev_id == NULL)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Invalid changes line in rev-file"));

  /* Get the change type. */
  str = apr_strtok(NULL, " ", &last_str);
  if (str == NULL)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Invalid changes line in rev-file"));

  /* Don't bother to check the format number before looking for
   * node-kinds: just read them if you find them. */
  change->node_kind = svn_node_unknown;
  kind_str = strchr(str, '-');
  if (kind_str)
    {
      /* Cap off the end of "str" (the action). */
      *kind_str = '\0';
      kind_str++;
      if (strcmp(kind_str, KIND_FILE) == 0)
        change->node_kind = svn_node_file;
      else if (strcmp(kind_str, KIND_DIR) == 0)
        change->node_kind = svn_node_dir;
      else
        return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                _("Invalid changes line in rev-file"));
    }

  if (strcmp(str, ACTION_MODIFY) == 0)
    {
      change->kind = svn_fs_path_change_modify;
    }
  else if (strcmp(str, ACTION_ADD) == 0)
    {
      change->kind = svn_fs_path_change_add;
    }
  else if (strcmp(str, ACTION_DELETE) == 0)
    {
      change->kind = svn_fs_path_change_delete;
    }
  else if (strcmp(str, ACTION_REPLACE) == 0)
    {
      change->kind = svn_fs_path_change_replace;
    }
  else if (strcmp(str, ACTION_RESET) == 0)
    {
      change->kind = svn_fs_path_change_reset;
    }
  else
    {
      return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                              _("Invalid change kind in rev file"));
    }

  /* Get the text-mod flag. */
  str = apr_strtok(NULL, " ", &last_str);
  if (str == NULL)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Invalid changes line in rev-file"));

  if (strcmp(str, FLAG_TRUE) == 0)
    {
      change->text_mod = TRUE;
    }
  else if (strcmp(str, FLAG_FALSE) == 0)
    {
      change->text_mod = FALSE;
    }
  else
    {
      return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                              _("Invalid text-mod flag in rev-file"));
    }

  /* Get the prop-mod flag. */
  str = apr_strtok(NULL, " ", &last_str);
  if (str == NULL)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Invalid changes line in rev-file"));

  if (strcmp(str, FLAG_TRUE) == 0)
    {
      change->prop_mod = TRUE;
    }
  else if (strcmp(str, FLAG_FALSE) == 0)
    {
      change->prop_mod = FALSE;
    }
  else
    {
      return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                              _("Invalid prop-mod flag in rev-file"));
    }

  /* Get the changed path. */
  change->path = apr_pstrdup(pool, last_str);


  /* Read the next line, the copyfrom line. */
  len = sizeof(buf);
  SVN_ERR(svn_io_read_length_line(file, buf, &len, pool));

  if (len == 0)
    {
      change->copyfrom_rev = SVN_INVALID_REVNUM;
      change->copyfrom_path = NULL;
    }
  else
    {
      str = apr_strtok(buf, " ", &last_str);
      if (! str)
        return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                _("Invalid changes line in rev-file"));
      change->copyfrom_rev = SVN_STR_TO_REV(str);

      if (! last_str)
        return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                _("Invalid changes line in rev-file"));

      change->copyfrom_path = apr_pstrdup(pool, last_str);
    }

  *change_p = change;

  return SVN_NO_ERROR;
}

/* Fetch all the changed path entries from FILE and store then in
   *CHANGED_PATHS.  Folding is done to remove redundant or unnecessary
   *data.  Store a hash of paths to copyfrom revisions/paths in
   COPYFROM_HASH if it is non-NULL.  If PREFOLDED is true, assume that
   the changed-path entries have already been folded (by
   write_final_changed_path_info) and may be out of order, so we shouldn't
   remove children of replaced or deleted directories.  Do all
   allocations in POOL. */
static svn_error_t *
fetch_all_changes(apr_hash_t *changed_paths,
                  apr_hash_t *copyfrom_hash,
                  apr_file_t *file,
                  svn_boolean_t prefolded,
                  apr_pool_t *pool)
{
  change_t *change;
  apr_pool_t *iterpool = svn_pool_create(pool);

  /* Read in the changes one by one, folding them into our local hash
     as necessary. */

  SVN_ERR(read_change(&change, file, iterpool));

  while (change)
    {
      SVN_ERR(fold_change(changed_paths, change, copyfrom_hash));

      /* Now, if our change was a deletion or replacement, we have to
         blow away any changes thus far on paths that are (or, were)
         children of this path.
         ### i won't bother with another iteration pool here -- at
         most we talking about a few extra dups of paths into what
         is already a temporary subpool.
      */

      if (((change->kind == svn_fs_path_change_delete)
           || (change->kind == svn_fs_path_change_replace))
          && ! prefolded)
        {
          apr_hash_index_t *hi;

          for (hi = apr_hash_first(iterpool, changed_paths);
               hi;
               hi = apr_hash_next(hi))
            {
              /* KEY is the path. */
              const char *path = svn__apr_hash_index_key(hi);
              apr_ssize_t klen = svn__apr_hash_index_klen(hi);

              /* If we come across our own path, ignore it. */
              if (strcmp(change->path, path) == 0)
                continue;

              /* If we come across a child of our path, remove it. */
              if (svn_dirent_is_child(change->path, path, iterpool))
                apr_hash_set(changed_paths, path, klen, NULL);
            }
        }

      /* Clear the per-iteration subpool. */
      svn_pool_clear(iterpool);

      SVN_ERR(read_change(&change, file, iterpool));
    }

  /* Destroy the per-iteration subpool. */
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__txn_changes_fetch(apr_hash_t **changed_paths_p,
                             svn_fs_t *fs,
                             const char *txn_id,
                             apr_pool_t *pool)
{
  apr_file_t *file;
  apr_hash_t *changed_paths = apr_hash_make(pool);

  SVN_ERR(svn_io_file_open(&file, path_txn_changes(fs, txn_id, pool),
                           APR_READ | APR_BUFFERED, APR_OS_DEFAULT, pool));

  SVN_ERR(fetch_all_changes(changed_paths, NULL, file, FALSE, pool));

  SVN_ERR(svn_io_file_close(file, pool));

  *changed_paths_p = changed_paths;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__paths_changed(apr_hash_t **changed_paths_p,
                         svn_fs_t *fs,
                         svn_revnum_t rev,
                         apr_hash_t *copyfrom_cache,
                         apr_pool_t *pool)
{
  apr_off_t changes_offset;
  apr_hash_t *changed_paths;
  apr_file_t *revision_file;

  SVN_ERR(ensure_revision_exists(fs, rev, pool));

  SVN_ERR(open_pack_or_rev_file(&revision_file, fs, rev, pool));

  SVN_ERR(get_root_changes_offset(NULL, &changes_offset, revision_file, fs,
                                  rev, pool));

  SVN_ERR(svn_io_file_seek(revision_file, APR_SET, &changes_offset, pool));

  changed_paths = apr_hash_make(pool);

  SVN_ERR(fetch_all_changes(changed_paths, copyfrom_cache, revision_file,
                            TRUE, pool));

  /* Close the revision file. */
  SVN_ERR(svn_io_file_close(revision_file, pool));

  *changed_paths_p = changed_paths;

  return SVN_NO_ERROR;
}

/* Copy a revision node-rev SRC into the current transaction TXN_ID in
   the filesystem FS.  This is only used to create the root of a transaction.
   Allocations are from POOL.  */
static svn_error_t *
create_new_txn_noderev_from_rev(svn_fs_t *fs,
                                const char *txn_id,
                                svn_fs_id_t *src,
                                apr_pool_t *pool)
{
  node_revision_t *noderev;
  const char *node_id, *copy_id;

  SVN_ERR(svn_fs_fs__get_node_revision(&noderev, fs, src, pool));

  if (svn_fs_fs__id_txn_id(noderev->id))
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Copying from transactions not allowed"));

  noderev->predecessor_id = noderev->id;
  noderev->predecessor_count++;
  noderev->copyfrom_path = NULL;
  noderev->copyfrom_rev = SVN_INVALID_REVNUM;

  /* For the transaction root, the copyroot never changes. */

  node_id = svn_fs_fs__id_node_id(noderev->id);
  copy_id = svn_fs_fs__id_copy_id(noderev->id);
  noderev->id = svn_fs_fs__id_txn_create(node_id, copy_id, txn_id, pool);

  return svn_fs_fs__put_node_revision(fs, noderev->id, noderev, TRUE, pool);
}

/* A structure used by get_and_increment_txn_key_body(). */
struct get_and_increment_txn_key_baton {
  svn_fs_t *fs;
  char *txn_id;
  apr_pool_t *pool;
};

/* Callback used in the implementation of create_txn_dir().  This gets
   the current base 36 value in PATH_TXN_CURRENT and increments it.
   It returns the original value by the baton. */
static svn_error_t *
get_and_increment_txn_key_body(void *baton, apr_pool_t *pool)
{
  struct get_and_increment_txn_key_baton *cb = baton;
  const char *txn_current_filename = path_txn_current(cb->fs, pool);
  apr_file_t *txn_current_file = NULL;
  const char *tmp_filename;
  char next_txn_id[MAX_KEY_SIZE+3];
  svn_error_t *err = SVN_NO_ERROR;
  apr_pool_t *iterpool;
  apr_size_t len;
  int i;

  cb->txn_id = apr_palloc(cb->pool, MAX_KEY_SIZE);

  iterpool = svn_pool_create(pool);
  for (i = 0; i < RECOVERABLE_RETRY_COUNT; ++i)
    {
      svn_pool_clear(iterpool);

      RETRY_RECOVERABLE(err, txn_current_file,
                        svn_io_file_open(&txn_current_file,
                                         txn_current_filename,
                                         APR_READ | APR_BUFFERED,
                                         APR_OS_DEFAULT, iterpool));
      len = MAX_KEY_SIZE;
      RETRY_RECOVERABLE(err, txn_current_file,
                        svn_io_read_length_line(txn_current_file,
                                                cb->txn_id,
                                                &len,
                                                iterpool));
      IGNORE_RECOVERABLE(err, svn_io_file_close(txn_current_file,
                                                iterpool));

      break;
    }
  SVN_ERR(err);

  svn_pool_destroy(iterpool);

  /* Increment the key and add a trailing \n to the string so the
     txn-current file has a newline in it. */
  svn_fs_fs__next_key(cb->txn_id, &len, next_txn_id);
  next_txn_id[len] = '\n';
  ++len;
  next_txn_id[len] = '\0';

  SVN_ERR(svn_io_write_unique(&tmp_filename,
                              svn_dirent_dirname(txn_current_filename, pool),
                              next_txn_id, len, svn_io_file_del_none, pool));
  SVN_ERR(move_into_place(tmp_filename, txn_current_filename,
                          txn_current_filename, pool));

  return SVN_NO_ERROR;
}

/* Create a unique directory for a transaction in FS based on revision
   REV.  Return the ID for this transaction in *ID_P.  Use a sequence
   value in the transaction ID to prevent reuse of transaction IDs. */
static svn_error_t *
create_txn_dir(const char **id_p, svn_fs_t *fs, svn_revnum_t rev,
               apr_pool_t *pool)
{
  struct get_and_increment_txn_key_baton cb;
  const char *txn_dir;

  /* Get the current transaction sequence value, which is a base-36
     number, from the txn-current file, and write an
     incremented value back out to the file.  Place the revision
     number the transaction is based off into the transaction id. */
  cb.pool = pool;
  cb.fs = fs;
  SVN_ERR(with_txn_current_lock(fs,
                                get_and_increment_txn_key_body,
                                &cb,
                                pool));
  *id_p = apr_psprintf(pool, "%ld-%s", rev, cb.txn_id);

  txn_dir = svn_dirent_join_many(pool,
                                 fs->path,
                                 PATH_TXNS_DIR,
                                 apr_pstrcat(pool, *id_p, PATH_EXT_TXN,
                                             (char *)NULL),
                                 NULL);

  return svn_io_dir_make(txn_dir, APR_OS_DEFAULT, pool);
}

/* Create a unique directory for a transaction in FS based on revision
   REV.  Return the ID for this transaction in *ID_P.  This
   implementation is used in svn 1.4 and earlier repositories and is
   kept in 1.5 and greater to support the --pre-1.4-compatible and
   --pre-1.5-compatible repository creation options.  Reused
   transaction IDs are possible with this implementation. */
static svn_error_t *
create_txn_dir_pre_1_5(const char **id_p, svn_fs_t *fs, svn_revnum_t rev,
                       apr_pool_t *pool)
{
  unsigned int i;
  apr_pool_t *subpool;
  const char *unique_path, *prefix;

  /* Try to create directories named "<txndir>/<rev>-<uniqueifier>.txn". */
  prefix = svn_dirent_join_many(pool, fs->path, PATH_TXNS_DIR,
                                apr_psprintf(pool, "%ld", rev), NULL);

  subpool = svn_pool_create(pool);
  for (i = 1; i <= 99999; i++)
    {
      svn_error_t *err;

      svn_pool_clear(subpool);
      unique_path = apr_psprintf(subpool, "%s-%u" PATH_EXT_TXN, prefix, i);
      err = svn_io_dir_make(unique_path, APR_OS_DEFAULT, subpool);
      if (! err)
        {
          /* We succeeded.  Return the basename minus the ".txn" extension. */
          const char *name = svn_dirent_basename(unique_path, subpool);
          *id_p = apr_pstrndup(pool, name,
                               strlen(name) - strlen(PATH_EXT_TXN));
          svn_pool_destroy(subpool);
          return SVN_NO_ERROR;
        }
      if (! APR_STATUS_IS_EEXIST(err->apr_err))
        return svn_error_trace(err);
      svn_error_clear(err);
    }

  return svn_error_createf(SVN_ERR_IO_UNIQUE_NAMES_EXHAUSTED,
                           NULL,
                           _("Unable to create transaction directory "
                             "in '%s' for revision %ld"),
                           svn_dirent_local_style(fs->path, pool),
                           rev);
}

svn_error_t *
svn_fs_fs__create_txn(svn_fs_txn_t **txn_p,
                      svn_fs_t *fs,
                      svn_revnum_t rev,
                      apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  svn_fs_txn_t *txn;
  svn_fs_id_t *root_id;

  txn = apr_pcalloc(pool, sizeof(*txn));

  /* Get the txn_id. */
  if (ffd->format >= SVN_FS_FS__MIN_TXN_CURRENT_FORMAT)
    SVN_ERR(create_txn_dir(&txn->id, fs, rev, pool));
  else
    SVN_ERR(create_txn_dir_pre_1_5(&txn->id, fs, rev, pool));

  txn->fs = fs;
  txn->base_rev = rev;

  txn->vtable = &txn_vtable;
  *txn_p = txn;

  /* Create a new root node for this transaction. */
  SVN_ERR(svn_fs_fs__rev_get_root(&root_id, fs, rev, pool));
  SVN_ERR(create_new_txn_noderev_from_rev(fs, txn->id, root_id, pool));

  /* Create an empty rev file. */
  SVN_ERR(svn_io_file_create(path_txn_proto_rev(fs, txn->id, pool), "",
                             pool));

  /* Create an empty rev-lock file. */
  SVN_ERR(svn_io_file_create(path_txn_proto_rev_lock(fs, txn->id, pool), "",
                             pool));

  /* Create an empty changes file. */
  SVN_ERR(svn_io_file_create(path_txn_changes(fs, txn->id, pool), "",
                             pool));

  /* Create the next-ids file. */
  return svn_io_file_create(path_txn_next_ids(fs, txn->id, pool), "0 0\n",
                            pool);
}

/* Store the property list for transaction TXN_ID in PROPLIST.
   Perform temporary allocations in POOL. */
static svn_error_t *
get_txn_proplist(apr_hash_t *proplist,
                 svn_fs_t *fs,
                 const char *txn_id,
                 apr_pool_t *pool)
{
  svn_stream_t *stream;

  /* Check for issue #3696. (When we find and fix the cause, we can change
   * this to an assertion.) */
  if (txn_id == NULL)
    return svn_error_create(SVN_ERR_INCORRECT_PARAMS, NULL,
                            _("Internal error: a null transaction id was "
                              "passed to get_txn_proplist()"));

  /* Open the transaction properties file. */
  SVN_ERR(svn_stream_open_readonly(&stream, path_txn_props(fs, txn_id, pool),
                                   pool, pool));

  /* Read in the property list. */
  SVN_ERR(svn_hash_read2(proplist, stream, SVN_HASH_TERMINATOR, pool));

  return svn_stream_close(stream);
}

svn_error_t *
svn_fs_fs__change_txn_prop(svn_fs_txn_t *txn,
                           const char *name,
                           const svn_string_t *value,
                           apr_pool_t *pool)
{
  apr_array_header_t *props = apr_array_make(pool, 1, sizeof(svn_prop_t));
  svn_prop_t prop;

  prop.name = name;
  prop.value = value;
  APR_ARRAY_PUSH(props, svn_prop_t) = prop;

  return svn_fs_fs__change_txn_props(txn, props, pool);
}

svn_error_t *
svn_fs_fs__change_txn_props(svn_fs_txn_t *txn,
                            const apr_array_header_t *props,
                            apr_pool_t *pool)
{
  const char *txn_prop_filename;
  svn_stringbuf_t *buf;
  svn_stream_t *stream;
  apr_hash_t *txn_prop = apr_hash_make(pool);
  int i;
  svn_error_t *err;

  err = get_txn_proplist(txn_prop, txn->fs, txn->id, pool);
  /* Here - and here only - we need to deal with the possibility that the
     transaction property file doesn't yet exist.  The rest of the
     implementation assumes that the file exists, but we're called to set the
     initial transaction properties as the transaction is being created. */
  if (err && (APR_STATUS_IS_ENOENT(err->apr_err)))
    svn_error_clear(err);
  else if (err)
    return svn_error_trace(err);

  for (i = 0; i < props->nelts; i++)
    {
      svn_prop_t *prop = &APR_ARRAY_IDX(props, i, svn_prop_t);

      apr_hash_set(txn_prop, prop->name, APR_HASH_KEY_STRING, prop->value);
    }

  /* Create a new version of the file and write out the new props. */
  /* Open the transaction properties file. */
  buf = svn_stringbuf_create_ensure(1024, pool);
  stream = svn_stream_from_stringbuf(buf, pool);
  SVN_ERR(svn_hash_write2(txn_prop, stream, SVN_HASH_TERMINATOR, pool));
  SVN_ERR(svn_stream_close(stream));
  SVN_ERR(svn_io_write_unique(&txn_prop_filename,
                              path_txn_dir(txn->fs, txn->id, pool),
                              buf->data,
                              buf->len,
                              svn_io_file_del_none,
                              pool));
  return svn_io_file_rename(txn_prop_filename,
                            path_txn_props(txn->fs, txn->id, pool),
                            pool);
}

svn_error_t *
svn_fs_fs__get_txn(transaction_t **txn_p,
                   svn_fs_t *fs,
                   const char *txn_id,
                   apr_pool_t *pool)
{
  transaction_t *txn;
  node_revision_t *noderev;
  svn_fs_id_t *root_id;

  txn = apr_pcalloc(pool, sizeof(*txn));
  txn->proplist = apr_hash_make(pool);

  SVN_ERR(get_txn_proplist(txn->proplist, fs, txn_id, pool));
  root_id = svn_fs_fs__id_txn_create("0", "0", txn_id, pool);

  SVN_ERR(svn_fs_fs__get_node_revision(&noderev, fs, root_id, pool));

  txn->root_id = svn_fs_fs__id_copy(noderev->id, pool);
  txn->base_id = svn_fs_fs__id_copy(noderev->predecessor_id, pool);
  txn->copies = NULL;

  *txn_p = txn;

  return SVN_NO_ERROR;
}

/* Write out the currently available next node_id NODE_ID and copy_id
   COPY_ID for transaction TXN_ID in filesystem FS.  The next node-id is
   used both for creating new unique nodes for the given transaction, as
   well as uniquifying representations.  Perform temporary allocations in
   POOL. */
static svn_error_t *
write_next_ids(svn_fs_t *fs,
               const char *txn_id,
               const char *node_id,
               const char *copy_id,
               apr_pool_t *pool)
{
  apr_file_t *file;
  svn_stream_t *out_stream;

  SVN_ERR(svn_io_file_open(&file, path_txn_next_ids(fs, txn_id, pool),
                           APR_WRITE | APR_TRUNCATE,
                           APR_OS_DEFAULT, pool));

  out_stream = svn_stream_from_aprfile2(file, TRUE, pool);

  SVN_ERR(svn_stream_printf(out_stream, pool, "%s %s\n", node_id, copy_id));

  SVN_ERR(svn_stream_close(out_stream));
  return svn_io_file_close(file, pool);
}

/* Find out what the next unique node-id and copy-id are for
   transaction TXN_ID in filesystem FS.  Store the results in *NODE_ID
   and *COPY_ID.  The next node-id is used both for creating new unique
   nodes for the given transaction, as well as uniquifying representations.
   Perform all allocations in POOL. */
static svn_error_t *
read_next_ids(const char **node_id,
              const char **copy_id,
              svn_fs_t *fs,
              const char *txn_id,
              apr_pool_t *pool)
{
  apr_file_t *file;
  char buf[MAX_KEY_SIZE*2+3];
  apr_size_t limit;
  char *str, *last_str;

  SVN_ERR(svn_io_file_open(&file, path_txn_next_ids(fs, txn_id, pool),
                           APR_READ | APR_BUFFERED, APR_OS_DEFAULT, pool));

  limit = sizeof(buf);
  SVN_ERR(svn_io_read_length_line(file, buf, &limit, pool));

  SVN_ERR(svn_io_file_close(file, pool));

  /* Parse this into two separate strings. */

  str = apr_strtok(buf, " ", &last_str);
  if (! str)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("next-id file corrupt"));

  *node_id = apr_pstrdup(pool, str);

  str = apr_strtok(NULL, " ", &last_str);
  if (! str)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("next-id file corrupt"));

  *copy_id = apr_pstrdup(pool, str);

  return SVN_NO_ERROR;
}

/* Get a new and unique to this transaction node-id for transaction
   TXN_ID in filesystem FS.  Store the new node-id in *NODE_ID_P.
   Node-ids are guaranteed to be unique to this transction, but may
   not necessarily be sequential.  Perform all allocations in POOL. */
static svn_error_t *
get_new_txn_node_id(const char **node_id_p,
                    svn_fs_t *fs,
                    const char *txn_id,
                    apr_pool_t *pool)
{
  const char *cur_node_id, *cur_copy_id;
  char *node_id;
  apr_size_t len;

  /* First read in the current next-ids file. */
  SVN_ERR(read_next_ids(&cur_node_id, &cur_copy_id, fs, txn_id, pool));

  node_id = apr_pcalloc(pool, strlen(cur_node_id) + 2);

  len = strlen(cur_node_id);
  svn_fs_fs__next_key(cur_node_id, &len, node_id);

  SVN_ERR(write_next_ids(fs, txn_id, node_id, cur_copy_id, pool));

  *node_id_p = apr_pstrcat(pool, "_", cur_node_id, (char *)NULL);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__create_node(const svn_fs_id_t **id_p,
                       svn_fs_t *fs,
                       node_revision_t *noderev,
                       const char *copy_id,
                       const char *txn_id,
                       apr_pool_t *pool)
{
  const char *node_id;
  const svn_fs_id_t *id;

  /* Get a new node-id for this node. */
  SVN_ERR(get_new_txn_node_id(&node_id, fs, txn_id, pool));

  id = svn_fs_fs__id_txn_create(node_id, copy_id, txn_id, pool);

  noderev->id = id;

  SVN_ERR(svn_fs_fs__put_node_revision(fs, noderev->id, noderev, FALSE, pool));

  *id_p = id;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__purge_txn(svn_fs_t *fs,
                     const char *txn_id,
                     apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  /* Remove the shared transaction object associated with this transaction. */
  SVN_ERR(purge_shared_txn(fs, txn_id, pool));
  /* Remove the directory associated with this transaction. */
  SVN_ERR(svn_io_remove_dir2(path_txn_dir(fs, txn_id, pool), FALSE,
                             NULL, NULL, pool));
  if (ffd->format >= SVN_FS_FS__MIN_PROTOREVS_DIR_FORMAT)
    {
      /* Delete protorev and its lock, which aren't in the txn
         directory.  It's OK if they don't exist (for example, if this
         is post-commit and the proto-rev has been moved into
         place). */
      SVN_ERR(svn_io_remove_file2(path_txn_proto_rev(fs, txn_id, pool),
                                  TRUE, pool));
      SVN_ERR(svn_io_remove_file2(path_txn_proto_rev_lock(fs, txn_id, pool),
                                  TRUE, pool));
    }
  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_fs__abort_txn(svn_fs_txn_t *txn,
                     apr_pool_t *pool)
{
  SVN_ERR(svn_fs__check_fs(txn->fs, TRUE));

  /* Now, purge the transaction. */
  SVN_ERR_W(svn_fs_fs__purge_txn(txn->fs, txn->id, pool),
            apr_psprintf(pool, _("Transaction '%s' cleanup failed"),
                         txn->id));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_fs__set_entry(svn_fs_t *fs,
                     const char *txn_id,
                     node_revision_t *parent_noderev,
                     const char *name,
                     const svn_fs_id_t *id,
                     svn_node_kind_t kind,
                     apr_pool_t *pool)
{
  representation_t *rep = parent_noderev->data_rep;
  const char *filename = path_txn_node_children(fs, parent_noderev->id, pool);
  apr_file_t *file;
  svn_stream_t *out;
  fs_fs_data_t *ffd = fs->fsap_data;

  if (!rep || !rep->txn_id)
    {
      const char *unique_suffix;

      {
        apr_hash_t *entries;
        apr_pool_t *subpool = svn_pool_create(pool);

        /* Before we can modify the directory, we need to dump its old
           contents into a mutable representation file. */
        SVN_ERR(svn_fs_fs__rep_contents_dir(&entries, fs, parent_noderev,
                                            subpool));
        SVN_ERR(unparse_dir_entries(&entries, entries, subpool));
        SVN_ERR(svn_io_file_open(&file, filename,
                                 APR_WRITE | APR_CREATE | APR_BUFFERED,
                                 APR_OS_DEFAULT, pool));
        out = svn_stream_from_aprfile2(file, TRUE, pool);
        SVN_ERR(svn_hash_write2(entries, out, SVN_HASH_TERMINATOR, subpool));

        svn_pool_destroy(subpool);
      }

      /* Mark the node-rev's data rep as mutable. */
      rep = apr_pcalloc(pool, sizeof(*rep));
      rep->revision = SVN_INVALID_REVNUM;
      rep->txn_id = txn_id;
      SVN_ERR(get_new_txn_node_id(&unique_suffix, fs, txn_id, pool));
      rep->uniquifier = apr_psprintf(pool, "%s/%s", txn_id, unique_suffix);
      parent_noderev->data_rep = rep;
      SVN_ERR(svn_fs_fs__put_node_revision(fs, parent_noderev->id,
                                           parent_noderev, FALSE, pool));
    }
  else
    {
      /* The directory rep is already mutable, so just open it for append. */
      SVN_ERR(svn_io_file_open(&file, filename, APR_WRITE | APR_APPEND,
                               APR_OS_DEFAULT, pool));
      out = svn_stream_from_aprfile2(file, TRUE, pool);
    }

  /* if we have a directory cache for this transaction, update it */
  if (ffd->txn_dir_cache)
    {
      apr_pool_t *subpool = svn_pool_create(pool);

      /* build parameters: (name, new entry) pair */
      const char *key =
          svn_fs_fs__id_unparse(parent_noderev->id, subpool)->data;
      replace_baton_t baton;
      baton.name = name;
      baton.new_entry = NULL;

      if (id)
        {
          baton.new_entry = apr_pcalloc(subpool, sizeof(*baton.new_entry));
          baton.new_entry->name = name;
          baton.new_entry->kind = kind;
          baton.new_entry->id = id;
        }

      /* actually update the cached directory (if cached) */
      SVN_ERR(svn_cache__set_partial(ffd->txn_dir_cache, key, svn_fs_fs__replace_dir_entry, &baton, subpool));

      svn_pool_destroy(subpool);
    }

  /* Append an incremental hash entry for the entry change. */
  if (id)
    {
      const char *val = unparse_dir_entry(kind, id, pool);

      SVN_ERR(svn_stream_printf(out, pool, "K %" APR_SIZE_T_FMT "\n%s\n"
                                "V %" APR_SIZE_T_FMT "\n%s\n",
                                strlen(name), name,
                                strlen(val), val));
    }
  else
    {
      SVN_ERR(svn_stream_printf(out, pool, "D %" APR_SIZE_T_FMT "\n%s\n",
                                strlen(name), name));
    }

  return svn_io_file_close(file, pool);
}

/* Write a single change entry, path PATH, change CHANGE, and copyfrom
   string COPYFROM, into the file specified by FILE.  Only include the
   node kind field if INCLUDE_NODE_KIND is true.  All temporary
   allocations are in POOL. */
static svn_error_t *
write_change_entry(apr_file_t *file,
                   const char *path,
                   svn_fs_path_change2_t *change,
                   svn_boolean_t include_node_kind,
                   apr_pool_t *pool)
{
  const char *idstr, *buf;
  const char *change_string = NULL;
  const char *kind_string = "";

  switch (change->change_kind)
    {
    case svn_fs_path_change_modify:
      change_string = ACTION_MODIFY;
      break;
    case svn_fs_path_change_add:
      change_string = ACTION_ADD;
      break;
    case svn_fs_path_change_delete:
      change_string = ACTION_DELETE;
      break;
    case svn_fs_path_change_replace:
      change_string = ACTION_REPLACE;
      break;
    case svn_fs_path_change_reset:
      change_string = ACTION_RESET;
      break;
    default:
      return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                               _("Invalid change type %d"),
                               change->change_kind);
    }

  if (change->node_rev_id)
    idstr = svn_fs_fs__id_unparse(change->node_rev_id, pool)->data;
  else
    idstr = ACTION_RESET;

  if (include_node_kind)
    {
      SVN_ERR_ASSERT(change->node_kind == svn_node_dir
                     || change->node_kind == svn_node_file);
      kind_string = apr_psprintf(pool, "-%s",
                                 change->node_kind == svn_node_dir
                                 ? KIND_DIR : KIND_FILE);
    }
  buf = apr_psprintf(pool, "%s %s%s %s %s %s\n",
                     idstr, change_string, kind_string,
                     change->text_mod ? FLAG_TRUE : FLAG_FALSE,
                     change->prop_mod ? FLAG_TRUE : FLAG_FALSE,
                     path);

  SVN_ERR(svn_io_file_write_full(file, buf, strlen(buf), NULL, pool));

  if (SVN_IS_VALID_REVNUM(change->copyfrom_rev))
    {
      buf = apr_psprintf(pool, "%ld %s", change->copyfrom_rev,
                         change->copyfrom_path);
      SVN_ERR(svn_io_file_write_full(file, buf, strlen(buf), NULL, pool));
    }

  return svn_io_file_write_full(file, "\n", 1, NULL, pool);
}

svn_error_t *
svn_fs_fs__add_change(svn_fs_t *fs,
                      const char *txn_id,
                      const char *path,
                      const svn_fs_id_t *id,
                      svn_fs_path_change_kind_t change_kind,
                      svn_boolean_t text_mod,
                      svn_boolean_t prop_mod,
                      svn_node_kind_t node_kind,
                      svn_revnum_t copyfrom_rev,
                      const char *copyfrom_path,
                      apr_pool_t *pool)
{
  apr_file_t *file;
  svn_fs_path_change2_t *change;

  SVN_ERR(svn_io_file_open(&file, path_txn_changes(fs, txn_id, pool),
                           APR_APPEND | APR_WRITE | APR_CREATE
                           | APR_BUFFERED, APR_OS_DEFAULT, pool));

  change = svn_fs__path_change_create_internal(id, change_kind, pool);
  change->text_mod = text_mod;
  change->prop_mod = prop_mod;
  change->node_kind = node_kind;
  change->copyfrom_rev = copyfrom_rev;
  change->copyfrom_path = apr_pstrdup(pool, copyfrom_path);

  SVN_ERR(write_change_entry(file, path, change, TRUE, pool));

  return svn_io_file_close(file, pool);
}

/* This baton is used by the representation writing streams.  It keeps
   track of the checksum information as well as the total size of the
   representation so far. */
struct rep_write_baton
{
  /* The FS we are writing to. */
  svn_fs_t *fs;

  /* Actual file to which we are writing. */
  svn_stream_t *rep_stream;

  /* A stream from the delta combiner.  Data written here gets
     deltified, then eventually written to rep_stream. */
  svn_stream_t *delta_stream;

  /* Where is this representation header stored. */
  apr_off_t rep_offset;

  /* Start of the actual data. */
  apr_off_t delta_start;

  /* How many bytes have been written to this rep already. */
  svn_filesize_t rep_size;

  /* The node revision for which we're writing out info. */
  node_revision_t *noderev;

  /* Actual output file. */
  apr_file_t *file;
  /* Lock 'cookie' used to unlock the output file once we've finished
     writing to it. */
  void *lockcookie;

  svn_checksum_ctx_t *md5_checksum_ctx;
  svn_checksum_ctx_t *sha1_checksum_ctx;

  apr_pool_t *pool;

  apr_pool_t *parent_pool;
};

/* Handler for the write method of the representation writable stream.
   BATON is a rep_write_baton, DATA is the data to write, and *LEN is
   the length of this data. */
static svn_error_t *
rep_write_contents(void *baton,
                   const char *data,
                   apr_size_t *len)
{
  struct rep_write_baton *b = baton;

  SVN_ERR(svn_checksum_update(b->md5_checksum_ctx, data, *len));
  SVN_ERR(svn_checksum_update(b->sha1_checksum_ctx, data, *len));
  b->rep_size += *len;

  /* If we are writing a delta, use that stream. */
  if (b->delta_stream)
    return svn_stream_write(b->delta_stream, data, len);
  else
    return svn_stream_write(b->rep_stream, data, len);
}

/* Given a node-revision NODEREV in filesystem FS, return the
   representation in *REP to use as the base for a text representation
   delta.  Perform temporary allocations in *POOL. */
static svn_error_t *
choose_delta_base(representation_t **rep,
                  svn_fs_t *fs,
                  node_revision_t *noderev,
                  apr_pool_t *pool)
{
  int count;
  node_revision_t *base;

  /* If we have no predecessors, then use the empty stream as a
     base. */
  if (! noderev->predecessor_count)
    {
      *rep = NULL;
      return SVN_NO_ERROR;
    }

  /* Flip the rightmost '1' bit of the predecessor count to determine
     which file rev (counting from 0) we want to use.  (To see why
     count & (count - 1) unsets the rightmost set bit, think about how
     you decrement a binary number.) */
  count = noderev->predecessor_count;
  count = count & (count - 1);

  /* Walk back a number of predecessors equal to the difference
     between count and the original predecessor count.  (For example,
     if noderev has ten predecessors and we want the eighth file rev,
     walk back two predecessors.) */
  base = noderev;
  while ((count++) < noderev->predecessor_count)
    SVN_ERR(svn_fs_fs__get_node_revision(&base, fs,
                                         base->predecessor_id, pool));

  *rep = base->data_rep;

  return SVN_NO_ERROR;
}

/* Something went wrong and the pool for the rep write is being
   cleared before we've finished writing the rep.  So we need
   to remove the rep from the protorevfile and we need to unlock
   the protorevfile. */
static apr_status_t
rep_write_cleanup(void *data)
{
  struct rep_write_baton *b = data;
  const char *txn_id = svn_fs_fs__id_txn_id(b->noderev->id);
  svn_error_t *err;
  
  /* Truncate and close the protorevfile. */
  err = svn_io_file_trunc(b->file, b->rep_offset, b->pool);
  err = svn_error_compose_create(err, svn_io_file_close(b->file, b->pool));

  /* Remove our lock regardless of any preceeding errors so that the 
     being_written flag is always removed and stays consistent with the
     file lock which will be removed no matter what since the pool is
     going away. */
  err = svn_error_compose_create(err, unlock_proto_rev(b->fs, txn_id,
                                                       b->lockcookie, b->pool));
  if (err)
    {
      apr_status_t rc = err->apr_err;
      svn_error_clear(err);
      return rc;
    }

  return APR_SUCCESS;
}


/* Get a rep_write_baton and store it in *WB_P for the representation
   indicated by NODEREV in filesystem FS.  Perform allocations in
   POOL.  Only appropriate for file contents, not for props or
   directory contents. */
static svn_error_t *
rep_write_get_baton(struct rep_write_baton **wb_p,
                    svn_fs_t *fs,
                    node_revision_t *noderev,
                    apr_pool_t *pool)
{
  struct rep_write_baton *b;
  apr_file_t *file;
  representation_t *base_rep;
  svn_stream_t *source;
  const char *header;
  svn_txdelta_window_handler_t wh;
  void *whb;
  fs_fs_data_t *ffd = fs->fsap_data;
  int diff_version = ffd->format >= SVN_FS_FS__MIN_SVNDIFF1_FORMAT ? 1 : 0;

  b = apr_pcalloc(pool, sizeof(*b));

  b->sha1_checksum_ctx = svn_checksum_ctx_create(svn_checksum_sha1, pool);
  b->md5_checksum_ctx = svn_checksum_ctx_create(svn_checksum_md5, pool);

  b->fs = fs;
  b->parent_pool = pool;
  b->pool = svn_pool_create(pool);
  b->rep_size = 0;
  b->noderev = noderev;

  /* Open the prototype rev file and seek to its end. */
  SVN_ERR(get_writable_proto_rev(&file, &b->lockcookie,
                                 fs, svn_fs_fs__id_txn_id(noderev->id),
                                 b->pool));

  b->file = file;
  b->rep_stream = svn_stream_from_aprfile2(file, TRUE, b->pool);

  SVN_ERR(get_file_offset(&b->rep_offset, file, b->pool));

  /* Get the base for this delta. */
  SVN_ERR(choose_delta_base(&base_rep, fs, noderev, b->pool));
  SVN_ERR(read_representation(&source, fs, base_rep, b->pool));

  /* Write out the rep header. */
  if (base_rep)
    {
      header = apr_psprintf(b->pool, REP_DELTA " %ld %" APR_OFF_T_FMT " %"
                            SVN_FILESIZE_T_FMT "\n",
                            base_rep->revision, base_rep->offset,
                            base_rep->size);
    }
  else
    {
      header = REP_DELTA "\n";
    }
  SVN_ERR(svn_io_file_write_full(file, header, strlen(header), NULL,
                                 b->pool));

  /* Now determine the offset of the actual svndiff data. */
  SVN_ERR(get_file_offset(&b->delta_start, file, b->pool));

  /* Cleanup in case something goes wrong. */
  apr_pool_cleanup_register(b->pool, b, rep_write_cleanup,
                            apr_pool_cleanup_null);

  /* Prepare to write the svndiff data. */
  svn_txdelta_to_svndiff3(&wh,
                          &whb,
                          b->rep_stream,
                          diff_version,
                          SVN_DELTA_COMPRESSION_LEVEL_DEFAULT,
                          pool);

  b->delta_stream = svn_txdelta_target_push(wh, whb, source, b->pool);

  *wb_p = b;

  return SVN_NO_ERROR;
}

/* Close handler for the representation write stream.  BATON is a
   rep_write_baton.  Writes out a new node-rev that correctly
   references the representation we just finished writing. */
static svn_error_t *
rep_write_contents_close(void *baton)
{
  struct rep_write_baton *b = baton;
  fs_fs_data_t *ffd = b->fs->fsap_data;
  const char *unique_suffix;
  representation_t *rep;
  representation_t *old_rep;
  apr_off_t offset;

  rep = apr_pcalloc(b->parent_pool, sizeof(*rep));
  rep->offset = b->rep_offset;

  /* Close our delta stream so the last bits of svndiff are written
     out. */
  if (b->delta_stream)
    SVN_ERR(svn_stream_close(b->delta_stream));

  /* Determine the length of the svndiff data. */
  SVN_ERR(get_file_offset(&offset, b->file, b->pool));
  rep->size = offset - b->delta_start;

  /* Fill in the rest of the representation field. */
  rep->expanded_size = b->rep_size;
  rep->txn_id = svn_fs_fs__id_txn_id(b->noderev->id);
  SVN_ERR(get_new_txn_node_id(&unique_suffix, b->fs, rep->txn_id, b->pool));
  rep->uniquifier = apr_psprintf(b->parent_pool, "%s/%s", rep->txn_id,
                                 unique_suffix);
  rep->revision = SVN_INVALID_REVNUM;

  /* Finalize the checksum. */
  SVN_ERR(svn_checksum_final(&rep->md5_checksum, b->md5_checksum_ctx,
                              b->parent_pool));
  SVN_ERR(svn_checksum_final(&rep->sha1_checksum, b->sha1_checksum_ctx,
                              b->parent_pool));

  /* Check and see if we already have a representation somewhere that's
     identical to the one we just wrote out. */
  if (ffd->rep_sharing_allowed)
    {
      svn_error_t *err;
      err = svn_fs_fs__get_rep_reference(&old_rep, b->fs, rep->sha1_checksum,
                                         b->parent_pool);
      /* ### Other error codes that we shouldn't mask out? */
      if (err == SVN_NO_ERROR
          || err->apr_err == SVN_ERR_FS_CORRUPT
          || SVN_ERROR_IN_CATEGORY(err->apr_err,
                                   SVN_ERR_MALFUNC_CATEGORY_START))
        {
          /* Fatal error; don't mask it.

             In particular, this block is triggered when the rep-cache refers
             to revisions in the future.  We signal that as a corruption situation
             since, once those revisions are less than youngest (because of more
             commits), the rep-cache would be invalid.
           */
          SVN_ERR(err);
        }
      else
        {
          /* Something's wrong with the rep-sharing index.  We can continue
             without rep-sharing, but warn.
           */
          (b->fs->warning)(b->fs->warning_baton, err);
          svn_error_clear(err);
          old_rep = NULL;
        }
    }
  else
    old_rep = NULL;

  if (old_rep)
    {
      /* We need to erase from the protorev the data we just wrote. */
      SVN_ERR(svn_io_file_trunc(b->file, b->rep_offset, b->pool));

      /* Use the old rep for this content. */
      old_rep->md5_checksum = rep->md5_checksum;
      old_rep->uniquifier = rep->uniquifier;
      b->noderev->data_rep = old_rep;
    }
  else
    {
      /* Write out our cosmetic end marker. */
      SVN_ERR(svn_stream_printf(b->rep_stream, b->pool, "ENDREP\n"));

      b->noderev->data_rep = rep;
    }

  /* Remove cleanup callback. */
  apr_pool_cleanup_kill(b->pool, b, rep_write_cleanup);

  /* Write out the new node-rev information. */
  SVN_ERR(svn_fs_fs__put_node_revision(b->fs, b->noderev->id, b->noderev, FALSE,
                                       b->pool));

  SVN_ERR(svn_io_file_close(b->file, b->pool));
  SVN_ERR(unlock_proto_rev(b->fs, rep->txn_id, b->lockcookie, b->pool));
  svn_pool_destroy(b->pool);

  return SVN_NO_ERROR;
}

/* Store a writable stream in *CONTENTS_P that will receive all data
   written and store it as the file data representation referenced by
   NODEREV in filesystem FS.  Perform temporary allocations in
   POOL.  Only appropriate for file data, not props or directory
   contents. */
static svn_error_t *
set_representation(svn_stream_t **contents_p,
                   svn_fs_t *fs,
                   node_revision_t *noderev,
                   apr_pool_t *pool)
{
  struct rep_write_baton *wb;

  if (! svn_fs_fs__id_txn_id(noderev->id))
    return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                             _("Attempted to write to non-transaction '%s'"),
                             svn_fs_fs__id_unparse(noderev->id, pool)->data);

  SVN_ERR(rep_write_get_baton(&wb, fs, noderev, pool));

  *contents_p = svn_stream_create(wb, pool);
  svn_stream_set_write(*contents_p, rep_write_contents);
  svn_stream_set_close(*contents_p, rep_write_contents_close);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__set_contents(svn_stream_t **stream,
                        svn_fs_t *fs,
                        node_revision_t *noderev,
                        apr_pool_t *pool)
{
  if (noderev->kind != svn_node_file)
    return svn_error_create(SVN_ERR_FS_NOT_FILE, NULL,
                            _("Can't set text contents of a directory"));

  return set_representation(stream, fs, noderev, pool);
}

svn_error_t *
svn_fs_fs__create_successor(const svn_fs_id_t **new_id_p,
                            svn_fs_t *fs,
                            const svn_fs_id_t *old_idp,
                            node_revision_t *new_noderev,
                            const char *copy_id,
                            const char *txn_id,
                            apr_pool_t *pool)
{
  const svn_fs_id_t *id;

  if (! copy_id)
    copy_id = svn_fs_fs__id_copy_id(old_idp);
  id = svn_fs_fs__id_txn_create(svn_fs_fs__id_node_id(old_idp), copy_id,
                                txn_id, pool);

  new_noderev->id = id;

  if (! new_noderev->copyroot_path)
    {
      new_noderev->copyroot_path = apr_pstrdup(pool,
                                               new_noderev->created_path);
      new_noderev->copyroot_rev = svn_fs_fs__id_rev(new_noderev->id);
    }

  SVN_ERR(svn_fs_fs__put_node_revision(fs, new_noderev->id, new_noderev, FALSE,
                                       pool));

  *new_id_p = id;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__set_proplist(svn_fs_t *fs,
                        node_revision_t *noderev,
                        apr_hash_t *proplist,
                        apr_pool_t *pool)
{
  const char *filename = path_txn_node_props(fs, noderev->id, pool);
  apr_file_t *file;
  svn_stream_t *out;

  /* Dump the property list to the mutable property file. */
  SVN_ERR(svn_io_file_open(&file, filename,
                           APR_WRITE | APR_CREATE | APR_TRUNCATE
                           | APR_BUFFERED, APR_OS_DEFAULT, pool));
  out = svn_stream_from_aprfile2(file, TRUE, pool);
  SVN_ERR(svn_hash_write2(proplist, out, SVN_HASH_TERMINATOR, pool));
  SVN_ERR(svn_io_file_close(file, pool));

  /* Mark the node-rev's prop rep as mutable, if not already done. */
  if (!noderev->prop_rep || !noderev->prop_rep->txn_id)
    {
      noderev->prop_rep = apr_pcalloc(pool, sizeof(*noderev->prop_rep));
      noderev->prop_rep->txn_id = svn_fs_fs__id_txn_id(noderev->id);
      SVN_ERR(svn_fs_fs__put_node_revision(fs, noderev->id, noderev, FALSE, pool));
    }

  return SVN_NO_ERROR;
}

/* Read the 'current' file for filesystem FS and store the next
   available node id in *NODE_ID, and the next available copy id in
   *COPY_ID.  Allocations are performed from POOL. */
static svn_error_t *
get_next_revision_ids(const char **node_id,
                      const char **copy_id,
                      svn_fs_t *fs,
                      apr_pool_t *pool)
{
  char *buf;
  char *str, *last_str;

  SVN_ERR(read_current(svn_fs_fs__path_current(fs, pool), &buf, pool));

  str = apr_strtok(buf, " ", &last_str);
  if (! str)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Corrupt 'current' file"));

  str = apr_strtok(NULL, " ", &last_str);
  if (! str)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Corrupt 'current' file"));

  *node_id = apr_pstrdup(pool, str);

  str = apr_strtok(NULL, " ", &last_str);
  if (! str)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Corrupt 'current' file"));

  *copy_id = apr_pstrdup(pool, str);

  return SVN_NO_ERROR;
}

/* This baton is used by the stream created for write_hash_rep. */
struct write_hash_baton
{
  svn_stream_t *stream;

  apr_size_t size;

  svn_checksum_ctx_t *checksum_ctx;
};

/* The handler for the write_hash_rep stream.  BATON is a
   write_hash_baton, DATA has the data to write and *LEN is the number
   of bytes to write. */
static svn_error_t *
write_hash_handler(void *baton,
                   const char *data,
                   apr_size_t *len)
{
  struct write_hash_baton *whb = baton;

  SVN_ERR(svn_checksum_update(whb->checksum_ctx, data, *len));

  SVN_ERR(svn_stream_write(whb->stream, data, len));
  whb->size += *len;

  return SVN_NO_ERROR;
}

/* Write out the hash HASH as a text representation to file FILE.  In
   the process, record the total size of the dump in *SIZE, and the
   md5 digest in CHECKSUM.  Perform temporary allocations in POOL. */
static svn_error_t *
write_hash_rep(svn_filesize_t *size,
               svn_checksum_t **checksum,
               apr_file_t *file,
               apr_hash_t *hash,
               apr_pool_t *pool)
{
  svn_stream_t *stream;
  struct write_hash_baton *whb;

  whb = apr_pcalloc(pool, sizeof(*whb));

  whb->stream = svn_stream_from_aprfile2(file, TRUE, pool);
  whb->size = 0;
  whb->checksum_ctx = svn_checksum_ctx_create(svn_checksum_md5, pool);

  stream = svn_stream_create(whb, pool);
  svn_stream_set_write(stream, write_hash_handler);

  SVN_ERR(svn_stream_printf(whb->stream, pool, "PLAIN\n"));

  SVN_ERR(svn_hash_write2(hash, stream, SVN_HASH_TERMINATOR, pool));

  /* Store the results. */
  SVN_ERR(svn_checksum_final(checksum, whb->checksum_ctx, pool));
  *size = whb->size;

  return svn_stream_printf(whb->stream, pool, "ENDREP\n");
}

/* Sanity check ROOT_NODEREV, a candidate for being the root node-revision
   of (not yet committed) revision REV in FS.  Use POOL for temporary
   allocations.
 */
static svn_error_t *
validate_root_noderev(svn_fs_t *fs,
                      node_revision_t *root_noderev,
                      svn_revnum_t rev,
                      apr_pool_t *pool)
{
  svn_revnum_t head_revnum = rev-1;
  int head_predecessor_count;

  SVN_ERR_ASSERT(rev > 0);

  /* Compute HEAD_PREDECESSOR_COUNT. */
  {
    svn_fs_root_t *head_revision;
    const svn_fs_id_t *head_root_id;
    node_revision_t *head_root_noderev;

    /* Get /@HEAD's noderev. */
    SVN_ERR(svn_fs_fs__revision_root(&head_revision, fs, head_revnum, pool));
    SVN_ERR(svn_fs_fs__node_id(&head_root_id, head_revision, "/", pool));
    SVN_ERR(svn_fs_fs__get_node_revision(&head_root_noderev, fs, head_root_id,
                                         pool));

    head_predecessor_count = head_root_noderev->predecessor_count;
  }

  /* Check that the root noderev's predecessor count equals REV.

     This kind of corruption was seen on svn.apache.org (both on
     the root noderev and on other fspaths' noderevs); see
       http://mid.gmane.org/20111002202833.GA12373@daniel3.local

     Normally (rev == root_noderev->predecessor_count), but here we
     use a more roundabout check that should only trigger on new instances
     of the corruption, rather then trigger on each and every new commit
     to a repository that has triggered the bug somewhere in its root
     noderev's history.
   */
  if (root_noderev->predecessor_count != -1
      && (root_noderev->predecessor_count - head_predecessor_count)
         != (rev - head_revnum))
    {
      return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                               _("predecessor count for "
                                 "the root node-revision is wrong: "
                                 "found (%d+%ld != %d), committing r%ld"),
                                 head_predecessor_count,
                                 rev - head_revnum, /* This is equal to 1. */
                                 root_noderev->predecessor_count,
                                 rev);
    }

  return SVN_NO_ERROR;
}

/* Copy a node-revision specified by id ID in fileystem FS from a
   transaction into the proto-rev-file FILE.  Set *NEW_ID_P to a
   pointer to the new node-id which will be allocated in POOL.
   If this is a directory, copy all children as well.

   START_NODE_ID and START_COPY_ID are
   the first available node and copy ids for this filesystem, for older
   FS formats.

   REV is the revision number that this proto-rev-file will represent.

   INITIAL_OFFSET is the offset of the proto-rev-file on entry to
   commit_body.

   If REPS_TO_CACHE is not NULL, append to it a copy (allocated in
   REPS_POOL) of each data rep that is new in this revision.

   AT_ROOT is true if the node revision being written is the root
   node-revision.  It is only controls additional sanity checking
   logic.

   Temporary allocations are also from POOL. */
static svn_error_t *
write_final_rev(const svn_fs_id_t **new_id_p,
                apr_file_t *file,
                svn_revnum_t rev,
                svn_fs_t *fs,
                const svn_fs_id_t *id,
                const char *start_node_id,
                const char *start_copy_id,
                apr_off_t initial_offset,
                apr_array_header_t *reps_to_cache,
                apr_pool_t *reps_pool,
                svn_boolean_t at_root,
                apr_pool_t *pool)
{
  node_revision_t *noderev;
  apr_off_t my_offset;
  char my_node_id_buf[MAX_KEY_SIZE + 2];
  char my_copy_id_buf[MAX_KEY_SIZE + 2];
  const svn_fs_id_t *new_id;
  const char *node_id, *copy_id, *my_node_id, *my_copy_id;
  fs_fs_data_t *ffd = fs->fsap_data;

  *new_id_p = NULL;

  /* Check to see if this is a transaction node. */
  if (! svn_fs_fs__id_txn_id(id))
    return SVN_NO_ERROR;

  SVN_ERR(svn_fs_fs__get_node_revision(&noderev, fs, id, pool));

  if (noderev->kind == svn_node_dir)
    {
      apr_pool_t *subpool;
      apr_hash_t *entries, *str_entries;
      apr_array_header_t *sorted_entries;
      int i;

      /* This is a directory.  Write out all the children first. */
      subpool = svn_pool_create(pool);

      SVN_ERR(svn_fs_fs__rep_contents_dir(&entries, fs, noderev, pool));
      /* For the sake of the repository administrator sort the entries
         so that the final file is deterministic and repeatable,
         however the rest of the FSFS code doesn't require any
         particular order here. */
      sorted_entries = svn_sort__hash(entries, svn_sort_compare_items_lexically,
                                      pool);
      for (i = 0; i < sorted_entries->nelts; ++i)
        {
          svn_fs_dirent_t *dirent = APR_ARRAY_IDX(sorted_entries, i,
                                                  svn_sort__item_t).value;

          svn_pool_clear(subpool);
          SVN_ERR(write_final_rev(&new_id, file, rev, fs, dirent->id,
                                  start_node_id, start_copy_id, initial_offset,
                                  reps_to_cache, reps_pool, FALSE,
                                  subpool));
          if (new_id && (svn_fs_fs__id_rev(new_id) == rev))
            dirent->id = svn_fs_fs__id_copy(new_id, pool);
        }
      svn_pool_destroy(subpool);

      if (noderev->data_rep && noderev->data_rep->txn_id)
        {
          /* Write out the contents of this directory as a text rep. */
          SVN_ERR(unparse_dir_entries(&str_entries, entries, pool));

          noderev->data_rep->txn_id = NULL;
          noderev->data_rep->revision = rev;
          SVN_ERR(get_file_offset(&noderev->data_rep->offset, file, pool));
          SVN_ERR(write_hash_rep(&noderev->data_rep->size,
                                 &noderev->data_rep->md5_checksum, file,
                                 str_entries, pool));
          noderev->data_rep->expanded_size = noderev->data_rep->size;
        }
    }
  else
    {
      /* This is a file.  We should make sure the data rep, if it
         exists in a "this" state, gets rewritten to our new revision
         num. */

      if (noderev->data_rep && noderev->data_rep->txn_id)
        {
          noderev->data_rep->txn_id = NULL;
          noderev->data_rep->revision = rev;

          /* See issue 3845.  Some unknown mechanism caused the
             protorev file to get truncated, so check for that
             here.  */
          if (noderev->data_rep->offset + noderev->data_rep->size
              > initial_offset)
            return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                    _("Truncated protorev file detected"));
        }
    }

  /* Fix up the property reps. */
  if (noderev->prop_rep && noderev->prop_rep->txn_id)
    {
      apr_hash_t *proplist;

      SVN_ERR(svn_fs_fs__get_proplist(&proplist, fs, noderev, pool));
      SVN_ERR(get_file_offset(&noderev->prop_rep->offset, file, pool));
      SVN_ERR(write_hash_rep(&noderev->prop_rep->size,
                             &noderev->prop_rep->md5_checksum, file,
                             proplist, pool));

      noderev->prop_rep->txn_id = NULL;
      noderev->prop_rep->revision = rev;
    }


  /* Convert our temporary ID into a permanent revision one. */
  SVN_ERR(get_file_offset(&my_offset, file, pool));

  node_id = svn_fs_fs__id_node_id(noderev->id);
  if (*node_id == '_')
    {
      if (ffd->format >= SVN_FS_FS__MIN_NO_GLOBAL_IDS_FORMAT)
        my_node_id = apr_psprintf(pool, "%s-%ld", node_id + 1, rev);
      else
        {
          svn_fs_fs__add_keys(start_node_id, node_id + 1, my_node_id_buf);
          my_node_id = my_node_id_buf;
        }
    }
  else
    my_node_id = node_id;

  copy_id = svn_fs_fs__id_copy_id(noderev->id);
  if (*copy_id == '_')
    {
      if (ffd->format >= SVN_FS_FS__MIN_NO_GLOBAL_IDS_FORMAT)
        my_copy_id = apr_psprintf(pool, "%s-%ld", copy_id + 1, rev);
      else
        {
          svn_fs_fs__add_keys(start_copy_id, copy_id + 1, my_copy_id_buf);
          my_copy_id = my_copy_id_buf;
        }
    }
  else
    my_copy_id = copy_id;

  if (noderev->copyroot_rev == SVN_INVALID_REVNUM)
    noderev->copyroot_rev = rev;

  new_id = svn_fs_fs__id_rev_create(my_node_id, my_copy_id, rev, my_offset,
                                    pool);

  noderev->id = new_id;

  /* Write out our new node-revision. */
  if (at_root)
    SVN_ERR(validate_root_noderev(fs, noderev, rev, pool));
  SVN_ERR(svn_fs_fs__write_noderev(svn_stream_from_aprfile2(file, TRUE, pool),
                                   noderev, ffd->format,
                                   svn_fs_fs__fs_supports_mergeinfo(fs),
                                   pool));

  /* Save the data representation's hash in the rep cache. */
  if (ffd->rep_sharing_allowed
        && noderev->data_rep && noderev->kind == svn_node_file
        && noderev->data_rep->revision == rev)
    {
      SVN_ERR_ASSERT(reps_to_cache && reps_pool);
      APR_ARRAY_PUSH(reps_to_cache, representation_t *)
        = svn_fs_fs__rep_copy(noderev->data_rep, reps_pool);
    }

  /* Return our ID that references the revision file. */
  *new_id_p = noderev->id;

  return SVN_NO_ERROR;
}

/* Write the changed path info from transaction TXN_ID in filesystem
   FS to the permanent rev-file FILE.  *OFFSET_P is set the to offset
   in the file of the beginning of this information.  Perform
   temporary allocations in POOL. */
static svn_error_t *
write_final_changed_path_info(apr_off_t *offset_p,
                              apr_file_t *file,
                              svn_fs_t *fs,
                              const char *txn_id,
                              apr_pool_t *pool)
{
  apr_hash_t *changed_paths;
  apr_off_t offset;
  apr_pool_t *iterpool = svn_pool_create(pool);
  fs_fs_data_t *ffd = fs->fsap_data;
  svn_boolean_t include_node_kinds =
      ffd->format >= SVN_FS_FS__MIN_KIND_IN_CHANGED_FORMAT;
  apr_array_header_t *sorted_changed_paths;
  int i;

  SVN_ERR(get_file_offset(&offset, file, pool));

  SVN_ERR(svn_fs_fs__txn_changes_fetch(&changed_paths, fs, txn_id, pool));
  /* For the sake of the repository administrator sort the changes so
     that the final file is deterministic and repeatable, however the
     rest of the FSFS code doesn't require any particular order here. */
  sorted_changed_paths = svn_sort__hash(changed_paths,
                                        svn_sort_compare_items_lexically, pool);

  /* Iterate through the changed paths one at a time, and convert the
     temporary node-id into a permanent one for each change entry. */
  for (i = 0; i < sorted_changed_paths->nelts; ++i)
    {
      node_revision_t *noderev;
      const svn_fs_id_t *id;
      svn_fs_path_change2_t *change;
      const char *path;

      svn_pool_clear(iterpool);

      change = APR_ARRAY_IDX(sorted_changed_paths, i, svn_sort__item_t).value;
      path = APR_ARRAY_IDX(sorted_changed_paths, i, svn_sort__item_t).key;

      id = change->node_rev_id;

      /* If this was a delete of a mutable node, then it is OK to
         leave the change entry pointing to the non-existent temporary
         node, since it will never be used. */
      if ((change->change_kind != svn_fs_path_change_delete) &&
          (! svn_fs_fs__id_txn_id(id)))
        {
          SVN_ERR(svn_fs_fs__get_node_revision(&noderev, fs, id, iterpool));

          /* noderev has the permanent node-id at this point, so we just
             substitute it for the temporary one. */
          change->node_rev_id = noderev->id;
        }

      /* Write out the new entry into the final rev-file. */
      SVN_ERR(write_change_entry(file, path, change, include_node_kinds,
                                 iterpool));
    }

  svn_pool_destroy(iterpool);

  *offset_p = offset;

  return SVN_NO_ERROR;
}

/* Atomically update the 'current' file to hold the specifed REV,
   NEXT_NODE_ID, and NEXT_COPY_ID.  (The two next-ID parameters are
   ignored and may be NULL if the FS format does not use them.)
   Perform temporary allocations in POOL. */
static svn_error_t *
write_current(svn_fs_t *fs, svn_revnum_t rev, const char *next_node_id,
              const char *next_copy_id, apr_pool_t *pool)
{
  char *buf;
  const char *tmp_name, *name;
  fs_fs_data_t *ffd = fs->fsap_data;

  /* Now we can just write out this line. */
  if (ffd->format >= SVN_FS_FS__MIN_NO_GLOBAL_IDS_FORMAT)
    buf = apr_psprintf(pool, "%ld\n", rev);
  else
    buf = apr_psprintf(pool, "%ld %s %s\n", rev, next_node_id, next_copy_id);

  name = svn_fs_fs__path_current(fs, pool);
  SVN_ERR(svn_io_write_unique(&tmp_name,
                              svn_dirent_dirname(name, pool),
                              buf, strlen(buf),
                              svn_io_file_del_none, pool));

  return move_into_place(tmp_name, name, name, pool);
}

/* Update the 'current' file to hold the correct next node and copy_ids
   from transaction TXN_ID in filesystem FS.  The current revision is
   set to REV.  Perform temporary allocations in POOL. */
static svn_error_t *
write_final_current(svn_fs_t *fs,
                    const char *txn_id,
                    svn_revnum_t rev,
                    const char *start_node_id,
                    const char *start_copy_id,
                    apr_pool_t *pool)
{
  const char *txn_node_id, *txn_copy_id;
  char new_node_id[MAX_KEY_SIZE + 2];
  char new_copy_id[MAX_KEY_SIZE + 2];
  fs_fs_data_t *ffd = fs->fsap_data;

  if (ffd->format >= SVN_FS_FS__MIN_NO_GLOBAL_IDS_FORMAT)
    return write_current(fs, rev, NULL, NULL, pool);

  /* To find the next available ids, we add the id that used to be in
     the 'current' file, to the next ids from the transaction file. */
  SVN_ERR(read_next_ids(&txn_node_id, &txn_copy_id, fs, txn_id, pool));

  svn_fs_fs__add_keys(start_node_id, txn_node_id, new_node_id);
  svn_fs_fs__add_keys(start_copy_id, txn_copy_id, new_copy_id);

  return write_current(fs, rev, new_node_id, new_copy_id, pool);
}

/* Verify that the user registed with FS has all the locks necessary to
   permit all the changes associate with TXN_NAME.
   The FS write lock is assumed to be held by the caller. */
static svn_error_t *
verify_locks(svn_fs_t *fs,
             const char *txn_name,
             apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  apr_hash_t *changes;
  apr_hash_index_t *hi;
  apr_array_header_t *changed_paths;
  svn_stringbuf_t *last_recursed = NULL;
  int i;

  /* Fetch the changes for this transaction. */
  SVN_ERR(svn_fs_fs__txn_changes_fetch(&changes, fs, txn_name, pool));

  /* Make an array of the changed paths, and sort them depth-first-ily.  */
  changed_paths = apr_array_make(pool, apr_hash_count(changes) + 1,
                                 sizeof(const char *));
  for (hi = apr_hash_first(pool, changes); hi; hi = apr_hash_next(hi))
    APR_ARRAY_PUSH(changed_paths, const char *) = svn__apr_hash_index_key(hi);
  qsort(changed_paths->elts, changed_paths->nelts,
        changed_paths->elt_size, svn_sort_compare_paths);

  /* Now, traverse the array of changed paths, verify locks.  Note
     that if we need to do a recursive verification a path, we'll skip
     over children of that path when we get to them. */
  for (i = 0; i < changed_paths->nelts; i++)
    {
      const char *path;
      svn_fs_path_change2_t *change;
      svn_boolean_t recurse = TRUE;

      svn_pool_clear(subpool);
      path = APR_ARRAY_IDX(changed_paths, i, const char *);

      /* If this path has already been verified as part of a recursive
         check of one of its parents, no need to do it again.  */
      if (last_recursed
          && svn_dirent_is_child(last_recursed->data, path, subpool))
        continue;

      /* Fetch the change associated with our path.  */
      change = apr_hash_get(changes, path, APR_HASH_KEY_STRING);

      /* What does it mean to succeed at lock verification for a given
         path?  For an existing file or directory getting modified
         (text, props), it means we hold the lock on the file or
         directory.  For paths being added or removed, we need to hold
         the locks for that path and any children of that path.

         WHEW!  We have no reliable way to determine the node kind
         of deleted items, but fortunately we are going to do a
         recursive check on deleted paths regardless of their kind.  */
      if (change->change_kind == svn_fs_path_change_modify)
        recurse = FALSE;
      SVN_ERR(svn_fs_fs__allow_locked_operation(path, fs, recurse, TRUE,
                                                subpool));

      /* If we just did a recursive check, remember the path we
         checked (so children can be skipped).  */
      if (recurse)
        {
          if (! last_recursed)
            last_recursed = svn_stringbuf_create(path, pool);
          else
            svn_stringbuf_set(last_recursed, path);
        }
    }
  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}

/* Baton used for commit_body below. */
struct commit_baton {
  svn_revnum_t *new_rev_p;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  apr_array_header_t *reps_to_cache;
  apr_pool_t *reps_pool;
};

/* The work-horse for svn_fs_fs__commit, called with the FS write lock.
   This implements the svn_fs_fs__with_write_lock() 'body' callback
   type.  BATON is a 'struct commit_baton *'. */
static svn_error_t *
commit_body(void *baton, apr_pool_t *pool)
{
  struct commit_baton *cb = baton;
  fs_fs_data_t *ffd = cb->fs->fsap_data;
  const char *old_rev_filename, *rev_filename, *proto_filename;
  const char *revprop_filename, *final_revprop;
  const svn_fs_id_t *root_id, *new_root_id;
  const char *start_node_id = NULL, *start_copy_id = NULL;
  svn_revnum_t old_rev, new_rev;
  apr_file_t *proto_file;
  void *proto_file_lockcookie;
  apr_off_t initial_offset, changed_path_offset;
  char *buf;
  apr_hash_t *txnprops;
  apr_array_header_t *txnprop_list;
  svn_prop_t prop;
  svn_string_t date;

  /* Get the current youngest revision. */
  SVN_ERR(svn_fs_fs__youngest_rev(&old_rev, cb->fs, pool));

  /* Check to make sure this transaction is based off the most recent
     revision. */
  if (cb->txn->base_rev != old_rev)
    return svn_error_create(SVN_ERR_FS_TXN_OUT_OF_DATE, NULL,
                            _("Transaction out of date"));

  /* Locks may have been added (or stolen) between the calling of
     previous svn_fs.h functions and svn_fs_commit_txn(), so we need
     to re-examine every changed-path in the txn and re-verify all
     discovered locks. */
  SVN_ERR(verify_locks(cb->fs, cb->txn->id, pool));

  /* Get the next node_id and copy_id to use. */
  if (ffd->format < SVN_FS_FS__MIN_NO_GLOBAL_IDS_FORMAT)
    SVN_ERR(get_next_revision_ids(&start_node_id, &start_copy_id, cb->fs,
                                  pool));

  /* We are going to be one better than this puny old revision. */
  new_rev = old_rev + 1;

  /* Get a write handle on the proto revision file. */
  SVN_ERR(get_writable_proto_rev(&proto_file, &proto_file_lockcookie,
                                 cb->fs, cb->txn->id, pool));
  SVN_ERR(get_file_offset(&initial_offset, proto_file, pool));

  /* Write out all the node-revisions and directory contents. */
  root_id = svn_fs_fs__id_txn_create("0", "0", cb->txn->id, pool);
  SVN_ERR(write_final_rev(&new_root_id, proto_file, new_rev, cb->fs, root_id,
                          start_node_id, start_copy_id, initial_offset,
                          cb->reps_to_cache, cb->reps_pool, TRUE,
                          pool));

  /* Write the changed-path information. */
  SVN_ERR(write_final_changed_path_info(&changed_path_offset, proto_file,
                                        cb->fs, cb->txn->id, pool));

  /* Write the final line. */
  buf = apr_psprintf(pool, "\n%" APR_OFF_T_FMT " %" APR_OFF_T_FMT "\n",
                     svn_fs_fs__id_offset(new_root_id),
                     changed_path_offset);
  SVN_ERR(svn_io_file_write_full(proto_file, buf, strlen(buf), NULL,
                                 pool));
  SVN_ERR(svn_io_file_flush_to_disk(proto_file, pool));
  SVN_ERR(svn_io_file_close(proto_file, pool));

  /* We don't unlock the prototype revision file immediately to avoid a
     race with another caller writing to the prototype revision file
     before we commit it. */

  /* Remove any temporary txn props representing 'flags'. */
  SVN_ERR(svn_fs_fs__txn_proplist(&txnprops, cb->txn, pool));
  txnprop_list = apr_array_make(pool, 3, sizeof(svn_prop_t));
  prop.value = NULL;

  if (apr_hash_get(txnprops, SVN_FS__PROP_TXN_CHECK_OOD, APR_HASH_KEY_STRING))
    {
      prop.name = SVN_FS__PROP_TXN_CHECK_OOD;
      APR_ARRAY_PUSH(txnprop_list, svn_prop_t) = prop;
    }

  if (apr_hash_get(txnprops, SVN_FS__PROP_TXN_CHECK_LOCKS,
                   APR_HASH_KEY_STRING))
    {
      prop.name = SVN_FS__PROP_TXN_CHECK_LOCKS;
      APR_ARRAY_PUSH(txnprop_list, svn_prop_t) = prop;
    }

  if (! apr_is_empty_array(txnprop_list))
    SVN_ERR(svn_fs_fs__change_txn_props(cb->txn, txnprop_list, pool));

  /* Create the shard for the rev and revprop file, if we're sharding and
     this is the first revision of a new shard.  We don't care if this
     fails because the shard already existed for some reason. */
  if (ffd->max_files_per_dir && new_rev % ffd->max_files_per_dir == 0)
    {
      if (1)
        {
          const char *new_dir = path_rev_shard(cb->fs, new_rev, pool);
          svn_error_t *err = svn_io_dir_make(new_dir, APR_OS_DEFAULT, pool);
          if (err && !APR_STATUS_IS_EEXIST(err->apr_err))
            return svn_error_trace(err);
          svn_error_clear(err);
          SVN_ERR(svn_io_copy_perms(svn_dirent_join(cb->fs->path,
                                                    PATH_REVS_DIR,
                                                    pool),
                                    new_dir, pool));
        }

      /* Create the revprops shard. */
      SVN_ERR_ASSERT(! is_packed_revprop(cb->fs, new_rev));
        {
          const char *new_dir = path_revprops_shard(cb->fs, new_rev, pool);
          svn_error_t *err = svn_io_dir_make(new_dir, APR_OS_DEFAULT, pool);
          if (err && !APR_STATUS_IS_EEXIST(err->apr_err))
            return svn_error_trace(err);
          svn_error_clear(err);
          SVN_ERR(svn_io_copy_perms(svn_dirent_join(cb->fs->path,
                                                    PATH_REVPROPS_DIR,
                                                    pool),
                                    new_dir, pool));
        }
    }

  /* Move the finished rev file into place. */
  SVN_ERR(svn_fs_fs__path_rev_absolute(&old_rev_filename,
                                       cb->fs, old_rev, pool));
  rev_filename = path_rev(cb->fs, new_rev, pool);
  proto_filename = path_txn_proto_rev(cb->fs, cb->txn->id, pool);
  SVN_ERR(move_into_place(proto_filename, rev_filename, old_rev_filename,
                          pool));

  /* Now that we've moved the prototype revision file out of the way,
     we can unlock it (since further attempts to write to the file
     will fail as it no longer exists).  We must do this so that we can
     remove the transaction directory later. */
  SVN_ERR(unlock_proto_rev(cb->fs, cb->txn->id, proto_file_lockcookie, pool));

  /* Update commit time to ensure that svn:date revprops remain ordered. */
  date.data = svn_time_to_cstring(apr_time_now(), pool);
  date.len = strlen(date.data);

  SVN_ERR(svn_fs_fs__change_txn_prop(cb->txn, SVN_PROP_REVISION_DATE,
                                     &date, pool));

  /* Move the revprops file into place. */
  SVN_ERR_ASSERT(! is_packed_revprop(cb->fs, new_rev));
  revprop_filename = path_txn_props(cb->fs, cb->txn->id, pool);
  final_revprop = path_revprops(cb->fs, new_rev, pool);
  SVN_ERR(move_into_place(revprop_filename, final_revprop,
                          old_rev_filename, pool));

  /* Update the 'current' file. */
  SVN_ERR(write_final_current(cb->fs, cb->txn->id, new_rev, start_node_id,
                              start_copy_id, pool));

  /* At this point the new revision is committed and globally visible
     so let the caller know it succeeded by giving it the new revision
     number, which fulfills svn_fs_commit_txn() contract.  Any errors
     after this point do not change the fact that a new revision was
     created. */
  *cb->new_rev_p = new_rev;

  ffd->youngest_rev_cache = new_rev;

  /* Remove this transaction directory. */
  SVN_ERR(svn_fs_fs__purge_txn(cb->fs, cb->txn->id, pool));

  return SVN_NO_ERROR;
}

/* Add the representations in REPS_TO_CACHE (an array of representation_t *)
 * to the rep-cache database of FS. */
static svn_error_t *
write_reps_to_cache(svn_fs_t *fs,
                    const apr_array_header_t *reps_to_cache,
                    apr_pool_t *scratch_pool)
{
  int i;

  for (i = 0; i < reps_to_cache->nelts; i++)
    {
      representation_t *rep = APR_ARRAY_IDX(reps_to_cache, i, representation_t *);

      /* FALSE because we don't care if another parallel commit happened to
       * collide with us.  (Non-parallel collisions will not be detected.) */
      SVN_ERR(svn_fs_fs__set_rep_reference(fs, rep, FALSE, scratch_pool));
    }

  return SVN_NO_ERROR;
}

/* Implements svn_sqlite__transaction_callback_t. */
static svn_error_t *
commit_sqlite_txn_callback(void *baton, svn_sqlite__db_t *db,
                           apr_pool_t *scratch_pool)
{
  struct commit_baton *cb = baton;

  /* Write new entries to the rep-sharing database.
   *
   * We use an sqlite transcation to speed things up;
   * see <http://www.sqlite.org/faq.html#q19>.
   */
  SVN_ERR(write_reps_to_cache(cb->fs, cb->reps_to_cache, scratch_pool));

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__commit(svn_revnum_t *new_rev_p,
                  svn_fs_t *fs,
                  svn_fs_txn_t *txn,
                  apr_pool_t *pool)
{
  struct commit_baton cb;
  fs_fs_data_t *ffd = fs->fsap_data;

  cb.new_rev_p = new_rev_p;
  cb.fs = fs;
  cb.txn = txn;

  if (ffd->rep_sharing_allowed)
    {
      cb.reps_to_cache = apr_array_make(pool, 5, sizeof(representation_t *));
      cb.reps_pool = pool;
    }
  else
    {
      cb.reps_to_cache = NULL;
      cb.reps_pool = NULL;
    }

  SVN_ERR(svn_fs_fs__with_write_lock(fs, commit_body, &cb, pool));

  if (ffd->rep_sharing_allowed)
    {
      /* At this point, *NEW_REV_P has been set, so errors here won't affect
         the success of the commit.  (See svn_fs_commit_txn().)  */
      SVN_ERR(svn_fs_fs__open_rep_cache(fs, pool));
      SVN_ERR(svn_sqlite__with_transaction(ffd->rep_cache_db,
                                           commit_sqlite_txn_callback,
                                           &cb, pool));
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_fs__reserve_copy_id(const char **copy_id_p,
                           svn_fs_t *fs,
                           const char *txn_id,
                           apr_pool_t *pool)
{
  const char *cur_node_id, *cur_copy_id;
  char *copy_id;
  apr_size_t len;

  /* First read in the current next-ids file. */
  SVN_ERR(read_next_ids(&cur_node_id, &cur_copy_id, fs, txn_id, pool));

  copy_id = apr_pcalloc(pool, strlen(cur_copy_id) + 2);

  len = strlen(cur_copy_id);
  svn_fs_fs__next_key(cur_copy_id, &len, copy_id);

  SVN_ERR(write_next_ids(fs, txn_id, cur_node_id, copy_id, pool));

  *copy_id_p = apr_pstrcat(pool, "_", cur_copy_id, (char *)NULL);

  return SVN_NO_ERROR;
}

/* Write out the zeroth revision for filesystem FS. */
static svn_error_t *
write_revision_zero(svn_fs_t *fs)
{
  const char *path_revision_zero = path_rev(fs, 0, fs->pool);
  apr_hash_t *proplist;
  svn_string_t date;

  /* Write out a rev file for revision 0. */
  SVN_ERR(svn_io_file_create(path_revision_zero,
                             "PLAIN\nEND\nENDREP\n"
                             "id: 0.0.r0/17\n"
                             "type: dir\n"
                             "count: 0\n"
                             "text: 0 0 4 4 "
                             "2d2977d1c96f487abe4a1e202dd03b4e\n"
                             "cpath: /\n"
                             "\n\n17 107\n", fs->pool));
  SVN_ERR(svn_io_set_file_read_only(path_revision_zero, FALSE, fs->pool));

  /* Set a date on revision 0. */
  date.data = svn_time_to_cstring(apr_time_now(), fs->pool);
  date.len = strlen(date.data);
  proplist = apr_hash_make(fs->pool);
  apr_hash_set(proplist, SVN_PROP_REVISION_DATE, APR_HASH_KEY_STRING, &date);
  return set_revision_proplist(fs, 0, proplist, fs->pool);
}

svn_error_t *
svn_fs_fs__create(svn_fs_t *fs,
                  const char *path,
                  apr_pool_t *pool)
{
  int format = SVN_FS_FS__FORMAT_NUMBER;
  fs_fs_data_t *ffd = fs->fsap_data;

  fs->path = apr_pstrdup(pool, path);
  /* See if compatibility with older versions was explicitly requested. */
  if (fs->config)
    {
      if (apr_hash_get(fs->config, SVN_FS_CONFIG_PRE_1_4_COMPATIBLE,
                                   APR_HASH_KEY_STRING))
        format = 1;
      else if (apr_hash_get(fs->config, SVN_FS_CONFIG_PRE_1_5_COMPATIBLE,
                                        APR_HASH_KEY_STRING))
        format = 2;
      else if (apr_hash_get(fs->config, SVN_FS_CONFIG_PRE_1_6_COMPATIBLE,
                                        APR_HASH_KEY_STRING))
        format = 3;
    }
  ffd->format = format;

  /* Override the default linear layout if this is a new-enough format. */
  if (format >= SVN_FS_FS__MIN_LAYOUT_FORMAT_OPTION_FORMAT)
    ffd->max_files_per_dir = SVN_FS_FS_DEFAULT_MAX_FILES_PER_DIR;

  /* Create the revision data directories. */
  if (ffd->max_files_per_dir)
    SVN_ERR(svn_io_make_dir_recursively(path_rev_shard(fs, 0, pool), pool));
  else
    SVN_ERR(svn_io_make_dir_recursively(svn_dirent_join(path, PATH_REVS_DIR,
                                                        pool),
                                        pool));

  /* Create the revprops directory. */
  if (ffd->max_files_per_dir)
    SVN_ERR(svn_io_make_dir_recursively(path_revprops_shard(fs, 0, pool),
                                        pool));
  else
    SVN_ERR(svn_io_make_dir_recursively(svn_dirent_join(path,
                                                        PATH_REVPROPS_DIR,
                                                        pool),
                                        pool));

  /* Create the transaction directory. */
  SVN_ERR(svn_io_make_dir_recursively(svn_dirent_join(path, PATH_TXNS_DIR,
                                                      pool),
                                      pool));

  /* Create the protorevs directory. */
  if (format >= SVN_FS_FS__MIN_PROTOREVS_DIR_FORMAT)
    SVN_ERR(svn_io_make_dir_recursively(svn_dirent_join(path, PATH_TXN_PROTOS_DIR,
                                                      pool),
                                        pool));

  /* Create the 'current' file. */
  SVN_ERR(svn_io_file_create(svn_fs_fs__path_current(fs, pool),
                             (format >= SVN_FS_FS__MIN_NO_GLOBAL_IDS_FORMAT
                              ? "0\n" : "0 1 1\n"),
                             pool));
  SVN_ERR(svn_io_file_create(path_lock(fs, pool), "", pool));
  SVN_ERR(svn_fs_fs__set_uuid(fs, svn_uuid_generate(pool), pool));

  SVN_ERR(write_revision_zero(fs));

  SVN_ERR(write_config(fs, pool));

  SVN_ERR(read_config(fs, pool));

  /* Create the min unpacked rev file. */
  if (ffd->format >= SVN_FS_FS__MIN_PACKED_FORMAT)
    SVN_ERR(svn_io_file_create(path_min_unpacked_rev(fs, pool), "0\n", pool));

  /* Create the txn-current file if the repository supports
     the transaction sequence file. */
  if (format >= SVN_FS_FS__MIN_TXN_CURRENT_FORMAT)
    {
      SVN_ERR(svn_io_file_create(path_txn_current(fs, pool),
                                 "0\n", pool));
      SVN_ERR(svn_io_file_create(path_txn_current_lock(fs, pool),
                                 "", pool));
    }

  /* This filesystem is ready.  Stamp it with a format number. */
  SVN_ERR(write_format(path_format(fs, pool),
                       ffd->format, ffd->max_files_per_dir, FALSE, pool));

  ffd->youngest_rev_cache = 0;
  return SVN_NO_ERROR;
}

/* Part of the recovery procedure.  Return the largest revision *REV in
   filesystem FS.  Use POOL for temporary allocation. */
static svn_error_t *
recover_get_largest_revision(svn_fs_t *fs, svn_revnum_t *rev, apr_pool_t *pool)
{
  /* Discovering the largest revision in the filesystem would be an
     expensive operation if we did a readdir() or searched linearly,
     so we'll do a form of binary search.  left is a revision that we
     know exists, right a revision that we know does not exist. */
  apr_pool_t *iterpool;
  svn_revnum_t left, right = 1;

  iterpool = svn_pool_create(pool);
  /* Keep doubling right, until we find a revision that doesn't exist. */
  while (1)
    {
      svn_error_t *err;
      apr_file_t *file;

      err = open_pack_or_rev_file(&file, fs, right, iterpool);
      svn_pool_clear(iterpool);

      if (err && err->apr_err == SVN_ERR_FS_NO_SUCH_REVISION)
        {
          svn_error_clear(err);
          break;
        }
      else
        SVN_ERR(err);

      right <<= 1;
    }

  left = right >> 1;

  /* We know that left exists and right doesn't.  Do a normal bsearch to find
     the last revision. */
  while (left + 1 < right)
    {
      svn_revnum_t probe = left + ((right - left) / 2);
      svn_error_t *err;
      apr_file_t *file;

      err = open_pack_or_rev_file(&file, fs, probe, iterpool);
      svn_pool_clear(iterpool);

      if (err && err->apr_err == SVN_ERR_FS_NO_SUCH_REVISION)
        {
          svn_error_clear(err);
          right = probe;
        }
      else
        {
          SVN_ERR(err);
          left = probe;
        }
    }

  svn_pool_destroy(iterpool);

  /* left is now the largest revision that exists. */
  *rev = left;
  return SVN_NO_ERROR;
}

/* A baton for reading a fixed amount from an open file.  For
   recover_find_max_ids() below. */
struct recover_read_from_file_baton
{
  apr_file_t *file;
  apr_pool_t *pool;
  apr_off_t remaining;
};

/* A stream read handler used by recover_find_max_ids() below.
   Read and return at most BATON->REMAINING bytes from the stream,
   returning nothing after that to indicate EOF. */
static svn_error_t *
read_handler_recover(void *baton, char *buffer, apr_size_t *len)
{
  struct recover_read_from_file_baton *b = baton;
  svn_filesize_t bytes_to_read = *len;

  if (b->remaining == 0)
    {
      /* Return a successful read of zero bytes to signal EOF. */
      *len = 0;
      return SVN_NO_ERROR;
    }

  if (bytes_to_read > b->remaining)
    bytes_to_read = b->remaining;
  b->remaining -= bytes_to_read;

  return svn_io_file_read_full2(b->file, buffer, (apr_size_t) bytes_to_read,
                                len, NULL, b->pool);
}

/* Part of the recovery procedure.  Read the directory noderev at offset
   OFFSET of file REV_FILE (the revision file of revision REV of
   filesystem FS), and set MAX_NODE_ID and MAX_COPY_ID to be the node-id
   and copy-id of that node, if greater than the current value stored
   in either.  Recurse into any child directories that were modified in
   this revision.

   MAX_NODE_ID and MAX_COPY_ID must be arrays of at least MAX_KEY_SIZE.

   Perform temporary allocation in POOL. */
static svn_error_t *
recover_find_max_ids(svn_fs_t *fs, svn_revnum_t rev,
                     apr_file_t *rev_file, apr_off_t offset,
                     char *max_node_id, char *max_copy_id,
                     apr_pool_t *pool)
{
  apr_hash_t *headers;
  char *value;
  representation_t *data_rep;
  struct rep_args *ra;
  struct recover_read_from_file_baton baton;
  svn_stream_t *stream;
  apr_hash_t *entries;
  apr_hash_index_t *hi;
  apr_pool_t *iterpool;

  SVN_ERR(svn_io_file_seek(rev_file, APR_SET, &offset, pool));
  SVN_ERR(read_header_block(&headers, svn_stream_from_aprfile2(rev_file, TRUE,
                                                               pool),
                            pool));

  /* Check that this is a directory.  It should be. */
  value = apr_hash_get(headers, HEADER_TYPE, APR_HASH_KEY_STRING);
  if (value == NULL || strcmp(value, KIND_DIR) != 0)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Recovery encountered a non-directory node"));

  /* Get the data location.  No data location indicates an empty directory. */
  value = apr_hash_get(headers, HEADER_TEXT, APR_HASH_KEY_STRING);
  if (!value)
    return SVN_NO_ERROR;
  SVN_ERR(read_rep_offsets(&data_rep, value, NULL, FALSE, pool));

  /* If the directory's data representation wasn't changed in this revision,
     we've already scanned the directory's contents for noderevs, so we don't
     need to again.  This will occur if a property is changed on a directory
     without changing the directory's contents. */
  if (data_rep->revision != rev)
    return SVN_NO_ERROR;

  /* We could use get_dir_contents(), but this is much cheaper.  It does
     rely on directory entries being stored as PLAIN reps, though. */
  offset = data_rep->offset;
  SVN_ERR(svn_io_file_seek(rev_file, APR_SET, &offset, pool));
  SVN_ERR(read_rep_line(&ra, rev_file, pool));
  if (ra->is_delta)
    return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                            _("Recovery encountered a deltified directory "
                              "representation"));

  /* Now create a stream that's allowed to read only as much data as is
     stored in the representation. */
  baton.file = rev_file;
  baton.pool = pool;
  baton.remaining = data_rep->expanded_size;
  stream = svn_stream_create(&baton, pool);
  svn_stream_set_read(stream, read_handler_recover);

  /* Now read the entries from that stream. */
  entries = apr_hash_make(pool);
  SVN_ERR(svn_hash_read2(entries, stream, SVN_HASH_TERMINATOR, pool));
  SVN_ERR(svn_stream_close(stream));

  /* Now check each of the entries in our directory to find new node and
     copy ids, and recurse into new subdirectories. */
  iterpool = svn_pool_create(pool);
  for (hi = apr_hash_first(pool, entries); hi; hi = apr_hash_next(hi))
    {
      char *str_val;
      char *str, *last_str;
      svn_node_kind_t kind;
      svn_fs_id_t *id;
      const char *node_id, *copy_id;
      apr_off_t child_dir_offset;
      const svn_string_t *path = svn__apr_hash_index_val(hi);

      svn_pool_clear(iterpool);

      str_val = apr_pstrdup(iterpool, path->data);

      str = apr_strtok(str_val, " ", &last_str);
      if (str == NULL)
        return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                _("Directory entry corrupt"));

      if (strcmp(str, KIND_FILE) == 0)
        kind = svn_node_file;
      else if (strcmp(str, KIND_DIR) == 0)
        kind = svn_node_dir;
      else
        {
          return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                  _("Directory entry corrupt"));
        }

      str = apr_strtok(NULL, " ", &last_str);
      if (str == NULL)
        return svn_error_create(SVN_ERR_FS_CORRUPT, NULL,
                                _("Directory entry corrupt"));

      id = svn_fs_fs__id_parse(str, strlen(str), iterpool);

      if (svn_fs_fs__id_rev(id) != rev)
        {
          /* If the node wasn't modified in this revision, we've already
             checked the node and copy id. */
          continue;
        }

      node_id = svn_fs_fs__id_node_id(id);
      copy_id = svn_fs_fs__id_copy_id(id);

      if (svn_fs_fs__key_compare(node_id, max_node_id) > 0)
        {
          SVN_ERR_ASSERT(strlen(node_id) < MAX_KEY_SIZE);
          apr_cpystrn(max_node_id, node_id, MAX_KEY_SIZE);
        }
      if (svn_fs_fs__key_compare(copy_id, max_copy_id) > 0)
        {
          SVN_ERR_ASSERT(strlen(copy_id) < MAX_KEY_SIZE);
          apr_cpystrn(max_copy_id, copy_id, MAX_KEY_SIZE);
        }

      if (kind == svn_node_file)
        continue;

      child_dir_offset = svn_fs_fs__id_offset(id);
      SVN_ERR(recover_find_max_ids(fs, rev, rev_file, child_dir_offset,
                                   max_node_id, max_copy_id, iterpool));
    }
  svn_pool_destroy(iterpool);

  return SVN_NO_ERROR;
}

/* Baton used for recover_body below. */
struct recover_baton {
  svn_fs_t *fs;
  svn_cancel_func_t cancel_func;
  void *cancel_baton;
};

/* The work-horse for svn_fs_fs__recover, called with the FS
   write lock.  This implements the svn_fs_fs__with_write_lock()
   'body' callback type.  BATON is a 'struct recover_baton *'. */
static svn_error_t *
recover_body(void *baton, apr_pool_t *pool)
{
  struct recover_baton *b = baton;
  svn_fs_t *fs = b->fs;
  fs_fs_data_t *ffd = fs->fsap_data;
  svn_revnum_t max_rev;
  char next_node_id_buf[MAX_KEY_SIZE], next_copy_id_buf[MAX_KEY_SIZE];
  char *next_node_id = NULL, *next_copy_id = NULL;
  svn_revnum_t youngest_rev;
  svn_node_kind_t youngest_revprops_kind;

  /* First, we need to know the largest revision in the filesystem. */
  SVN_ERR(recover_get_largest_revision(fs, &max_rev, pool));

  /* Get the expected youngest revision */
  SVN_ERR(get_youngest(&youngest_rev, fs->path, pool));

  /* Policy note:

     Since the revprops file is written after the revs file, the true
     maximum available revision is the youngest one for which both are
     present.  That's probably the same as the max_rev we just found,
     but if it's not, we could, in theory, repeatedly decrement
     max_rev until we find a revision that has both a revs and
     revprops file, then write db/current with that.

     But we choose not to.  If a repository is so corrupt that it's
     missing at least one revprops file, we shouldn't assume that the
     youngest revision for which both the revs and revprops files are
     present is healthy.  In other words, we're willing to recover
     from a missing or out-of-date db/current file, because db/current
     is truly redundant -- it's basically a cache so we don't have to
     find max_rev each time, albeit a cache with unusual semantics,
     since it also officially defines when a revision goes live.  But
     if we're missing more than the cache, it's time to back out and
     let the admin reconstruct things by hand: correctness at that
     point may depend on external things like checking a commit email
     list, looking in particular working copies, etc.

     This policy matches well with a typical naive backup scenario.
     Say you're rsyncing your FSFS repository nightly to the same
     location.  Once revs and revprops are written, you've got the
     maximum rev; if the backup should bomb before db/current is
     written, then db/current could stay arbitrarily out-of-date, but
     we can still recover.  It's a small window, but we might as well
     do what we can. */

  /* Even if db/current were missing, it would be created with 0 by
     get_youngest(), so this conditional remains valid. */
  if (youngest_rev > max_rev)
    return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                             _("Expected current rev to be <= %ld "
                               "but found %ld"), max_rev, youngest_rev);

  /* We only need to search for maximum IDs for old FS formats which
     se global ID counters. */
  if (ffd->format < SVN_FS_FS__MIN_NO_GLOBAL_IDS_FORMAT)
    {
      /* Next we need to find the maximum node id and copy id in use across the
         filesystem.  Unfortunately, the only way we can get this information
         is to scan all the noderevs of all the revisions and keep track as
         we go along. */
      svn_revnum_t rev;
      apr_pool_t *iterpool = svn_pool_create(pool);
      char max_node_id[MAX_KEY_SIZE] = "0", max_copy_id[MAX_KEY_SIZE] = "0";
      apr_size_t len;

      for (rev = 0; rev <= max_rev; rev++)
        {
          apr_file_t *rev_file;
          apr_off_t root_offset;

          svn_pool_clear(iterpool);

          if (b->cancel_func)
            SVN_ERR(b->cancel_func(b->cancel_baton));

          SVN_ERR(open_pack_or_rev_file(&rev_file, fs, rev, iterpool));
          SVN_ERR(get_root_changes_offset(&root_offset, NULL, rev_file, fs, rev,
                                          iterpool));
          SVN_ERR(recover_find_max_ids(fs, rev, rev_file, root_offset,
                                       max_node_id, max_copy_id, iterpool));
          SVN_ERR(svn_io_file_close(rev_file, iterpool));
        }
      svn_pool_destroy(iterpool);

      /* Now that we finally have the maximum revision, node-id and copy-id, we
         can bump the two ids to get the next of each. */
      len = strlen(max_node_id);
      svn_fs_fs__next_key(max_node_id, &len, next_node_id_buf);
      next_node_id = next_node_id_buf;
      len = strlen(max_copy_id);
      svn_fs_fs__next_key(max_copy_id, &len, next_copy_id_buf);
      next_copy_id = next_copy_id_buf;
    }

  /* Before setting current, verify that there is a revprops file
     for the youngest revision.  (Issue #2992) */
  SVN_ERR(svn_io_check_path(path_revprops(fs, max_rev, pool),
                            &youngest_revprops_kind, pool));
  if (youngest_revprops_kind == svn_node_none)
    {
      if (1)
        {
          return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                   _("Revision %ld has a revs file but no "
                                     "revprops file"),
                                   max_rev);
        }
    }
  else if (youngest_revprops_kind != svn_node_file)
    {
      return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                               _("Revision %ld has a non-file where its "
                                 "revprops file should be"),
                               max_rev);
    }

  /* Prune younger-than-(newfound-youngest) revisions from the rep cache. */
  if (ffd->format >= SVN_FS_FS__MIN_REP_SHARING_FORMAT)
    SVN_ERR(svn_fs_fs__del_rep_reference(fs, max_rev, pool));

  /* Now store the discovered youngest revision, and the next IDs if
     relevant, in a new 'current' file. */
  return write_current(fs, max_rev, next_node_id, next_copy_id, pool);
}

/* This implements the fs_library_vtable_t.recover() API. */
svn_error_t *
svn_fs_fs__recover(svn_fs_t *fs,
                   svn_cancel_func_t cancel_func, void *cancel_baton,
                   apr_pool_t *pool)
{
  struct recover_baton b;

  /* We have no way to take out an exclusive lock in FSFS, so we're
     restricted as to the types of recovery we can do.  Luckily,
     we just want to recreate the 'current' file, and we can do that just
     by blocking other writers. */
  b.fs = fs;
  b.cancel_func = cancel_func;
  b.cancel_baton = cancel_baton;
  return svn_fs_fs__with_write_lock(fs, recover_body, &b, pool);
}

svn_error_t *
svn_fs_fs__get_uuid(svn_fs_t *fs,
                    const char **uuid_p,
                    apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  *uuid_p = apr_pstrdup(pool, ffd->uuid);
  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__set_uuid(svn_fs_t *fs,
                    const char *uuid,
                    apr_pool_t *pool)
{
  char *my_uuid;
  apr_size_t my_uuid_len;
  const char *tmp_path;
  const char *uuid_path = path_uuid(fs, pool);
  fs_fs_data_t *ffd = fs->fsap_data;

  if (! uuid)
    uuid = svn_uuid_generate(pool);

  /* Make sure we have a copy in FS->POOL, and append a newline. */
  my_uuid = apr_pstrcat(fs->pool, uuid, "\n", (char *)NULL);
  my_uuid_len = strlen(my_uuid);

  SVN_ERR(svn_io_write_unique(&tmp_path,
                              svn_dirent_dirname(uuid_path, pool),
                              my_uuid, my_uuid_len,
                              svn_io_file_del_none, pool));

  /* We use the permissions of the 'current' file, because the 'uuid'
     file does not exist during repository creation. */
  SVN_ERR(move_into_place(tmp_path, uuid_path,
                          svn_fs_fs__path_current(fs, pool), pool));

  /* Remove the newline we added, and stash the UUID. */
  my_uuid[my_uuid_len - 1] = '\0';
  ffd->uuid = my_uuid;

  return SVN_NO_ERROR;
}

/** Node origin lazy cache. */

/* If directory PATH does not exist, create it and give it the same
   permissions as FS->path.*/
svn_error_t *
svn_fs_fs__ensure_dir_exists(const char *path,
                             const char *fs_path,
                             apr_pool_t *pool)
{
  svn_error_t *err = svn_io_dir_make(path, APR_OS_DEFAULT, pool);
  if (err && APR_STATUS_IS_EEXIST(err->apr_err))
    {
      svn_error_clear(err);
      return SVN_NO_ERROR;
    }
  SVN_ERR(err);

  /* We successfully created a new directory.  Dup the permissions
     from FS->path. */
  return svn_io_copy_perms(fs_path, path, pool);
}

/* Set *NODE_ORIGINS to a hash mapping 'const char *' node IDs to
   'svn_string_t *' node revision IDs.  Use POOL for allocations. */
static svn_error_t *
get_node_origins_from_file(svn_fs_t *fs,
                           apr_hash_t **node_origins,
                           const char *node_origins_file,
                           apr_pool_t *pool)
{
  apr_file_t *fd;
  svn_error_t *err;
  svn_stream_t *stream;

  *node_origins = NULL;
  err = svn_io_file_open(&fd, node_origins_file,
                         APR_READ, APR_OS_DEFAULT, pool);
  if (err && APR_STATUS_IS_ENOENT(err->apr_err))
    {
      svn_error_clear(err);
      return SVN_NO_ERROR;
    }
  SVN_ERR(err);

  stream = svn_stream_from_aprfile2(fd, FALSE, pool);
  *node_origins = apr_hash_make(pool);
  SVN_ERR(svn_hash_read2(*node_origins, stream, SVN_HASH_TERMINATOR, pool));
  return svn_stream_close(stream);
}

svn_error_t *
svn_fs_fs__get_node_origin(const svn_fs_id_t **origin_id,
                           svn_fs_t *fs,
                           const char *node_id,
                           apr_pool_t *pool)
{
  apr_hash_t *node_origins;

  *origin_id = NULL;
  SVN_ERR(get_node_origins_from_file(fs, &node_origins,
                                     path_node_origin(fs, node_id, pool),
                                     pool));
  if (node_origins)
    {
      svn_string_t *origin_id_str =
        apr_hash_get(node_origins, node_id, APR_HASH_KEY_STRING);
      if (origin_id_str)
        *origin_id = svn_fs_fs__id_parse(origin_id_str->data,
                                         origin_id_str->len, pool);
    }
  return SVN_NO_ERROR;
}


/* Helper for svn_fs_fs__set_node_origin.  Takes a NODE_ID/NODE_REV_ID
   pair and adds it to the NODE_ORIGINS_PATH file.  */
static svn_error_t *
set_node_origins_for_file(svn_fs_t *fs,
                          const char *node_origins_path,
                          const char *node_id,
                          svn_string_t *node_rev_id,
                          apr_pool_t *pool)
{
  const char *path_tmp;
  svn_stream_t *stream;
  apr_hash_t *origins_hash;
  svn_string_t *old_node_rev_id;

  SVN_ERR(svn_fs_fs__ensure_dir_exists(svn_dirent_join(fs->path,
                                                       PATH_NODE_ORIGINS_DIR,
                                                       pool),
                                       fs->path, pool));

  /* Read the previously existing origins (if any), and merge our
     update with it. */
  SVN_ERR(get_node_origins_from_file(fs, &origins_hash,
                                     node_origins_path, pool));
  if (! origins_hash)
    origins_hash = apr_hash_make(pool);

  old_node_rev_id = apr_hash_get(origins_hash, node_id, APR_HASH_KEY_STRING);

  if (old_node_rev_id && !svn_string_compare(node_rev_id, old_node_rev_id))
    return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                             _("Node origin for '%s' exists with a different "
                               "value (%s) than what we were about to store "
                               "(%s)"),
                             node_id, old_node_rev_id->data, node_rev_id->data);

  apr_hash_set(origins_hash, node_id, APR_HASH_KEY_STRING, node_rev_id);

  /* Sure, there's a race condition here.  Two processes could be
     trying to add different cache elements to the same file at the
     same time, and the entries added by the first one to write will
     be lost.  But this is just a cache of reconstructible data, so
     we'll accept this problem in return for not having to deal with
     locking overhead. */

  /* Create a temporary file, write out our hash, and close the file. */
  SVN_ERR(svn_stream_open_unique(&stream, &path_tmp,
                                 svn_dirent_dirname(node_origins_path, pool),
                                 svn_io_file_del_none, pool, pool));
  SVN_ERR(svn_hash_write2(origins_hash, stream, SVN_HASH_TERMINATOR, pool));
  SVN_ERR(svn_stream_close(stream));

  /* Rename the temp file as the real destination */
  return svn_io_file_rename(path_tmp, node_origins_path, pool);
}


svn_error_t *
svn_fs_fs__set_node_origin(svn_fs_t *fs,
                           const char *node_id,
                           const svn_fs_id_t *node_rev_id,
                           apr_pool_t *pool)
{
  svn_error_t *err;
  const char *filename = path_node_origin(fs, node_id, pool);

  err = set_node_origins_for_file(fs, filename,
                                  node_id,
                                  svn_fs_fs__id_unparse(node_rev_id, pool),
                                  pool);
  if (err && APR_STATUS_IS_EACCES(err->apr_err))
    {
      /* It's just a cache; stop trying if I can't write. */
      svn_error_clear(err);
      err = NULL;
    }
  return svn_error_trace(err);
}


svn_error_t *
svn_fs_fs__list_transactions(apr_array_header_t **names_p,
                             svn_fs_t *fs,
                             apr_pool_t *pool)
{
  const char *txn_dir;
  apr_hash_t *dirents;
  apr_hash_index_t *hi;
  apr_array_header_t *names;
  apr_size_t ext_len = strlen(PATH_EXT_TXN);

  names = apr_array_make(pool, 1, sizeof(const char *));

  /* Get the transactions directory. */
  txn_dir = svn_dirent_join(fs->path, PATH_TXNS_DIR, pool);

  /* Now find a listing of this directory. */
  SVN_ERR(svn_io_get_dirents3(&dirents, txn_dir, TRUE, pool, pool));

  /* Loop through all the entries and return anything that ends with '.txn'. */
  for (hi = apr_hash_first(pool, dirents); hi; hi = apr_hash_next(hi))
    {
      const char *name = svn__apr_hash_index_key(hi);
      apr_ssize_t klen = svn__apr_hash_index_klen(hi);
      const char *id;

      /* The name must end with ".txn" to be considered a transaction. */
      if ((apr_size_t) klen <= ext_len
          || (strcmp(name + klen - ext_len, PATH_EXT_TXN)) != 0)
        continue;

      /* Truncate the ".txn" extension and store the ID. */
      id = apr_pstrndup(pool, name, strlen(name) - ext_len);
      APR_ARRAY_PUSH(names, const char *) = id;
    }

  *names_p = names;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__open_txn(svn_fs_txn_t **txn_p,
                    svn_fs_t *fs,
                    const char *name,
                    apr_pool_t *pool)
{
  svn_fs_txn_t *txn;
  svn_node_kind_t kind;
  transaction_t *local_txn;

  /* First check to see if the directory exists. */
  SVN_ERR(svn_io_check_path(path_txn_dir(fs, name, pool), &kind, pool));

  /* Did we find it? */
  if (kind != svn_node_dir)
    return svn_error_createf(SVN_ERR_FS_NO_SUCH_TRANSACTION, NULL,
                             _("No such transaction '%s'"),
                             name);

  txn = apr_pcalloc(pool, sizeof(*txn));

  /* Read in the root node of this transaction. */
  txn->id = apr_pstrdup(pool, name);
  txn->fs = fs;

  SVN_ERR(svn_fs_fs__get_txn(&local_txn, fs, name, pool));

  txn->base_rev = svn_fs_fs__id_rev(local_txn->base_id);

  txn->vtable = &txn_vtable;
  *txn_p = txn;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__txn_proplist(apr_hash_t **table_p,
                        svn_fs_txn_t *txn,
                        apr_pool_t *pool)
{
  apr_hash_t *proplist = apr_hash_make(pool);
  SVN_ERR(get_txn_proplist(proplist, txn->fs, txn->id, pool));
  *table_p = proplist;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__delete_node_revision(svn_fs_t *fs,
                                const svn_fs_id_t *id,
                                apr_pool_t *pool)
{
  node_revision_t *noderev;

  SVN_ERR(svn_fs_fs__get_node_revision(&noderev, fs, id, pool));

  /* Delete any mutable property representation. */
  if (noderev->prop_rep && noderev->prop_rep->txn_id)
    SVN_ERR(svn_io_remove_file2(path_txn_node_props(fs, id, pool), FALSE,
                                pool));

  /* Delete any mutable data representation. */
  if (noderev->data_rep && noderev->data_rep->txn_id
      && noderev->kind == svn_node_dir)
    {
      fs_fs_data_t *ffd = fs->fsap_data;
      SVN_ERR(svn_io_remove_file2(path_txn_node_children(fs, id, pool), FALSE,
                                  pool));

      /* remove the corresponding entry from the cache, if such exists */
      if (ffd->txn_dir_cache)
        {
          const char *key = svn_fs_fs__id_unparse(id, pool)->data;
          SVN_ERR(svn_cache__set(ffd->txn_dir_cache, key, NULL, pool));
        }
    }

  return svn_io_remove_file2(path_txn_node_rev(fs, id, pool), FALSE, pool);
}



/*** Revisions ***/

svn_error_t *
svn_fs_fs__revision_prop(svn_string_t **value_p,
                         svn_fs_t *fs,
                         svn_revnum_t rev,
                         const char *propname,
                         apr_pool_t *pool)
{
  apr_hash_t *table;

  SVN_ERR(svn_fs__check_fs(fs, TRUE));
  SVN_ERR(svn_fs_fs__revision_proplist(&table, fs, rev, pool));

  *value_p = apr_hash_get(table, propname, APR_HASH_KEY_STRING);

  return SVN_NO_ERROR;
}


/* Baton used for change_rev_prop_body below. */
struct change_rev_prop_baton {
  svn_fs_t *fs;
  svn_revnum_t rev;
  const char *name;
  const svn_string_t *const *old_value_p;
  const svn_string_t *value;
};

/* The work-horse for svn_fs_fs__change_rev_prop, called with the FS
   write lock.  This implements the svn_fs_fs__with_write_lock()
   'body' callback type.  BATON is a 'struct change_rev_prop_baton *'. */

static svn_error_t *
change_rev_prop_body(void *baton, apr_pool_t *pool)
{
  struct change_rev_prop_baton *cb = baton;
  apr_hash_t *table;

  SVN_ERR(svn_fs_fs__revision_proplist(&table, cb->fs, cb->rev, pool));

  if (cb->old_value_p)
    {
      const svn_string_t *wanted_value = *cb->old_value_p;
      const svn_string_t *present_value = apr_hash_get(table, cb->name,
                                                       APR_HASH_KEY_STRING);
      if ((!wanted_value != !present_value)
          || (wanted_value && present_value
              && !svn_string_compare(wanted_value, present_value)))
        {
          /* What we expected isn't what we found. */
          return svn_error_createf(SVN_ERR_FS_PROP_BASEVALUE_MISMATCH, NULL,
                                   _("revprop '%s' has unexpected value in "
                                     "filesystem"),
                                   cb->name);
        }
      /* Fall through. */
    }
  apr_hash_set(table, cb->name, APR_HASH_KEY_STRING, cb->value);

  return set_revision_proplist(cb->fs, cb->rev, table, pool);
}

svn_error_t *
svn_fs_fs__change_rev_prop(svn_fs_t *fs,
                           svn_revnum_t rev,
                           const char *name,
                           const svn_string_t *const *old_value_p,
                           const svn_string_t *value,
                           apr_pool_t *pool)
{
  struct change_rev_prop_baton cb;

  SVN_ERR(svn_fs__check_fs(fs, TRUE));

  cb.fs = fs;
  cb.rev = rev;
  cb.name = name;
  cb.old_value_p = old_value_p;
  cb.value = value;

  return svn_fs_fs__with_write_lock(fs, change_rev_prop_body, &cb, pool);
}



/*** Transactions ***/

svn_error_t *
svn_fs_fs__get_txn_ids(const svn_fs_id_t **root_id_p,
                       const svn_fs_id_t **base_root_id_p,
                       svn_fs_t *fs,
                       const char *txn_name,
                       apr_pool_t *pool)
{
  transaction_t *txn;
  SVN_ERR(svn_fs_fs__get_txn(&txn, fs, txn_name, pool));
  *root_id_p = txn->root_id;
  *base_root_id_p = txn->base_id;
  return SVN_NO_ERROR;
}


/* Generic transaction operations.  */

svn_error_t *
svn_fs_fs__txn_prop(svn_string_t **value_p,
                    svn_fs_txn_t *txn,
                    const char *propname,
                    apr_pool_t *pool)
{
  apr_hash_t *table;
  svn_fs_t *fs = txn->fs;

  SVN_ERR(svn_fs__check_fs(fs, TRUE));
  SVN_ERR(svn_fs_fs__txn_proplist(&table, txn, pool));

  *value_p = apr_hash_get(table, propname, APR_HASH_KEY_STRING);

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__begin_txn(svn_fs_txn_t **txn_p,
                     svn_fs_t *fs,
                     svn_revnum_t rev,
                     apr_uint32_t flags,
                     apr_pool_t *pool)
{
  svn_string_t date;
  svn_prop_t prop;
  apr_array_header_t *props = apr_array_make(pool, 3, sizeof(svn_prop_t));

  SVN_ERR(svn_fs__check_fs(fs, TRUE));

  SVN_ERR(svn_fs_fs__create_txn(txn_p, fs, rev, pool));

  /* Put a datestamp on the newly created txn, so we always know
     exactly how old it is.  (This will help sysadmins identify
     long-abandoned txns that may need to be manually removed.)  When
     a txn is promoted to a revision, this property will be
     automatically overwritten with a revision datestamp. */
  date.data = svn_time_to_cstring(apr_time_now(), pool);
  date.len = strlen(date.data);

  prop.name = SVN_PROP_REVISION_DATE;
  prop.value = &date;
  APR_ARRAY_PUSH(props, svn_prop_t) = prop;

  /* Set temporary txn props that represent the requested 'flags'
     behaviors. */
  if (flags & SVN_FS_TXN_CHECK_OOD)
    {
      prop.name = SVN_FS__PROP_TXN_CHECK_OOD;
      prop.value = svn_string_create("true", pool);
      APR_ARRAY_PUSH(props, svn_prop_t) = prop;
    }

  if (flags & SVN_FS_TXN_CHECK_LOCKS)
    {
      prop.name = SVN_FS__PROP_TXN_CHECK_LOCKS;
      prop.value = svn_string_create("true", pool);
      APR_ARRAY_PUSH(props, svn_prop_t) = prop;
    }

  return svn_fs_fs__change_txn_props(*txn_p, props, pool);
}


/****** Packing FSFS shards *********/

/* Write a file FILENAME in directory FS_PATH, containing a single line
 * with the number REVNUM in ASCII decimal.  Move the file into place
 * atomically, overwriting any existing file.
 *
 * Similar to write_current(). */
static svn_error_t *
write_revnum_file(const char *fs_path,
                  const char *filename,
                  svn_revnum_t revnum,
                  apr_pool_t *scratch_pool)
{
  const char *final_path, *tmp_path;
  svn_stream_t *tmp_stream;

  final_path = svn_dirent_join(fs_path, filename, scratch_pool);
  SVN_ERR(svn_stream_open_unique(&tmp_stream, &tmp_path, fs_path,
                                   svn_io_file_del_none,
                                   scratch_pool, scratch_pool));
  SVN_ERR(svn_stream_printf(tmp_stream, scratch_pool, "%ld\n", revnum));
  SVN_ERR(svn_stream_close(tmp_stream));
  SVN_ERR(move_into_place(tmp_path, final_path, final_path, scratch_pool));
  return SVN_NO_ERROR;
}

/* Pack a single shard SHARD in REVS_DIR, using POOL for allocations.
   CANCEL_FUNC and CANCEL_BATON are what you think they are.

   If for some reason we detect a partial packing already performed, we
   remove the pack file and start again. */
static svn_error_t *
pack_shard(const char *revs_dir,
           const char *fs_path,
           apr_int64_t shard,
           int max_files_per_dir,
           svn_fs_pack_notify_t notify_func,
           void *notify_baton,
           svn_cancel_func_t cancel_func,
           void *cancel_baton,
           apr_pool_t *pool)
{
  const char *pack_file_path, *manifest_file_path, *shard_path;
  const char *pack_file_dir;
  svn_stream_t *pack_stream, *manifest_stream;
  svn_revnum_t start_rev, end_rev, rev;
  apr_off_t next_offset;
  apr_pool_t *iterpool;

  /* Some useful paths. */
  pack_file_dir = svn_dirent_join(revs_dir,
                        apr_psprintf(pool, "%" APR_INT64_T_FMT ".pack", shard),
                        pool);
  pack_file_path = svn_dirent_join(pack_file_dir, "pack", pool);
  manifest_file_path = svn_dirent_join(pack_file_dir, "manifest", pool);
  shard_path = svn_dirent_join(revs_dir,
                               apr_psprintf(pool, "%" APR_INT64_T_FMT, shard),
                               pool);

  /* Notify caller we're starting to pack this shard. */
  if (notify_func)
    SVN_ERR(notify_func(notify_baton, shard, svn_fs_pack_notify_start,
                        pool));

  /* Remove any existing pack file for this shard, since it is incomplete. */
  SVN_ERR(svn_io_remove_dir2(pack_file_dir, TRUE, cancel_func, cancel_baton,
                             pool));

  /* Create the new directory and pack and manifest files. */
  SVN_ERR(svn_io_dir_make(pack_file_dir, APR_OS_DEFAULT, pool));
  SVN_ERR(svn_stream_open_writable(&pack_stream, pack_file_path, pool,
                                    pool));
  SVN_ERR(svn_stream_open_writable(&manifest_stream, manifest_file_path,
                                   pool, pool));

  start_rev = (svn_revnum_t) (shard * max_files_per_dir);
  end_rev = (svn_revnum_t) ((shard + 1) * (max_files_per_dir) - 1);
  next_offset = 0;
  iterpool = svn_pool_create(pool);

  /* Iterate over the revisions in this shard, squashing them together. */
  for (rev = start_rev; rev <= end_rev; rev++)
    {
      svn_stream_t *rev_stream;
      apr_finfo_t finfo;
      const char *path;

      svn_pool_clear(iterpool);

      /* Get the size of the file. */
      path = svn_dirent_join(shard_path, apr_psprintf(iterpool, "%ld", rev),
                             iterpool);
      SVN_ERR(svn_io_stat(&finfo, path, APR_FINFO_SIZE, iterpool));

      /* Update the manifest. */
      SVN_ERR(svn_stream_printf(manifest_stream, iterpool, "%" APR_OFF_T_FMT
                                "\n", next_offset));
      next_offset += finfo.size;

      /* Copy all the bits from the rev file to the end of the pack file. */
      SVN_ERR(svn_stream_open_readonly(&rev_stream, path, iterpool, iterpool));
      SVN_ERR(svn_stream_copy3(rev_stream, svn_stream_disown(pack_stream,
                                                             iterpool),
                          cancel_func, cancel_baton, iterpool));
    }

  SVN_ERR(svn_stream_close(manifest_stream));
  SVN_ERR(svn_stream_close(pack_stream));
  SVN_ERR(svn_io_copy_perms(shard_path, pack_file_dir, pool));
  SVN_ERR(svn_io_set_file_read_only(pack_file_path, FALSE, pool));
  SVN_ERR(svn_io_set_file_read_only(manifest_file_path, FALSE, pool));

  /* Update the min-unpacked-rev file to reflect our newly packed shard.
   * (This doesn't update ffd->min_unpacked_rev.  That will be updated by
   * update_min_unpacked_rev() when necessary.) */
  SVN_ERR(write_revnum_file(fs_path, PATH_MIN_UNPACKED_REV,
                            (svn_revnum_t)((shard + 1) * max_files_per_dir),
                            iterpool));
  svn_pool_destroy(iterpool);

  /* Finally, remove the existing shard directory. */
  SVN_ERR(svn_io_remove_dir2(shard_path, TRUE, cancel_func, cancel_baton,
                             pool));

  /* Notify caller we're starting to pack this shard. */
  if (notify_func)
    SVN_ERR(notify_func(notify_baton, shard, svn_fs_pack_notify_end,
                        pool));

  return SVN_NO_ERROR;
}

struct pack_baton
{
  svn_fs_t *fs;
  svn_fs_pack_notify_t notify_func;
  void *notify_baton;
  svn_cancel_func_t cancel_func;
  void *cancel_baton;
};


/* The work-horse for svn_fs_fs__pack, called with the FS write lock.
   This implements the svn_fs_fs__with_write_lock() 'body' callback
   type.  BATON is a 'struct pack_baton *'.

   WARNING: if you add a call to this function, please note:
     The code currently assumes that any piece of code running with
     the write-lock set can rely on the ffd->min_unpacked_rev and
     ffd->min_unpacked_revprop caches to be up-to-date (and, by
     extension, on not having to use a retry when calling
     svn_fs_fs__path_rev_absolute() and friends).  If you add a call
     to this function, consider whether you have to call
     update_min_unpacked_rev() and update_min_unpacked_revprop()
     afterwards.
     See this thread: http://thread.gmane.org/1291206765.3782.3309.camel@edith
 */
static svn_error_t *
pack_body(void *baton,
          apr_pool_t *pool)
{
  struct pack_baton *pb = baton;
  int format, max_files_per_dir;
  apr_int64_t completed_shards;
  apr_int64_t i;
  svn_revnum_t youngest;
  apr_pool_t *iterpool;
  const char *data_path;
  svn_revnum_t min_unpacked_rev;

  SVN_ERR(read_format(&format, &max_files_per_dir, path_format(pb->fs, pool),
                      pool));
  SVN_ERR(check_format(format));

  /* If the repository isn't a new enough format, we don't support packing.
     Return a friendly error to that effect. */
  if (format < SVN_FS_FS__MIN_PACKED_FORMAT)
    return svn_error_createf(SVN_ERR_UNSUPPORTED_FEATURE, NULL,
      _("FSFS format (%d) too old to pack; please upgrade the filesystem."),
      format);

  /* If we aren't using sharding, we can't do any packing, so quit. */
  if (!max_files_per_dir)
    return SVN_NO_ERROR;

  SVN_ERR(read_min_unpacked_rev(&min_unpacked_rev,
                                path_min_unpacked_rev(pb->fs, pool),
                                pool));

  SVN_ERR(get_youngest(&youngest, pb->fs->path, pool));
  completed_shards = (youngest + 1) / max_files_per_dir;

  /* See if we've already completed all possible shards thus far. */
  if (min_unpacked_rev == (completed_shards * max_files_per_dir))
    return SVN_NO_ERROR;

  data_path = svn_dirent_join(pb->fs->path, PATH_REVS_DIR, pool);

  iterpool = svn_pool_create(pool);
  for (i = min_unpacked_rev / max_files_per_dir; i < completed_shards; i++)
    {
      svn_pool_clear(iterpool);

      if (pb->cancel_func)
        SVN_ERR(pb->cancel_func(pb->cancel_baton));

      SVN_ERR(pack_shard(data_path, pb->fs->path, i, max_files_per_dir,
                         pb->notify_func, pb->notify_baton,
                         pb->cancel_func, pb->cancel_baton, iterpool));
    }

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__pack(svn_fs_t *fs,
                svn_fs_pack_notify_t notify_func,
                void *notify_baton,
                svn_cancel_func_t cancel_func,
                void *cancel_baton,
                apr_pool_t *pool)
{
  struct pack_baton pb = { 0 };
  pb.fs = fs;
  pb.notify_func = notify_func;
  pb.notify_baton = notify_baton;
  pb.cancel_func = cancel_func;
  pb.cancel_baton = cancel_baton;
  return svn_fs_fs__with_write_lock(fs, pack_body, &pb, pool);
}
