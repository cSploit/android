/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import com.sleepycat.bind.EntryBinding;
import com.sleepycat.bind.tuple.TupleBase;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.DatabaseEntry;

/**
 * A persistence key binding for a given key class.
 *
 * @author Mark Hayes
 */
public class PersistKeyBinding implements EntryBinding {

    /* See Store.refresh for an explanation of the use of volatile fields. */
    volatile Catalog catalog;
    volatile Format keyFormat;
    final boolean rawAccess;

    /**
     * Creates a key binding for a given key class.
     */
    public PersistKeyBinding(Catalog catalogParam,
                             String clsName,
                             boolean rawAccess) {
        catalog = catalogParam;
        try {
            keyFormat = PersistEntityBinding.getOrCreateFormat
                (catalog, clsName, rawAccess);
        } catch (RefreshException e) {
            /* Must assign catalog field in constructor. */
            catalog = e.refresh();
            try {
                keyFormat = PersistEntityBinding.getOrCreateFormat
                    (catalog, clsName, rawAccess);
            } catch (RefreshException e2) {
                throw DbCompat.unexpectedException(e2);
            }
        }
        if (!keyFormat.isSimple() &&
            !keyFormat.isEnum() &&
            !(keyFormat.getClassMetadata() != null &&
              keyFormat.getClassMetadata().getCompositeKeyFields() != null)) {
            throw new IllegalArgumentException
                ("Key class is not a simple type, an enum, or a composite " +
                 "key class (composite keys must include @KeyField " +
                 "annotations): " +
                 clsName);
        }
        this.rawAccess = rawAccess;
    }

    /**
     * Creates a key binding dynamically for use by PersistComparator.  Formats
     * are created from scratch rather than using a shared catalog.
     */
    PersistKeyBinding(final Catalog catalog,
                      final Class cls,
                      final String[] compositeFieldOrder) {
        this.catalog = catalog;
        keyFormat = new CompositeKeyFormat(catalog, cls, compositeFieldOrder);
        keyFormat.initializeIfNeeded(catalog, null /*model*/);
        rawAccess = false;
    }

    /**
     * Binds bytes to an object for use by PersistComparator as well as
     * entryToObject.
     */
    Object bytesToObject(byte[] bytes, int offset, int length)
        throws RefreshException {

        return readKey(keyFormat, catalog, bytes, offset, length, rawAccess);
    }

    /**
     * Binds bytes to an object for use by PersistComparator as well as
     * entryToObject.
     */
    static Object readKey(Format keyFormat,
                          Catalog catalog,
                          byte[] bytes,
                          int offset,
                          int length,
                          boolean rawAccess)
        throws RefreshException {

        EntityInput input = new RecordInput
            (catalog, rawAccess, null, 0, bytes, offset, length);
        return input.readKeyObject(keyFormat);
    }

    public Object entryToObject(DatabaseEntry entry) {
        try {
            return entryToObjectInternal(entry);
        } catch (RefreshException e) {
            e.refresh();
            try {
                return entryToObjectInternal(entry);
            } catch (RefreshException e2) {
                throw DbCompat.unexpectedException(e2);
            }
        }
    }

    private Object entryToObjectInternal(DatabaseEntry entry)
        throws RefreshException {

        return bytesToObject
            (entry.getData(), entry.getOffset(), entry.getSize());
    }

    public void objectToEntry(Object object, DatabaseEntry entry) {
        try {
            objectToEntryInternal(object, entry);
        } catch (RefreshException e) {
            e.refresh();
            try {
                objectToEntryInternal(object, entry);
            } catch (RefreshException e2) {
                throw DbCompat.unexpectedException(e2);
            }
        }
    }

    private void objectToEntryInternal(Object object, DatabaseEntry entry)
        throws RefreshException {

        RecordOutput output = new RecordOutput(catalog, rawAccess);
        output.writeKeyObject(object, keyFormat);
        TupleBase.outputToEntry(output, entry);
    }

    /**
     * See Store.refresh.
     */
    void refresh(final PersistCatalog newCatalog) {
        catalog = newCatalog;
        keyFormat = catalog.getFormat(keyFormat.getClassName());
    }
}
