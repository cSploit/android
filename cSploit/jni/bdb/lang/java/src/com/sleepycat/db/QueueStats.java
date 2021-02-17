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
The QueueStats object is used to return Queue database statistics.
*/
public class QueueStats extends DatabaseStats {
    // no public constructor
    /* package */ QueueStats() {}

    private int qs_magic;
    /**
    The magic number that identifies the file as a Queue file.
    */
    public int getMagic() {
        return qs_magic;
    }

    private int qs_version;
    /**
    The version of the Queue database.
    */
    public int getVersion() {
        return qs_version;
    }

    private int qs_metaflags;
    /**
    Reports internal flags. For internal use only.
    */
    public int getMetaFlags() {
        return qs_metaflags;
    }

    private int qs_nkeys;
    /**
    The number of records in the database.
    <p>
    If the {@link com.sleepycat.db.Database#getStats Database.getStats} call was configured by the
    {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method, the count will be the last
    saved value unless it has never been calculated, in which case it
    will be 0.
    */
    public int getNumKeys() {
        return qs_nkeys;
    }

    private int qs_ndata;
    /**
    The number of records in the database.
    <p>
    If the {@link com.sleepycat.db.Database#getStats Database.getStats} call was configured by the
    {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method, the count will be the last
    saved value unless it has never been calculated, in which case it
    will be 0.
    */
    public int getNumData() {
        return qs_ndata;
    }

    private int qs_pagesize;
    /**
    The underlying database page size, in bytes.
    */
    public int getPageSize() {
        return qs_pagesize;
    }

    private int qs_extentsize;
    /**
    The underlying database extent size, in pages.
    */
    public int getExtentSize() {
        return qs_extentsize;
    }

    private int qs_pages;
    /**
    The number of pages in the database.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public int getPages() {
        return qs_pages;
    }

    private int qs_re_len;
    /**
    The length of the records.
    */
    public int getReLen() {
        return qs_re_len;
    }

    private int qs_re_pad;
    /**
    The padding byte value for the records.
    */
    public int getRePad() {
        return qs_re_pad;
    }

    private int qs_pgfree;
    /**
    The number of bytes free in database pages.
<p>
The information is only included if the {@link com.sleepycat.db.Database#getStats Database.getStats} call
was not configured by the {@link com.sleepycat.db.StatsConfig#setFast StatsConfig.setFast} method.
    */
    public int getPagesFree() {
        return qs_pgfree;
    }

    private int qs_first_recno;
    /**
    The first undeleted record in the database.
    */
    public int getFirstRecno() {
        return qs_first_recno;
    }

    private int qs_cur_recno;
    /**
    The next available record number.
    */
    public int getCurRecno() {
        return qs_cur_recno;
    }

    /**
    For convenience, the QueueStats class has a toString method
    that lists all the data fields.
    */
    public String toString() {
        return "QueueStats:"
            + "\n  qs_magic=" + qs_magic
            + "\n  qs_version=" + qs_version
            + "\n  qs_metaflags=" + qs_metaflags
            + "\n  qs_nkeys=" + qs_nkeys
            + "\n  qs_ndata=" + qs_ndata
            + "\n  qs_pagesize=" + qs_pagesize
            + "\n  qs_extentsize=" + qs_extentsize
            + "\n  qs_pages=" + qs_pages
            + "\n  qs_re_len=" + qs_re_len
            + "\n  qs_re_pad=" + qs_re_pad
            + "\n  qs_pgfree=" + qs_pgfree
            + "\n  qs_first_recno=" + qs_first_recno
            + "\n  qs_cur_recno=" + qs_cur_recno
            ;
    }
}
