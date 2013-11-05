/*
 * activity.c: DeltaV activity handling
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

#include <errno.h>

#include <apr_md5.h>

#include <httpd.h>
#include <mod_dav.h>

#include "svn_checksum.h"
#include "svn_error.h"
#include "svn_io.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_fs.h"
#include "svn_props.h"
#include "svn_repos.h"
#include "svn_pools.h"

#include "private/svn_fs_private.h"

#include "dav_svn.h"


/* Escape ACTIVITY_ID to be safely usable as a filename.  Simply
   returns the MD5 checksum of the id.
*/
static const char *
escape_activity(const char *activity_id, apr_pool_t *pool)
{
  svn_checksum_t *checksum;
  svn_error_clear(svn_checksum(&checksum, svn_checksum_md5, activity_id,
                               strlen(activity_id), pool));
  return svn_checksum_to_cstring_display(checksum, pool);
}

/* Return filename for ACTIVITY_ID under the repository in REPOS. */
static const char *
activity_pathname(const dav_svn_repos *repos, const char *activity_id)
{
  return svn_dirent_join(repos->activities_db,
                         escape_activity(activity_id, repos->pool),
                         repos->pool);
}

/* Return the transaction name of the activity stored in file
   PATHNAME, or NULL if PATHNAME cannot be read for any reason. */
static const char *
read_txn(const char *pathname, apr_pool_t *pool)
{
  apr_file_t *activity_file;
  apr_pool_t *iterpool = svn_pool_create(pool);
  apr_size_t len;
  svn_error_t *err = SVN_NO_ERROR;
  char *txn_name = apr_palloc(pool, SVN_FS__TXN_MAX_LEN+1);
  int i;

  /* Try up to 10 times to read the txn name, retrying on ESTALE
     (stale NFS file handle because of dav_svn__store_activity
     renaming the activity file into place).
   */
  for (i = 0; i < 10; i++)
    {
      svn_error_clear(err);
      svn_pool_clear(iterpool);

      err = svn_io_file_open(&activity_file, pathname,
                             APR_READ  | APR_BUFFERED,
                             APR_OS_DEFAULT, iterpool);
      if (err)
        {
#ifdef ESTALE
          if (APR_TO_OS_ERROR(err->apr_err) == ESTALE)
            /* Retry on ESTALE... */
            continue;
#endif
          /* ...else bail. */
          break;
        }

      len = SVN_FS__TXN_MAX_LEN;
      err = svn_io_read_length_line(activity_file, txn_name, &len, iterpool);
      if (err)
        {
#ifdef ESTALE
          if (APR_TO_OS_ERROR(err->apr_err) == ESTALE)
            continue;
#endif
          break;
        }

      err = svn_io_file_close(activity_file, iterpool);
#ifdef ESTALE
      if (err)
        {
          if (APR_TO_OS_ERROR(err->apr_err) == ESTALE)
            {
              /* No retry, just completely ignore this ESTALE. */
              svn_error_clear(err);
              err = SVN_NO_ERROR;
            }
        }
#endif

      /* We have a txn_name or had a non-ESTALE close failure; either
         way, we're finished. */
      break;
    }
  svn_pool_destroy(iterpool);

  /* ### let's just assume that any error means the
     ### activity/transaction doesn't exist */
  if (err)
    {
      svn_error_clear(err);
      return NULL;
    }

  return txn_name;
}


const char *
dav_svn__get_txn(const dav_svn_repos *repos, const char *activity_id)
{
  return read_txn(activity_pathname(repos, activity_id), repos->pool);
}


dav_error *
dav_svn__delete_activity(const dav_svn_repos *repos, const char *activity_id)
{
  dav_error *err = NULL;
  const char *pathname;
  const char *txn_name;
  svn_error_t *serr;

  /* gstein sez: If the activity ID is not in the database, return a
     404.  If the transaction is not present or is immutable, return a
     204.  For all other failures, return a 500. */

  pathname = activity_pathname(repos, activity_id);
  txn_name = read_txn(pathname, repos->pool);
  if (txn_name == NULL)
    {
      return dav_svn__new_error(repos->pool, HTTP_NOT_FOUND, 0,
                                "could not find activity.");
    }

  /* After this point, we have to cleanup the value and database. */
  if (*txn_name)
    {
      if ((err = dav_svn__abort_txn(repos, txn_name, repos->pool)))
        return err;
    }

  /* Finally, we remove the activity from the activities database. */
  serr = svn_io_remove_file2(pathname, FALSE, repos->pool);
  if (serr)
    err = dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                               "unable to remove activity.",
                               repos->pool);

  return err;
}


dav_error *
dav_svn__store_activity(const dav_svn_repos *repos,
                        const char *activity_id,
                        const char *txn_name)
{
  const char *final_path;
  const char *tmp_path;
  const char *activity_contents;
  svn_error_t *err;

  /* Create activities directory if it does not yet exist. */
  err = svn_io_make_dir_recursively(repos->activities_db, repos->pool);
  if (err != NULL)
    return dav_svn__convert_err(err, HTTP_INTERNAL_SERVER_ERROR,
                                "could not initialize activity db.",
                                repos->pool);

  final_path = activity_pathname(repos, activity_id);

  activity_contents = apr_psprintf(repos->pool, "%s\n%s\n",
                                   txn_name, activity_id);

  /* ### is there another directory we already have and can write to? */
  err = svn_io_write_unique(&tmp_path,
                            svn_dirent_dirname(final_path, repos->pool),
                            activity_contents, strlen(activity_contents),
                            svn_io_file_del_none, repos->pool);
  if (err)
    {
      svn_error_t *serr = svn_error_quick_wrap(err,
                                               "Can't write activity db");

      /* Try to remove the tmp file, but we already have an error... */
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "could not write files.",
                                  repos->pool);
    }

  err = svn_io_file_rename(tmp_path, final_path, repos->pool);
  if (err)
    {
      svn_error_clear(svn_io_remove_file2(tmp_path, TRUE, repos->pool));
      return dav_svn__convert_err(err, HTTP_INTERNAL_SERVER_ERROR,
                                  "could not replace files.",
                                  repos->pool);
    }

  return NULL;
}


dav_error *
dav_svn__create_txn(const dav_svn_repos *repos,
                    const char **ptxn_name,
                    apr_pool_t *pool)
{
  svn_revnum_t rev;
  svn_fs_txn_t *txn;
  svn_error_t *serr;
  apr_hash_t *revprop_table = apr_hash_make(pool);

  if (repos->username)
    {
      apr_hash_set(revprop_table, SVN_PROP_REVISION_AUTHOR, APR_HASH_KEY_STRING,
                   svn_string_create(repos->username, pool));
    }

  serr = svn_fs_youngest_rev(&rev, repos->fs, pool);
  if (serr != NULL)
    {
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "could not determine youngest revision",
                                  repos->pool);
    }

  serr = svn_repos_fs_begin_txn_for_commit2(&txn, repos->repos, rev,
                                            revprop_table,
                                            repos->pool);
  if (serr != NULL)
    {
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "could not begin a transaction",
                                  repos->pool);
    }

  serr = svn_fs_txn_name(ptxn_name, txn, pool);
  if (serr != NULL)
    {
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "could not fetch transaction name",
                                  repos->pool);
    }

  return NULL;
}


dav_error *
dav_svn__abort_txn(const dav_svn_repos *repos,
                   const char *txn_name,
                   apr_pool_t *pool)
{
  svn_error_t *serr;
  svn_fs_txn_t *txn;

  /* If we fail only because the transaction doesn't exist, don't
     sweat it (but then, also don't try to remove it). */
  if ((serr = svn_fs_open_txn(&txn, repos->fs, txn_name, pool)))
    {
      if (serr->apr_err == SVN_ERR_FS_NO_SUCH_TRANSACTION)
        {
          svn_error_clear(serr);
          serr = SVN_NO_ERROR;
        }
      else
        {
          return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                      "could not open transaction.", pool);
        }
    }
  else if ((serr = svn_fs_abort_txn(txn, pool)))
    {
      return dav_svn__convert_err(serr, HTTP_INTERNAL_SERVER_ERROR,
                                  "could not abort transaction.", pool);
    }
  return NULL;
}
