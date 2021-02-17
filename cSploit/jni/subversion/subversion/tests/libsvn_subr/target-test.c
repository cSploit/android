/*
 * target-test.c: test the condense_targets functions
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


#ifdef _MSC_VER
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h> /* for getcwd() */
#endif

#define SVN_DEPRECATED

#include "svn_pools.h"
#include "svn_path.h"

#include "../svn_test.h"

typedef svn_error_t *(*condense_targets_func_t)
                           (const char **pcommon,
                            apr_array_header_t **pcondensed_targets,
                            const apr_array_header_t *targets,
                            svn_boolean_t remove_redundancies,
                            apr_pool_t *pool);

/** Executes function CONDENSE_TARGETS twice - with and without requesting the
 * condensed targets list -  on TEST_TARGETS (comma sep. string) and compares
 * the results with EXP_COMMON and EXP_TARGETS (comma sep. string).
 *
 * @note: a '%' character at the beginning of EXP_COMMON or EXP_TARGETS will
 * be replaced by the current working directory.
 *
 * Returns an error if any of the comparisons fail.
 */
static svn_error_t *
condense_targets_tests_helper(const char* title,
                              const char* test_targets,
                              const char* exp_common,
                              const char* exp_targets,
                              const char* func_name,
                              condense_targets_func_t condense_targets,
                              apr_pool_t *pool)
{
  apr_array_header_t *targets;
  apr_array_header_t *condensed_targets;
  const char *common_path, *common_path2, *curdir;
  char *token, *iter;
  const char *exp_common_abs = exp_common;
  int i;
  char buf[8192];

  if (! getcwd(buf, sizeof(buf)))
    return svn_error_create(SVN_ERR_BASE, NULL, "getcwd() failed");
  curdir = svn_path_internal_style(buf, pool);

  /* Create the target array */
  targets = apr_array_make(pool, sizeof(test_targets), sizeof(const char *));
  token = apr_strtok(apr_pstrdup(pool, test_targets), ",", &iter);
  while (token)
    {
      APR_ARRAY_PUSH(targets, const char *) =
        svn_path_internal_style(token, pool);
      token = apr_strtok(NULL, ",", &iter);
    };

  /* Call the function */
  SVN_ERR(condense_targets(&common_path, &condensed_targets, targets,
                           TRUE, pool));

  /* Verify the common part with the expected (prefix with cwd). */
  if (*exp_common == '%')
    exp_common_abs = apr_pstrcat(pool, curdir, exp_common + 1, (char *)NULL);

  if (strcmp(common_path, exp_common_abs) != 0)
    {
      return svn_error_createf
        (SVN_ERR_TEST_FAILED, NULL,
         "%s (test %s) returned %s instead of %s",
           func_name, title,
           common_path, exp_common_abs);
    }

  /* Verify the condensed targets */
  token = apr_strtok(apr_pstrdup(pool, exp_targets), ",", &iter);
  for (i = 0; i < condensed_targets->nelts; i++)
    {
      const char * target = APR_ARRAY_IDX(condensed_targets, i, const char*);
      if (token && (*token == '%'))
        token = apr_pstrcat(pool, curdir, token + 1, (char *)NULL);
      if (! token ||
          (target && (strcmp(target, token) != 0)))
        {
          return svn_error_createf
            (SVN_ERR_TEST_FAILED, NULL,
             "%s (test %s) couldn't find %s in expected targets list",
               func_name, title,
               target);
        }
      token = apr_strtok(NULL, ",", &iter);
    }

  /* Now ensure it works without the pbasename */
  SVN_ERR(condense_targets(&common_path2, NULL, targets, TRUE, pool));

  /* Verify the common part again */
  if (strcmp(common_path, common_path2) != 0)
    {
      return svn_error_createf
        (SVN_ERR_TEST_FAILED, NULL,
         "%s (test %s): Common path without getting targets %s does not match" \
         "common path with targets %s",
          func_name, title,
          common_path2, common_path);
    }

  return SVN_NO_ERROR;
}


static svn_error_t *
test_path_condense_targets(apr_pool_t *pool)
{
  int i;
  struct {
    const char* title;
    const char* targets;
    const char* exp_common;
    const char* exp_targets;
  } tests[] = {
    { "normal use", "z/A/B,z/A,z/A/C,z/D/E,z/D/F,z/D,z/G,z/G/H,z/G/I",
      "%/z", "A,D,G" },
    {"identical dirs", "z/A,z/A,z/A,z/A",
     "%/z/A", "" },
    {"identical files", "z/A/file,z/A/file,z/A/file,z/A/file",
     "%/z/A/file", "" },
    {"single dir", "z/A",
     "%/z/A", "" },
    {"single file", "z/A/file",
     "%/z/A/file", "" },
    {"URLs", "http://host/A/C,http://host/A/C/D,http://host/A/B/D",
     "http://host/A", "C,B/D" },
    {"URLs with no common prefix",
     "http://host1/A/C,http://host2/A/C/D,http://host3/A/B/D",
     "", "http://host1/A/C,http://host2/A/C/D,http://host3/A/B/D" },
    {"file URLs with no common prefix", "file:///A/C,file:///B/D",
     "", "file:///A/C,file:///B/D" },
    {"URLs with mixed protocols",
     "http://host/A/C,file:///B/D,gopher://host/A",
     "", "http://host/A/C,file:///B/D,gopher://host/A" },
    {"mixed paths and URLs",
     "z/A/B,z/A,http://host/A/C/D,http://host/A/C",
     "", "%/z/A,http://host/A/C" },
  };

  for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
      SVN_ERR(condense_targets_tests_helper(tests[i].title,
                                            tests[i].targets,
                                            tests[i].exp_common,
                                            tests[i].exp_targets,
                                            "svn_path_condense_targets",
                                            svn_path_condense_targets,
                                            pool));
    }

  return SVN_NO_ERROR;
}



/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(test_path_condense_targets,
                   "test svn_path_condense_targets"),
    SVN_TEST_NULL
  };
