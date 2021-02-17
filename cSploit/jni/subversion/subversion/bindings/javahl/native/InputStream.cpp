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
 * @file InputStream.cpp
 * @brief Implementation of the class InputStream
 */

#include "InputStream.h"
#include "JNIUtil.h"
#include "JNIByteArray.h"

/**
 * Create an InputStream object.
 * @param jthis the Java object to be stored
 */
InputStream::InputStream(jobject jthis)
{
  m_jthis = jthis;
}

InputStream::~InputStream()
{
  // The m_jthis does not need to be destroyed, because it is the
  // passed in parameter to the Java method.
}

/**
 * Create a svn_stream_t structure for this object. This will be used
 * as an input stream by Subversion.
 * @param pool  the pool, from which the structure is allocated
 * @return the input stream
 */
svn_stream_t *InputStream::getStream(const SVN::Pool &pool)
{
  // Create a stream with this as the baton and set the read and
  // close functions.
  svn_stream_t *ret = svn_stream_create(this, pool.getPool());
  svn_stream_set_read(ret, InputStream::read);
  svn_stream_set_close(ret, InputStream::close);
  return ret;
}

/**
 * Implements svn_read_fn_t to read to data into Subversion.
 * @param baton     an InputStream object for the callback
 * @param buffer    the buffer for the read data
 * @param len       on input the buffer len, on output the number of read bytes
 * @return a subversion error or SVN_NO_ERROR
 */
svn_error_t *InputStream::read(void *baton, char *buffer, apr_size_t *len)
{
  JNIEnv *env = JNIUtil::getEnv();
  // An object of our class is passed in as the baton.
  InputStream *that = (InputStream*)baton;

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->FindClass("java/io/InputStream");
      if (JNIUtil::isJavaExceptionThrown())
        return SVN_NO_ERROR;

      mid = env->GetMethodID(clazz, "read", "([B)I");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        return SVN_NO_ERROR;

      env->DeleteLocalRef(clazz);
    }

  // Allocate a Java byte array to read the data.
  jbyteArray data = JNIUtil::makeJByteArray((const signed char*)buffer,
                                            *len);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  // Read the data.
  jint jread = env->CallIntMethod(that->m_jthis, mid, data);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  // Put the Java byte array into a helper object to retrieve the
  // data bytes.
  JNIByteArray outdata(data, true);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  // Catch when the Java method tells us it read too much data.
  if (jread > (jint) *len)
    jread = -1;

  // In the case of success copy the data back to the Subversion
  // buffer.
  if (jread > 0)
    memcpy(buffer, outdata.getBytes(), jread);

  // Copy the number of read bytes back to Subversion.
  *len = jread;

  return SVN_NO_ERROR;
}

/**
 * Implements svn_close_fn_t to close the input stream.
 * @param baton     an InputStream object for the callback
 * @return a subversion error or SVN_NO_ERROR
 */
svn_error_t *InputStream::close(void *baton)
{
  JNIEnv *env = JNIUtil::getEnv();

  // An object of our class is passed in as the baton
  InputStream *that = (InputStream*)baton;

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->FindClass("java/io/InputStream");
      if (JNIUtil::isJavaExceptionThrown())
        return SVN_NO_ERROR;

      mid = env->GetMethodID(clazz, "close", "()V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        return SVN_NO_ERROR;

      env->DeleteLocalRef(clazz);
    }

  // Call the Java object, to close the stream.
  env->CallVoidMethod(that->m_jthis, mid);
  // We don't need to check for an exception here because we return anyway.

  return SVN_NO_ERROR;
}
