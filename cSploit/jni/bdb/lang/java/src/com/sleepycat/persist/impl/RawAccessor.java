/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.persist.impl;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.persist.raw.RawObject;

/**
 * Implements Accessor for RawObject access.
 *
 * @author Mark Hayes
 */
class RawAccessor implements Accessor {

    private Format parentFormat;
    private Accessor superAccessor;
    private FieldInfo priKeyField;
    private List<FieldInfo> secKeyFields;
    private List<FieldInfo> nonKeyFields;
    private boolean isCompositeKey;

    RawAccessor(Format parentFormat,
                Accessor superAccessor,
                FieldInfo priKeyField,
                List<FieldInfo> secKeyFields,
                List<FieldInfo> nonKeyFields) {
        this.parentFormat = parentFormat;
        this.superAccessor = superAccessor;
        this.priKeyField = priKeyField;
        this.secKeyFields = secKeyFields;
        this.nonKeyFields = nonKeyFields;
    }

    RawAccessor(Format parentFormat,
                List<FieldInfo> nonKeyFields) {
        this.parentFormat = parentFormat;
        this.nonKeyFields = nonKeyFields;
        secKeyFields = Collections.emptyList();
        isCompositeKey = true;
    }

    public Object newInstance() {
        RawObject superObject;
        if (superAccessor != null) {
            superObject = ((RawObject) superAccessor.newInstance());
        } else {
            superObject = null;
        }
        return new RawObject
            (parentFormat, new HashMap<String, Object>(), superObject);
    }

    public Object newArray(int len) {
        throw DbCompat.unexpectedState();
    }

    public boolean isPriKeyFieldNullOrZero(Object o) {
        if (priKeyField != null) {
            Object val = getValue(o, priKeyField);
            Format format = priKeyField.getType();
            if (format.isPrimitive()) {
                return ((Number) val).longValue() == 0L;
            } else {
                return val == null;
            }
        } else if (superAccessor != null) {
            return superAccessor.isPriKeyFieldNullOrZero(getSuper(o));
        } else {
            throw DbCompat.unexpectedState("No primary key field");
        }
    }

    public void writePriKeyField(Object o, EntityOutput output)
        throws RefreshException {

        if (priKeyField != null) {
            Object val = getValue(o, priKeyField);
            Format format = priKeyField.getType();
            output.writeKeyObject(val, format);
        } else if (superAccessor != null) {
            superAccessor.writePriKeyField(getSuper(o), output);
        } else {
            throw DbCompat.unexpectedState("No primary key field");
        }
    }

    public void readPriKeyField(Object o, EntityInput input)
        throws RefreshException {

        if (priKeyField != null) {
            Format format = priKeyField.getType();
            Object val = input.readKeyObject(format);
            setValue(o, priKeyField, val);
        } else if (superAccessor != null) {
            superAccessor.readPriKeyField(getSuper(o), input);
        } else {
            throw DbCompat.unexpectedState("No primary key field");
        }
    }

    public void writeSecKeyFields(Object o, EntityOutput output)
        throws RefreshException {

        if (priKeyField != null && 
            !priKeyField.getType().isPrimitive() && 
            priKeyField.getType().getId() != Format.ID_STRING) {
            output.registerPriKeyObject(getValue(o, priKeyField));
        }
        if (superAccessor != null) {
            superAccessor.writeSecKeyFields(getSuper(o), output);
        }
        for (int i = 0; i < secKeyFields.size(); i += 1) {
            writeField(o, secKeyFields.get(i), output);
        }
    }

    public void readSecKeyFields(Object o,
                                 EntityInput input,
                                 int startField,
                                 int endField,
                                 int superLevel)
        throws RefreshException {

        if (priKeyField != null && 
            !priKeyField.getType().isPrimitive() &&
            priKeyField.getType().getId() != Format.ID_STRING) {
            input.registerPriKeyObject(getValue(o, priKeyField));
        } else if (priKeyField != null && 
                   priKeyField.getType().getId() == Format.ID_STRING) {
            input.registerPriStringKeyObject(getValue(o, priKeyField));
        }
        if (superLevel != 0 && superAccessor != null) {
            superAccessor.readSecKeyFields
                (getSuper(o), input, startField, endField, superLevel - 1);
        } else {
            if (superLevel > 0) {
                throw DbCompat.unexpectedState("Super class does not exist");
            }
        }
        if (superLevel <= 0) {
            for (int i = startField;
                 i <= endField && i < secKeyFields.size();
                 i += 1) {
                readField(o, secKeyFields.get(i), input);
            }
        }
    }

    public void writeNonKeyFields(Object o, EntityOutput output)
        throws RefreshException {

        if (superAccessor != null) {
            superAccessor.writeNonKeyFields(getSuper(o), output);
        }
        for (int i = 0; i < nonKeyFields.size(); i += 1) {
            writeField(o, nonKeyFields.get(i), output);
        }
    }

    public void readNonKeyFields(Object o,
                                 EntityInput input,
                                 int startField,
                                 int endField,
                                 int superLevel)
        throws RefreshException {

        if (superLevel != 0 && superAccessor != null) {
            superAccessor.readNonKeyFields
                (getSuper(o), input, startField, endField, superLevel - 1);
        } else {
            if (superLevel > 0) {
                throw DbCompat.unexpectedState("Super class does not exist");
            }
        }
        if (superLevel <= 0) {
            for (int i = startField;
                 i <= endField && i < nonKeyFields.size();
                 i += 1) {
                readField(o, nonKeyFields.get(i), input);
            }
        }
    }

    public void writeCompositeKeyFields(Object o, EntityOutput output)
        throws RefreshException {

        for (int i = 0; i < nonKeyFields.size(); i += 1) {
            writeField(o, nonKeyFields.get(i), output);
        }
    }

    public void readCompositeKeyFields(Object o, EntityInput input)
        throws RefreshException {

        for (int i = 0; i < nonKeyFields.size(); i += 1) {
            readField(o, nonKeyFields.get(i), input);
        }
    }

    public Object getField(Object o,
                           int field,
                           int superLevel,
                           boolean isSecField) {
        if (superLevel > 0) {
            return superAccessor.getField
                (getSuper(o), field, superLevel - 1, isSecField);
        }
        FieldInfo fld =
            isSecField ? secKeyFields.get(field) : nonKeyFields.get(field);
        return getValue(o, fld);
    }

    public void setField(Object o,
                         int field,
                         int superLevel,
                         boolean isSecField,
                         Object value) {
        if (superLevel > 0) {
            superAccessor.setField
                (getSuper(o), field, superLevel - 1, isSecField, value);
            return;
        }
        FieldInfo fld =
            isSecField ? secKeyFields.get(field) : nonKeyFields.get(field);
        setValue(o, fld, value);
    }
    
    public void setPriField(Object o, Object value) {
        if (priKeyField != null) {
            setValue(o, priKeyField, value);
        } else if (superAccessor != null) {
            superAccessor.setPriField(getSuper(o), value);
        } else {
            throw DbCompat.unexpectedState("No primary key field");
        }
    }

    private RawObject getSuper(Object o) {
        return ((RawObject) o).getSuper();
    }

    private Object getValue(Object o, FieldInfo field) {
        return ((RawObject) o).getValues().get(field.getName());
    }

    private void setValue(Object o, FieldInfo field, Object val) {
        ((RawObject) o).getValues().put(field.getName(), val);
    }

    private void writeField(Object o, FieldInfo field, EntityOutput output)
        throws RefreshException {

        Object val = getValue(o, field);
        Format format = field.getType();
        if (isCompositeKey || format.isPrimitive()) {
            output.writeKeyObject(val, format);
        } else if (format.getId() == Format.ID_STRING) {
            output.writeString((String) val);
        } else {
            output.writeObject(val, format);
        }
    }

    private void readField(Object o, FieldInfo field, EntityInput input)
        throws RefreshException {

        Format format = field.getType();
        Object val;
        if (isCompositeKey || format.isPrimitive()) {
            val = input.readKeyObject(format);
        } else if (format.getId() == Format.ID_STRING) {
            val = input.readStringObject();
        } else {
            val = input.readObject();
        }
        setValue(o, field, val);
    }
    
    public FieldInfo getPriKeyField() {
        return priKeyField;
    }
}
