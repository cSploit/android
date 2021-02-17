/*
 * revision-test.c -- test the revision functions
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

#include "svn_types.h"

#include "../svn_test.h"

static svn_error_t *
test_revnum_parse(apr_pool_t *pool)
{
  const char **t;

  const char *failure_tests[] = {
    "",
    "abc",
    "-456",
    NULL
  };

  const char *success_tests[] = {
    "0",
    "12345",
    "12345ABC",
    NULL
  };

  /* These tests should succeed. */
  for (t=success_tests; *t; ++t)
    {
      svn_revnum_t rev = -123;
      const char *endptr;

      /* Do one test with a NULL end pointer and then with non-NULL
         pointer. */
      SVN_ERR(svn_revnum_parse(&rev, *t, NULL));
      SVN_ERR(svn_revnum_parse(&rev, *t, &endptr));

      if (-123 == rev)
        return svn_error_createf
          (SVN_ERR_TEST_FAILED,
           NULL,
           "svn_revnum_parse('%s') should change the revision for "
           "a good string",
           *t);

      if (endptr == *t)
        return svn_error_createf
          (SVN_ERR_TEST_FAILED,
           NULL,
           "End pointer for svn_revnum_parse('%s') should not "
           "point to the start of the string",
           *t);
    }

  /* These tests should fail. */
  for (t=failure_tests; *t; ++t)
    {
      svn_revnum_t rev = -123;
      const char *endptr;

      /* Do one test with a NULL end pointer and then with non-NULL
         pointer. */
      svn_error_t *err = svn_revnum_parse(&rev, *t, NULL);
      svn_error_clear(err);

      err = svn_revnum_parse(&rev, *t, &endptr);
      if (! err)
        return svn_error_createf
          (SVN_ERR_TEST_FAILED, NULL,
           "svn_revnum_parse('%s') succeeded when it should "
           "have failed",
           *t);
      svn_error_clear(err);

      if (-123 != rev)
        return svn_error_createf
          (SVN_ERR_TEST_FAILED,
           NULL,
           "svn_revnum_parse('%s') should not change the revision "
           "for a bad string",
           *t);

      if (endptr != *t)
        return svn_error_createf
          (SVN_ERR_TEST_FAILED,
           NULL,
           "End pointer for svn_revnum_parse('%s') does not "
           "point to the start of the string",
           *t);
    }

  return SVN_NO_ERROR;
}


/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(test_revnum_parse,
                   "test svn_revnum_parse"),
    SVN_TEST_NULL
  };
