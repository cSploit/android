/*-
 * Automatically built by dist/s_java_stat.
 * Only the javadoc comments can be edited.
 *
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

package com.sleepycat.db;

/**
Lock statistics for a database environment.
*/
public class LockStats {
    // no public constructor
    /* package */ LockStats() {}

    private int st_id;
    /**
    The last allocated locker ID.
    */
    public int getId() {
        return st_id;
    }

    private int st_cur_maxid;
    /**
    The current maximum unused locker ID.
    */
    public int getCurMaxId() {
        return st_cur_maxid;
    }

    private int st_initlocks;
    /** The initial number of locks allocated in the lock table. */
    public int getInitlocks() {
        return st_initlocks;
    }

    private int st_initlockers;
    /** The initial number of lockers allocated in lock table. */
    public int getInitlockers() {
        return st_initlockers;
    }

    private int st_initobjects;
    /** The initial number of lock objects allocated in lock table. */
    public int getInitobjects() {
        return st_initobjects;
    }

    private int st_locks;
    /** The current number of locks allocated in lock table. */
    public int getLocks() {
        return st_locks;
    }

    private int st_lockers;
    /** The current number of lockers allocated in lock table. */
    public int getLockers() {
        return st_lockers;
    }

    private int st_objects;
    /** The current number of lock objects allocated in lock table. */
    public int getObjects() {
        return st_objects;
    }

    private int st_maxlocks;
    /**
    The maximum number of locks possible.
    */
    public int getMaxLocks() {
        return st_maxlocks;
    }

    private int st_maxlockers;
    /**
    The maximum number of lockers possible.
    */
    public int getMaxLockers() {
        return st_maxlockers;
    }

    private int st_maxobjects;
    /**
    The maximum number of lock objects possible.
    */
    public int getMaxObjects() {
        return st_maxobjects;
    }

    private int st_partitions;
    /** 
    The number of lock table partitions.
    */
    public int getPartitions() {
        return st_partitions;
    }

    private int st_tablesize;
    /** The size of object hash table. */
    public int getTableSize() {
        return st_tablesize;
    }

    private int st_nmodes;
    /**
    The number of lock modes.
    */
    public int getNumModes() {
        return st_nmodes;
    }

    private int st_nlockers;
    /**
    The number of current lockers.
    */
    public int getNumLockers() {
        return st_nlockers;
    }

    private int st_nlocks;
    /**
    The number of current locks.
    */
    public int getNumLocks() {
        return st_nlocks;
    }

    private int st_maxnlocks;
    /**
    The maximum number of locks at any one time.  Note that if there is more than one partition, this is the sum of the maximum across all partitions.
    */
    public int getMaxNlocks() {
        return st_maxnlocks;
    }

    private int st_maxhlocks;
    /** 
    The maximum number of locks in any hash bucket at any one time.
    */
    public int getMaxHlocks() {
        return st_maxhlocks;
    }

    private long st_locksteals;
    /** 
    The maximum number of locks stolen by an empty partition. 
    */
    public long getLocksteals() {
        return st_locksteals;
    }

    private long st_maxlsteals;
    /**
    The maximum number of lock steals for any one partition. 
    */
    public long getMaxLsteals() {
        return st_maxlsteals;
    }

    private int st_maxnlockers;
    /**
    The maximum number of lockers at any one time.
    */
    public int getMaxNlockers() {
        return st_maxnlockers;
    }

    private int st_nobjects;
    /**
    The number of current lock objects.
    */
    public int getNobjects() {
        return st_nobjects;
    }

    private int st_maxnobjects;
    /**
    The maximum number of lock objects at any one time.  Note that if there is more than one partition this is the sum of the maximum across all partitions.
    */
    public int getMaxNobjects() {
        return st_maxnobjects;
    }

    private int st_maxhobjects;
    /**
    The maximum number of objects in any hash bucket at any one time.
    */
    public int getMaxHobjects() {
        return st_maxhobjects;
    }

    private long st_objectsteals;
    /**
    The maximum number of objects stolen by an empty partition.
    */
    public long getObjectsteals() {
        return st_objectsteals;
    }

    private long st_maxosteals;
    /**
    The maximum number of object steals for any one partition.
    */
    public long getMaxOsteals() {
        return st_maxosteals;
    }

    private long st_nrequests;
    /**
    The total number of locks requested.
    */
    public long getNumRequests() {
        return st_nrequests;
    }

    private long st_nreleases;
    /**
    The total number of locks released.
    */
    public long getNumReleases() {
        return st_nreleases;
    }

    private long st_nupgrade;
    /** The total number of locks upgraded. **/
    public long getNumUpgrade() {
        return st_nupgrade;
    }

    private long st_ndowngrade;
    /** The total number of locks downgraded. **/
    public long getNumDowngrade() {
        return st_ndowngrade;
    }

    private long st_lock_wait;
    /**
    The number of lock requests not immediately available due to conflicts,
    for which the thread of control waited.
    */
    public long getLockWait() {
        return st_lock_wait;
    }

    private long st_lock_nowait;
    /**
    The number of lock requests not immediately available due to conflicts,
    for which the thread of control did not wait.
    */
    public long getLockNowait() {
        return st_lock_nowait;
    }

    private long st_ndeadlocks;
    /**
    The number of deadlocks.
    */
    public long getNumDeadlocks() {
        return st_ndeadlocks;
    }

    private int st_locktimeout;
    /**
    Lock timeout value.
    */
    public int getLockTimeout() {
        return st_locktimeout;
    }

    private long st_nlocktimeouts;
    /**
    The number of lock requests that have timed out.
    */
    public long getNumLockTimeouts() {
        return st_nlocktimeouts;
    }

    private int st_txntimeout;
    /**
    Transaction timeout value.
    */
    public int getTxnTimeout() {
        return st_txntimeout;
    }

    private long st_ntxntimeouts;
    /**
    The number of transactions that have timed out.  This value is also
    a component of st_ndeadlocks, the total number of deadlocks detected.
    */
    public long getNumTxnTimeouts() {
        return st_ntxntimeouts;
    }

    private long st_part_wait;
    /** 
     The number of times that a thread of control was forced to wait before 
     obtaining a lock partition mutex.
     * */
    public long getPartWait() {
        return st_part_wait;
    }

    private long st_part_nowait;
    /** 
     The number of times that a thread of control was able to obtain a lock 
     partition mutex without waiting.
     * */
    public long getPartNowait() {
        return st_part_nowait;
    }

    private long st_part_max_wait;
    /** 
     The maximum number of times that a thread of control was forced to wait 
     before obtaining any one lock partition mutex.
     * */
    public long getPartMaxWait() {
        return st_part_max_wait;
    }

    private long st_part_max_nowait;
    /** 
     The number of times that a thread of control was able to obtain any one 
     lock partition mutex without waiting.
     * */
    public long getPartMaxNowait() {
        return st_part_max_nowait;
    }

    private long st_objs_wait;
    /**
    The number of requests to allocate or deallocate an object for which the
    thread of control waited.
    */
    public long getObjsWait() {
        return st_objs_wait;
    }

    private long st_objs_nowait;
    /**
    The number of requests to allocate or deallocate an object for which the
    thread of control did not wait.
    */
    public long getObjsNowait() {
        return st_objs_nowait;
    }

    private long st_lockers_wait;
    /**
    The number of requests to allocate or deallocate a locker for which the
    thread of control waited.
    */
    public long getLockersWait() {
        return st_lockers_wait;
    }

    private long st_lockers_nowait;
    /**
    The number of requests to allocate or deallocate a locker for which the
    thread of control did not wait.
    */
    public long getLockersNowait() {
        return st_lockers_nowait;
    }

    private long st_region_wait;
    /**
    The number of times that a thread of control was forced to wait
    before obtaining the region lock.
    */
    public long getRegionWait() {
        return st_region_wait;
    }

    private long st_region_nowait;
    /**
    The number of times that a thread of control was able to obtain the
    region lock without waiting.
    */
    public long getRegionNowait() {
        return st_region_nowait;
    }

    private long st_nlockers_hit;
    /**
    The number of hits in the thread locker cache.
    */
    public long getNumLockersHit() {
        return st_nlockers_hit;
    }

    private long st_nlockers_reused;
    /**
    Total number of lockers reused.
    */
    public long getNumLockersReused() {
        return st_nlockers_reused;
    }

    private int st_hash_len;
    /**
    Maximum length of a lock hash bucket.
    */
    public int getHashLen() {
        return st_hash_len;
    }

    private long st_regsize;
    /**
    The size of the lock region.
    */
    public long getRegSize() {
        return st_regsize;
    }

    /**
    For convenience, the LockStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "LockStats:"
            + "\n  st_id=" + st_id
            + "\n  st_cur_maxid=" + st_cur_maxid
            + "\n  st_initlocks=" + st_initlocks
            + "\n  st_initlockers=" + st_initlockers
            + "\n  st_initobjects=" + st_initobjects
            + "\n  st_locks=" + st_locks
            + "\n  st_lockers=" + st_lockers
            + "\n  st_objects=" + st_objects
            + "\n  st_maxlocks=" + st_maxlocks
            + "\n  st_maxlockers=" + st_maxlockers
            + "\n  st_maxobjects=" + st_maxobjects
            + "\n  st_partitions=" + st_partitions
            + "\n  st_tablesize=" + st_tablesize
            + "\n  st_nmodes=" + st_nmodes
            + "\n  st_nlockers=" + st_nlockers
            + "\n  st_nlocks=" + st_nlocks
            + "\n  st_maxnlocks=" + st_maxnlocks
            + "\n  st_maxhlocks=" + st_maxhlocks
            + "\n  st_locksteals=" + st_locksteals
            + "\n  st_maxlsteals=" + st_maxlsteals
            + "\n  st_maxnlockers=" + st_maxnlockers
            + "\n  st_nobjects=" + st_nobjects
            + "\n  st_maxnobjects=" + st_maxnobjects
            + "\n  st_maxhobjects=" + st_maxhobjects
            + "\n  st_objectsteals=" + st_objectsteals
            + "\n  st_maxosteals=" + st_maxosteals
            + "\n  st_nrequests=" + st_nrequests
            + "\n  st_nreleases=" + st_nreleases
            + "\n  st_nupgrade=" + st_nupgrade
            + "\n  st_ndowngrade=" + st_ndowngrade
            + "\n  st_lock_wait=" + st_lock_wait
            + "\n  st_lock_nowait=" + st_lock_nowait
            + "\n  st_ndeadlocks=" + st_ndeadlocks
            + "\n  st_locktimeout=" + st_locktimeout
            + "\n  st_nlocktimeouts=" + st_nlocktimeouts
            + "\n  st_txntimeout=" + st_txntimeout
            + "\n  st_ntxntimeouts=" + st_ntxntimeouts
            + "\n  st_part_wait=" + st_part_wait
            + "\n  st_part_nowait=" + st_part_nowait
            + "\n  st_part_max_wait=" + st_part_max_wait
            + "\n  st_part_max_nowait=" + st_part_max_nowait
            + "\n  st_objs_wait=" + st_objs_wait
            + "\n  st_objs_nowait=" + st_objs_nowait
            + "\n  st_lockers_wait=" + st_lockers_wait
            + "\n  st_lockers_nowait=" + st_lockers_nowait
            + "\n  st_region_wait=" + st_region_wait
            + "\n  st_region_nowait=" + st_region_nowait
            + "\n  st_nlockers_hit=" + st_nlockers_hit
            + "\n  st_nlockers_reused=" + st_nlockers_reused
            + "\n  st_hash_len=" + st_hash_len
            + "\n  st_regsize=" + st_regsize
            ;
    }
}
