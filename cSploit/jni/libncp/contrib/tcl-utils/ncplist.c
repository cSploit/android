/*
    ncplist.c from
    nwdslist.c - NWDSList() and NWDSExtSyncList() API demo
    Copyright (C) 1999  Petr Vandrovec

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

	0.00  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

	1.01  2000, April 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSGetCountByClassAndName demo.

	1.02  2000, April 30		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSExtSyncList demo.

	1.03  2001, February 17		Patrick Pollet <patrick.pollet@insa-lyon.fr>
             Duplicated in /contrib/tcl-utils from /contrib/testing/nwdslist.c
             and renamed ncplist.c
                Added flag quiet to reduce output lines to a minimum
                Added flag abbreviate to trim out context part ( -c required)
                This program is meant to be called by a TCL/tk from end in -Q mode
                          so it should emit  the word "failed" in case of trouble
                          somewhere in the line.
                Added option -T tree
                Changed parameters -O -> -o and -C -> -c for consitency with other ncp utilities.
                TCL/tk usage to get list of contexts :
                if {$context !=""}{
                        catch  {exec  ncplist -T $tree -v 4 -Q -A -o $context -c $context -l "Org*"} result
                }else{
                        catch  {exec  ncplist -T $tree -v 4 -Q -l "Org*"} result
               }

               to get list of volumes (NDS format) ncplist -T tree -v 4 -Q -A -o ctx -c ctx -l "Vol*"

	1.04  2001, March 10		Patrick Pollet <patrick.pollet@insa-lyon.fr>
               Corrected #196 : check size of parameter -T
               Corrected #252 : removed wrong comments
               Modified  #237 : tretament of tree moved later  (if (!server))
	       
	1.05  2001, December 16		Petr Vandrovec <vandrove@vc.cvut.cz>
		const <-> non const pointers cleanup

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
	       "-c context_name Name of current context (default OrgUnit.Org.Country)\n"
	       "-C confidence   DCK_CONFIDENCE value (default 0)\n"
	       "-S server       Server to start with (default CDROM)\n"
               "-T tree         Tree to begin with (default env variable)\n"
	       "-q req_type     Info type (default 0)\n"
	       "-s size         Reply buffer size (default 4096)\n"
	       "-f subject_name Filter for subordinates (default NONE, may contain wildcards)\n"
	       "-l class_name   Filter for base class names (default NONE)\n"
	       "-t timestamp    Return objects after this time (default ALL)\n"
               "-Q              Short output (only names of objects found) \n"
               "-A              Abbreviate name if possible (-c required)\n"
	       "\n"));
}

#ifdef N_PLAT_MSW4
char* strnwerror(int err) {
	static char errc[200];

	sprintf(errc, "failed: error %u / %d / 0x%X", err, err, err);
	return errc;
}
#endif

int main(int argc, char *argv[]) {
	NWDSCCODE dserr;
	NWDSContextHandle ctx;
	NWCONN_HANDLE conn;
	const char* objectname = "[Root]";
	const char* context = "";
	const char* server = NULL;
        const char* classname = NULL;
	const char* subjectname = NULL;
	TimeStamp_T ts = { 0, 0, 0 };
	int ts_defined = 0;
	int opt;
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;
	u_int32_t req = 0;
	nuint32 ih = NO_MORE_ITERATIONS;
	Buf_T* buf;
	size_t size = DEFAULT_MESSAGE_LEN;
	size_t cnt;

        char     treeName    [MAX_TREE_NAME_CHARS +1]="";
        char     serverName  [MAX_SCHEMA_NAME_CHARS+1]="";
        int quiet=0;       /*default = not quiet*/
        int abbreviate=0;

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
	while ((opt = getopt(argc, argv, "h?QAo:c:v:S:C:q:s:f:l:t:T:")) != EOF)
	{
		switch (opt)
		{
		case 'C':
			confidence = strtoul(optarg, NULL, 0);
			break;
		case 'o':
			objectname = optarg;
			break;
		case 'c':
			context = optarg;
			break;
		case 'v':
			ctxflag = strtoul(optarg, NULL, 0);
			break;
		case 'q':
			req = strtoul(optarg, NULL, 0);
			break;
		case 's':
			size = strtoul(optarg, NULL, 0);
			break;
		case 'h':
		case '?':
			help();
			goto finished;
		case 'S':
			server = optarg;
			break;
                case 'T':
                        if (strlen(optarg) <sizeof(treeName))
                                strcpy(treeName,optarg);
                        else {
                                fprintf(stderr,"failed:parameter treename is too long.\n");
                                exit(3);
                        }
                        break;
		case 'f':
			subjectname = optarg;
			break;
		case 'l':
			classname = optarg;
			break;
		case 't':
			ts.wholeSeconds = strtoul(optarg, NULL, 0);
			ts_defined = 1;
			break;
                case 'Q':
			quiet=1;
			break;
                case 'A':
			abbreviate=1;
			break;
		default:
			usage();
			goto finished;
		}
	}

        if (server && treeName[0]) {
          	fprintf(stderr, "failed:cannot have both -T tree and -S server options\n");
		exit(101);
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
		return 122;
	}
	dserr = NWDSSetContext(ctx, DCK_CONFIDENCE, &confidence);
	if (dserr) {
		fprintf(stderr, "NWDSSetContext(DCK_CONFIDENCE) failed: %s\n",
			strnwerror(dserr));
		return 122;
	}
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
        if (server) {
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
        }else {

                if (!treeName[0]) {
                         NWCXGetPreferredDSTree(treeName,sizeof(treeName));

                }
                if (!treeName[0]) {
                        fprintf(stderr,"failed:You must specify a server or a tree\n");
                       exit(106);
                }


                  if ((dserr=NWCXAttachToTreeByName (&conn,treeName))) {
                         fprintf(stderr,"failed:Cannot attach to tree %s.err:%s\n",
                                      treeName,strnwerror(dserr));

                        exit(108);
                }
                if((dserr=NWCCGetConnInfo(conn,NWCC_INFO_SERVER_NAME,sizeof(serverName),serverName))){
                         fprintf(stderr,"failed:Cannot get server name from connection to tree %s.err:%s\n",
                                      treeName,strnwerror(dserr));
                        NWCCCloseConn(conn);
                        exit(109);
                }

                server=serverName;

        }
	dserr = NWDSAddConnection(ctx, conn);
#endif
	if (dserr) {
		fprintf(stderr, "failed:Cannot bind connection to context: %s\n",
			strnwerror(dserr));
	}
	dserr = NWDSAllocBuf(size, &buf);
	if (dserr) {
		fprintf(stderr, "failed:Cannot create reply buffer: %s\n",
			strnwerror(dserr));
	}
	if (ts_defined)
		req = 4;
	else if ((classname || subjectname) && (req == 0 || req == 1))
		req = 2;
	do {

                switch (req) {
			case 0:dserr = NWDSList(ctx, objectname, &ih, buf); break;
			case 1:dserr = NWDSListContainers(ctx, objectname, &ih, buf); break;
			case 2:dserr = NWDSListByClassAndName(ctx, objectname, classname, subjectname, &ih, buf); break;
			case 3:dserr = NWDSGetCountByClassAndName(ctx, objectname, classname, subjectname, &cnt); break;
			case 4:dserr = NWDSExtSyncList(ctx, objectname,
					classname, subjectname, &ih, &ts,
					0, buf);
				break;
			default:dserr = 12345;
		}
		if (dserr) {
			fprintf(stderr, "List failed with %s\n",
				strnwerror(dserr));
		} else if (req != 3) {
			unsigned char* pp;
			size_t cnt;

			if (!quiet)
                            printf("Iteration handle: %08X\n", ih);

			dserr = NWDSGetObjectCount(ctx, buf, &cnt);
			if (dserr) {
				fprintf(stderr, "GetObjectCount failed with %s\n",
					strnwerror(dserr));
			} else {
				if (!quiet)
                                   printf("%u objects\n", cnt);
				while (cnt--) {
					Object_Info_T tt;
					char name[1024];
                                        char name_abbr[1024];
					size_t attrs;

					dserr = NWDSGetObjectName(ctx, buf, name, &attrs, &tt);
					if (dserr) {
						fprintf(stderr, "GetObjectName failed with %s\n",
							strnwerror(dserr));
						break;
					}
                                        if (abbreviate){
                                                dserr= NWDSAbbreviateName(ctx,name,name_abbr);
                                                if (dserr) {
                                                        fprintf(stderr, "NWDSAbbreviateName failed: %s\n",
                                                                 strnwerror(dserr));
                                                        break;
                                                }
                                                strcpy(name,name_abbr);
                                        }
                                        if (!quiet){
					        printf("Name: %s\n", name);
					        printf("  Attrs: %u\n", attrs);
					        printf("  Flags: %08X\n", tt.objectFlags);
					        printf("  Subordinate Count: %u\n", tt.subordinateCount);
					        printf("  Modification Time: %lu\n", tt.modificationTime);
					        printf("  Base Class: %s\n", tt.baseClass);
                                        }
                                        else    printf("%s\n", name);
				}
			}

#ifndef N_PLAT_MSW4
                        if (!quiet){
			        pp = buf->curPos;
			        for (; pp < buf->dataend; pp++) {
				        printf("%02X ", *pp);
			        }
			        printf("\n");
                        }
#endif
		} else {
			if (!quiet)
                                printf("%u objects found\n", cnt);
		}
	} while ((dserr == 0) && (ih != NO_MORE_ITERATIONS));
	NWCCCloseConn(conn);
	dserr = NWDSFreeContext(ctx);
	if (dserr) {
		fprintf(stderr, "NWDSFreeContext failed with %s\n", strnwerror(dserr));
		return 121;
	}
finished:
	return 0;
}

