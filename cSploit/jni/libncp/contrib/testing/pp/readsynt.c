/*
    test_read_syntaxes.c - Testing of NWDS*Syntaxes API call
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

	1.00  2001, Jan 16		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Initial version.
       UNSOLVED : the receivibng buffer for the syntax NAME must be
                   MAX_SCHEMA_NAME_BYTES in size and not MAX_SCHEMA_NAME_CHAR 
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



/************************************ syntax matching rules
DS_STRING 0x0001 Indicates that the syntax can contain string values and
therefore attributes with this syntax can be used as naming
attributes.
DS_SINGLE_VALUED 0x0002 Indicates that attributes using this syntax can have only one
value.
DS_SUPPORTS_ORDER 0x0004 Indicates that attributes using this syntax must be open to
comparisons of less than, equal to, and greater than.
DS_SUPPORTS_EQUAL 0x0008 Indicates that attributes using this syntax match for equality
when all of the following conditions are met. The attributes
values are identical; the attributes use the same syntax,
and the attributes data type conforms to the syntax.
DS_IGNORE_CASE 0x0010 Indicates that attributes using this syntax ignore case
during comparisons.
DS_IGNORE_SPACE 0x0020 Indicates that attributes using this syntax ignore extra
space during comparisons.
DS_IGNORE_DASH 0x0040 Indicates that attributes using this syntax ignore dashes
during comparisons.
DS_ONLY_DIGITS 0x0080 Indicates that attributes using this syntax must support only
digits in their values.
DS_ONLY_PRINTABLE 0x0100 Indicates that attributes using this syntax must support only
printable characters in their values. For a list of these
characters, see the Printable String syntax.
DS_SIZEABLE 0x0200 Indicates that attributes using this syntax must set upper
and lower limits to their values.
DS_BITWISE_EQUAL 0x0400 Indicates that attributes using this syntax support substring
(wildcard) and approximate matching.
****************************************************************************/
static const char* RULES[]={"DS_STRING","DS_SINGLE_VALUED","DS_SUPPORT_ORDER","DS_SUPPORT_EQUALS",
                    "DS_IGNORE_CASE","DS_IGNORE_SPACE","DS_IGNORE_DASH","DS_ONLY_DIGITS",
                    "DS_ONLY_PRINTABLE","DS_SIZEABLE","DS_BITWISE_EQUALS"};
static nint16 RULESMASKS[]={0x0001,0x0002,0x0004,0x0008,0x0010,0x0020,0x0040,0x0080,0x0100,0x0200,0x0400};

#define nflags sizeof(RULESMASKS)/sizeof(nint16)

static void print_flag (nint16 flag) {
  unsigned i;
  printf ("[");
  for (i=0;i <nflags; i++)
   if ((flag & RULESMASKS[i])!=0) printf("%s ",RULES[i]);
  printf ("]");
}


//NWDSCCODE read_one_syntax_def (NWDSContextHandle ctx, char *syntaxName){
static NWDSCCODE read_some_syntax_def (NWDSContextHandle ctx, char **syntaxNames, int optIndex){
 pBuf_T              pOutBuf;
 pBuf_T              pInBuf;
 NWDSCCODE           ccode;
 nuint32              iterHandle = NO_MORE_ITERATIONS;
 nuint32             count,i;
 char                foundsyntaxName[MAX_SCHEMA_NAME_BYTES+1];
 Syntax_Info_T       syntaxInfo;
 char *              sn;

  printf ("entering read_one_syntax_def\n");


ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pInBuf);
   if(ccode)
   {
      printf("\nNWDSAllocBuf returned %X", ccode);
      return ccode;
   }
ccode = NWDSInitBuf (ctx,DSV_READ_SYNTAXES,pInBuf);
   if(ccode)
   {
      printf("\nNWDSInitBuf returned %X", ccode);
      goto free_bufs;
   }

ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pOutBuf);
   if(ccode)
   {
      printf("\nNWDSAllocBuf returned %X", ccode);
      goto free_bufs;
   }

 while ((sn=syntaxNames[optIndex++])) {
  printf ("NWDSPutSyntaxName call with %s\n",sn);
  ccode = NWDSPutSyntaxName(ctx,pInBuf,sn);
   if(ccode)
   {
      printf("\nNWDSPutSyntaxName returned %X", ccode);
      goto free_bufs2;
   }

 }
 printf ("NWDSReadSyntax call\n");
 /* Loop while there is data in the buffer Needed ???*/
   do
   {
      ccode = NWDSReadSyntaxes(ctx, DS_SYNTAX_DEFS, 0, pInBuf,
                               &iterHandle, pOutBuf);
      if (ccode)
      {
         printf("\nNWDSReadSyntaxes returned %X", ccode);
         goto free_bufs;
      }
       printf ("NWDSReadSyntax returns \n");
      /* Get and print data found in the buffer */
      ccode = NWDSGetSyntaxCount(ctx, pOutBuf, &count);
      if(ccode)
      {
         printf("\nNWDSGetSyntaxCount returned %X", ccode);
         goto free_bufs;
      }

      printf("\nNumber of syntaxes: %d\n", count);

      /* Loop for each class name found */
      for (i = 0; i < count; i++)
      {
         ccode = NWDSGetSyntaxDef(ctx, pOutBuf, foundsyntaxName,&syntaxInfo);
         if(ccode)
         {
             printf("\nNWDSGetSyntaxDef returned %X", ccode);
             goto free_bufs;
         }
         printf("SN: %-29s SD: %-22s %-2u %4x ", foundsyntaxName, syntaxInfo.defStr,
	 		syntaxInfo.ID, syntaxInfo.flags);
         print_flag(syntaxInfo.flags);
         printf("\n");
      }

   } while (iterHandle != NO_MORE_ITERATIONS);  /* read loop */

free_bufs2:;
 NWDSFreeBuf(pOutBuf);
free_bufs:;
 NWDSFreeBuf(pInBuf);
 return ccode;
}


static NWDSCCODE read_all_syntax_names (NWDSContextHandle ctx){
 pBuf_T              pOutBuf;
 NWDSCCODE           ccode;
 nuint32              iterHandle = NO_MORE_ITERATIONS;
 nuint32             count,i;
 char                syntaxName[MAX_SCHEMA_NAME_BYTES+1];



  printf ("entering read_all_syntax_names\n");

ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pOutBuf);
   if(ccode)
   {
      printf("\nNWDSAllocBuf returned %X", ccode);
      return ccode;
   }

 /* Loop while there is data in the buffer */
   do
   {
      ccode = NWDSReadSyntaxes(ctx, DS_SYNTAX_NAMES, 1, NULL,
                               &iterHandle, pOutBuf);
      if (ccode)
      {
         printf("\nNWDSReadSyntaxes returned %d", ccode);
         goto free_bufs;
      }

      /* Get and print data found in the buffer */
      ccode = NWDSGetSyntaxCount(ctx, pOutBuf, &count);
      if(ccode)
      {
         printf("\nNWDSGetSyntaxCount returned %X", ccode);
         goto free_bufs;
      }

      printf("\nNumber of syntaxes: %d\n", count);

      /* Loop for each class name found */
      for (i = 0; i < count; i++)
      {
         ccode = NWDSGetSyntaxDef(ctx, pOutBuf, syntaxName,NULL);
         if(ccode)
         {
             printf("\nNWDSGetSyntaxDef returned %X", ccode);
             goto free_bufs;
         }
         printf("SN: %s\n", syntaxName);
#if 0
          {int j;
            for (j=0;j<MAX_SCHEMA_NAME_BYTES+1;j++){
              nuint8 c;
              c=BVAL(syntaxName,j);
              printf("%04X|",c);
          }
          printf("\n");
          }
#endif

      }

   } while (iterHandle != NO_MORE_ITERATIONS);  /* read loop */


free_bufs:;
 NWDSFreeBuf(pOutBuf);
 return ccode;


}

static NWDSCCODE read_all_syntax_defs (NWDSContextHandle ctx){
 pBuf_T              pOutBuf;
 NWDSCCODE           ccode;
 nuint32             iterHandle = NO_MORE_ITERATIONS;
 nuint32             count,i;
 char                syntaxName[MAX_SCHEMA_NAME_BYTES+1];
 Syntax_Info_T       syntaxInfo;

ccode = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &pOutBuf);
   if(ccode)
   {
      printf("\nNWDSAllocBuf returned %X", ccode);
      return ccode;
   }

  printf ("entering read_all_syntax_defs\n");

 /* Loop while there is data in the buffer */
   do
   {
      ccode = NWDSReadSyntaxes(ctx, DS_SYNTAX_DEFS, 1, NULL,
                               &iterHandle, pOutBuf);
      if (ccode)
      {
         printf("\nNWDSReadSyntaxes returned %X", ccode);
         goto free_bufs;
      }

      /* Get and print data found in the buffer */
      ccode = NWDSGetSyntaxCount(ctx, pOutBuf, &count);
      if(ccode)
      {
         printf("\nNWDSGetSyntaxCount returned %X", ccode);
         goto free_bufs;
      }

      printf("\nNumber of syntaxes: %d\n", count);

      /* Loop for each class name found */
      for (i = 0; i < count; i++)
      {
         ccode = NWDSGetSyntaxDef(ctx, pOutBuf, syntaxName,&syntaxInfo);
         if(ccode)
         {
             printf("\nNWDSGetSyntaxDef returned %X", ccode);
             goto free_bufs;
         }
         printf("SN: %-29s SD: %-22s %-2u %4x ", syntaxName, syntaxInfo.defStr,
	 		syntaxInfo.ID, syntaxInfo.flags);
         print_flag(syntaxInfo.flags);
         printf("\n");
      }

   } while (iterHandle != NO_MORE_ITERATIONS);  /* read loop */

free_bufs:
 NWDSFreeBuf(pOutBuf);
 return ccode;
}

static NWDSCCODE read_syntaxes (NWDSContextHandle ctx, char **syntaxNames, int optIndex,int namesAndDefs) {

  if (syntaxNames[optIndex])
     return read_some_syntax_def (ctx,syntaxNames,optIndex);
  else if (namesAndDefs) return read_all_syntax_defs (ctx);
     else return read_all_syntax_names (ctx);

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
		 "usage: %s [options] serverDN [syntax_name...]\nn"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
               "-n              syntax names only (default)\n"
               "-d              syntax names and defs \n"
               "-x              do not xlate strings (default=yes) \n"
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
        int showdefs=0;
        int xlate=1;

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
	while ((opt = getopt(argc, argv, "h?ndx")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			goto finished_ctx;
                case 'd': showdefs=1; break;
                case 'n': showdefs=0; break;
                case 'x': xlate=0; break;
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

        if (xlate)
         ctxflag |= DCV_XLATE_STRINGS|DCV_TYPELESS_NAMES;
        else
          ctxflag |=DCV_TYPELESS_NAMES;

	dserr = NWDSSetContext(ctx, DCK_FLAGS, &ctxflag);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_FLAGS) failed: %s\n",
			strnwerror(dserr));
		goto finished_ctx;
	}
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
		long err;

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
       read_syntaxes (ctx, argv,optind+1,showdefs);

finished_conn:
	NWCCCloseConn(conn);

finished_ctx:
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 121;
	}
        exit (0);
}




