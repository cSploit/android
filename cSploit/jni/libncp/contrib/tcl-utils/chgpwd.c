/*
    chgpwd.c - NWDSChangeObjectPasswor() API demo
    Copyright (C) 2000  Petr Vandrovec

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

	1.00  2000, April 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.01  2001, February 23		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added '-r' option. Feature request from Patrick Pollet.

	1.02  2001, March 14		Patrick Pollet
		Added -T' option.
		Added errors code specific to password changing

	1.03  2001, December 16		Petr Vandrovec <vandrove@vc.cvut.cz>
		const <-> non-const pointers cleanup
		Moved error codes to libncp.

        1.04 2002 March 14 , Patrick Pollet
		try to fix absent context problem by retrieving Default context if it exists

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
#include <ncp/nwclient.h>

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
	         "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-h              Print this help text\n"
	       "-v value        Context DCK_FLAGS value (default 0)\n"
	       "-o object_name  Name of object (default [Root])\n"
	       "-c context_name Name of current context (default [Root])\n"
	       "-C confidence   DCK_CONFIDENCE value (default 0)\n"
	       "-S server       Server to start with (no default)\n"
               "-T tree         NDS tree (no default)\n"
	       "-P password     Old password (default TEST)\n"
	       "-n newpassword  New password (default T2)\n"
	       "-r              Raw, do not uppercase passwords\n"
	       "\n"));
}

#ifdef N_PLAT_MSW4
char* strnwerror(int dserr) {
	static char errc[200];

	sprintf(errc, "error %u / %d / 0x%X", dserr, dserr, dserr);
	return errc;
}
#endif


static const char*
strnwerror2(int err)
{
        return strnwerror(err);
}


/* Uppercase string. Do NOT write to string if all characters are
   uppercase, const uppercased strings (TEST, T2) may be passed
   to this function! */
static void safe_strupr(char* p) {
	while (*p) {
		if (islower(*p))
			*p = toupper(*p);
		p++;
	}
}

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWDSContextHandle ctx;
	NWCONN_HANDLE conn=NULL;
	const char* objectname = "[Root]";
	char	    context[MAX_TREE_NAME_CHARS +1]="";
	const char* server = NULL;
	const char* pwd = "TEST";
	const char* newpwd = "T2";
	char	    treeName[MAX_TREE_NAME_CHARS +1]="";
	int opt;
	int raw = 0;
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;

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

	while ((opt = getopt(argc, argv, "h?o:c:v:S:C:P:n:T:r")) != EOF)
	{
		switch (opt)
		{
		case 'C':
			confidence = strtoul(optarg, NULL, 0);
			break;
		case 'o':
			objectname = optarg;
			break;
		//case 'c':
		//	context = optarg;
		//	break;
		case 'c':
			if (strlen(optarg) <sizeof(context))
				strcpy(context,optarg);
			else {
				fprintf(stderr,"failed:parameter context is too long.\n");
				exit(3);
			}
			break;
		case 'P':
			pwd = optarg;
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
               	case 'T': if (strlen(optarg) <sizeof(treeName))
                                strcpy(treeName,optarg);
                       else {
                                fprintf(stderr,"failed:parameter treename is too long.\n");
                                exit(3);
                       }
			break;
                case 'n':
			newpwd = optarg;
			break;
		case 'r':
			raw = 1;
			break;
		default:
			usage();
			goto finished;
		}
	}
	if (!raw) {
		safe_strupr(pwd);
		safe_strupr(newpwd);
	}

       if (server && treeName[0]) {
                fprintf(stderr, "failed:cannot have both -T tree and -S server options\n");
                exit (4);
        }


       	dserr = NWDSCreateContextHandle(&ctx);
	if (dserr) {
		fprintf(stderr, "NWDSCreateContextHandle failed with %s\n", strnwerror(dserr));
		return 123;
	}

        ctxflag |= DCV_XLATE_STRINGS;
	ctxflag |= DCV_CANONICALIZE_NAMES;

	dserr = NWDSSetContext(ctx, DCK_FLAGS, &ctxflag);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_FLAGS) failed: %s\n",
			strnwerror(dserr));
		dserr= 120;
                goto finished;
	}

 	if (!context[0])
  		NWCXGetDefaultNameContext(treeName,context,sizeof(context));
                /* got one */

    	if (context[0]) {
	dserr = NWDSSetContext(ctx, DCK_NAME_CONTEXT, context);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_NAME_CONTEXT) failed: %s\n",
			strnwerror(dserr));
		dserr= 121;
                goto finished;
	}
	}
	dserr = NWDSSetContext(ctx, DCK_CONFIDENCE, &confidence);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_CONFIDENCE) failed: %s\n",
			strnwerror(dserr));
		dserr=122;
                goto finished;
	}
#ifndef N_PLAT_MSW4
	{
		static const u_int32_t add[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
		dserr = NWDSSetTransport(ctx, 16, add);
		if (dserr) {
			fprintf(stderr, "NWDSSetTransport failed: %s\n",
				strnwerror(dserr));
			dserr= 124;
                        goto finished;
		}
	}
#endif
#ifdef N_PLAT_MSW4
	dserr = NWDSOpenConnToNDSServer(ctx, server, &conn);
#else
        if (server) {
	        if (server[0] == '/') {
        		dserr = ncp_open_mount(server, &conn);
	        	if (dserr) {
		        	fprintf(stderr, "ncp_open_mount failed with %s\n",
			        	strnwerror(dserr));
			        dserr= 112;
                                goto finished;
        		}
	        } else {
		        struct ncp_conn_spec connsp;
                        long err;

		        memset(&connsp, 0, sizeof(connsp));
		        strcpy(connsp.server, server);
		        conn = ncp_open(&connsp, &err);
		        if (!conn) {
			        fprintf(stderr, "ncp_open failed with %s\n",
				        strnwerror(dserr));
			        dserr=113;
                                goto finished;
		        }
	        }
        } else {
                if (!treeName[0]) {
                         NWCXGetPreferredDSTree(treeName,sizeof(treeName));

                }
                if (!treeName[0]) {
                        fprintf(stderr,"failed: You must specify a server or a tree\n");
                        dserr=114;
                        goto finished;
                }

                if ((dserr=NWCXAttachToTreeByName (&conn,treeName))) {
                        fprintf(stderr,"failed:Cannot attach to tree %s.dserr:%s\n",
                                    treeName,strnwerror(dserr));
                        dserr=115;
                        goto finished;

                }

        }

	dserr = NWDSAddConnection(ctx, conn);
#endif
	if (dserr) {
		fprintf(stderr, "Cannot bind connection to context: %s\n",
			strnwerror(dserr));
	}

        {
        char tstcontext[256];
        dserr = NWDSGetContext(ctx, DCK_NAME_CONTEXT, tstcontext);
	if (dserr) {
		fprintf(stderr, "NWDSFetContext(DCK_NAME_CONTEXT) failed: %s\n",
			strnwerror(dserr));
		dserr= 121;
                goto finished;
	}
        /*printf ("changing password of user %s.%s\n",objectname,tstcontext);*/
        }
        dserr = NWDSChangeObjectPassword(ctx, NDS_PASSWORD, objectname, pwd, newpwd);
	if (dserr) {
		fprintf(stderr, "Change password failed: %s\n",
			strnwerror2(dserr));
	}
finished:
        if (conn) NWCCCloseConn(conn);
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		dserr=121;

	}
	return dserr;
}
