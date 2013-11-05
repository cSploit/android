/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.5.2.26540 Firebird 2.5 **********************/
/*
 *	PROGRAM:	JRD Data Definition Utility
 *	MODULE:		dyn.epp
 *	DESCRIPTION:	Dynamic data definition
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
 *    2001.04.20 Claudio Valderrama: Fix bug in grant/revoke by making user
 *                                      case insensitive.
 *    2001.05.24 Claudio Valderrama: Move DYN_delete_role to dyn_del.e.
 *    2001.06.05 John Bellardo: Renamed the revoke static function to
 *                                revoke_permission, because there is already
 *                                a revoke(2) function in *nix.
 *    2001.06.20 Claudio Valderrama: Enable calls to DYN_delete_generator.
 *    2001.10.01 Claudio Valderrama: Enable explicit GRANT...to ROLE role_name.
 *			and complain if the grantee ROLE doesn't exist.
 *    2001.10.06 Claudio Valderrama: Forbid "NONE" from role-related operations.
 *				Honor explicit USER keyword in GRANTs and REVOKEs. *
 *    2002.08.10 Dmitry Yemanov: ALTER VIEW
 *    2002.10.29 Nickolay Samofatov: Added support for savepoints
 *
 *	Alex Peshkov
 */

#include "firebird.h"
#include <stdio.h>
#include <string.h>

#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/ods.h"
#include "../jrd/tra.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/UserManagement.h"
#include "../jrd/scl.h"
#include "../jrd/drq.h"
#include "../jrd/flags.h"
#include "../jrd/ibase.h"
#include "../jrd/lls.h"
#include "../jrd/met.h"
#include "../jrd/btr.h"
#include "../jrd/intl.h"
#include "../jrd/dyn.h"
//#include "../jrd/license.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/dyn_proto.h"
#include "../jrd/dyn_df_proto.h"
#include "../jrd/dyn_dl_proto.h"
#include "../jrd/dyn_md_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/inf_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/isc_f_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/thread_proto.h"
#include "../common/utils_proto.h"
#include "../jrd/jrd_pwd.h"
#include "../utilities/gsec/gsec.h"
#include "../jrd/msg_encode.h"

using MsgFormat::SafeArg;


/*DATABASE DB = STATIC "ODS.RDB";*/
static const UCHAR	jrd_0 [124] =
   {	// blr string 

4,2,4,0,9,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,
0,7,0,7,0,7,0,41,0,0,7,0,12,0,15,'K',18,0,0,2,1,25,0,4,0,24,0,
3,0,1,25,0,8,0,24,0,2,0,1,25,0,5,0,24,0,7,0,1,25,0,6,0,24,0,6,
0,1,25,0,0,0,24,0,1,0,1,25,0,1,0,24,0,0,0,1,25,0,2,0,24,0,4,0,
1,41,0,3,0,7,0,24,0,5,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_11 [71] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',5,0,0,
'G',47,24,0,14,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,
0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_16 [156] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,2,0,41,3,0,32,0,7,0,4,1,3,0,41,3,0,32,0,7,
0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,2,7,'C',1,'K',5,0,
0,'G',58,47,24,0,0,0,25,0,1,0,58,47,24,0,1,0,25,0,0,0,61,24,0,
14,0,255,2,14,1,2,1,24,0,14,0,41,1,0,0,2,0,1,21,8,0,1,0,0,0,25,
1,1,0,255,17,0,9,13,12,3,18,0,12,2,10,0,1,2,1,41,2,0,0,1,0,24,
1,14,0,255,255,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_29 [124] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,2,0,41,3,0,32,0,7,0,4,0,2,0,41,
3,0,32,0,7,0,12,0,2,7,'C',1,'K',18,0,0,'G',58,47,24,0,0,0,25,
0,0,0,47,24,0,6,0,25,0,1,0,255,2,14,1,2,1,24,0,1,0,25,1,0,0,1,
21,8,0,1,0,0,0,25,1,1,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,
255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_40 [166] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,2,0,41,3,0,32,0,7,0,4,0,5,0,41,
3,0,32,0,41,3,0,32,0,7,0,7,0,41,0,0,7,0,12,0,2,7,'C',1,'K',18,
0,0,'G',58,47,24,0,2,0,25,0,4,0,58,47,24,0,4,0,25,0,1,0,58,47,
24,0,7,0,25,0,3,0,58,47,24,0,0,0,25,0,0,0,47,24,0,6,0,25,0,2,
0,255,2,14,1,2,1,24,0,1,0,25,1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,
255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,
0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_54 [181] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,2,0,41,3,0,32,0,7,0,4,0,6,0,41,
3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,41,0,0,7,0,12,0,2,7,
'C',1,'K',18,0,0,'G',58,47,24,0,2,0,25,0,5,0,58,47,24,0,4,0,25,
0,2,0,58,47,24,0,7,0,25,0,4,0,58,47,24,0,0,0,25,0,1,0,58,47,24,
0,6,0,25,0,3,0,47,24,0,5,0,25,0,0,0,255,2,14,1,2,1,24,0,1,0,25,
1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,17,0,9,13,12,3,18,0,12,2,
5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_69 [135] =
   {	// blr string 

4,2,4,1,2,0,7,0,7,0,4,0,4,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,12,
0,2,7,'C',1,'K',18,0,0,'G',58,47,24,0,0,0,103,25,0,1,0,58,47,
24,0,6,0,25,0,3,0,58,47,24,0,4,0,25,0,0,0,58,47,24,0,7,0,25,0,
2,0,47,24,0,2,0,21,15,3,0,1,0,'M',255,14,1,2,1,21,8,0,1,0,0,0,
25,1,0,0,1,24,0,3,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,0,
0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_78 [144] =
   {	// blr string 

4,2,4,1,4,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,4,0,1,0,41,
3,0,32,0,12,0,2,7,'C',2,'K',5,0,0,'K',7,0,1,'G',58,47,24,0,1,
0,25,0,0,0,58,59,61,24,0,4,0,58,47,24,1,0,0,24,0,1,0,47,24,1,
2,0,24,0,10,0,255,14,1,2,1,24,0,4,0,25,1,0,0,1,24,1,1,0,25,1,
1,0,1,24,0,0,0,25,1,2,0,1,21,8,0,1,0,0,0,25,1,3,0,255,14,1,1,
21,8,0,0,0,0,0,25,1,3,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_86 [159] =
   {	// blr string 

4,2,4,1,5,0,41,3,0,32,0,7,0,7,0,7,0,7,0,4,0,5,0,41,3,0,32,0,41,
3,0,32,0,7,0,7,0,41,0,0,7,0,12,0,2,7,'C',1,'K',18,0,0,'G',58,
47,24,0,0,0,103,25,0,1,0,58,47,24,0,6,0,25,0,3,0,58,47,24,0,4,
0,25,0,0,0,58,47,24,0,7,0,25,0,2,0,47,24,0,2,0,25,0,4,0,255,14,
1,2,1,24,0,5,0,41,1,0,0,4,0,1,21,8,0,1,0,0,0,25,1,1,0,1,24,0,
3,0,41,1,3,0,2,0,255,14,1,1,21,8,0,0,0,0,0,25,1,1,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_99 [87] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,2,7,'C',
1,'K',6,0,0,'G',58,47,24,0,8,0,25,0,1,0,47,24,0,13,0,103,25,0,
0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,
0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_105 [86] =
   {	// blr string 

4,2,4,1,1,0,7,0,4,0,2,0,41,3,0,32,0,41,3,0,32,0,12,0,2,7,'C',
1,'K',5,0,0,'G',58,47,24,0,1,0,25,0,1,0,47,24,0,0,0,25,0,0,0,
255,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,255,14,1,1,21,8,0,0,0,0,
0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_111 [86] =
   {	// blr string 

4,2,4,1,3,0,7,0,7,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',1,'K',
6,0,0,'G',47,24,0,8,0,25,0,0,0,255,14,1,2,1,21,8,0,1,0,0,0,25,
1,0,0,1,24,0,15,0,41,1,2,0,1,0,255,14,1,1,21,8,0,0,0,0,0,25,1,
0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_118 [188] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,3,0,7,0,7,0,7,0,4,0,6,0,41,3,
0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,41,0,0,7,0,12,0,2,7,'C',
1,'K',18,0,0,'G',58,47,24,0,4,0,25,0,2,0,58,47,24,0,7,0,25,0,
4,0,58,47,24,0,2,0,25,0,5,0,58,47,24,0,0,0,25,0,1,0,58,47,24,
0,6,0,25,0,3,0,58,47,24,0,1,0,25,0,0,0,61,24,0,5,0,255,2,14,1,
2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,3,0,41,1,2,0,1,0,255,17,0,
9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,8,0,0,0,0,0,25,1,0,
0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_134 [197] =
   {	// blr string 

4,2,4,3,1,0,7,0,4,2,1,0,7,0,4,1,3,0,7,0,7,0,7,0,4,0,7,0,41,3,
0,32,0,41,3,0,32,0,41,3,0,32,0,41,3,0,32,0,7,0,7,0,41,0,0,7,0,
12,0,2,7,'C',1,'K',18,0,0,'G',58,47,24,0,4,0,25,0,3,0,58,47,24,
0,7,0,25,0,5,0,58,47,24,0,2,0,25,0,6,0,58,47,24,0,0,0,25,0,2,
0,58,47,24,0,6,0,25,0,4,0,58,47,24,0,1,0,25,0,1,0,47,24,0,5,0,
25,0,0,0,255,2,14,1,2,1,21,8,0,1,0,0,0,25,1,0,0,1,24,0,3,0,41,
1,2,0,1,0,255,17,0,9,13,12,3,18,0,12,2,5,0,255,255,14,1,1,21,
8,0,0,0,0,0,25,1,0,0,255,255,76
   };	// end of blr string 
static const UCHAR	jrd_151 [85] =
   {	// blr string 

4,2,4,1,2,0,41,3,0,32,0,7,0,4,0,1,0,41,3,0,32,0,12,0,2,7,'C',
1,'K',31,0,0,'G',47,24,0,0,0,25,0,0,0,255,14,1,2,1,24,0,1,0,25,
1,0,0,1,21,8,0,1,0,0,0,25,1,1,0,255,14,1,1,21,8,0,0,0,0,0,25,
1,1,0,255,255,76
   };	// end of blr string 


using namespace Jrd;
using namespace Firebird;


static void grant(Global*, const UCHAR**);
static bool grantor_can_grant(Global*, const TEXT*, const TEXT*, const MetaName&,
	const MetaName&, bool);
static bool grantor_can_grant_role(thread_db*, Global*, const MetaName&, const MetaName&);
static const char* privilege_name(char symbol);
static void	revoke_permission(Global*, const UCHAR**);
static void revoke_all(Global*, const UCHAR**);
static void store_privilege(Global*, const MetaName&, const MetaName&, const MetaName&,
	const TEXT*, SSHORT, SSHORT, int, const MetaName&);
static void	set_field_class_name(Global*, const MetaName&, const MetaName&);
static void dyn_user(Global*, const UCHAR**);


void DYN_ddl(/*Attachment* attachment,*/ jrd_tra* transaction, USHORT length, const UCHAR* ddl)
{
/**************************************
 *
 *	D Y N _ d d l
 *
 **************************************
 *
 * Functional description
 *	Do meta-data.
 *
 **************************************/

	thread_db* const tdbb = JRD_get_thread_data();
	Database* const dbb = tdbb->getDatabase();

	const UCHAR* ptr = ddl;

	if (*ptr++ != isc_dyn_version_1) {
		ERR_post(Arg::Gds(isc_wrodynver));
	}

	fb_utils::init_status(tdbb->tdbb_status_vector);

	Global gbl(transaction);

	// Create a pool for DYN to operate in.  It will be released when
	// the routine exits.
	MemoryPool* const tempPool = dbb->createPool();
	Jrd::ContextPoolHolder context(tdbb, tempPool);

	try {
		Database::CheckoutLockGuard guard(dbb, dbb->dbb_dyn_mutex);

		VIO_start_save_point(tdbb, transaction);
		transaction->tra_save_point->sav_verb_count++;

		DYN_execute(&gbl, &ptr, NULL, NULL, NULL, NULL, NULL);
		transaction->tra_save_point->sav_verb_count--;
		VIO_verb_cleanup(tdbb, transaction);
	}
	catch (const Exception& ex)
	{
		stuff_exception(tdbb->tdbb_status_vector, ex);
		const Savepoint* savepoint = transaction->tra_save_point;
		if (savepoint && savepoint->sav_verb_count)
		{
			// An error during undo is very bad and has to be dealt with
			// by aborting the transaction.  The easiest way is to kill
			// the application by calling bugcheck.

			try {
				VIO_verb_cleanup(tdbb, transaction);
			}
			catch (const Exception&) {
				BUGCHECK(290);	// msg 290 error during savepoint backout
			}
		}

		dbb->deletePool(tempPool);

		ERR_punt();
	}

	dbb->deletePool(tempPool);
}


void DYN_error(bool	status_flag, USHORT number, const SafeArg& sarg)
{
/**************************************
 *
 *	D Y N _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	DDL failed.
 *
 **************************************/
	thread_db* const tdbb = JRD_get_thread_data();

	if (tdbb->tdbb_status_vector[1] == isc_no_meta_update)
		return;

	TEXT error_buffer[BUFFER_MEDIUM];
	Arg::Gds local_status(isc_no_meta_update);

	if (number)
	{
		fb_msg_format(NULL, DYN_MSG_FAC, number, sizeof(error_buffer), error_buffer, sarg);

		ISC_STATUS_ARRAY temp_status;
		fb_utils::init_status(temp_status);
		temp_status[1] = ENCODE_ISC_MSG(number, DYN_MSG_FAC);
		FB_SQLSTATE_STRING sqlstate;
		fb_sqlstate(sqlstate, temp_status);
		if (!strcmp(sqlstate, "HY000"))
			strcpy(sqlstate, "42000"); // default SQLSTATE for DYN

		local_status << Arg::Gds(isc_random) << Arg::Str(error_buffer) << Arg::SqlState(sqlstate);
	}
	ERR_make_permanent(local_status);
	if (status_flag) {
		local_status.append(Arg::StatusVector(tdbb->tdbb_status_vector));
	}

	local_status.copyTo(tdbb->tdbb_status_vector);
}


void DYN_error_punt(bool status_flag, USHORT number, const SafeArg& arg)
{
/**************************************
 *
 *	D Y N _ e r r o r _ p u n t
 *
 **************************************
 *
 * Functional description
 *	DDL failed.
 *
 **************************************/

	DYN_error(status_flag, number, arg);
	ERR_punt();
}


void DYN_error_punt(bool status_flag, USHORT number, const char* str)
{
/**************************************
 *
 *	D Y N _ e r r o r _ p u n t
 *
 **************************************
 *
 * Functional description
 *	DDL failed.
 *
 **************************************/

	DYN_error(status_flag, number, SafeArg() << str);
	ERR_punt();
}


void DYN_error_punt(bool status_flag, USHORT number)
{
/**************************************
 *
 *	D Y N _ e r r o r _ p u n t
 *
 **************************************
 *
 * Functional description
 *	DDL failed.
 *
 **************************************/

	static const SafeArg dummy;
	DYN_error(status_flag, number, dummy);
	ERR_punt();
}


bool DYN_is_it_sql_role(Global* gbl,
						const MetaName&	input_name,
						MetaName& output_name,
						thread_db* tdbb)
{
   struct {
          TEXT  jrd_155 [32];	// RDB$OWNER_NAME 
          SSHORT jrd_156;	// gds__utility 
   } jrd_154;
   struct {
          TEXT  jrd_153 [32];	// RDB$ROLE_NAME 
   } jrd_152;
/**************************************
 *
 *	D Y N _ i s _ i t _ s q l _ r o l e
 *
 **************************************
 *
 * Functional description
 *
 *	If input_name is found in RDB$ROLES, then returns true. Otherwise
 *    returns false.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	const USHORT major_version = dbb->dbb_ods_version;
	const USHORT minor_original = dbb->dbb_minor_original;
	bool found = false;

	if (ENCODE_ODS(major_version, minor_original) < ODS_9_0)
		return found;

	jrd_req* request = CMP_find_request(tdbb, drq_get_role_nm, DYN_REQUESTS);

	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		X IN RDB$ROLES WITH
			X.RDB$ROLE_NAME EQ input_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_151, sizeof(jrd_151), true);
	gds__vtov ((const char*) input_name.c_str(), (char*) jrd_152.jrd_153, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_152);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_154);
	   if (!jrd_154.jrd_156) break;
		if (!DYN_REQUEST(drq_get_role_nm))
			DYN_REQUEST(drq_get_role_nm) = request;

		found = true;
		output_name = /*X.RDB$OWNER_NAME*/
			      jrd_154.jrd_155;

	/*END_FOR;*/
	   }
	}

	if (!DYN_REQUEST(drq_get_role_nm))
		DYN_REQUEST(drq_get_role_nm) = request;

	return found;
}


void DYN_unsupported_verb()
{
/**************************************
 *
 *	D Y N _ u n s u p p o r t e d _ v e r b
 *
 **************************************
 *
 * Functional description
 *	We encountered an unsupported dyn verb.
 *
 **************************************/

	static const SafeArg dummy;
	DYN_error_punt(false, 2, dummy);	// msg 2: "unsupported DYN verb"
}


void DYN_execute(Global* gbl,
				 const UCHAR** ptr,
				 const MetaName* relation_name,
				 MetaName* field_name,
				 MetaName* trigger_name,
				 MetaName* function_name,
				 MetaName* procedure_name)
{
/**************************************
 *
 *	D Y N _ e x e c u t e
 *
 **************************************
 *
 * Functional description
 *	Execute a dynamic ddl statement.
 *
 **************************************/
	UCHAR verb;

	switch (verb = *(*ptr)++)
	{
	case isc_dyn_begin:
		while (**ptr != isc_dyn_end)
		{
			DYN_execute(gbl, ptr, relation_name, field_name,
						trigger_name, function_name, procedure_name);
		}
		++(*ptr);
		break;

		// Runtime security-related dynamic DDL should not require licensing.
		// A placeholder case statement for SQL 3 Roles is reserved below.

	case isc_dyn_grant:
		grant(gbl, ptr);
		break;

	case isc_dyn_revoke:
		revoke_permission(gbl, ptr);
		break;

	case isc_dyn_revoke_all:
		revoke_all(gbl, ptr);
		break;

/***
    case isc_dyn_def_role:
        create_role (gbl, ptr);
	break;
***/
	default:
		// make sure that the license allows metadata operations

		switch (verb)
		{
		case isc_dyn_mod_database:
			DYN_modify_database(gbl, ptr);
			break;

		case isc_dyn_def_collation:
			DYN_define_collation(gbl, ptr);
			break;

		case isc_dyn_del_collation:
			DYN_delete_collation(gbl, ptr);
			break;

		case isc_dyn_def_rel:
		case isc_dyn_def_view:
			DYN_define_relation(gbl, ptr);
			break;

		case isc_dyn_mod_rel:
			DYN_modify_relation(gbl, ptr);
			break;

		case isc_dyn_mod_view:
			DYN_modify_view(gbl, ptr);
			break;

		case isc_dyn_delete_rel:
			DYN_delete_relation(gbl, ptr, relation_name);
			break;

		case isc_dyn_def_security_class:
			DYN_define_security_class(gbl, ptr);
			break;

		case isc_dyn_delete_security_class:
			DYN_delete_security_class(gbl, ptr);
			break;

		case isc_dyn_def_exception:
			DYN_define_exception(gbl, ptr);
			break;

		case isc_dyn_mod_exception:
			DYN_modify_exception(gbl, ptr);
			break;

		case isc_dyn_del_exception:
			DYN_delete_exception(gbl, ptr);
			break;

		case isc_dyn_def_filter:
			DYN_define_filter(gbl, ptr);
			break;

		case isc_dyn_mod_filter:
			DYN_modify_filter(gbl, ptr);
			break;

		case isc_dyn_delete_filter:
			DYN_delete_filter(gbl, ptr);
			break;

		case isc_dyn_def_function:
			DYN_define_function(gbl, ptr);
			break;

		case isc_dyn_def_function_arg:
			DYN_define_function_arg(gbl, ptr, function_name);
			break;

		case isc_dyn_mod_function:
			DYN_modify_function(gbl, ptr);
			break;

		case isc_dyn_delete_function:
			DYN_delete_function(gbl, ptr);
			break;

		case isc_dyn_def_generator:
			DYN_define_generator(gbl, ptr);
			break;

		case isc_dyn_mod_generator:
			DYN_modify_generator(gbl, ptr);
			break;

		case isc_dyn_delete_generator:
			DYN_delete_generator(gbl, ptr);
			break;

		case isc_dyn_def_sql_role:
			DYN_define_role(gbl, ptr);
			break;

		case isc_dyn_mod_sql_role:
			DYN_modify_role(gbl, ptr);
			break;

		case isc_dyn_del_sql_role:
			DYN_delete_role(gbl, ptr);
			break;

		case isc_dyn_def_procedure:
			DYN_define_procedure(gbl, ptr);
			break;

		case isc_dyn_mod_procedure:
			DYN_modify_procedure(gbl, ptr);
			break;

		case isc_dyn_delete_procedure:
			DYN_delete_procedure(gbl, ptr);
			break;

		case isc_dyn_def_parameter:
			DYN_define_parameter(gbl, ptr, procedure_name);
			break;

		case isc_dyn_mod_prc_parameter:
			DYN_modify_parameter(gbl, ptr);
			break;

		case isc_dyn_delete_parameter:
			DYN_delete_parameter(gbl, ptr, procedure_name);
			break;

		case isc_dyn_def_shadow:
			DYN_define_shadow(gbl, ptr);
			break;

		case isc_dyn_delete_shadow:
			DYN_delete_shadow(gbl, ptr);
			break;

		case isc_dyn_def_trigger:
			DYN_define_trigger(gbl, ptr, relation_name, NULL, false);
			break;

		case isc_dyn_mod_trigger:
			DYN_modify_trigger(gbl, ptr);
			break;

		case isc_dyn_delete_trigger:
			DYN_delete_trigger(gbl, ptr);
			break;

		case isc_dyn_def_trigger_msg:
			DYN_define_trigger_msg(gbl, ptr, trigger_name);
			break;

		case isc_dyn_mod_trigger_msg:
			DYN_modify_trigger_msg(gbl, ptr, trigger_name);
			break;

		case isc_dyn_delete_trigger_msg:
			DYN_delete_trigger_msg(gbl, ptr, trigger_name);
			break;

		case isc_dyn_def_global_fld:
			DYN_define_global_field(gbl, ptr, relation_name, field_name);
			break;

		case isc_dyn_mod_global_fld:
			DYN_modify_global_field(gbl, ptr, relation_name, field_name);
			break;

		case isc_dyn_delete_global_fld:
			DYN_delete_global_field(gbl, ptr);
			break;

		case isc_dyn_def_local_fld:
			DYN_define_local_field(gbl, ptr, relation_name, field_name);
			break;

		case isc_dyn_mod_local_fld:
			DYN_modify_local_field(gbl, ptr, relation_name);
			break;

		case isc_dyn_delete_local_fld:
			DYN_delete_local_field(gbl, ptr, relation_name); //, field_name);
			break;

		case isc_dyn_mod_sql_fld:
			DYN_modify_sql_field(gbl, ptr, relation_name);
			break;

		case isc_dyn_def_sql_fld:
			DYN_define_sql_field(gbl, ptr, relation_name, field_name);
			break;

		case isc_dyn_rel_constraint:
			DYN_define_constraint(gbl, ptr, relation_name, field_name);
			break;

		case isc_dyn_delete_rel_constraint:
			DYN_delete_constraint(gbl, ptr, relation_name);
			break;

		case isc_dyn_def_idx:
			DYN_define_index(gbl, ptr, relation_name, verb, NULL, NULL, NULL, NULL);
			break;

		case isc_dyn_mod_idx:
			DYN_modify_index(gbl, ptr);
			break;

		case isc_dyn_delete_idx:
			DYN_delete_index(gbl, ptr);
			break;

		case isc_dyn_view_relation:
			DYN_define_view_relation(gbl, ptr, relation_name);
			break;

		case isc_dyn_def_dimension:
			DYN_define_dimension(gbl, ptr, relation_name, field_name);
			break;

		case isc_dyn_delete_dimensions:
			DYN_delete_dimensions(gbl, ptr); // relation_name, field_name);
			break;

		case isc_dyn_mod_charset:
			DYN_modify_charset(gbl, ptr);
			break;

		case isc_dyn_mod_collation:
			DYN_modify_collation(gbl, ptr);
			break;

		case isc_dyn_mapping:
			DYN_modify_mapping(gbl, ptr);
			break;

		case isc_dyn_user:
			dyn_user(gbl, ptr);
			break;

		default:
			DYN_unsupported_verb();
			break;
		}
	}
}


SLONG DYN_get_number(const UCHAR** ptr)
{
/**************************************
 *
 *	D Y N _ g e t _ n u m b e r
 *
 **************************************
 *
 * Functional description
 *	Pick up a number and possible clear a null flag.
 *
 **************************************/
	const UCHAR* p = *ptr;
	USHORT length = *p++;
	length |= (*p++) << 8;
	*ptr = p + length;

	return gds__vax_integer(p, length);
}


USHORT DYN_get_string(const TEXT** ptr, TEXT* field, size_t size, bool transliterate)
{
/**************************************
 *
 *	D Y N _ g e t _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Pick up a string, move to a target, and, if requested,
 *	set a null flag.  Return length of string.
 *	If destination field size is too small, punt.
 *	Strings need enough space for null pad.
 *
 **************************************/
	const TEXT* p = *ptr;
	USHORT length = (UCHAR) *p++;
	length |= ((USHORT) ((UCHAR) (*p++))) << 8;

	HalfStaticArray<TEXT, MAX_SQL_IDENTIFIER_LEN> temp;
	USHORT l = length;

	if (l)
	{
		if (length >= size) {
			DYN_error_punt(false, 159); // msg 159: Name longer than database field size
		}

		TEXT* p2 = (transliterate ? temp.getBuffer(length) : field);

		do {
			*p2++ = *p++;
		} while (--l);
	}

	*ptr = p;

	if (transliterate)
	{
		length = INTL_convert_bytes(JRD_get_thread_data(),
			ttype_metadata, (BYTE*) field, size - 1,
			ttype_dynamic, (const BYTE*) temp.begin(), length, ERR_post);
	}

	field[length] = 0;

	return length;
}


USHORT DYN_get_string(const TEXT** ptr, MetaName& field, size_t, bool transliterate)
{
/**************************************
 *
 *	D Y N _ g e t _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Pick up an object name, move to a target. Return length of string.
 *	If destination field size is too small, punt.
 *
 **************************************/
	const TEXT* p = *ptr;
	USHORT length = (UCHAR) *p++;
	length |= ((USHORT) ((UCHAR) (*p++))) << 8;

	if (length > MAX_SQL_IDENTIFIER_LEN)
	{
		DYN_error_punt(false, 159);
		// msg 159: Name longer than database field size
	}
	field.assign(p, length);
	p += length;

	*ptr = p;

	if (transliterate)
	{
		TEXT temp[MAX_SQL_IDENTIFIER_LEN];

		length = INTL_convert_bytes(JRD_get_thread_data(),
			ttype_metadata, (BYTE*) temp, sizeof(temp),
			ttype_dynamic, (const BYTE*) field.c_str(), field.length(), ERR_post);
		field.assign(temp, length);
	}

	return length;
}


USHORT DYN_get_string(const TEXT** ptr, string& field, size_t, bool transliterate)
{
/**************************************
 *
 *	D Y N _ g e t _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Pick up an object name, move to a target. Return length of string.
 *	If destination field size is too small, punt.
 *
 **************************************/
	const TEXT* p = *ptr;
	USHORT length = (UCHAR) *p++;
	length |= ((USHORT) ((UCHAR) (*p++))) << 8;

	if (length > MAX_SQL_IDENTIFIER_LEN)
	{
		DYN_error_punt(false, 159);
		// msg 159: Name longer than database field size
	}
	field.assign(p, length);
	p += length;

	*ptr = p;

	if (transliterate)
	{
		TEXT temp[MAX_SQL_IDENTIFIER_LEN];

		length = INTL_convert_bytes(JRD_get_thread_data(),
			ttype_metadata, (BYTE*) temp, sizeof(temp),
			ttype_dynamic, (const BYTE*) field.c_str(), field.length(), ERR_post);
		field.assign(temp, length);
	}

	return length;
}


USHORT DYN_get_string(const TEXT** ptr, PathName& field, size_t, bool transliterate)
{
/**************************************
 *
 *	D Y N _ g e t _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Pick up a file name, move to a target. Return length
 *	of string.
 *
 **************************************/
	const TEXT* p = *ptr;
	USHORT length = (UCHAR) *p++;
	length |= ((USHORT) ((UCHAR) (*p++))) << 8;

	field.assign(p, length);
	p += length;

	*ptr = p;

	if (transliterate)
	{
		thread_db* tdbb = JRD_get_thread_data();
		PathName temp;

		temp.reserve(
			INTL_convert_bytes(tdbb,
				ttype_metadata, NULL, 0,
				ttype_dynamic, (const BYTE*) field.begin(), field.length(), ERR_post));

		length = INTL_convert_bytes(tdbb,
			ttype_metadata, (BYTE*) temp.begin(), temp.capacity(),
			ttype_dynamic, (const BYTE*) field.begin(), field.length(), ERR_post);
		field.assign(temp.c_str(), length);
	}

	return length;
}


USHORT DYN_get_string(const TEXT** ptr, UCharBuffer& array, size_t, bool transliterate)
{
/**************************************
 *
 *	D Y N _ g e t _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Pick up a string, move to a target. Return length
 *	of string.
 *
 **************************************/
	const TEXT* p = *ptr;
	USHORT length = (UCHAR) *p++;
	length |= ((USHORT) ((UCHAR) (*p++))) << 8;

	if (transliterate)
	{
		UCharBuffer temp;
		memcpy(temp.getBuffer(length), p, length);

		thread_db* tdbb = JRD_get_thread_data();

		array.resize(
			INTL_convert_bytes(tdbb,
				ttype_metadata, NULL, 0,
				ttype_dynamic, temp.begin(), length, ERR_post));

		length = INTL_convert_bytes(tdbb,
			ttype_metadata, array.begin(), array.getCapacity(),
			ttype_dynamic, temp.begin(), length, ERR_post);
		array.resize(length);
	}
	else
		memcpy(array.getBuffer(length), p, length);

	p += length;
	*ptr = p;

	return length;
}


USHORT DYN_put_blr_blob(Global* gbl, const UCHAR** ptr, bid* blob_id)
{
/**************************************
 *
 *	D Y N _ p u t _ b l r _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Write out a blr blob.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	const UCHAR *p = *ptr;
	USHORT length = *p++;
	length |= (*p++) << 8;

	if (!length)
	{
		*ptr = p;
		return length;
	}

	try {
		blb* blob = BLB_create(tdbb, gbl->gbl_transaction, blob_id);
		BLB_put_segment(tdbb, blob, p, length);
		BLB_close(tdbb, blob);
	}
	catch (const Exception& ex)
	{
		stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_error_punt(true, 106);
		// msg 106: "Create metadata blob failed"
	}

	*ptr = p + length;

	return length;
}

USHORT DYN_put_text_blob(Global* gbl, const UCHAR** ptr, bid* blob_id)
{
/**************************************
 *
 *	D Y N _ p u t _ t e x t _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Write out a text blob.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();

	const UCHAR* p = *ptr;
	USHORT length = *p++;
	length |= (*p++) << 8;

	if (!length)
	{
		*ptr = p;
		return length;
	}

	// make the code die at some place if DYN_error_punt doesn't jump far away.
	const UCHAR* end = NULL;

	try {
		UCharBuffer bpb;

		Database* dbb = tdbb->getDatabase();
		if (ENCODE_ODS(dbb->dbb_ods_version, dbb->dbb_minor_original) >= ODS_11_1)
		{
			const size_t convSize = 15;
			bpb.resize(convSize);

			UCHAR* p = bpb.begin();
			*p++ = isc_bpb_version1;

			*p++ = isc_bpb_source_type;
			*p++ = 2;
			put_vax_short(p, isc_blob_text);
			p += 2;
			*p++ = isc_bpb_source_interp;
			*p++ = 1;
			*p++ = tdbb->getAttachment()->att_charset;

			*p++ = isc_bpb_target_type;
			*p++ = 2;
			put_vax_short(p, isc_blob_text);
			p += 2;
			*p++ = isc_bpb_target_interp;
			*p++ = 1;
			*p++ = CS_METADATA;
			fb_assert(size_t(p - bpb.begin()) <= convSize);

			// set the array count to the number of bytes we used
			bpb.shrink(p - bpb.begin());
		}

		blb* blob = BLB_create2(tdbb, gbl->gbl_transaction, blob_id, bpb.getCount(), bpb.begin());

		for (end = p + length; p < end; p += TEXT_BLOB_LENGTH)
		{
			length = (p + TEXT_BLOB_LENGTH <= end) ? TEXT_BLOB_LENGTH : end - p;
			BLB_put_segment(tdbb, blob, p, length);
		}
		BLB_close(tdbb, blob);
	}
	catch (const Exception& ex)
	{
		stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_error_punt(true, 106);
		// msg 106: "Create metadata blob failed"
	}

	*ptr = end;

	return length;
}

void DYN_rundown_request(jrd_req* handle, SSHORT id)
{
/**************************************
 *
 *	D Y N _ r u n d o w n _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Unwind a request and save it
 *	in the dyn internal request list.
 *
 **************************************/

	thread_db* tdbb = JRD_get_thread_data();
	Database* dbb  = tdbb->getDatabase();

	if (!handle) {
		return;
	}

	EXE_unwind(tdbb, handle);
	if (id >= 0 && !DYN_REQUEST(id)) {
		DYN_REQUEST(id) = handle;
	}
}


USHORT DYN_skip_attribute(const UCHAR** ptr)
{
/**************************************
 *
 *	D Y N _ s k i p _ a t t r i b u t e
 *
 **************************************
 *
 * Functional description
 *	Skip over attribute returning length (excluding
 *	size of count bytes).
 *
 **************************************/
	const UCHAR* p = *ptr;
	USHORT length = *p++;
	length |= (*p++) << 8;
	*ptr = p + length;

	return length;
}


static void grant( Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_133;	// gds__utility 
   } jrd_132;
   struct {
          SSHORT jrd_131;	// gds__utility 
   } jrd_130;
   struct {
          SSHORT jrd_127;	// gds__utility 
          SSHORT jrd_128;	// gds__null_flag 
          SSHORT jrd_129;	// RDB$GRANT_OPTION 
   } jrd_126;
   struct {
          TEXT  jrd_120 [32];	// RDB$GRANTOR 
          TEXT  jrd_121 [32];	// RDB$USER 
          TEXT  jrd_122 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_123;	// RDB$USER_TYPE 
          SSHORT jrd_124;	// RDB$OBJECT_TYPE 
          TEXT  jrd_125 [7];	// RDB$PRIVILEGE 
   } jrd_119;
   struct {
          SSHORT jrd_150;	// gds__utility 
   } jrd_149;
   struct {
          SSHORT jrd_148;	// gds__utility 
   } jrd_147;
   struct {
          SSHORT jrd_144;	// gds__utility 
          SSHORT jrd_145;	// gds__null_flag 
          SSHORT jrd_146;	// RDB$GRANT_OPTION 
   } jrd_143;
   struct {
          TEXT  jrd_136 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_137 [32];	// RDB$GRANTOR 
          TEXT  jrd_138 [32];	// RDB$USER 
          TEXT  jrd_139 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_140;	// RDB$USER_TYPE 
          SSHORT jrd_141;	// RDB$OBJECT_TYPE 
          TEXT  jrd_142 [7];	// RDB$PRIVILEGE 
   } jrd_135;
/**************************************
 *
 *	g r a n t
 *
 **************************************
 *
 * Functional description
 *	Execute SQL grant operation.
 *
 **************************************/

	SSHORT id;
	char privileges[16];
	MetaName object;
	MetaName field;
	MetaName user;
	TEXT priv[2];
	bool grant_role_stmt = false;

	thread_db* tdbb = JRD_get_thread_data();
	Database* const dbb = tdbb->getDatabase();
	MetaName grantor = tdbb->getAttachment()->att_user->usr_user_name;

	const USHORT major_version  = dbb->dbb_ods_version;
	const USHORT minor_original = dbb->dbb_minor_original;

	GET_STRING(ptr, privileges);
	if (!strcmp(privileges, "A")) {
		strcpy(privileges, ALL_PRIVILEGES);
	}

	int    options   = 0;
	SSHORT user_type = -1;
	SSHORT obj_type  = -1;
    MetaName dummy_name;

	UCHAR verb;
	while ((verb = *(*ptr)++) != isc_dyn_end)
	{
		switch (verb)
		{
		case isc_dyn_rel_name:
			obj_type = obj_relation;
			GET_STRING(ptr, object);
			break;

		case isc_dyn_prc_name:
			obj_type = obj_procedure;
			GET_STRING(ptr, object);
			break;

		case isc_dyn_fld_name:
			GET_STRING(ptr, field);
			break;

		case isc_dyn_grant_user_group:
			user_type = obj_user_group;
			GET_STRING(ptr, user);
			break;

		case isc_dyn_grant_user:
			{
				GET_STRING(ptr, user);
				// This test may become obsolete as we now allow explicit ROLE keyword.
				if (DYN_is_it_sql_role(gbl, user, dummy_name, tdbb))
				{
					user_type = obj_sql_role;
					if (user == NULL_ROLE)
					{
						DYN_error_punt(false, 195, user.c_str());
						// msg 195: keyword NONE could not be used as SQL role name.
					}
				}
				else
				{
					user_type = obj_user;
					user.upper7();
				}
			}
			break;

		case isc_dyn_grant_user_explicit:
		    GET_STRING(ptr, user);
			user_type = obj_user;
			user.upper7();
		    break;

		case isc_dyn_grant_role:
		    user_type = obj_sql_role;
		    GET_STRING(ptr, user);
			if (!DYN_is_it_sql_role(gbl, user, dummy_name, tdbb))
			{
				DYN_error_punt(false, 188, user.c_str());
				// msg 188: Role doesn't exist.
			}
			if (user == NULL_ROLE)
			{
				DYN_error_punt(false, 195, user.c_str());
				// msg 195: keyword NONE could not be used as SQL role name.
			}
			break;

		case isc_dyn_sql_role_name:	// role name in role_name_list
			if (ENCODE_ODS(major_version, minor_original) < ODS_9_0) {
                DYN_error_punt(false, 196);
			}
			else
			{
				obj_type = obj_sql_role;
				GET_STRING(ptr, object);
				grant_role_stmt = true;
				if (object == NULL_ROLE)
				{
					DYN_error_punt(false, 195, object.c_str());
					// msg 195: keyword NONE could not be used as SQL role name.
				}
			}
			break;

		case isc_dyn_grant_proc:
			user_type = obj_procedure;
			GET_STRING(ptr, user);
			break;

		case isc_dyn_grant_trig:
			user_type = obj_trigger;
			GET_STRING(ptr, user);
			break;

		case isc_dyn_grant_view:
			user_type = obj_view;
			GET_STRING(ptr, user);
			break;

		case isc_dyn_grant_options:
		case isc_dyn_grant_admin_options:
			options = DYN_get_number(ptr);
			break;

		case isc_dyn_grant_grantor:
			if (! tdbb->getAttachment()->locksmith())
			{
				DYN_error_punt(false, 252, SYSDBA_USER_NAME);
			}
			GET_STRING(ptr, grantor);
			break;

		default:
			DYN_unsupported_verb();
		}
	}
	grantor.upper7();

	jrd_req* request = NULL;

	try {

	request = CMP_find_request(tdbb,
		(USHORT) (field.length() > 0 ? drq_l_grant1 : drq_l_grant2), DYN_REQUESTS);
	for (const TEXT* pr = privileges; *pr; pr++)
	{
		bool duplicate = false;
		priv[0] = *pr;
		priv[1] = 0;
		if (field.length() > 0)
		{
			id = drq_l_grant1;
			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				PRIV IN RDB$USER_PRIVILEGES WITH
					PRIV.RDB$RELATION_NAME EQ object.c_str() AND
					PRIV.RDB$OBJECT_TYPE = obj_type AND
					PRIV.RDB$PRIVILEGE EQ priv AND
					PRIV.RDB$USER = user.c_str() AND
					PRIV.RDB$USER_TYPE = user_type AND
					PRIV.RDB$GRANTOR EQ grantor.c_str() AND
					PRIV.RDB$FIELD_NAME EQ field.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_134, sizeof(jrd_134), true);
			gds__vtov ((const char*) field.c_str(), (char*) jrd_135.jrd_136, 32);
			gds__vtov ((const char*) grantor.c_str(), (char*) jrd_135.jrd_137, 32);
			gds__vtov ((const char*) user.c_str(), (char*) jrd_135.jrd_138, 32);
			gds__vtov ((const char*) object.c_str(), (char*) jrd_135.jrd_139, 32);
			jrd_135.jrd_140 = user_type;
			jrd_135.jrd_141 = obj_type;
			gds__vtov ((const char*) priv, (char*) jrd_135.jrd_142, 7);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 139, (UCHAR*) &jrd_135);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_143);
			   if (!jrd_143.jrd_144) break;
				if (!DYN_REQUEST(drq_l_grant1)) {
					DYN_REQUEST(drq_l_grant1) = request;
				}

				if ((/*PRIV.RDB$GRANT_OPTION.NULL*/
				     jrd_143.jrd_145) ||
					(/*PRIV.RDB$GRANT_OPTION*/
					 jrd_143.jrd_146) ||
					(/*PRIV.RDB$GRANT_OPTION*/
					 jrd_143.jrd_146 == options))
				{
					duplicate = true;
				}
				else
				{
					/*ERASE PRIV;*/
					EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_147);	// has to be 0 and options == 1
				}
			/*END_FOR;*/
			   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_149);
			   }
			}
			if (!DYN_REQUEST(drq_l_grant1))
			{
				DYN_REQUEST(drq_l_grant1) = request;
			}
		}
		else
		{
			id = drq_l_grant2;
			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				PRIV IN RDB$USER_PRIVILEGES WITH
					PRIV.RDB$RELATION_NAME EQ object.c_str() AND
					PRIV.RDB$OBJECT_TYPE = obj_type AND
					PRIV.RDB$PRIVILEGE EQ priv AND
					PRIV.RDB$USER = user.c_str() AND
					PRIV.RDB$USER_TYPE = user_type AND
					PRIV.RDB$GRANTOR EQ grantor.c_str() AND
					PRIV.RDB$FIELD_NAME MISSING*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_118, sizeof(jrd_118), true);
			gds__vtov ((const char*) grantor.c_str(), (char*) jrd_119.jrd_120, 32);
			gds__vtov ((const char*) user.c_str(), (char*) jrd_119.jrd_121, 32);
			gds__vtov ((const char*) object.c_str(), (char*) jrd_119.jrd_122, 32);
			jrd_119.jrd_123 = user_type;
			jrd_119.jrd_124 = obj_type;
			gds__vtov ((const char*) priv, (char*) jrd_119.jrd_125, 7);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 107, (UCHAR*) &jrd_119);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_126);
			   if (!jrd_126.jrd_127) break;
				if (!DYN_REQUEST(drq_l_grant2))
					DYN_REQUEST(drq_l_grant2) = request;

				if ((/*PRIV.RDB$GRANT_OPTION.NULL*/
				     jrd_126.jrd_128) ||
					(/*PRIV.RDB$GRANT_OPTION*/
					 jrd_126.jrd_129) ||
					(/*PRIV.RDB$GRANT_OPTION*/
					 jrd_126.jrd_129 == options))
				{
					duplicate = true;
				}
				else
				{
					/*ERASE PRIV;*/
					EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_130);	// has to be 0 and options == 1
				}
			/*END_FOR;*/
			   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_132);
			   }
			}
			if (!DYN_REQUEST(drq_l_grant2))
			{
				DYN_REQUEST(drq_l_grant2) = request;
			}
		}

		if (duplicate) {
			continue;
		}

		if (grant_role_stmt)
		{
			if (ENCODE_ODS(major_version, minor_original) < ODS_9_0)
			{
	            DYN_error_punt(false, 196);
			}
			id = drq_get_role_nm;

			if (!grantor_can_grant_role(tdbb, gbl, grantor, object))
			{
				// ERR_punt();
				goto do_punt;
			}

			if (user_type == obj_sql_role)
			{
				/****************************************************
	            **
	            ** temporary restriction. This should be removed once
	            ** GRANT role1 TO rolex is supported and this message
	            ** could be reused for blocking cycles of role grants
	            **
	            *****************************************************/
	            DYN_error_punt(false, 192, user.c_str());
			}
		}
		else
		{
			/* In the case where the object is a view, then the grantor must have
			   some kind of grant privileges on the base table(s)/view(s).  If the
			   grantor is the owner of the view, then we have to explicitely check
			   this because the owner of a view by default has grant privileges on
			   his own view.  If the grantor is not the owner of the view, then the
			   base table/view grant privilege checks were made when the grantor
			   got its grant privilege on the view and no further checks are
			   necessary.
			   As long as only locksmith can use GRANTED BY, no need specially checking
			   for privileges of current user. AP-2008 */

			if (!obj_type)
			{
				// relation or view because we cannot distinguish at this point.
				id = drq_gcg1;
				if (!grantor_can_grant(gbl,
										tdbb->getAttachment()->att_user->usr_user_name.c_str(),
										priv,
										object,
										field,
										true))
				{
					// grantor_can_grant has moved the error string already.
					// just punt back to the setjump
					// ERR_punt();
					goto do_punt;
				}
			}
		}

		id = drq_s_grant;
		store_privilege(gbl, object, user, field, pr, user_type, obj_type, options, grantor);
	}	// for (...)

	}	// try
	catch (const Exception& ex)
	{
		stuff_exception(tdbb->tdbb_status_vector, ex);
		// we need to rundown as we have to set the env.
		// But in case the error is from store_priveledge we have already
		// unwound the request so passing that as null
		jrd_req* req1 = (id == drq_s_grant || id == drq_gcg1) ? NULL : request;
		DYN_rundown_request(req1, -1);

		switch (id)
		{
		case drq_l_grant1:
			DYN_error_punt(true, 77);
			// msg 77: "SELECT RDB$USER_PRIVILEGES failed in grant"
			break;
		case drq_l_grant2:
			DYN_error_punt(true, 78);
			// msg 78: "SELECT RDB$USER_PRIVILEGES failed in grant"
			break;
		case drq_s_grant:
		case drq_gcg1:
			ERR_punt();
			// store_priviledge || grantor_can_grant error already handled,
			// just bail out
			break;
		case drq_get_role_nm:
			ERR_punt();
			break;
		default:
			fb_assert(id == drq_gcg1);
			DYN_error_punt(true, 78);
			// msg 78: "SELECT RDB$USER_PRIVILEGES failed in grant"
			break;
		}
	}

	return;

do_punt:	// ugly, rethink logic of this function
	ERR_punt();
}


static bool grantor_can_grant(Global* gbl,
							  const TEXT* grantor,
							  const TEXT* privilege,
							  const MetaName& relation_name,
							  const MetaName& field_name,
							  bool top_level)
{
   struct {
          TEXT  jrd_82 [32];	// RDB$BASE_FIELD 
          TEXT  jrd_83 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_84 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_85;	// gds__utility 
   } jrd_81;
   struct {
          TEXT  jrd_80 [32];	// RDB$RELATION_NAME 
   } jrd_79;
   struct {
          TEXT  jrd_94 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_95;	// gds__utility 
          SSHORT jrd_96;	// gds__null_flag 
          SSHORT jrd_97;	// RDB$GRANT_OPTION 
          SSHORT jrd_98;	// gds__null_flag 
   } jrd_93;
   struct {
          TEXT  jrd_88 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_89 [32];	// RDB$USER 
          SSHORT jrd_90;	// RDB$OBJECT_TYPE 
          SSHORT jrd_91;	// RDB$USER_TYPE 
          TEXT  jrd_92 [7];	// RDB$PRIVILEGE 
   } jrd_87;
   struct {
          SSHORT jrd_104;	// gds__utility 
   } jrd_103;
   struct {
          TEXT  jrd_101 [32];	// RDB$OWNER_NAME 
          TEXT  jrd_102 [32];	// RDB$RELATION_NAME 
   } jrd_100;
   struct {
          SSHORT jrd_110;	// gds__utility 
   } jrd_109;
   struct {
          TEXT  jrd_107 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_108 [32];	// RDB$RELATION_NAME 
   } jrd_106;
   struct {
          SSHORT jrd_115;	// gds__utility 
          SSHORT jrd_116;	// gds__null_flag 
          SSHORT jrd_117;	// RDB$FLAGS 
   } jrd_114;
   struct {
          TEXT  jrd_113 [32];	// RDB$RELATION_NAME 
   } jrd_112;
/**************************************
 *
 *	g r a n t o r _ c a n _ g r a n t
 *
 **************************************
 *
 * Functional description
 *	return: true is the grantor has grant privilege on the relation/field.
 *		false otherwise.
 *
 **************************************/
	USHORT err_num;

	thread_db* tdbb = JRD_get_thread_data();
	Database* const dbb = tdbb->getDatabase();

	// Verify that the input relation exists.

	jrd_req* request = CMP_find_request(tdbb, drq_gcg4, DYN_REQUESTS);


	try {

	err_num = 182;				// for the longjump
	bool sql_relation = false;
	bool relation_exists = false;
	// SELECT RDB$RELATIONS failed in grant
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		REL IN RDB$RELATIONS WITH
			REL.RDB$RELATION_NAME = relation_name.c_str()*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_111, sizeof(jrd_111), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_112.jrd_113, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_112);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 6, (UCHAR*) &jrd_114);
	   if (!jrd_114.jrd_115) break;
		relation_exists = true;
		if ((!/*REL.RDB$FLAGS.NULL*/
		      jrd_114.jrd_116) && (/*REL.RDB$FLAGS*/
      jrd_114.jrd_117 & REL_sql))
			sql_relation = true;
		if (!DYN_REQUEST(drq_gcg4))
			DYN_REQUEST(drq_gcg4) = request;
	/*END_FOR;*/
	   }
	}
	if (!DYN_REQUEST(drq_gcg4))
		DYN_REQUEST(drq_gcg4) = request;
	if (!relation_exists)
	{
		DYN_error(false, 175, SafeArg() << relation_name.c_str());
		// table/view .. does not exist
		return false;
	}

	// Verify the the input field exists.

	if (field_name.length() > 0)
	{
		err_num = 183;
		bool field_exists = false;

		// SELECT RDB$RELATION_FIELDS failed in grant
		request = CMP_find_request(tdbb, drq_gcg5, DYN_REQUESTS);
		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			G_FLD IN RDB$RELATION_FIELDS WITH
				G_FLD.RDB$RELATION_NAME = relation_name.c_str() AND
				G_FLD.RDB$FIELD_NAME = field_name.c_str()*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_105, sizeof(jrd_105), true);
		gds__vtov ((const char*) field_name.c_str(), (char*) jrd_106.jrd_107, 32);
		gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_106.jrd_108, 32);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_106);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_109);
		   if (!jrd_109.jrd_110) break;
			if (!DYN_REQUEST(drq_gcg5))
				DYN_REQUEST(drq_gcg5) = request;
			field_exists = true;
		/*END_FOR;*/
		   }
		}
		if (!DYN_REQUEST(drq_gcg5))
			DYN_REQUEST(drq_gcg5) = request;
		if (!field_exists)
		{
			DYN_error(false, 176, SafeArg() << field_name.c_str() << relation_name.c_str());
			// column .. does not exist in table/view ..
			return false;
		}
	}

	// If the current user is locksmith - allow all grants to occur

	if (tdbb->getAttachment()->locksmith()) {
		return true;
	}

	// If this is a non-sql table, then the owner will probably not have any
	// entries in the rdb$user_privileges table.  Give the owner of a GDML
	// table all privileges.
	err_num = 184;
	bool grantor_is_owner = false;
	// SELECT RDB$RELATIONS/RDB$OWNER_NAME failed in grant

	request = CMP_find_request(tdbb, drq_gcg2, DYN_REQUESTS);
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		REL IN RDB$RELATIONS WITH
			REL.RDB$RELATION_NAME = relation_name.c_str() AND
			REL.RDB$OWNER_NAME = UPPERCASE(grantor)*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_99, sizeof(jrd_99), true);
	gds__vtov ((const char*) grantor, (char*) jrd_100.jrd_101, 32);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_100.jrd_102, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_100);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 2, (UCHAR*) &jrd_103);
	   if (!jrd_103.jrd_104) break;
		if (!DYN_REQUEST(drq_gcg2))
			DYN_REQUEST(drq_gcg2) = request;
		grantor_is_owner = true;
	/*END_FOR;*/
	   }
	}
	if (!DYN_REQUEST(drq_gcg2))
		DYN_REQUEST(drq_gcg2) = request;
	if (!sql_relation && grantor_is_owner) {
		return true;
	}

	// Remember the grant option for non field-specific user-privileges, and
	// the grant option for the user-privileges for the input field.
	// -1 = no privilege found (yet)
	// 0 = privilege without grant option found
	// 1 = privilege with grant option found
	SSHORT go_rel = -1;
	SSHORT go_fld = -1;


	// Verify that the grantor has the grant option for this relation/field
	// in the rdb$user_privileges.  If not, then we don't need to look further.

	err_num = 185;
	// SELECT RDB$USER_PRIVILEGES failed in grant

	request = CMP_find_request(tdbb, drq_gcg1, DYN_REQUESTS);
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		PRV IN RDB$USER_PRIVILEGES WITH
			PRV.RDB$USER = UPPERCASE(grantor) AND
			PRV.RDB$USER_TYPE = obj_user AND
			PRV.RDB$RELATION_NAME = relation_name.c_str() AND
			PRV.RDB$OBJECT_TYPE = obj_relation AND
			PRV.RDB$PRIVILEGE = privilege*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_86, sizeof(jrd_86), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_87.jrd_88, 32);
	gds__vtov ((const char*) grantor, (char*) jrd_87.jrd_89, 32);
	jrd_87.jrd_90 = obj_relation;
	jrd_87.jrd_91 = obj_user;
	gds__vtov ((const char*) privilege, (char*) jrd_87.jrd_92, 7);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 75, (UCHAR*) &jrd_87);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 40, (UCHAR*) &jrd_93);
	   if (!jrd_93.jrd_95) break;
		if (!DYN_REQUEST(drq_gcg1))
			DYN_REQUEST(drq_gcg1) = request;

		if (/*PRV.RDB$FIELD_NAME.NULL*/
		    jrd_93.jrd_98)
		{
			if (/*PRV.RDB$GRANT_OPTION.NULL*/
			    jrd_93.jrd_96 || !/*PRV.RDB$GRANT_OPTION*/
     jrd_93.jrd_97)
				go_rel = 0;
			else if (go_rel)
				go_rel = 1;
		}
		else
		{
			if (/*PRV.RDB$GRANT_OPTION.NULL*/
			    jrd_93.jrd_96 || !/*PRV.RDB$GRANT_OPTION*/
     jrd_93.jrd_97)
			{
				if (field_name.length() && field_name == /*PRV.RDB$FIELD_NAME*/
									 jrd_93.jrd_94)
					go_fld = 0;
			}
			else
			{
				if (field_name.length() && field_name == /*PRV.RDB$FIELD_NAME*/
									 jrd_93.jrd_94)
					go_fld = 1;
			}
		}
	/*END_FOR;*/
	   }
	}
	if (!DYN_REQUEST(drq_gcg1))
	{
		DYN_REQUEST(drq_gcg1) = request;
	}

	if (field_name.length())
	{
		if (go_fld == 0)
		{
			DYN_error(false,
					(USHORT)(top_level ? 167 : 168),
					SafeArg() << privilege << field_name.c_str() << relation_name.c_str());
			// no grant option for privilege .. on column .. of [base] table/view ..
			return false;
		}

		if (go_fld == -1)
		{
			if (go_rel == 0)
			{
				DYN_error(false,
						(USHORT)(top_level ? 169 : 170),
						SafeArg() << privilege << relation_name.c_str() << field_name.c_str());
				// no grant option for privilege .. on [base] table/view .. (for column ..)
				return false;
			}

			if (go_rel == -1)
			{
				DYN_error(false,
						(USHORT)(top_level ? 171 : 172),
						SafeArg() << privilege << relation_name.c_str() << field_name.c_str());
				// no .. privilege with grant option on [base] table/view .. (for column ..)
				return false;
			}
		}
	}
	else
	{
		if (go_rel == 0)
		{
			DYN_error(false, 173, SafeArg() << privilege << relation_name.c_str());
			// no grant option for privilege .. on table/view ..
			return false;
		}

		if (go_rel == -1)
		{
			DYN_error(false, 174, SafeArg() << privilege << relation_name.c_str());
			// no .. privilege with grant option on table/view ..
			return false;
		}
	}

	// If the grantor is not the owner of the relation, then we don't need to
	// check the base table(s)/view(s) because that check was performed when
	// the grantor was given its privileges.

	if (!grantor_is_owner) {
		return true;
	}

	// Find all the base fields/relations and check for the correct grant privileges on them.

	err_num = 186;
	// SELECT RDB$VIEW_RELATIONS/RDB$RELATION_FIELDS/... failed in grant

	request = CMP_find_request(tdbb, drq_gcg3, DYN_REQUESTS);
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		G_FLD IN RDB$RELATION_FIELDS CROSS
			G_VIEW IN RDB$VIEW_RELATIONS WITH
			G_FLD.RDB$RELATION_NAME = relation_name.c_str() AND
			G_FLD.RDB$BASE_FIELD NOT MISSING AND
			G_VIEW.RDB$VIEW_NAME EQ G_FLD.RDB$RELATION_NAME AND
			G_VIEW.RDB$VIEW_CONTEXT EQ G_FLD.RDB$VIEW_CONTEXT*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_78, sizeof(jrd_78), true);
	gds__vtov ((const char*) relation_name.c_str(), (char*) jrd_79.jrd_80, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 32, (UCHAR*) &jrd_79);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 98, (UCHAR*) &jrd_81);
	   if (!jrd_81.jrd_85) break;
		if (!DYN_REQUEST(drq_gcg3)) {
			DYN_REQUEST(drq_gcg3) = request;
		}

		if (field_name.length())
		{
			if (field_name == /*G_FLD.RDB$FIELD_NAME*/
					  jrd_81.jrd_84)
				if (!grantor_can_grant(gbl, grantor, privilege, /*G_VIEW.RDB$RELATION_NAME*/
										jrd_81.jrd_83,
										/*G_FLD.RDB$BASE_FIELD*/
										jrd_81.jrd_82, false))
				{
					return false;
				}
		}
		else
		{
			if (!grantor_can_grant(gbl, grantor, privilege, /*G_VIEW.RDB$RELATION_NAME*/
									jrd_81.jrd_83,
									/*G_FLD.RDB$BASE_FIELD*/
									jrd_81.jrd_82, false))
			{
				return false;
			}
		}
	/*END_FOR;*/
	   }
	}
	if (!DYN_REQUEST(drq_gcg3))
		DYN_REQUEST(drq_gcg3) = request;

	// all done.

	}	// try
	catch (const Exception& ex)
	{
		stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, err_num);
		// msg 77: "SELECT RDB$USER_PRIVILEGES failed in grant"

		return false;
	}

	return true;
}


static bool grantor_can_grant_role(thread_db* tdbb,
								   Global* gbl,
								   const MetaName& grantor,
								   const MetaName& role_name)
{
   struct {
          SSHORT jrd_76;	// gds__utility 
          SSHORT jrd_77;	// RDB$GRANT_OPTION 
   } jrd_75;
   struct {
          TEXT  jrd_71 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_72 [32];	// RDB$USER 
          SSHORT jrd_73;	// RDB$OBJECT_TYPE 
          SSHORT jrd_74;	// RDB$USER_TYPE 
   } jrd_70;
/**************************************
 *
 *	g r a n t o r _ c a n _ g r a n t _ r o l e
 *
 **************************************
 *
 * Functional description
 *
 *	return: true if the grantor has admin privilege on the role.
 *		false otherwise.
 *
 **************************************/

	bool grantable = false;
	bool no_admin = false;

	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	// Fetch the name of the owner of the ROLE
	MetaName owner;
	if (DYN_is_it_sql_role(gbl, role_name, owner, tdbb))
	{
		// Both SYSDBA and the owner of this ROLE can grant membership
		if (tdbb->getAttachment()->locksmith() || owner == grantor)
			return true;
	}
	else
	{
		// role name not exist.
		DYN_error(false, 188, SafeArg() << role_name.c_str());
		return false;
	}

	jrd_req* request = CMP_find_request(tdbb, drq_get_role_au, DYN_REQUESTS);

	// The 'grantor' is not the owner of the ROLE, see if they
	// have admin privilege on the role
	/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		PRV IN RDB$USER_PRIVILEGES WITH
			PRV.RDB$USER = UPPERCASE(grantor.c_str()) AND
			PRV.RDB$USER_TYPE = obj_user AND
			PRV.RDB$RELATION_NAME EQ role_name.c_str() AND
			PRV.RDB$OBJECT_TYPE = obj_sql_role AND
			PRV.RDB$PRIVILEGE EQ "M"*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_69, sizeof(jrd_69), true);
	gds__vtov ((const char*) role_name.c_str(), (char*) jrd_70.jrd_71, 32);
	gds__vtov ((const char*) grantor.c_str(), (char*) jrd_70.jrd_72, 32);
	jrd_70.jrd_73 = obj_sql_role;
	jrd_70.jrd_74 = obj_user;
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 68, (UCHAR*) &jrd_70);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 4, (UCHAR*) &jrd_75);
	   if (!jrd_75.jrd_76) break;
		if (!DYN_REQUEST(drq_get_role_au)) {
			DYN_REQUEST(drq_get_role_au) = request;
		}

		if (/*PRV.RDB$GRANT_OPTION*/
		    jrd_75.jrd_77 == 2)
			grantable = true;
		else
			no_admin = true;

	/*END_FOR;*/
	   }
	}

	if (!DYN_REQUEST(drq_get_role_au))
	{
		DYN_REQUEST(drq_get_role_au) = request;
	}

	if (!grantable)
	{
		// 189: user have no admin option.
		// 190: user is not a member of the role.
		DYN_error(false, no_admin ? 189 : 190, SafeArg() << grantor.c_str() << role_name.c_str());
		return false;
	}

	return true;
}


static const char* privilege_name(char symbol)
{
/**************************************
 *
 *	p r i v i l e g e _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Diagnostics print helper.
 *
 **************************************/
	switch (UPPER7(symbol))
	{
		case 'A': return "All";
		case 'I': return "Insert";
		case 'U': return "Update";
		case 'D': return "Delete";
		case 'S': return "Select";
		case 'X': return "Execute";
		case 'M': return "Role";
		case 'R': return "Reference";
	}
	return "<Unknown>";
}


static void revoke_permission(Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_53;	// gds__utility 
   } jrd_52;
   struct {
          SSHORT jrd_51;	// gds__utility 
   } jrd_50;
   struct {
          TEXT  jrd_48 [32];	// RDB$GRANTOR 
          SSHORT jrd_49;	// gds__utility 
   } jrd_47;
   struct {
          TEXT  jrd_42 [32];	// RDB$USER 
          TEXT  jrd_43 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_44;	// RDB$USER_TYPE 
          SSHORT jrd_45;	// RDB$OBJECT_TYPE 
          TEXT  jrd_46 [7];	// RDB$PRIVILEGE 
   } jrd_41;
   struct {
          SSHORT jrd_68;	// gds__utility 
   } jrd_67;
   struct {
          SSHORT jrd_66;	// gds__utility 
   } jrd_65;
   struct {
          TEXT  jrd_63 [32];	// RDB$GRANTOR 
          SSHORT jrd_64;	// gds__utility 
   } jrd_62;
   struct {
          TEXT  jrd_56 [32];	// RDB$FIELD_NAME 
          TEXT  jrd_57 [32];	// RDB$USER 
          TEXT  jrd_58 [32];	// RDB$RELATION_NAME 
          SSHORT jrd_59;	// RDB$USER_TYPE 
          SSHORT jrd_60;	// RDB$OBJECT_TYPE 
          TEXT  jrd_61 [7];	// RDB$PRIVILEGE 
   } jrd_55;
/**************************************
 *
 *	r e v o k e _ p e r m i s s i o n
 *
 **************************************
 *
 * Functional description
 *	Revoke some privileges.
 *
 *	Note: According to SQL II, section 11.37, pg 280,
 *	general rules 8 & 9,
 *	if the specified priviledge for the specified user
 *	does not exist, it is not an exception error condition
 *	but a completion condition.  Since V4.0 does not support
 *	completion conditions (warnings) we do not return
 *	any indication that the revoke operation didn't do
 *	anything.
 *	1994-August-2 Rich Damon & David Schnepper
 *
 **************************************/
	UCHAR verb;
	char privileges[16];
	MetaName object, field, user;
	MetaName dummy_name;

	thread_db* tdbb = JRD_get_thread_data();
	Database* const dbb = tdbb->getDatabase();

	const USHORT major_version = dbb->dbb_ods_version;
	const USHORT minor_original = dbb->dbb_minor_original;

	// Stash away a copy of the revoker's name, in uppercase form

	const UserId* revoking_user = tdbb->getAttachment()->att_user;
	MetaName revoking_as_user_name(revoking_user->usr_user_name);
	revoking_as_user_name.upper7();

	GET_STRING(ptr, privileges);
	if (!strcmp(privileges, "A")) {
		strcpy(privileges, ALL_PRIVILEGES);
	}

	int options = 0;
	SSHORT user_type = -1;
	SSHORT obj_type = -1;

	while ((verb = *(*ptr)++) != isc_dyn_end)
	{
		switch (verb)
		{
		case isc_dyn_rel_name:
			obj_type = obj_relation;
			GET_STRING(ptr, object);
			break;

		case isc_dyn_prc_name:
			obj_type = obj_procedure;
			GET_STRING(ptr, object);
			break;

		case isc_dyn_fld_name:
			GET_STRING(ptr, field);
			break;

		case isc_dyn_grant_user_group:
			user_type = obj_user_group;
			GET_STRING(ptr, user);
			break;

		case isc_dyn_grant_user:
			GET_STRING(ptr, user);
			// This test may become obsolete as we now allow explicit ROLE keyword.
			if (DYN_is_it_sql_role(gbl, user, dummy_name, tdbb))
			{
				user_type = obj_sql_role;
				if (user == NULL_ROLE)
				{
					DYN_error_punt(false, 195, user.c_str());
					// msg 195: keyword NONE could not be used as SQL role name.
				}
			}
			else
			{
				user_type = obj_user;
				user.upper7();
			}
			break;

		case isc_dyn_grant_user_explicit:
		    GET_STRING(ptr, user);
	        user_type = obj_user;
			user.upper7();
			break;

		case isc_dyn_grant_role:
			user_type = obj_sql_role;
			GET_STRING(ptr, user);
			if (!DYN_is_it_sql_role(gbl, user, dummy_name, tdbb))
			{
				DYN_error_punt(false, 188, user.c_str());
				// msg 188: Role doesn't exist.
			}
			if (user == NULL_ROLE)
			{
				DYN_error_punt(false, 195, user.c_str());
				// msg 195: keyword NONE could not be used as SQL role name.
			}
		    break;

		case isc_dyn_sql_role_name:	// role name in role_name_list
			if (ENCODE_ODS(major_version, minor_original) < ODS_9_0) {
                DYN_error_punt(false, 196);
			}
			else
			{
				obj_type = obj_sql_role;
				GET_STRING(ptr, object);
/*
				CVC: Make this a warning in the future.
				if (object == NULL_ROLE)
					DYN_error_punt(false, 195, object.c_str());
*/
				// msg 195: keyword NONE could not be used as SQL role name.
			}
			break;

		case isc_dyn_grant_proc:
			user_type = obj_procedure;
			GET_STRING(ptr, user);
			break;

		case isc_dyn_grant_trig:
			user_type = obj_trigger;
			GET_STRING(ptr, user);
			break;

		case isc_dyn_grant_view:
			user_type = obj_view;
			GET_STRING(ptr, user);
			break;

		case isc_dyn_grant_options:
		case isc_dyn_grant_admin_options:
			options = DYN_get_number(ptr);
			break;

		case isc_dyn_grant_grantor:
			if (! tdbb->getAttachment()->locksmith())
			{
				DYN_error_punt(false, 252, SYSDBA_USER_NAME);
			}
			GET_STRING(ptr, revoking_as_user_name);
			break;

		default:
			DYN_unsupported_verb();
		}
	}
	revoking_as_user_name.upper7();


	USHORT id = field.length() ? drq_e_grant1 : drq_e_grant2;

	jrd_req* request = CMP_find_request(tdbb, id, DYN_REQUESTS);

	try {

    TEXT temp[2];
	temp[1] = 0;
	for (const TEXT* pr = privileges; (temp[0] = *pr); pr++)
	{
		bool grant_erased = false;
		bool bad_grantor = false;

		if (field.length())
		{
			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				PRIV IN RDB$USER_PRIVILEGES WITH
					PRIV.RDB$PRIVILEGE EQ temp AND
					PRIV.RDB$RELATION_NAME EQ object.c_str() AND
					PRIV.RDB$OBJECT_TYPE = obj_type AND
					PRIV.RDB$USER = user.c_str() AND
					PRIV.RDB$USER_TYPE = user_type AND
					PRIV.RDB$FIELD_NAME EQ field.c_str()*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_54, sizeof(jrd_54), true);
			gds__vtov ((const char*) field.c_str(), (char*) jrd_55.jrd_56, 32);
			gds__vtov ((const char*) user.c_str(), (char*) jrd_55.jrd_57, 32);
			gds__vtov ((const char*) object.c_str(), (char*) jrd_55.jrd_58, 32);
			jrd_55.jrd_59 = user_type;
			jrd_55.jrd_60 = obj_type;
			gds__vtov ((const char*) temp, (char*) jrd_55.jrd_61, 7);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 107, (UCHAR*) &jrd_55);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_62);
			   if (!jrd_62.jrd_64) break;
				if (!DYN_REQUEST(drq_e_grant1))
					DYN_REQUEST(drq_e_grant1) = request;

				if (revoking_as_user_name == /*PRIV.RDB$GRANTOR*/
							     jrd_62.jrd_63)
				{
					/*ERASE PRIV;*/
					EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_65);
					grant_erased = true;
				}
				else
				{
					bad_grantor = true;
				}
			/*END_FOR;*/
			   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_67);
			   }
			}
			if (!DYN_REQUEST(drq_e_grant1)) {
				DYN_REQUEST(drq_e_grant1) = request;
			}
		}
		else
		{
			/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
				PRIV IN RDB$USER_PRIVILEGES WITH
					PRIV.RDB$PRIVILEGE EQ temp AND
					PRIV.RDB$RELATION_NAME EQ object.c_str() AND
					PRIV.RDB$OBJECT_TYPE = obj_type AND
					PRIV.RDB$USER EQ user.c_str() AND
					PRIV.RDB$USER_TYPE = user_type*/
			{
			if (!request)
			   request = CMP_compile2 (tdbb, (UCHAR*) jrd_40, sizeof(jrd_40), true);
			gds__vtov ((const char*) user.c_str(), (char*) jrd_41.jrd_42, 32);
			gds__vtov ((const char*) object.c_str(), (char*) jrd_41.jrd_43, 32);
			jrd_41.jrd_44 = user_type;
			jrd_41.jrd_45 = obj_type;
			gds__vtov ((const char*) temp, (char*) jrd_41.jrd_46, 7);
			EXE_start (tdbb, request, gbl->gbl_transaction);
			EXE_send (tdbb, request, 0, 75, (UCHAR*) &jrd_41);
			while (1)
			   {
			   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_47);
			   if (!jrd_47.jrd_49) break;
				if (!DYN_REQUEST(drq_e_grant2))
					DYN_REQUEST(drq_e_grant2) = request;

				// revoking a permission at the table level implies
				// revoking the perm. on all columns. So for all fields
				// in this table which have been granted the privilege, we
				// erase the entries from RDB$USER_PRIVILEGES.

				if (revoking_as_user_name == /*PRIV.RDB$GRANTOR*/
							     jrd_47.jrd_48)
				{
					/*ERASE PRIV;*/
					EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_50);
					grant_erased = true;
				}
				else
				{
					bad_grantor = true;
				}
			/*END_FOR;*/
			   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_52);
			   }
			}
			if (!DYN_REQUEST(drq_e_grant2))
				DYN_REQUEST(drq_e_grant2) = request;
		}

		if (options && grant_erased)
		{
			// Add the privilege without the grant option
			// There is a modify trigger on the rdb$user_privileges
			// which disallows the table from being updated.  It would
			// have to be changed such that only the grant_option
			// field can be updated.

			const USHORT old_id = id;
			id = drq_s_grant;

			store_privilege(gbl, object, user, field, pr, user_type, obj_type,
							0, revoking_as_user_name);

			id = old_id;
		}

		if (bad_grantor && !grant_erased)
		{
			DYN_error_punt(false, 246, SafeArg() << revoking_as_user_name.c_str() <<
				privilege_name(temp[0]) << object.c_str() << user.c_str());
			// msg 246: @1 is not grantor of @2 on @3 to @4.
		}

		if (!grant_erased)
		{
			ERR_post_warning(Arg::Warning(isc_dyn_miss_priv_warning) <<
							 Arg::Str(privilege_name(temp[0])) << Arg::Str(object) << Arg::Str(user));
			// msg 247: Warning: @1 on @2 is not granted to @3.
		}
	}

	}
	catch (const Exception& ex)
	{
		stuff_exception(tdbb->tdbb_status_vector, ex);
		// we need to rundown as we have to set the env.
		// But in case the error is from store_priveledge we have already
		// unwound the request so passing that as null
		DYN_rundown_request(((id == drq_s_grant) ? NULL : request), -1);
		if (id == drq_e_grant1)
		{
			DYN_error_punt(true, 111);
			// msg 111: "ERASE RDB$USER_PRIVILEGES failed in revoke(1)"
		}
		else if (id == drq_e_grant2)
		{
			DYN_error_punt(true, 113);
			// msg 113: "ERASE RDB$USER_PRIVILEGES failed in revoke (3)"
		}
		else
		{
			ERR_punt();
			// store_priviledge error already handled, just bail out
		}
	}
}


static void revoke_all(Global* gbl, const UCHAR** ptr)
{
   struct {
          SSHORT jrd_39;	// gds__utility 
   } jrd_38;
   struct {
          SSHORT jrd_37;	// gds__utility 
   } jrd_36;
   struct {
          TEXT  jrd_34 [32];	// RDB$GRANTOR 
          SSHORT jrd_35;	// gds__utility 
   } jrd_33;
   struct {
          TEXT  jrd_31 [32];	// RDB$USER 
          SSHORT jrd_32;	// RDB$USER_TYPE 
   } jrd_30;
/**************************************
 *
 *	r e v o k e _ a l l
 *
 **************************************
 *
 * Functional description
 *	Revoke all privileges on all objects from user/role.
 *
 **************************************/
	UCHAR verb;
	MetaName user, dummy_name;
	SSHORT user_type = -1;

	thread_db* tdbb = JRD_get_thread_data();
	Database* const dbb = tdbb->getDatabase();

	while ((verb = *(*ptr)++) != isc_dyn_end)
	{
		switch (verb)
		{

		case isc_dyn_grant_user:
			GET_STRING(ptr, user);
			// This test may become obsolete as we now allow explicit ROLE keyword.
			if (DYN_is_it_sql_role(gbl, user, dummy_name, tdbb))
			{
				user_type = obj_sql_role;
				if (user == NULL_ROLE)
				{
					DYN_error_punt(false, 195, user.c_str());
					// msg 195: keyword NONE could not be used as SQL role name.
				}
			}
			else
			{
				user_type = obj_user;
				user.upper7();
			}
			break;

		case isc_dyn_grant_user_explicit:
		    GET_STRING(ptr, user);
	        user_type = obj_user;
			user.upper7();
			break;

		case isc_dyn_grant_role:
			GET_STRING(ptr, user);
			user_type = obj_sql_role;
			if (!DYN_is_it_sql_role(gbl, user, dummy_name, tdbb)) {
				DYN_error_punt(false, 188, user.c_str()); // msg 188: Role doesn't exist.
			}
			if (user == NULL_ROLE)
			{
				DYN_error_punt(false, 195, user.c_str());
				// msg 195: keyword NONE could not be used as SQL role name.
			}
		    break;

		default:
			DYN_unsupported_verb();
		}
	}

	const UserId* revoking_user = tdbb->getAttachment()->att_user;
	MetaName revoking_as_user_name(revoking_user->usr_user_name);
	revoking_as_user_name.upper7();

	bool grant_erased = false;
	bool bad_grantor = false;

	jrd_req* request = CMP_find_request(tdbb, drq_e_grant3, DYN_REQUESTS);

	try {
		/*FOR(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			PRIV IN RDB$USER_PRIVILEGES WITH
				PRIV.RDB$USER = user.c_str() AND
				PRIV.RDB$USER_TYPE = user_type*/
		{
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_29, sizeof(jrd_29), true);
		gds__vtov ((const char*) user.c_str(), (char*) jrd_30.jrd_31, 32);
		jrd_30.jrd_32 = user_type;
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 34, (UCHAR*) &jrd_30);
		while (1)
		   {
		   EXE_receive (tdbb, request, 1, 34, (UCHAR*) &jrd_33);
		   if (!jrd_33.jrd_35) break;

			if (!DYN_REQUEST(drq_e_grant3))
				DYN_REQUEST(drq_e_grant3) = request;

			if (revoking_user->locksmith() || revoking_as_user_name == /*PRIV.RDB$GRANTOR*/
										   jrd_33.jrd_34)
			{
				/*ERASE PRIV;*/
				EXE_send (tdbb, request, 2, 2, (UCHAR*) &jrd_36);
				grant_erased = true;
			}
			else
			{
				bad_grantor = true;
			}
		/*END_FOR;*/
		   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_38);
		   }
		}
		if (!DYN_REQUEST(drq_e_grant3)) {
			DYN_REQUEST(drq_e_grant3) = request;
		}

		if (!grant_erased)
		{
			const char* all = "ALL";
			if (bad_grantor)
			{
				DYN_error_punt(false, 246, SafeArg() << revoking_as_user_name.c_str() <<
					all << all << user.c_str());
				// msg 246: @1 is not grantor of @2 on @3 to @4.
			}

			ERR_post_warning(Arg::Warning(isc_dyn_miss_priv_warning) << all << all << user);
			// msg 247: Warning: @1 on @2 is not granted to @3.
		}
	}
	catch (const Exception& ex)
	{
		ex.stuff_exception(tdbb->tdbb_status_vector);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 255);
		// msg 255: "ERASE RDB$USER_PRIVILEGES failed in REVOKE ALL ON ALL"
	}
}


static void set_field_class_name(Global* gbl, const MetaName& relation, const MetaName& field)
{
   struct {
          SSHORT jrd_15;	// gds__utility 
   } jrd_14;
   struct {
          TEXT  jrd_13 [32];	// RDB$SECURITY_CLASS 
   } jrd_12;
   struct {
          SSHORT jrd_28;	// gds__utility 
   } jrd_27;
   struct {
          TEXT  jrd_25 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_26;	// gds__null_flag 
   } jrd_24;
   struct {
          TEXT  jrd_21 [32];	// RDB$SECURITY_CLASS 
          SSHORT jrd_22;	// gds__utility 
          SSHORT jrd_23;	// gds__null_flag 
   } jrd_20;
   struct {
          TEXT  jrd_18 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_19 [32];	// RDB$FIELD_NAME 
   } jrd_17;
/**************************************
 *
 *	s e t _ f i e l d _ c l a s s _ n a m e
 *
 **************************************
 *
 * Functional description
 *	For field level grants, be sure the
 *      field has a unique class name.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* const dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_s_f_class, DYN_REQUESTS);
	jrd_req* request2 = NULL;

	bool unique = false;

	/*FOR (REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
		RFR IN RDB$RELATION_FIELDS
		WITH RFR.RDB$FIELD_NAME = field.c_str() AND
			RFR.RDB$RELATION_NAME = relation.c_str() AND
		RFR.RDB$SECURITY_CLASS MISSING*/
	{
	if (!request)
	   request = CMP_compile2 (tdbb, (UCHAR*) jrd_16, sizeof(jrd_16), true);
	gds__vtov ((const char*) relation.c_str(), (char*) jrd_17.jrd_18, 32);
	gds__vtov ((const char*) field.c_str(), (char*) jrd_17.jrd_19, 32);
	EXE_start (tdbb, request, gbl->gbl_transaction);
	EXE_send (tdbb, request, 0, 64, (UCHAR*) &jrd_17);
	while (1)
	   {
	   EXE_receive (tdbb, request, 1, 36, (UCHAR*) &jrd_20);
	   if (!jrd_20.jrd_22) break;

		/*MODIFY RFR*/
		{
		
			while (!unique)
		    {
				sprintf(/*RFR.RDB$SECURITY_CLASS*/
					jrd_20.jrd_21, "%s%" SQUADFORMAT, SQL_FLD_SECCLASS_PREFIX,
				DPM_gen_id(tdbb, MET_lookup_generator(tdbb, "RDB$SECURITY_CLASS"), false, 1));

				unique = true;
				request2 = CMP_find_request(tdbb, drq_s_u_class, DYN_REQUESTS);
				/*FOR (REQUEST_HANDLE request2 TRANSACTION_HANDLE gbl->gbl_transaction)
					RFR1 IN RDB$RELATION_FIELDS
					WITH RFR1.RDB$SECURITY_CLASS = RFR.RDB$SECURITY_CLASS*/
				{
				if (!request2)
				   request2 = CMP_compile2 (tdbb, (UCHAR*) jrd_11, sizeof(jrd_11), true);
				gds__vtov ((const char*) jrd_20.jrd_21, (char*) jrd_12.jrd_13, 32);
				EXE_start (tdbb, request2, gbl->gbl_transaction);
				EXE_send (tdbb, request2, 0, 32, (UCHAR*) &jrd_12);
				while (1)
				   {
				   EXE_receive (tdbb, request2, 1, 2, (UCHAR*) &jrd_14);
				   if (!jrd_14.jrd_15) break;
					unique = false;
				/*END_FOR;*/
				   }
				}
		    }

			/*RFR.RDB$SECURITY_CLASS.NULL*/
			jrd_20.jrd_23 = FALSE;
		/*END_MODIFY;*/
		gds__vtov((const char*) jrd_20.jrd_21, (char*) jrd_24.jrd_25, 32);
		jrd_24.jrd_26 = jrd_20.jrd_23;
		EXE_send (tdbb, request, 2, 34, (UCHAR*) &jrd_24);
		}

	/*END_FOR;*/
	   EXE_send (tdbb, request, 3, 2, (UCHAR*) &jrd_27);
	   }
	}

	if (!DYN_REQUEST(drq_s_f_class))
		DYN_REQUEST(drq_s_f_class) = request;


	if (request2 && !DYN_REQUEST(drq_s_u_class))
		DYN_REQUEST(drq_s_u_class) = request2;
}


static void store_privilege(Global* gbl,
							const MetaName&	object,
							const MetaName&	user,
							const MetaName&	field,
							const TEXT* privilege,
							SSHORT user_type,
							SSHORT obj_type,
							int option,
							const MetaName&	grantor)
{
   struct {
          TEXT  jrd_2 [32];	// RDB$GRANTOR 
          TEXT  jrd_3 [32];	// RDB$USER 
          TEXT  jrd_4 [32];	// RDB$RELATION_NAME 
          TEXT  jrd_5 [32];	// RDB$FIELD_NAME 
          SSHORT jrd_6;	// RDB$GRANT_OPTION 
          SSHORT jrd_7;	// RDB$OBJECT_TYPE 
          SSHORT jrd_8;	// RDB$USER_TYPE 
          SSHORT jrd_9;	// gds__null_flag 
          TEXT  jrd_10 [7];	// RDB$PRIVILEGE 
   } jrd_1;
/**************************************
 *
 *	s t o r e _ p r i v i l e g e
 *
 **************************************
 *
 * Functional description
 *	Does its own cleanup in case of error, so calling
 *	routine should not.
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* const dbb = tdbb->getDatabase();

	jrd_req* request = CMP_find_request(tdbb, drq_s_grant, DYN_REQUESTS);

	// need to unwind our own request here!! SM 27-Sep-96

	try {
		/*STORE(REQUEST_HANDLE request TRANSACTION_HANDLE gbl->gbl_transaction)
			PRIV IN RDB$USER_PRIVILEGES*/
		{
		
			/*PRIV.RDB$FIELD_NAME.NULL*/
			jrd_1.jrd_9 = TRUE;
			strcpy(/*PRIV.RDB$RELATION_NAME*/
			       jrd_1.jrd_4, object.c_str());
			strcpy(/*PRIV.RDB$USER*/
			       jrd_1.jrd_3, user.c_str());
			strcpy(/*PRIV.RDB$GRANTOR*/
			       jrd_1.jrd_2, grantor.c_str());
			/*PRIV.RDB$USER_TYPE*/
			jrd_1.jrd_8 = user_type;
			/*PRIV.RDB$OBJECT_TYPE*/
			jrd_1.jrd_7 = obj_type;
			if (field.length())
			{
				strcpy(/*PRIV.RDB$FIELD_NAME*/
				       jrd_1.jrd_5, field.c_str());
				/*PRIV.RDB$FIELD_NAME.NULL*/
				jrd_1.jrd_9 = FALSE;
				set_field_class_name(gbl, object, field);
			}
			/*PRIV.RDB$PRIVILEGE*/
			jrd_1.jrd_10[0] = privilege[0];
			/*PRIV.RDB$PRIVILEGE*/
			jrd_1.jrd_10[1] = 0;
			/*PRIV.RDB$GRANT_OPTION*/
			jrd_1.jrd_6 = option;

		/*END_STORE;*/
		if (!request)
		   request = CMP_compile2 (tdbb, (UCHAR*) jrd_0, sizeof(jrd_0), true);
		EXE_start (tdbb, request, gbl->gbl_transaction);
		EXE_send (tdbb, request, 0, 143, (UCHAR*) &jrd_1);
		}

		if (!DYN_REQUEST(drq_s_grant)) {
			DYN_REQUEST(drq_s_grant) = request;
		}
	}
	catch (const Exception& ex)
	{
		stuff_exception(tdbb->tdbb_status_vector, ex);
		DYN_rundown_request(request, -1);
		DYN_error_punt(true, 79);
		// msg 79: "STORE RDB$USER_PRIVILEGES failed in grant"
	}
}


static void dyn_user(Global* /*gbl*/, const UCHAR** ptr)
{
/**************************************
 *
 *	d y n _ u s e r
 *
 **************************************
 *
 * Functional description
 *	Implements CREATE/ALTER/DROP USER
 *
 **************************************/
	thread_db* tdbb = JRD_get_thread_data();
	Database* const dbb = tdbb->getDatabase();
	jrd_tra* const tra = tdbb->getTransaction();

	Database::Checkout dcoHolder(dbb);

	ISC_STATUS_ARRAY status;
	try
	{
		internal_user_data* userData = FB_NEW(*tra->tra_pool) internal_user_data;
		UCHAR verb;
		while ((verb = *(*ptr)++) != isc_user_end)
		{
			string text;
			GET_STRING(ptr, text);

			switch(verb)
			{
			case isc_dyn_user_add:
				text.upper();
				userData->operation = ADD_OPER;
				text.copyTo(userData->user_name, sizeof(userData->user_name));
				userData->user_name_entered = true;
				break;

			case isc_dyn_user_mod:
				text.upper();
				userData->operation = MOD_OPER;
				text.copyTo(userData->user_name, sizeof(userData->user_name));
				userData->user_name_entered = true;
				break;

			case isc_dyn_user_del:
				text.upper();
				userData->operation = DEL_OPER;
				text.copyTo(userData->user_name, sizeof(userData->user_name));
				userData->user_name_entered = true;
				break;

			case isc_dyn_user_passwd:
				if (text.isEmpty())
				{
					// 250: Password should not be empty string
					status_exception::raise(Arg::Gds(ENCODE_ISC_MSG(250, DYN_MSG_FAC)));
				}
				text.copyTo(userData->password, sizeof(userData->password));
				userData->password_entered = true;
				break;

			case isc_dyn_user_first:
				if (text.hasData())
				{
					text.copyTo(userData->first_name, sizeof(userData->first_name));
					userData->first_name_entered = true;
				}
				else
				{
					userData->first_name_entered = false;
					userData->first_name_specified = true;
				}
				break;

			case isc_dyn_user_middle:
				if (text.hasData())
				{
					text.copyTo(userData->middle_name, sizeof(userData->middle_name));
					userData->middle_name_entered = true;
				}
				else
				{
					userData->middle_name_entered = false;
					userData->middle_name_specified = true;
				}
				break;

			case isc_dyn_user_last:
				if (text.hasData())
				{
					text.copyTo(userData->last_name, sizeof(userData->last_name));
					userData->last_name_entered = true;
				}
				else
				{
					userData->last_name_entered = false;
					userData->last_name_specified = true;
				}
				break;

			case isc_dyn_user_admin:
				userData->admin = text[0] - '0';
				userData->admin_entered = true;
				break;
			}
		}

		USHORT id = tra->getUserManagement()->put(userData);
		DFW_post_work(tra, dfw_user_management, NULL, id);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
		memmove(&status[2], &status[0], sizeof(status) - 2 * sizeof(status[0]));
		status[0] = isc_arg_gds;
		status[1] = isc_no_meta_update;
		status_exception::raise(status);
	}
}
