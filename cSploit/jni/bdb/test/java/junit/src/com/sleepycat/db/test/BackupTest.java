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
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.sleepycat.db.*;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

import com.sleepycat.db.test.TestUtils;
import com.sleepycat.bind.tuple.IntegerBinding;

public class BackupTest {
    public static final String BACKUPTEST_DBNAME = "backuptest.db";
    @BeforeClass public static void ClassInit() {
        TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(BACKUPTEST_DBNAME), true, true);
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(BACKUPTEST_DBNAME));
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(BACKUPTEST_DBNAME), true, true);
    }

    @Before public void PerTestInit()
        throws Exception {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(BACKUPTEST_DBNAME));
        TestUtils.removeall(true, true, TestUtils.BASETEST_BACKUPDIR, TestUtils.getBackupFileName(BACKUPTEST_DBNAME));
    }

    @After public void PerTestShutdown()
        throws Exception {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(BACKUPTEST_DBNAME));
        TestUtils.removeall(true, true, TestUtils.BASETEST_BACKUPDIR, TestUtils.getBackupFileName(BACKUPTEST_DBNAME));
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
        TestUtils.debug_level = 2;
        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        envc.setTransactional(true);
        envc.setInitializeLocking(true);
        envc.setCacheSize(64 * 1024);
        Environment dbEnv = new Environment(TestUtils.BASETEST_DBFILE, envc);

        // Set up a transactional DB.
        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setType(DatabaseType.BTREE);
        dbConfig.setAllowCreate(true);
        Database db = dbEnv.openDatabase(null, BACKUPTEST_DBNAME, null, dbConfig);
        populateDb(db, 1000);
        db.close();

        BackupOptions opt = new BackupOptions();
        opt.setAllowCreate(true);
        opt.setExclusiveCreate(true);
        dbEnv.backup(TestUtils.BASETEST_BACKUPDIR, opt);
        assertTrue(TestUtils.BASETEST_BACKUPFILE.listFiles().length > 0);

        dbEnv.close();
    }

    @Test public void test2()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.debug_level = 2;
        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        envc.setTransactional(true);
        envc.setInitializeLocking(true);
        envc.setCacheSize(64 * 1024);
        Environment dbEnv = new Environment(TestUtils.BASETEST_DBFILE, envc);

        // Set up a transactional DB.
        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setType(DatabaseType.BTREE);
        dbConfig.setAllowCreate(true);
        Database db = dbEnv.openDatabase(null, BACKUPTEST_DBNAME, null, dbConfig);
        populateDb(db, 1000);
        db.close();

        dbEnv.backupDatabase(BACKUPTEST_DBNAME, TestUtils.BASETEST_BACKUPDIR, true);
        assertTrue(TestUtils.BASETEST_BACKUPFILE.listFiles().length == 1);

        dbEnv.close();
    }

    @Test public void test3()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.debug_level = 2;
        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        envc.setTransactional(true);
        envc.setInitializeLocking(true);
        envc.setCacheSize(64 * 1024);
        envc.setBackupReadCount(4096);
        envc.setBackupReadSleep(1000);
        envc.setBackupSize(1024);
        envc.setBackupWriteDirect(true);
        Environment dbEnv = new Environment(TestUtils.BASETEST_DBFILE, envc);

        // Set up a transactional DB.
        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setType(DatabaseType.BTREE);
        dbConfig.setAllowCreate(true);
        Database db = dbEnv.openDatabase(null, BACKUPTEST_DBNAME, null, dbConfig);
        populateDb(db, 1000);
        db.close();

        dbEnv.backupDatabase(BACKUPTEST_DBNAME, TestUtils.BASETEST_BACKUPDIR, true);
        assertTrue(TestUtils.BASETEST_BACKUPFILE.listFiles().length == 1);

        dbEnv.close();
    }

    static class BackupWriter implements BackupHandler {
        RandomAccessFile f;

        public int open(String target, String dbname) {
            try {
                String path = target + '/' + dbname;
                f = new RandomAccessFile(path, "rwd");
            } catch (FileNotFoundException fnfe) {
                return 1;
            }
            return 0;
        }

        public int write(long pos, byte[] buf, int off, int len) {
            try {
                f.seek(pos);
                f.write(buf, off, len);
            } catch (IOException ioe) {
                return 1;
            }
            return 0;
        }

        public int close(String dbname) {
            try {
                f.close();
            } catch (IOException ioe) {
                return 1;
            }
           return 0;
        }
    }

    @Test public void test4()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.debug_level = 2;
        BackupWriter bw = new BackupTest.BackupWriter();

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        envc.setTransactional(true);
        envc.setInitializeLocking(true);
        envc.setCacheSize(64 * 1024);
        envc.setBackupHandler(bw);
        Environment dbEnv = new Environment(TestUtils.BASETEST_DBFILE, envc);

        // Set up a transactional DB.
        DatabaseConfig dbConfig = new DatabaseConfig();
        dbConfig.setType(DatabaseType.BTREE);
        dbConfig.setAllowCreate(true);
        Database db = dbEnv.openDatabase(null, BACKUPTEST_DBNAME, null, dbConfig);
        populateDb(db, 1000);
        db.close();

        BackupOptions opt = new BackupOptions();
        opt.setAllowCreate(true);
        opt.setExclusiveCreate(true);
        dbEnv.backup(TestUtils.BASETEST_BACKUPDIR, opt);
        assertTrue(TestUtils.BASETEST_BACKUPFILE.listFiles().length > 0);

        dbEnv.close();
    }
    
    public void populateDb(Database db, int nrecs) 
        throws DatabaseException {
        byte[] arr = new byte[1024];
        DatabaseEntry key, data;

        data = new DatabaseEntry(arr);
        key = new DatabaseEntry();
        for (int i = 0; i < nrecs; i++) {
            IntegerBinding.intToEntry(i, key);
            db.putNoOverwrite(null, key, data);
        }
    }
}
