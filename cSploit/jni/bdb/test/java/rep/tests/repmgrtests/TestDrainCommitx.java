/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.sleepycat.db.BtreeStats;
import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseEntry;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.ReplicationManagerAckPolicy;
import com.sleepycat.db.ReplicationManagerConnectionStatus;
import com.sleepycat.db.ReplicationManagerSiteConfig;
import com.sleepycat.db.ReplicationManagerSiteInfo;
import com.sleepycat.db.ReplicationManagerStartPolicy;
import com.sleepycat.db.ReplicationTimeoutType;
import com.sleepycat.db.VerboseConfig;

/**
 * Basic test of handling full output queue.  Use fiddler to simulate
 * a clogged client during internal init.  With a completely clogged
 * output queue, make sure we don't block live txn threads, but that
 * we do count a perm failure since we don't get an ack (since of
 * course the client didn't even receive our message.)
 */
public class TestDrainCommitx {
    private static final String TEST_DIR_NAME = "TESTDIR";

    private File testdir;
    private byte[] data;
    private Process fiddler;
    private int masterPort;
    private int clientPort;
    private int masterSpoofPort;
    private int mgrPort;

    @Before
	public void setUp() throws Exception {
        testdir = new File(TEST_DIR_NAME);
        Util.rm_rf(testdir);
        testdir.mkdir();

        String alphabet = "abcdefghijklmnopqrstuvwxyz";
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        OutputStreamWriter w = new OutputStreamWriter(baos);
        for (int i=0; i<100; i++) { w.write(alphabet); }
        w.close();
        data = baos.toByteArray();

        
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
            PortsConfig p = new PortsConfig(2);
            masterPort = p.getRealPort(0);
            clientPort = p.getRealPort(1);
            fiddler = Util.startFiddler(p, getClass().getName(), mgrPort);
        }
    }

    @After public void tearDown() throws Exception {
        if (fiddler != null) { fiddler.destroy(); }
    }
	
    @Test public void testDraining() throws Exception {
        // TODO: move this to a test runner.
        if (Boolean.getBoolean("debug.pause")) {
            // force DB to be loaded first (is this necessary?)
            Environment.getVersionString();
            System.out.println(".> ");
            System.in.read();
        }

        EnvironmentConfig masterConfig = makeBasicConfig();
        masterConfig.setReplicationLimit(100000000);
        ReplicationManagerSiteConfig site
            = new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setLocalSite(true);
        masterConfig.addReplicationManagerSite(site);
        EventHandler masterMonitor = new EventHandler();
        masterConfig.setEventHandler(masterMonitor);
        File masterDir = mkdir("master");
        Environment master = new Environment(masterDir, masterConfig);
        master.setReplicationTimeout(ReplicationTimeoutType.ACK_TIMEOUT, 5000000);
        master.replicationManagerStart(3, ReplicationManagerStartPolicy.REP_MASTER);
		
        DatabaseConfig dc = new DatabaseConfig(); 
        dc.setTransactional(true);
        dc.setAllowCreate(true);
        dc.setType(DatabaseType.BTREE);
        dc.setPageSize(4096);
        Database db = master.openDatabase(null, "test.db", null, dc);
		
        DatabaseEntry key = new DatabaseEntry();
        DatabaseEntry value = new DatabaseEntry();
        value.setData(data);
        for (int i=0;
             ((BtreeStats)db.getStats(null, null)).getPageCount() < 500;
             i++)
        {
            String k = "The record number is: " + i;
            key.setData(k.getBytes());
            db.put(null, key, value);
        }
        db.close();

        // tell fiddler to stop reading once it sees a PAGE message
        // 
        Socket s = new Socket("localhost", mgrPort);
        OutputStreamWriter w = new OutputStreamWriter(s.getOutputStream());

        String path1 = "{" + masterPort + "," + clientPort + "}"; // looks like {6000,6001}
        w.write("{init," + path1 + ",page_clog}\r\n");
        w.flush();
        BufferedReader br = new BufferedReader(new InputStreamReader(s.getInputStream()));
        br.readLine();          // for now, ignore returned serial number
        assertEquals("ok", br.readLine());

        EnvironmentConfig ec = makeBasicConfig();
        site = new ReplicationManagerSiteConfig("localhost", clientPort);
        site.setLocalSite(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setBootstrapHelper(true);
        ec.addReplicationManagerSite(site);
        
        EventHandler clientMonitor = new EventHandler();
        ec.setEventHandler(clientMonitor);
        Environment client = new Environment(mkdir("client"), ec);
        client.setReplicationTimeout(ReplicationTimeoutType.CONNECTION_RETRY, 3000000);
        client.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_CLIENT);
        
        for (long deadline = System.currentTimeMillis() + 20000;;Thread.sleep(100)) {
            if (System.currentTimeMillis() > deadline)
                throw new Exception("what's taking so long to connect?");
            ReplicationManagerSiteInfo[] connections = master.getReplicationManagerSiteList();
            if (connections.length == 1 && 
                connections[0].getConnectionStatus() == ReplicationManagerConnectionStatus.CONNECTED) {
                assertEquals("localhost", connections[0].addr.host);
                assertEquals(clientPort, connections[0].addr.port);
                break;
            }
        }

        // Wait until it gets stuck.  (For now, the best we know how
        // to do is to pause for 5 seconds.  It would be better if we
        // could figure out a way to check for this more reliably.)
        Thread.sleep(5000);

        // do another new live transaction at the master, make sure
        // that can work.  Instead of writing into an existing
        // database, create a new one.  That avoid a potential
        // conflict with internal init when we close it (#15820).
        // 
        int initialCount = masterMonitor.getPermFailCount();
        long start = System.currentTimeMillis();
        db = master.openDatabase(null, "dummy.db", null, dc);
        long duration = System.currentTimeMillis() - start;
        assertEquals(initialCount+1, masterMonitor.getPermFailCount());

        // In this case, the master should be able to realize that
        // there's no hope of an ack, so it shouldn't wait for the
        // timeout.
        // 
        assertTrue("duration: " + duration, duration < 1000);
        db.close(false);

        // Hey, this is good.  I get for free a test of releasing the
        // thread on db_rep->finished.  Make sure it doesn't take too
        // long (although this might be too undependable).
        //
        start = System.currentTimeMillis();
        master.close();
        duration = System.currentTimeMillis() - start;
        assertTrue("duration: " + duration, duration < 1000);

        // At this point, the client is wedged, stuck in internal
        // init.  It might be nice to clean it up, but doing so is a
        // bit difficult, and since we're really done with this test
        // anyway it's not crucial.

        w.write("shutdown\r\n");
        w.flush();
        assertEquals("ok", br.readLine());
        s.close();
        fiddler = null;
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
        ec.setReplicationInMemory(true);
        ec.setCacheSize(256 * 1024 * 1024);
        ec.setReplicationManagerAckPolicy(ReplicationManagerAckPolicy.ONE);
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
