/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import java.io.Serializable;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;

import com.sleepycat.compat.DbCompat;

/**
 * The btree comparator for persistent key classes.  The serialized form of
 * this comparator is stored in the BDB JE database descriptor so that the
 * comparator can be re-created during recovery.
 *
 * @author Mark Hayes
 */
public class PersistComparator implements
    Comparator<byte[]>, Serializable {

    private static final long serialVersionUID = 5221576538843355317L;

    private String keyClassName;
    private String[] comositeFieldOrder;
    private Map<String, String[]> fieldFormatData;
    private transient PersistKeyBinding binding;

    public PersistComparator(PersistKeyBinding binding) {
        this.binding = binding;
        /* Save info necessary to recreate binding during deserialization. */
        final CompositeKeyFormat format =
            (CompositeKeyFormat) binding.keyFormat;
        keyClassName = format.getClassName();
        comositeFieldOrder = CompositeKeyFormat.getFieldNameArray
            (format.getClassMetadata().getCompositeKeyFields());
        /* Currently only enum formats have per-class data. */
        for (FieldInfo field : format.getFieldInfo()) {
            Format fieldFormat = field.getType();
            if (fieldFormat.isEnum()) {
                EnumFormat enumFormat = (EnumFormat) fieldFormat;
                if (fieldFormatData == null) {
                    fieldFormatData = new HashMap<String, String[]>();
                }
                fieldFormatData.put(enumFormat.getClassName(),
                                    enumFormat.getFormatData());
            }
        }
    }

    /**
     * In BDB JE this method will be called after the comparator is
     * deserialized, including during recovery.  We must construct the binding
     * here, without access to the stored catalog since recovery is not
     * complete.
     */
    public void initialize(ClassLoader loader) {
        final Catalog catalog;
        if (fieldFormatData == null) {
            catalog = new ComparatorCatalog(loader, null);
        } else {
            final Map<String, Format> enumFormats =
                new HashMap<String, Format>();
            catalog = new ComparatorCatalog(loader, enumFormats);
            for (Map.Entry<String, String[]> entry :
                 fieldFormatData.entrySet()) {
                final String fldClassName = entry.getKey();
                final String[] enumNames = entry.getValue();
                final Class fldClass;
                try {
                    fldClass = catalog.resolveClass(fldClassName);
                } catch (ClassNotFoundException e) {
                    throw new IllegalStateException(e);
                }
                enumFormats.put(fldClassName,
                                new EnumFormat(catalog, fldClass, enumNames));
            }
            for (Format fldFormat : enumFormats.values()) {
                fldFormat.initializeIfNeeded(catalog, null /*model*/);
            }
        }
        final Class keyClass;
        try {
            keyClass = catalog.resolveClass(keyClassName);
        } catch (ClassNotFoundException e) {
            throw new IllegalStateException(e);
        }
        binding = new PersistKeyBinding(catalog, keyClass,
                                        comositeFieldOrder);
    }

    public int compare(byte[] b1, byte[] b2) {

        /*
         * In BDB JE, the binding is initialized by the initialize method.  In
         * BDB, the binding is intialized by the constructor.
         */
        if (binding == null) {
            throw DbCompat.unexpectedState("Not initialized");
        }

        try {
            Comparable k1 =
                (Comparable) binding.bytesToObject(b1, 0, b1.length);
            Comparable k2 =
                (Comparable) binding.bytesToObject(b2, 0, b2.length);

            return k1.compareTo(k2);
        } catch (RefreshException e) {

            /*
             * Refresh is not applicable to PersistComparator, which is used
             * during recovery.  All field formats used by the comparator are
             * guaranteed to be predefined, because they must be primitive or
             * primitive wrapper types.  So they are always present in the
             * catalog, and cannot change as the result of class evolution or
             * replication.
             */
            throw DbCompat.unexpectedException(e);
        }
    }

    @Override
    public String toString() {
        StringBuilder b = new StringBuilder();
        b.append("[DPL comparator ");
        b.append(" keyClassName = ").append(keyClassName);
        b.append(" comositeFieldOrder = [");
        for (String s : comositeFieldOrder) {
            b.append(s).append(',');
        }
        b.append(']');
        b.append(" fieldFormatData = {");
        for (Map.Entry<String, String[]> entry : fieldFormatData.entrySet()) {
            b.append(entry.getKey()).append(": [");
            for (String s : entry.getValue()) {
                b.append(s).append(',');
            }
            b.append(']');
        }
        b.append('}');
        b.append(']');
        return b.toString();
    }
}
