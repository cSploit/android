/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing a replication site used by Replication Manager
    /// </summary>
    public class RepMgrSite {
        
        /// <summary>
        /// Environment ID assigned by the replication manager. This is the same
        /// value that is passed to
        /// <see cref="DatabaseEnvironment.EventNotify"/> for the
        /// <see cref="NotificationEvent.REP_NEWMASTER"/> event.
        /// </summary>
        public int EId;
        /// <summary>
        /// The address of the site
        /// </summary>
        public ReplicationHostAddress Address;
        /// <summary>
        /// If true, the site is connected.
        /// </summary>
        public bool isConnected;
        /// <summary>
        /// If true, the site is client-to-client peer.
        /// </summary>
        public bool isPeer;
        /// <summary>
        /// If true, the site is a view site.
        /// </summary>
        public bool isView;

        internal RepMgrSite(DB_REPMGR_SITE site) {
            EId = site.eid;
            Address = new ReplicationHostAddress(site.host, site.port);
            isConnected = (site.status == DbConstants.DB_REPMGR_CONNECTED);
            isPeer = (site.flags & DbConstants.DB_REPMGR_ISPEER) != 0;
            isView = (site.flags & DbConstants.DB_REPMGR_ISVIEW) != 0;
        }
    }
}
