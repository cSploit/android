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

package org.apache.subversion.javahl;

/**
 * The result returned by the callback API used to handle conflicts
 * encountered during merge/update/switch operations.  Includes a poor
 * man's enum for <code>svn_wc_conflict_choice_t</code>.
 */
public class ConflictResult
{
    /**
     * A value corresponding to the
     * <code>svn_wc_conflict_choice_t</code> enum.
     */
    private Choice choice;

    /**
     * The path to the result of a merge, or <code>null</code>.
     */
    private String mergedPath;

    /**
     * Create a new conflict result instace.
     */
    public ConflictResult(Choice choice, String mergedPath)
    {
      this.choice = choice;
      this.mergedPath = mergedPath;
    }

    /**
     * @return A value corresponding to the
     * <code>svn_wc_conflict_choice_t</code> enum.
     */
    public Choice getChoice()
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

    public enum Choice
    {
        /** Nothing done to resolve the conflict; conflict remains.  */
        postpone,

        /** Resolve the conflict by choosing the base file.  */
        chooseBase,

        /**
         * Resolve the conflict by choosing the incoming (repository)
         * version of the object.
         */
        chooseTheirsFull,

        /**
         * Resolve the conflict by choosing own (local) version of the
         * object.
         */
        chooseMineFull,

        /**
         * Resolve the conflict by choosing the incoming (repository)
         * version of the object (for conflicted hunks only).
         */
        chooseTheirsConflict,

        /**
         * Resolve the conflict by choosing own (local) version of the
         * object (for conflicted hunks only).
         */
        chooseMineConflict,

        /**
         * Resolve the conflict by choosing the merged object
         * (potentially manually edited).
         */
        chooseMerged;
    }
}
