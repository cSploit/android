/*
    readsynt.c - Raw access to NWCFragmentRequest()
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

	1.00  2000, May 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

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
	         "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
	       "-v value        Context DCK_FLAGS value (default 0)\n"
	       "-c context_name Name of current context (default OrgUnit.Org.Country)\n"
	       "-C confidence   DCK_CONFIDENCE value (default 0)\n"
	       "-S server       Server to start with (default CDROM)\n"
	       "-q req_type     Request verb (default 40)\n"
	       "-s size         Reply buffer size (default 4096)\n"
	       "-t dword        Add this dword to request\n"
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
	const char* context = "OrgUnit.Org.Country";
	const char* server = "CDROM";
	int opt;
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;
	u_int32_t req = 40;
	Buf_T* buf;
	size_t size = DEFAULT_MESSAGE_LEN;
	Buf_T* ibuf;
	NW_FRAGMENT rq_frag;
	NW_FRAGMENT rp_frag;
	nuint8 rqb[4096];
	nuint8 rpb[4096];
	
	rq_frag.fragAddr.ro = rqb;
	rq_frag.fragSize = 0;
	rp_frag.fragAddr.rw = rpb;
	rp_frag.fragSize = sizeof(rpb);

#ifndef N_PLAT_MSW4
	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
#endif

	progname = argv[0];

	NWCallsInit(NULL, NULL);
#ifndef N_PLAT_MSW4
	NWDSInitRequester();
#endif

	dserr = NWDSCreateContextHandle(&ctx);
	if (dserr) {
		fprintf(stderr, "NWDSCretateContextHandle failed with %s\n", strnwerror(dserr));
		return 123;
	}
	dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &ibuf);
	if (dserr) {	
		fprintf(stderr, "NWDSAllocBuf(input) failed with %s\n",
			strnwerror(dserr));
		return 119;
	}
	dserr = NWDSInitBuf(ctx, DSV_READ_CLASS_DEF, ibuf);
	if (dserr) {
		fprintf(stderr, "NWDSInitBuf(input) failed with %s\n",
			strnwerror(dserr));
		return 118;
	}
	while ((opt = getopt(argc, argv, "h?c:v:S:C:q:s:t:")) != EOF)
	{
		switch (opt)
		{
		case 'C':
			confidence = strtoul(optarg, NULL, 0);
			break;
		case 'c':
			context = optarg;
			break;
		case 'v':
			ctxflag = strtoul(optarg, NULL, 0);
			break;
		case 'q':
			req = strtoul(optarg, NULL, 0);
			break;
		case 's':
			size = strtoul(optarg, NULL, 0);
			break;
		case 'h':
		case '?':
			help();
			goto finished;
		case 'S':
			server = optarg;
			break;
		case 't':
			DSET_LH(rqb, rq_frag.fragSize, strtoul(optarg, NULL, 0));
			rq_frag.fragSize += 4;
			break;
		default:
			usage();
			goto finished;
		}
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
	dserr = NWDSAllocBuf(size, &buf);
	if (dserr) {
		fprintf(stderr, "Cannot create reply buffer: %s\n",
			strnwerror(dserr));
	}
	
	dserr = NWCFragmentRequest(conn, req, 1, &rq_frag, 1, &rp_frag, 0);
	if (dserr)
		fprintf(stderr, "Failed: %s\n", strnwerror(dserr));
	else {
		nuint8* pp;
		for (pp = rp_frag.fragAddr.rw; pp < (nuint8*)rp_frag.fragAddr.rw + rp_frag.fragSize; ) {
			printf("%02X ", *pp++);
		}
		printf("\n");
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
	
