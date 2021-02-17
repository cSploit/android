/*
    pqstat.c - List the jobs in a print queue on a server
    Copyright (C) 1998 by David Woodhouse

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

	0.00  1998			David Woodhouse <dave@imladris.demon.co.uk>
		Initial revision. Derived from pqlist.c.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

	1.01  2001, February 18		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added support for NDS queues.
		
	1.02  2003  March 24             Bruno Browning <bruno@lss.wisc.edu>
	     Added -B switch to show bannername instead of username
  
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ncp/nwcalls.h>

#include "dsqueue.h"

#include "private/libintl.h"
#define _(X) gettext(X)
#define N_(X) (X)

/* move this to library ? */
static int
ncp_time_to_tm(struct tm* out, u_int8_t* netwareTime)
{
	struct tm tmp;

	tmp.tm_year = netwareTime[0];
	if (tmp.tm_year < 80) tmp.tm_year += 100;
	tmp.tm_mon = netwareTime[1]-1;
	if ((tmp.tm_mon < 0) || (tmp.tm_mon >= 12)) return 1;
	tmp.tm_mday = netwareTime[2];
	if ((tmp.tm_mday < 1) || (tmp.tm_mday >= 32)) return 1;
	tmp.tm_hour = netwareTime[3];
	if (tmp.tm_hour >= 24) return 1;
	tmp.tm_min = netwareTime[4];
	if (tmp.tm_min >= 60) return 1;
	tmp.tm_sec = netwareTime[5];
	if (tmp.tm_sec >= 60) return 1;
	memcpy(out, &tmp, sizeof(tmp));
	return 0;
}

static int
ncp_cmp_time(struct tm* tm1, struct tm* tm2)
{
#undef XTST
#define XTST(Y) if (tm1->tm_##Y != tm2->tm_##Y) { \
			return (tm1->tm_##Y > tm2->tm_##Y) ? 1 : -1; \
		}
	XTST(year);
	XTST(mon);
	XTST(mday);
	XTST(hour);
	XTST(min);
	XTST(sec);
#undef XTST
	return 0;
}

static void get_object_name(NWCONN_HANDLE conn, NWObjectID objID, char* buff) {
	NWCCODE err;

	err = NWGetObjectName(conn, objID, buff, NULL);
	if (err) {
		NWDSContextHandle ctx;

		err = NWDSCreateContextHandle(&ctx);
		if (!err) {
			err = NWDSAddConnection(ctx, conn);
			if (!err) {
				err = NWDSMapIDToName(ctx, conn, objID, buff);
			}
			NWDSFreeContext(ctx);
		}
	}
	if (err) {
		sprintf(buff, "<%s>", strnwerror(err));
	}
}

static const char *progname;

static void usage(void) {
	fprintf(stderr, _("usage: %s [-B] <queue> [<qlen>]\n"), progname);
}

int
main(int argc, char **argv)
{
	struct ncp_conn *conn;
	struct ncp_bindery_object q;
	unsigned long maxqlen=~0;
	const char *queueName;
	long err;
	int useBanner = 0;
	int opt;

	u_int32_t qlen, idl1,idl2, job_id;
	struct nw_queue_job_entry j;

	progname = argv[0];

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	if ((conn = ncp_initialize(&argc, argv, 1, &err)) == NULL)
	{
		com_err(progname, err, _("when initializing"));
		return 1;
	}

	while ((opt = getopt(argc, argv, "B")) != -1) {
		switch (opt) {
			case 'B':
				useBanner = 1;
				break;
			default:
				usage();
				return 1;
		}
	}
	if (optind >= argc || optind + 2 < argc)
	{
		usage();
		return 1;
	}
	queueName = argv[optind++];
	if (optind < argc) {
                char *end;

                maxqlen=strtoul(argv[optind++], &end, 10);
                if (*end != 0) {
			usage();
			return 1;
		}
        }

	if (ncp_get_bindery_object_id(conn, NCP_BINDERY_PQUEUE, 
                                      queueName, &q) != 0)
	{
		NWObjectID id;
		
		if (try_nds_queue(&conn, queueName, &id)) {
			char server[NW_MAX_SERVER_NAME_LEN];
			const char* sptr = server;
			NWCCODE err2;

			err2 = NWCCGetConnInfo(conn, NWCC_INFO_SERVER_NAME, 
				sizeof(server), server);
			if (err2)
				sptr = "?";
	               	printf(_("Queue \"%s\" on server %s not found.\n"),
				queueName, server);
                	ncp_close(conn);
	                exit(1);
		}
		q.object_id = id;
	}

	if (isatty(1))
	{
		char server[NW_MAX_SERVER_NAME_LEN];
		char qname[MAX_DN_BYTES];
		const char* sptr = server;
		NWCCODE err2;

		err2 = NWCCGetConnInfo(conn, NWCC_INFO_SERVER_NAME,
			sizeof(server), server);
		if (err2)
			sptr = "?";

		get_object_name(conn, q.object_id, qname);
		printf(_("\nServer: %s\tQueue: %s\tQueue ID: %8.8X\n"), sptr, 
			qname, q.object_id);
                printf(_(" %5s  %-12s  %-32s  %-7s  %-4s  %-8s\n"
		       "-----------------------------------------------"
		       "--------------------------------\n"),
		       _("Seq"),_("Name"),
		       _("Description"), _("Status"), _("Form"), _("Job ID"));
	}


        if ((err=ncp_get_queue_length(conn, q.object_id, &qlen)) != 0)
        {
		if (err == NWE_Q_NO_RIGHTS) {
			fprintf(stderr, _("You have insufficient rights to list queue jobs\n"));
		} else {
			com_err(progname, err, _(": cannot get queue length"));
		}
                ncp_close(conn);
                exit(1);
        }
/*        printf("There are %d jobs in the queue.\n",qlen); */
               
        idl1=1;
        job_id =0;
        
        if ((err=ncp_get_queue_job_ids(conn, q.object_id, 1, 
                                       &idl1, &idl2, &job_id)) != 0)
        {
                printf(_("Error getting queue jobs ids: %ld\n"),err);
                ncp_close(conn);
                exit(1);
        }
        
/*        printf("First queue job ID is %8X\n",job_id);*/
     
        while (maxqlen-- && job_id && (ncp_get_queue_job_info(conn, q.object_id, job_id, &j) == 0))
	{
		const char* jst;
		char user[MAX_DN_BYTES];

		if (useBanner) {
			memcpy(user, &j.ClientRecordArea[32], 13);
		} else {
			get_object_name(conn, ntohl(j.ClientObjectID), user);
		}

                j.JobFileName[j.FileNameLen]=0;

		if (j.JobControlFlags & 0xC0) {
			jst = N_("Held");
		} else if (j.JobControlFlags & 0x20) {
			jst = N_("Adding");
		} else if (j.ServerStation) {
			jst = N_("Active");
		} else {
			struct tm jobtime;

			jst = N_("Ready");
			if (!ncp_time_to_tm(&jobtime, j.TargetExecTime)) {
				time_t ltime;
				struct tm* loctime;

				time(&ltime);
				loctime = localtime(&ltime);
				
				if (ncp_cmp_time(&jobtime, loctime) >= 0)
					jst = N_("Waiting");
			}
		}
               
                printf(_(" %5d  %-12s  %-32.32s  %-7s  %4d  %08X\n"),
                       j.JobPosition, user, j.JobTextDescription, _(jst), ntohs(j.JobType), j.JobNumber);
                if (j.next == job_id) 
                        job_id = 0;
                else job_id = j.next;
        }

	ncp_close(conn);
	return 0;
}
