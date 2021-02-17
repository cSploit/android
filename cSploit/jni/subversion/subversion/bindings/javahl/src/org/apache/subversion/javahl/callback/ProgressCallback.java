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
 */

package org.apache.subversion.javahl.callback;

import org.apache.subversion.javahl.ProgressEvent;

import java.util.EventListener;

/**
 * Subversion progress event interface.
 *
 * Implement this interface to provide a custom progress event handler
 * to the SVNClient class.  If you need to pass extra information to
 * the notification handler, add it to your implementing class.
 */
public interface ProgressCallback extends EventListener
{
    /**
     * Implement this API to receive progress events for Subversion
     * operations.
     *
     * @param event everything to know about this event
     */
    public void onProgress(ProgressEvent event);
}
