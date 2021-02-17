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
Cache statistics for a database environment.
*/
public class CacheStats {
    // no public constructor
    /* package */ CacheStats() {}

    private int st_gbytes;
    /**
    Gigabytes of cache (total cache size is st_gbytes + st_bytes).
    */
    public int getGbytes() {
        return st_gbytes;
    }

    private int st_bytes;
    /**
    Bytes of cache (total cache size is st_gbytes + st_bytes).
    */
    public int getBytes() {
        return st_bytes;
    }

    private int st_ncache;
    /**
    Number of caches.
    */
    public int getNumCache() {
        return st_ncache;
    }

    private int st_max_ncache;
    /**
    Maximum number of caches, as configured with
    {@link EnvironmentConfig#setCacheMax}.
    */
    public int getMaxNumCache() {
        return st_max_ncache;
    }

    private long st_mmapsize;
    /**
    Maximum file size for mmap.
     */
    public long getMmapSize() {
        return st_mmapsize;
    }

    private int st_maxopenfd;
    /**
    Maximum number of open file descriptors.
     */
    public int getMaxOpenfd() {
        return st_maxopenfd;
    }

    private int st_maxwrite;
    /**
    The maximum number of sequential write operations scheduled by the library
    when flushing dirty pages from the cache.
     */
    public int getMaxWrite() {
        return st_maxwrite;
    }

    private int st_maxwrite_sleep;
    /**
    The number of microseconds the thread of control should pause before
    scheduling further write operations.
     */
    public int getMaxWriteSleep() {
        return st_maxwrite_sleep;
    }

    private int st_pages;
    /**
    Pages in the cache.
    */
    public int getPages() {
        return st_pages;
    }

    private int st_map;
    /**
    Requested pages mapped into the process' address space (there is no
    available information about whether or not this request caused disk I/O,
    although examining the application page fault rate may be helpful).
    */
    public int getMap() {
        return st_map;
    }

    private long st_cache_hit;
    /**
    Requested pages found in the cache.
    */
    public long getCacheHit() {
        return st_cache_hit;
    }

    private long st_cache_miss;
    /**
    Requested pages not found in the cache.
    */
    public long getCacheMiss() {
        return st_cache_miss;
    }

    private long st_page_create;
    /**
    Pages created in the cache.
    */
    public long getPageCreate() {
        return st_page_create;
    }

    private long st_page_in;
    /**
    Pages read into the cache.
    */
    public long getPageIn() {
        return st_page_in;
    }

    private long st_page_out;
    /**
    Pages written from the cache to the backing file.
    */
    public long getPageOut() {
        return st_page_out;
    }

    private long st_ro_evict;
    /**
    Clean pages forced from the cache.
    */
    public long getRoEvict() {
        return st_ro_evict;
    }

    private long st_rw_evict;
    /**
    Dirty pages forced from the cache.
    */
    public long getRwEvict() {
        return st_rw_evict;
    }

    private long st_page_trickle;
    /**
    Dirty pages written using {@link com.sleepycat.db.Environment#trickleCacheWrite Environment.trickleCacheWrite}.
    */
    public long getPageTrickle() {
        return st_page_trickle;
    }

    private int st_page_clean;
    /**
    Clean pages currently in the cache.
    */
    public int getPageClean() {
        return st_page_clean;
    }

    private int st_page_dirty;
    /**
    Dirty pages currently in the cache.
    */
    public int getPageDirty() {
        return st_page_dirty;
    }

    private int st_hash_buckets;
    /**
    Number of hash buckets in the buffer hash table.
    */
    public int getHashBuckets() {
        return st_hash_buckets;
    }

    private int st_hash_mutexes;
    /** The number of hash bucket mutexes in the buffer hash table. */
    public int getHashMutexes() {
        return st_hash_mutexes;
    }

    private int st_pagesize;
    /**
    Page size in bytes.
    */
    public int getPageSize() {
        return st_pagesize;
    }

    private int st_hash_searches;
    /**
    Total number of buffer hash table lookups.
    */
    public int getHashSearches() {
        return st_hash_searches;
    }

    private int st_hash_longest;
    /**
    The longest chain ever encountered in buffer hash table lookups.
    */
    public int getHashLongest() {
        return st_hash_longest;
    }

    private long st_hash_examined;
    /**
    Total number of hash elements traversed during hash table lookups.
    */
    public long getHashExamined() {
        return st_hash_examined;
    }

    private long st_hash_nowait;
    /**
    The number of times that a thread of control was able to obtain a
    hash bucket lock without waiting.
    */
    public long getHashNowait() {
        return st_hash_nowait;
    }

    private long st_hash_wait;
    /**
    The number of times that a thread of control was forced to wait
    before obtaining a hash bucket lock.
    */
    public long getHashWait() {
        return st_hash_wait;
    }

    private long st_hash_max_nowait;
    /**
    The number of times a thread of control was able to obtain the
    hash bucket lock without waiting on the bucket which had the
    maximum number of times that a thread of control needed to wait.
    */
    public long getHashMaxNowait() {
        return st_hash_max_nowait;
    }

    private long st_hash_max_wait;
    /**
    The maximum number of times any hash bucket lock was waited for by
    a thread of control.
    */
    public long getHashMaxWait() {
        return st_hash_max_wait;
    }

    private long st_region_nowait;
    /**
    The number of times that a thread of control was able to obtain a
    region lock without waiting.
    */
    public long getRegionNowait() {
        return st_region_nowait;
    }

    private long st_region_wait;
    /**
    The number of times that a thread of control was forced to wait
    before obtaining a region lock.
    */
    public long getRegionWait() {
        return st_region_wait;
    }

    private long st_mvcc_frozen;
    /**
    Number of buffers frozen.
    */
    public long getMultiversionFrozen() {
        return st_mvcc_frozen;
    }

    private long st_mvcc_thawed;
    /**
    Number of buffers thawed.
    */
    public long getMultiversionThawed() {
        return st_mvcc_thawed;
    }

    private long st_mvcc_freed;
    /**
    Number of frozen buffers freed.
    */
    public long getMultiversionFreed() {
        return st_mvcc_freed;
    }

    private long st_mvcc_reused;
    /**
    Number of outdated intermediate versions reused.
    */
    public long getMultiversionReused() {
        return st_mvcc_reused;
    }

    private long st_alloc;
    /**
    Number of page allocations.
    */
    public long getAlloc() {
        return st_alloc;
    }

    private long st_alloc_buckets;
    /**
    Number of hash buckets checked during allocation.
    */
    public long getAllocBuckets() {
        return st_alloc_buckets;
    }

    private long st_alloc_max_buckets;
    /**
    Maximum number of hash buckets checked during an allocation.
    */
    public long getAllocMaxBuckets() {
        return st_alloc_max_buckets;
    }

    private long st_alloc_pages;
    /**
    Number of pages checked during allocation.
    */
    public long getAllocPages() {
        return st_alloc_pages;
    }

    private long st_alloc_max_pages;
    /**
    Maximum number of pages checked during an allocation.
    */
    public long getAllocMaxPages() {
        return st_alloc_max_pages;
    }

    private long st_io_wait;
    /**
    Number of operations blocked waiting for I/O to complete.
    */
    public long getIoWait() {
        return st_io_wait;
    }

    private long st_sync_interrupted;
    /**
    Number of mpool sync operations interrupted.
    */
    public long getSyncInterrupted() {
        return st_sync_interrupted;
    }

    private int st_oddfsize_detect;
    /**
    Odd file size detected.
    */
    public int getOddfSizeDetect() {
        return st_oddfsize_detect;
    }

    private int st_oddfsize_resolve;
    /**
    Odd file size resolved.
    */
    public int getOddfSizeResolve() {
        return st_oddfsize_resolve;
    }

    private long st_regsize;
    /**
    Individual cache size.
    */
    public long getRegSize() {
        return st_regsize;
    }

    private long st_regmax;
    /** The max size of the mutex region size. */
    public long getRegmax() {
        return st_regmax;
    }

    /**
    For convenience, the CacheStats class has a toString method that
    lists all the data fields.
    */
    public String toString() {
        return "CacheStats:"
            + "\n  st_gbytes=" + st_gbytes
            + "\n  st_bytes=" + st_bytes
            + "\n  st_ncache=" + st_ncache
            + "\n  st_max_ncache=" + st_max_ncache
            + "\n  st_mmapsize=" + st_mmapsize
            + "\n  st_maxopenfd=" + st_maxopenfd
            + "\n  st_maxwrite=" + st_maxwrite
            + "\n  st_maxwrite_sleep=" + st_maxwrite_sleep
            + "\n  st_pages=" + st_pages
            + "\n  st_map=" + st_map
            + "\n  st_cache_hit=" + st_cache_hit
            + "\n  st_cache_miss=" + st_cache_miss
            + "\n  st_page_create=" + st_page_create
            + "\n  st_page_in=" + st_page_in
            + "\n  st_page_out=" + st_page_out
            + "\n  st_ro_evict=" + st_ro_evict
            + "\n  st_rw_evict=" + st_rw_evict
            + "\n  st_page_trickle=" + st_page_trickle
            + "\n  st_page_clean=" + st_page_clean
            + "\n  st_page_dirty=" + st_page_dirty
            + "\n  st_hash_buckets=" + st_hash_buckets
            + "\n  st_hash_mutexes=" + st_hash_mutexes
            + "\n  st_pagesize=" + st_pagesize
            + "\n  st_hash_searches=" + st_hash_searches
            + "\n  st_hash_longest=" + st_hash_longest
            + "\n  st_hash_examined=" + st_hash_examined
            + "\n  st_hash_nowait=" + st_hash_nowait
            + "\n  st_hash_wait=" + st_hash_wait
            + "\n  st_hash_max_nowait=" + st_hash_max_nowait
            + "\n  st_hash_max_wait=" + st_hash_max_wait
            + "\n  st_region_nowait=" + st_region_nowait
            + "\n  st_region_wait=" + st_region_wait
            + "\n  st_mvcc_frozen=" + st_mvcc_frozen
            + "\n  st_mvcc_thawed=" + st_mvcc_thawed
            + "\n  st_mvcc_freed=" + st_mvcc_freed
            + "\n  st_mvcc_reused=" + st_mvcc_reused
            + "\n  st_alloc=" + st_alloc
            + "\n  st_alloc_buckets=" + st_alloc_buckets
            + "\n  st_alloc_max_buckets=" + st_alloc_max_buckets
            + "\n  st_alloc_pages=" + st_alloc_pages
            + "\n  st_alloc_max_pages=" + st_alloc_max_pages
            + "\n  st_io_wait=" + st_io_wait
            + "\n  st_sync_interrupted=" + st_sync_interrupted
            + "\n  st_oddfsize_detect=" + st_oddfsize_detect
            + "\n  st_oddfsize_resolve=" + st_oddfsize_resolve
            + "\n  st_regsize=" + st_regsize
            + "\n  st_regmax=" + st_regmax
            ;
    }
}
