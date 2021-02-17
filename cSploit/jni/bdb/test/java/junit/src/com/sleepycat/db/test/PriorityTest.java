/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
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
public class PriorityTest {
    public static final String PRIORITYTEST_DBNAME = "prioritytest.db";
    @BeforeClass public static void ClassInit() {
        TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(PRIORITYTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(PRIORITYTEST_DBNAME));
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(PRIORITYTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(PRIORITYTEST_DBNAME));
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

    @Test public void test1()
        throws DatabaseException, FileNotFoundException
    {
	EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        envc.setInitializeLocking(true);
	envc.setInitializeLogging(true);
	envc.setTransactional(true);
	Environment dbEnv = new Environment(TestUtils.BASETEST_DBFILE, envc);

        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setErrorStream(TestUtils.getErrorStream());
        dbConfig.setErrorPrefix("TxnPriorityTest");
        dbConfig.setType(DatabaseType.BTREE);
        dbConfig.setAllowCreate(true);
	dbConfig.setTransactional(true);
	
	Transaction txn = dbEnv.beginTransaction(null, null);
	txn.setPriority(555);
	
        Database db = dbEnv.openDatabase(txn, PRIORITYTEST_DBNAME, null, dbConfig);
	assertEquals(txn.getPriority(), 555);
	TransactionStats st = dbEnv.getTransactionStats(null);
	TransactionStats.Active[] active = st.getTxnarray();
	assertEquals(active.length, 1);
	assertEquals(active[0].getPriority(), txn.getPriority());
	
	txn.commit();
	db.close();
	
	int lockerid = dbEnv.createLockerID();
	dbEnv.setLockerPriority(lockerid, 558);
	assertEquals(dbEnv.getLockerPriority(lockerid), 558);
	dbEnv.freeLockerID(lockerid);
	
	dbEnv.close();
    }
}
