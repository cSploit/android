/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.bind.tuple;

import java.math.BigDecimal;
import java.math.BigInteger;

import com.sleepycat.util.FastInputStream;
import com.sleepycat.util.PackedInteger;
import com.sleepycat.util.UtfOps;

/**
 * An <code>InputStream</code> with <code>DataInput</code>-like methods for
 * reading tuple fields.  It is used by <code>TupleBinding</code>.
 *
 * <p>This class has many methods that have the same signatures as methods in
 * the {@link java.io.DataInput} interface.  The reason this class does not
 * implement {@link java.io.DataInput} is because it would break the interface
 * contract for those methods because of data format differences.</p>
 *
 * @see <a href="package-summary.html#formats">Tuple Formats</a>
 *
 * @author Mark Hayes
 */
public class TupleInput extends FastInputStream {

    /**
     * Creates a tuple input object for reading a byte array of tuple data.  A
     * reference to the byte array will be kept by this object (it will not be
     * copied) and therefore the byte array should not be modified while this
     * object is in use.
     *
     * @param buffer is the byte array to be read and should contain data in
     * tuple format.
     */
    public TupleInput(byte[] buffer) {

        super(buffer);
    }

    /**
     * Creates a tuple input object for reading a byte array of tuple data at
     * a given offset for a given length.  A reference to the byte array will
     * be kept by this object (it will not be copied) and therefore the byte
     * array should not be modified while this object is in use.
     *
     * @param buffer is the byte array to be read and should contain data in
     * tuple format.
     *
     * @param offset is the byte offset at which to begin reading.
     *
     * @param length is the number of bytes to be read.
     */
    public TupleInput(byte[] buffer, int offset, int length) {

        super(buffer, offset, length);
    }

    /**
     * Creates a tuple input object from the data contained in a tuple output
     * object.  A reference to the tuple output's byte array will be kept by
     * this object (it will not be copied) and therefore the tuple output
     * object should not be modified while this object is in use.
     *
     * @param output is the tuple output object containing the data to be read.
     */
    public TupleInput(TupleOutput output) {

        super(output.getBufferBytes(), output.getBufferOffset(),
              output.getBufferLength());
    }

    // --- begin DataInput compatible methods ---

    /**
     * Reads a null-terminated UTF string from the data buffer and converts
     * the data from UTF to Unicode.
     * Reads values that were written using {@link
     * TupleOutput#writeString(String)}.
     *
     * @return the converted string.
     *
     * @throws IndexOutOfBoundsException if no null terminating byte is found
     * in the buffer.
     *
     * @throws IllegalArgumentException malformed UTF data is encountered.
     *
     * @see <a href="package-summary.html#stringFormats">String Formats</a>
     */
    public final String readString()
        throws IndexOutOfBoundsException, IllegalArgumentException  {

        byte[] myBuf = buf;
        int myOff = off;
        if (available() >= 2 &&
            myBuf[myOff] == TupleOutput.NULL_STRING_UTF_VALUE &&
            myBuf[myOff + 1] == 0) {
            skip(2);
            return null;
        } else {
            int byteLen = UtfOps.getZeroTerminatedByteLength(myBuf, myOff);
            skip(byteLen + 1);
            return UtfOps.bytesToString(myBuf, myOff, byteLen);
        }
    }

    /**
     * Reads a char (two byte) unsigned value from the buffer.
     * Reads values that were written using {@link TupleOutput#writeChar}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final char readChar()
        throws IndexOutOfBoundsException {

        return (char) readUnsignedShort();
    }

    /**
     * Reads a boolean (one byte) unsigned value from the buffer and returns
     * true if it is non-zero and false if it is zero.
     * Reads values that were written using {@link TupleOutput#writeBoolean}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final boolean readBoolean()
        throws IndexOutOfBoundsException {

        int c = readFast();
        if (c < 0) {
            throw new IndexOutOfBoundsException();
        }
        return (c != 0);
    }

    /**
     * Reads a signed byte (one byte) value from the buffer.
     * Reads values that were written using {@link TupleOutput#writeByte}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final byte readByte()
        throws IndexOutOfBoundsException {

        return (byte) (readUnsignedByte() ^ 0x80);
    }

    /**
     * Reads a signed short (two byte) value from the buffer.
     * Reads values that were written using {@link TupleOutput#writeShort}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final short readShort()
        throws IndexOutOfBoundsException {

        return (short) (readUnsignedShort() ^ 0x8000);
    }

    /**
     * Reads a signed int (four byte) value from the buffer.
     * Reads values that were written using {@link TupleOutput#writeInt}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final int readInt()
        throws IndexOutOfBoundsException {

        return (int) (readUnsignedInt() ^ 0x80000000);
    }

    /**
     * Reads a signed long (eight byte) value from the buffer.
     * Reads values that were written using {@link TupleOutput#writeLong}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final long readLong()
        throws IndexOutOfBoundsException {

        return readUnsignedLong() ^ 0x8000000000000000L;
    }

    /**
     * Reads an unsorted float (four byte) value from the buffer.
     * Reads values that were written using {@link TupleOutput#writeFloat}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#floatFormats">Floating Point
     * Formats</a>
     */
    public final float readFloat()
        throws IndexOutOfBoundsException {

        return Float.intBitsToFloat((int) readUnsignedInt());
    }

    /**
     * Reads an unsorted double (eight byte) value from the buffer.
     * Reads values that were written using {@link TupleOutput#writeDouble}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#floatFormats">Floating Point
     * Formats</a>
     */
    public final double readDouble()
        throws IndexOutOfBoundsException {

        return Double.longBitsToDouble(readUnsignedLong());
    }

    /**
     * Reads a sorted float (four byte) value from the buffer.
     * Reads values that were written using {@link
     * TupleOutput#writeSortedFloat}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#floatFormats">Floating Point
     * Formats</a>
     */
    public final float readSortedFloat()
        throws IndexOutOfBoundsException {

        int val = (int) readUnsignedInt();
        val ^= (val < 0) ? 0x80000000 : 0xffffffff;
        return Float.intBitsToFloat(val);
    }

    /**
     * Reads a sorted double (eight byte) value from the buffer.
     * Reads values that were written using {@link
     * TupleOutput#writeSortedDouble}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#floatFormats">Floating Point
     * Formats</a>
     */
    public final double readSortedDouble()
        throws IndexOutOfBoundsException {

        long val = readUnsignedLong();
        val ^= (val < 0) ? 0x8000000000000000L : 0xffffffffffffffffL;
        return Double.longBitsToDouble(val);
    }

    /**
     * Reads an unsigned byte (one byte) value from the buffer.
     * Reads values that were written using {@link
     * TupleOutput#writeUnsignedByte}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final int readUnsignedByte()
        throws IndexOutOfBoundsException {

        int c = readFast();
        if (c < 0) {
            throw new IndexOutOfBoundsException();
        }
        return c;
    }

    /**
     * Reads an unsigned short (two byte) value from the buffer.
     * Reads values that were written using {@link
     * TupleOutput#writeUnsignedShort}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final int readUnsignedShort()
        throws IndexOutOfBoundsException {

        int c1 = readFast();
        int c2 = readFast();
        if ((c1 | c2) < 0) {
             throw new IndexOutOfBoundsException();
        }
        return ((c1 << 8) | c2);
    }

    // --- end DataInput compatible methods ---

    /**
     * Reads an unsigned int (four byte) value from the buffer.
     * Reads values that were written using {@link
     * TupleOutput#writeUnsignedInt}.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final long readUnsignedInt()
        throws IndexOutOfBoundsException {

        long c1 = readFast();
        long c2 = readFast();
        long c3 = readFast();
        long c4 = readFast();
        if ((c1 | c2 | c3 | c4) < 0) {
            throw new IndexOutOfBoundsException();
        }
        return ((c1 << 24) | (c2 << 16) | (c3 << 8) | c4);
    }

    /**
     * This method is private since an unsigned long cannot be treated as
     * such in Java, nor converted to a BigInteger of the same value.
     */
    private final long readUnsignedLong()
        throws IndexOutOfBoundsException {

        long c1 = readFast();
        long c2 = readFast();
        long c3 = readFast();
        long c4 = readFast();
        long c5 = readFast();
        long c6 = readFast();
        long c7 = readFast();
        long c8 = readFast();
        if ((c1 | c2 | c3 | c4 | c5 | c6 | c7 | c8) < 0) {
             throw new IndexOutOfBoundsException();
        }
        return ((c1 << 56) | (c2 << 48) | (c3 << 40) | (c4 << 32) |
                (c5 << 24) | (c6 << 16) | (c7 << 8)  | c8);
    }

    /**
     * Reads the specified number of bytes from the buffer, converting each
     * unsigned byte value to a character of the resulting string.
     * Reads values that were written using {@link TupleOutput#writeBytes}.
     *
     * @param length is the number of bytes to be read.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final String readBytes(int length)
        throws IndexOutOfBoundsException {

        StringBuilder buf = new StringBuilder(length);
        for (int i = 0; i < length; i++) {
            int c = readFast();
            if (c < 0) {
                throw new IndexOutOfBoundsException();
            }
            buf.append((char) c);
        }
        return buf.toString();
    }

    /**
     * Reads the specified number of characters from the buffer, converting
     * each two byte unsigned value to a character of the resulting string.
     * Reads values that were written using {@link TupleOutput#writeChars}.
     *
     * @param length is the number of characters to be read.
     *
     * @return the value read from the buffer.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final String readChars(int length)
        throws IndexOutOfBoundsException {

        StringBuilder buf = new StringBuilder(length);
        for (int i = 0; i < length; i++) {
            buf.append(readChar());
        }
        return buf.toString();
    }

    /**
     * Reads the specified number of bytes from the buffer, converting each
     * unsigned byte value to a character of the resulting array.
     * Reads values that were written using {@link TupleOutput#writeBytes}.
     *
     * @param chars is the array to receive the data and whose length is used
     * to determine the number of bytes to be read.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final void readBytes(char[] chars)
        throws IndexOutOfBoundsException {

        for (int i = 0; i < chars.length; i++) {
            int c = readFast();
            if (c < 0) {
                throw new IndexOutOfBoundsException();
            }
            chars[i] = (char) c;
        }
    }

    /**
     * Reads the specified number of characters from the buffer, converting
     * each two byte unsigned value to a character of the resulting array.
     * Reads values that were written using {@link TupleOutput#writeChars}.
     *
     * @param chars is the array to receive the data and whose length is used
     * to determine the number of characters to be read.
     *
     * @throws IndexOutOfBoundsException if not enough bytes are available in
     * the buffer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final void readChars(char[] chars)
        throws IndexOutOfBoundsException {

        for (int i = 0; i < chars.length; i++) {
            chars[i] = readChar();
        }
    }

    /**
     * Reads the specified number of UTF characters string from the data
     * buffer and converts the data from UTF to Unicode.
     * Reads values that were written using {@link
     * TupleOutput#writeString(char[])}.
     *
     * @param length is the number of characters to be read.
     *
     * @return the converted string.
     *
     * @throws IndexOutOfBoundsException if no null terminating byte is found
     * in the buffer.
     *
     * @throws IllegalArgumentException malformed UTF data is encountered.
     *
     * @see <a href="package-summary.html#stringFormats">String Formats</a>
     */
    public final String readString(int length)
        throws IndexOutOfBoundsException, IllegalArgumentException  {

        char[] chars = new char[length];
        readString(chars);
        return new String(chars);
    }

    /**
     * Reads the specified number of UTF characters string from the data
     * buffer and converts the data from UTF to Unicode.
     * Reads values that were written using {@link
     * TupleOutput#writeString(char[])}.
     *
     * @param chars is the array to receive the data and whose length is used
     * to determine the number of characters to be read.
     *
     * @throws IndexOutOfBoundsException if no null terminating byte is found
     * in the buffer.
     *
     * @throws IllegalArgumentException malformed UTF data is encountered.
     *
     * @see <a href="package-summary.html#stringFormats">String Formats</a>
     */
    public final void readString(char[] chars)
        throws IndexOutOfBoundsException, IllegalArgumentException  {

        off = UtfOps.bytesToChars(buf, off, chars, 0, chars.length, false);
    }

    /**
     * Returns the byte length of a null-terminated UTF string in the data
     * buffer, including the terminator.  Used with string values that were
     * written using {@link TupleOutput#writeString(String)}.
     *
     * @throws IndexOutOfBoundsException if no null terminating byte is found
     * in the buffer.
     *
     * @throws IllegalArgumentException malformed UTF data is encountered.
     *
     * @see <a href="package-summary.html#stringFormats">String Formats</a>
     */
    public final int getStringByteLength()
        throws IndexOutOfBoundsException, IllegalArgumentException  {

        if (available() >= 2 &&
            buf[off] == TupleOutput.NULL_STRING_UTF_VALUE &&
            buf[off + 1] == 0) {
            return 2;
        } else {
            return UtfOps.getZeroTerminatedByteLength(buf, off) + 1;
        }
    }

    /**
     * Reads an unsorted packed integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final int readPackedInt() {

        int len = PackedInteger.getReadIntLength(buf, off);
        int val = PackedInteger.readInt(buf, off);

        off += len;
        return val;
    }

    /**
     * Returns the byte length of a packed integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final int getPackedIntByteLength() {
        return PackedInteger.getReadIntLength(buf, off);
    }

    /**
     * Reads an unsorted packed long integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final long readPackedLong() {

        int len = PackedInteger.getReadLongLength(buf, off);
        long val = PackedInteger.readLong(buf, off);

        off += len;
        return val;
    }

    /**
     * Returns the byte length of a packed long integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final int getPackedLongByteLength() {
        return PackedInteger.getReadLongLength(buf, off);
    }

    /**
     * Reads a sorted packed integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final int readSortedPackedInt() {

        int len = PackedInteger.getReadSortedIntLength(buf, off);
        int val = PackedInteger.readSortedInt(buf, off);

        off += len;
        return val;
    }

    /**
     * Returns the byte length of a sorted packed integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final int getSortedPackedIntByteLength() {
        return PackedInteger.getReadSortedIntLength(buf, off);
    }

    /**
     * Reads a sorted packed long integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final long readSortedPackedLong() {

        int len = PackedInteger.getReadSortedLongLength(buf, off);
        long val = PackedInteger.readSortedLong(buf, off);

        off += len;
        return val;
    }

    /**
     * Returns the byte length of a sorted packed long integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final int getSortedPackedLongByteLength() {
        return PackedInteger.getReadSortedLongLength(buf, off);
    }

    /**
     * Reads a {@code BigInteger}.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final BigInteger readBigInteger() {
    
        int len = readShort();
        if (len < 0) {
            len = (- len);
        }
        byte[] a = new byte[len];
        a[0] = readByte();
        readFast(a, 1, a.length - 1);
        return new BigInteger(a);
    }

    /**
     * Returns the byte length of a {@code BigInteger}.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final int getBigIntegerByteLength() {
    
        int saveOff = off;
        int len = readShort();
        off = saveOff;
        if (len < 0) {
            len = (- len);
        }
        return len + 2;
    }
    
    /**
     * Reads an unsorted {@code BigDecimal}.
     *
     * @see <a href="package-summary.html#bigDecimalFormats">BigDecimal
     * Formats</a>
     */
    public final BigDecimal readBigDecimal() {
    
        int scale = readPackedInt();
        int len = readPackedInt();
        byte[] a = new byte[len];
        readFast(a, 0, len);
        BigInteger unscaledVal = new BigInteger(a);
        return new BigDecimal(unscaledVal, scale);
    }
    
    /**
     * Returns the byte length of an unsorted {@code BigDecimal}.
     *
     * @see <a href="package-summary.html#bigDecimalFormats">BigDecimal
     * Formats</a>
     */
    public final int getBigDecimalByteLength() {
    
        /* First get the length of the scale. */
        int scaleLen = getPackedIntByteLength();
        int saveOff = off;
        off += scaleLen;
        
        /* 
         * Then get the length of the value which store the length of the 
         * following bytes. 
         */
        int lenOfUnscaleValLen = getPackedIntByteLength();
        
        /* Finally get the length of the following bytes. */
        int unscaledValLen = readPackedInt();
        off = saveOff;
        return scaleLen + lenOfUnscaleValLen + unscaledValLen;
    }
    
    /**
     * Reads a sorted {@code BigDecimal}, with support for correct default 
     * sorting.
     *
     * @see <a href="package-summary.html#bigDecimalFormats">BigDecimal
     * Formats</a>
     */
    public final BigDecimal readSortedBigDecimal() {    
        /* Get the sign of the BigDecimal. */
        int sign = readByte();
        
        /* Get the exponent of the BigDecimal. */
        int exponent = readSortedPackedInt();
        
        /*Get the normalized BigDecimal. */
        BigDecimal normalizedVal = readSortedNormalizedBigDecimal();
        
        /* 
         * After getting the normalized BigDecimal, we need to scale the value
         * with the exponent.
         */
        return normalizedVal.scaleByPowerOfTen(exponent * sign);
    }
    
    /**
     * Reads a sorted {@code BigDecimal} in normalized format with a single
     * digit to the left of the decimal point.
     */
    private final BigDecimal readSortedNormalizedBigDecimal() { 

        StringBuilder valStr = new StringBuilder(32);
        int subVal = readSortedPackedInt();
        int sign = subVal < 0 ? -1 : 1;
        
        /* Read through the buf, until we meet the terminator byte. */
        while (subVal != -1) {
        
            /* Adjust the sub-value back to the original. */
            subVal = subVal < 0 ? subVal + 1 : subVal;
            String groupDigits = Integer.toString(Math.abs(subVal));
        
            /* 
             * subVal < 100000000 means some leading zeros have been removed,
             * we have to add them back.
             */ 
            if (groupDigits.length() < 9) {
                final int insertLen = 9 - groupDigits.length();
                for (int i = 0; i < insertLen; i++) {
                    valStr.append("0");
                }
            }
            valStr.append(groupDigits);
            subVal = readSortedPackedInt();
        }

        BigInteger digitsVal = new BigInteger(valStr.toString());
        if (sign < 0) {
            digitsVal = digitsVal.negate();
        }
        /* The normalized decimal has 1 digits in the int part. */
        int scale = valStr.length() - 1;
        
        /* 
         * Since we may pad trailing zeros for serialization, when doing 
         * de-serialization, we need to delete the trailing zeros. 
         */
        return new BigDecimal(digitsVal, scale).stripTrailingZeros();
    }
    
    /**
     * Returns the byte length of a sorted {@code BigDecimal}.
     *
     * @see <a href="package-summary.html#bigDecimalFormats">BigDecimal
     * Formats</a>
     */
    public final int getSortedBigDecimalByteLength() {
    
        /* Save the original position, and read past the sigh byte. */
        int saveOff = off++;

        /* Get the length of the exponent. */
        int len = getSortedPackedIntByteLength(); /* the exponent */

        /* Skip to the digit part. */
        off += len;
        
        /* 
         * Travel through the following SortedPackedIntegers, until we meet the 
         * terminator byte. 
         */
        int subVal = readSortedPackedInt();
        while (subVal != -1) {
            subVal = readSortedPackedInt();
        }
        
        /* 
         * off is the value of end offset, while saveOff is the beginning 
         * offset.
         */
        len = off - saveOff;
        off = saveOff;
        return len;
    }
}
