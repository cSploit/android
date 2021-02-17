/*
 * Test connection timeout
 */

#include "common.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if TIME_WITH_SYS_TIME
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# endif
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#if HAVE_POLL_H
#include <poll.h>
#endif /* HAVE_POLL_H */

#include <freetds/tds.h>
#include <freetds/thread.h>
#include "replacements.h"

#if TDS_HAVE_MUTEX

static void init_connect(void);

static void
init_connect(void)
{
	CHKAllocEnv(&odbc_env, "S");
	SQLSetEnvAttr(odbc_env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) (SQL_OV_ODBC3), SQL_IS_UINTEGER);
	CHKAllocConnect(&odbc_conn, "S");
}

static tds_thread fake_thread;
static tds_mutex mtx;
static TDS_SYS_SOCKET fake_sock;

static TDS_THREAD_PROC_DECLARE(fake_thread_proc, arg);

/* build a listening socket to connect to */
static int
init_fake_server(int ip_port)
{
	struct sockaddr_in sin;
	TDS_SYS_SOCKET s;
	int err;

	memset(&sin, 0, sizeof(sin));
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons((short) ip_port);
	sin.sin_family = AF_INET;

	if (TDS_IS_SOCKET_INVALID(s = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("socket");
		exit(1);
	}
	if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		perror("bind");
		CLOSESOCKET(s);
		return 1;
	}
	listen(s, 5);
	err = tds_thread_create(&fake_thread, fake_thread_proc, int2ptr(s));
	if (err != 0) {
		perror("tds_thread_create");
		exit(1);
	}
	return 0;
}

/* accept a socket and read data as much as you can */
static TDS_THREAD_PROC_DECLARE(fake_thread_proc, arg)
{
	TDS_SYS_SOCKET s = ptr2int(arg), sock;
	socklen_t len;
	char buf[128];
	struct sockaddr_in sin;
	struct pollfd fd;

	memset(&sin, 0, sizeof(sin));
	len = sizeof(sin);

	fd.fd = s;
	fd.events = POLLIN;
	fd.revents = 0;
	if (poll(&fd, 1, 30000) <= 0) {
		perror("poll");
		exit(1);
	}

	if (TDS_IS_SOCKET_INVALID(sock = tds_accept(s, (struct sockaddr *) &sin, &len))) {
		perror("accept");
		exit(1);
	}
	tds_mutex_lock(&mtx);
	fake_sock = sock;
	tds_mutex_unlock(&mtx);
	CLOSESOCKET(s);

	for (;;) {
		int len;

		fd.fd = sock;
		fd.events = POLLIN;
		fd.revents = 0;
		if (poll(&fd, 1, 30000) <= 0) {
			perror("poll");
			exit(1);
		}

		/* just read and discard */
		len = READSOCKET(sock, buf, sizeof(buf));
		if (len == 0)
			break;
		if (len < 0 && sock_errno != TDSSOCK_EINPROGRESS)
			break;
	}
	return NULL;
}

int
main(int argc, char *argv[])
{
	SQLTCHAR tmp[2048];
	char conn[128];
	SQLTCHAR sqlstate[6];
	SQLSMALLINT len;
	int port;
	time_t start_time, end_time;

#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);
#endif

	if (tds_mutex_init(&mtx))
		return 1;

	if (odbc_read_login_info())
		exit(1);

	/*
	 * prepare our odbcinst.ini 
	 * is better to do it before connect cause uniODBC cache INIs
	 * the name must be odbcinst.ini cause unixODBC accept only this name
	 */
	if (odbc_driver[0]) {
		FILE *f = fopen("odbcinst.ini", "w");

		if (f) {
			fprintf(f, "[FreeTDS]\nDriver = %s\n", odbc_driver);
			fclose(f);
			/* force iODBC */
			setenv("ODBCINSTINI", "./odbcinst.ini", 1);
			setenv("SYSODBCINSTINI", "./odbcinst.ini", 1);
			/* force unixODBC (only directory) */
			setenv("ODBCSYSINI", ".", 1);
		}
	}

	for (port = 12340; port < 12350; ++port)
		if (!init_fake_server(port))
			break;
	if (port == 12350) {
		fprintf(stderr, "Cannot bind to a port\n");
		return 1;
	}
	printf("Fake server binded at port %d\n", port);

	init_connect();
	CHKSetConnectAttr(SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER) 10, sizeof(SQLINTEGER), "SI");
	CHKSetConnectAttr(SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER) 10, sizeof(SQLINTEGER), "SI");

	/* this is expected to work with unixODBC */
	printf("try to connect to our port just to check connection timeout\n");
	sprintf(conn, "DRIVER=FreeTDS;SERVER=127.0.0.1;Port=%d;TDS_Version=7.0;UID=test;PWD=test;DATABASE=tempdb;", port);
	start_time = time(NULL);
	CHKDriverConnect(NULL, T(conn), SQL_NTS, tmp, ODBC_VECTOR_SIZE(tmp), &len, SQL_DRIVER_NOPROMPT, "E");
	end_time = time(NULL);

	memset(sqlstate, 'X', sizeof(sqlstate));
	tmp[0] = 0;
	CHKGetDiagRec(SQL_HANDLE_DBC, odbc_conn, 1, sqlstate, NULL, tmp, ODBC_VECTOR_SIZE(tmp), NULL, "SI");
	odbc_disconnect();
	tds_mutex_lock(&mtx);
	CLOSESOCKET(fake_sock);
	tds_mutex_unlock(&mtx);
	tds_thread_join(fake_thread, NULL);

	printf("Message: %s - %s\n", C(sqlstate), C(tmp));
	if (strcmp(C(sqlstate), "HYT00") || !strstr(C(tmp), "Timeout")) {
		fprintf(stderr, "Invalid timeout message\n");
		return 1;
	}
	if (end_time - start_time < 10 || end_time - start_time > 16) {
		fprintf(stderr, "Unexpected connect timeout (%d)\n", (int) (end_time - start_time));
		return 1;
	}

	printf("Done.\n");
	ODBC_FREE();
	return 0;
}

#else	/* !TDS_HAVE_MUTEX */
int
main(void)
{
	printf("Not possible for this platform.\n");
	return 0;
}
#endif
