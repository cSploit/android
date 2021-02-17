/*-------------------------------------------------------------------------
 *
 * bgwriter.c
 *
 * The background writer (bgwriter) is new as of Postgres 8.0.	It attempts
 * to keep regular backends from having to write out dirty shared buffers
 * (which they would only do when needing to free a shared buffer to read in
 * another page).  In the best scenario all writes from shared buffers will
 * be issued by the background writer process.	However, regular backends are
 * still empowered to issue writes if the bgwriter fails to maintain enough
 * clean shared buffers.
 *
 * The bgwriter is also charged with handling all checkpoints.	It will
 * automatically dispatch a checkpoint after a certain amount of time has
 * elapsed since the last one, and it can be signaled to perform requested
 * checkpoints as well.  (The GUC parameter that mandates a checkpoint every
 * so many WAL segments is implemented by having backends signal the bgwriter
 * when they fill WAL segments; the bgwriter itself doesn't watch for the
 * condition.)
 *
 * The bgwriter is started by the postmaster as soon as the startup subprocess
 * finishes, or as soon as recovery begins if we are doing archive recovery.
 * It remains alive until the postmaster commands it to terminate.
 * Normal termination is by SIGUSR2, which instructs the bgwriter to execute
 * a shutdown checkpoint and then exit(0).	(All backends must be stopped
 * before SIGUSR2 is issued!)  Emergency termination is by SIGQUIT; like any
 * backend, the bgwriter will simply abort and exit on SIGQUIT.
 *
 * If the bgwriter exits unexpectedly, the postmaster treats that the same
 * as a backend crash: shared memory may be corrupted, so remaining backends
 * should be killed by SIGQUIT and then a recovery cycle started.  (Even if
 * shared memory isn't corrupted, we have lost information about which
 * files need to be fsync'd for the next checkpoint, and so a system
 * restart needs to be forced.)
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/backend/postmaster/bgwriter.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "access/xlog_internal.h"
#include "catalog/pg_control.h"
#include "libpq/pqsignal.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "postmaster/bgwriter.h"
#include "replication/syncrep.h"
#include "storage/bufmgr.h"
#include "storage/fd.h"
#include "storage/ipc.h"
#include "storage/lwlock.h"
#include "storage/pmsignal.h"
#include "storage/shmem.h"
#include "storage/smgr.h"
#include "storage/spin.h"
#include "tcop/tcopprot.h"
#include "utils/guc.h"
#include "utils/memutils.h"
#include "utils/resowner.h"


/*----------
 * Shared memory area for communication between bgwriter and backends
 *
 * The ckpt counters allow backends to watch for completion of a checkpoint
 * request they send.  Here's how it works:
 *	* At start of a checkpoint, bgwriter reads (and clears) the request flags
 *	  and increments ckpt_started, while holding ckpt_lck.
 *	* On completion of a checkpoint, bgwriter sets ckpt_done to
 *	  equal ckpt_started.
 *	* On failure of a checkpoint, bgwriter increments ckpt_failed
 *	  and sets ckpt_done to equal ckpt_started.
 *
 * The algorithm for backends is:
 *	1. Record current values of ckpt_failed and ckpt_started, and
 *	   set request flags, while holding ckpt_lck.
 *	2. Send signal to request checkpoint.
 *	3. Sleep until ckpt_started changes.  Now you know a checkpoint has
 *	   begun since you started this algorithm (although *not* that it was
 *	   specifically initiated by your signal), and that it is using your flags.
 *	4. Record new value of ckpt_started.
 *	5. Sleep until ckpt_done >= saved value of ckpt_started.  (Use modulo
 *	   arithmetic here in case counters wrap around.)  Now you know a
 *	   checkpoint has started and completed, but not whether it was
 *	   successful.
 *	6. If ckpt_failed is different from the originally saved value,
 *	   assume request failed; otherwise it was definitely successful.
 *
 * ckpt_flags holds the OR of the checkpoint request flags sent by all
 * requesting backends since the last checkpoint start.  The flags are
 * chosen so that OR'ing is the correct way to combine multiple requests.
 *
 * num_backend_writes is used to count the number of buffer writes performed
 * by non-bgwriter processes.  This counter should be wide enough that it
 * can't overflow during a single bgwriter cycle.  num_backend_fsync
 * counts the subset of those writes that also had to do their own fsync,
 * because the background writer failed to absorb their request.
 *
 * The requests array holds fsync requests sent by backends and not yet
 * absorbed by the bgwriter.
 *
 * Unlike the checkpoint fields, num_backend_writes, num_backend_fsync, and
 * the requests fields are protected by BgWriterCommLock.
 *----------
 */
typedef struct
{
	RelFileNode	rnode;
	ForkNumber	forknum;
	BlockNumber segno;			/* see md.c for special values */
	/* might add a real request-type field later; not needed yet */
} BgWriterRequest;

typedef struct
{
	pid_t		bgwriter_pid;	/* PID of bgwriter (0 if not started) */

	slock_t		ckpt_lck;		/* protects all the ckpt_* fields */

	int			ckpt_started;	/* advances when checkpoint starts */
	int			ckpt_done;		/* advances when checkpoint done */
	int			ckpt_failed;	/* advances when checkpoint fails */

	int			ckpt_flags;		/* checkpoint flags, as defined in xlog.h */

	uint32		num_backend_writes;		/* counts non-bgwriter buffer writes */
	uint32		num_backend_fsync;		/* counts non-bgwriter fsync calls */

	int			num_requests;	/* current # of requests */
	int			max_requests;	/* allocated array size */
	BgWriterRequest requests[1];	/* VARIABLE LENGTH ARRAY */
} BgWriterShmemStruct;

static BgWriterShmemStruct *BgWriterShmem;

/* interval for calling AbsorbFsyncRequests in CheckpointWriteDelay */
#define WRITES_PER_ABSORB		1000

/*
 * GUC parameters
 */
int			BgWriterDelay = 200;
int			CheckPointTimeout = 300;
int			CheckPointWarning = 30;
double		CheckPointCompletionTarget = 0.5;

/*
 * Flags set by interrupt handlers for later service in the main loop.
 */
static volatile sig_atomic_t got_SIGHUP = false;
static volatile sig_atomic_t checkpoint_requested = false;
static volatile sig_atomic_t shutdown_requested = false;

/*
 * Private state
 */
static bool am_bg_writer = false;

static bool ckpt_active = false;

/* these values are valid when ckpt_active is true: */
static pg_time_t ckpt_start_time;
static XLogRecPtr ckpt_start_recptr;
static double ckpt_cached_elapsed;

static pg_time_t last_checkpoint_time;
static pg_time_t last_xlog_switch_time;

/* Prototypes for private functions */

static void CheckArchiveTimeout(void);
static void BgWriterNap(void);
static bool IsCheckpointOnSchedule(double progress);
static bool ImmediateCheckpointRequested(void);
static bool CompactBgwriterRequestQueue(void);

/* Signal handlers */

static void bg_quickdie(SIGNAL_ARGS);
static void BgSigHupHandler(SIGNAL_ARGS);
static void ReqCheckpointHandler(SIGNAL_ARGS);
static void ReqShutdownHandler(SIGNAL_ARGS);


/*
 * Main entry point for bgwriter process
 *
 * This is invoked from BootstrapMain, which has already created the basic
 * execution environment, but not enabled signals yet.
 */
void
BackgroundWriterMain(void)
{
	sigjmp_buf	local_sigjmp_buf;
	MemoryContext bgwriter_context;

	BgWriterShmem->bgwriter_pid = MyProcPid;
	am_bg_writer = true;

	/*
	 * If possible, make this process a group leader, so that the postmaster
	 * can signal any child processes too.	(bgwriter probably never has any
	 * child processes, but for consistency we make all postmaster child
	 * processes do this.)
	 */
#ifdef HAVE_SETSID
	if (setsid() < 0)
		elog(FATAL, "setsid() failed: %m");
#endif

	/*
	 * Properly accept or ignore signals the postmaster might send us
	 *
	 * Note: we deliberately ignore SIGTERM, because during a standard Unix
	 * system shutdown cycle, init will SIGTERM all processes at once.	We
	 * want to wait for the backends to exit, whereupon the postmaster will
	 * tell us it's okay to shut down (via SIGUSR2).
	 *
	 * SIGUSR1 is presently unused; keep it spare in case someday we want this
	 * process to participate in ProcSignal signalling.
	 */
	pqsignal(SIGHUP, BgSigHupHandler);	/* set flag to read config file */
	pqsignal(SIGINT, ReqCheckpointHandler);		/* request checkpoint */
	pqsignal(SIGTERM, SIG_IGN); /* ignore SIGTERM */
	pqsignal(SIGQUIT, bg_quickdie);		/* hard crash time */
	pqsignal(SIGALRM, SIG_IGN);
	pqsignal(SIGPIPE, SIG_IGN);
	pqsignal(SIGUSR1, SIG_IGN); /* reserve for ProcSignal */
	pqsignal(SIGUSR2, ReqShutdownHandler);		/* request shutdown */

	/*
	 * Reset some signals that are accepted by postmaster but not here
	 */
	pqsignal(SIGCHLD, SIG_DFL);
	pqsignal(SIGTTIN, SIG_DFL);
	pqsignal(SIGTTOU, SIG_DFL);
	pqsignal(SIGCONT, SIG_DFL);
	pqsignal(SIGWINCH, SIG_DFL);

	/* We allow SIGQUIT (quickdie) at all times */
	sigdelset(&BlockSig, SIGQUIT);

	/*
	 * Initialize so that first time-driven event happens at the correct time.
	 */
	last_checkpoint_time = last_xlog_switch_time = (pg_time_t) time(NULL);

	/*
	 * Create a resource owner to keep track of our resources (currently only
	 * buffer pins).
	 */
	CurrentResourceOwner = ResourceOwnerCreate(NULL, "Background Writer");

	/*
	 * Create a memory context that we will do all our work in.  We do this so
	 * that we can reset the context during error recovery and thereby avoid
	 * possible memory leaks.  Formerly this code just ran in
	 * TopMemoryContext, but resetting that would be a really bad idea.
	 */
	bgwriter_context = AllocSetContextCreate(TopMemoryContext,
											 "Background Writer",
											 ALLOCSET_DEFAULT_MINSIZE,
											 ALLOCSET_DEFAULT_INITSIZE,
											 ALLOCSET_DEFAULT_MAXSIZE);
	MemoryContextSwitchTo(bgwriter_context);

	/*
	 * If an exception is encountered, processing resumes here.
	 *
	 * See notes in postgres.c about the design of this coding.
	 */
	if (sigsetjmp(local_sigjmp_buf, 1) != 0)
	{
		/* Since not using PG_TRY, must reset error stack by hand */
		error_context_stack = NULL;

		/* Prevent interrupts while cleaning up */
		HOLD_INTERRUPTS();

		/* Report the error to the server log */
		EmitErrorReport();

		/*
		 * These operations are really just a minimal subset of
		 * AbortTransaction().	We don't have very many resources to worry
		 * about in bgwriter, but we do have LWLocks, buffers, and temp files.
		 */
		LWLockReleaseAll();
		AbortBufferIO();
		UnlockBuffers();
		/* buffer pins are released here: */
		ResourceOwnerRelease(CurrentResourceOwner,
							 RESOURCE_RELEASE_BEFORE_LOCKS,
							 false, true);
		/* we needn't bother with the other ResourceOwnerRelease phases */
		AtEOXact_Buffers(false);
		AtEOXact_SMgr();
		AtEOXact_Files();
		AtEOXact_HashTables(false);

		/* Warn any waiting backends that the checkpoint failed. */
		if (ckpt_active)
		{
			/* use volatile pointer to prevent code rearrangement */
			volatile BgWriterShmemStruct *bgs = BgWriterShmem;

			SpinLockAcquire(&bgs->ckpt_lck);
			bgs->ckpt_failed++;
			bgs->ckpt_done = bgs->ckpt_started;
			SpinLockRelease(&bgs->ckpt_lck);

			ckpt_active = false;
		}

		/*
		 * Now return to normal top-level context and clear ErrorContext for
		 * next time.
		 */
		MemoryContextSwitchTo(bgwriter_context);
		FlushErrorState();

		/* Flush any leaked data in the top-level context */
		MemoryContextResetAndDeleteChildren(bgwriter_context);

		/* Now we can allow interrupts again */
		RESUME_INTERRUPTS();

		/*
		 * Sleep at least 1 second after any error.  A write error is likely
		 * to be repeated, and we don't want to be filling the error logs as
		 * fast as we can.
		 */
		pg_usleep(1000000L);

		/*
		 * Close all open files after any error.  This is helpful on Windows,
		 * where holding deleted files open causes various strange errors.
		 * It's not clear we need it elsewhere, but shouldn't hurt.
		 */
		smgrcloseall();
	}

	/* We can now handle ereport(ERROR) */
	PG_exception_stack = &local_sigjmp_buf;

	/*
	 * Unblock signals (they were blocked when the postmaster forked us)
	 */
	PG_SETMASK(&UnBlockSig);

	/*
	 * Use the recovery target timeline ID during recovery
	 */
	if (RecoveryInProgress())
		ThisTimeLineID = GetRecoveryTargetTLI();

	/* Do this once before starting the loop, then just at SIGHUP time. */
	SyncRepUpdateSyncStandbysDefined();

	/*
	 * Loop forever
	 */
	for (;;)
	{
		bool		do_checkpoint = false;
		int			flags = 0;
		pg_time_t	now;
		int			elapsed_secs;

		/*
		 * Emergency bailout if postmaster has died.  This is to avoid the
		 * necessity for manual cleanup of all postmaster children.
		 */
		if (!PostmasterIsAlive(true))
			exit(1);

		/*
		 * Process any requests or signals received recently.
		 */
		AbsorbFsyncRequests();

		if (got_SIGHUP)
		{
			got_SIGHUP = false;
			ProcessConfigFile(PGC_SIGHUP);
			/* update global shmem state for sync rep */
			SyncRepUpdateSyncStandbysDefined();
		}
		if (checkpoint_requested)
		{
			checkpoint_requested = false;
			do_checkpoint = true;
			BgWriterStats.m_requested_checkpoints++;
		}
		if (shutdown_requested)
		{
			/*
			 * From here on, elog(ERROR) should end with exit(1), not send
			 * control back to the sigsetjmp block above
			 */
			ExitOnAnyError = true;
			/* Close down the database */
			ShutdownXLOG(0, 0);
			/* Normal exit from the bgwriter is here */
			proc_exit(0);		/* done */
		}

		/*
		 * Force a checkpoint if too much time has elapsed since the last one.
		 * Note that we count a timed checkpoint in stats only when this
		 * occurs without an external request, but we set the CAUSE_TIME flag
		 * bit even if there is also an external request.
		 */
		now = (pg_time_t) time(NULL);
		elapsed_secs = now - last_checkpoint_time;
		if (elapsed_secs >= CheckPointTimeout)
		{
			if (!do_checkpoint)
				BgWriterStats.m_timed_checkpoints++;
			do_checkpoint = true;
			flags |= CHECKPOINT_CAUSE_TIME;
		}

		/*
		 * Do a checkpoint if requested, otherwise do one cycle of
		 * dirty-buffer writing.
		 */
		if (do_checkpoint)
		{
			bool		ckpt_performed = false;
			bool		do_restartpoint;

			/* use volatile pointer to prevent code rearrangement */
			volatile BgWriterShmemStruct *bgs = BgWriterShmem;

			/*
			 * Check if we should perform a checkpoint or a restartpoint. As a
			 * side-effect, RecoveryInProgress() initializes TimeLineID if
			 * it's not set yet.
			 */
			do_restartpoint = RecoveryInProgress();

			/*
			 * Atomically fetch the request flags to figure out what kind of a
			 * checkpoint we should perform, and increase the started-counter
			 * to acknowledge that we've started a new checkpoint.
			 */
			SpinLockAcquire(&bgs->ckpt_lck);
			flags |= bgs->ckpt_flags;
			bgs->ckpt_flags = 0;
			bgs->ckpt_started++;
			SpinLockRelease(&bgs->ckpt_lck);

			/*
			 * The end-of-recovery checkpoint is a real checkpoint that's
			 * performed while we're still in recovery.
			 */
			if (flags & CHECKPOINT_END_OF_RECOVERY)
				do_restartpoint = false;

			/*
			 * We will warn if (a) too soon since last checkpoint (whatever
			 * caused it) and (b) somebody set the CHECKPOINT_CAUSE_XLOG flag
			 * since the last checkpoint start.  Note in particular that this
			 * implementation will not generate warnings caused by
			 * CheckPointTimeout < CheckPointWarning.
			 */
			if (!do_restartpoint &&
				(flags & CHECKPOINT_CAUSE_XLOG) &&
				elapsed_secs < CheckPointWarning)
				ereport(LOG,
						(errmsg_plural("checkpoints are occurring too frequently (%d second apart)",
				"checkpoints are occurring too frequently (%d seconds apart)",
									   elapsed_secs,
									   elapsed_secs),
						 errhint("Consider increasing the configuration parameter \"checkpoint_segments\".")));

			/*
			 * Initialize bgwriter-private variables used during checkpoint.
			 */
			ckpt_active = true;
			if (!do_restartpoint)
				ckpt_start_recptr = GetInsertRecPtr();
			ckpt_start_time = now;
			ckpt_cached_elapsed = 0;

			/*
			 * Do the checkpoint.
			 */
			if (!do_restartpoint)
			{
				CreateCheckPoint(flags);
				ckpt_performed = true;
			}
			else
				ckpt_performed = CreateRestartPoint(flags);

			/*
			 * After any checkpoint, close all smgr files.	This is so we
			 * won't hang onto smgr references to deleted files indefinitely.
			 */
			smgrcloseall();

			/*
			 * Indicate checkpoint completion to any waiting backends.
			 */
			SpinLockAcquire(&bgs->ckpt_lck);
			bgs->ckpt_done = bgs->ckpt_started;
			SpinLockRelease(&bgs->ckpt_lck);

			if (ckpt_performed)
			{
				/*
				 * Note we record the checkpoint start time not end time as
				 * last_checkpoint_time.  This is so that time-driven
				 * checkpoints happen at a predictable spacing.
				 */
				last_checkpoint_time = now;
			}
			else
			{
				/*
				 * We were not able to perform the restartpoint (checkpoints
				 * throw an ERROR in case of error).  Most likely because we
				 * have not received any new checkpoint WAL records since the
				 * last restartpoint. Try again in 15 s.
				 */
				last_checkpoint_time = now - CheckPointTimeout + 15;
			}

			ckpt_active = false;
		}
		else
			BgBufferSync();

		/* Check for archive_timeout and switch xlog files if necessary. */
		CheckArchiveTimeout();

		/* Nap for the configured time. */
		BgWriterNap();
	}
}

/*
 * CheckArchiveTimeout -- check for archive_timeout and switch xlog files
 *
 * This will switch to a new WAL file and force an archive file write
 * if any activity is recorded in the current WAL file, including just
 * a single checkpoint record.
 */
static void
CheckArchiveTimeout(void)
{
	pg_time_t	now;
	pg_time_t	last_time;

	if (XLogArchiveTimeout <= 0 || RecoveryInProgress())
		return;

	now = (pg_time_t) time(NULL);

	/* First we do a quick check using possibly-stale local state. */
	if ((int) (now - last_xlog_switch_time) < XLogArchiveTimeout)
		return;

	/*
	 * Update local state ... note that last_xlog_switch_time is the last time
	 * a switch was performed *or requested*.
	 */
	last_time = GetLastSegSwitchTime();

	last_xlog_switch_time = Max(last_xlog_switch_time, last_time);

	/* Now we can do the real check */
	if ((int) (now - last_xlog_switch_time) >= XLogArchiveTimeout)
	{
		XLogRecPtr	switchpoint;

		/* OK, it's time to switch */
		switchpoint = RequestXLogSwitch();

		/*
		 * If the returned pointer points exactly to a segment boundary,
		 * assume nothing happened.
		 */
		if ((switchpoint.xrecoff % XLogSegSize) != 0)
			ereport(DEBUG1,
				(errmsg("transaction log switch forced (archive_timeout=%d)",
						XLogArchiveTimeout)));

		/*
		 * Update state in any case, so we don't retry constantly when the
		 * system is idle.
		 */
		last_xlog_switch_time = now;
	}
}

/*
 * BgWriterNap -- Nap for the configured time or until a signal is received.
 */
static void
BgWriterNap(void)
{
	long		udelay;

	/*
	 * Send off activity statistics to the stats collector
	 */
	pgstat_send_bgwriter();

	/*
	 * Nap for the configured time, or sleep for 10 seconds if there is no
	 * bgwriter activity configured.
	 *
	 * On some platforms, signals won't interrupt the sleep.  To ensure we
	 * respond reasonably promptly when someone signals us, break down the
	 * sleep into 1-second increments, and check for interrupts after each
	 * nap.
	 *
	 * We absorb pending requests after each short sleep.
	 */
	if (bgwriter_lru_maxpages > 0 || ckpt_active)
		udelay = BgWriterDelay * 1000L;
	else if (XLogArchiveTimeout > 0)
		udelay = 1000000L;		/* One second */
	else
		udelay = 10000000L;		/* Ten seconds */

	while (udelay > 999999L)
	{
		if (got_SIGHUP || shutdown_requested ||
		(ckpt_active ? ImmediateCheckpointRequested() : checkpoint_requested))
			break;
		pg_usleep(1000000L);
		AbsorbFsyncRequests();
		udelay -= 1000000L;
	}

	if (!(got_SIGHUP || shutdown_requested ||
	  (ckpt_active ? ImmediateCheckpointRequested() : checkpoint_requested)))
		pg_usleep(udelay);
}

/*
 * Returns true if an immediate checkpoint request is pending.	(Note that
 * this does not check the *current* checkpoint's IMMEDIATE flag, but whether
 * there is one pending behind it.)
 */
static bool
ImmediateCheckpointRequested(void)
{
	if (checkpoint_requested)
	{
		volatile BgWriterShmemStruct *bgs = BgWriterShmem;

		/*
		 * We don't need to acquire the ckpt_lck in this case because we're
		 * only looking at a single flag bit.
		 */
		if (bgs->ckpt_flags & CHECKPOINT_IMMEDIATE)
			return true;
	}
	return false;
}

/*
 * CheckpointWriteDelay -- yield control to bgwriter during a checkpoint
 *
 * This function is called after each page write performed by BufferSync().
 * It is responsible for keeping the bgwriter's normal activities in
 * progress during a long checkpoint, and for throttling BufferSync()'s
 * write rate to hit checkpoint_completion_target.
 *
 * The checkpoint request flags should be passed in; currently the only one
 * examined is CHECKPOINT_IMMEDIATE, which disables delays between writes.
 *
 * 'progress' is an estimate of how much of the work has been done, as a
 * fraction between 0.0 meaning none, and 1.0 meaning all done.
 */
void
CheckpointWriteDelay(int flags, double progress)
{
	static int	absorb_counter = WRITES_PER_ABSORB;

	/* Do nothing if checkpoint is being executed by non-bgwriter process */
	if (!am_bg_writer)
		return;

	/*
	 * Perform the usual bgwriter duties and take a nap, unless we're behind
	 * schedule, in which case we just try to catch up as quickly as possible.
	 */
	if (!(flags & CHECKPOINT_IMMEDIATE) &&
		!shutdown_requested &&
		!ImmediateCheckpointRequested() &&
		IsCheckpointOnSchedule(progress))
	{
		if (got_SIGHUP)
		{
			got_SIGHUP = false;
			ProcessConfigFile(PGC_SIGHUP);
			/* update global shmem state for sync rep */
			SyncRepUpdateSyncStandbysDefined();
		}

		AbsorbFsyncRequests();
		absorb_counter = WRITES_PER_ABSORB;

		BgBufferSync();
		CheckArchiveTimeout();
		BgWriterNap();
	}
	else if (--absorb_counter <= 0)
	{
		/*
		 * Absorb pending fsync requests after each WRITES_PER_ABSORB write
		 * operations even when we don't sleep, to prevent overflow of the
		 * fsync request queue.
		 */
		AbsorbFsyncRequests();
		absorb_counter = WRITES_PER_ABSORB;
	}
}

/*
 * IsCheckpointOnSchedule -- are we on schedule to finish this checkpoint
 *		 in time?
 *
 * Compares the current progress against the time/segments elapsed since last
 * checkpoint, and returns true if the progress we've made this far is greater
 * than the elapsed time/segments.
 */
static bool
IsCheckpointOnSchedule(double progress)
{
	XLogRecPtr	recptr;
	struct timeval now;
	double		elapsed_xlogs,
				elapsed_time;

	Assert(ckpt_active);

	/* Scale progress according to checkpoint_completion_target. */
	progress *= CheckPointCompletionTarget;

	/*
	 * Check against the cached value first. Only do the more expensive
	 * calculations once we reach the target previously calculated. Since
	 * neither time or WAL insert pointer moves backwards, a freshly
	 * calculated value can only be greater than or equal to the cached value.
	 */
	if (progress < ckpt_cached_elapsed)
		return false;

	/*
	 * Check progress against WAL segments written and checkpoint_segments.
	 *
	 * We compare the current WAL insert location against the location
	 * computed before calling CreateCheckPoint. The code in XLogInsert that
	 * actually triggers a checkpoint when checkpoint_segments is exceeded
	 * compares against RedoRecptr, so this is not completely accurate.
	 * However, it's good enough for our purposes, we're only calculating an
	 * estimate anyway.
	 */
	if (!RecoveryInProgress())
	{
		recptr = GetInsertRecPtr();
		elapsed_xlogs =
			(((double) (int32) (recptr.xlogid - ckpt_start_recptr.xlogid)) * XLogSegsPerFile +
			 ((double) recptr.xrecoff - (double) ckpt_start_recptr.xrecoff) / XLogSegSize) /
			CheckPointSegments;

		if (progress < elapsed_xlogs)
		{
			ckpt_cached_elapsed = elapsed_xlogs;
			return false;
		}
	}

	/*
	 * Check progress against time elapsed and checkpoint_timeout.
	 */
	gettimeofday(&now, NULL);
	elapsed_time = ((double) ((pg_time_t) now.tv_sec - ckpt_start_time) +
					now.tv_usec / 1000000.0) / CheckPointTimeout;

	if (progress < elapsed_time)
	{
		ckpt_cached_elapsed = elapsed_time;
		return false;
	}

	/* It looks like we're on schedule. */
	return true;
}


/* --------------------------------
 *		signal handler routines
 * --------------------------------
 */

/*
 * bg_quickdie() occurs when signalled SIGQUIT by the postmaster.
 *
 * Some backend has bought the farm,
 * so we need to stop what we're doing and exit.
 */
static void
bg_quickdie(SIGNAL_ARGS)
{
	PG_SETMASK(&BlockSig);

	/*
	 * We DO NOT want to run proc_exit() callbacks -- we're here because
	 * shared memory may be corrupted, so we don't want to try to clean up our
	 * transaction.  Just nail the windows shut and get out of town.  Now that
	 * there's an atexit callback to prevent third-party code from breaking
	 * things by calling exit() directly, we have to reset the callbacks
	 * explicitly to make this work as intended.
	 */
	on_exit_reset();

	/*
	 * Note we do exit(2) not exit(0).	This is to force the postmaster into a
	 * system reset cycle if some idiot DBA sends a manual SIGQUIT to a random
	 * backend.  This is necessary precisely because we don't clean up our
	 * shared memory state.  (The "dead man switch" mechanism in pmsignal.c
	 * should ensure the postmaster sees this as a crash, too, but no harm in
	 * being doubly sure.)
	 */
	exit(2);
}

/* SIGHUP: set flag to re-read config file at next convenient time */
static void
BgSigHupHandler(SIGNAL_ARGS)
{
	got_SIGHUP = true;
}

/* SIGINT: set flag to run a normal checkpoint right away */
static void
ReqCheckpointHandler(SIGNAL_ARGS)
{
	checkpoint_requested = true;
}

/* SIGUSR2: set flag to run a shutdown checkpoint and exit */
static void
ReqShutdownHandler(SIGNAL_ARGS)
{
	shutdown_requested = true;
}


/* --------------------------------
 *		communication with backends
 * --------------------------------
 */

/*
 * BgWriterShmemSize
 *		Compute space needed for bgwriter-related shared memory
 */
Size
BgWriterShmemSize(void)
{
	Size		size;

	/*
	 * Currently, the size of the requests[] array is arbitrarily set equal to
	 * NBuffers.  This may prove too large or small ...
	 */
	size = offsetof(BgWriterShmemStruct, requests);
	size = add_size(size, mul_size(NBuffers, sizeof(BgWriterRequest)));

	return size;
}

/*
 * BgWriterShmemInit
 *		Allocate and initialize bgwriter-related shared memory
 */
void
BgWriterShmemInit(void)
{
	Size		size = BgWriterShmemSize();
	bool		found;

	BgWriterShmem = (BgWriterShmemStruct *)
		ShmemInitStruct("Background Writer Data",
						size,
						&found);

	if (!found)
	{
		/*
		 * First time through, so initialize.  Note that we zero the whole
		 * requests array; this is so that CompactBgwriterRequestQueue
		 * can assume that any pad bytes in the request structs are zeroes.
		 */
		MemSet(BgWriterShmem, 0, size);
		SpinLockInit(&BgWriterShmem->ckpt_lck);
		BgWriterShmem->max_requests = NBuffers;
	}
}

/*
 * RequestCheckpoint
 *		Called in backend processes to request a checkpoint
 *
 * flags is a bitwise OR of the following:
 *	CHECKPOINT_IS_SHUTDOWN: checkpoint is for database shutdown.
 *	CHECKPOINT_END_OF_RECOVERY: checkpoint is for end of WAL recovery.
 *	CHECKPOINT_IMMEDIATE: finish the checkpoint ASAP,
 *		ignoring checkpoint_completion_target parameter.
 *	CHECKPOINT_FORCE: force a checkpoint even if no XLOG activity has occured
 *		since the last one (implied by CHECKPOINT_IS_SHUTDOWN or
 *		CHECKPOINT_END_OF_RECOVERY).
 *	CHECKPOINT_WAIT: wait for completion before returning (otherwise,
 *		just signal bgwriter to do it, and return).
 *	CHECKPOINT_CAUSE_XLOG: checkpoint is requested due to xlog filling.
 *		(This affects logging, and in particular enables CheckPointWarning.)
 */
void
RequestCheckpoint(int flags)
{
	/* use volatile pointer to prevent code rearrangement */
	volatile BgWriterShmemStruct *bgs = BgWriterShmem;
	int			ntries;
	int			old_failed,
				old_started;

	/*
	 * If in a standalone backend, just do it ourselves.
	 */
	if (!IsPostmasterEnvironment)
	{
		/*
		 * There's no point in doing slow checkpoints in a standalone backend,
		 * because there's no other backends the checkpoint could disrupt.
		 */
		CreateCheckPoint(flags | CHECKPOINT_IMMEDIATE);

		/*
		 * After any checkpoint, close all smgr files.	This is so we won't
		 * hang onto smgr references to deleted files indefinitely.
		 */
		smgrcloseall();

		return;
	}

	/*
	 * Atomically set the request flags, and take a snapshot of the counters.
	 * When we see ckpt_started > old_started, we know the flags we set here
	 * have been seen by bgwriter.
	 *
	 * Note that we OR the flags with any existing flags, to avoid overriding
	 * a "stronger" request by another backend.  The flag senses must be
	 * chosen to make this work!
	 */
	SpinLockAcquire(&bgs->ckpt_lck);

	old_failed = bgs->ckpt_failed;
	old_started = bgs->ckpt_started;
	bgs->ckpt_flags |= flags;

	SpinLockRelease(&bgs->ckpt_lck);

	/*
	 * Send signal to request checkpoint.  It's possible that the bgwriter
	 * hasn't started yet, or is in process of restarting, so we will retry a
	 * few times if needed.  Also, if not told to wait for the checkpoint to
	 * occur, we consider failure to send the signal to be nonfatal and merely
	 * LOG it.
	 */
	for (ntries = 0;; ntries++)
	{
		if (BgWriterShmem->bgwriter_pid == 0)
		{
			if (ntries >= 20)	/* max wait 2.0 sec */
			{
				elog((flags & CHECKPOINT_WAIT) ? ERROR : LOG,
				"could not request checkpoint because bgwriter not running");
				break;
			}
		}
		else if (kill(BgWriterShmem->bgwriter_pid, SIGINT) != 0)
		{
			if (ntries >= 20)	/* max wait 2.0 sec */
			{
				elog((flags & CHECKPOINT_WAIT) ? ERROR : LOG,
					 "could not signal for checkpoint: %m");
				break;
			}
		}
		else
			break;				/* signal sent successfully */

		CHECK_FOR_INTERRUPTS();
		pg_usleep(100000L);		/* wait 0.1 sec, then retry */
	}

	/*
	 * If requested, wait for completion.  We detect completion according to
	 * the algorithm given above.
	 */
	if (flags & CHECKPOINT_WAIT)
	{
		int			new_started,
					new_failed;

		/* Wait for a new checkpoint to start. */
		for (;;)
		{
			SpinLockAcquire(&bgs->ckpt_lck);
			new_started = bgs->ckpt_started;
			SpinLockRelease(&bgs->ckpt_lck);

			if (new_started != old_started)
				break;

			CHECK_FOR_INTERRUPTS();
			pg_usleep(100000L);
		}

		/*
		 * We are waiting for ckpt_done >= new_started, in a modulo sense.
		 */
		for (;;)
		{
			int			new_done;

			SpinLockAcquire(&bgs->ckpt_lck);
			new_done = bgs->ckpt_done;
			new_failed = bgs->ckpt_failed;
			SpinLockRelease(&bgs->ckpt_lck);

			if (new_done - new_started >= 0)
				break;

			CHECK_FOR_INTERRUPTS();
			pg_usleep(100000L);
		}

		if (new_failed != old_failed)
			ereport(ERROR,
					(errmsg("checkpoint request failed"),
					 errhint("Consult recent messages in the server log for details.")));
	}
}

/*
 * ForwardFsyncRequest
 *		Forward a file-fsync request from a backend to the bgwriter
 *
 * Whenever a backend is compelled to write directly to a relation
 * (which should be seldom, if the bgwriter is getting its job done),
 * the backend calls this routine to pass over knowledge that the relation
 * is dirty and must be fsync'd before next checkpoint.  We also use this
 * opportunity to count such writes for statistical purposes.
 *
 * This functionality is only supported for regular (not backend-local)
 * relations, so the rnode argument is intentionally RelFileNode not
 * RelFileNodeBackend.
 *
 * segno specifies which segment (not block!) of the relation needs to be
 * fsync'd.  (Since the valid range is much less than BlockNumber, we can
 * use high values for special flags; that's all internal to md.c, which
 * see for details.)
 *
 * To avoid holding the lock for longer than necessary, we normally write
 * to the requests[] queue without checking for duplicates.  The bgwriter
 * will have to eliminate dups internally anyway.  However, if we discover
 * that the queue is full, we make a pass over the entire queue to compact
 * it.	This is somewhat expensive, but the alternative is for the backend
 * to perform its own fsync, which is far more expensive in practice.  It
 * is theoretically possible a backend fsync might still be necessary, if
 * the queue is full and contains no duplicate entries.  In that case, we
 * let the backend know by returning false.
 */
bool
ForwardFsyncRequest(RelFileNode rnode, ForkNumber forknum, BlockNumber segno)
{
	BgWriterRequest *request;

	if (!IsUnderPostmaster)
		return false;			/* probably shouldn't even get here */

	if (am_bg_writer)
		elog(ERROR, "ForwardFsyncRequest must not be called in bgwriter");

	LWLockAcquire(BgWriterCommLock, LW_EXCLUSIVE);

	/* Count all backend writes regardless of if they fit in the queue */
	BgWriterShmem->num_backend_writes++;

	/*
	 * If the background writer isn't running or the request queue is full,
	 * the backend will have to perform its own fsync request.	But before
	 * forcing that to happen, we can try to compact the background writer
	 * request queue.
	 */
	if (BgWriterShmem->bgwriter_pid == 0 ||
		(BgWriterShmem->num_requests >= BgWriterShmem->max_requests
		 && !CompactBgwriterRequestQueue()))
	{
		/*
		 * Count the subset of writes where backends have to do their own
		 * fsync
		 */
		BgWriterShmem->num_backend_fsync++;
		LWLockRelease(BgWriterCommLock);
		return false;
	}
	request = &BgWriterShmem->requests[BgWriterShmem->num_requests++];
	request->rnode = rnode;
	request->forknum = forknum;
	request->segno = segno;
	LWLockRelease(BgWriterCommLock);
	return true;
}

/*
 * CompactBgwriterRequestQueue
 *		Remove duplicates from the request queue to avoid backend fsyncs.
 *		Returns "true" if any entries were removed.
 *
 * Although a full fsync request queue is not common, it can lead to severe
 * performance problems when it does happen.  So far, this situation has
 * only been observed to occur when the system is under heavy write load,
 * and especially during the "sync" phase of a checkpoint.	Without this
 * logic, each backend begins doing an fsync for every block written, which
 * gets very expensive and can slow down the whole system.
 *
 * Trying to do this every time the queue is full could lose if there
 * aren't any removable entries.  But that should be vanishingly rare in
 * practice: there's one queue entry per shared buffer.
 */
static bool
CompactBgwriterRequestQueue(void)
{
	struct BgWriterSlotMapping
	{
		BgWriterRequest request;
		int			slot;
	};

	int			n,
				preserve_count;
	int			num_skipped = 0;
	HASHCTL		ctl;
	HTAB	   *htab;
	bool	   *skip_slot;

	/* must hold BgWriterCommLock in exclusive mode */
	Assert(LWLockHeldByMe(BgWriterCommLock));

	/* Initialize skip_slot array */
	skip_slot = palloc0(sizeof(bool) * BgWriterShmem->num_requests);

	/* Initialize temporary hash table */
	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(BgWriterRequest);
	ctl.entrysize = sizeof(struct BgWriterSlotMapping);
	ctl.hash = tag_hash;
	ctl.hcxt = CurrentMemoryContext;

	htab = hash_create("CompactBgwriterRequestQueue",
					   BgWriterShmem->num_requests,
					   &ctl,
					   HASH_ELEM | HASH_FUNCTION | HASH_CONTEXT);

	/*
	 * The basic idea here is that a request can be skipped if it's followed
	 * by a later, identical request.  It might seem more sensible to work
	 * backwards from the end of the queue and check whether a request is
	 * *preceded* by an earlier, identical request, in the hopes of doing less
	 * copying.  But that might change the semantics, if there's an
	 * intervening FORGET_RELATION_FSYNC or FORGET_DATABASE_FSYNC request, so
	 * we do it this way.  It would be possible to be even smarter if we made
	 * the code below understand the specific semantics of such requests (it
	 * could blow away preceding entries that would end up being canceled
	 * anyhow), but it's not clear that the extra complexity would buy us
	 * anything.
	 */
	for (n = 0; n < BgWriterShmem->num_requests; n++)
	{
		BgWriterRequest *request;
		struct BgWriterSlotMapping *slotmap;
		bool		found;

		/*
		 * We use the request struct directly as a hashtable key.  This
		 * assumes that any padding bytes in the structs are consistently the
		 * same, which should be okay because we zeroed them in
		 * BgWriterShmemInit.  Note also that RelFileNode had better
		 * contain no pad bytes.
		 */
		request = &BgWriterShmem->requests[n];
		slotmap = hash_search(htab, request, HASH_ENTER, &found);
		if (found)
		{
			/* Duplicate, so mark the previous occurrence as skippable */
			skip_slot[slotmap->slot] = true;
			num_skipped++;
		}
		/* Remember slot containing latest occurrence of this request value */
		slotmap->slot = n;
	}

	/* Done with the hash table. */
	hash_destroy(htab);

	/* If no duplicates, we're out of luck. */
	if (!num_skipped)
	{
		pfree(skip_slot);
		return false;
	}

	/* We found some duplicates; remove them. */
	preserve_count = 0;
	for (n = 0; n < BgWriterShmem->num_requests; n++)
	{
		if (skip_slot[n])
			continue;
		BgWriterShmem->requests[preserve_count++] = BgWriterShmem->requests[n];
	}
	ereport(DEBUG1,
	   (errmsg("compacted fsync request queue from %d entries to %d entries",
			   BgWriterShmem->num_requests, preserve_count)));
	BgWriterShmem->num_requests = preserve_count;

	/* Cleanup. */
	pfree(skip_slot);
	return true;
}

/*
 * AbsorbFsyncRequests
 *		Retrieve queued fsync requests and pass them to local smgr.
 *
 * This is exported because it must be called during CreateCheckPoint;
 * we have to be sure we have accepted all pending requests just before
 * we start fsync'ing.  Since CreateCheckPoint sometimes runs in
 * non-bgwriter processes, do nothing if not bgwriter.
 */
void
AbsorbFsyncRequests(void)
{
	BgWriterRequest *requests = NULL;
	BgWriterRequest *request;
	int			n;

	if (!am_bg_writer)
		return;

	/*
	 * We have to PANIC if we fail to absorb all the pending requests (eg,
	 * because our hashtable runs out of memory).  This is because the system
	 * cannot run safely if we are unable to fsync what we have been told to
	 * fsync.  Fortunately, the hashtable is so small that the problem is
	 * quite unlikely to arise in practice.
	 */
	START_CRIT_SECTION();

	/*
	 * We try to avoid holding the lock for a long time by copying the request
	 * array.
	 */
	LWLockAcquire(BgWriterCommLock, LW_EXCLUSIVE);

	/* Transfer write count into pending pgstats message */
	BgWriterStats.m_buf_written_backend += BgWriterShmem->num_backend_writes;
	BgWriterStats.m_buf_fsync_backend += BgWriterShmem->num_backend_fsync;

	BgWriterShmem->num_backend_writes = 0;
	BgWriterShmem->num_backend_fsync = 0;

	n = BgWriterShmem->num_requests;
	if (n > 0)
	{
		requests = (BgWriterRequest *) palloc(n * sizeof(BgWriterRequest));
		memcpy(requests, BgWriterShmem->requests, n * sizeof(BgWriterRequest));
	}
	BgWriterShmem->num_requests = 0;

	LWLockRelease(BgWriterCommLock);

	for (request = requests; n > 0; request++, n--)
		RememberFsyncRequest(request->rnode, request->forknum, request->segno);

	if (requests)
		pfree(requests);

	END_CRIT_SECTION();
}
