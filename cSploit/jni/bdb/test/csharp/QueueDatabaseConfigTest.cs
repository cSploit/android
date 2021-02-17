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
	public class QueueDatabaseConfigTest : DatabaseConfigTest
	{

		[TestFixtureSetUp]
		public override void SetUpTestFixture()
		{
			testFixtureName = "QueueDatabaseConfigTest";
			base.SetUpTestfixture();
		}

		[Test]
		 new public void TestConfigWithoutEnv()
		{
			testName = "TestConfigWithoutEnv";
			SetUpTest(false);
			XmlElement xmlElem = Configuration.TestSetUp(
			    testFixtureName, testName);
			QueueDatabaseConfig queueDBConfig =
			    new QueueDatabaseConfig();
			Config(xmlElem, ref queueDBConfig, true);
			Confirm(xmlElem, queueDBConfig, true);
		}

		public static void Confirm(XmlElement xmlElement,
		    QueueDatabaseConfig queueDBConfig, bool compulsory)
		{
			DatabaseConfig dbConfig = queueDBConfig;
			Confirm(xmlElement, dbConfig, compulsory);

			// Confirm Queue database specific configuration
			Configuration.ConfirmBool(xmlElement, "ConsumeInOrder",
			    queueDBConfig.ConsumeInOrder, compulsory);
			Configuration.ConfirmCreatePolicy(xmlElement, "Creation",
			    queueDBConfig.Creation, compulsory);
			Configuration.ConfirmUint(xmlElement, "Length",
			    queueDBConfig.Length, compulsory);
			Configuration.ConfirmInt(xmlElement, "PadByte",
			    queueDBConfig.PadByte, compulsory);
			Configuration.ConfirmUint(xmlElement, "ExtentSize",
			    queueDBConfig.ExtentSize, compulsory);
		}

		public static void Config(XmlElement xmlElement,
		    ref QueueDatabaseConfig queueDBConfig, bool compulsory)
		{
			uint uintValue = new uint();
			int intValue = new int();
			DatabaseConfig dbConfig = queueDBConfig;
			Config(xmlElement, ref dbConfig, compulsory);

			// Configure specific fields/properties of Queue database
			Configuration.ConfigBool(xmlElement, "ConsumeInOrder",
			    ref queueDBConfig.ConsumeInOrder, compulsory);
			Configuration.ConfigCreatePolicy(xmlElement, "Creation",
			    ref queueDBConfig.Creation, compulsory);
			if (Configuration.ConfigUint(xmlElement, "Length",
			    ref uintValue, compulsory))
				queueDBConfig.Length = uintValue;
			if (Configuration.ConfigInt(xmlElement, "PadByte",
			    ref intValue, compulsory))
				queueDBConfig.PadByte = intValue;
			if (Configuration.ConfigUint(xmlElement, "ExtentSize",
			    ref uintValue, compulsory))
				queueDBConfig.ExtentSize = uintValue;
		}
	}
}
