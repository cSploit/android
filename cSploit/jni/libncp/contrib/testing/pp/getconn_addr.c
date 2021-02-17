/*
    test_get_conn_addr.c
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

#ifdef N_PLAT_MSW4
char* strnwerror(int err) {
	static char errc[200];

	sprintf(errc, "error %u / %d / 0x%X", err, err, err);
	return errc;
}
#endif

int main(int argc, char *argv[]) {
	NWCONN_HANDLE conn;



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
        printf (" via ncp_init \n");
       // NB si on met 1 = login required , si il n'y a pas
       // de password dans ~./nwclient le demande !!!!
        conn=ncp_initialize(&argc,argv,0,&err);
         if (!conn) {
	    fprintf(stderr, "ncp_initialize failed with %s\n",strnwerror(err));
            err= 111;
            goto closing_ctx;
        }
        printf ("DONE\n");


        // testing code goes here



        {
        unsigned char myBuf[32];
        NWCCTranAddr ta;
	unsigned int i, mySize = 0;
        char me[128];


        /*
        printf (" direct reading: \n");
         if (conn->addr.any.sa_family == AF_IPX) {  printf ("TYPE:AF_IPX\n");
         if (conn->addr.any.sa_family == AF_INET) {  printf ("TYPE:AF_INET\n");
         if (conn->addr.any.sa_family == AF_UNIX) {  printf ("TYPE:AF_UNIX\n");
        */


        ta.buffer=myBuf;
        ta.len=sizeof(myBuf);

         err=NWCCGetConnInfo(conn,NWCC_INFO_TRAN_ADDR,sizeof(ta),&ta);
         if (err) {
           fprintf(stderr, "NWCCGetConnInfo TRAN_ADDR failed with %s\n",strnwerror(err));
           //goto closing_all;
        }

        printf (" got the following tran address: \n");
        printf ("TYPE:%d \n",ta.type);
        printf ("LEN: %d \n",ta.len);
        for (i=0; i<ta.len;i++)
          printf ("%x ",myBuf[i]);
        printf("\n");

        err=NWCCGetConnInfo(conn,NWCC_INFO_USER_NAME,sizeof(me),&me);
        if (err) {
           fprintf(stderr, "NWCCGetConnInfo failed with %s\n",strnwerror(err));
           goto closing_all;
        }
        printf("hello %s\n",me);
        err= NWCCGetConnAddressLength(conn,&mySize);
        if (err) {
           fprintf(stderr, "NWCCGetConnAddressLength failed with %s\n",strnwerror(err));
           //goto closing_all;
        }

        printf (" length of the tran address is:%d \n",mySize);
        err= NWCCGetConnAddress(conn,mySize,&ta);
        if(err) {
           fprintf(stderr, "NWCCGetConnAddress failed with %s\n",strnwerror(err));
           //goto closing_all;
        }

        printf (" got the following tran address: \n");
        printf ("TYPE:%d \n",ta.type);
        printf ("LEN: %d \n",ta.len);
        for (i=0; i<mySize;i++)
          printf ("%x ",myBuf[i]);
        printf("\n");
       }
        // end of testing code
        err=0;

closing_all:
	NWCCCloseConn(conn);
closing_ctx:
	// no ctx needed here
	return err;
}

/* testing

   */
