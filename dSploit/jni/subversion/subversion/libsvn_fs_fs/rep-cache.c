/* rep-sharing.c --- the rep-sharing cache for fsfs
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

#include "svn_private_config.h"

#include "fs_fs.h"
#include "fs.h"
#include "rep-cache.h"
#include "../libsvn_fs/fs-loader.h"

#include "svn_path.h"

#include "private/svn_sqlite.h"

#include "rep-cache-db.h"

/* A few magic values */
#define REP_CACHE_SCHEMA_FORMAT   1

REP_CACHE_DB_SQL_DECLARE_STATEMENTS(statements);


static svn_error_t *
open_rep_cache(void *baton,
               apr_pool_t *pool)
{
  svn_fs_t *fs = baton;
  fs_fs_data_t *ffd = fs->fsap_data;
  svn_sqlite__db_t *sdb;
  const char *db_path;
  int version;

  /* Open (or create) the sqlite database.  It will be automatically
     closed when fs->pool is destoyed. */
  db_path = svn_dirent_join(fs->path, REP_CACHE_DB_NAME, pool);
  SVN_ERR(svn_sqlite__open(&sdb, db_path,
                           svn_sqlite__mode_rwcreate, statements,
                           0, NULL,
                           fs->pool, pool));

  SVN_ERR(svn_sqlite__read_schema_version(&version, sdb, pool));
  if (version < REP_CACHE_SCHEMA_FORMAT)
    {
      /* Must be 0 -- an uninitialized (no schema) database. Create
         the schema. Results in schema version of 1.  */
      SVN_ERR(svn_sqlite__exec_statements(sdb, STMT_CREATE_SCHEMA));
    }

  /* This is used as a flag that the database is available so don't
     set it earlier. */
  ffd->rep_cache_db = sdb;

  return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__open_rep_cache(svn_fs_t *fs,
                          apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  svn_error_t *err = svn_atomic__init_once(&ffd->rep_cache_db_opened,
                                           open_rep_cache, fs, pool);
  return svn_error_quick_wrap(err, _("Couldn't open rep-cache database"));
}

/* This function's caller ignores most errors it returns.
   If you extend this function, check the callsite to see if you have
   to make it not-ignore additional error codes.  */
svn_error_t *
svn_fs_fs__get_rep_reference(representation_t **rep,
                             svn_fs_t *fs,
                             svn_checksum_t *checksum,
                             apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  svn_sqlite__stmt_t *stmt;
  svn_boolean_t have_row;

  SVN_ERR_ASSERT(ffd->rep_sharing_allowed);
  if (! ffd->rep_cache_db)
    SVN_ERR(svn_fs_fs__open_rep_cache(fs, pool));

  /* We only allow SHA1 checksums in this table. */
  if (checksum->kind != svn_checksum_sha1)
    return svn_error_create(SVN_ERR_BAD_CHECKSUM_KIND, NULL,
                            _("Only SHA1 checksums can be used as keys in the "
                              "rep_cache table.\n"));

  SVN_ERR(svn_sqlite__get_statement(&stmt, ffd->rep_cache_db, STMT_GET_REP));
  SVN_ERR(svn_sqlite__bindf(stmt, "s",
                            svn_checksum_to_cstring(checksum, pool)));

  SVN_ERR(svn_sqlite__step(&have_row, stmt));
  if (have_row)
    {
      *rep = apr_pcalloc(pool, sizeof(**rep));
      (*rep)->sha1_checksum = svn_checksum_dup(checksum, pool);
      (*rep)->revision = svn_sqlite__column_revnum(stmt, 0);
      (*rep)->offset = svn_sqlite__column_int64(stmt, 1);
      (*rep)->size = svn_sqlite__column_int64(stmt, 2);
      (*rep)->expanded_size = svn_sqlite__column_int64(stmt, 3);
    }
  else
    *rep = NULL;

  /* Sanity check. */
  if (*rep)
    {
      svn_revnum_t youngest;

      youngest = ffd->youngest_rev_cache;
      if (youngest < (*rep)->revision)
      {
        /* Stale cache. */
        SVN_ERR(svn_fs_fs__youngest_rev(&youngest, fs, pool));

        /* Fresh cache. */
        if (youngest < (*rep)->revision)
          return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                   _("Youngest revision is r%ld, but "
                                     "rep-cache contains r%ld"),
                                   youngest, (*rep)->revision);
      }
    }

  return svn_sqlite__reset(stmt);
}

svn_error_t *
svn_fs_fs__set_rep_reference(svn_fs_t *fs,
                             representation_t *rep,
                             svn_boolean_t reject_dup,
                             apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  svn_sqlite__stmt_t *stmt;
  svn_error_t *err;

  SVN_ERR_ASSERT(ffd->rep_sharing_allowed);
  if (! ffd->rep_cache_db)
    SVN_ERR(svn_fs_fs__open_rep_cache(fs, pool));

  /* We only allow SHA1 checksums in this table. */
  if (rep->sha1_checksum == NULL)
    return svn_error_create(SVN_ERR_BAD_CHECKSUM_KIND, NULL,
                            _("Only SHA1 checksums can be used as keys in the "
                              "rep_cache table.\n"));

  SVN_ERR(svn_sqlite__get_statement(&stmt, ffd->rep_cache_db, STMT_SET_REP));
  SVN_ERR(svn_sqlite__bindf(stmt, "siiii",
                            svn_checksum_to_cstring(rep->sha1_checksum, pool),
                            (apr_int64_t) rep->revision,
                            (apr_int64_t) rep->offset,
                            (apr_int64_t) rep->size,
                            (apr_int64_t) rep->expanded_size));

  err = svn_sqlite__insert(NULL, stmt);
  if (err)
    {
      representation_t *old_rep;

      if (err->apr_err != SVN_ERR_SQLITE_CONSTRAINT)
        return svn_error_trace(err);

      svn_error_clear(err);

      /* Constraint failed so the mapping for SHA1_CHECKSUM->REP
         should exist.  If so, and the value is the same one we were
         about to write, that's cool -- just do nothing.  If, however,
         the value is *different*, that's a red flag!  */
      SVN_ERR(svn_fs_fs__get_rep_reference(&old_rep, fs, rep->sha1_checksum,
                                           pool));

      if (old_rep)
        {
          if (reject_dup && ((old_rep->revision != rep->revision)
                             || (old_rep->offset != rep->offset)
                             || (old_rep->size != rep->size)
                             || (old_rep->expanded_size != rep->expanded_size)))
            return svn_error_createf(SVN_ERR_FS_CORRUPT, NULL,
                                     apr_psprintf(pool,
                              _("Representation key for checksum '%%s' exists "
                                "in filesystem '%%s' with a different value "
                                "(%%ld,%%%s,%%%s,%%%s) than what we were about "
                                "to store (%%ld,%%%s,%%%s,%%%s)"),
                              APR_OFF_T_FMT, SVN_FILESIZE_T_FMT,
                              SVN_FILESIZE_T_FMT, APR_OFF_T_FMT,
                              SVN_FILESIZE_T_FMT, SVN_FILESIZE_T_FMT),
                 svn_checksum_to_cstring_display(rep->sha1_checksum, pool),
                 fs->path, old_rep->revision, old_rep->offset, old_rep->size,
                 old_rep->expanded_size, rep->revision, rep->offset, rep->size,
                 rep->expanded_size);
          else
            return SVN_NO_ERROR;
        }
      else
        {
          /* Something really odd at this point, we failed to insert the
             checksum AND failed to read an existing checksum.  Do we need
             to flag this? */
        }
    }

  return SVN_NO_ERROR;
}


svn_error_t *
svn_fs_fs__del_rep_reference(svn_fs_t *fs,
                             svn_revnum_t youngest,
                             apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  svn_sqlite__stmt_t *stmt;

  SVN_ERR_ASSERT(ffd->format >= SVN_FS_FS__MIN_REP_SHARING_FORMAT);
  if (! ffd->rep_cache_db)
    SVN_ERR(svn_fs_fs__open_rep_cache(fs, pool));

  SVN_ERR(svn_sqlite__get_statement(&stmt, ffd->rep_cache_db,
                                    STMT_DEL_REPS_YOUNGER_THAN_REV));
  SVN_ERR(svn_sqlite__bindf(stmt, "r", youngest));
  SVN_ERR(svn_sqlite__step_done(stmt));

  return SVN_NO_ERROR;
}
