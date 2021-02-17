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

namespace CsharpAPITest {
    [TestFixture]
    public class LockTest  : CSharpTestFixture
    {
        DatabaseEnvironment testLockStatsEnv;
        BTreeDatabase testLockStatsDb;
        int DeadlockDidPut = 0;

        [TestFixtureSetUp]
        public void SetUpTestFixture() {
            testFixtureName = "LockTest";
            base.SetUpTestfixture();
        }

        [Test]
        public void TestLockStats() {
            testName = "TestLockStats";
            SetUpTest(true);

            // Configure locking subsystem.
            LockingConfig lkConfig = new LockingConfig();
            lkConfig.MaxLockers = 60;
            lkConfig.MaxLocks = 50;
            lkConfig.MaxObjects = 70;
            lkConfig.Partitions = 20;
            lkConfig.DeadlockResolution = DeadlockPolicy.DEFAULT;

            // Configure and open environment.
            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.FreeThreaded = true;
            envConfig.LockSystemCfg = lkConfig;
            envConfig.LockTimeout = 1000;
            envConfig.MPoolSystemCfg = new MPoolConfig();
            envConfig.MPoolSystemCfg.CacheSize = new CacheInfo(0, 104800, 1);
            envConfig.NoLocking = false;
            envConfig.TxnTimeout = 2000;
            envConfig.UseLocking = true;
            envConfig.UseMPool = true;
            envConfig.UseTxns = true;
            DatabaseEnvironment env =
                DatabaseEnvironment.Open(testHome, envConfig);

            // Get and confirm locking subsystem statistics.
            LockStats stats = env.LockingSystemStats();
            env.PrintLockingSystemStats(true, true);
            Assert.AreEqual(0, stats.AllocatedLockers);
            Assert.AreNotEqual(0, stats.AllocatedLocks);
            Assert.AreNotEqual(0, stats.AllocatedObjects);
            Assert.AreEqual(0, stats.InitLockers);
            Assert.AreNotEqual(0, stats.InitLocks);
            Assert.AreNotEqual(0, stats.InitObjects);
            Assert.AreEqual(0, stats.LastAllocatedLockerID);
            Assert.AreEqual(0, stats.LockConflictsNoWait);
            Assert.AreEqual(0, stats.LockConflictsWait);
            Assert.AreEqual(0, stats.LockDeadlocks);
            Assert.AreEqual(0, stats.LockDowngrades);
            Assert.AreEqual(0, stats.LockerNoWait);
            Assert.AreEqual(0, stats.Lockers);
            Assert.AreEqual(0, stats.LockerWait);
            Assert.AreEqual(9, stats.LockModes);
            Assert.AreEqual(0, stats.LockPuts);
            Assert.AreEqual(0, stats.LockRequests);
            Assert.AreEqual(0, stats.Locks);
            Assert.AreEqual(0, stats.LockSteals);
            Assert.AreEqual(1000, stats.LockTimeoutLength);
            Assert.AreEqual(0, stats.LockTimeouts);
            Assert.AreEqual(0, stats.LockUpgrades);
            Assert.AreEqual(0, stats.MaxBucketLength);
            Assert.AreEqual(0, stats.MaxLockers);
            Assert.AreEqual(60, stats.MaxLockersInTable);
            Assert.AreEqual(0, stats.MaxLocks);
            Assert.AreEqual(0, stats.MaxLocksInBucket);
            Assert.AreEqual(50, stats.MaxLocksInTable);
            Assert.AreEqual(0, stats.MaxLockSteals);
            Assert.AreEqual(0, stats.MaxObjects);
            Assert.AreEqual(0, stats.MaxObjectsInBucket);
            Assert.AreEqual(70, stats.MaxObjectsInTable);
            Assert.AreEqual(0, stats.MaxObjectSteals);
            Assert.AreEqual(0, stats.MaxPartitionLockNoWait);
            Assert.AreEqual(0, stats.MaxPartitionLockWait);
            Assert.AreNotEqual(0, stats.MaxUnusedID);
            Assert.AreEqual(20, stats.nPartitions);
            Assert.AreEqual(0, stats.ObjectNoWait);
            Assert.AreEqual(0, stats.Objects);
            Assert.AreEqual(0, stats.ObjectSteals);
            Assert.AreEqual(0, stats.ObjectWait);
            Assert.LessOrEqual(0, stats.PartitionLockNoWait);
            Assert.AreEqual(0, stats.PartitionLockWait);
            Assert.Less(0, stats.RegionNoWait);
            Assert.AreNotEqual(0, stats.RegionSize);
            Assert.AreEqual(0, stats.RegionWait);
            Assert.AreNotEqual(0, stats.TableSize);
            Assert.AreEqual(2000, stats.TxnTimeoutLength);
            Assert.AreEqual(0, stats.TxnTimeouts);

            env.PrintLockingSystemStats();

            Transaction txn = env.BeginTransaction();
            BTreeDatabaseConfig dbConfig = new BTreeDatabaseConfig();
            dbConfig.Creation = CreatePolicy.IF_NEEDED;
            dbConfig.Env = env;
                dbConfig.FreeThreaded = true;
            BTreeDatabase db = BTreeDatabase.Open(
                testName + ".db", dbConfig, txn);
            txn.Commit();

            testLockStatsEnv = env;
            testLockStatsDb = db;

            // Use some locks, to ensure  the stats work when populated.
            txn = testLockStatsEnv.BeginTransaction();
            for (int i = 0; i < 500; i++)
            {
                testLockStatsDb.Put(
                    new DatabaseEntry(BitConverter.GetBytes(i)),
                    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes(
                    Configuration.RandomString(i))), txn);
                testLockStatsDb.Sync();
            }
            txn.Commit();

            env.PrintLockingSystemStats();
            stats = env.LockingSystemStats();
            Assert.Less(0, stats.LastAllocatedLockerID);
            Assert.Less(0, stats.LockDowngrades);
            Assert.LessOrEqual(0, stats.LockerNoWait);
            Assert.Less(0, stats.Lockers);
            Assert.LessOrEqual(0, stats.LockerWait);
            Assert.Less(0, stats.LockPuts);
            Assert.Less(0, stats.LockRequests);
            Assert.Less(0, stats.Locks);
            Assert.LessOrEqual(0, stats.LockSteals);
            Assert.LessOrEqual(0, stats.LockTimeouts);
            Assert.LessOrEqual(0, stats.LockUpgrades);
            Assert.Less(0, stats.MaxBucketLength);
            Assert.Less(0, stats.MaxLockers);
            Assert.Less(0, stats.MaxLocks);
            Assert.Less(0, stats.MaxLocksInBucket);
            Assert.LessOrEqual(0, stats.MaxLockSteals);
            Assert.Less(0, stats.MaxObjects);
            Assert.Less(0, stats.MaxObjectsInBucket);
            Assert.LessOrEqual(0, stats.MaxObjectSteals);
            Assert.LessOrEqual(0, stats.MaxPartitionLockNoWait);
            Assert.LessOrEqual(0, stats.MaxPartitionLockWait);
            Assert.Less(0, stats.MaxUnusedID);
            Assert.LessOrEqual(0, stats.ObjectNoWait);
            Assert.Less(0, stats.Objects);
            Assert.LessOrEqual(0, stats.ObjectSteals);
            Assert.LessOrEqual(0, stats.ObjectWait);
            Assert.Less(0, stats.PartitionLockNoWait);
            Assert.LessOrEqual(0, stats.PartitionLockWait);
            Assert.Less(0, stats.RegionNoWait);
            Assert.LessOrEqual(0, stats.RegionWait);
            Assert.LessOrEqual(0, stats.TxnTimeouts);

            // Force a deadlock to ensure the stats work.
            txn = testLockStatsEnv.BeginTransaction();
            testLockStatsDb.Put(new DatabaseEntry(BitConverter.GetBytes(10)),
                new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes(
                Configuration.RandomString(200))), txn);

            Thread thread1 = new Thread(GenerateDeadlock);
            thread1.Start();
            while (DeadlockDidPut == 0)
                Thread.Sleep(10);

            try
            {
                testLockStatsDb.Get(new DatabaseEntry(
                    BitConverter.GetBytes(100)), txn);
            }
            catch (DeadlockException) { }
            // Abort unconditionally - we don't care about the transaction
            txn.Abort();
            thread1.Join();

            stats = env.LockingSystemStats();
            Assert.Less(0, stats.LockConflictsNoWait);
            Assert.LessOrEqual(0, stats.LockConflictsWait);

            db.Close(); 
            env.Close();
        }

        public void GenerateDeadlock()
        {
            Transaction txn = testLockStatsEnv.BeginTransaction();
            try
            {
                testLockStatsDb.Put(
                    new DatabaseEntry(BitConverter.GetBytes(100)),
                    new DatabaseEntry(ASCIIEncoding.ASCII.GetBytes(
                    Configuration.RandomString(200))), txn);
                DeadlockDidPut = 1;
                testLockStatsDb.Get(new DatabaseEntry(
                BitConverter.GetBytes(10)), txn);

            }
            catch (DeadlockException) { }
            // Abort unconditionally - we don't care about the transaction
            txn.Abort();
        }

        public static void LockingEnvSetUp(string testHome,
            string testName, out DatabaseEnvironment env,
            uint maxLock, uint maxLocker, uint maxObject,
            uint partition) {
            // Configure env and locking subsystem.
            LockingConfig lkConfig = new LockingConfig();
            /*
             * If the maximum number of locks/lockers/objects
             * is given, then the LockingConfig is set. Unless, 
             * it is not set to any value.  
             */
            if (maxLock != 0)
                lkConfig.MaxLocks = maxLock;
            if (maxLocker != 0)
                lkConfig.MaxLockers = maxLocker;
            if (maxObject != 0)
                lkConfig.MaxObjects = maxObject;
            if (partition != 0)
                lkConfig.Partitions = partition;

            DatabaseEnvironmentConfig envConfig =
                new DatabaseEnvironmentConfig();
            envConfig.Create = true;
            envConfig.LockSystemCfg = lkConfig;
            envConfig.UseLocking = true;
            envConfig.ErrorPrefix = testName;

            env = DatabaseEnvironment.Open(testHome, envConfig);
        }
    }
}
