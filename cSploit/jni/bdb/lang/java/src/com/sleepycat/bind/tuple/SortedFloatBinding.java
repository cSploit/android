/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.bind.tuple;

import com.sleepycat.db.DatabaseEntry;

/**
 * A concrete <code>TupleBinding</code> for a sorted <code>Float</code>
 * primitive wrapper or sorted a <code>float</code> primitive.
 *
 * <p>There are two ways to use this class:</p>
 * <ol>
 * <li>When using the {@link com.sleepycat.db} package directly, the static
 * methods in this class can be used to convert between primitive values and
 * {@link DatabaseEntry} objects.</li>
 * <li>When using the {@link com.sleepycat.collections} package, an instance of
 * this class can be used with any stored collection.</li>
 * </ol>
 *
 * @see <a href="package-summary.html#floatFormats">Floating Point Formats</a>
 */
public class SortedFloatBinding extends TupleBinding<Float> {

    /* javadoc is inherited */
    public Float entryToObject(TupleInput input) {

        return input.readSortedFloat();
    }

    /* javadoc is inherited */
    public void objectToEntry(Float object, TupleOutput output) {

        output.writeSortedFloat(object);
    }

    /* javadoc is inherited */
    protected TupleOutput getTupleOutput(Float object) {

        return FloatBinding.sizedOutput();
    }

    /**
     * Converts an entry buffer into a simple <code>float</code> value.
     *
     * @param entry is the source entry buffer.
     *
     * @return the resulting value.
     */
    public static float entryToFloat(DatabaseEntry entry) {

        return entryToInput(entry).readSortedFloat();
    }

    /**
     * Converts a simple <code>float</code> value into an entry buffer.
     *
     * @param val is the source value.
     *
     * @param entry is the destination entry buffer.
     */
    public static void floatToEntry(float val, DatabaseEntry entry) {

        outputToEntry(FloatBinding.sizedOutput().writeSortedFloat(val), entry);
    }
}
