/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */


package com.sleepycat.db.test;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import com.sleepycat.db.*;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import com.sleepycat.db.test.TestUtils;
public class EnvRegionSizeTest {
    public static final String ENVREGIONSIZETEST_DBNAME = "envregionsizetest.db";
    @BeforeClass public static void ClassInit() {
        TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(ENVREGIONSIZETEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(ENVREGIONSIZETEST_DBNAME));
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(ENVREGIONSIZETEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(ENVREGIONSIZETEST_DBNAME));
    }

    @Before public void PerTestInit()
        throws Exception {
    }

    @After public void PerTestShutdown()
        throws Exception {
    }
    /*
     * Test case implementations.
     * To disable a test mark it with @Ignore
     * To set a timeout(ms) notate like: @Test(timeout=1000)
     * To indicate an expected exception notate like: (expected=Exception)
     */

    @Test public void testRegionMemoryInitialSize()
        throws DatabaseException, FileNotFoundException
    {
        /* Create an environment. */
        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        envc.setRegionMemoryInitialSize(RegionResourceType.LOCK, 1024);
        envc.setRegionMemoryInitialSize(RegionResourceType.LOCK_OBJECT, 1024);
        envc.setRegionMemoryInitialSize(RegionResourceType.LOCKER, 1024);
        envc.setRegionMemoryInitialSize(RegionResourceType.LOG_ID, 1024);
        envc.setRegionMemoryInitialSize(RegionResourceType.TRANSACTION, 1024);
        envc.setRegionMemoryInitialSize(RegionResourceType.THREAD, 1024);
        
        /* Check to see that they "stuck" before opening env. */
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.LOCK), 1024);
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.LOCK_OBJECT), 1024);
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.LOCKER), 1024);
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.LOG_ID), 1024);
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.TRANSACTION), 1024);
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.THREAD), 1024);

        Environment dbEnv = new Environment(TestUtils.BASETEST_DBFILE, envc);

        /* Check again after opnening. */
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.LOCK), 1024);
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.LOCK_OBJECT), 1024);
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.LOCKER), 1024);
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.LOG_ID), 1024);
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.TRANSACTION), 1024);
        assertEquals(
        envc.getRegionMemoryInitialSize(RegionResourceType.THREAD), 1024);
    }

    @Test public void testLockTableSize()
        throws DatabaseException, FileNotFoundException
    {
        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        envc.setLockTableSize(1024);
        assertEquals(envc.getLockTableSize(), 1024);
        Environment dbEnv = new Environment(TestUtils.BASETEST_DBFILE, envc);
        assertEquals(envc.getLockTableSize(), 1024);
    }

    @Test public void testInitialMutexes()
        throws DatabaseException, FileNotFoundException
    {
        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        envc.setInitialMutexes(1024);
        assertEquals(envc.getInitialMutexes(), 1024);
        Environment dbEnv = new Environment(TestUtils.BASETEST_DBFILE, envc);
        assertEquals(envc.getInitialMutexes(), 1024);
    }

    @Test public void testRegionMemoryMax()
        throws DatabaseException, FileNotFoundException
    {
        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        envc.setRegionMemoryMax(5 * 1024 * 1024);
        assertEquals(envc.getRegionMemoryMax(), 5 * 1024 * 1024);
        Environment dbEnv = new Environment(TestUtils.BASETEST_DBFILE, envc);
        assertEquals(envc.getRegionMemoryMax(), 5 * 1024 * 1024);
    }
}
