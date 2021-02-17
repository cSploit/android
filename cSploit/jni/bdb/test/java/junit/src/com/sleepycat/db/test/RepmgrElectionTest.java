/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 */

package com.sleepycat.db.test;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.After;
import org.junit.AfterClass;
import org.junit.Test;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.Vector;

import com.sleepycat.db.*;

public class RepmgrElectionTest extends EventHandlerAdapter
{
    static String address = "localhost";
    static int    basePort = 4242;
    static String baseDirName = "";
    File homedir;
    EnvironmentConfig envConfig;
    Environment dbenv;
    int masterIndex = -1;
    int maxLoopWait = 30;

    @BeforeClass public static void ClassInit() {
        TestUtils.loadConfig(null);
        baseDirName = TestUtils.BASETEST_DBDIR + "/TESTDIR";
    }

    @AfterClass public static void ClassShutdown() {
    }

    @Before public void PerTestInit()
        throws Exception {
    }

    @After public void PerTestShutdown()
        throws Exception {
        for(int j = 0; j < NUM_WORKER_SITES; j++) {
            TestUtils.removeDir(baseDirName + j);
        }
    }

    private static boolean lastSiteStarted = false;
    private static int NUM_WORKER_SITES = 5;
    @Test public void startConductor()
    {
        Vector<RepmgrElectionTest> workers =
          new Vector<RepmgrElectionTest>(NUM_WORKER_SITES);

        // Start all worker sites.
        for (int i = 0; i < NUM_WORKER_SITES; i++) {
            RepmgrElectionTest worker;
            worker = new RepmgrElectionTest(i, i == 0 ? -1 : 0);
            worker.run();
            workers.add(worker);
        }

        // Ensure that the first site is master.
        ReplicationStats rs = null;
        try {
            rs = workers.elementAt(0).dbenv.getReplicationStats(StatsConfig.DEFAULT);
        } catch (DatabaseException dbe) {
            fail("Unexpected database exception came from get replication stats." + dbe);
        }

        assertTrue(rs.getEnvId() == rs.getMaster());

        // Stop the master
        try {
            workers.elementAt(0).dbenv.close();
            workers.elementAt(0).dbenv = null;
            workers.elementAt(0).envConfig = null;
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came during shutdown." + dbe);
        }

        // Ensure the election is completed.
        try {
            java.lang.Thread.sleep(5000);
        } catch (InterruptedException ie) {} // Do nothing if interrupted.
 
        // Ensure that the site with priority = 75 is selected as new master.
        try {
            rs = workers.elementAt(1).dbenv.getReplicationStats(StatsConfig.DEFAULT);
        } catch (DatabaseException dbe) {
            fail("Unexpected database exception came from get replication stats." + dbe);
        }

       assertTrue(rs.getEnvId() == rs.getMaster());

        // Re-start original master using the new master as bootstrap helper.
        RepmgrElectionTest rejoin = new RepmgrElectionTest(0, 1);
        rejoin.run();
        workers.remove(0);
        workers.insertElementAt(rejoin, 0);
 
        // Close all the sites and remove their environment contents.
        for (int i = 0; i < NUM_WORKER_SITES; i++) {
            try {
                if (workers.elementAt(i).dbenv != null) {
                    workers.elementAt(i).dbenv.close();
                    workers.elementAt(i).dbenv = null;
                    workers.elementAt(i).envConfig = null;
                }
            } catch(DatabaseException dbe) {
                fail("Unexpected database exception came during shutdown." + dbe);
            }
        }
    }

    /*
     * Worker site implementation
     */
    private final static int priorities[] = {100, 75, 50, 50, 25};
    private int siteIndex;
    public RepmgrElectionTest() {
        // needed to comply with JUnit, since there is also another constructor.
    }
    RepmgrElectionTest(int siteIndex, int masterIndex) {
        this.siteIndex = siteIndex;
        this.masterIndex = masterIndex;
    }

    public void run() {
        String homedirName = baseDirName + siteIndex;
        TestUtils.removeDir(homedirName);

        try {
            homedir = new File(homedirName);
            homedir.mkdir();
        } catch (Exception e) {
            TestUtils.DEBUGOUT(2, "Warning: initialization had a problem creating a clean directory.\n" + e);
        }
        try {
            homedir = new File(homedirName);
        } catch (NullPointerException npe) {
            // can't really happen :)
        }
 
        TestUtils.DEBUGOUT(1, "Creating worker: " + siteIndex);

        envConfig = new EnvironmentConfig();
        envConfig.setErrorStream(TestUtils.getErrorStream());
        envConfig.setErrorPrefix("RepmgrElectionTest test("+siteIndex+")");
        envConfig.setAllowCreate(true);
        envConfig.setRunRecovery(true);
        envConfig.setThreaded(true);
        envConfig.setInitializeLocking(true);
        envConfig.setInitializeLogging(true);
        envConfig.setInitializeCache(true);
        envConfig.setTransactional(true);
        envConfig.setTxnNoSync(true);
        envConfig.setInitializeReplication(true);
        envConfig.setVerboseReplication(false);

        ReplicationManagerSiteConfig localConfig = new ReplicationManagerSiteConfig(address, basePort + siteIndex);
        localConfig.setLocalSite(true);
        envConfig.addReplicationManagerSite(localConfig);

        envConfig.setReplicationPriority(priorities[siteIndex]);
        envConfig.setEventHandler(this);
        envConfig.setReplicationManagerAckPolicy(ReplicationManagerAckPolicy.ALL);

        if (masterIndex >= 0) {
            // If we already have the master, then set it as the bootstrap helper,
            // otherwise, set local site as new master.
            ReplicationManagerSiteConfig remoteConfig =
                new ReplicationManagerSiteConfig(address, basePort + masterIndex);
            remoteConfig.setBootstrapHelper(true);
            envConfig.addReplicationManagerSite(remoteConfig);
        }

        try {
            dbenv = new Environment(homedir, envConfig);

        } catch(FileNotFoundException e) {
            fail("Unexpected FNFE in standard environment creation." + e);
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came from environment create." + dbe);
        }
 
        try {
            // If we do not have master, then set local site as new master.
            if(masterIndex == -1)
                dbenv.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_MASTER);
            else
                dbenv.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_CLIENT);
        } catch(DatabaseException dbe) {
            fail("Unexpected database exception came from replicationManagerStart." + dbe);
        }

        TestUtils.DEBUGOUT(1, "Started replication site: " + siteIndex);
        lastSiteStarted = true;

        try {
            java.lang.Thread.sleep(1000 * (1+ siteIndex));
        } catch (InterruptedException ie) {}

        if(masterIndex != -1) {
            // Wait for "Start-up done" for each client, then add next client.
            ReplicationStats rs = null;
            int i = 0;
            do {
                try {
                    java.lang.Thread.sleep(2000);
                } catch (InterruptedException e) {}

                try {
                    rs = dbenv.getReplicationStats(StatsConfig.DEFAULT);
                } catch (DatabaseException dbe) {
                    dbe.printStackTrace();
                    fail("Unexpected database exception came from getReplicationStats." + dbe);
                }
            } while (!rs.getStartupComplete() && i++ < maxLoopWait);
            assertTrue(rs.getStartupComplete());
        }
    }

        /*
         * End worker site implementation
         */
        public void handleRepMasterEvent() {
            TestUtils.DEBUGOUT(1, "Got a REP_MASTER message");
            TestUtils.DEBUGOUT(1, "My priority: " + priorities[siteIndex]);
        }

        public void handleRepClientEvent() {
            TestUtils.DEBUGOUT(1, "Got a REP_CLIENT message");         
        }

        public void handleRepNewMasterEvent() {
            TestUtils.DEBUGOUT(1, "Got a REP_NEW_MASTER message");
            TestUtils.DEBUGOUT(1, "My priority: " + priorities[siteIndex]);
        }
}
