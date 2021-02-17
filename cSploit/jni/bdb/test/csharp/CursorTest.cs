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
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
	[TestFixture]
	public class CursorTest : CSharpTestFixture
	{
		private DatabaseEnvironment paramEnv;
		private BTreeDatabase paramDB;
		private Transaction readTxn;
		private Transaction updateTxn;

		private EventWaitHandle signal;

		private delegate void CursorMoveFuncDelegate(
		    Cursor cursor, uint kOffset, uint kLen, 
		    uint dOffset, uint dLen, LockingInfo lockingInfo);
		private CursorMoveFuncDelegate cursorFunc;

		[TestFixtureSetUp]
		public void SetUpTestFixture()
		{
			testFixtureName = "CursorTest";
			base.SetUpTestfixture();
		}

		[Test]
		public void TestAdd()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestAdd";
			SetUpTest(true);

			// Open a database and a cursor.
			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);

			// Add a record and confirm that it exists.
			AddOneByCursor(db, cursor);

			// Intend to insert pair to a wrong position.
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair =
			    new KeyValuePair<DatabaseEntry, DatabaseEntry>(
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
			try {
				try {
					cursor.Add(pair, Cursor.InsertLocation.AFTER);
					throw new TestException();
				} catch (ArgumentException)
				{}

				try {
					cursor.Add(pair, Cursor.InsertLocation.BEFORE);
					throw new TestException();
				} catch (ArgumentException)
				{}
			} finally {
				cursor.Close();
				db.Close();
			}	
		}

        [Test]
        public void TestCompare() {
            BTreeDatabase db;
            BTreeCursor dbc1, dbc2;
            DatabaseEntry data, key;

            testName = "TestCompare";
            SetUpTest(true);

            // Open a database and a cursor. Then close it.
            GetCursorInBtreeDBWithoutEnv(testHome, testName,
                out db, out dbc1);
            dbc2 = db.Cursor();

            for (int i = 0; i < 10; i++) {
                key = new DatabaseEntry(BitConverter.GetBytes(i));
                data = new DatabaseEntry(BitConverter.GetBytes(i));
                db.Put(key, data);
            }
            key = new DatabaseEntry(BitConverter.GetBytes(5));
            Assert.IsTrue(dbc1.Move(key, true));
            Assert.IsTrue(dbc2.Move(key, true));
            Assert.IsTrue(dbc1.Compare(dbc2));
            Assert.IsTrue(dbc1.MoveNext());
            Assert.IsFalse(dbc1.Compare(dbc2));
            dbc1.Close();
            dbc2.Close();
            db.Close();
        }

        [Test]
        public void TestClose()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestClose";
			SetUpTest(true);

			// Open a database and a cursor. Then close it.
			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);
			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestCloseResmgr()
		{
			testName = "TestCloseResmgr";
			SetUpTest(true);

			TestCloseResmgr_int(true);
			TestCloseResmgr_int(false);
		}
		void TestCloseResmgr_int(bool havetxn)
		{
			BTreeDatabase db;
			BTreeCursor cursor, csr;
			CursorConfig cursorConfig;
			DatabaseEnvironment env;
			Transaction txn;

			DatabaseEntry key, data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			txn = null;

			cursorConfig = new CursorConfig();
			// Open an environment, a database and a cursor.
			if (havetxn)
				GetCursorInBtreeDBInTDS(testHome, testName, 
				    cursorConfig, out env, out db, 
				    out cursor, out txn);
			else
				GetCursorInBtreeDB(testHome, testName, 
				    cursorConfig, out env, out db, 
				    out cursor);

			// Add a record to db via cursor.
			key = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("key"));
			data = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("data"));
			pair = new KeyValuePair<DatabaseEntry,DatabaseEntry>
			    (key, data);
			byte []kbytes;
			byte []dbytes;

			for (int i = 0; i < 100; i++) {
				kbytes = ASCIIEncoding.ASCII.
				    GetBytes("key" + i);
				dbytes = ASCIIEncoding.ASCII.
				    GetBytes("data" + i);
				key.Data = kbytes;
				
				data.Data = dbytes;
				
				cursor.Add(pair);
			}
			// Do not close cursor.

			csr = db.Cursor(cursorConfig, txn);

			while (csr.MoveNext()) {
				// Do nothing for now.
			}
			// Do not close csr.
			if (havetxn && txn != null)
				txn.Commit();
			env.CloseForceSync();
			
		}

		[Test]
		public void TestCount()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestCount";
			SetUpTest(true);

			// Write one record into database with cursor.
			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);
			AddOneByCursor(db, cursor);

			/*
			 * Confirm that that the count operation returns 1 as
			 * the number of records in the database.
			 */
			Assert.AreEqual(1, cursor.Count());
			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestCurrent()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestCurrent";
			SetUpTest(true);

			// Write a record into database with cursor.
			GetCursorInBtreeDBWithoutEnv(testHome,
			    testName, out db, out cursor);
			AddOneByCursor(db, cursor);

			/*
			 * Confirm the current record that the cursor
			 * points to is the one that just added by the
			 * cursor.
			 */
			Assert.IsTrue(cursor.MoveFirst());
			Assert.AreEqual(
			    ASCIIEncoding.ASCII.GetBytes("key"),
			    cursor.Current.Key.Data);
			Assert.AreEqual(
			    ASCIIEncoding.ASCII.GetBytes("data"),
			    cursor.Current.Value.Data);
			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestDelete()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestDelete";
			SetUpTest(true);

			// Write a record into database with cursor.
			GetCursorInBtreeDBWithoutEnv(testHome,
			    testName, out db, out cursor);
			AddOneByCursor(db, cursor);

			// Delete the current record.
			cursor.Delete();

			// Confirm that the record no longer exists in the db.
			Assert.AreEqual(0, cursor.Count());

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestDispose()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestDispose";
			SetUpTest(true);

			// Write a record into database with cursor.
			GetCursorInBtreeDBWithoutEnv(testHome,
			    testName, out db, out cursor);

			// Dispose the cursor.
			cursor.Dispose();

			db.Close();
		}

		[Test]
		public void TestDuplicate()
		{
			DatabaseEnvironment env;
			BTreeDatabase db;
			BTreeCursor cursor;
			Transaction txn;
			testName = "TestDuplicate";
			SetUpTest(true);

			GetCursorInBtreeDBInTDS(testHome, testName,
			    new CursorConfig(), out env, out db, out cursor,
			    out txn);
			for (int i = 0; i < 10; i++)
				cursor.Add(new KeyValuePair<
				    DatabaseEntry, DatabaseEntry>(
				    new DatabaseEntry(BitConverter.GetBytes(i)),
				    new DatabaseEntry(BitConverter.GetBytes(i))));
			// Duplicate the cursor and keep its position.
			BTreeCursor newCursor = cursor.Duplicate(true);
			Assert.AreEqual(newCursor.Current.Key, 
			    cursor.Current.Key);
			Assert.AreEqual(newCursor.Current.Key,
			    cursor.Current.Key);
			cursor.Close();
			newCursor.Close();
			db.Close();
			txn.Commit();
			env.Close();
		}

		[Test]
		public void TestIsolationDegree()
		{
			BTreeDatabase db;
			BTreeCursor cursor;
			CursorConfig cursorConfig;
			DatabaseEnvironment env;
			Transaction txn;

			testName = "TestIsolationDegree";
			SetUpTest(true);

			Isolation[] isolationDegrees = new Isolation[3];
			isolationDegrees[0] = Isolation.DEGREE_ONE;
			isolationDegrees[1] = Isolation.DEGREE_TWO;
			isolationDegrees[2] = Isolation.DEGREE_THREE;

			IsolationDelegate[] delegates = {
				new IsolationDelegate(CursorReadUncommited),
				new IsolationDelegate(CursorReadCommited),
				new IsolationDelegate(CursorRead)};

			cursorConfig = new CursorConfig();
			for (int i = 0; i < 3; i++) {
				cursorConfig.IsolationDegree = isolationDegrees[i];
				GetCursorInBtreeDBInTDS(testHome + "/" + i.ToString(), 
				    testName, cursorConfig, out env, out db,
				    out cursor, out txn);
				cursor.Close();
				db.Close();
				txn.Commit();
				env.Close();
			}
		}

		public delegate void IsolationDelegate(
		    DatabaseEnvironment env, BTreeDatabase db,
		    Cursor cursor, Transaction txn);

		/*
		 * Configure a transactional cursor to have degree 2
		 * isolation, which permits data read by this cursor
		 * to be deleted prior to the commit of the transaction
		 * for this cursor.
		 */
		public void CursorReadCommited(
		    DatabaseEnvironment env, BTreeDatabase db,
		    Cursor cursor, Transaction txn)
		{
			Console.WriteLine("CursorReadCommited");
		}

		/*
		 * Configure a transactional cursor to have degree 1
		 * isolation. The cursor's read operations could return
		 * modified but not yet commited data.
		 */
		public void CursorReadUncommited(
		    DatabaseEnvironment env, BTreeDatabase db,
		    Cursor cursor, Transaction txn)
		{
			Console.WriteLine("CursorReadUncommited");
		}

		/*
		 * Only return committed data.
		 */
		public void CursorRead(
		    DatabaseEnvironment env, BTreeDatabase db,
		    Cursor cursor, Transaction txn)
		{
			Console.WriteLine("CursorRead");
		}

		[Test]
		public void TestMoveToExactKey()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestMoveToExactKey";
			SetUpTest(true);

			GetCursorInBtreeDBWithoutEnv(testHome, 
			   testName, out db, out cursor);

			// Add one record into database.
			DatabaseEntry key = new DatabaseEntry(
			    BitConverter.GetBytes((int)0));
			DatabaseEntry data = new DatabaseEntry(
			    BitConverter.GetBytes((int)0));
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair =
			    new KeyValuePair<DatabaseEntry,DatabaseEntry>(
			    key, data);
			cursor.Add(pair);

			// Move the cursor exactly to the specified key.
			MoveCursor(cursor, 1, 1, 2, 2, null);
			cursor.Close();
			db.Close();	
		}

		[Test]
		public void TestMoveToExactPair()
		{
			BTreeDatabase db;
			BTreeCursor cursor;
			DatabaseEntry key, data;

			testName = "TestMoveToExactPair";
			SetUpTest(true);

			GetCursorInBtreeDBWithoutEnv(testHome,
			   testName, out db, out cursor);

			// Add one record into database.
			key = new DatabaseEntry(
			    BitConverter.GetBytes((int)0));
			data = new DatabaseEntry(
			    BitConverter.GetBytes((int)0));
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair =
			    new KeyValuePair<DatabaseEntry, DatabaseEntry>(key, data);
			cursor.Add(pair);

			// Move the cursor exactly to the specified key/data pair.
			MoveCursor(cursor, 1, 1, 2, 2, null);
			cursor.Close();
			db.Close();	
			
		}

		[Test]
		public void TestMoveWithRMW()
		{
			testName = "TestMoveWithRMW";
			SetUpTest(true);

			// Use MoveCursor() as its move function.
			cursorFunc = new CursorMoveFuncDelegate(MoveCursor);

			// Move to a specified key and a key/data pair.
			MoveWithRMW(testHome, testName);
		}

		[Test]
		public void TestMoveFirst()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestMoveFirst";
			SetUpTest(true);

			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);
			AddOneByCursor(db, cursor);

			// Move to the first record.
			MoveCursorToFirst(cursor, 1, 1, 2, 2, null);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestMoveFirstWithRMW()
		{
			testName = "TestMoveFirstWithRMW";
			SetUpTest(true);

			// Use MoveCursorToFirst() as its move function.
			cursorFunc = new CursorMoveFuncDelegate(MoveCursorToFirst);

			// Read the first record with write lock.
			MoveWithRMW(testHome, testName);
		}

		[Test]
		public void TestMoveLast()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestMoveLast";
			SetUpTest(true);

			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);
			AddOneByCursor(db, cursor);

			// Move the cursor to the last record.
			MoveCursorToLast(cursor, 1, 1, 2, 2, null);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestMoveLastWithRMW()
		{
			testName = "TestMoveLastWithRMW";
			SetUpTest(true);

			// Use MoveCursorToLast() as its move function.
			cursorFunc = new CursorMoveFuncDelegate(MoveCursorToLast);

			// Read the last recod with write lock.
			MoveWithRMW(testHome, testName);
		}

		[Test]
		public void TestMoveNext()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestMoveNext";
			SetUpTest(true);

			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);
			
			// Put ten records to the database.
			for (int i = 0; i < 10; i++)
				db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
				    new DatabaseEntry(BitConverter.GetBytes(i)));

			// Move the cursor from the first record to the fifth.
			MoveCursorToNext(cursor, 1, 1, 2, 2, null);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestMoveNextWithRMW()
		{
			testName = "TestMoveLastWithRMW";
			SetUpTest(true);

			// Use MoveCursorToNext() as its move function.
			cursorFunc = new CursorMoveFuncDelegate(
			    MoveCursorToNext);

			/*
			 * Read the first record to the fifth record with 
			 * write lock.
			 */ 
			MoveWithRMW(testHome, testName);
		}

		[Test]
		public void TestMoveNextDuplicate()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestMoveNextDuplicate";
			SetUpTest(true);

			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);

			// Add ten duplicate records to the database.
			for (int i = 0; i < 10; i++)
				db.Put(new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes("key")),
				    new DatabaseEntry(
				    BitConverter.GetBytes(i)));

			/*
			 * Move the cursor from one duplicate record to
			 * another duplicate one.
			 */ 
			MoveCursorToNextDuplicate(cursor, 1, 1, 2, 2, null);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestMoveNextDuplicateWithRMW()
		{
			testName = "TestMoveNextDuplicateWithRMW";
			SetUpTest(true);

			/*
			 * Use MoveCursorToNextDuplicate() as its 
			 * move function.
			 */ 
			cursorFunc = new CursorMoveFuncDelegate(
			    MoveCursorToNextDuplicate);

			/*
			 * Read the first record to the fifth record with 
			 * write lock.
			 */
			MoveWithRMW(testHome, testName);
		}
		
		[Test]
		public void TestMoveNextUnique()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestMoveNextUnique";
			SetUpTest(true);

			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);

			// Add ten different records to the database.
			for (int i = 0; i < 10; i++)
				db.Put(new DatabaseEntry(
				     BitConverter.GetBytes(i)), 
				     new DatabaseEntry(
				     BitConverter.GetBytes(i)));

			// Move to five unique records.
			MoveCursorToNextUnique(cursor, 1, 1, 2, 2, null);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestMoveNextUniqueWithRMW()
		{
			testName = "TestMoveNextUniqueWithRMW";
			SetUpTest(true);

			/*
			 * Use MoveCursorToNextUnique() as its 
			 * move function.
			 */
			cursorFunc = new CursorMoveFuncDelegate(
			    MoveCursorToNextUnique);

			/*
			 * Move to five unique records.
			 */
			MoveWithRMW(testHome, testName);
		}

		[Test]
		public void TestMovePrev()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestMovePrev";
			SetUpTest(true);

			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);

			// Add ten records to the database.
			for (int i = 0; i < 10; i++)
				AddOneByCursor(db, cursor);

			// Move the cursor to previous five records
			MoveCursorToPrev(cursor, 1, 1, 2, 2, null);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestMovePrevWithRMW()
		{
			testName = "TestMovePrevWithRMW";
			SetUpTest(true);

			/*
			 * Use MoveCursorToNextDuplicate() as its 
			 * move function.
			 */
			cursorFunc = new CursorMoveFuncDelegate(
			    MoveCursorToPrev);

			// Read previous record in write lock.
			MoveWithRMW(testHome, testName);
		}

		[Test]
		public void TestMovePrevDuplicate()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestMovePrevDuplicate";
			SetUpTest(true);

			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);

			// Add ten records to the database.
			for (int i = 0; i < 10; i++)
				AddOneByCursor(db, cursor);

			// Move the cursor to previous duplicate records.
			MoveCursorToPrevDuplicate(cursor, 1, 1, 2, 2, null);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestMovePrevDuplicateWithRMW()
		{
			testName = "TestMovePrevDuplicateWithRMW";
			SetUpTest(true);

			/*
			 * Use MoveCursorToNextDuplicate() as its 
			 * move function.
			 */
			cursorFunc = new CursorMoveFuncDelegate(
			    MoveCursorToPrevDuplicate);

			// Read the previous duplicate record in write lock.
			MoveWithRMW(testHome, testName);
		}

		[Test]
		public void TestMovePrevUnique()
		{
			BTreeDatabase db;
			BTreeCursor cursor;

			testName = "TestMovePrevUnique";
			SetUpTest(true);

			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);

			// Add ten records to the database.
			for (int i = 0; i < 10; i++)
				db.Put(new DatabaseEntry(BitConverter.GetBytes(i)), 
				    new DatabaseEntry(BitConverter.GetBytes(i)));

			// Move the cursor to previous unique records.
			MoveCursorToPrevUnique(cursor, 1, 1, 2, 2, null);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestMovePrevUniqueWithRMW()
		{
			testName = "TestMovePrevDuplicateWithRMW";
			SetUpTest(true);

			/*
			 * Use MoveCursorToPrevUnique() as its 
			 * move function.
			 */
			cursorFunc = new CursorMoveFuncDelegate(
			    MoveCursorToPrevUnique);

			// Read the previous unique record in write lock.
			MoveWithRMW(testHome, testName);
		}

		[Test]
		public void TestPartialPut() 
		{
			testName = "TestPartialPut";
			SetUpTest(true);

			BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
			cfg.Creation = CreatePolicy.IF_NEEDED;
			BTreeDatabase db = BTreeDatabase.Open(
			    testHome + "/" + testName, cfg);
			PopulateDb(ref db);

			/*
			 * Partially replace the first byte in the current
			 * data with 'a', and verify that the first byte in 
			 * the retrieved current data is 'a'.
			 */
			BTreeCursor cursor = db.Cursor();
			byte[] bytes = new byte[1];
			bytes[0] = (byte)'a';
			DatabaseEntry data = new DatabaseEntry(bytes, 0, 1);
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			while(cursor.MoveNext()) {
				cursor.Overwrite(data);
				Assert.IsTrue(cursor.Refresh());
				pair = cursor.Current;
				Assert.AreEqual((byte)'a', pair.Value.Data[0]);
			}

			cursor.Close();
			db.Close();
		}

		private void PopulateDb(ref BTreeDatabase db) 
		{
			if (db == null) {
				BTreeDatabaseConfig cfg =
				    new BTreeDatabaseConfig();
				cfg.Creation = CreatePolicy.IF_NEEDED;
				db = BTreeDatabase.Open(
				    testHome + "/" + testName, cfg);
			}
			for (int i = 0; i < 100; i++)
				db.Put(
				    new DatabaseEntry(BitConverter.GetBytes(i)), 
				    new DatabaseEntry(BitConverter.GetBytes(i)));
		}

		[Test]
		public void TestPriority()
		{
			CachePriority[] priorities;
			CursorConfig cursorConfig;

			testName = "TestPriority";
			SetUpTest(true);
			cursorConfig = new CursorConfig();
			priorities = new CachePriority[6];
			priorities[0] = CachePriority.DEFAULT;
			priorities[1] = CachePriority.HIGH;
			priorities[2] = CachePriority.LOW;
			priorities[3] = CachePriority.VERY_HIGH;
			priorities[4] = CachePriority.VERY_LOW;
			priorities[5] = null;

			for (int i = 0; i < 6; i++) {
				if (priorities[i] != null) {
					cursorConfig.Priority = priorities[i];
					Assert.AreEqual(priorities[i], cursorConfig.Priority);
				}
				GetCursorWithConfig(testHome + "/" + testName + ".db", 
				    DatabaseType.BTREE.ToString(), cursorConfig, DatabaseType.BTREE);
				GetCursorWithConfig(testHome + "/" + testName + ".db", 
				    DatabaseType.HASH.ToString(), cursorConfig, DatabaseType.HASH);
				GetCursorWithConfig(testHome + "/" + testName + ".db", 
				    DatabaseType.QUEUE.ToString(), cursorConfig, DatabaseType.QUEUE);
				GetCursorWithConfig(testHome + "/" + testName + ".db", 
				    DatabaseType.RECNO.ToString(), cursorConfig, DatabaseType.RECNO);
			}
		}

		private void GetCursorWithConfig(string dbFile, string dbName, 
		    CursorConfig cfg, DatabaseType type)
		{
			Database db;
			Cursor cursor;
			Configuration.ClearDir(testHome);
			if (type == DatabaseType.BTREE) {
				BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
				dbConfig.Creation = CreatePolicy.IF_NEEDED;
				db = BTreeDatabase.Open(dbFile, dbName, dbConfig);
				cursor = ((BTreeDatabase)db).Cursor(cfg);
			} else if (type == DatabaseType.HASH) {
				HashDatabaseConfig dbConfig = new HashDatabaseConfig();
				dbConfig.Creation = CreatePolicy.IF_NEEDED;
				db = (HashDatabase)HashDatabase.Open(dbFile, dbName, dbConfig);
				cursor = ((HashDatabase)db).Cursor(cfg);
			} else if (type == DatabaseType.QUEUE) {
				QueueDatabaseConfig dbConfig = new QueueDatabaseConfig();
				dbConfig.Creation = CreatePolicy.IF_NEEDED;
				dbConfig.Length = 100;
				db = QueueDatabase.Open(dbFile, dbConfig);
				cursor = ((QueueDatabase)db).Cursor(cfg);
			} else if (type == DatabaseType.RECNO) {
				RecnoDatabaseConfig dbConfig = new RecnoDatabaseConfig();
				dbConfig.Creation = CreatePolicy.IF_NEEDED;
				db = RecnoDatabase.Open(dbFile, dbName, dbConfig);
				cursor = ((RecnoDatabase)db).Cursor(cfg);
			} else
				throw new TestException();

			if (cfg.Priority != null)
				Assert.AreEqual(cursor.Priority, cfg.Priority);
			else
				Assert.AreEqual(CachePriority.DEFAULT, cursor.Priority);

			Cursor dupCursor = cursor.Duplicate(false);
			Assert.AreEqual(cursor.Priority, dupCursor.Priority);
			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestRefresh()
		{
			BTreeDatabase db;
			BTreeCursor cursor;
			DatabaseEnvironment env;
			Transaction txn;

			testName = "TestRefresh";
			SetUpTest(true);

			GetCursorInBtreeDBWithoutEnv(testHome, testName,
			    out db, out cursor);
			// Write a record with cursor and Refresh the cursor.
			MoveCursorToCurrentRec(cursor, 1, 1, 2, 2, null);
			cursor.Dispose();
			db.Dispose();

			Configuration.ClearDir(testHome);
			GetCursorInBtreeDBInTDS(testHome, testName, null, out env, 
			    out db, out cursor, out txn);
			LockingInfo lockingInfo = new LockingInfo();
			lockingInfo.IsolationDegree = Isolation.DEGREE_ONE;
			MoveCursorToCurrentRec(cursor, 1, 1, 2, 2, lockingInfo);
			cursor.Close();
			txn.Commit();
			db.Close();
			env.Close();
		}

		[Test]
		public void TestRefreshWithRMW()
		{
			testName = "TestRefreshWithRMW";
			SetUpTest(true);

			cursorFunc = new CursorMoveFuncDelegate(
			    MoveCursorToCurrentRec);

			// Read the previous unique record in write lock.
			MoveWithRMW(testHome, testName);
		}

		[Test]
		public void TestSnapshotIsolation()
		{
			BTreeDatabaseConfig dbConfig;
			DatabaseEntry key, data;
			DatabaseEnvironmentConfig envConfig;
			Thread readThread, updateThread;
			Transaction txn;

			updateTxn = null;
			readTxn = null;
			paramDB = null;
			paramEnv = null;
			testName = "TestSnapshotIsolation";
			SetUpTest(true);

			/*
			 * Open environment with DB_MULTIVERSION
			 * which is required by DB_TXN_SNAPSHOT.
			 */
			envConfig = new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseMVCC = true;
			envConfig.UseTxns = true;
			envConfig.UseMPool = true;
			envConfig.UseLocking = true;
			envConfig.TxnTimeout = 1000;
			paramEnv = DatabaseEnvironment.Open(
			    testHome, envConfig);
			paramEnv.DetectDeadlocks(DeadlockPolicy.YOUNGEST);

			/*
			 * Open a transactional database and put 1000 records
			 * into it within transaction.
			 */
			txn = paramEnv.BeginTransaction();
			dbConfig = new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.UseMVCC = true;
			dbConfig.Env = paramEnv;
			paramDB = BTreeDatabase.Open(
			    testName + ".db", dbConfig, txn);
			for (int i = 0; i < 256; i++)
			{
				key = new DatabaseEntry(
				    BitConverter.GetBytes(i));
				data = new DatabaseEntry(
				    BitConverter.GetBytes(i));
				paramDB.Put(key, data, txn);
			}
			txn.Commit();

			/*
			 * Begin two threads, read and update thread.
			 * The update thread runs a update transaction
			 * using full read/write locking. The read thread
			 * set DB_TXN_SNAPSHOT on read-only cursor.
			 */
			readThread = new Thread(new ThreadStart(ReadTxn));
			updateThread = new Thread(new ThreadStart(UpdateTxn));
			updateThread.Start();
			Thread.Sleep(1000);
			readThread.Start();
			readThread.Join();
			updateThread.Join();

			// Commit transacion in both two threads.
			if (updateTxn != null)
				updateTxn.Commit();
			if (readTxn != null)
				readTxn.Commit();

			/*
			 * Confirm that the overwrite operation works.
			 */
			ConfirmOverwrite();

			paramDB.Close();
			paramEnv.Close();
		}

		public void ReadTxn()
		{
			// Get a new transaction for reading the db.
			TransactionConfig txnConfig =
			    new TransactionConfig();
			txnConfig.Snapshot = true;
			readTxn = paramEnv.BeginTransaction(
			    txnConfig);

			// Get a new cursor for putting record into db.
			CursorConfig cursorConfig = new CursorConfig();
			cursorConfig.WriteCursor = false;
			BTreeCursor cursor = paramDB.Cursor(
			    cursorConfig, readTxn);

			// Continually reading record from db.
			try
			{
				Assert.IsTrue(cursor.MoveFirst());
				int i = 0;
				do
				{
					Assert.AreEqual(
					    BitConverter.ToInt32(
					    cursor.Current.Key.Data, 0),
					    BitConverter.ToInt32(
					    cursor.Current.Value.Data, 0));
				} while (i <= 1000 && cursor.MoveNext());
			}
			catch (DeadlockException)
			{
			}
			finally
			{
				cursor.Close();
			}
		}

		public void UpdateTxn()
		{
			int int32Value;
			DatabaseEntry data;

			// Get a new transaction for updating the db.
			TransactionConfig txnConfig =
			    new TransactionConfig();
			txnConfig.IsolationDegree =
			    Isolation.DEGREE_THREE;

			updateTxn =
			    paramEnv.BeginTransaction(txnConfig);

			// Continually putting record to db.

			BTreeCursor cursor =
			    paramDB.Cursor(updateTxn);

			// Move the cursor to the first record.
			Assert.IsTrue(cursor.MoveFirst());
			int i = 0;
			try
			{
				do
				{

					int32Value = BitConverter.ToInt32(
					     cursor.Current.Value.Data, 0);
					data = new DatabaseEntry(
					     BitConverter.GetBytes(int32Value - 1));
					cursor.Overwrite(data);
				} while (i <= 1000 && cursor.MoveNext());
			}
			catch (DeadlockException)
			{
			}
			finally
			{
				cursor.Close();
			}
		}

		[Test]
		public void TestWriteCursor()
		{
			BTreeCursor cursor;
			BTreeDatabase db;
			CursorConfig cursorConfig;
			DatabaseEnvironment env;

			testName = "TestWriteCursor";
			SetUpTest(true);

			cursorConfig = new CursorConfig();
			cursorConfig.WriteCursor = true;

			GetCursorInBtreeDBInCDS(testHome, testName,
			    cursorConfig, out env, out db, out cursor);

			/*
			 * Add a record by cursor to the database. If the
			 * WriteCursor doesn't work, exception will be
			 * throwed in the environment which is configured
			 * with DB_INIT_CDB.
			 */
			try
			{
				AddOneByCursor(db, cursor);
			}
			catch (DatabaseException)
			{
				throw new TestException();
			}
			finally
			{
				cursor.Close();
				db.Close();
				env.Close();
			}
		}

		public void ConfirmOverwrite()
		{
			Transaction confirmTxn = paramEnv.BeginTransaction();
			BTreeCursor cursor = paramDB.Cursor(confirmTxn);

			int i = 0;
			Assert.IsTrue(cursor.MoveFirst());
			do
			{
				Assert.AreNotEqual(
				    cursor.Current.Key.Data, 
				    cursor.Current.Value.Data);
			} while (i <= 1000 && cursor.MoveNext());

			cursor.Close();
			confirmTxn.Commit();
		}

		public static void AddOneByCursor(Database db, Cursor cursor)
		{
			DatabaseEntry key, data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			// Add a record to db via cursor.
			key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key"));
			data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data"));
			pair = new KeyValuePair<DatabaseEntry,DatabaseEntry>(key, data);
			cursor.Add(pair);

			// Confirm that the record has been put to the database.
			Assert.IsTrue(db.Exists(key));
		}

		public static void GetCursorInBtreeDBWithoutEnv(
		    string home, string name, out BTreeDatabase db,
		    out BTreeCursor cursor)
		{
			string dbFileName = home + "/" + name + ".db";

			BTreeDatabaseConfig dbConfig =
			    new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Duplicates = DuplicatesPolicy.UNSORTED;
			db = BTreeDatabase.Open(dbFileName, dbConfig);
			cursor = db.Cursor();
		}

		public static void GetCursorInBtreeDBInTDS(
		    string home, string name,
		    CursorConfig cursorConfig,
		    out DatabaseEnvironment env, out BTreeDatabase db,
		    out BTreeCursor cursor, out Transaction txn)
		{
			string dbFileName = name + ".db";

			Configuration.ClearDir(home);

			// Open an environment.
			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseMPool = true;
			envConfig.UseTxns = true;
			envConfig.NoMMap = false;
			envConfig.UseLocking = true;
			env = DatabaseEnvironment.Open(home, envConfig);

			// Begin a transaction.
			txn = env.BeginTransaction();

			/*
			 * Open an btree database. The underlying database
			 * should be opened with ReadUncommitted if the
			 * cursor's isolation degree will be set to be 1.
			 */
			BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Env = env;
			if (cursorConfig != null && 
			    cursorConfig.IsolationDegree == Isolation.DEGREE_ONE)
				dbConfig.ReadUncommitted = true;

			db = BTreeDatabase.Open(dbFileName, dbConfig, txn);

			// Get a cursor in the transaction.
			if (cursorConfig != null)
				cursor = db.Cursor(cursorConfig, txn);
			else
				cursor = db.Cursor(txn);
		}

		public static void GetCursorInBtreeDB(
		    string home, string name,
		    CursorConfig cursorConfig,
		    out DatabaseEnvironment env, out BTreeDatabase db,
		    out BTreeCursor cursor)
		{
			string dbFileName = name + ".db";

			Configuration.ClearDir(home);

			// Open an environment.
			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseMPool = true;
			envConfig.UseTxns = true;
			envConfig.NoMMap = false;
			envConfig.UseLocking = true;
			env = DatabaseEnvironment.Open(home, envConfig);

			/*
			 * Open an btree database. The underlying database
			 * should be opened with ReadUncommitted if the
			 * cursor's isolation degree will be set to be 1.
			 */
			BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Env = env;
			if (cursorConfig.IsolationDegree == Isolation.DEGREE_ONE)
				dbConfig.ReadUncommitted = true;

			db = BTreeDatabase.Open(dbFileName, dbConfig, null);

			// Get a cursor in the transaction.
			cursor = db.Cursor(cursorConfig, null);
		}
		// Get a cursor in CDS.
		public static void GetCursorInBtreeDBInCDS(
		    string home, string name,
		    CursorConfig cursorConfig,
		    out DatabaseEnvironment env, out BTreeDatabase db,
		    out BTreeCursor cursor)
		{
			string dbFileName = name + ".db";
			Configuration.ClearDir(home);

			// Open an environment.
			DatabaseEnvironmentConfig envConfig =
				new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseCDB = true;
			envConfig.UseMPool = true;
			env = DatabaseEnvironment.Open(home, envConfig);

			/*
			 * Open an btree database. The underlying database
			 * should be opened with ReadUncommitted if the
			 * cursor's isolation degree will be set to be 1.
			 */
			BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Env = env;

			if (cursorConfig.IsolationDegree == Isolation.DEGREE_ONE)
				dbConfig.ReadUncommitted = true;

			db = BTreeDatabase.Open(dbFileName, dbConfig);

			// Get a cursor in the transaction.
			cursor = db.Cursor(cursorConfig);
		}

		public void RdMfWt()
		{
			Transaction txn = paramEnv.BeginTransaction();
			Cursor dbc = paramDB.Cursor(txn);

			try
			{
				LockingInfo lck = new LockingInfo();
				lck.ReadModifyWrite = true;

				// Read record.
				cursorFunc(dbc, 1, 1, 2, 2, lck);

				// Block the current thread until event is set.
				signal.WaitOne();

				// Write new records into database.
				DatabaseEntry key = new DatabaseEntry(
				    BitConverter.GetBytes(55));
				DatabaseEntry data = new DatabaseEntry(
				     BitConverter.GetBytes(55));
				dbc.Add(new KeyValuePair<DatabaseEntry,
				     DatabaseEntry>(key, data));

				dbc.Close();
				txn.Commit();
			}
			catch (DeadlockException)
			{
				dbc.Close();
				txn.Abort();
			}
		}

		
		public void MoveWithRMW(string home, string name)
		{
			paramEnv = null;
			paramDB = null;

			// Open the environment.
			DatabaseEnvironmentConfig envCfg =
			    new DatabaseEnvironmentConfig();
			envCfg.Create = true;
			envCfg.FreeThreaded = true;
			envCfg.UseLocking = true;
			envCfg.UseLogging = true;
			envCfg.UseMPool = true;
			envCfg.UseTxns = true;
			paramEnv = DatabaseEnvironment.Open(home, envCfg);

			// Open database in transaction.
			Transaction openTxn = paramEnv.BeginTransaction();
			BTreeDatabaseConfig cfg = new BTreeDatabaseConfig();
			cfg.Creation = CreatePolicy.ALWAYS;
			cfg.Env = paramEnv;
			cfg.FreeThreaded = true;
			cfg.PageSize = 4096;
			cfg.Duplicates = DuplicatesPolicy.UNSORTED;
			paramDB = BTreeDatabase.Open(name + ".db", cfg, openTxn);
			openTxn.Commit();

			/*
			 * Put 10 different, 2 duplicate and another different 
			 * records into database.
			 */ 
			Transaction txn = paramEnv.BeginTransaction();
			for (int i = 0; i < 14; i++)
			{
				DatabaseEntry key, data;
				if (i == 10 || i == 11 || i == 12)
				{
					key = new DatabaseEntry(
					    ASCIIEncoding.ASCII.GetBytes("key"));
					data = new DatabaseEntry(
					    BitConverter.GetBytes(i));
				}
				else
				{
					key = new DatabaseEntry(
					    BitConverter.GetBytes(i));
					data = new DatabaseEntry(
					    BitConverter.GetBytes(i));
				}
				paramDB.Put(key, data, txn);
			}
			
			txn.Commit();

			// Get a event wait handle.
			signal = new EventWaitHandle(false, EventResetMode.ManualReset);

			/*
			 * Start RdMfWt() in two threads. RdMfWt() reads 
			 * and writes data into database.
			 */
			Thread t1 = new Thread(new ThreadStart(RdMfWt));
			Thread t2 = new Thread(new ThreadStart(RdMfWt));
			t1.Start();
			t2.Start();

			/* 
			 * Give both threads time to read before signalling 
			 * them to write. 
			 */
			Thread.Sleep(1000);

			// Invoke the write operation in both threads.
			signal.Set();

			// Return the number of deadlocks.
			while (t1.IsAlive || t2.IsAlive)
			{
				/*
				 * Give both threads time to write before 
				 * counting the number of deadlocks.
				 */
				Thread.Sleep(1000);
				uint deadlocks = paramEnv.DetectDeadlocks(
				   DeadlockPolicy.DEFAULT);

				// Confirm that there won't be any deadlock. 
				Assert.AreEqual(0, deadlocks);
			}

			t1.Join();
			t2.Join();
			paramDB.Close();
			paramEnv.Close();
		}

		/*
		 * Move the cursor to an exisiting key and key/data 
		 * pair with LockingInfo.
		 */
		public void MoveCursor(Cursor dbc,
		    uint kOffset, uint kLen,
		    uint dOffset, uint dLen,
		    LockingInfo lck)
		{
			DatabaseEntry key, data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			key = new DatabaseEntry(
			    BitConverter.GetBytes((int)0));
			data = new DatabaseEntry(
			    BitConverter.GetBytes((int)0));
			pair = new KeyValuePair<DatabaseEntry,
			    DatabaseEntry>(key, data);

			if (lck == null) {
				// Move to an existing key.
				Assert.IsTrue(dbc.Move(key, true));

				// Move to an existing key/data pair.
				Assert.IsTrue(dbc.Move(pair, true));

				/* Move to an existing key, and return 
				 * partial data.
				 */
				data = new DatabaseEntry(dOffset, dLen);
				Assert.IsTrue(dbc.Move(key, data, true));
				CheckPartial(null, 0, 0, 
				    dbc.Current.Value, dOffset, dLen);

				// Partially key match.
				byte[] partbytes = new byte[kLen];
				for (int i = 0; i < kLen; i++)
				partbytes[i] = BitConverter.GetBytes(
				    (int)1)[kOffset + i];
				key = new DatabaseEntry(partbytes, kOffset, kLen);
				Assert.IsTrue(dbc.Move(key, false));
				Assert.AreEqual(kLen, dbc.Current.Key.Data.Length);
				CheckPartial(dbc.Current.Key, kOffset, kLen,
				    null, 0, 0);
			} else {
				// Move to an existing key.
				Assert.IsTrue(dbc.Move(key, true, lck));

				// Move to an existing key/data pair.
				Assert.IsTrue(dbc.Move(pair, true, lck));

				/*
				 * Move to an existing key, and return
				 * partial data.
				 */
				data = new DatabaseEntry(dOffset, dLen);
				Assert.IsTrue(dbc.Move(key, data, true, lck));
				CheckPartial(null, 0, 0,
				    dbc.Current.Value, dOffset, dLen);

				// Partially key match.
				byte[] partbytes = new byte[kLen];
				for (int i = 0; i < kLen; i++)
					partbytes[i] = BitConverter.GetBytes(
					    (int)1)[kOffset + i];
				key = new DatabaseEntry(partbytes, kOffset, kLen);
				Assert.IsTrue(dbc.Move(key, false, lck));
				Assert.AreEqual(kLen, dbc.Current.Key.Data.Length);
				CheckPartial(dbc.Current.Key, kOffset, kLen,
				    null, 0, 0);
			}
		}

		/*
		 * Move the cursor to the first record in a nonempty 
		 * database. The returning value should be true.
		 */
		public void MoveCursorToFirst(Cursor dbc, 
		    uint kOffset, uint kLen,
		    uint dOffset, uint dLen, LockingInfo lck)
		{
			DatabaseEntry key = new DatabaseEntry(kOffset, kLen);
			DatabaseEntry data = new DatabaseEntry(dOffset, dLen);
			if (lck == null) {
				Assert.IsTrue(dbc.MoveFirst());
				Assert.IsTrue(dbc.MoveFirst(key, data));
			} else {
				Assert.IsTrue(dbc.MoveFirst(lck));
				Assert.IsTrue(dbc.MoveFirst(key, data, lck));
			}

			CheckPartial(dbc.Current.Key, kOffset, kLen,
			    dbc.Current.Value, dOffset, dLen);
		}

		/*
		 * Move the cursor to last record in a nonempty 
		 * database. The returning value should be true.
		 */
		public void MoveCursorToLast(Cursor dbc,
		    uint kOffset, uint kLen,
		    uint dOffset, uint dLen, LockingInfo lck)
		{
			DatabaseEntry key = new DatabaseEntry(kOffset, kLen);
			DatabaseEntry data = new DatabaseEntry(dOffset, dLen);
			if (lck == null) {
				Assert.IsTrue(dbc.MoveLast());
				Assert.IsTrue(dbc.MoveLast(key, data));
			} else {
				Assert.IsTrue(dbc.MoveLast(lck));
				Assert.IsTrue(dbc.MoveLast(key, data, lck));
			}
			CheckPartial(dbc.Current.Key, kOffset, kLen,
			    dbc.Current.Value, dOffset, dLen);
		}

		/*
		 * Move the cursor to the next record in the database 
		 * with more than five records. The returning values of 
		 * every move operation should be true.
		 */ 
		public void MoveCursorToNext(Cursor dbc,
		    uint kOffset, uint kLen,
		    uint dOffset, uint dLen, LockingInfo lck)
		{
			DatabaseEntry key = new DatabaseEntry(kOffset, kLen);
			DatabaseEntry data = new DatabaseEntry(dOffset, dLen);
			for (int i = 0; i < 5; i++) {
				if (lck == null) {
					Assert.IsTrue(dbc.MoveNext());
					Assert.IsTrue(dbc.MoveNext(key, data));
				} else {
					Assert.IsTrue(dbc.MoveNext(lck));
					Assert.IsTrue(
					    dbc.MoveNext(key, data, lck));
				}
				CheckPartial(dbc.Current.Key, kOffset, kLen,
				    dbc.Current.Value, dOffset, dLen);
			}
		}

		/*
		 * Move the cursor to the next duplicate record in 
		 * the database which has more than 2 duplicate 
		 * records. The returning value should be true.
		 */ 
		public void MoveCursorToNextDuplicate(Cursor dbc,
		    uint kOffset, uint kLen,
		    uint dOffset, uint dLen, LockingInfo lck)
		{
			DatabaseEntry key = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("key"));

			/*
			 * The cursor should point to any record in the 
			 * database before it move to the next duplicate 
			 * record.
			 */
			if (lck == null) {
				dbc.Move(key, true);
				Assert.IsTrue(dbc.MoveNextDuplicate());
				Assert.IsTrue(dbc.MoveNextDuplicate(
				    new DatabaseEntry(kOffset, kLen),
				    new DatabaseEntry(dOffset, dLen)));
			} else {
				/*
				 * Both the move and move next duplicate 
				 * operation should use LockingInfo. If any 
				 * one doesn't use LockingInfo, deadlock still
				 * occurs.
				 */ 
				dbc.Move(key, true, lck);
				Assert.IsTrue(dbc.MoveNextDuplicate(lck));
				Assert.IsTrue(dbc.MoveNextDuplicate(
				    new DatabaseEntry(kOffset, kLen),
				    new DatabaseEntry(dOffset, dLen), lck));
			}
		}

		/*
		 * Move the cursor to next unique record in the database.
		 * The returning value should be true.
		 */
		public void MoveCursorToNextUnique(Cursor dbc,
		uint kOffset, uint kLen,
		uint dOffset, uint dLen, LockingInfo lck)
		{
			for (int i = 0; i < 5; i++) {
				if (lck == null) {
					Assert.IsTrue(dbc.MoveNextUnique());
					Assert.IsTrue(dbc.MoveNextUnique(
					    new DatabaseEntry(kOffset, kLen),
					    new DatabaseEntry(dOffset, dLen)));
				} else {
					Assert.IsTrue(dbc.MoveNextUnique(lck));
					Assert.IsTrue(dbc.MoveNextUnique(
					    new DatabaseEntry(kOffset, kLen),
					    new DatabaseEntry(dOffset, dLen),
					    lck));
				}
				CheckPartial(dbc.Current.Key, kOffset, kLen,
				    dbc.Current.Value, dOffset, dLen);
			}
		}

		/*
		 * Move the cursor to previous record in the database.
		 * The returning value should be true;
		 */ 
		public void MoveCursorToPrev(Cursor dbc,
		uint kOffset, uint kLen,
		uint dOffset, uint dLen, LockingInfo lck)
		{
			if (lck == null) {
				dbc.MoveLast();
				for (int i = 0; i < 5; i++) {
					Assert.IsTrue(dbc.MovePrev());
					dbc.MoveNext();
					Assert.IsTrue(dbc.MovePrev(
					    new DatabaseEntry(kOffset, kLen),
					    new DatabaseEntry(dOffset, dLen)));
					CheckPartial(
					    dbc.Current.Key, kOffset, kLen,
					    dbc.Current.Value, dOffset, dLen);
				}
			} else {
				dbc.MoveLast(lck);
				for (int i = 0; i < 5; i++) {
					Assert.IsTrue(dbc.MovePrev(lck));
					dbc.MoveNext();
					Assert.IsTrue(dbc.MovePrev(
					    new DatabaseEntry(kOffset, kLen),
					    new DatabaseEntry(dOffset, dLen),
					    lck));
					CheckPartial(
					    dbc.Current.Key, kOffset, kLen,
					    dbc.Current.Value, dOffset, dLen);
				}
			}
		}

		/*
		 * Move the cursor to a duplicate record and then to 
		 * another duplicate one. And finally move to it previous 
		 * one. Since the previous duplicate one exist, the return
		 * value of move previous duplicate record should be 
		 * true;
		 */ 
		public void MoveCursorToPrevDuplicate(Cursor dbc,
		uint kOffset, uint kLen,
		uint dOffset, uint dLen, LockingInfo lck)
		{
			if (lck == null) {
				dbc.Move(new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes("key")), true);
				dbc.MoveNextDuplicate();
				Assert.IsTrue(dbc.MovePrevDuplicate());

				dbc.Move(new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes("key")), true);
				dbc.MoveNextDuplicate();
				Assert.IsTrue(dbc.MovePrevDuplicate(
				    new DatabaseEntry(kOffset, kLen),
				    new DatabaseEntry(dOffset, dLen)));
			} else {
				dbc.Move(new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes("key")),
				    true, lck);
				dbc.MoveNextDuplicate(lck);
				Assert.IsTrue(dbc.MovePrevDuplicate(lck));

				dbc.Move(new DatabaseEntry(
				    ASCIIEncoding.ASCII.GetBytes("key")), 
				    true, lck);
				dbc.MoveNextDuplicate(lck);
				Assert.IsTrue(dbc.MovePrevDuplicate(
				    new DatabaseEntry(kOffset, kLen),
				    new DatabaseEntry(dOffset, dLen), lck));
			}

		CheckPartial(dbc.Current.Key, kOffset, kLen,
		    dbc.Current.Value, dOffset, dLen);
		}

		/*
		 * Move the cursor to previous unique record in a 
		 * database with more than 2 records. The returning
		 * value should be true.
		 */ 
		public void MoveCursorToPrevUnique(Cursor dbc,
		uint kOffset, uint kLen,
		uint dOffset, uint dLen, LockingInfo lck)
		{
			DatabaseEntry key = new DatabaseEntry(kOffset, kLen);
			DatabaseEntry data = new DatabaseEntry(dOffset, dLen);
			for (int i = 0; i < 5; i++) {
				if (lck == null) {
					Assert.IsTrue(dbc.MovePrevUnique());
					Assert.IsTrue(
					    dbc.MovePrevUnique(key, data));
				} else {
					Assert.IsTrue(dbc.MovePrevUnique(lck));
					Assert.IsTrue(
					    dbc.MovePrevUnique(key, data, lck));
				}
				CheckPartial(dbc.Current.Key, kOffset, kLen,
				    dbc.Current.Value, dOffset, dLen);
			}
		}

		/*
		 * Move the cursor to current existing record. The returning 
		 * value should be true.
		 */ 
		public void MoveCursorToCurrentRec(Cursor dbc,
		    uint kOffset, uint kLen,
		    uint dOffset, uint dLen, LockingInfo lck)
		{
			DatabaseEntry key, data;

			// Add a record to the database.
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			key = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("key"));
			data = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("data"));
			pair = new KeyValuePair<DatabaseEntry,
			    DatabaseEntry>(key, data);
			dbc.Add(pair);

			if (lck == null)
				Assert.IsTrue(dbc.Refresh());
			else
			Assert.IsTrue(dbc.Refresh(lck));
			Assert.AreEqual(key.Data, dbc.Current.Key.Data);
			Assert.AreEqual(data.Data, dbc.Current.Value.Data);

			key = new DatabaseEntry(kOffset, kLen);
			data = new DatabaseEntry(dOffset, dLen);
			if (lck == null)
				Assert.IsTrue(dbc.Refresh(key, data));
			else
				Assert.IsTrue(dbc.Refresh(key, data, lck));
			CheckPartial(dbc.Current.Key, kOffset, kLen,
			    dbc.Current.Value, dOffset, dLen);
		}

		public void CheckPartial(
		    DatabaseEntry key, uint kOffset, uint kLen,
		    DatabaseEntry data, uint dOffset, uint dLen) 
		{
			if (key != null) {
			    Assert.IsTrue(key.Partial);
			    Assert.AreEqual(kOffset, key.PartialOffset);
			    Assert.AreEqual(kLen, key.PartialLen);
			    Assert.AreEqual(kLen, (uint)key.Data.Length);
			}

			if (data != null) {
			    Assert.IsTrue(data.Partial);
			    Assert.AreEqual(dOffset, data.PartialOffset);
			    Assert.AreEqual(dLen, data.PartialLen);
			    Assert.AreEqual(dLen, (uint)data.Data.Length);
			}
		}
	}
}
