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
import com.sleepycat.db.ReplicationManagerSiteConfig;
import com.sleepycat.db.ReplicationManagerStartPolicy;
import com.sleepycat.db.ReplicationTimeoutType;
import com.sleepycat.db.VerboseConfig;

/**
 * Get a connection hopelessly clogged, and then kill the connection.
 * Verify that the blocked thread is immediately freed.
 */
public class TestDrainAbandon {
    private static final String TEST_DIR_NAME = "TESTDIR";

    private File testdir;
    private byte[] data;
    private int masterPort;
    private int clientPort;
    private int client2Port;
    private int client3Port;
    private int mgrPort;

    @Before public void setUp() throws Exception {
        testdir = new File(TEST_DIR_NAME);
        Util.rm_rf(testdir);
        testdir.mkdir();

        String alphabet = "abcdefghijklmnopqrstuvwxyz";
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        OutputStreamWriter w = new OutputStreamWriter(baos);
        while (baos.size() < 1000) // arbitrary min. size
            w.write(alphabet);
        w.close();
        data = baos.toByteArray();

        if (Boolean.getBoolean("MANUAL_FIDDLER_START")) {
            masterPort = 6000;
            clientPort = 6001;
            client2Port = 6002;
            client3Port = 6003;
            mgrPort = 8000;
        } else {
            String mgrPortNum = System.getenv("DB_TEST_FAKE_PORT");
            assertNotNull("required DB_TEST_FAKE_PORT environment variable not found",
                          mgrPortNum);
            mgrPort = Integer.parseInt(mgrPortNum);
            PortsConfig p = new PortsConfig(4);
            masterPort = p.getRealPort(0);
            clientPort = p.getRealPort(1);
            client2Port = p.getRealPort(2);
            client3Port = p.getRealPort(3);
            Util.startFiddler(p, getClass().getName(), mgrPort);
        }
    }
	
    @Test public void testDraining() throws Exception {
        EnvironmentConfig masterConfig = makeBasicConfig();
        masterConfig.setReplicationLimit(100000000);
        ReplicationManagerSiteConfig site =
            new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setLocalSite(true);
        site.setLegacy(true);
        masterConfig.addReplicationManagerSite(site);

        site = new ReplicationManagerSiteConfig("localhost", clientPort);
        site.setLegacy(true);
        masterConfig.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", client2Port);
        site.setLegacy(true);
        masterConfig.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", client3Port);
        site.setLegacy(true);
        masterConfig.addReplicationManagerSite(site);

        Environment master = new Environment(mkdir("master"), masterConfig);
        setTimeouts(master);
        // Prevent connection retries, so that all connections
        // originate from clients 
        master.setReplicationTimeout(ReplicationTimeoutType.CONNECTION_RETRY,
                                     Integer.MAX_VALUE);
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

        // tell fiddler to stop reading once it sees a PAGE message
        Socket s = new Socket("localhost", mgrPort);
        OutputStreamWriter w = new OutputStreamWriter(s.getOutputStream());

        String path1 = "{" + masterPort + "," + clientPort + "}"; // looks like {6000,6001}
        w.write("{init," + path1 + ",page_clog}\r\n");
        w.flush();
        BufferedReader br = new BufferedReader(new InputStreamReader(s.getInputStream()));
        br.readLine();
        assertEquals("ok", br.readLine());
        // create client
        // 
        EnvironmentConfig ec = makeBasicConfig();
        site = new ReplicationManagerSiteConfig("localhost", clientPort);
        site.setLocalSite(true);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", client2Port);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", client3Port);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        Environment client = new Environment(mkdir("client"), ec);
        setTimeouts(client);
        client.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_CLIENT);

        // wait til it gets stuck
        Thread.sleep(5000);     // FIXME

        // Do the same for another client, because the master has 2
        // msg processing threads.  (It's no longer possible to
        // configure just 1.)
        String path2 = "{" + masterPort + "," + client2Port + "}";
        w.write("{init," + path2 + ",page_clog}\r\n");
        w.flush();
        br = new BufferedReader(new InputStreamReader(s.getInputStream()));
        br.readLine();
        assertEquals("ok", br.readLine());

        ec = makeBasicConfig();
        site = new ReplicationManagerSiteConfig("localhost", client2Port);
        site.setLocalSite(true);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", clientPort);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", client3Port);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        Environment client2 = new Environment(mkdir("client2"), ec);
        setTimeouts(client2);
        client2.replicationManagerStart(1, ReplicationManagerStartPolicy.REP_CLIENT);

        // wait til it gets stuck
        Thread.sleep(5000);


        // With the connection stuck, the master cannot write out log
        // records for new "live" transactions.  Knowing we didn't
        // write the record, we should not bother waiting for an ack
        // that cannot possibly arrive; so we should simply return
        // quickly.  The duration should be very quick, but anything
        // less than the ack timeout indicates correct behavior (in
        // case this test runs on a slow, overloaded system).
        // 
        long startTime = System.currentTimeMillis();
        key.setData("one extra record".getBytes());
        db.put(null, key, value);
        long duration = System.currentTimeMillis() - startTime;
        assertTrue("txn duration: " + duration, duration < 29000);
        System.out.println("txn duration: " + duration);
        db.close();

        // Tell fiddler to close the connections.  That should trigger
        // us to abandon the timeout.  Then create another client and
        // see that it can complete its internal init quickly.  Since
        // we have limited threads at the master, this demonstrates
        // that they were abandoned.
        //
        path1 = "{" + clientPort + "," + masterPort + "}"; // looks like {6001,6000}
        w.write("{" + path1 + ",shutdown}\r\n");
        w.flush();
        assertEquals("ok", br.readLine());
        path2 = "{" + client2Port + "," + masterPort + "}"; // looks like {6001,6000}
        w.write("{" + path2 + ",shutdown}\r\n");
        w.flush();
        assertEquals("ok", br.readLine());

        ec = makeBasicConfig();
        site = new ReplicationManagerSiteConfig("localhost", client3Port);
        site.setLocalSite(true);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", masterPort);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", clientPort);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        site = new ReplicationManagerSiteConfig("localhost", client2Port);
        site.setLegacy(true);
        ec.addReplicationManagerSite(site);
        
        EventHandler clientMonitor = new EventHandler();
        ec.setEventHandler(clientMonitor);
        Environment client3 = new Environment(mkdir("client3"), ec);
        setTimeouts(client3);
        startTime = System.currentTimeMillis();
        client3.replicationManagerStart(2, ReplicationManagerStartPolicy.REP_CLIENT);
        clientMonitor.await();
        duration = System.currentTimeMillis() - startTime;
        assertTrue("sync duration: " + duration, duration < 20000); // 20 seconds should be plenty

        client3.close();
        master.close();

        w.write("shutdown\r\n");
        w.flush();
        assertEquals("ok", br.readLine());
        s.close();
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
        if (Boolean.getBoolean("VERB_REPLICATION"))
            ec.setVerbose(VerboseConfig.REPLICATION, true);
        return (ec);
    }
	
    private void setTimeouts(Environment e) throws Exception {
        e.setReplicationTimeout(ReplicationTimeoutType.ACK_TIMEOUT,
                                     30000000);
    }

    public File mkdir(String dname) {
        File f = new File(testdir, dname);
        f.mkdir();
        return f;
    }
}
