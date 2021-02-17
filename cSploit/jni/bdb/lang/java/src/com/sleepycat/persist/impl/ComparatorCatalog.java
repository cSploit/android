/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import java.util.Map;

/**
 * Read-only catalog used by a PersistComparator to return simple formats plus
 * reconstituted enum formats.
 *
 * @author Mark Hayes
 */
class ComparatorCatalog extends SimpleCatalog {

    private final Map<String, Format> formatMap;

    ComparatorCatalog(final ClassLoader classLoader,
                      final Map<String, Format> formatMap) {
        super(classLoader);
        this.formatMap = formatMap;
    }

    public Format getFormat(final String className) {
        if (formatMap != null) {
            final Format f = formatMap.get(className);
            if (f != null) {
                return f;
            }
        }
        return super.getFormat(className);
    }
}
