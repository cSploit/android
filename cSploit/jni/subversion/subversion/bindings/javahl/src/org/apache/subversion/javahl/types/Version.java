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

package org.apache.subversion.javahl.types;

/**
 * Encapsulates version information about the underlying native
 * libraries.  Basically a wrapper for <a
 * href="http://svn.apache.org/repos/asf/subversion/trunk/subversion/include/svn_version.h"><code>svn_version.h</code></a>.
 */
public class Version
{
    /**
     * @return The full version string for the loaded JavaHL library,
     * as defined by <code>MAJOR.MINOR.PATCH INFO</code>.
     */
    public String toString()
    {
        StringBuffer version = new StringBuffer();
        version.append(getMajor())
            .append('.').append(getMinor())
            .append('.').append(getPatch())
            .append(getNumberTag())
            .append(getTag());
        return version.toString();
    }

    /**
     * @return The major version number for the loaded JavaHL library.
     */
    public native int getMajor();

    /**
     * @return The minor version number for the loaded JavaHL library.
     */
    public native int getMinor();

    /**
     * @return The patch-level version number for the loaded JavaHL
     * library.
     */
    public native int getPatch();

    /**
     * @return Whether the JavaHL native library version is at least
     * of <code>major.minor.patch</code> level.
     */
    public boolean isAtLeast(int major, int minor, int patch)
    {
        int actualMajor = getMajor();
        int actualMinor = getMinor();
        return ((major < actualMajor)
                || (major == actualMajor && minor < actualMinor)
                || (major == actualMajor && minor == actualMinor &&
                    patch <= getPatch()));
    }

    /**
     * @return Some text further describing the library version
     * (e.g. <code>" (r1234)"</code>, <code>" (Alpha 1)"</code>,
     * <code>" (dev build)"</code>, etc.).
     */
    private native String getTag();

    /**
     * @return Some text further describing the library version
     * (e.g. "r1234", "Alpha 1", "dev build", etc.).
     */
    private native String getNumberTag();
}
