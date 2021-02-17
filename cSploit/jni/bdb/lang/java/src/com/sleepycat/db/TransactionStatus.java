/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbEnv;

/**
Status values from transaction application checking operations.
*/
public final class TransactionStatus {
    /**
    The transaction has been applied on the current replica node.
    */
    public static final TransactionStatus APPLIED =
        new TransactionStatus("APPLIED", 0);
    /**
    The transaction has not yet been applied on this replica node.
    */
    public static final TransactionStatus TIMEOUT =
        new TransactionStatus("TIMEOUT", DbConstants.DB_TIMEOUT);
    /**
    The transaction did not update the database.
    */
    public static final TransactionStatus EMPTY_TRANSACTION =
        new TransactionStatus("EMPTY_TRANSACTION", DbConstants.DB_KEYEMPTY);
    /**
    No such transaction found in the replication group.
    */
    public static final TransactionStatus NOTFOUND =
        new TransactionStatus("NOTFOUND", DbConstants.DB_NOTFOUND);

    /* package */
    static TransactionStatus fromInt(final int errCode) {
        switch(errCode) {
        case 0:
            return APPLIED;
        case DbConstants.DB_TIMEOUT:
            return TIMEOUT;
        case DbConstants.DB_KEYEMPTY:
            return EMPTY_TRANSACTION;
        case DbConstants.DB_NOTFOUND:
            return NOTFOUND;
        default:
            throw new IllegalArgumentException(
                "Unknown error code: " + DbEnv.strerror(errCode));
        }
    }

    /* For toString */
    private String statusName;
    private int errCode;

    private TransactionStatus(final String statusName, int errCode) {
        this.statusName = statusName;
        this.errCode = errCode;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "TransactionStatus." + statusName;
    }
}
