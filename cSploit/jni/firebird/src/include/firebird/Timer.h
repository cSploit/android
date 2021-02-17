/*
 *	PROGRAM:		Firebird interface.
 *	MODULE:			firebird/Timer.h
 *	DESCRIPTION:	Timer interface definition.
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2011 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FIREBIRD_TIMER_H
#define FIREBIRD_TIMER_H

#include "./Interface.h"

namespace Firebird {

// Identifies particular timer.
// Callback function is invoked when timer fires.
class ITimer : public IRefCounted
{
public:
	virtual void FB_CARG handler() = 0;
};
#define FB_TIMER_VERSION (FB_REFCOUNTED_VERSION + 1)

typedef ISC_INT64 TimerDelay;

// Interface to set timer for particular time
class ITimerControl : public IVersioned
{
public:
	// Set timer
	virtual void FB_CARG start(ITimer* timer, TimerDelay microSeconds) = 0;
	// Stop timer
	virtual void FB_CARG stop(ITimer* timer) = 0;
};
#define FB_TIMER_CONTROL_VERSION (FB_VERSIONED_VERSION + 2)

}	// namespace Firebird


#endif	// FIREBIRD_TIMER_H
