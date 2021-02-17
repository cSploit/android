/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import java.io.Serializable;
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.persist.model.EntityModel;
import com.sleepycat.persist.raw.RawField;
import com.sleepycat.persist.model.FieldMetadata;
import com.sleepycat.persist.model.ClassMetadata;

/**
 * A field definition used by ComplexFormat and CompositeKeyFormat.
 *
 * <p>Note that the equals(), compareTo() and hashCode() methods only use the
 * name field in this class.  Comparing two FieldInfo objects is only done when
 * both are declared in the same class, so comparing the field name is
 * sufficient.</p>
 *
 * @author Mark Hayes
 */
class FieldInfo implements RawField, Serializable, Comparable<FieldInfo> {

    private static final long serialVersionUID = 2062721100372306296L;

    /**
     * Returns a list of all non-transient non-static fields that are declared
     * in the given class.
     */
    static List<FieldInfo> getInstanceFields(Class cls,
                                             ClassMetadata clsMeta) {
        List<FieldInfo> fields = null;
        if (clsMeta != null) {
            Collection<FieldMetadata> persistentFields =
                clsMeta.getPersistentFields();
            if (persistentFields != null) {
                fields = new ArrayList<FieldInfo>(persistentFields.size());
                String clsName = cls.getName();
                for (FieldMetadata fieldMeta : persistentFields) {
                    if (!clsName.equals(fieldMeta.getDeclaringClassName())) {
                        throw new IllegalArgumentException
                            ("Persistent field " + fieldMeta +
                             " must be declared in " + clsName);
                    }
                    Field field;
                    try {
                        field = cls.getDeclaredField(fieldMeta.getName());
                    } catch (NoSuchFieldException e) {
                        throw new IllegalArgumentException
                            ("Persistent field " + fieldMeta +
                             " is not declared in this class");
                    }
                    if (!field.getType().getName().equals
                        (fieldMeta.getClassName())) {
                        throw new IllegalArgumentException
                            ("Persistent field " + fieldMeta +
                             " must be of type " + field.getType().getName());
                    }
                    if (Modifier.isStatic(field.getModifiers())) {
                        throw new IllegalArgumentException
                            ("Persistent field " + fieldMeta +
                             " may not be static");
                    }
                    fields.add(new FieldInfo(field));
                }
            }
        }
        if (fields == null) {
            Field[] declaredFields = cls.getDeclaredFields();
            fields = new ArrayList<FieldInfo>(declaredFields.length);
            for (Field field : declaredFields) {
                int mods = field.getModifiers();
                if (!Modifier.isTransient(mods) && !Modifier.isStatic(mods)) {
                    fields.add(new FieldInfo(field));
                }
            }
        }
        return fields;
    }

    static FieldInfo getField(List<FieldInfo> fields, String fieldName) {
        int i = getFieldIndex(fields, fieldName);
        if (i >= 0) {
            return fields.get(i);
        } else {
            return null;
        }
    }

    static int getFieldIndex(List<FieldInfo> fields, String fieldName) {
        for (int i = 0; i < fields.size(); i += 1) {
            FieldInfo field = fields.get(i);
            if (fieldName.equals(field.getName())) {
                return i;
            }
        }
        return -1;
    }

    private String name;
    private String className;
    private Format format;
    private transient Class cls;
    private transient Field field;

    private FieldInfo(Field field) {
        name = field.getName();
        cls = field.getType();
        className = cls.getName();
        this.field = field;
    }

    void collectRelatedFormats(Catalog catalog,
                               Map<String, Format> newFormats) {

        /*
         * Prior to intialization we save the newly created format in the
         * format field so that it can be used by class evolution.  But note
         * that it may be replaced by the initialize method.  [#16233]
         */
        format = catalog.createFormat(cls, newFormats);
        
        /*
         * If the created format is a NonPersistentFormat, and the field is a
         * map or a collection, then the generic types of this field are 
         * ParameterizedTypes, e.g., Map<MyClass1, MyClass2>, so the formats of 
         * the generic types for this field, i.e., MyClass1 and MyClass2 will 
         * be created here. [#19377]
         */
        Class cls = field.getType();
        if (format instanceof NonPersistentFormat &&
            (java.util.Map.class.isAssignableFrom(cls) ||
             java.util.Collection.class.isAssignableFrom(cls))) {
            if (field != null && 
                field.getGenericType() instanceof ParameterizedType) {
                collectParameterizedTypeFormats(catalog, newFormats, 
                    (ParameterizedType)field.getGenericType());
            }
        }
    }
    
    /*
     * Create formats for the parameterized types, e.g., will create formats 
     * for MyClass1 and MyClass2 when meeting Map<MyClass1, Set<MyClass2>>, 
     * where MyClass1 and MyClass2 are instance of java.lang.Class.
     */
    private void 
        collectParameterizedTypeFormats(Catalog catalog,
                                        Map<String, Format> newFormats,
                                        ParameterizedType parameType) {
        Type[] types = parameType.getActualTypeArguments();
        for (int i = 0; i < types.length; i++) {
            if (types[i] instanceof ParameterizedType) {
                collectParameterizedTypeFormats(catalog, newFormats, 
                                                (ParameterizedType)types[i]);
            } else if (types[i] instanceof Class) {

                /* 
                 * Only use Catalog.createFormat to create the format for the
                 * class which is instance of java.lang.class.
                 */
                catalog.createFormat((Class)types[i], newFormats);
            }
        }
    }

    void migrateFromBeta(Map<String, Format> formatMap) {
        if (format == null) {
            format = formatMap.get(className);
            if (format == null) {
                throw DbCompat.unexpectedState(className);
            }
        }
    }

    void initialize(Catalog catalog, EntityModel model, int initVersion) {

        /*
         * Reset the format if it was never initialized, which can occur when a
         * new format instance created during class evolution and discarded
         * because nothing changed. [#16233]
         *
         * Note that the format field may be null when used in a composite key
         * format used as a key comparator (via PersistComparator).  In that
         * case (null format), we must not attempt to reset the format.
         */
        if (format != null && format.isNew()) {
            format = catalog.getFormat(className);
        }
    }

    Class getFieldClass(Catalog catalog) {
        if (cls == null) {
            try {
                cls = catalog.resolveClass(className);
            } catch (ClassNotFoundException e) {
                throw DbCompat.unexpectedException(e);
            }
        }
        return cls;
    }

    String getClassName() {
        return className;
    }

    public String getName() {
        return name;
    }

    public Format getType() {
        return format;
    }

    public int compareTo(FieldInfo o) {
        return name.compareTo(o.name);
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof FieldInfo) {
            FieldInfo o = (FieldInfo) other;
            return name.equals(o.name);
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() {
        return name.hashCode();
    }

    @Override
    public String toString() {
        return "[Field name: " + name + " class: " + className + ']';
    }
}
