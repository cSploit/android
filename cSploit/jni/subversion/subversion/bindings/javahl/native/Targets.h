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
 * @file Targets.h
 * @brief Interface of the class Targets
 */

#ifndef TARGETS_H
#define TARGETS_H

#include <jni.h>
#include "Pool.h"
#include "StringArray.h"

struct apr_array_header_t;

#include "Path.h"
#include "Pool.h"
#include <vector>

class Targets
{
 private:
  SVN::Pool m_subpool;
  std::vector<const char*> m_targets;
  StringArray *m_strArray;
  svn_error_t *m_error_occured;
 public:
  Targets(StringArray &strArray, SVN::Pool &in_pool);
  Targets(const char *path, SVN::Pool &in_pool);
  void add(const char *path);
  ~Targets();
  const apr_array_header_t *array(const SVN::Pool &pool);
  svn_error_t *error_occured();
};

#endif // TARGETS_H
