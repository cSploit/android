/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

/**
 * The interface implemented for setting multi-valued foreign keys to null.
 *
 * <p>A key nullifier is used with a secondary database that is configured to
 * have a foreign key integrity constraint and a delete action of {@link
 * ForeignKeyDeleteAction#NULLIFY}.  The key nullifier is specified by calling
 * {@link SecondaryConfig#setForeignMultiKeyNullifier}.</p>
 *
 * <p>When a referenced record in the foreign key database is deleted and the
 * foreign key delete action is <code>NULLIFY</code>, the {@link
 * ForeignMultiKeyNullifier#nullifyForeignKey} method is called.  This method
 * sets the foreign key reference to null in the datum of the primary
 * database. The primary database is then updated to contain the modified
 * datum.  The result is that the secondary key is deleted.</p>
 *
 * This interface may be used along with {@link SecondaryKeyCreator} or {@link
 * SecondaryMultiKeyCreator} for many-to-many, one-to-many, many-to-one and
 * one-to-one relationships.
 */
public interface ForeignMultiKeyNullifier {
    /**
     * Sets the foreign key reference to null in the datum of the primary
     * database.
     *
     * @param secondary the database in which the foreign key integrity
     * constraint is defined. This parameter is passed for informational
     * purposes but is not commonly used.
     *
     * @param key the existing primary key.  This parameter is passed for
     * informational purposes but is not commonly used.
     *
     * @param data the existing primary datum in which the foreign key
     * reference should be set to null.  This parameter should be updated by
     * this method if it returns true.
     *
     * @param secKey the secondary key to be nullified.  This parameter is
     * needed for knowing which key to nullify when multiple keys are present,
     * as when {@link SecondaryMultiKeyCreator} is used.
     *
     * @return true if the datum was modified, or false to indicate that the
     * key is not present.
     *
     * @throws DatabaseException if an error occurs attempting to clear the key
     * reference.
     */
    boolean nullifyForeignKey(SecondaryDatabase secondary, DatabaseEntry key, DatabaseEntry data, DatabaseEntry secKey)
	    throws DatabaseException;
}
