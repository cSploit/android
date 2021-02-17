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
A ReplicationManagerSite handle is used to manage a site in a replication group.
ReplicationManagerSite handles are opened using the {@link com.sleepycat.db.Environment#getReplicationManagerSite Environment.getReplicationManagerSite} method.
*/
public class ReplicationManagerSite {
    private DbSite dbsite;

    /* package */
    ReplicationManagerSite(final DbSite dbsite)
        throws DatabaseException {

        this.dbsite = dbsite;
        dbsite.wrapper = this;
    }

    /**
    Close the site.
    */
    public void close()
        throws DatabaseException {

        dbsite.close();
    }

    /**
    Get the address of the site.
    <p>
    @return the address of the site.
    */
    public ReplicationHostAddress getAddress()
        throws DatabaseException {

        ReplicationHostAddress address = dbsite.get_address();
        return address;
    }

    /**
    Get the address and configuration of the site.
    <p>
    @return the configuration of the site.
    */
    public ReplicationManagerSiteConfig getConfig()
        throws DatabaseException {

        return new ReplicationManagerSiteConfig(dbsite);
    }

    /**
    Get the environment id of the site.
    <p>
    @return the environment id of the site.
    */
    public int getEid()
    	throws DatabaseException {
    	return dbsite.get_eid();
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
    public void setBootstrapHelper(final boolean helper) 
        throws DatabaseException {

        dbsite.set_config(DbConstants.DB_BOOTSTRAP_HELPER, helper);
    }

    /**
    Return if the site is a helper for the local site.
    <p>
    @return
    If the site is a helper for the local site.
    */
    public boolean getBootstrapHelper() 
        throws DatabaseException {

        return dbsite.get_config(DbConstants.DB_BOOTSTRAP_HELPER);
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
    If true, set the site to be a group creator.
    */
    public void setGroupCreator(final boolean groupCreator) 
        throws DatabaseException {

        dbsite.set_config(DbConstants.DB_GROUP_CREATOR, groupCreator);
    }

    /**
    Return if the site is a group creator.
    <p>
    @return
    If the the site is a group creator.
    */
    public boolean getGroupCreator() 
        throws DatabaseException {

        return dbsite.get_config(DbConstants.DB_GROUP_CREATOR);
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
    public void setLegacy(final boolean legacy) 
        throws DatabaseException {

        dbsite.set_config(DbConstants.DB_LEGACY, legacy);
    }

    /**
    Return if the site is in a legacy group.
    <p>
    @return
    If the site is in a legacy group.
    */
    public boolean getLegacy() 
        throws DatabaseException {

        return dbsite.get_config(DbConstants.DB_LEGACY);
    }

    /**
    Set the site to be the local site.
    <p>
    @param localSite
    If true, it is local site.
    */
    public void setLocalSite(final boolean localSite) 
        throws DatabaseException {

        dbsite.set_config(DbConstants.DB_LOCAL_SITE, localSite);
    }

    /**
    Return if the site is the local site.
    <p>
    @return
    If the site is the local site.
    */
    public boolean getLocalSite() 
        throws DatabaseException {

        return dbsite.get_config(DbConstants.DB_LOCAL_SITE);
    }

    /**
    Set the site to be peer to local site. 
    <p>
    A peer site may be used as a target for "client-to-client" synchronization
    messages. It only makes sense to specify this for a remote site. 
    @param peer
    If true, it is peer to loca site.
    */
    public void setPeer(final boolean peer) 
        throws DatabaseException {

        dbsite.set_config(DbConstants.DB_REPMGR_PEER, peer);
    }

    /**
    Return if the site is peer to local site.
    <p>
    @return
    If the site is peer to local site.
    */
    public boolean getPeer()
        throws DatabaseException {

        return dbsite.get_config(DbConstants.DB_REPMGR_PEER);
    }

    /**
    Remove the site.
    */
    public void remove()
        throws DatabaseException {

        dbsite.remove();
    }
}
