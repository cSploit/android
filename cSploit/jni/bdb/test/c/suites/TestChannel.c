/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates. All rights reserved.
 */

#include <ctype.h>
#include <stdio.h>
#include <db.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#ifndef _WIN32
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/uio.h>
#endif

#include "CuTest.h"
#include "test_util.h"

#define MAX_SEGS 10
#define MAX_MSGS 10

#undef LOCK_MUTEX
#undef UNLOCK_MUTEX
#ifdef _WIN32
#define sleep(s) Sleep(1000 * (s))
typedef HANDLE mutex_t;
#define	mutex_init(m)						   \
    (((*(m) = CreateMutex(NULL, FALSE, NULL)) != NULL) ? 0 : -1)
#define	mutex_lock(m)							   \
    ((WaitForSingleObject(*(m), INFINITE) == WAIT_OBJECT_0) ?              \
        0 : GetLastError())
#define	mutex_unlock(m)		(ReleaseMutex(*(m)) ? 0 : GetLastError())
#define	mutex_destroy(m)	(CloseHandle(*(m)) ? 0 : GetLastError())
typedef HANDLE cond_t;
#define	cond_init(c)	((*(c) = CreateEvent(NULL,	\
		    TRUE, FALSE, NULL)) == NULL ? GetLastError() : 0)
#define cond_wait(c, m) (SignalObjectAndWait(*(m), *(c), INFINITE, FALSE) == WAIT_OBJECT_0 ? \
	    0 : GetLastError())
#define cond_wake(c) (SetEvent(*(c)) ? 0 : GetLastError())
#else
typedef pthread_mutex_t mutex_t;
#define	mutex_init(m)	pthread_mutex_init((m), NULL)
#define	mutex_lock(m)		pthread_mutex_lock(m)
#define	mutex_unlock(m)		pthread_mutex_unlock(m)
#define	mutex_destroy(m)	pthread_mutex_destroy(m)
typedef pthread_cond_t cond_t;
#define	cond_init(c)	pthread_cond_init((c), NULL)
#define	cond_wait(c, m)	pthread_cond_wait((c), (m))
#define	cond_wake(c)	pthread_cond_broadcast(c)
#endif

struct channel_test_globals {
	CuTest *test;
	mutex_t mtx;
	cond_t cond;
	u_int *ports;
	int ports_cnt;
};

struct report {
	int dbt_count;
	DBT dbt[MAX_SEGS];

	int msg_count;
	char *msg[MAX_MSGS];

	int done, ret;
};

struct reports {
	mutex_t m;
	int count;
	struct report rpt[2];
};

struct env_info {
	struct report *rpt;
	struct reports *rpts;
	struct channel_test_globals *g;
	int startupdone;
};

struct msginfo {
	DB_ENV *dbenv;
	int count;
};

typedef int (*PRED) __P((void *));

static int await_condition __P((PRED, void *, long));
static int check_dbt_string __P((DBT *, const char *));
static void clear_rpt __P((DB_ENV *));
static void clear_rpt_int __P((struct report *));
static int env_done __P((void *));
static struct report *get_rpt __P((const DB_ENV *));
static int fortify __P((DB_ENV *, struct channel_test_globals *));
static int get_avail_ports __P((u_int *, int));
static int has_msgs __P((void *));
static void msg_disp __P((DB_ENV *, DB_CHANNEL *, DBT *, u_int32_t, u_int32_t));
static void msg_disp2 __P((DB_ENV *, DB_CHANNEL *, DBT *, u_int32_t, u_int32_t));
static void msg_disp3 __P((DB_ENV *, DB_CHANNEL *, DBT *, u_int32_t, u_int32_t));
static void msg_disp4 __P((DB_ENV *, DB_CHANNEL *, DBT *, u_int32_t, u_int32_t));
static void msg_disp5 __P((DB_ENV *, DB_CHANNEL *, DBT *, u_int32_t, u_int32_t));
static void notify __P((DB_ENV *, u_int32_t, void *));
static int is_started __P((void *));
static void td __P((DB_ENV *));
static void test_data_init __P((DBT *, char *));
static void test_zeroes __P((DB_CHANNEL *, DB_ENV *, CuTest *));
static int two_done __P((void *));

#define	LOCK_MUTEX(m) do {		      \
	int __ret; \
	__ret = mutex_lock(m); \
	assert(__ret == 0); \
} while (0)

#define	UNLOCK_MUTEX(m) do {		      \
	int __ret;			      \
	__ret = mutex_unlock(m); \
	assert(__ret == 0); \
} while (0)

int TestChannelSuiteSetup(CuSuite *suite) {
	return (0);
}

int TestChannelSuiteTeardown(CuSuite *suite) {
	return (0);
}

int TestChannelTestSetup(CuTest *test) {
	struct channel_test_globals *g;
	int ret;

	if ((g = calloc(1, sizeof(*g))) == NULL)
		return (ENOMEM);
	if ((ret = mutex_init(&g->mtx)) != 0) {
		free(g);
		return (ret);
	}
	if ((ret = cond_init(&g->cond)) != 0) {
		mutex_destroy(&g->mtx);
		free(g);
		return (ret);
	}
	g->test = test;
	test->context = g;
	return (0);
}

int TestChannelTestTeardown(CuTest *test) {
	struct channel_test_globals *g;
	int ret;

	g = test->context;
	assert(g != NULL);
	ret = mutex_destroy(&g->mtx);
	free(g);
	test->context = NULL;
	return (ret);
}

static void
myerrcall(const DB_ENV *dbenv, const char *errpfx, const char *msg) {
	struct report *rpt = get_rpt(dbenv);
	char *msgp;

	assert(rpt->msg_count < MAX_MSGS);
	msgp = strdup(msg);
	assert(msgp != NULL);
	rpt->msg[rpt->msg_count++] = msgp;
}

static int
fortify(dbenv, g)
	DB_ENV *dbenv;
	struct channel_test_globals *g;
{
	struct report *rpt;
	struct env_info *info;

	if ((info = calloc(1, sizeof(*info))) == NULL)
		return (ENOMEM);
	if ((rpt = calloc(1, sizeof(*rpt))) == NULL) {
		free(info);
		return (ENOMEM);
	}
	info->rpt = rpt;
	info->rpts = NULL;
	info->g = g;
	info->startupdone = 0;
	dbenv->app_private = info;
	return (0);
}

static int
setup(envp1, envp2, envp3, g)
	DB_ENV **envp1, **envp2, **envp3;
	struct channel_test_globals *g;
{
	DB_ENV *dbenv1, *dbenv2, *dbenv3;
	DB_SITE *dbsite;
	u_int32_t flags;
	int ret;
	u_int *ports;

#define CHECK(call) \
	do {	    \
	if ((ret = (call)) != 0) {		\
	fprintf(stderr, "error %d from %s", ret, #call);	\
	goto err;						\
	}							\
	} while (0);

	ports = g->ports;

	dbenv1 = dbenv2 = dbenv3 = NULL;
	CHECK(db_env_create(&dbenv1, 0));
	CHECK(fortify(dbenv1, g));
	dbenv1->set_errpfx(dbenv1, "ENV1");
	dbenv1->set_errcall(dbenv1, myerrcall);
	flags = DB_INIT_REP | DB_INIT_LOG | DB_INIT_LOCK | DB_INIT_MPOOL |
	    DB_INIT_TXN | DB_RECOVER | DB_THREAD | DB_CREATE;
	setup_envdir("DIR1", 1);
	CHECK(dbenv1->open(dbenv1, "DIR1", flags, 0));
	
	CHECK(dbenv1->rep_set_config(dbenv1, DB_REPMGR_CONF_ELECTIONS, 0));
	CHECK(dbenv1->repmgr_site(dbenv1, "localhost", ports[0], &dbsite, 0));
	CHECK(dbsite->set_config(dbsite, DB_LOCAL_SITE, 1));
	CHECK(dbsite->close(dbsite));
	CHECK(dbenv1->set_event_notify(dbenv1, notify));
	CHECK(dbenv1->repmgr_msg_dispatch(dbenv1, msg_disp, 0));
	CHECK(dbenv1->repmgr_start(dbenv1, 2, DB_REP_MASTER));

	CHECK(db_env_create(&dbenv2, 0));
	CHECK(fortify(dbenv2, g));
	dbenv2->set_errpfx(dbenv2, "ENV2");
	dbenv2->set_errcall(dbenv2, myerrcall);
	setup_envdir("DIR2", 1);
	CHECK(dbenv2->open(dbenv2, "DIR2", flags, 0));
	CHECK(dbenv2->rep_set_config(dbenv2, DB_REPMGR_CONF_ELECTIONS, 0));

	CHECK(dbenv2->repmgr_site(dbenv2, "localhost", ports[1], &dbsite, 0));
	CHECK(dbsite->set_config(dbsite, DB_LOCAL_SITE, 1));
	CHECK(dbsite->close(dbsite));
	CHECK(dbenv2->repmgr_site(dbenv2, "localhost", ports[0], &dbsite, 0));
	CHECK(dbsite->set_config(dbsite, DB_BOOTSTRAP_HELPER, 1));
	CHECK(dbsite->close(dbsite));
	CHECK(dbenv2->set_event_notify(dbenv2, notify));
	CHECK(dbenv2->repmgr_start(dbenv2, 2, DB_REP_CLIENT));

	await_condition(is_started, dbenv2, 60);
	if (!is_started(dbenv2)) {
		dbenv2->errx(dbenv2, "startup done not achieved in 60 seconds");
		ret = DB_TIMEOUT;
		goto err;
	}

	CHECK(db_env_create(&dbenv3, 0));
	CHECK(fortify(dbenv3, g));
	dbenv3->set_errpfx(dbenv3, "ENV3");
	dbenv3->set_errcall(dbenv3, myerrcall);
	CHECK(dbenv3->repmgr_msg_dispatch(dbenv3, msg_disp2, 0));
	setup_envdir("DIR3", 1);
	CHECK(dbenv3->open(dbenv3, "DIR3", flags, 0));
	CHECK(dbenv3->rep_set_config(dbenv3, DB_REPMGR_CONF_ELECTIONS, 0));

	CHECK(dbenv3->repmgr_site(dbenv3, "localhost", ports[2], &dbsite, 0));
	CHECK(dbsite->set_config(dbsite, DB_LOCAL_SITE, 1));
	CHECK(dbsite->close(dbsite));
	CHECK(dbenv3->repmgr_site(dbenv3, "localhost", ports[0], &dbsite, 0));
	CHECK(dbsite->set_config(dbsite, DB_BOOTSTRAP_HELPER, 1));
	CHECK(dbsite->close(dbsite));
	CHECK(dbenv3->set_event_notify(dbenv3, notify));
	CHECK(dbenv3->repmgr_start(dbenv3, 2, DB_REP_CLIENT));

	await_condition(is_started, dbenv3, 60);
	if (!is_started(dbenv3)) {
		dbenv3->errx(dbenv3, "startup done not achieved in 60 seconds");
		ret = DB_TIMEOUT;
		goto err;
	}

	*envp1 = dbenv1;
	*envp2 = dbenv2;
	*envp3 = dbenv3;
	return (0);

err:
	if (dbenv3 != NULL)
		td(dbenv3);
	if (dbenv2 != NULL)
		td(dbenv2);
	if (dbenv1 != NULL)
		td(dbenv1);
	return (ret);
}

static void
td(dbenv)
	DB_ENV *dbenv;
{
	struct env_info *info;
	
	dbenv->set_errcall(dbenv, NULL);
	dbenv->set_event_notify(dbenv, NULL);

	info = dbenv->app_private;
	dbenv->close(dbenv, 0);
	if (info != NULL) {
		clear_rpt_int(info->rpt);
		free(info->rpt);
		free(info);
	}
}

static void
clear_rpt_int(rpt)
	struct report *rpt;
{
	int i;

	for (i = 0; i < rpt->dbt_count; i++)
		free(rpt->dbt[i].data);
	rpt->dbt_count = 0;

	for (i = 0; i < rpt->msg_count; i++)
		free(rpt->msg[i]);
	rpt->msg_count = 0;

	rpt->done = 0;
}

static void
clear_rpt(dbenv)
	DB_ENV *dbenv;
{
	struct env_info *info;
	struct report *rpt;

	info = dbenv->app_private;
	rpt = info->rpt;
	clear_rpt_int(rpt);
}	

static int
env_done(ctx)
	void *ctx;
{
	DB_ENV *dbenv = ctx;
	struct report *rpt = get_rpt(dbenv);

	return (rpt->done);
}

static void
await_done(dbenv)
	DB_ENV *dbenv;
{
	await_condition(env_done, dbenv, 60);
	assert(env_done(dbenv));
}

static int
has_msgs(ctx)
	void *ctx;
{
	struct msginfo *inf = ctx;
	DB_ENV *dbenv = inf->dbenv;
	struct report *rpt = get_rpt(dbenv);
	
	return (rpt->msg_count == inf->count);
}

static struct report *
get_rpt(dbenv)
	const DB_ENV *dbenv;
{
	struct env_info *info;

	if ((info = dbenv->app_private) == NULL)
		return (NULL);
	return (info->rpt);
}

int TestChannelFeature(CuTest *ct) {
/* Run this test only when replication is supported. */
#ifdef HAVE_REPLICATION
	DB_ENV *dbenv1, *dbenv2, *dbenv3;
	DB_CHANNEL *ch;
	DB_REP_STAT *stats;
	DB_SITE *dbsite;
	DBT dbt, rdbts[10], resp;
	struct channel_test_globals *g;
	struct report *rpt;
	struct reports rpts;
	struct msginfo info;
	char *p;
	void *pointer, *vp, *buffer;
	u_int8_t short_buf[4];
	size_t sz;
	int done, eid, ret;
	u_int ports[3];
	
#ifdef _WIN32
	setvbuf(stdout, NULL, _IONBF, 0);
#endif
	printf("this is a test for repmgr channels feature\n");

	g = ct->context;
	ret = get_avail_ports(ports, 3);
	CuAssertTrue(ct, (ret == 0));

	g->ports = ports;
	g->ports_cnt = 3;

	printf("use ports: {%u, %u, %u}\n", ports[0], ports[1], ports[2]);
	
	/* 
	 * The ports[0] will be the local port for dbenv1, ports[1] for dbenv2,
	 * and ports[2] for dbenv3.
	 */
	CuAssertTrue(ct, (ret = setup(&dbenv1, &dbenv2, &dbenv3, g) == 0));

	/*
	 * For this first section, we're sending to ENV2.  
	 */
	CuAssertTrue(ct,
	    (ret = dbenv1->repmgr_site(dbenv1,
		"localhost", ports[1], &dbsite, 0)) == 0);
	CuAssertTrue(ct, (ret = dbsite->get_eid(dbsite, &eid)) == 0);
	CuAssertTrue(ct, (ret = dbsite->close(dbsite)) == 0);
	CuAssertTrue(ct, (ret = dbenv1->repmgr_channel(dbenv1, eid, &ch, 0)) == 0);

	memset(&dbt, 0, sizeof(dbt));
	p = "foobar";
	dbt.data = p;
	dbt.size = (u_int32_t)strlen(p) + 1;
	memset(&resp, 0, sizeof(resp));
	resp.flags = DB_DBT_MALLOC;
	printf("1. send async msg with no msg dispatch in place\n");
	clear_rpt(dbenv2);
	CuAssertTrue(ct, (ret = ch->send_msg(ch, &dbt, 1, 0)) == 0);

	/* Wait til dbenv2 has reported 1 msg. */
	info.dbenv = dbenv2;
	info.count = 1;
	await_condition(has_msgs, &info, 90);
	rpt = get_rpt(dbenv2);
	CuAssertTrue(ct, rpt->msg_count == 1);
	CuAssertTrue(ct, strncmp(rpt->msg[0], "BDB3670", strlen("BDB3670")) == 0);

	printf("2. send request with no msg dispatch in place\n");
	clear_rpt(dbenv2);
	ret = ch->send_request(ch, &dbt, 1, &resp, 0, 0);
	CuAssertTrue(ct, ret == DB_NOSERVER);
	if (resp.data != NULL)
		free(resp.data);
	await_condition(has_msgs, &info, 90);
	CuAssertTrue(ct, rpt->msg_count == 1);
	CuAssertTrue(ct, strncmp(rpt->msg[0], "BDB3670", strlen("BDB3670")) == 0);

	CuAssertTrue(ct, (ret = dbenv2->repmgr_msg_dispatch(dbenv2, msg_disp, 0)) == 0);

	printf("3. send request where recip forgot resp\n");
	clear_rpt(dbenv2);
	ret = ch->send_request(ch, &dbt, 1, &resp, 0, 0);
	CuAssertTrue(ct, ret == DB_KEYEMPTY);
	if (resp.data != NULL)
		free(resp.data);
	await_done(dbenv2);
	CuAssertTrue(ct, rpt->msg_count == 1);
	CuAssertTrue(ct, strncmp(rpt->msg[0], "BDB3671", strlen("BDB3671")) == 0);

	printf("4. now with dispatch fn installed, send a simple async msg\n");
	clear_rpt(dbenv2);
	test_data_init(&dbt, "Mr. Watson -- come here -- I want to see you.");
	CuAssertTrue(ct, (ret = ch->send_msg(ch, &dbt, 1, 0)) == 0);
	await_done(dbenv2);
	CuAssertTrue(ct, rpt->dbt_count == 1);
	check_dbt_string(&rpt->dbt[0],
	    "Mr. Watson -- come here -- I want to see you.");
	CuAssertTrue(ct, rpt->msg_count == 0);
	
	printf("5. send a multi-seg request\n");
	clear_rpt(dbenv2);
	memset(&resp, 0, sizeof(resp));
	resp.flags = DB_DBT_MALLOC;
	test_data_init(&rdbts[0], "I wish I were a fish");
	test_data_init(&rdbts[1], "I wish I were a bass");
	test_data_init(&rdbts[2],
	    "I'd climb up on a slippery rock and slide down on my ... hands and knees");
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 3, &resp, 0, 0)) == 0);
	check_dbt_string(&resp, "this is the answer to the request");
	if (resp.data)
		free(resp.data);
	await_done(dbenv2);
	CuAssertTrue(ct, rpt->dbt_count == 3);
	check_dbt_string(&rpt->dbt[0], "I wish I were a fish");
	check_dbt_string(&rpt->dbt[1], "I wish I were a bass");
	check_dbt_string(&rpt->dbt[2],
	    "I'd climb up on a slippery rock and slide down on my ... hands and knees");
	CuAssertTrue(ct, rpt->msg_count == 0);

	test_zeroes(ch, dbenv2, ct);

	printf("7. send request with too-small USERMEM buffer\n");
	clear_rpt(dbenv2);
	resp.data = short_buf;
	resp.ulen = sizeof(short_buf);
	resp.flags = DB_DBT_USERMEM;
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 3, &resp, 0, 0)) == DB_BUFFER_SMALL);
	await_done(dbenv2);
	CuAssertTrue(ct, rpt->msg_count == 1);
	CuAssertTrue(ct, strncmp(rpt->msg[0], "BDB3659", strlen("BDB3659")) == 0);
	CuAssertTrue(ct, rpt->ret == EINVAL);

#define BUFLEN 20000
	buffer = malloc(BUFLEN);
	if (buffer == NULL)
		return (2);
	resp.data = buffer;
	resp.ulen = BUFLEN;
	resp.flags = DB_DBT_USERMEM;

	printf("8. send USERMEM request without necessary DB_MULTIPLE\n");
	clear_rpt(dbenv2);
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 2, &resp, 0, 0)) == DB_BUFFER_SMALL);
	await_done(dbenv2);
	CuAssertTrue(ct, rpt->msg_count == 1);
	CuAssertTrue(ct, strncmp(rpt->msg[0], "BDB3658", strlen("BDB3658")) == 0);
	CuAssertTrue(ct, rpt->ret == EINVAL);

	printf("9. send USERMEM request with DB_MULTIPLE\n");
	clear_rpt(dbenv2);
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 2, &resp, 0, DB_MULTIPLE)) == 0);
	DB_MULTIPLE_INIT(pointer, &resp);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, rpt->ret == 0);
	CuAssertTrue(ct, strcmp((char*)vp, "roses are red") == 0);
	CuAssertTrue(ct, sz == strlen((char*)vp) + 1);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, strcmp((char*)vp, "violets are blue") == 0);
	CuAssertTrue(ct, sz == strlen((char*)vp) + 1);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, pointer == NULL);
	
	ch->close(ch, 0);


	/* ------------------------------- */

	
	CuAssertTrue(ct, (ret = dbenv2->repmgr_channel(dbenv2, DB_EID_MASTER, &ch, 0)) == 0);
	CuAssertTrue(ct, (ret = dbenv1->repmgr_msg_dispatch(dbenv1, msg_disp2, 0)) == 0);

	// do a request to master
	// switch masters
	// do a request to new master
	printf("(now we try a couple of operations on a master channel)\n");

	printf("10. send request to original master\n");
	rpt = get_rpt(dbenv1);
	clear_rpt(dbenv1);
	resp.data = buffer;
	resp.ulen = BUFLEN;
	resp.flags = DB_DBT_USERMEM;
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 1, &resp, 0, 0)) == 0);
	check_dbt_string(&resp, "ENV1");
	await_done(dbenv1);
	CuAssertTrue(ct, rpt->ret == 0);
	CuAssertTrue(ct, rpt->dbt_count == 1);
	check_dbt_string(&rpt->dbt[0], "I wish I were a fish");

	printf("switch master and wait for our client to see the change\n");
	((struct env_info *)dbenv2->app_private)->startupdone = 0;
	CuAssertTrue(ct, (ret = dbenv1->repmgr_start(dbenv1, 0, DB_REP_CLIENT)) == 0);
	sleep(1);		/* workaround for 19329 */
	for (done = 0; ; ) {
		/*
		 * Become master, and then make sure it really happened.
		 * Occasionally a race develops, where we're still holding on to
		 * the msg lockout at env3 at this point, in which case the
		 * rep_start() call (underlying our repmgr_start() call here) is
		 * simply dropped on the floor.
		 */ 
		CuAssertTrue(ct, (ret = dbenv3->repmgr_start(dbenv3,
			    0, DB_REP_MASTER)) == 0);
		CuAssertTrue(ct, (ret = dbenv3->rep_stat(dbenv3,
			    &stats, 0)) == 0);
		done = stats->st_status == DB_REP_MASTER;
		free(stats);
		if (done)
			break;
		sleep(1);
	};

	/*
	 * !!!
	 * Workaround for 19297: wait until verify dance is complete at env2,
	 * because (just a little bit) later we're going to switch master again,
	 * to env2.  If rep_start(MASTER) at env2 happens while processing
	 * VERIFY match record, core rep ignores the rep_start() (even though it
	 * returns 0).
	 */ 
	LOCK_MUTEX(&g->mtx);
	while (!((struct env_info *)dbenv2->app_private)->startupdone) {
		cond_wait(&g->cond, &g->mtx);
	}
// TODO: fix these macros so that this ridiculous hack isn't necessary
#ifndef _WIN32
	UNLOCK_MUTEX(&g->mtx);
#endif


	printf("11. send request which should go to new master (only)\n");
	clear_rpt(dbenv1);
	clear_rpt(dbenv3);
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 1, &resp, 0, 0)) == 0);
	check_dbt_string(&resp, "ENV3");
	rpt = get_rpt(dbenv3);
	await_done(dbenv3);
	CuAssertTrue(ct, rpt->ret == 0);
	CuAssertTrue(ct, rpt->dbt_count == 1);
	check_dbt_string(&rpt->dbt[0], "I wish I were a fish");
	rpt = get_rpt(dbenv1);
	CuAssertTrue(ct, !rpt->done);	/* old master shouldn't have recvd anything */

	printf("switch master again, to ``self''\n");
	CuAssertTrue(ct, (ret = dbenv3->repmgr_start(dbenv3, 0, DB_REP_CLIENT)) == 0);
	CuAssertTrue(ct, (ret = dbenv2->repmgr_start(dbenv2, 0, DB_REP_MASTER)) == 0);
	/* No need to wait for env2 to see that env2 has become master. */

	clear_rpt(dbenv1);
	clear_rpt(dbenv2);
	clear_rpt(dbenv3);
	printf("12. send to self, async\n");
	CuAssertTrue(ct, (ret = ch->send_msg(ch, &dbt, 1, 0)) == 0);
	await_done(dbenv2);
	rpt = get_rpt(dbenv2);
	CuAssertTrue(ct, rpt->dbt_count == 1);
	check_dbt_string(&rpt->dbt[0],
	    "Mr. Watson -- come here -- I want to see you.");
	CuAssertTrue(ct, rpt->msg_count == 0);
	printf("    (check that other two sites didn't receive it)\n");
	sleep(1);
	CuAssertTrue(ct, !get_rpt(dbenv1)->done);
	CuAssertTrue(ct, !get_rpt(dbenv3)->done);

	printf("13. send-to-self request\n");
	clear_rpt(dbenv2);
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 2, &resp, 0, DB_MULTIPLE)) == 0);
	DB_MULTIPLE_INIT(pointer, &resp);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, rpt->ret == 0);
	CuAssertTrue(ct, strcmp((char*)vp, "roses are red") == 0);
	CuAssertTrue(ct, sz == strlen((char*)vp) + 1);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, strcmp((char*)vp, "violets are blue") == 0);
	CuAssertTrue(ct, sz == strlen((char*)vp) + 1);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, pointer == NULL);
	
	/*
	 * re-test the 0-length cases, in the send-to-self context (the
	 * implementation has a bunch of separate code)
	 */
	test_zeroes(ch, dbenv2, ct);

	ch->close(ch, 0);

	/* ---------------------------------------- */

	// If you go from env2 to env, we know that it's ports[0]
	// 
	CuAssertTrue(ct, (ret = dbenv1->repmgr_msg_dispatch(dbenv1, msg_disp3, 0)) == 0);
	CuAssertTrue(ct,
	    (ret = dbenv2->repmgr_site(dbenv2,
		"localhost", ports[0], &dbsite, 0)) == 0);
	CuAssertTrue(ct, (ret = dbsite->get_eid(dbsite, &eid)) == 0);
	CuAssertTrue(ct, (ret = dbsite->close(dbsite)) == 0);
	CuAssertTrue(ct, (ret = dbenv2->repmgr_channel(dbenv2, eid, &ch, 0)) == 0);

	printf("14. send request to site that has been shut down\n");
	td(dbenv1);
	memset(&resp, 0, sizeof(resp));
	resp.flags = DB_DBT_MALLOC;
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 2, &resp, 0, 0)) ==
	    DB_REP_UNAVAIL);
	if (resp.data != NULL)
		free(resp.data);

	// TODO: a much more interesting case is to have the remote site shut
	// down while waiting for the response, because that exercises some
	// clean-up code.  But I guess that requires running in a couple of
	// threads.
	
	ch->close(ch, 0);

	printf("15. try to connect to a down site\n");
	CuAssertTrue(ct, (ret = dbenv2->repmgr_channel(dbenv2, eid, &ch, 0)) == DB_REP_UNAVAIL);

	printf("16. try to connect to a non-existent EID\n");
	CuAssertTrue(ct, (ret = dbenv2->repmgr_channel(dbenv2, 1732, &ch, 0)) == EINVAL);

	printf("17. connect master to self from the start\n");
	CuAssertTrue(ct, (ret = dbenv2->repmgr_channel(dbenv2, DB_EID_MASTER, &ch, 0)) == 0);
	CuAssertTrue(ct, (ret = dbenv2->repmgr_msg_dispatch(dbenv2, msg_disp2, 0)) == 0);
	rpt = get_rpt(dbenv2);
	clear_rpt(dbenv2);
	resp.data = buffer;
	resp.ulen = BUFLEN;
	resp.flags = DB_DBT_USERMEM;
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 1, &resp, 0, 0)) == 0);
	check_dbt_string(&resp, "ENV2");
	await_done(dbenv2);
	CuAssertTrue(ct, rpt->ret == 0);
	CuAssertTrue(ct, rpt->dbt_count == 1);
	check_dbt_string(&rpt->dbt[0], "I wish I were a fish");

	ch->close(ch, 0);

	/*
	 * Send an async message from env2 to env3, at which point env3 will
	 * reply by returning two async messages back to env2.
	 */
	printf("18. test async replies to (async) messages\n");
	CuAssertTrue(ct, (ret = dbenv3->repmgr_msg_dispatch(dbenv3, msg_disp3, 0)) == 0);
	CuAssertTrue(ct, (ret = dbenv2->repmgr_msg_dispatch(dbenv2, msg_disp4, 0)) == 0);
	CuAssertTrue(ct,
	    (ret = dbenv2->repmgr_site(dbenv2,
		"localhost", ports[2], &dbsite, 0)) == 0);
	CuAssertTrue(ct, (ret = dbsite->get_eid(dbsite, &eid)) == 0);
	CuAssertTrue(ct, (ret = dbsite->close(dbsite)) == 0);
	CuAssertTrue(ct, (ret = dbenv2->repmgr_channel(dbenv2, eid, &ch, 0)) == 0);
	rpt = get_rpt(dbenv3);
	clear_rpt(dbenv3);
	((struct env_info *)dbenv2->app_private)->rpts = &rpts;
	memset(&rpts, 0, sizeof(rpts));
	mutex_init(&rpts.m);
	CuAssertTrue(ct, (ret = ch->send_msg(ch, rdbts, 1, 0)) == 0);
	await_done(dbenv3);
	CuAssertTrue(ct, rpt->ret == 0);
	CuAssertTrue(ct, rpt->dbt_count == 1);
	check_dbt_string(&rpt->dbt[0], "I wish I were a fish");
	CuAssertTrue(ct, await_condition(two_done, dbenv2, 10));
	CuAssertTrue(ct, rpts.rpt[0].done);
	CuAssertTrue(ct, rpts.rpt[0].dbt_count == 1);
	check_dbt_string(&rpts.rpt[0].dbt[0], "roses may be pink");

	CuAssertTrue(ct, rpts.rpt[1].done);
	CuAssertTrue(ct, rpts.rpt[1].dbt_count == 1);
	check_dbt_string(&rpts.rpt[1].dbt[0], "I think");
	clear_rpt_int(&rpts.rpt[0]);
	clear_rpt_int(&rpts.rpt[1]);

	ch->close(ch, 0);
	sleep(1);		/* wait for "EOF on connection" msg before cleaning, below */
	// This kluge disappears when GM fixes that err msg to become an event

	printf("19. test illegal calls from the msg disp function\n");
	clear_rpt(dbenv3);
	CuAssertTrue(ct, (ret = dbenv3->repmgr_msg_dispatch(dbenv3, msg_disp5, 0)) == 0);
	CuAssertTrue(ct, (ret = dbenv2->repmgr_channel(dbenv2, eid, &ch, 0)) == 0);
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 1, &resp, 0, 0)) == 0);
	await_done(dbenv3);
	rpt = get_rpt(dbenv3);
	CuAssertTrue(ct, rpt->ret == EINVAL);
	CuAssertTrue(ct, rpt->msg_count == 3);
	CuAssertTrue(ct, strncmp(rpt->msg[0], "BDB3660", strlen("BDB3660")) == 0);
	CuAssertTrue(ct, strncmp(rpt->msg[1], "BDB3660", strlen("BDB3660")) == 0);
	CuAssertTrue(ct, strncmp(rpt->msg[2], "BDB3660", strlen("BDB3660")) == 0);
	ch->close(ch, 0);

	free(buffer);

	td(dbenv2);
	td(dbenv3);
#else
	printf("TestChannelFeature is not supported by the build.\n");
#endif /* HAVE_REPLICATION */
	return (0);
}

static int
two_done(ctx)
	void *ctx;
{
	DB_ENV *dbenv = ctx;
	struct reports *rpts = ((struct env_info *)dbenv->app_private)->rpts;

	return (rpts->count == 2 && rpts->rpt[0].done && rpts->rpt[1].done);
}

/* return 1 ("true") for a match, 0 ("false") otherwise */
static int
check_dbt_string(dbt, s)
	DBT *dbt;
	const char *s;
{
	if (dbt->size != strlen(s))
		return (0);
	if (dbt->size == 0)
		return (1);
	return (strcmp((char*)dbt->data, s) == 0);
}

static int
is_started(ctx)
	void *ctx;
{
	DB_ENV *dbenv = ctx;
	DB_REP_STAT *st;
	u_int32_t ans;
	int ret;

	if ((ret = dbenv->rep_stat(dbenv, &st, 0)) != 0) {
		dbenv->err(dbenv, ret, "rep_stat");
		return (0);
	}
	ans = st->st_startup_complete;
	free(st);
	return (ans);
}

static int
await_condition(pred, ctx, limit)
	PRED pred;
	void *ctx;
	long limit;
{
#ifndef _WIN32
	struct timeval t;
#endif
	time_t tim;

	tim = time(NULL) + limit;
	while (time(NULL) < tim) {
		if ((*pred)(ctx))
			return (1);
		// sleep 1/10th of a second at a time
		// (maybe Windows can use select() too, if include Winsock2.h)
#ifdef _WIN32
		Sleep(100);
#else
		t.tv_sec = 0;
		t.tv_usec = 100000;
		select(0, NULL, NULL, NULL, &t);
#endif
	}
	return (0);
}


static void
notify(dbenv, event, unused)
	DB_ENV *dbenv;
	u_int32_t event;
	void *unused;
{
	struct channel_test_globals *g;
	struct env_info *info;

	if (event == DB_EVENT_PANIC) {
		fprintf(stderr, "BDB panic");
		abort();
	} else if (event == DB_EVENT_REP_STARTUPDONE) {
		info = dbenv->app_private;
		g = info->g;
		LOCK_MUTEX(&g->mtx);
		info->startupdone = 1;
		cond_wake(&g->cond);
		UNLOCK_MUTEX(&g->mtx);
	}		
}

static void
msg_disp(dbenv, ch, request, nseg, flags)
	DB_ENV *dbenv;
	DB_CHANNEL *ch;
	DBT *request;
	u_int32_t nseg;
	u_int32_t flags;
{
	CuTest *ct;
	struct report *rpt = get_rpt(dbenv);
	DBT answer, mult[3];
	char *p;
	size_t sz;
	u_int32_t i;
	int ret;

	ct = ((struct env_info *)dbenv->app_private)->g->test;
	CuAssertTrue(ct, nseg < MAX_SEGS);
	for (i = 0; i < nseg; i++) {
		if ((sz = (rpt->dbt[rpt->dbt_count].size = request[i].size)) > 0) {
			CuAssertTrue(ct, (rpt->dbt[rpt->dbt_count].data = malloc(sz)) != NULL);
			memcpy(rpt->dbt[rpt->dbt_count].data,
			    request[i].data, sz);
		} else
			rpt->dbt[rpt->dbt_count].data = NULL;
		rpt->dbt_count++;
	}

	ret = 0;
	if (flags & DB_REPMGR_NEED_RESPONSE) {
		if (nseg == 2) {
			/* Try a multi-segment response. */
			memset(&mult, 0, sizeof(mult));
			p = "roses are red";
			mult[0].data = p;
			mult[0].size = (u_int32_t)strlen(p) + 1;
			p = "violets are blue";
			mult[1].data = p;
			mult[1].size = (u_int32_t)strlen(p) + 1;
			ret = ch->send_msg(ch, &mult[0], 2, 0);
		} else if (nseg == 1) {
			// pretend to ``forget'' to respond
		} else if (nseg == 4) {
			// send a response of zero segments
			ret = ch->send_msg(ch, &answer, 0, 0);
		} else if (nseg == 5) {
			// send a response with a segment of zero length
			memset(&answer, 0, sizeof(answer));
			answer.size = 0;
			ret = ch->send_msg(ch, &answer, 1, 0);

			// TODO: we still need to try this with the DB_MULTIPLE approach too
		} else if (nseg == 6) {
			// patience, ...
			/* Try a multi-segment response. */
			memset(&mult, 0, sizeof(mult));
			p = "roses are red";
			mult[0].data = p;
			mult[0].size = (u_int32_t)strlen(p) + 1;
			p = "violets are blue";
			mult[1].size = 0;
			mult[2].data = p;
			mult[2].size = (u_int32_t)strlen(p) + 1;
			ret = ch->send_msg(ch, &mult[0], 3, 0);
			
		} else {
			memset(&answer, 0, sizeof(answer));
			p = "this is the answer to the request";
			answer.data = p;
			answer.size = (u_int32_t)strlen(p) + 1;
			ret = ch->send_msg(ch, &answer, 1, 0);
		}
	}
	rpt->ret = ret;
	rpt->done = 1;
}

static void
msg_disp2(dbenv, ch, request, nseg, flags)
	DB_ENV *dbenv;
	DB_CHANNEL *ch;
	DBT *request;
	u_int32_t nseg;
	u_int32_t flags;
{
	CuTest *ct;
	struct report *rpt = get_rpt(dbenv);
	DBT answer;
	const char *p;
	char buf[100];
	size_t sz;
	u_int32_t i;
	int ret;

	ct = ((struct env_info *)dbenv->app_private)->g->test;
	CuAssertTrue(ct, nseg < MAX_SEGS);
	for (i = 0; i < nseg; i++) {
		if ((sz = (rpt->dbt[rpt->dbt_count].size = request[i].size)) > 0) {
			CuAssertTrue(ct, (rpt->dbt[rpt->dbt_count].data = malloc(sz)) != NULL);
			memcpy(rpt->dbt[rpt->dbt_count].data,
			    request[i].data, sz);
		} else
			rpt->dbt[rpt->dbt_count].data = NULL;
		rpt->dbt_count++;
	}

	if (flags & DB_REPMGR_NEED_RESPONSE) {
		memset(&answer, 0, sizeof(answer));
		dbenv->get_errpfx(dbenv, &p);
		strncpy(buf, p, sizeof(buf));
		answer.data = buf;
		answer.size = (u_int32_t)strlen(p) + 1;
		if (answer.size > sizeof(buf))
			answer.size = sizeof(buf);
		ret = ch->send_msg(ch, &answer, 1, 0);
	}
	rpt->ret = ret;
	rpt->done = 1;
}

/* Test async replies to (async) messages. */
static void
msg_disp3(dbenv, ch, request, nseg, flags)
	DB_ENV *dbenv;
	DB_CHANNEL *ch;
	DBT *request;
	u_int32_t nseg;
	u_int32_t flags;
{
	CuTest *ct;
	struct report *rpt = get_rpt(dbenv);
	DBT answer;
	char *p;
	size_t sz;
	u_int32_t i;
	int ret;

	ct = ((struct env_info *)dbenv->app_private)->g->test;
	CuAssertTrue(ct, nseg < MAX_SEGS);
	for (i = 0; i < nseg; i++) {
		if ((sz = (rpt->dbt[rpt->dbt_count].size = request[i].size)) > 0) {
			CuAssertTrue(ct, (rpt->dbt[rpt->dbt_count].data = malloc(sz)) != NULL);
			memcpy(rpt->dbt[rpt->dbt_count].data,
			    request[i].data, sz);
		} else
			rpt->dbt[rpt->dbt_count].data = NULL;
		rpt->dbt_count++;
	}

	ret = 0;

	// TODO: test that multiple calls to send_msg are not allowed on a request.
	CuAssertTrue(ct, !(flags & DB_REPMGR_NEED_RESPONSE));

	memset(&answer, 0, sizeof(answer));
	p = "roses may be pink";
	answer.data = p;
	answer.size = (u_int32_t)strlen(p) + 1;
	ret = ch->send_msg(ch, &answer, 1, 0);

	if (ret == 0) {
		p = "I think";
		answer.data = p;
		answer.size = (u_int32_t)strlen(p) + 1;
		ret = ch->send_msg(ch, &answer, 1, 0);
	}
	rpt->ret = ret;
	rpt->done = 1;
}
static void
msg_disp4(dbenv, ch, request, nseg, flags)
	DB_ENV *dbenv;
	DB_CHANNEL *ch;
	DBT *request;
	u_int32_t nseg;
	u_int32_t flags;
{
	CuTest *ct;
	struct reports *rpts = ((struct env_info *)dbenv->app_private)->rpts;
	struct report *rpt;
	size_t sz;
	u_int32_t i;

	ct = ((struct env_info *)dbenv->app_private)->g->test;
	LOCK_MUTEX(&rpts->m);
	rpt = &rpts->rpt[rpts->count++];
	UNLOCK_MUTEX(&rpts->m);


	CuAssertTrue(ct, !(flags & DB_REPMGR_NEED_RESPONSE));
	CuAssertTrue(ct, nseg < MAX_SEGS);
	for (i = 0; i < nseg; i++) {
		if ((sz = (rpt->dbt[rpt->dbt_count].size = request[i].size)) > 0) {
			CuAssertTrue(ct, (rpt->dbt[rpt->dbt_count].data = malloc(sz)) != NULL);
			memcpy(rpt->dbt[rpt->dbt_count].data,
			    request[i].data, sz);
		} else
			rpt->dbt[rpt->dbt_count].data = NULL;
		rpt->dbt_count++;
	}

	rpt->done = 1;
}

static void
msg_disp5(dbenv, ch, request, nseg, flags)
	DB_ENV *dbenv;
	DB_CHANNEL *ch;
	DBT *request;
	u_int32_t nseg;
	u_int32_t flags;
{
	struct report *rpt = get_rpt(dbenv);
	DBT answer;
	u_int8_t buf[100];
	char *p;
	int ret;

	memset(&answer, 0, sizeof(answer));
	answer.flags = DB_DBT_USERMEM;
	answer.ulen = sizeof(buf);
	answer.data = buf;
	if ((ret = ch->set_timeout(ch, 45000000)) != EINVAL ||
	    (ret = ch->close(ch, 0)) != EINVAL)
		rpt->ret = ret;
	else
		rpt->ret = ch->send_request(ch, request, nseg, &answer, 0, 0);

	memset(&answer, 0, sizeof(answer));
	p = "roses may be pink";
	answer.data = p;
	answer.size = (u_int32_t)strlen(p) + 1;
	ret = ch->send_msg(ch, &answer, 1, 0);
	
	rpt->done = 1;
}

static void
test_data_init(dbt, data)
	DBT *dbt;
	char *data;
{
	memset(dbt, 0, sizeof(*dbt));
	dbt->data = data;
	dbt->size = (u_int32_t)strlen(data) + 1;
}

static void
test_zeroes(ch, dest, ct)
	DB_CHANNEL *ch;
	DB_ENV *dest;		/* destination env handle */
	CuTest *ct;
{
	DBT resp;
	DBT rdbts[6];
	struct report *rpt;
	void *pointer, *vp;
	size_t sz;
	int i, ret;

	memset(&resp, 0, sizeof(resp));
	resp.flags = DB_DBT_MALLOC;

#define	DATA0 "Dear kindly judge, your honor"
#define	DATA1 "my parents treat me rough"
#define	DATA2 "with all their marijuana"
#define	DATA3 "they won't give me a puff"
#define	DATA4 "The didn't wanna have me, but somehow I was had"
#define	DATA5 "Leapin' lizards, that's why I'm so bad!"

#undef ADD_TEST_DATA
#define ADD_TEST_DATA(x) do { test_data_init(&rdbts[i++], (x)); } while (0);

	i = 0;
	ADD_TEST_DATA(DATA0);
	ADD_TEST_DATA(DATA1);
	ADD_TEST_DATA(DATA2);
	ADD_TEST_DATA(DATA3);
	ADD_TEST_DATA(DATA4);
	ADD_TEST_DATA(DATA5);
	
	rpt = get_rpt(dest);

	printf("6. send zero-segment request\n");
	clear_rpt(dest);
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 0, &resp, 0, 0)) == 0);
	CuAssertTrue(ct, rpt->dbt_count == 0);
	CuAssertTrue(ct, rpt->msg_count == 0);
	check_dbt_string(&resp, "this is the answer to the request");
	if (resp.data)
		free(resp.data);

	printf("6.a) send request with a zero-length segment (now why would anyone want to do that?)\n");
	clear_rpt(dest);
	rdbts[1].size = 0;
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 3, &resp, 0, 0)) == 0);
	await_done(dest);
	CuAssertTrue(ct, rpt->dbt_count == 3);
	check_dbt_string(&rpt->dbt[0], DATA0);
	check_dbt_string(&rpt->dbt[1], "");
	check_dbt_string(&rpt->dbt[2], DATA2);
	CuAssertTrue(ct, rpt->msg_count == 0);
	check_dbt_string(&resp, "this is the answer to the request");
	if (resp.data)
		free(resp.data);
	i = 1; ADD_TEST_DATA(DATA1); /* restore perturbed test data */

	printf("6.b) get a zero-length response\n");
	clear_rpt(dest);
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 4, &resp, 0, 0)) == 0);
	await_done(dest);
	CuAssertTrue(ct, rpt->dbt_count == 4);
	CuAssertTrue(ct, rpt->msg_count == 0);
	CuAssertTrue(ct, rpt->ret == 0);
	CuAssertTrue(ct, resp.size == 0);
	if (resp.data)
		free(resp.data);

	printf("6.c) get a zero-length response (alternate version)\n");
	clear_rpt(dest);
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 5, &resp, 0, 0)) == 0);
	await_done(dest);
	CuAssertTrue(ct, rpt->dbt_count == 5);
	CuAssertTrue(ct, rpt->msg_count == 0);
	CuAssertTrue(ct, rpt->ret == 0);
	CuAssertTrue(ct, resp.size == 0);
	if (resp.data)
		free(resp.data);

	printf("6.d) get a zero-length response (DB_MULTIPLE, zero segments)\n");
	clear_rpt(dest);
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 4, &resp, 0, DB_MULTIPLE)) == 0);
	await_done(dest);
	CuAssertTrue(ct, rpt->msg_count == 0);
	CuAssertTrue(ct, rpt->ret == 0);
	DB_MULTIPLE_INIT(pointer, &resp);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, pointer == NULL);
	if (resp.data)
		free(resp.data);

	printf("6.e) get a zero-length response (DB_MULTIPLE, a zero-length segment)\n");
	clear_rpt(dest);
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 5, &resp, 0, DB_MULTIPLE)) == 0);
	await_done(dest);
	CuAssertTrue(ct, rpt->msg_count == 0);
	CuAssertTrue(ct, rpt->ret == 0);
	DB_MULTIPLE_INIT(pointer, &resp);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, sz == 0);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, pointer == NULL);
	if (resp.data)
		free(resp.data);

	printf("6.f) get a zero-length response (DB_MULTIPLE, a zero-length segment in the middle)\n");
	clear_rpt(dest);
	CuAssertTrue(ct, (ret = ch->send_request(ch, rdbts, 6, &resp, 0, DB_MULTIPLE)) == 0);
	await_done(dest);
	CuAssertTrue(ct, rpt->msg_count == 0);
	CuAssertTrue(ct, rpt->ret == 0);
	DB_MULTIPLE_INIT(pointer, &resp);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, rpt->ret == 0);
	CuAssertTrue(ct, strcmp((char*)vp, "roses are red") == 0);
	CuAssertTrue(ct, sz == strlen((char*)vp) + 1);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, sz == 0);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, strcmp((char*)vp, "violets are blue") == 0);
	CuAssertTrue(ct, sz == strlen((char*)vp) + 1);
	DB_MULTIPLE_NEXT(pointer, &resp, vp, sz);
	CuAssertTrue(ct, pointer == NULL);
	if (resp.data)
		free(resp.data);
}	

static int get_avail_ports(ports, count)
	u_int *ports;
	int count;
{
/* This function is used only when replication is supported. */
#ifdef HAVE_REPLICATION
	u_int base, port, upper, curport;
	int ret, t_ret, incr, i;
	char buf[20], *rbuf, *str;
	socket_t s;
	ADDRINFO *orig_ai, *ai;
#ifdef _WIN32
#define in_port_t u_short
#ifndef EADDRINUSE
#define EADDRINUSE WSAEADDRINUSE
#endif
	WSADATA wsaData;
	if ((ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		printf("WSAStartup failed with error: %d\n", ret);
		return (ret);
	}
#endif
	base = 30100;
	upper = 65535;

	/* 
	 * It is very convenient to have a very simple mapping between port
	 * numbers and sites. So usually, we search a sequence of ports 
	 * starting from (10 * N + 1). To avoid redundant check on a port,
	 * we set the incr to be times of 10 and just bigger or equal to count.
	 */
	incr = 10 * ((count + 9) / 10);

	/* 
	 * The format for BDBPORTRANGE should be base:upper, 
	 * either of base or upper can be empty, and if empty,
	 * we will use the default value for it.
	 * If no colon, the whole is considered to be base.
	 */
	rbuf = buf;
	if ((ret = __os_getenv(NULL, "BDBPORTRANGE", &rbuf, sizeof(buf))) != 0)
		goto end;
	if (rbuf != NULL && rbuf[0] != '\0') {
		if ((str = strsep(&rbuf, ":")) != NULL && str[0] != '\0')
			base = (u_int)atoi(str);
		if (rbuf != NULL && rbuf[0] != '\0')
			upper = (u_int)atoi(rbuf);
	}

	for (port = base + 1; (port + incr) <= upper; port += incr) {
		curport = port;
		i = incr;

		while (i-- > 0) {
			if ((ret = __repmgr_getaddr(NULL, "localhost", curport,
			    AI_PASSIVE, &orig_ai)) != 0)
				goto end;

			for (ai = orig_ai; ai != NULL; ai = ai->ai_next) {
				if ((s = socket(ai->ai_family, ai->ai_socktype,
				    ai->ai_protocol)) == INVALID_SOCKET)
					continue;

				if (bind(s, ai->ai_addr, 
				    (socklen_t)ai->ai_addrlen) != 0) {
					ret = net_errno;
					(void)closesocket(s);
					s = INVALID_SOCKET;
					continue;
				}

				ret = 0;
				goto clean;
			}
			if (ret == 0)
				ret = net_errno;
clean:
			if (s != INVALID_SOCKET)
				(void)closesocket(s);
			__os_freeaddrinfo(NULL, orig_ai);
			if (ret != 0 && ret != EADDRINUSE)
				goto end;
			else if (ret != 0)
				break;
			curport++;
		}

		/* We've found the port sequence now */
		if (ret == 0) {
			for (i = 0; i < count; i++)
				ports[i] = port + i;
			break;
		}
	}
end:
#ifdef _WIN32
	if (WSACleanup() == SOCKET_ERROR) {
		t_ret = net_errno;
		printf("WSACleanup failed with error: %d\n", t_ret);
		if (ret == 0)
			ret = t_ret;
	}
#endif

	return (ret);
#else
	return (0);
#endif /* HAVE_REPLICATION */
}

