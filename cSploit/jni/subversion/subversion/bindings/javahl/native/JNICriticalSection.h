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
 * @file JNICriticalSection.h
 * @brief Interface of the class JNICriticalSection
 */

#ifndef JNICRITICALSECTION_H
#define JNICRITICALSECTION_H

class JNIMutex;

/**
 * This class holds a mutex which will be locked during the constructor and
 * released during the destructor. If the object is created on the stack, this
 * garanties that the mutex will be released all the time if the block is left.
 * Only one thread can enter all the critrical sections secured by the same
 * mutex.
 */
class JNICriticalSection
{
 public:
  JNICriticalSection(JNIMutex &mutex);
  ~JNICriticalSection();
 private:
  /**
   * The mutex to be locked and released.
   */
  JNIMutex *m_mutex;
};

#endif  // JNICRITICALSECTION_H
