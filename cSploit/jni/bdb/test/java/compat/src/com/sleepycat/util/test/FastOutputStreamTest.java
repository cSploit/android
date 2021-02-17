/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.util.test;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

import com.sleepycat.util.FastOutputStream;

/**
 * @author Mark Hayes
 */
public class FastOutputStreamTest extends TestBase {

    @Test
    public void testBufferSizing() {
        FastOutputStream fos = new FastOutputStream();
        assertEquals
            (FastOutputStream.DEFAULT_INIT_SIZE, fos.getBufferBytes().length);

        /* Write X+1 bytes, expect array size 2X+1 */
        fos.write(new byte[FastOutputStream.DEFAULT_INIT_SIZE + 1]);
        assertEquals
            ((FastOutputStream.DEFAULT_INIT_SIZE * 2) + 1,
             fos.getBufferBytes().length);

        /* Write X+1 bytes, expect array size 4X+3 = (2(2X+1) + 1) */
        fos.write(new byte[FastOutputStream.DEFAULT_INIT_SIZE + 1]);
        assertEquals
            ((FastOutputStream.DEFAULT_INIT_SIZE * 4) + 3,
             fos.getBufferBytes().length);
    }
}
