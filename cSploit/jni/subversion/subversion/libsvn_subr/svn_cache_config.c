/* svn_cache_config.c : configuration of internal caches
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

#include <apr_atomic.h>

#include "svn_cache_config.h"
#include "private/svn_cache.h"

#include "svn_pools.h"

/* The cache settings as a process-wide singleton.
 */
static svn_cache_config_t cache_settings =
  {
    /* default configuration:
     *
     * Please note that the resources listed below will be allocated
     * PER PROCESS. Thus, the defaults chosen here are kept deliberately
     * low to still make a difference yet to ensure that pre-fork servers
     * on machines with small amounts of RAM aren't severely impacted.
     */
    0x1000000,   /* 16 MB for caches.
                  * If you are running a single server process,
                  * you may easily increase that to 50+% of your RAM
                  * using svn_fs_set_cache_config().
                  */
    16,          /* up to 16 files kept open.
                  * Most OS restrict the number of open file handles to
                  * about 1000. To minimize I/O and OS overhead, values
                  * of 500+ can be beneficial (use svn_fs_set_cache_config()
                  * to change the configuration).
                  * When running with a huge in-process cache, this number
                  * has little impact on performance and a more modest
                  * value (< 100) may be more suitable.
                  */
#if APR_HAS_THREADS
    FALSE        /* assume multi-threaded operation.
                  * Because this simply activates proper synchronization
                  * between threads, it is a safe default.
                  */
#else
    TRUE         /* single-threaded is the only supported mode of operation */
#endif
};

/* Get the current FSFS cache configuration. */
const svn_cache_config_t *
svn_cache_config_get(void)
{
  return &cache_settings;
}

/* Access the process-global (singleton) membuffer cache. The first call
 * will automatically allocate the cache using the current cache config.
 * NULL will be returned if the desired cache size is 0 or if the cache
 * could not be created for some reason.
 */
svn_membuffer_t *
svn_cache__get_global_membuffer_cache(void)
{
  static svn_membuffer_t * volatile cache = NULL;

  apr_uint64_t cache_size = cache_settings.cache_size;
  if (!cache && cache_size)
    {
      svn_error_t *err;

      svn_membuffer_t *old_cache = NULL;
      svn_membuffer_t *new_cache = NULL;

      /* auto-allocate cache */
      apr_allocator_t *allocator = NULL;
      apr_pool_t *pool = NULL;

      if (apr_allocator_create(&allocator))
        return NULL;

      /* Ensure that we free partially allocated data if we run OOM
       * before the cache is complete: If the cache cannot be allocated
       * in its full size, the create() function will clear the pool
       * explicitly. The allocator will make sure that any memory no
       * longer used by the pool will actually be returned to the OS.
       *
       * Please note that this pool and allocator is used *only* to
       * allocate the large membuffer. All later dynamic allocations
       * come from other, temporary pools and allocators.
       */
      apr_allocator_max_free_set(allocator, 1);

      /* don't terminate upon OOM but make pool return a NULL pointer
       * instead so we can disable caching gracefully and continue
       * operation without membuffer caches.
       */
      apr_pool_create_ex(&pool, NULL, NULL, allocator);
      if (pool == NULL)
        return NULL;
      apr_allocator_owner_set(allocator, pool);

      err = svn_cache__membuffer_cache_create(
          &new_cache,
          (apr_size_t)cache_size,
          (apr_size_t)(cache_size / 16),
          ! svn_cache_config_get()->single_threaded,
          pool);

      /* Some error occured. Most likely it's an OOM error but we don't
       * really care. Simply release all cache memory and disable caching
       */
      if (err)
        {
          /* Memory and error cleanup */
          svn_error_clear(err);
          apr_pool_destroy(pool);

          /* Prevent future attempts to create the cache. However, an
           * existing cache instance (see next comment) remains valid.
           */
          cache_settings.cache_size = 0;

          /* The current caller won't get the cache object.
           * However, a concurrent call might have succeeded in creating
           * the cache object. That call and all following ones will then
           * use the successfully created cache instance.
           */
          return NULL;
        }

      /* Handle race condition: if we are the first to create a
       * cache object, make it our global singleton. Otherwise,
       * discard the new cache and keep the existing one.
       *
       * Cast is necessary because of APR bug:
       * https://issues.apache.org/bugzilla/show_bug.cgi?id=50731
       */
      old_cache = apr_atomic_casptr((volatile void **)&cache, new_cache, NULL);
      if (old_cache != NULL)
        apr_pool_destroy(pool);
    }

  return cache;
}

void
svn_cache_config_set(const svn_cache_config_t *settings)
{
  cache_settings = *settings;
}

