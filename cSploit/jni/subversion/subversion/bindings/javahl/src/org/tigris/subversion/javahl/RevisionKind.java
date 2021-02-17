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
 * Various ways of specifying revisions.
 *
 * Note:
 * In contexts where local mods are relevant, the `working' kind
 * refers to the uncommitted "working" revision, which may be modified
 * with respect to its base revision.  In other contexts, `working'
 * should behave the same as `committed' or `current'.
 *
 */
public interface RevisionKind
{
    /** No revision information given. */
    public static final int unspecified = 0;

    /** revision given as number */
    public static final int number = 1;

    /** revision given as date */
    public static final int date = 2;

    /** rev of most recent change */
    public static final int committed = 3;

    /** (rev of most recent change) - 1 */
    public static final int previous = 4;

    /** .svn/entries current revision */
    public static final int base = 5;

    /** current, plus local mods */
    public static final int working = 6;

    /** repository youngest */
    public static final int head = 7;
}
