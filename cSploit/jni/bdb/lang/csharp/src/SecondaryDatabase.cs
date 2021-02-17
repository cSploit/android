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
    /// A class representing a secondary Berkeley DB database, a base class for
    /// access method specific classes.
    /// </summary>
    public class SecondaryDatabase : BaseDatabase {
        private SecondaryKeyGenDelegate keyGenHandler;
        internal BDB_AssociateDelegate doAssocRef;
        private ForeignKeyNullifyDelegate nullifierHandler;
        internal BDB_AssociateForeignDelegate doNullifyRef;

        #region Constructors
        /// <summary>
        /// Protected construtor
        /// </summary>
        /// <param name="env">The environment in which to open the DB</param>
        /// <param name="flags">Flags to pass to DB->create</param>
        protected SecondaryDatabase(DatabaseEnvironment env, uint flags)
            : base(env, flags) { }

        internal static SecondaryDatabase fromDB(DB dbp) {
            try {
                return (SecondaryDatabase)dbp.api_internal;
            } catch { }
            return null;
        }

        /// <summary>
        /// Protected method to configure the DB.  Only valid before DB->open.
        /// </summary>
        /// <param name="cfg">Configuration parameters.</param>
        protected void Config(SecondaryDatabaseConfig cfg) {
            base.Config(cfg);
            KeyGen = cfg.KeyGen;
            Nullifier = cfg.ForeignKeyNullfier;

            db.set_flags(cfg.flags);
        }

        /// <summary>
        /// Instantiate a new SecondaryDatabase object, open the database
        /// represented by <paramref name="Filename"/> and associate the
        /// database with the <see cref="SecondaryDatabaseConfig.Primary">
        /// primary index</see>. The file specified by
        /// <paramref name="Filename"/> must exist.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <see cref="DatabaseConfig.AutoCommit"/> is set, the operation
        /// will be implicitly transaction protected. Note that transactionally
        /// protected operations on a datbase object requires the object itself
        /// be transactionally protected during its open.
        /// </para>
        /// </remarks>
        /// <param name="Filename">
        /// The name of an underlying file that will be used to back the
        /// database.
        /// </param>
        /// <param name="cfg">The database's configuration</param>
        /// <returns>A new, open database object</returns>
        public static SecondaryDatabase Open(
            string Filename, SecondaryDatabaseConfig cfg) {
            return Open(Filename, null, cfg, null);
        }
        /// <summary>
        /// Instantiate a new SecondaryDatabase object, open the database
        /// represented by <paramref name="Filename"/> and associate the
        /// database with the <see cref="SecondaryDatabaseConfig.Primary">
        /// primary index</see>. The file specified by
        /// <paramref name="Filename"/> must exist.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <paramref name="Filename"/> is null and 
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
        /// database.
        /// </param>
        /// <param name="DatabaseName">
        /// This parameter allows applications to have multiple databases in a
        /// single file. Although no DatabaseName needs to be specified, it is
        /// an error to attempt to open a second database in a file that was not
        /// initially created using a database name.
        /// </param>
        /// <param name="cfg">The database's configuration</param>
        /// <returns>A new, open database object</returns>
        public static SecondaryDatabase Open(string Filename,
            string DatabaseName, SecondaryDatabaseConfig cfg) {
            return Open(Filename, DatabaseName, cfg, null);
        }
        /// <summary>
        /// Instantiate a new SecondaryDatabase object, open the database
        /// represented by <paramref name="Filename"/> and associate the
        /// database with the <see cref="SecondaryDatabaseConfig.Primary">
        /// primary index</see>. The file specified by
        /// <paramref name="Filename"/> must exist.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <see cref="DatabaseConfig.AutoCommit"/> is set, the operation
        /// will be implicitly transaction protected. Note that transactionally
        /// protected operations on a datbase object requires the object itself
        /// be transactionally protected during its open.
        /// </para>
        /// </remarks>
        /// <param name="Filename">
        /// The name of an underlying file that will be used to back the
        /// database.
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
        public static SecondaryDatabase Open(string Filename,
            SecondaryDatabaseConfig cfg, Transaction txn) {
            return Open(Filename, null, cfg, txn);
        }
        /// <summary>
        /// Instantiate a new SecondaryDatabase object, open the database
        /// represented by <paramref name="Filename"/> and associate the
        /// database with the <see cref="SecondaryDatabaseConfig.Primary">
        /// primary index</see>. The file specified by
        /// <paramref name="Filename"/> must exist.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If <paramref name="Filename"/> is null and 
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
        /// database.
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
        public static SecondaryDatabase Open(string Filename,
            string DatabaseName, SecondaryDatabaseConfig cfg, Transaction txn) {
            if (cfg.DbType == DatabaseType.BTREE) {
                return SecondaryBTreeDatabase.Open(Filename,
                    DatabaseName, (SecondaryBTreeDatabaseConfig)cfg, txn);
            } else if (cfg.DbType == DatabaseType.HASH) {
               return SecondaryHashDatabase.Open(Filename,
                   DatabaseName, (SecondaryHashDatabaseConfig)cfg, txn);
            }

            SecondaryDatabase ret = new SecondaryDatabase(cfg.Env, 0);
            ret.Config(cfg);
            ret.db.open(Transaction.getDB_TXN(txn), Filename,
                DatabaseName, cfg.DbType.getDBTYPE(), cfg.openFlags, 0);
            ret.doAssocRef = new BDB_AssociateDelegate(doAssociate);
            cfg.Primary.db.associate(Transaction.getDB_TXN(txn),
                ret.db, ret.doAssocRef, cfg.assocFlags);

            if (cfg.ForeignKeyDatabase != null) {
                if (cfg.OnForeignKeyDelete == ForeignKeyDeleteAction.NULLIFY)
                    ret.doNullifyRef =
                        new BDB_AssociateForeignDelegate(doNullify);
                else
                    ret.doNullifyRef = null;
                cfg.ForeignKeyDatabase.db.associate_foreign(ret.db,
                    ret.doNullifyRef, cfg.foreignFlags);
            }
            return ret;
        }

        #endregion Constructors

        #region Callbacks
        /// <summary>
        /// Protected method to call the key generation function.
        /// </summary>
        /// <param name="dbp">Secondary DB Handle</param>
        /// <param name="keyp">Primary Key</param>
        /// <param name="datap">Primary Data</param>
        /// <param name="skeyp">Scondary Key</param>
        /// <returns>0 on success, !0 on failure</returns>
        protected static int doAssociate(
            IntPtr dbp, IntPtr keyp, IntPtr datap, IntPtr skeyp) {
            DB db = new DB(dbp, false);
            DBT key = new DBT(keyp, false);
            DBT data = new DBT(datap, false);
            DBT skey = new DBT(skeyp, false);
            IntPtr dataPtr, sdataPtr;
            int nrecs, dbt_sz;

            DatabaseEntry s =
                ((SecondaryDatabase)db.api_internal).KeyGen(
                DatabaseEntry.fromDBT(key), DatabaseEntry.fromDBT(data));

            if (s == null)
                return DbConstants.DB_DONOTINDEX;
            if (s is MultipleDatabaseEntry) {
                MultipleDatabaseEntry mde = (MultipleDatabaseEntry)s;
                nrecs = mde.nRecs;
                /* 
                 * Allocate an array of nrecs DBT in native memory.  The call 
                 * returns sizeof(DBT), so that we know where one DBT ends and 
                 * the next begins.
                 */
                dbt_sz = (int)libdb_csharp.alloc_dbt_arr(null, nrecs, out sdataPtr);
                /* 
                 * We need a managed array to copy each DBT into and then we'll
                 * copy the managed array to the native array we just allocated.
                 * We're not able to copy native -> native.
                 */
                byte[] arr = new byte[nrecs * dbt_sz];
                IntPtr p;
                int off = 0;
                /* Copy each DBT into the array. */
                foreach (DatabaseEntry dbt in mde) {
                    /* Allocate room for the data in native memory. */
                    dataPtr = libdb_csharp.__os_umalloc(null, dbt.size);
                    Marshal.Copy(dbt.Data, 0, dataPtr, (int)dbt.size);
                    dbt.dbt.dataPtr = dataPtr;
                    dbt.flags |= DbConstants.DB_DBT_APPMALLOC;

                    p = DBT.getCPtr(DatabaseEntry.getDBT(dbt)).Handle;
                    Marshal.Copy(p, arr, off, dbt_sz);
                    off += dbt_sz;
                }
                Marshal.Copy(arr, 0, sdataPtr, nrecs * dbt_sz);
                skey.dataPtr = sdataPtr;
                skey.size = (uint)mde.nRecs;
                skey.flags = DbConstants.DB_DBT_MULTIPLE | DbConstants.DB_DBT_APPMALLOC;
            } else
                skey.data = s.Data;
            return 0;
        }

        /// <summary>
        /// Protected method to nullify a foreign key
        /// </summary>
        /// <param name="dbp">Secondary DB Handle</param>
        /// <param name="keyp">Primary Key</param>
        /// <param name="datap">Primary Data</param>
        /// <param name="fkeyp">Foreign Key</param>
        /// <param name="changed">Whether the foreign key has changed</param>
        /// <returns>0 on success, !0 on failure</returns>
        protected static int doNullify(IntPtr dbp,
            IntPtr keyp, IntPtr datap, IntPtr fkeyp, ref int changed) {
            DB db = new DB(dbp, false);
            DBT key = new DBT(keyp, false);
            DBT data = new DBT(datap, false);
            DBT fkey = new DBT(fkeyp, false);

            DatabaseEntry d = ((SecondaryDatabase)db.api_internal).Nullifier(
                DatabaseEntry.fromDBT(key),
                DatabaseEntry.fromDBT(data), DatabaseEntry.fromDBT(fkey));

            if (d == null)
                changed = 0;
            else {
                changed = 1;
                data.data = d.Data;
            }

            return 0;
        }

        #endregion Callbacks

        #region Properties
        /// <summary>
        /// The delegate that creates the set of secondary keys corresponding to
        /// a given primary key and data pair. 
        /// </summary>
        public SecondaryKeyGenDelegate KeyGen {
            get { return keyGenHandler; }
            private set {
                keyGenHandler = value;
            }
        }

        /// <summary>
        /// The nullifying function used to set the foreign key to null.
        /// </summary>
        public ForeignKeyNullifyDelegate Nullifier {
            get { return nullifierHandler; }
            private set { nullifierHandler = value; }
        }
        
        #endregion Properties

        #region Methods
        /// <summary>
        /// Create a secondary database cursor.
        /// </summary>
        /// <returns>A newly created cursor</returns>
        public SecondaryCursor SecondaryCursor() { 
            return SecondaryCursor(new CursorConfig(), null); 
        }
        /// <summary>
        /// Create a secondary database cursor with the given configuration.
        /// </summary>
        /// <param name="cfg">
        /// The configuration properties for the cursor.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public SecondaryCursor SecondaryCursor(CursorConfig cfg) {
            return SecondaryCursor(cfg, null);
        }
        /// <summary>
        /// Create a transactionally protected secondary database cursor.
        /// </summary>
        /// <param name="txn">
        /// The transaction context in which the cursor may be used.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public SecondaryCursor SecondaryCursor(Transaction txn) {
            return SecondaryCursor(new CursorConfig(), txn);
        }
        /// <summary>
        /// Create a transactionally protected secondary database cursor with
        /// the given configuration.
        /// </summary>
        /// <param name="cfg">
        /// The configuration properties for the cursor.
        /// </param>
        /// <param name="txn">
        /// The transaction context in which the cursor may be used.
        /// </param>
        /// <returns>A newly created cursor</returns>
        public SecondaryCursor SecondaryCursor(
            CursorConfig cfg, Transaction txn) {
            return new SecondaryCursor(
                db.cursor(Transaction.getDB_TXN(txn), cfg.flags));
        }

        #endregion Methods
    }
}
