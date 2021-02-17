/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertNotNull;

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
import com.sleepycat.db.ReplicationManagerSiteConfig;
import com.sleepycat.db.ReplicationManagerStartPolicy;
import com.sleepycat.db.ReplicationTimeoutType;
import com.sleepycat.db.VerboseConfig;

/**
 * Test that a second client can be created and do internal init,
 * while the first thread is still clogged up.
 */
public class TestDrainIntInit {
    private static final String TEST_DIR_NAME = "TESTDIR";

    private File testdir;
    private byte[] data;
    private Process fiddler;
    private int masterPort;
    private int clientPort;
    private int clientPort2;
    private int mgrPort;

    @Before public void setUp() throws Exception {
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
            clientPort2 = 6002;
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
            clientPort2 = p.getRealPort(2);
            fiddler = Util.startFiddler(p, getClass().getName(), mgrPort);
        }
    }

    @After public void tearDown() throws Exception {
        if (fiddler != null) { fiddler.destroy(); }
    }
	
    // with only a single msg processing thread, clog up client,
    // verify that thread eventually gives up, by starting another
    // client and making sure it can do internal init.  Can we also
    // check that the amount of time it waits before giving up seems
    // reasonable?
    // 
    @Test public void drainBlockInternalInit() throws Exception {
        EnvironmentConfig ec = makeBasicConfig();
        ec.setReplicationLimit(100000000);
        ReplicationManagerSiteConfig site =
            new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setLocalSite(true);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);

        site = new ReplicationManagerSiteConfig("localhost", clientPort);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", clientPort2);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);

        Environment master = new Environment(mkdir("master"), ec);
        master.setReplicationTimeout(ReplicationTimeoutType.ACK_TIMEOUT,
                                     3000000);
        master.replicationManagerStart(2, ReplicationManagerStartPolicy.REP_MASTER);
		
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

        // create client
        // Tell fiddler to stop reading once it sees a PAGE message
        // 
        Socket s = new Socket("localhost", mgrPort);
        OutputStreamWriter w = new OutputStreamWriter(s.getOutputStream());

        String path1 = "{" + masterPort + "," + clientPort + "}"; // looks like {6000,6001}
        w.write("{init," + path1 + ",page_clog}\r\n");
        w.flush();
        BufferedReader br = new BufferedReader(new InputStreamReader(s.getInputStream()));
        br.readLine();          // for now, ignore returned serial number
        assertEquals("ok", br.readLine());

        ec = makeBasicConfig();
        site = new ReplicationManagerSiteConfig("localhost", clientPort);
        site.setLocalSite(true);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);

        site = new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", clientPort2);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        EventHandler mon = new EventHandler();
        ec.setEventHandler(mon);
        Environment client = new Environment(mkdir("client"), ec);
        client.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_CLIENT);

        // wait for NEWMASTER
        mon.awaitNewmaster();

        // wait til it gets stuck
        Thread.sleep(5000);     // FIXME

        ec = makeBasicConfig();
        site = new ReplicationManagerSiteConfig("localhost", clientPort2);
        site.setLocalSite(true);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);

        site = new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", clientPort);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);

        mon = new EventHandler();
        ec.setEventHandler(mon);
        long start = System.currentTimeMillis();
        Environment client2 = new Environment(mkdir("client2"), ec);
        client2.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_CLIENT);

        mon.await();
        long duration = System.currentTimeMillis() - start;

        assertTrue("duration: " + duration, duration < 5000);

        // TODO: close client work-around
        client2.close();
        master.close();

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
