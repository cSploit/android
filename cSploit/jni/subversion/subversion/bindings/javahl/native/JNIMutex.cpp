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
 * @file JNIMutex.cpp
 * @brief Implementation of the class JNIMutex
 */

#include "JNIMutex.h"
#include <apr_strings.h>
#include <apr_tables.h>
#include <apr_general.h>
#include <apr_lib.h>
#include "JNIUtil.h"

/**
 * Create an object and allocate an apr mutex
 * @param pool  the pool from which the mutex is allocated
 */
JNIMutex::JNIMutex(apr_pool_t *pool)
{
  apr_status_t apr_err =
    apr_thread_mutex_create (&m_mutex, APR_THREAD_MUTEX_NESTED, pool);
  if (apr_err)
    JNIUtil::handleAPRError(apr_err, "apr_thread_mutex_create");
}

/**
 * Destroy the apr mutex and the object.
 */
JNIMutex::~JNIMutex()
{
  apr_status_t apr_err = apr_thread_mutex_destroy (m_mutex);
  if (apr_err)
    JNIUtil::handleAPRError(apr_err, "apr_thread_mutex_destroy");
}
