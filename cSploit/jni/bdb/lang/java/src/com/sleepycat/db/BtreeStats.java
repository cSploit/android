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
The BtreeStats object is used to return Btree
or Recno database statistics.
*/
public class BtreeStats extends DatabaseStats {
    // no public constructor
    /* package */ BtreeStats() {}

    private int bt_magic;
    /**
    The magic number that identifies the file as a Btree database.
    */
    public int getMagic() {
        return bt_magic;
    }

    private int bt_version;
    /**
    The version of the Btree database.
    */
    public int getVersion() {
        return bt_version;
    }

    private int bt_metaflags;
    /**
    Reports internal flags. For internal use only.
    */
    public int getMetaFlags() {
        return bt_metaflags;
    }

    private int bt_nkeys;
    /**
    The number of keys or records in the database.
    <p>
    For the Btree Access Method, the number of keys in the database.  If
    the {@link com.sleepycat.db.Database#getStats Database.getStats} call was not configured by the
    {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method or the database was configured
    to support retrieval by record number, the count will be exact.
    Otherwise, the count will be the last saved value unless it has
    never been calculated, in which case it will be 0.
    <p>
    For the Recno Access Method, the number of records in the database.
    If the database was configured with mutable record numbers the count
    will be exact.  Otherwise, if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
    was configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method, the
    count will be exact but will include deleted records; if the
    {@link com.sleepycat.db.Database#getStats Database.getStats} call was not configured by the
    {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method, the count will be exact and
    will not include deleted records.
    */
    public int getNumKeys() {
        return bt_nkeys;
    }

    private int bt_ndata;
    /**
    The number of key/data pairs or records in the database.
    <p>
    For the Btree Access Method, the number of key/data pairs in the
    database.  If the {@link com.sleepycat.db.Database#getStats Database.getStats} call was not
    configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method, the count
    will be exact.  Otherwise, the count will be the last saved value
    unless it has never been calculated, in which case it will be 0.
    <p>
    For the Recno Access Method, the number of records in the database.
    If the database was configured with mutable record numbers, the
    count will be exact.  Otherwise, if the {@link com.sleepycat.db.Database#getStats Database.getStats}
    call was configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method, the
    count will be exact but will include deleted records; if the
    {@link com.sleepycat.db.Database#getStats Database.getStats} call was not configured by the
    {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method, the count will be exact and
    will not include deleted records.
    */
    public int getNumData() {
        return bt_ndata;
    }

    private int bt_pagecnt;
    /**
    The number of pages in the database.
    <p>
    Returned if {@link StatsConfig#setFast} was configured.
    */
    public int getPageCount() {
        return bt_pagecnt;
    }

    private int bt_pagesize;
    /**
    The underlying database page size, in bytes.
    */
    public int getPageSize() {
        return bt_pagesize;
    }

    private int bt_minkey;
    /**
    The minimum keys per page.
    */
    public int getMinKey() {
        return bt_minkey;
    }

    private int bt_nblobs;
    /**
    The number of blob records.
    */
    public int getNumBlobs() {
        return bt_nblobs;
    }

    private int bt_re_len;
    /**
    The length of fixed-length records.
    */
    public int getReLen() {
        return bt_re_len;
    }

    private int bt_re_pad;
    /**
    The padding byte value for fixed-length records.
    */
    public int getRePad() {
        return bt_re_pad;
    }

    private int bt_levels;
    /**
    The number of levels in the database.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public int getLevels() {
        return bt_levels;
    }

    private int bt_int_pg;
    /**
    The number of database internal pages.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public int getIntPages() {
        return bt_int_pg;
    }

    private int bt_leaf_pg;
    /**
    The number of database leaf pages.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public int getLeafPages() {
        return bt_leaf_pg;
    }

    private int bt_dup_pg;
    /**
    The number of database duplicate pages.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public int getDupPages() {
        return bt_dup_pg;
    }

    private int bt_over_pg;
    /**
    The number of database overflow pages.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public int getOverPages() {
        return bt_over_pg;
    }

    private int bt_empty_pg;
    /**
    The number of empty database pages.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public int getEmptyPages() {
        return bt_empty_pg;
    }

    private int bt_free;
    /**
    The number of pages on the free list.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public int getFree() {
        return bt_free;
    }

    private long bt_int_pgfree;
    /**
    The number of bytes free in database internal pages.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public long getIntPagesFree() {
        return bt_int_pgfree;
    }

    private long bt_leaf_pgfree;
    /**
    The number of bytes free in database leaf pages.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public long getLeafPagesFree() {
        return bt_leaf_pgfree;
    }

    private long bt_dup_pgfree;
    /**
    The number of bytes free in database duplicate pages.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public long getDupPagesFree() {
        return bt_dup_pgfree;
    }

    private long bt_over_pgfree;
    /**
    The number of bytes free in database overflow pages.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public long getOverPagesFree() {
        return bt_over_pgfree;
    }

    /**
    For convenience, the BtreeStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "BtreeStats:"
            + "\n  bt_magic=" + bt_magic
            + "\n  bt_version=" + bt_version
            + "\n  bt_metaflags=" + bt_metaflags
            + "\n  bt_nkeys=" + bt_nkeys
            + "\n  bt_ndata=" + bt_ndata
            + "\n  bt_pagecnt=" + bt_pagecnt
            + "\n  bt_pagesize=" + bt_pagesize
            + "\n  bt_minkey=" + bt_minkey
            + "\n  bt_nblobs=" + bt_nblobs
            + "\n  bt_re_len=" + bt_re_len
            + "\n  bt_re_pad=" + bt_re_pad
            + "\n  bt_levels=" + bt_levels
            + "\n  bt_int_pg=" + bt_int_pg
            + "\n  bt_leaf_pg=" + bt_leaf_pg
            + "\n  bt_dup_pg=" + bt_dup_pg
            + "\n  bt_over_pg=" + bt_over_pg
            + "\n  bt_empty_pg=" + bt_empty_pg
            + "\n  bt_free=" + bt_free
            + "\n  bt_int_pgfree=" + bt_int_pgfree
            + "\n  bt_leaf_pgfree=" + bt_leaf_pgfree
            + "\n  bt_dup_pgfree=" + bt_dup_pgfree
            + "\n  bt_over_pgfree=" + bt_over_pgfree
            ;
    }
}
