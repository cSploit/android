/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.bind.serial;

import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectStreamClass;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.util.ClassResolver;
import com.sleepycat.util.RuntimeExceptionWrapper;

/**
 * A specialized <code>ObjectInputStream</code> that gets class description
 * information from a <code>ClassCatalog</code>.  It is used by
 * <code>SerialBinding</code>.
 *
 * <p>This class is used instead of an {@link ObjectInputStream}, which it
 * extends, to read an object stream written by the {@link SerialOutput} class.
 * For reading objects from a database normally one of the serial binding
 * classes is used.  {@link SerialInput} is used when an {@link
 * ObjectInputStream} is needed along with compact storage.  A {@link
 * ClassCatalog} must be supplied, however, to stored shared class
 * descriptions.</p>
 *
 * @see <a href="SerialBinding.html#evolution">Class Evolution</a>
 *
 * @author Mark Hayes
 */
public class SerialInput extends ClassResolver.Stream {

    private ClassCatalog classCatalog;

    /**
     * Creates a serial input stream.
     *
     * @param in is the input stream from which compact serialized objects will
     * be read.
     *
     * @param classCatalog is the catalog containing the class descriptions
     * for the serialized objects.
     */
    public SerialInput(InputStream in, ClassCatalog classCatalog)
        throws IOException {

        this(in, classCatalog, null);
    }

    /**
     * Creates a serial input stream.
     *
     * @param in is the input stream from which compact serialized objects will
     * be read.
     *
     * @param classCatalog is the catalog containing the class descriptions
     * for the serialized objects.
     *
     * @param classLoader is the class loader to use, or null if a default
     * class loader should be used.
     */
    public SerialInput(InputStream in,
                       ClassCatalog classCatalog,
                       ClassLoader classLoader)
        throws IOException {

        super(in, classLoader);
        this.classCatalog = classCatalog;
    }

    @Override
    protected ObjectStreamClass readClassDescriptor()
        throws IOException, ClassNotFoundException {

        try {
            byte len = readByte();
            byte[] id = new byte[len];
            readFully(id);

            return classCatalog.getClassFormat(id);
        } catch (DatabaseException e) {

            /*
             * Do not throw IOException from here since ObjectOutputStream
             * will write the exception to the stream, which causes another
             * call here, etc.
             */
            throw RuntimeExceptionWrapper.wrapIfNeeded(e);
        }
    }
}
