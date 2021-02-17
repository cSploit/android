/*
    nprint.c - Send data to a NetWare print queue.
    Copyright (C) 1995 by Volker Lendecke

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
    
	0.00  1995			Volker Lendecke
		Initial release.

	1.00  2000, January 15		Petr Vandrovec <vandrove@vc.cvut.cz>
		NDS queues support.

	1.01  2000, September 11	Bruno Browning <bruno@lss.wirc.edu>
		Added '-s' option, changed '-b' to '-B' to get it to work.

	1.02  2000, Feburary 18		Thomas Hood <jdthood@ubishops.ca>
		Use STRCPY_SAFE when possible, clarify help and manpage.
		
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include <ncp/nwnet.h>
#include "dsqueue.h"

#include "private/libintl.h"
#define _(X) gettext(X)
#define STRCPY_SAFE(d,s) { strncpy((d),(s),sizeof(d)-1) ; (d)[sizeof(d)-1] = '\0' ; }

static char *progname;
static void usage(void);
static void help(void);

int
main(int argc, char *argv[])
{
	static char default_queue[] = "*";
	char *queue = default_queue;

	int opt;
	
	int requestBanner, supressBanner;

	struct ncp_bindery_object q;
	struct queue_job j;
	struct print_job_record pj;

	int written, read_this_time;

	char buf[8192];

	const char *file_name;
	int file;
	long err;

	struct ncp_conn *conn;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	if ((argc == 2)
	    && (strcmp(argv[1], "-h") == 0))
	{
		help();
		return 127;
	}

	conn = ncp_initialize(&argc, argv, 1, &err);
	if (!conn)
	{
		com_err(argv[0], err, _("when initializing connection"));
		return 1;
	}
	
	memset(&j, 0, sizeof(j));
	memset(&pj, 0, sizeof(pj));
	memset(&q, 0, sizeof(q));

	/*
	 * Fill in default values for print job
	 */
	j.j.TargetServerID = 0xffffffff;	/* any server */
	/* at once */
	memset(&(j.j.TargetExecTime), 0xff, sizeof(j.j.TargetExecTime));
	j.j.JobType = htons(0);
	STRCPY_SAFE(j.j.JobTextDescription, _("No Description"));

	pj.Version = 0;
	pj.TabSize = 8;
	pj.Copies = htons(1);
	pj.CtrlFlags = 0;
	pj.Lines = htons(66);
	pj.Rows = htons(80);
	strcpy(pj.FnameHeader, "stdin");
	
	requestBanner = 0;
	supressBanner = 0;

	while ((opt = getopt(argc, argv, "h?q:d:p:B:sf:l:r:c:t:F:TN")) != EOF)
	{
		switch (opt)
		{
		case 'h':
		case '?':
			help();
			ncp_close(conn);
			exit(1);
		case 'p':
			/* Path */
			requestBanner = 1;
			STRCPY_SAFE(pj.Path, optarg);
			break;
		case 'B':
			/* Banner Name */
			requestBanner = 1;
			STRCPY_SAFE(pj.BannerName, optarg);
			break;
		case 's':
			/* Supress Banner Page */
			supressBanner = 1;
			break;
		case 'f':
			/* File Name in Banner */
			requestBanner = 1;
			STRCPY_SAFE(pj.FnameBanner, optarg);
			break;
		case 'l':
			/* lines, default: 66 */
			if ((atoi(optarg) < 0) || (atoi(optarg) > 65535))
			{
				fprintf(stderr,
					_("invalid line number: %s\n"), optarg);
				break;
			}
			pj.Lines = htons(atoi(optarg));
			pj.CtrlFlags |= EXPAND_TABS;
			break;
		case 'r':
			/* rows, default: 80 */
			if ((atoi(optarg) < 0) || (atoi(optarg) > 65535))
			{
				fprintf(stderr,
					_("invalid row number: %s\n"), optarg);
				break;
			}
			pj.Rows = htons(atoi(optarg));
			pj.CtrlFlags |= EXPAND_TABS;
			break;
		case 'c':
			/* copies, default: 1 */
			if ((atoi(optarg) < 0) || (atoi(optarg) > 65000))
			{
				fprintf(stderr,
					_("invalid copies: %s\n"), optarg);
				break;
			}
			pj.Copies = htons(atoi(optarg));
			pj.CtrlFlags |= EXPAND_TABS;
			break;
		case 't':
			/* tab size, default: 8 */
			if ((atoi(optarg) < 0) || (atoi(optarg) > 255))
			{
				fprintf(stderr,
					_("invalid tab size: %s\n"), optarg);
				break;
			}
			pj.TabSize = atoi(optarg);
			pj.CtrlFlags |= EXPAND_TABS;
			break;
		case 'T':
			/* expand tabs, default tabsize: 8 */
			pj.CtrlFlags |= EXPAND_TABS;
			break;
		case 'N':
			/* no form feed */
			pj.CtrlFlags |= NO_FORM_FEED;
			break;
		case 'F':
			/* Form number, default: 0 */
			if ((atoi(optarg) < 0) || (atoi(optarg) > 255))
			{
				fprintf(stderr,
					_("invalid form number: %s\n"), optarg);
				break;
			}
			j.j.JobType = htons(atoi(optarg));
			break;
		case 'q':
			/* Queue name to print on, default: '*' */
			queue = optarg;
			break;
		case 'd':
			/* Job Description */
			requestBanner = 1;
			STRCPY_SAFE(j.j.JobTextDescription, optarg);
			break;

		default:
			usage();
			ncp_close(conn);
			exit(1);
		}
	}
	
	if (requestBanner && !supressBanner)
	{
		pj.CtrlFlags |= PRINT_BANNER;
	}

	if (optind < argc - 1) {
		usage();
		ncp_close(conn);
		exit(1);
	} else if (optind == argc - 1) {
		file_name = argv[optind];
	} else {
		file_name = "-";
	}

	if (strcmp(file_name, "-") == 0) {
		file = 0;	/* stdin */
	} else {
		file = open(file_name, O_RDONLY, 0);
		if (file < 0)
		{
			perror(_("could not open file"));
			ncp_close(conn);
			exit(1);
		}
		STRCPY_SAFE(pj.FnameHeader, file_name);
		if (strlen(pj.FnameBanner) == 0)
			STRCPY_SAFE(pj.FnameBanner, file_name);
	}

	memcpy(j.j.ClientRecordArea, &pj, sizeof(pj));

	str_upper(queue);

	if (ncp_scan_bindery_object(conn, 0xffffffff, NCP_BINDERY_PQUEUE,
				    queue, &q) != 0)
	{
		NWObjectID id;
		
		if (try_nds_queue(&conn, queue, &id)) {
			printf(_("Could not find queue %s\n"), queue);
			ncp_close(conn);
			exit(1);
		}
		q.object_id = id;
	}
	err = ncp_create_queue_job_and_file(conn, q.object_id, &j);
	if (err)
	{
		printf(_("Cannot create print job: %s\n"), strnwerror(err));
		ncp_close(conn);
		exit(1);
	}
	written = 0;
	do
	{
		do {
			read_this_time = read(file, buf, sizeof(buf));
		} while (read_this_time == -1 && errno == EINTR);
		if (read_this_time < 0)
		{
			break;
		}
		if (ncp_write(conn, j.file_handle,
			      written, read_this_time, buf) < read_this_time)
		{
			break;
		}
		written += read_this_time;
	}
	while (read_this_time > 0);

	close(file);

	err = ncp_close_file_and_start_job(conn, q.object_id, &j);
	if (err)
	{
		printf(_("Cannot start print job: %s\n"), strnwerror(err));
		ncp_close(conn);
		exit(1);
	}
	ncp_close(conn);
	return 0;
}

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options] file\n"), progname);
}

static void
help(void)
{
	printf("\n");
	printf(_("usage: %s [options] file\n"), progname);
	printf(_("\n"
	       "-S server      Server name to be used\n"
	       "-U username    Username sent to server\n"
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "-C             Don't convert password to uppercase\n"
	       "-q queue name  Name of the printing queue to use\n"
	       "-d job desc    Job description\n"
	       "-p pathname    Pathname to appear on banner (up to 79 chars)\n"
	       "-B username    Username to appear on banner (up to 12 chars)\n"
	       "-f filename    Filename to appear on banner (up to 12 chars)\n"
	       "-s             Supress banner page\n"
	       "-l lines       Number of lines per page\n"
	       "-r rows        Number of rows per page\n"
	       "-t tab         Number of spaces per tab\n"
	       "-T             Print server tab expantion\n"
	       "-N             Surpress print server form feeds\n"
	       "-F form #      Form number to print on\n"
	       "-h             print this help text\n"
	       "\n"));
}
