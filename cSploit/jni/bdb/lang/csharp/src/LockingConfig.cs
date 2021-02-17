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
    /// A class representing configuration parameters for a
    /// <see cref="DatabaseEnvironment"/>'s locking subsystem.
    /// </summary>
    public class LockingConfig {
        private byte[,] _conflicts;
        private int _nmodes;
        /// <summary>
        /// The locking conflicts matrix. 
        /// </summary>
        /// <remarks>
        /// <para>
        /// If Conflicts is never set, a standard conflicts array is used; see
        /// Standard Lock Modes in the Programmer's Reference Guide for more
        /// information.
        /// </para>
        /// <para>
        /// Conflicts parameter is an nmodes by nmodes array. A non-0 value for
        /// the array element indicates that requested_mode and held_mode
        /// conflict:
        /// <code>
        /// conflicts[requested_mode][held_mode] 
        /// </code>
        /// </para>
        /// <para>The not-granted mode must be represented by 0.</para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// Conflicts will be ignored.
        /// </para>
        /// </remarks>
        public byte[,] Conflicts {
            get { return _conflicts; }
            set {
                double sz;
                sz = Math.Sqrt(value.Length);
                if (Math.Truncate(sz) == sz) {
                    _conflicts = value;
                    _nmodes = (int)sz;
                } else
                    throw new ArgumentException("Conflicts matrix must be square.");
            }
        }

        private uint _initlockercount;
        internal bool initLockerCountIsSet;
        /// <summary>
        /// The initial number of simultaneous locking entities created by the
        /// Berkeley DB environment
        /// </summary>
        /// <remarks>
        /// <para>
        /// This value is used by <see cref="DatabaseEnvironment.Open"/> to
        /// force Berkeley DB to allocate a certain number of locker objects
        /// when the environment is created. This can be useful if an
        /// application uses a large number of locker objects, and experiences
        /// performance issues with the default dynamic allocation algorithm.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// InitLockers will be ignored.
        /// </para>
        /// </remarks>
        public uint InitLockerCount {
            get { return _initlockercount; }
            set {
                initLockerCountIsSet = true;
                _initlockercount = value;
            }
        }

        private uint _initlockcount;
        internal bool initLockCountIsSet;
        /// <summary>
        /// The initial number of locks created by the Berkeley DB environment
        /// </summary>
        /// <remarks>
        /// <para>
        /// This value is used by <see cref="DatabaseEnvironment.Open"/> to
        /// force Berkeley DB to allocate a certain number of locks when
        /// the environment is created. This can be useful if an application
        /// uses a large number of locks, and experiences performance issues
        /// with the default dynamic allocation algorithm.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// InitLocks will be ignored.
        /// </para>
        /// </remarks>
        public uint InitLockCount {
            get { return _initlockcount; }
            set {
                initLockCountIsSet = true;
                _initlockcount = value;
            }
        }

        private uint _initlockobjectcount;
        internal bool initLockObjectCountIsSet;
        /// <summary>
        /// The initial number of lock objects created by the Berkeley DB
        /// environment
        /// </summary>
        /// <remarks>
        /// <para>
        /// This value is used by <see cref="DatabaseEnvironment.Open"/> to
        /// force Berkeley DB to allocate a certain number of lock objects
        /// when the environment is created. This can be useful if an
        /// application uses a large number of lock objects, and experiences
        /// performance issues with the default dynamic allocation algorithm.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// InitLockObjects will be ignored.
        /// </para>
        /// </remarks>
        public uint InitLockObjectCount {
            get { return _initlockobjectcount; }
            set {
                initLockObjectCountIsSet = true;
                _initlockobjectcount = value;
            }
        }

        private uint _maxlockers;
        internal bool maxLockersIsSet;
        /// <summary>
        /// The maximum number of simultaneous locking entities supported by the
        /// Berkeley DB environment
        /// </summary>
        /// <remarks>
        /// <para>
        /// This value is used by <see cref="DatabaseEnvironment.Open"/> to
        /// estimate how much space to allocate for various lock-table data
        /// structures. The default value is 1000 lockers. For specific
        /// information on configuring the size of the lock subsystem, see
        /// Configuring locking: sizing the system in the Programmer's Reference
        /// Guide.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// MaxLockers will be ignored.
        /// </para>
        /// </remarks>
        public uint MaxLockers {
            get { return _maxlockers; }
            set {
                maxLockersIsSet = true;
                _maxlockers = value;
            }
        }
        private uint _maxlocks;
        internal bool maxLocksIsSet;
        /// <summary>
        /// The maximum number of locks supported by the Berkeley DB
        /// environment.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This value is used by <see cref="DatabaseEnvironment.Open"/> to
        /// estimate how much space to allocate for various lock-table data
        /// structures. The default value is 1000 lockers. For specific
        /// information on configuring the size of the lock subsystem, see
        /// Configuring locking: sizing the system in the Programmer's Reference
        /// Guide.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// MaxLocks will be ignored.
        /// </para>
        /// </remarks>
        public uint MaxLocks {
            get { return _maxlocks; }
            set {
                maxLocksIsSet = true;
                _maxlocks = value;
            }
        }
        private uint _maxobjects;
        internal bool maxObjectsIsSet;
        /// <summary>
        /// The maximum number of locked objects supported by the Berkeley DB
        /// environment.
        /// </summary>
        /// <remarks>
        /// <para>
        /// This value is used by <see cref="DatabaseEnvironment.Open"/> to
        /// estimate how much space to allocate for various lock-table data
        /// structures. The default value is 1000 lockers. For specific
        /// information on configuring the size of the lock subsystem, see
        /// Configuring locking: sizing the system in the Programmer's Reference
        /// Guide.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// MaxObjects will be ignored.
        /// </para>
        /// </remarks>
        public uint MaxObjects {
            get { return _maxobjects; }
            set {
                maxObjectsIsSet = true;
                _maxobjects = value;
            }
        }
        private uint _partitions;
        internal bool partitionsIsSet;
        /// <summary>
        /// The number of lock table partitions in the Berkeley DB environment.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The default value is 10 times the number of CPUs on the system if
        /// there is more than one CPU. Increasing the number of partitions can
        /// provide for greater throughput on a system with multiple CPUs and
        /// more than one thread contending for the lock manager. On single
        /// processor systems more than one partition may increase the overhead
        /// of the lock manager. Systems often report threading contexts as
        /// CPUs. If your system does this, set the number of partitions to 1 to
        /// get optimal performance.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// Partitions will be ignored.
        /// </para>
        /// </remarks>
        public uint Partitions {
            get { return _partitions; }
            set {
                partitionsIsSet = true;
                _partitions = value;
            }
        }
        private uint _tablesize;
        internal bool tablesizeIsSet;
        /// <summary>
        /// Set the number of buckets in the lock object hash table in the
        /// Berkeley DB environment.
        /// </summary>
        /// <remarks>
        /// <para>
        /// The table is generally set to be close to the number of lock
        /// objects in the system to avoid collisions and delays in processing
        /// of lock operations.
        /// </para>
        /// <para>
        /// If the database environment already exists when
        /// <see cref="DatabaseEnvironment.Open"/> is called, the value of
        /// tablesize will be ignored.
        /// </para>
        /// </remarks>
        public uint TableSize {
            get { return _tablesize; }
            set {
                tablesizeIsSet = true;
                _tablesize = value;
            }
        }

        /// <summary>
        /// If non-null, the deadlock detector is to be run whenever a lock
        /// conflict occurs, lock request(s) should be rejected according to the
        /// specified policy.
        /// </summary>
        /// <remarks>
        /// <para>
        /// As transactions acquire locks on behalf of a single locker ID,
        /// rejecting a lock request associated with a transaction normally
        /// requires the transaction be aborted.
        /// </para>
        /// </remarks>
        public DeadlockPolicy DeadlockResolution;
    }
}
