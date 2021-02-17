/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		isc_ipc.cpp
 *	DESCRIPTION:	Handing and posting of signals (Windows)
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
 * Solaris x86 changes - Konstantin Kuznetsov, Neil McCalden
 *
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete ports:
 *                          - EPSON, DELTA, IMP, NCR3000 and M88K
 *
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "UNIXWARE" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 * 2002.08.27 Nickolay Samofatov - create Windows version of this module
 *
 */


#include "firebird.h"
#include "../../../common/classes/init.h"
#include "../../../common/utils_proto.h"
#include "gen/iberror.h"
#include "../yvalve/gds_proto.h"
#include "../common/isc_proto.h"
#include "../common/os/isc_i_proto.h"
#include "../common/isc_s_proto.h"

#include <windows.h>
#include <process.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

// signals may be not defined in MINGW
#ifndef SIG_SGE
#define SIG_SGE (void (__cdecl *)(int))3	// signal gets error
#endif
#ifndef SIG_ACK
#define SIG_ACK (void (__cdecl *)(int))4	// acknowledge
#endif


namespace {

static int process_id		= 0;
const int MAX_OPN_EVENTS	= 40;

class OpenEvents
{
public:
	explicit OpenEvents(Firebird::MemoryPool&)
	{
		memset(&m_events, 0, sizeof(m_events));
		m_count = 0;
		m_clock = 0;
	}

	~OpenEvents()
	{
		process_id = 0;

		Item* evnt = m_events + m_count;
		m_count = 0;
		while (evnt-- > m_events)
			CloseHandle(evnt->handle);
	}

	HANDLE getEvent(SLONG pid, SLONG signal_number)
	{
		Item* oldestEvent = NULL;
		ULONG oldestAge = ~0;

		Item* evnt = m_events;
		const Item* const end = evnt + m_count;
		for (; evnt < end; evnt++)
		{
			if (evnt->pid == pid && evnt->signal == signal_number)
				break;

			if (evnt->age < oldestAge)
			{
				oldestEvent = evnt;
				oldestAge = evnt->age;
			}
		}

		if (evnt >= end)
		{
			HANDLE handle = ISC_make_signal(false, false, pid, signal_number);
			if (!handle)
				return NULL;

			if (m_count < MAX_OPN_EVENTS)
				m_count++;
			else
			{
				evnt = oldestEvent;
				CloseHandle(evnt->handle);
			}

			evnt->pid = pid;
			evnt->signal = signal_number;
			evnt->handle = handle;
		}

		evnt->age = ++m_clock;
		return evnt->handle;
	}

private:
	class Item
	{
	public:
		SLONG pid;
		SLONG signal;	// pseudo-signal number
		HANDLE handle;	// local handle to foreign event
		ULONG age;
	};

	Item m_events[MAX_OPN_EVENTS];
	int m_count;
	ULONG m_clock;
};

}  // namespace

Firebird::GlobalPtr<OpenEvents> openEvents;

int ISC_kill(SLONG pid, SLONG signal_number, void *object_hndl)
{
/**************************************
 *
 *	I S C _ k i l l		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Notify somebody else.
 *
 **************************************/

	// If we're simply trying to poke ourselves, do so directly.
	ISC_signal_init();

	if (pid == process_id)
		return SetEvent(object_hndl) ? 0 : -1;

	HANDLE handle = openEvents->getEvent(pid, signal_number);
	if (!handle)
		return -1;

	return SetEvent(handle) ? 0 : -1;
}

void* ISC_make_signal(bool create_flag, bool manual_reset, int process_idL, int signal_number)
{
/**************************************
 *
 *	I S C _ m a k e _ s i g n a l		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Create or open a Windows/NT event.
 *	Use the signal number and process id
 *	in naming the object.
 *
 **************************************/
	ISC_signal_init();

	const BOOL man_rst = manual_reset ? TRUE : FALSE;

	if (!signal_number)
		return CreateEvent(NULL, man_rst, FALSE, NULL);

	TEXT event_name[BUFFER_TINY];
	sprintf(event_name, "_firebird_process%u_signal%d", process_idL, signal_number);

	if (!fb_utils::prefix_kernel_object_name(event_name, sizeof(event_name)))
	{
		SetLastError(ERROR_FILENAME_EXCED_RANGE);
		return NULL;
	}

	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, event_name);

	if (create_flag)
	{
		fb_assert(!hEvent);
		hEvent = CreateEvent(ISC_get_security_desc(), man_rst, FALSE, event_name);
	}

	return hEvent;
}

namespace
{
	class SignalInit
	{
	public:
		static void init()
		{
			process_id = getpid();
			ISC_get_security_desc();
		}
	};

	Firebird::InitMutex<SignalInit> signalInit("SignalInit");
} // anonymous namespace

void ISC_signal_init()
{
/**************************************
 *
 *	I S C _ s i g n a l _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize any system signal handlers.
 *
 **************************************/

	signalInit.init();
}
