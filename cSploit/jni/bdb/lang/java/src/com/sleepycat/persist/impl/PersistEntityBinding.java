/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import com.sleepycat.bind.EntityBinding;
import com.sleepycat.bind.tuple.TupleBase;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.persist.raw.RawObject;

/**
 * A persistence entity binding for a given entity class.
 *
 * @author Mark Hayes
 */
public class PersistEntityBinding implements EntityBinding {

    /* See Store.refresh for an explanation of the use of volatile fields. */
    volatile PersistCatalog catalog;
    volatile Format entityFormat;
    final boolean rawAccess;
    PersistKeyAssigner keyAssigner;

    /**
     * Creates a key binding for a given entity class.
     */
    public PersistEntityBinding(final PersistCatalog catalogParam,
                                final String entityClassName,
                                final boolean rawAccess) {
        catalog = catalogParam;

        try {
            entityFormat = getOrCreateFormat(catalog, entityClassName,
                                             rawAccess);
        } catch (RefreshException e) {
            /* Must assign catalog field in constructor. */
            catalog = e.refresh();
            try {
                entityFormat = getOrCreateFormat(catalog, entityClassName,
                                                 rawAccess);
            } catch (RefreshException e2) {
                throw DbCompat.unexpectedException(e2);
            }
        }
        if (!entityFormat.isEntity()) {
            throw new IllegalArgumentException
                ("Not an entity class: " + entityClassName);
        }
        this.rawAccess = rawAccess;
    }

    public PersistKeyAssigner getKeyAssigner() {
        return keyAssigner;
    }

    public Object entryToObject(final DatabaseEntry key,
                                final DatabaseEntry data) {
        try {
            return entryToObjectInternal(key, null, data);
        } catch (RefreshException e) {
            e.refresh();
            try {
                return entryToObjectInternal(key, null, data);
            } catch (RefreshException e2) {
                throw DbCompat.unexpectedException(e2);
            }
        }
    }
    
    /**
     * This method will be used in PrimaryIndex.get, where the primary key is 
     * known to DPL. This method will force to call readEntityWithPriKey to 
     * directly assign primary key to the de-serialized object.
     */     
    public Object entryToObjectWithPriKey(final Object priKey,
                                          final DatabaseEntry data) {
        try {
            if (priKey == null) {
                throw new 
                    IllegalArgumentException("Primary key cannot be null.");
            }
            return entryToObjectInternal(null, priKey, data);
        } catch (RefreshException e) {
            e.refresh();
            try {
                return entryToObjectInternal(null, priKey, data);
            } catch (RefreshException e2) {
                throw DbCompat.unexpectedException(e2);
            }
        }
    }

    /**
     * This method is always called after reading an entity.  If a refresh is
     * needed, we detect that in PersistCatalog.getFormat(int).
     */
    private Object entryToObjectInternal(final DatabaseEntry key,
                                         final Object priKey,
                                         final DatabaseEntry data)
        throws RefreshException {
        
        return readEntity(catalog, key, priKey, data, rawAccess);
    }

    /**
     * Creates the instance, reads the entity key first to track visited
     * entities correctly, then reads the data and returns the entity.
     *
     * This is a special case of EntityInput.readObject for a top level entity.
     * Special treatments are:
     * - The formatId must be >= 0; since this is the top level instance, it
     *   cannot refer to a visited object nor be a null reference.
     * - The entity is not checked for existence in the visited object set;
     *   entities cannot be referenced by another entity.
     * - Reader.readPriKey must be called prior to calling Reader.readObject.
     */
    static Object readEntity(Catalog useCatalog,
                             DatabaseEntry key,
                             Object priKey,
                             DatabaseEntry data,
                             boolean rawAccess)
        throws RefreshException {

        RecordInput dataInput = new RecordInput
            (useCatalog, rawAccess, null, 0,
             data.getData(), data.getOffset(), data.getSize());
        int initialOffset = dataInput.getBufferOffset();
        int formatId = dataInput.readPackedInt();
        Format format = useCatalog.getFormat(formatId, true /*expectStored*/);
        dataInput.registerEntityFormat(format);
        Reader reader = format.getReader();
        Object entity = reader.newInstance(dataInput, rawAccess);
        if (priKey == null) {
            /* If priKey is null, need to deserialize the primary key. */
            RecordInput keyInput = new RecordInput
                (useCatalog, rawAccess, null, 0,
                 key.getData(), key.getOffset(), key.getSize());
            reader.readPriKey(entity, keyInput, rawAccess);
        } else {
        
            /* 
             * If priKey is not null, directly assign it to the primary key 
             * field. [#19248]
             */
            Accessor accessor = 
                reader.getAccessor(entity instanceof RawObject ?
                                   true :
                                   rawAccess);
            if (accessor == null) {
                accessor = format.getLatestVersion().getReader().
                               getAccessor(entity instanceof RawObject ?
                                           true :
                                           rawAccess);
            }
            accessor.setPriField(entity, priKey);
        }
        dataInput.registerEntity(entity, initialOffset);
        entity = reader.readObject(entity, dataInput, rawAccess);
        return entity;
    }

    public void objectToData(final Object entity, final DatabaseEntry data) {
        try {
            objectToDataInternal(entity, data);
        } catch (RefreshException e) {
            e.refresh();
            try {
                objectToDataInternal(entity, data);
            } catch (RefreshException e2) {
                throw DbCompat.unexpectedException(e2);
            }
        }
    }

    /**
     * This method is always called before writing an entity.  If a refresh is
     * needed, we detect that here.
     */
    private void objectToDataInternal(final Object entity,
                                      final DatabaseEntry data)
        throws RefreshException {

        Format format = getValidFormat(entity);
        /* Before a write, check whether a refresh is needed. [#16655] */
        catalog.checkWriteInReplicaUpgradeMode();
        writeEntity(format, catalog, entity, data, rawAccess);
    }

    /**
     * Writes the formatId and object, and returns the bytes.
     *
     * This is a special case of EntityOutput.writeObject for a top level
     * entity.  Special treatments are:
     * - The entity may not be null.
     * - The entity is not checked for existence in the visited object set;
     *   entities cannot be referenced by another entity.
     */
    static void writeEntity(Format format,
                            Catalog catalog,
                            Object entity,
                            DatabaseEntry data,
                            boolean rawAccess)
        throws RefreshException {

        RecordOutput output = new RecordOutput(catalog, rawAccess);
        output.registerEntity(entity);
        output.writePackedInt(format.getId());
        format.writeObject(entity, output, rawAccess);
        TupleBase.outputToEntry(output, data);
    }
    
    public void objectToKey(final Object entity, final DatabaseEntry key) {
        try {
            objectToKeyInternal(entity, key);
        } catch (RefreshException e) {
            e.refresh();
            try {
                objectToKeyInternal(entity, key);
            } catch (RefreshException e2) {
                throw DbCompat.unexpectedException(e2);
            }
        }
    }

    private void objectToKeyInternal(final Object entity,
                                     final DatabaseEntry key)
        throws RefreshException {

        /*
         * Write the primary key field as a special case since the output
         * format is for a key binding, not entity data.
         */
        Format format = getValidFormat(entity);
        RecordOutput output = new RecordOutput(catalog, rawAccess);

        /* Write the primary key and return the bytes. */
        format.writePriKey(entity, output, rawAccess);
        TupleBase.outputToEntry(output, key);
    }

    /**
     * Returns the format for the given entity and validates it, throwing an
     * exception if it is invalid for this binding.
     */
    private Format getValidFormat(Object entity)
        throws RefreshException {

        /* A null entity is not allowed. */
        if (entity == null) {
            throw new IllegalArgumentException("An entity may not be null");
        }

        /*
         * Get the format.  getFormat throws IllegalArgumentException if the
         * class is not persistent.
         */
        Format format;
        if (rawAccess) {
            if (!(entity instanceof RawObject)) {
                throw new IllegalArgumentException
                    ("Entity must be a RawObject");
            }
            format = (Format) ((RawObject) entity).getType();
        } else {
            format = catalog.getFormat
                (entity.getClass(), true /*checkEntitySubclassIndexes*/);
        }

        /* Check that the entity class/subclass is valid for this binding. */
        if (format.getEntityFormat() != entityFormat) {
            throw new IllegalArgumentException
                ("The entity class (" + format.getClassName() +
                 ") must be this entity class or a subclass of it: " +
                 entityFormat.getClassName());
        }

        return format;
    }
    
    /**
     * Utility method for getting or creating a format as appropriate for
     * bindings and key creators.
     */
    static Format getOrCreateFormat(Catalog useCatalog,
                                    String clsName,
                                    boolean rawAccess)
        throws RefreshException {

        if (rawAccess) {
            Format format = useCatalog.getFormat(clsName);
            if (format == null) {
                throw new IllegalArgumentException
                    ("Not a persistent class: " + clsName);
            }
            return format;
        } else {
            Class cls = useCatalog.resolveKeyClass(clsName);
            return useCatalog.getFormat(cls,
                                        true /*checkEntitySubclassIndexes*/);
        }
    }

    /**
     * See Store.refresh.
     */
    void refresh(final PersistCatalog newCatalog) {
        catalog = newCatalog;
        entityFormat = newCatalog.getFormat(entityFormat.getClassName());
        if (keyAssigner != null) {
            keyAssigner.refresh(newCatalog);
        }
    }
}
