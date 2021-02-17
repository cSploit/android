/*-------------------------------------------------------------------------
 *
 * standby.c
 *	  Misc functions used in Hot Standby mode.
 *
 *	All functions for handling RM_STANDBY_ID, which relate to
 *	AccessExclusiveLocks and starting snapshots for Hot Standby mode.
 *	Plus conflict recovery processing.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/storage/ipc/standby.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "access/transam.h"
#include "access/twophase.h"
#include "access/xact.h"
#include "access/xlog.h"
#include "miscadmin.h"
#include "storage/bufmgr.h"
#include "storage/lmgr.h"
#include "storage/proc.h"
#include "storage/procarray.h"
#include "storage/sinvaladt.h"
#include "storage/standby.h"
#include "utils/ps_status.h"

/* User-settable GUC parameters */
int			vacuum_defer_cleanup_age;
int			max_standby_archive_delay = 30 * 1000;
int			max_standby_streaming_delay = 30 * 1000;

static List *RecoveryLockList;

static void ResolveRecoveryConflictWithVirtualXIDs(VirtualTransactionId *waitlist,
									   ProcSignalReason reason);
static void ResolveRecoveryConflictWithLock(Oid dbOid, Oid relOid);
static void LogCurrentRunningXacts(RunningTransactions CurrRunningXacts);
static void LogAccessExclusiveLocks(int nlocks, xl_standby_lock *locks);


/*
 * InitRecoveryTransactionEnvironment
 *		Initialize tracking of in-progress transactions in master
 *
 * We need to issue shared invalidations and hold locks. Holding locks
 * means others may want to wait on us, so we need to make a lock table
 * vxact entry like a real transaction. We could create and delete
 * lock table entries for each transaction but its simpler just to create
 * one permanent entry and leave it there all the time. Locks are then
 * acquired and released as needed. Yes, this means you can see the
 * Startup process in pg_locks once we have run this.
 */
void
InitRecoveryTransactionEnvironment(void)
{
	VirtualTransactionId vxid;

	/*
	 * Initialize shared invalidation management for Startup process, being
	 * careful to register ourselves as a sendOnly process so we don't need to
	 * read messages, nor will we get signalled when the queue starts filling
	 * up.
	 */
	SharedInvalBackendInit(true);

	/*
	 * Lock a virtual transaction id for Startup process.
	 *
	 * We need to do GetNextLocalTransactionId() because
	 * SharedInvalBackendInit() leaves localTransactionid invalid and the lock
	 * manager doesn't like that at all.
	 *
	 * Note that we don't need to run XactLockTableInsert() because nobody
	 * needs to wait on xids. That sounds a little strange, but table locks
	 * are held by vxids and row level locks are held by xids. All queries
	 * hold AccessShareLocks so never block while we write or lock new rows.
	 */
	vxid.backendId = MyBackendId;
	vxid.localTransactionId = MyProc->lxid = GetNextLocalTransactionId();
	VirtualXactLockTableInsert(vxid);

	standbyState = STANDBY_INITIALIZED;
}

/*
 * ShutdownRecoveryTransactionEnvironment
 *		Shut down transaction tracking
 *
 * Prepare to switch from hot standby mode to normal operation. Shut down
 * recovery-time transaction tracking.
 */
void
ShutdownRecoveryTransactionEnvironment(void)
{
	VirtualTransactionId vxid;

	/* Mark all tracked in-progress transactions as finished. */
	ExpireAllKnownAssignedTransactionIds();

	/* Release all locks the tracked transactions were holding */
	StandbyReleaseAllLocks();

	/* Cleanup our VirtualTransaction */
	vxid.backendId = MyBackendId;
	vxid.localTransactionId = MyProc->lxid;
	VirtualXactLockTableDelete(vxid);
}


/*
 * -----------------------------------------------------
 *		Standby wait timers and backend cancel logic
 * -----------------------------------------------------
 */

/*
 * Determine the cutoff time at which we want to start canceling conflicting
 * transactions.  Returns zero (a time safely in the past) if we are willing
 * to wait forever.
 */
static TimestampTz
GetStandbyLimitTime(void)
{
	TimestampTz rtime;
	bool		fromStream;

	/*
	 * The cutoff time is the last WAL data receipt time plus the appropriate
	 * delay variable.	Delay of -1 means wait forever.
	 */
	GetXLogReceiptTime(&rtime, &fromStream);
	if (fromStream)
	{
		if (max_standby_streaming_delay < 0)
			return 0;			/* wait forever */
		return TimestampTzPlusMilliseconds(rtime, max_standby_streaming_delay);
	}
	else
	{
		if (max_standby_archive_delay < 0)
			return 0;			/* wait forever */
		return TimestampTzPlusMilliseconds(rtime, max_standby_archive_delay);
	}
}

#define STANDBY_INITIAL_WAIT_US  1000
static int	standbyWait_us = STANDBY_INITIAL_WAIT_US;

/*
 * Standby wait logic for ResolveRecoveryConflictWithVirtualXIDs.
 * We wait here for a while then return. If we decide we can't wait any
 * more then we return true, if we can wait some more return false.
 */
static bool
WaitExceedsMaxStandbyDelay(void)
{
	TimestampTz ltime;

	/* Are we past the limit time? */
	ltime = GetStandbyLimitTime();
	if (ltime && GetCurrentTimestamp() >= ltime)
		return true;

	/*
	 * Sleep a bit (this is essential to avoid busy-waiting).
	 */
	pg_usleep(standbyWait_us);

	/*
	 * Progressively increase the sleep times, but not to more than 1s, since
	 * pg_usleep isn't interruptable on some platforms.
	 */
	standbyWait_us *= 2;
	if (standbyWait_us > 1000000)
		standbyWait_us = 1000000;

	return false;
}

/*
 * This is the main executioner for any query backend that conflicts with
 * recovery processing. Judgement has already been passed on it within
 * a specific rmgr. Here we just issue the orders to the procs. The procs
 * then throw the required error as instructed.
 */
static void
ResolveRecoveryConflictWithVirtualXIDs(VirtualTransactionId *waitlist,
									   ProcSignalReason reason)
{
	TimestampTz waitStart;
	char	   *new_status;

	/* Fast exit, to avoid a kernel call if there's no work to be done. */
	if (!VirtualTransactionIdIsValid(*waitlist))
		return;

	waitStart = GetCurrentTimestamp();
	new_status = NULL;			/* we haven't changed the ps display */

	while (VirtualTransactionIdIsValid(*waitlist))
	{
		/* reset standbyWait_us for each xact we wait for */
		standbyWait_us = STANDBY_INITIAL_WAIT_US;

		/* wait until the virtual xid is gone */
		while (!ConditionalVirtualXactLockTableWait(*waitlist))
		{
			/*
			 * Report via ps if we have been waiting for more than 500 msec
			 * (should that be configurable?)
			 */
			if (update_process_title && new_status == NULL &&
				TimestampDifferenceExceeds(waitStart, GetCurrentTimestamp(),
										   500))
			{
				const char *old_status;
				int			len;

				old_status = get_ps_display(&len);
				new_status = (char *) palloc(len + 8 + 1);
				memcpy(new_status, old_status, len);
				strcpy(new_status + len, " waiting");
				set_ps_display(new_status, false);
				new_status[len] = '\0'; /* truncate off " waiting" */
			}

			/* Is it time to kill it? */
			if (WaitExceedsMaxStandbyDelay())
			{
				pid_t		pid;

				/*
				 * Now find out who to throw out of the balloon.
				 */
				Assert(VirtualTransactionIdIsValid(*waitlist));
				pid = CancelVirtualTransaction(*waitlist, reason);

				/*
				 * Wait a little bit for it to die so that we avoid flooding
				 * an unresponsive backend when system is heavily loaded.
				 */
				if (pid != 0)
					pg_usleep(5000L);
			}
		}

		/* The virtual transaction is gone now, wait for the next one */
		waitlist++;
	}

	/* Reset ps display if we changed it */
	if (new_status)
	{
		set_ps_display(new_status, false);
		pfree(new_status);
	}
}

void
ResolveRecoveryConflictWithSnapshot(TransactionId latestRemovedXid, RelFileNode node)
{
	VirtualTransactionId *backends;

	/*
	 * If we get passed InvalidTransactionId then we are a little surprised,
	 * but it is theoretically possible in normal running. It also happens
	 * when replaying already applied WAL records after a standby crash or
	 * restart. If latestRemovedXid is invalid then there is no conflict. That
	 * rule applies across all record types that suffer from this conflict.
	 */
	if (!TransactionIdIsValid(latestRemovedXid))
		return;

	backends = GetConflictingVirtualXIDs(latestRemovedXid,
										 node.dbNode);

	ResolveRecoveryConflictWithVirtualXIDs(backends,
										 PROCSIG_RECOVERY_CONFLICT_SNAPSHOT);
}

void
ResolveRecoveryConflictWithTablespace(Oid tsid)
{
	VirtualTransactionId *temp_file_users;

	/*
	 * Standby users may be currently using this tablespace for for their
	 * temporary files. We only care about current users because
	 * temp_tablespace parameter will just ignore tablespaces that no longer
	 * exist.
	 *
	 * Ask everybody to cancel their queries immediately so we can ensure no
	 * temp files remain and we can remove the tablespace. Nuke the entire
	 * site from orbit, it's the only way to be sure.
	 *
	 * XXX: We could work out the pids of active backends using this
	 * tablespace by examining the temp filenames in the directory. We would
	 * then convert the pids into VirtualXIDs before attempting to cancel
	 * them.
	 *
	 * We don't wait for commit because drop tablespace is non-transactional.
	 */
	temp_file_users = GetConflictingVirtualXIDs(InvalidTransactionId,
												InvalidOid);
	ResolveRecoveryConflictWithVirtualXIDs(temp_file_users,
									   PROCSIG_RECOVERY_CONFLICT_TABLESPACE);
}

void
ResolveRecoveryConflictWithDatabase(Oid dbid)
{
	/*
	 * We don't do ResolveRecoveryConflictWithVirtualXIDs() here since that
	 * only waits for transactions and completely idle sessions would block
	 * us. This is rare enough that we do this as simply as possible: no wait,
	 * just force them off immediately.
	 *
	 * No locking is required here because we already acquired
	 * AccessExclusiveLock. Anybody trying to connect while we do this will
	 * block during InitPostgres() and then disconnect when they see the
	 * database has been removed.
	 */
	while (CountDBBackends(dbid) > 0)
	{
		CancelDBBackends(dbid, PROCSIG_RECOVERY_CONFLICT_DATABASE, true);

		/*
		 * Wait awhile for them to die so that we avoid flooding an
		 * unresponsive backend when system is heavily loaded.
		 */
		pg_usleep(10000);
	}
}

static void
ResolveRecoveryConflictWithLock(Oid dbOid, Oid relOid)
{
	VirtualTransactionId *backends;
	bool		lock_acquired = false;
	int			num_attempts = 0;
	LOCKTAG		locktag;

	SET_LOCKTAG_RELATION(locktag, dbOid, relOid);

	/*
	 * If blowing away everybody with conflicting locks doesn't work, after
	 * the first two attempts then we just start blowing everybody away until
	 * it does work. We do this because its likely that we either have too
	 * many locks and we just can't get one at all, or that there are many
	 * people crowding for the same table. Recovery must win; the end
	 * justifies the means.
	 */
	while (!lock_acquired)
	{
		if (++num_attempts < 3)
			backends = GetLockConflicts(&locktag, AccessExclusiveLock);
		else
			backends = GetConflictingVirtualXIDs(InvalidTransactionId,
												 InvalidOid);

		ResolveRecoveryConflictWithVirtualXIDs(backends,
											 PROCSIG_RECOVERY_CONFLICT_LOCK);

		if (LockAcquireExtended(&locktag, AccessExclusiveLock, true, true, false)
			!= LOCKACQUIRE_NOT_AVAIL)
			lock_acquired = true;
	}
}

/*
 * ResolveRecoveryConflictWithBufferPin is called from LockBufferForCleanup()
 * to resolve conflicts with other backends holding buffer pins.
 *
 * We either resolve conflicts immediately or set a SIGALRM to wake us at
 * the limit of our patience. The sleep in LockBufferForCleanup() is
 * performed here, for code clarity.
 *
 * Resolve conflicts by sending a PROCSIG signal to all backends to check if
 * they hold one of the buffer pins that is blocking Startup process. If so,
 * backends will take an appropriate error action, ERROR or FATAL.
 *
 * We also must check for deadlocks.  Deadlocks occur because if queries
 * wait on a lock, that must be behind an AccessExclusiveLock, which can only
 * be cleared if the Startup process replays a transaction completion record.
 * If Startup process is also waiting then that is a deadlock. The deadlock
 * can occur if the query is waiting and then the Startup sleeps, or if
 * Startup is sleeping and the query waits on a lock. We protect against
 * only the former sequence here, the latter sequence is checked prior to
 * the query sleeping, in CheckRecoveryConflictDeadlock().
 *
 * Deadlocks are extremely rare, and relatively expensive to check for,
 * so we don't do a deadlock check right away ... only if we have had to wait
 * at least deadlock_timeout.  Most of the logic about that is in proc.c.
 */
void
ResolveRecoveryConflictWithBufferPin(void)
{
	bool		sig_alarm_enabled = false;
	TimestampTz ltime;
	TimestampTz now;

	Assert(InHotStandby);

	ltime = GetStandbyLimitTime();
	now = GetCurrentTimestamp();

	if (!ltime)
	{
		/*
		 * We're willing to wait forever for conflicts, so set timeout for
		 * deadlock check (only)
		 */
		if (enable_standby_sig_alarm(now, now, true))
			sig_alarm_enabled = true;
		else
			elog(FATAL, "could not set timer for process wakeup");
	}
	else if (now >= ltime)
	{
		/*
		 * We're already behind, so clear a path as quickly as possible.
		 */
		SendRecoveryConflictWithBufferPin(PROCSIG_RECOVERY_CONFLICT_BUFFERPIN);
	}
	else
	{
		/*
		 * Wake up at ltime, and check for deadlocks as well if we will be
		 * waiting longer than deadlock_timeout
		 */
		if (enable_standby_sig_alarm(now, ltime, false))
			sig_alarm_enabled = true;
		else
			elog(FATAL, "could not set timer for process wakeup");
	}

	/* Wait to be signaled by UnpinBuffer() */
	ProcWaitForSignal();

	if (sig_alarm_enabled)
	{
		if (!disable_standby_sig_alarm())
			elog(FATAL, "could not disable timer for process wakeup");
	}
}

void
SendRecoveryConflictWithBufferPin(ProcSignalReason reason)
{
	Assert(reason == PROCSIG_RECOVERY_CONFLICT_BUFFERPIN ||
		   reason == PROCSIG_RECOVERY_CONFLICT_STARTUP_DEADLOCK);

	/*
	 * We send signal to all backends to ask them if they are holding the
	 * buffer pin which is delaying the Startup process. We must not set the
	 * conflict flag yet, since most backends will be innocent. Let the
	 * SIGUSR1 handling in each backend decide their own fate.
	 */
	CancelDBBackends(InvalidOid, reason, false);
}

/*
 * In Hot Standby perform early deadlock detection.  We abort the lock
 * wait if we are about to sleep while holding the buffer pin that Startup
 * process is waiting for.
 *
 * Note: this code is pessimistic, because there is no way for it to
 * determine whether an actual deadlock condition is present: the lock we
 * need to wait for might be unrelated to any held by the Startup process.
 * Sooner or later, this mechanism should get ripped out in favor of somehow
 * accounting for buffer locks in DeadLockCheck().  However, errors here
 * seem to be very low-probability in practice, so for now it's not worth
 * the trouble.
 */
void
CheckRecoveryConflictDeadlock(void)
{
	Assert(!InRecovery);		/* do not call in Startup process */

	if (!HoldingBufferPinThatDelaysRecovery())
		return;

	/*
	 * Error message should match ProcessInterrupts() but we avoid calling
	 * that because we aren't handling an interrupt at this point. Note that
	 * we only cancel the current transaction here, so if we are in a
	 * subtransaction and the pin is held by a parent, then the Startup
	 * process will continue to wait even though we have avoided deadlock.
	 */
	ereport(ERROR,
			(errcode(ERRCODE_T_R_DEADLOCK_DETECTED),
			 errmsg("canceling statement due to conflict with recovery"),
	   errdetail("User transaction caused buffer deadlock with recovery.")));
}

/*
 * -----------------------------------------------------
 * Locking in Recovery Mode
 * -----------------------------------------------------
 *
 * All locks are held by the Startup process using a single virtual
 * transaction. This implementation is both simpler and in some senses,
 * more correct. The locks held mean "some original transaction held
 * this lock, so query access is not allowed at this time". So the Startup
 * process is the proxy by which the original locks are implemented.
 *
 * We only keep track of AccessExclusiveLocks, which are only ever held by
 * one transaction on one relation, and don't worry about lock queuing.
 *
 * We keep a single dynamically expandible list of locks in local memory,
 * RelationLockList, so we can keep track of the various entries made by
 * the Startup process's virtual xid in the shared lock table.
 *
 * We record the lock against the top-level xid, rather than individual
 * subtransaction xids. This means AccessExclusiveLocks held by aborted
 * subtransactions are not released as early as possible on standbys.
 *
 * List elements use type xl_rel_lock, since the WAL record type exactly
 * matches the information that we need to keep track of.
 *
 * We use session locks rather than normal locks so we don't need
 * ResourceOwners.
 */


void
StandbyAcquireAccessExclusiveLock(TransactionId xid, Oid dbOid, Oid relOid)
{
	xl_standby_lock *newlock;
	LOCKTAG		locktag;

	/* Already processed? */
	if (!TransactionIdIsValid(xid) ||
		TransactionIdDidCommit(xid) ||
		TransactionIdDidAbort(xid))
		return;

	elog(trace_recovery(DEBUG4),
		 "adding recovery lock: db %u rel %u", dbOid, relOid);

	/* dbOid is InvalidOid when we are locking a shared relation. */
	Assert(OidIsValid(relOid));

	newlock = palloc(sizeof(xl_standby_lock));
	newlock->xid = xid;
	newlock->dbOid = dbOid;
	newlock->relOid = relOid;
	RecoveryLockList = lappend(RecoveryLockList, newlock);

	/*
	 * Attempt to acquire the lock as requested, if not resolve conflict
	 */
	SET_LOCKTAG_RELATION(locktag, newlock->dbOid, newlock->relOid);

	if (LockAcquireExtended(&locktag, AccessExclusiveLock, true, true, false)
		== LOCKACQUIRE_NOT_AVAIL)
		ResolveRecoveryConflictWithLock(newlock->dbOid, newlock->relOid);
}

static void
StandbyReleaseLocks(TransactionId xid)
{
	ListCell   *cell,
			   *prev,
			   *next;

	/*
	 * Release all matching locks and remove them from list
	 */
	prev = NULL;
	for (cell = list_head(RecoveryLockList); cell; cell = next)
	{
		xl_standby_lock *lock = (xl_standby_lock *) lfirst(cell);

		next = lnext(cell);

		if (!TransactionIdIsValid(xid) || lock->xid == xid)
		{
			LOCKTAG		locktag;

			elog(trace_recovery(DEBUG4),
				 "releasing recovery lock: xid %u db %u rel %u",
				 lock->xid, lock->dbOid, lock->relOid);
			SET_LOCKTAG_RELATION(locktag, lock->dbOid, lock->relOid);
			if (!LockRelease(&locktag, AccessExclusiveLock, true))
				elog(LOG,
					 "RecoveryLockList contains entry for lock no longer recorded by lock manager: xid %u database %u relation %u",
					 lock->xid, lock->dbOid, lock->relOid);

			RecoveryLockList = list_delete_cell(RecoveryLockList, cell, prev);
			pfree(lock);
		}
		else
			prev = cell;
	}
}

/*
 * Release locks for a transaction tree, starting at xid down, from
 * RecoveryLockList.
 *
 * Called during WAL replay of COMMIT/ROLLBACK when in hot standby mode,
 * to remove any AccessExclusiveLocks requested by a transaction.
 */
void
StandbyReleaseLockTree(TransactionId xid, int nsubxids, TransactionId *subxids)
{
	int			i;

	StandbyReleaseLocks(xid);

	for (i = 0; i < nsubxids; i++)
		StandbyReleaseLocks(subxids[i]);
}

/*
 * Called at end of recovery and when we see a shutdown checkpoint.
 */
void
StandbyReleaseAllLocks(void)
{
	ListCell   *cell,
			   *prev,
			   *next;
	LOCKTAG		locktag;

	elog(trace_recovery(DEBUG2), "release all standby locks");

	prev = NULL;
	for (cell = list_head(RecoveryLockList); cell; cell = next)
	{
		xl_standby_lock *lock = (xl_standby_lock *) lfirst(cell);

		next = lnext(cell);

		elog(trace_recovery(DEBUG4),
			 "releasing recovery lock: xid %u db %u rel %u",
			 lock->xid, lock->dbOid, lock->relOid);
		SET_LOCKTAG_RELATION(locktag, lock->dbOid, lock->relOid);
		if (!LockRelease(&locktag, AccessExclusiveLock, true))
			elog(LOG,
				 "RecoveryLockList contains entry for lock no longer recorded by lock manager: xid %u database %u relation %u",
				 lock->xid, lock->dbOid, lock->relOid);
		RecoveryLockList = list_delete_cell(RecoveryLockList, cell, prev);
		pfree(lock);
	}
}

/*
 * StandbyReleaseOldLocks
 *		Release standby locks held by top-level XIDs that aren't running,
 *		as long as they're not prepared transactions.
 */
void
StandbyReleaseOldLocks(int nxids, TransactionId *xids)
{
	ListCell   *cell,
			   *prev,
			   *next;
	LOCKTAG		locktag;

	prev = NULL;
	for (cell = list_head(RecoveryLockList); cell; cell = next)
	{
		xl_standby_lock *lock = (xl_standby_lock *) lfirst(cell);
		bool	remove = false;

		next = lnext(cell);

		Assert(TransactionIdIsValid(lock->xid));

		if (StandbyTransactionIdIsPrepared(lock->xid))
			remove = false;
		else
		{
			int		i;
			bool	found = false;

			for (i = 0; i < nxids; i++)
			{
				if (lock->xid == xids[i])
				{
					found = true;
					break;
				}
			}

			/*
			 * If its not a running transaction, remove it.
			 */
			if (!found)
				remove = true;
		}

		if (remove)
		{
			elog(trace_recovery(DEBUG4),
				 "releasing recovery lock: xid %u db %u rel %u",
				 lock->xid, lock->dbOid, lock->relOid);
			SET_LOCKTAG_RELATION(locktag, lock->dbOid, lock->relOid);
			if (!LockRelease(&locktag, AccessExclusiveLock, true))
				elog(LOG,
					 "RecoveryLockList contains entry for lock no longer recorded by lock manager: xid %u database %u relation %u",
					 lock->xid, lock->dbOid, lock->relOid);
			RecoveryLockList = list_delete_cell(RecoveryLockList, cell, prev);
			pfree(lock);
		}
		else
			prev = cell;
	}
}

/*
 * --------------------------------------------------------------------
 *		Recovery handling for Rmgr RM_STANDBY_ID
 *
 * These record types will only be created if XLogStandbyInfoActive()
 * --------------------------------------------------------------------
 */

void
standby_redo(XLogRecPtr lsn, XLogRecord *record)
{
	uint8		info = record->xl_info & ~XLR_INFO_MASK;

	/* Do nothing if we're not in hot standby mode */
	if (standbyState == STANDBY_DISABLED)
		return;

	if (info == XLOG_STANDBY_LOCK)
	{
		xl_standby_locks *xlrec = (xl_standby_locks *) XLogRecGetData(record);
		int			i;

		for (i = 0; i < xlrec->nlocks; i++)
			StandbyAcquireAccessExclusiveLock(xlrec->locks[i].xid,
											  xlrec->locks[i].dbOid,
											  xlrec->locks[i].relOid);
	}
	else if (info == XLOG_RUNNING_XACTS)
	{
		xl_running_xacts *xlrec = (xl_running_xacts *) XLogRecGetData(record);
		RunningTransactionsData running;

		running.xcnt = xlrec->xcnt;
		running.subxid_overflow = xlrec->subxid_overflow;
		running.nextXid = xlrec->nextXid;
		running.latestCompletedXid = xlrec->latestCompletedXid;
		running.oldestRunningXid = xlrec->oldestRunningXid;
		running.xids = xlrec->xids;

		ProcArrayApplyRecoveryInfo(&running);
	}
	else
		elog(PANIC, "relation_redo: unknown op code %u", info);
}

static void
standby_desc_running_xacts(StringInfo buf, xl_running_xacts *xlrec)
{
	int			i;

	appendStringInfo(buf, " nextXid %u latestCompletedXid %u oldestRunningXid %u",
					 xlrec->nextXid,
					 xlrec->latestCompletedXid,
					 xlrec->oldestRunningXid);
	if (xlrec->xcnt > 0)
	{
		appendStringInfo(buf, "; %d xacts:", xlrec->xcnt);
		for (i = 0; i < xlrec->xcnt; i++)
			appendStringInfo(buf, " %u", xlrec->xids[i]);
	}

	if (xlrec->subxid_overflow)
		appendStringInfo(buf, "; subxid ovf");
}

void
standby_desc(StringInfo buf, uint8 xl_info, char *rec)
{
	uint8		info = xl_info & ~XLR_INFO_MASK;

	if (info == XLOG_STANDBY_LOCK)
	{
		xl_standby_locks *xlrec = (xl_standby_locks *) rec;
		int			i;

		appendStringInfo(buf, "AccessExclusive locks:");

		for (i = 0; i < xlrec->nlocks; i++)
			appendStringInfo(buf, " xid %u db %u rel %u",
							 xlrec->locks[i].xid, xlrec->locks[i].dbOid,
							 xlrec->locks[i].relOid);
	}
	else if (info == XLOG_RUNNING_XACTS)
	{
		xl_running_xacts *xlrec = (xl_running_xacts *) rec;

		appendStringInfo(buf, " running xacts:");
		standby_desc_running_xacts(buf, xlrec);
	}
	else
		appendStringInfo(buf, "UNKNOWN");
}

/*
 * Log details of the current snapshot to WAL. This allows the snapshot state
 * to be reconstructed on the standby.
 *
 * We can move directly to STANDBY_SNAPSHOT_READY at startup if we
 * start from a shutdown checkpoint because we know nothing was running
 * at that time and our recovery snapshot is known empty. In the more
 * typical case of an online checkpoint we need to jump through a few
 * hoops to get a correct recovery snapshot and this requires a two or
 * sometimes a three stage process.
 *
 * The initial snapshot must contain all running xids and all current
 * AccessExclusiveLocks at a point in time on the standby. Assembling
 * that information while the server is running requires many and
 * various LWLocks, so we choose to derive that information piece by
 * piece and then re-assemble that info on the standby. When that
 * information is fully assembled we move to STANDBY_SNAPSHOT_READY.
 *
 * Since locking on the primary when we derive the information is not
 * strict, we note that there is a time window between the derivation and
 * writing to WAL of the derived information. That allows race conditions
 * that we must resolve, since xids and locks may enter or leave the
 * snapshot during that window. This creates the issue that an xid or
 * lock may start *after* the snapshot has been derived yet *before* the
 * snapshot is logged in the running xacts WAL record. We resolve this by
 * starting to accumulate changes at a point just prior to when we derive
 * the snapshot on the primary, then ignore duplicates when we later apply
 * the snapshot from the running xacts record. This is implemented during
 * CreateCheckpoint() where we use the logical checkpoint location as
 * our starting point and then write the running xacts record immediately
 * before writing the main checkpoint WAL record. Since we always start
 * up from a checkpoint and are immediately at our starting point, we
 * unconditionally move to STANDBY_INITIALIZED. After this point we
 * must do 4 things:
 *	* move shared nextXid forwards as we see new xids
 *	* extend the clog and subtrans with each new xid
 *	* keep track of uncommitted known assigned xids
 *	* keep track of uncommitted AccessExclusiveLocks
 *
 * When we see a commit/abort we must remove known assigned xids and locks
 * from the completing transaction. Attempted removals that cannot locate
 * an entry are expected and must not cause an error when we are in state
 * STANDBY_INITIALIZED. This is implemented in StandbyReleaseLocks() and
 * KnownAssignedXidsRemove().
 *
 * Later, when we apply the running xact data we must be careful to ignore
 * transactions already committed, since those commits raced ahead when
 * making WAL entries.
 *
 * The loose timing also means that locks may be recorded that have a
 * zero xid, since xids are removed from procs before locks are removed.
 * So we must prune the lock list down to ensure we hold locks only for
 * currently running xids, performed by StandbyReleaseOldLocks().
 * Zero xids should no longer be possible, but we may be replaying WAL
 * from a time when they were possible.
 */
void
LogStandbySnapshot(void)
{
	RunningTransactions running;
	xl_standby_lock *locks;
	int			nlocks;

	Assert(XLogStandbyInfoActive());

	/*
	 * Get details of any AccessExclusiveLocks being held at the moment.
	 */
	locks = GetRunningTransactionLocks(&nlocks);
	if (nlocks > 0)
		LogAccessExclusiveLocks(nlocks, locks);
	pfree(locks);

	/*
	 * Log details of all in-progress transactions. This should be the last
	 * record we write, because standby will open up when it sees this.
	 */
	running = GetRunningTransactionData();
	LogCurrentRunningXacts(running);
	/* GetRunningTransactionData() acquired XidGenLock, we must release it */
	LWLockRelease(XidGenLock);
}

/*
 * Record an enhanced snapshot of running transactions into WAL.
 *
 * The definitions of RunningTransactionsData and xl_xact_running_xacts
 * are similar. We keep them separate because xl_xact_running_xacts
 * is a contiguous chunk of memory and never exists fully until it is
 * assembled in WAL.
 */
static void
LogCurrentRunningXacts(RunningTransactions CurrRunningXacts)
{
	xl_running_xacts xlrec;
	XLogRecData rdata[2];
	int			lastrdata = 0;
	XLogRecPtr	recptr;

	xlrec.xcnt = CurrRunningXacts->xcnt;
	xlrec.subxid_overflow = CurrRunningXacts->subxid_overflow;
	xlrec.nextXid = CurrRunningXacts->nextXid;
	xlrec.oldestRunningXid = CurrRunningXacts->oldestRunningXid;
	xlrec.latestCompletedXid = CurrRunningXacts->latestCompletedXid;

	/* Header */
	rdata[0].data = (char *) (&xlrec);
	rdata[0].len = MinSizeOfXactRunningXacts;
	rdata[0].buffer = InvalidBuffer;

	/* array of TransactionIds */
	if (xlrec.xcnt > 0)
	{
		rdata[0].next = &(rdata[1]);
		rdata[1].data = (char *) CurrRunningXacts->xids;
		rdata[1].len = xlrec.xcnt * sizeof(TransactionId);
		rdata[1].buffer = InvalidBuffer;
		lastrdata = 1;
	}

	rdata[lastrdata].next = NULL;

	recptr = XLogInsert(RM_STANDBY_ID, XLOG_RUNNING_XACTS, rdata);

	if (CurrRunningXacts->subxid_overflow)
		elog(trace_recovery(DEBUG2),
			 "snapshot of %u running transactions overflowed (lsn %X/%X oldest xid %u latest complete %u next xid %u)",
			 CurrRunningXacts->xcnt,
			 recptr.xlogid, recptr.xrecoff,
			 CurrRunningXacts->oldestRunningXid,
			 CurrRunningXacts->latestCompletedXid,
			 CurrRunningXacts->nextXid);
	else
		elog(trace_recovery(DEBUG2),
			 "snapshot of %u running transaction ids (lsn %X/%X oldest xid %u latest complete %u next xid %u)",
			 CurrRunningXacts->xcnt,
			 recptr.xlogid, recptr.xrecoff,
			 CurrRunningXacts->oldestRunningXid,
			 CurrRunningXacts->latestCompletedXid,
			 CurrRunningXacts->nextXid);
}

/*
 * Wholesale logging of AccessExclusiveLocks. Other lock types need not be
 * logged, as described in backend/storage/lmgr/README.
 */
static void
LogAccessExclusiveLocks(int nlocks, xl_standby_lock *locks)
{
	XLogRecData rdata[2];
	xl_standby_locks xlrec;

	xlrec.nlocks = nlocks;

	rdata[0].data = (char *) &xlrec;
	rdata[0].len = offsetof(xl_standby_locks, locks);
	rdata[0].buffer = InvalidBuffer;
	rdata[0].next = &rdata[1];

	rdata[1].data = (char *) locks;
	rdata[1].len = nlocks * sizeof(xl_standby_lock);
	rdata[1].buffer = InvalidBuffer;
	rdata[1].next = NULL;

	(void) XLogInsert(RM_STANDBY_ID, XLOG_STANDBY_LOCK, rdata);
}

/*
 * Individual logging of AccessExclusiveLocks for use during LockAcquire()
 */
void
LogAccessExclusiveLock(Oid dbOid, Oid relOid)
{
	xl_standby_lock xlrec;

	xlrec.xid = GetTopTransactionId();

	/*
	 * Decode the locktag back to the original values, to avoid sending lots
	 * of empty bytes with every message.  See lock.h to check how a locktag
	 * is defined for LOCKTAG_RELATION
	 */
	xlrec.dbOid = dbOid;
	xlrec.relOid = relOid;

	LogAccessExclusiveLocks(1, &xlrec);
}

/*
 * Prepare to log an AccessExclusiveLock, for use during LockAcquire()
 */
void
LogAccessExclusiveLockPrepare(void)
{
	/*
	 * Ensure that a TransactionId has been assigned to this transaction, for
	 * two reasons, both related to lock release on the standby. First, we
	 * must assign an xid so that RecordTransactionCommit() and
	 * RecordTransactionAbort() do not optimise away the transaction
	 * completion record which recovery relies upon to release locks. It's a
	 * hack, but for a corner case not worth adding code for into the main
	 * commit path. Second, must must assign an xid before the lock is
	 * recorded in shared memory, otherwise a concurrently executing
	 * GetRunningTransactionLocks() might see a lock associated with an
	 * InvalidTransactionId which we later assert cannot happen.
	 */
	(void) GetTopTransactionId();
}
