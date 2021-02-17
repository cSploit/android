/*
    reloadds.c - NWDSReloadDS() API demo
    Copyright (C) 2000  Petr Vandrovec

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

	1.00  2000, May 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.
		
 */

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <errno.h>
#include <string.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options]\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
	         "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-S mount_point    Mount point\n"
	       "-o server_name    Server which needs reload DS\n"
	       "-h                Print this help text\n"
	       "\n"));
}
			
int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	int opt;
	nuint32 flags;
	NWDSContextHandle ctx;
	const char* serverName = "CDROM";
	const char* objectName = "does.not.exist";

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);

	while ((opt = getopt(argc, argv, "h?o:S:")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			goto finished;
		case 'S':
			serverName = optarg;
			break;
		case 'o':
			objectName = optarg;
			break;
		default:
			usage();
			goto finished;
		}
	}

	dserr = ncp_open_mount(serverName, &conn);
	if (dserr) {
		fprintf(stderr, "ncp_open_mount failed: %s\n",
			strnwerror(dserr));
		return 123;
	}

	dserr = NWDSCreateContextHandle(&ctx);
	if (dserr)
		fprintf(stderr, "NWDSCreateContextHandle failed with %s\n", strnwerror(dserr));
	else {
		flags = DCV_XLATE_STRINGS;
		NWDSSetContext(ctx, DCK_FLAGS, &flags);
		NWDSAddConnection(ctx, conn);
		dserr = NWDSReloadDS(ctx, objectName);
		if (dserr)
			fprintf(stderr, "NWDSReloadDS failed with %s\n", strnwerror(dserr));
		else
			printf("NWDSReloadDS suceeded.\n");
		NWDSFreeContext(ctx);
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
