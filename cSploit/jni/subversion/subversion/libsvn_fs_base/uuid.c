/* uuid.c : operations on repository uuids
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

#include "svn_pools.h"
#include "fs.h"
#include "trail.h"
#include "err.h"
#include "uuid.h"
#include "bdb/uuids-table.h"
#include "../libsvn_fs/fs-loader.h"

#include "private/svn_fs_util.h"


struct get_uuid_args
{
  int idx;
  const char **uuid;
};


static svn_error_t *
txn_body_get_uuid(void *baton, trail_t *trail)
{
  struct get_uuid_args *args = baton;
  return svn_fs_bdb__get_uuid(trail->fs, args->idx, args->uuid,
                              trail, trail->pool);
}


svn_error_t *
svn_fs_base__get_uuid(svn_fs_t *fs,
                      const char **uuid,
                      apr_pool_t *pool)
{
  base_fs_data_t *bfd = fs->fsap_data;

  SVN_ERR(svn_fs__check_fs(fs, TRUE));

  /* Check for a cached UUID first.  Failing that, we hit the
     database. */
  if (bfd->uuid)
    {
      *uuid = apr_pstrdup(pool, bfd->uuid);
    }
  else
    {
      struct get_uuid_args args;
      apr_pool_t *scratch_pool = svn_pool_create(pool);

      args.idx = 1;
      args.uuid = uuid;
      SVN_ERR(svn_fs_base__retry_txn(fs, txn_body_get_uuid, &args,
                                     FALSE, scratch_pool));

      if (*uuid)
        {
          *uuid = apr_pstrdup(pool, *uuid);

          /* Toss what we find into the cache. */
          bfd->uuid = apr_pstrdup(fs->pool, *uuid);
        }

      svn_pool_destroy(scratch_pool);
    }

  return SVN_NO_ERROR;
}


struct set_uuid_args
{
  int idx;
  const char *uuid;
};


static svn_error_t *
txn_body_set_uuid(void *baton, trail_t *trail)
{
  struct set_uuid_args *args = baton;
  return svn_fs_bdb__set_uuid(trail->fs, args->idx, args->uuid,
                              trail, trail->pool);
}


svn_error_t *
svn_fs_base__set_uuid(svn_fs_t *fs,
                      const char *uuid,
                      apr_pool_t *pool)
{
  struct set_uuid_args args;
  base_fs_data_t *bfd = fs->fsap_data;

  SVN_ERR(svn_fs__check_fs(fs, TRUE));

  if (! uuid)
    uuid = svn_uuid_generate(pool);

  args.idx = 1;
  args.uuid = uuid;
  SVN_ERR(svn_fs_base__retry_txn(fs, txn_body_set_uuid, &args, TRUE, pool));

  /* Toss our value into the cache. */
  if (uuid)
    bfd->uuid = apr_pstrdup(fs->pool, uuid);

  return SVN_NO_ERROR;
}

