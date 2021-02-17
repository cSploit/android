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
    /// A class representing a HeapDatabase.
    /// </summary>
    public class HeapDatabase : Database {

        #region Constructors
        private HeapDatabase(DatabaseEnvironment env, uint flags)
            : base(env, flags) { }
        internal HeapDatabase(BaseDatabase clone) : base(clone) { }

        private void Config(HeapDatabaseConfig cfg) {
            base.Config(cfg);
            /* 
             * Database.Config calls set_flags, but that doesn't get the Heap
             * specific flags.  No harm in calling it again.
             */
            db.set_flags(cfg.flags);

            if (cfg.BlobDir != null && cfg.Env == null)
                db.set_blob_dir(cfg.BlobDir);

            if (cfg.blobThresholdIsSet)
                db.set_blob_threshold(cfg.BlobThreshold, 0);

            if (cfg.maxSizeIsSet)
                db.set_heapsize(cfg.MaxSizeGBytes, cfg.MaxSizeBytes);
            if (cfg.regionszIsSet)
                db.set_heap_regionsize(cfg.RegionSize);
        }

        /// <summary>
        /// Instantiate a new HeapDatabase object and open the database
        /// represented by <paramref name="Filename"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <paramref name="Filename"/> is null, the database is strictly
        /// temporary and cannot be opened by any other thread of control, thus
        /// the database can only be accessed by sharing the single database
        /// object that created it, in circumstances where doing so is safe.
        /// </para>
        /// <para>
        /// If <see cref="DatabaseConfig.AutoCommit"/> is set, the operation
        /// will be implicitly transaction protected. Note that transactionally
        /// protected operations on a datbase object requires the object itself
        /// be transactionally protected during its open.
        /// </para>
        /// </remarks>
        /// <param name="Filename">
        /// The name of an underlying file that will be used to back the
        /// database. In-memory databases never intended to be preserved on disk
        /// may be created by setting this parameter to null.
        /// </param>
        /// <param name="cfg">The database's configuration</param>
        /// <returns>A new, open database object</returns>
        public static HeapDatabase Open(
            string Filename, HeapDatabaseConfig cfg) {
            return Open(Filename, cfg, null);
        }
        /// <summary>
        /// Instantiate a new HeapDatabase object and open the database
        /// represented by <paramref name="Filename"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <paramref name="Filename"/> is null, the database is strictly
        /// temporary and cannot be opened by any other thread of control, thus
        /// the database can only be accessed by sharing the single database
        /// object that created it, in circumstances where doing so is safe.
        /// </para>
        /// <para>
        /// If <paramref name="txn"/> is null, but
        /// <see cref="DatabaseConfig.AutoCommit"/> is set, the operation will
        /// be implicitly transaction protected. Note that transactionally
        /// protected operations on a datbase object requires the object itself
        /// be transactionally protected during its open. Also note that the
        /// transaction must be committed before the object is closed.
        /// </para>
        /// </remarks>
        /// <param name="Filename">
        /// The name of an underlying file that will be used to back the
        /// database. In-memory databases never intended to be preserved on disk
        /// may be created by setting this parameter to null.
        /// </param>
        /// <param name="cfg">The database's configuration</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>A new, open database object</returns>
        public static HeapDatabase Open(
            string Filename, HeapDatabaseConfig cfg, Transaction txn) {
            HeapDatabase ret = new HeapDatabase(cfg.Env, 0);
            ret.Config(cfg);
            ret.db.open(Transaction.getDB_TXN(txn),
                Filename, null, DBTYPE.DB_HEAP, cfg.openFlags, 0);
            ret.isOpen = true;
            return ret;
        }

        #endregion Constructors

        #region Properties
        /// <summary>
        /// The path of the directory where blobs are stored.
        /// </summary>
        public string BlobDir {
            get {
                string dir;
                db.get_blob_dir(out dir);
                return dir;
            }
        }

        internal string BlobSubDir {
            get {
                string dir;
                db.get_blob_sub_dir(out dir);
                return dir;
            }
        }

        /// <summary>
        /// The threshold value in bytes beyond which data items are stored as
        /// blobs.
        /// <para>
        /// Any data item that is equal to or larger in size than the
        /// threshold value will automatically be stored as a blob.
        /// </para>
        /// <para>
        /// A value of 0 indicates that blobs are not used by the database.
        /// </para>
        /// </summary>
        public uint BlobThreshold {
            get {
                uint ret = 0;
                db.get_blob_threshold(ref ret);
                return ret;
            }
        }

        /// <summary>
        /// The gigabytes component of the maximum on-disk database file size.
        /// </summary>
        public uint MaxSizeGBytes {
            get {
                uint gbytes = 0;
                uint bytes = 0;
                db.get_heapsize(ref gbytes, ref bytes);
                return gbytes;
            }
        }
        /// <summary>
        /// The bytes component of the maximum on-disk database file size.
        /// </summary>
        public uint MaxSizeBytes {
            get {
                uint gbytes = 0;
                uint bytes = 0;
                db.get_heapsize(ref gbytes, ref bytes);
                return bytes;
            }
        }
        /// <summary>
        /// The number of pages in a region of the database. It is set by 
        /// <see cref="HeapDatabaseConfig.RegionSize"/> when the heap database
        /// is opened.
        /// </summary>
        public uint RegionSize {
            get {
                 uint ret = 0;
                 db.get_heap_regionsize(ref ret);
                 return ret;
            }
        }
        #endregion Properties

        #region Methods
        /// <summary>
        /// Append the data item to the end of the database.
        /// </summary>
        /// <param name="data">The data item to store in the database</param>
        /// <returns>The heap record id allocated to the record</returns>
        public HeapRecordId Append(DatabaseEntry data) {
            return Append(data, null);
        }
        /// <summary>
        /// Append the data item to the end of the database.
        /// </summary>
        /// <param name="data">The data item to store in the database</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>The heap record id allocated to the record</returns>
        public HeapRecordId Append(DatabaseEntry data, Transaction txn) {
            DatabaseEntry key = new DatabaseEntry();
            Put(key, data, txn, DbConstants.DB_APPEND);
            return HeapRecordId.fromArray(key.Data);
        }

        /// <summary>
        /// Return the database statistical information which does not require
        /// traversal of the database.
        /// </summary>
        /// <returns>
        /// The database statistical information which does not require
        /// traversal of the database.
        /// </returns>
        public HeapStats FastStats() {
            return Stats(null, true, Isolation.DEGREE_THREE);
        }
        /// <summary>
        /// Return the database statistical information which does not require
        /// traversal of the database.
        /// </summary>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>
        /// The database statistical information which does not require
        /// traversal of the database.
        /// </returns>
        public HeapStats FastStats(Transaction txn) {
            return Stats(txn, true, Isolation.DEGREE_THREE);
        }
        /// <summary>
        /// Return the database statistical information which does not require
        /// traversal of the database.
        /// </summary>
        /// <overloads>
        /// <para>
        /// Among other things, this method makes it possible for applications
        /// to request key and record counts without incurring the performance
        /// penalty of traversing the entire database. 
        /// </para>
        /// <para>
        /// The statistical information is described by the
        /// <see cref="BTreeStats"/>, <see cref="HashStats"/>,
        /// <see cref="HeapStats"/>, <see cref="QueueStats"/>, and 
        /// <see cref="RecnoStats"/> classes. 
        /// </para>
        /// </overloads>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <param name="isoDegree">
        /// The level of isolation for database reads.
        /// <see cref="Isolation.DEGREE_ONE"/> will be silently ignored for 
        /// databases which did not specify
        /// <see cref="DatabaseConfig.ReadUncommitted"/>.
        /// </param>
        /// <returns>
        /// The database statistical information which does not require
        /// traversal of the database.
        /// </returns>
        public HeapStats FastStats(Transaction txn, Isolation isoDegree) {
            return Stats(txn, true, isoDegree);
        }

        /// <summary>
        /// Return the database statistical information for this database.
        /// </summary>
        /// <returns>Database statistical information.</returns>
        public HeapStats Stats() {
            return Stats(null, false, Isolation.DEGREE_THREE);
        }
        /// <summary>
        /// Return the database statistical information for this database.
        /// </summary>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>Database statistical information.</returns>
        public HeapStats Stats(Transaction txn) {
            return Stats(txn, false, Isolation.DEGREE_THREE);
        }
        /// <summary>
        /// Return the database statistical information for this database.
        /// </summary>
        /// <overloads>
        /// The statistical information is described by
        /// <see cref="HeapStats"/>. 
        /// </overloads>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <param name="isoDegree">
        /// The level of isolation for database reads.
        /// <see cref="Isolation.DEGREE_ONE"/> will be silently ignored for 
        /// databases which did not specify
        /// <see cref="DatabaseConfig.ReadUncommitted"/>.
        /// </param>
        /// <returns>Database statistical information.</returns>
        public HeapStats Stats(Transaction txn, Isolation isoDegree) {
            return Stats(txn, false, isoDegree);
        }
        private HeapStats Stats(Transaction txn, bool fast, Isolation isoDegree) {
            uint flags = 0;
            flags |= fast ? DbConstants.DB_FAST_STAT : 0;
            switch (isoDegree) {
                case Isolation.DEGREE_ONE:
                    flags |= DbConstants.DB_READ_UNCOMMITTED;
                    break;
                case Isolation.DEGREE_TWO:
                    flags |= DbConstants.DB_READ_COMMITTED;
                    break;
            }
            HeapStatStruct st = db.stat_heap(Transaction.getDB_TXN(txn), flags);
            return new HeapStats(st);
        }
        #endregion Methods
    }
}
