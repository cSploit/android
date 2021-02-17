/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2013, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbStream;

/**
A database stream. The database stream is used to access the blob.
<p>
Once the database stream close method has been called, the handle may not
be accessed again.
<p>
To obtain a database stream with default attributes:
<blockquote><pre>
    DatabaseStream dbs = myCursor.openDatabaseStream(null);
</pre></blockquote>
To customize the attributes of a database stream,
use a DatabaseStreamConfig object.
<blockquote><pre>
    DatabaseStreamConfig config = new DatabaseStreamConfig();
    config.setReadOnly(true);
    DatabaseStream dbs = myCursor.openDatabaseStream(config);
</pre></blockquote>
*/
public class DatabaseStream {
    /* package */ DbStream dbs;
    /* package */ Cursor cursor;
    /* package */ DatabaseStreamConfig config;

    protected DatabaseStream(
        final Cursor cursor, final DatabaseStreamConfig config) {
        this.cursor = cursor;
        this.config = config;
    }

    DatabaseStream(final Cursor cursor,
        final DbStream dbs, final DatabaseStreamConfig config)
        throws DatabaseException {

        this.cursor = cursor;
        this.dbs = dbs;
        this.config = config;
    }

    /**
    Discard the database stream.
    <p>
    After the close method has been called, you cannot use the database stream
    handle again.
    <p>
    It is recommended to always close all database stream handles immediately
    after their use to release resources.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public synchronized void close()
        throws DatabaseException {

        if (dbs != null) {
            try {
                dbs.close(0);
            } finally {
                dbs = null;
            }
        }
    }

    /**
    Return this database stream configuration.
    <p>
    @return
    This database stream configuration.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public DatabaseStreamConfig getConfig() {
        return config;
    }

    /**
    Return the {@link com.sleepycat.db.Cursor Cursor} handle associated
    with this database stream.
    <p>
    @return
    The cursor handle associated with this database stream.
    */
    public Cursor getCursor() {
        return cursor;
    }

    /**
    Read from the blob accessed by this database stream.
    <p>
    @throws IllegalArgumentException if a failure occurs.
    <p>
    @param data the data read from the blob
    returned as output.  Its byte array does not need to be initialized by the
    caller.
    <p>
    @param offset the position in bytes in the blob where the reading starts.
    <p>
    @param size the number of bytes to read.
    <p>
    @return {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}
    if the operation succeeds.
    <p>
    @throws IllegalArgumentException if the operation fails.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public OperationStatus read(final DatabaseEntry data, long offset, int size)
        throws DatabaseException, IllegalArgumentException {

        return OperationStatus.fromInt(
            dbs.read(data, offset, size, 0));
    }

    /**
    Return the size in bytes of the blob accessed by the database stream.
    <p>
    @return
    The size in bytes of the blob accessed by the database stream.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public long size()
        throws DatabaseException {
        return dbs.size(0);
    }

    /**
    Write to the blob accessed by the database stream.
    <p>
    @param data the data {@link com.sleepycat.db.DatabaseEntry DatabaseEntry}
    to write into the blob.
    <p>
    @param offset the position in bytes in the blob where the writing starts.
    <p>
    @return {@link com.sleepycat.db.OperationStatus#SUCCESS OperationStatus.SUCCESS}
    if the operation succeeds.
    <p>
    @throws IllegalArgumentException if the operation fails.
    <p>
    @throws DatabaseException if a failure occurs.
    */
    public OperationStatus write(final DatabaseEntry data, long offset)
        throws DatabaseException, IllegalArgumentException {

        return OperationStatus.fromInt(dbs.write(data, offset, 0));
    }

}
