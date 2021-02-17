/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
	[TestFixture]
	public class QueueDatabaseTest : DatabaseTest
	{

		[TestFixtureSetUp]
		public void SetUpTestFixture()
		{
			testFixtureName = "QueueDatabaseTest";
			base.SetUpTestfixture();
		}

		[Test]
		public void TestAppendWithoutTxn()
		{
			testName = "TestAppendWithoutTxn";
			SetUpTest(true);
			string queueDBFileName = testHome + "/" + testName + ".db";

			
			QueueDatabaseConfig queueConfig = new QueueDatabaseConfig();
			queueConfig.Creation = CreatePolicy.ALWAYS;
			queueConfig.Length = 1000;
			QueueDatabase queueDB = QueueDatabase.Open(
			    queueDBFileName, queueConfig);

			byte[] byteArr = new byte[4];
			byteArr = BitConverter.GetBytes((int)1);
			DatabaseEntry data = new DatabaseEntry(byteArr);
			uint recno = queueDB.Append(data);

			// Confirm that the recno is larger than 0.
			Assert.AreNotEqual(0, recno);

			// Confirm that the record exists in the database.
			byteArr = BitConverter.GetBytes(recno);
			DatabaseEntry key = new DatabaseEntry();
			key.Data = byteArr;
			Assert.IsTrue(queueDB.Exists(key));
			queueDB.Close();
		}

		[Test]
		public void TestAppendWithTxn()
		{
			testName = "TestAppendWithTxn";
			SetUpTest(true);
			string queueDBFileName = testHome + "/" + testName + ".db";
			string queueDBName =
			    Path.GetFileNameWithoutExtension(queueDBFileName);

			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseTxns = true;
			envConfig.UseMPool = true;

			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);
			Transaction txn = env.BeginTransaction();

			QueueDatabaseConfig queueConfig = new QueueDatabaseConfig();
			queueConfig.Creation = CreatePolicy.ALWAYS;
			queueConfig.Env = env;
			queueConfig.Length = 1000;

			/* If environmnet home is set, the file name in
			 * Open() is the relative path.
			 */
			QueueDatabase queueDB = QueueDatabase.Open(
			    queueDBName, queueConfig, txn);
			DatabaseEntry data;
			int i = 1000;
			try
			{
				while (i > 0)
				{
					data = new DatabaseEntry(
					    BitConverter.GetBytes(i));
					queueDB.Append(data, txn);
					i--;
				}
				txn.Commit();
			}
			catch
			{
				txn.Abort();
			}
			finally
			{
				queueDB.Close();
				env.Close();
			}

		}

		[Test, ExpectedException(typeof(ExpectedTestException))]
		public void TestConsumeWithTxn()
		{
			testName = "TestConsumeWithTxn";
			SetUpTest(true);
			string queueDBFileName = testHome + "/" + testName + ".db";
			string queueDBName = Path.GetFileName(queueDBFileName);

			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseTxns = true;
			envConfig.UseMPool = true;

			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    testHome, envConfig);
			Transaction txn = env.BeginTransaction();

			QueueDatabaseConfig queueConfig =
			    new QueueDatabaseConfig();
			queueConfig.Creation = CreatePolicy.ALWAYS;
			queueConfig.Env = env;
			queueConfig.Length = 1000;
			QueueDatabase queueDB = QueueDatabase.Open(
			    queueDBName, queueConfig, txn);

			int i = 1;
			DatabaseEntry data;
			DatabaseEntry getData = new DatabaseEntry();
			while (i <= 10)
			{
				data = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes(i.ToString()));
				queueDB.Append(data, txn);
				if (i == 5)
				{
					getData = data;
				}
				i++;
			}

			KeyValuePair<uint, DatabaseEntry> pair = queueDB.Consume(false, txn);

			queueDB.Close();
			txn.Commit();
			env.Close();

			Database db = Database.Open(queueDBFileName,
			    new QueueDatabaseConfig());
			try
			{
				DatabaseEntry key =
					new DatabaseEntry(BitConverter.GetBytes(pair.Key));
				db.Get(key);
			}
			catch (NotFoundException)
			{
				throw new ExpectedTestException();
			}
			finally
			{
				db.Close();
			}
		}

		[Test, ExpectedException(typeof(ExpectedTestException))]
		public void TestConsumeWithoutTxn()
		{
			testName = "TestConsumeWithoutTxn";
			SetUpTest(true);
			string queueDBFileName = testHome + "/" +
			    testName + ".db";

			QueueDatabaseConfig queueConfig =
			    new QueueDatabaseConfig();
			queueConfig.Creation = CreatePolicy.ALWAYS;
			queueConfig.ErrorPrefix = testName;
			queueConfig.Length = 1000;

			QueueDatabase queueDB = QueueDatabase.Open(
			    queueDBFileName, queueConfig);
			DatabaseEntry data = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("data"));
			queueDB.Append(data);

			DatabaseEntry consumeData = new DatabaseEntry();
			KeyValuePair<uint, DatabaseEntry> pair = queueDB.Consume(false);
			try
			{
				DatabaseEntry key =
					new DatabaseEntry(BitConverter.GetBytes(pair.Key));
				queueDB.Get(key);
			}
			catch (NotFoundException)
			{
				throw new ExpectedTestException();
			}
			finally
			{
				queueDB.Close();
			}
		}

		public void TestCursor()
		{
			testName = "TestCursor";
			SetUpTest(true);

			GetCursur(testHome + "/" + testName + ".db", false);
		}

		public void TestCursorWithConfig()
		{
			testName = "TestCursorWithConfig";
			SetUpTest(true);

			GetCursur(testHome + "/" + testName + ".db", true);
		}

		public void GetCursur(string dbFileName, bool ifConfig)
		{
			QueueDatabaseConfig dbConfig = new QueueDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Length = 100;
			QueueDatabase db = QueueDatabase.Open(dbFileName, dbConfig);
			Cursor cursor;
			CursorConfig cursorConfig = new CursorConfig();
			cursorConfig.Priority = CachePriority.HIGH;
			if (ifConfig == false)
				cursor = db.Cursor();
			else
				cursor = db.Cursor(cursorConfig);
			cursor.Add(new KeyValuePair<DatabaseEntry,DatabaseEntry>(
			    new DatabaseEntry(BitConverter.GetBytes((int)1)),
			    new DatabaseEntry(BitConverter.GetBytes((int)1)))); 
			Cursor dupCursor = cursor.Duplicate(false);
			Assert.IsNull(dupCursor.Current.Key);
			Assert.AreEqual(CachePriority.HIGH, dupCursor.Priority);
			dupCursor.Close();
			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestDeleteMultiple()
		{
			testName = "TestDeleteMultiple";
			SetUpTest(true);

			QueueDatabaseConfig dbConfig = new QueueDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.ExtentSize = 1024;
			dbConfig.Length = 270;
			QueueDatabase db = QueueDatabase.Open(testHome + "/" +
			    testName + ".db", dbConfig);

			List<uint> rList = new List<uint>();
			List<DatabaseEntry> kList = new List<DatabaseEntry>();
			List<DatabaseEntry> vList = new List<DatabaseEntry>();
			DatabaseEntry key, data;

			for (uint i = 1; i <= 100; i++) {
				key = new DatabaseEntry(BitConverter.GetBytes(i));
				data = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes(
				    "data" + i.ToString() + 
				    Configuration.RandomString(256)));
				rList.Add(i);
				kList.Add(key);
				vList.Add(data);
				db.Put(key, data);
			}

			// Bulk delete all records with recno in rList. 
			db.Delete(new MultipleDatabaseEntry(rList));
			Cursor cursor = db.Cursor();
			Assert.IsFalse(cursor.MoveFirst());

			/*
			 * Bulk insert records whose key bulk buffer is
			 * constructed by recno lists, then delete all.
			 */ 			 
			db.Put(new MultipleDatabaseEntry(rList),
			    new MultipleDatabaseEntry(vList, false));
			Assert.IsTrue(cursor.MoveFirst());
			db.Delete(new MultipleDatabaseEntry(kList, true));
			Assert.IsFalse(cursor.MoveFirst());

			/*
			 * Bulk insert records whose key bulk buffer is
			 * constructed by DatabaseEntry lists, then delete all.
			 */ 			 
			db.Put(new MultipleDatabaseEntry(kList, true), 
			    new MultipleDatabaseEntry(vList, false));
			Assert.IsTrue(cursor.MoveFirst());
			db.Delete(new MultipleDatabaseEntry(kList, true));
			Assert.IsFalse(cursor.MoveFirst());

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestDeleteMultipleKey()
		{
			testName = "TestDeleteMultipleKey";
			SetUpTest(true);

			QueueDatabaseConfig dbConfig = new QueueDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.ExtentSize = 1024;
			dbConfig.Length = 520;
			QueueDatabase db = QueueDatabase.Open(testHome + "/" +
			    testName + ".db", dbConfig);

			List<KeyValuePair<DatabaseEntry, DatabaseEntry>> pList =
			    new List<KeyValuePair<DatabaseEntry, DatabaseEntry>>();
			DatabaseEntry key, data;

			for (uint i = 1; i <= 100; i++)
			{
				key = new DatabaseEntry(
				    BitConverter.GetBytes(i));
				data = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes(
				    "data" + i.ToString() +
				    Configuration.RandomString(512)));
				pList.Add(new KeyValuePair<
				    DatabaseEntry, DatabaseEntry>(key, data));
				db.Put(key, data);
			}

			// Create key/value pair bulk buffer and delete all.
			db.Delete(new MultipleKeyDatabaseEntry(pList, true));
			// Verify that the database is empty.
			Assert.AreEqual(0, db.Truncate());
			db.Close();
		}

		[Test, ExpectedException(typeof(ExpectedTestException))]
		public void TestKeyEmptyException()
		{
			testName = "TestKeyEmptyException";
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

			QueueDatabase db;
			try
			{
				Transaction openTxn = env.BeginTransaction();
				try
				{
					QueueDatabaseConfig queueConfig =
					    new QueueDatabaseConfig();
					queueConfig.Creation = CreatePolicy.IF_NEEDED;
					queueConfig.Length = 10;
					queueConfig.Env = env;
					db = QueueDatabase.Open(testName + ".db",
					    queueConfig, openTxn);
					openTxn.Commit();
				}
				catch (DatabaseException e)
				{
					openTxn.Abort();
					throw e;
				}

				Transaction cursorTxn = env.BeginTransaction();
				Cursor cursor;
				try
				{
					/*
					 * Put a record into queue database with 
					 * cursor and abort the operation.
					 */
					cursor = db.Cursor(cursorTxn);
					KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
					pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
					    new DatabaseEntry(BitConverter.GetBytes((int)10)),
					    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
					cursor.Add(pair);
					cursor.Close();
					cursorTxn.Abort();
				}
				catch (DatabaseException e)
				{
					cursorTxn.Abort();
					db.Close();
					throw e;
				}

				Transaction delTxn = env.BeginTransaction();
				try
				{
					/*
					 * The put operation is aborted in the queue 
					 * database so querying if the record still exists 
					 * throws KeyEmptyException.
					 */
					db.Exists(new DatabaseEntry(
					    BitConverter.GetBytes((int)10)), delTxn);
					delTxn.Commit();
				}
				catch (DatabaseException e)
				{
					delTxn.Abort();
					throw e;
				}
				finally
				{
					db.Close();
				}
			}
			catch (KeyEmptyException)
			{
				throw new ExpectedTestException();
			}
			finally
			{
				env.Close();
			}
		}

		[Test]
		public void TestOpenExistingQueueDB()
		{
			testName = "TestOpenExistingQueueDB";
			SetUpTest(true);
			string queueDBFileName = testHome + "/" + testName + ".db";

			QueueDatabaseConfig queueConfig = new QueueDatabaseConfig();
			queueConfig.Creation = CreatePolicy.ALWAYS;
			QueueDatabase queueDB = QueueDatabase.Open(
			    queueDBFileName, queueConfig);
			queueDB.Close();

			DatabaseConfig dbConfig = new DatabaseConfig();
			Database db = Database.Open(queueDBFileName, dbConfig);
			Assert.AreEqual(db.Type, DatabaseType.QUEUE);
			db.Close();
		}

		[Test]
		public void TestOpenNewQueueDB()
		{
			testName = "TestOpenNewQueueDB";
			SetUpTest(true);
			string queueDBFileName = testHome + "/" + testName + ".db";

			// Configure all fields/properties in queue database.
			XmlElement xmlElem = Configuration.TestSetUp(
			    testFixtureName, testName);
			QueueDatabaseConfig queueConfig = new QueueDatabaseConfig();
			QueueDatabaseConfigTest.Config(xmlElem, ref queueConfig, true);
			queueConfig.Feedback = new DatabaseFeedbackDelegate(DbFeedback);

			// Open the queue database with above configuration.
			QueueDatabase queueDB = QueueDatabase.Open(
			    queueDBFileName, queueConfig);

			// Check the fields/properties in opened queue database.
			Confirm(xmlElem, queueDB, true);

			queueDB.Close();
		}

		private void DbFeedback(DatabaseFeedbackEvent opcode, int percent)
		{
			if (opcode == DatabaseFeedbackEvent.UPGRADE)
				Console.WriteLine("Update for %d%", percent);

			if (opcode == DatabaseFeedbackEvent.VERIFY)
				Console.WriteLine("Vertify for %d", percent);
		}

		[Test, ExpectedException(typeof(NotFoundException))]
		public void TestPutToQueue()
		{
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			testName = "TestPutQueue";
			SetUpTest(true);
			string queueDBFileName = testHome + "/" +
			    testName + ".db";

			QueueDatabaseConfig queueConfig =
			    new QueueDatabaseConfig();
			queueConfig.Length = 512;
			queueConfig.Creation = CreatePolicy.ALWAYS;
			using (QueueDatabase queueDB = QueueDatabase.Open(
			    queueDBFileName, queueConfig))
			{
				DatabaseEntry key = new DatabaseEntry();
				key.Data = BitConverter.GetBytes((int)100);
				DatabaseEntry data = new DatabaseEntry(
				    BitConverter.GetBytes((int)1));
				queueDB.Put(key, data);
				pair = queueDB.GetBoth(key, data);
			}
		}

		[Test]
		public void TestPutMultiple()
		{
			testName = "TestPutMultiple";
			SetUpTest(true);

			QueueDatabaseConfig dbConfig = new QueueDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.ExtentSize = 1024;
			dbConfig.Length = 520;
			dbConfig.PadByte = 0;
			QueueDatabase db = QueueDatabase.Open(testHome + "/" +
			    testName + ".db", dbConfig);

			List<uint> kList = new List<uint>();
			List<DatabaseEntry> vList = new List<DatabaseEntry>();
			DatabaseEntry key, data;
			for (uint i = 1; i <= 9;i++) {
				key = new DatabaseEntry(BitConverter.GetBytes(i));
				data = new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes("data" + i +
				    Configuration.RandomString(512)));
				kList.Add(i);
				vList.Add(data);
			}

			// Create bulk buffer for recno based keys.
			MultipleDatabaseEntry kBuff = 
			    new MultipleDatabaseEntry(kList);
			Assert.IsTrue(kBuff.Recno);
			int val = 0;
			foreach (DatabaseEntry dbt in kBuff) {
				Assert.AreEqual(
				    BitConverter.GetBytes(kList[val]),
				    dbt.Data);
				val++;
			}
			Assert.AreEqual(9, val);

			// Create bulk buffer for data.
			MultipleDatabaseEntry vBuff =
			    new MultipleDatabaseEntry(vList, false);

			/*
			 * Create recno bulk buffer from another recno bulk
			 * buffer.
			 */ 			 
			MultipleDatabaseEntry kBuff1 =
			    new MultipleDatabaseEntry(kBuff.Data, kBuff.Recno);
			val = 0;
			foreach (DatabaseEntry dbt in kBuff1) {
				Assert.AreEqual(
				    BitConverter.GetBytes(kList[val]),
				    dbt.Data);
				val++;
			}
			Assert.AreEqual(9, val);

			// Bulk insert to database with key and value buffers.
			db.Put(kBuff, vBuff);
			Cursor cursor = db.Cursor();
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			val = 0;
			while (cursor.MoveNext()) {
				pair = cursor.Current;
				Assert.AreEqual(
				    BitConverter.GetBytes(kList[val]),
				    pair.Key.Data);
				for (int i = 0; i < 520; i++) {
					if (i < vList[val].Data.Length)
						Assert.AreEqual(vList[val].Data[i],
						    pair.Value.Data[i]);
					else
						// The pad byte is 0.
						Assert.AreEqual(0, pair.Value.Data[i]);
				}
				Assert.IsFalse(cursor.MoveNextDuplicate());
				val++;
			}
			Assert.AreEqual(9, val);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestStats()
		{
			testName = "TestStats";
			SetUpTest(true);
			string dbFileName = testHome + "/" +
			    testName + ".db";

			QueueDatabaseConfig dbConfig =
			    new QueueDatabaseConfig();
			ConfigCase1(dbConfig);
			QueueDatabase db = QueueDatabase.Open(dbFileName, dbConfig);

			QueueStats stats = db.Stats();
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

		public void StatsInTxn(string home, string name, bool ifIsolation)
		{
			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			EnvConfigCase1(envConfig);
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    home, envConfig);

			Transaction openTxn = env.BeginTransaction();
			QueueDatabaseConfig dbConfig =
			    new QueueDatabaseConfig();
			ConfigCase1(dbConfig);
			dbConfig.Env = env;
			QueueDatabase db = QueueDatabase.Open(name + ".db",
			    dbConfig, openTxn);
			openTxn.Commit();

			Transaction statsTxn = env.BeginTransaction();
			QueueStats stats;
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

		public void EnvConfigCase1(DatabaseEnvironmentConfig cfg)
		{
			cfg.Create = true;
			cfg.UseTxns = true;
			cfg.UseMPool = true;
			cfg.UseLogging = true;
		}

		public void ConfigCase1(QueueDatabaseConfig dbConfig)
		{
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.PageSize = 4096;
			dbConfig.ExtentSize = 1024;
			dbConfig.Length = 4000;
			dbConfig.PadByte = 32;
		}

		public void PutRecordCase1(QueueDatabase db, Transaction txn)
		{
			byte[] bigArray = new byte[4000];
			for (int i = 1; i <= 100; i++)
			{
				if (txn == null)
					db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
					    new DatabaseEntry(BitConverter.GetBytes(i)));
				else
					db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
					    new DatabaseEntry(BitConverter.GetBytes(i)), txn);
			}
			DatabaseEntry key = new DatabaseEntry(BitConverter.GetBytes((int)100));
			for (int i = 100; i <= 500; i++)
			{
				if (txn == null)
					db.Put(key, new DatabaseEntry(bigArray));
				else
					db.Put(key, new DatabaseEntry(bigArray), txn);
			}
		}

		public void ConfirmStatsPart1Case1(QueueStats stats)
		{
			Assert.AreEqual(1, stats.FirstRecordNumber);
			Assert.AreNotEqual(0, stats.MagicNumber);
			Assert.AreEqual(1, stats.NextRecordNumber);
			Assert.AreEqual(4096, stats.PageSize);
			Assert.AreEqual(1024, stats.PagesPerExtent);
			Assert.AreEqual(4000, stats.RecordLength);
			Assert.AreEqual(32, stats.RecordPadByte);
			Assert.AreEqual(4, stats.Version);	
		}

		public void ConfirmStatsPart2Case1(QueueStats stats)
		{
			Assert.AreNotEqual(0, stats.DataPages);
			Assert.AreEqual(0, stats.DataPagesBytesFree);
			Assert.AreEqual(0, stats.MetadataFlags);
			Assert.AreEqual(100, stats.nData);
			Assert.AreEqual(100, stats.nKeys);
		}

		public static void Confirm(XmlElement xmlElem,
		    QueueDatabase queueDB, bool compulsory)
		{
			DatabaseTest.Confirm(xmlElem, queueDB, compulsory);

			// Confirm queue database specific field/property
			Configuration.ConfirmUint(xmlElem, "ExtentSize",
			    queueDB.ExtentSize, compulsory);
			Configuration.ConfirmBool(xmlElem, "ConsumeInOrder",
			    queueDB.InOrder, compulsory);
			Configuration.ConfirmInt(xmlElem, "PadByte",
			    queueDB.PadByte, compulsory);
			Assert.AreEqual(DatabaseType.QUEUE, queueDB.Type);
			string type = queueDB.Type.ToString();
			Assert.IsNotNull(type);
		}
	}
}
