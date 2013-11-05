//____________________________________________________________
//
//		PROGRAM:	Alice (All Else) Utility
//		MODULE:		exe.cpp
//		DESCRIPTION:	Does the database calls
//
//  The contents of this file are subject to the Interbase Public
//  License Version 1.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy
//  of the License at http://www.Inprise.com/IPL.html
//
//  Software distributed under the License is distributed on an
//  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
//  or implied. See the License for the specific language governing
//  rights and limitations under the License.
//
//  The Original Code was created by Inprise Corporation
//  and its predecessors. Portions created by Inprise Corporation are
//  Copyright (C) Inprise Corporation.
//
//  All Rights Reserved.
//  Contributor(s): ______________________________________.
//
//
//____________________________________________________________
//
//
// 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
//                         conditionals, as the engine now fully supports
//                         readonly databases.
//
// 2002.10.30 Sean Leyne - Removed obsolete "PC_PLATFORM" define
//

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../jrd/ibase.h"
#include "../jrd/common.h"
#include "../alice/alice.h"
#include "../alice/alice_proto.h"
#include "../alice/aliceswi.h"
#include "../alice/alice_meta.h"
#include "../alice/tdr_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/constants.h"
#include "../common/classes/ClumpletWriter.h"


static void buildDpb(Firebird::ClumpletWriter&, const SINT64);
static void extract_db_info(const UCHAR*, size_t);

// Keep always in sync with function extract_db_info()
static const TEXT val_errors[] =
{
	isc_info_page_errors, isc_info_record_errors, isc_info_bpage_errors,
	isc_info_dpage_errors, isc_info_ipage_errors, isc_info_ppage_errors,
	isc_info_tpage_errors, isc_info_end
};


//____________________________________________________________
//
//

int EXE_action(const TEXT* database, const SINT64 switches)
{
	bool error = false;
	Firebird::AutoMemoryPool newPool(MemoryPool::createPool());
	{
		AliceGlobals* tdgbl = AliceGlobals::getSpecific();
		AliceContextPoolHolder context(tdgbl, newPool);

		for (USHORT i = 0; i < MAX_VAL_ERRORS; i++)
		{
			tdgbl->ALICE_data.ua_val_errors[i] = 0;
		}

		// generate the database parameter block for the attach,
		// based on the various switches
		Firebird::ClumpletWriter dpb(Firebird::ClumpletReader::Tagged, MAX_DPB_SIZE);
		buildDpb(dpb, switches);

		FB_API_HANDLE handle = 0;
		isc_attach_database(tdgbl->status, 0, database, &handle,
			dpb.getBufferLength(),
			reinterpret_cast<const SCHAR*>(dpb.getBuffer()));

		if (tdgbl->status[1] &&
			// Ignore isc_shutdown error produced when we switch to full shutdown mode. It is expected.
			(tdgbl->status[1] != isc_shutdown || !(switches & sw_shut) ||
				tdgbl->ALICE_data.ua_shutdown_mode != SHUT_FULL))
		{
			error = true;
		}

		if (tdgbl->status[2] == isc_arg_warning)
		{
			Firebird::makePermanentVector(tdgbl->status);
			ALICE_print_status(false, tdgbl->status);
		}
		else if (error)
		{
			Firebird::makePermanentVector(tdgbl->status);
		}

		if (handle != 0)
		{
			UCHAR error_string[128];
			if ((switches & sw_validate) && (tdgbl->status[1] != isc_bug_check))
			{
				isc_database_info(tdgbl->status, &handle, sizeof(val_errors),
							   val_errors, sizeof(error_string),
							   reinterpret_cast<char*>(error_string));

				extract_db_info(error_string, sizeof(error_string));
			}

			if (switches & sw_disable)
			{
				MET_disable_wal(tdgbl->status, handle);
			}

			isc_detach_database(tdgbl->status, &handle);
		}

		if (error)
		{
			tdgbl->uSvc->setServiceStatus(tdgbl->status);
		}
	}

	return error ? FINI_ERROR : FINI_OK;
}


//____________________________________________________________
//
//

int EXE_two_phase(const TEXT* database, const SINT64 switches)
{
	bool error = false;
	Firebird::AutoMemoryPool newPool(MemoryPool::createPool());
	{
		AliceGlobals* tdgbl = AliceGlobals::getSpecific();
		AliceContextPoolHolder context(tdgbl, newPool);

		for (USHORT i = 0; i < MAX_VAL_ERRORS; i++)
		{
			tdgbl->ALICE_data.ua_val_errors[i] = 0;
		}

		// generate the database parameter block for the attach,
		// based on the various switches
		Firebird::ClumpletWriter dpb(Firebird::ClumpletReader::Tagged, MAX_DPB_SIZE);
		buildDpb(dpb, switches);

		FB_API_HANDLE handle = 0;
		isc_attach_database(tdgbl->status, 0, database, &handle,
			dpb.getBufferLength(), reinterpret_cast<const SCHAR*>(dpb.getBuffer()));

		tdgbl->uSvc->started();

		if (tdgbl->status[1])
		{
			error = true;
		}
		else if (switches & sw_list)
		{
			TDR_list_limbo((handle), database, switches);
		}
		else if (switches & (sw_commit | sw_rollback | sw_two_phase))
		{
			error = TDR_reconnect_multiple(handle, tdgbl->ALICE_data.ua_transaction, database, switches);
		}

		if (handle)
		{
			isc_detach_database(tdgbl->status, &handle);
		}

		if (error)
		{
			tdgbl->uSvc->setServiceStatus(tdgbl->status);
		}
	}

	return (error ? FINI_ERROR : FINI_OK);
}

//____________________________________________________________
//
//
//  generate the database parameter block for the attach,
//  based on the various switches
//

static void buildDpb(Firebird::ClumpletWriter& dpb, const SINT64 switches)
{
	AliceGlobals* tdgbl = AliceGlobals::getSpecific();
	dpb.reset(isc_dpb_version1);
	dpb.insertTag(isc_dpb_gfix_attach);
	tdgbl->uSvc->getAddressPath(dpb);

	if (switches & sw_sweep) {
		dpb.insertByte(isc_dpb_sweep, isc_dpb_records);
	}
	else if (switches & sw_activate) {
		dpb.insertTag(isc_dpb_activate_shadow);
	}
	else if (switches & sw_validate)
	{
		UCHAR b = isc_dpb_pages;
		if (switches & sw_full)
			b |= isc_dpb_records;
		if (switches & sw_no_update)
			b |= isc_dpb_no_update;
		if (switches & sw_mend)
			b |= isc_dpb_repair;
		if (switches & sw_ignore)
			b |= isc_dpb_ignore;
		dpb.insertByte(isc_dpb_verify, b);
	}
	else if (switches & sw_housekeeping) {
		dpb.insertInt(isc_dpb_sweep_interval, tdgbl->ALICE_data.ua_sweep_interval);
	}
/*
	else if (switches & sw_begin_log)
	{
		dpb.insertString(isc_dpb_begin_log,
						 tdgbl->ALICE_data.ua_log_file,
						 strlen(tdgbl->ALICE_data.ua_log_file));
	}
	else if (switches & sw_quit_log) {
		dpb.insertTag(isc_dpb_quit_log);
	}
*/
	else if (switches & sw_buffers) {
		dpb.insertInt(isc_dpb_set_page_buffers, tdgbl->ALICE_data.ua_page_buffers);
	}
	else if (switches & sw_kill) {
		dpb.insertTag(isc_dpb_delete_shadow);
	}
	else if (switches & sw_write) {
		dpb.insertByte(isc_dpb_force_write, tdgbl->ALICE_data.ua_force ? 1 : 0);
	}
	else if (switches & sw_no_reserve) {
		dpb.insertByte(isc_dpb_no_reserve, tdgbl->ALICE_data.ua_no_reserve ? 1 : 0);
	}
	else if (switches & sw_mode) {
		dpb.insertByte(isc_dpb_set_db_readonly, tdgbl->ALICE_data.ua_read_only ? 1 : 0);
	}
	else if (switches & sw_shut)
	{
		UCHAR b = 0;
		if (switches & sw_attach)
			b |= isc_dpb_shut_attachment;
		else if (switches & sw_cache)
			b |= isc_dpb_shut_cache;
		else if (switches & sw_force)
			b |= isc_dpb_shut_force;
		else if (switches & sw_tran)
			b |= isc_dpb_shut_transaction;

		switch (tdgbl->ALICE_data.ua_shutdown_mode)
		{
		case SHUT_NORMAL:
			b |= isc_dpb_shut_normal;
			break;
		case SHUT_SINGLE:
			b |= isc_dpb_shut_single;
			break;
		case SHUT_MULTI:
			b |= isc_dpb_shut_multi;
			break;
		case SHUT_FULL:
			b |= isc_dpb_shut_full;
			break;
		default:
			break;
		}
		dpb.insertByte(isc_dpb_shutdown, b);
		// SSHORT is used for timeouts inside engine,
		// therefore convert larger values to MAX_SSHORT
		SLONG timeout = tdgbl->ALICE_data.ua_shutdown_delay;
		if (timeout > MAX_SSHORT)
		{
			timeout = MAX_SSHORT;
		}
		dpb.insertInt(isc_dpb_shutdown_delay, timeout);
	}
	else if (switches & sw_online)
	{
		UCHAR b = 0;
		switch (tdgbl->ALICE_data.ua_shutdown_mode)
		{
		case SHUT_NORMAL:
			b |= isc_dpb_shut_normal;
			break;
		case SHUT_SINGLE:
			b |= isc_dpb_shut_single;
			break;
		case SHUT_MULTI:
			b |= isc_dpb_shut_multi;
			break;
		case SHUT_FULL:
			b |= isc_dpb_shut_full;
			break;
		default:
			break;
		}
		dpb.insertByte(isc_dpb_online, b);
	}
	else if (switches & sw_disable) {
		dpb.insertTag(isc_dpb_disable_wal);
	}
	else if (switches & (sw_list | sw_commit | sw_rollback | sw_two_phase)) {
		dpb.insertTag(isc_dpb_no_garbage_collect);
	}
	else if (switches & sw_set_db_dialect) {
		dpb.insertInt(isc_dpb_set_db_sql_dialect, tdgbl->ALICE_data.ua_db_SQL_dialect);
	}

	if (tdgbl->ALICE_data.ua_user)
	{
		dpb.insertString(isc_dpb_user_name,
						 tdgbl->ALICE_data.ua_user, strlen(tdgbl->ALICE_data.ua_user));
	}
	if (tdgbl->ALICE_data.ua_password)
	{
		dpb.insertString(tdgbl->uSvc->isService() ? isc_dpb_password_enc : isc_dpb_password,
						 tdgbl->ALICE_data.ua_password, strlen(tdgbl->ALICE_data.ua_password));
	}
	if (tdgbl->ALICE_data.ua_tr_user)
	{
		tdgbl->uSvc->checkService();
		dpb.insertString(isc_dpb_trusted_auth,
						 tdgbl->ALICE_data.ua_tr_user, strlen(tdgbl->ALICE_data.ua_tr_user));
	}
	if (tdgbl->ALICE_data.ua_tr_role)
	{
		tdgbl->uSvc->checkService();
		dpb.insertString(isc_dpb_trusted_role, ADMIN_ROLE, strlen(ADMIN_ROLE));
	}
#ifdef TRUSTED_AUTH
	if (tdgbl->ALICE_data.ua_trusted)
	{
		if (!dpb.find(isc_dpb_trusted_auth)) {
			dpb.insertTag(isc_dpb_trusted_auth);
		}
	}
#endif
}


//____________________________________________________________
//
//		Extract database info from string
//

static void extract_db_info(const UCHAR* db_info_buffer, size_t buf_size)
{
	AliceGlobals* tdgbl = AliceGlobals::getSpecific();

	const UCHAR* p = db_info_buffer;
	const UCHAR* const end = p + buf_size;

	UCHAR item;
	while ((item = *p++) != isc_info_end && p < end - 1)
	{
		const SLONG length = gds__vax_integer(p, 2);
		p += 2;

		// TMN: Here we should really have the following assert
		// fb_assert(length <= MAX_SSHORT);
		// for all cases that use 'length' as input to 'gds__vax_integer'
		// Remember to keep this list in sync with the val_errors array.
		int pos = -1;

		switch (item)
		{
		case isc_info_page_errors:
			pos = VAL_PAGE_ERRORS;
			break;

		case isc_info_record_errors:
			pos = VAL_RECORD_ERRORS;
			break;

		case isc_info_bpage_errors:
			pos = VAL_BLOB_PAGE_ERRORS;
			break;

		case isc_info_dpage_errors:
			pos = VAL_DATA_PAGE_ERRORS;
			break;

		case isc_info_ipage_errors:
			pos = VAL_INDEX_PAGE_ERRORS;
			break;

		case isc_info_ppage_errors:
			pos = VAL_POINTER_PAGE_ERRORS;
			break;

		case isc_info_tpage_errors:
			pos = VAL_TIP_PAGE_ERRORS;
			break;

		case isc_info_error:
			// has to be a < V4 database.
			tdgbl->ALICE_data.ua_val_errors[VAL_INVALID_DB_VERSION] = 1;
			return;

		default:
			fb_assert(false);
		}

		if (pos != -1)
			tdgbl->ALICE_data.ua_val_errors[pos] = gds__vax_integer(p, (SSHORT) length);

		p += length;
	}
}
