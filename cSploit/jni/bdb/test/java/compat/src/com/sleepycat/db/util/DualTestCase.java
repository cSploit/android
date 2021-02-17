/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db.util;

import java.io.File;
import java.io.FileNotFoundException;

import com.sleepycat.util.test.TestBase;

import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;

public class DualTestCase extends TestBase {

    private boolean setUpInvoked = false;

    public DualTestCase() {
        super();
    }

    @Override
    public void setUp()
        throws Exception {

        setUpInvoked = true;
        super.setUp();
    }

    @Override
    public void tearDown()
        throws Exception {

        if (!setUpInvoked) {
            throw new IllegalStateException
                ("tearDown was invoked without a corresponding setUp() call");
        }
        super.tearDown();
    }

    protected Environment create(File envHome, EnvironmentConfig envConfig)
        throws DatabaseException {

	Environment env = null;
        try {
            env = new Environment(envHome, envConfig);
        } catch (FileNotFoundException e) {
            throw new RuntimeException(e);
        }
        return env;
    }

    protected void close(Environment env)
        throws DatabaseException {

        env.close();
    }

    public static boolean isReplicatedTest(Class<?> testCaseClass) {
        return false;
    }
}
