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

import java.util.EventListener;

/**
 * The callback API used to handle conflicts encountered during
 * merge/update/switch operations.
 *
 * @since 1.5
 */
public interface ConflictResolverCallback extends EventListener
{
    /**
     * The callback method invoked for each conflict during a
     * merge/update/switch operation.  NOTE: The files that are
     * potentially passed in the ConflictDescriptor are in
     * repository-normal format (LF line endings and contracted
     * keywords).
     *
     * @param descrip A description of the conflict.
     * @return The result of any conflict resolution, from the {@link
     * ConflictResult} enum.
     * @throws SubversionException If an error occurs.
     */
    public ConflictResult resolve(ConflictDescriptor descrip)
        throws SubversionException;
}
