/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    /// <summary>
    /// An interface that can optionally be used to override the default 
    /// behavior performed by the <see cref="DatabaseEnvironment.Backup"/> and 
    /// <see cref="DatabaseEnvironment.BackupDatabase"/> methods. Implementation 
    /// of this interface is required if the DatabaseEnvironment.backup or 
    /// DatabaseEnvironment.backupDatabase target parameter is null.  You 
    /// configure the environment with this handler via 
    /// <see cref="DatabaseEnvironment.BackupHandler"/>.
    /// </summary>
    public interface IBackup {
        /// <summary>
        /// Called when ending a backup and closing a backup target.
        /// </summary>
        /// <param name="dbname">The name of the database that has been backed 
        /// up.</param>
        /// <returns>0 on success, non-zero on error.</returns>
        int Close(string dbname);
        /// <summary>
        /// Called when a target location is opened during a backup. This method
        /// should do whatever is necessary to prepare the backup destination 
        /// for writing the data.
        /// </summary>
        /// <param name="dbname">The backup's directory destination.</param>
        /// <param name="target">The name of the database being backed up.</param>
        /// <returns>0 on success, non-zero on error.</returns>
        int Open(string dbname, string target);
        /// <summary>
        /// Called when writing data during a backup. 
        /// </summary>
        /// <param name="data">Identifies the buffer which contains the data to
        /// be backed up.</param>
        /// <param name="offset">Identifies the position in the file where 
        /// bytes from buf should be written.</param>
        /// <param name="count">Identifies the number of bytes to write to the 
        /// file from the buffer.</param>
        /// <returns>0 on success, non-zero on error.</returns>
        int Write(byte[] data, long offset, int count);
    }
}
