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

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#include "pool.h"
#include <freetds/server.h>
#include <freetds/string.h>

TDS_RCSID(var, "$Id: user.c,v 1.38 2012-03-11 15:52:22 freddy77 Exp $");

static TDS_POOL_USER *pool_user_find_new(TDS_POOL * pool);
static int pool_user_login(TDS_POOL * pool, TDS_POOL_USER * puser);
static void pool_user_read(TDS_POOL * pool, TDS_POOL_USER * puser);

extern int waiters;

void
pool_user_init(TDS_POOL * pool)
{
	/* allocate room for pool users */

	pool->users = (TDS_POOL_USER *)
		calloc(MAX_POOL_USERS, sizeof(TDS_POOL_USER));
}

static TDS_POOL_USER *
pool_user_find_new(TDS_POOL * pool)
{
	TDS_POOL_USER *puser;
	int i;

	/* first check for dead users to reuse */
	for (i=0; i<pool->max_users; i++) {
		puser = (TDS_POOL_USER *) & pool->users[i];
		if (!puser->tds)
			return puser;
	}

	/* did we exhaust the number of concurrent users? */
	if (pool->max_users >= MAX_POOL_USERS) {
		fprintf(stderr, "Max concurrent users exceeded, increase in pool.h\n");
		return NULL;
	}

	/* else take one off the top of the pool->users */
	puser = (TDS_POOL_USER *) & pool->users[pool->max_users];
	pool->max_users++;

	return puser;
}

/*
 * pool_user_create
 * accepts a client connection and adds it to the users list and returns it
 */
TDS_POOL_USER *
pool_user_create(TDS_POOL * pool, TDS_SYS_SOCKET s, struct sockaddr_in *sin)
{
	TDS_POOL_USER *puser;
	TDS_SYS_SOCKET fd;
	socklen_t len;
	TDSSOCKET *tds;

	puser = pool_user_find_new(pool);
	if (!puser)
		return NULL;

	fprintf(stderr, "accepting connection\n");
	len = sizeof(*sin);
	if (TDS_IS_SOCKET_INVALID(fd = tds_accept(s, (struct sockaddr *) sin, &len))) {
		perror("accept");
		return NULL;
	}
	tds = tds_alloc_socket(NULL, BLOCKSIZ);
	if (!tds) {
		CLOSESOCKET(fd);
		return NULL;
	}
	tds_set_parent(tds, NULL);
	/* FIX ME - little endian emulation should be config file driven */
	tds_conn(tds)->emul_little_endian = 1;
	tds->in_buf = (unsigned char *) calloc(1, BLOCKSIZ);
	tds_set_s(tds, fd);
	if (!tds->in_buf) {
		tds_free_socket(tds);
		return NULL;
	}
	tds->in_buf_max = BLOCKSIZ;
	tds->out_flag = TDS_LOGIN;
	puser->tds = tds;
	puser->user_state = TDS_SRV_LOGIN;
	return puser;
}

/* 
 * pool_free_user
 * close out a disconnected user.
 */
void
pool_free_user(TDS_POOL_USER * puser)
{
	/* make sure to decrement the waiters list if he is waiting */
	if (puser->user_state == TDS_SRV_WAIT)
		waiters--;
	tds_free_socket(puser->tds);
	memset(puser, 0, sizeof(TDS_POOL_USER));
}

/* 
 * pool_process_users
 * check the fd_set for user input, allocate a pool member to it, and forward
 * the query to that member.
 */
int
pool_process_users(TDS_POOL * pool, fd_set * fds)
{
	TDS_POOL_USER *puser;
	int i;
	int cnt = 0;

	for (i = 0; i < pool->max_users; i++) {

		puser = (TDS_POOL_USER *) & pool->users[i];

		if (!puser->tds)
			continue;	/* dead connection */

		if (FD_ISSET(tds_get_s(puser->tds), fds)) {
			cnt++;
			switch (puser->user_state) {
			case TDS_SRV_LOGIN:
				if (pool_user_login(pool, puser)) {
					/* login failed...free socket */
					pool_free_user(puser);
				}
				/* otherwise we have a good login */
				break;
			case TDS_SRV_IDLE:
				pool_user_read(pool, puser);
				break;
			case TDS_SRV_QUERY:
				/* what is this? a cancel perhaps */
				pool_user_read(pool, puser);
				break;
			/* just to avoid a warning */
			default:
				break;
			}	/* switch */
		}		/* if */
	}			/* for */
	return cnt;
}

/*
 * pool_user_login
 * Reads clients login packet and forges a login acknowledgement sequence 
 */
static int
pool_user_login(TDS_POOL * pool, TDS_POOL_USER * puser)
{
	TDSSOCKET *tds;
	TDSLOGIN *login = tds_alloc_login(1);

	/* FIXME */
	char msg[256];

	tds = puser->tds;
	tds_read_login(tds, login);
	dump_login(login);
	if (!strcmp(tds_dstr_cstr(&login->user_name), pool->user) && !strcmp(tds_dstr_cstr(&login->password), pool->password)) {
		tds->out_flag = TDS_REPLY;
		tds_env_change(tds, 1, "master", pool->database);
		sprintf(msg, "Changed database context to '%s'.", pool->database);
		tds_send_msg(tds, 5701, 2, 10, msg, "JDBC", "ZZZZZ", 1);
		if (!login->suppress_language) {
			tds_env_change(tds, 2, NULL, "us_english");
			tds_send_msg(tds, 5703, 1, 10, "Changed language setting to 'us_english'.", "JDBC", "ZZZZZ", 1);
		}
		tds_env_change(tds, 4, NULL, "512");
		tds_send_login_ack(tds, "sql server");
		/* tds_send_capabilities_token(tds); */
		tds_send_done_token(tds, 0, 1);
		puser->user_state = TDS_SRV_IDLE;

		/* send it! */
		tds_flush_packet(tds);
		tds_free_login(login);

		return 0;
	} else {
		tds_free_login(login);
		/* send nack before exiting */
		return 1;
	}
}

/*
 * pool_user_read
 * checks the packet type of data coming from the client and allocates a 
 * pool member if necessary.
 */
static void
pool_user_read(TDS_POOL * pool, TDS_POOL_USER * puser)
{
	TDSSOCKET *tds;
	TDS_POOL_MEMBER *pmbr;

	tds = puser->tds;
	/* FIXME read entire packet !!! */
	tds->in_len = read(tds_get_s(tds), tds->in_buf, tds->in_buf_max);
	if (tds->in_len == 0) {
		fprintf(stderr, "user disconnected\n");
		pool_free_user(puser);
		return;
	} else if (tds->in_len == -1) {
		perror("read");
		fprintf(stderr, "cleaning up user\n");
		if (puser->assigned_member) {
			fprintf(stderr, "user has assigned member, freeing\n");
			pmbr = puser->assigned_member;
			pool_deassign_member(pmbr);
			pool_reset_member(pmbr);
		}
		pool_free_user(puser);
	} else {
		dump_buf(tds->in_buf, tds->in_len);
		/* language packet or TDS5 language packet */
		if (tds->in_buf[0] == 0x01 || tds->in_buf[0] == 0x0F) {
			pool_user_query(pool, puser);
		} else if (tds->in_buf[0] == 0x06) {
			/* cancel */
		} else {
			fprintf(stderr, "Unrecognized packet type, closing user\n");
			pool_free_user(puser);
		}
	}
	/* fprintf(stderr,"read %d bytes from conn %d\n",len,i); */
}

void
pool_user_query(TDS_POOL * pool, TDS_POOL_USER * puser)
{
	TDS_POOL_MEMBER *pmbr;
	int ret;
	
	puser->user_state = TDS_SRV_QUERY;
	pmbr = pool_find_idle_member(pool);
	if (!pmbr) {
		/* 
		 * put into wait state 
		 * check when member is deallocated
		 */
		fprintf(stderr, "Not enough free members...placing user in WAIT\n");
		puser->user_state = TDS_SRV_WAIT;
		waiters++;
	} else {
		pmbr->state = TDS_QUERYING;
		pool_assign_member(pmbr, puser);
		/* cf. net.c for better technique.  */
		ret = WRITESOCKET(tds_get_s(pmbr->tds), puser->tds->in_buf, puser->tds->in_len);
		/* write failed, cleanup member */
		if (ret < 0) {
			pool_free_member(pmbr);
		}
	}
}
