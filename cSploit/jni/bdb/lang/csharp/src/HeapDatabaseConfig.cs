/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing configuration parameters for
    /// <see cref="HeapDatabase"/>
    /// </summary>
    public class HeapDatabaseConfig : DatabaseConfig {
        /// <summary>
        /// The path of the directory where blobs are stored.
        /// <para>
        /// If the database is opened within <see cref="DatabaseEnvironment"/>,
        /// this path setting will be ignored during
        /// <see cref="HeapDatabase.Open"/>. Use
        /// <see cref="HeapDatabase.BlobDir"/> to identify the current storage
        /// location of blobs after opening the database.
        /// </para>
        /// </summary>
        public string BlobDir;

        internal bool blobThresholdIsSet;
        private uint blobThreshold;
        /// <summary>
        /// The size in bytes which is used to determine when a data item will
        /// be stored as a blob.
        /// <para>
        /// Any data item that is equal to or larger in size than the
        /// threshold value will automatically be stored as a blob.
        /// </para>
        /// <para>
        /// If the threshold value is 0, blobs will never be used by the
        /// database.
        /// </para>
        /// <para>
        /// It is illegal to enable blob in the database which is configured
        /// as in-memory database or with chksum, encryption, duplicates,
        /// sorted duplicates, compression, multiversion concurrency control
        /// and transactional read operations with degree 1 isolation.
        /// </para>
        /// </summary>
        public uint BlobThreshold {
            get { return blobThreshold; }
            set {
                blobThresholdIsSet = true;
                blobThreshold = value;
            }
        }

        /// <summary>
        /// The policy for how to handle database creation.
        /// </summary>
        /// <remarks>
        /// If the database does not already exist and
        /// <see cref="CreatePolicy.NEVER"/> is set,
        /// <see cref="HeapDatabase.Open"/> will fail.
        /// </remarks>
        public CreatePolicy Creation;
        internal new uint openFlags {
            get {
                uint flags = base.openFlags;
                flags |= (uint)Creation;
                return flags;
            }
        }

        private uint _gbytes;
        private uint _bytes;
        internal bool maxSizeIsSet;
        /// <summary>
        /// The gigabytes component of the maximum on-disk database file size.
        /// </summary>
        public uint MaxSizeGBytes { get { return _gbytes; } }
        /// <summary>
        /// The bytes component of the maximum on-disk database file size.
        /// </summary>
        public uint MaxSizeBytes { get { return _bytes; } }
        /// <summary>
        /// Set the maximum on-disk database file size used by the database. If
        /// this value is never set, the database's file size can grow without
        /// bound. If this value is set, then the heap file can never grow
        /// larger than the limit defined by this method. In that case, attempts
        /// to update or create records in a database that has reached its
        /// maximum size will throw a <see cref="HeapFullException"/>.
        /// </summary>
        ///<remarks>
        /// The size specified to this method must be at least three times the
        /// database page size. That is, the database must contain at least 
        /// three database pages. You can set the database page size using 
        /// <see cref="DatabaseConfig.PageSize"/>.
        ///
        /// If this value is set for an existing database, the size specified
        /// here must match the size used to create the database. Note, however,
        /// that specifying an incorrect size will not result in an error 
        /// until the database is opened. 
        /// </remarks>
        /// <param name="GBytes">The size of the database is set to GBytes
        /// gigabytes plus Bytes.</param>
        /// <param name="Bytes">The size of the database is set to GBytes
        /// gigabytes plus Bytes.</param>
        public void MaxSize(uint GBytes, uint Bytes) {
            maxSizeIsSet = true;
            _gbytes = GBytes;
            _bytes = Bytes;
        }

        private uint _regionsz;
        internal bool regionszIsSet;
        /// <summary>
        /// The number of pages in a region of the database. If this value is
        /// never set, the default region size for the database's page size will
        /// be used. You can set the database page size using 
        /// <see cref="DatabaseConfig.PageSize"/>.
        ///
        /// If the database already exists, this value will be ignored. If the
        /// specified region size is larger than the maximum region size for the
        /// database's page size, an error will be returned when
        /// <see cref="Database.Open"/> is called. The maximum allowable region 
        /// size will be included in the error message. 
        /// </summary>
        public uint RegionSize {
            get { return _regionsz; }
            set { 
                _regionsz = value;
                regionszIsSet = true;
            }
        }
        /// <summary>
        /// Instantiate a new HeapDatabaseConfig object
        /// </summary>
        public HeapDatabaseConfig() {
            blobThresholdIsSet = false;
            Creation = CreatePolicy.NEVER;
        }
    }
}
