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
Settings that configure Berkeley DB replication.  Used in the {@link
Environment#setReplicationConfig} method.
*/
public final class ReplicationConfig implements Cloneable {
    /**
    The replication master should send groups of records to the clients in a
    single network transfer.
    **/
    public static final ReplicationConfig BULK =
        new ReplicationConfig("BULK", DbConstants.DB_REP_CONF_BULK);

    /**
    The client should delay synchronizing to a newly declared master
    (defaults to off).  Clients configured in this way will remain
    unsynchronized until the application calls the
    {@link Environment#syncReplication} method.
    **/
    public static final ReplicationConfig DELAYCLIENT =
      new ReplicationConfig("DELAYCLIENT", DbConstants.DB_REP_CONF_DELAYCLIENT);

    /**
    The replication master should automatically re-initialize outdated
    clients (defaults to on.)
    **/
    public static final ReplicationConfig AUTOINIT =
        new ReplicationConfig("AUTOINIT", DbConstants.DB_REP_CONF_AUTOINIT);

    /**
    Berkeley DB method calls that would normally block while clients are in
    recovery should return errors immediately.
    **/
    public static final ReplicationConfig NOWAIT =
        new ReplicationConfig("NOWAIT", DbConstants.DB_REP_CONF_NOWAIT);

    /** 
    Replication Manager observes the strict "majority" rule in managing
    elections, even in a group with only 2 sites.  This means the client in a
    2-site group will be unable to take over as master if the original master
    fails or becomes disconnected.  (See the
    <a href="{@docRoot}/../programmer_reference/rep_elect.html" target="_top">Elections</a>
    section in the Berkeley DB Reference Guide for more information.)  Both sites
    in the replication group should have the same value for this parameter.
    **/
    public static final ReplicationConfig STRICT_2SITE =
        new ReplicationConfig("STRICT_2SITE",
                              DbConstants.DB_REPMGR_CONF_2SITE_STRICT);

    /**
    Replication Manager automatically runs elections to choose a new
    master when the old master appears to have become disconnected.
    This option is turned on by default.
     */
    public static final ReplicationConfig ELECTIONS =
        new ReplicationConfig("ELECTIONS",
                              DbConstants.DB_REPMGR_CONF_ELECTIONS);

    /** 
    Master leases will be used for this site.
    <p>
    Configuring this option may result in the {@link Database#get Database.get()}
    and {@link Cursor Cursor.get*()} methods throwing a
    {@link DatabaseException} when attempting to read entries from a database
    after the site's master lease has expired.
    <p>
    Once this option is turned on, it may never be turned off.
    */
    public static final ReplicationConfig LEASE =
	new ReplicationConfig("LEASE", DbConstants.DB_REP_CONF_LEASE);

    /* package */
    static ReplicationConfig fromInt(int which) {
        switch(which) {
        case DbConstants.DB_REP_CONF_BULK:
            return BULK;
        case DbConstants.DB_REP_CONF_DELAYCLIENT:
            return DELAYCLIENT;
        case DbConstants.DB_REP_CONF_AUTOINIT:
            return AUTOINIT;
        case DbConstants.DB_REP_CONF_NOWAIT:
            return NOWAIT;
        case DbConstants.DB_REPMGR_CONF_2SITE_STRICT:
            return STRICT_2SITE;
        case DbConstants.DB_REPMGR_CONF_ELECTIONS:
            return ELECTIONS;
	case DbConstants.DB_REP_CONF_LEASE:
	    return LEASE;
        default:
            throw new IllegalArgumentException(
                "Unknown replication config: " + which);
        }
    }

    private String configName;
    private int flag;

    private ReplicationConfig(final String configName, final int flag) {
        this.configName = configName;
        this.flag = flag;
    }

    /* package */
    int getFlag() {
        return flag;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "ReplicationConfig." + configName;
    }
}
