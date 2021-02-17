/*
    nwvolinfo.c - Volume Information
    Copyright (C) 1996 by Volker Lendecke
  
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

	0.00  1996			Volker Lendecke
		Initial revision.
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <ncp/nwcalls.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options] pattern\n"), progname);
	return;
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "-U username    Username sent to server\n"
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "-C             Don't convert password to uppercase\n"
	       "\n"
	       "-v volume      volume name\n"
	       "-N             Numeric format\n"
	       "\n"));
}

int
main(int argc, char **argv)
{
	struct ncp_conn *conn;
	struct ncp_volume_info o;

	const char *volname = NULL;
	long err;
	int opt;
	int numk;
	u_int16_t type = 0;
	int useConn = 0;
	char volume[1000];

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	if ((conn = ncp_initialize_2(&argc, argv, 1, NCP_BINDERY_USER, &err, 0)) == NULL)
	{
		useConn = 1;
	}
	while ((opt = getopt(argc, argv, "h?Nv:")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			exit(1);
		case 'N':
			type = 1;
			break;
		case 'v':
			volname = optarg;
			break;
		default:
			usage();
			exit(1);
		}
	}

	if (!useConn) {
		if (optind < argc)
		{
			usage();
			exit(1);
		}
		if (!volname) volname = "SYS";
	} else {
		const char* path;

		if (optind < argc)
			path = argv[optind];
		else
			path = ".";
		if (NWParsePath(path, NULL, &conn, volume, NULL) || !conn) {
			fprintf(stderr, _("%s: %s is not Netware directory/file\n"),
				progname, path);
			return 1;
		}
		if (!volname) volname = volume;
	}

	if (ncp_get_volume_number(conn, volname, &opt))
	{
		fprintf(stderr, _("%s: Volume %s does not exist\n"), progname, volname);
		exit(1);
	}
	if (ncp_get_volume_info_with_number(conn, opt, &o))
	{
		fprintf(stderr, _("%s: Unable to get volume information\n"), progname);
		exit(1);
	}
	numk = o.sectors_per_block / 2;
	o.free_blocks += o.purgeable_blocks;
	if (type)
	{
		printf("%d %d %d %d %d %d\n",
		       o.total_blocks * numk,
		       o.free_blocks * numk,
		       o.purgeable_blocks * numk,
		       o.not_yet_purgeable_blocks * numk,
		       o.total_dir_entries,
		       o.available_dir_entries);
	} else
	{
		printf(_("Total    : %dK\n"), o.total_blocks * numk);
		printf(_("Free     : %dK\n"), o.free_blocks * numk);
		printf(_("Purgable : %dK\n"), o.purgeable_blocks * numk);
		printf(_("No Purg. : %dK\n"), o.not_yet_purgeable_blocks * numk);
		printf(_("Dirs     : %d\n"), o.total_dir_entries);
		printf(_("Free dirs: %d\n"), o.available_dir_entries);
	}


	ncp_close(conn);
	return 0;
}
