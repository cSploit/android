/*
    test_scan_volumes_restrictions.c
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


	1.00  2001, Jan 09		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Initial version.
		
	1.01  2001, March 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed example to work with corrected NWScanVolRestrictions*.
		Made changes to get it to compile under NWSDK: byteswapped ID,
			NWDSIsDSServer does not accept NULL argument, use
			NW* counterparts of ncp_* calls.

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
typedef nuint16 NWObjectType;
typedef u_int32_t Time_T;

unsigned long xswap(unsigned long p) {
	asm {
		mov eax,p
		xchg ah,al
		ror eax,16
		xchg ah,al
		mov p,eax
	};
	return p;
}

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

#define xswap(x) (x)
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
	         "usage: %s [options] \n"), progname);
	printf(_("\n"
	       "-h               Print this help text\n"
	       "-v value         Context DCK_FLAGS value (default 0)\n"
	       "-c context_name  Name of current context (default Root)\n"
	       "-C confidence    DCK_CONFIDENCE value (default 0)\n"
	       "-S server        Server to start with (default use .nwclient file)\n"
               "-V volume        NO Default value \n"
               "-l               Retrieve data by 16 blocks at atime (default=no , by 12)\n"
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
	int opt;
	NWVOL_NUM vol;
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;


        const char* NO_VOLUME_BUT_NOT_NULL="Unknown Volume";
        const char *volumeToTest=NO_VOLUME_BUT_NOT_NULL;
        int useBlock16=0;  // use old api by 12 blocks answers
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
	while ((opt = getopt(argc, argv, "h?lc:U:P:v:S:C:V:")) != EOF)
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
              	case 'V':
			volumeToTest = optarg;
			break;
                case 'l':
			useBlock16 = 1;
			break;

        	default:
		  usage();
		  goto finished;
		}
	}

	ctxflag |= DCV_XLATE_STRINGS | DCV_TYPELESS_NAMES;

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
               // must be logged to scan disk restrictions !!!!
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
#ifdef N_PLAT_LINUX
        {

              char infos[10000];
              dserr=NWDSSpyConns(ctx,infos);
              printf ("%s\n",infos);
         }
#endif
        {
        nuint32 sequence;
        NWVolumeRestrictions nwrests12;
        NWVOL_RESTRICTIONS nwrests16;
        int i,temp;

        char currentUserName[MAX_DN_CHARS + 1];
        NWObjectType nwotype;
	/* NWSDK does not allow NULL tree name */
        int isDSServer=NWIsDSServer(conn, currentUserName);


        printf ("testing %s:with %d blocks\n ",volumeToTest,useBlock16?16:12);

         err=NWGetVolumeNumber(conn,volumeToTest,&vol);
         //err=ncp_get_volume_number(conn, volumeToTest, &vol);
         if (err) {
                fprintf(stderr,"%s: Volume %s does not exist\n", progname, volumeToTest);
                goto closing_all;
        }
        printf ("%s has number %d\n",volumeToTest, vol);

	sequence = 0;
	temp = 0;
	do {
		if (useBlock16)
			err = NWScanVolDiskRestrictions2(conn, vol, &sequence, &nwrests16);
		else {
			err = NWScanVolDiskRestrictions(conn, vol, &sequence, &nwrests12);
			memcpy (&nwrests16,&nwrests12,sizeof(nwrests12));
		}
		if (err) {
			fprintf(stderr, "NWScanVolDiskRestrictions failed with error %s\n",
				strnwerror(err));
			goto closing_all;
		}
		printf("%u restrictions, sequence = %u\n", nwrests16.numberOfEntries, sequence);

		for (i = 0; i < nwrests16.numberOfEntries; temp++, i++) {
			nuint32 nwuid = xswap(nwrests16.resInfo[i].objectID);
			nuint32 res = nwrests16.resInfo[i].restriction;

			// this test is required , otherwise we get a lot of user unknown by using
			// NWGetObjectName() against a NDS server.
			if (isDSServer) {
				err = NWDSMapIDToName(ctx, conn, nwuid, currentUserName);
				NWDSAbbreviateName(ctx, currentUserName, currentUserName);
			} else
				err = NWGetObjectName(conn, nwuid, currentUserName, &nwotype);

			if (!err)
				if (res >=0x40000000L)
					printf ("%-4u: %-20s no restriction\n", temp, currentUserName);
				else
					printf ("%-4u:%-20s %d Kb\n", temp, currentUserName, res*4);
			else
				printf ("%4u: %x ->%8d (unknown). Failed with error %s\n",
					temp, nwuid, res*4, strnwerror(err));
			sequence++;
		}

	} while (nwrests16.numberOfEntries > 0);
	printf ("%d answers \n", temp);

	}
        // end of testing code
	err = 0;

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

   */
