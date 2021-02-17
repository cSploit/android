/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import java.lang.reflect.Field;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.Date;
import java.util.Map;
import java.util.Set;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.persist.model.EntityModel;

/**
 * Format for simple types, including primitives.  Additional methods are
 * included to optimize the handling of primitives.  Other classes such as
 * PrimitiveArrayFormat and ReflectAccessor take advantage of these methods.
 *
 * @author Mark Hayes
 */
public abstract class SimpleFormat extends Format {

    private static final long serialVersionUID = 4595245575868697702L;

    private final boolean primitive;
    private SimpleFormat wrapperFormat;

    SimpleFormat(Catalog catalog, Class type, boolean primitive) {
        super(catalog, type);
        this.primitive = primitive;
    }

    void setWrapperFormat(SimpleFormat wrapperFormat) {
        this.wrapperFormat = wrapperFormat;
    }

    @Override
    Format getWrapperFormat() {
        return wrapperFormat;
    }

    @Override
    public boolean isSimple() {
        return true;
    }

    @Override
    public boolean isPrimitive() {
        return primitive;
    }

    @Override
    void collectRelatedFormats(Catalog catalog,
                               Map<String, Format> newFormats) {
    }

    @Override
    void initialize(Catalog catalog, EntityModel model, int initVersion) {
    }

    @Override
    public Object readObject(Object o, EntityInput input, boolean rawAccess) {
        /* newInstance reads the value -- do nothing here. */
        return o;
    }

    @Override
    boolean evolve(Format newFormat, Evolver evolver) {
        evolver.useOldFormat(this, newFormat);
        return true;
    }

    /* -- Begin methods to be overridden by primitive formats only. -- */

    Object newPrimitiveArray(int len, EntityInput input)
        throws RefreshException {

        throw DbCompat.unexpectedState();
    }

    void writePrimitiveArray(Object o, EntityOutput output) {
        throw DbCompat.unexpectedState();
    }

    int getPrimitiveLength() {
        throw DbCompat.unexpectedState();
    }

    /**
     * @throws IllegalAccessException from subclasses.
     */
    void readPrimitiveField(Object o, EntityInput input, Field field)
        throws IllegalAccessException, RefreshException {

        throw DbCompat.unexpectedState();
    }

    /**
     * @throws IllegalAccessException from subclasses.
     */
    void writePrimitiveField(Object o, EntityOutput output, Field field)
        throws IllegalAccessException {

        throw DbCompat.unexpectedState();
    }

    /* -- End methods to be overridden by primitive formats only. -- */

    void skipPrimitiveArray(int len, RecordInput input) {
        input.skipFast(len * getPrimitiveLength());
    }

    void copySecMultiKeyPrimitiveArray(int len,
                                       RecordInput input,
                                       Set results) {
        int primLen = getPrimitiveLength();
        for (int i = 0; i < len; i += 1) {
            DatabaseEntry entry = new DatabaseEntry
                (input.getBufferBytes(), input.getBufferOffset(), primLen);
            results.add(entry);
            input.skipFast(primLen);
        }
    }

    public static class FBool extends SimpleFormat {

        private static final long serialVersionUID = -7724949525068533451L;

        FBool(Catalog catalog, boolean primitive) {
            super(catalog, primitive ? Boolean.TYPE : Boolean.class,
                  primitive);
        }

        @Override
        Object newArray(int len) {
            return new Boolean[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess)
            throws RefreshException {

            return Boolean.valueOf(input.readBoolean());
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeBoolean(((Boolean) o).booleanValue());
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(1);
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            output.writeFast(input.readFast());
        }

        @Override
        Object newPrimitiveArray(int len, EntityInput input)
            throws RefreshException {

            boolean[] a = new boolean[len];
            for (int i = 0; i < len; i += 1) {
                a[i] = input.readBoolean();
            }
            return a;
        }

        @Override
        void writePrimitiveArray(Object o, EntityOutput output) {
            boolean[] a = (boolean[]) o;
            int len = a.length;
            output.writeArrayLength(len);
            for (int i = 0; i < len; i += 1) {
                output.writeBoolean(a[i]);
            }
        }

        @Override
        int getPrimitiveLength() {
            return 1;
        }

        @Override
        void readPrimitiveField(Object o, EntityInput input, Field field)
            throws IllegalAccessException, RefreshException {

            field.setBoolean(o, input.readBoolean());
        }

        @Override
        void writePrimitiveField(Object o, EntityOutput output, Field field)
            throws IllegalAccessException {

            output.writeBoolean(field.getBoolean(o));
        }
    }

    public static class FByte extends SimpleFormat {

        private static final long serialVersionUID = 3651752958101447257L;

        FByte(Catalog catalog, boolean primitive) {
            super(catalog, primitive ? Byte.TYPE : Byte.class, primitive);
        }

        @Override
        Object newArray(int len) {
            return new Byte[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess)
            throws RefreshException {

            return Byte.valueOf(input.readByte());
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeByte(((Number) o).byteValue());
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(1);
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            output.writeFast(input.readFast());
        }

        @Override
        Object newPrimitiveArray(int len, EntityInput input)
            throws RefreshException {

            byte[] a = new byte[len];
            for (int i = 0; i < len; i += 1) {
                a[i] = input.readByte();
            }
            return a;
        }

        @Override
        void writePrimitiveArray(Object o, EntityOutput output) {
            byte[] a = (byte[]) o;
            int len = a.length;
            output.writeArrayLength(len);
            for (int i = 0; i < len; i += 1) {
                output.writeByte(a[i]);
            }
        }

        @Override
        int getPrimitiveLength() {
            return 1;
        }

        @Override
        void readPrimitiveField(Object o, EntityInput input, Field field)
            throws IllegalAccessException, RefreshException {

            field.setByte(o, input.readByte());
        }

        @Override
        void writePrimitiveField(Object o, EntityOutput output, Field field)
            throws IllegalAccessException {

            output.writeByte(field.getByte(o));
        }

        @Override
        Format getSequenceKeyFormat() {
            return this;
        }
    }

    public static class FShort extends SimpleFormat {

        private static final long serialVersionUID = -4909138198491785624L;

        FShort(Catalog catalog, boolean primitive) {
            super(catalog, primitive ? Short.TYPE : Short.class, primitive);
        }

        @Override
        Object newArray(int len) {
            return new Short[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess)
            throws RefreshException {

            return Short.valueOf(input.readShort());
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeShort(((Number) o).shortValue());
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(2);
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            output.writeFast(input.readFast());
            output.writeFast(input.readFast());
        }

        @Override
        Object newPrimitiveArray(int len, EntityInput input)
            throws RefreshException {

            short[] a = new short[len];
            for (int i = 0; i < len; i += 1) {
                a[i] = input.readShort();
            }
            return a;
        }

        @Override
        void writePrimitiveArray(Object o, EntityOutput output) {
            short[] a = (short[]) o;
            int len = a.length;
            output.writeArrayLength(len);
            for (int i = 0; i < len; i += 1) {
                output.writeShort(a[i]);
            }
        }

        @Override
        int getPrimitiveLength() {
            return 2;
        }

        @Override
        void readPrimitiveField(Object o, EntityInput input, Field field)
            throws IllegalAccessException, RefreshException {

            field.setShort(o, input.readShort());
        }

        @Override
        void writePrimitiveField(Object o, EntityOutput output, Field field)
            throws IllegalAccessException {

            output.writeShort(field.getShort(o));
        }

        @Override
        Format getSequenceKeyFormat() {
            return this;
        }
    }

    public static class FInt extends SimpleFormat {

        private static final long serialVersionUID = 2695910006049980013L;

        FInt(Catalog catalog, boolean primitive) {
            super(catalog, primitive ? Integer.TYPE : Integer.class,
                  primitive);
        }

        @Override
        Object newArray(int len) {
            return new Integer[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess)
            throws RefreshException {

            return Integer.valueOf(input.readInt());
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeInt(((Number) o).intValue());
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(4);
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            output.writeFast(input.readFast());
            output.writeFast(input.readFast());
            output.writeFast(input.readFast());
            output.writeFast(input.readFast());
        }

        @Override
        Object newPrimitiveArray(int len, EntityInput input)
            throws RefreshException {

            int[] a = new int[len];
            for (int i = 0; i < len; i += 1) {
                a[i] = input.readInt();
            }
            return a;
        }

        @Override
        void writePrimitiveArray(Object o, EntityOutput output) {
            int[] a = (int[]) o;
            int len = a.length;
            output.writeArrayLength(len);
            for (int i = 0; i < len; i += 1) {
                output.writeInt(a[i]);
            }
        }

        @Override
        int getPrimitiveLength() {
            return 4;
        }

        @Override
        void readPrimitiveField(Object o, EntityInput input, Field field)
            throws IllegalAccessException, RefreshException {

            field.setInt(o, input.readInt());
        }

        @Override
        void writePrimitiveField(Object o, EntityOutput output, Field field)
            throws IllegalAccessException {

            output.writeInt(field.getInt(o));
        }

        @Override
        Format getSequenceKeyFormat() {
            return this;
        }
    }

    public static class FLong extends SimpleFormat {

        private static final long serialVersionUID = 1872661106534776520L;

        FLong(Catalog catalog, boolean primitive) {
            super(catalog, primitive ? Long.TYPE : Long.class, primitive);
        }

        @Override
        Object newArray(int len) {
            return new Long[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess)
            throws RefreshException {

            return Long.valueOf(input.readLong());
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeLong(((Number) o).longValue());
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(8);
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            output.writeFast
                (input.getBufferBytes(), input.getBufferOffset(), 8);
            input.skipFast(8);
        }

        @Override
        Object newPrimitiveArray(int len, EntityInput input)
            throws RefreshException {

            long[] a = new long[len];
            for (int i = 0; i < len; i += 1) {
                a[i] = input.readLong();
            }
            return a;
        }

        @Override
        void writePrimitiveArray(Object o, EntityOutput output) {
            long[] a = (long[]) o;
            int len = a.length;
            output.writeArrayLength(len);
            for (int i = 0; i < len; i += 1) {
                output.writeLong(a[i]);
            }
        }

        @Override
        int getPrimitiveLength() {
            return 8;
        }

        @Override
        void readPrimitiveField(Object o, EntityInput input, Field field)
            throws IllegalAccessException, RefreshException {

            field.setLong(o, input.readLong());
        }

        @Override
        void writePrimitiveField(Object o, EntityOutput output, Field field)
            throws IllegalAccessException {

            output.writeLong(field.getLong(o));
        }

        @Override
        Format getSequenceKeyFormat() {
            return this;
        }
    }

    public static class FFloat extends SimpleFormat {

        private static final long serialVersionUID = 1033413049495053602L;

        FFloat(Catalog catalog, boolean primitive) {
            super(catalog, primitive ? Float.TYPE : Float.class, primitive);
        }

        @Override
        Object newArray(int len) {
            return new Float[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess)
            throws RefreshException {

            return Float.valueOf(input.readSortedFloat());
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeSortedFloat(((Number) o).floatValue());
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(4);
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            output.writeFast(input.readFast());
            output.writeFast(input.readFast());
            output.writeFast(input.readFast());
            output.writeFast(input.readFast());
        }

        @Override
        Object newPrimitiveArray(int len, EntityInput input)
            throws RefreshException {

            float[] a = new float[len];
            for (int i = 0; i < len; i += 1) {
                a[i] = input.readSortedFloat();
            }
            return a;
        }

        @Override
        void writePrimitiveArray(Object o, EntityOutput output) {
            float[] a = (float[]) o;
            int len = a.length;
            output.writeArrayLength(len);
            for (int i = 0; i < len; i += 1) {
                output.writeSortedFloat(a[i]);
            }
        }

        @Override
        int getPrimitiveLength() {
            return 4;
        }

        @Override
        void readPrimitiveField(Object o, EntityInput input, Field field)
            throws IllegalAccessException, RefreshException {

            field.setFloat(o, input.readSortedFloat());
        }

        @Override
        void writePrimitiveField(Object o, EntityOutput output, Field field)
            throws IllegalAccessException {

            output.writeSortedFloat(field.getFloat(o));
        }
    }

    public static class FDouble extends SimpleFormat {

        private static final long serialVersionUID = 646904456811041423L;

        FDouble(Catalog catalog, boolean primitive) {
            super(catalog, primitive ? Double.TYPE : Double.class, primitive);
        }

        @Override
        Object newArray(int len) {
            return new Double[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess)
            throws RefreshException {

            return Double.valueOf(input.readSortedDouble());
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeSortedDouble(((Number) o).doubleValue());
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(8);
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            output.writeFast
                (input.getBufferBytes(), input.getBufferOffset(), 8);
            input.skipFast(8);
        }

        @Override
        Object newPrimitiveArray(int len, EntityInput input)
            throws RefreshException {

            double[] a = new double[len];
            for (int i = 0; i < len; i += 1) {
                a[i] = input.readSortedDouble();
            }
            return a;
        }

        @Override
        void writePrimitiveArray(Object o, EntityOutput output) {
            double[] a = (double[]) o;
            int len = a.length;
            output.writeArrayLength(len);
            for (int i = 0; i < len; i += 1) {
                output.writeSortedDouble(a[i]);
            }
        }

        @Override
        int getPrimitiveLength() {
            return 8;
        }

        @Override
        void readPrimitiveField(Object o, EntityInput input, Field field)
            throws IllegalAccessException, RefreshException {

            field.setDouble(o, input.readSortedDouble());
        }

        @Override
        void writePrimitiveField(Object o, EntityOutput output, Field field)
            throws IllegalAccessException {

            output.writeSortedDouble(field.getDouble(o));
        }
    }

    public static class FChar extends SimpleFormat {

        private static final long serialVersionUID = -7609118195770005374L;

        FChar(Catalog catalog, boolean primitive) {
            super(catalog, primitive ? Character.TYPE : Character.class,
                  primitive);
        }

        @Override
        Object newArray(int len) {
            return new Character[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess)
            throws RefreshException {

            return Character.valueOf(input.readChar());
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeChar(((Character) o).charValue());
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(2);
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            output.writeFast(input.readFast());
            output.writeFast(input.readFast());
        }

        @Override
        Object newPrimitiveArray(int len, EntityInput input)
            throws RefreshException {

            char[] a = new char[len];
            for (int i = 0; i < len; i += 1) {
                a[i] = input.readChar();
            }
            return a;
        }

        @Override
        void writePrimitiveArray(Object o, EntityOutput output) {
            char[] a = (char[]) o;
            int len = a.length;
            output.writeArrayLength(len);
            for (int i = 0; i < len; i += 1) {
                output.writeChar(a[i]);
            }
        }

        @Override
        int getPrimitiveLength() {
            return 2;
        }

        @Override
        void readPrimitiveField(Object o, EntityInput input, Field field)
            throws IllegalAccessException, RefreshException {

            field.setChar(o, input.readChar());
        }

        @Override
        void writePrimitiveField(Object o, EntityOutput output, Field field)
            throws IllegalAccessException {

            output.writeChar(field.getChar(o));
        }
    }

    public static class FString extends SimpleFormat {

        private static final long serialVersionUID = 5710392786480064612L;

        FString(Catalog catalog) {
            super(catalog, String.class, false);
        }

        @Override
        Object newArray(int len) {
            return new String[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess)
            throws RefreshException {

            return input.readString();
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeString((String) o);
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(input.getStringByteLength());
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            int len = input.getStringByteLength();
            output.writeFast
                (input.getBufferBytes(), input.getBufferOffset(), len);
            input.skipFast(len);
        }
    }

    public static class FBigInt extends SimpleFormat {

        private static final long serialVersionUID = -5027098112507644563L;

        FBigInt(Catalog catalog) {
            super(catalog, BigInteger.class, false);
        }

        @Override
        Object newArray(int len) {
            return new BigInteger[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess)
            throws RefreshException {

            return input.readBigInteger();
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeBigInteger((BigInteger) o);
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(input.getBigIntegerByteLength());
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            int len = input.getBigIntegerByteLength();
            output.writeFast
                (input.getBufferBytes(), input.getBufferOffset(), len);
            input.skipFast(len);
        }
    }
    
    public static class FBigDec extends SimpleFormat {
        private static final long serialVersionUID = 6108874887143696463L;
        
        FBigDec(Catalog catalog) {
            super(catalog, BigDecimal.class, false);
        }

        @Override
        Object newArray(int len) {
            return new BigDecimal[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess) 
            throws RefreshException {
            
            return input.readSortedBigDecimal();
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeSortedBigDecimal((BigDecimal) o);
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(input.getSortedBigDecimalByteLength());
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            int len = input.getSortedBigDecimalByteLength();
            output.writeFast
                (input.getBufferBytes(), input.getBufferOffset(), len);
            input.skipFast(len);
        }
        
        @Override
        public boolean allowEvolveFromProxy() {
            return true;
        }
    }

    public static class FDate extends SimpleFormat {

        private static final long serialVersionUID = -5665773229869034145L;

        FDate(Catalog catalog) {
            super(catalog, Date.class, false);
        }

        @Override
        Object newArray(int len) {
            return new Date[len];
        }

        @Override
        public Object newInstance(EntityInput input, boolean rawAccess)
            throws RefreshException {

            return new Date(input.readLong());
        }

        @Override
        void writeObject(Object o, EntityOutput output, boolean rawAccess) {
            output.writeLong(((Date) o).getTime());
        }

        @Override
        void skipContents(RecordInput input) {
            input.skipFast(8);
        }

        @Override
        void copySecKey(RecordInput input, RecordOutput output) {
            output.writeFast
                (input.getBufferBytes(), input.getBufferOffset(), 8);
            input.skipFast(8);
        }
    }
}
