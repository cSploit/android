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
using System.Threading;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
	[TestFixture]
	public class SecondaryDatabaseTest : CSharpTestFixture
	{

		[TestFixtureSetUp]
		public void SetUpTestFixture()
		{
			testFixtureName = "SecondaryDatabaseTest";
			base.SetUpTestfixture();
		}

		[Test]
		public void TestDeleteMultiple()
		{
			testName = "TestDeleteMultiple";
			SetUpTest(true);
			DeleteMultipleAndMultipleKey(
			    testHome + "/" + testName + ".db", 
			    DatabaseType.BTREE.ToString(), 
			    DatabaseType.BTREE, false);
			DeleteMultipleAndMultipleKey(
			    testHome + "/" + testName + ".db",
			    DatabaseType.HASH.ToString(),
			    DatabaseType.HASH, false);
			DeleteMultipleAndMultipleKey(
			    testHome + "/" + testName + ".db",
			    DatabaseType.QUEUE.ToString(),
			    DatabaseType.QUEUE, false);
			DeleteMultipleAndMultipleKey(
			    testHome + "/" + testName + ".db",
			    DatabaseType.RECNO.ToString(),
			    DatabaseType.RECNO, false);
		}

		[Test]
		public void TestDeleteMultipleKey()
		{
			testName = "TestDeleteMultipleKey";
			SetUpTest(true);
			DeleteMultipleAndMultipleKey(
			    testHome + "/" + testName + ".db",
			    DatabaseType.BTREE.ToString(),
			    DatabaseType.BTREE, true);
			DeleteMultipleAndMultipleKey(
			    testHome + "/" + testName + ".db",
			    DatabaseType.HASH.ToString(),
			    DatabaseType.HASH, true);
			DeleteMultipleAndMultipleKey(
			    testHome + "/" + testName + ".db",
			    DatabaseType.QUEUE.ToString(),
			    DatabaseType.QUEUE, true);
			DeleteMultipleAndMultipleKey(
			    testHome + "/" + testName + ".db",
			    DatabaseType.RECNO.ToString(),
			    DatabaseType.RECNO, true);
		}

		private void DeleteMultipleAndMultipleKey(string dbFileName,
		    string dbName, DatabaseType type, bool mulKey)
		{
			List<DatabaseEntry> kList = new List<DatabaseEntry>();
			List<uint> rList = new List<uint>();
			List<KeyValuePair<DatabaseEntry, DatabaseEntry>> pList =
			    new List<KeyValuePair<DatabaseEntry, DatabaseEntry>>();
			DatabaseEntry key;
			Database db;
			SecondaryDatabase secDb;

			Configuration.ClearDir(testHome);

			if (type == DatabaseType.BTREE) {
				BTreeDatabaseConfig dbConfig =
				    new BTreeDatabaseConfig();
				dbConfig.Creation = CreatePolicy.IF_NEEDED;
				db = BTreeDatabase.Open(
				    dbFileName, dbName, dbConfig);
				SecondaryBTreeDatabaseConfig secDbConfig =
				    new SecondaryBTreeDatabaseConfig(db, null);
				secDbConfig.Creation = CreatePolicy.IF_NEEDED;
				secDbConfig.Duplicates = DuplicatesPolicy.SORTED;
				secDbConfig.KeyGen = 
				    new SecondaryKeyGenDelegate(SecondaryKeyGen);
				secDb = SecondaryBTreeDatabase.Open(
				    dbFileName, dbName + "_sec", secDbConfig);
			} else if (type == DatabaseType.HASH) {
				HashDatabaseConfig dbConfig =
				    new HashDatabaseConfig();
				dbConfig.Creation = CreatePolicy.IF_NEEDED;
				db = HashDatabase.Open(
				    dbFileName, dbName, dbConfig);
				SecondaryHashDatabaseConfig secDbConfig =
				    new SecondaryHashDatabaseConfig(db, null);
				secDbConfig.Creation = CreatePolicy.IF_NEEDED;
				secDbConfig.Duplicates = DuplicatesPolicy.SORTED;
				secDbConfig.KeyGen =
				    new SecondaryKeyGenDelegate(SecondaryKeyGen);
				secDb = SecondaryHashDatabase.Open(
				    dbFileName, dbName + "_sec", secDbConfig);
			} else if (type == DatabaseType.QUEUE) {
				QueueDatabaseConfig dbConfig =
				    new QueueDatabaseConfig();
				dbConfig.Creation = CreatePolicy.IF_NEEDED;
				dbConfig.Length = 4;
				db = QueueDatabase.Open(dbFileName, dbConfig);
				SecondaryQueueDatabaseConfig secDbConfig =
				    new SecondaryQueueDatabaseConfig(db, null);
				secDbConfig.Creation = CreatePolicy.IF_NEEDED;
				secDbConfig.Length = 4;
				secDbConfig.KeyGen =
				    new SecondaryKeyGenDelegate(SecondaryKeyGen);
				secDb = SecondaryQueueDatabase.Open(
				    dbFileName + "_sec", secDbConfig);
			} else if (type == DatabaseType.RECNO) {
				RecnoDatabaseConfig dbConfig =
				    new RecnoDatabaseConfig();
				dbConfig.Creation = CreatePolicy.IF_NEEDED;
				db = RecnoDatabase.Open(
				    dbFileName, dbName, dbConfig);
				SecondaryRecnoDatabaseConfig secDbConfig =
				    new SecondaryRecnoDatabaseConfig(db, null);
				secDbConfig.Creation = CreatePolicy.IF_NEEDED;
				secDbConfig.KeyGen =
				    new SecondaryKeyGenDelegate(SecondaryKeyGen);
				secDb = SecondaryRecnoDatabase.Open(
				    dbFileName, dbName + "_sec", secDbConfig);
			} else
				throw new TestException();

			for (uint i = 1; i <= 100; i++) {
				key = new DatabaseEntry(
				    BitConverter.GetBytes(i));
				if (i >= 50 && i < 60)
					kList.Add(key);
				else if (i > 80)
					pList.Add(new KeyValuePair<
					    DatabaseEntry, DatabaseEntry>(
					    key, key));
				else if (type == DatabaseType.QUEUE
				    || type == DatabaseType.RECNO)
					rList.Add(i);
				
				db.Put(key, key);
			}

			if (mulKey) {
				// Create bulk buffer for key/value pairs.
				MultipleKeyDatabaseEntry pBuff;
				if (type == DatabaseType.BTREE)
					pBuff = new MultipleKeyDatabaseEntry(
					    pList, false);
				else if (type == DatabaseType.HASH)
					pBuff = new MultipleKeyDatabaseEntry(
					    pList, false);
				else if (type == DatabaseType.QUEUE)
					pBuff = new MultipleKeyDatabaseEntry(
					    pList, true);
				else
					pBuff = new MultipleKeyDatabaseEntry(
					    pList, true);

				// Bulk delete with the key/value pair bulk buffer.
				secDb.Delete(pBuff);
				foreach (KeyValuePair<DatabaseEntry,
				    DatabaseEntry>pair in pList) {
					try {
						db.GetBoth(pair.Key, pair.Value);
						throw new TestException();
					} catch (NotFoundException e1) {
						if (type == DatabaseType.QUEUE)
							throw e1;
					} catch (KeyEmptyException e2) {
						if (type == DatabaseType.BTREE ||
						    type == DatabaseType.HASH ||
						    type == DatabaseType.RECNO)
							throw e2;
					}
				}

				/*
				 * Dump the database to verify that 80 records
				 * remain after bulk delete.
				 */ 				 
				Assert.AreEqual(80, db.Truncate());
			} else {
				// Create bulk buffer for key.
				MultipleDatabaseEntry kBuff;
				if (type == DatabaseType.BTREE)
					kBuff = new MultipleDatabaseEntry(
					    kList, false);
				else if (type == DatabaseType.HASH)
					kBuff = new MultipleDatabaseEntry(
					    kList, false);
				else if (type == DatabaseType.QUEUE)
					kBuff = new MultipleDatabaseEntry(
					    kList, true);
				else
					kBuff = new MultipleDatabaseEntry(
					    kList, true);

				/*
				 * Bulk delete in secondary database with key
				 * buffer. Primary records that the deleted 
				 * records in secondar database should be
				 * deleted as well.
				 */ 
				secDb.Delete(kBuff);
				foreach (DatabaseEntry dbt in kList) {
					try {
						db.Get(dbt);
						throw new TestException();
					} catch (NotFoundException e1) {
						if (type == DatabaseType.QUEUE ||
						    type == DatabaseType.RECNO)
							throw e1;
					} catch (KeyEmptyException e2) {
						if (type == DatabaseType.BTREE ||
						    type == DatabaseType.HASH)
							throw e2;
					}
				}

				/*
				 * Bulk delete in secondary database with recno
				 * based key buffer.
				 */ 				 
				if (type == DatabaseType.QUEUE ||
				    type == DatabaseType.RECNO) {
					MultipleDatabaseEntry rBuff =
					    new MultipleDatabaseEntry(rList);
					secDb.Delete(rBuff);
					Assert.AreEqual(20, db.Truncate());
				}
			}

			secDb.Close();
			db.Close();
		}

		[Test]
		public void TestKeyGen()
		{
			testName = "TestKeyGen";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";

			// Open primary database.
			BTreeDatabaseConfig primaryDBConfig =
			    new BTreeDatabaseConfig();
			primaryDBConfig.Creation = CreatePolicy.IF_NEEDED;
			BTreeDatabase primaryDB =
			    BTreeDatabase.Open(dbFileName, primaryDBConfig);

			// Open secondary database.
			SecondaryBTreeDatabaseConfig secDBConfig =
			    new SecondaryBTreeDatabaseConfig(primaryDB,
			    new SecondaryKeyGenDelegate(SecondaryKeyGen));
			SecondaryBTreeDatabase secDB =
			    SecondaryBTreeDatabase.Open(dbFileName,
			    secDBConfig);

			primaryDB.Put(new DatabaseEntry(
			    BitConverter.GetBytes((int)1)),
			    new DatabaseEntry(BitConverter.GetBytes((int)11)));

			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			pair = secDB.Get(new DatabaseEntry(
			    BitConverter.GetBytes((int)11)));
			Assert.IsNotNull(pair.Value);

			// Close secondary database.
			secDB.Close();

			// Close primary database.
			primaryDB.Close();
		}

        [Test]
        public void TestMultiKeyGen() {
            testName = "TestPrefixCompare";
            SetUpTest(true);
            string dbFileName = testHome + "/" + testName + ".db";

            // Open a primary btree database.
            BTreeDatabaseConfig btreeDBConfig =
                new BTreeDatabaseConfig();
            btreeDBConfig.Creation = CreatePolicy.ALWAYS;
            BTreeDatabase btreeDB = BTreeDatabase.Open(
                dbFileName, btreeDBConfig);

            // Open a secondary btree database. 
            SecondaryBTreeDatabaseConfig secBtreeDBConfig =
                new SecondaryBTreeDatabaseConfig(btreeDB, null);
            secBtreeDBConfig.Primary = btreeDB;
            secBtreeDBConfig.KeyGen =
                new SecondaryKeyGenDelegate(
                MultipleKeyGen);
            SecondaryBTreeDatabase secDB =
                SecondaryBTreeDatabase.Open(
                dbFileName, secBtreeDBConfig);

            /* Check that multiple secondary keys work */
            DatabaseEntry key, data;
            String keyStr = "key";
            key = new DatabaseEntry();
            Configuration.dbtFromString(key, keyStr);

            data = new DatabaseEntry();
            String[] dataStrs = { "abc", "def", "ghi", "jkl" };
            String dataStr = String.Join(",", dataStrs);
            Configuration.dbtFromString(data, dataStr);
            btreeDB.Put(key, data);

            foreach (String skeyStr in dataStrs) {
                DatabaseEntry skey = new DatabaseEntry();
                Configuration.dbtFromString(skey, skeyStr);
                Assert.IsTrue(secDB.Exists(skey));
            }

            /*
             * Check that a single secondary key, returned in a
             * MultipleDatabaseEntry works.
             */ 
            keyStr = "key2";
            key = new DatabaseEntry();
            Configuration.dbtFromString(key, keyStr);

            data = new DatabaseEntry();
            dataStrs = new string[1]{ "abcdefghijkl" };
            dataStr = String.Join(",", dataStrs);
            Configuration.dbtFromString(data, dataStr);
            btreeDB.Put(key, data);

            // Check that secondary keys work
            foreach (String skeyStr in dataStrs) {
                DatabaseEntry skey = new DatabaseEntry();
                Configuration.dbtFromString(skey, skeyStr);
                Assert.IsTrue(secDB.Exists(skey));
            }
        }

        public DatabaseEntry MultipleKeyGen(DatabaseEntry key, DatabaseEntry data) {
            LinkedList<DatabaseEntry> skeys = new LinkedList<DatabaseEntry>();
            String dataStr = Configuration.strFromDBT(data);
            foreach (String s in dataStr.Split(',')) {
                DatabaseEntry tmp = new DatabaseEntry();
                Configuration.dbtFromString(tmp, s);
                skeys.AddLast(tmp);
            }
            return new MultipleDatabaseEntry(skeys, false);
        }

		[Test]
		public void TestSecondaryCursor()
		{
			testName = "TestSecondaryCursor";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";

			// Open primary database.
			BTreeDatabaseConfig primaryDBConfig =
			    new BTreeDatabaseConfig();
			primaryDBConfig.Creation = CreatePolicy.IF_NEEDED;
			BTreeDatabase primaryDB =
			    BTreeDatabase.Open(dbFileName, primaryDBConfig);

			// Open secondary database.
			SecondaryBTreeDatabaseConfig secDBConfig =
			    new SecondaryBTreeDatabaseConfig(primaryDB,
			    new SecondaryKeyGenDelegate(SecondaryKeyGen));
			SecondaryBTreeDatabase secDB =
			    SecondaryBTreeDatabase.Open(dbFileName,
			    secDBConfig);

			primaryDB.Put(new DatabaseEntry(
			    BitConverter.GetBytes((int)1)),
			    new DatabaseEntry(BitConverter.GetBytes((int)11)));

			SecondaryCursor cursor = secDB.SecondaryCursor();
			cursor.Move(new DatabaseEntry(
			    BitConverter.GetBytes((int)11)), true);
			Assert.AreEqual(BitConverter.GetBytes((int)11),
			    cursor.Current.Key.Data);

			// Close the cursor.
			cursor.Close();

			// Close secondary database.
			secDB.Close();

			// Close primary database.
			primaryDB.Close();
		}

		[Test]
		public void TestSecondaryCursorWithConfig()
		{
			testName = "TestSecondaryCursorWithConfig";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";

			BTreeDatabase db;
			SecondaryBTreeDatabase secDB;
			OpenPrimaryAndSecondaryDB(dbFileName, out db, out secDB);

			for (int i = 0; i < 10; i++)
				db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
				    new DatabaseEntry(BitConverter.GetBytes((int)i)));

			CursorConfig cursorConfig = new CursorConfig();
			cursorConfig.WriteCursor = false;
			SecondaryCursor cursor =
			    secDB.SecondaryCursor(cursorConfig);

			cursor.Move(new DatabaseEntry(
			    BitConverter.GetBytes((int)5)), true);

			Assert.AreEqual(1, cursor.Count());

			// Close the cursor.
			cursor.Close();

			// Close secondary database.
			secDB.Close();

			// Close primary database.
			db.Close();
		}

		[Test]
		public void TestSecondaryCursorWithTxn()
		{
			testName = "TestSecondaryCursorWithTxn";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";

			GetSecondaryCursurWithTxn(testHome, testName, false);
		}

		[Test]
		public void TestSecondaryCursorWithConfigAndTxn()
		{
			testName = "TestSecondaryCursorWithConfigAndTxn";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";

			GetSecondaryCursurWithTxn(testHome, testName, true);
		}

		public void GetSecondaryCursurWithTxn(string home,
		    string name, bool ifCfg)
		{
			string dbFileName = name + ".db";
			SecondaryCursor cursor;

			// Open env.
			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseTxns = true;
			envConfig.UseMPool = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(home,
			    envConfig);

			// Open primary/secondary database.
			Transaction txn = env.BeginTransaction();
			BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Env = env;
			BTreeDatabase db = BTreeDatabase.Open(dbFileName,
			    dbConfig, txn);

			SecondaryBTreeDatabaseConfig secDBConfig = new
			    SecondaryBTreeDatabaseConfig(db,
			    new SecondaryKeyGenDelegate(SecondaryKeyGen));
			secDBConfig.Env = env;
			SecondaryBTreeDatabase secDB =
			    SecondaryBTreeDatabase.Open(dbFileName,
			    secDBConfig, txn);

			for (int i = 0; i < 10; i++)
				db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
				    new DatabaseEntry(BitConverter.GetBytes((int)i)), txn);

			// Create secondary cursor.
			if (ifCfg == false)
				secDB.SecondaryCursor(txn);
			else if (ifCfg == true)
			{
				CursorConfig cursorConfig = new CursorConfig();
				cursorConfig.WriteCursor = false;
				cursor = secDB.SecondaryCursor(cursorConfig, txn);
				cursor.Close();
			}

			secDB.Close();
			db.Close();
			txn.Commit();
			env.Close();
		}

		[Test]
		public void TestOpen()
		{
			testName = "TestOpen";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";
			string dbSecFileName = testHome + "/" +
			    testName + "_sec.db";

			OpenSecQueueDB(dbFileName, dbSecFileName, false);
		}

		[Test]
		public void TestOpenWithDBName()
		{
			testName = "TestOpenWithDBName";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";
			string dbSecFileName = testHome + "/" +
			    testName + "_sec.db";

			OpenSecQueueDB(dbFileName, dbSecFileName, true);
		}

		public void OpenSecQueueDB(string dbFileName, 
		    string dbSecFileName, bool ifDBName)
		{
			// Open a primary btree database.
			BTreeDatabaseConfig primaryDBConfig =
			    new BTreeDatabaseConfig();
			primaryDBConfig.Creation = CreatePolicy.IF_NEEDED;
			BTreeDatabase primaryDB;

			/*
			 * If secondary database name is given, the primary 
			 * database is also opened with database name.
			 */
			if (ifDBName == false)
				primaryDB = BTreeDatabase.Open(dbFileName,
				    primaryDBConfig);
			else
				primaryDB = BTreeDatabase.Open(dbFileName,
				    "primary", primaryDBConfig);

			try
			{
				// Open a new secondary database.
				SecondaryBTreeDatabaseConfig secBTDBConfig =
				    new SecondaryBTreeDatabaseConfig(
				    primaryDB, null);
				secBTDBConfig.Creation =
				    CreatePolicy.IF_NEEDED;

				SecondaryBTreeDatabase secBTDB;
				if (ifDBName == false)
					secBTDB = SecondaryBTreeDatabase.Open(
					    dbSecFileName, secBTDBConfig);
				else
					secBTDB = SecondaryBTreeDatabase.Open(
					    dbSecFileName, "secondary",
					    secBTDBConfig);

				// Close the secondary database.
				secBTDB.Close();

				// Open the existing secondary database.
				SecondaryDatabaseConfig secDBConfig =
				    new SecondaryDatabaseConfig(
				    primaryDB, null);

				SecondaryDatabase secDB;
				if (ifDBName == false)
					secDB = SecondaryBTreeDatabase.Open(
					    dbSecFileName, secDBConfig);
				else
					secDB = SecondaryBTreeDatabase.Open(
					    dbSecFileName, "secondary", secDBConfig);

				// Close secondary database.
				secDB.Close();
			}
			catch (DatabaseException)
			{
				throw new TestException();
			}
			finally
			{
				// Close primary database.
				primaryDB.Close();
			}
		}

		[Test]
		public void TestOpenWithinTxn()
		{
			testName = "TestOpenWithinTxn";
			SetUpTest(true);
			string dbFileName = testName + ".db";
			string dbSecFileName = testName + "_sec.db";

			OpenSecDBWithinTxn(testHome, dbFileName,
			    dbSecFileName, false);
		}

		[Test]
		public void TestOpenDBNameWithinTxn()
		{
			testName = "TestOpenDBNameWithinTxn";
			SetUpTest(true);
			string dbFileName = testName + ".db";
			string dbSecFileName = testName + "_sec.db";

			OpenSecDBWithinTxn(testHome, dbFileName,
			    dbSecFileName, true);
		}

		public void OpenSecDBWithinTxn(string home,
		    string dbFileName, string dbSecFileName, bool ifDbName)
		{
			// Open an environment.
			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseTxns = true;
			envConfig.UseMPool = true;
			envConfig.UseLogging = true;
			envConfig.UseLocking = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    home, envConfig);

			// Open a primary btree database.
			Transaction openDBTxn = env.BeginTransaction();
			BTreeDatabaseConfig dbConfig =
			    new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Env = env;
			BTreeDatabase db = BTreeDatabase.Open(
			    dbFileName, dbConfig, openDBTxn);

			// Open a secondary btree database.
			SecondaryBTreeDatabaseConfig secDBConfig =
			    new SecondaryBTreeDatabaseConfig(db,
			    new SecondaryKeyGenDelegate(SecondaryKeyGen));
			secDBConfig.Env = env;
			secDBConfig.Creation = CreatePolicy.IF_NEEDED;

			SecondaryBTreeDatabase secDB;
			if (ifDbName == false)
				secDB = SecondaryBTreeDatabase.Open(
				    dbSecFileName, secDBConfig, openDBTxn);
			else
				secDB = SecondaryBTreeDatabase.Open(
				    dbSecFileName, "secondary", secDBConfig,
				    openDBTxn);
			openDBTxn.Commit();
			secDB.Close();

			// Open the existing secondary database.
			Transaction secTxn = env.BeginTransaction();
			SecondaryDatabaseConfig secConfig =
			    new SecondaryDatabaseConfig(db,
			    new SecondaryKeyGenDelegate(SecondaryKeyGen));
			secConfig.Env = env;

			SecondaryDatabase secExDB;
			if (ifDbName == false)
				secExDB = SecondaryBTreeDatabase.Open(
				    dbSecFileName, secConfig, secTxn);
			else
				secExDB = SecondaryBTreeDatabase.Open(
				    dbSecFileName, "secondary", secConfig,
				    secTxn);
			secExDB.Close();
			secTxn.Commit();

			db.Close();
			env.Close();
		}

		public void OpenPrimaryAndSecondaryDB(string dbFileName,
		    out BTreeDatabase primaryDB,
		    out SecondaryBTreeDatabase secDB)
		{
			// Open primary database.
			BTreeDatabaseConfig primaryDBConfig =
			    new BTreeDatabaseConfig();
			primaryDBConfig.Creation = CreatePolicy.IF_NEEDED;
			primaryDB =
			    BTreeDatabase.Open(dbFileName, primaryDBConfig);

			// Open secondary database.
			SecondaryBTreeDatabaseConfig secDBConfig =
			    new SecondaryBTreeDatabaseConfig(primaryDB,
			    new SecondaryKeyGenDelegate(SecondaryKeyGen));
			secDB = SecondaryBTreeDatabase.Open(dbFileName,
			    secDBConfig);
		}

		[Test, ExpectedException(typeof(ExpectedTestException))]
		public void TestBadSecondaryException()
		{
			testName = "TestBadSecondaryException";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";
			string secDBFileName = testHome + "/" +
			    testName + "_sec.db";

			// Open primary database.
			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
			BTreeDatabase btreeDB =
			    BTreeDatabase.Open(dbFileName, btreeDBConfig);

			// Open secondary database.
			SecondaryBTreeDatabaseConfig secBtDbConfig =
			    new SecondaryBTreeDatabaseConfig(btreeDB,
			    new SecondaryKeyGenDelegate(SecondaryKeyGen));
			secBtDbConfig.Creation = CreatePolicy.IF_NEEDED;
			SecondaryBTreeDatabase secBtDb =
			    SecondaryBTreeDatabase.Open(secDBFileName,
			    secBtDbConfig);

			// Put some data into primary database.
			for (int i = 0; i < 10; i++)
				btreeDB.Put(new DatabaseEntry(
				    BitConverter.GetBytes(i)),
				    new DatabaseEntry(BitConverter.GetBytes(i)));

			// Close the secondary database.
			secBtDb.Close();

			// Delete record(5, 5) in primary database.
			btreeDB.Delete(new DatabaseEntry(
			    BitConverter.GetBytes((int)5)));

			// Reopen the secondary database.
			SecondaryDatabase secDB = SecondaryDatabase.Open(
			    secDBFileName,
			    new SecondaryDatabaseConfig(btreeDB,
			    new SecondaryKeyGenDelegate(SecondaryKeyGen)));

			/*
			 * Getting record(5, 5) by secondary database should 
			 * throw BadSecondaryException since it has been
			 * deleted in the primary database.
			 */
			try
			{
				secDB.Exists(new DatabaseEntry(
				    BitConverter.GetBytes((int)5)));
			}
			catch (BadSecondaryException)
			{
				throw new ExpectedTestException();
			}
			finally
			{
				secDB.Close();
				btreeDB.Close();
			}
		}

		public DatabaseEntry SecondaryKeyGen(
		    DatabaseEntry key, DatabaseEntry data)
		{
			DatabaseEntry dbtGen;
			dbtGen = new DatabaseEntry(data.Data);
			return dbtGen;
		}

	}

}