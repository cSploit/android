/*
 * checksum-test.c:  tests checksum functions.
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

#include <apr_pools.h>

#include "svn_error.h"

#include "../svn_test.h"

static svn_error_t *
test_checksum_parse(apr_pool_t *pool)
{
  const char *md5_digest = "8518b76f7a45fe4de2d0955085b41f98";
  const char *sha1_digest = "74d82379bcc6771454377db03b912c2b62704139";
  const char *checksum_display;
  svn_checksum_t *checksum;

  SVN_ERR(svn_checksum_parse_hex(&checksum, svn_checksum_md5, md5_digest, pool));
  checksum_display = svn_checksum_to_cstring_display(checksum, pool);

  if (strcmp(checksum_display, md5_digest) != 0)
    return svn_error_createf
      (SVN_ERR_CHECKSUM_MISMATCH, NULL,
       "verify-checksum: md5 checksum mismatch:\n"
       "   expected:  %s\n"
       "     actual:  %s\n", md5_digest, checksum_display);

  SVN_ERR(svn_checksum_parse_hex(&checksum, svn_checksum_sha1, sha1_digest,
                                 pool));
  checksum_display = svn_checksum_to_cstring_display(checksum, pool);

  if (strcmp(checksum_display, sha1_digest) != 0)
    return svn_error_createf
      (SVN_ERR_CHECKSUM_MISMATCH, NULL,
       "verify-checksum: sha1 checksum mismatch:\n"
       "   expected:  %s\n"
       "     actual:  %s\n", sha1_digest, checksum_display);

  return SVN_NO_ERROR;
}

/* An array of all test functions */
struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(test_checksum_parse,
                   "checksum parse"),
    SVN_TEST_NULL
  };
