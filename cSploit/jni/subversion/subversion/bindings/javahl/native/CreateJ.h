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
 * @file CreateJ.h
 * @brief Interface of the class CreateJ
 */

#ifndef CREATEJ_H
#define CREATEJ_H

#include <jni.h>
#include "svn_wc.h"
#include "svn_repos.h"
#include "svn_client.h"

#include <vector>

/**
 * This class passes centralizes the creating of Java objects from
 * Subversion's C structures.
 * @since 1.6
 */
class CreateJ
{
 public:
  static jobject
  ConflictDescriptor(const svn_wc_conflict_description2_t *desc);

  static jobject
  Checksum(const svn_checksum_t *checksum);

  static jobject
  Info(const char *path, const svn_client_info2_t *info);

  static jobject
  Lock(const svn_lock_t *lock);

  static jobject
  ChangedPath(const char *path, svn_log_changed_path2_t *log_item);

  static jobject
  Status(svn_wc_context_t *wc_ctx, const svn_client_status_t *status,
         apr_pool_t *pool);

  static jobject
  ClientNotifyInformation(const svn_wc_notify_t *notify);

  static jobject
  ReposNotifyInformation(const svn_repos_notify_t *notify);

  static jobject
  CommitItem(svn_client_commit_item3_t *item);

  static jobject
  CommitInfo(const svn_commit_info_t *info);

  static jobject
  RevisionRangeList(apr_array_header_t *ranges);

  static jobject
  StringSet(apr_array_header_t *strings);

  static jobject
  PropertyMap(apr_hash_t *prop_hash);

  /* This creates a set of Objects.  It derefs the members of the vector
   * after putting them in the set, so they caller doesn't need to. */
  static jobject
  Set(std::vector<jobject> &objects);

 protected:
  static jobject
  ConflictVersion(const svn_wc_conflict_version_t *version);

  static jobject
  Collection(std::vector<jobject> &object, const char *className);
};

#endif  // CREATEJ_H
