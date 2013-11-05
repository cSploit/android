/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sdw.cpp
 *	DESCRIPTION:	Disk Shadowing Manager
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
 */

#include "firebird.h"
#include <string.h>
#include "../jrd/common.h"
#include <stdio.h>

#include "../jrd/jrd.h"
#include "../jrd/lck.h"
#include "../jrd/ods.h"
#include "../jrd/cch.h"
#include "gen/iberror.h"
#include "../jrd/lls.h"
#include "../jrd/req.h"
#include "../jrd/os/pio.h"
#include "../jrd/sdw.h"
#include "../jrd/sbm.h"
#include "../jrd/flags.h"
#include "../jrd/cch_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/isc_f_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/sdw_proto.h"


using namespace Jrd;
using namespace Ods;
using namespace Firebird;


// Out of alpha order because the first one was public.
static void shutdown_shadow(Shadow* shadow);
static void activate_shadow(thread_db* tdbb);
static Shadow* allocate_shadow(jrd_file*, USHORT, USHORT);
static int blocking_ast_shadowing(void*);
static bool check_for_file(thread_db* tdbb, const SCHAR*, USHORT);
#ifdef NOT_USED_OR_REPLACED
static void check_if_got_ast(thread_db* tdbb, jrd_file*);
#endif
static void copy_header(thread_db* tdbb);
static void update_dbb_to_sdw(Database*);


void SDW_add(thread_db* tdbb, const TEXT* file_name, USHORT shadow_number, USHORT file_flags)
{
/**************************************
 *
 *	S D W _ a d d
 *
 **************************************
 *
 * Functional description
 *	Add a brand new shadowing file to the database.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// Verify database file path against DatabaseAccess entry of firebird.conf
	if (!JRD_verify_database_access(file_name))
	{
		ERR_post(Arg::Gds(isc_conf_access_denied) << Arg::Str("additional database file") <<
													 Arg::Str(file_name));
	}

	jrd_file* shadow_file = PIO_create(dbb, file_name, false, false, false);

	if (dbb->dbb_flags & (DBB_force_write | DBB_no_fs_cache))
	{
		PIO_force_write(shadow_file, dbb->dbb_flags & DBB_force_write,
			dbb->dbb_flags & DBB_no_fs_cache);
	}

	Shadow* shadow = allocate_shadow(shadow_file, shadow_number, file_flags);

	// dump out the header page, even if it is a conditional
	// shadow--the page will be fixed up properly

	if (shadow->sdw_flags & SDW_conditional)
		shadow->sdw_flags &= ~SDW_conditional;
	WIN window(HEADER_PAGE_NUMBER);
	CCH_FETCH(tdbb, &window, LCK_write, pag_header);
	CCH_MARK_MUST_WRITE(tdbb, &window);
	CCH_write_all_shadows(tdbb, 0, window.win_bdb, tdbb->tdbb_status_vector, 1, false);
	CCH_RELEASE(tdbb, &window);
	if (file_flags & FILE_conditional)
		shadow->sdw_flags |= SDW_conditional;
}


int SDW_add_file(thread_db* tdbb, const TEXT* file_name, SLONG start, USHORT shadow_number)
{
/**************************************
 *
 *	S D W _ a d d _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Add a file to a shadow set.
 *	Return the sequence number for the new file.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// Find the file to be extended

	jrd_file* shadow_file = 0;
	Shadow* shadow;
	for (shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
	{
		if ((shadow->sdw_number == shadow_number) &&
			!(shadow->sdw_flags & (SDW_IGNORE | SDW_rollover)))
		{
			shadow_file = shadow->sdw_file;
			break;
		}
	}

	if (!shadow) {
		return 0;
	}

	// find the last file in the list, open the new file

	jrd_file* file = shadow_file;
	while (file->fil_next) {
		file = file->fil_next;
	}

	// Verify shadow file path against DatabaseAccess entry of firebird.conf
	if (!JRD_verify_database_access(file_name))
	{
		ERR_post(Arg::Gds(isc_conf_access_denied) << Arg::Str("database shadow") <<
													 Arg::Str(file_name));
	}

	const SLONG sequence = PIO_add_file(dbb, shadow_file, file_name, start);
	if (!sequence)
		return 0;

	jrd_file* next = file->fil_next;

	if (dbb->dbb_flags & (DBB_force_write | DBB_no_fs_cache))
	{
		PIO_force_write(next, dbb->dbb_flags & DBB_force_write, dbb->dbb_flags & DBB_no_fs_cache);
	}

	// Always write the header page, even for a conditional
	// shadow that hasn't been activated.

	// allocate a spare buffer which is large enough,
	// and set up to release it in case of error. Align
	// the spare page buffer for raw disk access.

	SCHAR* const spare_buffer =
		FB_NEW(*tdbb->getDefaultPool()) char[dbb->dbb_page_size + MIN_PAGE_SIZE];
	// And why doesn't the code check that the allocation succeeds?

	SCHAR* spare_page =
		(SCHAR*) (((U_IPTR) spare_buffer + MIN_PAGE_SIZE - 1) & ~((U_IPTR) MIN_PAGE_SIZE - 1));

	try {

		// create the header using the spare_buffer

		header_page* header = (header_page*) spare_page;
		header->hdr_header.pag_type = pag_header;
		header->hdr_sequence = sequence;
		header->hdr_page_size = dbb->dbb_page_size;
		header->hdr_data[0] = HDR_end;
		header->hdr_end = HDR_SIZE;
		header->hdr_next_page = 0;

		// fool PIO_write into writing the scratch page into the correct place
		BufferDesc temp_bdb;
		temp_bdb.bdb_page = next->fil_min_page;
		temp_bdb.bdb_dbb = dbb;
		temp_bdb.bdb_buffer = (PAG) header;
		header->hdr_header.pag_checksum = CCH_checksum(&temp_bdb);
		if (!PIO_write(shadow_file, &temp_bdb, reinterpret_cast<Ods::pag*>(header), 0))
		{
			delete[] spare_buffer;
			return 0;
		}
		next->fil_fudge = 1;

		// Update the previous header page to point to new file --
		//	we can use the same header page, suitably modified,
		// because they all look pretty much the same at this point

		/*******************
		Fix for bug 7925. drop_gdb wan not dropping secondary file in
		multi-shadow files. The structure was not being filled with the
		info. Commented some code so that the structure will always be filled.

			-Sudesh 07/06/95

		The original code :
		===
		if (shadow_file == file)
		    copy_header(tdbb);
		else
		===
		************************/

		// Temporarly reverting the change ------- Sudesh 07/07/95 *******

		if (shadow_file == file)
		{
			copy_header(tdbb);
		}
		else
		{
			--start;
			header->hdr_data[0] = HDR_end;
			header->hdr_end = HDR_SIZE;
			header->hdr_next_page = 0;

			PAG_add_header_entry(tdbb, header, HDR_file, strlen(file_name),
								 reinterpret_cast<const UCHAR*>(file_name));
			PAG_add_header_entry(tdbb, header, HDR_last_page, sizeof(start),
								 reinterpret_cast<const UCHAR*>(&start));
			file->fil_fudge = 0;
			temp_bdb.bdb_page = file->fil_min_page;
			header->hdr_header.pag_checksum = CCH_checksum(&temp_bdb);
			if (!PIO_write(	shadow_file, &temp_bdb, reinterpret_cast<Ods::pag*>(header), 0))
			{
				delete[] spare_buffer;
				return 0;
			}
			if (file->fil_min_page) {
				file->fil_fudge = 1;
			}
		}

		if (file->fil_min_page) {
			file->fil_fudge = 1;
		}

		delete[] spare_buffer;

	}	// try
	catch (const Firebird::Exception&)
	{
		delete[] spare_buffer;
		throw;
	}

	return sequence;
}


void SDW_check(thread_db* tdbb)
{
/**************************************
 *
 *	S D W _ c h e c k
 *
 **************************************
 *
 * Functional description
 *	Check a shadow to see if it needs to
 *	be deleted or shut down.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	// first get rid of any shadows that need to be
	// deleted or shutdown; deleted shadows must also be shutdown

	// Check to see if there is a valid shadow in the shadow set,
	// if not then it is time to start an conditional shadow (if one has been defined).

	Shadow* next_shadow;
	for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = next_shadow)
	{
		next_shadow = shadow->sdw_next;

		if (shadow->sdw_flags & SDW_delete)
		{
			MET_delete_shadow(tdbb, shadow->sdw_number);
			gds__log("shadow %s deleted from database %s due to unavailability on write",
					 shadow->sdw_file->fil_string, dbb->dbb_filename.c_str());
		}

		// note that shutting down a shadow is destructive to the shadow block

		if (shadow->sdw_flags & SDW_shutdown)
			shutdown_shadow(shadow);
	}

	if (SDW_check_conditional(tdbb))
	{
		if (SDW_lck_update(tdbb, 0))
		{
			Lock temp_lock;
			Lock* lock = &temp_lock;
			lock->lck_dbb = dbb;
			lock->lck_length = sizeof(SLONG);
			lock->lck_key.lck_long = -1;
			lock->lck_type = LCK_update_shadow;
			lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
			lock->lck_parent = dbb->dbb_lock;

			LCK_lock(tdbb, lock, LCK_EX, LCK_NO_WAIT);
			if (lock->lck_physical == LCK_EX)
			{
				SDW_notify(tdbb);
				SDW_dump_pages(tdbb);
				LCK_release(tdbb, lock);
			}
		}
	}
}


bool SDW_check_conditional(thread_db* tdbb)
{
/**************************************
 *
 *	S D W _ c h e c k _ c o n d i t i o n a l
 *
 **************************************
 *
 * Functional description
 *	Check if a conditional shadow exists
 *	if so update meta data and return true
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// first get rid of any shadows that need to be
	// deleted or shutdown; deleted shadows must also be shutdown

	// Check to see if there is a valid shadow in the shadow set,
	// if not then it is time to start an conditional shadow (if one has been defined).

	bool start_conditional = true;
	Shadow* next_shadow;
	for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = next_shadow)
	{
		next_shadow = shadow->sdw_next;

		if (!(shadow->sdw_flags & (SDW_delete | SDW_shutdown)))
			if (!(shadow->sdw_flags & SDW_INVALID))
			{
				start_conditional = false;
				break;
			}
	}

	// if there weren't any conventional shadows, now is
	// the time to start the first conditional shadow in the list
	// Note that allocate_shadow keeps the sdw_next list sorted

	if (start_conditional)
	{
		for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
		{
			if ((shadow->sdw_flags & SDW_conditional) &&
				!(shadow->sdw_flags & (SDW_IGNORE | SDW_rollover)))
			{
				shadow->sdw_flags &= ~SDW_conditional;

				gds__log("conditional shadow %d %s activated for database %s",
						 shadow->sdw_number, shadow->sdw_file->fil_string, dbb->dbb_filename.c_str());
				USHORT file_flags = FILE_shadow;
				if (shadow->sdw_flags & SDW_manual)
					file_flags |= FILE_manual;
				MET_update_shadow(tdbb, shadow, file_flags);
				return true;
			}
		}
	}

	return false;
}


void SDW_close()
{
/**************************************
 *
 *	S D W _ c l o s e
 *
 **************************************
 *
 * Functional description
 *	Close all disk shadowing files associated with
 *	a database.
 *
 **************************************/
	Database* dbb = GET_DBB();

	for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
		PIO_close(shadow->sdw_file);
}


void SDW_dump_pages(thread_db* tdbb)
{
/**************************************
 *
 *	S D W _ d u m p _ p a g e s
 *
 **************************************
 *
 * Functional description
 *	Look for any shadow files that haven't been written yet.
 *	Fetch pages from the database and write them
 *	to all unwritten shadow files.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	gds__log("conditional shadow dumped for database %s", dbb->dbb_filename.c_str());
	const SLONG max = PAG_last_page(tdbb);

	// mark the first shadow in the list because we don't
	// want to start shadowing to any files that are added
	// while we are in the middle of dumping pages

	// none of these pages should need any alteration
	// since header pages for extend files are not handled at this level
	WIN window(DB_PAGE_SPACE, -1);
	window.win_flags = WIN_large_scan;
	window.win_scans = 1;

	for (SLONG page_number = HEADER_PAGE + 1; page_number <= max; page_number++)
	{
#ifdef SUPERSERVER_V2
		if (!(page_number % dbb->dbb_prefetch_sequence))
		{
			SLONG pages[PREFETCH_MAX_PAGES];

			SLONG number = page_number;
			SLONG i = 0;
			while (i < dbb->dbb_prefetch_pages && number <= max) {
				pages[i++] = number++;
			}

			CCH_PREFETCH(tdbb, pages, i);
		}
#endif
		for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
		{
			if (!(shadow->sdw_flags & (SDW_INVALID | SDW_dumped)))
			{
				window.win_page = page_number;

				// when copying a database, it is possible that there are some pages defined
				// in the pip that were never actually written to disk, in the case of a faked
				// page which was never written to disk because of a rollback; to prevent
				// checksum errors on this type of page, don't check for checksum when the
				// page type is 0

				CCH_FETCH_NO_CHECKSUM(tdbb, &window, LCK_read, pag_undefined);
				if (!CCH_write_all_shadows(tdbb, shadow, window.win_bdb,
										   tdbb->tdbb_status_vector, 1, false))
				{
					CCH_RELEASE(tdbb, &window);
					ERR_punt();
				}
				if (shadow->sdw_next)
					CCH_RELEASE(tdbb, &window);
				else
					CCH_RELEASE_TAIL(tdbb, &window);
			}
		}
	}

	// mark all shadows seen to this point as dumped

	for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
	{
		if (!(shadow->sdw_flags & SDW_INVALID))
			shadow->sdw_flags |= SDW_dumped;
	}
}


void SDW_get_shadows(thread_db* tdbb)
{
/**************************************
 *
 *	S D W _ g e t _ s h a d o w s
 *
 **************************************
 *
 * Functional description
 *	Get any new shadows that have been
 *	defined.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// unless we have one, get a shared lock to ensure that we don't miss any signals

	dbb->dbb_ast_flags &= ~DBB_get_shadows;

	Lock* lock = dbb->dbb_shadow_lock;

	if (lock->lck_physical != LCK_SR)
	{
		// fb_assert (lock->lck_physical == LCK_none);

		WIN window(HEADER_PAGE_NUMBER);
		const header_page* header = (header_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_header);
		lock->lck_key.lck_long = header->hdr_shadow_count;
		LCK_lock(tdbb, lock, LCK_SR, LCK_WAIT);
		CCH_RELEASE(tdbb, &window);
	}

	// get all new shadow files, marking that we looked at them first
	// to prevent missing any new ones later on, although it does not
	// matter for the purposes of the current page being written

	MET_get_shadow_files(tdbb, false);
}


void SDW_init(thread_db* tdbb, bool activate, bool delete_files)
{
/**************************************
 *
 *	S D W _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize shadowing by opening all shadow files and
 *	getting a lock on the semaphore for disk shadowing.
 *	When anyone tries to get an exclusive lock on this
 *	semaphore, it is a signal to check for a new file
 *	to use as a shadow.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// set up the lock block for synchronizing addition of new shadows

	header_page* header; // for sizeof here, used later
	const USHORT key_length = sizeof(header->hdr_shadow_count);
	Lock* lock = FB_NEW_RPT(*dbb->dbb_permanent, key_length) Lock();
	dbb->dbb_shadow_lock = lock;
	lock->lck_type = LCK_shadow;
	lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
	lock->lck_parent = dbb->dbb_lock;
	lock->lck_length = key_length;
	lock->lck_dbb = dbb;
	lock->lck_object = dbb;
	lock->lck_ast = blocking_ast_shadowing;

	if (activate)
		activate_shadow(tdbb);

	// get current shadow lock count from database header page

	WIN window(HEADER_PAGE_NUMBER);

	header = (header_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_header);
	lock->lck_key.lck_long = header->hdr_shadow_count;
	LCK_lock(tdbb, lock, LCK_SR, LCK_WAIT);
	CCH_RELEASE(tdbb, &window);

	MET_get_shadow_files(tdbb, delete_files);
}


bool SDW_lck_update(thread_db* tdbb, SLONG sdw_update_flags)
{
/**************************************
 *
 *	 S D W _ l c k _ u p d a t e
 *
 **************************************
 *
 * Functional description
 *  update the Lock struct with the flag
 *  The update type flag indicates the type of corrective action
 *  to be taken by the ASTs of other procs attached to this DB.
 *
 *  A non zero sdw_update_flag is passed, it indicates error handling
 *  Two processes may encounter the Shadow array at the same time
 *  and both will want to perform corrective action. Only one should
 *  be allowed. For that,
 *		check if current data is zero, else return
 *  	write the pid into the lock data, read back to verify
 *      if pid is different, another process has updated behind you, so
 *		let him handle it and return
 *  	Update the data with sdw_update_flag passed to the function
 *
 **************************************/
	Database* dbb = GET_DBB();
	Lock* lock = dbb->dbb_shadow_lock;
	if (!lock)
		return false;

	if (lock->lck_physical != LCK_SR)
	{
		// fb_assert (lock->lck_physical == LCK_none);
		return false;
	}

	if (!sdw_update_flags) {
		return !LCK_read_data(tdbb, lock);
	}

	if (LCK_read_data(tdbb, lock))
		return false;

	LCK_write_data(tdbb, lock, lock->lck_id);
	if (LCK_read_data(tdbb, lock) != lock->lck_id)
		return false;

	LCK_write_data(tdbb, lock, sdw_update_flags);
	return true;
}


void SDW_notify(thread_db* tdbb)
{
/**************************************
 *
 *	S D W _ n o t i f y
 *
 **************************************
 *
 * Functional description
 *	Notify other processes that there has been
 *	a shadow added.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// get current shadow lock count from database header page --
	// note that since other processes need the header page to issue locks
	// on the shadow count, this is effectively an uninterruptible operation

	WIN window(HEADER_PAGE_NUMBER);
	header_page* header = (header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
	CCH_MARK_MUST_WRITE(tdbb, &window);

	// get an exclusive lock on the current shadowing semaphore to
	// notify other processes to find my shadow -- if we have a shared
	// on it already, convert to exclusive

	Lock* lock = dbb->dbb_shadow_lock;

	if (lock->lck_physical == LCK_SR)
	{
		if (lock->lck_key.lck_long != header->hdr_shadow_count)
			BUGCHECK(162);		// msg 162 shadow lock not synchronized properly
		LCK_convert(tdbb, lock, LCK_EX, LCK_WAIT);
	}
	else
	{
		lock->lck_key.lck_long = header->hdr_shadow_count;
		LCK_lock(tdbb, lock, LCK_EX, LCK_WAIT);
	}

	LCK_release(tdbb, lock);

	// now get a shared lock on the incremented shadow count to ensure that
	// we will get notification of the next shadow add

	lock->lck_key.lck_long = ++header->hdr_shadow_count;
	LCK_lock(tdbb, lock, LCK_SR, LCK_WAIT);

	CCH_RELEASE(tdbb, &window);
}


bool SDW_rollover_to_shadow(thread_db* tdbb, jrd_file* file, const bool inAst)
{
/**************************************
 *
 *	S D W _ r o l l o v e r _ t o _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	if (file != pageSpace->file)
		return true;

	SLONG sdw_update_flags = SDW_rollover;

	// If our attachment is already purged and an error comes from
	// CCH_fini(), then consider us accessing the shadow exclusively.
	// LCK_update_shadow locking isn't going to work anyway. The below
	// code must be executed for valid active attachments only.

	AutoPtr<Lock> update_lock;

	if (tdbb->getAttachment())
	{
		update_lock = FB_NEW_RPT(*tdbb->getDefaultPool(), 0) Lock;
		update_lock->lck_dbb = dbb;
		update_lock->lck_length = sizeof(SLONG);
		update_lock->lck_key.lck_long = -1;
		update_lock->lck_type = LCK_update_shadow;
		update_lock->lck_owner_handle = LCK_get_owner_handle(tdbb, update_lock->lck_type);
		update_lock->lck_parent = dbb->dbb_lock;

		LCK_lock(tdbb, update_lock, LCK_EX, LCK_NO_WAIT);

		if (update_lock->lck_physical != LCK_EX ||
			file != pageSpace->file || !SDW_lck_update(tdbb, sdw_update_flags))
		{
			LCK_release(tdbb, update_lock);
			LCK_lock(tdbb, update_lock, LCK_SR, LCK_NO_WAIT);
			while (update_lock->lck_physical != LCK_SR)
			{
				if (dbb->dbb_ast_flags & DBB_get_shadows)
					break;
				if ((file != pageSpace->file ) || !dbb->dbb_shadow_lock)
					break;
				LCK_lock(tdbb, update_lock, LCK_SR, LCK_NO_WAIT);
			}

			if (update_lock->lck_physical == LCK_SR)
				LCK_release(tdbb, update_lock);

			return true;
		}
	}
	else
	{
		if (!SDW_lck_update(tdbb, sdw_update_flags))
			return true;
	}

	// At this point we should have an exclusive update lock as well
	// as our opcode being written into the shadow lock data.

	Lock* shadow_lock = dbb->dbb_shadow_lock;

	// check the various status flags to see if there
	// is a valid shadow to roll over to

	Shadow* shadow;
	for (shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
	{
		if (!(shadow->sdw_flags & SDW_dumped))
			continue;
		if (!(shadow->sdw_flags & SDW_INVALID))
			break;
	}

	if (!shadow)
	{
		LCK_write_data(tdbb, shadow_lock, (SLONG) 0);
		if (update_lock)
			LCK_release(tdbb, update_lock);
		return false;
	}

	if (file != pageSpace->file)
	{
		LCK_write_data(tdbb, shadow_lock, (SLONG) 0);
		if (update_lock)
			LCK_release(tdbb, update_lock);
		return true;
	}

	// close the main database file if possible and release all file blocks

	PIO_close(pageSpace->file);

	while ( (file = pageSpace->file) )
	{
		pageSpace->file = file->fil_next;
		delete file;
	}

	/* point the main database file at the file of the first shadow
	in the list and mark that shadow as rolled over to
	so that we won't write to it twice -- don't remove
	this shadow from the linked list of shadows because
	that would cause us to create a new shadow block for
	it the next time we do a MET_get_shadow_files () */

	pageSpace->file = shadow->sdw_file;
	shadow->sdw_flags |= SDW_rollover;

	// check conditional does a meta data update - since we were
	// successfull updating LCK_data we will be the only one doing so

	bool start_conditional = false;
	if (!inAst)
	{
		if ( (start_conditional = SDW_check_conditional(tdbb)) )
		{
			sdw_update_flags = (SDW_rollover | SDW_conditional);
			LCK_write_data(tdbb, shadow_lock, sdw_update_flags);
		}
	}

	SDW_notify(tdbb);
	LCK_write_data(tdbb, shadow_lock, (SLONG) 0);
	LCK_release(tdbb, shadow_lock);
	delete shadow_lock;
	dbb->dbb_shadow_lock = 0;
	if (update_lock)
		LCK_release(tdbb, update_lock);
	if (start_conditional && !inAst)
	{
		CCH_unwind(tdbb, false);
		SDW_dump_pages(tdbb);
		ERR_post(Arg::Gds(isc_deadlock));
	}

	return true;
}


// It's never called directly, but through SDW_check().
static void shutdown_shadow(Shadow* shadow)
{
/**************************************
 *
 *	s h u t d o w n _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *	Stop shadowing to a given shadow number.
 *
 **************************************/
	Database* dbb = GET_DBB();

	// find the shadow block and delete it from linked list

	for (Shadow** ptr = &dbb->dbb_shadow; *ptr; ptr = &(*ptr)->sdw_next)
	{
		if (*ptr == shadow) {
			*ptr = shadow->sdw_next;
			break;
		}
	}

	// close the shadow files and free up the associated memory

	if (shadow)
	{
		PIO_close(shadow->sdw_file);
		jrd_file* file;
		jrd_file* free = shadow->sdw_file;
		for (; (file = free->fil_next); free = file)
		{
			delete free;
		}
		delete free;
		delete shadow;
	}
}


void SDW_start(thread_db* tdbb, const TEXT* file_name,
			   USHORT shadow_number, USHORT file_flags, bool delete_files)
{
/**************************************
 *
 *	S D W _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Commence shadowing on a previously created shadow file.
 *
 *	<delete_files> is true if we are not actually starting shadowing,
 *	but deleting inaccessible shadow files.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	USHORT header_fetched = 0;

	// check that this shadow has not already been started,
	// (unless it is marked as invalid, in which case it may
	// be an old shadow of the same number)

	Shadow* shadow;
	for (shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
	{
		if ((shadow->sdw_number == shadow_number) && !(shadow->sdw_flags & SDW_INVALID))
		{
			return;
		}
	}

	for (shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
	{
		if (shadow->sdw_number == shadow_number)
			break;
	}

	// check to see if the shadow is the same as the current database --
	// if so, a shadow file is being accessed as a database

	Firebird::PathName expanded_name(file_name);
	ISC_expand_filename(expanded_name, false);
	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	jrd_file* dbb_file = pageSpace->file;

	if (dbb_file && dbb_file->fil_string && expanded_name == dbb_file->fil_string)
	{
		if (shadow && (shadow->sdw_flags & SDW_rollover))
			return;

		ERR_post(Arg::Gds(isc_shadow_accessed));
	}

	// Verify shadow file path against DatabaseAccess entry of firebird.conf
	if (!JRD_verify_database_access(expanded_name))
	{
		ERR_post(Arg::Gds(isc_conf_access_denied) << Arg::Str("database shadow") <<
													 Arg::Str(expanded_name));
	}

	// catch errors: delete the shadow file if missing, and deallocate the spare buffer

	shadow = NULL;
	SLONG* const spare_buffer =
		FB_NEW(*tdbb->getDefaultPool()) SLONG[(dbb->dbb_page_size + MIN_PAGE_SIZE) / sizeof(SLONG)];
	SLONG* spare_page = reinterpret_cast<SLONG*>((SCHAR *)
												(((U_IPTR) spare_buffer + MIN_PAGE_SIZE - 1) &
													~((U_IPTR) MIN_PAGE_SIZE - 1)));

	WIN window(DB_PAGE_SPACE, -1);
	jrd_file* shadow_file = 0;

	try {

	shadow_file = PIO_open(dbb, expanded_name, file_name, false);

	if (dbb->dbb_flags & (DBB_force_write | DBB_no_fs_cache))
	{
		PIO_force_write(shadow_file, dbb->dbb_flags & DBB_force_write,
			dbb->dbb_flags & DBB_no_fs_cache);
	}

	if (!(file_flags & FILE_conditional))
	{
		// make some sanity checks on the database and shadow header pages:
		// 1. make sure that the proper database filename is accessing this shadow
		// 2. make sure the database and shadow are in sync by checking the creation time/transaction id
		// 3. make sure that the shadow has not already been activated

		window.win_page = HEADER_PAGE_NUMBER;
		const header_page* database_header =
			(header_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_header);
		header_fetched++;

		if (!PIO_read(shadow_file, window.win_bdb, (PAG) spare_page, tdbb->tdbb_status_vector))
		{
			ERR_punt();
		}

		const header_page* shadow_header = (header_page*) spare_page;

		//          NOTE ! NOTE! NOTE!
		// Starting V4.0, header pages can have overflow pages.  For the shadow,
		// we are making an assumption that the shadow header page will not
		// overflow, as the only things written on a shadow header is the
		// HDR_root_file_name, HDR_file, and HDR_last_page

		const UCHAR* p = shadow_header->hdr_data;
		while (*p != HDR_end && *p != HDR_root_file_name) {
			p += 2 + p[1];
		}
		if (*p++ == HDR_end)
			BUGCHECK(163);		// msg 163 root file name not listed for shadow

		// if the database file is not the same and the original file is
		// still around, then there is a possibility for shadow corruption

		const int string_length = (USHORT) *p++;
		const char* fname = reinterpret_cast<const char*>(p);
		if (strncmp(dbb_file->fil_string, fname, string_length) &&
			check_for_file(tdbb, fname, string_length))
		{
			ERR_punt();
		}

		if ((shadow_header->hdr_creation_date[0] != database_header->hdr_creation_date[0]) ||
			(shadow_header->hdr_creation_date[1] != database_header->hdr_creation_date[1]) ||
			!(shadow_header->hdr_flags & hdr_active_shadow))
		{
			ERR_punt();
		}

		CCH_RELEASE(tdbb, &window);
		header_fetched--;
	}

	// allocate the shadow block and mark it as
	// dumped (except for the cases when it isn't)

	shadow = allocate_shadow(shadow_file, shadow_number, file_flags);
	if (!(file_flags & FILE_conditional)) {
		shadow->sdw_flags |= SDW_dumped;
	}

	// get the ancillary files and reset the error environment

	PAG_init2(tdbb, shadow_number);

	delete[] spare_buffer;

	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		if (header_fetched) {
			CCH_RELEASE(tdbb, &window);
		}
		if (shadow_file)
		{
			PIO_close(shadow_file);
			delete shadow_file;
		}
		delete[] spare_buffer;
		if (file_flags & FILE_manual && !delete_files) {
			ERR_post(Arg::Gds(isc_shadow_missing) << Arg::Num(shadow_number));
		}
		else
		{
			MET_delete_shadow(tdbb, shadow_number);
			gds__log("shadow %s deleted from database %s due to unavailability on attach",
					 expanded_name.c_str(), dbb_file->fil_string);
		}
	}
}


static void activate_shadow(thread_db* tdbb)
{
/**************************************
 *
 *	a c t i v a t e _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *	Change a shadow into a database.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	gds__log("activating shadow file %s", dbb->dbb_filename.c_str());

	MET_activate_shadow(tdbb);

	// clear the shadow bit on the header page

	WIN window(HEADER_PAGE_NUMBER);
	header_page* header = (header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
	CCH_MARK_MUST_WRITE(tdbb, &window);
	header->hdr_flags &= ~hdr_active_shadow;

	CCH_RELEASE(tdbb, &window);
}


static Shadow* allocate_shadow(jrd_file* shadow_file,
							   USHORT shadow_number, USHORT file_flags)
{
/**************************************
 *
 *	a l l o c a t e _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *	Allocate a shadow block, setting all
 *	the fields properly.
 *
 **************************************/
	Database* dbb = GET_DBB();

	Shadow* shadow = FB_NEW(*dbb->dbb_permanent) Shadow();
	shadow->sdw_file = shadow_file;
	shadow->sdw_number = shadow_number;
	if (file_flags & FILE_manual)
		shadow->sdw_flags |= SDW_manual;
	if (file_flags & FILE_conditional)
		shadow->sdw_flags |= SDW_conditional;

	// Link the new shadow into the list of shadows according to
	// shadow number position.  This is so we will activate
	// conditional shadows in the order specified by shadow number.
	// Note that the shadow number may not be unique in this list
	// - as could happen when shadow X is dropped, and then X is
	// recreated.

	Shadow** pShadow;
	for (pShadow = &dbb->dbb_shadow; *pShadow; pShadow = &((*pShadow)->sdw_next))
	{
		if ((*pShadow)->sdw_number >=shadow_number)
			break;
	}

	shadow->sdw_next = *pShadow;
	*pShadow = shadow;

	return shadow;
}


static int blocking_ast_shadowing(void* ast_object)
{
/**************************************
 *
 *	b l o c k i n g _ a s t _ s h a d o w i n g
 *
 **************************************
 *
 * Functional description
 *	A blocking AST has been issued to give up
 *	the lock on the shadowing semaphore.
 *	Do so after flagging the need to check for
 *	new shadow files before doing the next physical write.
 *
 **************************************/
	Database* new_dbb = static_cast<Database*>(ast_object);

	try
	{
		Database::SyncGuard dsGuard(new_dbb, true);

		Lock* lock = new_dbb->dbb_shadow_lock;

		// Since this routine will be called asynchronously,
		// we must establish a thread context
		ThreadContextHolder tdbb;
		tdbb->setDatabase(new_dbb);

		new_dbb->dbb_ast_flags |= DBB_get_shadows;
		if (LCK_read_data(tdbb, lock) & SDW_rollover)
			update_dbb_to_sdw(new_dbb);

		LCK_release(tdbb, lock);
	}
	catch (const Firebird::Exception&)
	{} // no-op

	return 0;
}


static bool check_for_file(thread_db* tdbb, const SCHAR* name, USHORT length)
{
/**************************************
 *
 *	c h e c k _ f o r _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Check for the existence of a file.
 *	Return true if it is there.
 *
 **************************************/

	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	const Firebird::PathName path(name, length);

	try {
		// This use of PIO_open is NOT checked against DatabaseAccess configuration
		// parameter. It's not required, because here we only check for presence of
		// existing file, never really use (or create) it.
		jrd_file* temp_file = PIO_open(dbb, path, path, false);
		PIO_close(temp_file);
	}	// try
	catch (const Firebird::Exception& ex)
	{
		Firebird::stuff_exception(tdbb->tdbb_status_vector, ex);
		return false;
	}

	return true;
}

#ifdef NOT_USED_OR_REPLACED
static void check_if_got_ast(thread_db* tdbb, jrd_file* file)
{
/**************************************
 *
 *	c h e c k _ i f _ g o t _ a s t
 *
 **************************************
 *
 * Functional description
 * 	have we got the signal indicating a
 *	a shadow update
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	Lock* lock = dbb->dbb_shadow_lock;
	if (!lock || (file != dbb->dbb_file)) {
		return;
	}

	while (true)
	{
		if (dbb->dbb_ast_flags & DBB_get_shadows)
			break;
		LCK_convert(tdbb, lock, LCK_SR, LCK_WAIT);
	}
}
#endif

static void copy_header(thread_db* tdbb)
{
/**************************************
 *
 *	c o p y _ h e a d e r
 *
 **************************************
 *
 * Functional description
 *	Fetch the header page from the database
 *	and write it to the shadow file.  This is
 *	done so that if this shadow is extended,
 *	the header page will be there for writing
 *	the name of the extend file.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// get the database header page and write it out --
	// CCH will take care of modifying it

	WIN window(HEADER_PAGE_NUMBER);
	CCH_FETCH(tdbb, &window, LCK_write, pag_header);
	CCH_MARK_MUST_WRITE(tdbb, &window);
	CCH_RELEASE(tdbb, &window);
}


static void update_dbb_to_sdw(Database* dbb)
{
/**************************************
 *
 *	 u p d a t e _ d b b _ t o _ s d w
 *
 **************************************
 *
 * Functional description
 * 	Another process has indicated that dbb is corrupt
 *	so close dbb and initialize Shadow to dbb
 *
 **************************************/

	// find shadow to rollover to

	Shadow* shadow;
	for (shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
	{
		if (!(shadow->sdw_flags & SDW_dumped))
			continue;
		if (!(shadow->sdw_flags & SDW_INVALID))
			break;
	}

	if (!shadow)
		return;					// should be a BUGCHECK

	// close the main database file if possible and release all file blocks

	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	PIO_close(pageSpace->file);

	jrd_file* file;
	while ( (file = pageSpace->file) )
	{
		pageSpace->file = file->fil_next;
		delete file;
	}

	pageSpace->file = shadow->sdw_file;
	shadow->sdw_flags |= SDW_rollover;
}
