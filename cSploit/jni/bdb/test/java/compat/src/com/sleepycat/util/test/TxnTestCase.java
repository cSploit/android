/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.util.test;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.junit.After;
import org.junit.Before;

import com.sleepycat.compat.DbCompat;
import com.sleepycat.db.CursorConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;
import com.sleepycat.db.Transaction;
import com.sleepycat.db.TransactionConfig;
import com.sleepycat.db.util.DualTestCase;

/**
 * Permutes test cases over three transaction types: null (non-transactional),
 * auto-commit, and user (explicit).
 *
 * <p>Overrides runTest, setUp and tearDown to open/close the environment and
 * to set up protected members for use by test cases.</p>
 *
 * <p>If a subclass needs to override setUp or tearDown, the overridden method
 * should call super.setUp or super.tearDown.</p>
 *
 * <p>When writing a test case based on this class, write it as if a user txn
 * were always used: call txnBegin, txnCommit and txnAbort for all write
 * operations.  Use the isTransactional protected field for setup of a database
 * config.</p>
 */
public abstract class TxnTestCase extends DualTestCase {

    public static final String TXN_NULL = "txn-null";
    public static final String TXN_AUTO = "txn-auto";
    public static final String TXN_USER = "txn-user";
    public static final String TXN_CDB = "txn-cdb";

    protected File envHome;
    protected Environment env;
    protected EnvironmentConfig envConfig;
    protected String txnType;
    protected boolean isTransactional;

    public static List<Object[]> getTxnTypes(String[] txnTypes, boolean rep) {
        if (txnTypes == null) {
            if (rep) {
                txnTypes = new String[] { // Skip non-transactional tests
                                          TxnTestCase.TXN_USER,
                                          TxnTestCase.TXN_AUTO };
            } else if (!DbCompat.CDB) {
                txnTypes = new String[] { TxnTestCase.TXN_NULL,
                                          TxnTestCase.TXN_USER,
                                          TxnTestCase.TXN_AUTO };
            } else {
                txnTypes = new String[] { TxnTestCase.TXN_NULL,
                                          TxnTestCase.TXN_USER,
                                          TxnTestCase.TXN_AUTO, 
                                          TxnTestCase.TXN_CDB };
            }
        } else {
            if (!DbCompat.CDB) {
                /* Remove TxnTestCase.TXN_CDB, if there is any. */
                ArrayList<String> tmp = new ArrayList<String> 
                                            (Arrays.asList(txnTypes));
                tmp.remove(TxnTestCase.TXN_CDB);
                txnTypes = new String[tmp.size()];
                tmp.toArray(txnTypes);
            }
        }
        List<Object[]> list = new ArrayList<Object[]>();
        for (String type : txnTypes) {
            list.add(new Object[] {type});
        }
        return list;
    }

    @Before
    public void setUp()
        throws Exception {

        super.setUp();
        envHome = SharedTestUtils.getNewDir();
        openEnv();
    }

    @After
    public void tearDown()
        throws Exception {

        super.tearDown();
        closeEnv();
        env = null;
    }
    
    protected void initEnvConfig() {
        if (envConfig == null) {
            envConfig = new EnvironmentConfig();
            envConfig.setAllowCreate(true);
            
            /* Always use write-no-sync (by default) to speed up tests. */
            if (!envConfig.getTxnNoSync() && !envConfig.getTxnWriteNoSync()) {
                envConfig.setTxnWriteNoSync(true);
            }
        }
    }

    /**
     * Closes the environment and sets the env field to null.
     * Used for closing and reopening the environment.
     */
    public void closeEnv()
        throws DatabaseException {

        if (env != null) {
            close(env);
            env = null;
        }
    }

    /**
     * Opens the environment based on the txnType for this test case.
     * Used for closing and reopening the environment.
     */
    public void openEnv()
        throws DatabaseException {

        if (txnType == TXN_NULL) {
            TestEnv.BDB.copyConfig(envConfig);
            env = create(envHome, envConfig);
        } else if (txnType == TXN_AUTO) {
            TestEnv.TXN.copyConfig(envConfig);
            env = create(envHome, envConfig);
        } else if (txnType == TXN_USER) {
            TestEnv.TXN.copyConfig(envConfig);
            env = create(envHome, envConfig);
        } else if (txnType == TXN_CDB) {
            TestEnv.CDB.copyConfig(envConfig);
            env = create(envHome, envConfig);
        } else {
            assert false;
        }
    }

    /**
     * Begin a txn if in TXN_USER mode; otherwise return null;
     */
    protected Transaction txnBegin()
        throws DatabaseException {

        return txnBegin(null, null);
    }

    /**
     * Begin a txn if in TXN_USER mode; otherwise return null;
     */
    protected Transaction txnBegin(Transaction parentTxn,
                                   TransactionConfig config)
        throws DatabaseException {

        if (txnType == TXN_USER) {
            return env.beginTransaction(parentTxn, config);
        }
        return null;
    }

    /**
     * Begin a txn if in TXN_USER or TXN_AUTO mode; otherwise return null;
     */
    protected Transaction txnBeginCursor()
        throws DatabaseException {

        return txnBeginCursor(null, null);
    }

    /**
     * Begin a txn if in TXN_USER or TXN_AUTO mode; otherwise return null;
     */
    protected Transaction txnBeginCursor(Transaction parentTxn,
                                         TransactionConfig config)
        throws DatabaseException {

        if (txnType == TXN_USER || txnType == TXN_AUTO) {
            return env.beginTransaction(parentTxn, config);
        } else {
            return null;
        }
    }
    
    /**
     * Create a write cursor config;
     */
    public CursorConfig getWriteCursorConfig() {
        if (txnType != TXN_CDB) {
            return null;
        }
        final CursorConfig config = new CursorConfig();
        DbCompat.setWriteCursor(config, true);
        return config;
    } 

    /**
     * Commit a txn if non-null.
     */
    protected void txnCommit(Transaction txn)
        throws DatabaseException {

        if (txn != null) {
            txn.commit();
        }
    }

    /**
     * Commit a txn if non-null.
     */
    protected void txnAbort(Transaction txn)
        throws DatabaseException {

        if (txn != null) {
            txn.abort();
        }
    }
}
