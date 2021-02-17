/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.bind.tuple.test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.Arrays;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.sleepycat.bind.tuple.TupleBinding;
import com.sleepycat.bind.tuple.TupleInput;
import com.sleepycat.bind.tuple.TupleOutput;
import com.sleepycat.db.DatabaseEntry;

/**
 * @author Mark Hayes
 */
public class TupleFormatTest {

    private TupleInput in;
    private TupleOutput out;
    private DatabaseEntry buffer;

    @Before
    public void setUp() {

        buffer = new DatabaseEntry();
        out = new TupleOutput();
    }

    @After
    public void tearDown() {

        /* Ensure that GC can cleanup. */
        in = null;
        out = null;
        buffer = null;
    }

    private void copyOutputToInput() {

        TupleBinding.outputToEntry(out, buffer);
        assertEquals(out.size(), buffer.getSize());
        in = TupleBinding.entryToInput(buffer);
        assertEquals(in.available(), buffer.getSize());
        assertEquals(in.getBufferLength(), buffer.getSize());
    }

    private void stringTest(String val) {

        out.reset();
        out.writeString(val);
        assertEquals(val.length() + 1, out.size()); // assume 1-byte chars
        copyOutputToInput();
        assertEquals(val, in.readString());
        assertEquals(0, in.available());
    }

    @Test
    public void testString() {

        stringTest("");
        stringTest("a");
        stringTest("abc");

        out.reset();
        out.writeString("abc");
        out.writeString("defg");
        assertEquals(9, out.size());
        copyOutputToInput();
        assertEquals("abc", in.readString());
        assertEquals("defg", in.readString());
        assertEquals(0, in.available());

        out.reset();
        out.writeString("abc");
        out.writeString("defg");
        out.writeString("hijkl");
        assertEquals(15, out.size());
        copyOutputToInput();
        assertEquals("abc", in.readString());
        assertEquals("defg", in.readString());
        assertEquals("hijkl", in.readString());
        assertEquals(0, in.available());
    }

    private void fixedStringTest(char[] val) {

        out.reset();
        out.writeString(val);
        assertEquals(val.length, out.size()); // assume 1 byte chars
        copyOutputToInput();
        char[] val2 = new char[val.length];
        in.readString(val2);
        assertTrue(Arrays.equals(val, val2));
        assertEquals(0, in.available());
        in.reset();
        String val3 = in.readString(val.length);
        assertTrue(Arrays.equals(val, val3.toCharArray()));
        assertEquals(0, in.available());
    }

    @Test
    public void testFixedString() {

        fixedStringTest(new char[0]);
        fixedStringTest(new char[] {'a'});
        fixedStringTest(new char[] {'a', 'b', 'c'});

        out.reset();
        out.writeString(new char[] {'a', 'b', 'c'});
        out.writeString(new char[] {'d', 'e', 'f', 'g'});
        assertEquals(7, out.size());
        copyOutputToInput();
        assertEquals("abc", in.readString(3));
        assertEquals("defg", in.readString(4));
        assertEquals(0, in.available());

        out.reset();
        out.writeString(new char[] {'a', 'b', 'c'});
        out.writeString(new char[] {'d', 'e', 'f', 'g'});
        out.writeString(new char[] {'h', 'i', 'j', 'k', 'l'});
        assertEquals(12, out.size());
        copyOutputToInput();
        assertEquals("abc", in.readString(3));
        assertEquals("defg", in.readString(4));
        assertEquals("hijkl", in.readString(5));
        assertEquals(0, in.available());
    }

    @Test
    public void testNullString() {

        out.reset();
        out.writeString((String) null);
        assertEquals(2, out.size());
        copyOutputToInput();
        assertEquals(null, in.readString());
        assertEquals(0, in.available());

        out.reset();
        out.writeString((String) null);
        out.writeString("x");
        assertEquals(4, out.size());
        copyOutputToInput();
        assertEquals(null, in.readString());
        assertEquals(2, in.available());
        assertEquals("x", in.readString());
        assertEquals(0, in.available());

        out.reset();
        out.writeString("x");
        out.writeString((String) null);
        assertEquals(4, out.size());
        copyOutputToInput();
        assertEquals("x", in.readString());
        assertEquals(2, in.available());
        assertEquals(null, in.readString());
        assertEquals(0, in.available());

        out.reset();
        out.writeString((String) null);
        out.writeInt(123);
        assertEquals(6, out.size());
        copyOutputToInput();
        assertEquals(null, in.readString());
        assertEquals(4, in.available());
        assertEquals(123, in.readInt());
        assertEquals(0, in.available());

        out.reset();
        out.writeInt(123);
        out.writeString((String) null);
        assertEquals(6, out.size());
        copyOutputToInput();
        assertEquals(123, in.readInt());
        assertEquals(2, in.available());
        assertEquals(null, in.readString());
        assertEquals(0, in.available());
    }

    private void charsTest(char[] val) {

        for (int mode = 0; mode < 2; mode += 1) {
            out.reset();
            switch (mode) {
                case 0: out.writeChars(val); break;
                case 1: out.writeChars(new String(val)); break;
                default: throw new IllegalStateException();
            }
            assertEquals(val.length * 2, out.size());
            copyOutputToInput();
            char[] val2 = new char[val.length];
            in.readChars(val2);
            assertTrue(Arrays.equals(val, val2));
            assertEquals(0, in.available());
            in.reset();
            String val3 = in.readChars(val.length);
            assertTrue(Arrays.equals(val, val3.toCharArray()));
            assertEquals(0, in.available());
        }
    }

    @Test
    public void testChars() {

        charsTest(new char[0]);
        charsTest(new char[] {'a'});
        charsTest(new char[] {'a', 'b', 'c'});

        out.reset();
        out.writeChars("abc");
        out.writeChars("defg");
        assertEquals(7 * 2, out.size());
        copyOutputToInput();
        assertEquals("abc", in.readChars(3));
        assertEquals("defg", in.readChars(4));
        assertEquals(0, in.available());

        out.reset();
        out.writeChars("abc");
        out.writeChars("defg");
        out.writeChars("hijkl");
        assertEquals(12 * 2, out.size());
        copyOutputToInput();
        assertEquals("abc", in.readChars(3));
        assertEquals("defg", in.readChars(4));
        assertEquals("hijkl", in.readChars(5));
        assertEquals(0, in.available());
    }

    private void bytesTest(char[] val) {

        char[] valBytes = new char[val.length];
        for (int i = 0; i < val.length; i += 1)
            valBytes[i] = (char) (val[i] & 0xFF);

        for (int mode = 0; mode < 2; mode += 1) {
            out.reset();
            switch (mode) {
                case 0: out.writeBytes(val); break;
                case 1: out.writeBytes(new String(val)); break;
                default: throw new IllegalStateException();
            }
            assertEquals(val.length, out.size());
            copyOutputToInput();
            char[] val2 = new char[val.length];
            in.readBytes(val2);
            assertTrue(Arrays.equals(valBytes, val2));
            assertEquals(0, in.available());
            in.reset();
            String val3 = in.readBytes(val.length);
            assertTrue(Arrays.equals(valBytes, val3.toCharArray()));
            assertEquals(0, in.available());
        }
    }

    @Test
    public void testBytes() {

        bytesTest(new char[0]);
        bytesTest(new char[] {'a'});
        bytesTest(new char[] {'a', 'b', 'c'});
        bytesTest(new char[] {0x7F00, 0x7FFF, 0xFF00, 0xFFFF});

        out.reset();
        out.writeBytes("abc");
        out.writeBytes("defg");
        assertEquals(7, out.size());
        copyOutputToInput();
        assertEquals("abc", in.readBytes(3));
        assertEquals("defg", in.readBytes(4));
        assertEquals(0, in.available());

        out.reset();
        out.writeBytes("abc");
        out.writeBytes("defg");
        out.writeBytes("hijkl");
        assertEquals(12, out.size());
        copyOutputToInput();
        assertEquals("abc", in.readBytes(3));
        assertEquals("defg", in.readBytes(4));
        assertEquals("hijkl", in.readBytes(5));
        assertEquals(0, in.available());
    }

    private void booleanTest(boolean val) {

        out.reset();
        out.writeBoolean(val);
        assertEquals(1, out.size());
        copyOutputToInput();
        assertEquals(val, in.readBoolean());
        assertEquals(0, in.available());
    }

    @Test
    public void testBoolean() {

        booleanTest(true);
        booleanTest(false);

        out.reset();
        out.writeBoolean(true);
        out.writeBoolean(false);
        assertEquals(2, out.size());
        copyOutputToInput();
        assertEquals(true, in.readBoolean());
        assertEquals(false, in.readBoolean());
        assertEquals(0, in.available());

        out.reset();
        out.writeBoolean(true);
        out.writeBoolean(false);
        out.writeBoolean(true);
        assertEquals(3, out.size());
        copyOutputToInput();
        assertEquals(true, in.readBoolean());
        assertEquals(false, in.readBoolean());
        assertEquals(true, in.readBoolean());
        assertEquals(0, in.available());
    }

    private void unsignedByteTest(int val) {

        unsignedByteTest(val, val);
    }

    private void unsignedByteTest(int val, int expected) {

        out.reset();
        out.writeUnsignedByte(val);
        assertEquals(1, out.size());
        copyOutputToInput();
        assertEquals(expected, in.readUnsignedByte());
    }

    @Test
    public void testUnsignedByte() {

        unsignedByteTest(0);
        unsignedByteTest(1);
        unsignedByteTest(254);
        unsignedByteTest(255);
        unsignedByteTest(256, 0);
        unsignedByteTest(-1, 255);
        unsignedByteTest(-2, 254);
        unsignedByteTest(-255, 1);

        out.reset();
        out.writeUnsignedByte(0);
        out.writeUnsignedByte(1);
        out.writeUnsignedByte(255);
        assertEquals(3, out.size());
        copyOutputToInput();
        assertEquals(0, in.readUnsignedByte());
        assertEquals(1, in.readUnsignedByte());
        assertEquals(255, in.readUnsignedByte());
        assertEquals(0, in.available());
    }

    private void unsignedShortTest(int val) {

        unsignedShortTest(val, val);
    }

    private void unsignedShortTest(int val, int expected) {

        out.reset();
        out.writeUnsignedShort(val);
        assertEquals(2, out.size());
        copyOutputToInput();
        assertEquals(expected, in.readUnsignedShort());
    }

    @Test
    public void testUnsignedShort() {

        unsignedShortTest(0);
        unsignedShortTest(1);
        unsignedShortTest(255);
        unsignedShortTest(256);
        unsignedShortTest(257);
        unsignedShortTest(Short.MAX_VALUE - 1);
        unsignedShortTest(Short.MAX_VALUE);
        unsignedShortTest(Short.MAX_VALUE + 1);
        unsignedShortTest(0xFFFF - 1);
        unsignedShortTest(0xFFFF);
        unsignedShortTest(0xFFFF + 1, 0);
        unsignedShortTest(0x7FFF0000, 0);
        unsignedShortTest(0xFFFF0000, 0);
        unsignedShortTest(-1, 0xFFFF);
        unsignedShortTest(-2, 0xFFFF - 1);
        unsignedShortTest(-0xFFFF, 1);

        out.reset();
        out.writeUnsignedShort(0);
        out.writeUnsignedShort(1);
        out.writeUnsignedShort(0xFFFF);
        assertEquals(6, out.size());
        copyOutputToInput();
        assertEquals(0, in.readUnsignedShort());
        assertEquals(1, in.readUnsignedShort());
        assertEquals(0xFFFF, in.readUnsignedShort());
        assertEquals(0, in.available());
    }

    private void unsignedIntTest(long val) {

        unsignedIntTest(val, val);
    }

    private void unsignedIntTest(long val, long expected) {

        out.reset();
        out.writeUnsignedInt(val);
        assertEquals(4, out.size());
        copyOutputToInput();
        assertEquals(expected, in.readUnsignedInt());
    }

    @Test
    public void testUnsignedInt() {

        unsignedIntTest(0L);
        unsignedIntTest(1L);
        unsignedIntTest(255L);
        unsignedIntTest(256L);
        unsignedIntTest(257L);
        unsignedIntTest(Short.MAX_VALUE - 1L);
        unsignedIntTest(Short.MAX_VALUE);
        unsignedIntTest(Short.MAX_VALUE + 1L);
        unsignedIntTest(Integer.MAX_VALUE - 1L);
        unsignedIntTest(Integer.MAX_VALUE);
        unsignedIntTest(Integer.MAX_VALUE + 1L);
        unsignedIntTest(0xFFFFFFFFL - 1L);
        unsignedIntTest(0xFFFFFFFFL);
        unsignedIntTest(0xFFFFFFFFL + 1L, 0L);
        unsignedIntTest(0x7FFFFFFF00000000L, 0L);
        unsignedIntTest(0xFFFFFFFF00000000L, 0L);
        unsignedIntTest(-1, 0xFFFFFFFFL);
        unsignedIntTest(-2, 0xFFFFFFFFL - 1L);
        unsignedIntTest(-0xFFFFFFFFL, 1L);

        out.reset();
        out.writeUnsignedInt(0L);
        out.writeUnsignedInt(1L);
        out.writeUnsignedInt(0xFFFFFFFFL);
        assertEquals(12, out.size());
        copyOutputToInput();
        assertEquals(0L, in.readUnsignedInt());
        assertEquals(1L, in.readUnsignedInt());
        assertEquals(0xFFFFFFFFL, in.readUnsignedInt());
        assertEquals(0L, in.available());
    }

    private void byteTest(int val) {

        out.reset();
        out.writeByte(val);
        assertEquals(1, out.size());
        copyOutputToInput();
        assertEquals((byte) val, in.readByte());
    }

    @Test
    public void testByte() {

        byteTest(0);
        byteTest(1);
        byteTest(-1);
        byteTest(Byte.MAX_VALUE - 1);
        byteTest(Byte.MAX_VALUE);
        byteTest(Byte.MAX_VALUE + 1);
        byteTest(Byte.MIN_VALUE + 1);
        byteTest(Byte.MIN_VALUE);
        byteTest(Byte.MIN_VALUE - 1);
        byteTest(0x7F);
        byteTest(0xFF);
        byteTest(0x7FFF);
        byteTest(0xFFFF);
        byteTest(0x7FFFFFFF);
        byteTest(0xFFFFFFFF);

        out.reset();
        out.writeByte(0);
        out.writeByte(1);
        out.writeByte(-1);
        assertEquals(3, out.size());
        copyOutputToInput();
        assertEquals(0, in.readByte());
        assertEquals(1, in.readByte());
        assertEquals(-1, in.readByte());
        assertEquals(0, in.available());
    }

    private void shortTest(int val) {

        out.reset();
        out.writeShort(val);
        assertEquals(2, out.size());
        copyOutputToInput();
        assertEquals((short) val, in.readShort());
    }

    @Test
    public void testShort() {

        shortTest(0);
        shortTest(1);
        shortTest(-1);
        shortTest(Short.MAX_VALUE - 1);
        shortTest(Short.MAX_VALUE);
        shortTest(Short.MAX_VALUE + 1);
        shortTest(Short.MIN_VALUE + 1);
        shortTest(Short.MIN_VALUE);
        shortTest(Short.MIN_VALUE - 1);
        shortTest(0x7F);
        shortTest(0xFF);
        shortTest(0x7FFF);
        shortTest(0xFFFF);
        shortTest(0x7FFFFFFF);
        shortTest(0xFFFFFFFF);

        out.reset();
        out.writeShort(0);
        out.writeShort(1);
        out.writeShort(-1);
        assertEquals(3 * 2, out.size());
        copyOutputToInput();
        assertEquals(0, in.readShort());
        assertEquals(1, in.readShort());
        assertEquals(-1, in.readShort());
        assertEquals(0, in.available());
    }

    private void intTest(int val) {

        out.reset();
        out.writeInt(val);
        assertEquals(4, out.size());
        copyOutputToInput();
        assertEquals(val, in.readInt());
    }

    @Test
    public void testInt() {

        intTest(0);
        intTest(1);
        intTest(-1);
        intTest(Integer.MAX_VALUE - 1);
        intTest(Integer.MAX_VALUE);
        intTest(Integer.MAX_VALUE + 1);
        intTest(Integer.MIN_VALUE + 1);
        intTest(Integer.MIN_VALUE);
        intTest(Integer.MIN_VALUE - 1);
        intTest(0x7F);
        intTest(0xFF);
        intTest(0x7FFF);
        intTest(0xFFFF);
        intTest(0x7FFFFFFF);
        intTest(0xFFFFFFFF);

        out.reset();
        out.writeInt(0);
        out.writeInt(1);
        out.writeInt(-1);
        assertEquals(3 * 4, out.size());
        copyOutputToInput();
        assertEquals(0, in.readInt());
        assertEquals(1, in.readInt());
        assertEquals(-1, in.readInt());
        assertEquals(0, in.available());
    }

    private void longTest(long val) {

        out.reset();
        out.writeLong(val);
        assertEquals(8, out.size());
        copyOutputToInput();
        assertEquals(val, in.readLong());
    }

    @Test
    public void testLong() {

        longTest(0);
        longTest(1);
        longTest(-1);
        longTest(Long.MAX_VALUE - 1);
        longTest(Long.MAX_VALUE);
        longTest(Long.MAX_VALUE + 1);
        longTest(Long.MIN_VALUE + 1);
        longTest(Long.MIN_VALUE);
        longTest(Long.MIN_VALUE - 1);
        longTest(0x7F);
        longTest(0xFF);
        longTest(0x7FFF);
        longTest(0xFFFF);
        longTest(0x7FFFFFFF);
        longTest(0xFFFFFFFF);
        longTest(0x7FFFFFFFFFFFFFFFL);
        longTest(0xFFFFFFFFFFFFFFFFL);

        out.reset();
        out.writeLong(0);
        out.writeLong(1);
        out.writeLong(-1);
        assertEquals(3 * 8, out.size());
        copyOutputToInput();
        assertEquals(0, in.readLong());
        assertEquals(1, in.readLong());
        assertEquals(-1, in.readLong());
        assertEquals(0, in.available());
    }

    private void floatTest(double val) {

        out.reset();
        out.writeFloat((float) val);
        assertEquals(4, out.size());
        copyOutputToInput();
        if (Double.isNaN(val)) {
            assertTrue(Float.isNaN(in.readFloat()));
        } else {
            assertEquals((float) val, in.readFloat(), 0);
        }
    }

    @Test
    public void testFloat() {

        floatTest(0);
        floatTest(1);
        floatTest(-1);
        floatTest(1.0);
        floatTest(0.1);
        floatTest(-1.0);
        floatTest(-0.1);
        floatTest(Float.NaN);
        floatTest(Float.NEGATIVE_INFINITY);
        floatTest(Float.POSITIVE_INFINITY);
        floatTest(Short.MAX_VALUE);
        floatTest(Short.MIN_VALUE);
        floatTest(Integer.MAX_VALUE);
        floatTest(Integer.MIN_VALUE);
        floatTest(Long.MAX_VALUE);
        floatTest(Long.MIN_VALUE);
        floatTest(Float.MAX_VALUE);
        floatTest(Float.MAX_VALUE + 1);
        floatTest(Float.MIN_VALUE + 1);
        floatTest(Float.MIN_VALUE);
        floatTest(Float.MIN_VALUE - 1);
        floatTest(0x7F);
        floatTest(0xFF);
        floatTest(0x7FFF);
        floatTest(0xFFFF);
        floatTest(0x7FFFFFFF);
        floatTest(0xFFFFFFFF);
        floatTest(0x7FFFFFFFFFFFFFFFL);
        floatTest(0xFFFFFFFFFFFFFFFFL);

        out.reset();
        out.writeFloat(0);
        out.writeFloat(1);
        out.writeFloat(-1);
        assertEquals(3 * 4, out.size());
        copyOutputToInput();
        assertEquals(0, in.readFloat(), 0);
        assertEquals(1, in.readFloat(), 0);
        assertEquals(-1, in.readFloat(), 0);
        assertEquals(0, in.available(), 0);
    }

    private void doubleTest(double val) {

        out.reset();
        out.writeDouble(val);
        assertEquals(8, out.size());
        copyOutputToInput();
        if (Double.isNaN(val)) {
            assertTrue(Double.isNaN(in.readDouble()));
        } else {
            assertEquals(val, in.readDouble(), 0);
        }
    }

    @Test
    public void testDouble() {

        doubleTest(0);
        doubleTest(1);
        doubleTest(-1);
        doubleTest(1.0);
        doubleTest(0.1);
        doubleTest(-1.0);
        doubleTest(-0.1);
        doubleTest(Double.NaN);
        doubleTest(Double.NEGATIVE_INFINITY);
        doubleTest(Double.POSITIVE_INFINITY);
        doubleTest(Short.MAX_VALUE);
        doubleTest(Short.MIN_VALUE);
        doubleTest(Integer.MAX_VALUE);
        doubleTest(Integer.MIN_VALUE);
        doubleTest(Long.MAX_VALUE);
        doubleTest(Long.MIN_VALUE);
        doubleTest(Float.MAX_VALUE);
        doubleTest(Float.MIN_VALUE);
        doubleTest(Double.MAX_VALUE - 1);
        doubleTest(Double.MAX_VALUE);
        doubleTest(Double.MAX_VALUE + 1);
        doubleTest(Double.MIN_VALUE + 1);
        doubleTest(Double.MIN_VALUE);
        doubleTest(Double.MIN_VALUE - 1);
        doubleTest(0x7F);
        doubleTest(0xFF);
        doubleTest(0x7FFF);
        doubleTest(0xFFFF);
        doubleTest(0x7FFFFFFF);
        doubleTest(0xFFFFFFFF);
        doubleTest(0x7FFFFFFFFFFFFFFFL);
        doubleTest(0xFFFFFFFFFFFFFFFFL);

        out.reset();
        out.writeDouble(0);
        out.writeDouble(1);
        out.writeDouble(-1);
        assertEquals(3 * 8, out.size());
        copyOutputToInput();
        assertEquals(0, in.readDouble(), 0);
        assertEquals(1, in.readDouble(), 0);
        assertEquals(-1, in.readDouble(), 0);
        assertEquals(0, in.available(), 0);
    }

    private void sortedFloatTest(double val) {

        out.reset();
        out.writeSortedFloat((float) val);
        assertEquals(4, out.size());
        copyOutputToInput();
        if (Double.isNaN(val)) {
            assertTrue(Float.isNaN(in.readSortedFloat()));
        } else {
            assertEquals((float) val, in.readSortedFloat(), 0);
        }
    }

    @Test
    public void testSortedFloat() {

        sortedFloatTest(0);
        sortedFloatTest(1);
        sortedFloatTest(-1);
        sortedFloatTest(1.0);
        sortedFloatTest(0.1);
        sortedFloatTest(-1.0);
        sortedFloatTest(-0.1);
        sortedFloatTest(Float.NaN);
        sortedFloatTest(Float.NEGATIVE_INFINITY);
        sortedFloatTest(Float.POSITIVE_INFINITY);
        sortedFloatTest(Short.MAX_VALUE);
        sortedFloatTest(Short.MIN_VALUE);
        sortedFloatTest(Integer.MAX_VALUE);
        sortedFloatTest(Integer.MIN_VALUE);
        sortedFloatTest(Long.MAX_VALUE);
        sortedFloatTest(Long.MIN_VALUE);
        sortedFloatTest(Float.MAX_VALUE);
        sortedFloatTest(Float.MAX_VALUE + 1);
        sortedFloatTest(Float.MIN_VALUE + 1);
        sortedFloatTest(Float.MIN_VALUE);
        sortedFloatTest(Float.MIN_VALUE - 1);
        sortedFloatTest(0x7F);
        sortedFloatTest(0xFF);
        sortedFloatTest(0x7FFF);
        sortedFloatTest(0xFFFF);
        sortedFloatTest(0x7FFFFFFF);
        sortedFloatTest(0xFFFFFFFF);
        sortedFloatTest(0x7FFFFFFFFFFFFFFFL);
        sortedFloatTest(0xFFFFFFFFFFFFFFFFL);

        out.reset();
        out.writeSortedFloat(0);
        out.writeSortedFloat(1);
        out.writeSortedFloat(-1);
        assertEquals(3 * 4, out.size());
        copyOutputToInput();
        assertEquals(0, in.readSortedFloat(), 0);
        assertEquals(1, in.readSortedFloat(), 0);
        assertEquals(-1, in.readSortedFloat(), 0);
        assertEquals(0, in.available(), 0);
    }

    private void sortedDoubleTest(double val) {

        out.reset();
        out.writeSortedDouble(val);
        assertEquals(8, out.size());
        copyOutputToInput();
        if (Double.isNaN(val)) {
            assertTrue(Double.isNaN(in.readSortedDouble()));
        } else {
            assertEquals(val, in.readSortedDouble(), 0);
        }
    }

    @Test
    public void testSortedDouble() {

        sortedDoubleTest(0);
        sortedDoubleTest(1);
        sortedDoubleTest(-1);
        sortedDoubleTest(1.0);
        sortedDoubleTest(0.1);
        sortedDoubleTest(-1.0);
        sortedDoubleTest(-0.1);
        sortedDoubleTest(Double.NaN);
        sortedDoubleTest(Double.NEGATIVE_INFINITY);
        sortedDoubleTest(Double.POSITIVE_INFINITY);
        sortedDoubleTest(Short.MAX_VALUE);
        sortedDoubleTest(Short.MIN_VALUE);
        sortedDoubleTest(Integer.MAX_VALUE);
        sortedDoubleTest(Integer.MIN_VALUE);
        sortedDoubleTest(Long.MAX_VALUE);
        sortedDoubleTest(Long.MIN_VALUE);
        sortedDoubleTest(Float.MAX_VALUE);
        sortedDoubleTest(Float.MIN_VALUE);
        sortedDoubleTest(Double.MAX_VALUE - 1);
        sortedDoubleTest(Double.MAX_VALUE);
        sortedDoubleTest(Double.MAX_VALUE + 1);
        sortedDoubleTest(Double.MIN_VALUE + 1);
        sortedDoubleTest(Double.MIN_VALUE);
        sortedDoubleTest(Double.MIN_VALUE - 1);
        sortedDoubleTest(0x7F);
        sortedDoubleTest(0xFF);
        sortedDoubleTest(0x7FFF);
        sortedDoubleTest(0xFFFF);
        sortedDoubleTest(0x7FFFFFFF);
        sortedDoubleTest(0xFFFFFFFF);
        sortedDoubleTest(0x7FFFFFFFFFFFFFFFL);
        sortedDoubleTest(0xFFFFFFFFFFFFFFFFL);

        out.reset();
        out.writeSortedDouble(0);
        out.writeSortedDouble(1);
        out.writeSortedDouble(-1);
        assertEquals(3 * 8, out.size());
        copyOutputToInput();
        assertEquals(0, in.readSortedDouble(), 0);
        assertEquals(1, in.readSortedDouble(), 0);
        assertEquals(-1, in.readSortedDouble(), 0);
        assertEquals(0, in.available(), 0);
    }

    private void packedIntTest(int val, int size) {

        out.reset();
        out.writePackedInt(val);
        assertEquals(size, out.size());
        copyOutputToInput();
        assertEquals(size, in.getPackedIntByteLength());
        assertEquals(val, in.readPackedInt());
    }

    @Test
    public void testPackedInt() {

        /* Exhaustive value testing is in PackedIntTest. */
        packedIntTest(119, 1);
        packedIntTest(0xFFFF + 119, 3);
        packedIntTest(Integer.MAX_VALUE, 5);

        out.reset();
        out.writePackedInt(119);
        out.writePackedInt(0xFFFF + 119);
        out.writePackedInt(Integer.MAX_VALUE);
        assertEquals(1 + 3 + 5, out.size());
        copyOutputToInput();
        assertEquals(119, in.readPackedInt(), 0);
        assertEquals(0xFFFF + 119, in.readPackedInt(), 0);
        assertEquals(Integer.MAX_VALUE, in.readPackedInt(), 0);
        assertEquals(0, in.available(), 0);
    }

    private void packedLongTest(long val, int size) {

        out.reset();
        out.writePackedLong(val);
        assertEquals(size, out.size());
        copyOutputToInput();
        assertEquals(size, in.getPackedLongByteLength());
        assertEquals(val, in.readPackedLong());
    }

    @Test
    public void testPackedLong() {

        /* Exhaustive value testing is in PackedIntTest. */
        packedLongTest(119, 1);
        packedLongTest(0xFFFFFFFFL + 119, 5);
        packedLongTest(Long.MAX_VALUE, 9);

        out.reset();
        out.writePackedLong(119);
        out.writePackedLong(0xFFFFFFFFL + 119);
        out.writePackedLong(Long.MAX_VALUE);
        assertEquals(1 + 5 + 9, out.size());
        copyOutputToInput();
        assertEquals(119, in.readPackedLong(), 0);
        assertEquals(0xFFFFFFFFL + 119, in.readPackedLong(), 0);
        assertEquals(Long.MAX_VALUE, in.readPackedLong(), 0);
        assertEquals(0, in.available(), 0);
    }
    
    private void sortedPackedIntTest(int val, int size) {

        out.reset();
        out.writeSortedPackedInt(val);
        assertEquals(size, out.size());
        copyOutputToInput();
        assertEquals(size, in.getSortedPackedIntByteLength());
        assertEquals(val, in.readSortedPackedInt());
    }
    
    @Test
    public void testSortedPackedInt() {

        /* Exhaustive value testing is in sortedPackedIntTest. */
        sortedPackedIntTest(-1, 1);
        sortedPackedIntTest(0, 1);
        sortedPackedIntTest(1, 1);
        sortedPackedIntTest(-119, 1);
        sortedPackedIntTest(120, 1);
        sortedPackedIntTest(121, 2);
        sortedPackedIntTest(-120, 2);
        sortedPackedIntTest(0xFF + 121, 2);
        sortedPackedIntTest(0xFFFFFF00 - 119, 2);
        sortedPackedIntTest(0xFF + 122, 3);
        sortedPackedIntTest(0xFFFFFF00 - 120, 3);
        sortedPackedIntTest(0xFFFF + 121, 3);
        sortedPackedIntTest(0xFFFF0000 - 119, 3);
        sortedPackedIntTest(0xFFFF + 122, 4);
        sortedPackedIntTest(0xFFFF0000 - 120, 4);
        sortedPackedIntTest(0xFFFFFF + 121, 4);
        sortedPackedIntTest(0xFF000000 - 119, 4);
        sortedPackedIntTest(0xFFFFFF + 122, 5);
        sortedPackedIntTest(0xFF000000 - 120, 5);
        sortedPackedIntTest(Integer.MAX_VALUE - 1, 5);
        sortedPackedIntTest(Integer.MAX_VALUE, 5);
        sortedPackedIntTest(Integer.MAX_VALUE + 1, 5);
        sortedPackedIntTest(Integer.MIN_VALUE + 1, 5);
        sortedPackedIntTest(Integer.MIN_VALUE, 5);
        sortedPackedIntTest(Integer.MIN_VALUE - 1, 5);

        out.reset();
        out.writeSortedPackedInt(120);
        out.writeSortedPackedInt(0xFFFF + 121);
        out.writeSortedPackedInt(Integer.MAX_VALUE);
        assertEquals(1 + 3 + 5, out.size());
        copyOutputToInput();
        assertEquals(120, in.readSortedPackedInt(), 0);
        assertEquals(0xFFFF + 121, in.readSortedPackedInt(), 0);
        assertEquals(Integer.MAX_VALUE, in.readSortedPackedInt(), 0);
        assertEquals(0, in.available(), 0);
    }
    
    private void sortedPackedLongTest(long val, int size) {

        out.reset();
        out.writeSortedPackedLong(val);
        assertEquals(size, out.size());
        copyOutputToInput();
        assertEquals(size, in.getSortedPackedLongByteLength());
        assertEquals(val, in.readSortedPackedLong());
    }
    
    @Test
    public void testSortedPackedLong() {

        /* Exhaustive value testing is in sortedPackedLongTest. */
        sortedPackedLongTest(-1L, 1);
        sortedPackedLongTest(0L, 1);
        sortedPackedLongTest(1L, 1);
        sortedPackedLongTest(-119L, 1);
        sortedPackedLongTest(120L, 1);
        sortedPackedLongTest(121L, 2);
        sortedPackedLongTest(-120L, 2);
        sortedPackedLongTest(0xFFL + 121, 2);
        sortedPackedLongTest(0xFFFFFFFFFFFFFF00L - 119, 2);
        sortedPackedLongTest(0xFFL + 122, 3);
        sortedPackedLongTest(0xFFFFFFFFFFFFFF00L - 120, 3);
        sortedPackedLongTest(0xFFFFL + 121, 3);
        sortedPackedLongTest(0xFFFFFFFFFFFF0000L - 119, 3);
        sortedPackedLongTest(0xFFFFL + 122, 4);
        sortedPackedLongTest(0xFFFFFFFFFFFF0000L - 120, 4);
        sortedPackedLongTest(0xFFFFFFL + 121, 4);
        sortedPackedLongTest(0xFFFFFFFFFF000000L - 119, 4);
        sortedPackedLongTest(0xFFFFFFL + 122, 5);
        sortedPackedLongTest(0xFFFFFFFFFF000000L - 120, 5);
        sortedPackedLongTest(0xFFFFFFFFL + 121, 5);
        sortedPackedLongTest(0xFFFFFFFF00000000L - 119, 5);
        sortedPackedLongTest(0xFFFFFFFFL + 122, 6);
        sortedPackedLongTest(0xFFFFFFFF00000000L - 120, 6);
        sortedPackedLongTest(0xFFFFFFFFFFL + 121, 6);
        sortedPackedLongTest(0xFFFFFF0000000000L - 119, 6);
        sortedPackedLongTest(0xFFFFFFFFFFL + 122, 7);
        sortedPackedLongTest(0xFFFFFF0000000000L - 120, 7);
        sortedPackedLongTest(0xFFFFFFFFFFFFL + 121, 7);
        sortedPackedLongTest(0xFFFF000000000000L - 119, 7);
        sortedPackedLongTest(0xFFFFFFFFFFFFL + 122, 8);
        sortedPackedLongTest(0xFFFF000000000000L - 120, 8);
        sortedPackedLongTest(0xFFFFFFFFFFFFFFL + 121, 8);
        sortedPackedLongTest(0xFF00000000000000L - 119, 8);
        sortedPackedLongTest(Long.MAX_VALUE - 1, 9);
        sortedPackedLongTest(Long.MAX_VALUE, 9);
        sortedPackedLongTest(Long.MAX_VALUE + 1, 9);
        sortedPackedLongTest(Long.MIN_VALUE + 1, 9);
        sortedPackedLongTest(Long.MIN_VALUE, 9);
        sortedPackedLongTest(Long.MIN_VALUE - 1, 9);

        out.reset();
        out.writeSortedPackedLong(120);
        out.writeSortedPackedLong(0xFFFFFFL + 122);
        out.writeSortedPackedLong(Long.MAX_VALUE);
        assertEquals(1 + 5 + 9, out.size());
        copyOutputToInput();
        assertEquals(120L, in.readSortedPackedLong(), 0);
        assertEquals(0xFFFFFFL + 122, in.readSortedPackedLong(), 0);
        assertEquals(Long.MAX_VALUE, in.readSortedPackedLong(), 0);
        assertEquals(0, in.available(), 0);
    }
    
    private void bigIntegerTest(BigInteger val) {

        out.reset();
        out.writeBigInteger(val);
        int size = val.toByteArray().length + 2;
        assertEquals(size, out.size());
        copyOutputToInput();
        assertEquals(size, in.getBigIntegerByteLength());
        assertEquals(val, in.readBigInteger());
    }
    
    @Test
    public void testBigInteger() {

        /* Exhaustive value testing is in bigIntegerTest. */
        bigIntegerTest(BigInteger.valueOf(-1));
        bigIntegerTest(BigInteger.ZERO);
        bigIntegerTest(BigInteger.ONE);
        bigIntegerTest(BigInteger.TEN);
        bigIntegerTest(BigInteger.valueOf(Long.MIN_VALUE));
        bigIntegerTest(BigInteger.valueOf(Long.MAX_VALUE));
        bigIntegerTest(new BigInteger("-11111111111111111111"));
        bigIntegerTest(new BigInteger("11111111111111111111"));

        out.reset();
        out.writeBigInteger(BigInteger.ZERO);
        out.writeBigInteger(BigInteger.valueOf(Long.MAX_VALUE));
        out.writeBigInteger(BigInteger.valueOf(Long.MIN_VALUE));
        int size = BigInteger.ZERO.toByteArray().length + 2 +
            BigInteger.valueOf(Long.MAX_VALUE).toByteArray().length + 2 +
            BigInteger.valueOf(Long.MIN_VALUE).toByteArray().length + 2;
        assertEquals(size, out.size());
        copyOutputToInput();
        assertEquals(BigInteger.ZERO, in.readBigInteger());
        assertEquals(BigInteger.valueOf(Long.MAX_VALUE), in.readBigInteger());
        assertEquals(BigInteger.valueOf(Long.MIN_VALUE), in.readBigInteger());
        assertEquals(0, in.available());
    }
    
    private void bigDecimalTest(BigDecimal val, int scaleLen) {

        out.reset();
        out.writeBigDecimal(val);
        BigInteger unscaledVal = val.unscaledValue();
        int lenOfUnscaleValLen = 1;
        int unscaledValLen = unscaledVal.toByteArray().length;
        int size = scaleLen + lenOfUnscaleValLen + unscaledValLen;
        assertEquals(size, out.size());
        copyOutputToInput();
        assertEquals(size, in.getBigDecimalByteLength());
        assertEquals(val, in.readBigDecimal());
    }
    
    @Test
    public void testBigDecimal() {

        /* Exhaustive value testing is in BigDecimal. */
        bigDecimalTest(new BigDecimal("0.0"), 1);
        bigDecimalTest(new BigDecimal("0.123456789"), 1);
        bigDecimalTest(new BigDecimal("-0.123456789"), 1);
        bigDecimalTest(new BigDecimal("12300000000"), 1);
        bigDecimalTest(new BigDecimal("-123456789123456789.123456789123456789"), 
                       1);
        bigDecimalTest(new BigDecimal("123456789.123456789E700"), 3);
        bigDecimalTest(new BigDecimal(BigInteger.valueOf(Long.MAX_VALUE), 
                       Integer.MAX_VALUE), 5);
        bigDecimalTest(new BigDecimal(BigInteger.valueOf(Long.MIN_VALUE), 
                       Integer.MIN_VALUE), 5);
        bigDecimalTest(new BigDecimal("-11111111111111111111"), 1);
        bigDecimalTest(new BigDecimal("11111111111111111111"), 1);

        out.reset();
        BigDecimal bigDecimal1 = new BigDecimal("0.0");
        BigDecimal bigDecimal2 = new BigDecimal("123456789.123456789E700");
        BigDecimal bigDecimal3 = new 
            BigDecimal(BigInteger.valueOf(Long.MIN_VALUE), Integer.MIN_VALUE);
        out.writeBigDecimal(bigDecimal1);
        out.writeBigDecimal(bigDecimal2);
        out.writeBigDecimal(bigDecimal3);
        int size = bigDecimal1.unscaledValue().toByteArray().length + 1 + 1 +
                   bigDecimal2.unscaledValue().toByteArray().length + 1 + 3 +
                   bigDecimal3.unscaledValue().toByteArray().length + 1 + 5;
        assertEquals(size, out.size());
        copyOutputToInput();
        assertEquals(bigDecimal1, in.readBigDecimal());
        assertEquals(bigDecimal2, in.readBigDecimal());
        assertEquals(bigDecimal3, in.readBigDecimal());
        assertEquals(0, in.available());
    }
    
    private void sortedBigDecimalTest(BigDecimal val, int size) {

        out.reset();
        out.writeSortedBigDecimal(val);
        assertEquals(size, out.size());
        copyOutputToInput();
        assertEquals(size, in.getSortedBigDecimalByteLength());
        
        /* 
         * Because the precision of sorted BigDecimal cannot be preserved, we 
         * use compareTo rather than equals for the comparison. 
         */
        assertEquals(0, val.compareTo(in.readSortedBigDecimal()));
    }
    
    @Test
    public void testSortedBigDecimal() {

        /* Exhaustive value testing is in BigDecimal. */
        sortedBigDecimalTest(new BigDecimal("0"), 1 + 1 + 1 + 1);
        sortedBigDecimalTest(new BigDecimal("-9876543219876"), 
                             1 + 1 + 5 + 5 + 1);
        sortedBigDecimalTest(new BigDecimal("987654321987000"), 
                             1 + 1 + 5 + 5 + 1);
        sortedBigDecimalTest(new BigDecimal("-987654321987.654321987"), 
                             1 + 1 + 5 + 5 + 5 + 1);
        sortedBigDecimalTest(new BigDecimal("987654321.0000654"), 
                             1 + 1 + 5 + 3 + 1);
        sortedBigDecimalTest(new BigDecimal("-0.9876543219876"), 
                             1 + 1 + 5 + 5 + 1);
        sortedBigDecimalTest(new BigDecimal("9876.543210000000009876"), 
                             1 + 1 + 5 + 1 + 5 + 1);
        sortedBigDecimalTest(new BigDecimal("0.0000987654321000"), 
                             1 + 1 + 5 + 1);
        sortedBigDecimalTest(new BigDecimal("123456789.123456789E700"), 
                             1 + 3 + 5 + 5 + 1);
        sortedBigDecimalTest(new BigDecimal("-123456789.123456789E-67000"), 
                             1 + 4 + 5 + 5 + 1);

        out.reset();
        BigDecimal bigDecimal1 = new BigDecimal("0");
        BigDecimal bigDecimal2 = new BigDecimal("-9876543219876");
        BigDecimal bigDecimal3 = new BigDecimal("-987654321987.654321987");
        out.writeSortedBigDecimal(bigDecimal1);
        out.writeSortedBigDecimal(bigDecimal2);
        out.writeSortedBigDecimal(bigDecimal3);
        int size = 1 + 1 + 1 + 1 +
                   1 + 1 + 5 + 5 + 1 +
                   1 + 1 + 5 + 5 + 5 + 1;
        assertEquals(size, out.size());
        copyOutputToInput();
        
        /* 
         * Because the precision of sorted BigDecimal cannot be preserved, we 
         * use compareTo rather than equals for the comparison. 
         */
        assertEquals(0, bigDecimal1.compareTo(in.readSortedBigDecimal()));
        assertEquals(0, bigDecimal2.compareTo(in.readSortedBigDecimal()));
        assertEquals(0, bigDecimal3.compareTo(in.readSortedBigDecimal()));
        assertEquals(0, in.available());
    }
}
