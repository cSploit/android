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
 * @file EnumMapper.h
 * @brief Interface of the class EnumMapper
 */

#ifndef ENUM_MAPPER_H
#define ENUM_MAPPER_H

#include <jni.h>
#include "svn_client.h"
#include "svn_wc.h"
#include "svn_repos.h"
#include "svn_types.h"

class JNIStringHolder;

/**
 * This class contains all the mappers between the C enum's and the
 * matching Java enums's.
 */
class EnumMapper
{
 public:
  /* Converting to C enum's */
  static svn_depth_t toDepth(jobject jdepth);
  static svn_opt_revision_kind toRevisionKind(jobject jkind);
  static svn_wc_conflict_choice_t toConflictChoice(jobject jchoice);
  static int toMergeinfoLogKind(jobject jLogKind);
  static int toLogLevel(jobject jLogLevel);

  /* Converting from C enum's */
  static jint mapCommitMessageStateFlags(apr_byte_t flags);
  static jobject mapChangePathAction(const char action);
  static jobject mapNotifyState(svn_wc_notify_state_t state);
  static jobject mapNotifyAction(svn_wc_notify_action_t action);
  static jobject mapReposNotifyNodeAction(svn_node_action action);
  static jobject mapReposNotifyAction(svn_repos_notify_action_t action);
  static jobject mapNodeKind(svn_node_kind_t nodeKind);
  static jobject mapNotifyLockState(svn_wc_notify_lock_state_t state);
  static jobject mapStatusKind(svn_wc_status_kind svnKind);
  static jobject mapScheduleKind(svn_wc_schedule_t schedule);
  static jobject mapChecksumKind(svn_checksum_kind_t kind);
  static jobject mapConflictKind(svn_wc_conflict_kind_t kind);
  static jobject mapConflictAction(svn_wc_conflict_action_t action);
  static jobject mapConflictReason(svn_wc_conflict_reason_t reason);
  static jobject mapDepth(svn_depth_t depth);
  static jobject mapOperation(svn_wc_operation_t);
  static jobject mapTristate(svn_tristate_t);
  static jobject mapSummarizeKind(svn_client_diff_summarize_kind_t);
 private:
  static jobject mapEnum(const char *clazzName, int offset);
  static int getOrdinal(const char *clazzName, jobject jenum);
};

#endif  // ENUM_MAPPER_H
