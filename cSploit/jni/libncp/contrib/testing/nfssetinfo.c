/*
    nfssetinfo.c - Set informations about file on NFS namespace
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
	       "-f name           Set name\n"
	       "-m mode           Set mode\n"
	       "-g gid            Set gid\n"
	       "-n nlinks         Set n_links\n"
	       "-r rdev           Set rdev\n"
	       "-l link           Set link\n"
	       "-c created        Set created\n"
	       "-u uid            Set uid\n"
	       "-o acsflag        Set acsflag\n"
	       "-y myflag         Set some flag\n"
	       "\n"));
}
			
int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWCONN_HANDLE conn;
	int opt;
	struct NWCCRootEntry nwccre;
	char* name_val = NULL;
	int mode_set = 0;
	int mode_val = 0;
	int gid_set = 0;
	int gid_val = 0;
	int nlinks_set = 0;
	int nlinks_val = 0;
	int rdev_set = 0;
	int rdev_val = 0;
	int link_set = 0;
	int link_val = 0;
	int created_set = 0;
	int created_val = 0;
	int uid_set = 0;
	int uid_val = 0;
	int acsflag_set = 0;
	int acsflag_val = 0;
	int myflag_set = 0;
	int myflag_val = 0;
	
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);
//	NWDSInitRequester();

	while ((opt = getopt(argc, argv, "h?f:m:g:n:r:l:c:u:o:y:")) != EOF)
	{
		switch (opt)
		{
		case 'f':
			name_val = optarg;
			break;
		case 'm':
			mode_set = 1;
			mode_val = strtoul(optarg, NULL, 0);
			break;
		case 'g':
			gid_set = 1;
			gid_val = strtoul(optarg, NULL, 0);
			break;
		case 'n':
			nlinks_set = 1;
			nlinks_val = strtoul(optarg, NULL, 0);
			break;
		case 'r':
			rdev_set = 1;
			rdev_val = strtoul(optarg, NULL, 0);
			break;
		case 'l':
			link_set = 1;
			link_val = strtoul(optarg, NULL, 0);
			break;
		case 'c':
			created_set = 1;
			created_val = strtoul(optarg, NULL, 0);
			break;
		case 'u':
			uid_set = 1;
			uid_val = strtoul(optarg, NULL, 0);
			break;
		case 'o':
			acsflag_set = 1;
			acsflag_val = strtoul(optarg, NULL, 0);
			break;
		case 'y':
			myflag_set = 1;
			myflag_val = strtoul(optarg, NULL, 0);
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
		unsigned char buffer[4000];
		size_t b = 0;
		u_int32_t mask = 0;

		if (name_val) {
			size_t l = strlen(name_val);
			mask |= 0x00000001;
			BSET(buffer, b, l);
			memcpy(buffer+b+1, name_val, l);
			b += l+1;
		}
		if (mode_set) {
			mask |= 0x00000002;
			DSET_LH(buffer, b, mode_val);
			b += 4;
		}
		if (gid_set) {
			mask |= 0x00000004;
			DSET_LH(buffer, b, gid_val);
			b += 4;
		}
		if (nlinks_set) {
			mask |= 0x00000008;
			DSET_LH(buffer, b, nlinks_val);
			b += 4;
		}
		if (rdev_set) {
			mask |= 0x00000010;
			DSET_LH(buffer, b, rdev_val);
			b += 4;
		}
		if (link_set) {
			mask |= 0x00000020;
			BSET(buffer, b, link_val);
			b ++;
		}
		if (created_set) {
			mask |= 0x00000040;
			BSET(buffer, b, created_val);
			b ++;
		}
		if (uid_set) {
			mask |= 0x00000080;
			DSET_LH(buffer, b, uid_val);
			b += 4;
		}
		if (acsflag_set) { 
			mask |= 0x00000100;
			BSET(buffer, b, acsflag_val);
			b ++;
		}
		if (myflag_set) {
			mask |= 0x00000200;
			DSET_LH(buffer, b, myflag_val);
			b += 4;
		}
		ncp_ns_modify_entry_namespace_info(conn,
			NW_NS_DOS, nwccre.volume, nwccre.dirEnt, NW_NS_NFS, mask, buffer, b);
	}
	ncp_close(conn);
finished:;
	return 0;
}
	
