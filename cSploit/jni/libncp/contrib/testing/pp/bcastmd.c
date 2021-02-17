/*
    test_bcast_modes.c
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
           Initial
	1.01  2001, Jan 09
            Added the NULL option for the default server to start with
*/


#ifdef N_PLAT_MSW4
#include <windows.h>

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

#include "private/libintl.h"
#define _(X) gettext(X)
#endif




static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options] bcast_mode\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
	         "usage: %s [options] server_bindery_name \n"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
	       "-v value        Context DCK_FLAGS value (default 0)\n"
	       "-c context_name Name of current context (default Root)\n"
	       "-C confidence   DCK_CONFIDENCE value (default 0)\n"
	       "-S server       Server to start with (default use nearest server)\n"
               "bcast_mode      0: all 1: system only 2: none\n"
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

	const char* context = "[Root]";
	//char* server = "CDROM";
        const char * server=NULL;
        const char * userName=NULL;
        const char * userPwd=NULL;

        int bcast=0;

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
	while ((opt = getopt(argc, argv, "h?c:U:P:v:S:C:")) != EOF)
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
                case 'U':userName =optarg; break;
                case 'P':userPwd =optarg; break;
		default:
		  usage();
		  goto finished;
		}
	}

        if (optind !=(argc-1)) {
		usage();
		goto finished;
	}

        bcast =strtoul (argv[optind],NULL,10) ;
        printf (" bcast will be set to %d \n",bcast);

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
                err = 122;
		goto closing_ctx;
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
         {
               printf (" via ncp_init \n");
               // NB si on met 1 = login required , si il n'y a pas
               // de password dans ~./nwclient le demande !!!!
               conn=ncp_initialize(&argc,argv,1,&err);
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
        char mytreename[MAX_DN_CHARS + 1];

#ifdef N_PLAT_LINUX
        char infos[10000];
	dserr=NWDSSpyConns(ctx,infos);
	printf ("%s\n",infos);
#endif

        dserr=NWCCGetConnInfo(conn,NWCC_INFO_TREE_NAME,sizeof(mytreename),mytreename);
  	if (dserr) {
		fprintf(stderr, "NWCCGetConnInfo(NWCC_INFO_TREE_NAME) failed: %s\n",
			strnwerror(dserr));
		 goto closing_all;
        }
        printf ("Tree Name is %s \n",mytreename);


        dserr= NWSetBroadcastMode(conn, bcast);
        if (dserr) {
		fprintf(stderr, "NWSetBroadcasts failed: %s\n",
			strnwerror(dserr));
		 goto closing_all;
        }
	printf("Waiting for message...\n");
/*I sleep some time to get me time to send messages from another station
(rejected in mode 1 or 2) or from a server console (always sent)
*/
#ifdef N_PLAT_LINUX
       sleep (200);
#else
#  ifdef N_PLAT_MSW4
       Sleep(200000);
#  else
#    error Define appropriate Sleep function
#  endif
#endif

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
I am "manually" NCPMOUNTED (so authentifications keys are in the kernel)
and my .nwclient file is set to that server ( with no password in it)
 via ncp_init
DONE
connections:
state= 8000,uid= cf010001,uid2=cf010001,serv=CIPCINSA,usr=PPOLLET



   */
