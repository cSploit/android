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
 * @file BlameCallback.cpp
 * @brief Implementation of the class BlameCallback
 */

#include "BlameCallback.h"
#include "CreateJ.h"
#include "JNIUtil.h"
#include "svn_time.h"
/**
 * Create a BlameCallback object
 * @param jcallback the Java callback object.
 */
BlameCallback::BlameCallback(jobject jcallback)
{
  m_callback = jcallback;
}
/**
 * Destroy a BlameCallback object
 */
BlameCallback::~BlameCallback()
{
  // the m_callback does not need to be destroyed, because it is the passed
  // in parameter to the Java SVNClient.blame method.
}

/* implements svn_client_blame_receiver3_t */
svn_error_t *
BlameCallback::callback(void *baton,
                        svn_revnum_t start_revnum,
                        svn_revnum_t end_revnum,
                        apr_int64_t line_no,
                        svn_revnum_t revision,
                        apr_hash_t *rev_props,
                        svn_revnum_t merged_revision,
                        apr_hash_t *merged_rev_props,
                        const char *merged_path,
                        const char *line,
                        svn_boolean_t local_change,
                        apr_pool_t *pool)
{
  if (baton)
    return ((BlameCallback *)baton)->singleLine(start_revnum, end_revnum,
                                                line_no, revision, rev_props,
                                                merged_revision,
                                                merged_rev_props, merged_path,
                                                line, local_change, pool);

  return SVN_NO_ERROR;
}

/**
 * Callback called for a single line in the file, for which the blame
 * information was requested.  See the Java-doc for more information.
 */
svn_error_t *
BlameCallback::singleLine(svn_revnum_t start_revnum, svn_revnum_t end_revnum,
                          apr_int64_t line_no, svn_revnum_t revision,
                          apr_hash_t *revProps, svn_revnum_t mergedRevision,
                          apr_hash_t *mergedRevProps, const char *mergedPath,
                          const char *line, svn_boolean_t localChange,
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
      jclass clazz = env->FindClass(JAVA_PACKAGE"/callback/BlameCallback");
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);

      mid = env->GetMethodID(clazz, "singleLine",
                             "(JJLjava/util/Map;JLjava/util/Map;"
                             "Ljava/lang/String;Ljava/lang/String;Z)V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  // convert the parameters to their Java relatives
  jobject jrevProps = CreateJ::PropertyMap(revProps);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  jobject jmergedRevProps = NULL;
  if (mergedRevProps != NULL)
    {
      jmergedRevProps = CreateJ::PropertyMap(mergedRevProps);
      if (JNIUtil::isJavaExceptionThrown())
        POP_AND_RETURN(SVN_NO_ERROR);
    }

  jstring jmergedPath = JNIUtil::makeJString(mergedPath);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  jstring jline = JNIUtil::makeJString(line);
  if (JNIUtil::isJavaExceptionThrown())
    POP_AND_RETURN(SVN_NO_ERROR);

  // call the Java method
  env->CallVoidMethod(m_callback, mid, (jlong)line_no, (jlong)revision,
                      jrevProps, (jlong)mergedRevision, jmergedRevProps,
                      jmergedPath, jline, (jboolean)localChange);
  // No need to check for an exception here, because we return anyway.

  env->PopLocalFrame(NULL);
  return SVN_NO_ERROR;
}
