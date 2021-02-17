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
 * @file OutputStream.h
 * @brief Interface of the class OutputStream
 */

#ifndef OUTPUT_STREAM_H
#define OUTPUT_STREAM_H

#include <jni.h>
#include "svn_io.h"
#include "Pool.h"

/**
 * This class contains a Java objects implementing the interface OutputStream
 * and implements the functions write & close of svn_stream_t
 */
class OutputStream
{
  /**
   * A local reference to the Java object.
   */
  jobject m_jthis;
  static svn_error_t *write(void *baton,
                            const char *buffer, apr_size_t *len);
  static svn_error_t *close(void *baton);
 public:
  OutputStream(jobject jthis);
  ~OutputStream();
  svn_stream_t *getStream(const SVN::Pool &pool);
};

#endif // OUTPUT_STREAM_H
