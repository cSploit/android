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

int main __P((int, char *[]));
int usage __P((void));
int version_check __P((void));

const char *progname;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DB *dbp, *dbp1;
	DB_ENV *dbenv;
	u_int32_t flags, cache;
	int ch, exitval, mflag, nflag, private;
	int quiet, resize, ret;
	char *blob_dir, *dname, *fname, *home, *passwd;

	if ((progname = __db_rpath(argv[0])) == NULL)
		progname = argv[0];
	else
		++progname;

	if ((ret = version_check()) != 0)
		return (ret);

	dbenv = NULL;
	dbp = NULL;
	cache = MEGABYTE;
	exitval = mflag = nflag = quiet = 0;
	flags = 0;
	blob_dir = home = passwd = NULL;
	while ((ch = getopt(argc, argv, "b:h:mNoP:quV")) != EOF)
		switch (ch) {
		case 'b':
			blob_dir = optarg;
			break;
		case 'h':
			home = optarg;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'N':
			nflag = 1;
			break;
		case 'P':
			if (passwd != NULL) {
				fprintf(stderr, DB_STR("5132",
					"Password may not be specified twice"));
				free(passwd);
				return (EXIT_FAILURE);
			}
			passwd = strdup(optarg);
			memset(optarg, 0, strlen(optarg));
			if (passwd == NULL) {
				fprintf(stderr, "%s: strdup: %s\n",
				    progname, strerror(errno));
				return (EXIT_FAILURE);
			}
			break;
		case 'o':
			LF_SET(DB_NOORDERCHK);
			break;
		case 'q':
			quiet = 1;
			break;
		case 'u':			/* Undocumented. */
			LF_SET(DB_UNREF);
			break;
		case 'V':
			printf("%s\n", db_version(NULL, NULL, NULL));
			return (EXIT_SUCCESS);
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (argc <= 0)
		return (usage());

	if (mflag) {
		dname = argv[0];
		fname = NULL;
	} else {
		fname = argv[0];
		dname = NULL;
	}

	/* Handle possible interruptions. */
	__db_util_siginit();

	/*
	 * Create an environment object and initialize it for error
	 * reporting.
	 */
retry:	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		goto err;
	}

	if (!quiet) {
		dbenv->set_errfile(dbenv, stderr);
		dbenv->set_errpfx(dbenv, progname);
	}

	if (blob_dir != NULL &&
	    (ret = dbenv->set_blob_dir(dbenv, blob_dir)) != 0) {
		dbenv->err(dbenv, ret, "set_blob_dir");
		goto err;
	}

	if (nflag) {
		if ((ret = dbenv->set_flags(dbenv, DB_NOLOCKING, 1)) != 0) {
			dbenv->err(dbenv, ret, "set_flags: DB_NOLOCKING");
			goto err;
		}
		if ((ret = dbenv->set_flags(dbenv, DB_NOPANIC, 1)) != 0) {
			dbenv->err(dbenv, ret, "set_flags: DB_NOPANIC");
			goto err;
		}
	}

	if (passwd != NULL &&
	    (ret = dbenv->set_encrypt(dbenv, passwd, DB_ENCRYPT_AES)) != 0) {
		dbenv->err(dbenv, ret, "set_passwd");
		goto err;
	}
	/*
	 * Attach to an mpool if it exists, but if that fails, attach to a
	 * private region.  In the latter case, declare a reasonably large
	 * cache so that we don't fail when verifying large databases.
	 */
	private = 0;
	if ((ret =
	    dbenv->open(dbenv, home, DB_INIT_MPOOL | DB_USE_ENVIRON, 0)) != 0) {
		if (ret != DB_VERSION_MISMATCH && ret != DB_REP_LOCKOUT) {
			if ((ret =
			    dbenv->set_cachesize(dbenv, 0, cache, 1)) != 0) {
				dbenv->err(dbenv, ret, "set_cachesize");
				goto err;
			}
			private = 1;
			ret = dbenv->open(dbenv, home, DB_CREATE |
			    DB_INIT_MPOOL | DB_PRIVATE | DB_USE_ENVIRON, 0);
		}
		if (ret != 0) {
			dbenv->err(dbenv, ret, "DB_ENV->open");
			goto err;
		}
	}

	/*
	 * Find out if we have a transactional environment so that we can
	 * make sure that we don't open the verify database with logging
	 * enabled.
	 */
	for (; !__db_util_interrupted() && argv[0] != NULL; ++argv) {
		if ((ret = db_create(&dbp, dbenv, 0)) != 0) {
			dbenv->err(dbenv, ret, "%s: db_create", progname);
			goto err;
		}

		if (TXN_ON(dbenv->env) &&
		    (ret = dbp->set_flags(dbp, DB_TXN_NOT_DURABLE)) != 0) {
			dbenv->err(
			    dbenv, ret, "%s: db_set_flags", progname);
			goto err;
		}

		/*
		 * We create a 2nd dbp to this database to get its pagesize
		 * because the dbp we're using for verify cannot be opened.
		 *
		 * If the database is corrupted, we may not be able to open
		 * it, of course.  In that case, just continue, using the
		 * cache size we have.
		 */
		if (private) {
			if ((ret = db_create(&dbp1, dbenv, 0)) != 0) {
				dbenv->err(
				    dbenv, ret, "%s: db_create", progname);
				goto err;
			}

			if (TXN_ON(dbenv->env) && (ret =
			    dbp1->set_flags(dbp1, DB_TXN_NOT_DURABLE)) != 0) {
				dbenv->err(
				    dbenv, ret, "%s: db_set_flags", progname);
				goto err;
			}

			ret = dbp1->open(dbp1,
			    NULL, fname, dname, DB_UNKNOWN, DB_RDONLY, 0);

			/*
			 * If we get here, we can check the cache/page.
			 * !!!
			 * If we have to retry with an env with a larger
			 * cache, we jump out of this loop.  However, we
			 * will still be working on the same argv when we
			 * get back into the for-loop.
			 */
			if (ret == 0) {
				if (__db_util_cache(
				    dbp1, &cache, &resize) == 0 && resize) {
					(void)dbp1->close(dbp1, 0);
					(void)dbp->close(dbp, 0);
					dbp = NULL;

					(void)dbenv->close(dbenv, 0);
					dbenv = NULL;
					goto retry;
				}
			}
			(void)dbp1->close(dbp1, 0);
		}

		/* The verify method is a destructor. */
		ret = dbp->verify(dbp, fname, dname, NULL, flags);
		dbp = NULL;
		if (ret != 0)
			exitval = 1;
		if (!quiet)
			printf(DB_STR_A("5105", "Verification of %s %s.\n",
			    "%s %s\n"), argv[0], ret == 0 ? 
			    DB_STR_P("succeeded") : DB_STR_P("failed"));
	}

	if (0) {
err:		exitval = 1;
	}

	if (dbp != NULL && (ret = dbp->close(dbp, 0)) != 0) {
		exitval = 1;
		dbenv->err(dbenv, ret, DB_STR("5106", "close"));
	}
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
usage()
{
	fprintf(stderr, "usage: %s %s\n", progname,
	    "[-NoqV] [-h home] [-P password] db_file ...");
	return (EXIT_FAILURE);
}

int
version_check()
{
	int v_major, v_minor, v_patch;

	/* Make sure we're loaded with the right version of the DB library. */
	(void)db_version(&v_major, &v_minor, &v_patch);
	if (v_major != DB_VERSION_MAJOR || v_minor != DB_VERSION_MINOR) {
		fprintf(stderr, DB_STR_A("5107",
		    "%s: version %d.%d doesn't match library version %d.%d\n",
		    "%s %d %d %d %d\n"), progname, DB_VERSION_MAJOR,
		    DB_VERSION_MINOR, v_major, v_minor);
		return (EXIT_FAILURE);
	}
	return (0);
}
