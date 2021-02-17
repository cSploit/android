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
 * Subversion path validation and manipulation.
 *
 * @since 1.4.0
 */
public class Path
{
    /**
     * Load the required native library.
     */
    static
    {
        org.apache.subversion.javahl.NativeResources.loadNativeLibrary();
    }

    /**
     * A valid path is a UTF-8 string without any control characters.
     *
     * @return Whether Subversion can store the path in a repository.
     */
    public static boolean isValid(String path)
    {
        try {
            byte[] bytes = path.getBytes("UTF-8");

            for (byte b : bytes)
            {
                if (b < 0x20)
                    return false;
            }

            return true;
        }
        catch (Exception ex)
        {
            return false;
        }
    }

    /**
     * Whether a URL is valid. Implementation may behave differently
     * than <code>svn_path_is_url()</code>.
     *
     * @param path The Subversion "path" to inspect.
     * @return Whether <code>path</code> is a URL.
     * @throws IllegalArgumentException If <code>path</code> is
     * <code>null</code>.
     */
    public static boolean isURL(String path)
    {
        if (path == null)
        {
            throw new IllegalArgumentException();
        }
        // Require at least "s://".
        return (path.indexOf("://") > 0);
    }
}
