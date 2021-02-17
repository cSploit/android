/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import org.junit.Test;

public class TestSimpleFwdConnect extends SimpleConnectTest {
    @Test public void forwardMixed() throws Exception {
        doTest(false);
    }
}    
    
