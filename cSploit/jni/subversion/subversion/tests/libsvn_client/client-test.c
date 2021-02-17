/*
 * Regression tests for logic in the libsvn_client library.
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



#define SVN_DEPRECATED

#include <limits.h>
#include "svn_mergeinfo.h"
#include "../../libsvn_client/mergeinfo.h"
#include "svn_pools.h"
#include "svn_client.h"
#include "svn_repos.h"
#include "svn_subst.h"

#include "../svn_test.h"
#include "../svn_test_fs.h"

typedef struct mergeinfo_catalog_item {
  const char *path;
  const char *unparsed_mergeinfo;
  svn_boolean_t remains;
} mergeinfo_catalog_item;

#define MAX_ITEMS 10

static mergeinfo_catalog_item elide_testcases[][MAX_ITEMS] = {
  { {"/foo", "/bar: 1-4", TRUE},
    {"/foo/beep/baz", "/bar/beep/baz: 1-4", FALSE},
    { NULL }},
  { {"/foo", "/bar: 1-4", TRUE},
    {"/foo/beep/baz", "/blaa/beep/baz: 1-4", TRUE},
    { NULL }},
  { {"/", "/gah: 1-4", TRUE},
    {"/foo/beep/baz", "/gah/foo/beep/baz: 1-4", FALSE},
    { NULL }}
};

static svn_error_t *
test_elide_mergeinfo_catalog(apr_pool_t *pool)
{
  int i;
  apr_pool_t *iterpool;

  iterpool = svn_pool_create(pool);

  for (i = 0;
       i < sizeof(elide_testcases) / sizeof(elide_testcases[0]);
       i++)
    {
      apr_hash_t *catalog;
      mergeinfo_catalog_item *item;

      svn_pool_clear(iterpool);

      catalog = apr_hash_make(iterpool);
      for (item = elide_testcases[i]; item->path; item++)
        {
          apr_hash_t *mergeinfo;

          SVN_ERR(svn_mergeinfo_parse(&mergeinfo, item->unparsed_mergeinfo,
                                      iterpool));
          apr_hash_set(catalog, item->path, APR_HASH_KEY_STRING, mergeinfo);
        }

      SVN_ERR(svn_client__elide_mergeinfo_catalog(catalog, iterpool));

      for (item = elide_testcases[i]; item->path; item++)
        {
          apr_hash_t *mergeinfo = apr_hash_get(catalog, item->path,
                                               APR_HASH_KEY_STRING);
          if (item->remains && !mergeinfo)
            return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                     "Elision for test case #%d incorrectly "
                                     "elided '%s'", i, item->path);
          if (!item->remains && mergeinfo)
            return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                     "Elision for test case #%d failed to "
                                     "elide '%s'", i, item->path);
        }
    }

  svn_pool_destroy(iterpool);
  return SVN_NO_ERROR;
}

static svn_error_t *
test_args_to_target_array(apr_pool_t *pool)
{
  apr_size_t i;
  apr_pool_t *iterpool;
  svn_client_ctx_t *ctx;
  static struct {
    const char *input;
    const char *output; /* NULL means an error is expected. */
  } const tests[] = {
    { ".",                      "" },
    { ".@BASE",                 "@BASE" },
    { "foo///bar",              "foo/bar" },
    { "foo///bar@13",           "foo/bar@13" },
    { "foo///bar@HEAD",         "foo/bar@HEAD" },
    { "foo///bar@{1999-12-31}", "foo/bar@{1999-12-31}" },
    { "http://a//b////",        "http://a/b" },
    { "http://a///b@27",        "http://a/b@27" },
    { "http://a/b//@COMMITTED", "http://a/b@COMMITTED" },
    { "foo///bar@1:2",          "foo/bar@1:2" },
    { "foo///bar@baz",          "foo/bar@baz" },
    { "foo///bar@",             "foo/bar@" },
    { "foo///bar///@13",        "foo/bar@13" },
    { "foo///bar@@13",          "foo/bar@@13" },
    { "foo///@bar@HEAD",        "foo/@bar@HEAD" },
    { "foo@///bar",             "foo@/bar" },
    { "foo@HEAD///bar",         "foo@HEAD/bar" },
  };

  SVN_ERR(svn_client_create_context(&ctx, pool));

  iterpool = svn_pool_create(pool);

  for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
      const char *input = tests[i].input;
      const char *expected_output = tests[i].output;
      apr_array_header_t *targets;
      apr_getopt_t *os;
      const int argc = 2;
      const char *argv[3] = { 0 };
      apr_status_t apr_err;
      svn_error_t *err;

      argv[0] = "opt-test";
      argv[1] = input;
      argv[2] = NULL;

      apr_err = apr_getopt_init(&os, iterpool, argc, argv);
      if (apr_err)
        return svn_error_wrap_apr(apr_err,
                                  "Error initializing command line arguments");

      err = svn_client_args_to_target_array2(&targets, os, NULL, ctx, FALSE,
                                             iterpool);

      if (expected_output)
        {
          const char *actual_output;

          if (err)
            return err;
          if (argc - 1 != targets->nelts)
            return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                     "Passed %d target(s) to "
                                     "svn_client_args_to_target_array() but "
                                     "got %d back.",
                                     argc - 1,
                                     targets->nelts);

          actual_output = APR_ARRAY_IDX(targets, 0, const char *);

          if (! svn_path_is_canonical(actual_output, iterpool))
            return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                     "Input '%s' to "
                                     "svn_client_args_to_target_array() should "
                                     "have returned a canonical path but "
                                     "'%s' is not.",
                                     input,
                                     actual_output);

          if (strcmp(expected_output, actual_output) != 0)
            return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                     "Input '%s' to "
                                     "svn_client_args_to_target_array() should "
                                     "have returned '%s' but returned '%s'.",
                                     input,
                                     expected_output,
                                     actual_output);
        }
      else
        {
          if (! err)
            return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                     "Unexpected success in passing '%s' "
                                     "to svn_client_args_to_target_array().",
                                     input);
        }
    }

  return SVN_NO_ERROR;
}


/* A helper function for test_patch().
 * It compares a patched or reject file against expected content using the
 *  specified EOL. It also deletes the file if the check was successful. */
static svn_error_t *
check_patch_result(const char *path, const char **expected_lines, const char *eol,
                   int num_expected_lines, apr_pool_t *pool)
{
  svn_stream_t *stream;
  apr_pool_t *iterpool;
  int i;

  SVN_ERR(svn_stream_open_readonly(&stream, path, pool, pool));
  i = 0;
  iterpool = svn_pool_create(pool);
  while (TRUE)
    {
      svn_boolean_t eof;
      svn_stringbuf_t *line;

      svn_pool_clear(iterpool);

      SVN_ERR(svn_stream_readline(stream, &line, eol, &eof, pool));
      if (i < num_expected_lines)
        if (strcmp(expected_lines[i++], line->data) != 0)
          return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                   "%s line %d didn't match the expected line "
                                   "(strlen=%d vs strlen=%d)", path, i,
                                   (int)strlen(expected_lines[i-1]),
                                   (int)strlen(line->data));

      if (eof)
        break;
    }
  svn_pool_destroy(iterpool);

  SVN_TEST_ASSERT(i == num_expected_lines);
  SVN_ERR(svn_io_remove_file2(path, FALSE, pool));

  return SVN_NO_ERROR;
}

/* A baton for the patch collection function. */
struct patch_collection_baton
{
  apr_hash_t *patched_tempfiles;
  apr_hash_t *reject_tempfiles;
  apr_pool_t *state_pool;
};

/* Collect all the patch information we're interested in. */
static svn_error_t *
patch_collection_func(void *baton,
                      svn_boolean_t *filtered,
                      const char *canon_path_from_patchfile,
                      const char *patch_abspath,
                      const char *reject_abspath,
                      apr_pool_t *scratch_pool)
{
  struct patch_collection_baton *pcb = baton;

  if (patch_abspath)
    apr_hash_set(pcb->patched_tempfiles,
                 apr_pstrdup(pcb->state_pool, canon_path_from_patchfile),
                 APR_HASH_KEY_STRING,
                 apr_pstrdup(pcb->state_pool, patch_abspath));

  if (reject_abspath)
    apr_hash_set(pcb->reject_tempfiles,
                 apr_pstrdup(pcb->state_pool, canon_path_from_patchfile),
                 APR_HASH_KEY_STRING,
                 apr_pstrdup(pcb->state_pool, reject_abspath));

  if (filtered)
    *filtered = FALSE;

  return SVN_NO_ERROR;
}

static svn_error_t *
test_patch(const svn_test_opts_t *opts,
           apr_pool_t *pool)
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *repos_url;
  const char *wc_path;
  const char *cwd;
  svn_revnum_t committed_rev;
  svn_opt_revision_t rev;
  svn_opt_revision_t peg_rev;
  svn_client_ctx_t *ctx;
  apr_file_t *patch_file;
  struct patch_collection_baton pcb;
  const char *patch_file_path;
  const char *patched_tempfile_path;
  const char *reject_tempfile_path;
  const char *key;
  int i;
#define NL APR_EOL_STR
#define UNIDIFF_LINES 7
  const char *unidiff_patch[UNIDIFF_LINES] =  {
    "Index: A/D/gamma" NL,
    "===================================================================\n",
    "--- A/D/gamma\t(revision 1)" NL,
    "+++ A/D/gamma\t(working copy)" NL,
    "@@ -1 +1 @@" NL,
    "-This is really the file 'gamma'." NL,
    "+It is really the file 'gamma'." NL
  };
#define EXPECTED_GAMMA_LINES 1
  const char *expected_gamma[EXPECTED_GAMMA_LINES] = {
    "This is the file 'gamma'."
  };
#define EXPECTED_GAMMA_REJECT_LINES 5
  const char *expected_gamma_reject[EXPECTED_GAMMA_REJECT_LINES] = {
    "--- A/D/gamma",
    "+++ A/D/gamma",
    "@@ -1,1 +1,1 @@",
    "-This is really the file 'gamma'.",
    "+It is really the file 'gamma'."
  };

  /* Create a filesytem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-patch-repos",
                                 opts, pool));
  fs = svn_repos_fs(repos);

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &committed_rev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(committed_rev));

  /* Check out the HEAD revision */
  SVN_ERR(svn_dirent_get_absolute(&cwd, "", pool));
  SVN_ERR(svn_uri_get_file_url_from_dirent(&repos_url, "test-patch-repos",
                                           pool));

  /* Put wc inside an unversioned directory.  Checking out a 1.7 wc
     directly inside a 1.6 wc doesn't work reliably, an intervening
     unversioned directory prevents the problems. */
  wc_path = svn_dirent_join(cwd, "test-patch", pool);
  SVN_ERR(svn_io_make_dir_recursively(wc_path, pool));
  svn_test_add_dir_cleanup(wc_path);

  wc_path = svn_dirent_join(wc_path, "test-patch-wc", pool);
  SVN_ERR(svn_io_remove_dir2(wc_path, TRUE, NULL, NULL, pool));
  rev.kind = svn_opt_revision_head;
  peg_rev.kind = svn_opt_revision_unspecified;
  SVN_ERR(svn_client_create_context(&ctx, pool));
  SVN_ERR(svn_client_checkout3(NULL, repos_url, wc_path,
                               &peg_rev, &rev, svn_depth_infinity,
                               TRUE, FALSE, ctx, pool));

  /* Create the patch file. */
  patch_file_path = svn_dirent_join_many(pool, cwd,
                                         "test-patch", "test-patch.diff", NULL);
  SVN_ERR(svn_io_file_open(&patch_file, patch_file_path,
                           (APR_READ | APR_WRITE | APR_CREATE | APR_TRUNCATE),
                           APR_OS_DEFAULT, pool));
  for (i = 0; i < UNIDIFF_LINES; i++)
    {
      apr_size_t len = strlen(unidiff_patch[i]);
      SVN_ERR(svn_io_file_write(patch_file, unidiff_patch[i], &len, pool));
      SVN_TEST_ASSERT(len == strlen(unidiff_patch[i]));
    }
  SVN_ERR(svn_io_file_flush_to_disk(patch_file, pool));

  /* Apply the patch. */
  pcb.patched_tempfiles = apr_hash_make(pool);
  pcb.reject_tempfiles = apr_hash_make(pool);
  pcb.state_pool = pool;
  SVN_ERR(svn_client_patch(patch_file_path, wc_path, FALSE, 0, FALSE,
                           FALSE, FALSE, patch_collection_func, &pcb,
                           ctx, pool));
  SVN_ERR(svn_io_file_close(patch_file, pool));

  SVN_TEST_ASSERT(apr_hash_count(pcb.patched_tempfiles) == 1);
  key = "A/D/gamma";
  patched_tempfile_path = apr_hash_get(pcb.patched_tempfiles, key,
                                       APR_HASH_KEY_STRING);
  SVN_ERR(check_patch_result(patched_tempfile_path, expected_gamma, "\n",
                             EXPECTED_GAMMA_LINES, pool));
  SVN_TEST_ASSERT(apr_hash_count(pcb.reject_tempfiles) == 1);
  key = "A/D/gamma";
  reject_tempfile_path = apr_hash_get(pcb.reject_tempfiles, key,
                                     APR_HASH_KEY_STRING);
  SVN_ERR(check_patch_result(reject_tempfile_path, expected_gamma_reject,
                             APR_EOL_STR, EXPECTED_GAMMA_REJECT_LINES, pool));

  return SVN_NO_ERROR;
}

static svn_error_t *
test_wc_add_scenarios(const svn_test_opts_t *opts,
                      apr_pool_t *pool)
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *repos_url;
  const char *wc_path;
  svn_revnum_t committed_rev;
  svn_client_ctx_t *ctx;
  svn_opt_revision_t rev, peg_rev;
  const char *new_dir_path;
  const char *ex_file_path;
  const char *ex_dir_path;
  const char *ex2_dir_path;

  /* Create a filesytem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-wc-add-repos",
                                 opts, pool));
  fs = svn_repos_fs(repos);

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &committed_rev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(committed_rev));

  SVN_ERR(svn_uri_get_file_url_from_dirent(&repos_url, "test-wc-add-repos",
                                           pool));

  SVN_ERR(svn_dirent_get_absolute(&wc_path, "test-wc-add", pool));

  /* Remove old test data from the previous run */
  SVN_ERR(svn_io_remove_dir2(wc_path, TRUE, NULL, NULL, pool));

  SVN_ERR(svn_io_make_dir_recursively(wc_path, pool));
  svn_test_add_dir_cleanup(wc_path);

  rev.kind = svn_opt_revision_head;
  peg_rev.kind = svn_opt_revision_unspecified;
  SVN_ERR(svn_client_create_context(&ctx, pool));
  /* Checkout greek tree as wc_path */
  SVN_ERR(svn_client_checkout3(NULL, repos_url, wc_path, &peg_rev, &rev,
                               svn_depth_infinity, FALSE, FALSE, ctx, pool));

  /* Now checkout again as wc_path/NEW */
  new_dir_path = svn_dirent_join(wc_path, "NEW", pool);
  SVN_ERR(svn_client_checkout3(NULL, repos_url, new_dir_path, &peg_rev, &rev,
                               svn_depth_infinity, FALSE, FALSE,
                               ctx, pool));

  ex_dir_path = svn_dirent_join(wc_path, "NEW_add", pool);
  ex2_dir_path = svn_dirent_join(wc_path, "NEW_add2", pool);
  SVN_ERR(svn_io_dir_make(ex_dir_path, APR_OS_DEFAULT, pool));
  SVN_ERR(svn_io_dir_make(ex2_dir_path, APR_OS_DEFAULT, pool));

  SVN_ERR(svn_io_open_uniquely_named(NULL, &ex_file_path, wc_path, "new_file",
                                     NULL, svn_io_file_del_none, pool, pool));

  /* Now use an access baton to do some add operations like an old client
     might do */
  {
    svn_wc_adm_access_t *adm_access, *adm2;
    svn_boolean_t locked;

    SVN_ERR(svn_wc_adm_open3(&adm_access, NULL, wc_path, TRUE, -1, NULL, NULL,
                             pool));

    /* ### The above svn_wc_adm_open3 creates a new svn_wc__db_t
       ### instance.  The svn_wc_add3 below doesn't work while the
       ### original svn_wc__db_t created by svn_client_create_context
       ### remains open.  Closing the wc-context gets around the
       ### problem but is obviously a hack. */
    SVN_ERR(svn_wc_context_destroy(ctx->wc_ctx));
    SVN_ERR(svn_wc_context_create(&ctx->wc_ctx, NULL, pool, pool));

    /* Fix up copy as add with history */
    SVN_ERR(svn_wc_add3(new_dir_path, adm_access, svn_depth_infinity,
                        repos_url, committed_rev, NULL, NULL, NULL, NULL,
                        pool));

    /* Verify if the paths are locked now */
    SVN_ERR(svn_wc_locked(&locked, wc_path, pool));
    SVN_TEST_ASSERT(locked && "wc_path locked");
    SVN_ERR(svn_wc_locked(&locked, new_dir_path, pool));
    SVN_TEST_ASSERT(locked && "new_path locked");

    SVN_ERR(svn_wc_adm_retrieve(&adm2, adm_access, new_dir_path, pool));
    SVN_TEST_ASSERT(adm2 != NULL && "available in set");

    /* Add local (new) file */
    SVN_ERR(svn_wc_add3(ex_file_path, adm_access, svn_depth_unknown, NULL,
                        SVN_INVALID_REVNUM, NULL, NULL, NULL, NULL, pool));

    /* Add local (new) directory */
    SVN_ERR(svn_wc_add3(ex_dir_path, adm_access, svn_depth_infinity, NULL,
                        SVN_INVALID_REVNUM, NULL, NULL, NULL, NULL, pool));

    SVN_ERR(svn_wc_adm_retrieve(&adm2, adm_access, ex_dir_path, pool));
    SVN_TEST_ASSERT(adm2 != NULL && "available in set");

    /* Add empty directory with copy trail */
    SVN_ERR(svn_wc_add3(ex2_dir_path, adm_access, svn_depth_infinity,
                        repos_url, committed_rev, NULL, NULL, NULL, NULL,
                        pool));

    SVN_ERR(svn_wc_adm_retrieve(&adm2, adm_access, ex2_dir_path, pool));
    SVN_TEST_ASSERT(adm2 != NULL && "available in set");

    SVN_ERR(svn_wc_adm_close2(adm_access, pool));
  }

  /* Some simple status calls to verify that the paths are added */
  {
    svn_wc_status3_t *status;

    SVN_ERR(svn_wc_status3(&status, ctx->wc_ctx, new_dir_path, pool, pool));

    SVN_TEST_ASSERT(status->node_status == svn_wc_status_added
                    && status->copied
                    && !strcmp(status->repos_relpath, "NEW"));

    SVN_ERR(svn_wc_status3(&status, ctx->wc_ctx, ex_file_path, pool, pool));

    SVN_TEST_ASSERT(status->node_status == svn_wc_status_added
                    && !status->copied);

    SVN_ERR(svn_wc_status3(&status, ctx->wc_ctx, ex_dir_path, pool, pool));

    SVN_TEST_ASSERT(status->node_status == svn_wc_status_added
                    && !status->copied);

    SVN_ERR(svn_wc_status3(&status, ctx->wc_ctx, ex2_dir_path, pool, pool));

    SVN_TEST_ASSERT(status->node_status == svn_wc_status_added
                    && status->copied);
  }

  /* ### Add a commit? */

  return SVN_NO_ERROR;
}

/* This is for issue #3234. */
static svn_error_t *
test_copy_crash(const svn_test_opts_t *opts,
                apr_pool_t *pool)
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  apr_array_header_t *sources;
  svn_revnum_t committed_rev;
  svn_opt_revision_t rev;
  svn_client_copy_source_t source;
  svn_client_ctx_t *ctx;
  const char *dest;
  const char *repos_url;

  /* Create a filesytem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-copy-crash",
                                 opts, pool));
  fs = svn_repos_fs(repos);

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &committed_rev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(committed_rev));

  SVN_ERR(svn_uri_get_file_url_from_dirent(&repos_url, "test-copy-crash",
                                           pool));

  svn_client_create_context(&ctx, pool);

  rev.kind = svn_opt_revision_head;
  dest = svn_path_url_add_component2(repos_url, "A/E", pool);
  source.path = svn_path_url_add_component2(repos_url, "A/B", pool);
  source.revision = &rev;
  source.peg_revision = &rev;
  sources = apr_array_make(pool, 1, sizeof(svn_client_copy_source_t *));
  APR_ARRAY_PUSH(sources, svn_client_copy_source_t *) = &source;

  /* This shouldn't crash. */
  SVN_ERR(svn_client_copy6(sources, dest, FALSE, TRUE, FALSE, NULL, NULL, NULL,
                           ctx, pool));

  return SVN_NO_ERROR;
}

#ifdef TEST16K_ADD
static svn_error_t *
test_16k_add(const svn_test_opts_t *opts,
                apr_pool_t *pool)
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  svn_revnum_t committed_rev;
  svn_opt_revision_t rev;
  svn_client_ctx_t *ctx;
  const char *repos_url;
  const char *cwd, *wc_path;
  svn_opt_revision_t peg_rev;
  apr_array_header_t *targets;
  apr_pool_t *iterpool = svn_pool_create(pool);
  int i;

  /* Create a filesytem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-16k-repos",
                                 opts, pool));
  fs = svn_repos_fs(repos);

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &committed_rev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(committed_rev));

  /* Check out the HEAD revision */
  SVN_ERR(svn_dirent_get_absolute(&cwd, "", pool));
  SVN_ERR(svn_uri_get_file_url_from_dirent(&repos_url, "test-16k-repos",
                                           pool));

  /* Put wc inside an unversioned directory.  Checking out a 1.7 wc
     directly inside a 1.6 wc doesn't work reliably, an intervening
     unversioned directory prevents the problems. */
  wc_path = svn_dirent_join(cwd, "test-16k", pool);
  SVN_ERR(svn_io_make_dir_recursively(wc_path, pool));
  svn_test_add_dir_cleanup(wc_path);

  wc_path = svn_dirent_join(wc_path, "trunk", pool);
  SVN_ERR(svn_io_remove_dir2(wc_path, TRUE, NULL, NULL, pool));
  rev.kind = svn_opt_revision_head;
  peg_rev.kind = svn_opt_revision_unspecified;
  SVN_ERR(svn_client_create_context(&ctx, pool));
  SVN_ERR(svn_client_checkout3(NULL, repos_url, wc_path,
                               &peg_rev, &rev, svn_depth_infinity,
                               TRUE, FALSE, ctx, pool));

  for (i = 0; i < 16384; i++)
    {
      const char *path;

      svn_pool_clear(iterpool);

      SVN_ERR(svn_io_open_unique_file3(NULL, &path, wc_path,
                                       svn_io_file_del_none,
                                       iterpool, iterpool));

      SVN_ERR(svn_client_add4(path, svn_depth_unknown, FALSE, FALSE, FALSE,
                              ctx, iterpool));
    }

  targets = apr_array_make(pool, 1, sizeof(const char *));
  APR_ARRAY_PUSH(targets, const char *) = wc_path;
  svn_pool_clear(iterpool);

  SVN_ERR(svn_client_commit5(targets, svn_depth_infinity, FALSE, FALSE, TRUE,
                             NULL, NULL, NULL, NULL, ctx, iterpool));


  return SVN_NO_ERROR;
}
#endif

/* ========================================================================== */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(test_elide_mergeinfo_catalog,
                   "test svn_client__elide_mergeinfo_catalog"),
    SVN_TEST_PASS2(test_args_to_target_array,
                   "test svn_client_args_to_target_array"),
    SVN_TEST_OPTS_PASS(test_patch, "test svn_client_patch"),
    SVN_TEST_OPTS_PASS(test_wc_add_scenarios, "test svn_wc_add3 scenarios"),
    SVN_TEST_OPTS_PASS(test_copy_crash, "test a crash in svn_client_copy5"),
#ifdef TEST16K_ADD
    SVN_TEST_OPTS_PASS(test_16k_add, "test adding 16k files"),
#endif
    SVN_TEST_NULL
  };
