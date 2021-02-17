/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing configuration parameters for
    /// <see cref="QueueDatabase"/>
    /// </summary>
    public class QueueDatabaseConfig : DatabaseConfig {
        /* Fields for DB->set_flags() */
        /// <summary>
        /// If true, modify the operation of <see cref="QueueDatabase.Consume"/>
        /// to return key/data pairs in order. That is, they will always return
        /// the key/data item from the head of the queue. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// The default behavior of queue databases is optimized for multiple
        /// readers, and does not guarantee that record will be retrieved in the
        /// order they are added to the queue. Specifically, if a writing thread
        /// adds multiple records to an empty queue, reading threads may skip
        /// some of the initial records when the next
        /// <see cref="QueueDatabase.Consume"/> call returns.
        /// </para>
        /// <para>
        /// This setting modifies <see cref="QueueDatabase.Consume"/> to verify
        /// that the record being returned is in fact the head of the queue.
        /// This will increase contention and reduce concurrency when there are
        /// many reading threads.
        /// </para>
        /// </remarks>
        public bool ConsumeInOrder;
        internal new uint flags {
            get {
                uint ret = base.flags;
                ret |= ConsumeInOrder ? Internal.DbConstants.DB_INORDER : 0;
                return ret;
            }
        }

        /// <summary>
        /// The policy for how to handle database creation.
        /// </summary>
        /// <remarks>
        /// If the database does not already exist and
        /// <see cref="CreatePolicy.NEVER"/> is set,
        /// <see cref="QueueDatabase.Open"/> will fail.
        /// </remarks>
        public CreatePolicy Creation;
        internal new uint openFlags {
            get {
                uint flags = base.openFlags;
                flags |= (uint)Creation;
                return flags;
            }
        }

        /// <summary>
        /// A function to call after the record number has been selected but
        /// before the data has been stored into the database.
        /// </summary>
        /// <remarks>
        /// <para>
        /// When using <see cref="QueueDatabase.Append"/>, it may be useful to
        /// modify the stored data based on the generated key. If a delegate is
        /// specified, it will be called after the record number has been
        /// selected, but before the data has been stored.
        /// </para>
        /// </remarks>
        public AppendRecordDelegate Append;

        private uint len;
        /// <summary>
        /// Specify the length of records in the database.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The record length must be enough smaller than
        /// <see cref="DatabaseConfig.PageSize"/> that at least one record plus
        /// the database page's metadata information can fit on each database
        /// page.
        /// </para>
        /// <para>
        /// Any records added to the database that are less than Length bytes
        /// long are automatically padded (see <see cref="PadByte"/> for more
        /// information).
        /// </para>
        /// <para>
        /// Any attempt to insert records into the database that are greater
        /// than Length bytes long will cause the call to fail immediately and
        /// return an error. 
        /// </para>
        /// <para>
        /// If the database already exists, this setting will be ignored.
        /// </para>
        /// </remarks>
        public uint Length {
            get { return len; }
            set { len = value; }
        }

        internal bool padIsSet;
        private int pad;
        /// <summary>
        /// The padding character for short, fixed-length records.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If no pad character is specified, space characters (that is, ASCII
        /// 0x20) are used for padding.
        /// </para>
        /// <para>
        /// If the database already exists, this setting will be ignored.
        /// </para>
        /// </remarks>
        public int PadByte {
            get { return pad; }
            set {
                padIsSet = true;
                pad = value;
            }
        }

        internal bool extentIsSet;
        private uint extentSz;
        /// <summary>
        /// The size of the extents used to hold pages in a
        /// <see cref="QueueDatabase"/>, specified as a number of pages. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// Each extent is created as a separate physical file. If no extent
        /// size is set, the default behavior is to create only a single
        /// underlying database file.
        /// </para>
        /// <para>
        /// For information on tuning the extent size, see Selecting a extent
        /// size in the Programmer's Reference Guide.
        /// </para>
        /// <para>
        /// If the database already exists, this setting will be ignored.
        /// </para>
        /// </remarks>
        public uint ExtentSize {
            get { return extentSz; }
            set {
                extentIsSet = true;
                extentSz = value;
            }
        }

        /// <summary>
        /// Instantiate a new QueueDatabaseConfig object
        /// </summary>
        public QueueDatabaseConfig() {
            ConsumeInOrder = false;
            Append = null;
            padIsSet = false;
            extentIsSet = false;
            Creation = CreatePolicy.NEVER;
        }
    }
}