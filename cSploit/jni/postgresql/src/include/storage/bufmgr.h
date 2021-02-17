/*-------------------------------------------------------------------------
 *
 * bufmgr.h
 *	  POSTGRES buffer manager definitions.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/storage/bufmgr.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef BUFMGR_H
#define BUFMGR_H

#include "storage/block.h"
#include "storage/buf.h"
#include "storage/bufpage.h"
#include "storage/relfilenode.h"
#include "utils/relcache.h"

typedef void *Block;

/* Possible arguments for GetAccessStrategy() */
typedef enum BufferAccessStrategyType
{
	BAS_NORMAL,					/* Normal random access */
	BAS_BULKREAD,				/* Large read-only scan (hint bit updates are
								 * ok) */
	BAS_BULKWRITE,				/* Large multi-block write (e.g. COPY IN) */
	BAS_VACUUM					/* VACUUM */
} BufferAccessStrategyType;

/* Possible modes for ReadBufferExtended() */
typedef enum
{
	RBM_NORMAL,					/* Normal read */
	RBM_ZERO,					/* Don't read from disk, caller will
								 * initialize */
	RBM_ZERO_ON_ERROR			/* Read, but return an all-zeros page on error */
} ReadBufferMode;

/* in globals.c ... this duplicates miscadmin.h */
extern PGDLLIMPORT int NBuffers;

/* in bufmgr.c */
extern bool zero_damaged_pages;
extern int	bgwriter_lru_maxpages;
extern double bgwriter_lru_multiplier;
extern int	target_prefetch_pages;

/* in buf_init.c */
extern PGDLLIMPORT char *BufferBlocks;
extern PGDLLIMPORT int32 *PrivateRefCount;

/* in localbuf.c */
extern PGDLLIMPORT int NLocBuffer;
extern PGDLLIMPORT Block *LocalBufferBlockPointers;
extern PGDLLIMPORT int32 *LocalRefCount;

/* special block number for ReadBuffer() */
#define P_NEW	InvalidBlockNumber		/* grow the file to get a new page */

/*
 * Buffer content lock modes (mode argument for LockBuffer())
 */
#define BUFFER_LOCK_UNLOCK		0
#define BUFFER_LOCK_SHARE		1
#define BUFFER_LOCK_EXCLUSIVE	2

/*
 * These routines are beaten on quite heavily, hence the macroization.
 */

/*
 * BufferIsValid
 *		True iff the given buffer number is valid (either as a shared
 *		or local buffer).
 *
 * This is not quite the inverse of the BufferIsInvalid() macro, since this
 * adds sanity rangechecks on the buffer number.
 *
 * Note: For a long time this was defined the same as BufferIsPinned,
 * that is it would say False if you didn't hold a pin on the buffer.
 * I believe this was bogus and served only to mask logic errors.
 * Code should always know whether it has a buffer reference,
 * independently of the pin state.
 */
#define BufferIsValid(bufnum) \
( \
	(bufnum) != InvalidBuffer && \
	(bufnum) >= -NLocBuffer && \
	(bufnum) <= NBuffers \
)

/*
 * BufferIsPinned
 *		True iff the buffer is pinned (also checks for valid buffer number).
 *
 *		NOTE: what we check here is that *this* backend holds a pin on
 *		the buffer.  We do not care whether some other backend does.
 */
#define BufferIsPinned(bufnum) \
( \
	!BufferIsValid(bufnum) ? \
		false \
	: \
		BufferIsLocal(bufnum) ? \
			(LocalRefCount[-(bufnum) - 1] > 0) \
		: \
			(PrivateRefCount[(bufnum) - 1] > 0) \
)

/*
 * BufferGetBlock
 *		Returns a reference to a disk page image associated with a buffer.
 *
 * Note:
 *		Assumes buffer is valid.
 */
#define BufferGetBlock(buffer) \
( \
	AssertMacro(BufferIsValid(buffer)), \
	BufferIsLocal(buffer) ? \
		LocalBufferBlockPointers[-(buffer) - 1] \
	: \
		(Block) (BufferBlocks + ((Size) ((buffer) - 1)) * BLCKSZ) \
)

/*
 * BufferGetPageSize
 *		Returns the page size within a buffer.
 *
 * Notes:
 *		Assumes buffer is valid.
 *
 *		The buffer can be a raw disk block and need not contain a valid
 *		(formatted) disk page.
 */
/* XXX should dig out of buffer descriptor */
#define BufferGetPageSize(buffer) \
( \
	AssertMacro(BufferIsValid(buffer)), \
	(Size)BLCKSZ \
)

/*
 * BufferGetPage
 *		Returns the page associated with a buffer.
 */
#define BufferGetPage(buffer) ((Page)BufferGetBlock(buffer))

/*
 * prototypes for functions in bufmgr.c
 */
extern void PrefetchBuffer(Relation reln, ForkNumber forkNum,
			   BlockNumber blockNum);
extern Buffer ReadBuffer(Relation reln, BlockNumber blockNum);
extern Buffer ReadBufferExtended(Relation reln, ForkNumber forkNum,
				   BlockNumber blockNum, ReadBufferMode mode,
				   BufferAccessStrategy strategy);
extern Buffer ReadBufferWithoutRelcache(RelFileNode rnode,
						  ForkNumber forkNum, BlockNumber blockNum,
						  ReadBufferMode mode, BufferAccessStrategy strategy);
extern void ReleaseBuffer(Buffer buffer);
extern void UnlockReleaseBuffer(Buffer buffer);
extern void MarkBufferDirty(Buffer buffer);
extern void IncrBufferRefCount(Buffer buffer);
extern Buffer ReleaseAndReadBuffer(Buffer buffer, Relation relation,
					 BlockNumber blockNum);

extern void InitBufferPool(void);
extern void InitBufferPoolAccess(void);
extern void InitBufferPoolBackend(void);
extern void AtEOXact_Buffers(bool isCommit);
extern void PrintBufferLeakWarning(Buffer buffer);
extern void CheckPointBuffers(int flags);
extern BlockNumber BufferGetBlockNumber(Buffer buffer);
extern BlockNumber RelationGetNumberOfBlocksInFork(Relation relation,
								ForkNumber forkNum);
extern void FlushRelationBuffers(Relation rel);
extern void FlushDatabaseBuffers(Oid dbid);
extern void DropRelFileNodeBuffers(RelFileNodeBackend rnode,
					   ForkNumber forkNum, BlockNumber firstDelBlock);
extern void DropDatabaseBuffers(Oid dbid);

#define RelationGetNumberOfBlocks(reln) \
	RelationGetNumberOfBlocksInFork(reln, MAIN_FORKNUM)

#ifdef NOT_USED
extern void PrintPinnedBufs(void);
#endif
extern Size BufferShmemSize(void);
extern void BufferGetTag(Buffer buffer, RelFileNode *rnode,
			 ForkNumber *forknum, BlockNumber *blknum);

extern void SetBufferCommitInfoNeedsSave(Buffer buffer);

extern void UnlockBuffers(void);
extern void LockBuffer(Buffer buffer, int mode);
extern bool ConditionalLockBuffer(Buffer buffer);
extern void LockBufferForCleanup(Buffer buffer);
extern bool ConditionalLockBufferForCleanup(Buffer buffer);
extern bool HoldingBufferPinThatDelaysRecovery(void);

extern void AbortBufferIO(void);

extern void BufmgrCommit(void);
extern void BgBufferSync(void);

extern void AtProcExit_LocalBuffers(void);

/* in freelist.c */
extern BufferAccessStrategy GetAccessStrategy(BufferAccessStrategyType btype);
extern void FreeAccessStrategy(BufferAccessStrategy strategy);

#endif
