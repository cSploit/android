/*
 *	MODULE:		TraceService.h
 *	DESCRIPTION:
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
 *  The Original Code was created by Khorsun Vladyslav
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Khorsun Vladyslav <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#ifndef JRD_TRACESERVICE_H
#define JRD_TRACESERVICE_H

#include "firebird.h"
#include "consts_pub.h"
#include "fb_exception.h"
#include "iberror.h"
#include "../../common/classes/fb_string.h"
#include "../../common/StatusArg.h"
#include "../../common/UtilSvc.h"
#include "../../jrd/constants.h"
#include "../../common/ThreadData.h"
#include "../../jrd/trace/TraceSession.h"


int TRACE_main(Firebird::UtilSvc*);


namespace Firebird {

class TraceSvcIntf
{
public:
	virtual void setAttachInfo(const string& service_name, const string& user,
		const string& pwd, bool isAdmin) = 0;

	virtual void startSession(TraceSession& session, bool interactive) = 0;
	virtual void stopSession(ULONG id) = 0;
	virtual void setActive(ULONG id, bool active) = 0;
	virtual void listSessions() = 0;

	virtual ~TraceSvcIntf() { }
};

void fbtrace(UtilSvc* uSvc, TraceSvcIntf* traceSvc);

} // namespace Firebird


#endif // JRD_TRACESERVICE_H
