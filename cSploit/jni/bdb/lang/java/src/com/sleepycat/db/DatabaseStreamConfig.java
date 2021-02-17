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
import com.sleepycat.db.internal.Dbc;

/**
Specify the attributes of database stream.  An instance created with the
default constructor is initialized with the system's default settings.
*/
public class DatabaseStreamConfig implements Cloneable {
    private boolean readOnly = false;
    private boolean readOnlyIsSet = false;
    private boolean syncPerWrite = false;

    /**
    Default configuration used if null is passed to methods that create a
    database stream.
    */
    public static final DatabaseStreamConfig DEFAULT =
        new DatabaseStreamConfig();

    /* package */
    static DatabaseStreamConfig checkNull(DatabaseStreamConfig config) {
        return (config == null) ? DEFAULT : config;
    }

    /**
    Configure the database stream as read only.
    <p>
    @param readOnly
    If true, configure the database stream to read only.
    */
    public void setReadOnly(final boolean readOnly) {
        this.readOnlyIsSet = true;
        this.readOnly = readOnly;
    }

    /**
    Return true if the database stream is read only.
    <p>
    @return
    true if the database stream is configured to read only.
    */
    public boolean getReadOnly() {
        return readOnly;
    }

    /**
    Configure the database stream to sync the blob on each write.
    <p>
    @param syncPerWrite
    If true, configure the database stream to sync the blob on
    each write.
    */
    public void setSyncPerWrite(final boolean syncPerWrite) {
        this.syncPerWrite = syncPerWrite;
    }

    /**
    Return if the database stream is configured to sync the blob
    on each write.
    <p>
    @return
    true if the database stream is configured to sync the blob
    on each write, and false otherwise.
    */
    public boolean getSyncPerWrite() {
        return syncPerWrite;
    }

    /* package */
    DbStream openDatabaseStream(final Dbc dbc)
        throws DatabaseException {

        int flags = 0;
        if (readOnlyIsSet)
            flags |= readOnly ? DbConstants.DB_STREAM_READ :
                DbConstants.DB_STREAM_WRITE;
        if (syncPerWrite)
                flags |= DbConstants.DB_STREAM_SYNC_WRITE;
        return dbc.db_stream(flags);
    }

}
