/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		wnet.cpp
 *	DESCRIPTION:	Windows Net Communications module.
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

#ifdef DEBUG
// define WNET_trace to 0 (zero) for no packet debugging
#define WNET_trace
#endif

#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../remote/remote.h"
#include "../jrd/ibase.h"
#include "../common/thd.h"

#include "../utilities/install/install_nt.h"

#include "../remote/proto_proto.h"
#include "../remote/remot_proto.h"
#include "../remote/os/win32/wnet_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/isc_proto.h"
#include "../common/isc_f_proto.h"
#include "../common/config/config.h"
#include "../common/utils_proto.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/classes/init.h"

#include <stdarg.h>

using namespace Firebird;

const int MAX_DATA		= 2048;
const int BUFFER_SIZE	= MAX_DATA;

const char* PIPE_PREFIX			= "pipe"; // win32-specific
const char* SERVER_PIPE_SUFFIX	= "server";
const char* EVENT_PIPE_SUFFIX	= "event";
AtomicCounter event_counter;

static GlobalPtr<PortsCleanup> wnet_ports;
static GlobalPtr<Mutex> init_mutex;
static volatile bool wnet_initialized = false;
static volatile bool wnet_shutdown = false;

static bool		accept_connection(rem_port*, const P_CNCT*);
static rem_port*		alloc_port(rem_port*);
static rem_port*		aux_connect(rem_port*, PACKET*);
static rem_port*		aux_request(rem_port*, PACKET*);
static bool		connect_client(rem_port*);
static void		disconnect(rem_port*);
#ifdef NOT_USED_OR_REPLACED
static void		exit_handler(void*);
#endif
static void		force_close(rem_port*);
static rem_str*		make_pipe_name(const RefPtr<Config>&, const TEXT*, const TEXT*, const TEXT*);
static rem_port*	receive(rem_port*, PACKET*);
static int		send_full(rem_port*, PACKET*);
static int		send_partial(rem_port*, PACKET*);
static int		xdrwnet_create(XDR*, rem_port*, UCHAR*, USHORT, xdr_op);
static bool_t	xdrwnet_endofrecord(XDR*);//, int);
static bool		wnet_error(rem_port*, const TEXT*, ISC_STATUS, int);
static void		wnet_gen_error(rem_port*, const Arg::StatusVector& v);
static bool_t	wnet_getbytes(XDR*, SCHAR*, u_int);
static bool_t	wnet_putbytes(XDR*, const SCHAR*, u_int);
static bool_t	wnet_read(XDR*);
static bool_t	wnet_write(XDR*); //, int);
#ifdef DEBUG
static void		packet_print(const TEXT*, const UCHAR*, const int);
#endif
static bool		packet_receive(rem_port*, UCHAR*, SSHORT, SSHORT*);
static bool		packet_send(rem_port*, const SCHAR*, SSHORT);
static void		wnet_make_file_name(TEXT*, DWORD);

static int		cleanup_ports(const int, const int, void*);

static xdr_t::xdr_ops wnet_ops =
{
	wnet_getbytes,
	wnet_putbytes
};


rem_port* WNET_analyze(ClntAuthBlock* cBlock,
					   const PathName& file_name,
					   const TEXT* node_name,
					   bool uv_flag,
					   RefPtr<Config>* config,
					   const Firebird::PathName* ref_db_name)
{
/**************************************
 *
 *	W N E T _ a n a l y z e
 *
 **************************************
 *
 * Functional description
 *	Determine whether the file name has a "\\nodename".
 *	If so, establish an external connection to the node.
 *
 *	If a connection is established, return a port block, otherwise
 *	return NULL.
 *
 **************************************/

	// We need to establish a connection to a remote server.  Allocate the necessary
	// blocks and get ready to go.

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

	P_CNCT* const cnct = &packet->p_cnct;
	packet->p_operation = op_connect;
	cnct->p_cnct_operation = op_attach;
	cnct->p_cnct_cversion = CONNECT_VERSION3;
	cnct->p_cnct_client = ARCHITECTURE;

	const PathName& cnct_file(ref_db_name ? (*ref_db_name) : file_name);
	cnct->p_cnct_file.cstr_length = (ULONG) cnct_file.length();
	cnct->p_cnct_file.cstr_address = reinterpret_cast<const UCHAR*>(cnct_file.c_str());

	// If we want user verification, we can't speak anything less than version 7

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
		port = WNET_connect(node_name, packet, 0, config);
	}
	catch (const Exception&)
	{
		delete rdb;
		throw;
	}

	// Get response packet from server.

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
			port->addServerKeys(&packet->p_acpd.p_acpt_keys);
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

	// once we've decided on a protocol, concatenate the version
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


rem_port* WNET_connect(const TEXT* name, PACKET* packet, USHORT flag, Firebird::RefPtr<Config>* config)
{
/**************************************
 *
 *	W N E T _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Establish half of a communication link.  If a connect packet is given,
 *	the connection is on behalf of a remote interface.  Otherwise the
 *	connect is for a server process.
 *
 **************************************/
	rem_port* const port = alloc_port(0);
	if (config)
	{
		port->port_config = *config;
	}

	delete port->port_connection;
	port->port_connection = make_pipe_name(port->getPortConfig(), name, SERVER_PIPE_SUFFIX, 0);

	// If we're a host, just make the connection

	if (packet)
	{
		while (true)
		{
			port->port_pipe = CreateFile(port->port_connection->str_data,
										 GENERIC_WRITE | GENERIC_READ,
										 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
			if (port->port_pipe != INVALID_HANDLE_VALUE) {
				break;
			}
			const ISC_STATUS status = GetLastError();
			if (status != ERROR_PIPE_BUSY)
			{
				wnet_error(port, "CreateFile", isc_net_connect_err, status);
				disconnect(port);
				return NULL;
			}
			WaitNamedPipe(port->port_connection->str_data, 3000L);
		}
		send_full(port, packet);
		return port;
	}

	// We're a server, so wait for a host to show up

	wnet_ports->registerPort(port);
	while (!wnet_shutdown)
	{
		port->port_pipe =
			CreateNamedPipe(port->port_connection->str_data,
							PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
							PIPE_WAIT | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
							PIPE_UNLIMITED_INSTANCES,
							MAX_DATA,
							MAX_DATA,
							0,
							ISC_get_security_desc());
		if (port->port_pipe == INVALID_HANDLE_VALUE)
		{
			const DWORD dwError = GetLastError();
			if (dwError == ERROR_CALL_NOT_IMPLEMENTED)
			{
				disconnect(port);
				wnet_shutdown = true;
				break;
			}

			wnet_error(port, "CreateNamedPipe", isc_net_connect_listen_err, dwError);
			disconnect(port);
			return NULL;
		}

		if (!connect_client(port))
			break;

		if (flag & (SRVR_debug | SRVR_multi_client))
		{
			port->port_server_flags |= SRVR_server;
			port->port_flags |= PORT_server;
			if (flag & SRVR_multi_client)
			{
				port->port_server_flags |= SRVR_multi_client;
			}

			return port;
		}

		TEXT name[MAXPATHLEN];
		GetModuleFileName(NULL, name, sizeof(name));

		string cmdLine;
		cmdLine.printf("%s -w -h %"HANDLEFORMAT"@%"ULONGFORMAT, name, port->port_pipe, GetCurrentProcessId());

		STARTUPINFO start_crud;
		PROCESS_INFORMATION pi;
		start_crud.cb = sizeof(STARTUPINFO);
		start_crud.lpReserved = NULL;
		start_crud.lpReserved2 = NULL;
		start_crud.cbReserved2 = 0;
		start_crud.lpDesktop = NULL;
		start_crud.lpTitle = NULL;
		start_crud.dwFlags = STARTF_FORCEOFFFEEDBACK;

		if (CreateProcess(NULL, cmdLine.begin(), NULL, NULL, FALSE,
						  (flag & SRVR_high_priority ?
							 HIGH_PRIORITY_CLASS | DETACHED_PROCESS :
							 NORMAL_PRIORITY_CLASS | DETACHED_PROCESS),
						  NULL, NULL, &start_crud, &pi))
		{
			// hvlad: child process will close our handle of client pipe
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
		else
		{
			gds__log("WNET/inet_error: fork/CreateProcess errno = %d", GetLastError());
			CloseHandle(port->port_pipe);
		}

		if (wnet_shutdown)
			disconnect(port);
	}

	if (wnet_shutdown)
	{
		Arg::Gds temp(isc_net_server_shutdown);
		temp << Arg::Str("WNET");
		temp.raise();
	}

	return NULL;
}


rem_port* WNET_reconnect(HANDLE handle)
{
/**************************************
 *
 *	W N E T _ r e c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	A communications link has been established by another
 *	process.  We have inherited the handle.  Set up
 *	a port block.
 *
 **************************************/
	rem_port* const port = alloc_port(0);

	delete port->port_connection;
	port->port_connection = make_pipe_name(port->getPortConfig(), NULL, SERVER_PIPE_SUFFIX, 0);

	port->port_pipe = handle;
	port->port_server_flags |= SRVR_server;
	port->port_flags |= PORT_server;

	return port;
}


static bool accept_connection( rem_port* port, const P_CNCT* cnct)
{
/**************************************
 *
 *	a c c e p t _ c o n n e c t i o n
 *
 **************************************
 *
 * Functional description
 *	Accept an incoming request for connection.  This is purely a lower
 *	level handshaking function, and does not constitute the server
 *	response for protocol selection.
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
	port->port_protocol_id = "WNET";

	return true;
}


static rem_port* alloc_port( rem_port* parent)
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

	if (!wnet_initialized)
	{
		MutexLockGuard guard(init_mutex, FB_FUNCTION);
		if (!wnet_initialized)
		{
			wnet_initialized = true;
			fb_shutdown_callback(0, cleanup_ports, fb_shut_postproviders, 0);
		}
	}

	rem_port* port = new rem_port(rem_port::PIPE, BUFFER_SIZE * 2);

	TEXT buffer[BUFFER_TINY];
	ISC_get_host(buffer, sizeof(buffer));
	port->port_host = REMOTE_make_string(buffer);
	port->port_connection = REMOTE_make_string(buffer);
	sprintf(buffer, "WNet (%s)", port->port_host->str_data);
	port->port_version = REMOTE_make_string(buffer);

	port->port_accept = accept_connection;
	port->port_disconnect = disconnect;
	port->port_force_close = force_close;
	port->port_receive_packet = receive;
	port->port_send_packet = send_full;
	port->port_send_partial = send_partial;
	port->port_connect = aux_connect;
	port->port_request = aux_request;
	port->port_buff_size = BUFFER_SIZE;

	port->port_event = CreateEvent(NULL, TRUE, TRUE, NULL);

	xdrwnet_create(&port->port_send, port, &port->port_buffer[BUFFER_SIZE], BUFFER_SIZE, XDR_ENCODE);

	xdrwnet_create(&port->port_receive, port, port->port_buffer, 0, XDR_DECODE);

	if (parent)
	{
		delete port->port_connection;
		port->port_connection = REMOTE_make_string(parent->port_connection->str_data);

		port->linkParent(parent);
	}

	return port;
}


static rem_port* aux_connect( rem_port* port, PACKET* packet)
{
/**************************************
 *
 *	a u x _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Try to establish an alternative connection.  Somebody has already
 *	done a successfull connect request ("packet" contains the response).
 *
 **************************************/
	// If this is a server, we're got an auxiliary connection.  Accept it

	if (port->port_server_flags)
	{
		if (!connect_client(port))
			return NULL;

		port->port_flags |= PORT_async;
		return port;
	}

	// The server will be sending its process id in the packet to
	// create a unique pipe name.

	P_RESP* response = &packet->p_resp;

	TEXT str_pid[32];
	const TEXT* p = 0;
	if (response->p_resp_data.cstr_length)
	{
		// Avoid B.O.
		const size_t len = MIN(response->p_resp_data.cstr_length, sizeof(str_pid) - 1);
		memcpy(str_pid, response->p_resp_data.cstr_address, len);
		str_pid[len] = 0;
		p = str_pid;
	}

	rem_port* const new_port = alloc_port(port->port_parent);
	port->port_async = new_port;
	new_port->port_flags = port->port_flags & PORT_no_oob;
	new_port->port_flags |= PORT_async;
	new_port->port_connection = make_pipe_name(port->getPortConfig(),
		port->port_connection->str_data, EVENT_PIPE_SUFFIX, p);

	while (true)
	{
		new_port->port_pipe =
			CreateFile(new_port->port_connection->str_data, GENERIC_READ, 0,
					   NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (new_port->port_pipe != INVALID_HANDLE_VALUE)
			break;
		const ISC_STATUS status = GetLastError();
		if (status != ERROR_PIPE_BUSY)
		{
			wnet_error(new_port, "CreateFile", isc_net_event_connect_err, status);
			return NULL;
		}
		WaitNamedPipe(new_port->port_connection->str_data, 3000L);
	}

	return new_port;
}


static rem_port* aux_request( rem_port* vport, PACKET* packet)
{
/**************************************
 *
 *	a u x _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	A remote interface has requested the server prepare an auxiliary
 *	connection; the server calls aux_request to set up the connection.
 *	Send the servers process id on the packet.  If at a later time
 *	a multi client server is used, there may be a need to
 *	generate a unique id based on connection.
 *
 **************************************/

	const DWORD server_pid = (vport->port_server_flags & SRVR_multi_client) ?
		++event_counter : GetCurrentProcessId();
	rem_port* const new_port = alloc_port(vport->port_parent);
	vport->port_async = new_port;
	new_port->port_server_flags = vport->port_server_flags;
	new_port->port_flags = vport->port_flags & PORT_no_oob;

	TEXT str_pid[32];
	wnet_make_file_name(str_pid, server_pid);
	new_port->port_connection = make_pipe_name(vport->getPortConfig(),
		vport->port_connection->str_data, EVENT_PIPE_SUFFIX, str_pid);

	new_port->port_pipe =
		CreateNamedPipe(new_port->port_connection->str_data,
						PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
						PIPE_WAIT | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
						PIPE_UNLIMITED_INSTANCES,
						MAX_DATA,
						MAX_DATA,
						0,
						ISC_get_security_desc());

	if (new_port->port_pipe == INVALID_HANDLE_VALUE)
	{
		wnet_error(new_port, "CreateNamedPipe", isc_net_event_listen_err, ERRNO);
		disconnect(new_port);
		return NULL;
	}

	P_RESP* response = &packet->p_resp;
	response->p_resp_data.cstr_length = (ULONG) strlen(str_pid);
	memcpy(response->p_resp_data.cstr_address, str_pid, response->p_resp_data.cstr_length);

	return new_port;
}


static bool connect_client(rem_port *port)
{
/**************************************
 *
 *	c o n n e c t _ c l i e n t
 *
 **************************************
 *
 * Functional description
 *	Wait for new client connected.
 *
 **************************************/

	OVERLAPPED ovrl = {0};
	ovrl.hEvent = port->port_event;
	if (!ConnectNamedPipe(port->port_pipe, &ovrl))
	{
		DWORD err = GetLastError();
		switch (err)
		{
		case ERROR_PIPE_CONNECTED:
			break;

		case ERROR_IO_PENDING:
			if (WaitForSingleObject(port->port_event, INFINITE) == WAIT_OBJECT_0)
			{
				if (!wnet_shutdown)
					break;
			}
			else
				err = GetLastError(); // fall thru

		default:
			if (!wnet_shutdown) {
				wnet_error(port, "ConnectNamedPipe", isc_net_connect_err, err);
			}
			disconnect(port);
			return false;
		}
	}
	return true;
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

	// If this is a sub-port, unlink it from its parent
	port->unlinkParent();

	if (port->port_server_flags & SRVR_server)
	{
		FlushFileBuffers(port->port_pipe);
		DisconnectNamedPipe(port->port_pipe);
	}
	if (port->port_event != INVALID_HANDLE_VALUE)
	{
		CloseHandle(port->port_event);
		port->port_event = INVALID_HANDLE_VALUE;
	}
	if (port->port_pipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(port->port_pipe);
		port->port_pipe = INVALID_HANDLE_VALUE;
	}

	wnet_ports->unRegisterPort(port);
	port->release();
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

	if (port->port_event != INVALID_HANDLE_VALUE)
	{
		port->port_state = rem_port::BROKEN;

		const HANDLE handle = port->port_pipe;
		port->port_pipe = INVALID_HANDLE_VALUE;
		SetEvent(port->port_event);
		CloseHandle(handle);
	}
}


#ifdef NOT_USED_OR_REPLACED
static void exit_handler(void* main_port)
{
/**************************************
 *
 *	e x i t _ h a n d l e r
 *
 **************************************
 *
 * Functional description
 *	Shutdown all active connections
 *	to allow restart.
 *
 **************************************/
	for (rem_port* vport = static_cast<rem_port*>(main_port); vport; vport = vport->port_next)
		CloseHandle(vport->port_pipe);
}
#endif


static rem_str* make_pipe_name(const RefPtr<Config>& config, const TEXT* connect_name,
	const TEXT* suffix_name, const TEXT* str_pid)
{
/**************************************
 *
 *	m a k e _ p i p e _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Construct a name for the pipe connection.
 *	Figure out whether we need a remote node name,
 *	and construct the pipe name accordingly.
 *	If a server pid != 0, append it to pipe name  as <>/<pid>
 *
 **************************************/
	string buffer("\\\\");

	const TEXT* p = connect_name;

	if (!p || *p++ != '\\' || *p++ != '\\')
		p = ".";

	while (*p && *p != '\\' && *p != '@')
		buffer += *p++;

	const TEXT* protocol = NULL;
	switch (*p)
	{
	case 0:
		protocol = config->getRemoteServiceName();
		break;
	case '@':
		protocol = p + 1;
		break;
	default:
		while (*p)
		{
			if (*p++ == '\\')
				protocol = p;
		}
	}

	buffer += '\\';
	buffer += PIPE_PREFIX;
	buffer += '\\';
	const char *pipe_name = config->getRemotePipeName();
	buffer += pipe_name;
	buffer += '\\';
	buffer += suffix_name;
	buffer += '\\';
	buffer += protocol;

	if (str_pid)
	{
		buffer += '\\';
		buffer += str_pid;
	}

	return REMOTE_make_string(buffer.c_str());
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
 *	Receive a message from a port or clients of a port.  If the process
 *	is a server and a connection request comes in, generate a new port
 *	block for the client.
 *
 **************************************/

#ifdef DEV_BUILD
	main_port->port_receive.x_client = !(main_port->port_flags & PORT_server);
#endif

	if (!xdr_protocol(&main_port->port_receive, packet))
		packet->p_operation = op_exit;

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
 *
 **************************************/

#ifdef DEV_BUILD
	port->port_send.x_client = !(port->port_flags & PORT_server);
#endif

	if (!xdr_protocol(&port->port_send, packet))
		return FALSE;

	return xdrwnet_endofrecord(&port->port_send); //, TRUE);
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


static int xdrwnet_create(XDR* xdrs,
						  rem_port* port,
						  UCHAR* buffer, USHORT length, xdr_op x_op)
{
/**************************************
 *
 *	x d r w n e t _ c r e a t e
 *
 **************************************
 *
 * Functional description
 *	Initialize an XDR stream for Apollo mailboxes.
 *
 **************************************/

	xdrs->x_public = (caddr_t) port;
	xdrs->x_base = xdrs->x_private = reinterpret_cast<SCHAR*>(buffer);
	xdrs->x_handy = length;
	xdrs->x_ops = &wnet_ops;
	xdrs->x_op = x_op;

	return TRUE;
}


static bool_t xdrwnet_endofrecord( XDR* xdrs) //, bool_t flushnow)
{
/**************************************
 *
 *	x d r w n e t _ e n d o f r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Write out the rest of a record.
 *
 **************************************/

	return wnet_write(xdrs); //, flushnow);
}


static bool wnet_error(rem_port* port,
					  const TEXT* function, ISC_STATUS operation, int status)
{
/**************************************
 *
 *	w n e t _ e r r o r
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
		if (port->port_state != rem_port::BROKEN) {
			gds__log("WNET/wnet_error: %s errno = %d", function, status);
		}

		wnet_gen_error(port, Arg::Gds(operation) << SYS_ERR(status));
	}
	else
	{
		wnet_gen_error(port, Arg::Gds(operation));
	}

	return false;
}


static void wnet_gen_error (rem_port* port, const Arg::StatusVector& v)
{
/**************************************
 *
 *	w n e t _ g e n _ e r r o r
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

	TEXT node_name[MAXPATHLEN];
	if (port->port_connection)
	{
		fb_utils::copy_terminate(node_name, port->port_connection->str_data + 2, sizeof(node_name));
		TEXT* const p = strchr(node_name, '\\');
		if (p != NULL)
			*p = '\0';
	}
	else
	{
		strcpy(node_name, "(unknown)");
	}

	Arg::Gds error(isc_network_error);
	error << Arg::Str(node_name) << v;
	error.raise();
}


static bool_t wnet_getbytes( XDR* xdrs, SCHAR* buff, u_int count)
{
/**************************************
 *
 *	w n e t _ g e t b y t e s
 *
 **************************************
 *
 * Functional description
 *	Get a bunch of bytes from a memory stream if it fits.
 *
 **************************************/
	SLONG bytecount = count;

	// Use memcpy to optimize bulk transfers.

	while (bytecount > (SLONG) sizeof(ISC_QUAD))
	{
		if (xdrs->x_handy >= bytecount)
		{
			memcpy(buff, xdrs->x_private, bytecount);
			xdrs->x_private += bytecount;
			xdrs->x_handy -= bytecount;
			return TRUE;
		}
		if (xdrs->x_handy > 0)
		{
			memcpy(buff, xdrs->x_private, xdrs->x_handy);
			xdrs->x_private += xdrs->x_handy;
			buff += xdrs->x_handy;
			bytecount -= xdrs->x_handy;
			xdrs->x_handy = 0;
		}
		if (!wnet_read(xdrs))
			return FALSE;
	}

	// Scalar values and bulk transfer remainder fall thru
	// to be moved byte-by-byte to avoid memcpy setup costs.

	if (!bytecount)
		return TRUE;

	if (xdrs->x_handy >= bytecount)
	{
		xdrs->x_handy -= bytecount;
		do {
			*buff++ = *xdrs->x_private++;
		} while (--bytecount);
		return TRUE;
	}

	while (--bytecount >= 0)
	{
		if (!xdrs->x_handy && !wnet_read(xdrs))
			return FALSE;
		*buff++ = *xdrs->x_private++;
		--xdrs->x_handy;
	}

	return TRUE;
}


static bool_t wnet_putbytes( XDR* xdrs, const SCHAR* buff, u_int count)
{
/**************************************
 *
 *	w n e t _ p u t b y t e s
 *
 **************************************
 *
 * Functional description
 *	Put a bunch of bytes to a memory stream if it fits.
 *
 **************************************/
	SLONG bytecount = count;

	// Use memcpy to optimize bulk transfers.

	while (bytecount > (SLONG) sizeof(ISC_QUAD))
	{
		if (xdrs->x_handy >= bytecount)
		{
			memcpy(xdrs->x_private, buff, bytecount);
			xdrs->x_private += bytecount;
			xdrs->x_handy -= bytecount;
			return TRUE;
		}
		if (xdrs->x_handy > 0)
		{
			memcpy(xdrs->x_private, buff, xdrs->x_handy);
			xdrs->x_private += xdrs->x_handy;
			buff += xdrs->x_handy;
			bytecount -= xdrs->x_handy;
			xdrs->x_handy = 0;
		}
		if (!wnet_write(xdrs /*, 0*/))
			return FALSE;
	}

	// Scalar values and bulk transfer remainder fall thru
	// to be moved byte-by-byte to avoid memcpy setup costs.

	if (!bytecount)
		return TRUE;

	if (xdrs->x_handy >= bytecount)
	{
		xdrs->x_handy -= bytecount;
		do {
			*xdrs->x_private++ = *buff++;
		} while (--bytecount);
		return TRUE;
	}

	while (--bytecount >= 0)
	{
		if (xdrs->x_handy <= 0 && !wnet_write(xdrs /*, 0*/))
			return FALSE;
		--xdrs->x_handy;
		*xdrs->x_private++ = *buff++;
	}

	return TRUE;
}


static bool_t wnet_read( XDR* xdrs)
{
/**************************************
 *
 *	w n e t _ r e a d
 *
 **************************************
 *
 * Functional description
 *	Read a buffer full of data.  If we receive a bad packet,
 *	send the moral equivalent of a NAK and retry.  ACK all
 *	partial packets.  Don't ACK the last packet -- the next
 *	message sent will handle this.
 *
 **************************************/
	rem_port* port = (rem_port*) xdrs->x_public;
	SCHAR* p = xdrs->x_base;
	const SCHAR* const end = p + BUFFER_SIZE;

	// If buffer is not completely empty, slide down what what's left

	if (xdrs->x_handy > 0)
	{
		memmove(p, xdrs->x_private, xdrs->x_handy);
		p += xdrs->x_handy;
	}

	while (true)
	{
		SSHORT length = end - p;
		if (!packet_receive(port, reinterpret_cast<UCHAR*>(p), length, &length))
		{
			return FALSE;
		}
		if (length >= 0)
		{
			p += length;
			break;
		}
		p -= length;
		if (!packet_send(port, 0, 0))
			return FALSE;
	}

	xdrs->x_handy = (int) (p - xdrs->x_base);
	xdrs->x_private = xdrs->x_base;

	return TRUE;
}


static bool_t wnet_write( XDR* xdrs /*, bool_t end_flag*/)
{
/**************************************
 *
 *	w n e t _ w r i t e
 *
 **************************************
 *
 * Functional description
 *	Write a buffer fulll of data.
 *  Obsolete: If the end_flag isn't set, indicate
 *	that the buffer is a fragment, and reset the XDR for another buffer
 *	load.
 *
 **************************************/
	// Encode the data portion of the packet

	rem_port* vport = (rem_port*) xdrs->x_public;
	const SCHAR* p = xdrs->x_base;
	SSHORT length = xdrs->x_private - p;

	// Send data in manageable hunks.  If a packet is partial, indicate
	// that with a negative length.  A positive length marks the end.

	while (length)
	{
		const SSHORT l = MIN(length, MAX_DATA);
		length -= l;
		if (!packet_send(vport, p, (SSHORT) (length ? -l : l)))
			return FALSE;
		p += l;
	}

	xdrs->x_private = xdrs->x_base;
	xdrs->x_handy = BUFFER_SIZE;

	return TRUE;
}


#ifdef DEBUG
static void packet_print(const TEXT* string, const UCHAR* packet, const int length)
{
/**************************************
 *
 *	p a c k e t _ p r i n t
 *
 **************************************
 *
 * Functional description
 *	Print a summary of packet.
 *
 **************************************/
	int sum = 0;
	int l = length;

	if (l)
	{
		do {
			sum += *packet++;
		} while (--l);
	}

	printf("%s\t: length = %d, checksum = %d\n", string, length, sum);
}
#endif


static bool packet_receive(rem_port* port, UCHAR* buffer, SSHORT buffer_length, SSHORT* length)
{
/**************************************
 *
 *	p a c k e t _ r e c e i v e
 *
 **************************************
 *
 * Functional description
 *	Receive a packet and pass on it's goodness.  If it's good,
 *	return true and the reported length of the packet, and update
 *	the receive sequence number.  If it's bad, return false.  If it's
 *	a duplicate message, just ignore it.
 *
 **************************************/
	DWORD n = 0;
	OVERLAPPED ovrl = {0};
	ovrl.hEvent = port->port_event;

	BOOL status = ReadFile(port->port_pipe, buffer, buffer_length, &n, &ovrl);
	DWORD dwError = GetLastError();

	if (!status && dwError == ERROR_IO_PENDING)
	{
		status = GetOverlappedResult(port->port_pipe, &ovrl, &n, TRUE);
		dwError = GetLastError();
	}
	if (!status && dwError != ERROR_BROKEN_PIPE) {
		return wnet_error(port, "ReadFile", isc_net_read_err, dwError);
	}

	if (!n)
	{
		if (port->port_flags & PORT_detached)
			return false;

		return wnet_error(port, "ReadFile end-of-file", isc_net_read_err, dwError);
	}

	// decrypt
	if (port->port_crypt_plugin)
	{
		LocalStatus st;
		port->port_crypt_plugin->decrypt(&st, n, buffer, buffer);
		if (!st.isSuccess())
		{
			status_exception::raise(st.get());
		}
	}

#if defined(DEBUG) && defined(WNET_trace)
	packet_print("receive", buffer, n);
#endif

	*length = (SSHORT) n;

	return true;
}


static bool packet_send( rem_port* port, const SCHAR* buffer, SSHORT buffer_length)
{
/**************************************
 *
 *	p a c k e t _ s e n d
 *
 **************************************
 *
 * Functional description
 *	Send some data on it's way.
 *
 **************************************/
	const SCHAR* data = buffer;
	const DWORD length = buffer_length;

	// encrypt
	HalfStaticArray<char, BUFFER_TINY> b;
	if (port->port_crypt_plugin && port->port_crypt_complete)
	{
		LocalStatus st;

		char* d = b.getBuffer(buffer_length);
		port->port_crypt_plugin->encrypt(&st, buffer_length, data, d);
		if (!st.isSuccess())
		{
			status_exception::raise(st.get());
		}

		data = d;
	}

	OVERLAPPED ovrl = {0};
	ovrl.hEvent = port->port_event;

	DWORD n;
	BOOL status = WriteFile(port->port_pipe, data, length, &n, &ovrl);
	DWORD dwError = GetLastError();

	if (!status && dwError == ERROR_IO_PENDING)
	{
		status = GetOverlappedResult(port->port_pipe, &ovrl, &n, TRUE);
		dwError = GetLastError();
	}
	if (!status)
		return wnet_error(port, "WriteFile", isc_net_write_err, dwError);
	if (n != length)
		return wnet_error(port, "WriteFile truncated", isc_net_write_err, dwError);

#if defined(DEBUG) && defined(WNET_trace)
	packet_print("send", reinterpret_cast<const UCHAR*>(buffer), buffer_length);
#endif

	return true;
}


static void wnet_make_file_name( TEXT* name, DWORD number)
{
/**************************************
 *
 *      w n e t _ m a k e _ f i l e _ n a m e
 *
 **************************************
 *
 * Functional description
 *      Create a file name out of a number making sure
 *	the Windows <8>.<3> limitations are handled.
 *
 **************************************/
	TEXT temp[32];

	sprintf(temp, "%lu", number);

	size_t length = strlen(temp);
	if (length < 8)
	{
		strcpy(name, temp);
		return;
	}

	TEXT* p = name;
	const TEXT* q = temp;

	while (length)
	{
		size_t len = (length > 8) ? 8 : length;
		length -= len;
		do {
			*p++ = *q++;
		} while (--len != 0);

		if (length)
			*p++ = '\\';
	}
	*p++ = 0;
}

static int cleanup_ports(const int, const int, void*)
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
	wnet_shutdown = true;

	wnet_ports->closePorts();
	return 0;
}
