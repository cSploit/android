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
 * @file JNIStackElement.h
 * @brief Interface of the class JNIStackElement
 */

#ifndef JNISTACKELEMENT_H
#define JNISTACKELEMENT_H

#include <jni.h>
#include "JNIUtil.h"

/**
 * Create a stack element on the stack, which will be used to track
 * the entry and exit of a method.  Assumes that there are a local
 * variables named "env" and "jthis" available.
 */
#define JNIEntry(c,m) JNIStackElement se(env, #c, #m, jthis);

/**
 * Create a stack element on the stack, which will be used to track
 * the entry and exit of a static method.  Assumes that there are a
 * local variables named "env" and "jthis" available.
 */
#define JNIEntryStatic(c,m) JNIStackElement se(env, #c, #m, jclazz);


/**
 * This class is used to mark the entry and exit of a method, and can
 * generate a log messages at those points.  The members are used to
 * generate the exit message
 */
class JNIStackElement
{
 public:
  JNIStackElement(JNIEnv *env, const char *clazz,
                  const char *method, jobject jthis);
  virtual ~JNIStackElement();

 private:
  /**
   * The name of the method.
   */
  const char *m_method;

  /**
   * The name of the class.
   */

  const char *m_clazz;

  /**
   * A buffer for the result for jthis.toString to identify the
   * object.
   */
  char m_objectID[JNIUtil::formatBufferSize];
};

#endif // JNISTACKELEMENT_H
