/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

using BerkeleyDB.Internal;

namespace BerkeleyDB {
    /// <summary>
    /// Configuration parameters for hot backup.
    /// </summary>
    public class BackupOptions {
        /// <summary>
        /// If true, all files are removed from the target backup directory 
        /// before the backup is performed.
        /// </summary>
        public bool Clean;
        /// <summary>
        /// If true, all ordinary files that might exist in the environment or
        /// in the environment's subdirectories are backed up; otherwise, only files
        /// necessary for the proper operation of Berkeley DB are backed up.
        /// </summary>
        public bool Files;
        /// <summary>
        /// If true, log files are not included in the backup. Instead, only 
        /// *.db files are copied to the target directory.
        /// </summary>
        public bool NoLogs;
        /// <summary>
        /// If true, then regardless of the directory structure used by the 
        /// source environment, place all backup files in the single target 
        /// directory.  Use this option if absolute path names to your 
        /// environment directory and the files within that directory are 
        /// required by your application. 
        /// </summary>
        public bool SingleDir;
        /// <summary>
        /// If true, perform an incremental back up, instead of a full backup.
        /// When this option is specified, only log files are copied to the 
        /// target directory. 
        /// </summary>
        public bool Update;
        /// <summary>
        /// Specify whether the target directory will be created if it does not
        /// already exist.
        /// </summary>
        public CreatePolicy Creation = CreatePolicy.NEVER;

        internal uint flags {
            get {
                uint ret = 0;
                ret |= Clean ? DbConstants.DB_BACKUP_CLEAN : 0;
                ret |= Files ? DbConstants.DB_BACKUP_FILES : 0;
                ret |= NoLogs ? DbConstants.DB_BACKUP_NO_LOGS : 0;
                ret |= SingleDir ? DbConstants.DB_BACKUP_SINGLE_DIR : 0;
                ret |= Update ? DbConstants.DB_BACKUP_UPDATE : 0;
                ret |= (uint)Creation;
                return ret;
            }
        }
    }
}
