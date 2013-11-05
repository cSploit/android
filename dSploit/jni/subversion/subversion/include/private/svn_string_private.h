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
 * @file svn_string_private.h
 * @brief Non-public string utility functions.
 */


#ifndef SVN_STRING_PRIVATE_H
#define SVN_STRING_PRIVATE_H

#include "svn_string.h"    /* for svn_boolean_t, svn_error_t */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup svn_string String handling
 * @{
 */


/** Private functions.
 *
 * @defgroup svn_string_private Private functions
 * @{
 */

/** Returns the #svn_string_t information contained in the data and
 * len members of @a strbuf. This is effectively a typecast, converting
 * @a strbuf into an #svn_string_t. This first will become invalid and must
 * not be accessed after this function returned.
 */
svn_string_t *
svn_stringbuf__morph_into_string(svn_stringbuf_t *strbuf);

/** Like apr_strtoff but provided here for backward compatibility
 *  with APR 0.9 */
apr_status_t
svn__strtoff(apr_off_t *offset, const char *buf, char **end, int base);
/** @} */

/** @} */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* SVN_STRING_PRIVATE_H */
