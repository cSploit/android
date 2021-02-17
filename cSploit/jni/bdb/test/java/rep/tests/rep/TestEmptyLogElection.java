/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package rep;

import static org.junit.Assert.assertTrue;

import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.EventHandler;
import com.sleepycat.db.EventHandlerAdapter;
import com.sleepycat.db.ReplicationManagerSiteConfig;
import com.sleepycat.db.ReplicationStats;
import com.sleepycat.db.ReplicationTimeoutType;
import com.sleepycat.db.ReplicationManagerStartPolicy;
import com.sleepycat.db.StatsConfig;
import com.sleepycat.db.VerboseConfig;

import repmgrtests.Util;
import org.junit.Test;

import java.io.File;

public class TestEmptyLogElection {
    @Test public void elect() throws Exception {
        int n = 10;             // arbitrary

        for (int i=0; i<n; i++)
            doOneElection();
    }

    private void doOneElection() throws Exception {
        int testPorts[] = Util.findAvailablePorts(2);
        int port1 = testPorts[0];
        int port2 = testPorts[1];

        MyEventHandler mon = new MyEventHandler();

        EnvironmentConfig ec = makeBasicConfig(port1, mon);
        ec.setReplicationPriority(100);
        File dir1 = Util.mkdir("site1");
        ReplicationManagerSiteConfig site =
            new ReplicationManagerSiteConfig("localhost", port2);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        Environment env1 = new Environment(dir1, ec);
        env1.setReplicationTimeout(ReplicationTimeoutType.ELECTION_TIMEOUT,
                                   500000);
        env1.setReplicationTimeout(ReplicationTimeoutType.ELECTION_RETRY,
                                   1000000);
        env1.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_ELECTION);

        ec = makeBasicConfig(port2, mon);
        ec.setReplicationPriority(200);
        site = new ReplicationManagerSiteConfig("localhost", port1);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        File dir2 = Util.mkdir("site2");
        Environment env2 = new Environment(dir2, ec);
        env2.setReplicationTimeout(ReplicationTimeoutType.ELECTION_TIMEOUT,
                                   500000);
        env2.setReplicationTimeout(ReplicationTimeoutType.ELECTION_RETRY,
                                   1000000);
        env2.setReplicationTimeout(ReplicationTimeoutType.CONNECTION_RETRY,
                                   500000);
        env2.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_ELECTION);

        mon.await();
        ReplicationStats s = env2.getReplicationStats(StatsConfig.DEFAULT);
        assertTrue(s.getEnvId() == s.getMaster());

        env1.close();
        env2.close();
    }

    class MyEventHandler extends EventHandlerAdapter {
        private boolean done, panic;

        @Override synchronized public void handleRepStartupDoneEvent() {
            done = true;
            notifyAll();
        }
        
        @Override synchronized public void handlePanicEvent() {
            done = true;
            panic = true;
            notifyAll();
        }
        
        synchronized void await() throws Exception {
            while (!done) { wait(); }
            if (panic)
                throw new Exception("aborted by panic in DB");
        }
    }

    private EnvironmentConfig makeBasicConfig(int port, EventHandler eh) throws Exception {
        EnvironmentConfig ec = new EnvironmentConfig();
        ec.setAllowCreate(true);
        ec.setInitializeCache(true);
        ec.setInitializeLocking(true);
        ec.setInitializeLogging(true);
        ec.setInitializeReplication(true);
        ec.setTransactional(true);
        ec.setThreaded(true);
        ec.setEventHandler(eh);
        if (Boolean.getBoolean("VERB_REPLICATION"))
            ec.setVerbose(VerboseConfig.REPLICATION, true);
        ReplicationManagerSiteConfig site =
            new ReplicationManagerSiteConfig("localhost", port);
        site.setLocalSite(true);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        return (ec);
    }
}
