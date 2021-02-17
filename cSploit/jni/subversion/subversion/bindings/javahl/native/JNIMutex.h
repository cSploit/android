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
 * @file JNIMutex.h
 * @brief Interface of the class JNIMutex
 */

#ifndef JNIMUTEX_H
#define JNIMUTEX_H

class JNICriticalSection;
struct apr_pool_t;
struct apr_thread_mutex_t;

/**
 * This class holds a apr mutex for the use of JNICriticalSection.
 */
class JNIMutex
{
 public:
  JNIMutex(apr_pool_t *pool);
  ~JNIMutex();
  friend class JNICriticalSection;
 private:
  /**
   * The apr mutex.
   */
  apr_thread_mutex_t *m_mutex;
};

#endif  // JNIMUTEX_H
