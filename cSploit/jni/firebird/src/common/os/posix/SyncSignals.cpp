/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		SyncSignals.cpp
 *	DESCRIPTION:	Control unix synchronous signals.
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
 *					Alex Peshkov
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gen/iberror.h"

/*
#include "../yvalve/gds_proto.h"
#include "../common/isc_proto.h"
#include "../common/os/isc_i_proto.h"
#include "../common/os/os_utils.h"
#include "../common/isc_s_proto.h"
#include "../common/file_params.h"
#include "../common/gdsassert.h"
#include "../common/config/config.h"
#include "../common/utils_proto.h"
#include "../common/StatusArg.h"
#include "../common/ThreadData.h"
*/

#include <setjmp.h>
#include "fb_pthread.h"
#include <signal.h>
#include "../common/classes/fb_tls.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <unistd.h>

#if defined FREEBSD || defined NETBSD || defined DARWIN || defined HPUX
#define sigset      signal
#endif

namespace {

	// Here we can't use atomic counter instead mutex/counter pair - or some thread may leave SyncSignalsSet()
	// before signals are actually set in the other thread, which incremented counter first
	Firebird::GlobalPtr<Firebird::Mutex> syncEnterMutex;
	int syncEnterCounter = 0;

	TLS_DECLARE(sigjmp_buf*, sigjmpPtr);

	extern "C" {

		void longjmpSigHandler(int sigNum)
		{
			siglongjmp(*TLS_GET(sigjmpPtr), sigNum);
		}

	} // extern "C"

#ifndef HAVE_SIGSET
	typedef void HandlerType(int);
	void sigset(int signum, HandlerType* handler)
	{
		struct sigaction act;
		memset(&act, 0, sizeof act);
		act.sa_handler = handler;
		sigaction(signum, &act, NULL);
	}
#endif

} // anonymous namespace

namespace Firebird {

void syncSignalsSet(void* arg)
{
/**************************************
 *
 *	s y n c S i g n a l s S e t
 *
 **************************************
 *
 * Functional description
 *	Set all the synchronous signals for a particular thread
 *
 **************************************/
	sigjmp_buf* const sigenv = static_cast<sigjmp_buf*>(arg);
	TLS_SET(sigjmpPtr, sigenv);

	Firebird::MutexLockGuard g(syncEnterMutex, "syncSignalsSet");

	if (syncEnterCounter++ == 0)
	{
		sigset(SIGILL, longjmpSigHandler);
		sigset(SIGFPE, longjmpSigHandler);
		sigset(SIGBUS, longjmpSigHandler);
		sigset(SIGSEGV, longjmpSigHandler);
	}
}


void syncSignalsReset()
{
/**************************************
 *
 *	s y n c S i g n a l s R e s e t
 *
 **************************************
 *
 * Functional description
 *	Reset all the synchronous signals for a particular thread
 * to default.
 *
 **************************************/

	Firebird::MutexLockGuard g(syncEnterMutex, "syncSignalsReset");

	fb_assert(syncEnterCounter > 0);

	if (--syncEnterCounter == 0)
	{
		sigset(SIGILL, SIG_DFL);
		sigset(SIGFPE, SIG_DFL);
		sigset(SIGBUS, SIG_DFL);
		sigset(SIGSEGV, SIG_DFL);
	}
}

} // namespace Firebird
