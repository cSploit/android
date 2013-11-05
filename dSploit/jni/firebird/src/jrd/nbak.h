/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		nbak.h
 *	DESCRIPTION:	Incremental backup interface definitions
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
 *
 */

#ifndef JRD_NBAK_H
#define JRD_NBAK_H

#include "../common/classes/tree.h"
#include "../common/classes/rwlock.h"
#include "../common/classes/alloc.h"
#include "../common/classes/fb_string.h"
#include "GlobalRWLock.h"
#include "../jrd/err_proto.h"

// Uncomment this line if you need to trace backup-related activity
//#define NBAK_DEBUG

#ifdef NBAK_DEBUG
DEFINE_TRACE_ROUTINE(nbak_trace);
#define NBAK_TRACE(args) nbak_trace args
#define NBAK_TRACE_AST(message) gds__trace(message)
#else
#define NBAK_TRACE(args) /* nothing */
#define NBAK_TRACE_AST(message) /* nothing */
#endif

namespace Ods {
	struct pag;
}

namespace Jrd {

class Lock;
class Record;
class thread_db;
class Database;
class jrd_file;

class AllocItem
{
public:
	ULONG db_page; // Page number in the main database file
	ULONG diff_page; // Page number in the difference file
	//Record* rec_data;
	static const ULONG& generate(const void* /*sender*/, const AllocItem& item)
	{
		return item.db_page;
	}
	AllocItem() {}
	AllocItem(ULONG db_pageL, ULONG diff_pageL)
	{
		this->db_page = db_pageL;
		this->diff_page = diff_pageL;
	}
};

typedef Firebird::BePlusTree<AllocItem, ULONG, MemoryPool, AllocItem> AllocItemTree;

// Class to synchronize access to backup state

class NBackupStateLock: public GlobalRWLock
{
public:
	NBackupStateLock(thread_db* tdbb, MemoryPool& p, BackupManager* bakMan);
	virtual ~NBackupStateLock() { }

protected:
	BackupManager *backup_manager;
	virtual void blockingAstHandler(thread_db* tdbb);
	virtual bool fetch(thread_db* tdbb);
	virtual void invalidate(thread_db* tdbb);
};

// Class to synchronize access to diff page allocation table
class NBackupAllocLock: public GlobalRWLock
{
public:
	NBackupAllocLock(thread_db* tdbb, MemoryPool& p, BackupManager* bakMan);
	virtual ~NBackupAllocLock() { }

protected:
	BackupManager* backup_manager;
	virtual bool fetch(thread_db* tdbb);

	virtual void invalidate(thread_db* tdbb)
	{
		GlobalRWLock::invalidate(tdbb);
		NBAK_TRACE( ("invalidate alloc table allocLock(%p)", this) );
	}
};

// Note this flags MUST correspond with backup mask in ods.h
const USHORT nbak_state_normal	= 0x000;	// Normal mode. Changes are simply written to main files
const USHORT nbak_state_stalled	= 0x400;	// 1024 Main files are locked. Changes are written to diff file
const USHORT nbak_state_merge	= 0x800;	// 2048 Merging changes from diff file into main files
const USHORT nbak_state_unknown	= USHORT(~0);	// State is unknown. Needs to be read from disk

/*
 *  The functional responsibilities of NBAK are:
 *  1. to redirect writes to difference files when asked (ALTER DATABASE BEGIN
 *  BACKUP statement)
 *  2. to produce a GUID for the database snapshot and write it into the database
 *  header before the ALTER DATABASE BEGIN BACKUP statement returns
 *  3. to merge differences into the database when asked (ALTER DATABASE END BACKUP
 *  statement)
 *  4. to mark pages written by the engine with the current SCN [System Change
 *  Number] counter value for the database
 *  5. to increment SCN on each change of backup state
 *
 *  The backup state cycle is:
 *  nbak_state_normal -> nbak_state_stalled -> nbak_state_merge -> nbak_state_normal
 *  - In normal state writes go directly to the main database files.
 *  - In stalled state writes go to the difference file only and the main files are
 *  read-only.
 *  - In merge state new pages are not allocated from difference files. Writes go to
 *  the main database files. Reads of mapped pages compare both page versions and
 *  return the version which is fresher, because we don't know if it is merged or not.
 *  Merged pages are written only in database file.
 *
 *  For synchronization NBAK uses 3 lock types via Firebird::GlobalRWLock:
 *  LCK_backup_database, LCK_backup_alloc, LCK_backup_end.
 *
 *  LCK_backup_database protects "clean" state of database. Database is meant to be
 *  clean when it has no dirty or fetched pages. When attachment needs to fake, to fetch or to mark a page as dirty
 *  (via CCH_mark) it needs to obtain READ (LCK_PR) lock of this kind. WRITE
 *  (LCK_EX) lock forces flush of all caches leaving no dirty pages to be written to
 *  database.
 *
 *  Modification process of a page is as follows:
 *  CCH_fetch -> CCH_mark -> CCH_release -> write_page
 *
 *  The dirty page is owned by the DATABASE between setting up and clearing the page dirty flag until write_page happens.
 *
 *  The page lock is requested every time under LCK_backup_database to prevent any deadlocks. So fecthed page is
 *  owned by the ATTACHMENT between CCH_FETCH_LOCK, CCH_FAKE and CCH_RELEASE.
 *  AST on LCK_backup_database forces all dirty pages owned by DATABASE to be
 *  written to disk via write_page. Since finalizing processing of the pages owned
 *  by attachment may require taking new locks BDB_must_write is set for them to
 *  ensure that LCK_backup_database lock is released as soon as possible.
 *
 *  To change backup state, engine takes LCK_backup_database lock first, forcing all
 *  dirty pages to the disk, modifies the header page reflecting the state change,
 *  and releases the lock allowing transaction processing to continue.
 *
 *  LCK_backup_alloc is used to protect mapping table between difference file
 *  (.delta) and the database. To add new page to the mapping attachment needs to
 *  take WRITE lock of this kind. READ lock is necessary to read the table.
 *
 *  LCK_backup_end is used to ensure reliable execution of state transition from
 *  nbak_state_merge to nbak_state_normal (MERGE process). Taking of WRITE (LCK_EX)
 *  lock of this kind is needed to perform the MERGE. Every new attachment attempts
 *  to finalize incomplete merge if the database is in nbak_state_merge mode and
 *  this lock is not taken.
 */


class BackupManager
{
public:
	class StateWriteGuard
	{
	public:
		StateWriteGuard(thread_db* _tdbb, WIN* wnd);
		~StateWriteGuard();

		void releaseHeader();
		void setSuccess()
		{
			success = true;
		}

	private:
		// copying is prohibited
		StateWriteGuard(const StateWriteGuard&);
		StateWriteGuard& operator=(const StateWriteGuard&);

		thread_db* tdbb;
		WIN* window;
		bool success;
	};

	class StateReadGuard
	{
	public:
		explicit StateReadGuard(thread_db* _tdbb) : tdbb(_tdbb)
		{
			lock(tdbb, LCK_WAIT);
		}

		~StateReadGuard()
		{
			unlock(tdbb);
		}

		static bool lock(thread_db* tdbb, SSHORT wait)
		{
			Attachment* att = tdbb->getAttachment();
			Database* dbb = tdbb->getDatabase();

			const bool ok = att ? 
				att->backupStateReadLock(tdbb, wait) :
				dbb->dbb_backup_manager->lockStateRead(tdbb, wait);

			if (!ok)
				ERR_bugcheck_msg("Can't lock state for read");

			return ok;
		}

		static void unlock(thread_db* tdbb)
		{
			Attachment* att = tdbb->getAttachment();
			Database* dbb = tdbb->getDatabase();

			if (att)
				att->backupStateReadUnLock(tdbb);
			else
				dbb->dbb_backup_manager->unlockStateRead(tdbb);
		}

	private:
		// copying is prohibited
		StateReadGuard(const StateReadGuard&);
		StateReadGuard& operator=(const StateReadGuard&);

		thread_db* tdbb;
	};

	class AllocWriteGuard
	{
	public:
		AllocWriteGuard(thread_db* _tdbb, BackupManager* _backupManager)
			: tdbb(_tdbb), backupManager(_backupManager)
		{
			backupManager->lockAllocWrite(tdbb);
		}

		~AllocWriteGuard()
		{
			backupManager->unlockAllocWrite(tdbb);
		}

	private:
		// copying is prohibited
		AllocWriteGuard(const AllocWriteGuard&);
		AllocWriteGuard& operator=(const AllocWriteGuard&);

		thread_db* tdbb;
		BackupManager* backupManager;
	};

	class AllocReadGuard
	{
	public:
		AllocReadGuard(thread_db* _tdbb, BackupManager* _backupManager)
			: tdbb(_tdbb), backupManager(_backupManager)
		{
			backupManager->lockAllocRead(tdbb);
		}

		~AllocReadGuard()
		{
			backupManager->unlockAllocRead(tdbb);
		}

	private:
		// copying is prohibited
		AllocReadGuard(const AllocReadGuard&);
		AllocReadGuard& operator=(const AllocReadGuard&);

		thread_db* tdbb;
		BackupManager* backupManager;
	};

	// Set when db is creating. Default = false
	bool dbCreating;

	BackupManager(thread_db* tdbb, Database* _database, int ini_state);
	~BackupManager();

	// Set difference file name in header.
	// State must be locked and equal to nbak_state_normal to call this method
	void setDifference(thread_db* tdbb, const char* filename);

	// Return current backup state
	USHORT getState() const
	{
		return backup_state;
	}

	void setState(const USHORT newState)
	{
		backup_state = newState;
	}

	// Return current SCN for database
	ULONG getCurrentSCN() const
	{
		return current_scn;
	}

	// Initialize and open difference file for writing
	void beginBackup(thread_db* tdbb);

	// Merge difference file to main files (if needed) and unlink() difference
	// file then. If merge is already in progress method silently returns false and
	// does nothing (so it can be used for recovery on database startup).
	void endBackup(thread_db* tdbb, bool recover);

	// State Lock member functions
	bool lockStateWrite(thread_db* tdbb, SSHORT wait)
	{
		tdbb->tdbb_flags |= TDBB_backup_write_locked;
		return stateLock->lockWrite(tdbb, wait);
	}

	void unlockStateWrite(thread_db* tdbb)
	{
		tdbb->tdbb_flags &= ~TDBB_backup_write_locked;
		stateLock->unlockWrite(tdbb);
	}

	bool lockStateRead(thread_db* tdbb, SSHORT wait)
	{
		if ( !(tdbb->tdbb_flags & TDBB_backup_write_locked))
			return stateLock->lockRead(tdbb, wait);
		return true;
	}

	void unlockStateRead(thread_db* tdbb)
	{
		if ( !(tdbb->tdbb_flags & TDBB_backup_write_locked))
			stateLock->unlockRead(tdbb);
	}

	void lockDirtyPage(thread_db* tdbb)
	{
		if (tdbb->tdbb_flags & TDBB_backup_write_locked)
			return;
		if (!stateLock->lockRead(tdbb, true, true))
			ERR_bugcheck_msg("Can't lock backup state to set dirty flag");
	}

	void unlockDirtyPage(thread_db* tdbb)
	{
		if (tdbb->tdbb_flags & TDBB_backup_write_locked)
			return;
		unlockStateRead(tdbb);
	}

	bool actualizeState(thread_db* tdbb);
	bool actualizeAlloc(thread_db* tdbb);

	// Return page index in difference file that can be used in
	// writeDifference call later.
	ULONG getPageIndex(thread_db* tdbb, ULONG db_page);

	// Return next page index in the difference file to be allocated
	ULONG allocateDifferencePage(thread_db* tdbb, ULONG db_page);

	// Must have ISC_STATUS because it is called from write_page
	void openDelta();
	void closeDelta();
	bool writeDifference(ISC_STATUS* status, ULONG diff_page, Ods::pag* page);
	bool readDifference(thread_db* tdbb, ULONG diff_page, Ods::pag* page);
	void flushDifference();

	void shutdown(thread_db* tdbb);

	void beginFlush()
	{
		flushInProgress = true;
	}

	void endFlush()
	{
		flushInProgress = false;
	}

	bool databaseFlushInProgress() const
	{
		return flushInProgress;
	}

	bool isShuttedDown() const
	{
		return shuttedDown;
	}

	// Get size (in pages) of locked database file
	ULONG getPageCount();
private:
	Database* database;
	jrd_file* diff_file;
	AllocItemTree* alloc_table; // Cached allocation table of pages in difference file
	USHORT backup_state;
	ULONG last_allocated_page; // Last physical page allocated in the difference file
	BYTE *temp_buffers_space;
	ULONG *alloc_buffer, *empty_buffer, *spare_buffer;
	ULONG current_scn;
	Firebird::PathName diff_name;
	bool explicit_diff_name;
	bool flushInProgress;
	bool shuttedDown;

	NBackupStateLock* stateLock;
	NBackupAllocLock* allocLock;

	void generateFilename();

	void lockAllocWrite(thread_db* tdbb)
	{
		if (!allocLock->lockWrite(tdbb, true))
			ERR_bugcheck_msg("Can't lock alloc table for writing");
	}

	void unlockAllocWrite(thread_db* tdbb)
	{
		allocLock->unlockWrite(tdbb);
	}

	void lockAllocRead(thread_db* tdbb)
	{
		if (!allocLock->lockRead(tdbb, true))
			ERR_bugcheck_msg("Can't lock alloc table for reading");

	}

	void unlockAllocRead(thread_db* tdbb)
	{
		allocLock->unlockRead(tdbb);
	}
};


} //namespace Jrd

#endif /* JRD_NBAK_H */
