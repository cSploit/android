/*
 * tree-conflicts.h: Tree conflicts.
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



#ifndef SVN_TREE_CONFLICTS_H
#define SVN_TREE_CONFLICTS_H

/*** Includes. ***/
#include <apr_pools.h>

#include "svn_types.h"
#include "svn_string.h"
#include "svn_wc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/**
 * Return in @a desc a possibly localized human readable
 * description of a tree conflict described by @a conflict.
 *
 * Allocate the result in @a pool.
 */
svn_error_t *
svn_cl__get_human_readable_tree_conflict_description(
  const char **desc,
  const svn_wc_conflict_description2_t *conflict,
  apr_pool_t *pool);

/**
 * Append to @a str an XML representation of the tree conflict data
 * for @a conflict, in a format suitable for 'svn info --xml'.
 */
svn_error_t *
svn_cl__append_tree_conflict_info_xml(
  svn_stringbuf_t *str,
  const svn_wc_conflict_description2_t *conflict,
  apr_pool_t *pool);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SVN_TREE_CONFLICTS_H */
