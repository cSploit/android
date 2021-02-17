/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import com.sleepycat.db.CheckpointConfig;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.EventHandlerAdapter;
import com.sleepycat.db.ReplicationConfig;
import com.sleepycat.db.ReplicationHostAddress;
import com.sleepycat.db.ReplicationManagerAckPolicy;
import com.sleepycat.db.ReplicationManagerConnectionStatus;
import com.sleepycat.db.ReplicationManagerStartPolicy;
import com.sleepycat.db.ReplicationTimeoutType;
import com.sleepycat.db.ReplicationManagerStats;
import com.sleepycat.db.ReplicationManagerSiteConfig;
import com.sleepycat.db.ReplicationManagerSiteInfo;
import com.sleepycat.db.ReplicationStats;
import com.sleepycat.db.StatsConfig;
import com.sleepycat.db.VerboseConfig;

import static org.junit.Assert.*;
import java.io.File;

public class CurrentImpl implements TestMixedHeartbeats.Ops, TestReverseConnect.Ops {
    private Config config;
    private Environment[] envs = new Environment[2];
    private MyEventHandler[] monitors = new MyEventHandler[2];

    class MyEventHandler extends EventHandlerAdapter {
        private boolean done = false;
        private boolean panic = false;
        private int permFailCount = 0;
        private int newmasterCount = 0;
        private boolean connTryFailed = false;
		
        @Override
            synchronized public void handleRepStartupDoneEvent() {
                done = true;
                notifyAll();
            }

        @Override
            synchronized public void handleRepPermFailedEvent() {
                permFailCount++;
            }

        synchronized public int getPermFailCount() { return permFailCount; }
		
        @Override
            synchronized public void handlePanicEvent() {
                done = true;
                panic = true;
                notifyAll();
            }

        @Override synchronized public void handleRepNewMasterEvent(int eid) {
            newmasterCount++;
            notifyAll();
        }

        @Override
            synchronized public void handleRepConnectTryFailedEvent() {
                connTryFailed = true;
                notifyAll();
            }

        synchronized public int getNewmasterCount() {
            return newmasterCount;
        }

        synchronized void awaitNewmaster(Environment site, long deadline)
            throws Exception
        {
            long now;

            for (;;) {
                ReplicationStats s = site.getReplicationStats(StatsConfig.DEFAULT);

                // am I the master?
                // 
                if (s.getEnvId() == s.getMaster()) { break; }
                
                if ((now = System.currentTimeMillis()) >= deadline)
                    throw new Exception("aborted by timeout");
                long waitTime = deadline-now;
                wait(waitTime);
                if (panic)
                    throw new Exception("aborted by panic in DB");
            }
        }

        synchronized void awaitConnTryFailed() throws Exception {
            for (;;) {
                if (connTryFailed) { break; }
                wait();
                if (panic)
                    throw new Exception("aborted by panic in DB");
            }
        }
		
        synchronized void await() throws Exception {
            while (!done) { wait(); }
            if (panic)
                throw new Exception("aborted by panic in DB");
        }
    }

    public void setConfig(Config c) { config = c; }

    public void joinExistingClient(int site, boolean useHB) throws Exception {
        EnvironmentConfig ec = makeBasicConfig();

        int p = config.getMyPort(site);
        ReplicationManagerSiteConfig dbsite = new ReplicationManagerSiteConfig("localhost", p);
        dbsite.setLocalSite(true);
        dbsite.setLegacy(true);
        ec.addReplicationManagerSite(dbsite);

        p = config.getOtherPort(site);
        dbsite = new ReplicationManagerSiteConfig("localhost", p);
        dbsite.setLegacy(true);
        ec.addReplicationManagerSite(dbsite);

        MyEventHandler monitor = new MyEventHandler();
        monitors[site] = monitor;
        ec.setEventHandler(monitor);
        File clientDir = new File(config.getBaseDir(), "dir" + site);
        assertTrue(clientDir.exists());
        Environment client = new Environment(clientDir, ec);
        client.setReplicationConfig(ReplicationConfig.STRICT_2SITE, false);

        if (useHB) {
            client.setReplicationTimeout(ReplicationTimeoutType.HEARTBEAT_SEND,
                                         3000000);
            client.setReplicationTimeout(ReplicationTimeoutType.HEARTBEAT_MONITOR,
                                         6000000);
        }

        envs[site] = client;
        client.setReplicationTimeout(ReplicationTimeoutType.CONNECTION_RETRY,
                                     1000000); // be impatient
        client.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_CLIENT);
        monitor.await();

        assertTrue(client.getReplicationStats(StatsConfig.DEFAULT).getStartupComplete());
    }

    /**
     * Passively waits to be chosen as new master in an election.
     */
    public void becomeMaster(int siteId) throws Exception {
        Environment site = envs[siteId];
        MyEventHandler monitor = (MyEventHandler)site.getConfig().getEventHandler();
        long deadline = System.currentTimeMillis() + 5000;
        monitor.awaitNewmaster(site, deadline);
    }

    public void checkpoint(int site) throws Exception {
        assertNull(envs[site]);

        EnvironmentConfig ec = makeBasicConfig();
        File dir = new File(config.getBaseDir(), "dir" + site);
        assertTrue(dir.exists());
        Environment e = new Environment(dir, ec);
        e.setReplicationConfig(ReplicationConfig.STRICT_2SITE, false);
        CheckpointConfig cc = new CheckpointConfig();
        cc.setForce(true);
        e.checkpoint(cc);
        e.close();
    }

    public MyStats getStats(int site) throws Exception {
        MyStats s = new MyStats();
        ReplicationStats rs = envs[site].getReplicationStats(StatsConfig.DEFAULT);
        s.egen = rs.getElectionGen();
        s.elections = rs.getElections();
        s.envId = rs.getEnvId();
        s.master = rs.getMaster();
        ReplicationManagerStats rms = envs[site].getReplicationManagerStats(StatsConfig.DEFAULT);
        s.permFailedCount = rms.getPermFailed();
        return s;
    }
        
    public void checkMinVersion() throws Exception {
        // For this test to make sense, we must be at version 4.7 or higher
        // 
        int major = Environment.getVersionMajor();
        int minor = Environment.getVersionMinor();
        assertTrue(major > 4 || (major == 4 && minor >= 7));
    }

    public boolean writeUpdates(int siteId, int txnCount) throws Exception {
        throw new Exception("not yet implemented");
    }

    public void shutDown(int siteId) throws Exception {
        envs[siteId].close();
        envs[siteId] = null;
    }

    public void restart(int siteId) throws Exception {
        EnvironmentConfig ec = makeBasicConfig();

        int p = config.getMyPort(siteId);
        ReplicationManagerSiteConfig dbsite = new ReplicationManagerSiteConfig("localhost", p);
        dbsite.setLocalSite(true);
        dbsite.setLegacy(true);
        ec.addReplicationManagerSite(dbsite);

        p = config.getOtherPort(siteId);
        dbsite = new ReplicationManagerSiteConfig("localhost", p);
        dbsite.setLegacy(true);
        ec.addReplicationManagerSite(dbsite);

        MyEventHandler monitor = new MyEventHandler();
        ec.setEventHandler(monitor);
        File clientDir = new File(config.getBaseDir(), "dir" + siteId);
        assertTrue(clientDir.exists());
        Environment client = new Environment(clientDir, ec);
        client.setReplicationConfig(ReplicationConfig.STRICT_2SITE, false);

        envs[siteId] = client;
        monitors[siteId] = monitor;
        // we want to make sure we don't retry from here after the
        // initial failure, because we want to make the old master
        // connect to us.
        client.setReplicationTimeout(ReplicationTimeoutType.CONNECTION_RETRY,
                                     Integer.MAX_VALUE);
        client.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_CLIENT);
    }

    public void awaitConnFailure(int site) throws Exception {
        monitors[site].awaitConnTryFailed();
    }

    public void verifyConnect(int site) throws Exception {
        int port = config.getOtherPort(site);
        int pollLimit = 10;
        for (int i=0; i<pollLimit; i++) {
            ReplicationManagerSiteInfo[] si = envs[site].getReplicationManagerSiteList();
            ReplicationManagerSiteInfo inf = null;
            for (ReplicationManagerSiteInfo in : si) {
                ReplicationHostAddress addr = in.addr;
                if (addr.port == port) {
                    inf = in;
                    break;
                }
            }
            assertNotNull("other port not in site list", inf);
            if (inf.getConnectionStatus() == ReplicationManagerConnectionStatus.CONNECTED) { return; }
            Thread.sleep(1000);
        }
        fail("was not connected to remote site within " + pollLimit + " seconds");
    }

    private EnvironmentConfig makeBasicConfig() {
        EnvironmentConfig ec = new EnvironmentConfig();
        ec.setAllowCreate(true);
        ec.setInitializeCache(true);
        ec.setInitializeLocking(true);
        ec.setInitializeLogging(true);
        ec.setInitializeReplication(true);
        ec.setTransactional(true);
        ec.setReplicationManagerAckPolicy(ReplicationManagerAckPolicy.ALL);
        ec.setRunRecovery(true);
        ec.setThreaded(true);
        if (Boolean.getBoolean("VERB_REPLICATION"))
            ec.setVerbose(VerboseConfig.REPLICATION, true);
        return (ec);
    }

}
