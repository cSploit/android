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
 * @file Array.h
 * @brief Interface of the class Array
 */

#ifndef ARRAY_H
#define ARRAY_H

#include <jni.h>
#include "Pool.h"

struct apr_array_header_t;

#include <vector>

class Array
{
 private:
  jobjectArray m_objectArray;
  std::vector<jobject> m_objects;
  void init(jobjectArray jobjects);
 public:
  Array(jobjectArray jobjects);
  Array(jobject jobjectsCollection);
  virtual ~Array();
  const std::vector<jobject> &vector(void) const;
};

#endif // ARRAY_H
