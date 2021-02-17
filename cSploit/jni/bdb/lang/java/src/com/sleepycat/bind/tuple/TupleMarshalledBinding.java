/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.bind.tuple;

import com.sleepycat.util.RuntimeExceptionWrapper;

/**
 * A concrete <code>TupleBinding</code> that delegates to the
 * <code>MarshalledTupleEntry</code> interface of the data or key object.
 *
 * <p>This class works by calling the methods of the {@link
 * MarshalledTupleEntry} interface, which must be implemented by the key or
 * data class, to convert between the key or data entry and the object.</p>
 *
 * @author Mark Hayes
 */
public class TupleMarshalledBinding<E extends MarshalledTupleEntry>
    extends TupleBinding<E> {

    private Class<E> cls;

    /**
     * Creates a tuple marshalled binding object.
     *
     * <p>The given class is used to instantiate key or data objects using
     * {@link Class#newInstance}, and therefore must be a public class and have
     * a public no-arguments constructor.  It must also implement the {@link
     * MarshalledTupleEntry} interface.</p>
     *
     * @param cls is the class of the key or data objects.
     */
    public TupleMarshalledBinding(Class<E> cls) {

        this.cls = cls;

        /* The class will be used to instantiate the object.  */
        if (!MarshalledTupleEntry.class.isAssignableFrom(cls)) {
            throw new IllegalArgumentException
                (cls.toString() + " does not implement MarshalledTupleEntry");
        }
    }

    // javadoc is inherited
    public E entryToObject(TupleInput input) {

        try {
            E obj = cls.newInstance();
            obj.unmarshalEntry(input);
            return obj;
        } catch (IllegalAccessException e) {
            throw RuntimeExceptionWrapper.wrapIfNeeded(e);
        } catch (InstantiationException e) {
            throw RuntimeExceptionWrapper.wrapIfNeeded(e);
        }
    }

    // javadoc is inherited
    public void objectToEntry(E object, TupleOutput output) {

        object.marshalEntry(output);
    }
}
