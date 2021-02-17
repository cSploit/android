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
 * @file File.h
 * @brief Interface of the class File
 */

#ifndef FILE_H
#define FILE_H

#include <jni.h>
#include "svn_io.h"
#include "Pool.h"
#include "JNIStringHolder.h"

/**
 * This class contains a Java objects implementing the interface File and
 * implements the functions read & close of svn_stream_t.
 */
class File
{
 private:
  /**
   * A local reference to the Java object.
   */
  jobject m_jthis;
  JNIStringHolder *stringHolder;
 public:
  File(jobject jthis);
  ~File();
  const char *getAbsPath();
  const char *getInternalStyle(const SVN::Pool &pool);
  bool isNull() const;
};

#endif // FILE_H
