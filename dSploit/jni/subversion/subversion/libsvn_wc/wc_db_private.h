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
 */

/* This file is not for general consumption; it should only be used by
   wc_db.c. */
#ifndef SVN_WC__I_AM_WC_DB
#error "You should not be using these data structures directly"
#endif /* SVN_WC__I_AM_WC_DB */

#ifndef WC_DB_PRIVATE_H
#define WC_DB_PRIVATE_H

#include "wc_db.h"


struct svn_wc__db_t {
  /* We need the config whenever we run into a new WC directory, in order
     to figure out where we should look for the corresponding datastore. */
  const svn_config_t *config;

  /* Should we attempt to automatically upgrade the database when it is
     opened, and found to be not-current?  */
  svn_boolean_t auto_upgrade;

  /* Should we ensure the WORK_QUEUE is empty when a WCROOT is opened?  */
  svn_boolean_t enforce_empty_wq;

  /* Map a given working copy directory to its relevant data.
     const char *local_abspath -> svn_wc__db_wcroot_t *wcroot  */
  apr_hash_t *dir_data;

  /* A few members to assist with caching of kind values for paths.  See
     get_path_kind() for use. */
  struct
  {
    svn_stringbuf_t *abspath;
    svn_node_kind_t kind;
    svn_boolean_t is_symlink;
  } parse_cache;

  /* As we grow the state of this DB, allocate that state here. */
  apr_pool_t *state_pool;
};


/* Hold information about an owned lock */
typedef struct svn_wc__db_wclock_t
{
  /* Relative path of the lock root */
  const char *local_relpath;

  /* Number of levels locked (0 for infinity) */
  int levels;
} svn_wc__db_wclock_t;


/** Hold information about a WCROOT.
 *
 * This structure is referenced by all per-directory handles underneath it.
 */
typedef struct svn_wc__db_wcroot_t {
  /* Location of this wcroot in the filesystem.  */
  const char *abspath;

  /* The SQLite database containing the metadata for everything in
     this wcroot.  */
  svn_sqlite__db_t *sdb;

  /* The WCROOT.id for this directory (and all its children).  */
  apr_int64_t wc_id;

  /* The format of this wcroot's metadata storage (see wc.h). If the
     format has not (yet) been determined, this will be UNKNOWN_FORMAT.  */
  int format;

  /* Array of svn_wc__db_wclock_t structures (not pointers!).
     Typically just one or two locks maximum. */
  apr_array_header_t *owned_locks;

  /* Map a working copy diretory to a cached adm_access baton.
     const char *local_abspath -> svn_wc_adm_access_t *adm_access */
  apr_hash_t *access_cache;

} svn_wc__db_wcroot_t;


/* */
svn_error_t *
svn_wc__db_close_many_wcroots(apr_hash_t *roots,
                              apr_pool_t *state_pool,
                              apr_pool_t *scratch_pool);


/* Construct a new svn_wc__db_wcroot_t. The WCROOT_ABSPATH and SDB parameters
   must have lifetime of at least RESULT_POOL.  */
svn_error_t *
svn_wc__db_pdh_create_wcroot(svn_wc__db_wcroot_t **wcroot,
                             const char *wcroot_abspath,
                             svn_sqlite__db_t *sdb,
                             apr_int64_t wc_id,
                             int format,
                             svn_boolean_t auto_upgrade,
                             svn_boolean_t enforce_empty_wq,
                             apr_pool_t *result_pool,
                             apr_pool_t *scratch_pool);


/* For a given LOCAL_ABSPATH, figure out what sqlite database (WCROOT) to
   use and the RELPATH within that wcroot.

   *LOCAL_RELPATH will be allocated within RESULT_POOL. Temporary allocations
   will be made in SCRATCH_POOL.

   *WCROOT will be allocated within DB->STATE_POOL.

   Certain internal structures will be allocated in DB->STATE_POOL.
*/
svn_error_t *
svn_wc__db_wcroot_parse_local_abspath(svn_wc__db_wcroot_t **wcroot,
                                      const char **local_relpath,
                                      svn_wc__db_t *db,
                                      const char *local_abspath,
                                      apr_pool_t *result_pool,
                                      apr_pool_t *scratch_pool);


/* Assert that the given WCROOT is usable.
   NOTE: the expression is multiply-evaluated!!  */
#define VERIFY_USABLE_WCROOT(wcroot)  SVN_ERR_ASSERT(               \
    (wcroot) != NULL && (wcroot)->format == SVN_WC__VERSION)


/* */
svn_error_t *
svn_wc__db_util_fetch_wc_id(apr_int64_t *wc_id,
                            svn_sqlite__db_t *sdb,
                            apr_pool_t *scratch_pool);

/* Open a connection in *SDB to the WC database found in the WC metadata
 * directory inside DIR_ABSPATH, having the filename SDB_FNAME.
 *
 * SMODE is passed to svn_sqlite__open().
 *
 * Register MY_STATEMENTS, or if that is null, the default set of WC DB
 * statements, as the set of statements to be prepared now and executed
 * later.  MY_STATEMENTS (the strings and the array itself) is not duplicated
 * internally, and should have a lifetime at least as long as RESULT_POOL.
 * See svn_sqlite__open() for details. */
svn_error_t *
svn_wc__db_util_open_db(svn_sqlite__db_t **sdb,
                        const char *dir_abspath,
                        const char *sdb_fname,
                        svn_sqlite__mode_t smode,
                        const char *const *my_statements,
                        apr_pool_t *result_pool,
                        apr_pool_t *scratch_pool);

/* Like svn_wc__db_read_info(), but taking WCROOT+LOCAL_RELPATH instead of
   DB+LOCAL_ABSPATH, and outputting repos ids instead of URL+UUID. */
svn_error_t *
svn_wc__db_read_info_internal(svn_wc__db_status_t *status,
                              svn_wc__db_kind_t *kind,
                              svn_revnum_t *revision,
                              const char **repos_relpath,
                              apr_int64_t *repos_id,
                              svn_revnum_t *changed_rev,
                              apr_time_t *changed_date,
                              const char **changed_author,
                              svn_depth_t *depth,
                              const svn_checksum_t **checksum,
                              const char **target,
                              const char **original_repos_relpath,
                              apr_int64_t *original_repos_id,
                              svn_revnum_t *original_revision,
                              svn_wc__db_lock_t **lock,
                              svn_filesize_t *recorded_size,
                              apr_time_t *recorded_mod_time,
                              const char **changelist,
                              svn_boolean_t *conflicted,
                              svn_boolean_t *op_root,
                              svn_boolean_t *had_props,
                              svn_boolean_t *props_mod,
                              svn_boolean_t *have_base,
                              svn_boolean_t *have_more_work,
                              svn_boolean_t *have_work,
                              svn_wc__db_wcroot_t *wcroot,
                              const char *local_relpath,
                              apr_pool_t *result_pool,
                              apr_pool_t *scratch_pool);


/* Transaction handling */

/* A callback which supplies WCROOTs and LOCAL_RELPATHs. */
typedef svn_error_t *(*svn_wc__db_txn_callback_t)(void *baton,
                                          svn_wc__db_wcroot_t *wcroot,
                                          const char *local_relpath,
                                          apr_pool_t *scratch_pool);


/* Run CB_FUNC in a SQLite transaction with CB_BATON, using WCROOT and
   LOCAL_RELPATH.  If callbacks require additional information, they may
   provide it using CB_BATON. */
svn_error_t *
svn_wc__db_with_txn(svn_wc__db_wcroot_t *wcroot,
                    const char *local_relpath,
                    svn_wc__db_txn_callback_t cb_func,
                    void *cb_baton,
                    apr_pool_t *scratch_pool);


#endif /* WC_DB_PRIVATE_H */
