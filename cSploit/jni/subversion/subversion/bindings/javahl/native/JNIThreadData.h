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
 * @file JNIThreadData.h
 * @brief Interface of the class JNIData
 */

#ifndef JNITHREADDATA_H
#define JNITHREADDATA_H

#include <jni.h>
#include "JNIUtil.h"

struct apr_threadkey_t;

/**
 * This class implements thread local storage for JNIUtil.
 */
class JNIThreadData
{
 public:
  static void del(void *);
  static JNIThreadData *getThreadData();
  static bool initThreadData();
  static void pushNewThreadData();
  static void popThreadData();
  JNIThreadData();
  ~JNIThreadData();

  /**
   * The current JNI environment.
   */
  JNIEnv *m_env;

  /**
   * Flag that a Java execption has been detected.
   */
  bool m_exceptionThrown;

  /**
   * A buffer used for formating messages.
   */
  char m_formatBuffer[JNIUtil::formatBufferSize];

 private:
  /**
   * Pointer to previous thread information to enable reentrent
   * calls.
   */
  JNIThreadData *m_previous;

  /**
   * The key to address this thread local storage.
   */
  static apr_threadkey_t *g_key;
};

#endif  // JNITHREADDATA_H
