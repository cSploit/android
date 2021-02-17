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
 * @file ReposNotifyCallback.h
 * @brief Interface of the class ReposNotifyCallback
 */

#ifndef REPOSNOTIFYCALLBACK_H
#define REPOSNOTIFYCALLBACK_H

#include <jni.h>
#include "svn_repos.h"

/**
 * This class passes notification from subversion to a Java object
 * (1.2 version).
 */
class ReposNotifyCallback
{
 private:
  /**
   * The local reference to the Java object.
   */
  jobject m_notify;

 public:
  ReposNotifyCallback(jobject p_notify);
  ~ReposNotifyCallback();

  /**
   * Implementation of the svn_repos_notify_func_t API.
   *
   * @param baton notification instance is passed using this parameter
   * @param notify all the information about the event
   * @param pool An APR pool from which to allocate memory.
   */
  static void notify(void *baton,
                     const svn_repos_notify_t *notify,
                     apr_pool_t *pool);

  /**
   * Handler for Subversion notifications.
   *
   * @param notify all the information about the event
   * @param pool An APR pool from which to allocate memory.
   */
  void onNotify(const svn_repos_notify_t *notify,
                apr_pool_t *pool);
};

#endif  // REPOSNOTIFYCALLBACK_H
