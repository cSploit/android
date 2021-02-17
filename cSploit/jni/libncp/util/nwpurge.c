/*
    nwpurge.c - Utility for purging deleted files from NetWare volumes
    Copyright (c) 1998, 1999 Petr Vandrovec

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

	0.00  1998			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
 */

#include <stdlib.h>
#include <ncp/nwcalls.h>
#include <unistd.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static void usage(void) {
	printf(_(
"usage: nwpurge [options] [directory]\n"
"\n"
"-h         Print this help text\n"
"-a         Purge files in subdirectories\n"
"-l         Do not purge files\n"
"-s         Silent mode\n"
"\n"
"directory  Directory to purge\n"
"\n"
));
}

static int subdirs = 0;
static int show = 1;
static int purge = 1;
static int files = 0;
static int failures = 0;

static void processpurge(struct ncp_conn* conn, int volume, u_int32_t directory_id) {
	struct ncp_deleted_file info;
	char tmp[1024];

	if (show) {
		if (!ncp_ns_get_full_name(conn, NW_NS_DOS, NW_NS_DOS,
			1, volume, directory_id, NULL, 0,
			tmp, sizeof(tmp))) {
			printf("%s\n", tmp);
		}
	}

	info.seq = -1;
	while (!ncp_ns_scan_salvageable_file(conn, NW_NS_DOS, 
			1, volume, directory_id, NULL, 0, 
			&info, tmp, sizeof(tmp))) {
		if (show) {
			printf("%8s%s\n", "", tmp);
		}
		files++;
		if (purge) {
			NWCCODE err;
			if ((err = ncp_ns_purge_file(conn, &info)) != 0) {
				failures++;
				if (!show) {
					printf("%8s%s\n", "", tmp);
				}
				printf(_("%8s-- failed (%s)\n"), "", strnwerror(err));
			}
		}
	}
	if (show) printf("\n");
	if (subdirs) {
		NWDIRLIST_HANDLE dh;

		if (!ncp_ns_search_init(conn, NW_NS_DOS, SA_SUBDIR_ALL, NCP_DIRSTYLE_DIRBASE, volume, directory_id,
				NULL, 0, 0, "\xFF*", 2, RIM_DIRECTORY, &dh)) {
			struct nw_info_struct2 dir;
			
			while (!ncp_ns_search_next(dh, &dir, sizeof(dir))) {
				processpurge(conn, dir.Directory.volNumber, dir.Directory.DosDirNum);
			}
			ncp_ns_search_end(dh);
		}
	}
}

int main(int argc, char* argv[]) {
	struct NWCCRootEntry root;
	const char* path;
	struct ncp_conn* conn;
	int err;
	int c;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	while ((c = getopt(argc, argv, "ahls")) != -1) {
		switch (c) {
			case '?':
			case ':':break;
			case 'a':subdirs = 1;
				 break;
			case 'h':usage(); exit(2);
			case 'l':purge = 0;
				 break;
			case 's':show = 0;
				 break;
			default: fprintf(stderr, _("Unexpected option `-%c'\n"), c);
				 break;
		}
	}
	if (optind < argc) {
		path = argv[optind++];
	} else {
		path = ".";
	}
	err = ncp_open_mount(path, &conn);
	if (err) {
		com_err("nwpurge", err, _("in ncp_open_mount"));
		exit(1);
	}
	err = NWCCGetConnInfo(conn, NWCC_INFO_ROOT_ENTRY, sizeof(root), &root);
	if (err) {
		com_err("nwpurge", err, _("when retrieving root entry"));
		goto finished;
	}
	processpurge(conn, root.volume, root.dirEnt);

	files = files-failures;
	if (!files) {
		printf(_("No files were purged from server.\n"));
	} else if (files == 1) {
		printf(_("1 file was purged from server.\n"));
	} else {
		printf(_("%d files were purged from server.\n"), files);
	}
	if (failures) {
		if (failures == 1) {
			printf(_("1 file was not purged due to error.\n"));
		} else {
			printf(_("%d files were not purged due to errors.\n"), failures);
		}
	}

finished:;
	ncp_close(conn);
	return 0;
}

