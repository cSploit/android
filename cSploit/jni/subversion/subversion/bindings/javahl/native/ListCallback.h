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
 * @file ListCallback.h
 * @brief Interface of the class ListCallback
 */

#ifndef LISTCALLBACK_H
#define LISTCALLBACK_H

#include <jni.h>
#include "svn_client.h"

/**
 * This class holds a Java callback object, which will receive every
 * directory entry for which the callback information is requested.
 */
class ListCallback
{
public:
  ListCallback(jobject jcallback);
  ~ListCallback();

  static svn_error_t *callback(void *baton,
                               const char *path,
                               const svn_dirent_t *dirent,
                               const svn_lock_t *lock,
                               const char *abs_path,
                               apr_pool_t *pool);

protected:
  svn_error_t *doList(const char *path,
                      const svn_dirent_t *dirent,
                      const svn_lock_t *lock,
                      const char *abs_path,
                      apr_pool_t *pool);

private:
  /**
   * This a local reference to the Java object.
   */
  jobject m_callback;

  jobject createJavaDirEntry(const char *path,
                             const char *absPath,
                             const svn_dirent_t *dirent);
};

#endif // LISTCALLBACK_H
