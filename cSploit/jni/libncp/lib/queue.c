/*
    queue.c - Queue handling
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
						Initial release.

	0.01  1999, June 1		Petr Vandrovec <vandrove@vc.cvut.cz>
						Splitted from ncplib.c. See ncplib.c
						for other contributions.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
						Added license.
 */

#include "ncplib_i.h"

#include <string.h>
#include <errno.h>

#ifndef ENOPKG
#define ENOPKG	ENOSYS
#endif

/* Here the ncp calls begin
 */

static inline void ncp_queue_job_2_out(struct nw_queue_job_entry *j) {
	j->JobNumber = DVAL_LH(&j->JobNumber, 0);
}

static inline void ncp_add_queue_job(struct ncp_conn *conn, const struct nw_queue_job_entry *j) {
	memcpy(conn->current_point, j, sizeof(*j));
	ncp_queue_job_2_out((struct nw_queue_job_entry*)conn->current_point);
	conn->current_point += sizeof(*j);
}

static void ncp_reply_queue_entry(struct nw_queue_job_entry *j, const void * reply, size_t replys) {
	memcpy(j, reply, replys);
	if (replys < sizeof(*j)) {
		unsigned char * v = (unsigned char*)j;
		
		memset(v + replys, 0, sizeof(*j) - replys);
	}
	ncp_queue_job_2_out(j);
}

static inline void ncp_reply_queue_job(struct queue_job *job, const void * reply, size_t replys) {
	ncp_reply_queue_entry(&job->j, reply, replys);
	ConvertToNWfromDWORD(job->j.JobFileHandle, job->file_handle);
}

/* Create a new job entry */
long
ncp_create_queue_job_and_file(struct ncp_conn *conn,
			      u_int32_t queue_id,
			      struct queue_job *job)
{
	long result;

	ncp_init_request_s(conn, 121);
	ncp_add_dword_hl(conn, queue_id);
	ncp_add_queue_job(conn, &job->j);

	if ((result = ncp_request(conn, 23)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	ncp_reply_queue_job(job, ncp_reply_data(conn, 0), 78);
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_close_file_and_start_job(struct ncp_conn *conn,
			     u_int32_t queue_id,
			     const struct queue_job *job)
{
	long result;

	ncp_init_request_s(conn, 127);
	ncp_add_dword_hl(conn, queue_id);
	ncp_add_dword_lh(conn, job->j.JobNumber);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_attach_to_queue(struct ncp_conn *conn,
		    u_int32_t queue_id)
{
	long result;

	ncp_init_request_s(conn, 111);
	ncp_add_dword_hl(conn, queue_id);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_detach_from_queue(struct ncp_conn *conn,
		      u_int32_t queue_id)
{
	long result;

	ncp_init_request_s(conn, 112);
	ncp_add_dword_hl(conn, queue_id);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_service_queue_job(struct ncp_conn *conn, u_int32_t queue_id, u_int16_t job_type,
		      struct queue_job *job)
{
	long result;

	ncp_init_request_s(conn, 124);
	ncp_add_dword_hl(conn, queue_id);
	ncp_add_word_hl(conn, job_type);

	if ((result = ncp_request(conn, 23)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	ncp_reply_queue_job(job, ncp_reply_data(conn, 0), 78);
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_finish_servicing_job(struct ncp_conn *conn, u_int32_t queue_id,
			 u_int32_t job_number, u_int32_t charge_info)
{
	long result;

	ncp_init_request_s(conn, 131);
	ncp_add_dword_hl(conn, queue_id);
	ncp_add_dword_lh(conn, job_number);
	ncp_add_dword_hl(conn, charge_info);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_change_job_position(struct ncp_conn *conn, NWObjectID queue_id,
			 u_int32_t job_number, unsigned int position)
{
	long result;

	if (position > 255) {
		position = 255;
	}
	ncp_init_request_s(conn, 110);
	ncp_add_dword_hl(conn, queue_id);
	ncp_add_word_lh(conn, job_number);
	ncp_add_byte(conn, position);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_abort_servicing_job(struct ncp_conn *conn, u_int32_t queue_id,
			u_int32_t job_number)
{
	long result;

	ncp_init_request_s(conn, 132);
	ncp_add_dword_hl(conn, queue_id);
	ncp_add_dword_lh(conn, job_number);

	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_get_queue_length(struct ncp_conn *conn,
		     u_int32_t queue_id,
		     u_int32_t *queue_length)
{
	long result=-EINVAL;

	ncp_init_request_s(conn, 125);
	ncp_add_dword_hl(conn, queue_id);

	if ((result = ncp_request(conn, 23)) != 0)
		goto out;

	if (conn->ncp_reply_size < 12)
	{
		result=NWE_INVALID_NCP_PACKET_LENGTH;
		goto out;
	}

	if (ncp_reply_dword_hl(conn,0) != queue_id)
	{
		result=-EINVAL;
	}
	else
		*queue_length = ncp_reply_dword_lh(conn,8);

out:
	ncp_unlock_conn(conn);
	return result;
}

long 
ncp_get_queue_job_ids(struct ncp_conn *conn,
		      u_int32_t queue_id,
		      u_int32_t queue_section,
		      u_int32_t *length1,
		      u_int32_t *length2,
		      u_int32_t ids[])
{
	long result;

	ncp_init_request_s(conn,129);
	ncp_add_dword_hl(conn, queue_id);
	ncp_add_dword_lh(conn, queue_section);

	if ((result = ncp_request(conn, 23)) != 0)
		goto out;

	if (conn->ncp_reply_size < 8)
	{
		result=NWE_INVALID_NCP_PACKET_LENGTH;
		goto out;
	}

	*length2 = ncp_reply_dword_lh(conn,4);
	if (conn->ncp_reply_size < 8 + 4*(*length2)) {
		result=NWE_INVALID_NCP_PACKET_LENGTH;
		goto out;
	}
	if (ids) {
		int count = min(*length1, *length2)*sizeof(u_int32_t);
		int pos;

		for (pos=0; pos<count; pos+=sizeof(u_int32_t)) {
			*ids++ = ncp_reply_dword_lh(conn, 8+pos);
		}
	}
	*length1 = ncp_reply_dword_lh(conn,0);

out:
	ncp_unlock_conn(conn);
	return result;
}

long 
ncp_get_queue_job_info(struct ncp_conn *conn,
		       u_int32_t queue_id,
		       u_int32_t job_id,
		       struct nw_queue_job_entry *jobdata)
{
	long result;

	ncp_init_request_s(conn,122);
	ncp_add_dword_hl(conn, queue_id);
	ncp_add_dword_lh(conn, job_id);

	if ((result = ncp_request(conn, 23)) != 0)
		goto out;

	if (conn->ncp_reply_size < sizeof(struct nw_queue_job_entry)) {
		result=NWE_INVALID_NCP_PACKET_LENGTH;
	} else {
		ncp_reply_queue_entry(jobdata, ncp_reply_data(conn,0), sizeof(struct nw_queue_job_entry));
	}

out:
	ncp_unlock_conn(conn);
	return result;
}

long
NWRemoveJobFromQueue2
(
	struct ncp_conn*	conn,
	u_int32_t			queueID,
	u_int32_t			jobNumber
) {
	long result;

	ncp_init_request_s(conn, 0x80);
	ncp_add_dword_hl(conn, queueID);
	ncp_add_dword_lh(conn, jobNumber);
	result = ncp_request(conn, 0x17);
	ncp_unlock_conn(conn);
	return result;
}

NWCCODE
NWChangeQueueJobEntry(NWCONN_HANDLE conn,
		      NWObjectID queue_id,
		      const struct nw_queue_job_entry *jobdata)
{
	NWCCODE result;

	ncp_init_request_s(conn, 123);
	ncp_add_dword_hl(conn, queue_id);
	ncp_add_queue_job(conn, jobdata);
	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}
