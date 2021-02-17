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


#ifndef SVN_DEBUG_REPORTER_H
#define SVN_DEBUG_REPORTER_H

#include "svn_ra.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Return a debug reporter that wraps @a wrapped_reporter.
 *
 * The debug reporter simply prints an indication of what callbacks are being
 * called to @c stderr, and is only intended for use in debugging subversion
 * reporters.
 */
svn_error_t *
svn_ra__get_debug_reporter(const svn_ra_reporter3_t **reporter,
                           void **report_baton,
                           const svn_ra_reporter3_t *wrapped_reporter,
                           void *wrapped_report_baton,
                           apr_pool_t *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_DEBUG_REPORTER_H */
