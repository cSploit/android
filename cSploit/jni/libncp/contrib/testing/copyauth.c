/*
    copyauth.c - Copies authentication information from one connection to another
    Copyright (C) 1999  Petr Vandrovec

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

	0.00  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

	1.01  2000, May 24		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changed to use NWLogoutFromFileServer.  
 */

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <string.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s source_directory dest_directory\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
	         "usage: %s source_directory dest_directory\n"), progname);
	printf(_("\n"
	       "-h                Print this help text\n"
	       "source_directory  Contains authentication information\n"
	       "dest_directory    ncpfs to be authenticated (must point to same tree)\n"
	       "\n"));
}

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWDSContextHandle ctx;
	NWCONN_HANDLE conn;
	NWCONN_HANDLE conn2;
	int opt;
	char* src;
	char* dst;
	
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);
	NWDSInitRequester();
	
	dserr = NWDSCreateContextHandle(&ctx);
	if (dserr) {
		fprintf(stderr, "NWDSCretateContextHandle failed with %s\n", strnwerror(dserr));
		return 123;
	}
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
	if (optind + 2 < argc) {
		usage();
		goto finished;
	}
	src = argv[optind++];
	dst = argv[optind++];

	dserr = ncp_open_mount(src, &conn);
	if (dserr) {
		fprintf(stderr, "ncp_open_mount(src) failed with %s\n", strnwerror(dserr));
		return 122;
	}
	NWDSAddConnection(ctx, conn);
	dserr = ncp_open_mount(dst, &conn2);
	if (dserr) {
		fprintf(stderr, "ncp_open_mount(dst) failed with %s\n", strnwerror(dserr));
		return 123;
	}
	/* We must do logout. Otherwise our code simple
	   ignores call to NWDSAuthenticateConn because
	   of connection is already authenticated... */
	NWLogoutFromFileServer(conn2);
	dserr = NWDSAuthenticateConn(ctx, conn2);
	if (dserr) {
		fprintf(stderr, "NWDSAuthenticateConn failed with %s\n", strnwerror(dserr));
	} else
		printf("OK!\n");
	ncp_close(conn);
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 121;
	}
finished:
	return 0;
}
	
