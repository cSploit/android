/*
 * svn_fs_private.h: Private declarations for the filesystem layer to
 * be consumed by libsvn_fs* and non-libsvn_fs* modules.
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

#ifndef SVN_FS_PRIVATE_H
#define SVN_FS_PRIVATE_H

#include "svn_fs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* The maximum length of a transaction name.  The Berkeley DB backend
   generates transaction names from a sequence expressed as a base 36
   number with a maximum of MAX_KEY_SIZE (currently 200) bytes.  The
   FSFS backend generates transaction names of the form
   <rev>-<base 36-number> where the base 36 number is a sequence value
   with a maximum length of MAX_KEY_SIZE bytes.  The maximum length is
   212, but use 220 just to have some extra space:
     10   -> 32 bit revision number
     1    -> '-'
     200  -> 200 digit base 36 number
     1    -> '\0'
 */
#define SVN_FS__TXN_MAX_LEN 220

/** Retrieve the lock-tokens associated in the context @a access_ctx.
 * The tokens are in a hash keyed with <tt>const char *</tt> tokens,
 * and with <tt>const char *</tt> values for the paths associated.
 *
 * You should always use svn_fs_access_add_lock_token2() if you intend
 * to use this function.  The result of the function is not guaranteed
 * if you use it with the deprecated svn_fs_access_add_lock_token()
 * API.
 *
 * @since New in 1.6. */
apr_hash_t *
svn_fs__access_get_lock_tokens(svn_fs_access_t *access_ctx);


/* Check whether PATH is valid for a filesystem, following (most of) the
 * requirements in svn_fs.h:"Directory entry names and directory paths".
 *
 * Return SVN_ERR_FS_PATH_SYNTAX if PATH is not valid.
 */
svn_error_t *
svn_fs__path_valid(const char *path, apr_pool_t *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_FS_PRIVATE_H */
