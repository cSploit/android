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
 * @file PatchCallback.h
 * @brief Interface of the class PatchCallback
 */

#ifndef PATCHCALLBACK_H
#define PATCHCALLBACK_H

#include <jni.h>
#include "svn_client.h"

struct info_entry;

/**
 * This class holds a Java callback object, which will receive every line of
 * the file for which the callback information is requested.
 */
class PatchCallback
{
 public:
  PatchCallback(jobject jcallback);
  ~PatchCallback();

  static svn_error_t *callback(void *baton,
                               svn_boolean_t *filtered,
                               const char *canon_path_from_patchfile,
                               const char *patch_abspath,
                               const char *reject_abspath,
                               apr_pool_t *pool);

 protected:
  svn_error_t *singlePatch(svn_boolean_t *filtered,
                           const char *canon_path_from_patchfile,
                           const char *patch_abspath,
                           const char *reject_abspath,
                           apr_pool_t *pool);

 private:
  /**
   * A local reference to the corresponding Java object.
   */
  jobject m_callback;
};

#endif  // PATCHCALLBACK_H
