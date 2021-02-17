/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$ 
 */

// File TxnGuideInMemory.java

package db.txn;

import com.sleepycat.bind.serial.StoredClassCatalog;

import com.sleepycat.db.Database;
import com.sleepycat.db.DatabaseConfig;
import com.sleepycat.db.DatabaseException;
import com.sleepycat.db.DatabaseType;
import com.sleepycat.db.LockDetectMode;

import com.sleepycat.db.Environment;
import com.sleepycat.db.EnvironmentConfig;

import java.io.File;
import java.io.FileNotFoundException;

public class TxnGuideInMemory {

    // DB handles
    private static Database myDb = null;
    private static Database myClassDb = null;
    private static Environment myEnv = null;

    private static int NUMTHREADS = 5;

    public static void main(String args[]) {
        try {
            // Open the environment and databases
            openEnv();

            // Get our class catalog (used to serialize objects)
            StoredClassCatalog classCatalog =
                new StoredClassCatalog(myClassDb);

            // Start the threads
            DBWriter[] threadArray;
            threadArray = new DBWriter[NUMTHREADS];
            for (int i = 0; i < NUMTHREADS; i++) {
                threadArray[i] = new DBWriter(myEnv, myDb, classCatalog, true);
                threadArray[i].start();
            }

            System.out.println("Threads started.\n");

            for (int i = 0; i < NUMTHREADS; i++) {
                threadArray[i].join();
            }
        } catch (Exception e) {
            System.err.println("TxnGuideInMemory: " + e.toString());
            e.printStackTrace();
        } finally {
            closeEnv();
        }
        System.out.println("All done.");
    }

    private static void openEnv() throws DatabaseException {
        System.out.println("opening env");

        // Set up the environment.
        EnvironmentConfig myEnvConfig = new EnvironmentConfig();

        // Region files are not backed by the filesystem, they are
        // backed by heap memory.
        myEnvConfig.setPrivate(true);
        myEnvConfig.setAllowCreate(true);
        myEnvConfig.setInitializeCache(true);
        myEnvConfig.setInitializeLocking(true);
        myEnvConfig.setInitializeLogging(true);
        myEnvConfig.setThreaded(true);

        myEnvConfig.setTransactional(true);
        // EnvironmentConfig.setThreaded(true) is the default behavior
        // in Java, so we do not have to do anything to cause the
        // environment handle to be free-threaded.

        // Indicate that we want db to internally perform deadlock
        // detection. Also indicate that the transaction that has
        // performed the least amount of write activity to
        // receive the deadlock notification, if any.
        myEnvConfig.setLockDetectMode(LockDetectMode.MINWRITE);

        // Specify in-memory logging
        myEnvConfig.setLogInMemory(true);
        // Specify the size of the in-memory log buffer
        // Must be large enough to handle the log data created by
        // the largest transaction.
        myEnvConfig.setLogBufferSize(10 * 1024 * 1024);
        // Specify the size of the in-memory cache
        // Set it large enough so that it won't page.
        myEnvConfig.setCacheSize(10 * 1024 * 1024);

        // Set up the database
        DatabaseConfig myDbConfig = new DatabaseConfig();
        myDbConfig.setType(DatabaseType.BTREE);
        myDbConfig.setAllowCreate(true);
        myDbConfig.setTransactional(true);
        myDbConfig.setSortedDuplicates(true);
        // no DatabaseConfig.setThreaded() method available.
        // db handles in java are free-threaded so long as the
        // env is also free-threaded.

        try {
            // Open the environment
            myEnv = new Environment(null,    // Env home
                                    myEnvConfig);

            // Open the database. Do not provide a txn handle. This open
            // is autocommitted because DatabaseConfig.setTransactional()
            // is true.
            myDb = myEnv.openDatabase(null,     // txn handle
                                      null,     // Database file name
                                      null,     // Database name
                                      myDbConfig);

            // Used by the bind API for serializing objects
            // Class database must not support duplicates
            myDbConfig.setSortedDuplicates(false);
            myClassDb = myEnv.openDatabase(null,     // txn handle
                                           null,     // Database file name
                                           null,     // Database name,
                                           myDbConfig);
        } catch (FileNotFoundException fnfe) {
            System.err.println("openEnv: " + fnfe.toString());
            System.exit(-1);
        }
    }

    private static void closeEnv() {
        System.out.println("Closing env");
        if (myDb != null ) {
            try {
                myDb.close();
            } catch (DatabaseException e) {
                System.err.println("closeEnv: myDb: " +
                    e.toString());
                e.printStackTrace();
            }
        }

        if (myClassDb != null ) {
            try {
                myClassDb.close();
            } catch (DatabaseException e) {
                System.err.println("closeEnv: myClassDb: " +
                    e.toString());
                e.printStackTrace();
            }
        }

        if (myEnv != null ) {
            try {
                myEnv.close();
            } catch (DatabaseException e) {
                System.err.println("closeEnv: " + e.toString());
                e.printStackTrace();
            }
        }
    }

    private TxnGuideInMemory() {}
}
