/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.EventHandlerAdapter;
import com.sleepycat.db.ReplicationHostAddress;
import com.sleepycat.db.ReplicationManagerAckPolicy;
import com.sleepycat.db.ReplicationManagerStartPolicy;
import com.sleepycat.db.ReplicationStats;
import com.sleepycat.db.ReplicationTimeoutType;
import com.sleepycat.db.StatsConfig;
import com.sleepycat.db.VerboseConfig;

import static org.junit.Assert.*;

import java.io.File;

public class V46impl implements TestMixedHeartbeats.Ops46, TestReverseConnect.Ops46 {
    private Config config;
    private Environment[] envs = new Environment[2];

    class MyEventHandler extends EventHandlerAdapter {
        private boolean done = false;
        private boolean panic = false;

        @Override
            synchronized public void handlePanicEvent() {
                done = true;
                panic = true;
                notifyAll();
            }

        @Override
            synchronized public void handleRepStartupDoneEvent() {
                done = true;
                notifyAll();
            }

        synchronized void await() throws Exception {
            while (!done) { wait(); }
            if (panic)
                throw new Exception("aborted by panic in DB");
        }
    }
    
    public void setConfig(Config c) { config = c; }

    public void createMaster(int site, boolean configureOther) throws Exception {
        EnvironmentConfig ec = makeBasicConfig();
        int p = config.getMyPort(site);
        ec.setReplicationManagerLocalSite(new ReplicationHostAddress("localhost", p));

        if (configureOther) {
            p = config.getOtherPort(site);
            ec.replicationManagerAddRemoteSite(new ReplicationHostAddress("localhost", p));
        }

        File masterDir = new File(config.getBaseDir(), "dir" + site);
        masterDir.mkdir();

        Environment master = new Environment(masterDir, ec);
        envs[site] = master;
        master.setReplicationTimeout(ReplicationTimeoutType.CONNECTION_RETRY,
                                     1000000); // be impatient
        master.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_MASTER);
        
        DatabaseConfig dc = new DatabaseConfig();
        dc.setTransactional(true);
        dc.setAllowCreate(true);
        dc.setType(DatabaseType.BTREE);
        Database db = master.openDatabase(null, "test.db", null, dc);
        db.close();
    }

    public void establishClient(int site, boolean configureOther) throws Exception {
        EnvironmentConfig ec = makeBasicConfig();

        int p = config.getMyPort(site);
        ec.setReplicationManagerLocalSite(new ReplicationHostAddress("localhost", p));

        if (configureOther) {
            p = config.getOtherPort(site);
            ec.replicationManagerAddRemoteSite(new ReplicationHostAddress("localhost", p));
        }
        MyEventHandler monitor = new MyEventHandler();
        ec.setEventHandler(monitor);
        File clientDir = new File(config.getBaseDir(), "dir" + site);
        clientDir.mkdir();
        Environment client = new Environment(clientDir, ec);

        client.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_CLIENT);
        monitor.await();

        assertTrue(client.getReplicationStats(StatsConfig.DEFAULT).getStartupComplete() == 1);

        client.close();
    }

    public void shutDown(int siteId) throws Exception {
        envs[siteId].close();
        envs[siteId] = null;
    }

    public void restart(int siteId) throws Exception {
        EnvironmentConfig ec = makeBasicConfig();
        int p = config.getMyPort(siteId);
        ec.setReplicationManagerLocalSite(new ReplicationHostAddress("localhost", p));

        p = config.getOtherPort(siteId);
        ec.replicationManagerAddRemoteSite(new ReplicationHostAddress("localhost", p));

        File dir = new File(config.getBaseDir(), "dir" + siteId);
        Environment e = new Environment(dir, ec);
        envs[siteId] = e;
        e.setReplicationTimeout(ReplicationTimeoutType.CONNECTION_RETRY,
                                     1000000); // be impatient
        e.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_MASTER);
    }
    
    public void remove(int site) throws Exception {
        assertNull(envs[site]);

        EnvironmentConfig ec = makeBasicConfig();
        File dir = new File(config.getBaseDir(), "dir" + site);
        assertTrue(dir.exists());
        Environment.remove(dir, false, ec);
    }

    public boolean writeUpdates(int siteId, int txnCount) throws Exception {
        DatabaseConfig dc = new DatabaseConfig();
        dc.setTransactional(true);
        Database db = envs[siteId].openDatabase(null, "test.db", null, dc);

        DatabaseEntry key = new DatabaseEntry();
        DatabaseEntry value = new DatabaseEntry();
        value.setData("hello world".getBytes());
        for (int i=0; i<txnCount; i++) {
            String k = "The record number is: " + i;
            key.setData(k.getBytes());
            db.put(null, key, value);
        }

        db.close();
        return true;
    }

    public void checkMaxVersion() throws Exception {
        // For this test to make sense, we must be at version 4.5 or 4.6.
        // 
        int major = Environment.getVersionMajor();
        int minor = Environment.getVersionMinor();
        assertTrue(major == 4 && (minor == 5 || minor == 6));
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
        ec.setReplicationNumSites(2);
        if (Boolean.getBoolean("VERB_REPLICATION"))
            ec.setVerbose(VerboseConfig.REPLICATION, true);
        return (ec);
    }

    public void restart(int siteId, File dir) throws Exception {
        throw new Exception("Not implemented");
    }
}
