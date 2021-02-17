/*-------------------------------------------------------------------------
 *
 * vacuumlo.c
 *	  This removes orphaned large objects from a database.
 *
 * Portions Copyright (c) 1996-2012, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  contrib/vacuumlo/vacuumlo.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "libpq-fe.h"

#define atooid(x)  ((Oid) strtoul((x), NULL, 10))

#define BUFSIZE			1024

extern char *optarg;
extern int	optind,
			opterr;

enum trivalue
{
	TRI_DEFAULT,
	TRI_NO,
	TRI_YES
};

struct _param
{
	char	   *pg_user;
	enum trivalue pg_prompt;
	char	   *pg_port;
	char	   *pg_host;
	int			verbose;
	int			dry_run;
	long		transaction_limit;
};

static int	vacuumlo(const char *database, const struct _param * param);
static void usage(const char *progname);



/*
 * This vacuums LOs of one database. It returns 0 on success, -1 on failure.
 */
static int
vacuumlo(const char *database, const struct _param * param)
{
	PGconn	   *conn;
	PGresult   *res,
			   *res2;
	char		buf[BUFSIZE];
	long		matched;
	long		deleted;
	int			i;
	static char *password = NULL;
	bool		new_pass;
	bool        success = true;

	/* Note: password can be carried over from a previous call */
	if (param->pg_prompt == TRI_YES && password == NULL)
		password = simple_prompt("Password: ", 100, false);

	/*
	 * Start the connection.  Loop until we have a password if requested by
	 * backend.
	 */
	do
	{
		new_pass = false;

		conn = PQsetdbLogin(param->pg_host,
							param->pg_port,
							NULL,
							NULL,
							database,
							param->pg_user,
							password);
		if (!conn)
		{
			fprintf(stderr, "Connection to database \"%s\" failed\n",
					database);
			return -1;
		}

		if (PQstatus(conn) == CONNECTION_BAD &&
			PQconnectionNeedsPassword(conn) &&
			password == NULL &&
			param->pg_prompt != TRI_NO)
		{
			PQfinish(conn);
			password = simple_prompt("Password: ", 100, false);
			new_pass = true;
		}
	} while (new_pass);

	/* check to see that the backend connection was successfully made */
	if (PQstatus(conn) == CONNECTION_BAD)
	{
		fprintf(stderr, "Connection to database \"%s\" failed:\n%s",
				database, PQerrorMessage(conn));
		PQfinish(conn);
		return -1;
	}

	if (param->verbose)
	{
		fprintf(stdout, "Connected to database \"%s\"\n", database);
		if (param->dry_run)
			fprintf(stdout, "Test run: no large objects will be removed!\n");
	}

	/*
	 * Don't get fooled by any non-system catalogs
	 */
	res = PQexec(conn, "SET search_path = pg_catalog");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "Failed to set search_path:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}
	PQclear(res);

	/*
	 * First we create and populate the LO temp table
	 */
	buf[0] = '\0';
	strcat(buf, "CREATE TEMP TABLE vacuum_l AS ");
	if (PQserverVersion(conn) >= 90000)
		strcat(buf, "SELECT oid AS lo FROM pg_largeobject_metadata");
	else
		strcat(buf, "SELECT DISTINCT loid AS lo FROM pg_largeobject");
	res = PQexec(conn, buf);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "Failed to create temp table:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}
	PQclear(res);

	/*
	 * Analyze the temp table so that planner will generate decent plans for
	 * the DELETEs below.
	 */
	buf[0] = '\0';
	strcat(buf, "ANALYZE vacuum_l");
	res = PQexec(conn, buf);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "Failed to vacuum temp table:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}
	PQclear(res);

	/*
	 * Now find any candidate tables that have columns of type oid.
	 *
	 * NOTE: we ignore system tables and temp tables by the expedient of
	 * rejecting tables in schemas named 'pg_*'.  In particular, the temp
	 * table formed above is ignored, and pg_largeobject will be too. If
	 * either of these were scanned, obviously we'd end up with nothing to
	 * delete...
	 *
	 * NOTE: the system oid column is ignored, as it has attnum < 1. This
	 * shouldn't matter for correctness, but it saves time.
	 */
	buf[0] = '\0';
	strcat(buf, "SELECT s.nspname, c.relname, a.attname ");
	strcat(buf, "FROM pg_class c, pg_attribute a, pg_namespace s, pg_type t ");
	strcat(buf, "WHERE a.attnum > 0 AND NOT a.attisdropped ");
	strcat(buf, "      AND a.attrelid = c.oid ");
	strcat(buf, "      AND a.atttypid = t.oid ");
	strcat(buf, "      AND c.relnamespace = s.oid ");
	strcat(buf, "      AND t.typname in ('oid', 'lo') ");
	strcat(buf, "      AND c.relkind = 'r'");
	strcat(buf, "      AND s.nspname !~ '^pg_'");
	res = PQexec(conn, buf);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "Failed to find OID columns:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}

	for (i = 0; i < PQntuples(res); i++)
	{
		char	   *schema,
				   *table,
				   *field;

		schema = PQgetvalue(res, i, 0);
		table = PQgetvalue(res, i, 1);
		field = PQgetvalue(res, i, 2);

		if (param->verbose)
			fprintf(stdout, "Checking %s in %s.%s\n", field, schema, table);

		schema = PQescapeIdentifier(conn, schema, strlen(schema));
		table = PQescapeIdentifier(conn, table, strlen(table));
		field = PQescapeIdentifier(conn, field, strlen(field));

		if (!schema || !table || !field)
		{
			fprintf(stderr, "Out of memory\n");
			PQclear(res);
			PQfinish(conn);
			return -1;
		}

		snprintf(buf, BUFSIZE,
				 "DELETE FROM vacuum_l "
				 "WHERE lo IN (SELECT %s FROM %s.%s)",
				 field, schema, table);
		res2 = PQexec(conn, buf);
		if (PQresultStatus(res2) != PGRES_COMMAND_OK)
		{
			fprintf(stderr, "Failed to check %s in table %s.%s:\n",
					field, schema, table);
			fprintf(stderr, "%s", PQerrorMessage(conn));
			PQclear(res2);
			PQclear(res);
			PQfinish(conn);
			return -1;
		}
		PQclear(res2);

		PQfreemem(schema);
		PQfreemem(table);
		PQfreemem(field);
	}
	PQclear(res);

	/*
	 * Now, those entries remaining in vacuum_l are orphans.  Delete 'em.
	 *
	 * We don't want to run each delete as an individual transaction, because
	 * the commit overhead would be high.  However, since 9.0 the backend will
	 * acquire a lock per deleted LO, so deleting too many LOs per transaction
	 * risks running out of room in the shared-memory lock table.
	 * Accordingly, we delete up to transaction_limit LOs per transaction.
	 */
	res = PQexec(conn, "begin");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "Failed to start transaction:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}
	PQclear(res);

	buf[0] = '\0';
	strcat(buf, "SELECT lo FROM vacuum_l");
	res = PQexec(conn, buf);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "Failed to read temp table:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}

	matched = PQntuples(res);
	deleted = 0;
	for (i = 0; i < matched; i++)
	{
		Oid			lo = atooid(PQgetvalue(res, i, 0));

		if (param->verbose)
		{
			fprintf(stdout, "\rRemoving lo %6u   ", lo);
			fflush(stdout);
		}

		if (param->dry_run == 0)
		{
			if (lo_unlink(conn, lo) < 0)
			{
				fprintf(stderr, "\nFailed to remove lo %u: ", lo);
				fprintf(stderr, "%s", PQerrorMessage(conn));
				if (PQtransactionStatus(conn) == PQTRANS_INERROR)
				{
					success = false;
					break;
				}
			}
			else
				deleted++;
		}
		else
			deleted++;
		if (param->transaction_limit > 0 &&
			(deleted % param->transaction_limit) == 0)
		{
			res2 = PQexec(conn, "commit");
			if (PQresultStatus(res2) != PGRES_COMMAND_OK)
			{
				fprintf(stderr, "Failed to commit transaction:\n");
				fprintf(stderr, "%s", PQerrorMessage(conn));
				PQclear(res2);
				PQclear(res);
				PQfinish(conn);
				return -1;
			}
			PQclear(res2);
			res2 = PQexec(conn, "begin");
			if (PQresultStatus(res2) != PGRES_COMMAND_OK)
			{
				fprintf(stderr, "Failed to start transaction:\n");
				fprintf(stderr, "%s", PQerrorMessage(conn));
				PQclear(res2);
				PQclear(res);
				PQfinish(conn);
				return -1;
			}
			PQclear(res2);
		}
	}
	PQclear(res);

	/*
	 * That's all folks!
	 */
	res = PQexec(conn, "commit");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "Failed to commit transaction:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}
	PQclear(res);

	PQfinish(conn);

	if (param->verbose)
	{
		if (param->dry_run)
			fprintf(stdout, "\rWould remove %ld large objects from database \"%s\".\n",
					deleted, database);
		else if (success)
			fprintf(stdout,
					"\rSuccessfully removed %ld large objects from database \"%s\".\n",
					deleted, database);
		else
			fprintf(stdout, "\rRemoval from database \"%s\" failed at object %ld of %ld.\n",
					database, deleted, matched);
	}

	return ((param->dry_run || success) ? 0 : -1);
}

static void
usage(const char *progname)
{
	printf("%s removes unreferenced large objects from databases.\n\n", progname);
	printf("Usage:\n  %s [OPTION]... DBNAME...\n\n", progname);
	printf("Options:\n");
	printf("  -h HOSTNAME  database server host or socket directory\n");
	printf("  -l LIMIT     commit after removing each LIMIT large objects\n");
	printf("  -n           don't remove large objects, just show what would be done\n");
	printf("  -p PORT      database server port\n");
	printf("  -U USERNAME  user name to connect as\n");
	printf("  -w           never prompt for password\n");
	printf("  -W           force password prompt\n");
	printf("  -v           write a lot of progress messages\n");
	printf("  --help       show this help, then exit\n");
	printf("  --version    output version information, then exit\n");
	printf("\n");
	printf("Report bugs to <pgsql-bugs@postgresql.org>.\n");
}


int
main(int argc, char **argv)
{
	int			rc = 0;
	struct _param param;
	int			c;
	int			port;
	const char *progname;

	progname = get_progname(argv[0]);

	/* Set default parameter values */
	param.pg_user = NULL;
	param.pg_prompt = TRI_DEFAULT;
	param.pg_host = NULL;
	param.pg_port = NULL;
	param.verbose = 0;
	param.dry_run = 0;
	param.transaction_limit = 1000;

	/* Process command-line arguments */
	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			usage(progname);
			exit(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			puts("vacuumlo (PostgreSQL) " PG_VERSION);
			exit(0);
		}
	}

	while (1)
	{
		c = getopt(argc, argv, "h:l:U:p:vnwW");
		if (c == -1)
			break;

		switch (c)
		{
			case '?':
				fprintf(stderr, _("Try \"%s --help\" for more information.\n"), progname);
				exit(1);
			case ':':
				exit(1);
			case 'v':
				param.verbose = 1;
				break;
			case 'n':
				param.dry_run = 1;
				param.verbose = 1;
				break;
			case 'l':
				param.transaction_limit = strtol(optarg, NULL, 10);
				if (param.transaction_limit < 0)
				{
					fprintf(stderr,
				"%s: transaction limit must not be negative (0 disables)\n",
						progname);
					exit(1);
				}
				break;
			case 'U':
				param.pg_user = strdup(optarg);
				break;
			case 'w':
				param.pg_prompt = TRI_NO;
				break;
			case 'W':
				param.pg_prompt = TRI_YES;
				break;
			case 'p':
				port = strtol(optarg, NULL, 10);
				if ((port < 1) || (port > 65535))
				{
					fprintf(stderr, "%s: invalid port number: %s\n", progname, optarg);
					exit(1);
				}
				param.pg_port = strdup(optarg);
				break;
			case 'h':
				param.pg_host = strdup(optarg);
				break;
		}
	}

	/* No database given? Show usage */
	if (optind >= argc)
	{
		fprintf(stderr, "vacuumlo: missing required argument: database name\n");
		fprintf(stderr, _("Try \"%s --help\" for more information.\n"), progname);
		exit(1);
	}

	for (c = optind; c < argc; c++)
	{
		/* Work on selected database */
		rc += (vacuumlo(argv[c], &param) != 0);
	}

	return rc;
}
