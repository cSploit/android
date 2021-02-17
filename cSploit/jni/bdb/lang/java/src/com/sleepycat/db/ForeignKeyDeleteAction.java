/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/**
 * The action taken when a referenced record in the foreign key database is
 * deleted.
 *
 * <p>The delete action applies to a secondary database that is configured to
 * have a foreign key integrity constraint.  The delete action is specified by
 * calling {@link SecondaryConfig#setForeignKeyDeleteAction}.</p>
 *
 * <p>When a record in the foreign key database is deleted, it is checked to
 * see if it is referenced by any record in the associated secondary database.
 * If the key is referenced, the delete action is applied.  By default, the
 * delete action is {@link #ABORT}.</p>
 *
 * @see SecondaryConfig
 */
public class ForeignKeyDeleteAction {
    private String name;
    private int id;

    private ForeignKeyDeleteAction(String name, int id) {
	this.name = name;
	this.id = id;
    }

    public String toString() {
	return "ForeignKeyDeleteAction." + name;
    }

    /* package */
    int getId() {
	return id;
    }

    static ForeignKeyDeleteAction fromInt(int type) {
	switch(type) {
	case DbConstants.DB_FOREIGN_ABORT:
	    return ABORT;
	case DbConstants.DB_FOREIGN_CASCADE:
	    return CASCADE;
	case DbConstants.DB_FOREIGN_NULLIFY:
	    return NULLIFY;
	default:
	    throw new IllegalArgumentException("Unknown action type: " + type);
	}
    }

    /**
     * When a referenced record in the foreign key database is deleted, abort
     * the transaction by throwing a {@link DatabaseException}.
     */
    public static ForeignKeyDeleteAction ABORT =
	new ForeignKeyDeleteAction("ABORT", DbConstants.DB_FOREIGN_ABORT);
    /**
     * When a referenced record in the foreign key database is deleted, delete
     * the primary database record that references it.
     */
    public static ForeignKeyDeleteAction CASCADE =
	new ForeignKeyDeleteAction("CASCADE", DbConstants.DB_FOREIGN_CASCADE);
    /**
     * When a referenced record in the foreign key database is deleted, set the
     * reference to null in the primary database record that references it,
     * thereby deleting the secondary key. @see ForeignKeyNullifier @see
     * ForeignMultiKeyNullifier
     */
    public static ForeignKeyDeleteAction NULLIFY =
	new ForeignKeyDeleteAction("NULLIFY", DbConstants.DB_FOREIGN_NULLIFY);
}
