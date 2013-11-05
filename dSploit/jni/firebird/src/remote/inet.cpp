/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		inet.cpp
 *	DESCRIPTION:	TCP/UCP/IP Communications module.
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "EPSON" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "XENIX" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "IMP" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "NCR3000" port
 *
 * 2002-02-23 Sean Leyne - Code Cleanup, removed old M88K and NCR3000 port
 *
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "UNIXWARE" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix/MIPS" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 * 2002.10.30 Sean Leyne - Code Cleanup, removed obsolete "SUN3_3" port
 * 2005.04.01 Konstantin Kuznetsov - allow setting NoNagle option in Classic
 *
 */

#include "firebird.h"
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "../jrd/common.h"
#include "../jrd/file_params.h"
#include <stdarg.h>

#include "../common/classes/timestamp.h"
#include "../common/classes/init.h"
#include "../jrd/ThreadStart.h"

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h> // for socket()
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif



#ifdef	WIN_NT
#define FD_SETSIZE 1024
#endif

#ifndef WIN_NT
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#if defined(HAVE_POLL_H)
#include <poll.h>
#elif defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#endif // !WIN_NT

const int INET_RETRY_CALL = 5;

#include "../remote/remote.h"
#include "../jrd/ibase.h"
#include "../common/thd.h"
#include "../remote/inet_proto.h"
#include "../remote/proto_proto.h"
#include "../remote/remot_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/os/isc_i_proto.h"

#include "../common/config/config.h"
#include "../common/utils_proto.h"
#include "../common/classes/ClumpletWriter.h"

// Please review. Maybe not needed. See H_ERRNO in common.h.
#if defined HPUX
extern int h_errno;
#endif

using namespace Firebird;


#ifdef WIN_NT

#include <fcntl.h>
#include <process.h>
#include <signal.h>
#include "../utilities/install/install_nt.h"

#define INET_RETRY_ERRNO	WSAEINPROGRESS
#define INET_ADDR_IN_USE	WSAEADDRINUSE
#define sleep(seconds)  Sleep ((seconds) * 1000)
const int NOTASOCKET = WSAENOTSOCK;

#else // WIN_NT

#ifndef INET_ADDR_IN_USE
#define INET_ADDR_IN_USE EADDRINUSE
#endif
#ifndef INET_RETRY_ERRNO
#define INET_RETRY_ERRNO TRY_AGAIN
#endif
const int NOTASOCKET = EBADF;

#endif // WIN_NT


static void SOCLOSE(SOCKET& socket)
{
	if (socket != INVALID_SOCKET)
	{
#ifdef WIN_NT
		closesocket(socket);
#else
		close(socket);
#endif
		socket = INVALID_SOCKET;
	}
};

// Can't find were it's used.
//#ifndef SIGURG
//#define SIGURG	SIGINT
//#endif

#ifndef ENOBUFS
#define ENOBUFS	0
#endif

#ifndef FB_SEND_FLAGS
#define FB_SEND_FLAGS 0
#endif

#ifndef FB_SETOPT_FLAGS
#define FB_SETOPT_FLAGS 0
#endif

//
//#define DEBUG	1
//

#ifdef DEBUG
#ifdef HAVE_SYS_TIMEB_H
# include <sys/timeb.h>
#endif
const int TRACE_packets		= 1 << 0;	// bit 0
const int TRACE_operations	= 1 << 1;	// bit 1
const int TRACE_summary		= 1 << 2;	// bit 2

static int INET_trace = TRACE_summary | TRACE_packets | TRACE_operations;
static time_t INET_start_time = 0;
SLONG INET_force_error = -1;	// simulate a network error
static ULONG INET_count_send = 0;
static ULONG INET_count_recv = 0;
static ULONG INET_bytes_send = 0;
static ULONG INET_bytes_recv = 0;

static ULONG inet_debug_timer()
{
/**************************************
 *
 *	i n e t _ d e b u g _ t i m e r
 *
 **************************************
 *
 * Functional description
 *	Utility function used in DEBUG mode only to put a timestamp
 *	since start of connection on each debug output message.
 *
 *	This has been implemented and tested on SOLARIS, and may
 *	need tweeking on any other platform where DEBUG is needed.
 *
 **************************************/
#ifdef HAVE_GETTIMEOFDAY
	struct timeval tv;
	GETTIMEOFDAY(&tv);
	return (tv.tv_sec * 1000 + tv.tv_usec - INET_start_time);
#else
	struct timeb now;
	ftime(&now);
	return (now.time * 1000 + now.millitm - INET_start_time);
#endif // HAVE_GETTIMEOFDAY
}
#endif // DEBUG

const SLONG MAX_DATA_LW		= 1448;		// Low  Water mark
const SLONG MAX_DATA_HW		= 32768;	// High Water mark
const SLONG DEF_MAX_DATA	= 8192;

//const int MAXHOSTLEN		= 64;

const int SELECT_TIMEOUT	= 60;		// Dispatch thread select timeout (sec)

class Select
{
#ifdef HAVE_POLL
private:
	static const int SEL_INIT_EVENTS = POLLIN;
	static const int SEL_CHECK_MASK = POLLIN;

	pollfd* getPollFd(int n)
	{
		pollfd* const end = slct_poll.end();
		for (pollfd* pf = slct_poll.begin(); pf < end; ++pf)
		{
			if (n == pf->fd)
			{
				return pf;
			}
		}

		return NULL;
	}

	static int compare(const void* a, const void* b)
	{
		// use C-cast here to be for sure compatible with libc
		return ((pollfd*)a)->fd - ((pollfd*)b)->fd;
	}
#endif

public:
#ifdef HAVE_POLL
	Select()
		: slct_time(0), slct_count(0), slct_poll(*getDefaultMemoryPool())
	{ }

	Select(Firebird::MemoryPool& pool)
		: slct_time(0), slct_count(0), slct_poll(pool)
	{ }
#else
	Select()
		: slct_time(0), slct_count(0), slct_width(0)
	{
		memset(&slct_fdset, 0, sizeof slct_fdset);
	}

	Select(Firebird::MemoryPool& /*pool*/)
		: slct_time(0), slct_count(0), slct_width(0)
	{
		memset(&slct_fdset, 0, sizeof slct_fdset);
	}
#endif

	enum HandleState {SEL_BAD, SEL_DISCONNECTED, SEL_NO_DATA, SEL_READY};

	HandleState ok(const rem_port* port)
	{
		SOCKET n = port->port_handle;
#if defined(WIN_NT)
		return FD_ISSET(n, &slct_fdset) ? SEL_READY : SEL_NO_DATA;
#elif defined(HAVE_POLL)
		const pollfd* pf = getPollFd(n);
		if (pf)
		{
			return pf->events & SEL_CHECK_MASK ? SEL_READY : SEL_NO_DATA;
		}
		return (n < 0) ? (port->port_flags & PORT_disconnect ? SEL_DISCONNECTED : SEL_BAD) : SEL_NO_DATA;
#else
		if (n < 0 || n >= FD_SETSIZE)
		{
			return port->port_flags & PORT_disconnect ? SEL_DISCONNECTED : SEL_BAD;
		}
		return (n < slct_width && FD_ISSET(n, &slct_fdset)) ? SEL_READY : SEL_NO_DATA;
#endif
	}

	void unset(SOCKET handle)
	{
#if defined(HAVE_POLL)
		pollfd* pf = getPollFd(handle);
		if (pf)
		{
			pf->events = 0;
		}
#else
		FD_CLR(handle, &slct_fdset);
		--slct_count;
#endif
	}

	void set(SOCKET handle)
	{
#ifdef HAVE_POLL
		pollfd* pf = getPollFd(handle);
		if (pf)
		{
			pf->events = SEL_INIT_EVENTS;
			return;
		}
		pollfd f;
		f.fd = handle;
		f.events = SEL_INIT_EVENTS;
		slct_poll.push(f);
#else
		FD_SET(handle, &slct_fdset);
#ifdef WIN_NT
		++slct_width;
#else
		slct_width = MAX(slct_width, handle + 1);
#endif // WIN_NT
#endif // HAVE_POLL
	}

	void clear()
	{
		slct_count = 0;
#if defined(HAVE_POLL)
		slct_poll.clear();
#else
		slct_width = 0;
		FD_ZERO(&slct_fdset);
#endif
	}

	void select(timeval* timeout)
	{
		bool hasRequest = false;

#ifdef HAVE_POLL
		pollfd* const end = slct_poll.end();
		for (pollfd* pf = slct_poll.begin(); pf < end; ++pf)
		{
			pf->revents = pf->events;
			if (pf->events & SEL_CHECK_MASK)
			{
				hasRequest = true;
			}
		}

		if (!hasRequest)
		{
			errno = NOTASOCKET;
			slct_count = -1;
			return;
		}

		int milliseconds = timeout ? timeout->tv_sec * 1000 + timeout->tv_usec / 1000 : -1;
		slct_count = ::poll(slct_poll.begin(), slct_poll.getCount(), milliseconds);

		if (slct_count >= 0)	// in case of error return revents may contain something bad
		{
			for (pollfd* pf = slct_poll.begin(); pf < end; ++pf)
			{
				pf->events = pf->revents;
			}
		}
#else
#ifdef WIN_NT
		slct_count = ::select(FD_SETSIZE, &slct_fdset, NULL, NULL, timeout);
#else
		slct_count = ::select(slct_width, &slct_fdset, NULL, NULL, timeout);
#endif // WIN_NT
#endif // HAVE_POLL

	}

	int getCount()
	{
		return slct_count;
	}

	time_t	slct_time;

private:
	int		slct_count;
#ifdef HAVE_POLL
	Firebird::HalfStaticArray<pollfd, 8> slct_poll;
#else
	int		slct_width;
	fd_set	slct_fdset;
#endif
};

static bool		accept_connection(rem_port*, const P_CNCT*);
#ifdef HAVE_SETITIMER
static void		alarm_handler(int);
#endif
static rem_port*		alloc_port(rem_port*, const USHORT = 0);
static rem_port*		aux_connect(rem_port*, PACKET*);
static rem_port*		aux_request(rem_port*, PACKET*);

#if !defined(WIN_NT)
static bool		check_host(rem_port*);
static THREAD_ENTRY_DECLARE waitThread(THREAD_ENTRY_PARAM);

static Firebird::GlobalPtr<Firebird::Mutex> waitThreadMutex;
static unsigned int procCount = 0;
#endif // WIN_NT

static void		disconnect(rem_port*);
static void		force_close(rem_port*);
static int		cleanup_ports(const int, const int, void*);

#ifdef NO_FORK
static int		fork();
#endif

#ifdef WIN_NT
static void		wsaExitHandler(void*);
static int		fork(SOCKET, USHORT);
static THREAD_ENTRY_DECLARE forkThread(THREAD_ENTRY_PARAM);

static Firebird::GlobalPtr<Firebird::Mutex> forkMutex;
static HANDLE forkEvent = INVALID_HANDLE_VALUE;
static bool forkThreadStarted = false;

typedef Firebird::Array<SOCKET> SocketsArray;
static SocketsArray* forkSockets;

#endif

static in_addr get_bind_address();
static int get_host_address(const char* name, in_addr* const host_addr_arr, const int arr_size);

static void copy_p_cnct_repeat_array(	p_cnct::p_cnct_repeat*			pDest,
										const p_cnct::p_cnct_repeat*	pSource,
										size_t							nEntries);

static int		inet_destroy(XDR*);
static void		inet_gen_error(rem_port*, const Firebird::Arg::StatusVector& v);
static bool_t	inet_getbytes(XDR*, SCHAR *, u_int);
static bool_t	inet_getlong(XDR*, SLONG *);
static u_int	inet_getpostn(XDR*);
static caddr_t	inet_inline(XDR*, u_int);
static void		inet_error(rem_port*, const TEXT*, ISC_STATUS, int);
static bool_t	inet_putlong(XDR*, const SLONG*);
static bool_t	inet_putbytes(XDR*, const SCHAR*, u_int);
static bool_t	inet_read(XDR*);
static bool_t	inet_setpostn(XDR*, u_int);
static rem_port*		inet_try_connect(	PACKET*,
									Rdb*,
									const Firebird::PathName&,
									const TEXT*,
									ISC_STATUS*,
									Firebird::ClumpletReader&);
static bool_t	inet_write(XDR*); //, int);

#ifdef DEBUG
static void packet_print(const TEXT*, const UCHAR*, int, ULONG);
#endif

static bool		packet_receive(rem_port*, UCHAR*, SSHORT, SSHORT*);
static bool		packet_send(rem_port*, const SCHAR*, SSHORT);
static rem_port*		receive(rem_port*, PACKET *);
static rem_port*		select_accept(rem_port*);

static void		select_port(rem_port*, Select*, RemPortPtr&);
static bool		select_multi(rem_port*, UCHAR* buffer, SSHORT bufsize, SSHORT* length, RemPortPtr&);
static bool		select_wait(rem_port*, Select*);
static int		send_full(rem_port*, PACKET *);
static int		send_partial(rem_port*, PACKET *);

static int		xdrinet_create(XDR*, rem_port*, UCHAR *, USHORT, enum xdr_op);
static bool		setNoNagleOption(rem_port*);
static FPTR_INT	tryStopMainThread = 0;



static XDR::xdr_ops inet_ops =
{
	inet_getlong,
	inet_putlong,
	inet_getbytes,
	inet_putbytes,
	inet_getpostn,
	inet_setpostn,
	inet_inline,
	inet_destroy
};


#define MAXCLIENTS	NOFILE - 10

// Select uses bit masks of file descriptors in longs.

#ifndef NBBY
#define	NBBY	8
#endif

#ifndef NFDBITS
#if !defined(WIN_NT)
#define NFDBITS		(sizeof(SLONG) * NBBY)

#define	FD_SET(n, p)	((p)->fds_bits[(n) / NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n) / NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n) / NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	memset(p, 0, sizeof(*(p)))
#endif
#endif

#ifdef	WIN_NT
#define	INTERRUPT_ERROR(x)	(SYSCALL_INTERRUPTED(x) || (x) == WSAEINTR)
#else
#define	INTERRUPT_ERROR(x)	(SYSCALL_INTERRUPTED(x))
#endif



SLONG INET_remote_buffer;
static Firebird::GlobalPtr<Firebird::Mutex> init_mutex;
static volatile bool INET_initialized = false;
static volatile bool INET_shutting_down = false;
static Firebird::GlobalPtr<Select> INET_select;
static int INET_max_clients;
static rem_port* inet_async_receive = NULL;


static Firebird::GlobalPtr<Firebird::Mutex> port_mutex;
static Firebird::GlobalPtr<PortsCleanup>	inet_ports;


rem_port* INET_analyze(const Firebird::PathName& file_name,
					ISC_STATUS*	status_vector,
					const TEXT*	node_name,
					const TEXT*	user_string,
					bool	uv_flag,
					Firebird::ClumpletReader &dpb)
{
/**************************************
 *
 *	I N E T _ a n a l y z e
 *
 **************************************
 *
 * Functional description
 *	File_name is on node_name.
 *	Establish an external connection to node_name.
 *
 *	If a connection is established, return a port block, otherwise
 *	return NULL.
 *
 *	If the "uv_flag" is non-zero, user verification also takes place.
 *
 **************************************/
	// We need to establish a connection to a remote server.  Allocate the necessary
	// blocks and get ready to go.

	Rdb* rdb = new Rdb;
	PACKET* packet = &rdb->rdb_packet;

	// Pick up some user identification information
	Firebird::ClumpletWriter user_id(Firebird::ClumpletReader::UnTagged, MAX_DPB_SIZE);
	Firebird::string buffer;
	int eff_gid;
	int eff_uid;

	ISC_get_user(&buffer, &eff_uid, &eff_gid, user_string);
	user_id.insertString(CNCT_user, buffer);

	ISC_get_host(buffer);
	buffer.lower();
	user_id.insertString(CNCT_host, buffer);

	if ((eff_uid == -1) || uv_flag) {
		user_id.insertTag(CNCT_user_verification);
	}
	else
	{
		// Communicate group id info to server, as user maybe running under group
		// id other than default specified in /etc/passwd.

		eff_gid = htonl(eff_gid);
		user_id.insertBytes(CNCT_group, reinterpret_cast<UCHAR*>(&eff_gid), sizeof(eff_gid));
	}

	// Establish connection to server

	// Note: prior to V3.1E a recievers could not in truth handle more
	// than 5 protocol descriptions, so we try them in chunks of 5 or less

	// If we want user verification, we can't speak anything less than version 7

	P_CNCT*	cnct = &packet->p_cnct;

	cnct->p_cnct_user_id.cstr_length = (USHORT) user_id.getBufferLength();
	cnct->p_cnct_user_id.cstr_address = user_id.getBuffer();

	static const p_cnct::p_cnct_repeat protocols_to_try1[] =
	{
		REMOTE_PROTOCOL(PROTOCOL_VERSION8, ptype_rpc, ptype_lazy_send, 1),
		REMOTE_PROTOCOL(PROTOCOL_VERSION10, ptype_rpc, ptype_lazy_send, 2),
		REMOTE_PROTOCOL(PROTOCOL_VERSION11, ptype_rpc, ptype_lazy_send, 3),
		REMOTE_PROTOCOL(PROTOCOL_VERSION12, ptype_rpc, ptype_lazy_send, 4)
#ifdef SCROLLABLE_CURSORS
		,
		REMOTE_PROTOCOL(PROTOCOL_SCROLLABLE_CURSORS, ptype_rpc, ptype_lazy_send, 99)
#endif
	};

	cnct->p_cnct_count = FB_NELEM(protocols_to_try1);

	copy_p_cnct_repeat_array(cnct->p_cnct_versions, protocols_to_try1, cnct->p_cnct_count);

	// Try connection using first set of protocols.  punt if error

	rem_port* port = inet_try_connect(packet, rdb, file_name, node_name, status_vector, dpb);
	if (!port) {
		return NULL;
	}

	if (packet->p_operation == op_reject && !uv_flag)
	{
		disconnect(port);

		// try again with next set of known protocols

		cnct->p_cnct_user_id.cstr_length = (USHORT) user_id.getBufferLength();
		cnct->p_cnct_user_id.cstr_address = user_id.getBuffer();

		static const p_cnct::p_cnct_repeat protocols_to_try2[] =
		{
			REMOTE_PROTOCOL(PROTOCOL_VERSION6, ptype_rpc, ptype_batch_send, 1),
			REMOTE_PROTOCOL(PROTOCOL_VERSION7, ptype_rpc, ptype_lazy_send, 2)
		};

		cnct->p_cnct_count = FB_NELEM(protocols_to_try2);

		copy_p_cnct_repeat_array(cnct->p_cnct_versions, protocols_to_try2, cnct->p_cnct_count);

		port = inet_try_connect(packet, rdb, file_name, node_name, status_vector, dpb);
		if (!port) {
			return NULL;
		}
	}

	if (packet->p_operation == op_reject && !uv_flag)
	{
		disconnect(port);

		// try again with next set of known protocols

		cnct->p_cnct_user_id.cstr_length = (USHORT) user_id.getBufferLength();
		cnct->p_cnct_user_id.cstr_address = user_id.getBuffer();

		static const p_cnct::p_cnct_repeat protocols_to_try3[] =
		{
			REMOTE_PROTOCOL(PROTOCOL_VERSION3, ptype_rpc, ptype_batch_send, 1),
			REMOTE_PROTOCOL(PROTOCOL_VERSION4, ptype_rpc, ptype_batch_send, 2)
		};

		cnct->p_cnct_count = FB_NELEM(protocols_to_try3);

		copy_p_cnct_repeat_array(cnct->p_cnct_versions, protocols_to_try3, cnct->p_cnct_count);

		port = inet_try_connect(packet, rdb, file_name, node_name, status_vector, dpb);
		if (!port) {
			return NULL;
		}
	}

	if (packet->p_operation != op_accept)
	{
		*status_vector++ = isc_arg_gds;
		*status_vector++ = isc_connect_reject;
		*status_vector++ = 0;
		disconnect(port);
		delete rdb;
		return NULL;
	}

	port->port_protocol = packet->p_acpt.p_acpt_version;

	// once we've decided on a protocol, concatenate the version
	// string to reflect it...
	Firebird::string temp;
	temp.printf("%s/P%d", port->port_version->str_data, port->port_protocol & FB_PROTOCOL_MASK);
	delete port->port_version;
	port->port_version = REMOTE_make_string(temp.c_str());

	if (packet->p_acpt.p_acpt_architecture == ARCHITECTURE) {
		port->port_flags |= PORT_symmetric;
	}

	if (packet->p_acpt.p_acpt_type == ptype_rpc) {
		port->port_flags |= PORT_rpc;
	}

	if (packet->p_acpt.p_acpt_type != ptype_out_of_band) {
		port->port_flags |= PORT_no_oob;
	}

	if (packet->p_acpt.p_acpt_type == ptype_lazy_send) {
		port->port_flags |= PORT_lazy;
	}

	return port;
}

rem_port* INET_connect(const TEXT* name,
					   PACKET* packet,
					   ISC_STATUS* status_vector,
					   USHORT flag, Firebird::ClumpletReader* dpb)
{
/**************************************
 *
 *	I N E T _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Establish half of a communication link.  If a connect packet is given,
 *	the connection is on behalf of a remote interface.  Otherwise the connect
 *	is for a server process.
 *
 **************************************/
#ifdef DEBUG
	{
		if (INET_trace & TRACE_operations)
		{
			fprintf(stdout, "INET_connect\n");
			fflush(stdout);
		}
		INET_start_time = inet_debug_timer();
		// CVC: I don't see the point in replacing this with fb_utils::readenv().
		const char* p = getenv("INET_force_error");
		if (p != NULL) {
			INET_force_error = atoi(p);
		}
	}
#endif

	rem_port* const port = alloc_port(NULL);
	port->port_status_vector = status_vector;
	REMOTE_get_timeout_params(port, dpb);
	status_vector[0] = isc_arg_gds;
	status_vector[1] = 0;
	status_vector[2] = isc_arg_end;

	Firebird::string host;
	Firebird::string protocol;

	if (name)
	{
		host = name;
		const size_t pos = host.find("/");
		if (pos != Firebird::string::npos)
		{
			protocol = host.substr(pos + 1);
			host = host.substr(0, pos);
		}
	}

	if (host.hasData())
	{
		delete port->port_connection;
		port->port_connection = REMOTE_make_string(host.c_str());
	}
	else {
		host = port->port_host->str_data;
	}

	if (protocol.isEmpty())
	{
		const unsigned short port2 = Config::getRemoteServicePort();
		if (port2) {
			protocol.printf("%hu", port2);
		}
		else {
			protocol = Config::getRemoteServiceName();
		}
	}

	// Set up Inter-Net socket address

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));

	// U N I X style sockets

	address.sin_family = AF_INET;

	// define maximum numbers of addresses for a host that we can handle
	const int MAX_HOST_ADDRESS_NUMBER = 8;

	in_addr host_addr;
	in_addr host_addr_arr[MAX_HOST_ADDRESS_NUMBER];
	int hostAddressNumber = 0;

	if (packet)
	{
		// client connection
		hostAddressNumber = get_host_address(host.c_str(), host_addr_arr, MAX_HOST_ADDRESS_NUMBER);
		if (hostAddressNumber > MAX_HOST_ADDRESS_NUMBER)
		{
			hostAddressNumber = MAX_HOST_ADDRESS_NUMBER;
		}

		if (! hostAddressNumber)
		{
			gds__log("INET/INET_connect: gethostbyname (%s) failed, error code = %d",
					 host.c_str(), H_ERRNO);
			inet_gen_error(port, Arg::Gds(isc_net_lookup_err) << Arg::Gds(isc_host_unknown));

			disconnect(port);
			return NULL;
		}
		host_addr = host_addr_arr[0];
	}
	else
	{
		// server connection
		host_addr = get_bind_address();
	}

	const struct servent* service = getservbyname(protocol.c_str(), "tcp");
#ifdef WIN_NT
	// On Windows NT/9x, getservbyname can only accomodate
	// 1 call at a time.  In this case it returns the error
	// WSAEINPROGRESS.
	// If this happens, retry the operation a few times.
	// NOTE: This still does not guarantee success, but helps.
	if (!service)
	{
		for (int retry = 0; H_ERRNO == INET_RETRY_ERRNO && retry < INET_RETRY_CALL; retry++)
		{
			if ( (service = getservbyname(protocol.c_str(), "tcp")) )
				break;
		}
	}
#endif // WIN_NT

	// Make sure getservbyname returns the protocol we searched for.
	// See also bug CORE-3819.

	if (service && fb_utils::stricmp(service->s_name, protocol.c_str()))
		service = NULL;

	// Modification by luz (slightly modified by FSG)
	// instead of failing here, try applying hard-wired
	// translation of "gds_db" into "3050"
	// This way, a connection to a remote FB server
	// works even from clients with missing "gds_db"
	// entry in "services" file, which is important
	// for zero-installation clients.

	if (!service)
	{
		if (protocol == FB_SERVICE_NAME)
		{
			// apply hardwired translation
			address.sin_port = htons(FB_SERVICE_PORT);
		}
		// modification by FSG 23.MAR.2001
		else
		{
			// modification by FSG 23.MAR.2001
			// The user has supplied something as protocol
			// let's see whether this is a port number
			// instead of a service name
			address.sin_port = htons(atoi(protocol.c_str()));
		}

		if (address.sin_port == 0)
		{
			// end of modification by FSG
			// this is the original code
			gds__log("INET/INET_connect: getservbyname failed, error code = %d", H_ERRNO);
			inet_gen_error(port, Arg::Gds(isc_net_lookup_err) <<
						   Arg::Gds(isc_service_unknown) << Arg::Str(protocol) << Arg::Str("tcp"));
			return NULL;
		}						// else / not hardwired gds_db translation
	}
	else
	{
		// if we have got a service-struct, get port number from there
		// (in case of hardwired gds_db to 3050 translation, address.sin_port was
		// already set above
		address.sin_port = service->s_port;
	}							// else (service found)

	// end of modifications by luz

	// Allocate a port block and initialize a socket for communications

	port->port_handle = socket(AF_INET, SOCK_STREAM, 0);

	if (port->port_handle == INVALID_SOCKET)
	{
		inet_error(port, "socket", isc_net_connect_err, INET_ERRNO);
		disconnect(port);
		return NULL;
	}

	// If we're a host, just make the connection

    int n;

	if (packet)
	{
		int optval = 1;
		n = setsockopt(port->port_handle, SOL_SOCKET, SO_KEEPALIVE, (SCHAR*) &optval, sizeof(optval));

		if (n == -1)
		{
			gds__log("setsockopt: error setting SO_KEEPALIVE");
		}

		if (! setNoNagleOption(port))
		{
			inet_error(port, "setsockopt TCP_NODELAY", isc_net_connect_err, INET_ERRNO);
			disconnect(port);
			return NULL;
		}

		int inetErrNo = 0;
		for (int i = 0; i < hostAddressNumber; i++)
		{
			address.sin_addr = host_addr_arr[i];

			// If host has two addresses and the first one failed,
			// but the second one succeeded - no need to worry

			n = connect(port->port_handle, (struct sockaddr*) &address, sizeof(address));
			inetErrNo = INET_ERRNO;

			if (n != -1 && send_full(port, packet))
				return port;
		}
		inet_error(port, "connect", isc_net_connect_err, inetErrNo);
		disconnect(port);
		return NULL;
	}

	// We're a server, so wait for a host to show up

	memcpy(&address.sin_addr, &host_addr, sizeof(address.sin_addr));

	if (flag & SRVR_multi_client)
	{
		struct linger lingerInfo;

		lingerInfo.l_onoff = 0;
		lingerInfo.l_linger = 0;

#ifndef WIN_NT
		// dimitr:	on Windows, lack of SO_REUSEADDR works the same way as it was specified on POSIX,
		//			i.e. it allows binding to a port in a TIME_WAIT/FIN_WAIT state. If this option
		//			is turned on explicitly, then a port can be re-bound regardless of its state,
		//			e.g. while it's listening. This is surely not what we want.

		int optval = TRUE;
		n = setsockopt(port->port_handle, SOL_SOCKET, SO_REUSEADDR,
					   (SCHAR*) &optval, sizeof(optval));
		if (n == -1)
		{
			inet_error(port, "setsockopt REUSE", isc_net_connect_listen_err, INET_ERRNO);
			disconnect(port);
			return NULL;
		}
#endif

		// Get any values for SO_LINGER so that they can be reset during
		// disconnect.  SO_LINGER should be set by default on the socket

		socklen_t optlen = sizeof(port->port_linger);
		n = getsockopt(port->port_handle, SOL_SOCKET, SO_LINGER,
					   (SCHAR *) & port->port_linger, &optlen);

		if (n != 0)				// getsockopt failed
			port->port_linger.l_onoff = 0;

		n = setsockopt(port->port_handle, SOL_SOCKET, SO_LINGER,
					   (SCHAR *) & lingerInfo, sizeof(lingerInfo));
		if (n == -1)
		{
			inet_error(port, "setsockopt LINGER", isc_net_connect_listen_err, INET_ERRNO);
			disconnect(port);
			return NULL;
		}

		if (! setNoNagleOption(port))
		{
			inet_error(port, "setsockopt TCP_NODELAY", isc_net_connect_listen_err, INET_ERRNO);
			disconnect(port);
			return NULL;
		}
	}

	n = bind(port->port_handle, (struct sockaddr*) &address, sizeof(address));

	if (n == -1)
	{
		// On Linux platform, when the server dies the system holds a port
		// for some time.

		for (int retry = 0; INET_ERRNO == INET_ADDR_IN_USE && retry < INET_RETRY_CALL; retry++)
		{
			sleep(10);
			n = bind(port->port_handle, (struct sockaddr *) &address, sizeof(address));
			if (n == 0)
				break;
		}
	}

	if (n == -1)
	{
		inet_error(port, "bind", isc_net_connect_listen_err, INET_ERRNO);
		disconnect(port);
		return NULL;
	}

	n = listen(port->port_handle, SOMAXCONN);

	if (n == -1)
	{
		inet_error(port, "listen", isc_net_connect_listen_err, INET_ERRNO);
		return NULL;
	}

	inet_ports->registerPort(port);

	if (flag & SRVR_multi_client)
	{
		// Prevent the generation of dummy keepalive packets on the connect port.

		port->port_dummy_packet_interval = 0;
		port->port_dummy_timeout = 0;
		port->port_server_flags |= (SRVR_server | SRVR_multi_client);

		return port;
	}

	while (true)
	{
		socklen_t l = sizeof(address);
		SOCKET s = accept(port->port_handle, (struct sockaddr*) &address, &l);
		const int inetErrNo = INET_ERRNO;
		if (s == INVALID_SOCKET)
		{
			if (!INET_shutting_down)
			{
				inet_error(port, "accept", isc_net_connect_err, inetErrNo);
				disconnect(port);
			}
			return NULL;
		}
#ifdef WIN_NT
		if (flag & SRVR_debug)
#else
		if ((flag & SRVR_debug) || !fork())
#endif
		{
			SOCLOSE(port->port_handle);
			port->port_handle = s;
			port->port_server_flags |= SRVR_server | SRVR_debug;
			port->port_flags |= PORT_server;
			return port;
		}

#ifdef WIN_NT
		Firebird::MutexLockGuard forkGuard(forkMutex);
		if (!forkThreadStarted)
		{
			forkThreadStarted = true;
			forkEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			forkSockets = new SocketsArray(*getDefaultMemoryPool());

			gds__thread_start(forkThread, (void*) flag, THREAD_medium, 0, 0);
		}
		forkSockets->add(s);
		SetEvent(forkEvent);
#else
		Firebird::MutexLockGuard guard(waitThreadMutex);
		if (! procCount++) {
			gds__thread_start(waitThread, 0, THREAD_medium, 0, 0);
		}

		SOCLOSE(s);
#endif
	}

#ifdef WIN_NT
	Firebird::MutexLockGuard forkGuard(forkMutex);
	if (forkThreadStarted)
	{
		SetEvent(forkEvent);
		CloseHandle(forkEvent);

		delete forkSockets;
		forkSockets = NULL;
	}
#endif
}


rem_port* INET_reconnect(SOCKET handle, ISC_STATUS* status_vector)
{
/**************************************
 *
 *	I N E T _ r e c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	A communications link has been established by another
 *	process.  We have inheritted the handle.  Set up
 *	a port block.
 *
 **************************************/
	rem_port* const port = alloc_port(NULL);
	port->port_status_vector = status_vector;
	status_vector[0] = isc_arg_gds;
	status_vector[1] = 0;
	status_vector[2] = isc_arg_end;

	port->port_handle = handle;
	port->port_flags |= PORT_server;
	port->port_server_flags |= SRVR_server;

	int n = 0, optval = TRUE;
	n = setsockopt(port->port_handle, SOL_SOCKET, SO_KEEPALIVE, (SCHAR*) &optval, sizeof(optval));
	if (n == -1) {
		gds__log("inet server err: setting KEEPALIVE socket option \n");
	}

	if (! setNoNagleOption(port)) {
		gds__log("inet server err: setting NODELAY socket option \n");
	}

	return port;
}

rem_port* INET_server(SOCKET sock)
{
/**************************************
 *
 *	I N E T _ s e r v e r
 *
 **************************************
 *
 * Functional description
 *	We have been spawned by a master server with a connection
 *	established.  Set up port block with the appropriate socket.
 *
 **************************************/
	int n = 0;
	rem_port* const port = alloc_port(NULL);
	port->port_flags |= PORT_server;
	port->port_server_flags |= SRVR_server;
	port->port_handle = sock;

	int optval = 1;
	n = setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (SCHAR*) &optval, sizeof(optval));
	if (n == -1) {
		gds__log("inet server err: setting KEEPALIVE socket option \n");
	}

	if (! setNoNagleOption(port)) {
		gds__log("inet server err: setting NODELAY socket option \n");
	}

	return port;
}

void INET_set_clients( int count)
{
/**************************************
 *
 *	I N E T _ s e t _ c l i e n t s
 *
 **************************************
 *
 * Functional description
 *	Set maxinum number of clients served before
 *	starting new server
 *
 **************************************/
	INET_max_clients = (count && count < MAXCLIENTS) ? count : MAXCLIENTS;
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
 *	Accept an incoming request for connection.  This is purely a lower
 *	level handshaking function, and does not constitute the server
 *	response for protocol selection.
 *
 **************************************/
	// Default account to "guest" (in theory all packets contain a name)

	Firebird::string name("guest"), password;

	// Pick up account and password, if given

	Firebird::ClumpletReader id(Firebird::ClumpletReader::UnTagged,
								cnct->p_cnct_user_id.cstr_address,
								cnct->p_cnct_user_id.cstr_length);

	SLONG eff_gid = -1, eff_uid = -1;
	bool user_verification = false;
	for (id.rewind(); !id.isEof(); id.moveNext())
	{
		switch (id.getClumpTag())
		{
		case CNCT_user:
			id.getString(name);
			break;

		case CNCT_passwd:
			id.getString(password);
			break;

		case CNCT_group:
			{
				const size_t length = id.getClumpLength();
				if (length <= sizeof(eff_gid) && length > 0)
				{
					eff_gid = 0;
					memcpy(&eff_gid, id.getBytes(), length);
					eff_gid = ntohl(eff_gid);
				}
				break;
			}

			// this case indicates that the client has requested that
			// we force the user name/password to be verified against
			// the security database

		case CNCT_user_verification:
			user_verification = true;
			break;
		}
	}

	// See if user exists.  If not, reject connection
	if (user_verification)
	{
		eff_gid = eff_uid = -1;
		// port->port_flags |= PORT_not_trusted; // never tested
	}

#ifndef WIN_NT
	else
	{
		if (!check_host(port))
		{
			return false;
		}
	}

	{ // scope
		// If the environment variable ISC_INET_SERVER_HOME is set,
		// change the home directory to the specified directory.
		// Note that this will overrule the normal setting of
		// the current directory to the effective user's home directory.
		// This feature was added primarily for testing via remote
		// loopback - but does seem to be of good general use, so
		// is activated for the release version.
		// 1995-February-27 David Schnepper

		Firebird::PathName home;
		if (fb_utils::readenv("ISC_INET_SERVER_HOME", home))
		{
			if (chdir(home.c_str())) {
				gds__log("inet_server: unable to cd to %s errno %d\n", home.c_str(), INET_ERRNO);
				// We continue after the error
			}
		}
	} // end scope
#endif // !WIN_NT

	// store FULL user identity in port_user_name for security purposes

	Firebird::string temp;
	temp.printf("%s.%ld.%ld", name.c_str(), eff_gid, eff_uid);
	port->port_user_name = REMOTE_make_string(temp.c_str());

	port->port_protocol_str = REMOTE_make_string("TCPv4");

	struct sockaddr_in address;
	socklen_t l = sizeof(address);

	memset(&address, 0, sizeof(address));
	int status = getpeername(port->port_handle, (struct sockaddr *) &address, &l);
	if (status == 0)
	{
		Firebird::string addr_str;
		const UCHAR* ip = (UCHAR*) &address.sin_addr;
		addr_str.printf(
			"%d.%d.%d.%d",
			static_cast<int>(ip[0]),
			static_cast<int>(ip[1]),
			static_cast<int>(ip[2]),
			static_cast<int>(ip[3]) );
		port->port_address_str = REMOTE_make_string(addr_str.c_str());
	}

	return true;
}


static rem_port* alloc_port(rem_port* const parent, const USHORT flags)
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

	if (!INET_initialized)
	{
		Firebird::MutexLockGuard guard(init_mutex);
		if (!INET_initialized)
		{
#ifdef WIN_NT
			static WSADATA wsadata;
			const WORD version = MAKEWORD(2, 0);
			const int wsaError = WSAStartup(version, &wsadata);
			if (wsaError)
			{
				if (parent)
					inet_error(parent, "WSAStartup", isc_net_init_error, wsaError);
				else {
					gds__log("INET/alloc_port: WSAStartup failed, error code = %d", wsaError);
				}
				return NULL;
			}
			gds__register_cleanup(wsaExitHandler, 0);
#endif
			INET_remote_buffer = Config::getTcpRemoteBufferSize();
			if (INET_remote_buffer < MAX_DATA_LW || INET_remote_buffer > MAX_DATA_HW)
			{
				INET_remote_buffer = DEF_MAX_DATA;
			}
#ifdef DEBUG
			gds__log(" Info: Remote Buffer Size set to %ld", INET_remote_buffer);
#endif

			fb_shutdown_callback(0, cleanup_ports, fb_shut_postproviders, 0);

			INET_initialized = true;

			// This should go AFTER 'INET_initialized = true' to avoid recursion
			inet_async_receive = alloc_port(0);
			inet_async_receive->port_flags |= PORT_server;
		}
	}

	rem_port* const port = new rem_port(rem_port::INET, INET_remote_buffer * 2);
	REMOTE_get_timeout_params(port, 0);

	TEXT buffer[BUFFER_SMALL];
	gethostname(buffer, sizeof(buffer));

	port->port_host = REMOTE_make_string(buffer);
	port->port_connection = REMOTE_make_string(buffer);
	SNPRINTF(buffer, FB_NELEM(buffer), "tcp (%s)", port->port_host->str_data);
	port->port_version = REMOTE_make_string(buffer);

	port->port_accept = accept_connection;
	port->port_disconnect = disconnect;
	port->port_force_close = force_close;
	port->port_receive_packet = receive;
	port->port_select_multi = select_multi;
	port->port_send_packet = send_full;
	port->port_send_partial = send_partial;
	port->port_connect = aux_connect;
	port->port_request = aux_request;
	port->port_buff_size = (USHORT) INET_remote_buffer;
	port->port_async_receive = inet_async_receive;
	port->port_flags = flags;

	xdrinet_create(	&port->port_send, port,
					&port->port_buffer[INET_remote_buffer],
					(USHORT) INET_remote_buffer,
					XDR_ENCODE);

	xdrinet_create(	&port->port_receive, port, port->port_buffer, 0, XDR_DECODE);

	if (parent && !(parent->port_server_flags & SRVR_thread_per_port))
	{
		Firebird::MutexLockGuard guard(port_mutex);
		port->linkParent(parent);
	}

	return port;
}

static rem_port* aux_connect(rem_port* port, PACKET* packet)
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
	struct sockaddr_in address;
	socklen_t l = sizeof(address);

	// If this is a server, we're got an auxiliary connection.  Accept it

	if (port->port_server_flags)
	{
		struct timeval timeout;
		timeout.tv_sec = port->port_connect_timeout;
		timeout.tv_usec = 0;

		Select slct;
		slct.set(port->port_channel);

		int inetErrNo = 0;

		while (true)
		{
			slct.select(&timeout);
			const int count = slct.getCount();
			inetErrNo = INET_ERRNO;

			if (count != -1 || !INTERRUPT_ERROR(inetErrNo))
			{
				if (count == 1)
				{
					break;
				}
				else
				{
					const ISC_STATUS error_code =
						(count == 0) ? isc_net_event_connect_timeout : isc_net_event_connect_err;
					inet_error(port, "select", error_code, inetErrNo);
					SOCLOSE(port->port_channel);
					return NULL;
				}
			}
		}

		const SOCKET n = accept(port->port_channel, (struct sockaddr*) &address, &l);
		inetErrNo = INET_ERRNO;

		if (n == INVALID_SOCKET)
		{
			inet_error(port, "accept", isc_net_event_connect_err, inetErrNo);
			SOCLOSE(port->port_channel);
			return NULL;
		}

		SOCLOSE(port->port_channel);
		port->port_handle = n;
		port->port_flags |= PORT_async;
		return port;
	}

	rem_port* const new_port = alloc_port(port->port_parent);
	port->port_async = new_port;
	new_port->port_dummy_packet_interval = port->port_dummy_packet_interval;
	new_port->port_dummy_timeout = new_port->port_dummy_packet_interval;
	new_port->port_flags = port->port_flags & PORT_no_oob;
	new_port->port_flags |= PORT_async;
	P_RESP* response = &packet->p_resp;

	// Set up new socket

	SOCKET n = socket(AF_INET, SOCK_STREAM, 0);
	if (n == INVALID_SOCKET)
	{
		inet_error(port, "socket", isc_net_event_connect_err, INET_ERRNO);
		return NULL;
	}

	// NJK - Determine address and port to use.
	//
	// The address returned by the server may be incorrect if it is behind a NAT box
	// so we must use the address that was used to connect the main socket, not the
	// address reported by the server.
	//
	// The port number reported by the server is used. For NAT support the port number
	// should be configured to be a fixed port number in the server configuration.

	memset(&address, 0, sizeof(address));
	int status = getpeername(port->port_handle, (struct sockaddr *) &address, &l);
	if (status != 0)
	{
		inet_error(port, "socket", isc_net_event_connect_err, INET_ERRNO);
		SOCLOSE(n);
		return NULL;
	}
	address.sin_family = AF_INET;
	address.sin_port = ((struct sockaddr_in *)(response->p_resp_data.cstr_address))->sin_port;

	int optval = 1;
	setsockopt(n, SOL_SOCKET, SO_KEEPALIVE, (SCHAR*) &optval, sizeof(optval));

	status = connect(n, (struct sockaddr *) &address, sizeof(address));
	const int inetErrNo = INET_ERRNO;

	if (status < 0)
	{
		inet_error(port, "connect", isc_net_event_connect_err, inetErrNo);
		SOCLOSE(n);
		return NULL;
	}

	new_port->port_handle = n;

	return new_port;
}

static rem_port* aux_request( rem_port* port, PACKET* packet)
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
 *
 **************************************/
	struct sockaddr_in address;

	// Set up new socket

	address.sin_family = AF_INET;
	in_addr bind_addr = get_bind_address();
	memcpy(&address.sin_addr, &bind_addr, sizeof(address.sin_addr));
	address.sin_port = htons(Config::getRemoteAuxPort());

	SOCKET n = socket(AF_INET, SOCK_STREAM, 0);
	if (n == INVALID_SOCKET)
	{
		inet_error(port, "socket", isc_net_event_listen_err, INET_ERRNO);
		return NULL;
	}

#ifndef WIN_NT
	// dimitr:	on Windows, lack of SO_REUSEADDR works the same way as it was specified on POSIX,
	//			i.e. it allows binding to a port in a TIME_WAIT/FIN_WAIT state. If this option
	//			is turned on explicitly, then a port can be re-bound regardless of its state,
	//			e.g. while it's listening. This is surely not what we want.

	int optval = TRUE;
	if (setsockopt(n, SOL_SOCKET, SO_REUSEADDR, (SCHAR*) &optval, sizeof(optval)) < 0)
	{
		inet_error(port, "setsockopt REUSE", isc_net_event_listen_err, INET_ERRNO);
		return NULL;
	}
#endif

	if (bind(n, (struct sockaddr *) &address, sizeof(address)) < 0)
	{
		inet_error(port, "bind", isc_net_event_listen_err, INET_ERRNO);
		return NULL;
	}

	socklen_t length = sizeof(address);

	if (getsockname(n, (struct sockaddr *) &address, &length) < 0)
	{
		inet_error(port, "getsockname", isc_net_event_listen_err, INET_ERRNO);
		return NULL;
	}

	if (listen(n, 1) < 0)
	{
		inet_error(port, "listen", isc_net_event_listen_err, INET_ERRNO);
		return NULL;
	}

    rem_port* const new_port = alloc_port(port->port_parent, PORT_async);
	port->port_async = new_port;
	new_port->port_dummy_packet_interval = port->port_dummy_packet_interval;
	new_port->port_dummy_timeout = new_port->port_dummy_packet_interval;

	new_port->port_server_flags = port->port_server_flags;
	new_port->port_channel = (int) n;
	new_port->port_flags |= port->port_flags & PORT_no_oob;

	P_RESP* response = &packet->p_resp;

	struct sockaddr_in port_address;
	if (getsockname(port->port_handle, (struct sockaddr *) &port_address, &length) < 0)
	{
		inet_error(port, "getsockname", isc_net_event_listen_err, INET_ERRNO);
		return NULL;
	}
	memcpy(&address.sin_addr, &port_address.sin_addr, sizeof(address.sin_addr));

	response->p_resp_data.cstr_length = sizeof(address);
	memcpy(response->p_resp_data.cstr_address, &address, sizeof(address));

	return new_port;
}

#ifndef WIN_NT
static bool check_host(rem_port* port)
{
/**************************************
 *
 *	c h e c k _ h o s t 	 ( n o n - V M S )
 *
 **************************************
 *
 * Functional description
 *	Check the host on the other end of the socket to see if it's localhost
 *
 **************************************/
	struct sockaddr_in address;

	socklen_t length = sizeof(address);

	if (getpeername(port->port_handle, (struct sockaddr*) &address, &length) == -1)
		return false;

	// If source address is in the loopback net - trust it
	return (ntohl(address.sin_addr.s_addr) >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET;
}
#endif // WIN_NT

#if !(defined WIN_NT)
static THREAD_ENTRY_DECLARE waitThread(THREAD_ENTRY_PARAM)
{
/**************************************
 *
 *	w a i t T h r e a d
 *
 **************************************
 *
 * Functional description
 *	Waits for processes started by standalone classic server (avoid zombies)
 *
 **************************************/
	while (procCount > 0)
	{
		int rc = wait(0);

		Firebird::MutexLockGuard guard(waitThreadMutex);
		if (rc > 0) {
			--procCount;
		}
	}

	return 0;
}
#endif // !defined(WIN_NT)

static void disconnect(rem_port* const port)
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

	// SO_LINGER was turned off on the initial bind when the server was started.
	// This will force a reset to be sent to the client when the socket is closed.
	// We only want this behavior in the case of the server terminating
	// abnormally and not on an orderly shut down.  Because of this, turn the
	// SO_LINGER option back on for the socket.  The result of setsockopt isn't
	// too important at this stage since we are closing the socket anyway.  This
	// is an attempt to return the socket to a state where a graceful shutdown can
	// occur.

	if (port->port_linger.l_onoff)
	{
		setsockopt(port->port_handle, SOL_SOCKET, SO_LINGER,
				   (SCHAR*) &port->port_linger, sizeof(port->port_linger));
	}

	if (port->port_handle != INVALID_SOCKET)
	{
		shutdown(port->port_handle, 2);
	}

	Firebird::MutexLockGuard guard(port_mutex);
	port->port_state = rem_port::DISCONNECTED;

	if (port->port_async)
	{
		disconnect(port->port_async);
		port->port_async = NULL;
	}

	// If this is a sub-port, unlink it from its parent
	port->unlinkParent();

	inet_ports->unRegisterPort(port);

	SOCLOSE(port->port_handle);
	SOCLOSE(port->port_channel);

	port->release();

#ifdef DEBUG
	if (INET_trace & TRACE_summary)
	{
		fprintf(stdout, "INET_count_send = %u packets\n", INET_count_send);
		fprintf(stdout, "INET_bytes_send = %u bytes\n", INET_bytes_send);
		fprintf(stdout, "INET_count_recv = %u packets\n", INET_count_recv);
		fprintf(stdout, "INET_bytes_recv = %u bytes\n", INET_bytes_recv);
		fflush(stdout);
	}
#endif

	return;
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
 *	Forcebly close remote connection.
 *
 **************************************/

	if (port->port_state != rem_port::PENDING)
		return;

	port->port_state = rem_port::BROKEN;

	if (port->port_handle != INVALID_SOCKET)
	{
		shutdown(port->port_handle, 2);
		SOCLOSE(port->port_handle);
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
	INET_shutting_down = true;

	inet_ports->closePorts();
	return 0;
}


#ifdef NO_FORK
static int fork()
{
/**************************************
 *
 *	f o r k		( N O _ F O R K )
 *
 **************************************
 *
 * Functional description
 *	Hmmm.
 *
 **************************************/

	return 1;
}
#endif

#ifdef WIN_NT
static void wsaExitHandler(void*)
{
/**************************************
 *
 *	w s a E x i t H a n d l e r
 *
 **************************************
 *
 * Functional description
 *	Cleanup WSA.
 *
 **************************************/
	SleepEx(0, FALSE);	// let select in other thread(s) shutdown gracefully
	WSACleanup();
}


static int fork(SOCKET old_handle, USHORT flag)
{
/**************************************
 *
 *	f o r k		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Create a child process.
 *
 **************************************/
	TEXT name[MAXPATHLEN];
	GetModuleFileName(NULL, name, sizeof(name));

	HANDLE new_handle;
	if (!DuplicateHandle(GetCurrentProcess(), (HANDLE) old_handle,
						 GetCurrentProcess(), &new_handle, 
						 0, TRUE, DUPLICATE_SAME_ACCESS))
	{
		gds__log("INET/inet_error: fork/DuplicateHandle errno = %d", GetLastError());
		return 0;
	}

	Firebird::string cmdLine;
	cmdLine.printf("%s -i -h %"HANDLEFORMAT"@%"ULONGFORMAT, name, new_handle, GetCurrentProcessId());

	STARTUPINFO start_crud;
	start_crud.cb = sizeof(STARTUPINFO);
	start_crud.lpReserved = NULL;
	start_crud.lpReserved2 = NULL;
	start_crud.cbReserved2 = 0;
	start_crud.lpDesktop = NULL;
	start_crud.lpTitle = NULL;
	start_crud.dwFlags = STARTF_FORCEOFFFEEDBACK;

	PROCESS_INFORMATION pi;
	if (CreateProcess(NULL, cmdLine.begin(), NULL, NULL, FALSE,
					  (flag & SRVR_high_priority ?
						 HIGH_PRIORITY_CLASS | DETACHED_PROCESS :
						 NORMAL_PRIORITY_CLASS | DETACHED_PROCESS),
					  NULL, NULL, &start_crud, &pi))
	{
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		// hvlad: child process will close our handle of just accepted socket
		return 1;
	}

	gds__log("INET/inet_error: fork/CreateProcess errno = %d", GetLastError());
	CloseHandle(new_handle);
	return 0;
}


THREAD_ENTRY_DECLARE forkThread(THREAD_ENTRY_PARAM arg)
{
	const USHORT flag = (USHORT) arg;

	while (!INET_shutting_down)
	{
		if (WaitForSingleObject(forkEvent, INFINITE) != WAIT_OBJECT_0)
			break;

		while (!INET_shutting_down)
		{
			SOCKET s = 0;
			{	// scope
				Firebird::MutexLockGuard forkGuard(forkMutex);

				if (!forkSockets || forkSockets->getCount() == 0)
					break;

				s = (*forkSockets)[0];
				forkSockets->remove((size_t) 0);
			}
			fork(s, flag);
			SOCLOSE(s);
		}
	}

	return 0;
}
#endif

namespace
{
	in_addr config_address;

	class GetAddress
	{
	public:
		static void init()
		{
			const char* config_option = Config::getRemoteBindAddress();
			if (config_option)
			{
				int n = get_host_address(config_option, &config_address, 1);
				if (n != 1)
				{
					// In case when config option is given with error,
					// bind to loopback interface only
					config_address.s_addr = htonl(INADDR_LOOPBACK);
					// log warning
					if (n == 0)
					{
						gds__log("Wrong RemoteBindAddress '%s' in firebird.conf - "
								 "binding to loopback interface", config_option);
					}
					else
					{
						gds__log("Host '%s' resolves to multiple interfaces - "
								 "binding to loopback interface", config_option);
					}
				}
			}
			else	// use default to listen all
			{
				config_address.s_addr = INADDR_ANY;
			}
		}

		static void cleanup() { }
	};
}

static in_addr get_bind_address()
{
/**************************************
 *
 *	g e t _ b i n d _ a d d r e s s
 *
 **************************************
 *
 * Functional description
 *	Return local address to bind sockets to.
 *
 **************************************/
	static Firebird::InitMutex<GetAddress> instance;

	instance.init();

	return config_address;
}


#ifdef WIN_NT
// Windows does not have an inet_aton function.
bool inet_aton(const char* name, in_addr* address)
{
	address->s_addr = inet_addr(name);
	return address->s_addr != INADDR_NONE;
}
#endif


static int get_host_address(const char* name,
							in_addr* const host_addr_arr,
							const int arr_size)
{
/**************************************
 *
 *	g e t _ h o s t _ a d d r e s s
 *
 **************************************
 *
 * Functional description
 *  Fills array with addresses up to arr_size (must be at least 1).
 *	Returns the required number of elements in array to be able to store
 *	all host addresses (may be less, equal or greater than arr_size).
 *
 **************************************/
	if (inet_aton(name, &host_addr_arr[0]))
	{
		return 1;
	}

	const hostent* host = gethostbyname(name);

	// On Windows NT/9x, gethostbyname can only accomodate
	// 1 call at a time.  In this case it returns the error
	// WSAEINPROGRESS. On UNIX systems, this call may not succeed
	// because of a temporary error.  In this case, it returns
	// h_error set to TRY_AGAIN.  When these errors occur,
	// retry the operation a few times.
	// NOTE: This still does not guarantee success, but helps.

	if (!host)
	{
		for (int retry = 0; H_ERRNO == INET_RETRY_ERRNO && retry < INET_RETRY_CALL; retry++)
		{
			if ( (host = gethostbyname(name)) )
				break;
		}
	}

	// We can't work with other types for now. Maybe AF_NETBIOS for MS, too?
	if (host && host->h_addrtype == AF_INET)
	{
		const in_addr* const* list = reinterpret_cast<in_addr**>(host->h_addr_list);
		int i = 0;
		while (list[i] != NULL)
		{
			if (i < arr_size)
			{
				host_addr_arr[i] = *list[i];
			}
			++i;
		}
		return i;
	}

	// give up
	return 0;
}

//____________________________________________________________
//
//	Copy an array of p_cnct::p_cnct_repeat.
//
static void copy_p_cnct_repeat_array(	p_cnct::p_cnct_repeat*			pDest,
										const p_cnct::p_cnct_repeat*	pSource,
										size_t							nEntries)
{
	for (size_t i = 0; i < nEntries; ++i) {
		pDest[i] = pSource[i];
	}
}


static rem_port* receive( rem_port* main_port, PACKET * packet)
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

	// loop as long as we are receiving dummy packets, just
	// throwing them away--note that if we are a server we won't
	// be receiving them, but it is better to check for them at
	// this level rather than try to catch them in all places where
	// this routine is called

	do {
		if (!xdr_protocol(&main_port->port_receive, packet))
		{
			packet->p_operation = main_port->port_flags & PORT_partial_data ? op_partial : op_exit;
			main_port->port_flags &= ~PORT_partial_data;

			if (packet->p_operation == op_exit) {
				main_port->port_state = rem_port::BROKEN;
			}
			break;
		}
#ifdef DEBUG
		{
			static ULONG op_rec_count = 0;
			op_rec_count++;
			if (INET_trace & TRACE_operations)
			{
				fprintf(stdout, "%04u: OP Recd %5u opcode %d\n",
						   inet_debug_timer(),
						   op_rec_count, packet->p_operation);
				fflush(stdout);
			}
		}
#endif
	} while (packet->p_operation == op_dummy);

	return main_port;
}

static bool select_multi(rem_port* main_port, UCHAR* buffer, SSHORT bufsize, SSHORT* length,
						 RemPortPtr& port)
{
/**************************************
 *
 *	s e l e c t _ m u l t i
 *
 **************************************
 *
 * Functional description
 *	Receive an IP packet from a port or clients of a port.
 *  Used only by the multiclient server on main server's port.
 *	If a connection request comes in, generate a new port
 *	block for the client.
 *
 **************************************/

	for (;;)
	{
		select_port(main_port, &INET_select, port);
		if (port == main_port && (port->port_server_flags & SRVR_multi_client))
		{
			if (INET_shutting_down)
			{
				if (main_port->port_state != rem_port::BROKEN)
				{
					main_port->port_state = rem_port::BROKEN;

					shutdown(main_port->port_handle, 2);
					SOCLOSE(main_port->port_handle);
				}
			}
			else if (port = select_accept(main_port))
			{
				if (!packet_receive(port, buffer, bufsize, length))
				{
					*length = 0;
				}
				return (*length) ? true : false;
			}

			continue;
		}
		if (port)
		{
			if (port->port_dummy_timeout < 0)
			{
				port->port_dummy_timeout = port->port_dummy_packet_interval;
				if (port->port_flags & PORT_async || port->port_protocol < PROTOCOL_VERSION8)
				{
					continue;
				}
				*length = 0;
				return true;
			}

			if (!packet_receive(port, buffer, bufsize, length))
			{
				if (port->port_flags & PORT_disconnect) {
					continue;
				}
				*length = 0;
			}
			return (*length) ? true : false;
		}
		if (!select_wait(main_port, &INET_select))
		{
			port = NULL;
			return false;
		}
	}
}

static rem_port* select_accept( rem_port* main_port)
{
/**************************************
 *
 *	s e l e c t _ a c c e p t
 *
 **************************************
 *
 * Functional description
 *	Accept a new connection request.
 *
 **************************************/
	struct sockaddr_in address;

	rem_port* const port = alloc_port(main_port);
	socklen_t l = sizeof(address);
	inet_ports->registerPort(port);

	port->port_handle = accept(main_port->port_handle, (struct sockaddr*) &address, &l);
	if (port->port_handle == INVALID_SOCKET)
	{
		inet_error(port, "accept", isc_net_connect_err, INET_ERRNO);
		disconnect(port);
		return 0;
	}

	int optval = 1;
	setsockopt(port->port_handle, SOL_SOCKET, SO_KEEPALIVE, (SCHAR*) &optval, sizeof(optval));

	port->port_flags |= PORT_server;

	if (main_port->port_server_flags & SRVR_thread_per_port)
	{
		port->port_server_flags = (SRVR_server | SRVR_inet | SRVR_thread_per_port);
		return port;
	}

	return 0;
}

static void select_port(rem_port* main_port, Select* selct, RemPortPtr& port)
{
/**************************************
 *
 *	s e l e c t _ p o r t
 *
 **************************************
 *
 * Functional description
 *	Select a descriptor that is ready to read
 *	and return the port block. Additionally,
 *	check if a port's keepalive timer has
 *	expired and return the port block so that
 *	a keepalive packet can be queued. Return
 *	NULL if	none are active.
 *
 **************************************/

	Firebird::MutexLockGuard guard(port_mutex);

	for (port = main_port; port; port = port->port_next)
	{
		Select::HandleState result = selct->ok(port);
		selct->unset(port->port_handle);
		switch (result)
		{
		case Select::SEL_BAD:
			if (port->port_state == rem_port::BROKEN)
				continue;
			return;

		case Select::SEL_DISCONNECTED:
			continue;

		case Select::SEL_READY:
			port->port_dummy_timeout = port->port_dummy_packet_interval;
			return;

		default:
			break;
		}

		if (port->port_dummy_timeout < 0)
		{
			return;
		}
	}
}

static bool select_wait( rem_port* main_port, Select* selct)
{
/**************************************
 *
 *	s e l e c t _ w a i t
 *
 **************************************
 *
 * Functional description
 *	Select interesting descriptors from
 *	port blocks and wait for something
 *	to read from them.
 *
 **************************************/
	struct timeval timeout;
	bool checkPorts = false;

	for (;;)
	{
		selct->clear();
		bool found = false;

		// Use the time interval between select() calls to expire
		// keepalive timers on all ports.

		time_t delta_time;
		if (selct->slct_time)
		{
			delta_time = time(NULL) - selct->slct_time;
			selct->slct_time += delta_time;
		}
		else
		{
			delta_time = 0;
			selct->slct_time = time(NULL);
		}

		{ // port_mutex scope
			Firebird::MutexLockGuard guard(port_mutex);
			for (rem_port* port = main_port; port; port = port->port_next)
			{
				if (port->port_state == rem_port::PENDING && 
					// don't wait on still listening (not connected) async port
					!(port->port_handle == INVALID_SOCKET && port->port_flags & PORT_async))
				{
					// Adjust down the port's keepalive timer.

					if (port->port_dummy_packet_interval)
					{
						port->port_dummy_timeout -= delta_time;
					}

					if (checkPorts)
					{
						// select() returned EBADF\WSAENOTSOCK - we have a broken socket
						// in current fdset. Search and return it to caller to close
						// broken connection correctly

						struct linger lngr;
						socklen_t optlen = sizeof(lngr);
						const bool badSocket =
#ifdef WIN_NT
							false;
#else
							(port->port_handle < 0 || port->port_handle >= FD_SETSIZE);
#endif

						if (badSocket || getsockopt(port->port_handle,
								SOL_SOCKET, SO_LINGER, (SCHAR*) &lngr, &optlen) != 0)
						{
							if (badSocket || INET_ERRNO == NOTASOCKET)
							{
								// not a socket, strange !
								gds__log("INET/select_wait: found \"not a socket\" socket : %"HANDLEFORMAT,
										 port->port_handle);

								// this will lead to receive() which will break bad connection
								selct->clear();
								if (!badSocket)
								{
									selct->set(port->port_handle);
								}
								return true;
							}
						}
					}

					// if process is shuting down - don't listen on main port
					if (!INET_shutting_down || port != main_port)
					{
						selct->set(port->port_handle);
						found = true;
					}
				}
			}
			checkPorts = false;
		} // port_mutex scope

		if (!found)
		{
			if (!INET_shutting_down && (main_port->port_server_flags & SRVR_multi_client)) {
				gds__log("INET/select_wait: client rundown complete, server exiting");
			}
			return false;
		}

		for (;;)
		{
			// Before waiting for incoming packet, check for server shutdown
			if (tryStopMainThread && tryStopMainThread())
			{
				// this is not server port any more
				main_port->port_server_flags &= ~SRVR_multi_client;
				return false;
			}

			// Some platforms change the timeout in the select call.
			// Reset timeout for each iteration to avoid problems.
			timeout.tv_sec = SELECT_TIMEOUT;
			timeout.tv_usec = 0;

			selct->select(&timeout);
			const int inetErrNo = INET_ERRNO;

			//if (INET_shutting_down) {
			//	return false;
			//}

			if (selct->getCount() != -1)
			{
				// if selct->slct_count is zero it means that we timed out of
				// select with nothing to read or accept, so clear the fd_set
				// bit as this value is undefined on some platforms (eg. HP-UX),
				// when the select call times out. Once these bits are cleared
				// they can be used in select_port()
				if (selct->getCount() == 0)
				{
					Firebird::MutexLockGuard guard(port_mutex);
					for (rem_port* port = main_port; port; port = port->port_next)
					{
						selct->unset(port->port_handle);
					}
				}
				return true;
			}
			if (INTERRUPT_ERROR(inetErrNo))
				continue;
			if (inetErrNo == NOTASOCKET)
			{
				checkPorts = true;
				break;
			}

			gds__log("INET/select_wait: select failed, errno = %d", inetErrNo);
			return false;
		}	// for (;;)
	}
}

static int send_full( rem_port* port, PACKET * packet)
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

	if (!xdr_protocol(&port->port_send, packet))
		return FALSE;

#ifdef DEBUG
	{ // scope
		static ULONG op_sent_count = 0;
		op_sent_count++;
		if (INET_trace & TRACE_operations)
		{
			fprintf(stdout, "%05u: OP Sent %5u opcode %d\n", inet_debug_timer(),
				op_sent_count, packet->p_operation);
			fflush(stdout);
		}
	} // end scope
#endif

	return inet_write(&port->port_send /*, TRUE*/);
}

static int send_partial( rem_port* port, PACKET * packet)
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

#ifdef DEBUG
	{ // scope
		static ULONG op_sentp_count = 0;
		op_sentp_count++;
		if (INET_trace & TRACE_operations)
		{
			fprintf(stdout, "%05u: OP Sent %5u opcode %d (partial)\n", inet_debug_timer(),
				op_sentp_count, packet->p_operation);
			fflush(stdout);
		}
	} // end scope
#endif

	return xdr_protocol(&port->port_send, packet);
}


static int xdrinet_create(XDR* xdrs, rem_port* port, UCHAR* buffer, USHORT length, enum xdr_op x_op)
{
/**************************************
 *
 *	x d r i n e t _ c r e a t e
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
	xdrs->x_ops = (xdr_t::xdr_ops*) &inet_ops;
	xdrs->x_op = x_op;

	return TRUE;
}

#ifdef HAVE_SETITIMER
static void alarm_handler( int x)
{
/**************************************
 *
 *	a l a r m _ h a n d l e r
 *
 **************************************
 *
 * Functional description
 *	Handle an alarm clock interrupt.  If we were waiting on
 *	a semaphone, zap it.
 *
 **************************************/
}
#endif

static XDR_INT inet_destroy( XDR*)
{
/**************************************
 *
 *	i n e t _ d e s t r o y
 *
 **************************************
 *
 * Functional description
 *	Destroy a stream.  A no-op.
 *
 **************************************/

	return (XDR_INT) 0;
}

static void inet_gen_error(rem_port* port, const Arg::StatusVector& v)
{
/**************************************
 *
 *	i n e t _ g e n _ e r r o r
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

	const char* node_name = port->port_connection ? port->port_connection->str_data : "(unknown)";

	Arg::Gds error(isc_network_error);
	error << Arg::Str(node_name) << v;

	ISC_STATUS* status_vector = NULL;
	if (port->port_context != NULL) {
		status_vector = port->port_context->get_status_vector();
	}
	if (status_vector == NULL) {
		status_vector = port->port_status_vector;
	}
	if (status_vector != NULL)
	{
		error.copyTo(status_vector);
		REMOTE_save_status_strings(status_vector);
	}
}


static bool_t inet_getbytes( XDR* xdrs, SCHAR* buff, u_int count)
{
/**************************************
 *
 *	i n e t _ g e t b y t e s
 *
 **************************************
 *
 * Functional description
 *	Get a bunch of bytes from a memory stream if it fits.
 *
 **************************************/
#ifdef REM_SERVER
	const rem_port* port = (rem_port*) xdrs->x_public;
	if ((port->port_flags & PORT_server) && !(port->port_server_flags & SRVR_debug))
	{
		return REMOTE_getbytes(xdrs, buff, count);
	}
#endif

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

		if (!inet_read(xdrs))
			return FALSE;
}

	// Scalar values and bulk transfer remainder fall thru
	// to be moved byte-by-byte to avoid memcpy setup costs.

	if (!bytecount)
		return TRUE;

	if (xdrs->x_handy >= bytecount)
	{
		xdrs->x_handy -= bytecount;
		while (bytecount--)
			*buff++ = *xdrs->x_private++;

		return TRUE;
	}

	while (--bytecount >= 0)
	{
		if (!xdrs->x_handy && !inet_read(xdrs))
			return FALSE;
		*buff++ = *xdrs->x_private++;
		--xdrs->x_handy;
	}

	return TRUE;
}


static bool_t inet_getlong( XDR* xdrs, SLONG* lp)
{
/**************************************
 *
 *	i n e t _ g e t l o n g
 *
 **************************************
 *
 * Functional description
 *	Fetch a longword into a memory stream if it fits.
 *
 **************************************/

	SLONG l;

	if (!(*xdrs->x_ops->x_getbytes) (xdrs, reinterpret_cast<char*>(&l), 4))
		return FALSE;

	*lp = ntohl(l);

	return TRUE;
}

static u_int inet_getpostn( XDR* xdrs)
{
/**************************************
 *
 *	i n e t _ g e t p o s t n
 *
 **************************************
 *
 * Functional description
 *	Get the current position (which is also current length) from stream.
 *
 **************************************/

	return (u_int) (xdrs->x_private - xdrs->x_base);
}

static caddr_t inet_inline( XDR* xdrs, u_int bytecount)
{
/**************************************
 *
 *	i n e t _  i n l i n e
 *
 **************************************
 *
 * Functional description
 *	Return a pointer to somewhere in the buffer.
 *
 **************************************/

	if (bytecount > (u_int) xdrs->x_handy)
		return FALSE;

	return xdrs->x_base + bytecount;
}

static void inet_error(rem_port* port, const TEXT* function, ISC_STATUS operation, int status)
{
/**************************************
 *
 *	i n e t _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	An I/O error has occurred.  Call
 * 	inet_gen_error with the appropriate args
 *	to format the status vector if any.
 *
 **************************************/
	if (status)
	{
		if (port->port_state != rem_port::BROKEN) {
			gds__log("INET/inet_error: %s errno = %d", function, status);
		}

		inet_gen_error(port, Arg::Gds(operation) << SYS_ERR(status));
	}
	else
	{
		// No status value, just format the basic arguments.
		inet_gen_error(port, Arg::Gds(operation));
	}
}

static bool_t inet_putbytes( XDR* xdrs, const SCHAR* buff, u_int count)
{
/**************************************
 *
 *	i n e t _ p u t b y t e s
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

		if (!inet_write(xdrs /*, 0*/))
			return FALSE;
	}

	// Scalar values and bulk transfer remainder fall thru
	// to be moved byte-by-byte to avoid memcpy setup costs.

	if (!bytecount)
		return TRUE;

	if (xdrs->x_handy >= bytecount)
	{
		xdrs->x_handy -= bytecount;
		while (bytecount--)
			*xdrs->x_private++ = *buff++;

		return TRUE;
	}

	while (--bytecount >= 0)
	{
		if (xdrs->x_handy <= 0 && !inet_write(xdrs /*, 0*/))
			return FALSE;
		--xdrs->x_handy;
		*xdrs->x_private++ = *buff++;
	}

	return TRUE;
}


static bool_t inet_putlong( XDR* xdrs, const SLONG* lp)
{
/**************************************
 *
 *	i n e t _ p u t l o n g
 *
 **************************************
 *
 * Functional description
 *	Fetch a longword into a memory stream if it fits.
 *
 **************************************/
	const SLONG l = htonl(*lp);
	return (*xdrs->x_ops->x_putbytes) (xdrs, reinterpret_cast<const char*>(&l), 4);
}

static bool_t inet_read( XDR* xdrs)
{
/**************************************
 *
 *	i n e t _ r e a d
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
	char* p = xdrs->x_base;
	const char* const end = p + INET_remote_buffer;

	// If buffer is not completely empty, slide down what's left

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

	xdrs->x_handy = (int) ((SCHAR *) p - xdrs->x_base);
	xdrs->x_private = xdrs->x_base;

	return TRUE;
}

static bool_t inet_setpostn( XDR* xdrs, u_int bytecount)
{
/**************************************
 *
 *	i n e t _ s e t p o s t n
 *
 **************************************
 *
 * Functional description
 *	Set the current position (which is also current length) from stream.
 *
 **************************************/

	if (bytecount > (u_int) xdrs->x_handy)
		return FALSE;

	xdrs->x_private = xdrs->x_base + bytecount;

	return TRUE;
}

static rem_port* inet_try_connect(PACKET* packet,
								  Rdb* rdb,
								  const Firebird::PathName& file_name,
								  const TEXT* node_name,
								  ISC_STATUS* status_vector,
								  Firebird::ClumpletReader& dpb)
{
/**************************************
 *
 *	i n e t _ t r y _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Given a packet with formatted protocol infomation,
 *	set header information and try the connection.
 *
 *	If a connection is established, return a port block, otherwise
 *	return NULL.
 *
 **************************************/
	P_CNCT* cnct = &packet->p_cnct;
	packet->p_operation = op_connect;
	cnct->p_cnct_operation = op_attach;
	cnct->p_cnct_cversion = CONNECT_VERSION2;
	cnct->p_cnct_client = ARCHITECTURE;
	cnct->p_cnct_file.cstr_length = (USHORT) file_name.length();
	cnct->p_cnct_file.cstr_address = reinterpret_cast<const UCHAR*>(file_name.c_str());

	// If we can't talk to a server, punt.  Let somebody else generate
	// an error.  status_vector will have the network error info.

	rem_port* port = INET_connect(node_name, packet, status_vector, FALSE, &dpb);
	if (!port)
	{
		delete rdb;
		return NULL;
	}

	// Get response packet from server.

	rdb->rdb_port = port;
	port->port_context = rdb;
	if (!port->receive(packet))
	{
		inet_error(port, "receive in try_connect", isc_net_connect_err, INET_ERRNO);
		disconnect(port);
		delete rdb;
		return NULL;
	}

	return port;
}

static bool_t inet_write( XDR* xdrs /*, bool_t end_flag*/)
{
/**************************************
 *
 *	i n e t _ w r i t e
 *
 **************************************
 *
 * Functional description
 *	Write a buffer full of data.
 *  Obsolete: If the end_flag isn't set, indicate
 *	that the buffer is a fragment, and reset the XDR for another buffer
 *	load.
 *
 **************************************/
	// Encode the data portion of the packet

	rem_port* port = (rem_port*) xdrs->x_public;
	const char* p = xdrs->x_base;
	SSHORT length = xdrs->x_private - p;

	// Send data in manageable hunks.  If a packet is partial, indicate
	// that with a negative length.  A positive length marks the end.

	//p = xdrs->x_base; redundant

	while (length)
	{
		const SSHORT l = (SSHORT) MIN(length, INET_remote_buffer);
		length -= l;
		if (!packet_send(port, p, (SSHORT) (length ? -l : l)))
			return FALSE;
		p += l;
	}

	xdrs->x_private = xdrs->x_base;
	xdrs->x_handy = INET_remote_buffer;

	return TRUE;

#ifdef PIGGYBACK
	// CVC: Screwed logic here: if I initialize l2 to zero, nothing useful executes.
	SCHAR aux_buffer[BUFFER_SIZE];
	SSHORT l2 = 0;
#error Assign l2 some meaningful value before running this.
	// If the other end has not piggy-backed the next packet, we're done.

	if (!l2)
		return TRUE;

	// We've got a piggy-backed response.  If the packet is partial,
	// send an ACK for part we did receive.

	char* p2 = aux_buffer;

	while (l2 < 0)
	{
		if (!packet_send(port, 0, 0))
			return FALSE;
		p2 -= l2;
		length = aux_buffer + sizeof(aux_buffer) - p2;
		if (!packet_receive(port, p2, length, &l2))
		{
			p2 += l2;
			continue;
		}
	}

	length = p2 - aux_buffer + l2;

	// Now we're got a encode glump ready to stuff into the read buffer.
	// Unfortunately, if we just add it to the read buffer, we will shortly
	// overflow the buffer.  To avoid this, "scrumpf down" the active bits
	// in the read buffer, then add out stuff at the end.

	xdrs = &port->port_receive;
	p2 = xdrs->x_base;

	if (xdrs->x_handy && p2 != xdrs->x_private) {
		memmove(p2, xdrs->x_private, xdrs->x_handy);
	}

	p2 += xdrs->x_handy;

	xdrs->x_private = xdrs->x_base;

	// xdrs->x_handy += JAP_decode (aux_buffer, length, p2);

	return TRUE;
#endif
}

#ifdef DEBUG
static void packet_print(const TEXT* string, const UCHAR* packet, int length, ULONG counter)
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
	for (int l = length; l > 0; --l)
		sum += *packet++;

	fprintf(stdout, "%05u:    PKT %s\t(%u): length = %4d, checksum = %d\n",
			   inet_debug_timer(), string, counter, length, sum);
	fflush(stdout);
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

	if (port->port_flags & PORT_disconnect) {
		return false;
	}

	timeval timeout;
	timeout.tv_usec = 0;
	timeval* time_ptr = NULL;

	if (port->port_protocol == 0)
	{
		// If the protocol is 0 we are still in the process of establishing
		// a connection. Add a time out to the wait.
		timeout.tv_sec = port->port_connect_timeout;
		time_ptr = &timeout;
	}
	else if (port->port_protocol >= PROTOCOL_VERSION8 && port->port_dummy_packet_interval > 0)
	{
		// Set the time interval for sending dummy packets to the client
		timeout.tv_sec = port->port_dummy_packet_interval;
		time_ptr = &timeout;
	}

	// On Linux systems (and possibly others too) select will eventually
	// change timout values so save it here for later reuse.
	// Thanks to Brad Pepers who reported this bug  FSG 3 MAY 2001
	const timeval savetime = timeout;

	const SOCKET ph = port->port_handle;

	// Unsed to send a dummy packet, but too big to be defined in the loop.
	PACKET packet;

	int n = 0;
	int inetErrNo;

	for (;;)
	{
		// Implement an error-detection protocol to ensure that the client
		// is still there.  Use the select() call with a timeout to wait on
		// the connection for an incoming packet.  If none comes within a
		// suitable time interval, write a dummy packet on the connection.
		// If the client is not there, an error will be returned on the write.
		// If the client is there, the dummy packet will be ignored by all
		// InterBase clients V4 or greater.  This protocol will detect when
		// clients are lost abnormally through reboot or network disconnect.

		// Don't send op_dummy packets on aux port; the server won't
		// read them because it only writes to aux ports.

		if ( !(port->port_flags & PORT_async) )
		{
			Select slct;
			slct.set(ph);

			int slct_count;
			for (;;)
			{
				slct.select(time_ptr);
				slct_count = slct.getCount();
				inetErrNo = INET_ERRNO;

				// restore original timeout value FSG 3 MAY 2001
				timeout = savetime;

				if (slct_count != -1 || !INTERRUPT_ERROR(inetErrNo))
				{
					break;
				}
			}

			if (slct_count == -1)
			{
				if (!(port->port_flags & PORT_disconnect)) {
					inet_error(port, "select in packet_receive", isc_net_read_err, inetErrNo);
				}
				return false;
			}

			if (!slct_count && port->port_protocol >= PROTOCOL_VERSION8)
			{
#ifdef DEBUG
				if (INET_trace & TRACE_operations)
				{
					fprintf(stdout, "%05u: OP Sent: op_dummy\n", inet_debug_timer());
					fflush(stdout);
				}
#endif
				packet.p_operation = op_dummy;
				if (!send_full(port, &packet))
				{
					return false;
				}
				continue;
			}

			if (!slct_count && port->port_protocol == 0)
			{
				return false;
			}
		}

		n = recv(port->port_handle, reinterpret_cast<char*>(buffer), buffer_length, 0);
		inetErrNo = INET_ERRNO;

		if (n != -1 || !INTERRUPT_ERROR(inetErrNo))
			break;
	}

	if (n <= 0 && (port->port_flags & PORT_disconnect)) {
		return false;
	}

	if (n == -1)
	{
		inet_error(port, "read", isc_net_read_err, inetErrNo);
		return false;
	}

	if (!n)
	{
		port->port_state = rem_port::BROKEN;
		return false;
	}

#ifdef DEBUG
	{ // scope
		INET_count_recv++;
		INET_bytes_recv += n;
		if (INET_trace & TRACE_packets)
			packet_print("receive", buffer, n, INET_count_recv);
		INET_force_error--;
		if (INET_force_error == 0)
		{
			INET_force_error = 1;
			inet_error(port, "simulated error - read", isc_net_read_err, 0);
			return false;
		}
	} // end scope
#endif

	*length = n;

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

	const char* data = buffer;
	SSHORT length = buffer_length;

	while (length)
	{
#ifdef DEBUG
		if (INET_trace & TRACE_operations)
		{
			fprintf(stdout, "Before Send\n");
			fflush(stdout);
		}
#endif
		SSHORT n = send(port->port_handle, data, length, FB_SEND_FLAGS);
#ifdef DEBUG
		if (INET_trace & TRACE_operations)
		{
			fprintf(stdout, "After Send n is %d\n", n);
			fflush(stdout);
		}
#endif
		if (n == length) {
			break;
		}

		if (n == -1)
		{
			if (INTERRUPT_ERROR(INET_ERRNO)) {
				continue;
			}

			inet_error(port, "send", isc_net_write_err, INET_ERRNO);
			return false;
		}

		data += n;
		length -= n;
	}

#ifdef HAVE_SETITIMER
	struct itimerval internal_timer, client_timer;
	struct sigaction internal_handler, client_handler;
#endif // HAVE_SETITIMER

	if ((port->port_flags & PORT_async) && !(port->port_flags & PORT_no_oob))
	{
		int count = 0;
		SSHORT n;
		int inetErrNo = 0;
		const char* b = buffer;
		while ((n = send(port->port_handle, b, 1, MSG_OOB | FB_SEND_FLAGS)) == -1 &&
				(INET_ERRNO == ENOBUFS || INTERRUPT_ERROR(INET_ERRNO)))
		{
			inetErrNo = INET_ERRNO;

			if (count++ > 20) {
				break;
			}

#ifndef HAVE_SETITIMER
#ifdef WIN_NT
			SleepEx(50, TRUE);
#else
			sleep(1);
#endif
		} // end of while() loop for systems without setitimer.
#else // HAVE_SETITIMER
			if (count == 1)
			{
				// Wait in a loop until the lock becomes available

				internal_timer.it_interval.tv_sec = 0;
				internal_timer.it_interval.tv_usec = 0;
				internal_timer.it_value.tv_sec = 0;
				internal_timer.it_value.tv_usec = 0;
				setitimer(ITIMER_REAL, &internal_timer, &client_timer);
				internal_handler.sa_handler = alarm_handler;
				sigemptyset(&internal_handler.sa_mask);
				internal_handler.sa_flags = SA_RESTART;
				sigaction(SIGALRM, &internal_handler, &client_handler);
			}

			internal_timer.it_value.tv_sec = 0;
			internal_timer.it_value.tv_usec = 50000;
			setitimer(ITIMER_REAL, &internal_timer, NULL);
			pause();
		} // end of while() loop for systems with setitimer

		if (count)
		{
			// Restore user's outstanding alarm request and handler

			internal_timer.it_value.tv_sec = 0;
			internal_timer.it_value.tv_usec = 0;
			setitimer(ITIMER_REAL, &internal_timer, NULL);
			sigaction(SIGALRM, &client_handler, NULL);
			setitimer(ITIMER_REAL, &client_timer, NULL);
		}
#endif // HAVE_SETITIMER

		if (n == -1)
		{
			inet_error(port, "send/oob", isc_net_write_err, inetErrNo);
			return false;
		}
	}

#ifdef DEBUG
	{ // scope
		INET_count_send++;
		INET_bytes_send += buffer_length;
		if (INET_trace & TRACE_packets)
			packet_print("send", (const UCHAR*) buffer, buffer_length, INET_count_send);
		INET_force_error--;
		if (INET_force_error == 0)
		{
			INET_force_error = 1;
			inet_error(port, "simulated error - send", isc_net_write_err, 0);
			return false;
		}
	} // end scope
#endif

	return true;
}

static bool setNoNagleOption(rem_port* port)
{
/**************************************
 *
 *      s e t N o N a g l e O p t i o n
 *
 **************************************
 *
 * Functional description
 *      Set TCP_NODELAY, return false
 *		in case of unexpected error
 *
 **************************************/
	if (Config::getTcpNoNagle())
	{
		int optval = TRUE;
		int n = setsockopt(port->port_handle, IPPROTO_TCP, TCP_NODELAY,
						   (SCHAR*) &optval, sizeof(optval));

		if (n == -1)
		{
			return false;
		}
	}
	return true;
}

void setStopMainThread(FPTR_INT func)
{
/**************************************
 *
 *   s e t S t o p M a i n T h r e a d
 *
 **************************************
 *
 * Functional description
 *	Set function called by main thread
 *	in order to check for shutdown.
 *
 **************************************/
	tryStopMainThread = func;
}
