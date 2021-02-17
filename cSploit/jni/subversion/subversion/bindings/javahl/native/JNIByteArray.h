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
 * @file JNIByteArray.h
 * @brief Interface of the class JNIByteArray
 */

#ifndef JNIBYTEARRAY_H
#define JNIBYTEARRAY_H

#include <jni.h>

/**
 * This class holds a Java byte array to give easy access to its
 * bytes.
 */
class JNIByteArray
{
 private:
  /**
   *  A local reference to the byte array.
   */
  jbyteArray m_array;

  /**
   * The cache bytes of the byte array.
   */
  jbyte *m_data;

  /**
   * Flag that the underlying byte array reference should be deleted
   * at destruction.
   */
  bool m_deleteByteArray;
 public:
  bool isNull() const;
  const signed char *getBytes() const;
  int getLength();
  JNIByteArray(jbyteArray jba, bool deleteByteArray = false);
  ~JNIByteArray();
};

#endif // JNIBYTEARRAY_H
