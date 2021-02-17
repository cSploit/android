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
    "Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.\n";
#endif

int	 main __P((int, char *[]));

#ifndef HAVE_REPLICATION_THREADS
int
main(argc, argv)
	int argc;
	char *argv[];
{
	fprintf(stderr, DB_STR_A("5092",
	    "Cannot run %s without Replication Manager.\n", "%s\n"),
	    argv[0]);
	COMPQUIET(argc, 0);
	exit (1);
}
#else

static int	usage __P((void));
static int	version_check __P((void));
static void	event_callback __P((DB_ENV *, u_int32_t, void *));
static int	db_replicate_logmsg __P((DB_ENV *, const char *));
static void	prog_close __P((DB_ENV *, int));

/* * Buffer for logging messages.  */
#define	MSG_SIZE	256
char log_msg[MSG_SIZE];
char *logfile;
FILE *logfp;
pid_t pid;

char progname[MSG_SIZE];
int panic_exit;
#define	REP_NTHREADS	3
#define	MAX_RETRY	3

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DB_ENV *dbenv;
	time_t now;
	long argval;
	db_timeout_t max_req;
	u_int32_t repmgr_th, seconds, start_state;
	int ch, count, done, exitval, ret, verbose;
	char *blob_dir, *home, *passwd, *prog, time_buf[CTIME_BUFLEN];

	dbenv = NULL;
	logfp = NULL;
	log_msg[MSG_SIZE - 1] = '\0';
	__os_id(NULL, &pid, NULL);
	panic_exit = 0;

	if ((prog = __db_rpath(argv[0])) == NULL)
		prog = argv[0];
	else
		++prog;

	if ((size_t)(count = snprintf(progname, sizeof(progname), "%s(%lu)",
	    prog, (u_long)pid)) >= sizeof(progname)) {
		fprintf(stderr, DB_STR("5093", "Program name too long\n"));
		goto err;
	}
	if ((ret = version_check()) != 0)
		goto err;

	/*
	 * !!!
	 * Don't allow a fully unsigned 32-bit number, some compilers get
	 * upset and require it to be specified in hexadecimal and so on.
	 */
#define	MAX_UINT32_T	2147483647


	/*
	 * Create an environment object and initialize it for error
	 * reporting.  Create it before parsing args so that we can
	 * call methods to set the values directly.
	 */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		goto err;
	}

	(void)dbenv->set_event_notify(dbenv, event_callback);
	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);

	exitval = verbose = 0;
	blob_dir = home = logfile = passwd = NULL;
	seconds = 30;
	start_state = DB_REP_ELECTION;
	repmgr_th = REP_NTHREADS;
	while ((ch = getopt(argc, argv, "b:h:L:MP:T:t:Vv")) != EOF)
		switch (ch) {
		case 'b':
			blob_dir = optarg;
			break;
		case 'h':
			home = optarg;
			break;
		case 'L':
			logfile = optarg;
			break;
		case 'M':
			start_state = DB_REP_MASTER;
			break;
		case 'P':
			passwd = strdup(optarg);
			memset(optarg, 0, strlen(optarg));
			if (passwd == NULL) {
				fprintf(stderr, DB_STR_A("5094",
				    "%s: strdup: %s\n", "%s %s\n"),
				    progname, strerror(errno));
				return (EXIT_FAILURE);
			}
			ret = dbenv->set_encrypt(dbenv, passwd, DB_ENCRYPT_AES);
			free(passwd);
			if (ret != 0) {
				dbenv->err(dbenv, ret, "set_passwd");
				goto err;
			}
			break;
		case 'T':
			if (__db_getlong(NULL, progname,
			    optarg, 1, (long)MAX_UINT32_T, &argval))
				return (EXIT_FAILURE);
			repmgr_th = (u_int32_t)argval;
			break;
		case 't':
			if (__db_getlong(NULL, progname,
			    optarg, 1, (long)MAX_UINT32_T, &argval))
				return (EXIT_FAILURE);
			seconds = (u_int32_t)argval;
			break;
		case 'V':
			printf("%s\n", db_version(NULL, NULL, NULL));
			return (EXIT_SUCCESS);
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		return (usage());

	/* Handle possible interruptions. */
	__db_util_siginit();

	/*
	 * Log our process ID.  This is a specialized case of
	 * __db_util_logset because we retain the logfp and keep
	 * the file open for additional logging.
	 */
	if (logfile != NULL) {
		if ((logfp = fopen(logfile, "w")) == NULL)
			goto err;
		if ((ret = db_replicate_logmsg(dbenv, "STARTED")) != 0)
			goto err;
	}

	if (blob_dir != NULL && 
	    (ret = dbenv->set_blob_dir(dbenv, blob_dir)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->set_blob_dir");
		goto err;
	}

	/*
	 * If attaching to a pre-existing environment fails, error.
	 */
#define	ENV_FLAGS (DB_THREAD | DB_USE_ENVIRON)
	if ((ret = dbenv->open(dbenv, home, ENV_FLAGS, 0)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->open");
		goto err;
	}

	/*
	 * Confirm that replication is configured in the underlying
	 * environment.  We need the max request value anyway and
	 * the method to get the value returns an error if replication
	 * is not configured.
	 */
	if ((ret = dbenv->rep_get_request(dbenv, NULL, &max_req)) != 0) {
		dbenv->err(dbenv, ret, "rep_get_request");
		goto err;
	}

	/*
	 * Start replication.
	 */
	if (verbose && ((ret = dbenv->set_verbose(dbenv,
	    DB_VERB_REPLICATION, 1)) != 0)) {
		dbenv->err(dbenv, ret, "set_verbose");
		goto err;
	}
	count = done = 0;
	while (!done && count < MAX_RETRY) {
		/*
		 * Retry if we get an error that indicates that the port is
		 * in use.  An old version of this program could still be
		 * running.  The application restarts with recovery, and that
		 * should panic the old environment, but it may take a little
		 * bit of time for the old program to notice the panic.  
		 *
		 * We wait the max_req time because at worst the rerequest
		 * thread runs every max_req time and should notice a panic.  On
		 * the other hand, if we're joining the replication group for
		 * the first time and the master is not available
		 * (DB_REP_UNAVAIL), it makes sense to pause a bit longer before
		 * retrying.
		 */
		if ((ret = dbenv->repmgr_start(dbenv,
			    repmgr_th, start_state)) == DB_REP_UNAVAIL) {
			count++;
			__os_yield(dbenv->env, 5, 0);
		} else if (ret != 0) {
			count++;
			__os_yield(dbenv->env, 0, max_req);
		} else
			done = 1;
	}
	if (!done) {
		dbenv->err(dbenv, ret, "repmgr_start");
		goto err;
	}

	/* Main loop of the program. */
	while (!__db_util_interrupted() && !panic_exit) {
		/*
		 * The program itself does not have much to do.  All the
		 * interesting replication stuff is happening underneath.
		 * Each period, we'll wake up and call rep_flush just to
		 * force a log record and cause any gaps to fill as well as
		 * check program status to see if it was interrupted.
		 */
		__os_yield(dbenv->env, seconds, 0);
		if (verbose) {
			(void)time(&now);
			dbenv->errx(dbenv, DB_STR_A("5095",
			    "db_replicate begin: %s", "%s"),
			    __os_ctime(&now, time_buf));
		}

		/*
		 * Hmm, do we really want to exit on error here?  This is
		 * a non-essential piece of the program, so if it gets
		 * an error, we may just want to ignore it.  Note we call
		 * rep_flush without checking if we're a master or client.
		 */
		if ((ret = dbenv->rep_flush(dbenv)) != 0) {
			dbenv->err(dbenv, ret, "rep_flush");
			goto err;
		}
	}

	if (panic_exit)
err:		exitval = 1;

	prog_close(dbenv, exitval);
	return (exitval == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void
prog_close(dbenv, exitval)
	DB_ENV *dbenv;
	int exitval;
{
	int ret;

	if (logfp != NULL) {
		fclose(logfp);
		(void)remove(logfile);
	}
	/* Clean up the environment. */
	if (dbenv != NULL && (ret = dbenv->close(dbenv, 0)) != 0) {
		exitval = 1;
		fprintf(stderr,
		    "%s: dbenv->close: %s\n", progname, db_strerror(ret));
	}

	/* Resend any caught signal. */
	__db_util_sigresend();

	exit (exitval == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

static void
event_callback(dbenv, which, info)
	DB_ENV *dbenv;
	u_int32_t which;
	void *info;
{
	COMPQUIET(info, NULL);
	switch (which) {
	case DB_EVENT_PANIC:
		/*
		 * If the app restarted with recovery, and we're an old
		 * program running against the old regions, we'll discover
		 * the panic and want to exit quickly to give a new 
		 * instantiation of the program access to the port.
		 */
		printf(DB_STR("5096", "received panic event\n"));
		db_replicate_logmsg(dbenv, "PANIC");
		panic_exit = 1;
		break;

	case DB_EVENT_REP_CLIENT:
		db_replicate_logmsg(dbenv, "CLIENT");
		break;

	case DB_EVENT_REP_CONNECT_BROKEN:
		db_replicate_logmsg(dbenv, "CONNECTIONBROKEN");
		break;
		
	case DB_EVENT_REP_DUPMASTER:
		db_replicate_logmsg(dbenv, "DUPMASTER");
		break;

	case DB_EVENT_REP_ELECTED:
		db_replicate_logmsg(dbenv, "ELECTED");
		break;

	case DB_EVENT_REP_MASTER:
		db_replicate_logmsg(dbenv, "MASTER");
		break;

	case DB_EVENT_REP_NEWMASTER:
		db_replicate_logmsg(dbenv, "NEWMASTER");
		break;

	case DB_EVENT_REP_STARTUPDONE:
		db_replicate_logmsg(dbenv, "STARTUPDONE");
		break;

	case DB_EVENT_REP_CONNECT_ESTD:
	case DB_EVENT_REP_CONNECT_TRY_FAILED:
	case DB_EVENT_REP_INIT_DONE:
	case DB_EVENT_REP_LOCAL_SITE_REMOVED:
	case DB_EVENT_REP_PERM_FAILED:
	case DB_EVENT_REP_SITE_ADDED:
	case DB_EVENT_REP_SITE_REMOVED:
		/* We don't care about these, for now. */
		break;

	default:
		db_replicate_logmsg(dbenv, "IGNORED");
		dbenv->errx(dbenv, DB_STR_A("5097", "ignoring event %d",
		    "%d"), which);
	}
}

static int
usage()
{
	(void)fprintf(stderr, "usage: %s [-MVv]\n\t%s\n", progname,
"[-h home] [-P password] [-T nthreads] [-t seconds]");
	return (EXIT_FAILURE);
}

static int
version_check()
{
	int v_major, v_minor, v_patch;

	/* Make sure we're loaded with the right version of the DB library. */
	(void)db_version(&v_major, &v_minor, &v_patch);
	if (v_major != DB_VERSION_MAJOR || v_minor != DB_VERSION_MINOR) {
		fprintf(stderr, DB_STR_A("5098",
		    "%s: version %d.%d doesn't match library version %d.%d\n",
		    "%s %d %d %d %d\n"), progname,
		    DB_VERSION_MAJOR, DB_VERSION_MINOR,
		    v_major, v_minor);
		return (EXIT_FAILURE);
	}
	return (0);
}

static int
db_replicate_logmsg(dbenv, msg)
	DB_ENV *dbenv;
	const char *msg;
{
	time_t now;
	int cnt;
	char time_buf[CTIME_BUFLEN];

	if (logfp == NULL)
		return (0);

	(void)time(&now);
	(void)__os_ctime(&now, time_buf);
	if ((size_t)(cnt = snprintf(log_msg, sizeof(log_msg), "%s: %lu %s %s",
	    progname, (u_long)pid, time_buf, msg)) >= sizeof(log_msg)) {
		dbenv->errx(dbenv, DB_STR_A("5099",
		    "%s: %lu %s %s: message too long", "%s %lu %s %s"),
		    progname, (u_long)pid, time_buf, msg);
		return (1);
	}
	fprintf(logfp, "%s\n", log_msg);
	return (0);
}
#endif
