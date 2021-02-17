/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$ 
 */

package com.sleepycat.db;

/**
 * The interface implemented for setting single-valued foreign keys to null.
 *
 * <p>A key nullifier is used with a secondary database that is configured to
 * have a foreign key integrity constraint and a delete action of {@link
 * ForeignKeyDeleteAction#NULLIFY}.  The key nullifier is specified by calling
 * {@link SecondaryConfig#setForeignKeyNullifier}.</p>
 *
 * <p>When a referenced record in the foreign key database is deleted and the
 * foreign key delete action is <code>NULLIFY</code>, the {@link
 * ForeignKeyNullifier#nullifyForeignKey} method is called.  This method sets
 * the foreign key reference to null in the datum of the primary database.  The
 * primary database is then updated to contain the modified datum.  The result
 * is that the secondary key is deleted.</p>
 *
 * This interface may be used along with {@link SecondaryKeyCreator} for
 * many-to-one and one-to-one relationships.  It may <em>not</em> be used with
 * {@link SecondaryMultiKeyCreator} because the secondary key is not passed as
 * a parameter to the nullifyForeignKey method and this method would not know
 * which key to nullify.  When using {@link SecondaryMultiKeyCreator}, use
 * {@link ForeignMultiKeyNullifier} instead.
 */
public interface ForeignKeyNullifier {
    /**
     * Sets the foreign key reference to null in the datum of the primary
     * database.
     *
     * @param secondary the database in which the foreign key integrity
     * constraint is defined. This parameter is passed for informational
     * purposes but is not commonly used.
     *
     * @param data the existing primary datum in which the foreign key
     * reference should be set to null.  This parameter should be updated by
     * this method if it returns true.
     *
     * @return true if the datum was modified, or false to indicate that the
     * key is not present.
     *
     * @throws DatabaseException if an error occurs attempting to clear the key
     * reference.
     */
    boolean nullifyForeignKey(SecondaryDatabase secondary, DatabaseEntry data)
	    throws DatabaseException;
}
