/* caching.c : in-memory caching
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

#include "fs.h"
#include "fs_fs.h"
#include "id.h"
#include "dag.h"
#include "temp_serializer.h"
#include "../libsvn_fs/fs-loader.h"

#include "svn_config.h"
#include "svn_cache_config.h"

#include "svn_private_config.h"
#include "svn_hash.h"
#include "private/svn_debug.h"

/* Return a memcache in *MEMCACHE_P for FS if it's configured to use
   memcached, or NULL otherwise.  Also, sets *FAIL_STOP to a boolean
   indicating whether cache errors should be returned to the caller or
   just passed to the FS warning handler.  Use FS->pool for allocating
   the memcache, and POOL for temporary allocations. */
static svn_error_t *
read_config(svn_memcache_t **memcache_p,
            svn_boolean_t *fail_stop,
            svn_boolean_t *cache_txdeltas,
            svn_boolean_t *cache_fulltexts,
            svn_fs_t *fs,
            apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  SVN_ERR(svn_cache__make_memcache_from_config(memcache_p, ffd->config,
                                              fs->pool));

  /* don't cache text deltas by default.
   * Once we reconstructed the fulltexts from the deltas,
   * these deltas are rarely re-used. Therefore, only tools
   * like svnadmin will activate this to speed up operations
   * dump and verify.
   */
  *cache_txdeltas
    = svn_hash__get_bool(fs->config,
                         SVN_FS_CONFIG_FSFS_CACHE_DELTAS,
                         FALSE);
  /* by default, cache fulltexts.
   * Most SVN tools care about reconstructed file content.
   * Thus, this is a reasonable default.
   * SVN admin tools may set that to FALSE because fulltexts
   * won't be re-used rendering the cache less effective
   * by squeezing wanted data out.
   */
  *cache_fulltexts
    = svn_hash__get_bool(fs->config,
                         SVN_FS_CONFIG_FSFS_CACHE_FULLTEXTS,
                         TRUE);

  return svn_config_get_bool(ffd->config, fail_stop,
                             CONFIG_SECTION_CACHES, CONFIG_OPTION_FAIL_STOP,
                             FALSE);
}


/* Implements svn_cache__error_handler_t */
static svn_error_t *
warn_on_cache_errors(svn_error_t *err,
                     void *baton,
                     apr_pool_t *pool)
{
  svn_fs_t *fs = baton;
  (fs->warning)(fs->warning_baton, err);
  svn_error_clear(err);
  return SVN_NO_ERROR;
}

#ifdef SVN_DEBUG_CACHE_DUMP_STATS
/* Baton to be used for the dump_cache_statistics() pool cleanup function, */
struct dump_cache_baton_t
{
  /* the pool about to be cleaned up. Will be used for temp. allocations. */
  apr_pool_t *pool;

  /* the cache to dump the statistics for */
  svn_cache__t *cache;
};

/* APR pool cleanup handler that will printf the statistics of the
   cache referenced by the baton in BATON_VOID. */
static apr_status_t
dump_cache_statistics(void *baton_void)
{
  struct dump_cache_baton_t *baton = baton_void;

  apr_status_t result = APR_SUCCESS;
  svn_cache__info_t info;
  svn_string_t *text_stats;
  apr_array_header_t *lines;
  int i;

  svn_error_t *err = svn_cache__get_info(baton->cache,
                                         &info,
                                         TRUE,
                                         baton->pool);

  if (! err)
    {
      text_stats = svn_cache__format_info(&info, baton->pool);
      lines = svn_cstring_split(text_stats->data, "\n", FALSE, baton->pool);

      for (i = 0; i < lines->nelts; ++i)
        {
          const char *line = APR_ARRAY_IDX(lines, i, const char *);
#ifdef SVN_DEBUG
          SVN_DBG(("%s\n", line));
#endif
        }
    }

  /* process error returns */
  if (err)
    {
      result = err->apr_err;
      svn_error_clear(err);
    }

  return result;
}
#endif /* SVN_DEBUG_CACHE_DUMP_STATS */

/* This function sets / registers the required callbacks for a given
 * not transaction-specific CACHE object in FS, if CACHE is not NULL.
 *
 * All these svn_cache__t instances shall be handled uniformly. Unless
 * NO_HANDLER is true, register an error handler that reports errors
 * as warnings for the given CACHE.
 */
static svn_error_t *
init_callbacks(svn_cache__t *cache,
               svn_fs_t *fs,
               svn_boolean_t no_handler,
               apr_pool_t *pool)
{
  if (cache != NULL)
    {
#ifdef SVN_DEBUG_CACHE_DUMP_STATS

      /* schedule printing the access statistics upon pool cleanup,
       * i.e. end of FSFS session.
       */
      struct dump_cache_baton_t *baton;

      baton = apr_palloc(pool, sizeof(*baton));
      baton->pool = pool;
      baton->cache = cache;

      apr_pool_cleanup_register(pool,
                                baton,
                                dump_cache_statistics,
                                apr_pool_cleanup_null);
#endif

      if (! no_handler)
        SVN_ERR(svn_cache__set_error_handler(cache,
                                             warn_on_cache_errors,
                                             fs,
                                             pool));

    }

  return SVN_NO_ERROR;
}

/* Sets *CACHE_P to cache instance based on provided options.
 * Creates memcache if MEMCACHE is not NULL. Creates membuffer cache if
 * MEMBUFFER is not NULL. Fallbacks to inprocess cache if MEMCACHE and
 * MEMBUFFER are NULL and pages is non-zero.  Sets *CACHE_P to NULL
 * otherwise.
 *
 * Cache is allocated in POOL.
 * */
static svn_error_t *
create_cache(svn_cache__t **cache_p,
             svn_memcache_t *memcache,
             svn_membuffer_t *membuffer,
             apr_int64_t pages,
             apr_int64_t items_per_page,
             svn_cache__serialize_func_t serializer,
             svn_cache__deserialize_func_t deserializer,
             apr_ssize_t klen,
             const char *prefix,
             apr_pool_t *pool)
{
    if (memcache)
      {
        SVN_ERR(svn_cache__create_memcache(cache_p, memcache,
                                           serializer, deserializer, klen,
                                           prefix, pool));
      }
    else if (membuffer)
      {
        SVN_ERR(svn_cache__create_membuffer_cache(
                  cache_p, membuffer, serializer, deserializer,
                  klen, prefix, pool));
      }
    else if (pages)
      {
        SVN_ERR(svn_cache__create_inprocess(
                  cache_p, serializer, deserializer, klen, pages,
                  items_per_page, FALSE, prefix, pool));
      }
    else
    {
      *cache_p = NULL;
    }

    return SVN_NO_ERROR;
}

svn_error_t *
svn_fs_fs__initialize_caches(svn_fs_t *fs,
                             apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;
  const char *prefix = apr_pstrcat(pool,
                                   "fsfs:", ffd->uuid,
                                   "/", fs->path, ":",
                                   (char *)NULL);
  svn_memcache_t *memcache;
  svn_membuffer_t *membuffer;
  svn_boolean_t no_handler;
  svn_boolean_t cache_txdeltas;
  svn_boolean_t cache_fulltexts;

  /* Evaluating the cache configuration. */
  SVN_ERR(read_config(&memcache,
                      &no_handler,
                      &cache_txdeltas,
                      &cache_fulltexts,
                      fs,
                      pool));

  membuffer = svn_cache__get_global_membuffer_cache();

  /* Make the cache for revision roots.  For the vast majority of
   * commands, this is only going to contain a few entries (svnadmin
   * dump/verify is an exception here), so to reduce overhead let's
   * try to keep it to just one page.  I estimate each entry has about
   * 72 bytes of overhead (svn_revnum_t key, svn_fs_id_t +
   * id_private_t + 3 strings for value, and the cache_entry); the
   * default pool size is 8192, so about a hundred should fit
   * comfortably. */
  SVN_ERR(create_cache(&(ffd->rev_root_id_cache),
                       NULL,
                       membuffer,
                       1, 100,
                       svn_fs_fs__serialize_id,
                       svn_fs_fs__deserialize_id,
                       sizeof(svn_revnum_t),
                       apr_pstrcat(pool, prefix, "RRI", (char *)NULL),
                       fs->pool));

  SVN_ERR(init_callbacks(ffd->rev_root_id_cache, fs, no_handler, pool));

  /* Rough estimate: revision DAG nodes have size around 320 bytes, so
   * let's put 16 on a page. */
  SVN_ERR(create_cache(&(ffd->rev_node_cache),
                       NULL,
                       membuffer,
                       1024, 16,
                       svn_fs_fs__dag_serialize,
                       svn_fs_fs__dag_deserialize,
                       APR_HASH_KEY_STRING,
                       apr_pstrcat(pool, prefix, "DAG", NULL),
                       fs->pool));

  SVN_ERR(init_callbacks(ffd->rev_node_cache, fs, no_handler, pool));

  /* Very rough estimate: 1K per directory. */
  SVN_ERR(create_cache(&(ffd->dir_cache),
                       NULL,
                       membuffer,
                       1024, 8,
                       svn_fs_fs__serialize_dir_entries,
                       svn_fs_fs__deserialize_dir_entries,
                       APR_HASH_KEY_STRING,
                       apr_pstrcat(pool, prefix, "DIR", (char *)NULL),
                       fs->pool));

  SVN_ERR(init_callbacks(ffd->dir_cache, fs, no_handler, pool));

  /* Only 16 bytes per entry (a revision number + the corresponding offset).
     Since we want ~8k pages, that means 512 entries per page. */
  SVN_ERR(create_cache(&(ffd->packed_offset_cache),
                       NULL,
                       membuffer,
                       32, 1,
                       svn_fs_fs__serialize_manifest,
                       svn_fs_fs__deserialize_manifest,
                       sizeof(svn_revnum_t),
                       apr_pstrcat(pool, prefix, "PACK-MANIFEST",
                                   (char *)NULL),
                       fs->pool));

  SVN_ERR(init_callbacks(ffd->packed_offset_cache, fs, no_handler, pool));

  /* initialize fulltext cache as configured */
  ffd->fulltext_cache = NULL;
  if (cache_fulltexts)
    {
      SVN_ERR(create_cache(&(ffd->fulltext_cache),
                           memcache,
                           membuffer,
                           0, 0, /* Do not use inprocess cache */
                           /* Values are svn_string_t */
                           NULL, NULL,
                           APR_HASH_KEY_STRING,
                           apr_pstrcat(pool, prefix, "TEXT", (char *)NULL),
                           fs->pool));
    }

  SVN_ERR(init_callbacks(ffd->fulltext_cache, fs, no_handler, pool));

  /* initialize txdelta window cache, if that has been enabled */
  if (cache_txdeltas)
    {
      SVN_ERR(create_cache(&(ffd->txdelta_window_cache),
                           NULL,
                           membuffer,
                           0, 0, /* Do not use inprocess cache */
                           svn_fs_fs__serialize_txdelta_window,
                           svn_fs_fs__deserialize_txdelta_window,
                           APR_HASH_KEY_STRING,
                           apr_pstrcat(pool, prefix, "TXDELTA_WINDOW",
                                       (char *)NULL),
                           fs->pool));
    }
  else
    {
      ffd->txdelta_window_cache = NULL;
    }

  SVN_ERR(init_callbacks(ffd->txdelta_window_cache, fs, no_handler, pool));

  /* initialize node revision cache, if caching has been enabled */
  SVN_ERR(create_cache(&(ffd->node_revision_cache),
                       NULL,
                       membuffer,
                       0, 0, /* Do not use inprocess cache */
                       svn_fs_fs__serialize_node_revision,
                       svn_fs_fs__deserialize_node_revision,
                       APR_HASH_KEY_STRING,
                       apr_pstrcat(pool, prefix, "NODEREVS", (char *)NULL),
                       fs->pool));

  SVN_ERR(init_callbacks(ffd->node_revision_cache, fs, no_handler, pool));

  return SVN_NO_ERROR;
}

/* Baton to be used for the remove_txn_cache() pool cleanup function, */
struct txn_cleanup_baton_t
{
  /* the cache to reset */
  svn_cache__t *txn_cache;

  /* the position where to reset it */
  svn_cache__t **to_reset;
};

/* APR pool cleanup handler that will reset the cache pointer given in
   BATON_VOID. */
static apr_status_t
remove_txn_cache(void *baton_void)
{
  struct txn_cleanup_baton_t *baton = baton_void;

  /* be careful not to hurt performance by resetting newer txn's caches. */
  if (*baton->to_reset == baton->txn_cache)
    {
     /* This is equivalent to calling svn_fs_fs__reset_txn_caches(). */
      *baton->to_reset  = NULL;
    }

  return  APR_SUCCESS;
}

/* This function sets / registers the required callbacks for a given
 * transaction-specific *CACHE object, if CACHE is not NULL and a no-op
 * otherwise. In particular, it will ensure that *CACHE gets reset to NULL
 * upon POOL destruction latest.
 */
static void
init_txn_callbacks(svn_cache__t **cache,
                   apr_pool_t *pool)
{
  if (*cache != NULL)
    {
      struct txn_cleanup_baton_t *baton;

      baton = apr_palloc(pool, sizeof(*baton));
      baton->txn_cache = *cache;
      baton->to_reset = cache;

      apr_pool_cleanup_register(pool,
                                baton,
                                remove_txn_cache,
                                apr_pool_cleanup_null);
    }
}

svn_error_t *
svn_fs_fs__initialize_txn_caches(svn_fs_t *fs,
                                 const char *txn_id,
                                 apr_pool_t *pool)
{
  fs_fs_data_t *ffd = fs->fsap_data;

  /* Transaction content needs to be carefully prefixed to virtually
     eliminate any chance for conflicts. The (repo, txn_id) pair
     should be unique but if a transaction fails, it might be possible
     to start a new transaction later that receives the same id.
     Therefore, throw in a uuid as well - just to be sure. */
  const char *prefix = apr_pstrcat(pool,
                                   "fsfs:", ffd->uuid,
                                   "/", fs->path,
                                   ":", txn_id,
                                   ":", svn_uuid_generate(pool), ":",
                                   (char *)NULL);

  /* We don't support caching for concurrent transactions in the SAME
   * FSFS session. Maybe, you forgot to clean POOL. */
  if (ffd->txn_dir_cache != NULL || ffd->concurrent_transactions)
    {
      ffd->txn_dir_cache = NULL;
      ffd->concurrent_transactions = TRUE;

      return SVN_NO_ERROR;
    }

  /* create a txn-local directory cache */
  if (svn_cache__get_global_membuffer_cache())
    SVN_ERR(svn_cache__create_membuffer_cache(&(ffd->txn_dir_cache),
                                              svn_cache__get_global_membuffer_cache(),
                                              svn_fs_fs__serialize_dir_entries,
                                              svn_fs_fs__deserialize_dir_entries,
                                              APR_HASH_KEY_STRING,
                                              apr_pstrcat(pool, prefix, "TXNDIR",
                                                          (char *)NULL),
                                              pool));
  else
    SVN_ERR(svn_cache__create_inprocess(&(ffd->txn_dir_cache),
                                        svn_fs_fs__serialize_dir_entries,
                                        svn_fs_fs__deserialize_dir_entries,
                                        APR_HASH_KEY_STRING,
                                        1024, 8, FALSE,
                                        apr_pstrcat(pool, prefix, "TXNDIR",
                                            (char *)NULL),
                                        pool));

  /* reset the transaction-specific cache if the pool gets cleaned up. */
  init_txn_callbacks(&(ffd->txn_dir_cache), pool);

  return SVN_NO_ERROR;
}

void
svn_fs_fs__reset_txn_caches(svn_fs_t *fs)
{
  /* we can always just reset the caches. This may degrade performance but
   * can never cause in incorrect behavior. */

  fs_fs_data_t *ffd = fs->fsap_data;
  ffd->txn_dir_cache = NULL;
}
