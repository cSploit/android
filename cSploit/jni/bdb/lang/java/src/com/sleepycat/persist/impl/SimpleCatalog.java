/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;

import com.sleepycat.compat.DbCompat;
import java.util.IdentityHashMap;
import com.sleepycat.persist.raw.RawObject;
import com.sleepycat.util.ClassResolver;

/**
 * A static catalog containing simple types only.  Once created, this catalog
 * is immutable.
 *
 * For bindings accessed by a PersistComparator during recovery, the
 * SimpleCatalog provides formats for all simple types.  To reduce redundant
 * format objects, the SimpleCatalog's formats are copied when creating a
 * regular PersistCatalog.
 *
 * This class also contains utility methods for dealing with primitives.
 *
 * @author Mark Hayes
 */
public class SimpleCatalog implements Catalog {

    private static final Map<String, Class> keywordToPrimitive;
    static {
        keywordToPrimitive = new HashMap<String, Class>(8);
        keywordToPrimitive.put("boolean", Boolean.TYPE);
        keywordToPrimitive.put("char", Character.TYPE);
        keywordToPrimitive.put("byte", Byte.TYPE);
        keywordToPrimitive.put("short", Short.TYPE);
        keywordToPrimitive.put("int", Integer.TYPE);
        keywordToPrimitive.put("long", Long.TYPE);
        keywordToPrimitive.put("float", Float.TYPE);
        keywordToPrimitive.put("double", Double.TYPE);
    }

    private static final Map<Class, Class> primitiveTypeToWrapper;
    static {
        primitiveTypeToWrapper = new HashMap<Class, Class>(8);
        primitiveTypeToWrapper.put(Boolean.TYPE, Boolean.class);
        primitiveTypeToWrapper.put(Character.TYPE, Character.class);
        primitiveTypeToWrapper.put(Byte.TYPE, Byte.class);
        primitiveTypeToWrapper.put(Short.TYPE, Short.class);
        primitiveTypeToWrapper.put(Integer.TYPE, Integer.class);
        primitiveTypeToWrapper.put(Long.TYPE, Long.class);
        primitiveTypeToWrapper.put(Float.TYPE, Float.class);
        primitiveTypeToWrapper.put(Double.TYPE, Double.class);
    }

    private static final SimpleCatalog instance = new SimpleCatalog(null);

    static boolean isSimpleType(Class type) {
        return instance.formatMap.containsKey(type.getName());
    }

    static Class primitiveToWrapper(Class type) {
        Class cls = primitiveTypeToWrapper.get(type);
        if (cls == null) {
            throw DbCompat.unexpectedState(type.getName());
        }
        return cls;
    }

    public static Class resolveClass(String className, ClassLoader loader)
        throws ClassNotFoundException {

        Class cls = keywordToPrimitive.get(className);
        if (cls == null) {
            cls = ClassResolver.resolveClass(className, loader);
        }
        return cls;
    }

    public static Class resolveKeyClass(String className, ClassLoader loader) {
        Class cls = keywordToPrimitive.get(className);
        if (cls != null) {
            cls = primitiveTypeToWrapper.get(cls);
        } else {
            try {
                cls = ClassResolver.resolveClass(className, loader);
            } catch (ClassNotFoundException e) {
                throw new IllegalArgumentException
                    ("Key class not found: " + className);
            }
        }
        return cls;
    }

    public static String keyClassName(String className) {
        Class cls = keywordToPrimitive.get(className);
        if (cls != null) {
            cls = primitiveTypeToWrapper.get(cls);
            return cls.getName();
        } else {
            return className;
        }
    }

    static List<Format> getAllSimpleFormats(ClassLoader loader) {
        return new ArrayList<Format>(new SimpleCatalog(loader).formatList);
    }

    static boolean addMissingSimpleFormats(ClassLoader loader,
                                           List<Format> copyToList) {
        boolean anyCopied = false;
        SimpleCatalog tempCatalog = null;
        for (int i = 0; i <= Format.ID_PREDEFINED; i += 1) {
            final Format thisFormat = instance.formatList.get(i);
            final Format otherFormat = copyToList.get(i);
            if (thisFormat != null && otherFormat == null) {
                assert thisFormat.getWrapperFormat() == null;
                if (tempCatalog == null) {
                    tempCatalog = new SimpleCatalog(loader);
                }
                copyToList.set(i, tempCatalog.formatList.get(i));
                anyCopied = true;
            }
        }
        return anyCopied;
    }

    private final ClassLoader classLoader;
    private final List<SimpleFormat> formatList;
    private final Map<String, SimpleFormat> formatMap;

    SimpleCatalog(final ClassLoader classLoader) {
        this.classLoader = classLoader;

        /*
         * Reserve slots for all predefined IDs, so that that next ID assigned
         * will be Format.ID_PREDEFINED plus one.
         */
        int initCapacity = Format.ID_PREDEFINED * 2;
        formatList = new ArrayList<SimpleFormat>(initCapacity);
        formatMap = new HashMap<String, SimpleFormat>(initCapacity);

        for (int i = 0; i <= Format.ID_PREDEFINED; i += 1) {
            formatList.add(null);
        }

        /* Initialize all predefined formats.  */
        setFormat(Format.ID_BOOL,     new SimpleFormat.FBool(this, true));
        setFormat(Format.ID_BOOL_W,   new SimpleFormat.FBool(this, false));
        setFormat(Format.ID_BYTE,     new SimpleFormat.FByte(this, true));
        setFormat(Format.ID_BYTE_W,   new SimpleFormat.FByte(this, false));
        setFormat(Format.ID_SHORT,    new SimpleFormat.FShort(this, true));
        setFormat(Format.ID_SHORT_W,  new SimpleFormat.FShort(this, false));
        setFormat(Format.ID_INT,      new SimpleFormat.FInt(this, true));
        setFormat(Format.ID_INT_W,    new SimpleFormat.FInt(this, false));
        setFormat(Format.ID_LONG,     new SimpleFormat.FLong(this, true));
        setFormat(Format.ID_LONG_W,   new SimpleFormat.FLong(this, false));
        setFormat(Format.ID_FLOAT,    new SimpleFormat.FFloat(this, true));
        setFormat(Format.ID_FLOAT_W,  new SimpleFormat.FFloat(this, false));
        setFormat(Format.ID_DOUBLE,   new SimpleFormat.FDouble(this, true));
        setFormat(Format.ID_DOUBLE_W, new SimpleFormat.FDouble(this, false));
        setFormat(Format.ID_CHAR,     new SimpleFormat.FChar(this, true));
        setFormat(Format.ID_CHAR_W,   new SimpleFormat.FChar(this, false));
        setFormat(Format.ID_STRING,   new SimpleFormat.FString(this));
        setFormat(Format.ID_BIGINT,   new SimpleFormat.FBigInt(this));
        setFormat(Format.ID_BIGDEC,   new SimpleFormat.FBigDec(this));
        setFormat(Format.ID_DATE,     new SimpleFormat.FDate(this));

        /* Tell primitives about their wrapper class. */
        setWrapper(Format.ID_BOOL, Format.ID_BOOL_W);
        setWrapper(Format.ID_BYTE, Format.ID_BYTE_W);
        setWrapper(Format.ID_SHORT, Format.ID_SHORT_W);
        setWrapper(Format.ID_INT, Format.ID_INT_W);
        setWrapper(Format.ID_LONG, Format.ID_LONG_W);
        setWrapper(Format.ID_FLOAT, Format.ID_FLOAT_W);
        setWrapper(Format.ID_DOUBLE, Format.ID_DOUBLE_W);
        setWrapper(Format.ID_CHAR, Format.ID_CHAR_W);
    }

    /**
     * Sets a format for which space in the formatList has been preallocated,
     * and makes it the current format for the class.
     */
    private void setFormat(int id, SimpleFormat format) {
        format.setId(id);
        format.initializeIfNeeded(this, null /*model*/);
        formatList.set(id, format);
        formatMap.put(format.getClassName(), format);
    }

    /**
     * Tells a primitive format about the format for its corresponding
     * primitive wrapper class.
     */
    private void setWrapper(int primitiveId, int wrapperId) {
        SimpleFormat primitiveFormat = formatList.get(primitiveId);
        SimpleFormat wrapperFormat = formatList.get(wrapperId);
        primitiveFormat.setWrapperFormat(wrapperFormat);
    }

    public int getInitVersion(Format format, boolean forReader) {
        return Catalog.CURRENT_VERSION;
    }

    public Format getFormat(int formatId, boolean expectStored) {
        Format format;
        try {
            format = formatList.get(formatId);
            if (format == null) {
                throw DbCompat.unexpectedState
                    ("Not a simple type: " + formatId);
            }
            return format;
        } catch (NoSuchElementException e) {
            throw DbCompat.unexpectedState
                ("Not a simple type: " + formatId);
        }
    }

    public Format getFormat(Class cls, boolean checkEntitySubclassIndexes) {
        Format format = formatMap.get(cls.getName());
        if (format == null) {
            throw new IllegalArgumentException
                ("Not a simple type: " + cls.getName());
        }
        return format;
    }

    public Format getFormat(String className) {
        return formatMap.get(className);
    }

    public Format createFormat(String clsName,
                               Map<String, Format> newFormats) {
        throw DbCompat.unexpectedState();
    }

    public Format createFormat(Class type, Map<String, Format> newFormats) {
        throw DbCompat.unexpectedState();
    }

    public boolean isRawAccess() {
        return false;
    }

    public Object convertRawObject(RawObject o, IdentityHashMap converted) {
        throw DbCompat.unexpectedState();
    }

    public Class resolveClass(String clsName)
        throws ClassNotFoundException {

        return SimpleCatalog.resolveClass(clsName, classLoader);
    }

    public Class resolveKeyClass(String clsName) {
        return SimpleCatalog.resolveKeyClass(clsName, classLoader);
    }
    
    /* Registering proxy is not allowed for SimpleType. */
    public static boolean allowRegisterProxy(Class type) {
        return !isSimpleType(type);
    }
}
