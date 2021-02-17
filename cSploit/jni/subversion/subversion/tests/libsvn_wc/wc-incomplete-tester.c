/*
 * wc-incomplete-tester.c :  mark a directory incomplete at a given revision
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

#include "svn_pools.h"
#include "svn_types.h"
#include "svn_path.h"
#include "../../libsvn_wc/wc.h"
#include "../../libsvn_wc/wc_db.h"
#include <stdio.h>
#include <stdlib.h>

static svn_error_t *
incomplete(const char *wc_path,
           const char *rev_str,
           const char *repos_relpath,
           apr_pool_t *pool)
{
  svn_wc_context_t *wc_ctx;
  const char *local_abspath;
  apr_int64_t revnum;

  SVN_ERR(svn_wc_context_create(&wc_ctx, NULL, pool, pool));

  SVN_ERR(svn_path_cstring_to_utf8(&wc_path, wc_path, pool));
  wc_path = svn_dirent_canonicalize(wc_path, pool);
  SVN_ERR(svn_dirent_get_absolute(&local_abspath, wc_path, pool));

  SVN_ERR(svn_cstring_atoi64(&revnum, rev_str));

  if (repos_relpath)
    repos_relpath = svn_relpath_canonicalize(repos_relpath, pool);
  else
    SVN_ERR(svn_wc__db_read_info(NULL, NULL, NULL,
                                 &repos_relpath,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 wc_ctx->db, local_abspath, pool, pool));

  SVN_ERR(svn_wc__db_temp_op_start_directory_update(wc_ctx->db,
                                                    local_abspath,
                                                    repos_relpath,
                                                    (svn_revnum_t)revnum,
                                                    pool));

  return SVN_NO_ERROR;
}

int main(int argc, const char *argv[])
{
  apr_pool_t *pool;
  svn_error_t *err;

  if (argc != 3 && argc != 4)
    {
      fprintf(stderr,
              "Usage: wc-incomplete-tester WCPATH REVISION [REPOS_RELPATH]\n"
              "Mark WCPATH incomplete at REVISION [and REPOS_RELPATH]\n");
      exit(EXIT_FAILURE);
    }
          
  if (apr_initialize())
    {
      fprintf(stderr, "apr_initialize failed\n");
      exit(EXIT_FAILURE);
    }
  pool = svn_pool_create(NULL);

  err = incomplete(argv[1], argv[2], (argc == 4 ? argv[3] : NULL), pool);
  if (err)
    svn_handle_error2(err, stderr, TRUE, "wc-incomplete-tester: ");

  svn_pool_destroy(pool);
  apr_terminate();

  return EXIT_SUCCESS;
}
