/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

namespace BerkeleyDB {
    /// <summary>
    /// A class representing configuration parameters for log verification.
    /// </summary>
    public class LogVerifyConfig {
	private String envhome;
        /// <summary>
        /// The home directory of the Berkeley DB environment to be used 
        /// internally during log verification.
        /// </summary>
        public String EnvHome {
            get { return envhome; }
            set {
                envhome = value;
            }
        }
	
	private uint cachesz;
        /// <summary>
        /// The cache size for the internal database environment.
        /// </summary>
        public uint CacheSize {
            get { return cachesz; }
            set {
                cachesz = value;
            }
        }

	private String dbfile;
        /// <summary>
        /// The database file name whose logs are to be verified.
        /// </summary>
        public String DbFile {
            get { return dbfile; }
            set {
                dbfile = value;
            }
        }

	private String dbname;
        /// <summary>
        /// The database name whose logs are to be verified.
        /// </summary>
        public String DbName {
            get { return dbname; }
            set {
                dbname = value;
            }
        }

	private LSN startlsn;
        /// <summary>
        /// The starting lsn to verify.
        /// </summary>
        public LSN StartLsn {
            get { return startlsn; }
            set {
                startlsn.LogFileNumber = value.LogFileNumber;
                startlsn.Offset = value.Offset;
            }
        }

	private LSN endlsn;
        /// <summary>
        /// The ending lsn to verify.
        /// </summary>
        public LSN EndLsn {
            get { return endlsn; }
            set {
                endlsn.LogFileNumber = value.LogFileNumber;
                endlsn.Offset = value.Offset;
            }
        }
	private DateTime stime;
        /// <summary>
        /// The starting time to verify.
        /// </summary>
        public DateTime StartTime {
            get { return stime; }
            set {
                stime = value;
            }
        }
	private DateTime etime;
        /// <summary>
        /// The ending time to verify.
        /// </summary>
        public DateTime EndTime {
            get { return etime; }
            set {
                etime = value;
            }
        }
	private bool caf;
        /// <summary>
        /// Whether continue verifying after a failed check.
        /// </summary>
        public bool ContinueAfterFail {
            get { return caf; }
            set {
                caf = value;
            }
        }
	private bool verbose;
        /// <summary>
        /// Whether output verbose information.
        /// </summary>
        public bool Verbose {
            get { return verbose; }
            set {
                verbose = value;
            }
        }

        /// <summary>
        /// Instantiate a new LogVerifyConfig object, with the default
        /// configuration settings.
        /// </summary>
        public LogVerifyConfig() {
            envhome = null;
            dbfile = null;
            dbname = null;
            caf = true;
            verbose = false;
            startlsn = new LSN(0, 0);
            endlsn = new LSN(0, 0);
            cachesz = 0;
        }
    }
}
