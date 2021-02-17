/*
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
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 */
#ifndef	ALICE_ALICESWI_H
#define	ALICE_ALICESWI_H

#include "../jrd/ibase.h"
#include "../jrd/constants.h"

// switch definitions

const SINT64 sw_list			= 0x0000000000000001L;	// Byte 0, Bit 0
const SINT64 sw_prompt			= 0x0000000000000002L;
const SINT64 sw_commit			= 0x0000000000000004L;
const SINT64 sw_rollback		= 0x0000000000000008L;
const SINT64 sw_sweep			= 0x0000000000000010L;
const SINT64 sw_validate		= 0x0000000000000020L;
const SINT64 sw_no_update		= 0x0000000000000040L;
const SINT64 sw_full			= 0x0000000000000080L;
const SINT64 sw_mend			= 0x0000000000000100L;	// Byte 1, Bit 0
const SINT64 sw_all				= 0x0000000000000200L;
const SINT64 sw_enable			= 0x0000000000000400L;
const SINT64 sw_disable			= 0x0000000000000800L;
const SINT64 sw_ignore			= 0x0000000000001000L;
const SINT64 sw_activate		= 0x0000000000002000L;
const SINT64 sw_two_phase		= 0x0000000000004000L;
const SINT64 sw_housekeeping	= 0x0000000000008000L;
const SINT64 sw_kill			= 0x0000000000010000L;	// Byte 2, Bit 0
//const SINT64 sw_begin_log		= 0x0000000000020000L;
//const SINT64 sw_quit_log		= 0x0000000000040000L;
const SINT64 sw_write			= 0x0000000000080000L;
const SINT64 sw_no_reserve		= 0x0000000000100000L;
const SINT64 sw_user			= 0x0000000000200000L;
const SINT64 sw_password		= 0x0000000000400000L;
const SINT64 sw_shut			= 0x0000000000800000L;
const SINT64 sw_online			= 0x0000000001000000L;	// Byte 3, Bit 0
const SINT64 sw_cache			= 0x0000000002000000L;
const SINT64 sw_attach			= 0x0000000004000000L;
const SINT64 sw_force			= 0x0000000008000000L;
const SINT64 sw_tran			= 0x0000000010000000L;
const SINT64 sw_buffers			= 0x0000000020000000L;
const SINT64 sw_mode			= 0x0000000040000000L;
const SINT64 sw_set_db_dialect	= 0x0000000080000000L;
const SINT64 sw_trusted_auth	= QUADCONST(0x0000000100000000);	// Byte 4, Bit 0
//const SINT64 sw_trusted_svc	= QUADCONST(0x0000000200000000);
//const SINT64 sw_trusted_role	= QUADCONST(0x0000000400000000);
const SINT64 sw_fetch_password	= QUADCONST(0x0000000800000000);
const SINT64 sw_nolinger		= QUADCONST(0x0000001000000000);


enum alice_switches
{
	IN_SW_ALICE_0					=	0,	// not a known switch
	IN_SW_ALICE_LIST				=	1,
	IN_SW_ALICE_PROMPT				=	2,
	IN_SW_ALICE_COMMIT				=	3,
	IN_SW_ALICE_ROLLBACK			=	4,
	IN_SW_ALICE_SWEEP				=	5,
	IN_SW_ALICE_VALIDATE			=	6,
	IN_SW_ALICE_NO_UPDATE			=	7,
	IN_SW_ALICE_FULL				=	8,
	IN_SW_ALICE_MEND				=	9,
	IN_SW_ALICE_ALL					=	10,
	IN_SW_ALICE_ENABLE				=	11,
	//IN_SW_ALICE_DISABLE				=	12,
	IN_SW_ALICE_IGNORE				=	13,
	IN_SW_ALICE_ACTIVATE			=	14,
	IN_SW_ALICE_TWO_PHASE			=	15,
	IN_SW_ALICE_HOUSEKEEPING		=	16,
	IN_SW_ALICE_KILL				=	17,
	//IN_SW_ALICE_BEGIN_LOG			=	18,
	//IN_SW_ALICE_QUIT_LOG			=	19,
	IN_SW_ALICE_WRITE				=	20,
	IN_SW_ALICE_NO_RESERVE			=	21,
	IN_SW_ALICE_USER				=	22,
	IN_SW_ALICE_PASSWORD			=	23,
	IN_SW_ALICE_SHUT				=	24,
	IN_SW_ALICE_ONLINE				=	25,
	IN_SW_ALICE_CACHE				=	26,
	IN_SW_ALICE_ATTACH				=	27,
	IN_SW_ALICE_FORCE				=	28,
	IN_SW_ALICE_TRAN				=	29,
	IN_SW_ALICE_BUFFERS				=	30,
	IN_SW_ALICE_VERSION				=	31,
	IN_SW_ALICE_X					=	32,	// set debug mode on
	IN_SW_ALICE_HIDDEN_ASYNC		=	33,
	IN_SW_ALICE_HIDDEN_SYNC			=	34,
	IN_SW_ALICE_HIDDEN_USEALL		=	35,
	IN_SW_ALICE_HIDDEN_RESERVE		=	36,
	IN_SW_ALICE_HIDDEN_RDONLY		=	37,
	IN_SW_ALICE_HIDDEN_RDWRITE		=	38,
	IN_SW_ALICE_MODE				=	39,
	IN_SW_ALICE_HIDDEN_FORCE		=	40,
	IN_SW_ALICE_HIDDEN_TRAN			=	41,
	IN_SW_ALICE_HIDDEN_ATTACH		=	42,
	IN_SW_ALICE_SET_DB_SQL_DIALECT	=	43,
#ifdef TRUSTED_AUTH
	IN_SW_ALICE_TRUSTED_AUTH		=	44,
#endif
//	IN_SW_ALICE_TRUSTED_USER		=	45,
//	IN_SW_ALICE_TRUSTED_ROLE		=	46,
	IN_SW_ALICE_HIDDEN_ONLINE		=	47,
	IN_SW_ALICE_FETCH_PASSWORD		=	48,
	IN_SW_ALICE_NOLINGER			=	49
};

static const char* const ALICE_SW_ASYNC	= "ASYNC";
static const char* const ALICE_SW_SYNC	= "SYNC";
static const char* const ALICE_SW_MODE_RO	= "READ_ONLY";
static const char* const ALICE_SW_MODE_RW	= "READ_WRITE";

// Switch table
static const Switches::in_sw_tab_t alice_in_sw_table[] =
{
	{IN_SW_ALICE_ACTIVATE, isc_spb_prp_activate, "ACTIVATE_SHADOW", sw_activate,
		0, ~(sw_activate | sw_user | sw_password | sw_nolinger), false, 25, 2, NULL},
	// msg 25: \t-activate shadow file for database usage
	{IN_SW_ALICE_ATTACH, isc_spb_prp_attachments_shutdown, "ATTACH", sw_attach,
		sw_shut, 0, false, 26, 2, NULL},
	// msg 26: \t-attach\tshutdown new database attachments
#ifdef DEV_BUILD
/*
	{IN_SW_ALICE_BEGIN_LOG, 0, "BEGIN_LOG", sw_begin_log,
		0, ~(sw_begin_log | sw_user | sw_password), false, 27, 3, NULL},
	// msg 27: \t-begin_log\tbegin logging for replay utility
*/
#endif
	{IN_SW_ALICE_BUFFERS, isc_spb_prp_page_buffers, "BUFFERS", sw_buffers,
		0, 0, false, 28, 1, NULL},
	// msg 28: \t-buffers\tset page buffers <n>
	{IN_SW_ALICE_COMMIT, isc_spb_rpr_commit_trans, "COMMIT", sw_commit,
		0, ~(sw_commit | sw_user | sw_password | sw_nolinger), false, 29, 2, NULL},
	// msg 29: \t-commit\t\tcommit transaction <tr / all>
	{IN_SW_ALICE_CACHE, 0, "CACHE", sw_cache,
		sw_shut, 0, false, 30, 2, NULL},
	// msg 30: \t-cache\t\tshutdown cache manager
#ifdef DEV_BUILD
/*
	{IN_SW_ALICE_DISABLE, 0, "DISABLE", sw_disable,
		0, 0, false, 31, 3, NULL},
	// msg 31: \t-disable\tdisable WAL
	*/
#endif
	{IN_SW_ALICE_FULL, isc_spb_rpr_full, "FULL", sw_full,
		sw_validate, 0, false, 32, 2, NULL},
	// msg 32: \t-full\t\tvalidate record fragments (-v)
	{IN_SW_ALICE_FORCE, isc_spb_prp_force_shutdown, "FORCE_SHUTDOWN", sw_force,
		sw_shut, 0, false, 33, 2, NULL},
	// msg 33: \t-force\t\tforce database shutdown
	{IN_SW_ALICE_FETCH_PASSWORD, 0, "FETCH_PASSWORD", sw_fetch_password,
		0, sw_password, false, 119, 2, NULL},
	// msg 119: -fetch_password fetch_password from file
	{IN_SW_ALICE_HOUSEKEEPING, isc_spb_prp_sweep_interval, "HOUSEKEEPING", sw_housekeeping,
		0, 0, false, 34, 1, NULL},
	// msg 34: \t-housekeeping\tset sweep interval <n>
	{IN_SW_ALICE_IGNORE, isc_spb_rpr_ignore_checksum, "IGNORE", sw_ignore,
		0, 0, false, 35, 1, NULL},
	// msg 35: \t-ignore\t\tignore checksum errors
	{IN_SW_ALICE_KILL, isc_spb_rpr_kill_shadows, "KILL_SHADOW", sw_kill,
		0, 0, false, 36, 1, NULL},
	// msg 36: \t-kill\t\tkill all unavailable shadow files
	{IN_SW_ALICE_LIST, isc_spb_rpr_list_limbo_trans, "LIST", sw_list,
		0, ~(sw_list | sw_user | sw_password | sw_nolinger), false, 37, 1, NULL},
	// msg 37: \t-list\t\tshow limbo transactions
	{IN_SW_ALICE_MEND, isc_spb_rpr_mend_db, "MEND", sw_mend | sw_validate | sw_full,
		0, ~(sw_no_update | sw_user | sw_password | sw_nolinger), false, 38, 2, NULL},
	// msg 38: \t-mend\t\tprepare corrupt database for backup
	{IN_SW_ALICE_MODE, 0, "MODE", sw_mode,
		0, ~(sw_mode | sw_user | sw_password | sw_nolinger), false, 109, 2, NULL},
	// msg 109: \t-mode\t\tread_only or read_write
	{IN_SW_ALICE_NOLINGER, isc_spb_prp_nolinger, "NOLINGER", sw_nolinger,
		0, sw_shut, false, 121, 3, NULL},
	// msg 121:	-nolinger	do not use linger on database this time (once)
	{IN_SW_ALICE_NO_UPDATE, isc_spb_rpr_check_db, "NO_UPDATE", sw_no_update,
		sw_validate, 0, false, 39, 1, NULL},
	// msg 39: \t-no_update\tread-only validation (-v)
	{IN_SW_ALICE_ONLINE, isc_spb_prp_db_online, "ONLINE", sw_online,
		0, 0, false, 40, 1	, NULL},
	// msg 40: \t-online\t\tdatabase online
	{IN_SW_ALICE_PROMPT, 0, "PROMPT", sw_prompt,
		sw_list, 0, false, 41, 2, NULL},
	// msg 41: \t-prompt\t\tprompt for commit/rollback (-l)
	{IN_SW_ALICE_PASSWORD, 0, "PASSWORD", sw_password,
		0, (sw_trusted_auth /*| sw_trusted_svc | sw_trusted_role */| sw_fetch_password),
		false, 42, 2, NULL},
	// msg 42: \t-password\tdefault password
#ifdef DEV_BUILD
/*
	{IN_SW_ALICE_QUIT_LOG, 0, "QUIT_LOG", sw_quit_log,
		0, ~(sw_quit_log | sw_user | sw_password), false, 43, 3, NULL},
	// msg 43: \t-quit_log\tquit logging for replay utility
*/
#endif
	{IN_SW_ALICE_ROLLBACK, isc_spb_rpr_rollback_trans, "ROLLBACK", sw_rollback,
		0, ~(sw_rollback | sw_user | sw_password | sw_nolinger), false, 44, 1, NULL},
	// msg 44: \t-rollback\trollback transaction <tr / all>
	{IN_SW_ALICE_SET_DB_SQL_DIALECT, isc_spb_prp_set_sql_dialect, "SQL_DIALECT", sw_set_db_dialect,
		0, 0, false, 111, 2, NULL},
	// msg 111: \t-SQL_dialect\t\set dataabse dialect n
	{IN_SW_ALICE_SWEEP, isc_spb_rpr_sweep_db, "SWEEP", sw_sweep,
		0, ~(sw_sweep | sw_user | sw_password | sw_nolinger), false, 45, 2, NULL},
	// msg 45: \t-sweep\t\tforce garbage collection
	{IN_SW_ALICE_SHUT, isc_spb_prp_shutdown_mode, "SHUTDOWN", sw_shut,
		0, ~(sw_shut | sw_attach | sw_cache | sw_force | sw_tran | sw_user | sw_password),
		false, 46, 2, NULL},
	// msg 46: \t-shut\t\tshutdown
	{IN_SW_ALICE_TWO_PHASE, isc_spb_rpr_recover_two_phase, "TWO_PHASE", sw_two_phase,
		0, ~(sw_two_phase | sw_user | sw_password | sw_nolinger), false, 47, 2, NULL},
	// msg 47: \t-two_phase\tperform automated two-phase recovery
	{IN_SW_ALICE_TRAN, isc_spb_prp_transactions_shutdown, "TRANSACTION", sw_tran,
		sw_shut, 0, false, 48, 3, NULL},
	// msg 48: \t-tran\t\tshutdown transaction startup
#ifdef TRUSTED_AUTH
	{IN_SW_ALICE_TRUSTED_AUTH, 0, "TRUSTED", sw_trusted_auth,
		0, (sw_user | sw_password), false, 115, 3, NULL},
	// msg 115: 	-trusted	use trusted authentication
#endif
/*	{IN_SW_ALICE_TRUSTED_USER, 0, TRUSTED_USER_SWITCH, sw_trusted_svc,
		0, (sw_trusted_svc | sw_user | sw_password), false, 0, TRUSTED_USER_SWITCH_LEN, NULL},
	{IN_SW_ALICE_TRUSTED_ROLE, 0, TRUSTED_ROLE_SWITCH, sw_trusted_role,
		sw_trusted_svc, (sw_user | sw_password), false, 0, TRUSTED_ROLE_SWITCH_LEN, NULL}, */
	{IN_SW_ALICE_NO_RESERVE, 0, "USE", sw_no_reserve,
		0, ~(sw_no_reserve | sw_user | sw_password | sw_nolinger), false, 49, 1, NULL},
	// msg 49: \t-use\t\tuse full or reserve space for versions
	{IN_SW_ALICE_USER, 0, "USER", sw_user,
		0, (sw_trusted_auth /*| sw_trusted_svc | sw_trusted_role*/), false, 50, 4, NULL},
	// msg 50: \t-user\t\tdefault user name
	{IN_SW_ALICE_VALIDATE, isc_spb_rpr_validate_db, "VALIDATE", sw_validate,
		0, ~(sw_validate | sw_user | sw_password | sw_nolinger), false, 51, 1, NULL},
	// msg 51: \t-validate\tvalidate database structure
	{IN_SW_ALICE_WRITE, 0, "WRITE", sw_write,
		0, ~(sw_write | sw_user | sw_password | sw_nolinger), false, 52, 1, NULL},
	// msg 52: \t-write\t\twrite synchronously or asynchronously
#ifdef DEV_BUILD
	{IN_SW_ALICE_X, 0, "X", 0,
		0, 0, false, 53, 1, NULL},
	// msg 53: \t-x\t\tset debug on
#endif
	{IN_SW_ALICE_VERSION, 0, "Z", 0,
		0, 0, false, 54, 1, NULL},
	// msg 54: \t-z\t\tprint software version number
/************************************************************************/
// WARNING: All new switches should be added right before this comments
/************************************************************************/
/* The next nine 'virtual' switches are hidden from user and are needed
   for services API
 ************************************************************************/
	{IN_SW_ALICE_HIDDEN_ASYNC, isc_spb_prp_wm_async, "WRITE ASYNC",
		0, 0, 0, false, 0, 0, NULL},
	{IN_SW_ALICE_HIDDEN_SYNC, isc_spb_prp_wm_sync, "WRITE SYNC",
		0, 0, 0, false, 0, 0, NULL},
	{IN_SW_ALICE_HIDDEN_USEALL, isc_spb_prp_res_use_full, "USE FULL",
		0, 0, 0, false, 0, 0, NULL},
	{IN_SW_ALICE_HIDDEN_RESERVE, isc_spb_prp_res, "USE RESERVE",
		0, 0, 0, false, 0, 0, NULL},
	{IN_SW_ALICE_HIDDEN_FORCE, isc_spb_prp_shutdown_db, "SHUT -FORCE",
		0, 0, 0, false, 0, 0, NULL},
	{IN_SW_ALICE_HIDDEN_TRAN, isc_spb_prp_deny_new_transactions, "SHUT -TRAN",
		0, 0, 0, false, 0, 0, NULL},
	{IN_SW_ALICE_HIDDEN_ATTACH, isc_spb_prp_deny_new_attachments, "SHUT -ATTACH",
		0, 0, 0, false, 0, 0, NULL},
	{IN_SW_ALICE_HIDDEN_ONLINE, isc_spb_prp_online_mode, "ONLINE",
		0, 0, 0, false, 0, 0, NULL},
	{IN_SW_ALICE_HIDDEN_RDONLY, isc_spb_prp_am_readonly, "MODE READ_ONLY",
		0, 0, 0, false, 0, 0, NULL},
	{IN_SW_ALICE_HIDDEN_RDWRITE, isc_spb_prp_am_readwrite, "MODE READ_WRITE",
		0, 0, 0, false, 0, 0, NULL},
/************************************************************************/
	{IN_SW_ALICE_0, 0, NULL, 0,
     	0, 0, false, 0, 0, NULL}
};

static const char* alice_mode_sw_table[] =
{
	"NORMAL", "MULTI", "SINGLE", "FULL"
};

#endif // ALICE_ALICESWI_H
