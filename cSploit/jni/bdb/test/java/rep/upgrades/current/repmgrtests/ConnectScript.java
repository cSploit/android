/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import com.sleepycat.db.CheckpointConfig;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.EventHandlerAdapter;
import com.sleepycat.db.ReplicationConfig;
import com.sleepycat.db.ReplicationHostAddress;
import com.sleepycat.db.ReplicationManagerAckPolicy;
import com.sleepycat.db.ReplicationManagerConnectionStatus;
import com.sleepycat.db.ReplicationManagerSiteConfig;
import com.sleepycat.db.ReplicationManagerSiteInfo;
import com.sleepycat.db.ReplicationManagerStartPolicy;
import com.sleepycat.db.ReplicationTimeoutType;
import com.sleepycat.db.VerboseConfig;

import static org.junit.Assert.*;

public class ConnectScript implements SimpleConnectTest.Ops {
    private SimpleConnectTest.Config conf;
    private Environment client;

    public void setConfig(SimpleConnectTest.Config c) {
        conf = c;
    }

    public void upgradeClient() throws Exception {
        int[] remotePorts = new int[1];
        remotePorts[0] = conf.masterPort;
        EnvironmentConfig ec = makeBasicConfig(conf.clientPort, remotePorts);
        client = new Environment(conf.clientDir, ec);

        CheckpointConfig cc = new CheckpointConfig();
        cc.setForce(true);
        client.checkpoint(cc);
        client.close();
        
        MyEventHandler mon = new MyEventHandler();
        ec.setEventHandler(mon);
        client = new Environment(conf.clientDir, ec);

        // For the "reverse" test, make it practically impossible that
        // the client will retry connecting to the master after its
        // initial failed attempt.
        client.setReplicationTimeout(ReplicationTimeoutType.CONNECTION_RETRY,
                                     conf.reverse ? Integer.MAX_VALUE : 1000000);
        client.setReplicationConfig(ReplicationConfig.STRICT_2SITE, true);
        client.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_CLIENT);
    }

    public void shutdownClient() throws Exception {
        client.close();
    }

    private EnvironmentConfig makeBasicConfig(int myPort, int[] remotePorts)
        throws Exception
    {
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

        ReplicationManagerSiteConfig conf =
            new ReplicationManagerSiteConfig("localhost", myPort);
        conf.setLocalSite(true);
        conf.setLegacy(true);
        ec.addReplicationManagerSite(conf);
        for (int p : remotePorts) {
            conf = new ReplicationManagerSiteConfig("localhost", p);
            conf.setLegacy(true);
            ec.addReplicationManagerSite(conf);
        }
        
        if (Boolean.getBoolean("VERB_REPLICATION"))
            ec.setVerbose(VerboseConfig.REPLICATION, true);
        return (ec);
    }

    class MyEventHandler extends EventHandlerAdapter {
        private boolean done = false;
        private boolean panic = false;
        private boolean connFail = false;

        @Override synchronized public void handleRepStartupDoneEvent() {
            done = true;
            notifyAll();
        }

        @Override synchronized public void handlePanicEvent() {
            panic = true;
            done = true;
            notifyAll();
        }

        @Override
            synchronized public void handleRepConnectTryFailedEvent() {
                connFail = true;
                notifyAll();
            }

        synchronized void awaitConnFailure() throws Exception {
            for (;;) {
                if (connFail) { break; }
                wait();
                if (panic)
                    throw new Exception("aborted by panic in DB");
            }
        }
        synchronized void awaitStartupDone() throws Exception {
            long deadline = System.currentTimeMillis() + 10000;
            while (!done) {
                long now = System.currentTimeMillis();
                if (now >= deadline)
                    throw new Exception("timeout expired");
                long duration = deadline - now;
                wait(duration);
            }
            if (panic)
                throw new Exception("aborted by panic in DB");
        }
    }

    public void awaitClientConnFailure() throws Exception {
        ((MyEventHandler)client.getConfig().getEventHandler()).awaitConnFailure();
    }

    public void verifyClientConnect() throws Exception {
        int pollLimit = 10;
        for (int i=0; i<pollLimit; i++) {
            ReplicationManagerSiteInfo[] si = client.getReplicationManagerSiteList();
            ReplicationManagerSiteInfo inf = null;
            for (ReplicationManagerSiteInfo in : si) {
                ReplicationHostAddress addr = in.addr;
                if (addr.port == conf.masterPort) {
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
}
