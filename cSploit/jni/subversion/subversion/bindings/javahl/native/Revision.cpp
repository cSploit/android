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
 * @file Revision.cpp
 * @brief Implementation of the class Revision
 */

#include "Revision.h"
#include "JNIUtil.h"
#include "EnumMapper.h"

const svn_opt_revision_kind Revision::START = svn_opt_revision_unspecified;
const svn_opt_revision_kind Revision::HEAD = svn_opt_revision_head;

Revision::Revision (const svn_opt_revision_kind kind)
{
  m_revision.kind = kind;
  m_revision.value.number = 0;
}

Revision::Revision(jobject jthis, bool headIfUnspecified,
                   bool oneIfUnspecified)
{
  if (jthis == NULL)
    {
      m_revision.kind = svn_opt_revision_unspecified;
      m_revision.value.number = 0;
    }
  else
    {
      JNIEnv *env = JNIUtil::getEnv();

      // Create a local frame for our references
      env->PushLocalFrame(LOCAL_FRAME_SIZE);
      if (JNIUtil::isJavaExceptionThrown())
        return;

      static jfieldID fid = 0;
      if (fid == 0)
        {
          jclass clazz = env->FindClass(JAVA_PACKAGE"/types/Revision");
          if (JNIUtil::isJavaExceptionThrown())
            POP_AND_RETURN_NOTHING();

          fid = env->GetFieldID(clazz, "revKind",
                                "L"JAVA_PACKAGE"/types/Revision$Kind;");
          if (JNIUtil::isJavaExceptionThrown())
            POP_AND_RETURN_NOTHING();
        }
      jobject jKind = env->GetObjectField(jthis, fid);
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN_NOTHING();

      m_revision.value.number = 0;
      m_revision.kind = EnumMapper::toRevisionKind(jKind);

      switch(m_revision.kind)
        {
        case svn_opt_revision_number:
          {
            static jfieldID fidNum = 0;
            if (fidNum == 0)
              {
                jclass clazz =
                  env->FindClass(JAVA_PACKAGE"/types/Revision$Number");
                if (JNIUtil::isJavaExceptionThrown())
                  POP_AND_RETURN_NOTHING();

                fidNum = env->GetFieldID(clazz, "revNumber", "J");
                if (JNIUtil::isJavaExceptionThrown())
                  POP_AND_RETURN_NOTHING();
              }
            jlong jNumber = env->GetLongField(jthis, fidNum);
            m_revision.value.number = (svn_revnum_t) jNumber;
          }
          break;
        case svn_opt_revision_date:
          {
            static jfieldID fidDate = 0;
            if (fidDate == 0)
              {
                jclass clazz =
                  env->FindClass(JAVA_PACKAGE"/types/Revision$DateSpec");
                if (JNIUtil::isJavaExceptionThrown())
                  POP_AND_RETURN_NOTHING();

                fidDate = env->GetFieldID(clazz, "revDate",
                                          "Ljava/util/Date;");
                if (JNIUtil::isJavaExceptionThrown())
                  POP_AND_RETURN_NOTHING();
              }
            jobject jDate = env->GetObjectField(jthis, fidDate);
            if (JNIUtil::isJavaExceptionThrown())
              POP_AND_RETURN_NOTHING();

            static jmethodID mid = 0;
            if (mid == 0)
              {
                jclass clazz = env->FindClass("java/util/Date");
                if (JNIUtil::isJavaExceptionThrown())
                  POP_AND_RETURN_NOTHING();

                mid = env->GetMethodID(clazz, "getTime", "()J");
                if (JNIUtil::isJavaExceptionThrown())
                  POP_AND_RETURN_NOTHING();
              }
            jlong jMillSec = env->CallLongMethod(jDate, mid);
            if (JNIUtil::isJavaExceptionThrown())
              POP_AND_RETURN_NOTHING();

            m_revision.value.date = jMillSec * 1000;
          }
          break;
        default:
          /* None of the other revision kinds need special handling. */
          break;
        }
      env->PopLocalFrame(NULL);
    }
  if (headIfUnspecified && m_revision.kind == svn_opt_revision_unspecified)
    m_revision.kind = svn_opt_revision_head;
  else if (oneIfUnspecified && m_revision.kind == svn_opt_revision_unspecified)
    {
      m_revision.kind = svn_opt_revision_number;
      m_revision.value.number = 1;
    }
}

Revision::~Revision()
{
}

const svn_opt_revision_t *Revision::revision () const
{
  return &m_revision;
}

jobject
Revision::makeJRevision(svn_revnum_t rev)
{
  JNIEnv *env = JNIUtil::getEnv();
  jclass clazz = env->FindClass(JAVA_PACKAGE "/types/Revision");
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  static jmethodID getInstance = 0;
  if (getInstance == 0)
    {
      getInstance = env->GetStaticMethodID(clazz, "getInstance",
                                           "(J)L" JAVA_PACKAGE "/types/Revision;");
      if (JNIUtil::isExceptionThrown())
        return NULL;
    }

  jobject jrevision = env->CallStaticObjectMethod(clazz, getInstance,
                                                  (jlong) rev);
  if (JNIUtil::isExceptionThrown())
    return NULL;

  return jrevision;
}
