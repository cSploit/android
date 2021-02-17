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
 * @file CopySources.h
 * @brief Interface of the class CopySources
 */

#ifndef COPY_SOURCES_H
#define COPY_SOURCES_H

#include <jni.h>
#include <apr_tables.h>

#include "Pool.h"
#include "Array.h"

/**
 * A container for our copy sources, which can convert them into an
 * array of svn_client_copy_item_t's.
 */
class CopySources
{
 public:
  /**
   * Create a CopySources object.
   * @param jobjectArray An array of CopySource Java objects.
   */
  CopySources(Array &copySources);

  /**
   * Destroy a CopySources object
   */
  ~CopySources();

  /**
   * Converts the array of CopySources to an apr_array_header_t of
   * svn_client_copy_source_t *'s.
   *
   * @param pool The pool from which to perform allocations.
   * @return A pointer to the new array.
   */
  apr_array_header_t *array(SVN::Pool &pool);

  /**
   * Make a (single) CopySource Java object.
   */
  static jobject makeJCopySource(const char *path, svn_revnum_t rev,
                                 SVN::Pool &pool);

 private:
  /**
   * A local reference to the Java CopySources peer.
   */
  Array &m_copySources;

  CopySources(const CopySources &from);
  CopySources & operator=(const CopySources &);
};

#endif  /* COPY_SOURCES_H */
