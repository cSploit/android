/*
    nwtrustee.c - List Trustees Directory Assignments of an object
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
#include <string.h>
#include <ncp/ncplib.h>

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
	       "-l volumeid    Volume id to be searched\n"
	       "-L volname     Volume name instead of id\n"
	       "-O objectid    Object id\n"
	       "-o objname     Object name ( type must be specified )\n"
	       "-t type        Object type\n"
	       "-v             Verbose listing\n"
	       "\n"));
}

static int
fromhex(char c)
{
	c -= '0';
	if (c > 9)
		c = toupper(c) + '0' - 'A' + 10;
	return c & 15;
}

int
main(int argc, char **argv)
{
	struct ncp_conn *conn;
	struct ncp_bindery_object bobj;

	char ppath[256];
	char *p;
	long err;
	int opt, vid, type = 1;
	int verbose = 0;
	u_int32_t oid = 1;
	u_int16_t nextp, trust;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	if ((conn = ncp_initialize(&argc, argv, 1, &err)) == NULL)
	{
		com_err(argv[0], err, _("when initializing"));
		return 1;
	}
	while ((opt = getopt(argc, argv, "h?vl:L:o:O:t:")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			exit(1);
		case 'l':
			vid = atoi(optarg);
			break;
		case 'L':
			if (ncp_get_volume_number(conn, optarg, &vid))
			{
				ncp_close(conn);
				return 1;
			}
			break;

		case 'O':
			oid = 0;
			while (*optarg)
			{
				oid = (oid << 4) | fromhex(*optarg);
				optarg++;
			}
			break;
		case 'o':
			for (p = optarg; *p != 0; p++)
			{
				*p = toupper(*p);
			}
			if (ncp_get_bindery_object_id(conn, type, optarg, &bobj))
			{
				ncp_close(conn);
				return 1;
			}
			oid = bobj.object_id;
			break;
		case 'v':
			verbose = 1;
			break;
		case 't':
			type = atoi(optarg);
			break;
		default:
			usage();
			exit(1);
		}
	}

	if (optind < argc)
	{
		usage();
		exit(1);
	}
	nextp = 0;
	while (!ncp_get_trustee(conn, oid, vid, ppath, &trust, &nextp))
	{
		if (!nextp)
			break;
		printf("%s ", ppath);
		if (verbose)
		{
			strcpy(ppath, "[ R W O C E A F M ]");
			p = ppath + 2;
			for (type = 1; type < 256; type <<= 1)
			{
				if (!(trust & type))
					*p = ' ';
				p += 2;
			}
			printf("%s\n", ppath);
		} else
		{
			printf("%X\n", trust);
		}
	}
	ncp_close(conn);
	return 0;
}
