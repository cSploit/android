/*
    test_tree_list.c - List servers and trees that are known in the IPX network.
    Demonstate NWCCGetConnInfo (conn, NWCC_INFO_TREE_NAME,...) call

    Copyright (C) 2000 by Patrick Pollet

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

	0.00  2001/01/02			Patrick Pollet
		Initial revision.
        0.01  2001/01/07
                Added Bindery context and server DN
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef N_PLAT_LINUX
#include <unistd.h>
#include <ncp/ncplib.h>
#include <ncp/nwnet.h>

#include "private/libintl.h"
#define _(X) gettext(X)
#else
#include "getopt.h"

#include <nwcalls.h>
#include <nwnet.h>
#define _(X) (X)

static const char* strnwerror(NWDSCCODE err) {
	static char x[100];
	sprintf(x, "error: %u / %d / 0x%X", err, err, err);
	return x;
}
#endif

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn,*conn2;
	struct ncp_bindery_object obj;
        struct ncp_conn_spec *spec=NULL;

	int found = 0;
	char default_pattern[] = "*";
	char *pattern = default_pattern;
	char *p;
	long err,err2;
        char treeName[48];
        nuint32 flags;

        char bindctx[MAX_DN_BYTES];
        char servername[MAX_DN_CHARS];
	NWDSContextHandle ctx;


        NWCallsInit(NULL,NULL);   // needed ????

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);

	if (argc > 2)
	{
		printf(_("usage: %s [pattern]\n"), argv[0]);
		exit(1);
	}
	if (argc == 2)
	{
		pattern = argv[1];
	}
	for (p = pattern; *p != '\0'; p++)
	{
		*p = toupper(*p);
	}
        // NULL  nearest server ????
	if ((conn = ncp_open(NULL, &err)) == NULL)
	{
		com_err(argv[0], err, _("in ncp_open"));
		exit(1);
	}
	if (isatty(1))
	{
		printf("\n%-30s%-10s%-14s%-12s%-20s%-20s\n"
		       "-----------------------------------------------"
		       "-----------------------------------------------\n",
		       _("Known NetWare File Servers"),
		       _("Network"),
		       _("Node Address"),
                       _("Tree Name"),
                       _("Server DN"),
                       _("Bindery CTX"));
	}
	obj.object_id = 0xffffffff;

	while (ncp_scan_bindery_object(conn, obj.object_id,
				       NCP_BINDERY_FSERVER, pattern,
				       &obj) == 0)
	{
		struct nw_property prop;
		struct prop_net_address *naddr
		= (struct prop_net_address *) &prop;

		found = 1;

		printf("%-30s", obj.object_name);
                // 0 = no login required ==> user,pass, and uid to a NULL  value
                spec = ncp_find_conn_spec(obj.object_name,NULL, NULL,
                                        0, -1, &err);

               if (spec == NULL)
                 {
                  printf("err when  trying to find server %lu",err);
                  exit(1);
                 }

		if (ncp_read_property_value(conn, NCP_BINDERY_FSERVER,
					    obj.object_name, 1, "NET_ADDRESS",
					    &prop) == 0)
		{
			ipx_print_network(naddr->network);
			printf("  ");
			ipx_print_node(naddr->node);
		}

                servername[0]='\0';
                conn2 = ncp_open(spec, &err2);
                 if (conn2){
                     err=NWCCGetConnInfo(conn2,NWCC_INFO_TREE_NAME,sizeof(treeName),treeName);
                     switch (err){
                      case 0: printf (" %-15s",treeName);
                              err = NWDSCreateContextHandle(&ctx);
	                      if (err)
		               fprintf(stderr, "NWDSCreateContextHandle failed with %s\n", strnwerror(err));
	                     else {
		               flags = DCV_XLATE_STRINGS|DCV_TYPELESS_NAMES;
		               NWDSSetContext(ctx, DCK_FLAGS, &flags);
                               err=NWDSGetServerDN(ctx,conn2,servername);
                               err=NWDSGetBinderyContext(ctx,conn2,bindctx);
                               printf(" %-20s",servername);
                               printf(" %-20s",bindctx);
                               NWDSFreeContext(ctx);
                              }
                              break;
                      default :printf(" not a DS server");
                    }
                   ncp_close(conn2);
               }else printf(" erreur ncp_open %lu",err2);
                printf("\n");
	}

	if ((found == 0) && (isatty(1)))
	{
		printf(_("No servers found\n"));
	}
	ncp_close(conn);
	return 0;
}


