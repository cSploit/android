/* fs-test.c --- tests for the filesystem
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
#include <string.h>
#include <apr_pools.h>
#include <assert.h>

#include "../svn_test.h"

#include "svn_pools.h"
#include "svn_time.h"
#include "svn_string.h"
#include "svn_fs.h"
#include "svn_checksum.h"
#include "svn_mergeinfo.h"
#include "svn_props.h"

#include "private/svn_fs_private.h"

#include "../svn_test_fs.h"

#include "../../libsvn_delta/delta.h"

#define SET_STR(ps, s) ((ps)->data = (s), (ps)->len = strlen(s))


/*-----------------------------------------------------------------*/

/** The actual fs-tests called by `make check` **/

/* Helper:  commit TXN, expecting either success or failure:
 *
 * If EXPECTED_CONFLICT is null, then the commit is expected to
 * succeed.  If it does succeed, set *NEW_REV to the new revision;
 * else return error.
 *
 * If EXPECTED_CONFLICT is non-null, it is either the empty string or
 * the expected path of the conflict.  If it is the empty string, any
 * conflict is acceptable.  If it is a non-empty string, the commit
 * must fail due to conflict, and the conflict path must match
 * EXPECTED_CONFLICT.  If they don't match, return error.
 *
 * If a conflict is expected but the commit succeeds anyway, return
 * error.  If the commit fails but does not provide an error, return
 * error.
 */
static svn_error_t *
test_commit_txn(svn_revnum_t *new_rev,
                svn_fs_txn_t *txn,
                const char *expected_conflict,
                apr_pool_t *pool)
{
  const char *conflict;
  svn_error_t *err;

  err = svn_fs_commit_txn(&conflict, new_rev, txn, pool);

  if (err && (err->apr_err == SVN_ERR_FS_CONFLICT))
    {
      svn_error_clear(err);
      if (! expected_conflict)
        {
          return svn_error_createf
            (SVN_ERR_FS_CONFLICT, NULL,
             "commit conflicted at '%s', but no conflict expected",
             conflict ? conflict : "(missing conflict info!)");
        }
      else if (conflict == NULL)
        {
          return svn_error_createf
            (SVN_ERR_FS_CONFLICT, NULL,
             "commit conflicted as expected, "
             "but no conflict path was returned ('%s' expected)",
             expected_conflict);
        }
      else if ((strcmp(expected_conflict, "") != 0)
               && (strcmp(conflict, expected_conflict) != 0))
        {
          return svn_error_createf
            (SVN_ERR_FS_CONFLICT, NULL,
             "commit conflicted at '%s', but expected conflict at '%s')",
             conflict, expected_conflict);
        }

      /* The svn_fs_commit_txn() API promises to set *NEW_REV to an
         invalid revision number in the case of a conflict.  */
      if (SVN_IS_VALID_REVNUM(*new_rev))
        {
          return svn_error_createf
            (SVN_ERR_FS_GENERAL, NULL,
             "conflicting commit returned valid new revision");
        }
    }
  else if (err)   /* commit may have succeeded, but always report an error */
    {
      if (SVN_IS_VALID_REVNUM(*new_rev))
        return svn_error_quick_wrap
          (err, "commit succeeded but something else failed");
      else
        return svn_error_quick_wrap
          (err, "commit failed due to something other than a conflict");
    }
  else            /* err == NULL, commit should have succeeded */
    {
      if (! SVN_IS_VALID_REVNUM(*new_rev))
        {
          return svn_error_create
            (SVN_ERR_FS_GENERAL, NULL,
             "commit failed but no error was returned");
        }

      if (expected_conflict)
        {
          return svn_error_createf
            (SVN_ERR_FS_GENERAL, NULL,
             "commit succeeded that was expected to fail at '%s'",
             expected_conflict);
        }
    }

  return SVN_NO_ERROR;
}



/* Begin a txn, check its name, then close it */
static svn_error_t *
trivial_transaction(const svn_test_opts_t *opts,
                    apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  const char *txn_name;
  int is_invalid_char[256];
  int i;
  const char *p;

  SVN_ERR(svn_test__create_fs(&fs, "test-repo-trivial-txn",
                              opts, pool));

  /* Begin a new transaction that is based on revision 0.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));

  /* Test that the txn name is non-null. */
  SVN_ERR(svn_fs_txn_name(&txn_name, txn, pool));

  if (! txn_name)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "Got a NULL txn name.");

  /* Test that the txn name contains only valid characters.  See
     svn_fs.h for the list of valid characters. */
  for (i = 0; i < sizeof(is_invalid_char)/sizeof(*is_invalid_char); ++i)
    is_invalid_char[i] = 1;
  for (i = '0'; i <= '9'; ++i)
    is_invalid_char[i] = 0;
  for (i = 'a'; i <= 'z'; ++i)
    is_invalid_char[i] = 0;
  for (i = 'A'; i <= 'Z'; ++i)
    is_invalid_char[i] = 0;
  for (p = "-."; *p; ++p)
    is_invalid_char[(unsigned char) *p] = 0;

  for (p = txn_name; *p; ++p)
    {
      if (is_invalid_char[(unsigned char) *p])
        return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                                 "The txn name '%s' contains an illegal '%c' "
                                 "character", txn_name, *p);
    }

  return SVN_NO_ERROR;
}



/* Open an existing transaction by name. */
static svn_error_t *
reopen_trivial_transaction(const svn_test_opts_t *opts,
                           apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  const char *txn_name;
  apr_pool_t *subpool = svn_pool_create(pool);

  SVN_ERR(svn_test__create_fs(&fs, "test-repo-reopen-trivial-txn",
                              opts, pool));

  /* Begin a new transaction that is based on revision 0.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));

  /* Don't use the subpool, txn_name must persist beyond the current txn */
  SVN_ERR(svn_fs_txn_name(&txn_name, txn, pool));

  /* Close the transaction. */
  svn_pool_clear(subpool);

  /* Reopen the transaction by name */
  SVN_ERR(svn_fs_open_txn(&txn, fs, txn_name, subpool));

  /* Close the transaction ... again. */
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}



/* Create a file! */
static svn_error_t *
create_file_transaction(const svn_test_opts_t *opts,
                        apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;

  SVN_ERR(svn_test__create_fs(&fs, "test-repo-create-file-txn",
                              opts, pool));

  /* Begin a new transaction that is based on revision 0.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));

  /* Get the txn root */
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create a new file in the root directory. */
  SVN_ERR(svn_fs_make_file(txn_root, "beer.txt", pool));

  return SVN_NO_ERROR;
}


/* Make sure we get txn lists correctly. */
static svn_error_t *
verify_txn_list(const svn_test_opts_t *opts,
                apr_pool_t *pool)
{
  svn_fs_t *fs;
  apr_pool_t *subpool;
  svn_fs_txn_t *txn1, *txn2;
  const char *name1, *name2;
  apr_array_header_t *txn_list;

  SVN_ERR(svn_test__create_fs(&fs, "test-repo-verify-txn-list",
                              opts, pool));

  /* Begin a new transaction, get its name (in the top pool), close it.  */
  subpool = svn_pool_create(pool);
  SVN_ERR(svn_fs_begin_txn(&txn1, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_name(&name1, txn1, pool));
  svn_pool_destroy(subpool);

  /* Begin *another* transaction, get its name (in the top pool), close it.  */
  subpool = svn_pool_create(pool);
  SVN_ERR(svn_fs_begin_txn(&txn2, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_name(&name2, txn2, pool));
  svn_pool_destroy(subpool);

  /* Get the list of active transactions from the fs. */
  SVN_ERR(svn_fs_list_transactions(&txn_list, fs, pool));

  /* Check the list. It should have *exactly* two entries. */
  if (txn_list->nelts != 2)
    goto all_bad;

  /* We should be able to find our 2 txn names in the list, in some
     order. */
  if ((! strcmp(name1, APR_ARRAY_IDX(txn_list, 0, const char *)))
      && (! strcmp(name2, APR_ARRAY_IDX(txn_list, 1, const char *))))
    goto all_good;

  else if ((! strcmp(name2, APR_ARRAY_IDX(txn_list, 0, const char *)))
           && (! strcmp(name1, APR_ARRAY_IDX(txn_list, 1, const char *))))
    goto all_good;

 all_bad:

  return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                          "Got a bogus txn list.");
 all_good:

  return SVN_NO_ERROR;
}


/* Generate N consecutive transactions, then abort them all.  Return
   the list of transaction names. */
static svn_error_t *
txn_names_are_not_reused_helper1(apr_hash_t **txn_names,
                                 svn_fs_t *fs,
                                 apr_pool_t *pool)
{
  apr_hash_index_t *hi;
  const int N = 10;
  int i;

  *txn_names = apr_hash_make(pool);

  /* Create the transactions and store in a hash table the transaction
     name as the key and the svn_fs_txn_t * as the value. */
  for (i = 0; i < N; ++i)
    {
      svn_fs_txn_t *txn;
      const char *name;
      SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
      SVN_ERR(svn_fs_txn_name(&name, txn, pool));
      if (apr_hash_get(*txn_names, name, APR_HASH_KEY_STRING) != NULL)
        return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                                 "beginning a new transaction used an "
                                 "existing transaction name '%s'",
                                 name);
      apr_hash_set(*txn_names, name, APR_HASH_KEY_STRING, txn);
    }

  i = 0;
  for (hi = apr_hash_first(pool, *txn_names); hi; hi = apr_hash_next(hi))
    {
      void *val;
      apr_hash_this(hi, NULL, NULL, &val);
      SVN_ERR(svn_fs_abort_txn((svn_fs_txn_t *)val, pool));
      ++i;
    }

  if (i != N)
    return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                             "created %d transactions, but only aborted %d",
                             N, i);

  return SVN_NO_ERROR;
}

/* Compare two hash tables and ensure that no keys in the first hash
   table appear in the second hash table. */
static svn_error_t *
txn_names_are_not_reused_helper2(apr_hash_t *ht1,
                                 apr_hash_t *ht2,
                                 apr_pool_t *pool)
{
  apr_hash_index_t *hi;

  for (hi = apr_hash_first(pool, ht1); hi; hi = apr_hash_next(hi))
    {
      const void *key;
      const char *key_string;
      apr_hash_this(hi, &key, NULL, NULL);
      key_string = key;
      if (apr_hash_get(ht2, key, APR_HASH_KEY_STRING) != NULL)
        return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                                 "the transaction name '%s' was reused",
                                 key_string);
    }

  return SVN_NO_ERROR;
}

/* Make sure that transaction names are not reused. */
static svn_error_t *
txn_names_are_not_reused(const svn_test_opts_t *opts,
                         apr_pool_t *pool)
{
  svn_fs_t *fs;
  apr_pool_t *subpool;
  apr_hash_t *txn_names1, *txn_names2;

  /* Bail (with success) on known-untestable scenarios */
  if ((strcmp(opts->fs_type, "fsfs") == 0)
      && (opts->server_minor_version && (opts->server_minor_version < 5)))
    return SVN_NO_ERROR;

  SVN_ERR(svn_test__create_fs(&fs, "test-repo-txn-names-are-not-reused",
                              opts, pool));

  subpool = svn_pool_create(pool);

  /* Create N transactions, abort them all, and collect the generated
     transaction names.  Do this twice. */
  SVN_ERR(txn_names_are_not_reused_helper1(&txn_names1, fs, subpool));
  SVN_ERR(txn_names_are_not_reused_helper1(&txn_names2, fs, subpool));

  /* Check that no transaction names appear in both hash tables. */
  SVN_ERR(txn_names_are_not_reused_helper2(txn_names1, txn_names2, subpool));
  SVN_ERR(txn_names_are_not_reused_helper2(txn_names2, txn_names1, subpool));

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}



/* Test writing & reading a file's contents. */
static svn_error_t *
write_and_read_file(const svn_test_opts_t *opts,
                    apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  svn_stream_t *rstream;
  svn_stringbuf_t *rstring;
  svn_stringbuf_t *wstring;

  wstring = svn_stringbuf_create("Wicki wild, wicki wicki wild.", pool);
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-read-and-write-file",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Add an empty file. */
  SVN_ERR(svn_fs_make_file(txn_root, "beer.txt", pool));

  /* And write some data into this file. */
  SVN_ERR(svn_test__set_file_contents(txn_root, "beer.txt",
                                      wstring->data, pool));

  /* Now let's read the data back from the file. */
  SVN_ERR(svn_fs_file_contents(&rstream, txn_root, "beer.txt", pool));
  SVN_ERR(svn_test__stream_to_string(&rstring, rstream, pool));

  /* Compare what was read to what was written. */
  if (! svn_stringbuf_compare(rstring, wstring))
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "data read != data written.");

  return SVN_NO_ERROR;
}



/* Create a file, a directory, and a file in that directory! */
static svn_error_t *
create_mini_tree_transaction(const svn_test_opts_t *opts,
                             apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;

  SVN_ERR(svn_test__create_fs(&fs, "test-repo-create-mini-tree-txn",
                              opts, pool));

  /* Begin a new transaction that is based on revision 0.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));

  /* Get the txn root */
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create a new file in the root directory. */
  SVN_ERR(svn_fs_make_file(txn_root, "wine.txt", pool));

  /* Create a new directory in the root directory. */
  SVN_ERR(svn_fs_make_dir(txn_root, "keg", pool));

  /* Now, create a file in our new directory. */
  SVN_ERR(svn_fs_make_file(txn_root, "keg/beer.txt", pool));

  return SVN_NO_ERROR;
}


/* Create a file, a directory, and a file in that directory! */
static svn_error_t *
create_greek_tree_transaction(const svn_test_opts_t *opts,
                              apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-create-greek-tree-txn",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create and verify the greek tree. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));

  return SVN_NO_ERROR;
}


/* Verify that entry KEY is present in ENTRIES, and that its value is
   an svn_fs_dirent_t whose name and id are not null. */
static svn_error_t *
verify_entry(apr_hash_t *entries, const char *key)
{
  svn_fs_dirent_t *ent = apr_hash_get(entries, key,
                                      APR_HASH_KEY_STRING);

  if (ent == NULL)
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       "didn't find dir entry for \"%s\"", key);

  if ((ent->name == NULL) && (ent->id == NULL))
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       "dir entry for \"%s\" has null name and null id", key);

  if (ent->name == NULL)
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       "dir entry for \"%s\" has null name", key);

  if (ent->id == NULL)
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       "dir entry for \"%s\" has null id", key);

  if (strcmp(ent->name, key) != 0)
     return svn_error_createf
     (SVN_ERR_FS_GENERAL, NULL,
      "dir entry for \"%s\" contains wrong name (\"%s\")", key, ent->name);

  return SVN_NO_ERROR;
}


static svn_error_t *
list_directory(const svn_test_opts_t *opts,
               apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  apr_hash_t *entries;

  SVN_ERR(svn_test__create_fs(&fs, "test-repo-list-dir",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* We create this tree
   *
   *         /q
   *         /A/x
   *         /A/y
   *         /A/z
   *         /B/m
   *         /B/n
   *         /B/o
   *
   * then list dir A.  It should have 3 files: "x", "y", and "z", no
   * more, no less.
   */

  /* Create the tree. */
  SVN_ERR(svn_fs_make_file(txn_root, "q", pool));
  SVN_ERR(svn_fs_make_dir(txn_root, "A", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "A/x", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "A/y", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "A/z", pool));
  SVN_ERR(svn_fs_make_dir(txn_root, "B", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "B/m", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "B/n", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "B/o", pool));

  /* Get A's entries. */
  SVN_ERR(svn_fs_dir_entries(&entries, txn_root, "A", pool));

  /* Make sure exactly the right set of entries is present. */
  if (apr_hash_count(entries) != 3)
    {
      return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                              "unexpected number of entries in dir");
    }
  else
    {
      SVN_ERR(verify_entry(entries, "x"));
      SVN_ERR(verify_entry(entries, "y"));
      SVN_ERR(verify_entry(entries, "z"));
    }

  return SVN_NO_ERROR;
}


/* If EXPR raises SVN_ERR_FS_PROP_BASEVALUE_MISMATCH, continue; else, fail
 * the test. */
#define FAILS_WITH_BOV(expr) \
  do { \
      svn_error_t *__err = (expr); \
      if (!__err || __err->apr_err != SVN_ERR_FS_PROP_BASEVALUE_MISMATCH) \
        return svn_error_create(SVN_ERR_TEST_FAILED, __err, \
                                "svn_fs_change_rev_prop2() failed to " \
                                "detect unexpected old value"); \
      else \
        svn_error_clear(__err); \
  } while (0)

static svn_error_t *
revision_props(const svn_test_opts_t *opts,
               apr_pool_t *pool)
{
  svn_fs_t *fs;
  apr_hash_t *proplist;
  svn_string_t *value;
  int i;
  svn_string_t s1;

  const char *initial_props[4][2] = {
    { "color", "red" },
    { "size", "XXL" },
    { "favorite saturday morning cartoon", "looney tunes" },
    { "auto", "Green 1997 Saturn SL1" }
    };

  const char *final_props[4][2] = {
    { "color", "violet" },
    { "flower", "violet" },
    { "favorite saturday morning cartoon", "looney tunes" },
    { "auto", "Red 2000 Chevrolet Blazer" }
    };

  /* Open the fs */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-rev-props",
                              opts, pool));

  /* Set some properties on the revision. */
  for (i = 0; i < 4; i++)
    {
      SET_STR(&s1, initial_props[i][1]);
      SVN_ERR(svn_fs_change_rev_prop(fs, 0, initial_props[i][0], &s1, pool));
    }

  /* Change some of the above properties. */
  SET_STR(&s1, "violet");
  SVN_ERR(svn_fs_change_rev_prop(fs, 0, "color", &s1, pool));

  SET_STR(&s1, "Red 2000 Chevrolet Blazer");
  SVN_ERR(svn_fs_change_rev_prop(fs, 0, "auto", &s1, pool));

  /* Remove a property altogether */
  SVN_ERR(svn_fs_change_rev_prop(fs, 0, "size", NULL, pool));

  /* Copy a property's value into a new property. */
  SVN_ERR(svn_fs_revision_prop(&value, fs, 0, "color", pool));

  s1.data = value->data;
  s1.len = value->len;
  SVN_ERR(svn_fs_change_rev_prop(fs, 0, "flower", &s1, pool));

  /* Test svn_fs_change_rev_prop2().  If the whole block goes through, then
   * it is a no-op (it undoes all changes it makes). */
    {
      const svn_string_t s2 = { "wrong value", 11 };
      const svn_string_t *s2_p = &s2;
      const svn_string_t *s1_p = &s1;
      const svn_string_t *unset = NULL;
      const svn_string_t *s1_dup;

      /* Value of "flower" is 's1'. */

      FAILS_WITH_BOV(svn_fs_change_rev_prop2(fs, 0, "flower", &s2_p, s1_p, pool));
      s1_dup = svn_string_dup(&s1, pool);
      SVN_ERR(svn_fs_change_rev_prop2(fs, 0, "flower", &s1_dup, s2_p, pool));

      /* Value of "flower" is 's2'. */

      FAILS_WITH_BOV(svn_fs_change_rev_prop2(fs, 0, "flower", &s1_p, NULL, pool));
      SVN_ERR(svn_fs_change_rev_prop2(fs, 0, "flower", &s2_p, NULL, pool));

      /* Value of "flower" is <not set>. */

      FAILS_WITH_BOV(svn_fs_change_rev_prop2(fs, 0, "flower", &s2_p, s1_p, pool));
      SVN_ERR(svn_fs_change_rev_prop2(fs, 0, "flower", &unset, s1_p, pool));

      /* Value of "flower" is 's1'. */
    }

  /* Obtain a list of all current properties, and make sure it matches
     the expected values. */
  SVN_ERR(svn_fs_revision_proplist(&proplist, fs, 0, pool));
  {
    svn_string_t *prop_value;

    if (apr_hash_count(proplist) < 4 )
      return svn_error_createf
        (SVN_ERR_FS_GENERAL, NULL,
         "too few revision properties found");

    /* Loop through our list of expected revision property name/value
       pairs. */
    for (i = 0; i < 4; i++)
      {
        /* For each expected property: */

        /* Step 1.  Find it by name in the hash of all rev. props
           returned to us by svn_fs_revision_proplist.  If it can't be
           found, return an error. */
        prop_value = apr_hash_get(proplist,
                                  final_props[i][0],
                                  APR_HASH_KEY_STRING);
        if (! prop_value)
          return svn_error_createf
            (SVN_ERR_FS_GENERAL, NULL,
             "unable to find expected revision property");

        /* Step 2.  Make sure the value associated with it is the same
           as what was expected, else return an error. */
        if (strcmp(prop_value->data, final_props[i][1]))
          return svn_error_createf
            (SVN_ERR_FS_GENERAL, NULL,
             "revision property had an unexpected value");
      }
  }

  return SVN_NO_ERROR;
}


static svn_error_t *
transaction_props(const svn_test_opts_t *opts,
                  apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  apr_hash_t *proplist;
  svn_string_t *value;
  svn_revnum_t after_rev;
  int i;
  svn_string_t s1;

  const char *initial_props[4][2] = {
    { "color", "red" },
    { "size", "XXL" },
    { "favorite saturday morning cartoon", "looney tunes" },
    { "auto", "Green 1997 Saturn SL1" }
    };

  const char *final_props[5][2] = {
    { "color", "violet" },
    { "flower", "violet" },
    { "favorite saturday morning cartoon", "looney tunes" },
    { "auto", "Red 2000 Chevrolet Blazer" },
    { SVN_PROP_REVISION_DATE, "<some datestamp value>" }
    };

  /* Open the fs */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-txn-props",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));

  /* Set some properties on the revision. */
  for (i = 0; i < 4; i++)
    {
      SET_STR(&s1, initial_props[i][1]);
      SVN_ERR(svn_fs_change_txn_prop(txn, initial_props[i][0], &s1, pool));
    }

  /* Change some of the above properties. */
  SET_STR(&s1, "violet");
  SVN_ERR(svn_fs_change_txn_prop(txn, "color", &s1, pool));

  SET_STR(&s1, "Red 2000 Chevrolet Blazer");
  SVN_ERR(svn_fs_change_txn_prop(txn, "auto", &s1, pool));

  /* Remove a property altogether */
  SVN_ERR(svn_fs_change_txn_prop(txn, "size", NULL, pool));

  /* Copy a property's value into a new property. */
  SVN_ERR(svn_fs_txn_prop(&value, txn, "color", pool));

  s1.data = value->data;
  s1.len = value->len;
  SVN_ERR(svn_fs_change_txn_prop(txn, "flower", &s1, pool));

  /* Obtain a list of all current properties, and make sure it matches
     the expected values. */
  SVN_ERR(svn_fs_txn_proplist(&proplist, txn, pool));
  {
    svn_string_t *prop_value;

    /* All transactions get a datestamp property at their inception,
       so we expect *5*, not 4 properties. */
    if (apr_hash_count(proplist) != 5 )
      return svn_error_createf
        (SVN_ERR_FS_GENERAL, NULL,
         "unexpected number of transaction properties were found");

    /* Loop through our list of expected revision property name/value
       pairs. */
    for (i = 0; i < 5; i++)
      {
        /* For each expected property: */

        /* Step 1.  Find it by name in the hash of all rev. props
           returned to us by svn_fs_revision_proplist.  If it can't be
           found, return an error. */
        prop_value = apr_hash_get(proplist,
                                  final_props[i][0],
                                  APR_HASH_KEY_STRING);
        if (! prop_value)
          return svn_error_createf
            (SVN_ERR_FS_GENERAL, NULL,
             "unable to find expected transaction property");

        /* Step 2.  Make sure the value associated with it is the same
           as what was expected, else return an error. */
        if (strcmp(final_props[i][0], SVN_PROP_REVISION_DATE))
          if (strcmp(prop_value->data, final_props[i][1]))
            return svn_error_createf
              (SVN_ERR_FS_GENERAL, NULL,
               "transaction property had an unexpected value");
      }
  }

  /* Commit the transaction. */
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
  if (after_rev != 1)
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       "committed transaction got wrong revision number");

  /* Obtain a list of all properties on the new revision, and make
     sure it matches the expected values.  If you're wondering, the
     expected values should be the exact same set of properties that
     existed on the transaction just prior to its being committed. */
  SVN_ERR(svn_fs_revision_proplist(&proplist, fs, after_rev, pool));
  {
    svn_string_t *prop_value;

    if (apr_hash_count(proplist) < 5 )
      return svn_error_createf
        (SVN_ERR_FS_GENERAL, NULL,
         "unexpected number of revision properties were found");

    /* Loop through our list of expected revision property name/value
       pairs. */
    for (i = 0; i < 5; i++)
      {
        /* For each expected property: */

        /* Step 1.  Find it by name in the hash of all rev. props
           returned to us by svn_fs_revision_proplist.  If it can't be
           found, return an error. */
        prop_value = apr_hash_get(proplist,
                                  final_props[i][0],
                                  APR_HASH_KEY_STRING);
        if (! prop_value)
          return svn_error_createf
            (SVN_ERR_FS_GENERAL, NULL,
             "unable to find expected revision property");

        /* Step 2.  Make sure the value associated with it is the same
           as what was expected, else return an error. */
        if (strcmp(final_props[i][0], SVN_PROP_REVISION_DATE))
          if (strcmp(prop_value->data, final_props[i][1]))
            return svn_error_createf
              (SVN_ERR_FS_GENERAL, NULL,
               "revision property had an unexpected value");
      }
  }

  return SVN_NO_ERROR;
}


static svn_error_t *
node_props(const svn_test_opts_t *opts,
           apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  apr_hash_t *proplist;
  svn_string_t *value;
  int i;
  svn_string_t s1;

  const char *initial_props[4][2] = {
    { "Best Rock Artist", "Creed" },
    { "Best Rap Artist", "Eminem" },
    { "Best Country Artist", "(null)" },
    { "Best Sound Designer", "Pluessman" }
    };

  const char *final_props[4][2] = {
    { "Best Rock Artist", "P.O.D." },
    { "Best Rap Artist", "Busta Rhymes" },
    { "Best Sound Designer", "Pluessman" },
    { "Biggest Cakewalk Fanatic", "Pluessman" }
    };

  /* Open the fs and transaction */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-node-props",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Make a node to put some properties into */
  SVN_ERR(svn_fs_make_file(txn_root, "music.txt", pool));

  /* Set some properties on the nodes. */
  for (i = 0; i < 4; i++)
    {
      SET_STR(&s1, initial_props[i][1]);
      SVN_ERR(svn_fs_change_node_prop
              (txn_root, "music.txt", initial_props[i][0], &s1, pool));
    }

  /* Change some of the above properties. */
  SET_STR(&s1, "P.O.D.");
  SVN_ERR(svn_fs_change_node_prop(txn_root, "music.txt", "Best Rock Artist",
                                  &s1, pool));

  SET_STR(&s1, "Busta Rhymes");
  SVN_ERR(svn_fs_change_node_prop(txn_root, "music.txt", "Best Rap Artist",
                                  &s1, pool));

  /* Remove a property altogether */
  SVN_ERR(svn_fs_change_node_prop(txn_root, "music.txt",
                                  "Best Country Artist", NULL, pool));

  /* Copy a property's value into a new property. */
  SVN_ERR(svn_fs_node_prop(&value, txn_root, "music.txt",
                           "Best Sound Designer", pool));

  s1.data = value->data;
  s1.len = value->len;
  SVN_ERR(svn_fs_change_node_prop(txn_root, "music.txt",
                                  "Biggest Cakewalk Fanatic", &s1, pool));

  /* Obtain a list of all current properties, and make sure it matches
     the expected values. */
  SVN_ERR(svn_fs_node_proplist(&proplist, txn_root, "music.txt", pool));
  {
    svn_string_t *prop_value;

    if (apr_hash_count(proplist) != 4 )
      return svn_error_createf
        (SVN_ERR_FS_GENERAL, NULL,
         "unexpected number of node properties were found");

    /* Loop through our list of expected node property name/value
       pairs. */
    for (i = 0; i < 4; i++)
      {
        /* For each expected property: */

        /* Step 1.  Find it by name in the hash of all node props
           returned to us by svn_fs_node_proplist.  If it can't be
           found, return an error. */
        prop_value = apr_hash_get(proplist,
                                  final_props[i][0],
                                  APR_HASH_KEY_STRING);
        if (! prop_value)
          return svn_error_createf
            (SVN_ERR_FS_GENERAL, NULL,
             "unable to find expected node property");

        /* Step 2.  Make sure the value associated with it is the same
           as what was expected, else return an error. */
        if (strcmp(prop_value->data, final_props[i][1]))
          return svn_error_createf
            (SVN_ERR_FS_GENERAL, NULL,
             "node property had an unexpected value");
      }
  }

  return SVN_NO_ERROR;
}



/* Set *PRESENT to true if entry NAME is present in directory PATH
   under ROOT, else set *PRESENT to false. */
static svn_error_t *
check_entry(svn_fs_root_t *root,
            const char *path,
            const char *name,
            svn_boolean_t *present,
            apr_pool_t *pool)
{
  apr_hash_t *entries;
  svn_fs_dirent_t *ent;

  SVN_ERR(svn_fs_dir_entries(&entries, root, path, pool));
  ent = apr_hash_get(entries, name, APR_HASH_KEY_STRING);

  if (ent)
    *present = TRUE;
  else
    *present = FALSE;

  return SVN_NO_ERROR;
}


/* Return an error if entry NAME is absent in directory PATH under ROOT. */
static svn_error_t *
check_entry_present(svn_fs_root_t *root, const char *path,
                    const char *name, apr_pool_t *pool)
{
  svn_boolean_t present;
  SVN_ERR(check_entry(root, path, name, &present, pool));

  if (! present)
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       "entry \"%s\" absent when it should be present", name);

  return SVN_NO_ERROR;
}


/* Return an error if entry NAME is present in directory PATH under ROOT. */
static svn_error_t *
check_entry_absent(svn_fs_root_t *root, const char *path,
                   const char *name, apr_pool_t *pool)
{
  svn_boolean_t present;
  SVN_ERR(check_entry(root, path, name, &present, pool));

  if (present)
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       "entry \"%s\" present when it should be absent", name);

  return SVN_NO_ERROR;
}


/* Fetch the youngest revision from a repos. */
static svn_error_t *
fetch_youngest_rev(const svn_test_opts_t *opts,
                   apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  svn_revnum_t new_rev;
  svn_revnum_t youngest_rev, new_youngest_rev;

  SVN_ERR(svn_test__create_fs(&fs, "test-repo-youngest-rev",
                              opts, pool));

  /* Get youngest revision of brand spankin' new filesystem. */
  SVN_ERR(svn_fs_youngest_rev(&youngest_rev, fs, pool));

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));

  /* Commit it. */
  SVN_ERR(test_commit_txn(&new_rev, txn, NULL, pool));

  /* Get the new youngest revision. */
  SVN_ERR(svn_fs_youngest_rev(&new_youngest_rev, fs, pool));

  if (youngest_rev == new_rev)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "commit didn't bump up revision number");

  if (new_youngest_rev != new_rev)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "couldn't fetch youngest revision");

  return SVN_NO_ERROR;
}


/* Test committing against an empty repository.
   todo: also test committing against youngest? */
static svn_error_t *
basic_commit(const svn_test_opts_t *opts,
             apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *revision_root;
  svn_revnum_t before_rev, after_rev;
  const char *conflict;

  /* Prepare a filesystem. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-basic-commit",
                              opts, pool));

  /* Save the current youngest revision. */
  SVN_ERR(svn_fs_youngest_rev(&before_rev, fs, pool));

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Paranoidly check that the current youngest rev is unchanged. */
  SVN_ERR(svn_fs_youngest_rev(&after_rev, fs, pool));
  if (after_rev != before_rev)
    return svn_error_create
      (SVN_ERR_FS_GENERAL, NULL,
       "youngest revision changed unexpectedly");

  /* Create the greek tree. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));

  /* Commit it. */
  SVN_ERR(svn_fs_commit_txn(&conflict, &after_rev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(after_rev));

  /* Make sure it's a different revision than before. */
  if (after_rev == before_rev)
    return svn_error_create
      (SVN_ERR_FS_GENERAL, NULL,
       "youngest revision failed to change");

  /* Get root of the revision */
  SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));

  /* Check the tree. */
  SVN_ERR(svn_test__check_greek_tree(revision_root, pool));

  return SVN_NO_ERROR;
}



static svn_error_t *
test_tree_node_validation(const svn_test_opts_t *opts,
                          apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *revision_root;
  svn_revnum_t after_rev;
  const char *conflict;
  apr_pool_t *subpool;

  /* Prepare a filesystem. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-validate-tree-entries",
                              opts, pool));

  /* In a txn, create the greek tree. */
  subpool = svn_pool_create(pool);
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "iota",        "This is the file 'iota'.\n" },
      { "A",           0 },
      { "A/mu",        "This is the file 'mu'.\n" },
      { "A/B",         0 },
      { "A/B/lambda",  "This is the file 'lambda'.\n" },
      { "A/B/E",       0 },
      { "A/B/E/alpha", "This is the file 'alpha'.\n" },
      { "A/B/E/beta",  "This is the file 'beta'.\n" },
      { "A/B/F",       0 },
      { "A/C",         0 },
      { "A/D",         0 },
      { "A/D/gamma",   "This is the file 'gamma'.\n" },
      { "A/D/G",       0 },
      { "A/D/G/pi",    "This is the file 'pi'.\n" },
      { "A/D/G/rho",   "This is the file 'rho'.\n" },
      { "A/D/G/tau",   "This is the file 'tau'.\n" },
      { "A/D/H",       0 },
      { "A/D/H/chi",   "This is the file 'chi'.\n" },
      { "A/D/H/psi",   "This is the file 'psi'.\n" },
      { "A/D/H/omega", "This is the file 'omega'.\n" }
    };
    SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
    SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));

    /* Carefully validate that tree in the transaction. */
    SVN_ERR(svn_test__validate_tree(txn_root, expected_entries, 20,
                                    subpool));

    /* Go ahead and commit the tree, and destroy the txn object.  */
    SVN_ERR(svn_fs_commit_txn(&conflict, &after_rev, txn, subpool));
    SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(after_rev));

    /* Carefully validate that tree in the new revision, now. */
    SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, subpool));
    SVN_ERR(svn_test__validate_tree(revision_root, expected_entries, 20,
                                    subpool));
  }
  svn_pool_destroy(subpool);

  /* In a new txn, modify the greek tree. */
  subpool = svn_pool_create(pool);
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "iota",          "This is a new version of 'iota'.\n" },
      { "A",             0 },
      { "A/B",           0 },
      { "A/B/lambda",    "This is the file 'lambda'.\n" },
      { "A/B/E",         0 },
      { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
      { "A/B/E/beta",    "This is the file 'beta'.\n" },
      { "A/B/F",         0 },
      { "A/C",           0 },
      { "A/C/kappa",     "This is the file 'kappa'.\n" },
      { "A/D",           0 },
      { "A/D/gamma",     "This is the file 'gamma'.\n" },
      { "A/D/H",         0 },
      { "A/D/H/chi",     "This is the file 'chi'.\n" },
      { "A/D/H/psi",     "This is the file 'psi'.\n" },
      { "A/D/H/omega",   "This is the file 'omega'.\n" },
      { "A/D/I",         0 },
      { "A/D/I/delta",   "This is the file 'delta'.\n" },
      { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
    };

    SVN_ERR(svn_fs_begin_txn(&txn, fs, after_rev, subpool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "iota", "This is a new version of 'iota'.\n",
             subpool));
    SVN_ERR(svn_fs_delete(txn_root, "A/mu", subpool));
    SVN_ERR(svn_fs_delete(txn_root, "A/D/G", subpool));
    SVN_ERR(svn_fs_make_dir(txn_root, "A/D/I", subpool));
    SVN_ERR(svn_fs_make_file(txn_root, "A/D/I/delta", subpool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "A/D/I/delta", "This is the file 'delta'.\n",
             subpool));
    SVN_ERR(svn_fs_make_file(txn_root, "A/D/I/epsilon", subpool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "A/D/I/epsilon", "This is the file 'epsilon'.\n",
             subpool));
    SVN_ERR(svn_fs_make_file(txn_root, "A/C/kappa", subpool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "A/C/kappa", "This is the file 'kappa'.\n",
             subpool));

    /* Carefully validate that tree in the transaction. */
    SVN_ERR(svn_test__validate_tree(txn_root, expected_entries, 19,
                                    subpool));

    /* Go ahead and commit the tree, and destroy the txn object.  */
    SVN_ERR(svn_fs_commit_txn(&conflict, &after_rev, txn, subpool));
    SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(after_rev));

    /* Carefully validate that tree in the new revision, now. */
    SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, subpool));
    SVN_ERR(svn_test__validate_tree(revision_root, expected_entries,
                                    19, subpool));
  }
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


/* Commit with merging (committing against non-youngest). */
static svn_error_t *
merging_commit(const svn_test_opts_t *opts,
               apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *revision_root;
  svn_revnum_t after_rev;
  svn_revnum_t revisions[24];
  apr_size_t i;
  svn_revnum_t revision_count;

  /* Prepare a filesystem. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-merging-commit",
                              opts, pool));

  /* Initialize our revision number stuffs. */
  for (i = 0;
       i < ((sizeof(revisions)) / (sizeof(svn_revnum_t)));
       i++)
    revisions[i] = SVN_INVALID_REVNUM;
  revision_count = 0;
  revisions[revision_count++] = 0; /* the brand spankin' new revision */

  /***********************************************************************/
  /* REVISION 0 */
  /***********************************************************************/

  /* In one txn, create and commit the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));

  /***********************************************************************/
  /* REVISION 1 */
  /***********************************************************************/
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "iota",        "This is the file 'iota'.\n" },
      { "A",           0 },
      { "A/mu",        "This is the file 'mu'.\n" },
      { "A/B",         0 },
      { "A/B/lambda",  "This is the file 'lambda'.\n" },
      { "A/B/E",       0 },
      { "A/B/E/alpha", "This is the file 'alpha'.\n" },
      { "A/B/E/beta",  "This is the file 'beta'.\n" },
      { "A/B/F",       0 },
      { "A/C",         0 },
      { "A/D",         0 },
      { "A/D/gamma",   "This is the file 'gamma'.\n" },
      { "A/D/G",       0 },
      { "A/D/G/pi",    "This is the file 'pi'.\n" },
      { "A/D/G/rho",   "This is the file 'rho'.\n" },
      { "A/D/G/tau",   "This is the file 'tau'.\n" },
      { "A/D/H",       0 },
      { "A/D/H/chi",   "This is the file 'chi'.\n" },
      { "A/D/H/psi",   "This is the file 'psi'.\n" },
      { "A/D/H/omega", "This is the file 'omega'.\n" }
    };
    SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
    SVN_ERR(svn_test__validate_tree(revision_root, expected_entries,
                                    20, pool));
  }
  revisions[revision_count++] = after_rev;

  /* Let's add a directory and some files to the tree, and delete
     'iota' */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[revision_count-1], pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_make_dir(txn_root, "A/D/I", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "A/D/I/delta", pool));
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "A/D/I/delta", "This is the file 'delta'.\n", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "A/D/I/epsilon", pool));
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "A/D/I/epsilon", "This is the file 'epsilon'.\n", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "A/C/kappa", pool));
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "A/C/kappa", "This is the file 'kappa'.\n", pool));
  SVN_ERR(svn_fs_delete(txn_root, "iota", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));

  /***********************************************************************/
  /* REVISION 2 */
  /***********************************************************************/
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "A",             0 },
      { "A/mu",          "This is the file 'mu'.\n" },
      { "A/B",           0 },
      { "A/B/lambda",    "This is the file 'lambda'.\n" },
      { "A/B/E",         0 },
      { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
      { "A/B/E/beta",    "This is the file 'beta'.\n" },
      { "A/B/F",         0 },
      { "A/C",           0 },
      { "A/C/kappa",     "This is the file 'kappa'.\n" },
      { "A/D",           0 },
      { "A/D/gamma",     "This is the file 'gamma'.\n" },
      { "A/D/G",         0 },
      { "A/D/G/pi",      "This is the file 'pi'.\n" },
      { "A/D/G/rho",     "This is the file 'rho'.\n" },
      { "A/D/G/tau",     "This is the file 'tau'.\n" },
      { "A/D/H",         0 },
      { "A/D/H/chi",     "This is the file 'chi'.\n" },
      { "A/D/H/psi",     "This is the file 'psi'.\n" },
      { "A/D/H/omega",   "This is the file 'omega'.\n" },
      { "A/D/I",         0 },
      { "A/D/I/delta",   "This is the file 'delta'.\n" },
      { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
    };
    SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
    SVN_ERR(svn_test__validate_tree(revision_root, expected_entries,
                                    23, pool));
  }
  revisions[revision_count++] = after_rev;

  /* We don't think the A/D/H directory is pulling its weight...let's
     knock it off.  Oh, and let's re-add iota, too. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[revision_count-1], pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_delete(txn_root, "A/D/H", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "iota", pool));
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "iota", "This is the new file 'iota'.\n", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));

  /***********************************************************************/
  /* REVISION 3 */
  /***********************************************************************/
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "iota",          "This is the new file 'iota'.\n" },
      { "A",             0 },
      { "A/mu",          "This is the file 'mu'.\n" },
      { "A/B",           0 },
      { "A/B/lambda",    "This is the file 'lambda'.\n" },
      { "A/B/E",         0 },
      { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
      { "A/B/E/beta",    "This is the file 'beta'.\n" },
      { "A/B/F",         0 },
      { "A/C",           0 },
      { "A/C/kappa",     "This is the file 'kappa'.\n" },
      { "A/D",           0 },
      { "A/D/gamma",     "This is the file 'gamma'.\n" },
      { "A/D/G",         0 },
      { "A/D/G/pi",      "This is the file 'pi'.\n" },
      { "A/D/G/rho",     "This is the file 'rho'.\n" },
      { "A/D/G/tau",     "This is the file 'tau'.\n" },
      { "A/D/I",         0 },
      { "A/D/I/delta",   "This is the file 'delta'.\n" },
      { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
    };
    SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
    SVN_ERR(svn_test__validate_tree(revision_root, expected_entries,
                                    20, pool));
  }
  revisions[revision_count++] = after_rev;

  /* Delete iota (yet again). */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[revision_count-1], pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_delete(txn_root, "iota", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));

  /***********************************************************************/
  /* REVISION 4 */
  /***********************************************************************/
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "A",             0 },
      { "A/mu",          "This is the file 'mu'.\n" },
      { "A/B",           0 },
      { "A/B/lambda",    "This is the file 'lambda'.\n" },
      { "A/B/E",         0 },
      { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
      { "A/B/E/beta",    "This is the file 'beta'.\n" },
      { "A/B/F",         0 },
      { "A/C",           0 },
      { "A/C/kappa",     "This is the file 'kappa'.\n" },
      { "A/D",           0 },
      { "A/D/gamma",     "This is the file 'gamma'.\n" },
      { "A/D/G",         0 },
      { "A/D/G/pi",      "This is the file 'pi'.\n" },
      { "A/D/G/rho",     "This is the file 'rho'.\n" },
      { "A/D/G/tau",     "This is the file 'tau'.\n" },
      { "A/D/I",         0 },
      { "A/D/I/delta",   "This is the file 'delta'.\n" },
      { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
    };
    SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
    SVN_ERR(svn_test__validate_tree(revision_root, expected_entries,
                                    19, pool));
  }
  revisions[revision_count++] = after_rev;

  /***********************************************************************/
  /* GIVEN:  A and B, with common ancestor ANCESTOR, where A and B
     directories, and E, an entry in either A, B, or ANCESTOR.

     For every E, the following cases exist:
      - E exists in neither ANCESTOR nor A.
      - E doesn't exist in ANCESTOR, and has been added to A.
      - E exists in ANCESTOR, but has been deleted from A.
      - E exists in both ANCESTOR and A ...
        - but refers to different node revisions.
        - and refers to the same node revision.

     The same set of possible relationships with ANCESTOR holds for B,
     so there are thirty-six combinations.  The matrix is symmetrical
     with A and B reversed, so we only have to describe one triangular
     half, including the diagonal --- 21 combinations.

     Our goal here is to test all the possible scenarios that can
     occur given the above boolean logic table, and to make sure that
     the results we get are as expected.

     The test cases below have the following features:

     - They run straight through the scenarios as described in the
       `structure' document at this time.

     - In each case, a txn is begun based on some revision (ANCESTOR),
       is modified into a new tree (B), and then is attempted to be
       committed (which happens against the head of the tree, A).

     - If the commit is successful (and is *expected* to be such),
       that new revision (which exists now as a result of the
       successful commit) is thoroughly tested for accuracy of tree
       entries, and in the case of files, for their contents.  It is
       important to realize that these successful commits are
       advancing the head of the tree, and each one effective becomes
       the new `A' described in further test cases.
  */
  /***********************************************************************/

  /* (6) E exists in neither ANCESTOR nor A. */
  {
    /* (1) E exists in neither ANCESTOR nor B.  Can't occur, by
       assumption that E exists in either A, B, or ancestor. */

    /* (1) E has been added to B.  Add E in the merged result. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[0], pool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
    SVN_ERR(svn_fs_make_file(txn_root, "theta", pool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "theta", "This is the file 'theta'.\n", pool));
    SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));

    /*********************************************************************/
    /* REVISION 5 */
    /*********************************************************************/
    {
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "theta",         "This is the file 'theta'.\n" },
        { "A",             0 },
        { "A/mu",          "This is the file 'mu'.\n" },
        { "A/B",           0 },
        { "A/B/lambda",    "This is the file 'lambda'.\n" },
        { "A/B/E",         0 },
        { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
        { "A/B/E/beta",    "This is the file 'beta'.\n" },
        { "A/B/F",         0 },
        { "A/C",           0 },
        { "A/C/kappa",     "This is the file 'kappa'.\n" },
        { "A/D",           0 },
        { "A/D/gamma",     "This is the file 'gamma'.\n" },
        { "A/D/G",         0 },
        { "A/D/G/pi",      "This is the file 'pi'.\n" },
        { "A/D/G/rho",     "This is the file 'rho'.\n" },
        { "A/D/G/tau",     "This is the file 'tau'.\n" },
        { "A/D/I",         0 },
        { "A/D/I/delta",   "This is the file 'delta'.\n" },
        { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
      };
      SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
      SVN_ERR(svn_test__validate_tree(revision_root,
                                      expected_entries,
                                      20, pool));
    }
    revisions[revision_count++] = after_rev;

    /* (1) E has been deleted from B.  Can't occur, by assumption that
       E doesn't exist in ANCESTOR. */

    /* (3) E exists in both ANCESTOR and B.  Can't occur, by
       assumption that E doesn't exist in ancestor. */
  }

  /* (5) E doesn't exist in ANCESTOR, and has been added to A. */
  {
    /* (1) E doesn't exist in ANCESTOR, and has been added to B.
       Conflict. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[4], pool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
    SVN_ERR(svn_fs_make_file(txn_root, "theta", pool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "theta", "This is another file 'theta'.\n", pool));
    SVN_ERR(test_commit_txn(&after_rev, txn, "/theta", pool));
    SVN_ERR(svn_fs_abort_txn(txn, pool));

    /* (1) E exists in ANCESTOR, but has been deleted from B.  Can't
       occur, by assumption that E doesn't exist in ANCESTOR. */

    /* (3) E exists in both ANCESTOR and B.  Can't occur, by assumption
       that E doesn't exist in ANCESTOR. */
  }

  /* (4) E exists in ANCESTOR, but has been deleted from A */
  {
    /* (1) E exists in ANCESTOR, but has been deleted from B.  If
       neither delete was a result of a rename, then omit E from the
       merged tree.  Otherwise, conflict. */
    /* ### cmpilato todo: the rename case isn't actually handled by
       merge yet, so we know we won't get a conflict here. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/D/H", pool));

    /* ### FIXME: It is at this point that our test stops being valid,
       ### hence its expected failure.  The following call will now
       ### conflict on /A/D/H, causing revision 6 *not* to be created,
       ### and the remainer of this test (which was written long ago)
       ### to suffer from a shift in the expected state and behavior
       ### of the filesystem as a result of this commit not happening.
    */

    SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
    /*********************************************************************/
    /* REVISION 6 */
    /*********************************************************************/
    {
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "theta",         "This is the file 'theta'.\n" },
        { "A",             0 },
        { "A/mu",          "This is the file 'mu'.\n" },
        { "A/B",           0 },
        { "A/B/lambda",    "This is the file 'lambda'.\n" },
        { "A/B/E",         0 },
        { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
        { "A/B/E/beta",    "This is the file 'beta'.\n" },
        { "A/B/F",         0 },
        { "A/C",           0 },
        { "A/C/kappa",     "This is the file 'kappa'.\n" },
        { "A/D",           0 },
        { "A/D/gamma",     "This is the file 'gamma'.\n" },
        { "A/D/G",         0 },
        { "A/D/G/pi",      "This is the file 'pi'.\n" },
        { "A/D/G/rho",     "This is the file 'rho'.\n" },
        { "A/D/G/tau",     "This is the file 'tau'.\n" },
        { "A/D/I",         0 },
        { "A/D/I/delta",   "This is the file 'delta'.\n" },
        { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
      };
      SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
      SVN_ERR(svn_test__validate_tree(revision_root,
                                      expected_entries,
                                      20, pool));
    }
    revisions[revision_count++] = after_rev;

    /* Try deleting a file F inside a subtree S where S does not exist
       in the most recent revision, but does exist in the ancestor
       tree.  This should conflict. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/D/H/omega", pool));
    SVN_ERR(test_commit_txn(&after_rev, txn, "/A/D/H", pool));
    SVN_ERR(svn_fs_abort_txn(txn, pool));

    /* E exists in both ANCESTOR and B ... */
    {
      /* (1) but refers to different nodes.  Conflict. */
      SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
      SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
      SVN_ERR(svn_fs_delete(txn_root, "A/D/H", pool));
      SVN_ERR(svn_fs_make_dir(txn_root, "A/D/H", pool));
      SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
      revisions[revision_count++] = after_rev;

      /*********************************************************************/
      /* REVISION 7 */
      /*********************************************************************/

      /* Re-remove A/D/H because future tests expect it to be absent. */
      {
        SVN_ERR(svn_fs_begin_txn
                (&txn, fs, revisions[revision_count - 1], pool));
        SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
        SVN_ERR(svn_fs_delete(txn_root, "A/D/H", pool));
        SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
        revisions[revision_count++] = after_rev;
      }

      /*********************************************************************/
      /* REVISION 8 (looks exactly like revision 6, we hope) */
      /*********************************************************************/

      /* (1) but refers to different revisions of the same node.
         Conflict. */
      SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
      SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
      SVN_ERR(svn_fs_make_file(txn_root, "A/D/H/zeta", pool));
      SVN_ERR(test_commit_txn(&after_rev, txn, "/A/D/H", pool));
      SVN_ERR(svn_fs_abort_txn(txn, pool));

      /* (1) and refers to the same node revision.  Omit E from the
         merged tree.  This is already tested in Merge-Test 3
         (A/D/H/chi, A/D/H/psi, e.g.), but we'll test it here again
         anyway.  A little paranoia never hurt anyone.  */
      SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
      SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
      SVN_ERR(svn_fs_delete(txn_root, "A/mu", pool)); /* unrelated change */
      SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));

      /*********************************************************************/
      /* REVISION 9 */
      /*********************************************************************/
      {
        static svn_test__tree_entry_t expected_entries[] = {
          /* path, contents (0 = dir) */
          { "theta",         "This is the file 'theta'.\n" },
          { "A",             0 },
          { "A/B",           0 },
          { "A/B/lambda",    "This is the file 'lambda'.\n" },
          { "A/B/E",         0 },
          { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
          { "A/B/E/beta",    "This is the file 'beta'.\n" },
          { "A/B/F",         0 },
          { "A/C",           0 },
          { "A/C/kappa",     "This is the file 'kappa'.\n" },
          { "A/D",           0 },
          { "A/D/gamma",     "This is the file 'gamma'.\n" },
          { "A/D/G",         0 },
          { "A/D/G/pi",      "This is the file 'pi'.\n" },
          { "A/D/G/rho",     "This is the file 'rho'.\n" },
          { "A/D/G/tau",     "This is the file 'tau'.\n" },
          { "A/D/I",         0 },
          { "A/D/I/delta",   "This is the file 'delta'.\n" },
          { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
        };
        SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
        SVN_ERR(svn_test__validate_tree(revision_root,
                                        expected_entries,
                                        19, pool));
      }
      revisions[revision_count++] = after_rev;
    }
  }

  /* Preparation for upcoming tests.
     We make a new head revision, with A/mu restored, but containing
     slightly different contents than its first incarnation. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[revision_count-1], pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_make_file(txn_root, "A/mu", pool));
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "A/mu", "A new file 'mu'.\n", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "A/D/G/xi", pool));
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "A/D/G/xi", "This is the file 'xi'.\n", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
  /*********************************************************************/
  /* REVISION 10 */
  /*********************************************************************/
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "theta",         "This is the file 'theta'.\n" },
      { "A",             0 },
      { "A/mu",          "A new file 'mu'.\n" },
      { "A/B",           0 },
      { "A/B/lambda",    "This is the file 'lambda'.\n" },
      { "A/B/E",         0 },
      { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
      { "A/B/E/beta",    "This is the file 'beta'.\n" },
      { "A/B/F",         0 },
      { "A/C",           0 },
      { "A/C/kappa",     "This is the file 'kappa'.\n" },
      { "A/D",           0 },
      { "A/D/gamma",     "This is the file 'gamma'.\n" },
      { "A/D/G",         0 },
      { "A/D/G/pi",      "This is the file 'pi'.\n" },
      { "A/D/G/rho",     "This is the file 'rho'.\n" },
      { "A/D/G/tau",     "This is the file 'tau'.\n" },
      { "A/D/G/xi",      "This is the file 'xi'.\n" },
      { "A/D/I",         0 },
      { "A/D/I/delta",   "This is the file 'delta'.\n" },
      { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
    };
    SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
    SVN_ERR(svn_test__validate_tree(revision_root, expected_entries,
                                    21, pool));
  }
  revisions[revision_count++] = after_rev;

  /* (3) E exists in both ANCESTOR and A, but refers to different
     nodes. */
  {
    /* (1) E exists in both ANCESTOR and B, but refers to different
       nodes, and not all nodes are directories.  Conflict. */

    /* ### kff todo: A/mu's contents will be exactly the same.
       If the fs ever starts optimizing this case, these tests may
       start to fail. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/mu", pool));
    SVN_ERR(svn_fs_make_file(txn_root, "A/mu", pool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "A/mu", "This is the file 'mu'.\n", pool));
    SVN_ERR(test_commit_txn(&after_rev, txn, "/A/mu", pool));
    SVN_ERR(svn_fs_abort_txn(txn, pool));

    /* (1) E exists in both ANCESTOR and B, but refers to different
       revisions of the same node.  Conflict. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "A/mu", "A change to file 'mu'.\n", pool));
    SVN_ERR(test_commit_txn(&after_rev, txn, "/A/mu", pool));
    SVN_ERR(svn_fs_abort_txn(txn, pool));

    /* (1) E exists in both ANCESTOR and B, and refers to the same
       node revision.  Replace E with A's node revision.  */
    {
      svn_stringbuf_t *old_mu_contents;
      SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
      SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
      SVN_ERR(svn_test__get_file_contents
              (txn_root, "A/mu", &old_mu_contents, pool));
      if ((! old_mu_contents) || (strcmp(old_mu_contents->data,
                                         "This is the file 'mu'.\n") != 0))
        {
          return svn_error_create
            (SVN_ERR_FS_GENERAL, NULL,
             "got wrong contents from an old revision tree");
        }
      SVN_ERR(svn_fs_make_file(txn_root, "A/sigma", pool));
      SVN_ERR(svn_test__set_file_contents  /* unrelated change */
              (txn_root, "A/sigma", "This is the file 'sigma'.\n", pool));
      SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
      /*********************************************************************/
      /* REVISION 11 */
      /*********************************************************************/
      {
        static svn_test__tree_entry_t expected_entries[] = {
          /* path, contents (0 = dir) */
          { "theta",         "This is the file 'theta'.\n" },
          { "A",             0 },
          { "A/mu",          "A new file 'mu'.\n" },
          { "A/sigma",       "This is the file 'sigma'.\n" },
          { "A/B",           0 },
          { "A/B/lambda",    "This is the file 'lambda'.\n" },
          { "A/B/E",         0 },
          { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
          { "A/B/E/beta",    "This is the file 'beta'.\n" },
          { "A/B/F",         0 },
          { "A/C",           0 },
          { "A/C/kappa",     "This is the file 'kappa'.\n" },
          { "A/D",           0 },
          { "A/D/gamma",     "This is the file 'gamma'.\n" },
          { "A/D/G",         0 },
          { "A/D/G/pi",      "This is the file 'pi'.\n" },
          { "A/D/G/rho",     "This is the file 'rho'.\n" },
          { "A/D/G/tau",     "This is the file 'tau'.\n" },
          { "A/D/G/xi",      "This is the file 'xi'.\n" },
          { "A/D/I",         0 },
          { "A/D/I/delta",   "This is the file 'delta'.\n" },
          { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
        };
        SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
        SVN_ERR(svn_test__validate_tree(revision_root,
                                        expected_entries,
                                        22, pool));
      }
      revisions[revision_count++] = after_rev;
    }
  }

  /* Preparation for upcoming tests.
     We make a new head revision.  There are two changes in the new
     revision: A/B/lambda has been modified.  We will also use the
     recent addition of A/D/G/xi, treated as a modification to
     A/D/G. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[revision_count-1], pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "A/B/lambda", "Change to file 'lambda'.\n", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
  /*********************************************************************/
  /* REVISION 12 */
  /*********************************************************************/
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "theta",         "This is the file 'theta'.\n" },
      { "A",             0 },
      { "A/mu",          "A new file 'mu'.\n" },
      { "A/sigma",       "This is the file 'sigma'.\n" },
      { "A/B",           0 },
      { "A/B/lambda",    "Change to file 'lambda'.\n" },
      { "A/B/E",         0 },
      { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
      { "A/B/E/beta",    "This is the file 'beta'.\n" },
      { "A/B/F",         0 },
      { "A/C",           0 },
      { "A/C/kappa",     "This is the file 'kappa'.\n" },
      { "A/D",           0 },
      { "A/D/gamma",     "This is the file 'gamma'.\n" },
      { "A/D/G",         0 },
      { "A/D/G/pi",      "This is the file 'pi'.\n" },
      { "A/D/G/rho",     "This is the file 'rho'.\n" },
      { "A/D/G/tau",     "This is the file 'tau'.\n" },
      { "A/D/G/xi",      "This is the file 'xi'.\n" },
      { "A/D/I",         0 },
      { "A/D/I/delta",   "This is the file 'delta'.\n" },
      { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
    };
    SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
    SVN_ERR(svn_test__validate_tree(revision_root, expected_entries,
                                    22, pool));
  }
  revisions[revision_count++] = after_rev;

  /* (2) E exists in both ANCESTOR and A, but refers to different
     revisions of the same node. */
  {
    /* (1a) E exists in both ANCESTOR and B, but refers to different
       revisions of the same file node.  Conflict. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "A/B/lambda", "A different change to 'lambda'.\n",
             pool));
    SVN_ERR(test_commit_txn(&after_rev, txn, "/A/B/lambda", pool));
    SVN_ERR(svn_fs_abort_txn(txn, pool));

    /* (1b) E exists in both ANCESTOR and B, but refers to different
       revisions of the same directory node.  Merge A/E and B/E,
       recursively.  Succeed, because no conflict beneath E. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
    SVN_ERR(svn_fs_make_file(txn_root, "A/D/G/nu", pool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "A/D/G/nu", "This is the file 'nu'.\n", pool));
    SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
    /*********************************************************************/
    /* REVISION 13 */
    /*********************************************************************/
    {
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "theta",         "This is the file 'theta'.\n" },
        { "A",             0 },
        { "A/mu",          "A new file 'mu'.\n" },
        { "A/sigma",       "This is the file 'sigma'.\n" },
        { "A/B",           0 },
        { "A/B/lambda",    "Change to file 'lambda'.\n" },
        { "A/B/E",         0 },
        { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
        { "A/B/E/beta",    "This is the file 'beta'.\n" },
        { "A/B/F",         0 },
        { "A/C",           0 },
        { "A/C/kappa",     "This is the file 'kappa'.\n" },
        { "A/D",           0 },
        { "A/D/gamma",     "This is the file 'gamma'.\n" },
        { "A/D/G",         0 },
        { "A/D/G/pi",      "This is the file 'pi'.\n" },
        { "A/D/G/rho",     "This is the file 'rho'.\n" },
        { "A/D/G/tau",     "This is the file 'tau'.\n" },
        { "A/D/G/xi",      "This is the file 'xi'.\n" },
        { "A/D/G/nu",      "This is the file 'nu'.\n" },
        { "A/D/I",         0 },
        { "A/D/I/delta",   "This is the file 'delta'.\n" },
        { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
      };
      SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
      SVN_ERR(svn_test__validate_tree(revision_root,
                                      expected_entries,
                                      23, pool));
    }
    revisions[revision_count++] = after_rev;

    /* (1c) E exists in both ANCESTOR and B, but refers to different
       revisions of the same directory node.  Merge A/E and B/E,
       recursively.  Fail, because conflict beneath E. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
    SVN_ERR(svn_fs_make_file(txn_root, "A/D/G/xi", pool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "A/D/G/xi", "This is a different file 'xi'.\n", pool));
    SVN_ERR(test_commit_txn(&after_rev, txn, "/A/D/G/xi", pool));
    SVN_ERR(svn_fs_abort_txn(txn, pool));

    /* (1) E exists in both ANCESTOR and B, and refers to the same node
       revision.  Replace E with A's node revision.  */
    {
      svn_stringbuf_t *old_lambda_ctnts;
      SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
      SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
      SVN_ERR(svn_test__get_file_contents
              (txn_root, "A/B/lambda", &old_lambda_ctnts, pool));
      if ((! old_lambda_ctnts)
          || (strcmp(old_lambda_ctnts->data,
                     "This is the file 'lambda'.\n") != 0))
        {
          return svn_error_create
            (SVN_ERR_FS_GENERAL, NULL,
             "got wrong contents from an old revision tree");
        }
      SVN_ERR(svn_test__set_file_contents
              (txn_root, "A/D/G/rho",
               "This is an irrelevant change to 'rho'.\n", pool));
      SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
      /*********************************************************************/
      /* REVISION 14 */
      /*********************************************************************/
      {
        static svn_test__tree_entry_t expected_entries[] = {
          /* path, contents (0 = dir) */
          { "theta",         "This is the file 'theta'.\n" },
          { "A",             0 },
          { "A/mu",          "A new file 'mu'.\n" },
          { "A/sigma",       "This is the file 'sigma'.\n" },
          { "A/B",           0 },
          { "A/B/lambda",    "Change to file 'lambda'.\n" },
          { "A/B/E",         0 },
          { "A/B/E/alpha",   "This is the file 'alpha'.\n" },
          { "A/B/E/beta",    "This is the file 'beta'.\n" },
          { "A/B/F",         0 },
          { "A/C",           0 },
          { "A/C/kappa",     "This is the file 'kappa'.\n" },
          { "A/D",           0 },
          { "A/D/gamma",     "This is the file 'gamma'.\n" },
          { "A/D/G",         0 },
          { "A/D/G/pi",      "This is the file 'pi'.\n" },
          { "A/D/G/rho",     "This is an irrelevant change to 'rho'.\n" },
          { "A/D/G/tau",     "This is the file 'tau'.\n" },
          { "A/D/G/xi",      "This is the file 'xi'.\n" },
          { "A/D/G/nu",      "This is the file 'nu'.\n"},
          { "A/D/I",         0 },
          { "A/D/I/delta",   "This is the file 'delta'.\n" },
          { "A/D/I/epsilon", "This is the file 'epsilon'.\n" }
        };
        SVN_ERR(svn_fs_revision_root(&revision_root, fs, after_rev, pool));
        SVN_ERR(svn_test__validate_tree(revision_root,
                                        expected_entries,
                                        23, pool));
      }
      revisions[revision_count++] = after_rev;
    }
  }

  /* (1) E exists in both ANCESTOR and A, and refers to the same node
     revision. */
  {
    /* (1) E exists in both ANCESTOR and B, and refers to the same
       node revision.  Nothing has happened to ANCESTOR/E, so no
       change is necessary. */

    /* This has now been tested about fifty-four trillion times.  We
       don't need to test it again here. */
  }

  /* E exists in ANCESTOR, but has been deleted from A.  E exists in
     both ANCESTOR and B but refers to different revisions of the same
     node.  Conflict.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, revisions[1], pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "iota", "New contents for 'iota'.\n", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, "/iota", pool));
  SVN_ERR(svn_fs_abort_txn(txn, pool));

  return SVN_NO_ERROR;
}


static svn_error_t *
copy_test(const svn_test_opts_t *opts,
          apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *rev_root;
  svn_revnum_t after_rev;

  /* Prepare a filesystem. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-copy-test",
                              opts, pool));

  /* In first txn, create and commit the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));

  /* In second txn, copy the file A/D/G/pi into the subtree A/D/H as
     pi2.  Change that file's contents to state its new name.  Along
     the way, test that the copy history was preserved both during the
     transaction and after the commit. */

  SVN_ERR(svn_fs_revision_root(&rev_root, fs, after_rev, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, after_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_copy(rev_root, "A/D/G/pi",
                      txn_root, "A/D/H/pi2",
                      pool));
  { /* Check that copy history was preserved. */
    svn_revnum_t rev;
    const char *path;

    SVN_ERR(svn_fs_copied_from(&rev, &path, txn_root,
                               "A/D/H/pi2", pool));

    if (rev != after_rev)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "pre-commit copy history not preserved (rev lost) for A/D/H/pi2");

    if (strcmp(path, "/A/D/G/pi") != 0)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "pre-commit copy history not preserved (path lost) for A/D/H/pi2");
  }
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "A/D/H/pi2", "This is the file 'pi2'.\n", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));

  { /* Check that copy history is still preserved _after_ the commit. */
    svn_fs_root_t *root;
    svn_revnum_t rev;
    const char *path;

    SVN_ERR(svn_fs_revision_root(&root, fs, after_rev, pool));
    SVN_ERR(svn_fs_copied_from(&rev, &path, root, "A/D/H/pi2", pool));

    if (rev != (after_rev - 1))
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "post-commit copy history wrong (rev) for A/D/H/pi2");

    if (strcmp(path, "/A/D/G/pi") != 0)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "post-commit copy history wrong (path) for A/D/H/pi2");
  }

  /* Let's copy the copy we just made, to make sure copy history gets
     chained correctly. */
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, after_rev, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, after_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_copy(rev_root, "A/D/H/pi2", txn_root, "A/D/H/pi3", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
  { /* Check the copy history. */
    svn_fs_root_t *root;
    svn_revnum_t rev;
    const char *path;

    /* Check that the original copy still has its old history. */
    SVN_ERR(svn_fs_revision_root(&root, fs, (after_rev - 1), pool));
    SVN_ERR(svn_fs_copied_from(&rev, &path, root, "A/D/H/pi2", pool));

    if (rev != (after_rev - 2))
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "first copy history wrong (rev) for A/D/H/pi2");

    if (strcmp(path, "/A/D/G/pi") != 0)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "first copy history wrong (path) for A/D/H/pi2");

    /* Check that the copy of the copy has the right history. */
    SVN_ERR(svn_fs_revision_root(&root, fs, after_rev, pool));
    SVN_ERR(svn_fs_copied_from(&rev, &path, root, "A/D/H/pi3", pool));

    if (rev != (after_rev - 1))
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "second copy history wrong (rev) for A/D/H/pi3");

    if (strcmp(path, "/A/D/H/pi2") != 0)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "second copy history wrong (path) for A/D/H/pi3");
  }

  /* Commit a regular change to a copy, make sure the copy history
     isn't inherited. */
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, after_rev, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, after_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "A/D/H/pi3", "This is the file 'pi3'.\n", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
  { /* Check the copy history. */
    svn_fs_root_t *root;
    svn_revnum_t rev;
    const char *path;

    /* Check that the copy still has its history. */
    SVN_ERR(svn_fs_revision_root(&root, fs, (after_rev - 1), pool));
    SVN_ERR(svn_fs_copied_from(&rev, &path, root, "A/D/H/pi3", pool));

    if (rev != (after_rev - 2))
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (rev) for A/D/H/pi3");

    if (strcmp(path, "/A/D/H/pi2") != 0)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (path) for A/D/H/pi3");

    /* Check that the next revision after the copy has no copy history. */
    SVN_ERR(svn_fs_revision_root(&root, fs, after_rev, pool));
    SVN_ERR(svn_fs_copied_from(&rev, &path, root, "A/D/H/pi3", pool));

    if (rev != SVN_INVALID_REVNUM)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (rev) for A/D/H/pi3");

    if (path != NULL)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (path) for A/D/H/pi3");
  }

  /* Then, as if that wasn't fun enough, copy the whole subtree A/D/H
     into the root directory as H2! */
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, after_rev, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, after_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_copy(rev_root, "A/D/H", txn_root, "H2", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
  { /* Check the copy history. */
    svn_fs_root_t *root;
    svn_revnum_t rev;
    const char *path;

    /* Check that the top of the copy has history. */
    SVN_ERR(svn_fs_revision_root(&root, fs, after_rev, pool));
    SVN_ERR(svn_fs_copied_from(&rev, &path, root, "H2", pool));

    if (rev != (after_rev - 1))
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (rev) for H2");

    if (strcmp(path, "/A/D/H") != 0)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (path) for H2");

    /* Check that a random file under H2 reports no copy history. */
    SVN_ERR(svn_fs_copied_from(&rev, &path, root, "H2/omega", pool));

    if (rev != SVN_INVALID_REVNUM)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (rev) for H2/omega");

    if (path != NULL)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (path) for H2/omega");

    /* Note that H2/pi2 still has copy history, though.  See the doc
       string for svn_fs_copied_from() for more on this. */
  }

  /* Let's live dangerously.  What happens if we copy a path into one
     of its own children.  Looping filesystem?  Cyclic ancestry?
     Another West Virginia family tree with no branches?  We certainly
     hope that's not the case. */
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, after_rev, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, after_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_copy(rev_root, "A/B", txn_root, "A/B/E/B", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, pool));
  { /* Check the copy history. */
    svn_fs_root_t *root;
    svn_revnum_t rev;
    const char *path;

    /* Check that the copy has history. */
    SVN_ERR(svn_fs_revision_root(&root, fs, after_rev, pool));
    SVN_ERR(svn_fs_copied_from(&rev, &path, root, "A/B/E/B", pool));

    if (rev != (after_rev - 1))
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (rev) for A/B/E/B");

    if (strcmp(path, "/A/B") != 0)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (path) for A/B/E/B");

    /* Check that the original does not have copy history. */
    SVN_ERR(svn_fs_revision_root(&root, fs, after_rev, pool));
    SVN_ERR(svn_fs_copied_from(&rev, &path, root, "A/B", pool));

    if (rev != SVN_INVALID_REVNUM)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (rev) for A/B");

    if (path != NULL)
      return svn_error_create
        (SVN_ERR_FS_GENERAL, NULL,
         "copy history wrong (path) for A/B");
  }

  /* After all these changes, let's see if the filesystem looks as we
     would expect it to. */
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "iota",        "This is the file 'iota'.\n" },
      { "H2",          0 },
      { "H2/chi",      "This is the file 'chi'.\n" },
      { "H2/pi2",      "This is the file 'pi2'.\n" },
      { "H2/pi3",      "This is the file 'pi3'.\n" },
      { "H2/psi",      "This is the file 'psi'.\n" },
      { "H2/omega",    "This is the file 'omega'.\n" },
      { "A",           0 },
      { "A/mu",        "This is the file 'mu'.\n" },
      { "A/B",         0 },
      { "A/B/lambda",  "This is the file 'lambda'.\n" },
      { "A/B/E",       0 },
      { "A/B/E/alpha", "This is the file 'alpha'.\n" },
      { "A/B/E/beta",  "This is the file 'beta'.\n" },
      { "A/B/E/B",         0 },
      { "A/B/E/B/lambda",  "This is the file 'lambda'.\n" },
      { "A/B/E/B/E",       0 },
      { "A/B/E/B/E/alpha", "This is the file 'alpha'.\n" },
      { "A/B/E/B/E/beta",  "This is the file 'beta'.\n" },
      { "A/B/E/B/F",       0 },
      { "A/B/F",       0 },
      { "A/C",         0 },
      { "A/D",         0 },
      { "A/D/gamma",   "This is the file 'gamma'.\n" },
      { "A/D/G",       0 },
      { "A/D/G/pi",    "This is the file 'pi'.\n" },
      { "A/D/G/rho",   "This is the file 'rho'.\n" },
      { "A/D/G/tau",   "This is the file 'tau'.\n" },
      { "A/D/H",       0 },
      { "A/D/H/chi",   "This is the file 'chi'.\n" },
      { "A/D/H/pi2",   "This is the file 'pi2'.\n" },
      { "A/D/H/pi3",   "This is the file 'pi3'.\n" },
      { "A/D/H/psi",   "This is the file 'psi'.\n" },
      { "A/D/H/omega", "This is the file 'omega'.\n" }
    };
    SVN_ERR(svn_fs_revision_root(&rev_root, fs, after_rev, pool));
    SVN_ERR(svn_test__validate_tree(rev_root, expected_entries,
                                    34, pool));
  }

  return SVN_NO_ERROR;
}


/* This tests deleting of mutable nodes.  We build a tree in a
 * transaction, then try to delete various items in the tree.  We
 * never commit the tree, so every entry being deleted points to a
 * mutable node.
 *
 * ### todo: this test was written before commits worked.  It might
 * now be worthwhile to combine it with delete().
 */
static svn_error_t *
delete_mutables(const svn_test_opts_t *opts,
                apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  svn_error_t *err;

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-del-from-dir",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));

  /* Baby, it's time to test like you've never tested before.  We do
   * the following, in this order:
   *
   *    1. Delete a single file somewhere, succeed.
   *    2. Delete two files of three, then make sure the third remains.
   *    3. Delete the third and last file.
   *    4. Try again to delete the dir, succeed.
   *    5. Delete one of the natively empty dirs, succeed.
   *    6. Try to delete root, fail.
   *    7. Try to delete a top-level file, succeed.
   *
   * Specifically, that's:
   *
   *    1. Delete A/D/gamma.
   *    2. Delete A/D/G/pi, A/D/G/rho.
   *    3. Delete A/D/G/tau.
   *    4. Try again to delete A/D/G, succeed.
   *    5. Delete A/C.
   *    6. Try to delete /, fail.
   *    7. Try to delete iota, succeed.
   *
   * Before and after each deletion or attempted deletion, we probe
   * the affected directory, to make sure everything is as it should
   * be.
   */

  /* 1 */
  {
    const svn_fs_id_t *gamma_id;
    SVN_ERR(svn_fs_node_id(&gamma_id, txn_root, "A/D/gamma", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "gamma", pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/D/gamma", pool));
    SVN_ERR(check_entry_absent(txn_root, "A/D", "gamma", pool));
  }

  /* 2 */
  {
    const svn_fs_id_t *pi_id, *rho_id, *tau_id;
    SVN_ERR(svn_fs_node_id(&pi_id, txn_root, "A/D/G/pi", pool));
    SVN_ERR(svn_fs_node_id(&rho_id, txn_root, "A/D/G/rho", pool));
    SVN_ERR(svn_fs_node_id(&tau_id, txn_root, "A/D/G/tau", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "pi", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "rho", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "tau", pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/D/G/pi", pool));
    SVN_ERR(check_entry_absent(txn_root, "A/D/G", "pi", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "rho", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "tau", pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/D/G/rho", pool));
    SVN_ERR(check_entry_absent(txn_root, "A/D/G", "pi", pool));
    SVN_ERR(check_entry_absent(txn_root, "A/D/G", "rho", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "tau", pool));
  }

  /* 3 */
  {
    const svn_fs_id_t *tau_id;
    SVN_ERR(svn_fs_node_id(&tau_id, txn_root, "A/D/G/tau", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "tau", pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/D/G/tau", pool));
    SVN_ERR(check_entry_absent(txn_root, "A/D/G", "tau", pool));
  }

  /* 4 */
  {
    const svn_fs_id_t *G_id;
    SVN_ERR(svn_fs_node_id(&G_id, txn_root, "A/D/G", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "G", pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/D/G", pool));        /* succeed */
    SVN_ERR(check_entry_absent(txn_root, "A/D", "G", pool));
  }

  /* 5 */
  {
    const svn_fs_id_t *C_id;
    SVN_ERR(svn_fs_node_id(&C_id, txn_root, "A/C", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "C", pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/C", pool));
    SVN_ERR(check_entry_absent(txn_root, "A", "C", pool));
  }

  /* 6 */
  {
    const svn_fs_id_t *root_id;
    SVN_ERR(svn_fs_node_id(&root_id, txn_root, "", pool));

    err = svn_fs_delete(txn_root, "", pool);

    if (err && (err->apr_err != SVN_ERR_FS_ROOT_DIR))
      {
        return svn_error_createf
          (SVN_ERR_FS_GENERAL, NULL,
           "deleting root directory got wrong error");
      }
    else if (! err)
      {
        return svn_error_createf
          (SVN_ERR_FS_GENERAL, NULL,
           "deleting root directory failed to get error");
      }
    svn_error_clear(err);

  }

  /* 7 */
  {
    const svn_fs_id_t *iota_id;
    SVN_ERR(svn_fs_node_id(&iota_id, txn_root, "iota", pool));
    SVN_ERR(check_entry_present(txn_root, "", "iota", pool));
    SVN_ERR(svn_fs_delete(txn_root, "iota", pool));
    SVN_ERR(check_entry_absent(txn_root, "", "iota", pool));
  }

  return SVN_NO_ERROR;
}


/* This tests deleting in general.
 *
 * ### todo: this test was written after (and independently of)
 * delete_mutables().  It might be worthwhile to combine them.
 */
static svn_error_t *
delete(const svn_test_opts_t *opts,
       apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  svn_revnum_t new_rev;

  /* This function tests 5 cases:
   *
   * 1. Delete mutable file.
   * 2. Delete mutable directory.
   * 3. Delete mutable directory with immutable nodes.
   * 4. Delete immutable file.
   * 5. Delete immutable directory.
   */

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-del-tree",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));

  /* 1. Delete mutable file. */
  {
    const svn_fs_id_t *iota_id, *gamma_id;
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "A",           0 },
      { "A/mu",        "This is the file 'mu'.\n" },
      { "A/B",         0 },
      { "A/B/lambda",  "This is the file 'lambda'.\n" },
      { "A/B/E",       0 },
      { "A/B/E/alpha", "This is the file 'alpha'.\n" },
      { "A/B/E/beta",  "This is the file 'beta'.\n" },
      { "A/C",         0 },
      { "A/B/F",       0 },
      { "A/D",         0 },
      { "A/D/G",       0 },
      { "A/D/G/pi",    "This is the file 'pi'.\n" },
      { "A/D/G/rho",   "This is the file 'rho'.\n" },
      { "A/D/G/tau",   "This is the file 'tau'.\n" },
      { "A/D/H",       0 },
      { "A/D/H/chi",   "This is the file 'chi'.\n" },
      { "A/D/H/psi",   "This is the file 'psi'.\n" },
      { "A/D/H/omega", "This is the file 'omega'.\n" }
    };

    /* Check nodes revision ID is gone.  */
    SVN_ERR(svn_fs_node_id(&iota_id, txn_root, "iota", pool));
    SVN_ERR(svn_fs_node_id(&gamma_id, txn_root, "A/D/gamma", pool));

    SVN_ERR(check_entry_present(txn_root, "", "iota", pool));

    /* Try deleting mutable files. */
    SVN_ERR(svn_fs_delete(txn_root, "iota", pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/D/gamma", pool));
    SVN_ERR(check_entry_absent(txn_root, "", "iota", pool));
    SVN_ERR(check_entry_absent(txn_root, "A/D", "gamma", pool));

    /* Validate the tree.  */
    SVN_ERR(svn_test__validate_tree(txn_root, expected_entries, 18, pool));
  }
  /* Abort transaction.  */
  SVN_ERR(svn_fs_abort_txn(txn, pool));

  /* 2. Delete mutable directory. */

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));

  {
    const svn_fs_id_t *A_id, *mu_id, *B_id, *lambda_id, *E_id, *alpha_id,
      *beta_id, *F_id, *C_id, *D_id, *gamma_id, *H_id, *chi_id,
      *psi_id, *omega_id, *G_id, *pi_id, *rho_id, *tau_id;

    /* Check nodes revision ID is gone.  */
    SVN_ERR(svn_fs_node_id(&A_id, txn_root, "/A", pool));
    SVN_ERR(check_entry_present(txn_root, "", "A", pool));
    SVN_ERR(svn_fs_node_id(&mu_id, txn_root, "/A/mu", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "mu", pool));
    SVN_ERR(svn_fs_node_id(&B_id, txn_root, "/A/B", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "B", pool));
    SVN_ERR(svn_fs_node_id(&lambda_id, txn_root, "/A/B/lambda", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B", "lambda", pool));
    SVN_ERR(svn_fs_node_id(&E_id, txn_root, "/A/B/E", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B", "E", pool));
    SVN_ERR(svn_fs_node_id(&alpha_id, txn_root, "/A/B/E/alpha", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B/E", "alpha", pool));
    SVN_ERR(svn_fs_node_id(&beta_id, txn_root, "/A/B/E/beta", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B/E", "beta", pool));
    SVN_ERR(svn_fs_node_id(&F_id, txn_root, "/A/B/F", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B", "F", pool));
    SVN_ERR(svn_fs_node_id(&C_id, txn_root, "/A/C", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "C", pool));
    SVN_ERR(svn_fs_node_id(&D_id, txn_root, "/A/D", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "D", pool));
    SVN_ERR(svn_fs_node_id(&gamma_id, txn_root, "/A/D/gamma", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "gamma", pool));
    SVN_ERR(svn_fs_node_id(&H_id, txn_root, "/A/D/H", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "H", pool));
    SVN_ERR(svn_fs_node_id(&chi_id, txn_root, "/A/D/H/chi", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/H", "chi", pool));
    SVN_ERR(svn_fs_node_id(&psi_id, txn_root, "/A/D/H/psi", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/H", "psi", pool));
    SVN_ERR(svn_fs_node_id(&omega_id, txn_root, "/A/D/H/omega", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/H", "omega", pool));
    SVN_ERR(svn_fs_node_id(&G_id, txn_root, "/A/D/G", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "G", pool));
    SVN_ERR(svn_fs_node_id(&pi_id, txn_root, "/A/D/G/pi", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "pi", pool));
    SVN_ERR(svn_fs_node_id(&rho_id, txn_root, "/A/D/G/rho", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "rho", pool));
    SVN_ERR(svn_fs_node_id(&tau_id, txn_root, "/A/D/G/tau", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "tau", pool));

    /* Try deleting a mutable empty dir. */
    SVN_ERR(svn_fs_delete(txn_root, "A/C", pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/B/F", pool));
    SVN_ERR(check_entry_absent(txn_root, "A", "C", pool));
    SVN_ERR(check_entry_absent(txn_root, "A/B", "F", pool));

    /* Now delete a mutable non-empty dir. */
    SVN_ERR(svn_fs_delete(txn_root, "A", pool));
    SVN_ERR(check_entry_absent(txn_root, "", "A", pool));

    /* Validate the tree.  */
    {
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "iota",        "This is the file 'iota'.\n" } };
      SVN_ERR(svn_test__validate_tree(txn_root, expected_entries, 1, pool));
    }
  }

  /* Abort transaction.  */
  SVN_ERR(svn_fs_abort_txn(txn, pool));

  /* 3. Delete mutable directory with immutable nodes. */

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));

  /* Commit the greek tree. */
  SVN_ERR(svn_fs_commit_txn(NULL, &new_rev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(new_rev));

  /* Create new transaction. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, new_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  {
    const svn_fs_id_t *A_id, *mu_id, *B_id, *lambda_id, *E_id, *alpha_id,
      *beta_id, *F_id, *C_id, *D_id, *gamma_id, *H_id, *chi_id,
      *psi_id, *omega_id, *G_id, *pi_id, *rho_id, *tau_id, *sigma_id;

    /* Create A/D/G/sigma.  This makes all components of A/D/G
       mutable.  */
    SVN_ERR(svn_fs_make_file(txn_root, "A/D/G/sigma", pool));
    SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/G/sigma",
                                        "This is another file 'sigma'.\n", pool));

    /* Check that mutable node-revision-IDs are removed and immutable
       ones still exist.  */
    SVN_ERR(svn_fs_node_id(&A_id, txn_root, "/A", pool));
    SVN_ERR(check_entry_present(txn_root, "", "A", pool));
    SVN_ERR(svn_fs_node_id(&mu_id, txn_root, "/A/mu", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "mu", pool));
    SVN_ERR(svn_fs_node_id(&B_id, txn_root, "/A/B", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "B", pool));
    SVN_ERR(svn_fs_node_id(&lambda_id, txn_root, "/A/B/lambda", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B", "lambda", pool));
    SVN_ERR(svn_fs_node_id(&E_id, txn_root, "/A/B/E", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B", "E", pool));
    SVN_ERR(svn_fs_node_id(&alpha_id, txn_root, "/A/B/E/alpha", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B/E", "alpha", pool));
    SVN_ERR(svn_fs_node_id(&beta_id, txn_root, "/A/B/E/beta", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B/E", "beta", pool));
    SVN_ERR(svn_fs_node_id(&F_id, txn_root, "/A/B/F", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B", "F", pool));
    SVN_ERR(svn_fs_node_id(&C_id, txn_root, "/A/C", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "C", pool));
    SVN_ERR(svn_fs_node_id(&D_id, txn_root, "/A/D", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "D", pool));
    SVN_ERR(svn_fs_node_id(&gamma_id, txn_root, "/A/D/gamma", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "gamma", pool));
    SVN_ERR(svn_fs_node_id(&H_id, txn_root, "/A/D/H", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "H", pool));
    SVN_ERR(svn_fs_node_id(&chi_id, txn_root, "/A/D/H/chi", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/H", "chi", pool));
    SVN_ERR(svn_fs_node_id(&psi_id, txn_root, "/A/D/H/psi", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/H", "psi", pool));
    SVN_ERR(svn_fs_node_id(&omega_id, txn_root, "/A/D/H/omega", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/H", "omega", pool));
    SVN_ERR(svn_fs_node_id(&G_id, txn_root, "/A/D/G", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "G", pool));
    SVN_ERR(svn_fs_node_id(&pi_id, txn_root, "/A/D/G/pi", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "pi", pool));
    SVN_ERR(svn_fs_node_id(&rho_id, txn_root, "/A/D/G/rho", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "rho", pool));
    SVN_ERR(svn_fs_node_id(&tau_id, txn_root, "/A/D/G/tau", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "tau", pool));
    SVN_ERR(svn_fs_node_id(&sigma_id, txn_root, "/A/D/G/sigma", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "sigma", pool));

    /* Delete "A" */
    SVN_ERR(svn_fs_delete(txn_root, "A", pool));
    SVN_ERR(check_entry_absent(txn_root, "", "A", pool));

    /* Validate the tree.  */
    {
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "iota",        "This is the file 'iota'.\n" }
      };

      SVN_ERR(svn_test__validate_tree(txn_root, expected_entries, 1, pool));
    }
  }

  /* Abort transaction.  */
  SVN_ERR(svn_fs_abort_txn(txn, pool));

  /* 4. Delete immutable file. */

  /* Create new transaction. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, new_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  {
    const svn_fs_id_t *iota_id, *gamma_id;

    /* Check nodes revision ID is present.  */
    SVN_ERR(svn_fs_node_id(&iota_id, txn_root, "iota", pool));
    SVN_ERR(svn_fs_node_id(&gamma_id, txn_root, "A/D/gamma", pool));
    SVN_ERR(check_entry_present(txn_root, "", "iota", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "gamma", pool));

    /* Delete some files. */
    SVN_ERR(svn_fs_delete(txn_root, "iota", pool));
    SVN_ERR(svn_fs_delete(txn_root, "A/D/gamma", pool));
    SVN_ERR(check_entry_absent(txn_root, "", "iota", pool));
    SVN_ERR(check_entry_absent(txn_root, "A/D", "iota", pool));

    /* Validate the tree.  */
    {
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "A",           0 },
        { "A/mu",        "This is the file 'mu'.\n" },
        { "A/B",         0 },
        { "A/B/lambda",  "This is the file 'lambda'.\n" },
        { "A/B/E",       0 },
        { "A/B/E/alpha", "This is the file 'alpha'.\n" },
        { "A/B/E/beta",  "This is the file 'beta'.\n" },
        { "A/B/F",       0 },
        { "A/C",         0 },
        { "A/D",         0 },
        { "A/D/G",       0 },
        { "A/D/G/pi",    "This is the file 'pi'.\n" },
        { "A/D/G/rho",   "This is the file 'rho'.\n" },
        { "A/D/G/tau",   "This is the file 'tau'.\n" },
        { "A/D/H",       0 },
        { "A/D/H/chi",   "This is the file 'chi'.\n" },
        { "A/D/H/psi",   "This is the file 'psi'.\n" },
        { "A/D/H/omega", "This is the file 'omega'.\n" }
      };
      SVN_ERR(svn_test__validate_tree(txn_root, expected_entries, 18, pool));
    }
  }

  /* Abort transaction.  */
  SVN_ERR(svn_fs_abort_txn(txn, pool));

  /* 5. Delete immutable directory. */

  /* Create new transaction. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, new_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  {
    const svn_fs_id_t *A_id, *mu_id, *B_id, *lambda_id, *E_id, *alpha_id,
      *beta_id, *F_id, *C_id, *D_id, *gamma_id, *H_id, *chi_id,
      *psi_id, *omega_id, *G_id, *pi_id, *rho_id, *tau_id;

    /* Check nodes revision ID is present.  */
    SVN_ERR(svn_fs_node_id(&A_id, txn_root, "/A", pool));
    SVN_ERR(check_entry_present(txn_root, "", "A", pool));
    SVN_ERR(svn_fs_node_id(&mu_id, txn_root, "/A/mu", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "mu", pool));
    SVN_ERR(svn_fs_node_id(&B_id, txn_root, "/A/B", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "B", pool));
    SVN_ERR(svn_fs_node_id(&lambda_id, txn_root, "/A/B/lambda", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B", "lambda", pool));
    SVN_ERR(svn_fs_node_id(&E_id, txn_root, "/A/B/E", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B", "E", pool));
    SVN_ERR(svn_fs_node_id(&alpha_id, txn_root, "/A/B/E/alpha", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B/E", "alpha", pool));
    SVN_ERR(svn_fs_node_id(&beta_id, txn_root, "/A/B/E/beta", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B/E", "beta", pool));
    SVN_ERR(svn_fs_node_id(&F_id, txn_root, "/A/B/F", pool));
    SVN_ERR(check_entry_present(txn_root, "A/B", "F", pool));
    SVN_ERR(svn_fs_node_id(&C_id, txn_root, "/A/C", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "C", pool));
    SVN_ERR(svn_fs_node_id(&D_id, txn_root, "/A/D", pool));
    SVN_ERR(check_entry_present(txn_root, "A", "D", pool));
    SVN_ERR(svn_fs_node_id(&gamma_id, txn_root, "/A/D/gamma", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "gamma", pool));
    SVN_ERR(svn_fs_node_id(&H_id, txn_root, "/A/D/H", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "H", pool));
    SVN_ERR(svn_fs_node_id(&chi_id, txn_root, "/A/D/H/chi", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/H", "chi", pool));
    SVN_ERR(svn_fs_node_id(&psi_id, txn_root, "/A/D/H/psi", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/H", "psi", pool));
    SVN_ERR(svn_fs_node_id(&omega_id, txn_root, "/A/D/H/omega", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/H", "omega", pool));
    SVN_ERR(svn_fs_node_id(&G_id, txn_root, "/A/D/G", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D", "G", pool));
    SVN_ERR(svn_fs_node_id(&pi_id, txn_root, "/A/D/G/pi", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "pi", pool));
    SVN_ERR(svn_fs_node_id(&rho_id, txn_root, "/A/D/G/rho", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "rho", pool));
    SVN_ERR(svn_fs_node_id(&tau_id, txn_root, "/A/D/G/tau", pool));
    SVN_ERR(check_entry_present(txn_root, "A/D/G", "tau", pool));

    /* Delete "A" */
    SVN_ERR(svn_fs_delete(txn_root, "A", pool));
    SVN_ERR(check_entry_absent(txn_root, "", "A", pool));

    /* Validate the tree.  */
    {
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "iota",        "This is the file 'iota'.\n" }
      };
      SVN_ERR(svn_test__validate_tree(txn_root, expected_entries, 1, pool));
    }
  }

  return SVN_NO_ERROR;
}



/* Test the datestamps on commits. */
static svn_error_t *
commit_date(const svn_test_opts_t *opts,
            apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  svn_revnum_t rev;
  svn_string_t *datestamp;
  apr_time_t before_commit, at_commit, after_commit;

  /* Prepare a filesystem. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-commit-date",
                              opts, pool));

  before_commit = apr_time_now();

  /* Commit a greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(NULL, &rev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(rev));

  after_commit = apr_time_now();

  /* Get the datestamp of the commit. */
  SVN_ERR(svn_fs_revision_prop(&datestamp, fs, rev, SVN_PROP_REVISION_DATE,
                               pool));

  if (datestamp == NULL)
    return svn_error_create
      (SVN_ERR_FS_GENERAL, NULL,
       "failed to get datestamp of committed revision");

  SVN_ERR(svn_time_from_cstring(&at_commit, datestamp->data, pool));

  if (at_commit < before_commit)
    return svn_error_create
      (SVN_ERR_FS_GENERAL, NULL,
       "datestamp too early");

  if (at_commit > after_commit)
    return svn_error_create
      (SVN_ERR_FS_GENERAL, NULL,
       "datestamp too late");

  return SVN_NO_ERROR;
}


static svn_error_t *
check_old_revisions(const svn_test_opts_t *opts,
                    apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  svn_revnum_t rev;
  apr_pool_t *subpool = svn_pool_create(pool);

  /* Prepare a filesystem. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-check-old-revisions",
                              opts, pool));

  /* Commit a greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(rev));
  svn_pool_clear(subpool);

  /* Modify and commit iota a few times, then test to see if we can
     retrieve all the committed revisions. */
  {
    /* right-side numbers match revision numbers */
#define iota_contents_1 "This is the file 'iota'.\n"

    /* Add a char to the front. */
#define iota_contents_2 "XThis is the file 'iota'.\n"

    /* Add a char to the end. */
#define iota_contents_3 "XThis is the file 'iota'.\nX"

    /* Add a couple of chars in the middle. */
#define iota_contents_4 "XThis is the X file 'iota'.\nX"

    /* Randomly add and delete chars all over. */
#define iota_contents_5 \
    "XTYhQis is ACK, PHHHT! no longer 'ioZZZZZta'.blarf\nbye"

    /* Reassure iota that it will live for quite some time. */
#define iota_contents_6 "Matthew 5:18 (Revised Standard Version) --\n\
For truly, I say to you, till heaven and earth pass away, not an iota,\n\
not a dot, will pass from the law until all is accomplished."

    /* Revert to the original contents. */
#define iota_contents_7 "This is the file 'iota'.\n"

    /* Revision 2. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, rev, subpool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "iota", iota_contents_2, subpool));
    SVN_ERR(svn_fs_commit_txn(NULL, &rev, txn, subpool));
    SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(rev));
    svn_pool_clear(subpool);

    /* Revision 3. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, rev, subpool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "iota", iota_contents_3, subpool));
    SVN_ERR(svn_fs_commit_txn(NULL, &rev, txn, subpool));
    SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(rev));
    svn_pool_clear(subpool);

    /* Revision 4. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, rev, subpool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "iota", iota_contents_4, subpool));
    SVN_ERR(svn_fs_commit_txn(NULL, &rev, txn, subpool));
    SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(rev));
    svn_pool_clear(subpool);

    /* Revision 5. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, rev, subpool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "iota", iota_contents_5, subpool));
    SVN_ERR(svn_fs_commit_txn(NULL, &rev, txn, subpool));
    SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(rev));
    svn_pool_clear(subpool);

    /* Revision 6. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, rev, subpool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "iota", iota_contents_6, subpool));
    SVN_ERR(svn_fs_commit_txn(NULL, &rev, txn, subpool));
    SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(rev));
    svn_pool_clear(subpool);

    /* Revision 7. */
    SVN_ERR(svn_fs_begin_txn(&txn, fs, rev, subpool));
    SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
    SVN_ERR(svn_test__set_file_contents
            (txn_root, "iota", iota_contents_7, subpool));
    SVN_ERR(svn_fs_commit_txn(NULL, &rev, txn, subpool));
    SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(rev));
    svn_pool_clear(subpool);

    /** Now check the full Greek Tree in all of those revisions,
        adjusting `iota' for each one. ***/

    /* Validate revision 1.  */
    {
      svn_fs_root_t *root;
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "iota",        iota_contents_1 },
        { "A",           0 },
        { "A/mu",        "This is the file 'mu'.\n" },
        { "A/B",         0 },
        { "A/B/lambda",  "This is the file 'lambda'.\n" },
        { "A/B/E",       0 },
        { "A/B/E/alpha", "This is the file 'alpha'.\n" },
        { "A/B/E/beta",  "This is the file 'beta'.\n" },
        { "A/B/F",       0 },
        { "A/C",         0 },
        { "A/D",         0 },
        { "A/D/gamma",   "This is the file 'gamma'.\n" },
        { "A/D/G",       0 },
        { "A/D/G/pi",    "This is the file 'pi'.\n" },
        { "A/D/G/rho",   "This is the file 'rho'.\n" },
        { "A/D/G/tau",   "This is the file 'tau'.\n" },
        { "A/D/H",       0 },
        { "A/D/H/chi",   "This is the file 'chi'.\n" },
        { "A/D/H/psi",   "This is the file 'psi'.\n" },
        { "A/D/H/omega", "This is the file 'omega'.\n" }
      };

      SVN_ERR(svn_fs_revision_root(&root, fs, 1, pool));
      SVN_ERR(svn_test__validate_tree(root, expected_entries, 20, pool));
    }

    /* Validate revision 2.  */
    {
      svn_fs_root_t *root;
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "iota",        iota_contents_2 },
        { "A",           0 },
        { "A/mu",        "This is the file 'mu'.\n" },
        { "A/B",         0 },
        { "A/B/lambda",  "This is the file 'lambda'.\n" },
        { "A/B/E",       0 },
        { "A/B/E/alpha", "This is the file 'alpha'.\n" },
        { "A/B/E/beta",  "This is the file 'beta'.\n" },
        { "A/B/F",       0 },
        { "A/C",         0 },
        { "A/D",         0 },
        { "A/D/gamma",   "This is the file 'gamma'.\n" },
        { "A/D/G",       0 },
        { "A/D/G/pi",    "This is the file 'pi'.\n" },
        { "A/D/G/rho",   "This is the file 'rho'.\n" },
        { "A/D/G/tau",   "This is the file 'tau'.\n" },
        { "A/D/H",       0 },
        { "A/D/H/chi",   "This is the file 'chi'.\n" },
        { "A/D/H/psi",   "This is the file 'psi'.\n" },
        { "A/D/H/omega", "This is the file 'omega'.\n" }
      };

      SVN_ERR(svn_fs_revision_root(&root, fs, 2, pool));
      SVN_ERR(svn_test__validate_tree(root, expected_entries, 20, pool));
    }

    /* Validate revision 3.  */
    {
      svn_fs_root_t *root;
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "iota",        iota_contents_3 },
        { "A",           0 },
        { "A/mu",        "This is the file 'mu'.\n" },
        { "A/B",         0 },
        { "A/B/lambda",  "This is the file 'lambda'.\n" },
        { "A/B/E",       0 },
        { "A/B/E/alpha", "This is the file 'alpha'.\n" },
        { "A/B/E/beta",  "This is the file 'beta'.\n" },
        { "A/B/F",       0 },
        { "A/C",         0 },
        { "A/D",         0 },
        { "A/D/gamma",   "This is the file 'gamma'.\n" },
        { "A/D/G",       0 },
        { "A/D/G/pi",    "This is the file 'pi'.\n" },
        { "A/D/G/rho",   "This is the file 'rho'.\n" },
        { "A/D/G/tau",   "This is the file 'tau'.\n" },
        { "A/D/H",       0 },
        { "A/D/H/chi",   "This is the file 'chi'.\n" },
        { "A/D/H/psi",   "This is the file 'psi'.\n" },
        { "A/D/H/omega", "This is the file 'omega'.\n" }
      };

      SVN_ERR(svn_fs_revision_root(&root, fs, 3, pool));
      SVN_ERR(svn_test__validate_tree(root, expected_entries, 20, pool));
    }

    /* Validate revision 4.  */
    {
      svn_fs_root_t *root;
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "iota",        iota_contents_4 },
        { "A",           0 },
        { "A/mu",        "This is the file 'mu'.\n" },
        { "A/B",         0 },
        { "A/B/lambda",  "This is the file 'lambda'.\n" },
        { "A/B/E",       0 },
        { "A/B/E/alpha", "This is the file 'alpha'.\n" },
        { "A/B/E/beta",  "This is the file 'beta'.\n" },
        { "A/B/F",       0 },
        { "A/C",         0 },
        { "A/D",         0 },
        { "A/D/gamma",   "This is the file 'gamma'.\n" },
        { "A/D/G",       0 },
        { "A/D/G/pi",    "This is the file 'pi'.\n" },
        { "A/D/G/rho",   "This is the file 'rho'.\n" },
        { "A/D/G/tau",   "This is the file 'tau'.\n" },
        { "A/D/H",       0 },
        { "A/D/H/chi",   "This is the file 'chi'.\n" },
        { "A/D/H/psi",   "This is the file 'psi'.\n" },
        { "A/D/H/omega", "This is the file 'omega'.\n" }
      };

      SVN_ERR(svn_fs_revision_root(&root, fs, 4, pool));
      SVN_ERR(svn_test__validate_tree(root, expected_entries, 20, pool));
    }

    /* Validate revision 5.  */
    {
      svn_fs_root_t *root;
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "iota",        iota_contents_5 },
        { "A",           0 },
        { "A/mu",        "This is the file 'mu'.\n" },
        { "A/B",         0 },
        { "A/B/lambda",  "This is the file 'lambda'.\n" },
        { "A/B/E",       0 },
        { "A/B/E/alpha", "This is the file 'alpha'.\n" },
        { "A/B/E/beta",  "This is the file 'beta'.\n" },
        { "A/B/F",       0 },
        { "A/C",         0 },
        { "A/D",         0 },
        { "A/D/G",       0 },
        { "A/D/gamma",   "This is the file 'gamma'.\n" },
        { "A/D/G/pi",    "This is the file 'pi'.\n" },
        { "A/D/G/rho",   "This is the file 'rho'.\n" },
        { "A/D/G/tau",   "This is the file 'tau'.\n" },
        { "A/D/H",       0 },
        { "A/D/H/chi",   "This is the file 'chi'.\n" },
        { "A/D/H/psi",   "This is the file 'psi'.\n" },
        { "A/D/H/omega", "This is the file 'omega'.\n" }
      };

      SVN_ERR(svn_fs_revision_root(&root, fs, 5, pool));
      SVN_ERR(svn_test__validate_tree(root, expected_entries, 20, pool));
    }

    /* Validate revision 6.  */
    {
      svn_fs_root_t *root;
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "iota",        iota_contents_6 },
        { "A",           0 },
        { "A/mu",        "This is the file 'mu'.\n" },
        { "A/B",         0 },
        { "A/B/lambda",  "This is the file 'lambda'.\n" },
        { "A/B/E",       0 },
        { "A/B/E/alpha", "This is the file 'alpha'.\n" },
        { "A/B/E/beta",  "This is the file 'beta'.\n" },
        { "A/B/F",       0 },
        { "A/C",         0 },
        { "A/D",         0 },
        { "A/D/gamma",   "This is the file 'gamma'.\n" },
        { "A/D/G",       0 },
        { "A/D/G/pi",    "This is the file 'pi'.\n" },
        { "A/D/G/rho",   "This is the file 'rho'.\n" },
        { "A/D/G/tau",   "This is the file 'tau'.\n" },
        { "A/D/H",       0 },
        { "A/D/H/chi",   "This is the file 'chi'.\n" },
        { "A/D/H/psi",   "This is the file 'psi'.\n" },
        { "A/D/H/omega", "This is the file 'omega'.\n" }
      };

      SVN_ERR(svn_fs_revision_root(&root, fs, 6, pool));
      SVN_ERR(svn_test__validate_tree(root, expected_entries, 20, pool));
    }

    /* Validate revision 7.  */
    {
      svn_fs_root_t *root;
      static svn_test__tree_entry_t expected_entries[] = {
        /* path, contents (0 = dir) */
        { "iota",        iota_contents_7 },
        { "A",           0 },
        { "A/mu",        "This is the file 'mu'.\n" },
        { "A/B",         0 },
        { "A/B/lambda",  "This is the file 'lambda'.\n" },
        { "A/B/E",       0 },
        { "A/B/E/alpha", "This is the file 'alpha'.\n" },
        { "A/B/E/beta",  "This is the file 'beta'.\n" },
        { "A/B/F",       0 },
        { "A/C",         0 },
        { "A/D",         0 },
        { "A/D/gamma",   "This is the file 'gamma'.\n" },
        { "A/D/G",       0 },
        { "A/D/G/pi",    "This is the file 'pi'.\n" },
        { "A/D/G/rho",   "This is the file 'rho'.\n" },
        { "A/D/G/tau",   "This is the file 'tau'.\n" },
        { "A/D/H",       0 },
        { "A/D/H/chi",   "This is the file 'chi'.\n" },
        { "A/D/H/psi",   "This is the file 'psi'.\n" },
        { "A/D/H/omega", "This is the file 'omega'.\n" }
      };

      SVN_ERR(svn_fs_revision_root(&root, fs, 7, pool));
      SVN_ERR(svn_test__validate_tree(root, expected_entries, 20, pool));
    }
  }

  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}


/* For each revision R in FS, from 0 to MAX_REV, check that it
   matches the tree in EXPECTED_TREES[R].  Use POOL for any
   allocations.  This is a helper function for check_all_revisions. */
static svn_error_t *
validate_revisions(svn_fs_t *fs,
                   svn_test__tree_t *expected_trees,
                   svn_revnum_t max_rev,
                   apr_pool_t *pool)
{
  svn_fs_root_t *revision_root;
  svn_revnum_t i;
  svn_error_t *err;
  apr_pool_t *subpool = svn_pool_create(pool);

  /* Validate all revisions up to the current one. */
  for (i = 0; i <= max_rev; i++)
    {
      SVN_ERR(svn_fs_revision_root(&revision_root, fs,
                                   (svn_revnum_t)i, subpool));
      err = svn_test__validate_tree(revision_root,
                                    expected_trees[i].entries,
                                    expected_trees[i].num_entries,
                                    subpool);
      if (err)
        return svn_error_createf
          (SVN_ERR_FS_GENERAL, err,
           "Error validating revision %ld (youngest is %ld)", i, max_rev);
      svn_pool_clear(subpool);
    }

  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}


static svn_error_t *
check_all_revisions(const svn_test_opts_t *opts,
                    apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  svn_revnum_t youngest_rev;
  svn_test__tree_t expected_trees[5]; /* one tree per commit, please */
  svn_revnum_t revision_count = 0;
  apr_pool_t *subpool = svn_pool_create(pool);

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-check-all-revisions",
                              opts, pool));

  /***********************************************************************/
  /* REVISION 0 */
  /***********************************************************************/
  {
    expected_trees[revision_count].num_entries = 0;
    expected_trees[revision_count].entries = 0;
    SVN_ERR(validate_revisions(fs, expected_trees, revision_count, subpool));
    revision_count++;
  }
  svn_pool_clear(subpool);

  /* Create and commit the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  /***********************************************************************/
  /* REVISION 1 */
  /***********************************************************************/
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "iota",        "This is the file 'iota'.\n" },
      { "A",           0 },
      { "A/mu",        "This is the file 'mu'.\n" },
      { "A/B",         0 },
      { "A/B/lambda",  "This is the file 'lambda'.\n" },
      { "A/B/E",       0 },
      { "A/B/E/alpha", "This is the file 'alpha'.\n" },
      { "A/B/E/beta",  "This is the file 'beta'.\n" },
      { "A/B/F",       0 },
      { "A/C",         0 },
      { "A/D",         0 },
      { "A/D/gamma",   "This is the file 'gamma'.\n" },
      { "A/D/G",       0 },
      { "A/D/G/pi",    "This is the file 'pi'.\n" },
      { "A/D/G/rho",   "This is the file 'rho'.\n" },
      { "A/D/G/tau",   "This is the file 'tau'.\n" },
      { "A/D/H",       0 },
      { "A/D/H/chi",   "This is the file 'chi'.\n" },
      { "A/D/H/psi",   "This is the file 'psi'.\n" },
      { "A/D/H/omega", "This is the file 'omega'.\n" }
    };
    expected_trees[revision_count].entries = expected_entries;
    expected_trees[revision_count].num_entries = 20;
    SVN_ERR(validate_revisions(fs, expected_trees, revision_count, subpool));
    revision_count++;
  }
  svn_pool_clear(subpool);

  /* Make a new txn based on the youngest revision, make some changes,
     and commit those changes (which makes a new youngest
     revision). */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  {
    static svn_test__txn_script_command_t script_entries[] = {
      { 'a', "A/delta",     "This is the file 'delta'.\n" },
      { 'a', "A/epsilon",   "This is the file 'epsilon'.\n" },
      { 'a', "A/B/Z",       0 },
      { 'a', "A/B/Z/zeta",  "This is the file 'zeta'.\n" },
      { 'd', "A/C",         0 },
      { 'd', "A/mu",        "" },
      { 'd', "A/D/G/tau",   "" },
      { 'd', "A/D/H/omega", "" },
      { 'e', "iota",        "Changed file 'iota'.\n" },
      { 'e', "A/D/G/rho",   "Changed file 'rho'.\n" }
    };
    SVN_ERR(svn_test__txn_script_exec(txn_root, script_entries, 10,
                                      subpool));
  }
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  /***********************************************************************/
  /* REVISION 2 */
  /***********************************************************************/
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "iota",        "Changed file 'iota'.\n" },
      { "A",           0 },
      { "A/delta",     "This is the file 'delta'.\n" },
      { "A/epsilon",   "This is the file 'epsilon'.\n" },
      { "A/B",         0 },
      { "A/B/lambda",  "This is the file 'lambda'.\n" },
      { "A/B/E",       0 },
      { "A/B/E/alpha", "This is the file 'alpha'.\n" },
      { "A/B/E/beta",  "This is the file 'beta'.\n" },
      { "A/B/F",       0 },
      { "A/B/Z",       0 },
      { "A/B/Z/zeta",  "This is the file 'zeta'.\n" },
      { "A/D",         0 },
      { "A/D/gamma",   "This is the file 'gamma'.\n" },
      { "A/D/G",       0 },
      { "A/D/G/pi",    "This is the file 'pi'.\n" },
      { "A/D/G/rho",   "Changed file 'rho'.\n" },
      { "A/D/H",       0 },
      { "A/D/H/chi",   "This is the file 'chi'.\n" },
      { "A/D/H/psi",   "This is the file 'psi'.\n" }
    };
    expected_trees[revision_count].entries = expected_entries;
    expected_trees[revision_count].num_entries = 20;
    SVN_ERR(validate_revisions(fs, expected_trees, revision_count, subpool));
    revision_count++;
  }
  svn_pool_clear(subpool);

  /* Make a new txn based on the youngest revision, make some changes,
     and commit those changes (which makes a new youngest
     revision). */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  {
    static svn_test__txn_script_command_t script_entries[] = {
      { 'a', "A/mu",        "Re-added file 'mu'.\n" },
      { 'a', "A/D/H/omega", 0 }, /* re-add omega as directory! */
      { 'd', "iota",        "" },
      { 'e', "A/delta",     "This is the file 'delta'.\nLine 2.\n" }
    };
    SVN_ERR(svn_test__txn_script_exec(txn_root, script_entries, 4,
                                      subpool));
  }
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  /***********************************************************************/
  /* REVISION 3 */
  /***********************************************************************/
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "A",           0 },
      { "A/delta",     "This is the file 'delta'.\nLine 2.\n" },
      { "A/epsilon",   "This is the file 'epsilon'.\n" },
      { "A/mu",        "Re-added file 'mu'.\n" },
      { "A/B",         0 },
      { "A/B/lambda",  "This is the file 'lambda'.\n" },
      { "A/B/E",       0 },
      { "A/B/E/alpha", "This is the file 'alpha'.\n" },
      { "A/B/E/beta",  "This is the file 'beta'.\n" },
      { "A/B/F",       0 },
      { "A/B/Z",       0 },
      { "A/B/Z/zeta",  "This is the file 'zeta'.\n" },
      { "A/D",         0 },
      { "A/D/gamma",   "This is the file 'gamma'.\n" },
      { "A/D/G",       0 },
      { "A/D/G/pi",    "This is the file 'pi'.\n" },
      { "A/D/G/rho",   "Changed file 'rho'.\n" },
      { "A/D/H",       0 },
      { "A/D/H/chi",   "This is the file 'chi'.\n" },
      { "A/D/H/psi",   "This is the file 'psi'.\n" },
      { "A/D/H/omega", 0 }
    };
    expected_trees[revision_count].entries = expected_entries;
    expected_trees[revision_count].num_entries = 21;
    SVN_ERR(validate_revisions(fs, expected_trees, revision_count, subpool));
    revision_count++;
  }
  svn_pool_clear(subpool);

  /* Make a new txn based on the youngest revision, make some changes,
     and commit those changes (which makes a new youngest
     revision). */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  {
    static svn_test__txn_script_command_t script_entries[] = {
      { 'c', "A/D/G",        "A/D/G2" },
      { 'c', "A/epsilon",    "A/B/epsilon" },
    };
    SVN_ERR(svn_test__txn_script_exec(txn_root, script_entries, 2, subpool));
  }
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  /***********************************************************************/
  /* REVISION 4 */
  /***********************************************************************/
  {
    static svn_test__tree_entry_t expected_entries[] = {
      /* path, contents (0 = dir) */
      { "A",           0 },
      { "A/delta",     "This is the file 'delta'.\nLine 2.\n" },
      { "A/epsilon",   "This is the file 'epsilon'.\n" },
      { "A/mu",        "Re-added file 'mu'.\n" },
      { "A/B",         0 },
      { "A/B/epsilon", "This is the file 'epsilon'.\n" },
      { "A/B/lambda",  "This is the file 'lambda'.\n" },
      { "A/B/E",       0 },
      { "A/B/E/alpha", "This is the file 'alpha'.\n" },
      { "A/B/E/beta",  "This is the file 'beta'.\n" },
      { "A/B/F",       0 },
      { "A/B/Z",       0 },
      { "A/B/Z/zeta",  "This is the file 'zeta'.\n" },
      { "A/D",         0 },
      { "A/D/gamma",   "This is the file 'gamma'.\n" },
      { "A/D/G",       0 },
      { "A/D/G/pi",    "This is the file 'pi'.\n" },
      { "A/D/G/rho",   "Changed file 'rho'.\n" },
      { "A/D/G2",      0 },
      { "A/D/G2/pi",   "This is the file 'pi'.\n" },
      { "A/D/G2/rho",  "Changed file 'rho'.\n" },
      { "A/D/H",       0 },
      { "A/D/H/chi",   "This is the file 'chi'.\n" },
      { "A/D/H/psi",   "This is the file 'psi'.\n" },
      { "A/D/H/omega", 0 }
    };
    expected_trees[revision_count].entries = expected_entries;
    expected_trees[revision_count].num_entries = 25;
    SVN_ERR(validate_revisions(fs, expected_trees, revision_count, subpool));
    revision_count++;
  }
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


/* Helper function for large_file_integrity().  Given a ROOT and PATH
   to a file, set *CHECKSUM to the checksum of kind CHECKSUM_KIND for the
   contents of the file. */
static svn_error_t *
get_file_checksum(svn_checksum_t **checksum,
                  svn_checksum_kind_t checksum_kind,
                  svn_fs_root_t *root,
                  const char *path,
                  apr_pool_t *pool)
{
  svn_stream_t *stream;
  svn_stream_t *checksum_stream;

  /* Get a stream for the file contents. */
  SVN_ERR(svn_fs_file_contents(&stream, root, path, pool));

  /* Get a checksummed stream for the contents. */
  checksum_stream = svn_stream_checksummed2(stream, checksum, NULL,
                                            checksum_kind, TRUE, pool);

  /* Close the stream, forcing a complete read and copy the digest. */
  SVN_ERR(svn_stream_close(checksum_stream));

  return SVN_NO_ERROR;
}


/* Return a pseudo-random number in the range [0,SCALAR) i.e. return
   a number N such that 0 <= N < SCALAR */
static int my_rand(int scalar, apr_uint32_t *seed)
{
  static const apr_uint32_t TEST_RAND_MAX = 0xffffffffUL;
  /* Assumes TEST_RAND_MAX+1 can be exactly represented in a double */
  return (int)(((double)svn_test_rand(seed)
                / ((double)TEST_RAND_MAX+1.0))
               * (double)scalar);
}


/* Put pseudo-random bytes in buffer BUF (which is LEN bytes long).
   If FULL is TRUE, simply replace every byte in BUF with a
   pseudo-random byte, else, replace a pseudo-random collection of
   bytes with pseudo-random data. */
static void
random_data_to_buffer(char *buf,
                      apr_size_t buf_len,
                      svn_boolean_t full,
                      apr_uint32_t *seed)
{
  apr_size_t i;
  apr_size_t num_bytes;
  apr_size_t offset;

  int ds_off = 0;
  const char *dataset = "0123456789";
  int dataset_size = strlen(dataset);

  if (full)
    {
      for (i = 0; i < buf_len; i++)
        {
          ds_off = my_rand(dataset_size, seed);
          buf[i] = dataset[ds_off];
        }

      return;
    }

  num_bytes = my_rand(buf_len / 100, seed) + 1;
  for (i = 0; i < num_bytes; i++)
    {
      offset = my_rand(buf_len - 1, seed);
      ds_off = my_rand(dataset_size, seed);
      buf[offset] = dataset[ds_off];
    }

  return;
}


static svn_error_t *
file_integrity_helper(apr_size_t filesize, apr_uint32_t *seed,
                      const svn_test_opts_t *opts, const char *fs_name,
                      apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *rev_root;
  svn_revnum_t youngest_rev = 0;
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_string_t contents;
  char *content_buffer;
  svn_checksum_t *checksum;
  svn_checksum_kind_t checksum_kind = svn_checksum_md5;
  svn_checksum_t *checksum_list[100];
  svn_txdelta_window_handler_t wh_func;
  void *wh_baton;
  svn_revnum_t j;

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_fs(&fs, fs_name, opts, pool));

  /* Set up our file contents string buffer. */
  content_buffer = apr_palloc(pool, filesize);

  contents.data = content_buffer;
  contents.len = filesize;

  /* THE PLAN:

     The plan here is simple.  We have a very large file (FILESIZE
     bytes) that we initialize with pseudo-random data and commit.
     Then we make pseudo-random modifications to that file's contents,
     committing after each mod.  Prior to each commit, we generate an
     MD5 checksum for the contents of the file, storing each of those
     checksums in an array.  After we've made a whole bunch of edits
     and commits, we'll re-check that file's contents as of each
     revision in the repository, recalculate a checksum for those
     contents, and make sure the "before" and "after" checksums
     match.  */

  /* Create a big, ugly, pseudo-random-filled file and commit it.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_make_file(txn_root, "bigfile", subpool));
  random_data_to_buffer(content_buffer, filesize, TRUE, seed);
  SVN_ERR(svn_checksum(&checksum, checksum_kind, contents.data, contents.len,
                       pool));
  SVN_ERR(svn_fs_apply_textdelta
          (&wh_func, &wh_baton, txn_root, "bigfile", NULL, NULL, subpool));
  SVN_ERR(svn_txdelta_send_string(&contents, wh_func, wh_baton, subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  SVN_ERR(svn_fs_deltify_revision(fs, youngest_rev, subpool));
  checksum_list[youngest_rev] = checksum;
  svn_pool_clear(subpool);

  /* Now, let's make some edits to the beginning of our file, and
     commit those. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  random_data_to_buffer(content_buffer, 20, TRUE, seed);
  SVN_ERR(svn_checksum(&checksum, checksum_kind, contents.data, contents.len,
                       pool));
  SVN_ERR(svn_fs_apply_textdelta
          (&wh_func, &wh_baton, txn_root, "bigfile", NULL, NULL, subpool));
  SVN_ERR(svn_txdelta_send_string(&contents, wh_func, wh_baton, subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  SVN_ERR(svn_fs_deltify_revision(fs, youngest_rev, subpool));
  checksum_list[youngest_rev] = checksum;
  svn_pool_clear(subpool);

  /* Now, let's make some edits to the end of our file. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  random_data_to_buffer(content_buffer + (filesize - 20), 20, TRUE, seed);
  SVN_ERR(svn_checksum(&checksum, checksum_kind, contents.data, contents.len,
                       pool));
  SVN_ERR(svn_fs_apply_textdelta
          (&wh_func, &wh_baton, txn_root, "bigfile", NULL, NULL, subpool));
  SVN_ERR(svn_txdelta_send_string(&contents, wh_func, wh_baton, subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  SVN_ERR(svn_fs_deltify_revision(fs, youngest_rev, subpool));
  checksum_list[youngest_rev] = checksum;
  svn_pool_clear(subpool);

  /* How about some edits to both the beginning and the end of the
     file? */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  random_data_to_buffer(content_buffer, 20, TRUE, seed);
  random_data_to_buffer(content_buffer + (filesize - 20), 20, TRUE, seed);
  SVN_ERR(svn_checksum(&checksum, checksum_kind, contents.data, contents.len,
                       pool));
  SVN_ERR(svn_fs_apply_textdelta
          (&wh_func, &wh_baton, txn_root, "bigfile", NULL, NULL, subpool));
  SVN_ERR(svn_txdelta_send_string(&contents, wh_func, wh_baton, subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  SVN_ERR(svn_fs_deltify_revision(fs, youngest_rev, subpool));
  checksum_list[youngest_rev] = checksum;
  svn_pool_clear(subpool);

  /* Alright, now we're just going to go crazy.  Let's make many more
     edits -- pseudo-random numbers and offsets of bytes changed to
     more pseudo-random values.  */
  for (j = youngest_rev; j < 30; j = youngest_rev)
    {
      SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
      SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
      random_data_to_buffer(content_buffer, filesize, FALSE, seed);
      SVN_ERR(svn_checksum(&checksum, checksum_kind, contents.data,
                           contents.len, pool));
      SVN_ERR(svn_fs_apply_textdelta(&wh_func, &wh_baton, txn_root,
                                     "bigfile", NULL, NULL, subpool));
      SVN_ERR(svn_txdelta_send_string
              (&contents, wh_func, wh_baton, subpool));
      SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
      SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
      SVN_ERR(svn_fs_deltify_revision(fs, youngest_rev, subpool));
      checksum_list[youngest_rev] = checksum;
      svn_pool_clear(subpool);
    }

  /* Now, calculate an MD5 digest for the contents of our big ugly
     file in each revision currently in existence, and make the sure
     the checksum matches the checksum of the data prior to its
     commit. */
  for (j = youngest_rev; j > 0; j--)
    {
      SVN_ERR(svn_fs_revision_root(&rev_root, fs, j, subpool));
      SVN_ERR(get_file_checksum(&checksum, checksum_kind, rev_root, "bigfile",
                                subpool));
      if (!svn_checksum_match(checksum, checksum_list[j]))
        return svn_error_createf
          (SVN_ERR_FS_GENERAL, NULL,
           "verify-checksum: checksum mismatch, revision %ld:\n"
           "   expected:  %s\n"
           "     actual:  %s\n", j,
        svn_checksum_to_cstring(checksum_list[j], pool),
        svn_checksum_to_cstring(checksum, pool));

      svn_pool_clear(subpool);
    }

  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}


static svn_error_t *
small_file_integrity(const svn_test_opts_t *opts,
                     apr_pool_t *pool)
{
  apr_uint32_t seed = (apr_uint32_t) apr_time_now();

  /* Just use a really small file size... */
  return file_integrity_helper(20, &seed, opts,
                               "test-repo-small-file-integrity", pool);
}


static svn_error_t *
medium_file_integrity(const svn_test_opts_t *opts,
                      apr_pool_t *pool)
{
  apr_uint32_t seed = (apr_uint32_t) apr_time_now();

  /* Being no larger than the standard delta window size affects
     deltification internally, so test that. */
  return file_integrity_helper(SVN_DELTA_WINDOW_SIZE, &seed, opts,
                               "test-repo-medium-file-integrity", pool);
}


static svn_error_t *
large_file_integrity(const svn_test_opts_t *opts,
                     apr_pool_t *pool)
{
  apr_uint32_t seed = (apr_uint32_t) apr_time_now();

  /* Being larger than the standard delta window size affects
     deltification internally, so test that. */
  return file_integrity_helper(SVN_DELTA_WINDOW_SIZE + 1, &seed, opts,
                               "test-repo-large-file-integrity", pool);
}


static svn_error_t *
check_root_revision(const svn_test_opts_t *opts,
                    apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *rev_root;
  svn_revnum_t youngest_rev, test_rev;
  apr_pool_t *subpool = svn_pool_create(pool);
  int i;

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-check-root-revision",
                              opts, pool));

  /* Create and commit the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  /* Root node's revision should be the same as YOUNGEST_REV. */
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_node_created_rev(&test_rev, rev_root, "", subpool));
  if (test_rev != youngest_rev)
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       "Root node in revision %ld has unexpected stored revision %ld",
       youngest_rev, test_rev);
  svn_pool_clear(subpool);

  for (i = 0; i < 10; i++)
    {
      /* Create and commit the greek tree. */
      SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
      SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
      SVN_ERR(svn_test__set_file_contents
              (txn_root, "iota",
               apr_psprintf(subpool, "iota version %d", i + 2), subpool));

      SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
      SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

      /* Root node's revision should be the same as YOUNGEST_REV. */
      SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, subpool));
      SVN_ERR(svn_fs_node_created_rev(&test_rev, rev_root, "", subpool));
      if (test_rev != youngest_rev)
        return svn_error_createf
          (SVN_ERR_FS_GENERAL, NULL,
           "Root node in revision %ld has unexpected stored revision %ld",
           youngest_rev, test_rev);
      svn_pool_clear(subpool);
    }

  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}


struct node_created_rev_args {
  const char *path;
  svn_revnum_t rev;
};


static svn_error_t *
verify_path_revs(svn_fs_root_t *root,
                 struct node_created_rev_args *args,
                 int num_path_revs,
                 apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  int i;
  svn_revnum_t rev;

  for (i = 0; i < num_path_revs; i++)
    {
      svn_pool_clear(subpool);
      SVN_ERR(svn_fs_node_created_rev(&rev, root, args[i].path, subpool));
      if (rev != args[i].rev)
        return svn_error_createf
          (SVN_ERR_FS_GENERAL, NULL,
           "verify_path_revs: '%s' has created rev '%ld' "
           "(expected '%ld')",
           args[i].path, rev, args[i].rev);
    }

  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}


static svn_error_t *
test_node_created_rev(const svn_test_opts_t *opts,
                      apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *rev_root;
  svn_revnum_t youngest_rev = 0;
  int i;
  struct node_created_rev_args path_revs[21];
  const char *greek_paths[21] = {
    /*  0 */ "",
    /*  1 */ "iota",
    /*  2 */ "A",
    /*  3 */ "A/mu",
    /*  4 */ "A/B",
    /*  5 */ "A/B/lambda",
    /*  6 */ "A/B/E",
    /*  7 */ "A/B/E/alpha",
    /*  8 */ "A/B/E/beta",
    /*  9 */ "A/B/F",
    /* 10 */ "A/C",
    /* 11 */ "A/D",
    /* 12 */ "A/D/gamma",
    /* 13 */ "A/D/G",
    /* 14 */ "A/D/G/pi",
    /* 15 */ "A/D/G/rho",
    /* 16 */ "A/D/G/tau",
    /* 17 */ "A/D/H",
    /* 18 */ "A/D/H/chi",
    /* 19 */ "A/D/H/psi",
    /* 20 */ "A/D/H/omega",
  };

  /* Initialize the paths in our args list. */
  for (i = 0; i < 20; i++)
    path_revs[i].path = greek_paths[i];

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-node-created-rev",
                              opts, pool));

  /* Created the greek tree in revision 1. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));

  /* Now, prior to committing, all these nodes should have an invalid
     created rev.  After all, the rev has been created yet.  Verify
     this. */
  for (i = 0; i < 20; i++)
    path_revs[i].rev = SVN_INVALID_REVNUM;
  SVN_ERR(verify_path_revs(txn_root, path_revs, 20, subpool));

  /* Now commit the transaction. */
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  /* Now, we have a new revision, and all paths in it should have a
     created rev of 1.  Verify this. */
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, subpool));
  for (i = 0; i < 20; i++)
    path_revs[i].rev = 1;
  SVN_ERR(verify_path_revs(rev_root, path_revs, 20, subpool));

  /*** Let's make some changes/commits here and there, and make sure
       we can keep this whole created rev thing in good standing.  The
       general rule here is that prior to commit, mutable things have
       an invalid created rev, immutable things have their original
       created rev.  After the commit, those things which had invalid
       created revs in the transaction now have the youngest revision
       as their created rev.

       ### NOTE: Bubble-up currently affect the created revisions for
       directory nodes.  I'm not sure if this is the behavior we've
       settled on as desired.
  */

  /*** clear the per-commit pool */
  svn_pool_clear(subpool);
  /* begin a new transaction */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  /* The created revs on a txn root should be the same as on the rev
     root it came from, if we haven't made changes yet.  (See issue
     #2608.) */
  SVN_ERR(verify_path_revs(txn_root, path_revs, 20, subpool));
  /* make mods */
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "iota", "pointless mod here", subpool));
  /* verify created revs */
  path_revs[0].rev = SVN_INVALID_REVNUM; /* (root) */
  path_revs[1].rev = SVN_INVALID_REVNUM; /* iota */
  SVN_ERR(verify_path_revs(txn_root, path_revs, 20, subpool));
  /* commit transaction */
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  /* get a revision root for the new revision */
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, subpool));
  /* verify created revs */
  path_revs[0].rev = 2; /* (root) */
  path_revs[1].rev = 2; /* iota */
  SVN_ERR(verify_path_revs(rev_root, path_revs, 20, subpool));

  /*** clear the per-commit pool */
  svn_pool_clear(subpool);
  /* begin a new transaction */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  /* make mods */
  SVN_ERR(svn_test__set_file_contents
          (txn_root, "A/D/H/omega", "pointless mod here", subpool));
  /* verify created revs */
  path_revs[0].rev  = SVN_INVALID_REVNUM; /* (root) */
  path_revs[2].rev  = SVN_INVALID_REVNUM; /* A */
  path_revs[11].rev = SVN_INVALID_REVNUM; /* D */
  path_revs[17].rev = SVN_INVALID_REVNUM; /* H */
  path_revs[20].rev = SVN_INVALID_REVNUM; /* omega */
  SVN_ERR(verify_path_revs(txn_root, path_revs, 20, subpool));
  /* commit transaction */
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  /* get a revision root for the new revision */
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, subpool));
  /* verify created revs */
  path_revs[0].rev  = 3; /* (root) */
  path_revs[2].rev  = 3; /* A */
  path_revs[11].rev = 3; /* D */
  path_revs[17].rev = 3; /* H */
  path_revs[20].rev = 3; /* omega */
  SVN_ERR(verify_path_revs(rev_root, path_revs, 20, subpool));

  /* Destroy the per-commit subpool. */
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


static svn_error_t *
check_related(const svn_test_opts_t *opts,
              apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *rev_root;
  svn_revnum_t youngest_rev = 0;

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-check-related",
                              opts, pool));

  /*** Step I: Build up some state in our repository through a series
       of commits */

  /* Using files because bubble-up complicates the testing.  However,
     the algorithm itself is ambivalent about what type of node is
     being examined.

     - New files show up in this order (through time): A,B,C,D,E,F
     - Number following filename is the revision.
     - Vertical motion shows revision history
     - Horizontal motion show copy history.

     A1---------C4         E7
     |          |          |
     A2         C5         E8---F9
     |          |               |
     A3---B4    C6              F10
     |    |
     A4   B5----------D6
          |           |
          B6          D7
  */
  /* Revision 1 */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_make_file(txn_root, "A", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A", "1", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);
  /* Revision 2 */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A", "2", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);
  /* Revision 3 */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A", "3", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);
  /* Revision 4 */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A", "4", subpool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, 3, subpool));
  SVN_ERR(svn_fs_copy(rev_root, "A", txn_root, "B", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "B", "4", subpool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, 1, subpool));
  SVN_ERR(svn_fs_copy(rev_root, "A", txn_root, "C", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "C", "4", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);
  /* Revision 5 */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "B", "5", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "C", "5", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);
  /* Revision 6 */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "B", "6", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "C", "6", subpool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, 5, subpool));
  SVN_ERR(svn_fs_copy(rev_root, "B", txn_root, "D", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "D", "5", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);
  /* Revision 7 */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "D", "7", subpool));
  SVN_ERR(svn_fs_make_file(txn_root, "E", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "E", "7", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);
  /* Revision 8 */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "E", "8", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);
  /* Revision 9 */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, 8, subpool));
  SVN_ERR(svn_fs_copy(rev_root, "E", txn_root, "F", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "F", "9", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);
  /* Revision 10 */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "F", "10", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /*** Step II: Exhaustively verify relationship between all nodes in
       existence. */
  {
    int i, j;

    struct path_rev_t
    {
      const char *path;
      svn_revnum_t rev;
    };

    /* Our 16 existing files/revisions. */
    struct path_rev_t path_revs[16] = {
      { "A", 1 }, { "A", 2 }, { "A", 3 }, { "A", 4 },
      { "B", 4 }, { "B", 5 }, { "B", 6 }, { "C", 4 },
      { "C", 5 }, { "C", 6 }, { "D", 6 }, { "D", 7 },
      { "E", 7 }, { "E", 8 }, { "F", 9 }, { "F", 10 }
    };

    int related_matrix[16][16] = {
      /* A1 ... F10 across the top here*/
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* A1 */
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* A2 */
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* A3 */
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* A4 */
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* B4 */
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* B5 */
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* B6 */
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* C4 */
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* C5 */
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* C6 */
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* D6 */
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, /* D7 */
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 }, /* E7 */
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 }, /* E8 */
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 }, /* F9 */
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 }  /* F10 */
    };

    /* Here's the fun part.  Running the tests. */
    for (i = 0; i < 16; i++)
      {
        for (j = 0; j < 16; j++)
          {
            struct path_rev_t pr1 = path_revs[i];
            struct path_rev_t pr2 = path_revs[j];
            const svn_fs_id_t *id1, *id2;
            int related = 0;

            /* Get the ID for the first path/revision combination. */
            SVN_ERR(svn_fs_revision_root(&rev_root, fs, pr1.rev, pool));
            SVN_ERR(svn_fs_node_id(&id1, rev_root, pr1.path, pool));

            /* Get the ID for the second path/revision combination. */
            SVN_ERR(svn_fs_revision_root(&rev_root, fs, pr2.rev, pool));
            SVN_ERR(svn_fs_node_id(&id2, rev_root, pr2.path, pool));

            /* <exciting> Now, run the relationship check! </exciting> */
            related = svn_fs_check_related(id1, id2) ? 1 : 0;
            if (related == related_matrix[i][j])
              {
                /* xlnt! */
              }
            else if (related && (! related_matrix[i][j]))
              {
                return svn_error_createf
                  (SVN_ERR_TEST_FAILED, NULL,
                   "expected '%s:%d' to be related to '%s:%d'; it was not",
                   pr1.path, (int)pr1.rev, pr2.path, (int)pr2.rev);
              }
            else if ((! related) && related_matrix[i][j])
              {
                return svn_error_createf
                  (SVN_ERR_TEST_FAILED, NULL,
                   "expected '%s:%d' to not be related to '%s:%d'; it was",
                   pr1.path, (int)pr1.rev, pr2.path, (int)pr2.rev);
              }

            svn_pool_clear(subpool);
          } /* for ... */
      } /* for ... */
  }

  /* Destroy the subpool. */
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


static svn_error_t *
branch_test(const svn_test_opts_t *opts,
            apr_pool_t *pool)
{
  apr_pool_t *spool = svn_pool_create(pool);
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *rev_root;
  svn_revnum_t youngest_rev = 0;

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-branch-test",
                              opts, pool));

  /*** Revision 1:  Create the greek tree in revision.  ***/
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /*** Revision 2:  Copy A/D/G/rho to A/D/G/rho2.  ***/
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_copy(rev_root, "A/D/G/rho", txn_root, "A/D/G/rho2", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /*** Revision 3:  Copy A/D/G to A/D/G2.  ***/
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_copy(rev_root, "A/D/G", txn_root, "A/D/G2", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /*** Revision 4:  Copy A/D to A/D2.  ***/
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_copy(rev_root, "A/D", txn_root, "A/D2", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /*** Revision 5:  Edit all the rho's! ***/
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/G/rho",
                                      "Edited text.", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/G/rho2",
                                      "Edited text.", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/G2/rho",
                                      "Edited text.", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/G2/rho2",
                                      "Edited text.", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D2/G/rho",
                                      "Edited text.", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D2/G/rho2",
                                      "Edited text.", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D2/G2/rho",
                                      "Edited text.", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D2/G2/rho2",
                                      "Edited text.", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  svn_pool_destroy(spool);

  return SVN_NO_ERROR;
}


static svn_error_t *
verify_checksum(const svn_test_opts_t *opts,
                apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  svn_stringbuf_t *str;
  svn_checksum_t *expected_checksum, *actual_checksum;

  /* Write a file, compare the repository's idea of its checksum
     against our idea of its checksum.  They should be the same. */

  str = svn_stringbuf_create("My text editor charges me rent.", pool);
  SVN_ERR(svn_checksum(&expected_checksum, svn_checksum_md5, str->data,
                       str->len, pool));

  SVN_ERR(svn_test__create_fs(&fs, "test-repo-verify-checksum",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_make_file(txn_root, "fact", pool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "fact", str->data, pool));
  SVN_ERR(svn_fs_file_checksum(&actual_checksum, svn_checksum_md5, txn_root,
                               "fact", TRUE, pool));

  if (!svn_checksum_match(expected_checksum, actual_checksum))
    return svn_error_createf
      (SVN_ERR_FS_GENERAL, NULL,
       "verify-checksum: checksum mismatch:\n"
       "   expected:  %s\n"
       "     actual:  %s\n",
       svn_checksum_to_cstring(expected_checksum, pool),
       svn_checksum_to_cstring(actual_checksum, pool));

  return SVN_NO_ERROR;
}


/* Helper for closest_copy_test().  Verify that CLOSEST_PATH and the
   revision associated with CLOSEST_ROOT match the EXPECTED_PATH and
   EXPECTED_REVISION, respectively. */
static svn_error_t *
test_closest_copy_pair(svn_fs_root_t *closest_root,
                       const char *closest_path,
                       svn_revnum_t expected_revision,
                       const char *expected_path)
{
  svn_revnum_t closest_rev = SVN_INVALID_REVNUM;

  /* Callers must pass valid -- EXPECTED_PATH and EXPECTED_REVISION
     come as a both-or-nothing pair. */
  assert(((! expected_path) && (! SVN_IS_VALID_REVNUM(expected_revision)))
         || (expected_path && SVN_IS_VALID_REVNUM(expected_revision)));

  /* CLOSEST_PATH and CLOSEST_ROOT come as a both-or-nothing pair, too. */
  if (closest_path && (! closest_root))
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "got closest path but no closest root");
  if ((! closest_path) && closest_root)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "got closest root but no closest path");

  /* Now that our pairs are known sane, we can compare them. */
  if (closest_path && (! expected_path))
    return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                             "got closest path ('%s') when none expected",
                             closest_path);
  if ((! closest_path) && expected_path)
    return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                             "got no closest path; expected '%s'",
                             expected_path);
  if (closest_path && (strcmp(closest_path, expected_path) != 0))
    return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                             "got a different closest path than expected:\n"
                             "   expected:  %s\n"
                             "     actual:  %s",
                             expected_path, closest_path);
  if (closest_root)
    closest_rev = svn_fs_revision_root_revision(closest_root);
  if (closest_rev != expected_revision)
    return svn_error_createf(SVN_ERR_FS_GENERAL, NULL,
                             "got a different closest rev than expected:\n"
                             "   expected:  %ld\n"
                             "     actual:  %ld",
                             expected_revision, closest_rev);

  return SVN_NO_ERROR;
}


static svn_error_t *
closest_copy_test(const svn_test_opts_t *opts,
                  apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *rev_root, *croot;
  svn_revnum_t after_rev;
  const char *cpath;
  apr_pool_t *spool = svn_pool_create(pool);

  /* Prepare a filesystem. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-closest-copy",
                              opts, pool));

  /* In first txn, create and commit the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, spool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, spool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, after_rev, spool));

  /* Copy A to Z, and commit. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, after_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_fs_copy(rev_root, "A", txn_root, "Z", spool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, spool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, after_rev, spool));

  /* Anything under Z should have a closest copy pair of ("/Z", 2), so
     we'll pick some spots to test.  Stuff under A should have no
     relevant closest copy. */
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "Z", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, 2, "/Z"));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "Z/D/G", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, 2, "/Z"));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "Z/mu", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, 2, "/Z"));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "Z/B/E/beta", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, 2, "/Z"));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "A", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, SVN_INVALID_REVNUM, NULL));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "A/D/G", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, SVN_INVALID_REVNUM, NULL));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "A/mu", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, SVN_INVALID_REVNUM, NULL));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "A/B/E/beta", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, SVN_INVALID_REVNUM, NULL));

  /* Okay, so let's do some more stuff.  We'll edit Z/mu, copy A to
     Z2, copy A/D/H to Z2/D/H2, and edit Z2/D/H2/chi.  We'll also make
     new Z/t and Z2/D/H2/t files. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, after_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "Z/mu",
                                      "Edited text.", spool));
  SVN_ERR(svn_fs_copy(rev_root, "A", txn_root, "Z2", spool));
  SVN_ERR(svn_fs_copy(rev_root, "A/D/H", txn_root, "Z2/D/H2", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "Z2/D/H2/chi",
                                      "Edited text.", spool));
  SVN_ERR(svn_fs_make_file(txn_root, "Z/t", pool));
  SVN_ERR(svn_fs_make_file(txn_root, "Z2/D/H2/t", pool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, spool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, after_rev, spool));

  /* Okay, just for kicks, let's modify Z2/D/H2/t.  Shouldn't affect
     its closest-copy-ness, right?  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, after_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "Z2/D/H2/t",
                                      "Edited text.", spool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, spool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, after_rev, spool));

  /* Now, we expect Z2/D/H2 to have a closest copy of ("/Z2/D/H2", 3)
     because of the deepest path rule.  We expected Z2/D to have a
     closest copy of ("/Z2", 3).  Z/mu should still have a closest
     copy of ("/Z", 2).  As for the two new files (Z/t and Z2/D/H2/t),
     neither should have a closest copy. */
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "A/mu", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, SVN_INVALID_REVNUM, NULL));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "Z/mu", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, 2, "/Z"));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "Z2/D/H2", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, 3, "/Z2/D/H2"));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "Z2/D", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, 3, "/Z2"));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "Z/t", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, SVN_INVALID_REVNUM, NULL));
  SVN_ERR(svn_fs_closest_copy(&croot, &cpath, rev_root, "Z2/D/H2/t", spool));
  SVN_ERR(test_closest_copy_pair(croot, cpath, SVN_INVALID_REVNUM, NULL));

  return SVN_NO_ERROR;
}

static svn_error_t *
root_revisions(const svn_test_opts_t *opts,
               apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *rev_root;
  svn_revnum_t after_rev, fetched_rev;
  apr_pool_t *spool = svn_pool_create(pool);

  /* Prepare a filesystem. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-root-revisions",
                              opts, pool));

  /* In first txn, create and commit the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, spool));
  SVN_ERR(test_commit_txn(&after_rev, txn, NULL, spool));

  /* First, verify that a revision root based on our new revision
     reports the correct associated revision. */
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, after_rev, spool));
  fetched_rev = svn_fs_revision_root_revision(rev_root);
  if (after_rev != fetched_rev)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "expected revision '%d'; "
       "got '%d' from svn_fs_revision_root_revision(rev_root)",
       (int)after_rev, (int)fetched_rev);

  /* Then verify that we can't ask about the txn-base-rev from a
     revision root. */
  fetched_rev = svn_fs_txn_root_base_revision(rev_root);
  if (fetched_rev != SVN_INVALID_REVNUM)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "expected SVN_INVALID_REVNUM; "
       "got '%d' from svn_fs_txn_root_base_revision(rev_root)",
       (int)fetched_rev);

  /* Now, create a second txn based on AFTER_REV. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, after_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));

  /* Verify that it reports the right base revision. */
  fetched_rev = svn_fs_txn_root_base_revision(txn_root);
  if (after_rev != fetched_rev)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "expected '%d'; "
       "got '%d' from svn_fs_txn_root_base_revision(txn_root)",
       (int)after_rev, (int)fetched_rev);

  /* Then verify that we can't ask about the rev-root-rev from a
     txn root. */
  fetched_rev = svn_fs_revision_root_revision(txn_root);
  if (fetched_rev != SVN_INVALID_REVNUM)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "expected SVN_INVALID_REVNUM; "
       "got '%d' from svn_fs_revision_root_revision(txn_root)",
       (int)fetched_rev);

  return SVN_NO_ERROR;
}


static svn_error_t *
unordered_txn_dirprops(const svn_test_opts_t *opts,
                       apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn, *txn2;
  svn_fs_root_t *txn_root, *txn_root2;
  svn_string_t pval;
  svn_revnum_t new_rev, not_rev;

  /* This is a regression test for issue #2751. */

  /* Prepare a filesystem. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-unordered-txn-dirprops",
                              opts, pool));

  /* Create and commit the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(test_commit_txn(&new_rev, txn, NULL, pool));

  /* Open two transactions */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, new_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_begin_txn(&txn2, fs, new_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root2, txn2, pool));

  /* Change a child file in one. */
  SVN_ERR(svn_test__set_file_contents(txn_root, "/A/B/E/alpha",
                                      "New contents", pool));

  /* Change dir props in the other.  (We're using svn:mergeinfo
     property just to make sure special handling logic for that
     property doesn't croak.) */
  SET_STR(&pval, "/A/C:1");
  SVN_ERR(svn_fs_change_node_prop(txn_root2, "/A/B", "svn:mergeinfo",
                                  &pval, pool));

  /* Commit the second one first. */
  SVN_ERR(test_commit_txn(&new_rev, txn2, NULL, pool));

  /* Then commit the first -- but expect a conflict due to the
     propchanges made by the other txn. */
  SVN_ERR(test_commit_txn(&not_rev, txn, "/A/B", pool));
  SVN_ERR(svn_fs_abort_txn(txn, pool));

  /* Now, let's try those in reverse.  Open two transactions */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, new_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_begin_txn(&txn2, fs, new_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root2, txn2, pool));

  /* Change a child file in one. */
  SVN_ERR(svn_test__set_file_contents(txn_root, "/A/B/E/alpha",
                                      "New contents", pool));

  /* Change dir props in the other. */
  SET_STR(&pval, "/A/C:1");
  SVN_ERR(svn_fs_change_node_prop(txn_root2, "/A/B", "svn:mergeinfo",
                                  &pval, pool));

  /* Commit the first one first. */
  SVN_ERR(test_commit_txn(&new_rev, txn, NULL, pool));

  /* Then commit the second -- but expect an conflict because the
     directory wasn't up-to-date, which is required for propchanges. */
  SVN_ERR(test_commit_txn(&not_rev, txn2, "/A/B", pool));
  SVN_ERR(svn_fs_abort_txn(txn2, pool));

  return SVN_NO_ERROR;
}

static svn_error_t *
set_uuid(const svn_test_opts_t *opts,
         apr_pool_t *pool)
{
  svn_fs_t *fs;
  const char *fixed_uuid = svn_uuid_generate(pool);
  const char *fetched_uuid;

  /* Prepare a filesystem. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-set-uuid",
                              opts, pool));

  /* Set the repository UUID to something fixed. */
  SVN_ERR(svn_fs_set_uuid(fs, fixed_uuid, pool));

  /* Make sure we get back what we set. */
  SVN_ERR(svn_fs_get_uuid(fs, &fetched_uuid, pool));
  if (strcmp(fixed_uuid, fetched_uuid) != 0)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL, "expected UUID '%s'; got '%s'",
       fixed_uuid, fetched_uuid);

  /* Set the repository UUID to something new (and unknown). */
  SVN_ERR(svn_fs_set_uuid(fs, NULL, pool));

  /* Make sure we *don't* get back what we previously set (after all,
     this stuff is supposed to be universally unique!). */
  SVN_ERR(svn_fs_get_uuid(fs, &fetched_uuid, pool));
  if (strcmp(fixed_uuid, fetched_uuid) == 0)
    return svn_error_createf
      (SVN_ERR_TEST_FAILED, NULL,
       "expected something other than UUID '%s', but got that one",
       fixed_uuid);

  return SVN_NO_ERROR;
}

static svn_error_t *
node_origin_rev(const svn_test_opts_t *opts,
                apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *root;
  svn_revnum_t youngest_rev = 0;
  int i;

  struct path_rev_t {
    const char *path;
    svn_revnum_t rev;
  };

  /* Create the repository. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-node-origin-rev",
                              opts, pool));

  /* Revision 1: Create the Greek tree.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 2: Modify A/D/H/chi and A/B/E/alpha.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/H/chi", "2", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/B/E/alpha", "2", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 3: Copy A/D to A/D2, and create A/D2/floop new.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_revision_root(&root, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_copy(root, "A/D", txn_root, "A/D2", subpool));
  SVN_ERR(svn_fs_make_file(txn_root, "A/D2/floop", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 4: Modify A/D/H/chi and A/D2/H/chi.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/H/chi", "4", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D2/H/chi", "4", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 5: Delete A/D2/G, add A/B/E/alfalfa.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_delete(txn_root, "A/D2/G", subpool));
  SVN_ERR(svn_fs_make_file(txn_root, "A/B/E/alfalfa", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 6: Restore A/D2/G (from version 4).  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_revision_root(&root, fs, 4, subpool));
  SVN_ERR(svn_fs_copy(root, "A/D2/G", txn_root, "A/D2/G", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 7: Move A/D2 to A/D (replacing it), Add a new file A/D2,
     and tweak A/D/floop.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_revision_root(&root, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_delete(txn_root, "A/D", subpool));
  SVN_ERR(svn_fs_copy(root, "A/D2", txn_root, "A/D", subpool));
  SVN_ERR(svn_fs_delete(txn_root, "A/D2", subpool));
  SVN_ERR(svn_fs_make_file(txn_root, "A/D2", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/floop", "7", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Now test some origin revisions. */
  {
    struct path_rev_t pathrevs[5] = { { "A/D",             1 },
                                      { "A/D/floop",       3 },
                                      { "A/D2",            7 },
                                      { "iota",            1 },
                                      { "A/B/E/alfalfa",   5 } };

    SVN_ERR(svn_fs_revision_root(&root, fs, youngest_rev, pool));
    for (i = 0; i < (sizeof(pathrevs) / sizeof(struct path_rev_t)); i++)
      {
        struct path_rev_t path_rev = pathrevs[i];
        svn_revnum_t revision;
        SVN_ERR(svn_fs_node_origin_rev(&revision, root, path_rev.path, pool));
        if (path_rev.rev != revision)
          return svn_error_createf
            (SVN_ERR_TEST_FAILED, NULL,
             "expected origin revision of '%ld' for '%s'; got '%ld'",
             path_rev.rev, path_rev.path, revision);
      }
  }

  /* Also, we'll check a couple of queries into a transaction root. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_make_file(txn_root, "bloop", subpool));
  SVN_ERR(svn_fs_make_dir(txn_root, "A/D/blarp", subpool));

  {
    struct path_rev_t pathrevs[6] = { { "A/D",             1 },
                                      { "A/D/floop",       3 },
                                      { "bloop",          -1 },
                                      { "A/D/blarp",      -1 },
                                      { "iota",            1 },
                                      { "A/B/E/alfalfa",   5 } };

    root = txn_root;
    for (i = 0; i < (sizeof(pathrevs) / sizeof(struct path_rev_t)); i++)
      {
        struct path_rev_t path_rev = pathrevs[i];
        svn_revnum_t revision;
        SVN_ERR(svn_fs_node_origin_rev(&revision, root, path_rev.path, pool));
        if (! SVN_IS_VALID_REVNUM(revision))
          revision = -1;
        if (path_rev.rev != revision)
          return svn_error_createf
            (SVN_ERR_TEST_FAILED, NULL,
             "expected origin revision of '%ld' for '%s'; got '%ld'",
             path_rev.rev, path_rev.path, revision);
      }
  }

  return SVN_NO_ERROR;
}

/* Issue 4340, "fs layer should reject filenames with trailing \n" */
static svn_error_t *
filename_trailing_newline(const svn_test_opts_t *opts,
                          apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *root;
  svn_revnum_t youngest_rev = 0;
  svn_error_t *err;
  svn_boolean_t allow_newlines;
  
  /* Some filesystem implementations can handle newlines in filenames
   * and can be white-listed here.
   * Currently, only BDB supports \n in filenames. */
  allow_newlines = (strcmp(opts->fs_type, "bdb") == 0);

  SVN_ERR(svn_test__create_fs(&fs, "test-filename-trailing-newline",
                              opts, pool));

  /* Revision 1:  Add a directory /foo  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_make_dir(txn_root, "/foo", subpool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Attempt to copy /foo to "/bar\n". This should fail on FSFS. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_revision_root(&root, fs, youngest_rev, subpool));
  err = svn_fs_copy(root, "/foo", txn_root, "/bar\n", subpool);
  if (allow_newlines)
    SVN_TEST_ASSERT(err == SVN_NO_ERROR);
  else
    {
      SVN_TEST_ASSERT(err && err->apr_err == SVN_ERR_FS_PATH_SYNTAX);
      svn_error_clear(err);
    }

  /* Attempt to create a file /foo/baz\n. This should fail on FSFS. */
  err = svn_fs_make_file(txn_root, "/foo/baz\n", subpool);
  if (allow_newlines)
    SVN_TEST_ASSERT(err == SVN_NO_ERROR);
  else
    {
      SVN_TEST_ASSERT(err && err->apr_err == SVN_ERR_FS_PATH_SYNTAX);
      svn_error_clear(err);
    }

  return SVN_NO_ERROR;
}


/* ------------------------------------------------------------------------ */

/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_OPTS_PASS(trivial_transaction,
                       "begin a txn, check its name, then close it"),
    SVN_TEST_OPTS_PASS(reopen_trivial_transaction,
                       "open an existing transaction by name"),
    SVN_TEST_OPTS_PASS(create_file_transaction,
                       "begin a txn, get the txn root, and add a file"),
    SVN_TEST_OPTS_PASS(verify_txn_list,
                       "create 2 txns, list them, and verify the list"),
    SVN_TEST_OPTS_PASS(txn_names_are_not_reused,
                       "check that transaction names are not reused"),
    SVN_TEST_OPTS_PASS(write_and_read_file,
                       "write and read a file's contents"),
    SVN_TEST_OPTS_PASS(create_mini_tree_transaction,
                       "test basic file and subdirectory creation"),
    SVN_TEST_OPTS_PASS(create_greek_tree_transaction,
                       "make The Official Subversion Test Tree"),
    SVN_TEST_OPTS_PASS(list_directory,
                       "fill a directory, then list it"),
    SVN_TEST_OPTS_PASS(revision_props,
                       "set and get some revision properties"),
    SVN_TEST_OPTS_PASS(transaction_props,
                       "set/get txn props, commit, validate new rev props"),
    SVN_TEST_OPTS_PASS(node_props,
                       "set and get some node properties"),
    SVN_TEST_OPTS_PASS(delete_mutables,
                       "delete mutable nodes from directories"),
    SVN_TEST_OPTS_PASS(delete,
                       "delete nodes tree"),
    SVN_TEST_OPTS_PASS(fetch_youngest_rev,
                       "fetch the youngest revision from a filesystem"),
    SVN_TEST_OPTS_PASS(basic_commit,
                       "basic commit"),
    SVN_TEST_OPTS_PASS(test_tree_node_validation,
                       "testing tree validation helper"),
    SVN_TEST_OPTS_WIMP(merging_commit,
                       "merging commit",
                       "needs to be written to match new"
                       " merge() algorithm expectations"),
    SVN_TEST_OPTS_PASS(copy_test,
                       "copying and tracking copy history"),
    SVN_TEST_OPTS_PASS(commit_date,
                       "commit datestamps"),
    SVN_TEST_OPTS_PASS(check_old_revisions,
                       "check old revisions"),
    SVN_TEST_OPTS_PASS(check_all_revisions,
                       "after each commit, check all revisions"),
    SVN_TEST_OPTS_PASS(medium_file_integrity,
                       "create and modify medium file"),
    SVN_TEST_OPTS_PASS(large_file_integrity,
                       "create and modify large file"),
    SVN_TEST_OPTS_PASS(check_root_revision,
                       "ensure accurate storage of root node"),
    SVN_TEST_OPTS_PASS(test_node_created_rev,
                       "svn_fs_node_created_rev test"),
    SVN_TEST_OPTS_PASS(check_related,
                       "test svn_fs_check_related"),
    SVN_TEST_OPTS_PASS(branch_test,
                       "test complex copies (branches)"),
    SVN_TEST_OPTS_PASS(verify_checksum,
                       "test checksums"),
    SVN_TEST_OPTS_PASS(closest_copy_test,
                       "calculating closest history-affecting copies"),
    SVN_TEST_OPTS_PASS(root_revisions,
                       "svn_fs_root_t (base) revisions"),
    SVN_TEST_OPTS_PASS(unordered_txn_dirprops,
                       "test dir prop preservation in unordered txns"),
    SVN_TEST_OPTS_PASS(set_uuid,
                       "test svn_fs_set_uuid"),
    SVN_TEST_OPTS_PASS(node_origin_rev,
                       "test svn_fs_node_origin_rev"),
    SVN_TEST_OPTS_PASS(small_file_integrity,
                       "create and modify small file"),
    SVN_TEST_OPTS_PASS(filename_trailing_newline,
                       "filenames with trailing \\n might be rejected"),
    SVN_TEST_NULL
  };
