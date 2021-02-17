/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <db.h>

#include "rep_chan.h"

/*
 * Perform command line parsing and replication setup.
 */
int
rep_setup(dbenv, argc, argv, setup_info)
	DB_ENV *dbenv;
	int argc;
	char *argv[];
	SETUP_DATA *setup_info;
{
	DB_SITE *dbsite;
	repsite_t site, *site_list;
	extern char *optarg;
	char ch, *portstr;
	int ack_policy, got_self, i, maxsites, priority, ret;

	got_self = maxsites = ret = site.peer = site.creator = 0;

	priority = 100;
	ack_policy = DB_REPMGR_ACKS_QUORUM;
	setup_info->role = UNKNOWN;

	/*
	 * Replication setup calls that are only needed if a command
	 * line option is specified are made within this while/switch
	 * statement.  Replication setup calls that should be made
	 * whether or not a command line option is specified are after
	 * this while/switch statement.
	 */
	while ((ch = getopt(argc, argv, "Ch:L:l:Mp:R:r:v")) != EOF) {
		switch (ch) {
		case 'C':
			setup_info->role = CLIENT;
			break;
		case 'h':
			setup_info->home = optarg;
			break;
		case 'L':
			setup_info->self.creator = 1; /* FALLTHROUGH */
		case 'l':
			setup_info->self.host = strtok(optarg, ":");
			if ((portstr = strtok(NULL, ":")) == NULL) {
				fprintf(stderr, "Bad host specification.\n");
				goto err;
			}
			setup_info->self.port = (unsigned short)atoi(portstr);
			setup_info->self.peer = 0;
			got_self = 1;
			break;
		case 'M':
			setup_info->role = MASTER;
			break;
		case 'p':
			priority = atoi(optarg);
			break;
		case 'R':
			site.peer = 1; /* FALLTHROUGH */
		case 'r':
			site.host = optarg;
			site.host = strtok(site.host, ":");
			if ((portstr = strtok(NULL, ":")) == NULL) {
				fprintf(stderr, "Bad host specification.\n");
				goto err;
			}
			site.port = (unsigned short)atoi(portstr);
			if (setup_info->site_list == NULL ||
			    setup_info->remotesites >= maxsites) {
				maxsites = maxsites == 0 ? 10 : 2 * maxsites;
				if ((setup_info->site_list =
				    realloc(setup_info->site_list,
				    maxsites * sizeof(repsite_t))) == NULL) {
					fprintf(stderr, "System error %s\n",
					    strerror(errno));
					goto err;
				}
			}
			(setup_info->site_list)[(setup_info->remotesites)++] =
				site;
			site.peer = 0;
			site.creator = 0;
			break;
		case 'v':
			if ((ret = dbenv->set_verbose(dbenv,
			    DB_VERB_REPLICATION, 1)) != 0)
				goto err;
			break;
		case '?':
		default:
			usage(setup_info->progname);
		}
	}

	/* Error check command line. */
	if (!got_self || setup_info->home == NULL)
		usage(setup_info->progname);

	/*
	 * Set replication group election priority for this environment.
	 * An election first selects the site with the most recent log
	 * records as the new master.  If multiple sites have the most
	 * recent log records, the site with the highest priority value
	 * is selected as master.
	 */
	if ((ret = dbenv->rep_set_priority(dbenv, priority)) != 0) {
		dbenv->err(dbenv, ret, "Could not set priority.\n");
		goto err;
	}

	/*
	 * Set the policy that determines how master and client
	 * sites handle acknowledgement of replication messages needed for
	 * permanent records.  The default policy of "quorum" requires only
	 * a quorum of electable peers sufficient to ensure a permanent
	 * record remains durable if an election is held.
	 */
	if ((ret = dbenv->repmgr_set_ack_policy(dbenv, ack_policy)) != 0) {
		dbenv->err(dbenv, ret, "Could not set ack policy.\n");
		goto err;
	}

	/*
	 * Set the threshold for the minimum and maximum time the client
	 * waits before requesting retransmission of a missing message.
	 * Base these values on the performance and load characteristics
	 * of the master and client host platforms as well as the round
	 * trip message time.
	 */
	if ((ret = dbenv->rep_set_request(dbenv, 20000, 500000)) != 0) {
		dbenv->err(dbenv, ret,
		    "Could not set client_retransmission defaults.\n");
		goto err;
	}

	/*
	 * Configure deadlock detection to ensure that any deadlocks
	 * are broken by having one of the conflicting lock requests
	 * rejected. DB_LOCK_DEFAULT uses the lock policy specified
	 * at environment creation time or DB_LOCK_RANDOM if none was
	 * specified.
	 */
	if ((ret = dbenv->set_lk_detect(dbenv, DB_LOCK_DEFAULT)) != 0) {
		dbenv->err(dbenv, ret,
		    "Could not configure deadlock detection.\n");
		goto err;
	}

	/* Configure the local site. */
	if ((ret = dbenv->repmgr_site(dbenv, setup_info->self.host,
	    setup_info->self.port, &dbsite, 0)) != 0) {
		dbenv->err(dbenv, ret, "Could not set local site.");
		goto err;
	}
	dbsite->set_config(dbsite, DB_LOCAL_SITE, 1);
	if (setup_info->self.creator)
		dbsite->set_config(dbsite, DB_GROUP_CREATOR, 1);

	if ((ret = dbsite->close(dbsite)) != 0) {
		dbenv->err(dbenv, ret, "DB_SITE->close");
		goto err;
	}

	site_list = setup_info->site_list;
	for (i = 0; i < setup_info->remotesites; i++) {
		if ((ret = dbenv->repmgr_site(dbenv, site_list[i].host,
		    site_list[i].port, &dbsite, 0)) != 0) {
			dbenv->err(dbenv, ret, "Could not add site %s:%d",
			    site_list[i].host, (int)site_list[i].port);
			goto err;
		}
		dbsite->set_config(dbsite, DB_BOOTSTRAP_HELPER, 1);
		if (site_list[i].peer)
			dbsite->set_config(dbsite, DB_REPMGR_PEER, 1);
		if ((ret = dbsite->close(dbsite)) != 0) {
			dbenv->err(dbenv, ret, "DB_SITE->close");
		}
	}

	/*
	 * Configure heartbeat timeouts so that repmgr monitors the
	 * health of the TCP connection.  Master sites broadcast a heartbeat
	 * at the frequency specified by the DB_REP_HEARTBEAT_SEND timeout.
	 * Client sites wait for message activity the length of the
	 * DB_REP_HEARTBEAT_MONITOR timeout before concluding that the
	 * connection to the master is lost.  The DB_REP_HEARTBEAT_MONITOR
	 * timeout should be longer than the DB_REP_HEARTBEAT_SEND timeout.
	 */
	if ((ret = dbenv->rep_set_timeout(dbenv, DB_REP_HEARTBEAT_SEND,
	    5000000)) != 0)
		dbenv->err(dbenv, ret,
		    "Could not set heartbeat send timeout.\n");
	if ((ret = dbenv->rep_set_timeout(dbenv, DB_REP_HEARTBEAT_MONITOR,
	    10000000)) != 0)
		dbenv->err(dbenv, ret,
		    "Could not set heartbeat monitor timeout.\n");

err:
	return (ret);
}

void
event_callback(dbenv, which, info)
	DB_ENV *dbenv;
	u_int32_t which;
	void *info;
{
	GLOBAL *global = dbenv->app_private;
	int err;

	switch (which) {
	case DB_EVENT_PANIC:
		err = *(int*)info;
		printf("Got a panic: %s (%d)\n", db_strerror(err), err);
		abort();
	case DB_EVENT_REP_CLIENT:
		if ((err = mutex_lock(&global->mutex)) != 0) {
			fprintf(stderr, "can't lock mutex %d\n", err);
			abort();
		}
		global->is_master = 0;
		global->in_client_sync = 1;
		if (mutex_unlock(&global->mutex) != 0) {
			fprintf(stderr, "can't unlock mutex\n");
			abort();
		}
		break;
	case DB_EVENT_REP_MASTER:
		if ((err = mutex_lock(&global->mutex)) != 0) {
			fprintf(stderr, "can't lock mutex %d\n", err);
			abort();
		}
		global->is_master = 1;
		global->in_client_sync = 0;
		if (mutex_unlock(&global->mutex) != 0) {
			fprintf(stderr, "can't unlock mutex\n");
			abort();
		}
		break;
	case DB_EVENT_REP_NEWMASTER:
		global->in_client_sync = 1;
		break;
	case DB_EVENT_REP_PERM_FAILED:
		/*
		 * Did not get enough acks to guarantee transaction
		 * durability based on the configured ack policy.  This
		 * transaction will be flushed to the master site's
		 * local disk storage for durability.
		 */
		printf(
    "Insufficient acknowledgements to guarantee transaction durability.\n");
		break;
	case DB_EVENT_REP_STARTUPDONE:
		global->in_client_sync = 0;
		break;
	default:
		dbenv->errx(dbenv, "ignoring event %d", which);
		break;
	}
}

int
print_stocks(dbp)
	DB *dbp;
{
	DBC *dbc;
	DBT key, data;
	int first, ret, t_ret;

	if ((ret = dbp->cursor(dbp, NULL, &dbc, 0)) != 0) {
		dbp->err(dbp, ret, "can't open cursor");
		return (ret);
	}

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	for (first = 1, ret = dbc->get(dbc, &key, &data, DB_FIRST);
	    ret == 0;
	    ret = dbc->get(dbc, &key, &data, DB_NEXT)) {
		    print_one(key.data, key.size, data.data, data.size, first);
		    first = 0;
	}
	if ((t_ret = dbc->close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	switch (ret) {
	case 0:
	case DB_NOTFOUND:
	case DB_LOCK_DEADLOCK:
		return (0);
	default:
		return (ret);
	}
}

void
print_one(key, klen, data, dlen, print_hdr)
	void *key, *data;
	u_int32_t klen, dlen;
	int print_hdr;
{
#define	MAXKEYSIZE	10
#define	MAXDATASIZE	20
	char keybuf[MAXKEYSIZE + 1], databuf[MAXDATASIZE + 1];
	u_int32_t keysize, datasize;

	if (print_hdr) {
		printf("\tSymbol\tPrice\n");
		printf("\t======\t=====\n");
	}

	keysize = klen > MAXKEYSIZE ? MAXKEYSIZE : klen;
	memcpy(keybuf, key, keysize);
	keybuf[keysize] = '\0';

	datasize = dlen >= MAXDATASIZE ? MAXDATASIZE : dlen;
	memcpy(databuf, data, datasize);
	databuf[datasize] = '\0';
	printf("\t%s\t%s\n", keybuf, databuf);
	printf("\n");
	fflush(stdout);
}

/* Start checkpoint and log archive support threads. */
int
start_support_threads(dbenv, sup_args, ckp_thr, lga_thr)
	DB_ENV *dbenv;
	GLOBAL *sup_args;
	thread_t *ckp_thr;
	thread_t *lga_thr;
{
	int ret;

	ret = 0;
	if ((ret = thread_create(ckp_thr, NULL, checkpoint_thread,
	    sup_args)) != 0) {
		dbenv->errx(dbenv, "can't create checkpoint thread");
		goto err;
	}
	if ((ret = thread_create(lga_thr, NULL, log_archive_thread,
	    sup_args)) != 0)
		dbenv->errx(dbenv, "can't create log archive thread");
err:
	return (ret);

}

/* Wait for checkpoint and log archive support threads to finish. */
int
finish_support_threads(ckp_thr, lga_thr)
	thread_t *ckp_thr;
	thread_t *lga_thr;
{
	void *ctstatus, *ltstatus;
	int ret;

	ret = 0;
	if (thread_join(*lga_thr, &ltstatus) ||
	    thread_join(*ckp_thr, &ctstatus)) {
		ret = -1;
		goto err;
	}
	if ((uintptr_t)ltstatus != EXIT_SUCCESS ||
	    (uintptr_t)ctstatus != EXIT_SUCCESS)
		ret = -1;
err:
	return (ret);
}

int
create_env(progname, dbenvp)
	const char *progname;
	DB_ENV **dbenvp;
{
	DB_ENV *dbenv;
	int ret;

	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr, "can't create env handle: %s\n",
		    db_strerror(ret));
		return (ret);
	}

	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);

	*dbenvp = dbenv;
	return (0);
}

/* Open and configure an environment. */
int
env_init(dbenv, home)
	DB_ENV *dbenv;
	const char *home;
{
	u_int32_t flags;
	int ret;

	(void)dbenv->set_cachesize(dbenv, 0, CACHESIZE, 0);
	(void)dbenv->set_flags(dbenv, DB_TXN_NOSYNC, 1);

	flags = DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL |
	    DB_INIT_REP | DB_INIT_TXN | DB_RECOVER | DB_THREAD;
	if ((ret = dbenv->open(dbenv, home, flags, 0)) != 0)
		dbenv->err(dbenv, ret, "can't open environment");
	return (ret);
}

/*
 * In this application, we specify all communication via the command line.  In
 * a real application, we would expect that information about the other sites
 * in the system would be maintained in some sort of configuration file.  The
 * critical part of this interface is that we assume at startup that we can
 * find out
 *	1) what host/port we wish to listen on for connections,
 *	2) a (possibly empty) list of other sites we should attempt to connect
 *	to; and
 *	3) what our Berkeley DB home environment is.
 *
 * These pieces of information are expressed by the following flags.
 * -C or -M start up as client or master
 * -h home directory (required)
 * -l host:port (required unless -L is specified; l stands for local)
 * -L host:port (optional; L means the group creator)
 * -p priority (optional: defaults to 100)
 * -r host:port (optional; r stands for remote; any number of these may be
 *	specified)
 * -R host:port (optional; remote peer)
 * -v (optional; v stands for verbose)
 */
void
usage(progname)
	const char *progname;
{
	fprintf(stderr, "usage: %s ", progname);
	fprintf(stderr, "[-CM]-h home -l|-L host:port %s\n",
	    "[-r host:port][-R host:port][-p priority][-v]");
	exit(EXIT_FAILURE);
}

/*
 * This is a very simple thread that performs checkpoints at a fixed
 * time interval.  For a master site, the time interval is one minute
 * plus the duration of the checkpoint_delay timeout (30 seconds by
 * default.)  For a client site, the time interval is one minute.
 */
void *
checkpoint_thread(args)
	void *args;
{
	DB_ENV *dbenv;
	GLOBAL *global;
	int i, ret;

	global = (GLOBAL *)args;
	dbenv = global->dbenv;

	for (;;) {
		/*
		 * Wait for one minute, polling once per second to see if
		 * application has finished.  When application has finished,
		 * terminate this thread.
		 */
		for (i = 0; i < 60; i++) {
			sleep(1);
			if (global->app_finished == 1)
				return ((void *)EXIT_SUCCESS);
		}

		/* Perform a checkpoint. */
		if ((ret = dbenv->txn_checkpoint(dbenv, 0, 0, 0)) != 0) {
			dbenv->err(dbenv, ret,
			    "Could not perform checkpoint.\n");
			return ((void *)EXIT_FAILURE);
		}
	}
}

/*
 * This is a simple log archive thread.  Once per minute, it removes all but
 * the most recent 3 logs that are safe to remove according to a call to
 * DB_ENV->log_archive().
 *
 * Log cleanup is needed to conserve disk space, but aggressive log cleanup
 * can cause more frequent client initializations if a client lags too far
 * behind the current master.  This can happen in the event of a slow client,
 * a network partition, or a new master that has not kept as many logs as the
 * previous master.
 *
 * The approach in this routine balances the need to mitigate against a
 * lagging client by keeping a few more of the most recent unneeded logs
 * with the need to conserve disk space by regularly cleaning up log files.
 * Use of automatic log removal (DB_ENV->log_set_config() DB_LOG_AUTO_REMOVE
 * flag) is not recommended for replication due to the risk of frequent
 * client initializations.
 */
void *
log_archive_thread(args)
	void *args;
{
	DB_ENV *dbenv;
	GLOBAL *global;
	char **begin, **list;
	int i, listlen, logs_to_keep, minlog, ret;

	global = (GLOBAL *)args;
	dbenv = global->dbenv;
	logs_to_keep = 3;

	for (;;) {
		/*
		 * Wait for one minute, polling once per second to see if
		 * application has finished.  When application has finished,
		 * terminate this thread.
		 */
		for (i = 0; i < 60; i++) {
			sleep(1);
			if (global->app_finished == 1)
				return ((void *)EXIT_SUCCESS);
		}

		/* Get the list of unneeded log files. */
		if ((ret = dbenv->log_archive(dbenv, &list, DB_ARCH_ABS))
		    != 0) {
			dbenv->err(dbenv, ret,
			    "Could not get log archive list.");
			return ((void *)EXIT_FAILURE);
		}
		if (list != NULL) {
			listlen = 0;
			/* Get the number of logs in the list. */
			for (begin = list; *begin != NULL; begin++, listlen++);
			/*
			 * Remove all but the logs_to_keep most recent
			 * unneeded log files.
			 */
			minlog = listlen - logs_to_keep;
			for (begin = list, i= 0; i < minlog; list++, i++) {
				if ((ret = unlink(*list)) != 0) {
					dbenv->err(dbenv, ret,
					    "logclean: remove %s", *list);
					dbenv->errx(dbenv,
					    "logclean: Error remove %s", *list);
					free(begin);
					return ((void *)EXIT_FAILURE);
				}
			}
			free(begin);
		}
	}
}

int
parse_cmd(cmd)
	char *cmd;
{
	 /* 
	  * Commands are:
	  * <return> | print
	  * exit | quit | q 
	  * get 
	  * put
	  * put_sync
	  */
	if (cmd == NULL || strcmp(cmd, "print") == 0)
		return (REPCMD_PRINT);
	if (strcmp(cmd, "exit") == 0 ||
	    strcmp(cmd, "quit") == 0 ||
	    strcmp(cmd, "q") == 0)
		return (REPCMD_EXIT);
	if (strcmp(cmd, "get") == 0)
		return (REPCMD_GET);
	if (strcmp(cmd, "?") == 0 ||
	    strcmp(cmd, "help") == 0)
		return (REPCMD_HELP);
	if (strcmp(cmd, "put_sync") == 0)
		return (REPCMD_PUTSYNC);
	if (strcmp(cmd, "put") == 0)
		return (REPCMD_PUT);
	printf("Unknown invalid command %s\n", cmd);
	return (REPCMD_INVALID);
}

void
print_cmdhelp()
{
	printf("<return> | print - Print out current contents.\n");
	printf("? | help - Print out commands.\n");
	printf("exit | quit - Stop program.\n");
	printf("get key key ... - Read the given keys at the master site.\n");
	printf("put key value key value ... - Write given pairs in one txn.\n");
	printf("put_sync key value key value ... ");
	printf("- Write given pairs in one\n");
	printf("  txn and don't return until locally available.\n");
	return;
}

int
open_dbp(dbenv, is_master, dbpp)
	DB_ENV *dbenv;
	int is_master;
	DB **dbpp;
{
	DB *dbp;
	u_int32_t flags;
	int ret, t_ret;

	if ((ret = db_create(dbpp, dbenv, 0)) != 0)
		return (ret);

	dbp = *dbpp;
	flags = DB_AUTO_COMMIT;
	/*
	 * Open database with DB_CREATE only if this is
	 * a master database.  A client database uses
	 * polling to attempt to open the database without
	 * DB_CREATE until it is successful. 
	 *
	 * This DB_CREATE polling logic can be simplified
	 * under some circumstances.  For example, if the
	 * application can be sure a database is already
	 * there, it would never need to open it with
	 * DB_CREATE.
	 */
	if (is_master)
		flags |= DB_CREATE;
	
	if ((ret = dbp->open(dbp,
	    NULL, DATABASE, NULL, DB_BTREE, flags, 0)) != 0) {
		if (ret == ENOENT) {
			printf("No stock database yet available.\n");
			*dbpp = NULL;
			if ((t_ret = dbp->close(dbp, 0)) != 0) {
				ret = t_ret;
				dbenv->err(dbenv, ret, "DB->close");
				goto err;
			}
		}
		if (ret == DB_REP_HANDLE_DEAD ||
		    ret == DB_LOCK_DEADLOCK) {
			dbenv->err(dbenv, ret,
			    "please retry the operation");
			dbp->close(dbp, DB_NOSYNC);
			*dbpp = NULL;
		}
		dbenv->err(dbenv, ret, "DB->open");
		goto err;
	}
err:
	return (ret);
}
