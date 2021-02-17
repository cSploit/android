/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates. All rights reserved.
 *
 * $Id: LogVerifyConfig.java,v 0f73af5ae3da 2010/05/10 05:38:40 alexander $
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.Db;
import com.sleepycat.db.internal.DbConstants;
import com.sleepycat.db.internal.DbEnv;
import com.sleepycat.db.internal.DbTxn;
import com.sleepycat.db.internal.DbUtil;
import java.util.Date;

/**
Specify the attributes of a database.
*/
public class LogVerifyConfig implements Cloneable {
    private String envHome = null;
    private String dbFile = null, dbName = null;
    private LogSequenceNumber startLsn = null, endLsn = null;
    private Date startTime = null, endTime = null;
    private boolean continueAfterFail = true, verbose = false;
    private int cachesz = 0;

    public LogVerifyConfig() {
    }
 
    /**
    Return the home directory to use by the log verification internally.
    <p>
    @return
    Return the environment home directory path.
    */
    public String getEnvHome() {
        return envHome;
    }

    /**
    Set the home directory to use by the log verification internally.
    <p>
    @param home
    The home directory of the internal database environment we use to store
    intermediate data during the verification process, can be null, meaning
    we will create the environment and all databases in memory, which can be
    overwhelming if the log takes up more space than available memory.
    <p>
    @throws IllegalArgumentException if home is an empty string.
    */
    public void setEnvHome(String home) {
        if (home != null && home.equals(""))
            throw new IllegalArgumentException(
                "The environment home directory to verify is invalid.");
        envHome = home;
    }

    /**
    Return the internal database environment cache size.
    <p>
    @return
    Return the internal database environment cache size.
    */
    public int getCacheSize() {
        return cachesz;
    }

    /**
    Set the internal database environment cache size.
    <p>
    @param cachesize
    The internal database environment cache size.
    */
    public void setCacheSize(int cachesize) {
        cachesz = cachesize;
    }
    
    /**
    Return the database file name whose logs we will verify.
    <p>
    @return
    Return the database file name whose logs we will verify.
    */
    public String getDbFile() {
        return dbFile;
    }

    /**
    Set the database file name whose logs we will verify.
    <p>
    @param file
    The database file name whose logs we will verify.
    */
    public void setDbFile(String file) {
        dbFile = file;
    }


    /**
    Return the database name whose logs we will verify.
    <p>
    @return
    Return the database name whose logs we will verify.
    */
    public String getDbName() {
        return dbName;
    }

    /**
    Set the database name whose logs we will verify.
    <p>
    @param name
    The database name whose logs we will verify.
    */
    public void setDbName(String name) {
        dbName = name;
    }

    /**
    Return the starting lsn to verify.
    <p>
    @return
    The starting lsn to verify.
    */
    public LogSequenceNumber getStartLsn() {
        return startLsn;
    }

    /**
    Set the starting lsn to verify.
    <p>
    @param stLsn
    The starting lsn to verify.
    */
    public void setStartLsn(LogSequenceNumber stLsn) {
        startLsn = new LogSequenceNumber(stLsn.getFile(), stLsn.getOffset());
    }

    /**
    Return the ending lsn to verify.
    <p>
    @return
    The ending lsn to verify.
    */
    public LogSequenceNumber getEndLsn() {
        return endLsn;
    }

    /**
    Set the ending lsn to verify.
    <p>
    @param endlsn
    The ending lsn to verify.
    */
    public void setEndLsn(LogSequenceNumber endlsn) {
        endLsn = new LogSequenceNumber(endlsn.getFile(), endlsn.getOffset());
    }

    /**
    Return the starting time to verify.
    <p>
    @return
    The starting time to verify.
    */
    public Date getStartTime() {
        return startTime;
    }

    /**
    Set the starting time to verify.
    <p>
    @param stime
    The starting time to verify.
    */
    public void setStartTime(Date stime) {
        startTime = stime;
    }

    /**
    Return the ending time to verify.
    <p>
    @return
    The ending time to verify.
    */
    public Date getEndTime() {
        return endTime;
    }

    /**
    Set the ending lsn to verify.
    <p>
    @param etime
    The ending time to verify.
    */
    public void setEndTime(Date etime) {
        endTime = etime;
    }

    /**
    Return if continue after a failed check.
    <p>
    @return
    If continue after a failed check.
    */
    public boolean getContinueAfterFail() {
        return continueAfterFail;
    }

    /**
    Set if continue after a failed check.
    <p>
    @param caf
    If continue after a failed check.
    */
    public void setContinueAfterFail(boolean caf) {
        continueAfterFail = caf;
    }

    /**
    Return if print verbose information.
    <p>
    @return
    If print verbose information.
    */
    public boolean getVerbose() {
        return verbose;
    }

    /**
    Set if print verbose information.
    <p>
    @param verbs
    If print verbose information.
    */
    public void setVerbose(boolean verbs) {
        verbose = verbs;
    }

}

