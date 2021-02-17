/*
    nwfstime.c - get/set a file server's time
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

  	0.01  1999			Frank A. Vorstenbosch <frank@falstaff.demon.co.uk>
		Update local time from NetWare server.
  
	0.02  2000, April 23th		Frank A. Vorstenbosch <frank@falstaff.demon.co.uk>
		Wait for second to roll over for more accurate time.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include <ncp/ncplib.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [pattern]\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
                 "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "-U username    Username sent to server\n"
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "-C             Don't convert password to uppercase\n"
	       "\n"
	       "-a             Be more accurate about reading the time\n"
	       "-g             Update local time from file server\n"
	       "-s             Set file server's time from local time\n"
	       "\n"));
}

static inline size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm) {
	return strftime(s, max, fmt, tm);
}

int
main(int argc, char **argv)
{
	struct ncp_conn *conn;
	int opt;
	long err;
	int set = 0, get = 0, accurate = 0;
	time_t t;
	struct timeval timeval;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	if ((conn = ncp_initialize(&argc, argv, 1, &err)) == NULL)
	{
		com_err(argv[0], err, _("when initializing"));
		return 1;
	}
	while ((opt = getopt(argc, argv, "h?sga")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			break;
		case 's':
			set = 1;
			break;
		case 'g':
			get = 1;
			break;
		case 'a':
			accurate = 1;
			break;
		default:
			usage();
			goto finished;
		}
	}

      finished:

	if (set != 0)
	{
		time(&t);
		if ((err = ncp_set_file_server_time(conn, &t)) != 0)
		{
			com_err(argv[0], err, _("when setting file server time"));
			ncp_close(conn);
			return 1;
		}
	} else
	{	int offset;
		time_t last;

		if ((err = ncp_get_file_server_time(conn, &t)) != 0)
		{
	     get_error: com_err(argv[0], err, _("when getting file server time"));
			ncp_close(conn);
			return 1;
		}

		if(accurate)
		{
			do
			{	if ((err = ncp_get_file_server_time(conn, &last)) != 0)
					goto get_error;
			} while(last==t);
			t=last;
			offset=0;	/* we can read the time 1000s of times a second */
	       	}
		else
			offset=500;	/* if no accurate measure, then assume offset of 500ms */

		if(get)
		{	timeval.tv_sec = t;
			timeval.tv_usec = offset*1000;
			settimeofday(&timeval, NULL);
		}
		{
			char text_server_time[200];
			struct tm* tm;
			
			tm = localtime(&t);
			my_strftime(text_server_time, sizeof(text_server_time), "%c", tm);
			printf("%s\n", text_server_time);
		}
	}

	ncp_close(conn);
	return 0;
}
