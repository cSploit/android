/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2012 Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_POLL_H
#include <poll.h>
#endif /* HAVE_POLL_H */

#include <freetds/tds.h>
#include <freetds/bytes.h>
#include <freetds/iconv.h>
#include "replacements.h"
#ifdef DMALLOC
#include <dmalloc.h>
#endif

/**
 * \addtogroup network
 * @{ 
 */

#if ENABLE_ODBC_MARS
static TDSRET tds_update_recv_wnd(TDSSOCKET *tds, TDS_UINT new_recv_wnd);
static short tds_packet_write(TDSCONNECTION *conn);

/* read partial packet */
static void
tds_packet_read(TDSCONNECTION *conn, TDSSOCKET *tds)
{
	TDSPACKET *packet = conn->recv_packet;
	int len;

	/* allocate some space to read data (minimun 8 bytes) */
	if (!packet) {
		conn->recv_packet = packet = tds_alloc_packet(NULL, 8);
		if (!packet) goto Memory_Error;
	}

	assert(packet->pos >= 0 && packet->len > 0 && packet->pos < packet->len);

	len = tds_connection_read(tds, packet->buf+packet->pos, packet->len - packet->pos);
	if (len < 0)
		goto Severe_Error;
	packet->pos += len;
	assert(packet->pos >= 0 && packet->len > 0 && packet->pos <= packet->len);

	/* handle SMP */
	if (packet->pos > 0 && packet->buf[0] == TDS72_SMP) {
		TDS72_SMP_HEADER mars_header;
		short sid;
		TDSSOCKET *tds;
		TDS_UINT size;

		if (packet->pos < 16) {
			if (packet->len < 16) {
				packet = tds_realloc_packet(packet, 16);
				if (!packet) goto Memory_Error;
				conn->recv_packet = packet;
			}
			return;
		}

		memcpy(&mars_header, packet->buf, sizeof(mars_header));
		tdsdump_dump_buf(TDS_DBG_HEADER, "Received MARS header", &mars_header, sizeof(mars_header));
		sid = TDS_GET_A2LE(&mars_header.sid);

		/* FIXME this is done even by caller !! */
		tds = NULL;
		tds_mutex_lock(&conn->list_mtx);
		if (sid >= 0 && sid < conn->num_sessions)
			tds = conn->sessions[sid];
		tds_mutex_unlock(&conn->list_mtx);
		packet->sid = sid;

		if (tds == BUSY_SOCKET) {
			if (mars_header.type != TDS_SMP_FIN) {
				tdsdump_log(TDS_DBG_ERROR, "Received MARS with no session (%d)\n", sid);
				goto Severe_Error;
			}

			/* check if was just a "zombie" socket */
			tds_mutex_lock(&conn->list_mtx);
			conn->sessions[sid] = NULL;
			tds_mutex_unlock(&conn->list_mtx);

			tds_free_packets(packet);
			conn->recv_packet = NULL;
			return;
		}

		if (!tds) {
			/* server sent a unknown packet, close connection */
			goto Severe_Error;
		}

		tds->send_wnd = TDS_GET_A4LE(&mars_header.wnd);
		size = TDS_GET_A4LE(&mars_header.size);
		if (mars_header.type == TDS_SMP_ACK) {
			if (size != sizeof(mars_header))
				goto Severe_Error;
		} else if (mars_header.type == TDS_SMP_DATA) {
			if (size < 0x18)
				goto Severe_Error;
			/* avoid recursive SMP */
			if (packet->len > 16 && packet->buf[16] == TDS72_SMP)
				goto Severe_Error;
			/* TODO is possible to put 2 TDS packet inside a single DATA ?? */
			if (packet->len >= 20 && TDS_GET_A2BE(&packet->buf[18]) != size - 16)
				goto Severe_Error;
			tds->recv_seq = TDS_GET_A4LE(&mars_header.seq);
			/*
			 * does not sent ACK here cause this would lead to memory waste
			 * if session is not able to handle all that packets
			 */
		} else if (mars_header.type == TDS_SMP_FIN) {
			if (size != sizeof(mars_header))
				goto Severe_Error;
			/* this socket shold now not start another session */
//			tds_set_state(tds, TDS_DEAD);
//			tds->sid = -1;
		} else
			goto Severe_Error;

		if (mars_header.type != TDS_SMP_DATA)
			return;
		if (packet->len < 0x18) {
			packet = tds_realloc_packet(packet, 0x18);
			if (!packet) goto Memory_Error;
			conn->recv_packet = packet;
			return;
		}
		packet->pos -= 16;
		packet->len -= 16;
		memmove(packet->buf, packet->buf+16, packet->pos);
	}
	assert(packet->pos >= 0 && packet->len > 0 && packet->pos <= packet->len);

	/* normal packet */
	if (packet->pos >= 8) {
		len = TDS_GET_A2BE(&packet->buf[2]);
		if (len < 8)
			goto Severe_Error;
		if (packet->len < len) {
			packet = tds_realloc_packet(packet, len);
			if (!packet) goto Memory_Error;
		}
		conn->recv_packet = packet;
	}
	return;

Memory_Error:
Severe_Error:
	tds_connection_close(conn);
	tds_free_packets(packet);
	conn->recv_packet = NULL;
}

static void
tds_alloc_new_sid(TDSSOCKET *tds)
{
	int sid = -1;
	TDSCONNECTION *conn = tds_conn(tds);
	TDSSOCKET **s;

	tds_mutex_lock(&conn->list_mtx);
	tds->sid = -1;
	for (sid = 0; sid < conn->num_sessions; ++sid)
		if (!conn->sessions[sid])
			break;
	if (sid == conn->num_sessions) {
		/* extend array */
		s = (TDSSOCKET **) realloc(conn->sessions, sizeof(*s) * (sid+64));
		if (!s) goto error;
		conn->sessions = s;
		memset(s + conn->num_sessions, 0, sizeof(*s) * 64);
		conn->num_sessions += 64;
	}
	conn->sessions[sid] = tds;
	tds->sid = sid;
error:
	tds_mutex_unlock(&conn->list_mtx);
}

static TDSPACKET*
tds_build_packet(TDSSOCKET *tds, unsigned char *buf, unsigned len)
{
	unsigned start;
	TDS72_SMP_HEADER mars[2], *p;
	TDSPACKET *packet;

	p = mars;
	if (buf[0] != TDS72_SMP && tds->conn->mars) {
		/* allocate a new sid */
		if (tds->sid == -1) {
			p->signature = TDS72_SMP;
			p->type = TDS_SMP_SYN; /* start session */
			/* FIXME check !!! */
			tds_alloc_new_sid(tds);
			tds->recv_seq = 0;
			tds->send_seq = 0;
			tds->recv_wnd = 4;
			tds->send_wnd = 4;
			TDS_PUT_A2LE(&p->sid, tds->sid);
			p->size = TDS_HOST4LE(0x10);
			p->seq = TDS_HOST4LE(0);
			TDS_PUT_A4LE(&p->wnd, tds->recv_wnd);
			p++;
		}
		if (tds->sid >= 0) {
			p->signature = TDS72_SMP;
			p->type = TDS_SMP_DATA;
			TDS_PUT_A2LE(&p->sid, tds->sid);
			TDS_PUT_A4LE(&p->size, len+16);
			TDS_PUT_A4LE(&p->seq, ++tds->send_seq);
			/* this is the acknowledge we give to server to stop sending !!! */
			tds->recv_wnd = tds->recv_seq + 4;
			TDS_PUT_A4LE(&p->wnd, tds->recv_wnd);
			p++;
		}
	}

	start = (p - mars) * sizeof(mars[0]);
	packet = tds_alloc_packet(NULL, len + start);
	if (TDS_LIKELY(packet)) {
		packet->sid = tds->sid;
		memcpy(packet->buf, mars, start);
		memcpy(packet->buf + start, buf, len);
	}
	return packet;
}

static void
tds_append_packet(TDSPACKET **p_packet, TDSPACKET *packet)
{
	while (*p_packet)
		p_packet = &((*p_packet)->next);
	*p_packet = packet;
}

int
tds_append_cancel(TDSSOCKET *tds)
{
	unsigned char buf[8];
	TDSPACKET *packet;

	buf[0] = TDS_CANCEL;
	buf[1] = 1;
	TDS_PUT_A2BE(buf+2, 8);
	TDS_PUT_A4(buf+4, 0);
	if (IS_TDS7_PLUS(tds->conn) && !tds->login)
		buf[6] = 0x01;

	packet = tds_build_packet(tds, buf, 8);
	if (!packet)
		return TDS_FAIL;

	tds_append_packet(&tds->conn->send_packets, packet);

	return TDS_SUCCESS;
}


static void
tds_connection_network(TDSCONNECTION *conn, TDSSOCKET *tds, int send)
{
	assert(!conn->in_net_tds);
	conn->in_net_tds = tds;
	tds_mutex_unlock(&conn->list_mtx);

	for (;;) {
		/* wait packets or update */
		int rc = tds_select(tds, conn->send_packets ? TDSSELREAD|TDSSELWRITE : TDSSELREAD, tds->query_timeout);

		if (rc < 0) {
			/* FIXME better error report */
			tds_connection_close(conn);
			break;
		}

		/* change notify */
		/* TODO async */

		if (!rc) { /* timeout */
			tdsdump_log(TDS_DBG_INFO1, "timeout\n");
			switch (rc = tdserror(tds_get_ctx(tds), tds, TDSETIME, sock_errno)) {
			case TDS_INT_CONTINUE:
				continue;
			default:
			case TDS_INT_CANCEL:
				tds_close_socket(tds);
			}
			break;
		}

		/*
		 * we must write first to catch write errors as
		 * write errors, not as read
		 */
		/* something to send */
		if (conn->send_packets && (rc & POLLOUT) != 0) {
			TDSSOCKET *s;

			short sid = tds_packet_write(conn);
			if (sid == tds->sid) break;	/* return to caller */

			tds_mutex_lock(&conn->list_mtx);
			if (sid >= 0 && sid < conn->num_sessions) {
				s = conn->sessions[sid];
				if (TDSSOCKET_VALID(s))
					tds_cond_signal(&s->packet_cond);
			}
			tds_mutex_unlock(&conn->list_mtx);
			/* avoid using a possible closed connection */
			continue;
		}

		/* received */
		if (rc & POLLIN) {
			TDSPACKET *packet;
			TDSSOCKET *s;

			/* try to read a packet */
			tds_packet_read(conn, tds);
			packet = conn->recv_packet;
			if (!packet || packet->pos < packet->len) continue;
			conn->recv_packet = NULL;

			tdsdump_dump_buf(TDS_DBG_NETWORK, "Received packet", packet->buf, packet->len);

			tds_mutex_lock(&conn->list_mtx);
			assert(packet->sid >= 0);
			if (packet->sid >= 0 && packet->sid < conn->num_sessions) {
				s = conn->sessions[packet->sid];
				if (TDSSOCKET_VALID(s)) {
					/* append to correct session */
					tds_append_packet(&conn->packets, packet);
					packet = NULL;
					/* notify */
					tds_cond_signal(&s->packet_cond);
				}
			}
			tds_free_packets(packet);
			tds_mutex_unlock(&conn->list_mtx);
			/* if we are receiving return the packet */
			if (!send) break;
		}
	}

	tds_mutex_lock(&conn->list_mtx);
	conn->in_net_tds = NULL;
}

static int
tds_connection_put_packet(TDSSOCKET *tds, TDSPACKET *packet)
{
	TDSCONNECTION *conn = tds->conn;
	static const char zero = 0;

	if (!packet) {
		tds_close_socket(tds);
		return TDS_FAIL;
	}
	tds->out_pos = 0;

	tds_mutex_lock(&conn->list_mtx);
	for (;;) {
		int wait_res;

		if (IS_TDSDEAD(tds)) {
			tdsdump_log(TDS_DBG_NETWORK, "Write attempt when state is TDS_DEAD");
			break;
		}

		/* limit packet sending looking at sequence/window (blob1 test fails) */
		if (tds->send_seq <= tds->send_wnd) {
			/* append packet */
			tds_append_packet(&conn->send_packets, packet);
			packet = NULL;
		}

		/* network ok ? process network */
		if (!conn->in_net_tds) {
			tds_connection_network(conn, tds, packet ? 0 : 1);
			if (packet) continue;
			/* FIXME we are not sure we sent the packet !!! */
			break;
		}

		/* signal thread processing network to handle our packet */
		/* TODO check result */
		send(conn->s_signal, &zero, sizeof(zero), 0);

		/* wait local condition */
		wait_res = tds_cond_timedwait(&tds->packet_cond, &conn->list_mtx, tds->query_timeout);
		if (wait_res == ETIMEDOUT
		    && tdserror(tds_get_ctx(tds), tds, TDSETIME, ETIMEDOUT) != TDS_INT_CONTINUE) {
			tds_mutex_unlock(&conn->list_mtx);
			tds_close_socket(tds);
			tds_free_packets(packet);
			return TDS_FAIL;
		}
	}
	tds_mutex_unlock(&conn->list_mtx);
	if (TDS_UNLIKELY(packet)) {
		tds_free_packets(packet);
		return TDS_FAIL;
	}
	if (IS_TDSDEAD(tds))
		return TDS_FAIL;
	return TDS_SUCCESS;
}
#endif /* ENABLE_ODBC_MARS */

/**
 * Read in one 'packet' from the server.  This is a wrapped outer packet of
 * the protocol (they bundle result packets into chunks and wrap them at
 * what appears to be 512 bytes regardless of how that breaks internal packet
 * up.   (tetherow\@nol.org)
 * @return bytes read or -1 on failure
 */
int
tds_read_packet(TDSSOCKET * tds)
{
#if ENABLE_ODBC_MARS
	TDSCONNECTION *conn = tds->conn;

	tds_mutex_lock(&conn->list_mtx);

	for (;;) {
		int wait_res;
		TDSPACKET **p_packet;

		if (IS_TDSDEAD(tds)) {
			tdsdump_log(TDS_DBG_NETWORK, "Read attempt when state is TDS_DEAD\n");
			break;
		}

		/* if there is a packet for me return it */
		for (p_packet = &conn->packets; *p_packet; p_packet = &(*p_packet)->next)
			if ((*p_packet)->sid == tds->sid)
				break;

		if (*p_packet) {
			/* remove our packet from list */
			TDSPACKET *packet = *p_packet;
			*p_packet = packet->next;
			tds_mutex_unlock(&conn->list_mtx);
			packet->next = NULL;

			/* send acknowledge if needed */
			if (tds->recv_seq + 2 >= tds->recv_wnd)
				tds_update_recv_wnd(tds, tds->recv_seq + 4);

			if (packet->len > tds->in_buf_max) {
				void *p;
				if (!tds->in_buf)
					p = malloc(packet->len);
				else
					p = realloc(tds->in_buf, packet->len);
				if (!p) {
					tds_free_packets(packet);
					tds_connection_close(tds_conn(tds));
					return -1;
				}
				tds->in_buf = (unsigned char *) p;
				tds->in_buf_max = packet->len;
			}
			/* TODO avoid copy reusing packet buffer */
			memcpy(tds->in_buf, packet->buf, packet->len);
			tds->in_len = packet->len;
			tds->in_pos  = 8;
			tds->in_flag = tds->in_buf[0];
			tds_free_packets(packet);
			/* ignore any SMP packet (already handled) */
			if (tds->in_flag == TDS72_SMP)
				continue;
			return tds->in_len;
		}

		/* network ok ? process network */
		if (!conn->in_net_tds) {
			tds_connection_network(conn, tds, 0);
			continue;
		}

		/* wait local condition */
		wait_res = tds_cond_timedwait(&tds->packet_cond, &conn->list_mtx, tds->query_timeout);
		if (wait_res == ETIMEDOUT
		    && tdserror(tds_get_ctx(tds), tds, TDSETIME, ETIMEDOUT) != TDS_INT_CONTINUE) {
			tds_mutex_unlock(&conn->list_mtx);
			tds_close_socket(tds);
			return -1;
		}
	}

	tds_mutex_unlock(&conn->list_mtx);
	return -1;
#else /* !ENABLE_ODBC_MARS */
	unsigned char *pkt = tds->in_buf, *p, *end;

	if (IS_TDSDEAD(tds)) {
		tdsdump_log(TDS_DBG_NETWORK, "Read attempt when state is TDS_DEAD");
		return -1;
	}

	tds->in_len = 0;
	tds->in_pos = 0;
	for (p = pkt, end = p+8; p < end;) {
		int len = tds_connection_read(tds, p, end - p);
		if (len <= 0) {
			tds_close_socket(tds);
			return -1;
		}

		p += len;
		if (p - pkt >= 4) {
			unsigned pktlen = pkt[2] * 256u + pkt[3];
			/* packet must at least contains header */
			if (TDS_UNLIKELY(pktlen < 8)) {
				tds_close_socket(tds);
				return -1;
			}
			if (TDS_UNLIKELY(pktlen > tds->in_buf_max)) {
				pkt = (unsigned char *) realloc(tds->in_buf, pktlen);
				if (TDS_UNLIKELY(!pkt)) {
					tds_close_socket(tds);
					return -1;
				}
				p = pkt + (p-tds->in_buf);
				tds->in_buf = pkt;
				/* Set the new maximum packet size */
				tds->in_buf_max = pktlen;
			}
			end = pkt + pktlen;
		}
	}

	/* set the received packet type flag */
	tds->in_flag = pkt[0];

	/* Set the length and pos (not sure what pos is used for now */
	tds->in_len = p - pkt;
	tds->in_pos = 8;
	tdsdump_dump_buf(TDS_DBG_NETWORK, "Received packet", tds->in_buf, tds->in_len);

	return tds->in_len;
#endif /* !ENABLE_ODBC_MARS */
}

#if ENABLE_ODBC_MARS
static TDSRET
tds_update_recv_wnd(TDSSOCKET *tds, TDS_UINT new_recv_wnd)
{
	TDS72_SMP_HEADER mars;
	TDSPACKET *packet;

	if (!tds->conn->mars || tds->sid < 0)
		return TDS_SUCCESS;

	mars.signature = TDS72_SMP;
	mars.type = TDS_SMP_ACK;
	TDS_PUT_A2LE(&mars.sid, tds->sid);
	mars.size = TDS_HOST4LE(16);
	TDS_PUT_A4LE(&mars.seq, tds->send_seq);
	tds->recv_wnd = new_recv_wnd;
	TDS_PUT_A4LE(&mars.wnd, tds->recv_wnd);

	packet = tds_alloc_packet(&mars, sizeof(mars));
	if (!packet)
		return TDS_FAIL;	/* TODO check result */
	packet->sid = tds->sid;

	tds_mutex_lock(&tds->conn->list_mtx);
	tds_append_packet(&tds->conn->send_packets, packet);
	tds_mutex_unlock(&tds->conn->list_mtx);

	return TDS_SUCCESS;
}

TDSRET
tds_append_fin(TDSSOCKET *tds)
{
	TDS72_SMP_HEADER mars;
	TDSPACKET *packet;

	if (!tds->conn->mars || tds->sid < 0)
		return TDS_SUCCESS;

	mars.signature = TDS72_SMP;
	mars.type = TDS_SMP_FIN;
	TDS_PUT_A2LE(&mars.sid, tds->sid);
	mars.size = TDS_HOST4LE(16);
	TDS_PUT_A4LE(&mars.seq, tds->send_seq);
	tds->recv_wnd = tds->recv_seq + 4;
	TDS_PUT_A4LE(&mars.wnd, tds->recv_wnd);

	packet = tds_alloc_packet(&mars, sizeof(mars));
	if (!packet)
		return TDS_FAIL;	/* TODO check result */
	packet->sid = tds->sid;

	/* we already hold lock so do not lock */
	tds_append_packet(&tds->conn->send_packets, packet);

	/* now is no more an active session */
	tds->conn->sessions[tds->sid] = BUSY_SOCKET;
	tds_set_state(tds, TDS_DEAD);
	tds->sid = -1;

	return TDS_SUCCESS;
}
#endif /* ENABLE_ODBC_MARS */


TDSRET
tds_write_packet(TDSSOCKET * tds, unsigned char final)
{
	int res;
	unsigned int left = 0;

#if TDS_ADDITIONAL_SPACE != 0
	if (tds->out_pos > tds->out_buf_max) {
		left = tds->out_pos - tds->out_buf_max;
		tds->out_pos = tds->out_buf_max;
	}
#endif

	/* we must assure server can accept our packet looking at
	 * send_wnd and waiting for proper send_wnd if send_seq > send_wnd
	 */
	tds->out_buf[0] = tds->out_flag;
	tds->out_buf[1] = final;
	TDS_PUT_A2BE(tds->out_buf+2, tds->out_pos);
	TDS_PUT_A4(tds->out_buf+4, 0);
	if (IS_TDS7_PLUS(tds->conn) && !tds->login)
		tds->out_buf[6] = 0x01;

#if ENABLE_ODBC_MARS
	res = tds_connection_put_packet(tds, tds_build_packet(tds, tds->out_buf, tds->out_pos));
#else /* !ENABLE_ODBC_MARS */
	tdsdump_dump_buf(TDS_DBG_NETWORK, "Sending packet", tds->out_buf, tds->out_pos);

	/* GW added in check for write() returning <0 and SIGPIPE checking */
	res = tds_connection_write(tds, tds->out_buf, tds->out_pos, final) <= 0 ?
		TDS_FAIL : TDS_SUCCESS;
#endif /* !ENABLE_ODBC_MARS */

#if TDS_ADDITIONAL_SPACE != 0
	memcpy(tds->out_buf + 8, tds->out_buf + tds->out_buf_max, left);
#endif
	tds->out_pos = left + 8;

	return res;
}

#if !ENABLE_ODBC_MARS
int
tds_put_cancel(TDSSOCKET * tds)
{
	unsigned char out_buf[8];
	int sent;

	out_buf[0] = TDS_CANCEL;	/* out_flag */
	out_buf[1] = 1;	/* final */
	out_buf[2] = 0;
	out_buf[3] = 8;
	TDS_PUT_A4(out_buf+4, 0);
	if (IS_TDS7_PLUS(tds->conn) && !tds->login)
		out_buf[6] = 0x01;

	tdsdump_dump_buf(TDS_DBG_NETWORK, "Sending packet", out_buf, 8);

	sent = tds_connection_write(tds, out_buf, 8, 1);

	if (sent > 0)
		tds->in_cancel = 1;

	/* GW added in check for write() returning <0 and SIGPIPE checking */
	return sent <= 0 ? TDS_FAIL : TDS_SUCCESS;
}
#endif /* !ENABLE_ODBC_MARS */


#if ENABLE_ODBC_MARS
static short
tds_packet_write(TDSCONNECTION *conn)
{
	int sent;
	TDSPACKET *packet = conn->send_packets;

	if (!packet)
		return -1;

	if (packet->pos == 0)
		tdsdump_dump_buf(TDS_DBG_NETWORK, "Sending packet", packet->buf, packet->len);

	/* final does not take into account other packets for this session */
	sent = tds_connection_write(conn->in_net_tds, packet->buf+packet->pos, packet->len-packet->pos, packet->next == NULL);

	if (sent < 0) {
		/* TODO tdserror called ?? */
		tds_connection_close(conn);
		return -1;
	}

	/* update sent data */
	packet->pos += sent;
	/* remove packet if sent all data */
	if (packet->pos >= packet->len) {
		short sid = packet->sid;
		tds_mutex_lock(&conn->list_mtx);
		conn->send_packets = packet->next;
		tds_mutex_unlock(&conn->list_mtx);
		packet->next = NULL;
		tds_free_packets(packet);
		return sid;
	}

	return -1;
}
#endif /* ENABLE_ODBC_MARS */

/** @} */
