/*
    pqlist.c - List all print queues on a server
    Copyright (C) 1996 by Volker Lendecke
  
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

	0.00  1996			Volker Lendecke
		Initial revision.
		
	1.00  2000, January 15		Petr Vandrovec <vandrove@vc.cvut.cz>
		NDS queues support.

	1.01  2000, January 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added call to NWDSGetObjectHostServerAddress to get real queue server.

	1.02  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Use NWObjectCount for object count.

 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <ncp/nwnet.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static int bind_qlist(NWCONN_HANDLE conn, const char* pattern) {
	int found = 0;
	struct ncp_bindery_object q;
	char* dpattern;
	char* p;

	dpattern = strdup(pattern);
	if (!dpattern) {
		return 0;
	}
	q.object_id = 0xffffffff;

	for (p = dpattern; *p != '\0'; p++) {
		if (islower(*p))
			*p = toupper(*p);
	}

	while (ncp_scan_bindery_object(conn, q.object_id,
				       NCP_BINDERY_PQUEUE, dpattern, &q) == 0)
	{
		found = 1;
		printf("%-52s", q.object_name);
		printf("%08X\n", (unsigned int) q.object_id);
	}
	free(dpattern);
	return found;
}

#ifdef NDS_SUPPORT
static int nds_qlist(NWCONN_HANDLE conn, char* pattern) {
	int found = 0;
	NWDSContextHandle ctx;
	NWDSCCODE err;
	nuint32 dsi;
	nuint32 dckflags;
	Buf_T* buf;
	nuint32 iter = NO_MORE_ITERATIONS;
	
	err = NWDSCreateContextHandle(&ctx);
	if (err)
		goto quit;
	err = NWDSAddConnection(ctx, conn);
	if (err)
		goto freectx;
	err = NWDSGetContext(ctx, DCK_FLAGS, &dckflags);
	if (err)
		goto freectx;
	dckflags |= DCV_TYPELESS_NAMES;
	err = NWDSSetContext(ctx, DCK_FLAGS, &dckflags);
	if (err)
		goto freectx;
	dsi = DSI_OUTPUT_FIELDS | DSI_ENTRY_DN;
	err = NWDSSetContext(ctx, DCK_DSI_FLAGS, &dsi);
	if (err)
		goto freectx;
	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &buf);
	if (err)
		goto freectx;
	while ((err = NWDSListByClassAndName(ctx, pattern, "Queue", NULL, &iter, buf)) == 0) {
		NWObjectCount cnt;
		
		err = NWDSGetObjectCount(ctx, buf, &cnt);
		if (err)
			break;
		while (cnt) {
			char name[MAX_DN_BYTES+1];
			char host[MAX_DN_BYTES+1];
			char* b;
			NWObjectID id;
			
			err = NWDSGetObjectNameAndInfo(ctx, buf, name, NULL, &b);
			if (err)
				goto freebuf;
			found = 1;
			printf("%-51s hosted on\n", name);
			err = NWDSGetObjectHostServerAddress(ctx, name, host, NULL);
			if (!err) {
				NWCONN_HANDLE conn2;
				
				printf("  %-49s ", host);
				err = NWDSOpenConnToNDSServer(ctx, host, &conn2);
				if (!err) {
					err = NWDSMapNameToID(ctx, conn2, name, &id);
					if (!err) {
						printf("%08X\n", (unsigned int)id);
					} else {
						printf("<unable to get queue ID: %s>\n", strnwerror(err));
					}
					NWCCCloseConn(conn2);
				} else {
					printf("<unable to connect to host server: %s>\n", strnwerror(err));
				}
			} else {
				printf("  <unable to get host server: %s>\n", strnwerror(err));
			}
		}
		if (iter == NO_MORE_ITERATIONS)
			break;
	}
freebuf:;
	NWDSFreeBuf(buf);
freectx:;
	NWDSFreeContext(ctx);
quit:;
	return found;
}
#else
static int nds_qlist(NWCONN_HANDLE conn, char* pattern) {
	return 0;
}
#endif

int
main(int argc, char **argv)
{
	struct ncp_conn *conn;
	int found = 0;

	long err;
	int optchar;
	int nds_avail;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	if ((conn = ncp_initialize(&argc, argv, 1, &err)) == NULL)
	{
		com_err(argv[0], err, _("when initializing"));
		return 1;
	}
	while ((optchar = getopt(argc, argv, "h")) != -1) {
		switch(optchar) {
			case 'h':
				fprintf(stderr, _("usage: %s [options] [pattern]\n"), argv[0]);
			default:
				ncp_close(conn);
				return 1;
		}
	}

#ifdef NDS_SUPPORT
	nds_avail = NWIsDSServer(conn, NULL);
#else
	nds_avail = 0;
#endif

	if (isatty(1))
	{
		char server[NW_MAX_SERVER_NAME_LEN];
		NWCCODE err2;

		err2 = NWCCGetConnInfo(conn, NWCC_INFO_SERVER_NAME,
			sizeof(server), server);
		if (err2)
			printf(_("\nServer: Unknown (%s)\n"), strnwerror(err2));
		else
			printf(_("\nServer: %s\n"), server);
		printf("%-52s%-10s\n"
		       "-----------------------------------------------"
		       "-------------\n",
		       _("Print queue name"),
		       _("Queue ID"));
	}
	if (optind == argc) {
		found = bind_qlist(conn, "*");
	} else {
		int i;
		
		for (i = optind; i < argc; i++) {
		  	found |= bind_qlist(conn, argv[i]);
		}
	}
	if (nds_avail && !found) {
		int i;
		
		for (i = optind; i < argc; i++) {
			found |= nds_qlist(conn, argv[i]);
		}
	}

	if ((found == 0) && (isatty(1)))
	{
		printf(_("No queues found\n"));
	}
	ncp_close(conn);
	return 0;
}
