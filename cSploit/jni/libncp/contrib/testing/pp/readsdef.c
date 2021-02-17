/*
    test_read_syntaxes_def.c - Testing of NWDSReadSyntaxDef API call
    a simple table reading
    Copyright (C) 2000  Patrick Pollet

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

	1.00  2001, Jan 03		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Initial version.

*/

#define debug  1


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
char* strnwerror(int err) {
	static char errc[200];

	sprintf(errc, "error %u / %d / 0x%X", err, err, err);
	return errc;
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
#endif

static char *progname;



static void printflags (nint16 flag) {
	static const char* RULES[] = {
			"DS_STRING",		"DS_SINGLE_VALUED",	
			"DS_SUPPORT_ORDER",	"DS_SUPPORT_EQUALS",
			"DS_IGNORE_CASE",	"DS_IGNORE_SPACE",
			"DS_IGNORE_DASH",	"DS_ONLY_DIGITS",
			"DS_ONLY_PRINTABLE",	"DS_SIZEABLE",
			"DS_BITWISE_EQUALS"};
	static unsigned int RULESMASKS[] = {
			0x0001,			0x0002,
			0x0004,			0x0008,
			0x0010,			0x0020,
			0x0040,			0x0080,
			0x0100,			0x0200,
			0x0400,			0};

	const char* s = "";
	unsigned int i;

	printf ("[");
	for (i = 0; RULESMASKS[i]; i++) {
		if (flag & RULESMASKS[i]) {
			printf("%s%s", s, RULES[i]);
			s = ", ";
		}
	}
	if (!s) {
		printf("-");
	}
	printf ("]");
}

static void
usage(void)
{
	fprintf(stderr, _("usage: %s serverDN \n"), progname);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s serverDN \nn"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
	       "serverDN        Distinguished name of server to read schema from\n"
	       "\n"));
}



char buf[MAX_DN_CHARS + 1];
char buf2[MAX_DN_CHARS + 1];

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWDSContextHandle ctx;
	NWCONN_HANDLE conn;

	const char* server = "CIPCINSA";

	int opt;
        nuint32 ctxflag=0;
        long err;

	progname = argv[0];
#ifndef N_PLAT_MSW4
        setlocale(LC_ALL, "");
        bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
        textdomain(NCPFS_PACKAGE);
#endif
	NWCallsInit(NULL, NULL);

#ifndef N_PLAT_MSW4

	NWDSInitRequester();
#endif

	dserr = NWDSCreateContextHandle(&ctx);
	if (dserr) {
		fprintf(stderr, "NWDSCreateContextHandle failed with %s\n", strnwerror(dserr));
		return 123;
	}
	while ((opt = getopt(argc, argv, "h?")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			goto finished_ctx;
		default:
			usage();
			goto finished_ctx;
		}
	}

	if (optind + 1 > argc) {
                printf ("????");
		usage();
		goto finished_ctx;
	}
	server = argv[optind];
        /*
        // ne change rien ? pourquoi ????
	dserr = NWDSSetContext(ctx, DCK_NAME_CONTEXT, context);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_NAME8CTX) failed: %s\n",
			strnwerror(dserr));
		goto finished_ctx;
	}
        */
#ifndef N_PLAT_MSW4
	{
		static const u_int32_t add[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
		dserr = NWDSSetTransport(ctx, 16, add);
		if (dserr) {
			fprintf(stderr, "NWDSSetTransport failed: %s\n",
				strnwerror(dserr));
			return 124;
		}
	}
#endif
#ifdef N_PLAT_MSW4
	dserr = NWDSOpenConnToNDSServer(ctx, server, &conn);
#else
	if (server[0] == '/') {
		dserr = ncp_open_mount(server, &conn);
		if (dserr) {
			fprintf(stderr, "ncp_open_mount failed with %s\n",
				strnwerror(dserr));
			return 111;
		}
	} else {
		struct ncp_conn_spec connsp;

		memset(&connsp, 0, sizeof(connsp));
		strcpy(connsp.server, server);
		conn = ncp_open(&connsp, &err);
		if (!conn) {
			fprintf(stderr, "ncp_open failed with %s\n",
				strnwerror(err));
			return 111;
		}
	}
	dserr = NWDSAddConnection(ctx, conn);
#endif
	if (dserr) {
		fprintf(stderr, "Cannot bind connection to context: %s\n",
			strnwerror(dserr));
	}

        ctxflag |= DCV_XLATE_STRINGS|DCV_TYPELESS_NAMES;

	dserr = NWDSSetContext(ctx, DCK_FLAGS, &ctxflag);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_FLAGS) failed: %s\n",
			strnwerror(dserr));
		goto finished_ctx;
	}

        dserr = NWDSGetContext(ctx, DCK_TREE_NAME, buf);
	if (dserr) {
		fprintf(stderr, "NWDSGetContext(DCK_TREE_NAME) failed: %s\n",
			strnwerror(dserr));
		goto finished_ctx;
	}
        printf ("Tree Name is %s \n",buf);
	dserr = NWDSGetContext(ctx, DCK_NAME_CONTEXT, buf);
	if (dserr) {
		fprintf(stderr, "NWDSGetContext(DCK_NAME_CTX) failed: %s\n",
			strnwerror(dserr));
		goto finished_ctx;
	}
        printf ("Name context is %s \n",buf);

     dserr= NWDSGetServerDN(ctx,conn,buf);
      if (dserr) {
		fprintf(stderr, "NWDSGetServerDN (%s) failed: %s\n",
			argv[optind], strnwerror(dserr));
                goto finished_conn;
	}

      printf ("Server DN is  %s \n",buf);

      {
        Syntax_Info_T st;
        int i;


	dserr = NWDSGetContext(ctx, DCK_FLAGS, &ctxflag);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_FLAGS) failed: %s\n",
			strnwerror(dserr));
		goto finished_ctx;
	}
        printf ("*** flags are %x\n",ctxflag);
        for ( i=0;i<28;i++) {
          if ((err=NWDSReadSyntaxDef (ctx,i,&st)) != 0) {
		fprintf(stderr, "NWDSReadSyntaxDef  failed: %s\n",strnwerror(err));
                goto finished_conn;
	 }
         printf ("%-8u",st.ID);
         printf ("%-24s",st.defStr);
         /***
         for (j=0;j<MAX_SCHEMA_NAME_BYTES;j++)
            printf ("%2x",st.defStr[j]);
             //printf ("%s\t",st.defStr);
          printf ("\n");
         ***/
         printflags (st.flags);
         printf("\n");
         //printf ("%s",L"hello pp");
        }

      }


finished_conn:
	NWCCCloseConn(conn);

finished_ctx:
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 121;
	}
	return 0;
}




