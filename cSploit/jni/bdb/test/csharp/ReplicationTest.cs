/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Text;
using System.Threading;
using System.Xml;
using NUnit.Framework;
using BerkeleyDB;

namespace CsharpAPITest
{
	[TestFixture]
	public class ReplicationTest : CSharpTestFixture
	{
		private EventWaitHandle clientStartSignal;
		private EventWaitHandle masterCloseSignal;

		private EventWaitHandle client1StartSignal;
		private EventWaitHandle client2StartSignal;
		private EventWaitHandle client3StartSignal;
		private EventWaitHandle clientsElectionSignal;
		private EventWaitHandle masterLeaveSignal;

		private uint startUpDone;
		private bool electionDone;

		List<uint> ports = new List<uint>();

		[TestFixtureSetUp]
		public void SetUp()
		{
			testFixtureName = "ReplicationTest";
			base.SetUpTestfixture();
		}

		[Test]
		public void TestReplicationView()
		{
			testName = "TestReplicationView";
			SetUpTest(true);

			string masterHome = testHome + "\\Master";
			Configuration.ClearDir(masterHome);

			string clientHome1 = testHome + "\\Client1";
			Configuration.ClearDir(clientHome1);

			string clientHome2 = testHome + "\\Client2";
			Configuration.ClearDir(clientHome2);

			ports.Clear();
			AvailablePorts portGen = new AvailablePorts();
			uint mPort = portGen.Current;
			portGen.MoveNext();
			uint cPort1 = portGen.Current;
			portGen.MoveNext();
			uint cPort2 = portGen.Current;

			/* Open environment with replication configuration. */
			DatabaseEnvironmentConfig cfg =
			    new DatabaseEnvironmentConfig();
			cfg.Create = true;
			cfg.RunRecovery = true;
			cfg.UseLocking = true;
			cfg.UseLogging = true;
			cfg.UseMPool = true;
			cfg.UseReplication = true;
			cfg.FreeThreaded = true;
			cfg.UseTxns = true;
			cfg.EventNotify = new EventNotifyDelegate(stuffHappened);

			cfg.RepSystemCfg = new ReplicationConfig();
			cfg.RepSystemCfg.RepmgrSitesConfig.Add(new DbSiteConfig());
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Host = "127.0.0.1";
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Port = mPort;
			cfg.RepSystemCfg.RepmgrSitesConfig[0].LocalSite = true;
			cfg.RepSystemCfg.RepmgrSitesConfig[0].GroupCreator = true;
			cfg.RepSystemCfg.Priority = 100;

			/* Start up the master site. */
			DatabaseEnvironment mEnv = DatabaseEnvironment.Open(
			    masterHome, cfg);
			mEnv.DeadlockResolution = DeadlockPolicy.DEFAULT;
			mEnv.RepMgrStartMaster(2);

			/* Open the environment of the client 1 site. */
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Port = cPort1;
			cfg.RepSystemCfg.RepmgrSitesConfig[0].GroupCreator = false;
			cfg.RepSystemCfg.Priority = 10;
			cfg.RepSystemCfg.RepmgrSitesConfig.Add(new DbSiteConfig());
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Host = "127.0.0.1";
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Port = mPort;
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Helper = true;
			/* Set the site as a partial view. */
			cfg.RepSystemCfg.ReplicationView = repView;
			DatabaseEnvironment cEnv1 = DatabaseEnvironment.Open(
			    clientHome1, cfg);

			/* Open the environment of the client 2 site. */
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Port = cPort2;
			cfg.RepSystemCfg.RepmgrSitesConfig[0].GroupCreator = false;
			cfg.RepSystemCfg.Priority = 10;
			/* Set the site as a full view. */
			cfg.RepSystemCfg.ReplicationView = null;
			DatabaseEnvironment cEnv2 = DatabaseEnvironment.Open(
			    clientHome2, cfg);

			/*
			 * Create two database files db1.db and db2.db
			 * on the master.
			 */
			BTreeDatabaseConfig btreeDBConfig =
			    new BTreeDatabaseConfig();
			btreeDBConfig.Env = mEnv;
			btreeDBConfig.Creation = CreatePolicy.ALWAYS;
			btreeDBConfig.AutoCommit = true;
			BTreeDatabase db1 =
			    BTreeDatabase.Open("db1.db", btreeDBConfig);
			BTreeDatabase db2 =
			    BTreeDatabase.Open("db2.db", btreeDBConfig);
			db1.Close();
			db2.Close();

			/* Start up the client sites. */
			cEnv1.RepMgrStartClient(2, false);
			cEnv2.RepMgrStartClient(2, false);

			/* Wait for clients to start up */
			int i = 0;
			while (!cEnv1.ReplicationSystemStats().ClientStartupComplete)
			{
				if (i < 20)
				{
					Thread.Sleep(1000);
					i++;
				}
				else
					throw new TestException();
			}
			i = 0;
			while (!cEnv2.ReplicationSystemStats().ClientStartupComplete)
			{
				if (i < 20)
				{
					Thread.Sleep(1000);
					i++;
				}
				else
					throw new TestException();
			}

			/*
			 * Verify that the file db2.db is replicated to the
			 * client 2 (full view), but not to the client 1
			 * (partial view), and the file db1.db is
			 * replicated to both sites.
			 */
			btreeDBConfig.Env = cEnv1;
			btreeDBConfig.Creation = CreatePolicy.NEVER;
			db1 = BTreeDatabase.Open("db1.db", btreeDBConfig);
			try
			{
				db2 = BTreeDatabase.Open("db2.db", btreeDBConfig);
				throw new TestException();
			}
			catch (DatabaseException e){
				Assert.AreEqual(0, String.Compare(
				    "No such file or directory", e.Message));
			}
			db1.Close();
			btreeDBConfig.Env = cEnv2;
			db1 = BTreeDatabase.Open("db1.db", btreeDBConfig);
			db2 = BTreeDatabase.Open("db2.db", btreeDBConfig);
			db1.Close();
			db2.Close();

			/* Get the replication manager statistic. */
			RepMgrStats repMgrStats = mEnv.RepMgrSystemStats();
			Assert.AreEqual(1, repMgrStats.ParticipantSites);
			Assert.AreEqual(3, repMgrStats.TotalSites);
			Assert.AreEqual(2, repMgrStats.ViewSites);

			/*
			 * Verify the master is not a view locally
			 * or from remote site.
			 */
			ReplicationStats repstats =
			    mEnv.ReplicationSystemStats();
			Assert.AreEqual(false, repstats.View);
			RepMgrSite[] rsite = cEnv1.RepMgrRemoteSites;
			Assert.AreEqual(2, rsite.Length);
			for (i = 0; i < rsite.Length; i++) {
				if (rsite[i].Address.Port == mPort)
					break;
			}
			Assert.Greater(rsite.Length, i);
			Assert.AreEqual(false, rsite[i].isView);

			/*
			 * Verify the clients are views locally
			 * and from remote site.
			 */
			rsite = mEnv.RepMgrRemoteSites;
			Assert.AreEqual(2, rsite.Length);
			Assert.AreEqual(true, rsite[0].isView);
			Assert.AreEqual(true, rsite[1].isView);
			repstats = cEnv1.ReplicationSystemStats();
			Assert.AreEqual(true, repstats.View);
			repstats = cEnv2.ReplicationSystemStats();
			Assert.AreEqual(true, repstats.View);

			cEnv2.Close();
			cEnv1.Close();
			mEnv.Close();
		}

		int repView(string name, ref int result, uint flags)
		{
			if (name == "db1.db")
				result = 1;
			else
				result = 0;
			return (0);
		}

		[Test]
		public void TestRepMgrSite() 
		{
			testName = "TestRepMgrSite";
			SetUpTest(true);

			string masterHome = testHome + "\\Master";
			Configuration.ClearDir(masterHome);

			string clientHome = testHome + "\\Client";
			Configuration.ClearDir(clientHome);

			ports.Clear();
			AvailablePorts portGen = new AvailablePorts();
			uint mPort = portGen.Current;
			portGen.MoveNext();
			uint cPort = portGen.Current;

			// Open environment with replication configuration.
			DatabaseEnvironmentConfig cfg =
			    new DatabaseEnvironmentConfig();
			cfg.Create = true;
			cfg.RunRecovery = true;
			cfg.UseLocking = true;
			cfg.UseLogging = true;
			cfg.UseMPool = true;
			cfg.UseReplication = true;
			cfg.FreeThreaded = true;
			cfg.UseTxns = true;
			cfg.EventNotify = new EventNotifyDelegate(stuffHappened);

			cfg.RepSystemCfg = new ReplicationConfig();
			cfg.RepSystemCfg.RepmgrSitesConfig.Add(new DbSiteConfig());
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Host = "127.0.0.1";
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Port = mPort;
			cfg.RepSystemCfg.RepmgrSitesConfig[0].LocalSite = true;
			cfg.RepSystemCfg.RepmgrSitesConfig[0].GroupCreator = true;
			cfg.RepSystemCfg.Priority = 100;

			DatabaseEnvironment mEnv = DatabaseEnvironment.Open(
			    masterHome, cfg);
			mEnv.DeadlockResolution = DeadlockPolicy.DEFAULT;
			mEnv.RepMgrStartMaster(2);

			cfg.RepSystemCfg.RepmgrSitesConfig[0].Port = cPort;
			cfg.RepSystemCfg.RepmgrSitesConfig[0].GroupCreator = false;
			cfg.RepSystemCfg.Priority = 10;
			cfg.RepSystemCfg.RepmgrSitesConfig.Add(new DbSiteConfig());
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Host = "127.0.0.1";
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Port = mPort;
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Helper = true;
			DatabaseEnvironment cEnv = DatabaseEnvironment.Open(
			    clientHome, cfg);
			cEnv.RepMgrStartClient(2, false);

			/* Wait for client to start up */
			int i = 0;
			while (!cEnv.ReplicationSystemStats().ClientStartupComplete) {
				if (i < 20) {
					Thread.Sleep(1000);
					i++;
				} else
					throw new TestException();
			}

			/*
			 * Verify the client info could be observed by master's
			 * remote site.
			 */ 			 
			Assert.AreEqual(1, mEnv.RepMgrRemoteSites.Length);
			RepMgrSite rsite = mEnv.RepMgrRemoteSites[0];
			Assert.AreEqual("127.0.0.1", rsite.Address.Host);
			Assert.AreEqual(cPort, rsite.Address.Port);
			Assert.AreEqual(true, rsite.isConnected);
			Assert.AreEqual(false, rsite.isPeer);

			DbSite site = mEnv.RepMgrSite("127.0.0.1", mPort);
			Assert.AreEqual("127.0.0.1", site.Address.Host);
			Assert.AreEqual(mPort, site.Address.Port);
			Assert.LessOrEqual(0, site.EId);
			Assert.AreEqual(true, site.GroupCreator);
			Assert.AreEqual(true, site.LocalSite);
			Assert.AreEqual(false, site.Helper);
			Assert.AreEqual(false, site.Legacy);
			Assert.AreEqual(false, site.Peer);
			site.Close();

			site = mEnv.RepMgrSite("127.0.0.1", cPort);
			Assert.AreEqual("127.0.0.1", site.Address.Host);
			Assert.AreEqual(cPort, site.Address.Port);
			Assert.AreEqual(rsite.EId, site.EId);
			Assert.AreEqual(false, site.GroupCreator);
			Assert.AreEqual(false, site.LocalSite);
			Assert.AreEqual(false, site.Helper);
			Assert.AreEqual(false, site.Legacy);
			Assert.AreEqual(false, site.Peer);
			site.Remove();

			cEnv.Close();
			mEnv.Close();

			/*
			 * Update the repmgr site, and verify it is updated in
			 * unmanaged memory.
			 */ 			 
			rsite.Address = new ReplicationHostAddress(
			    "192.168.1.1", 1000);
			rsite.EId = 1024;
			rsite.isConnected = false;
			rsite.isPeer = true;
			Assert.AreEqual("192.168.1.1", rsite.Address.Host);
			Assert.AreEqual(1000, rsite.Address.Port);
			Assert.AreEqual(1024, rsite.EId);
			Assert.AreEqual(false, rsite.isConnected);
			Assert.AreEqual(true, rsite.isPeer);
		}

		[Test]
		public void TestRepMgr()
		{
			testName = "TestRepMgr";
			SetUpTest(true);

			// Initialize ports for master and client.
			ports.Clear();
			AvailablePorts portGen = new AvailablePorts();
			ports.Insert(0, portGen.Current);
			portGen.MoveNext();
			ports.Insert(1, portGen.Current);

			clientStartSignal = new AutoResetEvent(false);
			masterCloseSignal = new AutoResetEvent(false);

			Thread thread1 = new Thread(new ThreadStart(Master));
			Thread thread2 = new Thread(new ThreadStart(Client));

			// Start master thread before client thread.
			thread1.Start();
			Thread.Sleep(1000);
			thread2.Start();
			thread2.Join();
			thread1.Join();

			clientStartSignal.Close();
			masterCloseSignal.Close();
		}

		public void Master()
		{
			string home = testHome + "/Master";
			string dbName = "rep.db";
			Configuration.ClearDir(home);

			/*
			 * Configure and open environment with replication 
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
			cfg.RepSystemCfg.RepmgrSitesConfig.Add(new DbSiteConfig());
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Host = "127.0.0.1";
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Port = ports[0];
			cfg.RepSystemCfg.RepmgrSitesConfig[0].LocalSite = true;
			cfg.RepSystemCfg.Priority = 100;
			cfg.RepSystemCfg.BulkTransfer = true;
			cfg.RepSystemCfg.AckTimeout = 2000;
			cfg.RepSystemCfg.BulkTransfer = true;
			cfg.RepSystemCfg.CheckpointDelay = 1500;
			cfg.RepSystemCfg.Clockskew(102, 100);
			cfg.RepSystemCfg.ConnectionRetry = 10;
			cfg.RepSystemCfg.DelayClientSync = false;
			cfg.RepSystemCfg.ElectionRetry = 5;
			cfg.RepSystemCfg.ElectionTimeout = 3000;
			cfg.RepSystemCfg.FullElectionTimeout = 5000;
			cfg.RepSystemCfg.HeartbeatMonitor = 100;
			cfg.RepSystemCfg.HeartbeatSend = 10;
			cfg.RepSystemCfg.LeaseTimeout = 1300;
			cfg.RepSystemCfg.AutoInit = true;
			cfg.RepSystemCfg.NoBlocking = false;
			cfg.RepSystemCfg.RepMgrAckPolicy = 
			    AckPolicy.ALL_PEERS;
			cfg.RepSystemCfg.RetransmissionRequest(10, 100);
			cfg.RepSystemCfg.Strict2Site = true;
			cfg.RepSystemCfg.UseMasterLeases = false;
			cfg.EventNotify = new EventNotifyDelegate(stuffHappened);
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    home, cfg);

			// Get initial replication stats.
			ReplicationStats repStats = env.ReplicationSystemStats();
			env.PrintReplicationSystemStats();
			Assert.AreEqual(100, repStats.EnvPriority);
			Assert.AreEqual(1, 
			    repStats.CurrentElectionGenerationNumber);
			Assert.AreEqual(0, repStats.CurrentGenerationNumber);
			Assert.AreEqual(0, repStats.AppliedTransactions);
			Assert.AreEqual(0, repStats.ElectionDataGeneration);

			// Start a master site with replication manager.
			env.RepMgrStartMaster(3);

			// Open a btree database and write some data.
			Transaction txn = env.BeginTransaction();
			BTreeDatabaseConfig dbConfig = 
			    new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.IF_NEEDED;
			dbConfig.Env = env;
			dbConfig.PageSize = 512;
			BTreeDatabase db = BTreeDatabase.Open(dbName, 
			    dbConfig, txn);
			txn.Commit();
			txn = env.BeginTransaction();
			for (int i = 0; i < 5; i++)
				db.Put(new DatabaseEntry(BitConverter.GetBytes(i)),
				    new DatabaseEntry(BitConverter.GetBytes(i)), txn);
			txn.Commit();

			Console.WriteLine(
			    "Master: Finished initialization and data#1.");

			// Client site could enter now.
			clientStartSignal.Set();
			Console.WriteLine(
			    "Master: Wait for Client to join and get #1.");

			Console.WriteLine("...");

			// Put some new data into master site.
			txn = env.BeginTransaction();
			for (int i = 10; i < 15; i++)
				db.Put(new DatabaseEntry(BitConverter.GetBytes(i)), 
				    new DatabaseEntry(BitConverter.GetBytes(i)),
				    txn);
			txn.Commit();
			Console.WriteLine(
			    "Master: Write something new, data #2.");
			Console.WriteLine("Master: Wait for client to read #2...");

			// Get the stats.
			repStats = env.ReplicationSystemStats(true);
			env.PrintReplicationSystemStats();
			Assert.LessOrEqual(0, repStats.AppliedTransactions);
			Assert.LessOrEqual(0, repStats.AwaitedLSN.LogFileNumber);
			Assert.LessOrEqual(0, repStats.AwaitedLSN.Offset);
			Assert.LessOrEqual(0, repStats.AwaitedPage);
			Assert.LessOrEqual(0, repStats.BadGenerationMessages);
			Assert.LessOrEqual(0, repStats.BulkBufferFills);
			Assert.LessOrEqual(0, repStats.BulkBufferOverflows);
			Assert.LessOrEqual(0, repStats.BulkBufferTransfers);
			Assert.LessOrEqual(0, repStats.BulkRecordsStored);
			Assert.LessOrEqual(0, repStats.ClientServiceRequests);
			Assert.LessOrEqual(0, repStats.ClientServiceRequestsMissing);
			Assert.IsInstanceOf(typeof(bool), repStats.ClientStartupComplete);
			Assert.AreEqual(2, repStats.CurrentElectionGenerationNumber);
			Assert.AreEqual(1, repStats.CurrentGenerationNumber);
			Assert.LessOrEqual(0, repStats.CurrentQueuedLogRecords);
			Assert.LessOrEqual(0, repStats.CurrentWinner);
			Assert.LessOrEqual(0, repStats.CurrentWinnerMaxLSN.LogFileNumber);
			Assert.LessOrEqual(0, repStats.CurrentWinnerMaxLSN.Offset);
			Assert.LessOrEqual(0, repStats.DuplicateLogRecords);
			Assert.LessOrEqual(0, repStats.DuplicatePages);
			Assert.LessOrEqual(0, repStats.DupMasters);
			Assert.LessOrEqual(0, repStats.ElectionGenerationNumber);
			Assert.LessOrEqual(0, repStats.ElectionPriority);
			Assert.LessOrEqual(0, repStats.Elections);
			Assert.LessOrEqual(0, repStats.ElectionStatus);
			Assert.LessOrEqual(0, repStats.ElectionsWon);
			Assert.LessOrEqual(0, repStats.ElectionTiebreaker);
			Assert.LessOrEqual(0, repStats.ElectionTimeSec);
			Assert.LessOrEqual(0, repStats.ElectionTimeUSec);
			Assert.AreEqual(repStats.EnvID, repStats.MasterEnvID);
			Assert.LessOrEqual(0, repStats.EnvPriority);
			Assert.LessOrEqual(0, repStats.FailedMessageSends);
			Assert.LessOrEqual(0, repStats.ForcedRerequests);
			Assert.LessOrEqual(0, repStats.IgnoredMessages);
			Assert.LessOrEqual(0, repStats.MasterChanges);
			Assert.LessOrEqual(0, repStats.MasterEnvID);
			Assert.LessOrEqual(0, repStats.MaxLeaseSec);
			Assert.LessOrEqual(0, repStats.MaxLeaseUSec);
			Assert.LessOrEqual(0, repStats.MaxPermanentLSN.Offset);
			Assert.LessOrEqual(0, repStats.MaxQueuedLogRecords);
			Assert.LessOrEqual(0, repStats.MessagesSent);
			Assert.LessOrEqual(0, repStats.MissedLogRecords);
			Assert.LessOrEqual(0, repStats.MissedPages);
			Assert.LessOrEqual(0, repStats.NewSiteMessages);
			Assert.LessOrEqual(repStats.MaxPermanentLSN.LogFileNumber, 
			    repStats.NextLSN.LogFileNumber);
			if (repStats.MaxPermanentLSN.LogFileNumber ==
			    repStats.NextLSN.LogFileNumber)
				Assert.Less(repStats.MaxPermanentLSN.Offset,
				    repStats.NextLSN.Offset);
			Assert.LessOrEqual(0, repStats.NextPage);
			Assert.LessOrEqual(0, repStats.Outdated);
			Assert.LessOrEqual(0, repStats.QueuedLogRecords);
			Assert.LessOrEqual(0, repStats.ReceivedLogRecords);
			Assert.LessOrEqual(0, repStats.ReceivedMessages);
			Assert.LessOrEqual(0, repStats.ReceivedPages);
			Assert.LessOrEqual(0, repStats.RegisteredSites);
			Assert.LessOrEqual(0, repStats.RegisteredSitesNeeded);
			Assert.LessOrEqual(0, repStats.Sites);
			Assert.LessOrEqual(0, repStats.StartSyncMessagesDelayed);
			Assert.AreEqual(2, repStats.Status);
			Assert.LessOrEqual(0, repStats.Throttled);
			Assert.LessOrEqual(0, repStats.Votes);	

			// Get replication manager statistics.
			RepMgrStats repMgrStats = env.RepMgrSystemStats(true);
			Assert.AreEqual(0, repMgrStats.AutoTakeovers);
			Assert.LessOrEqual(0, repMgrStats.DroppedConnections);
			Assert.LessOrEqual(0, repMgrStats.DroppedMessages);
			Assert.LessOrEqual(0, repMgrStats.ElectionThreads);
			Assert.LessOrEqual(0, repMgrStats.FailedConnections);
			Assert.LessOrEqual(0, repMgrStats.FailedMessages);
			Assert.Less(0, repMgrStats.MaxElectionThreads);
			Assert.LessOrEqual(0, repMgrStats.QueuedMessages);

			// Print them out.
			env.PrintRepMgrSystemStats();

			// Wait until client has finished reading.
			masterCloseSignal.WaitOne();
			Console.WriteLine("Master: Leave as well.");

			// Close all.
			db.Close(false);
			env.LogFlush();
			env.Close();
		}

		public void Client()
		{
			string home = testHome + "/Client";
			Configuration.ClearDir(home);

			clientStartSignal.WaitOne();
			Console.WriteLine("Client: Join the replication");

			// Open a environment.
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
			cfg.LockTimeout = 50000;
			cfg.RepSystemCfg = new ReplicationConfig();
			cfg.RepSystemCfg.RepmgrSitesConfig.Add(new DbSiteConfig());
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Host = "127.0.0.1";
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Port = ports[1];
			cfg.RepSystemCfg.RepmgrSitesConfig[0].LocalSite = true;
			cfg.RepSystemCfg.Priority = 10;
			cfg.RepSystemCfg.RepmgrSitesConfig.Add(new DbSiteConfig());
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Host = "127.0.0.1";
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Port = ports[0];
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Helper = true;
			cfg.EventNotify = new EventNotifyDelegate(stuffHappened);
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    home, cfg);

			// Start a client site with replication manager.
			env.RepMgrStartClient(3, false);

			// Leave enough time to sync.
			Thread.Sleep(20000);

			// Open database.
			BTreeDatabaseConfig dbConfig = 
			    new BTreeDatabaseConfig();
			dbConfig.Creation = CreatePolicy.NEVER;
			dbConfig.AutoCommit = true;
			dbConfig.Env = env;
			dbConfig.PageSize = 512;
			BTreeDatabase db = BTreeDatabase.Open("rep.db", 
			    dbConfig);

			// Write data into database.
			Console.WriteLine("Client: Start reading data #1.");
			for (int i = 0; i < 5; i++)
				db.GetBoth(new DatabaseEntry(
				    BitConverter.GetBytes(i)), new DatabaseEntry(
				    BitConverter.GetBytes(i)));

			// Leave sometime for client to read new data from master.
			Thread.Sleep(20000);

			/* 
			 * Read the data. All data exists in master site should
			 * appear in the client site.
			 */ 
			Console.WriteLine("Client: Start reading data #2.");
			for (int i = 10; i < 15; i++)
				db.GetBoth(new DatabaseEntry(
				    BitConverter.GetBytes(i)), new DatabaseEntry(
				    BitConverter.GetBytes(i)));

			// Get the latest replication subsystem statistics.
			ReplicationStats repStats = env.ReplicationSystemStats();
			Assert.IsTrue(repStats.ClientStartupComplete);
			Assert.LessOrEqual(0, repStats.DuplicateLogRecords);
			Assert.LessOrEqual(0, repStats.EnvID);
			Assert.LessOrEqual(0, repStats.NextPage);
			Assert.LessOrEqual(0, repStats.ReceivedPages);
			Assert.AreEqual(1, repStats.Status);

			// Close all.
			db.Close(false);
			env.LogFlush();
			env.Close();
			Console.WriteLine(
			    "Client: All data is read. Leaving the replication");

			// The master is closed after client's close.
			masterCloseSignal.Set();
		}

		private void stuffHappened(NotificationEvent eventCode, byte[] info)
		{
			switch (eventCode)
			{
				case NotificationEvent.REP_AUTOTAKEOVER_FAILED:
				Console.WriteLine("Event: REP_AUTOTAKEOVER_FAILED");
					break;
				case NotificationEvent.REP_CLIENT:
				Console.WriteLine("Event: CLIENT");
					break;
				case NotificationEvent.REP_CONNECT_BROKEN:
					Console.WriteLine("Event: REP_CONNECT_BROKEN");
					break;
				case NotificationEvent.REP_CONNECT_ESTD:
					Console.WriteLine("Event: REP_CONNECT_ESTD");
					break;
				case NotificationEvent.REP_CONNECT_TRY_FAILED:
					Console.WriteLine("Event: REP_CONNECT_TRY_FAILED");
					break;
				case NotificationEvent.REP_MASTER:
					Console.WriteLine("Event: MASTER");
					break;
				case NotificationEvent.REP_NEWMASTER:
					electionDone = true;
					Console.WriteLine("Event: NEWMASTER");
					break;
				case NotificationEvent.REP_LOCAL_SITE_REMOVED:
					Console.WriteLine("Event: REP_LOCAL_SITE_REMOVED");
					break;
				case NotificationEvent.REP_SITE_ADDED:
					Console.WriteLine("Event: REP_SITE_ADDED");
					break;
				case NotificationEvent.REP_SITE_REMOVED:
					Console.WriteLine("Event: REP_SITE_REMOVED");
					break;
				case NotificationEvent.REP_STARTUPDONE:
					startUpDone++;
					Console.WriteLine("Event: REP_STARTUPDONE");
					break;
				case NotificationEvent.REP_PERM_FAILED:
					Console.WriteLine("Event: Insufficient Acks.");
					break;
				default:
					Console.WriteLine("Event: {0}", eventCode);
					break;
			}
		}

		[Test]
		public void TestElection()
		{
			testName = "TestElection";
			SetUpTest(true);

			// Initialize ports for one master, and three clients.
			ports.Clear();
			AvailablePorts portGen = new AvailablePorts();
			ports.Insert(0, portGen.Current);
			portGen.MoveNext();
			ports.Insert(1, portGen.Current);
			portGen.MoveNext();
			ports.Insert(2, portGen.Current);
			portGen.MoveNext();
			ports.Insert(3, portGen.Current);

			/*
			 * The *Signals are used as triggers to control the test
			 * work flow.  The client1StartSignal would be set once
			 * the master is ready and notify the client1 to start.
			 * The client2StartSignal would be set once the client1
			 * is ready and notify the client2 to start.  The 
			 * client3StartSignal is similar.  The masterLeaveSignal
			 * would be set once the last client client3 is ready
			 * and notify the master to leave.  The 
			 * clientsElectionSignal would be set when the master
			 * has already left and new master would be elected.
			 */ 
			client1StartSignal = new AutoResetEvent(false);
			client2StartSignal = new AutoResetEvent(false);
			client3StartSignal = new AutoResetEvent(false);
			clientsElectionSignal = new AutoResetEvent(false);
			masterLeaveSignal = new AutoResetEvent(false);
			// Count startup done event.
			startUpDone = 0;
			// Whether finish election.
			electionDone = false;

			Thread thread1 = new Thread(
			    new ThreadStart(UnstableMaster));
			Thread thread2 = new Thread(
			    new ThreadStart(StableClient1));
			Thread thread3 = new Thread(
			    new ThreadStart(StableClient2));
			Thread thread4 = new Thread(
			    new ThreadStart(StableClient3));

			try {
				thread1.Start();

				/*
				 * After start up is done at master, start
				 * client1.  Wait for the signal for 50000 ms. 
				 */
				if (!client1StartSignal.WaitOne(50000))
					throw new TestException();
				thread2.Start();

				 // Ready to start client2.
				if (!client2StartSignal.WaitOne(50000))
					throw new TestException();
				thread3.Start();

				// Ready to start client3.
				if (!client3StartSignal.WaitOne(50000))
					throw new TestException();
				thread4.Start();

				thread4.Join();
				thread3.Join();
				thread2.Join();
				thread1.Join();

				Assert.IsTrue(electionDone);
			} catch (TestException e) {
				if (thread1.IsAlive)
					thread1.Abort();
				if (thread2.IsAlive)
					thread2.Abort();
				if (thread3.IsAlive)
					thread3.Abort();
				if (thread4.IsAlive)
					thread4.Abort();
				throw e;
			} finally {
				client1StartSignal.Close();
				client2StartSignal.Close();
				client3StartSignal.Close();
				clientsElectionSignal.Close();
				masterLeaveSignal.Close();
			}
		}

		public void UnstableMaster()
		{
			string home = testHome + "/UnstableMaster";
			Configuration.ClearDir(home);

			// Open environment with replication configuration.
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
			cfg.RepSystemCfg.RepmgrSitesConfig.Add(
			    new DbSiteConfig());
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Host = "127.0.0.1";
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Port = ports[0];
			cfg.RepSystemCfg.RepmgrSitesConfig[0].LocalSite = true;
			cfg.RepSystemCfg.Priority = 200;
			cfg.RepSystemCfg.ElectionRetry = 10;
			cfg.RepSystemCfg.RepMgrAckPolicy = AckPolicy.ALL;
			cfg.EventNotify = new EventNotifyDelegate(stuffHappened);
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    home, cfg);
			env.DeadlockResolution = DeadlockPolicy.DEFAULT;

			try {
				// Start as master site.
				env.RepMgrStartMaster(3);
				Console.WriteLine(
				    "Master: Create a new repmgr group");

				// Client1 could start now.
				client1StartSignal.Set();

				// Wait for initialization of all clients.
				if (!masterLeaveSignal.WaitOne(50000))
					throw new TestException();

				// Check remote sites are valid.
				foreach (RepMgrSite site in 
				    env.RepMgrRemoteSites) {
					Assert.AreEqual("127.0.0.1",
					    site.Address.Host);
					Assert.IsTrue(ports.Contains(
					    site.Address.Port));
				}

				// Close the current master.
				Console.WriteLine("Master: Unexpected leave.");
				env.LogFlush();
			} catch(Exception e) {
				Console.WriteLine(e.Message);
			} finally {
				/*
				 * Clean up electionDone and startUpDone to 
				 * check election for new master and start-up
				 * done on clients. 
				 */ 
				electionDone = false;
				startUpDone = 0;

				env.Close();

				/*
				 * Need to set signals for three times, each
				 * site would wait for one.
				 */ 
				for (int i = 0; i < 3; i++)
					clientsElectionSignal.Set();
			}
		}

		public void StableClient1()
		{
			StableClient(testHome + "/StableClient1", 1, ports[1],
			    10, ports[0], 0, client2StartSignal);
		}

		public void StableClient2()
		{
			StableClient(testHome + "/StableClient2", 2, ports[2],
			    20, ports[0], ports[3], client3StartSignal);
		}

		public void StableClient3()
		{
			StableClient(testHome + "/StableClient3", 3, ports[3],
			    80, ports[0], ports[2], masterLeaveSignal);
		}

		public void StableClient(string home, int clientIdx,
		    uint localPort, uint priority, 
		    uint helperPort, uint peerPort, 
		    EventWaitHandle notifyHandle)
		{
			int timeout = 0;

			Configuration.ClearDir(home);

			Console.WriteLine(
			    "Client{0}: Join the replication", clientIdx);

			DatabaseEnvironmentConfig cfg =
			    new DatabaseEnvironmentConfig();
			cfg.Create = true;
			cfg.FreeThreaded = true;
			cfg.MPoolSystemCfg = new MPoolConfig();
			cfg.MPoolSystemCfg.CacheSize = 
			    new CacheInfo(0, 20485760, 1);
			cfg.RunRecovery = true;
			cfg.TxnNoSync = true;
			cfg.UseLocking = true;
			cfg.UseMPool = true;
			cfg.UseTxns = true;
			cfg.UseReplication = true;
			cfg.UseLogging = true;
			cfg.RepSystemCfg = new ReplicationConfig();
			cfg.RepSystemCfg.RepmgrSitesConfig.Add(
			    new DbSiteConfig());
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Host = "127.0.0.1";
			cfg.RepSystemCfg.RepmgrSitesConfig[0].Port = localPort;
			cfg.RepSystemCfg.RepmgrSitesConfig[0].LocalSite = true;
			cfg.RepSystemCfg.Priority = priority;
			cfg.RepSystemCfg.RepmgrSitesConfig.Add(
			    new DbSiteConfig());
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Host = "127.0.0.1";
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Port = helperPort;
			cfg.RepSystemCfg.RepmgrSitesConfig[1].Helper = true;
			cfg.RepSystemCfg.ElectionRetry = 100;
			cfg.RepSystemCfg.RepMgrAckPolicy = AckPolicy.NONE;
			cfg.EventNotify = new EventNotifyDelegate(stuffHappened);
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    home, cfg);
			env.DeadlockResolution = DeadlockPolicy.DEFAULT;

			try {
				// Start the client.
				Assert.AreEqual(clientIdx - 1, startUpDone);
				env.RepMgrStartClient(3, false);
				while (startUpDone < clientIdx) {
					if (timeout > 10)
						throw new TestException();
					timeout++;
					Thread.Sleep(1000);
				}

				ReplicationStats repStats =
				    env.ReplicationSystemStats();
				Assert.LessOrEqual(0, repStats.Elections);
				Assert.LessOrEqual(0, 
				    repStats.ElectionTiebreaker);
				Assert.LessOrEqual(0, repStats.ElectionTimeSec +
				    repStats.ElectionTimeUSec);
				Assert.LessOrEqual(0, repStats.MasterChanges);
				Assert.LessOrEqual(0, repStats.NewSiteMessages);
				Assert.LessOrEqual(0, 
				    repStats.ReceivedLogRecords);
				Assert.LessOrEqual(0, repStats.ReceivedMessages);
				Assert.LessOrEqual(0, repStats.ReceivedPages);
				Assert.GreaterOrEqual(clientIdx + 1,
				    repStats.RegisteredSitesNeeded);
				Assert.LessOrEqual(0, repStats.Sites);

				// Notify the next event.
				notifyHandle.Set();

				// Wait for master's leave.
				if (!clientsElectionSignal.WaitOne(50000))
					throw new TestException();

				timeout = 0;
				while (!electionDone) {
					if (timeout > 10)
						throw new TestException();
					timeout++;
					Thread.Sleep(1000);
				}

				env.LogFlush();

				timeout = 0;
				// Start up done event happens on client.
				while (startUpDone < 2) {
					if (timeout > 10)
						throw new TestException();
					timeout++;
					Thread.Sleep(1000);
				}
				Thread.Sleep((int)priority * 100);
			} catch (Exception e) {
				Console.WriteLine(e.Message);
			} finally {
				env.Close();
				Console.WriteLine(
				    "Client{0}: Leaving the replication",
				    clientIdx);
			}
		}

		[Test]
		public void TestAckPolicy()
		{
			testName = "TestAckPolicy";
			SetUpTest(true);

			SetRepMgrAckPolicy(testHome + "_ALL", AckPolicy.ALL);
			SetRepMgrAckPolicy(testHome + "_ALL_AVAILABLE",
			    AckPolicy.ALL_AVAILABLE);
			SetRepMgrAckPolicy(testHome + "_ALL_PEERS",
			    AckPolicy.ALL_PEERS);
			SetRepMgrAckPolicy(testHome + "_NONE", 
			    AckPolicy.NONE);
			SetRepMgrAckPolicy(testHome + "_ONE", 
			    AckPolicy.ONE);
			SetRepMgrAckPolicy(testHome + "_ONE_PEER",
			    AckPolicy.ONE_PEER);
			SetRepMgrAckPolicy(testHome + "_QUORUM", 
			    AckPolicy.QUORUM);
			SetRepMgrAckPolicy(testHome + "_NULL", null);
		}

		public void SetRepMgrAckPolicy(string home, AckPolicy policy)
		{
			Configuration.ClearDir(home);
			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseLocking = true;
			envConfig.UseLogging = true;
			envConfig.UseMPool = true;
			envConfig.UseReplication = true;
			envConfig.UseTxns = true;
			DatabaseEnvironment env = DatabaseEnvironment.Open(
			    home, envConfig);
			if (policy != null)
			{
				env.RepMgrAckPolicy = policy;
				Assert.AreEqual(policy, env.RepMgrAckPolicy);
			}
			env.Close();
		}

		[Test]
		public void TestLocalSite() {
			testName = "TestLocalSite";
			SetUpTest(true);
			Configuration.ClearDir(testHome);
			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseLocking = true;
			envConfig.UseLogging = true;
			envConfig.UseMPool = true;
			envConfig.UseReplication = true;
			envConfig.UseTxns = true;
			ReplicationHostAddress addr =
			    new ReplicationHostAddress("localhost:6060");
			ReplicationConfig repCfg = new ReplicationConfig();
			DbSiteConfig dbSiteConfig = new DbSiteConfig();
			dbSiteConfig.Host = addr.Host;
			dbSiteConfig.Port = addr.Port;
			dbSiteConfig.LocalSite = true;
			repCfg.RepmgrSitesConfig.Add(dbSiteConfig);
			envConfig.RepSystemCfg = repCfg;
			DatabaseEnvironment env =
			    DatabaseEnvironment.Open(testHome, envConfig);
			ReplicationHostAddress testAddr =
			    env.RepMgrLocalSite.Address;
			Assert.AreEqual(addr.Host, testAddr.Host);
			Assert.AreEqual(addr.Port, testAddr.Port);
			env.Close();
		}

		public class AvailablePorts : IEnumerable<uint>
		{
			private List<uint> availablePort = new List<uint>();
			private List<uint> usingPort = new List<uint>();
			private int position = -1;

			public AvailablePorts()
			{
				// Initial usingPort array with all TCP ports being used.
				IPGlobalProperties properties = IPGlobalProperties.GetIPGlobalProperties(); 
				foreach (IPEndPoint point in properties.GetActiveTcpListeners())
					usingPort.Add((uint)point.Port);

				// Initial availablePort array with available ports ranging from 10000 to 15000.
				for (uint i = 10000; i <= 15000; i++)
				{
					if (!usingPort.Contains(i))
						availablePort.Add(i);
				}
			}

			IEnumerator IEnumerable.GetEnumerator() { return GetEnumerator(); }

			public IEnumerator<uint> GetEnumerator()
			{
				while (MoveNext())
					yield return availablePort[position];
			}

			public bool MoveNext()
			{
				position++;
				return position < availablePort.Count;
			}

			public uint Current
			{
				get {
					if (position == -1)
						position = 0;
					return availablePort[position];
				}
			}
		}

		[Test]
		public void TestSiteRepConfig()
		{
			testName = "TestSiteRepConfig";
			SetUpTest(true);
			Configuration.ClearDir(testHome);
			DatabaseEnvironmentConfig envConfig =
			    new DatabaseEnvironmentConfig();
			envConfig.Create = true;
			envConfig.UseLocking = true;
			envConfig.UseLogging = true;
			envConfig.UseMPool = true;
			envConfig.UseReplication = true;
			envConfig.UseTxns = true;
			ReplicationHostAddress addr =
			    new ReplicationHostAddress("localhost:6060");
			ReplicationConfig repCfg = new ReplicationConfig();
			DbSiteConfig dbSiteConfig = new DbSiteConfig();
			dbSiteConfig.Host = addr.Host;
			dbSiteConfig.Port = addr.Port;
			dbSiteConfig.LocalSite = true;
			repCfg.RepmgrSitesConfig.Add(dbSiteConfig);

			// DatabaseEnvironment.RepInMemory defaults to false.
			envConfig.RepSystemCfg = repCfg;
			DatabaseEnvironment env =
			    DatabaseEnvironment.Open(testHome, envConfig);
			Assert.AreEqual(false, env.RepInMemory);
			env.Close();
			Assert.Less(0,
			    Directory.GetFiles(testHome, "__db.rep.*").Length);

			// Set RepInMemory as true.
			Configuration.ClearDir(testHome);
			repCfg.InMemory = true;
			envConfig.RepSystemCfg = repCfg;
			env = DatabaseEnvironment.Open(testHome, envConfig);
			Assert.AreEqual(true, env.RepInMemory);
			env.Close();
			Assert.AreEqual(0,
			    Directory.GetFiles(testHome, "__db.rep.*").Length);

			// Set RepInMemory as false.
			Configuration.ClearDir(testHome);
			repCfg.InMemory = false;
			envConfig.RepSystemCfg = repCfg;
			env = DatabaseEnvironment.Open(testHome, envConfig);
			Assert.AreEqual(false, env.RepInMemory);
			env.Close();
			Assert.Less(0, 
			    Directory.GetFiles(testHome, "__db.rep.*").Length);
		}
	}
}
