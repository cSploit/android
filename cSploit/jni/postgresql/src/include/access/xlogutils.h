/*
 * xlogutils.h
 *
 * PostgreSQL transaction log manager utility routines
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/xlogutils.h
 */
#ifndef XLOG_UTILS_H
#define XLOG_UTILS_H

#include "storage/buf.h"
#include "storage/bufmgr.h"
#include "storage/relfilenode.h"
#include "storage/block.h"
#include "utils/relcache.h"


extern void XLogCheckInvalidPages(void);

extern void XLogDropRelation(RelFileNode rnode, ForkNumber forknum);
extern void XLogDropDatabase(Oid dbid);
extern void XLogTruncateRelation(RelFileNode rnode, ForkNumber forkNum,
					 BlockNumber nblocks);

extern Buffer XLogReadBuffer(RelFileNode rnode, BlockNumber blkno, bool init);
extern Buffer XLogReadBufferExtended(RelFileNode rnode, ForkNumber forknum,
					   BlockNumber blkno, ReadBufferMode mode);

extern Relation CreateFakeRelcacheEntry(RelFileNode rnode);
extern void FreeFakeRelcacheEntry(Relation fakerel);

#endif
