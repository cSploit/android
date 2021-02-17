/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// Configuration properties for a site in replication manager.
    /// </summary>
    public class DbSiteConfig {

        /// <summary>
        /// The host of the site.
        /// </summary>
        public string Host;

        /// <summary>
        /// The port of the site.
        /// </summary>
        public uint Port;

        private bool _groupCreator;
        internal bool groupCreatorIsSet;
        /// <summary>
        /// Whether the site is a group creator.
        /// </summary>
        /// <remarks>
        /// Only the local site could be applied as a group creator. The group
        /// creator would create the initial membership database, defining a
        /// replication group of just the one site, rather than trying to join
        /// an existing group when it starts for the first time.
        /// </remarks>
        public bool GroupCreator {
            get { return _groupCreator; }
            set {
                _groupCreator = value;
                groupCreatorIsSet = true;
            }
        }

        private bool _helper;
        internal bool helperIsSet;
        /// <summary>
        /// Whether the site is a helper site.
        /// </summary>
        /// <remarks>
        /// A remote site may be a helper site when the local site first joins
        /// the replication group. Once the local site has been established as
        /// a member of the group, this config setting is ignored. 
        /// </remarks>
        public bool Helper {
            get { return _helper; }
            set {
                _helper = value;
                helperIsSet = true;
            }
        }

        private bool _legacy;
        internal bool legacyIsSet;
        /// <summary>
        /// Whether the site is within a legacy group.
        /// </summary>
        /// <remarks>
        /// Specify the site in a legacy group. It would be considered as part
        /// of an existing group, upgrading from a previous version of BDB. All
        /// sites in the legacy group must specify this for themselves (the 
        /// local site) and for all other sites initially in the group. 
        /// </remarks>
        public bool Legacy {
            get { return _legacy; }
            set {
                _legacy = value;
                legacyIsSet = true;
            }
        }

        private bool _localSite;
        internal bool localSiteIsSet;
        /// <summary>
        /// Whether it is local site.
        /// </summary>
        public bool LocalSite {
            get { return _localSite; }
            set {
                _localSite = value;
                localSiteIsSet = true;
            }
        }

        private bool _peerSite;
        internal bool peerIsSet;
        /// <summary>
        /// Whether the site is peer to local site. 
        /// </summary>
        /// <remarks>
        /// A peer site may be used as a target for "client-to-client" 
        /// synchronization messages. It only makes sense to specify this for a
        /// remote site. 
        /// </remarks>
        public bool Peer {
            get { return _peerSite; }
            set {
                _peerSite = value;
                peerIsSet = true;
            }
        }

        /// <summary>
        /// Create a new DbSiteConfig object
        /// </summary>
        public DbSiteConfig() {
            Host = null;
            Port = 0;
        }
    }
}
