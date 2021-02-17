/*
    nwbpadd.c - Set the contents of a SET property of a bindery object on a NetWare server
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

	1.00  2000, June 1		Bruce Richardson <brichardson@lineone.net>
		Swapped object ID to match with other tools.

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
	fprintf(stderr, _("usage: %s [options]\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
	         "usage: %s [options] [values]\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "-U username    Username sent to server\n"
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "-C             Don't convert password to uppercase\n"
	       "\n"
	       "-o object_name Name of accessed object\n"
	       "-t type        Object type (decimal value)\n"
	       "-p property    Name of property to be touched\n"
	       "value          value to be added\n"
	       "\n"
	       "If property is of type SET, value is an object id (hex)\n"
	       "Otherwise, value is either a string value to be written, or\n"
	       "a count of bytes to be written. The latter is assumed if\n"
	       "more than one value argument is given. The count is decimal,\n"
	       "and the following arguments are interpreted as bytes in\n"
	       "hexadecimal notation.\n"
	       "\n"));
}

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	char *object_name = NULL;
	int object_type = -1;
	char *property_name = NULL;
	char *value = NULL;
	struct ncp_property_info info;
	long err;
	int result = 1;
	int opt;
	int dontohl = 0;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	if ((conn = ncp_initialize(&argc, argv, 1, &err)) == NULL)
	{
		com_err(argv[0], err, _("when initializing"));
		goto finished;
	}
	while ((opt = getopt(argc, argv, "h?o:t:p:v:O")) != EOF)
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
		case 'v':
			value = optarg;
			break;
		case 'O':
			dontohl = 1;
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
	if (optind > argc - 1)
	{
		fprintf(stderr, _("%s: You must specify a property value\n"),
			argv[0]);
		goto finished;
	}
	value = argv[optind];
	optind += 1;

	if (ncp_scan_property(conn, object_type, object_name,
			      0xffffffff, property_name, &info) != 0)
	{
		fprintf(stderr, _("%s: Could not find property\n"), argv[0]);
		goto finished;
	}
	if ((info.property_flags & 2) != 0)
	{
		/* Property is of type SET */
		struct ncp_bindery_object o;
		NWObjectID id;
		
		if (optind != argc)
		{
			fprintf(stderr, _("%s: For the SET property %s, you must"
				" specify an object id as value\n"),
				progname, property_name);
			goto finished;
		}
		id = strtoul(value, NULL, 16);
		if (dontohl)
			id = ntohl(id);
		if (ncp_get_bindery_object_name(conn,
						id,
						&o) != 0)
		{
			fprintf(stderr, _("%s: %s is not a valid object id\n"),
				progname, value);
			goto finished;
		}
		if (ncp_add_object_to_set(conn, object_type, object_name,
					  property_name,
					  o.object_type, o.object_name) != 0)
		{
			fprintf(stderr, _("%s: could not add object %s\n"),
				progname, o.object_name);
			goto finished;
		}
	} else
	{
		/* Property is of type ITEM */
		char contents[255 * 128];
		unsigned int segno;
		size_t length;

		memset(contents, 0, sizeof(contents));

		if (optind == argc)
		{
			/* value is the string to add */
			length = strlen(value);
			if (length >= sizeof(contents))
			{
				fprintf(stderr, _("%s: Value too long\n"),
					progname);
				goto finished;
			}
			strcpy(contents, value);
		} else
		{
			/* value is the byte count */
			int i;
			length = atoi(value);
			if (length >= sizeof(contents))
			{
				fprintf(stderr, _("%s: Value too long\n"),
					progname);
				goto finished;
			}
			if (length != (unsigned)(argc - optind))
			{
				fprintf(stderr, _("%s: Byte count does not match"
					" number of bytes\n"), progname);
				goto finished;
			}
			i = 0;
			while (optind < argc)
			{
				contents[i] = strtol(argv[optind], NULL, 16);
				i += 1;
				optind += 1;
			}
		}

		for (segno = 1; segno <= 255; segno++)
		{
			struct nw_property segment;
			size_t offset = (segno - 1) * 128;

			if (offset > length)
			{
				/* everything written */
				break;
			}
			memcpy(segment.value, &(contents[offset]), 128);
			segment.more_flag = segno * 128 < length;
			if (ncp_write_property_value(conn, object_type,
						     object_name,
						     property_name,
						     segno, &segment) != 0)
			{
				fprintf(stderr, _("%s: Could not write "
					"property\n"), progname);
				goto finished;
			}
		}
	}
	result = 0;

      finished:
	ncp_close(conn);
	return result;
}
