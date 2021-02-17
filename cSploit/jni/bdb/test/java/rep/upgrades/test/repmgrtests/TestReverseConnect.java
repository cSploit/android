/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import org.junit.Test;
import java.io.File;

/**
 * A simple test for upgrades from 4.6 to verify that cross-version
 * handshake processing works OK in the opposite sense from which we
 * usually try it.
 */
public class TestReverseConnect extends AbstractUpgTest {
    public interface Ops {
        public void setConfig(Config c);
        public void joinExistingClient(int site, boolean useHB)
            throws Exception;
        public void verifyConnect(int site) throws Exception;
        public void restart(int siteId) throws Exception;
        public void awaitConnFailure(int siteId) throws Exception;
        public void shutDown(int siteId) throws Exception;
    }

    public interface Ops46 {
        public void setConfig(Config c);
        public void createMaster(int site, boolean configClient)
            throws Exception;
        public void establishClient(int site, boolean configMaster)
            throws Exception;
        public void restart(int siteId) throws Exception;
        public void shutDown(int siteId) throws Exception;
    }
    
    public TestReverseConnect() {
        super("46", "repmgrtests.V46impl", "repmgrtests.CurrentImpl");
    }

    @Test public void reverseMixed() throws Exception {
        Ops46 oldGroup = (Ops46)oldGroup_o;
        Ops currentGroup = (Ops)currentGroup_o;

        File testdir = new File("TESTDIR");
        Util.rm_rf(testdir);
        testdir.mkdir();
        Config config = new Config(testdir);
        oldGroup.setConfig(config);
        currentGroup.setConfig(config);

        oldGroup.createMaster(0, true);
        oldGroup.establishClient(1, false);
        oldGroup.shutDown(0);

        currentGroup.restart(1);
        currentGroup.awaitConnFailure(1);
        oldGroup.restart(0);
        currentGroup.verifyConnect(1);

        oldGroup.shutDown(0);
        currentGroup.shutDown(1);
    }
}
