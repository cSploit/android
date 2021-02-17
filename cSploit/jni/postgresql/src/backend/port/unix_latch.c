/*-------------------------------------------------------------------------
 *
 * unix_latch.c
 *	  Routines for inter-process latches
 *
 * The Unix implementation uses the so-called self-pipe trick to overcome
 * the race condition involved with select() and setting a global flag
 * in the signal handler. When a latch is set and the current process
 * is waiting for it, the signal handler wakes up the select() in
 * WaitLatch by writing a byte to a pipe. A signal by itself doesn't
 * interrupt select() on all platforms, and even on platforms where it
 * does, a signal that arrives just before the select() call does not
 * prevent the select() from entering sleep. An incoming byte on a pipe
 * however reliably interrupts the sleep, and causes select() to return
 * immediately even if the signal arrives before select() begins.
 *
 * When SetLatch is called from the same process that owns the latch,
 * SetLatch writes the byte directly to the pipe. If it's owned by another
 * process, SIGUSR1 is sent and the signal handler in the waiting process
 * writes the byte to the pipe on behalf of the signaling process.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/port/unix_latch.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "miscadmin.h"
#include "storage/latch.h"
#include "storage/shmem.h"

/* Are we currently in WaitLatch? The signal handler would like to know. */
static volatile sig_atomic_t waiting = false;

/* Read and write ends of the self-pipe */
static int	selfpipe_readfd = -1;
static int	selfpipe_writefd = -1;

/* Private function prototypes */
static void sendSelfPipeByte(void);
static void drainSelfPipe(void);


/*
 * Initialize the process-local latch infrastructure.
 *
 * This must be called once during startup of any process that can wait on
 * latches, before it issues any InitLatch() or OwnLatch() calls.
 */
void
InitializeLatchSupport(void)
{
	int			pipefd[2];

	Assert(selfpipe_readfd == -1);

	/*
	 * Set up the self-pipe that allows a signal handler to wake up the
	 * select() in WaitLatch. Make the write-end non-blocking, so that
	 * SetLatch won't block if the event has already been set many times
	 * filling the kernel buffer. Make the read-end non-blocking too, so that
	 * we can easily clear the pipe by reading until EAGAIN or EWOULDBLOCK.
	 */
	if (pipe(pipefd) < 0)
		elog(FATAL, "pipe() failed: %m");
	if (fcntl(pipefd[0], F_SETFL, O_NONBLOCK) < 0)
		elog(FATAL, "fcntl() failed on read-end of self-pipe: %m");
	if (fcntl(pipefd[1], F_SETFL, O_NONBLOCK) < 0)
		elog(FATAL, "fcntl() failed on write-end of self-pipe: %m");

	selfpipe_readfd = pipefd[0];
	selfpipe_writefd = pipefd[1];
}

/*
 * Initialize a backend-local latch.
 */
void
InitLatch(volatile Latch *latch)
{
	/* Assert InitializeLatchSupport has been called in this process */
	Assert(selfpipe_readfd >= 0);

	latch->is_set = false;
	latch->owner_pid = MyProcPid;
	latch->is_shared = false;
}

/*
 * Initialize a shared latch that can be set from other processes. The latch
 * is initially owned by no-one; use OwnLatch to associate it with the
 * current process.
 *
 * InitSharedLatch needs to be called in postmaster before forking child
 * processes, usually right after allocating the shared memory block
 * containing the latch with ShmemInitStruct. (The Unix implementation
 * doesn't actually require that, but the Windows one does.) Because of
 * this restriction, we have no concurrency issues to worry about here.
 */
void
InitSharedLatch(volatile Latch *latch)
{
	latch->is_set = false;
	latch->owner_pid = 0;
	latch->is_shared = true;
}

/*
 * Associate a shared latch with the current process, allowing it to
 * wait on the latch.
 *
 * Although there is a sanity check for latch-already-owned, we don't do
 * any sort of locking here, meaning that we could fail to detect the error
 * if two processes try to own the same latch at about the same time.  If
 * there is any risk of that, caller must provide an interlock to prevent it.
 *
 * In any process that calls OwnLatch(), make sure that
 * latch_sigusr1_handler() is called from the SIGUSR1 signal handler,
 * as shared latches use SIGUSR1 for inter-process communication.
 */
void
OwnLatch(volatile Latch *latch)
{
	/* Assert InitializeLatchSupport has been called in this process */
	Assert(selfpipe_readfd >= 0);

	Assert(latch->is_shared);

	/* sanity check */
	if (latch->owner_pid != 0)
		elog(ERROR, "latch already owned");

	latch->owner_pid = MyProcPid;
}

/*
 * Disown a shared latch currently owned by the current process.
 */
void
DisownLatch(volatile Latch *latch)
{
	Assert(latch->is_shared);
	Assert(latch->owner_pid == MyProcPid);

	latch->owner_pid = 0;
}

/*
 * Wait for given latch to be set or until timeout is exceeded.
 * If the latch is already set, the function returns immediately.
 *
 * The 'timeout' is given in milliseconds, and -1 means wait forever.
 * On some platforms, signals do not interrupt the wait, or even
 * cause the timeout to be restarted, so beware that the function can sleep
 * for several times longer than the requested timeout.  However, this
 * difficulty is not so great as it seems, because the signal handlers for any
 * signals that the caller should respond to ought to be programmed to end the
 * wait by calling SetLatch.  Ideally, the timeout parameter is vestigial.
 *
 * The latch must be owned by the current process, ie. it must be a
 * backend-local latch initialized with InitLatch, or a shared latch
 * associated with the current process by calling OwnLatch.
 *
 * Returns 'true' if the latch was set, or 'false' if timeout was reached.
 */
bool
WaitLatch(volatile Latch *latch, long timeout)
{
	return WaitLatchOrSocket(latch, PGINVALID_SOCKET, false, false, timeout) > 0;
}

/*
 * Like WaitLatch, but will also return when there's data available in
 * 'sock' for reading or writing. Returns 0 if timeout was reached,
 * 1 if the latch was set, 2 if the socket became readable or writable.
 */
int
WaitLatchOrSocket(volatile Latch *latch, pgsocket sock, bool forRead,
				  bool forWrite, long timeout)
{
	struct timeval tv,
			   *tvp = NULL;
	fd_set		input_mask;
	fd_set		output_mask;
	int			rc;
	int			result = 0;

	if (latch->owner_pid != MyProcPid)
		elog(ERROR, "cannot wait on a latch owned by another process");

	/* Initialize timeout */
	if (timeout >= 0)
	{
		tv.tv_sec = timeout / 1000L;
		tv.tv_usec = (timeout % 1000L) * 1000L;
		tvp = &tv;
	}

	waiting = true;
	for (;;)
	{
		int			hifd;

		/*
		 * Clear the pipe, then check if the latch is set already. If someone
		 * sets the latch between this and the select() below, the setter will
		 * write a byte to the pipe (or signal us and the signal handler will
		 * do that), and the select() will return immediately.
		 *
		 * Note: we assume that the kernel calls involved in drainSelfPipe()
		 * and SetLatch() will provide adequate synchronization on machines
		 * with weak memory ordering, so that we cannot miss seeing is_set
		 * if the signal byte is already in the pipe when we drain it.
		 */
		drainSelfPipe();

		if (latch->is_set)
		{
			result = 1;
			break;
		}

		/* Must wait ... set up the event masks for select() */
		FD_ZERO(&input_mask);
		FD_ZERO(&output_mask);

		FD_SET(selfpipe_readfd, &input_mask);
		hifd = selfpipe_readfd;

		if (sock != PGINVALID_SOCKET && forRead)
		{
			FD_SET(sock, &input_mask);
			if (sock > hifd)
				hifd = sock;
		}

		if (sock != PGINVALID_SOCKET && forWrite)
		{
			FD_SET(sock, &output_mask);
			if (sock > hifd)
				hifd = sock;
		}

		rc = select(hifd + 1, &input_mask, &output_mask, NULL, tvp);
		if (rc < 0)
		{
			if (errno == EINTR)
				continue;
			waiting = false;
			ereport(ERROR,
					(errcode_for_socket_access(),
					 errmsg("select() failed: %m")));
		}
		if (rc == 0)
		{
			/* timeout exceeded */
			result = 0;
			break;
		}
		if (sock != PGINVALID_SOCKET &&
			((forRead && FD_ISSET(sock, &input_mask)) ||
			 (forWrite && FD_ISSET(sock, &output_mask))))
		{
			result = 2;
			break;				/* data available in socket */
		}
	}
	waiting = false;

	return result;
}

/*
 * Sets a latch and wakes up anyone waiting on it.
 *
 * This is cheap if the latch is already set, otherwise not so much.
 *
 * NB: when calling this in a signal handler, be sure to save and restore
 * errno around it.  (That's standard practice in most signal handlers, of
 * course, but we used to omit it in handlers that only set a flag.)
 */
void
SetLatch(volatile Latch *latch)
{
	pid_t		owner_pid;

	/*
	 * XXX there really ought to be a memory barrier operation right here,
	 * to ensure that any flag variables we might have changed get flushed
	 * to main memory before we check/set is_set.  Without that, we have to
	 * require that callers provide their own synchronization for machines
	 * with weak memory ordering (see latch.h).
	 */

	/* Quick exit if already set */
	if (latch->is_set)
		return;

	latch->is_set = true;

	/*
	 * See if anyone's waiting for the latch. It can be the current process if
	 * we're in a signal handler. We use the self-pipe to wake up the select()
	 * in that case. If it's another process, send a signal.
	 *
	 * Fetch owner_pid only once, in case the latch is concurrently getting
	 * owned or disowned. XXX: This assumes that pid_t is atomic, which isn't
	 * guaranteed to be true! In practice, the effective range of pid_t fits
	 * in a 32 bit integer, and so should be atomic. In the worst case, we
	 * might end up signaling the wrong process. Even then, you're very
	 * unlucky if a process with that bogus pid exists and belongs to
	 * Postgres; and PG database processes should handle excess SIGUSR1
	 * interrupts without a problem anyhow.
	 *
	 * Another sort of race condition that's possible here is for a new process
	 * to own the latch immediately after we look, so we don't signal it.
	 * This is okay so long as all callers of ResetLatch/WaitLatch follow the
	 * standard coding convention of waiting at the bottom of their loops,
	 * not the top, so that they'll correctly process latch-setting events that
	 * happen before they enter the loop.
	 */
	owner_pid = latch->owner_pid;
	if (owner_pid == 0)
		return;
	else if (owner_pid == MyProcPid)
	{
		if (waiting)
			sendSelfPipeByte();
	}
	else
		kill(owner_pid, SIGUSR1);
}

/*
 * Clear the latch. Calling WaitLatch after this will sleep, unless
 * the latch is set again before the WaitLatch call.
 */
void
ResetLatch(volatile Latch *latch)
{
	/* Only the owner should reset the latch */
	Assert(latch->owner_pid == MyProcPid);

	latch->is_set = false;

	/*
	 * XXX there really ought to be a memory barrier operation right here, to
	 * ensure that the write to is_set gets flushed to main memory before we
	 * examine any flag variables.  Otherwise a concurrent SetLatch might
	 * falsely conclude that it needn't signal us, even though we have missed
	 * seeing some flag updates that SetLatch was supposed to inform us of.
	 * For the moment, callers must supply their own synchronization of flag
	 * variables (see latch.h).
	 */
}

/*
 * SetLatch uses SIGUSR1 to wake up the process waiting on the latch.
 *
 * Wake up WaitLatch, if we're waiting.  (We might not be, since SIGUSR1 is
 * overloaded for multiple purposes; or we might not have reached WaitLatch
 * yet, in which case we don't need to fill the pipe either.)
 *
 * NB: when calling this in a signal handler, be sure to save and restore
 * errno around it.
 */
void
latch_sigusr1_handler(void)
{
	if (waiting)
		sendSelfPipeByte();
}

/* Send one byte to the self-pipe, to wake up WaitLatch */
static void
sendSelfPipeByte(void)
{
	int			rc;
	char		dummy = 0;

retry:
	rc = write(selfpipe_writefd, &dummy, 1);
	if (rc < 0)
	{
		/* If interrupted by signal, just retry */
		if (errno == EINTR)
			goto retry;

		/*
		 * If the pipe is full, we don't need to retry, the data that's there
		 * already is enough to wake up WaitLatch.
		 */
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;

		/*
		 * Oops, the write() failed for some other reason. We might be in a
		 * signal handler, so it's not safe to elog(). We have no choice but
		 * silently ignore the error.
		 */
		return;
	}
}

/*
 * Read all available data from the self-pipe
 *
 * Note: this is only called when waiting = true.  If it fails and doesn't
 * return, it must reset that flag first (though ideally, this will never
 * happen).
 */
static void
drainSelfPipe(void)
{
	/*
	 * There shouldn't normally be more than one byte in the pipe, or maybe a
	 * few bytes if multiple processes run SetLatch at the same instant.
	 */
	char		buf[16];
	int			rc;

	for (;;)
	{
		rc = read(selfpipe_readfd, buf, sizeof(buf));
		if (rc < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;			/* the pipe is empty */
			else if (errno == EINTR)
				continue;		/* retry */
			else
			{
				waiting = false;
				elog(ERROR, "read() on self-pipe failed: %m");
			}
		}
		else if (rc == 0)
		{
			waiting = false;
			elog(ERROR, "unexpected EOF on self-pipe");
		}
		else if (rc < sizeof(buf))
		{
			/* we successfully drained the pipe; no need to read() again */
			break;
		}
		/* else buffer wasn't big enough, so read again */
	}
}
