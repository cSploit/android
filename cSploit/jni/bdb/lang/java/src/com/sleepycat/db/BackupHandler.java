/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

package com.sleepycat.db;

/**
An interface that can optionally be used to override the default behavior
performed by the
{@link com.sleepycat.db.Environment#backup Environment.backup}
and
{@link com.sleepycat.db.Environment#backupDatabase Environment.backupDatabase}
methods. Implementation of this interface is required if the 
Environment.backup or Environment.backupDatabase target parameter is null.
<p>
You configure the environment with this handler using the
{@link com.sleepycat.db.EnvironmentConfig#setBackupHandler EnvironmentConfig.setBackupHandler}
method.
*/


public interface BackupHandler {
    /**
     Called when ending a backup and closing a backup target.
     <p>
     @param dbname
     The name of the database that has been backed up.
     */
    int close(String dbname);
    /**
     Called when a target location is opened during a backup. This method
     should do whatever is necessary to prepare the backup destination for
     writing the data. 
     <p>
     @param target
     The backup's directory destination.
     <p>
     @param dbname
     The name of the database being backed up.
     */
    int open(String target, String dbname);
    /**
     Called when writing data during a backup. 
     <p>
     @param file_pos
     Identifies the position in the target file where the bytes 
     from the buffer should be written.
     <p>
     @param buf
     Identifies the buffer which contains the data to be backed up.
     <p>
     @param off
     Specifies the start of the data in the buffer. This will always be
     zero.
     <p>
     @param len
     Identifies the number of bytes to back up from the buffer.
     */
    int write(long file_pos, byte[] buf, int off, int len);
}

