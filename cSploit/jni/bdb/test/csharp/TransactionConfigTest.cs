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
	public class TransactionConfigTest : CSharpTestFixture
	{

		[TestFixtureSetUp]
		public void SetUpTestFixture()
		{
			testFixtureName = "TransactionConfigTest";
			base.SetUpTestfixture();
		}

		[Test]
		public void TestConfig()
		{
			testName = "TestConfig";
			SetUpTest(false);
			/* 
			 * Configure the fields/properties and see if 
			 * they are updated successfully.
			 */
			TransactionConfig txnConfig = new TransactionConfig();
			XmlElement xmlElem = Configuration.TestSetUp(
			    testFixtureName, testName);
			Config(xmlElem, ref txnConfig, true);
			Confirm(xmlElem, txnConfig, true);
		}

		public static void Confirm(XmlElement xmlElement, 
		    TransactionConfig txnConfig, bool compulsory)
		{
			Configuration.ConfirmIsolation(xmlElement,
			    "IsolationDegree", txnConfig.IsolationDegree,
			    compulsory);
			Configuration.ConfirmBool(xmlElement, "NoWait",
			    txnConfig.NoWait, compulsory);
			Configuration.ConfirmBool(xmlElement, "Snapshot",
			    txnConfig.Snapshot, compulsory);
			Configuration.ConfirmLogFlush(xmlElement, "SyncAction",
			    txnConfig.SyncAction, compulsory);
		}

		public static void Config(XmlElement xmlElement,
		    ref TransactionConfig txnConfig, bool compulsory)
		{
			Configuration.ConfigIsolation(xmlElement,
			    "IsolationDegree", ref txnConfig.IsolationDegree,
			    compulsory);
			Configuration.ConfigBool(xmlElement, "NoWait",
			    ref txnConfig.NoWait, compulsory);
			Configuration.ConfigBool(xmlElement, "Snapshot",
			    ref txnConfig.Snapshot, compulsory);
			Configuration.ConfigLogFlush(xmlElement, "SyncAction",
			    ref txnConfig.SyncAction, compulsory);
		}
	}
}
