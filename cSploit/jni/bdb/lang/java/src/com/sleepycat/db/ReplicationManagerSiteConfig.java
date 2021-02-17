/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbSite;

/**
Specifies the attributes of a site in the replication group.
*/
public class ReplicationManagerSiteConfig implements Cloneable {
    /* package */

    private String host = null;
    private long port = 0;
    private boolean helper = false;
    private boolean groupCreator = false;
    private boolean legacy = false;
    private boolean localSite = false;
    private boolean peer = false;

    /**
    An instance created using the default constructor is initialized
    with the system's default settings.
    */
    public ReplicationManagerSiteConfig() {
    }

    /**
    Configure the host and port for a site in replication group.
    @param host
    @param port
    */
    public ReplicationManagerSiteConfig(String host, long port) {
        this.host = host;
        this.port = port;
    }

    /**
    Configure the address for a site in replication group.
    <p>
    @param address
    */
    public void setAddress(ReplicationHostAddress address) {
        this.host = address.host;
        this.port = address.port;
    }

    /**
    Return the address of the site.
    <p>
    @return the address of the site.
    */
    public ReplicationHostAddress getAddress() {
	ReplicationHostAddress address = new ReplicationHostAddress();
	if (this.host != null && this.port != 0) {
            address.host = this.host;
            address.port = (int)this.port;
        }
        return address;
    }

    /**
    Configure the host of the site.
    <p>
    @param host
    */
    public void setHost(String host) {
        this.host = host;
    }

    /**
    Return the host of the site.
    <p>
    @return the host of the site.
    */
    public String getHost() {
        return host;
    }

    /**
    Configure the port of the site.
    <p>
    @param port
    */
    public void setPort(long port) {
        this.port = port;
    }

    /**
    Return the port of the site.
    <p>
    @return the port of the site.
    */
    public long getPort() {
        return port;
    }

    /**
    Set the site to be a helper site.
    <p>
    A remote site may be used as a helper when the local site first joins the
    replication group. Once the local site has been established as a member of
    the group, this config setting is ignored. 
    <p>
    @param helper
    If true, the site will be a helper.
    */
    public void setBootstrapHelper(final boolean helper) {
        this.helper = helper;
    }

    /**
    Return if the site is a helper for the local site.
    <p>
    @return
    If the site is a helper for the local site.
    */
    public boolean getBootstrapHelper() {
        return helper;
    }

    /**
    Set the site to be a group creator.
    <p>
    Only the local site could be applied as a group creator. The group creator
    would create the initial membership database, defining a replication group
    of just the one site, rather than trying to join an existing group when it 
    starts for the first time.
    <p>
    @param groupCreator
    If true, set the site a group creator.
    */
    public void setGroupCreator(final boolean groupCreator) {
        this.groupCreator = groupCreator;
    }

    /**
    Return if the site is a group creator.
    <p>
    @return
    If the the site is a group creator.
    */
    public boolean getGroupCreator() {
        return groupCreator;
    }

    /**
    Specify the site in a legacy group. It would be considered as part of an
    existing group, upgrading from a previous version of BDB. All sites in the
    legacy group must specify this for themselves (the local site) and for all
    other sites initially in the group. 
    <p>
    @param legacy
    If true, specify the site in a legacy group.
    */
    public void setLegacy(final boolean legacy) {
        this.legacy = legacy;
    }

    /**
    Return if the site is in a legacy group.
    <p>
    @return
    If the site is in a legacy group.
    */
    public boolean getLegacy() {
        return legacy;
    }

    /**
    Set the site to be the local site.
    <p>
    @param localSite
    If true, it is local site.
    */
    public void setLocalSite(final boolean localSite) {
        this.localSite = localSite;
    }

    /**
    Return if the site is the local site.
    <p>
    @return
    If the site is the local site.
    */
    public boolean getLocalSite() {
        return localSite;
    }

    /**
    Set the site to be peer to local site. 
    <p>
    A peer site may be used as a target for "client-to-client" synchronization
    messages. It only makes sense to specify this for a remote site. 
    @param peer
    If true, it is peer to loca site.
    */
    public void setPeer(final boolean peer) {
        this.peer = peer;
    }

    /**
    Return if the site is peer.
    <p>
    @return
    If the site is peer.
    */
    public boolean getPeer() {
        return peer;
    }

    /* package */
    ReplicationManagerSiteConfig(final DbSite dbsite)
        throws DatabaseException {
        ReplicationHostAddress address = dbsite.get_address();
        host = address.host;
        port = address.port;

        helper = dbsite.get_config(DbConstants.DB_BOOTSTRAP_HELPER);
        groupCreator = dbsite.get_config(DbConstants.DB_GROUP_CREATOR);
        legacy = dbsite.get_config(DbConstants.DB_LEGACY);
        localSite = dbsite.get_config(DbConstants.DB_LOCAL_SITE);
        peer = dbsite.get_config(DbConstants.DB_REPMGR_PEER);
    }
}
