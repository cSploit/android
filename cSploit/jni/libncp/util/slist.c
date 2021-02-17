/*
    slist.c - List all file server that are known in the IPX network.
    Copyright (C) 1995 by Volker Lendecke
  
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

	0.00  1995			Volker Lendecke
		Initial revision.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <ncp/ncplib.h>

#include "private/libintl.h"
#define _(X) gettext(X)

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	struct ncp_bindery_object obj;
	int found = 0;
	char default_pattern[] = "*";
	char *pattern = default_pattern;
	char *p;
	long err;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	if (argc > 2)
	{
		printf(_("usage: %s [pattern]\n"), argv[0]);
		exit(1);
	}
	if (argc == 2)
	{
		pattern = argv[1];
	}
	for (p = pattern; *p != '\0'; p++)
	{
		*p = toupper(*p);
	}

	if ((conn = ncp_open(NULL, &err)) == NULL)
	{
		com_err(argv[0], err, _("in ncp_open"));
		exit(1);
	}
	if (isatty(1))
	{
		printf("\n%-52s%-10s%-12s\n"
		       "-----------------------------------------------"
		       "---------------------------\n",
		       _("Known NetWare File Servers"),
		       _("Network"),
		       _("Node Address"));
	}
	obj.object_id = 0xffffffff;

	while (ncp_scan_bindery_object(conn, obj.object_id,
				       NCP_BINDERY_FSERVER, pattern,
				       &obj) == 0)
	{
		struct nw_property prop;
		struct prop_net_address *naddr
		= (struct prop_net_address *) &prop;

		found = 1;

		printf("%-52s", obj.object_name);

		if (ncp_read_property_value(conn, NCP_BINDERY_FSERVER,
					    obj.object_name, 1, "NET_ADDRESS",
					    &prop) == 0)
		{
			ipx_print_network(naddr->network);
			printf("  ");
			ipx_print_node(naddr->node);
		}
		printf("\n");
	}

	if ((found == 0) && (isatty(1)))
	{
		printf(_("No servers found\n"));
	}
	ncp_close(conn);
	return 0;
}
