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
    public class HeapDatabaseConfigTest : DatabaseConfigTest {

        [TestFixtureSetUp]
        public override void SetUpTestFixture() {
            testFixtureName = "HeapDatabaseConfigTest";
            base.SetUpTestfixture();
        }

        [Test]
        new public void TestConfigWithoutEnv() {
            testName = "TestConfigWithoutEnv";
            SetUpTest(false);
            XmlElement xmlElem = Configuration.TestSetUp(
                testFixtureName, testName);
            HeapDatabaseConfig heapDBConfig =
                new HeapDatabaseConfig();
            Config(xmlElem, ref heapDBConfig, true);
            Confirm(xmlElem, heapDBConfig, true);
        }

        public static void Confirm(XmlElement xmlElement,
            HeapDatabaseConfig heapDBConfig, bool compulsory) {
            DatabaseConfig dbConfig = heapDBConfig;
            Confirm(xmlElement, dbConfig, compulsory);

            // Confirm Heap database specific configuration
            Configuration.ConfirmCreatePolicy(xmlElement, "Creation",
                heapDBConfig.Creation, compulsory);
        }

        public static void Config(XmlElement xmlElement,
            ref HeapDatabaseConfig heapDBConfig, bool compulsory) {
            DatabaseConfig dbConfig = heapDBConfig;
            Config(xmlElement, ref dbConfig, compulsory);

            // Configure specific fields/properties of Heap database
            Configuration.ConfigCreatePolicy(xmlElement, "Creation",
                ref heapDBConfig.Creation, compulsory);
        }
    }
}
