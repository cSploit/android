/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertNull;

import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.ReplicationManagerSite;
import com.sleepycat.db.ReplicationManagerSiteConfig;

import org.junit.Before;
import org.junit.Test;

import java.io.File;

public class TestConfig {
    private EnvironmentConfig ec;
    private File dir;

    @Before public void setUp() throws Exception {
        ec = new EnvironmentConfig();
        ec.setAllowCreate(true);
        ec.setInitializeCache(true);
        ec.setInitializeLocking(true);
        ec.setInitializeLogging(true);
        ec.setInitializeReplication(true);
        ec.setTransactional(true);
        ec.setThreaded(true);

        dir = Util.mkdir("site1");
    }

    @Test(expected=IllegalArgumentException.class)
        public void nullHost() throws Exception
    {
        ReplicationManagerSiteConfig conf =
            new ReplicationManagerSiteConfig(null, 6000);
        conf.setLocalSite(true);
        ec.addReplicationManagerSite(conf);

        Environment env = new Environment(dir, ec);
        env.close();
    }

    @Test public void host() throws Exception
    {
        ReplicationManagerSiteConfig conf =
            new ReplicationManagerSiteConfig("localhost", 6000);
        conf.setLocalSite(true);
        ec.addReplicationManagerSite(conf);

        Environment env = new Environment(dir, ec);
        env.close();
    }

    @Test public void getLocalSite() throws Exception
    {
        String host = "localhost";
        long port = 6000;
        ReplicationManagerSiteConfig conf =
            new ReplicationManagerSiteConfig(host, port);
        conf.setLocalSite(true);
        ec.addReplicationManagerSite(conf);

        Environment env = new Environment(dir, ec);
        ReplicationManagerSite dbsite = env.getReplicationManagerLocalSite();

	assertTrue(host.equals(dbsite.getAddress().host));
	assertEquals(port, dbsite.getAddress().port);
        env.close();
    }

    @Test public void noLocalSite() throws Exception {
        Environment env = new Environment(dir, ec);
        assertNull(env.getReplicationManagerLocalSite());
        env.close();
    }
}
