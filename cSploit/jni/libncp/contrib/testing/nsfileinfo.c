/*
    nsfileinfo.c - Dumps informations about file on specified namespace
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
	       "-m requestedInfo  Requested flags\n"
	       "-n namespace      Destination Namespace\n"
	       "-w                Execute write-attrs request (DO NOT USE!)\n"
	       "\n"));
}
			
static void printstruct2(struct ncp_namespace_format* info) {
	int i;
	
	printf("Bits: Fixed:    %08X, count %d\n", info->BitMask.fixed,    info->BitsDefined.fixed);
	printf("Bits: Variable: %08X, count %d\n", info->BitMask.variable, info->BitsDefined.variable);
	printf("Bits: Huge:     %08X, count %d\n", info->BitMask.huge,     info->BitsDefined.huge);
	for (i = 0; i < 32; i++) {
		int prt = 0;
		int mask = 1 << i;
		
		printf("Field %02X: %d bytes (", i, info->FieldsLength[i]);
		if (info->BitMask.fixed & mask) {
			printf("fixed");
			prt = 1;
		}
		if (info->BitMask.variable & mask) {
			if (prt)
				printf(", ");
			printf("variable");
			prt = 1;
		}
		if (info->BitMask.huge & mask) {
			if (prt)
				printf(", ");
			printf("huge");
			prt = 1;
		}
		if (!prt)
			printf("unused");
		printf(")\n");
	}
}								    
						      
int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	int opt;
	struct NWCCRootEntry nwccre;
	int dowrite = 0;
	u_int32_t destns = NW_NS_DOS;
	u_int32_t mask = ~0;
	
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);
//	NWDSInitRequester();

	while ((opt = getopt(argc, argv, "h?n:m:w")) != EOF)
	{
		switch (opt)
		{
		case 'n':
			destns = strtoul(optarg, NULL, 0);
			break;
		case 'm':
			mask = strtoul(optarg, NULL, 0);
			break;
		case 'w':
			dowrite = 1;
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
	dserr = NWCCGetConnInfo(conn, NWCC_INFO_ROOT_ENTRY, sizeof(nwccre), &nwccre);
	if (dserr) {
		fprintf(stderr, "Cannot get entry info: %s\n",
			strerror(dserr));
	} else {
		struct ncp_namespace_format iii;

		memset(&iii, 0xCC, sizeof(iii));
		dserr = ncp_ns_obtain_namespace_info_format(conn,
			nwccre.volume, destns, &iii, sizeof(iii));
		if (dserr) {
			fprintf(stderr, "Cannot obtain info: %s\n",
				strnwerror(dserr));
		} else {
			unsigned char buffer[4000];
			size_t b;
			
			printstruct2(&iii);
			dserr = ncp_ns_obtain_entry_namespace_info(conn,
				NW_NS_DOS, nwccre.volume, nwccre.dirEnt, destns, mask, buffer, &b, sizeof(buffer));
			if (dserr) {
				fprintf(stderr, "Cannot obtain file info: %s\n",
					strnwerror(dserr));
			} else {
				unsigned int i;

				for (i = 0; i < 32; i++) {
					unsigned char ib[4000];
					size_t il;

					printf("Item #%u: ", i);
					dserr = ncp_ns_get_namespace_info_element(&iii, mask, buffer, b, i, &ib, &il, sizeof(ib));
					if (dserr) {
						printf("error %s\n", strnwerror(dserr));
					} else {
						size_t p;

						for (p = 0; p < il; p++)
							printf("%02X ", ib[p]);
						printf("\n");
					}
				}
				if (dowrite) {
					unsigned char tb[] = {0xFF,0x01,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 3, 0, 0,0,0,0, 1, 0,0,0,0};
					ncp_ns_modify_entry_namespace_info(conn,
						NW_NS_DOS, nwccre.volume, nwccre.dirEnt, destns, 0x3fe, tb, 24+3);
				}
			}
		}
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
