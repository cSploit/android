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
 * The result returned by the callback API used to handle conflicts
 * encountered during merge/update/switch operations.  Includes a poor
 * man's enum for <code>svn_wc_conflict_choice_t</code>.
 *
 * @since 1.5
 */
public class ConflictResult
{
    /**
     * Nothing done to resolve the conflict; conflict remains.
     */
    public static final int postpone = 0;

    /**
     * Resolve the conflict by choosing the base file.
     */
    public static final int chooseBase = 1;

    /**
     * Resolve the conflict by choosing the incoming (repository)
     * version of the object.
     */
    public static final int chooseTheirsFull = 2;

    /**
     * Resolve the conflict by choosing own (local) version of the
     * object.
     */
    public static final int chooseMineFull = 3;

    /**
     * Resolve the conflict by choosing the incoming (repository)
     * version of the object (for conflicted hunks only).
     */
    public static final int chooseTheirsConflict = 4;

    /**
     * Resolve the conflict by choosing own (local) version of the
     * object (for conflicted hunks only).
     */
    public static final int chooseMineConflict = 5;

    /**
     * Resolve the conflict by choosing the merged object
     * (potentially manually edited).
     */
    public static final int chooseMerged = 6;

    /**
     * A value corresponding to the
     * <code>svn_wc_conflict_choice_t</code> enum.
     */
    private int choice;

    /**
     * The path to the result of a merge, or <code>null</code>.
     */
    private String mergedPath;

    /**
     * Create a new conflict result instace.
     */
    public ConflictResult(int choice, String mergedPath)
    {
      this.choice = choice;
      this.mergedPath = mergedPath;
    }

    public org.apache.subversion.javahl.ConflictResult toApache()
    {
        return new org.apache.subversion.javahl.ConflictResult(
            org.apache.subversion.javahl.ConflictResult.Choice.values()[choice],
            mergedPath);
    }

    /**
     * @return A value corresponding to the
     * <code>svn_wc_conflict_choice_t</code> enum.
     */
    public int getChoice()
    {
        return choice;
    }

    /**
     * @return The path to the result of a merge, or <code>null</code>.
     */
    public String getMergedPath()
    {
        return mergedPath;
    }
}
