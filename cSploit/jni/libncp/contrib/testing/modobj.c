/*
    modobj.c - NWDSModifyObject() API demo
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

	1.01  2000, July 2		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added attribute type detection code.
		
	1.02  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Add include strings.h (strcasecmp on AIX4.3)

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
#include <strings.h>

#include "private/libintl.h"
#define _(X) gettext(X)
#endif

#include "rsynt.h"

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
	       "-o object_name  Name of object (default [Root])\n"
	       "-c context_name Name of current context (default OrgUnit.Org.Country)\n"
	       "-C confidence   DCK_CONFIDENCE value (default 0)\n"
	       "-S server       Server to start with (default CDROM)\n"
	       "-s size         Request buffer size (default 4096)\n"
	       "-q operation    0=add attr, 1=remove attr, 2=add value, 3=remove value,\n"
	       "                4=add additional value, 5=overwrite value, 6=clear attr,\n"
	       "                7=clear value\n"
	       "-a attr_name    Attribute name (may be specified multiple times)\n"
	       "-V value        Attribute Value (may be specified multiple times)\n"
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
	int opt;
	u_int32_t oper = DS_ADD_ATTRIBUTE;
	u_int32_t ctxflag = DCV_XLATE_STRINGS;
	u_int32_t confidence = 0;
	Buf_T* buf;
	size_t size = DEFAULT_MESSAGE_LEN;
	enum SYNTAX synt = SYN_UNKNOWN;
	Boolean_T boolval;
	Integer_T intval;

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
	dserr = NWDSSetContext(ctx, DCK_FLAGS, &ctxflag);
	dserr = NWDSSetContext(ctx, DCK_NAME_CONTEXT, context);
	dserr = NWDSSetContext(ctx, DCK_CONFIDENCE, &confidence);
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
	dserr = NWDSAllocBuf(size, &buf);
	if (dserr) {
		fprintf(stderr, "Cannot create request buffer: %s\n",
			strnwerror(dserr));
	}
	dserr = NWDSInitBuf(ctx, DSV_MODIFY_ENTRY, buf);
	if (dserr) {
		fprintf(stderr, "Cannot init buffer: %s\n",
			strnwerror(dserr));
	}
	while ((opt = getopt(argc, argv, "h?o:c:v:S:C:s:q:a:V:")) != EOF)
	{
		switch (opt)
		{
		case 'C':
			confidence = strtoul(optarg, NULL, 0);
			dserr = NWDSSetContext(ctx, DCK_CONFIDENCE, &confidence);
			break;
		case 'o':
			objectname = optarg;
			break;
		case 'c':
			context = optarg;
			dserr = NWDSSetContext(ctx, DCK_NAME_CONTEXT, context);
			break;
		case 'v':
			ctxflag = strtoul(optarg, NULL, 0);
			ctxflag |= DCV_XLATE_STRINGS;
			dserr = NWDSSetContext(ctx, DCK_FLAGS, &ctxflag);
			break;
		case 's':
			size = strtoul(optarg, NULL, 0);
			break;
		case 'a':
			dserr = NWDSPutChange(ctx, buf, oper, optarg);
			if (dserr) {
				fprintf(stderr, "Cannot put attrname `%s': %s\n",
					optarg, strnwerror(dserr));
			} else {
				dserr = MyNWDSReadSyntaxID(ctx, optarg, &synt);
				if (dserr) {
					fprintf(stderr, "Cannot find syntax ID for `%s': %s\n",
						optarg, strnwerror(dserr));
				}
			}
			break;
		case 'V':
			dserr = 0;
			switch (synt) {
				case SYN_UNKNOWN:
					fprintf(stderr, "Cannot do SYN_UNKNOWN values\n");
					break;
				case SYN_DIST_NAME:
				case SYN_CE_STRING:
				case SYN_CI_STRING:
				case SYN_PR_STRING:
				case SYN_NU_STRING:
				case SYN_CLASS_NAME:
					dserr = NWDSPutAttrVal(ctx, buf, synt, optarg);
					break;
				case SYN_BOOLEAN:
					if (!strcasecmp(optarg, "yes") ||
					    !strcasecmp(optarg, "on"))
						boolval = 1;
					else if (!strcasecmp(optarg, "no") ||
						 !strcasecmp(optarg, "off"))
						boolval = 0;
					else
						boolval = strtoul(optarg, NULL, 0);
					dserr = NWDSPutAttrVal(ctx, buf, synt, &boolval);
					break;
				case SYN_INTEGER:
				case SYN_COUNTER:
				case SYN_INTERVAL:
					intval = strtoul(optarg, NULL, 0);
					dserr = NWDSPutAttrVal(ctx, buf, synt, &intval);
					break;
				case SYN_NET_ADDRESS:
				case SYN_OCTET_STRING:
				case SYN_STREAM:
					{
						unsigned char t[100];
						size_t ln;
						Octet_String_T osval;

						ln = 0;
						while (*optarg) {
							char x[3];

							x[0] = *optarg++;
							x[1] = *optarg++;
							x[2] = 0;
							t[ln++] = strtoul(x, NULL, 16);
						}
						osval.length = ln;
						osval.data = t;
						dserr = NWDSPutAttrVal(ctx, buf, SYN_OCTET_STRING, &osval);
						
					}
					break;
				case SYN_CI_LIST:
				case SYN_TEL_NUMBER:
				case SYN_FAX_NUMBER:
				case SYN_OCTET_LIST:
				case SYN_EMAIL_ADDRESS:
				case SYN_PATH:
				case SYN_REPLICA_POINTER:
				case SYN_OBJECT_ACL:
				case SYN_PO_ADDRESS:
				case SYN_TIMESTAMP:
				case SYN_BACK_LINK:
				case SYN_TIME:
				case SYN_TYPED_NAME:
				case SYN_HOLD:
					fprintf(stderr, "Cannot do %d syntax...\n", synt);
					break;
				default:
					fprintf(stderr, "Unknown syntax %d\n", synt);
					break;
			}
			if (dserr) {
				fprintf(stderr, "Cannot put value `%s': %s\n",
					optarg, strnwerror(dserr));
			}
			break;
		case 'q':
			oper = strtoul(optarg, NULL, 0);
			break;
		case 'h':
		case '?':
			help();
			goto finished;
		case 'S':
			server = optarg;
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
			break;
		default:
			usage();
			goto finished;
		}
	}

	dserr = NWDSModifyObject(ctx, objectname, NULL, 0, buf);
	if (dserr) {
		fprintf(stderr, "ModifyObject failed with %s\n", 
			strnwerror(dserr));
	} else {
		printf("ModifyObject suceeded\n");
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
	
