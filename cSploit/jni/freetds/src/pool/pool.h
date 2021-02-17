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

#ifndef _pool_h_
#define _pool_h_

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

/* 
 * POSIX says fd_set type may be defined in either sys/select.h or sys/time.h. 
 */
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <freetds/tds.h>

TDS_RCSID(pool_h, "$Id: pool.h,v 1.15 2006-06-12 19:45:59 freddy77 Exp $");

/* defines */
#define PGSIZ 2048
#define BLOCKSIZ 512
#define MAX_POOL_USERS 1024

/* enums and typedefs */
typedef enum
{
	TDS_SRV_LOGIN,
	TDS_SRV_IDLE,
	TDS_SRV_QUERY,
	TDS_SRV_WAIT,		/* if no members are free wait */
	TDS_SRV_CANCEL,
	TDS_SRV_DEAD
} TDS_USER_STATE;

/* forward declaration */
typedef struct tds_pool_member TDS_POOL_MEMBER;


typedef struct tds_pool_user
{
	TDSSOCKET *tds;
	TDS_USER_STATE user_state;
	TDS_POOL_MEMBER *assigned_member;
}
TDS_POOL_USER;

struct tds_pool_member
{
	TDSSOCKET *tds;
	/* sometimes we get a partial packet */
	int need_more;
	int state;
	time_t last_used_tm;
	TDS_POOL_USER *current_user;
	/* 
	 * these variables are used for tracking the state of the TDS protocol 
	 * so we know when to return the state to TDS_IDLE.
	 */
	int num_bytes_left;
	unsigned char fragment[PGSIZ];
};

typedef struct tds_pool
{
	char *name;
	char *user;
	char *password;
	char *server;
	char *database;
	int port;
	int max_member_age;	/* in seconds */
	int min_open_conn;
	int max_open_conn;
	int num_members;
	TDS_POOL_MEMBER *members;
	int max_users;
	TDS_POOL_USER *users;
}
TDS_POOL;

/* prototypes */

/* member.c */
int pool_process_members(TDS_POOL * pool, fd_set * fds);
TDS_POOL_MEMBER *pool_find_idle_member(TDS_POOL * pool);
void pool_mbr_init(TDS_POOL * pool);
void pool_free_member(TDS_POOL_MEMBER * pmbr);
void pool_assign_member(TDS_POOL_MEMBER * pmbr, TDS_POOL_USER *puser);
void pool_deassign_member(TDS_POOL_MEMBER * pmbr);
void pool_reset_member(TDS_POOL_MEMBER * pmbr);

/* user.c */
int pool_process_users(TDS_POOL * pool, fd_set * fds);
void pool_user_init(TDS_POOL * pool);
TDS_POOL_USER *pool_user_create(TDS_POOL * pool, TDS_SYS_SOCKET s, struct sockaddr_in *sin);
void pool_free_user(TDS_POOL_USER * puser);
void pool_user_query(TDS_POOL * pool, TDS_POOL_USER * puser);

/* util.c */
void dump_buf(const void *buf, int length);
void dump_login(TDSLOGIN * login);
void die_if(int expr, const char *msg);

/* config.c */
int pool_read_conf_file(char *poolname, TDS_POOL * pool);


#endif
