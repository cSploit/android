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
Describes the connection status of a replication site.  Returned from the {@link
ReplicationManagerSiteInfo#getConnectionStatus} method.
*/
public final class ReplicationManagerConnectionStatus implements Cloneable {
    /**
     * The site is connected.
     */
    public static final ReplicationManagerConnectionStatus CONNECTED =
        new ReplicationManagerConnectionStatus("CONNECTED", DbConstants.DB_REPMGR_CONNECTED);

    /**
     * The site is disconnected.
     */
    public static final ReplicationManagerConnectionStatus DISCONNECTED =
        new ReplicationManagerConnectionStatus("DISCONNECTED", DbConstants.DB_REPMGR_DISCONNECTED);

    /**
     * The connection status cannot be determined.
     */
    public static final ReplicationManagerConnectionStatus UNKNOWN =
        new ReplicationManagerConnectionStatus("UNKNOWN", 0);

    /* package */
    static ReplicationManagerConnectionStatus fromInt(int which) {
        switch(which) {
        case DbConstants.DB_REPMGR_CONNECTED:
            return CONNECTED;
        case DbConstants.DB_REPMGR_DISCONNECTED:
            return DISCONNECTED;
        case 0:
            return UNKNOWN;
        default:
            throw new IllegalArgumentException("Unknown connection status: " + which);
        }
    }

    private String configName;
    private int status;

    private ReplicationManagerConnectionStatus(final String configName, final int status) {
        this.configName = configName;
        this.status = status;
    }

    /* package */
    int getStatus() {
        return status;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "ReplicationManagerConnectionStatus." + configName;
    }
}
