/*
    replops.c - NWDS*Replica*, NWDS*Partition* API demo
    Copyright (C) 2000  Petr Vandrovec

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

	1.00  2000, April 29		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.01  2000, April 30		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added calls to NWDSPartitionReceiveAllUpdates,
			NWDSPartitionSendAllUpdates and NWDSSyncPartition.

	1.02  2000, May 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added calls to NWDSRepairTimeStamps.

 */

#ifdef N_PLAT_MSW4
#include <nwcalls.h>
#include <nwnet.h>
#include <nwclxcon.h>

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "getopt.h"

#define _(X) X

typedef unsigned int u_int32_t;
typedef unsigned long NWObjectID;
typedef u_int32_t Time_T;
#else
#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <string.h>

#include "private/libintl.h"
#define _(X) gettext(X)
#endif

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
	         "usage: %s {add|remove|xtype|supdt|rupdt|sync|rtstmp} [options]\n"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
	       "-v value        Context DCK_FLAGS value (default 0)\n"
	       "-o object_name  Name of partition (default [Root])\n"
	       "-r replica_holder  Name of partition holder (default [Root])\n"
	       "-c context_name Name of current context (default OrgUnit.Org.Country)\n"
	       "-C confidence   DCK_CONFIDENCE value (default 0)\n"
	       "-S server       Server to start with (default CDROM)\n"
	       "-q flags        Replica type for add and xtype, delay for sync (default 0)\n"
	       "\n"));
}

#ifdef N_PLAT_MSW4
char* strnwerror(int err) {
	static char errc[200];

	sprintf(errc, "error %u / %d / 0x%X", err, err, err);
	return errc;
}
#endif

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWDSContextHandle ctx;
	NWCONN_HANDLE conn;
	const char* objectname = "[Root]";
	const char* context = "OrgUnit.Org.Country";
	const char* server = "CDROM";
	const char* holder = "[Root]";
	int opt;
	int op;
	const char* oper;
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;
	u_int32_t req = 0;
	int req_present = 0;

#ifndef N_PLAT_MSW4
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
#endif

	progname = argv[0];

	NWCallsInit(NULL, NULL);

	dserr = NWDSCreateContextHandle(&ctx);
	if (dserr) {
		fprintf(stderr, "NWDSCretateContextHandle failed with %s\n", strnwerror(dserr));
		return 123;
	}
	while ((opt = getopt(argc, argv, "h?o:c:v:S:C:q:r:")) != EOF)
	{
		switch (opt)
		{
		case 'C':
			confidence = strtoul(optarg, NULL, 0);
			break;
		case 'o':
			objectname = optarg;
			break;
		case 'c':
			context = optarg;
			break;
		case 'v':
			ctxflag = strtoul(optarg, NULL, 0);
			break;
		case 'q':
			req = strtoul(optarg, NULL, 0);
			req_present = 1;
			break;
		case 'h':
		case '?':
			help();
			goto finished;
		case 'S':
			server = optarg;
			break;
		case 'r':
			holder = optarg;
			break;
		default:
			usage();
			goto finished;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Missing operation (one of add/remove/chgtype)\n");
		exit(6);
	}
	switch (argv[optind][0]) {
		case 'a':
			op = 'a'; oper = "NWDSAddReplica"; break;
		case 'r':
			if (argv[optind][1] == 'u') {
				op = 'R'; oper = "NWDSPartitionReceiveAllUpdates";
				break;
			}
			if (argv[optind][1] == 't') {
				op = 't'; oper = "NWDSRepairTimeStamps";
				break;
			}
		case 'd':
			op = 'r'; oper = "NWDSRemoveReplica"; break;
		case 'c':
		case 'x':
			op = 'c'; oper = "NWDSChangeReplicaType"; break;
		case 's':
			if (argv[optind][1] == 'u') {
				op = 'u'; oper = "NWDSPartitionSendAllUpdates";
			} else {
				op = 's'; oper = "NWDSSyncPartition";
			}
			break;
		default:
			fprintf(stderr, "Unknown operation specified\n");
			exit(5);
	}
	ctxflag |= DCV_XLATE_STRINGS;
	
	dserr = NWDSSetContext(ctx, DCK_FLAGS, &ctxflag);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_FLAGS) failed: %s\n",
			strnwerror(dserr));
		return 123;
	}
	dserr = NWDSSetContext(ctx, DCK_NAME_CONTEXT, context);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_NAME_CONTEXT) failed: %s\n",
			strnwerror(dserr));
		return 122;
	}
	dserr = NWDSSetContext(ctx, DCK_CONFIDENCE, &confidence);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_CONFIDENCE) failed: %s\n",
			strnwerror(dserr));
		return 122;
	}
#ifndef N_PLAT_MSW4
	{
		static const u_int32_t add[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
		dserr = NWDSSetTransport(ctx, 16, add);
		if (dserr) {
			fprintf(stderr, "NWDSSetTransport failed: %s\n",
				strnwerror(dserr));
			return 124;
		}
	}
#endif
#ifdef N_PLAT_MSW4
	dserr = NWDSOpenConnToNDSServer(ctx, server, &conn);
#else
	if (server[0] == '/') {
		dserr = ncp_open_mount(server, &conn);
		if (dserr) {
			fprintf(stderr, "ncp_open_mount failed with %s\n",
				strnwerror(dserr));
			return 111;
		}
	} else {
		struct ncp_conn_spec connsp;
		long err;
		
		memset(&connsp, 0, sizeof(connsp));
		strcpy(connsp.server, server);
		conn = ncp_open(&connsp, &err);
		if (!conn) {
			fprintf(stderr, "ncp_open failed with %s\n",
				strnwerror(err));
			return 111;
		}
	}
	dserr = NWDSAddConnection(ctx, conn);
#endif
	if (dserr) {
		fprintf(stderr, "Cannot bind connection to context: %s\n",
			strnwerror(dserr));
	}
	switch (op) {
		case 'a':
			dserr = NWDSAddReplica(ctx, holder, objectname, req);
			break;
		case 'c':
			dserr = NWDSChangeReplicaType(ctx, objectname, holder,
					req);
			break;
		case 'r':
			dserr = NWDSRemoveReplica(ctx, holder, objectname);
			break;
		case 'R':
			dserr = NWDSPartitionReceiveAllUpdates(ctx, objectname,
					holder);
			break;
		case 's':
			dserr = NWDSSyncPartition(ctx, holder, objectname, req);
			break;
		case 't':
			dserr = NWDSRepairTimeStamps(ctx, objectname);
			break;
		case 'u':
			dserr = NWDSPartitionSendAllUpdates(ctx, objectname,
					holder);
			break;
		default:
			dserr = 12345; break;
	}
	if (dserr) {
		fprintf(stderr, "%s failed with %s\n", oper, strnwerror(dserr));
	}
	NWCCCloseConn(conn);
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 121;
	}
finished:
	return 0;
}
