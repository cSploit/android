/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		event.cpp
 *	DESCRIPTION:	Event Manager
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
 */

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gen/iberror.h"
#include "../common/classes/init.h"
#include "../common/config/config.h"
#include "../common/ThreadStart.h"
#include "../jrd/event.h"
#include "../common/gdsassert.h"
#include "../jrd/event_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/isc_proto.h"
#include "../common/isc_s_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/err_proto.h"
#include "../common/os/isc_i_proto.h"
#include "../common/utils_proto.h"
#include "../jrd/Database.h"
#include "../jrd/Attachment.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef UNIX
#include <signal.h>
#endif

#ifdef WIN_NT
#include <process.h>
#endif

#define SRQ_BASE                  ((UCHAR*) m_sharedMemory->getHeader())

using namespace Firebird;

namespace Jrd {

GlobalPtr<EventManager::DbEventMgrMap> EventManager::g_emMap;
GlobalPtr<Mutex> EventManager::g_mapMutex;


void EventManager::init(Attachment* attachment)
{
	Database* const dbb = attachment->att_database;
	EventManager* eventMgr = dbb->dbb_event_mgr;

	if (!eventMgr)
	{
		const string id = dbb->getUniqueFileId();

		MutexLockGuard guard(g_mapMutex, FB_FUNCTION);

		if (!g_emMap->get(id, eventMgr))
		{
			eventMgr = new EventManager(id, dbb->dbb_config);

			if (g_emMap->put(id, eventMgr))
			{
				fb_assert(false);
			}
		}

		fb_assert(eventMgr);

		eventMgr->addRef();
		dbb->dbb_event_mgr = eventMgr;
	}

	if (!attachment->att_event_session)
		attachment->att_event_session = eventMgr->create_session();
}


void EventManager::destroy(EventManager* eventMgr)
{
	if (eventMgr)
	{
		const Firebird::string id = eventMgr->m_dbId;

		Firebird::MutexLockGuard guard(g_mapMutex, FB_FUNCTION);

		if (!eventMgr->release())
		{
			if (!g_emMap->remove(id))
			{
				fb_assert(false);
			}
		}
	}
}


EventManager::EventManager(const Firebird::string& id, Firebird::RefPtr<Config> conf)
	: PID(getpid()),
	  m_process(NULL),
	  m_processOffset(0),
	  m_dbId(getPool(), id),
	  m_config(conf),
	  m_sharedFileCreated(false),
	  m_exiting(false)
{
	attach_shared_file();
}


EventManager::~EventManager()
{
	m_exiting = true;
	const SLONG process_offset = m_processOffset;

	Arg::StatusVector localStatus;

	if (m_process)
	{
		// Terminate the event watcher thread
		m_startupSemaphore.tryEnter(5);
		(void) m_sharedMemory->eventPost(&m_process->prb_event);
		m_cleanupSemaphore.tryEnter(5);

#ifdef HAVE_OBJECT_MAP
		m_sharedMemory->unmapObject(localStatus, &m_process);
#else
		m_process = NULL;
#endif
	}

	acquire_shmem();
	m_processOffset = 0;
	if (process_offset)
	{
		delete_process(process_offset);
	}
	if (m_sharedMemory->getHeader() && SRQ_EMPTY(m_sharedMemory->getHeader()->evh_processes))
	{
		m_sharedMemory->removeMapFile();
	}
	release_shmem();

	detach_shared_file();
}


void EventManager::attach_shared_file()
{
	Firebird::PathName name;
	get_shared_file_name(name);

	SharedMemory<evh>* tmp = FB_NEW(*getDefaultMemoryPool())
		SharedMemory<evh>(name.c_str(), m_config->getEventMemSize(), this);

	// initialize will reset m_sharedMemory
	fb_assert(m_sharedMemory == tmp);
	fb_assert(m_sharedMemory->getHeader()->mhb_type == SharedMemoryBase::SRAM_EVENT_MANAGER);
	fb_assert(m_sharedMemory->getHeader()->mhb_version == EVENT_VERSION);
}


void EventManager::detach_shared_file()
{
	delete m_sharedMemory.release();
}


void EventManager::get_shared_file_name(Firebird::PathName& name) const
{
	name.printf(EVENT_FILE, m_dbId.c_str());
}


SLONG EventManager::create_session()
{
/**************************************
 *
 *	c r e a t e S e s s i o n
 *
 **************************************
 *
 * Functional description
 *	Create session.
 *
 **************************************/
	if (!m_processOffset)
	{
		create_process();
	}

	acquire_shmem();

	ses* const session = (ses*) alloc_global(type_ses, sizeof(ses), false);
	prb* const process = (prb*) SRQ_ABS_PTR(m_processOffset);
	session->ses_flags = 0;

	insert_tail(&process->prb_sessions, &session->ses_sessions);
	SRQ_INIT(session->ses_requests);
	const SLONG id = SRQ_REL_PTR(session);

	release_shmem();

	return id;
}


void EventManager::deleteSession(SLONG session_id)
{
/**************************************
 *
 *	d e l e t e S e s s i o n
 *
 **************************************
 *
 * Functional description
 *	Delete a session.
 *
 **************************************/
	acquire_shmem();
	delete_session(session_id);
	release_shmem();
}


SLONG EventManager::queEvents(SLONG session_id,
							  USHORT events_length, const UCHAR* events,
							  Firebird::IEventCallback* ast)
{
/**************************************
 *
 *	q u e E v e n t s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	// Sanity check

	if (events_length && (!events || events[0] != EPB_version1))
	{
		Firebird::Arg::Gds(isc_bad_epb_form).raise();
	}

	acquire_shmem();

	// Allocate request block

	evt_req* request = (evt_req*) alloc_global(type_reqb, sizeof(evt_req), false);
	ses* session = (ses*) SRQ_ABS_PTR(session_id);
	insert_tail(&session->ses_requests, &request->req_requests);
	request->req_session = session_id;
	request->req_process = m_processOffset;
	request->req_ast = ast;
	const SLONG id = ++(m_sharedMemory->getHeader()->evh_request_id);
	request->req_request_id = id;

	const SLONG request_offset = SRQ_REL_PTR(request);

	// Process event block

	SRQ_PTR* ptr = &request->req_interests;
	SLONG ptr_offset = SRQ_REL_PTR(ptr);
	const UCHAR* p = events + 1;
	const UCHAR* const end = events + events_length;
	bool flag = false;

	while (p < end)
	{
		const USHORT count = *p++;

		// Sanity check

		if (count > end - events)
		{
			release_shmem();
			Firebird::Arg::Gds(isc_bad_epb_form).raise();
		}

		// The data in the event block may have trailing blanks. Strip them off.

        const UCHAR* find_end = p + count;
		while (--find_end >= p && *find_end == ' ')
			; // nothing to do.
		const USHORT len = find_end - p + 1;

		evnt* event = find_event(len, reinterpret_cast<const char*>(p));
		if (!event)
		{
			event = make_event(len, reinterpret_cast<const char*>(p));
			session = (ses*) SRQ_ABS_PTR(session_id);
			request = (evt_req*) SRQ_ABS_PTR(request_offset);
			ptr = (SRQ_PTR *) SRQ_ABS_PTR(ptr_offset);
		}

		p += count;
		const SLONG event_offset = SRQ_REL_PTR(event);

		req_int* interest, *prior;
		if (interest = historical_interest(session, event_offset))
		{
			for (SRQ_PTR* ptr2 = &session->ses_interests;
				 *ptr2 && (prior = (req_int*) SRQ_ABS_PTR(*ptr2));
				 ptr2 = &prior->rint_next)
			{
				if (prior == interest)
				{
					*ptr2 = interest->rint_next;
					interest->rint_next = 0;
					break;
				}
			}
		}
		else
		{
			interest = (req_int*) alloc_global(type_rint, sizeof(req_int), false);
			event = (evnt*) SRQ_ABS_PTR(event_offset);
			insert_tail(&event->evnt_interests, &interest->rint_interests);
			interest->rint_event = event_offset;

			request = (evt_req*) SRQ_ABS_PTR(request_offset);
			ptr = (SRQ_PTR *) SRQ_ABS_PTR(ptr_offset);
			session = (ses*) SRQ_ABS_PTR(session_id);
		}

		*ptr = SRQ_REL_PTR(interest);
		ptr = &interest->rint_next;
		ptr_offset = SRQ_REL_PTR(ptr);

		interest->rint_request = request_offset;
		interest->rint_count = gds__vax_integer(p, 4);
		p += 4;
		if (interest->rint_count <= event->evnt_count)
		{
			flag = true;
		}
	}

	if (flag)
	{
		if (!post_process((prb*) SRQ_ABS_PTR(m_processOffset)))
		{
			release_shmem();
			(Firebird::Arg::Gds(isc_random) << "post_process() failed").raise();
		}
	}

	release_shmem();

	return id;
}


void EventManager::cancelEvents(SLONG request_id)
{
/**************************************
 *
 *	c a n c e l E v e n t s
 *
 **************************************
 *
 * Functional description
 *	Cancel an outstanding event.
 *
 **************************************/
	acquire_shmem();

	prb* const process = (prb*) SRQ_ABS_PTR(m_processOffset);

	srq* que2;
	SRQ_LOOP(process->prb_sessions, que2)
	{
		ses* const session = (ses*) ((UCHAR*) que2 - OFFSET(ses*, ses_sessions));
		srq* event_srq;
		SRQ_LOOP(session->ses_requests, event_srq)
		{
			evt_req* const request = (evt_req*) ((UCHAR*) event_srq - OFFSET(evt_req*, req_requests));
			if (request->req_request_id == request_id)
			{
				delete_request(request);
				release_shmem();
				return;
			}
		}
	}

	release_shmem();
}


void EventManager::postEvent(USHORT length, const TEXT* string, USHORT count)
{
/**************************************
 *
 *	p o s t E v e n t
 *
 **************************************
 *
 * Functional description
 *	Post an event.
 *
 **************************************/
	acquire_shmem();

	evnt* const event = find_event(length, string);

	if (event)
	{
		event->evnt_count += count;
		srq* event_srq;
		SRQ_LOOP(event->evnt_interests, event_srq)
		{
			req_int* const interest = (req_int*) ((UCHAR*) event_srq - OFFSET(req_int*, rint_interests));
			if (interest->rint_request)
			{
				evt_req* const request = (evt_req*) SRQ_ABS_PTR(interest->rint_request);

				if (interest->rint_count <= event->evnt_count)
				{
					prb* const process = (prb*) SRQ_ABS_PTR(request->req_process);
					process->prb_flags |= PRB_wakeup;
				}
			}
		}
	}

	release_shmem();
}


void EventManager::deliverEvents()
{
/**************************************
 *
 *	d e l i v e r E v e n t s
 *
 **************************************
 *
 * Functional description
 *	Post an event (step 2).
 *
 *  This routine is called by DFW_perform_post_commit_work()
 *  once all pending events are prepared
 *  for delivery with postEvent().
 *
 **************************************/
	acquire_shmem();

	// Deliver requests for posted events

	bool flag = true;

	while (flag)
	{
		flag = false;
		srq* event_srq;
		SRQ_LOOP (m_sharedMemory->getHeader()->evh_processes, event_srq)
		{
			prb* const process = (prb*) ((UCHAR*) event_srq - OFFSET (prb*, prb_processes));
			if (process->prb_flags & PRB_wakeup)
			{
				if (!post_process(process))
				{
					release_shmem();
					(Firebird::Arg::Gds(isc_random) << "post_process() failed").raise();
				}
				flag = true;
				break;
			}
		}
	}

	release_shmem();
}


void EventManager::acquire_shmem()
{
/**************************************
 *
 *	a c q u i r e _ s h m e m
 *
 **************************************
 *
 * Functional description
 *	Acquire exclusive access to shared global region.
 *
 **************************************/

	m_sharedMemory->mutexLock();

	// Check for shared memory state consistency

	while (SRQ_EMPTY(m_sharedMemory->getHeader()->evh_processes))
	{
		if (! m_sharedFileCreated)
		{
			// Someone is going to delete shared file? Reattach.
			m_sharedMemory->mutexUnlock();
			detach_shared_file();

			THD_yield();

			attach_shared_file();
			m_sharedMemory->mutexLock();
		}
		else
		{
			// complete initialization
			m_sharedFileCreated = false;

			break;
		}
	}
	fb_assert(!m_sharedFileCreated);

	m_sharedMemory->getHeader()->evh_current_process = m_processOffset;

	if (m_sharedMemory->getHeader()->evh_length > m_sharedMemory->sh_mem_length_mapped)
	{
		const ULONG length = m_sharedMemory->getHeader()->evh_length;

#ifdef HAVE_OBJECT_MAP
		Arg::StatusVector localStatus;
		if (!m_sharedMemory->remapFile(localStatus, length, false))
#endif
		{
			release_shmem();
			iscLogStatus("Event table remap failed", localStatus.value());
			exit(FINI_ERROR);
		}
	}
}


frb* EventManager::alloc_global(UCHAR type, ULONG length, bool recurse)
{
/**************************************
 *
 *	a l l o c _ g l o b a l
 *
 **************************************
 *
 * Functional description
 *	Allocate a block in shared global region.
 *
 **************************************/
	frb* free;
	SLONG best_tail = MAX_SLONG;

	length = FB_ALIGN(length, FB_ALIGNMENT);
	SRQ_PTR* best = NULL;

	for (SRQ_PTR* ptr = &m_sharedMemory->getHeader()->evh_free;
		 (free = (frb*) SRQ_ABS_PTR(*ptr)) && *ptr;
		 ptr = &free->frb_next)
	{
		const SLONG tail = free->frb_header.hdr_length - length;
		if (tail >= 0 && (!best || tail < best_tail))
		{
			best = ptr;
			best_tail = tail;
		}
	}

#ifdef HAVE_OBJECT_MAP
	if (!best && !recurse)
	{
		const ULONG old_length = m_sharedMemory->sh_mem_length_mapped;
		const ULONG ev_length = old_length + m_config->getEventMemSize();

		Arg::StatusVector localStatus;
		if (m_sharedMemory->remapFile(localStatus, ev_length, true))
		{
			free = (frb*) (((UCHAR*) m_sharedMemory->getHeader()) + old_length);
			//free->frb_header.hdr_length = EVENT_EXTEND_SIZE - sizeof (struct evh);
			free->frb_header.hdr_length = m_sharedMemory->sh_mem_length_mapped - old_length;
			free->frb_header.hdr_type = type_frb;
			free->frb_next = 0;

			m_sharedMemory->getHeader()->evh_length = m_sharedMemory->sh_mem_length_mapped;

			free_global(free);

			return alloc_global(type, length, true);
		}
	}
#endif

	if (!best)
	{
		release_shmem();
		fb_utils::logAndDie("Event table space exhausted");
	}

	free = (frb*) SRQ_ABS_PTR(*best);

	if (best_tail < (SLONG) sizeof(frb))
	{
		*best = free->frb_next;
	}
	else
	{
		free->frb_header.hdr_length -= length;
		free = (frb*) ((UCHAR*) free + free->frb_header.hdr_length);
		free->frb_header.hdr_length = length;
	}

	memset((UCHAR*) free + sizeof(event_hdr), 0, free->frb_header.hdr_length - sizeof(event_hdr));
	free->frb_header.hdr_type = type;

	return free;
}


void EventManager::create_process()
{
/**************************************
 *
 *	c r e a t e _ p r o c e s s
 *
 **************************************
 *
 * Functional description
 *	Create process block unless it already exists.
 *
 **************************************/
	acquire_shmem();

	prb* const process = (prb*) alloc_global(type_prb, sizeof(prb), false);
	process->prb_process_id = PID;
	insert_tail(&m_sharedMemory->getHeader()->evh_processes, &process->prb_processes);
	SRQ_INIT(process->prb_sessions);

	if (m_sharedMemory->eventInit(&process->prb_event) != FB_SUCCESS)
	{
		release_shmem();
		(Firebird::Arg::Gds(isc_random) << "eventInit() failed").raise();
	}

	m_processOffset = SRQ_REL_PTR(process);

#ifdef HAVE_OBJECT_MAP
	Arg::StatusVector localStatus;
	m_process = m_sharedMemory->mapObject<prb>(localStatus, m_processOffset);

	if (!m_process)
	{
		release_shmem();
		localStatus.raise();
	}
#else
	m_process = process;
#endif

	probe_processes();

	release_shmem();

	Thread::start(watcher_thread, this, THREAD_medium);
}


void EventManager::delete_event(evnt* event)
{
/**************************************
 *
 *	d e l e t e _ e v e n t
 *
 **************************************
 *
 * Functional description
 *	Delete an unused and unloved event.
 *
 **************************************/
	remove_que(&event->evnt_events);
	free_global((frb*) event);
}


void EventManager::delete_process(SLONG process_offset)
{
/**************************************
 *
 *	d e l e t e _ p r o c e s s
 *
 **************************************
 *
 * Functional description
 *	Delete a process block including friends and relations.
 *
 **************************************/
	prb* const process = (prb*) SRQ_ABS_PTR(process_offset);

	// Delete any open sessions

	while (!SRQ_EMPTY(process->prb_sessions))
	{
		ses* const session = (ses*) ((UCHAR*) SRQ_NEXT(process->prb_sessions) - OFFSET(ses*, ses_sessions));
		delete_session(SRQ_REL_PTR(session));
	}

	m_sharedMemory->eventFini(&process->prb_event);

	// Untangle and release process block

	remove_que(&process->prb_processes);
	free_global((frb*) process);
}


void EventManager::delete_request(evt_req* request)
{
/**************************************
 *
 *	d e l e t e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Release an unwanted and unloved request.
 *
 **************************************/
	ses* const session = (ses*) SRQ_ABS_PTR(request->req_session);

	while (request->req_interests)
	{
		req_int* const interest = (req_int*) SRQ_ABS_PTR(request->req_interests);

		request->req_interests = interest->rint_next;
		if (historical_interest(session, interest->rint_event))
		{
			remove_que(&interest->rint_interests);
			free_global((frb*) interest);
		}
		else
		{
			interest->rint_next = session->ses_interests;
			session->ses_interests = SRQ_REL_PTR(interest);
			interest->rint_request = (SRQ_PTR)0;
		}
	}

	remove_que(&request->req_requests);
	free_global((frb*) request);
}


void EventManager::delete_session(SLONG session_id)
{
/**************************************
 *
 *	d e l e t e _ s e s s i o n
 *
 **************************************
 *
 * Functional description
 *	Delete a session.
 *
 **************************************/
	ses* const session = (ses*) SRQ_ABS_PTR(session_id);

	// if session currently delivered events, delay its deletion until deliver ends
	if (session->ses_flags & SES_delivering)
	{
		session->ses_flags |= SES_purge;

		// give a chance for delivering thread to detect SES_purge flag we just set
		release_shmem();
		THREAD_SLEEP(100);
		acquire_shmem();

		return;
	}

	// Delete all requests

	while (!SRQ_EMPTY(session->ses_requests))
	{
		srq requests = session->ses_requests;
		evt_req* request = (evt_req*) ((UCHAR*) SRQ_NEXT(requests) - OFFSET(evt_req*, req_requests));
		delete_request(request);
	}

	// Delete any historical interests

	while (session->ses_interests)
	{
		req_int* const interest = (req_int*) SRQ_ABS_PTR(session->ses_interests);
		evnt* const event = (evnt*) SRQ_ABS_PTR(interest->rint_event);
		session->ses_interests = interest->rint_next;
		remove_que(&interest->rint_interests);
		free_global((frb*) interest);
		if (SRQ_EMPTY(event->evnt_interests))
		{
			delete_event(event);
		}
	}

	remove_que(&session->ses_sessions);
	free_global((frb*) session);
}


void EventManager::deliver()
{
/**************************************
 *
 *	d e l i v e r
 *
 **************************************
 *
 * Functional description
 *	We've been poked -- deliver any satisfying requests.
 *
 **************************************/
	prb* process = (prb*) SRQ_ABS_PTR(m_processOffset);
	process->prb_flags &= ~PRB_pending;

	srq* que2 = SRQ_NEXT(process->prb_sessions);
	while (que2 != &process->prb_sessions)
	{
		ses* session = (ses*) ((UCHAR*) que2 - OFFSET(ses*, ses_sessions));
		session->ses_flags |= SES_delivering;
		const SLONG session_offset = SRQ_REL_PTR(session);
		const SLONG que2_offset = SRQ_REL_PTR(que2);
		for (bool flag = true; flag;)
		{
			flag = false;
			srq* event_srq;
			SRQ_LOOP(session->ses_requests, event_srq)
			{
				evt_req* request = (evt_req*) ((UCHAR*) event_srq - OFFSET(evt_req*, req_requests));
				if (request_completed(request))
				{
					deliver_request(request);
					process = (prb*) SRQ_ABS_PTR(m_processOffset);
					session = (ses*) SRQ_ABS_PTR(session_offset);
					que2 = (srq *) SRQ_ABS_PTR(que2_offset);
					flag = !(session->ses_flags & SES_purge);
					break;
				}
			}
		}
		session->ses_flags &= ~SES_delivering;
		if (session->ses_flags & SES_purge)
		{
			que2 = SRQ_NEXT((*que2));
			delete_session(SRQ_REL_PTR(session));
			break;
		}
		else
		{
			que2 = SRQ_NEXT((*que2));
		}
	}
}


void EventManager::deliver_request(evt_req* request)
{
/**************************************
 *
 *	d e l i v e r _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Request has been satisfied, send updated event block to user, then
 *	Clean up request.
 *
 **************************************/
	Firebird::HalfStaticArray<UCHAR, BUFFER_MEDIUM> buffer;
	UCHAR* p = buffer.getBuffer(1);

	Firebird::IEventCallback* ast = request->req_ast;

	*p++ = EPB_version1;

	// Loop thru interest block picking up event name, counts, and unlinking stuff

	try
	{
		req_int* interest;
		for (SRQ_PTR next = request->req_interests;
			 next && (interest = (req_int*) SRQ_ABS_PTR(next));
			 next = interest->rint_next)
		{
			//interest = (req_int*) SRQ_ABS_PTR(next); same line as in the condition above
			evnt* const event = (evnt*) SRQ_ABS_PTR(interest->rint_event);

			const size_t length = buffer.getCount();
			const size_t extent = event->evnt_length + sizeof(UCHAR) + sizeof(SLONG);

			if (length + extent > MAX_USHORT)
			{
				Firebird::BadAlloc::raise();
			}

			buffer.grow(length + extent);
			p = buffer.begin() + length;

			*p++ = event->evnt_length;
			memcpy(p, event->evnt_name, event->evnt_length);
			p += event->evnt_length;
			const SLONG count = event->evnt_count + 1;
			*p++ = (UCHAR) (count);
			*p++ = (UCHAR) (count >> 8);
			*p++ = (UCHAR) (count >> 16);
			*p++ = (UCHAR) (count >> 24);
		}
	}
	catch (const Firebird::BadAlloc&)
	{
		gds__log("Out of memory. Failed to post all events.");
	}

	delete_request(request);
	release_shmem();
	ast->eventCallbackFunction(p - buffer.begin(), buffer.begin());
	acquire_shmem();
}


evnt* EventManager::find_event(USHORT length, const TEXT* string)
{
/**************************************
 *
 *	f i n d _ e v e n t
 *
 **************************************
 *
 * Functional description
 *	Lookup an event.
 *
 **************************************/
	srq* event_srq;
	SRQ_LOOP(m_sharedMemory->getHeader()->evh_events, event_srq)
	{
		evnt* const event = (evnt*) ((UCHAR*) event_srq - OFFSET(evnt*, evnt_events));

		if (event->evnt_length == length && !memcmp(string, event->evnt_name, length))
			return event;
	}

	return NULL;
}


void EventManager::free_global(frb* block)
{
/**************************************
 *
 *	f r e e _ g l o b a l
 *
 **************************************
 *
 * Functional description
 *	Free a previous allocated block.
 *
 **************************************/
	SRQ_PTR *ptr;
	frb* free = NULL;

	frb* prior = NULL;
	const SRQ_PTR offset = SRQ_REL_PTR(block);
	block->frb_header.hdr_type = type_frb;

	for (ptr = &m_sharedMemory->getHeader()->evh_free;
		 (free = (frb*) SRQ_ABS_PTR(*ptr)) && *ptr;
		 prior = free, ptr = &free->frb_next)
	{
		if ((SCHAR *) block < (SCHAR *) free)
			break;
	}

	if (offset <= 0 || static_cast<ULONG>(offset) > m_sharedMemory->getHeader()->evh_length ||
		(prior && (UCHAR*) block < (UCHAR*) prior + prior->frb_header.hdr_length))
	{
		punt("free_global: bad block");
		return;
	}

	// Start by linking block into chain

	block->frb_next = *ptr;
	*ptr = offset;

	// Try to merge free block with next block

	if (free && (SCHAR *) block + block->frb_header.hdr_length == (SCHAR *) free)
	{
		block->frb_header.hdr_length += free->frb_header.hdr_length;
		block->frb_next = free->frb_next;
	}

	// Next, try to merge the free block with the prior block

	if (prior && (SCHAR *) prior + prior->frb_header.hdr_length ==	(SCHAR *) block)
	{
		prior->frb_header.hdr_length += block->frb_header.hdr_length;
		prior->frb_next = block->frb_next;
	}
}


req_int* EventManager::historical_interest(ses* session, SRQ_PTR event_offset)
{
/**************************************
 *
 *	h i s t o r i c a l _ i n t e r e s t
 *
 **************************************
 *
 * Functional description
 *	Find a historical interest, if any, of an event with a session.
 *
 **************************************/
	req_int* interest;

	for (SRQ_PTR ptr = session->ses_interests;
		 ptr && (interest = (req_int*) SRQ_ABS_PTR(ptr)); ptr = interest->rint_next)
	{
		if (interest->rint_event == event_offset)
			return interest;
	}

	return NULL;
}


void EventManager::mutexBug(int osErrorCode, const char* text)
{
	mutex_bugcheck(text, osErrorCode);
}


bool EventManager::initialize(SharedMemoryBase* sm, bool init)
{
/**************************************
 *
 *	i n i t _ s h m e m
 *
 **************************************
 *
 * Functional description
 *	Initialize global region header.
 *
 **************************************/

	m_sharedFileCreated = init;

	// reset m_sharedMemory in advance to be able to use SRQ_BASE macro
	m_sharedMemory.reset(reinterpret_cast<SharedMemory<evh>*>(sm));

	if (init)
	{
		evh* header = m_sharedMemory->getHeader();

		header->mhb_type = SharedMemoryBase::SRAM_EVENT_MANAGER;
		header->mhb_version = EVENT_VERSION;
		header->mhb_timestamp = TimeStamp::getCurrentTimeStamp().value();

		header->evh_length = sm->sh_mem_length_mapped;
		header->evh_request_id = 0;

		SRQ_INIT(header->evh_processes);
		SRQ_INIT(header->evh_events);

		frb* const free = (frb*) ((UCHAR*) header + sizeof(evh));
		free->frb_header.hdr_length = sm->sh_mem_length_mapped - sizeof(evh);
		free->frb_header.hdr_type = type_frb;
		free->frb_next = 0;

		header->evh_free = (UCHAR*) free - (UCHAR*) header;
	}

	return true;
}


void EventManager::insert_tail(srq * event_srq, srq * node)
{
/**************************************
 *
 *	i n s e r t _ t a i l
 *
 **************************************
 *
 * Functional description
 *	Insert a node at the tail of a event_srq.
 *
 **************************************/
	node->srq_forward = SRQ_REL_PTR(event_srq);
	node->srq_backward = event_srq->srq_backward;

	srq* const prior = (srq *) SRQ_ABS_PTR(event_srq->srq_backward);
	prior->srq_forward = SRQ_REL_PTR(node);
	event_srq->srq_backward = SRQ_REL_PTR(node);
}


evnt* EventManager::make_event(USHORT length, const TEXT* string)
{
/**************************************
 *
 *	m a k e _ e v e n t
 *
 **************************************
 *
 * Functional description
 *	Allocate a link in an event.
 *
 **************************************/
	evnt* const event = (evnt*) alloc_global(type_evnt, sizeof(evnt) + length, false);

	insert_tail(&m_sharedMemory->getHeader()->evh_events, &event->evnt_events);
	SRQ_INIT(event->evnt_interests);
	event->evnt_length = length;
	memcpy(event->evnt_name, string, length);

	return event;
}


void EventManager::mutex_bugcheck(const TEXT* string, int mutex_state)
{
/**************************************
 *
 *	m u t e x _ b u g c h e c k
 *
 **************************************
 *
 * Functional description
 *	There has been a bugcheck during a mutex operation.
 *	Post the bugcheck.
 *
 **************************************/
	TEXT msg[BUFFER_TINY];

	sprintf(msg, "EVENT: %s error, status = %d", string, mutex_state);
	fb_utils::logAndDie(msg);
}


bool EventManager::post_process(prb* process)
{
/**************************************
 *
 *	p o s t _ p r o c e s s
 *
 **************************************
 *
 * Functional description
 *	Wakeup process.
 *
 **************************************/
	process->prb_flags &= ~PRB_wakeup;
	process->prb_flags |= PRB_pending;
	return m_sharedMemory->eventPost(&process->prb_event) == FB_SUCCESS;
}


void EventManager::probe_processes()
{
/**************************************
 *
 *	p r o b e _ p r o c e s s e s
 *
 **************************************
 *
 * Functional description
 *	Probe a process to see if it still exists.
 *	If it doesn't, get rid of it.
 *
 **************************************/
	srq* event_srq;
	SRQ_LOOP(m_sharedMemory->getHeader()->evh_processes, event_srq)
	{
		prb* const process = (prb*) ((UCHAR*) event_srq - OFFSET(prb*, prb_processes));
		const SLONG process_offset = SRQ_REL_PTR(process);
		if (process_offset != m_processOffset &&
			!ISC_check_process_existence(process->prb_process_id))
		{
			event_srq = (srq *) SRQ_ABS_PTR(event_srq->srq_backward);
			delete_process(process_offset);
		}
	}
}


void EventManager::punt(const TEXT* string)
{
/**************************************
 *
 *	p u n t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	printf("(EVENT) punt: global region corrupt -- %s\n", string);
}


void EventManager::release_shmem()
{
/**************************************
 *
 *	r e l e a s e _ s h m e m
 *
 **************************************
 *
 * Functional description
 *	Release exclusive control of shared global region.
 *
 **************************************/
#ifdef DEBUG_EVENT
	validate();
#endif

	m_sharedMemory->getHeader()->evh_current_process = 0;
	m_sharedMemory->mutexUnlock();
}


void EventManager::remove_que(srq* node)
{
/**************************************
 *
 *	r e m o v e _ q u e
 *
 **************************************
 *
 * Functional description
 *	Remove a node from a self-relative event_srq.
 *
 **************************************/
	srq* event_srq = (srq*) SRQ_ABS_PTR(node->srq_forward);
	event_srq->srq_backward = node->srq_backward;

	event_srq = (srq*) SRQ_ABS_PTR(node->srq_backward);
	event_srq->srq_forward = node->srq_forward;
	node->srq_forward = node->srq_backward = 0;
}


bool EventManager::request_completed(evt_req* request)
{
/**************************************
 *
 *	r e q u e s t _ c o m p l e t e d
 *
 **************************************
 *
 * Functional description
 *	See if request is completed.
 *
 **************************************/
	req_int* interest;
	for (SRQ_PTR next = request->req_interests; next; next = interest->rint_next)
	{
		interest = (req_int*) SRQ_ABS_PTR(next);
		evnt* event = (evnt*) SRQ_ABS_PTR(interest->rint_event);
		if (interest->rint_count <= event->evnt_count)
			return true;
	}

	return false;
}


#ifdef DEBUG_EVENT
int EventManager::validate()
{
/**************************************
 *
 *	v a l i d a t e
 *
 **************************************
 *
 * Functional description
 *	Make sure everything looks ok.
 *
 **************************************/
// Check consistency of global region (debugging only)

	SRQ_PTR next_free = 0;
	ULONG offset;

	for (offset = sizeof(evh); offset < m_sharedMemory->getHeader()->evh_length;
		offset += block->frb_header.hdr_length)
	{
		const event_hdr* block = (event_hdr*) SRQ_ABS_PTR(offset);
		if (!block->frb_header.hdr_length || !block->frb_header.hdr_type ||
			block->frb_header.hdr_type >= type_max)
		{
			punt("bad block length or type");
			break;
		}
		if (next_free)
		{
			if (offset == next_free)
				next_free = 0;
			else if (offset > next_free)
				punt("bad free chain");
		}
		if (block->frb_header.hdr_type == type_frb)
		{
			next_free = ((frb*) block)->frb_next;
			if (next_free >= m_sharedMemory->getHeader()->evh_length)
				punt("bad frb_next");
		}
	}

	if (offset != m_sharedMemory->getHeader()->evh_length)
		punt("bad block length");
}
#endif


void EventManager::watcher_thread()
{
/**************************************
 *
 *	w a t c h e r _ t h r e a d
 *
 **************************************
 *
 * Functional description
 *	Wait for something to happen.
 *
 **************************************/
	bool startup = true;

	try
	{
		while (!m_exiting)
		{
			acquire_shmem();

			prb* process = (prb*) SRQ_ABS_PTR(m_processOffset);
			process->prb_flags &= ~PRB_wakeup;

			const SLONG value = m_sharedMemory->eventClear(&process->prb_event);

			if (process->prb_flags & PRB_pending)
			{
				deliver();
			}

			release_shmem();

			if (startup)
			{
				startup = false;
				m_startupSemaphore.release();
			}

			if (m_exiting)
				break;

			(void) m_sharedMemory->eventWait(&m_process->prb_event, value, 0);
		}

		m_cleanupSemaphore.release();
	}
	catch (const Firebird::Exception& ex)
	{
		iscLogException("Error in event watcher thread\n", ex);
	}
}

} // namespace
