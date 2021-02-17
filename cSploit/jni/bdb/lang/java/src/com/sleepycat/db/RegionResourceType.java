/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/**
 * Advanced configuration is possible for various systems that use shared
 * memory (regions) for cross process sharing. This class defines those
 * resources. 
 */
public final class RegionResourceType {
    /**
    The locks.
    */
    public static final RegionResourceType LOCK =
        new RegionResourceType("LOCK", DbConstants.DB_MEM_LOCK);
    /**
    The lock objects.
    */
    public static final RegionResourceType LOCK_OBJECT =
        new RegionResourceType("LOCK_OBJECT", DbConstants.DB_MEM_LOCKOBJECT);
    /**
    The lockers.
    */
    public static final RegionResourceType LOCKER =
        new RegionResourceType("LOCKER", DbConstants.DB_MEM_LOCKER);
    /**
    The log identifiers.
    */
    public static final RegionResourceType LOG_ID =
        new RegionResourceType("LOG_ID", DbConstants.DB_MEM_LOGID);
    /**
    The transaction handles.
    */
    public static final RegionResourceType TRANSACTION =
        new RegionResourceType("TRANSACTION", DbConstants.DB_MEM_TRANSACTION);
    /**
    The threads.
    */
    public static final RegionResourceType THREAD =
        new RegionResourceType("THREAD", DbConstants.DB_MEM_THREAD);

    /* package */
    static RegionResourceType fromFlag(int flag) {
        switch (flag) {
        case 0:
        case DbConstants.DB_MEM_LOCK:
            return LOCK;
        case DbConstants.DB_MEM_LOCKOBJECT:
            return LOCK_OBJECT;
        case DbConstants.DB_MEM_LOCKER:
            return LOCKER;
        case DbConstants.DB_MEM_LOGID:
            return LOG_ID;
        case DbConstants.DB_MEM_TRANSACTION:
            return TRANSACTION;
        case DbConstants.DB_MEM_THREAD:
            return THREAD;
        default:
            throw new IllegalArgumentException(
                "Unknown region memory type: " + flag);
        }
    }

    private final String typeName;
    private final int flag;

    private RegionResourceType(final String typeName, final int flag) {
        this.typeName = typeName;
        this.flag = flag;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "RegionResourceType." + typeName;
    }

    /* package */
    int getFlag() {
        return flag;
    }
}
