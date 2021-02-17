/*
    readaddr.c
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
	       "-v value        Context DCK_FLAGS value\n"
	       "-c context_name Name of current context\n"
	       "-n name_format  Name format (default 0)\n"
	       "-S server_name  Server name (default CDROM)\n"
	       "\n"));
}

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWDSContextHandle ctx;
	NWCONN_HANDLE conn;
	const char* context = "OrgUnit.Org.Country";
	unsigned char buff[1000];
	const char* server = "CDROM";
	int opt;
	u_int32_t ctxflag = 0;
	u_int32_t nameform = 0;

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
	while ((opt = getopt(argc, argv, "h?c:v:S:V:n:")) != EOF)
	{
		switch (opt)
		{
		case 'n':
			nameform = strtoul(optarg, NULL, 0);
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
	dserr = NWDSSetContext(ctx, DCK_NAME_FORM, &nameform);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_NAME_FORM) failed: %s\n",
			strnwerror(dserr));
		return 129;
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
	dserr = NWDSGetServerDN(ctx, conn, buff);
	if (dserr)
		fprintf(stderr, "NWDSGetServerDN() failed with %s\n",
			strnwerror(dserr));
	else {
		printf("Server name: >%s<\n", buff);
	}
	{
		Buf_T *b;
		
		dserr = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &b);
		if (dserr)
			fprintf(stderr, "Cannot allocate buffer: %s\n",
				strnwerror(dserr));
		else {
			NWObjectCount cnt;
			
			dserr = NWDSGetServerAddress(ctx, conn, &cnt, b);
			if (dserr)
				fprintf(stderr, "NWDSGetServerAddress() failed: %s\n",
					strnwerror(dserr));
			else {
				printf("Returned %u addresses\n", cnt);
				while (cnt--) {
					size_t len;
					Net_Address_T* p;
					u_int8_t* v;
					int printed = 0;
									
					dserr = NWDSComputeAttrValSize(ctx, b, SYN_NET_ADDRESS, &len);
					if (dserr) {
						fprintf(stderr, "NWDSComputeAttrValSize failed: %s\n",
							strnwerror(dserr));
						break;
					}	
					p = (Net_Address_T*)malloc(len);
					if (!p) {
						fprintf(stderr, "malloc(%u): Out of memory\n",
							len);
						break;
					}
					dserr = NWDSGetAttrVal(ctx, b, SYN_NET_ADDRESS, p);
					if (dserr) {
						fprintf(stderr, "NWDSGetAttrVal failed: %s\n",
							strnwerror(dserr));
						free(p);
						break;
					}
					v = p->address;
					len = p->addressLength;
					switch (p->addressType) {
						case NT_IPX:
							if (len != 12)
								break;
							printf("IPX: %02X%02X%02X%02X:%02X%02X%02X%02X%02X%02X/%02X%02X\n",
								v[0], v[1], v[2], v[3], 
								v[4], v[5], v[6], v[7], v[8], v[9],
								v[10], v[11]);
							printed = 1;
							break;
						case NT_UDP:
							if (len != 6)
								break;
							printf("UDP: %d.%d.%d.%d:%d\n",
								v[2], v[3], v[4], v[5], v[0] * 256 + v[1]);
							printed = 1;
							break;
						case NT_TCP:
							if (len != 6)
								break;
							printf("TCP: %d.%d.%d.%d:%d\n",
								v[2], v[3], v[4], v[5], v[0] * 256 + v[1]);
							printed = 1;
							break;
						case 13:
							printf("URL: ");
							while (len) {
								printf("%c", *v);
								v += 2;
								len -= 2;
							}
							printf("\n");
							printed = 1;
							break;
					}
					if (!printed) {
						printf("Address: %d, len: %d, content: ", p->addressType, p->addressLength);
						while (len--) {
							printf("%02X ", *v++);
						}
						printf("\n");
					}
					free(p);
				}
			}
			NWDSFreeBuf(b);
		}
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
	
