/*
    physlock.c
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

	1.00  2002, July 4		Petr Vandrovec <vandrove@vc.cvut.cz>
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
	       "-p volume_path    Remote path (default is derived from path)\n"
	       "\n"));
}
			
int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	char volume[1000];
	char volpath[1000];
	int opt;
	int len;
	unsigned char buffer[1000];
	u_int32_t searchattr = 0x8006;
	char* s = NULL;
	
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
	}
	strcat(volume, ":");
	strcat(volume, volpath);
	len = ncp_path_to_NW_format(s?s:volume, buffer, sizeof(buffer));
	if (len < 0) {
		fprintf(stderr, "Cannot convert path: %s\n",
			strerror(-len));
	} else {
		struct nw_info_struct2 iii;
		u_int8_t fh[10];
		
		dserr =	ncp_ns_open_create_entry(conn, NW_NS_DOS, searchattr, NCP_DIRSTYLE_NOHANDLE, 
			0, 0, buffer, len, -1, OC_MODE_OPEN | OC_MODE_OPEN_64BIT_ACCESS, 
			0, AR_READ_ONLY|AR_WRITE_ONLY, RIM_ALL, 
			&iii, sizeof(iii), NULL, NULL, fh);
		if (dserr) {
			fprintf(stderr, "Cannot open file: %s\n",
				strnwerror(dserr));
		} else {
			unsigned int i;
			
			printf("File handle:");
			for (i = 0; i < 6; i++) {
				printf(" %02X", fh[i]);
			}
			printf("\n");
			
			dserr = ncp_log_physical_record(conn, fh, 20, 10, NCP_PHYSREC_EX, 10);
			printf("Exclusive lock (20,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_release_physical_record(conn, fh, 20, 10);
			printf("Release (20,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_release_physical_record(conn, fh, 20, 10);
			printf("Release (20,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_clear_physical_record(conn, fh, 20, 10);
			printf("Clear (20,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_clear_physical_record(conn, fh, 20, 10);
			printf("Clear (20,10) done (should fail): %s\n", strnwerror(dserr));
			dserr = ncp_log_physical_record(conn, fh, 20, 10, NCP_PHYSREC_SH, 0);
			printf("Shareable lock (20,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_release_physical_record(conn, fh, 20, 10);
			printf("Release (20,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_clear_physical_record(conn, fh, 20, 10);
			printf("Clear (20,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_log_physical_record(conn, fh, 20, 10, NCP_PHYSREC_LOG, 0);
			printf("Logged (20,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_release_physical_record(conn, fh, 20, 10);
			printf("Release (20,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_clear_physical_record(conn, fh, 20, 10);
			printf("Clear (20,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_log_physical_record(conn, fh, 2000000000000ULL, 10, NCP_PHYSREC_EX, 0);
			printf("Exclusive lock (2000000000000,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_release_physical_record(conn, fh, 2000000000000ULL, 10);
			printf("Release (2000000000000,10) done: %s\n", strnwerror(dserr));
			dserr = ncp_clear_physical_record(conn, fh, 2000000000000ULL, 10);
			printf("Clear (2000000000000,10) done: %s\n", strnwerror(dserr));
		}
		ncp_close_file(conn, fh);
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
