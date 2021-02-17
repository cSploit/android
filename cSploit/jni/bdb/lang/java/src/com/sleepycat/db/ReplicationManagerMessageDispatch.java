/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

public interface ReplicationManagerMessageDispatch {

    void dispatch(ReplicationChannel chan, java.util.Set messages, boolean need_response);	
}
