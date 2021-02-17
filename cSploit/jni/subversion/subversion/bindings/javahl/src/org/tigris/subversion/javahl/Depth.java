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
 * The concept of depth for directories.
 *
 * Note:
 * This is similar to, but not exactly the same as, the WebDAV and LDAP
 * concepts of depth.
 *
 */
public final class Depth
{
    /* The order of these depths is important: the higher the number,
       the deeper it descends.  This allows us to compare two depths
       numerically to decide which should govern. */

    /** Depth undetermined or ignored. */
    public static final int unknown = -2;

    /** Exclude (i.e, don't descend into) directory D. */
    public static final int exclude = -1;

    /** Just the named directory D, no entries.  Updates will not pull in
        any files or subdirectories not already present. */
    public static final int empty = 0;

    /** D + its file children, but not subdirs.  Updates will pull in any
        files not already present, but not subdirectories. */
    public static final int files = 1;

    /** D + immediate children (D and its entries).  Updates will pull in
        any files or subdirectories not already present; those
        subdirectories' this_dir entries will have depth-empty. */
    public static final int immediates = 2;

    /** D + all descendants (full recursion from D).  Updates will pull
        in any files or subdirectories not already present; those
        subdirectories' this_dir entries will have depth-infinity.
        Equivalent to the pre-1.5 default update behavior. */
    public static final int infinity = 3;

    /**
     * @return A depth value of {@link #infinity} when
     * <code>recurse</code> is <code>true</code>, or {@link #empty}
     * otherwise.
     */
    public static final int infinityOrEmpty(boolean recurse)
    {
        return (recurse ? infinity : empty);
    }

    /**
     * @return A depth value of {@link #infinity} when
     * <code>recurse</code> is <code>true</code>, or {@link #files}
     * otherwise.
     */
    public static final int infinityOrFiles(boolean recurse)
    {
        return (recurse ? infinity : files);
    }

    /**
     * @return A depth value of {@link #infinity} when
     * <code>recurse</code> is <code>true</code>, or {@link
     * #immediates} otherwise.
     */
    public static final int infinityOrImmediates(boolean recurse)
    {
        return (recurse ? infinity : immediates);
    }

    /**
     * @return A depth value of {@link #unknown} when
     * <code>recurse</code> is <code>true</code>, or {@link #empty}
     * otherwise.
     */
    public static final int unknownOrEmpty(boolean recurse)
    {
        return (recurse ? unknown : empty);
    }

    /**
     * @return A depth value of {@link #unknown} when
     * <code>recurse</code> is <code>true</code>, or {@link #files}
     * otherwise.
     */
    public static final int unknownOrFiles(boolean recurse)
    {
        return (recurse ? unknown : files);
    }

    /**
     * @return A depth value of {@link #unknown} when
     * <code>recurse</code> is <code>true</code>, or {@link
     * #immediates} otherwise.
     */
    public static final int unknownOrImmediates(boolean recurse)
    {
        return (recurse ? unknown : immediates);
    }

    public static org.apache.subversion.javahl.types.Depth toADepth(int depth)
    {
       switch(depth)
       {
            case infinity:
                return org.apache.subversion.javahl.types.Depth.infinity;
            case immediates:
                return org.apache.subversion.javahl.types.Depth.immediates;
            case files:
                return org.apache.subversion.javahl.types.Depth.files;
            case empty:
                return org.apache.subversion.javahl.types.Depth.empty;
            case exclude:
                return org.apache.subversion.javahl.types.Depth.exclude;
            case unknown:
            default:
                return org.apache.subversion.javahl.types.Depth.unknown;
       }
    }

    public static int fromADepth(org.apache.subversion.javahl.types.Depth aDepth)
    {
        if (aDepth == null)
          return unknown;

        switch(aDepth)
        {
            case infinity:
                return infinity;
            case immediates:
                return immediates;
            case files:
                return files;
            case empty:
                return empty;
            case exclude:
                return exclude;
            case unknown:
            default:
                return unknown;
        }
    }
}
