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
 * @file RevpropTable.cpp
 * @brief Implementation of the class RevpropTable
 */

#include "RevpropTable.h"
#include "JNIUtil.h"
#include "JNIStringHolder.h"
#include "Array.h"
#include <apr_tables.h>
#include <apr_strings.h>
#include <apr_hash.h>
#include "svn_path.h"
#include "svn_props.h"
#include <iostream>

RevpropTable::~RevpropTable()
{
  if (m_revpropTable != NULL)
    JNIUtil::getEnv()->DeleteLocalRef(m_revpropTable);
}

const apr_hash_t *RevpropTable::hash(const SVN::Pool &pool)
{
  if (m_revprops.size() == 0)
    return NULL;

  apr_hash_t *revprop_table = apr_hash_make(pool.getPool());

  std::map<std::string, std::string>::const_iterator it;
  for (it = m_revprops.begin(); it != m_revprops.end(); ++it)
    {
      const char *propname = apr_pstrdup(pool.getPool(), it->first.c_str());
      if (!svn_prop_name_is_valid(propname))
        {
          const char *msg = apr_psprintf(pool.getPool(),
                                         "Invalid property name: '%s'",
                                         propname);
          JNIUtil::throwNativeException(JAVA_PACKAGE "/ClientException", msg,
                                        NULL, SVN_ERR_CLIENT_PROPERTY_NAME);
          return NULL;
        }

      svn_string_t *propval = svn_string_create(it->second.c_str(),
                                                pool.getPool());

      apr_hash_set(revprop_table, propname, APR_HASH_KEY_STRING, propval);
    }

  return revprop_table;
}

RevpropTable::RevpropTable(jobject jrevpropTable)
{
  m_revpropTable = jrevpropTable;

  if (jrevpropTable != NULL)
    {
      static jmethodID keySet = 0, toArray = 0, get = 0;
      JNIEnv *env = JNIUtil::getEnv();

      jclass mapClazz = env->FindClass("java/util/Map");

      if (keySet == 0)
        {
          keySet = env->GetMethodID(mapClazz, "keySet",
                                    "()Ljava/util/Set;");
          if (JNIUtil::isExceptionThrown())
            return;
        }

      jobject jkeySet = env->CallObjectMethod(jrevpropTable, keySet);
      if (JNIUtil::isExceptionThrown())
        return;

      if (get == 0)
        {
          get = env->GetMethodID(mapClazz, "get",
                                 "(Ljava/lang/Object;)Ljava/lang/Object;");
          if (JNIUtil::isExceptionThrown())
            return;
        }

      Array keyArray(jkeySet);
      std::vector<jobject> keys = keyArray.vector();

      for (std::vector<jobject>::const_iterator it = keys.begin();
            it < keys.end(); ++it)
        {
          JNIStringHolder propname((jstring)*it);
          if (JNIUtil::isExceptionThrown())
            return;

          jobject jpropval = env->CallObjectMethod(jrevpropTable, get, *it);
          if (JNIUtil::isExceptionThrown())
            return;

          JNIStringHolder propval((jstring)jpropval);
          if (JNIUtil::isExceptionThrown())
            return;

          m_revprops[std::string((const char *)propname)]
            = std::string((const char *)propval);

          JNIUtil::getEnv()->DeleteLocalRef(jpropval);
        }

      JNIUtil::getEnv()->DeleteLocalRef(jkeySet);
    }
}
