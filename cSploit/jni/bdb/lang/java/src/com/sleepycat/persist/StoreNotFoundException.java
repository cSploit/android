/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id: StoreNotFoundException.java,v 0f73af5ae3da 2010/05/10 05:38:40 alexander $
 */

package com.sleepycat.persist;

import com.sleepycat.db.DatabaseException;

/**
 * Thrown by the {@link EntityStore} constructor when the {@link
 * StoreConfig#setAllowCreate AllowCreate} configuration parameter is false and
 * the store's internal catalog database does not exist.
 *
 * @author Mark Hayes
 */
public class StoreNotFoundException extends DatabaseException {

    private static final long serialVersionUID = 1895430616L;

    public StoreNotFoundException(String message) {
        super(message);
    }
}
