/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

/*
 * tree-conflict-data-test.c -- test the storage of tree conflict data
 */

#include <stdio.h>
#include <string.h>
#include <apr_hash.h>
#include <apr_tables.h>

#include "svn_pools.h"
#include "svn_hash.h"
#include "svn_types.h"
#include "svn_wc.h"
#include "private/svn_wc_private.h"
#include "utils.h"
#include "../svn_test.h"
#include "../../libsvn_wc/tree_conflicts.h"
#include "../../libsvn_wc/wc.h"
#include "../../libsvn_wc/wc_db.h"

/* A quick way to create error messages.  */
static svn_error_t *
fail(apr_pool_t *pool, const char *fmt, ...)
{
  va_list ap;
  char *msg;

  va_start(ap, fmt);
  msg = apr_pvsprintf(pool, fmt, ap);
  va_end(ap);

  return svn_error_create(SVN_ERR_TEST_FAILED, 0, msg);
}

/* Raise a test error if EXPECTED and ACTUAL differ. */
static svn_error_t *
compare_version(const svn_wc_conflict_version_t *expected,
                const svn_wc_conflict_version_t *actual)
{
  SVN_TEST_STRING_ASSERT(expected->repos_url, actual->repos_url);
  SVN_TEST_ASSERT(expected->peg_rev == actual->peg_rev);
  SVN_TEST_STRING_ASSERT(expected->path_in_repos, actual->path_in_repos);
  SVN_TEST_ASSERT(expected->node_kind == actual->node_kind);
  return SVN_NO_ERROR;
}

/* Raise a test error if EXPECTED and ACTUAL differ or if ACTUAL is NULL. */
static svn_error_t *
compare_conflict(const svn_wc_conflict_description2_t *expected,
                 const svn_wc_conflict_description2_t *actual)
{
  SVN_TEST_ASSERT(actual != NULL);

  SVN_TEST_STRING_ASSERT(expected->local_abspath, actual->local_abspath);
  SVN_TEST_ASSERT(expected->node_kind == actual->node_kind);
  SVN_TEST_ASSERT(expected->kind == actual->kind);
  SVN_TEST_STRING_ASSERT(expected->property_name, actual->property_name);
  SVN_TEST_ASSERT(expected->is_binary == actual->is_binary);
  SVN_TEST_STRING_ASSERT(expected->mime_type, actual->mime_type);
  SVN_TEST_ASSERT(expected->action == actual->action);
  SVN_TEST_ASSERT(expected->reason == actual->reason);
  SVN_TEST_STRING_ASSERT(expected->base_abspath, actual->base_abspath);
  SVN_TEST_STRING_ASSERT(expected->their_abspath, actual->their_abspath);
  SVN_TEST_STRING_ASSERT(expected->my_abspath, actual->my_abspath);
  SVN_TEST_STRING_ASSERT(expected->merged_file, actual->merged_file);
  SVN_TEST_ASSERT(expected->operation == actual->operation);
  compare_version(expected->src_left_version, actual->src_left_version);
  compare_version(expected->src_right_version, actual->src_right_version);
  return SVN_NO_ERROR;
}

/* Create and return a tree conflict description */
static svn_wc_conflict_description2_t *
tree_conflict_create(const char *local_abspath,
                     svn_node_kind_t node_kind,
                     svn_wc_operation_t operation,
                     svn_wc_conflict_action_t action,
                     svn_wc_conflict_reason_t reason,
                     const char *left_repo,
                     const char *left_path,
                     svn_revnum_t left_revnum,
                     svn_node_kind_t left_kind,
                     const char *right_repo,
                     const char *right_path,
                     svn_revnum_t right_revnum,
                     svn_node_kind_t right_kind,
                     apr_pool_t *result_pool)
{
  svn_wc_conflict_version_t *left, *right;
  svn_wc_conflict_description2_t *conflict;

  left = svn_wc_conflict_version_create(left_repo, left_path, left_revnum,
                                        left_kind, result_pool);
  right = svn_wc_conflict_version_create(right_repo, right_path, right_revnum,
                                         right_kind, result_pool);
  conflict = svn_wc_conflict_description_create_tree2(
                    local_abspath, node_kind, operation,
                    left, right, result_pool);
  conflict->action = action;
  conflict->reason = reason;
  return conflict;
}

static svn_error_t *
test_deserialize_tree_conflict(apr_pool_t *pool)
{
  const svn_wc_conflict_description2_t *conflict;
  svn_wc_conflict_description2_t *exp_conflict;
  const char *tree_conflict_data;
  const char *local_abspath;
  const svn_skel_t *skel;

  tree_conflict_data = "(conflict Foo.c file update deleted edited "
                        "(version 0  2 -1 0  0 ) (version 0  2 -1 0  0 ))";

  SVN_ERR(svn_dirent_get_absolute(&local_abspath, "Foo.c", pool));
  exp_conflict = svn_wc_conflict_description_create_tree2(
                        local_abspath, svn_node_file, svn_wc_operation_update,
                        NULL, NULL, pool);
  exp_conflict->action = svn_wc_conflict_action_delete;
  exp_conflict->reason = svn_wc_conflict_reason_edited;

  skel = svn_skel__parse(tree_conflict_data, strlen(tree_conflict_data), pool);
  SVN_ERR(svn_wc__deserialize_conflict(&conflict, skel, "", pool, pool));

  if ((conflict->node_kind != exp_conflict->node_kind) ||
      (conflict->action    != exp_conflict->action) ||
      (conflict->reason    != exp_conflict->reason) ||
      (conflict->operation != exp_conflict->operation) ||
      (strcmp(conflict->local_abspath, exp_conflict->local_abspath) != 0))
    return fail(pool, "Unexpected tree conflict");

  return SVN_NO_ERROR;
}

static svn_error_t *
test_serialize_tree_conflict(apr_pool_t *pool)
{
  svn_wc_conflict_description2_t *conflict;
  const char *tree_conflict_data;
  const char *expected;
  const char *local_abspath;
  svn_skel_t *skel;

  SVN_ERR(svn_dirent_get_absolute(&local_abspath, "Foo.c", pool));

  conflict = svn_wc_conflict_description_create_tree2(
                    local_abspath, svn_node_file, svn_wc_operation_update,
                    NULL, NULL, pool);
  conflict->action = svn_wc_conflict_action_delete;
  conflict->reason = svn_wc_conflict_reason_edited;

  SVN_ERR(svn_wc__serialize_conflict(&skel, conflict, pool, pool));
  tree_conflict_data = svn_skel__unparse(skel, pool)->data;

  expected = "(conflict Foo.c file update deleted edited "
             "(version 0  2 -1 0  0 ) (version 0  2 -1 0  0 ))";

  if (strcmp(expected, tree_conflict_data) != 0)
    return fail(pool, "Unexpected text from tree conflict\n"
                      "  Expected: %s\n"
                      "  Actual:   %s\n", expected, tree_conflict_data);

  return SVN_NO_ERROR;
}

/* Test WC-DB-level conflict APIs. Especially tree conflicts. */
static svn_error_t *
test_read_write_tree_conflicts(const svn_test_opts_t *opts,
                               apr_pool_t *pool)
{
  svn_test__sandbox_t sbox;

  const char *parent_abspath;
  const char *child1_abspath, *child2_abspath;
  svn_wc_conflict_description2_t *conflict1, *conflict2;

  SVN_ERR(svn_test__sandbox_create(&sbox, "read_write_tree_conflicts", opts, pool));
  parent_abspath = svn_dirent_join(sbox.wc_abspath, "A", pool);
  SVN_ERR(svn_wc__db_op_add_directory(sbox.wc_ctx->db, parent_abspath, NULL,
                                      pool));
  child1_abspath = svn_dirent_join(parent_abspath, "foo", pool);
  child2_abspath = svn_dirent_join(parent_abspath, "bar", pool);

  conflict1 = tree_conflict_create(child1_abspath, svn_node_file,
                                   svn_wc_operation_update,
                                   svn_wc_conflict_action_delete,
                                   svn_wc_conflict_reason_edited,
                                   "dummy://localhost", "path/to/foo",
                                   51, svn_node_file,
                                   "dummy://localhost", "path/to/foo",
                                   52, svn_node_none,
                                   pool);

  conflict2 = tree_conflict_create(child2_abspath, svn_node_dir,
                                   svn_wc_operation_merge,
                                   svn_wc_conflict_action_replace,
                                   svn_wc_conflict_reason_edited,
                                   "dummy://localhost", "path/to/bar",
                                   51, svn_node_dir,
                                   "dummy://localhost", "path/to/bar",
                                   52, svn_node_file,
                                   pool);

  /* Write (conflict1 through WC-DB API, conflict2 through WC API) */
  SVN_ERR(svn_wc__db_op_set_tree_conflict(sbox.wc_ctx->db, child1_abspath,
                                          conflict1, pool));
  SVN_ERR(svn_wc__add_tree_conflict(sbox.wc_ctx, /*child2_abspath,*/
                                    conflict2, pool));

  /* Query (conflict1 through WC-DB API, conflict2 through WC API) */
  {
    svn_boolean_t text_c, prop_c, tree_c;

    SVN_ERR(svn_wc__internal_conflicted_p(&text_c, &prop_c, &tree_c,
                                          sbox.wc_ctx->db, child1_abspath, pool));
    SVN_TEST_ASSERT(tree_c);
    SVN_TEST_ASSERT(! text_c && ! prop_c);

    SVN_ERR(svn_wc_conflicted_p3(&text_c, &prop_c, &tree_c,
                                 sbox.wc_ctx, child2_abspath, pool));
    SVN_TEST_ASSERT(tree_c);
    SVN_TEST_ASSERT(! text_c && ! prop_c);
  }

  /* Read one (conflict1 through WC-DB API, conflict2 through WC API) */
  {
    const svn_wc_conflict_description2_t *read_conflict;

    SVN_ERR(svn_wc__db_op_read_tree_conflict(&read_conflict, sbox.wc_ctx->db,
                                             child1_abspath, pool, pool));
    SVN_ERR(compare_conflict(conflict1, read_conflict));

    SVN_ERR(svn_wc__get_tree_conflict(&read_conflict, sbox.wc_ctx,
                                      child2_abspath, pool, pool));
    SVN_ERR(compare_conflict(conflict2, read_conflict));
  }

  /* Read many (both through WC-DB API, both through WC API) */
  {
    apr_hash_t *all_conflicts;
    const svn_wc_conflict_description2_t *read_conflict;

    SVN_ERR(svn_wc__db_op_read_all_tree_conflicts(
              &all_conflicts, sbox.wc_ctx->db, parent_abspath, pool, pool));
    SVN_TEST_ASSERT(apr_hash_count(all_conflicts) == 2);
    read_conflict = apr_hash_get(all_conflicts, "foo", APR_HASH_KEY_STRING);
    SVN_ERR(compare_conflict(conflict1, read_conflict));
    read_conflict = apr_hash_get(all_conflicts, "bar", APR_HASH_KEY_STRING);
    SVN_ERR(compare_conflict(conflict2, read_conflict));

    SVN_ERR(svn_wc__get_all_tree_conflicts(
              &all_conflicts, sbox.wc_ctx, parent_abspath, pool, pool));
    SVN_TEST_ASSERT(apr_hash_count(all_conflicts) == 2);
    read_conflict = apr_hash_get(all_conflicts, child1_abspath,
                                 APR_HASH_KEY_STRING);
    SVN_ERR(compare_conflict(conflict1, read_conflict));
    read_conflict = apr_hash_get(all_conflicts, child2_abspath,
                                 APR_HASH_KEY_STRING);
    SVN_ERR(compare_conflict(conflict2, read_conflict));
  }

  /* ### TODO: to test...
   * svn_wc__db_read_conflict_victims
   * svn_wc__db_read_conflicts
   * svn_wc__node_get_conflict_info
   * svn_wc__del_tree_conflict
   */

  return SVN_NO_ERROR;
}

/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(test_deserialize_tree_conflict,
                   "deserialize tree conflict"),
    SVN_TEST_PASS2(test_serialize_tree_conflict,
                   "serialize tree conflict"),
    SVN_TEST_OPTS_PASS(test_read_write_tree_conflicts,
                       "read and write tree conflicts"),
    SVN_TEST_NULL
  };

