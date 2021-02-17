/*
    vlist.c - List all volumes on a file server.
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

	0.00  2001			Patrick Pollet
		Initial revision.

really simple but can be called from tcl/tk front ends to fill
a listbox with available volumes on one server
 */

#ifdef N_PLAT_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include "private/libintl.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <nwcalls.h>
#include <nwnet.h>
#include <nwclxcon.h>
#include "getopt.h"
#define setlocale(a,b)
#define bindtextdomain(a,b)
#define textdomain(a)
static char* strnwerror(NWDSCCODE err) {
	static char x[100];
	sprintf(x, "Err: %d / 0x%X", err, err);
	return x;
}
#endif

int
main(int argc, char *argv[])
{
	NWCONN_HANDLE conn;
	long err;
        char * server;
        nuint v;
        char vName[256];

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);

	if (argc != 2 )
	{
		printf("usage: %s server\n", argv[0]);
		exit(1);
	}
	err = NWCallsInit(NULL, NULL);
	if (err) {
		fprintf(stderr, "NWCallsInit failed: %s\n", strnwerror(err));
		return 2;
	}
        server=argv[1];

        err= NWCCOpenConnByName (NULL,server,NWCC_NAME_FORMAT_BIND,0,NWCC_RESERVED,&conn);
        if (err) {
          fprintf(stderr,"trouble: %s\n", strnwerror(err));
          exit (1);
        }
        for (v=0; v<64;v++) {
          err=NWGetVolumeName(conn, v, vName);
          if (!err && vName[0]) printf ("%s\n",vName);
          else break;
        }
	NWCCCloseConn(conn);
	return 0;
}

