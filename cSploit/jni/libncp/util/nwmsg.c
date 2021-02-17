/*
    nwmsg.c - Fetch NetWare broadcast messages and write to the user
    Copyright (C) 1996 by Volker Lendecke
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


    Revision history:

	0.00  1996			Volker Lendecke
		Initial revision.

	0.01  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Checking for 'mesg n' on all terminals.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
  
 */

#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <paths.h>
#include <utmp.h>
#include <mntent.h>
#include <string.h>
#include <errno.h>
#include <ncp/nwcalls.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static int search_utmp(const char *user, char *tty);

static char *progname;

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	char message[256];
	char *mount_point;
	uid_t uid;
	struct passwd *pwd;
	char tty_path[256];
	FILE *tty_file;
	long err;
	char sn[256];

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);

	progname = argv[0];

	openlog("nwmsg", LOG_PID, LOG_LPR);

	if (argc != 2)
	{
		fprintf(stderr, _("usage: %s mount-point\n"),
			progname);
		exit(1);
	}
	mount_point = argv[1];
	if ((err = ncp_open_mount(mount_point, &conn)) != 0)
	{
		com_err(progname, err, _("in ncp_open_mount"));
		exit(1);
	}
	if (ncp_get_broadcast_message(conn, message) != 0)
	{
		fprintf(stderr, _("%s: could not get broadcast message\n"),
			progname);
		ncp_close(conn);
		exit(1);
	}
	if (strlen(message) == 0)
	{
		ncp_close(conn);
		syslog(LOG_DEBUG, _("no message"));
		exit(0);
	}
#if 0
	syslog(LOG_DEBUG, _("message: %s"), message);
#endif

	if (NWCCGetConnInfo(conn, NWCC_INFO_MOUNT_UID, sizeof(uid), &uid)) {
		fprintf(stderr, _("%s: could not get uid on connection: %s\n"),
			progname, strerror(errno));
		ncp_close(conn);
		exit(1);
	}
	if (NWCCGetConnInfo(conn, NWCC_INFO_SERVER_NAME, sizeof(sn), sn)) {
		strcpy(sn, _("<unknown>"));
	}
	ncp_close(conn);

	if ((pwd = getpwuid(uid)) == NULL)
	{
		fprintf(stderr, _("%s: user %d not known\n"),
			progname, uid);
		exit(1);
	}
	if (search_utmp(pwd->pw_name, tty_path) != 0)
	{
		exit(1);
	}
	if ((tty_file = fopen(tty_path, "w")) == NULL)
	{
		fprintf(stderr, _("%s: cannot open %s: %s\n"),
			progname, tty_path, strerror(errno));
		exit(1);
	}
	fprintf(tty_file, _("\r\n\007\007\007Message from NetWare Server: %s\r\n%s\r\n"),
		sn, message);
	fclose(tty_file);
	return 0;
}

/* The following routines have been taken from util-linux-2.5's write.c */

/*
 * term_chk - check that a terminal exists, and get the message bit
 *     and the access time
 */
static int
term_chk(const char *path, int *msgsokP, time_t * atimeP, int *showerror)
{
	struct stat s;

	if (stat(path, &s) < 0)
	{
		if (showerror)
			(void) fprintf(stderr,
				    _("write: %s: %s\n"), path, strerror(errno));
		return (1);
	}
	*msgsokP = (s.st_mode & (S_IWRITE >> 3)) != 0;	/* group write bit */
	*atimeP = s.st_atime;
	return (0);
}

/*
 * search_utmp - search utmp for the "best" terminal to write to
 *
 * Ignores terminals with messages disabled, and of the rest, returns
 * the one with the most recent access time.  Returns as value the number
 * of the user's terminals with messages enabled, or -1 if the user is
 * not logged in at all.
 *
 * Special case for writing to yourself - ignore the terminal you're
 * writing from, unless that's the only terminal with messages enabled.
 */
static int
search_utmp(const char *user, char *tty)
{
	struct utmp u;
	time_t bestatime, atime;
	int ufd, nloggedttys, nttys, msgsok;

	char atty[5 + sizeof(u.ut_line) + 1];

	if ((ufd = open(_PATH_UTMP, O_RDONLY)) < 0)
	{
		perror(_("utmp"));
		return -1;
	}
	nloggedttys = nttys = 0;
	bestatime = 0;
	memcpy(atty, "/dev/", 5);
	while (read(ufd, (char *) &u, sizeof(u)) == sizeof(u))
		if (strncmp(user, u.ut_name, sizeof(u.ut_name)) == 0)
		{
			++nloggedttys;

			strncpy(atty + 5, u.ut_line, sizeof(u.ut_line));
			atty[5 + sizeof(u.ut_line)] = '\0';

			if (u.ut_type != USER_PROCESS)
				continue;	/* it's not a valid entry */
			if (term_chk(atty, &msgsok, &atime, 0))
				continue;	/* bad term? skip */
			if (!msgsok)
				continue;	/* skip ttys with msgs off */

			++nttys;
			if (atime > bestatime)
			{
				bestatime = atime;
				(void) strcpy(tty, atty);
			}
		}
	(void) close(ufd);
	if (nloggedttys == 0)
	{
		(void) fprintf(stderr, _("nwmsg: %s is not logged in\n"), user);
		return -1;
	}
	if (!bestatime)
	{
		(void) fprintf(stderr, _("nwmsg: %s ignores messages\n"), user);
		return -1;
	}
	return 0;
}
