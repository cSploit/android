/* pool.c:  pool wrappers for Subversion
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



#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <apr_general.h>
#include <apr_pools.h>

#include "svn_pools.h"


#if APR_POOL_DEBUG
/* file_line for the non-debug case. */
static const char SVN_FILE_LINE_UNDEFINED[] = "svn:<undefined>";
#endif /* APR_POOL_DEBUG */



/*-----------------------------------------------------------------*/


/* Pool allocation handler which just aborts, since we aren't generally
   prepared to deal with out-of-memory errors.
 */
static int
abort_on_pool_failure(int retcode)
{
  /* Don't translate this string! It requires memory allocation to do so!
     And we don't have any of it... */
  printf("Out of memory - terminating application.\n");
  abort();
}


#if APR_POOL_DEBUG
#undef svn_pool_create_ex
#endif /* APR_POOL_DEBUG */

#if !APR_POOL_DEBUG

apr_pool_t *
svn_pool_create_ex(apr_pool_t *parent_pool, apr_allocator_t *allocator)
{
  apr_pool_t *pool;
  apr_pool_create_ex(&pool, parent_pool, abort_on_pool_failure, allocator);
  return pool;
}

/* Wrapper that ensures binary compatibility */
apr_pool_t *
svn_pool_create_ex_debug(apr_pool_t *pool, apr_allocator_t *allocator,
                         const char *file_line)
{
  return svn_pool_create_ex(pool, allocator);
}

#else /* APR_POOL_DEBUG */

apr_pool_t *
svn_pool_create_ex_debug(apr_pool_t *parent_pool, apr_allocator_t *allocator,
                         const char *file_line)
{
  apr_pool_t *pool;
  apr_pool_create_ex_debug(&pool, parent_pool, abort_on_pool_failure,
                           allocator, file_line);
  return pool;
}

/* Wrapper that ensures binary compatibility */
apr_pool_t *
svn_pool_create_ex(apr_pool_t *pool, apr_allocator_t *allocator)
{
  return svn_pool_create_ex_debug(pool, allocator, SVN_FILE_LINE_UNDEFINED);
}

#endif /* APR_POOL_DEBUG */
