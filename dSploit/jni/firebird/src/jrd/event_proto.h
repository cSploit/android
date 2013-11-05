/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		event_proto.h
 *	DESCRIPTION:	Prototype Header file for event.cpp
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

#ifndef JRD_EVENT_PROTO_H
#define JRD_EVENT_PROTO_H

#include "../common/classes/init.h"
#include "../common/classes/semaphore.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/RefCounted.h"
#include "../jrd/ThreadData.h"
#include "../jrd/event.h"

namespace Jrd {

class Attachment;

class EventManager : private Firebird::RefCounted, public Firebird::GlobalStorage
{
	typedef Firebird::GenericMap<Firebird::Pair<Firebird::Left<Firebird::string, EventManager*> > > DbEventMgrMap;

	static Firebird::GlobalPtr<DbEventMgrMap> g_emMap;
	static Firebird::GlobalPtr<Firebird::Mutex> g_mapMutex;

	const int PID;

public:
	static void init(Attachment*);
	static void destroy(EventManager*);

	explicit EventManager(const Firebird::string&);
	~EventManager();

	void deleteSession(SLONG);

	SLONG queEvents(SLONG, USHORT, const TEXT*, USHORT, const UCHAR*,
				    FPTR_EVENT_CALLBACK, void*);
	void cancelEvents(SLONG);
	void postEvent(USHORT, const TEXT*, USHORT, const TEXT*, USHORT);
	void deliverEvents();

private:
	evh* acquire_shmem();
	frb* alloc_global(UCHAR type, ULONG length, bool recurse);
	void create_process();
	SLONG create_session();
	void delete_event(evnt*);
	void delete_process(SLONG);
	void delete_request(evt_req*);
	void delete_session(SLONG);
	void deliver();
	void deliver_request(evt_req*);
	void exit_handler(void *);
	evnt* find_event(USHORT, const TEXT*, evnt*);
	void free_global(frb*);
	req_int* historical_interest(ses*, SLONG);
	void init_shmem(sh_mem*, bool);
	void insert_tail(srq*, srq*);
	evnt* make_event(USHORT, const TEXT*, SLONG);
	bool post_process(prb*);
	void probe_processes();
	void release_shmem();
	void remove_que(srq*);
	bool request_completed(evt_req*);
	void watcher_thread();
	void attach_shared_file();
	void detach_shared_file();
	void get_shared_file_name(Firebird::PathName&) const;

	static THREAD_ENTRY_DECLARE watcher_thread(THREAD_ENTRY_PARAM arg)
	{
		EventManager* const eventMgr = static_cast<EventManager*>(arg);
		eventMgr->watcher_thread();
		return 0;
	}

	static void init_shmem(void* arg, sh_mem* shmem, bool init)
	{
		EventManager* const eventMgr = static_cast<EventManager*>(arg);
		eventMgr->init_shmem(shmem, init);
	}

	static void mutex_bugcheck(const TEXT*, int);
	static void punt(const TEXT*);

	evh* m_header;
	prb* m_process;
	SLONG m_processOffset;
	sh_mem m_shmemData;

	Firebird::string m_dbId;

	Firebird::Semaphore m_startupSemaphore;
	Firebird::Semaphore m_cleanupSemaphore;

#ifdef WIN_NT
	struct mtx m_mutex;
#else
	struct mtx* m_mutex;
#endif

	bool m_sharedFileCreated;
	bool m_exiting;
};

} // namespace

#endif // JRD_EVENT_PROTO_H

