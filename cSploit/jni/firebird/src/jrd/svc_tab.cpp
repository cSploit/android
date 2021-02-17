/*
 *	PROGRAM:		Firebird services table
 *	MODULE:			svc_tab.cpp
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

#include "firebird.h"
#include "../jrd/svc_tab.h"
#include "gen/iberror.h"
#include "../jrd/svc.h"
#include "../jrd/trace/TraceService.h"

// Service Functions
#include "../burp/burp_proto.h"
#include "../alice/alice_proto.h"
int main_gstat(Firebird::UtilSvc* uSvc);
#include "../utilities/nbackup/nbk_proto.h"
#include "../utilities/gsec/gsec_proto.h"

namespace Jrd {

#ifdef DEBUG
int test_thread(Firebird::UtilSvc* uSvc);
void test_cmd(USHORT, SCHAR *, TEXT **);
#define TEST_THREAD test_thread
#define TEST_CMD test_cmd

// The following two functions are temporary stubs and will be
// removed as the services API takes shape.  They are used to
// test that the paths for starting services and parsing command-lines
// are followed correctly.
int test_thread(Firebird::UtilSvc* uSvc)
{
	gds__log("Starting service");
	return FINI_OK;
}

void test_cmd(USHORT /*spb_length*/, SCHAR* /*spb*/, TEXT** /*switches*/)
{
	gds__log("test_cmd called");
}

#else
#define TEST_THREAD NULL
#define TEST_CMD NULL
#endif

const serv_entry services[] =
{

	{ isc_action_max, "print_cache", "", NULL },
	{ isc_action_max, "print_locks", "", NULL },
	{ isc_action_max, "start_cache", "", NULL },
	{ isc_action_max, "analyze_database", "", ALICE_main },
	{ isc_action_max, "backup", "-b", BURP_main },
	{ isc_action_max, "create", "-c", BURP_main },
	{ isc_action_max, "restore", "-r", BURP_main },
	{ isc_action_max, "gdef", "-svc", NULL },
#ifndef EMBEDDED
	// this restriction for embedded is temporarty and will gone when new build system will be introduced
	{ isc_action_max, "gsec", "-svc", GSEC_main },
#endif
	{ isc_action_max, "disable_journal", "-disable", NULL },
	{ isc_action_max, "dump_journal", "-online_dump", NULL },
	{ isc_action_max, "enable_journal", "-enable", NULL },
	{ isc_action_max, "monitor_journal", "-console", NULL },
	{ isc_action_max, "query_server", NULL, NULL },
	{ isc_action_max, "start_journal", "-server", NULL },
	{ isc_action_max, "stop_cache", "-shut -cache", ALICE_main },
	{ isc_action_max, "stop_journal", "-console", NULL },
	{ isc_action_max, "anonymous", NULL, NULL },

	// NEW VERSION 2 calls, the name field MUST be different from those names above
	{ isc_action_max, "service_mgr", NULL, NULL },
	{ isc_action_svc_backup, "Backup Database", NULL, BURP_main },
	{ isc_action_svc_restore, "Restore Database", NULL, BURP_main },
	{ isc_action_svc_repair, "Repair Database", NULL, ALICE_main },
#ifndef EMBEDDED
	{ isc_action_svc_add_user, "Add User", NULL, GSEC_main },
	{ isc_action_svc_delete_user, "Delete User", NULL, GSEC_main },
	{ isc_action_svc_modify_user, "Modify User", NULL, GSEC_main },
	{ isc_action_svc_display_user, "Display User", NULL, GSEC_main },
#endif
	{ isc_action_svc_properties, "Database Properties", NULL, ALICE_main },
	{ isc_action_svc_lock_stats, "Lock Stats", NULL, TEST_THREAD },
	{ isc_action_svc_db_stats, "Database Stats", NULL, main_gstat },
	{ isc_action_svc_get_fb_log, "Get Log File", NULL, Service::readFbLog },
	{ isc_action_svc_nbak, "Incremental Backup Database", NULL, NBACKUP_main },
	{ isc_action_svc_nrest, "Incremental Restore Database", NULL, NBACKUP_main },
	{ isc_action_svc_trace_start, "Start Trace Session", NULL, TRACE_main },
	{ isc_action_svc_trace_stop, "Stop Trace Session", NULL, TRACE_main },
	{ isc_action_svc_trace_suspend, "Suspend Trace Session", NULL, TRACE_main },
	{ isc_action_svc_trace_resume, "Resume Trace Session", NULL, TRACE_main },
	{ isc_action_svc_trace_list, "List Trace Sessions", NULL, TRACE_main },
#ifndef EMBEDDED
	{ isc_action_svc_set_mapping, "Set Domain Admins Mapping to RDB$ADMIN", NULL, GSEC_main },
	{ isc_action_svc_drop_mapping, "Drop Domain Admins Mapping to RDB$ADMIN", NULL, GSEC_main },
	{ isc_action_svc_display_user_adm, "Display User with Admin Info", NULL, GSEC_main },
#endif
	// actions with no names are undocumented
	{ isc_action_svc_set_config, NULL, NULL, TEST_THREAD },
	{ isc_action_svc_default_config, NULL, NULL, TEST_THREAD },
	{ isc_action_svc_set_env, NULL, NULL, TEST_THREAD },
	{ isc_action_svc_set_env_lock, NULL, NULL, TEST_THREAD },
	{ isc_action_svc_set_env_msg, NULL, NULL, TEST_THREAD },
	{ 0, NULL, NULL, NULL }
};

} //namespace Jrd
