/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"
#include "dbinc/db_page.h"
#include "dbinc/qam.h"

#ifndef lint
static const char copyright[] =
    "Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.\n";
#endif

enum which_open { OPEN_ORIGINAL, OPEN_HOT_BACKUP };

int env_init __P((DB_ENV **,
     char *, char *, char **, char ***, char *, enum which_open, int));
int main __P((int, char *[]));
int usage __P((void));
int version_check __P((void));
void __db_util_msg __P((const DB_ENV *, const char *));

const char *progname;

void __db_util_msg(dbenv, msgstr)
	const DB_ENV *dbenv;
	const char *msgstr;
{
	COMPQUIET(dbenv, NULL);
	printf("%s: %s\n", progname, msgstr);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	time_t now;
	DB_ENV *dbenv;
	u_int data_cnt, data_next;
	int ch, checkpoint, db_config, debug, env_copy, exitval;
	int ret, update, verbose;
	char *backup_dir, **data_dir;
	char *home, *home_blob_dir, *log_dir, *passwd;
	char home_buf[DB_MAXPATHLEN], time_buf[CTIME_BUFLEN];
	u_int32_t flags;

	/*
	 * Make sure all verbose message are output before any error messages
	 * in the case where the output is being logged into a file.  This
	 * call has to be done before any operation is performed on the stream.
	 *
	 * Use unbuffered I/O because line-buffered I/O requires a buffer, and
	 * some operating systems have buffer alignment and size constraints we
	 * don't want to care about.  There isn't enough output for the calls
	 * to matter.
	 */
	(void)setvbuf(stdout, NULL, _IONBF, 0);

	if ((progname = __db_rpath(argv[0])) == NULL)
		progname = argv[0];
	else
		++progname;

	if ((ret = version_check()) != 0)
		return (ret);

	/* We default to the safe environment copy. */
	env_copy = 1;

	checkpoint = db_config = data_cnt = data_next = debug = 
	    exitval = update = verbose = 0;
	data_dir = NULL;
	backup_dir = home = home_blob_dir = passwd = NULL;
	log_dir = NULL;
	while ((ch = getopt(argc, argv, "b:cDd:Fgh:i:l:P:uVv")) != EOF)
		switch (ch) {
		case 'b':
			backup_dir = optarg;
			break;
		case 'c':
			checkpoint = 1;
			break;
		case 'D':
			db_config = 1;
			break;
		case 'd':
			/*
			 * User can specify a list of directories -- keep an
			 * array, leaving room for the trailing NULL.
			 */
			if (data_dir == NULL || data_next >= data_cnt - 2) {
				data_cnt = data_cnt == 0 ? 20 : data_cnt * 2;
				if ((data_dir = realloc(data_dir,
				    data_cnt * sizeof(*data_dir))) == NULL) {
					fprintf(stderr, "%s: %s\n",
					    progname, strerror(errno));
					exitval = (EXIT_FAILURE);
					goto clean;
				}
			}
			data_dir[data_next++] = optarg;
			break;
		case 'F':
			/* The default is to use environment copy. */
			env_copy = 0;
			break;
		case 'g':
			debug = 1;
			break;
		case 'h':
			home = optarg;
			break;
		case 'i':
			home_blob_dir = optarg;
			break;
		case 'l':
			log_dir = optarg;
			break;
		case 'P':
			if (passwd != NULL) {
				fprintf(stderr, "%s: %s", progname,
				    DB_STR("5133",
				    "Password may not be specified twice\n"));
				free(passwd);
				return (EXIT_FAILURE);
			}
			passwd = strdup(optarg);
			memset(optarg, 0, strlen(optarg));
			if (passwd == NULL) {
				fprintf(stderr, "%s: ", progname);
				fprintf(stderr, DB_STR_A("5026",
				    "strdup: %s\n", "%s\n"), strerror(errno));
				exitval = (EXIT_FAILURE);
				goto clean;
			}
			break;
		case 'u':
			update = 1;
			break;
		case 'V':
			printf("%s\n", db_version(NULL, NULL, NULL));
			exitval = (EXIT_SUCCESS);
			goto clean;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			exitval = usage();
			goto clean;
		}
	argc -= optind;
	argv += optind;

	if (argc != 0) {
		exitval = usage();
		goto clean;
	}

	/* NULL-terminate any list of data directories. */
	if (data_dir != NULL) {
		data_dir[data_next] = NULL;
		/*
		 * -d is relative to the current directory, to run a checkpoint
		 * we must have directories relative to the environment.
		 */
		if (checkpoint == 1) {
			fprintf(stderr, "%s: %s",
			      DB_STR("5027", "cannot specify -d and -c\n"),
			      progname);
			    
			exitval = usage();
			goto clean;
		}
	}

	if (db_config && (data_dir != NULL || log_dir != NULL)) {
		fprintf(stderr, "%s: %s", DB_STR("5028",
		    "cannot specify -D and -d or -l\n"), progname);
		exitval = usage();
		goto clean;
	}

	/* Handle possible interruptions. */
	__db_util_siginit();

	/*
	 * The home directory defaults to the environment variable DB_HOME.
	 * The log directory defaults to the home directory.
	 *
	 * We require a source database environment directory and a target
	 * backup directory.
	 */
	if (home == NULL) {
		home = home_buf;
		if ((ret = __os_getenv(
		    NULL, "DB_HOME", &home, sizeof(home_buf))) != 0) {
			fprintf(stderr, "%s: ", progname);
			fprintf(stderr, DB_STR_A("5029",
		    "failed to get environment variable DB_HOME: %s\n",
			    "%s"), db_strerror(ret));
			exitval = (EXIT_FAILURE);
			goto clean;
		}
		/*
		 * home set to NULL if __os_getenv failed to find DB_HOME.
		 */
	}
	if (home == NULL) {
		fprintf(stderr, "%s: %s", DB_STR("5030",
		    "no source database environment specified\n"), progname);
		exitval = usage();
		goto clean;
	}
	if (backup_dir == NULL) {
		fprintf(stderr, "%s: %s", DB_STR("5031",
		    "no target backup directory specified\n"), progname);
		exitval = usage();
		goto clean;
	}

	if (verbose) {
		(void)time(&now);
		printf("%s: ", progname);
		printf(DB_STR_A("5032", "hot backup started at %s",
		    "%s"), __os_ctime(&now, time_buf));
	}

	/* Open the source environment. */
	if (env_init(&dbenv, home, home_blob_dir,
	     (db_config || log_dir != NULL) ? &log_dir : NULL,
	     &data_dir, passwd, OPEN_ORIGINAL, verbose) != 0)
		goto err;
	
	if (env_copy) {
		if ((ret = dbenv->get_open_flags(dbenv, &flags)) != 0)
			goto err;
		if (flags & DB_PRIVATE) {
			fprintf(stderr, "%s: %s", progname,  DB_STR("5140",
"The environment does not exist or cannot be opened. \"-F\" is required.\n"));
			goto err;
		}
	}

	if (log_dir != NULL) {
		if (db_config && __os_abspath(log_dir)) {
			fprintf(stderr, "%s: %s", progname, DB_STR("5033",
	"DB_CONFIG must not contain an absolute path for the log directory\n"));
			goto err;
		}
	}

	/*
	 * If the -c option is specified, checkpoint the source home
	 * database environment, and remove any unnecessary log files.
	 */
	if (checkpoint) {
		if (verbose) {
			printf("%s: ", progname);
			printf(DB_STR_A("5035", "%s: force checkpoint\n",
			    "%s"), home);
		}
		if ((ret =
		    dbenv->txn_checkpoint(dbenv, 0, 0, DB_FORCE)) != 0) {
			dbenv->err(dbenv, ret, "DB_ENV->txn_checkpoint");
			goto err;
		}
		if (!update) {
			if (verbose) {
				printf("%s: ", progname);
				printf(DB_STR_A("5036",
				    "%s: remove unnecessary log files\n",
				    "%s"), home);
			}
			if ((ret = dbenv->log_archive(dbenv,
			     NULL, DB_ARCH_REMOVE)) != 0) {
				dbenv->err(dbenv, ret, "DB_ENV->log_archive");
				goto err;
			}
		}
	}

	flags = DB_CREATE | DB_BACKUP_CLEAN | DB_BACKUP_FILES;
	if (update)
		LF_SET(DB_BACKUP_UPDATE);

	if (!db_config)
		LF_SET(DB_BACKUP_SINGLE_DIR);
	if ((ret = dbenv->backup(dbenv, backup_dir, flags)) != 0)
		goto err;

	/* Close the source environment. */
	if ((ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: dbenv->close: %s\n", progname, db_strerror(ret));
		dbenv = NULL;
		goto err;
	}
	/* Perform catastrophic recovery on the hot backup. */
	if (verbose) {
		printf("%s: ", progname);
		printf(DB_STR_A("5040", "%s: run catastrophic recovery\n",
		    "%s"), backup_dir);
	}
	if (env_init(&dbenv, backup_dir, NULL,
	    NULL, NULL, passwd, OPEN_HOT_BACKUP, verbose) != 0)
		goto err;

	/*
	 * Remove any unnecessary log files from the hot backup.
	 * For debugging purposes, leave them around.
	 */
	if (debug == 0) {
		if (verbose) {
			printf("%s: ", progname);
			printf(DB_STR_A("5041",
			    "%s: remove unnecessary log files\n",
			    "%s"), backup_dir);
		}

		if ((ret =
		    dbenv->log_archive(dbenv, NULL, DB_ARCH_REMOVE)) != 0) {
			dbenv->err(dbenv, ret, "DB_ENV->log_archive");
			goto err;
		}
	}

	if (0) {
err:		exitval = 1;
	}

	if (dbenv != NULL && (ret = dbenv->close(dbenv, 0)) != 0) {
		exitval = 1;
		fprintf(stderr,
		    "%s: dbenv->close: %s\n", progname, db_strerror(ret));
	}

	if (exitval == 0) {
		if (verbose) {
			(void)time(&now);
			printf("%s: ", progname);
			printf(DB_STR_A("5042",
			    "hot backup completed at %s", "%s"),
			    __os_ctime(&now, time_buf));
		}
	} else {
		fprintf(stderr, "%s: %s", progname,
		     DB_STR("5043", "HOT BACKUP FAILED!\n"));
	}

	/* Resend any caught signal. */
	__db_util_sigresend();

clean:
	if (data_cnt > 0)
		free(data_dir);
	if (passwd != NULL)
		free(passwd);

	return (exitval == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/*
 * env_init --
 *	Open a database environment.
 */
int
env_init(dbenvp, home, blob_dir, log_dirp, data_dirp, passwd, which, verbose)
	DB_ENV **dbenvp;
	char *home, *blob_dir, **log_dirp, ***data_dirp, *passwd;
	enum which_open which;
	int verbose;
{
	DB_ENV *dbenv;
	const char *log_dir, **data_dir;
	char buf[DB_MAXPATHLEN];
	int homehome, ret;

	*dbenvp = NULL;

	/*
	 * Create an environment object and initialize it for error reporting.
	 */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		return (1);
	}

	if (verbose) {
		(void)dbenv->set_verbose(dbenv, DB_VERB_BACKUP, 1);
		dbenv->set_msgcall(dbenv, __db_util_msg);
	}
	dbenv->set_errfile(dbenv, stderr);
	(void)setvbuf(stderr, NULL, _IONBF, 0);
	dbenv->set_errpfx(dbenv, progname);

	/* Any created intermediate directories are created private. */
	if ((ret = dbenv->set_intermediate_dir_mode(dbenv, "rwx------")) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->set_intermediate_dir_mode");
		return (1);
	}

	/* Set the blob directory. */
	if (blob_dir != NULL &&
	    (ret = dbenv->set_blob_dir(dbenv, blob_dir)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->set_blob-dir");
		return (1);
	}

	/*
	 * If a log directory has been specified, and it's not the same as the
	 * home directory, set it for the environment.
	 */
	if (log_dirp != NULL && *log_dirp != NULL &&
	    (ret = dbenv->set_lg_dir(dbenv, *log_dirp)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->set_lg_dir: %s", *log_dirp);
		return (1);
	}

	/* Optionally set the password. */
	if (passwd != NULL &&
	    (ret = dbenv->set_encrypt(dbenv, passwd, DB_ENCRYPT_AES)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->set_encrypt");
		return (1);
	}

	switch (which) {
	case OPEN_ORIGINAL:
		if (data_dirp != NULL && *data_dirp != NULL) {
			/*
			 * Backward compatibility: older versions 
			 * did not have data dirs relative to home.
			 * Check to see if there is a sub-directory with
			 * the same name has the home directory.  If not
			 * trim the home directory from the data directory
			 * passed in.
			 */
			(void) sprintf(buf, "%s/%s", home, home);
			homehome = 0;
			(void)__os_exists(dbenv->env, buf, &homehome);
				
			for (data_dir = (const char **)*data_dirp;
			    *data_dir != NULL; data_dir++) {
				if (!homehome &&
				    !strncmp(home, *data_dir, strlen(home))) {
					if (strchr(PATH_SEPARATOR,
					    (*data_dir)[strlen(home)]) != NULL)
						(*data_dir) += strlen(home) + 1;
					/* Just in case an extra / was added. */
					else if (strchr(PATH_SEPARATOR,
					    home[strlen(home) - 1]) != NULL)
						(*data_dir) += strlen(home);
				}

			    	if ((ret = dbenv->add_data_dir(
				    dbenv, *data_dir)) != 0) {
					dbenv->err(dbenv, ret,
					    "DB_ENV->add_data_dir: %s",
					    *data_dir);
					return (1);
				}
			}
		}
		/*
		 * Opening the database environment we're trying to back up.
		 * We try to attach to a pre-existing environment; if that
		 * fails, we create a private environment and try again.
		 */
		if ((ret = dbenv->open(dbenv, home, DB_USE_ENVIRON, 0)) != 0 &&
		    (ret == DB_VERSION_MISMATCH || ret == DB_REP_LOCKOUT ||
		    (ret = dbenv->open(dbenv, home, DB_CREATE | DB_INIT_LOG |
		    DB_INIT_MPOOL | DB_INIT_TXN | DB_PRIVATE | DB_USE_ENVIRON,
		    0)) != 0)) {
			dbenv->err(dbenv, ret, "DB_ENV->open: %s", home);
			return (1);
		}
		if (log_dirp != NULL) {
			(void)dbenv->get_lg_dir(dbenv, &log_dir);
			if (*log_dirp == NULL)
				*log_dirp = (char*)log_dir;
			else if (strcmp(*log_dirp, log_dir)) {
				fprintf(stderr, DB_STR_A("5057",
		    "%s: cannot specify -l with conflicting DB_CONFIG file\n",
				    "%s\n"), progname);
				return (usage());
			} else {
				/* 
				 * Do we have -l and an existing DB_CONFIG? 
				 * That is a usage problem, but for backward
				 * compatibility, keep going if log_dir happens
				 * to be the same as the DB_CONFIG path.
				 */
				(void)snprintf(buf, sizeof(buf), "%s%c%s", 
				    home, PATH_SEPARATOR[0], "DB_CONFIG");
				if (__os_exists(dbenv->env, buf, NULL) == 0)
					fprintf(stderr, 
					    "%s: %s", DB_STR("5141",
			    "use of -l with DB_CONFIG file is deprecated\n"),
					    progname);
			}
		}
		if (data_dirp != NULL && *data_dirp == NULL)
			(void)dbenv->get_data_dirs(
			    dbenv, (const char ***)data_dirp);
		break;
	case OPEN_HOT_BACKUP:
		/*
		 * Opening the backup copy of the database environment.  We
		 * better be the only user, we're running recovery.
		 * Ensure that there at least minimal cache for worst
		 * case page size.
		 */
		if ((ret =
		    dbenv->set_cachesize(dbenv, 0, 64 * 1024 * 10, 0)) != 0) {
			dbenv->err(dbenv,
			     ret, "DB_ENV->set_cachesize: %s", home);
			return (1);
		}
		if (verbose == 1)
			(void)dbenv->set_verbose(dbenv, DB_VERB_RECOVERY, 1);
		if ((ret = dbenv->open(dbenv, home, DB_CREATE |
		    DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN | DB_PRIVATE |
		    DB_RECOVER_FATAL | DB_USE_ENVIRON, 0)) != 0) {
			dbenv->err(dbenv, ret, "DB_ENV->open: %s", home);
			return (1);
		}
		break;
	}

	*dbenvp = dbenv;
	return (0);
}

int
usage()
{
	(void)fprintf(stderr, "usage: %s [-cDuVv]\n\t%s\n", progname,
"[-d data_dir ...] [-i home_blob_dir] [-h home] [-l log_dir] [-P password] -b backup_dir");
	return (EXIT_FAILURE);
}

int
version_check()
{
	int v_major, v_minor, v_patch;

	/* Make sure we're loaded with the right version of the DB library. */
	(void)db_version(&v_major, &v_minor, &v_patch);
	if (v_major != DB_VERSION_MAJOR || v_minor != DB_VERSION_MINOR) {
		fprintf(stderr, "%s: ", progname);
		fprintf(stderr, DB_STR_A("5071",
		    "version %d.%d doesn't match library version %d.%d\n",
		    "%d %d %d %d\n"),
		    DB_VERSION_MAJOR, DB_VERSION_MINOR,
		    v_major, v_minor);
		return (EXIT_FAILURE);
	}
	return (0);
}
