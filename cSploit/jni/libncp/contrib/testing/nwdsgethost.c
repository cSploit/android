/*
    nwdsgethost.c - NWDSGetObjectHostServerAddress() API demo
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

	1.00  2000, January 26			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial version, copied from readattr demo.

	1.01  2001, May 31			Petr Vandrovec <vandrove@vc.cvut.cz>
		Use NWObjectCount and not size_t...

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
	       "-s size         Reply buffer size (default 4096)\n"
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
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;
	Buf_T* buf;
	size_t size = DEFAULT_MESSAGE_LEN;
	char servername[MAX_DN_BYTES+1];

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
	while ((opt = getopt(argc, argv, "h?o:c:v:S:C:s:")) != EOF)
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
		fprintf(stderr, "Cannot create address buffer: %s\n",
			strnwerror(dserr));
	}
	dserr = NWDSGetObjectHostServerAddress(ctx, objectname, servername, buf);
	if (dserr) {
		fprintf(stderr, "NWDSGetObjectHostServerAddress failed with %s\n", 
			strnwerror(dserr));
	} else {
		unsigned char* pp;
		nuint32 cnt;

		printf("Host server is '%s'\n", servername);

		pp = buf->curPos;		
		dserr = NWDSGetAttrCount(ctx, buf, &cnt);
		if (dserr) {
			fprintf(stderr, "GetAttrCount failed with %s\n",
				strnwerror(dserr));
		} else {
			printf("Attributes: %u\n", cnt);
			pp = buf->curPos;
			while (cnt--) {
				char name[65536];
				nuint32 valcnt;
				enum SYNTAX synt;
				
				dserr = NWDSGetAttrName(ctx, buf, name, &valcnt, &synt);
				if (dserr) {
					fprintf(stderr, "GetAttrName failed with %s\n",
						strnwerror(dserr));
					break;
				}
				printf("  Attribute: '%s'\n", name);

				printf("    Syntax: %d\n", synt);
				printf("    Values count: %d\n", valcnt);
				while (valcnt--) {
					size_t cs;

					printf("      Value:\n");
					dserr = NWDSComputeAttrValSize(ctx, buf, synt, &cs);
					if (dserr) {
						fprintf(stderr, "ComputeAttrValSize failed for SYNT %d\n", synt);
						cs = ~0;
					}
					memset(name, 0xEE, sizeof(name));
					dserr = NWDSGetAttrVal(ctx, buf, synt, name);
					if (dserr) {
						fprintf(stderr, "GetAttrVal failed with %s\n",
							strnwerror(dserr));
						break;
					}
					{
						size_t rl = sizeof(name);
						while ((rl > 0) && (name[rl - 1] == '\xEE')) rl--;
						if (cs != rl) {
							printf("SYNT %d: computed: %u, real: %u\n", synt, cs, rl);
						}
					}
					switch (synt) {
						case SYN_DIST_NAME:
						case SYN_CI_STRING:
						case SYN_CE_STRING:
						case SYN_PR_STRING:
						case SYN_NU_STRING:
						case SYN_TEL_NUMBER:
						case SYN_CLASS_NAME:
							printf("        Value: '%s'\n", name);
							break;
						case SYN_OCTET_STRING:
							{
								Octet_String_T* os = (Octet_String_T*)name;
								size_t i;
								
								printf("        Value: length=%u\n", os->length);
								printf("               ");
								for (i = 0; i < os->length; i++)
									printf("%02X ", os->data[i]);
								printf("\n");
							}
							break;
						case SYN_COUNTER:
						case SYN_INTEGER:
						case SYN_INTERVAL:
							printf("        Value: %u (0x%X)\n", *((Integer_T*)name), *((Integer_T*)name));
							break;
						case SYN_BOOLEAN:
							{
								Boolean_T* b = (Boolean_T*)name;
									
								printf("        Value: %u (%s)\n", *b, (*b==1)?"true":(*b)?"unknown":"false");
							}
							break;
						case SYN_TIMESTAMP:
							{
								TimeStamp_T* stamp = (TimeStamp_T*)name;
								printf("        Value: %u / %u.%u\n",
									stamp->wholeSeconds, stamp->replicaNum,
									stamp->eventID);
							}
							break;
						case SYN_NET_ADDRESS:
							{
								Net_Address_T* na = (Net_Address_T*)name;
								size_t z;

								printf("            Type: %u\n", na->addressType);
								printf("            Length: %u\n", na->addressLength);
								printf("            Data: ");
								for (z = 0; z < na->addressLength; z++)
									printf("%02X ", na->address[z]);
								printf("\n");
							}
							break;
						case SYN_REPLICA_POINTER:
							{
								Replica_Pointer_T* rp = (Replica_Pointer_T*)name;
								Net_Address_T* qp;
								size_t rpcnt;
			
								printf("        Value:\n");
								printf("          Server Name: '%s'\n", rp->serverName);
								printf("          Replica Type: %u\n", rp->replicaType);
								printf("          Replica Number: %u\n", rp->replicaNumber);
								printf("          Address Count: %u\n", rp->count);
								for (rpcnt = rp->count, qp = rp->replicaAddressHint; rpcnt--; qp++) {
									size_t z;

									printf("            Type: %u\n", qp->addressType);
									printf("            Length: %u\n", qp->addressLength);
									printf("            Data: ");
									for (z = 0; z < qp->addressLength; z++)
										printf("%02X ", qp->address[z]);
									printf("\n");
								}
							}
							break;
						case SYN_OBJECT_ACL:
							{
								Object_ACL_T* oacl = (Object_ACL_T*)name;
					
								printf("        Protected Attribute: '%s'\n", oacl->protectedAttrName);
								printf("        Subject Name: '%s'\n", oacl->subjectName);
								printf("        Privileges: %08X\n", oacl->privileges);
							}
							break;
						case SYN_PATH:
							{
								Path_T* p = (Path_T*)name;
								
								printf("        Name Space Type: %u\n", p->nameSpaceType);
								printf("        Volume Name: '%s'\n", p->volumeName);
								printf("        Path: '%s'\n", p->path);
							}
							break;
						case SYN_TIME:
							{
								printf("        Time: %s", ctime((Time_T*)name));
							}
							break;
						case SYN_TYPED_NAME:
							{
								Typed_Name_T* tn = (Typed_Name_T*)name;
									
								printf("        Interval: %u\n", tn->interval);
								printf("        Level: %u\n", tn->level);
								printf("        Name: '%s'\n", tn->objectName);
							}
							break;
						case SYN_CI_LIST:
							{
								CI_List_T* cl = (CI_List_T*)name;
								
								for (; cl; cl = cl->next) {
									printf("        Value: '%s'\n", cl->s);
								}
							}
							break;
						case SYN_OCTET_LIST:
							{
								Octet_List_T* ol = (Octet_List_T*)name;
								
								for (; ol; ol = ol->next) {
									size_t i;
									
									printf("        Value: Length: %u\n", ol->length);
									printf("               ");
									for (i = 0; i < ol->length; i++)
										printf("%02X ", ol->data[i]);
									printf("\n");
								}
							}
							break;
						case SYN_BACK_LINK:
							{
								Back_Link_T* bl = (Back_Link_T*)name;
								
								printf("        Remote ID: %08X\n", bl->remoteID);
								printf("        Object Name: '%s'\n", bl->objectName);
							}
							break;
						case SYN_FAX_NUMBER:
							{
								Fax_Number_T* fn = (Fax_Number_T*)name;
								
								printf("        Fax Number: '%s'\n", fn->telephoneNumber);
								printf("        Parameter bits: %u\n", fn->parameters.numOfBits);
							}
							break;
						case SYN_EMAIL_ADDRESS:
							{
								EMail_Address_T* ea = (EMail_Address_T*)name;
							
								printf("        Type: %u\n", ea->type);
								printf("        Address: '%s'\n", ea->address);
							}
							break;
						case SYN_PO_ADDRESS:
							{
								Postal_Address_T* pa = (Postal_Address_T*)name;

								printf("        Line 1: '%s'\n", (*pa)[0]);
								printf("        Line 2: '%s'\n", (*pa)[1]);
								printf("        Line 3: '%s'\n", (*pa)[2]);
								printf("        Line 4: '%s'\n", (*pa)[3]);
								printf("        Line 5: '%s'\n", (*pa)[4]);
								printf("        Line 6: '%s'\n", (*pa)[5]);
							}
							break;
						case SYN_HOLD:
							{
								Hold_T* h = (Hold_T*)name;
									
								printf("        Amount: %u\n", h->amount);
								printf("        Object Name: '%s'\n", h->objectName);
							}
							break;
						default:
							printf("        Value: unprintable\n");
							break;
					}
				}
				if (dserr)
					break;
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
	NWCCCloseConn(conn);
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 121;
	}
finished:
	return 0;
}
	
