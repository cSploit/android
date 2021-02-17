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
    /// A class representing a site in Berkeley DB HA replication manager.
    /// </summary>
    public class DbSite : IDisposable {
        private DB_SITE site;
        private bool isOpen;

        /// <summary>
        /// The host and port of the site.
        /// </summary>
        public ReplicationHostAddress Address {
            get {
                ReplicationHostAddress address = new ReplicationHostAddress();
                IntPtr intPtr;
                uint port = 0;
                site.get_address(out intPtr, ref port);
                address.Host = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(intPtr);
                address.Port = port;
                return address;
            }
        }

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
            get {
                uint ret = 0;
                site.get_config(DbConstants.DB_GROUP_CREATOR, ref ret);
                return (ret != 0) ? true : false;
            }
            set {
                site.set_config(DbConstants.DB_GROUP_CREATOR,
                    Convert.ToUInt32(value));
            }
        }

        /// <summary>
        /// Whether the site is a helper site.
        /// </summary>
        /// <remarks>
        /// A remote site may be a helper site when the local site first joins
        /// the replication group. Once the local site has been established as
        /// a member of the group, this config setting is ignored. 
        /// </remarks>
        public bool Helper {
            get {
                uint ret = 0;
                site.get_config(DbConstants.DB_BOOTSTRAP_HELPER, ref ret);
                return (ret != 0) ? true : false;
            }
            set {
                site.set_config(DbConstants.DB_BOOTSTRAP_HELPER,
                    Convert.ToUInt32(value));
            }
        }

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
            get {
                uint ret = 0;
                site.get_config(DbConstants.DB_LEGACY, ref ret);
                return (ret != 0) ? true : false;
            }
            set {
                site.set_config(DbConstants.DB_LEGACY,
                    Convert.ToUInt32(value));
            }
        }

        /// <summary>
        /// Whether it is local site.
        /// </summary>
        public bool LocalSite {
            get {
                uint ret = 0;
                site.get_config(DbConstants.DB_LOCAL_SITE, ref ret);
                return (ret != 0) ? true : false;
            }
            set {
                site.set_config(DbConstants.DB_LOCAL_SITE,
                    Convert.ToUInt32(value));
            }
        }

        /// <summary>
        /// Whether the site is peer to local site. 
        /// </summary>
        /// <remarks>
        /// A peer site may be used as a target for "client-to-client" 
        /// synchronization messages. It only makes sense to specify this for a
        /// remote site. 
        /// </remarks>
        public bool Peer {
            get {
                uint ret = 0;
                site.get_config(DbConstants.DB_REPMGR_PEER, ref ret);
                return (ret != 0) ? true : false;
            }
            set {
                site.set_config(DbConstants.DB_REPMGR_PEER,
                    Convert.ToUInt32(value));
            }
        }

        /// <summary>
        /// The eid of the site.
        /// </summary>
        public int EId {
            get {
                int eid = 0;
                site.get_eid(ref eid);
                return eid;
            }
        }

        /// <summary>
        /// Close the site.
        /// </summary>
        /// <remarks>
        /// All open DbSite must be closed before its owning DatabaseEnvironment
        /// is closed. 
        /// </remarks>
        public void Close() {
            isOpen = false;
            site.close();
        }

        /// <summary>
        /// Release the resources held by this object, and close the site if
        /// it's still open.
        /// </summary>
        public void Dispose() {
            if (isOpen)
                site.close();
            site.Dispose();
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Remove the site from the replication manager.
        /// </summary>
        public void Remove() {
            isOpen = false;
            site.remove();
        }

        internal DbSite(DB_SITE site) {
            this.site = site;
            isOpen = true;
        }
    }
}
