/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import java.math.BigDecimal;
import java.math.BigInteger;

/**
 * Used for reading object fields.
 *
 * <p>Unlike TupleInput, Strings are returned by {@link #readObject} when using
 * this class.</p>
 *
 * @author Mark Hayes
 */
public interface EntityInput {

    /**
     * Returns the Catalog associated with this input.
     */
    Catalog getCatalog();

    /**
     * Return whether this input is in raw mode, i.e., whether it is returning
     * raw instances.
     */
    boolean isRawAccess();

    /**
     * Changes raw mode and returns the original mode, which is normally
     * restored later.  For temporarily changing the mode during a conversion.
     */
    boolean setRawAccess(boolean rawAccessParam);

    /**
     * Called via Accessor to read all fields with reference types, except for
     * the primary key field and composite key fields (see readKeyObject
     * below).
     */
    Object readObject()
        throws RefreshException;

    /**
     * Called for a primary key field or a composite key field with a reference
     * type.
     *
     * <p>For such key fields, no formatId is present nor can the object
     * already be present in the visited object set.</p>
     */
    Object readKeyObject(Format format)
        throws RefreshException;

    /**
     * Called for a String field, that is not a primary key field or a
     * composite key field with a reference type.
     *
     * <p>For the new String format, no formatId is present nor can the object
     * already be present in the visited object set.  For the old String
     * format, this method simply calls readObject for compatibility.</p>
     */
    Object readStringObject()
        throws RefreshException;

    /**
     * Called via Accessor.readSecKeyFields for a primary key field with a
     * reference type.  This method must be called before reading any other
     * fields.
     */
    void registerPriKeyObject(Object o);
    
    /**
     * Called via Accessor.readSecKeyFields for a primary String key field.  
     * This method must be called before reading any other fields.
     */
    void registerPriStringKeyObject(Object o);

    /**
     * Called by ObjectArrayFormat and PrimitiveArrayFormat to read the array
     * length.
     */
    int readArrayLength();

    /**
     * Called by EnumFormat to read and return index of the enum constant.
     */
    int readEnumConstant(String[] names);

    /**
     * Called via PersistKeyCreator to skip fields prior to the secondary key
     * field.  Also called during class evolution so skip deleted fields.
     */
    void skipField(Format declaredFormat)
        throws RefreshException;

    /* The following methods are a subset of the methods in TupleInput. */

    String readString()
        throws RefreshException;
    char readChar()
        throws RefreshException;
    boolean readBoolean()
        throws RefreshException;
    byte readByte()
        throws RefreshException;
    short readShort()
        throws RefreshException;
    int readInt()
        throws RefreshException;
    long readLong()
        throws RefreshException;
    float readSortedFloat()
        throws RefreshException;
    double readSortedDouble()
        throws RefreshException;
    BigInteger readBigInteger()
        throws RefreshException;
    BigDecimal readSortedBigDecimal()
        throws RefreshException;
}
