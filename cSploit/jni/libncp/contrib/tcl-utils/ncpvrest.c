/*
    ncpvrest.c

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


	1.00  2001, Feb 19 	Patrick Pollet <patrick.pollet@insa-lyon.fr>
		was named nwdsgetvolumerestrictions.c in /contrib/testing/pp

        1.01  2001  March 10th  Patrick Pollet <patrick.pollet@insa-lyon.fr>
		This program is ncpfs only
		Since this program uses nwcalls aonly, removed unneeded contexts

	1.02  2001  December 16th	Petr Vandrovec <vandrove@vc.cvut.cz>
		const <-> non const pointers cleanup

*/

#include <ncp/nwcalls.h>

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <wchar.h>

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
	         "usage: %s [options] \n"), progname);
	printf(_("\n"
	       "-h               Print this help text\n"
	       "-S server        Server to start with (default use .nwclient file)\n"
               "-N user name     default= current user\n"
               "-V volume        NO Default value \n"
	       "\n"));
}


int main(int argc, char *argv[]) {
	NWCCODE dserr;
	NWCONN_HANDLE conn;

        char * server=NULL;
	int opt;
        char *userToTest=NULL;
        static const char NO_VOLUME_BUT_NOT_NULL[]="Unknown Volume";
        const char *volumeToTest=NO_VOLUME_BUT_NOT_NULL;


         long err;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);

	progname = argv[0];

	NWCallsInit(NULL, NULL);

	while ((opt = getopt(argc, argv, "h?S:N:V:")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			goto finished;
		case 'S':
			server = optarg;
			break;
               	case 'N':
			userToTest = optarg;
			break;
              	case 'V':
			volumeToTest = optarg;
			break;

		default:
		  usage();
		  goto finished;
		}
	}

	if (server) {
          if  (server[0] == '/') {
		dserr = ncp_open_mount(server, &conn);
		if (dserr) {
			fprintf(stderr, "ncp_open_mount failed with %s\n",
				strnwerror(dserr));
			exit(111);

		}
	  } else {
		struct ncp_conn_spec connsp;
		memset(&connsp, 0, sizeof(connsp));
		strcpy(connsp.server, server);
		conn = ncp_open(&connsp, &err);
                if (!conn) {
	   	  fprintf(stderr, "ncp_open failed with %s\n",strnwerror(err));
		  exit(111);
                ;
                }
	     }

        }
        else {
               conn=ncp_initialize(&argc,argv,0,&err);
               if (!conn) {
		 fprintf(stderr, "ncp_initialize failed with %s\n",strnwerror(err));
		exit(111);

                }
        }

	/* testing code goes here*/

        {

        int optv=-1;
        nuint32 nwuid,rest,inUse;
        struct ncp_volume_info target;

         err=NWGetVolumeNumber(conn,volumeToTest,&optv);
         if (err) {
                fprintf(stderr,"%s: Volume %s does not exist\n", progname, volumeToTest);
                goto closing_all;
        }

         err=ncp_get_volume_info_with_number(conn, optv,&target);
         if (err) {
              fprintf(stderr,"%s: ncp_get_volume_info failed %s\n", progname,strnwerror(err) );
              goto closing_all;
        }


        if (!userToTest) {
                err=NWCCGetConnInfo (conn,NWCC_INFO_USER_ID,sizeof(nwuid),&nwuid);
                if (err) {
	         	fprintf(stderr, "NWCCGetConnInfo NWCC_INFO_USER_ID failed with error %s\n",
		        	strnwerror(err));
                        goto closing_all;
                }
                userToTest=malloc(256);
                err=NWCCGetConnInfo (conn,NWCC_INFO_USER_NAME,256,userToTest);
                if (err) {
	 	        fprintf(stderr, "NWCCGetConnInfo NWCC_INFO_USER_NAME failed with error %s\n",
			        strnwerror(err));
                         goto closing_all;
                }
        } else {
                err=NWGetObjectID(conn, userToTest, OT_USER, &nwuid);
                if (err) {
	                fprintf(stderr, "%s is unknown on this server. Failed with error %s\n",
			        userToTest,strnwerror(err));
                         goto closing_all;

                }
         }

         err=NWGetObjDiskRestrictions (conn,optv,nwuid,&rest,&inUse);
         if (err) {
		fprintf(stderr, "NWGetObjDiskRestrictions failed with error %s\n",
			strnwerror(err));
                goto closing_all;

        }
          if (rest==0x40000000)   /* no restriction */
                    rest=target.free_blocks;
           /* printf ("%s has %d Kb limit on %s and uses %d Kb\n",userToTest,rest*4,volumeToTest,inUse*4);*/
           printf ("%s:%s:%d:%d\n",volumeToTest,userToTest,rest*4,inUse*4);


       }/* end of testing code*/
        err=0;

closing_all:
	NWCCCloseConn(conn);
finished:
	return err;
}

/* testing

[root@pent107g-2 tcl-utils]# ./ncpvrest -V apps -N ppollet
apps:ppollet:223864:405632     <-- using ./nwclient
[root@pent107g-2 tcl-utils]# ./ncpvrest -V apps -N ppollet -S CIPCINSA
apps:ppollet:360816:96976
[root@pent107g-2 tcl-utils]# ./ncpvrest -V apps -N ppollet -S EURINSA
apps:ppollet:55748:226888
[root@pent107g-2 tcl-utils]# ./ncpvrest -V edu -N ppollet -S EURINSA
edu:ppollet:28448:272364
[root@pent107g-2 tcl-utils]# ./ncpvrest -V edu -N alexis -S EURINSA
edu:alexis:200:0
   */
