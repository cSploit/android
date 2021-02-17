/*
    nwbols.c - List bindery objects
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
	       "-t type        Object type to be listed (decimal)\n"
	       "-o object      Object pattern\n"
	       "-v             Verbose listing\n"
	       "-a             Alternative output format\n"
	       "-d             Show object type in decimal\n"
	       "\n"));
}

int
main(int argc, char **argv)
{
	struct ncp_conn *conn;
	struct ncp_bindery_object o;
	int found = 0;

	char default_pattern[] = "*";
	char *pattern = default_pattern;
	char *p;
	long err;
	int opt;
	int verbose = 0;
	int fmt = 0;
	int decl = 0;
	u_int16_t type = 0xffff;
	int useConn = 0;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	if ((conn = ncp_initialize_2(&argc, argv, 1, NCP_BINDERY_USER, &err, 0)) == NULL)
	{
		useConn = 1;
	}
	while ((opt = getopt(argc, argv, "h?vt:o:ad")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			exit(1);
		case 't':
			type = strtol(optarg, NULL, 0);
			break;
		case 'o':
			pattern = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'a':
			fmt = 1;
			break;
		case 'd':
			decl = 1;
			break;
		default:
			usage();
			exit(1);
		}
	}

	if (useConn) {
		const char* path;

		if (optind < argc)
			path = argv[optind];
		else
			path = ".";
		if (NWParsePath(path, NULL, &conn, NULL, NULL) || !conn) {
			fprintf(stderr, _("%s: %s is not on ncpfs filesystem\n"), progname, path);
			return 1;
		}
	} else {
		if (optind < argc)
		{
			usage();
			exit(1);
		}
	}
	for (p = pattern; *p != '\0'; p++)
	{
		*p = toupper(*p);
	}

	o.object_id = 0xffffffff;

	while (ncp_scan_bindery_object(conn, o.object_id,
				       type, pattern, &o) == 0)
	{
		found = 1;
		if (verbose != 0)
		{
			if (fmt) {
				printf("%08X ", o.object_id);
				if (decl) {
					printf("%05u ", o.object_type);
				} else {
					printf("%04X ", o.object_type);
				}
				printf("%-48s %u %02X %u\n",
				       o.object_name, o.object_flags,
				       o.object_security, o.object_has_prop);
			} else {
				printf("%s %08X ", o.object_name, o.object_id);
				if (decl) {
					printf("%05u ", o.object_type);
				} else {
					printf("%04X ", o.object_type);
				}
				printf("%u %02X %u\n", o.object_flags,
				       o.object_security, o.object_has_prop);
			}
		} else
		{
			if (fmt) {
				printf("%08X ", o.object_id);
				if (decl) {
					printf("%05u ", o.object_type);
				} else {
					printf("%04X ", o.object_type);
				}
				printf("%s\n", o.object_name);
			} else {
				printf("%s %08X ", o.object_name, o.object_id);
				if (decl) {
					printf("%05u\n", o.object_type);
				} else {
					printf("%04X\n", o.object_type);
				}
			}
		}
	}

	ncp_close(conn);
	return 0;
}
