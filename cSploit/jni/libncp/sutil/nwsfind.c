/*
    nwsfind.c
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


    Find a NetWare server and open a route to it.
    This tool can safely be setuid root, if normal users should be able to
    access any NetWare server.


    Revision history:

	0.00  1996			Volker Lendecke
		Initial release.

	0.01  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Added 'find by address' feature.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

 */

#include "config.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ncp/ncplib.h>

#include "private/libintl.h"
#define _(X) gettext(X)

#ifdef CONFIG_NATIVE_IPX
extern int ipx_make_reachable_rip(const struct sockaddr_ipx*);
#endif

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [server]\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [server]\n"), progname);
	printf(_("\n"
	       "-t             Server type, default: File server\n"
	       "-a             server is in form <net>:<node>:<socket>\n"
	       "-h             Print this help text\n"
	       "\n"));
}

int
main(int argc, char *argv[])
{
	const char *server = NULL;
	int object_type = NCP_BINDERY_FSERVER;
	long err;
	union ncp_sockaddr addr;
#ifdef CONFIG_NATIVE_IPX
	int is_address = 0;
#endif

	int opt;

	progname = argv[0];

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	while ((opt = getopt(argc, argv, 
#ifdef CONFIG_NATIVE_IPX
					"a"
#endif
					"t:")) != EOF)
	{
		switch (opt)
		{
		case 't':
			object_type = atoi(optarg);
			break;
#ifdef CONFIG_NATIVE_IPX
		case 'a':
		    is_address = 1;
		    break;
#endif
		case 'h':
		case '?':
			help();
			exit(1);
		default:
			usage();
			exit(1);
		}
	}

	if (optind < argc - 1)
	{
		usage();
		exit(1);
	}
#ifdef CONFIG_NATIVE_IPX
	if (is_address)
	{
		if ((optind > argc - 1) || 
		    ipx_sscanf_saddr(argv[optind], &addr.ipx))
		{
			usage();
			exit(1);
		}
		err = ipx_make_reachable_rip(&addr.ipx);
		server = argv[optind];
        }
        else
#endif	/* CONFIG_NATIVE_IPX */
        {
		if (optind == argc - 1)
		{
			server = argv[optind];
			if (strlen(server) >= NCP_BINDERY_NAME_LEN)
			{
				fprintf(stderr, _("%s: Server name too long\n"), argv[0]);
				exit(1);
			}
		}
		err = ncp_find_server(&server, object_type, &addr.any, sizeof(addr));
	}

	if (err != 0)
	{
		com_err(argv[0], err, _("when trying to find server"));
		exit(1);
	}
#ifdef CONFIG_NATIVE_IPX
	ipx_print_saddr(&addr.ipx);
#else
	printf("X");
#endif
	printf(" %s\n", server);
	return 0;
}
