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
 * @file Targets.cpp
 * @brief Implementation of the class Targets
 */

#include "Targets.h"
#include "JNIUtil.h"
#include "JNIStringHolder.h"
#include <apr_tables.h>
#include <apr_strings.h>
#include "svn_path.h"
#include <iostream>

Targets::~Targets()
{
}

Targets::Targets(const char *path, SVN::Pool &in_pool)
    : m_subpool(in_pool)
{
  m_strArray = NULL;
  m_targets.push_back (apr_pstrdup(m_subpool.getPool(), path));
  m_error_occured = NULL;
}

void Targets::add(const char *path)
{
  m_targets.push_back (path);
}

const apr_array_header_t *Targets::array(const SVN::Pool &pool)
{
  if (m_strArray != NULL)
    {
      const std::vector<std::string> &vec = m_strArray->vector();

      std::vector<std::string>::const_iterator it;
      for (it = vec.begin(); it < vec.end(); ++it)
        {
          const char *tt = it->c_str();
          svn_error_t *err = JNIUtil::preprocessPath(tt, pool.getPool());
          if (err != NULL)
            {
              m_error_occured = err;
              break;
            }
          m_targets.push_back(tt);
        }
    }

  std::vector<const char*>::const_iterator it;

  apr_pool_t *apr_pool = pool.getPool();
  apr_array_header_t *apr_targets = apr_array_make (apr_pool,
                                                    m_targets.size(),
                                                    sizeof(const char *));

  for (it = m_targets.begin(); it != m_targets.end(); ++it)
    {
      const char *target = *it;

      svn_error_t *err = JNIUtil::preprocessPath(target, pool.getPool());
      if (err != NULL)
        {
            m_error_occured = err;
            break;
        }
      APR_ARRAY_PUSH(apr_targets, const char *) = target;
    }

  return apr_targets;
}

Targets::Targets(StringArray &strArray, SVN::Pool &in_pool)
    : m_subpool(in_pool)
{
  m_strArray = &strArray;
  m_error_occured = NULL;
}

svn_error_t *Targets::error_occured()
{
  return m_error_occured;
}
