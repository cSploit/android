/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2013, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing configuration parameters for
    /// <see cref="DatabaseStream"/>
    /// </summary>
    public class DatabaseStreamConfig {
        internal bool readOnlyIsSet;
        private bool readOnly;
        /// <summary>
        /// The database stream is read only.
        /// </summary>
        public bool ReadOnly {
            get {
                return readOnly;
            }
            set {
                readOnlyIsSet = true;
                readOnly = value;
            }
        }

        /// <summary>
        /// True if the database stream syncs the blob on each write.
        /// </summary>
        public bool SyncPerWrite;

        /// <summary>
        /// Instantiate a new DatabaseStreamConfig object.
        /// </summary>
        public DatabaseStreamConfig() {
            readOnly = false;
            SyncPerWrite = false;
        }

        internal uint flags {
            get {
                uint ret = 0;
                if (readOnlyIsSet)
                    ret |= readOnly ? DbConstants.DB_STREAM_READ :
                        DbConstants.DB_STREAM_WRITE;
                if (SyncPerWrite)
                    ret |= DbConstants.DB_STREAM_SYNC_WRITE;
                return ret;
            }
        }
    }
}
