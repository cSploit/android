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
	public class SecondaryBTreeDatabaseTest : CSharpTestFixture
	{

		[TestFixtureSetUp]
		public void SetUpTestFixture()
		{
			testFixtureName = "SecondaryBTreeDatabaseTest";
			base.SetUpTestfixture();
		}

		[Test]
		public void TestOpen()
		{
			testName = "TestOpen";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";
			string dbSecFileName = testHome + "/" +
			    testName + "_sec.db";

			XmlElement xmlElem = Configuration.TestSetUp(
			    testFixtureName, testName);

			// Open a primary btree database.
			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    dbFileName, btreeDBConfig);

			// Open a secondary btree database.
			SecondaryBTreeDatabaseConfig secDBConfig =
			    new SecondaryBTreeDatabaseConfig(btreeDB, null);

			SecondaryBTreeDatabaseConfigTest.Config(xmlElem,
			    ref secDBConfig, true);
			SecondaryBTreeDatabase secDB =
			   SecondaryBTreeDatabase.Open(dbSecFileName,
			   secDBConfig);

			// Confirm its flags configured in secDBConfig.
			Confirm(xmlElem, secDB, true);

			secDB.Close();
			btreeDB.Close();
		}

		[Test]
		public void TestCompare()
		{
			testName = "TestCompare";
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
			    new SecondaryBTreeDatabaseConfig(null, null);
			secBtreeDBConfig.Primary = btreeDB;
			secBtreeDBConfig.Compare =
			    new EntryComparisonDelegate(
			    SecondaryEntryComparison);
			secBtreeDBConfig.KeyGen =
			    new SecondaryKeyGenDelegate(SecondaryKeyGen);
			SecondaryBTreeDatabase secDB =
			    SecondaryBTreeDatabase.Open(
			    dbFileName, secBtreeDBConfig);

			/*
			 * Get the compare function set in the configuration 
			 * and run it in a comparison to see if it is alright.
			 */
			EntryComparisonDelegate cmp =
			    secDB.Compare;
			DatabaseEntry dbt1, dbt2;
			dbt1 = new DatabaseEntry(
			    BitConverter.GetBytes((int)257));
			dbt2 = new DatabaseEntry(
			    BitConverter.GetBytes((int)255));
			Assert.Less(0, cmp(dbt1, dbt2));


			for (int i = 0; i < 1000; i++)
				btreeDB.Put(new DatabaseEntry(
				    BitConverter.GetBytes(i)), new DatabaseEntry(
				    BitConverter.GetBytes(i)));

			secDB.Close();
			btreeDB.Close();
		}

		[Test]
		public void TestDupCompare() 
		{
			testName = "TestDupCompare";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";

			BTreeDatabaseConfig cfg =
			    new BTreeDatabaseConfig();
			cfg.Creation = CreatePolicy.ALWAYS;
			BTreeDatabase pDb = BTreeDatabase.Open(
			    dbFileName, "p", cfg);

			SecondaryBTreeDatabaseConfig sCfg =
			    new SecondaryBTreeDatabaseConfig(pDb,
			    new SecondaryKeyGenDelegate(SecondaryKeyGen));
			sCfg.Creation = CreatePolicy.IF_NEEDED;

			DatabaseEntry dbt1, dbt2;
			dbt1 = new DatabaseEntry(
			    BitConverter.GetBytes((Int32)1));
			dbt2 = new DatabaseEntry(
			    BitConverter.GetBytes((Int64)4294967297));

			// Do not set the duplicate comparison delegate.
			using (SecondaryBTreeDatabase sDb =
			    SecondaryBTreeDatabase.Open(dbFileName, "s1", sCfg)) {
				try {
					int ret = sDb.DupCompare(dbt1, dbt2);
					throw new TestException();
				} catch (NullReferenceException) {
				}
			}

			sCfg.DuplicateCompare = new EntryComparisonDelegate(
			    SecondaryEntryComparison);
			using (SecondaryBTreeDatabase sDb =
			    SecondaryBTreeDatabase.Open(dbFileName, "s2", sCfg)) {
				/*
				 * Get the dup compare function set in the 
				 * configuration and run it in a comparison to 
				 * see if it is alright.
				 */
				int ret = 1 - BitConverter.ToInt32(
				    BitConverter.GetBytes((Int64)4294967297), 0);

				Assert.AreEqual(ret, sDb.DupCompare(dbt1, dbt2));
			}
			pDb.Close();
		}

		[Test]
		public void TestDuplicates()
		{
			testName = "TestDuplicates";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";
			string dbSecFileName = testHome + "/" + testName
			    + "_sec.db";

			// Open a primary btree database.
			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    dbFileName, btreeDBConfig);

			// Open a secondary btree database. 
			SecondaryBTreeDatabaseConfig secBtreeDBConfig =
			    new SecondaryBTreeDatabaseConfig(btreeDB, null);
			secBtreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
			secBtreeDBConfig.Duplicates = DuplicatesPolicy.SORTED;
			SecondaryBTreeDatabase secDB =
			    SecondaryBTreeDatabase.Open(dbSecFileName,
			    secBtreeDBConfig);
			Assert.AreEqual(DuplicatesPolicy.SORTED, secDB.Duplicates);

			secDB.Close();
			btreeDB.Close();
		}

		[Test]
		public void TestPrefixCompare()
		{
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
			secBtreeDBConfig.Compare =
			    new EntryComparisonDelegate(
			    SecondaryEntryComparison);
			secBtreeDBConfig.PrefixCompare =
			    new EntryPrefixComparisonDelegate(
			    SecondaryPrefixEntryComparison);
			secBtreeDBConfig.KeyGen =
			    new SecondaryKeyGenDelegate(
			    SecondaryKeyGen);
			SecondaryBTreeDatabase secDB =
			    SecondaryBTreeDatabase.Open(
			    dbFileName, secBtreeDBConfig);

			/*
			 * Get the prefix compare function set in the 
			 * configuration and run it in a comparison to 
			 * see if it is alright.
			 */
			EntryPrefixComparisonDelegate cmp =
			    secDB.PrefixCompare;
			DatabaseEntry dbt1, dbt2;
			dbt1 = new DatabaseEntry(
			    BitConverter.GetBytes((Int32)1));
			dbt2 = new DatabaseEntry(
			    BitConverter.GetBytes((Int64)4294967297));
			Assert.AreEqual(5, cmp(dbt1, dbt2));

			secDB.Close();
			btreeDB.Close();
		}

		public int SecondaryEntryComparison(
		    DatabaseEntry dbt1, DatabaseEntry dbt2)
		{
			int a, b;
			a = BitConverter.ToInt32(dbt1.Data, 0);
			b = BitConverter.ToInt32(dbt2.Data, 0);
			return a - b;
		}

		private uint SecondaryPrefixEntryComparison(DatabaseEntry dbt1,
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

		public DatabaseEntry SecondaryKeyGen(
		    DatabaseEntry key, DatabaseEntry data)
		{
			DatabaseEntry dbtGen;
			dbtGen = new DatabaseEntry(data.Data);
			return dbtGen;
		}

		public static void Confirm(XmlElement xmlElem,
		    SecondaryBTreeDatabase secDB, bool compulsory)
		{
			Configuration.ConfirmDuplicatesPolicy(xmlElem,
			    "Duplicates", secDB.Duplicates, compulsory);
			Configuration.ConfirmUint(xmlElem, "MinKeysPerPage",
			    secDB.MinKeysPerPage, compulsory);
			Configuration.ConfirmBool(xmlElem, "NoReverseSplitting",
                secDB.ReverseSplit, compulsory);
			Configuration.ConfirmBool(xmlElem, "UseRecordNumbers",
			    secDB.RecordNumbers, compulsory);
		}
	}
}
