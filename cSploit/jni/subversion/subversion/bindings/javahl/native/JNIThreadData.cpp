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
 * @file JNIThreadData.cpp
 * @brief Implementation of the class JNIThreadData
 */

#include "JNIThreadData.h"
#include <apr_strings.h>
#include <apr_tables.h>
#include <apr_general.h>
#include <apr_lib.h>
#include <apr_thread_proc.h>
#include "JNIUtil.h"

apr_threadkey_t *JNIThreadData::g_key;

/**
 * Create and initialize a new object.
 */
JNIThreadData::JNIThreadData()
{
  m_env = NULL;
  m_exceptionThrown = false;
  m_previous = NULL;
}

JNIThreadData::~JNIThreadData()
{
}

/**
 * Initialize the thread local storage.
 * @return success or failure
 */
bool JNIThreadData::initThreadData()
{
  // If already initialized -> nothing to do.
  if (g_key != NULL)
    return false;

  // Request a key for the thread local storage from the global pool
  // and register a callback function called when the thread is
  // deleted.
  apr_status_t apr_err = apr_threadkey_private_create(&g_key,
                                                      del,
                                                      JNIUtil::getPool());
  if (apr_err)
    {
      JNIUtil::handleAPRError(apr_err, "apr_threadkey_private_create");
      return false;
    }

  return true;
}

/**
 * Get the thread local storage for this thread.
 * @return thread local storage
 */
JNIThreadData *JNIThreadData::getThreadData()
{
  // We should never be called before initThreadData
  if (g_key == NULL)
    return NULL;

  // Retrieve the thread local storage from APR.
  JNIThreadData *data = NULL;
  apr_status_t apr_err = apr_threadkey_private_get
    (reinterpret_cast<void**>(&data), g_key);
  if (apr_err)
    {
      JNIUtil::handleAPRError(apr_err, "apr_threadkey_private_get");
      return NULL;
    }

  // Not already allocated.
  if (data == NULL)
    {
      // Allocate and store to APR.
      data = new JNIThreadData;
      apr_err = apr_threadkey_private_set (data, g_key);
      if (apr_err)
        {
          JNIUtil::handleAPRError(apr_err, "apr_threadkey_private_set");
          return NULL;
        }
    }
  return data;
}

/**
 * Allocate a new ThreadData for the current call from Java and push
 * it on the stack
 */
void JNIThreadData::pushNewThreadData()
{
  JNIThreadData *data = NULL;
  apr_status_t apr_err = apr_threadkey_private_get
    (reinterpret_cast<void**>(&data), g_key);
  if (apr_err)
    {
      JNIUtil::handleAPRError(apr_err, "apr_threadkey_private_get");
      return;
    }
  JNIThreadData *newData = new JNIThreadData();
  newData->m_previous =data;
  apr_err = apr_threadkey_private_set (newData, g_key);
  if (apr_err)
    {
      JNIUtil::handleAPRError(apr_err, "apr_threadkey_private_set");
      return;
    }
}

/**
 * Pop the current ThreadData from the stack, because the call
 * completed.
 */
void JNIThreadData::popThreadData()
{
  JNIThreadData *data = NULL;
  apr_status_t apr_err = apr_threadkey_private_get
    (reinterpret_cast<void**>(&data), g_key);
  if (apr_err)
    {
      JNIUtil::handleAPRError(apr_err, "apr_threadkey_private_get");
      return;
    }
  if (data == NULL)
    return;

  JNIThreadData *oldData = data->m_previous;
  delete data;
  apr_err = apr_threadkey_private_set (oldData, g_key);
  if (apr_err)
    {
      JNIUtil::handleAPRError(apr_err, "apr_threadkey_private_set");
      return;
    }
}

/**
 * Callback called by APR when the thread dies.  Deletes the thread
 * local storage.
 */
void JNIThreadData::del(void *p)
{
  delete reinterpret_cast<JNIThreadData*>(p);
}
