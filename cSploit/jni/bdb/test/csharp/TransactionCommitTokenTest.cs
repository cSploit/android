/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
    [TestFixture]
    public class TransactionCommitTokenTest : CSharpTestFixture
    {
        private uint masterPort;
        private uint clientPort;
        private string ip = "127.0.0.1";

 	[TestFixtureSetUp]
	public void SetUpTestFixture()
 	{
		testFixtureName = "TransactionCommitTokenTest";
		base.SetUpTestfixture();
		ReplicationTest.AvailablePorts portGen = 
		    new ReplicationTest.AvailablePorts();
		masterPort = portGen.Current;
		portGen.MoveNext();
		clientPort = portGen.Current;
	}

        public DatabaseEnvironment SetUpEnv(String home, uint priority, uint port, bool isMaster)
        {
            try    {
                Configuration.ClearDir(home);
            } catch (Exception e)    {
                Console.WriteLine(e.Message);
                throw new TestException("Please clean the directory");
            }

            /* Configure and open environment with replication
             * application.
             */
            DatabaseEnvironmentConfig cfg =
                new DatabaseEnvironmentConfig();
            cfg.UseReplication = true;
            cfg.MPoolSystemCfg = new MPoolConfig();
            cfg.MPoolSystemCfg.CacheSize =
                new CacheInfo(0, 20485760, 1);
            cfg.UseLocking = true;
            cfg.UseTxns = true;
            cfg.UseMPool = true;
            cfg.Create = true;
            cfg.UseLogging = true;
            cfg.RunRecovery = true;
            cfg.TxnNoSync = true;
            cfg.FreeThreaded = true;

            cfg.RepSystemCfg = new ReplicationConfig();
            DbSiteConfig dbSiteConfig = new DbSiteConfig();
            dbSiteConfig.Host = ip;
            dbSiteConfig.Port = port;
            dbSiteConfig.LocalSite = true;
            cfg.RepSystemCfg.RepmgrSitesConfig.Add(dbSiteConfig);
            cfg.RepSystemCfg.Priority = priority;
            if (!isMaster) {
                DbSiteConfig dbSiteConfig1 = new DbSiteConfig();
                dbSiteConfig1.Host = ip;
                dbSiteConfig1.Port = masterPort;
                dbSiteConfig1.Helper = true;
                cfg.RepSystemCfg.RepmgrSitesConfig.Add(dbSiteConfig1);
            }
            cfg.RepSystemCfg.BulkTransfer = false;
            cfg.RepSystemCfg.AckTimeout = 5000;

            cfg.RepSystemCfg.RepMgrAckPolicy =
                AckPolicy.ALL_PEERS;

            DatabaseEnvironment env = DatabaseEnvironment.Open(
                home, cfg);

            return env;
        }

        public BTreeDatabase Open(DatabaseEnvironment env, bool isMaster)
        {
            string dbName = "rep.db";

            // Set up the database.
            BTreeDatabaseConfig dbCfg = new BTreeDatabaseConfig();
            dbCfg.Env = env;
            if (isMaster)
                dbCfg.Creation = CreatePolicy.IF_NEEDED;

            dbCfg.AutoCommit = true;
            dbCfg.FreeThreaded = true;

            /*
             * Open the database. Do not provide a txn handle. This
             * Open is autocommitted because BTreeDatabaseConfig.AutoCommit
             * is true.
             */
            return BTreeDatabase.Open(dbName, dbCfg);
        }

        [Test]
        public void TestCommitSuccess()
        {
            testName = "TestCommitSuccess";
            SetUpTest(true);
            string[] keys = {"key 1", "key 2", "key 3", "key 4",
                "key 5", "key 6", "key 7", "key 8", "key 9", "key 10"};

            DatabaseEnvironment master = SetUpEnv(testHome + "/master", 100, masterPort, true);
            DatabaseEnvironment client = SetUpEnv(testHome + "/client", 0, clientPort, false);

            master.RepMgrStartMaster(2);
            Thread.Sleep(2000);
            client.RepMgrStartClient(2);

            BTreeDatabase db1 = Open(master, true);
            BTreeDatabase db2 = null;
            for (; ; ) {
                if (db2 == null)
                    try {
                       db2 = Open(client, false);
                       break;
                    } catch (DatabaseException) {
                        if (db2 != null) {
                            db2.Close(true);
                            db2 = null;
                        }
                        System.Threading.Thread.Sleep(1000);
                        continue;
                    }
            }

            try {
                for (int i = 0; i < 3; i++)    {
                    Transaction txn = master.BeginTransaction();

                    // Get the key.
                    DatabaseEntry key, data;
                    key = new DatabaseEntry(
                        ASCIIEncoding.ASCII.GetBytes(keys[i]));

                    data = new DatabaseEntry(
                        ASCIIEncoding.ASCII.GetBytes(keys[i]));

                    db1.Put(key, data, txn);
                    txn.Commit();

                    byte[] token = txn.CommitToken;
                    Assert.AreEqual(master.IsTransactionApplied(token, 5000), TransactionAppliedStatus.APPLIED);
                    Assert.AreEqual(client.IsTransactionApplied(token, 200000), TransactionAppliedStatus.APPLIED);
                }
            }
            finally {
                db1.Close();
                db2.Close();
                master.Close();
                client.Close();
            }
        }

        [Test]
        public void TestNullArgument()
        {
            testName = "TestNullArgument";
            SetUpTest(true);
            DatabaseEnvironment master = SetUpEnv(testHome + "/master", 100, 8000, true);
            Assert.Throws<ArgumentNullException>(
                delegate { master.IsTransactionApplied(null, 0); });
            master.Close();
        }

        [Test]
        public void TestValidArgument()
        {
            testName = "TestValidArgument";
            SetUpTest(true);
            DatabaseEnvironment master = SetUpEnv(testHome + "/master", 100, 8000, true);
            Assert.Throws<ArgumentOutOfRangeException>(
                delegate { master.IsTransactionApplied(new byte[10], 0); });
            master.Close();
        }

        [Test]
        public void TestEmptyException() {
            testName = "TestEmptyException";
            SetUpTest(true);
            DatabaseEnvironment master = SetUpEnv(testHome + "/master", 100, 8000, true);
            Transaction txn = master.BeginTransaction();
            txn.Commit();
            byte[] token = txn.CommitToken;
            Assert.AreEqual(TransactionAppliedStatus.EMPTY_TRANSACTION,
                master.IsTransactionApplied(token, 1000));
            master.Close();
        }

        [Test]
        public void TestInvalidToken() {
            testName = "TestInvalidToken";
            SetUpTest(true);
            DatabaseEntry key, data;
            DatabaseEnvironment master = SetUpEnv(testHome + "/master", 100, 8000, true);
            BTreeDatabase db = Open(master,true);
            Transaction txn = master.BeginTransaction();
            key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("Key"));
            data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("Data"));
            db.Put(key, data, txn);
            txn.Commit();
            byte[] token = txn.CommitToken;
            token[19] += 123;
            Assert.Throws<DatabaseException>(delegate { master.IsTransactionApplied(token, 1000); });
            db.Close();
            master.Close();
        }

        [Test]
        public void TestOperationOrder() {
            testName = "TestOperationOrder";
            SetUpTest(true);
            DatabaseEnvironment master = SetUpEnv(testHome + "/master", 100, 8000, true);
            Transaction txn = master.BeginTransaction();
            txn.Abort();
            byte[] token;
            Assert.Throws<InvalidOperationException>(delegate { token = txn.CommitToken; });
            master.Close();
        }

        [Test]
        public void TestNestedTXN() {
            testName = "TestNestedTXN";
            SetUpTest(true);
            DatabaseEntry key, data;
            DatabaseEnvironment master = SetUpEnv(testHome + "/master", 100, 8000, true);
            BTreeDatabase db = Open(master, true);
            TransactionConfig txnconfig = new TransactionConfig();
            Transaction txn = master.BeginTransaction();
            Transaction txn1 = master.BeginTransaction(txnconfig, txn);

            key = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("Key"));
            data = new DatabaseEntry(
                ASCIIEncoding.ASCII.GetBytes("Data"));
            db.Put(key, data, txn1);
            txn1.Commit();
            txn.Commit();

            byte[] token;
            Assert.Throws<ArgumentException>(delegate { token = txn1.CommitToken; });
            master.Close();
        }
    }
}
