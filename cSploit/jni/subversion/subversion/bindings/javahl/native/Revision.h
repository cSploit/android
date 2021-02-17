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
 * @file Revision.h
 * @brief Interface of the class Revision
 */

#ifndef REVISION_H
#define REVISION_H

#include <jni.h>
#include "svn_opt.h"

class Revision
{
 private:
  svn_opt_revision_t m_revision;

 public:
  static const svn_opt_revision_kind START;
  static const svn_opt_revision_kind HEAD;

  Revision(jobject jthis, bool headIfUnspecified = false,
           bool oneIfUnspecified = false);
  Revision(const svn_opt_revision_kind kind = svn_opt_revision_unspecified);
  ~Revision();

  const svn_opt_revision_t *revision() const;

  /**
   * Make a Revision Java object.
   */
  static jobject makeJRevision(svn_revnum_t rev);
};

#endif // REVISION_H
