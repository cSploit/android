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
Statistics for a file in the cache.
*/
public class CacheFileStats {
    // no public constructor
    /* package */ CacheFileStats() {}

    private int st_pagesize;
    /**
    Page size in bytes.
    */
    public int getPageSize() {
        return st_pagesize;
    }

    private int st_map;
    /**
    Requested pages mapped into the process' address space.
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

    private long st_backup_spins;
    /** Spins while trying to back up the file. */
    public long getBackupSpins() {
        return st_backup_spins;
    }

    private String file_name;
    /**
    The name of the file.
    */
    public String getFileName() {
        return file_name;
    }

    /**
    For convenience, the CacheFileStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "CacheFileStats:"
            + "\n  st_pagesize=" + st_pagesize
            + "\n  st_map=" + st_map
            + "\n  st_cache_hit=" + st_cache_hit
            + "\n  st_cache_miss=" + st_cache_miss
            + "\n  st_page_create=" + st_page_create
            + "\n  st_page_in=" + st_page_in
            + "\n  st_page_out=" + st_page_out
            + "\n  st_backup_spins=" + st_backup_spins
            + "\n  file_name=" + file_name
            ;
    }
}
