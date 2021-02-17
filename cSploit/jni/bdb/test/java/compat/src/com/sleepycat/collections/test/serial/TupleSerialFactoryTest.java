/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
package com.sleepycat.collections.test.serial;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

import com.sleepycat.bind.serial.StoredClassCatalog;
import com.sleepycat.bind.serial.test.MarshalledObject;
import com.sleepycat.collections.TransactionRunner;
import com.sleepycat.collections.TransactionWorker;
import com.sleepycat.collections.TupleSerialFactory;
import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.Environment;
import com.sleepycat.db.ForeignKeyDeleteAction;
import com.sleepycat.db.SecondaryConfig;
import com.sleepycat.db.SecondaryDatabase;
import com.sleepycat.util.test.SharedTestUtils;
import com.sleepycat.util.test.TestBase;
import com.sleepycat.util.test.TestEnv;

/**
 * @author Mark Hayes
 */

@RunWith(Parameterized.class)
public class TupleSerialFactoryTest extends TestBase
    implements TransactionWorker {

    @Parameters
    public static List<Object[]> genParams() {
        List<Object[]> params = new ArrayList<Object[]>();
        for (TestEnv testEnv : TestEnv.ALL) {
            for (int sorted = 0; sorted < 2; sorted += 1) {
                params.add(new Object[]{testEnv, sorted != 0 });
            }
        }
            
        return params;
    }

    private TestEnv testEnv;
    private Environment env;
    private StoredClassCatalog catalog;
    private TransactionRunner runner;
    private TupleSerialFactory factory;
    private Database store1;
    private Database store2;
    private SecondaryDatabase index1;
    private SecondaryDatabase index2;
    private final boolean isSorted;
    private Map storeMap1;
    private Map storeMap2;
    private Map indexMap1;
    private Map indexMap2;

    public TupleSerialFactoryTest(TestEnv testEnv, boolean isSorted) {


        this.testEnv = testEnv;
        this.isSorted = isSorted;

        String name = "TupleSerialFactoryTest-" + testEnv.getName();
        name += isSorted ? "-sorted" : "-unsorted";
        customName = name;
    }

    @Before
    public void setUp()
        throws Exception {

        super.setUp();
        SharedTestUtils.printTestName(customName);
        env = testEnv.open(customName);
        runner = new TransactionRunner(env);

        createDatabase();
    }

    @After
    public void tearDown() {

        try {
            if (index1 != null) {
                index1.close();
            }
            if (index2 != null) {
                index2.close();
            }
            if (store1 != null) {
                store1.close();
            }
            if (store2 != null) {
                store2.close();
            }
            if (catalog != null) {
                catalog.close();
            }
            if (env != null) {
                env.close();
            }
        } catch (Exception e) {
            System.out.println("Ignored exception during tearDown: " + e);
        } finally {
            /* Ensure that GC can cleanup. */
            index1 = null;
            index2 = null;
            store1 = null;
            store2 = null;
            catalog = null;
            env = null;
            testEnv = null;
            runner = null;
            factory = null;
            storeMap1 = null;
            storeMap2 = null;
            indexMap1 = null;
            indexMap2 = null;
        }
    }

    @Test
    public void runTest()
        throws Exception {

        runner.run(this);
    }

    public void doWork() {
        createViews();
        writeAndRead();
    }

    private void createDatabase()
        throws Exception {

        catalog = new StoredClassCatalog(openDb("catalog.db"));
        factory = new TupleSerialFactory(catalog);
        assertSame(catalog, factory.getCatalog());

        store1 = openDb("store1.db");
        store2 = openDb("store2.db");
        index1 = openSecondaryDb(factory, "1", store1, "index1.db", null);
        index2 = openSecondaryDb(factory, "2", store2, "index2.db", store1);
    }

    private Database openDb(String file)
        throws Exception {

        DatabaseConfig config = new DatabaseConfig();
        config.setTransactional(testEnv.isTxnMode());
        config.setAllowCreate(true);
        DbCompat.setTypeBtree(config);

        return DbCompat.testOpenDatabase(env, null, file, null, config);
    }

    private SecondaryDatabase openSecondaryDb(TupleSerialFactory factory,
                                              String keyName,
                                              Database primary,
                                              String file,
                                              Database foreignStore)
        throws Exception {

        SecondaryConfig secConfig = new SecondaryConfig();
        secConfig.setTransactional(testEnv.isTxnMode());
        secConfig.setAllowCreate(true);
        DbCompat.setTypeBtree(secConfig);
        secConfig.setKeyCreator(factory.getKeyCreator(MarshalledObject.class,
                                                      keyName));
        if (foreignStore != null) {
            secConfig.setForeignKeyDatabase(foreignStore);
            secConfig.setForeignKeyDeleteAction(
                    ForeignKeyDeleteAction.CASCADE);
        }

        return DbCompat.testOpenSecondaryDatabase
            (env, null, file, null, primary, secConfig);
    }

    private void createViews() {
        if (isSorted) {
            storeMap1 = factory.newSortedMap(store1, String.class,
                                             MarshalledObject.class, true);
            storeMap2 = factory.newSortedMap(store2, String.class,
                                             MarshalledObject.class, true);
            indexMap1 = factory.newSortedMap(index1, String.class,
                                             MarshalledObject.class, true);
            indexMap2 = factory.newSortedMap(index2, String.class,
                                             MarshalledObject.class, true);
        } else {
            storeMap1 = factory.newMap(store1, String.class,
                                       MarshalledObject.class, true);
            storeMap2 = factory.newMap(store2, String.class,
                                       MarshalledObject.class, true);
            indexMap1 = factory.newMap(index1, String.class,
                                       MarshalledObject.class, true);
            indexMap2 = factory.newMap(index2, String.class,
                                       MarshalledObject.class, true);
        }
    }

    private void writeAndRead() {
        MarshalledObject o1 = new MarshalledObject("data1", "pk1", "ik1", "");
        assertNull(storeMap1.put(null, o1));

        assertEquals(o1, storeMap1.get("pk1"));
        assertEquals(o1, indexMap1.get("ik1"));

        MarshalledObject o2 = new MarshalledObject("data2", "pk2", "", "pk1");
        assertNull(storeMap2.put(null, o2));

        assertEquals(o2, storeMap2.get("pk2"));
        assertEquals(o2, indexMap2.get("pk1"));

        /*
         * store1 contains o1 with primary key "pk1" and index key "ik1"
         * store2 contains o2 with primary key "pk2" and foreign key "pk1"
         * which is the primary key of store1
         */

        storeMap1.remove("pk1");
        assertNull(storeMap1.get("pk1"));
        assertNull(indexMap1.get("ik1"));
        assertNull(storeMap2.get("pk2"));
        assertNull(indexMap2.get("pk1"));
    }
}
