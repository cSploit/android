/*
 *	PROGRAM:	JRD Remote Interface/Server
 *  MODULE:		xnet.cpp
 *  DESCRIPTION:	Interprocess Server Communications module.
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
 *
 * 2003.05.01 Victor Seryodkin, Dmitry Yemanov: Completed XNET implementation
 */

#include "firebird.h"
#include <stdio.h>
#include "../../../remote/remote.h"
#include "../../../jrd/ibase.h"
#include "../../../common/thd.h"
#include "../../../remote/os/win32/xnet.h"
#include "../../../utilities/install/install_nt.h"
#include "../../../remote/proto_proto.h"
#include "../../../remote/remot_proto.h"
#include "../../../remote/os/win32/xnet_proto.h"
#include "../../../remote/server/serve_proto.h"
#include "../../../yvalve/gds_proto.h"
#include "../../../common/isc_proto.h"
#include "../../../common/isc_f_proto.h"
#include "../../../common/classes/init.h"
#include "../../../common/classes/fb_string.h"
#include "../../../common/config/config.h"
#include "../../../common/classes/ClumpletWriter.h"
#include "../../../common/utils_proto.h"
#include <time.h>

#ifdef WIN_NT
#include <process.h>
#include <windows.h>
#else
#pragma FB_COMPILER_MESSAGE("POSIX implementation is required")
#endif // WIN_NT

using namespace Firebird;

static bool accept_connection(rem_port*, const P_CNCT*);
static rem_port* alloc_port(rem_port*, UCHAR*, ULONG, UCHAR*, ULONG);
static rem_port* aux_connect(rem_port*, PACKET*);
static rem_port* aux_request(rem_port*, PACKET*);

static void cleanup_comm(XCC);
static void cleanup_mapping(XPM);
static void cleanup_port(rem_port*);
static rem_port* connect_client(PACKET*, const Firebird::RefPtr<Config>*);
static rem_port* connect_server(USHORT);
static void disconnect(rem_port*);
static void force_close(rem_port*);
static int cleanup_ports(const int, const int, void* arg);

static rem_port* receive(rem_port*, PACKET*);
static int send_full(rem_port*, PACKET*);
static int send_partial(rem_port*, PACKET*);

static void server_shutdown(rem_port* port);
static rem_port* get_server_port(ULONG, XPM, ULONG, ULONG, ULONG);
static void make_map(ULONG, ULONG, FILE_ID*, CADDR_T*);
static XPM make_xpm(ULONG, ULONG);
static bool server_init(USHORT);
static XPM get_free_slot(ULONG*, ULONG*, ULONG*);
static bool fork(ULONG, USHORT, ULONG*);

static int xdrxnet_create(XDR*, rem_port*, UCHAR*, USHORT, xdr_op);

static bool_t xnet_getbytes(XDR*, SCHAR*, u_int);
static bool_t xnet_putbytes(XDR*, const SCHAR*, u_int);
static bool_t xnet_read(XDR* xdrs);
static bool_t xnet_write(XDR* xdrs);

static xdr_t::xdr_ops xnet_ops =
{
	xnet_getbytes,
	xnet_putbytes
};

static ULONG global_pages_per_slot = XPS_DEF_PAGES_PER_CLI;
static ULONG global_slots_per_map = XPS_DEF_NUM_CLI;
static XPM global_client_maps = NULL;

#ifdef WIN_NT

static HANDLE xnet_connect_mutex = 0;
static HANDLE xnet_connect_map_h = 0;
static CADDR_T xnet_connect_map = 0;

static HANDLE xnet_connect_event = 0;
static HANDLE xnet_response_event = 0;
static DWORD current_process_id;

// XNET endpoint is the IPC prefix name used to access the server.
// It may have to be dynamically determined and has to be initialized
// before the protocol can be used. It is initialized at the following points:
//  - XNET_reconnect (classic servant size)
//  - connect_client (client side)
//  - server_init (listener side)
static char xnet_endpoint[BUFFER_TINY] = "";

#endif // WIN_NT


static volatile bool xnet_initialized = false;
static volatile bool xnet_shutdown = false;
static GlobalPtr<Mutex> xnet_mutex;
static GlobalPtr<PortsCleanup>	xnet_ports;
static ULONG xnet_next_free_map_num = 0;

static bool connect_init();
static void connect_fini();
static void release_all();

namespace
{
	class ExitHandler
	{
	public:
		explicit ExitHandler(MemoryPool&) {};

		~ExitHandler()
		{
			xnet_shutdown = true;
			release_all();
		}
	};

	static GlobalPtr<ExitHandler> xnet_exit_handler;
}


inline void make_obj_name(char* buffer, size_t size, const char* format)
{
	fb_assert(strcmp(xnet_endpoint, "") != 0);

	fb_utils::snprintf(buffer, size, format, xnet_endpoint);
}

inline void make_map_name(char* buffer, size_t size, const char* format, ULONG arg1, ULONG arg2)
{
	fb_assert(strcmp(xnet_endpoint, "") != 0);

	fb_utils::snprintf(buffer, size, format, xnet_endpoint, arg1, arg2);
}

inline void make_event_name(char* buffer, size_t size, const char* format, ULONG arg1, ULONG arg2, ULONG arg3)
{
	fb_assert(strcmp(xnet_endpoint, "") != 0);

	fb_utils::snprintf(buffer, size, format, xnet_endpoint, arg1, arg2, arg3);
}


static void xnet_error(rem_port*, ISC_STATUS, int);

static void xnet_log_error(const char* err_msg, const Exception& ex)
{
	string str("XNET error: ");
	str += err_msg;
	iscLogException(str.c_str(), ex);
}

static void xnet_log_error(const char* err_msg)
{
	gds__log("XNET error: %s", err_msg);
}

#ifdef DEV_BUILD
#define ERR_STR2(str, lnum) (str #lnum)
#define ERR_STR1(str, lnum) ERR_STR2(str " at line ", lnum)
#define ERR_STR(str) ERR_STR1(str, __LINE__)
#else
#define ERR_STR(str) (str)
#endif

rem_port* XNET_analyze(ClntAuthBlock* cBlock,
					   const PathName& file_name,
					   bool uv_flag,
					   RefPtr<Config>* config,
					   const Firebird::PathName* ref_db_name)
{
/**************************************
 *
 *  X N E T _ a n a l y z e
 *
 **************************************
 *
 * Functional description
 *  Client performs attempt to establish connection
 *  based on the set of protocols.
 *	If a connection is established, return a port block,
 *	otherwise return NULL.
 *
 **************************************/

	// We need to establish a connection to a remote server.
	// Allocate the necessary blocks and get ready to go.

	Rdb* rdb = new Rdb;
	PACKET* packet = &rdb->rdb_packet;

	// Pick up some user identification information

	string buffer;
	ClumpletWriter user_id(ClumpletReader::UnTagged, 64000);
	if (cBlock)
	{
		cBlock->extractDataFromPluginTo(user_id);
	}

	ISC_get_user(&buffer, 0, 0);
	buffer.lower();
	ISC_systemToUtf8(buffer);
	user_id.insertString(CNCT_user, buffer);

	ISC_get_host(buffer);
	buffer.lower();
	ISC_systemToUtf8(buffer);
	user_id.insertString(CNCT_host, buffer);

	if (uv_flag) {
		user_id.insertTag(CNCT_user_verification);
	}

	// Establish connection to server

	P_CNCT* cnct = &packet->p_cnct;
	packet->p_operation = op_connect;
	cnct->p_cnct_operation = op_attach;
	cnct->p_cnct_cversion = CONNECT_VERSION3;
	cnct->p_cnct_client = ARCHITECTURE;

	const PathName& cnct_file(ref_db_name ? (*ref_db_name) : file_name);
	cnct->p_cnct_file.cstr_length = (ULONG) cnct_file.length();
	cnct->p_cnct_file.cstr_address = reinterpret_cast<const UCHAR*>(cnct_file.c_str());

	cnct->p_cnct_user_id.cstr_length = (ULONG) user_id.getBufferLength();
	cnct->p_cnct_user_id.cstr_address = user_id.getBuffer();

	static const p_cnct::p_cnct_repeat protocols_to_try[] =
	{
		REMOTE_PROTOCOL(PROTOCOL_VERSION10, ptype_batch_send, 1),
		REMOTE_PROTOCOL(PROTOCOL_VERSION11, ptype_batch_send, 2),
		REMOTE_PROTOCOL(PROTOCOL_VERSION12, ptype_batch_send, 3),
		REMOTE_PROTOCOL(PROTOCOL_VERSION13, ptype_batch_send, 4)
	};
	fb_assert(FB_NELEM(protocols_to_try) <= FB_NELEM(cnct->p_cnct_versions));
	cnct->p_cnct_count = FB_NELEM(protocols_to_try);

	for (size_t i = 0; i < cnct->p_cnct_count; i++) {
		cnct->p_cnct_versions[i] = protocols_to_try[i];
	}

	// If we can't talk to a server, punt. Let somebody else generate an error.

	rem_port* port = NULL;
	try
	{
		port = XNET_connect(packet, 0, config);
	}
	catch (const Exception&)
	{
		delete rdb;
		throw;
	}

	// Get response packet from server

	rdb->rdb_port = port;
	port->port_context = rdb;
	port->receive(packet);

	P_ACPT* accept = NULL;
	switch (packet->p_operation)
	{
	case op_accept_data:
	case op_cond_accept:
		accept = &packet->p_acpd;
		if (cBlock)
		{
			cBlock->storeDataForPlugin(packet->p_acpd.p_acpt_data.cstr_length,
									   packet->p_acpd.p_acpt_data.cstr_address);
			cBlock->authComplete = packet->p_acpd.p_acpt_authenticated;
			cBlock->resetClnt(&file_name, &packet->p_acpd.p_acpt_keys);
		}
		break;

	case op_accept:
		if (cBlock)
		{
			cBlock->resetClnt(&file_name);
		}
		accept = &packet->p_acpt;
		break;

	case op_response:
		try
		{
			Firebird::LocalStatus warning;		// Ignore connect warnings for a while
			REMOTE_check_response(&warning, rdb, packet);
		}
		catch(const Firebird::Exception&)
		{
			disconnect(port);
			delete rdb;
			throw;
		}
		// fall through - response is not a required accept

	default:
		disconnect(port);
		delete rdb;
		Arg::Gds(isc_connect_reject).raise();
		break;
	}

	fb_assert(accept);
	fb_assert(port);
	port->port_protocol = accept->p_acpt_version;

	// Once we've decided on a protocol, concatenate the version
	// string to reflect it...

	string temp;
	temp.printf("%s/P%d", port->port_version->str_data,
						  port->port_protocol & FB_PROTOCOL_MASK);

	delete port->port_version;
	port->port_version = REMOTE_make_string(temp.c_str());

	if (accept->p_acpt_architecture == ARCHITECTURE)
		port->port_flags |= PORT_symmetric;

	if (accept->p_acpt_type != ptype_out_of_band)
		port->port_flags |= PORT_no_oob;

	return port;
}


rem_port* XNET_connect(PACKET* packet,
					   USHORT flag,
					   Firebird::RefPtr<Config>* config)
{
/**************************************
 *
 *  X N E T _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Establish half of a communication link.
 *
 **************************************/
	if (xnet_shutdown)
	{
		Arg::StatusVector temp;
		temp << Arg::Gds(isc_net_server_shutdown) << Arg::Str("XNET");
		temp.raise();
	}

	if (packet)
	{
		return connect_client(packet, config);
	}

	return connect_server(flag);
}


rem_port* XNET_reconnect(ULONG client_pid)
{
/**************************************
 *
 *  X N E T _ r e c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Classic server initialization code
 *
 **************************************/

	rem_port* port = NULL;
	XPM xpm = NULL;

	// Initialize server-side IPC endpoint to a value we know we have permissions to listen at
	if (strcmp(xnet_endpoint, "") == 0)
	{
		fb_utils::copy_terminate(xnet_endpoint, Config::getDefaultConfig()->getIpcName(),
			sizeof(xnet_endpoint));
		fb_utils::prefix_kernel_object_name(xnet_endpoint, sizeof(xnet_endpoint));
	}

	global_slots_per_map = 1;
	global_pages_per_slot = XPS_MAX_PAGES_PER_CLI;
	xnet_response_event = 0;

	// current_process_id used as map number
	current_process_id = getpid();

	try
	{
		TEXT name_buffer[BUFFER_TINY];
		make_obj_name(name_buffer, sizeof(name_buffer), XNET_RESPONSE_EVENT);
		xnet_response_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
		if (!xnet_response_event) {
			system_error::raise(ERR_STR("OpenEvent"));
		}

		xpm = make_xpm(current_process_id, 0);

		port = get_server_port(client_pid, xpm, current_process_id, 0, 0);
	}
	catch (const Exception& ex)
	{
		xnet_log_error("Unable to initialize child process", ex);

		if (port)
		{
			cleanup_port(port);
			port = NULL;
		}
		else if (xpm)
			cleanup_mapping(xpm);
	}

	if (xnet_response_event)
	{
		SetEvent(xnet_response_event); // to prevent client blocking
		CloseHandle(xnet_response_event);
	}

	return port;
}


static bool connect_init()
{
/**************************************
 *
 *  c o n n e c t _ i n i t
 *
 **************************************
 *
 * Functional description
 *  Initialization of client side resources used
 *  when client performs connect to server
 *
 **************************************/
	TEXT name_buffer[BUFFER_TINY];

	xnet_connect_mutex = 0;
	xnet_connect_map_h = 0;
	xnet_connect_map = 0;

	xnet_connect_event = 0;
	xnet_response_event = 0;

	try
	{
		make_obj_name(name_buffer, sizeof(name_buffer), XNET_CONNECT_MUTEX);
		xnet_connect_mutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, name_buffer);
		if (!xnet_connect_mutex)
		{
			if (ERRNO == ERROR_FILE_NOT_FOUND)
			{
				Arg::Gds(isc_unavailable).raise();
			}

			system_error::raise(ERR_STR("OpenMutex"));
		}

		make_obj_name(name_buffer, sizeof(name_buffer), XNET_CONNECT_EVENT);
		xnet_connect_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
		if (!xnet_connect_event) {
			system_error::raise(ERR_STR("OpenEvent"));
		}

		make_obj_name(name_buffer, sizeof(name_buffer), XNET_RESPONSE_EVENT);
		xnet_response_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
		if (!xnet_response_event) {
			system_error::raise(ERR_STR("OpenEvent"));
		}

		make_obj_name(name_buffer, sizeof(name_buffer), XNET_CONNECT_MAP);
		xnet_connect_map_h = OpenFileMapping(FILE_MAP_WRITE, TRUE, name_buffer);
		if (!xnet_connect_map_h) {
			system_error::raise(ERR_STR("OpenFileMapping"));
		}

		xnet_connect_map = MapViewOfFile(xnet_connect_map_h, FILE_MAP_WRITE, 0, 0,
										 XNET_CONNECT_RESPONZE_SIZE);
		if (!xnet_connect_map) {
			system_error::raise(ERR_STR("MapViewOfFile"));
		}

		return true;
	}
	catch (const Exception&)
	{
		connect_fini();
		throw;
	}
}


static void connect_fini()
{
/**************************************
 *
 *  c o n n e c t _ f i n i
 *
 **************************************
 *
 * Functional description
 *  Release resources allocated in
 *  connect_init()
 *
 **************************************/

	if (xnet_connect_mutex)
	{
		CloseHandle(xnet_connect_mutex);
		xnet_connect_mutex = 0;
	}

	if (xnet_connect_event)
	{
		CloseHandle(xnet_connect_event);
		xnet_connect_event = 0;
	}

	if (xnet_response_event)
	{
		CloseHandle(xnet_response_event);
		xnet_response_event = 0;
	}

	if (xnet_connect_map)
	{
		UnmapViewOfFile(xnet_connect_map);
		xnet_connect_map = 0;
	}

	if (xnet_connect_map_h)
	{
		CloseHandle(xnet_connect_map_h);
		xnet_connect_map_h = 0;
	}
}


static bool accept_connection(rem_port* port, const P_CNCT* cnct)
{
/**************************************
 *
 *	a c c e p t _ c o n n e c t i o n
 *
 **************************************
 *
 * Functional description
 *	Accept an incoming request for connection.
 *
 **************************************/
	// Default account to "guest" (in theory all packets contain a name)

	string user_name("guest"), host_name;

	// Pick up account and host name, if given

	ClumpletReader id(ClumpletReader::UnTagged,
					  cnct->p_cnct_user_id.cstr_address,
					  cnct->p_cnct_user_id.cstr_length);

	for (id.rewind(); !id.isEof(); id.moveNext())
	{
		switch (id.getClumpTag())
		{
		case CNCT_user:
			id.getString(user_name);
			break;

		case CNCT_host:
			id.getString(host_name);
			break;

		default:
			break;
		}
	}

	port->port_login = port->port_user_name = user_name;
	port->port_peer_name = host_name;
	port->port_protocol_id = "XNET";

	return true;
}


static rem_port* alloc_port(rem_port* parent,
							UCHAR* send_buffer,
							ULONG send_length,
							UCHAR* receive_buffer,
							ULONG /*receive_length*/)
{
/**************************************
 *
 *	a l l o c _ p o r t
 *
 **************************************
 *
 * Functional description
 *	Allocate a port block, link it in to parent (if there is a parent),
 *	and initialize input and output XDR streams.
 *
 **************************************/
	rem_port* const port = new rem_port(rem_port::XNET, 0);

	TEXT buffer[BUFFER_TINY];
	ISC_get_host(buffer, sizeof(buffer));
	port->port_host = REMOTE_make_string(buffer);
	port->port_connection = REMOTE_make_string(buffer);
	fb_utils::snprintf(buffer, sizeof(buffer), "XNet (%s)", port->port_host->str_data);
	port->port_version = REMOTE_make_string(buffer);

	port->port_accept = accept_connection;
	port->port_disconnect = disconnect;
	port->port_force_close = force_close;
	port->port_receive_packet = receive;
	port->port_send_packet = send_full;
	port->port_send_partial = send_partial;
	port->port_connect = aux_connect;
	port->port_request = aux_request;
	port->port_buff_size = send_length;

	xdrxnet_create(&port->port_send, port, send_buffer, send_length, XDR_ENCODE);
	xdrxnet_create(&port->port_receive, port, receive_buffer, 0, XDR_DECODE);

	if (parent)
	{
		delete port->port_connection;
		port->port_connection = REMOTE_make_string(parent->port_connection->str_data);

		port->linkParent(parent);
	}

	return port;
}


static rem_port* aux_connect(rem_port* port, PACKET* /*packet*/)
{
/**************************************
 *
 *	a u x _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Try to establish an alternative connection for handling events.
 *  Somebody has already done a successfull connect request.
 *  This uses the existing xcc for the parent port to more
 *  or less duplicate a new xcc for the new aux port pointing
 *  to the event stuff in the map.
 *
 **************************************/

	if (port->port_server_flags)
	{
		port->port_flags |= PORT_async;
		return port;
	}

	XCC parent_xcc = NULL;
	XCC xcc = NULL;
	TEXT name_buffer[BUFFER_TINY];
	XPS xps = NULL;
	XPM xpm = NULL;

	try {

		// make a new xcc
		parent_xcc = port->port_xcc;
		xps = (XPS) parent_xcc->xcc_mapped_addr;

		xcc = new struct xcc;

		xpm = xcc->xcc_xpm = parent_xcc->xcc_xpm;
		xcc->xcc_map_num = parent_xcc->xcc_map_num;
		xcc->xcc_slot = parent_xcc->xcc_slot;
		DuplicateHandle(GetCurrentProcess(), parent_xcc->xcc_proc_h,
						GetCurrentProcess(), &xcc->xcc_proc_h,
						0, FALSE, DUPLICATE_SAME_ACCESS);
		xcc->xcc_flags = 0;
		xcc->xcc_map_handle = parent_xcc->xcc_map_handle;
		xcc->xcc_mapped_addr = parent_xcc->xcc_mapped_addr;
		xcc->xcc_xpm->xpm_count++;

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_C2S_EVNT_CHAN_FILLED,
						xcc->xcc_map_num, xcc->xcc_slot, xpm->xpm_timestamp);
		xcc->xcc_event_send_channel_filled =
			OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
		if (!xcc->xcc_event_send_channel_filled) {
			system_call_failed::raise(ERR_STR("OpenEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_C2S_EVNT_CHAN_EMPTED,
						xcc->xcc_map_num, xcc->xcc_slot, xpm->xpm_timestamp);
		xcc->xcc_event_send_channel_empted =
			OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
		if (!xcc->xcc_event_send_channel_empted) {
			system_call_failed::raise(ERR_STR("OpenEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_S2C_EVNT_CHAN_FILLED,
						xcc->xcc_map_num, xcc->xcc_slot, xpm->xpm_timestamp);
		xcc->xcc_event_recv_channel_filled =
			OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
		if (!xcc->xcc_event_recv_channel_filled) {
			system_call_failed::raise(ERR_STR("OpenEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_S2C_EVNT_CHAN_EMPTED,
						xcc->xcc_map_num, xcc->xcc_slot, xpm->xpm_timestamp);
		xcc->xcc_event_recv_channel_empted =
			OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
		if (!xcc->xcc_event_recv_channel_empted) {
			system_call_failed::raise(ERR_STR("OpenEvent"));
		}

		// send and receive events channels

		xcc->xcc_send_channel = &xps->xps_channels[XPS_CHANNEL_C2S_EVENTS];
		xcc->xcc_recv_channel = &xps->xps_channels[XPS_CHANNEL_S2C_EVENTS];

		UCHAR* const channel_c2s_client_ptr =
			((UCHAR*) xcc->xcc_mapped_addr + sizeof(struct xps));
		UCHAR* const channel_s2c_client_ptr =
			((UCHAR*) xcc->xcc_mapped_addr + sizeof(struct xps) + (XNET_EVENT_SPACE));

		// alloc new port and link xcc to it
		rem_port* const new_port = alloc_port(NULL,
											  channel_c2s_client_ptr, xcc->xcc_send_channel->xch_size,
											  channel_s2c_client_ptr, xcc->xcc_recv_channel->xch_size);

		port->port_async = new_port;
		new_port->port_flags = port->port_flags & PORT_no_oob;
		new_port->port_flags |= PORT_async;
		new_port->port_xcc = xcc;

		return new_port;
	}
	catch (const Exception&)
	{

		xnet_log_error("aux_connect() failed");

		if (xcc)
		{
			if (xcc->xcc_event_send_channel_filled) {
				CloseHandle(xcc->xcc_event_send_channel_filled);
			}
			if (xcc->xcc_event_send_channel_empted) {
				CloseHandle(xcc->xcc_event_send_channel_empted);
			}
			if (xcc->xcc_event_recv_channel_filled) {
				CloseHandle(xcc->xcc_event_recv_channel_filled);
			}
			if (xcc->xcc_event_recv_channel_empted) {
				CloseHandle(xcc->xcc_event_recv_channel_empted);
			}
			delete xcc;
		}

		return NULL;
	}
}


static rem_port* aux_request(rem_port* port, PACKET* packet)
{
/**************************************
 *
 *	a u x _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *  A remote interface has requested the server to
 *  prepare an auxiliary connection.   This is done
 *  by allocating a new port and comm (xcc) structure,
 *  using the event stuff in the map rather than the
 *  normal database channels.
 *
 **************************************/

	XCC xcc = NULL;
	TEXT name_buffer[BUFFER_TINY];

	try {

		// make a new xcc

		XCC parent_xcc = port->port_xcc;
		XPS xps = (XPS) parent_xcc->xcc_mapped_addr;

		xcc = new struct xcc;

		XPM xpm = xcc->xcc_xpm = parent_xcc->xcc_xpm;
		xcc->xcc_map_num = parent_xcc->xcc_map_num;
		xcc->xcc_slot = parent_xcc->xcc_slot;
		DuplicateHandle(GetCurrentProcess(), parent_xcc->xcc_proc_h,
						GetCurrentProcess(), &xcc->xcc_proc_h,
						0, FALSE, DUPLICATE_SAME_ACCESS);
		xcc->xcc_flags = XCCF_ASYNC;
		xcc->xcc_map_handle = parent_xcc->xcc_map_handle;
		xcc->xcc_mapped_addr = parent_xcc->xcc_mapped_addr;
		xcc->xcc_xpm->xpm_count++;

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_C2S_EVNT_CHAN_FILLED,
						xcc->xcc_map_num, xcc->xcc_slot, xpm->xpm_timestamp);
		xcc->xcc_event_recv_channel_filled =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
		if (!xcc->xcc_event_recv_channel_filled ||
			(xcc->xcc_event_recv_channel_filled && ERRNO == ERROR_ALREADY_EXISTS))
		{
			system_call_failed::raise(ERR_STR("CreateEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_C2S_EVNT_CHAN_EMPTED,
						xcc->xcc_map_num, xcc->xcc_slot, xpm->xpm_timestamp);
		xcc->xcc_event_recv_channel_empted =
			CreateEvent(ISC_get_security_desc(), FALSE, TRUE, name_buffer);
		if (!xcc->xcc_event_recv_channel_empted ||
			(xcc->xcc_event_recv_channel_empted && ERRNO == ERROR_ALREADY_EXISTS))
		{
			system_call_failed::raise(ERR_STR("CreateEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_S2C_EVNT_CHAN_FILLED,
						xcc->xcc_map_num, xcc->xcc_slot, xpm->xpm_timestamp);
		xcc->xcc_event_send_channel_filled =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
		if (!xcc->xcc_event_send_channel_filled ||
			(xcc->xcc_event_send_channel_filled && ERRNO == ERROR_ALREADY_EXISTS))
		{
			system_call_failed::raise(ERR_STR("CreateEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_S2C_EVNT_CHAN_EMPTED,
						xcc->xcc_map_num, xcc->xcc_slot, xpm->xpm_timestamp);
		xcc->xcc_event_send_channel_empted =
			CreateEvent(ISC_get_security_desc(), FALSE, TRUE, name_buffer);
		if (!xcc->xcc_event_send_channel_empted ||
			(xcc->xcc_event_send_channel_empted && ERRNO == ERROR_ALREADY_EXISTS))
		{
			system_call_failed::raise(ERR_STR("CreateEvent"));
		}

		// send and receive events channels

		xcc->xcc_send_channel = &xps->xps_channels[XPS_CHANNEL_S2C_EVENTS];
		xcc->xcc_recv_channel = &xps->xps_channels[XPS_CHANNEL_C2S_EVENTS];

		UCHAR* const channel_s2c_client_ptr =
			((UCHAR*) xcc->xcc_mapped_addr + sizeof(struct xps) + (XNET_EVENT_SPACE));
		UCHAR* const channel_c2s_client_ptr =
			((UCHAR*) xcc->xcc_mapped_addr + sizeof(struct xps));

		// alloc new port and link xcc to it
		rem_port* const new_port = alloc_port(NULL,
											  channel_s2c_client_ptr, xcc->xcc_send_channel->xch_size,
											  channel_c2s_client_ptr, xcc->xcc_recv_channel->xch_size);

		port->port_async = new_port;
		new_port->port_xcc = xcc;
		new_port->port_flags = port->port_flags & PORT_no_oob;
		new_port->port_server_flags = port->port_server_flags;

		P_RESP* response = &packet->p_resp;
		response->p_resp_data.cstr_length = 0;
		response->p_resp_data.cstr_address = NULL;

		return new_port;
	}
	catch (const Exception&)
	{

		xnet_log_error("aux_request() failed");

		if (xcc)
		{
			if (xcc->xcc_event_send_channel_filled) {
				CloseHandle(xcc->xcc_event_send_channel_filled);
			}
			if (xcc->xcc_event_send_channel_empted) {
				CloseHandle(xcc->xcc_event_send_channel_empted);
			}
			if (xcc->xcc_event_recv_channel_filled) {
				CloseHandle(xcc->xcc_event_recv_channel_filled);
			}
			if (xcc->xcc_event_recv_channel_empted) {
				CloseHandle(xcc->xcc_event_recv_channel_empted);
			}
			delete xcc;
		}

		return NULL;
	}
}


static void cleanup_comm(XCC xcc)
{
/**************************************
 *
 *  c l e a n u p _ c o m m
 *
 **************************************
 *
 * Functional description
 *  Clean up an xcc structure, close its handles,
 *  unmap its file, and free it.
 *
 **************************************/
	XPS xps = (XPS) xcc->xcc_mapped_addr;
	if (xps) {
		xps->xps_flags |= XPS_DISCONNECTED;
	}

	if (xcc->xcc_event_send_channel_filled) {
		CloseHandle(xcc->xcc_event_send_channel_filled);
	}
	if (xcc->xcc_event_send_channel_empted) {
		CloseHandle(xcc->xcc_event_send_channel_empted);
	}
	if (xcc->xcc_event_recv_channel_filled) {
		CloseHandle(xcc->xcc_event_recv_channel_filled);
	}
	if (xcc->xcc_event_recv_channel_empted) {
		CloseHandle(xcc->xcc_event_recv_channel_empted);
	}
	if (xcc->xcc_proc_h) {
		CloseHandle(xcc->xcc_proc_h);
	}

	XPM xpm = xcc->xcc_xpm;
	if (xpm && !(xcc->xcc_flags & XCCF_ASYNC)) {
		cleanup_mapping(xpm);
	}

	delete xcc;
}


static void cleanup_mapping(XPM xpm)
{
	MutexLockGuard guard(xnet_mutex, FB_FUNCTION);

	// if this was the last area for this map, unmap it
	xpm->xpm_count--;
	if (!xpm->xpm_count && global_client_maps)
	{
		UnmapViewOfFile(xpm->xpm_address);
		CloseHandle(xpm->xpm_handle);

		// find xpm in chain and release

		for (XPM* pxpm = &global_client_maps; *pxpm; pxpm = &(*pxpm)->xpm_next)
		{
			if (*pxpm == xpm)
			{
				*pxpm = xpm->xpm_next;
				break;
			}
		}

		delete xpm;
	}
}


static void cleanup_port(rem_port* port)
{
/**************************************
 *
 *  c l e a n u p _ p o r t
 *
 **************************************
 *
 * Functional description
 *  Walk through the port structure freeing
 *  allocated memory and then free the port.
 *
 **************************************/

	if (port->port_xcc)
	{
		cleanup_comm(port->port_xcc);
		port->port_xcc = NULL;
	}

	port->release();
}


static void raise_lostconn_or_syserror(const char* msg)
{
	if (ERRNO == ERROR_FILE_NOT_FOUND)
		status_exception::raise(Arg::Gds(isc_conn_lost));
	else
		system_error::raise(msg);
}


static rem_port* connect_client(PACKET* packet, const Firebird::RefPtr<Config>* config)
{
/**************************************
 *
 *  c o n n e c t _ c l i e n t
 *
 **************************************
 *
 * Functional description
 *	Establish a client side part of the connection
 *
 **************************************/

	const Firebird::RefPtr<Config>& conf(config ? *config : Config::getDefaultConfig());

	if (!xnet_initialized)
	{
		MutexLockGuard guard(xnet_mutex, FB_FUNCTION);
		if (!xnet_initialized)
		{
			xnet_initialized = true;
			current_process_id = getpid();

			// Allow other (server) process to SYNCHRONIZE with our process
			// to establish communication
			ISC_get_security_desc();
		}
	}

	XNET_RESPONSE response;

	{ // xnet_mutex scope
		MutexLockGuard guard(xnet_mutex, FB_FUNCTION);

		// First, try to connect using default kernel namespace.
		// This should work on Win9X, NT4 and on later OS when server is running
		// under restricted account in the same session as the client
		fb_utils::copy_terminate(xnet_endpoint, conf->getIpcName(), sizeof(xnet_endpoint));

		try
		{
			connect_init();
		}
		catch (const Exception&)
		{
			// The client may not have permissions to create global objects,
			// but still be able to connect to a local server that has such permissions.
			// This is why we try to connect using Global\ namespace unconditionally
			fb_utils::snprintf(xnet_endpoint, sizeof(xnet_endpoint), "Global\\%s", conf->getIpcName());

			if (!connect_init()) {
				return NULL;
			}
		}
	}

	// setup status with net read error in case of wait timeout
	Arg::StatusVector temp;
	temp << Arg::Gds(isc_net_read_err);

	static const int timeout = conf->getConnectionTimeout() * 1000;

	// waiting for XNET connect lock to release

	DWORD err = WaitForSingleObject(xnet_connect_mutex, timeout);

	{ // xnet_mutex scope
		MutexLockGuard guard(xnet_mutex, FB_FUNCTION);

		if (err != WAIT_OBJECT_0)
		{
			connect_fini();

			temp << SYS_ERR(err);
			temp.raise();
		}

		// writing connect request

		// mark connect area with XNET_INVALID_MAP_NUM to
		// detect server faults on response

		((XNET_RESPONSE*) xnet_connect_map)->map_num = XNET_INVALID_MAP_NUM;
		((XNET_RESPONSE*) xnet_connect_map)->proc_id = current_process_id;

		SetEvent(xnet_connect_event);
	}

	// waiting for server response

	err = WaitForSingleObject(xnet_response_event, timeout);

	{ // xnet_mutex scope
		MutexLockGuard guard(xnet_mutex, FB_FUNCTION);

		if (err != WAIT_OBJECT_0)
		{
			ReleaseMutex(xnet_connect_mutex);
			connect_fini();

			temp << SYS_ERR(err);
			temp.raise();
		}

		memcpy(&response, xnet_connect_map, XNET_CONNECT_RESPONZE_SIZE);
		ReleaseMutex(xnet_connect_mutex);
		connect_fini();
	}

	if (response.map_num == XNET_INVALID_MAP_NUM)
	{
		xnet_log_error("Server failed to respond on connect request");

		Arg::StatusVector temp;
		temp << Arg::Gds(isc_net_connect_err);
		temp.raise();
	}

	global_pages_per_slot = response.pages_per_slot;
	global_slots_per_map = response.slots_per_map;
	const ULONG map_num = response.map_num;
	const ULONG slot_num = response.slot_num;
	const ULONG timestamp = response.timestamp;

	TEXT name_buffer[BUFFER_TINY];
	FILE_ID file_handle = 0;
	CADDR_T mapped_address = 0;

	XCC xcc = NULL;
	XPM xpm = NULL;
	XPS xps = NULL;

	try {

		{ // xnet_mutex scope
			MutexLockGuard guard(xnet_mutex, FB_FUNCTION);

			// see if area is already mapped for this client

			for (xpm = global_client_maps; xpm; xpm = xpm->xpm_next)
			{
				if (xpm->xpm_number == map_num &&
					xpm->xpm_timestamp == timestamp &&
					!(xpm->xpm_flags & XPMF_SERVER_SHUTDOWN))
				{
					break;
				}
			}

			if (!xpm)
			{
				// Area hasn't been mapped. Open new file mapping.

				make_map_name(name_buffer, sizeof(name_buffer), XNET_MAPPED_FILE_NAME,
							map_num, timestamp);
				file_handle = OpenFileMapping(FILE_MAP_WRITE, FALSE, name_buffer);
				if (!file_handle) {
					raise_lostconn_or_syserror(ERR_STR("OpenFileMapping"));
				}

				mapped_address = MapViewOfFile(file_handle, FILE_MAP_WRITE, 0L, 0L,
											XPS_MAPPED_SIZE(global_slots_per_map, global_pages_per_slot));
				if (!mapped_address) {
					system_error::raise(ERR_STR("MapViewOfFile"));
				}

				xpm = new struct xpm;

				xpm->xpm_next = global_client_maps;
				global_client_maps = xpm;
				xpm->xpm_count = 0;
				xpm->xpm_number = map_num;
				xpm->xpm_handle = file_handle;
				xpm->xpm_address = mapped_address;
				xpm->xpm_timestamp = timestamp;
				xpm->xpm_flags = 0;
			}

			xpm->xpm_count++;
		} // xnet_mutex scope

		// there's no thread structure, so make one
		xcc = new struct xcc;

		xcc->xcc_map_handle = xpm->xpm_handle;
		xcc->xcc_mapped_addr =
			(UCHAR*) xpm->xpm_address + XPS_SLOT_OFFSET(global_pages_per_slot, slot_num);
		xcc->xcc_map_num = map_num;
		xcc->xcc_slot = slot_num;
		xcc->xcc_xpm = xpm;
		xcc->xcc_flags = 0;
		xcc->xcc_proc_h = 0;

		xps = (XPS) xcc->xcc_mapped_addr;

		// only speak if server has correct protocol

		if (xps->xps_server_protocol != XPI_SERVER_PROTOCOL_VERSION) {
			fatal_exception::raise("Unknown XNET protocol version");
		}

		xps->xps_client_protocol = XPI_CLIENT_PROTOCOL_VERSION;

		// open server process handle to watch server health
		// during communication session

		xcc->xcc_proc_h = OpenProcess(SYNCHRONIZE, 0, xps->xps_server_proc_id);
		if (!xcc->xcc_proc_h) {
			system_error::raise(ERR_STR("OpenProcess"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_C2S_DATA_CHAN_FILLED,
						map_num, slot_num, timestamp);
		xcc->xcc_event_send_channel_filled = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
		if (!xcc->xcc_event_send_channel_filled) {
			raise_lostconn_or_syserror(ERR_STR("OpenEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_C2S_DATA_CHAN_EMPTED,
						map_num, slot_num, timestamp);
		xcc->xcc_event_send_channel_empted = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
		if (!xcc->xcc_event_send_channel_empted) {
			raise_lostconn_or_syserror(ERR_STR("OpenEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_S2C_DATA_CHAN_FILLED,
						map_num, slot_num, timestamp);
		xcc->xcc_event_recv_channel_filled = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
		if (!xcc->xcc_event_recv_channel_filled) {
			raise_lostconn_or_syserror(ERR_STR("OpenEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_S2C_DATA_CHAN_EMPTED,
						map_num, slot_num, timestamp);
		xcc->xcc_event_recv_channel_empted = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
		if (!xcc->xcc_event_recv_channel_empted) {
			raise_lostconn_or_syserror(ERR_STR("OpenEvent"));
		}

		// added this here from the server side as this part is called by the client
		// and the server address need not be valid for the client -smistry 10/29/98
		xcc->xcc_recv_channel = &xps->xps_channels[XPS_CHANNEL_S2C_DATA];
		xcc->xcc_send_channel = &xps->xps_channels[XPS_CHANNEL_C2S_DATA];

		// we also need to add client side flags or channel pointer as they
		// differ from the server side

		const ULONG avail =
			(ULONG) (XPS_USEFUL_SPACE(global_pages_per_slot) - (XNET_EVENT_SPACE * 2)) / 2;
		UCHAR* start_ptr = (UCHAR*) xps + (sizeof(struct xps) + (XNET_EVENT_SPACE * 2));

		// send and receive channels
		UCHAR* const channel_c2s_client_ptr = start_ptr;
		UCHAR* const channel_s2c_client_ptr = start_ptr + avail;

		rem_port* const port =
			alloc_port(NULL,
					   channel_c2s_client_ptr, xcc->xcc_send_channel->xch_size,
					   channel_s2c_client_ptr, xcc->xcc_recv_channel->xch_size);

		port->port_xcc = xcc;
		xnet_ports->registerPort(port);
		send_full(port, packet);
		if (config)
		{
			port->port_config = *config;
		}

		return port;
	}
	catch (const Exception&)
	{
		if (xcc)
			cleanup_comm(xcc);
		else if (xpm)
			cleanup_mapping(xpm);
		else if (file_handle)
		{
			if (mapped_address) {
				UnmapViewOfFile(mapped_address);
			}
			CloseHandle(file_handle);
		}

		throw;
	}
}


static rem_port* connect_server(USHORT flag)
{
/**************************************
 *
 *  c o n n e c t _ s e r v e r
 *
 **************************************
 *
 * Functional description
 *	Establish a server side part of the connection
 *
 **************************************/
	current_process_id = getpid();

	if (!server_init(flag))
		return NULL;

	XNET_RESPONSE* const presponse = (XNET_RESPONSE*) xnet_connect_map;

	while (!xnet_shutdown)
	{
		const DWORD wait_res = WaitForSingleObject(xnet_connect_event, INFINITE);

		if (wait_res != WAIT_OBJECT_0)
		{
			xnet_log_error("WaitForSingleObject() failed");
			break;
		}

		if (xnet_shutdown)
			break;

		// read client process id
		const ULONG client_pid = presponse->proc_id;
		if (!client_pid)
			continue; // dummy xnet_connect_event fire - no connect request

		presponse->slots_per_map = global_slots_per_map;
		presponse->pages_per_slot = global_pages_per_slot;
		presponse->timestamp = 0;

		if (flag & (SRVR_debug | SRVR_multi_client))
		{
			XPM xpm = NULL;
			try
			{
				// MSDN says: In Visual C++ 2005, time is a wrapper for _time64 and time_t is,
				// by default, equivalent to __time64_t.
				// This means that sizeof(time_t) > sizeof(ULONG).
				ULONG timestamp = (ULONG) time(NULL);
				ULONG map_num, slot_num;
				// searching for free slot
				xpm = get_free_slot(&map_num, &slot_num, &timestamp);

				// pack combined mapped area and number
				presponse->proc_id = 0;
				presponse->map_num = map_num;
				presponse->slot_num = slot_num;
				presponse->timestamp = timestamp;

				rem_port* port = get_server_port(client_pid, xpm, map_num, slot_num, timestamp);

				SetEvent(xnet_response_event);

				return port;
			}
			catch (const Exception& ex)
			{
				xnet_log_error("Failed to allocate server port for communication", ex);

				if (xpm)
					cleanup_mapping(xpm);

				SetEvent(xnet_response_event);
				break;
			}
		}

		// in case process we'll fail to start child process
		presponse->slot_num = 0;

		// child process ID (presponse->map_num) used as map number
		if (!fork(client_pid, flag, &presponse->map_num))
		{
			// if fork successfully creates child process, then
			// child process will call SetEvent(xnet_response_event)by itself
			SetEvent(xnet_response_event);
		}
	}

	if (xnet_shutdown)
	{
		Arg::StatusVector temp;
		temp << Arg::Gds(isc_net_server_shutdown) << Arg::Str("XNET");
		temp.raise();
	}

	return NULL;
}


static void disconnect(rem_port* port)
{
/**************************************
 *
 *	d i s c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Break a remote connection.
 *
 **************************************/

	if (port->port_async)
	{
		disconnect(port->port_async);
		port->port_async = NULL;
	}
	port->port_context = NULL;

	// If this is a sub-port, unlink it from it's parent
	port->unlinkParent();
	xnet_ports->unRegisterPort(port);
	cleanup_port(port);
}


static void force_close(rem_port* port)
{
/**************************************
 *
 *	f o r c e _ c l o s e
 *
 **************************************
 *
 * Functional description
 *	Forcibly close remote connection.
 *
 **************************************/
	if (port->port_state != rem_port::PENDING || !port->port_xcc)
		return;

	port->port_state = rem_port::BROKEN;

	XCC xcc = port->port_xcc;

	if (!xcc)
		return;

	if (xcc->xcc_event_send_channel_filled)
	{
		CloseHandle(xcc->xcc_event_send_channel_filled);
		xcc->xcc_event_send_channel_filled = 0;
	}
	if (xcc->xcc_event_send_channel_empted)
	{
		CloseHandle(xcc->xcc_event_send_channel_empted);
		xcc->xcc_event_send_channel_empted = 0;
	}
	if (xcc->xcc_event_recv_channel_filled)
	{
		CloseHandle(xcc->xcc_event_recv_channel_filled);
		xcc->xcc_event_recv_channel_filled = 0;
	}
	if (xcc->xcc_event_recv_channel_empted)
	{
		CloseHandle(xcc->xcc_event_recv_channel_empted);
		xcc->xcc_event_recv_channel_empted = 0;
	}
}


static int cleanup_ports(const int, const int, void* /*arg*/)
{
/**************************************
 *
 *	c l e a n u p _ p o r t s
 *
 **************************************
 *
 * Functional description
 *	Shutdown all active connections
 *	to allow correct shutdown.
 *
 **************************************/
	xnet_shutdown = true;

	SetEvent(xnet_connect_event);
	xnet_ports->closePorts();

	return 0;
}


static rem_port* receive( rem_port* main_port, PACKET* packet)
{
/**************************************
 *
 *	r e c e i v e
 *
 **************************************
 *
 * Functional description
 *	Receive a message from a port.
 *
 **************************************/

#ifdef DEV_BUILD
	main_port->port_receive.x_client = !(main_port->port_flags & PORT_server);
#endif

	try
	{
		if (!xdr_protocol(&main_port->port_receive, packet))
			packet->p_operation = op_exit;
	}
	catch (const Exception&)
	{
		packet->p_operation = op_exit;
	}

	return main_port;
}


static int send_full( rem_port* port, PACKET* packet)
{
/**************************************
 *
 *	s e n d _ f u l l
 *
 **************************************
 *
 * Functional description
 *	Send a packet across a port to another process.
 *  Flush data to remote interface
 *
 **************************************/

#ifdef DEV_BUILD
	port->port_send.x_client = !(port->port_flags & PORT_server);
#endif

	if (!xdr_protocol(&port->port_send, packet))
		return FALSE;

	if (xnet_write(&port->port_send))
		return TRUE;

	xnet_error(port, isc_net_write_err, ERRNO);
	return FALSE;
}


static int send_partial( rem_port* port, PACKET* packet)
{
/**************************************
 *
 *	s e n d _ p a r t i a l
 *
 **************************************
 *
 * Functional description
 *	Send a packet across a port to another process.
 *
 **************************************/

#ifdef DEV_BUILD
	port->port_send.x_client = !(port->port_flags & PORT_server);
#endif

	return xdr_protocol(&port->port_send, packet);
}

static void server_shutdown(rem_port* port)
{
/**************************************
 *
 *      p e e r _ s h u t d o w n
 *
 **************************************
 *
 * Functional description
 *   Server shutdown handler (client side only).
 *
 **************************************/
	fb_assert(!(port->port_flags & PORT_server));

	xnet_log_error("Server shutdown detected");

	XCC xcc = port->port_xcc;
	xcc->xcc_flags |= XCCF_SERVER_SHUTDOWN;

	XPM xpm = xcc->xcc_xpm;
	if (!(xpm->xpm_flags & XPMF_SERVER_SHUTDOWN))
	{
		const ULONG dead_proc_id = XPS(xpm->xpm_address)->xps_server_proc_id;

		// mark all mapped areas connected to the process with pid == dead_proc_id

		MutexLockGuard guard(xnet_mutex, FB_FUNCTION);

		for (xpm = global_client_maps; xpm; xpm = xpm->xpm_next)
		{
			if (!(xpm->xpm_flags & XPMF_SERVER_SHUTDOWN) &&
				(XPS(xpm->xpm_address)->xps_server_proc_id == dead_proc_id))
			{
				xpm->xpm_flags |= XPMF_SERVER_SHUTDOWN;
				xpm->xpm_handle = 0;
				xpm->xpm_address = NULL;
			}
		}
	}
}


static int xdrxnet_create(XDR* xdrs, rem_port* port, UCHAR* buffer, USHORT length, xdr_op x_op)
{
/**************************************
 *
 *  x d r x n e t _ c r e a t e
 *
 **************************************
 *
 * Functional description
 *  Initialize an XDR stream.
 *
 **************************************/

	xdrs->x_public = (caddr_t) port;
	xdrs->x_private = reinterpret_cast<SCHAR*>(buffer);
	xdrs->x_base = xdrs->x_private;
	xdrs->x_handy = length;
	xdrs->x_ops = &xnet_ops;
	xdrs->x_op = x_op;
	xdrs->x_local = true;

	return TRUE;
}


static void xnet_gen_error (rem_port* port, const Arg::StatusVector& v)
{
/**************************************
 *
 *      x n e t _ g e n _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	An error has occurred.  Mark the port as broken.
 *	Format the status vector if there is one and
 *	save the status vector strings in a permanent place.
 *
 **************************************/
	port->port_state = rem_port::BROKEN;
	v.raise();
}


static void xnet_error(rem_port* port, ISC_STATUS operation, int status)
{
/**************************************
 *
 *      x n e t _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	An I/O error has occurred.  If a status vector is present,
 *	generate an error return.  In any case, return NULL, which
 *	is used to indicate and error.
 *
 **************************************/
	if (status)
	{
		if (port->port_state != rem_port::BROKEN)
		{
			gds__log("XNET/xnet_error: errno = %d", status);
		}

		xnet_gen_error(port, Arg::Gds(operation) << SYS_ERR(status));
	}
	else
	{
		xnet_gen_error(port, Arg::Gds(operation));
	}
}


static bool_t xnet_getbytes(XDR* xdrs, SCHAR* buff, u_int count)
{
/**************************************
 *
 *      x n e t _ g e t b y t e s
 *
 **************************************
 *
 * Functional description
 *	Fetch a bunch of bytes from remote interface.
 *
 **************************************/

	SLONG bytecount = count;

	rem_port* port = (rem_port*) xdrs->x_public;
	const bool portServer = (port->port_flags & PORT_server);
	XCC xcc = port->port_xcc;
	XPM xpm = xcc->xcc_xpm;

	while (bytecount && !xnet_shutdown)
	{
		if (!portServer && (xpm->xpm_flags & XPMF_SERVER_SHUTDOWN))
		{
			if (!(xcc->xcc_flags & XCCF_SERVER_SHUTDOWN))
			{
				xcc->xcc_flags |= XCCF_SERVER_SHUTDOWN;
				xnet_error(port, isc_conn_lost, 0);
			}
			return FALSE;
		}

		SLONG to_copy;
		if (xdrs->x_handy >= bytecount)
			to_copy = bytecount;
		else
			to_copy = xdrs->x_handy;

		if (xdrs->x_handy)
		{
			if (to_copy == sizeof(SLONG))
				*((SLONG*)buff)	= *((SLONG*)xdrs->x_private);
			else
				memcpy(buff, xdrs->x_private, to_copy);

			xdrs->x_handy -= to_copy;
			xdrs->x_private += to_copy;
		}
		else
		{
			if (!xnet_read(xdrs))
				return FALSE;
		}

		if (to_copy)
		{
			bytecount -= to_copy;
			buff += to_copy;
		}
	}

	return xnet_shutdown ? FALSE : TRUE;
}


static bool_t xnet_putbytes(XDR* xdrs, const SCHAR* buff, u_int count)
{
/**************************************
 *
 *      x n e t _ p u t b y t e s
 *
 **************************************
 *
 * Functional description
 *	Put a bunch of bytes into a memory stream.
 *
 **************************************/
	SLONG bytecount = count;

	rem_port* port = (rem_port*)xdrs->x_public;
	const bool portServer = (port->port_flags & PORT_server);
	XCC xcc = port->port_xcc;
	XCH xch = xcc->xcc_send_channel;
	XPM xpm = xcc->xcc_xpm;
	XPS xps = (XPS) xcc->xcc_mapped_addr;

	while (bytecount && !xnet_shutdown)
	{
		if (!portServer && (xpm->xpm_flags & XPMF_SERVER_SHUTDOWN))
		{
			if (!(xcc->xcc_flags & XCCF_SERVER_SHUTDOWN))
			{
				xcc->xcc_flags |= XCCF_SERVER_SHUTDOWN;
				xnet_error(port, isc_conn_lost, 0);
			}
			return FALSE;
		}

		SLONG to_copy;
		if (xdrs->x_handy >= bytecount)
			to_copy = bytecount;
		else
			to_copy = xdrs->x_handy;

		if (xdrs->x_handy)
		{

			if ((ULONG) xdrs->x_handy == xch->xch_size)
			{
				while (!xnet_shutdown)
				{
					if (!portServer && (xpm->xpm_flags & XPMF_SERVER_SHUTDOWN))
					{
						if (!(xcc->xcc_flags & XCCF_SERVER_SHUTDOWN))
						{
							xcc->xcc_flags |= XCCF_SERVER_SHUTDOWN;
							xnet_error(port, isc_conn_lost, 0);
						}
						return FALSE;
					}

					const DWORD wait_result =
						WaitForSingleObject(xcc->xcc_event_send_channel_empted,
										    XNET_SEND_WAIT_TIMEOUT);

					if (wait_result == WAIT_OBJECT_0) {
						break;
					}
					if (wait_result == WAIT_TIMEOUT)
					{
						// Check whether another side is alive
						if (WaitForSingleObject(xcc->xcc_proc_h, 1) == WAIT_TIMEOUT)
						{
							// Check whether the channel has been disconnected
							if (!(xps->xps_flags & XPS_DISCONNECTED))
								continue; // another side is alive
						}

						// Another side is dead or something bad has happened
						if (!(xps->xps_flags & XPS_DISCONNECTED) && !portServer)
						{
							server_shutdown(port);
						}

						xnet_error(port, isc_conn_lost, 0);
						return FALSE;
					}

					xnet_error(port, isc_net_write_err, ERRNO);
					return FALSE; // a non-timeout result is an error
				}
			}

			if (to_copy == sizeof(SLONG))
				*((SLONG*)xdrs->x_private) = *((SLONG*)buff);
			else
				memcpy(xdrs->x_private, buff, to_copy);

			xdrs->x_handy -= to_copy;
			xdrs->x_private += to_copy;
		}
		else
		{
			if (!xnet_write(xdrs))
			{
				xnet_error(port, isc_net_write_err, ERRNO);
				return FALSE;
			}
		}

		if (to_copy)
		{
			bytecount -= to_copy;
			buff += to_copy;
		}
	}

	return xnet_shutdown ? FALSE : TRUE;
}


static bool_t xnet_read(XDR* xdrs)
{
/**************************************
 *
 *      x n e t _ r e a d
 *
 **************************************
 *
 * Functional description
 *	Read a buffer full of data.
 *
 **************************************/
	rem_port* port = (rem_port*)xdrs->x_public;
	const bool portServer = (port->port_flags & PORT_server);
	XCC xcc = port->port_xcc;
	XCH xch = xcc->xcc_recv_channel;
	XPM xpm = xcc->xcc_xpm;
	XPS xps = (XPS) xcc->xcc_mapped_addr;

	if (xnet_shutdown)
		return FALSE;

	if (!SetEvent(xcc->xcc_event_recv_channel_empted))
	{
		xnet_error(port, isc_net_read_err, ERRNO);
		return FALSE;
	}

	while (!xnet_shutdown)
	{
		if (!portServer && (xpm->xpm_flags & XPMF_SERVER_SHUTDOWN))
		{
			if (!(xcc->xcc_flags & XCCF_SERVER_SHUTDOWN))
			{
				xcc->xcc_flags |= XCCF_SERVER_SHUTDOWN;
				xnet_error(port, isc_conn_lost, 0);
			}
			return FALSE;
		}

		const DWORD wait_result =
			WaitForSingleObject(xcc->xcc_event_recv_channel_filled, XNET_RECV_WAIT_TIMEOUT);

		if (wait_result == WAIT_OBJECT_0)
		{
			// Client has written some data for us (server) to read
			xdrs->x_handy = xch->xch_length;
			xdrs->x_private = xdrs->x_base;
			return TRUE;
		}
		if (wait_result == WAIT_TIMEOUT)
		{
			// Check if another side is alive
			if (WaitForSingleObject(xcc->xcc_proc_h, 1) == WAIT_TIMEOUT)
			{
				// Check whether the channel has been disconnected
				if (!(xps->xps_flags & XPS_DISCONNECTED))
					continue; // another side is alive
			}

			// Another side is dead or something bad has happened
			if (!(xps->xps_flags & XPS_DISCONNECTED) && !portServer)
			{
				server_shutdown(port);
			}

			xnet_error(port, isc_conn_lost, 0);
			return FALSE;
		}

		xnet_error(port, isc_net_read_err, ERRNO);
		return FALSE; // a non-timeout result is an error
	}

	return FALSE;
}


static bool_t xnet_write(XDR* xdrs)
{
/**************************************
 *
 *      x n e t _ w r i t e
 *
 **************************************
 *
 * Functional description
 *	Signal remote interface that memory stream is
 *  filled and ready for reading.
 *
 **************************************/
	rem_port* port = (rem_port*)xdrs->x_public;
	XCC xcc = port->port_xcc;
	XCH xch = xcc->xcc_send_channel;

	xch->xch_length = xdrs->x_private - xdrs->x_base;
	if (SetEvent(xcc->xcc_event_send_channel_filled))
	{
		xdrs->x_private = xdrs->x_base;
		xdrs->x_handy = xch->xch_size;
		return TRUE;
	}

	return FALSE;
}


void release_all()
{
/**************************************
 *
 *  r e l e a s e _ a l l
 *
 **************************************
 *
 * Functional description
 *      Release all connections and dependant stuff.
 *
 **************************************/

	if (!xnet_initialized)
		return;

	connect_fini();

	MutexLockGuard guard(xnet_mutex, FB_FUNCTION);

	// release all map stuff left not released by broken ports

	XPM xpm, nextxpm;
	for (xpm = nextxpm = global_client_maps; nextxpm; xpm = nextxpm)
	{
		nextxpm = nextxpm->xpm_next;
		UnmapViewOfFile(xpm->xpm_address);
		CloseHandle(xpm->xpm_handle);
		delete xpm;
	}

	global_client_maps = NULL;
	xnet_initialized = false;
}


/***********************************************************************/
/********************** ONLY SERVER CODE FROM HERE *********************/
/***********************************************************************/

static void make_map(ULONG map_number, ULONG timestamp, FILE_ID* map_handle, CADDR_T* map_address)
{
/**************************************
 *
 *	m a k e _ m a p
 *
 **************************************
 *
 * Functional description
 *	Create memory map
 *
 **************************************/
	TEXT name_buffer[BUFFER_TINY];

	make_map_name(name_buffer, sizeof(name_buffer), XNET_MAPPED_FILE_NAME, map_number, timestamp);
	*map_handle = CreateFileMapping(INVALID_HANDLE_VALUE,
									ISC_get_security_desc(),
									PAGE_READWRITE,
									0L,
									XPS_MAPPED_SIZE(global_slots_per_map, global_pages_per_slot),
									name_buffer);

	try
	{
		if (!(*map_handle) || (ERRNO == ERROR_ALREADY_EXISTS))
			system_error::raise(ERR_STR("CreateFileMapping"));

		*map_address = MapViewOfFile(*map_handle, FILE_MAP_WRITE, 0, 0,
									 XPS_MAPPED_SIZE(global_slots_per_map, global_pages_per_slot));

		if (!(*map_address))
			system_error::raise(ERR_STR("MapViewOfFile"));
	}
	catch (const Exception&)
	{
		if (*map_handle)
			CloseHandle(*map_handle);
		throw;
	}
}


static XPM make_xpm(ULONG map_number, ULONG timestamp)
{
/**************************************
 *
 *  m a k e _ x p m
 *
 **************************************
 *
 * Functional description
 *  Create new xpm structure
 *
 **************************************/
	FILE_ID map_handle = 0;
	CADDR_T map_address = 0;

	make_map(map_number, timestamp, &map_handle, &map_address);

	// allocate XPM structure and initialize it

	XPM xpm = new struct xpm;

	xpm->xpm_handle = map_handle;
	xpm->xpm_address = map_address;
	xpm->xpm_number = map_number;
	xpm->xpm_count = 0;
	xpm->xpm_timestamp = timestamp;

	for (USHORT i = 0; i < global_slots_per_map; i++) {
		xpm->xpm_ids[i] = XPM_FREE;
	}
	xpm->xpm_flags = 0;

	MutexLockGuard guard(xnet_mutex, FB_FUNCTION);

	xpm->xpm_next = global_client_maps;
	global_client_maps = xpm;

	return xpm;
}


static bool server_init(USHORT flag)
{
/**************************************
 *
 *  s e r v e r _ i n i t
 *
 **************************************
 *
 * Functional description
 *  Initialization of server side resources used
 *  when clients perform connect to server
 *
 **************************************/
	if (xnet_initialized)
		return true;

	TEXT name_buffer[BUFFER_TINY];

	// Initialize server-side IPC endpoint to a value we know we have permissions to listen at
	if (strcmp(xnet_endpoint, "") == 0)
	{
		fb_utils::copy_terminate(xnet_endpoint, Config::getDefaultConfig()->getIpcName(),
			sizeof(name_buffer));
		fb_utils::prefix_kernel_object_name(xnet_endpoint, sizeof(xnet_endpoint));
	}

	// init the limits

	if (flag & (SRVR_multi_client | SRVR_debug)) {
		global_slots_per_map = XPS_MAX_NUM_CLI;
	}
	else {
		global_slots_per_map = 1;
	}
	global_pages_per_slot = XPS_MAX_PAGES_PER_CLI;

	xnet_connect_mutex = 0;
	xnet_connect_map_h = 0;
	xnet_connect_map = 0;

	xnet_connect_event = 0;
	xnet_response_event = 0;

	try {

		make_obj_name(name_buffer, sizeof(name_buffer), XNET_CONNECT_MUTEX);
		xnet_connect_mutex = CreateMutex(ISC_get_security_desc(), FALSE, name_buffer);
		if (!xnet_connect_mutex || (xnet_connect_mutex && ERRNO == ERROR_ALREADY_EXISTS))
		{
			system_error::raise(ERR_STR("CreateMutex"));
		}

		make_obj_name(name_buffer, sizeof(name_buffer), XNET_CONNECT_EVENT);
		xnet_connect_event = CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
		if (!xnet_connect_event || (xnet_connect_event && ERRNO == ERROR_ALREADY_EXISTS))
		{
			system_error::raise(ERR_STR("CreateEvent"));
		}

		make_obj_name(name_buffer, sizeof(name_buffer), XNET_RESPONSE_EVENT);
		xnet_response_event = CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
		if (!xnet_response_event || (xnet_response_event && ERRNO == ERROR_ALREADY_EXISTS))
		{
			system_error::raise(ERR_STR("CreateEvent"));
		}

		make_obj_name(name_buffer, sizeof(name_buffer), XNET_CONNECT_MAP);
		xnet_connect_map_h = CreateFileMapping(INVALID_HANDLE_VALUE,
											   ISC_get_security_desc(),
											   PAGE_READWRITE,
											   0,
											   XNET_CONNECT_RESPONZE_SIZE,
											   name_buffer);
		if (!xnet_connect_map_h || (xnet_connect_map_h && ERRNO == ERROR_ALREADY_EXISTS))
		{
			system_error::raise(ERR_STR("CreateFileMapping"));
		}

		xnet_connect_map = MapViewOfFile(xnet_connect_map_h, FILE_MAP_WRITE, 0L, 0L,
										 XNET_CONNECT_RESPONZE_SIZE);
		if (!xnet_connect_map) {
			system_error::raise(ERR_STR("MapViewOfFile"));
		}
	}
	catch (const Exception& ex)
	{
		xnet_log_error("XNET server initialization failed. "
			"Probably another instance of server is already running.", ex);

		connect_fini();
		xnet_shutdown = true;

		// the real error is already logged, return isc_net_server_shutdown instead
		Arg::StatusVector temp;
		temp << Arg::Gds(isc_net_server_shutdown) << Arg::Str("XNET");
		temp.raise();
	}

	xnet_initialized = true;
	fb_shutdown_callback(0, cleanup_ports, fb_shut_postproviders, 0);

	return true;
}


static XPM get_free_slot(ULONG* map_num, ULONG* slot_num, ULONG* timestamp)
{
/**************************************
 *
 *  g e t _ f r e e _ s l o t
 *
 **************************************
 *
 * Functional description
 *  Search for free slot in map stuff
  *
 **************************************/

	XPM xpm = NULL;
	ULONG free_slot = 0, free_map = 0;

	MutexLockGuard guard(xnet_mutex, FB_FUNCTION);

	// go through list of maps

	for (xpm = global_client_maps; xpm; xpm = xpm->xpm_next)
	{
		// find an available unused comm area

		for (free_slot = 0; free_slot < global_slots_per_map; free_slot++)
		{
			if (xpm->xpm_ids[free_slot] == XPM_FREE)
				break;
		}

		if (free_slot < global_slots_per_map)
		{
			xpm->xpm_count++;
			xpm->xpm_ids[free_slot] = XPM_BUSY;
			free_map = xpm->xpm_number;
			break;
		}
	}

	// if the mapped file structure has not yet been initialized,
	// make one now

	if (!xpm)
	{
		free_map = xnet_next_free_map_num++;

		// allocate new map file and first slot

		xpm = make_xpm(free_map, *timestamp);

		free_slot = 0;
		xpm->xpm_ids[0] = XPM_BUSY;
		xpm->xpm_count++;
	}
	else
		*timestamp = xpm->xpm_timestamp;

	*map_num = free_map;
	*slot_num = free_slot;

	return xpm;
}


static bool fork(ULONG client_pid, USHORT flag, ULONG* forked_pid)
{
/**************************************
 *
 *  f o r k
 *
 **************************************
 *
 * Functional description
 *  Create child process to serve client connection
 *  It's for classic server only
 *
 **************************************/
	TEXT name[MAXPATHLEN];
	GetModuleFileName(NULL, name, sizeof(name));

	string cmdLine;
	cmdLine.printf("%s -x -h %"ULONGFORMAT, name, client_pid);

	STARTUPINFO start_crud;
	start_crud.cb = sizeof(STARTUPINFO);
	start_crud.lpReserved = NULL;
	start_crud.lpReserved2 = NULL;
	start_crud.cbReserved2 = 0;
	start_crud.lpDesktop = NULL;
	start_crud.lpTitle = NULL;
	start_crud.dwFlags = STARTF_FORCEOFFFEEDBACK;
	PROCESS_INFORMATION pi;

	const bool cp_result =
		CreateProcess(NULL, cmdLine.begin(), NULL, NULL, FALSE,
					  (flag & SRVR_high_priority ? HIGH_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS)
						| DETACHED_PROCESS | CREATE_SUSPENDED,
					   NULL, NULL, &start_crud, &pi);

	// Child process ID (forked_pid) used as map number

	if (cp_result)
	{
		*forked_pid = pi.dwProcessId;
		ResumeThread(pi.hThread);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
	else {
		xnet_log_error("CreateProcess() failed");
	}

	return cp_result;
}


static rem_port* get_server_port(ULONG client_pid,
								 XPM xpm,
								 ULONG map_num,
								 ULONG slot_num,
								 ULONG timestamp)
{
/**************************************
 *
 *  g e t _ s e r v e r _ p o r t
 *
 **************************************
 *
 * Functional description
 *	Allocates new rem_port for server side communication.
 *
 **************************************/
	rem_port* port = NULL;
	TEXT name_buffer[BUFFER_TINY];

	// allocate a communications control structure and fill it in

	XCC xcc = new struct xcc;

	try {

		UCHAR* p = (UCHAR*) xpm->xpm_address + XPS_SLOT_OFFSET(global_pages_per_slot, slot_num);
		memset(p, 0, XPS_MAPPED_PER_CLI(global_pages_per_slot));
		xcc->xcc_next = NULL;
		xcc->xcc_mapped_addr = p;
		xcc->xcc_xpm = xpm;
		xcc->xcc_slot = slot_num;
		xcc->xcc_flags = 0;

		// Open client process handle to watch clients health
		// during communication session

		xcc->xcc_proc_h = OpenProcess(SYNCHRONIZE, 0, client_pid);
		if (!xcc->xcc_proc_h) {
			system_error::raise(ERR_STR("OpenProcess"));
		}

		xcc->xcc_map_num = map_num;
		XPS xps = (XPS) xcc->xcc_mapped_addr;
		xps->xps_client_proc_id = client_pid;
		xps->xps_server_proc_id = current_process_id;

		// make sure client knows what this server speaks

		xps->xps_server_protocol = XPI_SERVER_PROTOCOL_VERSION;
		xps->xps_client_protocol = 0L;

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_C2S_DATA_CHAN_FILLED,
						map_num, slot_num, timestamp);
		xcc->xcc_event_recv_channel_filled =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
		if (!xcc->xcc_event_recv_channel_filled) {
			system_error::raise(ERR_STR("CreateEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_C2S_DATA_CHAN_EMPTED,
						map_num, slot_num, timestamp);
		xcc->xcc_event_recv_channel_empted =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
		if (!xcc->xcc_event_recv_channel_empted) {
			system_error::raise(ERR_STR("CreateEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_S2C_DATA_CHAN_FILLED,
						map_num, slot_num, timestamp);
		xcc->xcc_event_send_channel_filled =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
		if (!xcc->xcc_event_send_channel_filled) {
			system_error::raise(ERR_STR("CreateEvent"));
		}

		make_event_name(name_buffer, sizeof(name_buffer), XNET_E_S2C_DATA_CHAN_EMPTED,
						map_num, slot_num, timestamp);
		xcc->xcc_event_send_channel_empted =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
		if (!xcc->xcc_event_send_channel_empted) {
			system_error::raise(ERR_STR("CreateEvent"));
		}

		// set up the channel structures

		p += sizeof(struct xps);

		const ULONG avail =
			(ULONG) (XPS_USEFUL_SPACE(global_pages_per_slot) - (XNET_EVENT_SPACE * 2)) / 2;

		// client to server events
		//UCHAR* const channel_c2s_event_buffer = p;
		xps->xps_channels[XPS_CHANNEL_C2S_EVENTS].xch_size = XNET_EVENT_SPACE;

		p += XNET_EVENT_SPACE;
		// server to client events
		//UCHAR* const channel_s2c_event_buffer = p;
		xps->xps_channels[XPS_CHANNEL_S2C_EVENTS].xch_size = XNET_EVENT_SPACE;

		p += XNET_EVENT_SPACE;
		// client to server data
		UCHAR* const channel_c2s_data_buffer = p;
		xps->xps_channels[XPS_CHANNEL_C2S_DATA].xch_size = avail;

		p += avail;
		// server to client data
		UCHAR* const channel_s2c_data_buffer = p;
		xps->xps_channels[XPS_CHANNEL_S2C_DATA].xch_size = avail;

		xcc->xcc_recv_channel = &xps->xps_channels[XPS_CHANNEL_C2S_DATA];
		xcc->xcc_send_channel = &xps->xps_channels[XPS_CHANNEL_S2C_DATA];

		// finally, allocate and set the port structure for this client

		port = alloc_port(NULL,
						  channel_s2c_data_buffer, xcc->xcc_send_channel->xch_size,
						  channel_c2s_data_buffer, xcc->xcc_recv_channel->xch_size);

		port->port_xcc = xcc;
		port->port_server_flags |= SRVR_server;
		port->port_flags |= PORT_server;

		xnet_ports->registerPort(port);
	}
	catch (const Exception&)
	{
		if (port)
			cleanup_port(port);
		else if (xcc)
			cleanup_comm(xcc);

		throw;
	}

	return port;
}
