/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id: StoreExistsException.java,v 0f73af5ae3da 2010/05/10 05:38:40 alexander $
 */

package com.sleepycat.persist;

import com.sleepycat.db.DatabaseException;

/**
 * Thrown by the {@link EntityStore} constructor when the {@link
 * StoreConfig#setExclusiveCreate ExclusiveCreate} configuration parameter is
 * true and the store's internal catalog database already exists.
 *
 * @author Mark Hayes
 */
public class StoreExistsException extends DatabaseException {

    private static final long serialVersionUID = 1;

    public StoreExistsException(String message) {
        super(message);
    }
}
