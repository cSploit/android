/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/fop.h"
#include "dbinc/hash.h"
#ifdef HAVE_HEAP
#include "dbinc/heap.h"
#endif
#include "dbinc/qam.h"
#include "dbinc/txn.h"

#ifndef lint
static const char copyright[] =
    "Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.\n";
#endif

int db_printlog_print_app_record __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
int db_printlog_env_init_print __P((ENV *, u_int32_t, DB_DISTAB *));
int db_printlog_env_init_print_42 __P((ENV *, DB_DISTAB *));
int db_printlog_env_init_print_43 __P((ENV *, DB_DISTAB *));
int db_printlog_env_init_print_47 __P((ENV *, DB_DISTAB *));
int db_printlog_env_init_print_48 __P((ENV *, DB_DISTAB *));
int db_printlog_env_init_print_53 __P((ENV *, DB_DISTAB *));
int db_printlog_env_init_print_60 __P((ENV *, DB_DISTAB *));
int db_printlog_lsn_arg __P((char *, DB_LSN *));
int db_printlog_main __P((int, char *[]));
int db_printlog_open_rep_db __P((DB_ENV *, DB **, DBC **));
int db_printlog_usage __P((void));
int db_printlog_version_check __P((void));

const char *progname;

int
db_printlog(args)
	char *args;
{
	int argc;
	char **argv;

	__db_util_arg("db_printlog", args, &argc, &argv);
	return (db_printlog_main(argc, argv) ? EXIT_FAILURE : EXIT_SUCCESS);
}

#include <stdio.h>
#define	ERROR_RETURN	ERROR

int
db_printlog_main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind, __db_getopt_reset;
	DB *dbp;
	DBC *dbc;
	DBT data, keydbt;
	DB_DISTAB dtab;
	DB_ENV	*dbenv;
	DB_LOG dblog;
	DB_LOGC *logc;
	DB_LSN key, start, stop, verslsn;
	ENV *env;
	u_int32_t logcflag, newversion, version;
	int ch, cmp, exitval, i, nflag, rflag, ret, repflag;
	char *data_len, *home, *passwd;

	if ((progname = __db_rpath(argv[0])) == NULL)
		progname = argv[0];
	else
		++progname;

	if ((ret = db_printlog_version_check()) != 0)
		return (ret);

	dbp = NULL;
	dbc = NULL;
	dbenv = NULL;
	env = NULL;
	logc = NULL;
	ZERO_LSN(start);
	ZERO_LSN(stop);
	exitval = nflag = rflag = repflag = 0;
	data_len = home = passwd = NULL;

	memset(&dtab, 0, sizeof(dtab));
	memset(&dblog, 0, sizeof(dblog));

	__db_getopt_reset = 1;
	while ((ch = getopt(argc, argv, "b:D:e:h:NP:rRV")) != EOF)
		switch (ch) {
		case 'b':
			/* Don't use getsubopt(3), not all systems have it. */
			if (db_printlog_lsn_arg(optarg, &start))
				return (db_printlog_usage());
			break;
		case 'D':
			data_len = optarg;
			break;
		case 'e':
			/* Don't use getsubopt(3), not all systems have it. */
			if (db_printlog_lsn_arg(optarg, &stop))
				return (db_printlog_usage());
			break;
		case 'h':
			home = optarg;
			break;
		case 'N':
			nflag = 1;
			break;
		case 'P':
			if (passwd != NULL) {
				fprintf(stderr, DB_STR("5138",
					"Password may not be specified twice"));
				free(passwd);
				return (EXIT_FAILURE);
			}
			passwd = strdup(optarg);
			memset(optarg, 0, strlen(optarg));
			if (passwd == NULL) {
				fprintf(stderr, DB_STR_A("5010",
				    "%s: strdup: %s\n", "%s %s\n"),
				    progname, strerror(errno));
				return (EXIT_FAILURE);
			}
			break;
		case 'r':
			rflag = 1;
			break;
		case 'R':		/* Undocumented */
			repflag = 1;
			break;
		case 'V':
			printf("%s\n", db_version(NULL, NULL, NULL));
			return (EXIT_SUCCESS);
		case '?':
		default:
			return (db_printlog_usage());
		}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		return (db_printlog_usage());

	/* Handle possible interruptions. */
	__db_util_siginit();

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
	dbenv->set_msgfile(dbenv, stdout);

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

	if (passwd != NULL && (ret = dbenv->set_encrypt(dbenv,
	    passwd, DB_ENCRYPT_AES)) != 0) {
		dbenv->err(dbenv, ret, "set_passwd");
		goto err;
	}

	/*
	 * Set up an app-specific dispatch function so that we can gracefully
	 * handle app-specific log records.
	 */
	if ((ret = dbenv->set_app_dispatch(
	    dbenv, db_printlog_print_app_record)) != 0) {
		dbenv->err(dbenv, ret, "app_dispatch");
		goto err;
	}

	/*
	 * An environment is required, but as all we're doing is reading log
	 * files, we create one if it doesn't already exist.  If we create
	 * it, create it private so it automatically goes away when we're done.
	 * If we are reading the replication database, do not open the env
	 * with logging, because we don't want to log the opens.
	 */
	if (repflag) {
		if ((ret = dbenv->open(dbenv, home,
		    DB_INIT_MPOOL | DB_USE_ENVIRON, 0)) != 0 &&
		    (ret == DB_VERSION_MISMATCH || ret == DB_REP_LOCKOUT ||
		    (ret = dbenv->open(dbenv, home,
		    DB_CREATE | DB_INIT_MPOOL | DB_PRIVATE | DB_USE_ENVIRON, 0))
		    != 0)) {
			dbenv->err(dbenv, ret, "DB_ENV->open");
			goto err;
		}
	} else if ((ret = dbenv->open(dbenv, home, DB_USE_ENVIRON, 0)) != 0 &&
	    (ret == DB_VERSION_MISMATCH || ret == DB_REP_LOCKOUT ||
	    (ret = dbenv->open(dbenv, home,
	    DB_CREATE | DB_INIT_LOG | DB_PRIVATE | DB_USE_ENVIRON, 0)) != 0)) {
		dbenv->err(dbenv, ret, "DB_ENV->open");
		goto err;
	}

	/*
	 * Set data_len after environment opens. We want the value passed
	 * by -D takes priority.
	 */
	if (data_len != NULL && (ret = dbenv->set_data_len(dbenv,
	    (u_int32_t)atol(data_len))) != 0) {
		dbenv->err(dbenv, ret, "set_data_len");
		goto err;
	}

	env = dbenv->env;

	/* Allocate a log cursor. */
	if (repflag) {
		if ((ret = db_printlog_open_rep_db(dbenv, &dbp, &dbc)) != 0)
			goto err;
	} else if ((ret = dbenv->log_cursor(dbenv, &logc, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->log_cursor");
		goto err;
	}

	if (IS_ZERO_LSN(start)) {
		memset(&keydbt, 0, sizeof(keydbt));
		logcflag = rflag ? DB_PREV : DB_NEXT;
	} else {
		key = start;
		logcflag = DB_SET;
	}
	memset(&data, 0, sizeof(data));

	/*
	 * If we're using the repflag, we're immediately initializing
	 * the print table.  Use the current version.  If we're printing
	 * the log then initialize version to 0 so that we get the
	 * correct version right away.
	 */
	if (repflag)
		version = DB_LOGVERSION;
	else
		version = 0;
	ZERO_LSN(verslsn);

	/* Initialize print callbacks if repflag. */
	if (repflag &&
	    (ret = db_printlog_env_init_print(env, version, &dtab)) != 0) {
		dbenv->err(dbenv, ret, DB_STR("5011",
		    "callback: initialization"));
		goto err;
	}
	for (; !__db_util_interrupted(); logcflag = rflag ? DB_PREV : DB_NEXT) {
		if (repflag) {
			ret = dbc->get(dbc, &keydbt, &data, logcflag);
			if (ret == 0)
				key = ((__rep_control_args *)keydbt.data)->lsn;
		} else
			ret = logc->get(logc, &key, &data, logcflag);
		if (ret != 0) {
			if (ret == DB_NOTFOUND)
				break;
			dbenv->err(dbenv,
			    ret, repflag ? "DBC->get" : "DB_LOGC->get");
			goto err;
		}

		/*
		 * We may have reached the end of the range we're displaying.
		 */
		if (!IS_ZERO_LSN(stop)) {
			cmp = LOG_COMPARE(&key, &stop);
			if ((rflag && cmp < 0) || (!rflag && cmp > 0))
				break;
		}
		if (!repflag && key.file != verslsn.file) {
			/*
			 * If our log file changed, we need to see if the
			 * version of the log file changed as well.
			 * If it changed, reset the print table.
			 */
			if ((ret = logc->version(logc, &newversion, 0)) != 0) {
				dbenv->err(dbenv, ret, "DB_LOGC->version");
				goto err;
			}
			if (version != newversion) {
				version = newversion;
				if ((ret = db_printlog_env_init_print(env, version,
				    &dtab)) != 0) {
					dbenv->err(dbenv, ret, DB_STR("5012",
					    "callback: initialization"));
					goto err;
				}
			}
		}

		ret = __db_dispatch(env,
		    &dtab, &data, &key, DB_TXN_PRINT, (void*)&dblog);

		/*
		 * XXX
		 * Just in case the underlying routines don't flush.
		 */
		(void)fflush(stdout);

		if (ret != 0) {
			dbenv->err(dbenv, ret, DB_STR("5013",
			    "tx: dispatch"));
			goto err;
		}
	}

	if (0) {
err:		exitval = 1;
	}

	if (dtab.int_dispatch != NULL)
		__os_free(env, dtab.int_dispatch);
	if (dtab.ext_dispatch != NULL)
		__os_free(env, dtab.ext_dispatch);
	/*
	 * Call __db_close to free the dummy DB handles that were used
	 * by the print routines.
	 */
	for (i = 0; i < dblog.dbentry_cnt; i++)
		if (dblog.dbentry[i].dbp != NULL)
			(void)__db_close(dblog.dbentry[i].dbp, NULL, DB_NOSYNC);
	if (env != NULL && dblog.dbentry != NULL)
		__os_free(env, dblog.dbentry);
	if (logc != NULL && (ret = logc->close(logc, 0)) != 0)
		exitval = 1;

	if (dbc != NULL && (ret = dbc->close(dbc)) != 0)
		exitval = 1;

	if (dbp != NULL && (ret = dbp->close(dbp, 0)) != 0)
		exitval = 1;

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

/*
 * env_init_print --
 *
 *	Fill the dispatch table for printing this log version's records.
 */
int
db_printlog_env_init_print(env, version, dtabp)
	ENV *env;
	u_int32_t version;
	DB_DISTAB *dtabp;
{
	int ret;

	/*
	 * We need to prime the print table with the current print
	 * functions.  Then we overwrite only specific entries based on
	 * each previous version we support.
	 */
	if ((ret = __bam_init_print(env, dtabp)) != 0)
		goto err;
	if ((ret = __crdel_init_print(env, dtabp)) != 0)
		goto err;
	if ((ret = __db_init_print(env, dtabp)) != 0)
		goto err;
	if ((ret = __dbreg_init_print(env, dtabp)) != 0)
		goto err;
	if ((ret = __fop_init_print(env, dtabp)) != 0)
		goto err;
#ifdef HAVE_HASH
	if ((ret = __ham_init_print(env, dtabp)) != 0)
		goto err;
#endif
#ifdef HAVE_HEAP
	if ((ret = __heap_init_print(env, dtabp)) != 0)
		goto err;
#endif
#ifdef HAVE_QUEUE
	if ((ret = __qam_init_print(env, dtabp)) != 0)
		goto err;
#endif
#ifdef HAVE_REPLICATION_THREADS
	if ((ret = __repmgr_init_print(env, dtabp)) != 0)
		goto err;
#endif
	if ((ret = __txn_init_print(env, dtabp)) != 0)
		goto err;

	if (version == DB_LOGVERSION)
		goto done;

	/* DB_LOGVERSION_60p1 changed the fop_write_file log. */
	if (version > DB_LOGVERSION_60)
		goto done;
	if ((ret = db_printlog_env_init_print_60(env, dtabp)) != 0)
		goto err;

	/* DB_LOGVERSION_53 changed the heap addrem log record. */
	if (version > DB_LOGVERSION_53)
		goto done;
	if ((ret = db_printlog_env_init_print_53(env, dtabp)) != 0)
		goto err;
	/*
	 * Since DB_LOGVERSION_53 is a strict superset of DB_LOGVERSION_50,
	 * there is no need to check for log version between them; so only
	 * check > DB_LOGVERSION_48p2.  Patch 2 of 4.8 added __db_pg_trunc but
	 * didn't alter any log records so we want the same override as 4.8.
	 */
	if (version > DB_LOGVERSION_48p2)
		goto done;
	if ((ret = db_printlog_env_init_print_48(env, dtabp)) != 0)
		goto err;
	if (version >= DB_LOGVERSION_48)
		goto done;
	if ((ret = db_printlog_env_init_print_47(env, dtabp)) != 0)
		goto err;
	if (version == DB_LOGVERSION_47)
		goto done;
	/*
	 * There are no log record/recovery differences between 4.4 and 4.5.
	 * The log version changed due to checksum.  There are no log recovery
	 * differences between 4.5 and 4.6.  The name of the rep_gen in
	 * txn_checkpoint changed (to spare, since we don't use it anymore).
	 */
	if (version >= DB_LOGVERSION_44)
		goto done;
	if ((ret = db_printlog_env_init_print_43(env, dtabp)) != 0)
		goto err;
	if (version == DB_LOGVERSION_43)
		goto done;
	if (version != DB_LOGVERSION_42) {
		__db_errx(env, DB_STR_A("5014",
		    "Unknown version %lu", "%lu"), (u_long)version);
		ret = EINVAL;
		goto err;
	}
	ret = db_printlog_env_init_print_42(env, dtabp);
done:
err:	return (ret);
}

int
db_printlog_env_init_print_42(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_relink_42_print, DB___db_relink_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_alloc_42_print, DB___db_pg_alloc_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_free_42_print, DB___db_pg_free_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_freedata_42_print, DB___db_pg_freedata_42)) != 0)
		goto err;
#if HAVE_HASH
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_metagroup_42_print, DB___ham_metagroup_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_groupalloc_42_print, DB___ham_groupalloc_42)) != 0)
		goto err;
#endif
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_ckp_42_print, DB___txn_ckp_42)) != 0)
		goto err;
err:
	return (ret);
}

int
db_printlog_env_init_print_43(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_relink_43_print, DB___bam_relink_43)) != 0)
		goto err;
	/*
	 * We want to use the 4.2-based txn_regop record.
	 */
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_regop_42_print, DB___txn_regop_42)) != 0)
		goto err;

err:
	return (ret);
}

/*
 * env_init_print_47 --
 *
 */
int
db_printlog_env_init_print_47(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	if ((ret = __db_add_recovery_int(env, dtabp,
	   __bam_split_42_print, DB___bam_split_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_sort_44_print, DB___db_pg_sort_44)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_sort_44_print, DB___db_pg_sort_44)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_create_42_print, DB___fop_create_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_write_42_print, DB___fop_write_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_rename_42_print, DB___fop_rename_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __fop_rename_42_print, DB___fop_rename_noundo_46)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __txn_xa_regop_42_print, DB___txn_xa_regop_42)) != 0)
		goto err;

err:
	return (ret);
}

int
db_printlog_env_init_print_48(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_pg_sort_44_print, DB___db_pg_sort_44)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_addrem_42_print, DB___db_addrem_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __db_big_42_print, DB___db_big_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __bam_split_48_print, DB___bam_split_48)) != 0)
		goto err;
#ifdef HAVE_HASH
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_insdel_42_print, DB___ham_insdel_42)) != 0)
		goto err;
	if ((ret = __db_add_recovery_int(env, dtabp,
	    __ham_replace_42_print, DB___ham_replace_42)) != 0)
		goto err;
#endif

err:
	return (ret);
}

int
db_printlog_env_init_print_53(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;
#ifdef HAVE_HEAP
	ret = __db_add_recovery_int(env,
	     dtabp,__heap_addrem_50_print, DB___heap_addrem_50);
#else
	COMPQUIET(env, NULL);
	COMPQUIET(dtabp, NULL);
	COMPQUIET(ret, 0);
#endif
	return (ret);
}

int
db_printlog_env_init_print_60(env, dtabp)
	ENV *env;
	DB_DISTAB *dtabp;
{
	int ret;

	ret = __db_add_recovery_int(env,
	     dtabp,__fop_write_file_60_print, DB___fop_write_file_60);

	return (ret);
}

int
db_printlog_usage()
{
	fprintf(stderr, "usage: %s %s%s\n", progname,
	    "[-NrV] [-b file/offset] [-D data_len] ",
	    "[-e file/offset] [-h home] [-P password]");
	return (EXIT_FAILURE);
}

int
db_printlog_version_check()
{
	int v_major, v_minor, v_patch;

	/* Make sure we're loaded with the right version of the DB library. */
	(void)db_version(&v_major, &v_minor, &v_patch);
	if (v_major != DB_VERSION_MAJOR || v_minor != DB_VERSION_MINOR) {
		fprintf(stderr, DB_STR_A("5015",
		    "%s: version %d.%d doesn't match library version %d.%d\n",
		    "%s %d %d %d %d\n"), progname,
		    DB_VERSION_MAJOR, DB_VERSION_MINOR,
		    v_major, v_minor);
		return (EXIT_FAILURE);
	}
	return (0);
}

/* Print an unknown, application-specific log record as best we can. */
int
db_printlog_print_app_record(dbenv, dbt, lsnp, op)
	DB_ENV *dbenv;
	DBT *dbt;
	DB_LSN *lsnp;
	db_recops op;
{
	u_int32_t i, rectype;
	int ch;

	DB_ASSERT(dbenv->env, op == DB_TXN_PRINT);

	COMPQUIET(dbenv, NULL);
	COMPQUIET(op, DB_TXN_PRINT);

	/*
	 * Fetch the rectype, which always must be at the beginning of the
	 * record (if dispatching is to work at all).
	 */
	memcpy(&rectype, dbt->data, sizeof(rectype));

	/*
	 * Applications may wish to customize the output here based on the
	 * rectype.  We just print the entire log record in the generic
	 * mixed-hex-and-printable format we use for binary data.
	 */
	printf(DB_STR_A("5016",
	    "[%lu][%lu]application specific record: rec: %lu\n",
	    "%lu %lu %lu"), (u_long)lsnp->file, (u_long)lsnp->offset,
	    (u_long)rectype);
	printf(DB_STR("5017", "\tdata: "));
	for (i = 0; i < dbt->size; i++) {
		ch = ((u_int8_t *)dbt->data)[i];
		printf(isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	printf("\n\n");

	return (0);
}

int
db_printlog_open_rep_db(dbenv, dbpp, dbcp)
	DB_ENV *dbenv;
	DB **dbpp;
	DBC **dbcp;
{
	int ret;

	DB *dbp;
	*dbpp = NULL;
	*dbcp = NULL;

	if ((ret = db_create(dbpp, dbenv, 0)) != 0) {
		dbenv->err(dbenv, ret, "db_create");
		return (ret);
	}

	dbp = *dbpp;
	if ((ret = dbp->open(dbp, NULL,
	    REPDBNAME, NULL, DB_BTREE, DB_RDONLY, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->open");
		goto err;
	}

	if ((ret = dbp->cursor(dbp, NULL, dbcp, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB->cursor");
		goto err;
	}

	return (0);

err:	if (*dbpp != NULL)
		(void)(*dbpp)->close(*dbpp, 0);
	return (ret);
}

/*
 * lsn_arg --
 *	Parse a LSN argument.
 */
int
db_printlog_lsn_arg(arg, lsnp)
	char *arg;
	DB_LSN *lsnp;
{
	u_long uval;
	char *p;

	/*
	 * Expected format is: lsn.file/lsn.offset.
	 */
	if ((p = strchr(arg, '/')) == NULL)
		return (1);
	*p = '\0';

	if (__db_getulong(NULL, progname, arg, 0, UINT32_MAX, &uval))
		return (1);
	lsnp->file = uval;
	if (__db_getulong(NULL, progname, p + 1, 0, UINT32_MAX, &uval))
		return (1);
	lsnp->offset = uval;
	return (0);
}
