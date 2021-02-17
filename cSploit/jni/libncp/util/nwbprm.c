/*
    nwbprm.c - Delete a property of a bindery object on a NetWare server
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
#include <string.h>
#include <ncp/ncplib.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options] [pattern]\n"), progname);
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
	       "-o object_name Name of object\n"
	       "-t type        Object type (decimal value)\n"
	       "-p property    Name of property to be deleted\n"
	       "\n"));
}

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	char *object_name = NULL;
	int object_type = -1;
	char *property_name = NULL;
	long err;

	int result = 1;

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
	while ((opt = getopt(argc, argv, "h?o:t:p:")) != EOF)
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
		case 'p':
			property_name = optarg;
			if (strlen(property_name) > 15)
			{
				fprintf(stderr, _("%s: Property Name too long\n"),
					argv[0]);
				exit(1);
			}
			str_upper(property_name);
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
	if (property_name == NULL)
	{
		fprintf(stderr, _("%s: You must specify a property name\n"),
			argv[0]);
		goto finished;
	}
	if (ncp_delete_property(conn, object_type, object_name,
				property_name) != 0)
	{
		fprintf(stderr, _("%s: Could not delete the property\n"), argv[0]);
	} else
	{
		result = 0;
	}

      finished:
	ncp_close(conn);
	return result;
}
