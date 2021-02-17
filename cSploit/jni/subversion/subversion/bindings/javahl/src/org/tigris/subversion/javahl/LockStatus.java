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

/**
 * @since 1.2
 * what happened to a lock during an operation
 */
public interface LockStatus
{
    /**
     * does not make sense for this operation
     */
    public static final int inapplicable = 0;

    /**
     * unknown lock state
     */
    public static final int unknown = 1;

    /**
     * the lock change did not change
     */
    public static final int unchanged = 2;

    /**
     * the item was locked
     */
    public static final int locked = 3;

    /**
     * the item was unlocked
     */
    public static final int unlocked = 4;

    public static final String[] stateNames =
    {
        "inapplicable",
        "unknown",
        "unchanged",
        "locked",
        "unlocked",
    };
}
