/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#ifndef lint
static const char copyright[] =
    "Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.\n";
#endif

int db_deadlock_main __P((int, char *[]));
int db_deadlock_usage __P((void));
int db_deadlock_version_check __P((void));

const char *progname;

int
db_deadlock(args)
	char *args;
{
	int argc;
	char **argv;

	__db_util_arg("db_deadlock", args, &argc, &argv);
	return (db_deadlock_main(argc, argv) ? EXIT_FAILURE : EXIT_SUCCESS);
}

#include <stdio.h>
#define	ERROR_RETURN	ERROR

int
db_deadlock_main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind, __db_getopt_reset;
	DB_ENV *dbenv;
	u_int32_t atype;
	time_t now;
	u_long secs, usecs;
	int rejected, ch, exitval, ret, verbose;
	char *home, *logfile, *passwd, *str, time_buf[CTIME_BUFLEN];

	if ((progname = __db_rpath(argv[0])) == NULL)
		progname = argv[0];
	else
		++progname;

	if ((ret = db_deadlock_version_check()) != 0)
		return (ret);

	dbenv = NULL;
	atype = DB_LOCK_DEFAULT;
	home = logfile = passwd = NULL;
	secs = usecs = 0;
	exitval = verbose = 0;
	__db_getopt_reset = 1;
	while ((ch = getopt(argc, argv, "a:h:L:P:t:Vv")) != EOF)
		switch (ch) {
		case 'a':
			switch (optarg[0]) {
			case 'e':
				atype = DB_LOCK_EXPIRE;
				break;
			case 'm':
				atype = DB_LOCK_MAXLOCKS;
				break;
			case 'n':
				atype = DB_LOCK_MINLOCKS;
				break;
			case 'o':
				atype = DB_LOCK_OLDEST;
				break;
			case 'W':
				atype = DB_LOCK_MAXWRITE;
				break;
			case 'w':
				atype = DB_LOCK_MINWRITE;
				break;
			case 'y':
				atype = DB_LOCK_YOUNGEST;
				break;
			default:
				return (db_deadlock_usage());
				/* NOTREACHED */
			}
			if (optarg[1] != '\0')
				return (db_deadlock_usage());
			break;
		case 'h':
			home = optarg;
			break;
		case 'L':
			logfile = optarg;
			break;
		case 'P':
			if (passwd != NULL) {
				fprintf(stderr, DB_STR("5136",
					"Password may not be specified twice"));
				free(passwd);
				return (EXIT_FAILURE);
			}
			passwd = strdup(optarg);
			memset(optarg, 0, strlen(optarg));
			if (passwd == NULL) {
				fprintf(stderr, DB_STR_A("5100",
				    "%s: strdup: %s\n", "%s %s\n"),
				    progname, strerror(errno));
				return (EXIT_FAILURE);
			}
			break;
		case 't':
			if ((str = strchr(optarg, '.')) != NULL) {
				*str++ = '\0';
				if (*str != '\0' && __db_getulong(
				    NULL, progname, str, 0, LONG_MAX, &usecs))
					return (EXIT_FAILURE);
			}
			if (*optarg != '\0' && __db_getulong(
			    NULL, progname, optarg, 0, LONG_MAX, &secs))
				return (EXIT_FAILURE);
			if (secs == 0 && usecs == 0)
				return (db_deadlock_usage());

			break;
		case 'V':
			printf("%s\n", db_version(NULL, NULL, NULL));
			return (EXIT_SUCCESS);
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			return (db_deadlock_usage());
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		return (db_deadlock_usage());

	/* Handle possible interruptions. */
	__db_util_siginit();

	/* Log our process ID. */
	if (logfile != NULL && __db_util_logset(progname, logfile))
		goto err;

	/*
	 * Create an environment object and initialize it for error
	 * reporting.
	 */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		goto err;
	}

	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);

	if (passwd != NULL && (ret = dbenv->set_encrypt(dbenv,
	    passwd, DB_ENCRYPT_AES)) != 0) {
		dbenv->err(dbenv, ret, "set_passwd");
		goto err;
	}

	if (verbose) {
		(void)dbenv->set_verbose(dbenv, DB_VERB_DEADLOCK, 1);
		(void)dbenv->set_verbose(dbenv, DB_VERB_WAITSFOR, 1);
	}

	/* An environment is required. */
	if ((ret = dbenv->open(dbenv, home, DB_USE_ENVIRON, 0)) != 0) {
		dbenv->err(dbenv, ret, DB_STR("5101", "open"));
		goto err;
	}

	while (!__db_util_interrupted()) {
		if (verbose) {
			(void)time(&now);
			dbenv->errx(dbenv, DB_STR_A("5102",
			    "running at %.24s", "%.24s"),
			     __os_ctime(&now, time_buf));
		}

		if ((ret =
		    dbenv->lock_detect(dbenv, 0, atype, &rejected)) != 0) {
			dbenv->err(dbenv, ret, "DB_ENV->lock_detect");
			goto err;
		}
		if (verbose)
			dbenv->errx(dbenv, DB_STR_A("5103",
			    "rejected %d locks", "%d"), rejected);

		/* Make a pass every "secs" secs and "usecs" usecs. */
		if (secs == 0 && usecs == 0)
			break;
		__os_yield(dbenv->env, secs, usecs);
	}

	if (0) {
err:		exitval = 1;
	}

	/* Clean up the logfile. */
	if (logfile != NULL)
		(void)remove(logfile);

	/* Clean up the environment. */
	if (dbenv != NULL && (ret = dbenv->close(dbenv, 0)) != 0) {
		exitval = 1;
		fprintf(stderr,
		    "%s: dbenv->close: %s\n", progname, db_strerror(ret));
	}

	if (passwd != NULL)
		free(passwd);

	/* Resend any caught signal. */
	__db_util_sigresend();

	return (exitval == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

int
db_deadlock_usage()
{
	(void)fprintf(stderr,
	    "usage: %s [-Vv] [-a e | m | n | o | W | w | y]\n\t%s\n", progname,
	    "[-h home] [-L file] [-P password] [-t sec.usec]");
	return (EXIT_FAILURE);
}

int
db_deadlock_version_check()
{
	int v_major, v_minor, v_patch;

	/* Make sure we're loaded with the right version of the DB library. */
	(void)db_version(&v_major, &v_minor, &v_patch);
	if (v_major != DB_VERSION_MAJOR || v_minor != DB_VERSION_MINOR) {
		fprintf(stderr, DB_STR_A("5104",
		    "%s: version %d.%d doesn't match library version %d.%d\n",
		    "%s %d %d %d %d\n"), progname, DB_VERSION_MAJOR,
		    DB_VERSION_MINOR, v_major, v_minor);
		return (EXIT_FAILURE);
	}
	return (0);
}
