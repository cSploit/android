/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

typedef int (*HEARTBEAT_ACTION) __P((ENV *));

static int accept_handshake __P((ENV *, REPMGR_CONNECTION *, char *, int *));
static int accept_v1_handshake __P((ENV *, REPMGR_CONNECTION *, char *));
static void check_min_log_file __P((ENV *));
static int dispatch_msgin __P((ENV *, REPMGR_CONNECTION *));
static int prepare_input __P((ENV *, REPMGR_CONNECTION *));
static int process_own_msg __P((ENV *, REPMGR_CONNECTION *));
static int process_parameters __P((ENV *,
    REPMGR_CONNECTION *, char *, u_int, u_int32_t, int, u_int32_t));
static int read_version_response __P((ENV *, REPMGR_CONNECTION *));
static int record_permlsn __P((ENV *, REPMGR_CONNECTION *));
static int __repmgr_call_election __P((ENV *));
static int __repmgr_check_listener __P((ENV *));
static int __repmgr_check_master_listener __P((ENV *));
static int __repmgr_connector_main __P((ENV *, REPMGR_RUNNABLE *));
static void *__repmgr_connector_thread __P((void *));
static int __repmgr_next_timeout __P((ENV *,
    db_timespec *, HEARTBEAT_ACTION *));
static int __repmgr_reset_last_rcvd __P((ENV *));
static int __repmgr_retry_connections __P((ENV *));
static int __repmgr_send_heartbeat __P((ENV *));
static int __repmgr_start_takeover __P((ENV *));
static void *__repmgr_takeover_thread __P((void *));
static int __repmgr_try_one __P((ENV *, int, int));
static int resolve_collision __P((ENV *, REPMGR_SITE *, REPMGR_CONNECTION *));
static int send_version_response __P((ENV *, REPMGR_CONNECTION *));

#define	ONLY_HANDSHAKE(env, conn) do {				     \
	if (conn->msg_type != REPMGR_HANDSHAKE) {		     \
		__db_errx(env, DB_STR_A("3613",		     \
		    "unexpected msg type %d in state %d", "%d %d"),  \
		    (int)conn->msg_type, conn->state);		     \
		return (DB_REP_UNAVAIL);			     \
	}							     \
} while (0)

/*
 * PUBLIC: void *__repmgr_select_thread __P((void *));
 */
void *
__repmgr_select_thread(argsp)
	void *argsp;
{
	ENV *env;
	DB_THREAD_INFO *ip;
	int ret;
	REPMGR_RUNNABLE *args;

	args = argsp;
	env = args->env;
	ip = NULL;
	ret = 0;

	ENV_ENTER_RET(env, ip, ret);
	if (ret != 0 || (ret = __repmgr_select_loop(env)) != 0) {
		__db_err(env, ret, DB_STR("3614", "select loop failed"));
		ENV_LEAVE(env, ip);
		(void)__repmgr_thread_failure(env, ret);
	}
	if (ret == 0)
		ENV_LEAVE(env, ip);
	return (NULL);
}

/*
 * PUBLIC: int __repmgr_bow_out __P((ENV *));
 */
int
__repmgr_bow_out(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	int ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	LOCK_MUTEX(db_rep->mutex);
	ret = __repmgr_stop_threads(env);
	UNLOCK_MUTEX(db_rep->mutex);
	/*
	 * Reset sites_avail so that it will be calculated correctly if this
	 * site rejoins the group in the future.
	 */
	rep->sites_avail = 0;
	DB_EVENT(env, DB_EVENT_REP_LOCAL_SITE_REMOVED, NULL);
	return (ret);
}

/*
 * PUBLIC: int __repmgr_accept __P((ENV *));
 */
int
__repmgr_accept(env)
	ENV *env;
{
	DB_REP *db_rep;
	REPMGR_CONNECTION *conn;
	ACCEPT_ADDR siaddr;
	socklen_t addrlen;
	socket_t s;
	int ret;

	db_rep = env->rep_handle;
	addrlen = sizeof(siaddr);
	if ((s = accept(db_rep->listen_fd, (struct sockaddr *)&siaddr,
	    &addrlen)) == -1) {
		/*
		 * Some errors are innocuous and so should be ignored.  MSDN
		 * Library documents the Windows ones; the Unix ones are
		 * advocated in Stevens' UNPv1, section 16.6; and Linux
		 * Application Development, p. 416.
		 */
		switch (ret = net_errno) {
#ifdef DB_WIN32
		case WSAECONNRESET:
		case WSAEWOULDBLOCK:
#else
		case EINTR:
		case EWOULDBLOCK:
		case ECONNABORTED:
		case ENETDOWN:
#ifdef EPROTO
		case EPROTO:
#endif
		case ENOPROTOOPT:
		case EHOSTDOWN:
#ifdef ENONET
		case ENONET:
#endif
		case EHOSTUNREACH:
		case EOPNOTSUPP:
		case ENETUNREACH:
#endif
			VPRINT(env, (env, DB_VERB_REPMGR_MISC,
			    "accept error %d considered innocuous", ret));
			return (0);
		default:
			__db_err(env, ret, DB_STR("3615", "accept error"));
			return (ret);
		}
	}
	RPRINT(env, (env, DB_VERB_REPMGR_MISC, "accepted a new connection"));

	if ((ret =
	    __repmgr_new_connection(env, &conn, s, CONN_NEGOTIATE)) != 0) {
		(void)closesocket(s);
		return (ret);
	}
	if ((ret = __repmgr_set_keepalive(env, conn)) != 0) {
		(void)__repmgr_destroy_conn(env, conn);
		return (ret);
	}
	if ((ret = __repmgr_set_nonblock_conn(conn)) != 0) {
		__db_err(env, ret, DB_STR("3616",
		    "can't set nonblock after accept"));
		(void)__repmgr_destroy_conn(env, conn);
		return (ret);
	}

	/*
	 * We don't yet know which site this connection is coming from.  So for
	 * now, put it on the "orphans" list; we'll move it to the appropriate
	 * site struct later when we discover who we're talking with, and what
	 * type of connection it is.
	 */
	conn->eid = -1;
	TAILQ_INSERT_TAIL(&db_rep->connections, conn, entries);
	conn->ref_count++;

	return (0);
}

/*
 * Computes how long we should wait for input, in other words how long until we
 * have to wake up and do something.  Returns TRUE if timeout is set; FALSE if
 * there is nothing to wait for.
 *
 * Note that the resulting timeout could be zero; but it can't be negative.
 *
 * PUBLIC: int __repmgr_compute_timeout __P((ENV *, db_timespec *));
 */
int
__repmgr_compute_timeout(env, timeout)
	ENV *env;
	db_timespec *timeout;
{
	DB_REP *db_rep;
	REPMGR_RETRY *retry;
	db_timespec now, t;
	int have_timeout;

	db_rep = env->rep_handle;

	/*
	 * There are four factors to consider: are heartbeats in use? do we
	 * have any sites with broken connections that we ought to retry?
	 * is there a listener process running locally? do we need to call
	 * an election if no master listener exists?
	 */
	have_timeout = __repmgr_next_timeout(env, &t, NULL);

	/* List items are in order, so we only have to examine the first one. */
	if (!TAILQ_EMPTY(&db_rep->retries)) {
		retry = TAILQ_FIRST(&db_rep->retries);
		if (have_timeout)
			/* Choose earliest timeout deadline. */
			t = timespeccmp(&retry->time, &t, <) ? retry->time : t;
		else {
			t = retry->time;
			have_timeout = TRUE;
		}
	}

	/* Check listener every timeout in subordinate rep-aware process. */
	if (IS_LISTENER_CAND(db_rep)) {
		if (!timespecisset(&db_rep->l_listener_chk)) {
			__os_gettime(env, &now, 1);
			TIMESPEC_ADD_DB_TIMEOUT(&now, db_rep->l_listener_wait);
			db_rep->l_listener_chk = now;
		}
		if (have_timeout)
			t = timespeccmp(&db_rep->l_listener_chk, &t, <) ?
			    db_rep->l_listener_chk : t;
		else {
			t = db_rep->l_listener_chk;
			have_timeout = TRUE;
		}
	}

	/* Check master listener if needed. */
	if (FLD_ISSET(db_rep->region->config, REP_C_AUTOTAKEOVER) &&
	    timespecisset(&db_rep->m_listener_chk)) {
		if (have_timeout)
			t = timespeccmp(&db_rep->m_listener_chk, &t, <) ?
			    db_rep->m_listener_chk : t;
		else {
			t = db_rep->m_listener_chk;
			have_timeout = TRUE;
		}
	}

	if (have_timeout) {
		__os_gettime(env, &now, 1);
		if (timespeccmp(&now, &t, >=))
			timespecclear(timeout);
		else {
			*timeout = t;
			timespecsub(timeout, &now);
		}
	}

	return (have_timeout);
}

/*
 * Figures out the next heartbeat-related thing to be done, and when it should
 * be done.  The code is factored this way because this computation needs to be
 * done both before each select() call, and after (when we're checking for timer
 * expiration).
 */
static int
__repmgr_next_timeout(env, deadline, action)
	ENV *env;
	db_timespec *deadline;
	HEARTBEAT_ACTION *action;
{
	DB_REP *db_rep;
	REP *rep;
	HEARTBEAT_ACTION my_action;
	REPMGR_CONNECTION *conn;
	REPMGR_SITE *master;
	db_timespec t;
	u_int32_t version;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	if (rep->master_id == db_rep->self_eid &&
	    rep->heartbeat_frequency > 0) {
		t = db_rep->last_bcast;
		TIMESPEC_ADD_DB_TIMEOUT(&t, rep->heartbeat_frequency);
		my_action = __repmgr_send_heartbeat;
	} else if ((master = __repmgr_connected_master(env)) != NULL &&
	    !IS_SUBORDINATE(db_rep) &&
	    rep->heartbeat_monitor_timeout > 0) {
		version = 0;
		if ((conn = master->ref.conn.in) != NULL &&
		    IS_READY_STATE(conn->state))
			version = conn->version;
		if ((conn = master->ref.conn.out) != NULL &&
		    IS_READY_STATE(conn->state) &&
		    conn->version > version)
			version = conn->version;
		if (version >= HEARTBEAT_MIN_VERSION) {
			/*
			 * If we have a working connection to a heartbeat-aware
			 * master, let's monitor it.  Otherwise there's really
			 * nothing we can do.
			 */
			t = master->last_rcvd_timestamp;
			TIMESPEC_ADD_DB_TIMEOUT(&t,
			    rep->heartbeat_monitor_timeout);
			my_action = __repmgr_call_election;
		} else
			return (FALSE);
	} else
		return (FALSE);

	*deadline = t;
	if (action != NULL)
		*action = my_action;
	return (TRUE);
}

/*
 * Sends a heartbeat message.
 *
 * repmgr also uses the heartbeat facility to manage rerequests.  We
 * send the master's current generation and max_perm_lsn with the heartbeat
 * message to help a client determine whether it has all master transactions.
 * When a client receives a heartbeat message, it also checks whether it
 * needs to rerequest anything.  Note that heartbeats must be enabled for
 * this rerequest processing to occur.
 */
static int
__repmgr_send_heartbeat(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	DBT control, rec;
	__repmgr_permlsn_args permlsn;
	u_int8_t buf[__REPMGR_PERMLSN_SIZE];
	u_int unused1, unused2;
	int ret, unused3;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	permlsn.generation = rep->gen;
	if ((ret = __rep_get_maxpermlsn(env, &permlsn.lsn)) != 0)
		return (ret);
	__repmgr_permlsn_marshal(env, &permlsn, buf);
	control.data = buf;
	control.size = __REPMGR_PERMLSN_SIZE;

	DB_INIT_DBT(rec, NULL, 0);
	return (__repmgr_send_broadcast(env,
	    REPMGR_HEARTBEAT, &control, &rec, &unused1, &unused2, &unused3));
}

/*
 * PUBLIC: REPMGR_SITE *__repmgr_connected_master __P((ENV *));
 */
REPMGR_SITE *
__repmgr_connected_master(env)
	ENV *env;
{
	DB_REP *db_rep;
	REPMGR_SITE *master;
	int master_id;

	db_rep = env->rep_handle;
	master_id = db_rep->region->master_id;

	if (!IS_KNOWN_REMOTE_SITE(master_id))
		return (NULL);
	master = SITE_FROM_EID(master_id);
	if (master->state == SITE_CONNECTED)
		return (master);
	return (NULL);
}

static int
__repmgr_call_election(env)
	ENV *env;
{
	REPMGR_CONNECTION *conn;
	REPMGR_SITE *master;
	int ret;

	master = __repmgr_connected_master(env);
	if (master == NULL)
		return (0);
	RPRINT(env, (env, DB_VERB_REPMGR_MISC,
	    "heartbeat monitor timeout expired"));
	STAT(env->rep_handle->region->mstat.st_connection_drop++);
	if ((conn = master->ref.conn.in) != NULL &&
	    (ret = __repmgr_bust_connection(env, conn)) != 0)
		return (ret);
	if ((conn = master->ref.conn.out) != NULL &&
	    (ret = __repmgr_bust_connection(env, conn)) != 0)
		return (ret);
	return (0);
}

/*
 * PUBLIC: int __repmgr_check_timeouts __P((ENV *));
 *
 * !!!
 * Assumes caller holds the mutex.
 */
int
__repmgr_check_timeouts(env)
	ENV *env;
{
	db_timespec when, now;
	HEARTBEAT_ACTION action;
	int ret;

	/*
	 * Figure out the next heartbeat-related thing to be done.  Then, if
	 * it's time to do it, do so.
	 */
	if (__repmgr_next_timeout(env, &when, &action)) {
		__os_gettime(env, &now, 1);
		if (timespeccmp(&when, &now, <=) &&
		    (ret = (*action)(env)) != 0)
			return (ret);
	}

	/* Check the existence of local listener. */
	if ((ret = __repmgr_check_listener(env)) != 0)
		return (ret);

	/* Check the existence of master listener. */
	if ((ret = __repmgr_check_master_listener(env)) != 0)
		return (ret);

	return (__repmgr_retry_connections(env));
}

/*
 * Check the existence of the listener process on the local site.  If one
 * does not exist and the current process is a subordinate rep-aware process,
 * then start a takeover thread to covert this process to the listener process.
 */
static int
__repmgr_check_listener(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	SITEINFO *sites;
	db_timespec t;
	int ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	ret = 0;

	/*
	 * Only subordinate rep-aware process can take over listener role, so
	 * no need to check listener in listener process or rep unaware process.
	 */
	if (!IS_LISTENER_CAND(db_rep))
		return (0);

	/*
	 * If the listener quits due to site removal, no subordinate process
	 * should take over as listener as the current site is not expected
	 * to be active in the group.  Check the status from the site array
	 * in the shared region instead of that in the GMDB.  We do this
	 * because the GMDB doesn't apply the change yet when replication
	 * is stopped on the removed site.
	 */
	sites = R_ADDR(env->reginfo, rep->siteinfo_off);
	if (sites[rep->self_eid].status == SITE_DELETING)
		return (0);

	/*
	 * Check the listener after timeout.  If there is no listener, we
	 * take over.  During takeover, we will refresh all connections.
	 * A subordinate process does not have an up-to-date site list, so sync
	 * up addresses from the in-memory site array before takeover.
	 */
	__os_gettime(env, &t, 1);
	if (timespeccmp(&t, &db_rep->l_listener_chk, >=)) {
		/* Compute the next timeout. */
		TIMESPEC_ADD_DB_TIMEOUT(&t, db_rep->l_listener_wait);
		db_rep->l_listener_chk = t;

		/* Check if site address information needs to be refreshed. */
		if ((rep->siteinfo_seq > db_rep->siteinfo_seq) &&
		    (ret = __repmgr_sync_siteaddr(env)) != 0)
			return (ret);

		if (rep->listener == 0)
			ret = __repmgr_start_takeover(env);
	}
	return (ret);
}

/*
 * Start a thread to take over the listener role in the current subordinate
 * process.
 */
static int
__repmgr_start_takeover(env)
	ENV *env;
{
	DB_REP *db_rep;
	REPMGR_RUNNABLE *th;
	int ret;

	db_rep = env->rep_handle;
	th = db_rep->takeover_thread;
	if (th == NULL) {
		if ((ret = __os_calloc(env, 1, sizeof(REPMGR_RUNNABLE),
		    &th)) != 0)
			return (ret);
		db_rep->takeover_thread = th;
	} else if (th->finished) {
		if ((ret = __repmgr_thread_join(th)) != 0)
			return (ret);
	} else {
		RPRINT(env, (env, DB_VERB_REPMGR_MISC,
		    "takeover thread still running"));
		return (0);
	}
	th->run = __repmgr_takeover_thread;
	if ((ret = __repmgr_thread_start(env, th)) != 0) {
		__os_free(env, th);
		db_rep->takeover_thread = NULL;
	}
	return (ret);
}

/*
 * Take over listener role in the current subordinate process.
 */
static void *
__repmgr_takeover_thread(argsp)
	void *argsp;
{
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	REP *rep;
	REPMGR_RUNNABLE *th;
	int nthreads, ret, save_policy;

	th = argsp;
	env = th->env;
	db_rep = env->rep_handle;
	ip = NULL;
	rep = db_rep->region;
	ret = 0;

	ENV_ENTER_RET(env, ip, ret);
	if (ret != 0)
		goto out;
	RPRINT(env, (env, DB_VERB_REPMGR_MISC, "starting takeover thread"));
	/*
	 * It is likely that there is an old heartbeat ready to expire
	 * immediately upon restarting repmgr, leading to an unnecessary
	 * election.  Reset the expiration countdown here to avoid this.
	 */
	if ((ret = __repmgr_reset_last_rcvd(env)) != 0)
		goto out;
	/*
	 * If nthreads is set to be 0 in the current subordinate process, use
	 * the value in the last listener. The nthreads should be larger than
	 * 0 in listener.
	 */
	nthreads = db_rep->config_nthreads == 0 ? (int)rep->listener_nthreads :
	    db_rep->config_nthreads;
	/*
	 * It is possible that this subordinate process does not have intact
	 * connections to the other sites.  For most ack policies, restarting
	 * repmgr will wait for acks when it commits its transaction to reload
	 * the gmdb.  Temporarily set the ack policy to NONE for the takeover
	 * so that it is not delayed waiting for acks that can never come.  
	 */
	save_policy = rep->perm_policy;
	rep->perm_policy = DB_REPMGR_ACKS_NONE;
	/*
	 * Restart the repmgr as listener.  If DB_REP_IGNORE is returned,
	 * the current process has become listener.  If DB_REP_UNAVAIL is
	 * returned, the site has been removed from the group and no listener
	 * should be started.  For any other error, if the replication is
	 * stopped because of the takeover thread, we will notify the
	 * application.
	 */
	ret = __repmgr_start_int(env, nthreads, F_ISSET(rep, REP_F_MASTER) ?
	    DB_REP_MASTER : DB_REP_CLIENT);
	if (ret == 0 && !IS_SUBORDINATE(db_rep) &&
	    db_rep->repmgr_status == running) {
		STAT(rep->mstat.st_takeovers++);
		RPRINT(env, (env, DB_VERB_REPMGR_MISC,
		    "finished takeover and became listener"));
	} else if (ret != 0 && db_rep->repmgr_status == stopped) {
		RPRINT(env, (env, DB_VERB_REPMGR_MISC,
		    "failed to take over, repmgr was stopped"));
		DB_EVENT(env, DB_EVENT_REP_AUTOTAKEOVER_FAILED, NULL);
	} else {
		/* The current process is not changed to listener. */
		RPRINT(env, (env, DB_VERB_REPMGR_MISC, "failed to take over"));
	}
	rep->perm_policy = save_policy;
	RPRINT(env, (env, DB_VERB_REPMGR_MISC, "takeover thread is exiting"));
	ENV_LEAVE(env, ip);
out:	th->finished = TRUE;
	return (NULL);
}

/*
 * Reset the last_rcvd_timestamp to restart the wait for a heartbeat
 * monitor expiration.
 */
static int
__repmgr_reset_last_rcvd(env)
	ENV *env;
{
	DB_REP *db_rep;
	REPMGR_SITE *master;

	db_rep = env->rep_handle;

	LOCK_MUTEX(db_rep->mutex);
	if ((master = __repmgr_connected_master(env)) != NULL)
		__os_gettime(env, &master->last_rcvd_timestamp, 1);
	UNLOCK_MUTEX(db_rep->mutex);
	return (0);
}

/*
 * Monitor the connection to master listener.  When the master listener is
 * disconnected and some other master process might take over as listener
 * soon, we will delay the election.  After the delay if there is still no
 * connection from master listener, call an election then.
 */
static int
__repmgr_check_master_listener(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	REPMGR_SITE *master;
	db_timespec t;
	u_int32_t flags;
	int ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	ret = 0;

	/*
	 * We only check for a master listener if m_listener_chk is set.
	 * The field is only set when __repmgr_bust_connection() previously
	 * detected the loss of our connection to the master listener.
	 * If rep->master_id is invalid, wait until it is ready to check.
	 */
	if (!FLD_ISSET((db_rep)->region->config, REP_C_AUTOTAKEOVER) ||
	    !timespecisset(&db_rep->m_listener_chk) ||
	    !IS_VALID_EID(rep->master_id))
		return (0);

	__os_gettime(env, &t, 1);
	if (timespeccmp(&t, &db_rep->m_listener_chk, >=)) {
		master = SITE_FROM_EID(db_rep->region->master_id);
		if (master->ref.conn.out == NULL &&
		    master->ref.conn.in == NULL) {
			flags = ELECT_F_EVENT_NOTIFY;
			if (FLD_ISSET(db_rep->region->config, REP_C_ELECTIONS))
				LF_SET(ELECT_F_IMMED | ELECT_F_FAST);
			else
				RPRINT(env, (env, DB_VERB_REPMGR_MISC,
				    "Master failure, but no elections"));

			ret = __repmgr_init_election(env, flags);
		}
		/*
		 * If the delay has expired reset m_listener_chk.  We reset
		 * it whether or not the master listener process comes back
		 * so that we will not continue checking for a master listener
		 * indefinitely.
		 */
		timespecclear(&db_rep->m_listener_chk);
	}
	return (ret);
}

/*
 * Wake up I/O waiting in selector thread, refresh connections to all connected
 * and present sites.
 *
 * PUBLIC: int __repmgr_refresh_selector __P((ENV *));
 */
int
__repmgr_refresh_selector(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	REPMGR_RETRY *retry;
	REPMGR_SITE *site;
	SITEINFO *sites;
	int eid, ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	if ((ret = __repmgr_wake_main_thread(env)) != 0)
		return (ret);

	FOR_EACH_REMOTE_SITE_INDEX(eid) {
		SET_LISTENER_CAND(1, = 0);
		site = SITE_FROM_EID(eid);

		/*
		 * It is possible some sites were left in a paused state
		 * during the switch, so they have to be removed from the
		 * retry list.
		 */
		if (site->state == SITE_PAUSING) {
			retry = site->ref.retry;
			if (retry != NULL) {
				RPRINT(env, (env, DB_VERB_REPMGR_MISC,
				    "Removing site from retry list eid %lu",
				    (u_long)eid));
				TAILQ_REMOVE(&db_rep->retries, retry, entries);
				__os_free(env, retry);
				site->ref.retry = NULL;
			}
		
		}
		/*
		 * Try to connect to any site that is now PRESENT after
		 * rereading the gmdb.
		 */
		if (site->membership == SITE_PRESENT &&
		    (ret = __repmgr_try_one(env, eid, TRUE)) != 0)
			return (ret);
	}
	return (0);
}

/*
 * Initiates connection attempts for any sites on the idle list whose retry
 * times have expired.
 */
static int
__repmgr_retry_connections(env)
	ENV *env;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	REPMGR_RETRY *retry;
	db_timespec now;
	int eid, ret;

	db_rep = env->rep_handle;
	__os_gettime(env, &now, 1);

	while (!TAILQ_EMPTY(&db_rep->retries)) {
		retry = TAILQ_FIRST(&db_rep->retries);
		if (timespeccmp(&retry->time, &now, >=))
			break;	/* since items are in time order */

		TAILQ_REMOVE(&db_rep->retries, retry, entries);

		eid = retry->eid;
		__os_free(env, retry);
		DB_ASSERT(env, IS_VALID_EID(eid));
		site = SITE_FROM_EID(eid);
		site->ref.retry = NULL;
		DB_ASSERT(env, site->state == SITE_PAUSING);

		if (site->membership == SITE_PRESENT) {
			if ((ret = __repmgr_try_one(env, eid, FALSE)) != 0)
				return (ret);
		} else
			site->state = SITE_IDLE;
	}
	return (0);
}

/*
 * PUBLIC: int __repmgr_first_try_connections __P((ENV *));
 *
 * !!!
 * Assumes caller holds the mutex.
 */
int
__repmgr_first_try_connections(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	REPMGR_SITE *site;
	SITEINFO *sites;
	int eid, ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	FOR_EACH_REMOTE_SITE_INDEX(eid) {
		SET_LISTENER_CAND(1, = 0);
		site = SITE_FROM_EID(eid);
		/*
		 * Normally all sites would be IDLE here.  But if a user thread
		 * triggered an auto-start in a subordinate process, our send()
		 * function may have found new sites when it sync'ed site
		 * addresses, and that action causes connection attempts to be
		 * scheduled (resulting in PAUSING state here, or conceivably
		 * even CONNECTING or CONNECTED).
		 */
		if (site->state == SITE_IDLE &&
		    site->membership == SITE_PRESENT &&
		    (ret = __repmgr_try_one(env, eid, FALSE)) != 0)
			return (ret);
	}
	return (0);
}

/*
 * Starts a thread to open a connection to the site at the given EID.  We might
 * have no connection to the site, or an existing connection to be replaced.
 */
static int
__repmgr_try_one(env, eid, refresh)
	ENV *env;
	int eid;
	int refresh;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	REPMGR_RUNNABLE *th;
	int ret;

	db_rep = env->rep_handle;
	DB_ASSERT(env, IS_VALID_EID(eid));
	site = SITE_FROM_EID(eid);
	th = site->connector;
	if (th == NULL) {
		if ((ret = __os_malloc(env, sizeof(REPMGR_RUNNABLE), &th)) != 0)
			return (ret);
		site->connector = th;
	} else if (th->finished) {
		if ((ret = __repmgr_thread_join(th)) != 0)
			return (ret);
	} else {
		RPRINT(env, (env, DB_VERB_REPMGR_MISC,
		  "eid %lu previous connector thread still running; will retry",
		    (u_long)eid));
		return (__repmgr_schedule_connection_attempt(env,
			eid, refresh));
	}

	site->state = SITE_CONNECTING;

	th->run = __repmgr_connector_thread;
	th->args.conn_th.eid = eid;
	/*
	 * The flag CONNECT_F_REFRESH indicates an immediate connection attempt
	 * should be scheduled if the current connection attempt fails.  It is
	 * turned on before the first attempt to refresh the connection but
	 * turned off if the first attempt fails.  In this way, when refreshing
	 * the connection, there will be at most two immediate connection
	 * attempts, after that, retry as usual.
	 */
	th->args.conn_th.flags = refresh ? CONNECT_F_REFRESH : 0;
	if ((ret = __repmgr_thread_start(env, th)) != 0) {
		__os_free(env, th);
		site->connector = NULL;
	}
	return (ret);
}

static void *
__repmgr_connector_thread(argsp)
	void *argsp;
{
	ENV *env;
	DB_THREAD_INFO *ip;
	REPMGR_RUNNABLE *th;
	int ret;

	th = argsp;
	env = th->env;
	ip = NULL;
	ret = 0;

	ENV_ENTER_RET(env, ip, ret);
	if (ret == 0)
		RPRINT(env, (env, DB_VERB_REPMGR_MISC,
		    "starting connector thread, eid %u",
		    th->args.conn_th.eid));
	if (ret != 0 || (ret = __repmgr_connector_main(env, th)) != 0) {
		__db_err(env, ret, DB_STR("3617", "connector thread failed"));
		RPRINT(env, (env,
		    DB_VERB_REPMGR_MISC, "connector thread is exiting"));
		ENV_LEAVE(env, ip);
		(void)__repmgr_thread_failure(env, ret);
	}
	if (ret == 0) {
		RPRINT(env, (env,
		    DB_VERB_REPMGR_MISC, "connector thread is exiting"));
		ENV_LEAVE(env, ip);
	}
	th->finished = TRUE;
	return (NULL);
}

static int
__repmgr_connector_main(env, th)
	ENV *env;
	REPMGR_RUNNABLE *th;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	REPMGR_CONNECTION *conn;
	DB_REPMGR_CONN_ERR info;
	repmgr_netaddr_t netaddr;
	SITE_STRING_BUFFER site_string;
	int err, ret, t_ret;

	db_rep = env->rep_handle;
	ret = 0;

	LOCK_MUTEX(db_rep->mutex);
	DB_ASSERT(env, IS_VALID_EID(th->args.conn_th.eid));
	site = SITE_FROM_EID(th->args.conn_th.eid);
	if (site->state != SITE_CONNECTING && db_rep->repmgr_status == stopped)
		goto unlock;

	/*
	 * Drop the mutex during operations that could block.  During those
	 * times, the site struct could move (if we had to grow the sites
	 * array), but host wouldn't.
	 *
	 * Also, during those times we might receive an incoming connection from
	 * the site, which would change its state.  So, check state each time we
	 * reacquire the mutex, and quit if the state of the world changed while
	 * we were away.
	 */
	netaddr = site->net_addr;
	RPRINT(env, (env, DB_VERB_REPMGR_MISC, "connecting to %s",
		__repmgr_format_site_loc(site, site_string)));
	UNLOCK_MUTEX(db_rep->mutex);

	if ((ret = __repmgr_connect(env, &netaddr, &conn, &err)) == 0) {
		DB_EVENT(env,
		    DB_EVENT_REP_CONNECT_ESTD, &th->args.conn_th.eid);
		LOCK_MUTEX(db_rep->mutex);
		if ((ret = __repmgr_set_nonblock_conn(conn)) != 0) {
			__db_err(env, ret, DB_STR("3618",
			    "set_nonblock in connnect thread"));
			goto cleanup;
		}
		conn->type = REP_CONNECTION;
		site = SITE_FROM_EID(th->args.conn_th.eid);
		if (site->state != SITE_CONNECTING ||
		    db_rep->repmgr_status == stopped)
			goto cleanup;

		conn->eid = th->args.conn_th.eid;
		site = SITE_FROM_EID(th->args.conn_th.eid);
		/*
		 * If there is an existing outgoing connection, disable it and
		 * replace it with a new connection.  The sites for a formerly
		 * subordinate handle that is now taking over might still be
		 * SITE_CONNECTING.  Set to SITE_CONNECTED before disabling
		 * connection so that sites_avail is correctly maintained.
		 */
		site->state = SITE_CONNECTED;
		if (site->ref.conn.out != NULL)
			(void)__repmgr_disable_connection(env,
			    site->ref.conn.out);
		site->ref.conn.out = conn;
		__os_gettime(env, &site->last_rcvd_timestamp, 1);
		ret = __repmgr_wake_main_thread(env);
	} else if (ret == DB_REP_UNAVAIL) {
		/* Retryable error while trying to connect: retry later. */
		info.eid = th->args.conn_th.eid;
		info.error = err;
		DB_EVENT(env, DB_EVENT_REP_CONNECT_TRY_FAILED, &info);
		STAT(db_rep->region->mstat.st_connect_fail++);

		LOCK_MUTEX(db_rep->mutex);
		site = SITE_FROM_EID(th->args.conn_th.eid);
		if (site->state != SITE_CONNECTING ||
		    db_rep->repmgr_status == stopped) {
			ret = 0;
			goto unlock;
		}
		/*
		 * If it fails to create a new outgoing connection to replace
		 * the existing one in the first attempt, schedule another
		 * immediate attempt.  If it is our second attempt, disable
		 * the existing connections and retry as normal.
		 */
		if (site->ref.conn.out != NULL && th->args.conn_th.flags == 0)
			(void)__repmgr_disable_connection(env,
			    site->ref.conn.out);
		ret = __repmgr_schedule_connection_attempt(env,
		    th->args.conn_th.eid,
		    th->args.conn_th.flags == CONNECT_F_REFRESH);
	} else
		goto out;

	if (0) {
cleanup:
		if ((t_ret = __repmgr_destroy_conn(env, conn)) != 0 &&
		    ret == 0)
			ret = t_ret;
	}

unlock:
	UNLOCK_MUTEX(db_rep->mutex);
out:
	return (ret);
}

/*
 * PUBLIC: int __repmgr_send_v1_handshake __P((ENV *,
 * PUBLIC:     REPMGR_CONNECTION *, void *, size_t));
 */
int
__repmgr_send_v1_handshake(env, conn, buf, len)
	ENV *env;
	REPMGR_CONNECTION *conn;
	void *buf;
	size_t len;
{
	DB_REP *db_rep;
	REP *rep;
	repmgr_netaddr_t *my_addr;
	DB_REPMGR_V1_HANDSHAKE buffer;
	DBT cntrl, rec;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	my_addr = &SITE_FROM_EID(db_rep->self_eid)->net_addr;

	/*
	 * We're about to send from a structure that has padding holes in it.
	 * Initializing it keeps Valgrind happy, plus we really shouldn't be
	 * sending out random garbage anyway (pro forma privacy issue).
	 */
	memset(&buffer, 0, sizeof(buffer));
	buffer.version = 1;
	buffer.priority = htonl(rep->priority);
	buffer.port = my_addr->port;
	cntrl.data = &buffer;
	cntrl.size = sizeof(buffer);

	rec.data = buf;
	rec.size = (u_int32_t)len;

	/*
	 * It would of course be disastrous to block the select() thread, so
	 * pass the "maxblock" argument as 0.  Fortunately blocking should
	 * never be necessary here, because the hand-shake is always the first
	 * thing we send.  Which is a good thing, because it would be almost as
	 * disastrous if we allowed ourselves to drop a handshake.
	 */
	return (__repmgr_send_one(env,
	    conn, REPMGR_HANDSHAKE, &cntrl, &rec, 0));
}

/*
 * PUBLIC: int __repmgr_read_from_site __P((ENV *, REPMGR_CONNECTION *));
 *
 * !!!
 * Caller is assumed to hold repmgr->mutex, 'cuz we call queue_put() from here.
 */
int
__repmgr_read_from_site(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	int ret;

	db_rep = env->rep_handle;

	/*
	 * Loop, just in case we get EINTR and need to restart the I/O.  (All
	 * other branches return.)
	 */
	for (;;) {
		switch ((ret = __repmgr_read_conn(conn))) {
#ifndef DB_WIN32
		case EINTR:
			continue;
#endif

#if defined(DB_REPMGR_EAGAIN) && DB_REPMGR_EAGAIN != WOULDBLOCK
		case DB_REPMGR_EAGAIN:
#endif
		case WOULDBLOCK:
			return (0);

		case DB_REP_UNAVAIL:
			/* Error 0 is understood to mean EOF. */
			__repmgr_fire_conn_err_event(env, conn, 0);
			STAT(env->rep_handle->
			    region->mstat.st_connection_drop++);
			return (DB_REP_UNAVAIL);

		case 0:
			if (IS_VALID_EID(conn->eid)) {
				site = SITE_FROM_EID(conn->eid);
				__os_gettime(env,
				    &site->last_rcvd_timestamp, 1);
			}
			return (conn->reading_phase == SIZES_PHASE ?
			    prepare_input(env, conn) :
			    dispatch_msgin(env, conn));

		default:
#ifdef EBADF
			DB_ASSERT(env, ret != EBADF);
#endif
			__repmgr_fire_conn_err_event(env, conn, ret);
			STAT(db_rep->region->mstat.st_connection_drop++);
			return (DB_REP_UNAVAIL);
		}
	}
}

/*
 * Reads in the current input phase, as defined by the connection's IOVECS
 * struct.
 *
 * Returns DB_REP_UNAVAIL for EOF.
 *
 * Makes no assumption about synchronization: it's up to the caller to hold
 * mutex if necessary.
 *
 * PUBLIC: int __repmgr_read_conn __P((REPMGR_CONNECTION *));
 */
int
__repmgr_read_conn(conn)
	REPMGR_CONNECTION *conn;
{
	size_t nr;
	int ret;

	/*
	 * Keep reading pieces as long as we're making some progress, or until
	 * we complete the current read phase as defined in iovecs.
	 */
	for (;;) {
		if ((ret = __repmgr_readv(conn->fd,
		    &conn->iovecs.vectors[conn->iovecs.offset],
		    conn->iovecs.count - conn->iovecs.offset, &nr)) != 0)
			return (ret);

		if (nr == 0)
			return (DB_REP_UNAVAIL);

		if (__repmgr_update_consumed(&conn->iovecs, nr)) {
			/* We've fully read as much as we wanted. */
			return (0);
		}
	}
}

/*
 * Having finished reading the 9-byte message header, figure out what kind of
 * message we're about to receive, and prepare input buffers accordingly.  The
 * header includes enough information for us to figure out how much buffer space
 * we need to allocate (though in some cases we need to do a bit of computation
 * to arrive at the answer).
 *
 * Caller must hold mutex.
 */
static int
prepare_input(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
#define	MEM_ALIGN sizeof(double)
	DBT *dbt;
	__repmgr_msg_hdr_args msg_hdr;
	REPMGR_RESPONSE *resp;
	u_int32_t control_size, rec_size, size;
	size_t memsize, control_offset, rec_offset;
	void *membase;
	int ret, skip;

	DB_ASSERT(env, conn->reading_phase == SIZES_PHASE);

	/*
	 * We can only get here after having read the full 9 bytes that we
	 * expect, so this can't fail.
	 */
	ret = __repmgr_msg_hdr_unmarshal(env, &msg_hdr,
	    conn->msg_hdr_buf, __REPMGR_MSG_HDR_SIZE, NULL);
	DB_ASSERT(env, ret == 0);

	__repmgr_iovec_init(&conn->iovecs);
	skip = FALSE;

	switch ((conn->msg_type = msg_hdr.type)) {
	case REPMGR_HEARTBEAT:
		/*
		 * The underlying byte-receiving mechanism will already have
		 * noted the fact that we got some traffic on this connection,
		 * which is all that is needed to monitor the heartbeat.  But
		 * we also put the heartbeat message on the message queue so
		 * that it will perform rerequest processing.
		 */
	case REPMGR_REP_MESSAGE:
		env->rep_handle->seen_repmsg = TRUE;
		control_size = REP_MSG_CONTROL_SIZE(msg_hdr);
		rec_size = REP_MSG_REC_SIZE(msg_hdr);
		if (control_size == 0) {
			if (conn->msg_type == REPMGR_HEARTBEAT) {
				/*
				 * Got an old-style heartbeat without payload,
				 * nothing to do.
				 */
				skip = TRUE;
				break;
			} else {
				__db_errx(env, DB_STR("3619",
				    "illegal size for rep msg"));
				return (DB_REP_UNAVAIL);
			}
		}
		/*
		 * Allocate a block of memory large enough to hold a
		 * DB_REPMGR_MESSAGE wrapper, plus the (one or) two DBT
		 * data areas that it points to.  Start by calculating
		 * the total memory needed.
		 */
		memsize = DB_ALIGN(sizeof(REPMGR_MESSAGE), MEM_ALIGN);
		control_offset = memsize;
		memsize += control_size;
		if (rec_size > 0) {
			memsize = DB_ALIGN(memsize, MEM_ALIGN);
			rec_offset = memsize;
			memsize += rec_size;
		} else
			COMPQUIET(rec_offset, 0);
		if ((ret = __os_malloc(env, memsize, &membase)) != 0)
			return (ret);
		conn->input.rep_message = membase;
		conn->input.rep_message->msg_hdr = msg_hdr;
		conn->input.rep_message->v.repmsg.originating_eid = conn->eid;

		DB_INIT_DBT(conn->input.rep_message->v.repmsg.control,
		    (u_int8_t*)membase + control_offset, control_size);
		__repmgr_add_dbt(&conn->iovecs,
		    &conn->input.rep_message->v.repmsg.control);

		if (rec_size > 0) {
			DB_INIT_DBT(conn->input.rep_message->v.repmsg.rec,
			    (rec_size > 0 ?
				(u_int8_t*)membase + rec_offset : NULL),
			    rec_size);
			__repmgr_add_dbt(&conn->iovecs,
			    &conn->input.rep_message->v.repmsg.rec);
		} else
			DB_INIT_DBT(conn->input.rep_message->v.repmsg.rec,
			    NULL, 0);
		break;

	case REPMGR_APP_MESSAGE:
		/*
		 * We need a buffer big enough to hold the REPMGR_MESSAGE struct
		 * and the data that we expect to receive on the wire.  We must
		 * extend the struct size for the variable-length DBT array at
		 * the end.
		 */
		size = DB_ALIGN((size_t)(sizeof(REPMGR_MESSAGE) +
		    APP_MSG_SEGMENT_COUNT(msg_hdr) * sizeof(DBT)),
		    MEM_ALIGN);
		memsize = size + APP_MSG_BUFFER_SIZE(msg_hdr);
		if ((ret = __os_malloc(env, memsize, &membase)) != 0)
			return (ret);
		conn->input.rep_message = membase;
		conn->input.rep_message->msg_hdr = msg_hdr;
		conn->input.rep_message->v.appmsg.conn = conn;

		DB_INIT_DBT(conn->input.rep_message->v.appmsg.buf,
		    (u_int8_t*)membase + size,
		    APP_MSG_BUFFER_SIZE(msg_hdr));
		__repmgr_add_dbt(&conn->iovecs,
		    &conn->input.rep_message->v.appmsg.buf);
		break;

	case REPMGR_OWN_MSG:
		size = sizeof(REPMGR_MESSAGE) + REPMGR_OWN_BUF_SIZE(msg_hdr);
		if ((ret = __os_malloc(env, size, &membase)) != 0)
			return (ret);
		conn->input.rep_message = membase;
		conn->input.rep_message->msg_hdr = msg_hdr;

		/*
		 * Save "conn" pointer in case this turns out to be a one-shot
		 * request.  If it isn't, it won't matter.
		 */
		/*
		 * An OWN msg that arrives in PARAMETERS state has bypassed the
		 * final handshake, implying that this connection is to be used
		 * for a one-shot GMDB request.
		 */
		if (REPMGR_OWN_BUF_SIZE(msg_hdr) == 0) {
			__db_errx(env, DB_STR_A("3680",
			    "invalid own buf size %lu in prepare_input", "%lu"),
			    (u_long)REPMGR_OWN_BUF_SIZE(msg_hdr));
			return (DB_REP_UNAVAIL);
		}
		DB_INIT_DBT(conn->input.rep_message->v.gmdb_msg.request,
		    (u_int8_t*)membase + sizeof(REPMGR_MESSAGE),
		    REPMGR_OWN_BUF_SIZE(msg_hdr));
		__repmgr_add_dbt(&conn->iovecs,
		    &conn->input.rep_message->v.gmdb_msg.request);
		break;

	case REPMGR_APP_RESPONSE:
		size = APP_RESP_BUFFER_SIZE(msg_hdr);
		conn->cur_resp = APP_RESP_TAG(msg_hdr);
		if (conn->cur_resp >= conn->aresp) {
			__db_errx(env, DB_STR_A("3681",
			    "invalid cur resp %lu in prepare_input", "%lu"),
			    (u_long)conn->cur_resp);
			return (DB_REP_UNAVAIL);
		}
		resp = &conn->responses[conn->cur_resp];
		DB_ASSERT(env, F_ISSET(resp, RESP_IN_USE));

		dbt = &resp->dbt;

		/*
		 * Prepare to read message body into either the user-supplied
		 * buffer, or one we allocate here.
		 */
		ret = 0;
		if (!F_ISSET(resp, RESP_THREAD_WAITING)) {
			/* Caller already timed out; allocate dummy buffer. */
			if (size > 0) {
				memset(dbt, 0, sizeof(*dbt));
				ret = __os_malloc(env, size, &dbt->data);
				F_SET(resp, RESP_DUMMY_BUF);
			} else
				F_CLR(resp, RESP_IN_USE);
		} else if (F_ISSET(dbt, DB_DBT_MALLOC))
			ret = __os_umalloc(env, size, &dbt->data);
		else if (F_ISSET(dbt, DB_DBT_REALLOC)) {
			if (dbt->data == NULL || dbt->size < size)
				ret = __os_urealloc(env, size, &dbt->data);
		} else if (F_ISSET(dbt, DB_DBT_USERMEM)) {
			/* Recipient should have checked size limit. */
			DB_ASSERT(env, size <= dbt->ulen);
		}
		dbt->size = size;
		if (ret != 0)
			return (ret);

		if (size > 0) {
			__repmgr_add_dbt(&conn->iovecs, dbt);
			F_SET(resp, RESP_READING);
		} else {
			skip = TRUE;
			if (F_ISSET(resp, RESP_THREAD_WAITING)) {
				F_SET(resp, RESP_COMPLETE);
				if ((ret = __repmgr_wake_waiters(env,
				    &conn->response_waiters)) != 0)
					return (ret);
			}
		}
		break;

	case REPMGR_RESP_ERROR:
		DB_ASSERT(env, RESP_ERROR_TAG(msg_hdr) < conn->aresp &&
		    conn->responses != NULL);
		resp = &conn->responses[RESP_ERROR_TAG(msg_hdr)];
		DB_ASSERT(env, !F_ISSET(resp, RESP_READING));
		if (F_ISSET(resp, RESP_THREAD_WAITING)) {
			F_SET(resp, RESP_COMPLETE);

			/*
			 * DB errors are always negative, but we only send
			 * unsigned values on the wire.
			 */
			resp->ret = -((int)RESP_ERROR_CODE(msg_hdr));
			if ((ret = __repmgr_wake_waiters(env,
			    &conn->response_waiters)) != 0)
				return (ret);
		} else
			F_CLR(resp, RESP_IN_USE);
		skip = TRUE;
		break;

	case REPMGR_HANDSHAKE:
	case REPMGR_PERMLSN:
		if ((ret = __repmgr_prepare_simple_input(env,
		    conn, &msg_hdr)) != 0)
			return (ret);
		break;

	default:
		__db_errx(env, DB_STR_A("3676",
		    "unexpected msg type %lu in prepare_input", "%lu"),
		    (u_long)conn->msg_type);
		return (DB_REP_UNAVAIL);
	}

	if (skip) {
		/*
		 * We can skip the DATA_PHASE, because the current message type
		 * only has a header, no following data.
		 */
		__repmgr_reset_for_reading(conn);
	} else
		conn->reading_phase = DATA_PHASE;

	return (0);
}

/*
 * PUBLIC: int __repmgr_prepare_simple_input __P((ENV *,
 * PUBLIC:     REPMGR_CONNECTION *, __repmgr_msg_hdr_args *));
 */
int
__repmgr_prepare_simple_input(env, conn, msg_hdr)
	ENV *env;
	REPMGR_CONNECTION *conn;
	__repmgr_msg_hdr_args *msg_hdr;
{
	DBT *dbt;
	u_int32_t control_size, rec_size;
	int ret;

	control_size = REP_MSG_CONTROL_SIZE(*msg_hdr);
	rec_size = REP_MSG_REC_SIZE(*msg_hdr);

	dbt = &conn->input.repmgr_msg.cntrl;
	if ((dbt->size = control_size) > 0) {
		if ((ret = __os_malloc(env,
		    dbt->size, &dbt->data)) != 0)
			return (ret);
		__repmgr_add_dbt(&conn->iovecs, dbt);
	}

	dbt = &conn->input.repmgr_msg.rec;
	if ((dbt->size = rec_size) > 0) {
		if ((ret = __os_malloc(env,
		    dbt->size, &dbt->data)) != 0) {
			dbt = &conn->input.repmgr_msg.cntrl;
			if (dbt->size > 0)
				__os_free(env, dbt->data);
			return (ret);
		}
		__repmgr_add_dbt(&conn->iovecs, dbt);
	}
	return (0);
}

/*
 * Processes an incoming message, depending on our current state.
 *
 * Caller must hold mutex.
 */
static int
dispatch_msgin(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DBT *dbt;
	DB_REP *db_rep;
	REP *rep;
	REPMGR_RESPONSE *resp;
	REPMGR_RUNNABLE *th;
	REPMGR_SITE *site;
	char *hostname;
	int eid, ret, subord;

	DB_ASSERT(env, conn->reading_phase == DATA_PHASE);
	db_rep = env->rep_handle;
	rep = db_rep->region;

	switch (conn->state) {
	case CONN_CONNECTED:
		/*
		 * In this state, we know we're working with an outgoing
		 * connection.  We've sent a version proposal, and now expect
		 * the response (which could be a dumb old V1 handshake).
		 */
		ONLY_HANDSHAKE(env, conn);

		/*
		 * Here is a good opportunity to clean up this site's connector
		 * thread, because we generally come through here after making
		 * an outgoing connection, yet we're out of the main loop, so we
		 * don't hit this often.
		 */
		eid = conn->eid;
		DB_ASSERT(env, IS_KNOWN_REMOTE_SITE(conn->eid));
		site = SITE_FROM_EID(eid);
		th = site->connector;
		if (th != NULL && th->finished) {
			if ((ret = __repmgr_thread_join(th)) != 0)
				return (ret);
			__os_free(env, th);
			site->connector = NULL;
		}

		if ((ret = read_version_response(env, conn)) != 0)
			return (ret);
		break;

	case CONN_NEGOTIATE:
		/*
		 * Since we're in this state, we know we're working with an
		 * incoming connection, and this is the first message we've
		 * received.  So it must be a version negotiation proposal (or a
		 * legacy V1 handshake).  (We'll verify this of course.)
		 */
		ONLY_HANDSHAKE(env, conn);
		if ((ret = send_version_response(env, conn)) != 0)
			return (ret);
		break;

	case CONN_PARAMETERS:
		/*
		 * We've previously agreed on a (>1) version, so we expect
		 * either the other side's parameters handshake, or possibly a
		 * GMDB request on a one-shot, dedicated connection.
		 */
		switch (conn->msg_type) {
		case REPMGR_HANDSHAKE:
			dbt = &conn->input.repmgr_msg.rec;
			hostname = dbt->data;
			hostname[dbt->size-1] = '\0';
			if ((ret = accept_handshake(env,
			    conn, hostname, &subord)) != 0)
				return (ret);
			conn->state = CONN_READY;
			site = SITE_FROM_EID(conn->eid);
			/*
			 * Do not increase sites_avail redundantly for an
			 * incoming subordinate connection.
			 */
			if (conn->type == REP_CONNECTION &&
			    site->state == SITE_CONNECTED && !subord) {
				rep->sites_avail++;
				RPRINT(env, (env, DB_VERB_REPMGR_MISC,
    "msgin: EID %lu CONNECTED, READY.  sites_avail %lu",
    (u_long)conn->eid, (u_long)rep->sites_avail));
			}
			break;
		case REPMGR_OWN_MSG:
			/*
			 * GM change requests arrive in their own dedicated
			 * connections, and when they're served the entire
			 * connection isn't needed any more.  So the message
			 * processing thread will do the entire job of serving
			 * the request and finishing off the connection; so we
			 * don't have to read it any more.  Note that normally
			 * whenever we remove a connection from our list we
			 * decrement the reference count; but we also increment
			 * it whenever we pass a reference over to the message
			 * processing threads' queue.  So in this case it's a
			 * wash.
			 */
			conn->input.rep_message->v.gmdb_msg.conn = conn;
			TAILQ_REMOVE(&db_rep->connections, conn, entries);
			if ((ret = __repmgr_queue_put(env,
			    conn->input.rep_message)) != 0)
				return (ret);
			break;

		default:
			__db_errx(env, DB_STR_A("3620",
			    "unexpected msg type %d in PARAMETERS state", "%d"),
			    (int)conn->msg_type);
			return (DB_REP_UNAVAIL);
		}

		break;

	case CONN_READY:
	case CONN_CONGESTED:
		/*
		 * We have a complete message, so process it.  Acks and
		 * handshakes get processed here, in line.  Regular rep messages
		 * get posted to a queue, to be handled by a thread from the
		 * message thread pool.
		 */
		switch (conn->msg_type) {
		case REPMGR_PERMLSN:
			if ((ret = record_permlsn(env, conn)) != 0)
				return (ret);
			break;

		case REPMGR_HEARTBEAT:
		case REPMGR_APP_MESSAGE:
		case REPMGR_REP_MESSAGE:
			if ((ret = __repmgr_queue_put(env,
			    conn->input.rep_message)) != 0)
				return (ret);
			/*
			 * The queue has taken over responsibility for the
			 * rep_message buffer, and will free it later.
			 */
			if (conn->msg_type == REPMGR_APP_MESSAGE)
				conn->ref_count++;
			break;

		case REPMGR_OWN_MSG:
			/*
			 * Since we're in one of the "ready" states we know this
			 * isn't a one-shot request, so we are not giving
			 * ownership of this connection over to the message
			 * thread queue; we're going to keep reading on it
			 * ourselves.  The message thread that processes this
			 * request has no need for a connection anyway, since
			 * there is no response that needs to be returned.
			 */
			conn->input.rep_message->v.gmdb_msg.conn = NULL;
			if ((ret = process_own_msg(env, conn)) != 0)
				return (ret);
			break;

		case REPMGR_APP_RESPONSE:
			DB_ASSERT(env, conn->cur_resp < conn->aresp &&
			    conn->responses != NULL);
			resp = &conn->responses[conn->cur_resp];
			DB_ASSERT(env, F_ISSET(resp, RESP_READING));
			F_CLR(resp, RESP_READING);
			if (F_ISSET(resp, RESP_THREAD_WAITING)) {
				F_SET(resp, RESP_COMPLETE);
				if ((ret = __repmgr_wake_waiters(env,
				    &conn->response_waiters)) != 0)
					return (ret);
			} else {
				/*
				 * If the calling thread is no longer with us,
				 * yet we're reading, it can only mean we're
				 * reading into a dummy buffer, so free it now.
				 */
				DB_ASSERT(env, F_ISSET(resp, RESP_DUMMY_BUF));
				__os_free(env, resp->dbt.data);
				F_CLR(resp, RESP_IN_USE);
			}
			break;

		case REPMGR_RESP_ERROR:
		default:
			__db_errx(env, DB_STR_A("3621",
			    "unexpected msg type rcvd in ready state: %d",
			    "%d"), (int)conn->msg_type);
			return (DB_REP_UNAVAIL);
		}
		break;

	case CONN_DEFUNCT:
		break;

	default:
		DB_ASSERT(env, FALSE);
	}

	switch (conn->msg_type) {
	case REPMGR_HANDSHAKE:
	case REPMGR_PERMLSN:
		dbt = &conn->input.repmgr_msg.cntrl;
		if (dbt->size > 0)
			__os_free(env, dbt->data);
		dbt = &conn->input.repmgr_msg.rec;
		if (dbt->size > 0)
			__os_free(env, dbt->data);
		break;
	default:
		/*
		 * Some messages in REPMGR_OWN_MSG format are also handled
		 */
		break;
	}
	__repmgr_reset_for_reading(conn);
	return (0);
}

/*
 * Process one of repmgr's "own" message types, and one that occurs on a regular
 * (not one-shot) connection.
 */
static int
process_own_msg(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	DBT *dbt;
	REPMGR_SITE *site;
	REPMGR_MESSAGE *msg;
	__repmgr_connect_reject_args reject;
	__repmgr_v4connect_reject_args v4reject;
	__repmgr_parm_refresh_args parms;
	int ret;

	db_rep = env->rep_handle;
	ret = 0;
	/*
	 * Set "msg" to point to the message struct.  If we do all necessary
	 * processing here now, leave it set so that it can be freed.  On the
	 * other hand, if we pass it off to the message queue for later
	 * processing by a message thread, we want to avoid freeing the memory
	 * here, so clear the pointer in such a case.
	 */
	switch (REPMGR_OWN_MSG_TYPE((msg = conn->input.rep_message)->msg_hdr)) {
	case REPMGR_CONNECT_REJECT:
		dbt = &msg->v.gmdb_msg.request;
		if (conn->version < 5) {
			if ((ret = __repmgr_v4connect_reject_unmarshal(env,
			    &v4reject, dbt->data, dbt->size, NULL)) != 0)
				return (DB_REP_UNAVAIL);
			reject.version = v4reject.version;
			reject.gen = v4reject.gen;
			reject.status = 0;
		} else {
			if ((ret = __repmgr_connect_reject_unmarshal(env,
			    &reject, dbt->data, dbt->size, NULL)) != 0)
				return (DB_REP_UNAVAIL);
		}

		/*
		 * If we're being rejected by someone who has more up-to-date
		 * membership information than we do, it means we are not in
		 * the group.  If we've just gotten started, or our status is
		 * adding, we can make one attempt at automatically rejoining;
		 * otherwise we bow out gracefully.
		 */
		RPRINT(env, (env, DB_VERB_REPMGR_MISC,
	"got rejection msg citing version %lu/%lu mine %lu/%lu membership %lu",
			(u_long)reject.gen, (u_long)reject.version,
			(u_long)db_rep->member_version_gen,
			(u_long)db_rep->membership_version,
			(u_long)reject.status));

		if (__repmgr_gmdb_version_cmp(env,
		    reject.gen, reject.version) > 0) {
			if (db_rep->seen_repmsg && reject.status != SITE_ADDING)
				ret = DB_DELETED;
			else {
				/*
				 * If 2SITE_STRICT is off, we are likely to
				 * win an election with our own vote before 
				 * discovering there is already a master.
				 * Set indicator to defer the election until
				 * after rejoining group.
				 */
				if (!FLD_ISSET(db_rep->region->config,
				    REP_C_2SITE_STRICT))
					db_rep->rejoin_pending = TRUE;
				if ((ret = __repmgr_defer_op(env,
				    REPMGR_REJOIN)) == 0)
					ret = DB_REP_UNAVAIL;
			}
		} else
			ret = DB_REP_UNAVAIL;
		DB_ASSERT(env, ret != 0);
		return (ret);

	case REPMGR_SHARING:
		if ((ret = __repmgr_queue_put(env, msg)) != 0)
			return (ret);
		/* Show that we no longer own this memory. */
		msg = NULL;
		break;

	case REPMGR_PARM_REFRESH:
		dbt = &conn->input.rep_message->v.gmdb_msg.request;
		if ((ret = __repmgr_parm_refresh_unmarshal(env,
		    &parms, dbt->data, dbt->size, NULL)) != 0)
			return (DB_REP_UNAVAIL);
		DB_ASSERT(env, conn->type == REP_CONNECTION &&
		    IS_KNOWN_REMOTE_SITE(conn->eid));
		site = SITE_FROM_EID(conn->eid);
		site->ack_policy = (int)parms.ack_policy;
		if (F_ISSET(&parms, ELECTABLE_SITE))
			F_SET(site, SITE_ELECTABLE);
		else
			F_CLR(site, SITE_ELECTABLE);
		F_SET(site, SITE_HAS_PRIO);
		break;

	case REPMGR_GM_FAILURE:
	case REPMGR_GM_FORWARD:
	case REPMGR_JOIN_REQUEST:
	case REPMGR_JOIN_SUCCESS:
	case REPMGR_REMOVE_REQUEST:
	case REPMGR_RESOLVE_LIMBO:
	default:
		__db_errx(env, DB_STR_A("3677",
		    "unexpected msg type %lu in process_own_msg", "%lu"),
		    (u_long)REPMGR_OWN_MSG_TYPE(msg->msg_hdr));
		return (DB_REP_UNAVAIL);
	}
	/*
	 * If we haven't given ownership of the msg buffer to another thread,
	 * free it now.
	 */
	if (msg != NULL)
		__os_free(env, msg);
	return (ret);
}

/*
 * Examine and verify the incoming version proposal message, and send an
 * appropriate response.
 */
static int
send_version_response(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	__repmgr_version_proposal_args versions;
	__repmgr_version_confirmation_args conf;
	repmgr_netaddr_t *my_addr;
	char *hostname;
	u_int8_t buf[__REPMGR_VERSION_CONFIRMATION_SIZE+1];
	DBT vi;
	int ret;

	db_rep = env->rep_handle;
	my_addr = &SITE_FROM_EID(db_rep->self_eid)->net_addr;

	if ((ret = __repmgr_find_version_info(env, conn, &vi)) != 0)
		return (ret);
	if (vi.size == 0) {
		/* No version info, so we must be talking to a v1 site. */
		hostname = conn->input.repmgr_msg.rec.data;
		if ((ret = accept_v1_handshake(env, conn, hostname)) != 0)
			return (ret);
		if ((ret = __repmgr_send_v1_handshake(env,
		    conn, my_addr->host, strlen(my_addr->host) + 1)) != 0)
			return (ret);
		conn->state = CONN_READY;
	} else {
		if ((ret = __repmgr_version_proposal_unmarshal(env,
		    &versions, vi.data, vi.size, NULL)) != 0)
			return (DB_REP_UNAVAIL);

		if (DB_REPMGR_VERSION >= versions.min &&
		    DB_REPMGR_VERSION <= versions.max)
			conf.version = DB_REPMGR_VERSION;
		else if (versions.max >= DB_REPMGR_MIN_VERSION &&
		    versions.max <= DB_REPMGR_VERSION)
			conf.version = versions.max;
		else {
			/*
			 * User must have wired up a combination of versions
			 * exceeding what we said we'd support.
			 */
			__db_errx(env, DB_STR_A("3622",
			    "No available version between %lu and %lu",
			    "%lu %lu"), (u_long)versions.min,
			    (u_long)versions.max);
			return (DB_REP_UNAVAIL);
		}
		conn->version = conf.version;

		__repmgr_version_confirmation_marshal(env, &conf, buf);
		buf[__REPMGR_VERSION_CONFIRMATION_SIZE] = '\0';
		DB_ASSERT(env, !IS_SUBORDINATE(db_rep));
		if ((ret = __repmgr_send_handshake(env,
		     conn, buf, sizeof(buf), 0)) != 0)
			return (ret);

		conn->state = CONN_PARAMETERS;
	}
	return (ret);
}

/*
 * Sends a version-aware handshake to the remote site, only after we've verified
 * that it is indeed version-aware.  We can send either v2 or v3 handshake,
 * depending on the connection's version.
 *
 * PUBLIC: int __repmgr_send_handshake __P((ENV *,
 * PUBLIC:     REPMGR_CONNECTION *, void *, size_t, u_int32_t));
 */
int
__repmgr_send_handshake(env, conn, opt, optlen, flags)
	ENV *env;
	REPMGR_CONNECTION *conn;
	void *opt;
	size_t optlen;
	u_int32_t flags;
{
	DB_REP *db_rep;
	REP *rep;
	DBT cntrl, rec;
	__repmgr_handshake_args hs;
	__repmgr_v2handshake_args v2hs;
	__repmgr_v3handshake_args v3hs;
	repmgr_netaddr_t *my_addr;
	size_t hostname_len, rec_len;
	void *buf;
	u_int8_t *p;
	u_int32_t cntrl_len;
	int ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	my_addr = &SITE_FROM_EID(db_rep->self_eid)->net_addr;

	/*
	 * The cntrl part has various parameters (varies by version).  The rec
	 * part has the host name, followed by whatever optional extra data was
	 * passed to us.
	 *
	 * Version awareness was introduced with protocol version 2 (so version
	 * 1 is handled elsewhere).
	 */
	switch (conn->version) {
	case 2:
		cntrl_len = __REPMGR_V2HANDSHAKE_SIZE;
		break;
	case 3:
		cntrl_len = __REPMGR_V3HANDSHAKE_SIZE;
		break;
	case 4:
	case 5:
		cntrl_len = __REPMGR_HANDSHAKE_SIZE;
		break;
	default:
		__db_errx(env, DB_STR_A("3678",
		    "unexpected conn version %lu in send_handshake", "%lu"),
		    (u_long)conn->version);
		return (DB_REP_UNAVAIL);
	}
	hostname_len = strlen(my_addr->host);
	rec_len = hostname_len + 1 +
	    (opt == NULL ? 0 : optlen);

	if ((ret = __os_malloc(env, cntrl_len + rec_len, &buf)) != 0)
		return (ret);

	cntrl.data = p = buf;
	switch (conn->version) {
	case 2:
		/* Not allowed to use multi-process feature in v2 group. */
		DB_ASSERT(env, !IS_SUBORDINATE(db_rep));
		v2hs.port = my_addr->port;
		v2hs.priority = rep->priority;
		__repmgr_v2handshake_marshal(env, &v2hs, p);
		break;
	case 3:
		v3hs.port = my_addr->port;
		v3hs.priority = rep->priority;
		v3hs.flags = flags;
		__repmgr_v3handshake_marshal(env, &v3hs, p);
		break;
	case 4:
	case 5:
		hs.port = my_addr->port;
		hs.alignment = MEM_ALIGN;
		hs.ack_policy = (u_int32_t)rep->perm_policy;
		hs.flags = flags;
		if (rep->priority > 0)
			F_SET(&hs, ELECTABLE_SITE);
		__repmgr_handshake_marshal(env, &hs, p);
		break;
	default:
		DB_ASSERT(env, FALSE);
		break;
	}
	cntrl.size = cntrl_len;

	p = rec.data = &p[cntrl_len];
	(void)strcpy((char*)p, my_addr->host);
	p += hostname_len + 1;
	if (opt != NULL) {
		memcpy(p, opt, optlen);
		p += optlen;
	}
	rec.size = (u_int32_t)(p - (u_int8_t*)rec.data);

	/* Never block on select thread: pass maxblock as 0. */
	ret = __repmgr_send_one(env,
	    conn, REPMGR_HANDSHAKE, &cntrl, &rec, 0);
	__os_free(env, buf);
	return (ret);
}

static int
read_version_response(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	__repmgr_version_confirmation_args conf;
	DBT vi;
	REP *rep;
	REPMGR_SITE *site;
	char *hostname;
	u_int32_t flags;
	int ret, subord;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	if ((ret = __repmgr_find_version_info(env, conn, &vi)) != 0)
		return (ret);
	hostname = conn->input.repmgr_msg.rec.data;
	if (vi.size == 0) {
		if ((ret = accept_v1_handshake(env, conn, hostname)) != 0)
			return (ret);
	} else {
		if ((ret = __repmgr_version_confirmation_unmarshal(env,
		    &conf, vi.data, vi.size, NULL)) != 0)
			return (DB_REP_UNAVAIL);
		if (conf.version >= DB_REPMGR_MIN_VERSION &&
		    conf.version <= DB_REPMGR_VERSION)
			conn->version = conf.version;
		else {
			/*
			 * Remote site "confirmed" a version outside of the
			 * range we proposed.  It should never do that.
			 */
			__db_errx(env, DB_STR_A("3623",
			    "Can't support confirmed version %lu", "%lu"),
			    (u_long)conf.version);
			return (DB_REP_UNAVAIL);
		}

		if ((ret = accept_handshake(env, conn, hostname, &subord)) != 0)
			return (ret);
		if (!IS_SUBORDINATE(db_rep))
			flags = 0;
		else {
			flags = REPMGR_SUBORDINATE;
			if (FLD_ISSET(rep->config, REP_C_AUTOTAKEOVER) &&
			    db_rep->repmgr_status == running)
				/*
				 * Takeover is enabled in rep-aware subordinate
				 * process.
				 */
				flags |= REPMGR_AUTOTAKEOVER;
		}
		if ((ret = __repmgr_send_handshake(env,
		    conn, NULL, 0, flags)) != 0)
			return (ret);
	}
	conn->state = CONN_READY;
	site = SITE_FROM_EID(conn->eid);
	/*
	 * Do not increase sites_avail redundantly for a new outgoing
	 * connection from a subordinate process.
	 */
	if (conn->type == REP_CONNECTION &&
	    site->state == SITE_CONNECTED && !IS_SUBORDINATE(db_rep)) {
		rep->sites_avail++;
		RPRINT(env, (env, DB_VERB_REPMGR_MISC,
		    "vers_resp: EID %lu CONNECTED, READY.  sites_avail %lu",
		    (u_long)conn->eid, (u_long)rep->sites_avail));
	}
	return (ret);
}

/*
 * Examine the rec part of a handshake message to see if it has any version
 * information in it.  This is the magic that lets us allows version-aware sites
 * to exchange information, and yet avoids tripping up v1 sites, which don't
 * know how to look for it.
 *
 * PUBLIC: int __repmgr_find_version_info __P((ENV *,
 * PUBLIC:     REPMGR_CONNECTION *, DBT *));
 */
int
__repmgr_find_version_info(env, conn, vi)
	ENV *env;
	REPMGR_CONNECTION *conn;
	DBT *vi;
{
	DBT *dbt;
	char *hostname;
	u_int32_t hostname_len;

	dbt = &conn->input.repmgr_msg.rec;
	if (dbt->size == 0) {
		__db_errx(env, DB_STR("3624",
		    "handshake is missing rec part"));
		return (DB_REP_UNAVAIL);
	}
	hostname = dbt->data;
	hostname[dbt->size-1] = '\0';
	hostname_len = (u_int32_t)strlen(hostname);
	if (hostname_len + 1 == dbt->size) {
		/*
		 * The rec DBT held only the host name.  This is a simple legacy
		 * V1 handshake; it contains no version information.
		 */
		vi->size = 0;
	} else {
		/*
		 * There's more data than just the host name.  The remainder is
		 * available to be treated as a normal byte buffer (and read in
		 * by one of the unmarshal functions).  Note that the remaining
		 * length should not include the padding byte that we have
		 * already clobbered.
		 */
		vi->data = &((u_int8_t *)dbt->data)[hostname_len + 1];
		vi->size = (dbt->size - (hostname_len+1)) - 1;
	}
	return (0);
}

static int
accept_handshake(env, conn, hostname, subordinate)
	ENV *env;
	REPMGR_CONNECTION *conn;
	char *hostname;
	int *subordinate;
{
	__repmgr_handshake_args hs;
	__repmgr_v2handshake_args hs2;
	__repmgr_v3handshake_args hs3;
	u_int port;
	u_int32_t ack, flags;
	int electable;

	*subordinate = 0;
	switch (conn->version) {
	case 2:
		if (__repmgr_v2handshake_unmarshal(env, &hs2,
		    conn->input.repmgr_msg.cntrl.data,
		    conn->input.repmgr_msg.cntrl.size, NULL) != 0)
			return (DB_REP_UNAVAIL);
		port = hs2.port;
		electable = hs2.priority > 0;
		ack = flags = 0;
		break;
	case 3:
		if (__repmgr_v3handshake_unmarshal(env, &hs3,
		   conn->input.repmgr_msg.cntrl.data,
		   conn->input.repmgr_msg.cntrl.size, NULL) != 0)
			return (DB_REP_UNAVAIL);
		port = hs3.port;
		electable = hs3.priority > 0;
		flags = hs3.flags;
		ack = 0;
		break;
	case 4:
	case 5:
		if (__repmgr_handshake_unmarshal(env, &hs,
		   conn->input.repmgr_msg.cntrl.data,
		   conn->input.repmgr_msg.cntrl.size, NULL) != 0)
			return (DB_REP_UNAVAIL);
		port = hs.port;
		electable = F_ISSET(&hs, ELECTABLE_SITE);
		flags = hs.flags;
		ack = hs.ack_policy;
		if (LF_ISSET(REPMGR_SUBORDINATE))
			*subordinate = 1;
		break;
	default:
		__db_errx(env, DB_STR_A("3679",
		    "unexpected conn version %lu in accept_handshake", "%lu"),
		    (u_long)conn->version);
		return (DB_REP_UNAVAIL);
	}

	return (process_parameters(env,
	    conn, hostname, port, ack, electable, flags));
}

static int
accept_v1_handshake(env, conn, hostname)
	ENV *env;
	REPMGR_CONNECTION *conn;
	char *hostname;
{
	DB_REPMGR_V1_HANDSHAKE *handshake;
	u_int32_t prio;
	int electable;

	handshake = conn->input.repmgr_msg.cntrl.data;
	if (conn->input.repmgr_msg.cntrl.size != sizeof(*handshake) ||
	    handshake->version != 1) {
		__db_errx(env, DB_STR("3625", "malformed V1 handshake"));
		return (DB_REP_UNAVAIL);
	}

	conn->version = 1;
	prio = ntohl(handshake->priority);
	electable = prio > 0;
	return (process_parameters(env,
	    conn, hostname, handshake->port, 0, electable, 0));
}

/* Caller must hold mutex. */
static int
process_parameters(env, conn, host, port, ack, electable, flags)
	ENV *env;
	REPMGR_CONNECTION *conn;
	char *host;
	u_int port;
	int electable;
	u_int32_t ack, flags;
{
	DB_REP *db_rep;
	REP *rep;
	REPMGR_RETRY *retry;
	REPMGR_SITE *site;
	SITEINFO *sites;
	__repmgr_connect_reject_args reject;
	__repmgr_v4connect_reject_args v4reject;
	u_int8_t reject_buf[__REPMGR_CONNECT_REJECT_SIZE];
	int eid, ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	/* Connection state can be used to discern incoming versus outgoing. */
	if (conn->state == CONN_CONNECTED) {
		/*
		 * Since we initiated this as an outgoing connection, we
		 * obviously already know the host, port and site.  We just need
		 * the other site's electability flag (which we'll grab below,
		 * after the big "else" clause).
		 */
		DB_ASSERT(env, IS_KNOWN_REMOTE_SITE(conn->eid));
		site = SITE_FROM_EID(conn->eid);
		RPRINT(env, (env, DB_VERB_REPMGR_MISC,
		    "handshake from connection to %s:%lu EID %u",
		    site->net_addr.host,
		    (u_long)site->net_addr.port, conn->eid));
	} else {
		DB_ASSERT(env, conn->state == CONN_NEGOTIATE ||
		    conn->state == CONN_PARAMETERS);
		/*
		 * Incoming connection: until now we haven't known what kind of
		 * connection we're dealing with (and in the case of a
		 * REP_CONNECTION, what its EID is); so it must be on the
		 * "orphans" list.  But now that we've received the parameters
		 * we'll be able to figure all that out.
		 */
		if (LF_ISSET(APP_CHANNEL_CONNECTION)) {
			conn->type = APP_CONNECTION;
			return (0);
		} else
			conn->type = REP_CONNECTION;

		/*
		 * Now that we've been given the host and port, use them to find
		 * the site.
		 */
		if ((site = __repmgr_lookup_site(env, host, port)) != NULL &&
		    site->membership == SITE_PRESENT) {
			TAILQ_REMOVE(&db_rep->connections, conn, entries);
			conn->ref_count--;

			eid = EID_FROM_SITE(site);
			if (LF_ISSET(REPMGR_SUBORDINATE)) {
				/*
				 * Accept it, as a supplementary source of
				 * input, but nothing else.
				 */
				TAILQ_INSERT_TAIL(&site->sub_conns,
				    conn, entries);
				conn->eid = eid;
				conn->auto_takeover =
				    LF_ISSET(REPMGR_AUTOTAKEOVER) ? 1 : 0;
				SET_LISTENER_CAND(conn->auto_takeover, ++);
				RPRINT(env, (env, DB_VERB_REPMGR_MISC,
		"handshake from subordinate %sconnection at site %s:%u EID %u",
				    LF_ISSET(REPMGR_AUTOTAKEOVER)?
				    "takeover ": "", host, port, eid));
			} else {
				DB_EVENT(env,
				    DB_EVENT_REP_CONNECT_ESTD, &eid);
				switch (site->state) {
				case SITE_PAUSING:
					RPRINT(env, (env, DB_VERB_REPMGR_MISC,
				      "handshake from paused site %s:%u EID %u",
					    host, port, eid));
					retry = site->ref.retry;
					TAILQ_REMOVE(&db_rep->retries,
					    retry, entries);
					__os_free(env, retry);
					site->ref.retry = NULL;
					break;
				case SITE_CONNECTED:
					/*
					 * We got an incoming connection for a
					 * site we were already connected to; at
					 * least we thought we were.
					 */
					RPRINT(env, (env, DB_VERB_REPMGR_MISC,
			 "connection from %s:%u EID %u while already connected",
					    host, port, eid));
					if ((ret = resolve_collision(env,
					    site, conn)) != 0)
						return (ret);
					break;
				case SITE_CONNECTING:
					RPRINT(env, (env, DB_VERB_REPMGR_MISC,
				  "handshake from connecting site %s:%u EID %u",
					    host, port, eid));
					/*
					 * Connector thread will give up when it
					 * sees this site's state change, so we
					 * don't have to do anything else here.
					 */
					break;
				default:
					DB_ASSERT(env, FALSE);
				}
				conn->eid = eid;
				site->state = SITE_CONNECTED;
				site->ref.conn.in = conn;
				__os_gettime(env,
				    &site->last_rcvd_timestamp, 1);
			}
		} else {
			RPRINT(env, (env, DB_VERB_REPMGR_MISC,
		  "rejecting connection from unknown or provisional site %s:%u",
			    host, port));
			if (conn->version < 5) {
				v4reject.version = db_rep->membership_version;
				v4reject.gen = db_rep->member_version_gen;
				__repmgr_v4connect_reject_marshal(env,
				    &v4reject, reject_buf);
			} else {
				reject.version = db_rep->membership_version;
				reject.gen = db_rep->member_version_gen;
				reject.status = (site) ? site->membership : 0;
				__repmgr_connect_reject_marshal(env,
				    &reject, reject_buf);
			}

			if ((ret = __repmgr_send_own_msg(env, conn,
			    REPMGR_CONNECT_REJECT, reject_buf,
			    __REPMGR_CONNECT_REJECT_SIZE)) != 0)
				return (ret);

			/*
			 * Since we haven't set conn->eid, bust_connection will
			 * not schedule a retry for this "failure", which is
			 * exactly what we want.
			 */
			return (DB_REP_UNAVAIL);
		}
	}

	if (electable)
		F_SET(site, SITE_ELECTABLE);
	else
		F_CLR(site, SITE_ELECTABLE);
	F_SET(site, SITE_HAS_PRIO);
	site->ack_policy = (int)ack;

	/*
	 * If we're moping around wishing we knew who the master was, then
	 * getting in touch with another site might finally provide sufficient
	 * connectivity to find out.
	 */
	if (!IS_SUBORDINATE(db_rep) && /* us */
	    !__repmgr_master_is_known(env) &&
	    !LF_ISSET(REPMGR_SUBORDINATE)) { /* the remote site */
		RPRINT(env, (env, DB_VERB_REPMGR_MISC,
		    "handshake with no known master to wake election thread"));
		db_rep->new_connection = TRUE;
		if ((ret = __repmgr_signal(&db_rep->check_election)) != 0)
			return (ret);
	}

	return (0);
}

static int
resolve_collision(env, site, conn)
	ENV *env;
	REPMGR_SITE *site;
	REPMGR_CONNECTION *conn;
{
	int ret;

	/*
	 * No need for site-oriented recovery, since we now have a replacement
	 * connection; so skip bust_connection() and call disable_conn()
	 * directly.
	 *
	 * If we already had an incoming connection, this new one always
	 * replaces it.  Whether it also/alternatively replaces an outgoing
	 * connection depends on whether we're client or server (so as to avoid
	 * connection collisions resulting in no remaining connections).  (If
	 * it's an older version that doesn't know about our collision
	 * resolution protocol, it will behave like a client.)
	 */
	if (site->ref.conn.in != NULL) {
		ret = __repmgr_disable_connection(env, site->ref.conn.in);
		site->ref.conn.in = NULL;
		if (ret != 0)
			return (ret);
	}
	if (site->ref.conn.out != NULL &&
	    conn->version >= CONN_COLLISION_VERSION &&
	    __repmgr_is_server(env, site)) {
		ret = __repmgr_disable_connection(env, site->ref.conn.out);
		site->ref.conn.out = NULL;
		if (ret != 0)
			return (ret);
	}
	return (0);
}

static int
record_permlsn(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	__repmgr_permlsn_args *ackp, ack;
	SITE_STRING_BUFFER location;
	u_int32_t gen;
	int ret;
	u_int do_log_check;

	db_rep = env->rep_handle;
	do_log_check = 0;

	if (conn->version == 0 ||
	    !IS_READY_STATE(conn->state) || !IS_VALID_EID(conn->eid)) {
		__db_errx(env, DB_STR("3682",
		    "unexpected connection info in record_permlsn"));
		return (DB_REP_UNAVAIL);
	}
	site = SITE_FROM_EID(conn->eid);

	/*
	 * Extract the LSN.  Save it only if it is an improvement over what the
	 * site has already ack'ed.
	 */
	if (conn->version == 1) {
		ackp = conn->input.repmgr_msg.cntrl.data;
		if (conn->input.repmgr_msg.cntrl.size != sizeof(ack) ||
		    conn->input.repmgr_msg.rec.size != 0) {
			__db_errx(env, DB_STR("3627", "bad ack msg size"));
			return (DB_REP_UNAVAIL);
		}
	} else {
		ackp = &ack;
		if ((ret = __repmgr_permlsn_unmarshal(env, ackp,
			 conn->input.repmgr_msg.cntrl.data,
			 conn->input.repmgr_msg.cntrl.size, NULL)) != 0)
			return (DB_REP_UNAVAIL);
	}

	/* Ignore stale acks. */
	gen = db_rep->region->gen;
	if (ackp->generation < gen) {
		VPRINT(env, (env, DB_VERB_REPMGR_MISC,
		    "ignoring stale ack (%lu<%lu), from %s",
		     (u_long)ackp->generation, (u_long)gen,
		     __repmgr_format_site_loc(site, location)));
		return (0);
	}
	VPRINT(env, (env, DB_VERB_REPMGR_MISC,
	    "got ack [%lu][%lu](%lu) from %s", (u_long)ackp->lsn.file,
	    (u_long)ackp->lsn.offset, (u_long)ackp->generation,
	    __repmgr_format_site_loc(site, location)));

	if (ackp->generation == gen &&
	    LOG_COMPARE(&ackp->lsn, &site->max_ack) == 1) {
		/*
		 * If file number for this site changed, check lowest log
		 * file needed after recording new permlsn for this site.
		 */
		if (ackp->lsn.file > site->max_ack.file)
			do_log_check = 1;
		memcpy(&site->max_ack, &ackp->lsn, sizeof(DB_LSN));
		if (do_log_check)
			check_min_log_file(env);
		if ((ret = __repmgr_wake_waiters(env,
		    &db_rep->ack_waiters)) != 0)
			return (ret);
	}
	return (0);
}

/*
 * Maintains lowest log file still needed by the repgroup.  This is stored
 * in shared rep region so that it is accessible to repmgr subordinate
 * processes that may not themselves have connections to other sites
 * (e.g. a separate db_archive process.)
 */
static void
check_min_log_file(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	REPMGR_CONNECTION *conn;
	REPMGR_SITE *site;
	u_int32_t min_log;
	int eid;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	min_log = 0;

	/*
	 * Record the lowest log file number from all connected sites.  If this
	 * is a client, ignore the master because the master does not maintain
	 * nor send out its repmgr perm LSN in this way.  Consider connections
	 * so that we don't allow a site that has been down a long time to
	 * indefinitely prevent log archiving.
	 */
	FOR_EACH_REMOTE_SITE_INDEX(eid) {
		if (eid == rep->master_id)
			continue;
		site = SITE_FROM_EID(eid);
		if (site->state == SITE_CONNECTED &&
		    (((conn = site->ref.conn.in) != NULL &&
		    conn->state == CONN_READY) ||
		    ((conn = site->ref.conn.out) != NULL &&
		    conn->state == CONN_READY)) &&
		    !IS_ZERO_LSN(site->max_ack) &&
		    (min_log == 0 || site->max_ack.file < min_log))
			min_log = site->max_ack.file;
	}
	/*
	 * During normal operation min_log should increase over time, but it
	 * is possible if a site returns after being disconnected for a while
	 * that min_log could decrease.
	 */
	if (min_log != 0 && min_log != rep->min_log_file)
		rep->min_log_file = min_log;
}

/*
 * PUBLIC: int __repmgr_write_some __P((ENV *, REPMGR_CONNECTION *));
 */
int
__repmgr_write_some(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	QUEUED_OUTPUT *output;
	REPMGR_FLAT *msg;
	int bytes, ret;

	while (!STAILQ_EMPTY(&conn->outbound_queue)) {
		output = STAILQ_FIRST(&conn->outbound_queue);
		msg = output->msg;
		if ((bytes = sendsocket(conn->fd, &msg->data[output->offset],
		    msg->length - output->offset, 0)) == SOCKET_ERROR) {
			switch (ret = net_errno) {
			case WOULDBLOCK:
#if defined(DB_REPMGR_EAGAIN) && DB_REPMGR_EAGAIN != WOULDBLOCK
			case DB_REPMGR_EAGAIN:
#endif
				return (0);
			default:
				__repmgr_fire_conn_err_event(env, conn, ret);
				STAT(env->rep_handle->
				    region->mstat.st_connection_drop++);
				return (DB_REP_UNAVAIL);
			}
		}

		if ((output->offset += (size_t)bytes) >= msg->length) {
			STAILQ_REMOVE_HEAD(&conn->outbound_queue, entries);
			__os_free(env, output);
			conn->out_queue_length--;
			if (--msg->ref_count <= 0)
				__os_free(env, msg);

			/*
			 * We've achieved enough movement to free up at least
			 * one space in the outgoing queue.  Wake any message
			 * threads that may be waiting for space.  Leave
			 * CONGESTED state so that when the queue reaches the
			 * high-water mark again, the filling thread will be
			 * allowed to try waiting again.
			 */
			conn->state = CONN_READY;
			if ((ret = __repmgr_signal(&conn->drained)) != 0)
				return (ret);
		}
	}

	return (0);
}
