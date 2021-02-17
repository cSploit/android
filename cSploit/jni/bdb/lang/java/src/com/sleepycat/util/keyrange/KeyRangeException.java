/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.util.keyrange;

/**
 * An exception thrown when a key is out of range.
 *
 * @author Mark Hayes
 */
public class KeyRangeException extends IllegalArgumentException {

    private static final long serialVersionUID = 1048575489L;

    /**
     * Creates a key range exception.
     */
    public KeyRangeException(String msg) {

        super(msg);
    }
}
