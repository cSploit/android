/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
package com.sleepycat.persist.test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.sleepycat.persist.evolve.EvolveConfig;
import com.sleepycat.persist.evolve.EvolveEvent;
import com.sleepycat.persist.evolve.EvolveListener;
import com.sleepycat.persist.evolve.EvolveStats;
import com.sleepycat.persist.impl.PersistCatalog;
import com.sleepycat.util.test.SharedTestUtils;

/**
 * Runs part two of the EvolveTest.  This part is run with the new/updated
 * version of EvolveClasses in the classpath.  It uses the environment and
 * store created by EvolveTestInit.  It verifies that it can read/write/evolve
 * objects serialized using the old class format, and that it can create new
 * objects with the new class format.
 *
 * @author Mark Hayes
 */
public class EvolveTest extends EvolveTestBase {

    public EvolveTest(String originalClsName, String evolvedClsName)
            throws Exception {
        super(originalClsName, evolvedClsName);
    }

    /* Toggle to use listener every other test case. */
    private static boolean useEvolveListener;

    private int evolveNRead;
    private int evolveNConverted;

    boolean useEvolvedClass() {
        return true;
    }

    @Before
    public void setUp()
        throws Exception {

        super.setUp();
        
        /* Copy the log files created by EvolveTestInit. */
        envHome = getTestInitHome(true /*evolved*/);
        envHome.mkdirs();
        SharedTestUtils.emptyDir(envHome);
        SharedTestUtils.copyFiles(getTestInitHome(false /*evolved*/), envHome);
    }
    
    @After
    public void tearDown() {
        try { super.tearDown(); } catch (Throwable e) { }
    }

    @Test
    public void testLazyEvolve()
        throws Exception {

        openEnv();

        /*
         * Open in raw mode to check unevolved raw object and formats.  This
         * is possible whether or not we can open the store further below to
         * evolve formats without errors.
         */
        openRawStore();
        caseObj.checkUnevolvedModel(rawStore.getModel(), env);
        caseObj.readRawObjects
            (rawStore, false /*expectEvolved*/, false /*expectUpdated*/);
        closeRawStore();

        /*
         * Check evolution in read-only mode. Since Replica upgrade mode is
         * effectively read-only mode, this also helps to test evolution during
         * replication group upgrades. [#18690]
         */
        if (openStoreReadOnly()) {
            caseObj.checkEvolvedModel
                (store.getModel(), env, true /*oldTypesExist*/);
            caseObj.readObjects(store, false /*doUpdate*/);
            closeStore();
        }

        if (openStoreReadWrite()) {

            /*
             * When opening read-write, formats are evolved lazily.  Check by
             * reading evolved objects.
             */
            caseObj.checkEvolvedModel
                (store.getModel(), env, true /*oldTypesExist*/);
            caseObj.readObjects(store, false /*doUpdate*/);
            closeStore();

            /*
             * Read raw objects again to check that the evolved objects are
             * returned even though the stored objects were not evolved.
             */
            openRawStore();
            caseObj.checkEvolvedModel
                (rawStore.getModel(), env, true /*oldTypesExist*/);
            caseObj.readRawObjects
                (rawStore, true /*expectEvolved*/, false /*expectUpdated*/);
            closeRawStore();

            /*
             * Open read-only to ensure that the catalog does not need to
             * change (evolve formats) unnecessarily.
             */
            PersistCatalog.expectNoClassChanges = true;
            try {
                assertTrue(openStoreReadOnly());
            } finally {
                PersistCatalog.expectNoClassChanges = false;
            }
            caseObj.checkEvolvedModel
                (store.getModel(), env, true /*oldTypesExist*/);
            caseObj.readObjects(store, false /*doUpdate*/);
            closeStore();

            /*
             * Open read-write to update objects and store them in evolved
             * format.
             */
            openStoreReadWrite();
            caseObj.checkEvolvedModel
                (store.getModel(), env, true /*oldTypesExist*/);
            caseObj.readObjects(store, true /*doUpdate*/);
            caseObj.checkEvolvedModel
                (store.getModel(), env, true /*oldTypesExist*/);
            closeStore();

            /*
             * Check raw objects again after the evolved objects were stored.
             */
            openRawStore();
            caseObj.checkEvolvedModel
                (rawStore.getModel(), env, true /*oldTypesExist*/);
            caseObj.readRawObjects
                (rawStore, true /*expectEvolved*/, true /*expectUpdated*/);
            closeRawStore();
        }

        closeAll();
    }

    @Test
    public void testEagerEvolve()
        throws Exception {

        /* If the store cannot be opened, this test is not appropriate. */
        if (caseObj.getStoreOpenException() != null) {
            return;
        }

        EvolveConfig config = new EvolveConfig();

        /*
         * Use listener every other time to ensure that the stats are returned
         * correctly when no listener is configured. [#17024]
         */
        useEvolveListener = !useEvolveListener;
        if (useEvolveListener) {
            config.setEvolveListener(new EvolveListener() {
                public boolean evolveProgress(EvolveEvent event) {
                    EvolveStats stats = event.getStats();
                    evolveNRead = stats.getNRead();
                    evolveNConverted = stats.getNConverted();
                    return true;
                }
            });
        }

        openEnv();

        openStoreReadWrite();

        /*
         * Evolve and expect that the expected number of entities are
         * converted.
         */
        int nExpected = caseObj.getNRecordsExpected();
        evolveNRead = 0;
        evolveNConverted = 0;
        PersistCatalog.unevolvedFormatsEncountered = false;
        EvolveStats stats = store.evolve(config);
        if (nExpected > 0) {
            assertTrue(PersistCatalog.unevolvedFormatsEncountered);
        }
        assertTrue(stats.getNRead() == nExpected);
        assertTrue(stats.getNConverted() == nExpected);
        assertTrue(stats.getNConverted() >= stats.getNRead());
        if (useEvolveListener) {
            assertEquals(evolveNRead, stats.getNRead());
            assertEquals(evolveNConverted, stats.getNConverted());
        }

        /* Evolve again and expect that no entities are converted. */
        evolveNRead = 0;
        evolveNConverted = 0;
        PersistCatalog.unevolvedFormatsEncountered = false;
        stats = store.evolve(config);
        assertTrue(!PersistCatalog.unevolvedFormatsEncountered);
        assertEquals(0, stats.getNRead());
        assertEquals(0, stats.getNConverted());
        if (useEvolveListener) {
            assertTrue(evolveNRead == 0);
            assertTrue(evolveNConverted == 0);
        }

        /* Ensure that we can read all entities without evolution. */
        PersistCatalog.unevolvedFormatsEncountered = false;
        caseObj.readObjects(store, false /*doUpdate*/);
        assertTrue(!PersistCatalog.unevolvedFormatsEncountered);

        /*
         * When automatic unused type deletion is implemented in the future the
         * oldTypesExist parameters below should be changed to false.
         */

        /* Open again and try an update. */
        caseObj.checkEvolvedModel
            (store.getModel(), env, true /*oldTypesExist*/);
        caseObj.readObjects(store, true /*doUpdate*/);
        caseObj.checkEvolvedModel
            (store.getModel(), env, true /*oldTypesExist*/);
        closeStore();

        /* Open read-only and double check that everything is OK. */
        assertTrue(openStoreReadOnly());
        caseObj.checkEvolvedModel
            (store.getModel(), env, true /*oldTypesExist*/);
        caseObj.readObjects(store, false /*doUpdate*/);
        caseObj.checkEvolvedModel
            (store.getModel(), env, true /*oldTypesExist*/);
        closeStore();

        /* Check raw objects. */
        openRawStore();
        caseObj.checkEvolvedModel
            (rawStore.getModel(), env, true /*oldTypesExist*/);
        caseObj.readRawObjects
            (rawStore, true /*expectEvolved*/, true /*expectUpdated*/);

        /*
         * Test copy raw object to new store via convertRawObject.  In this
         * test we can pass false for oldTypesExist because newStore starts
         * with the new/evolved class model.
         */
        openNewStore();
        caseObj.copyRawObjects(rawStore, newStore);
        caseObj.readObjects(newStore, true /*doUpdate*/);
        caseObj.checkEvolvedModel
            (newStore.getModel(), env, false /*oldTypesExist*/);
        closeNewStore();
        closeRawStore();

        closeAll();
    }
}
