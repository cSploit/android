/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		isc_ipc.cpp
 *	DESCRIPTION:	Handing and posting of signals (POSIX)
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
 * 2003.08.27 Nickolay Samofatov - create POSIX version of this module
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include "../jrd/gdsassert.h"
#include "../jrd/common.h"
#include "gen/iberror.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/os/isc_i_proto.h"
#include "../jrd/isc_s_proto.h"
#include "../common/classes/locks.h"
#include "../common/classes/init.h"

#ifdef HAVE_VFORK_H
#include <vfork.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif

#ifdef HAVE_SYS_SIGINFO_H
#include <sys/siginfo.h>
#endif

#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef O_RDWR
#include <fcntl.h>
#endif

//#define LOCAL_SEMAPHORES 4

struct sig
{
	struct sig* sig_next;
	int sig_signal;
	union {
		FPTR_VOID_PTR user;
		void (*client1) (int);
		void (*client3) (int, siginfo_t *, void *);
		FPTR_INT_VOID_PTR informs;
		FPTR_VOID untyped;
	} sig_routine;
	void* sig_arg;
	USHORT sig_flags;
	USHORT sig_w_siginfo;
};

typedef sig *SIG;

// flags
const USHORT SIG_user		= 0;		/* Our routine */
const USHORT SIG_client		= 1;		/* Not our routine */
const USHORT SIG_informs	= 2;		/* routine tells us whether to chain */

const SLONG SIG_informs_continue	= 0;	/* continue on signal processing */
const SLONG SIG_informs_stop		= 1;	/* stop signal processing */

static SIG volatile signals = NULL;
static SLONG volatile overflow_count = 0;

static Firebird::GlobalPtr<Firebird::Mutex> sig_mutex;

const char* GDS_RELAY	= "/bin/gds_relay";

static int volatile relay_pipe = 0;


static void signal_cleanup(void* arg);
static bool isc_signal2(int signal, FPTR_VOID handler, void* arg, ULONG);
static SIG que_signal(int signal, FPTR_VOID handler, void* arg, int flags, bool w_siginfo);

#ifdef SA_SIGINFO
static void CLIB_ROUTINE signal_action(int number, siginfo_t *siginfo, void *context);
#else
static void CLIB_ROUTINE signal_action(int number);
#endif

#ifndef SIG_HOLD
#define SIG_HOLD	SIG_DFL
#endif


bool ISC_signal(int signal_number, FPTR_VOID_PTR handler, void* arg)
{
/**************************************
 *
 *	I S C _ s i g n a l
 *
 **************************************
 *
 * Functional description
 *	Multiplex multiple handers into single signal.
 *
 **************************************/
	ISC_signal_init();

	return isc_signal2(signal_number, reinterpret_cast<FPTR_VOID>(handler), arg, SIG_user);
}


static bool isc_signal2(int signal_number, FPTR_VOID handler, void* arg, ULONG flags)
{
/**************************************
 *
 *	i s c _ s i g n a l 2		( u n i x ,   W I N _ N T ,   O S 2 )
 *
 **************************************
 *
 * Functional description
 *	Multiplex multiple handers into single signal.
 *
 **************************************/

	SIG sig;

	Firebird::MutexLockGuard guard(sig_mutex);

/* See if this signal has ever been cared about before */

	for (sig = signals; sig; sig = sig->sig_next)
		if (sig->sig_signal == signal_number)
			break;

/* If it hasn't been attach our chain handler to the signal,
   and queue up whatever used to handle it as a non-ISC
   routine (they are invoked differently).  Note that if
   the old action was SIG_DFL, SIG_HOLD, SIG_IGN or our
   multiplexor, there is no need to save it. */

	bool old_sig_w_siginfo = false;
	bool rc = false;
	if (!sig) {
		struct sigaction act, oact;

#ifdef SA_SIGINFO
		act.sa_sigaction = signal_action;
		act.sa_flags = SA_RESTART | SA_SIGINFO;
#else
		act.sa_handler = signal_action;
#endif
		sigemptyset(&act.sa_mask);
		sigaddset(&act.sa_mask, signal_number);
		sigaction(signal_number, &act, &oact);
#ifdef SA_SIGINFO
		old_sig_w_siginfo = oact.sa_flags & SA_SIGINFO;
#endif

		if (
#ifdef SA_SIGINFO
			oact.sa_sigaction != signal_action &&
#else
			oact.sa_handler != signal_action &&
#endif
			oact.sa_handler != SIG_DFL &&
			oact.sa_handler != SIG_HOLD &&
			oact.sa_handler != SIG_IGN)
		{
			que_signal(signal_number,
#ifdef SA_SIGINFO
			old_sig_w_siginfo ?
				reinterpret_cast<FPTR_VOID>(oact.sa_sigaction) :
#endif
				reinterpret_cast<FPTR_VOID>(oact.sa_handler),
				NULL, SIG_client, old_sig_w_siginfo);
			rc = true;
		}
	}

	/* Que up the new ISC signal handler routine */

	que_signal(signal_number, handler, arg, flags, false);

	return rc;
}


void ISC_signal_cancel(int signal_number, FPTR_VOID_PTR handler, void* arg)
{
/**************************************
 *
 *	I S C _ s i g n a l _ c a n c e l
 *
 **************************************
 *
 * Functional description
 *	Cancel a signal handler.
 *	If handler == NULL, cancel all handlers for a given signal.
 *
 **************************************/
	ISC_signal_init();

	SIG sig;
	volatile SIG* ptr;

	Firebird::MutexLockGuard guard(sig_mutex);

	for (ptr = &signals; sig = *ptr;) {
		if (sig->sig_signal == signal_number &&
			(handler == NULL ||
			 (sig->sig_routine.user == handler && sig->sig_arg == arg)))
		{
			*ptr = sig->sig_next;
			gds__free(sig);
		}
		else
			ptr = &(*ptr)->sig_next;
	}
}


namespace
{
	class SignalInit
	{
	public:
		static void init()
		{
			GDS_init_prefix();

			overflow_count = 0;
			gds__register_cleanup(signal_cleanup, 0);
		}

		static void cleanup()
		{
			signals = NULL;
		}
	};

	Firebird::InitMutex<SignalInit> signalInit;
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


static void signal_cleanup(void*)
{
/**************************************
 *
 *	s i g n a l _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 *	Module level cleanup handler.
 *
 **************************************/
	signalInit.cleanup();
}


static SIG que_signal(int signal_number,
					  FPTR_VOID handler,
					  void* arg,
					  int flags,
					  bool sig_w_siginfo)
{
/**************************************
 *
 *	q u e _ s i g n a l
 *
 **************************************
 *
 * Functional description
 *	Que signal for later action.
 *
 **************************************/
	SIG sig = (SIG) gds__alloc((SLONG) sizeof(struct sig));
/* FREE: unknown */
	if (!sig) {					/* NOMEM: */
		DEV_REPORT("que_signal: out of memory");
		return NULL;			/* NOMEM: not handled, too difficult */
	}
#ifndef SA_SIGINFO
	if (sig_w_siginfo) {
		DEV_REPORT("SA_SIGINFO not supported");
		return NULL;
	}
#endif

#ifdef DEBUG_GDS_ALLOC
/* This will only be freed when a signal handler is de-registered
 * and we don't do that at process exit - so this not always
 * a freed structure.
 */
	gds_alloc_flag_unfreed((void *) sig);
#endif

	sig->sig_signal = signal_number;
	sig->sig_routine.untyped = handler;
	sig->sig_arg = arg;
	sig->sig_flags = flags;
	sig->sig_w_siginfo = sig_w_siginfo;

	sig->sig_next = signals;
	signals = sig;

	return sig;
}


#ifdef SA_SIGINFO
static void CLIB_ROUTINE signal_action(int number, siginfo_t *siginfo, void *context)
#else
static void CLIB_ROUTINE signal_action(int number)
#endif
{
/**************************************
 *
 *	s i g n a l _ a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Checkin with various signal handlers.
 *
 **************************************/

	// Invoke everybody who may have expressed an interest.
	for (SIG sig = signals; sig; sig = sig->sig_next)
	{
		if (sig->sig_signal == number)
		{
			if (sig->sig_flags & SIG_client)
			{
#ifdef SA_SIGINFO
				if (sig->sig_w_siginfo)
				{
					(*sig->sig_routine.client3)(number, siginfo, context);
				}
				else
#endif
				{
					(*sig->sig_routine.client1)(number);
				}
			}
			else if (sig->sig_flags & SIG_informs)
			{
				/* Routine will tell us whether to chain the signal to other handlers */
				if ((*sig->sig_routine.informs)(sig->sig_arg) == SIG_informs_stop)
				{
					break;
				}
			}
			else
			{
				(*sig->sig_routine.user) (sig->sig_arg);
			}
		}
	}
}

