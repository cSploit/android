/* TDSPool - Connection pooling for TDS based databases
 * Copyright (C) 2001, 2002, 2003, 2004, 2005  Brian Bruns
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

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

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
#include "replacements.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif /* MAXHOSTNAMELEN */

TDS_RCSID(var, "$Id: member.c,v 1.53 2012-03-11 15:52:22 freddy77 Exp $");

static int pool_packet_read(TDS_POOL_MEMBER * pmbr);
static TDSSOCKET *pool_mbr_login(TDS_POOL * pool);

/*
 * pool_mbr_login open a single pool login, to be call at init time or
 * to reconnect.
 */
static TDSSOCKET *
pool_mbr_login(TDS_POOL * pool)
{
	TDSCONTEXT *context;
	TDSLOGIN *login;
	TDSSOCKET *tds;
	TDSLOGIN *connection;
	TDSRET rc;
	char *query;
	char hostname[MAXHOSTNAMELEN];

	login = tds_alloc_login(1);
	tds_set_passwd(login, pool->password);
	tds_set_user(login, pool->user);
	tds_set_app(login, "tdspool");
#if HAVE_GETHOSTNAME
	if (gethostname(hostname, MAXHOSTNAMELEN) < 0)
#endif
		tds_strlcpy(hostname, "tdspool", MAXHOSTNAMELEN);
	tds_set_host(login, hostname);
	tds_set_library(login, "TDS-Library");
	tds_set_server(login, pool->server);
	tds_set_client_charset(login, "iso_1");
	tds_set_language(login, "us_english");
	tds_set_packet(login, 512);
	context = tds_alloc_context(NULL);
	tds = tds_alloc_socket(context, 512);
	connection = tds_read_config_info(tds, login, context->locale);
	if (!connection || TDS_FAILED(tds_connect_and_login(tds, connection))) {
		tds_free_socket(tds);
		tds_free_login(connection);
		/* what to do? */
		fprintf(stderr, "Could not open connection to server %s\n", pool->server);
		return NULL;
	}
	tds_free_login(connection);
	/*
	 * FIXME -- tds_connect_and_login no longer preallocates the in_buf need to 
	 * do something like what tds_read_packet does
	 */
	tds->in_buf = (unsigned char *) calloc(BLOCKSIZ, 1);

	if (pool->database && strlen(pool->database)) {
		query = (char *) malloc(strlen(pool->database) + 5);
		sprintf(query, "use %s", pool->database);
		rc = tds_submit_query(tds, query);
		free(query);
		if (TDS_FAILED(rc)) {
			fprintf(stderr, "changing database failed\n");
			return NULL;
		}

		if (TDS_FAILED(tds_process_simple_query(tds)))
			return NULL;
	}


	return tds;
}

void
pool_assign_member(TDS_POOL_MEMBER * pmbr, TDS_POOL_USER *puser)
{
	pmbr->current_user = puser;
	puser->assigned_member = pmbr;
}

void
pool_deassign_member(TDS_POOL_MEMBER * pmbr)
{
	if (pmbr->current_user)
		pmbr->current_user->assigned_member = NULL;
	pmbr->current_user = NULL;
}

/*
 * if a dead connection on the client side left this member in a questionable
 * state, let's clean it up.
 */
void
pool_reset_member(TDS_POOL_MEMBER * pmbr)
{
	pool_free_member(pmbr);
}

void
pool_free_member(TDS_POOL_MEMBER * pmbr)
{
	if (!IS_TDSDEAD(pmbr->tds)) {
		tds_close_socket(pmbr->tds);
	}
	pmbr->tds = NULL;
	/*
	 * if he is allocated disconnect the client 
	 * otherwise we end up with broken client.
	 */
	if (pmbr->current_user) {
		pool_free_user(pmbr->current_user);
		pmbr->current_user = NULL;
	}
	pmbr->state = TDS_IDLE;
}

void
pool_mbr_init(TDS_POOL * pool)
{
	TDS_POOL_MEMBER *pmbr;
	int i;

	/* allocate room for pool members */

	pool->members = (TDS_POOL_MEMBER *)
		calloc(pool->num_members, sizeof(TDS_POOL_MEMBER));

	/* open connections for each member */

	for (i = 0; i < pool->num_members; i++) {
		pmbr = &pool->members[i];
		if (i < pool->min_open_conn) {
			pmbr->tds = pool_mbr_login(pool);
			pmbr->last_used_tm = time(NULL);
			if (!pmbr->tds) {
				fprintf(stderr, "Could not open initial connection %d\n", i);
				exit(1);
			}
		}
		pmbr->state = TDS_IDLE;
	}
}

/* 
 * pool_process_members
 * check the fd_set for members returning data to the client, lookup the 
 * client holding this member and forward the results.
 */
int
pool_process_members(TDS_POOL * pool, fd_set * fds)
{
	TDS_POOL_MEMBER *pmbr;
	TDS_POOL_USER *puser;
	TDSSOCKET *tds;
	int i, age, ret;
	int cnt = 0;
	unsigned char *buf;
	time_t time_now;

	for (i = 0; i < pool->num_members; i++) {
		pmbr = (TDS_POOL_MEMBER *) & pool->members[i];

		if (!pmbr->tds)
			break;	/* dead connection */

		tds = pmbr->tds;
		time_now = time(NULL);
		if (FD_ISSET(tds_get_s(tds), fds)) {
			pmbr->last_used_tm = time_now;
			cnt++;
			/* tds->in_len = read(tds->s, tds->in_buf, BLOCKSIZ); */
			if (pool_packet_read(pmbr))
				continue;

			if (tds->in_len == 0) {
				fprintf(stderr, "Uh oh! member %d disconnected\n", i);
				/* mark as dead */
				pool_free_member(pmbr);
			} else if (tds->in_len == -1) {
				fprintf(stderr, "Uh oh! member %d disconnected\n", i);
				perror("read");
				pool_free_member(pmbr);
			} else {
				/* fprintf(stderr, "read %d bytes from member %d\n", tds->in_len, i); */
				if (pmbr->current_user) {
					puser = pmbr->current_user;
					buf = tds->in_buf;
					/* 
					 * check the netlib final packet flag
					 * instead of looking for done tokens.
					 * It's more efficient and generic to 
					 * all protocol versions. -- bsb 
					 * 2004-12-12 
					 */
					if (buf[1]) {
					/* if (pool_find_end_token(pmbr, buf + 8, tds->in_len - 8)) { */
						/* we are done...deallocate member */
						fprintf(stdout, "deassigning user from member %d\n",i);
						pool_deassign_member(pmbr);

						pmbr->state = TDS_IDLE;
						puser->user_state = TDS_SRV_IDLE;
					}
					/* cf. net.c for better technique.  */
					ret = WRITESOCKET(tds_get_s(puser->tds), buf, tds->in_len);
					if (ret < 0) { /* couldn't write, ditch the user */
						fprintf(stdout, "member %d received error while writing\n",i);
						pool_free_user(pmbr->current_user);
						pool_deassign_member(pmbr);
						pool_reset_member(pmbr);
					}
				}
			}
		}
		age = time_now - pmbr->last_used_tm;
		if (age > pool->max_member_age && i >= pool->min_open_conn) {
			fprintf(stderr, "member %d is %d seconds old...closing\n", i, age);
			pool_free_member(pmbr);
		}
	}
	return cnt;
}

/*
 * pool_find_idle_member
 * returns the first pool member in TDS_IDLE state
 */
TDS_POOL_MEMBER *
pool_find_idle_member(TDS_POOL * pool)
{
	int i, active_members;
	TDS_POOL_MEMBER *pmbr;

	active_members = 0;
	for (i = 0; i < pool->num_members; i++) {
		pmbr = &pool->members[i];
		if (pmbr->tds) {
			active_members++;
			if (pmbr->state == TDS_IDLE) {
				/*
				 * make sure member wasn't idle more that the timeout 
				 * otherwise it'll send the query and close leaving a
				 * hung client
				 */
				pmbr->last_used_tm = time(NULL);
				return pmbr;
			}
		}
	}
	/* if we have dead connections we can open */
	if (active_members < pool->num_members) {
		pmbr = NULL;
		for (i = 0; i < pool->num_members; i++) {
			pmbr = &pool->members[i];
			if (!pmbr->tds) {
				fprintf(stderr, "No open connections left, opening member number %d\n", i);
				pmbr->tds = pool_mbr_login(pool);
				pmbr->last_used_tm = time(NULL);
				break;
			}
		}
		if (pmbr)
			return pmbr;
	}
	fprintf(stderr, "No idle members left, increase MAX_POOL_CONN\n");
	return NULL;
}

static int
pool_packet_read(TDS_POOL_MEMBER * pmbr)
{
	TDSSOCKET *tds;
	int packet_len;

	tds = pmbr->tds;

	if (pmbr->need_more) {
		tds->in_len += read(tds_get_s(tds), &tds->in_buf[tds->in_len], BLOCKSIZ - tds->in_len);
	} else {
		tds->in_len = read(tds_get_s(tds), tds->in_buf, BLOCKSIZ);
	}
	packet_len = ntohs(*(short *) &tds->in_buf[2]);
	if (tds->in_len < packet_len) {
		pmbr->need_more = 1;
	} else {
		pmbr->need_more = 0;
	}
	return pmbr->need_more;
}
