/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.lang.IllegalArgumentException;

/**
 Content used for the key in a Heap database record. Berkeley DB creates
 this value for you when you create a record in a Heap database. You
 should never create this structure yourself; Berkeley DB must create it
 for you.
 
 This structure is returned in the key DatabaseEntry parameter of the
  method that you use to add a record to the Heap database. 
*/ 
public class HeapRecordId {
    private int pgno;
    private short indx;

    /**
       Construct a new record id, given a page number and index.
       <p>
       @param pgno
       The database page number where the record is stored.
       <p>
       @param indx
       Index in the offset table where the record can be found.
    */
    public HeapRecordId(int pgno, short indx) {
	this.pgno = pgno;
	this.indx = indx;
    }

    /**
       Construct a HeapRecordId from a byte array, typically from
       a {@link com.sleepycat.db.DatabaseEntry}.
       <p>
       @param data
       The array representing the record id.
       <p>
       @return A new HeapRecordId
    */
    public static HeapRecordId fromArray(byte[] data)
        throws IllegalArgumentException {
        return fromArray(data, ByteOrder.LITTLE_ENDIAN);
    }
    /**
       Construct a HeapRecordId from a byte array, typically from
       a {@link com.sleepycat.db.DatabaseEntry}.
       <p>
       @param data
       The array representing the record id.
       <p>
       @param order
       The byte order of data stored in the array.
       <p>
       @return A new HeapRecordId
    */
    public static HeapRecordId fromArray(byte[] data, ByteOrder order)
        throws IllegalArgumentException {
        /* Need 4 bytes for int and 2 bytes for short. */
        if (data.length < 6) {
            throw new IllegalArgumentException("Invalid buffer size.");
        }

        ByteBuffer buf = ByteBuffer.wrap(data);
        buf.order(order);
        int pgno = buf.getInt();
        short indx = buf.getShort();

        return new HeapRecordId(pgno, indx);
    }

    /**
       Return a byte array representing this record id.
       <p>
       @return
       A byte array representing this record id.
    */
    public byte[] toArray() {
        return this.toArray(ByteOrder.LITTLE_ENDIAN);
    }
    /**
       Return a byte array representing this record id.
       <p>
       @param order
       The byte order to use when constructing the array.
       <p>
       @return
       A byte array representing this record id.
    */
    public byte[] toArray(ByteOrder order) {
        /* Need 4 bytes for int and 2 bytes for short. */
        ByteBuffer buf = ByteBuffer.allocate(6);
        buf.order(order);
        buf.putInt(this.pgno).putShort(this.indx);

        return buf.array();
    }

    /**
       Get the database page number where the record is stored.
       <p>
       @return
       The database page number where the record is stored.
    */
    public int getPageNumber() {
        return this.pgno;
    }
    /**
       Set the database page number where the record is stored.
    */
    public void setPageNumber(final int pgno) {
        this.pgno = pgno;
    }

    /**
       Get the index in the offset table where the record can be found.
       <p>
       @return
       The index in the offset table where the record can be found.
    */
    public short getIndex() {
        return this.indx;
    }
    /**
       Set the index in the offset table where the record can be found.
    */
    public void setIndex(final short indx) {
        this.indx = indx;
    }

}
