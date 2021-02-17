/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import java.util.Set;

import com.sleepycat.persist.model.ClassMetadata;
import com.sleepycat.persist.model.EntityMetadata;
import com.sleepycat.persist.model.EntityModel;

/**
 * The EntityModel used when a RawStore is opened.  The metadata and raw type
 * information comes from the catalog directly, without using the current
 * class definitions.
 *
 * @author Mark Hayes
 */
class StoredModel extends EntityModel {

    private volatile PersistCatalog catalog;
    private volatile Set<String> knownClasses;

    StoredModel(final PersistCatalog catalog) {
        this.catalog = catalog;
    }

    /**
     * This method is used to initialize the model when catalog creation is
     * complete, and reinitialize it when a Replica refresh occurs.
     */
    @Override
    protected void setCatalog(final PersistCatalog newCatalog) {
        super.setCatalog(newCatalog);
        this.catalog = newCatalog;
        knownClasses = newCatalog.getModelClasses();
    }

    @Override
    public ClassMetadata getClassMetadata(String className) {
        ClassMetadata metadata = null;
        Format format = catalog.getFormat(className);
        if (format != null && format.isCurrentVersion()) {
            metadata = format.getClassMetadata();
        }
        return metadata;
    }

    @Override
    public EntityMetadata getEntityMetadata(String className) {
        EntityMetadata metadata = null;
        Format format = catalog.getFormat(className);
        if (format != null && format.isCurrentVersion()) {
            metadata = format.getEntityMetadata();
        }
        return metadata;
    }

    @Override
    public Set<String> getKnownClasses() {
        return knownClasses;
    }
}
