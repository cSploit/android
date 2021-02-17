/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import static org.junit.Assert.assertEquals;
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
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.ReplicationManagerAckPolicy;
import com.sleepycat.db.ReplicationManagerSiteConfig;
import com.sleepycat.db.ReplicationManagerStartPolicy;
import com.sleepycat.db.VerboseConfig;

/**
 * A simple test of repmgr allowing a new, seemingly "redundant"
 * incoming connection to take over, giving up (closing) an existing
 * connection.
 */
public class TestRedundantTakeover {
    private static final String TEST_DIR_NAME = "TESTDIR";

    private File testdir;
    private Process fiddler;
    private int mgrPort;
    private int masterPort;
    private int clientPort;

    @Before public void setUp() throws Exception {
        testdir = new File(TEST_DIR_NAME);
        Util.rm_rf(testdir);
        testdir.mkdir();

        if (Boolean.getBoolean("MANUAL_FIDDLER_START")) {
            masterPort = 6000;
            clientPort = 6001;
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
            fiddler = Util.startFiddler(p, getClass().getName(), mgrPort);
        }
    }

    @After public void tearDown() throws Exception {
        if (fiddler != null) { fiddler.destroy(); }
    }

    @Test public void testExchange() throws Exception {
        EnvironmentConfig masterConfig = makeBasicConfig();
        ReplicationManagerSiteConfig site =
            new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setLocalSite(true);
        masterConfig.addReplicationManagerSite(site);
        EventHandler masterMonitor = new EventHandler();
        masterConfig.setEventHandler(masterMonitor);
        File masterDir = mkdir("master");
        Environment master = new Environment(masterDir, masterConfig);
        master.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_MASTER);
		
        // create client, wait for it to finish sync-ing up
        // 
        EventHandler clientMonitor = new EventHandler();
        EnvironmentConfig ec = makeClientConfig(clientMonitor, clientPort, masterPort);
        
        File clientDir = mkdir("client");
        Environment client = new Environment(clientDir, ec);
        client.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_CLIENT);
        clientMonitor.await();

        // tell fiddler to freeze the connection without killing it
        // 
        Socket s = new Socket("localhost", mgrPort);
        OutputStreamWriter w = new OutputStreamWriter(s.getOutputStream());

        String path1 = "{" + clientPort + "," + masterPort + "}"; // looks like {6001,6000}
        w.write("{" + path1 + ",toss_all}\r\n");
        w.flush();
        BufferedReader br = new BufferedReader(new InputStreamReader(s.getInputStream()));
        assertEquals("ok", br.readLine());

        // close client (which will be hidden from master), and try
        // opening it again
        client.close();
        clientMonitor = new EventHandler();
        ec = makeClientConfig(clientMonitor, clientPort, masterPort);
        ec.setRunRecovery(true);
        client = new Environment(clientDir, ec);
        client.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_ELECTION);
        clientMonitor.await();

        // verify with fiddler that the old connection was closed.
        // TODO

        // do a new live transaction at the master, make sure
        // that can work.
        // 
        int initialCount = masterMonitor.getPermFailCount();
        DatabaseConfig dc2 = new DatabaseConfig();
        dc2.setTransactional(true);
        dc2.setAllowCreate(true);
        dc2.setType(DatabaseType.BTREE);
        Database db2 = master.openDatabase(null, "test2.db", null, dc2);
        db2.close();
        assertEquals(initialCount, masterMonitor.getPermFailCount());


        master.close();
        client.close();

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
        ec.setReplicationManagerAckPolicy(ReplicationManagerAckPolicy.ALL);
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
