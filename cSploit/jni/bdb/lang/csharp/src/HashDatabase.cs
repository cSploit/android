/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing a HashDatabase. The Hash format is an extensible,
    /// dynamic hashing scheme.
    /// </summary>
    public class HashDatabase : Database {
        private HashFunctionDelegate hashHandler;
        private EntryComparisonDelegate compareHandler;
        private EntryComparisonDelegate dupCompareHandler;
        private PartitionDelegate partitionHandler;
        private BDB_CompareDelegate doCompareRef;
        private BDB_HashDelegate doHashRef;
        private BDB_CompareDelegate doDupCompareRef;
        private BDB_PartitionDelegate doPartitionRef;
        private DatabaseEntry[] partitionKeys;
        private uint nparts;
        
        #region Constructors
        private HashDatabase(DatabaseEnvironment env, uint flags)
            : base(env, flags) { }
        internal HashDatabase(BaseDatabase clone) : base(clone) { }

        private void Config(HashDatabaseConfig cfg) {
            base.Config(cfg);
            /* 
             * Database.Config calls set_flags, but that doesn't get the Hash
             * specific flags.  No harm in calling it again.
             */
            db.set_flags(cfg.flags);

            if (cfg.BlobDir != null && cfg.Env == null)
                db.set_blob_dir(cfg.BlobDir);

            if (cfg.blobThresholdIsSet)
                db.set_blob_threshold(cfg.BlobThreshold, 0);
            
            if (cfg.HashFunction != null)
                HashFunction = cfg.HashFunction;
            // The duplicate comparison function cannot change.
            if (cfg.DuplicateCompare != null)
                DupCompare = cfg.DuplicateCompare;
            if (cfg.fillFactorIsSet)
                db.set_h_ffactor(cfg.FillFactor);
            if (cfg.nelemIsSet)
                db.set_h_nelem(cfg.TableSize);
            if (cfg.HashComparison != null)
                Compare = cfg.HashComparison;

            if (cfg.partitionIsSet) {
                nparts = cfg.NParts;
                Partition = cfg.Partition;
                if (Partition == null)
                    doPartitionRef = null;
                else
                    doPartitionRef = new BDB_PartitionDelegate(doPartition);
                partitionKeys = cfg.PartitionKeys;
                IntPtr[] ptrs = null;
                if (partitionKeys != null) {
                    int size = (int)nparts - 1;
                    ptrs = new IntPtr[size];
                    for (int i = 0; i < size; i++) {
                        ptrs[i] = DBT.getCPtr(
                            DatabaseEntry.getDBT(partitionKeys[i])).Handle;
                    }
                }
                db.set_partition(nparts, ptrs, doPartitionRef);
            }
        }

        /// <summary>
        /// Instantiate a new HashDatabase object and open the database
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
        public static HashDatabase Open(
            string Filename, HashDatabaseConfig cfg) {
            return Open(Filename, null, cfg, null);
        }
        /// <summary>
        /// Instantiate a new HashDatabase object and open the database
        /// represented by <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If both <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/> are null, the database is strictly
        /// temporary and cannot be opened by any other thread of control, thus
        /// the database can only be accessed by sharing the single database 
        /// object that created it, in circumstances where doing so is safe. If
        /// <paramref name="Filename"/> is null and
        /// <paramref name="DatabaseName"/> is non-null, the database can be
        /// opened by other threads of control and will be replicated to client
        /// sites in any replication group.
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
        /// <param name="DatabaseName">
        /// This parameter allows applications to have multiple databases in a
        /// single file. Although no DatabaseName needs to be specified, it is
        /// an error to attempt to open a second database in a file that was not
        /// initially created using a database name.
        /// </param>
        /// <param name="cfg">The database's configuration</param>
        /// <returns>A new, open database object</returns>
        public static HashDatabase Open(
            string Filename, string DatabaseName, HashDatabaseConfig cfg) {
            return Open(Filename, DatabaseName, cfg, null);
        }
        /// <summary>
        /// Instantiate a new HashDatabase object and open the database
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
        public static HashDatabase Open(
            string Filename, HashDatabaseConfig cfg, Transaction txn) {
            return Open(Filename, null, cfg, txn);
        }
        /// <summary>
        /// Instantiate a new HashDatabase object and open the database
        /// represented by <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/>.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If both <paramref name="Filename"/> and
        /// <paramref name="DatabaseName"/> are null, the database is strictly
        /// temporary and cannot be opened by any other thread of control, thus
        /// the database can only be accessed by sharing the single database 
        /// object that created it, in circumstances where doing so is safe. If
        /// <paramref name="Filename"/> is null and
        /// <paramref name="DatabaseName"/> is non-null, the database can be
        /// opened by other threads of control and will be replicated to client
        /// sites in any replication group.
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
        /// <param name="DatabaseName">
        /// This parameter allows applications to have multiple databases in a
        /// single file. Although no DatabaseName needs to be specified, it is
        /// an error to attempt to open a second database in a file that was not
        /// initially created using a database name.
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
        public static HashDatabase Open(string Filename,
            string DatabaseName, HashDatabaseConfig cfg, Transaction txn) {
            HashDatabase ret = new HashDatabase(cfg.Env, 0);
            ret.Config(cfg);
            ret.db.open(Transaction.getDB_TXN(txn),
                Filename, DatabaseName, DBTYPE.DB_HASH, cfg.openFlags, 0);
            ret.isOpen = true;
            return ret;
        }

        #endregion Constructors

        #region Callbacks
        private static int doDupCompare(
            IntPtr dbp, IntPtr dbt1p, IntPtr dbt2p, IntPtr locp) {
            DB db = new DB(dbp, false);
            DBT dbt1 = new DBT(dbt1p, false);
            DBT dbt2 = new DBT(dbt2p, false);
            if (locp != IntPtr.Zero)
                locp = IntPtr.Zero;
            return ((HashDatabase)(db.api_internal)).DupCompare(
                DatabaseEntry.fromDBT(dbt1), DatabaseEntry.fromDBT(dbt2));
        }
        private static uint doHash(IntPtr dbp, IntPtr datap, uint len) {
            DB db = new DB(dbp, false);
            byte[] t_data = new byte[len];
            Marshal.Copy(datap, t_data, 0, (int)len);

            return ((HashDatabase)(db.api_internal)).hashHandler(t_data);
        }
        private static int doCompare(IntPtr dbp,
            IntPtr dbtp1, IntPtr dbtp2, IntPtr locp) {
            DB db = new DB(dbp, false);
            DBT dbt1 = new DBT(dbtp1, false);
            DBT dbt2 = new DBT(dbtp2, false);
            if (locp != IntPtr.Zero)
                locp = IntPtr.Zero;

            return ((HashDatabase)(db.api_internal)).compareHandler(
                DatabaseEntry.fromDBT(dbt1), DatabaseEntry.fromDBT(dbt2));
        }
        private static uint doPartition(IntPtr dbp, IntPtr dbtp) {
            DB db = new DB(dbp, false);
            DatabaseEntry dbt = DatabaseEntry.fromDBT(new DBT(dbtp, false));
            HashDatabase btdb = (HashDatabase)(db.api_internal);
            return btdb.Partition(dbt);
        }

        #endregion Callbacks

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
        /// The Hash key comparison function. The comparison function is called
        /// whenever it is necessary to compare a key specified by the
        /// application with a key currently stored in the tree. 
        /// </summary>
        public EntryComparisonDelegate Compare {
            get { return compareHandler; }
            private set {
                if (value == null)
                    db.set_h_compare(null);
                else if (compareHandler == null) {
                    if (doCompareRef == null)
                        doCompareRef = new BDB_CompareDelegate(doCompare);
                    db.set_h_compare(doCompareRef);
                }
                compareHandler = value;
            }
        }
        /// <summary>
        /// The duplicate data item comparison function.
        /// </summary>
        public EntryComparisonDelegate DupCompare {
            get { return dupCompareHandler; }
            private set {
                /* Cannot be called after open. */
                if (value == null)
                    db.set_dup_compare(null);
                else if (dupCompareHandler == null) {
                    if (doDupCompareRef == null)
                        doDupCompareRef = new BDB_CompareDelegate(doDupCompare);
                    db.set_dup_compare(doDupCompareRef);
                }
                dupCompareHandler = value;
            }
        }
        /// <summary>
        /// Whether the insertion of duplicate data items in the database is
        /// permitted, and whether duplicates items are sorted.
        /// </summary>
        public DuplicatesPolicy Duplicates {
            get {
                uint flags = 0;
                db.get_flags(ref flags);
                if ((flags & DbConstants.DB_DUPSORT) != 0)
                    return DuplicatesPolicy.SORTED;
                else if ((flags & DbConstants.DB_DUP) != 0)
                    return DuplicatesPolicy.UNSORTED;
                else
                    return DuplicatesPolicy.NONE;
            }
        }
        /// <summary>
        /// The desired density within the hash table.
        /// </summary>
        public uint FillFactor {
            get {
                uint ret = 0;
                db.get_h_ffactor(ref ret);
                return ret;
            }
        }
        /// <summary>
        /// A user-defined hash function; if no hash function is specified, a
        /// default hash function is used. 
        /// </summary>
        public HashFunctionDelegate HashFunction {
            get { return hashHandler; }
            private set {
                if (value == null)
                    db.set_h_hash(null);
                else if (hashHandler == null) {
                    if (doHashRef == null)
                        doHashRef = new BDB_HashDelegate(doHash);
                    db.set_h_hash(doHashRef);
                }
                hashHandler = value;
            }
        }

        /// <summary>
        /// An estimate of the final size of the hash table.
        /// </summary>
        public uint TableSize {
            get {
                uint ret = 0;
                db.get_h_nelem(ref ret);
                return ret;
            }
        }

        #endregion Properties

        #region Methods
        /// <summary>
        /// Compact the database, and optionally return unused database pages to
        /// the underlying filesystem. 
        /// </summary>
        /// <remarks>
        /// If the operation occurs in a transactional database, the operation
        /// will be implicitly transaction protected using multiple
        /// transactions. These transactions will be periodically committed to
        /// avoid locking large sections of the tree. Any deadlocks encountered
        /// cause the compaction operation to be retried from the point of the
        /// last transaction commit.
        /// </remarks>
        /// <param name="cdata">Compact configuration parameters</param>
        /// <returns>
        /// Compact operation statistics, where <see cref="CompactData.End"/>
        /// holds the integer value representing which bucket the compaction
        /// stopped in.
        /// </returns>
        public CompactData Compact(CompactConfig cdata) {
            return Compact(cdata, null);
        }
        /// <summary>
        /// Compact the database, and optionally return unused database pages to
        /// the underlying filesystem. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <paramref name="txn"/> is non-null, then the operation is
        /// performed using that transaction. In this event, large sections of
        /// the tree may be locked during the course of the transaction.
        /// </para>
        /// <para>
        /// If <paramref name="txn"/> is null, but the operation occurs in a
        /// transactional database, the operation will be implicitly transaction
        /// protected using multiple transactions. These transactions will be
        /// periodically committed to avoid locking large sections of the tree.
        /// Any deadlocks encountered cause the compaction operation to be
        /// retried from the point of the last transaction commit.
        /// </para>
        /// </remarks>
        /// <param name="cdata">Compact configuration parameters</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        /// <returns>
        /// Compact operation statistics, where <see cref="CompactData.End"/>
        /// holds the integer value representing which bucket the compaction
        /// stopped in.
        /// </returns>
        public CompactData Compact(CompactConfig cdata, Transaction txn) {
            DatabaseEntry end = null;
            if (cdata.returnEnd)
                end = new DatabaseEntry();

            db.compact(Transaction.getDB_TXN(txn), cdata.start, cdata.stop,
                CompactConfig.getDB_COMPACT(cdata), cdata.flags, end);
            return new CompactData(CompactConfig.getDB_COMPACT(cdata), end);
        }

        /// <summary>
        /// Create a database cursor.
        /// </summary>
        /// <returns>A newly created cursor</returns>
        public new HashCursor Cursor() {
            return Cursor(new CursorConfig(), null);
        }
        /// <summary>
        /// Create a database cursor with the given configuration.
        /// </summary>
        /// <param name="cfg">
        /// The configuration properties for the cursor.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public new HashCursor Cursor(CursorConfig cfg) {
            return Cursor(cfg, null);
        }
        /// <summary>
        /// Create a transactionally protected database cursor.
        /// </summary>
        /// <param name="txn">
        /// The transaction context in which the cursor may be used.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public new HashCursor Cursor(Transaction txn) {
            return Cursor(new CursorConfig(), txn);
        }
        /// <summary>
        /// Create a transactionally protected database cursor with the given
        /// configuration.
        /// </summary>
        /// <param name="cfg">
        /// The configuration properties for the cursor.
        /// </param>
        /// <param name="txn">
        /// The transaction context in which the cursor may be used.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public new HashCursor Cursor(CursorConfig cfg, Transaction txn) {
            if (cfg.Priority == CachePriority.DEFAULT)
                return new HashCursor(
                    db.cursor(Transaction.getDB_TXN(txn), cfg.flags), Pagesize);
            else
                return new HashCursor(db.cursor(Transaction.getDB_TXN(txn),
                    cfg.flags), Pagesize, cfg.Priority);
        }

        /// <summary>
        /// Return the database statistical information which does not require
        /// traversal of the database.
        /// </summary>
        /// <returns>
        /// The database statistical information which does not require
        /// traversal of the database.
        /// </returns>
        public HashStats FastStats() {
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
        public HashStats FastStats(Transaction txn) {
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
        /// <see cref="QueueStats"/>, and <see cref="RecnoStats"/> classes. 
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
        public HashStats FastStats(Transaction txn, Isolation isoDegree) {
            return Stats(txn, true, isoDegree);
        }

        /// <summary>
        /// Return pages to the filesystem that are already free and at the end
        /// of the file.
        /// </summary>
        /// <returns>
        /// The number of database pages returned to the filesystem
        /// </returns>
        public uint TruncateUnusedPages() {
            return TruncateUnusedPages(null);
        }
        /// <summary>
        /// Return pages to the filesystem that are already free and at the end
        /// of the file.
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
        /// The number of database pages returned to the filesystem
        /// </returns>
        public uint TruncateUnusedPages(Transaction txn) {
            DB_COMPACT cdata = new DB_COMPACT();
            db.compact(Transaction.getDB_TXN(txn),
                null, null, cdata, DbConstants.DB_FREELIST_ONLY, null);
            return cdata.compact_pages_truncated;
        }

        /// <summary>
        /// Return the number of partitions created in the database.
        /// </summary>
        public uint NParts {
            get {
                db.get_partition_parts(ref nparts);
                return nparts;
            }
            private set { nparts = value; }
        }

        /// <summary>
        /// Return the application-specified partitioning function.
        /// </summary>
        public PartitionDelegate Partition {
            get { return partitionHandler; }
            private set { partitionHandler = value; }
        }

        /// <summary>
        /// Return an array of type DatabaseEntry where each array entry
        /// contains the range of keys contained in one of the database's
        /// partitions. The array contains the information for the entire
        /// database.
        /// </summary>
        public DatabaseEntry[] PartitionKeys {
            get {
                partitionKeys = db.get_partition_keys();
                return partitionKeys;
            }
            private set { partitionKeys = value; }
        }

        /// <summary>
        /// Store the key/data pair in the database only if it does not already
        /// appear in the database. 
        /// </summary>
        /// <param name="key">The key to store in the database</param>
        /// <param name="data">The data item to store in the database</param>
        public void PutNoDuplicate(DatabaseEntry key, DatabaseEntry data) {
            PutNoDuplicate(key, data, null);
        }
        /// <summary>
        /// Store the key/data pair in the database only if it does not already
        /// appear in the database. 
        /// </summary>
        /// <param name="key">The key to store in the database</param>
        /// <param name="data">The data item to store in the database</param>
        /// <param name="txn">
        /// If the operation is part of an application-specified transaction,
        /// <paramref name="txn"/> is a Transaction object returned from
        /// <see cref="DatabaseEnvironment.BeginTransaction"/>; if
        /// the operation is part of a Berkeley DB Concurrent Data Store group,
        /// <paramref name="txn"/> is a handle returned from
        /// <see cref="DatabaseEnvironment.BeginCDSGroup"/>; otherwise null.
        /// </param>
        public void PutNoDuplicate(
            DatabaseEntry key, DatabaseEntry data, Transaction txn) {
            Put(key, data, txn, DbConstants.DB_NODUPDATA);
        }

        /// <summary>
        /// Return the database statistical information for this database.
        /// </summary>
        /// <returns>Database statistical information.</returns>
        public HashStats Stats() {
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
        public HashStats Stats(Transaction txn) {
            return Stats(txn, false, Isolation.DEGREE_THREE);
        }
        /// <summary>
        /// Return the database statistical information for this database.
        /// </summary>
        /// <overloads>
        /// The statistical information is described by
        /// <see cref="BTreeStats"/>. 
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
        public HashStats Stats(Transaction txn, Isolation isoDegree) {
            return Stats(txn, false, isoDegree);
        }
        private HashStats Stats(
            Transaction txn, bool fast, Isolation isoDegree) {
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
            HashStatStruct st = db.stat_hash(Transaction.getDB_TXN(txn), flags);
            return new HashStats(st);
        }
        #endregion Methods
    }
}
