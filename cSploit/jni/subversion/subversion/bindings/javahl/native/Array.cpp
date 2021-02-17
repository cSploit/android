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
 * @file Array.cpp
 * @brief Implementation of the class Array
 */

#include "Array.h"
#include "JNIUtil.h"
#include "JNIStringHolder.h"
#include <apr_tables.h>
#include <apr_strings.h>
#include <iostream>

Array::~Array()
{
  if (m_objectArray != NULL)
    {
      for (std::vector<jobject>::iterator it = m_objects.begin();
            it < m_objects.end(); ++it)
        {
          JNIUtil::getEnv()->DeleteLocalRef(*it);
        }

      JNIUtil::getEnv()->DeleteLocalRef(m_objectArray);
    }
}

const std::vector<jobject> &Array::vector(void) const
{
  return m_objects;
}

void
Array::init(jobjectArray jobjects)
{
  m_objectArray = jobjects;

  if (jobjects != NULL)
    {
      JNIEnv *env = JNIUtil::getEnv();
      jint arraySize = env->GetArrayLength(jobjects);
      if (JNIUtil::isExceptionThrown())
        return;

      for (int i = 0; i < arraySize; ++i)
        {
          jobject jobj = env->GetObjectArrayElement(jobjects, i);
          if (JNIUtil::isExceptionThrown())
            return;

          m_objects.push_back(jobj);
        }
    }
}

Array::Array(jobjectArray jobjects)
{
  init(jobjects);
}

Array::Array(jobject jobjectCollection)
{
  jobjectArray jobjects = NULL;

  if (jobjectCollection != NULL)
    {
      JNIEnv *env = JNIUtil::getEnv();

      jclass clazz = env->FindClass("java/util/Collection");

      static jmethodID mid_toArray = 0;
      if (mid_toArray == 0)
        {
          mid_toArray = env->GetMethodID(clazz, "toArray",
                                         "()[Ljava/lang/Object;");
          if (JNIUtil::isExceptionThrown())
            return;
        }

      jobjects = (jobjectArray) env->CallObjectMethod(jobjectCollection,
                                                      mid_toArray);

    }

  init(jobjects);
}
