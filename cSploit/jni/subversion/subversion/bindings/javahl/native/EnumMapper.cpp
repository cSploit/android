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
 * @file EnumMapper.cpp
 * @brief Implementation of the class EnumMapper
 */

#include "svn_types.h"
#include "svn_wc.h"
#include "svn_client.h"
#include "EnumMapper.h"
#include "JNIUtil.h"
#include "JNIStringHolder.h"
#include "../include/org_apache_subversion_javahl_CommitItemStateFlags.h"

/**
 * Map a C commit state flag constant to the Java constant.
 * @param state     the C commit state flage constant
 * @returns the Java constant
 */
jint EnumMapper::mapCommitMessageStateFlags(apr_byte_t flags)
{
  jint jstateFlags = 0;
  if (flags & SVN_CLIENT_COMMIT_ITEM_ADD)
    jstateFlags |=
      org_apache_subversion_javahl_CommitItemStateFlags_Add;
  if (flags & SVN_CLIENT_COMMIT_ITEM_DELETE)
    jstateFlags |=
      org_apache_subversion_javahl_CommitItemStateFlags_Delete;
  if (flags & SVN_CLIENT_COMMIT_ITEM_TEXT_MODS)
    jstateFlags |=
      org_apache_subversion_javahl_CommitItemStateFlags_TextMods;
  if (flags & SVN_CLIENT_COMMIT_ITEM_PROP_MODS)
    jstateFlags |=
      org_apache_subversion_javahl_CommitItemStateFlags_PropMods;
  if (flags & SVN_CLIENT_COMMIT_ITEM_IS_COPY)
    jstateFlags |=
      org_apache_subversion_javahl_CommitItemStateFlags_IsCopy;
  return jstateFlags;
}

jobject EnumMapper::mapChangePathAction(const char action)
{
  switch (action)
    {
      case 'A':
        return mapEnum(JAVA_PACKAGE"/types/ChangePath$Action", 0);
      case 'D':
        return mapEnum(JAVA_PACKAGE"/types/ChangePath$Action", 1);
      case 'R':
        return mapEnum(JAVA_PACKAGE"/types/ChangePath$Action", 2);
      case 'M':
        return mapEnum(JAVA_PACKAGE"/types/ChangePath$Action", 3);
      default:
        return NULL;
    }
}

/**
 * Map a C notify state constant to the Java constant.
 */
jobject EnumMapper::mapNotifyState(svn_wc_notify_state_t state)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/ClientNotifyInformation$Status", (int) state);
}

/**
 * Map a C notify action constant to the Java constant.
 */
jobject EnumMapper::mapNotifyAction(svn_wc_notify_action_t action)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/ClientNotifyInformation$Action", (int) action);
}

jobject EnumMapper::mapReposNotifyNodeAction(svn_node_action action)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/ReposNotifyInformation$NodeAction", (int) action);
}

/**
 * Map a C repos notify action constant to the Java constant.
 */
jobject EnumMapper::mapReposNotifyAction(svn_repos_notify_action_t action)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/ReposNotifyInformation$Action", (int) action);
}

/**
 * Map a C node kind constant to the Java constant.
 */
jobject EnumMapper::mapNodeKind(svn_node_kind_t nodeKind)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/types/NodeKind", (int) nodeKind);
}

/**
 * Map a C notify lock state constant to the Java constant.
 */
jobject EnumMapper::mapNotifyLockState(svn_wc_notify_lock_state_t state)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/ClientNotifyInformation$LockStatus", (int) state);
}

/**
 * Map a C wc schedule constant to the Java constant.
 */
jobject EnumMapper::mapScheduleKind(svn_wc_schedule_t schedule)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/types/Info$ScheduleKind", (int) schedule);
}

/**
 * Map a C wc state constant to the Java constant.
 */
jobject EnumMapper::mapStatusKind(svn_wc_status_kind svnKind)
{
  // We're assuming a valid value for the C enum above
  // The offset here is +1
  return mapEnum(JAVA_PACKAGE"/types/Status$Kind", ((int) svnKind) - 1);
}

jobject EnumMapper::mapChecksumKind(svn_checksum_kind_t kind)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/types/Checksum$Kind", (int) kind);
}

jobject EnumMapper::mapConflictKind(svn_wc_conflict_kind_t kind)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/ConflictDescriptor$Kind", (int) kind);
}

jobject EnumMapper::mapConflictAction(svn_wc_conflict_action_t action)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/ConflictDescriptor$Action", (int) action);
}

jobject EnumMapper::mapConflictReason(svn_wc_conflict_reason_t reason)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/ConflictDescriptor$Reason", (int) reason);
}

int EnumMapper::toMergeinfoLogKind(jobject jLogKind)
{
  return getOrdinal(JAVA_PACKAGE"/types/Mergeinfo$LogKind", jLogKind);
}

int EnumMapper::toLogLevel(jobject jLogLevel)
{
  return getOrdinal(JAVA_PACKAGE"/SVNClient$ClientLogLevel", jLogLevel);
}

svn_depth_t EnumMapper::toDepth(jobject jdepth)
{
  // The offset for depths is -2
  return (svn_depth_t) (getOrdinal(JAVA_PACKAGE"/types/Depth", jdepth) - 2);
}

jobject EnumMapper::mapDepth(svn_depth_t depth)
{
  // We're assuming a valid value for the C enum above
  // The offset for depths is -2
  return mapEnum(JAVA_PACKAGE"/types/Depth", ((int) depth) + 2);
}

jobject EnumMapper::mapOperation(svn_wc_operation_t operation)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/ConflictDescriptor$Operation", (int) operation);
}

jobject EnumMapper::mapTristate(svn_tristate_t tristate)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/types/Tristate",
                 (int) (tristate - svn_tristate_false));
}

svn_wc_conflict_choice_t EnumMapper::toConflictChoice(jobject jchoice)
{
  return (svn_wc_conflict_choice_t) getOrdinal(
                        JAVA_PACKAGE"/ConflictResult$Choice", jchoice);
}

svn_opt_revision_kind EnumMapper::toRevisionKind(jobject jkind)
{
  return (svn_opt_revision_kind) getOrdinal(JAVA_PACKAGE"/types/Revision$Kind",
                                            jkind);
}

jobject EnumMapper::mapSummarizeKind(svn_client_diff_summarize_kind_t sKind)
{
  // We're assuming a valid value for the C enum above
  return mapEnum(JAVA_PACKAGE"/DiffSummary$DiffKind", (int) sKind);
}

jobject EnumMapper::mapEnum(const char *clazzName, int index)
{
  // The fact that we can even do this depends upon a couple of assumptions,
  // mainly some knowledge about the orderin of the various constants in
  // both the C and Java enums.  Should those values ever change,
  // the World Will End.

  std::string methodSig("()[L");
  methodSig.append(clazzName);
  methodSig.append(";");

  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  jclass clazz = env->FindClass(clazzName);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jmethodID mid = env->GetStaticMethodID(clazz, "values", methodSig.c_str());
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jobjectArray jvalues = (jobjectArray) env->CallStaticObjectMethod(clazz, mid);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jobject jthing = env->GetObjectArrayElement(jvalues, index);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  return env->PopLocalFrame(jthing);
}

int EnumMapper::getOrdinal(const char *clazzName, jobject jenum)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return -1;

  jclass clazz = env->FindClass(clazzName);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(-1);

  jmethodID mid = env->GetMethodID(clazz, "ordinal", "()I");
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(-1);

  jint jorder = env->CallIntMethod(jenum, mid);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(-1);

  env->PopLocalFrame(NULL);
  return (int) jorder;
}
