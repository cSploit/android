/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		nbak.cpp
 *	DESCRIPTION:	Incremental backup technology
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
 *  Contributor(s):
 *
 *  Roman Simakov <roman-simakov@users.sourceforge.net>
 *  Khorsun Vladyslav <hvlad@users.sourceforge.net>
 *
 */

#include "firebird.h"
#include "common.h"
#include "jrd.h"
#include "nbak.h"
#include "ods.h"
#include "lck.h"
#include "cch.h"
#include "lck_proto.h"
#include "pag_proto.h"
#include "err_proto.h"
#include "cch_proto.h"
#include "isc_proto.h"
#include "../jrd/thread_proto.h"
#include "os/pio_proto.h"
#include "gen/iberror.h"
#include "gds_proto.h"
#include "os/guid.h"
#include "os/isc_i_proto.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef NBAK_DEBUG
#include <stdarg.h>
IMPLEMENT_TRACE_ROUTINE(nbak_trace, "NBAK")
#endif


using namespace Jrd;
using namespace Firebird;


/******************************** NBackupStateLock ******************************/

NBackupStateLock::NBackupStateLock(thread_db* tdbb, MemoryPool& p, BackupManager* bakMan):
	GlobalRWLock(tdbb, p, LCK_backup_database, LCK_OWNER_database), backup_manager(bakMan)
{
}

bool NBackupStateLock::fetch(thread_db* tdbb)
{
	backup_manager->endFlush();
	NBAK_TRACE( ("backup_manager->endFlush()") );
	if (!backup_manager->actualizeState(tdbb))
	{
		ERR_bugcheck_msg("Can't actualize backup state");
	}
	return true;
}

void NBackupStateLock::invalidate(thread_db* tdbb)
{
	GlobalRWLock::invalidate(tdbb);
	NBAK_TRACE( ("invalidate state stateLock(%p)", this) );
	backup_manager->setState(nbak_state_unknown);
	backup_manager->closeDelta();
}

void NBackupStateLock::blockingAstHandler(thread_db* tdbb)
{
	const bool wasWrite = (cachedLock->lck_physical == LCK_write);

	if (!backup_manager->databaseFlushInProgress())
	{
		backup_manager->beginFlush();
		NBAK_TRACE_AST( ("backup_manager->beginFlush()") );
		CCH_flush_ast(tdbb);
		NBAK_TRACE_AST(("database FLUSHED"));
	}

	GlobalRWLock::blockingAstHandler(tdbb);

	if (wasWrite && (cachedLock->lck_physical == LCK_read))
		backup_manager->endFlush();
}


/******************************** NBackupAllocLock ******************************/

NBackupAllocLock::NBackupAllocLock(thread_db* tdbb, MemoryPool& p, BackupManager* bakMan):
	GlobalRWLock(tdbb, p, LCK_backup_alloc, LCK_OWNER_database), backup_manager(bakMan)
{
}

bool NBackupAllocLock::fetch(thread_db* tdbb)
{
	if (!backup_manager->actualizeAlloc(tdbb))
		ERR_bugcheck_msg("Can't actualize alloc table");
	return true;
}

/******************************** BackupManager::StateWriteGuard ******************************/

BackupManager::StateWriteGuard::StateWriteGuard(thread_db* _tdbb, Jrd::WIN* wnd)
	: tdbb(_tdbb), window(NULL), success(false)
{
	Database* dbb = tdbb->getDatabase();
	dbb->dbb_backup_manager->beginFlush();
	CCH_flush(tdbb, FLUSH_ALL, 0); // Flush local cache to release all dirty pages

	if (!tdbb->getAttachment()->backupStateWriteLock(tdbb, true))
		ERR_bugcheck_msg("Can't lock state for write");

	dbb->dbb_backup_manager->endFlush();
	NBAK_TRACE(("backup state locked for write"));
	CCH_FETCH(tdbb, wnd, LCK_write, pag_header);
	window = wnd;
}

BackupManager::StateWriteGuard::~StateWriteGuard()
{
	// It is important to set state into nbak_state_unknown *before* release of state lock, 
	// else someone could acquire state lock, fetch and modify some page before state will 
	// be set into unknown. But dirty page can't be written when backup state is unknown 
	// because write target (database or delta) is also unknown.

	if (!success)
	{
		NBAK_TRACE( ("invalidate state") );
		Database* dbb = tdbb->getDatabase();
		dbb->dbb_backup_manager->setState(nbak_state_unknown);
	}

	releaseHeader();
	tdbb->getAttachment()->backupStateWriteUnLock(tdbb);
}

void BackupManager::StateWriteGuard::releaseHeader()
{
	if (window)
	{
		CCH_RELEASE(tdbb, window);
		window = NULL;
	}
}

/********************************** CORE LOGIC ********************************/

void BackupManager::generateFilename()
{
	diff_name = database->dbb_filename + ".delta";
	explicit_diff_name = false;
}

void BackupManager::openDelta()
{
	fb_assert(!diff_file);
	diff_file = PIO_open(database, diff_name, diff_name, false);

	if (database->dbb_flags & (DBB_force_write | DBB_no_fs_cache))
	{
		PIO_force_write(diff_file,
			database->dbb_flags & DBB_force_write,
			database->dbb_flags & DBB_no_fs_cache);
	}
}

void BackupManager::closeDelta()
{
	if (diff_file) {
		PIO_close(diff_file);
		diff_file = NULL;
	}
}

// Initialize and open difference file for writing
void BackupManager::beginBackup(thread_db* tdbb)
{
	NBAK_TRACE(("beginBackup"));

	SET_TDBB(tdbb);

	// Check for raw device
	if ((!explicit_diff_name) && database->onRawDevice()) {
		ERR_post(Arg::Gds(isc_need_difference));
	}

	WIN window(HEADER_PAGE_NUMBER);

	StateWriteGuard stateGuard(tdbb, &window);
	Ods::header_page* header = (Ods::header_page*) window.win_buffer;

	// Check state
	if (backup_state != nbak_state_normal) {
		NBAK_TRACE(("begin backup - invalid state %d", backup_state));
		stateGuard.setSuccess();
		return;
	}

	try
	{
		// Create file
		NBAK_TRACE(("Creating difference file %s", diff_name.c_str()));
		diff_file = PIO_create(database, diff_name, true, false, false);
	}
	catch (const Firebird::Exception&)
	{
		// no reasons to set it to unknown if we just failed to create difference file
		stateGuard.setSuccess();
		backup_state = nbak_state_normal;
		throw;
	}

	{ // logical scope
		if (database->dbb_flags & (DBB_force_write | DBB_no_fs_cache))
		{
			PIO_force_write(diff_file,
				database->dbb_flags & DBB_force_write,
				database->dbb_flags & DBB_no_fs_cache);
		}

#ifdef UNIX
		// adjust difference file access rights to make it match main DB ones
		if (diff_file && geteuid() == 0)
		{
			struct stat st;
			PageSpace* pageSpace = database->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
			char* func = NULL;
			while (!func && fstat(pageSpace->file->fil_desc, &st) != 0)
			{
				if (errno != EINTR) 
					func = "fstat";
			}
			while (!func && fchown(diff_file->fil_desc, st.st_uid, st.st_gid) != 0)
			{
				if (errno != EINTR) 
					func = "fchown";
			}
			while (!func && fchmod(diff_file->fil_desc, st.st_mode) != 0)
			{
				if (errno != EINTR) 
					func = "fchmod";
			}
			if (func) 
			{
				stateGuard.setSuccess();
				Firebird::system_call_failed::raise(func);
			}
		}
#endif

		// Zero out first page (empty allocation table)
		BufferDesc temp_bdb;
		temp_bdb.bdb_page = 0;
		temp_bdb.bdb_dbb = database;
		temp_bdb.bdb_buffer = reinterpret_cast<Ods::pag*>(alloc_buffer);
		memset(alloc_buffer, 0, database->dbb_page_size);
		if (!PIO_write(diff_file, &temp_bdb, temp_bdb.bdb_buffer, tdbb->tdbb_status_vector))
			ERR_punt();
		NBAK_TRACE(("Set backup state in header"));
		FB_GUID guid;
		GenerateGuid(&guid);
		// Set state in database header page. All changes are written to main database file yet.
		CCH_MARK_MUST_WRITE(tdbb, &window);
		const int newState = nbak_state_stalled; // Should be USHORT?
		header->hdr_flags = (header->hdr_flags & ~Ods::hdr_backup_mask) | newState;
		const ULONG adjusted_scn = ++header->hdr_header.pag_scn; // Generate new SCN
		PAG_replace_entry_first(tdbb, header, Ods::HDR_backup_guid, sizeof(guid),
			reinterpret_cast<const UCHAR*>(&guid));

		stateGuard.releaseHeader();
		stateGuard.setSuccess();

		backup_state = newState;
		current_scn = adjusted_scn;

		// All changes go to the difference file now
	}
}


// Determine actual DB size (raw devices support)
ULONG BackupManager::getPageCount()
{
	if (backup_state != nbak_state_stalled)
	{
		// calculate pages only when database is locked for backup:
		// other case such service is just dangerous
		return 0;
	}

	class PioCount : public Jrd::PageCountCallback
	{
	private:
	    BufferDesc temp_bdb;
		PageSpace* pageSpace;

	public:
		explicit PioCount(Database* d)
		{
			fb_assert(d);
			temp_bdb.bdb_dbb = d;
			pageSpace = d->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
			fb_assert(pageSpace);
		}
		virtual void newPage(const SLONG pageNum, Ods::pag* buf)
		{
			temp_bdb.bdb_buffer = buf;
			temp_bdb.bdb_page = pageNum;
			ISC_STATUS_ARRAY status;
			if (!PIO_read(pageSpace->file, &temp_bdb, temp_bdb.bdb_buffer, status))
			{
				Firebird::status_exception::raise(status);
			}
		}
	};
	PioCount pioCount(database);

	return PAG_page_count(database, &pioCount);
}


// Merge difference file to main files (if needed) and unlink() difference
// file then. If merge is already in progress method silently returns and
// does nothing (so it can be used for recovery on database startup).
void BackupManager::endBackup(thread_db* tdbb, bool recover)
{
	NBAK_TRACE(("end_backup, recover=%i", recover));

	// Check for recover

	GlobalRWLock endLock(tdbb, *database->dbb_permanent, LCK_backup_end,
		LCK_OWNER_attachment, false);

	if (!endLock.lockWrite(tdbb, LCK_NO_WAIT)) {
		// Someboby holds write lock on LCK_backup_end. We need not to do end_backup
		return;
	}

	// STEP 1. Change state in header to "merge"
	WIN window(HEADER_PAGE_NUMBER);
	Ods::header_page* header;

	ULONG adjusted_scn; // We use this value to prevent race conditions.
						// They are possible because we release state lock
						// for some instants and anything is possible at
						// that times.

	try
	{
		// Check state under PR lock of backup state for speed
		{ // scope
			StateReadGuard stateGuard(tdbb);
			// Nobody is doing end_backup but database isn't in merge state.
			if ( (recover || backup_state != nbak_state_stalled) && (backup_state != nbak_state_merge ) )
			{
				NBAK_TRACE(("invalid state %d", backup_state));
				endLock.unlockWrite(tdbb);
				return;
			}
		}

		// Here backup state can be changed. Need to check it again after lock
		StateWriteGuard stateGuard(tdbb, &window);

		if ( (recover || backup_state != nbak_state_stalled) && (backup_state != nbak_state_merge ) )
		{
			stateGuard.setSuccess();
			NBAK_TRACE(("invalid state %d", backup_state));
			endLock.unlockWrite(tdbb);
			return;
		}
		header = (Ods::header_page*) window.win_buffer;

		NBAK_TRACE(("difference file %s, current backup state is %d", diff_name.c_str(), backup_state));

		// Set state in database header
		backup_state = nbak_state_merge;
		adjusted_scn = ++current_scn;
		NBAK_TRACE(("New state is getting to become %d", backup_state));
		CCH_MARK_MUST_WRITE(tdbb, &window);
		// Generate new SCN
		header->hdr_header.pag_scn = current_scn;
		NBAK_TRACE(("new SCN=%d is getting written to header", header->hdr_header.pag_scn));
		// Adjust state
		header->hdr_flags = (header->hdr_flags & ~Ods::hdr_backup_mask) | backup_state;
		NBAK_TRACE(("Setting state %d in header page is over", backup_state));
		stateGuard.setSuccess();
	}
	catch (const Firebird::Exception&) {
		endLock.unlockWrite(tdbb);
		throw;
	}

	// STEP 2. Merging database and delta
	// Here comes the dirty work. We need to reapply all changes from difference file to database
	// Release write state lock and get read lock.
	// Merge process should not inhibit normal operations.

	try
	{
		NBAK_TRACE(("database locked to merge"));

		StateReadGuard stateGuard(tdbb);

		NBAK_TRACE(("Merge. State=%d, current_scn=%d, adjusted_scn=%d",
			backup_state, current_scn, adjusted_scn));

		actualizeAlloc(tdbb);
		NBAK_TRACE(("Merge. Alloc table is actualized."));
		AllocItemTree::Accessor all(alloc_table);

		if (all.getFirst()) {
			do {
				WIN window2(DB_PAGE_SPACE, all.current().db_page);
				NBAK_TRACE(("Merge page %d, diff=%d", all.current().db_page, all.current().diff_page));
				Ods::pag* page = CCH_FETCH(tdbb, &window2, LCK_write, pag_undefined);
				NBAK_TRACE(("Merge: page %d is fetched", all.current().db_page));
				if (page->pag_scn != current_scn) {
					CCH_MARK(tdbb, &window2);
					NBAK_TRACE(("Merge: page %d is marked", all.current().db_page));
				}
				CCH_RELEASE(tdbb, &window2);
				NBAK_TRACE(("Merge: page %d is released", all.current().db_page));
			} while (all.getNext());
		}

		CCH_flush(tdbb, FLUSH_ALL, 0);
		NBAK_TRACE(("Merging is over. Database unlocked"));
	}
	catch (const Firebird::Exception&) {
		endLock.unlockWrite(tdbb);
		throw;
	}

	// STEP 3. Change state in header to "normal"
	// We finished. We need to reflect it in our database header page
	try {
		window.win_page = HEADER_PAGE;
		window.win_flags = 0;

		StateWriteGuard stateGuard(tdbb, &window);

		header = (Ods::header_page*) window.win_buffer;

		// Set state in database header
		backup_state = nbak_state_normal;
		CCH_MARK_MUST_WRITE(tdbb, &window);
		// Adjust state
		header->hdr_flags = (header->hdr_flags & ~Ods::hdr_backup_mask) | backup_state;
		NBAK_TRACE(("Set state %d in header page", backup_state));
		// Generate new SCN
		header->hdr_header.pag_scn = ++current_scn;
		NBAK_TRACE(("new SCN=%d is getting written to header", header->hdr_header.pag_scn));

		stateGuard.releaseHeader();
		stateGuard.setSuccess();

		// Page allocation table cache is no longer valid
		NBAK_TRACE(("Dropping alloc table"));
		delete alloc_table;
		alloc_table = NULL;
		last_allocated_page = 0;
		if (!allocLock->tryReleaseLock(tdbb))
			ERR_bugcheck_msg("There are holders of alloc_lock after end_backup finish");

		closeDelta();
		unlink(diff_name.c_str());

		NBAK_TRACE(("backup is over"));
		endLock.unlockWrite(tdbb);
	}
	catch (const Firebird::Exception&) {
		endLock.unlockWrite(tdbb);
		throw;
	}

	return;
}

bool BackupManager::actualizeAlloc(thread_db* tdbb)
{
	ISC_STATUS *status_vector = tdbb->tdbb_status_vector;
	try {
		NBAK_TRACE(("actualize_alloc last_allocated_page=%d alloc_table=%p",
			last_allocated_page, alloc_table));
		// For SuperServer this routine is really executed only at database startup when
		// it has exlock or when exclusive access to database is enabled
		if (!alloc_table)
			alloc_table = FB_NEW(*database->dbb_permanent) AllocItemTree(database->dbb_permanent);
		while (true)
		{
			BufferDesc temp_bdb;
			// Difference file pointer pages have one ULONG as number of pages allocated on the page and
			// then go physical numbers of pages from main database file. Offsets of numbers correspond
			// to difference file pages.

			// Get offset of pointer page. We can do so because page sizes are powers of 2
			temp_bdb.bdb_page = last_allocated_page & ~(database->dbb_page_size / sizeof(ULONG) - 1);
			temp_bdb.bdb_dbb = database;
			temp_bdb.bdb_buffer = reinterpret_cast<Ods::pag*>(alloc_buffer);

			if (!PIO_read(diff_file, &temp_bdb, temp_bdb.bdb_buffer, status_vector)) {
				return false;
			}
			for (ULONG i = last_allocated_page - temp_bdb.bdb_page.getPageNum(); i < alloc_buffer[0]; i++)
			{
				NBAK_TRACE(("alloc item page=%d, diff=%d", alloc_buffer[i + 1], temp_bdb.bdb_page.getPageNum() + i + 1));
				if (!alloc_table->add(AllocItem(alloc_buffer[i + 1], temp_bdb.bdb_page.getPageNum() + i + 1)))
				{
					database->dbb_flags |= DBB_bugcheck;
					ERR_build_status(status_vector,
						Arg::Gds(isc_bug_check) << Arg::Str("Duplicated item in allocation table detected"));
					return false;
				}
			}
			last_allocated_page = temp_bdb.bdb_page.getPageNum() + alloc_buffer[0];
			if (alloc_buffer[0] == database->dbb_page_size / sizeof(ULONG) - 1)
				last_allocated_page++;	// if page is full adjust position for next pointer page
			else
				break;	// We finished reading allocation table
		}
	}
	catch (const Firebird::Exception& ex) {
		// Handle out of memory error, etc
		delete alloc_table;
		Firebird::stuff_exception(status_vector, ex);
		alloc_table = NULL;
		last_allocated_page = 0;
		return false;
	}
	return true;
}

// Return page index in difference file that can be used in
// writeDifference call later.
ULONG BackupManager::getPageIndex(thread_db* tdbb, ULONG db_page)
{
	NBAK_TRACE(("get_page_index"));

	AllocItemTree::Accessor a(alloc_table);
	ULONG diff_page = a.locate(db_page) ? a.current().diff_page : 0;

	return diff_page;
}

// Mark next difference page as used by some database page
ULONG BackupManager::allocateDifferencePage(thread_db* tdbb, ULONG db_page)
{
	// This page may be allocated by other
	if (ULONG diff_page = getPageIndex(tdbb, db_page)) {
		return diff_page;
	}

	NBAK_TRACE(("allocate_difference_page"));
	fb_assert(last_allocated_page % (database->dbb_page_size / sizeof(ULONG)) == alloc_buffer[0]);

	ISC_STATUS* status_vector = tdbb->tdbb_status_vector;
	// Grow file first. This is done in such order to keep difference
	// file consistent in case of write error. We should always be able
	// to read next alloc page when previous one is full.
	BufferDesc temp_bdb;
	temp_bdb.bdb_page = last_allocated_page + 1;
	temp_bdb.bdb_dbb = database;
	temp_bdb.bdb_buffer = reinterpret_cast<Ods::pag*>(empty_buffer);
	if (!PIO_write(diff_file, &temp_bdb, (Ods::pag*)empty_buffer, status_vector)) {
		return 0;
	}

	const bool alloc_page_full = alloc_buffer[0] == database->dbb_page_size / sizeof(ULONG) - 2;
	if (alloc_page_full) {
		// Pointer page is full. Its time to create new one.
		temp_bdb.bdb_page = last_allocated_page + 2;
		temp_bdb.bdb_dbb = database;
		temp_bdb.bdb_buffer = reinterpret_cast<Ods::pag*>(empty_buffer);
		if (!PIO_write(diff_file, &temp_bdb, (Ods::pag*)empty_buffer, status_vector)) {
			return 0;
		}
	}

	// Write new item to the allocation table
	temp_bdb.bdb_page = last_allocated_page & ~(database->dbb_page_size / sizeof(ULONG) - 1);
	temp_bdb.bdb_dbb = database;
	temp_bdb.bdb_buffer = reinterpret_cast<Ods::pag*>(alloc_buffer);
	alloc_buffer[++alloc_buffer[0]] = db_page;
	if (!PIO_write(diff_file, &temp_bdb, temp_bdb.bdb_buffer, status_vector)) {
		return 0;
	}
	last_allocated_page++;
	// Register new page in the alloc table
	try {
		alloc_table->add(AllocItem(db_page, last_allocated_page));
	}
	catch (const Firebird::Exception& ex) {
		// Handle out of memory error
		delete alloc_table;
		alloc_table = NULL;
		last_allocated_page = 0;
		Firebird::stuff_exception(status_vector, ex);
		return 0;
	}
	// Adjust buffer and counters if we allocated new alloc page earlier
	if (alloc_page_full) {
		last_allocated_page++;
		memset(alloc_buffer, 0, database->dbb_page_size);
		return last_allocated_page - 1;
	}

	return last_allocated_page;
}

bool BackupManager::writeDifference(ISC_STATUS* status, ULONG diff_page, Ods::pag* page)
{
	if (!diff_page)
	{
		// We should never be here but if it happens let not overwrite first allocation
		// page with garbage.

		(Arg::Gds(isc_random) << "Can't allocate difference page").copyTo(status);
		return false;
	}

	NBAK_TRACE(("write_diff"));
	BufferDesc temp_bdb;
	temp_bdb.bdb_page = diff_page;
	temp_bdb.bdb_dbb = database;
	temp_bdb.bdb_buffer = page;
	// Check that diff page is not allocation page
	fb_assert(diff_page % (database->dbb_page_size / sizeof(ULONG)));
	if (!PIO_write(diff_file, &temp_bdb, page, status))
		return false;
	return true;
}

bool BackupManager::readDifference(thread_db* tdbb, ULONG diff_page, Ods::pag* page)
{
	NBAK_TRACE(("read_diff"));
	BufferDesc temp_bdb;
	temp_bdb.bdb_page = diff_page;
	temp_bdb.bdb_dbb = database;
	temp_bdb.bdb_buffer = page;
	if (!PIO_read(diff_file, &temp_bdb, page, tdbb->tdbb_status_vector))
		return false;
	return true;
}

void BackupManager::flushDifference()
{
	PIO_flush(database, diff_file);
}

BackupManager::BackupManager(thread_db* tdbb, Database* _database, int ini_state) :
	dbCreating(false), database(_database), diff_file(NULL), alloc_table(NULL),
	last_allocated_page(0), current_scn(0), diff_name(*_database->dbb_permanent),
	explicit_diff_name(false), flushInProgress(false), shuttedDown(false), 
	stateLock(FB_NEW(*database->dbb_permanent) NBackupStateLock(tdbb, *database->dbb_permanent, this)),
	allocLock(FB_NEW(*database->dbb_permanent) NBackupAllocLock(tdbb, *database->dbb_permanent, this))
{
	// Allocate various database page buffers needed for operation
	temp_buffers_space = FB_NEW(*database->dbb_permanent) BYTE[database->dbb_page_size * 3 + MIN_PAGE_SIZE];
	// Align it at sector boundary for faster IO (also guarantees correct alignment for ULONG later)
	BYTE *temp_buffers = reinterpret_cast<BYTE*>(
		FB_ALIGN(reinterpret_cast<U_IPTR>(temp_buffers_space), MIN_PAGE_SIZE));
	memset(temp_buffers, 0, database->dbb_page_size * 3);

	backup_state = ini_state;

	empty_buffer = reinterpret_cast<ULONG*>(temp_buffers);
	spare_buffer = reinterpret_cast<ULONG*>(temp_buffers + database->dbb_page_size);
	alloc_buffer = reinterpret_cast<ULONG*>(temp_buffers + database->dbb_page_size * 2);

	NBAK_TRACE(("Create BackupManager, database=%s", database->dbb_filename.c_str()));
}

BackupManager::~BackupManager()
{
	delete stateLock;
	delete allocLock;
	delete alloc_table;
	delete[] temp_buffers_space;
}

void BackupManager::setDifference(thread_db* tdbb, const char* filename)
{
	SET_TDBB(tdbb);

	if (filename) {
		WIN window(HEADER_PAGE_NUMBER);
		Ods::header_page* header =
			(Ods::header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
		CCH_MARK_MUST_WRITE(tdbb, &window);
		PAG_replace_entry_first(tdbb, header, Ods::HDR_difference_file,
			strlen(filename), reinterpret_cast<const UCHAR*>(filename));
		CCH_RELEASE(tdbb, &window);
		diff_name = filename;
		explicit_diff_name = true;
	}
	else {
		PAG_delete_clump_entry(tdbb, HEADER_PAGE, Ods::HDR_difference_file);
		generateFilename();
	}
}

bool BackupManager::actualizeState(thread_db* tdbb)
{
	// State is unknown. We need to read it from the disk.
	// We cannot use CCH for this because of likely recursion.
	NBAK_TRACE(("actualizeState: current_state=%i", backup_state));

	if (dbCreating) {
		backup_state = nbak_state_normal;
		return true;
	}

	SET_TDBB(tdbb);

	ISC_STATUS *status = tdbb->tdbb_status_vector;

	// Read original page from database file or shadows.
	SSHORT retryCount = 0;
	Ods::header_page* header = reinterpret_cast<Ods::header_page*>(spare_buffer);
	BufferDesc temp_bdb;
	temp_bdb.bdb_page = HEADER_PAGE_NUMBER;
	temp_bdb.bdb_dbb = database;
	temp_bdb.bdb_buffer = reinterpret_cast<Ods::pag*>(header);
	PageSpace* pageSpace = database->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	fb_assert(pageSpace);
	jrd_file* file = pageSpace->file;
	while (!PIO_read(file, &temp_bdb, temp_bdb.bdb_buffer, status))
	{
		if (!CCH_rollover_to_shadow(tdbb, database, file, false)) {
			NBAK_TRACE(("Shadow change error"));
			return false;
		}
		if (file != pageSpace->file)
			file = pageSpace->file;
		else {
			if (retryCount++ == 3) {
				NBAK_TRACE(("IO error"));
				return false;
			}
		}
	}

	const int new_backup_state = (database->dbb_ods_version >= ODS_VERSION11) ?
		header->hdr_flags & Ods::hdr_backup_mask : nbak_state_normal;
	NBAK_TRACE(("backup state read from header is %d", new_backup_state));
	// Check is we missed lock/unlock cycle and need to invalidate
	// our allocation table and file handle
	const bool missed_cycle = (header->hdr_header.pag_scn - current_scn) > 1;
	current_scn = header->hdr_header.pag_scn;

	// Read difference file name from header clumplets
	explicit_diff_name = false;
	const UCHAR* p = header->hdr_data;
	while (true) {
		switch (*p)
		{
		case Ods::HDR_backup_guid:
			p += p[1] + 2;
			continue;
		case Ods::HDR_difference_file:
			explicit_diff_name = true;
			diff_name.assign(reinterpret_cast<const char*>(p + 2), p[1]);
		}
		break;
	}
	if (!explicit_diff_name)
		generateFilename();

	if (new_backup_state == nbak_state_normal || missed_cycle)
	{
		// Page allocation table cache is no longer valid.
		if (alloc_table) {
			NBAK_TRACE(("Dropping alloc table"));
			delete alloc_table;
			alloc_table = NULL;
			last_allocated_page = 0;
			if (!allocLock->tryReleaseLock(tdbb))
				ERR_bugcheck_msg("There are holders of alloc_lock after end_backup finish");
		}
	}

	if (new_backup_state != nbak_state_normal && !diff_file)
		openDelta();
	// Adjust state at the very and to ensure proper error handling
	backup_state = new_backup_state;

	return true;
}

void BackupManager::shutdown(thread_db* tdbb)
{
	shuttedDown = true;

	closeDelta();
	stateLock->shutdownLock();
	allocLock->shutdownLock();
}
