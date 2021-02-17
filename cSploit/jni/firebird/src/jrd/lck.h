/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		lck.h
 *	DESCRIPTION:	Lock hander definitions (NOT lock manager)
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

#ifndef JRD_LCK_H
#define JRD_LCK_H

//#define DEBUG_LCK

#ifdef DEBUG_LCK
#include "../common/classes/SyncObject.h"
#endif

#include "../jrd/EngineInterface.h"

namespace Jrd {

class Database;
class Attachment;
class thread_db;

// Lock types

enum lck_t {
	LCK_database = 1,			// Root of lock tree
	LCK_relation,				// Individual relation lock
	LCK_bdb,					// Individual buffer block
	LCK_tra,					// Individual transaction lock
	LCK_rel_exist,				// Relation existence lock
	LCK_idx_exist,				// Index existence lock
	LCK_attachment,				// Attachment lock
	LCK_shadow,					// Lock to synchronize addition of shadows
	LCK_sweep,					// Sweep lock for single sweeper
	LCK_expression,				// Expression index caching mechanism
	LCK_prc_exist,				// Procedure existence lock
	LCK_update_shadow,			// shadow update sync lock
	LCK_backup_alloc,           // Lock for page allocation table in backup spare file
	LCK_backup_database,        // Lock to protect writing to database file
	LCK_backup_end,				// Lock to protect end_backup consistency
	LCK_rel_partners,			// Relation partners lock
	LCK_page_space,				// Page space ID lock
	LCK_dsql_cache,				// DSQL cache lock
	LCK_monitor,				// Lock to dump the monitoring data
	LCK_tt_exist,				// TextType existence lock
	LCK_cancel,					// Cancellation lock
	LCK_btr_dont_gc,			// Prevent removal of b-tree page from index
	LCK_shared_counter,			// Database-wide shared counter
	LCK_tra_pc,					// Precommitted transaction lock
	LCK_fun_exist,				// Function existence lock
	LCK_rel_rescan,				// Relation forced rescan lock
	LCK_crypt,					// Crypt lock for single crypt thread
	LCK_crypt_status			// Notifies about changed database encryption status
};

// Lock owner types

enum lck_owner_t {
	LCK_OWNER_database = 1,		// A database is the owner of the lock
	LCK_OWNER_attachment		// An attachment is the owner of the lock
};

class Lock : public pool_alloc_rpt<UCHAR, type_lck>
{
public:
	Lock(thread_db* tdbb, USHORT length, lck_t type, void* object = NULL, lock_ast_t ast = NULL);

	Lock* detach();

	Firebird::RefPtr<JAttachment> getLockInterface()
	{
		return lck_attachment;
	}

	Attachment* getLockAttachment()
	{
		return lck_attachment ? lck_attachment->getHandle() : NULL;
	}

	void setLockAttachment(thread_db* tdbb, Attachment* att);

#ifdef DEBUG_LCK
	Firebird::SyncObject	lck_sync;
#endif

	Database* lck_dbb;				// Database object is contained in

private:
	Firebird::RefPtr<JAttachment> lck_attachment;		// Attachment that owns lock, set only using set_lock_attachment()

public:
	void* lck_compatible;			// Enter into internal_enqueue() and treat as compatible
	void* lck_compatible2;			// Sub-level for internal compatibility

	lock_ast_t lck_ast;				// Blocking AST routine
	void* lck_object;				// Argument to be passed to AST

//private:
	Lock* lck_next;					// lck_next and lck_prior form a doubly linked list of locks
	Lock* lck_prior;				// bound to attachment

	Lock* lck_collision;			// Collisions in compatibility table
	Lock* lck_identical;			// Identical locks in compatibility table

	SLONG lck_id;					// Lock id from the lock manager
	SLONG lck_owner_handle;			// Lock owner handle from the lock manager's point of view
	USHORT lck_length;				// Length of lock key string
	lck_t lck_type;					// Lock type

public:
	UCHAR lck_logical;				// Logical lock level
	UCHAR lck_physical;				// Physical lock level
	SLONG lck_data;					// Data associated with a lock

	union
	{
		UCHAR lck_string[1];
		SLONG lck_long;
	} lck_key;						// Lock key string

	UCHAR lck_tail[1];				// Makes the allocator happy
};

} // namespace Jrd

#endif // JRD_LCK_H
