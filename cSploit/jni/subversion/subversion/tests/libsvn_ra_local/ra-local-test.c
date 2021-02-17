/*
 * ra-local-test.c :  basic tests for the RA LOCAL library
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



#include <apr_general.h>
#include <apr_pools.h>

#define SVN_DEPRECATED

#include "svn_error.h"
#include "svn_delta.h"
#include "svn_ra.h"
#include "svn_client.h"

#include "../svn_test.h"
#include "../svn_test_fs.h"
#include "../../libsvn_ra_local/ra_local.h"

/*-------------------------------------------------------------------*/

/** Helper routines. **/


static svn_error_t *
make_and_open_local_repos(svn_ra_session_t **session,
                          const char *repos_name,
                          const svn_test_opts_t *opts,
                          apr_pool_t *pool)
{
  svn_repos_t *repos;
  const char *url;
  svn_ra_callbacks2_t *cbtable;

  SVN_ERR(svn_ra_create_callbacks(&cbtable, pool));

  SVN_ERR(svn_test__create_repos(&repos, repos_name, opts, pool));
  SVN_ERR(svn_ra_initialize(pool));

  SVN_ERR(svn_uri_get_file_url_from_dirent(&url, repos_name, pool));

  SVN_ERR(svn_ra_open3(session,
                       url,
                       NULL,
                       cbtable,
                       NULL,
                       NULL,
                       pool));

  return SVN_NO_ERROR;
}


/*-------------------------------------------------------------------*/

/** The tests **/

/* Open an RA session to a local repository. */
static svn_error_t *
open_ra_session(const svn_test_opts_t *opts,
                apr_pool_t *pool)
{
  svn_ra_session_t *session;

  SVN_ERR(make_and_open_local_repos(&session,
                                    "test-repo-open", opts, pool));

  return SVN_NO_ERROR;
}


/* Discover the youngest revision in a repository.  */
static svn_error_t *
get_youngest_rev(const svn_test_opts_t *opts,
                 apr_pool_t *pool)
{
  svn_ra_session_t *session;
  svn_revnum_t latest_rev;

  SVN_ERR(make_and_open_local_repos(&session,
                                    "test-repo-getrev", opts,
                                    pool));

  /* Get the youngest revision and make sure it's 0. */
  SVN_ERR(svn_ra_get_latest_revnum(session, &latest_rev, pool));

  if (latest_rev != 0)
    return svn_error_create(SVN_ERR_FS_GENERAL, NULL,
                            "youngest rev isn't 0!");

  return SVN_NO_ERROR;
}


/* Helper function.  Run svn_ra_local__split_URL with interest only in
   the return error code */
static apr_status_t
try_split_url(const char *url, apr_pool_t *pool)
{
  svn_repos_t *repos;
  const char *repos_path, *fs_path;
  svn_error_t *err;
  apr_status_t apr_err;

  err = svn_ra_local__split_URL(&repos, &repos_path, &fs_path, url, pool);

  if (! err)
    return SVN_NO_ERROR;

  apr_err = err->apr_err;
  svn_error_clear(err);
  return apr_err;
}


static svn_error_t *
split_url_syntax(apr_pool_t *pool)
{
  apr_status_t apr_err;

  /* TEST 1:  Make sure we can recognize bad URLs (this should not
     require a filesystem) */

  /* Use `blah' for scheme instead of `file' */
  apr_err = try_split_url("blah:///bin/svn", pool);
  if (apr_err != SVN_ERR_RA_ILLEGAL_URL)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_ra_local__split_URL failed to catch bad URL (scheme)");

  /* Use only a hostname, with no path */
  apr_err = try_split_url("file://hostname", pool);
  if (apr_err != SVN_ERR_RA_ILLEGAL_URL)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_ra_local__split_URL failed to catch bad URL (no path)");

  return SVN_NO_ERROR;
}

static svn_error_t *
split_url_bad_host(apr_pool_t *pool)
{
  apr_status_t apr_err;

  /* Give a hostname other than `' or `localhost' */
  apr_err = try_split_url("file://myhost/repos/path", pool);
  if (apr_err != SVN_ERR_RA_ILLEGAL_URL)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_ra_local__split_URL failed to catch bad URL (hostname)");

  return SVN_NO_ERROR;
}

static svn_error_t *
split_url_host(apr_pool_t *pool)
{
  apr_status_t apr_err;

  /* Make sure we *don't* fuss about a good URL (note that this URL
     still doesn't point to an existing versioned resource) */
  apr_err = try_split_url("file:///repos/path", pool);
  if (apr_err == SVN_ERR_RA_ILLEGAL_URL)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_ra_local__split_URL cried foul about a good URL (no hostname)");

  apr_err = try_split_url("file://localhost/repos/path", pool);
  if (apr_err == SVN_ERR_RA_ILLEGAL_URL)
    return svn_error_create
      (SVN_ERR_TEST_FAILED, NULL,
       "svn_ra_local__split_URL cried foul about a good URL (localhost)");

  return SVN_NO_ERROR;
}


/* Helper function.  Creates a repository in the current working
   directory named REPOS_PATH, then assembes a URL that points to that
   FS, plus additional cruft (IN_REPOS_PATH) that theoretically refers to a
   versioned resource in that repository.  Finally, it runs this URL
   through svn_ra_local__split_URL to verify that it accurately
   separates the filesystem path and the repository path cruft.

   If IN_REPOS_PATH is NULL, we'll split the root URL and verify our
   parts that way (noting that that in-repos-path that results should
   be "/").  */
static svn_error_t *
check_split_url(const char *repos_path,
                const char *in_repos_path,
                const svn_test_opts_t *opts,
                apr_pool_t *pool)
{
  svn_repos_t *repos;
  const char *url, *root_url, *repos_part, *in_repos_part;

  /* Create a filesystem and repository */
  SVN_ERR(svn_test__create_repos(&repos, repos_path, opts, pool));

  SVN_ERR(svn_uri_get_file_url_from_dirent(&root_url, repos_path, pool));
  if (in_repos_path)
    url = apr_pstrcat(pool, root_url, in_repos_path, (char *)NULL);
  else
    url = root_url;

  /* Run this URL through our splitter... */
  SVN_ERR(svn_ra_local__split_URL(&repos, &repos_part, &in_repos_part,
                                  url, pool));

  /* We better see the REPOS_PART looking just like our ROOT_URL.  And
     we better see in the IN_REPOS_PART either exactly the same as the
     IN_REPOS_PATH provided us, or "/" if we weren't provided an
     IN_REPOS_PATH.  */
  if ((strcmp(repos_part, root_url) == 0)
      && ((in_repos_path && (strcmp(in_repos_part, in_repos_path) == 0))
          || ((! in_repos_path) && (strcmp(in_repos_part, "/") == 0))))
    return SVN_NO_ERROR;

  return svn_error_createf
    (SVN_ERR_TEST_FAILED, NULL,
     "svn_ra_local__split_URL failed to properly split the URL\n"
     "%s\n%s\n%s\n%s",
     repos_part, root_url, in_repos_part,
     in_repos_path ? in_repos_path : "(null)");
}


static svn_error_t *
split_url_test(const svn_test_opts_t *opts,
               apr_pool_t *pool)
{
  /* TEST 2: Given well-formed URLs, make sure that we can correctly
     find where the filesystem portion of the path ends and the
     in-repository path begins.  */
  SVN_ERR(check_split_url("test-repo-split-fs1",
                          "/trunk/foobar/quux.c",
                          opts,
                          pool));
  SVN_ERR(check_split_url("test-repo-split-fs2",
                          "/alpha/beta/gamma/delta/epsilon/zeta/eta/theta",
                          opts,
                          pool));
  SVN_ERR(check_split_url("test-repo-split-fs3",
                          NULL,
                          opts,
                          pool));

  return SVN_NO_ERROR;
}



/* The test table.  */

#if defined(WIN32) || defined(__CYGWIN__)
#define HAS_UNC_HOST 1
#else
#define HAS_UNC_HOST 0
#endif

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_OPTS_PASS(open_ra_session,
                       "open an ra session to a local repository"),
    SVN_TEST_OPTS_PASS(get_youngest_rev,
                      "get the youngest revision in a repository"),
    SVN_TEST_PASS2(split_url_syntax,
                   "svn_ra_local__split_URL: syntax validation"),
    SVN_TEST_SKIP2(split_url_bad_host, HAS_UNC_HOST,
                   "svn_ra_local__split_URL: invalid host names"),
    SVN_TEST_PASS2(split_url_host,
                   "svn_ra_local__split_URL: valid host names"),
    SVN_TEST_OPTS_PASS(split_url_test,
                       "test svn_ra_local__split_URL correctness"),
    SVN_TEST_NULL
  };
