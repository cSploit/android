/*
    pqrm.c - Remove job from a print queue on a server
    Copyright (C) 1998, 1999  Petr Vandrovec, David Woodhouse
  
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
					Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision. Derived from pqlist.c.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
		
	1.01  2001, February 18		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added support for NDS queues.

 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <ncp/nwcalls.h>

#include "dsqueue.h"

#include "private/libintl.h"
#define _(X) gettext(X)

static const char* progname;
static int isPQRM = 0;

static void usage(void) {
	fprintf(stderr, _("\n"));
	fprintf(stderr, _("usage: %s [options] <queue> <jobID> [<jobID> ...]\n"), progname);
	fprintf(stderr, _("\n"));
	fprintf(stderr, _("-S server      Server name to be used\n"));
	fprintf(stderr, _("-U username    User name\n"));
	fprintf(stderr, _("-P password    Use this password\n"));
	fprintf(stderr, _("-n             Do not use any password\n"));
	fprintf(stderr, _("-d             Delete job from queue (default for pqrm)\n"));
	fprintf(stderr, _("-r             Resume job\n"));
	fprintf(stderr, _("\n"));
}

int
main(int argc, char **argv)
{
	struct ncp_conn *conn;
	struct ncp_bindery_object q;
	long err;
	enum { C_UNK, C_DEL, C_RESUME } action = C_UNK;
	const char* pgm;
	int opt;

	progname = argv[0];
	pgm = strrchr(progname, '/');
	if (pgm) {
		progname = pgm + 1;
	}
	if (!strcmp(progname, "pqrm")) {
		isPQRM = 1;
	}

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	if ((conn = ncp_initialize(&argc, argv, 1, &err)) == NULL)
	{
		com_err(progname, err, _("when initializing"));
		return 1;
	}

	while ((opt = getopt(argc, argv, "rd")) != -1) {
		switch (opt) {
			case 'd':
				action = C_DEL;
				break;
			case 'r':
				action = C_RESUME;
				break;
			default:
				ncp_close(conn);
				usage();
				return 1;
		}
	}
	if (action == C_UNK) {
		if (isPQRM) {
			action = C_DEL;
		} else {
			ncp_close(conn);
			fprintf(stderr, _("%s: At least one of -d or -r must be specified\n"), progname);
			return 2;
		}
	}
	if (optind + 2 > argc) {
		ncp_close(conn);
		usage();
		return 1;
	}

	if (ncp_get_bindery_object_id(conn, NCP_BINDERY_PQUEUE, 
                                      argv[optind], &q) != 0)
	{
		NWObjectID id;
		
		if (try_nds_queue(&conn, argv[optind], &id)) {
			char server[NW_MAX_SERVER_NAME_LEN];
			const char* sptr = server;
			NWCCODE err2;

			err2 = NWCCGetConnInfo(conn, NWCC_INFO_SERVER_NAME,
				sizeof(server), server);
			if (err2)
				sptr = "?";

	                printf(_("Queue \"%s\" on server %s not found.\n"),
                	       argv[optind], sptr);
        	        ncp_close(conn);
	                exit(1);
		}
		q.object_id = id;
	}
	optind++;

	for (; optind < argc; optind++) {
		u_int32_t jobID;
		char* end;

		jobID = strtoul(argv[optind], &end, 16);
		if (*end) {
			fprintf(stderr, _("Cannot parse \"%s\" - jobID must be hexadecimal number\n"), argv[optind]);
		} else {
			NWCCODE err2;

			if (action == C_RESUME) {
				struct nw_queue_job_entry j;

				err2 = ncp_get_queue_job_info(conn, q.object_id, jobID, &j);
				if (!err2) {
					j.JobControlFlags &= ~(QJE_OPER_HOLD | QJE_USER_HOLD);
					err2 = NWChangeQueueJobEntry(conn, q.object_id, &j);
				}
			} else {
				err2 = NWRemoveJobFromQueue2(conn, q.object_id, jobID);
			}
			if (err2 == NWE_Q_NO_JOB) {
				fprintf(stderr, _("Job %08X does not exist\n"), jobID);
			} else if (err2 == NWE_Q_NO_JOB_RIGHTS) {
				fprintf(stderr, _("You have not rights to cancel job %08X\n"), jobID);
			} else if (err2) {
				fprintf(stderr, _("Cannot cancel job %08X: %s\n"), jobID, strnwerror(err2));
			}
		}
	}
	ncp_close(conn);
	return 0;
}
