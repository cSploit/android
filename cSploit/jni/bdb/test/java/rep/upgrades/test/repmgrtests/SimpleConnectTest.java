/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import org.junit.Before;
import java.io.File;

public abstract class SimpleConnectTest extends AbstractUpgTest {
    public static final int[] noRemotePorts = new int[0];

    private Config conf;

    public static class Config {
        public File masterDir, clientDir;
        public int masterPort, clientPort;

        /**
         * Flag to control whether master site is pre-configured with
         * the network address of, and therefore initiates connection
         * to, the client.  Normally it's the other way around;
         */
        public boolean reverse;
    }

    public interface Ops {
        public void setConfig(Config c);
        public void upgradeClient() throws Exception;
        public void awaitClientConnFailure() throws Exception;
        public void verifyClientConnect() throws Exception;
        public void shutdownClient() throws Exception;
    }

    public interface Ops47 {
        public void setConfig(Config c);
        public void createGroup() throws Exception;
        public void startMaster() throws Exception;
        public void shutdownMaster() throws Exception;
    }

    public SimpleConnectTest() {
        super("47", "repmgrtests.ConnectScript", "repmgrtests.ConnectScript");
    }

    @Before public void setup() throws Exception {
        File testdir = new File("TESTDIR");
        Util.rm_rf(testdir);
        testdir.mkdir();

        conf = new Config();
        conf.masterDir = new File(testdir, "master");
        conf.masterDir.mkdir();
        conf.clientDir = new File(testdir, "client");
        conf.clientDir.mkdir();
        
        int[] ports = Util.findAvailablePorts(2);
        conf.masterPort = ports[0];
        conf.clientPort = ports[1];
    }        

    protected void doTest(boolean reverse) throws Exception {
        Ops47 old = (Ops47)oldGroup_o;
        Ops current = (Ops)currentGroup_o;
        conf.reverse = reverse;
        old.setConfig(conf);
        current.setConfig(conf);

        old.createGroup();
        
        if (reverse) {
            current.upgradeClient();
            current.awaitClientConnFailure();
            old.startMaster();
        } else {
            old.startMaster();
            current.upgradeClient();
        }
        current.verifyClientConnect();
        
        current.shutdownClient();
        old.shutdownMaster();
    }
}
