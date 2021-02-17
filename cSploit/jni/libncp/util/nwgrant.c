/*
    nwgrant.c - Add trustee rights to file or directory
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

	0.01  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Support for Netware 3.11 and above.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

	1.01  2000, May 17		Bruce Richardson <brichardson@lineone.net>
		Added rights string alternative for -r
  
  	1.02  2000, August 14		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added DS object resolve (on request from Mike Richichi <mrichich@drew.edu>)

	1.03  2000, October 28		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changed some constants to symbolic names (SA_*, NCP_DIRSTYLE_*).

 */

#include "config.h"

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include <unistd.h>
#include <stdlib.h>
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
		 "usage: %s [options] file/directory\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "-U username    Username sent to server\n"
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "-C             Don't convert password to uppercase\n"
	       "\n"
	       "-o object_name Name of object added as trustee\n"
	       "-t type        Object type (decimal value)\n"
	       "-r rights      Rights mask (see manual page)\n"
	       "\n"
	       "directory\n"
	       "\n"));
}

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	char *object_name = NULL;
	int object_type = -1;
	struct ncp_bindery_object o;
	u_int16_t rights = ~0;
	int perr = 0;
	const char *path = NULL;
	long err;
	int result = 1;
	int opt;
	int useConn = 0;
	char volume[1000];
	unsigned char encpath[2000];
	int enclen;
	TRUSTEE_INFO tstinfo;
	NWCCODE nwerr;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	if ((conn = ncp_initialize_2(&argc, argv, 1, NCP_BINDERY_USER, &err, 0)) == NULL)
	{
		useConn = 1;
	}
	while ((opt = getopt(argc, argv, "h?o:t:r:")) != EOF)
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
		case 'r':
			if (*optarg == '[') {
				perr = ncp_str_to_perms(optarg, &rights);
			} else {
				rights = strtol(optarg, NULL, 16);
			}
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

	if (object_name == NULL)
	{
		fprintf(stderr, _("%s: You must specify an object name\n"),
			argv[0]);
		goto finished;
	}
	if (perr < 0)
	{
		fprintf(stderr, _("%s: You must give a valid rights string\n"),
			argv[0]);
		goto finished;
	}
	if (rights == (u_int16_t)~0)
	{
		fprintf(stderr, _("%s: You must specify a rights mask\n"),
			progname);
		goto finished;
	}

	if (!useConn) {
		if (optind != argc - 1)
		{
			fprintf(stderr, _("%s: You must specify a directory\n"),
				progname);
			goto finished;
		}
		path = argv[optind];
	} else {
		char directory[1000];

		if (optind < argc)
			path = argv[optind];
		else
			path = ".";

		if (NWParsePath(path, NULL, &conn, volume, directory) || !conn) {
			fprintf(stderr, _("%s: Could not find directory %s\n"), progname, path);
			goto finished;
		}
		strcat(volume, ":");
		strcat(volume, directory);
		path = volume;
	}

	if (object_type < 0) {
#ifdef NDS_SUPPORT
		u_int32_t flags;
		NWDSContextHandle ctx;
		
		nwerr = NWDSCreateContextHandle(&ctx);
		if (nwerr) {
failDS:;
			fprintf(stderr, _("%s: Could not find object %s: %s\n"),
				progname, object_name, strnwerror(nwerr));
			goto finished;
		}
		nwerr = NWDSAddConnection(ctx, conn);
		if (nwerr) {
			goto failDS;
		}
		if (!NWDSGetContext(ctx, DCK_FLAGS, &flags)) {
			flags |= DCV_XLATE_STRINGS | DCV_TYPELESS_NAMES;
			NWDSSetContext(ctx, DCK_FLAGS, &flags);
		}
		nwerr = NWDSMapNameToID(ctx, conn, object_name, &o.object_id);
		if (nwerr) {
			goto failDS;
		}
		NWDSFreeContext(ctx);
#else
		fprintf(stderr, _("%s: Could not find object %s: %s\n"),
			progname, object_name, strnwerror(nwerr));
		goto finished;
#endif
	} else {
		if ((nwerr = ncp_get_bindery_object_id(conn, object_type, object_name, &o)) != 0)
		{
			fprintf(stderr, _("%s: Could not find object %s: %s\n"),
				progname, object_name, strnwerror(nwerr));
			goto finished;
		}
	}
	enclen = ncp_path_to_NW_format(path, encpath, sizeof(encpath));
	if (enclen < 0) {
		fprintf(stderr, _("%s: Invalid path: %s\n"),
			progname, strerror(-enclen));
		goto finished;
	}
	tstinfo.objectID = o.object_id;
	tstinfo.objectRights = rights;
	nwerr = ncp_ns_trustee_add(conn, NW_NS_DOS, SA_ALL,
			NCP_DIRSTYLE_NOHANDLE, 0, 0, encpath, enclen, &tstinfo, 1, ~0);
	if (nwerr)
	{
		fprintf(stderr, _("%s: Could not add trustee rights: %s\n"), 
			progname, strnwerror(nwerr));
		goto finished;
	}
	result = 0;

      finished:
	ncp_close(conn);
	return result;
}
