/*
 * compat-test.c:  tests svn_ver_compatible
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
#include "svn_version.h"

#include "../svn_test.h"
#include "svn_private_config.h"

#ifndef SVN_DISABLE_FULL_VERSION_MATCH
#define FALSE_IF_FULL FALSE
#else
#define FALSE_IF_FULL TRUE
#endif

static svn_error_t *
test_version_compatibility(apr_pool_t *pool)
{
  unsigned int i;

  struct version_pair {
    svn_version_t my_version;
    svn_version_t lib_version;
    svn_boolean_t result;
  } versions[] = {
    { {1, 0, 0, ""}, {1, 0, 0, ""}, TRUE },
    { {1, 0, 0, ""}, {2, 0, 0, ""}, FALSE },
    { {2, 0, 0, ""}, {1, 0, 0, ""}, FALSE },

    { {1, 0, 0, ""}, {1, 0, 1, ""}, TRUE },
    { {1, 0, 1, ""}, {1, 0, 0, ""}, TRUE },
    { {1, 0, 1, ""}, {1, 0, 1, ""}, TRUE },

    { {1, 0, 0, ""}, {1, 1, 0, ""}, TRUE },
    { {1, 0, 1, ""}, {1, 1, 0, ""}, TRUE },
    { {1, 0, 0, ""}, {1, 1, 1, ""}, TRUE },
    { {1, 1, 0, ""}, {1, 0, 0, ""}, FALSE },

    { {1, 0, 0, "dev"}, {1, 0, 0, "dev"}, TRUE },
    { {1, 0, 1, "dev"}, {1, 0, 1, "dev"}, TRUE },
    { {1, 1, 0, "dev"}, {1, 1, 0, "dev"}, TRUE },
    { {1, 1, 1, "dev"}, {1, 1, 1, "dev"}, TRUE },
    { {1, 0, 0, "dev"}, {1, 0, 1, "dev"}, FALSE_IF_FULL },
    { {1, 0, 0, "dev"}, {1, 1, 0, "dev"}, FALSE_IF_FULL },
    { {1, 0, 0, "cev"}, {1, 0, 0, "dev"}, FALSE_IF_FULL },
    { {1, 0, 0, "eev"}, {1, 0, 0, "dev"}, FALSE_IF_FULL },
    { {1, 0, 1, "dev"}, {1, 0, 0, "dev"}, FALSE_IF_FULL },
    { {1, 1, 0, "dev"}, {1, 0, 0, "dev"}, FALSE },

    { {1, 0, 0, ""},    {1, 0, 0, "dev"}, FALSE_IF_FULL },

    { {1, 0, 0, "dev"}, {1, 0, 0, ""}, FALSE_IF_FULL },
    { {1, 0, 1, "dev"}, {1, 0, 0, ""}, TRUE },
    { {1, 1, 0, "dev"}, {1, 0, 0, ""}, FALSE },
    { {1, 1, 1, "dev"}, {1, 1, 0, ""}, TRUE },
    { {1, 1, 1, "dev"}, {1, 0, 0, ""}, FALSE },
    { {2, 0, 0, "dev"}, {1, 0, 0, ""}, FALSE },
    { {1, 0, 0, "dev"}, {2, 0, 0, ""}, FALSE },
  };

  for (i = 0; i < sizeof(versions)/sizeof(versions[0]); ++i)
    {
      if (svn_ver_compatible(&versions[i].my_version,
                             &versions[i].lib_version) != versions[i].result)
        return svn_error_createf
          (SVN_ERR_TEST_FAILED, NULL,
           "svn_ver_compatible (%d.%d.%d(%s), %d.%d.%d(%s)) failed",
           versions[i].my_version.major,
           versions[i].my_version.minor,
           versions[i].my_version.patch,
           versions[i].my_version.tag,
           versions[i].lib_version.major,
           versions[i].lib_version.minor,
           versions[i].lib_version.patch,
           versions[i].lib_version.tag);
    }

  return SVN_NO_ERROR;
}

/* An array of all test functions */
struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(test_version_compatibility,
                   "svn_ver_compatible"),
    SVN_TEST_NULL
  };
