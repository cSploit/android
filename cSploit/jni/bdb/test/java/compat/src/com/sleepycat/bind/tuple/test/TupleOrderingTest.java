/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.bind.tuple.test;

import static org.junit.Assert.fail;

import java.math.BigDecimal;
import java.math.BigInteger;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.sleepycat.bind.tuple.TupleOutput;

/**
 * @author Mark Hayes
 */
public class TupleOrderingTest {

    private TupleOutput out;
    private byte[] prevBuf;

    @Before
    public void setUp() {

        out = new TupleOutput();
        prevBuf = null;
    }

    @After
    public void tearDown() {

        /* Ensure that GC can cleanup. */
        out = null;
        prevBuf = null;
    }

    /**
     * Each tuple written must be strictly less than (by comparison of bytes)
     * the tuple written just before it.  The check() method compares bytes
     * just written to those written before the previous call to check().
     */
    private void check() {

        check(-1);
    }

    private void check(int dataIndex) {

        byte[] buf = new byte[out.size()];
        System.arraycopy(out.getBufferBytes(), out.getBufferOffset(),
                         buf, 0, buf.length);
        if (prevBuf != null) {
            int errOffset = -1;
            int len = Math.min(prevBuf.length, buf.length);
            boolean areEqual = true;
            for (int i = 0; i < len; i += 1) {
                int val1 = prevBuf[i] & 0xFF;
                int val2 = buf[i] & 0xFF;
                if (val1 < val2) {
                    areEqual = false;
                    break;
                } else if (val1 > val2) {
                    errOffset = i;
                    break;
                }
            }
            if (areEqual) {
                if (prevBuf.length < buf.length) {
                    areEqual = false;
                } else if (prevBuf.length > buf.length) {
                    areEqual = false;
                    errOffset = buf.length + 1;
                }
            }
            if (errOffset != -1 || areEqual) {
                StringBuilder msg = new StringBuilder();
                if (errOffset != -1) {
                    msg.append("Left >= right at byte offset " + errOffset);
                } else if (areEqual) {
                    msg.append("Bytes are equal");
                } else {
                    throw new IllegalStateException();
                }
                msg.append("\nLeft hex bytes: ");
                for (int i = 0; i < prevBuf.length; i += 1) {
                    msg.append(' ');
                    int val = prevBuf[i] & 0xFF;
                    if ((val & 0xF0) == 0) {
                        msg.append('0');
                    }
                    msg.append(Integer.toHexString(val));
                }
                msg.append("\nRight hex bytes:");
                for (int i = 0; i < buf.length; i += 1) {
                    msg.append(' ');
                    int val = buf[i] & 0xFF;
                    if ((val & 0xF0) == 0) {
                        msg.append('0');
                    }
                    msg.append(Integer.toHexString(val));
                }
                if (dataIndex >= 0) {
                    msg.append("\nData index: " + dataIndex);
                }
                fail(msg.toString());
            }
        }
        prevBuf = buf;
        out.reset();
    }

    private void reset() {

        prevBuf = null;
        out.reset();
    }

    @Test
    public void testString() {

        final String[] DATA = {
            "", "\u0001", "\u0002",
            "A", "a", "ab", "b", "bb", "bba",
            "c", "c\u0001", "d",
            new String(new char[] { 0x7F }),
            new String(new char[] { 0x7F, 0 }),
            new String(new char[] { 0xFF }),
            new String(new char[] { Character.MAX_VALUE }),
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeString(DATA[i]);
            check(i);
        }
        reset();
        out.writeString("a");
        check();
        out.writeString("a");
        out.writeString("");
        check();
        out.writeString("a");
        out.writeString("");
        out.writeString("a");
        check();
        out.writeString("a");
        out.writeString("b");
        check();
        out.writeString("aa");
        check();
        out.writeString("b");
        check();
    }

    @Test
    public void testFixedString() {

        final char[][] DATA = {
            {}, {'a'}, {'a', 'b'}, {'b'}, {'b', 'b'}, {0x7F}, {0xFF},
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeString(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testChars() {

        final char[][] DATA = {
            {}, {0}, {'a'}, {'a', 0}, {'a', 'b'}, {'b'}, {'b', 'b'},
            {0x7F}, {0x7F, 0}, {0xFF}, {0xFF, 0},
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeChars(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testBytes() {

        final char[][] DATA = {
            {}, {0}, {'a'}, {'a', 0}, {'a', 'b'}, {'b'}, {'b', 'b'},
            {0x7F}, {0xFF},
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeBytes(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testBoolean() {

        final boolean[] DATA = {
            false, true
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeBoolean(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testUnsignedByte() {

        final int[] DATA = {
            0, 1, 0x7F, 0xFF
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeUnsignedByte(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testUnsignedShort() {

        final int[] DATA = {
            0, 1, 0xFE, 0xFF, 0x800, 0x7FFF, 0xFFFF
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeUnsignedShort(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testUnsignedInt() {

        final long[] DATA = {
            0, 1, 0xFE, 0xFF, 0x800, 0x7FFF, 0xFFFF, 0x80000,
            0x7FFFFFFF, 0x80000000, 0xFFFFFFFF
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeUnsignedInt(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testByte() {

        final byte[] DATA = {
            Byte.MIN_VALUE, Byte.MIN_VALUE + 1,
            -1, 0, 1,
            Byte.MAX_VALUE - 1, Byte.MAX_VALUE,
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeByte(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testShort() {

        final short[] DATA = {
            Short.MIN_VALUE, Short.MIN_VALUE + 1,
            Byte.MIN_VALUE, Byte.MIN_VALUE + 1,
            -1, 0, 1,
            Byte.MAX_VALUE - 1, Byte.MAX_VALUE,
            Short.MAX_VALUE - 1, Short.MAX_VALUE,
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeShort(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testInt() {

        final int[] DATA = {
            Integer.MIN_VALUE, Integer.MIN_VALUE + 1,
            Short.MIN_VALUE, Short.MIN_VALUE + 1,
            Byte.MIN_VALUE, Byte.MIN_VALUE + 1,
            -1, 0, 1,
            Byte.MAX_VALUE - 1, Byte.MAX_VALUE,
            Short.MAX_VALUE - 1, Short.MAX_VALUE,
            Integer.MAX_VALUE - 1, Integer.MAX_VALUE,
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeInt(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testLong() {

        final long[] DATA = {
            Long.MIN_VALUE, Long.MIN_VALUE + 1,
            Integer.MIN_VALUE, Integer.MIN_VALUE + 1,
            Short.MIN_VALUE, Short.MIN_VALUE + 1,
            Byte.MIN_VALUE, Byte.MIN_VALUE + 1,
            -1, 0, 1,
            Byte.MAX_VALUE - 1, Byte.MAX_VALUE,
            Short.MAX_VALUE - 1, Short.MAX_VALUE,
            Integer.MAX_VALUE - 1, Integer.MAX_VALUE,
            Long.MAX_VALUE - 1, Long.MAX_VALUE,
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeLong(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testFloat() {

        // Only positive floats and doubles are ordered deterministically

        final float[] DATA = {
            0, Float.MIN_VALUE, 2 * Float.MIN_VALUE,
            (float) 0.01, (float) 0.02, (float) 0.99,
            1, (float) 1.01, (float) 1.02, (float) 1.99,
            Byte.MAX_VALUE - 1, Byte.MAX_VALUE,
            Short.MAX_VALUE - 1, Short.MAX_VALUE,
            Integer.MAX_VALUE,
            Long.MAX_VALUE / 2, Long.MAX_VALUE,
            Float.MAX_VALUE,
            Float.POSITIVE_INFINITY,
            Float.NaN,
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeFloat(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testDouble() {

        // Only positive floats and doubles are ordered deterministically

        final double[] DATA = {
            0, Double.MIN_VALUE, 2 * Double.MIN_VALUE,
            0.001, 0.002, 0.999,
            1, 1.001, 1.002, 1.999,
            Byte.MAX_VALUE - 1, Byte.MAX_VALUE,
            Short.MAX_VALUE - 1, Short.MAX_VALUE,
            Integer.MAX_VALUE - 1, Integer.MAX_VALUE,
            Long.MAX_VALUE / 2, Long.MAX_VALUE,
            Float.MAX_VALUE, Double.MAX_VALUE,
            Double.POSITIVE_INFINITY,
            Double.NaN,
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeDouble(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testSortedFloat() {

        final float[] DATA = {
            Float.NEGATIVE_INFINITY,
            (- Float.MAX_VALUE),
            Long.MIN_VALUE,
            Long.MIN_VALUE / 2,
            Integer.MIN_VALUE,
            Short.MIN_VALUE,
            Short.MIN_VALUE + 1,
            Byte.MIN_VALUE,
            Byte.MIN_VALUE + 1,
            (float) -1.99,
            (float) -1.02,
            (float) -1.01,
            -1,
            (float) -0.99,
            (float) -0.02,
            (float) -0.01,
            2 * (- Float.MIN_VALUE),
            (- Float.MIN_VALUE),
            0,
            Float.MIN_VALUE,
            2 * Float.MIN_VALUE,
            (float) 0.01,
            (float) 0.02,
            (float) 0.99,
            1,
            (float) 1.01,
            (float) 1.02,
            (float) 1.99,
            Byte.MAX_VALUE - 1,
            Byte.MAX_VALUE,
            Short.MAX_VALUE - 1,
            Short.MAX_VALUE,
            Integer.MAX_VALUE,
            Long.MAX_VALUE / 2,
            Long.MAX_VALUE,
            Float.MAX_VALUE,
            Float.POSITIVE_INFINITY,
            Float.NaN,
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeSortedFloat(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testSortedDouble() {

        final double[] DATA = {
            Double.NEGATIVE_INFINITY,
            (- Double.MAX_VALUE),
            (- Float.MAX_VALUE),
            Long.MIN_VALUE,
            Long.MIN_VALUE / 2,
            Integer.MIN_VALUE,
            Short.MIN_VALUE,
            Short.MIN_VALUE + 1,
            Byte.MIN_VALUE,
            Byte.MIN_VALUE + 1,
            -1.999,
            -1.002,
            -1.001,
            -1,
            -0.999,
            -0.002,
            -0.001,
            2 * (- Double.MIN_VALUE),
            (- Double.MIN_VALUE),
            0,
            Double.MIN_VALUE,
            2 * Double.MIN_VALUE,
            0.001,
            0.002,
            0.999,
            1,
            1.001,
            1.002,
            1.999,
            Byte.MAX_VALUE - 1,
            Byte.MAX_VALUE,
            Short.MAX_VALUE - 1,
            Short.MAX_VALUE,
            Integer.MAX_VALUE - 1,
            Integer.MAX_VALUE,
            Long.MAX_VALUE / 2,
            Long.MAX_VALUE,
            Float.MAX_VALUE,
            Double.MAX_VALUE,
            Double.POSITIVE_INFINITY,
            Double.NaN,
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeSortedDouble(DATA[i]);
            check(i);
        }
    }

    @Test
    public void testPackedIntAndLong() {
        /* Only packed int/long values from 0 to 630 are ordered correctly */
        for (int i = 0; i <= 630; i += 1) {
            out.writePackedInt(i);
            check(i);
        }
        reset();
        for (int i = 0; i <= 630; i += 1) {
            out.writePackedLong(i);
            check(i);
        }
    }
    
    @Test
    public void testSortedPackedInt() {
        final int[] DATA = {
            Integer.MIN_VALUE, Integer.MIN_VALUE + 1,
            Short.MIN_VALUE, Short.MIN_VALUE + 1,
            Byte.MIN_VALUE, Byte.MIN_VALUE + 1,
            -1, 0, 1,
            Byte.MAX_VALUE - 1, Byte.MAX_VALUE,
            Short.MAX_VALUE - 1, Short.MAX_VALUE,
            Integer.MAX_VALUE - 1, Integer.MAX_VALUE,
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeSortedPackedInt(DATA[i]);
            check(i);
        }
    }
    
    @Test
    public void testSortedPackedLong() {
        final long[] DATA = {
            Long.MIN_VALUE, Long.MIN_VALUE + 1,
            Integer.MIN_VALUE, Integer.MIN_VALUE + 1,
            Short.MIN_VALUE, Short.MIN_VALUE + 1,
            Byte.MIN_VALUE, Byte.MIN_VALUE + 1,
            -1, 0, 1,
            Byte.MAX_VALUE - 1, Byte.MAX_VALUE,
            Short.MAX_VALUE - 1, Short.MAX_VALUE,
            Integer.MAX_VALUE - 1, Integer.MAX_VALUE,
            Long.MAX_VALUE - 1, Long.MAX_VALUE,
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeSortedPackedLong(DATA[i]);
            check(i);
        }
    }
    
    @Test
    public void testBigInteger() {
        final BigInteger[] DATA = {
            new BigInteger("-1111111111111111111111111"),
            new BigInteger("-11111111111111111111"),
            BigInteger.valueOf(Long.MIN_VALUE), 
            BigInteger.valueOf(Long.MIN_VALUE + 1),
            BigInteger.valueOf(Integer.MIN_VALUE), 
            BigInteger.valueOf(Integer.MIN_VALUE + 1),
            BigInteger.valueOf(Short.MIN_VALUE), 
            BigInteger.valueOf(Short.MIN_VALUE + 1),
            BigInteger.valueOf(Byte.MIN_VALUE), 
            BigInteger.valueOf(Byte.MIN_VALUE + 1),
            BigInteger.valueOf(-1), 
            BigInteger.ZERO, BigInteger.ONE,
            BigInteger.valueOf(Byte.MAX_VALUE - 1), 
            BigInteger.valueOf(Byte.MAX_VALUE),
            BigInteger.valueOf(Short.MAX_VALUE - 1), 
            BigInteger.valueOf(Short.MAX_VALUE),
            BigInteger.valueOf(Integer.MAX_VALUE - 1), 
            BigInteger.valueOf(Integer.MAX_VALUE),
            BigInteger.valueOf(Long.MAX_VALUE - 1), 
            BigInteger.valueOf(Long.MAX_VALUE),
            new BigInteger("11111111111111111111"),
            new BigInteger("1111111111111111111111111"),
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeBigInteger(DATA[i]);
            check(i);
        }
    }
    
    @Test
    public void testSortedBigDecimal() {
        final BigDecimal[] DATA = {
            new BigDecimal(BigInteger.valueOf(Long.MIN_VALUE), 
                           Short.MIN_VALUE),
            new BigDecimal("-9999999999999999999.9999999999999999999"),
            new BigDecimal("-123456789.123456789"),
            new BigDecimal("-0.9999999999999999999999999999999999999"),
            new BigDecimal("-123456789.123456789E-700"),
            new BigDecimal("-123456789.123456789E-6700"),
            new BigDecimal(BigInteger.valueOf(Long.MIN_VALUE), 
                           Short.MAX_VALUE),
            new BigDecimal("0.0"),
            new BigDecimal(BigInteger.valueOf(Long.MAX_VALUE), 
                           Short.MAX_VALUE),
            new BigDecimal("0.9999999999999999999999999999999999999"),
            new BigDecimal("123456789.123456789"),
            new BigDecimal("9999999999999999999.9999999999999999999"),
            new BigDecimal("123456789.123456789E700"),
            new BigDecimal("123456789.123456789E6700"),
            new BigDecimal(BigInteger.valueOf(Long.MAX_VALUE), 
                           Short.MIN_VALUE),
        };
        for (int i = 0; i < DATA.length; i += 1) {
            out.writeSortedBigDecimal(DATA[i]);
            check(i);
        }
    }
}
