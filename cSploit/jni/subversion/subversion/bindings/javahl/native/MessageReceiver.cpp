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
 * @file MessageReceiver.cpp
 * @brief Implementation of the class MessageReceiver
 */

#include "MessageReceiver.h"
#include "JNIUtil.h"

/**
 * Create a new object and store the local reference to the Java
 * object.
 */
MessageReceiver::MessageReceiver(jobject jthis)
{
  m_jthis = jthis;
}

/**
 * Destroy the object.
 */
MessageReceiver::~MessageReceiver()
{
  // The m_callback does not need to be destroyed, because it is the
  // passed in parameter to the Java method.
}

/**
 * send a message to the Java object.
 * @param message   the message to be send
 */
void MessageReceiver::receiveMessage(const char *message)
{
  JNIEnv *env = JNIUtil::getEnv();

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/ISVNAdmin$MessageReceiver");
      if (JNIUtil::isJavaExceptionThrown())
        return;

      mid = env->GetMethodID(clazz, "receiveMessageLine",
                             "(Ljava/lang/String;)V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        return;

      env->DeleteLocalRef(clazz);
    }

  // Convert the message to a Java string.
  jstring jmsg = JNIUtil::makeJString(message);
  if (JNIUtil::isJavaExceptionThrown())
    return;

  // Call the Java method.
  env->CallVoidMethod(m_jthis, mid);
  if (JNIUtil::isJavaExceptionThrown())
    return;

  // Delete the Java string.
  env->DeleteLocalRef(jmsg);
}
