/* lock-test.c --- tests for the filesystem locking functions
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

#include <string.h>
#include <apr_pools.h>
#include <apr_time.h>

#include "../svn_test.h"

#include "svn_error.h"
#include "svn_fs.h"

#include "../svn_test_fs.h"


/*-----------------------------------------------------------------*/

/** Helper functions **/

/* Implementations of the svn_fs_get_locks_callback_t interface and
   baton, for verifying expected output from svn_fs_get_locks(). */

struct get_locks_baton_t
{
  apr_hash_t *locks;
};

static svn_error_t *
get_locks_callback(void *baton,
                   svn_lock_t *lock,
                   apr_pool_t *pool)
{
  struct get_locks_baton_t *b = baton;
  apr_pool_t *hash_pool = apr_hash_pool_get(b->locks);
  svn_string_t *lock_path = svn_string_create(lock->path, hash_pool);
  apr_hash_set(b->locks, lock_path->data, lock_path->len,
               svn_lock_dup(lock, hash_pool));
  return SVN_NO_ERROR;
}

/* A factory function. */

static struct get_locks_baton_t *
make_get_locks_baton(apr_pool_t *pool)
{
  struct get_locks_baton_t *baton = apr_pcalloc(pool, sizeof(*baton));
  baton->locks = apr_hash_make(pool);
  return baton;
}


/* And verification function(s). */

static svn_error_t *
verify_matching_lock_paths(struct get_locks_baton_t *baton,
                           const char *expected_paths[],
                           apr_size_t num_expected_paths,
                           apr_pool_t *pool)
{
  apr_size_t i;
  if (num_expected_paths != apr_hash_count(baton->locks))
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Unexpected number of locks.");
  for (i = 0; i < num_expected_paths; i++)
    {
      const char *path = expected_paths[i];
      if (! apr_hash_get(baton->locks, path, APR_HASH_KEY_STRING))
        return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                 "Missing lock for path '%s'", path);
    }
  return SVN_NO_ERROR;
}


/*-----------------------------------------------------------------*/

/** The actual lock-tests called by `make check` **/



/* Test that we can create a lock--nothing more.  */
static svn_error_t *
lock_only(const svn_test_opts_t *opts,
          apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *conflict;
  svn_revnum_t newrev;
  svn_fs_access_t *access;
  svn_lock_t *mylock;

  /* Prepare a filesystem and a new txn. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-lock-only",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree and commit it. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* We are now 'bubba'. */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));

  /* Lock /A/D/G/rho. */
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0, 0,
                      SVN_INVALID_REVNUM, FALSE, pool));

  return SVN_NO_ERROR;
}





/* Test that we can create, fetch, and destroy a lock.  It exercises
   each of the five public fs locking functions.  */
static svn_error_t *
lookup_lock_by_path(const svn_test_opts_t *opts,
                    apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *conflict;
  svn_revnum_t newrev;
  svn_fs_access_t *access;
  svn_lock_t *mylock, *somelock;

  /* Prepare a filesystem and a new txn. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-lookup-lock-by-path",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree and commit it. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* We are now 'bubba'. */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));

  /* Lock /A/D/G/rho. */
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0, 0,
                      SVN_INVALID_REVNUM, FALSE, pool));

  /* Can we look up the lock by path? */
  SVN_ERR(svn_fs_get_lock(&somelock, fs, "/A/D/G/rho", pool));
  if ((! somelock) || (strcmp(somelock->token, mylock->token) != 0))
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Couldn't look up a lock by pathname.");

  return SVN_NO_ERROR;
}

/* Test that we can create a lock outside of the fs and attach it to a
   path.  */
static svn_error_t *
attach_lock(const svn_test_opts_t *opts,
            apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *conflict;
  svn_revnum_t newrev;
  svn_fs_access_t *access;
  svn_lock_t *somelock;
  svn_lock_t *mylock;
  const char *token;

  /* Prepare a filesystem and a new txn. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-attach-lock",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree and commit it. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* We are now 'bubba'. */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));

  SVN_ERR(svn_fs_generate_lock_token(&token, fs, pool));
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", token,
                      "This is a comment.  Yay comment!", 0,
                      apr_time_now() + apr_time_from_sec(3),
                      SVN_INVALID_REVNUM, FALSE, pool));

  /* Can we look up the lock by path? */
  SVN_ERR(svn_fs_get_lock(&somelock, fs, "/A/D/G/rho", pool));
  if ((! somelock) || (strcmp(somelock->token, mylock->token) != 0))
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Couldn't look up a lock by pathname.");

  /* Unlock /A/D/G/rho, and verify that it's gone. */
  SVN_ERR(svn_fs_unlock(fs, mylock->path, mylock->token, 0, pool));
  SVN_ERR(svn_fs_get_lock(&somelock, fs, "/A/D/G/rho", pool));
  if (somelock)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Removed a lock, but it's still there.");

  return SVN_NO_ERROR;
}


/* Test that we can get all locks under a directory. */
static svn_error_t *
get_locks(const svn_test_opts_t *opts,
          apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *conflict;
  svn_revnum_t newrev;
  svn_fs_access_t *access;
  svn_lock_t *mylock;
  struct get_locks_baton_t *get_locks_baton;
  apr_size_t i, num_expected_paths;

  /* Prepare a filesystem and a new txn. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-get-locks",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree and commit it. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* We are now 'bubba'. */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));

  /* Lock our paths; verify from "/". */
  {
    static const char *expected_paths[] = {
      "/A/D/G/pi",
      "/A/D/G/rho",
      "/A/D/G/tau",
      "/A/D/H/psi",
      "/A/D/H/chi",
      "/A/D/H/omega",
      "/A/B/E/alpha",
      "/A/B/E/beta",
    };
    num_expected_paths = sizeof(expected_paths) / sizeof(const char *);
    for (i = 0; i < num_expected_paths; i++)
      {
        SVN_ERR(svn_fs_lock(&mylock, fs, expected_paths[i], NULL, "", 0, 0,
                            SVN_INVALID_REVNUM, FALSE, pool));
      }
    get_locks_baton = make_get_locks_baton(pool);
    SVN_ERR(svn_fs_get_locks(fs, "", get_locks_callback,
                             get_locks_baton, pool));
    SVN_ERR(verify_matching_lock_paths(get_locks_baton, expected_paths,
                                       num_expected_paths, pool));
  }

  /* Verify from "/A/B". */
  {
    static const char *expected_paths[] = {
      "/A/B/E/alpha",
      "/A/B/E/beta",
    };
    num_expected_paths = sizeof(expected_paths) / sizeof(const char *);
    get_locks_baton = make_get_locks_baton(pool);
    SVN_ERR(svn_fs_get_locks(fs, "A/B", get_locks_callback,
                             get_locks_baton, pool));
    SVN_ERR(verify_matching_lock_paths(get_locks_baton, expected_paths,
                                       num_expected_paths, pool));
  }

  /* Verify from "/A/D". */
  {
    static const char *expected_paths[] = {
      "/A/D/G/pi",
      "/A/D/G/rho",
      "/A/D/G/tau",
      "/A/D/H/psi",
      "/A/D/H/chi",
      "/A/D/H/omega",
    };
    num_expected_paths = sizeof(expected_paths) / sizeof(const char *);
    get_locks_baton = make_get_locks_baton(pool);
    SVN_ERR(svn_fs_get_locks(fs, "A/D", get_locks_callback,
                             get_locks_baton, pool));
    SVN_ERR(verify_matching_lock_paths(get_locks_baton, expected_paths,
                                       num_expected_paths, pool));
  }

  /* Verify from "/A/D/G". */
  {
    static const char *expected_paths[] = {
      "/A/D/G/pi",
      "/A/D/G/rho",
      "/A/D/G/tau",
    };
    num_expected_paths = sizeof(expected_paths) / sizeof(const char *);
    get_locks_baton = make_get_locks_baton(pool);
    SVN_ERR(svn_fs_get_locks(fs, "A/D/G", get_locks_callback,
                             get_locks_baton, pool));
    SVN_ERR(verify_matching_lock_paths(get_locks_baton, expected_paths,
                                       num_expected_paths, pool));
  }

  /* Verify from "/A/D/H/omega". */
  {
    static const char *expected_paths[] = {
      "/A/D/H/omega",
    };
    num_expected_paths = sizeof(expected_paths) / sizeof(const char *);
    get_locks_baton = make_get_locks_baton(pool);
    SVN_ERR(svn_fs_get_locks(fs, "A/D/H/omega", get_locks_callback,
                             get_locks_baton, pool));
    SVN_ERR(verify_matching_lock_paths(get_locks_baton, expected_paths,
                                       num_expected_paths, pool));
  }

  /* Verify from "/iota" (which wasn't locked... tricky...). */
  {
    static const char *expected_paths[] = { 0 };
    num_expected_paths = 0;
    get_locks_baton = make_get_locks_baton(pool);
    SVN_ERR(svn_fs_get_locks(fs, "iota", get_locks_callback,
                             get_locks_baton, pool));
    SVN_ERR(verify_matching_lock_paths(get_locks_baton, expected_paths,
                                       num_expected_paths, pool));
  }

  return SVN_NO_ERROR;
}


/* Test that we can create, fetch, and destroy a lock.  It exercises
   each of the five public fs locking functions.  */
static svn_error_t *
basic_lock(const svn_test_opts_t *opts,
           apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *conflict;
  svn_revnum_t newrev;
  svn_fs_access_t *access;
  svn_lock_t *mylock, *somelock;

  /* Prepare a filesystem and a new txn. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-basic-lock",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree and commit it. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* We are now 'bubba'. */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));

  /* Lock /A/D/G/rho. */
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0, 0,
                      SVN_INVALID_REVNUM, FALSE, pool));

  /* Can we look up the lock by path? */
  SVN_ERR(svn_fs_get_lock(&somelock, fs, "/A/D/G/rho", pool));
  if ((! somelock) || (strcmp(somelock->token, mylock->token) != 0))
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Couldn't look up a lock by pathname.");

  /* Unlock /A/D/G/rho, and verify that it's gone. */
  SVN_ERR(svn_fs_unlock(fs, mylock->path, mylock->token, 0, pool));
  SVN_ERR(svn_fs_get_lock(&somelock, fs, "/A/D/G/rho", pool));
  if (somelock)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Removed a lock, but it's still there.");

  return SVN_NO_ERROR;
}



/* Test that locks are enforced -- specifically that both a username
   and token are required to make use of the lock.  */
static svn_error_t *
lock_credentials(const svn_test_opts_t *opts,
                 apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *conflict;
  svn_revnum_t newrev;
  svn_fs_access_t *access;
  svn_lock_t *mylock;
  svn_error_t *err;

  /* Prepare a filesystem and a new txn. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-lock-credentials",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree and commit it. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* We are now 'bubba'. */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));

  /* Lock /A/D/G/rho. */
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0, 0,
                      SVN_INVALID_REVNUM, FALSE, pool));

  /* Push the proper lock-token into the fs access context. */
  SVN_ERR(svn_fs_access_add_lock_token(access, mylock->token));

  /* Make a new transaction and change rho. */
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, newrev, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "/A/D/G/rho",
                                      "new contents", pool));

  /* We are no longer 'bubba'.  We're nobody. */
  SVN_ERR(svn_fs_set_access(fs, NULL));

  /* Try to commit the file change.  Should fail, because we're nobody. */
  err = svn_fs_commit_txn(&conflict, &newrev, txn, pool);
  SVN_TEST_ASSERT(! SVN_IS_VALID_REVNUM(newrev));
  if (! err)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "Uhoh, able to commit locked file without any fs username.");
  svn_error_clear(err);

  /* We are now 'hortense'. */
  SVN_ERR(svn_fs_create_access(&access, "hortense", pool));
  SVN_ERR(svn_fs_set_access(fs, access));

  /* Try to commit the file change.  Should fail, because we're 'hortense'. */
  err = svn_fs_commit_txn(&conflict, &newrev, txn, pool);
  SVN_TEST_ASSERT(! SVN_IS_VALID_REVNUM(newrev));
  if (! err)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "Uhoh, able to commit locked file as non-owner.");
  svn_error_clear(err);

  /* Be 'bubba' again. */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));

  /* Try to commit the file change.  Should fail, because there's no token. */
  err = svn_fs_commit_txn(&conflict, &newrev, txn, pool);
  SVN_TEST_ASSERT(! SVN_IS_VALID_REVNUM(newrev));
  if (! err)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "Uhoh, able to commit locked file with no lock token.");
  svn_error_clear(err);

  /* Push the proper lock-token into the fs access context. */
  SVN_ERR(svn_fs_access_add_lock_token(access, mylock->token));

  /* Commit should now succeed. */
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  return SVN_NO_ERROR;
}



/* Test that locks are enforced at commit time.  Somebody might lock
   something behind your back, right before you run
   svn_fs_commit_txn().  Also, this test verifies that recursive
   lock-checks on directories is working properly. */
static svn_error_t *
final_lock_check(const svn_test_opts_t *opts,
                 apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *conflict;
  svn_revnum_t newrev;
  svn_fs_access_t *access;
  svn_lock_t *mylock;
  svn_error_t *err;

  /* Prepare a filesystem and a new txn. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-final-lock-check",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree and commit it. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* Make a new transaction and delete "/A" */
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, newrev, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_delete(txn_root, "/A", pool));

  /* Become 'bubba' and lock "/A/D/G/rho". */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0, 0,
                      SVN_INVALID_REVNUM, FALSE, pool));

  /* We are no longer 'bubba'.  We're nobody. */
  SVN_ERR(svn_fs_set_access(fs, NULL));

  /* Try to commit the transaction.  Should fail, because a child of
     the deleted directory is locked by someone else. */
  err = svn_fs_commit_txn(&conflict, &newrev, txn, pool);
  SVN_TEST_ASSERT(! SVN_IS_VALID_REVNUM(newrev));
  if (! err)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "Uhoh, able to commit dir deletion when a child is locked.");
  svn_error_clear(err);

  /* Supply correct username and token;  commit should work. */
  SVN_ERR(svn_fs_set_access(fs, access));
  SVN_ERR(svn_fs_access_add_lock_token(access, mylock->token));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  return SVN_NO_ERROR;
}



/* If a directory's child is locked by someone else, we should still
   be able to commit a propchange on the directory. */
static svn_error_t *
lock_dir_propchange(const svn_test_opts_t *opts,
                    apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *conflict;
  svn_revnum_t newrev;
  svn_fs_access_t *access;
  svn_lock_t *mylock;

  /* Prepare a filesystem and a new txn. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-lock-dir-propchange",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree and commit it. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* Become 'bubba' and lock "/A/D/G/rho". */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0, 0,
                      SVN_INVALID_REVNUM, FALSE, pool));

  /* We are no longer 'bubba'.  We're nobody. */
  SVN_ERR(svn_fs_set_access(fs, NULL));

  /* Make a new transaction and make a propchange on "/A" */
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, newrev, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_change_node_prop(txn_root, "/A",
                                  "foo", svn_string_create("bar", pool),
                                  pool));

  /* Commit should succeed;  this means we're doing a non-recursive
     lock-check on directory, rather than a recursive one.  */
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  return SVN_NO_ERROR;
}

/* Test that locks auto-expire correctly. */
static svn_error_t *
lock_expiration(const svn_test_opts_t *opts,
                apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *conflict;
  svn_revnum_t newrev;
  svn_fs_access_t *access;
  svn_lock_t *mylock;
  svn_error_t *err;
  struct get_locks_baton_t *get_locks_baton;

  /* Prepare a filesystem and a new txn. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-lock-expiration",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree and commit it. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* Make a new transaction and change rho. */
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, newrev, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "/A/D/G/rho",
                                      "new contents", pool));

  /* We are now 'bubba'. */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));

  /* Lock /A/D/G/rho, with an expiration 3 seconds from now. */
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0,
                      apr_time_now() + apr_time_from_sec(3),
                      SVN_INVALID_REVNUM, FALSE, pool));

  /* Become nobody. */
  SVN_ERR(svn_fs_set_access(fs, NULL));

  /* Try to commit.  Should fail because we're 'nobody', and the lock
     hasn't expired yet. */
  err = svn_fs_commit_txn(&conflict, &newrev, txn, pool);
  SVN_TEST_ASSERT(! SVN_IS_VALID_REVNUM(newrev));
  if (! err)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "Uhoh, able to commit a file that has a non-expired lock.");
  svn_error_clear(err);

  /* Check that the lock is there, by getting it via the paths parent. */
  {
    static const char *expected_paths [] = {
      "/A/D/G/rho"
    };
    apr_size_t num_expected_paths = (sizeof(expected_paths)
                                     / sizeof(expected_paths[0]));
    get_locks_baton = make_get_locks_baton(pool);
    SVN_ERR(svn_fs_get_locks(fs, "/A/D/G", get_locks_callback,
                             get_locks_baton, pool));
    SVN_ERR(verify_matching_lock_paths(get_locks_baton, expected_paths,
                                       num_expected_paths, pool));
  }

  /* Sleep 5 seconds, so the lock auto-expires.  Anonymous commit
     should then succeed. */
  apr_sleep(apr_time_from_sec(5));

  /* Verify that the lock auto-expired even in the recursive case. */
  {
    static const char *expected_paths [] = { 0 };
    apr_size_t num_expected_paths = 0;
    get_locks_baton = make_get_locks_baton(pool);
    SVN_ERR(svn_fs_get_locks(fs, "/A/D/G", get_locks_callback,
                             get_locks_baton, pool));
    SVN_ERR(verify_matching_lock_paths(get_locks_baton, expected_paths,
                                       num_expected_paths, pool));
  }

  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  return SVN_NO_ERROR;
}

/* Test that a lock can be broken, stolen, or refreshed */
static svn_error_t *
lock_break_steal_refresh(const svn_test_opts_t *opts,
                         apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *conflict;
  svn_revnum_t newrev;
  svn_fs_access_t *access;
  svn_lock_t *mylock, *somelock;

  /* Prepare a filesystem and a new txn. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-steal-refresh",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree and commit it. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* Become 'bubba' and lock "/A/D/G/rho". */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0, 0,
                      SVN_INVALID_REVNUM, FALSE, pool));

  /* Become 'hortense' and break bubba's lock, then verify it's gone. */
  SVN_ERR(svn_fs_create_access(&access, "hortense", pool));
  SVN_ERR(svn_fs_set_access(fs, access));
  SVN_ERR(svn_fs_unlock(fs, mylock->path, mylock->token,
                        1 /* FORCE BREAK */, pool));
  SVN_ERR(svn_fs_get_lock(&somelock, fs, "/A/D/G/rho", pool));
  if (somelock)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Tried to break a lock, but it's still there.");

  /* As hortense, create a new lock, and verify that we own it. */
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0, 0,
                      SVN_INVALID_REVNUM, FALSE, pool));
  SVN_ERR(svn_fs_get_lock(&somelock, fs, "/A/D/G/rho", pool));
  if (strcmp(somelock->owner, mylock->owner) != 0)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Made a lock, but we don't seem to own it.");

  /* As bubba, steal hortense's lock, creating a new one that expires. */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0,
                      apr_time_now() + apr_time_from_sec(300), /* 5 min. */
                      SVN_INVALID_REVNUM,
                      TRUE /* FORCE STEAL */,
                      pool));
  SVN_ERR(svn_fs_get_lock(&somelock, fs, "/A/D/G/rho", pool));
  if (strcmp(somelock->owner, mylock->owner) != 0)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Made a lock, but we don't seem to own it.");
  if (! somelock->expiration_date)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Made expiring lock, but seems not to expire.");

  /* Refresh the lock, so that it never expires. */
  SVN_ERR(svn_fs_lock(&somelock, fs, somelock->path, somelock->token,
                      somelock->comment, 0, 0,
                      SVN_INVALID_REVNUM,
                      TRUE /* FORCE STEAL */,
                      pool));
  SVN_ERR(svn_fs_get_lock(&somelock, fs, "/A/D/G/rho", pool));
  if (somelock->expiration_date)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Made non-expirirng lock, but it expires.");

  return SVN_NO_ERROR;
}


/* Test that svn_fs_lock() and svn_fs_attach_lock() can do
   out-of-dateness checks..  */
static svn_error_t *
lock_out_of_date(const svn_test_opts_t *opts,
                 apr_pool_t *pool)
{
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  const char *conflict;
  svn_revnum_t newrev;
  svn_fs_access_t *access;
  svn_lock_t *mylock;
  svn_error_t *err;

  /* Prepare a filesystem and a new txn. */
  SVN_ERR(svn_test__create_fs(&fs, "test-repo-lock-out-of-date",
                              opts, pool));
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, 0, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create the greek tree and commit it. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* Commit a small change to /A/D/G/rho, creating revision 2. */
  SVN_ERR(svn_fs_begin_txn2(&txn, fs, newrev, SVN_FS_TXN_CHECK_LOCKS, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "/A/D/G/rho",
                                      "new contents", pool));
  SVN_ERR(svn_fs_commit_txn(&conflict, &newrev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(newrev));

  /* We are now 'bubba'. */
  SVN_ERR(svn_fs_create_access(&access, "bubba", pool));
  SVN_ERR(svn_fs_set_access(fs, access));

  /* Try to lock /A/D/G/rho, but claim that we still have r1 of the file. */
  err = svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0, 0, 1, FALSE, pool);
  if (! err)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "Uhoh, able to lock an out-of-date file.");
  svn_error_clear(err);

  /* Attempt lock again, this time claiming to have r2. */
  SVN_ERR(svn_fs_lock(&mylock, fs, "/A/D/G/rho", NULL, "", 0,
                      0, 2, FALSE, pool));

  /* 'Refresh' the lock, claiming to have r1... should fail. */
  err = svn_fs_lock(&mylock, fs, mylock->path,
                    mylock->token, mylock->comment, 0,
                    apr_time_now() + apr_time_from_sec(50),
                    1,
                    TRUE /* FORCE STEAL */,
                    pool);
  if (! err)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "Uhoh, able to refresh a lock on an out-of-date file.");
  svn_error_clear(err);

  return SVN_NO_ERROR;
}



/* ------------------------------------------------------------------------ */

/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_OPTS_PASS(lock_only,
                       "lock only"),
    SVN_TEST_OPTS_PASS(lookup_lock_by_path,
                       "lookup lock by path"),
    SVN_TEST_OPTS_PASS(attach_lock,
                       "attach lock"),
    SVN_TEST_OPTS_PASS(get_locks,
                       "get locks"),
    SVN_TEST_OPTS_PASS(basic_lock,
                       "basic locking"),
    SVN_TEST_OPTS_PASS(lock_credentials,
                       "test that locking requires proper credentials"),
    SVN_TEST_OPTS_PASS(final_lock_check,
                       "test that locking is enforced in final commit step"),
    SVN_TEST_OPTS_PASS(lock_dir_propchange,
                       "dir propchange can be committed with locked child"),
    SVN_TEST_OPTS_PASS(lock_expiration,
                       "test that locks can expire"),
    SVN_TEST_OPTS_PASS(lock_break_steal_refresh,
                       "breaking, stealing, refreshing a lock"),
    SVN_TEST_OPTS_PASS(lock_out_of_date,
                       "check out-of-dateness before locking"),
    SVN_TEST_NULL
  };
