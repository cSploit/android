/*
    timeinfo.c - NWTime* funcs demo
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

	1.00  2000, May 11		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

 */

#ifndef N_PLAT_LINUX
#include <windows.h>	/* for Sleep() */

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
	       "-c context_name Name of current context (default [Root])\n"
	       "-C confidence   DCK_CONFIDENCE value (default 0)\n"
	       "-S server       NDS Server name (default CDROM.VC.CVUT.CZ)\n"
	       "\n"));
}

nuint8 rpbuff[1024];
NW_FRAGMENT rp[1];
nuint8 rqbuff[100000];
NW_FRAGMENT rq[1];

static void InitBuff(void) {
	memset(rpbuff, 0xDB, sizeof(rpbuff));
	rp[0].fragAddress = rpbuff;
	rp[0].fragSize = sizeof(rpbuff);
	rq[0].fragAddress = rqbuff;
	rq[0].fragSize = 3;
	/* buffer length */
	rqbuff[0] = 0;
	rqbuff[1] = 1;
	/* subfunction... caller will fill it */
	rqbuff[2] = 0;
}

#ifdef N_PLAT_LINUX
#define MyNWRequest NWRequest
#else
NWCCODE MyNWRequest(NWCONN_HANDLE conn, int fn, size_t rqfrags, NW_FRAGMENT *rq, size_t rpfrags, NW_FRAGMENT *rp) {
	NWCCODE err;
	
	err = NWRequest(conn, fn, rqfrags, rq, rpfrags, rp);
	if (err)
		return err;
	if (rp->fragSize == sizeof(rpbuff)) {
		size_t i;

		/* API did not returned real data length, so... try guessing it */
		for (i = sizeof(rpbuff); i > 0 && rpbuff[i-1] == 0xDB; i--);
		rp->fragSize = i;
	}
	return 0;
}

#endif

#ifndef N_PLAT_LINUX
char* strnwerror(int err) {
	static char errc[200];

	sprintf(errc, "error %u / %d / 0x%X", err, err, err);
	return errc;
}
#endif

static void DemoGetTime(NWCONN_HANDLE conn) {
	NWCCODE err;
	
	InitBuff();
	rqbuff[2] = 1;
	printf("GetUTCTime:\n");
	err = MyNWRequest(conn, 114, 1, rq, 1, rp);
	if (err) {
		fprintf(stderr, "NCP 114.1 failed with %s\n", strnwerror(err));
		return;
	}
	printf("Returned data length (at least 28): %u\n", rp->fragSize);
	printf("Seconds:   %u\n", *(u_int32_t*)rpbuff);
	printf("Fractions: 0x%08X (%u)\n", *(u_int32_t*)(rpbuff + 4), *(u_int32_t*)(rpbuff + 4));
	printf("Flags:     0x%08X\n", *(u_int32_t*)(rpbuff + 8));
	printf("An event stamps: %08X %08X %08X %08X\n",
		*(u_int32_t*)(rpbuff + 12),
		*(u_int32_t*)(rpbuff + 16),
		*(u_int32_t*)(rpbuff + 20),
		*(u_int32_t*)(rpbuff + 24));
}

static void DemoGetVersion(NWCONN_HANDLE conn) {
	NWCCODE err;
	
	InitBuff();
	rqbuff[2] = 12;
	printf("GetTimeVersion:\n");
	err = MyNWRequest(conn, 114, 1, rq, 1, rp);
	if (err) {
		fprintf(stderr, "NCP 114.12 failed with %s\n", strnwerror(err));
		return;
	}
	printf("Returned data length (at least 4): %u\n", rp->fragSize);
	printf("Version:   0x%08X (%u)\n", *(u_int32_t*)rpbuff, *(u_int32_t*)rpbuff);
}

static void DemoGetGarbage(NWCONN_HANDLE conn) {
	NWCCODE err;
	size_t i;
	
	InitBuff();
	rqbuff[2] = 6;
	printf("GetTimeGarbage:\n");
	err = MyNWRequest(conn, 114, 1, rq, 1, rp);
	if (err) {
		fprintf(stderr, "NCP 114.6 failed with %s\n", strnwerror(err));
		return;
	}
	printf("Returned data length: %u\n", rp->fragSize);
	printf("Data: ");
	for (i = 0; i < rp->fragSize; i++)
		printf(" %02X", rpbuff[i]);
	printf("\n");
}

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWDSContextHandle ctx;
	NWCONN_HANDLE conn;
	const char* context = "[Root]";
	const char* server = "CDROM.VC.CVUT.CZ";
	int opt;
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;

#ifdef N_PLAT_LINUX
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
	while ((opt = getopt(argc, argv, "h?o:c:v:S:C:t:")) != EOF)
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
		case 'h':
		case '?':
			help();
			goto finished;
		case 'S':
			server = optarg;
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
#ifdef N_PLAT_LINUX
	{
		static const u_int32_t add[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
		dserr = NWDSSetTransport(ctx, 16, add);
		if (dserr) {
			fprintf(stderr, "NWDSSetTransport failed: %s\n",
				strnwerror(dserr));
			return 124;
		}
	}
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
#else	
	dserr = NWDSOpenConnToNDSServer(ctx, server, &conn);
#endif
	if (dserr) {
		fprintf(stderr, "Cannot bind connection to context: %s\n",
			strnwerror(dserr));
	}
	DemoGetTime(conn);
	printf("Waiting 500 ms\n"); 
#ifdef N_PLAT_LINUX
	usleep(500000);
#else
	Sleep(500);
#endif
	DemoGetTime(conn);
	DemoGetVersion(conn);
	DemoGetGarbage(conn);
	NWCCCloseConn(conn);
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 121;
	}
finished:
	return 0;
}
	
