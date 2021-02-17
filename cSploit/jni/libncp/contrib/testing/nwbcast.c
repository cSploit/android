/*
    nwbcast.c - NWGetBroadcastMode, NWSetBroadcastMode demo
	      
    Copyright (C) 2001  Petr Vandrovec

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

	1.00  2001, February 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

 */

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <ncp/eas.h>

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options] path\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
	         "Display broadcast state:\n"
	         "usage: %s path\n"), progname);
	printf(_("\n"
	         "Set broadcast state:\n"
	         "usage: %s -m mode path\n"), progname);
	printf(_("\n"
	         "-h                Print this help text\n"
	         "-m mode           Set this broadcast state\n"
	         "\n"));
}

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	int opt;
	int mode = -1;
	int act = 0;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);

	while ((opt = getopt(argc, argv, "h?m:")) != EOF)
	{
		switch (opt)
		{
		case 'm':
			mode = strtoul(optarg, NULL, 0);
			act = 1;
			break;
		case 'h':
		case '?':
			help();
			goto finished;
		default:
			usage();
			goto finished;
		}
	}

	dserr = ncp_open_mount(argv[optind++], &conn);
	if (dserr) {
		fprintf(stderr, "ncp_open_mount failed: %s\n",
			strnwerror(dserr));
		return 123;
	}
	if (act) {
		dserr = NWSetBroadcastMode(conn, mode);
		if (dserr)
			fprintf(stderr, "NWSetBroadcastMode failed with %s\n", strnwerror(dserr));
		else
			printf("NWSetBroadcastMode suceeded\n");
	} else {
		nuint16 m;

		dserr = NWGetBroadcastMode(conn, &m);
		if (dserr)
			fprintf(stderr, "NWGetBroadcastMode failed with %s\n", strnwerror(dserr));
		else {
			const char* p;
			
			switch(m) {
				case NWCC_BCAST_PERMIT_ALL:	p = "all allowed"; break;
				case NWCC_BCAST_PERMIT_NONE:	p = "none allowed"; break;
				case NWCC_BCAST_PERMIT_SYSTEM:	p = "system allowed"; break;
				case NWCC_BCAST_PERMIT_POLL:	p = "polling enabled"; break;
				default:			p = "unknown"; break;
			}
			printf("NWGetBroadcastMode says that current mode is %s (%d)\n", p, m);
			
		}
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
