/*
    readattr.c - NWDSRead(), NWDSExtSyncRead and 
    		 NWDSListAttrsEffectiveRights() API demo
    Copyright (C) 1999, 2000  Petr Vandrovec

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

	1.01  2000, April 30		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSListEffectiveRights example.
		Added NWDSExtSyncRead example.

	1.02  2001, May 14		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added option "-a" to specify attributes list.

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
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <string.h>

#include "private/libintl.h"
#define _(X) gettext(X)
#endif

static char *progname;

static int nflag = 0;

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
	       "-n              Only print attribute value\n"
	       "-v value        Context DCK_FLAGS value (default 0)\n"
	       "-o object_name  Name of object (default [Root])\n"
	       "-c context_name Name of current context (default OrgUnit.Org.Country)\n"
	       "-C confidence   DCK_CONFIDENCE value (default 0)\n"
	       "-S server       Server to start with (default CDROM)\n"
	       "-q req_type     Info type (default 0)\n"
	       "-s size         Reply buffer size (default 4096)\n"
	       "-u subject_name If specified, retrieve effective rights\n"
	       "-t time         If specified, return data modified after time\n"
	       "-a attrname     If specified, return information only about specified attribute\n"
	       "\n"));
}

#ifdef N_PLAT_MSW4
char* strnwerror(int err) {
	static char errc[200];

	sprintf(errc, "error %u / %d / 0x%X", err, err, err);
	return errc;
}
#endif

static int nprintf(char const *format, ...)
{
	va_list ap;
	int ret = 0;

	va_start(ap, format);
	if (!nflag)
		ret = vprintf(format, ap);
	va_end(ap);
	return ret;
}

static void printenum(char const *label, int val, char const * const *names,
		int maxval)
{
	if (!nflag)
		printf("%s: ", label);
	if (val < 0 || val >= maxval)
		printf("%d", val);
	else if (nflag)
		printf("%s", names[val]);
	else
		printf("%s (%d)", names[val], val);
	if (!nflag)
		printf("\n");
}

static void printstring(char const *label, char const *val)
{
	if (nflag)
		printf("%s", val);
	else
		printf("%s: '%s'\n", label, val);
}

static char const * const syntaxes[] = {
	"UNKNOWN",
	"DIST_NAME",
	"CE_STRING",
	"CI_STRING",
	"PR_STRING",
	"NU_STRING",
	"CI_LIST",
	"BOOLEAN",
	"INTEGER",
	"OCTET_STRING",
	"TEL_NUMBER",
	"FAX_NUMBER",
	"NET_ADDRESS",
	"OCTET_LIST",
	"EMAIL_ADDRESS",
	"PATH",
	"REPLICA_POINTER",
	"OBJECT_ACL",
	"PO_ADDRESS",
	"TIMESTAMP",
	"CLASS_NAME",
	"STREAM",
	"COUNTER",
	"BACK_LINK",
	"TIME",
	"TYPED_NAME",
	"HOLD",
	"INTERVAL",
};
#define NSYNTAXES (sizeof(syntaxes) / sizeof(syntaxes[0]))

static char const * const nettypes[] = {
	"IPX",
	"IP",
	"SDLC",
	"TOKENRING_ETHERNET",
	"OSI",
	"APPLETALK",
	"NETBEUI",
	"SOCKADDR",
	"UDP",
	"TCP",
	"UDP6",
	"TCP6",
	"INTERNAL",
	"URL",
};
#define NNETTYPES (sizeof(nettypes) / sizeof(nettypes[0]))

static char const * const namespaces[] = {
	"DOS",
	"MACINTOSH",
	"UNIX",
	"FTAM",
	"OS2",
};
#define NNAMESPACES (sizeof(namespaces) / sizeof(namespaces[0]))

static void printnetaddr(char *name)
{
	Net_Address_T* na = (Net_Address_T*)name;
	size_t z;

	printenum("        Type", na->addressType, nettypes, NNETTYPES);
	if (nflag) printf(":");
	nprintf("        Length: %u\n", na->addressLength);
	nprintf("        Data: ");
	switch (na->addressType) {
	case NT_IPX:
		for (z = 0; z < na->addressLength; z++)
			printf("%02X%s", na->address[z],
			       z == 3 || z == 9 ? ":" : "");
		break;
	case NT_IP:
		if (na->addressLength < 4) break;
		printf("%d.%d.%d.%d", na->address[0], na->address[1],
		       na->address[2], na->address[3]);
		break;
	case NT_TCP:
	case NT_UDP:
		if (na->addressLength < 6) break;
		printf("%d.%d.%d.%d:%d", na->address[2], na->address[3],
		       na->address[4], na->address[5],
		       (na->address[0] << 8) + na->address[1]);
		break;
	case NT_URL:
		/* Ugly, but URLs are confined to a subset of ASCII anyway */
		for (z = 0; z < na->addressLength; z += 2)
			printf("%c", na->address[z]);
		break;
	default:
		for (z = 0; z < na->addressLength; z++)
			printf("%02X ", na->address[z]);
		break;
	}
	printf("\n");
}

static void printattr(NWDSContextHandle ctx, int req, Buf_T *buf)
{
	char name[65536];
	NWObjectCount valcnt;
	enum SYNTAX synt;
	NWDSCCODE dserr;
				
	dserr = NWDSGetAttrName(ctx, buf, name, &valcnt, &synt);
	if (dserr) {
		fprintf(stderr, "GetAttrName failed with %s\n",
			strnwerror(dserr));
		return;
	}
	if (!nflag)
		printstring("  Attribute", name);
	if (req > 0) {
		if (!nflag)
			printenum("    Syntax", synt, syntaxes, NSYNTAXES);
		while (valcnt--) {
			size_t cs;

			nprintf("      Value:\n");
			if ((req == 3) || (req == 4)) {
				nuint32 flags;
				TimeStamp_T stamp;

				dserr = NWDSGetAttrValFlags(ctx, buf, &flags);
				if (dserr) {
					fprintf(stderr, "GetAttrFlags failed with %s\n",
						strnwerror(dserr));
					break;
				}
				nprintf("        Flags: %08X\n", flags);
				dserr = NWDSGetAttrValModTime(ctx, buf, &stamp);
				if (dserr) {
					fprintf(stderr, "GetAttrModTime failed with %s\n",
						strnwerror(dserr));
					break;
				}
				nprintf("        ModTime: %u / %u.%u\n", stamp.wholeSeconds,
				       stamp.replicaNum, stamp.eventID);
			}
			if (req == 4) {
				nuint32 internalLen;

				dserr = NWDSGetAttrValFlags(ctx, buf, &internalLen);
				if (dserr) {
					fprintf(stderr, "GetAttrFlags failed with %s\n",
						strnwerror(dserr));
					break;
				}
				nprintf("        Internal Value Length: %u\n", internalLen);
				continue;
			}
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
					nprintf("SYNT %d: computed: %u, real: %u\n", synt, cs, rl);
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
				printstring("        Value", name);
				if (nflag)
					printf("\n");
				break;
			case SYN_OCTET_STRING:
			{
				Octet_String_T* os = (Octet_String_T*)name;
				size_t i;
										
				nprintf("        Value: length=%u\n", os->length);
				nprintf("               ");
				for (i = 0; i < os->length; i++)
					printf("%02X ", os->data[i]);
				printf("\n");
			}
			break;
			case SYN_COUNTER:
			case SYN_INTEGER:
			case SYN_INTERVAL:
				if (nflag)
					printf("%d\n", *((Integer_T*)name));
				else
					printf("        Value: %d (0x%X)\n", *((Integer_T*)name), *((Integer_T*)name));
				break;
			case SYN_BOOLEAN:
			{
				Boolean_T* b = (Boolean_T*)name;

				if (nflag)
					printf("%s", *b ? "TRUE" : "FALSE");
				else
					printf("        Value: %u (%s)\n", *b, (*b==1)?"true":(*b)?"unknown":"false");
			}
			break;
			case SYN_TIMESTAMP:
			{
				TimeStamp_T* stamp = (TimeStamp_T*)name;
				nprintf("        Value: ");
				printf("%u / %u.%u\n",
				       stamp->wholeSeconds, stamp->replicaNum,
				       stamp->eventID);
			}
			break;
			case SYN_NET_ADDRESS:
				printnetaddr(name);
				break;
			case SYN_REPLICA_POINTER:
			{
				Replica_Pointer_T* rp = (Replica_Pointer_T*)name;
				Net_Address_T* qp;
				int cntv;
				
				nprintf("        Value:\n");
				nprintf("          Server Name: '%s'\n", rp->serverName);
				nprintf("          Replica Type: %u\n", rp->replicaType);
				nprintf("          Replica Number: %u\n", rp->replicaNumber);
				nprintf("          Address Count: %u\n", rp->count);
				for (cntv = rp->count, qp = rp->replicaAddressHint; cntv--; qp++) {
					size_t z;

					nprintf("            Type: %u\n", qp->addressType);
					nprintf("            Length: %u\n", qp->addressLength);
					nprintf("            Data: ");
					for (z = 0; z < qp->addressLength; z++)
						nprintf("%02X ", qp->address[z]);
					nprintf("\n");
				}
			}
			break;
			case SYN_OBJECT_ACL:
			{
				Object_ACL_T* oacl = (Object_ACL_T*)name;
						
				nprintf("        Protected Attribute: '%s'\n", oacl->protectedAttrName);
				nprintf("        Subject Name: '%s'\n", oacl->subjectName);
				nprintf("        Privileges: %08X\n", oacl->privileges);
			}
			break;
			case SYN_PATH:
			{
				Path_T* p = (Path_T*)name;

				printenum("        Name Space Type",
					  p->nameSpaceType,
					  namespaces, NNAMESPACES);
				if (nflag)
					printf("\n");
				printstring("        Volume Name",
					    p->volumeName);
				if (nflag)
					printf("\n");
				printstring("        Path", p->path);
				if (nflag)
					printf("\n");
			}
			break;
			case SYN_TIME:
			{
				nprintf("        Time: %s", ctime((Time_T*)name));
			}
			break;
			case SYN_TYPED_NAME:
			{
				Typed_Name_T* tn = (Typed_Name_T*)name;
										
				nprintf("        Interval: %u\n", tn->interval);
				nprintf("        Level: %u\n", tn->level);
				nprintf("        Name: '%s'\n", tn->objectName);
			}
			break;
			case SYN_CI_LIST:
			{
				CI_List_T* cl = (CI_List_T*)name;
									
				for (; cl; cl = cl->next) {
					nprintf("        Value: '%s'\n", cl->s);
				}
			}
			break;
			case SYN_OCTET_LIST:
			{
				Octet_List_T* ol = (Octet_List_T*)name;
									
				for (; ol; ol = ol->next) {
					size_t i;
											
					nprintf("        Value: Length: %u\n", ol->length);
					nprintf("               ");
					for (i = 0; i < ol->length; i++)
						nprintf("%02X ", ol->data[i]);
					nprintf("\n");
				}
			}
			break;
			case SYN_BACK_LINK:
			{
				Back_Link_T* bl = (Back_Link_T*)name;
										
				nprintf("        Remote ID: %08X\n", bl->remoteID);
				nprintf("        Object Name: '%s'\n", bl->objectName);
			}
			break;
			case SYN_FAX_NUMBER:
			{
				Fax_Number_T* fn = (Fax_Number_T*)name;
										
				nprintf("        Fax Number: '%s'\n", fn->telephoneNumber);
				nprintf("        Parameter bits: %u\n", fn->parameters.numOfBits);
			}
			break;
			case SYN_EMAIL_ADDRESS:
			{
				EMail_Address_T* ea = (EMail_Address_T*)name;
										
				nprintf("        Type: %u\n", ea->type);
				nprintf("        Address: '%s'\n", ea->address);
			}
			break;
			case SYN_PO_ADDRESS:
			{
				Postal_Address_T* pa = (Postal_Address_T*)name;
										
				nprintf("        Line 1: '%s'\n", (*pa)[0]);
				nprintf("        Line 2: '%s'\n", (*pa)[1]);
				nprintf("        Line 3: '%s'\n", (*pa)[2]);
				nprintf("        Line 4: '%s'\n", (*pa)[3]);
				nprintf("        Line 5: '%s'\n", (*pa)[4]);
				nprintf("        Line 6: '%s'\n", (*pa)[5]);
			}
			break;
			case SYN_HOLD:
			{
				Hold_T* h = (Hold_T*)name;
										
				nprintf("        Amount: %u\n", h->amount);
				nprintf("        Object Name: '%s'\n", h->objectName);
			}
			break;
			default:
				nprintf("        Value: unprintable\n");
				break;
			}
		}
		if (dserr)
			return;
	}

}

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
	Buf_T* attr = NULL;
	int allattr = 1;

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
	while ((opt = getopt(argc, argv, "h?no:c:v:S:C:q:s:u:t:a:")) != EOF)
	{
		switch (opt)
		{
		case 'n':
			nflag++;
			break;
		case 'a':
			if (!attr) {
				dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &attr);
				if (dserr) {
					fprintf(stderr, "NWDSAllocBuf failed with %s\n", strnwerror(dserr));
					return 125;
				}
				dserr = NWDSInitBuf(ctx, DSV_READ, attr);
				if (dserr) {
					fprintf(stderr, "NWDSInitBuf failed with %s\n", strnwerror(dserr));
					return 126;
				}
			}
			dserr = NWDSPutAttrName(ctx, attr, optarg);
			if (dserr) {
				fprintf(stderr, "NWDSPutAttrName failed with %s\n", strnwerror(dserr));
				return 127;
			}
			allattr = 0;
			break;
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
		if (subjectname) {
			req = 2;
			dserr = NWDSListAttrsEffectiveRights(ctx, objectname,
					subjectname, allattr, attr, &ih, buf);
		} else if (ts_specified)
			dserr = NWDSExtSyncRead(ctx, objectname, req, allattr, attr,
					&ih, &ts, buf);
		else
			dserr = NWDSRead(ctx, objectname, req, allattr, attr, 
					&ih, buf);
		if (dserr) {
			fprintf(stderr, "Read failed with %s\n", 
				strnwerror(dserr));
		} else {
			unsigned char* pp;
			NWObjectCount cnt;

			nprintf("Iteration handle: %08X\n", ih);
			pp = buf->curPos;		
			dserr = NWDSGetAttrCount(ctx, buf, &cnt);
			if (dserr) {
				fprintf(stderr, "GetAttrCount failed with %s\n",
					strnwerror(dserr));
			} else {
				nprintf("Attributes: %u\n", cnt);
				pp = buf->curPos;
				while (cnt--) {
					printattr(ctx, req, buf);
				}
				pp = buf->curPos;

			}
#ifndef N_PLAT_MSW4
			for (; pp < buf->dataend; pp++) {
				nprintf("%02X ", *pp);
			}
			nprintf("\n");
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
