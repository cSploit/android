/*
 * op-depth-test.c :  test layered tree changes
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

/* To avoid warnings... */
#define SVN_DEPRECATED

#include <apr_pools.h>
#include <apr_general.h>

#include "svn_types.h"
#include "svn_io.h"
#include "svn_dirent_uri.h"
#include "svn_pools.h"
#include "svn_repos.h"
#include "svn_wc.h"
#include "svn_client.h"
#include "svn_hash.h"

#include "utils.h"

#include "private/svn_wc_private.h"
#include "private/svn_sqlite.h"
#include "private/svn_dep_compat.h"
#include "../../libsvn_wc/wc.h"
#include "../../libsvn_wc/wc_db.h"
#define SVN_WC__I_AM_WC_DB
#include "../../libsvn_wc/wc_db_private.h"

#include "../svn_test.h"

#ifdef _MSC_VER
#pragma warning(disable: 4221) /* nonstandard extension used */
#endif

/* Compare strings, like strcmp but either or both may be NULL which
 * compares equal to NULL and not equal to any non-NULL string. */
static int
strcmp_null(const char *s1, const char *s2)
{
  if (s1 && s2)
    return strcmp(s1, s2);
  else if (s1 || s2)
    return 1;
  else
    return 0;
}


/* ---------------------------------------------------------------------- */
/* Reading the WC DB */

static svn_error_t *
open_wc_db(svn_sqlite__db_t **sdb,
           const char *wc_root_abspath,
           const char *const *my_statements,
           apr_pool_t *result_pool,
           apr_pool_t *scratch_pool)
{
  SVN_ERR(svn_wc__db_util_open_db(sdb, wc_root_abspath, "wc.db",
                                  svn_sqlite__mode_readwrite, my_statements,
                                  result_pool, scratch_pool));
  return SVN_NO_ERROR;
}


/* ---------------------------------------------------------------------- */
/* Functions for easy manipulation of a WC. Paths given to these functions
 * can be relative to the WC root as stored in the WC baton. */

/* Return the abspath of PATH which is absolute or relative to the WC in B. */
#define wc_path(b, path) (svn_dirent_join((b)->wc_abspath, (path), (b)->pool))

/* Create a file on disk at PATH, with TEXT as its content. */
static void
file_write(svn_test__sandbox_t *b, const char *path, const char *text)
{
  FILE *f = fopen(wc_path(b, path), "w");
  fputs(text, f);
  fclose(f);
}

/* Schedule for addition the single node that exists on disk at PATH,
 * non-recursively. */
static svn_error_t *
wc_add(svn_test__sandbox_t *b, const char *path)
{
  const char *parent_abspath;
  path = wc_path(b, path);
  parent_abspath = svn_dirent_dirname(path, b->pool);
  SVN_ERR(svn_wc__acquire_write_lock(NULL, b->wc_ctx, parent_abspath, FALSE,
                                     b->pool, b->pool));
  SVN_ERR(svn_wc_add_from_disk(b->wc_ctx, path, NULL, NULL, b->pool));
  SVN_ERR(svn_wc__release_write_lock(b->wc_ctx, parent_abspath, b->pool));
  return SVN_NO_ERROR;
}

/* Create a single directory on disk. */
static svn_error_t *
disk_mkdir(svn_test__sandbox_t *b, const char *path)
{
  path = wc_path(b, path);
  SVN_ERR(svn_io_dir_make(path, APR_FPROT_OS_DEFAULT, b->pool));
  return SVN_NO_ERROR;
}

/* Create a single directory on disk and schedule it for addition. */
static svn_error_t *
wc_mkdir(svn_test__sandbox_t *b, const char *path)
{
  SVN_ERR(disk_mkdir(b, path));
  SVN_ERR(wc_add(b, path));
  return SVN_NO_ERROR;
}

#if 0 /* not used */
/* Copy the file or directory tree FROM_PATH to TO_PATH which must not exist
 * beforehand. */
static svn_error_t *
disk_copy(svn_test__sandbox_t *b, const char *from_path, const char *to_path)
{
  const char *to_dir, *to_name;
  from_path = wc_path(b, from_path);
  to_path = wc_path(b, to_path);
  svn_dirent_split(&to_dir, &to_name, to_path, b->pool);
  return svn_io_copy_dir_recursively(from_path, to_dir, to_name,
                                     FALSE, NULL, NULL, b->pool);
}
#endif

/* Copy the WC file or directory tree FROM_PATH to TO_PATH which must not
 * exist beforehand. */
static svn_error_t *
wc_copy(svn_test__sandbox_t *b, const char *from_path, const char *to_path)
{
  from_path = wc_path(b, from_path);
  to_path = wc_path(b, to_path);
  return svn_wc_copy3(b->wc_ctx, from_path, to_path, FALSE,
                      NULL, NULL, NULL, NULL, b->pool);
}

/* Revert a WC file or directory tree at PATH */
static svn_error_t *
wc_revert(svn_test__sandbox_t *b, const char *path, svn_depth_t depth)
{
  const char *abspath = wc_path(b, path);
  const char *dir_abspath = svn_dirent_dirname(abspath, b->pool);
  const char *lock_root_abspath;

  SVN_ERR(svn_wc__acquire_write_lock(&lock_root_abspath, b->wc_ctx,
                                     dir_abspath, FALSE /* lock_anchor */,
                                     b->pool, b->pool));
  SVN_ERR(svn_wc_revert4(b->wc_ctx, abspath, depth, FALSE, NULL,
                         NULL, NULL, /* cancel baton + func */
                         NULL, NULL, /* notify baton + func */
                         b->pool));
  SVN_ERR(svn_wc__release_write_lock(b->wc_ctx, lock_root_abspath, b->pool));
  return SVN_NO_ERROR;
}

static svn_error_t *
wc_delete(svn_test__sandbox_t *b, const char *path)
{
  const char *abspath = wc_path(b, path);
  const char *dir_abspath = svn_dirent_dirname(abspath, b->pool);
  const char *lock_root_abspath;

  SVN_ERR(svn_wc__acquire_write_lock(&lock_root_abspath, b->wc_ctx,
                                     dir_abspath, FALSE,
                                     b->pool, b->pool));
  SVN_ERR(svn_wc_delete4(b->wc_ctx, abspath, FALSE, TRUE,
                         NULL, NULL, /* cancel baton + func */
                         NULL, NULL, /* notify baton + func */
                         b->pool));
  SVN_ERR(svn_wc__release_write_lock(b->wc_ctx, lock_root_abspath, b->pool));
  return SVN_NO_ERROR;
}

static svn_error_t *
wc_exclude(svn_test__sandbox_t *b, const char *path)
{
  const char *abspath = wc_path(b, path);
  const char *lock_root_abspath;

  SVN_ERR(svn_wc__acquire_write_lock(&lock_root_abspath, b->wc_ctx,
                                     abspath, TRUE,
                                     b->pool, b->pool));
  SVN_ERR(svn_wc_exclude(b->wc_ctx, abspath,
                         NULL, NULL, /* cancel baton + func */
                         NULL, NULL, /* notify baton + func */
                         b->pool));
  SVN_ERR(svn_wc__release_write_lock(b->wc_ctx, lock_root_abspath, b->pool));
  return SVN_NO_ERROR;
}

static svn_error_t *
wc_commit(svn_test__sandbox_t *b, const char *path)
{
  svn_client_ctx_t *ctx;
  apr_array_header_t *targets = apr_array_make(b->pool, 1,
                                               sizeof(const char *));

  APR_ARRAY_PUSH(targets, const char *) = wc_path(b, path);
  SVN_ERR(svn_client_create_context(&ctx, b->pool));
  return svn_client_commit5(targets, svn_depth_infinity,
                            FALSE, FALSE, TRUE, /* keep locks/cl's/use_ops*/
                            NULL, NULL, NULL, NULL, ctx, b->pool);
}

static svn_error_t *
wc_update(svn_test__sandbox_t *b, const char *path, svn_revnum_t revnum)
{
  svn_client_ctx_t *ctx;
  apr_array_header_t *result_revs;
  apr_array_header_t *paths = apr_array_make(b->pool, 1,
                                             sizeof(const char *));
  svn_opt_revision_t revision;
  revision.kind = svn_opt_revision_number;
  revision.value.number = revnum;

  APR_ARRAY_PUSH(paths, const char *) = wc_path(b, path);
  SVN_ERR(svn_client_create_context(&ctx, b->pool));
  return svn_client_update4(&result_revs, paths, &revision, svn_depth_infinity,
                            TRUE, FALSE, FALSE, FALSE, FALSE,
                            ctx, b->pool);
}

static svn_error_t *
wc_resolved(svn_test__sandbox_t *b, const char *path)
{
  svn_client_ctx_t *ctx;

  SVN_ERR(svn_client_create_context(&ctx, b->pool));
  return svn_client_resolved(wc_path(b, path), TRUE, ctx, b->pool);
}

static svn_error_t *
wc_move(svn_test__sandbox_t *b, const char *src, const char *dst)
{
  svn_client_ctx_t *ctx;
  apr_array_header_t *paths = apr_array_make(b->pool, 1,
                                             sizeof(const char *));

  SVN_ERR(svn_client_create_context(&ctx, b->pool));
  APR_ARRAY_PUSH(paths, const char *) = wc_path(b, src);
  return svn_client_move6(paths, wc_path(b, dst),
                          FALSE, FALSE, NULL, NULL, NULL, ctx, b->pool);
}

static svn_error_t *
wc_propset(svn_test__sandbox_t *b,
           const char *name,
           const char *value,
           const char *path)
{
  svn_client_ctx_t *ctx;
  apr_array_header_t *paths = apr_array_make(b->pool, 1,
                                             sizeof(const char *));

  SVN_ERR(svn_client_create_context(&ctx, b->pool));
  APR_ARRAY_PUSH(paths, const char *) = wc_path(b, path);
  return svn_client_propset_local(name, svn_string_create(value, b->pool),
                                  paths, svn_depth_empty, TRUE, NULL, ctx,
                                  b->pool);
}

/* Create the Greek tree on disk in the WC, and commit it. */
static svn_error_t *
add_and_commit_greek_tree(svn_test__sandbox_t *b)
{
  const char *greek_tree_dirs[8] =
  {
    "A",
    "A/B",
    "A/B/E",
    "A/B/F",
    "A/C",
    "A/D",
    "A/D/G",
    "A/D/H"
  };
  const char *greek_tree_files[12][2] =
  {
    { "iota",         "This is the file 'iota'.\n" },
    { "A/mu",         "This is the file 'mu'.\n" },
    { "A/B/lambda",   "This is the file 'lambda'.\n" },
    { "A/B/E/alpha",  "This is the file 'alpha'.\n" },
    { "A/B/E/beta",   "This is the file 'beta'.\n" },
    { "A/D/gamma",    "This is the file 'gamma'.\n" },
    { "A/D/G/pi",     "This is the file 'pi'.\n" },
    { "A/D/G/rho",    "This is the file 'rho'.\n" },
    { "A/D/G/tau",    "This is the file 'tau'.\n" },
    { "A/D/H/chi",    "This is the file 'chi'.\n" },
    { "A/D/H/psi",    "This is the file 'psi'.\n" },
    { "A/D/H/omega",  "This is the file 'omega'.\n" }
  };
  int i;

  for (i = 0; i < 8; i++)
    SVN_ERR(wc_mkdir(b, greek_tree_dirs[i]));

  for (i = 0; i < 12; i++)
    {
      file_write(b, greek_tree_files[i][0], greek_tree_files[i][1]);
      SVN_ERR(wc_add(b, greek_tree_files[i][0]));
    }

  SVN_ERR(wc_commit(b, ""));

  return SVN_NO_ERROR;
}


/* ---------------------------------------------------------------------- */
/* Functions for comparing expected and found WC DB data. */

/* Some of the fields from a NODES table row. */
typedef struct nodes_row_t {
    int op_depth;
    const char *local_relpath;
    const char *presence;
    svn_revnum_t repo_revnum;
    const char *repo_relpath;
    svn_boolean_t file_external;
} nodes_row_t;

/* Macro for filling in the REPO_* fields of a non-base NODES_ROW_T
 * that has no copy-from info. */
#define NO_COPY_FROM SVN_INVALID_REVNUM, NULL

/* Return a human-readable string representing ROW. */
static const char *
print_row(const nodes_row_t *row,
          apr_pool_t *result_pool)
{
  const char *file_external_str;

  if (row == NULL)
    return "(null)";

  if (row->file_external)
    file_external_str = ", file-external";
  else
    file_external_str = "";
      
  if (row->repo_revnum == SVN_INVALID_REVNUM)
    return apr_psprintf(result_pool, "%d, %s, %s%s",
                        row->op_depth, row->local_relpath, row->presence,
                        file_external_str);
  else
    return apr_psprintf(result_pool, "%d, %s, %s, from ^/%s@%d%s",
                        row->op_depth, row->local_relpath, row->presence,
                        row->repo_relpath, (int)row->repo_revnum,
                        file_external_str);
}

/* A baton to pass through svn_hash_diff() to compare_nodes_rows(). */
typedef struct comparison_baton_t {
    apr_hash_t *expected_hash;  /* Maps "OP_DEPTH PATH" to nodes_row_t. */
    apr_hash_t *found_hash;     /* Maps "OP_DEPTH PATH" to nodes_row_t. */
    apr_pool_t *scratch_pool;
    svn_error_t *errors;        /* Chain of errors found in comparison. */
} comparison_baton_t;

/* Compare two hash entries indexed by KEY, in the two hashes in BATON.
 * Append an error message to BATON->errors if they differ or are not both
 * present.
 *
 * Implements svn_hash_diff_func_t. */
static svn_error_t *
compare_nodes_rows(const void *key, apr_ssize_t klen,
                   enum svn_hash_diff_key_status status,
                   void *baton)
{
  comparison_baton_t *b = baton;
  nodes_row_t *expected = apr_hash_get(b->expected_hash, key, klen);
  nodes_row_t *found = apr_hash_get(b->found_hash, key, klen);

  if (! expected)
    {
      b->errors = svn_error_createf(
                    SVN_ERR_TEST_FAILED, b->errors,
                    "found   {%s}",
                    print_row(found, b->scratch_pool));
    }
  else if (! found)
    {
      b->errors = svn_error_createf(
                    SVN_ERR_TEST_FAILED, b->errors,
                    "expected {%s}",
                    print_row(expected, b->scratch_pool));
    }
  else if (expected->repo_revnum != found->repo_revnum
           || (strcmp_null(expected->repo_relpath, found->repo_relpath) != 0)
           || (strcmp_null(expected->presence, found->presence) != 0)
           || (expected->file_external != found->file_external))
    {
      b->errors = svn_error_createf(
                    SVN_ERR_TEST_FAILED, b->errors,
                    "expected {%s}; found {%s}",
                    print_row(expected, b->scratch_pool),
                    print_row(found, b->scratch_pool));
    }

  /* Don't terminate the comparison: accumulate all differences. */
  return SVN_NO_ERROR;
}


/* Examine the WC DB for paths ROOT_PATH and below, and check that their
 * rows in the 'NODES' table (only those at op_depth > 0) match EXPECTED_ROWS
 * (which is terminated by a row of null fields).
 *
 * Return a chain of errors describing any and all mismatches. */
static svn_error_t *
check_db_rows(svn_test__sandbox_t *b,
              const char *root_path,
              const nodes_row_t *expected_rows)
{
  const char *base_relpath = root_path;
  svn_sqlite__db_t *sdb;
  int i;
  svn_sqlite__stmt_t *stmt;
  static const char *const statements[] = {
    "SELECT op_depth, presence, local_relpath, revision, repos_path, "
      "     file_external "
      "FROM nodes "
      "WHERE local_relpath = ?1 OR local_relpath LIKE ?2",
    NULL };
#define STMT_SELECT_NODES_INFO 0

  svn_boolean_t have_row;
  apr_hash_t *found_hash = apr_hash_make(b->pool);
  apr_hash_t *expected_hash = apr_hash_make(b->pool);
  comparison_baton_t comparison_baton;

  comparison_baton.expected_hash = expected_hash;
  comparison_baton.found_hash = found_hash;
  comparison_baton.scratch_pool = b->pool;
  comparison_baton.errors = NULL;

  /* Fill ACTUAL_HASH with data from the WC DB. */
  SVN_ERR(open_wc_db(&sdb, b->wc_abspath, statements, b->pool, b->pool));
  SVN_ERR(svn_sqlite__get_statement(&stmt, sdb, STMT_SELECT_NODES_INFO));
  SVN_ERR(svn_sqlite__bindf(stmt, "ss", base_relpath,
                            (base_relpath[0]
                             ? apr_psprintf(b->pool, "%s/%%", base_relpath)
                             : "_%")));
  SVN_ERR(svn_sqlite__step(&have_row, stmt));
  while (have_row)
    {
      const char *key;
      nodes_row_t *row = apr_palloc(b->pool, sizeof(*row));

      row->op_depth = svn_sqlite__column_int(stmt, 0);
      row->presence = svn_sqlite__column_text(stmt, 1, b->pool);
      row->local_relpath = svn_sqlite__column_text(stmt, 2, b->pool);
      row->repo_revnum = svn_sqlite__column_revnum(stmt, 3);
      row->repo_relpath = svn_sqlite__column_text(stmt, 4, b->pool);
      row->file_external = !svn_sqlite__column_is_null(stmt, 5);

      key = apr_psprintf(b->pool, "%d %s", row->op_depth, row->local_relpath);
      apr_hash_set(found_hash, key, APR_HASH_KEY_STRING, row);

      SVN_ERR(svn_sqlite__step(&have_row, stmt));
    }
  SVN_ERR(svn_sqlite__reset(stmt));

  /* Fill EXPECTED_HASH with data from EXPECTED_ROWS. */
  for (i = 0; expected_rows[i].local_relpath != NULL; i++)
    {
      const char *key;
      const nodes_row_t *row = &expected_rows[i];

      key = apr_psprintf(b->pool, "%d %s", row->op_depth, row->local_relpath);
      apr_hash_set(expected_hash, key, APR_HASH_KEY_STRING, row);
    }

  /* Compare EXPECTED_HASH with ACTUAL_HASH and return any errors. */
  SVN_ERR(svn_hash_diff(expected_hash, found_hash,
                        compare_nodes_rows, &comparison_baton, b->pool));
  SVN_ERR(svn_sqlite__close(sdb));
  return comparison_baton.errors;
}


/* ---------------------------------------------------------------------- */
/* The test functions */

/* Definition of a copy sub-test and its expected results. */
struct copy_subtest_t
{
  /* WC-relative or repo-relative source and destination paths. */
  const char *from_path;
  const char *to_path;
  /* All the expected nodes table rows within the destination sub-tree.
   * Terminated by an all-zero row. */
  nodes_row_t expected[20];
};

#define source_everything   "A/B"

#define source_base_file    "A/B/lambda"
#define source_base_dir     "A/B/E"

#define source_added_file   "A/B/file-added"
#define source_added_dir    "A/B/D-added"
#define source_added_dir2   "A/B/D-added/D2"

#define source_copied_file  "A/B/lambda-copied"
#define source_copied_dir   "A/B/E-copied"

/* Check that all kinds of WC-to-WC copies give correct op_depth results:
 * create a Greek tree, make copies in it, and check the resulting DB rows. */
static svn_error_t *
wc_wc_copies(svn_test__sandbox_t *b)
{
  SVN_ERR(add_and_commit_greek_tree(b));

  /* Create the various kinds of source node which will be copied */

  file_write(b, source_added_file, "New file");
  SVN_ERR(wc_add(b, source_added_file));
  SVN_ERR(wc_mkdir(b, source_added_dir));
  SVN_ERR(wc_mkdir(b, source_added_dir2));

  SVN_ERR(wc_copy(b, source_base_file, source_copied_file));
  SVN_ERR(wc_copy(b, source_base_dir, source_copied_dir));

  /* Delete some nodes so that we can test copying onto these paths */

  SVN_ERR(wc_delete(b, "A/D/gamma"));
  SVN_ERR(wc_delete(b, "A/D/G"));

  /* Test copying various things */

  {
    struct copy_subtest_t subtests[] =
      {
        /* base file */
        { source_base_file, "A/C/copy1", {
            { 3, "",                "normal",   1, source_base_file }
          } },

        /* base dir */
        { source_base_dir, "A/C/copy2", {
            { 3, "",                "normal",   1, source_base_dir },
            { 3, "alpha",           "normal",   1, "A/B/E/alpha" },
            { 3, "beta",            "normal",   1, "A/B/E/beta" }
          } },

        /* added file */
        { source_added_file, "A/C/copy3", {
            { 3, "",                "normal",   NO_COPY_FROM }
          } },

        /* added dir */
        { source_added_dir, "A/C/copy4", {
            { 3, "",                "normal",   NO_COPY_FROM },
            { 4, "D2",              "normal",   NO_COPY_FROM }
          } },

        /* copied file */
        { source_copied_file, "A/C/copy5", {
            { 3, "",                "normal",   1, source_base_file }
          } },

        /* copied dir */
        { source_copied_dir, "A/C/copy6", {
            { 3, "",                "normal",   1, source_base_dir },
            { 3, "alpha",           "normal",   1, "A/B/E/alpha" },
            { 3, "beta",            "normal",   1, "A/B/E/beta" }
          } },

        /* copied tree with everything in it */
        { source_everything, "A/C/copy7", {
            { 3, "",                "normal",   1, source_everything },
            { 3, "lambda",          "normal",   1, "A/B/lambda" },
            { 3, "E",               "normal",   1, "A/B/E" },
            { 3, "E/alpha",         "normal",   1, "A/B/E/alpha" },
            { 3, "E/beta",          "normal",   1, "A/B/E/beta" },
            { 3, "F",               "normal",   1, "A/B/F" },
            /* Each add is an op_root */
            { 4, "file-added",      "normal",   NO_COPY_FROM },
            { 4, "D-added",         "normal",   NO_COPY_FROM },
            { 5, "D-added/D2",      "normal",   NO_COPY_FROM },
            /* Each copied-copy subtree is an op_root */
            { 4, "lambda-copied",   "normal",   1, source_base_file },
            { 4, "E-copied",        "normal",   1, source_base_dir },
            { 4, "E-copied/alpha",  "normal",   1, "A/B/E/alpha" },
            { 4, "E-copied/beta",   "normal",   1, "A/B/E/beta" }
          } },

        /* dir onto a schedule-delete file */
        { source_base_dir, "A/D/gamma", {
            { 0, "",                "normal",   1, "A/D/gamma" },
            { 3, "",                "normal",   1, source_base_dir },
            { 3, "alpha",           "normal",   1, "A/B/E/alpha" },
            { 3, "beta",            "normal",   1, "A/B/E/beta" }
          } },

        /* file onto a schedule-delete dir */
        { source_base_file, "A/D/G", {
            { 0, "",                "normal",   1, "A/D/G" },
            { 0, "pi",              "normal",   1, "A/D/G/pi" },
            { 0, "rho",             "normal",   1, "A/D/G/rho" },
            { 0, "tau",             "normal",   1, "A/D/G/tau" },
            { 3, "",                "normal",   1, source_base_file },
            { 3, "pi",              "base-deleted",   NO_COPY_FROM },
            { 3, "rho",             "base-deleted",   NO_COPY_FROM },
            { 3, "tau",             "base-deleted",   NO_COPY_FROM }
          } },

        { 0 }
      };
    struct copy_subtest_t *subtest;

    /* Fix up the expected->local_relpath fields in the subtest data to be
     * relative to the WC root rather than to the copy destination dir. */
    for (subtest = subtests; subtest->from_path; subtest++)
      {
        nodes_row_t *row;
        for (row = &subtest->expected[0]; row->local_relpath; row++)
          row->local_relpath = svn_dirent_join(subtest->to_path,
                                               row->local_relpath, b->pool);
      }

    /* Perform each subtest in turn. */
    for (subtest = subtests; subtest->from_path; subtest++)
      {
        SVN_ERR(svn_wc_copy3(b->wc_ctx,
                             wc_path(b, subtest->from_path),
                             wc_path(b, subtest->to_path),
                             FALSE /* metadata_only */,
                             NULL, NULL, NULL, NULL, b->pool));
        SVN_ERR(check_db_rows(b, subtest->to_path, subtest->expected));
      }
  }

  return SVN_NO_ERROR;
}

/* Check that all kinds of repo-to-WC copies give correct op_depth results:
 * create a Greek tree, make copies in it, and check the resulting DB rows. */
static svn_error_t *
repo_wc_copies(svn_test__sandbox_t *b)
{
  SVN_ERR(add_and_commit_greek_tree(b));

  /* Delete some nodes so that we can test copying onto these paths */

  SVN_ERR(wc_delete(b, "A/B/lambda"));
  SVN_ERR(wc_delete(b, "A/D/gamma"));
  SVN_ERR(wc_delete(b, "A/D/G"));
  SVN_ERR(wc_delete(b, "A/D/H"));

  /* Test copying various things */

  {
    struct copy_subtest_t subtests[] =
      {
        /* file onto nothing */
        { "iota", "A/C/copy1", {
            { 3, "",                "normal",       1, "iota" },
          } },

        /* dir onto nothing */
        { "A/B/E", "A/C/copy2", {
            { 3, "",                "normal",       1, "A/B/E" },
            { 3, "alpha",           "normal",       1, "A/B/E/alpha" },
            { 3, "beta",            "normal",       1, "A/B/E/beta" },
          } },

        /* file onto a schedule-delete file */
        { "iota", "A/B/lambda", {
            { 0, "",                "normal",       1, "A/B/lambda" },
            { 3, "",                "normal",       1, "iota" },
          } },

        /* dir onto a schedule-delete dir */
        { "A/B/E", "A/D/G", {
            { 0, "",                "normal",       1, "A/D/G" },
            { 0, "pi",              "normal",       1, "A/D/G/pi" },
            { 0, "rho",             "normal",       1, "A/D/G/rho" },
            { 0, "tau",             "normal",       1, "A/D/G/tau" },
            { 3, "",                "normal",       1, "A/B/E" },
            { 3, "pi",              "base-deleted", NO_COPY_FROM },
            { 3, "rho",             "base-deleted", NO_COPY_FROM },
            { 3, "tau",             "base-deleted", NO_COPY_FROM },
            { 3, "alpha",           "normal",       1, "A/B/E/alpha" },
            { 3, "beta",            "normal",       1, "A/B/E/beta" },
          } },

        /* dir onto a schedule-delete file */
        { "A/B/E", "A/D/gamma", {
            { 0, "",                "normal",       1, "A/D/gamma" },
            { 3, "",                "normal",       1, "A/B/E" },
            { 3, "alpha",           "normal",       1, "A/B/E/alpha" },
            { 3, "beta",            "normal",       1, "A/B/E/beta" },
          } },

        /* file onto a schedule-delete dir */
        { "iota", "A/D/H", {
            { 0, "",                "normal",       1, "A/D/H" },
            { 0, "chi",             "normal",       1, "A/D/H/chi" },
            { 0, "psi",             "normal",       1, "A/D/H/psi" },
            { 0, "omega",           "normal",       1, "A/D/H/omega" },
            { 3, "",                "normal",       1, "iota" },
            { 3, "chi",             "base-deleted", NO_COPY_FROM },
            { 3, "psi",             "base-deleted", NO_COPY_FROM },
            { 3, "omega",           "base-deleted", NO_COPY_FROM },
          } },

        { 0 }
      };
    struct copy_subtest_t *subtest;
    svn_client_ctx_t *ctx;

    /* Fix up the expected->local_relpath fields in the subtest data to be
     * relative to the WC root rather than to the copy destination dir. */
    for (subtest = subtests; subtest->from_path; subtest++)
      {
        nodes_row_t *row;
        for (row = &subtest->expected[0]; row->local_relpath; row++)
          row->local_relpath = svn_dirent_join(subtest->to_path,
                                               row->local_relpath, b->pool);
      }

    /* Perform each copy. */
    SVN_ERR(svn_client_create_context(&ctx, b->pool));
    for (subtest = subtests; subtest->from_path; subtest++)
      {
        svn_opt_revision_t rev = { svn_opt_revision_number, { 1 } };
        svn_client_copy_source_t source;
        apr_array_header_t *sources
          = apr_array_make(b->pool, 0, sizeof(svn_client_copy_source_t *));

        source.path = svn_path_url_add_component2(b->repos_url,
                                                  subtest->from_path,
                                                  b->pool);
        source.revision = &rev;
        source.peg_revision = &rev;
        APR_ARRAY_PUSH(sources, svn_client_copy_source_t *) = &source;
        SVN_ERR(svn_client_copy6(sources,
                                 wc_path(b, subtest->to_path),
                                 FALSE, FALSE, FALSE,
                                 NULL, NULL, NULL, ctx, b->pool));
      }

    /* Check each result. */
    for (subtest = subtests; subtest->from_path; subtest++)
      {
        SVN_ERR(check_db_rows(b, subtest->to_path, subtest->expected));
      }
  }

  return SVN_NO_ERROR;
}


static svn_error_t *
test_wc_wc_copies(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "wc_wc_copies", opts, pool));

  return wc_wc_copies(&b);
}

static svn_error_t *
test_reverts(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;
  nodes_row_t no_node_rows_expected[] = { { 0 } };

  SVN_ERR(svn_test__sandbox_create(&b, "reverts", opts, pool));

  SVN_ERR(wc_wc_copies(&b));


    /* Implement revert tests below, now that we have a wc with lots of
     copy-changes */

  SVN_ERR(wc_revert(&b, "A/B/D-added", svn_depth_infinity));
  SVN_ERR(check_db_rows(&b, "A/B/D-added", no_node_rows_expected));

  return SVN_NO_ERROR;
}

static svn_error_t *
test_deletes(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "deletes", opts, pool));
  SVN_ERR(add_and_commit_greek_tree(&b));

  file_write(&b,     "A/B/E/new-file", "New file");
  SVN_ERR(wc_add(&b, "A/B/E/new-file"));
  {
    nodes_row_t rows[] = {
      { 4, "A/B/E/new-file", "normal", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B/E/new-file", rows));
  }

  SVN_ERR(wc_delete(&b, "A/B/E/alpha"));
  {
    nodes_row_t rows[] = {
      { 0, "A/B/E/alpha", "normal",       1, "A/B/E/alpha" },
      { 4, "A/B/E/alpha", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B/E/alpha", rows));
  }

  SVN_ERR(wc_delete(&b, "A/B/F"));
  {
    nodes_row_t rows[] = {
      { 0, "A/B/F", "normal",       1, "A/B/F" },
      { 3, "A/B/F", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B/F", rows));
  }

  SVN_ERR(wc_delete(&b, "A/B"));
  {
    nodes_row_t rows[] = {
      { 0, "A/B",         "normal",       1, "A/B",        },
      { 2, "A/B/lambda",  "base-deleted", NO_COPY_FROM },
      { 0, "A/B/lambda",  "normal",       1, "A/B/lambda", },
      { 2, "A/B",         "base-deleted", NO_COPY_FROM },
      { 0, "A/B/E",       "normal",       1, "A/B/E",      },
      { 2, "A/B/E",       "base-deleted", NO_COPY_FROM },
      { 0, "A/B/E/alpha", "normal",       1, "A/B/E/alpha" },
      { 2, "A/B/E/alpha", "base-deleted", NO_COPY_FROM },
      { 0, "A/B/E/beta",  "normal",       1, "A/B/E/beta" },
      { 2, "A/B/E/beta",  "base-deleted", NO_COPY_FROM },
      { 0, "A/B/F",       "normal",       1, "A/B/F",      },
      { 2, "A/B/F",       "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B", rows));
  }

  return SVN_NO_ERROR;
}

static svn_error_t *
test_adds(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "adds", opts, pool));
  SVN_ERR(add_and_commit_greek_tree(&b));

  /* add file */
  file_write(&b, "new-file", "New file");
  SVN_ERR(wc_add(&b, "new-file"));
  {
    nodes_row_t rows[] = {
      { 1, "new-file",    "normal",       NO_COPY_FROM     },
      { 0 } };
    SVN_ERR(check_db_rows(&b, "new-file", rows));
  }

  /* add dir */
  SVN_ERR(wc_mkdir(&b, "new-dir"));
  SVN_ERR(wc_mkdir(&b, "new-dir/D2"));
  {
    nodes_row_t rows[] = {
      { 1, "new-dir",     "normal",       NO_COPY_FROM     },
      { 2, "new-dir/D2",  "normal",       NO_COPY_FROM     },
      { 0 } };
    SVN_ERR(check_db_rows(&b, "new-dir", rows));
  }

  /* replace file */
  SVN_ERR(wc_delete(&b, "iota"));
  file_write(&b, "iota", "New iota file");
  SVN_ERR(wc_add(&b, "iota"));
  {
    nodes_row_t rows[] = {
      { 0, "iota",        "normal",       1, "iota"        },
      { 1, "iota",        "normal",       NO_COPY_FROM     },
      { 0 } };
    SVN_ERR(check_db_rows(&b, "iota", rows));
  }

  /* replace dir */
  SVN_ERR(wc_delete(&b, "A/B/E"));
  SVN_ERR(wc_mkdir(&b, "A/B/E"));
  SVN_ERR(wc_mkdir(&b, "A/B/E/D2"));
  {
    nodes_row_t rows[] = {
      { 0, "A/B/E",       "normal",       1, "A/B/E"       },
      { 0, "A/B/E/alpha", "normal",       1, "A/B/E/alpha" },
      { 0, "A/B/E/beta",  "normal",       1, "A/B/E/beta"  },
      { 3, "A/B/E",       "normal",       NO_COPY_FROM     },
      { 4, "A/B/E/D2",    "normal",       NO_COPY_FROM     },
      { 3, "A/B/E/alpha", "base-deleted", NO_COPY_FROM     },
      { 3, "A/B/E/beta",  "base-deleted", NO_COPY_FROM     },
      { 0 } };
    SVN_ERR(check_db_rows(&b, "A/B/E", rows));
  }

  return SVN_NO_ERROR;
}

static svn_error_t *
test_adds_change_kind(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "adds", opts, pool));
  SVN_ERR(add_and_commit_greek_tree(&b));

  /* replace dir with file */
  SVN_ERR(wc_delete(&b, "A/B/E"));
  file_write(&b, "A/B/E", "New E file");
  SVN_ERR(wc_add(&b, "A/B/E"));
  {
    nodes_row_t rows[] = {
      { 0, "A/B/E",       "normal",       1, "A/B/E"       },
      { 0, "A/B/E/alpha", "normal",       1, "A/B/E/alpha" },
      { 0, "A/B/E/beta",  "normal",       1, "A/B/E/beta"  },
      { 3, "A/B/E",       "normal",       NO_COPY_FROM     },
      { 3, "A/B/E/alpha", "base-deleted", NO_COPY_FROM     },
      { 3, "A/B/E/beta",  "base-deleted", NO_COPY_FROM     },
      { 0 } };
    SVN_ERR(check_db_rows(&b, "A/B/E", rows));
  }

  /* replace file with dir */
  SVN_ERR(wc_delete(&b, "iota"));
  SVN_ERR(wc_mkdir(&b, "iota"));
  SVN_ERR(wc_mkdir(&b, "iota/D2"));
  {
    nodes_row_t rows[] = {
      { 0, "iota",        "normal",       1, "iota"        },
      { 1, "iota",        "normal",       NO_COPY_FROM     },
      { 2, "iota/D2",     "normal",       NO_COPY_FROM     },
      { 0 } };
    SVN_ERR(check_db_rows(&b, "iota", rows));
  }

  return SVN_NO_ERROR;
}


static svn_error_t *
test_delete_of_copies(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "deletes_of_copies", opts, pool));
  SVN_ERR(add_and_commit_greek_tree(&b));
  SVN_ERR(wc_copy(&b, "A/B", "A/B-copied"));

  SVN_ERR(wc_delete(&b, "A/B-copied/E"));
  {
    nodes_row_t rows[] = {
      { 2, "A/B-copied/E",       "normal",       1, "A/B/E" },
      { 2, "A/B-copied/E/alpha", "normal",       1, "A/B/E/alpha" },
      { 2, "A/B-copied/E/beta",  "normal",       1, "A/B/E/beta" },
      { 3, "A/B-copied/E",       "base-deleted", NO_COPY_FROM },
      { 3, "A/B-copied/E/alpha", "base-deleted", NO_COPY_FROM },
      { 3, "A/B-copied/E/beta",  "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B-copied/E", rows));
  }

  SVN_ERR(wc_copy(&b, "A/D/G", "A/B-copied/E"));
  {
    nodes_row_t rows[] = {
      { 2, "A/B-copied/E",       "normal",       1, "A/B/E" },
      { 2, "A/B-copied/E/alpha", "normal",       1, "A/B/E/alpha" },
      { 2, "A/B-copied/E/beta",  "normal",       1, "A/B/E/beta" },
      { 3, "A/B-copied/E",       "normal",       1, "A/D/G" },
      { 3, "A/B-copied/E/alpha", "base-deleted", NO_COPY_FROM },
      { 3, "A/B-copied/E/beta",  "base-deleted", NO_COPY_FROM },
      { 3, "A/B-copied/E/pi",    "normal",       1, "A/D/G/pi" },
      { 3, "A/B-copied/E/rho",   "normal",       1, "A/D/G/rho" },
      { 3, "A/B-copied/E/tau",   "normal",       1, "A/D/G/tau" },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B-copied/E", rows));
  }

  SVN_ERR(wc_delete(&b, "A/B-copied/E/rho"));
  {
    nodes_row_t rows[] = {
      { 2, "A/B-copied/E",       "normal",       1, "A/B/E" },
      { 2, "A/B-copied/E/alpha", "normal",       1, "A/B/E/alpha" },
      { 2, "A/B-copied/E/beta",  "normal",       1, "A/B/E/beta" },
      { 3, "A/B-copied/E",       "normal",       1, "A/D/G" },
      { 3, "A/B-copied/E/alpha", "base-deleted", NO_COPY_FROM },
      { 3, "A/B-copied/E/beta",  "base-deleted", NO_COPY_FROM },
      { 3, "A/B-copied/E/pi",    "normal",       1, "A/D/G/pi" },
      { 3, "A/B-copied/E/rho",   "normal",       1, "A/D/G/rho" },
      { 3, "A/B-copied/E/tau",   "normal",       1, "A/D/G/tau" },
      { 4, "A/B-copied/E/rho",   "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B-copied/E", rows));
  }

  SVN_ERR(wc_delete(&b, "A/B-copied/E"));
  {
    nodes_row_t rows[] = {
      { 2, "A/B-copied/E",       "normal",       1, "A/B/E" },
      { 2, "A/B-copied/E/alpha", "normal",       1, "A/B/E/alpha" },
      { 2, "A/B-copied/E/beta",  "normal",       1, "A/B/E/beta" },
      { 3, "A/B-copied/E",       "base-deleted", NO_COPY_FROM },
      { 3, "A/B-copied/E/alpha", "base-deleted", NO_COPY_FROM },
      { 3, "A/B-copied/E/beta",  "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B-copied/E", rows));
  }

  SVN_ERR(wc_copy(&b, "A/B", "A/B-copied/E"));

  SVN_ERR(wc_delete(&b, "A/B-copied/E/F"));
  {
    nodes_row_t rows[] = {
      { 3, "A/B-copied/E/F", "normal",       1, "A/B/F" },
      { 4, "A/B-copied/E/F", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B-copied/E/F", rows));
  }

  SVN_ERR(wc_delete(&b, "A/B-copied"));
  {
    nodes_row_t rows[] = { { 0 } };
    SVN_ERR(check_db_rows(&b, "A/B-copied", rows));
  }

  return SVN_NO_ERROR;
}


static svn_error_t *
test_delete_with_base(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "deletes_with_base", opts, pool));
  SVN_ERR(add_and_commit_greek_tree(&b));
  SVN_ERR(wc_delete(&b, "A/B/E/beta"));
  SVN_ERR(wc_commit(&b, ""));

  SVN_ERR(wc_delete(&b, "A/B/E"));
  {
    nodes_row_t rows[] = {
      { 0, "A/B/E",       "normal",        1, "A/B/E"},
      { 0, "A/B/E/alpha", "normal",        1, "A/B/E/alpha"},
      { 0, "A/B/E/beta",  "not-present",   2, "A/B/E/beta"},
      { 3, "A/B/E",       "base-deleted",  NO_COPY_FROM},
      { 3, "A/B/E/alpha", "base-deleted",  NO_COPY_FROM},
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B/E", rows));
  }

  SVN_ERR(wc_copy(&b, "A/B/F", "A/B/E"));
  SVN_ERR(wc_copy(&b, "A/mu",  "A/B/E/alpha"));
  SVN_ERR(wc_copy(&b, "A/mu",  "A/B/E/beta"));
  {
    nodes_row_t rows[] = {
      { 0, "A/B/E",       "normal",        1, "A/B/E"},
      { 0, "A/B/E/alpha", "normal",        1, "A/B/E/alpha"},
      { 0, "A/B/E/beta",  "not-present",   2, "A/B/E/beta"},
      { 3, "A/B/E",       "base-deleted",  NO_COPY_FROM},
      { 3, "A/B/E/alpha", "base-deleted",  NO_COPY_FROM},
      { 3, "A/B/E",       "normal",        1, "A/B/F"},
      { 4, "A/B/E/alpha", "normal",        1, "A/mu"},
      { 4, "A/B/E/beta",  "normal",        1, "A/mu"},
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B/E", rows));
  }

  SVN_ERR(wc_delete(&b, "A/B/E"));
  {
    nodes_row_t rows[] = {
      { 0, "A/B/E",       "normal",        1, "A/B/E"},
      { 0, "A/B/E/alpha", "normal",        1, "A/B/E/alpha"},
      { 0, "A/B/E/beta",  "not-present",   2, "A/B/E/beta"},
      { 3, "A/B/E",       "base-deleted",  NO_COPY_FROM},
      { 3, "A/B/E/alpha", "base-deleted",  NO_COPY_FROM},
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A/B/E", rows));
  }

  return SVN_NO_ERROR;
}

static svn_error_t *
test_repo_wc_copies(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "repo_wc_copies", opts, pool));

  return repo_wc_copies(&b);
}

static svn_error_t *
test_delete_with_update(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "delete_with_update", opts, pool));
  SVN_ERR(wc_mkdir(&b, "A"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_mkdir(&b, "A/B"));
  SVN_ERR(wc_mkdir(&b, "A/B/C"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_update(&b, "", 1));

  SVN_ERR(wc_delete(&b, "A"));
  SVN_ERR(wc_mkdir(&b, "A"));
  SVN_ERR(wc_mkdir(&b, "A/B"));
  {
    nodes_row_t rows[] = {
      { 0, "A",       "normal",        1, "A"},
      { 1, "A",       "normal",        NO_COPY_FROM},
      { 2, "A/B",     "normal",        NO_COPY_FROM},
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A", rows));
  }
  SVN_ERR(wc_update(&b, "", 2));
  {
    nodes_row_t rows[] = {
      { 0, "A",       "normal",        2, "A"},
      { 0, "A/B",     "normal",        2, "A/B"},
      { 0, "A/B/C",   "normal",        2, "A/B/C"},
      { 1, "A",       "normal",        NO_COPY_FROM},
      { 1, "A/B",     "base-deleted",  NO_COPY_FROM},
      { 1, "A/B/C",   "base-deleted",  NO_COPY_FROM},
      { 2, "A/B",     "normal",        NO_COPY_FROM},
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A", rows));
  }
  SVN_ERR(wc_resolved(&b, ""));
  SVN_ERR(wc_update(&b, "", 1));
  {
    nodes_row_t rows[] = {
      { 0, "A",       "normal",        1, "A"},
      { 1, "A",       "normal",        NO_COPY_FROM},
      { 2, "A/B",     "normal",        NO_COPY_FROM},
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A", rows));
  }

  return SVN_NO_ERROR;
}


static svn_error_t *
insert_dirs(svn_test__sandbox_t *b,
            nodes_row_t *nodes)
{
  svn_sqlite__db_t *sdb;
  svn_sqlite__stmt_t *stmt;
  static const char * const statements[] = {
    "DELETE FROM nodes;",
    "INSERT INTO nodes (local_relpath, op_depth, presence, repos_path,"
    "                   revision, wc_id, repos_id, kind, depth)"
    "           VALUES (?1, ?2, ?3, ?4, ?5, 1, 1, 'dir', 'infinity');",
    "INSERT INTO nodes (local_relpath, op_depth, presence, repos_path,"
    "                   revision, parent_relpath, wc_id, repos_id, kind, depth)"
    "           VALUES (?1, ?2, ?3, ?4, ?5, ?6, 1, 1, 'dir', 'infinity');",
    NULL,
  };

  SVN_ERR(open_wc_db(&sdb, b->wc_abspath, statements, b->pool, b->pool));

  SVN_ERR(svn_sqlite__get_statement(&stmt, sdb, 0));
  SVN_ERR(svn_sqlite__step_done(stmt));

  while(nodes->local_relpath)
    {
      if (nodes->local_relpath[0])
        {
          SVN_ERR(svn_sqlite__get_statement(&stmt, sdb, 2));
          SVN_ERR(svn_sqlite__bindf(stmt, "sissrs",
                                    nodes->local_relpath,
                                    (apr_int64_t)nodes->op_depth,
                                    nodes->presence,
                                    nodes->repo_relpath,
                                    nodes->repo_revnum,
                                    svn_relpath_dirname(nodes->local_relpath,
                                                        b->pool)));
        }
      else
        {
          SVN_ERR(svn_sqlite__get_statement(&stmt, sdb, 1));
          SVN_ERR(svn_sqlite__bindf(stmt, "sissr",
                                    nodes->local_relpath,
                                    (apr_int64_t)nodes->op_depth,
                                    nodes->presence,
                                    nodes->repo_relpath,
                                    nodes->repo_revnum));
        }

      SVN_ERR(svn_sqlite__step_done(stmt));
      ++nodes;
    }

  SVN_ERR(svn_sqlite__close(sdb));

  return SVN_NO_ERROR;
}

static apr_int64_t count_rows(nodes_row_t *rows)
{
  nodes_row_t *first = rows;
  while(rows->local_relpath)
    ++rows;
  return rows - first;
}

static svn_error_t *
base_dir_insert_remove(svn_test__sandbox_t *b,
                       const char *local_relpath,
                       svn_revnum_t revision,
                       nodes_row_t *before,
                       nodes_row_t *added)
{
  nodes_row_t *after;
  const char *dir_abspath = wc_path(b, local_relpath);
  int i;
  apr_int64_t num_before = count_rows(before), num_added = count_rows(added);

  SVN_ERR(insert_dirs(b, before));

  SVN_ERR(svn_wc__db_base_add_directory(b->wc_ctx->db, dir_abspath,
                                        dir_abspath,
                                        local_relpath, b->repos_url,
                                        "not-even-a-uuid", revision,
                                        apr_hash_make(b->pool), revision,
                                        0, NULL, NULL, svn_depth_infinity,
                                        NULL, NULL, FALSE, NULL, NULL,
                                        b->pool));

  after = apr_palloc(b->pool, sizeof(*after) * (num_before + num_added + 1));
  for (i = 0; i < num_before; ++i)
    after[i] = before[i];
  for (i = 0; i < num_added; ++i)
    after[num_before+i] = added[i];
  after[num_before+num_added].local_relpath = NULL;

  SVN_ERR(check_db_rows(b, "", after));

  SVN_ERR(svn_wc__db_base_remove(b->wc_ctx->db, dir_abspath, b->pool));

  SVN_ERR(check_db_rows(b, "", before));

  return SVN_NO_ERROR;
}

static svn_error_t *
test_base_dir_insert_remove(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "base_dir_insert_remove", opts, pool));

  {
    /* /  normal                     /    normal
       A  normal                     A    normal
                                     A/B  normal
    */
    nodes_row_t before[] = {
      { 0, "",  "normal", 2, "" },
      { 0, "A", "normal", 2, "A" },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B", "normal", 2, "A/B" },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B", 2, before, added));
  }
  {
    /* /  normal                      /    normal
       A  normal  base-del            A    normal  base-del
                                      A/B  normal  base-del
    */
    nodes_row_t before[] = {
      { 0, "",  "normal",       2, "" },
      { 0, "A", "normal",       2, "A" },
      { 1, "A", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B", "normal",       2, "A/B" },
      { 1, "A/B", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B", 2, before, added));
  }
  {
    /* /  normal                       /    normal
       A  normal  normal               A    normal  normal
                                       A/B  normal  base-del
     */
    nodes_row_t before[] = {
      { 0, "",  "normal", 2, "" },
      { 0, "A", "normal", 2, "A" },
      { 1, "A", "normal", 1, "X" },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B", "normal",       2, "A/B" },
      { 1, "A/B", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B", 2, before, added));
  }
  {
    /* /    normal                     /      normal
       A    normal  normal             A      normal  normal
       A/B  normal  not-pres           A/B    normal  not-pres
                                       A/B/C  normal  base-del
     */
    nodes_row_t before[] = {
      { 0, "",    "normal",      2, "" },
      { 0, "A",   "normal",      2, "A" },
      { 0, "A/B", "normal",      2, "A/B" },
      { 1, "A",   "normal",      1, "X" },
      { 1, "A/B", "not-present", NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B/C", "normal",       2, "A/B/C" },
      { 1, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B/C", 2, before, added));
  }
  {
    /* /    normal                      /    normal
       A    normal  normal              A    normal  normal
       A/B          normal              A/B  normal  normal
     */
    nodes_row_t before[] = {
      { 0, "",    "normal", 2, "" },
      { 0, "A",   "normal", 2, "A" },
      { 1, "A",   "normal", 1, "X" },
      { 1, "A/B", "normal", NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B", "normal",       2, "A/B" },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B", 2, before, added));
  }
  {
    /* /    normal                       /    normal
       A    normal  normal               A    normal  normal
       A/B          not-pres             A/B  normal  not-pres
     */
    nodes_row_t before[] = {
      { 0, "",    "normal",      2, "" },
      { 0, "A",   "normal",      2, "A" },
      { 1, "A",   "normal",      1, "X" },
      { 1, "A/B", "not-present", NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B", "normal",       2, "A/B" },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B", 2, before, added));
  }
  {
    /* /    normal                       /    normal
       A    normal  normal               A    normal  normal
       A/B                  normal       A/B  normal  base-del  normal
     */
    nodes_row_t before[] = {
      { 0, "",    "normal",      2, "" },
      { 0, "A",   "normal",      2, "A" },
      { 1, "A",   "normal",      1, "X" },
      { 2, "A/B", "normal",      1, "Y" },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B", "normal",       2, "A/B" },
      { 1, "A/B", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B", 2, before, added));
  }
  {
    /* /      normal                          /      normal
       A      normal  normal                  A      normal  normal
       A/B    normal  base-del  normal        A/B    normal  base-del  normal
       A/B/C                    normal        A/B/C  normal  base-del  normal
     */
    nodes_row_t before[] = {
      { 0, "",    "normal",       2, "" },
      { 0, "A",   "normal",       2, "A" },
      { 0, "A/B", "normal",       2, "A/B" },
      { 1, "A",   "normal",       1, "X" },
      { 1, "A/B", "base-deleted", NO_COPY_FROM },
      { 2, "A/B", "normal",       1, "Y" },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B/C", "normal",       2, "A/B/C" },
      { 1, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B/C", 2, before, added));
  }
  {
    /* /      normal                          /      normal
       A      normal  normal                  A      normal  normal
       A/B    normal  not-pres  normal        A/B    normal  not-pres  normal
       A/B/C                    normal        A/B/C  normal  base-del  normal
     */
    nodes_row_t before[] = {
      { 0, "",      "normal",      2, "" },
      { 0, "A",     "normal",      2, "A" },
      { 0, "A/B",   "normal",      2, "A/B" },
      { 1, "A",     "normal",      1, "X" },
      { 1, "A/B",   "not-present", NO_COPY_FROM },
      { 2, "A/B",   "normal",      1, "Y" },
      { 2, "A/B/C", "normal",      NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B/C", "normal",       2, "A/B/C" },
      { 1, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B/C", 2, before, added));
  }
  {
    /*  /      normal                         /
        A      normal  normal                 A      normal  normal
        A/B    normal  base-del  normal       A/B    normal  base-del  normal
        A/B/C                    not-pres     A/B/C  normal  base-del  not-pres
     */
    nodes_row_t before[] = {
      { 0, "",      "normal",       2, "" },
      { 0, "A",     "normal",       2, "A" },
      { 0, "A/B",   "normal",       2, "A/B" },
      { 1, "A",     "normal",       1, "X" },
      { 1, "A/B",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B",   "normal",       1, "Y" },
      { 2, "A/B/C", "not-present",  NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B/C", "normal",       2, "A/B/C" },
      { 1, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B/C", 2, before, added));
  }
  {
    /*  /      normal                         /
        A      normal  normal                 A      normal  normal
        A/B    normal  not-pres  normal       A/B    normal  not-pres  normal
        A/B/C                    not-pres     A/B/C  normal  base-del  not-pres
     */
    nodes_row_t before[] = {
      { 0, "",      "normal",      2, "" },
      { 0, "A",     "normal",      2, "A" },
      { 0, "A/B",   "normal",      2, "A/B" },
      { 1, "A",     "normal",      1, "X" },
      { 1, "A/B",   "not-present", NO_COPY_FROM },
      { 2, "A/B",   "normal",      1, "Y" },
      { 2, "A/B/C", "not-present", NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B/C", "normal",       2, "A/B/C" },
      { 1, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B/C", 2, before, added));
  }
  {
    /*  /      norm                       /
        A      norm  norm                 A      norm  norm
        A/B    norm  not-p  norm          A/B    norm  not-p  norm
        A/B/C                     norm    A/B/C  norm  b-del        norm
     */
    nodes_row_t before[] = {
      { 0, "",      "normal",      2, "" },
      { 0, "A",     "normal",      2, "A" },
      { 0, "A/B",   "normal",      2, "A/B" },
      { 1, "A",     "normal",      1, "X" },
      { 1, "A/B",   "not-present", NO_COPY_FROM },
      { 2, "A/B",   "normal",      1, "Y" },
      { 3, "A/B/C", "normal",      NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B/C", "normal",       2, "A/B/C" },
      { 1, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B/C", 2, before, added));
  }
  {
    /* /      norm                     /        norm
       A      norm                     A        norm
       A/B    norm                     A/B      norm
       A/B/C  norm  -  -  norm         A/B/C    norm   -  -  norm
                                       A/B/C/D  norm   -  -  b-del
    */
    nodes_row_t before[] = {
      { 0, "",      "normal", 2, "" },
      { 0, "A",     "normal", 2, "A" },
      { 0, "A/B",   "normal", 2, "A/B" },
      { 0, "A/B/C", "normal", 2, "A/B/C" },
      { 3, "A/B/C", "normal", NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B/C/D", "normal",       2, "A/B/C/D" },
      { 3, "A/B/C/D", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B/C/D", 2, before, added));
  }
  {
    /* /      norm                     /        norm
       A      norm                     A        norm
       A/B    norm                     A/B      norm
       A/B/C  norm  -  -  norm         A/B/C    norm   -  -  norm
       A/B/C/D                  norm   A/B/C/D  norm   -  -  b-del  norm
    */
    nodes_row_t before[] = {
      { 0, "",        "normal", 2, "" },
      { 0, "A",       "normal", 2, "A" },
      { 0, "A/B",     "normal", 2, "A/B" },
      { 0, "A/B/C",   "normal", 2, "A/B/C" },
      { 3, "A/B/C",   "normal", NO_COPY_FROM },
      { 4, "A/B/C/D", "normal", NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t added[] = {
      { 0, "A/B/C/D", "normal",       2, "A/B/C/D" },
      { 3, "A/B/C/D", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(base_dir_insert_remove(&b, "A/B/C/D", 2, before, added));
  }

  return SVN_NO_ERROR;
}

static svn_error_t *
temp_op_make_copy(svn_test__sandbox_t *b,
                  const char *local_relpath,
                  nodes_row_t *before,
                  nodes_row_t *after)
{
  const char *dir_abspath = svn_path_join(b->wc_abspath, local_relpath,
                                          b->pool);

  SVN_ERR(insert_dirs(b, before));

  SVN_ERR(svn_wc__db_temp_op_make_copy(b->wc_ctx->db, dir_abspath, b->pool));

  SVN_ERR(check_db_rows(b, "", after));

  return SVN_NO_ERROR;
}

static svn_error_t *
test_temp_op_make_copy(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "temp_op_make_copy", opts, pool));

  {
    /*  /           norm        -
        A           norm        -
        A/B         norm        -       norm
        A/B/C       norm        -       base-del    norm
        A/F         norm        -       norm
        A/F/G       norm        -       norm
        A/F/H       norm        -       not-pres
        A/F/E       norm        -       base-del
        A/X         norm        -
        A/X/Y       incomplete  -
    */
    nodes_row_t before[] = {
      { 0, "",      "normal",       2, "" },
      { 0, "A",     "normal",       2, "A" },
      { 0, "A/B",   "normal",       2, "A/B" },
      { 0, "A/B/C", "normal",       2, "A/B/C" },
      { 0, "A/F",   "normal",       2, "A/F" },
      { 0, "A/F/G", "normal",       2, "A/F/G" },
      { 0, "A/F/H", "normal",       2, "A/F/H" },
      { 0, "A/F/E", "normal",       2, "A/F/E" },
      { 0, "A/X",   "normal",       2, "A/X" },
      { 0, "A/X/Y", "incomplete",   2, "A/X/Y" },
      { 2, "A/B",   "normal",       NO_COPY_FROM },
      { 2, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 3, "A/B/C", "normal",       NO_COPY_FROM },
      { 2, "A/F",   "normal",       1, "S2" },
      { 2, "A/F/G", "normal",       1, "S2/G" },
      { 2, "A/F/H", "not-present",  1, "S2/H" },
      { 2, "A/F/E", "base-deleted", 2, "A/F/E" },
      { 0 }
    };
    /*  /           norm        -
        A           norm        norm
        A/B         norm        norm        norm
        A/B/C       norm        norm        base-del    norm
        A/F         norm        norm        norm
        A/F/G       norm        norm        norm
        A/F/H       norm        norm        not-pres
        A/F/E       norm        norm        base-del
        A/X         norm        norm
        A/X/Y       incomplete  incomplete
    */
    nodes_row_t after[] = {
      { 0, "",      "normal",       2, "" },
      { 0, "A",     "normal",       2, "A" },
      { 0, "A/B",   "normal",       2, "A/B" },
      { 0, "A/B/C", "normal",       2, "A/B/C" },
      { 0, "A/F",   "normal",       2, "A/F" },
      { 0, "A/F/G", "normal",       2, "A/F/G" },
      { 0, "A/F/H", "normal",       2, "A/F/H" },
      { 0, "A/F/E", "normal",       2, "A/F/E" },
      { 0, "A/X",   "normal",       2, "A/X" },
      { 0, "A/X/Y", "incomplete",   2, "A/X/Y" },
      { 1, "A",     "normal",       2, "A" },
      { 1, "A/B",   "normal",       2, "A/B" },
      { 1, "A/B/C", "normal",       2, "A/B/C" },
      { 1, "A/F",   "normal",       2, "A/F" },
      { 1, "A/F/G", "normal",       2, "A/F/G" },
      { 1, "A/F/H", "normal",       2, "A/F/H" },
      { 1, "A/F/E", "normal",       2, "A/F/E" },
      { 1, "A/X",   "normal",       2, "A/X" },
      { 1, "A/X/Y", "incomplete",   2, "A/X/Y" },
      { 2, "A/B",   "normal",       NO_COPY_FROM },
      { 2, "A/B",   "normal",       NO_COPY_FROM },
      { 2, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 2, "A/F",   "normal",       1, "S2" },
      { 2, "A/F/E", "base-deleted", 2, "A/F/E" },
      { 2, "A/F/G", "normal",       1, "S2/G" },
      { 2, "A/F/H", "not-present",  1, "S2/H" },
      { 3, "A/B/C", "normal",       NO_COPY_FROM },
      { 0 }
    };

    SVN_ERR(temp_op_make_copy(&b, "A", before, after));
  }

  return SVN_NO_ERROR;
}

static svn_error_t *
test_wc_move(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "wc_move", opts, pool));
  SVN_ERR(wc_mkdir(&b, "A"));
  SVN_ERR(wc_mkdir(&b, "A/B"));
  SVN_ERR(wc_mkdir(&b, "A/B/C"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_update(&b, "", 1));

  SVN_ERR(wc_move(&b, "A/B/C", "A/B/C-move"));
  {
    nodes_row_t rows[] = {
      { 0, "",           "normal",       1, "" },
      { 0, "A",          "normal",       1, "A" },
      { 0, "A/B",        "normal",       1, "A/B" },
      { 0, "A/B/C",      "normal",       1, "A/B/C" },
      { 3, "A/B/C",      "base-deleted", NO_COPY_FROM },
      { 3, "A/B/C-move", "normal",       1, "A/B/C" },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "", rows));
  }

  SVN_ERR(wc_move(&b, "A/B", "A/B-move"));
  {
    nodes_row_t rows[] = {
      { 0, "",                "normal",       1, "" },
      { 0, "A",               "normal",       1, "A" },
      { 0, "A/B",             "normal",       1, "A/B" },
      { 0, "A/B/C",           "normal",       1, "A/B/C" },
      { 2, "A/B",             "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C",           "base-deleted", NO_COPY_FROM },
      { 2, "A/B-move",        "normal",       1, "A/B" },
      { 2, "A/B-move/C",      "normal",       1, "A/B/C" },
      { 3, "A/B-move/C",      "base-deleted", NO_COPY_FROM },
      { 3, "A/B-move/C-move", "normal",       1, "A/B/C" },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "", rows));
  }

  return SVN_NO_ERROR;
}

static svn_error_t *
test_mixed_rev_copy(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "mixed_rev_copy", opts, pool));
  SVN_ERR(wc_mkdir(&b, "A"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_mkdir(&b, "A/B"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_mkdir(&b, "A/B/C"));
  SVN_ERR(wc_commit(&b, ""));

  SVN_ERR(wc_copy(&b, "A", "X"));
  {
    nodes_row_t rows[] = {
      { 1, "X",     "normal",       1, "A" },
      { 1, "X/B",   "not-present",  2, "A/B" },
      { 2, "X/B",   "normal",       2, "A/B" },
      { 2, "X/B/C", "not-present",  3, "A/B/C" },
      { 3, "X/B/C", "normal",       3, "A/B/C" },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "X", rows));
  }

  SVN_ERR(wc_copy(&b, "A/B", "X/Y"));
  {
    nodes_row_t rows[] = {
      { 1, "X",     "normal",       1, "A" },
      { 1, "X/B",   "not-present",  2, "A/B" },
      { 2, "X/B",   "normal",       2, "A/B" },
      { 2, "X/B/C", "not-present",  3, "A/B/C" },
      { 3, "X/B/C", "normal",       3, "A/B/C" },
      { 2, "X/Y",   "normal",       2, "A/B" },
      { 2, "X/Y/C", "not-present",  3, "A/B/C" },
      { 3, "X/Y/C", "normal",       3, "A/B/C" },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "X", rows));
  }

  SVN_ERR(wc_delete(&b, "X/B/C"));
  {
    nodes_row_t rows[] = {
      { 1, "X",     "normal",       1, "A" },
      { 1, "X/B",   "not-present",  2, "A/B" },
      { 2, "X/B",   "normal",       2, "A/B" },
      { 2, "X/B/C", "not-present",  3, "A/B/C" },
      { 2, "X/Y",   "normal",       2, "A/B" },
      { 2, "X/Y/C", "not-present",  3, "A/B/C" },
      { 3, "X/Y/C", "normal",       3, "A/B/C" },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "X", rows));
  }

  SVN_ERR(wc_delete(&b, "X"));
  SVN_ERR(wc_update(&b, "A/B/C", 0));
  {
    nodes_row_t rows[] = {
      { 0, "",      "normal",       0, "" },
      { 0, "A",     "normal",       1, "A" },
      { 0, "A/B",   "normal",       2, "A/B" },
      { 0, "A/B/C", "not-present",  0, "A/B/C" },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "", rows));
  }

  SVN_ERR(wc_copy(&b, "A", "X"));
  {
    nodes_row_t rows[] = {
      { 1, "X",     "normal",       1, "A" },
      { 1, "X/B",   "not-present",  2, "A/B" },
      { 2, "X/B",   "normal",       2, "A/B" },
      { 2, "X/B/C", "not-present",  0, "A/B/C" },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "X", rows));
  }

  return SVN_NO_ERROR;
}

static svn_error_t *
test_delete_of_replace(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "delete_of_replace", opts, pool));
  SVN_ERR(wc_mkdir(&b, "A"));
  SVN_ERR(wc_mkdir(&b, "A/B"));
  SVN_ERR(wc_mkdir(&b, "A/B/C"));
  SVN_ERR(wc_mkdir(&b, "A/B/C/F"));
  SVN_ERR(wc_mkdir(&b, "A/B/C/F/K"));
  SVN_ERR(wc_mkdir(&b, "A/B/C/G"));
  SVN_ERR(wc_mkdir(&b, "A/B/C/G/K"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_update(&b, "", 1));

  SVN_ERR(wc_copy(&b, "A", "X"));
  SVN_ERR(wc_move(&b, "X/B/C/F", "X/B/C/H"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_update(&b, "", 2));

  SVN_ERR(wc_delete(&b, "A/B"));
  SVN_ERR(wc_copy(&b, "X/B", "A/B"));
  {
    nodes_row_t rows[] = {
      { 0, "A",         "normal",       2, "A" },
      { 0, "A/B",       "normal",       2, "A/B" },
      { 0, "A/B/C",     "normal",       2, "A/B/C" },
      { 0, "A/B/C/F",   "normal",       2, "A/B/C/F" },
      { 0, "A/B/C/F/K", "normal",       2, "A/B/C/F/K" },
      { 0, "A/B/C/G",   "normal",       2, "A/B/C/G" },
      { 0, "A/B/C/G/K", "normal",       2, "A/B/C/G/K" },
      { 2, "A/B",       "normal",       2, "X/B" },
      { 2, "A/B/C",     "normal",       2, "X/B/C" },
      { 2, "A/B/C/F",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C/F/K", "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C/G",   "normal",       2, "X/B/C/G" },
      { 2, "A/B/C/G/K", "normal",       2, "X/B/C/G/K" },
      { 2, "A/B/C/H",   "normal",       2, "X/B/C/H" },
      { 2, "A/B/C/H/K", "normal",       2, "X/B/C/H/K" },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A", rows));
  }

  SVN_ERR(wc_delete(&b, "A/B"));
  {
    nodes_row_t rows[] = {
      { 0, "A",         "normal",       2, "A" },
      { 0, "A/B",       "normal",       2, "A/B" },
      { 0, "A/B/C",     "normal",       2, "A/B/C" },
      { 0, "A/B/C/F",   "normal",       2, "A/B/C/F" },
      { 0, "A/B/C/F/K", "normal",       2, "A/B/C/F/K" },
      { 0, "A/B/C/G",   "normal",       2, "A/B/C/G" },
      { 0, "A/B/C/G/K", "normal",       2, "A/B/C/G/K" },
      { 2, "A/B",       "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C",     "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C/F",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C/F/K", "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C/G",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C/G/K", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A", rows));
  }

  return SVN_NO_ERROR;
}

static svn_error_t *
test_del_replace_not_present(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "del_replace_not_present", opts, pool));
  SVN_ERR(wc_mkdir(&b, "A"));
  SVN_ERR(wc_mkdir(&b, "A/B"));
  SVN_ERR(wc_mkdir(&b, "A/B/X"));
  SVN_ERR(wc_mkdir(&b, "A/B/Y"));
  SVN_ERR(wc_mkdir(&b, "A/B/Z"));
  SVN_ERR(wc_commit(&b, ""));

  SVN_ERR(wc_copy(&b, "A", "X"));
  SVN_ERR(wc_mkdir(&b, "X/B/W"));
  SVN_ERR(wc_commit(&b, ""));

  SVN_ERR(wc_update(&b, "", 2));
  SVN_ERR(wc_update(&b, "A/B/X", 0));
  SVN_ERR(wc_update(&b, "A/B/Y", 0));
  SVN_ERR(wc_update(&b, "X/B/W", 0));
  SVN_ERR(wc_update(&b, "X/B/Y", 0));
  SVN_ERR(wc_update(&b, "X/B/Z", 0));

  SVN_ERR(wc_delete(&b, "A"));
  {
    nodes_row_t rows[] = {
      { 0, "A",         "normal",       2, "A" },
      { 0, "A/B",       "normal",       2, "A/B" },
      { 0, "A/B/X",     "not-present",  0, "A/B/X" },
      { 0, "A/B/Y",     "not-present",  0, "A/B/Y" },
      { 0, "A/B/Z",     "normal",       2, "A/B/Z" },
      { 1, "A",         "base-deleted", NO_COPY_FROM },
      { 1, "A/B",       "base-deleted", NO_COPY_FROM },
      { 1, "A/B/Z",     "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A", rows));
  }

  SVN_ERR(wc_copy(&b, "X", "A"));
  {
    nodes_row_t rows[] = {
      { 0, "A",         "normal",       2, "A" },
      { 0, "A/B",       "normal",       2, "A/B" },
      { 0, "A/B/X",     "not-present",  0, "A/B/X" },
      { 0, "A/B/Y",     "not-present",  0, "A/B/Y" },
      { 0, "A/B/Z",     "normal",       2, "A/B/Z" },
      { 1, "A",         "normal",       2, "X" },
      { 1, "A/B",       "normal",       2, "X/B" },
      { 1, "A/B/W",     "not-present",  0, "X/B/W" },
      { 1, "A/B/X",     "normal",       2, "X/B/X" },
      { 1, "A/B/Y",     "not-present",  0, "X/B/Y" },
      { 1, "A/B/Z",     "not-present",  0, "X/B/Z" },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A", rows));
  }

  SVN_ERR(wc_delete(&b, "A"));
  {
    nodes_row_t rows[] = {
      { 0, "A",         "normal",       2, "A" },
      { 0, "A/B",       "normal",       2, "A/B" },
      { 0, "A/B/X",     "not-present",  0, "A/B/X" },
      { 0, "A/B/Y",     "not-present",  0, "A/B/Y" },
      { 0, "A/B/Z",     "normal",       2, "A/B/Z" },
      { 1, "A",         "base-deleted", NO_COPY_FROM },
      { 1, "A/B",       "base-deleted", NO_COPY_FROM },
      { 1, "A/B/Z",     "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A", rows));
  }

  return SVN_NO_ERROR;
}

typedef struct actual_row_t {
  const char *local_relpath;
  const char *changelist;
} actual_row_t;

static svn_error_t *
insert_actual(svn_test__sandbox_t *b,
              actual_row_t *actual)
{
  svn_sqlite__db_t *sdb;
  svn_sqlite__stmt_t *stmt;
  static const char * const statements[] = {
    "DELETE FROM actual_node;",
    "INSERT INTO actual_node (local_relpath, changelist, wc_id)"
    "                 VALUES (?1, ?2, 1)",
    "INSERT INTO actual_node (local_relpath, parent_relpath, changelist, wc_id)"
    "                VALUES (?1, ?2, ?3, 1)",
    "UPDATE nodes SET kind = 'file' WHERE wc_id = 1 and local_relpath = ?1",
    NULL,
  };

  if (!actual)
    return SVN_NO_ERROR;

  SVN_ERR(open_wc_db(&sdb, b->wc_abspath, statements, b->pool, b->pool));

  SVN_ERR(svn_sqlite__get_statement(&stmt, sdb, 0));
  SVN_ERR(svn_sqlite__step_done(stmt));

  while(actual->local_relpath)
    {
      if (actual->local_relpath[0])
        {
          SVN_ERR(svn_sqlite__get_statement(&stmt, sdb, 2));
          SVN_ERR(svn_sqlite__bindf(stmt, "sss",
                                    actual->local_relpath,
                                    svn_relpath_dirname(actual->local_relpath,
                                                        b->pool),
                                    actual->changelist));
        }
      else
        {
          SVN_ERR(svn_sqlite__get_statement(&stmt, sdb, 1));
          SVN_ERR(svn_sqlite__bindf(stmt, "ss",
                                    actual->local_relpath,
                                    actual->changelist));
        }
      SVN_ERR(svn_sqlite__step_done(stmt));
      if (actual->changelist)
        {
          SVN_ERR(svn_sqlite__get_statement(&stmt, sdb, 3));
          SVN_ERR(svn_sqlite__bindf(stmt, "s", actual->local_relpath));
          SVN_ERR(svn_sqlite__step_done(stmt));
        }
      ++actual;
    }
  SVN_ERR(svn_sqlite__close(sdb));

  return SVN_NO_ERROR;
}

static svn_error_t *
check_db_actual(svn_test__sandbox_t* b, actual_row_t *rows)
{
  svn_sqlite__db_t *sdb;
  svn_sqlite__stmt_t *stmt;
  static const char * const statements[] = {
    "SELECT local_relpath FROM actual_node WHERE wc_id = 1;",
    NULL,
  };
  svn_boolean_t have_row;
  apr_hash_t *path_hash = apr_hash_make(b->pool);

  if (!rows)
    return SVN_NO_ERROR;

  while(rows->local_relpath)
    {
      apr_hash_set(path_hash, rows->local_relpath, APR_HASH_KEY_STRING,
                   (void*)1);
      ++rows;
    }

  SVN_ERR(open_wc_db(&sdb, b->wc_abspath, statements, b->pool, b->pool));

  SVN_ERR(svn_sqlite__get_statement(&stmt, sdb, 0));
  SVN_ERR(svn_sqlite__step(&have_row, stmt));
  while (have_row)
    {
      const char *local_relpath = svn_sqlite__column_text(stmt, 0, b->pool);
      if (!apr_hash_get(path_hash, local_relpath, APR_HASH_KEY_STRING))
        return svn_error_createf(SVN_ERR_TEST_FAILED, svn_sqlite__close(sdb),
                                 "actual '%s' unexpected", local_relpath);
      apr_hash_set(path_hash, local_relpath, APR_HASH_KEY_STRING, NULL);
      SVN_ERR(svn_sqlite__step(&have_row, stmt));
    }

  if (apr_hash_count(path_hash))
    {
      const char *local_relpath
        = svn__apr_hash_index_key(apr_hash_first(b->pool, path_hash));
      return svn_error_createf(SVN_ERR_TEST_FAILED, svn_sqlite__close(sdb),
                               "actual '%s' expected", local_relpath);
    }

  SVN_ERR(svn_sqlite__reset(stmt));
  SVN_ERR(svn_sqlite__close(sdb));

  return SVN_NO_ERROR;
}

static svn_error_t *
revert(svn_test__sandbox_t *b,
       const char *local_relpath,
       svn_depth_t depth,
       nodes_row_t *before_nodes,
       nodes_row_t *after_nodes,
       actual_row_t *before_actual,
       actual_row_t *after_actual)
{
  const char *local_abspath = wc_path(b, local_relpath);
  svn_error_t *err;

  if (!before_actual)
    {
      actual_row_t actual[] = { { 0 } };
      SVN_ERR(insert_actual(b, actual));
    }

  SVN_ERR(insert_dirs(b, before_nodes));
  SVN_ERR(insert_actual(b, before_actual));
  SVN_ERR(check_db_rows(b, "", before_nodes));
  SVN_ERR(check_db_actual(b, before_actual));
  err = svn_wc__db_op_revert(b->wc_ctx->db, local_abspath, depth,
                             b->pool, b->pool);
  if (err)
    {
      /* If db_op_revert returns an error the DB should be unchanged so
         verify and return a verification error if a change is detected
         or the revert error if unchanged. */
      err = svn_error_compose_create(check_db_rows(b, "", before_nodes), err);
      err = svn_error_compose_create(check_db_actual(b, before_actual), err);
      return err;
    }
  SVN_ERR(check_db_rows(b, "", after_nodes));
  SVN_ERR(check_db_actual(b, after_actual));

  return SVN_NO_ERROR;
}

static svn_error_t *
test_op_revert(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;
  svn_error_t *err;

  SVN_ERR(svn_test__sandbox_create(&b, "test_op_revert", opts, pool));

  {
    nodes_row_t before[] = {
      { 0, "",    "normal", 4, "" },
      { 0, "A",   "normal", 4, "A" },
      { 2, "A/B", "normal", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after[] = {
      { 0, "",    "normal", 4, "" },
      { 0, "A",   "normal", 4, "A" },
      { 0 },
    };
    actual_row_t before_actual1[] = {
      { "A", NULL },
      { "A/B", NULL },
      { 0 }
    };
    actual_row_t after_actual1[] = {
      { "A", NULL },
      { 0 }
    };
    actual_row_t before_actual2[] = {
      { "A/B", NULL },
      { "A/B/C", NULL },
      { 0 }
    };
    actual_row_t after_actual2[] = {
      { "A/B", NULL },
      { 0 }
    };
    actual_row_t before_actual3[] = {
      { "", NULL },
      { "A", NULL },
      { "A/B", NULL },
      { 0 }
    };
    actual_row_t after_actual3[] = {
      { "", NULL },
      { "A/B", NULL },
      { 0 }
    };
    actual_row_t before_actual4[] = {
      { "", NULL },
      { "A/B", NULL },
      { 0 }
    };
    actual_row_t after_actual4[] = {
      { "A/B", NULL },
      { 0 }
    };
    actual_row_t common_actual5[] = {
      { "A/B", NULL },
      { "A/B/C", NULL },
      { 0 }
    };
    actual_row_t common_actual6[] = {
      { "A/B", NULL },
      { "A/B/C", NULL },
      { "A/B/C/D", NULL },
      { 0 }
    };
    SVN_ERR(revert(&b, "A/B", svn_depth_empty,
                   before, after, NULL, NULL));
    SVN_ERR(revert(&b, "A/B", svn_depth_empty,
                   before, after, before_actual1, after_actual1));
    SVN_ERR(revert(&b, "A/B/C", svn_depth_empty,
                   before, before, before_actual2, after_actual2));
    SVN_ERR(revert(&b, "A", svn_depth_empty,
                   before, before, before_actual3, after_actual3));
    SVN_ERR(revert(&b, "", svn_depth_empty,
                   before, before, before_actual4, after_actual4));
    err = revert(&b, "A/B", svn_depth_empty,
                 before, before, common_actual5, common_actual5);
    SVN_TEST_ASSERT(err && err->apr_err == SVN_ERR_WC_INVALID_OPERATION_DEPTH);
    svn_error_clear(err);
    err = revert(&b, "A/B/C", svn_depth_empty,
                 before, before, common_actual6, common_actual6);
    SVN_TEST_ASSERT(err && err->apr_err == SVN_ERR_WC_INVALID_OPERATION_DEPTH);
    svn_error_clear(err);
  }

  {
    nodes_row_t common[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "P",       "normal", 4, "P" },
      { 0, "P/Q",     "normal", 4, "P/Q" },
      { 1, "P",       "normal", 3, "V" },
      { 1, "P/Q",     "normal", 3, "V/Q" },
      { 2, "A/B",     "normal", 2, "X/B" },
      { 2, "A/B/C",   "normal", 2, "X/B/C" },
      { 2, "A/B/C/D", "normal", 2, "X/B/C/D" },
      { 1, "X",       "normal", NO_COPY_FROM },
      { 2, "X/Y",     "normal", NO_COPY_FROM },
      { 0 },
    };
    actual_row_t common_actual[] = {
      { "A/B/C/D", NULL },
      { "A/B/C", NULL },
      { "A/B", NULL },
      { "P", NULL },
      { "X", NULL },
      { 0 }
    };
    actual_row_t actual1[] = {
      { "A/B/C", NULL },
      { "A/B", NULL },
      { "P", NULL },
      { "X", NULL },
      { 0 }
    };
    actual_row_t actual2[] = {
      { "A/B/C/D", NULL },
      { "A/B", NULL },
      { "P", NULL },
      { "X", NULL },
      { 0 }
    };

    SVN_ERR(revert(&b, "A/B/C/D", svn_depth_empty,
                   common, common, NULL, NULL));
    SVN_ERR(revert(&b, "A/B/C/D", svn_depth_empty,
                   common, common, common_actual, actual1));

    SVN_ERR(revert(&b, "A/B/C", svn_depth_empty,
                   common, common, NULL, NULL));
    SVN_ERR(revert(&b, "A/B/C", svn_depth_empty,
                   common, common, common_actual, actual2));

    err = revert(&b, "A/B", svn_depth_empty,
                 common, common, NULL, NULL);
    SVN_TEST_ASSERT(err && err->apr_err == SVN_ERR_WC_INVALID_OPERATION_DEPTH);
    svn_error_clear(err);
    err = revert(&b, "A/B", svn_depth_empty,
                 common, common, common_actual, common_actual);
    SVN_TEST_ASSERT(err && err->apr_err == SVN_ERR_WC_INVALID_OPERATION_DEPTH);
    svn_error_clear(err);

    err = revert(&b, "P", svn_depth_empty,
                 common, common, NULL, NULL);
    SVN_TEST_ASSERT(err && err->apr_err == SVN_ERR_WC_INVALID_OPERATION_DEPTH);
    svn_error_clear(err);
    err = revert(&b, "P", svn_depth_empty,
                 common, common, common_actual, common_actual);
    SVN_TEST_ASSERT(err && err->apr_err == SVN_ERR_WC_INVALID_OPERATION_DEPTH);
    svn_error_clear(err);

    err = revert(&b, "X", svn_depth_empty,
                 common, common, NULL, NULL);
    SVN_TEST_ASSERT(err && err->apr_err == SVN_ERR_WC_INVALID_OPERATION_DEPTH);
    svn_error_clear(err);
    err = revert(&b, "X", svn_depth_empty,
                 common, common, common_actual, common_actual);
    SVN_TEST_ASSERT(err && err->apr_err == SVN_ERR_WC_INVALID_OPERATION_DEPTH);
    svn_error_clear(err);
  }

  {
    nodes_row_t before[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 3, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 0 },
    };
    actual_row_t before_actual[] = {
      { "A/B", NULL },
      { "A/B/C", NULL },
      { 0 }
    };
    actual_row_t after_actual[] = {
      { "A/B", NULL },
      { 0 }
    };
    SVN_ERR(revert(&b, "A/B/C", svn_depth_empty,
                   before, after, NULL, NULL));
    SVN_ERR(revert(&b, "A/B/C", svn_depth_empty,
                   before, after, before_actual, after_actual));
  }

  {
    nodes_row_t before[] = {
      { 0, "",    "normal", 4, "" },
      { 1, "A",   "normal", 2, "X" },
      { 1, "A/B", "normal", 2, "X/B" },
      { 2, "A/B", "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after[] = {
      { 0, "",    "normal", 4, "" },
      { 1, "A",   "normal", 2, "X" },
      { 1, "A/B", "normal", 2, "X/B" },
      { 0 },
    };
    actual_row_t before_actual[] = {
      { "A", NULL },
      { "A/B", NULL },
      { 0 }
    };
    actual_row_t after_actual[] = {
      { "A", NULL },
      { 0 }
    };
    SVN_ERR(revert(&b, "A/B", svn_depth_empty,
                   before, after, NULL, NULL));
    SVN_ERR(revert(&b, "A/B", svn_depth_empty,
                   before, after, before_actual, after_actual));
  }

  {
    nodes_row_t before[] = {
      { 0, "",    "normal", 4, "" },
      { 0, "A",   "normal", 4, "A" },
      { 0, "A/B", "normal", 4, "A/B" },
      { 1, "A",   "normal", 2, "X" },
      { 1, "A/B", "normal", 2, "X/B" },
      { 2, "A/B", "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after[] = {
      { 0, "",    "normal", 4, "" },
      { 0, "A",   "normal", 4, "A" },
      { 0, "A/B", "normal", 4, "A/B" },
      { 1, "A",   "normal", 2, "X" },
      { 1, "A/B", "normal", 2, "X/B" },
      { 0 },
    };
    actual_row_t before_actual[] = {
      { "A", NULL },
      { "A/B", NULL },
      { 0 },
    };
    actual_row_t after_actual[] = {
      { "A", NULL },
      { 0 },
    };
    SVN_ERR(revert(&b, "A/B", svn_depth_empty,
                   before, after, NULL, NULL));
    SVN_ERR(revert(&b, "A/B", svn_depth_empty,
                   before, after, before_actual, after_actual));
  }

  {
    nodes_row_t before[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 2, "A/B",     "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C/D", "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 3, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 3, "A/B/C/D", "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    SVN_ERR(revert(&b, "A/B", svn_depth_empty,
                   before, after, NULL, NULL));
  }

  {
    nodes_row_t before[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 1, "A",       "normal", NO_COPY_FROM },
      { 1, "A/B",     "base-deleted", NO_COPY_FROM },
      { 1, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B",     "normal", NO_COPY_FROM },
      { 3, "A/B/C",   "normal", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after1[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 1, "A",       "normal", NO_COPY_FROM },
      { 1, "A/B",     "base-deleted", NO_COPY_FROM },
      { 1, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B",     "normal", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after2[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 1, "A",       "normal", NO_COPY_FROM },
      { 1, "A/B",     "base-deleted", NO_COPY_FROM },
      { 1, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    SVN_ERR(revert(&b, "A/B/C", svn_depth_empty,
                   before, after1, NULL, NULL));
    SVN_ERR(revert(&b, "A/B", svn_depth_empty,
                   after1, after2, NULL, NULL));
  }

  {
    nodes_row_t before[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 0, "A/B/C/D", "normal", 4, "A/B/C/D" },
      { 2, "A/B",     "normal", NO_COPY_FROM },
      { 2, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C/D", "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 0, "A/B/C/D", "normal", 4, "A/B/C/D" },
      { 3, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 3, "A/B/C/D", "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    SVN_ERR(revert(&b, "A/B", svn_depth_empty,
                   before, after, NULL, NULL));
  }

  {
    nodes_row_t common[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 0, "A/B/C/D", "normal", 4, "A/B/C/D" },
      { 1, "A",       "normal", 2, "X/Y" },
      { 1, "A/B",     "normal", 2, "X/Y/B" },
      { 1, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 1, "A/B/C/D", "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    err = revert(&b, "A", svn_depth_empty,
                 common, common, NULL, NULL);
    SVN_TEST_ASSERT(err && err->apr_err == SVN_ERR_WC_INVALID_OPERATION_DEPTH);
    svn_error_clear(err);
  }

  {
    nodes_row_t before[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 1, "A",       "normal", NO_COPY_FROM },
      { 1, "A/B",     "base-deleted", NO_COPY_FROM },
      { 1, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B",     "normal", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after1[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 0 },
    };
    nodes_row_t after2[] = {
      { 0, "",        "normal", 4, "" },
      { 0, "A",       "normal", 4, "A" },
      { 0, "A/B",     "normal", 4, "A/B" },
      { 0, "A/B/C",   "normal", 4, "A/B/C" },
      { 1, "A",       "normal", NO_COPY_FROM },
      { 1, "A/B",     "base-deleted", NO_COPY_FROM },
      { 1, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    SVN_ERR(revert(&b, "", svn_depth_infinity,
                   before, after1, NULL, NULL));
    SVN_ERR(revert(&b, "A", svn_depth_infinity,
                   before, after1, NULL, NULL));
    SVN_ERR(revert(&b, "A/B", svn_depth_infinity,
                   before, after2, NULL, NULL));
    SVN_ERR(revert(&b, "A/B/C", svn_depth_empty,
                   before, before, NULL, NULL));
  }

  {
    nodes_row_t before[] = {
      { 0, "",      "normal", 4, "" },
      { 0, "A",     "normal", 4, "A" },
      { 0, "A/B",   "normal", 4, "A/B" },
      { 1, "A",     "normal", 2, "X" },
      { 1, "A/B",   "normal", 2, "X/B" },
      { 1, "A/B/C", "normal", 2, "X/B/C" },
      { 2, "A/B",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after1[] = {
      { 0, "",      "normal", 4, "" },
      { 0, "A",     "normal", 4, "A" },
      { 0, "A/B",   "normal", 4, "A/B" },
      { 1, "A",     "normal", 2, "X" },
      { 1, "A/B",   "normal", 2, "X/B" },
      { 1, "A/B/C", "normal", 2, "X/B/C" },
      { 3, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after2[] = {
      { 0, "",      "normal", 4, "" },
      { 0, "A",     "normal", 4, "A" },
      { 0, "A/B",   "normal", 4, "A/B" },
      { 1, "A",     "normal", 2, "X" },
      { 1, "A/B",   "normal", 2, "X/B" },
      { 1, "A/B/C", "normal", 2, "X/B/C" },
      { 0 },
    };
    SVN_ERR(revert(&b, "A/B", svn_depth_empty,
                   before, after1, NULL, NULL));
    SVN_ERR(revert(&b, "A/B", svn_depth_infinity,
                   before, after2, NULL, NULL));
  }

  return SVN_NO_ERROR;
}

static svn_error_t *
test_op_revert_changelist(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "test_op_revert_changelist", opts, pool));

  {
    nodes_row_t before[] = {
      { 0, "",    "normal", 4, "" },
      { 0, "A",   "normal", 4, "A" },
      { 2, "A/f", "normal", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after[] = {
      { 0, "",    "normal", 4, "" },
      { 0, "A",   "normal", 4, "A" },
      { 0 },
    };
    actual_row_t before_actual[] = {
      { "A/f", "qq" },
      { 0 },
    };
    actual_row_t after_actual[] = {
      { 0 },
    };
    SVN_ERR(revert(&b, "A/f", svn_depth_empty,
                   before, after, before_actual, after_actual));
    SVN_ERR(revert(&b, "A/f", svn_depth_infinity,
                   before, after, before_actual, after_actual));
    SVN_ERR(revert(&b, "", svn_depth_infinity,
                   before, after, before_actual, after_actual));
  }

  {
    nodes_row_t before[] = {
      { 0, "",    "normal", 4, "" },
      { 0, "A",   "normal", 4, "A" },
      { 0, "A/f", "normal", 4, "A/f" },
      { 2, "A/f", "base-deleted", NO_COPY_FROM },
      { 0 },
    };
    nodes_row_t after[] = {
      { 0, "",    "normal", 4, "" },
      { 0, "A",   "normal", 4, "A" },
      { 0, "A/f", "normal", 4, "A/f" },
      { 0 },
    };
    actual_row_t common_actual[] = {
      { "A/f", "qq" },
      { 0 },
    };
    SVN_ERR(revert(&b, "A/f", svn_depth_empty,
                   before, after, common_actual, common_actual));
    SVN_ERR(revert(&b, "A/f", svn_depth_infinity,
                   before, after, common_actual, common_actual));
    SVN_ERR(revert(&b, "", svn_depth_infinity,
                   before, after, common_actual, common_actual));
  }

  {
    nodes_row_t before[] = {
      { 0, "",    "normal", 4, "" },
      { 0, "A",   "normal", 4, "A" },
      { 0, "A/f", "normal", 4, "A/f" },
      { 0 },
    };
    nodes_row_t after[] = {
      { 0, "",    "normal", 4, "" },
      { 0, "A",   "normal", 4, "A" },
      { 0, "A/f", "normal", 4, "A/f" },
      { 0 },
    };
    actual_row_t common_actual[] = {
      { "A/f", "qq" },
      { 0 },
    };
    SVN_ERR(revert(&b, "A/f", svn_depth_empty,
                   before, after, common_actual, common_actual));
    SVN_ERR(revert(&b, "A/f", svn_depth_infinity,
                   before, after, common_actual, common_actual));
    SVN_ERR(revert(&b, "", svn_depth_infinity,
                   before, after, common_actual, common_actual));
  }

  return SVN_NO_ERROR;
}

/* Check that the (const char *) keys of HASH are exactly the
 * EXPECTED_NUM strings in EXPECTED_STRINGS.  Return an error if not. */
static svn_error_t *
check_hash_keys(apr_hash_t *hash,
                int expected_num,
                const char **expected_strings,
                apr_pool_t *scratch_pool)
{
  svn_error_t *err = SVN_NO_ERROR;
  int i;
  apr_hash_index_t *hi;

  for (i = 0; i < expected_num; i++)
    {
      const char *name = expected_strings[i];

      if (apr_hash_get(hash, name, APR_HASH_KEY_STRING))
        apr_hash_set(hash, name, APR_HASH_KEY_STRING, NULL);
      else
        err = svn_error_compose_create(
                err, svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                       _("Expected, not found: '%s'"), name));
    }
  for (hi = apr_hash_first(scratch_pool, hash); hi;
       hi = apr_hash_next(hi))
    {
      const char *name = svn__apr_hash_index_key(hi);
      err = svn_error_compose_create(
              err, svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                     _("Found, not expected: '%s'"), name));
    }
  return err;
}

/* Check that the (const char *) keys of APR_HASH are exactly the
 * strings in (const char *[]) C_ARRAY.  Return an error if not. */
#define CHECK_HASH(apr_hash, c_array, scratch_pool) \
  check_hash_keys(apr_hash, sizeof(c_array) / sizeof(c_array[0]), \
                  c_array, scratch_pool)

/* Check that the basenames of the (const char *) paths in ARRAY are exactly
 * the EXPECTED_NUM strings in EXPECTED_STRINGS.  Return an error if not. */
static svn_error_t *
check_array_strings(const apr_array_header_t *array,
                    int expected_num,
                    const char **expected_strings,
                    apr_pool_t *scratch_pool)
{
  int i;
  apr_hash_t *hash = apr_hash_make(scratch_pool);

  for (i = 0; i < array->nelts; i++)
    {
      const char *path = APR_ARRAY_IDX(array, i, const char *);

      apr_hash_set(hash, svn_path_basename(path, scratch_pool),
                   APR_HASH_KEY_STRING, "");
    }

  return check_hash_keys(hash, expected_num, expected_strings, scratch_pool);
}

/* Check that the basenames of the (const char *) paths in APR_ARRAY are
 * exactly the strings in (const char *[]) C_ARRAY. Return an error if not. */
#define CHECK_ARRAY(apr_array, c_array, scratch_pool) \
  check_array_strings(apr_array, sizeof(c_array) / sizeof(c_array[0]), \
                      c_array, scratch_pool)


/* The purpose of this test is to check whether a child of a deleted-and-
 * replaced directory is reported by various "list the children" APIs. */
static svn_error_t *
test_children_of_replaced_dir(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;
  const apr_array_header_t *children_array;
  apr_hash_t *children_hash, *conflicts_hash;
  const char *A_abspath;
  const char *working_children_exc_hidden[] = { "G", "H", "I", "J", "K", "L" };
  const char *working_children_inc_hidden[] = { "G", "H", "I", "J", "K", "L" };
  const char *all_children_inc_hidden[] = { "F", "G", "H", "I", "J", "K", "L" };

  /*
   * F - base only
   * G - base, working (from copy of X; schedule-delete)
   * H - base, working (from copy of X)
   * I - working only (from copy of X)
   * J - working only (schedule-add)
   * K - working only (from copy of X; schedule-delete)
   * L - base, working (not in copy; schedule-add)
   */

  SVN_ERR(svn_test__sandbox_create(&b, "children_of_replaced_dir", opts, pool));
  A_abspath = svn_dirent_join(b.wc_abspath, "A", pool);

  /* Set up the base state as revision 1. */
  SVN_ERR(wc_mkdir(&b, "A"));
  SVN_ERR(wc_mkdir(&b, "A/F"));
  SVN_ERR(wc_mkdir(&b, "A/G"));
  SVN_ERR(wc_mkdir(&b, "A/H"));
  SVN_ERR(wc_mkdir(&b, "A/L"));
  SVN_ERR(wc_mkdir(&b, "X"));
  SVN_ERR(wc_mkdir(&b, "X/G"));
  SVN_ERR(wc_mkdir(&b, "X/H"));
  SVN_ERR(wc_mkdir(&b, "X/I"));
  SVN_ERR(wc_mkdir(&b, "X/K"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_update(&b, "", 1));

  /* Replace A with a copy of X. */
  SVN_ERR(wc_delete(&b, "A"));
  SVN_ERR(wc_copy(&b, "X", "A"));

  /* Make other local mods. */
  SVN_ERR(wc_delete(&b, "A/G"));
  SVN_ERR(wc_mkdir(&b, "A/J"));
  SVN_ERR(wc_mkdir(&b, "A/L"));

  /* Test several variants of "list the children of 'A'". */

  SVN_ERR(svn_wc__db_read_children(&children_array, b.wc_ctx->db, A_abspath,
                                   pool, pool));
  SVN_ERR(CHECK_ARRAY(children_array, all_children_inc_hidden, pool));

  SVN_ERR(svn_wc__db_read_children_of_working_node(
            &children_array, b.wc_ctx->db, A_abspath, pool, pool));
  SVN_ERR(CHECK_ARRAY(children_array, working_children_inc_hidden, pool));

  SVN_ERR(svn_wc__node_get_children(&children_array, b.wc_ctx, A_abspath,
                                    TRUE /* show_hidden */, pool, pool));
  SVN_ERR(CHECK_ARRAY(children_array, all_children_inc_hidden, pool));

  /* I am not testing svn_wc__node_get_children(show_hidden=FALSE) because
   * I'm not sure what result we should expect if a certain child path is a
   * child of a deleted-and-replaced dir (so should be included) and is also
   * a 'hidden' child of the working dir (so should be excluded). */

  SVN_ERR(svn_wc__node_get_children_of_working_node(
            &children_array, b.wc_ctx, A_abspath, TRUE /* show_hidden */,
            pool, pool));
  SVN_ERR(CHECK_ARRAY(children_array, working_children_inc_hidden, pool));

  SVN_ERR(svn_wc__node_get_children_of_working_node(
            &children_array, b.wc_ctx, A_abspath, FALSE /* show_hidden */,
            pool, pool));
  SVN_ERR(CHECK_ARRAY(children_array, working_children_exc_hidden, pool));

  SVN_ERR(svn_wc__db_read_children_info(&children_hash, &conflicts_hash,
                                        b.wc_ctx->db, A_abspath, pool, pool));
  SVN_ERR(CHECK_HASH(children_hash, all_children_inc_hidden, pool));

  /* We don't yet have a svn_wc__db_read_children_info2() to test. */

  return SVN_NO_ERROR;
}

static svn_error_t *
do_delete(svn_test__sandbox_t *b,
          const char *local_relpath,
          nodes_row_t *before,
          nodes_row_t *after,
          actual_row_t *actual_before,
          actual_row_t *actual_after)
{
  const char *local_abspath = wc_path(b, local_relpath);

  SVN_ERR(insert_dirs(b, before));
  SVN_ERR(insert_actual(b, actual_before));
  SVN_ERR(check_db_rows(b, "", before));
  SVN_ERR(check_db_actual(b, actual_before));
  SVN_ERR(svn_wc__db_op_delete(b->wc_ctx->db, local_abspath,
                               NULL, NULL /* notification */,
                               NULL, NULL /* cancellation */,
                               b->pool));
  SVN_ERR(check_db_rows(b, "", after));
  SVN_ERR(check_db_actual(b, actual_after));

  return SVN_NO_ERROR;
}

static svn_error_t *
test_op_delete(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;
  SVN_ERR(svn_test__sandbox_create(&b, "op_delete", opts, pool));

  {
    nodes_row_t before1[] = {
      { 0, "",    "normal",       5, "" },
      { 0, "A",   "normal",       5, "A" },
      { 0, "A/B", "normal",       5, "A/B" },
      { 0 }
    };
    nodes_row_t before2[] = {
      { 0, "",    "normal",       5, "" },
      { 0, "A",   "normal",       5, "A" },
      { 0, "A/B", "normal",       5, "A/B" },
      { 1, "A",   "normal",       NO_COPY_FROM },
      { 2, "A/B", "normal",       NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t after[] = {
      { 0, "",    "normal",       5, "" },
      { 0, "A",   "normal",       5, "A" },
      { 0, "A/B", "normal",       5, "A/B" },
      { 1, "A",   "base-deleted", NO_COPY_FROM },
      { 1, "A/B", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(do_delete(&b, "A", before1, after, NULL, NULL));
    SVN_ERR(do_delete(&b, "A", before2, after, NULL, NULL));
  }

  {
    nodes_row_t before[] = {
      { 0, "",      "normal",       5, "" },
      { 0, "A",     "normal",       5, "A" },
      { 2, "A/B",   "normal",       3, "X/B" },
      { 2, "A/B/C", "normal",       3, "X/B/C" },
      { 0 }
    };
    nodes_row_t after[] = {
      { 0, "",    "normal",       5, "" },
      { 0, "A",   "normal",       5, "A" },
      { 0 }
    };
    SVN_ERR(do_delete(&b, "A/B", before, after, NULL, NULL));
  }

  {
    nodes_row_t before[] = {
      { 0, "",      "normal",       5, "" },
      { 0, "A",     "normal",       5, "A" },
      { 0, "A/B",   "normal",       5, "A/B" },
      { 0, "A/B/C", "normal",       5, "A/B/C" },
      { 1, "A",     "normal",       3, "X" },
      { 1, "A/B",   "normal",       3, "X/B" },
      { 1, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 1, "A/B/D", "normal",       3, "X/B/D" },
      { 0 }
    };
    nodes_row_t after1[] = {
      { 0, "",      "normal",       5, "" },
      { 0, "A",     "normal",       5, "A" },
      { 0, "A/B",   "normal",       5, "A/B" },
      { 0, "A/B/C", "normal",       5, "A/B/C" },
      { 1, "A",     "normal",       3, "X" },
      { 1, "A/B",   "normal",       3, "X/B" },
      { 1, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 1, "A/B/D", "normal",       3, "X/B/D" },
      { 2, "A/B",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B/D", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t after2[] = {
      { 0, "",      "normal",       5, "" },
      { 0, "A",     "normal",       5, "A" },
      { 0, "A/B",   "normal",       5, "A/B" },
      { 0, "A/B/C", "normal",       5, "A/B/C" },
      { 1, "A",     "base-deleted", NO_COPY_FROM },
      { 1, "A/B",   "base-deleted", NO_COPY_FROM },
      { 1, "A/B/C", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(do_delete(&b, "A/B", before, after1, NULL, NULL));
    SVN_ERR(do_delete(&b, "A", before, after2, NULL, NULL));
  }

  {
    nodes_row_t before[] = {
      { 0, "",        "normal",       5, "" },
      { 0, "A",       "normal",       5, "A" },
      { 0, "A/B",     "normal",       5, "A/B" },
      { 0, "A/B/C",   "normal",       5, "A/B/C" },
      { 3, "A/B/C",   "normal",       3, "X" },
      { 3, "A/B/C/D", "normal",       3, "X/D" },
      { 4, "A/B/C/D", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t after[] = {
      { 0, "",        "normal",       5, "" },
      { 0, "A",       "normal",       5, "A" },
      { 0, "A/B",     "normal",       5, "A/B" },
      { 0, "A/B/C",   "normal",       5, "A/B/C" },
      { 1, "A",       "base-deleted", NO_COPY_FROM },
      { 1, "A/B",     "base-deleted", NO_COPY_FROM },
      { 1, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(do_delete(&b, "A", before, after, NULL, NULL));
  }

  {
    nodes_row_t state1[] = {
      { 0, "",        "normal", 5, "" },
      { 0, "A",       "normal", 5, "A" },
      { 0, "A/B",     "normal", 5, "A/B" },
      { 0, "A/B/C",   "normal", 5, "A/B/C" },
      { 0, "A/B/C/D", "normal", 5, "A/B/C" },
      { 4, "A/B/C/X", "normal", NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t state2[] = {
      { 0, "",        "normal",       5, "" },
      { 0, "A",       "normal",       5, "A" },
      { 0, "A/B",     "normal",       5, "A/B" },
      { 0, "A/B/C",   "normal",       5, "A/B/C" },
      { 0, "A/B/C/D", "normal",       5, "A/B/C" },
      { 4, "A/B/C/D", "base-deleted", NO_COPY_FROM },
      { 4, "A/B/C/X", "normal",       NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t state3[] = {
      { 0, "",        "normal",       5, "" },
      { 0, "A",       "normal",       5, "A" },
      { 0, "A/B",     "normal",       5, "A/B" },
      { 0, "A/B/C",   "normal",       5, "A/B/C" },
      { 0, "A/B/C/D", "normal",       5, "A/B/C" },
      { 2, "A/B",     "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 2, "A/B/C/D", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    nodes_row_t state4[] = {
      { 0, "",        "normal",       5, "" },
      { 0, "A",       "normal",       5, "A" },
      { 0, "A/B",     "normal",       5, "A/B" },
      { 0, "A/B/C",   "normal",       5, "A/B/C" },
      { 0, "A/B/C/D", "normal",       5, "A/B/C" },
      { 1, "A",       "base-deleted", NO_COPY_FROM },
      { 1, "A/B",     "base-deleted", NO_COPY_FROM },
      { 1, "A/B/C",   "base-deleted", NO_COPY_FROM },
      { 1, "A/B/C/D", "base-deleted", NO_COPY_FROM },
      { 0 }
    };
    SVN_ERR(do_delete(&b, "A/B/C/D", state1, state2, NULL, NULL));
    SVN_ERR(do_delete(&b, "A/B", state2, state3, NULL, NULL));
    SVN_ERR(do_delete(&b, "A", state3, state4, NULL, NULL));
  }

  {
    nodes_row_t before[] = {
      { 0, "",    "normal", 5, "" },
      { 0, "A",   "normal", 5, "" },
      { 0, "A/f", "normal", 5, "" },
      { 2, "A/B", "normal", 5, "" },
      { 0 }
    };
    nodes_row_t after[] = {
      { 0, "",    "normal", 5, "" },
      { 0, "A",   "normal", 5, "" },
      { 0, "A/f", "normal", 5, "" },
      { 1, "A",   "base-deleted", NO_COPY_FROM},
      { 1, "A/f", "base-deleted", NO_COPY_FROM},
      { 0 }
    };
    actual_row_t before_actual[] = {
      { "A",     NULL },
      { "A/f",   "qq" },
      { "A/B",   NULL },
      { "A/B/C", NULL },
      { 0 },
    };
    actual_row_t after_actual[] = {
      { "A/f", "qq" },
      { 0 },
    };
    SVN_ERR(do_delete(&b, "A", before, after, before_actual, after_actual));
  }

  {
    nodes_row_t before[] = {
      { 0, "",      "normal",       5, "" },
      { 0, "A",     "normal",       5, "A" },
      { 0, "A/B",   "normal",       5, "A/B" },
      { 0, "A/B/f", "normal",       5, "A/B/f" },
      { 0, "A/B/g", "normal",       5, "A/B/g" },
      { 1, "A",     "normal",       4, "A" },
      { 1, "A/B",   "normal",       4, "A/B" },
      { 1, "A/B/f", "normal",       4, "A/B/f" },
      { 1, "A/B/g", "base-deleted", NO_COPY_FROM},
      { 0 }
    };
    nodes_row_t after[] = {
      { 0, "",      "normal",       5, "" },
      { 0, "A",     "normal",       5, "A" },
      { 0, "A/B",   "normal",       5, "A/B" },
      { 0, "A/B/f", "normal",       5, "A/B/f" },
      { 0, "A/B/g", "normal",       5, "A/B/g" },
      { 1, "A",     "normal",       4, "A" },
      { 1, "A/B",   "normal",       4, "A/B" },
      { 1, "A/B/f", "normal",       4, "A/B/f" },
      { 1, "A/B/g", "base-deleted", NO_COPY_FROM},
      { 2, "A/B",   "base-deleted", NO_COPY_FROM},
      { 2, "A/B/f", "base-deleted", NO_COPY_FROM},
      { 0 }
    };
    SVN_ERR(do_delete(&b, "A/B", before, after, NULL, NULL));
  }

  return SVN_NO_ERROR;
}

/* The purpose of this test is to check what happens if a deleted child is
   replaced by the same nodes. */
static svn_error_t *
test_child_replace_with_same_origin(const svn_test_opts_t *opts,
                                    apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "child_replace_with_same", opts, pool));

  /* Set up the base state as revision 1. */
  SVN_ERR(wc_mkdir(&b, "A"));
  SVN_ERR(wc_mkdir(&b, "A/B"));
  SVN_ERR(wc_mkdir(&b, "A/B/C"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_update(&b, "", 1));

  SVN_ERR(wc_copy(&b, "A", "X"));

  {
    nodes_row_t rows[] = {
      {1, "X",       "normal",           1, "A"},
      {1, "X/B",     "normal",           1, "A/B"},
      {1, "X/B/C",   "normal",           1, "A/B/C"},
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "X", rows));
  }

  SVN_ERR(wc_delete(&b, "X/B"));
  {
    nodes_row_t rows[] = {
      {1, "X",       "normal",           1, "A"},
      {1, "X/B",     "normal",           1, "A/B"},
      {1, "X/B/C",   "normal",           1, "A/B/C"},

      {2, "X/B",     "base-deleted",     NO_COPY_FROM },
      {2, "X/B/C",   "base-deleted",     NO_COPY_FROM },

      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "X", rows));
  }

  SVN_ERR(wc_copy(&b, "A/B", "X/B"));
  {
    /* The revisions match what was here, so for an optimal commit
       this should have exactly the same behavior as reverting X/B.

       Another copy would be fine, as that is really what the user
       did. */
    nodes_row_t rows[] = {
      {1, "X",       "normal",           1, "A"},
      {1, "X/B",     "normal",           1, "A/B"},
      {1, "X/B/C",   "normal",           1, "A/B/C"},

      /* We either expect this */
      {2, "X/B",     "normal",           1, "A/B" },
      {2, "X/B/C",   "normal",           1, "A/B/C" },

      /* Or we expect that op_depth 2 does not exist */

      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "X", rows));
  }

  return SVN_NO_ERROR;
}

/* The purpose of this test is to check what happens below a shadowed update,
   in a few scenarios */
static svn_error_t *
test_shadowed_update(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "shadowed_update", opts, pool));

  /* Set up the base state as revision 1. */
  file_write(&b, "iota", "This is iota");
  SVN_ERR(wc_add(&b, "iota"));
  SVN_ERR(wc_commit(&b, ""));

  /* And create two trees in r2 */
  SVN_ERR(wc_mkdir(&b, "A"));
  SVN_ERR(wc_mkdir(&b, "A/B"));
  SVN_ERR(wc_mkdir(&b, "A/B/C"));

  SVN_ERR(wc_mkdir(&b, "K"));
  SVN_ERR(wc_mkdir(&b, "K/L"));
  SVN_ERR(wc_mkdir(&b, "K/L/M"));
  SVN_ERR(wc_commit(&b, ""));

  /* And change something in r3 */
  file_write(&b, "iota", "This is a new iota");
  SVN_ERR(wc_commit(&b, ""));

  /* And delete C & M */
  SVN_ERR(wc_delete(&b, "A/B/C"));
  SVN_ERR(wc_delete(&b, "K/L/M"));
  SVN_ERR(wc_commit(&b, ""));

  /* And now create the shadowed situation */
  SVN_ERR(wc_update(&b, "", 2));
  SVN_ERR(wc_copy(&b, "A", "A_tmp"));
  SVN_ERR(wc_update(&b, "", 1));
  SVN_ERR(wc_move(&b, "A_tmp", "A"));

  SVN_ERR(wc_mkdir(&b, "K"));
  SVN_ERR(wc_mkdir(&b, "K/L"));
  SVN_ERR(wc_mkdir(&b, "K/L/M"));

  /* Verify situation before update */
  {
    nodes_row_t rows[] = {
      {0, "",        "normal",           1, ""},
      {0, "iota",    "normal",           1, "iota"},

      {1, "A",       "normal",           2, "A"},
      {1, "A/B",     "normal",           2, "A/B"},
      {1, "A/B/C",   "normal",           2, "A/B/C"},

      {1, "K",       "normal",           NO_COPY_FROM},
      {2, "K/L",     "normal",           NO_COPY_FROM},
      {3, "K/L/M",   "normal",           NO_COPY_FROM},
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "", rows));
  }

  /* And now bring in A and K below the local information */
  SVN_ERR(wc_update(&b, "", 3));

  {
    nodes_row_t rows[] = {

      {0, "",        "normal",           3, ""},
      {0, "iota",    "normal",           3, "iota"},

      {0, "A",       "normal",           3, "A"},
      {0, "A/B",     "normal",           3, "A/B"},
      {0, "A/B/C",   "normal",           3, "A/B/C"},

      {1, "A",       "normal",           2, "A"},
      {1, "A/B",     "normal",           2, "A/B"},
      {1, "A/B/C",   "normal",           2, "A/B/C"},

      {0, "K",       "normal",           3, "K"},
      {0, "K/L",     "normal",           3, "K/L"},
      {0, "K/L/M",   "normal",           3, "K/L/M"},

      {1, "K",       "normal",           NO_COPY_FROM},
      {1, "K/L",     "base-deleted",     NO_COPY_FROM},
      {1, "K/L/M",   "base-deleted",     NO_COPY_FROM},

      {2, "K/L",     "normal",           NO_COPY_FROM},
      {3, "K/L/M",   "normal",           NO_COPY_FROM},

      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "", rows));
  }

  /* Update again to remove C and M */
  SVN_ERR(wc_resolved(&b, "A"));
  SVN_ERR(wc_resolved(&b, "K"));
  SVN_ERR(wc_update(&b, "", 4));

  {
    nodes_row_t rows[] = {

      {0, "",        "normal",           4, ""},
      {0, "iota",    "normal",           4, "iota"},

      {0, "A",       "normal",           4, "A"},
      {0, "A/B",     "normal",           4, "A/B"},

      {1, "A",       "normal",           2, "A"},
      {1, "A/B",     "normal",           2, "A/B"},
      {1, "A/B/C",   "normal",           2, "A/B/C"},

      {0, "K",       "normal",           4, "K"},
      {0, "K/L",     "normal",           4, "K/L"},

      {1, "K",       "normal",           NO_COPY_FROM},
      {1, "K/L",     "base-deleted",     NO_COPY_FROM},

      {2, "K/L",     "normal",           NO_COPY_FROM},
      {3, "K/L/M",   "normal",           NO_COPY_FROM},

      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "", rows));
  }

  /* Update again to bring C and M back */
  SVN_ERR(wc_resolved(&b, "A"));
  SVN_ERR(wc_resolved(&b, "K"));
  SVN_ERR(wc_update(&b, "", 3));

  SVN_ERR(wc_delete(&b, "K/L/M"));
  {
    nodes_row_t rows[] = {
      {0, "",        "normal",           3, ""},
      {0, "iota",    "normal",           3, "iota"},

      {0, "A",       "normal",           3, "A"},
      {0, "A/B",     "normal",           3, "A/B"},
      {0, "A/B/C",   "normal",           3, "A/B/C"},

      {1, "A",       "normal",           2, "A"},
      {1, "A/B",     "normal",           2, "A/B"},
      {1, "A/B/C",   "normal",           2, "A/B/C"},

      {0, "K",       "normal",           3, "K"},
      {0, "K/L",     "normal",           3, "K/L"},
      {0, "K/L/M",   "normal",           3, "K/L/M"},

      {1, "K",       "normal",           NO_COPY_FROM},
      {1, "K/L",     "base-deleted",     NO_COPY_FROM},
      {1, "K/L/M",   "base-deleted",     NO_COPY_FROM},

      {2, "K/L",     "normal",           NO_COPY_FROM},

      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "", rows));
  }

  /* Resolve conflict on K and go back to r1 */
  SVN_ERR(wc_revert(&b, "K", svn_depth_infinity));
  SVN_ERR(wc_update(&b, "", 1));

  SVN_ERR(wc_mkdir(&b, "K"));
  SVN_ERR(wc_mkdir(&b, "K/L"));

  SVN_ERR(wc_update(&b, "", 3));

  {
    nodes_row_t rows[] = {

      {0, "K",       "normal",           3, "K"},
      {0, "K/L",     "normal",           3, "K/L"},
      {0, "K/L/M",   "normal",           3, "K/L/M"},

      {1, "K",       "normal",           NO_COPY_FROM},
      {1, "K/L",     "base-deleted",     NO_COPY_FROM},
      {1, "K/L/M",   "base-deleted",     NO_COPY_FROM},

      {2, "K/L",     "normal",           NO_COPY_FROM},

      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "K", rows));
  }

  /* Update the shadowed K/L/M to r4 where they do not exit */
  SVN_ERR(wc_resolved(&b, "K"));
  SVN_ERR(wc_update(&b, "K/L/M", 4));
  SVN_ERR(wc_resolved(&b, "A"));
  SVN_ERR(wc_update(&b, "A/B/C", 4));

  {
    nodes_row_t rows[] = {

      {0, "",        "normal",           3, ""},
      {0, "iota",    "normal",           3, "iota"},

      {0, "A",       "normal",           3, "A"},
      {0, "A/B",     "normal",           3, "A/B"},
      {0, "A/B/C",   "not-present",      4, "A/B/C"},

      {1, "A",       "normal",           2, "A"},
      {1, "A/B",     "normal",           2, "A/B"},
      {1, "A/B/C",   "normal",           2, "A/B/C"},

      {0, "K",       "normal",           3, "K"},
      {0, "K/L",     "normal",           3, "K/L"},
      {0, "K/L/M",   "not-present",      4, "K/L/M"},

      {1, "K",       "normal",           NO_COPY_FROM},
      {1, "K/L",     "base-deleted",     NO_COPY_FROM},

      {2, "K/L",     "normal",           NO_COPY_FROM},

      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "", rows));
  }


  return SVN_NO_ERROR;
}

/* The purpose of this test is to check what happens below a shadowed update,
   in a few scenarios */
static svn_error_t *
test_copy_of_deleted(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "copy_of_deleted", opts, pool));
  SVN_ERR(add_and_commit_greek_tree(&b));

  /* Recreate the test scenario from copy_tests.py copy_wc_url_with_absent */

  /* Delete A/B */
  SVN_ERR(wc_delete(&b, "A/B"));

  /* A/no not-present but in HEAD */
  SVN_ERR(wc_copy(&b, "A/mu", "A/no"));
  SVN_ERR(wc_commit(&b, "A/no"));
  SVN_ERR(wc_update(&b, "A/no", 1));

  /* A/mu not-present and not in HEAD */
  SVN_ERR(wc_delete(&b, "A/mu"));
  SVN_ERR(wc_commit(&b, "A/mu"));

  /* A/D excluded */
  SVN_ERR(wc_exclude(&b, "A/D"));

  /* This should have created this structure */
  {
    nodes_row_t rows[] = {

      {0, "A",           "normal",           1, "A"},
      {0, "A/B",         "normal",           1, "A/B"},
      {0, "A/B/E",       "normal",           1, "A/B/E"},
      {0, "A/B/E/alpha", "normal",           1, "A/B/E/alpha"},
      {0, "A/B/E/beta",  "normal",           1, "A/B/E/beta"},
      {0, "A/B/F",       "normal",           1, "A/B/F"},
      {0, "A/B/lambda",  "normal",           1, "A/B/lambda"},
      {0, "A/C",         "normal",           1, "A/C"},
      {0, "A/D",         "excluded",         1, "A/D"},
      {0, "A/mu",        "not-present",      3, "A/mu"},
      {0, "A/no",        "not-present",      1, "A/no"},

      {2, "A/B",         "base-deleted",     NO_COPY_FROM},
      {2, "A/B/E",       "base-deleted",     NO_COPY_FROM},
      {2, "A/B/E/alpha", "base-deleted",     NO_COPY_FROM},
      {2, "A/B/E/beta",  "base-deleted",     NO_COPY_FROM},
      {2, "A/B/lambda",  "base-deleted",     NO_COPY_FROM},
      {2, "A/B/F",       "base-deleted",     NO_COPY_FROM},

      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A", rows));
  }

  SVN_ERR(wc_copy(&b, "A", "A_copied"));

  /* I would expect this behavior, as this copies all layers where possible
     instead of just constructing a top level layer with not-present nodes
     whenever we find a deletion. */
  {
    nodes_row_t rows[] = {

      {1, "A_copied",           "normal",           1, "A"},
      {1, "A_copied/B",         "normal",           1, "A/B"},
      {1, "A_copied/B/E",       "normal",           1, "A/B/E"},
      {1, "A_copied/B/E/alpha", "normal",           1, "A/B/E/alpha"},
      {1, "A_copied/B/E/beta",  "normal",           1, "A/B/E/beta"},
      {1, "A_copied/B/F",       "normal",           1, "A/B/F"},
      {1, "A_copied/B/lambda",  "normal",           1, "A/B/lambda"},
      {1, "A_copied/C",         "normal",           1, "A/C"},
      {1, "A_copied/D",         "excluded",         1, "A/D"},
      {1, "A_copied/mu",        "not-present",      3, "A/mu"},
      {1, "A_copied/no",        "not-present",      1, "A/no"},

      {2, "A_copied/B",         "base-deleted",     NO_COPY_FROM},
      {2, "A_copied/B/E",       "base-deleted",     NO_COPY_FROM},
      {2, "A_copied/B/E/alpha", "base-deleted",     NO_COPY_FROM},
      {2, "A_copied/B/E/beta",  "base-deleted",     NO_COPY_FROM},
      {2, "A_copied/B/lambda",  "base-deleted",     NO_COPY_FROM},
      {2, "A_copied/B/F",       "base-deleted",     NO_COPY_FROM},

      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "A_copied", rows));
  }

  return SVN_NO_ERROR;
}

/* Part of issue #3702, #3865 */
static svn_error_t *
test_case_rename(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;
  apr_hash_t *dirents;

  SVN_ERR(svn_test__sandbox_create(&b, "case_rename", opts, pool));
  SVN_ERR(add_and_commit_greek_tree(&b));

  SVN_ERR(wc_move(&b, "A", "a"));
  SVN_ERR(wc_move(&b, "iota", "iotA"));

  SVN_ERR(svn_io_get_dirents3(&dirents, wc_path(&b, ""), TRUE, pool, pool));

  /* A shouldn't be there, but a should */
  SVN_TEST_ASSERT(apr_hash_get(dirents, "a", APR_HASH_KEY_STRING));
  SVN_TEST_ASSERT(apr_hash_get(dirents, "A", APR_HASH_KEY_STRING) == NULL);
  /* iota shouldn't be there, but iotA should */
  SVN_TEST_ASSERT(apr_hash_get(dirents, "iotA", APR_HASH_KEY_STRING));
  SVN_TEST_ASSERT(apr_hash_get(dirents, "iota", APR_HASH_KEY_STRING) == NULL);

  return SVN_NO_ERROR;
}

static svn_error_t *
commit_file_external(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "commit_file_external", opts, pool));
  file_write(&b, "f", "this is f\n");
  SVN_ERR(wc_add(&b, "f"));
  SVN_ERR(wc_propset(&b, "svn:externals", "^/f g", ""));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_update(&b, "", 1));
  file_write(&b, "g", "this is f\nmodified via g\n");
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_update(&b, "", 2));

  {
    nodes_row_t rows[] = {
      { 0, "",  "normal",       2, "" },
      { 0, "f", "normal",       2, "f" },
      { 0, "g", "normal",       2, "f", TRUE },
      { 0 }
    };
    SVN_ERR(check_db_rows(&b, "", rows));
  }

  return SVN_NO_ERROR;
}

/* Issue 4040 */
static svn_error_t *
incomplete_switch(const svn_test_opts_t *opts, apr_pool_t *pool)
{
  svn_test__sandbox_t b;

  SVN_ERR(svn_test__sandbox_create(&b, "incomplete_switch", opts, pool));

  SVN_ERR(wc_mkdir(&b, "A"));
  SVN_ERR(wc_mkdir(&b, "A/B"));
  SVN_ERR(wc_mkdir(&b, "A/B/C"));
  SVN_ERR(wc_mkdir(&b, "A/B/C/D"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_copy(&b, "A", "X"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_copy(&b, "A", "X/A"));
  SVN_ERR(wc_commit(&b, ""));
  SVN_ERR(wc_delete(&b, "X/A"));
  SVN_ERR(wc_commit(&b, ""));

  {
    /* Interrupted switch from A@1 to X@3 */
    nodes_row_t before[] = {
      {0, "",      "incomplete", 3, "X"},
      {0, "A",     "incomplete", 3, "X/A"},
      {0, "A/B",   "incomplete", 3, "X/A/B"},
      {0, "A/B/C", "incomplete", 3, "X/A/B/C"},
      {0, "B",     "normal",     1, "A/B"},
      {0, "B/C",   "normal",     1, "A/B/C"},
      {0, "B/C/D", "normal",     1, "A/B/C/D"},
      {0}
    };

    nodes_row_t after_update[] = { 
      {0, "",      "normal", 4, "X"},
      {0, "B",     "normal", 4, "A/B"},
      {0, "B/C",   "normal", 4, "A/B/C"},
      {0, "B/C/D", "normal", 4, "A/B/C/D"},
      {0}
    };

    SVN_ERR(insert_dirs(&b, before));
    SVN_ERR(check_db_rows(&b, "", before));
    SVN_ERR(wc_update(&b, "", 4));
    SVN_ERR(check_db_rows(&b, "", after_update));
  }

  return SVN_NO_ERROR;
}

/* ---------------------------------------------------------------------- */
/* The list of test functions */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_OPTS_PASS(test_wc_wc_copies,
                       "test_wc_wc_copies"),
    SVN_TEST_OPTS_PASS(test_reverts,
                       "test_reverts"),
    SVN_TEST_OPTS_PASS(test_deletes,
                       "test_deletes"),
    SVN_TEST_OPTS_PASS(test_delete_of_copies,
                       "test_delete_of_copies"),
    SVN_TEST_OPTS_PASS(test_delete_with_base,
                       "test_delete_with_base"),
    SVN_TEST_OPTS_PASS(test_adds,
                       "test_adds"),
    SVN_TEST_OPTS_PASS(test_repo_wc_copies,
                       "test_repo_wc_copies"),
    SVN_TEST_OPTS_PASS(test_delete_with_update,
                       "test_delete_with_update"),
    SVN_TEST_OPTS_PASS(test_adds_change_kind,
                       "test_adds_change_kind"),
    SVN_TEST_OPTS_PASS(test_base_dir_insert_remove,
                       "test_base_dir_insert_remove"),
    SVN_TEST_OPTS_PASS(test_temp_op_make_copy,
                       "test_temp_op_make_copy"),
    SVN_TEST_OPTS_PASS(test_wc_move,
                       "test_wc_move"),
    SVN_TEST_OPTS_PASS(test_mixed_rev_copy,
                        "test_mixed_rev_copy"),
    SVN_TEST_OPTS_PASS(test_delete_of_replace,
                       "test_delete_of_replace"),
    SVN_TEST_OPTS_PASS(test_del_replace_not_present,
                       "test_del_replace_not_present"),
    SVN_TEST_OPTS_PASS(test_op_revert,
                       "test_op_revert"),
    SVN_TEST_OPTS_PASS(test_op_revert_changelist,
                       "test_op_revert_changelist"),
    SVN_TEST_OPTS_PASS(test_children_of_replaced_dir,
                       "test_children_of_replaced_dir"),
    SVN_TEST_OPTS_PASS(test_op_delete,
                       "test_op_delete"),
    SVN_TEST_OPTS_PASS(test_child_replace_with_same_origin,
                       "test_child_replace_with_same"),
    SVN_TEST_OPTS_PASS(test_shadowed_update,
                       "test_shadowed_update"),
    SVN_TEST_OPTS_PASS(test_copy_of_deleted,
                       "test_copy_of_deleted (issue #3873)"),
#ifndef DARWIN
    SVN_TEST_OPTS_PASS(test_case_rename,
                       "test_case_rename on case (in)sensitive system"),
#else
    /* apr doesn't implement APR_FILEPATH_TRUENAME for MAC OS yet */
    SVN_TEST_OPTS_XFAIL(test_case_rename,
                        "test_case_rename on case (in)sensitive system"),
#endif
    SVN_TEST_OPTS_PASS(commit_file_external,
                       "commit_file_external (issue #4002)"),
    SVN_TEST_OPTS_PASS(incomplete_switch,
                       "incomplete_switch (issue 4040)"),
    SVN_TEST_NULL
  };
