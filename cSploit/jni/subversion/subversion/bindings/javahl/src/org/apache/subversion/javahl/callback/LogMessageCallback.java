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

import org.apache.subversion.javahl.ISVNClient;
import org.apache.subversion.javahl.types.ChangePath;

import java.util.Map;
import java.util.Set;

/**
 * This interface is used to receive every log message for the log
 * messages found by a {@link ISVNClient#logMessages} call.
 *
 * All log messages are returned in a list, which is terminated by an
 * invocation of this callback with the revision set to SVN_INVALID_REVNUM.
 *
 * If the includeMergedRevisions parameter to {@link ISVNClient#logMessages}
 * is true, then messages returned through this callback may have the
 * hasChildren parameter set.  This parameter indicates that a separate list,
 * which includes messages for merged revisions, will immediately follow.
 * This list is also terminated with SVN_INVALID_REVNUM, after which the
 * previous log message list continues.
 *
 * Log message lists may be nested arbitrarily deep, depending on the ancestry
 * of the requested paths.
 */
public interface LogMessageCallback
{
    /**
     * The method will be called for every log message.
     *
     * @param changedPaths   a set of the paths that were changed
     * @param revision       the revision of the commit
     * @param revprops       All of the requested revision properties,
     *                       possibly including svn:date, svn:author,
     *                       and svn:log.
     * @param hasChildren    when merge sensitive option was requested,
     *                       whether or not this entry has child entries.
     */
    public void singleMessage(Set<ChangePath> changedPaths,
                              long revision,
                              Map<String, byte[]> revprops,
                              boolean hasChildren);
}
