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

import org.apache.subversion.javahl.ClientException;

import java.util.Map;
import org.apache.subversion.javahl.ISVNClient;

/**
 * This interface is used to receive every single line for a file on a
 * the {@link ISVNClient#blame} call.
 */
public interface BlameCallback
{
    /**
     * the method will be called for every line in a file.
     * @param lineNum           the line number for this line
     * @param revision          the revision of the last change.
     * @param revProps          the revision properties for this revision.
     * @param mergedRevision    the revision of the last merged change.
     * @param mergedRevProps    the revision properties for the last merged
     *                          change.
     * @param mergedPath        the path of the last merged change.
     * @param line              the line in the file.
     * @param localChange       true if the line was locally modified.
     */
    public void singleLine(long lineNum, long revision,
                           Map<String, byte[]> revProps, long mergedRevision,
                           Map<String, byte[]> mergedRevProps,
                           String mergedPath, String line, boolean localChange)
        throws ClientException;
}
