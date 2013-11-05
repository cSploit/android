/*
 *	PROGRAM:	SQL Trace plugin
 *	MODULE:		paramtable.h
 *	DESCRIPTION:	Definitions for configuration file parameters
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *  2008 Khorsun Vladyslav
 */

// The table with parameters used by Trace API
// User of this header is supposed to define:
// PATH_PARAMETER, STR_PARAMETER, BOOL_PARAMETER, UINT_PARAMETER
// DATABASE_PARAMS, SERVICE_PARAMS


STR_PARAMETER(include_filter, "")
STR_PARAMETER(exclude_filter, "")
PATH_PARAMETER(log_filename, "")
BOOL_PARAMETER(log_errors, false)
BOOL_PARAMETER(enabled, false)
UINT_PARAMETER(max_log_size, 0)

#ifdef DATABASE_PARAMS
BOOL_PARAMETER(log_connections, false)
UINT_PARAMETER(connection_id, 0)
BOOL_PARAMETER(log_transactions, false)
BOOL_PARAMETER(log_statement_prepare, false)
BOOL_PARAMETER(log_statement_free, false)
BOOL_PARAMETER(log_statement_start, false)
BOOL_PARAMETER(log_statement_finish, false)
BOOL_PARAMETER(log_procedure_start, false)
BOOL_PARAMETER(log_procedure_finish, false)
BOOL_PARAMETER(log_trigger_start, false)
BOOL_PARAMETER(log_trigger_finish, false)
BOOL_PARAMETER(print_plan, false)
BOOL_PARAMETER(print_perf, false)
BOOL_PARAMETER(log_context, false)
BOOL_PARAMETER(log_blr_requests, false)
BOOL_PARAMETER(print_blr, false)
BOOL_PARAMETER(log_dyn_requests, false)
BOOL_PARAMETER(print_dyn, false)
BOOL_PARAMETER(log_sweep, false)
UINT_PARAMETER(max_sql_length, 300)
UINT_PARAMETER(max_blr_length, 500)
UINT_PARAMETER(max_dyn_length, 500)
UINT_PARAMETER(max_arg_length, 80)
UINT_PARAMETER(max_arg_count, 30)
UINT_PARAMETER(time_threshold, 100)
#endif

#ifdef SERVICE_PARAMS
BOOL_PARAMETER(log_services, false)
BOOL_PARAMETER(log_service_query, false)
#endif
