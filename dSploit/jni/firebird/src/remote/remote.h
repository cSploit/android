/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		remote.h
 *	DESCRIPTION:	Common descriptions
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef REMOTE_REMOTE_H
#define REMOTE_REMOTE_H

#include "../jrd/common.h"
#include "gen/iberror.h"
#include "../remote/remote_def.h"
#include "../jrd/ThreadData.h"
#include "../jrd/ThreadStart.h"
#include "../common/thd.h"
#include "../common/classes/objects_array.h"
#include "../auth/trusted/AuthSspi.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/classes/RefMutex.h"
#include "../common/StatusHolder.h"

#if !defined(SUPERCLIENT) && !defined(EMBEDDED)
#define REM_SERVER
#endif

// Include some apollo include files for tasking

#ifndef WIN_NT
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET  -1
#endif
#endif // !WIN_NT


// Uncomment this line if you need to trace module activity
//#define REMOTE_DEBUG

#ifdef REMOTE_DEBUG
DEFINE_TRACE_ROUTINE(remote_trace);
#define REMOTE_TRACE(args) remote_trace args
#else
#define REMOTE_TRACE(args) // nothing
#endif

#ifdef DEV_BUILD
// Debug packet/XDR memory allocation

// Temporarily disabling DEBUG_XDR_MEMORY
// #define DEBUG_XDR_MEMORY

#endif

const int BLOB_LENGTH		= 16384;

#include "../remote/protocol.h"
#include "fb_blk.h"

// fwd. decl.
struct rem_port;


typedef Firebird::AutoPtr<UCHAR, Firebird::ArrayDelete<UCHAR> > UCharArrayAutoPtr;


struct Rdb : public Firebird::GlobalStorage, public TypedHandle<rem_type_rdb>
{
	USHORT			rdb_id;
	USHORT			rdb_flags;
	FB_API_HANDLE	rdb_handle;				// database handle
	rem_port*		rdb_port;				// communication port
	struct Rtr*		rdb_transactions;		// linked list of transactions
	struct Rrq*		rdb_requests;			// compiled requests
	struct Rvnt*	rdb_events;				// known events
	struct Rsr*		rdb_sql_requests;		// SQL requests
	PACKET			rdb_packet;				// Communication structure
private:
	ISC_STATUS*		rdb_status_vector;		// Normally used status vector
	ISC_STATUS*		rdb_async_status_vector;	// status vector for async thread
	FB_THREAD_ID	rdb_async_thread_id;		// Id of async thread (when active)
public:
	Firebird::Mutex	rdb_async_lock;			// Sync to avoid 2 async calls at once

public:
	// Values for rdb_flags
	enum {
		SERVICE = 1
	};

public:
	Rdb() :
		rdb_id(0), rdb_flags(0), rdb_handle(0),
		rdb_port(0), rdb_transactions(0), rdb_requests(0),
		rdb_events(0), rdb_sql_requests(0), rdb_status_vector(0),
		rdb_async_status_vector(0), rdb_async_thread_id(0)
	{
	}

	static ISC_STATUS badHandle() { return isc_bad_db_handle; }

	// This 2 functions assume rdb_async_lock to be locked 
	void set_async_vector(ISC_STATUS* userStatus) throw();
	void reset_async_vector() throw();

	ISC_STATUS* get_status_vector() throw();
	void set_status_vector(ISC_STATUS* userStatus) throw()
	{
		rdb_status_vector = userStatus;
	}
	void status_assert(ISC_STATUS* userStatus)
	{
		fb_assert(rdb_status_vector == userStatus);
	}
	void save_status_vector(ISC_STATUS*& save) throw()
	{
		save = rdb_status_vector;
	}
};


struct Rtr : public Firebird::GlobalStorage, public TypedHandle<rem_type_rtr>
{
	Rdb*		rtr_rdb;
	Rtr*		rtr_next;
	struct Rbl*	rtr_blobs;
	FB_API_HANDLE rtr_handle;
	USHORT		rtr_id;
	bool		rtr_limbo;

public:
	Rtr() :
		rtr_rdb(0), rtr_next(0), rtr_blobs(0),
		rtr_handle(0), rtr_id(0), rtr_limbo(0)
	{ }

	static ISC_STATUS badHandle() { return isc_bad_trans_handle; }
};


struct Rbl : public Firebird::GlobalStorage, public TypedHandle<rem_type_rbl>
{
	Firebird::HalfStaticArray<UCHAR, BLOB_LENGTH> rbl_data;
	Rdb*		rbl_rdb;
	Rtr*		rbl_rtr;
	Rbl*		rbl_next;
	UCHAR*		rbl_buffer;
	UCHAR*		rbl_ptr;
	FB_API_HANDLE rbl_handle;
	SLONG		rbl_offset;			// Apparent (to user) offset in blob
	USHORT		rbl_id;
	USHORT		rbl_flags;
	USHORT		rbl_buffer_length;
	USHORT		rbl_length;
	USHORT		rbl_fragment_length;
	USHORT		rbl_source_interp;	// source interp (for writing)
	USHORT		rbl_target_interp;	// destination interp (for reading)

public:
	// Values for rbl_flags
	enum {
		EOF_SET = 1,
		SEGMENT = 2,
		EOF_PENDING = 4,
		CREATE = 8
	};

public:
	Rbl() :
		rbl_data(getPool()), rbl_rdb(0), rbl_rtr(0), rbl_next(0),
		rbl_buffer(rbl_data.getBuffer(BLOB_LENGTH)), rbl_ptr(rbl_buffer), rbl_handle(0),
		rbl_offset(0), rbl_id(0), rbl_flags(0),
		rbl_buffer_length(BLOB_LENGTH), rbl_length(0), rbl_fragment_length(0),
		rbl_source_interp(0), rbl_target_interp(0)
	{ }

	static ISC_STATUS badHandle() { return isc_bad_segstr_handle; }
};


struct Rvnt : public Firebird::GlobalStorage
{
	Rvnt*		rvnt_next;
	Rdb*		rvnt_rdb;
	FPTR_EVENT_CALLBACK	rvnt_ast;
	void*		rvnt_arg;
	SLONG		rvnt_id;
	SLONG		rvnt_rid;	// used by server to store client-side id
	rem_port*	rvnt_port;	// used to id server from whence async came
	const UCHAR*		rvnt_items;
	USHORT		rvnt_length;

public:
	Rvnt() :
		rvnt_next(0), rvnt_rdb(0), rvnt_ast(0),
		rvnt_arg(0), rvnt_id(0), rvnt_rid(0),
		rvnt_port(0), rvnt_items(0), rvnt_length(0)
	{ }
};


struct rem_str : public pool_alloc_rpt<SCHAR>
{
	USHORT		str_length;
	SCHAR		str_data[2];
};


// Include definition of descriptor

#include "../jrd/dsc.h"


struct rem_fmt : public Firebird::GlobalStorage
{
	USHORT		fmt_length;
	USHORT		fmt_net_length;
	USHORT		fmt_count;
	USHORT		fmt_version;
	Firebird::Array<dsc> fmt_desc;

public:
	explicit rem_fmt(size_t rpt) :
		fmt_length(0), fmt_net_length(0), fmt_count(0),
		fmt_version(0), fmt_desc(getPool(), rpt)
	{
		fmt_desc.grow(rpt);
	}
};

// Windows declares a msg structure, so rename the structure
// to avoid overlap problems.

struct RMessage : public Firebird::GlobalStorage
{
	RMessage*	msg_next;			// Next available message
#ifdef SCROLLABLE_CURSORS
	RMessage*	msg_prior;			// Next available message
	ULONG		msg_absolute; 		// Absolute record number in cursor result set
#endif
	USHORT		msg_number;			// Message number
	UCHAR*		msg_address;		// Address of message
	UCharArrayAutoPtr msg_buffer;	// Allocated message

public:
	explicit RMessage(size_t rpt) :
		msg_next(0),
#ifdef SCROLLABLE_CURSORS
		msg_prior(0), msg_absolute(0),
#endif
		msg_number(0), msg_address(0), msg_buffer(FB_NEW(getPool()) UCHAR[rpt])
	{
		memset(msg_buffer, 0, rpt);
	}
};


// remote stored procedure request
struct Rpr : public Firebird::GlobalStorage
{
	Rdb*		rpr_rdb;
	Rtr*		rpr_rtr;
	FB_API_HANDLE rpr_handle;
	RMessage*	rpr_in_msg;		// input message
	RMessage*	rpr_out_msg;	// output message
	rem_fmt*	rpr_in_format;	// Format of input message
	rem_fmt*	rpr_out_format;	// Format of output message

public:
	Rpr() :
		rpr_rdb(0), rpr_rtr(0), rpr_handle(0),
		rpr_in_msg(0), rpr_out_msg(0), rpr_in_format(0), rpr_out_format(0)
	{ }
};

struct Rrq : public Firebird::GlobalStorage, public TypedHandle<rem_type_rrq>
{
	Rdb*	rrq_rdb;
	Rtr*	rrq_rtr;
	Rrq*	rrq_next;
	Rrq*	rrq_levels;		// RRQ block for next level
	FB_API_HANDLE rrq_handle;
	USHORT		rrq_id;
	USHORT		rrq_max_msg;
	USHORT		rrq_level;
	ISC_STATUS_ARRAY	rrq_status_vector;

	struct		rrq_repeat
	{
		rem_fmt*	rrq_format;		// format for this message
		RMessage*	rrq_message; 	// beginning or end of cache, depending on whether it is client or server
		RMessage*	rrq_xdr;		// point at which cache is read or written by xdr
#ifdef SCROLLABLE_CURSORS
		RMessage*	rrq_last;		// last message returned
		ULONG		rrq_absolute;	// current offset in result set for record being read into cache
		USHORT		rrq_flags;
#endif
		USHORT		rrq_msgs_waiting;	// count of full rrq_messages
		USHORT		rrq_rows_pending;	// How many rows in waiting
		USHORT		rrq_reorder_level;	// Reorder when rows_pending < this level
		USHORT		rrq_batch_count;	// Count of batches in pipeline

	};
	Firebird::Array<rrq_repeat> rrq_rpt;

public:
#ifdef SCROLLABLE_CURSORS
	enum {
		BACKWARD = 1,			// the cache was created in the backward direction
		ABSOLUTE_BACKWARD = 2,	// rrq_absolute is measured from the end of the stream
		LAST_BACKWARD = 4		// last time, the next level up asked for us to scroll in the backward direction
	};
#endif

public:
	explicit Rrq(size_t rpt) :
		rrq_rdb(0), rrq_rtr(0), rrq_next(0), rrq_levels(0),
		rrq_handle(0), rrq_id(0), rrq_max_msg(0), rrq_level(0),
		rrq_rpt(getPool(), rpt)
	{
		memset(rrq_status_vector, 0, sizeof rrq_status_vector);
		rrq_rpt.grow(rpt);
	}

	Rrq* clone() const
	{
		Rrq* rc = new Rrq(rrq_rpt.getCount());
		*rc = *this;
		return rc;
	}

	static ISC_STATUS badHandle() { return isc_bad_req_handle; }
};


template <typename T>
class RFlags
{
public:
	RFlags() :
		m_flags(0)
	{
		// Require base flags field to be unsigned.
		// This is a compile-time assertion; it won't build if you use a signed flags field.
		typedef int dummy[T(-1) > 0];
	}
	explicit RFlags(const T flags) :
		m_flags(flags)
	{}
	// At least one bit in the parameter is 1 in the object.
	bool test(const T flags) const
	{
		return m_flags & flags;
	}
	// All bits received as parameter are 1 in the object.
	bool testAll(const T flags) const
	{
		return (m_flags & flags) == flags;
	}
	void set(const T flags)
	{
		m_flags |= flags;
	}
	void clear(const T flags)
	{
		m_flags &= ~flags;
	}
	void reset()
	{
		m_flags = 0;
	}
private:
	T m_flags;
};


// remote SQL request
struct Rsr : public Firebird::GlobalStorage, public TypedHandle<rem_type_rsr>
{
	Rsr*			rsr_next;
	Rdb*			rsr_rdb;
	Rtr*			rsr_rtr;
	FB_API_HANDLE	rsr_handle;
	rem_fmt*		rsr_bind_format;		// Format of bind message
	rem_fmt*		rsr_select_format;		// Format of select message
	rem_fmt*		rsr_user_select_format; // Format of user's select message
	rem_fmt*		rsr_format;				// Format of current message
	RMessage*		rsr_message;			// Next message to process
	RMessage*		rsr_buffer;				// Next buffer to use
	Firebird::StatusHolder* rsr_status;		// saved status for buffered errors
	USHORT			rsr_id;
	RFlags<USHORT>	rsr_flags;
	USHORT			rsr_fmt_length;

	ULONG			rsr_rows_pending;	// How many rows are pending
	USHORT			rsr_msgs_waiting; 	// count of full rsr_messages
	USHORT			rsr_reorder_level; 	// Trigger pipelining at this level
	USHORT			rsr_batch_count; 	// Count of batches in pipeline

public:
	// Values for rsr_flags.
	enum {
		FETCHED = 1,		// Cleared by execute, set by fetch
		EOF_SET = 2,		// End-of-stream encountered
		BLOB = 4,			// Statement relates to blob op
		NO_BATCH = 8,		// Do not batch fetch rows
		STREAM_ERR = 16,	// There is an error pending in the batched rows
		LAZY = 32,			// To be allocated at the first reference
		DEFER_EXECUTE = 64,	// op_execute can be deferred
		PAST_EOF = 128		// EOF was returned by fetch from this statement
	};

public:
	Rsr() :
		rsr_next(0), rsr_rdb(0), rsr_rtr(0), rsr_handle(0),
		rsr_bind_format(0), rsr_select_format(0), rsr_user_select_format(0),
		rsr_format(0), rsr_message(0), rsr_buffer(0), rsr_status(0),
		rsr_id(0), rsr_fmt_length(0),
		rsr_rows_pending(0), rsr_msgs_waiting(0), rsr_reorder_level(0), rsr_batch_count(0)
		{ }

	void saveException(const ISC_STATUS* status, bool overwrite);
	void clearException();
	ISC_STATUS haveException();
	void raiseException();
	void releaseException();

	static ISC_STATUS badHandle() { return isc_bad_req_handle; }
};


// Makes it possible to safely store all handles in single array
class RemoteObject
{
private:
	union {
		Rdb* rdb;
		Rtr* rtr;
		Rbl* rbl;
		Rrq* rrq;
		Rsr* rsr;
	} ptr;

public:
	RemoteObject() { ptr.rdb = 0; }

	template <typename R>
	R* get(R* r)
	{
		if (! r->checkHandle())
		{
			Firebird::status_exception::raise(Firebird::Arg::Gds(R::badHandle()));
		}
		return r;
	}

	void operator=(Rdb* v) { ptr.rdb = v; }
	void operator=(Rtr* v) { ptr.rtr = v; }
	void operator=(Rbl* v) { ptr.rbl = v; }
	void operator=(Rrq* v) { ptr.rrq = v; }
	void operator=(Rsr* v) { ptr.rsr = v; }

	operator Rdb*() { return get(ptr.rdb); }
	operator Rtr*() { return get(ptr.rtr); }
	operator Rbl*() { return get(ptr.rbl); }
	operator Rrq*() { return get(ptr.rrq); }
	operator Rsr*() { return get(ptr.rsr); }

	bool isMissing() const { return ptr.rdb == NULL; }
	void release() { ptr.rdb = 0; }
};



inline void Rsr::saveException(const ISC_STATUS* status, bool overwrite)
{
	if (!rsr_status) {
		rsr_status = new Firebird::StatusHolder();
	}
	if (overwrite || !rsr_status->getError()) {
		rsr_status->save(status);
	}
}

inline void Rsr::clearException()
{
	if (rsr_status)
		rsr_status->clear();
}

inline ISC_STATUS Rsr::haveException()
{
	return (rsr_status ? rsr_status->getError() : 0);
}

inline void Rsr::raiseException()
{
	if (rsr_status)
		rsr_status->raise();
}

inline void Rsr::releaseException()
{
	delete rsr_status;
	rsr_status = NULL;
}

#include "../remote/xdr.h"


// Generalized port definition.

#ifndef WIN_NT
typedef int SOCKET;
#endif


//////////////////////////////////////////////////////////////////
// fwd. decl.
struct p_cnct;
struct rmtque;
struct xcc; // defined in xnet.h

// Queue of deferred packets

struct rem_que_packet
{
	PACKET packet;
	bool sent;
};

typedef Firebird::Array<rem_que_packet> PacketQueue;

#ifdef TRUSTED_AUTH
// delayed authentication block for trusted auth callback
class ServerAuth : public Firebird::GlobalStorage
{
public:
	typedef void Part2(rem_port*, P_OP, const char* fName, int fLen, const UCHAR* pb, int pbLen, PACKET*);
	Firebird::PathName fileName;
	Firebird::HalfStaticArray<UCHAR, 128> clumplet;
	AuthSspi* authSspi;
	Part2* part2;
	P_OP operation;

	ServerAuth(const char* fName, int fLen, const Firebird::ClumpletWriter& pb, Part2* p2, P_OP op);
	~ServerAuth();
};
#endif // TRUSTED_AUTH


// port_flags
const USHORT PORT_symmetric		= 0x0001;	// Server/client architectures are symmetic
const USHORT PORT_rpc			= 0x0002;	// Protocol is remote procedure call
const USHORT PORT_async			= 0x0004;	// Port is asynchronous channel for events
const USHORT PORT_no_oob		= 0x0008;	// Don't send out of band data
const USHORT PORT_disconnect	= 0x0010;	// Disconnect is in progress
// This is set only in inet.cpp but never tested
//const USHORT PORT_not_trusted	= 0x0020;	// Connection is from an untrusted node
const USHORT PORT_dummy_pckt_set= 0x0040;	// A dummy packet interval is set
const USHORT PORT_partial_data	= 0x0080;	// Physical packet doesn't contain all API packet
const USHORT PORT_lazy			= 0x0100;	// Deferred operations are allowed
const USHORT PORT_server		= 0x0200;	// Server (not client) port
const USHORT PORT_detached		= 0x0400;	// op_detach, op_drop_database or op_service_detach was processed
const USHORT PORT_rdb_shutdown	= 0x0800;	// Database is shut down

// Port itself

typedef rem_port* (*t_port_connect)(rem_port*, PACKET*);

typedef Firebird::RefPtr<rem_port> RemPortPtr;

struct rem_port : public Firebird::GlobalStorage, public Firebird::RefCounted
{
#ifdef DEV_BUILD
	static Firebird::AtomicCounter portCounter;
#endif

	// sync objects
	Firebird::RefPtr<Firebird::RefMutex> port_sync;
#ifdef REM_SERVER
	Firebird::RefPtr<Firebird::RefMutex> port_que_sync;
#endif
	Firebird::RefPtr<Firebird::RefMutex> port_write_sync;

	// port function pointers (C "emulation" of virtual functions)
	bool			(*port_accept)(rem_port*, const p_cnct*);
	void			(*port_disconnect)(rem_port*);
	void			(*port_force_close)(rem_port*);
	rem_port*		(*port_receive_packet)(rem_port*, PACKET*);
	XDR_INT			(*port_send_packet)(rem_port*, PACKET*);
	XDR_INT			(*port_send_partial)(rem_port*, PACKET*);
	t_port_connect	port_connect;		// Establish secondary connection
	rem_port*		(*port_request)(rem_port*, PACKET*);	// Request to establish secondary connection
	bool			(*port_select_multi)(rem_port*, UCHAR*, SSHORT, SSHORT*, RemPortPtr&);	// get packet from active port

	enum rem_port_t {
		INET,			// Internet (TCP/IP)
		PIPE,			// Windows NT named pipe connection
		XNET			// Windows NT shared memory connection
	}				port_type;
	enum state_t {
		PENDING,		// connection is pending
		BROKEN,			// connection is broken
		DISCONNECTED	// port is disconnected
	}				port_state;

	//P_ARCH		port_client_arch;	// so we can tell arch of client
	rem_port*		port_clients;		// client ports
	rem_port*		port_next;			// next client port
	rem_port*		port_parent;		// parent port (for client ports)
	rem_port*		port_async;			// asynchronous sibling port
	rem_port*		port_async_receive;	// async packets receiver
	struct srvr*	port_server;		// server of port
	USHORT			port_server_flags;	// TRUE if server
	USHORT			port_protocol;		// protocol version number
	USHORT			port_buff_size;		// port buffer size (approx)
	USHORT			port_flags;			// Misc flags
	SLONG			port_connect_timeout;   // Connection timeout value
	SLONG			port_dummy_packet_interval; // keep alive dummy packet interval
	SLONG			port_dummy_timeout;	// time remaining until keepalive packet
	ISC_STATUS*		port_status_vector;
	SOCKET			port_handle;		// handle for INET socket
	SOCKET			port_channel;		// handle for connection (from by OS)
	struct linger	port_linger;		// linger value as defined by SO_LINGER
	Rdb*			port_context;
	ThreadHandle	port_events_thread;	// handle of thread, handling incoming events
	void			(*port_events_shutdown)(rem_port*);	// hack - avoid changing API at beta stage
#ifdef WIN_NT
	HANDLE			port_pipe;			// port pipe handle
	HANDLE			port_event;			// event associated with a port
#endif
	XDR				port_receive;
	XDR				port_send;
#ifdef DEBUG_XDR_MEMORY
	r e m _ v e c*	port_packet_vector;		// Vector of send/receive packets
#endif
	Firebird::Array<RemoteObject> port_objects;
	rem_str*		port_version;
	rem_str*		port_host;				// Our name
	rem_str*		port_connection;		// Name of connection
	rem_str*		port_user_name;
	rem_str*		port_passwd;
	rem_str*		port_protocol_str;		// String containing protocol name for this port
	rem_str*		port_address_str;		// Protocol-specific address string for the port
	Rpr*			port_rpr;				// port stored procedure reference
	Rsr*			port_statement;			// Statement for execute immediate
	rmtque*			port_receive_rmtque;	// for client, responses waiting
	Firebird::AtomicCounter	port_requests_queued;	// requests currently queued
	xcc*			port_xcc;				// interprocess structure
	PacketQueue*	port_deferred_packets;	// queue of deferred packets
	OBJCT			port_last_object_id;	// cached last id
#ifdef REM_SERVER
	Firebird::ObjectsArray< Firebird::Array<char> > port_queue;
	size_t			port_qoffset;			// current packet in the queue
#endif
#ifdef TRUSTED_AUTH
	ServerAuth*		port_trusted_auth;
#endif
	UCharArrayAutoPtr	port_buffer;

public:
	rem_port(rem_port_t t, size_t rpt) :
		port_sync(FB_NEW(getPool()) Firebird::RefMutex()),
#ifdef REM_SERVER
		port_que_sync(FB_NEW(getPool()) Firebird::RefMutex()),
#endif
		port_write_sync(FB_NEW(getPool()) Firebird::RefMutex()),
		port_accept(0), port_disconnect(0), port_force_close(0), port_receive_packet(0), port_send_packet(0),
		port_send_partial(0), port_connect(0), port_request(0), port_select_multi(0),
		port_type(t), port_state(PENDING), //port_client_arch(arch_generic),
		port_clients(0), port_next(0), port_parent(0), port_async(0), port_async_receive(0),
		port_server(0), port_server_flags(0), port_protocol(0), port_buff_size(0),
		port_flags(0), port_connect_timeout(0), port_dummy_packet_interval(0),
		port_dummy_timeout(0), port_status_vector(0), port_handle(INVALID_SOCKET), port_channel(INVALID_SOCKET),
		port_context(0), port_events_thread(0), port_events_shutdown(0),
#ifdef WIN_NT
		port_pipe(INVALID_HANDLE_VALUE), port_event(INVALID_HANDLE_VALUE),
#endif
#ifdef DEBUG_XDR_MEMORY
		port_packet_vector(0),
#endif
		port_objects(getPool()), port_version(0), port_host(0),
		port_connection(0), port_user_name(0), port_passwd(0), port_protocol_str(0),
		port_address_str(0), port_rpr(0), port_statement(0), port_receive_rmtque(0),
		port_requests_queued(0), port_xcc(0), port_deferred_packets(0), port_last_object_id(0),
#ifdef REM_SERVER
		port_queue(getPool()), port_qoffset(0),
#endif
#ifdef TRUSTED_AUTH
		port_trusted_auth(0),
#endif
		port_buffer(FB_NEW(getPool()) UCHAR[rpt])
	{
		addRef();
		memset (&port_linger, 0, sizeof port_linger);
		memset (port_buffer, 0, rpt);
#ifdef DEV_BUILD
		++portCounter;
#endif
	}

private:		// this is refCounted object
	~rem_port();

public:
	void linkParent(rem_port* const parent);

	void unlinkParent();

	template <typename T>
	void getHandle(T*& blk, OBJCT id)
	{
		if ((port_flags & PORT_lazy) && (id == INVALID_OBJECT))
		{
			id = port_last_object_id;
		}
		if (id >= port_objects.getCount() || port_objects[id].isMissing())
		{
			Firebird::status_exception::raise(Firebird::Arg::Gds(T::badHandle()));
		}
		blk = port_objects[id];
	}

	template <typename T>
	OBJCT setHandle(T* const object, const OBJCT id)
	{
		if (id >= port_objects.getCount())
		{
			// Prevent the creation of object handles that can't be
			// transferred by the remote protocol.
			if (id > MAX_OBJCT_HANDLES)
			{
				return (OBJCT) 0;
			}

			port_objects.grow(id + 1);
		}

		port_objects[id] = object;
		return id;
	}

	// Allocate an object slot for an object.
	template <typename T>
	OBJCT get_id(T* object)
	{
		// Reserve slot 0 so we can distinguish something from nothing.
		// NOTE: prior to server version 4.5.0 id==0 COULD be used - so
		// only the server side can now depend on id==0 meaning "invalid id"
		unsigned int i = 1;
		for (; i < port_objects.getCount(); ++i)
		{
			if (port_objects[i].isMissing())
			{
				break;
			}
		}

		port_last_object_id = setHandle(object, static_cast<OBJCT>(i));
		return port_last_object_id;
	}

	void releaseObject(OBJCT id)
	{
		if (id != INVALID_OBJECT)
		{
			port_objects[id].release();
		}
	}


public:
	// TMN: Beginning of C++ port
	// TMN: ugly, but at least a start
	bool	accept(p_cnct* cnct);
	void	disconnect();
	void	force_close();
	rem_port*	receive(PACKET* pckt);
	XDR_INT	send(PACKET* pckt);
	XDR_INT	send_partial(PACKET* pckt);
	rem_port*	connect(PACKET* pckt);
	rem_port*	request(PACKET* pckt);
	bool		select_multi(UCHAR* buffer, SSHORT bufsize, SSHORT* length, RemPortPtr& port);

#ifdef REM_SERVER
	bool haveRecvData()
	{
		Firebird::RefMutexGuard queGuard(*port_que_sync);
		return ((port_receive.x_handy > 0) || (port_qoffset < port_queue.getCount()));
	}

	void clearRecvQue()
	{
		Firebird::RefMutexGuard queGuard(*port_que_sync);
		port_queue.clear();
		port_qoffset = 0;
		port_receive.x_private = port_receive.x_base;
	}

	class RecvQueState
	{
	public:
		int save_handy;
		size_t save_private;
		size_t save_qoffset;

		RecvQueState(const rem_port* port)
		{
			save_handy = port->port_receive.x_handy;
			save_private = port->port_receive.x_private - port->port_receive.x_base;
			save_qoffset = port->port_qoffset;
		}
	};

	RecvQueState getRecvState() const
	{
		return RecvQueState(this);
	}

	void setRecvState(const RecvQueState& rs)
	{
		if (rs.save_qoffset > 0 && (rs.save_qoffset != port_qoffset))
		{
			Firebird::Array<char>& q = port_queue[rs.save_qoffset - 1];
			memcpy(port_receive.x_base, q.begin(), q.getCount());
		}
		port_qoffset = rs.save_qoffset;
		port_receive.x_private = port_receive.x_base + rs.save_private;
		port_receive.x_handy = rs.save_handy;
	}
#endif // REM_SERVER

	// TMN: The following member functions are conceptually private
	// to server.cpp and should be _made_ private in due time!
	// That is, if we don't factor these method out.

	ISC_STATUS	compile(P_CMPL*, PACKET*);
	ISC_STATUS	ddl(P_DDL*, PACKET*);
	void	disconnect(PACKET*, PACKET*);
	void	drop_database(P_RLSE*, PACKET*);

	ISC_STATUS	end_blob(P_OP, P_RLSE*, PACKET*);
	ISC_STATUS	end_database(P_RLSE*, PACKET*);
	ISC_STATUS	end_request(P_RLSE*, PACKET*);
	ISC_STATUS	end_statement(P_SQLFREE*, PACKET*);
	ISC_STATUS	end_transaction(P_OP, P_RLSE*, PACKET*);
	ISC_STATUS	execute_immediate(P_OP, P_SQLST*, PACKET*);
	ISC_STATUS	execute_statement(P_OP, P_SQLDATA*, PACKET*);
	ISC_STATUS	fetch(P_SQLDATA*, PACKET*);
	ISC_STATUS	fetch_blob(P_SQLDATA*, PACKET*);
	ISC_STATUS	get_segment(P_SGMT*, PACKET*);
	ISC_STATUS	get_slice(P_SLC*, PACKET*);
	ISC_STATUS	info(P_OP, P_INFO*, PACKET*);
	ISC_STATUS	insert(P_SQLDATA*, PACKET*);
	ISC_STATUS	open_blob(P_OP, P_BLOB*, PACKET*);
	ISC_STATUS	prepare(P_PREP*, PACKET*);
	ISC_STATUS	prepare_statement(P_SQLST*, PACKET*);
	ISC_STATUS	put_segment(P_OP, P_SGMT*, PACKET*);
	ISC_STATUS	put_slice(P_SLC*, PACKET*);
	ISC_STATUS	que_events(P_EVENT*, PACKET*);
	ISC_STATUS	receive_after_start(P_DATA*, PACKET*, ISC_STATUS*);
	ISC_STATUS	receive_msg(P_DATA*, PACKET*);
	ISC_STATUS	seek_blob(P_SEEK*, PACKET*);
	ISC_STATUS	send_msg(P_DATA*, PACKET*);
	ISC_STATUS	send_response(PACKET*, OBJCT, USHORT, const ISC_STATUS*, bool);
	ISC_STATUS	service_attach(const char*, const USHORT, Firebird::ClumpletWriter&, PACKET*);
	ISC_STATUS	service_end(P_RLSE*, PACKET*);
	ISC_STATUS	service_start(P_INFO*, PACKET*);
	ISC_STATUS	set_cursor(P_SQLCUR*, PACKET*);
	ISC_STATUS	start(P_OP, P_DATA*, PACKET*);
	ISC_STATUS	start_and_send(P_OP, P_DATA*, PACKET*);
	ISC_STATUS	start_transaction(P_OP, P_STTR*, PACKET*);
	ISC_STATUS	transact_request(P_TRRQ *, PACKET*);
	SSHORT		asyncReceive(PACKET* asyncPacket, const UCHAR* buffer, SSHORT dataSize);
};



// Queuing structure for Client batch fetches

typedef bool (*t_rmtque_fn)(rem_port*, rmtque*, ISC_STATUS*, USHORT);

struct rmtque : public Firebird::GlobalStorage
{
	rmtque*				rmtque_next;	// Next entry in queue
	void*				rmtque_parm;	// What request has response in queue
	Rrq::rrq_repeat*	rmtque_message;	// What message is pending
	Rdb*				rmtque_rdb;		// What database has pending msg

	// Fn that receives queued entry
	t_rmtque_fn			rmtque_function;

public:
	rmtque() :
		rmtque_next(0), rmtque_parm(0), rmtque_message(0), rmtque_rdb(0), rmtque_function(0)
	{ }
};


// contains ports which must be closed at engine shutdown
class PortsCleanup
{
public:
	PortsCleanup() :
	  m_ports(NULL),
	  m_mutex()
	{}

	explicit PortsCleanup(MemoryPool&) :
	  m_ports(NULL),
	  m_mutex()
	{}

	~PortsCleanup()
	{}

	void registerPort(rem_port*);
	void unRegisterPort(rem_port*);

	void closePorts();

private:
	typedef Firebird::SortedArray<rem_port*> PortsArray;
	PortsArray*		m_ports;
	Firebird::Mutex	m_mutex;
};

#endif // REMOTE_REMOTE_H
