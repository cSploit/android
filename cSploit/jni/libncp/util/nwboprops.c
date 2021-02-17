/*
    nwboprops.c - List properties of a bindery object
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

#include <unistd.h>
#include <stdlib.h>
#include <ncp/ncplib.h>

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
	printf("\n");
	printf(_("usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "-U username    Username sent to server\n"
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "-C             Don't convert password to uppercase\n"
	       "\n"
	       "-o object_name Name of object inspected\n"
	       "-t type        Object type (decimal value)\n"
	       "-v             Verbose listing\n"
	       "\n"));
}

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	char *object_name = NULL;
	int object_type = -1;
	long err;

	struct ncp_property_info info;

	int result = 1;
	int verbose = 0;
	int opt;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	if ((conn = ncp_initialize(&argc, argv, 1, &err)) == NULL)
	{
		com_err(argv[0], err, _("when initializing"));
		goto finished;
	}
	while ((opt = getopt(argc, argv, "h?o:t:v")) != EOF)
	{
		switch (opt)
		{
		case 'o':
			object_name = optarg;
			str_upper(object_name);
			break;
		case 't':
			object_type = atoi(optarg);
			break;
		case 'v':
			verbose = 1;
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

	if (object_type < 0)
	{
		fprintf(stderr, _("%s: You must specify an object type\n"),
			argv[0]);
		goto finished;
	}
	if (object_name == NULL)
	{
		fprintf(stderr, _("%s: You must specify an object name\n"),
			argv[0]);
		goto finished;
	}
	info.search_instance = 0xffffffff;

	while (ncp_scan_property(conn, object_type, object_name,
				 info.search_instance, "*", &info) == 0)
	{
		if (verbose != 0)
		{
			printf("%s %d %02x %d\n",
			       info.property_name, info.property_flags,
			       info.property_security,
			       info.value_available_flag);
		} else
		{
			printf("%s\n", info.property_name);
		}
	}

      finished:
	ncp_close(conn);
	return result;
}
