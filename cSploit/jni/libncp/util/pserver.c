/*
    pserver.c
    Copyright (C) 1996 by Volker Lendecke
  
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

	0.00  1996			Volker Lendecke
		Initial revision.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <ncp/ncplib.h>

#include "private/libintl.h"
#define _(X) gettext(X)

struct nw_queue
{
	struct ncp_conn *conn;

	char queue_name[NCP_BINDERY_NAME_LEN];
	u_int32_t queue_id;
	u_int16_t job_type;

	char *command;
};

static volatile int term_request;
static char *progname;

static void
usage(void)
{
	fprintf(stderr, _("usage: %s [options] file\n"), progname);
}

static void
help(void)
{
	printf(_("\n"
		 "usage: %s [options]\n"), progname);
	printf(_("\n"
	       "-S server      Server name to be used\n"
	       "-U username    Print Server name sent to server\n"
	       "-P password    Use this password\n"
	       "-n             Do not use any password\n"
	       "-C             Don't convert password to uppercase\n"
	       "-q queue name  Name of the printing queue to use\n"
	       "-c command     Name of print command, default: 'lpr'\n"
	       "-j job type    Type of job (Form number) to service\n"
	       "-t timeout     Polling interval, default: 30 sec\n"
	       "-d             Debug: don't daemonize\n"
	       "-h             print this help text\n"
	       "\n"));
}

#ifndef NCP_BINDERY_PSERVER
#define NCP_BINDERY_PSERVER (0x0007)
#endif

static void
terminate_handler(int dummy __attribute__((unused)))
{
	signal(SIGTERM, terminate_handler);
	signal(SIGINT, terminate_handler);
	term_request = 1;
}

/* Daemon_init is taken from Stevens, Adv. Unix programming */
static int
daemon_init(void)
{
	pid_t pid;

	if ((pid = fork()) < 0)
	{
		return -1;
	} else if (pid != 0)
	{
		exit(0);	/* parent vanishes */
	}
	/* child process */
	setsid();
	chdir("/");
	umask(0);
	close(0);
	close(1);
	close(2);
	return 0;
}

static int
init_queue(struct ncp_conn *conn, char *queue_name, char *command,
	   struct nw_queue *q)
{
	struct ncp_bindery_object obj;
	long err;

	str_upper(queue_name);

	q->conn = conn;
	q->command = command;

	err = ncp_get_bindery_object_id(conn, NCP_BINDERY_PQUEUE, 
					queue_name, &obj);
	if (err) {
		fprintf(stderr, _("Queue %s not found: %s\n"), 
			queue_name, strnwerror(err));
		return -1;
	}
	q->queue_id = obj.object_id;
	memcpy(q->queue_name, obj.object_name, sizeof(q->queue_name));

	err = ncp_attach_to_queue(conn, q->queue_id);
	if (err) {
		fprintf(stderr, _("Could not attach to queue %s: %s\n"),
			queue_name, strnwerror(err));
		return -1;
	}
	return 0;
}


static void
build_command(struct nw_queue *q, struct queue_job *j,
	      char *target, int target_size, char *user)
{
	char *s = q->command;
	char *target_end = target + target_size;

	void add_string(const char *str)
	{
		int len = strlen(str);
		if (target + len + 1 > target_end)
		{
			len = target_end - target - 1;
		}
		strncpy(target, str, len);
		target += len;
	}

	memset(target, 0, target_size);

	while ((*s != 0) && (target < target_end))
	{
		if (*s != '%')
		{
			*target = *s;
			target += 1;
			s += 1;
			continue;
		}
		switch (*(s + 1))
		{
		case '%':
			*target = '%';
			target += 1;
			break;
		case 'u':
		        add_string(user);
			break;
                case 'd':
                        if (j->j.JobTextDescription[0])
                                add_string(j->j.JobTextDescription);
                        else
                                add_string(_("No Description"));
                        break;
		default:
			*target = '%';
			*(target + 1) = *(s + 1);
			target += 2;
		}
		s += 2;
	}
}




static int
poll_queue(struct nw_queue *q)
{
	struct queue_job job;
	int fd[2];
	int pid;
	int retcode;
	struct ncp_bindery_object u;
	char user[50];

	retcode = ncp_service_queue_job(q->conn, q->queue_id, q->job_type, &job);
	if (retcode != 0) {
		if (retcode == NWE_Q_NO_JOB || retcode == NWE_SERVER_FAILURE) {
			/* No job for us */
			return 0;
		}
		syslog(LOG_ERR, _("Cannot service print job: %s\n"), strnwerror(retcode));
		return 2;
	}

        if (ncp_get_queue_job_info(q->conn, q->queue_id, job.j.JobNumber, &job.j) != 0)
        {
                job.j.JobTextDescription[0]=0;
        }

	if ((retcode=ncp_get_bindery_object_name
	     (q->conn, htonl(job.j.ClientObjectID), &u))
	    == 0)
	  {
	    memcpy(user,u.object_name,48);
            user[48]=0;
	  } 
	else
	  {
	    sprintf(user,_("<Unknown>"));
	  }
	
	if (pipe(fd) < 0)
	{
		syslog(LOG_ERR, _("pipe error: %m"));
		goto fail;
	}
	if ((pid = fork()) < 0)
	{
		syslog(LOG_ERR, _("fork error: %m"));
		goto fail;
	}
	if (pid > 0)
	{
		/* parent */
		char buf[1024];
		size_t result;
		off_t offset = 0;

		close(fd[0]);	/* close read end */

		while ((result = ncp_read(q->conn, job.file_handle, offset,
					  sizeof(buf), buf)) > 0)
		{
			offset += result;
			if (write(fd[1], buf, result) != (int)result)
			{
				goto fail;
			}
		}

		close(fd[1]);	/* and close write end */

		if (waitpid(pid, NULL, 0) < 0)
		{
			syslog(LOG_ERR, _("waitpid: %m\n"));
		}
	} else
	{
		/* child */

		char command[2048];

		close(fd[1]);	/* close write end */

		if (fd[0] != STDIN_FILENO)
		{
			if (dup2(fd[0], STDIN_FILENO) != STDIN_FILENO)
			{
				syslog(LOG_ERR, _("dup2 error: %m\n"));
				close(fd[0]);
				exit(1);
			}
			close(fd[0]);
		}

		build_command(q, &job, command, sizeof(command), user);

		execl("/bin/sh", "sh", "-c", command, NULL);
		syslog(LOG_ERR, _("exec error: %m\n"));
		close(fd[0]);
		exit(1);
	}

	ncp_finish_servicing_job(q->conn, q->queue_id, job.j.JobNumber, 0);
	return 1;

      fail:
	ncp_abort_servicing_job(q->conn, q->queue_id, job.j.JobNumber);
	/* We tell that we did not have a job to avoid overloading
	   when something's wrong */
	return 0;
}

static struct nw_queue q;

int
main(int argc, char *argv[])
{
	struct ncp_conn *conn;
	int poll_timeout = 30;
	int opt;
	int job_type = 0xffff;
	int debug = 0;
	int i;
	long err;

	char *queue_name = NULL;

	char default_command[] = "lpr";
	char *command = default_command;

	setlocale(LC_ALL, "");
	bindtextdomain(NCPFS_PACKAGE, LOCALEDIR);
	textdomain(NCPFS_PACKAGE);
	
	progname = argv[0];

	for (i = 1; i < argc; i += 1)
	{
		if ((strcmp(argv[i], "-h") == 0)
		    || (strcmp(argv[i], "-?") == 0))
		{
			help();
			exit(0);
		}
	}

	for (i = 1; i < argc; i += 1)
	{
		if (strcmp(argv[i], "-d") == 0)
		{
			debug = 1;
			break;
		}
	}

	if (debug == 0)
	{
		daemon_init();
		openlog("pserver", LOG_PID, LOG_LPR);
	}
	if ((conn = ncp_initialize_as(&argc, argv, 1,
				      NCP_BINDERY_PSERVER, &err)) == NULL)
	{
		com_err(argv[0], err, _("when initializing"));
		return 1;
	}
	while ((opt = getopt(argc, argv, "q:c:j:t:dh")) != EOF)
	{
		switch (opt)
		{
		case 'q':
			queue_name = optarg;
			break;
		case 'c':
			command = optarg;
			break;
		case 'j':
			job_type = atoi(optarg);
			break;
		case 't':
			poll_timeout = atoi(optarg);
			break;
		case 'd':
			debug = 1;
			break;
		case 'h':
			break;
		default:
			usage();
			return -1;
		}
	}

	if (argc != optind)
	{
		usage();
		return -1;
	}
	memset(&q, 0, sizeof(q));

	if (queue_name == NULL)
	{
		fprintf(stderr, _("You must specify a queue\n"));
		return 1;
	}
	if (init_queue(conn, queue_name, command, &q) != 0)
	{
		ncp_close(conn);
		return 1;
	}
	q.job_type = job_type;

	term_request = 0;
	signal(SIGTERM, terminate_handler);
	signal(SIGINT, terminate_handler);

	while (1)
	{
		err = poll_queue(&q);
		if (term_request || err == 2)
			break;
		if (err == 0)
			sleep(poll_timeout);
	}

	ncp_detach_from_queue(conn, q.queue_id);

	ncp_close(conn);
	return 0;
}

