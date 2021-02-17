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
 * class for kind status of the item or its properties
 */
public interface StatusKind
{
    /** does not exist */
    public static final int none = 0;

    /** exists, but uninteresting */
    public static final int normal = 1;

    /** text or props have been modified */
    public static final int modified = 2;

    /** is scheduled for additon */
    public static final int added = 3;

    /** scheduled for deletion */
    public static final int deleted = 4;

    /** is not a versioned thing in this wc */
    public static final int unversioned = 5;

    /** under v.c., but is missing */
    public static final int missing = 6;

    /** was deleted and then re-added */
    public static final int replaced = 7;

    /** local mods received repos mods */
    public static final int merged = 8;

    /** local mods received conflicting repos mods */
    public static final int conflicted = 9;

    /** an unversioned resource is in the way of the versioned resource */
    public static final int obstructed = 10;

    /** a resource marked as ignored */
    public static final int ignored = 11;

    /** a directory doesn't contain a complete entries list */
    public static final int incomplete = 12;

    /** an unversioned path populated by an svn:externals property */
    public static final int external = 13;
}
