/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.model;

import com.sleepycat.persist.impl.PersistCatalog;

/**
 * Internal access class that should not be used by applications.
 *
 * @author Mark Hayes
 */
public class ModelInternal {

    /**
     * Internal access method that should not be used by applications.
     */
    public static void setCatalog(EntityModel model, PersistCatalog catalog) {
        model.setCatalog(catalog);
    }

    /**
     * Internal access method that should not be used by applications.
     */
    public static void setClassLoader(EntityModel model, ClassLoader loader) {
        /* Do not overwrite loader with null value. */
        if (loader != null) {
            model.setClassLoader(loader);
        }
    }

    /**
     * Internal access method that should not be used by applications.
     */
    public static ClassLoader getClassLoader(EntityModel model) {
        return model.getClassLoader();
    }
}
