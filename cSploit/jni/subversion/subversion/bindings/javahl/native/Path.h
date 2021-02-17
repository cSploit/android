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
 * @file Path.h
 * @brief Interface of the C++ class Path.
 */

#ifndef PATH_H
#define PATH_H

#include <string>
#include <jni.h>
#include "Pool.h"
struct svn_error_t;

/**
 * Encapsulation for Subversion Path handling
 */
class Path
{
 private:
  // The path to be stored.
  std::string m_path;

  svn_error_t *m_error_occured;

  /**
   * Initialize the class.
   *
   * @param pi_path Path string
   */
  void init(const char *pi_path, SVN::Pool &in_pool);

 public:
  /**
   * Constructor that takes a string as parameter.
   * The string is converted to subversion internal
   * representation. The string is copied.
   *
   * @param pi_path Path string
   */
  Path(const std::string &pi_path, SVN::Pool &in_pool);

  /**
   * Constructor
   *
   * @see Path::Path (const std::string &)
   * @param pi_path Path string
   */
  Path(const char *pi_path, SVN::Pool &in_pool);

  /**
   * Copy constructor
   *
   * @param pi_path Path to be copied
   */
  Path(const Path &pi_path, SVN::Pool &in_pool);

  /**
   * Assignment operator
   */
  Path &operator=(const Path&);

  /**
   * @return Path string
   */
  const std::string &path() const;

  /**
   * @return Path string as a C string
   */
  const char *c_str() const;

  svn_error_t *error_occured() const;

  /**
   * Returns whether @a path is non-NULL and passes the @c
   * svn_path_check_valid() test.
   *
   * @since 1.4.0
   */
  static jboolean isValid(const char *path);
};

#endif  // PATH_H
