/*
    modele.c
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


	1.01  2000, April 30		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSListEffectiveRights example.
		Added NWDSExtSyncRead example.
		
	1.02  2001, January-February	Patrick Pollet
		Turn this example into something completely different.

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


// test PP
static NWDSCCODE NWCXNextObject
        ( NWCONN_HANDLE    conn,
          const NWDSChar * scanFilter,
          nuint32*  iterationHandle,
          NWObjectType    objType,
          struct ncp_bindery_object* nextObject
          ) {

   NWCCODE err;
    // just in case I forget and pass NULL or ""  instead of "*" !!!
   if ((!scanFilter) || (strlen(scanFilter)==0))
           scanFilter="*";

     err=ncp_scan_bindery_object(conn,*iterationHandle,objType,scanFilter,nextObject);
     // do it even in case of err !!!
     *iterationHandle=nextObject->object_id; // current object_id will be starting point for the next turn
     return err;
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
	         "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
	       "-v value        Context DCK_FLAGS value (default 0)\n"
	       "-c context_name Name of current context (default OrgUnit.Org.Country)\n"
	       "-C confidence   DCK_CONFIDENCE value (default 0)\n"
	       "-S server       Server to start with (default CDROM)\n"
               "-t objet_type   Default = 0x04 (ncp_bindery_file_server)\n"
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
        const char * server=NULL;
        NWObjectType objType= NCP_BINDERY_FSERVER;


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
	while ((opt = getopt(argc, argv, "h?c:v:S:C:c:t:T:")) != EOF)
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
        	case 't':
			objType =strtoul(optarg, NULL, 0);
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

        {
         nuint32 iterHandle=-1,cnt=0;
         struct ncp_bindery_object next;
         const char * scanFilter="*";

         printf ("starting searching for %d\n",objType);

         while ((err=NWCXNextObject(conn,scanFilter,&iterHandle,objType,&next))==0)
          {
             printf ("%x %s d \n",next.object_type,next.object_name);cnt++;
          }
         if (err==0x89FC) printf ("end of scan: %u found \n",cnt);
         else printf ("ended with %ld\n",err);
        }

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

/********************
 testing
 -t 0xffff  wild
 -t 0x278    tree names
 ....
 ****************/




