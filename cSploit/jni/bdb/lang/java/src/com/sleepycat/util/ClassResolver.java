/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.util;

import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectStreamClass;

/**
 * Implements policies for loading user-supplied classes.  The {@link
 * #resolveClass} method should be used to load all user-supplied classes, and
 * the {@link Stream} class should be used as a replacement for
 * ObjectInputStream to deserialize instances of user-supplied classes.
 * <p>
 * The ClassLoader specified as a param should be the one configured using
 * EnvironmentConfig.setClassLoader.  This loader is used, if non-null.  If the
 * loader param is null, but a non-null thread-context loader is available, the
 * latter is used.  If the loader param and thread-context loader are both
 * null, or if they fail to load a class by throwing ClassNotFoundException,
 * then the default Java mechanisms for determining the class loader are used.
 */
public class ClassResolver {
    
    /**
     * A specialized ObjectInputStream that supports use of a user-specified
     * ClassLoader.
     *
     * If the loader param and thread-context loader are both null, of if they
     * throw ClassNotFoundException, then ObjectInputStream.resolveClass is
     * called, which has its own special rules for class loading.
     */
    public static class Stream extends ObjectInputStream {

        private final ClassLoader classLoader;

        public Stream(InputStream in, ClassLoader classLoader)
            throws IOException {

            super(in);
            this.classLoader = classLoader;
        }

        @Override
        protected Class resolveClass(ObjectStreamClass desc)
            throws IOException, ClassNotFoundException {

            ClassNotFoundException firstException = null;
            if (classLoader != null) {
                try {
                    return Class.forName(desc.getName(), false /*initialize*/,
                                         classLoader);
                } catch (ClassNotFoundException e) {
                    if (firstException == null) {
                        firstException = e;
                    }
                }
            }
            final ClassLoader threadLoader = 
                Thread.currentThread().getContextClassLoader();
            if (threadLoader != null) {
                try {
                    return Class.forName(desc.getName(), false /*initialize*/,
                                         threadLoader);
                } catch (ClassNotFoundException e) {
                    if (firstException == null) {
                        firstException = e;
                    }
                }
            }
            try {
                return super.resolveClass(desc);
            } catch (ClassNotFoundException e) {
                if (firstException == null) {
                    firstException = e;
                }
            }
            throw firstException;
        }
    }

    /**
     * A specialized Class.forName method that supports use of a user-specified
     * ClassLoader.
     *
     * If the loader param and thread-context loader are both null, of if they
     * throw ClassNotFoundException, then Class.forName is called and the
     * "current loader" (the one used to load JE) will be used.
     */
    public static Class resolveClass(String className,
                                     ClassLoader classLoader)
        throws ClassNotFoundException {

        ClassNotFoundException firstException = null;
        if (classLoader != null) {
            try {
                return Class.forName(className, true /*initialize*/,
                                     classLoader);
            } catch (ClassNotFoundException e) {
                if (firstException == null) {
                    firstException = e;
                }
            }
        }
        final ClassLoader threadLoader = 
            Thread.currentThread().getContextClassLoader();
        if (threadLoader != null) {
            try {
                return Class.forName(className, true /*initialize*/,
                                     threadLoader);
            } catch (ClassNotFoundException e) {
                if (firstException == null) {
                    firstException = e;
                }
            }
        }
        try {
            return Class.forName(className);
        } catch (ClassNotFoundException e) {
            if (firstException == null) {
                firstException = e;
            }
        }
        throw firstException;
    }
}
