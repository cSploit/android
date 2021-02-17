/*-------------------------------------------------------------------------
 *
 * lmgr.c
 *	  POSTGRES lock manager code
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/storage/lmgr/lmgr.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/subtrans.h"
#include "access/transam.h"
#include "access/xact.h"
#include "catalog/catalog.h"
#include "miscadmin.h"
#include "storage/lmgr.h"
#include "storage/procarray.h"
#include "utils/inval.h"


/*
 * RelationInitLockInfo
 *		Initializes the lock information in a relation descriptor.
 *
 *		relcache.c must call this during creation of any reldesc.
 */
void
RelationInitLockInfo(Relation relation)
{
	Assert(RelationIsValid(relation));
	Assert(OidIsValid(RelationGetRelid(relation)));

	relation->rd_lockInfo.lockRelId.relId = RelationGetRelid(relation);

	if (relation->rd_rel->relisshared)
		relation->rd_lockInfo.lockRelId.dbId = InvalidOid;
	else
		relation->rd_lockInfo.lockRelId.dbId = MyDatabaseId;
}

/*
 * SetLocktagRelationOid
 *		Set up a locktag for a relation, given only relation OID
 */
static inline void
SetLocktagRelationOid(LOCKTAG *tag, Oid relid)
{
	Oid			dbid;

	if (IsSharedRelation(relid))
		dbid = InvalidOid;
	else
		dbid = MyDatabaseId;

	SET_LOCKTAG_RELATION(*tag, dbid, relid);
}

/*
 *		LockRelationOid
 *
 * Lock a relation given only its OID.	This should generally be used
 * before attempting to open the relation's relcache entry.
 */
void
LockRelationOid(Oid relid, LOCKMODE lockmode)
{
	LOCKTAG		tag;
	LockAcquireResult res;

	SetLocktagRelationOid(&tag, relid);

	res = LockAcquire(&tag, lockmode, false, false);

	/*
	 * Now that we have the lock, check for invalidation messages, so that we
	 * will update or flush any stale relcache entry before we try to use it.
	 * We can skip this in the not-uncommon case that we already had the same
	 * type of lock being requested, since then no one else could have
	 * modified the relcache entry in an undesirable way.  (In the case where
	 * our own xact modifies the rel, the relcache update happens via
	 * CommandCounterIncrement, not here.)
	 */
	if (res != LOCKACQUIRE_ALREADY_HELD)
		AcceptInvalidationMessages();
}

/*
 *		ConditionalLockRelationOid
 *
 * As above, but only lock if we can get the lock without blocking.
 * Returns TRUE iff the lock was acquired.
 *
 * NOTE: we do not currently need conditional versions of all the
 * LockXXX routines in this file, but they could easily be added if needed.
 */
bool
ConditionalLockRelationOid(Oid relid, LOCKMODE lockmode)
{
	LOCKTAG		tag;
	LockAcquireResult res;

	SetLocktagRelationOid(&tag, relid);

	res = LockAcquire(&tag, lockmode, false, true);

	if (res == LOCKACQUIRE_NOT_AVAIL)
		return false;

	/*
	 * Now that we have the lock, check for invalidation messages; see notes
	 * in LockRelationOid.
	 */
	if (res != LOCKACQUIRE_ALREADY_HELD)
		AcceptInvalidationMessages();

	return true;
}

/*
 *		UnlockRelationId
 *
 * Unlock, given a LockRelId.  This is preferred over UnlockRelationOid
 * for speed reasons.
 */
void
UnlockRelationId(LockRelId *relid, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_RELATION(tag, relid->dbId, relid->relId);

	LockRelease(&tag, lockmode, false);
}

/*
 *		UnlockRelationOid
 *
 * Unlock, given only a relation Oid.  Use UnlockRelationId if you can.
 */
void
UnlockRelationOid(Oid relid, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SetLocktagRelationOid(&tag, relid);

	LockRelease(&tag, lockmode, false);
}

/*
 *		LockRelation
 *
 * This is a convenience routine for acquiring an additional lock on an
 * already-open relation.  Never try to do "relation_open(foo, NoLock)"
 * and then lock with this.
 */
void
LockRelation(Relation relation, LOCKMODE lockmode)
{
	LOCKTAG		tag;
	LockAcquireResult res;

	SET_LOCKTAG_RELATION(tag,
						 relation->rd_lockInfo.lockRelId.dbId,
						 relation->rd_lockInfo.lockRelId.relId);

	res = LockAcquire(&tag, lockmode, false, false);

	/*
	 * Now that we have the lock, check for invalidation messages; see notes
	 * in LockRelationOid.
	 */
	if (res != LOCKACQUIRE_ALREADY_HELD)
		AcceptInvalidationMessages();
}

/*
 *		ConditionalLockRelation
 *
 * This is a convenience routine for acquiring an additional lock on an
 * already-open relation.  Never try to do "relation_open(foo, NoLock)"
 * and then lock with this.
 */
bool
ConditionalLockRelation(Relation relation, LOCKMODE lockmode)
{
	LOCKTAG		tag;
	LockAcquireResult res;

	SET_LOCKTAG_RELATION(tag,
						 relation->rd_lockInfo.lockRelId.dbId,
						 relation->rd_lockInfo.lockRelId.relId);

	res = LockAcquire(&tag, lockmode, false, true);

	if (res == LOCKACQUIRE_NOT_AVAIL)
		return false;

	/*
	 * Now that we have the lock, check for invalidation messages; see notes
	 * in LockRelationOid.
	 */
	if (res != LOCKACQUIRE_ALREADY_HELD)
		AcceptInvalidationMessages();

	return true;
}

/*
 *		UnlockRelation
 *
 * This is a convenience routine for unlocking a relation without also
 * closing it.
 */
void
UnlockRelation(Relation relation, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_RELATION(tag,
						 relation->rd_lockInfo.lockRelId.dbId,
						 relation->rd_lockInfo.lockRelId.relId);

	LockRelease(&tag, lockmode, false);
}

/*
 *		LockHasWaitersRelation
 *
 * This is a functiion to check if someone else is waiting on a
 * lock, we are currently holding.
 */
bool
LockHasWaitersRelation(Relation relation, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_RELATION(tag,
						 relation->rd_lockInfo.lockRelId.dbId,
						 relation->rd_lockInfo.lockRelId.relId);

	return LockHasWaiters(&tag, lockmode, false);
}

/*
 *		LockRelationIdForSession
 *
 * This routine grabs a session-level lock on the target relation.	The
 * session lock persists across transaction boundaries.  It will be removed
 * when UnlockRelationIdForSession() is called, or if an ereport(ERROR) occurs,
 * or if the backend exits.
 *
 * Note that one should also grab a transaction-level lock on the rel
 * in any transaction that actually uses the rel, to ensure that the
 * relcache entry is up to date.
 */
void
LockRelationIdForSession(LockRelId *relid, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_RELATION(tag, relid->dbId, relid->relId);

	(void) LockAcquire(&tag, lockmode, true, false);
}

/*
 *		UnlockRelationIdForSession
 */
void
UnlockRelationIdForSession(LockRelId *relid, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_RELATION(tag, relid->dbId, relid->relId);

	LockRelease(&tag, lockmode, true);
}

/*
 *		LockRelationForExtension
 *
 * This lock tag is used to interlock addition of pages to relations.
 * We need such locking because bufmgr/smgr definition of P_NEW is not
 * race-condition-proof.
 *
 * We assume the caller is already holding some type of regular lock on
 * the relation, so no AcceptInvalidationMessages call is needed here.
 */
void
LockRelationForExtension(Relation relation, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_RELATION_EXTEND(tag,
								relation->rd_lockInfo.lockRelId.dbId,
								relation->rd_lockInfo.lockRelId.relId);

	(void) LockAcquire(&tag, lockmode, false, false);
}

/*
 *		UnlockRelationForExtension
 */
void
UnlockRelationForExtension(Relation relation, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_RELATION_EXTEND(tag,
								relation->rd_lockInfo.lockRelId.dbId,
								relation->rd_lockInfo.lockRelId.relId);

	LockRelease(&tag, lockmode, false);
}

/*
 *		LockPage
 *
 * Obtain a page-level lock.  This is currently used by some index access
 * methods to lock individual index pages.
 */
void
LockPage(Relation relation, BlockNumber blkno, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_PAGE(tag,
					 relation->rd_lockInfo.lockRelId.dbId,
					 relation->rd_lockInfo.lockRelId.relId,
					 blkno);

	(void) LockAcquire(&tag, lockmode, false, false);
}

/*
 *		ConditionalLockPage
 *
 * As above, but only lock if we can get the lock without blocking.
 * Returns TRUE iff the lock was acquired.
 */
bool
ConditionalLockPage(Relation relation, BlockNumber blkno, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_PAGE(tag,
					 relation->rd_lockInfo.lockRelId.dbId,
					 relation->rd_lockInfo.lockRelId.relId,
					 blkno);

	return (LockAcquire(&tag, lockmode, false, true) != LOCKACQUIRE_NOT_AVAIL);
}

/*
 *		UnlockPage
 */
void
UnlockPage(Relation relation, BlockNumber blkno, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_PAGE(tag,
					 relation->rd_lockInfo.lockRelId.dbId,
					 relation->rd_lockInfo.lockRelId.relId,
					 blkno);

	LockRelease(&tag, lockmode, false);
}

/*
 *		LockTuple
 *
 * Obtain a tuple-level lock.  This is used in a less-than-intuitive fashion
 * because we can't afford to keep a separate lock in shared memory for every
 * tuple.  See heap_lock_tuple before using this!
 */
void
LockTuple(Relation relation, ItemPointer tid, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_TUPLE(tag,
					  relation->rd_lockInfo.lockRelId.dbId,
					  relation->rd_lockInfo.lockRelId.relId,
					  ItemPointerGetBlockNumber(tid),
					  ItemPointerGetOffsetNumber(tid));

	(void) LockAcquire(&tag, lockmode, false, false);
}

/*
 *		ConditionalLockTuple
 *
 * As above, but only lock if we can get the lock without blocking.
 * Returns TRUE iff the lock was acquired.
 */
bool
ConditionalLockTuple(Relation relation, ItemPointer tid, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_TUPLE(tag,
					  relation->rd_lockInfo.lockRelId.dbId,
					  relation->rd_lockInfo.lockRelId.relId,
					  ItemPointerGetBlockNumber(tid),
					  ItemPointerGetOffsetNumber(tid));

	return (LockAcquire(&tag, lockmode, false, true) != LOCKACQUIRE_NOT_AVAIL);
}

/*
 *		UnlockTuple
 */
void
UnlockTuple(Relation relation, ItemPointer tid, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_TUPLE(tag,
					  relation->rd_lockInfo.lockRelId.dbId,
					  relation->rd_lockInfo.lockRelId.relId,
					  ItemPointerGetBlockNumber(tid),
					  ItemPointerGetOffsetNumber(tid));

	LockRelease(&tag, lockmode, false);
}

/*
 *		XactLockTableInsert
 *
 * Insert a lock showing that the given transaction ID is running ---
 * this is done when an XID is acquired by a transaction or subtransaction.
 * The lock can then be used to wait for the transaction to finish.
 */
void
XactLockTableInsert(TransactionId xid)
{
	LOCKTAG		tag;

	SET_LOCKTAG_TRANSACTION(tag, xid);

	(void) LockAcquire(&tag, ExclusiveLock, false, false);
}

/*
 *		XactLockTableDelete
 *
 * Delete the lock showing that the given transaction ID is running.
 * (This is never used for main transaction IDs; those locks are only
 * released implicitly at transaction end.	But we do use it for subtrans IDs.)
 */
void
XactLockTableDelete(TransactionId xid)
{
	LOCKTAG		tag;

	SET_LOCKTAG_TRANSACTION(tag, xid);

	LockRelease(&tag, ExclusiveLock, false);
}

/*
 *		XactLockTableWait
 *
 * Wait for the specified transaction to commit or abort.
 *
 * Note that this does the right thing for subtransactions: if we wait on a
 * subtransaction, we will exit as soon as it aborts or its top parent commits.
 * It takes some extra work to ensure this, because to save on shared memory
 * the XID lock of a subtransaction is released when it ends, whether
 * successfully or unsuccessfully.	So we have to check if it's "still running"
 * and if so wait for its parent.
 */
void
XactLockTableWait(TransactionId xid)
{
	LOCKTAG		tag;

	for (;;)
	{
		Assert(TransactionIdIsValid(xid));
		Assert(!TransactionIdEquals(xid, GetTopTransactionIdIfAny()));

		SET_LOCKTAG_TRANSACTION(tag, xid);

		(void) LockAcquire(&tag, ShareLock, false, false);

		LockRelease(&tag, ShareLock, false);

		if (!TransactionIdIsInProgress(xid))
			break;
		xid = SubTransGetParent(xid);
	}
}

/*
 *		ConditionalXactLockTableWait
 *
 * As above, but only lock if we can get the lock without blocking.
 * Returns TRUE if the lock was acquired.
 */
bool
ConditionalXactLockTableWait(TransactionId xid)
{
	LOCKTAG		tag;

	for (;;)
	{
		Assert(TransactionIdIsValid(xid));
		Assert(!TransactionIdEquals(xid, GetTopTransactionIdIfAny()));

		SET_LOCKTAG_TRANSACTION(tag, xid);

		if (LockAcquire(&tag, ShareLock, false, true) == LOCKACQUIRE_NOT_AVAIL)
			return false;

		LockRelease(&tag, ShareLock, false);

		if (!TransactionIdIsInProgress(xid))
			break;
		xid = SubTransGetParent(xid);
	}

	return true;
}


/*
 *		VirtualXactLockTableInsert
 *
 * Insert a lock showing that the given virtual transaction ID is running ---
 * this is done at main transaction start when its VXID is assigned.
 * The lock can then be used to wait for the transaction to finish.
 */
void
VirtualXactLockTableInsert(VirtualTransactionId vxid)
{
	LOCKTAG		tag;

	Assert(VirtualTransactionIdIsValid(vxid));

	SET_LOCKTAG_VIRTUALTRANSACTION(tag, vxid);

	(void) LockAcquire(&tag, ExclusiveLock, false, false);
}

/*
 *		VirtualXactLockTableDelete
 *
 * Release a Virtual Transaction lock. Only called by Startup process
 * at end of Hot Standby.
 */
void
VirtualXactLockTableDelete(VirtualTransactionId vxid)
{
	LOCKTAG		tag;

	Assert(VirtualTransactionIdIsValid(vxid));

	SET_LOCKTAG_VIRTUALTRANSACTION(tag, vxid);

	(void) LockRelease(&tag, ExclusiveLock, false);
}

/*
 *		VirtualXactLockTableWait
 *
 * Waits until the lock on the given VXID is released, which shows that
 * the top-level transaction owning the VXID has ended.
 */
void
VirtualXactLockTableWait(VirtualTransactionId vxid)
{
	LOCKTAG		tag;

	Assert(VirtualTransactionIdIsValid(vxid));

	SET_LOCKTAG_VIRTUALTRANSACTION(tag, vxid);

	(void) LockAcquire(&tag, ShareLock, false, false);

	LockRelease(&tag, ShareLock, false);
}

/*
 *		ConditionalVirtualXactLockTableWait
 *
 * As above, but only lock if we can get the lock without blocking.
 * Returns TRUE if the lock was acquired.
 */
bool
ConditionalVirtualXactLockTableWait(VirtualTransactionId vxid)
{
	LOCKTAG		tag;

	Assert(VirtualTransactionIdIsValid(vxid));

	SET_LOCKTAG_VIRTUALTRANSACTION(tag, vxid);

	if (LockAcquire(&tag, ShareLock, false, true) == LOCKACQUIRE_NOT_AVAIL)
		return false;

	LockRelease(&tag, ShareLock, false);

	return true;
}


/*
 *		LockDatabaseObject
 *
 * Obtain a lock on a general object of the current database.  Don't use
 * this for shared objects (such as tablespaces).  It's unwise to apply it
 * to relations, also, since a lock taken this way will NOT conflict with
 * locks taken via LockRelation and friends.
 */
void
LockDatabaseObject(Oid classid, Oid objid, uint16 objsubid,
				   LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_OBJECT(tag,
					   MyDatabaseId,
					   classid,
					   objid,
					   objsubid);

	(void) LockAcquire(&tag, lockmode, false, false);

	/* Make sure syscaches are up-to-date with any changes we waited for */
	AcceptInvalidationMessages();
}

/*
 *		UnlockDatabaseObject
 */
void
UnlockDatabaseObject(Oid classid, Oid objid, uint16 objsubid,
					 LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_OBJECT(tag,
					   MyDatabaseId,
					   classid,
					   objid,
					   objsubid);

	LockRelease(&tag, lockmode, false);
}

/*
 *		LockSharedObject
 *
 * Obtain a lock on a shared-across-databases object.
 */
void
LockSharedObject(Oid classid, Oid objid, uint16 objsubid,
				 LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_OBJECT(tag,
					   InvalidOid,
					   classid,
					   objid,
					   objsubid);

	(void) LockAcquire(&tag, lockmode, false, false);

	/* Make sure syscaches are up-to-date with any changes we waited for */
	AcceptInvalidationMessages();
}

/*
 *		UnlockSharedObject
 */
void
UnlockSharedObject(Oid classid, Oid objid, uint16 objsubid,
				   LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_OBJECT(tag,
					   InvalidOid,
					   classid,
					   objid,
					   objsubid);

	LockRelease(&tag, lockmode, false);
}

/*
 *		LockSharedObjectForSession
 *
 * Obtain a session-level lock on a shared-across-databases object.
 * See LockRelationIdForSession for notes about session-level locks.
 */
void
LockSharedObjectForSession(Oid classid, Oid objid, uint16 objsubid,
						   LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_OBJECT(tag,
					   InvalidOid,
					   classid,
					   objid,
					   objsubid);

	(void) LockAcquire(&tag, lockmode, true, false);
}

/*
 *		UnlockSharedObjectForSession
 */
void
UnlockSharedObjectForSession(Oid classid, Oid objid, uint16 objsubid,
							 LOCKMODE lockmode)
{
	LOCKTAG		tag;

	SET_LOCKTAG_OBJECT(tag,
					   InvalidOid,
					   classid,
					   objid,
					   objsubid);

	LockRelease(&tag, lockmode, true);
}


/*
 * Append a description of a lockable object to buf.
 *
 * Ideally we would print names for the numeric values, but that requires
 * getting locks on system tables, which might cause problems since this is
 * typically used to report deadlock situations.
 */
void
DescribeLockTag(StringInfo buf, const LOCKTAG *tag)
{
	switch ((LockTagType) tag->locktag_type)
	{
		case LOCKTAG_RELATION:
			appendStringInfo(buf,
							 _("relation %u of database %u"),
							 tag->locktag_field2,
							 tag->locktag_field1);
			break;
		case LOCKTAG_RELATION_EXTEND:
			appendStringInfo(buf,
							 _("extension of relation %u of database %u"),
							 tag->locktag_field2,
							 tag->locktag_field1);
			break;
		case LOCKTAG_PAGE:
			appendStringInfo(buf,
							 _("page %u of relation %u of database %u"),
							 tag->locktag_field3,
							 tag->locktag_field2,
							 tag->locktag_field1);
			break;
		case LOCKTAG_TUPLE:
			appendStringInfo(buf,
							 _("tuple (%u,%u) of relation %u of database %u"),
							 tag->locktag_field3,
							 tag->locktag_field4,
							 tag->locktag_field2,
							 tag->locktag_field1);
			break;
		case LOCKTAG_TRANSACTION:
			appendStringInfo(buf,
							 _("transaction %u"),
							 tag->locktag_field1);
			break;
		case LOCKTAG_VIRTUALTRANSACTION:
			appendStringInfo(buf,
							 _("virtual transaction %d/%u"),
							 tag->locktag_field1,
							 tag->locktag_field2);
			break;
		case LOCKTAG_OBJECT:
			appendStringInfo(buf,
							 _("object %u of class %u of database %u"),
							 tag->locktag_field3,
							 tag->locktag_field2,
							 tag->locktag_field1);
			break;
		case LOCKTAG_USERLOCK:
			/* reserved for old contrib code, now on pgfoundry */
			appendStringInfo(buf,
							 _("user lock [%u,%u,%u]"),
							 tag->locktag_field1,
							 tag->locktag_field2,
							 tag->locktag_field3);
			break;
		case LOCKTAG_ADVISORY:
			appendStringInfo(buf,
							 _("advisory lock [%u,%u,%u,%u]"),
							 tag->locktag_field1,
							 tag->locktag_field2,
							 tag->locktag_field3,
							 tag->locktag_field4);
			break;
		default:
			appendStringInfo(buf,
							 _("unrecognized locktag type %d"),
							 (int) tag->locktag_type);
			break;
	}
}
