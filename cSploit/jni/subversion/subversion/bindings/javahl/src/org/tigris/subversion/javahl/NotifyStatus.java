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
 * Status of the text or the property of the item triggering the
 * notification.
 */
public interface NotifyStatus
{
    /** It not applicable*/
    public static final int inapplicable = 0;

    /** Notifier doesn't know or isn't saying. */
    public static final int unknown = 1;

    /** The state did not change. */
    public static final int unchanged = 2;

    /** The item wasn't present. */
    public static final int missing = 3;

    /** An unversioned item obstructed work. */
    public static final int obstructed = 4;

    /** Pristine state was modified. */
    public static final int changed = 5;

    /** Modified state had mods merged in. */
    public static final int merged = 6;

    /** Modified state got conflicting mods. */
    public static final int conflicted = 7;

    /**
     * the textual represention for the status types
     */
    public static final String[] statusNames =
    {
        "inapplicable",
        "unknown",
        "unchanged",
        "missing",
        "obstructed",
        "changed",
        "merged",
        "conflicted",
    };
}
