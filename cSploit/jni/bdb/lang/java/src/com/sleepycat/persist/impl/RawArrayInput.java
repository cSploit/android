/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import com.sleepycat.persist.raw.RawObject;
import java.util.IdentityHashMap;

/**
 * Extends RawAbstractInput to convert array (ObjectArrayFormat and
 * PrimitiveArrayteKeyFormat) RawObject instances.
 *
 * @author Mark Hayes
 */
class RawArrayInput extends RawAbstractInput {

    private Object[] array;
    private int index;
    private Format componentFormat;

    RawArrayInput(Catalog catalog,
                  boolean rawAccess,
                  IdentityHashMap converted,
                  RawObject raw,
                  Format componentFormat) {
        super(catalog, rawAccess, converted);
        array = raw.getElements();
        this.componentFormat = componentFormat;
    }

    @Override
    public int readArrayLength() {
        return array.length;
    }

    @Override
    Object readNext()
        throws RefreshException {

        Object o = array[index++];
        return checkAndConvert(o, componentFormat);
    }
}
