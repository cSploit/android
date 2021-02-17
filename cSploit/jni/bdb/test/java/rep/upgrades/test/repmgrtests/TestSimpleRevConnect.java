/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import org.junit.Test;

public class TestSimpleRevConnect extends SimpleConnectTest {
    @Test public void reverseMixed() throws Exception {
        doTest(true);
    }
}    
    
