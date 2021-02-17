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
#include "../common/file_params.h"
#include "../common/gdsassert.h"
#include "../remote/proto_proto.h"
#include "../remote/remot_proto.h"
#include "../common/xdr_proto.h"
#include "../yvalve/gds_proto.h"
#include "../jrd/thread_proto.h"
#include "../common/config/config.h"
#include "../common/classes/init.h"
#include "../common/db_alias.h"
#include "firebird/Provider.h"

#ifdef DEV_BUILD
Firebird::AtomicCounter rem_port::portCounter;
#endif

#ifdef REMOTE_DEBUG
IMPLEMENT_TRACE_ROUTINE(remote_trace, "REMOTE")
#endif


const ParametersSet dpbParam =
{
	isc_dpb_dummy_packet_interval,
	isc_dpb_user_name,
	isc_dpb_auth_block,
	isc_dpb_password,
	isc_dpb_password_enc,
	isc_dpb_trusted_auth,
	isc_dpb_auth_plugin_name,
	isc_dpb_auth_plugin_list,
	isc_dpb_specific_auth_data,
	isc_dpb_address_path,
	isc_dpb_process_id,
	isc_dpb_process_name,
	isc_dpb_encrypt_key,
	isc_dpb_client_version,
	isc_dpb_remote_protocol,
	isc_dpb_host_name,
	isc_dpb_os_user,
	isc_dpb_config
};

const ParametersSet spbParam =
{
	isc_spb_dummy_packet_interval,
	isc_spb_user_name,
	isc_spb_auth_block,
	isc_spb_password,
	isc_spb_password_enc,
	isc_spb_trusted_auth,
	isc_spb_auth_plugin_name,
	isc_spb_auth_plugin_list,
	isc_spb_specific_auth_data,
	isc_spb_address_path,
	isc_spb_process_id,
	isc_spb_process_name,
	0,
	isc_spb_client_version,
	isc_spb_remote_protocol,
	isc_spb_host_name,
	isc_spb_os_user,
	isc_spb_config
};

const ParametersSet connectParam =
{
	0,
	CNCT_login,
	0,
	0,
	0,
	0,
	CNCT_plugin_name,
	CNCT_plugin_list,
	CNCT_specific_data,
	0,
	0,
	0,
	0,
	0,
	0,
	CNCT_host,
	CNCT_user,
	0
};


const SLONG DUMMY_INTERVAL		= 60;	// seconds
const int ATTACH_FAILURE_SPACE	= 16 * 1024;	// bytes

static Firebird::MakeUpgradeInfo<> upInfo;


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


USHORT REMOTE_compute_batch_size(rem_port* port,
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

	fb_assert(result <= MAX_USHORT);
	return static_cast<USHORT>(result);
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
		xdr.x_local = (port->port_type == rem_port::XNET);
#ifdef DEV_BUILD
		xdr.x_client = false;
#endif

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

	port->port_connect_timeout = pb && pb->find(isc_dpb_connect_timeout) ?
		pb->getInt() : port->getPortConfig()->getConnectionTimeout();

	port->port_flags |= PORT_dummy_pckt_set;
	port->port_dummy_packet_interval = port->getPortConfig()->getDummyPacketInterval();
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
	const USHORT length = static_cast<USHORT>(strlen(input));
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

	//request->rrq_status_vector[1] = 0;
	request->rrqStatus.clear();
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
	this->port_config = parent->port_config;

	parent->port_clients = parent->port_next = this;
}

const Firebird::RefPtr<Config>& rem_port::getPortConfig() const
{
	return port_config.hasData() ? port_config : Config::getDefaultConfig();
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

void rem_port::abort_aux_connection()
{
	if (this->port_abort_aux_connection)
	{
		(*this->port_abort_aux_connection)(this);
	}
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

void rem_port::auxAcceptError(PACKET* packet)
{
	if (port_protocol >= PROTOCOL_VERSION13)
	{
		packet->p_operation = op_abort_aux_connection;
		// Ignore error return - we are already processing auxiliary connection error from the wire
		send(packet);
	}
}

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
		Firebird::RefMutexGuard queGuard(*port->port_que_sync, FB_FUNCTION);
		if (port->port_qoffset >= port->port_queue.getCount())
		{
			port->port_flags |= PORT_partial_data;
			return FALSE;
		}

		xdrs->x_handy = (int) port->port_queue[port->port_qoffset].getCount();
		fb_assert(xdrs->x_handy <= port->port_buff_size);
		memcpy(xdrs->x_base, port->port_queue[port->port_qoffset].begin(), xdrs->x_handy);
		++port->port_qoffset;
		xdrs->x_private = xdrs->x_base;
	}

	return TRUE;
}

void PortsCleanup::registerPort(rem_port* port)
{
	Firebird::MutexLockGuard guard(m_mutex, FB_FUNCTION);
	if (!m_ports)
	{
		Firebird::MemoryPool& pool = *getDefaultMemoryPool();
		m_ports = FB_NEW (pool) PortsArray(pool);
	}

	m_ports->add(port);
}

void PortsCleanup::unRegisterPort(rem_port* port)
{
	Firebird::MutexLockGuard guard(m_mutex, FB_FUNCTION);

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
	Firebird::MutexLockGuard guard(m_mutex, FB_FUNCTION);

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

ServerAuthBase::~ServerAuthBase()
{
}

ServerCallbackBase::~ServerCallbackBase()
{
}

rem_port::~rem_port()
{
	if (port_events_shutdown)
	{
		port_events_shutdown(this);
	}

	delete port_srv_auth;
	delete port_srv_auth_block;
	delete port_version;
	delete port_connection;
	delete port_host;
	delete port_server_crypt_callback;

#ifdef DEBUG_XDR_MEMORY
	delete port_packet_vector;
#endif

	while (port_crypt_keys.hasData())
	{
		delete port_crypt_keys.pop();
	}

	if (port_crypt_plugin)
		Firebird::PluginManagerInterfacePtr()->releasePlugin(port_crypt_plugin);

#ifdef DEV_BUILD
	--portCounter;
#endif
}

/*
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
*/

Rrq::~Rrq()
{
}

void Rrq::saveStatus(const Firebird::Exception& ex) throw()
{
	if (rrqStatus.isSuccess())
	{
		ISC_STATUS_ARRAY tmp;
		ex.stuff_exception(tmp);
		rrqStatus.save(tmp);
	}
}

void Rrq::saveStatus(const Firebird::IStatus* v) throw()
{
	if (rrqStatus.isSuccess())
	{
		rrqStatus.save(v->get());
	}
}

void Rsr::saveException(const Firebird::Exception& ex, bool overwrite)
{
	if (!rsr_status) {
		rsr_status = new Firebird::StatusHolder();
	}

	if (overwrite || !rsr_status->getError())
	{
		ISC_STATUS_ARRAY temp;
		ex.stuff_exception(temp);
		rsr_status->save(temp);
	}
}

Firebird::string rem_port::getRemoteId() const
{
	fb_assert(port_protocol_id.hasData());
	Firebird::string id = port_protocol_id;

	if (port_address.hasData())
		id += Firebird::string("/") + port_address;

	return id;
}

bool REMOTE_legacy_auth(const char* nm, int p)
{
	const char* legacyTrusted = "WIN_SSPI";
	if (fb_utils::stricmp(legacyTrusted, nm) == 0 &&
		(p == PROTOCOL_VERSION11 || p == PROTOCOL_VERSION12))
	{
		return true;
	}

	const char* legacyAuth = "LEGACY_AUTH";
	if (fb_utils::stricmp(legacyAuth, nm) == 0 && p < PROTOCOL_VERSION13)
	{
		return true;
	}

	return false;
}

Firebird::PathName ClntAuthBlock::getPluginName()
{
	return plugins.hasData() ? plugins.name() : "";
}

template <typename T>
static void addMutliPartConnectParameter(const T& dataToAdd,
	Firebird::ClumpletWriter& user_id, UCHAR param)
{
	size_t remaining = dataToAdd.getCount();
	fb_assert(remaining <= 254u * 256u); // paranoid check => 65024
	UCHAR part = 0;
	UCHAR buffer[255];
	typename T::const_pointer ptr = dataToAdd.begin();

	while (remaining > 0)
	{
		size_t step = remaining;
		if (step > 254)
			step = 254;

		remaining -= step;
		buffer[0] = part++;
		fb_assert(part || remaining == 0);
		memcpy(&buffer[1], ptr, step);
		ptr += step;

		user_id.insertBytes(param, buffer, step + 1);
		if (!part) // we completed 256 loops, almost impossible but check anyway.
			break;
	}
}

void ClntAuthBlock::extractDataFromPluginTo(Firebird::ClumpletWriter& user_id)
{
	// Add user login name
	if (userName.hasData())
	{
		HANDSHAKE_DEBUG(fprintf(stderr, "Cli: extractDataFromPluginTo: userName=%s\n", userName.c_str()));
		user_id.insertString(CNCT_login, userName);
	}

	// Add plugin name
	Firebird::PathName pluginName = getPluginName();
	if (pluginName.hasData())
	{
		HANDSHAKE_DEBUG(fprintf(stderr, "Cli: extractDataFromPluginTo: pluginName=%s\n", pluginName.c_str()));
		user_id.insertPath(CNCT_plugin_name, pluginName);
	}

	// Add plugin list
	if (pluginList.hasData())
	{
		user_id.insertPath(CNCT_plugin_list, pluginList);
	}

	// This is specially tricky field - user_id is limited to 255 bytes per entry,
	// and we have no ways to override this limit cause it can be sent to any version server.
	// Therefore divide data into 254-byte parts, leaving first byte for the number of that part.
	// This appears more reliable than put them in strict order.
	addMutliPartConnectParameter(dataFromPlugin, user_id, CNCT_specific_data);

	// Client's wirecrypt requested level
	user_id.insertInt(CNCT_client_crypt, config->getWireCrypt(WC_CLIENT));
}

void ClntAuthBlock::resetClnt(const Firebird::PathName* fileName, const CSTRING* listStr)
{
	if (listStr)
	{
		if (dataForPlugin.hasData())
		{
			// We should not change plugins iterator now
			return;
		}

		Firebird::ClumpletReader srvList(Firebird::ClumpletReader::UnTagged,
										 listStr->cstr_address, listStr->cstr_length);

		if (srvList.find(TAG_KNOWN_PLUGINS))
		{
			srvList.getPath(serverPluginList);
		}
	}

	dataForPlugin.clear();
	dataFromPlugin.clear();
	firstTime = true;

	config = REMOTE_get_config(fileName, &dpbConfig);
	pluginList = config->getPlugins(Firebird::PluginType::AuthClient);

	Firebird::PathName final;
	if (serverPluginList.hasData())
	{
		Remote::ParsedList onClient, fromServer, merged;
		REMOTE_parseList(onClient, pluginList);
		REMOTE_parseList(fromServer, serverPluginList);

		for (unsigned c = 0; c < onClient.getCount(); ++c)
		{
			// do not expect too long lists, therefore use double loop
			for (unsigned s = 0; s < fromServer.getCount(); ++s)
			{
				if (onClient[c] == fromServer[s])
				{
					merged.push(onClient[c]);
				}
			}
		}

		if (merged.getCount() == 0)
		{
			HANDSHAKE_DEBUG(fprintf(stderr, "Cli: No matching plugins on client\n"));
			(Firebird::Arg::Gds(isc_login)
#ifdef DEV_BUILD
								<< Firebird::Arg::Gds(isc_random) << "No matching plugins on client"
#endif
								).raise();
		}

		REMOTE_makeList(final, merged);
	}
	else
	{
		final = pluginList;
	}

	plugins.set(final.c_str());
}

Firebird::RefPtr<Config>* ClntAuthBlock::getConfig()
{
	return config.hasData() ? &config : NULL;
}

void ClntAuthBlock::storeDataForPlugin(unsigned int length, const unsigned char* data)
{
	dataForPlugin.assign(data, length);
	HANDSHAKE_DEBUG(fprintf(stderr, "Cli: accepted data for plugin length=%d\n", length));
}

Firebird::RefPtr<Config> REMOTE_get_config(const Firebird::PathName* dbName,
	const Firebird::string* dpb_config)
{
	Firebird::RefPtr<Config> config;

	if (dbName && dbName->hasData())
	{
		Firebird::PathName dummy;
		expandDatabaseName(*dbName, dummy, &config);
	}
	else
		config = Config::getDefaultConfig();

	Config::merge(config, dpb_config);

	return config;
}

void REMOTE_parseList(Remote::ParsedList& parsed, Firebird::PathName list)
{
	list.alltrim(" \t");
	parsed.clear();
	const char* sep = " \t,;";

	for(;;)
	{
		Firebird::PathName::size_type p = list.find_first_of(sep);
		if (p == Firebird::PathName::npos)
		{
			if (list.hasData())
			{
				parsed.push(list);
			}
			break;
		}

		parsed.push(list.substr(0, p));
		list = list.substr(p + 1);
		list.ltrim(" \t,;");
	}
}

void REMOTE_makeList(Firebird::PathName& list, const Remote::ParsedList& parsed)
{
	fb_assert(parsed.hasData());
	//list.erase();
	list = parsed[0];
	for (unsigned i = 1; i < parsed.getCount(); ++i)
	{
		list += ' ';
		list += parsed[i];
	}
}

void REMOTE_check_response(Firebird::IStatus* warning, Rdb* rdb, PACKET* packet, bool checkKeys)
{
/**************************************
 *
 *	R E M O T E _ c h e c k _ r e s p o n s e
 *
 **************************************
 *
 * Functional description
 *	Check response to a remote call.
 *
 **************************************/

	rdb->rdb_port->checkResponse(warning, packet, checkKeys);
}

void rem_port::checkResponse(Firebird::IStatus* warning, PACKET* packet, bool checkKeys)
{
/**************************************
 *
 *	R E M O T E _ c h e c k _ r e s p o n s e
 *
 **************************************
 *
 * Functional description
 *	Check response to a remote call.
 *
 **************************************/

	// Get status vector

	const ISC_STATUS success_vector[] = {isc_arg_gds, FB_SUCCESS, isc_arg_end};
	const ISC_STATUS *vector = success_vector;
	if (packet->p_resp.p_resp_status_vector)
	{
		vector = packet->p_resp.p_resp_status_vector->value();
	}

	// Translate any gds codes into local operating specific codes

	Firebird::SimpleStatusVector newVector;

	while (*vector != isc_arg_end)
	{
		const ISC_STATUS vec = *vector++;
		newVector.push(vec);

		switch ((USHORT) vec)
		{
		case isc_arg_warning:
		case isc_arg_gds:
			newVector.push(*vector++);
			break;

		case isc_arg_cstring:
			newVector.push(*vector++);
			// fall down

		default:
			newVector.push(*vector++);
			break;
		}
	}

	newVector.push(isc_arg_end);
	vector = newVector.begin();

	const ISC_STATUS pktErr = vector[1];
	if (pktErr == isc_shutdown || pktErr == isc_att_shutdown)
	{
		port_flags |= PORT_rdb_shutdown;
	}
	else if (checkKeys)
	{
		addServerKeys(&packet->p_resp.p_resp_data);
	}

	if ((packet->p_operation == op_response || packet->p_operation == op_response_piggyback) &&
		!vector[1])
	{
		warning->set(vector);
		return;
	}

	if (!vector[1])
	{
		Firebird::Arg::Gds(isc_net_read_err).raise();
	}

	Firebird::status_exception::raise(vector);
}

static void setCStr(CSTRING& to, const char* from)
{
	to.cstr_address = reinterpret_cast<UCHAR*>(const_cast<char*>(from));
	to.cstr_length = (ULONG) strlen(from);
	to.cstr_allocated = 0;
}

void rem_port::addServerKeys(CSTRING* passedStr)
{
	Firebird::ClumpletReader newKeys(Firebird::ClumpletReader::UnTagged,
									 passedStr->cstr_address, passedStr->cstr_length);

	for (newKeys.rewind(); !newKeys.isEof(); newKeys.moveNext())
	{
		if (newKeys.getClumpTag() == TAG_KNOWN_PLUGINS)
		{
			continue;
		}

		KnownServerKey key;
		fb_assert(newKeys.getClumpTag() == TAG_KEY_TYPE);
		newKeys.getPath(key.type);
		newKeys.moveNext();
		if (newKeys.isEof())
		{
			break;
		}
		fb_assert(newKeys.getClumpTag() == TAG_KEY_PLUGINS);
		newKeys.getPath(key.plugins);
		key.plugins += ' ';
		key.plugins.insert(0, " ");

		port_known_server_keys.add(key);
	}
}

bool rem_port::tryNewKey(InternalCryptKey* cryptKey)
{
	for (unsigned t = 0; t < port_known_server_keys.getCount(); ++t)
	{
		if (tryKeyType(port_known_server_keys[t], cryptKey))
		{
			return true;
		}
	}

	port_crypt_keys.push(cryptKey);
	return false;
}

bool rem_port::tryKeyType(const KnownServerKey& srvKey, InternalCryptKey* cryptKey)
{
	if (port_crypt_complete)
	{
		return true;
	}

	if (srvKey.type != cryptKey->type)
	{
		return false;
	}

	if (getPortConfig()->getWireCrypt(WC_CLIENT) == WIRE_CRYPT_DISABLED)
	{
		port_crypt_complete = true;
		return true;
	}

	// we got correct key's type pair
	// check what about crypt plugin for it
	Remote::ParsedList clientPlugins;
	REMOTE_parseList(clientPlugins, getPortConfig()->getPlugins(Firebird::PluginType::WireCrypt));
	for (unsigned n = 0; n < clientPlugins.getCount(); ++n)
	{
		Firebird::PathName p(clientPlugins[n]);
		if (srvKey.plugins.find(" " + p + " ") != Firebird::PathName::npos)
		{
			Firebird::GetPlugins<Firebird::IWireCryptPlugin>
				cp(Firebird::PluginType::WireCrypt, FB_WIRECRYPT_PLUGIN_VERSION, upInfo, p.c_str());
			if (cp.hasData())
			{
				Firebird::LocalStatus st;

				// Looks like we've found correct crypt plugin and key for it
				port_crypt_plugin = cp.plugin();
				port_crypt_plugin->addRef();

				// Pass key to plugin
				port_crypt_plugin->setKey(&st, cryptKey);
				if (!st.isSuccess())
				{
					Firebird::status_exception::raise(st.get());
				}

				// Now it's time to notify server about choice done
				// Notice - port_crypt_complete flag is not set still,
				// therefore sent packet will be not encrypted
				PACKET crypt;
				crypt.p_operation = op_crypt;
				setCStr(crypt.p_crypt.p_key, cryptKey->type);
				setCStr(crypt.p_crypt.p_plugin, p.c_str());
				send(&crypt);

				// Validate answer - decryptor is not affected by port_crypt_complete,
				// therefore OK to do
				receive(&crypt);
				checkResponse(&st, &crypt);

				// Complete port-crypt init
				port_crypt_complete = true;
				return true;
			}
		}
	}

	return false;
}

const char* SrvAuthBlock::getLogin()
{
	return userName.nullStr();
}

const unsigned char* SrvAuthBlock::getData(unsigned int* length)
{
	*length = (ULONG) dataForPlugin.getCount();

	if (*length && pluginName != plugins->name())
		*length = 0;

	return *length ? dataForPlugin.begin() : NULL;
}

void SrvAuthBlock::putData(Firebird::IStatus* status, unsigned int length, const void* data)
{
	status->init();
	try
	{
		memcpy(dataFromPlugin.getBuffer(length), data, length);
	}
	catch (const Firebird::Exception& ex)
	{
		ex.stuffException(status);
	}
}

void SrvAuthBlock::putKey(Firebird::IStatus* status, Firebird::FbCryptKey* cryptKey)
{
	status->init();
	try
	{
		const char* t = cryptKey->type;
		if (!t)
		{
			fb_assert(pluginName.hasData());
			t = pluginName.c_str();
		}

		InternalCryptKey* k = FB_NEW(*getDefaultMemoryPool())
			InternalCryptKey(t, cryptKey->encryptKey, cryptKey->encryptLength,
							 cryptKey->decryptKey, cryptKey->decryptLength);
		port->port_crypt_keys.push(k);
		newKeys.push(k->type);
	}
	catch (const Firebird::Exception& ex)
	{
		ex.stuffException(status);
	}
}


signed char wcCompatible[3][3] = {
/*				DISABLED	ENABLED		REQUIRED */
/* DISABLED */	{0,			0,			-1},
/* ENABLED  */	{0,			1,			2},
/* REQUIRED */	{-1,		2,			2}
};
