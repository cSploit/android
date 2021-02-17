/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertNotNull;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.ReplicationManagerSiteConfig;
import com.sleepycat.db.ReplicationManagerStartPolicy;
import com.sleepycat.db.ReplicationManagerStats;
import com.sleepycat.db.ReplicationTimeoutType;
import com.sleepycat.db.StatsConfig;
import com.sleepycat.db.VerboseConfig;

/**
 * Tests for repmgr heartbeat feature.
 */
public class TestHeartbeats {
    private static final String TEST_DIR_NAME = "TESTDIR";

    private File testdir;
    private Process fiddler;
    Socket s;
    private int masterPort;
    private int clientPort;
    private int client2Port;
    private int mgrPort;

    @Before public void setUp() throws Exception {
        testdir = new File(TEST_DIR_NAME);
        Util.rm_rf(testdir);
        testdir.mkdir();

        if (Boolean.getBoolean("MANUAL_FIDDLER_START")) {
            masterPort = 6000;
            clientPort = 6001;
            client2Port = 6002;
            mgrPort = 8000;
            fiddler = null;
        } else {
            String mgrPortNum = System.getenv("DB_TEST_FAKE_PORT");
            assertNotNull("required DB_TEST_FAKE_PORT environment variable not found",
                          mgrPortNum);
            mgrPort = Integer.parseInt(mgrPortNum);
            PortsConfig p = new PortsConfig(3);
            masterPort = p.getRealPort(0);
            clientPort = p.getRealPort(1);
            client2Port = p.getRealPort(2);
            fiddler = Util.startFiddler(p, getClass().getName(), mgrPort);
        }
    }

    @After public void tearDown() throws Exception {
        if (fiddler != null) { fiddler.destroy(); }
    }

    // (Is 2 minutes enough?)
    // 
    @Test(timeout=120000) public void testMonitorCallElect() throws Exception {
        EnvironmentConfig masterConfig = makeBasicConfig();
        ReplicationManagerSiteConfig site =
            new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setLocalSite(true);
        masterConfig.addReplicationManagerSite(site);
        File masterDir = mkdir("master");
        Environment master = new Environment(masterDir, masterConfig);
        master.setReplicationTimeout(ReplicationTimeoutType.HEARTBEAT_SEND, 3000000);
        master.setReplicationTimeout(ReplicationTimeoutType.HEARTBEAT_MONITOR, 5000000);
        master.setReplicationTimeout(ReplicationTimeoutType.CONNECTION_RETRY, 3000000);
        master.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_MASTER);
        DatabaseConfig dc = new DatabaseConfig();
        dc.setTransactional(true);
        dc.setAllowCreate(true);
        dc.setType(DatabaseType.BTREE);
        Database db = master.openDatabase(null, "test.db", null, dc);
		

        // create two clients, wait for them to finish sync-ing up
        // 
        EventHandler clientMonitor = new EventHandler();
        EnvironmentConfig ec = makeClientConfig(clientMonitor, clientPort, masterPort);
        
        File clientDir = mkdir("client");
        Environment client = new Environment(clientDir, ec);
        client.setReplicationTimeout(ReplicationTimeoutType.HEARTBEAT_SEND, 3000000);
        client.setReplicationTimeout(ReplicationTimeoutType.HEARTBEAT_MONITOR, 5000000);
        client.setReplicationTimeout(ReplicationTimeoutType.CONNECTION_RETRY, 3000000);
        client.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_CLIENT);
        clientMonitor.await();

        EventHandler client2Monitor = new EventHandler();
        ec = makeClientConfig(client2Monitor, client2Port, masterPort);
        
        File client2Dir = mkdir("client2");
        Environment client2 = new Environment(client2Dir, ec);
        client2.setReplicationTimeout(ReplicationTimeoutType.HEARTBEAT_SEND, 3000000);
        client2.setReplicationTimeout(ReplicationTimeoutType.HEARTBEAT_MONITOR, 5000000);
        client2.setReplicationTimeout(ReplicationTimeoutType.CONNECTION_RETRY, 3000000);
        client2.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_CLIENT);
        client2Monitor.await();

        StatsConfig statsConfig = new StatsConfig();
        statsConfig.setClear(true);
        master.getReplicationManagerStats(statsConfig);

        // do some transactions, at a leisurely pace (but quicker than
        // the hb period)
        // 
        DatabaseEntry key = new DatabaseEntry();
        DatabaseEntry value = new DatabaseEntry();
        value.setData("foo".getBytes());
        for (int i=0; i<3; i++) {
            Thread.sleep(2000);
            String k = "The record number is: " + i;
            key.setData(k.getBytes());
            db.put(null, key, value);
        }

        // a few more, but not quick enough to prevent HB's
        //
        for (int i=3; i<6; i++) {
            Thread.sleep(4000);
            String k = "The record number is: " + i;
            key.setData(k.getBytes());
            db.put(null, key, value);
        }

        // pause for a few HB times
        Thread.sleep(10000);
        assertFalse(clientMonitor.isMaster() || client2Monitor.isMaster());

        // tell fiddler to freeze (not really the right word) the
        // connection paths from master to both clients.
        // 
        if (s == null)
            s = new Socket("localhost", mgrPort);
        OutputStreamWriter w = new OutputStreamWriter(s.getOutputStream());

        String path1 = "{" + masterPort + "," + clientPort + "}"; // looks like {6000,6001}
        w.write("{" + path1 + ",toss_all}\r\n");
        w.flush();
        BufferedReader br = new BufferedReader(new InputStreamReader(s.getInputStream()));
        assertEquals("ok", br.readLine());

        String path2 = "{" + masterPort + "," + client2Port + "}"; // looks like {6000,6002}
        w.write("{" + path2 + ",toss_all}\r\n");
        w.flush();
        assertEquals("ok", br.readLine());

        // wait for the clients to notice a problem (5 sec max), plus
        // give them a couple extra seconds to hold an election
        // 
        Thread.sleep(5000 + 2000);
        ReplicationManagerStats masterStats =
            master.getReplicationManagerStats(StatsConfig.DEFAULT);
        assertTrue(clientMonitor.isMaster() || client2Monitor.isMaster());

        client2.close();
        client.close();
        db.close();
        master.close();

        w.write("shutdown\r\n");
        w.flush();
        assertEquals("ok", br.readLine());
        s.close();
        fiddler = null;
    }

    private EnvironmentConfig makeClientConfig(EventHandler evHandler,
                                               int clientPort, int masterPort)
        throws Exception
    {
        EnvironmentConfig ec = makeBasicConfig();
        ReplicationManagerSiteConfig site =
            new ReplicationManagerSiteConfig("localhost", clientPort);
        site.setLocalSite(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setBootstrapHelper(true);
        ec.addReplicationManagerSite(site);
        ec.setEventHandler(evHandler);
        return (ec);
    }
    
    public static EnvironmentConfig makeBasicConfig() {
        EnvironmentConfig ec = new EnvironmentConfig();
        ec.setAllowCreate(true);
        ec.setInitializeCache(true);
        ec.setInitializeLocking(true);
        ec.setInitializeLogging(true);
        ec.setInitializeReplication(true);
        ec.setTransactional(true);
        ec.setThreaded(true);
        if (Boolean.getBoolean("VERB_REPLICATION"))
            ec.setVerbose(VerboseConfig.REPLICATION, true);
        return (ec);
    }
	
    public File mkdir(String dname) {
        File f = new File(testdir, dname);
        f.mkdir();
        return f;
    }
}
