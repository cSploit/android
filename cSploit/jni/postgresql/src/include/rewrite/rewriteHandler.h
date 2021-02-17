/*-------------------------------------------------------------------------
 *
 * rewriteHandler.h
 *		External interface to query rewriter.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/rewrite/rewriteHandler.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef REWRITEHANDLER_H
#define REWRITEHANDLER_H

#include "utils/relcache.h"
#include "nodes/parsenodes.h"

extern List *QueryRewrite(Query *parsetree);
extern void AcquireRewriteLocks(Query *parsetree, bool forUpdatePushedDown);
extern Node *build_column_default(Relation rel, int attrno);

#endif   /* REWRITEHANDLER_H */
