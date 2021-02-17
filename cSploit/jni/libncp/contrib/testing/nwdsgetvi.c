/*
    nwdsgetvi.c - NWDSGetDSVerInfo, NWDSGetBinderyContext API demo
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

	1.00  2000, April 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.
		
	1.01  2000, May 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSGetBinderyContext demo.

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
	         "usage: %s [options] path\n"), progname);
	printf(_("\n"
	       "-h                Print this help text\n"
	       "\n"));
}
			
int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	int opt;
	nuint32 dsv;
	nuint32 rme;
	nuint32 flags;
	char sapname[MAX_TREE_NAME_CHARS+1];
	wchar_t treename[MAX_TREE_NAME_CHARS+1];
	char bindctx[MAX_DN_BYTES];
	NWDSContextHandle ctx;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);

	while ((opt = getopt(argc, argv, "h?")) != EOF)
	{
		switch (opt)
		{
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

	dserr = NWDSGetDSVerInfo(conn, &dsv, &rme, sapname, &flags, treename);
	if (dserr)
		fprintf(stderr, "Cannot retrieve version info: %s (%08X)\n", strnwerror(dserr), dserr);
	else {
		printf("DS version:    %8u\n", dsv);
		printf("RootMostEntry: %8u\n", rme);
		printf("Flags:         %08X\n", flags);
		printf("SAP Tree Name: %s\n", sapname);
	}
	dserr = NWDSCreateContextHandle(&ctx);
	if (dserr)
		fprintf(stderr, "NWDSCreateContextHandle failed with %s\n", strnwerror(dserr));
	else {
		flags = DCV_XLATE_STRINGS;
		NWDSSetContext(ctx, DCK_FLAGS, &flags);
		NWDSAddConnection(ctx, conn);
		dserr = NWDSGetBinderyContext(ctx, conn, bindctx);
		if (dserr)
			fprintf(stderr, "NWDSGetBinderyContext failed with %s\n", strnwerror(dserr));
		else
			printf("Bindery emulation context: %s\n", bindctx);
		NWDSFreeContext(ctx);
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
