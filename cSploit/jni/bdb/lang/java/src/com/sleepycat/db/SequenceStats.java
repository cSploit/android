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
A SequenceStats object is used to return sequence statistics.
*/
public class SequenceStats {
    // no public constructor
    /* package */ SequenceStats() {}

    private long st_wait;
    /**
    The number of times a thread of control was forced to wait on the
    handle mutex.
    */
    public long getWait() {
        return st_wait;
    }

    private long st_nowait;
    /**
    The number of times that a thread of control was able to obtain handle
    mutex without waiting.
    */
    public long getNowait() {
        return st_nowait;
    }

    private long st_current;
    /**
    The current value of the sequence in the database.
    */
    public long getCurrent() {
        return st_current;
    }

    private long st_value;
    /**
    The current cached value of the sequence.
    */
    public long getValue() {
        return st_value;
    }

    private long st_last_value;
    /**
    The last cached value of the sequence.
    */
    public long getLastValue() {
        return st_last_value;
    }

    private long st_min;
    /**
    The minimum permitted value of the sequence.
    */
    public long getMin() {
        return st_min;
    }

    private long st_max;
    /**
    The maximum permitted value of the sequence.
    */
    public long getMax() {
        return st_max;
    }

    private int st_cache_size;
    /**
    The number of values that will be cached in this handle.
    */
    public int getCacheSize() {
        return st_cache_size;
    }

    private int st_flags;
    /**
    The flags value for the sequence.
    */
    public int getFlags() {
        return st_flags;
    }

    /** {@inheritDoc} */
    public String toString() {
        return "SequenceStats:"
            + "\n  st_wait=" + st_wait
            + "\n  st_nowait=" + st_nowait
            + "\n  st_current=" + st_current
            + "\n  st_value=" + st_value
            + "\n  st_last_value=" + st_last_value
            + "\n  st_min=" + st_min
            + "\n  st_max=" + st_max
            + "\n  st_cache_size=" + st_cache_size
            + "\n  st_flags=" + st_flags
            ;
    }
}
