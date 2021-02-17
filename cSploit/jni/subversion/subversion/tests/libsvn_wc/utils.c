/* utils.c --- wc/client test utilities
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

#include "svn_error.h"
#include "svn_client.h"
#include "svn_pools.h"

#include "utils.h"

#include "../svn_test_fs.h"

#include "../../libsvn_wc/wc.h"
#include "../../libsvn_wc/wc-queries.h"
#define SVN_WC__I_AM_WC_DB
#include "../../libsvn_wc/wc_db_private.h"


/* Create an empty repository and WC for the test TEST_NAME.  Set *REPOS_URL
 * to the URL of the new repository and *WC_ABSPATH to the root path of the
 * new WC.
 *
 * Create the repository and WC in subdirectories called
 * REPOSITORIES_WORK_DIR/TEST_NAME and WCS_WORK_DIR/TEST_NAME respectively,
 * within the current working directory.
 *
 * Register the repo and WC to be cleaned up when the test suite exits. */
static svn_error_t *
create_repos_and_wc(const char **repos_url,
                    const char **wc_abspath,
                    const char *test_name,
                    const svn_test_opts_t *opts,
                    apr_pool_t *pool)
{
  const char *repos_path = svn_relpath_join(REPOSITORIES_WORK_DIR, test_name,
                                            pool);
  const char *wc_path = svn_relpath_join(WCS_WORK_DIR, test_name, pool);

  /* Remove the repo and WC dirs if they already exist, to ensure the test
   * will run even if a previous failed attempt was not cleaned up. */
  SVN_ERR(svn_io_remove_dir2(repos_path, TRUE, NULL, NULL, pool));
  SVN_ERR(svn_io_remove_dir2(wc_path, TRUE, NULL, NULL, pool));

  /* Create the parent dirs of the repo and WC if necessary. */
  SVN_ERR(svn_io_make_dir_recursively(REPOSITORIES_WORK_DIR, pool));
  SVN_ERR(svn_io_make_dir_recursively(WCS_WORK_DIR, pool));

  /* Create a repos. Register it for clean-up. Set *REPOS_URL to its path. */
  {
    svn_repos_t *repos;

    /* Use a subpool to create the repository and then destroy the subpool
       so the repository's underlying filesystem is closed.  If opts->fs_type
       is BDB this prevents any attempt to open a second environment handle
       within the same process when we checkout the WC below.  BDB 4.4+ allows
       only a single environment handle to be open per process. */
    apr_pool_t *subpool = svn_pool_create(pool);

    SVN_ERR(svn_test__create_repos(&repos, repos_path, opts, subpool));
    SVN_ERR(svn_uri_get_file_url_from_dirent(repos_url, repos_path, pool));
    svn_pool_destroy(subpool);
  }

  /* Create a WC. Set *WC_ABSPATH to its path. */
  {
    svn_client_ctx_t *ctx;
    svn_opt_revision_t head_rev = { svn_opt_revision_head, {0} };

    SVN_ERR(svn_client_create_context(&ctx, pool));
    /* SVN_ERR(svn_config_get_config(&ctx->config, config_dir, pool)); */
    SVN_ERR(svn_dirent_get_absolute(wc_abspath, wc_path, pool));
    SVN_ERR(svn_client_checkout3(NULL, *repos_url, *wc_abspath,
                                 &head_rev, &head_rev, svn_depth_infinity,
                                 FALSE /* ignore_externals */,
                                 FALSE /* allow_unver_obstructions */,
                                 ctx, pool));
  }

  /* Register this WC for cleanup. */
  svn_test_add_dir_cleanup(*wc_abspath);

  return SVN_NO_ERROR;
}


WC_QUERIES_SQL_DECLARE_STATEMENTS(statements);

svn_error_t *
svn_test__create_fake_wc(const char *wc_abspath,
                         const char *extra_statements,
                         apr_pool_t *result_pool,
                         apr_pool_t *scratch_pool)
{
  const char *dotsvn_abspath = svn_dirent_join(wc_abspath, ".svn",
                                               scratch_pool);
  const char *db_abspath = svn_dirent_join(dotsvn_abspath, "wc.db",
                                           scratch_pool);
  svn_sqlite__db_t *sdb;
  const char **my_statements;
  int i;

  /* Allocate MY_STATEMENTS in RESULT_POOL because the SDB will continue to
   * refer to it over its lifetime. */
  my_statements = apr_palloc(result_pool, 6 * sizeof(const char *));
  my_statements[0] = statements[STMT_CREATE_SCHEMA];
  my_statements[1] = statements[STMT_CREATE_NODES];
  my_statements[2] = statements[STMT_CREATE_NODES_TRIGGERS];
  my_statements[3] = statements[STMT_CREATE_EXTERNALS];
  my_statements[4] = extra_statements;
  my_statements[5] = NULL;

  /* Create fake-wc/SUBDIR/.svn/ for placing the metadata. */
  SVN_ERR(svn_io_make_dir_recursively(dotsvn_abspath, scratch_pool));

  svn_error_clear(svn_io_remove_file2(db_abspath, FALSE, scratch_pool));
  SVN_ERR(svn_wc__db_util_open_db(&sdb, wc_abspath, "wc.db",
                                  svn_sqlite__mode_rwcreate, my_statements,
                                  result_pool, scratch_pool));
  for (i = 0; my_statements[i] != NULL; i++)
    SVN_ERR(svn_sqlite__exec_statements(sdb, /* my_statements[] */ i));

  return SVN_NO_ERROR;
}


svn_error_t *
svn_test__sandbox_create(svn_test__sandbox_t *sandbox,
                         const char *test_name,
                         const svn_test_opts_t *opts,
                         apr_pool_t *pool)
{
  sandbox->pool = pool;
  SVN_ERR(create_repos_and_wc(&sandbox->repos_url, &sandbox->wc_abspath,
                              test_name, opts, pool));
  SVN_ERR(svn_wc_context_create(&sandbox->wc_ctx, NULL, pool, pool));
  return SVN_NO_ERROR;
}
