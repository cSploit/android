/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id: db_log_verify.c,v 0f73af5ae3da 2010/05/10 05:38:40 alexander $
 */
#include "db_config.h"

#include "db_int.h"

#define	MB 1024 * 1024

int main __P((int, char *[]));
int lsn_arg __P((char *, DB_LSN *));
int usage __P((void));
int version_check __P((void));
int db_log_verify_app_record __P((DB_ENV *, DBT *, DB_LSN *, db_recops));
const char *progname;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DB_ENV	*dbenv;
	DB_LSN start, stop;
	int ch, cmb, exitval, nflag, rflag, ret, vsn_mismtch;
	time_t starttime, endtime;
	char *dbfile, *dbname, *home, *lvhome, *passwd;
	DB_LOG_VERIFY_CONFIG lvconfig;

	vsn_mismtch = 0;
	if ((progname = __db_rpath(argv[0])) == NULL)
		progname = argv[0];
	else
		++progname;

	if ((ret = version_check()) != 0)
		return (ret);

	dbenv = NULL;
	dbfile = dbname = home = lvhome = passwd = NULL;
	exitval = nflag = rflag = 0;
	starttime = endtime = 0;
	ZERO_LSN(start);
	ZERO_LSN(stop);

	memset(&lvconfig, 0, sizeof(lvconfig));

	while ((ch = getopt(argc, argv, "b:cC:d:D:e:h:H:NP:Vvs:z:")) != EOF)
		switch (ch) {
		case 'b':
			/* Don't use getsubopt(3), not all systems have it. */
			if (lsn_arg(optarg, &start))
				return (usage());
			break;
		case 'c':
			lvconfig.continue_after_fail = 1;
			break;
		case 'C':
			cmb = atoi(optarg);
			if (cmb <= 0)
				return (usage());
			lvconfig.cachesize = cmb * MB;
			break;
		case 'd':
			dbfile = optarg;
			break;
		case 'D':
			dbname = optarg;
			break;
		case 'e':
			/* Don't use getsubopt(3), not all systems have it. */
			if (lsn_arg(optarg, &stop))
				return (usage());
			break;
		case 'h':
			home = optarg;
			break;
		case 'H':
			lvhome = optarg;
			break;
		case 'N':
			nflag = 1;
			break;
		case 'P':
			if ((ret = __os_strdup(NULL, optarg, &passwd)) != 0) {
				__db_err(NULL, ret, "__os_strdup: ");
				return (EXIT_FAILURE);
			}
			memset(optarg, 0, strlen(optarg));
			break;
		case 'V':
			printf("%s\n", db_version(NULL, NULL, NULL));
			return (EXIT_SUCCESS);
		case 'v':
			lvconfig.verbose = 1;
			break;
		case 's':
			starttime = atoi(optarg);
			break;
		case 'z':
			endtime = atoi(optarg);
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		return (usage());

	lvconfig.temp_envhome = lvhome;
	lvconfig.dbfile = dbfile;
	lvconfig.dbname = dbname;
	lvconfig.start_lsn = start;
	lvconfig.end_lsn = stop;
	lvconfig.start_time = starttime;
	lvconfig.end_time = endtime;

create_again:
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
	    dbenv, db_log_verify_app_record)) != 0) {
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
	if (!vsn_mismtch && (ret = dbenv->open(dbenv, home,
	    DB_USE_ENVIRON, 0)) != 0) {
		if (dbenv->close(dbenv, 0) != 0) {
			dbenv = NULL;
			goto err;
		}
		vsn_mismtch = 1;
		goto create_again;
	}
	if (vsn_mismtch && (ret = dbenv->open(dbenv, home, DB_CREATE |
	    DB_INIT_LOG | DB_PRIVATE | DB_USE_ENVIRON, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->open");
		goto err;
	}

	/* Handle possible interruptions. */
	__db_util_siginit();

	if ((ret = dbenv->log_verify(dbenv, &lvconfig)) != 0)
		goto err;

	if (0) {
err:		exitval = 1;
	}

	if (dbenv != NULL && (ret = dbenv->close(dbenv, 0)) != 0) {
		exitval = 1;
		fprintf(stderr,
		    "%s: dbenv->close: %s\n", progname, db_strerror(ret));
	}

	/* Resend any caught signal. */
	__db_util_sigresend();

	if (passwd != NULL)
		__os_free(NULL, passwd);

	return (exitval == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

int
usage()
{
	fprintf(stderr, "\nusage: %s %s\n%s\n%s\n%s\n%s\n", progname,
	    "[-NcvV] [-h home] "
	    "[-H temporary environment home for internal use]",
	    "[-P password] [-C cache size in megabytes]",
	    "[-d db file name] [-D db name]",
	    "[-b file/offset] [-e file/offset]",
	    "[-s start time] [-z end time]");

	return (EXIT_FAILURE);
}

int
version_check()
{
	int v_major, v_minor, v_patch;

	/* Make sure we're loaded with the right version of the DB library. */
	(void)db_version(&v_major, &v_minor, &v_patch);
	if (v_major != DB_VERSION_MAJOR || v_minor != DB_VERSION_MINOR) {
		fprintf(stderr, DB_STR_A("5003",
		    "%s: version %d.%d doesn't match library version %d.%d\n",
		    "%s %d %d %d %d\n"), progname,
		    DB_VERSION_MAJOR, DB_VERSION_MINOR,
		    v_major, v_minor);
		return (EXIT_FAILURE);
	}
	return (0);
}

/*
 * Print an unknown, application-specific log record as best we can, this is
 * all we can do to such a log record during the verification. The counting
 * is done in __db_dispatch because we can't pass the log verify handle into
 * this function.
 */
int
db_log_verify_app_record(dbenv, dbt, lsnp, op)
	DB_ENV *dbenv;
	DBT *dbt;
	DB_LSN *lsnp;
	db_recops op;
{
	u_int32_t i, len, len2, rectype;
	int ret;
	u_int8_t ch;
	char *buf, *p;

	DB_ASSERT(dbenv->env, op == DB_TXN_LOG_VERIFY);
	COMPQUIET(op, DB_TXN_LOG_VERIFY);
	ch = 0;
	ret = 0;
	i = len = len2 = rectype = 0;
	buf = p = NULL;

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
	if ((ret = __os_malloc(dbenv->env,
	    len = 256 + 2 * dbt->size, &buf)) != 0)
		goto err;
	memset(buf, 0, len);
	snprintf(buf, len, DB_STR_A("5004",
	    "[%lu][%lu] App-specific log record: %lu\n\tdata: ",
	    "%lu %lu %lu"), (u_long)lsnp->file, (u_long)lsnp->offset,
	    (u_long)rectype);

	/*
	 * Each unprintable character takes up several bytes, so be aware of
	 * memory violation.
	 */
	for (i = 0; i < dbt->size && len2 < len; i++) {
		ch = ((u_int8_t *)dbt->data)[i];
		len2 = (u_int32_t)strlen(buf);
		p = buf + len2;
		snprintf(p, len - len2 - 1,
		    isprint(ch) || ch == 0x0a ? "%c" : "%#x ", ch);
	}
	len2 = (u_int32_t)strlen(buf);
	p = buf + len2;
	snprintf(p, len - len2 - 1, "\n\n");
	__db_msg(dbenv->env, "%s", buf);

err:	if (buf != NULL)
		__os_free(dbenv->env, buf);
	return (ret);
}

/*
 * lsn_arg --
 *	Parse a LSN argument.
 */
int
lsn_arg(arg, lsnp)
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
