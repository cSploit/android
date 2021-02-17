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

import java.util.Set;

import org.apache.subversion.javahl.CommitItem;

/**
 * This is callback interface which has to implemented by the client
 * to receive which files will be committed and to enter the log
 * message.
 */
public interface CommitMessageCallback
{
    /**
     * Retrieve a commit message from the user based on the items to
     * be committed
     * @param elementsToBeCommitted Array of elements to be committed
     * @return the log message of the commit.
     */
    String getLogMessage(Set<CommitItem> elementsToBeCommitted);
}
