/* svn_test_fs.h --- test helpers for the filesystem
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

#ifndef SVN_TEST_FS_H
#define SVN_TEST_FS_H

#include <apr_pools.h>
#include "svn_error.h"
#include "svn_fs.h"
#include "svn_repos.h"
#include "svn_delta.h"
#include "svn_test.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*-------------------------------------------------------------------*/

/** Helper routines for filesystem functionality. **/


/* Set *FS_P to a fresh, unopened FS object, with the right warning
   handling function set.  */
svn_error_t *
svn_test__fs_new(svn_fs_t **fs_p, apr_pool_t *pool);


/* Creates a filesystem which is always of type "bdb" in a subdir NAME
   and return a new FS object which points to it.  (Ignores any
   fs-type declaration in OPTS.)  */
svn_error_t *
svn_test__create_bdb_fs(svn_fs_t **fs_p,
                        const char *name,
                        const svn_test_opts_t *opts,
                        apr_pool_t *pool);


/* Create a filesystem based on OPTS in a subdir NAME and return a new
   FS object which points to it.  */
svn_error_t *
svn_test__create_fs(svn_fs_t **fs_p,
                    const char *name,
                    const svn_test_opts_t *opts,
                    apr_pool_t *pool);


/* Create a repository with a filesystem based on OPTS in a subdir NAME
   and return a new REPOS object which points to it.  */
svn_error_t *
svn_test__create_repos(svn_repos_t **repos_p,
                       const char *name,
                       const svn_test_opts_t *opts,
                       apr_pool_t *pool);

/* Read all data from a generic read STREAM, and return it in STRING.
   Allocate the svn_stringbuf_t in POOL.  (All data in STRING will be
   dup'ed from STREAM using POOL too.) */
svn_error_t *
svn_test__stream_to_string(svn_stringbuf_t **string,
                           svn_stream_t *stream,
                           apr_pool_t *pool);


/* Set the contents of file in PATH under ROOT to CONTENTS.  */
svn_error_t *
svn_test__set_file_contents(svn_fs_root_t *root,
                            const char *path,
                            const char *contents,
                            apr_pool_t *pool);


/* Get the contents of file in PATH under ROOT, and copy them into
   STR.  */
svn_error_t *
svn_test__get_file_contents(svn_fs_root_t *root,
                            const char *path,
                            svn_stringbuf_t **str,
                            apr_pool_t *pool);



/* The Helper Functions to End All Helper Functions */

/* Structure used for testing integrity of the filesystem's revision
   using validate_tree(). */
typedef struct svn_test__tree_entry_t
{
  const char *path;     /* full path of this node */
  const char *contents; /* text contents (NULL for directories) */
}
svn_test__tree_entry_t;


/* Wrapper for an array of the above svn_test__tree_entry_t's.  */
typedef struct svn_test__tree_t
{
  svn_test__tree_entry_t *entries;
  int num_entries;
}
svn_test__tree_t;


/* Given a transaction or revision root (ROOT), check to see if the
   tree that grows from that root has all the path entries, and only
   those entries, passed in the array ENTRIES (which is an array of
   NUM_ENTRIES svn_test__tree_entry_t's).  */
svn_error_t *
svn_test__validate_tree(svn_fs_root_t *root,
                        svn_test__tree_entry_t *entries,
                        int num_entries,
                        apr_pool_t *pool);

/* Structure for describing script-ish commands to perform on a
   transaction using svn_test__txn_script_exec().  */
typedef struct svn_test__txn_script_command_t
{
  /* command:

     'a' -- add (PARAM1 is file contents, or NULL for directories)
     'c' -- copy (PARAM1 is target path, copy source is youngest rev)
     'd' -- delete
     'e' -- edit (PARAM1 is new file contents)
  */
  int cmd;
  const char *path; /* path to resource in the filesystem */
  const char *param1; /* command parameter (see above) */
}
svn_test__txn_script_command_t;


/* Execute a "script" SCRIPT on items under TXN_ROOT.  */
svn_error_t *
svn_test__txn_script_exec(svn_fs_root_t *txn_root,
                          svn_test__txn_script_command_t *script,
                          int num_edits,
                          apr_pool_t *pool);

/* Verify that the tree that exists under ROOT is exactly the Greek
   Tree. */
svn_error_t *
svn_test__check_greek_tree(svn_fs_root_t *root,
                           apr_pool_t *pool);


/* Create the Greek Tree under TXN_ROOT.  See ./greek-tree.txt.  */
svn_error_t *
svn_test__create_greek_tree(svn_fs_root_t *txn_root,
                            apr_pool_t *pool);

/* Create the Greek Tree under TXN_ROOT at dir ROOT_DIR.  */
svn_error_t *
svn_test__create_greek_tree_at(svn_fs_root_t *txn_root,
                               const char *root_dir,
                               apr_pool_t *pool);

/* Create a new repository with a greek tree, trunk, branch and some
   merges between them. */
svn_error_t *
svn_test__create_blame_repository(svn_repos_t **out_repos,
                                  const char *test_name,
                                  const svn_test_opts_t *opts,
                                  apr_pool_t *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* SVN_TEST_FS_H */
