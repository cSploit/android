/*
    dirlimit.c
    Copyright (C) 1999-2001  Petr Vandrovec

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

	1.00  2001, June 3		Petr Vandrovec <vandrove@vc.cvut.cz>
		Created from fileinfo.c demo.

	1.01  2001, July 15		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added option '-l' to set directory space limit.
  
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
	       "-l new_limit      Directory space limit (4KB units)\n"
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
	char* s = NULL;
	unsigned long maxlimit = 0;
	int setlimit = 0;
	
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);

	while ((opt = getopt(argc, argv, "h?p:l:")) != EOF)
	{
		switch (opt)
		{
		case 'p':
			s = optarg;
			break;
		case 'l':
			maxlimit = strtoul(optarg, NULL, 10);
			setlimit = 1;
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
	} else if (setlimit) {
		struct ncp_dos_info limit;
			
		limit.MaximumSpace = maxlimit;
		dserr = ncp_ns_modify_entry_dos_info(conn, NW_NS_DOS, SA_ALL, 
			NCP_DIRSTYLE_NOHANDLE, 0, 0, buffer, len, 
			DM_MAXIMUM_SPACE, &limit);
		if (dserr) {
			fprintf(stderr, "Cannot set directory space limit: %s\n", strnwerror(dserr));
		} else {
			printf("New directory limit set\n");
		}
	} else {
		NWDIR_HANDLE d;
		
		dserr = ncp_ns_alloc_short_dir_handle(conn, NW_NS_DOS, NCP_DIRSTYLE_NOHANDLE,
				0, 0, buffer, len, NCP_ALLOC_PERMANENT, &d, NULL);
		if (dserr) {
			fprintf(stderr, "Cannot obtain directory handle: %s\n", strnwerror(dserr));
		} else {
			NW_LIMIT_LIST x;
			DIR_SPACE_INFO dsi;

			dserr = NWGetDirSpaceLimitList2(conn, d, &x);
			if (dserr) {
				fprintf(stderr, "Cannot obtain dirspace limit list: %s\n", strnwerror(dserr));
			} else {
				size_t c;
				
				printf("%u entries returned\n", x.numEntries);
				for (c = 0; c < x.numEntries; c++) {
					printf("Entry %u: Level: %u\n", c, x.list[c].level);
					printf("  Max:     %u\n", x.list[c].max);
					printf("  Current: %u\n", x.list[c].current);
					printf("  Used:    %u\n", x.list[c].max - x.list[c].current);
				}
			}
			dserr = ncp_get_directory_info(conn, d, &dsi);
			if (dserr) {
				fprintf(stderr, "Cannot obtain directory info: %s\n", strnwerror(dserr));
			} else {
				printf("Total blocks:      %u\n", dsi.totalBlocks);
				printf("Available blocks:  %u\n", dsi.availableBlocks);
				printf("Purgeable blocks:  %u\n", dsi.purgeableBlocks);
				printf("Not yet purg.b.:   %u\n", dsi.notYetPurgeableBlocks);
				printf("Total inodes:      %u\n", dsi.totalDirEntries);
				printf("Available inodes:  %u\n", dsi.availableDirEntries);
				printf("Reserved:          %u\n", dsi.reserved);
				printf("Sectors Per Block: %u\n", dsi.sectorsPerBlock);
				printf("Volume name:       %s\n", dsi.volName);
			}
		}
		ncp_dealloc_dir_handle(conn, d);
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
