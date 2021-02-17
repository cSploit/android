/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package db.repquote;

import com.sleepycat.db.ReplicationHostAddress;

public class RepRemoteHost{
    private ReplicationHostAddress addr;
    private boolean isPeer;

    public RepRemoteHost(ReplicationHostAddress remoteAddr, boolean hostIsPeer){
        addr = remoteAddr;
        isPeer = hostIsPeer;
    }

    public ReplicationHostAddress getAddress(){
        return addr;
    }

    public boolean isPeer(){
        return isPeer;
    }
}
