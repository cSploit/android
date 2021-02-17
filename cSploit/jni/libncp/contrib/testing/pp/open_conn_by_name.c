/*
    test_open_conn_by_name.c
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
	fprintf(stderr, _("usage: %s [options]\n"), progname);
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
               "server_bindery_name server to open a connection to \n"
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
	int nameType = NWCC_NAME_FORMAT_BIND;

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
	while ((opt = getopt(argc, argv, "h?c:U:P:v:t:S:C:")) != EOF)
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
		case 't':
			nameType = strtoul(optarg, NULL, 0);
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
        char me[MAX_DN_CHARS + 1];
        char me2[MAX_DN_CHARS + 1];
        char myserver[MAX_DN_CHARS + 1];
        char mybindctx[MAX_DN_CHARS + 1];
        char mybindctx2[MAX_DN_CHARS + 1];
        char myctx[MAX_DN_CHARS + 1];
        char mytreename[MAX_DN_CHARS + 1];
        NWCONN_HANDLE conn2;

        char infos[10000];
   dserr=NWDSSpyConns(ctx,infos);
   printf ("%s\n",infos);

  
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


      dserr=NWCCOpenConnByName(NULL,argv[optind],nameType,
                               0, NWCC_RESERVED,&conn2);
     if (dserr) {
		fprintf(stderr, "NWDSOpenConnByName(%s) failed: %s\n",
			argv[optind], strnwerror(dserr));
                goto closing_all;
	}
      // the connection DO appears at least on monitor with " NOT-LOGGED_IN.  looks good


      dserr = NWDSAddConnection(ctx, conn2);
      printf (" before authenticating :\n");
      dserr=NWDSSpyConns(ctx,infos);
      printf ("%s\n",infos);


      // ask him something on conn2
      dserr=NWCCGetConnInfo(conn2,NWCC_INFO_SERVER_NAME,sizeof(me2),me2);
      if (dserr) {
		fprintf(stderr, "NWCcGetConnInfo (%s) failed: %s\n",
			argv[optind], strnwerror(dserr));

	}
      if  (NWIsDSServer(conn2,NULL)) {
         dserr= NWDSGetServerDN(ctx,conn2,mybindctx2);
         if (dserr) {
		fprintf(stderr, "NWDSGetServerDN (%s) failed: %s\n",
			argv[optind], strnwerror(dserr));
	}
        printf ("DN name of  %s is %s\n",me2,mybindctx2);
        dserr= NWDSGetBinderyContext(ctx,conn2,mybindctx2);
        if (dserr) {
		fprintf(stderr, "NWDSGetBinderyContext(%s) failed: %s\n",
			argv[optind], strnwerror(dserr));
                 goto closing_all;
	}
        printf ("Bindery context on %s is %s\n",me2,mybindctx2);
      }
      else printf(" bindery server ?\n");

       // impossible to authenticate ( not_logged_in yet)
       // I still miss something ;-)
       // IT WORKS ONLY if kernel is compiled with option CONFIG_NCP_FS_NDS_DOMAINS
        // the connection is LOGGED-IN !
      dserr=NWDSAuthenticateConn(ctx,conn2);
      if (dserr) {
		fprintf(stderr, "NWDSAuthenticateConn failed: %s\n",
			 strnwerror(dserr));
	}
      printf (" after authenticating :\n");
      dserr=NWDSSpyConns(ctx,infos);
      printf ("%s\n",infos);

      // now ask him something that requires login  (some space restrictions on sys)
      // code from test_scan_volume_resctrictions
         {
          nuint32 sequence;
          NWVOL_RESTRICTIONS nwrests16;

          //struct ncp_bindery_object target;
          int i,temp;
          int isDSServer=NWIsDSServer(conn2,NULL);

          char currentUserName[MAX_DN_CHARS + 1];
          NWObjectType nwotype;

          sequence=0;
          temp =0;
              err=NWScanVolDiskRestrictions2 (conn2,0,&sequence,&nwrests16);
           if (err) {
	 	fprintf(stderr, "NWScanVolDiskRestrictions failed with error %s\n",
			strnwerror(err));
                 goto TOO_BAD_NOT_LOGGED_IN;
           }

          for (i=0; i<nwrests16.numberOfEntries;i++,temp++) {
            nuint32 nwuid= nwrests16.resInfo[i].objectID;
            nuint32 res=nwrests16.resInfo[i].restriction;

           // this test is required , otherwise we get a lot of user unknown by using
           // NWGetObjectName() against a NDS server.
           // we must use conn2 to map ID's since they are different on each server !
           if (isDSServer) {
             err =NWDSMapIDToName(ctx,conn2,nwuid,currentUserName);
             NWDSAbbreviateName(ctx,currentUserName,currentUserName);
             }
           else
             err=NWGetObjectName(conn2,nwuid, currentUserName,&nwotype);

            if (!err)
              if (res >=0x40000000L)
                  printf ("%-4d: %-20s no restriction\n",sequence,currentUserName);
              else
                 printf ("%-4d:%-20s %d Kb\n",sequence,currentUserName,res*4);

            else
	      printf ("%4d: %x ->%8d (unknown). Failed with error %s\n",
		        sequence,nwuid,res*4,strnwerror(err));
           }
         printf ("%d answers \n", temp);

       TOO_BAD_NOT_LOGGED_IN:;
       }


      //
       //I don't close it, so I can see it on monitor for about 5 minutes
      dserr=NWCCCloseConn(conn2);
       if (dserr) {
		fprintf(stderr, "NWDSCloseConnfailed: %s\n",
			 strnwerror(dserr));
	}

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
Tree Name is
Name context is [Root]
Server DN is  CN=CIPCINSA.O=PC
Bindery context on CN=CIPCINSA.O=PC is O=PC
 before authenticating :
connections:
state= 0,uid= 0,uid2=0,serv=EURINSA,usr=	state= 8000,uid= cf010001,uid2=cf010001,serv=CIPCINSA,usr=PPOLLET
DN name of  EURINSA is CN=EURINSA.O=PC
Bindery context on EURINSA is O=PC
 after authenticating :
connections:
state= 8000,uid= e6010001,uid2=e6010001,serv=EURINSA,usr=PPOLLET	state= 8000,uid= cf010001,uid2=cf010001,serv=CIPCINSA,usr=PPOLLET
16  :CN=C11.OU=GCP.O=PC   0 Kb
16  :CN=Pagustiballester.O=PC 1024 Kb
16  :CN=Gnoel.O=PC        1024 Kb
16  :CN=Gsauvage.O=PC     1024 Kb
16  :CN=P1.OU=R.O=PC      0 Kb
16  :CN=Gfullsack.O=PC    1024 Kb
16  :CN=Gmanenc.O=PC      1024 Kb
16  :CN=Glecaillon.O=PC   1024 Kb
16  :CN=C12.OU=GCP.O=PC   0 Kb
16  :CN=Ftremulot.O=PC    1024 Kb
16  :CN=Sbogdan.O=PC      1024 Kb
16  :CN=Fwagner.O=PC      1024 Kb
16  :CN=Fpohl.O=PC        1024 Kb
16  :CN=Danstotz.O=PC     1024 Kb
16  :CN=C13.OU=GCP.O=PC   0 Kb
16  :CN=Fmayor.O=PC       1024 Kb
16 answers

-------------------------- another server on another TREE (same user, same password)
it fails because in the ndai structure my name is stored as PPOLLET.PC
lªri:ÿÿÿÿ4POLLET.O=PCBV1.7bBCBA0BL¤NN8........

 via ncp_init
DONE
connections:
state= 8000,uid= cf010001,uid2=cf010001,serv=CIPCINSA,usr=PPOLLET
Tree Name is
Name context is [Root]
Server DN is  CN=CIPCINSA.O=PC
Bindery context on CN=CIPCINSA.O=PC is O=PC
 before authenticating :
connections:
state= 0,uid= 0,uid2=0,serv=GMD,usr=	state= 8000,uid= cf010001,uid2=cf010001,serv=CIPCINSA,usr=PPOLLET
DN name of  GMD is CN=GMD.O=GMD-INSA
Bindery context on GMD is O=GMD-INSA
NWDSAuthenticateConn failed: No such entry (-601)
 after authenticating :
connections:
state= 0,uid= 0,uid2=8081c24,serv=GMD,usr=	state= 8000,uid= cf010001,uid2=cf010001,serv=CIPCINSA,usr=PPOLLET


NWScanVolDiskRestrictions failed with error Unknown Server error (0x89FB)
   */
