/*
 *	PROGRAM:		Firebird services table
 *	MODULE:			svc_tab.h
 *	DESCRIPTION:	Services threads' entrypoints
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
 *  Copyright (c) 2010 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef JRD_SVC_TAB_H
#define JRD_SVC_TAB_H

#include "../common/UtilSvc.h"

namespace Jrd {

typedef int ServiceEntry(Firebird::UtilSvc*);

struct serv_entry
{
	USHORT				serv_action;		// isc_action_svc_....
	const TEXT*			serv_name;			// old service name
	const TEXT*			serv_std_switches;	// old cmd-line switches
	ServiceEntry*		serv_thd;			// thread to execute
};

extern const serv_entry services[];

} // namespace Jrd

#endif // JRD_SVC_TAB_H
