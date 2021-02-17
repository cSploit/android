/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.bind.serial.test;

import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectStreamClass;
import java.util.HashMap;
import java.util.Map;

import com.sleepycat.bind.serial.ClassCatalog;
import com.sleepycat.bind.tuple.IntegerBinding;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.util.FastInputStream;
import com.sleepycat.util.FastOutputStream;
import com.sleepycat.util.RuntimeExceptionWrapper;

/**
 * @author Mark Hayes
 */
public class TestClassCatalog implements ClassCatalog {

    private final Map<Integer, ObjectStreamClass> idToDescMap =
        new HashMap<Integer, ObjectStreamClass>();
    private final Map<String, Integer> nameToIdMap =
        new HashMap<String, Integer>();
    private int nextId = 1;

    public TestClassCatalog() {
    }

    public void close() {
    }

    public synchronized byte[] getClassID(ObjectStreamClass desc) {
        String className = desc.getName();
        Integer intId = nameToIdMap.get(className);
        if (intId == null) {
            intId = nextId;
            nextId += 1;

            idToDescMap.put(intId, desc);
            nameToIdMap.put(className, intId);
        }
        DatabaseEntry entry = new DatabaseEntry();
        IntegerBinding.intToEntry(intId, entry);
        return entry.getData();
    }

    public synchronized ObjectStreamClass getClassFormat(byte[] byteId)
        throws DatabaseException {

        DatabaseEntry entry = new DatabaseEntry();
        entry.setData(byteId);
        int intId = IntegerBinding.entryToInt(entry);

        ObjectStreamClass desc = (ObjectStreamClass) idToDescMap.get(intId);
        if (desc == null) {
            throw new RuntimeException("classID not found");
        }
        
        /*
         * Workaround for a Harmony bug that appears on Android.  The
         * ObjectStreamClass is not properly initialized, and using it later
         * will cause NullPointerException.  Serializing it and then
         * deserializing it causes is to be initialized properly.  [#18163]
         */
        if (DbCompat.isDalvik()) {
            try {
                /* Serialize desc first. */
                FastOutputStream fo = new FastOutputStream();
                ObjectOutputStream oos = new ObjectOutputStream(fo);
                oos.writeObject(desc);
                byte[] bytes = fo.toByteArray();
                /* Then deserialize classFormat. */
                FastInputStream fi = new FastInputStream(bytes);
                ObjectInputStream ois = new ObjectInputStream(fi);
                desc = (ObjectStreamClass) ois.readObject();
            } catch (Exception e) {
                throw RuntimeExceptionWrapper.wrapIfNeeded(e);
            }
        }
        return desc;
    }

    public ClassLoader getClassLoader() {
        return null;
    }
}
