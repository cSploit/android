/*
    vlist.c - List all volumes on a file server,even not connected.
       really simple but can be called from tcl/tk front ends to fill
       a listbox with available volumes on one server
    Copyright (C) 2001 by Patrick Pollet

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

	0.00  2001	February 2001		Patrick Pollet
		Initial revision.

        1.00  2001	March 7rd  2001		Patrick Pollet
		Added the word "failed" to all error messages to
                simplify TCL/Tk parsing.
                Use the "new ncp_volume_list_* API calls.


 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>

#include "private/libintl.h"
#define _(X) gettext(X)

static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("failed. usage: %s [options] servername\n"), progname);
        exit(1);
}

static void
help(void)
{
	printf(_("\n"
	         "failed.usage: %s [options] servername\n"), progname);
	printf(_("\n"
	       "-h                Print this help text\n"
	       "-n namespace      0=DOS, 1=MAC, 2=NFS, 3=FTAM, 4=OS2\n"
	       "\n"));
        exit(1);
}



int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	NWDSCCODE  err;
        char * server;
        unsigned int v;
        char vName[256];
      	u_int32_t destns = NW_NS_DOS;
	NWVOL_HANDLE handle;
    	int opt;


        progname = argv[0];
        setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);

        NWCallsInit(NULL, NULL);
//	NWDSInitRequester();


       	while ((opt = getopt(argc, argv, "h?n:")) != EOF)
	{
		switch (opt)
		{
		case 'n':{  char *zz=NULL;
			        destns = strtoul(optarg, &zz, 0);
                                if (zz && *zz && !isspace(zz)){
                                          printf ("failed: Value %s for optional name space is not a number.\n",optarg);
                                          usage();
                                }
			        break;
                         }
		case 'h':
		case '?':
			help();

		default:
			usage();

		}
	}


	if (optind >=argc )
	{
		usage();
	}
        server=argv[optind];

        err= NWCCOpenConnByName (NULL,server,NWCC_NAME_FORMAT_BIND,0,NWCC_RESERVED,&conn);
        if (err) {
          fprintf(stderr,"failed:unable to open connection to %s",server);
          exit (1);
        }
        /********* old code
        for (v=0; v<64;v++) {
          err=NWGetVolumeName(conn,v,&vName);
          if (!err && vName[0]) printf ("%s\n",vName);
          else break;
        }
        *************************/
       	err = ncp_volume_list_init(conn, destns, 1, &handle);
	if (err)
		fprintf(stderr, "failed: Cannot initialize list: %s (%08X)\n", strnwerror(err), err);
	else {
		while ((err = ncp_volume_list_next(handle, &v, vName, sizeof(vName))) == 0) {
			printf("%s\n", vName);
		}
		err=ncp_volume_list_end(handle);
	}
	NWCCCloseConn(conn);
        /*printf ("err=%d\n",err);*/
	return err;
}


