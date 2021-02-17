/* TDSPool - Connection pooling for TDS based databases
 * Copyright (C) 2001 Brian Bruns
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Note on terminology: a pool member is a connection to the database,
 * a pool user is a client connection that is temporarily assigned to a
 * pool member.
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>
#include <signal.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */

#include "pool.h"

TDS_RCSID(var, "$Id: main.c,v 1.27 2011-06-03 21:13:27 freddy77 Exp $");

/* to be set by sig term */
static int term = 0;

/* number of users in wait state */
int waiters = 0;

static void term_handler(int sig);
static void pool_schedule_waiters(TDS_POOL * pool);
static TDS_POOL *pool_init(char *name);
static void pool_main_loop(TDS_POOL * pool);

static void
term_handler(int sig)
{
	fprintf(stdout, "Shutdown Requested\n");
	term = 1;
}

/*
 * pool_init creates a named pool and opens connections to the database
 */
static TDS_POOL *
pool_init(char *name)
{
	TDS_POOL *pool;

	/* initialize the pool */

	pool = (TDS_POOL *) calloc(1, sizeof(TDS_POOL));
	/* FIXME -- read this from the conf file */
	if (!pool_read_conf_file(name, pool)) {
		fprintf(stderr, "Configuration for pool ``%s'' not found.\n", name);
		exit(EXIT_FAILURE);
	}
	pool->num_members = pool->max_open_conn;

	pool->name = strdup(name);

	pool_mbr_init(pool);
	pool_user_init(pool);

	return pool;
}

static void
pool_schedule_waiters(TDS_POOL * pool)
{
	TDS_POOL_USER *puser;
	TDS_POOL_MEMBER *pmbr;
	int i, free_mbrs;

	/* first see if there are free members to do the request */
	free_mbrs = 0;
	for (i = 0; i < pool->num_members; i++) {
		pmbr = (TDS_POOL_MEMBER *) & pool->members[i];
		if (pmbr->tds && pmbr->state == TDS_IDLE)
			free_mbrs++;
	}

	if (!free_mbrs)
		return;

	for (i = 0; i < pool->max_users; i++) {
		puser = (TDS_POOL_USER *) & pool->users[i];
		if (puser->user_state == TDS_SRV_WAIT) {
			/* place back in query state */
			puser->user_state = TDS_SRV_QUERY;
			waiters--;
			/* now try again */
			pool_user_query(pool, puser);
			return;
		}
	}
}

/* 
 * pool_main_loop
 * Accept new connections from clients, and handle all input from clients and
 * pool members.
 */
static void
pool_main_loop(TDS_POOL * pool)
{
	TDS_POOL_USER *puser;
	TDS_POOL_MEMBER *pmbr;
	struct sockaddr_in sin;
	int s, maxfd, i;
	fd_set rfds;
	int socktrue = 1;

	/* FIXME -- read the interfaces file and bind accordingly */
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(pool->port);
	sin.sin_family = AF_INET;

	if (TDS_IS_SOCKET_INVALID(s = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("socket");
		exit(1);
	}
	/* don't keep addr in use from s.craig@andronics.com */
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *) &socktrue, sizeof(socktrue));

	fprintf(stderr, "Listening on port %d\n", pool->port);
	if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		perror("bind");
		exit(1);
	}
	listen(s, 5);

	FD_ZERO(&rfds);
	FD_SET(s, &rfds);
	maxfd = s;

	while (!term) {
		/* fprintf(stderr, "waiting for a connect\n"); */
		/* FIXME check return value */
		select(maxfd + 1, &rfds, NULL, NULL, NULL);
		if (term)
			break;

		/* process the sockets */
		if (FD_ISSET(s, &rfds)) {
			pool_user_create(pool, s, &sin);
		}
		pool_process_users(pool, &rfds);
		pool_process_members(pool, &rfds);
		/* back from members */
		if (waiters) {
			pool_schedule_waiters(pool);
		}

		FD_ZERO(&rfds);
		/* add the listening socket to the read list */
		FD_SET(s, &rfds);
		maxfd = s;

		/* add the user sockets to the read list */
		for (i = 0; i < pool->max_users; i++) {
			puser = (TDS_POOL_USER *) & pool->users[i];
			/* skip dead connections */
			if (!IS_TDSDEAD(puser->tds)) {
				if (tds_get_s(puser->tds) > maxfd)
					maxfd = tds_get_s(puser->tds);
				FD_SET(tds_get_s(puser->tds), &rfds);
			}
		}

		/* add the pool member sockets to the read list */
		for (i = 0; i < pool->num_members; i++) {
			pmbr = (TDS_POOL_MEMBER *) & pool->members[i];
			if (!IS_TDSDEAD(pmbr->tds)) {
				if (tds_get_s(pmbr->tds) > maxfd)
					maxfd = tds_get_s(pmbr->tds);
				FD_SET(tds_get_s(pmbr->tds), &rfds);
			}
		}
	}			/* while !term */
	CLOSESOCKET(s);
	for (i = 0; i < pool->max_users; i++) {
		puser = (TDS_POOL_USER *) & pool->users[i];
		if (!IS_TDSDEAD(puser->tds)) {
			fprintf(stderr, "Closing user %d\n", i);
			tds_close_socket(puser->tds);
		}
	}
	for (i = 0; i < pool->num_members; i++) {
		pmbr = (TDS_POOL_MEMBER *) & pool->members[i];
		if (!IS_TDSDEAD(pmbr->tds)) {
			fprintf(stderr, "Closing member %d\n", i);
			tds_close_socket(pmbr->tds);
		}
	}
}

int
main(int argc, char **argv)
{
	TDS_POOL *pool;

	signal(SIGTERM, term_handler);
	signal(SIGINT, term_handler);
	if (argc < 2) {
		fprintf(stderr, "Usage: tdspool <pool name>\n");
		return 1;
	}
	pool = pool_init(argv[1]);
	pool_main_loop(pool);
	fprintf(stdout, "tdspool Shutdown\n");
	return EXIT_SUCCESS;
}
