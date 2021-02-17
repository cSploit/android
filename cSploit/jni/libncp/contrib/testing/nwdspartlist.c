/*
    nwdspartlist.c - NWDSListPartitions() API demo
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

	1.00  2000, April 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.01  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Use NWObjectCount instead of size_t.

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
#include <string.h>

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
	       "-q req_type     Info type (default 0)\n"
	       "-s size         Reply buffer size (default 4096)\n"
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
	const char* objectname = "[Root]";
	const char* context = "OrgUnit.Org.Country";
	const char* server = "CDROM";
	int opt;
	u_int32_t ctxflag = 0;
	u_int32_t confidence = 0;
	u_int32_t req = 0;
	int req_present = 0;
	nuint32 ih = NO_MORE_ITERATIONS;
	Buf_T* buf;
	size_t size = DEFAULT_MESSAGE_LEN;

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
		fprintf(stderr, "NWDSCretateContextHandle failed with %s\n", strnwerror(dserr));
		return 123;
	}
	while ((opt = getopt(argc, argv, "h?o:c:v:S:C:q:s:")) != EOF)
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
			req_present = 1;
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
	dserr = NWDSAllocBuf(size, &buf);
	if (dserr) {
		fprintf(stderr, "Cannot create reply buffer: %s\n",
			strnwerror(dserr));
	}
	do {
		if (req_present)
			dserr = NWDSListPartitionsExtInfo(ctx, &ih, objectname, req, buf);
		else
			dserr = NWDSListPartitions(ctx, &ih, objectname, buf);
		if (dserr) {
			fprintf(stderr, "ListPartitions failed with %s\n", 
				strnwerror(dserr));
		} else {
			unsigned char* pp;
			NWObjectCount cnt;
			char sn[MAX_DN_CHARS + 1];
			size_t pcnt;

			printf("Iteration handle: %08X\n", ih);
		
			dserr = NWDSGetServerName(ctx, buf, sn, &cnt);
			if (dserr) {
				fprintf(stderr, "GetServerName failed with %s\n",
					strnwerror(dserr));
			} else {
				pcnt = 0;
				printf("Server %s holds %u partitions\n",
					sn, cnt);
				while (!dserr && cnt--) {
					printf("Partition #%u\n", pcnt++);
					if (req_present) {
						char* start;
						char* end;
						
						dserr = NWDSGetPartitionExtInfoPtr(
								ctx, buf, &start, &end);
						if (dserr) {
							fprintf(stderr, "  NWDSGetPartitionExtInfoPtr failed with %s\n",
								strnwerror(dserr));
						} else {
							nuint32 pt;
							union {
							nuint32     x32;
							TimeStamp_T ts;
							NWObjectID  id;
							char        name[MAX_DN_CHARS + 1];
							} info;
							nuint32 bit;
							size_t len;
							
							printf("  Partition info length:  %10u\n", end - start);
							if (req & DSP_OUTPUT_FIELDS) {
								dserr = NWDSGetPartitionExtInfo(ctx, start, end, DSP_OUTPUT_FIELDS, &len, &pt);
								if (dserr) {
									fprintf(stderr, "  NWDSGetPartitionExtInfo(DSP_OUTPUT_FIELDS) failed with %s\n",
										strnwerror(dserr));
									pt = req;
								} else {
									printf("  Output fields:            %08X (%u bytes)\n", pt, len);
								}
							} else {
								pt = req;
							}
							for (bit = DSP_PARTITION_ID; bit; bit <<= 1) {
								if (pt & bit) {
									dserr = NWDSGetPartitionExtInfo(ctx, start, end, bit, &len, &info);
									if (dserr)
										fprintf(stderr, "  NWDSGetPartitionExtInfo(0x%08X) failed with %s\n",
											bit, strnwerror(dserr));
									else {
										switch (bit) {
											case DSP_PARTITION_ID:
												printf("  Partition ID:             %08X (%u bytes)\n", info.id, len);
												break;
											case DSP_REPLICA_STATE:
												printf("  Replica state:          %10u (%u bytes)\n", info.x32, len);
												break;
											case DSP_MODIFICATION_TIMESTAMP:
												printf("  Modification timestamp: %10u.%u.%u (%u bytes)\n",
													info.ts.wholeSeconds, info.ts.replicaNum,
													info.ts.eventID, len);
												break;
											case DSP_PURGE_TIME:
												printf("  Purge time:             %10u (%u bytes)\n", 
													info.x32, len);
												break;
											case DSP_LOCAL_PARTITION_ID:
												printf("  Local partition ID:     %10u (%u bytes)\n",
													info.x32, len);
												break;
											case DSP_PARTITION_DN:
												printf("  Partition DN:           %s (%u bytes)\n",
													info.name, len);
												break;
											case DSP_REPLICA_TYPE:
												printf("  Replica type:           %10u (%u bytes)\n",
													info.x32, len);
												break;
											case DSP_PARTITION_BUSY:
												printf("  Partition busy:         %10u (%u bytes)\n",
													info.x32, len);
												break;
											default:
												printf("  Unknown %08X:       %10u (%u bytes)\n",
													bit, info.x32, len);
												break;
										}
									}
								}
							}
						}
					} else {
						nuint32 pt;
						
						dserr = NWDSGetPartitionInfo(ctx, buf, sn, &pt);
						if (dserr) {
							fprintf(stderr, "  NWDSGetPartitionInfo failed with %s\n",
								strnwerror(dserr));
						} else {
							printf("  Partition DN: %s\n", sn);
							printf("  Replica type: %u\n", pt);
						}
					}
#if 0
					Object_Info_T tt;
					char name[8192];
					size_t attrs;
					
					dserr = NWDSGetObjectName(ctx, buf, name, &attrs, &tt);
					if (dserr) {
						fprintf(stderr, "GetObjectName failed with %s\n",
							strnwerror(dserr));
						break;
					}
					printf("Name: %s\n", name);
					printf("  Attrs: %u\n", attrs);
					printf("  Flags: %08X\n", tt.objectFlags);
					printf("  Subordinate Count: %u\n", tt.subordinateCount);
					printf("  Modification Time: %lu\n", tt.modificationTime);
					printf("  Base Class: %s\n", tt.baseClass);
#endif
				}
			}
#ifndef N_PLAT_MSW4
			pp = buf->curPos;	
			for (; pp < buf->dataend; pp++) {
				printf("%02X ", *pp);
			}
			printf("\n");
#endif
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
