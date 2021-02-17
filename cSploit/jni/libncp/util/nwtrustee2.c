/*
    nwtrustee2.c - List trustees to file or directory
    Copyright (C) 1999  Petr Vandrovec

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

	0.00  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
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
	fprintf(stderr, _("usage: %s [options] [file/directory...]\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [options] [file/directory...]\n"), progname);
	printf(_("\n"
	       "-v             Verbose\n"
	       "\n"
	       "file/directory List of files to scan for trustees\n"
	       "\n"));
}

static char*
maskToText(u_int16_t mask) {
	static char rval[1 + 8 + 1 + 1];
	
	strcpy(rval, "[        ]");
	if (mask & TR_SUPERVISOR)
		rval[1] = 'S';
	if (mask & TR_READ)
		rval[2] = 'R';
	if (mask & TR_WRITE)
		rval[3] = 'W';
	if (mask & TR_CREATE)
		rval[4] = 'C';
	if (mask & TR_ERASE)
		rval[5] = 'E';
	if (mask & TR_MODIFY)
		rval[6] = 'M';
	if (mask & TR_SEARCH)
		rval[7] = 'F';
	if (mask & TR_ACCESS_CTRL)
		rval[8] = 'A';
	return rval;
}

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	char *path = NULL;
	int result = 1;
	int opt;
	NWCCODE err;
	NWDSContextHandle ctx;
	NWCCODE errctx = -1;
	struct NWCCRootEntry nwccre;
	u_int32_t iter;
	int verbose = 0;
		
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);
	
	while ((opt = getopt(argc, argv, "h?v")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			goto finished;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
			goto finished;
		}
	}

	if (optind >= argc)
	{
		optind = argc - 1;
		argv[optind] = (char*)".";
	}
	while (optind < argc) {
		int printed = 0;
		
		path = argv[optind++];
		if (verbose) {
			printf("%s:\n", path);
			printed = 1;
		}
		err = ncp_open_mount(path, &conn);
		if (err) {
			fprintf(stderr, _("%s: Could not find directory %s: %s\n"), progname,
				path, strnwerror(err));
			continue;
		}
		err = NWCCGetConnInfo(conn, NWCC_INFO_ROOT_ENTRY, sizeof(nwccre), &nwccre);
		if (err) {
			fprintf(stderr, _("%s: Could not find Netware information about directory %s: %s\n"),
				progname, path, strnwerror(err));
			ncp_close(conn);
			continue;
		}
#ifdef NDS_SUPPORT
		{
			u_int16_t ver;
		
			err = NWGetFileServerVersion(conn, &ver);
			if (!err && (ver >= 0x0400)) {
				errctx = NWDSCreateContextHandle(&ctx);
				if (!errctx) {
					u_int32_t flags;
					NWDSAddConnection(ctx, conn);
					NWDSGetContext(ctx, DCK_FLAGS, &flags);
					flags |= DCV_XLATE_STRINGS;
					NWDSSetContext(ctx, DCK_FLAGS, &flags);
				}
			}
		}
#endif		
		iter = 0;
		while (1) {
			unsigned int i;
			unsigned int count;
			TRUSTEE_INFO tinfo[100];

			count = 100;
			err = ncp_ns_trustee_scan(conn, NW_NS_DOS, SA_ALL, 0x01,
				nwccre.volume, nwccre.dirEnt, NULL, 0,
				&iter, tinfo, &count);
			if (err)
				break;
				
			if (!printed) {
				printf("%s:\n", path);
				printed = 1;
			}
			for (i = 0; i < count; i++) {
				char name[1024];
				u_int16_t type;
			
#ifdef NDS_SUPPORT
				if (!errctx)
					err = NWDSMapIDToName(ctx, conn, tinfo[i].objectID, name);
				else
#endif
					err = NWGetObjectName(conn, tinfo[i].objectID, name, &type);
				if (err)
					sprintf(name, "<#%08X>", tinfo[i].objectID);
				printf("%s %s\n", maskToText(tinfo[i].objectRights), name);
			}
		}
		if (err != 0x899C)
			fprintf(stderr, _("Trustee scan failed, %s\n"), strnwerror(err));
		result = 0;
		ncp_close(conn);
#ifdef NDS_SUPPORT
		if (!errctx)
			NWDSFreeContext(ctx);
#endif
		errctx = 1;
	}
finished:
	return result;
}
