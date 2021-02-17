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
 * @file JNIStackElement.cpp
 * @brief Implementation of the class JNIStackElement
 */

#include "JNIStackElement.h"
#include "JNIUtil.h"
#include "JNIStringHolder.h"
#include "JNIThreadData.h"
#include <apr_strings.h>

/**
 * Create a new object and generate a log message,if requested
 * @param env       the JNI-environment
 * @param clazz     the class name of the method
 * @param method    the name of the method
 * @param jthis     the Java object for which the method is call
 */
JNIStackElement::JNIStackElement(JNIEnv *env, const char *clazz,
                                 const char *method, jobject jthis)
{
  JNIUtil::JNIInit(env);
  // Generating a log message is expensive.
  if (JNIUtil::getLogLevel() >= JNIUtil::entryLog)
    {
      jclass jlo = env->FindClass("java/lang/Object");
      if (JNIUtil::isJavaExceptionThrown())
        return;

      // The method id will not change during the time this library
      // is loaded, so it can be cached.
      static jmethodID mid = 0;
      if (mid == 0)
        {
          mid = env->GetMethodID(jlo, "toString", "()Ljava/lang/String;");
          if (JNIUtil::isJavaExceptionThrown())
            return;
        }

      // This will call java.lang.Object.toString, even when it is
      // overriden.
      jobject oStr = env->CallNonvirtualObjectMethod(jthis, jlo, mid);
      if (JNIUtil::isJavaExceptionThrown())
        return;

      // Copy the result to a buffer.
      JNIStringHolder name(reinterpret_cast<jstring>(oStr));
      *m_objectID = 0;
      strncat(m_objectID, name, JNIUtil::formatBufferSize -1);

      // Release the Java string.
      env->DeleteLocalRef(jlo);
      env->DeleteLocalRef(oStr);

      // Remember the parameter for the exit of the method.
      m_clazz = clazz;
      m_method = method;

      // Generate the log message.
      char *buffer = JNIUtil::getFormatBuffer();
      apr_snprintf(buffer, JNIUtil::formatBufferSize,
                   "entry class %s method %s object %s", m_clazz, m_method,
                   m_objectID);
      JNIUtil::logMessage(buffer);
    }
  else
    {
      // Initialize the object cleanly.
      m_clazz = NULL;
      m_method = NULL;
      *m_objectID = 0;
    }
}

/**
 * Destroy an object and create a log message for the exit of the
 * method, if one was create for the entry.
 */
JNIStackElement::~JNIStackElement()
{
  if (m_clazz != NULL)
    {
      // Generate a log message.
      char *buffer = JNIUtil::getFormatBuffer();
      apr_snprintf(buffer, JNIUtil::formatBufferSize,
                   "exit class %s method %s object %s", m_clazz,
                   m_method, m_objectID);
      JNIUtil::logMessage(buffer);
    }
  JNIThreadData::popThreadData();
}
