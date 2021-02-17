/* repos-test.c --- tests for the filesystem
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

#include "../svn_test.h"

#include "svn_pools.h"
#include "svn_error.h"
#include "svn_fs.h"
#include "svn_repos.h"
#include "svn_path.h"
#include "svn_delta.h"
#include "svn_config.h"
#include "svn_props.h"

#include "../svn_test_fs.h"

#include "dir-delta-editor.h"

/* Used to terminate lines in large multi-line string literals. */
#define NL APR_EOL_STR

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif


static svn_error_t *
dir_deltas(const svn_test_opts_t *opts,
           apr_pool_t *pool)
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *revision_root;
  svn_revnum_t youngest_rev;
  void *edit_baton;
  const svn_delta_editor_t *editor;
  svn_test__tree_t expected_trees[8];
  int revision_count = 0;
  int i, j;
  apr_pool_t *subpool = svn_pool_create(pool);

  /* The Test Plan

     The filesystem function svn_repos_dir_delta2 exists to drive an
     editor in such a way that given a source tree S and a target tree
     T, that editor manipulation will transform S into T, insomuch as
     directories and files, and their contents and properties, go.
     The general notion of the test plan will be to create pairs of
     trees (S, T), and an editor that edits a copy of tree S, run them
     through svn_repos_dir_delta2, and then verify that the edited copy of
     S is identical to T when it is all said and done.  */

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-repo-dir-deltas",
                                 opts, pool));
  fs = svn_repos_fs(repos);
  expected_trees[revision_count].num_entries = 0;
  expected_trees[revision_count++].entries = 0;

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
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
    SVN_ERR(svn_fs_revision_root(&revision_root, fs,
                                 youngest_rev, subpool));
    SVN_ERR(svn_test__validate_tree
            (revision_root, expected_trees[revision_count].entries,
             expected_trees[revision_count].num_entries, subpool));
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
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
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
    SVN_ERR(svn_fs_revision_root(&revision_root, fs,
                                 youngest_rev, subpool));
    SVN_ERR(svn_test__validate_tree
            (revision_root, expected_trees[revision_count].entries,
             expected_trees[revision_count].num_entries, subpool));
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
    SVN_ERR(svn_test__txn_script_exec(txn_root, script_entries, 4, subpool));
  }
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
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
    SVN_ERR(svn_fs_revision_root(&revision_root, fs,
                                 youngest_rev, subpool));
    SVN_ERR(svn_test__validate_tree
            (revision_root, expected_trees[revision_count].entries,
             expected_trees[revision_count].num_entries, subpool));
    revision_count++;
  }
  svn_pool_clear(subpool);

  /* Make a new txn based on the youngest revision, make some changes,
     and commit those changes (which makes a new youngest
     revision). */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_revision_root(&revision_root, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_copy(revision_root, "A/D/G",
                      txn_root, "A/D/G2",
                      subpool));
  SVN_ERR(svn_fs_copy(revision_root, "A/epsilon",
                      txn_root, "A/B/epsilon",
                      subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
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
    SVN_ERR(svn_fs_revision_root(&revision_root, fs,
                                 youngest_rev, pool));
    SVN_ERR(svn_test__validate_tree
            (revision_root, expected_trees[revision_count].entries,
             expected_trees[revision_count].num_entries, subpool));
    revision_count++;
  }
  svn_pool_clear(subpool);

  /* THE BIG IDEA: Now that we have a collection of revisions, let's
     first make sure that given any two revisions, we can get the
     right delta between them.  We'll do this by selecting our two
     revisions, R1 and R2, basing a transaction off R1, deltafying the
     txn with respect to R2, and then making sure our final txn looks
     exactly like R2.  This should work regardless of the
     chronological order in which R1 and R2 were created.  */
  for (i = 0; i < revision_count; i++)
    {
      for (j = 0; j < revision_count; j++)
        {
          /* Prepare a txn that will receive the changes from
             svn_repos_dir_delta2 */
          SVN_ERR(svn_fs_begin_txn(&txn, fs, i, subpool));
          SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));

          /* Get the editor that will be modifying our transaction. */
          SVN_ERR(dir_delta_get_editor(&editor,
                                       &edit_baton,
                                       fs,
                                       txn_root,
                                       "",
                                       subpool));

          /* Here's the kicker...do the directory delta. */
          SVN_ERR(svn_fs_revision_root(&revision_root, fs, j, subpool));
          SVN_ERR(svn_repos_dir_delta2(txn_root,
                                       "",
                                       "",
                                       revision_root,
                                       "",
                                       editor,
                                       edit_baton,
                                       NULL,
                                       NULL,
                                       TRUE,
                                       svn_depth_infinity,
                                       FALSE,
                                       FALSE,
                                       subpool));

          /* Hopefully at this point our transaction has been modified
             to look exactly like our latest revision.  We'll check
             that. */
          SVN_ERR(svn_test__validate_tree
                  (txn_root, expected_trees[j].entries,
                   expected_trees[j].num_entries, subpool));

          /* We don't really want to do anything with this
             transaction...so we'll abort it (good for software, bad
             bad bad for society). */
          svn_error_clear(svn_fs_abort_txn(txn, subpool));
          svn_pool_clear(subpool);
        }
    }

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


static svn_error_t *
node_tree_delete_under_copy(const svn_test_opts_t *opts,
                            apr_pool_t *pool)
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *revision_root, *revision_2_root;
  svn_revnum_t youngest_rev;
  void *edit_baton;
  const svn_delta_editor_t *editor;
  svn_repos_node_t *tree;
  apr_pool_t *subpool = svn_pool_create(pool);

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-repo-del-under-copy",
                                 opts, pool));
  fs = svn_repos_fs(repos);

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));

  /* Create and commit the greek tree. */
  SVN_ERR(svn_test__create_greek_tree(txn_root, pool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  /* Now, commit again, this time after copying a directory, and then
     deleting some paths under that directory. */
  SVN_ERR(svn_fs_revision_root(&revision_root, fs, youngest_rev, pool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, pool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, pool));
  SVN_ERR(svn_fs_copy(revision_root, "A", txn_root, "Z", pool));
  SVN_ERR(svn_fs_delete(txn_root, "Z/D/G/rho", pool));
  SVN_ERR(svn_fs_delete(txn_root, "Z/D/H", pool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, pool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  /* Now, we run the node_tree editor code, and see that a) it doesn't
     bomb out, and b) that our nodes are all good. */
  SVN_ERR(svn_fs_revision_root(&revision_2_root, fs, youngest_rev, pool));
  SVN_ERR(svn_repos_node_editor(&editor, &edit_baton, repos,
                                revision_root, revision_2_root,
                                pool, subpool));
  SVN_ERR(svn_repos_replay2(revision_2_root, "", SVN_INVALID_REVNUM, FALSE,
                            editor, edit_baton, NULL, NULL, subpool));

  /* Get the root of the generated tree, and cleanup our mess. */
  tree = svn_repos_node_from_baton(edit_baton);
  svn_pool_destroy(subpool);

  /* See that we got what we expected (fortunately, svn_repos_replay
     drivers editor paths in a predictable fashion!). */

  if (! (tree /* / */
         && tree->child /* /Z */
         && tree->child->child /* /Z/D */
         && tree->child->child->child /* /Z/D/G */
         && tree->child->child->child->child /* /Z/D/G/rho */
         && tree->child->child->child->sibling)) /* /Z/D/H */
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Generated node tree is bogus.");

  if (! ((strcmp(tree->name, "") == 0)
         && (strcmp(tree->child->name, "Z") == 0)
         && (strcmp(tree->child->child->name, "D") == 0)
         && (strcmp(tree->child->child->child->name, "G") == 0)
         && ((strcmp(tree->child->child->child->child->name, "rho") == 0)
             && (tree->child->child->child->child->kind == svn_node_file)
             && (tree->child->child->child->child->action == 'D'))
         && ((strcmp(tree->child->child->child->sibling->name, "H") == 0)
             && (tree->child->child->child->sibling->kind == svn_node_dir)
             && (tree->child->child->child->sibling->action == 'D'))))
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Generated node tree is bogus.");

  return SVN_NO_ERROR;
}


/* Helper for revisions_changed(). */
static const char *
print_chrevs(const apr_array_header_t *revs_got,
             int num_revs_expected,
             const svn_revnum_t *revs_expected,
             apr_pool_t *pool)
{
  int i;
  const char *outstr;
  svn_revnum_t rev;

  outstr = apr_psprintf(pool, "Got: { ");
  if (revs_got)
    {
      for (i = 0; i < revs_got->nelts; i++)
        {
          rev = APR_ARRAY_IDX(revs_got, i, svn_revnum_t);
          outstr = apr_pstrcat(pool,
                               outstr,
                               apr_psprintf(pool, "%ld ", rev),
                               (char *)NULL);
        }
    }
  outstr = apr_pstrcat(pool, outstr, "}  Expected: { ", (char *)NULL);
  for (i = 0; i < num_revs_expected; i++)
    {
      outstr = apr_pstrcat(pool,
                           outstr,
                           apr_psprintf(pool, "%ld ",
                                        revs_expected[i]),
                           (char *)NULL);
    }
  return apr_pstrcat(pool, outstr, "}", (char *)NULL);
}


/* Implements svn_repos_history_func_t interface.  Accumulate history
   revisions the apr_array_header_t * which is the BATON. */
static svn_error_t *
history_to_revs_array(void *baton,
                      const char *path,
                      svn_revnum_t revision,
                      apr_pool_t *pool)
{
  apr_array_header_t *revs_array = baton;
  APR_ARRAY_PUSH(revs_array, svn_revnum_t) = revision;
  return SVN_NO_ERROR;
}

struct revisions_changed_results
{
  const char *path;
  int num_revs;
  svn_revnum_t revs_changed[11];
};


static svn_error_t *
revisions_changed(const svn_test_opts_t *opts,
                  apr_pool_t *pool)
{
  apr_pool_t *spool = svn_pool_create(pool);
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *rev_root;
  svn_revnum_t youngest_rev = 0;

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-repo-revisions-changed",
                                 opts, pool));
  fs = svn_repos_fs(repos);

  /*** Testing Algorithm ***

     1.  Create a greek tree in revision 1.
     2.  Make a series of new revisions, changing a file here and file
         there.
     3.  Loop over each path in each revision, verifying that we get
         the right revisions-changed array back from the filesystem.
  */

  /* Created the greek tree in revision 1. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /* Revision 2 - mu, alpha, omega */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/mu", "2", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/B/E/alpha", "2", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/H/omega", "2", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /* Revision 3 - iota, lambda, psi, omega */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "iota", "3", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/B/lambda", "3", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/H/psi", "3", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/H/omega", "3", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /* Revision 4 - iota, beta, gamma, pi, rho */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "iota", "4", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/B/E/beta", "4", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/gamma", "4", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/G/pi", "4", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/G/rho", "4", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /* Revision 5 - mu, alpha, tau, chi */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/mu", "5", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/B/E/alpha", "5", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/G/tau", "5", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/H/chi", "5", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /* Revision 6 - move A/D to A/Z */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_copy(rev_root, "A/D", txn_root, "A/Z", spool));
  SVN_ERR(svn_fs_delete(txn_root, "A/D", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /* Revision 7 - edit A/Z/G/pi */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/Z/G/pi", "7", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /* Revision 8 - move A/Z back to A/D, edit iota */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_copy(rev_root, "A/Z", txn_root, "A/D", spool));
  SVN_ERR(svn_fs_delete(txn_root, "A/Z", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "iota", "8", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /* Revision 9 - copy A/D/G to A/D/Q */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_fs_revision_root(&rev_root, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_copy(rev_root, "A/D/G", txn_root, "A/D/Q", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /* Revision 10 - edit A/D/Q/pi and A/D/Q/rho */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, spool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/Q/pi", "10", spool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/Q/rho", "10", spool));
  SVN_ERR(svn_fs_commit_txn(NULL, &youngest_rev, txn, spool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(spool);

  /* Now, it's time to verify our results. */
  {
    int j;
    /* Number, and list of, changed revisions for each path.  Note
       that for now, bubble-up in directories causes the directory to
       appear changed though no entries were added or removed, and no
       property mods occurred.  Also note that this matrix represents
       only the final state of the paths existing in HEAD of the
       repository.

       Notice for each revision, you can glance down that revision's
       column in this table and see all the paths modified directory or
       via bubble-up. */
    static const struct revisions_changed_results test_data[25] = {
      /* path,          num,    revisions changed... */
      { "",              11,    { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 } },
      { "iota",           4,    {        8,          4, 3,    1    } },
      { "A",             10,    { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1    } },
      { "A/mu",           3,    {                 5,       2, 1    } },
      { "A/B",            5,    {                 5, 4, 3, 2, 1    } },
      { "A/B/lambda",     2,    {                       3,    1    } },
      { "A/B/E",          4,    {                 5, 4,    2, 1    } },
      { "A/B/E/alpha",    3,    {                 5,       2, 1    } },
      { "A/B/E/beta",     2,    {                    4,       1    } },
      { "A/B/F",          1,    {                             1    } },
      { "A/C",            1,    {                             1    } },
      { "A/D",           10,    { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1    } },
      { "A/D/gamma",      4,    {        8,    6,    4,       1    } },
      { "A/D/G",          6,    {        8, 7, 6, 5, 4,       1    } },
      { "A/D/G/pi",       5,    {        8, 7, 6,    4,       1    } },
      { "A/D/G/rho",      4,    {        8,    6,    4,       1    } },
      { "A/D/G/tau",      4,    {        8,    6, 5,          1    } },
      { "A/D/Q",          8,    { 10, 9, 8, 7, 6, 5, 4,       1    } },
      { "A/D/Q/pi",       7,    { 10, 9, 8, 7, 6,    4,       1    } },
      { "A/D/Q/rho",      6,    { 10, 9, 8,    6,    4,       1    } },
      { "A/D/Q/tau",      5,    {     9, 8,    6, 5,          1    } },
      { "A/D/H",          6,    {        8,    6, 5,    3, 2, 1    } },
      { "A/D/H/chi",      4,    {        8,    6, 5,          1    } },
      { "A/D/H/psi",      4,    {        8,    6,       3,    1    } },
      { "A/D/H/omega",    5,    {        8,    6,       3, 2, 1    } }
    };

    /* Now, for each path in the revision, get its changed-revisions
       array and compare the array to the static results above.  */
    for (j = 0; j < 25; j++)
      {
        int i;
        const char *path = test_data[j].path;
        int num_revs = test_data[j].num_revs;
        const svn_revnum_t *revs_changed = test_data[j].revs_changed;
        apr_array_header_t *revs = apr_array_make(spool, 10,
                                                  sizeof(svn_revnum_t));

        SVN_ERR(svn_repos_history(fs, path, history_to_revs_array, revs,
                                  0, youngest_rev, TRUE, spool));

        /* Are we at least looking at the right number of returned
           revisions? */
        if ((! revs) || (revs->nelts != num_revs))
          return svn_error_createf
            (SVN_ERR_FS_GENERAL, NULL,
             "Changed revisions differ from expected for '%s'\n%s",
             path, print_chrevs(revs, num_revs, revs_changed, spool));

        /* Do the revisions lists match up exactly? */
        for (i = 0; i < num_revs; i++)
          {
            svn_revnum_t rev = APR_ARRAY_IDX(revs, i, svn_revnum_t);
            if (rev != revs_changed[i])
              return svn_error_createf
                (SVN_ERR_FS_GENERAL, NULL,
                 "Changed revisions differ from expected for '%s'\n%s",
                 path, print_chrevs(revs, num_revs, revs_changed, spool));
          }

        /* Clear the per-iteration subpool. */
        svn_pool_clear(spool);
      }
  }

  /* Destroy the subpool. */
  svn_pool_destroy(spool);

  return SVN_NO_ERROR;
}



struct locations_info
{
  svn_revnum_t rev;
  const char *path;
};

/* Check that LOCATIONS contain everything in INFO and nothing more. */
static svn_error_t *
check_locations_info(apr_hash_t *locations, const struct locations_info *info)
{
  unsigned int i;
  for (i = 0; info->rev != 0; ++i, ++info)
    {
      const char *p = apr_hash_get(locations, &info->rev, sizeof
                                   (svn_revnum_t));
      if (!p)
        return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                 "Missing path for revision %ld", info->rev);
      if (strcmp(p, info->path) != 0)
        return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                 "Pth mismatch for rev %ld", info->rev);
    }

  if (apr_hash_count(locations) > i)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Returned locations contain too many elements.");

  return SVN_NO_ERROR;
}

/* Check that all locations in INFO exist in REPOS for PATH and PEG_REVISION.
 */
static svn_error_t *
check_locations(svn_fs_t *fs, struct locations_info *info,
                const char *path, svn_revnum_t peg_revision,
                apr_pool_t *pool)
{
  apr_array_header_t *a = apr_array_make(pool, 0, sizeof(svn_revnum_t));
  apr_hash_t *h;
  struct locations_info *iter;

  for (iter = info; iter->rev != 0; ++iter)
    APR_ARRAY_PUSH(a, svn_revnum_t) = iter->rev;

  SVN_ERR(svn_repos_trace_node_locations(fs, &h, path, peg_revision, a,
                                         NULL, NULL, pool));
  SVN_ERR(check_locations_info(h, info));

  return SVN_NO_ERROR;
}

static svn_error_t *
node_locations(const svn_test_opts_t *opts,
               apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *root;
  svn_revnum_t youngest_rev;

  /* Create the repository with a Greek tree. */
  SVN_ERR(svn_test__create_repos(&repos, "test-repo-node-locations",
                                 opts, pool));
  fs = svn_repos_fs(repos);
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Move a file. Rev 2. */
  SVN_ERR(svn_fs_revision_root(&root, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_copy(root, "/A/mu", txn_root, "/mu.new", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  {
    struct locations_info info[] =
      {
        { 1, "/A/mu" },
        { 2, "/mu.new" },
        { 0 }
      };

    /* Test this twice, once with a leading slash, once without,
       because we know that the "without" form has caused us trouble
       in the past. */
    SVN_ERR(check_locations(fs, info, "/mu.new", 2, pool));
    SVN_ERR(check_locations(fs, info, "mu.new", 2, pool));
  }
  svn_pool_clear(subpool);

  return SVN_NO_ERROR;
}


static svn_error_t *
node_locations2(const svn_test_opts_t *opts,
                apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *root;
  svn_revnum_t youngest_rev = 0;

  /* Create the repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-repo-node-locations2",
                                 opts, pool));
  fs = svn_repos_fs(repos);

  /* Revision 1:  Add a directory /foo  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_make_dir(txn_root, "/foo", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 2: Copy /foo to /bar, and add /bar/baz  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_revision_root(&root, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_copy(root, "/foo", txn_root, "/bar", subpool));
  SVN_ERR(svn_fs_make_file(txn_root, "/bar/baz", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 3: Modify /bar/baz  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "/bar/baz", "brrt", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 4: Modify /bar/baz again  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "/bar/baz", "bzzz", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Now, check locations. */
  {
    struct locations_info info[] =
      {
        { 3, "/bar/baz" },
        { 2, "/bar/baz" },
        { 0 }
      };
    SVN_ERR(check_locations(fs, info, "/bar/baz", youngest_rev, pool));
  }

  return SVN_NO_ERROR;
}



/* Testing the reporter. */

/* Functions for an editor that will catch removal of defunct locks. */

/* The main editor baton. */
typedef struct rmlocks_baton_t {
  apr_hash_t *removed;
  apr_pool_t *pool;
} rmlocks_baton_t;

/* The file baton. */
typedef struct rmlocks_file_baton_t {
  rmlocks_baton_t *main_baton;
  const char *path;
} rmlocks_file_baton_t;

/* An svn_delta_editor_t function. */
static svn_error_t *
rmlocks_open_file(const char *path,
                  void *parent_baton,
                  svn_revnum_t base_revision,
                  apr_pool_t *file_pool,
                  void **file_baton)
{
  rmlocks_file_baton_t *fb = apr_palloc(file_pool, sizeof(*fb));
  rmlocks_baton_t *b = parent_baton;

  fb->main_baton = b;
  fb->path = apr_pstrdup(b->pool, path);

  *file_baton = fb;

  return SVN_NO_ERROR;
}

/* An svn_delta_editor_t function. */
static svn_error_t *
rmlocks_change_prop(void *file_baton,
                    const char *name,
                    const svn_string_t *value,
                    apr_pool_t *pool)
{
  rmlocks_file_baton_t *fb = file_baton;

  if (strcmp(name, SVN_PROP_ENTRY_LOCK_TOKEN) == 0)
    {
      if (value != NULL)
        return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                                "Value for lock-token property not NULL");

      /* We only want it removed once. */
      if (apr_hash_get(fb->main_baton->removed, fb->path,
                       APR_HASH_KEY_STRING) != NULL)
        return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                 "Lock token for '%s' already removed",
                                 fb->path);

      /* Mark as removed. */
      apr_hash_set(fb->main_baton->removed, fb->path, APR_HASH_KEY_STRING,
                   (void *)1);
    }

  return SVN_NO_ERROR;
}

/* An svn_delta_editor_t function. */
static svn_error_t *
rmlocks_open_root(void *edit_baton,
                  svn_revnum_t base_revision,
                  apr_pool_t *dir_pool,
                  void **root_baton)
{
  *root_baton = edit_baton;
  return SVN_NO_ERROR;
}

/* An svn_delta_editor_t function. */
static svn_error_t *
rmlocks_open_directory(const char *path,
                       void *parent_baton,
                       svn_revnum_t base_revision,
                       apr_pool_t *pool,
                       void **dir_baton)
{
  *dir_baton = parent_baton;
  return SVN_NO_ERROR;
}

/* Create an svn_delta_editor/baton, storing them in EDITOR/EDIT_BATON,
   that will store paths for which lock tokens were *REMOVED in REMOVED.
   Allocate the editor and *REMOVED in POOL. */
static svn_error_t *
create_rmlocks_editor(svn_delta_editor_t **editor,
                      void **edit_baton,
                      apr_hash_t **removed,
                      apr_pool_t *pool)
{
  rmlocks_baton_t *baton = apr_palloc(pool, sizeof(*baton));

  /* Create the editor. */
  *editor = svn_delta_default_editor(pool);
  (*editor)->open_root = rmlocks_open_root;
  (*editor)->open_directory = rmlocks_open_directory;
  (*editor)->open_file = rmlocks_open_file;
  (*editor)->change_file_prop = rmlocks_change_prop;

  /* Initialize the baton. */
  baton->removed = apr_hash_make(pool);
  baton->pool = pool;
  *edit_baton = baton;

  *removed = baton->removed;

  return SVN_NO_ERROR;
}

/* Check that HASH contains exactly the const char * entries for all entries
   in the NULL-terminated array SPEC. */
static svn_error_t *
rmlocks_check(const char **spec, apr_hash_t *hash)
{
  apr_size_t n = 0;

  for (; *spec; ++spec, ++n)
    {
      if (! apr_hash_get(hash, *spec, APR_HASH_KEY_STRING))
        return svn_error_createf
          (SVN_ERR_TEST_FAILED, NULL,
           "Lock token for '%s' should have been removed", *spec);
    }

  if (n < apr_hash_count(hash))
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Lock token for one or more paths unexpectedly "
                            "removed");
  return SVN_NO_ERROR;
}

/* Test that defunct locks are removed by the reporter. */
static svn_error_t *
rmlocks(const svn_test_opts_t *opts,
        apr_pool_t *pool)
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_revnum_t youngest_rev;
  svn_delta_editor_t *editor;
  void *edit_baton, *report_baton;
  svn_lock_t *l1, *l2, *l3, *l4;
  svn_fs_access_t *fs_access;
  apr_hash_t *removed;

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-repo-rmlocks",
                                 opts, pool));
  fs = svn_repos_fs(repos);

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  SVN_ERR(svn_fs_create_access(&fs_access, "user1", pool));
  SVN_ERR(svn_fs_set_access(fs, fs_access));

  /* Lock some files, break a lock, steal another and check that those get
     removed. */
  {
    const char *expected [] = { "A/mu", "A/D/gamma", NULL };

    SVN_ERR(svn_fs_lock(&l1, fs, "/iota", NULL, NULL, 0, 0, youngest_rev,
                        FALSE, subpool));
    SVN_ERR(svn_fs_lock(&l2, fs, "/A/mu", NULL, NULL, 0, 0, youngest_rev,
                        FALSE, subpool));
    SVN_ERR(svn_fs_lock(&l3, fs, "/A/D/gamma", NULL, NULL, 0, 0, youngest_rev,
                        FALSE, subpool));

    /* Break l2. */
    SVN_ERR(svn_fs_unlock(fs, "/A/mu", NULL, TRUE, subpool));

    /* Steal l3 from ourselves. */
    SVN_ERR(svn_fs_lock(&l4, fs, "/A/D/gamma", NULL, NULL, 0, 0, youngest_rev,
                        TRUE, subpool));

    /* Create the editor. */
    SVN_ERR(create_rmlocks_editor(&editor, &edit_baton, &removed, subpool));

    /* Report what we have. */
    SVN_ERR(svn_repos_begin_report2(&report_baton, 1, repos, "/", "", NULL,
                                    FALSE, svn_depth_infinity, FALSE, FALSE,
                                    editor, edit_baton, NULL, NULL, subpool));
    SVN_ERR(svn_repos_set_path3(report_baton, "", 1,
                                svn_depth_infinity,
                                FALSE, NULL, subpool));
    SVN_ERR(svn_repos_set_path3(report_baton, "iota", 1,
                                svn_depth_infinity,
                                FALSE, l1->token, subpool));
    SVN_ERR(svn_repos_set_path3(report_baton, "A/mu", 1,
                                svn_depth_infinity,
                                FALSE, l2->token, subpool));
    SVN_ERR(svn_repos_set_path3(report_baton, "A/D/gamma", 1,
                                svn_depth_infinity,
                                FALSE, l3->token, subpool));

    /* End the report. */
    SVN_ERR(svn_repos_finish_report(report_baton, pool));

    /* And check that the edit did what we wanted. */
    SVN_ERR(rmlocks_check(expected, removed));
  }

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}



/* Helper for the authz test.  Set *AUTHZ_P to a representation of
   AUTHZ_CONTENTS, using POOL for temporary allocation. */
static svn_error_t *
authz_get_handle(svn_authz_t **authz_p, const char *authz_contents,
                 apr_pool_t *pool)
{
  const char *authz_file_path;

  /* Create a temporary file. */
  SVN_ERR_W(svn_io_write_unique(&authz_file_path, NULL,
                                authz_contents, strlen(authz_contents),
                                svn_io_file_del_on_pool_cleanup, pool),
            "Writing temporary authz file");

  /* Read the authz configuration back and start testing. */
  SVN_ERR_W(svn_repos_authz_read(authz_p, authz_file_path, TRUE, pool),
            "Opening test authz file");

  /* Done with the file. */
  SVN_ERR_W(svn_io_remove_file(authz_file_path, pool),
            "Removing test authz file");

  return SVN_NO_ERROR;
}



/* Test that authz is giving out the right authorizations. */
static svn_error_t *
authz(apr_pool_t *pool)
{
  const char *contents;
  svn_authz_t *authz_cfg;
  svn_error_t *err;
  svn_boolean_t access_granted;
  apr_pool_t *subpool = svn_pool_create(pool);
  int i;
  /* Definition of the paths to test and expected replies for each. */
  struct
  {
    const char *path;
    const char *user;
    const svn_repos_authz_access_t required;
    const svn_boolean_t expected;
  } test_set[] = {
    /* Test that read rules are correctly used. */
    { "/A", NULL, svn_authz_read, TRUE },
    { "/iota", NULL, svn_authz_read, FALSE },
    /* Test that write rules are correctly used. */
    { "/A", "plato", svn_authz_write, TRUE },
    { "/A", NULL, svn_authz_write, FALSE },
    /* Test that pan-repository rules are found and used. */
    { "/A/B/lambda", "plato", svn_authz_read, TRUE },
    { "/A/B/lambda", NULL, svn_authz_read, FALSE },
    /* Test that authz uses parent path ACLs if no rule for the path
       exists. */
    { "/A/C", NULL, svn_authz_read, TRUE },
    /* Test that recursive access requests take into account the rules
       of subpaths. */
    { "/A/D", "plato", svn_authz_read | svn_authz_recursive, TRUE },
    { "/A/D", NULL, svn_authz_read | svn_authz_recursive, FALSE },
    /* Test global write access lookups. */
    { NULL, "plato", svn_authz_read, TRUE },
    { NULL, NULL, svn_authz_write, FALSE },
    /* Sentinel */
    { NULL, NULL, svn_authz_none, FALSE }
  };

  /* The test logic:
   *
   * 1. Perform various access tests on a set of authz rules.  Each
   * test has a known outcome and tests different aspects of authz,
   * such as inheriting parent-path authz, pan-repository rules or
   * recursive access.  'plato' is our friendly neighborhood user with
   * more access rights than other anonymous philosophers.
   *
   * 2. Load an authz file containing a cyclic dependency in groups
   * and another containing a reference to an undefined group.  Verify
   * that svn_repos_authz_read fails to load both and returns an
   * "invalid configuration" error.
   *
   * 3. Regression test for a bug in how recursion is handled in
   * authz.  The bug was that paths not under the parent path
   * requested were being considered during the determination of
   * access rights (eg. a rule for /dir2 matched during a lookup for
   * /dir), due to incomplete tests on path relations.
   */

  /* The authz rules for the phase 1 tests. */
  contents =
    "[greek:/A]"                                                             NL
    "* = r"                                                                  NL
    "plato = w"                                                              NL
    ""                                                                       NL
    "[greek:/iota]"                                                          NL
    "* ="                                                                    NL
    ""                                                                       NL
    "[/A/B/lambda]"                                                          NL
    "plato = r"                                                              NL
    "* ="                                                                    NL
    ""                                                                       NL
    "[greek:/A/D]"                                                           NL
    "plato = r"                                                              NL
    "* = r"                                                                  NL
    ""                                                                       NL
    "[greek:/A/D/G]"                                                         NL
    "plato = r"                                                              NL
    "* ="                                                                    NL
    ""                                                                       NL
    "[greek:/A/B/E/beta]"                                                    NL
    "* ="                                                                    NL
    ""                                                                       NL
    "[/nowhere]"                                                             NL
    "nobody = r"                                                             NL
    ""                                                                       NL;

  /* Load the test authz rules. */
  SVN_ERR(authz_get_handle(&authz_cfg, contents, subpool));

  /* Loop over the test array and test each case. */
  for (i = 0; !(test_set[i].path == NULL
               && test_set[i].required == svn_authz_none); i++)
    {
      SVN_ERR(svn_repos_authz_check_access(authz_cfg, "greek",
                                           test_set[i].path,
                                           test_set[i].user,
                                           test_set[i].required,
                                           &access_granted, subpool));

      if (access_granted != test_set[i].expected)
        {
          return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                   "Authz incorrectly %s %s%s access "
                                   "to greek:%s for user %s",
                                   access_granted ?
                                   "grants" : "denies",
                                   test_set[i].required
                                   & svn_authz_recursive ?
                                   "recursive " : "",
                                   test_set[i].required
                                   & svn_authz_read ?
                                   "read" : "write",
                                   test_set[i].path,
                                   test_set[i].user ?
                                   test_set[i].user : "-");
        }
    }


  /* The authz rules for the phase 2 tests, first case (cyclic
     dependency). */
  contents =
    "[groups]"                                                               NL
    "slaves = cooks,scribes,@gladiators"                                     NL
    "gladiators = equites,thraces,@slaves"                                   NL
    ""                                                                       NL
    "[greek:/A]"                                                             NL
    "@slaves = r"                                                            NL;

  /* Load the test authz rules and check that group cycles are
     reported. */
  err = authz_get_handle(&authz_cfg, contents, subpool);
  if (!err || err->apr_err != SVN_ERR_AUTHZ_INVALID_CONFIG)
    return svn_error_createf(SVN_ERR_TEST_FAILED, err,
                             "Got %s error instead of expected "
                             "SVN_ERR_AUTHZ_INVALID_CONFIG",
                             err ? "unexpected" : "no");
  svn_error_clear(err);

  /* The authz rules for the phase 2 tests, second case (missing group
     definition). */
  contents =
    "[greek:/A]"                                                             NL
    "@senate = r"                                                            NL;

  /* Check that references to undefined groups are reported. */
  err = authz_get_handle(&authz_cfg, contents, subpool);
  if (!err || err->apr_err != SVN_ERR_AUTHZ_INVALID_CONFIG)
    return svn_error_createf(SVN_ERR_TEST_FAILED, err,
                             "Got %s error instead of expected "
                             "SVN_ERR_AUTHZ_INVALID_CONFIG",
                             err ? "unexpected" : "no");
  svn_error_clear(err);

  /* The authz rules for the phase 3 tests */
  contents =
    "[/]"                                                                    NL
    "* = rw"                                                                 NL
    ""                                                                       NL
    "[greek:/dir2/secret]"                                                   NL
    "* ="                                                                    NL;

  /* Load the test authz rules. */
  SVN_ERR(authz_get_handle(&authz_cfg, contents, subpool));

  /* Verify that the rule on /dir2/secret doesn't affect this
     request */
  SVN_ERR(svn_repos_authz_check_access(authz_cfg, "greek",
                                       "/dir", NULL,
                                       (svn_authz_read
                                        | svn_authz_recursive),
                                       &access_granted, subpool));
  if (!access_granted)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "Regression: incomplete ancestry test "
                            "for recursive access lookup.");

  /* That's a wrap! */
  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}



/* Callback for the commit editor tests that relays requests to
   authz. */
static svn_error_t *
commit_authz_cb(svn_repos_authz_access_t required,
                svn_boolean_t *allowed,
                svn_fs_root_t *root,
                const char *path,
                void *baton,
                apr_pool_t *pool)
{
  svn_authz_t *authz_file = baton;

  return svn_repos_authz_check_access(authz_file, "test", path,
                                      "plato", required, allowed,
                                      pool);
}



/* Test that the commit editor is taking authz into account
   properly */
static svn_error_t *
commit_editor_authz(const svn_test_opts_t *opts,
                    apr_pool_t *pool)
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  svn_revnum_t youngest_rev;
  void *edit_baton;
  void *root_baton, *dir_baton, *dir2_baton, *file_baton;
  svn_error_t *err;
  const svn_delta_editor_t *editor;
  svn_authz_t *authz_file;
  apr_pool_t *subpool = svn_pool_create(pool);
  const char *authz_contents;

  /* The Test Plan
   *
   * We create a greek tree repository, then create a commit editor
   * and try to perform various operations that will run into authz
   * callbacks.  Check that all operations are properly
   * authorized/denied when necessary.  We don't try to be exhaustive
   * in the kinds of authz lookups.  We just make sure that the editor
   * replies to the calls in a way that proves it is doing authz
   * lookups.
   *
   * Note that this use of the commit editor is not kosher according
   * to the generic editor API (we aren't allowed to continue editing
   * after an error, nor are we allowed to assume that errors are
   * returned by the operations which caused them).  But it should
   * work fine with this particular editor implementation.
   */

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-repo-commit-authz",
                                 opts, subpool));
  fs = svn_repos_fs(repos);

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  /* Load the authz rules for the greek tree. */
  authz_contents =
    ""                                                                       NL
    ""                                                                       NL
    "[/]"                                                                    NL
    "plato = r"                                                              NL
    ""                                                                       NL
    "[/A]"                                                                   NL
    "plato = rw"                                                             NL
    ""                                                                       NL
    "[/A/alpha]"                                                             NL
    "plato = "                                                               NL
    ""                                                                       NL
    "[/A/C]"                                                                 NL
    ""                                                                       NL
    "plato = "                                                               NL
    ""                                                                       NL
    "[/A/D]"                                                                 NL
    "plato = rw"                                                             NL
    ""                                                                       NL
    "[/A/D/G]"                                                               NL
    "plato = r"; /* No newline at end of file. */

  SVN_ERR(authz_get_handle(&authz_file, authz_contents, subpool));

  /* Create a new commit editor in which we're going to play with
     authz */
  SVN_ERR(svn_repos_get_commit_editor4(&editor, &edit_baton, repos,
                                       NULL, "file://test", "/",
                                       "plato", "test commit", NULL,
                                       NULL, commit_authz_cb, authz_file,
                                       subpool));

  /* Start fiddling.  First get the root, which is readonly.  All
     write operations fail because of the root's permissions. */
  SVN_ERR(editor->open_root(edit_baton, 1, subpool, &root_baton));

  /* Test denied file deletion. */
  err = editor->delete_entry("/iota", SVN_INVALID_REVNUM, root_baton, subpool);
  if (err == SVN_NO_ERROR || err->apr_err != SVN_ERR_AUTHZ_UNWRITABLE)
    return svn_error_createf(SVN_ERR_TEST_FAILED, err,
                             "Got %s error instead of expected "
                             "SVN_ERR_AUTHZ_UNWRITABLE",
                             err ? "unexpected" : "no");
  svn_error_clear(err);

  /* Test authorized file open. */
  SVN_ERR(editor->open_file("/iota", root_baton, SVN_INVALID_REVNUM,
                            subpool, &file_baton));

  /* Test unauthorized file prop set. */
  err = editor->change_file_prop(file_baton, "svn:test",
                                 svn_string_create("test", subpool),
                                 subpool);
  if (err == SVN_NO_ERROR || err->apr_err != SVN_ERR_AUTHZ_UNWRITABLE)
    return svn_error_createf(SVN_ERR_TEST_FAILED, err,
                             "Got %s error instead of expected "
                             "SVN_ERR_AUTHZ_UNWRITABLE",
                             err ? "unexpected" : "no");
  svn_error_clear(err);

  /* Test denied file addition. */
  err = editor->add_file("/alpha", root_baton, NULL, SVN_INVALID_REVNUM,
                         subpool, &file_baton);
  if (err == SVN_NO_ERROR || err->apr_err != SVN_ERR_AUTHZ_UNWRITABLE)
    return svn_error_createf(SVN_ERR_TEST_FAILED, err,
                             "Got %s error instead of expected "
                             "SVN_ERR_AUTHZ_UNWRITABLE",
                             err ? "unexpected" : "no");
  svn_error_clear(err);

  /* Test denied file copy. */
  err = editor->add_file("/alpha", root_baton, "file://test/A/B/lambda",
                         youngest_rev, subpool, &file_baton);
  if (err == SVN_NO_ERROR || err->apr_err != SVN_ERR_AUTHZ_UNWRITABLE)
    return svn_error_createf(SVN_ERR_TEST_FAILED, err,
                             "Got %s error instead of expected "
                             "SVN_ERR_AUTHZ_UNWRITABLE",
                             err ? "unexpected" : "no");
  svn_error_clear(err);

  /* Test denied directory addition. */
  err = editor->add_directory("/I", root_baton, NULL,
                              SVN_INVALID_REVNUM, subpool, &dir_baton);
  if (err == SVN_NO_ERROR || err->apr_err != SVN_ERR_AUTHZ_UNWRITABLE)
    return svn_error_createf(SVN_ERR_TEST_FAILED, err,
                             "Got %s error instead of expected "
                             "SVN_ERR_AUTHZ_UNWRITABLE",
                             err ? "unexpected" : "no");
  svn_error_clear(err);

  /* Test denied directory copy. */
  err = editor->add_directory("/J", root_baton, "file://test/A/D",
                              youngest_rev, subpool, &dir_baton);
  if (err == SVN_NO_ERROR || err->apr_err != SVN_ERR_AUTHZ_UNWRITABLE)
    return svn_error_createf(SVN_ERR_TEST_FAILED, err,
                             "Got %s error instead of expected "
                             "SVN_ERR_AUTHZ_UNWRITABLE",
                             err ? "unexpected" : "no");
  svn_error_clear(err);

  /* Open directory /A, to which we have read/write access. */
  SVN_ERR(editor->open_directory("/A", root_baton,
                                 SVN_INVALID_REVNUM,
                                 subpool, &dir_baton));

  /* Test denied file addition.  Denied because of a conflicting rule
     on the file path itself. */
  err = editor->add_file("/A/alpha", dir_baton, NULL,
                         SVN_INVALID_REVNUM, subpool, &file_baton);
  if (err == SVN_NO_ERROR || err->apr_err != SVN_ERR_AUTHZ_UNWRITABLE)
    return svn_error_createf(SVN_ERR_TEST_FAILED, err,
                             "Got %s error instead of expected "
                             "SVN_ERR_AUTHZ_UNWRITABLE",
                             err ? "unexpected" : "no");
  svn_error_clear(err);

  /* Test authorized file addition. */
  SVN_ERR(editor->add_file("/A/B/theta", dir_baton, NULL,
                           SVN_INVALID_REVNUM, subpool,
                           &file_baton));

  /* Test authorized file deletion. */
  SVN_ERR(editor->delete_entry("/A/mu", SVN_INVALID_REVNUM, dir_baton,
                               subpool));

  /* Test authorized directory creation. */
  SVN_ERR(editor->add_directory("/A/E", dir_baton, NULL,
                                SVN_INVALID_REVNUM, subpool,
                                &dir2_baton));

  /* Test authorized copy of a tree. */
  SVN_ERR(editor->add_directory("/A/J", dir_baton, "file://test/A/D",
                                youngest_rev, subpool,
                                &dir2_baton));

  /* Open /A/D.  This should be granted. */
  SVN_ERR(editor->open_directory("/A/D", dir_baton, SVN_INVALID_REVNUM,
                                 subpool, &dir_baton));

  /* Test denied recursive deletion. */
  err = editor->delete_entry("/A/D/G", SVN_INVALID_REVNUM, dir_baton,
                             subpool);
  if (err == SVN_NO_ERROR || err->apr_err != SVN_ERR_AUTHZ_UNWRITABLE)
    return svn_error_createf(SVN_ERR_TEST_FAILED, err,
                             "Got %s error instead of expected "
                             "SVN_ERR_AUTHZ_UNWRITABLE",
                             err ? "unexpected" : "no");
  svn_error_clear(err);

  /* Test authorized recursive deletion. */
  SVN_ERR(editor->delete_entry("/A/D/H", SVN_INVALID_REVNUM,
                               dir_baton, subpool));

  /* Test authorized propset (open the file first). */
  SVN_ERR(editor->open_file("/A/D/gamma", dir_baton, SVN_INVALID_REVNUM,
                            subpool, &file_baton));
  SVN_ERR(editor->change_file_prop(file_baton, "svn:test",
                                   svn_string_create("test", subpool),
                                   subpool));

  /* Done. */
  SVN_ERR(editor->abort_edit(edit_baton, subpool));
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

/* This implements svn_commit_callback2_t. */
static svn_error_t *
dummy_commit_cb(const svn_commit_info_t *commit_info,
                void *baton, apr_pool_t *pool)
{
  return SVN_NO_ERROR;
}

/* Test using explicit txns during a commit. */
static svn_error_t *
commit_continue_txn(const svn_test_opts_t *opts,
                    apr_pool_t *pool)
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *revision_root;
  svn_revnum_t youngest_rev;
  void *edit_baton;
  void *root_baton, *file_baton;
  const svn_delta_editor_t *editor;
  apr_pool_t *subpool = svn_pool_create(pool);
  const char *txn_name;

  /* The Test Plan
   *
   * We create a greek tree repository, then create a transaction and
   * a commit editor from that txn.  We do one change, abort the edit, reopen
   * the txn and create a new commit editor, do anyther change and commit.
   * We check that both changes were done.
   */

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-repo-commit-continue",
                                 opts, subpool));
  fs = svn_repos_fs(repos);

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_name(&txn_name, txn, subpool));
  SVN_ERR(svn_repos_get_commit_editor4(&editor, &edit_baton, repos,
                                       txn, "file://test", "/",
                                       "plato", "test commit",
                                       dummy_commit_cb, NULL, NULL, NULL,
                                       subpool));

  SVN_ERR(editor->open_root(edit_baton, 1, subpool, &root_baton));

  SVN_ERR(editor->add_file("/f1", root_baton, NULL, SVN_INVALID_REVNUM,
                           subpool, &file_baton));
  SVN_ERR(editor->close_file(file_baton, NULL, subpool));
  /* This should leave the transaction. */
  SVN_ERR(editor->abort_edit(edit_baton, subpool));

  /* Reopen the transaction. */
  SVN_ERR(svn_fs_open_txn(&txn, fs, txn_name, subpool));
  SVN_ERR(svn_repos_get_commit_editor4(&editor, &edit_baton, repos,
                                       txn, "file://test", "/",
                                       "plato", "test commit",
                                       dummy_commit_cb,
                                       NULL, NULL, NULL,
                                       subpool));

  SVN_ERR(editor->open_root(edit_baton, 1, subpool, &root_baton));

  SVN_ERR(editor->add_file("/f2", root_baton, NULL, SVN_INVALID_REVNUM,
                           subpool, &file_baton));
  SVN_ERR(editor->close_file(file_baton, NULL, subpool));

  /* Finally, commit it. */
  SVN_ERR(editor->close_edit(edit_baton, subpool));

  /* Check that the edits really happened. */
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
      { "A/D/H/omega", "This is the file 'omega'.\n" },
      { "f1",          "" },
      { "f2",          "" }
    };
    SVN_ERR(svn_fs_revision_root(&revision_root, fs,
                                 2, subpool));
    SVN_ERR(svn_test__validate_tree
            (revision_root, expected_entries,
             sizeof(expected_entries) / sizeof(expected_entries[0]),
             subpool));
  }

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


struct nls_receiver_baton
{
  int count;
  svn_location_segment_t *expected_segments;
};


static const char *
format_segment(svn_location_segment_t *segment,
               apr_pool_t *pool)
{
  return apr_psprintf(pool, "[r%ld-r%ld: /%s]",
                      segment->range_start,
                      segment->range_end,
                      segment->path ? segment->path : "(null)");
}


static svn_error_t *
nls_receiver(svn_location_segment_t *segment,
             void *baton,
             apr_pool_t *pool)
{
  struct nls_receiver_baton *b = baton;
  svn_location_segment_t *expected_segment = b->expected_segments + b->count;

  /* expected_segments->range_end can't be 0, so if we see that, it's
     our end-of-the-list sentry. */
  if (! expected_segment->range_end)
    return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                             "Got unexpected location segment: %s",
                             format_segment(segment, pool));

  if (expected_segment->range_start != segment->range_start)
    return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                             "Location segments differ\n"
                             "   Expected location segment: %s\n"
                             "     Actual location segment: %s",
                             format_segment(expected_segment, pool),
                             format_segment(segment, pool));
  b->count++;
  return SVN_NO_ERROR;
}


static svn_error_t *
check_location_segments(svn_repos_t *repos,
                        const char *path,
                        svn_revnum_t peg_rev,
                        svn_revnum_t start_rev,
                        svn_revnum_t end_rev,
                        svn_location_segment_t *expected_segments,
                        apr_pool_t *pool)
{
  struct nls_receiver_baton b;
  svn_location_segment_t *segment;

  /* Run svn_repos_node_location_segments() with a receiver that
     validates against EXPECTED_SEGMENTS.  */
  b.count = 0;
  b.expected_segments = expected_segments;
  SVN_ERR(svn_repos_node_location_segments(repos, path, peg_rev,
                                           start_rev, end_rev, nls_receiver,
                                           &b, NULL, NULL, pool));

  /* Make sure we saw all of our expected segments.  (If the
     'range_end' member of our expected_segments is 0, it's our
     end-of-the-list sentry.  Otherwise, it's some segment we expect
     to see.)  If not, raise an error.  */
  segment = expected_segments + b.count;
  if (segment->range_end)
    return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                             "Failed to get expected location segment: %s",
                             format_segment(segment, pool));
  return SVN_NO_ERROR;
}


static svn_error_t *
node_location_segments(const svn_test_opts_t *opts,
                       apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root, *root;
  svn_revnum_t youngest_rev = 0;

  /* Bail (with success) on known-untestable scenarios */
  if ((strcmp(opts->fs_type, "bdb") == 0)
      && (opts->server_minor_version == 4))
    return SVN_NO_ERROR;

  /* Create the repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-repo-node-location-segments",
                                 opts, pool));
  fs = svn_repos_fs(repos);

  /* Revision 1: Create the Greek tree.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 2: Modify A/D/H/chi and A/B/E/alpha.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/H/chi", "2", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/B/E/alpha", "2", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 3: Copy A/D to A/D2.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_revision_root(&root, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_copy(root, "A/D", txn_root, "A/D2", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 4: Modify A/D/H/chi and A/D2/H/chi.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D/H/chi", "4", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/D2/H/chi", "4", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 5: Delete A/D2/G.  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_delete(txn_root, "A/D2/G", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 6: Restore A/D2/G (from version 4).  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_revision_root(&root, fs, 4, subpool));
  SVN_ERR(svn_fs_copy(root, "A/D2/G", txn_root, "A/D2/G", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 7: Move A/D2 to A/D (replacing it).  */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_fs_revision_root(&root, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_delete(txn_root, "A/D", subpool));
  SVN_ERR(svn_fs_copy(root, "A/D2", txn_root, "A/D", subpool));
  SVN_ERR(svn_fs_delete(txn_root, "A/D2", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Check locations for /@HEAD. */
  {
    svn_location_segment_t expected_segments[] =
      {
        { 0, 7, "" },
        { 0 }
      };
    SVN_ERR(check_location_segments(repos, "",
                                    SVN_INVALID_REVNUM,
                                    SVN_INVALID_REVNUM,
                                    SVN_INVALID_REVNUM,
                                    expected_segments, pool));
  }

  /* Check locations for A/D@HEAD. */
  {
    svn_location_segment_t expected_segments[] =
      {
        { 7, 7, "A/D" },
        { 3, 6, "A/D2" },
        { 1, 2, "A/D" },
        { 0 }
      };
    SVN_ERR(check_location_segments(repos, "A/D",
                                    SVN_INVALID_REVNUM,
                                    SVN_INVALID_REVNUM,
                                    SVN_INVALID_REVNUM,
                                    expected_segments, pool));
  }

  /* Check a subset of the locations for A/D@HEAD. */
  {
    svn_location_segment_t expected_segments[] =
      {
        { 3, 5, "A/D2" },
        { 2, 2, "A/D" },
        { 0 }
      };
    SVN_ERR(check_location_segments(repos, "A/D",
                                    SVN_INVALID_REVNUM,
                                    5,
                                    2,
                                    expected_segments, pool));
  }

  /* Check a subset of locations for A/D2@5. */
  {
    svn_location_segment_t expected_segments[] =
      {
        { 3, 3, "A/D2" },
        { 2, 2, "A/D" },
        { 0 }
      };
    SVN_ERR(check_location_segments(repos, "A/D2",
                                    5,
                                    3,
                                    2,
                                    expected_segments, pool));
  }

  /* Check locations for A/D@6. */
  {
    svn_location_segment_t expected_segments[] =
      {
        { 1, 6, "A/D" },
        { 0 }
      };
    SVN_ERR(check_location_segments(repos, "A/D",
                                    6,
                                    6,
                                    SVN_INVALID_REVNUM,
                                    expected_segments, pool));
  }

  /* Check locations for A/D/G@HEAD. */
  {
    svn_location_segment_t expected_segments[] =
      {
        { 7, 7, "A/D/G" },
        { 6, 6, "A/D2/G" },
        { 5, 5, NULL },
        { 3, 4, "A/D2/G" },
        { 1, 2, "A/D2/G" },
        { 0 }
      };
    SVN_ERR(check_location_segments(repos, "A/D/G",
                                    SVN_INVALID_REVNUM,
                                    SVN_INVALID_REVNUM,
                                    SVN_INVALID_REVNUM,
                                    expected_segments, pool));
  }

  /* Check a subset of the locations for A/D/G@HEAD. */
  {
    svn_location_segment_t expected_segments[] =
      {
        { 3, 3, "A/D2/G" },
        { 2, 2, "A/D2/G" },
        { 0 }
      };
    SVN_ERR(check_location_segments(repos, "A/D/G",
                                    SVN_INVALID_REVNUM,
                                    3,
                                    2,
                                    expected_segments, pool));
  }

  return SVN_NO_ERROR;
}


/* Test that the reporter doesn't send deltas under excluded paths. */
static svn_error_t *
reporter_depth_exclude(const svn_test_opts_t *opts,
                       apr_pool_t *pool)
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_revnum_t youngest_rev;
  const svn_delta_editor_t *editor;
  void *edit_baton, *report_baton;
  svn_error_t *err;

  SVN_ERR(svn_test__create_repos(&repos, "test-repo-reporter-depth-exclude",
                                 opts, pool));
  fs = svn_repos_fs(repos);

  /* Prepare a txn to receive the greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 0, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Revision 2: make a bunch of changes */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  {
    static svn_test__txn_script_command_t script_entries[] = {
      { 'e', "iota",      "Changed file 'iota'.\n" },
      { 'e', "A/D/G/pi",  "Changed file 'pi'.\n" },
      { 'e', "A/mu",      "Changed file 'mu'.\n" },
      { 'a', "A/D/foo",    "New file 'foo'.\n" },
      { 'a', "A/B/bar",    "New file 'bar'.\n" },
      { 'd', "A/D/H",      NULL },
      { 'd', "A/B/E/beta", NULL }
    };
    SVN_ERR(svn_test__txn_script_exec(txn_root,
                                      script_entries,
                                      sizeof(script_entries)/
                                       sizeof(script_entries[0]),
                                      subpool));
  }
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));
  svn_pool_clear(subpool);

  /* Confirm the contents of r2. */
  {
    svn_fs_root_t *revision_root;
    static svn_test__tree_entry_t entries[] = {
      { "iota",        "Changed file 'iota'.\n" },
      { "A",           0 },
      { "A/mu",        "Changed file 'mu'.\n" },
      { "A/B",         0 },
      { "A/B/bar",     "New file 'bar'.\n" },
      { "A/B/lambda",  "This is the file 'lambda'.\n" },
      { "A/B/E",       0 },
      { "A/B/E/alpha", "This is the file 'alpha'.\n" },
      { "A/B/F",       0 },
      { "A/C",         0 },
      { "A/D",         0 },
      { "A/D/foo",     "New file 'foo'.\n" },
      { "A/D/gamma",   "This is the file 'gamma'.\n" },
      { "A/D/G",       0 },
      { "A/D/G/pi",    "Changed file 'pi'.\n" },
      { "A/D/G/rho",   "This is the file 'rho'.\n" },
      { "A/D/G/tau",   "This is the file 'tau'.\n" },
    };
    SVN_ERR(svn_fs_revision_root(&revision_root, fs,
                                 youngest_rev, subpool));
    SVN_ERR(svn_test__validate_tree(revision_root,
                                    entries,
                                    sizeof(entries)/sizeof(entries[0]),
                                    subpool));
  }

  /* Run an update from r1 to r2, excluding iota and everything under
     A/D.  Record the editor commands in a temporary txn. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 1, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(dir_delta_get_editor(&editor, &edit_baton, fs,
                               txn_root, "", subpool));

  SVN_ERR(svn_repos_begin_report2(&report_baton, 2, repos, "/", "", NULL,
                                  TRUE, svn_depth_infinity, FALSE, FALSE,
                                  editor, edit_baton, NULL, NULL, subpool));
  SVN_ERR(svn_repos_set_path3(report_baton, "", 1,
                              svn_depth_infinity,
                              FALSE, NULL, subpool));
  SVN_ERR(svn_repos_set_path3(report_baton, "iota", SVN_INVALID_REVNUM,
                              svn_depth_exclude,
                              FALSE, NULL, subpool));
  SVN_ERR(svn_repos_set_path3(report_baton, "A/D", SVN_INVALID_REVNUM,
                              svn_depth_exclude,
                              FALSE, NULL, subpool));
  SVN_ERR(svn_repos_finish_report(report_baton, subpool));

  /* Confirm the contents of the txn. */
  /* This should have iota and A/D from r1, and everything else from
     r2. */
  {
    static svn_test__tree_entry_t entries[] = {
      { "iota",        "This is the file 'iota'.\n" },
      { "A",           0 },
      { "A/mu",        "Changed file 'mu'.\n" },
      { "A/B",         0 },
      { "A/B/bar",     "New file 'bar'.\n" },
      { "A/B/lambda",  "This is the file 'lambda'.\n" },
      { "A/B/E",       0 },
      { "A/B/E/alpha", "This is the file 'alpha'.\n" },
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
    SVN_ERR(svn_test__validate_tree(txn_root,
                                    entries,
                                    sizeof(entries)/sizeof(entries[0]),
                                    subpool));
  }

  /* Clean up after ourselves. */
  svn_error_clear(svn_fs_abort_txn(txn, subpool));
  svn_pool_clear(subpool);

  /* Expect an error on an illegal report for r1 to r2.  The illegal
     sequence is that we exclude A/D, then set_path() below A/D. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, 1, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(dir_delta_get_editor(&editor, &edit_baton, fs,
                               txn_root, "", subpool));

  SVN_ERR(svn_repos_begin_report2(&report_baton, 2, repos, "/", "", NULL,
                                  TRUE, svn_depth_infinity, FALSE, FALSE,
                                  editor, edit_baton, NULL, NULL, subpool));
  SVN_ERR(svn_repos_set_path3(report_baton, "", 1,
                              svn_depth_infinity,
                              FALSE, NULL, subpool));
  SVN_ERR(svn_repos_set_path3(report_baton, "iota", SVN_INVALID_REVNUM,
                              svn_depth_exclude,
                              FALSE, NULL, subpool));
  SVN_ERR(svn_repos_set_path3(report_baton, "A/D", SVN_INVALID_REVNUM,
                              svn_depth_exclude,
                              FALSE, NULL, subpool));

  /* This is the illegal call, since A/D was excluded above; the call
     itself will not error, but finish_report() will.  As of r868172,
     this delayed error behavior is not actually promised by the
     reporter API, which merely warns callers not to touch a path
     underneath a previously excluded path without defining what will
     happen if they do.  However, it's still useful to test for the
     error, since the reporter code is sensitive and we'd certainly
     want to know about it if the behavior were to change. */
  SVN_ERR(svn_repos_set_path3(report_baton, "A/D/G/pi",
                              SVN_INVALID_REVNUM,
                              svn_depth_infinity,
                              FALSE, NULL, subpool));
  err = svn_repos_finish_report(report_baton, subpool);
  if (! err)
    {
      return svn_error_createf
        (SVN_ERR_TEST_FAILED, NULL,
         "Illegal report of \"A/D/G/pi\" did not error as expected");
    }
  else if (err->apr_err != SVN_ERR_FS_NOT_FOUND)
    {
      return svn_error_createf
        (SVN_ERR_TEST_FAILED, err,
         "Illegal report of \"A/D/G/pi\" got wrong kind of error:");
    }

  /* Clean up after ourselves. */
  svn_error_clear(err);
  svn_error_clear(svn_fs_abort_txn(txn, subpool));

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}



/* Test if prop values received by the server are validated.
 * These tests "send" property values to the server and diagnose the
 * behaviour.
 */

/* Helper function that makes an arbitrary change to a given repository
 * REPOS and runs a commit with a specific revision property set to a
 * certain value. The property name, type and value are given in PROP_KEY,
 * PROP_KLEN and PROP_VAL, as in apr_hash_set(), using a const char* key.
 *
 * The FILENAME argument names a file in the test repository to add in
 * this commit, e.g. "/A/should_fail_1".
 *
 * On success, the given file is added to the repository. So, using
 * the same name multiple times on the same repository might fail. Thus,
 * use different FILENAME arguments for every call to this function
 * (e.g. "/A/f1", "/A/f2", "/A/f3" etc).
 */
static svn_error_t *
prop_validation_commit_with_revprop(const char *filename,
                                    const char *prop_key,
                                    apr_ssize_t prop_klen,
                                    const svn_string_t *prop_val,
                                    svn_repos_t *repos,
                                    apr_pool_t *pool)
{
  const svn_delta_editor_t *editor;
  void *edit_baton;
  void *root_baton;
  void *file_baton;

  /* Prepare revision properties */
  apr_hash_t *revprop_table = apr_hash_make(pool);

  /* Add the requested property */
  apr_hash_set(revprop_table, prop_key, prop_klen, prop_val);

  /* Set usual author and log props, if not set already */
  if (strcmp(prop_key, SVN_PROP_REVISION_AUTHOR) != 0)
    {
      apr_hash_set(revprop_table, SVN_PROP_REVISION_AUTHOR,
                   APR_HASH_KEY_STRING,
                   svn_string_create("plato", pool));
    }
  else if (strcmp(prop_key, SVN_PROP_REVISION_LOG) != 0)
    {
      apr_hash_set(revprop_table, SVN_PROP_REVISION_LOG,
                   APR_HASH_KEY_STRING,
                   svn_string_create("revision log", pool));
    }

  /* Make an arbitrary change and commit using above values... */

  SVN_ERR(svn_repos_get_commit_editor5(&editor, &edit_baton, repos,
                                       NULL, "file://test", "/",
                                       revprop_table,
                                       NULL, NULL, NULL, NULL, pool));

  SVN_ERR(editor->open_root(edit_baton, 0, pool, &root_baton));

  SVN_ERR(editor->add_file(filename, root_baton, NULL,
                           SVN_INVALID_REVNUM, pool,
                           &file_baton));

  SVN_ERR(editor->close_file(file_baton, NULL, pool));

  SVN_ERR(editor->close_directory(root_baton, pool));

  SVN_ERR(editor->close_edit(edit_baton, pool));

  return SVN_NO_ERROR;
}


/* Expect failure of invalid commit in these cases:
 *  - log message contains invalid UTF-8 octet (issue 1796)
 *  - log message contains invalid linefeed style (non-LF) (issue 1796)
 */
static svn_error_t *
prop_validation(const svn_test_opts_t *opts,
                apr_pool_t *pool)
{
  svn_error_t *err;
  svn_repos_t *repos;
  const char non_utf8_string[5] = { 'a', 0xff, 'b', '\n', 0 };
  const char *non_lf_string = "a\r\nb\n\rc\rd\n";
  apr_pool_t *subpool = svn_pool_create(pool);

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-repo-prop-validation",
                                 opts, subpool));


  /* Test an invalid commit log message: UTF-8 */
  err = prop_validation_commit_with_revprop
            ("/non_utf8_log_msg",
             SVN_PROP_REVISION_LOG, APR_HASH_KEY_STRING,
             svn_string_create(non_utf8_string, subpool),
             repos, subpool);

  if (err == SVN_NO_ERROR)
    return svn_error_create(SVN_ERR_TEST_FAILED, err,
                            "Failed to reject a log with invalid "
                            "UTF-8");
  else if (err->apr_err != SVN_ERR_BAD_PROPERTY_VALUE)
    return svn_error_create(SVN_ERR_TEST_FAILED, err,
                            "Expected SVN_ERR_BAD_PROPERTY_VALUE for "
                            "a log with invalid UTF-8, "
                            "got another error.");
  svn_error_clear(err);


  /* Test an invalid commit log message: LF */
  err = prop_validation_commit_with_revprop
            ("/non_lf_log_msg",
             SVN_PROP_REVISION_LOG, APR_HASH_KEY_STRING,
             svn_string_create(non_lf_string, subpool),
             repos, subpool);

  if (err == SVN_NO_ERROR)
    return svn_error_create(SVN_ERR_TEST_FAILED, err,
                            "Failed to reject a log with inconsistent "
                            "line ending style");
  else if (err->apr_err != SVN_ERR_BAD_PROPERTY_VALUE)
    return svn_error_create(SVN_ERR_TEST_FAILED, err,
                            "Expected SVN_ERR_BAD_PROPERTY_VALUE for "
                            "a log with inconsistent line ending style, "
                            "got another error.");
  svn_error_clear(err);


  /* Done. */
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}



/* Tests for svn_repos_get_logsN() */

/* Log receiver which simple increments a counter. */
static svn_error_t *
log_receiver(void *baton,
             svn_log_entry_t *log_entry,
             apr_pool_t *pool)
{
  int *count = baton;
  (*count)++;
  return SVN_NO_ERROR;
}


static svn_error_t *
get_logs(const svn_test_opts_t *opts,
         apr_pool_t *pool)
{
  svn_repos_t *repos;
  svn_fs_t *fs;
  svn_fs_txn_t *txn;
  svn_fs_root_t *txn_root;
  svn_revnum_t start, end, youngest_rev = 0;
  apr_pool_t *subpool = svn_pool_create(pool);

  /* Create a filesystem and repository. */
  SVN_ERR(svn_test__create_repos(&repos, "test-repo-get-logs",
                                 opts, pool));
  fs = svn_repos_fs(repos);

  /* Revision 1:  Add the Greek tree. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__create_greek_tree(txn_root, subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  /* Revision 2:  Tweak A/mu and A/B/E/alpha. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/mu",
                                      "Revision 2", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/B/E/alpha",
                                      "Revision 2", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));

  /* Revision 3:  Tweak A/B/E/alpha and A/B/E/beta. */
  SVN_ERR(svn_fs_begin_txn(&txn, fs, youngest_rev, subpool));
  SVN_ERR(svn_fs_txn_root(&txn_root, txn, subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/B/E/alpha",
                                      "Revision 3", subpool));
  SVN_ERR(svn_test__set_file_contents(txn_root, "A/B/E/beta",
                                      "Revision 3", subpool));
  SVN_ERR(svn_repos_fs_commit_txn(NULL, repos, &youngest_rev, txn, subpool));
  SVN_TEST_ASSERT(SVN_IS_VALID_REVNUM(youngest_rev));


  for (start = 0; start <= youngest_rev; start++)
    {
      for (end = 0; end <= youngest_rev; end++)
        {
          svn_revnum_t start_arg = start ? start : SVN_INVALID_REVNUM;
          svn_revnum_t end_arg   = end ? end : SVN_INVALID_REVNUM;
          svn_revnum_t eff_start = start ? start : youngest_rev;
          svn_revnum_t eff_end   = end ? end : youngest_rev;
          int limit, max_logs =
            MAX(eff_start, eff_end) + 1 - MIN(eff_start, eff_end);
          int num_logs;

          for (limit = 0; limit <= max_logs; limit++)
            {
              int num_expected = limit ? limit : max_logs;

              svn_pool_clear(subpool);
              num_logs = 0;
              SVN_ERR(svn_repos_get_logs4(repos, NULL, start_arg, end_arg,
                                          limit, FALSE, FALSE, FALSE, NULL,
                                          NULL, NULL, log_receiver, &num_logs,
                                          subpool));
              if (num_logs != num_expected)
                return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                                         "Log with start=%ld,end=%ld,limit=%d "
                                         "returned %d entries (expected %d)",
                                         start_arg, end_arg, limit,
                                         num_logs, max_logs);
            }
        }
    }
  svn_pool_destroy(subpool);
  return SVN_NO_ERROR;
}


/* Tests for svn_repos_get_file_revsN() */

typedef struct file_revs_t {
    svn_revnum_t rev;
    const char *path;
    svn_boolean_t result_of_merge;
    const char *author;
} file_revs_t;

/* Finds the revision REV in the hash table passed in in BATON, and checks
   if the PATH and RESULT_OF_MERGE match are as expected. */
static svn_error_t *
file_rev_handler(void *baton, const char *path, svn_revnum_t rev,
                 apr_hash_t *rev_props, svn_boolean_t result_of_merge,
                 svn_txdelta_window_handler_t *delta_handler,
                 void **delta_baton, apr_array_header_t *prop_diffs,
                 apr_pool_t *pool)
{
  apr_hash_t *ht = baton;
  const char *author;
  file_revs_t *file_rev = apr_hash_get(ht, &rev, sizeof(svn_revnum_t));

  if (!file_rev)
    return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                             "Revision rev info not expected for rev %ld "
                             "from path %s",
                             rev, path);

  author = svn_prop_get_value(rev_props,
                              SVN_PROP_REVISION_AUTHOR);

  SVN_TEST_STRING_ASSERT(author, file_rev->author);
  SVN_TEST_STRING_ASSERT(path, file_rev->path);
  SVN_TEST_ASSERT(rev == file_rev->rev);
  SVN_TEST_ASSERT(result_of_merge == file_rev->result_of_merge);

  /* Remove this revision from this list so we'll be able to verify that we
     have seen all expected revisions. */
  apr_hash_set(ht, &rev, sizeof(svn_revnum_t), NULL);

  return SVN_NO_ERROR;
}

static svn_error_t *
test_get_file_revs(const svn_test_opts_t *opts,
                   apr_pool_t *pool)
{
  svn_repos_t *repos = NULL;
  svn_fs_t *fs;
  svn_revnum_t youngest_rev = 0;
  apr_pool_t *subpool = svn_pool_create(pool);
  int i;

  file_revs_t trunk_results[] = {
    { 2, "/trunk/A/mu", FALSE, "initial" },
    { 3, "/trunk/A/mu", FALSE, "user-trunk" },
    { 4, "/branches/1.0.x/A/mu", TRUE, "copy" },
    { 5, "/trunk/A/mu", FALSE, "user-trunk" },
    { 6, "/branches/1.0.x/A/mu", TRUE, "user-branch" },
    { 7, "/branches/1.0.x/A/mu", TRUE, "user-merge1" },
    { 8, "/trunk/A/mu", FALSE, "user-merge2" },
  };
  file_revs_t branch_results[] = {
    { 2, "/trunk/A/mu", FALSE, "initial" },
    { 3, "/trunk/A/mu", FALSE, "user-trunk" },
    { 4, "/branches/1.0.x/A/mu", FALSE, "copy" },
    { 5, "/trunk/A/mu", TRUE, "user-trunk" },
    { 6, "/branches/1.0.x/A/mu", FALSE, "user-branch" },
    { 7, "/branches/1.0.x/A/mu", FALSE, "user-merge1" },
  };
  apr_hash_t *ht_trunk_results = apr_hash_make(subpool);
  apr_hash_t *ht_branch_results = apr_hash_make(subpool);

  for (i = 0; i < sizeof(trunk_results) / sizeof(trunk_results[0]); i++)
    apr_hash_set(ht_trunk_results, &trunk_results[i].rev,
                 sizeof(svn_revnum_t), &trunk_results[i]);

  for (i = 0; i < sizeof(branch_results) / sizeof(branch_results[0]); i++)
    apr_hash_set(ht_branch_results, &branch_results[i].rev,
                 sizeof(svn_revnum_t), &branch_results[i]);

  /* Create the repository and verify blame results. */
  SVN_ERR(svn_test__create_blame_repository(&repos, "test-repo-get-filerevs",
                                            opts, subpool));
  fs = svn_repos_fs(repos);

  SVN_ERR(svn_fs_youngest_rev(&youngest_rev, fs, subpool));

  /* Verify blame of /trunk/A/mu */
  SVN_ERR(svn_repos_get_file_revs2(repos, "/trunk/A/mu", 0, youngest_rev,
                                   1, NULL, NULL,
                                   file_rev_handler,
                                   ht_trunk_results,
                                   subpool));
  SVN_TEST_ASSERT(apr_hash_count(ht_trunk_results) == 0);

  /* Verify blame of /branches/1.0.x/A/mu */
  SVN_ERR(svn_repos_get_file_revs2(repos, "/branches/1.0.x/A/mu", 0,
                                   youngest_rev,
                                   1, NULL, NULL,
                                   file_rev_handler,
                                   ht_branch_results,
                                   subpool));
  SVN_TEST_ASSERT(apr_hash_count(ht_branch_results) == 0);

  /* ### TODO: Verify blame of /branches/1.0.x/A/mu in range 6-7 */

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

static svn_error_t *
issue_4060(const svn_test_opts_t *opts,
           apr_pool_t *pool)
{
  apr_pool_t *subpool = svn_pool_create(pool);
  svn_authz_t *authz_cfg;
  svn_boolean_t allowed;
  const char *authz_contents =
    "[/A/B]"                                                               NL
    "ozymandias = rw"                                                      NL
    "[/]"                                                                  NL
    "ozymandias = r"                                                       NL
    ""                                                                     NL;

  SVN_ERR(authz_get_handle(&authz_cfg, authz_contents, subpool));

  SVN_ERR(svn_repos_authz_check_access(authz_cfg, "babylon",
                                       "/A/B/C", "ozymandias",
                                       svn_authz_write | svn_authz_recursive,
                                       &allowed, subpool));
  SVN_TEST_ASSERT(allowed);

  SVN_ERR(svn_repos_authz_check_access(authz_cfg, "",
                                       "/A/B/C", "ozymandias",
                                       svn_authz_write | svn_authz_recursive,
                                       &allowed, subpool));
  SVN_TEST_ASSERT(allowed);

  SVN_ERR(svn_repos_authz_check_access(authz_cfg, NULL,
                                       "/A/B/C", "ozymandias",
                                       svn_authz_write | svn_authz_recursive,
                                       &allowed, subpool));
  SVN_TEST_ASSERT(allowed);

  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}


/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_OPTS_PASS(dir_deltas,
                       "test svn_repos_dir_delta2"),
    SVN_TEST_OPTS_PASS(node_tree_delete_under_copy,
                       "test deletions under copies in node_tree code"),
    SVN_TEST_OPTS_PASS(revisions_changed,
                       "test svn_repos_history() (partially)"),
    SVN_TEST_OPTS_PASS(node_locations,
                       "test svn_repos_node_locations"),
    SVN_TEST_OPTS_PASS(node_locations2,
                       "test svn_repos_node_locations some more"),
    SVN_TEST_OPTS_PASS(rmlocks,
                       "test removal of defunct locks"),
    SVN_TEST_PASS2(authz,
                   "test authz access control"),
    SVN_TEST_OPTS_PASS(commit_editor_authz,
                       "test authz in the commit editor"),
    SVN_TEST_OPTS_PASS(commit_continue_txn,
                       "test commit with explicit txn"),
    SVN_TEST_OPTS_PASS(node_location_segments,
                       "test svn_repos_node_location_segments"),
    SVN_TEST_OPTS_PASS(reporter_depth_exclude,
                       "test reporter and svn_depth_exclude"),
    SVN_TEST_OPTS_PASS(prop_validation,
                       "test if revprops are validated by repos"),
    SVN_TEST_OPTS_PASS(get_logs,
                       "test svn_repos_get_logs ranges and limits"),
    SVN_TEST_OPTS_PASS(test_get_file_revs,
                       "test svn_repos_get_file_revsN"),
    SVN_TEST_OPTS_PASS(issue_4060,
                       "test issue 4060"),
    SVN_TEST_NULL
  };
