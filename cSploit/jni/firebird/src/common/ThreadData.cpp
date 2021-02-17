/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ThreadData.cpp
 *	DESCRIPTION:	Thread support routines
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
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * Alex Peshkov
 */

#include "firebird.h"
#include <stdio.h>
#include <errno.h>
#include "../common/ThreadData.h"
#include "../common/gdsassert.h"
#include "../common/classes/fb_tls.h"


#ifdef WIN_NT
#include <process.h>
#include <windows.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#if defined SOLARIS || defined AIX
#include <sched.h>
#endif

#ifdef SOLARIS
#include <thread.h>
#include <signal.h>
#endif

#include "../common/classes/locks.h"
#include "../common/classes/rwlock.h"

using namespace Firebird;

namespace {

TLS_DECLARE (ThreadData*, tData);

}


ThreadData* ThreadData::getSpecific()
{
	return TLS_GET(tData);
}


void ThreadData::putSpecific()
{
	threadDataPriorContext = TLS_GET(tData);
	TLS_SET(tData, this);
}


void ThreadData::restoreSpecific()
{
	ThreadData* current_context = getSpecific();
	TLS_SET(tData, current_context->threadDataPriorContext);
}
