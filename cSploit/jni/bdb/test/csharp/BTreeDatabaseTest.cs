/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
	[TestFixture]
	public class BTreeDatabaseTest : DatabaseTest
	{

		[TestFixtureSetUp]
		public void SetUpTestFixture()
		{
			testFixtureName = "BTreeDatabaseTest";
			base.SetUpTestfixture();
		}

		[Test]
		public void TestBlob()
		{
			testName = "TestBlob";
			SetUpTest(false);
			// Test opening the blob database without environment.
			TestBlobBtreeDatabase(0, null, 6, null, false);

			/*
			 * Test opening the blob database without environment
			 * but specifying blob directory.
			 */
			TestBlobBtreeDatabase(0, null, 6,
			    testHome + "/DBBLOB", true);

			// Test opening the blob database with environment.
			TestBlobBtreeDatabase(3, "ENVBLOB", 6, null, false);

			/*
			 * Test opening the blob database with environment
			 * and specifying blob directory.
			 */
			TestBlobBtreeDatabase(3, null, 6, "/DBBLOB", true);
		}

		/*
		 * Test the blob database with or without environment.
		 * 1. Config and open the environment;
		 * 2. Verify the environment blob configs;
		 * 3. Config and open the database;
		 * 4. Verify the database blob configs;
		 * 5. Insert and verify some blob data by database methods;
		 * 6. Insert some blob data by cursor, update it and verify
		 * the update by database stream and cursor;
		 * 7. Verify the stats;
		 * 8. Close all handles.
		 * If "blobdbt" is true, set the data DatabaseEntry.Blob as
		 * true, otherwise make the data DatabaseEntry reach the blob
		 * threshold in size.
		 */
		void TestBlobBtreeDatabase(uint env_threshold,
		    string env_blobdir, uint db_threshold,
		    string db_blobdir, bool blobdbt)
		{
			if (env_threshold == 0 && db_threshold == 0)
				return;

			string btreeDBName =
			    testHome + "/" + testName + ".db";

			Configuration.ClearDir(testHome);
			BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
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
				DatabaseEnvironment env =
				    DatabaseEnvironment.Open(
				    testHome, envConfig);
				if (env_blobdir == null)
					Assert.IsNull(env.BlobDir);
				else
					Assert.AreEqual(0, env.BlobDir.
					    CompareTo(env_blobdir));
				Assert.AreEqual(env_threshold,
				    env.BlobThreshold);
				cfg.Env = env;
				btreeDBName = testName + ".db";
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

			BTreeDatabase db =
			    BTreeDatabase.Open(btreeDBName, cfg);
			Assert.AreEqual(
			    db_threshold > 0 ? db_threshold : env_threshold,
			    db.BlobThreshold);
			if (db_blobdir == null && cfg.Env == null)
				Assert.IsNull(db.BlobDir);
			else
				Assert.AreEqual(0,
				    db.BlobDir.CompareTo(blrootdir));

			// Insert and verify some blob data by database methods.
			string[] records = {"a", "b", "c", "d", "e", "f", "g",
			    "h", "i", "j", "k", "l", "m", "n", "o", "p", "q",
			    "r", "s", "t", "u", "v", "w", "x", "y", "z"};
			DatabaseEntry kdbt = new DatabaseEntry();
			DatabaseEntry ddbt = new DatabaseEntry();
			byte[] kdata, ddata;
			string str;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			ddbt.Blob = blobdbt;
			Assert.AreEqual(blobdbt, ddbt.Blob);
			for (int i = 0; i < records.Length; i++)
			{
				kdata = BitConverter.GetBytes(i);
				str = records[i];
				if (!blobdbt) {
					for (int j = 0; j < db_threshold; j++)
						str = str + records[i];
				}
				ddata = Encoding.ASCII.GetBytes(str);
				kdbt.Data = kdata;
				ddbt.Data = ddata;
				db.Put(kdbt, ddbt);
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
			 * Insert some blob data by cursor, update it and verify
			 * the update by database stream.
			 */
			kdata = BitConverter.GetBytes(records.Length);
			ddata = Encoding.ASCII.GetBytes("abc");
			kdbt.Data = kdata;
			ddbt.Data = ddata;
			ddbt.Blob = true;
			Assert.IsTrue(ddbt.Blob);
			pair = new KeyValuePair<
			    DatabaseEntry, DatabaseEntry>(kdbt, ddbt);
			CursorConfig dbcConfig = new CursorConfig();
			Transaction txn = null;
			if (cfg.Env != null)
				txn = cfg.Env.BeginTransaction();
			BTreeCursor cursor = db.Cursor(dbcConfig, txn);
			cursor.Add(pair);
			DatabaseStreamConfig dbsc = new DatabaseStreamConfig();
			dbsc.SyncPerWrite = true;
			DatabaseStream dbs = cursor.DbStream(dbsc);
			Assert.AreNotEqual(null, dbs);
			Assert.IsFalse(dbs.GetConfig.ReadOnly);
			Assert.IsTrue(dbs.GetConfig.SyncPerWrite);
			Assert.AreEqual(3, dbs.Size());
			DatabaseEntry sdbt = dbs.Read(0, 3);
			Assert.IsNotNull(sdbt);
			Assert.AreEqual(ddata, sdbt.Data);
			sdbt = new DatabaseEntry(
			    Encoding.ASCII.GetBytes("defg"));
			Assert.IsTrue(dbs.Write(sdbt, 3));
			Assert.AreEqual(7, dbs.Size());
			sdbt = dbs.Read(0, 7);
			Assert.IsNotNull(sdbt);
			Assert.AreEqual(
			    Encoding.ASCII.GetBytes("abcdefg"), sdbt.Data);
			dbs.Close();

			/*
			 * Verify the database stream can not write when it is
			 * configured to be read-only.
			 */
			dbsc.ReadOnly = true;
			dbs = cursor.DbStream(dbsc);
			Assert.IsTrue(dbs.GetConfig.ReadOnly);
			try
			{
				dbs.Write(sdbt, 7);
				throw new TestException();
			}
			catch (DatabaseException)
			{
			}
			dbs.Close();

			// Verify the update by cursor.
			Assert.IsTrue(cursor.Move(kdbt, true));
			pair = cursor.Current;
			Assert.AreEqual(Encoding.ASCII.GetBytes("abcdefg"),
			    pair.Value.Data);
			cursor.Close();
			if (cfg.Env != null)
				txn.Commit();

			/*
			 * Verify the blob files are created
			 * in the expected location.
			 * This part of test code is disabled since
			 * BTreeDatabase.BlobSubDir is not exposed to users.
			 */
			
			//if (cfg.Env != null)
			//	blrootdir = testHome + "/" + blrootdir;
			//string blobdir = blrootdir + "/" + db.BlobSubDir;
			//Assert.AreEqual(records.Length + 1,
			//    Directory.GetFiles(blobdir, "__db.bl*").Length);
			//Assert.AreEqual(1, Directory.GetFiles(
			//    blobdir, "__db_blob_meta.db").Length);

			// Verify the stats.
			BTreeStats st = db.Stats();
			Assert.AreEqual(records.Length + 1, st.nBlobRecords);

			// Close all handles.
			db.Close();
			if (cfg.Env != null)
				cfg.Env.Close();

			/*
			 * Remove the default blob directory when it
			 * is not under the test home.
			 */
			if (db_blobdir == null && cfg.Env == null)
				Directory.Delete("__db_bl", true);
		}

		[Test]
		public void TestCompactWithoutTxn()
		{
			int i, nRecs;
			nRecs = 1000;
			testName = "TestCompactWithoutTxn";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			// The minimum page size
			btreeDBConfig.PageSize = 512;
			btreeDBConfig.BTreeCompare =
			    new EntryComparisonDelegate(dbIntCompare);
			using (BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBConfig))
			{
				DatabaseEntry key;
				DatabaseEntry data;

				// Fill the database with entries from 0 to 999
				for (i = 0; i < nRecs; i++)
				{
					key = new DatabaseEntry(
					    BitConverter.GetBytes(i));
					data = new DatabaseEntry(
ASCIIEncoding.ASCII.GetBytes(Configuration.RandomString(100)));
					btreeDB.Put(key, data);
				}

				/*
				 * Delete entries below 50, between 300 and
				 * 500 and above 700
				 */
				for (i = 0; i < nRecs; i++)
					if (i < (int)(nRecs * 0.05) ||
					    i > (int)(nRecs * 0.7) ||
					    (i < (int)(nRecs * 0.5) &&
					    i > (int)(nRecs * 0.3)))
					{
						key = new DatabaseEntry(
						    BitConverter.GetBytes(i));
						btreeDB.Delete(key);
					}

				btreeDB.Sync();
				long fileSize = new FileInfo(
				   btreeDBFileName).Length;

				// Compact database
				CompactConfig cCfg = new CompactConfig();
				cCfg.FillPercentage = 80;
				cCfg.Pages = 4;
				cCfg.Timeout = 1000;
				cCfg.TruncatePages = true;
				cCfg.start = new DatabaseEntry(
				    BitConverter.GetBytes(1));
				cCfg.stop = new DatabaseEntry(
				    BitConverter.GetBytes(7000));
				CompactData compactData = btreeDB.Compact(cCfg);
				// Verify output statistics fields.
				Assert.AreEqual(0, compactData.Deadlocks);
				Assert.LessOrEqual(0, compactData.EmptyBuckets);
				Assert.LessOrEqual(0, compactData.Levels);
				Assert.Less(0, compactData.PagesExamined);
				Assert.Less(0, compactData.PagesFreed);
				Assert.Less(compactData.PagesFreed,
				    compactData.PagesTruncated);

				btreeDB.Sync();
				long compactedFileSize =
				    new FileInfo(btreeDBFileName).Length;
				Assert.Less(compactedFileSize, fileSize);
			}
		}
		
                [Test]
                public void TestCompression() {
                        testName = "TestCompression";
                        SetUpTest(true);
                        string btreeDBName = testHome + "/" + testName + ".db";

                        BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
                        cfg.Creation = CreatePolicy.ALWAYS;
                        cfg.SetCompression(compress, decompress);
                        BTreeDatabase db = BTreeDatabase.Open(btreeDBName, cfg);
                        DatabaseEntry key, data;
                        char[] keyData = { 'A', 'A', 'A', 'A' };
                        data = new DatabaseEntry(
                                ASCIIEncoding.ASCII.GetBytes("abcdefghij"));
                        int i;
                        for (i = 0; i < 2000; i++) {
                                // Write random data
                                key = new DatabaseEntry(
                                        ASCIIEncoding.ASCII.GetBytes(keyData));
                                db.Put(key, data);

                                // Bump the key. Rollover from Z to A if necessary
                                int j = keyData.Length;
                                do {
                                        j--;
                                        if (keyData[j]++ == 'Z')
                                                keyData[j] = 'A';
                                } while (keyData[j] == 'A');
                       }
                        db.Close();
                }

                bool compress(DatabaseEntry prevKey, DatabaseEntry prevData, 
                    DatabaseEntry key, DatabaseEntry data, ref byte[] dest, out int size) {
                        /* 
                         * Just a dummy function that doesn't do any compression.  It just
                         * writes the 5 byte key and 10 byte data to the buffer.
                         */
                        size = key.Data.Length + data.Data.Length;
                        if (size > dest.Length)
                                return false;
                        key.Data.CopyTo(dest, 0);
                        data.Data.CopyTo(dest, key.Data.Length);
                        return true;
                }

                KeyValuePair<DatabaseEntry, DatabaseEntry> decompress(
                    DatabaseEntry prevKey, DatabaseEntry prevData, byte[] compressed, out uint bytesRead) {
                        byte[] keyData = new byte[4];
                        byte[] dataData = new byte[10];
                        Array.ConstrainedCopy(compressed, 0, keyData, 0, 4);
                        Array.ConstrainedCopy(compressed, 4, dataData, 0, 10);
                        DatabaseEntry key = new DatabaseEntry(keyData);
                        DatabaseEntry data = new DatabaseEntry(dataData);
                        bytesRead = (uint)(key.Data.Length + data.Data.Length);
                        return new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
                }

                /*
                 * Test the default compression - which is prefix compression
                 * of keys. This should work well with ordered keys and short
                 * data items.
                 */
                [Test]
                public void TestCompressionDefault() {
                        testName = "TestCompressionDefault";
                        SetUpTest(true);
                        string btreeDBName = testHome + "/" + testName + ".db";

                        BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
                        cfg.Creation = CreatePolicy.ALWAYS;
                        BTreeDatabase db = BTreeDatabase.Open(btreeDBName, cfg);
                        DatabaseEntry key, data;
                        char[] keyData = { 'A', 'A', 'A', 'A' };
                        data = new DatabaseEntry(
                                ASCIIEncoding.ASCII.GetBytes("A"));
                        int i;
                        for (i = 0; i < 5000; i++) {
                                // Write random data
                                key = new DatabaseEntry(
                                        ASCIIEncoding.ASCII.GetBytes(keyData));
                                db.Put(key, data);

                                // Bump the key.
                                int j = keyData.Length;
                                do {
                                        j--;
                                        if (keyData[j]++ == 'Z')
                                                keyData[j] = 'A';
                                } while (keyData[j] == 'A');
                        }
                        db.Close();

                        FileInfo dbInfo = new FileInfo(btreeDBName);
                        long uncompressedSize = dbInfo.Length;
                        Configuration.ClearDir(testHome);

                        cfg = new BTreeDatabaseConfig();
                        cfg.Creation = CreatePolicy.ALWAYS;
                        cfg.SetCompression();
                        db = BTreeDatabase.Open(btreeDBName, cfg);
                        keyData = new char[]{ 'A', 'A', 'A', 'A' };
                        data = new DatabaseEntry(
                                ASCIIEncoding.ASCII.GetBytes("A"));
                        for (i = 0; i < 5000; i++) {
                                // Write random data
                                key = new DatabaseEntry(
                                        ASCIIEncoding.ASCII.GetBytes(keyData));
                                db.Put(key, data);

                                // Bump the key.
                                int j = keyData.Length;
                                do {
                                        j--;
                                        if (keyData[j]++ == 'Z')
                                                keyData[j] = 'A';
                                } while (keyData[j] == 'A');
                        }
                        Cursor dbc = db.Cursor();
                        foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> kvp
                                in dbc) 
                                i--;
                        dbc.Close();
                        Assert.AreEqual(i, 0);
                        db.Close();

                        dbInfo = new FileInfo(btreeDBName);
                        Assert.Less(dbInfo.Length, uncompressedSize);
                        Console.WriteLine(
                                "Uncompressed: {0}", uncompressedSize);
                        Console.WriteLine("Compressed: {0}", dbInfo.Length);

                        Configuration.ClearDir(testHome);

                        cfg = new BTreeDatabaseConfig();
                        cfg.Creation = CreatePolicy.ALWAYS;
                        cfg.SetCompression();
                        db = BTreeDatabase.Open(btreeDBName, cfg);
                        for (i = 1023; i < 1124; i++){
                                key = new DatabaseEntry(BitConverter.GetBytes(i));
                                data = new DatabaseEntry(BitConverter.GetBytes(i + 3));
                                db.Put(key, data);
                        }
                        dbc = db.Cursor();
                        foreach (KeyValuePair<DatabaseEntry, DatabaseEntry> kvp in dbc){ 
                                int keyInt = BitConverter.ToInt32(kvp.Key.Data, 0);
                                int dataInt = BitConverter.ToInt32(kvp.Value.Data, 0);
                                Assert.AreEqual(3, dataInt - keyInt);
                        }
                        dbc.Close();

                        db.Close();    
                }

		[Test, ExpectedException(typeof(AccessViolationException))]
		public void TestClose()
		{
			testName = "TestClose";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBConfig);
			btreeDB.Close();
			DatabaseEntry key = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("hi"));
			DatabaseEntry data = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("hi"));
			btreeDB.Put(key, data);
		}

		[Test]
		public void TestCloseWithoutSync()
		{
			testName = "TestCloseWithoutSync";
			SetUpTest(true);
			string btreeDBName = testName + ".db";

			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.ForceFlush = true;
			envConfig.UseTxns = true;
			envConfig.UseMPool = true;
			envConfig.UseLogging = true;
			envConfig.LogSystemCfg = new LogConfig();
			envConfig.LogSystemCfg.ForceSync = false;
			envConfig.LogSystemCfg.AutoRemove = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);

			TransactionConfig txnConfig = new TransactionConfig();
			txnConfig.SyncAction =
			    TransactionConfig.LogFlush.WRITE_NOSYNC;
			Transaction txn = env.BeginTransaction(txnConfig);

			BTreeDatabaseConfig btreeConfig =
			    new BTreeDatabaseConfig();
			btreeConfig.Creation = CreatePolicy.ALWAYS;
			btreeConfig.Env = env;

			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBName, btreeConfig, txn);

			DatabaseEntry key = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("key"));
			DatabaseEntry data = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("data"));
			Assert.IsFalse(btreeDB.Exists(key, txn));
			btreeDB.Put(key, data, txn);
			btreeDB.Close(false);
			txn.Commit();
			env.Close();

			BTreeDatabaseConfig dbConfig =
			    new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.NEVER;
			using (BTreeDatabase db = BTreeDatabase.Open(
			    testHome + "/" + btreeDBName, dbConfig))
			{
				Assert.IsFalse(db.Exists(key));
			}
		}

		[Test]
		public void TestCursorWithoutEnv()
		{
			BTreeCursor cursor;
			BTreeDatabase db;
			string dbFileName;

			testName = "TestCursorWithoutEnv";
			SetUpTest(true);
			dbFileName = testHome + "/" + testName + ".db";

			// Open btree database.
			OpenBtreeDB(null, null, dbFileName, out db);

			// Get a cursor.
			cursor = db.Cursor();

			/*
			 * Add a record to the database with cursor and 
			 * confirm that the record exists in the database.
			 */
			CursorTest.AddOneByCursor(db, cursor);

			// Close cursor and database.
			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestCursorWithConfigInTxn()
		{
			BTreeCursor cursor;
			BTreeDatabase db;
			DatabaseEnvironment env;
			Transaction txn;
			string dbFileName;

			testName = "TestCursorWithConfigInTxn";
			SetUpTest(true);
			dbFileName = testName + ".db";

			// Open environment and begin a transaction.

			SetUpEnvAndTxn(testHome, out env, out txn);
			OpenBtreeDB(env, txn, dbFileName, out db);

			// Config and get a cursor.
			cursor = db.Cursor(new CursorConfig(), txn);

			/*
			 * Add a record to the database with cursor and 
			 * confirm that the record exists in the database.
			 */
			CursorTest.AddOneByCursor(db, cursor);

			/*
			 * Close cursor, database, commit the transaction 
			 * and close the environment.
			 */
			cursor.Close();
			db.Close();
			txn.Commit();
			env.Close();
		}

		[Test]
		public void TestCursorWithoutConfigInTxn()
		{
			BTreeCursor cursor;
			BTreeDatabase db;
			DatabaseEnvironment env;
			Transaction txn;
			string dbFileName;

			testName = "TestCursorWithoutConfigInTxn";
			SetUpTest(true);
			dbFileName = testName + ".db";

			// Open environment and begin a transaction.
			SetUpEnvAndTxn(testHome, out env, out txn);
			OpenBtreeDB(env, txn, dbFileName, out db);

			// Get a cursor in the transaction.
			cursor = db.Cursor(txn);

			/*
			 * Add a record to the database with cursor and 
			 * confirm that the record exists in the database.
			 */
			CursorTest.AddOneByCursor(db, cursor);

			/*
			 * Close cursor, database, commit the transaction 
			 * and close the environment.
			 */
			cursor.Close();
			db.Close();
			txn.Commit();
			env.Close();
		}

		[Test, ExpectedException(typeof(ExpectedTestException))]
		public void TestDelete()
		{
			testName = "TestDelete";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBConfig);
			DatabaseEntry key = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("key"));
			DatabaseEntry data = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("data"));
			btreeDB.Put(key, data);
			btreeDB.Delete(key);
			try
			{
				btreeDB.Get(key);
			}
			catch (NotFoundException)
			{
				throw new ExpectedTestException();
			}
			finally
			{
				btreeDB.Close();
			}
		}

		[Test]
		public void TestDupCompare()
		{
			testName = "TestDupCompare";
			SetUpTest(true);

			BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
			cfg.Creation = CreatePolicy.IF_NEEDED;
			cfg.Duplicates = DuplicatesPolicy.UNSORTED;

			// Do not set the duplicate comparison delegate.
			using (BTreeDatabase db = BTreeDatabase.Open(
			    testHome + "/" + testName + ".db", cfg)) {
				try {
					int ret = db.DupCompare(
					    new DatabaseEntry(
					    BitConverter.GetBytes(255)),
					    new DatabaseEntry(
					    BitConverter.GetBytes(257)));
					throw new TestException();
				} catch(NullReferenceException) {
				}
			}

			/*
			 * Set the duplicate comparison delegate, and verify
			 * the comparison delegate.
			 */ 
			cfg.DuplicateCompare =
			    new EntryComparisonDelegate(dbIntCompare);
			using (BTreeDatabase db = BTreeDatabase.Open(
			    testHome + "/" + testName + "1.db", cfg)) {
				int ret = db.DupCompare(
				    new DatabaseEntry(BitConverter.GetBytes(255)),
				    new DatabaseEntry(BitConverter.GetBytes(257)));
				Assert.Greater(0, ret);
			}
		}

		[Test]
		public void TestEncryption() {
			testName = "TestEncryption";
			SetUpTest(true);

			BTreeDatabase db1;

			BTreeDatabaseConfig dbCfg =
			    new BTreeDatabaseConfig();
			dbCfg.Creation = CreatePolicy.IF_NEEDED;

			// Open an encrypted database.
			dbCfg.SetEncryption("bdb", EncryptionAlgorithm.AES);
			using (db1 = BTreeDatabase.Open(
			    testHome + "/" + testName + ".db", dbCfg)) {
				Assert.IsTrue(db1.Encrypted);
				for (int i = 0; i < 10; i++)
					db1.Put(new DatabaseEntry
					    (BitConverter.GetBytes(i)),
					    new DatabaseEntry(
					    BitConverter.GetBytes(i)));
			}

			// Verify the database is encrypted.
			BTreeDatabaseConfig verifyDbCfg =
			    new BTreeDatabaseConfig();
			verifyDbCfg.Creation = CreatePolicy.IF_NEEDED;
			verifyDbCfg.SetEncryption(
			    dbCfg.EncryptionPassword,
			    dbCfg.EncryptAlgorithm);
			verifyDbCfg.Encrypted = true;
			using (db1 = BTreeDatabase.Open(
			    testHome + "/" + testName + ".db",
			    verifyDbCfg)) {
				for (int i = 0; i < 10; i++)
					db1.Get(new DatabaseEntry(
					    BitConverter.GetBytes(i)));
			};
		}

		[Test]
		public void TestExist()
		{
			testName = "TestExist";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
			BTreeDatabase btreeDB;
			using (btreeDB = BTreeDatabase.Open(
			    dbFileName, btreeDBConfig))
			{
				DatabaseEntry key = new DatabaseEntry(
				     ASCIIEncoding.ASCII.GetBytes("key"));
				DatabaseEntry data = new DatabaseEntry(
				     ASCIIEncoding.ASCII.GetBytes("data"));

				btreeDB.Put(key, data);
				Assert.IsTrue(btreeDB.Exists(
				     new DatabaseEntry(
				     ASCIIEncoding.ASCII.GetBytes("key"))));
				Assert.IsFalse(btreeDB.Exists(
				     new DatabaseEntry(
				     ASCIIEncoding.ASCII.GetBytes("data"))));
			}
		}

		[Test]
		public void TestExistWithTxn()
		{
			BTreeDatabase btreeDB;
			Transaction txn;
			DatabaseEnvironmentConfig envConfig;
			DatabaseEnvironment env;
			DatabaseEntry key, data;

			testName = "TestExistWithTxn";
			SetUpTest(true);

			// Open an environment.
			envConfig = new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseMPool = true;
			envConfig.UseTxns = true;
			env = DatabaseEnvironment.Open(testHome, envConfig);

			// Begin a transaction.
			txn = env.BeginTransaction();

			// Open a database.
			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
			btreeDBConfig.Env = env;
			btreeDB = BTreeDatabase.Open(testName + ".db",
			    btreeDBConfig, txn);

			// Put key data pair into database.
			key = new DatabaseEntry(
				     ASCIIEncoding.ASCII.GetBytes("key"));
			data = new DatabaseEntry(
			     ASCIIEncoding.ASCII.GetBytes("data"));
			btreeDB.Put(key, data, txn);

			// Confirm that the pair exists in the database.
			Assert.IsTrue(btreeDB.Exists(new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("key")), txn));
			Assert.IsFalse(btreeDB.Exists(new DatabaseEntry(
			     ASCIIEncoding.ASCII.GetBytes("data")), txn));

			// Dispose all.
			btreeDB.Close();
			txn.Commit();
			env.Close();
		}

		[Test]
		public void TestExistWithLockingInfo()
		{
			BTreeDatabase btreeDB;
			DatabaseEnvironment env;
			DatabaseEntry key, data;

			testName = "TestExistWithLockingInfo";
			SetUpTest(true);

			// Open the environment.
			DatabaseEnvironmentConfig envCfg =
			    new DatabaseEnvironmentConfig();
			envCfg.Create = true;
			envCfg.FreeThreaded = true;
			envCfg.UseLocking = true;
			envCfg.UseLogging = true;
			envCfg.UseMPool = true;
			envCfg.UseTxns = true;
			env = DatabaseEnvironment.Open(
			    testHome, envCfg);

			// Open database in transaction.
			Transaction openTxn = env.BeginTransaction();
			BTreeDatabaseConfig cfg =
			    new BTreeDatabaseConfig();
			cfg.Creation = CreatePolicy.ALWAYS;
			cfg.Env = env;
			cfg.FreeThreaded = true;
			cfg.PageSize = 4096;
			cfg.Duplicates = DuplicatesPolicy.UNSORTED;
			btreeDB = BTreeDatabase.Open(testName + ".db",
			    cfg, openTxn);
			openTxn.Commit();

			// Put key data pair into database.
			Transaction txn = env.BeginTransaction();
			key = new DatabaseEntry(
				     ASCIIEncoding.ASCII.GetBytes("key"));
			data = new DatabaseEntry(
			     ASCIIEncoding.ASCII.GetBytes("data"));
			btreeDB.Put(key, data, txn);

			// Confirm that the pair exists in the database with LockingInfo.
			LockingInfo lockingInfo = new LockingInfo();
			lockingInfo.ReadModifyWrite = true;

			// Confirm that the pair exists in the database.
			Assert.IsTrue(btreeDB.Exists(new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("key")), txn, lockingInfo));
			Assert.IsFalse(btreeDB.Exists(new DatabaseEntry(
			     ASCIIEncoding.ASCII.GetBytes("data")), txn, lockingInfo));
			txn.Commit();

			btreeDB.Close();
			env.Close();
		}

		[Test]
		public void TestGetByKey()
		{
			testName = "TestGetByKey";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
				     testName + ".db";
			string btreeDBName =
			    Path.GetFileNameWithoutExtension(btreeDBFileName);

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBName, btreeDBConfig);

			DatabaseEntry key = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("key"));
			DatabaseEntry data = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("data"));
			btreeDB.Put(key, data);

			KeyValuePair<DatabaseEntry, DatabaseEntry> pair =
			    new KeyValuePair<DatabaseEntry, DatabaseEntry>();
			pair = btreeDB.Get(key);
			Assert.AreEqual(pair.Key.Data, key.Data);
			Assert.AreEqual(pair.Value.Data, data.Data);
			btreeDB.Close();
		}

		[Test]
		public void TestGetByRecno()
		{
			testName = "TestGetByRecno";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string btreeDBName =
			    Path.GetFileNameWithoutExtension(btreeDBFileName);

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			btreeDBConfig.UseRecordNumbers = true;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBName, btreeDBConfig);
			Assert.IsTrue(btreeDB.RecordNumbers);

			DatabaseEntry key = new DatabaseEntry();
			DatabaseEntry data = new DatabaseEntry();
			uint recno, count, value;
			for (recno = 1; recno <= 100; recno++)
			{
				value = 200 - recno;
				Configuration.dbtFromString(key,
				    Convert.ToString(value));
				Configuration.dbtFromString(data,
				    Convert.ToString(value));
				btreeDB.Put(key, data);
			}

			KeyValuePair<DatabaseEntry, DatabaseEntry> pair =
			    new KeyValuePair<DatabaseEntry, DatabaseEntry>();

			for (count = 1; ; count++)
			{
				try
				{
					pair = btreeDB.Get(count);
				}
				catch (NotFoundException)
				{
					Assert.AreEqual(101, count);
					break;
				}
				value = 299 - 200 + count;
				Assert.AreEqual(value.ToString(),
				    Configuration.strFromDBT(pair.Key));
			}

			btreeDB.Close();
		}

		[Test, ExpectedException(typeof(NotFoundException))]
		public void TestGetBoth()
		{
			testName = "TestGetBoth";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string btreeDBName =
			    Path.GetFileNameWithoutExtension(btreeDBFileName);

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			using (BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBName, btreeDBConfig))
			{
				DatabaseEntry key = new DatabaseEntry();
				DatabaseEntry data = new DatabaseEntry();

				Configuration.dbtFromString(key, "key");
				Configuration.dbtFromString(data, "data");
				btreeDB.Put(key, data);
				KeyValuePair<DatabaseEntry, DatabaseEntry> pair =
				    new KeyValuePair<DatabaseEntry, DatabaseEntry>();
				pair = btreeDB.GetBoth(key, data);
				Assert.AreEqual(key.Data, pair.Key.Data);
				Assert.AreEqual(data.Data, pair.Value.Data);

				Configuration.dbtFromString(key, "key");
				Configuration.dbtFromString(data, "key");
				btreeDB.GetBoth(key, data);
			}
		}

		[Test]
		public void TestGetBothMultiple()
		{
			testName = "TestGetBothMultiple";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string btreeDBName = testName;
			DatabaseEntry key, data;
			KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> kvp;
			int cnt;

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			btreeDBConfig.Duplicates = DuplicatesPolicy.UNSORTED;
			btreeDBConfig.PageSize = 1024;
			using (BTreeDatabase btreeDB = GetMultipleDB(
			    btreeDBFileName, btreeDBName, btreeDBConfig)) {
				key = new DatabaseEntry(BitConverter.GetBytes(100));
				data = new DatabaseEntry(BitConverter.GetBytes(100));

				kvp = btreeDB.GetBothMultiple(key, data);
				cnt = 0;
				foreach (DatabaseEntry dbt in kvp.Value)
					cnt++;
				Assert.AreEqual(cnt, 10);

				kvp = btreeDB.GetBothMultiple(key, data, 1024);
				cnt = 0;
				foreach (DatabaseEntry dbt in kvp.Value)
					cnt++;
				Assert.AreEqual(cnt, 10);
			}
		}
		
		[Test]
		public void TestGetMultiple() 
		{
			testName = "TestGetMultiple";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string btreeDBName = testName;

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			btreeDBConfig.Duplicates = DuplicatesPolicy.UNSORTED;
			btreeDBConfig.PageSize = 512;
			
			using (BTreeDatabase btreeDB = GetMultipleDB(
			    btreeDBFileName, btreeDBName, btreeDBConfig)) {
				DatabaseEntry key = new DatabaseEntry(
                                    BitConverter.GetBytes(10));
				KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> kvp = 
                                    btreeDB.GetMultiple(key, 1024);
				int cnt = 0;
				foreach (DatabaseEntry dbt in kvp.Value)
					cnt++;
				Assert.AreEqual(cnt, 10);

				key = new DatabaseEntry(
                                    BitConverter.GetBytes(102));
				kvp = btreeDB.GetMultiple(key, 1024);
				cnt = 0;
				foreach (DatabaseEntry dbt in kvp.Value)
					cnt++;
				Assert.AreEqual(cnt, 1);
			}
		}

		[Test]
		public void TestGetMultipleByRecno()
		{
			testName = "TestGetMultipleByRecno";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string btreeDBName =
			    Path.GetFileNameWithoutExtension(btreeDBFileName);

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			btreeDBConfig.Duplicates = DuplicatesPolicy.NONE;
			btreeDBConfig.UseRecordNumbers = true;
			using (BTreeDatabase btreeDB = GetMultipleDB(
			    btreeDBFileName, btreeDBName, btreeDBConfig)) {
				int recno = 44;
				KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> kvp = 
                                    btreeDB.GetMultiple((uint)recno);
				int cnt = 0;
				int kdata = BitConverter.ToInt32(kvp.Key.Data, 0);
				Assert.AreEqual(kdata, recno);
				foreach (DatabaseEntry dbt in kvp.Value) {
					cnt++;
					int ddata = BitConverter.ToInt32(dbt.Data, 0);
					Assert.AreEqual(ddata, recno);
				}
				Assert.AreEqual(cnt, 1);
			}
		}

		[Test]
		public void TestGetMultipleByRecnoInSize()
		{
			testName = "TestGetMultipleByRecnoInSize";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string btreeDBName =
			    Path.GetFileNameWithoutExtension(btreeDBFileName);

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			btreeDBConfig.Duplicates = DuplicatesPolicy.NONE;
			btreeDBConfig.UseRecordNumbers = true;
			btreeDBConfig.PageSize = 512;
			using (BTreeDatabase btreeDB = GetMultipleDB(
			    btreeDBFileName, btreeDBName, btreeDBConfig)) {
				int recno = 100;
				int bufferSize = 1024;
				KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> kvp = 
                                    btreeDB.GetMultiple((uint)recno, bufferSize);
				int cnt = 0;
				int kdata = BitConverter.ToInt32(kvp.Key.Data, 0);
				Assert.AreEqual(kdata, recno);
				foreach (DatabaseEntry dbt in kvp.Value) {
					cnt++;
					Assert.AreEqual(dbt.Data.Length, 111);
				}
				Assert.AreEqual(1, cnt);
			}
		}

		[Test]
		public void TestGetMultipleInSize()
		{
			testName = "TestGetMultipleInSize";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string btreeDBName =
			    Path.GetFileNameWithoutExtension(btreeDBFileName);

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			btreeDBConfig.Duplicates = DuplicatesPolicy.UNSORTED;
			btreeDBConfig.PageSize = 1024;
			using (BTreeDatabase btreeDB = GetMultipleDB(
			    btreeDBFileName, btreeDBName, btreeDBConfig)) {

				int num = 101;
				DatabaseEntry key = new DatabaseEntry(
                                    BitConverter.GetBytes(num));
				int bufferSize = 10240;
				KeyValuePair<DatabaseEntry, MultipleDatabaseEntry> kvp = 
                                    btreeDB.GetMultiple(key, bufferSize);
				int cnt = 0;
				foreach (DatabaseEntry dbt in kvp.Value) {
					cnt++;
					Assert.AreEqual(BitConverter.ToInt32(
                                            dbt.Data, 0), num);
					num++;
				}
				Assert.AreEqual(cnt, 923);
			}
		}
        
		[Test]
		public void TestGetWithTxn()
		{
			testName = "TestGetWithTxn";
			SetUpTest(true);

			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseLogging = true;
			envConfig.UseMPool = true;
			envConfig.UseTxns = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);

			try
			{
				Transaction openTxn = env.BeginTransaction();
				BTreeDatabaseConfig dbConfig =
				    new BTreeDatabaseConfig();
				dbConfig.Env = env;
				dbConfig.Creation = CreatePolicy.IF_NEEDED;
				BTreeDatabase db = BTreeDatabase.Open(
				    testName + ".db", dbConfig, openTxn);
				openTxn.Commit();

				Transaction putTxn = env.BeginTransaction();
				try
				{
					for (int i = 0; i < 20; i++)
						db.Put(new DatabaseEntry(
						    BitConverter.GetBytes(i)),
						    new DatabaseEntry(
						    BitConverter.GetBytes(i)), putTxn);
					putTxn.Commit();
				}
				catch (DatabaseException e)
				{
					putTxn.Abort();
					db.Close();
					throw e;
				}

				Transaction getTxn = env.BeginTransaction();
				KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
				try
				{
					for (int i = 0; i < 20; i++)
					{
						pair = db.Get(new DatabaseEntry(
						    BitConverter.GetBytes(i)), getTxn);
						Assert.AreEqual(BitConverter.GetBytes(i),
						    pair.Key.Data);
					}

					getTxn.Commit();
					db.Close();
				}
				catch (DatabaseException)
				{
					getTxn.Abort();
					db.Close();
					throw new TestException();
				}
			}
			catch (DatabaseException)
			{
			}
			finally
			{
				env.Close();
			}
		}

		[Test]
		public void TestKeyRange()
		{
			testName = "TestKeyRange";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string btreeDBName = Path.GetFileNameWithoutExtension(
			    btreeDBFileName);

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBName, btreeDBConfig);

			DatabaseEntry key = new DatabaseEntry();
			DatabaseEntry data = new DatabaseEntry();
			uint recno;
			for (recno = 1; recno <= 10; recno++)
			{
				Configuration.dbtFromString(key,
				    Convert.ToString(recno));
				Configuration.dbtFromString(data,
				    Convert.ToString(recno));
				btreeDB.Put(key, data);
			}

			Configuration.dbtFromString(key, Convert.ToString(5));
			KeyRange keyRange = btreeDB.KeyRange(key);
			Assert.AreEqual(0.5, keyRange.Less);
			Assert.AreEqual(0.1, keyRange.Equal);
			Assert.AreEqual(0.4, keyRange.Greater);

			btreeDB.Close();
		}

		[Test]
		public void TestNoWaitDbExclusiveLock()
		{
			testName = "TestNoWaitDbExclusiveLock";
			SetUpTest(true);

			// Open an environment.
			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.AutoCommit = true;
			envConfig.Create = true;
			envConfig.UseMPool = true;
			envConfig.UseLocking = true;
			envConfig.UseLogging = true;
			envConfig.UseTxns = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);

			// Open a database.
			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Env = env;
 			string btreeDBFileName = testName + ".db";
			BTreeDatabase btreeDB;

 			btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
			btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBConfig);
			Assert.IsNull(btreeDB.NoWaitDbExclusiveLock);
			btreeDB.Close();

			btreeDBConfig.NoWaitDbExclusiveLock = false;
			btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBConfig);
			Assert.AreEqual(btreeDB.NoWaitDbExclusiveLock, false);
			btreeDB.Close();

			btreeDBConfig.NoWaitDbExclusiveLock = true;
			btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBConfig);
			Assert.AreEqual(btreeDB.NoWaitDbExclusiveLock, true);
			btreeDB.Close();
			env.Close();
		}

		[Test]
		public void TestOpenExistingBtreeDB()
		{
			testName = "TestOpenExistingBtreeDB";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			     testName + ".db";

			BTreeDatabaseConfig btreeConfig =
			    new BTreeDatabaseConfig();
			btreeConfig.Creation = CreatePolicy.ALWAYS;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeConfig);
			btreeDB.Close();

			DatabaseConfig dbConfig = new DatabaseConfig();
			Database db = Database.Open(btreeDBFileName, 
			    dbConfig);
			Assert.AreEqual(db.Type, DatabaseType.BTREE);
			Assert.AreEqual(db.Creation, CreatePolicy.NEVER);
			db.Close();
		}

		[Test]
		public void TestOpenNewBtreeDB()
		{
			testName = "TestOpenNewBtreeDB";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";

			XmlElement xmlElem = Configuration.TestSetUp(
			    testFixtureName, testName);
			BTreeDatabaseConfig btreeConfig =
			    new BTreeDatabaseConfig();
			BTreeDatabaseConfigTest.Config(xmlElem,
			    ref btreeConfig, true);
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeConfig);
			Confirm(xmlElem, btreeDB, true);
			btreeDB.Close();
		}

		[Test]
		public void TestOpenMulDBInSingleFile()
		{
			testName = "TestOpenMulDBInSingleFile";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string[] btreeDBArr = new string[4];

			for (int i = 0; i < 4; i++)
				btreeDBArr[i] = Path.GetFileNameWithoutExtension(
				    btreeDBFileName) + i;

			Configuration.ClearDir(testHome);

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;

			BTreeDatabase btreeDB;
			for (int i = 0; i < 4; i++)
			{
				btreeDB = BTreeDatabase.Open(btreeDBFileName,
				    btreeDBArr[i], btreeDBConfig);
				Assert.AreEqual(CreatePolicy.IF_NEEDED, btreeDB.Creation);
				btreeDB.Close();
			}

			DatabaseConfig dbConfig = new DatabaseConfig();
			Database db;
			for (int i = 0; i < 4; i++)
			{
				using (db = Database.Open(btreeDBFileName,
				    btreeDBArr[i], dbConfig))
				{
					Assert.AreEqual(btreeDBArr[i],
					    db.DatabaseName);
					Assert.AreEqual(DatabaseType.BTREE,
					    db.Type);
				}
			}
		}

		[Test]
		public void TestOpenWithTxn()
		{
			testName = "TestOpenWithTxn";
			SetUpTest(true);
			string btreeDBName = testName + ".db";

			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseTxns = true;
			envConfig.UseMPool = true;

			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);
			Transaction txn = env.BeginTransaction(
			    new TransactionConfig());

			BTreeDatabaseConfig btreeConfig =
			    new BTreeDatabaseConfig();
			btreeConfig.Creation = CreatePolicy.ALWAYS;
			btreeConfig.Env = env;

			/*
			 * If environmnet home is set, the file name in Open()
			 * is the relative path.
			 */
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBName, btreeConfig, txn);
			Assert.IsTrue(btreeDB.Transactional);
			btreeDB.Close();
			txn.Commit();
			env.Close();
		}

		[Test]
		public void TestPartialGet() 
		{
			testName = "TestPartialGet";
			SetUpTest(true);

			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseTxns = true;
			envConfig.UseMPool = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);

			BTreeDatabase db;
			PopulateDb(env, out db);

			// Partially get the first byte and verify it is 'h'.
			DatabaseEntry key, data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			Transaction txn = env.BeginTransaction();
			for (int i = 0; i < 100; i++) {
				key = new DatabaseEntry(BitConverter.GetBytes(i));
				data = new DatabaseEntry(0, 1);
				pair = db.Get(key, data, txn);
				Assert.AreEqual(1, pair.Value.Data.Length);
				Assert.AreEqual((byte)'h', pair.Value.Data[0]);
			}
			txn.Commit();
			db.Close();
			env.Close();

			/*
			 * Open the existing database and partially get
			 * non-existing data.
			 */
			using (Database edb = Database.Open(
			    testHome + "/" + testName + ".db", 
			    new DatabaseConfig())) {
				key = new DatabaseEntry(BitConverter.GetBytes(0));
				data = new DatabaseEntry(10, 1);
				pair = edb.Get(key, data);
				Assert.AreEqual(null, pair.Value.Data);
			}
		}

		[Test]
		public void TestPartialPut() 
		{
			testName = "TestPartialPut";
			SetUpTest(true);

			BTreeDatabase db;
			PopulateDb(null, out db);

			// Partially update the records.
			DatabaseEntry key, data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			byte[] partialBytes = ASCIIEncoding.ASCII.GetBytes("aa");
			byte[] valueBytes;
			for (int i = 0; i < 100; i++) {
				key = new DatabaseEntry(BitConverter.GetBytes(i));
				if (i < 50)
					data = new DatabaseEntry(
					    partialBytes, 0, 2);
				else {
					data = new DatabaseEntry(partialBytes);
					data.Partial = true;
					data.PartialLen = 2;
					data.PartialOffset = 0;
				}
				Assert.AreEqual(0, data.PartialOffset);
				Assert.AreEqual(2, data.PartialLen);
				Assert.AreEqual(true, data.Partial);
				db.Put(key, data);
			}
			// Verify that the records have been partially changed.
			valueBytes = ASCIIEncoding.ASCII.GetBytes("hello");
			valueBytes[0] = (byte)'a';
			valueBytes[1] = (byte)'a';
			for (int i = 0; i < 100; i++) {
				key = new DatabaseEntry(BitConverter.GetBytes(i));
				pair = db.Get(key);
				Assert.AreEqual(
				    BitConverter.GetBytes(i), pair.Key.Data);
				Assert.AreEqual(valueBytes, pair.Value.Data);
			}

			// Partial put from non-existing offset.
			key = new DatabaseEntry(BitConverter.GetBytes(0));
			data = new DatabaseEntry(partialBytes, 100, 2);
			db.Put(key, data);

			// Verify that the records have been partially changed.
			pair = db.Get(key);
			Assert.AreEqual(102, pair.Value.Data.Length);
			Assert.AreEqual(valueBytes[0],
			    pair.Value.Data[100]);
			Assert.AreEqual(valueBytes[0], 
			    pair.Value.Data[101]);

			/*
			 * Reuse the DatabaseEntry for non-partial. The partial
			 * length and offset should be ignored.
			 */ 
			key = new DatabaseEntry(BitConverter.GetBytes(0));
			data.Partial = false;
			db.Put(key, data);
			pair = db.Get(key);
			Assert.AreEqual(2, pair.Value.Data.Length);

			db.Close();
		}

		private void PopulateDb(DatabaseEnvironment env,
		    out BTreeDatabase db) 
		{
			DatabaseEntry key, data;
			Transaction txn = null;
			string dbName;

			if (env != null) {
				txn = env.BeginTransaction();
				dbName = testName + ".db";
			} else
				dbName = testHome + "/" + testName + ".db";

			OpenBtreeDB(env, txn, dbName, out db);

			for (int i = 0; i < 100; i++) {
				key = new DatabaseEntry(
				    BitConverter.GetBytes(i));
				data = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes("hello"));
				db.Put(key, data, txn);
			}

			if (txn != null)
				txn.Commit();
		}

		[Test]
		public void TestPartition()
		{
			testName = "TestPartition";
			SetUpTest(true);
			string btreeDBName = testHome + "/" + testName + ".db";

 			BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
			BTreeDatabase db;
			DatabaseEntry[] keys;
 			DatabaseEntry key, data;
			string[] keyData =
			    { "a", "b", "i", "k", "l", "q", "v", "z" };
			int i;
			uint parts;

			cfg.Creation = CreatePolicy.ALWAYS;
			parts = 3;
			keys = new DatabaseEntry[parts - 1];
			keys[0] = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("i"));
			keys[1] = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("q"));

			/*
			 * Test that neither key array nor
			 * partiton callback is set.
			 */
			Assert.AreEqual(false, cfg.SetPartitionByKeys(null));
			Assert.AreEqual(false,
			    cfg.SetPartitionByCallback(parts, null));

			/* Test creating the partitioned database by keys. */
			Assert.AreEqual(true, cfg.SetPartitionByKeys(keys));
			db = BTreeDatabase.Open(btreeDBName, cfg);
			for (i = 0; i < keyData.Length; i++)
			{
				key = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes(keyData[i]));
				data = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes(keyData[i]));
				db.Put(key, data);
			}
			Assert.AreEqual(parts, db.NParts);
			Assert.AreEqual(parts - 1, db.PartitionKeys.Length);
			Assert.AreEqual(
			    keys[0].Data, db.PartitionKeys[0].Data);
			Assert.AreEqual(
			    keys[1].Data, db.PartitionKeys[1].Data);
			Assert.AreEqual(db.Partition, null);
			db.Close();
			string[] files =
			    Directory.GetFiles(testHome, "__dbp.*");
			Assert.AreEqual(parts, files.Length);

			/*
			 * Test creating the partitioned database by callback.
			 */
			Directory.Delete(testHome, true);
			Directory.CreateDirectory(testHome);
			Assert.AreEqual(true,
			    cfg.SetPartitionByCallback(parts, partition));
			db = BTreeDatabase.Open(btreeDBName, cfg);
			for (i = 0; i < keyData.Length; i++)
			{
				key = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes(keyData[i]));
				data = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes(keyData[i]));
				db.Put(key, data);
			}
			Assert.AreEqual(parts, db.NParts);
			Assert.AreEqual(
			    new PartitionDelegate(partition), db.Partition);
			db.Close();
			files = Directory.GetFiles(testHome, "__dbp.*");
			Assert.AreEqual(parts, files.Length);
		}

		uint partition(DatabaseEntry key)
		{
			if (String.Compare(
			    ASCIIEncoding.ASCII.GetString(key.Data), "i") < 0)
				return 0;
			else if (String.Compare(
			    ASCIIEncoding.ASCII.GetString(key.Data), "q") < 0)
				return 1;
			else
				return 2;
		}

		[Test]
		public void TestPrefixCompare()
		{
			testName = "TestPrefixCompare";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			     testName + ".db";

			BTreeDatabaseConfig dbConfig =
			    new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Duplicates = DuplicatesPolicy.SORTED;
			dbConfig.BTreeCompare =
			    new EntryComparisonDelegate(dbIntCompare);
			dbConfig.BTreePrefixCompare =
			    new EntryPrefixComparisonDelegate(dbPrefixCompare);
			BTreeDatabase db = BTreeDatabase.Open(
			    btreeDBFileName, dbConfig);

			Assert.AreEqual(3, db.PrefixCompare(new DatabaseEntry(
			    BitConverter.GetBytes((Int16)255)), new DatabaseEntry(
			    BitConverter.GetBytes((Int32)65791))));

			Assert.AreEqual(1, db.PrefixCompare(new DatabaseEntry(
			BitConverter.GetBytes((Int64)255)), new DatabaseEntry(
			BitConverter.GetBytes((Int64)4294967552))));

			db.Close();
		}

		[Test]
		public void TestPutMultiple()
		{
			testName = "TestPutMultiple";
			SetUpTest(true);

			PutMultiple(null);

			Configuration.ClearDir(testHome);
			DatabaseEnvironmentConfig cfg =
			    new DatabaseEnvironmentConfig();
			cfg.Create = true;
			cfg.UseLogging = true;
			cfg.UseMPool = true;
			cfg.UseTxns = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, cfg);
			PutMultiple(env);
			env.Close();
		}

		private void PutMultiple(DatabaseEnvironment env)
		{
			List<DatabaseEntry> kList = new List<DatabaseEntry>();
			List<DatabaseEntry> vList = new List<DatabaseEntry>();
			BTreeDatabase db;
			DatabaseEntry key, value;
			Transaction txn;
			string dbFileName = (env == null) ? testHome + 
			    "/" + testName + ".db" : testName + ".db";
			int i;

			BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			if (env != null) {
				dbConfig.Env = env;
				txn = env.BeginTransaction();
				db = BTreeDatabase.Open(
				    dbFileName, dbConfig, txn);
				txn.Commit();
			} else
				db = BTreeDatabase.Open(dbFileName, dbConfig);

			for (i = 0; i < 100; i++) {
				key = new DatabaseEntry(
				    BitConverter.GetBytes(i));
				value = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes("data" + i + 
				    Configuration.RandomString(512)));
				kList.Add(key);
				vList.Add(value);
			}

			// Create bulk buffer for non-recno based keys.
			MultipleDatabaseEntry keyBuff =
			    new MultipleDatabaseEntry(kList, false);
			Assert.IsFalse(keyBuff.Recno);

			// Create bulk buffer for values.
			MultipleDatabaseEntry valBuff =
			    new MultipleDatabaseEntry(vList, false);
			i = 0;
			foreach (DatabaseEntry dbt in valBuff) {
				Assert.AreEqual(vList[i].Data, dbt.Data);
				i++;
			}
			Assert.AreEqual(100, i);

			// Create bulk buffer from another key buffer.
			MultipleDatabaseEntry keyBuff1 =
			    new MultipleDatabaseEntry(
			    keyBuff.Data, keyBuff.Recno);
			i = 0;
			foreach (DatabaseEntry dbt in keyBuff1) {
				Assert.AreEqual(kList[i].Data, dbt.Data);
				i++;
			}
			Assert.AreEqual(100, i);

			if (env != null) {
				txn = env.BeginTransaction();
				db.Put(keyBuff, valBuff, txn);
				Assert.AreEqual(100, db.Truncate(txn));
				txn.Commit();
			} else {
				/*
				 * Bulk insert to database with key and value
				 * buffers.
				 */ 				 
				db.Put(keyBuff, valBuff);

				// Verify all records exist as expected.
				Cursor cursor = db.Cursor();
				i = 99;
				Assert.IsTrue(cursor.MoveLast());
				Assert.AreEqual(kList[i].Data,
				    cursor.Current.Key.Data);
				Assert.AreEqual(vList[i].Data,
				    cursor.Current.Value.Data);
				while (cursor.MovePrev()) {
					i--;
					Assert.AreEqual(kList[i].Data,
					    cursor.Current.Key.Data);
					Assert.AreEqual(vList[i].Data,
					    cursor.Current.Value.Data);
				}
				Assert.AreEqual(0, i);
				cursor.Close();
				/*
				 * Dumped all records. The number of records
				 * should be 100.
				 */ 				 
				Assert.AreEqual(100, db.Truncate());

				/* 
				 * Bulk insert to database with a copied key
				 * buffer and a value buffer.
				 */ 
				db.Put(keyBuff1, valBuff);
				cursor = db.Cursor();
				Assert.IsTrue(cursor.MoveLast());
				i = 99;
				Assert.AreEqual(kList[i].Data,
				    cursor.Current.Key.Data);
				Assert.AreEqual(vList[i].Data,
				    cursor.Current.Value.Data);
				while (cursor.MovePrev()) {
					i--;
					Assert.AreEqual(kList[i].Data,
					    cursor.Current.Key.Data);
					Assert.AreEqual(vList[i].Data,
					    cursor.Current.Value.Data);
				}
				cursor.Close();
				Assert.AreEqual(0, i);
				/*
				 * Dumped all records. The number of records
				 * should be 100.
				 */ 				 
				Assert.AreEqual(100, db.Truncate());
			}

			db.Close();
		}

		[Test]
		public void TestPutMultipleKey()
		{
			testName = "TestPutMultipleKey";
			SetUpTest(true);

			PutMultipleKey(null);

			Configuration.ClearDir(testHome);
			DatabaseEnvironmentConfig cfg =
			    new DatabaseEnvironmentConfig();
			cfg.Create = true;
			cfg.UseLogging = true;
			cfg.UseMPool = true;
			cfg.UseTxns = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, cfg);
			PutMultipleKey(env);
			env.Close();
		}

		private void PutMultipleKey(DatabaseEnvironment env)
		{
			List<KeyValuePair<DatabaseEntry, DatabaseEntry>> pList =
			    new List<KeyValuePair<
			    DatabaseEntry, DatabaseEntry>>();
			BTreeDatabase db;
			DatabaseEntry key, value;
			Transaction txn;
			string dbFileName = (env == null) ? testHome +
			    "/" + testName + ".db" : testName + ".db";
			int i;

			BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			if (env != null) {
				dbConfig.Env = env;
				txn = env.BeginTransaction();
				db = BTreeDatabase.Open(
				    dbFileName, dbConfig, txn);
				txn.Commit();
			} else
				db = BTreeDatabase.Open(dbFileName, dbConfig);
			
			for (i = 0; i < 100; i++) {
				key = new DatabaseEntry(
				    BitConverter.GetBytes(i));
				value = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes("data" + 
				    i + Configuration.RandomString(512)));
				pList.Add(new KeyValuePair<
				    DatabaseEntry, DatabaseEntry>(key, value));
			}

			// Create btree bulk buffer for key/value pairs.
			MultipleKeyDatabaseEntry pairBuff =
			    new MultipleKeyDatabaseEntry(pList, false);
			i = 0;
			foreach (KeyValuePair<DatabaseEntry, DatabaseEntry>
			    pair in pairBuff) {
				Assert.AreEqual(pList[i].Key.Data,
				    pair.Key.Data);
				Assert.AreEqual(pList[i].Value.Data,
				    pair.Value.Data);
				i++;
			}
			Assert.AreEqual(100, i);

			/*
			 * Create bulk buffer from another key/value pairs
			 * bulk buffer.
			 */ 			 
			MultipleKeyDatabaseEntry pairBuff1 =
			    new MultipleKeyDatabaseEntry(
			    pairBuff.Data, false);
			Assert.AreEqual(false, pairBuff1.Recno);
			i = 0;
			foreach (KeyValuePair<DatabaseEntry, DatabaseEntry>
			    pair in pairBuff1) {
				Assert.AreEqual(pList[i].Key.Data, 
				    pair.Key.Data);
				Assert.AreEqual(pList[i].Value.Data,
				    pair.Value.Data);
				i++;
			}
			Assert.AreEqual(100, i);

			if (env == null) {
				// Bulk insert with key/value pair bulk buffer.
				db.Put(pairBuff);
				Cursor cursor = db.Cursor();
				Assert.IsTrue(cursor.MoveFirst());
				i = 0;
				Assert.AreEqual(pList[i].Key.Data,
				     cursor.Current.Key.Data);
				Assert.AreEqual(pList[i].Value.Data,
				     cursor.Current.Value.Data);
				while (cursor.MoveNext()) {
					i++;
					Assert.AreEqual(pList[i].Key.Data,
					    cursor.Current.Key.Data);
					Assert.AreEqual(pList[i].Value.Data,
					    cursor.Current.Value.Data);
				}
				Assert.AreEqual(99, i);
				cursor.Close();
				/*
				 * Dump all records from the database. The 
				 * number of records should be 100.
				 */ 				 
				Assert.AreEqual(100, db.Truncate());

				// Bulk insert with copied key/value pair buffer.
				db.Put(pairBuff1);
				cursor = db.Cursor();
				Assert.IsTrue(cursor.MoveFirst());
				i = 0;
				Assert.AreEqual(pList[i].Key.Data,
				    cursor.Current.Key.Data);
				Assert.AreEqual(pList[i].Value.Data,
				    cursor.Current.Value.Data);
				while (cursor.MoveNext()) {
					i++;
					Assert.AreEqual(pList[i].Key.Data,
					    cursor.Current.Key.Data);
					Assert.AreEqual(pList[i].Value.Data,
					    cursor.Current.Value.Data);
				}
				Assert.AreEqual(99, i);
				cursor.Close();
				Assert.AreEqual(100, db.Truncate());
			} else {
				txn = env.BeginTransaction();
				db.Put(pairBuff, txn);
				Assert.AreEqual(100, db.Truncate(txn));
				txn.Commit();
			}

			db.Close();
		}

		[Test]
		public void TestPutWithoutTxn()
		{
			testName = "TestPutWithoutTxn";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			DatabaseEntry key, data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			using (BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBConfig))
			{
				// Put integer into database
				key = new DatabaseEntry(
				    BitConverter.GetBytes((int)0));
				data = new DatabaseEntry(
				    BitConverter.GetBytes((int)0));
				btreeDB.Put(key, data);
				pair = btreeDB.Get(key);
				Assert.AreEqual(key.Data, pair.Key.Data);
				Assert.AreEqual(data.Data, pair.Value.Data);

 				// Partial put to update the existing pair.
				data.Partial = true;
				data.PartialLen = 4;
				data.PartialOffset = 4;
				btreeDB.Put(key, data);
				pair = btreeDB.Get(key);
				Assert.AreEqual(key.Data, pair.Key.Data);
				Assert.AreEqual(8, pair.Value.Data.Length);
			}
		}

		[Test, ExpectedException(typeof(KeyExistException))]
		public void TestPutNoOverWrite()
		{
			testName = "TestPutNoOverWrite";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			DatabaseEntry key, data, newData;
			using (BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBConfig))
			{
				key = new DatabaseEntry(
				    BitConverter.GetBytes((int)0));
				data = new DatabaseEntry(
				    BitConverter.GetBytes((int)0));
				newData = new DatabaseEntry(
				    BitConverter.GetBytes((int)1));

				btreeDB.Put(key, data);
				btreeDB.PutNoOverwrite(key, newData);
			}
		}

		[Test, ExpectedException(typeof(ExpectedTestException))]
		public void TestPutNoDuplicateWithTxn()
		{
			testName = "TestPutNoDuplicateWithTxn";
			SetUpTest(true);

			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseLogging = true;
			envConfig.UseMPool = true;
			envConfig.UseTxns = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);

			Transaction txn = env.BeginTransaction();
			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			btreeDBConfig.Env = env;
			btreeDBConfig.Duplicates = DuplicatesPolicy.SORTED;

			DatabaseEntry key, data;
			try
			{
				using (BTreeDatabase btreeDB =
				    BTreeDatabase.Open(
				    testName + ".db", btreeDBConfig, txn))
				{
					key = new DatabaseEntry(
					    BitConverter.GetBytes((int)0));
					data = new DatabaseEntry(
					    BitConverter.GetBytes((int)0));

					btreeDB.Put(key, data, txn);
					btreeDB.PutNoDuplicate(key, data, txn);
				}
				txn.Commit();
			}
			catch (KeyExistException)
			{
				txn.Abort();
				throw new ExpectedTestException();
			}
			finally
			{
				env.Close();
			}
		}

		[Test, ExpectedException(typeof(ExpectedTestException))]
		public void TestRemoveDBFile()
		{
			testName = "TestRemoveDBFile";
			SetUpTest(true);
			

			RemoveDBWithoutEnv(testHome, testName, false);
		}

		[Test, ExpectedException(typeof(ExpectedTestException))]
		public void TestRemoveOneDBFromDBFile()
		{
			testName = "TestRemoveOneDBFromDBFile";
			SetUpTest(true);

			RemoveDBWithoutEnv(testHome, testName, true);
		}

		public void RemoveDBWithoutEnv(string home, string dbName, bool ifDBName)
		{
			string dbFileName = home + "/" + dbName + ".db";

			Configuration.ClearDir(home);

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;

			BTreeDatabase btreeDB;
			if (ifDBName == false)
			{
				btreeDB = BTreeDatabase.Open(dbFileName, btreeDBConfig);
				btreeDB.Close();
				BTreeDatabase.Remove(dbFileName);
				throw new ExpectedTestException();
			}
			else
			{
				btreeDB = BTreeDatabase.Open(dbFileName, dbName, btreeDBConfig);
				btreeDB.Close();
				BTreeDatabase.Remove(dbFileName, dbName);
				Assert.IsTrue(File.Exists(dbFileName));
				try
				{
					btreeDB = BTreeDatabase.Open(dbFileName, dbName, new BTreeDatabaseConfig());
					btreeDB.Close();
				}
				catch (DatabaseException)
				{
					throw new ExpectedTestException();
				}
			}
		}

		[Test]
		public void TestRemoveDBFromFileInEnv()
		{
			testName = "TestRemoveDBFromFileInEnv";
			SetUpTest(true);

			RemoveDatabase(testHome, testName, true, true);
		}

		[Test]
		public void TestRemoveDBFromEnv()
		{
			testName = "TestRemoveDBFromEnv";
			SetUpTest(true);

			RemoveDatabase(testHome, testName, true, false);
		}

		public void RemoveDatabase(string home, string dbName,
		    bool ifEnv, bool ifDBName)
		{
			string dbFileName = dbName + ".db";

			Configuration.ClearDir(home);

			DatabaseEnvironmentConfig envConfig = 
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseCDB = true;
			envConfig.UseMPool = true;

			DatabaseEnvironment env;
			env = DatabaseEnvironment.Open(home, envConfig);

			try
			{
				BTreeDatabaseConfig btreeDBConfig =
				    new BTreeDatabaseConfig();
				btreeDBConfig.Creation = CreatePolicy.ALWAYS;
				btreeDBConfig.Env = env;
				BTreeDatabase btreeDB = BTreeDatabase.Open(
				    dbFileName, dbName, btreeDBConfig);
				btreeDB.Close();

				if (ifEnv == true && ifDBName == true)
					BTreeDatabase.Remove(dbFileName, dbName, env);
				else if (ifEnv == true)
					BTreeDatabase.Remove(dbFileName, env);
			}
			catch (DatabaseException)
			{
				throw new TestException();
			}
			finally
			{
				try
				{
					BTreeDatabaseConfig dbConfig =
					    new BTreeDatabaseConfig();
					dbConfig.Creation = CreatePolicy.NEVER;
					dbConfig.Env = env;
					BTreeDatabase db = BTreeDatabase.Open(
					    dbFileName, dbName, dbConfig);
					Assert.AreEqual(db.Creation, CreatePolicy.NEVER);
					db.Close();
					throw new TestException();
				}
				catch (DatabaseException)
				{
				}
				finally
				{
					env.Close();
				}
			}
		}

		[Test]
		public void TestRenameDB()
		{
			testName = "TestRenameDB";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" + testName + ".db";
			string btreeDBName = testName;
			string newBtreeDBName = btreeDBName + "1" + ".db";

			try
			{
				BTreeDatabaseConfig btreeDBConfig =
				    new BTreeDatabaseConfig();
				btreeDBConfig.Creation = CreatePolicy.ALWAYS;
				BTreeDatabase btreeDB = BTreeDatabase.Open(
				    btreeDBFileName, btreeDBName, btreeDBConfig);
				btreeDB.Close();
				BTreeDatabase.Rename(btreeDBFileName,
				    btreeDBName, newBtreeDBName);

				BTreeDatabaseConfig dbConfig =
				    new BTreeDatabaseConfig();
				dbConfig.Creation = CreatePolicy.NEVER;
				BTreeDatabase newDB = BTreeDatabase.Open(
				    btreeDBFileName, newBtreeDBName, dbConfig);
				newDB.Close();
			}
			catch (DatabaseException e)
			{
				throw new TestException(e.Message);
			}
			finally
			{
				try
				{
					BTreeDatabaseConfig dbConfig =
					    new BTreeDatabaseConfig();
					dbConfig.Creation = CreatePolicy.NEVER;
					BTreeDatabase db = BTreeDatabase.Open(
					    btreeDBFileName, btreeDBName,
					    dbConfig);
					throw new TestException(testName);
				}
				catch (DatabaseException)
				{
				}
			}
		}

		[Test]
		public void TestRenameDBFile()
		{
			testName = "TestRenameDB";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string newBtreeDBFileName = testHome + "/" +
			    testName + "1.db";

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBConfig);
			btreeDB.Close();

			BTreeDatabase.Rename(btreeDBFileName,
			    newBtreeDBFileName);
			Assert.IsFalse(File.Exists(btreeDBFileName));
			Assert.IsTrue(File.Exists(newBtreeDBFileName));
		}

		[Test]
		public void TestSync()
		{
			testName = "TestSync";
			SetUpTest(true);
			string btreeDBName = testName + ".db";

			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.ForceFlush = true;
			envConfig.UseTxns = true;
			envConfig.UseMPool = true;
			envConfig.UseLogging = true;
			envConfig.LogSystemCfg = new LogConfig();
			envConfig.LogSystemCfg.ForceSync = false;
			envConfig.LogSystemCfg.AutoRemove = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);

			TransactionConfig txnConfig = new TransactionConfig();
			txnConfig.SyncAction =
			    TransactionConfig.LogFlush.WRITE_NOSYNC;
			Transaction txn = env.BeginTransaction(txnConfig);

			BTreeDatabaseConfig btreeConfig =
			    new BTreeDatabaseConfig();
			btreeConfig.Creation = CreatePolicy.ALWAYS;
			btreeConfig.Env = env;

			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBName, btreeConfig, txn);

			DatabaseEntry key = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("key"));
			DatabaseEntry data = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("data"));
			Assert.IsFalse(btreeDB.Exists(key, txn));
			btreeDB.Put(key, data, txn);
			btreeDB.Sync();
			btreeDB.Close(false);
			txn.Commit();
			env.Close();

			BTreeDatabaseConfig dbConfig =
			    new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.NEVER;
			using (BTreeDatabase db = BTreeDatabase.Open(
			    testHome + "/" + btreeDBName, dbConfig))
			{
				Assert.IsTrue(db.Exists(key));
			}
		}

		[Test]
		public void TestTruncate()
		{
			testName = "TestTruncate";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			btreeDBConfig.CacheSize =
			    new CacheInfo(0, 30 * 1024, 1);
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBConfig);
			DatabaseEntry key;
			DatabaseEntry data;
			for (int i = 0; i < 100; i++)
			{
				key = new DatabaseEntry(
				    BitConverter.GetBytes(i));
				data = new DatabaseEntry(
				    BitConverter.GetBytes(i));
				btreeDB.Put(key, data);
			}
			uint count = btreeDB.Truncate();
			Assert.AreEqual(100, count);
			Assert.IsFalse(btreeDB.Exists(new DatabaseEntry(
			    BitConverter.GetBytes((int)50))));
			btreeDB.Close();
		}

		[Test]
		public void TestTruncateInTxn()
		{
			testName = "TestTruncateInTxn";
			SetUpTest(true);

			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseMPool = true;
			envConfig.UseTxns = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);

			try
			{
				Transaction openTxn = env.BeginTransaction();
				BTreeDatabaseConfig dbConfig =
				    new BTreeDatabaseConfig();
				dbConfig.Creation = CreatePolicy.IF_NEEDED;
				dbConfig.Env = env;
				BTreeDatabase db = BTreeDatabase.Open(
				    testName + ".db", dbConfig, openTxn);
				openTxn.Commit();

				Transaction putTxn = env.BeginTransaction();
				try
				{
					DatabaseEntry key, data;
					for (int i = 0; i < 10; i++)
					{
						key = new DatabaseEntry(
						    BitConverter.GetBytes(i));
						data = new DatabaseEntry(
						    BitConverter.GetBytes(i));
						db.Put(key, data, putTxn);
					}

					putTxn.Commit();
				}
				catch (DatabaseException e)
				{
					putTxn.Abort();
					db.Close();
					throw e;
				}

				Transaction trunTxn = env.BeginTransaction();
				try
				{
					uint count = db.Truncate(trunTxn);
					Assert.AreEqual(10, count);
					Assert.IsFalse(db.Exists(
					    new DatabaseEntry(
					    BitConverter.GetBytes((int)5)), trunTxn));
					trunTxn.Commit();
					db.Close();
				}
				catch (DatabaseException)
				{
					trunTxn.Abort();
					db.Close();
					throw new TestException();
				}
			}
			catch (DatabaseException)
			{
			}
			finally
			{
				env.Close();
			}	
		}

		[Test]
		public void TestTruncateUnusedPages()
		{
			testName = "TestTruncateUnusedPages";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";

			BTreeDatabaseConfig dbConfig =
			    new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.ALWAYS;
			dbConfig.PageSize = 512;
			BTreeDatabase db = BTreeDatabase.Open(
			    dbFileName, dbConfig);
			DatabaseEntry key, data;
			for (int i = 0; i < 100; i++)
			{
				key = new DatabaseEntry(
				    BitConverter.GetBytes(i));
				data = new DatabaseEntry(
				    BitConverter.GetBytes(i));
				db.Put(key, data);
			}

			for (int i = 0; i < 80; i++)
				db.Delete(new DatabaseEntry(
				    BitConverter.GetBytes(i)));

			uint count = db.TruncateUnusedPages();
			Assert.LessOrEqual(0, count);

			db.Close();
		}

		[Test]
		public void TestTruncateUnusedPagesWithTxn()
		{
			testName = "TestTruncateUnusedPagesWithTxn";
			SetUpTest(true);

			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseMPool = true;
			envConfig.UseTxns = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);

			BTreeDatabase db;
			try
			{
				Transaction openTxn = env.BeginTransaction();
				try
				{
					BTreeDatabaseConfig dbConfig =
					    new BTreeDatabaseConfig();
					dbConfig.Creation = CreatePolicy.IF_NEEDED;
					dbConfig.Env = env;
					dbConfig.PageSize = 512;
					db = BTreeDatabase.Open(
					    testName + ".db", dbConfig, openTxn);
					openTxn.Commit();
					Assert.AreEqual(512, db.Pagesize);
				}
				catch (DatabaseException e)
				{
					openTxn.Abort();
					throw e;
				}
				
				Transaction putTxn = env.BeginTransaction();
				try
				{
					DatabaseEntry key, data;
					for (int i = 0; i < 100; i++)
					{
						key = new DatabaseEntry(
						    BitConverter.GetBytes(i));
						data = new DatabaseEntry(
						    BitConverter.GetBytes(i));
						db.Put(key, data, putTxn);
					}

					putTxn.Commit();
				}
				catch (DatabaseException e)
				{
					putTxn.Abort();
					db.Close();
					throw e;
				}

				Transaction delTxn = env.BeginTransaction();
				try
				{
					for (int i = 20; i <= 80; i++)
						db.Delete(new DatabaseEntry(
						    BitConverter.GetBytes(i)), delTxn);
					delTxn.Commit();
				}
				catch (DatabaseException e)
				{
					delTxn.Abort();
					db.Close();
					throw e;
				}

				Transaction trunTxn = env.BeginTransaction();
				try
				{
					uint trunPages = db.TruncateUnusedPages(
					    trunTxn);
					Assert.LessOrEqual(0, trunPages);
					trunTxn.Commit();
					db.Close();
				}
				catch (DatabaseException)
				{
					trunTxn.Abort();
					db.Close();
					throw new Exception();
				}
			}
			catch (DatabaseException)
			{
			}
			finally
			{
				env.Close();
			}
		}

		[Test]
		public void TestSalvage()
		{
			testName = "TestSalvage";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string printableOutPut = testHome + "/" +
			    "printableOutPut";
			string inprintableOutPut = testHome + "/" +
			    "inprintableOutPut";

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBConfig);

			DatabaseEntry key;
			DatabaseEntry data;

			for (uint i = 0; i < 10; i++)
			{
				key = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes(i.ToString()));
				data = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes(i.ToString()));
				btreeDB.Put(key, data);
			}

			btreeDB.Close();

			StreamWriter sw1 = new StreamWriter(printableOutPut);
			StreamWriter sw2 = new StreamWriter(inprintableOutPut);
			BTreeDatabase.Salvage(btreeDBFileName, btreeDBConfig,
			    true, true, sw1);
			BTreeDatabase.Salvage(btreeDBFileName, btreeDBConfig,
			    false, true, sw2);
			sw1.Close();
			sw2.Close();

			FileStream file1 = new FileStream(printableOutPut,
			    FileMode.Open);
			FileStream file2 = new FileStream(inprintableOutPut,
			    FileMode.Open);
			if (file1.Length == file2.Length)
			{
				int filebyte1 = 0;
				int filebyte2 = 0;
				do
				{
					filebyte1 = file1.ReadByte();
					filebyte2 = file2.ReadByte();
				} while ((filebyte1 == filebyte2) &&
				    (filebyte1 != -1));
				Assert.AreNotEqual(filebyte1, filebyte2);
			}

			file1.Close();
			file2.Close();
		}

		[Test]
		public void TestUpgrade()
		{
			testName = "TestUpgrade";
			SetUpTest(true);

			string srcDBFileName = "../../bdb4.7.db";
			string testDBFileName = testHome + "/bdb4.7.db";

			FileInfo srcDBFileInfo = new FileInfo(srcDBFileName);

			//Copy the file.
			srcDBFileInfo.CopyTo(testDBFileName);
			Assert.IsTrue(File.Exists(testDBFileName));

			BTreeDatabase.Upgrade(testDBFileName,
			    new DatabaseConfig(), true);

			// Open the upgraded database file.
			BTreeDatabase db = BTreeDatabase.Open(
			    testDBFileName, new BTreeDatabaseConfig());
			db.Close();
		}

		[Test, ExpectedException(typeof(TestException))]
		public void TestVerify()
		{
			testName = "TestVerify";
			SetUpTest(true);
			string btreeDBFileName = testHome + "/" +
			    testName + ".db";
			string btreeDBName =
			    Path.GetFileNameWithoutExtension(btreeDBFileName);

			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    btreeDBFileName, btreeDBName, btreeDBConfig);
			btreeDB.Close();
			btreeDBConfig.Duplicates = DuplicatesPolicy.SORTED;
			BTreeDatabase.Verify(btreeDBFileName, btreeDBConfig,
			    Database.VerifyOperation.NO_ORDER_CHECK);
			try
			{
				BTreeDatabase.Verify(btreeDBFileName,
				    btreeDBConfig,
				    Database.VerifyOperation.ORDER_CHECK_ONLY);
			}
			catch (DatabaseException)
			{
				throw new TestException(testName);
			}
			finally
			{
				BTreeDatabase.Verify(btreeDBFileName,
				    btreeDBName, btreeDBConfig,
				    Database.VerifyOperation.ORDER_CHECK_ONLY);
			}

		}

		[Test]
		public void TestStats()
		{
			testName = "TestStats";
			SetUpTest(true);
			string dbFileName = testHome + "/" +
			    testName + ".db";

			BTreeDatabaseConfig dbConfig =
			    new BTreeDatabaseConfig();
			ConfigCase1(dbConfig);
			BTreeDatabase db = BTreeDatabase.Open(dbFileName,
			    dbConfig);

			BTreeStats stats = db.Stats();
			ConfirmStatsPart1Case1(stats);

			// Put 500 records into the database.
			PutRecordCase1(db, null);

			stats = db.Stats();
			ConfirmStatsPart2Case1(stats);

			// Delete some data to get some free pages.
			byte[] bigArray = new byte[10240];
			db.Delete(new DatabaseEntry(bigArray));

			db.PrintStats();
			db.PrintFastStats();

			db.Close();
		}

		[Test]
		public void TestStatsInTxn()
		{
			testName = "TestStatsInTxn";
			SetUpTest(true);

			StatsInTxn(testHome, testName, false);
		}

		[Test]
		public void TestStatsWithIsolation()
		{
			testName = "TestStatsWithIsolation";
			SetUpTest(true);

			StatsInTxn(testHome, testName, true);
		}

        [Test]
        public void TestMultipleDBSingleFile()
        {
            testName = "TestMultipleDBSingleFile";
            SetUpTest(true);
            string btreeDBName = testHome + "/" + testName + ".db";

            string dbName = "test";

            /* Create and initialize database object, open the database. */
            BTreeDatabaseConfig btreeDBconfig = new BTreeDatabaseConfig();
            btreeDBconfig.Creation = CreatePolicy.IF_NEEDED;
            btreeDBconfig.ErrorPrefix = testName;
            btreeDBconfig.UseRecordNumbers = true;

            BTreeDatabase btreeDB = BTreeDatabase.Open(btreeDBName, dbName,
                btreeDBconfig);
            btreeDB.Close();
            btreeDB = BTreeDatabase.Open(btreeDBName, dbName + "2",
                btreeDBconfig);
            btreeDB.Close();

            BTreeDatabaseConfig dbcfg = new BTreeDatabaseConfig();
            dbcfg.ReadOnly = true;
            BTreeDatabase newDb = BTreeDatabase.Open(btreeDBName, dbcfg);
            Boolean val = newDb.HasMultiple;
            Assert.IsTrue(val);
            newDb.Close();
        }

		public void StatsInTxn(string home, string name, bool ifIsolation)
		{
			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			EnvConfigCase1(envConfig);
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    home, envConfig);

			Transaction openTxn = env.BeginTransaction();
			BTreeDatabaseConfig dbConfig =
			    new BTreeDatabaseConfig();
			ConfigCase1(dbConfig);
			dbConfig.Env = env;
			BTreeDatabase db = BTreeDatabase.Open(name + ".db",
			    dbConfig, openTxn);
			openTxn.Commit();

			Transaction statsTxn = env.BeginTransaction();
			BTreeStats stats;
			BTreeStats fastStats;
			if (ifIsolation == false)
			{
				stats = db.Stats(statsTxn);
				fastStats = db.FastStats(statsTxn);
			}
			else
			{
				stats = db.Stats(statsTxn, Isolation.DEGREE_ONE);
				fastStats = db.FastStats(statsTxn, 
				    Isolation.DEGREE_ONE);
			}
			ConfirmStatsPart1Case1(stats);

			// Put 500 records into the database.
			PutRecordCase1(db, statsTxn);

			if (ifIsolation == false)
				stats = db.Stats(statsTxn);
			else
				stats = db.Stats(statsTxn, Isolation.DEGREE_TWO);
			ConfirmStatsPart2Case1(stats);

			// Delete some data to get some free pages.
			byte[] bigArray = new byte[10240];
			db.Delete(new DatabaseEntry(bigArray), statsTxn);
			if (ifIsolation == false)
				stats = db.Stats(statsTxn);
			else
				stats = db.Stats(statsTxn, Isolation.DEGREE_THREE);
			ConfirmStatsPart3Case1(stats);

			db.PrintStats(true);
			Assert.AreEqual(0, stats.EmptyPages);

			statsTxn.Commit();
			db.Close();
			env.Close();
		}

		public void EnvConfigCase1(DatabaseEnvironmentConfig cfg)
		{
			cfg.Create = true;
			cfg.UseTxns = true;
			cfg.UseMPool = true;
			cfg.UseLogging = true;
		}

		public void ConfigCase1(BTreeDatabaseConfig dbConfig)
		{
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Duplicates = DuplicatesPolicy.UNSORTED;
			dbConfig.PageSize = 4096;
			dbConfig.MinKeysPerPage = 10;
		}

		public void PutRecordCase1(BTreeDatabase db, Transaction txn)
		{
			byte[] bigArray = new byte[10240];
			for (int i = 0; i < 100; i++)
			{
				if (txn == null)
					db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
					    new DatabaseEntry(BitConverter.GetBytes(i)));
				else
					db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
					    new DatabaseEntry(BitConverter.GetBytes(i)), txn);
			}
			for (int i = 100; i < 500; i++)
			{
				if (txn == null)
					db.Put(new DatabaseEntry(bigArray),
					    new DatabaseEntry(bigArray));
				else
					db.Put(new DatabaseEntry(bigArray),
					    new DatabaseEntry(bigArray), txn);
			}
		}

		public void ConfirmStatsPart1Case1(BTreeStats stats)
		{
			Assert.AreEqual(1, stats.EmptyPages);
			Assert.AreNotEqual(0, stats.LeafPagesFreeBytes);
			Assert.AreEqual(1, stats.Levels);
			Assert.AreNotEqual(0, stats.MagicNumber);
			Assert.AreEqual(1, stats.MetadataFlags);
			Assert.AreEqual(10, stats.MinKey);
			Assert.AreEqual(2, stats.nPages);
			Assert.AreEqual(4096, stats.PageSize);
			Assert.AreEqual(10, stats.Version);
		}

		public void ConfirmStatsPart2Case1(BTreeStats stats)
		{
			Assert.AreNotEqual(0, stats.DuplicatePages);
			Assert.AreNotEqual(0, stats.DuplicatePagesFreeBytes);
			Assert.AreNotEqual(0, stats.InternalPages);
			Assert.AreNotEqual(0, stats.InternalPagesFreeBytes);
			Assert.AreNotEqual(0, stats.LeafPages);
			Assert.AreEqual(500, stats.nData);
			Assert.AreEqual(101, stats.nKeys);
			Assert.AreNotEqual(0, stats.OverflowPages);
			Assert.AreNotEqual(0, stats.OverflowPagesFreeBytes);
		}

		public void ConfirmStatsPart3Case1(BTreeStats stats)
		{
			Assert.AreNotEqual(0, stats.FreePages);
		}

		private int dbIntCompare(DatabaseEntry dbt1,
		    DatabaseEntry dbt2)
		{
			int a, b;
			a = BitConverter.ToInt32(dbt1.Data, 0);
			b = BitConverter.ToInt32(dbt2.Data, 0);
			return a - b;
		}

		private uint dbPrefixCompare(DatabaseEntry dbt1,
		    DatabaseEntry dbt2)
		{
			uint cnt, len;

			len = Math.Min((uint)dbt1.Data.Length, (uint)dbt2.Data.Length);
			for (cnt = 0; cnt < len; cnt++)
			{
				if (dbt1.Data[cnt] != dbt2.Data[cnt])
					return cnt + 1;
			}
			if (dbt1.Data.Length > dbt2.Data.Length)
				return (uint)dbt2.Data.Length + 1;
			else if (dbt1.Data.Length < dbt2.Data.Length)
				return (uint)dbt1.Data.Length + 1;
			else
				return (uint)dbt1.Data.Length;
		}

		public void SetUpEnvAndTxn(string home,
		    out DatabaseEnvironment env, out Transaction txn)
		{
			DatabaseEnvironmentConfig envConfig =
				new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseTxns = true;
			envConfig.UseMPool = true;
			env = DatabaseEnvironment.Open(home, envConfig);
			txn = env.BeginTransaction();
		}

		public void OpenBtreeDB(DatabaseEnvironment env,
		    Transaction txn, string dbFileName,
		    out BTreeDatabase db)
		{
			BTreeDatabaseConfig dbConfig =
			    new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			if (env != null)
			{
				dbConfig.Env = env;
				dbConfig.NoMMap = false;
				db = BTreeDatabase.Open(dbFileName, dbConfig, txn);
			}
			else
			{
				db = BTreeDatabase.Open(dbFileName, dbConfig);
			}
		}

                private BTreeDatabase GetMultipleDB(
                    string filename, string dbname, BTreeDatabaseConfig cfg) {
                        BTreeDatabase ret;
                        DatabaseEntry data, key;
 
                        ret = BTreeDatabase.Open(filename, dbname, cfg);
                        key = null;
                        if (cfg.UseRecordNumbers) {
                                /* 
                                 * Dups aren't allowed with record numbers, so
                                 * we have to put different data.  Also, record
                                 * numbers start at 1, so we do too, which makes 
                                 * checking results easier.
                                 */
                                for (int i = 1; i < 100; i++) {
                                        key = new DatabaseEntry(
                                            BitConverter.GetBytes(i));
                                        data = new DatabaseEntry(
                                            BitConverter.GetBytes(i));
                                        ret.Put(key, data);
                                }

                                key = new DatabaseEntry(
                                    BitConverter.GetBytes(100));
                                data = new DatabaseEntry();
                                data.Data = new byte[111];
                                for (int i = 0; i < 111; i++)
                                        data.Data[i] = (byte)i;
                                        ret.Put(key, data);
                        } else {
                                for (int i = 0; i < 100; i++) {
                                        if (i % 10 == 0)
                                        key = new DatabaseEntry(
                                            BitConverter.GetBytes(i));
                                        data = new DatabaseEntry(
                                            BitConverter.GetBytes(i));
                                        /* Don't put nulls into the db. */
                                        Assert.IsFalse(key == null);
                                        Assert.IsFalse(data == null);
                                        ret.Put(key, data);
                                }
                                
                                if (cfg.Duplicates == DuplicatesPolicy.UNSORTED) {
                                        /* Add in duplicates to check GetBothMultiple */
                                        key = new DatabaseEntry(
                                            BitConverter.GetBytes(100));
                                        data = new DatabaseEntry(
                                            BitConverter.GetBytes(100));
                                        for (int i = 0; i < 10; i++)
                                                ret.Put(key, data);

                                        /*
                                         * Add duplicates to check GetMultiple 
                                         * with given buffer size. 
                                         */
                                        for (int i = 101; i < 1024; i++) {
                                                key = new DatabaseEntry(
                                                    BitConverter.GetBytes(101));
                                                data = new DatabaseEntry(
                                                    BitConverter.GetBytes(i));
                                                ret.Put(key, data);
                                        }

                                        key = new DatabaseEntry(
                                            BitConverter.GetBytes(102));
                                        data = new DatabaseEntry();
                                        data.Data = new byte[112];
                                        for (int i = 0; i < 112; i++)
			                     data.Data[i] = (byte)i;
                                        ret.Put(key, data);
                                }
                        }
	               return ret;	
                }

		public static void Confirm(XmlElement xmlElem,
		    BTreeDatabase btreeDB, bool compulsory)
		{
			DatabaseTest.Confirm(xmlElem, btreeDB, compulsory);
			Configuration.ConfirmDuplicatesPolicy(xmlElem,
			    "Duplicates", btreeDB.Duplicates, compulsory);
			Configuration.ConfirmUint(xmlElem, "MinKeysPerPage",
			    btreeDB.MinKeysPerPage, compulsory);
			/*
			 * BTreeDatabase.RecordNumbers is the value of
			 * BTreeDatabaseConfig.UseRecordNumbers.
			 */
			Configuration.ConfirmBool(xmlElem, "UseRecordNumbers",
			    btreeDB.RecordNumbers, compulsory);
			/*
			 * BTreeDatabase.ReverseSplit is the value of
			 * BTreeDatabaseConfig.NoReverseSplitting.
			 */
			Configuration.ConfirmBool(xmlElem, "NoReverseSplitting",
			    btreeDB.ReverseSplit, compulsory);
			Assert.AreEqual(DatabaseType.BTREE, btreeDB.Type);
			string type = btreeDB.ToString();
			Assert.IsNotNull(type);
		}
	}
}

