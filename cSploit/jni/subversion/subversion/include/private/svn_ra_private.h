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
 * @file svn_ra_private.h
 * @brief The Subversion repository access library - Internal routines
 */

#ifndef SVN_RA_PRIVATE_H
#define SVN_RA_PRIVATE_H

#include <apr_pools.h>

#include "svn_error.h"
#include "svn_ra.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Return an error with code SVN_ERR_UNSUPPORTED_FEATURE, and an error
   message referencing PATH_OR_URL, if the "server" pointed to by
   RA_SESSION doesn't support Merge Tracking (e.g. is pre-1.5).
   Perform temporary allocations in POOL. */
svn_error_t *
svn_ra__assert_mergeinfo_capable_server(svn_ra_session_t *ra_session,
                                        const char *path_or_url,
                                        apr_pool_t *pool);


/*** Operational Locks ***/

/** This is a function type which allows svn_ra__get_operational_lock()
 * to report lock attempt failures.  If non-NULL, @a locktoken is the
 * preexisting lock which prevented lock acquisition.
 *
 * @since New in 1.7.
 */
typedef svn_error_t *(*svn_ra__lock_retry_func_t)(void *baton,
                                                  const svn_string_t *locktoken,
                                                  apr_pool_t *pool);

/** Acquire a lock (of sorts) on the repository associated with the
 * given RA @a session, retrying as necessary up to @a num_retries
 * times, and set @a *lock_string_p to the value of the acquired lock
 * token.  Allocate the returned token from @a pool.  (See this
 * function's counterpart svn_ra__release_operational_lock() for your
 * lock removal needs.)
 *
 * @a lock_revprop_name is the name of the revision-0 property used to
 * store the lock.
 *
 * If @a steal_lock is set, then replace any pre-existing lock on the
 * repository with our own.  Iff such a theft occurs and
 * @a stolen_lock_p is non-NULL, set @a *stolen_lock_p to the token of
 * the lock we stole.
 *
 * Call @a retry_func with @a retry_baton each time the retry loop
 * fails to acquire a lock.
 *
 * Use @a cancel_func and @a cancel_baton to check for early
 * cancellation.
 *
 * @note If the server does not support #SVN_RA_CAPABILITY_ATOMIC_REVPROPS
 * (i.e., is a pre-1.7 server), then this function makes a "best effort"
 * attempt to obtain the lock, but is susceptible to a race condition; see
 * issue #3546.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_ra__get_operational_lock(const svn_string_t **lock_string_p,
                             const svn_string_t **stolen_lock_p,
                             svn_ra_session_t *session,
                             const char *lock_revprop_name,
                             svn_boolean_t steal_lock,
                             int num_retries,
                             svn_ra__lock_retry_func_t retry_func,
                             void *retry_baton,
                             svn_cancel_func_t cancel_func,
                             void *cancel_baton,
                             apr_pool_t *pool);

/** Release an operational lock (whose value is @a mylocktoken) on the
 * repository associated with RA @a session.  (This is the counterpart
 * to svn_ra__get_operational_lock().)
 *
 * @a lock_revprop_name is the name of the revision-0 property used to
 * store the lock.
 *
 * Use @a scratch_pool for temporary allocations.
 *
 * @since New in 1.7.
 */
svn_error_t *
svn_ra__release_operational_lock(svn_ra_session_t *session,
                                 const char *lock_revprop_name,
                                 const svn_string_t *mylocktoken,
                                 apr_pool_t *scratch_pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_RA_PRIVATE_H */
