/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

/*
 * Alexg 23-4-06
 * Based on scr016 TestConstruct01 and TestConstruct02
 * test applications.
 */

package com.sleepycat.db.test;

import org.junit.Before;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import com.sleepycat.db.BtreeStats;
import com.sleepycat.db.Cursor;
import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DatabaseStream;
import com.sleepycat.db.DatabaseStreamConfig;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.HashStats;
import com.sleepycat.db.HeapStats;
import com.sleepycat.db.LockMode;
import com.sleepycat.db.MultipleDataEntry;
import com.sleepycat.db.OperationStatus;
import com.sleepycat.db.PartitionHandler;
import com.sleepycat.db.Transaction;

import java.io.File;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.lang.reflect.Array;
import com.sleepycat.db.test.TestUtils;

public class DatabaseTest {

    public static final String DATABASETEST_DBNAME  =       "databasetest.db";

    private static int itemcount;	// count the number of items in the database
 
    @BeforeClass public static void ClassInit() {
	    TestUtils.loadConfig(null);
        TestUtils.check_file_removed(TestUtils.getDBFileName(DATABASETEST_DBNAME), true, true);
	    TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(DATABASETEST_DBNAME));
        itemcount = 0;
    }

    @AfterClass public static void ClassShutdown() {
        TestUtils.check_file_removed(TestUtils.getDBFileName(DATABASETEST_DBNAME), true, true);
	    TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(DATABASETEST_DBNAME));
    }

    @Before public void PerTestInit()
        throws Exception {
    }

    @After public void PerTestShutdown()
        throws Exception {
    }

    /*
     * Test creating a new database, and then
     * opening and adding records to an existing database.
     */
    @Test public void test1()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR,
            TestUtils.getDBFileName(DATABASETEST_DBNAME));
        itemcount = 0;

        // Create a new database.
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test1 ");

        rundb(itemcount++, options);

        // Open and add records to the existing database.
        rundb(itemcount++, options);
    }

    /*
     * Test modifying the error prefix multiple times.
     */
    @Test public void test2()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR,
            TestUtils.getDBFileName(DATABASETEST_DBNAME));
        itemcount = 0;

        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test2 ");

        for (int i=0; i<100; i++)
            options.db_config.setErrorPrefix("str" + i);

        rundb(itemcount++, options);
    }

    /*
     * Test opening a database with an env.
     */
    @Test public void test3()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR,
            TestUtils.getDBFileName(DATABASETEST_DBNAME));
        itemcount = 0;

        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test3 ");

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
    	options.db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

        rundb(itemcount++, options);
        options.db_env.close();
    }

    /*
     * Test opening multiple databases using the same env.
     */
    @Test public void test4()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR,
            TestUtils.getDBFileName(DATABASETEST_DBNAME));
        itemcount = 0;

        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test4 ");

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
    	options.db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

        rundb(itemcount++, options);

        rundb(itemcount++, options);

        options.db_env.close();
    }

    /*
     * Test just opening and closing a DB and an Env without
     * doing any operations.
     */
    @Test public void test5()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR,
            TestUtils.getDBFileName(DATABASETEST_DBNAME));

        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test5 ");
        options.db_config.setAllowCreate(true);

        Database db =
            new Database(TestUtils.getDBFileName(DATABASETEST_DBNAME),
            null, options.db_config);

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
    	Environment db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

    	db.close();
    	db_env.close();

    	System.gc();
    	System.runFinalization();
    }

    /*
     * test6 leaves a db and dbenv open; it should be detected.
     */
    /* Not sure if relevant with current API.
    @Test public void test6()
        throws DatabaseException, FileNotFoundException
    {
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test6 ");

        Database db = new Database(TestUtils.getDBFileName(DATABASETEST_DBNAME), null, options.db_config);

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
    	Environment db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

	    System.gc();
	    System.runFinalization();
    }
    */
    /*
     * Test leaving database and cursors open won't harm.
     */
    @Test public void test8()
        throws DatabaseException, FileNotFoundException
    {
	System.out.println("\nTest 10 transactional.");
	test8_int(true);
	System.out.println("\nTest 10 non-transactional.");
	test8_int(false);
    }

    void test8_int(boolean havetxn)
        throws DatabaseException, FileNotFoundException
    {
	String name;
	Transaction txn = null;
	itemcount = 0;
	TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(DATABASETEST_DBNAME));
	TestOptions options = new TestOptions();
	options.save_db = true;
	options.db_config.setErrorPrefix("DatabaseTest::test8 ");

	EnvironmentConfig envc = new EnvironmentConfig();
	envc.setAllowCreate(true);
	envc.setInitializeCache(true);
	envc.setPrivate(true);
	if (havetxn) {
		envc.setInitializeLogging(true);
		envc.setInitializeLocking(true);
		envc.setTransactional(true);
	}
	options.db_env = new Environment(null, envc);

	if (havetxn)
		txn = options.db_env.beginTransaction(null, null);
	try {
		options.db_config.setAllowCreate(true);
		options.database = options.db_env.openDatabase(txn, null, null, options.db_config);

		// Acquire a cursor for the database and use it to insert data, but don't close it.
		Cursor dbc = options.database.openCursor(txn, CursorConfig.DEFAULT);

		DatabaseEntry key = new DatabaseEntry();
		DatabaseEntry data = new DatabaseEntry();
		byte bytes[] = new byte[4];
		int ii;
		for (int i = 0; i < 100; i++) {
			ii = i;
			bytes[0] = (byte)ii;
			ii >>= 8;
			bytes[1] = (byte)ii;
			ii >>= 8;
			bytes[2] = (byte)ii;
			ii >>= 8;
			bytes[3] = (byte)ii;
			key.setData(bytes);
			key.setSize(bytes.length);
			data.setData(bytes);
			data.setSize(bytes.length);

			dbc.putKeyLast(key, data);
		}
		// Do not close dbc.

		// Acquire a cursor for the database and use it to read data, but don't close it.
		Cursor csr = options.database.openCursor(txn, CursorConfig.DEFAULT);
		int i = 0;
		while (i < 100 && csr.getNext(key, data, LockMode.DEFAULT) == OperationStatus.SUCCESS) {
			i++;	
		}
		// Do not close csr.
		if (havetxn)
			txn.commit();
	} catch (DatabaseException e) {
		if (havetxn && txn != null)
			txn.abort();
	}
	// Do not close options.database
	options.db_env.closeForceSync();
    }
    /*
     * Test creating a new database.
     */
    @Test public void test7()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR,
            TestUtils.getDBFileName(DATABASETEST_DBNAME));
        itemcount = 0;

        TestOptions options = new TestOptions();
        options.save_db = true;
        options.db_config.setErrorPrefix("DatabaseTest::test7 ");

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
    	options.db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

        // stop rundb from closing the database, and pass in one created.
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);

    	options.database.close();
    	options.database = null;

    	// reopen the same database.
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);
    	rundb(itemcount++, options);

        options.database.close();
    	options.database = null;
        options.db_env.close();

    }

    /*
     * Test setting database handle exclusive lock.
     */
    @Test public void test9()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(DATABASETEST_DBNAME));
        itemcount = 0;
        TestOptions options = new TestOptions();
        options.save_db = true;
        options.db_config.setErrorPrefix("DatabaseTest::test9 ");

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);
        envc.setInitializeLogging(true);
        envc.setInitializeLocking(true);
        envc.setTransactional(true);
        envc.setThreaded(false);
        options.db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);

        rundb(itemcount++, options);
        assertNull(options.database.getConfig().getNoWaitDbExclusiveLock());

        options.database.close();
        options.database = null;

        options.db_config.setNoWaitDbExclusiveLock(Boolean.TRUE);
        rundb(itemcount++, options);
        assertEquals(options.database.getConfig().getNoWaitDbExclusiveLock(), Boolean.TRUE);

        options.database.close();
        options.database = null;

        options.db_config.setNoWaitDbExclusiveLock(Boolean.FALSE);
        rundb(itemcount++, options);
        assertEquals(options.database.getConfig().getNoWaitDbExclusiveLock(), Boolean.FALSE);

        // Test noWaitDbExclusiveLock can not be set after database is opened.
        try {
            DatabaseConfig newConfig = options.database.getConfig();
            newConfig.setNoWaitDbExclusiveLock(Boolean.TRUE);
            options.database.setConfig(newConfig);
        } catch (IllegalArgumentException e) {
        } finally {
            options.database.close();
            options.database = null;
            options.db_env.close();
        }
    }

    /*
     * Test setting metadata directory
     */
    @Test public void test10()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(DATABASETEST_DBNAME));
        itemcount = 0;
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test10 ");

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);

        options.db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);
        rundb(itemcount++, options);
        assertNull(options.db_env.getConfig().getMetadataDir());
        options.db_env.close();

        String mddir = "metadataDir";
        envc.setMetadataDir(new java.io.File(mddir));
        options.db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);
        rundb(itemcount++, options);
        assertEquals(options.db_env.getConfig().getMetadataDir().getPath(), mddir);
        options.db_env.close();
    }

    /*
     * Test setting heap region size
     */
    @Test public void test11()
        throws DatabaseException, FileNotFoundException
    {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR, TestUtils.getDBFileName(DATABASETEST_DBNAME));
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test11 ");
        options.db_config.setAllowCreate(true);
        options.db_config.setType(DatabaseType.HEAP);
        options.db_config.setHeapRegionSize(4);

        Database db = new Database(TestUtils.getDBFileName(DATABASETEST_DBNAME), null, options.db_config);
        assertEquals(db.getConfig().getHeapRegionSize(), 4);

        HeapStats stats = (HeapStats)db.getStats(null, null);
        assertEquals(stats.getHeapRegionSize(), 4);

        db.close();
    }

    /*
     * Test creating partition database by keys
     */
    @Test public void test12()
        throws DatabaseException, Exception, FileNotFoundException
    {
        // Test the success case
        String errpfx = "DatabaseTest::test12 ";

        // Create the partition key
        int parts = 3;
        MultipleDataEntry keyRanges = new MultipleDataEntry();
        keyRanges.setData(new byte[1024]);
        keyRanges.setUserBuffer(1024, true);

        DatabaseEntry kdbt1 = new DatabaseEntry();
        DatabaseEntry kdbt2 = new DatabaseEntry();
        kdbt1.setData("d".getBytes());
        kdbt2.setData("g".getBytes());

        keyRanges.append(kdbt1);
        keyRanges.append(kdbt2);

        // Success case: set partition by key
        test_partition_db(parts, keyRanges, null, 0, errpfx);

        // Test the exception case: parts != (size of key array + 1)
        parts++;
        try {
            test_partition_db(parts, keyRanges, null, 0, errpfx);
            throw new Exception("Unexpected exception: setPartitionByRange" +
               "should fail as parts != (size of key array + 1).");
        } catch (IllegalArgumentException e) {
        }

        // Test the exception case: keys == null
        try {
            test_partition_db(parts, null, null, 0, errpfx);
            throw new Exception("Unexpected exception: database open should" +
                "fail as partition key array and callback are null.");
        } catch (IllegalArgumentException e) {
        }

        // Test the exception case: there is no data in the keys
        try {
            test_partition_db(parts, new MultipleDataEntry(), null, 0, errpfx);
            throw new Exception("Unexpected exception: database open should" +
                "fail as there is no data in the partition keys which is" +
                "a MultipleDataEntry. ");
        } catch (IllegalArgumentException e) {
        }

        // Test the exception case: parts == 1
        try {
            test_partition_db(1, null, null, 2, errpfx);
            throw new Exception("Unexpected exception: database open should" +
                "fail as the number of partition is set to 1.");
        } catch (IllegalArgumentException e) {
        }

    }

    /*
     * Test creating partition database by callback
     */
    @Test public void test13()
        throws DatabaseException, Exception, FileNotFoundException
    {
        String errpfx = "DatabaseTest::test13 ";

        // Success case: set partition by callback
        PartitionCallback part_func = new PartitionCallback();
        int parts = 2;
        test_partition_db(parts, null, part_func, 1, errpfx);

        // Test the exception case: callback and key array are both set
        MultipleDataEntry keyRanges = new MultipleDataEntry();
        keyRanges.setData(new byte[1024]);
        keyRanges.setUserBuffer(1024, true);
        DatabaseEntry kdbt = new DatabaseEntry();
        kdbt.setData("b".getBytes());
        keyRanges.append(kdbt);

        try {
            test_partition_db(parts, keyRanges, part_func, 2, errpfx);
            throw new Exception("Unexpected exception: database open should " +
                "fail as partition callback and key array are both set.");
        } catch (IllegalArgumentException e) {
        }

    }

    /*
     * Test setting the blob directory and threshold value.
     */
    @Test public void test14()
        throws DatabaseException, Exception, FileNotFoundException
    {
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix("DatabaseTest::test14 ");
        options.save_db = true;

        EnvironmentConfig envc = new EnvironmentConfig();
        envc.setAllowCreate(true);
        envc.setInitializeCache(true);

        // Test setting the blob directory.
        String dir[] = {"null", "", "BLOBDIR"};
        for (int i = -1; i < dir.length; i++) {
            TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR,
                TestUtils.getDBFileName(DATABASETEST_DBNAME));
            // Set the blob directory.
            if (i >= 0) {
                if (dir[i].compareTo("null") == 0)
                    envc.setBlobDir(null);
                else
                    envc.setBlobDir(new java.io.File(dir[i]));
            }
            // Set the blob threshold value.
            envc.setBlobThreshold(10485760);
            // Open the env.
            options.db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);
            // Verify the blob directory and threshold value.
            assertEquals(10485760,
                options.db_env.getConfig().getBlobThreshold());
            if (i == -1 || dir[i].compareTo("null") == 0)
                assertNull(options.db_env.getConfig().getBlobDir());
            else
                assertEquals(0, options.db_env.getConfig().getBlobDir().
                    toString().compareTo(dir[i]));
            options.db_env.close();
        }

        // Test setting the db blob threshold value and open it with no env.
        test_blob_db(0, null, 3,
            TestUtils.BASETEST_DBDIR + File.separator + "DBBLOB",
            0, "DatabaseTest::test14 ", DatabaseType.BTREE);

        // Test setting the blob directory in the database and then
        // opening the db within env and verifying the setting is ignored.
        test_blob_db(3, null, 0, "DBBLOB",
            0, "DatabaseTest::test14 ", DatabaseType.BTREE);
        test_blob_db(3, "ENVBLOB", 0, "DBBLOB",
            0, "DatabaseTest::test14 ", DatabaseType.BTREE);

        // Test setting the blob directory in the environment.
        test_blob_db(3, "ENVBLOB", 0, null,
            0, "DatabaseTest::test14 ", DatabaseType.BTREE);

        // Test the db blob threshold value defaults to env blob threshold
        // value.
        test_blob_db(3, null, 0, null,
            0, "DatabaseTest::test14 ", DatabaseType.BTREE);

        // Test setting db own blob threshold and open it within the env.
        test_blob_db(4, null, 3, null,
            0, "DatabaseTest::test14 ", DatabaseType.HASH);

        // Test putting the data items whose size does not reach the blob
        // threshold but set as blob data items.
        test_blob_db(3, null, 0, null,
            1, "DatabaseTest::test14 ", DatabaseType.HEAP);
    }

    // Check that key/data for 0 - count-1 are already present,
    // and write a key/data for count.  The key and data are
    // both "0123...N" where N == count-1.
    //
    // For some reason on Windows, we need to open using the full pathname
    // of the file when there is no environment, thus the 'has_env'
    // variable.
    //
    void rundb(int count, TestOptions options)
            throws DatabaseException, FileNotFoundException
    {
    	String name;

        Database db;
        if(options.database == null)
        {
        	if (options.db_env != null)
        	    name = DATABASETEST_DBNAME;
        	else
        	    name = TestUtils.getDBFileName(DATABASETEST_DBNAME);

            if(count == 0)
                options.db_config.setAllowCreate(true);

            if(options.db_env == null)
                db = new Database(name, null, options.db_config);
            else
                db = options.db_env.openDatabase(options.txn, name, null, options.db_config);
        } else {
            db = options.database;
        }

    	// The bit map of keys we've seen
    	long bitmap = 0;

    	// The bit map of keys we expect to see
    	long expected = (1 << (count+1)) - 1;

    	byte outbuf[] = new byte[count+1];
    	int i;
    	for (i=0; i<count; i++) {
    		outbuf[i] = (byte)('0' + i);
    	}
    	outbuf[i++] = (byte)'x';

    	DatabaseEntry key = new DatabaseEntry(outbuf, 0, i);
    	DatabaseEntry data = new DatabaseEntry(outbuf, 0, i);

    	TestUtils.DEBUGOUT("Put: " + (char)outbuf[0] + ": " + new String(outbuf, 0, i));
    	db.putNoOverwrite(options.txn, key, data);

    	// Acquire a cursor for the table.
    	Cursor dbcp = db.openCursor(options.txn, CursorConfig.DEFAULT);

    	// Walk through the table, checking
    	DatabaseEntry readkey = new DatabaseEntry();
    	DatabaseEntry readdata = new DatabaseEntry();
    	DatabaseEntry whoknows = new DatabaseEntry();

    	/*
    	 * NOTE: Maybe want to change from user-buffer to DB buffer
    	 *       depending on the flag options.user_buffer (setReuseBuffer)
    	 * The old version set MALLOC/REALLOC here - not sure if it is the same.
    	 */

    	TestUtils.DEBUGOUT("Dbc.get");
    	while (dbcp.getNext(readkey, readdata, LockMode.DEFAULT) == OperationStatus.SUCCESS) {
    		String key_string =
    		new String(readkey.getData(), 0, readkey.getSize());
    		String data_string =
    		new String(readdata.getData(), 0, readkey.getSize());
    		TestUtils.DEBUGOUT("Got: " + key_string + ": " + data_string);
    		int len = key_string.length();
    		if (len <= 0 || key_string.charAt(len-1) != 'x') {
    			TestUtils.ERR("reread terminator is bad");
    		}
    		len--;
    		long bit = (1 << len);
    		if (len > count) {
    			TestUtils.ERR("reread length is bad: expect " + count + " got "+ len + " (" + key_string + ")" );
    		}
    		else if (!data_string.equals(key_string)) {
    			TestUtils.ERR("key/data don't match");
    		}
    		else if ((bitmap & bit) != 0) {
    			TestUtils.ERR("key already seen");
    		}
    		else if ((expected & bit) == 0) {
    			TestUtils.ERR("key was not expected");
    		}
    		else {
    			bitmap |= bit;
    			expected &= ~(bit);
    			for (i=0; i<len; i++) {
    				if (key_string.charAt(i) != ('0' + i)) {
    					System.out.print(" got " + key_string
    					+ " (" + (int)key_string.charAt(i)
    					+ "), wanted " + i
    					+ " (" + (int)('0' + i)
    					+ ") at position " + i + "\n");
    					TestUtils.ERR("key is corrupt");
    				}
    			}
    		}
    	}
    	if (expected != 0) {
    		System.out.print(" expected more keys, bitmap is: " + expected + "\n");
    		TestUtils.ERR("missing keys in database");
    	}

    	dbcp.close();
    	TestUtils.DEBUGOUT("options.save_db " + options.save_db + " options.database " + options.database);
    	if(options.save_db == false)
    	    db.close(false);
    	else if (options.database == null)
    	    options.database = db;
    }

    // Test if setPartitionByRange and setPartitionByCallback work by the
    // following steps: 1) config the partition by keys and/or callback;
    // 2) open the database; 3) insert some records; 4) verify the partition
    // configs; 5) close the database.
    //
    // The parameter "apicall" indicates which API is tested. If it is 0,
    // test setPartitionByRange. If it is 1, test setPartitionByCallback.
    // Otherwise test both of them.
    void test_partition_db(int nparts, MultipleDataEntry keys,
        PartitionHandler funcp, int apicall, String errpfx)
        throws DatabaseException, FileNotFoundException,
            IllegalArgumentException
    {
        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR,
            TestUtils.getDBFileName(DATABASETEST_DBNAME));
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix(errpfx);
        options.db_config.setAllowCreate(true);
        options.db_config.setType(DatabaseType.BTREE);
        // Config the partition.
        // Parameter apicall:
        // If 0 then call setPartitionByRange;
        // If 1 then call setPartitionByCallback;
        // Otherwise call both.
        if (apicall == 0)
            options.db_config.setPartitionByRange(nparts, keys);
        else if (apicall == 1)
            options.db_config.setPartitionByCallback(nparts, funcp);
        else {
            options.db_config.setPartitionByRange(nparts, keys);
            options.db_config.setPartitionByCallback(nparts, funcp);
        }

        // Open the database.
        Database db = new Database(
            TestUtils.getDBFileName(DATABASETEST_DBNAME),
            null, options.db_config);

        // Insert some records.
        String[] records = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
            "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w",
            "x", "y", "z"};
        DatabaseEntry ddbt, kdbt;
        for (int i = 0; i < records.length; i++) {
           kdbt = new DatabaseEntry();
           ddbt = new DatabaseEntry();
           kdbt.setData(records[i].getBytes());
           ddbt.setData(records[i].getBytes());
           db.putNoOverwrite(null, kdbt, ddbt);
        }

        // Verify the number of partitions.
        assertEquals(nparts, db.getConfig().getPartitionParts());

        // Verify the number of partitioned files.
        File testdir = new File(TestUtils.BASETEST_DBDIR);
        File[] flist = testdir.listFiles();
        int cnt = 0;
        for (int i = 0; i < Array.getLength(flist); i++) {
            if (flist[i].getName().substring(0, 6).compareTo("__dbp.") == 0)
                cnt++;
        }
        assertEquals(nparts, cnt);

        // Verify the keys.
        if (keys != null) {
            MultipleDataEntry orig_key = new MultipleDataEntry(keys.getData());
            MultipleDataEntry get_key =new MultipleDataEntry(
                db.getConfig().getPartitionKeys().getData());
            String s1, s2;
            for (kdbt = new DatabaseEntry(), ddbt = new DatabaseEntry();
                orig_key.next(kdbt) == true;
                kdbt = new DatabaseEntry(), ddbt = new DatabaseEntry()) {
                assertEquals(true, get_key.next(ddbt));
                s1 = new String(kdbt.getData(), kdbt.getOffset(), kdbt.getSize());
                s2 = new String(ddbt.getData(), ddbt.getOffset(), ddbt.getSize());
                assertEquals(0, s1.compareTo(s2));
            }
            assertEquals(false, get_key.next(ddbt));
        }

        // Verify the callback.
        assertEquals(funcp, db.getConfig().getPartitionHandler());

        // Close the database.
        db.close();

    }

    // Test if the BLOB basic APIs work by the following steps:
    // 1) configure the blob threshold value and blob directory;
    // 2) open the database with/without the environment;
    // 3) insert and verify the blob data by database methods;
    // 4) insert blob data by cursor, update the blob data and verify
    // the update by database stream;
    // 5) verify the blob configs, whether the blobs are created in
    // expected location and the stats;
    // 6) close the database and environment.
    //
    // The parameter "env_threshold" indicates the blob threshold value
    // set in the environment and whether the database is opened within
    // the enviornment. If it is <= 0, open the database without the
    // enviornment. Otherwise open the database within the enviornment.
    // The parameter "blobdbt" indicates whether DatabaseEntry.setBlob()
    // is called on the data items to put. If it is not 0, set the data
    // items as blob data and make its size < the blob threshold. Otherwise
    // make the size of the data item reach the threshold and do not set
    // the data item as blob data.
    void test_blob_db(int env_threshold, String env_blob_dir,
        int db_threshold, String db_blob_dir, int blobdbt,
        String errpfx, DatabaseType dbtype)
        throws DatabaseException, Exception, FileNotFoundException
    {
        // The blob threshold is set at least once either in the environment
        // or in the database.
        if (env_threshold <= 0 && db_threshold <= 0)
            return;

        TestUtils.removeall(true, true, TestUtils.BASETEST_DBDIR,
            TestUtils.getDBFileName(DATABASETEST_DBNAME));
        TestOptions options = new TestOptions();
        options.db_config.setErrorPrefix(errpfx);
        options.db_config.setAllowCreate(true);
        options.db_config.setType(dbtype);

        // Configure and open the environment.
        EnvironmentConfig envc = new EnvironmentConfig();
        if (env_threshold <= 0)
            options.db_env = null;
        else {
            envc.setAllowCreate(true);
            envc.setErrorStream(TestUtils.getErrorStream());
            envc.setInitializeCache(true);
            envc.setBlobThreshold(env_threshold);
            if (env_blob_dir != null)
                envc.setBlobDir(new java.io.File(env_blob_dir));
            options.db_env = new Environment(TestUtils.BASETEST_DBFILE, envc);
        }

        // Configure and open the database.
        if (db_threshold > 0)
            options.db_config.setBlobThreshold(db_threshold);
        if (db_blob_dir != null)
            options.db_config.setBlobDir(new java.io.File(db_blob_dir));
        if (options.db_env == null)
            options.database =
                new Database(TestUtils.getDBFileName(DATABASETEST_DBNAME),
                null, options.db_config);
        else {
            options.database = options.db_env.openDatabase(null,
                DATABASETEST_DBNAME, null, options.db_config);
        }

        // Insert and verify some blob data by database method, and then
        // update the blob data by database stream and verify the update.
        Cursor cursor = options.database.openCursor(null, null);
        DatabaseStream dbs;
        DatabaseStreamConfig dbs_config = new DatabaseStreamConfig();
        dbs_config.setSyncPerWrite(true);
        assertEquals(true, dbs_config.getSyncPerWrite());
        assertEquals(false, dbs_config.getReadOnly());
        String[] records = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
            "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w",
            "x", "y", "z"};
        DatabaseEntry ddbt, kdbt, sdbt;
        String data;
        for (int i = 0; i < records.length; i++) {
            kdbt = new DatabaseEntry();
            ddbt = new DatabaseEntry();
            kdbt.setData(records[i].getBytes());
            data = records[i];
            if (blobdbt != 0) {
                ddbt.setBlob(true);
                assertTrue(ddbt.getBlob());
            } else {
                for (int j = 1;
                    j < options.database.getConfig().getBlobThreshold(); j++)
                    data = data + records[i];
            }
            ddbt.setData(data.getBytes());
            if (dbtype == DatabaseType.HEAP)
                options.database.append(null, kdbt, ddbt);
            else
                options.database.put(null, kdbt, ddbt);

            // Verify the blob data by database get method.
            assertEquals(OperationStatus.SUCCESS,
                options.database.get(null, kdbt, ddbt, null));
            assertArrayEquals(data.getBytes(), ddbt.getData());

            // Update the blob data by database stream and verify the update.
            assertEquals(OperationStatus.SUCCESS,
                cursor.getSearchKey(kdbt, ddbt, null));
            dbs = cursor.openDatabaseStream(dbs_config);
            assertEquals(data.length(), dbs.size());
            sdbt = new DatabaseEntry("abc".getBytes());
            assertEquals(OperationStatus.SUCCESS, dbs.write(sdbt, dbs.size()));
            assertEquals(OperationStatus.SUCCESS,
                dbs.read(sdbt, 0, (int)dbs.size()));
            assertArrayEquals((data + "abc").getBytes(), sdbt.getData());
            dbs.close();
        }
        cursor.close();

        // Insert the blob data by cursor, update the blob data by database
        // stream and verify the update.
        if (dbtype != DatabaseType.HEAP) {
            cursor = options.database.openCursor(null, null);
            kdbt = new DatabaseEntry("abc".getBytes());
            ddbt = new DatabaseEntry("abc".getBytes());
            ddbt.setBlob(true);
            assertEquals(true, ddbt.getBlob());
            assertEquals(OperationStatus.SUCCESS,
                cursor.putKeyFirst(kdbt, ddbt));

            dbs = cursor.openDatabaseStream(dbs_config);
            assertEquals(3, dbs.size());
            sdbt = new DatabaseEntry("defg".getBytes());
            assertEquals(OperationStatus.SUCCESS, dbs.write(sdbt, dbs.size()));
            dbs.close();

            // Verify the update and that database stream can not write when it
            // is configured to be read-only.
            dbs_config.setReadOnly(true);
            assertEquals(true, dbs_config.getReadOnly());
            dbs = cursor.openDatabaseStream(dbs_config);
            assertEquals(7, dbs.size());
            assertEquals(OperationStatus.SUCCESS,
                dbs.read(sdbt, 0, (int)dbs.size()));
            assertArrayEquals("abcdefg".getBytes(), sdbt.getData());
            try {
                dbs.write(sdbt, 7);
                throw new Exception("database stream write should fail"
                    + "as it is configured to be read-only");
            } catch (IllegalArgumentException e) {
            }
            dbs.close();

            cursor.close();
        }

        // Verify the blob config of the enviornment.
        if (options.db_env != null && env_threshold > 0) {
            assertEquals(env_threshold,
                options.db_env.getConfig().getBlobThreshold());
            if (env_blob_dir == null)
                assertNull(options.db_env.getConfig().getBlobDir());
            else
                assertEquals(0, options.db_env.getConfig().
                    getBlobDir().toString().compareTo(env_blob_dir));
        }

        // Verify the blob config of the database.
        assertEquals(db_threshold > 0 ? db_threshold : env_threshold,
            options.database.getConfig().getBlobThreshold());
        String blrootdir;
        if (options.db_env != null) {
            if (env_blob_dir == null)
                blrootdir = "__db_bl";
            else
                blrootdir = env_blob_dir;
        } else if (db_blob_dir == null) {
            blrootdir = "__db_bl";
        } else {
            blrootdir = db_blob_dir;
        }
        assertEquals(0, options.database.getConfig().
            getBlobDir().toString().compareTo(blrootdir));

        // Verify the blobs are created in the expected location.
        // This part of test is disabled since the Database.getBlobSubDir()
        // is not expsed to users.
        //if (options.db_env != null)
        //    blrootdir = options.db_env.getHome().toString() + "/" + blrootdir;
        //assertNotNull(options.database.getBlobSubDir().toString());
        //File blobdir = new File(blrootdir + "/" +
        //    options.database.getBlobSubDir().toString());
        //assertTrue(blobdir.listFiles().length > records.length);

        // Verify the stats.
        if (dbtype == DatabaseType.HASH) {
            HashStats stats = (HashStats)options.database.getStats(null, null);
            assertEquals(records.length + 1, stats.getNumBlobs());
        } else if (dbtype == DatabaseType.HEAP) {
            HeapStats stats = (HeapStats)options.database.getStats(null, null);
            assertEquals(records.length, stats.getHeapNumBlobs());
        } else {
            BtreeStats stats =
               (BtreeStats)options.database.getStats(null, null);
            assertEquals(records.length + 1, stats.getNumBlobs());
        }

        // Close the database and set up the blob directory configuration
        // used in removing the database.
        options.database.close();
        if (options.db_env != null)
            blrootdir = TestUtils.BASETEST_DBDIR + File.separator + blrootdir;
        options.db_config.setBlobDir(new File(blrootdir));

        // TestUtils.removeall does not work on the blob database since it
        // removes the database with the default database configuration. So
        // remove the blob database with blob configuration here.
        Database.remove(TestUtils.getDBFileName(DATABASETEST_DBNAME),
            null, options.db_config);

        // All blobs are deleted but the blob directory remains after db
        // remove. Verify it and delete the blob directory.
        File[] files = options.db_config.getBlobDir().listFiles();
        assertTrue(files.length > 0);
        for (int i = 0; i < files.length; i++) {
            if (files[i].isDirectory())
                assertEquals(0, files[i].listFiles().length);
        }
        TestUtils.removeDir(blrootdir);

        // Close the environment.
        if (options.db_env != null)
            options.db_env.close();           
    }
}


class TestOptions
{
    int testmask = 0;           // which tests to run
    int user_buffer = 0;    // use DB_DBT_USERMEM or DB_DBT_MALLOC
    int successcounter =0;
    boolean save_db = false;
    Environment db_env = null;
    DatabaseConfig db_config;
    Database database = null; // db is saved here by rundb if save_db is true.
    Transaction txn = null;

    public TestOptions()
    {
        this.testmask = 0;
        this.user_buffer = 0;
        this.successcounter = 0;
        this.db_env = null;
        this.txn = null;

        db_config = new DatabaseConfig();
        db_config.setErrorStream(TestUtils.getErrorStream());
        db_config.setErrorPrefix("DatabaseTest");
        db_config.setType(DatabaseType.BTREE);
    	// We don't really care about the pagesize
        db_config.setPageSize(1024);

    }

}

class PartitionCallback implements PartitionHandler
{
    public int partition(Database db, DatabaseEntry key)
    {
        String data = new String(key.getData());

        if (data.compareTo("d") >= 0)
            return 1;
        else
            return 0;
    }
}
