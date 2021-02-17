/*
    test_nwdsgetsyntxid.c
    Basic NDS api testing utility
    Copyright (C) 1999, 2000  Petr Vandrovec, Patrick Pollet

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


	1.00  2001, Jan 05		Patrick Pollet <patrick.pollet@insa-lyon.fr>
	
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
typedef char NWDSChar;
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



// tempo TEST before moving to NWNET.C
// conn is an extra parameter since I cannot access to __NWDSGetConnexion from here
static NWDSCCODE PP_NWDSWhoAmI ( NWDSContextHandle ctx,  NWCONN_HANDLE conn, NWDSChar * me ) {
   // strcpy(me,"ppollet.PC");
   NWDSCCODE dserr;
   NWCCODE err;
   //NWCONN_HANDLE conn;
   nuint32 myID;

   //if (!NWDSIsContextValid(ctx)) return  ERR_BAD_CONTEXT; ; //TEMPO
   //if (dserr = __NWDSGetConnexion (ctx, &conn)) return dserr;
   err= NWCCGetConnInfo(conn,  NWCC_INFO_USER_ID  , sizeof(myID), &myID);
   if (!err) {
       dserr= NWDSMapIDToName(ctx, conn, myID, me);
       return dserr;
   }
   else return err;
}

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
	         "usage: %s [options] \"Attribute Name\"\n"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
	       "-v value        Context DCK_FLAGS value (default 0)\n"
	       "-c context_name Name of current context (default Root)\n"
	       "-C confidence   DCK_CONFIDENCE value (default 0)\n"
	       "-S server       Server to start with (default use .nwclient file)\n"
               "Attribute Name  The attribute to read syntax id (default=Unknown)\n"
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

	char* context = NULL;
	//char* server = "CDROM";
        char * server=NULL;



	int opt;
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;


         long err;

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
		fprintf(stderr, "NWDSCreateContextHandle failed with %s\n", strnwerror(dserr));
		return 123;
	}
	while ((opt = getopt(argc, argv, "h?c:v:S:C:c:")) != EOF)
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
	if (context) {
		dserr = NWDSSetContext(ctx, DCK_NAME_CONTEXT, context);
		if (dserr) {
			fprintf(stderr, "NWDSSetContext(DCK_NAME_CONTEXT) failed: %s\n",
				strnwerror(dserr));
	                err = 122;
			goto closing_ctx;
		}
	}
	dserr = NWDSSetContext(ctx, DCK_CONFIDENCE, &confidence);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_CONFIDENCE) failed: %s\n",
			strnwerror(dserr));
                err=121;
		goto closing_ctx;
	}
#ifndef N_PLAT_MSW4
	{
		static const u_int32_t add[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
		dserr = NWDSSetTransport(ctx, 16, add);
		if (dserr) {
			fprintf(stderr, "NWDSSetTransport failed: %s\n",
				strnwerror(dserr));
			err= 124;
                        goto closing_ctx;
		}
	}
#endif

#ifdef N_PLAT_MSW4
	dserr = NWDSOpenConnToNDSServer(ctx, server, &conn);
#else
        printf ("serveur !%s!\n",server);
	if (server) {
          printf (" SERVEUR IS NOT NULL\n");
          if  (server[0] == '/') {
		dserr = ncp_open_mount(server, &conn);
		if (dserr) {
			fprintf(stderr, "ncp_open_mount failed with %s\n",
				strnwerror(dserr));
			err= 111;
                        goto closing_ctx;
		}
	  } else {
		struct ncp_conn_spec connsp;
                printf (" via ncp_open\n");
		memset(&connsp, 0, sizeof(connsp));
		strcpy(connsp.server, server);
		conn = ncp_open(&connsp, &err);
                if (!conn) {
	   	  fprintf(stderr, "ncp_open failed with %s\n",strnwerror(err));
		  err= 111;
                goto closing_ctx;
                }
	     }

        }
        else {
               printf (" via ncp_init \n");
               // NB si on met 1 = login required , si il n'y a pas
               // de password dans ~./nwclient le demande !!!!
               conn=ncp_initialize(&argc,argv,0,&err);
               if (!conn) {
		 fprintf(stderr, "ncp_initialize failed with %s\n",strnwerror(err));
		err= 111;
                goto closing_ctx;
                }
        }
        printf ("DONE\n");
	dserr = NWDSAddConnection(ctx, conn);

#endif

	if (dserr) {
		fprintf(stderr, "Cannot bind connection to context: %s\n",
			strnwerror(dserr));
                err=dserr;
                goto closing_all;

        }

        // testing code goes here
        {
        char me[MAX_DN_CHARS + 1];
        char me2[MAX_DN_CHARS + 1];
        char myserver[MAX_DN_CHARS + 1];
        char mybindctx[MAX_DN_CHARS + 1];
        char myctx[MAX_DN_CHARS + 1];
        char mytreename[MAX_DN_CHARS + 1];

#ifdef N_PLAT_LINUX
        char infos[10000];
	dserr=NWDSSpyConns(ctx,infos);
	printf ("%s\n",infos);
#endif
   /*
   dserr = NWDSGetContext(ctx, DCK_TREE_NAME, mytreename);
	if (dserr) {
		fprintf(stderr, "NWDSGetContext(DCK_TREE_NAME) failed: %s\n",
			strnwerror(dserr));
		 goto closing_all;
	}
  */
        dserr=NWCCGetConnInfo(conn,NWCC_INFO_TREE_NAME,sizeof(mytreename),mytreename);
  	if (dserr) {
		fprintf(stderr, "NWCCGetConnInfo(NWCC_INFO_TREE_NAME) failed: %s\n",
			strnwerror(dserr));
		 goto closing_all;
        }
        printf ("Tree Name is %s \n",mytreename);
	dserr = NWDSGetContext(ctx, DCK_NAME_CONTEXT, myctx);
	if (dserr) {
		fprintf(stderr, "NWDSGetContext(DCK_NAME_CTX) failed: %s\n",
			strnwerror(dserr));
		 goto closing_all;
	}
        printf ("Name context is %s \n",myctx);

     dserr= NWDSGetServerDN(ctx,conn,myserver);
      if (dserr) {
		fprintf(stderr, "NWDSGetServerDN (%s) failed: %s\n",
			argv[optind], strnwerror(dserr));
                 goto closing_all;
	}

      printf ("Server DN is  %s \n",myserver);
      dserr= NWDSGetBinderyContext(ctx,conn,mybindctx);
      if (dserr) {
		fprintf(stderr, "NWDSGetBinderyContext(%s) failed: %s\n",
			argv[optind], strnwerror(dserr));
                 goto closing_all;
	}
      printf ("Bindery context on %s is %s\n",myserver,mybindctx);
#ifdef N_PLAT_MSW4
    dserr= NWDSWhoAmI(ctx,me);
#else
    //dserr= PP_NWDSWhoAmI(ctx,conn,me);
    dserr= NWDSWhoAmI(ctx,me);
#endif

      if (dserr) {
		fprintf(stderr, "NWDSWhoami failed: %s\n", strnwerror(dserr));
                 goto closing_all;
	}
      printf ("I am  %s on %s\n",me,myserver);

      dserr= NWDSAbbreviateName(ctx,me,me2);
      if (dserr) {
		fprintf(stderr, "NWDSAbbreviateName failed: %s\n", strnwerror(dserr));
                goto closing_all;
	}
      printf ("Or in short  %s on %s\n",me2,myserver);

      dserr= NWDSCanonicalizeName(ctx,me2,me);
      if (dserr) {
		fprintf(stderr, "NWDSCanonicalizeName failed: %s\n", strnwerror(dserr));
                goto closing_all;
	}
      printf ("Back to normal %s on %s\n",me,myserver);

       }
        // end of testing code
        err=0;

closing_all:
	NWCCCloseConn(conn);
closing_ctx:
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 121;
	}
finished:
	return err;
}

/* testing
    with EURINSA:apps  ncpmounted as Alexis
./test_nwwhoami -v 4 -c PC
   serveur !(null)!
   via ncp_init
   DONE
   Tree Name is
   Name context is PC
   Server DN is  EURINSA.PC
   Bindery context on EURINSA.PC is O=PC
   I am  Alexis.PC on EURINSA.PC
   Or in short  Alexis on EURINSA.PC
   Back to normal Alexis.PC on EURINSA.PC

./test_nwwhoami -v 4 -c PC  -S eurinsa
   serveur !eurinsa!
   SERVEUR IS NOT NULL
   via ncp_open
   DONE
   Tree Name is
   Name context is PC
   Server DN is  EURINSA.PC
   Bindery context on EURINSA.PC is O=PC
   I am  Alexis.PC on EURINSA.PC
   Or in short  Alexis on EURINSA.PC
   Back to normal Alexis.PC on EURINSA.PC        .

./test_nwwhoami -v 4 -c PC  -S cipcinsa
   serveur !cipcinsa!
   SERVEUR IS NOT NULL
   via ncp_open
   DONE
   Tree Name is
   Name context is PC
   Server DN is  CIPCINSA.PC
   Bindery context on CIPCINSA.PC is O=PC
   NWDSWhoami failed: Unknown Server error (0x89FB)   <-- BIND_NO_SUCH_PROPERTY ????

   */
