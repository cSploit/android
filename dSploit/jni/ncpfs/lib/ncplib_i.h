/*
    ncplib.h
    Copyright (C) 1995, 1996 by Volker Lendecke
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

	0.00  1995			Volker Lendecke
		Initial revision.

	0.01  1995-1999			see ncplib.c
		See ncplib.c

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
		
	1.01  2000, January 16		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changes for 32bit uids.

	1.02  2001, February 24		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added broadcast state information.
		Added global_fd code.
	
	1.03  2001, March 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added conn_ring code.

	1.04  2001, June 2		Petr Vandrovec <vandrove@vc.cvut.cz>
		Moved shuffle() declaration here from ndscrypt.h.

	1.05  2001, July 16		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added put_req_size_unsigned prototype.

 */

#ifndef _NCPLIB_I_H
#define _NCPLIB_I_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ncp/ncplib.h>

#include "private/list.h"
#include "private/libncp-lock.h"
#include "private/libncp-atomic.h"
#include "private/ncp_fs.h"

#include <string.h>

#define UNUSED(x) x __attribute__((unused))

#define ncp_const_cast(x) ((void*)(unsigned long)(x))

#define NWCC_BCAST_PERMIT_UNKNOWN	-9876	/* something unused */

struct ncp_conn {

	enum connect_state is_connected;

	struct NWDSConnection* nds_conn;
	struct list_head nds_ring;
	
	char* user;
	int user_id_valid;
	NWObjectID user_id;

	struct ncp_fs_info_v2 i;
	/* Fields for use with permanent connections */
	int mount_fid;
	
	char* mount_point;
	ncpt_atomic_t store_count;
	u_int32_t state;

	union ncp_sockaddr addr;
	struct {
		size_t			count;
		struct tagBuf_T*	buffer;
	} serverAddresses;
#define CONNECTION_AUTHENTICATED	0x8000
#define CONNECTION_LICENSED		0x0004
	unsigned int connState;

	/* Fields for use with temporary connections */
	int ncp_sock;
	int wdog_sock;
	int wdog_pipe;
	u_int8_t sequence;
	ncpt_atomic_t use_count;	/* it was completion code */
	int conn_status;

	/* Fields used to setup ncp requests */
	char *current_point;
	int has_subfunction;
	int verbose;
	size_t ncp_reply_size;
	
	void *private_key;
	size_t private_key_len;

	int lock;

	char packet[NCP_PACKET_SIZE];
	char *ncp_reply;
	char *ncp_reply_buffer;
	size_t ncp_reply_alloc;

	/* Field used to make packet signatures */
	int sign_wanted;
	int sign_active;
	struct ncp_sign_init sign_data;
	ncpt_mutex_t buffer_mutex;
	struct {
		ncpt_mutex_t mutex;
		unsigned char valid;
		char* serverName;
		struct {
			unsigned int major, minor, revision;
		} version;
		int ncp64bit;
	} serverInfo;
	
	enum NET_ADDRESS_TYPE nt;
	
	int bcast_state;
	int global_fd;
	
	struct list_head conn_ring;
};

/* saving pointer somewhere */
static inline long ncp_conn_store(struct ncp_conn* conn) {
	ncpt_atomic_inc(&conn->store_count);
	return 0;
}

/* forgetting pointer */
long ncp_conn_release(struct ncp_conn* conn);

/* using pointer... */
static inline long ncp_conn_use(struct ncp_conn* conn) {
	ncpt_atomic_inc(&conn->use_count);
	return 0;
}
/* ncp_close is counterpart to ncp_conn_use... */

long ncp_request(struct ncp_conn* conn, int request);
void ncp_lock_conn(struct ncp_conn* conn);
void ncp_unlock_conn(struct ncp_conn* conn);

#ifdef NCP_TRACE_ENABLE
void __ncp_trace(char* module, int line, char* p, ...);
void __dump_hex(const char* msg, const unsigned char* buf, size_t len);
#define NCP_TRACE(X...)	__ncp_trace(__FILE__, __LINE__, X)
#define DUMP_HEX(msg,buf,len) __dump_hex(msg, buf, len)
#define ncp_printf NCP_TRACE
#else
#define NCP_TRACE(X...)
#define DUMP_HEX(msg,buf,len)
#define ncp_printf printf
#endif

static inline void assert_conn_locked(struct ncp_conn *conn) {
        if (conn->lock == 0) {
		ncp_printf("ncpfs: connection not locked!\n");
        }
}

#define DEFINE_ASSERT_FUNCTION(name,type)	\
static inline void assert_##name(struct ncp_conn *conn, type x ) {	\
        assert_conn_locked(conn);					\
	name(conn, x);							\
}

static inline void ncp_add_byte(struct ncp_conn *conn, byte x) {
	BSET(conn->current_point, 0, x); 
	conn->current_point++;
}

static inline void ncp_add_word_lh(struct ncp_conn *conn, word x) {
	WSET_LH(conn->current_point, 0, x);
	conn->current_point += 2;
}

static inline void ncp_add_word_hl(struct ncp_conn *conn, word x) {
	WSET_HL(conn->current_point, 0, x);
	conn->current_point += 2;
}

static inline void ncp_add_dword_lh(struct ncp_conn *conn, dword x) {
	DSET_LH(conn->current_point, 0, x);
	conn->current_point += 4;
}

static inline void ncp_add_dword_hl(struct ncp_conn *conn, dword x) {
	DSET_HL(conn->current_point, 0, x);
	conn->current_point += 4;
}

static inline void ncp_add_qword_hl(struct ncp_conn *conn, u_int64_t x) {
	QSET_HL(conn->current_point, 0, x);
	conn->current_point += 8;
}

DEFINE_ASSERT_FUNCTION(ncp_add_byte,byte)
DEFINE_ASSERT_FUNCTION(ncp_add_word_lh,word)
DEFINE_ASSERT_FUNCTION(ncp_add_word_hl,word)
DEFINE_ASSERT_FUNCTION(ncp_add_dword_lh,dword)
DEFINE_ASSERT_FUNCTION(ncp_add_dword_hl,dword)

static inline void ncp_add_mem(struct ncp_conn *conn, const void *source, int size) {
	assert_conn_locked(conn);
	memcpy(conn->current_point, source, size);
	conn->current_point += size;
}

static inline int ncp_add_seek(struct ncp_conn *conn, int pos) {
	assert_conn_locked(conn);
	if (conn->current_point > conn->packet + pos)
		return NWE_BUFFER_OVERFLOW;
	conn->current_point = conn->packet + pos;
	return 0;
}

void ncp_add_pstring(struct ncp_conn *conn, const char *s);

static inline char* ncp_reply_data(struct ncp_conn *conn, int offset) {
	return conn->ncp_reply + sizeof(struct ncp_reply_header) + offset;
}

static inline byte ncp_reply_byte(struct ncp_conn *conn, int offset) {
	return BVAL(ncp_reply_data(conn, offset), 0);
}

static inline word ncp_reply_word_hl(struct ncp_conn *conn, int offset) {
	return WVAL_HL(ncp_reply_data(conn, offset), 0);
}

static inline word ncp_reply_word_lh(struct ncp_conn *conn, int offset) {
	return WVAL_LH(ncp_reply_data(conn, offset), 0);
}

static inline dword ncp_reply_dword_hl(struct ncp_conn *conn, int offset) {
	return DVAL_HL(ncp_reply_data(conn, offset), 0);
}

static inline dword ncp_reply_dword_lh(struct ncp_conn *conn, int offset) {
	return DVAL_LH(ncp_reply_data(conn, offset), 0);
}

static inline u_int64_t ncp_reply_qword_lh(struct ncp_conn *conn, int offset) {
	return QVAL_LH(ncp_reply_data(conn, offset), 0);
}

static inline u_int32_t cpu_to_le32(u_int32_t val) {
	return DVAL_LH(&val, 0);
}

static inline u_int32_t cpu_to_be32(u_int32_t val) {
	return htonl(val);
}

static inline unsigned int min(unsigned int a, unsigned int b) {
	return (a < b) ? a : b;
}

static inline void ConvertToNWfromDWORD(u_int32_t sfd, char nwhandle[6]) {
	DSET_LH(nwhandle, 2, sfd);
	WSET_LH(nwhandle, 0, sfd+1);
}

static inline u_int32_t ConvertToDWORDfromNW(const char nwhandle[6]) {
	return DVAL_LH(nwhandle, 2);
}

void ncp_init_request(struct ncp_conn *conn);
void ncp_init_request_s(struct ncp_conn *conn, int subfunction);

long ncp_sign_start(struct ncp_conn *conn, const char *sign_root);

void shuffle(const unsigned char *objid, const unsigned char *pwd, int buflen, unsigned char *out);

NWCCODE ncp_set_private_key(struct ncp_conn *conn, const void* pk, size_t pk_len);
NWCCODE ncp_get_private_key(struct ncp_conn *conn, void* pk, size_t* pk_len);

NWCCODE ncp_next_conn(struct ncp_conn *conn, struct ncp_conn **nextConn);

#ifdef SIGNATURES
static inline int ncp_get_sign_wanted(struct ncp_conn *conn) {
	return conn->sign_wanted;
}

static inline int ncp_get_sign_active(struct ncp_conn *conn) {
	return conn->sign_active;
}
#else
#define ncp_get_sign_wanted(X) (0)
#define ncp_get_sign_active(X) (0)
#endif

extern ncpt_mutex_t nds_ring_lock;

NWCCODE ncp_put_req_size_unsigned(void *buffer, size_t reqlen, unsigned long value);

NWCCODE x_recvfrom(int sock, void *buf, int len, unsigned int flags,
	       struct sockaddr *sender, socklen_t* addrlen, int timeout,
	       size_t *rlen);

static inline NWCCODE x_recv(int sock, void *buf, int len, unsigned int flags, int timeout, size_t *rlen) {
	return x_recvfrom(sock, buf, len, flags, NULL, 0, timeout, rlen);
}

NWCCODE __NWReadFileServerInfo(struct ncp_conn* conn);

#ifdef __cplusplus
}
#endif

#endif	/* _NCPLIB_I_H */
