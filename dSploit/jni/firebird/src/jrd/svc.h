/*
 *	PROGRAM:	JRD access method
 *	MODULE:		svc.h
 *	DESCRIPTION:	Service manager declarations
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

#ifndef JRD_SVC_H
#define JRD_SVC_H

#include <stdio.h>

#include "fb_blk.h"

#include "../jrd/jrd_pwd.h"
#include "../jrd/svc_undoc.h"
#include "../jrd/ThreadStart.h"

#include "../common/classes/semaphore.h"
#include "../common/classes/array.h"
#include "../common/classes/SafeArg.h"
#include "../common/UtilSvc.h"
#include "../burp/spit.h"

#ifndef IO_BUFFER_SIZE
#ifdef BUFSIZ
const int SVC_IO_BUFFER_SIZE = (16 * (BUFSIZ));
#else // BUFSIZ
const int SVC_IO_BUFFER_SIZE = (16 * (1024));
#endif // BUFSIZ
#else // IO_BUFFER_SIZE
const int SVC_IO_BUFFER_SIZE = (16 * (IO_BUFFER_SIZE));
#endif // IO_BUFFER_SIZE

// forward decl.
struct serv_entry;
namespace Firebird {
	class ClumpletReader;
	namespace Arg {
		class StatusVector;
	}
}

namespace Jrd {

const ULONG SERVICE_VERSION			= 2;

const int SVC_STDOUT_BUFFER_SIZE	= 1024;

/* Flag of capabilities supported by the server */
//const ULONG WAL_SUPPORT					= 0x1L;	// Write Ahead Log
const ULONG MULTI_CLIENT_SUPPORT		= 0x2L;	/* SuperServer model (vs. multi-inet) */
const ULONG REMOTE_HOP_SUPPORT			= 0x4L;	/* Server can connect to other server */
//const ULONG NO_SVR_STATS_SUPPORT		= 0x8L;	// Does not support statistics

//const ULONG NO_DB_STATS_SUPPORT			= 0x10L;	// Does not support statistics
// Really the 16 bit LIBS here?
//const ULONG LOCAL_ENGINE_SUPPORT		= 0x20L;	// The local 16 bit engine
//const ULONG NO_FORCED_WRITE_SUPPORT		= 0x40L;	// Can not configure sync writes
//const ULONG NO_SHUTDOWN_SUPPORT			= 0x80L;	// Can not shutdown/restart databases
const ULONG NO_SERVER_SHUTDOWN_SUPPORT	= 0x100L;	/* Can not shutdown server */
const ULONG SERVER_CONFIG_SUPPORT		= 0x200L;	/* Can configure server */
const ULONG QUOTED_FILENAME_SUPPORT		= 0x400L;	/* Can pass quoted filenames in */

/* Range definitions for service actions.  Any action outside of
   this range is not supported */
const USHORT isc_action_min				= 1;
const USHORT isc_action_max				= isc_action_svc_last;

/* Range definitions for service actions.  Any action outside of
   this range is not supported */
//define isc_info_min                  50
//define isc_info_max                  67

/* Bitmask values for the svc_flags variable */
const int SVC_shutdown		= 0x1;
const int SVC_timeout		= 0x2;
//const int SVC_forked		= 0x4;
const int SVC_detached		= 0x8;
const int SVC_finished		= 0x10;
const int SVC_thd_running	= 0x20;
const int SVC_evnt_fired	= 0x40;
const int SVC_cmd_line		= 0x80;

// forward decl.
class thread_db;
class TraceManager;

// Service manager
class Service : public Firebird::UtilSvc, public TypedHandle<type_svc>
{
public:		// utilities interface with service
	// output to svc_stdout verbose info
	virtual void outputVerbose(const char* text);
	// outpur error text
	virtual void outputError(const char* text);
	// output some data to service
	virtual void outputData(const char* text);
	// printf() to svc_stdout
    virtual void printf(bool err, const SCHAR* format, ...);
	// returns true - it's service :)
	virtual bool isService();
	// client thread started
	virtual void started();
	// client thread finished
	virtual void finish();
	// put various info items in info buffer
    virtual void putLine(char tag, const char* val);
    virtual void putSLong(char tag, SLONG val);
	virtual void putChar(char tag, char val);
	// put raw bytes to svc_stdout
	virtual void putBytes(const UCHAR*, size_t);
	// get raw bytes from svc_stdin
	virtual ULONG getBytes(UCHAR*, ULONG);
	// append status_vector to service's status
	virtual void setServiceStatus(const ISC_STATUS* status_vector);
	// append error message to service's status
	virtual void setServiceStatus(const USHORT facility, const USHORT errcode, const MsgFormat::SafeArg& args);
	// no-op for services
	virtual void hidePasswd(ArgvType&, int);
	// return service status
    virtual const ISC_STATUS* getStatus();
	// reset service status
	virtual void initStatus();
	// no-op for services
	virtual void checkService();
	// add address path (taken from spb) to dpb if present
	virtual void getAddressPath(Firebird::ClumpletWriter& dpb);

	virtual TraceManager* getTraceManager()
	{
		return svc_trace_manager;
	}

	virtual bool finished()
	{
		return ((svc_flags & (SVC_finished | SVC_detached)) != 0)
			|| checkForShutdown();
	}

public:		// external interface with service
	// Attach - service ctor
	Service(const TEXT* service_name, USHORT spb_length, const UCHAR* spb_data);
	// Start service thread
	void start(USHORT spb_length, const UCHAR* spb_data);
	// Query service state (v. 1 & 2)
	void query(USHORT send_item_length, const UCHAR* send_items, USHORT recv_item_length,
			   const UCHAR* recv_items, USHORT buffer_length, UCHAR* info);
	ISC_STATUS query2(thread_db* tdbb, USHORT send_item_length, const UCHAR* send_items,
			   USHORT recv_item_length, const UCHAR* recv_items, USHORT buffer_length, UCHAR* info);
	// Detach from service
	void detach();
	// get service version
	USHORT getVersion() const
	{
		return svc_spb_version;
	}

	// Firebird log reader
	static THREAD_ENTRY_DECLARE readFbLog(THREAD_ENTRY_PARAM arg);
	// Shuts all service threads (should be called after databases shutdown)
	static void shutdownServices();

	// Total number of service attachments
	static ULONG totalCount();

	const char* getServiceMgr() const;
	const char* getServiceName() const;

	const Firebird::string&	getUserName() const
	{
		if (svc_username.empty())
			return svc_trusted_login;

		return svc_username;
	}

	const Firebird::string&	getNetworkProtocol() const	{ return svc_network_protocol; }
	const Firebird::string&	getRemoteAddress() const	{ return svc_remote_address; }
	const Firebird::string&	getRemoteProcess() const	{ return svc_remote_process; }
	int	getRemotePID() const { return svc_remote_pid; }

private:
	// Service must have private destructor, called from finish
	// when both (server and client) threads are finished
	~Service();
	// Find current service in global services list
	bool	locateInAllServices(size_t* posPtr = NULL);
	// Detach self from global services list
	void	removeFromAllServices();
	// The only service, implemented internally
	void	readFbLog();
	// Create argv, argc and svc_parsed_sw
	void	parseSwitches();
	// Put data into stdout buffer
	void	enqueue(const UCHAR* s, ULONG len);
	// true if there is no data in stdout buffer
	bool	empty() const;
	// true if no more space in stdout buffer
	bool	full() const;
	// start service thread
	void	start(ThreadEntryPoint* service_thread);
	// Set the flag (either SVC_finished for the main service thread or SVC_detached for the client thread).
	// If both main thread and client thread are completed that is main thread is finished and
	// client is detached then free memory used by service.
	void	finish(USHORT flag);
	// Throws shutdown exception if global flag is set for it
	bool	checkForShutdown();
	// Transfer data from svc_stdout into buffer
	void	get(UCHAR* buffer, USHORT length, USHORT flags, USHORT timeout, USHORT* return_length);
	// Sends stdin for a service
	// Returns number of bytes service wants more
	ULONG	put(const UCHAR* buffer, ULONG length);

	// Increment circular buffer pointer
	static ULONG		add_one(ULONG i);
	static ULONG		add_val(ULONG i, ULONG val);
	// Convert spb flags to utility switches
	static void			conv_switches(Firebird::ClumpletReader& spb, Firebird::string& switches);
	// Find spb switch in switch table
	static const TEXT*	find_switch(int in_spb_sw, const in_sw_tab_t* table);
	// Loop through the appropriate switch table looking for the text for the given command switch
	static bool			process_switches(Firebird::ClumpletReader& spb, Firebird::string& switches);
	// Get bitmask from within spb buffer, find corresponding switches within specified table,
	// add them to the command line
	static bool get_action_svc_bitmask(const Firebird::ClumpletReader& spb, const in_sw_tab_t* table,
									   Firebird::string& sw);
	// Get string from within spb buffer, add it to the command line
	static void get_action_svc_string(const Firebird::ClumpletReader& spb, Firebird::string& sw);
	// Get integer from within spb buffer, add it to the command line
	static void get_action_svc_data(const Firebird::ClumpletReader& spb, Firebird::string& sw);
	// Get parameter from within spb buffer, find corresponding switch within specified table,
	// add it to the command line
	static bool get_action_svc_parameter(UCHAR tag, const in_sw_tab_t* table, Firebird::string&);
	// Create 'SYSDBA needed' error in status vector
	static void need_admin_privs(Firebird::Arg::StatusVector& status, const char* message);
	// Does info buffer have enough space for SLONG?
	static bool ck_space_for_numeric(UCHAR*& info, const UCHAR* const end);
	// Make status vector permamnent, if one present in worker thread's space
	void makePermanentStatusVector() throw();

private:
	ISC_STATUS_ARRAY svc_status;		// status vector for running service
	Firebird::string svc_parsed_sw;		// Here point elements of argv
	ULONG	svc_stdout_head;
	ULONG	svc_stdout_tail;
	UCHAR	svc_stdout[SVC_STDOUT_BUFFER_SIZE];		// output from service
	Firebird::Semaphore	svcStart;
	const serv_entry*	svc_service;			// attached service's entry
	const serv_entry*	svc_service_run;		// running service's entry
	Firebird::Array<UCHAR> svc_resp_alloc;
	UCHAR*	svc_resp_buf;
	const UCHAR*	svc_resp_ptr;
	USHORT	svc_resp_buf_len;
	USHORT	svc_resp_len;
	USHORT	svc_flags;
	USHORT	svc_user_flag;
	USHORT	svc_spb_version;
	bool	svc_do_shutdown;

	Firebird::string	svc_username;
	Firebird::string	svc_enc_password;
	Firebird::string	svc_trusted_login;
	bool                svc_trusted_role;
	bool				svc_uses_security_database;
	Firebird::string	svc_switches;	// Full set of switches
	Firebird::string	svc_perm_sw;	// Switches, taken from services table and/or passed using spb_command_line
	Firebird::string	svc_address_path;

	Firebird::string	svc_network_protocol;
	Firebird::string	svc_remote_address;
	Firebird::string	svc_remote_process;
	SLONG				svc_remote_pid;

	TraceManager*		svc_trace_manager;

public:
	struct StatusStringsHelper
	{
		FB_THREAD_ID workerThread;
		Firebird::Mutex mtx;
	};

	Firebird::Semaphore	svc_detach_sem;

private:
	StatusStringsHelper	svc_thread_strings;

	Firebird::Semaphore svc_sem_empty, svc_sem_full;

	//Service existence guard
	class ExistenceGuard
	{
	public:
		explicit ExistenceGuard(Service* svc);
		~ExistenceGuard();
		void release();
	private:
		Service* svc;
		bool locked;
	};
	friend class ExistenceGuard;
	Firebird::Mutex		svc_existence_lock;
	ExistenceGuard*		svc_current_guard;

	// Data pipe from client to service
	Firebird::Semaphore svc_stdin_semaphore;
	Firebird::Mutex svc_stdin_mutex;
	// Size of data, requested by service (set in getBytes, reset in put)
	ULONG svc_stdin_size_requested;
	// Buffer passed by service
	UCHAR* svc_stdin_buffer;
	// Size of data, preloaded by user (set in put, reset in getBytes)
	ULONG svc_stdin_size_preload;
	// Buffer for datam preloaded by user
	Firebird::AutoPtr<UCHAR> svc_stdin_preload;
	// Size of data, requested from user to preload (set in getBytes)
	ULONG svc_stdin_preload_requested;
	// Size of data, placed into svc_stdin_buffer (set in put)
	ULONG svc_stdin_user_size;
	static const ULONG PRELOAD_BUFFER_SIZE = SVC_IO_BUFFER_SIZE;
};

} //namespace Jrd

#endif // JRD_SVC_H
