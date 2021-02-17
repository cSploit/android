/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import java.lang.reflect.Array;
import java.util.Map;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.persist.model.EntityModel;

/**
 * Format for a non-persistent class that is only used for declared field
 * types and arrays.  Currently used only for Object and interface types.
 *
 * @author Mark Hayes
 */
class NonPersistentFormat extends Format {

    private static final long serialVersionUID = -7488355830875148784L;

    NonPersistentFormat(Catalog catalog, Class type) {
        super(catalog, type);
    }

    @Override
    void initialize(Catalog catalog, EntityModel model, int initVersion) {
    }

    @Override
    void collectRelatedFormats(Catalog catalog,
                               Map<String, Format> newFormats) {
    }

    @Override
    Object newArray(int len) {
        return Array.newInstance(getType(), len);
    }

    @Override
    public Object newInstance(EntityInput input, boolean rawAccess) {
        throw DbCompat.unexpectedState
            ("Cannot instantiate non-persistent class: " + getClassName());
    }

    @Override
    public Object readObject(Object o, EntityInput input, boolean rawAccess) {
        throw DbCompat.unexpectedState();
    }

    @Override
    void writeObject(Object o, EntityOutput output, boolean rawAccess) {
        throw DbCompat.unexpectedState();
    }

    @Override
    void skipContents(RecordInput input) {
        throw DbCompat.unexpectedState();
    }

    @Override
    boolean evolve(Format newFormat, Evolver evolver) {
        evolver.useOldFormat(this, newFormat);
        return true;
    }
}
