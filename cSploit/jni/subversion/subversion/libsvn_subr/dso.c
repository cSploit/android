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

#include <apr_thread_mutex.h>
#include <apr_hash.h>

#include "svn_dso.h"
#include "svn_pools.h"
#include "svn_private_config.h"

/* A mutex to protect our global pool and cache. */
#if APR_HAS_THREADS
static apr_thread_mutex_t *dso_mutex;
#endif

/* Global pool to allocate DSOs in. */
static apr_pool_t *dso_pool;

/* Global cache for storing DSO objects. */
static apr_hash_t *dso_cache;

/* Just an arbitrary location in memory... */
static int not_there_sentinel;

/* A specific value we store in the dso_cache to indicate that the
   library wasn't found.  This keeps us from allocating extra memory
   from dso_pool when trying to find libraries we already know aren't
   there.  */
#define NOT_THERE ((void *) &not_there_sentinel)

svn_error_t *
svn_dso_initialize2(void)
{
#if APR_HAS_THREADS
  apr_status_t status;
#endif
  if (dso_pool)
    return SVN_NO_ERROR;

  dso_pool = svn_pool_create(NULL);

#if APR_HAS_THREADS
  status = apr_thread_mutex_create(&dso_mutex,
                                   APR_THREAD_MUTEX_DEFAULT, dso_pool);
  if (status)
    return svn_error_wrap_apr(status, _("Can't create DSO mutex"));
#endif

  dso_cache = apr_hash_make(dso_pool);
  return SVN_NO_ERROR;
}

#if APR_HAS_DSO
svn_error_t *
svn_dso_load(apr_dso_handle_t **dso, const char *fname)
{
  apr_status_t status;

  if (! dso_pool)
    SVN_ERR(svn_dso_initialize2());

#if APR_HAS_THREADS
  status = apr_thread_mutex_lock(dso_mutex);
  if (status)
    return svn_error_wrap_apr(status, _("Can't grab DSO mutex"));
#endif

  *dso = apr_hash_get(dso_cache, fname, APR_HASH_KEY_STRING);

  /* First check to see if we've been through this before...  We do this
     to avoid calling apr_dso_load multiple times for a given library,
     which would result in wasting small amounts of memory each time. */
  if (*dso == NOT_THERE)
    {
      *dso = NULL;
#if APR_HAS_THREADS
      status = apr_thread_mutex_unlock(dso_mutex);
      if (status)
        return svn_error_wrap_apr(status, _("Can't ungrab DSO mutex"));
#endif
      return SVN_NO_ERROR;
    }

  /* If we got nothing back from the cache, try and load the library. */
  if (! *dso)
    {
      status = apr_dso_load(dso, fname, dso_pool);
      if (status)
        {
#if 0
          char buf[1024];
          fprintf(stderr, "%s\n", apr_dso_error(*dso, buf, 1024));
#endif
          *dso = NULL;

          /* It wasn't found, so set the special "we didn't find it" value. */
          apr_hash_set(dso_cache,
                       apr_pstrdup(dso_pool, fname),
                       APR_HASH_KEY_STRING,
                       NOT_THERE);

#if APR_HAS_THREADS
          status = apr_thread_mutex_unlock(dso_mutex);
          if (status)
            return svn_error_wrap_apr(status, _("Can't ungrab DSO mutex"));
#endif
          return SVN_NO_ERROR;
        }

      /* Stash the dso so we can use it next time. */
      apr_hash_set(dso_cache,
                   apr_pstrdup(dso_pool, fname),
                   APR_HASH_KEY_STRING,
                   *dso);
    }

#if APR_HAS_THREADS
  status = apr_thread_mutex_unlock(dso_mutex);
  if (status)
    return svn_error_wrap_apr(status, _("Can't ungrab DSO mutex"));
#endif

  return SVN_NO_ERROR;
}
#endif /* APR_HAS_DSO */
