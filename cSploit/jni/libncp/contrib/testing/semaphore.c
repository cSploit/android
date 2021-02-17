/*
    semaphore.c
    Copyright (C) 1999, 2002  Petr Vandrovec

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


    Dumps requested informations about file
    ncp_ns_obtain_entry_info API demo


    Revision history:

	1.00  2002, July 5		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.
  
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
	fprintf(stderr, _("usage: %s [options]\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
	         "usage: %s [options] path\n"), progname);
	printf(_("\n"
	       "-h                Print this help text\n"
	       "-s search_attr    Search attributes\n"
	       "-p semaphore_name Semaphore name\n"
	       "\n"));
}
			
int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	char volume[1000];
	char volpath[1000];
	int opt;
	u_int32_t searchattr = 0x8006;
	const char* s = "TESTSEMAPHORE";
	
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);
//	NWDSInitRequester();

	while ((opt = getopt(argc, argv, "h?s:p:")) != EOF)
	{
		switch (opt)
		{
		case 's':
			searchattr = strtoul(optarg, NULL, 0);
			break;
		case 'p':
			s = optarg;
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

	dserr = NWParsePath(argv[optind++], NULL, &conn, volume, volpath);
	if (dserr) {
		fprintf(stderr, "NWParsePath failed: %s\n",
			strnwerror(dserr));
		return 123;
	}
	if (!conn) {
		fprintf(stderr, "Path is not remote\n");
		return 124;
	} else {
		u_int32_t sh;
		u_int32_t sh2;
		u_int16_t v;
		u_int16_t v2;

		dserr = NWOpenSemaphore(conn, s, 0, &sh, &v);
		printf("Open semaphore done: %s\n", strnwerror(dserr));
		dserr = NWOpenSemaphore(conn, "TS", 0, &sh2, &v2);
		printf("Open semaphore2 done: %s\n", strnwerror(dserr));
		if (!dserr) {
			printf("SH=%08X, V=%u\n", sh, v);
			printf("SH2=%08X, V2=%u\n", sh2, v2);
			dserr = NWWaitOnSemaphore(conn, sh, 1);
			printf("Wait on semaphore done: %s\n", strnwerror(dserr));
			dserr = NWWaitOnSemaphore(conn, sh2, 1);
			printf("Wait on semaphore done: %s\n", strnwerror(dserr));
			dserr = NWSignalSemaphore(conn, sh2);
			printf("Signal semaphore done: %s\n", strnwerror(dserr));
			sleep(1);
			dserr = NWCancelWait(conn);
			printf("Cancel Wait done: %s\n", strnwerror(dserr));
			sleep(1);
//			dserr = NWSignalSemaphore(conn, sh);
//			printf("Signal semaphore done: %s\n", strnwerror(dserr));
			NWCloseSemaphore(conn, sh);
			NWCloseSemaphore(conn, sh2);
			sleep(15);
			dserr = NWCancelWait(conn);
			printf("Cancel Wait done: %s\n", strnwerror(dserr));
		}
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
