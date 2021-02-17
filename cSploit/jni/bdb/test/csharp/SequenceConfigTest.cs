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
	public class SequenceConfigTest : CSharpTestFixture
	{

		[TestFixtureSetUp]
		public void SetUpTestFixture()
		{
			testFixtureName = "SequenceConfigTest";
			base.SetUpTestfixture();
		}

		[Test]
		public void TestConfig()
		{
			testName = "TestConfig";
			SetUpTest(false);
			SequenceConfig seqConfig = new SequenceConfig();
			XmlElement xmlElem = Configuration.TestSetUp(
			    testFixtureName, testName);
			Config(xmlElem, ref seqConfig, true);
			Confirm(xmlElem, seqConfig, true);
		}

		[Test]
		public void TestConfigObj()
		{
			testName = "TestConfigObj";
			SetUpTest(true);
			string dbFileName = testHome + "/" + testName + ".db";

			// Open a database.
			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Creation = CreatePolicy.IF_NEEDED;
			BTreeDatabase btreeDB = BTreeDatabase.Open(
			    dbFileName, btreeDBConfig);

			/* Configure and initialize sequence. */
			SequenceConfig seqConfig = new SequenceConfig();
			seqConfig.BackingDatabase = btreeDB;
            seqConfig.Creation = CreatePolicy.IF_NEEDED;
			seqConfig.key = new DatabaseEntry(
			    ASCIIEncoding.ASCII.GetBytes("key"));
			seqConfig.SetRange(Int64.MinValue, Int64.MaxValue);
			Sequence seq = new Sequence(seqConfig);

			// Confirm the objects set in SequenceConfig.
			Assert.AreEqual(dbFileName,
			    seq.BackingDatabase.FileName);
			Assert.AreEqual(ASCIIEncoding.ASCII.GetBytes("key"),
			    seq.Key.Data);
			Assert.AreEqual(Int64.MinValue, seq.Min);
			Assert.AreEqual(Int64.MaxValue, seq.Max);

			/* Close sequence, database and environment. */
			seq.Close();
			btreeDB.Close();
		}

		public static void Confirm(XmlElement xmlElement,
		    SequenceConfig seqConfig, bool compulsory)
		{
			Configuration.ConfirmUint(xmlElement, "CacheSize",
			    seqConfig.CacheSize, compulsory);
            Configuration.ConfirmCreatePolicy(xmlElement, "Creation",
                seqConfig.Creation, compulsory);
            Configuration.ConfirmBool(xmlElement, "Decrement",
			    seqConfig.Decrement, compulsory);
			Configuration.ConfirmBool(xmlElement, "FreeThreaded",
			    seqConfig.FreeThreaded, compulsory);
			Configuration.ConfirmBool(xmlElement, "Increment",
			    seqConfig.Increment, compulsory);
			Configuration.ConfirmLong(xmlElement, "InitialValue",
			    seqConfig.InitialValue, compulsory);
			Configuration.ConfirmBool(xmlElement, "Wrap",
			    seqConfig.Wrap, compulsory);
		}

		public static void Config(XmlElement xmlElement,
		    ref SequenceConfig seqConfig, bool compulsory)
		{
			uint uintValue = new uint();
			bool boolValue = new bool();
			long longValue = new long();

			if (Configuration.ConfigUint(xmlElement, "CacheSize",
			    ref uintValue, compulsory))
				seqConfig.CacheSize = uintValue;
            Configuration.ConfigCreatePolicy(xmlElement, "Creation",
                ref seqConfig.Creation, compulsory);
            if (Configuration.ConfigBool(xmlElement, "Decrement",
			    ref boolValue, compulsory))
				seqConfig.Decrement = boolValue;
			Configuration.ConfigBool(xmlElement, "FreeThreaded",
			    ref seqConfig.FreeThreaded, compulsory);
			if (Configuration.ConfigBool(xmlElement, "Increment",
			    ref boolValue, compulsory))
				seqConfig.Increment = boolValue;
			if (Configuration.ConfigLong(xmlElement, "InitialValue",
			    ref longValue, compulsory))
				seqConfig.InitialValue = longValue;
			Configuration.ConfigBool(xmlElement, "Wrap",
			    ref seqConfig.Wrap, compulsory);
		}
	}
}
