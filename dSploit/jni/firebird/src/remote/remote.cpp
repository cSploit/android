/*
 *	PROGRAM:	JRD Remote Interface
 *	MODULE:		remote.cpp
 *	DESCRIPTION:	Common routines for remote interface/server
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include <string.h>
#include <stdlib.h>
#include "../jrd/ibase.h"
#include "../remote/remote.h"
#include "../jrd/file_params.h"
#include "../jrd/gdsassert.h"
#include "../remote/proto_proto.h"
#include "../remote/remot_proto.h"
#include "../remote/xdr_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/thread_proto.h"
#include "../common/config/config.h"
#include "../common/classes/init.h"

#ifdef DEV_BUILD
Firebird::AtomicCounter rem_port::portCounter;
#endif

#ifdef REMOTE_DEBUG
IMPLEMENT_TRACE_ROUTINE(remote_trace, "REMOTE")
#endif

const SLONG DUMMY_INTERVAL		= 60;	// seconds
const int ATTACH_FAILURE_SPACE	= 16 * 1024;	// bytes


void REMOTE_cleanup_transaction( Rtr* transaction)
{
/**************************************
 *
 *	R E M O T E _ c l e a n u p _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	A transaction is being committed or rolled back.
 *	Purge any active messages in case the user calls
 *	receive while we still have something cached.
 *
 **************************************/
	for (Rrq* request = transaction->rtr_rdb->rdb_requests; request; request = request->rrq_next)
	{
		if (request->rrq_rtr == transaction)
		{
			REMOTE_reset_request(request, 0);
			request->rrq_rtr = NULL;
		}
		for (Rrq* level = request->rrq_levels; level; level = level->rrq_next)
		{
			if (level->rrq_rtr == transaction)
			{
				REMOTE_reset_request(level, 0);
				level->rrq_rtr = NULL;
			}
		}
	}

	for (Rsr* statement = transaction->rtr_rdb->rdb_sql_requests; statement;
		 statement = statement->rsr_next)
	{
		if (statement->rsr_rtr == transaction)
		{
			REMOTE_reset_statement(statement);
			statement->rsr_flags.clear(Rsr::FETCHED);
			statement->rsr_rtr = NULL;
		}
	}
}


ULONG REMOTE_compute_batch_size(rem_port* port,
								USHORT buffer_used, P_OP op_code,
								const rem_fmt* format)
{
/**************************************
 *
 *	R E M O T E _ c o m p u t e _ b a t c h _ s i z e
 *
 **************************************
 *
 * Functional description
 *
 * When batches of records are returned, they are returned as
 *    follows:
 *     <op_fetch_response> <data_record 1>
 *     <op_fetch_response> <data_record 2>
 * 	...
 *     <op_fetch_response> <data_record n-1>
 *     <op_fetch_response> <data_record n>
 *
 * end-of-batch is indicated by setting p_sqldata_messages to
 * 0 in the op_fetch_response.  End of cursor is indicated
 * by setting p_sqldata_status to a non-zero value.  Note
 * that a fetch CAN be attempted after end of cursor, this
 * is sent to the server for the server to return the appropriate
 * error code.
 *
 * Each data block has one overhead packet
 * to indicate the data is present.
 *
 * (See also op_send in receive_msg() - which is a kissing cousin
 *  to this routine)
 *
 * Here we make a guess for the optimal number of records to
 * send in each batch.  This is important as we wait for the
 * whole batch to be received before we return the first item
 * to the client program.  How many are cached on the client also
 * impacts client-side memory utilization.
 *
 * We optimize the number by how many can fit into a packet.
 * The client calculates this number (n from the list above)
 * and sends it to the server.
 *
 * I asked why it is that the client doesn't just ask for a packet
 * full of records and let the server return however many fits in
 * a packet.  According to Sudesh, this is because of a bug in
 * Superserver which showed up in the WIN_NT 4.2.x kits.  So I
 * imagine once we up the protocol so that we can be sure we're not
 * talking to a 4.2 kit, then we can make this optimization.
 *           - Deej 2/28/97
 *
 * Note: A future optimization can look at setting the packet
 * size to optimize the transfer.
 *
 * Note: This calculation must use worst-case to determine the
 * packing.  Should the data record have VARCHAR data, it is
 * often possible to fit more than the packing specification
 * into each packet.  This is also a candidate for future
 * optimization.
 *
 * The data size is either the XDR data representation, or the
 * actual message size (rounded up) if this is a symmetric
 * architecture connection.
 *
 **************************************/

	const USHORT MAX_PACKETS_PER_BATCH	= 4;	// packets    - picked by SWAG
	const USHORT MIN_PACKETS_PER_BATCH	= 2;	// packets    - picked by SWAG
	const USHORT DESIRED_ROWS_PER_BATCH	= 20;	// data rows  - picked by SWAG
	const USHORT MIN_ROWS_PER_BATCH		= 10;	// data rows  - picked by SWAG

	const USHORT op_overhead = (USHORT) xdr_protocol_overhead(op_code);

#ifdef DEBUG
	fprintf(stderr,
			   "port_buff_size = %d fmt_net_length = %d fmt_length = %d overhead = %d\n",
			   port->port_buff_size, format->fmt_net_length,
			   format->fmt_length, op_overhead);
#endif

	ULONG row_size;
	if (port->port_flags & PORT_symmetric)
	{
		// Same architecture connection
		row_size = (ROUNDUP(format->fmt_length, 4) + op_overhead);
	}
	else
	{
		// Using XDR for data transfer
		row_size = (ROUNDUP(format->fmt_net_length, 4) + op_overhead);
	}

	USHORT num_packets = (USHORT) (((DESIRED_ROWS_PER_BATCH * row_size)	// data set
							 + buffer_used	// used in 1st pkt
							 + (port->port_buff_size - 1))	// to round up
							/ port->port_buff_size);
	if (num_packets > MAX_PACKETS_PER_BATCH)
	{
		num_packets = (USHORT) (((MIN_ROWS_PER_BATCH * row_size)	// data set
								 + buffer_used	// used in 1st pkt
								 + (port->port_buff_size - 1))	// to round up
								/ port->port_buff_size);
	}
	num_packets = MAX(num_packets, MIN_PACKETS_PER_BATCH);

	// Now that we've picked the number of packets in a batch,
	// pack as many rows as we can into the set of packets

	ULONG result = (num_packets * port->port_buff_size - buffer_used) / row_size;

	// Must always send some messages, even if message size is more
	// than packet size.

	result = MAX(result, MIN_ROWS_PER_BATCH);

#ifdef DEBUG
	{
		// CVC: I don't see the point in replacing this with fb_utils::readenv().
		const char* p = getenv("DEBUG_BATCH_SIZE");
		if (p)
			result = atoi(p);
		fprintf(stderr, "row_size = %lu num_packets = %d\n", row_size, num_packets);
		fprintf(stderr, "result = %lu\n", result);
	}
#endif

	return result;
}


Rrq* REMOTE_find_request(Rrq* request, USHORT level)
{
/**************************************
 *
 *	R E M O T E _ f i n d _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Find sub-request if level is non-zero.
 *
 **************************************/

	// See if we already know about the request level

	for (;;)
	{
		if (request->rrq_level == level)
			return request;
		if (!request->rrq_levels)
			break;
		request = request->rrq_levels;
	}

	// This is a new level -- make up a new request block.

	request->rrq_levels = request->clone();
	// FREE: REMOTE_remove_request()
#ifdef DEBUG_REMOTE_MEMORY
	printf("REMOTE_find_request       allocate request %x\n", request->rrq_levels);
#endif
	request = request->rrq_levels;
	request->rrq_level = level;
	request->rrq_levels = NULL;

	// Allocate message block for known messages

	Rrq::rrq_repeat* tail = request->rrq_rpt.begin();
	const Rrq::rrq_repeat* const end = tail + request->rrq_max_msg;
	for (; tail <= end; tail++)
	{
		const rem_fmt* format = tail->rrq_format;
		if (!format)
			continue;
		RMessage* msg = new RMessage(format->fmt_length);
		tail->rrq_xdr = msg;
#ifdef DEBUG_REMOTE_MEMORY
		printf("REMOTE_find_request       allocate message %x\n", msg);
#endif
		msg->msg_next = msg;
#ifdef SCROLLABLE_CURSORS
		msg->msg_prior = msg;
#endif
		msg->msg_number = tail->rrq_message->msg_number;
		tail->rrq_message = msg;
	}

	return request;
}


void REMOTE_free_packet( rem_port* port, PACKET * packet, bool partial)
{
/**************************************
 *
 *	R E M O T E _ f r e e _ p a c k e t
 *
 **************************************
 *
 * Functional description
 *	Zero out a full packet block (partial == false) or
 *	part of packet used in last operation (partial == true)
 **************************************/
	XDR xdr;
	USHORT n;

	if (packet)
	{
		xdrmem_create(&xdr, reinterpret_cast<char*>(packet), sizeof(PACKET), XDR_FREE);
		xdr.x_public = (caddr_t) port;

		if (partial) {
			xdr_protocol(&xdr, packet);
		}
		else
		{
			for (n = (USHORT) op_connect; n < (USHORT) op_max; n++)
			{
				packet->p_operation = (P_OP) n;
				xdr_protocol(&xdr, packet);
			}
		}
#ifdef DEBUG_XDR_MEMORY
		// All packet memory allocations should now be voided.
		// note: this code will may work properly if partial == true

		for (n = 0; n < P_MALLOC_SIZE; n++)
			fb_assert(packet->p_malloc[n].p_operation == op_void);
#endif
		packet->p_operation = op_void;
	}
}


void REMOTE_get_timeout_params(rem_port* port, Firebird::ClumpletReader* pb)
{
/**************************************
 *
 *	R E M O T E _ g e t _ t i m e o u t _ p a r a m s
 *
 **************************************
 *
 * Functional description
 *	Determine the connection timeout parameter values for this newly created
 *	port.  If the client did a specification in the DPB, use those values.
 *	Otherwise, see if there is anything in the configuration file.  The
 *	configuration file management code will set the default values if there
 *	is no other specification.
 *
 **************************************/
	//bool got_dpb_connect_timeout = false;

	fb_assert(isc_dpb_connect_timeout == isc_spb_connect_timeout);

	port->port_connect_timeout =
		pb && pb->find(isc_dpb_connect_timeout) ? pb->getInt() : Config::getConnectionTimeout();

	port->port_flags |= PORT_dummy_pckt_set;
	port->port_dummy_packet_interval = Config::getDummyPacketInterval();
	if (port->port_dummy_packet_interval < 0)
		port->port_dummy_packet_interval = DUMMY_INTERVAL;

	port->port_dummy_timeout = port->port_dummy_packet_interval;

#ifdef DEBUG
	printf("REMOTE_get_timeout dummy = %lu conn = %lu\n",
			  port->port_dummy_packet_interval, port->port_connect_timeout);
	fflush(stdout);
#endif
}


rem_str* REMOTE_make_string(const SCHAR* input)
{
/**************************************
 *
 *	R E M O T E _ m a k e _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Copy a given string to a permanent location, returning
 *	address of new string.
 *
 **************************************/
	const USHORT length = strlen(input);
	rem_str* string = FB_NEW_RPT(*getDefaultMemoryPool(), length) rem_str;
#ifdef DEBUG_REMOTE_MEMORY
	printf("REMOTE_make_string        allocate string  %x\n", string);
#endif
	strcpy(string->str_data, input);
	string->str_length = length;

	return string;
}


void REMOTE_release_messages( RMessage* messages)
{
/**************************************
 *
 *	R E M O T E _ r e l e a s e _ m e s s a g e s
 *
 **************************************
 *
 * Functional description
 *	Release a circular list of messages.
 *
 **************************************/
	RMessage* message = messages;
	if (message)
	{
		while (true)
		{
			RMessage* temp = message;
			message = message->msg_next;
#ifdef DEBUG_REMOTE_MEMORY
			printf("REMOTE_release_messages   free message     %x\n", temp);
#endif
			delete temp;
			if (message == messages)
				break;
		}
	}
}


void REMOTE_release_request( Rrq* request)
{
/**************************************
 *
 *	R E M O T E _ r e l e a s e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Release a request block and friends.
 *
 **************************************/
	Rdb* rdb = request->rrq_rdb;

	for (Rrq** p = &rdb->rdb_requests; *p; p = &(*p)->rrq_next)
	{
		if (*p == request)
		{
			*p = request->rrq_next;
			break;
		}
	}

	// Get rid of request and all levels

	for (;;)
	{
		Rrq::rrq_repeat* tail = request->rrq_rpt.begin();
		const Rrq::rrq_repeat* const end = tail + request->rrq_max_msg;
		for (; tail <= end; tail++)
		{
		    RMessage* message = tail->rrq_message;
			if (message)
			{
				if (!request->rrq_level)
				{
#ifdef DEBUG_REMOTE_MEMORY
					printf("REMOTE_release_request    free format      %x\n", tail->rrq_format);
#endif
					delete tail->rrq_format;
				}
				REMOTE_release_messages(message);
			}
		}
		Rrq* next = request->rrq_levels;
#ifdef DEBUG_REMOTE_MEMORY
		printf("REMOTE_release_request    free request     %x\n", request);
#endif
		delete request;
		if (!(request = next))
			break;
	}
}


void REMOTE_reset_request( Rrq* request, RMessage* active_message)
{
/**************************************
 *
 *	R E M O T E _ r e s e t _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Clean up a request in preparation to use it again.  Since
 *	there may be an active message (start_and_send), exercise
 *	some care to avoid zapping that message.
 *
 **************************************/
	Rrq::rrq_repeat* tail = request->rrq_rpt.begin();
	const Rrq::rrq_repeat* const end = tail + request->rrq_max_msg;
	for (; tail <= end; tail++)
	{
	    RMessage* message = tail->rrq_message;
		if (message != NULL && message != active_message)
		{
			tail->rrq_xdr = message;
			tail->rrq_rows_pending = 0;
			tail->rrq_reorder_level = 0;
			tail->rrq_batch_count = 0;
			while (true)
			{
				message->msg_address = NULL;
				message = message->msg_next;
				if (message == tail->rrq_message)
					break;
			}
		}
	}

	// Initialize the request status to FB_SUCCESS

	request->rrq_status_vector[1] = 0;
}


void REMOTE_reset_statement( Rsr* statement)
{
/**************************************
 *
 *	R E M O T E _ r e s e t _ s t a t e m e n t
 *
 **************************************
 *
 * Functional description
 *	Reset a statement by releasing all buffers except 1
 *
 **************************************/
	RMessage* message;

	if (!statement || (!(message = statement->rsr_message)))
		return;

	// Reset all the pipeline counters

	statement->rsr_rows_pending = 0;
	statement->rsr_msgs_waiting = 0;
	statement->rsr_reorder_level = 0;
	statement->rsr_batch_count = 0;

	// only one entry

	if (message->msg_next == message)
		return;

	// find the entry before statement->rsr_message

	RMessage* temp = message->msg_next;
	while (temp->msg_next != message)
		temp = temp->msg_next;

	temp->msg_next = message->msg_next;
	message->msg_next = message;
#ifdef SCROLLABLE_CURSORS
	message->msg_prior = message;
#endif

	statement->rsr_buffer = statement->rsr_message;

	REMOTE_release_messages(temp);
}


void REMOTE_save_status_strings( ISC_STATUS* vector)
{
/**************************************
 *
 *	R E M O T E _ s a v e _ s t a t u s _ s t r i n g s
 *
 **************************************
 *
 * Functional description
 *	There has been a failure during attach/create database.
 *	The included string have been allocated off of the database block,
 *	which is going to be released before the message gets passed
 *	back to the user.  So, to preserve information, copy any included
 *	strings to a special buffer.
 *
 **************************************/
	Firebird::makePermanentVector(vector);
}


// TMN: Beginning of C++ port - ugly but a start

void rem_port::linkParent(rem_port* const parent)
{
	fb_assert(parent);
	fb_assert(this->port_parent == NULL);

	this->port_parent = parent;
	this->port_next = parent->port_clients;
	this->port_server = parent->port_server;
	this->port_server_flags = parent->port_server_flags;

	parent->port_clients = parent->port_next = this;
}

void rem_port::unlinkParent()
{
	if (this->port_parent == NULL)
		return;

#ifdef DEV_BUILD
	bool found = false;
#endif

	for (rem_port** ptr = &this->port_parent->port_clients; *ptr; ptr = &(*ptr)->port_next)
	{
		if (*ptr == this)
		{
			*ptr = this->port_next;

			if (ptr == &this->port_parent->port_clients)
			{
				fb_assert(this->port_parent->port_next == this);

				this->port_parent->port_next = *ptr;
			}

#ifdef DEV_BUILD
			found = true;
#endif
			break;
		}
	} // for

	fb_assert(found);

	this->port_parent = NULL;
}

bool rem_port::accept(p_cnct* cnct)
{
	return (*this->port_accept)(this, cnct);
}

void rem_port::disconnect()
{
	(*this->port_disconnect)(this);
}

void rem_port::force_close()
{
	(*this->port_force_close)(this);
}

rem_port* rem_port::receive(PACKET* pckt)
{
	return (*this->port_receive_packet)(this, pckt);
}

bool rem_port::select_multi(UCHAR* buffer, SSHORT bufsize, SSHORT* length, RemPortPtr& port)
{
	return (*this->port_select_multi)(this, buffer, bufsize, length, port);
}

XDR_INT rem_port::send(PACKET* pckt)
{
	return (*this->port_send_packet)(this, pckt);
}

XDR_INT rem_port::send_partial(PACKET* pckt)
{
	return (*this->port_send_partial)(this, pckt);
}

rem_port* rem_port::connect(PACKET* pckt)
{
	return (*this->port_connect)(this, pckt);
}

rem_port* rem_port::request(PACKET* pckt)
{
	return (*this->port_request)(this, pckt);
}

#ifdef REM_SERVER
bool_t REMOTE_getbytes (XDR* xdrs, SCHAR* buff, u_int count)
{
/**************************************
 *
 *	R E M O T E  _ g e t b y t e s
 *
 **************************************
 *
 * Functional description
 *	Get a bunch of bytes from a port buffer
 *
 **************************************/
	SLONG bytecount = count;

	// Use memcpy to optimize bulk transfers.

	while (bytecount > 0)
	{
		if (xdrs->x_handy >= bytecount)
		{
			memcpy(buff, xdrs->x_private, bytecount);
			xdrs->x_private += bytecount;
			xdrs->x_handy -= bytecount;
			break;
		}

		if (xdrs->x_handy > 0)
		{
			memcpy(buff, xdrs->x_private, xdrs->x_handy);
			xdrs->x_private += xdrs->x_handy;
			buff += xdrs->x_handy;
			bytecount -= xdrs->x_handy;
			xdrs->x_handy = 0;
		}
		rem_port* port = (rem_port*) xdrs->x_public;
		Firebird::RefMutexGuard queGuard(*port->port_que_sync);
		if (port->port_qoffset >= port->port_queue.getCount())
		{
			port->port_flags |= PORT_partial_data;
			return FALSE;
		}

		xdrs->x_handy = port->port_queue[port->port_qoffset].getCount();
		fb_assert(xdrs->x_handy <= port->port_buff_size);
		memcpy(xdrs->x_base, port->port_queue[port->port_qoffset].begin(), xdrs->x_handy);
		++port->port_qoffset;
		xdrs->x_private = xdrs->x_base;
	}

	return TRUE;
}
#endif //REM_SERVER

#ifdef TRUSTED_AUTH
ServerAuth::ServerAuth(const char* fName, int fLen, const Firebird::ClumpletWriter& pb,
					   ServerAuth::Part2* p2, P_OP op)
	: fileName(*getDefaultMemoryPool()), clumplet(*getDefaultMemoryPool()),
	  part2(p2), operation(op)
{
	fileName.assign(fName, fLen);
	size_t pbLen = pb.getBufferLength();
	if (pbLen)
	{
		memcpy(clumplet.getBuffer(pbLen), pb.getBuffer(), pbLen);
	}
	authSspi = FB_NEW(*getDefaultMemoryPool()) AuthSspi;
}

ServerAuth::~ServerAuth()
{
	delete authSspi;
}
#endif // TRUSTED_AUTH

void PortsCleanup::registerPort(rem_port* port)
{
	Firebird::MutexLockGuard guard(m_mutex);
	if (!m_ports)
	{
		Firebird::MemoryPool& pool = *getDefaultMemoryPool();
		m_ports = FB_NEW (pool) PortsArray(pool);
	}

	m_ports->add(port);
}

void PortsCleanup::unRegisterPort(rem_port* port)
{
	Firebird::MutexLockGuard guard(m_mutex);

	if (m_ports)
	{
		size_t i;
		const bool found = m_ports->find(port, i);
		//fb_assert(found);
		if (found)
			m_ports->remove(i);
	}
}

void PortsCleanup::closePorts()
{
	Firebird::MutexLockGuard guard(m_mutex);

	if (m_ports)
	{
		rem_port* const* ptr = m_ports->begin();
		const rem_port* const* end = m_ports->end();
		for (; ptr < end; ptr++) {
			(*ptr)->force_close();
		}

		delete m_ports;
		m_ports = NULL;
	}
}

rem_port::~rem_port()
{
	if (port_events_shutdown)
	{
		port_events_shutdown(this);
	}

	delete port_version;
	delete port_connection;
	delete port_user_name;
	delete port_host;
	delete port_protocol_str;
	delete port_address_str;

#ifdef DEBUG_XDR_MEMORY
	delete port_packet_vector;
#endif

#ifdef TRUSTED_AUTH
	delete port_trusted_auth;
#endif

#ifdef DEV_BUILD
	--portCounter;
#endif
}

void Rdb::set_async_vector(ISC_STATUS* userStatus) throw()
{
	rdb_async_status_vector = userStatus;
	rdb_async_thread_id = getThreadId();
}

void Rdb::reset_async_vector() throw()
{
	rdb_async_thread_id = 0;
	rdb_async_status_vector = NULL;
}

ISC_STATUS* Rdb::get_status_vector() throw()
{
	return rdb_async_thread_id == getThreadId() ? rdb_async_status_vector : rdb_status_vector;
}
