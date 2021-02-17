/*
    readcls.c - NWDSReadClassDef() API demo
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

	1.00  2000, May 6		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.01  2000, May 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSPutClassName calls.
		
	1.02  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Use NWObjectCount instead of size_t.

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
	       "-o object_name  Name of object (default [Root])\n"
	       "-c context_name Name of current context (default OrgUnit.Org.Country)\n"
	       "-C confidence   DCK_CONFIDENCE value (default 0)\n"
	       "-S server       Server to start with (default CDROM)\n"
	       "-q req_type     Info type (default 0)\n"
	       "-s size         Reply buffer size (default 4096)\n"
	       "-u subject_name If specified, retrieve effective rights\n"
	       "-t time         If specified, return data modified after time\n"
	       "-A class        If specified, return info about this class\n"
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
	const char* subjectname = NULL;
	TimeStamp_T ts = {0, 0, 0};
	int ts_specified = 0;
	int opt;
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;
	u_int32_t req = 0;
	nuint32 ih = NO_MORE_ITERATIONS;
	Buf_T* buf;
	size_t size = DEFAULT_MESSAGE_LEN;
	Buf_T* ibuf;

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
	while ((opt = getopt(argc, argv, "h?o:c:v:S:C:q:s:u:t:A:")) != EOF)
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
		case 'u':
			subjectname = optarg;
			break;
		case 't':
			ts.wholeSeconds = strtoul(optarg, NULL, 0);
			ts_specified = 1;
			break;
		case 'A':
			dserr = NWDSPutClassName(ctx, ibuf, optarg);
			if (dserr) {
				fprintf(stderr, "NWDSPutClassName failed with %s\n",
					strnwerror(dserr));
				return 117;
			}
			break;
		default:
			usage();
			goto finished;
		}
	}

	if (ts_specified && subjectname) {
		fprintf(stderr, "-u and -t cannot be used simultaneously!\n");
		return 111;
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
	do {
		dserr = NWDSReadClassDef(ctx, req, 0, ibuf, &ih, buf);
		if (dserr) {
			fprintf(stderr, "NWDSReadClassDef failed with %s\n", 
				strnwerror(dserr));
		} else {
			unsigned char* pp;
			NWObjectCount cnt;

			printf("Iteration handle: %08X\n", ih);
			pp = buf->curPos;		
			dserr = NWDSGetClassDefCount(ctx, buf, &cnt);
			if (dserr) {
				fprintf(stderr, "GetClassDefCount failed with %s\n",
					strnwerror(dserr));
			} else {
				printf("Classes: %u\n", cnt);
				pp = buf->curPos;
				while (cnt--) {
					char name[65536];
					Class_Info_T cinfo;
					size_t asnp;
					
					dserr = NWDSGetClassDef(ctx, buf, name, &cinfo);
					if (dserr) {
						fprintf(stderr, "GetClassDef failed with %s\n",
							strnwerror(dserr));
						break;
					}
					printf("  Class: '%s'\n", name);
					printf("    Flags: %08X\n", cinfo.classFlags);
					printf("    ASN1:  %d chars, ", cinfo.asn1ID.length);
					for (asnp = 0; asnp < cinfo.asn1ID.length; asnp++)
						printf("%02X", cinfo.asn1ID.data[asnp]);
					printf("\n");
					if (req != 0 && req != 3) {
						size_t c2;
						
						if (req == 5) {
							printf("    Skipping time\n");
							buf->curPos += 16;
						}
						for (c2 = 0; c2 < 5; c2++) {
							static const char* item[] = {
								"Superclass",
								"Containment",
								"Naming Attribute",
								"Mandatory Attribute",
								"Optional Attribute",
							};
							NWObjectCount itemcount;

							dserr = NWDSGetClassItemCount(ctx, buf, &itemcount);
							if (dserr) {
								fprintf(stderr, "NWDSGetClassItemCount failed with %s\n", strnwerror(dserr));
								break;
							}
							printf("    %s names: %u\n", item[c2], itemcount);
							while (itemcount--) {
								dserr = NWDSGetClassItem(ctx, buf, name);
								if (dserr) {
									fprintf(stderr, "NWDSGetClassItem failed with %s\n", strnwerror(dserr));
									break;
								}
								printf("      Name: '%s'\n", name);
							}
							if (dserr)
								break;
						}
						if (dserr)
							break;
						if (req == 4 || req == 5) {
							NWObjectCount itemcount;

							dserr = NWDSGetClassItemCount(ctx, buf, &itemcount);
							if (dserr) {
								fprintf(stderr, "NWDSGetClassItemCount failed with %s\n", strnwerror(dserr));
								break;
							}
							printf("    Default ACL count: %u\n", itemcount);
							while (itemcount--) {
								Object_ACL_T* oaclt = (void*)name;
								dserr = NWDSGetAttrVal(ctx, buf, SYN_OBJECT_ACL, name);
								if (dserr) {
									fprintf(stderr, "NWDSGetClassItem failed with %s\n", strnwerror(dserr));
									break;
								}
								printf("      ACL: %s -> %s, rights %08X\n", oaclt->subjectName, oaclt->protectedAttrName, oaclt->privileges);
							}
							if (dserr)
								break;
						}
					}
					pp = buf->curPos;
				}
			}
#ifndef N_PLAT_MSW4
			for (; pp < buf->dataend; pp++) {
				printf("%02X ", *pp);
			}
			printf("\n");
#endif
		}
	} while ((dserr == 0) && (ih != NO_MORE_ITERATIONS));
	NWCCCloseConn(conn);
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 121;
	}
finished:
	return 0;
}
	
