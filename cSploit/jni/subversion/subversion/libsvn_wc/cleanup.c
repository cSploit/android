/*
 * cleanup.c:  handle cleaning up workqueue items
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



#include <string.h>

#include "svn_wc.h"
#include "svn_error.h"
#include "svn_pools.h"
#include "svn_io.h"
#include "svn_dirent_uri.h"

#include "wc.h"
#include "adm_files.h"
#include "lock.h"
#include "workqueue.h"

#include "private/svn_wc_private.h"
#include "svn_private_config.h"


/*** Recursively do log things. ***/

/* */
static svn_error_t *
can_be_cleaned(int *wc_format,
               svn_wc__db_t *db,
               const char *local_abspath,
               apr_pool_t *scratch_pool)
{
  SVN_ERR(svn_wc__internal_check_wc(wc_format, db,
                                    local_abspath, FALSE, scratch_pool));

  /* a "version" of 0 means a non-wc directory */
  if (*wc_format == 0)
    return svn_error_createf(SVN_ERR_WC_NOT_WORKING_COPY, NULL,
                             _("'%s' is not a working copy directory"),
                             svn_dirent_local_style(local_abspath,
                                                    scratch_pool));

  if (*wc_format < SVN_WC__WC_NG_VERSION)
    return svn_error_create(SVN_ERR_WC_UNSUPPORTED_FORMAT, NULL,
                            _("Log format too old, please use "
                              "Subversion 1.6 or earlier"));

  return SVN_NO_ERROR;
}

/* Do a modifed check for LOCAL_ABSPATH, and all working children, to force
   timestamp repair. */
static svn_error_t *
repair_timestamps(svn_wc__db_t *db,
                  const char *local_abspath,
                  svn_cancel_func_t cancel_func,
                  void *cancel_baton,
                  apr_pool_t *scratch_pool)
{
  svn_wc__db_kind_t kind;
  svn_wc__db_status_t status;

  if (cancel_func)
    SVN_ERR(cancel_func(cancel_baton));

  SVN_ERR(svn_wc__db_read_info(&status, &kind,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL, NULL, NULL,
                               NULL, NULL, NULL,
                               db, local_abspath, scratch_pool, scratch_pool));

  if (status == svn_wc__db_status_server_excluded
      || status == svn_wc__db_status_deleted
      || status == svn_wc__db_status_excluded
      || status == svn_wc__db_status_not_present)
    return SVN_NO_ERROR;

  if (kind == svn_wc__db_kind_file
      || kind == svn_wc__db_kind_symlink)
    {
      svn_boolean_t modified;
      SVN_ERR(svn_wc__internal_file_modified_p(&modified,
                                               db, local_abspath, FALSE,
                                               scratch_pool));
    }
  else if (kind == svn_wc__db_kind_dir)
    {
      apr_pool_t *iterpool = svn_pool_create(scratch_pool);
      const apr_array_header_t *children;
      int i;

      SVN_ERR(svn_wc__db_read_children_of_working_node(&children, db,
                                                       local_abspath,
                                                       scratch_pool,
                                                       iterpool));
      for (i = 0; i < children->nelts; ++i)
        {
          const char *child_abspath;

          svn_pool_clear(iterpool);

          child_abspath = svn_dirent_join(local_abspath,
                                          APR_ARRAY_IDX(children, i,
                                                        const char *),
                                          iterpool);

          SVN_ERR(repair_timestamps(db, child_abspath,
                                    cancel_func, cancel_baton, iterpool));
        }
      svn_pool_destroy(iterpool);
    }

  return SVN_NO_ERROR;
}

/* */
static svn_error_t *
cleanup_internal(svn_wc__db_t *db,
                 const char *dir_abspath,
                 svn_cancel_func_t cancel_func,
                 void *cancel_baton,
                 apr_pool_t *scratch_pool)
{
  int wc_format;
  const char *cleanup_abspath;

  /* Can we even work with this directory?  */
  SVN_ERR(can_be_cleaned(&wc_format, db, dir_abspath, scratch_pool));

  /* ### This fails if ADM_ABSPATH is locked indirectly via a
     ### recursive lock on an ancestor. */
  SVN_ERR(svn_wc__db_wclock_obtain(db, dir_abspath, -1, TRUE, scratch_pool));

  /* Run our changes before the subdirectories. We may not have to recurse
     if we blow away a subdir.  */
  if (wc_format >= SVN_WC__HAS_WORK_QUEUE)
    SVN_ERR(svn_wc__wq_run(db, dir_abspath, cancel_func, cancel_baton,
                           scratch_pool));

  SVN_ERR(svn_wc__db_get_wcroot(&cleanup_abspath, db, dir_abspath,
                                scratch_pool, scratch_pool));

#ifdef SVN_DEBUG
  SVN_ERR(svn_wc__db_verify(db, dir_abspath, scratch_pool));
#endif

  /* Perform these operations if we lock the entire working copy.
     Note that we really need to check a wcroot value and not
     svn_wc__check_wcroot() as that function, will just return true
     once we start sharing databases with externals.
   */
  if (strcmp(cleanup_abspath, dir_abspath) == 0)
    {
    /* Cleanup the tmp area of the admin subdir, if running the log has not
       removed it!  The logs have been run, so anything left here has no hope
       of being useful. */
      SVN_ERR(svn_wc__adm_cleanup_tmp_area(db, dir_abspath, scratch_pool));

      /* Remove unreferenced pristine texts */
      SVN_ERR(svn_wc__db_pristine_cleanup(db, dir_abspath, scratch_pool));
    }

  SVN_ERR(repair_timestamps(db, dir_abspath, cancel_func, cancel_baton,
                            scratch_pool));

  /* All done, toss the lock */
  SVN_ERR(svn_wc__db_wclock_release(db, dir_abspath, scratch_pool));

  return SVN_NO_ERROR;
}


/* ### possibly eliminate the WC_CTX parameter? callers really shouldn't
   ### be doing anything *but* running a cleanup, and we need a special
   ### DB anyway. ... *shrug* ... consider later.  */
svn_error_t *
svn_wc_cleanup3(svn_wc_context_t *wc_ctx,
                const char *local_abspath,
                svn_cancel_func_t cancel_func,
                void *cancel_baton,
                apr_pool_t *scratch_pool)
{
  svn_wc__db_t *db;

  SVN_ERR_ASSERT(svn_dirent_is_absolute(local_abspath));

  /* We need a DB that allows a non-empty work queue (though it *will*
     auto-upgrade). We'll handle everything manually.  */
  SVN_ERR(svn_wc__db_open(&db,
                          NULL /* ### config */, TRUE, FALSE,
                          scratch_pool, scratch_pool));

  SVN_ERR(cleanup_internal(db, local_abspath, cancel_func, cancel_baton,
                           scratch_pool));

  /* The DAV cache suffers from flakiness from time to time, and the
     pre-1.7 prescribed workarounds aren't as user-friendly in WC-NG. */
  SVN_ERR(svn_wc__db_base_clear_dav_cache_recursive(db, local_abspath,
                                                    scratch_pool));

  /* We're done with this DB, so proactively close it.  */
  SVN_ERR(svn_wc__db_close(db));

  return SVN_NO_ERROR;
}
