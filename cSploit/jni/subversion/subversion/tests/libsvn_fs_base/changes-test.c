/* changes-test.c --- test `changes' interfaces
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
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <apr.h>

#include "../svn_test.h"

#include "svn_pools.h"
#include "svn_error.h"
#include "private/svn_skel.h"

#include "../svn_test_fs.h"
#include "../../libsvn_fs_base/util/fs_skels.h"
#include "../../libsvn_fs_base/bdb/changes-table.h"



/* Helper functions/variables.  */
static const char *standard_txns[]
  = { "0", "1", "2", "3", "4", "5", "6" };
static const char *standard_changes[19][6]
     /* KEY   PATH   NODEREVID  KIND     TEXT PROP */
  = { { "0",  "/foo",  "1.0.0",  "add",     0,  0  },
      { "0",  "/foo",  "1.0.0",  "modify",  0, "1" },
      { "0",  "/bar",  "2.0.0",  "add",     0,  0  },
      { "0",  "/bar",  "2.0.0",  "modify", "1", 0  },
      { "0",  "/bar",  "2.0.0",  "modify",  0, "1" },
      { "0",  "/baz",  "3.0.0",  "add",     0,  0  },
      { "0",  "/baz",  "3.0.0",  "modify", "1", 0  },
      { "1",  "/foo",  "1.0.1",  "modify", "1", 0  },
      { "2",  "/foo",  "1.0.2",  "modify",  0, "1" },
      { "2",  "/bar",  "2.0.2",  "modify", "1", 0  },
      { "3",  "/baz",  "3.0.3",  "modify", "1", 0  },
      { "4",  "/fob",  "4.0.4",  "add",     0,  0  },
      { "4",  "/fob",  "4.0.4",  "modify", "1", 0  },
      { "5",  "/baz",  "3.0.3",  "delete",  0,  0  },
      { "5",  "/baz",  "5.0.5",  "add",     0, "1" },
      { "5",  "/baz",  "5.0.5",  "modify", "1", 0  },
      { "6",  "/fob",  "4.0.6",  "modify", "1", 0  },
      { "6",  "/fob",  "4.0.6",  "reset",   0,  0  },
      { "6",  "/fob",  "4.0.6",  "modify",  0, "1" } };


static svn_fs_path_change_kind_t string_to_kind(const char *str)
{
  if (strcmp(str, "add") == 0)
    return svn_fs_path_change_add;
  if (strcmp(str, "delete") == 0)
    return svn_fs_path_change_delete;
  if (strcmp(str, "replace") == 0)
    return svn_fs_path_change_replace;
  if (strcmp(str, "modify") == 0)
    return svn_fs_path_change_modify;
  if (strcmp(str, "reset") == 0)
    return svn_fs_path_change_reset;
  return 0;
}


/* Common args structure for several different txn_body_* functions. */
struct changes_args
{
  svn_fs_t *fs;
  const char *key;
  change_t *change;
  apr_array_header_t *raw_changes;
  apr_hash_t *changes;
};


static svn_error_t *
txn_body_changes_add(void *baton, trail_t *trail)
{
  struct changes_args *b = baton;
  return svn_fs_bdb__changes_add(b->fs, b->key, b->change,
                                 trail, trail->pool);
}


static svn_error_t *
add_standard_changes(svn_fs_t *fs,
                     apr_pool_t *pool)
{
  int i;
  struct changes_args args;
  int num_changes = sizeof(standard_changes) / sizeof(const char *) / 6;

  for (i = 0; i < num_changes; i++)
    {
      change_t change;

      /* Set up the current change item. */
      change.path = standard_changes[i][1];
      change.noderev_id = svn_fs_parse_id(standard_changes[i][2],
                                          strlen(standard_changes[i][2]),
                                          pool);
      change.kind = string_to_kind(standard_changes[i][3]);
      change.text_mod = standard_changes[i][4] ? 1 : 0;
      change.prop_mod = standard_changes[i][5] ? 1 : 0;

      /* Set up transaction baton. */
      args.fs = fs;
      args.key = standard_changes[i][0];
      args.change = &change;

      /* Write new changes to the changes table. */
      SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_changes_add, &args,
                                     TRUE, pool));
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
txn_body_changes_fetch_raw(void *baton, trail_t *trail)
{
  struct changes_args *b = baton;
  return svn_fs_bdb__changes_fetch_raw(&(b->raw_changes), b->fs, b->key,
                                       trail, trail->pool);
}


static svn_error_t *
txn_body_changes_fetch(void *baton, trail_t *trail)
{
  struct changes_args *b = baton;
  return svn_fs_bdb__changes_fetch(&(b->changes), b->fs, b->key,
                                   trail, trail->pool);
}


static svn_error_t *
txn_body_changes_delete(void *baton, trail_t *trail)
{
  struct changes_args *b = baton;
  return svn_fs_bdb__changes_delete(b->fs, b->key, trail, trail->pool);
}



/* The tests.  */

static svn_error_t *
changes_add(const svn_test_opts_t *opts,
            apr_pool_t *pool)
{
  svn_fs_t *fs;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs(&fs, "test-repo-changes-add", opts,
                                  pool));

  /* Add the standard slew of changes. */
  SVN_ERR(add_standard_changes(fs, pool));

  return SVN_NO_ERROR;
}


static svn_error_t *
changes_fetch_raw(const svn_test_opts_t *opts,
                  apr_pool_t *pool)
{
  svn_fs_t *fs;
  int i;
  int num_txns = sizeof(standard_txns) / sizeof(const char *);
  int cur_change_index = 0;
  struct changes_args args;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs(&fs, "test-repo-changes-fetch", opts,
                                  pool));

  /* First, verify that we can request changes for an arbitrary key
     without error. */
  args.fs = fs;
  args.key = "blahbliggityblah";
  SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_changes_fetch_raw, &args,
                                 FALSE, pool));
  if ((! args.raw_changes) || (args.raw_changes->nelts))
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "expected empty changes array");

  /* Add the standard slew of changes. */
  SVN_ERR(add_standard_changes(fs, pool));

  /* For each transaction, fetch that transaction's changes, and
     compare those changes against the standard changes list.  Order
     matters throughout all the changes code, so we shouldn't have to
     worry about ordering of the arrays.  */
  for (i = 0; i < num_txns; i++)
    {
      const char *txn_id = standard_txns[i];
      int j;

      /* Setup the trail baton. */
      args.fs = fs;
      args.key = txn_id;

      /* And get those changes. */
      SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_changes_fetch_raw,
                                     &args, FALSE, pool));
      if (! args.raw_changes)
        return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                 "got no changes for key '%s'", txn_id);

      for (j = 0; j < args.raw_changes->nelts; j++)
        {
          svn_string_t *noderev_id;
          svn_fs_path_change_kind_t kind;
          change_t *change = APR_ARRAY_IDX(args.raw_changes, j, change_t *);
          int mod_bit = 0;

          /* Verify that the TXN_ID matches. */
          if (strcmp(standard_changes[cur_change_index][0], txn_id))
            return svn_error_createf
              (SVN_ERR_TEST_FAILED, NULL,
               "missing some changes for key '%s'", txn_id);

          /* Verify that the PATH matches. */
          if (strcmp(standard_changes[cur_change_index][1], change->path))
            return svn_error_createf
              (SVN_ERR_TEST_FAILED, NULL,
               "paths differ in change for key '%s'", txn_id);

          /* Verify that the NODE-REV-ID matches. */
          noderev_id = svn_fs_unparse_id(change->noderev_id, pool);
          if (strcmp(standard_changes[cur_change_index][2], noderev_id->data))
            return svn_error_createf
              (SVN_ERR_TEST_FAILED, NULL,
               "node revision ids differ in change for key '%s'", txn_id);

          /* Verify that the change KIND matches. */
          kind = string_to_kind(standard_changes[cur_change_index][3]);
          if (kind != change->kind)
            return svn_error_createf
              (SVN_ERR_TEST_FAILED, NULL,
               "change kinds differ in change for key '%s'", txn_id);

          /* Verify that the change TEXT-MOD bit matches. */
          mod_bit = standard_changes[cur_change_index][4] ? 1 : 0;
          if (mod_bit != change->text_mod)
            return svn_error_createf
              (SVN_ERR_TEST_FAILED, NULL,
               "change text-mod bits differ in change for key '%s'", txn_id);

          /* Verify that the change PROP-MOD bit matches. */
          mod_bit = standard_changes[cur_change_index][5] ? 1 : 0;
          if (mod_bit != change->prop_mod)
            return svn_error_createf
              (SVN_ERR_TEST_FAILED, NULL,
               "change prop-mod bits differ in change for key '%s'", txn_id);

          cur_change_index++;
        }
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
changes_delete(const svn_test_opts_t *opts,
               apr_pool_t *pool)
{
  svn_fs_t *fs;
  int i;
  int num_txns = sizeof(standard_txns) / sizeof(const char *);
  struct changes_args args;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs(&fs, "test-repo-changes-delete", opts,
                                  pool));

  /* Add the standard slew of changes. */
  SVN_ERR(add_standard_changes(fs, pool));

  /* Now, delete all the changes we know about, verifying their removal. */
  for (i = 0; i < num_txns; i++)
    {
      args.fs = fs;
      args.key = standard_txns[i];
      SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_changes_delete,
                                     &args, FALSE, pool));
      args.changes = 0;
      SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_changes_fetch_raw,
                                     &args, FALSE, pool));
      if ((! args.raw_changes) || (args.raw_changes->nelts))
        return svn_error_createf
          (SVN_ERR_TEST_FAILED, NULL,
           "expected empty changes array for txn '%s'", args.key);
    }

  return SVN_NO_ERROR;
}


static apr_hash_t *
get_ideal_changes(const char *txn_id,
                  apr_pool_t *pool)
{
  apr_hash_t *ideal = apr_hash_make(pool);
  svn_fs_path_change_t *change;
  if (strcmp(txn_id, "0") == 0)
    {
      change = apr_palloc(pool, sizeof(*change));
      change->node_rev_id = svn_fs_parse_id("1.0.0", 5, pool);
      change->change_kind = svn_fs_path_change_add;
      change->text_mod = 0;
      change->prop_mod = 1;
      apr_hash_set(ideal, "/foo", APR_HASH_KEY_STRING, change);

      change = apr_palloc(pool, sizeof(*change));
      change->node_rev_id = svn_fs_parse_id("2.0.0", 5, pool);
      change->change_kind = svn_fs_path_change_add;
      change->text_mod = 1;
      change->prop_mod = 1;
      apr_hash_set(ideal, "/bar", APR_HASH_KEY_STRING, change);

      change = apr_palloc(pool, sizeof(*change));
      change->node_rev_id = svn_fs_parse_id("3.0.0", 5, pool);
      change->change_kind = svn_fs_path_change_add;
      change->text_mod = 1;
      change->prop_mod = 0;
      apr_hash_set(ideal, "/baz", APR_HASH_KEY_STRING, change);
    }
  if (strcmp(txn_id, "1") == 0)
    {
      change = apr_palloc(pool, sizeof(*change));
      change->node_rev_id = svn_fs_parse_id("1.0.1", 5, pool);
      change->change_kind = svn_fs_path_change_modify;
      change->text_mod = 1;
      change->prop_mod = 0;
      apr_hash_set(ideal, "/foo", APR_HASH_KEY_STRING, change);
    }
  if (strcmp(txn_id, "2") == 0)
    {
      change = apr_palloc(pool, sizeof(*change));
      change->node_rev_id = svn_fs_parse_id("1.0.2", 5, pool);
      change->change_kind = svn_fs_path_change_modify;
      change->text_mod = 0;
      change->prop_mod = 1;
      apr_hash_set(ideal, "/foo", APR_HASH_KEY_STRING, change);

      change = apr_palloc(pool, sizeof(*change));
      change->node_rev_id = svn_fs_parse_id("2.0.2", 5, pool);
      change->change_kind = svn_fs_path_change_modify;
      change->text_mod = 1;
      change->prop_mod = 0;
      apr_hash_set(ideal, "/bar", APR_HASH_KEY_STRING, change);
    }
  if (strcmp(txn_id, "3") == 0)
    {
      change = apr_palloc(pool, sizeof(*change));
      change->node_rev_id = svn_fs_parse_id("3.0.3", 5, pool);
      change->change_kind = svn_fs_path_change_modify;
      change->text_mod = 1;
      change->prop_mod = 0;
      apr_hash_set(ideal, "/baz", APR_HASH_KEY_STRING, change);
    }
  if (strcmp(txn_id, "4") == 0)
    {
      change = apr_palloc(pool, sizeof(*change));
      change->node_rev_id = svn_fs_parse_id("4.0.4", 5, pool);
      change->change_kind = svn_fs_path_change_add;
      change->text_mod = 1;
      change->prop_mod = 0;
      apr_hash_set(ideal, "/fob", APR_HASH_KEY_STRING, change);
    }
  if (strcmp(txn_id, "5") == 0)
    {
      change = apr_palloc(pool, sizeof(*change));
      change->node_rev_id = svn_fs_parse_id("5.0.5", 5, pool);
      change->change_kind = svn_fs_path_change_replace;
      change->text_mod = 1;
      change->prop_mod = 1;
      apr_hash_set(ideal, "/baz", APR_HASH_KEY_STRING, change);
    }
  if (strcmp(txn_id, "6") == 0)
    {
      change = apr_palloc(pool, sizeof(*change));
      change->node_rev_id = svn_fs_parse_id("4.0.6", 5, pool);
      change->change_kind = svn_fs_path_change_modify;
      change->text_mod = 0;
      change->prop_mod = 1;
      apr_hash_set(ideal, "/fob", APR_HASH_KEY_STRING, change);
    }
  return ideal;
}


static svn_error_t *
compare_changes(apr_hash_t *ideals,
                apr_hash_t *changes,
                const svn_test_opts_t *opts,
                const char *txn_id,
                apr_pool_t *pool)
{
  apr_hash_index_t *hi;

  for (hi = apr_hash_first(pool, ideals); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      svn_fs_path_change_t *ideal_change, *change;
      const char *path;

      /* KEY will be the path, VAL the change. */
      apr_hash_this(hi, &key, NULL, &val);
      path = (const char *) key;
      ideal_change = val;

      /* Now get the change that refers to PATH in the actual
         changes hash. */
      change = apr_hash_get(changes, path, APR_HASH_KEY_STRING);
      if (! change)
        return svn_error_createf
          (SVN_ERR_TEST_FAILED, NULL,
           "missing expected change for path '%s' in txn_id '%s'",
           path, txn_id);

      /* Verify that the NODE-REV-ID matches. */
      if (svn_fs_compare_ids(change->node_rev_id,
                             ideal_change->node_rev_id))
        return svn_error_createf
          (SVN_ERR_TEST_FAILED, NULL,
           "node revision ids differ in change for key '%s'", txn_id);

      /* Verify that the change KIND matches. */
      if (change->change_kind != ideal_change->change_kind)
        return svn_error_createf
          (SVN_ERR_TEST_FAILED, NULL,
           "change kinds differ in change for key '%s'", txn_id);

      /* Verify that the change TEXT-MOD bit matches. */
      if (change->text_mod != ideal_change->text_mod)
        return svn_error_createf
          (SVN_ERR_TEST_FAILED, NULL,
           "change text-mod bits differ in change for key '%s'", txn_id);

      /* Verify that the change PROP-MOD bit matches. */
      if (change->prop_mod != ideal_change->prop_mod)
        return svn_error_createf
          (SVN_ERR_TEST_FAILED, NULL,
           "change prop-mod bits differ in change for key '%s'", txn_id);
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
changes_fetch(const svn_test_opts_t *opts,
              apr_pool_t *pool)
{
  svn_fs_t *fs;
  int i;
  int num_txns = sizeof(standard_txns) / sizeof(const char *);
  struct changes_args args;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs(&fs, "test-repo-changes-fetch", opts,
                                  pool));

  /* First, verify that we can request changes for an arbitrary key
     without error. */
  args.fs = fs;
  args.key = "blahbliggityblah";
  SVN_ERR(svn_fs_base__retry_txn(fs, txn_body_changes_fetch, &args,
                                 FALSE, pool));
  if ((! args.changes) || (apr_hash_count(args.changes)))
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "expected empty changes hash");

  /* Add the standard slew of changes. */
  SVN_ERR(add_standard_changes(fs, pool));

  /* For each transaction, fetch that transaction's changes, and
     compare those changes against our ideal compressed changes
     hash. */
  for (i = 0; i < num_txns; i++)
    {
      const char *txn_id = standard_txns[i];
      apr_hash_t *ideals;

      /* Get the ideal changes hash. */
      ideals = get_ideal_changes(txn_id, pool);

      /* Setup the trail baton. */
      args.fs = fs;
      args.key = txn_id;

      /* And get those changes via in the internal interface, and
         verify that they are accurate. */
      SVN_ERR(svn_fs_base__retry_txn(fs, txn_body_changes_fetch, &args,
                                     FALSE, pool));
      if (! args.changes)
        return svn_error_createf
          (SVN_ERR_TEST_FAILED, NULL,
           "got no changes for key '%s'", txn_id);
      if (apr_hash_count(ideals) != apr_hash_count(args.changes))
        return svn_error_createf
          (SVN_ERR_TEST_FAILED, NULL,
           "unexpected number of changes for key '%s'", txn_id);
      SVN_ERR(compare_changes(ideals, args.changes, opts, txn_id, pool));
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
changes_fetch_ordering(const svn_test_opts_t *opts,
                       apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_revnum_t youngest_rev = 0;
  const char *txn_name;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *rev_root;
  struct changes_args args;
  apr_pool_t *subpool = svn_pool_create(pool);
  apr_hash_index_t *hi;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs
          (&fs, "test-repo-changes-fetch-ordering", opts,
           pool));

  /*** REVISION 1: Make some files and dirs. ***/
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  {
    static svn_test__txn_script_command_t script_entries[] = {
      { 'a', "dir1",        0 },
      { 'a', "file1",       "This is the file 'file1'.\n" },
      { 'a', "dir1/file2",  "This is the file 'file2'.\n" },
      { 'a', "dir1/file3",  "This is the file 'file3'.\n" },
      { 'a', "dir1/file4",  "This is the file 'file4'.\n" },
    };
    SVN_ERR(svn_test__txn_script_exec(txn_root, script_entries, 5, subpool));
  }
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /*** REVISION 2: Delete and add some stuff, non-depth-first. ***/
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  /* Don't use subpool, txn_name is used after subpool is cleared */
  SVN_ERR(svn_fs_txn_name(&txn_name, txn, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  {
    static svn_test__txn_script_command_t script_entries[] = {
      { 'd', "file1",       "This is the file 'file1'.\n" },
      { 'd', "dir1/file2",  "This is the file 'file2'.\n" },
      { 'd', "dir1/file3",  "This is the file 'file3'.\n" },
      { 'a', "dir1/file5",  "This is the file 'file4'.\n" },
      { 'a', "dir1/dir2",   0 },
      { 'd', "dir1",        0 },
      { 'a', "dir3",        0 },
    };
    SVN_ERR(svn_test__txn_script_exec(txn_root, script_entries, 7, subpool));
  }
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /*** TEST:  We should have only three changes, the deletion of 'file1'
       the deletion of 'dir1', and the addition of 'dir3'. ***/
  args.fs = fs;
  args.key = txn_name;
  SVN_ERR(svn_fs_base__retry_txn(fs, txn_body_changes_fetch, &args,
                                 FALSE, subpool));
  if ((! args.changes) || (apr_hash_count(args.changes) != 3))
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "expected changes");
  for (hi = apr_hash_first(subpool, args.changes);
       hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      svn_fs_path_change_t *change;

      /* KEY will be the path, VAL the change. */
      apr_hash_this(hi, &key, NULL, &val);
      change = val;

      if ((change->change_kind == svn_fs_path_change_add)
          && (strcmp(key, "/dir3") == 0))
        ;
      else if ((change->change_kind == svn_fs_path_change_delete)
               && ((strcmp(key, "/dir1") == 0)
                   || (strcmp(key, "/file1") == 0)))
        ;
      else
        return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                                "got wrong changes");
    }

  /*** REVISION 3: Do the same stuff as in revision 1. ***/
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  {
    static svn_test__txn_script_command_t script_entries[] = {
      { 'a', "dir1",        0 },
      { 'a', "file1",       "This is the file 'file1'.\n" },
      { 'a', "dir1/file2",  "This is the file 'file2'.\n" },
      { 'a', "dir1/file3",  "This is the file 'file3'.\n" },
      { 'a', "dir1/file4",  "This is the file 'file4'.\n" },
    };
    SVN_ERR(svn_test__txn_script_exec(txn_root, script_entries, 5, subpool));
  }
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /*** REVISION 4: Do the same stuff as in revision 2, but use a copy
       overwrite of the top directory (instead of a delete) to test
       that the 'replace' change type works, too.  (And add 'dir4'
       instead of 'dir3', since 'dir3' still exists).  ***/
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  /* Don't use subpool, txn_name is used after subpool is cleared */
  SVN_ERR(svn_fs_txn_name(&txn_name, txn, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, 1, subpool));
  {
    static svn_test__txn_script_command_t script_entries[] = {
      { 'd', "file1",       "This is the file 'file1'.\n" },
      { 'd', "dir1/file2",  "This is the file 'file2'.\n" },
      { 'd', "dir1/file3",  "This is the file 'file3'.\n" },
      { 'a', "dir1/file5",  "This is the file 'file4'.\n" },
      { 'a', "dir1/dir2",   0 },
    };
    SVN_ERR(svn_test__txn_script_exec(txn_root, script_entries, 5, subpool));
    SVN_ERR(svn_fs_copy(rev_root, "dir1", txn_root, "dir1", subpool));
    SVN_ERR(svn_fs_make_dir(txn_root, "dir4", subpool));
  }
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /*** TEST:  We should have only three changes, the deletion of 'file1'
       the replacement of 'dir1', and the addition of 'dir4'. ***/
  args.fs = fs;
  args.key = txn_name;
  SVN_ERR(svn_fs_base__retry_txn(fs, txn_body_changes_fetch, &args,
                                 FALSE, subpool));
  if ((! args.changes) || (apr_hash_count(args.changes) != 3))
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "expected changes");
  for (hi = apr_hash_first(subpool, args.changes);
       hi; hi = apr_hash_next(hi))
    {
      const void *key;
      void *val;
      svn_fs_path_change_t *change;

      /* KEY will be the path, VAL the change. */
      apr_hash_this(hi, &key, NULL, &val);
      change = val;

      if ((change->change_kind == svn_fs_path_change_add)
          && (strcmp(key, "/dir4") == 0))
        ;
      else if ((change->change_kind == svn_fs_path_change_replace)
               && (strcmp(key, "/dir1") == 0))
        ;
      else if ((change->change_kind == svn_fs_path_change_delete)
               && (strcmp(key, "/file1") == 0))
        ;
      else
        return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                                "got wrong changes");
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
changes_bad_sequences(const svn_test_opts_t *opts,
                      apr_pool_t *pool)
{
  svn_fs_t *fs;
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_error_t *err;

  /* Create a new fs and repos */
  SVN_ERR(svn_test__create_bdb_fs
          (&fs, "test-repo-changes-bad-sequences", opts,
           pool));

  /* Test changes bogus because a path's node-rev-ID changed
     unexpectedly. */
  svn_pool_clear(subpool);
  {
    static const char *bogus_changes[][6]
         /* KEY   PATH   NODEREVID  KIND       TEXT PROP */
      = { { "x",  "/foo",  "1.0.0",  "add",     0 ,  0  },
          { "x",  "/foo",  "1.0.0",  "modify",  0 , "1" },
          { "x",  "/foo",  "2.0.0",  "modify", "1", "1" } };
    int num_changes = sizeof(bogus_changes) / sizeof(const char *) / 6;
    struct changes_args args;
    int i;

    for (i = 0; i < num_changes; i++)
      {
        change_t change;

        /* Set up the current change item. */
        change.path = bogus_changes[i][1];
        change.noderev_id = svn_fs_parse_id(bogus_changes[i][2],
                                            strlen(bogus_changes[i][2]),
                                            subpool);
        change.kind = string_to_kind(bogus_changes[i][3]);
        change.text_mod = bogus_changes[i][4] ? 1 : 0;
        change.prop_mod = bogus_changes[i][5] ? 1 : 0;

        /* Set up transaction baton. */
        args.fs = fs;
        args.key = "x";
        args.change = &change;

        /* Write new changes to the changes table. */
        SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_changes_add, &args,
                                       TRUE, subpool));
      }

    /* Now read 'em back, looking for an error. */
    args.fs = fs;
    args.key = "x";
    err = svn_fs_base__retry_txn(args.fs, txn_body_changes_fetch, &args,
                                 TRUE, subpool);
    if (!err)
      {
        return svn_error_create(SVN_ERR_TEST_FAILED, 0,
                                "Expected SVN_ERR_FS_CORRUPT, got no error.");
      }
    else if (err->apr_err != SVN_ERR_FS_CORRUPT)
      {
        return svn_error_create(SVN_ERR_TEST_FAILED, err,
                                "Expected SVN_ERR_FS_CORRUPT, got a different error.");
      }
    else
      {
        svn_error_clear(err);
      }

    /* Post-test cleanup. */
    SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_changes_delete, &args,
                                   TRUE, subpool));
  }

  /* Test changes bogus because there's a change other than an
     add-type changes on a deleted path. */
  svn_pool_clear(subpool);
  {
    static const char *bogus_changes[][6]
         /* KEY   PATH   NODEREVID  KIND       TEXT PROP */
      = { { "x",  "/foo",  "1.0.0",  "delete",  0 ,  0  },
          { "x",  "/foo",  "1.0.0",  "modify", "1",  0  } };
    int num_changes = sizeof(bogus_changes) / sizeof(const char *) / 6;
    struct changes_args args;
    int i;

    for (i = 0; i < num_changes; i++)
      {
        change_t change;

        /* Set up the current change item. */
        change.path = bogus_changes[i][1];
        change.noderev_id = svn_fs_parse_id(bogus_changes[i][2],
                                            strlen(bogus_changes[i][2]),
                                            subpool);
        change.kind = string_to_kind(bogus_changes[i][3]);
        change.text_mod = bogus_changes[i][4] ? 1 : 0;
        change.prop_mod = bogus_changes[i][5] ? 1 : 0;

        /* Set up transaction baton. */
        args.fs = fs;
        args.key = "x";
        args.change = &change;

        /* Write new changes to the changes table. */
        SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_changes_add, &args,
                                       TRUE, subpool));
      }

    /* Now read 'em back, looking for an error. */
    args.fs = fs;
    args.key = "x";
    err = svn_fs_base__retry_txn(args.fs, txn_body_changes_fetch, &args,
                                 TRUE, subpool);
    if (!err)
      {
        return svn_error_create(SVN_ERR_TEST_FAILED, 0,
                                "Expected SVN_ERR_FS_CORRUPT, got no error.");
      }
    else if (err->apr_err != SVN_ERR_FS_CORRUPT)
      {
        return svn_error_create(SVN_ERR_TEST_FAILED, err,
                                "Expected SVN_ERR_FS_CORRUPT, got a different error.");
      }
    else
      {
        svn_error_clear(err);
      }

    /* Post-test cleanup. */
    SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_changes_delete, &args,
                                   TRUE, subpool));
  }

  /* Test changes bogus because there's an add on a path that's got
     previous non-delete changes on it. */
  svn_pool_clear(subpool);
  {
    static const char *bogus_changes[][6]
         /* KEY   PATH   NODEREVID  KIND       TEXT PROP */
      = { { "x",  "/foo",  "1.0.0",  "modify", "1",  0  },
          { "x",  "/foo",  "1.0.0",  "add",    "1",  0  } };
    int num_changes = sizeof(bogus_changes) / sizeof(const char *) / 6;
    struct changes_args args;
    int i;

    for (i = 0; i < num_changes; i++)
      {
        change_t change;

        /* Set up the current change item. */
        change.path = bogus_changes[i][1];
        change.noderev_id = svn_fs_parse_id(bogus_changes[i][2],
                                            strlen(bogus_changes[i][2]),
                                            subpool);
        change.kind = string_to_kind(bogus_changes[i][3]);
        change.text_mod = bogus_changes[i][4] ? 1 : 0;
        change.prop_mod = bogus_changes[i][5] ? 1 : 0;

        /* Set up transaction baton. */
        args.fs = fs;
        args.key = "x";
        args.change = &change;

        /* Write new changes to the changes table. */
        SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_changes_add, &args,
                                       TRUE, subpool));
      }

    /* Now read 'em back, looking for an error. */
    args.fs = fs;
    args.key = "x";
    err = svn_fs_base__retry_txn(args.fs, txn_body_changes_fetch, &args,
                                 TRUE, subpool);
    if (!err)
      {
        return svn_error_create(SVN_ERR_TEST_FAILED, 0,
                                "Expected SVN_ERR_FS_CORRUPT, got no error.");
      }
    else if (err->apr_err != SVN_ERR_FS_CORRUPT)
      {
        return svn_error_create(SVN_ERR_TEST_FAILED, err,
                                "Expected SVN_ERR_FS_CORRUPT, got a different error.");
      }
    else
      {
        svn_error_clear(err);
      }

    /* Post-test cleanup. */
    SVN_ERR(svn_fs_base__retry_txn(args.fs, txn_body_changes_delete, &args,
                                   TRUE, subpool));
  }

  return SVN_NO_ERROR;
}



/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_OPTS_PASS(changes_add,
                       "add changes to the changes table"),
    SVN_TEST_OPTS_PASS(changes_fetch_raw,
                       "fetch raw changes from the changes table"),
    SVN_TEST_OPTS_PASS(changes_delete,
                       "delete changes from the changes table"),
    SVN_TEST_OPTS_PASS(changes_fetch,
                       "fetch compressed changes from the changes table"),
    SVN_TEST_OPTS_PASS(changes_fetch_ordering,
                       "verify ordered-ness of fetched compressed changes"),
    SVN_TEST_OPTS_PASS(changes_bad_sequences,
                       "verify that bad change sequences raise errors"),
    SVN_TEST_NULL
  };
