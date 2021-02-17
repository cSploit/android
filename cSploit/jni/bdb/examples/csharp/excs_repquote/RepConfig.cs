/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */
using System;
using System.Collections.Generic;
using System.Text;

using BerkeleyDB;

namespace excs_repquote
{
	public class RepConfig
	{
		/* Constant values used in the RepQuote application. */
		public static CacheInfo CACHESIZE = new CacheInfo(0, 10 * 1024 * 1024, 1);
		public static int SLEEPTIME = 5000;
		public static string progname = "RepQuoteExample";

		/* Member variables containing configuration information. */
		public AckPolicy ackPolicy;
		public bool bulk; /* Whether bulk transfer should be performed. */
		public string home; /* The home directory for rep files. */
		public DbSiteConfig localSite;
		public uint priority; /* Priority within the replication group. */
		public List<DbSiteConfig> remoteSites;
		public StartPolicy startPolicy; 

		public bool verbose;

		public RepConfig()
		{
			ackPolicy = AckPolicy.QUORUM;
			bulk = false;
			home = "";
			localSite = new DbSiteConfig();
			priority = 100;
			remoteSites = new List<DbSiteConfig>();
			startPolicy = StartPolicy.ELECTION;

			verbose = false;
		}
	}

	public enum StartPolicy { CLIENT, ELECTION, MASTER };
}
