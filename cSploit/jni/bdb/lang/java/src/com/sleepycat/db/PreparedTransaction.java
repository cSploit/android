/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
package com.sleepycat.db;

import com.sleepycat.db.internal.DbTxn;

/**
The PreparedTransaction object is used to encapsulate a single prepared,
but not yet resolved, transaction.
*/
public class PreparedTransaction {
    private byte[] gid;
    private Transaction txn;

    PreparedTransaction(final DbTxn txn, final byte[] gid) {
        this.txn = new Transaction(txn);
        this.gid = gid;
    }

    /**
    Return the global transaction ID for the transaction. The global transaction
    ID is the one specified when the transaction was prepared. The application is
    responsible for ensuring uniqueness among global transaction IDs.
    <p>
    @return The global transaction ID for the transaction.
    */
    public byte[] getGID() {
        return gid;
    }

    /**
    Return the transaction handle for the transaction.
    <p>
    @return
    The transaction handle for the transaction.
    */
    public Transaction getTransaction() {
        return txn;
    }
}
