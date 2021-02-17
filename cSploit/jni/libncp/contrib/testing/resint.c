/*
    resint.c - Parses and displays WalkTree replies
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


    Do not use this code as base for your programs! NWDSResolveNameInt is 
    ncpfs internal function and may vanish at any time.


    Revision history:

	0.00  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

 */

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>
#include <string.h>

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
	         "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
	       "-S server       Server\n"
	       "-f flag         Resolve flags\n"
	       "-v value        Context DCK_FLAGS value\n"
	       "-o object_name  Name of object\n"
	       "-c context_name Name of current context\n"
	       "\n"));
}

static int dumpSAddr(unsigned char* buff, size_t l, size_t* pos) {
	u_int32_t val;
	size_t x = *pos;
	
	if (l < x + 4) {
		goto dump;
	}
	val = DVAL_LH(buff, x);
	x += 4;
	printf("Server addresses: %d\n", val);
	while (val-- > 0) {
		u_int32_t type;
		u_int32_t lskip;
				
		if (l < x + 4) {
			goto dump;
		}
		type = DVAL_LH(buff, x);
		x += 4;
		printf("-> type: %d\n", type);
		if (l < x + 4) {
			goto dump;
		}
		type = DVAL_LH(buff, x);
		x += 4;
		printf("   length: %d, ", type);
		lskip = ((type + 3) & ~3) - type;
		while (type--) {
			if (l <= x) {
				goto dump;
			}
			printf("%02X ", buff[x++]);
		}
		printf("\n");
		if (lskip) {
			printf("   skip: ");
			while (lskip--) {
				if (l <= x) {
					goto dump;
				}
				printf("%02X ", buff[x++]);
			}
			printf("\n");
		}
	}
	*pos = x;
	return 0;
dump:;
	printf("end of buffer\n");
	*pos = x;
	return 1;
}

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWDSContextHandle ctx;
	NWCONN_HANDLE conn;
	const char* objectname = "[Root]";
	const char* context = "OrgUnit.Org.Country";
	unsigned char* buff;
	const char* server = "CDROM";
	int opt;
	int stop = 1;
	u_int32_t ctxflag = 0;
	u_int32_t version = 0;
	u_int32_t flag = 0x10;
	u_int32_t confidence = 0;
	Buf_T* buffT;
	size_t l;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	NWCallsInit(NULL, NULL);
	NWDSInitRequester();
	
	dserr = NWDSCreateContextHandle(&ctx);
	if (dserr) {
		fprintf(stderr, "NWDSCretateContextHandle failed with %s\n", strnwerror(dserr));
		return 123;
	}
	while ((opt = getopt(argc, argv, "h?o:c:v:S:V:f:C:")) != EOF)
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
		case 'f':
			flag = strtoul(optarg, NULL, 0);
			break;
		case 'V':
			version = strtoul(optarg, NULL, 0);
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
	{
		static const u_int32_t add[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
		dserr = NWDSSetTransport(ctx, 16, add);
		if (dserr) {
			fprintf(stderr, "NWDSSetTransport failed: %s\n",
				strnwerror(dserr));
			return 124;
		}
	}
	{
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
lll:;
	printf("Flag: %08X\n", flag);
	
	NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &buffT);
	dserr = NWDSResolveNameInt(ctx, conn, version, flag, objectname, buffT);
	if (dserr) {
		fprintf(stderr, "Resolve failed with %s\n", 
			strnwerror(dserr));
	} else {
		size_t x;
		u_int32_t val;

		buff = buffT->curPos;
		l = buffT->dataend - buff;
				
		printf("Resolved: %d bytes\n", l);
		x = 0;
		if (l < 4) goto dump;
		val = DVAL_LH(buff, x);
		x += 4;
		printf("Reply type: %08X ", val);
		if (val == 1) {
			printf("(local entry)\n");
			if (l < x + 4) {
				printf("end of buffer\n");
				goto dump;
			}
			val = DVAL_HL(buff, x);
			x += 4;
			printf("Local object ID: %08X\n", val);
			if (dumpSAddr(buff, l, &x))
				goto dump;
		} else if (val == 2) {
			printf("(remote entry)\n");
			if (l < x + 4) {
				printf("end of buffer\n");
				goto dump;
			}
			val = DVAL_HL(buff, x);
			x += 4;
			printf("Remote object ID: %08X\n", val);
			if (l < x + 4) {
				printf("end of buffer\n");
				goto dump;
			}
			val = DVAL_LH(buff, x);
			x += 4;
			printf("Unknown: %08X\n", val);
			if (dumpSAddr(buff, l, &x))
				goto dump;
		} else if (val == 3) {
			printf("(alias)\n");
			if (l < x + 4) {
				printf("end of buffer\n");
				goto dump;
			}
			val = DVAL_LH(buff, x);
			x += 4;
			printf("Aliased Object (%u bytes): ", val);
			if (l < x + val) {
				printf("end of buffer\n");
				goto dump;
			}
			while (val > 1) {
				printf("%c", BVAL(buff, x));
				x += 2;
				val -= 2;
			}
			printf("\n");
		} else if (val == 4) {
			printf("(referrals)\n");
			if (l < x + 4) {
				printf("end of buffer\n");
				goto dump;
			}
			val = DVAL_LH(buff, x);
			x += 4;
			printf("Unknown: %d\n", val);
			if (l < x + 4) {
				printf("end of buffer\n");
				goto dump;
			}
			val = DVAL_LH(buff, x);
			x += 4;
			printf("Servers count: %d\n", val);
			while (val-- > 0) {
				if (dumpSAddr(buff, l, &x))
					goto dump;
			}
		} else if (val == 6) {
			printf("(referrals2)\n");
			if (l < x + 4) {
				printf("end of buffer\n");
				goto dump;
			}
			val = DVAL_LH(buff, x);
			x += 4;
			printf("Unknown: %d\n", val);
			if (l < x + 4) {
				printf("end of buffer\n");
				goto dump;
			}
			val = DVAL_HL(buff, x);
			x += 4;
			printf("An ID?: %08X\n", val);
			if (l < x + 4) {
				printf("end of buffer\n");
				goto dump;
			}
			val = DVAL_LH(buff, x);
			x += 4;
			printf("Servers count: %d\n", val);
			while (val-- > 0) {
				if (dumpSAddr(buff, l, &x))
					goto dump;
			}
		} else {
			printf("(unknown)\n");
			stop = 1;
		}
		
dump:;		
		for ( ; x < l; x++) {
			printf("%02X ", buff[x]);
		}
		printf("\n");
	}
	NWDSFreeBuf(buffT);
	if (!stop) {
		flag++;
		goto lll;
	}
	ncp_close(conn);
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 121;
	}
finished:
	return 0;
}
	
