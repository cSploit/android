/*
 * wc-lock-tester.c :  wrapper around svn_wc__acquire_write_lock()
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

#include "svn_types.h"
#include "svn_pools.h"

#include "svn_cmdline.h"
#include "svn_dirent_uri.h"
#include "svn_path.h"
#include "svn_wc.h"

#include "private/svn_wc_private.h"
#include "../../libsvn_wc/wc.h"
#include "../../libsvn_wc/wc_db.h"

#include "svn_private_config.h"

#define USAGE_MSG \
  "Usage: %s [-r|-1] DIRNAME\n" \
  "\n" \
  "Locks one directory (-1), or a tree recursively (-r)\n"

static svn_error_t *
obtain_lock(const char *path, svn_boolean_t recursive,
            apr_pool_t *scratch_pool)
{
  const char *local_abspath;
  svn_wc_context_t *wc_ctx;

  SVN_ERR(svn_path_cstring_to_utf8(&path, path, scratch_pool));
  SVN_ERR(svn_dirent_get_absolute(&local_abspath, path, scratch_pool));

      SVN_ERR(svn_wc_context_create(&wc_ctx, NULL, scratch_pool,
                                    scratch_pool));

  if (recursive)
    {
      /* The WC-NG way */
      SVN_ERR(svn_wc__acquire_write_lock(NULL, wc_ctx, local_abspath, FALSE,
                                         scratch_pool, scratch_pool));
    }
  else
    {
      SVN_ERR(svn_wc__db_wclock_obtain(wc_ctx->db, local_abspath, 0, FALSE,
                                       scratch_pool));
    }

  SVN_ERR(svn_cmdline_printf(scratch_pool, "Lock on '%s' obtained, and we "
                             "are not going to release it.\n",
                             svn_dirent_local_style(local_abspath,
                                                    scratch_pool)));

  return SVN_NO_ERROR;
}

int
main(int argc, const char *argv[])
{
  apr_pool_t *pool;
  int exit_code = EXIT_SUCCESS;
  svn_error_t *err;
  svn_boolean_t recursive;

  if (argc != 3
      || (strcmp(argv[1], "-1") && apr_strnatcmp(argv[1], "-r")))
    {
      fprintf(stderr, USAGE_MSG, argv[0]);
      exit(EXIT_FAILURE);
    }

  if (apr_initialize() != APR_SUCCESS)
    {
      fprintf(stderr, "apr_initialize() failed.\n");
      exit(1);
    }

  /* set up the global pool */
  pool = svn_pool_create(NULL);

  recursive = (strcmp(argv[1], "-1") != 0);

  err = obtain_lock(argv[2], recursive, pool);

  if (err)
    {
      svn_handle_error2(err, stderr, FALSE, "wc-lock-tester: ");
      svn_error_clear(err);
      exit_code = EXIT_FAILURE;
    }

  /* Clean up, and get outta here */
  svn_pool_destroy(pool);
  apr_terminate();

  return exit_code;
}
