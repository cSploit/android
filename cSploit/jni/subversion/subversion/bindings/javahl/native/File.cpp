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
 * @file File.cpp
 * @brief Implementation of the class File
 */

#include "svn_path.h"

#include "File.h"
#include "JNIUtil.h"
#include "JNIByteArray.h"

/**
 * Create an File object.
 * @param jthis the Java object to be stored
 */
File::File(jobject jthis)
  : stringHolder(NULL)
{
  m_jthis = jthis;
}

File::~File()
{
  // The m_jthis does not need to be destroyed, because it is the
  // passed in parameter to the Java method.
  delete stringHolder;
}

/**
 * Create an absolute path from the File object and return it.
 * @return the input stream
 */
const char *File::getAbsPath()
{
  if (stringHolder == NULL)
    {
      if (m_jthis == NULL)
        return NULL;

      JNIEnv *env = JNIUtil::getEnv();

      jclass clazz = env->FindClass("java/io/File");
      if (JNIUtil::isJavaExceptionThrown())
        return NULL;

      static jmethodID mid = 0;
      if (mid == 0)
        {
          mid = env->GetMethodID(clazz, "getAbsolutePath",
                                 "()Ljava/lang/String;");
          if (JNIUtil::isJavaExceptionThrown())
            return NULL;
        }

      jstring jabsolutePath = (jstring) env->CallObjectMethod(m_jthis, mid);
      if (JNIUtil::isJavaExceptionThrown())
        return NULL;

      stringHolder = new JNIStringHolder(jabsolutePath);

      /* We don't remove the local ref for jabsolutePath here, because
         JNIStringHolder expects that ref to be valid for the life of
         the object, which in this case is allocated on the stack.
         
         So we just "leak" the reference, and it will get cleaned up when
         we eventually exit back to Java-land. */
      env->DeleteLocalRef(clazz);
    }

  return static_cast<const char *>(*stringHolder);
}

const char *File::getInternalStyle(const SVN::Pool &requestPool)
{
  const char *path = getAbsPath();
  if (path)
    return svn_dirent_internal_style(path, requestPool.getPool());
  else
    return NULL;
}

bool File::isNull() const
{
  return m_jthis == NULL;
}
