/*
    dirlist2.c - List the contents of a directory
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
  
 */

#define MAKE_NCPLIB
#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include "../../lib/ncplib_i.h"

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
	       "-n namespace      0=DOS, 1=MAC, 2=NFS, 3=FTAM, 4=OS2\n"
	       "-s search_attr    Search attributes\n"
	       "\n"));
}

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	int opt;
	struct NWCCRootEntry nwccre;
	unsigned int searchattr = SA_ALL;
	u_int32_t destns = NW_NS_DOS;
	int searchmode = 0;
	const char* sstr = "\xFF*";
	size_t sslen = 2;
		
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);
//	NWDSInitRequester();

	while ((opt = getopt(argc, argv, "h?n:s:m:")) != EOF)
	{
		switch (opt)
		{
		case 'n':
			destns = strtoul(optarg, NULL, 0);
			break;
		case 's':
			searchattr = strtoul(optarg, NULL, 0);
			break;
		case 'm':
			searchmode = strtoul(optarg, NULL, 0);
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

	/* NFS and MAC do not handle wildcards :-( */
	if ((destns == NW_NS_NFS) || (destns == NW_NS_MAC)) {
		sstr = NULL;
		sslen = 0;
	}
		
	dserr = ncp_open_mount(argv[optind++], &conn);
	if (dserr) {
		fprintf(stderr, "ncp_open_mount failed: %s\n",
			strnwerror(dserr));
		return 123;
	}
	dserr = NWCCGetConnInfo(conn, NWCC_INFO_ROOT_ENTRY, sizeof(nwccre), &nwccre);
	if (dserr) {
		fprintf(stderr, "Cannot get entry info: %s\n",
			strerror(dserr));
	} else {
		struct nw_info_struct2 nw;

		dserr = ncp_ns_obtain_entry_info(conn, NW_NS_DOS, SA_ALL, NCP_DIRSTYLE_DIRBASE, 
				nwccre.volume, nwccre.dirEnt, NULL, 0, destns, RIM_DIRECTORY, &nw, sizeof(nw));
		if (dserr)
			fprintf(stderr, "Cannot convert DOS entry to %u-NS entry: %s (%08X)\n", destns, strnwerror(dserr), dserr);
		else {
			NWDIRLIST_HANDLE handle;
			
			dserr = ncp_ns_search_init(conn, destns, searchattr, 1, nw.Directory.volNumber, nw.Directory.dirEntNum, NULL, 0,
					0, sstr, sslen, RIM_NAME, &handle);
			if (dserr)
				fprintf(stderr, "Initialize search failed: %s (%08X)\n", strnwerror(dserr), dserr);
			else {
				struct nw_info_struct2 i;
				while ((dserr = ncp_ns_search_next(handle, &i, sizeof(i))) == 0) {
					printf("Name: %s\n", i.Name.Name);
				}
				ncp_ns_search_end(handle);
			}
		}
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
