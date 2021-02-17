/*
 * cache-test.c -- test the in-memory cache
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

#include <stdio.h>
#include <string.h>
#include <apr_general.h>
#include <apr_lib.h>
#include <apr_time.h>

#include "svn_pools.h"

#include "private/svn_cache.h"
#include "svn_private_config.h"

#include "../svn_test.h"

/* Implements svn_cache__serialize_func_t */
static svn_error_t *
serialize_revnum(char **data,
                 apr_size_t *data_len,
                 void *in,
                 apr_pool_t *pool)
{
  *data_len = sizeof(svn_revnum_t);
  *data = apr_pmemdup(pool, in, *data_len);

  return SVN_NO_ERROR;
}


/* Implements svn_cache__deserialize_func_t */
static svn_error_t *
deserialize_revnum(void **out,
                   char *data,
                   apr_size_t data_len,
                   apr_pool_t *pool)
{
  const svn_revnum_t *in_rev = (const svn_revnum_t *) data;
  svn_revnum_t *out_rev;

  if (data_len != sizeof(*in_rev))
    return svn_error_create(SVN_ERR_REVNUM_PARSE_FAILURE, NULL,
                            _("Bad size for revision number in cache"));
  out_rev = apr_palloc(pool, sizeof(*out_rev));
  *out_rev = *in_rev;
  *out = out_rev;
  return SVN_NO_ERROR;
}

static svn_error_t *
basic_cache_test(svn_cache__t *cache,
                 svn_boolean_t size_is_one,
                 apr_pool_t *pool)
{
  svn_boolean_t found;
  svn_revnum_t twenty = 20, thirty = 30, *answer;
  apr_pool_t *subpool;

  /* We use a subpool for all calls in this test and aggressively
   * clear it, to try to find any bugs where the cached values aren't
   * actually saved away in the cache's pools. */
  subpool = svn_pool_create(pool);

  SVN_ERR(svn_cache__get((void **) &answer, &found, cache, "twenty", subpool));
  if (found)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "cache found an entry that wasn't there");
  svn_pool_clear(subpool);

  SVN_ERR(svn_cache__set(cache, "twenty", &twenty, subpool));
  svn_pool_clear(subpool);

  SVN_ERR(svn_cache__get((void **) &answer, &found, cache, "twenty", subpool));
  if (! found)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "cache failed to find entry for 'twenty'");
  if (*answer != 20)
    return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                             "expected 20 but found '%ld'", *answer);
  svn_pool_clear(subpool);

  SVN_ERR(svn_cache__set(cache, "thirty", &thirty, subpool));
  svn_pool_clear(subpool);

  SVN_ERR(svn_cache__get((void **) &answer, &found, cache, "thirty", subpool));
  if (! found)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "cache failed to find entry for 'thirty'");
  if (*answer != 30)
    return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                             "expected 30 but found '%ld'", *answer);

  if (size_is_one)
    {
      SVN_ERR(svn_cache__get((void **) &answer, &found, cache, "twenty", subpool));
      if (found)
        return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                                "cache found entry for 'twenty' that should have "
                                "expired");
    }
  svn_pool_destroy(subpool);

  return SVN_NO_ERROR;
}

static svn_error_t *
test_inprocess_cache_basic(apr_pool_t *pool)
{
  svn_cache__t *cache;

  /* Create a cache with just one entry. */
  SVN_ERR(svn_cache__create_inprocess(&cache,
                                      serialize_revnum,
                                      deserialize_revnum,
                                      APR_HASH_KEY_STRING,
                                      1,
                                      1,
                                      TRUE,
                                      "",
                                      pool));

  return basic_cache_test(cache, TRUE, pool);
}

static svn_error_t *
test_memcache_basic(const svn_test_opts_t *opts,
                    apr_pool_t *pool)
{
  svn_cache__t *cache;
  svn_config_t *config;
  svn_memcache_t *memcache = NULL;
  const char *prefix = apr_psprintf(pool,
                                    "test_memcache_basic-%" APR_TIME_T_FMT,
                                    apr_time_now());

  if (opts->config_file)
    {
      SVN_ERR(svn_config_read2(&config, opts->config_file,
                               TRUE, FALSE, pool));
      SVN_ERR(svn_cache__make_memcache_from_config(&memcache, config, pool));
    }

  if (! memcache)
    return svn_error_create(SVN_ERR_TEST_SKIPPED, NULL,
                            "not configured to use memcached");


  /* Create a memcache-based cache. */
  SVN_ERR(svn_cache__create_memcache(&cache,
                                    memcache,
                                    serialize_revnum,
                                    deserialize_revnum,
                                    APR_HASH_KEY_STRING,
                                    prefix,
                                    pool));

  return basic_cache_test(cache, FALSE, pool);
}

static svn_error_t *
test_membuffer_cache_basic(apr_pool_t *pool)
{
  svn_cache__t *cache;
  svn_membuffer_t *membuffer;
  svn_boolean_t thread_safe;

#if APR_HAS_THREADS
  thread_safe = TRUE;
#else
  thread_safe = FALSE;
#endif

  SVN_ERR(svn_cache__membuffer_cache_create(&membuffer, 10*1024, 1,
                                            thread_safe, pool));

  /* Create a cache with just one entry. */
  SVN_ERR(svn_cache__create_membuffer_cache(&cache,
                                            membuffer,
                                            serialize_revnum,
                                            deserialize_revnum,
                                            APR_HASH_KEY_STRING,
                                            "cache:",
                                            pool));

  return basic_cache_test(cache, FALSE, pool);
}


static svn_error_t *
test_memcache_long_key(const svn_test_opts_t *opts,
                       apr_pool_t *pool)
{
  svn_cache__t *cache;
  svn_config_t *config;
  svn_memcache_t *memcache = NULL;
  svn_revnum_t fifty = 50, *answer;
  svn_boolean_t found = FALSE;
  const char *prefix = apr_psprintf(pool,
                                    "test_memcache_long_key-%" APR_TIME_T_FMT,
                                    apr_time_now());
  static const char *long_key =
    "0123456789" "0123456789" "0123456789" "0123456789" "0123456789" /* 50 */
    "0123456789" "0123456789" "0123456789" "0123456789" "0123456789" /* 100 */
    "0123456789" "0123456789" "0123456789" "0123456789" "0123456789" /* 150 */
    "0123456789" "0123456789" "0123456789" "0123456789" "0123456789" /* 200 */
    "0123456789" "0123456789" "0123456789" "0123456789" "0123456789" /* 250 */
    "0123456789" "0123456789" "0123456789" "0123456789" "0123456789" /* 300 */
    ;

  if (opts->config_file)
    {
      SVN_ERR(svn_config_read2(&config, opts->config_file,
                               TRUE, FALSE, pool));
      SVN_ERR(svn_cache__make_memcache_from_config(&memcache, config, pool));
    }

  if (! memcache)
    return svn_error_create(SVN_ERR_TEST_SKIPPED, NULL,
                            "not configured to use memcached");


  /* Create a memcache-based cache. */
  SVN_ERR(svn_cache__create_memcache(&cache,
                                    memcache,
                                    serialize_revnum,
                                    deserialize_revnum,
                                    APR_HASH_KEY_STRING,
                                    prefix,
                                    pool));

  SVN_ERR(svn_cache__set(cache, long_key, &fifty, pool));
  SVN_ERR(svn_cache__get((void **) &answer, &found, cache, long_key, pool));

  if (! found)
    return svn_error_create(SVN_ERR_TEST_FAILED, NULL,
                            "cache failed to find entry for 'fifty'");
  if (*answer != 50)
    return svn_error_createf(SVN_ERR_TEST_FAILED, NULL,
                             "expected 50 but found '%ld'", *answer);

  return SVN_NO_ERROR;
}


/* The test table.  */

struct svn_test_descriptor_t test_funcs[] =
  {
    SVN_TEST_NULL,
    SVN_TEST_PASS2(test_inprocess_cache_basic,
                   "basic inprocess svn_cache test"),
    SVN_TEST_OPTS_PASS(test_memcache_basic,
                       "basic memcache svn_cache test"),
    SVN_TEST_OPTS_PASS(test_memcache_long_key,
                       "memcache svn_cache with very long keys"),
    SVN_TEST_PASS2(test_membuffer_cache_basic,
                   "basic membuffer svn_cache test"),
    SVN_TEST_NULL
  };
