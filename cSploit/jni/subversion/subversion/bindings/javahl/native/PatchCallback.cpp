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
 * @file PatchCallback.cpp
 * @brief Implementation of the class PatchCallback
 */

#include "PatchCallback.h"
#include "CreateJ.h"
#include "EnumMapper.h"
#include "JNIUtil.h"
#include "svn_time.h"
#include "svn_path.h"

/**
 * Create a PatchCallback object.  jcallback is the Java callback object.
 */
PatchCallback::PatchCallback(jobject jcallback)
{
  m_callback = jcallback;
}

PatchCallback::~PatchCallback()
{
  // The m_callback does not need to be destroyed, because it is the
  // passed in parameter to the Java SVNClient.patch method.
}

svn_error_t *
PatchCallback::callback(void *baton,
                        svn_boolean_t *filtered,
                        const char *canon_path_from_patchfile,
                        const char *patch_abspath,
                        const char *reject_abspath,
                        apr_pool_t *pool)
{
  if (baton)
    return ((PatchCallback *)baton)->singlePatch(filtered,
                                                 canon_path_from_patchfile,
                                                 patch_abspath, reject_abspath,
                                                 pool);

  return SVN_NO_ERROR;
}

svn_error_t *
PatchCallback::singlePatch(svn_boolean_t *filtered,
                           const char *canon_path_from_patchfile,
                           const char *patch_abspath,
                           const char *reject_abspath,
                           apr_pool_t *pool)
{
  JNIEnv *env = JNIUtil::getEnv();

  // Create a local frame for our references
  env->PushLocalFrame(LOCAL_FRAME_SIZE);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/PatchCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);

      mid = env->GetMethodID(clazz, "singlePatch",
                             "(Ljava/lang/String;Ljava/lang/String;"
                             "Ljava/lang/String;)Z");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  jstring jcanonPath = JNIUtil::makeJString(canon_path_from_patchfile);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  jstring jpatchAbsPath = JNIUtil::makeJString(patch_abspath);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  jstring jrejectAbsPath = JNIUtil::makeJString(reject_abspath);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  jboolean jfiltered = env->CallBooleanMethod(m_callback, mid, jcanonPath,
                                              jpatchAbsPath, jrejectAbsPath);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  *filtered = (jfiltered ? TRUE : FALSE);

  env->PopLocalFrame(NULL);
  return SVN_NO_ERROR;
}
