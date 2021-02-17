/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2011, 2014 Oracle and/or its affiliates.  All rights reserved.
 * $Id$
 */

/* User-specified role an environment should play in the replication group. */
typedef enum { MASTER, CLIENT, UNKNOWN } ENV_ROLE;

/* User-specified information about a replication site. */
typedef struct {
	char *host;		/* Host name. */
	u_int32_t port;		/* Port on which to connect to this site. */
	int peer;		/* Whether remote site is repmgr peer. */
	int creator;		/* Whether local site is group creator. */
} repsite_t;

/* Data used for common replication setup. */
typedef struct {
	const char *progname;
	char *home;
	int nsites;
	int remotesites;
	ENV_ROLE role;
	repsite_t self;
	repsite_t *site_list;
} SETUP_DATA;

/* Operational commands for the program. */
#define	REPCMD_INVALID	0
#define	REPCMD_EXIT	1
#define	REPCMD_GET	2
#define	REPCMD_HELP	3
#define	REPCMD_PRINT	4
#define	REPCMD_PUT	5	/* Put on master and return. */
#define	REPCMD_PUTSYNC	6	/* Put on master and wait for it
				   to be applied locally. */

/*
 * Max args for a command.  Although the API itself does not have
 * a maximum, it is simpler for the example code to just have space
 * allocated ahead of time.  We use an odd number so that we can
 * have the header DBT plus some number of key/data pairs.
 */
#define	REPCMD_MAX_DBT	33

/* Reply commands. */
#define	REPREPLY_ERROR	1
#define	REPREPLY_OK	0

#define	REPLY_ERROR_NDBT    2
#define	REPLY_TOKEN_TIMEOUT	5000000	/* 5 seconds, arbitrary */

/*
 * General program defines
 */
#define	BUFSIZE 1024
#define	CACHESIZE	(10 * 1024 * 1024)
#define	DATABASE	"quote.db"

extern const char *progname;

/* Portability macros for basic threading & timing */
#ifdef _WIN32
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#define	snprintf		_snprintf
#define	sleep(s)		Sleep(1000 * (s))

extern int getopt(int, char * const *, const char *);

typedef HANDLE thread_t;
#define	thread_create(thrp, attr, func, arg)				   \
    (((*(thrp) = CreateThread(NULL, 0,					   \
	(LPTHREAD_START_ROUTINE)(func), (arg), 0, NULL)) == NULL) ? -1 : 0)
#define	thread_join(thr, statusp)					   \
    ((WaitForSingleObject((thr), INFINITE) == WAIT_OBJECT_0) &&		   \
    GetExitCodeThread((thr), (LPDWORD)(statusp)) ? 0 : -1)

typedef HANDLE mutex_t;
#define	mutex_init(m, attr)                                                \
    (((*(m) = CreateMutex(NULL, FALSE, NULL)) != NULL) ? 0 : -1)
#define	mutex_lock(m)                                                      \
    ((WaitForSingleObject(*(m), INFINITE) == WAIT_OBJECT_0) ? 0 : -1)
#define	mutex_unlock(m)         (ReleaseMutex(*(m)) ? 0 : -1)

#else /* !_WIN32 */
#include <sys/time.h>
#include <pthread.h>

typedef pthread_t thread_t;
#define	thread_create(thrp, attr, func, arg)				   \
    pthread_create((thrp), (attr), (func), (arg))
#define	thread_join(thr, statusp) pthread_join((thr), (statusp))

typedef pthread_mutex_t mutex_t;
#define mutex_init(m, attr)     pthread_mutex_init((m), (attr))
#define mutex_lock(m)           pthread_mutex_lock(m)
#define mutex_unlock(m)         pthread_mutex_unlock(m)

#endif

/* Global data. */
typedef struct {
	DB_ENV *dbenv;
	mutex_t mutex;
	int is_master;
	int app_finished;
	int in_client_sync;
	DB_CHANNEL *channel;
	DB *dbp;
} GLOBAL;

void *checkpoint_thread __P((void *));
int create_env __P((const char *, DB_ENV **));
int env_init __P((DB_ENV *, const char *));
void event_callback __P((DB_ENV *, u_int32_t, void *));
int finish_support_threads __P((thread_t *, thread_t *));
void *log_archive_thread __P((void *));
int open_dbp __P((DB_ENV *, int, DB **));
int parse_cmd __P((char *));
void print_cmdhelp __P(());
void print_one __P((void *, u_int32_t, void *, u_int32_t, int));
int print_stocks __P((DB *));
int rep_setup __P((DB_ENV *, int, char **, SETUP_DATA *));
int start_support_threads __P((DB_ENV *, GLOBAL *, thread_t *,
    thread_t *));
void usage __P((const char *));
