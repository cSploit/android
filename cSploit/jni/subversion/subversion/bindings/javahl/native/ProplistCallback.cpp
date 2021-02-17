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
 * @file ProplistCallback.cpp
 * @brief Implementation of the class ProplistCallback
 */

#include "ProplistCallback.h"
#include "JNIUtil.h"
#include "CreateJ.h"
#include "svn_time.h"

/**
 * Create a ProplistCallback object
 * @param jcallback the Java callback object.
 */
ProplistCallback::ProplistCallback(jobject jcallback)
{
  m_callback = jcallback;
}

/**
 * Destroy a ProplistCallback object
 */
ProplistCallback::~ProplistCallback()
{
  // the m_callback does not need to be destroyed, because it is the passed
  // in parameter to the Java SVNClient.properties method.
}

svn_error_t *
ProplistCallback::callback(void *baton,
                           const char *path,
                           apr_hash_t *prop_hash,
                           apr_pool_t *pool)
{
  if (baton)
    return ((ProplistCallback *)baton)->singlePath(path, prop_hash, pool);

  return SVN_NO_ERROR;
}

/**
 * Callback called for a single path
 * @param path      the path name
 * @param prop_hash the hash of properties on this path
 * @param pool      memory pool for the use of this function
 */
svn_error_t *ProplistCallback::singlePath(const char *path,
                                          apr_hash_t *prop_hash,
                                          apr_pool_t *pool)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/ProplistCallback");
      if (JNIUtil::isJavaExceptionThrown())
        return SVN_NO_ERROR;

      mid = env->GetMethodID(clazz, "singlePath",
                             "(Ljava/lang/String;Ljava/util/Map;)V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  // convert the parameters to their Java relatives
  jstring jpath = JNIUtil::makeJString(path);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  jobject jmap = CreateJ::PropertyMap(prop_hash);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  // call the Java method
  env->CallVoidMethod(m_callback, mid, jpath, jmap);
  // We return whether an exception was thrown or not.

  env->PopLocalFrame(NULL);

  return SVN_NO_ERROR;
}
