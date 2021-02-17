/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.bind.tuple;

import java.math.BigDecimal;
import java.math.BigInteger;

import com.sleepycat.util.FastOutputStream;
import com.sleepycat.util.PackedInteger;
import com.sleepycat.util.UtfOps;

/**
 * An <code>OutputStream</code> with <code>DataOutput</code>-like methods for
 * writing tuple fields.  It is used by <code>TupleBinding</code>.
 *
 * <p>This class has many methods that have the same signatures as methods in
 * the {@link java.io.DataOutput} interface.  The reason this class does not
 * implement {@link java.io.DataOutput} is because it would break the interface
 * contract for those methods because of data format differences.</p>
 *
 * @see <a href="package-summary.html#formats">Tuple Formats</a>
 *
 * @author Mark Hayes
 */
public class TupleOutput extends FastOutputStream {

    /**
     * We represent a null string as a single FF UTF character, which cannot
     * occur in a UTF encoded string.
     */
    static final int NULL_STRING_UTF_VALUE = ((byte) 0xFF);

    /**
     * Creates a tuple output object for writing a byte array of tuple data.
     */
    public TupleOutput() {

        super();
    }

    /**
     * Creates a tuple output object for writing a byte array of tuple data,
     * using a given buffer.  A new buffer will be allocated only if the number
     * of bytes needed is greater than the length of this buffer.  A reference
     * to the byte array will be kept by this object and therefore the byte
     * array should not be modified while this object is in use.
     *
     * @param buffer is the byte array to use as the buffer.
     */
    public TupleOutput(byte[] buffer) {

        super(buffer);
    }

    // --- begin DataOutput compatible methods ---

    /**
     * Writes the specified bytes to the buffer, converting each character to
     * an unsigned byte value.
     * Writes values that can be read using {@link TupleInput#readBytes}.
     *
     * @param val is the string containing the values to be written.
     * Only characters with values below 0x100 may be written using this
     * method, since the high-order 8 bits of all characters are discarded.
     *
     * @return this tuple output object.
     *
     * @throws NullPointerException if the val parameter is null.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeBytes(String val) {

        writeBytes(val.toCharArray());
        return this;
    }

    /**
     * Writes the specified characters to the buffer, converting each character
     * to a two byte unsigned value.
     * Writes values that can be read using {@link TupleInput#readChars}.
     *
     * @param val is the string containing the characters to be written.
     *
     * @return this tuple output object.
     *
     * @throws NullPointerException if the val parameter is null.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeChars(String val) {

        writeChars(val.toCharArray());
        return this;
    }

    /**
     * Writes the specified characters to the buffer, converting each character
     * to UTF format, and adding a null terminator byte.
     * Writes values that can be read using {@link TupleInput#readString()}.
     *
     * @param val is the string containing the characters to be written.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#stringFormats">String Formats</a>
     */
    public final TupleOutput writeString(String val) {

        if (val != null) {
            writeString(val.toCharArray());
        } else {
            writeFast(NULL_STRING_UTF_VALUE);
        }
        writeFast(0);
        return this;
    }

    /**
     * Writes a char (two byte) unsigned value to the buffer.
     * Writes values that can be read using {@link TupleInput#readChar}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeChar(int val) {

        writeFast((byte) (val >>> 8));
        writeFast((byte) val);
        return this;
    }

    /**
     * Writes a boolean (one byte) unsigned value to the buffer, writing one
     * if the value is true and zero if it is false.
     * Writes values that can be read using {@link TupleInput#readBoolean}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeBoolean(boolean val) {

        writeFast(val ? (byte)1 : (byte)0);
        return this;
    }

    /**
     * Writes an signed byte (one byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readByte}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeByte(int val) {

        writeUnsignedByte(val ^ 0x80);
        return this;
    }

    /**
     * Writes an signed short (two byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readShort}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeShort(int val) {

        writeUnsignedShort(val ^ 0x8000);
        return this;
    }

    /**
     * Writes an signed int (four byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readInt}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeInt(int val) {

        writeUnsignedInt(val ^ 0x80000000);
        return this;
    }

    /**
     * Writes an signed long (eight byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readLong}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeLong(long val) {

        writeUnsignedLong(val ^ 0x8000000000000000L);
        return this;
    }

    /**
     * Writes an unsorted float (four byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readFloat}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#floatFormats">Floating Point
     * Formats</a>
     */
    public final TupleOutput writeFloat(float val) {

        writeUnsignedInt(Float.floatToIntBits(val));
        return this;
    }

    /**
     * Writes an unsorted double (eight byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readDouble}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#floatFormats">Floating Point
     * Formats</a>
     */
    public final TupleOutput writeDouble(double val) {

        writeUnsignedLong(Double.doubleToLongBits(val));
        return this;
    }

    /**
     * Writes a sorted float (four byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readSortedFloat}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#floatFormats">Floating Point
     * Formats</a>
     */
    public final TupleOutput writeSortedFloat(float val) {

        int intVal = Float.floatToIntBits(val);
        intVal ^= (intVal < 0) ? 0xffffffff : 0x80000000;
        writeUnsignedInt(intVal);
        return this;
    }

    /**
     * Writes a sorted double (eight byte) value to the buffer.
     * Writes values that can be read using {@link TupleInput#readSortedDouble}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#floatFormats">Floating Point
     * Formats</a>
     */
    public final TupleOutput writeSortedDouble(double val) {

        long longVal = Double.doubleToLongBits(val);
        longVal ^= (longVal < 0) ? 0xffffffffffffffffL : 0x8000000000000000L;
        writeUnsignedLong(longVal);
        return this;
    }

    // --- end DataOutput compatible methods ---

    /**
     * Writes the specified bytes to the buffer, converting each character to
     * an unsigned byte value.
     * Writes values that can be read using {@link TupleInput#readBytes}.
     *
     * @param chars is the array of values to be written.
     * Only characters with values below 0x100 may be written using this
     * method, since the high-order 8 bits of all characters are discarded.
     *
     * @return this tuple output object.
     *
     * @throws NullPointerException if the chars parameter is null.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeBytes(char[] chars) {

        for (int i = 0; i < chars.length; i++) {
            writeFast((byte) chars[i]);
        }
        return this;
    }

    /**
     * Writes the specified characters to the buffer, converting each character
     * to a two byte unsigned value.
     * Writes values that can be read using {@link TupleInput#readChars}.
     *
     * @param chars is the array of characters to be written.
     *
     * @return this tuple output object.
     *
     * @throws NullPointerException if the chars parameter is null.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeChars(char[] chars) {

        for (int i = 0; i < chars.length; i++) {
            writeFast((byte) (chars[i] >>> 8));
            writeFast((byte) chars[i]);
        }
        return this;
    }

    /**
     * Writes the specified characters to the buffer, converting each character
     * to UTF format.
     * Writes values that can be read using {@link TupleInput#readString(int)}
     * or {@link TupleInput#readString(char[])}.
     *
     * @param chars is the array of characters to be written.
     *
     * @return this tuple output object.
     *
     * @throws NullPointerException if the chars parameter is null.
     *
     * @see <a href="package-summary.html#stringFormats">String Formats</a>
     */
    public final TupleOutput writeString(char[] chars) {

        if (chars.length == 0) return this;

        int utfLength = UtfOps.getByteLength(chars);

        makeSpace(utfLength);
        UtfOps.charsToBytes(chars, 0, getBufferBytes(), getBufferLength(),
                            chars.length);
        addSize(utfLength);
        return this;
    }

    /**
     * Writes an unsigned byte (one byte) value to the buffer.
     * Writes values that can be read using {@link
     * TupleInput#readUnsignedByte}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeUnsignedByte(int val) {

        writeFast(val);
        return this;
    }

    /**
     * Writes an unsigned short (two byte) value to the buffer.
     * Writes values that can be read using {@link
     * TupleInput#readUnsignedShort}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeUnsignedShort(int val) {

        writeFast((byte) (val >>> 8));
        writeFast((byte) val);
        return this;
    }

    /**
     * Writes an unsigned int (four byte) value to the buffer.
     * Writes values that can be read using {@link
     * TupleInput#readUnsignedInt}.
     *
     * @param val is the value to write to the buffer.
     *
     * @return this tuple output object.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeUnsignedInt(long val) {

        writeFast((byte) (val >>> 24));
        writeFast((byte) (val >>> 16));
        writeFast((byte) (val >>> 8));
        writeFast((byte) val);
        return this;
    }

    /**
     * This method is private since an unsigned long cannot be treated as
     * such in Java, nor converted to a BigInteger of the same value.
     */
    private final TupleOutput writeUnsignedLong(long val) {

        writeFast((byte) (val >>> 56));
        writeFast((byte) (val >>> 48));
        writeFast((byte) (val >>> 40));
        writeFast((byte) (val >>> 32));
        writeFast((byte) (val >>> 24));
        writeFast((byte) (val >>> 16));
        writeFast((byte) (val >>> 8));
        writeFast((byte) val);
        return this;
    }

    /**
     * Writes an unsorted packed integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writePackedInt(int val) {

        makeSpace(PackedInteger.MAX_LENGTH);

        int oldLen = getBufferLength();
        int newLen = PackedInteger.writeInt(getBufferBytes(), oldLen, val);

        addSize(newLen - oldLen);
        return this;
    }

    /**
     * Writes an unsorted packed long integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writePackedLong(long val) {

        makeSpace(PackedInteger.MAX_LONG_LENGTH);

        int oldLen = getBufferLength();
        int newLen = PackedInteger.writeLong(getBufferBytes(), oldLen, val);

        addSize(newLen - oldLen);
        return this;
    }

    /**
     * Writes a sorted packed integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeSortedPackedInt(int val) {

        makeSpace(PackedInteger.MAX_LENGTH);
        int oldLen = getBufferLength();
        int newLen = PackedInteger.writeSortedInt(getBufferBytes(), oldLen,
                                                  val);
        addSize(newLen - oldLen);
        return this;
    }

    /**
     * Writes a sorted packed long integer.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeSortedPackedLong(long val) {

        makeSpace(PackedInteger.MAX_LONG_LENGTH);

        int oldLen = getBufferLength();
        int newLen = PackedInteger.writeSortedLong(getBufferBytes(), oldLen,
                                                   val);

        addSize(newLen - oldLen);
        return this;
    }

    /**
     * Writes a {@code BigInteger}.
     *
     * @throws NullPointerException if val is null.
     *
     * @throws IllegalArgumentException if the byte array representation of val
     * is larger than 0x7fff bytes.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public final TupleOutput writeBigInteger(BigInteger val) {
    
        byte[] a = val.toByteArray();
        if (a.length > Short.MAX_VALUE) {
            throw new IllegalArgumentException
                ("BigInteger byte array is larger than 0x7fff bytes");
        }
        int firstByte = a[0];
        writeShort((firstByte < 0) ? (- a.length) : a.length);
        writeByte(firstByte);
        writeFast(a, 1, a.length - 1);
        return this;
    }

    /**
     * Returns the exact byte length that would would be output for a given
     * {@code BigInteger} value if {@link TupleOutput#writeBigInteger} were
     * called.
     *
     * @see <a href="package-summary.html#integerFormats">Integer Formats</a>
     */
    public static int getBigIntegerByteLength(BigInteger val) {
        return 2 /* length bytes */ +
               (val.bitLength() + 1 /* sign bit */ + 7 /* round up */) / 8;
    }
    
    /**
     * Writes an unsorted {@code BigDecimal}.
     *
     * @throws NullPointerException if val is null.
     *
     * @see <a href="package-summary.html#bigDecimalFormats">BigDecimal
     * Formats</a>
     */
    public final TupleOutput writeBigDecimal(BigDecimal val) {
    
        /*
         * The byte format for a BigDecimal value is:
         *     Byte 0 ~ L:   The scale part written as a PackedInteger.
         *     Byte L+1 ~ M: The length of the unscaled value written as a
         *                   PackedInteger.
         *     Byte M+1 ~ N: The BigDecimal.toByteArray array, written 
         *                   without modification.
         *
         * Get the scale and the unscaled value of this BigDecimal.
         */
        int scale = val.scale();
        BigInteger unscaledVal = val.unscaledValue();
        
        /* Store the scale. */
        writePackedInt(scale);
        byte[] a = unscaledVal.toByteArray();
        int len = a.length;
        
        /* Store the length of the following bytes. */
        writePackedInt(len);
        
        /* Store the bytes of the BigDecimal, without modification. */
        writeFast(a, 0, len);
        return this;
    }
    
    /**
     * Returns the maximum byte length that would be output for a given {@code
     * BigDecimal} value if {@link TupleOutput#writeBigDecimal} were called.
     *
     * @see <a href="package-summary.html#bigDecimalFormats">BigDecimal
     * Formats</a>
     */
    public static int getBigDecimalMaxByteLength(BigDecimal val) {

        BigInteger unscaledVal = val.unscaledValue();
        return PackedInteger.MAX_LENGTH * 2 + 
               unscaledVal.toByteArray().length;
    }
    
    /**
     * Writes a sorted {@code BigDecimal}.
     *
     * @see <a href="package-summary.html#bigDecimalFormats">BigDecimal
     * Formats</a>
     */
    public final TupleOutput writeSortedBigDecimal(BigDecimal val) {

        /* 
         * We have several options for the serialization of sorted BigDecimal.
         * The reason for choosing this method is that it is simpler and more
         * compact, and in some cases, comparison time will be less.  For other
         * methods and detailed discussion, please refer to [#18379].
         * 
         * First, we need to do the normalization, which means we normalize a
         * given BigDecimal into two parts: decimal part and the exponent part.
         * The decimal part contains one integer (non zero). For example,
         *      1234.56 will be normalized to 1.23456E3;
         *      123.4E100 will be normalized to 1.234E102;
         *      -123.4E-100 will be normalized to -1.234E-98.
         *
         * After the normalization, the byte format is:
         *     Byte 0: sign (-1 represents negative, 0 represents zero, and 1 
         *             represents positive).
         *     Byte 1 ~ 5: the exponent with sign, and written as a 
         *                 SortedPackedInteger value.
         *     Byte 6 ~ N: the normalized decimal part with sign.
         *
         * Get the scale and the unscaled value of this BigDecimal..
         */
        BigDecimal valNoTrailZeros = val.stripTrailingZeros();
        int scale = valNoTrailZeros.scale();
        BigInteger unscaledVal = valNoTrailZeros.unscaledValue();
        int sign = valNoTrailZeros.signum();
        
        /* Then do the normalization. */
        String unscaledValStr = unscaledVal.abs().toString();
        int normalizedScale = unscaledValStr.length() - 1;
        BigDecimal normalizedVal = new BigDecimal(unscaledVal, 
                                                  normalizedScale);
        int exponent = (normalizedScale - scale) * sign;
        
        /* Start serializing each part. */
        writeByte(sign);
        writeSortedPackedInt(exponent);
        writeSortedNormalizedBigDecimal(normalizedVal);
        return this;
    }
    
    /**
     * Writes a normalized {@code BigDecimal}.
     */
    private final TupleOutput writeSortedNormalizedBigDecimal(BigDecimal val) {

        /*
         * The byte format for a sorted normalized {@code BigDecimal} value is:
         *     Byte 0 ~ N: Store all digits with sign. Each 9 digits is 
         *                 regarded as one integer, and written as a
         *                 SortedPackedInteger value.  If there are not enough
         *                 9 digits, pad trailing zeros. Since we may pad
         *                 trailing zeros for serialization, when doing
         *                 de-serialization, we need to delete the trailing
         *                 zeros. In order to designate a special value as the
         *                 terminator byte, we set
         *                     val = (val < 0) ? (val - 1) : val.
         *     Byte N + 1: Terminator byte. The terminator byte is -1, and
         *                 written as a SortedPackedInteger value.
         */

        /* get the precision, scale and sign of the BigDecimal. */
        int precision = val.precision();
        int scale = val.scale();
        int sign = val.signum();
        
        /* Start the serialization of the whole digits. */
        String digitsStr = val.abs().toPlainString();
        
        /* 
         * The default capacity of a StringBuilder is 16 chars, which is 
         * enough to hold a group of digits having 9 digits.
         */
        StringBuilder groupDigits = new StringBuilder();
        for (int i = 0; i < digitsStr.length();) {
            char digit = digitsStr.charAt(i++);
            
            /* Ignore the decimal. */
            if (digit != '.') {
                groupDigits.append(digit);
            }
            
            /* 
             * For the last group of the digits, if there are not 9 digits, pad
             * trailing zeros.
             */
            if (i == digitsStr.length() && groupDigits.length() < 9) {
                final int insertLen = 9 - groupDigits.length();
                for (int k = 0; k < insertLen; k++) {
                    groupDigits.append("0");
                }
            }
            
            /* Group every 9 digits as an Integer. */            
            if (groupDigits.length() == 9) {
                int subVal = Integer.valueOf(groupDigits.toString());
                if (sign < 0) {
                    subVal = -subVal;
                }
                
                /* 
                 * Reset the sub-value, so the value -1 will be designated as
                 * the terminator byte.
                 */
                subVal = subVal < 0 ? subVal - 1 : subVal;
                writeSortedPackedInt(subVal);
                groupDigits.setLength(0);
            }
        }
        
        /* Write the terminator byte. */
        writeSortedPackedInt(-1);
        return this;
    }
    
    /**
     * Returns the maximum byte length that would be output for a given {@code
     * BigDecimal} value if {@link TupleOutput#writeSortedBigDecimal} were
     * called.
     *
     * @see <a href="package-summary.html#bigDecimalFormats">BigDecimal
     * Formats</a>
     */
    public static int getSortedBigDecimalMaxByteLength(BigDecimal val) {
    
        String digitsStr = val.stripTrailingZeros().unscaledValue().abs().
                           toString();
        
        int numOfGroups = (digitsStr.length() + 8 /* round up */) / 9;
        
        return 1 /* sign */ + 
               PackedInteger.MAX_LENGTH /* exponent */ +
               PackedInteger.MAX_LENGTH * numOfGroups /* all the digits */ +
               1; /* terminator byte */
    }
}
