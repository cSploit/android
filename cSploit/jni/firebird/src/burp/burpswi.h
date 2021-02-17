/*
 *	PROGRAM:	JRD Backup and Restore Program
 *	MODULE: 	burpswi.h
 *	DESCRIPTION:	Burp switches
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
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *						   conditionals, as the engine now fully supports
 *						   readonly databases.
 */

#ifndef BURP_BURPSWI_H
#define BURP_BURPSWI_H

#include "../jrd/constants.h"
#include "../jrd/ibase.h"

/* Local copies of global variables.  They will be copied into
   a data structure. */

// Local variables

const int IN_SW_BURP_0			= 0;	// the unknowable switch
const int IN_SW_BURP_B			= 1;	// backup
const int IN_SW_BURP_C			= 2;	// create_database
//const int IN_SW_BURP_F		= 3;	// file names and starting page
const int IN_SW_BURP_M			= 4;	// backup only metadata
const int IN_SW_BURP_N			= 5;	// do not restore validity conditions
const int IN_SW_BURP_P			= 6;	// specify output page size
const int IN_SW_BURP_R			= 7;	// replace existing database
//const int IN_SW_BURP_U		= 9;	// don't back up security information
const int IN_SW_BURP_V			= 10;	// verify actions; verbose with fixed interval (10000)
const int IN_SW_BURP_Z			= 11;	// print version number
const int IN_SW_BURP_SKIP_DATA	= 12;	// skip data from table
const int IN_SW_BURP_E			= 13;	// expand (no compress)
const int IN_SW_BURP_Y			= 14;	// redirect/suppress status and error output
const int IN_SW_BURP_L			= 15;	// ignore limbo transactions
const int IN_SW_BURP_T			= 16;	// build a 'transportable' backup (V4 default)
const int IN_SW_BURP_O			= 17;	// commit after each relation
const int IN_SW_BURP_I			= 18;	// deactivate indexes
const int IN_SW_BURP_K			= 19;	// kill any shadows defined on database
const int IN_SW_BURP_G			= 20;	// inhibit garbage collection
const int IN_SW_BURP_IG 		= 21;	// database is largely trash try anyway
const int IN_SW_BURP_FA 		= 22;	// blocking factor
const int IN_SW_BURP_US 		= 23;	// use all space on data page
const int IN_SW_BURP_OL 		= 24;	// write RDB$DESCRIPTIONS & SOURCE in old manner
//const int IN_SW_BURP_7		= 25;	// force creation of an ODS 7 database
const int IN_SW_BURP_USER		= 26;	// default user name to use on attach
const int IN_SW_BURP_PASS		= 27;	// default password to use on attach
const int IN_SW_BURP_S			= 28;	// skip some number of bytes if find a bad attribute
const int IN_SW_BURP_NT 		= 29;	// build a "non-transportable" backup (V3 default)
//const int IN_SW_BURP_BUG8183	= 30;	// use workaround to allow restore database
										// v3.3 with comment field inside of index definition
const int IN_SW_BURP_ROLE		= 31;	// default SQL role to use on attach
const int IN_SW_BURP_CO 		= 32;	// convert external tables to internal tables during backup
const int IN_SW_BURP_BU 		= 33;	// specify page buffers for cache
const int IN_SW_BURP_SE 		= 34;	// use services manager
const int IN_SW_BURP_MODE		= 35;	// database could be restored ReadOnly
/**************************************************************************/
// The next two 'virtual' switches are hidden from user and are needed
// for services API
/**************************************************************************/
const int IN_SW_BURP_HIDDEN_RDONLY		= 36;
const int IN_SW_BURP_HIDDEN_RDWRITE		= 37;

const int IN_SW_BURP_RECREATE			= 38;	// recreate database
const int IN_SW_BURP_NOD				= 39;	// do not run database triggers
#ifdef TRUSTED_AUTH
const int IN_SW_BURP_TRUSTED_AUTH		= 40;	// force trusted auth
#endif
//const int IN_SW_BURP_TRUSTED_USER		= 41;	// trusted user name to use on attach
//const int IN_SW_BURP_TRUSTED_ROLE		= 42;	// use trusted role on attach

const int IN_SW_BURP_FIX_FSS_DATA		= 43;	// fix unicode_fss data
const int IN_SW_BURP_FIX_FSS_METADATA	= 44;	// fix unicode_fss metadata

const int IN_SW_BURP_FETCHPASS			= 45;	// fetch default password from file to use on attach
const int IN_SW_BURP_VERBINT			= 46;	// verbose but with specific interval

/**************************************************************************/
	// used 0BCDEFGILMNOPRSTUVYZ	available AHJQWX

static const char* const BURP_SW_MODE_RO = "READ_ONLY";
static const char* const BURP_SW_MODE_RW = "READ_WRITE";
static const char* const BURP_SW_OVERWRITE = "OVERWRITE"; // recreate with overwrite


enum BurpOptionType { boGeneral, boMain, boBackup, boRestore };

static const Switches::in_sw_tab_t reference_burp_in_sw_table[] =
{
	{IN_SW_BURP_B,	  0,						"BACKUP_DATABASE",	0, 0, 0, false, 60, 	1, NULL, boMain},
				// msg 60: @1BACKUP_DATABASE backup database to file
	{IN_SW_BURP_BU,   isc_spb_res_buffers,		"BUFFERS",			0, 0, 0, false, 257,	2, NULL, boRestore},
				// msg 257: @1BU(FFERS) override default page buffers
	{IN_SW_BURP_C,	  isc_spb_res_create,		"CREATE_DATABASE",	0, 0, 0, false, 73, 	1, NULL, boMain},
				// msg 73: @1CREATE_DATABASE create database from backup file
	{IN_SW_BURP_CO,   isc_spb_bkp_convert,		"CONVERT",			0, 0, 0, false, 254,	2, NULL, boBackup},
				// msg 254: @1CO(NVERT)  backup external files as tables
	{IN_SW_BURP_E,	  isc_spb_bkp_expand,		"EXPAND",			0, 0, 0, false, 97, 	1, NULL, boBackup},
				// msg 97: @1EXPAND no data compression
	//{IN_SW_BURP_F,	0,						"FILE_NAMES",		0, 0, 0, false, 0,		0, NULL},
	{IN_SW_BURP_FA,   isc_spb_bkp_factor,		"FACTOR",			0, 0, 0, false, 181,	2, NULL, boBackup},
				/* msg 181; @1FACTOR  blocking factor */
	{IN_SW_BURP_FETCHPASS, 0,					"FETCH_PASSWORD",	0, 0, 0, false, 306,	2, NULL, boGeneral},
				// msg 306: @1FE(TCH_PASSWORD)	   fetch password from file
	{IN_SW_BURP_FIX_FSS_DATA,		isc_spb_res_fix_fss_data,
												"FIX_FSS_DATA", 	0, 0, 0, false, 302,	9, NULL, boRestore},
				// msg 302: @1FIX_FSS_DATA		   fix malformed UNICODE_FSS data
	{IN_SW_BURP_FIX_FSS_METADATA,	isc_spb_res_fix_fss_metadata,
												"FIX_FSS_METADATA", 0, 0, 0, false, 303,	9, NULL, boRestore},
				// msg 303: @1FIX_FSS_METADATA	   fix malformed UNICODE_FSS metadata
	{IN_SW_BURP_G,	  isc_spb_bkp_no_garbage_collect, "GARBAGE_COLLECT", 0, 0, 0, false, 177, 1, NULL, boBackup},
				// msg 177:@1GARBAGE_COLLECT inhibit garbage collection
	{IN_SW_BURP_I,	  isc_spb_res_deactivate_idx, "INACTIVE",		0, 0, 0, false, 78, 	1, NULL, boRestore},
				// msg 78:@1INACTIVE deactivate indexes during restore
	{IN_SW_BURP_IG,   isc_spb_bkp_ignore_checksums,   "IGNORE", 	0, 0, 0, false, 178,	2, NULL, boBackup},
				// msg 178:@1IGNORE ignore bad checksums
	{IN_SW_BURP_K,	  isc_spb_res_no_shadow,	"KILL", 			0, 0, 0, false, 172,	1, NULL, boRestore},
				// msg 172:@1KILL restore without creating shadows
	{IN_SW_BURP_L,	  isc_spb_bkp_ignore_limbo, "LIMBO",			0, 0, 0, false, 98, 	1, NULL, boBackup},
				// msg 98 ignore transactions in limbo
	{IN_SW_BURP_M,	  isc_spb_bkp_metadata_only,	"METADATA", 	0, 0, 0, false, 0,		1, NULL, boGeneral},
	{IN_SW_BURP_M,	  0,						"META_DATA",		0, 0, 0, false, 63, 	1, NULL, boGeneral},
				// msg 63: @1META_DATA backup or restore metadata only
	{IN_SW_BURP_MODE, 0,						"MODE", 			0, 0, 0, false, 278,	2, NULL, boRestore},
				// msg 278: @1MODE read_only or read_write access
	{IN_SW_BURP_N,	  isc_spb_res_no_validity,	"NO_VALIDITY",		0, 0, 0, false, 187,	1, NULL, boRestore},
				// msg 187: @1N(O_VALIDITY) do not restore database validity conditions
	{IN_SW_BURP_NOD,  isc_spb_bkp_no_triggers,	"NODBTRIGGERS", 	0, 0, 0, false, 294,	3, NULL, boBackup},
				// msg 294: @1NOD(BTRIGGERS) do not run database triggers
	{IN_SW_BURP_NT,   isc_spb_bkp_non_transportable,	  "NT", 	0, 0, 0, false, 239,	2, NULL, boBackup},
				// msg 239: @1NT Non-Transportable backup file format
	{IN_SW_BURP_O,	  isc_spb_res_one_at_a_time, "ONE_AT_A_TIME",	0, 0, 0, false, 99,	1, NULL, boRestore},
				// msg 99: @1ONE_AT_A_TIME restore one relation at a time
	{IN_SW_BURP_OL,	isc_spb_bkp_old_descriptions, "OLD_DESCRIPTIONS", 0, 0, 0, false, 186, 2, NULL, boBackup},
				// msg 186: @1OLD_DESCRIPTIONS save old style metadata descriptions
	{IN_SW_BURP_P,	isc_spb_res_page_size,		"PAGE_SIZE",		0, 0, 0, false, 101,	1, NULL, boRestore},
				// msg 101: @1PAGE_SIZE override default page size
	{IN_SW_BURP_PASS, 0,						"PASSWORD", 		0, 0, 0, false, 190,	3, NULL, boGeneral},
				// msg 190: @1PA(SSWORD) Firebird password
	{IN_SW_BURP_RECREATE, 0,					"RECREATE_DATABASE", 0, 0, 0, false, 284,	1, NULL, boMain},
				// msg 284: @1R(ECREATE_DATABASE) [O(VERWRITE)] create (or replace if OVERWRITE used) database from backup file
	{IN_SW_BURP_R,	  isc_spb_res_replace,		"REPLACE_DATABASE", 0, 0, 0, false, 112,	3, NULL, boMain},
				// msg 112: @1REP(LACE_DATABASE) replace database from backup file
/**************************************************************
** msg 252: @1RO(LE) Firebird SQL role
***************************************************************/
	{IN_SW_BURP_ROLE, isc_spb_sql_role_name,	"ROLE",				0, 0, 0, false, 252,	2, NULL, boGeneral},
	{IN_SW_BURP_S,	  0,						"SKIP_BAD_DATA",	0, 0, 0, false, 0,		4, NULL, boRestore},
	{IN_SW_BURP_SE,   0,						"SERVICE",			0, 0, 0, false, 277,	2, NULL, boGeneral},
				// msg 277: @1SE(RVICE) use services manager
	{IN_SW_BURP_SKIP_DATA,   0,					"SKIP_DATA",		0, 0, 0, false, 355,	6, NULL, boGeneral},
				// msg 355: @1SKIP_DATA skip data for table
	{IN_SW_BURP_T,	  0,						"TRANSPORTABLE",	0, 0, 0, false, 175,	1, NULL, boBackup},
				// msg 175: @1TRANSPORTABLE transportable backup -- data in XDR format
#ifdef TRUSTED_AUTH
	{IN_SW_BURP_TRUSTED_AUTH, 0,				"TRUSTED",			0, 0, 0, false, 295,	3, NULL, boGeneral},
				// msg 295: @1TRU(STED) 		   use trusted authentication
#endif
//	{IN_SW_BURP_TRUSTED_USER, 0,				TRUSTED_USER_SWITCH, 0, 0, 0, false, 0,		TRUSTED_USER_SWITCH_LEN, NULL, boGeneral},
//	{IN_SW_BURP_TRUSTED_ROLE, 0,				TRUSTED_ROLE_SWITCH, 0, 0, 0, false, 0,		TRUSTED_ROLE_SWITCH_LEN, NULL, boGeneral},
/*
	{IN_SW_BURP_U,	  0,						"UNPROTECTED",		0, 0, 0, false, 0,		5, NULL, boGeneral},
*/
	{IN_SW_BURP_US,   isc_spb_res_use_all_space,	"USE_ALL_SPACE", 0, 0, 0, false, 276,	4, NULL, boRestore},
				// msg 276: @1USE_(ALL_SPACE) do not reserve space for record versions
	{IN_SW_BURP_USER, 0,						"USER",				0, 0, 0, false, 191,	4, NULL, boGeneral},
				// msg 191: @1USER Firebird user name
	{IN_SW_BURP_V,	  isc_spb_verbose,			"VERBOSE",			0, 0, 0, false, 0,		1, NULL, boGeneral},
	{IN_SW_BURP_V,	  0,						"VERIFY",			0, 0, 0, false, 113,	1, NULL, boGeneral},
				// msg 113: @1VERIFY report each action taken
	{IN_SW_BURP_VERBINT, isc_spb_verbint,		"VERBINT",			0, 0, 0, false, 328,	5, NULL, boGeneral},
				// msg 328: @1VERBI(NT) <n> verbose information with explicit interval
	{IN_SW_BURP_Y,	  0,						"Y",				0, 0, 0, false, 109,	1, NULL, boGeneral},
				// msg 109: @1Y redirect/suppress output (file path or OUTPUT_SUPPRESS)
	{IN_SW_BURP_Z,	  0,						"Z",				0, 0, 0, false, 104,	1, NULL, boGeneral},
				// msg 104: @1Z print version number
/**************************************************************************/
// The next two 'virtual' switches are hidden from user and are needed
// for services API
/**************************************************************************/
	{IN_SW_BURP_HIDDEN_RDONLY,	isc_spb_res_am_readonly,	"MODE READ_ONLY",	0, 0, 0, false, 0, 14, NULL, boRestore},
	{IN_SW_BURP_HIDDEN_RDWRITE, isc_spb_res_am_readwrite,	"MODE READ_WRITE",	0, 0, 0, false, 0, 15, NULL, boRestore},
/**************************************************************************/
	{IN_SW_BURP_0,		 0, NULL,			0, 0, 0, false, 0, 0, NULL, boGeneral}
};


#endif // BURP_BURP_H

