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
 * @file DiffSummaryReceiver.h
 * @brief Interface of the class DiffSummaryReceiver
 */

#ifndef DIFFSUMMARYRECEIVER_H
#define DIFFSUMMARYRECEIVER_H

#include <jni.h>
#include "svn_client.h"

/**
 * A diff summary receiver callback.
 */
class DiffSummaryReceiver
{
 public:
  /**
   * Create a DiffSummaryReceiver object.
   * @param jreceiver The Java callback object.
   */
  DiffSummaryReceiver(jobject jreceiver);

  /**
   * Destroy a DiffSummaryReceiver object
   */
  ~DiffSummaryReceiver();

  /**
   * Implementation of the svn_client_diff_summarize_func_t API.
   *
   * @param diff The diff summary.
   * @param baton A reference to the DiffSummaryReceiver instance.
   * @param pool An APR pool from which to allocate memory.
   */
  static svn_error_t *summarize(const svn_client_diff_summarize_t *diff,
                                void *baton,
                                apr_pool_t *pool);

 protected:
  /**
   * Callback invoked for every diff summary.
   *
   * @param diff The diff summary.
   * @param pool An APR pool from which to allocate memory.
   */
  svn_error_t *onSummary(const svn_client_diff_summarize_t *diff,
                         apr_pool_t *pool);

 private:
  /**
   * A local reference to the Java DiffSummaryReceiver peer.
   */
  jobject m_receiver;
};

#endif  // DIFFSUMMARYRECEIVER_H
