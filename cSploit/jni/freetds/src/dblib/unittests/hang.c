/* 
 * Purpose: Test for dbsqlexec on closed connection
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

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif /* HAVE_SYS_IOCTL_H */

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NET_INET_IN_H */

#include <freetds/sysdep_private.h>

static char software_version[] = "$Id: hang.c,v 1.3 2008-11-25 22:58:29 jklowden Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

char *UNITTEST;

#if HAVE_FSTAT && defined(S_IFSOCK)

static int end_socket = -1;

static int
shutdown_last_socket(void)
{
	int max_socket = -1, i;
	int sockets[2];

	for (i = 3; i < 1024; ++i) {
		struct stat file_stat;
		if (fstat(i, &file_stat))
			continue;
		if ((file_stat.st_mode & S_IFSOCK) == S_IFSOCK) {
			union {
				struct sockaddr sa;
				char data[256];
			} u;
			SOCKLEN_T addrlen;

			addrlen = sizeof(u);
			if (tds_getsockname(i, &u.sa, &addrlen) >= 0 && (u.sa.sa_family == AF_INET || u.sa.sa_family == AF_INET6))
				max_socket = i;
		}
	}
	if (max_socket < 0)
		return 0;

	/* replace socket with a new one */
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
		return 0;

	/* substitute socket */
	close(max_socket);
	dup2(sockets[0], max_socket);

	/* close connection */
	close(sockets[0]);
	end_socket = sockets[1];
	return 1;
}

static int
test(int close_socket)
{
	LOGINREC *login;
	DBPROCESS *dbproc;
	RETCODE ret;
	int expected_error = -1;

	printf("Starting %s\n", UNITTEST);

	dbinit();

	dberrhandle(syb_err_handler);
	dbmsghandle(syb_msg_handler);

	printf("About to logon\n");

	login = dblogin();
	DBSETLPWD(login, PASSWORD);
	DBSETLUSER(login, USER);
	DBSETLAPP(login, "t0020");

	printf("About to open\n");

	dbproc = dbopen(login, SERVER);
	dbsetuserdata(dbproc, (BYTE*) &expected_error);
	if (strlen(DATABASE))
		dbuse(dbproc, DATABASE);
	dbloginfree(login);

	dbcmd(dbproc, "select * from sysobjects");
	printf("dbsqlexec should not hang.\n");

	ret = dbsettime(15);
	if (ret != SUCCEED) {
		fprintf(stderr, "Failed.  Error setting timeout.\n");
		return 1;
	}

	if (!shutdown_last_socket()) {
		fprintf(stderr, "Error shutting down connection\n");
		return 1;
	}
	if (close_socket)
		close(end_socket);

	alarm(20);
	expected_error = close_socket ? 20006 : 20003;
	ret = dbsqlexec(dbproc);
	alarm(0);
	if (ret != FAIL) {
		fprintf(stderr, "Failed.  Expected FAIL to be returned.\n");
		return 1;
	}

	dbsetuserdata(dbproc, NULL);
	if (!close_socket)
		close(end_socket);
	dbexit();

	printf("dblib okay on %s\n", __FILE__);
	return 0;
}

int
main(int argc, char **argv)
{
	UNITTEST = argv[0];
	read_login_info(argc, argv);
	if (test(0) || test(1))
		return 1;
	return 0;
}

#else
int
main(void)
{
	fprintf(stderr, "Not possible for this platform.\n");
	return 0;
}
#endif

