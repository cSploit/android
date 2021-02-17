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
 * @file JNICriticalSection.cpp
 * @brief Implementation of the class JNICriticalSection
 */

#include "JNICriticalSection.h"
#include <apr_strings.h>
#include <apr_tables.h>
#include <apr_general.h>
#include <apr_lib.h>
#include "JNIUtil.h"
#include "JNIMutex.h"

/**
 * Create the critical section and lock the mutex
 * @param mutext    the underlying mutex
 */
JNICriticalSection::JNICriticalSection(JNIMutex &mutex)
{
  m_mutex = &mutex;
  apr_status_t apr_err = apr_thread_mutex_lock (mutex.m_mutex);
  if (apr_err)
    {
      JNIUtil::handleAPRError(apr_err, "apr_thread_mutex_lock");
      return;
    }

}

/**
 * Release the mutex and the destroy the critical section.
 */
JNICriticalSection::~JNICriticalSection()
{
  apr_status_t apr_err = apr_thread_mutex_unlock (m_mutex->m_mutex);
  if (apr_err)
    {
      JNIUtil::handleAPRError(apr_err, "apr_thread_mutex_unlock");
      return;
    }
}
