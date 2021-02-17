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

package org.tigris.subversion.javahl;

import java.util.Date;

/**
 * This interface is used to receive every single line for a file on a
 * the SVNClientInterface.blame call.
 *
 * @since 1.5
 */
public interface BlameCallback2
{
    /**
     * the method will be called for every line in a file.
     * @param date              the date of the last change.
     * @param revision          the revision of the last change.
     * @param author            the author of the last change.
     * @param merged_date       the date of the last merged change.
     * @param merged_revision   the revision of the last merged change.
     * @param merged_author     the author of the last merged change.
     * @param merged_path       the path of the last merged change.
     * @param line              the line in the file
     */
    public void singleLine(Date date, long revision, String author,
                           Date merged_date, long merged_revision,
                           String merged_author, String merged_path,
                           String line);
}
