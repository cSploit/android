/*
 *	server.c
 *
 *	database server functions
 *
 *	Copyright (c) 2010-2011, PostgreSQL Global Development Group
 *	contrib/pg_upgrade/server.c
 */

#include "pg_upgrade.h"


static PGconn *get_db_conn(ClusterInfo *cluster, const char *db_name);


/*
 * connectToServer()
 *
 *	Connects to the desired database on the designated server.
 *	If the connection attempt fails, this function logs an error
 *	message and calls exit() to kill the program.
 */
PGconn *
connectToServer(ClusterInfo *cluster, const char *db_name)
{
	PGconn	   *conn = get_db_conn(cluster, db_name);

	if (conn == NULL || PQstatus(conn) != CONNECTION_OK)
	{
		pg_log(PG_REPORT, "connection to database failed: %s\n",
			   PQerrorMessage(conn));

		if (conn)
			PQfinish(conn);

		printf("Failure, exiting\n");
		exit(1);
	}

	return conn;
}


/*
 * get_db_conn()
 *
 * get database connection
 */
static PGconn *
get_db_conn(ClusterInfo *cluster, const char *db_name)
{
	char		conn_opts[MAXPGPATH];

	snprintf(conn_opts, sizeof(conn_opts),
			 "dbname = '%s' user = '%s' port = %d", db_name, os_info.user,
			 cluster->port);

	return PQconnectdb(conn_opts);
}


/*
 * executeQueryOrDie()
 *
 *	Formats a query string from the given arguments and executes the
 *	resulting query.  If the query fails, this function logs an error
 *	message and calls exit() to kill the program.
 */
PGresult *
executeQueryOrDie(PGconn *conn, const char *fmt,...)
{
	static char command[8192];
	va_list		args;
	PGresult   *result;
	ExecStatusType status;

	va_start(args, fmt);
	vsnprintf(command, sizeof(command), fmt, args);
	va_end(args);

	pg_log(PG_DEBUG, "executing: %s\n", command);
	result = PQexec(conn, command);
	status = PQresultStatus(result);

	if ((status != PGRES_TUPLES_OK) && (status != PGRES_COMMAND_OK))
	{
		pg_log(PG_REPORT, "DB command failed\n%s\n%s\n", command,
			   PQerrorMessage(conn));
		PQclear(result);
		PQfinish(conn);
		printf("Failure, exiting\n");
		exit(1);
	}
	else
		return result;
}


/*
 * get_major_server_version()
 *
 * gets the version (in unsigned int form) for the given "datadir". Assumes
 * that datadir is an absolute path to a valid pgdata directory. The version
 * is retrieved by reading the PG_VERSION file.
 */
uint32
get_major_server_version(ClusterInfo *cluster)
{
	const char *datadir = cluster->pgdata;
	FILE	   *version_fd;
	char		ver_filename[MAXPGPATH];
	int			integer_version = 0;
	int			fractional_version = 0;

	snprintf(ver_filename, sizeof(ver_filename), "%s/PG_VERSION", datadir);
	if ((version_fd = fopen(ver_filename, "r")) == NULL)
		return 0;

	if (fscanf(version_fd, "%63s", cluster->major_version_str) == 0 ||
		sscanf(cluster->major_version_str, "%d.%d", &integer_version,
			   &fractional_version) != 2)
		pg_log(PG_FATAL, "could not get version from %s\n", datadir);

	fclose(version_fd);

	return (100 * integer_version + fractional_version) * 100;
}


static void
#ifdef HAVE_ATEXIT
stop_postmaster_atexit(void)
#else
stop_postmaster_on_exit(int exitstatus, void *arg)
#endif
{
	stop_postmaster(true);

}


void
start_postmaster(ClusterInfo *cluster)
{
	char		cmd[MAXPGPATH];
	PGconn	   *conn;
	bool		exit_hook_registered = false;
	int			pg_ctl_return = 0;

#ifndef WIN32
	char	   *output_filename = log_opts.filename;
#else

	/*
	 * On Win32, we can't send both pg_upgrade output and pg_ctl output to the
	 * same file because we get the error: "The process cannot access the file
	 * because it is being used by another process." so we have to send all
	 * other output to 'nul'.
	 */
	char	   *output_filename = DEVNULL;
#endif

	if (!exit_hook_registered)
	{
#ifdef HAVE_ATEXIT
		atexit(stop_postmaster_atexit);
#else
		on_exit(stop_postmaster_on_exit);
#endif
		exit_hook_registered = true;
	}

	/*
	 * Using autovacuum=off disables cleanup vacuum and analyze, but freeze
	 * vacuums can still happen, so we set autovacuum_freeze_max_age to its
	 * maximum.  We assume all datfrozenxid and relfrozen values are less than
	 * a gap of 2000000000 from the current xid counter, so autovacuum will
	 * not touch them.
	 */
	snprintf(cmd, sizeof(cmd),
			 SYSTEMQUOTE "\"%s/pg_ctl\" -w -l \"%s\" -D \"%s\" "
			 "-o \"-p %d %s\" start >> \"%s\" 2>&1" SYSTEMQUOTE,
			 cluster->bindir, output_filename, cluster->pgdata, cluster->port,
			 (cluster->controldata.cat_ver >=
			  BINARY_UPGRADE_SERVER_FLAG_CAT_VER) ? "-b" :
			 "-c autovacuum=off -c autovacuum_freeze_max_age=2000000000",
			 output_filename);

	/*
	 * Don't throw an error right away, let connecting throw the error because
	 * it might supply a reason for the failure.
	 */
	pg_ctl_return = exec_prog(false, "%s", cmd);

	/* Check to see if we can connect to the server; if not, report it. */
	if ((conn = get_db_conn(cluster, "template1")) == NULL ||
		PQstatus(conn) != CONNECTION_OK)
	{
		pg_log(PG_REPORT, "\nconnection to database failed: %s\n",
			   PQerrorMessage(conn));
		if (conn)
			PQfinish(conn);
		pg_log(PG_FATAL, "unable to connect to %s postmaster started with the command: %s\n",
			   CLUSTER_NAME(cluster), cmd);
	}
	PQfinish(conn);

	/* If the connection didn't fail, fail now */
	if (pg_ctl_return != 0)
		pg_log(PG_FATAL, "pg_ctl failed to start the %s server\n",
			   CLUSTER_NAME(cluster));

	os_info.running_cluster = cluster;
}


void
stop_postmaster(bool fast)
{
	char		cmd[MAXPGPATH];
	const char *bindir;
	const char *datadir;

#ifndef WIN32
	char	   *output_filename = log_opts.filename;
#else
	/* See comment in start_postmaster() about why win32 output is ignored. */
	char	   *output_filename = DEVNULL;
#endif

	if (os_info.running_cluster == &old_cluster)
	{
		bindir = old_cluster.bindir;
		datadir = old_cluster.pgdata;
	}
	else if (os_info.running_cluster == &new_cluster)
	{
		bindir = new_cluster.bindir;
		datadir = new_cluster.pgdata;
	}
	else
		return;					/* no cluster running */

	snprintf(cmd, sizeof(cmd),
			 SYSTEMQUOTE "\"%s/pg_ctl\" -w -l \"%s\" -D \"%s\" %s stop >> "
			 "\"%s\" 2>&1" SYSTEMQUOTE,
			 bindir, output_filename, datadir, fast ? "-m fast" : "",
			 output_filename);

	exec_prog(fast ? false : true, "%s", cmd);

	os_info.running_cluster = NULL;
}


/*
 * check_pghost_envvar()
 *
 * Tests that PGHOST does not point to a non-local server
 */
void
check_pghost_envvar(void)
{
	PQconninfoOption *option;
	PQconninfoOption *start;

	/* Get valid libpq env vars from the PQconndefaults function */

	start = PQconndefaults();

	for (option = start; option->keyword != NULL; option++)
	{
		if (option->envvar && (strcmp(option->envvar, "PGHOST") == 0 ||
							   strcmp(option->envvar, "PGHOSTADDR") == 0))
		{
			const char *value = getenv(option->envvar);

			if (value && strlen(value) > 0 &&
			/* check for 'local' host values */
				(strcmp(value, "localhost") != 0 && strcmp(value, "127.0.0.1") != 0 &&
				 strcmp(value, "::1") != 0 && value[0] != '/'))
				pg_log(PG_FATAL,
					   "libpq environment variable %s has a non-local server value: %s\n",
					   option->envvar, value);
		}
	}

	/* Free the memory that libpq allocated on our behalf */
	PQconninfoFree(start);
}
