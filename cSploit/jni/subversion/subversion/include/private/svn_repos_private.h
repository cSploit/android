/**
 * @copyright
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
 * @endcopyright
 *
 * @file svn_repos_private.h
 * @brief Subversion-internal repos APIs.
 */

#ifndef SVN_REPOS_PRIVATE_H
#define SVN_REPOS_PRIVATE_H

#include <apr_pools.h>

#include "svn_repos.h"
#include "svn_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Validate that property @a name is valid for use in a Subversion
 * repository; return @c SVN_ERR_REPOS_BAD_ARGS if it isn't.  For some
 * "svn:" properties, also validate the @a value, and return
 * @c SVN_ERR_BAD_PROPERTY_VALUE if it is not valid.
 *
 * Use @a pool for temporary allocations.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_repos__validate_prop(const char *name,
                         const svn_string_t *value,
                         apr_pool_t *pool);

/**
 * Given the error @a err from svn_repos_fs_commit_txn(), return an
 * string containing either or both of the svn_fs_commit_txn() error
 * and the SVN_ERR_REPOS_POST_COMMIT_HOOK_FAILED wrapped error from
 * the post-commit hook.  Any error tracing placeholders in the error
 * chain are skipped over.
 *
 * This function does not modify @a err.
 *
 * ### This method should not be necessary, but there are a few
 * ### places, e.g. mod_dav_svn, where only a single error message
 * ### string is returned to the caller and it is useful to have both
 * ### error messages included in the message.
 *
 * Use @a pool to do any allocations in.
 *
 * @since New in 1.7.
 */
const char *
svn_repos__post_commit_error_str(svn_error_t *err,
                                 apr_pool_t *pool);

/* A repos version of svn_fs_type */
svn_error_t *
svn_repos__fs_type(const char **fs_type,
                   const char *repos_path,
                   apr_pool_t *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_REPOS_PRIVATE_H */
