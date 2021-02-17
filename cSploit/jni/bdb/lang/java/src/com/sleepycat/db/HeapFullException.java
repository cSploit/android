/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbEnv;


/**
A HeapFullException is thrown when an attempt was made to add or update a record
in a Heap database. However, the size of the database was constrained using
{@link com.sleepycat.db.DatabaseConfig#setHeapsize DatabaseConfig.setHeapsize},
and that limit has been reached.
*/
public class HeapFullException extends DatabaseException {
    private String message;

    /* package */ HeapFullException(final String message,
  				final int errno, final DbEnv dbenv) {
        super(message, errno, dbenv);
	this.message = message;
    }

   /** {@inheritDoc} */
    public String toString() {
        return message;
    }

}
