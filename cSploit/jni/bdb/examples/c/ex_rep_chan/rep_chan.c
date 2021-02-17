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

static int doloop __P((DB_ENV *, GLOBAL *));
static void get_op __P((DB_ENV *, DB_CHANNEL *, GLOBAL *, DBT *,
    u_int32_t));
static int master_op __P((DB_ENV *, GLOBAL *, int, DBT *, int));
static void operation_dispatch __P((DB_ENV *, DB_CHANNEL *, DBT *,
    u_int32_t, u_int32_t));
static int parse_input __P((char *, int *, DBT *, int *));
static int process_reply __P((DB_ENV *, int, DBT *));
static void put_op __P((DB_ENV *, DB_CHANNEL *, int, GLOBAL *, DBT *,
    u_int32_t));
static void send_error_reply __P((DB_CHANNEL *, int));
static void send_reply __P((DB_CHANNEL *, DBT *, int));

const char *progname = "ex_rep_chan";

int
main(argc, argv)
	int argc;
	char *argv[];
{
	DB_ENV *dbenv;
	SETUP_DATA setup_info;
	GLOBAL global;
	thread_t ckp_thr, lga_thr;
	u_int32_t start_policy;
	int ret, t_ret;

	memset(&setup_info, 0, sizeof(SETUP_DATA));
	setup_info.progname = progname;
	memset(&global, 0, sizeof(GLOBAL));
	dbenv = NULL;

	if ((ret = mutex_init(&global.mutex, NULL)) != 0)
		return (ret);
	start_policy = DB_REP_ELECTION;

	if ((ret = create_env(progname, &dbenv)) != 0)
		goto err;
	dbenv->app_private = &global;
	(void)dbenv->set_event_notify(dbenv, event_callback);

	/*
	 * Set up the callback function for the channel.
	 */
	(void)dbenv->repmgr_msg_dispatch(dbenv, operation_dispatch, 0);

	/* Parse command line and perform common replication setup. */
	if ((ret = rep_setup(dbenv, argc, argv, &setup_info)) != 0)
		goto err;

	/* Perform repmgr-specific setup based on command line options. */
	if (setup_info.role == MASTER)
		start_policy = DB_REP_MASTER;
	else if (setup_info.role == CLIENT)
		start_policy = DB_REP_CLIENT;

	if ((ret = env_init(dbenv, setup_info.home)) != 0)
		goto err;

	/* Start checkpoint and log archive threads. */
	global.dbenv = dbenv;
	if ((ret = start_support_threads(dbenv, &global, &ckp_thr,
	    &lga_thr)) != 0)
		goto err;

	if ((ret = dbenv->repmgr_start(dbenv, 3, start_policy)) != 0)
		goto err;

	if ((ret = doloop(dbenv, &global)) != 0) {
		dbenv->err(dbenv, ret, "Site failed");
		goto err;
	}

	/* Finish checkpoint and log archive threads. */
	if ((ret = finish_support_threads(&ckp_thr, &lga_thr)) != 0)
		goto err;

	/*
	 * We have used the DB_TXN_NOSYNC environment flag for improved
	 * performance without the usual sacrifice of transactional durability,
	 * as discussed in the "Transactional guarantees" page of the Reference
	 * Guide: if one replication site crashes, we can expect the data to
	 * exist at another site.  However, in case we shut down all sites
	 * gracefully, we push out the end of the log here so that the most
	 * recent transactions don't mysteriously disappear.
	 */
	if ((ret = dbenv->log_flush(dbenv, NULL)) != 0) {
		dbenv->err(dbenv, ret, "log_flush");
		goto err;
	}

err:
	if (global.channel != NULL &&
	    (t_ret = global.channel->close(global.channel, 0)) != 0) {
		fprintf(stderr, "failure closing channel: %s (%d)\n",
		    db_strerror(t_ret), t_ret);
		if (ret == 0)
			ret = t_ret;
	}
	if (dbenv != NULL &&
	    (t_ret = dbenv->close(dbenv, 0)) != 0) {
		fprintf(stderr, "failure closing env: %s (%d)\n",
		    db_strerror(t_ret), t_ret);
		if (ret == 0)
			ret = t_ret;
	}

	return (ret);
}

/*
 * This is the main user-driven loop of the program.  Read a command
 * from the user and process it, sending it to the master if needed.
 */
static int
doloop(dbenv, global)
	DB_ENV *dbenv;
	GLOBAL *global;
{
	DB *dbp;
	DBT cmdargs[REPCMD_MAX_DBT];
	char buf[BUFSIZE];
	int cmd, ndbt, ret;

	dbp = NULL;
	ret = 0;

	memset(cmdargs, 0, sizeof(DBT) * REPCMD_MAX_DBT);
	for (;;) {
		printf("QUOTESERVER> ");
		fflush(stdout);
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		if ((ret = parse_input(buf, &cmd, cmdargs, &ndbt)) != 0)
			return (ret);
		if (cmd == REPCMD_INVALID)
			continue;
		if (cmd == REPCMD_HELP) {
			print_cmdhelp();
			continue;
		}
		if (cmd == REPCMD_EXIT) {
			global->app_finished = 1;
			break;
		}

		/*
		 * All other commands deal with the database.  Open it
		 * now if it isn't already open.
		 */
		if (dbp == NULL) {
			ret = open_dbp(dbenv, global->is_master, &dbp);
			if (ret == ENOENT || ret == DB_REP_HANDLE_DEAD ||
			    ret == DB_LOCK_DEADLOCK)
				continue;
			if (ret != 0)
				goto err;
		}
		if (cmd == REPCMD_PRINT) {
			/*
			 * If this is a client in the middle of
			 * synchronizing with the master, the client data
			 * is possibly stale and won't be displayed until
			 * client synchronization is finished.  It is also
			 * possible to display the stale data if this is
			 * acceptable to the application.
			 */
			if (global->in_client_sync)
				printf(
"Cannot read data during client synchronization - please try again.\n");
			else
				switch ((ret = print_stocks(dbp))) {
				case 0:
					break;
				case DB_REP_HANDLE_DEAD:
					(void)dbp->close(dbp, DB_NOSYNC);
					dbp = NULL;
					break;
				default:
					dbp->err(dbp, ret,
					    "Error traversing data");
					goto err;
				}
		} else {
			/*
			 * This is a command that should be forwarded
			 * to the master only if we actually have something
			 * to send.
			 *
			 * Even if this site is the master, the channel
			 * does the right thing and executes locally.
			 */
			if (ndbt > 1 && (ret = master_op(dbenv, global,
			    cmd, cmdargs, ndbt)) != 0)
				goto err;
		}
	}

err:
	if (dbp != NULL)
		(void)dbp->close(dbp, DB_NOSYNC);
	return (ret);
}

/*
 * Take the user input and break it up into its list of DBTs.
 */
static int
parse_input(buf, cmdp, argsdbt, ndbtp)
	char *buf;
	int *cmdp, *ndbtp;
	DBT *argsdbt;
{
	DBT *dbtp;
	int i, max, ret;
	char *arg, *cmd;

	*ndbtp = 0;
	ret = 0;

	/*
	 * Commands are:
	 * . <return> | print - Print out current contents.
	 * . ? | help - Print out commands.
	 * . exit | quit | q - Stop program.
	 * . get key key ... - Read the given keys at the master site.
	 * . put key value key value ... - Write given pairs in one txn.
	 * . put_sync key value key value ... - Write given pairs in one txn
	 *    and don't return until the data is replicated to the local site.
	 */
#define	DELIM " \t\n"
	cmd = strtok(&buf[0], DELIM);
	*cmdp = parse_cmd(cmd);

	/*
	 * These commands take no args.  Return (ignoring any args
	 * the user may have actually given us).
	 */
	if (*cmdp == REPCMD_INVALID || *cmdp == REPCMD_HELP ||
	    *cmdp == REPCMD_PRINT || *cmdp == REPCMD_EXIT)
		return (0);

	/*
	 * All other commands require at least one arg.  Print a message
	 * if there are none.  Don't return an error because we don't
	 * want the program to exit, just go back to the user.
	 */
	if ((arg = strtok(NULL, DELIM)) == NULL) {
		printf("%s command expects at least one arg\n",cmd);
		return (0);
	}

	/*
	 * The 0'th DBT is the command we send to the master.
	 */
	dbtp = &argsdbt[0];
	dbtp->data = cmdp;
	dbtp->size = sizeof(int);
	/*
	 * For a get, the master returns key/data pairs so we can only ask for
	 * half the maximum of keys.
	 */
	if (*cmdp == REPCMD_GET)
		max = REPCMD_MAX_DBT / 2;
	else
		max = REPCMD_MAX_DBT;
	for (i = 1; i < max && arg != NULL;
	    i++, arg = strtok(NULL, DELIM)) {
		dbtp = &argsdbt[i];
		dbtp->data = arg;
		dbtp->size = (u_int32_t)strlen(arg);
	}

	/*
	 * See if we reached the maximum size of input.
	 */
	*ndbtp = i;
	if (arg != NULL)
		printf("Reached maximum %d input tokens.  Ignoring remainder\n",
		    max);
	/*
	 * See if we have a mismatched number of key/data pairs for put.
	 * We check for % 2 == 0 for the straggler case because key/data
	 * pairs plus a command gives us an odd number.
	 */
	if ((*cmdp == REPCMD_PUT || *cmdp == REPCMD_PUTSYNC) &&
	    (i % 2) == 0) {
		*ndbtp = i - 1;
		printf("Mismatched key/data pairs.  Ignoring straggler.\n");
	}
	return (0);
}

/*
 * Operations that get forwarded to the master are an array of DBTs.
 * This is what each operation looks like on the request and reply side.
 * For every request, DBT[0] is a header DBT that contains the command.
 * For every reply, DBT[0] is the status;  if DBT[0]
 * is an error indicator, DBT[1] is always the error value.
 *
 * NOTE:  This example assumes a homogeneous set of environments.  If you
 * are running with mixed endian hardware you will need to be aware of
 * byte order issues between sender and recipient.
 *
 * This function is run on the requesting side of the channel.
 *
 * Each operation is performed at the master as a single transaction.
 * For example, when you write 3 key/data pairs in a single 'put' command
 * all 3 will be done under one transaction and will be applied all-or-none.
 *
 * get key1 key2 key3...
 * DBT[0] GET command
 * DBT[1] key1
 * DBT[2] key2...
 * Reply:
 * DBT[0] Success
 * DBT[1] key1
 * DBT[2] data1
 * DBT[3] key2
 * DBT[4] data2...
 *
 * put key1 data1 key2 data2...
 * DBT[0] PUT command
 * DBT[1] key1
 * DBT[2] data1
 * DBT[3] key2
 * DBT[4] data2...
 * Reply:
 * DBT[0] Success
 *
 * put_sync key1 data1 key2 data2...
 * DBT[0] PUT_NOWAIT command
 * DBT[1] key1
 * DBT[2] data1
 * DBT[3] key2
 * DBT[4] data2...
 * Reply:
 * DBT[0] Success
 * DBT[1] Read-your-write txn token
 */
static int
master_op(dbenv, global, cmd, cmdargs, ndbt)
	DB_ENV *dbenv;
	GLOBAL *global;
	int cmd, ndbt;
	DBT *cmdargs;
{
	DB_CHANNEL *chan;
	DBT resp;
	u_int32_t flags;
	int ret;

	memset(&resp, 0, sizeof(DBT));
	resp.flags = DB_DBT_MALLOC;
	if (global->channel == NULL &&
	    (ret = dbenv->repmgr_channel(dbenv, DB_EID_MASTER,
		    &global->channel, 0)) != 0) {
			return (ret);
		/*
		 * If you want to change the default channel timeout
		 * call global->channel->set_timeout here.
		 */
	}

	chan = global->channel;
	flags = 0;
	/*
	 * Send message.  Wait for reply if needed.  All replies can
	 * potentially get multiple DBTs, even a put operation.
	 * The reason is that if an error occurs, we send back an error
	 * indicator header DBT followed by a DBT with the error value.
	 * For get operations, we send back a DBT for each key given.
	 */
	flags = DB_MULTIPLE;
	if ((ret = chan->send_request(chan,
	    cmdargs, ndbt, &resp, 0, flags)) != 0)
		goto out;
	ret = process_reply(dbenv, cmd, &resp);
	free(resp.data);
out:
	return (ret);
}

/*
 * This is the recipient's (master's) callback function.  Functions called
 * by this function are going to be performed at the master and then the
 * results packaged up and sent back to the request originator.
 *
 * This function runs on the serving side of the channel.
 */
static void
operation_dispatch(dbenv, chan, reqdbts, ndbt, flags)
	DB_ENV *dbenv;
	DB_CHANNEL *chan;
	DBT *reqdbts;
	u_int32_t ndbt, flags;
{
	DBT *d;
	GLOBAL *global = dbenv->app_private;
	int cmd, ret;
	
	d = &reqdbts[0];
	cmd = *(int *)d->data;
	if (cmd != REPCMD_PUT && cmd != REPCMD_PUTSYNC &&
	    cmd != REPCMD_GET) {
		fprintf(stderr, "Received unknown operation %d\n", cmd);
		abort();
	}
	if (mutex_lock(&global->mutex) != 0) {
		fprintf(stderr, "can't lock mutex\n");
		abort();
	}
	if (global->dbp == NULL) {
retry:
		ret = open_dbp(dbenv, global->is_master, &global->dbp);
		if (ret == ENOENT || ret == DB_REP_HANDLE_DEAD ||
		    ret == DB_LOCK_DEADLOCK) {
			dbenv->err(dbenv, ret, "Retry opening database file.");
			goto retry;
		}
		/*
		 * If the sender is expecting a reply, send the error value.
		 */
		if (ret != 0 && flags != DB_REPMGR_NEED_RESPONSE) {
			send_error_reply(chan, ret);
			return;
		}
	}
	if (mutex_unlock(&global->mutex) != 0) {
		fprintf(stderr, "can't unlock mutex\n");
		abort();
	}
	switch (cmd) {
	case REPCMD_GET:
		get_op(dbenv, chan, global, reqdbts, ndbt);
		break;
	case REPCMD_PUT:
	case REPCMD_PUTSYNC:
		put_op(dbenv, chan, cmd, global, reqdbts, ndbt);
		break;
	}
	return;
}

/*
 * Receive a list of keys and send back the list of key/data pairs.
 *
 * This function runs on the serving side of the channel.
 */
static void
get_op(dbenv, chan, global, reqdbts, ndbt)
	DB_ENV *dbenv;
	DB_CHANNEL *chan;
	GLOBAL *global;
	DBT *reqdbts;
	u_int32_t ndbt;
{
	DB *dbp;
	DBT *key, reply[REPCMD_MAX_DBT];
	DB_TXN *txn;
	u_int32_t i, resp_ndbt;
	int ret;

	/*
	 * We have a valid dbp in the global structure at this point.
	 */
	memset(&reply, 0, sizeof(DBT) * REPCMD_MAX_DBT);
	if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
		txn = NULL;
		goto err;
	}

	dbp = global->dbp;
	/*
	 * Start at 1.  The 0'th DBT is reserved for the header.
	 * For the request DBTs, it contains the request header and for
	 * the reply DBTs it will contain the reply header.
	 * That will be filled in when we send the reply.
	 *
	 * On the reply, copy in the key given and then the data, so that
	 * we send back key/data pairs.
	 */
	for (resp_ndbt = 1, i = 1; i < ndbt; i++) {
		key = &reqdbts[i];
		/*
		 * Remove setting the DB_DBT_MALLOC flag for the key DBT
		 * that we're sending back in the reply.
		 */
		reply[resp_ndbt].data = reqdbts[i].data;
		reply[resp_ndbt].size = reqdbts[i].size;
		reply[resp_ndbt++].flags = 0;
		reply[resp_ndbt].flags = DB_DBT_MALLOC;
		if ((ret = dbp->get(dbp,
		    txn, key, &reply[resp_ndbt++], 0)) != 0)
			goto err;
	}
	if ((ret = txn->commit(txn, 0)) != 0) {
		txn = NULL;
		goto err;
	}
	send_reply(chan, &reply[0], resp_ndbt);
	/*
	 * On success, free any DBT that used DB_DBT_MALLOC.
	 */
	for (i = 1; i < resp_ndbt; i++) {
		if (reply[i].flags == DB_DBT_MALLOC)
			free(reply[i].data);
	}
	return;

err:
	if (txn != NULL)
		(void)txn->abort(txn);
	send_error_reply(chan, ret);
	for (i = 1; i < resp_ndbt; i++) {
		if (reply[i].flags == DB_DBT_MALLOC && reply[i].data != NULL)
			free(reply[i].data);
	}
	return;
	
}

/*
 * Receive a list of key/data pairs to write into the database.
 * If the request originator asked, send back the txn read token.
 *
 * This function runs on the serving side of the channel.
 */
static void
put_op(dbenv, chan, cmd, global, reqdbts, ndbt)
	DB_ENV *dbenv;
	DB_CHANNEL *chan;
	int cmd;
	GLOBAL *global;
	DBT *reqdbts;
	u_int32_t ndbt;
{
	DB *dbp;
	DBT *data, *key;
	DBT reply[REPCMD_MAX_DBT];
	DB_TXN *txn;
	DB_TXN_TOKEN t;
	u_int32_t i, reply_ndbt;
	int ret;

	/*
	 * We have a valid dbp in the global structure at this point.
	 */
	memset(reply, 0, sizeof(DBT) * REPCMD_MAX_DBT);
	if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0) {
		txn = NULL;
		goto err;
	}

	dbp = global->dbp;
	/*
	 * Start at 1.  The 0'th DBT is reserved for the header.
	 * For the request DBTs, it contains the request header and for
	 * the reply DBTs it will contain the reply header.
	 * That will be filled in when we send the reply.
	 */
	if (ndbt % 2 == 0) {
		fprintf(stderr, "ERROR: Unpaired key/data items.\n");
		ret = EINVAL;
		goto err;
	}
	for (i = 1; i < ndbt; i+=2) {
		key = &reqdbts[i];
		data = &reqdbts[i + 1];
		if ((ret = dbp->put(dbp, txn, key, data, 0)) != 0)
			goto err;
	}
	if (cmd == REPCMD_PUTSYNC &&
	    (ret = txn->set_commit_token(txn, &t)) != 0)
		goto err;
	if ((ret = txn->commit(txn, 0)) != 0) {
		txn = NULL;
		goto err;
	}
	if (cmd == REPCMD_PUTSYNC) {
		reply[1].data = &t;
		reply[1].size = sizeof(DB_TXN_TOKEN);
		reply_ndbt = 2;
	} else
		reply_ndbt = 1;
		
	send_reply(chan, &reply[0], reply_ndbt);
	return;

err:
	if (txn != NULL)
		(void)txn->abort(txn);
	send_error_reply(chan, ret);
	return;
}

/*
 * Send an error reply.  DBT[0] is the error status and
 * DBT[1] is the error return value.
 *
 * This function runs on the serving side of the channel.
 */
static void
send_error_reply(chan, ret)
	DB_CHANNEL *chan;
	int ret;
{
	DBT d[REPLY_ERROR_NDBT];
	int r;

	r = REPREPLY_ERROR;
	d[0].data = &r;
	d[0].size = sizeof(int);
	d[1].data = &ret;
	d[1].size = sizeof(int);
	(void)chan->send_msg(chan, d, REPLY_ERROR_NDBT, 0);
	return;
}

/*
 * Send back a success reply.  DBT[0] is the success status.
 * The rest of the DBTs were already filled in by the caller.
 *
 * This function runs on the serving side of the channel.
 */
static void
send_reply(chan, reply, ndbt)
	DB_CHANNEL *chan;
	DBT *reply;
	int ndbt;
{
	int r;

	/*
	 * Fill in the reply header now.  The rest is already
	 * filled in for us.
	 */
	r = REPREPLY_OK;
	reply[0].data = &r;
	reply[0].size = sizeof(int);
	(void)chan->send_msg(chan, reply, ndbt, 0);
	return;
}

/*
 * Process the reply sent by the master.  All operations use DB_MULTIPLE
 * because all have the potential to return multiple DBTs.
 *
 * This function runs on the requesting side of the channel.
 */
static int
process_reply(dbenv, cmd, resp)
	DB_ENV *dbenv;
	int cmd;
	DBT *resp;
{
	u_int32_t dlen, hlen, klen;
	int first, ret;
	char *cmdstr;
	void *dp, *hdr, *kp, *p;

	dlen = 0;
	kp = NULL;
	ret = 0;

	if (cmd == REPCMD_GET)
		cmdstr = "DB->get";
	else
		cmdstr = "DB->put";

	DB_MULTIPLE_INIT(p, resp);
	/*
	 * First part of all replies is the header.
	 * If it is an error reply, there is only 1 more expected data DBT,
	 * which is the error value.  If it is success, then what we do
	 * depends on the command operations.
	 */
	DB_MULTIPLE_NEXT(p, resp, hdr, hlen);
	if (*(int *)hdr == REPREPLY_ERROR) {
		DB_MULTIPLE_NEXT(p, resp, dp, dlen);
		ret = *(int *)dp;
		dbenv->err(dbenv, *(int *)dp, cmdstr);
		if (cmd == REPCMD_GET && ret == DB_NOTFOUND)
			ret = 0;
	} else if (cmd != REPCMD_PUT) {
		if (cmd == REPCMD_PUTSYNC) {
			/*
			 * The only expected successful response from this is
			 * the token.  Get it and wait for it to be applied
			 * locally.
			 */
			DB_MULTIPLE_NEXT(p, resp, dp, dlen);
			ret = dbenv->txn_applied(dbenv,
			    (DB_TXN_TOKEN *)dp, REPLY_TOKEN_TIMEOUT, 0);
			if (ret == DB_NOTFOUND)
				fprintf(stderr,
				    "%s: Token never expected to arrive.\n",
				    cmdstr);
			if (ret == DB_TIMEOUT)
				fprintf(stderr,
				    "%s: Token arrival timed out.\n",
				    cmdstr);
			goto out;
		} else {
			/*
			 * We have a get with an arbitrary number of key/data
			 * pairs as responses.  But they should come in pairs.
			 */
			first = 1;
			do {
				DB_MULTIPLE_NEXT(p, resp, kp, klen);
				/*
				 * If p is NULL here, we processed our last
				 * set of key/data pairs.
				 */
				if (p != NULL) {
					DB_MULTIPLE_NEXT(p, resp, dp, dlen);
					/*
					 * If p is NULL here, we got a key
					 * but no data.  That is an error.
					 */
					if (p == NULL) {
						fprintf(stderr,
    "%s: Unexpected pair mismatch\n", cmdstr);
						ret = EINVAL;
						goto out;
					}
					print_one(kp, klen, dp, dlen, first);
				}
				first = 0;
			} while (p != NULL);
		}
	}

out:
	return (ret);
}
