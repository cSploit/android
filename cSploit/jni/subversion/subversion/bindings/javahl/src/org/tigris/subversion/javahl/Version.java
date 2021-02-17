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
 * Encapsulates version information about the underlying native
 * libraries.  Basically a wrapper for <a
 * href="http://svn.apache.org/repos/asf/subversion/trunk/subversion/include/svn_version.h"><code>svn_version.h</code></a>.
 */
public class Version
{
    private org.apache.subversion.javahl.types.Version aVersion;

    public Version()
    {
        aVersion = new org.apache.subversion.javahl.types.Version();
    }

    public Version(org.apache.subversion.javahl.types.Version aVersion)
    {
        this.aVersion = aVersion;
    }

    /**
     * @return The full version string for the loaded JavaHL library,
     * as defined by <code>MAJOR.MINOR.PATCH INFO</code>.
     * @since 1.4.0
     */
    public String toString()
    {
        return aVersion.toString();
    }

    /**
     * @return The major version number for the loaded JavaHL library.
     * @since 1.4.0
     */
    public int getMajor()
    {
        return aVersion.getMajor();
    }

    /**
     * @return The minor version number for the loaded JavaHL library.
     * @since 1.4.0
     */
    public int getMinor()
    {
        return aVersion.getMinor();
    }

    /**
     * @return The patch-level version number for the loaded JavaHL
     * library.
     * @since 1.4.0
     */
    public int getPatch()
    {
        return aVersion.getPatch();
    }

    /**
     * @return Whether the JavaHL native library version is at least
     * of <code>major.minor.patch</code> level.
     * @since 1.5.0
     */
    public boolean isAtLeast(int major, int minor, int patch)
    {
        return aVersion.isAtLeast(major, minor, patch);
    }
}
