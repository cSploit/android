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
The HeapStats object is used to return Heap database statistics.
*/
public class HeapStats extends DatabaseStats {
    // no public constructor
    /* package */ HeapStats() {}

    private int heap_magic;
    /** 
	Magic number that identifies the file as a Heap file.
    */
    public int getHeapMagic() {
        return heap_magic;
    }

    private int heap_version;
    /** 
	The version of the Heap database.
    */
    public int getHeapVersion() {
        return heap_version;
    }

    private int heap_metaflags;
    /**
       Reports internal flags. For internal use only. 
    */
    public int getHeapMetaFlags() {
        return heap_metaflags;
    }

    private int heap_nblobs;
    /**
        The number of blob records.
    */
    public int getHeapNumBlobs() {
        return heap_nblobs;
    }

    private int heap_nrecs;
    /**
       Reports the number of records in the Heap database.
    */
    public int getHeapNumRecs() {
        return heap_nrecs;
    }

    private int heap_pagecnt;
    /**
       The number of pages in the database.
    */
    public int getHeapPageCount() {
        return heap_pagecnt;
    }

    private int heap_pagesize;
    /**
       The underlying database page (and bucket) size, in bytes.
    */
    public int getHeapPageSize() {
        return heap_pagesize;
    }

    private int heap_nregions;
    /**
       The number of regions in the Heap database.
    */
    public int getHeapNumRegions() {
        return heap_nregions;
    }

    private int heap_regionsize;
    /** 
    The number of pages in a region in the Heap database. Returned if
    DB_FAST_STAT is set. 
    */
    public int getHeapRegionSize() {
        return heap_regionsize;
    }

    /**
    For convenience, the HeapStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "HeapStats:"
            + "\n  heap_magic=" + heap_magic
            + "\n  heap_version=" + heap_version
            + "\n  heap_metaflags=" + heap_metaflags
            + "\n  heap_nblobs=" + heap_nblobs
            + "\n  heap_nrecs=" + heap_nrecs
            + "\n  heap_pagecnt=" + heap_pagecnt
            + "\n  heap_pagesize=" + heap_pagesize
            + "\n  heap_nregions=" + heap_nregions
            + "\n  heap_regionsize=" + heap_regionsize
            ;
    }
}
