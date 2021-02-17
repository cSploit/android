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
 * @file JNIStringHolder.cpp
 * @brief Implementation of the class JNIStringHolder
 */

#include <jni.h>
#include <apr_pools.h>
#include <apr_strings.h>
#include "JNIStringHolder.h"
#include "JNIUtil.h"

JNIStringHolder::JNIStringHolder(jstring jtext)
{
  if (jtext == NULL)
    {
      m_str = NULL;
      m_jtext = NULL;
      return;
    }
  m_str = JNIUtil::getEnv()->GetStringUTFChars(jtext, NULL);
  m_jtext = jtext;
  m_env = JNIUtil::getEnv();
}

JNIStringHolder::~JNIStringHolder()
{
  if (m_jtext && m_str)
    m_env->ReleaseStringUTFChars(m_jtext, m_str);
}

const char *
JNIStringHolder::pstrdup(apr_pool_t *pool)
{
  return (m_str ? apr_pstrdup(pool, m_str) : NULL);
}
