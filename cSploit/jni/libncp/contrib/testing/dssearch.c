/*
    dssearch.c - NWDSSearch() API demo
    Copyright (C) 1999-2003  Petr Vandrovec

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
		Initial revision of nwdslist.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

	1.01  2000, April 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSGetCountByClassAndName demo.  

	1.02  2000, April 30		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSExtSyncList demo.
		
	1.03  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Use NWObjectCount and not size_t (AIX4.3 fixes).
		
	1.04  2003, January 11		Petr Vandrovec <vandrove@vc.cvut.cz>
		Create dssearch.c from nwdslist.c

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
	       "-f subject_name Filter for subordinates (default NONE, may contain wildcards)\n"
	       "-l class_name   Filter for base class names (default NONE)\n"
	       "-t timestamp    Return objects after this time (default ALL)\n"
	       "\n"));
}

#ifdef N_PLAT_MSW4
char* strnwerror(int err) {
	static char errc[200];

	sprintf(errc, "error %u / %d / 0x%X", err, err, err);
	return errc;
}
#endif

static void prtattr(NWDSContextHandle ctx, Buf_T* buf, unsigned int cnt, unsigned int req, const char* pref) {
	unsigned char* pp;
	NWDSCCODE dserr;

	pp = buf->curPos;
	while (cnt--) {
		char name[65536];
		NWObjectCount valcnt;
		enum SYNTAX synt;

		dserr = NWDSGetAttrName(ctx, buf, name, &valcnt, &synt);
		if (dserr) {
			fprintf(stderr, "GetAttrName failed with %s\n",	strnwerror(dserr));
			break;
		}
		printf("%sAttribute: '%s'\n", pref, name);
		if (req > 0) {
			printf("%s  Syntax: %d\n", pref, synt);
			printf("%s  Values count: %d\n", pref, valcnt);
			while (valcnt--) {
				size_t cs;

				printf("%s    Value:\n", pref);
				if ((req == 3) || (req == 4)) {
					nuint32 flags;
					TimeStamp_T stamp;

					dserr = NWDSGetAttrValFlags(ctx, buf, &flags);
					if (dserr) {
						fprintf(stderr, "GetAttrFlags failed with %s\n", strnwerror(dserr));
						break;
					}
					printf("%s      Flags: %08X\n", pref, flags);
					dserr = NWDSGetAttrValModTime(ctx, buf, &stamp);
					if (dserr) {
						fprintf(stderr, "GetAttrModTime failed with %s\n", strnwerror(dserr));
						break;
					}
					printf("%s      ModTime: %u / %u.%u\n", pref, stamp.wholeSeconds,
						stamp.replicaNum, stamp.eventID);
				}
				dserr = NWDSComputeAttrValSize(ctx, buf, synt, &cs);
				if (dserr) {
					fprintf(stderr, "ComputeAttrValSize failed for SYNT %d\n", synt);
					cs = ~0;
				}
				memset(name, 0xEE, sizeof(name));
				dserr = NWDSGetAttrVal(ctx, buf, synt, name);
				if (dserr) {
					fprintf(stderr, "GetAttrVal failed with %s\n", strnwerror(dserr));
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
						printf("%s      Value: '%s'\n", pref, name);
						break;
					case SYN_OCTET_STRING:
						{
							Octet_String_T* os = (Octet_String_T*)name;
							size_t i;

							printf("%s      Value: length=%u\n", pref, os->length);
							printf("%s             ", pref);
							for (i = 0; i < os->length; i++)
								printf("%02X ", os->data[i]);
							printf("\n");
						}
						break;
					case SYN_COUNTER:
					case SYN_INTEGER:
					case SYN_INTERVAL:
						printf("%s      Value: %u (0x%X)\n", pref, *((Integer_T*)name), *((Integer_T*)name));
						break;
					case SYN_BOOLEAN:
						{
							Boolean_T* b = (Boolean_T*)name;

							printf("%s      Value: %u (%s)\n", pref, *b, (*b==1)?"true":(*b)?"unknown":"false");
						}
						break;
					case SYN_TIMESTAMP:
						{
							TimeStamp_T* stamp = (TimeStamp_T*)name;
							printf("%s      Value: %u / %u.%u\n", pref,
								stamp->wholeSeconds, stamp->replicaNum,
								stamp->eventID);
						}
						break;
					case SYN_NET_ADDRESS:
						{
							Net_Address_T* na = (Net_Address_T*)name;
							size_t z;

							printf("%s          Type: %u\n", pref, na->addressType);
							printf("%s          Length: %u\n", pref, na->addressLength);
							printf("%s          Data: ", pref);
							for (z = 0; z < na->addressLength; z++)
								printf("%02X ", na->address[z]);
							printf("\n");
						}
						break;
					case SYN_REPLICA_POINTER:
						{
							Replica_Pointer_T* rp = (Replica_Pointer_T*)name;
							Net_Address_T* qp;
							int cntv;

							printf("%s      Value:\n", pref);
							printf("%s        Server Name: '%s'\n", pref, rp->serverName);
							printf("%s        Replica Type: %u\n", pref, rp->replicaType);
							printf("%s        Replica Number: %u\n", pref, rp->replicaNumber);
							printf("%s        Address Count: %u\n", pref, rp->count);
							for (cntv = rp->count, qp = rp->replicaAddressHint; cntv--; qp++) {
								size_t z;

								printf("%s          Type: %u\n", pref, qp->addressType);
								printf("%s          Length: %u\n", pref, qp->addressLength);
								printf("%s          Data: ", pref);
								for (z = 0; z < qp->addressLength; z++)
									printf("%02X ", qp->address[z]);
								printf("\n");
							}
						}
						break;
					case SYN_OBJECT_ACL:
						{
							Object_ACL_T* oacl = (Object_ACL_T*)name;

							printf("%s      Protected Attribute: '%s'\n", pref, oacl->protectedAttrName);
							printf("%s      Subject Name: '%s'\n", pref, oacl->subjectName);
							printf("%s      Privileges: %08X\n", pref, oacl->privileges);
						}
						break;
					case SYN_PATH:
						{
							Path_T* p = (Path_T*)name;

							printf("%s      Name Space Type: %u\n", pref, p->nameSpaceType);
							printf("%s      Volume Name: '%s'\n", pref, p->volumeName);
							printf("%s      Path: '%s'\n", pref, p->path);
						}
						break;
					case SYN_TIME:
						{
							printf("%s      Time: %s", pref, ctime((Time_T*)name));
						}
						break;
					case SYN_TYPED_NAME:
						{
							Typed_Name_T* tn = (Typed_Name_T*)name;

							printf("%s      Interval: %u\n", pref, tn->interval);
							printf("%s      Level: %u\n", pref, tn->level);
							printf("%s      Name: '%s'\n", pref, tn->objectName);
						}
						break;
					case SYN_CI_LIST:
						{
							CI_List_T* cl = (CI_List_T*)name;

							for (; cl; cl = cl->next) {
								printf("%s      Value: '%s'\n", pref, cl->s);
							}
						}
						break;
					case SYN_OCTET_LIST:
						{
							Octet_List_T* ol = (Octet_List_T*)name;

							for (; ol; ol = ol->next) {
								size_t i;

								printf("%s      Value: Length: %u\n", pref, ol->length);
								printf("%s             ", pref);
								for (i = 0; i < ol->length; i++)
									printf("%02X ", ol->data[i]);
								printf("\n");
							}
						}
						break;
					case SYN_BACK_LINK:
						{
							Back_Link_T* bl = (Back_Link_T*)name;

							printf("%s      Remote ID: %08X\n", pref, bl->remoteID);
							printf("%s      Object Name: '%s'\n", pref, bl->objectName);
						}
						break;
					case SYN_FAX_NUMBER:
						{
							Fax_Number_T* fn = (Fax_Number_T*)name;

							printf("%s      Fax Number: '%s'\n", pref, fn->telephoneNumber);
							printf("%s      Parameter bits: %u\n", pref, fn->parameters.numOfBits);
						}
						break;
					case SYN_EMAIL_ADDRESS:
						{
							EMail_Address_T* ea = (EMail_Address_T*)name;

							printf("%s      Type: %u\n", pref, ea->type);
							printf("%s      Address: '%s'\n", pref, ea->address);
						}
						break;
					case SYN_PO_ADDRESS:
						{
							Postal_Address_T* pa = (Postal_Address_T*)name;

							printf("%s      Line 1: '%s'\n", pref, (*pa)[0]);
							printf("%s      Line 2: '%s'\n", pref, (*pa)[1]);
							printf("%s      Line 3: '%s'\n", pref, (*pa)[2]);
							printf("%s      Line 4: '%s'\n", pref, (*pa)[3]);
							printf("%s      Line 5: '%s'\n", pref, (*pa)[4]);
							printf("%s      Line 6: '%s'\n", pref, (*pa)[5]);
						}
						break;
					case SYN_HOLD:
						{
							Hold_T* h = (Hold_T*)name;

							printf("%s      Amount: %u\n", pref, h->amount);
							printf("%s      Object Name: '%s'\n", pref, h->objectName);
						}
						break;
					default:
						printf("%s      Value: unprintable\n", pref);
						break;
				}
			}
			if (dserr)
				break;
			pp = buf->curPos;
		}
	}
}

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWDSContextHandle ctx;
	NWCONN_HANDLE conn;
	const char* objectname = "[Root]";
	const char* context = "OrgUnit.Org.Country";
	const char* server = "CDROM";
	const char* classname = NULL;
	const char* subjectname = NULL;
	TimeStamp_T ts = { 0, 0, 0 };
	int ts_defined = 0;
	int opt;
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;
	u_int32_t req = 0;
	nuint32 ih = NO_MORE_ITERATIONS;
	Buf_T* buf;
	Buf_T* sf;
	size_t size = DEFAULT_MESSAGE_LEN;
	NWObjectCount cnt;

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
	dserr = NWDSAllocBuf(size, &sf);
	if (dserr) {
		fprintf(stderr, "NWDSAllocBuf failed with %s\n", strnwerror(dserr));
		return 124;
	}
	dserr = NWDSInitBuf(ctx, DSV_SEARCH_FILTER, sf);
	if (dserr) {
		fprintf(stderr, "NWDSInitBuf failed with %s\n", strnwerror(dserr));
		return 125;
	}
	while ((opt = getopt(argc, argv, "h?o:c:v:S:C:q:s:f:l:t:")) != EOF)
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
		case 'f':
			subjectname = optarg;
			break;
		case 'l':
			classname = optarg;
			break;
		case 't':
			ts.wholeSeconds = strtoul(optarg, NULL, 0);
			ts_defined = 1;
			break;
		default:
			usage();
			goto finished;
		}
	}
	{
		Filter_Cursor_T* cft;
		
		dserr = NWDSAllocFilter(&cft);
		if (dserr) {
			fprintf(stderr, "NWDSAllocFilter failed: %s\n", strnwerror(dserr));
			return 128;
		}
		while (optind < argc) {
			const char* p = argv[optind++];
			const void* val = NULL;
			enum SYNTAX synt = 0;
			unsigned int ftok = FTOK_END;
			
			if (strlen(p) == 1) {
				switch (p[0]) {
					case '!':
						ftok = FTOK_NOT;
						break;
					case '&':
						ftok = FTOK_AND;
						break;
					case '|':
						ftok = FTOK_OR;
						break;
					case '(':
						ftok = FTOK_LPAREN;
						break;
					case ')':
						ftok = FTOK_RPAREN;
						break;
					case '=':
						ftok = FTOK_EQ;
						break;
					case '~':
						ftok = FTOK_APPROX;
						break;
				}
			}
			if (ftok == FTOK_END) {
				if (!strcmp(p, "<="))
					ftok = FTOK_LE;
				else if (!strcmp(p, ">="))
					ftok = FTOK_GE;
				else if (p[0] == 'n') {
					ftok = FTOK_ANAME;
					val = p + 1;
				} else if (p[0] == 'v') {
					ftok = FTOK_AVAL;
					val = p + 1;
					synt = SYN_CI_STRING;
				} else if (p[0] == 'r') {
					ftok = FTOK_RDN;
					val = p + 1;
				} else if (p[0] == 'b') {
					ftok = FTOK_BASECLS;
					val = p + 1;
				}
			}
			if (ftok == FTOK_END) {
				fprintf(stderr, "Sorry, cannot parse '%s' as filter token\n", p);
				return 127;
			}
			dserr = NWDSAddFilterToken(cft, ftok, val, synt);
			if (dserr) {
				fprintf(stderr, "NWDSAddFilterToken(%s) failed: %s\n",
					p, strnwerror(dserr));
				return 128;
			}
		}
		dserr = NWDSAddFilterToken(cft, FTOK_END, NULL, 0);
		if (dserr) {
			fprintf(stderr, "NWDSAddFilterToken(FTOK_END) failed: %s\n",
				strnwerror(dserr));
			return 126;
		}
		dserr = NWDSPutFilter(ctx, sf, cft, NULL);
		if (dserr) {
			fprintf(stderr, "NWDSPutFilter failed: %s\n", strnwerror(dserr));
			return 127;
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
	do {
		dserr = NWDSSearch(ctx, objectname, DS_SEARCH_SUBTREE, 1,
				sf, req, 1, NULL, &ih, 10000, NULL, buf);
		if (dserr) {
			fprintf(stderr, "List failed with %s\n", 
				strnwerror(dserr));
		} else if (req != 3) {
			unsigned char* pp;

			printf("Iteration handle: %08X\n", ih);
			
			dserr = NWDSGetObjectCount(ctx, buf, &cnt);
			if (dserr) {
				fprintf(stderr, "GetObjectCount failed with %s\n",
					strnwerror(dserr));
			} else {
				printf("%u objects\n", cnt);
				while (cnt--) {
					Object_Info_T tt;
					char name[8192];
					NWObjectCount attrs;
					
					dserr = NWDSGetObjectName(ctx, buf, name, &attrs, &tt);
					if (dserr) {
						fprintf(stderr, "GetObjectName failed with %s\n",
							strnwerror(dserr));
						break;
					}
					printf("Name: %s\n", name);
					printf("  Attrs: %u\n", attrs);
					printf("  Flags: %08X\n", tt.objectFlags);
					printf("  Subordinate Count: %u\n", tt.subordinateCount);
					printf("  Modification Time: %lu\n", tt.modificationTime);
					printf("  Base Class: %s\n", tt.baseClass);
					prtattr(ctx, buf, attrs, req, "  ");
				}
			}
#ifndef N_PLAT_MSW4
			pp = buf->curPos;	
			for (; pp < buf->dataend; pp++) {
				printf("%02X ", *pp);
			}
			printf("\n");
#endif
		} else {
			printf("%u objects found\n", cnt);
		}
	} while ((dserr == 0) && (ih != NO_MORE_ITERATIONS));
	NWCCCloseConn(conn);
	NWDSFreeBuf(sf);
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 121;
	}
finished:
	return 0;
}
