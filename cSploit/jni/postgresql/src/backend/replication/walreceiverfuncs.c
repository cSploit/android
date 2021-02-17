/*-------------------------------------------------------------------------
 *
 * walreceiverfuncs.c
 *
 * This file contains functions used by the startup process to communicate
 * with the walreceiver process. Functions implementing walreceiver itself
 * are in walreceiver.c.
 *
 * Portions Copyright (c) 2010-2011, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/backend/replication/walreceiverfuncs.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "access/xlog_internal.h"
#include "replication/walreceiver.h"
#include "storage/fd.h"
#include "storage/pmsignal.h"
#include "storage/shmem.h"
#include "utils/guc.h"

WalRcvData *WalRcv = NULL;

/*
 * How long to wait for walreceiver to start up after requesting
 * postmaster to launch it. In seconds.
 */
#define WALRCV_STARTUP_TIMEOUT 10

/* Report shared memory space needed by WalRcvShmemInit */
Size
WalRcvShmemSize(void)
{
	Size		size = 0;

	size = add_size(size, sizeof(WalRcvData));

	return size;
}

/* Allocate and initialize walreceiver-related shared memory */
void
WalRcvShmemInit(void)
{
	bool		found;

	WalRcv = (WalRcvData *)
		ShmemInitStruct("Wal Receiver Ctl", WalRcvShmemSize(), &found);

	if (!found)
	{
		/* First time through, so initialize */
		MemSet(WalRcv, 0, WalRcvShmemSize());
		WalRcv->walRcvState = WALRCV_STOPPED;
		SpinLockInit(&WalRcv->mutex);
	}
}

/* Is walreceiver in progress (or starting up)? */
bool
WalRcvInProgress(void)
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	WalRcvState state;
	pg_time_t	startTime;

	SpinLockAcquire(&walrcv->mutex);

	state = walrcv->walRcvState;
	startTime = walrcv->startTime;

	SpinLockRelease(&walrcv->mutex);

	/*
	 * If it has taken too long for walreceiver to start up, give up. Setting
	 * the state to STOPPED ensures that if walreceiver later does start up
	 * after all, it will see that it's not supposed to be running and die
	 * without doing anything.
	 */
	if (state == WALRCV_STARTING)
	{
		pg_time_t	now = (pg_time_t) time(NULL);

		if ((now - startTime) > WALRCV_STARTUP_TIMEOUT)
		{
			SpinLockAcquire(&walrcv->mutex);

			if (walrcv->walRcvState == WALRCV_STARTING)
				state = walrcv->walRcvState = WALRCV_STOPPED;

			SpinLockRelease(&walrcv->mutex);
		}
	}

	if (state != WALRCV_STOPPED)
		return true;
	else
		return false;
}

/*
 * Stop walreceiver (if running) and wait for it to die.
 */
void
ShutdownWalRcv(void)
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	pid_t		walrcvpid = 0;

	/*
	 * Request walreceiver to stop. Walreceiver will switch to WALRCV_STOPPED
	 * mode once it's finished, and will also request postmaster to not
	 * restart itself.
	 */
	SpinLockAcquire(&walrcv->mutex);
	switch (walrcv->walRcvState)
	{
		case WALRCV_STOPPED:
			break;
		case WALRCV_STARTING:
			walrcv->walRcvState = WALRCV_STOPPED;
			break;

		case WALRCV_RUNNING:
			walrcv->walRcvState = WALRCV_STOPPING;
			/* fall through */
		case WALRCV_STOPPING:
			walrcvpid = walrcv->pid;
			break;
	}
	SpinLockRelease(&walrcv->mutex);

	/*
	 * Signal walreceiver process if it was still running.
	 */
	if (walrcvpid != 0)
		kill(walrcvpid, SIGTERM);

	/*
	 * Wait for walreceiver to acknowledge its death by setting state to
	 * WALRCV_STOPPED.
	 */
	while (WalRcvInProgress())
	{
		/*
		 * This possibly-long loop needs to handle interrupts of startup
		 * process.
		 */
		HandleStartupProcInterrupts();

		pg_usleep(100000);		/* 100ms */
	}
}

/*
 * Request postmaster to start walreceiver.
 *
 * recptr indicates the position where streaming should begin, and conninfo
 * is a libpq connection string to use.
 */
void
RequestXLogStreaming(XLogRecPtr recptr, const char *conninfo)
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	pg_time_t	now = (pg_time_t) time(NULL);

	/*
	 * We always start at the beginning of the segment. That prevents a broken
	 * segment (i.e., with no records in the first half of a segment) from
	 * being created by XLOG streaming, which might cause trouble later on if
	 * the segment is e.g archived.
	 */
	if (recptr.xrecoff % XLogSegSize != 0)
		recptr.xrecoff -= recptr.xrecoff % XLogSegSize;

	SpinLockAcquire(&walrcv->mutex);

	/* It better be stopped before we try to restart it */
	Assert(walrcv->walRcvState == WALRCV_STOPPED);

	if (conninfo != NULL)
		strlcpy((char *) walrcv->conninfo, conninfo, MAXCONNINFO);
	else
		walrcv->conninfo[0] = '\0';
	walrcv->walRcvState = WALRCV_STARTING;
	walrcv->startTime = now;

	/*
	 * If this is the first startup of walreceiver, we initialize receivedUpto
	 * and latestChunkStart to receiveStart.
	 */
	if (walrcv->receiveStart.xlogid == 0 &&
		walrcv->receiveStart.xrecoff == 0)
	{
		walrcv->receivedUpto = recptr;
		walrcv->latestChunkStart = recptr;
	}
	walrcv->receiveStart = recptr;

	SpinLockRelease(&walrcv->mutex);

	SendPostmasterSignal(PMSIGNAL_START_WALRECEIVER);
}

/*
 * Returns the last+1 byte position that walreceiver has written.
 *
 * Optionally, returns the previous chunk start, that is the first byte
 * written in the most recent walreceiver flush cycle.	Callers not
 * interested in that value may pass NULL for latestChunkStart.
 */
XLogRecPtr
GetWalRcvWriteRecPtr(XLogRecPtr *latestChunkStart)
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	XLogRecPtr	recptr;

	SpinLockAcquire(&walrcv->mutex);
	recptr = walrcv->receivedUpto;
	if (latestChunkStart)
		*latestChunkStart = walrcv->latestChunkStart;
	SpinLockRelease(&walrcv->mutex);

	return recptr;
}
