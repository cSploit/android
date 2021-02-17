/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

/**
An interface specifying how to set up database partitioning.
Implementation of this interface is required when
{@link com.sleepycat.db.Database} is opened and
{@link DatabaseConfig#setPartitionByCallback DatabaseConfig.setPartitionByCallback} is called.
<p>
You configure the database with this handler using the
{@link DatabaseConfig#setPartitionByCallback DatabaseConfig.setPartitionByCallback} method.
*/
public interface PartitionHandler {
    /**
    The application-specific database partitioning callback.
    <p>
    @param db
    The enclosing database handle.
    @param key
    A database entry representing a database key.
    @return
    A partition number for the key.
    */
    int partition(Database db, DatabaseEntry key);
}
