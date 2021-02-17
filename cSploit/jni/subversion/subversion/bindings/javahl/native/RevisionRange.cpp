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
 * @file RevisionRanges.cpp
 * @brief Implementation of the class RevisionRange.
 */

#include <apr_pools.h>
#include "svn_client.h"
#include "svn_types.h"

#include "JNIUtil.h"
#include "RevisionRange.h"
#include "Revision.h"
#include "Pool.h"

RevisionRange::RevisionRange(jobject jrevisionRange)
{
    m_range = jrevisionRange;
}

RevisionRange::~RevisionRange()
{
    // m_range is assume to be managed externally, and thus is not
    // explicitly destroyed.
}

const svn_opt_revision_range_t *RevisionRange::toRange(SVN::Pool &pool) const
{
  JNIEnv *env = JNIUtil::getEnv();

  jclass clazz = env->FindClass(JAVA_PACKAGE"/types/RevisionRange");
  if (JNIUtil::isExceptionThrown())
    return NULL;

  static jmethodID fmid = 0;
  if (fmid == 0)
    {
      fmid = env->GetMethodID(clazz, "getFromRevision",
                              "()L"JAVA_PACKAGE"/types/Revision;");
      if (JNIUtil::isJavaExceptionThrown())
        return NULL;
    }

  static jmethodID tmid = 0;
  if (tmid == 0)
    {
      tmid = env->GetMethodID(clazz, "getToRevision",
                              "()L"JAVA_PACKAGE"/types/Revision;");
      if (JNIUtil::isJavaExceptionThrown())
        return NULL;
    }

  jobject jstartRevision = env->CallObjectMethod(m_range, fmid);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  Revision startRevision(jstartRevision);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  jobject jendRevision = env->CallObjectMethod(m_range, tmid);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  Revision endRevision(jendRevision);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  svn_opt_revision_range_t *range =
    (svn_opt_revision_range_t *) apr_palloc(pool.getPool(), sizeof(*range));

  range->start = *startRevision.revision();
  if (JNIUtil::isExceptionThrown())
    return NULL;

  range->end = *endRevision.revision();
  if (JNIUtil::isExceptionThrown())
    return NULL;

  return range;
}

jobject
RevisionRange::makeJRevisionRange(svn_merge_range_t *range)
{
    JNIEnv *env = JNIUtil::getEnv();

    jclass rangeClazz = env->FindClass(JAVA_PACKAGE "/types/RevisionRange");
    if (JNIUtil::isJavaExceptionThrown())
        return NULL;
    static jmethodID rangeCtor = 0;
    if (rangeCtor == 0)
    {
        rangeCtor = env->GetMethodID(rangeClazz, "<init>", "(JJ)V");
        if (JNIUtil::isJavaExceptionThrown())
            return NULL;
    }
    jobject jrange = env->NewObject(rangeClazz, rangeCtor,
                                    (jlong) range->start,
                                    (jlong) range->end);
    if (JNIUtil::isJavaExceptionThrown())
        return NULL;

    return jrange;
}
