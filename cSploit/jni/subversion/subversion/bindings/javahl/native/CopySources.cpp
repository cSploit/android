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
 * @file CopySources.cpp
 * @brief Implementation of the class CopySources
 */

#include <apr_pools.h>
#include "svn_client.h"

#include "Pool.h"
#include "JNIUtil.h"
#include "JNIStringHolder.h"
#include "Revision.h"
#include "CopySources.h"

CopySources::CopySources(Array &copySources)
    : m_copySources(copySources)
{
}

CopySources::~CopySources()
{
}

jobject
CopySources::makeJCopySource(const char *path, svn_revnum_t rev, SVN::Pool &pool)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  jobject jpath = JNIUtil::makeJString(path);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jobject jrevision = Revision::makeJRevision(rev);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  jclass clazz = env->FindClass(JAVA_PACKAGE "/types/CopySource");
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  static jmethodID ctor = 0;
  if (ctor == 0)
    {
      ctor = env->GetMethodID(clazz, "<init>",
                              "(Ljava/lang/String;"
                              "L" JAVA_PACKAGE "/types/Revision;"
                              "L" JAVA_PACKAGE "/types/Revision;)V");
      if (JNIUtil::isExceptionThrown())
        POP_AND_RETURN_NULL;
    }

  jobject jcopySource = env->NewObject(clazz, ctor, jpath, jrevision, NULL);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN_NULL;

  return env->PopLocalFrame(jcopySource);
}

apr_array_header_t *
CopySources::array(SVN::Pool &pool)
{
  apr_pool_t *p = pool.getPool();

  JNIEnv *env = JNIUtil::getEnv();
  jclass clazz = env->FindClass(JAVA_PACKAGE "/types/CopySource");
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  std::vector<jobject> sources = m_copySources.vector();

  apr_array_header_t *copySources =
    apr_array_make(p, sources.size(), sizeof(svn_client_copy_source_t *));
  for (std::vector<jobject>::const_iterator it = sources.begin();
        it < sources.end(); ++it)
    {
      svn_client_copy_source_t *src =
        (svn_client_copy_source_t *) apr_palloc(p, sizeof(*src));

      // Extract the path or URL from the copy source.
      static jmethodID getPath = 0;
      if (getPath == 0)
        {
          getPath = env->GetMethodID(clazz, "getPath",
                                     "()Ljava/lang/String;");
          if (JNIUtil::isJavaExceptionThrown() || getPath == 0)
            return NULL;
        }
      jstring jpath = (jstring)
        env->CallObjectMethod(*it, getPath);
      if (JNIUtil::isJavaExceptionThrown())
        return NULL;

      const char *path = env->GetStringUTFChars(jpath, NULL);
      if (JNIUtil::isJavaExceptionThrown())
        return NULL;

      src->path = apr_pstrdup(p, path);
      env->ReleaseStringUTFChars(jpath, path);
      SVN_JNI_ERR(JNIUtil::preprocessPath(src->path, pool.getPool()),
                  NULL);
      env->DeleteLocalRef(jpath);

      // Extract source revision from the copy source.
      static jmethodID getRevision = 0;
      if (getRevision == 0)
        {
          getRevision = env->GetMethodID(clazz, "getRevision",
                                         "()L"JAVA_PACKAGE"/types/Revision;");
          if (JNIUtil::isJavaExceptionThrown() || getRevision == 0)
            return NULL;
        }
      jobject jrev = env->CallObjectMethod(*it, getRevision);
      if (JNIUtil::isJavaExceptionThrown())
        return NULL;

      // TODO: Default this to svn_opt_revision_undefined (or HEAD)
      Revision rev(jrev);
      src->revision = (const svn_opt_revision_t *)
        apr_palloc(p, sizeof(*src->revision));
      memcpy((void *) src->revision, rev.revision(),
             sizeof(*src->revision));
      env->DeleteLocalRef(jrev);

      // Extract pegRevision from the copy source.
      static jmethodID getPegRevision = 0;
      if (getPegRevision == 0)
        {
          getPegRevision = env->GetMethodID(clazz, "getPegRevision",
                                            "()L"JAVA_PACKAGE"/types/Revision;");
          if (JNIUtil::isJavaExceptionThrown() || getPegRevision == 0)
            return NULL;
        }
      jobject jPegRev = env->CallObjectMethod(*it, getPegRevision);
      if (JNIUtil::isJavaExceptionThrown())
        return NULL;

      Revision pegRev(jPegRev, true);
      src->peg_revision = (const svn_opt_revision_t *)
        apr_palloc(p, sizeof(*src->peg_revision));
      memcpy((void *) src->peg_revision, pegRev.revision(),
             sizeof(*src->peg_revision));
      env->DeleteLocalRef(jPegRev);

      APR_ARRAY_PUSH(copySources, svn_client_copy_source_t *) = src;
    }

  env->DeleteLocalRef(clazz);

  return copySources;
}
