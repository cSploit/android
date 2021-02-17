/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

import com.sleepycat.db.internal.DbConstants;

/**
Options for {@link Environment#backup} operations.
*/
public class BackupOptions implements Cloneable {
    /**
    Default configuration used if null is passed to methods that create a
    cursor.
    */
    public static final BackupOptions DEFAULT = new BackupOptions();

    /* package */
    static BackupOptions checkNull(BackupOptions opt) {
        return (opt == null) ? DEFAULT : opt;
    }

    private boolean clean = false;
    private boolean files = false;
    private boolean nologs = false;
    private boolean singledir = false;
    private boolean update = false;
    private boolean allowcreate = false;
    private boolean exclusivecreate = false;

    /**
    Construct a default options object for backup operations.
    */
    public BackupOptions() {
    }

    /**
     Sets whether to clean the target backup directory.
     <p>
     @param clean
     If true, all files are removed from the target backup directory before
     the back up is performed.
    */
    public void setClean(final boolean clean) {
        this.clean = clean;
    }
    /**
     Whether the target backup directory should be cleaned.
     <p>
     @return true if the all files are removed from the target backup directory
     before performing the backup.
    */
    public boolean getClean() {
        return clean;
    }

    /**
     Sets whether all ordinary files will be backed up.
     <p>
     @param files
     If true, all ordinary files that might exist in the environment, as well 
     as might exist in the environment's subdirectories, are backed up; otherwise,
     only files necessary for the proper operation of Berkeley DB are backed up.
    */
    public void setFiles(final boolean files) {
        this.files = files;
    }
    /**
     Whether ordinary files are included in the hot backup.
     <p>
     @return true if ordinary files are included in the backup.
    */
    public boolean getFiles() {
        return files;
    }

    /**
    Sets whether log files are skipped in the backup.
    <p>
    @param nologs
    If true, log files are not included in the backup. Instead, only *.db
    files are copied to the target directory.
    */
    public void setNoLogs(final boolean nologs) {
        this.nologs = nologs;
    }
    /**
     Whether log files are included in the backup.
     <p>
     @return
     true if log files will not be included in the backup. Instead, only *.db
     files are copied to the target directory.
    */
    public boolean getNoLogs() {
        return nologs;
    }

    /**
     Sets whether all backed up files are to be placed in a single directory.
     <p>
     @param singledir
     If true, then regardless of the directory structure used by the source
     environment, place all back up files in the single target directory
     Use this option if absolute path names to your environment directory
     and the files within that directory are required by your application. 
    */
    public void setSingleDir(final boolean singledir) {
        this.singledir = singledir;
    }
    /**
     Whether all backed up files are to be placed in a single directory.
     <p>
     @return
     true if all the backup files will be placed in a single target 
     directory regardless of the directory structure used by the source
     environment.
    */
    public boolean getSingleDir() {
        return singledir;
    }

    /**
     Sets whether to perform an incremental back up.
     <p>
     @param update
     If true, perform an incremental back up, instead of a full back up.
     When this option is specified, only log files are copied to the target
     directory. 
    */
    public void setUpdate(final boolean update) {
        this.update = update;
    }
    /**
     Whether an incremental backup will be performed.
     <p>
     @return
     true if an incremental backup, instead of a full backup, will be performed.
     Also, if true, then only log files are copied to the target directory.
    */
    public boolean getUpdate() {
        return update;
    }

    /**
     Sets whether the target directory will be created if it does not
     already exist.
     <p>
     @param allowcreate
     If true, then create the target directory and any subdirectories as 
     required to perform the backup; otherwise, any missing directories
     will result in the backup operation throwing a 
     {@link com.sleepycat.db.DatabaseException DatabaseException} exception.
    */
    public void setAllowCreate(final boolean allowcreate) {
        this.allowcreate = allowcreate;
    }
    /**
     Whether the target directory will be created if it does not already exist.
     <p>
     @return
     true if the backup operation will create the target directory and any
     subdirectories as required to perform the backup; otherwise, any
     missing directories will result in the backup operation throwing a 
     {@link com.sleepycat.db.DatabaseException DatabaseException} exception.
    */
    public boolean getAllowCreate() {
        return allowcreate;
    }

    /**
     Sets whether the target directory will be exclusively created.
     <p>
     @param exclusivecreate
     If true, then the backup operation will create the target directory and 
     any required subdirectories. If the target directory currently exists,
     the backup operation will throw a
     {@link com.sleepycat.db.DatabaseException DatabaseException} exception.
    */
    public void setExclusiveCreate(final boolean exclusivecreate) {
        this.exclusivecreate = exclusivecreate;
    }
    /**
     Whether the target directory will be exclusively created.
     <p>
     @return
     true if the backup operation will create the target directory and 
     any required subdirectories. If the target directory currently exists,
     the backup operation will throw a
     {@link com.sleepycat.db.DatabaseException DatabaseException} exception.
    */
    public boolean getExclusiveCreate() {
        return exclusivecreate;
    }

    /* package */
    int getFlags() {
        int flags = 0;
        flags |=  clean ? DbConstants.DB_BACKUP_CLEAN : 0;
        flags |=  files ? DbConstants.DB_BACKUP_FILES : 0;
        flags |=  nologs ? DbConstants.DB_BACKUP_NO_LOGS : 0;
        flags |=  singledir ? DbConstants.DB_BACKUP_SINGLE_DIR : 0;
        flags |=  update ? DbConstants.DB_BACKUP_UPDATE : 0;
        flags |=  allowcreate ? DbConstants.DB_CREATE : 0;
        flags |=  exclusivecreate ? DbConstants.DB_EXCL : 0;

        return flags;
    }
}
