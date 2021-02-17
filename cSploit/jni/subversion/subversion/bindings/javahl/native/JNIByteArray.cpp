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
 * @file JNIByteArray.cpp
 * @brief Implementation of the class JNIByteArray
 */
#include "JNIByteArray.h"
#include "JNIUtil.h"

/**
 * Create a new object
 * @param jba the local reference to the Java byte array
 * @param flag that the underlying byte array reference should be deleted at
 *        destruction
 */
JNIByteArray::JNIByteArray(jbyteArray jba, bool deleteByteArray)
{
  m_array = jba;
  m_deleteByteArray = deleteByteArray;
  if (jba != NULL)
    {
      // Get the bytes.
      JNIEnv *env = JNIUtil::getEnv();
      m_data = env->GetByteArrayElements(jba, NULL);
    }
  else
    {
      m_data = NULL;
    }
}

JNIByteArray::~JNIByteArray()
{
  if (m_array != NULL)
    {
      // Release the bytes
      JNIUtil::getEnv()->ReleaseByteArrayElements(m_array,
                                                  m_data,
                                                  JNI_ABORT);
      if (m_deleteByteArray)
        // And if needed the byte array.
        JNIUtil::getEnv()->DeleteLocalRef(m_array);
    }
}

/**
 * Returns the number of bytes in the byte array.
 * @return the number of bytes
 */
int JNIByteArray::getLength()
{
  if (m_data == NULL)
    return 0;
  else
    return JNIUtil::getEnv()->GetArrayLength(m_array);
}

/**
 * Returns the bytes of the byte array.
 * @return the bytes
 */
const signed char *JNIByteArray::getBytes() const
{
  return m_data;
}

/**
 * Returns if the byte array was not set.
 * @return if the byte array was not set
 */
bool JNIByteArray::isNull() const
{
  return m_data == NULL;
}
