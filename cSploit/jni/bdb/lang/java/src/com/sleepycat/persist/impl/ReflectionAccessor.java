/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Modifier;
import java.util.List;

import com.sleepycat.compat.DbCompat;

/**
 * Implements Accessor using reflection.
 *
 * @author Mark Hayes
 */
class ReflectionAccessor implements Accessor {

    private static final FieldAccess[] EMPTY_KEYS = {};

    private Class type;
    private Accessor superAccessor;
    private Constructor constructor;
    private FieldAccess priKey;
    private FieldAccess[] secKeys;
    private FieldAccess[] nonKeys;

    private ReflectionAccessor(Class type, Accessor superAccessor) {
        this.type = type;
        this.superAccessor = superAccessor;
        try {
            constructor = type.getDeclaredConstructor();
        } catch (NoSuchMethodException e) {
            throw DbCompat.unexpectedState(type.getName());
        }
        if (!Modifier.isPublic(type.getModifiers()) || 
            !Modifier.isPublic(constructor.getModifiers())) {
            setAccessible(constructor, type.getName() + "()");
        }
    }

    /**
     * Creates an accessor for a complex type.
     */
    ReflectionAccessor(Catalog catalog,
                       Class type,
                       Accessor superAccessor,
                       FieldInfo priKeyField,
                       List<FieldInfo> secKeyFields,
                       List<FieldInfo> nonKeyFields) {
        this(type, superAccessor);
        if (priKeyField != null) {
            priKey = getField(catalog, priKeyField,
                              true /*isRequiredKeyField*/);
        } else {
            priKey = null;
        }
        if (secKeyFields.size() > 0) {
            secKeys = getFields(catalog, secKeyFields,
                                false /*isRequiredKeyField*/);
        } else {
            secKeys = EMPTY_KEYS;
        }
        if (nonKeyFields.size() > 0) {
            nonKeys = getFields(catalog, nonKeyFields,
                                false /*isRequiredKeyField*/);
        } else {
            nonKeys = EMPTY_KEYS;
        }
    }

    /**
     * Creates an accessor for a composite key type.
     */
    ReflectionAccessor(Catalog catalog,
                       Class type,
                       List<FieldInfo> fieldInfos) {
        this(type, null);
        priKey = null;
        secKeys = EMPTY_KEYS;
        nonKeys = getFields(catalog, fieldInfos, true /*isRequiredKeyField*/);
    }

    private FieldAccess[] getFields(Catalog catalog,
                                    List<FieldInfo> fieldInfos,
                                    boolean isRequiredKeyField) {
        int index = 0;
        FieldAccess[] fields = new FieldAccess[fieldInfos.size()];
        for (FieldInfo info : fieldInfos) {
            fields[index] = getField(catalog, info, isRequiredKeyField);
            index += 1;
        }
        return fields;
    }

    private FieldAccess getField(Catalog catalog,
                                 FieldInfo fieldInfo,
                                 boolean isRequiredKeyField) {
        Field field;
        try {
            field = type.getDeclaredField(fieldInfo.getName());
        } catch (NoSuchFieldException e) {
            throw DbCompat.unexpectedException(e);
        }
        if (!Modifier.isPublic(type.getModifiers()) || 
            !Modifier.isPublic(field.getModifiers())) {
            setAccessible(field, field.getName());
        }
        Class fieldCls = field.getType();
        if (fieldCls.isPrimitive()) {
            assert SimpleCatalog.isSimpleType(fieldCls);
            return new PrimitiveAccess
                (field, (SimpleFormat) catalog.getFormat(fieldCls.getName()));
        } else if (isRequiredKeyField) {
            Format format = catalog.getFormat(fieldInfo.getClassName());
            assert format != null;
            return new KeyObjectAccess(field, format);
        } else if (fieldCls == String.class) {
            return new StringAccess(field);
        } else {
            return new ObjectAccess(field);
        }
    }

    private void setAccessible(AccessibleObject object, String memberName) {
        try {
            object.setAccessible(true);
        } catch (SecurityException e) {
            throw new IllegalStateException
                ("Unable to access non-public member: " +
                 type.getName() + '.' + memberName +
                 ". Please configure the Java Security Manager setting: " +
                 " ReflectPermission suppressAccessChecks", e);
        }
    }

    public Object newInstance() {
        try {
            return constructor.newInstance();
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        } catch (InstantiationException e) {
            throw DbCompat.unexpectedException(e);
        } catch (InvocationTargetException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    public Object newArray(int len) {
        return Array.newInstance(type, len);
    }

    public boolean isPriKeyFieldNullOrZero(Object o) {
        try {
            if (priKey != null) {
                return priKey.isNullOrZero(o);
            } else if (superAccessor != null) {
                return superAccessor.isPriKeyFieldNullOrZero(o);
            } else {
                throw DbCompat.unexpectedState("No primary key field");
            }
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    public void writePriKeyField(Object o, EntityOutput output)
        throws RefreshException {

        try {
            if (priKey != null) {
                priKey.write(o, output);
            } else if (superAccessor != null) {
                superAccessor.writePriKeyField(o, output);
            } else {
                throw DbCompat.unexpectedState("No primary key field");
            }
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    public void readPriKeyField(Object o, EntityInput input)
        throws RefreshException {

        try {
            if (priKey != null) {
                priKey.read(o, input);
            } else if (superAccessor != null) {
                superAccessor.readPriKeyField(o, input);
            } else {
                throw DbCompat.unexpectedState("No primary key field");
            }
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    public void writeSecKeyFields(Object o, EntityOutput output)
        throws RefreshException {

        try {
        
            /* 
             * In JE 5.0, String is treated as primitive type, so String does
             * not need to be registered. [#19247]
             */
            if (priKey != null && !priKey.isPrimitive && !priKey.isString) {
                output.registerPriKeyObject(priKey.field.get(o));
            }
            if (superAccessor != null) {
                superAccessor.writeSecKeyFields(o, output);
            }
            for (int i = 0; i < secKeys.length; i += 1) {
                secKeys[i].write(o, output);
            }
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    public void readSecKeyFields(Object o,
                                 EntityInput input,
                                 int startField,
                                 int endField,
                                 int superLevel)
        throws RefreshException {

        try {
            if (priKey != null && !priKey.isPrimitive && !priKey.isString) {
                input.registerPriKeyObject(priKey.field.get(o));
            } else if (priKey != null && priKey.isString) {
                input.registerPriStringKeyObject(priKey.field.get(o));
            }
            if (superLevel != 0 && superAccessor != null) {
                superAccessor.readSecKeyFields
                    (o, input, startField, endField, superLevel - 1);
            } else {
                if (superLevel > 0) {
                    throw DbCompat.unexpectedState
                        ("Superclass does not exist");
                }
            }
            if (superLevel <= 0) {
                for (int i = startField;
                     i <= endField && i < secKeys.length;
                     i += 1) {
                    secKeys[i].read(o, input);
                }
            }
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    public void writeNonKeyFields(Object o, EntityOutput output)
        throws RefreshException {

        try {
            if (superAccessor != null) {
                superAccessor.writeNonKeyFields(o, output);
            }
            for (int i = 0; i < nonKeys.length; i += 1) {
                nonKeys[i].write(o, output);
            }
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    public void readNonKeyFields(Object o,
                                 EntityInput input,
                                 int startField,
                                 int endField,
                                 int superLevel)
        throws RefreshException {

        try {
            if (superLevel != 0 && superAccessor != null) {
                superAccessor.readNonKeyFields
                    (o, input, startField, endField, superLevel - 1);
            } else {
                if (superLevel > 0) {
                    throw DbCompat.unexpectedState
                        ("Superclass does not exist");
                }
            }
            if (superLevel <= 0) {
                for (int i = startField;
                     i <= endField && i < nonKeys.length;
                     i += 1) {
                    nonKeys[i].read(o, input);
                }
            }
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    public void writeCompositeKeyFields(Object o, EntityOutput output)
        throws RefreshException {

        try {
            for (int i = 0; i < nonKeys.length; i += 1) {
                nonKeys[i].write(o, output);
            }
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    public void readCompositeKeyFields(Object o, EntityInput input)
        throws RefreshException {

        try {
            for (int i = 0; i < nonKeys.length; i += 1) {
                nonKeys[i].read(o, input);
            }
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    public Object getField(Object o,
                           int field,
                           int superLevel,
                           boolean isSecField) {
        if (superLevel > 0) {
            return superAccessor.getField
                (o, field, superLevel - 1, isSecField);
        }
        try {
            Field fld =
                isSecField ? secKeys[field].field : nonKeys[field].field;
            return fld.get(o);
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    public void setField(Object o,
                         int field,
                         int superLevel,
                         boolean isSecField,
                         Object value) {
        if (superLevel > 0) {
            superAccessor.setField
                (o, field, superLevel - 1, isSecField, value);
            return;
        }
        try {
            Field fld =
                isSecField ? secKeys[field].field : nonKeys[field].field;
            fld.set(o, value);
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }
    
    public void setPriField(Object o, Object value) {
        try {
            if (priKey != null) {
                priKey.field.set(o, value);
            } else if (superAccessor != null) {
                superAccessor.setPriField(o, value);
            } else {
                throw DbCompat.unexpectedState("No primary key field");
            }
        } catch (IllegalAccessException e) {
            throw DbCompat.unexpectedException(e);
        }
    }

    /**
     * Abstract base class for field access classes.
     */
    private static abstract class FieldAccess {

        Field field;
        boolean isPrimitive;
        boolean isString = false;

        FieldAccess(Field field) {
            this.field = field;
            isPrimitive = field.getType().isPrimitive();
            isString = 
                field.getType().getName().equals(String.class.getName());
        }

        /**
         * Writes a field.
         */
        abstract void write(Object o, EntityOutput out)
            throws IllegalAccessException, RefreshException;

        /**
         * Reads a field.
         */
        abstract void read(Object o, EntityInput in)
            throws IllegalAccessException, RefreshException;

        /**
         * Returns whether a field is null (for reference types) or zero (for
         * primitive integer types).  This implementation handles the reference
         * types.
         */
        boolean isNullOrZero(Object o)
            throws IllegalAccessException {

            return field.get(o) == null;
        }
    }

    /**
     * Access for fields with object types.
     */
    private static class ObjectAccess extends FieldAccess {

        ObjectAccess(Field field) {
            super(field);
        }

        @Override
        void write(Object o, EntityOutput out)
            throws IllegalAccessException, RefreshException {

            out.writeObject(field.get(o), null);
        }

        @Override
        void read(Object o, EntityInput in)
            throws IllegalAccessException, RefreshException {

            field.set(o, in.readObject());
        }
    }

    /**
     * Access for primary key fields and composite key fields with object
     * types.
     */
    private static class KeyObjectAccess extends FieldAccess {

        private Format format;

        KeyObjectAccess(Field field, Format format) {
            super(field);
            this.format = format;
        }

        @Override
        void write(Object o, EntityOutput out)
            throws IllegalAccessException, RefreshException {

            out.writeKeyObject(field.get(o), format);
        }

        @Override
        void read(Object o, EntityInput in)
            throws IllegalAccessException, RefreshException {

            field.set(o, in.readKeyObject(format));
        }
    }

    /**
     * Access for String fields, that are not primary key fields or composite
     * key fields with object types.
     */
    private static class StringAccess extends FieldAccess {
        StringAccess(Field field) {
            super(field);
        }

        @Override
        void write(Object o, EntityOutput out)
            throws IllegalAccessException, RefreshException {
            
            out.writeString((String) field.get(o));
        }

        @Override
        void read(Object o, EntityInput in)
            throws IllegalAccessException, RefreshException {

            field.set(o, in.readStringObject());
        }
    }

    /**
     * Access for fields with primitive types.
     */
    private static class PrimitiveAccess extends FieldAccess {

        private SimpleFormat format;

        PrimitiveAccess(Field field, SimpleFormat format) {
            super(field);
            this.format = format;
        }

        @Override
        void write(Object o, EntityOutput out)
            throws IllegalAccessException {

            format.writePrimitiveField(o, out, field);
        }

        @Override
        void read(Object o, EntityInput in)
            throws IllegalAccessException, RefreshException {

            format.readPrimitiveField(o, in, field);
        }

        @Override
        boolean isNullOrZero(Object o)
            throws IllegalAccessException {

            return field.getLong(o) == 0;
        }
    }
}
