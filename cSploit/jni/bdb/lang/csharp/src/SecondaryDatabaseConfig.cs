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
    /// A class representing configuration parameters for
    /// <see cref="SecondaryDatabase"/>
    /// </summary>
    public class SecondaryDatabaseConfig : DatabaseConfig {
        private Database pdp;
        /// <summary>
        /// All updates to Primary will be automatically reflected in the
        /// secondary and all reads from the secondary will return corresponding
        /// data from Primary.
        /// </summary>
        /// <remarks>
        /// Note that as primary keys must be unique for secondary indices to
        /// work, Primary must have been configured with
        /// <see cref="DuplicatesPolicy.NONE"/>.
        /// </remarks>
        public Database Primary {
            get { return pdp; }
            set { pdp = value; }
        }

        private SecondaryKeyGenDelegate func;
        /// <summary>
        /// The delegate that creates the set of secondary keys corresponding to
        /// a given primary key and data pair. 
        /// </summary>
        /// <remarks>
        /// KeyGen may be null if both
        /// <see cref="BaseDatabase.ReadOnly">Primary.ReadOnly</see> and
        /// <see cref="DatabaseConfig.ReadOnly"/> are true.
        /// </remarks>
        public SecondaryKeyGenDelegate KeyGen {
            get { return func; }
            set { func = value; }
        }

        /// <summary>
        /// If true and the secondary database is empty, walk through Primary
        /// and create an index to it in the empty secondary. This operation is
        /// potentially very expensive.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If the secondary database has been opened in an environment
        /// configured with transactions, the entire secondary index creation is
        /// performed in the context of a single transaction.
        /// </para>
        /// <para>
        /// Care should be taken not to use a newly-populated secondary database
        /// in another thread of control until
        /// <see cref="SecondaryDatabase.Open"/> has returned successfully in
        /// the first thread.
        /// </para>
        /// <para>
        /// If transactions are not being used, care should be taken not to
        /// modify a primary database being used to populate a secondary
        /// database, in another thread of control, until
        /// <see cref="SecondaryDatabase.Open"/> has returned successfully in
        /// the first thread. If transactions are being used, Berkeley DB will
        /// perform appropriate locking and the application need not do any
        /// special operation ordering.
        /// </para>
        /// </remarks>
        public bool Populate;
        /// <summary>
        /// If true, the secondary key is immutable.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This setting can be used to optimize updates when the secondary key
        /// in a primary record will never be changed after the primary record
        /// is inserted. For immutable secondary keys, a best effort is made to
        /// avoid calling the secondary callback function when primary records
        /// are updated. This optimization may reduce the overhead of update
        /// operations significantly if the callback function is expensive.
        /// </para>
        /// <para>
        /// Be sure to specify this setting only if the secondary key in the
        /// primary record is never changed. If this rule is violated, the
        /// secondary index will become corrupted, that is, it will become out
        /// of sync with the primary.
        /// </para>
        /// </remarks>
        public bool ImmutableKey;

        internal DatabaseType DbType;

        private Database fdbp;
        private ForeignKeyNullifyDelegate nullifier;
        private ForeignKeyDeleteAction fkaction;
        /// <summary>
        /// Specify the action taken when a referenced record in the foreign key
        /// database is deleted.
        /// </summary>
        /// <param name="ForeignDB">The foreign key database.</param>
        /// <param name="OnDelete">
        /// The action taken when a referenced record is deleted.
        /// </param>
        public void SetForeignKeyConstraint(
            Database ForeignDB, ForeignKeyDeleteAction OnDelete) {
            SetForeignKeyConstraint(ForeignDB, OnDelete, null);
        }
        /// <summary>
        /// Specify the action taken when a referenced record in the foreign key
        /// database is deleted.
        /// </summary>
        /// <param name="ForeignDB">The foreign key database.</param>
        /// <param name="OnDelete">
        /// The action taken when a reference record is deleted.
        /// </param>
        /// <param name="NullifyFunc">
        /// When <paramref name="OnDelete"/> is
        /// <see cref="ForeignKeyDeleteAction.NULLIFY"/>, NullifyFunc is used to
        /// set the foreign key to null.
        /// </param>
        public void SetForeignKeyConstraint(Database ForeignDB,
            ForeignKeyDeleteAction OnDelete,
            ForeignKeyNullifyDelegate NullifyFunc) {
            if (OnDelete == ForeignKeyDeleteAction.NULLIFY &&
                NullifyFunc == null)
                throw new ArgumentException(
                    "A nullifying function must " +
                    "be provided when ForeignKeyDeleteAction.NULLIFY is set.");
            fdbp = ForeignDB;
            fkaction = OnDelete;
            nullifier = NullifyFunc;
        }
        /// <summary>
        /// The database used to check the foreign key integrity constraint.
        /// </summary>
        public Database ForeignKeyDatabase { get { return fdbp; } }
        /// <summary>
        /// The action taken when a referenced record in
        /// <see cref="ForeignKeyDatabase"/> is deleted.
        /// </summary>
        public ForeignKeyDeleteAction OnForeignKeyDelete {
            get { return fkaction; }
        }
        /// <summary>
        /// The nullifying function used to set the foreign key to null.
        /// </summary>
        public ForeignKeyNullifyDelegate ForeignKeyNullfier {
            get { return nullifier; }
        }

        /// <summary>
        /// Instantiate a new SecondaryDatabaseConfig object, with the default
        /// configuration settings.
        /// </summary>
        public SecondaryDatabaseConfig(
            Database PrimaryDB, SecondaryKeyGenDelegate KeyGenFunc) {
            Primary = PrimaryDB;
            KeyGen = KeyGenFunc;
            DbType = DatabaseType.UNKNOWN;
        }

        internal uint assocFlags {
            get {
                uint ret = 0;
                ret |= Populate ? DbConstants.DB_CREATE : 0;
                ret |= ImmutableKey ? DbConstants.DB_IMMUTABLE_KEY : 0;
                return ret;
            }
        }

        internal uint foreignFlags {
            get {
                uint ret = 0;
                ret |= (uint)OnForeignKeyDelete;
                return ret;
            }
        }
    }
}