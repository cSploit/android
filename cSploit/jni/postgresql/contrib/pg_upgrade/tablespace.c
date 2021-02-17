/*
 *	tablespace.c
 *
 *	tablespace functions
 *
 *	Copyright (c) 2010-2011, PostgreSQL Global Development Group
 *	contrib/pg_upgrade/tablespace.c
 */

#include "pg_upgrade.h"

static void get_tablespace_paths(void);
static void set_tablespace_directory_suffix(ClusterInfo *cluster);


void
init_tablespaces(void)
{
	get_tablespace_paths();

	set_tablespace_directory_suffix(&old_cluster);
	set_tablespace_directory_suffix(&new_cluster);

	if (os_info.num_tablespaces > 0 &&
	strcmp(old_cluster.tablespace_suffix, new_cluster.tablespace_suffix) == 0)
		pg_log(PG_FATAL,
			   "Cannot upgrade to/from the same system catalog version when\n"
			   "using tablespaces.\n");
}


/*
 * get_tablespace_paths()
 *
 * Scans pg_tablespace and returns a malloc'ed array of all tablespace
 * paths. Its the caller's responsibility to free the array.
 */
static void
get_tablespace_paths(void)
{
	PGconn	   *conn = connectToServer(&old_cluster, "template1");
	PGresult   *res;
	int			tblnum;
	int			i_spclocation;

	res = executeQueryOrDie(conn,
							"SELECT	spclocation "
							"FROM	pg_catalog.pg_tablespace "
							"WHERE	spcname != 'pg_default' AND "
							"		spcname != 'pg_global'");

	if ((os_info.num_tablespaces = PQntuples(res)) != 0)
		os_info.tablespaces = (char **) pg_malloc(
								   os_info.num_tablespaces * sizeof(char *));
	else
		os_info.tablespaces = NULL;

	i_spclocation = PQfnumber(res, "spclocation");

	for (tblnum = 0; tblnum < os_info.num_tablespaces; tblnum++)
		os_info.tablespaces[tblnum] = pg_strdup(
									 PQgetvalue(res, tblnum, i_spclocation));

	PQclear(res);

	PQfinish(conn);

	return;
}


static void
set_tablespace_directory_suffix(ClusterInfo *cluster)
{
	if (GET_MAJOR_VERSION(cluster->major_version) <= 804)
		cluster->tablespace_suffix = pg_strdup("");
	else
	{
		/* This cluster has a version-specific subdirectory */
		cluster->tablespace_suffix = pg_malloc(4 +
										 strlen(cluster->major_version_str) +
											   10 /* OIDCHARS */ + 1);

		/* The leading slash is needed to start a new directory. */
		sprintf(cluster->tablespace_suffix, "/PG_%s_%d", cluster->major_version_str,
				cluster->controldata.cat_ver);
	}
}
