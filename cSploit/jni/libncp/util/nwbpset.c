/*
    nwbpset.c - Create a property and set its values
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
	       "\n"));
}

static char *
get_line(char *buf, int len, FILE * stream)
{
	char *result = fgets(buf, len, stream);
	if (result != NULL)
	{
		buf[strlen(buf) - 1] = '\0';	/* remove newline */
	}
	return result;
}

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	char object_name[49];
	int object_type = -1;
	char property_name[17];
	int property_flag, property_security;
	struct ncp_property_info info;
	long err;
	int result = 1;
	char buf[512];
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
	while ((opt = getopt(argc, argv, "h?")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			goto finished;
		default:
			usage();
			goto finished;
		}
	}

	memset(buf, 0, sizeof(buf));
	if (get_line(buf, sizeof(buf), stdin) == NULL)
	{
		fprintf(stderr, _("Illegal format on stdin\n"));
		goto finished;
	}
	object_type = strtoul(buf, NULL, 16);

	memset(object_name, 0, sizeof(object_name));
	if (get_line(object_name, sizeof(object_name), stdin) == NULL)
	{
		fprintf(stderr, _("Illegal format on stdin\n"));
		goto finished;
	}
	memset(property_name, 0, sizeof(property_name));
	if (get_line(property_name, sizeof(property_name), stdin) == NULL)
	{
		fprintf(stderr, _("Illegal format on stdin\n"));
		goto finished;
	}
	memset(buf, 0, sizeof(buf));
	if (get_line(buf, sizeof(buf), stdin) == NULL)
	{
		fprintf(stderr, _("Illegal format on stdin\n"));
		goto finished;
	}
	property_flag = (atoi(buf) & 3);

	memset(buf, 0, sizeof(buf));
	if (get_line(buf, sizeof(buf), stdin) == NULL)
	{
		fprintf(stderr, _("Illegal format on stdin\n"));
		goto finished;
	}
	property_security = (strtoul(buf, NULL, 16) & 0xff);

	if (ncp_scan_property(conn, object_type, object_name,
			      0xffffffff, property_name, &info) == 0)
	{
		/* Property already exists */

		if ((property_flag & 2) != (info.property_flags & 2))
		{
			fprintf(stderr, _("Tried to write %s property\n"),
				(property_flag & 2) != 0 ?
				_("SET over existing ITEM") :
				_("ITEM over existing SET"));
			goto finished;
		}
		if (info.property_security != property_security)
		{
			if (ncp_change_property_security(conn, object_type,
							 object_name,
							 property_name,
						      property_security) != 0)
			{
				fprintf(stderr, _("Could not change "
					"property security\n"));
				goto finished;
			}
		}
	} else
	{
		if (ncp_create_property(conn, object_type, object_name,
					property_name, property_flag,
					property_security) != 0)
		{
			fprintf(stderr, _("Could not create property\n"));
			goto finished;
		}
	}

	if ((property_flag & 2) == 0)
	{
		/* ITEM property */
		size_t i;
		int length;
		int segno;
		char property_value[255 * 128];

		memset(property_value, 0, sizeof(property_value));

		for (i = 0; i < sizeof(property_value); i++)
		{
			if (get_line(buf, sizeof(buf), stdin) == NULL)
			{
				break;
			}
			property_value[i] = strtoul(buf, NULL, 16);
		}
		length = i - 1;

		for (segno = 1; segno <= 255; segno++)
		{
			struct nw_property segment;
			int offset = (segno - 1) * 128;

			if (offset > length)
			{
				/* everything written */
				break;
			}
			memcpy(segment.value, &(property_value[offset]), 128);
			segment.more_flag = segno * 128 < length;
			if (ncp_write_property_value(conn, object_type,
						     object_name,
						     property_name,
						     segno, &segment) != 0)
			{
				fprintf(stderr, _("Could not write property\n"));
				goto finished;
			}
		}
	} else
	{
		/* SET property */

		while (get_line(buf, sizeof(buf), stdin) != NULL)
		{
			int element_type = strtoul(buf, NULL, 16);
			char element_name[49];
			int err2;

			memset(element_name, 0, sizeof(element_name));
			if (get_line(element_name, sizeof(element_name),
				     stdin) == NULL)
			{
				fprintf(stderr, _("Illegal format on stdin\n"));
				goto finished;
			}
			if ((err2 = ncp_add_object_to_set(conn, object_type,
							 object_name,
							 property_name,
							 element_type,
							 element_name)) != 0)
			{
				/* object already in set */
				if (err2 != NWE_BIND_MEMBER_ALREADY_EXISTS) {
					fprintf(stderr, _("Could not add object "
						"to set\n"));
					goto finished;
				}
			}
		}
	}

	result = 0;

      finished:
	ncp_close(conn);
	return result;
}
