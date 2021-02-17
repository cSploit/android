/*
    nwdpvalues.c
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


    Does something... Do not use this program in scripts, parameters and
    everything around is still subject of unilateral changes


    Revision history:

	0.00  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

 */

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

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
	         "usage: %s [options] [ncp filesystem]\n"), progname);
	printf(_("\n"
	       "-h             Print this help text\n"
	       "-S server      Server name to be used\n"
	       "-U username    Username sent to server\n"
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "-C             Don't convert password to uppercase\n"
	       "\n"
	       "-o object_name Object name to be converted to ID\n"
	       "-q objectID    Object ID to convert to name\n"
	       "-c context     Base context name (use/return abbreviated names)\n"
	       "\n"));
}

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	char *object_name = NULL;
	int verbose = 0;
	long err;
	int useConn = 0;
	NWDSCCODE dserr;
	NWDSContextHandle ctx;
	NWObjectID ID = 0;
	int dir = -1;
	char name[1000];
	int result = 1;
	int i;
	char *context = NULL;

	int opt;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);
	
	dserr = NWDSCreateContextHandle(&ctx);
	if (dserr) {
		fprintf(stderr, "NWDSCretateContextHandle failed with %s\n", strnwerror(dserr));
		return 123;
	}
	if ((conn = ncp_initialize_2(&argc, argv, 1, NCP_BINDERY_USER, &err, 0)) == NULL)
	{
		useConn = 1;
	}
	while ((opt = getopt(argc, argv, "h?o:vq:c:")) != EOF)
	{
		switch (opt)
		{
		case 'c':
			context = optarg;
			break;
		case 'o':
			object_name = optarg;
			dir = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'q':
			ID = strtoul(optarg, NULL, 0);
			dir = 0;
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

	if (useConn) {
		const char* path;

		if (optind < argc)
			path = argv[optind++];
		else
			path = ".";
		if (NWParsePath(path, NULL, &conn, NULL, NULL) || !conn) {
			fprintf(stderr, _("%s: %s is not ncpfs file or directory\n"), progname, path);
			goto finished;
		}
	}

	switch (dir) {
		case 0: /* ID -> name */
			for (i = 2; i < 8; i++) {
				static const char *arr[] = {
					"typed, dotted",
					"typeless, dotted",
					"typed, full dot",
					"typeless, full dot",
					"typed, slashed",
					"typeless, slashed"
				};
				nuint32 q;
		
				q = i >> 1;
				/* 1, 2 or 3 */
				NWDSSetContext(ctx, DCK_NAME_FORM, &q);
				q = (i & 1) ? DCV_TYPELESS_NAMES : 0;
				q |= DCV_XLATE_STRINGS;
				if (context)
					q |= DCV_CANONICALIZE_NAMES;
				NWDSSetContext(ctx, DCK_FLAGS, &q);
				if (context) {
					NWDSSetContext(ctx, DCK_NAME_CONTEXT, context);
				}
		
				printf("Format: %s, value: ", arr[i-2]);
				fflush(stdout);
				dserr = NWDSMapIDToName(ctx, conn, ID, name);
				if (dserr) {
					printf("err(%d)\n", dserr);
					fprintf(stderr, "NWDSMapIDToName failed with %s\n", strnwerror(dserr));
				} else {
					printf("'%s'\n", name);
				}
			}
			break;
		case 1:	/* name -> ID */
			{
				nuint32 q;
				
				q = DCV_TYPELESS_NAMES | DCV_XLATE_STRINGS;
				if (context) {
					q |= DCV_CANONICALIZE_NAMES;
				}
				NWDSSetContext(ctx, DCK_FLAGS, &q);
				if (context) {
					dserr = NWDSSetContext(ctx, DCK_NAME_CONTEXT, context);
					if (dserr) {
						fprintf(stderr, "NWDSSetContext failed with %s\n", strnwerror(dserr));
						break;
					}
				}
				dserr = NWDSMapNameToID(ctx, conn, object_name, &ID);
				if (dserr) {
					fprintf(stderr, "NWDSMapNameToID failed with %s\n", strnwerror(dserr));
				} else {
					fprintf(stderr, "Name '%s' translates to ID 0x%08X\n", object_name, ID);
				}
			}
			break;
		default:
			fprintf(stderr, "-o or -q option must be used!\n");
			break;
	}

	dserr =  NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 122;
	}
finished:
	ncp_close(conn);
	return result;
}

