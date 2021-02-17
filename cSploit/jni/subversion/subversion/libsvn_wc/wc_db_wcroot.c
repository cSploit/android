/*
 * wc_db_wcroot.c :  supporting datastructures for the administrative database
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

#define SVN_WC__I_AM_WC_DB

#include <assert.h>

#include "svn_dirent_uri.h"

#include "wc.h"
#include "adm_files.h"
#include "wc_db_private.h"
#include "wc-queries.h"

#include "svn_private_config.h"

/* ### Same values as wc_db.c */
#define SDB_FILE  "wc.db"
#define UNKNOWN_WC_ID ((apr_int64_t) -1)
#define FORMAT_FROM_SDB (-1)



/* Get the format version from a wc-1 directory. If it is not a working copy
   directory, then it sets VERSION to zero and returns no error.  */
static svn_error_t *
get_old_version(int *version,
                const char *abspath,
                apr_pool_t *scratch_pool)
{
  svn_error_t *err;
  const char *format_file_path;
  svn_node_kind_t kind;

  /* Try reading the format number from the entries file.  */
  format_file_path = svn_wc__adm_child(abspath, SVN_WC__ADM_ENTRIES,
                                       scratch_pool);

  /* Since trying to open a non-existent file is quite expensive, try a
     quick stat call first. In wc-ng w/cs, this will be an early exit. */
  SVN_ERR(svn_io_check_path(format_file_path, &kind, scratch_pool));
  if (kind == svn_node_none)
    {
      *version = 0;
      return SVN_NO_ERROR;
    }

  err = svn_io_read_version_file(version, format_file_path, scratch_pool);
  if (err == NULL)
    return SVN_NO_ERROR;
  if (err->apr_err != SVN_ERR_BAD_VERSION_FILE_FORMAT
      && !APR_STATUS_IS_ENOENT(err->apr_err)
      && !APR_STATUS_IS_ENOTDIR(err->apr_err))
    return svn_error_createf(SVN_ERR_WC_MISSING, err, _("'%s' does not exist"),
                             svn_dirent_local_style(abspath, scratch_pool));
  svn_error_clear(err);

  /* This must be a really old working copy!  Fall back to reading the
     format file.

     Note that the format file might not exist in newer working copies
     (format 7 and higher), but in that case, the entries file should
     have contained the format number. */
  format_file_path = svn_wc__adm_child(abspath, SVN_WC__ADM_FORMAT,
                                       scratch_pool);
  err = svn_io_read_version_file(version, format_file_path, scratch_pool);
  if (err == NULL)
    return SVN_NO_ERROR;

  /* Whatever error may have occurred... we can just ignore. This is not
     a working copy directory. Signal the caller.  */
  svn_error_clear(err);

  *version = 0;
  return SVN_NO_ERROR;
}


/* A helper function to parse_local_abspath() which returns the on-disk KIND
   of LOCAL_ABSPATH, using DB and SCRATCH_POOL as needed.

   This function may do strange things, but at long as it comes up with the
   Right Answer, we should be happy.

   Sets *KIND to svn_node_dir for symlinks. */
static svn_error_t *
get_path_kind(svn_node_kind_t *kind,
              svn_boolean_t *is_symlink,
              svn_wc__db_t *db,
              const char *local_abspath,
              apr_pool_t *scratch_pool)
{
  /* This implements a *really* simple LRU cache, where "simple" is defined
     as "only one element".  In other words, we remember the most recently
     queried path, and nothing else.  This gives >80% cache hits. */

  if (db->parse_cache.abspath
        && strcmp(db->parse_cache.abspath->data, local_abspath) == 0)
    {
      /* Cache hit! */
      *kind = db->parse_cache.kind;
      *is_symlink = db->parse_cache.is_symlink;
      return SVN_NO_ERROR;
    }

  if (!db->parse_cache.abspath)
    {
      db->parse_cache.abspath = svn_stringbuf_create(local_abspath,
                                                     db->state_pool);
    }
  else
    {
      svn_stringbuf_set(db->parse_cache.abspath, local_abspath);
    }

  SVN_ERR(svn_io_check_special_path(local_abspath, kind,
                                    is_symlink, scratch_pool));

  db->parse_cache.kind = *kind;
  db->parse_cache.is_symlink = *is_symlink;

  return SVN_NO_ERROR;
}


/* */
static svn_error_t *
verify_no_work(svn_sqlite__db_t *sdb)
{
  svn_sqlite__stmt_t *stmt;
  svn_boolean_t have_row;

  SVN_ERR(svn_sqlite__get_statement(&stmt, sdb, STMT_LOOK_FOR_WORK));
  SVN_ERR(svn_sqlite__step(&have_row, stmt));
  SVN_ERR(svn_sqlite__reset(stmt));

  if (have_row)
    return svn_error_create(SVN_ERR_WC_CLEANUP_REQUIRED, NULL,
                            NULL /* nothing to add.  */);

  return SVN_NO_ERROR;
}


/* */
static apr_status_t
close_wcroot(void *data)
{
  svn_wc__db_wcroot_t *wcroot = data;
  svn_error_t *err;

  SVN_ERR_ASSERT_NO_RETURN(wcroot->sdb != NULL);

  err = svn_sqlite__close(wcroot->sdb);
  wcroot->sdb = NULL;
  if (err)
    {
      apr_status_t result = err->apr_err;
      svn_error_clear(err);
      return result;
    }

  return APR_SUCCESS;
}


svn_error_t *
svn_wc__db_open(svn_wc__db_t **db,
                const svn_config_t *config,
                svn_boolean_t auto_upgrade,
                svn_boolean_t enforce_empty_wq,
                apr_pool_t *result_pool,
                apr_pool_t *scratch_pool)
{
  *db = apr_pcalloc(result_pool, sizeof(**db));
  (*db)->config = config;
  (*db)->auto_upgrade = auto_upgrade;
  (*db)->enforce_empty_wq = enforce_empty_wq;
  (*db)->dir_data = apr_hash_make(result_pool);

  /* Don't need to initialize (*db)->parse_cache, due to the calloc above */

  (*db)->state_pool = result_pool;

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc__db_close(svn_wc__db_t *db)
{
  apr_pool_t *scratch_pool = db->state_pool;
  apr_hash_t *roots = apr_hash_make(scratch_pool);
  apr_hash_index_t *hi;

  /* Collect all the unique WCROOT structures, and empty out DIR_DATA.  */
  for (hi = apr_hash_first(scratch_pool, db->dir_data);
       hi;
       hi = apr_hash_next(hi))
    {
      svn_wc__db_wcroot_t *wcroot = svn__apr_hash_index_val(hi);
      const char *local_abspath = svn__apr_hash_index_key(hi);

      if (wcroot->sdb)
        apr_hash_set(roots, wcroot->abspath, APR_HASH_KEY_STRING, wcroot);

      apr_hash_set(db->dir_data, local_abspath, APR_HASH_KEY_STRING, NULL);
    }

  /* Run the cleanup for each WCROOT.  */
  return svn_error_trace(svn_wc__db_close_many_wcroots(roots, db->state_pool,
                                                       scratch_pool));
}


svn_error_t *
svn_wc__db_pdh_create_wcroot(svn_wc__db_wcroot_t **wcroot,
                             const char *wcroot_abspath,
                             svn_sqlite__db_t *sdb,
                             apr_int64_t wc_id,
                             int format,
                             svn_boolean_t auto_upgrade,
                             svn_boolean_t enforce_empty_wq,
                             apr_pool_t *result_pool,
                             apr_pool_t *scratch_pool)
{
  if (sdb != NULL)
    SVN_ERR(svn_sqlite__read_schema_version(&format, sdb, scratch_pool));

  /* If we construct a wcroot, then we better have a format.  */
  SVN_ERR_ASSERT(format >= 1);

  /* If this working copy is PRE-1.0, then simply bail out.  */
  if (format < 4)
    {
      return svn_error_createf(
        SVN_ERR_WC_UNSUPPORTED_FORMAT, NULL,
        _("Working copy format of '%s' is too old (%d); "
          "please check out your working copy again"),
        svn_dirent_local_style(wcroot_abspath, scratch_pool), format);
    }

  /* If this working copy is from a future version, then bail out.  */
  if (format > SVN_WC__VERSION)
    {
      return svn_error_createf(
        SVN_ERR_WC_UNSUPPORTED_FORMAT, NULL,
        _("This client is too old to work with the working copy at\n"
          "'%s' (format %d).\n"
          "You need to get a newer Subversion client. For more details, see\n"
          "  http://subversion.apache.org/faq.html#working-copy-format-change\n"
          ),
        svn_dirent_local_style(wcroot_abspath, scratch_pool),
        format);
    }

  /* Verify that no work items exists. If they do, then our integrity is
     suspect and, thus, we cannot use this database.  */
  if (format >= SVN_WC__HAS_WORK_QUEUE
      && (enforce_empty_wq || (format < SVN_WC__VERSION && auto_upgrade)))
    {
      svn_error_t *err = verify_no_work(sdb);
      if (err)
        {
          /* Special message for attempts to upgrade a 1.7-dev wc with
             outstanding workqueue items. */
          if (err->apr_err == SVN_ERR_WC_CLEANUP_REQUIRED
              && format < SVN_WC__VERSION && auto_upgrade)
            err = svn_error_quick_wrap(err, _("Cleanup with an older 1.7 "
                                              "client before upgrading with "
                                              "this client"));
          return svn_error_trace(err);
        }
    }

  /* Auto-upgrade the SDB if possible.  */
  if (format < SVN_WC__VERSION && auto_upgrade)
    SVN_ERR(svn_wc__upgrade_sdb(&format, wcroot_abspath, sdb, format,
                                scratch_pool));

  *wcroot = apr_palloc(result_pool, sizeof(**wcroot));

  (*wcroot)->abspath = wcroot_abspath;
  (*wcroot)->sdb = sdb;
  (*wcroot)->wc_id = wc_id;
  (*wcroot)->format = format;
  /* 8 concurrent locks is probably more than a typical wc_ng based svn client
     uses. */
  (*wcroot)->owned_locks = apr_array_make(result_pool, 8,
                                          sizeof(svn_wc__db_wclock_t));
  (*wcroot)->access_cache = apr_hash_make(result_pool);

  /* SDB will be NULL for pre-NG working copies. We only need to run a
     cleanup when the SDB is present.  */
  if (sdb != NULL)
    apr_pool_cleanup_register(result_pool, *wcroot, close_wcroot,
                              apr_pool_cleanup_null);
  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc__db_close_many_wcroots(apr_hash_t *roots,
                              apr_pool_t *state_pool,
                              apr_pool_t *scratch_pool)
{
  apr_hash_index_t *hi;

  for (hi = apr_hash_first(scratch_pool, roots); hi; hi = apr_hash_next(hi))
    {
      svn_wc__db_wcroot_t *wcroot = svn__apr_hash_index_val(hi);
      apr_status_t result;

      result = apr_pool_cleanup_run(state_pool, wcroot, close_wcroot);
      if (result != APR_SUCCESS)
        return svn_error_wrap_apr(result, NULL);
    }

  return SVN_NO_ERROR;
}


/* POOL may be NULL if the lifetime of LOCAL_ABSPATH is sufficient.  */
static const char *
compute_relpath(const svn_wc__db_wcroot_t *wcroot,
                const char *local_abspath,
                apr_pool_t *result_pool)
{
  const char *relpath = svn_dirent_is_child(wcroot->abspath, local_abspath,
                                            result_pool);
  if (relpath == NULL)
    return "";
  return relpath;
}


svn_error_t *
svn_wc__db_wcroot_parse_local_abspath(svn_wc__db_wcroot_t **wcroot,
                                      const char **local_relpath,
                                      svn_wc__db_t *db,
                                      const char *local_abspath,
                                      apr_pool_t *result_pool,
                                      apr_pool_t *scratch_pool)
{
  const char *local_dir_abspath;
  const char *original_abspath = local_abspath;
  svn_node_kind_t kind;
  const char *build_relpath;
  svn_wc__db_wcroot_t *probe_wcroot;
  svn_wc__db_wcroot_t *found_wcroot = NULL;
  const char *scan_abspath;
  svn_sqlite__db_t *sdb;
  svn_boolean_t moved_upwards = FALSE;
  svn_boolean_t always_check = FALSE;
  svn_boolean_t is_symlink;
  int wc_format = 0;
  const char *adm_relpath;

  /* ### we need more logic for finding the database (if it is located
     ### outside of the wcroot) and then managing all of that within DB.
     ### for now: play quick & dirty. */

  probe_wcroot = apr_hash_get(db->dir_data, local_abspath,
                              APR_HASH_KEY_STRING);
  if (probe_wcroot != NULL)
    {
      *wcroot = probe_wcroot;

      /* We got lucky. Just return the thing BEFORE performing any I/O.  */
      /* ### validate SMODE against how we opened wcroot->sdb? and against
         ### DB->mode? (will we record per-dir mode?)  */

      /* ### for most callers, we could pass NULL for result_pool.  */
      *local_relpath = compute_relpath(probe_wcroot, local_abspath,
                                       result_pool);

      return SVN_NO_ERROR;
    }

  /* ### at some point in the future, we may need to find a way to get
     ### rid of this stat() call. it is going to happen for EVERY call
     ### into wc_db which references a file. calls for directories could
     ### get an early-exit in the hash lookup just above.  */
  SVN_ERR(get_path_kind(&kind, &is_symlink, db, local_abspath, scratch_pool));
  if (kind != svn_node_dir || is_symlink)
    {
      /* If the node specified by the path is NOT present, then it cannot
         possibly be a directory containing ".svn/wc.db".

         If it is a file, then it cannot contain ".svn/wc.db".

         For both of these cases, strip the basename off of the path and
         move up one level. Keep record of what we strip, though, since
         we'll need it later to construct local_relpath.  */
      svn_dirent_split(&local_dir_abspath, &build_relpath, local_abspath,
                       scratch_pool);

      /* Is this directory in our hash?  */
      probe_wcroot = apr_hash_get(db->dir_data, local_dir_abspath,
                                  APR_HASH_KEY_STRING);
      if (probe_wcroot != NULL)
        {
          const char *dir_relpath;

          *wcroot = probe_wcroot;

          /* Stashed directory's local_relpath + basename. */
          dir_relpath = compute_relpath(probe_wcroot, local_dir_abspath,
                                        NULL);
          *local_relpath = svn_relpath_join(dir_relpath,
                                            build_relpath,
                                            result_pool);
          return SVN_NO_ERROR;
        }

      /* If the requested path is not on the disk, then we don't know how
         many ancestors need to be scanned until we start hitting content
         on the disk. Set ALWAYS_CHECK to keep looking for .svn/entries
         rather than bailing out after the first check.  */
      if (kind == svn_node_none)
        always_check = TRUE;

      /* Start the scanning at LOCAL_DIR_ABSPATH.  */
      local_abspath = local_dir_abspath;
    }
  else
    {
      /* Start the local_relpath empty. If *this* directory contains the
         wc.db, then relpath will be the empty string.  */
      build_relpath = "";

      /* Remember the dir containing LOCAL_ABSPATH (they're the same).  */
      local_dir_abspath = local_abspath;
    }

  /* LOCAL_ABSPATH refers to a directory at this point. At this point,
     we've determined that an associated WCROOT is NOT in the DB's hash
     table for this directory. Let's find an existing one in the ancestors,
     or create one when we find the actual wcroot.  */

  /* Assume that LOCAL_ABSPATH is a directory, and look for the SQLite
     database in the right place. If we find it... great! If not, then
     peel off some components, and try again. */

  adm_relpath = svn_wc_get_adm_dir(scratch_pool);
  while (TRUE)
    {
      svn_error_t *err;
      svn_node_kind_t adm_subdir_kind;

      const char *adm_subdir = svn_dirent_join(local_abspath, adm_relpath,
                                               scratch_pool);

      SVN_ERR(svn_io_check_path(adm_subdir, &adm_subdir_kind, scratch_pool));

      if (adm_subdir_kind == svn_node_dir)
        {
          /* We always open the database in read/write mode.  If the database
             isn't writable in the filesystem, SQLite will internally open
             it as read-only, and we'll get an error if we try to do a write
             operation.

             We could decide what to do on a per-operation basis, but since
             we're caching database handles, it make sense to be as permissive
             as the filesystem allows. */
          err = svn_wc__db_util_open_db(&sdb, local_abspath, SDB_FILE,
                                        svn_sqlite__mode_readwrite, NULL,
                                        db->state_pool, scratch_pool);
          if (err == NULL)
            {
#ifdef SVN_DEBUG
              /* Install self-verification trigger statements. */
              SVN_ERR(svn_sqlite__exec_statements(sdb,
                                                  STMT_VERIFICATION_TRIGGERS));
#endif
              break;
            }
          if (err->apr_err != SVN_ERR_SQLITE_ERROR
              && !APR_STATUS_IS_ENOENT(err->apr_err))
            return svn_error_trace(err);
          svn_error_clear(err);

          /* If we have not moved upwards, then check for a wc-1 working copy.
             Since wc-1 has a .svn in every directory, and we didn't find one
             in the original directory, then we aren't looking at a wc-1.

             If the original path is not present, then we have to check on every
             iteration. The content may be the immediate parent, or possibly
             five ancetors higher. We don't test for directory presence (just
             for the presence of subdirs/files), so we don't know when we can
             stop checking ... so just check always.  */
          if (!moved_upwards || always_check)
            {
              SVN_ERR(get_old_version(&wc_format, local_abspath,
                                      scratch_pool));
              if (wc_format != 0)
                break;
            }
        }

      /* We couldn't open the SDB within the specified directory, so
         move up one more directory. */
      if (svn_dirent_is_root(local_abspath, strlen(local_abspath)))
        {
          /* Hit the root without finding a wcroot. */

          /* The wcroot could be a symlink to a directory.
           * (Issue #2557, #3987). If so, try again, this time scanning
           * for a db within the directory the symlink points to,
           * rather than within the symlink's parent directory. */
          if (is_symlink)
            {
              svn_node_kind_t resolved_kind;

              local_abspath = original_abspath;

              SVN_ERR(svn_io_check_resolved_path(local_abspath,
                                                 &resolved_kind,
                                                 scratch_pool));
              if (resolved_kind == svn_node_dir)
                {
                  /* Is this directory recorded in our hash?  */
                  found_wcroot = apr_hash_get(db->dir_data, local_abspath,
                                              APR_HASH_KEY_STRING);
                  if (found_wcroot)
                    break;

try_symlink_as_dir:
                  kind = svn_node_dir;
                  is_symlink = FALSE;
                  moved_upwards = FALSE;
                  local_dir_abspath = local_abspath;
                  build_relpath = "";

                  continue;
                }
            }

          return svn_error_createf(SVN_ERR_WC_NOT_WORKING_COPY, NULL,
                                   _("'%s' is not a working copy"),
                                   svn_dirent_local_style(original_abspath,
                                                          scratch_pool));
        }

      local_abspath = svn_dirent_dirname(local_abspath, scratch_pool);

      moved_upwards = TRUE;

      /* Is the parent directory recorded in our hash?  */
      found_wcroot = apr_hash_get(db->dir_data, local_abspath,
                                  APR_HASH_KEY_STRING);
      if (found_wcroot != NULL)
        break;
    }

  if (found_wcroot != NULL)
    {
      /* We found a hash table entry for an ancestor, so we stopped scanning
         since all subdirectories use the same WCROOT.  */
      *wcroot = found_wcroot;
    }
  else if (wc_format == 0)
    {
      /* We finally found the database. Construct a wcroot_t for it.  */

      apr_int64_t wc_id;
      svn_error_t *err;

      err = svn_wc__db_util_fetch_wc_id(&wc_id, sdb, scratch_pool);
      if (err)
        {
          if (err->apr_err == SVN_ERR_WC_CORRUPT)
            return svn_error_quick_wrap(
              err, apr_psprintf(scratch_pool,
                                _("Missing a row in WCROOT for '%s'."),
                                svn_dirent_local_style(original_abspath,
                                                       scratch_pool)));
          return svn_error_trace(err);
        }

      /* WCROOT.local_abspath may be NULL when the database is stored
         inside the wcroot, but we know the abspath is this directory
         (ie. where we found it).  */

      err = svn_wc__db_pdh_create_wcroot(wcroot,
                            apr_pstrdup(db->state_pool, local_abspath),
                            sdb, wc_id, FORMAT_FROM_SDB,
                            db->auto_upgrade, db->enforce_empty_wq,
                            db->state_pool, scratch_pool);
      if (err && err->apr_err == SVN_ERR_WC_UNSUPPORTED_FORMAT && is_symlink)
        {
          /* We found an unsupported WC after traversing upwards from a
           * symlink. Fall through to code below to check if the symlink
           * points at a supported WC. */
          svn_error_clear(err);
          *wcroot = NULL;
        }
      else
        SVN_ERR(err);
    }
  else
    {
      /* We found a wc-1 working copy directory.  */
      SVN_ERR(svn_wc__db_pdh_create_wcroot(wcroot,
                            apr_pstrdup(db->state_pool, local_abspath),
                            NULL, UNKNOWN_WC_ID, wc_format,
                            db->auto_upgrade, db->enforce_empty_wq,
                            db->state_pool, scratch_pool));
    }

  if (*wcroot)
    {
      const char *dir_relpath;

      /* The subdirectory's relpath is easily computed relative to the
         wcroot that we just found.  */
      dir_relpath = compute_relpath(*wcroot, local_dir_abspath, NULL);

      /* And the result local_relpath may include a filename.  */
      *local_relpath = svn_relpath_join(dir_relpath, build_relpath, result_pool);
    }

  if (is_symlink)
    {
      svn_boolean_t retry_if_dir = FALSE;
      svn_wc__db_status_t status;
      svn_boolean_t conflicted;
      svn_error_t *err;

      /* Check if the symlink is versioned or obstructs a versioned node
       * in this DB -- in that case, use this wcroot. Else, if the symlink
       * points to a directory, try to find a wcroot in that directory
       * instead. */
      
      if (*wcroot)
        {
          err = svn_wc__db_read_info_internal(&status, NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, &conflicted,
                                              NULL, NULL, NULL, NULL, NULL,
                                              NULL, *wcroot, *local_relpath,
                                              scratch_pool, scratch_pool);
          if (err)
            {
              if (err->apr_err != SVN_ERR_WC_PATH_NOT_FOUND
                  && !SVN_WC__ERR_IS_NOT_CURRENT_WC(err))
                return svn_error_trace(err);

              svn_error_clear(err);
              retry_if_dir = TRUE; /* The symlink is unversioned. */
            }
          else
            {
              /* The symlink is versioned, or obstructs a versioned node.
               * Ignore non-conflicted not-present/excluded nodes.
               * This allows the symlink to redirect the wcroot query to a
               * directory, regardless of 'invisible' nodes in this WC. */
              retry_if_dir = ((status == svn_wc__db_status_not_present ||
                               status == svn_wc__db_status_excluded ||
                               status == svn_wc__db_status_server_excluded)
                              && !conflicted);
            }
        }
      else
        retry_if_dir = TRUE;

      if (retry_if_dir)
        {
          svn_node_kind_t resolved_kind;

          SVN_ERR(svn_io_check_resolved_path(original_abspath,
                                             &resolved_kind,
                                             scratch_pool));
          if (resolved_kind == svn_node_dir)
            {
              local_abspath = original_abspath;
              goto try_symlink_as_dir;
            }
        }
    } 

  /* We've found the appropriate WCROOT for the requested path. Stash
     it into that path's directory.  */
  apr_hash_set(db->dir_data,
               apr_pstrdup(db->state_pool, local_dir_abspath),
               APR_HASH_KEY_STRING,
               *wcroot);

  /* Did we traverse up to parent directories?  */
  if (!moved_upwards)
    {
      /* We did NOT move to a parent of the original requested directory.
         We've constructed and filled in a WCROOT for the request, so we
         are done.  */
      return SVN_NO_ERROR;
    }

  /* The WCROOT that we just found/built was for the LOCAL_ABSPATH originally
     passed into this function. We stepped *at least* one directory above that.
     We should now associate the WROOT for each parent directory that does
     not (yet) have one.  */

  scan_abspath = local_dir_abspath;

  do
    {
      const char *parent_dir = svn_dirent_dirname(scan_abspath, scratch_pool);
      svn_wc__db_wcroot_t *parent_wcroot;

      parent_wcroot = apr_hash_get(db->dir_data, parent_dir,
                                   APR_HASH_KEY_STRING);
      if (parent_wcroot == NULL)
        {
          apr_hash_set(db->dir_data, apr_pstrdup(db->state_pool, parent_dir),
                       APR_HASH_KEY_STRING, *wcroot);
        }

      /* Move up a directory, stopping when we reach the directory where
         we found/built the WCROOT.  */
      scan_abspath = parent_dir;
    }
  while (strcmp(scan_abspath, local_abspath) != 0);

  return SVN_NO_ERROR;
}


svn_error_t *
svn_wc__db_drop_root(svn_wc__db_t *db,
                     const char *local_abspath,
                     apr_pool_t *scratch_pool)
{
  svn_wc__db_wcroot_t *root_wcroot = apr_hash_get(db->dir_data, local_abspath,
                                                  APR_HASH_KEY_STRING);
  apr_hash_index_t *hi;
  apr_status_t result;

  if (!root_wcroot)
    return SVN_NO_ERROR;

  if (strcmp(root_wcroot->abspath, local_abspath) != 0)
    return svn_error_createf(SVN_ERR_WC_NOT_WORKING_COPY, NULL,
                             _("'%s' is not a working copy root"),
                             svn_dirent_local_style(local_abspath,
                                                    scratch_pool));

  for (hi = apr_hash_first(scratch_pool, db->dir_data);
       hi;
       hi = apr_hash_next(hi))
    {
      svn_wc__db_wcroot_t *wcroot = svn__apr_hash_index_val(hi);

      if (wcroot == root_wcroot)
        apr_hash_set(db->dir_data,
                     svn__apr_hash_index_key(hi), APR_HASH_KEY_STRING, NULL);
    }

  result = apr_pool_cleanup_run(db->state_pool, root_wcroot, close_wcroot);
  if (result != APR_SUCCESS)
    return svn_error_wrap_apr(result, NULL);

  return SVN_NO_ERROR;
}
