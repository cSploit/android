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
	public class HashCursorTest : CSharpTestFixture
	{

		[TestFixtureSetUp]
		public void SetUpTestFixture()
		{
			testFixtureName = "HashCursorTest";
			base.SetUpTestfixture();
		}

		[Test]
		public void TestAddToLoc()
		{
			HashDatabase db;
			HashCursor cursor;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			testName = "TestAddToLoc";
			SetUpTest(true);

			/*
			 * Open a hash database and cursor and then 
			 * add record("key", "data") into database.
			 */ 
			GetHashDBAndCursor(testHome, testName, out db, out cursor);
			AddOneByCursor(cursor);

			/*
			 * Add the new record("key","data1") as the first
			 * of the data item of "key".
			 */
			pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data1")));
			cursor.Add(pair, Cursor.InsertLocation.FIRST);

			/*
			 * Confirm that the new record is added as the first of 
			 * the data item of "key".
			 */
			cursor.Move(new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")), true);
			Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("data1"), 
			    cursor.Current.Value.Data);

			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestAddUnique()
		{
			HashDatabase db;
			HashCursor cursor;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			testName = "TestAddUnique";
			SetUpTest(true);

			// Open a database and cursor.
			HashDatabaseConfig dbConfig = new HashDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;

			/*
			 * To put no duplicate data, the database should be 
			 * set to be sorted.
			 */
			dbConfig.Duplicates = DuplicatesPolicy.SORTED;
			db = HashDatabase.Open(
			    testHome + "/" + testName + ".db", dbConfig);
			cursor = db.Cursor();

			// Add record("key", "data") into database.
			AddOneByCursor(cursor);

			/*
			 * Fail to add duplicate record("key","data").
			 */
			pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
			try
			{
				cursor.AddUnique(pair);
			}
			catch (KeyExistException)
			{
			}
			finally
			{
				cursor.Close();
				db.Close();
			}
		}

		[Test]
		public void TestDuplicate()
		{
			HashDatabase db;
			HashCursor cursor, dupCursor;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;

			testName = "TestDuplicate";
			SetUpTest(true);

			GetHashDBAndCursor(testHome, testName,
			    out db, out cursor);

			/*
			 * Add a record("key", "data") by cursor and move 
			 * the cursor to the current record.
			 */
			AddOneByCursor(cursor);
			cursor.Refresh();

			//Duplicate a new cursor to the same position. 
			dupCursor = cursor.Duplicate(true);

			// Overwrite the record.
			dupCursor.Overwrite(new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("newdata")));

			// Confirm that the original data doesn't exist.
			pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
			Assert.IsFalse(dupCursor.Move(pair, true));

			dupCursor.Close();
			cursor.Close();
			db.Close();
		}

		[Test]
		public void TestInsertToLoc()
		{
			HashDatabase db;
			HashDatabaseConfig dbConfig;
			HashCursor cursor;
			DatabaseEntry data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			string dbFileName;

			testName = "TestInsertToLoc";
			SetUpTest(true);
			dbFileName = testHome + "/" + testName + ".db";

			// Open database and cursor.
			dbConfig = new HashDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;

			/*
			 * The database should be set to be unsorted to 
			 * insert before/after a certain record.
			 */
			dbConfig.Duplicates = DuplicatesPolicy.UNSORTED;
			db = HashDatabase.Open(dbFileName, dbConfig);
			cursor = db.Cursor();

			// Add record("key", "data") into database.
			AddOneByCursor(cursor);

			/*
			 * Insert the new record("key","data1") after the 
			 * record("key", "data").
			 */
			data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data1"));
			cursor.Insert(data, Cursor.InsertLocation.AFTER);

			/*
			 * Move the cursor to the record("key", "data") and 
			 * confirm that the next record is the one just inserted. 
			 */
			pair = new KeyValuePair<DatabaseEntry, DatabaseEntry>(
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key")),
			    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data")));
			Assert.IsTrue(cursor.Move(pair, true));
			Assert.IsTrue(cursor.MoveNext());
			Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("key"),
			    cursor.Current.Key.Data);
			Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("data1"),
			    cursor.Current.Value.Data);

			try {
				try {
					cursor.Insert(data, 
					    Cursor.InsertLocation.FIRST);
					throw new TestException();
				} catch (ArgumentException) { }
				try
				{
					cursor.Insert(data, 
					    Cursor.InsertLocation.LAST);
					throw new TestException();
				} catch (ArgumentException) { }
			} finally {
				cursor.Close();
				db.Close();
			}
		}

		public void GetHashDBAndCursor(string home, string name, 
		    out HashDatabase db, out HashCursor cursor)
		{
			string dbFileName = home + "/" + name + ".db";
			HashDatabaseConfig dbConfig = new HashDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			db = HashDatabase.Open(dbFileName, dbConfig);
			cursor = db.Cursor();
		}

		public void AddOneByCursor(HashCursor cursor)
		{
			DatabaseEntry key, data;
			KeyValuePair<DatabaseEntry, DatabaseEntry> pair;
			key = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("key"));
			data = new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes("data"));
			pair = new KeyValuePair<DatabaseEntry,DatabaseEntry>(key, data);
			cursor.Add(pair);
		}
	}
}
