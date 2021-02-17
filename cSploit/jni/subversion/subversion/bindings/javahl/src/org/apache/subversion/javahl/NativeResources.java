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

import org.apache.subversion.javahl.types.Version;

/**
 * Handles activities related to management of native resouces
 * (e.g. loading of native libraries).
 *
 * Public for backward compat.  This class may disappear in future versions
 * of the API.  You've been warned.
 */
public class NativeResources
{
    /**
     * @return Version information about the underlying native libraries.
     */
    private static Version version;

    /**
     * Returns version information about the underlying native libraries.
     *
     * @return version
     *
     */
    public static Version getVersion() {
        return version;
    }

    /**
     * Load the required native library whose path is specified by the
     * system property <code>subversion.native.library</code> (which
     * can be passed to the JVM on start-up using an argument like
     * <code>-Dsubversion.native.library=/usr/local/lib/libsvnjavahl-1.so</code>).
     * If the system property is not specified or cannot be loaded,
     * attempt to load the library using its expected name, and the
     * platform-dependent loading mechanism.
     *
     * @throws UnsatisfiedLinkError If the native library cannot be
     * loaded.
     * @throws LinkageError If the version of the loaded native
     * library is not compatible with this version of JavaHL's Java
     * APIs.
     */
    public static synchronized void loadNativeLibrary()
    {
        UnsatisfiedLinkError loadException = null;

        // If the user specified the fully qualified path to the
        // native library, try loading that first.
        try
        {
            String specifiedLibraryName =
                System.getProperty("subversion.native.library");
            if (specifiedLibraryName != null)
            {
                System.load(specifiedLibraryName);
                init();
                return;
            }
        }
        catch (UnsatisfiedLinkError ex)
        {
            // Since the user explicitly specified this path, this is
            // the best error to return if no other method succeeds.
            loadException = ex;
        }

        // Try to load the library by its name.  Failing that, try to
        // load it by its old name.
        String[] libraryNames = {"svnjavahl-1", "libsvnjavahl-1", "svnjavahl"};
        for (String libraryName : libraryNames)
        {
            try
            {
                System.loadLibrary(libraryName);
                init();
                return;
            }
            catch (UnsatisfiedLinkError ex)
            {
                if (loadException == null)
                {
                    loadException = ex;
                }
            }
        }

        // Re-throw the most relevant exception.
        if (loadException == null)
        {
            // This could only happen as the result of a programming error.
            loadException = new UnsatisfiedLinkError("Unable to load JavaHL " +
                                                     "native library");
        }
        throw loadException;
    }

    /**
     * Initializer for native resources to be invoked <em>after</em>
     * the native library has been loaded.  Sets library version
     * information, and initializes the re-entrance hack for native
     * code.
     * @throws LinkageError If the version of the loaded native
     * library is not compatible with this version of JavaHL's Java
     * APIs.
     */
    private static final void init()
    {
        initNativeLibrary();
        version = new Version();
        if (!version.isAtLeast(1, 7, 0))
        {
            throw new LinkageError("Native library version must be at least " +
                                   "1.7.0, but is only " + version);
        }
    }

    /**
     * Initialize the native library layer.
     */
    private static native void initNativeLibrary();
}
