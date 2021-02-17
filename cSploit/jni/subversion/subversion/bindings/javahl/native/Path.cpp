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
 * @file Path.cpp
 * @brief Implementation of the class Path
 */

#include <jni.h>
#include "Path.h"
#include "svn_path.h"
#include "JNIUtil.h"
#include "Pool.h"

/**
 * Constructor
 *
 * @see Path::Path(const std::string &)
 * @param path Path string
 */
Path::Path(const char *pi_path, SVN::Pool &in_pool)
{
  init(pi_path, in_pool);
}

/**
 * Constructor that takes a string as parameter.  The string is
 * converted to subversion internal representation. The string is
 * copied.
 *
 * @param path Path string
 */
Path::Path(const std::string &pi_path, SVN::Pool &in_pool)
{
  init(pi_path.c_str(), in_pool);
}

/**
 * Copy constructor
 *
 * @param path Path to be copied
 */
Path::Path(const Path &pi_path, SVN::Pool &in_pool)
{
  init(pi_path.c_str(), in_pool);
}

/**
 * initialize the class
 *
 * @param path Path string
 */
void
Path::init(const char *pi_path, SVN::Pool &in_pool)
{
  if (*pi_path == 0)
    {
      m_error_occured = NULL;
      m_path = "";
    }
  else
    {
      m_error_occured = JNIUtil::preprocessPath(pi_path, in_pool.getPool());

      m_path = pi_path;
    }
}

/**
 * @return Path string
 */
const std::string &
Path::path() const
{
  return m_path;
}

/**
 * @return Path string as a C string
 */
const char *
Path::c_str() const
{
  return m_path.c_str();
}

/**
 * Assignment operator
 */
Path&
Path::operator=(const Path &pi_path)
{
  m_error_occured = NULL;
  m_path = pi_path.m_path;

  return *this;
}

  svn_error_t *Path::error_occured() const
{
  return m_error_occured;
}

jboolean Path::isValid(const char *p)
{
  if (p == NULL)
    return JNI_FALSE;

  SVN::Pool requestPool;
  svn_error_t *err = svn_path_check_valid(p, requestPool.getPool());
  if (err == SVN_NO_ERROR)
    {
      return JNI_TRUE;
    }
  else
    {
      svn_error_clear(err);
      return JNI_FALSE;
    }
}
