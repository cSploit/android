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
 * @file OutputStream.cpp
 * @brief Implementation of the class OutputStream
 */

#include "OutputStream.h"
#include "JNIUtil.h"
#include "JNIByteArray.h"

/**
 * Create an OutputStream object.
 * @param jthis the Java object to be stored
 */
OutputStream::OutputStream(jobject jthis)
{
  m_jthis = jthis;
}

/**
 * Destroy an Inputer object.
 */
OutputStream::~OutputStream()
{
  // The m_jthis does not need to be destroyed, because it is the
  // passed in parameter to the Java method.
}

/**
 * Create a svn_stream_t structure for this object.  This will be used
 * as an output stream by Subversion.
 * @param pool  the pool, from which the structure is allocated
 * @return the output stream
 */
svn_stream_t *OutputStream::getStream(const SVN::Pool &pool)
{
  // Create a stream with this as the baton and set the write and
  // close functions.
  svn_stream_t *ret = svn_stream_create(this, pool.getPool());
  svn_stream_set_write(ret, OutputStream::write);
  svn_stream_set_close(ret, OutputStream::close);
  return ret;
}

/**
 * Implements svn_write_fn_t to write data out from Subversion.
 * @param baton     an OutputStream object for the callback
 * @param buffer    the buffer for the write data
 * @param len       on input the buffer len, on output the number of written
 *                  bytes
 * @return a subversion error or SVN_NO_ERROR
 */
svn_error_t *OutputStream::write(void *baton, const char *buffer,
                                 apr_size_t *len)
{
  JNIEnv *env = JNIUtil::getEnv();

  // An object of our class is passed in as the baton.
  OutputStream *that = (OutputStream*)baton;

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->FindClass("java/io/OutputStream");
      if (JNIUtil::isJavaExceptionThrown())
        return SVN_NO_ERROR;

      mid = env->GetMethodID(clazz, "write", "([B)V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        return SVN_NO_ERROR;

      env->DeleteLocalRef(clazz);
    }

  // convert the data to a Java byte array
  jbyteArray data = JNIUtil::makeJByteArray((const signed char*)buffer,
                                            *len);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  // write the data
  env->CallObjectMethod(that->m_jthis, mid, data);
  if (JNIUtil::isJavaExceptionThrown())
    return SVN_NO_ERROR;

  env->DeleteLocalRef(data);

  return SVN_NO_ERROR;
}

/**
 * Implements svn_close_fn_t to close the output stream.
 * @param baton     an OutputStream object for the callback
 * @return a subversion error or SVN_NO_ERROR
 */
svn_error_t *OutputStream::close(void *baton)
{
  JNIEnv *env = JNIUtil::getEnv();

  // An object of our class is passed in as the baton
  OutputStream *that = (OutputStream*)baton;

  // The method id will not change during the time this library is
  // loaded, so it can be cached.
  static jmethodID mid = 0;
  if (mid == 0)
    {
      jclass clazz = env->FindClass("java/io/OutputStream");
      if (JNIUtil::isJavaExceptionThrown())
        return SVN_NO_ERROR;

      mid = env->GetMethodID(clazz, "close", "()V");
      if (JNIUtil::isJavaExceptionThrown() || mid == 0)
        return SVN_NO_ERROR;

      env->DeleteLocalRef(clazz);
    }

  // Call the Java object, to close the stream.
  env->CallVoidMethod(that->m_jthis, mid);
  // No need to check for exception here because we return anyway.

  return SVN_NO_ERROR;
}
