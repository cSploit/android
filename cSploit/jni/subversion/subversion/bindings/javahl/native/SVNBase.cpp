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
 * @file SVNBase.cpp
 * @brief Implementation of the class SVNBase
 */

#include "SVNBase.h"
#include "JNIUtil.h"

SVNBase::SVNBase()
    : pool(JNIUtil::getPool())
{
  jthis = NULL;
}

SVNBase::~SVNBase()
{
}

jlong SVNBase::getCppAddr() const
{
  return reinterpret_cast<jlong>(this);
}

jlong SVNBase::findCppAddrForJObject(jobject jthis, jfieldID *fid,
                                     const char *className)
{
  JNIEnv *env = JNIUtil::getEnv();

  findCppAddrFieldID(fid, className, env);
  if (*fid == 0)
    {
      return 0;
    }
  else
    {
      jlong cppAddr = env->GetLongField(jthis, *fid);
      if (JNIUtil::isJavaExceptionThrown())
        return 0;

      if (cppAddr)
        {
          /* jthis is not guaranteed to be the same between JNI invocations, so
             we do a little dance here and store the updated version in our
             object for this invocation.

             findCppAddrForJObject() is, by necessity, called before any other
             methods on the C++ object, so by doing this we can guarantee a
             valid jthis pointer for subsequent uses. */
          (reinterpret_cast<SVNBase *> (cppAddr))->jthis = jthis;
        }
      return cppAddr;
    }
}

void SVNBase::finalize()
{
  // This object should've already been disposed of!
  if (JNIUtil::getLogLevel() >= JNIUtil::errorLog)
    JNIUtil::logMessage("An SVNBase object escaped disposal");

  JNIUtil::enqueueForDeletion(this);
}

void SVNBase::dispose(jfieldID *fid, const char *className)
{
  jobject my_jthis = this->jthis;

  delete this;
  JNIEnv *env = JNIUtil::getEnv();
  SVNBase::findCppAddrFieldID(fid, className, env);
  if (*fid == 0)
    return;

  env->SetLongField(my_jthis, *fid, 0);
  if (JNIUtil::isJavaExceptionThrown())
    return;
}

inline void SVNBase::findCppAddrFieldID(jfieldID *fid, const char *className,
                                        JNIEnv *env)
{
  if (*fid == 0)
    {
      jclass clazz = env->FindClass(className);
      if (!JNIUtil::isJavaExceptionThrown())
        {
          *fid = env->GetFieldID(clazz, "cppAddr", "J");
          if (JNIUtil::isJavaExceptionThrown())
            *fid = 0;
        }
    }
}
