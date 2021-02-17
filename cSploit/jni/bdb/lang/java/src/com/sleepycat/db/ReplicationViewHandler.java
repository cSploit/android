/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

/**
An interface specifying a callback function to be used by replication views.
*/
public interface ReplicationViewHandler {
    /**
    The application-specific function used by replication views to determine
    whether a database file is replicated.
    <p>
    @param dbenv
    The enclosing database environment handle.
    @param name
    The name of the database file.
    @param flags
    Currently unused.
    @return True if the file is replicated, or false if not.
    @throws DatabaseException if an error occurs when determining whether or
    not the database file is replicated.
    */
    boolean partial_view(Environment dbenv, String name, int flags)
        throws DatabaseException;
}
