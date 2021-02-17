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
 * @file RevisionRange.h
 * @brief Interface of the class RevisionRange
 */

#ifndef REVISION_RANGE_H
#define REVISION_RANGE_H

#include <jni.h>
#include "svn_types.h"

#include "Pool.h"

/**
 * A container for our copy sources, which can convert them into an
 * array of svn_client_copy_item_t's.
 */
class RevisionRange
{
 public:

  /**
   * Create a RevisionRange object from a Java object.
   */
  RevisionRange(jobject jthis);

  /**
   * Destroy a RevisionRange object
   */
  ~RevisionRange();

  /**
   * Return an svn_opt_revision_range_t.
   */
  const svn_opt_revision_range_t *toRange(SVN::Pool &pool) const;

  /**
   * Make a (single) RevisionRange Java object.
   */
  static jobject makeJRevisionRange(svn_merge_range_t *range);

 private:
  /**
   * A local reference to the Java RevisionRange peer.
   */
  jobject m_range;
};

#endif  // REVISION_RANGE_H
