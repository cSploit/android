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
    /// Statistical information about the locking subsystem
    /// </summary>
    public class LockStats {
        private Internal.LockStatStruct st;
        internal LockStats(Internal.LockStatStruct stats) {
            st = stats;
        }

        /// <summary>
        /// Current number of lockers allocated. 
        /// </summary>
        public uint AllocatedLockers { get { return st.st_lockers; } }
        /// <summary>
        /// Current number of locks allocated. 
        /// </summary>
        public uint AllocatedLocks { get { return st.st_locks; } }
        /// <summary>
        /// Current number of lock objects allocated.
        /// </summary>
        public uint AllocatedObjects { get { return st.st_objects; } }
        /// <summary>
        /// Initial number of locks allocated.
        /// </summary>
        public uint InitLocks { get { return st.st_initlocks; } }
        /// <summary>
        /// Initial number of lockers allocated.
        /// </summary>
        public uint InitLockers { get { return st.st_initlockers; } }
        /// <summary>
        /// Initial number of lock objects allocated.
        /// </summary>
        public uint InitObjects { get { return st.st_initobjects; } }
        /// <summary>
        /// Last allocated locker ID. 
        /// </summary>
        public uint LastAllocatedLockerID { get { return st.st_id; } }
        /// <summary>
        /// Lock conflicts w/ subsequent wait 
        /// </summary>
        public ulong LockConflictsWait { get { return st.st_lock_wait; } }
        /// <summary>
        /// Lock conflicts w/o subsequent wait 
        /// </summary>
        public ulong LockConflictsNoWait { get { return st.st_lock_nowait; } }
        /// <summary>
        /// Number of lock deadlocks. 
        /// </summary>
        public ulong LockDeadlocks { get { return st.st_ndeadlocks; } }
        /// <summary>
        /// Number of lock downgrades. 
        /// </summary>
        public ulong LockDowngrades { get { return st.st_ndowngrade; } }
        /// <summary>
        /// Number of lock modes. 
        /// </summary>
        public int LockModes { get { return st.st_nmodes; } }
        /// <summary>
        /// Number of lock puts. 
        /// </summary>
        public ulong LockPuts { get { return st.st_nreleases; } }
        /// <summary>
        /// Number of lock gets. 
        /// </summary>
        public ulong LockRequests { get { return st.st_nrequests; } }
        /// <summary>
        /// Number of lock steals so far. 
        /// </summary>
        public ulong LockSteals { get { return st.st_locksteals; } }
        /// <summary>
        /// Lock timeout. 
        /// </summary>
        public uint LockTimeoutLength { get { return st.st_locktimeout; } }
        /// <summary>
        /// Number of lock timeouts. 
        /// </summary>
        public ulong LockTimeouts { get { return st.st_nlocktimeouts; } }
        /// <summary>
        /// Number of lock upgrades. 
        /// </summary>
        public ulong LockUpgrades { get { return st.st_nupgrade; } }
        /// <summary>
        /// Locker lock granted without wait. 
        /// </summary>
        public ulong LockerNoWait { get { return st.st_lockers_nowait; } }
        /// <summary>
        /// Locker lock granted after wait. 
        /// </summary>
        public ulong LockerWait { get { return st.st_lockers_wait; } }
        /// <summary>
        /// Current number of lockers. 
        /// </summary>
        public uint Lockers { get { return st.st_nlockers; } }
        /// <summary>
        /// Number of hits in the thread locker cache. 
        /// </summary>
        public ulong LockersHit { get { return st.st_nlockers_hit; } }
        /// <summary>
        /// Total number of lockers reused. 
        /// </summary>
        public ulong LockersReused { get { return st.st_nlockers_reused; } }
        /// <summary>
        /// Current number of locks. 
        /// </summary>
        public uint Locks { get { return st.st_nlocks; } }
        /// <summary>
        /// Max length of bucket. 
        /// </summary>
        public uint MaxBucketLength { get { return st.st_hash_len; } }
        /// <summary>
        /// Maximum number steals in any partition. 
        /// </summary>
        public ulong MaxLockSteals { get { return st.st_maxlsteals; } }
        /// <summary>
        /// Maximum number of lockers so far. 
        /// </summary>
        public uint MaxLockers { get { return st.st_maxnlockers; } }
        /// <summary>
        /// Maximum num of lockers in table. 
        /// </summary>
        public uint MaxLockersInTable { get { return st.st_maxlockers; } }
        /// <summary>
        /// Maximum number of locks so far. 
        /// </summary>
        public uint MaxLocks { get { return st.st_maxnlocks; } }
        /// <summary>
        /// Maximum number of locks in any bucket. 
        /// </summary>
        public uint MaxLocksInBucket { get { return st.st_maxhlocks; } }
        /// <summary>
        /// Maximum number of locks in table. 
        /// </summary>
        public uint MaxLocksInTable { get { return st.st_maxlocks; } }
        /// <summary>
        /// Maximum number of steals in any partition. 
        /// </summary>
        public ulong MaxObjectSteals { get { return st.st_maxosteals; } }
        /// <summary>
        /// Maximum number of objects so far. 
        /// </summary>
        public uint MaxObjects { get { return st.st_maxnobjects; } }
        /// <summary>
        /// Maximum number of objectsin any bucket. 
        /// </summary>
        public uint MaxObjectsInBucket { get { return st.st_maxhobjects; } }
        /// <summary>
        /// Max partition lock granted without wait. 
        /// </summary>
        public ulong MaxPartitionLockNoWait { get { return st.st_part_max_nowait; } }
        /// <summary>
        /// Max partition lock granted after wait. 
        /// </summary>
        public ulong MaxPartitionLockWait { get { return st.st_part_max_wait; } }
        /// <summary>
        /// Current maximum unused ID. 
        /// </summary>
        public uint MaxUnusedID { get { return st.st_cur_maxid; } }
        /// <summary>
        /// Maximum num of objects in table. 
        /// </summary>
        public uint MaxObjectsInTable { get { return st.st_maxobjects; } }
        /// <summary>
        /// number of partitions. 
        /// </summary>
        public uint nPartitions { get { return st.st_partitions; } }
        /// <summary>
        /// Object lock granted without wait. 
        /// </summary>
        public ulong ObjectNoWait { get { return st.st_objs_nowait; } }
        /// <summary>
        /// Number of objects steals so far. 
        /// </summary>
        public ulong ObjectSteals { get { return st.st_objectsteals; } }
        /// <summary>
        /// Object lock granted after wait. 
        /// </summary>
        public ulong ObjectWait { get { return st.st_objs_wait; } }
        /// <summary>
        /// Current number of objects. 
        /// </summary>
        public uint Objects { get { return st.st_nobjects; } }
        /// <summary>
        /// Partition lock granted without wait. 
        /// </summary>
        public ulong PartitionLockNoWait { get { return st.st_part_nowait; } }
        /// <summary>
        /// Partition lock granted after wait. 
        /// </summary>
        public ulong PartitionLockWait { get { return st.st_part_wait; } }
        /// <summary>
        /// Region lock granted without wait. 
        /// </summary>
        public ulong RegionNoWait { get { return st.st_region_nowait; } }
        /// <summary>
        /// Region size. 
        /// </summary>
        public ulong RegionSize { get { return (ulong)st.st_regsize.ToInt64(); } }
        /// <summary>
        /// Region lock granted after wait. 
        /// </summary>
        public ulong RegionWait { get { return st.st_region_wait; } }
        /// <summary>
        /// Size of object hash table. 
        /// </summary>
        public uint TableSize { get { return st.st_tablesize; } }
        /// <summary>
        /// Transaction timeout. 
        /// </summary>
        public uint TxnTimeoutLength { get { return st.st_txntimeout; } }
        /// <summary>
        /// Number of transaction timeouts. 
        /// </summary>
        public ulong TxnTimeouts { get { return st.st_ntxntimeouts; } }
        
    }
}

