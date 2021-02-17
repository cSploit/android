/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest {
    [TestFixture]
    public class HeapDatabaseTest : DatabaseTest {

        [TestFixtureSetUp]
        public void SetUpTestFixture() {
            testFixtureName = "HeapDatabaseTest";
            base.SetUpTestfixture();
        }

        [Test]
        public void TestAppendWithoutTxn() {
            testName = "TestAppendWithoutTxn";
            SetUpTest(true);
            string heapDBFileName = testHome + "/" + testName + ".db";


            HeapDatabaseConfig heapConfig = new HeapDatabaseConfig();
            heapConfig.Creation = CreatePolicy.ALWAYS;
            HeapDatabase heapDB = HeapDatabase.Open(
                heapDBFileName, heapConfig);

            byte[] byteArr = new byte[4];
            byteArr = BitConverter.GetBytes((int)1);
            DatabaseEntry data = new DatabaseEntry(byteArr);
            HeapRecordId rid = heapDB.Append(data);

            // Confirm that the record lives on a page larger than 0.
            Assert.AreNotEqual(0, rid.PageNumber);

            // Confirm that the record exists in the database.
            DatabaseEntry key = new DatabaseEntry(rid.toArray());
            Assert.IsTrue(heapDB.Exists(key));
            heapDB.Close();
        }

        [Test]
        public void TestAppendWithTxn() {
            testName = "TestAppendWithTxn";
            SetUpTest(true);
            string heapDBFileName = testHome + "/" + testName + ".db";
            string heapDBName =
                Path.GetFileNameWithoutExtension(heapDBFileName);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseTxns = true;
            envConfig.UseMPool = true;

            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);
            Transaction txn = env.BeginTransaction();

            HeapDatabaseConfig heapConfig = new HeapDatabaseConfig();
            heapConfig.Creation = CreatePolicy.ALWAYS;
            heapConfig.Env = env;
            
            /* If environmnet home is set, the file name in
             * Open() is the relative path.
             */
            HeapDatabase heapDB = HeapDatabase.Open(
                heapDBName, heapConfig, txn);
            DatabaseEntry data;
            int i = 1000;
            try {
                while (i > 0) {
                    data = new DatabaseEntry(
                        BitConverter.GetBytes(i));
                    heapDB.Append(data, txn);
                    i--;
                }
                txn.Commit();
            } catch {
                txn.Abort();
            } finally {
                heapDB.Close();
                env.Close();
            }

        }

	[Test]
	public void TestBlob() {
		testName = "TestBlob";
		SetUpTest(false);
		// Test opening the blob database without environment.
		TestBlobHeapDatabase(0, null, 6, null, false);

		/*
		 * Test opening the blob database without environment
		 * but specifying blob directory.
		 */
		TestBlobHeapDatabase(0, null, 6, testHome + "/DBBLOB", true);

		// Test opening the blob database with environment.
		TestBlobHeapDatabase(3, "ENVBLOB", 6, null, false);

		/*
		 * Test opening the blob database with environment
		 * and specifying blob directory.
		 */
		TestBlobHeapDatabase(3, null, 6, "/DBBLOB", true);
	}

	/*
	 * Test the blob database with or without environment.
	 * 1. Config and open the environment;
	 * 2. Verify the environment blob configs;
	 * 3. Config and open the database;
	 * 4. Verify the database blob configs;
	 * 5. Insert and verify some blob data by database methods;
	 * 6. Verify the stats;
	 * 7. Close all handles.
	 * If "blobdbt" is true, set the data DatabaseEntry.Blob as
	 * true, otherwise make the data DatabaseEntry reach the blob
	 * threshold in size.
	 */
	void TestBlobHeapDatabase(uint env_threshold, string env_blobdir,
	    uint db_threshold, string db_blobdir, bool blobdbt)
	{
		if (env_threshold == 0 && db_threshold == 0)
			return;

		string heapDBName =
		    testHome + "/" + testName + ".db";

		Configuration.ClearDir(testHome);
		HeapDatabaseConfig cfg = new HeapDatabaseConfig();
		cfg.Creation = CreatePolicy.ALWAYS;
		string blrootdir = "__db_bl";

		// Open the environment and verify the blob configs.
		if (env_threshold > 0)
		{
			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.AutoCommit = true;
			envConfig.Create = true;
			envConfig.UseMPool = true;
			envConfig.UseLogging = true;
			envConfig.UseTxns = true;
			envConfig.UseLocking = true;
			envConfig.BlobThreshold = env_threshold;
			if (env_blobdir != null)
			{
				envConfig.BlobDir = env_blobdir;
				blrootdir = env_blobdir;
			}
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);
			if (env_blobdir == null)
				Assert.IsNull(env.BlobDir);
			else
				Assert.AreEqual(0,
				    env.BlobDir.CompareTo(env_blobdir));
			Assert.AreEqual(env_threshold, env.BlobThreshold);
			cfg.Env = env;
			heapDBName = testName + ".db";
		}

		// Open the database and verify the blob configs.
		if (db_threshold > 0)
			cfg.BlobThreshold = db_threshold;
		if (db_blobdir != null)
		{
			cfg.BlobDir = db_blobdir;
			/*
			 * The blob directory setting in the database
			 * is effective only when it is opened without
			 * an environment.
			 */
			if (cfg.Env == null)
				blrootdir = db_blobdir;
		}

		HeapDatabase db = HeapDatabase.Open(heapDBName, cfg);
		Assert.AreEqual(
		    db_threshold > 0 ? db_threshold : env_threshold,
		    db.BlobThreshold);
		if (db_blobdir == null && cfg.Env == null)
			Assert.IsNull(db.BlobDir);
		else
			Assert.AreEqual(0, db.BlobDir.CompareTo(blrootdir));

		// Insert and verify some blob data by database methods.
		string[] records = {"a", "b", "c", "d", "e", "f", "g", "h",
		    "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s",
		    "t", "u", "v", "w", "x", "y", "z"};
		DatabaseEntry kdbt = new DatabaseEntry();
		DatabaseEntry ddbt = new DatabaseEntry();
		byte[] ddata;
		string str;
		KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
		ddbt.Blob = blobdbt;
		Assert.AreEqual(blobdbt, ddbt.Blob);
		for (int i = 0; i < records.Length; i++)
		{
			str = records[i];
			if (!blobdbt)
			{
				for (int j = 0; j < db_threshold; j++)
					str = str + records[i];
			}
			ddata = Encoding.ASCII.GetBytes(str);
			ddbt.Data = ddata;
			kdbt = new DatabaseEntry((db.Append(ddbt)).toArray()) ;
			try
			{
				pair = db.Get(kdbt);
			}
			catch (DatabaseException)
			{
				db.Close();
				if (cfg.Env != null)
					cfg.Env.Close();
				throw new TestException();
			}
			Assert.AreEqual(ddata, pair.Value.Data);
		}

		/*
		 * Verify the blob files are created in the expected location.
		 * This part of test is disabled since BTreeDatabase.BlobSubDir
		 * is not exposed to users.
		 */
		//if (cfg.Env != null)
		//	blrootdir = testHome + "/" + blrootdir;
		//string blobdir = blrootdir + "/" + db.BlobSubDir;
		//Assert.AreEqual(records.Length,
		//    Directory.GetFiles(blobdir, "__db.bl*").Length);
		//Assert.AreEqual(1,
		//    Directory.GetFiles(blobdir, "__db_blob_meta.db").Length);

		// Verify the stats.
		HeapStats st = db.Stats();
		Assert.AreEqual(records.Length, st.nBlobRecords);

		// Close all handles.
		db.Close();
		if (cfg.Env != null)
			cfg.Env.Close();

		/*
		 * Remove the default blob directory
		 * when it is not under the test home.
		 */
		if (db_blobdir == null && cfg.Env == null)
			Directory.Delete("__db_bl", true);
	}

        [Test]
        public void TestCursor() {
            testName = "TestCursor";
            SetUpTest(true);

            GetCursur(testHome + "/" + testName + ".db", false);
        }

        [Test]
        public void TestCursorWithConfig() {
            testName = "TestCursorWithConfig";
            SetUpTest(true);

            GetCursur(testHome + "/" + testName + ".db", true);
        }

        public void GetCursur(string dbFileName, bool ifConfig) {
            HeapDatabaseConfig dbConfig = new HeapDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            HeapDatabase db = HeapDatabase.Open(dbFileName, dbConfig);
            HeapRecordId rid = db.Append(
                new DatabaseEntry(BitConverter.GetBytes((int)1)));
            Cursor cursor;
            CursorConfig cursorConfig = new CursorConfig();
            cursorConfig.Priority = CachePriority.HIGH;
            if (ifConfig == false)
                cursor = db.Cursor();
            else
                cursor = db.Cursor(cursorConfig);
            cursor.Add(new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                new DatabaseEntry(rid.toArray()),
                new DatabaseEntry(BitConverter.GetBytes((int)2))));
            Cursor dupCursor = cursor.Duplicate(false);
            Assert.IsNull(dupCursor.Current.Key);
            if (ifConfig)
                Assert.AreEqual(CachePriority.HIGH, dupCursor.Priority);
            dupCursor.Close();
            cursor.Close();
            db.Close();
        }

        [Test]
        public void TestExists() {
            testName = "TestExists";
            SetUpTest(true);

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.UseLocking = true;
            envConfig.UseLogging = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                testHome, envConfig);

            HeapDatabase db;
            try {
                Transaction openTxn = env.BeginTransaction();
                try {
                    HeapDatabaseConfig heapConfig =
                        new HeapDatabaseConfig();
                    heapConfig.Creation = CreatePolicy.IF_NEEDED;
                    heapConfig.Env = env;
                    db = HeapDatabase.Open(testName + ".db",
                        heapConfig, openTxn);
                    openTxn.Commit();
                } catch (DatabaseException e) {
                    openTxn.Abort();
                    throw e;
                }

                Transaction cursorTxn = env.BeginTransaction();
                Cursor cursor;
                HeapRecordId rid;
                try {
                    /*
                     * Put a record into heap database with 
                     * cursor and abort the operation.
                     */
                    rid = db.Append(new DatabaseEntry(
                        ASCIIEncoding.ASCII.GetBytes("foo")), cursorTxn);
                    cursor = db.Cursor(cursorTxn);
                    KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
                    pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
                        new DatabaseEntry(rid.toArray()),
                        new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("bar")));
                    cursor.Add(pair);
                    cursor.Close();
                    Assert.IsTrue(db.Exists(
                        new DatabaseEntry(rid.toArray()), cursorTxn));
                    cursorTxn.Abort();
                } catch (DatabaseException e) {
                    cursorTxn.Abort();
                    db.Close();
                    throw e;
                }

                Transaction delTxn = env.BeginTransaction();
                try {
                    /*
                     * The put operation is aborted in the heap 
                     * database so querying if the record still exists 
                     * throws KeyEmptyException.
                     */
                    Assert.IsFalse(db.Exists(
                        new DatabaseEntry(rid.toArray()), delTxn));
                    delTxn.Commit();
                } catch (DatabaseException e) {
                    delTxn.Abort();
                    throw e;
                } finally {
                    db.Close();
                }
            } finally {
                env.Close();
            }
        }

        [Test]
        public void TestOpenExistingHeapDB() {
            testName = "TestOpenExistingHeapDB";
            SetUpTest(true);
            string heapDBFileName = testHome + "/" + testName + ".db";

            HeapDatabaseConfig heapConfig = new HeapDatabaseConfig();
            heapConfig.Creation = CreatePolicy.ALWAYS;
            HeapDatabase heapDB = HeapDatabase.Open(
                heapDBFileName, heapConfig);
            heapDB.Close();

            DatabaseConfig dbConfig = new DatabaseConfig();
            Database db = Database.Open(heapDBFileName, dbConfig);
            Assert.AreEqual(db.Type, DatabaseType.HEAP);
            db.Close();
        }

        [Test]
        public void TestOpenNewHeapDB() {
            testName = "TestOpenNewHeapDB";
            SetUpTest(true);
            string heapDBFileName = testHome + "/" + testName + ".db";

            // Configure all fields/properties in heap database.
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            HeapDatabaseConfig heapConfig = new HeapDatabaseConfig();
            HeapDatabaseConfigTest.Config(xmlElem, ref heapConfig, true);
            heapConfig.Feedback = new DatabaseFeedbackDelegate(DbFeedback);

            // Open the heap database with above configuration.
            HeapDatabase heapDB = HeapDatabase.Open(
                heapDBFileName, heapConfig);

            // Check the fields/properties in opened heap database.
            Confirm(xmlElem, heapDB, true);

            heapDB.Close();
        }

        private void DbFeedback(DatabaseFeedbackEvent opcode, int percent) {
            if (opcode == DatabaseFeedbackEvent.UPGRADE)
                Console.WriteLine("Update for %d%", percent);

            if (opcode == DatabaseFeedbackEvent.VERIFY)
                Console.WriteLine("Vertify for %d", percent);
        }

        [Test]
        public void TestPutHeap() {
            KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

            testName = "TestPutHeap";
            SetUpTest(true);
            string heapDBFileName = testHome + "/" +
                testName + ".db";

            HeapDatabaseConfig heapConfig =
                new HeapDatabaseConfig();
            heapConfig.Creation = CreatePolicy.ALWAYS;
            using (HeapDatabase heapDB = HeapDatabase.Open(
                heapDBFileName, heapConfig)) {
                DatabaseEntry data = new DatabaseEntry(
                    BitConverter.GetBytes((int)1));
                HeapRecordId rid = heapDB.Append(data);
                DatabaseEntry key = new DatabaseEntry(rid.toArray());
                data = new DatabaseEntry(BitConverter.GetBytes((int)2));
                heapDB.Put(key, data);
                pair = heapDB.GetBoth(key, data);
            }
        }

        [Test]
        public void TestStats() {
            testName = "TestStats";
            SetUpTest(true);
            string dbFileName = testHome + "/" +
                testName + ".db";

            HeapDatabaseConfig dbConfig =
                new HeapDatabaseConfig();
            ConfigCase1(dbConfig);
            HeapDatabase db = HeapDatabase.Open(dbFileName, dbConfig);

            HeapStats stats = db.Stats();
            ConfirmStatsPart1Case1(stats);
            db.PrintFastStats(true);

            // Put 500 records into the database.
            PutRecordCase1(db, null);

            stats = db.Stats();
            ConfirmStatsPart2Case1(stats);
            db.PrintFastStats();

            db.Close();
        }

        [Test]
        public void TestStatsInTxn() {
            testName = "TestStatsInTxn";
            SetUpTest(true);

            StatsInTxn(testHome, testName, false);
        }

        [Test]
        public void TestStatsWithIsolation() {
            testName = "TestStatsWithIsolation";
            SetUpTest(true);

            StatsInTxn(testHome, testName, true);
        }

        [Test]
        public void TestHeapRegionsize() {
            /*
             * Check that with a very small region size, the number of regions
             * grows quickly.
             */
            testName = "TestHeapRegionsize";
            SetUpTest(true);

            Random r = new Random();
            byte[] buf = new byte[1024];
            r.NextBytes(buf);

            HeapDatabaseConfig cfg = new HeapDatabaseConfig();
            cfg.Creation = CreatePolicy.ALWAYS;
            cfg.PageSize = 512;
            cfg.RegionSize = 4;

            HeapDatabase db = HeapDatabase.Open(
                testHome + "/" + testName + ".db", cfg);
            DatabaseEntry dbt = new DatabaseEntry(buf);
            for (int i = 0; i < 10; i++)
                db.Append(dbt);

            Assert.AreEqual(db.RegionSize, 4);

            HeapStats stats = db.Stats();
            db.Close();

            Assert.AreEqual(stats.RegionSize, 4);
            Assert.Greater(stats.nRegions, 1);
        }

        public void StatsInTxn(string home, string name, bool ifIsolation) {
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            EnvConfigCase1(envConfig);
            DatabaseEnvironment env = DatabaseEnvironment.Open(
                home, envConfig);

            Transaction openTxn = env.BeginTransaction();
            HeapDatabaseConfig dbConfig =
                new HeapDatabaseConfig();
            ConfigCase1(dbConfig);
            dbConfig.Env = env;
            HeapDatabase db = HeapDatabase.Open(name + ".db",
                dbConfig, openTxn);
            openTxn.Commit();

            Transaction statsTxn = env.BeginTransaction();
            HeapStats stats;
            if (ifIsolation == false)
                stats = db.Stats(statsTxn);
            else
                stats = db.Stats(statsTxn, Isolation.DEGREE_ONE);

            ConfirmStatsPart1Case1(stats);
            db.PrintStats(true);

            // Put 500 records into the database.
            PutRecordCase1(db, statsTxn);

            if (ifIsolation == false)
                stats = db.Stats(statsTxn);
            else
                stats = db.Stats(statsTxn, Isolation.DEGREE_TWO);
            ConfirmStatsPart2Case1(stats);
            db.PrintStats();

            statsTxn.Commit();
            db.Close();
            env.Close();
        }

        public void EnvConfigCase1(DatabaseEnvironmentConfig cfg) {
            cfg.Create = true;
            cfg.UseTxns = true;
            cfg.UseMPool = true;
            cfg.UseLogging = true;
        }

        public void ConfigCase1(HeapDatabaseConfig dbConfig) {
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.PageSize = 4096;
        }

        public void PutRecordCase1(HeapDatabase db, Transaction txn) {
            byte[] bigArray = new byte[4000];
            HeapRecordId rid = new HeapRecordId(2, 0);
            for (int i = 1; i <= 100; i++) {
                if (txn == null)
                    rid = db.Append(
                        new DatabaseEntry(BitConverter.GetBytes(i)));
                else
                    rid = db.Append(
                        new DatabaseEntry(BitConverter.GetBytes(i)), txn);
            }
            DatabaseEntry key = new DatabaseEntry(rid.toArray());
            for (int i = 100; i <= 500; i++) {
                if (txn == null)
                    db.Put(key, new DatabaseEntry(bigArray));
                else
                    db.Put(key, new DatabaseEntry(bigArray), txn);
            }
        }

        public void ConfirmStatsPart1Case1(HeapStats stats) {
            Assert.AreNotEqual(0, stats.MagicNumber);
            Assert.AreEqual(4096, stats.PageSize);
            Assert.AreNotEqual(0, stats.RegionSize);
            Assert.AreEqual(2, stats.Version);
        }

        public void ConfirmStatsPart2Case1(HeapStats stats) {
            Assert.AreNotEqual(0, stats.nPages);
            Assert.AreEqual(0, stats.MetadataFlags);
            Assert.AreEqual(100, stats.nRecords);
            Assert.AreNotEqual(0, stats.nRegions);
        }

        public static void Confirm(XmlElement xmlElem,
            HeapDatabase heapDB, bool compulsory) {
            DatabaseTest.Confirm(xmlElem, heapDB, compulsory);

            // Confirm heap database specific field/property
            Assert.AreEqual(DatabaseType.HEAP, heapDB.Type);
            string type = heapDB.Type.ToString();
            Assert.IsNotNull(type);
        }
    }
}
