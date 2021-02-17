/*
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

#ifndef SVN_TEST_H
#define SVN_TEST_H

#ifndef SVN_ENABLE_DEPRECATION_WARNINGS_IN_TESTS
#undef SVN_DEPRECATED
#define SVN_DEPRECATED
#endif /* ! SVN_ENABLE_DEPRECATION_WARNINGS_IN_TESTS */

#include <apr_pools.h>

#include "svn_delta.h"
#include "svn_path.h"
#include "svn_types.h"
#include "svn_error.h"
#include "svn_string.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Handy macro to test a condition, returning SVN_ERR_TEST_FAILED if FALSE
 *
 * This macro should be used in place of SVN_ERR_ASSERT() since we don't
 * want to core-dump the test.
 */
#define SVN_TEST_ASSERT(expr)                                     \
  do {                                                            \
    if (!(expr))                                                  \
      return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,         \
                               "assertion '%s' failed at %s:%d",  \
                               #expr, __FILE__, __LINE__);        \
  } while (0)

/** Handy macro for testing string equality.
 */
#define SVN_TEST_STRING_ASSERT(expr, expected_expr)                 \
  do {                                                              \
    const char *tst_str1 = (expr);                                  \
    const char *tst_str2 = (expected_expr);                         \
                                                                    \
    if (tst_str2 == NULL && tst_str1 == NULL)                       \
      break;                                                        \
    if (   (tst_str2 != NULL && tst_str1 == NULL)                   \
        || (strcmp(tst_str2, tst_str1) != 0)  )                     \
      return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,           \
          "Strings not equal\n  Expected: '%s'\n  Found:    '%s'"   \
          "\n  at %s:%d",                                           \
          tst_str2, tst_str1, __FILE__, __LINE__);                  \
  } while(0)


/* Baton for any arguments that need to be passed from main() to svn
 * test functions.
 */
typedef struct svn_test_opts_t
{
  /* Description of the fs backend that should be used for testing. */
  const char *fs_type;
  /* Config file. */
  const char *config_file;
  /* Minor version to use for servers and FS backends, or zero to use
     the current latest version. */
  int server_minor_version;
  /* Add future "arguments" here. */
} svn_test_opts_t;

/* Prototype for test driver functions. */
typedef svn_error_t* (*svn_test_driver2_t)(apr_pool_t *pool);

/* Prototype for test driver functions which need options. */
typedef svn_error_t* (*svn_test_driver_opts_t)(const svn_test_opts_t *opts,
                                               apr_pool_t *pool);

/* Test modes. */
enum svn_test_mode_t
  {
    svn_test_pass,
    svn_test_xfail,
    svn_test_skip,
    svn_test_all
  };

/* Each test gets a test descriptor, holding the function and other
 * associated data.
 */
struct svn_test_descriptor_t
{
  /* Is the test marked XFAIL? */
  enum svn_test_mode_t mode;

  /* A pointer to the test driver function. */
  svn_test_driver2_t func2;

  /* A pointer to the test driver function. */
  svn_test_driver_opts_t func_opts;

  /* A descriptive message for this test. */
  const char *msg;

  /* An optional description of a work-in-progress test. */
  const char *wip;
};

/* All Subversion test programs include an array of svn_test_descriptor_t's
 * (all of our sub-tests) that begins and ends with a SVN_TEST_NULL entry.
 */
extern struct svn_test_descriptor_t test_funcs[];

/* A null initializer for the test descriptor. */
#define SVN_TEST_NULL  {0}

/* Initializer for PASS tests */
#define SVN_TEST_PASS2(func, msg)  {svn_test_pass, func, NULL, msg}

/* Initializer for XFAIL tests */
#define SVN_TEST_XFAIL2(func, msg) {svn_test_xfail, func, NULL, msg}

/* Initializer for conditional XFAIL tests */
#define SVN_TEST_XFAIL_COND2(func, p, msg) \
  {(p) ? svn_test_xfail : svn_test_pass, func, NULL, msg}

/* Initializer for SKIP tests */
#define SVN_TEST_SKIP2(func, p, msg) \
  {(p) ? svn_test_skip : svn_test_pass, func, NULL, msg}

/* Similar macros, but for tests needing options.  */
#define SVN_TEST_OPTS_PASS(func, msg)  {svn_test_pass, NULL, func, msg}
#define SVN_TEST_OPTS_XFAIL(func, msg) {svn_test_xfail, NULL, func, msg}
#define SVN_TEST_OPTS_XFAIL_COND(func, p, msg) \
  {(p) ? svn_test_xfail : svn_test_pass, NULL, func, msg}
#define SVN_TEST_OPTS_SKIP(func, p, msg) \
  {(p) ? svn_test_skip : svn_test_pass, NULL, func, msg}

/* Initializer for XFAIL tests for works-in-progress. */
#define SVN_TEST_WIMP(func, msg, wip) \
  {svn_test_xfail, func, NULL, msg, wip}
#define SVN_TEST_WIMP_COND(func, p, msg, wip) \
  {(p) ? svn_test_xfail : svn_test_pass, func, NULL, msg, wip}
#define SVN_TEST_OPTS_WIMP(func, msg, wip) \
  {svn_test_xfail, NULL, func, msg, wip}
#define SVN_TEST_OPTS_WIMP_COND(func, p, msg, wip) \
  {(p) ? svn_test_xfail : svn_test_pass, NULL, func, msg, wip}



/* Return a pseudo-random number based on SEED, and modify SEED.
 *
 * This is a "good" pseudo-random number generator, intended to replace
 * all those "bad" rand() implementations out there.
 */
apr_uint32_t svn_test_rand(apr_uint32_t *seed);


/* Add PATH to the test cleanup list.  */
void svn_test_add_dir_cleanup(const char *path);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_TEST_H */
