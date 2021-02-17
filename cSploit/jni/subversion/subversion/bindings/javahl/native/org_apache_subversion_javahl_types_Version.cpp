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
 * @file org_apache_subversion_javahl_types_Version.cpp
 * @brief Implementation of the native methods in the Java class Version.
 */

#include "../include/org_apache_subversion_javahl_types_Version.h"
#include "JNIStackElement.h"
#include "svn_version.h"

JNIEXPORT jint JNICALL
Java_org_apache_subversion_javahl_types_Version_getMajor(JNIEnv *env,
 jobject jthis)
{
  JNIEntry(Version, getMajor);
  return SVN_VER_MAJOR;
}

JNIEXPORT jint JNICALL
Java_org_apache_subversion_javahl_types_Version_getMinor(JNIEnv *env,
 jobject jthis)
{
  JNIEntry(Version, getMinor);
  return SVN_VER_MINOR;
}

JNIEXPORT jint JNICALL
Java_org_apache_subversion_javahl_types_Version_getPatch(JNIEnv *env,
 jobject jthis)
{
  JNIEntry(Version, getPatch);
  return SVN_VER_PATCH;
}

JNIEXPORT jstring JNICALL
Java_org_apache_subversion_javahl_types_Version_getTag(JNIEnv *env,
 jobject jthis)
{
  JNIEntry(Version, getTag);
  jstring tag = JNIUtil::makeJString(SVN_VER_TAG);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  return tag;
}

JNIEXPORT jstring JNICALL
Java_org_apache_subversion_javahl_types_Version_getNumberTag(JNIEnv *env,
 jobject jthis)
{
  JNIEntry(Version, getNumberTag);
  jstring numtag = JNIUtil::makeJString(SVN_VER_NUMTAG);
  if (JNIUtil::isJavaExceptionThrown())
    return NULL;

  return numtag;
}
