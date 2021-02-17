/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbEnv;

/**
Thrown if a new master has been chosen but the client is unable to synchronize
with the new master, possibly because the client has turned off automatic
internal initialization (the {@link ReplicationConfig#AUTOINIT} setting).
*/
public class ReplicationJoinFailureException extends DatabaseException {
    /* package */ ReplicationJoinFailureException(final String s,
                                   final int errno,
                                   final DbEnv dbenv) {
        super(s, errno, dbenv);
    }
}
