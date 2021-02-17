/*-------------------------------------------------------------------------
 *
 * gistget.c
 *	  fetch tuples from a GiST scan.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/access/gist/gistget.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/gist_private.h"
#include "access/relscan.h"
#include "executor/execdebug.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "storage/bufmgr.h"
#include "utils/builtins.h"
#include "utils/memutils.h"


/*
 * gistindex_keytest() -- does this index tuple satisfy the scan key(s)?
 *
 * The index tuple might represent either a heap tuple or a lower index page,
 * depending on whether the containing page is a leaf page or not.
 *
 * On success return for a heap tuple, *recheck_p is set to indicate
 * whether recheck is needed.  We recheck if any of the consistent() functions
 * request it.	recheck is not interesting when examining a non-leaf entry,
 * since we must visit the lower index page if there's any doubt.
 *
 * If we are doing an ordered scan, so->distances[] is filled with distance
 * data from the distance() functions before returning success.
 *
 * We must decompress the key in the IndexTuple before passing it to the
 * sk_funcs (which actually are the opclass Consistent or Distance methods).
 *
 * Note that this function is always invoked in a short-lived memory context,
 * so we don't need to worry about cleaning up allocated memory, either here
 * or in the implementation of any Consistent or Distance methods.
 */
static bool
gistindex_keytest(IndexScanDesc scan,
				  IndexTuple tuple,
				  Page page,
				  OffsetNumber offset,
				  bool *recheck_p)
{
	GISTScanOpaque so = (GISTScanOpaque) scan->opaque;
	GISTSTATE  *giststate = so->giststate;
	ScanKey		key = scan->keyData;
	int			keySize = scan->numberOfKeys;
	double	   *distance_p;
	Relation	r = scan->indexRelation;

	*recheck_p = false;

	/*
	 * If it's a leftover invalid tuple from pre-9.1, treat it as a match with
	 * minimum possible distances.	This means we'll always follow it to the
	 * referenced page.
	 */
	if (GistTupleIsInvalid(tuple))
	{
		int			i;

		if (GistPageIsLeaf(page))		/* shouldn't happen */
			elog(ERROR, "invalid GiST tuple found on leaf page");
		for (i = 0; i < scan->numberOfOrderBys; i++)
			so->distances[i] = -get_float8_infinity();
		return true;
	}

	/* Check whether it matches according to the Consistent functions */
	while (keySize > 0)
	{
		Datum		datum;
		bool		isNull;

		datum = index_getattr(tuple,
							  key->sk_attno,
							  giststate->tupdesc,
							  &isNull);

		if (key->sk_flags & SK_ISNULL)
		{
			/*
			 * On non-leaf page we can't conclude that child hasn't NULL
			 * values because of assumption in GiST: union (VAL, NULL) is VAL.
			 * But if on non-leaf page key IS NULL, then all children are
			 * NULL.
			 */
			if (key->sk_flags & SK_SEARCHNULL)
			{
				if (GistPageIsLeaf(page) && !isNull)
					return false;
			}
			else
			{
				Assert(key->sk_flags & SK_SEARCHNOTNULL);
				if (isNull)
					return false;
			}
		}
		else if (isNull)
		{
			return false;
		}
		else
		{
			Datum		test;
			bool		recheck;
			GISTENTRY	de;

			gistdentryinit(giststate, key->sk_attno - 1, &de,
						   datum, r, page, offset,
						   FALSE, isNull);

			/*
			 * Call the Consistent function to evaluate the test.  The
			 * arguments are the index datum (as a GISTENTRY*), the comparison
			 * datum, the comparison operator's strategy number and subtype
			 * from pg_amop, and the recheck flag.
			 *
			 * (Presently there's no need to pass the subtype since it'll
			 * always be zero, but might as well pass it for possible future
			 * use.)
			 *
			 * We initialize the recheck flag to true (the safest assumption)
			 * in case the Consistent function forgets to set it.
			 */
			recheck = true;

			test = FunctionCall5Coll(&key->sk_func,
									 key->sk_collation,
									 PointerGetDatum(&de),
									 key->sk_argument,
									 Int32GetDatum(key->sk_strategy),
									 ObjectIdGetDatum(key->sk_subtype),
									 PointerGetDatum(&recheck));

			if (!DatumGetBool(test))
				return false;
			*recheck_p |= recheck;
		}

		key++;
		keySize--;
	}

	/* OK, it passes --- now let's compute the distances */
	key = scan->orderByData;
	distance_p = so->distances;
	keySize = scan->numberOfOrderBys;
	while (keySize > 0)
	{
		Datum		datum;
		bool		isNull;

		datum = index_getattr(tuple,
							  key->sk_attno,
							  giststate->tupdesc,
							  &isNull);

		if ((key->sk_flags & SK_ISNULL) || isNull)
		{
			/* Assume distance computes as null and sorts to the end */
			*distance_p = get_float8_infinity();
		}
		else
		{
			Datum		dist;
			GISTENTRY	de;

			gistdentryinit(giststate, key->sk_attno - 1, &de,
						   datum, r, page, offset,
						   FALSE, isNull);

			/*
			 * Call the Distance function to evaluate the distance.  The
			 * arguments are the index datum (as a GISTENTRY*), the comparison
			 * datum, and the ordering operator's strategy number and subtype
			 * from pg_amop.
			 *
			 * (Presently there's no need to pass the subtype since it'll
			 * always be zero, but might as well pass it for possible future
			 * use.)
			 *
			 * Note that Distance functions don't get a recheck argument. We
			 * can't tolerate lossy distance calculations on leaf tuples;
			 * there is no opportunity to re-sort the tuples afterwards.
			 */
			dist = FunctionCall4Coll(&key->sk_func,
									 key->sk_collation,
									 PointerGetDatum(&de),
									 key->sk_argument,
									 Int32GetDatum(key->sk_strategy),
									 ObjectIdGetDatum(key->sk_subtype));

			*distance_p = DatumGetFloat8(dist);
		}

		key++;
		distance_p++;
		keySize--;
	}

	return true;
}

/*
 * Scan all items on the GiST index page identified by *pageItem, and insert
 * them into the queue (or directly to output areas)
 *
 * scan: index scan we are executing
 * pageItem: search queue item identifying an index page to scan
 * myDistances: distances array associated with pageItem, or NULL at the root
 * tbm: if not NULL, gistgetbitmap's output bitmap
 * ntids: if not NULL, gistgetbitmap's output tuple counter
 *
 * If tbm/ntids aren't NULL, we are doing an amgetbitmap scan, and heap
 * tuples should be reported directly into the bitmap.	If they are NULL,
 * we're doing a plain or ordered indexscan.  For a plain indexscan, heap
 * tuple TIDs are returned into so->pageData[].  For an ordered indexscan,
 * heap tuple TIDs are pushed into individual search queue items.
 *
 * If we detect that the index page has split since we saw its downlink
 * in the parent, we push its new right sibling onto the queue so the
 * sibling will be processed next.
 */
static void
gistScanPage(IndexScanDesc scan, GISTSearchItem *pageItem, double *myDistances,
			 TIDBitmap *tbm, int64 *ntids)
{
	GISTScanOpaque so = (GISTScanOpaque) scan->opaque;
	Buffer		buffer;
	Page		page;
	GISTPageOpaque opaque;
	OffsetNumber maxoff;
	OffsetNumber i;
	GISTSearchTreeItem *tmpItem = so->tmpTreeItem;
	bool		isNew;
	MemoryContext oldcxt;

	Assert(!GISTSearchItemIsHeap(*pageItem));

	buffer = ReadBuffer(scan->indexRelation, pageItem->blkno);
	LockBuffer(buffer, GIST_SHARE);
	gistcheckpage(scan->indexRelation, buffer);
	page = BufferGetPage(buffer);
	opaque = GistPageGetOpaque(page);

	/*
	 * Check if we need to follow the rightlink. We need to follow it if the
	 * page was concurrently split since we visited the parent (in which case
	 * parentlsn < nsn), or if the the system crashed after a page split but
	 * before the downlink was inserted into the parent.
	 */
	if (!XLogRecPtrIsInvalid(pageItem->data.parentlsn) &&
		(GistFollowRight(page) ||
		 XLByteLT(pageItem->data.parentlsn, opaque->nsn)) &&
		opaque->rightlink != InvalidBlockNumber /* sanity check */ )
	{
		/* There was a page split, follow right link to add pages */
		GISTSearchItem *item;

		/* This can't happen when starting at the root */
		Assert(myDistances != NULL);

		oldcxt = MemoryContextSwitchTo(so->queueCxt);

		/* Create new GISTSearchItem for the right sibling index page */
		item = palloc(sizeof(GISTSearchItem));
		item->next = NULL;
		item->blkno = opaque->rightlink;
		item->data.parentlsn = pageItem->data.parentlsn;

		/* Insert it into the queue using same distances as for this page */
		tmpItem->head = item;
		tmpItem->lastHeap = NULL;
		memcpy(tmpItem->distances, myDistances,
			   sizeof(double) * scan->numberOfOrderBys);

		(void) rb_insert(so->queue, (RBNode *) tmpItem, &isNew);

		MemoryContextSwitchTo(oldcxt);
	}

	so->nPageData = so->curPageData = 0;

	/*
	 * check all tuples on page
	 */
	maxoff = PageGetMaxOffsetNumber(page);
	for (i = FirstOffsetNumber; i <= maxoff; i = OffsetNumberNext(i))
	{
		IndexTuple	it = (IndexTuple) PageGetItem(page, PageGetItemId(page, i));
		bool		match;
		bool		recheck;

		/*
		 * Must call gistindex_keytest in tempCxt, and clean up any leftover
		 * junk afterward.
		 */
		oldcxt = MemoryContextSwitchTo(so->tempCxt);

		match = gistindex_keytest(scan, it, page, i, &recheck);

		MemoryContextSwitchTo(oldcxt);
		MemoryContextReset(so->tempCxt);

		/* Ignore tuple if it doesn't match */
		if (!match)
			continue;

		if (tbm && GistPageIsLeaf(page))
		{
			/*
			 * getbitmap scan, so just push heap tuple TIDs into the bitmap
			 * without worrying about ordering
			 */
			tbm_add_tuples(tbm, &it->t_tid, 1, recheck);
			(*ntids)++;
		}
		else if (scan->numberOfOrderBys == 0 && GistPageIsLeaf(page))
		{
			/*
			 * Non-ordered scan, so report heap tuples in so->pageData[]
			 */
			so->pageData[so->nPageData].heapPtr = it->t_tid;
			so->pageData[so->nPageData].recheck = recheck;
			so->nPageData++;
		}
		else
		{
			/*
			 * Must push item into search queue.  We get here for any lower
			 * index page, and also for heap tuples if doing an ordered
			 * search.
			 */
			GISTSearchItem *item;

			oldcxt = MemoryContextSwitchTo(so->queueCxt);

			/* Create new GISTSearchItem for this item */
			item = palloc(sizeof(GISTSearchItem));
			item->next = NULL;

			if (GistPageIsLeaf(page))
			{
				/* Creating heap-tuple GISTSearchItem */
				item->blkno = InvalidBlockNumber;
				item->data.heap.heapPtr = it->t_tid;
				item->data.heap.recheck = recheck;
			}
			else
			{
				/* Creating index-page GISTSearchItem */
				item->blkno = ItemPointerGetBlockNumber(&it->t_tid);
				/* lsn of current page is lsn of parent page for child */
				item->data.parentlsn = PageGetLSN(page);
			}

			/* Insert it into the queue using new distance data */
			tmpItem->head = item;
			tmpItem->lastHeap = GISTSearchItemIsHeap(*item) ? item : NULL;
			memcpy(tmpItem->distances, so->distances,
				   sizeof(double) * scan->numberOfOrderBys);

			(void) rb_insert(so->queue, (RBNode *) tmpItem, &isNew);

			MemoryContextSwitchTo(oldcxt);
		}
	}

	UnlockReleaseBuffer(buffer);
}

/*
 * Extract next item (in order) from search queue
 *
 * Returns a GISTSearchItem or NULL.  Caller must pfree item when done with it.
 *
 * NOTE: on successful return, so->curTreeItem is the GISTSearchTreeItem that
 * contained the result item.  Callers can use so->curTreeItem->distances as
 * the distances value for the item.
 */
static GISTSearchItem *
getNextGISTSearchItem(GISTScanOpaque so)
{
	for (;;)
	{
		GISTSearchItem *item;

		/* Update curTreeItem if we don't have one */
		if (so->curTreeItem == NULL)
		{
			so->curTreeItem = (GISTSearchTreeItem *) rb_leftmost(so->queue);
			/* Done when tree is empty */
			if (so->curTreeItem == NULL)
				break;
		}

		item = so->curTreeItem->head;
		if (item != NULL)
		{
			/* Delink item from chain */
			so->curTreeItem->head = item->next;
			if (item == so->curTreeItem->lastHeap)
				so->curTreeItem->lastHeap = NULL;
			/* Return item; caller is responsible to pfree it */
			return item;
		}

		/* curTreeItem is exhausted, so remove it from rbtree */
		rb_delete(so->queue, (RBNode *) so->curTreeItem);
		so->curTreeItem = NULL;
	}

	return NULL;
}

/*
 * Fetch next heap tuple in an ordered search
 */
static bool
getNextNearest(IndexScanDesc scan)
{
	GISTScanOpaque so = (GISTScanOpaque) scan->opaque;
	bool		res = false;

	do
	{
		GISTSearchItem *item = getNextGISTSearchItem(so);

		if (!item)
			break;

		if (GISTSearchItemIsHeap(*item))
		{
			/* found a heap item at currently minimal distance */
			scan->xs_ctup.t_self = item->data.heap.heapPtr;
			scan->xs_recheck = item->data.heap.recheck;
			res = true;
		}
		else
		{
			/* visit an index page, extract its items into queue */
			CHECK_FOR_INTERRUPTS();

			gistScanPage(scan, item, so->curTreeItem->distances, NULL, NULL);
		}

		pfree(item);
	} while (!res);

	return res;
}

/*
 * gistgettuple() -- Get the next tuple in the scan
 */
Datum
gistgettuple(PG_FUNCTION_ARGS)
{
	IndexScanDesc scan = (IndexScanDesc) PG_GETARG_POINTER(0);
	ScanDirection dir = (ScanDirection) PG_GETARG_INT32(1);
	GISTScanOpaque so = (GISTScanOpaque) scan->opaque;

	if (dir != ForwardScanDirection)
		elog(ERROR, "GiST only supports forward scan direction");

	if (!so->qual_ok)
		PG_RETURN_BOOL(false);

	if (so->firstCall)
	{
		/* Begin the scan by processing the root page */
		GISTSearchItem fakeItem;

		pgstat_count_index_scan(scan->indexRelation);

		so->firstCall = false;
		so->curTreeItem = NULL;
		so->curPageData = so->nPageData = 0;

		fakeItem.blkno = GIST_ROOT_BLKNO;
		memset(&fakeItem.data.parentlsn, 0, sizeof(GistNSN));
		gistScanPage(scan, &fakeItem, NULL, NULL, NULL);
	}

	if (scan->numberOfOrderBys > 0)
	{
		/* Must fetch tuples in strict distance order */
		PG_RETURN_BOOL(getNextNearest(scan));
	}
	else
	{
		/* Fetch tuples index-page-at-a-time */
		for (;;)
		{
			if (so->curPageData < so->nPageData)
			{
				/* continuing to return tuples from a leaf page */
				scan->xs_ctup.t_self = so->pageData[so->curPageData].heapPtr;
				scan->xs_recheck = so->pageData[so->curPageData].recheck;
				so->curPageData++;
				PG_RETURN_BOOL(true);
			}

			/* find and process the next index page */
			do
			{
				GISTSearchItem *item = getNextGISTSearchItem(so);

				if (!item)
					PG_RETURN_BOOL(false);

				CHECK_FOR_INTERRUPTS();

				/*
				 * While scanning a leaf page, ItemPointers of matching heap
				 * tuples are stored in so->pageData.  If there are any on
				 * this page, we fall out of the inner "do" and loop around to
				 * return them.
				 */
				gistScanPage(scan, item, so->curTreeItem->distances, NULL, NULL);

				pfree(item);
			} while (so->nPageData == 0);
		}
	}

	PG_RETURN_BOOL(false);		/* keep compiler quiet */
}

/*
 * gistgetbitmap() -- Get a bitmap of all heap tuple locations
 */
Datum
gistgetbitmap(PG_FUNCTION_ARGS)
{
	IndexScanDesc scan = (IndexScanDesc) PG_GETARG_POINTER(0);
	TIDBitmap  *tbm = (TIDBitmap *) PG_GETARG_POINTER(1);
	GISTScanOpaque so = (GISTScanOpaque) scan->opaque;
	int64		ntids = 0;
	GISTSearchItem fakeItem;

	if (!so->qual_ok)
		PG_RETURN_INT64(0);

	pgstat_count_index_scan(scan->indexRelation);

	/* Begin the scan by processing the root page */
	so->curTreeItem = NULL;
	so->curPageData = so->nPageData = 0;

	fakeItem.blkno = GIST_ROOT_BLKNO;
	memset(&fakeItem.data.parentlsn, 0, sizeof(GistNSN));
	gistScanPage(scan, &fakeItem, NULL, tbm, &ntids);

	/*
	 * While scanning a leaf page, ItemPointers of matching heap tuples will
	 * be stored directly into tbm, so we don't need to deal with them here.
	 */
	for (;;)
	{
		GISTSearchItem *item = getNextGISTSearchItem(so);

		if (!item)
			break;

		CHECK_FOR_INTERRUPTS();

		gistScanPage(scan, item, so->curTreeItem->distances, tbm, &ntids);

		pfree(item);
	}

	PG_RETURN_INT64(ntids);
}
