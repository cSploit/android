/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/fop.h"
#include "dbinc/hash.h"
#include "dbinc/heap.h"
#include "dbinc/mp.h"
#include "dbinc/partition.h"
#include "dbinc/qam.h"
#include "dbinc/db_verify.h"

static int	 __db_bmeta __P((ENV *, DB *, BTMETA *, u_int32_t));
static int	 __db_heapmeta __P((ENV *, DB *, HEAPMETA *, u_int32_t));
static int	 __db_heapint __P((DB *, HEAPPG *, u_int32_t));
static int	 __db_hmeta __P((ENV *, DB *, HMETA *, u_int32_t));
static void	 __db_meta __P((ENV *, DB *, DBMETA *, FN const *, u_int32_t));
static void	 __db_proff __P((ENV *, DB_MSGBUF *, void *));
static int	 __db_qmeta __P((ENV *, DB *, QMETA *, u_int32_t));
static int	 __db_prblob __P((DBC *, DBT *, DBT *, int, const char *,
    void *, int (*callback) __P((void *, const void *)), int, int));
static int	 __db_prblob_id __P((DB *, db_seq_t,
		    off_t, DBT *, int, const char *, void *,
		    int (*callback) __P((void *, const void *))));
#ifdef HAVE_STATISTICS
static void	 __db_prdb __P((DB *, u_int32_t));
static int	 __db_prtree __P((DB *, DB_TXN *,
		    u_int32_t, db_pgno_t, db_pgno_t));
#endif

/*
 * __db_loadme --
 *	A nice place to put a breakpoint.
 *
 * PUBLIC: void __db_loadme __P((void));
 */
void
__db_loadme()
{
	pid_t pid;

	__os_id(NULL, &pid, NULL);
}

#ifdef HAVE_STATISTICS
/*
 * __db_dumptree --
 *	Dump the tree to a file.
 *
 * PUBLIC: int __db_dumptree __P((DB *, DB_TXN *,
 * PUBLIC:     char *, char *, db_pgno_t, db_pgno_t));
 */
int
__db_dumptree(dbp, txn, op, name, first, last)
	DB *dbp;
	DB_TXN *txn;
	char *op, *name;
	db_pgno_t first, last;
{
	ENV *env;
	FILE *fp, *orig_fp;
	u_int32_t flags;
	int ret;

	env = dbp->env;

	for (flags = 0; *op != '\0'; ++op)
		switch (*op) {
		case 'a':
			LF_SET(DB_PR_PAGE);
			break;
		case 'h':
			break;
		case 'r':
			LF_SET(DB_PR_RECOVERYTEST);
			break;
		default:
			return (EINVAL);
		}

	if (name != NULL) {
		if ((fp = fopen(name, "w")) == NULL)
			return (__os_get_errno());

		orig_fp = dbp->dbenv->db_msgfile;
		dbp->dbenv->db_msgfile = fp;
	} else
		fp = orig_fp = NULL;

	__db_prdb(dbp, flags);

	__db_msg(env, "%s", DB_GLOBAL(db_line));

	ret = __db_prtree(dbp, txn, flags, first, last);

	if (fp != NULL) {
		(void)fclose(fp);
		env->dbenv->db_msgfile = orig_fp;
	}

	return (ret);
}

static const FN __db_flags_fn[] = {
	{ DB_AM_CHKSUM,			"checksumming" },
	{ DB_AM_COMPENSATE,		"created by compensating transaction" },
	{ DB_AM_CREATED,		"database created" },
	{ DB_AM_CREATED_MSTR,		"encompassing file created" },
	{ DB_AM_DBM_ERROR,		"dbm/ndbm error" },
	{ DB_AM_DELIMITER,		"variable length" },
	{ DB_AM_DISCARD,		"discard cached pages" },
	{ DB_AM_DUP,			"duplicates" },
	{ DB_AM_DUPSORT,		"sorted duplicates" },
	{ DB_AM_ENCRYPT,		"encrypted" },
	{ DB_AM_FIXEDLEN,		"fixed-length records" },
	{ DB_AM_INMEM,			"in-memory" },
	{ DB_AM_IN_RENAME,		"file is being renamed" },
	{ DB_AM_NOT_DURABLE,		"changes not logged" },
	{ DB_AM_OPEN_CALLED,		"open called" },
	{ DB_AM_PAD,			"pad value" },
	{ DB_AM_PGDEF,			"default page size" },
	{ DB_AM_RDONLY,			"read-only" },
	{ DB_AM_READ_UNCOMMITTED,	"read-uncommitted" },
	{ DB_AM_RECNUM,			"Btree record numbers" },
	{ DB_AM_RECOVER,		"opened for recovery" },
	{ DB_AM_RENUMBER,		"renumber" },
	{ DB_AM_REVSPLITOFF,		"no reverse splits" },
	{ DB_AM_SECONDARY,		"secondary" },
	{ DB_AM_SNAPSHOT,		"load on open" },
	{ DB_AM_SUBDB,			"subdatabases" },
	{ DB_AM_SWAP,			"needswap" },
	{ DB_AM_TXN,			"transactional" },
	{ DB_AM_VERIFYING,		"verifier" },
	{ 0,				NULL }
};

/*
 * __db_get_flags_fn --
 *	Return the __db_flags_fn array.
 *
 * PUBLIC: const FN * __db_get_flags_fn __P((void));
 */
const FN *
__db_get_flags_fn()
{
	return (__db_flags_fn);
}

/*
 * __db_prdb --
 *	Print out the DB structure information.
 */
static void
__db_prdb(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	BTREE *bt;
	DB_MSGBUF mb;
	ENV *env;
	HASH *h;
	QUEUE *q;
	HEAP *hp;

	env = dbp->env;

	DB_MSGBUF_INIT(&mb);
	__db_msg(env, "In-memory DB structure:");
	__db_msgadd(env, &mb, "%s: %#lx",
	    __db_dbtype_to_string(dbp->type), (u_long)dbp->flags);
	__db_prflags(env, &mb, dbp->flags, __db_flags_fn, " (", ")");
	DB_MSGBUF_FLUSH(env, &mb);

	switch (dbp->type) {
	case DB_BTREE:
	case DB_RECNO:
		bt = dbp->bt_internal;
		__db_msg(env, "bt_meta: %lu bt_root: %lu",
		    (u_long)bt->bt_meta, (u_long)bt->bt_root);
		__db_msg(env, "bt_minkey: %lu", (u_long)bt->bt_minkey);
		if (!LF_ISSET(DB_PR_RECOVERYTEST))
			__db_msg(env, "bt_compare: %#lx bt_prefix: %#lx",
			    P_TO_ULONG(bt->bt_compare),
			    P_TO_ULONG(bt->bt_prefix));
#ifdef HAVE_COMPRESSION
		if (!LF_ISSET(DB_PR_RECOVERYTEST))
			__db_msg(env, "bt_compress: %#lx bt_decompress: %#lx",
			    P_TO_ULONG(bt->bt_compress),
			    P_TO_ULONG(bt->bt_decompress));
#endif
		__db_msg(env, "bt_lpgno: %lu", (u_long)bt->bt_lpgno);
		if (dbp->type == DB_RECNO) {
			__db_msg(env,
		    "re_pad: %#lx re_delim: %#lx re_len: %lu re_source: %s",
			    (u_long)bt->re_pad, (u_long)bt->re_delim,
			    (u_long)bt->re_len,
			    bt->re_source == NULL ? "" : bt->re_source);
			__db_msg(env,
			    "re_modified: %d re_eof: %d re_last: %lu",
			    bt->re_modified, bt->re_eof, (u_long)bt->re_last);
		}
		break;
	case DB_HASH:
		h = dbp->h_internal;
		__db_msg(env, "meta_pgno: %lu", (u_long)h->meta_pgno);
		__db_msg(env, "h_ffactor: %lu", (u_long)h->h_ffactor);
		__db_msg(env, "h_nelem: %lu", (u_long)h->h_nelem);
		if (!LF_ISSET(DB_PR_RECOVERYTEST))
			__db_msg(env, "h_hash: %#lx", P_TO_ULONG(h->h_hash));
		break;
	case DB_QUEUE:
		q = dbp->q_internal;
		__db_msg(env, "q_meta: %lu", (u_long)q->q_meta);
		__db_msg(env, "q_root: %lu", (u_long)q->q_root);
		__db_msg(env, "re_pad: %#lx re_len: %lu",
		    (u_long)q->re_pad, (u_long)q->re_len);
		__db_msg(env, "rec_page: %lu", (u_long)q->rec_page);
		__db_msg(env, "page_ext: %lu", (u_long)q->page_ext);
		break;
	case DB_HEAP:
		hp = dbp->heap_internal;
		__db_msg(env, "gbytes: %lu", (u_long)hp->gbytes);
		__db_msg(env, "bytes: %lu", (u_long)hp->bytes);
		__db_msg(env, "curregion: %lu", (u_long)hp->curregion);
		__db_msg(env, "region_size: %lu", (u_long)hp->region_size);
		__db_msg(env, "maxpgno: %lu", (u_long)hp->maxpgno);
		break;
	case DB_UNKNOWN:
	default:
		break;
	}
}

/*
 * __db_prtree --
 *	Print out the entire tree.
 */
static int
__db_prtree(dbp, txn, flags, first, last)
	DB *dbp;
	DB_TXN *txn;
	u_int32_t flags;
	db_pgno_t first, last;
{
	DB_MPOOLFILE *mpf;
	PAGE *h;
	db_pgno_t i;
	int ret;

	mpf = dbp->mpf;

	if (dbp->type == DB_QUEUE)
		return (__db_prqueue(dbp, flags));

	/*
	 * Find out the page number of the last page in the database, then
	 * dump each page.
	 */
	if (last == PGNO_INVALID &&
	    (ret = __memp_get_last_pgno(mpf, &last)) != 0)
		return (ret);
	for (i = first; i <= last; ++i) {
		if ((ret = __memp_fget(mpf, &i, NULL, txn, 0, &h)) != 0)
			return (ret);
		(void)__db_prpage(dbp, h, flags);
		if ((ret = __memp_fput(mpf, NULL, h, dbp->priority)) != 0)
			return (ret);
	}

	return (0);
}

/*
 * __db_prnpage
 *	-- Print out a specific page.
 *
 * PUBLIC: int __db_prnpage __P((DB *, DB_TXN *, db_pgno_t));
 */
int
__db_prnpage(dbp, txn, pgno)
	DB *dbp;
	DB_TXN *txn;
	db_pgno_t pgno;
{
	DB_MPOOLFILE *mpf;
	PAGE *h;
	int ret, t_ret;

	mpf = dbp->mpf;

	if ((ret = __memp_fget(mpf, &pgno, NULL, txn, 0, &h)) != 0)
		return (ret);

	ret = __db_prpage(dbp, h, DB_PR_PAGE);

	if ((t_ret = __memp_fput(mpf, NULL, h, dbp->priority)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __db_prpage
 *	-- Print out a page.
 *
 * PUBLIC: int __db_prpage __P((DB *, PAGE *, u_int32_t));
 */
int
__db_prpage(dbp, h, flags)
	DB *dbp;
	PAGE *h;
	u_int32_t flags;
{
	DB_MSGBUF mb;
	u_int32_t pagesize;
	/*
	 * !!!
	 * Find out the page size.  We don't want to do it the "right" way,
	 * by reading the value from the meta-data page, that's going to be
	 * slow.  Reach down into the mpool region.
	 */
	pagesize = (u_int32_t)dbp->mpf->mfp->pagesize;
	DB_MSGBUF_INIT(&mb);
	return (__db_prpage_int(dbp->env,
	    &mb, dbp, "", h, pagesize, NULL, flags));
}

/*
 * __db_lockmode_to_string --
 *	Return the name of the lock mode.
 *
 * PUBLIC: const char * __db_lockmode_to_string __P((db_lockmode_t));
 */
const char *
__db_lockmode_to_string(mode)
	db_lockmode_t mode;
{
	switch (mode) {
	case DB_LOCK_NG:
		return ("Not granted");
	case DB_LOCK_READ:
		return ("Shared/read");
	case DB_LOCK_WRITE:
		return ("Exclusive/write");
	case DB_LOCK_WAIT:
		return ("Wait for event");
	case DB_LOCK_IWRITE:
		return ("Intent exclusive/write");
	case DB_LOCK_IREAD:
		return ("Intent shared/read");
	case DB_LOCK_IWR:
		return ("Intent to read/write");
	case DB_LOCK_READ_UNCOMMITTED:
		return ("Read uncommitted");
	case DB_LOCK_WWRITE:
		return ("Was written");
	default:
		break;
	}
	return ("UNKNOWN LOCK MODE");
}

#else /* !HAVE_STATISTICS */

/*
 * __db_dumptree --
 *	Dump the tree to a file.
 *
 * PUBLIC: int __db_dumptree __P((DB *, DB_TXN *,
 * PUBLIC:     char *, char *, db_pgno_t, db_pgno_t));
 */
int
__db_dumptree(dbp, txn, op, name, first, last)
	DB *dbp;
	DB_TXN *txn;
	char *op, *name;
	db_pgno_t first, last;
{
	COMPQUIET(txn, NULL);
	COMPQUIET(op, NULL);
	COMPQUIET(name, NULL);
	COMPQUIET(first, last);

	return (__db_stat_not_built(dbp->env));
}

/*
 * __db_get_flags_fn --
 *	Return the __db_flags_fn array.
 *
 * PUBLIC: const FN * __db_get_flags_fn __P((void));
 */
const FN *
__db_get_flags_fn()
{
	/*
	 * !!!
	 * The Tcl API uses this interface, stub it off.
	 */
	return (NULL);
}
#endif

/*
 * __db_meta --
 *	Print out common metadata information.
 */
static void
__db_meta(env, dbp, dbmeta, fn, flags)
	DB *dbp;
	ENV *env;
	DBMETA *dbmeta;
	FN const *fn;
	u_int32_t flags;
{
	DB_MPOOLFILE *mpf;
	DB_MSGBUF mb;
	PAGE *h;
	db_pgno_t pgno;
	u_int8_t *p;
	int cnt, ret;
	const char *sep;

	DB_MSGBUF_INIT(&mb);

	__db_msg(env, "\tmagic: %#lx", (u_long)dbmeta->magic);
	__db_msg(env, "\tversion: %lu", (u_long)dbmeta->version);
	__db_msg(env, "\tpagesize: %lu", (u_long)dbmeta->pagesize);
	__db_msg(env, "\ttype: %lu", (u_long)dbmeta->type);
	__db_msg(env, "\tmetaflags %#lx", (u_long)dbmeta->metaflags);
	__db_msg(env, "\tkeys: %lu\trecords: %lu",
	    (u_long)dbmeta->key_count, (u_long)dbmeta->record_count);
	if (dbmeta->nparts)
		__db_msg(env, "\tnparts: %lu", (u_long)dbmeta->nparts);

	/*
	 * If we're doing recovery testing, don't display the free list,
	 * it may have changed and that makes the dump diff not work.
	 */
	if (dbp != NULL && !LF_ISSET(DB_PR_RECOVERYTEST)) {
		mpf = dbp->mpf;
		__db_msgadd(
		    env, &mb, "\tfree list: %lu", (u_long)dbmeta->free);
		for (pgno = dbmeta->free,
		    cnt = 0, sep = ", "; pgno != PGNO_INVALID;) {
			if ((ret = __memp_fget(mpf,
			     &pgno, NULL, NULL, 0, &h)) != 0) {
				DB_MSGBUF_FLUSH(env, &mb);
				__db_msg(env,
			    "Unable to retrieve free-list page: %lu: %s",
				    (u_long)pgno, db_strerror(ret));
				break;
			}
			pgno = h->next_pgno;
			(void)__memp_fput(mpf, NULL, h, dbp->priority);
			__db_msgadd(env, &mb, "%s%lu", sep, (u_long)pgno);
			if (++cnt % 10 == 0) {
				DB_MSGBUF_FLUSH(env, &mb);
				cnt = 0;
				sep = "\t";
			} else
				sep = ", ";
		}
		DB_MSGBUF_FLUSH(env, &mb);
		__db_msg(env, "\tlast_pgno: %lu", (u_long)dbmeta->last_pgno);
	}

	if (fn != NULL) {
		DB_MSGBUF_FLUSH(env, &mb);
		__db_msgadd(env, &mb, "\tflags: %#lx", (u_long)dbmeta->flags);
		__db_prflags(env, &mb, dbmeta->flags, fn, " (", ")");
	}

	DB_MSGBUF_FLUSH(env, &mb);
	__db_msgadd(env, &mb, "\tuid: ");
	for (p = (u_int8_t *)dbmeta->uid,
	    cnt = 0; cnt < DB_FILE_ID_LEN; ++cnt) {
		__db_msgadd(env, &mb, "%x", *p++);
		if (cnt < DB_FILE_ID_LEN - 1)
			__db_msgadd(env, &mb, " ");
	}
	DB_MSGBUF_FLUSH(env, &mb);
}

/*
 * __db_bmeta --
 *	Print out the btree meta-data page.
 */
static int
__db_bmeta(env, dbp, h, flags)
	ENV *env;
	DB *dbp;
	BTMETA *h;
	u_int32_t flags;
{
	static const FN fn[] = {
		{ BTM_DUP,	"duplicates" },
		{ BTM_RECNO,	"recno" },
		{ BTM_RECNUM,	"btree:recnum" },
		{ BTM_FIXEDLEN,	"recno:fixed-length" },
		{ BTM_RENUMBER,	"recno:renumber" },
		{ BTM_SUBDB,	"multiple-databases" },
		{ BTM_DUPSORT,	"sorted duplicates" },
		{ BTM_COMPRESS,	"compressed" },
		{ 0,		NULL }
	};

	__db_meta(env, dbp, (DBMETA *)h, fn, flags);

	__db_msg(env, "\tminkey: %lu", (u_long)h->minkey);
	if (F_ISSET(&h->dbmeta, BTM_RECNO))
		__db_msg(env, "\tre_len: %#lx re_pad: %#lx",
		    (u_long)h->re_len, (u_long)h->re_pad);
	__db_msg(env, "\troot: %lu", (u_long)h->root);
	__db_msg(env, "\tblob_threshold: %lu", (u_long)h->blob_threshold);
	__db_msg(env, "\tblob_file_lo: %lu", (u_long)h->blob_file_lo);
	__db_msg(env, "\tblob_file_hi: %lu", (u_long)h->blob_file_hi);
	__db_msg(env, "\tblob_sdb_lo: %lu", (u_long)h->blob_sdb_lo);
	__db_msg(env, "\tblob_sdb_hi: %lu", (u_long)h->blob_sdb_hi);

	return (0);
}

/*
 * __db_hmeta --
 *	Print out the hash meta-data page.
 */
static int
__db_hmeta(env, dbp, h, flags)
	ENV *env;
	DB *dbp;
	HMETA *h;
	u_int32_t flags;
{
	static const FN fn[] = {
		{ DB_HASH_DUP,		"duplicates" },
		{ DB_HASH_SUBDB,	"multiple-databases" },
		{ DB_HASH_DUPSORT,	"sorted duplicates" },
		{ 0,			NULL }
	};
	DB_MSGBUF mb;
	int i;

	DB_MSGBUF_INIT(&mb);

	__db_meta(env, dbp, (DBMETA *)h, fn, flags);

	__db_msg(env, "\tmax_bucket: %lu", (u_long)h->max_bucket);
	__db_msg(env, "\thigh_mask: %#lx", (u_long)h->high_mask);
	__db_msg(env, "\tlow_mask:  %#lx", (u_long)h->low_mask);
	__db_msg(env, "\tffactor: %lu", (u_long)h->ffactor);
	__db_msg(env, "\tnelem: %lu", (u_long)h->nelem);
	__db_msg(env, "\th_charkey: %#lx", (u_long)h->h_charkey);
	__db_msg(env, "\tblob_threshold: %lu", (u_long)h->blob_threshold);
	__db_msg(env, "\tblob_file_lo: %lu", (u_long)h->blob_file_lo);
	__db_msg(env, "\tblob_file_hi: %lu", (u_long)h->blob_file_hi);
	__db_msg(env, "\tblob_sdb_lo: %lu", (u_long)h->blob_sdb_lo);
	__db_msg(env, "\tblob_sdb_hi: %lu", (u_long)h->blob_sdb_hi);
	__db_msgadd(env, &mb, "\tspare points:\n\t");
	for (i = 0; i < NCACHED; i++) {
		__db_msgadd(env, &mb, "%lu (%lu) ", (u_long)h->spares[i],
		    (u_long)(h->spares[i] == 0 ?
		    0 : h->spares[i] + (i == 0 ? 0 : 1 << (i-1))));
		if ((i + 1) % 8 == 0)
			__db_msgadd(env, &mb, "\n\t");
	}
	DB_MSGBUF_FLUSH(env, &mb);

	return (0);
}

/*
 * __db_qmeta --
 *	Print out the queue meta-data page.
 */
static int
__db_qmeta(env, dbp, h, flags)
	ENV *env;
	DB *dbp;
	QMETA *h;
	u_int32_t flags;
{

	__db_meta(env, dbp, (DBMETA *)h, NULL, flags);

	__db_msg(env, "\tfirst_recno: %lu", (u_long)h->first_recno);
	__db_msg(env, "\tcur_recno: %lu", (u_long)h->cur_recno);
	__db_msg(env, "\tre_len: %#lx re_pad: %lu",
	    (u_long)h->re_len, (u_long)h->re_pad);
	__db_msg(env, "\trec_page: %lu", (u_long)h->rec_page);
	__db_msg(env, "\tpage_ext: %lu", (u_long)h->page_ext);

	return (0);
}

/*
 * __db_heapmeta --
 *	Print out the heap meta-data page.
 */
static int
__db_heapmeta(env, dbp, h, flags)
	ENV *env;
	DB *dbp;
	HEAPMETA *h;
	u_int32_t flags;
{
	__db_meta(env, dbp, (DBMETA *)h, NULL, flags);

	__db_msg(env, "\tcurregion: %lu", (u_long)h->curregion);
	__db_msg(env, "\tregion_size: %lu", (u_long)h->region_size);
	__db_msg(env, "\tnregions: %lu", (u_long)h->nregions);
	__db_msg(env, "\tgbytes: %lu", (u_long)h->gbytes);
	__db_msg(env, "\tbytes: %lu", (u_long)h->bytes);
	__db_msg(env, "\tblob_threshold: %lu", (u_long)h->blob_threshold);
	__db_msg(env, "\tblob_file_lo: %lu", (u_long)h->blob_file_lo);
	__db_msg(env, "\tblob_file_hi: %lu", (u_long)h->blob_file_hi);

	return (0);
}

/*
 * __db_heapint --
 *	Print out the heap internal-data page.
 */
static int
__db_heapint(dbp, h, flags)
	DB *dbp;
	HEAPPG *h;
	u_int32_t flags;
{
	DB_MSGBUF mb;
	ENV *env;
	int count, printed;
	u_int32_t i, max;
	u_int8_t avail;

	env = dbp->env;
	DB_MSGBUF_INIT(&mb);
	count = printed = 0;
	COMPQUIET(flags, 0);

	__db_msgadd(env, &mb, "\thigh: %4lu\n", (u_long)h->high_pgno);
	/* How many entries could there be on a page */
	max = HEAP_REGION_SIZE(dbp);

	for (i = 0; i < max; i++, count++) {
		avail = HEAP_SPACE(dbp, h, i);
		if (avail != 0) {
			__db_msgadd(env, &mb,
			    "%5lu:%1lu ", (u_long)i, (u_long)avail);
			printed = 1;
		}
		/* We get 10 entries per line this way */
		if (count == 9) {
			DB_MSGBUF_FLUSH(env, &mb);
			count = -1;
		}
	}
	/* All pages were less than 33% full */
	if (printed == 0)
		 __db_msgadd(env, &mb,
		    "All pages in this region less than 33 percent full");

	DB_MSGBUF_FLUSH(env, &mb);
	return (0);
}

/*
 * For printing pages from the log we may be passed the data segment
 * separate from the header, if so then it starts at HOFFSET.
 */
#define	PR_ENTRY(dbp, h, i, data)				\
	(data == NULL ? P_ENTRY(dbp, h, i) :			\
	    (u_int8_t *)data + P_INP(dbp, h)[i] - HOFFSET(h))
/*
 * __db_prpage_int
 *	-- Print out a page.
 *
 * PUBLIC: int __db_prpage_int __P((ENV *, DB_MSGBUF *,
 * PUBLIC:      DB *, char *, PAGE *, u_int32_t, u_int8_t *, u_int32_t));
 */
int
__db_prpage_int(env, mbp, dbp, lead, h, pagesize, data, flags)
	ENV *env;
	DB_MSGBUF *mbp;
	DB *dbp;
	char *lead;
	PAGE *h;
	u_int32_t pagesize;
	u_int8_t *data;
	u_int32_t flags;
{
	BINTERNAL *bi;
	BKEYDATA *bk;
	BBLOB bl;
	HOFFPAGE a_hkd;
	HBLOB hblob;
	QAMDATA *qp, *qep;
	RINTERNAL *ri;
	HEAPHDR *hh;
	HEAPSPLITHDR *hs;
	HEAPBLOBHDR bhdr;
	db_indx_t dlen, len, i, *inp, max;
	db_pgno_t pgno;
	db_recno_t recno;
	off_t blob_size;
	db_seq_t blob_id;
	u_int32_t qlen;
	u_int8_t *ep, *hk, *p;
	int deleted, ret;
	const char *s;
	void *hdata, *sp;

	/*
	 * If we're doing recovery testing and this page is P_INVALID,
	 * assume it's a page that's on the free list, and don't display it.
	 */
	if (LF_ISSET(DB_PR_RECOVERYTEST) && TYPE(h) == P_INVALID)
		return (0);

	if ((s = __db_pagetype_to_string(TYPE(h))) == NULL) {
		__db_msg(env, "%sILLEGAL PAGE TYPE: page: %lu type: %lu",
		    lead, (u_long)h->pgno, (u_long)TYPE(h));
		return (EINVAL);
	}

	/* Page number, page type. */
	__db_msgadd(env, mbp, "%spage %lu: %s:", lead, (u_long)h->pgno, s);

	/*
	 * LSNs on a metadata page will be different from the original after an
	 * abort, in some cases.  Don't display them if we're testing recovery.
	 */
	if (!LF_ISSET(DB_PR_RECOVERYTEST) ||
	    (TYPE(h) != P_BTREEMETA && TYPE(h) != P_HASHMETA &&
	    TYPE(h) != P_QAMMETA && TYPE(h) != P_QAMDATA &&
	    TYPE(h) != P_HEAPMETA))
		__db_msgadd(env, mbp, " LSN [%lu][%lu]:",
		    (u_long)LSN(h).file, (u_long)LSN(h).offset);

	/*
	 * Page level (only applicable for Btree/Recno, but we always display
	 * it, for no particular reason, except for Heap.
	 */
	if (!HEAPTYPE(h))
	    __db_msgadd(env, mbp, " level %lu", (u_long)h->level);

	/* Record count. */
	if (TYPE(h) == P_IBTREE || TYPE(h) == P_IRECNO ||
	    (dbp != NULL && TYPE(h) == P_LRECNO &&
	    h->pgno == ((BTREE *)dbp->bt_internal)->bt_root))
		__db_msgadd(env, mbp, " records: %lu", (u_long)RE_NREC(h));
	DB_MSGBUF_FLUSH(env, mbp);

	switch (TYPE(h)) {
	case P_BTREEMETA:
		return (__db_bmeta(env, dbp, (BTMETA *)h, flags));
	case P_HASHMETA:
		return (__db_hmeta(env, dbp, (HMETA *)h, flags));
	case P_QAMMETA:
		return (__db_qmeta(env, dbp, (QMETA *)h, flags));
	case P_QAMDATA:				/* Should be meta->start. */
		if (!LF_ISSET(DB_PR_PAGE) || dbp == NULL)
			return (0);

		qlen = ((QUEUE *)dbp->q_internal)->re_len;
		recno = (h->pgno - 1) * QAM_RECNO_PER_PAGE(dbp) + 1;
		i = 0;
		qep = (QAMDATA *)((u_int8_t *)h + pagesize - qlen);
		for (qp = QAM_GET_RECORD(dbp, h, i); qp < qep;
		    recno++, i++, qp = QAM_GET_RECORD(dbp, h, i)) {
			if (!F_ISSET(qp, QAM_SET))
				continue;

			__db_msgadd(env, mbp, "%s",
			    F_ISSET(qp, QAM_VALID) ? "\t" : "       D");
			__db_msgadd(env, mbp, "[%03lu] %4lu ", (u_long)recno,
			    (u_long)((u_int8_t *)qp - (u_int8_t *)h));
			__db_prbytes(env, mbp, qp->data, qlen);
		}
		return (0);
	case P_HEAPMETA:
		return (__db_heapmeta(env, dbp, (HEAPMETA *)h, flags));
	case P_IHEAP:
		if (!LF_ISSET(DB_PR_PAGE) || dbp == NULL)
			return (0);
		return (__db_heapint(dbp, (HEAPPG *)h, flags));
	default:
		break;
	}

	s = "\t";
	if (!HEAPTYPE(h) && TYPE(h) != P_IBTREE && TYPE(h) != P_IRECNO) {
		__db_msgadd(env, mbp, "%sprev: %4lu next: %4lu",
		    s, (u_long)PREV_PGNO(h), (u_long)NEXT_PGNO(h));
		s = " ";
	}

	if (HEAPTYPE(h)) {
		__db_msgadd(env, mbp, "%shigh indx: %4lu free indx: %4lu", s,
		    (u_long)HEAP_HIGHINDX(h), (u_long)HEAP_FREEINDX(h));
		s = " ";
	}

	if (TYPE(h) == P_OVERFLOW) {
		__db_msgadd(env, mbp,
		    "%sref cnt: %4lu ", s, (u_long)OV_REF(h));
		if (dbp == NULL)
			__db_msgadd(env, mbp,
			    " len: %4lu ", (u_long)OV_LEN(h));
		else
			__db_prbytes(env,
			    mbp, (u_int8_t *)h + P_OVERHEAD(dbp), OV_LEN(h));
		return (0);
	}
	__db_msgadd(env, mbp, "%sentries: %4lu", s, (u_long)NUM_ENT(h));
	__db_msgadd(env, mbp, " offset: %4lu", (u_long)HOFFSET(h));
	DB_MSGBUF_FLUSH(env, mbp);

	if (dbp == NULL || TYPE(h) == P_INVALID || !LF_ISSET(DB_PR_PAGE))
		return (0);

	if (data != NULL)
		pagesize += HOFFSET(h);
	else if (pagesize < HOFFSET(h))
		return (0);

	ret = 0;
	inp = P_INP(dbp, h);
	max = TYPE(h) == P_HEAP ? HEAP_HIGHINDX(h) + 1 : NUM_ENT(h);
	for (i = 0; i < max; i++) {
		if (TYPE(h) == P_HEAP && inp[i] == 0)
			continue;
		if ((uintptr_t)(P_ENTRY(dbp, h, i) - (u_int8_t *)h) <
		    (uintptr_t)(P_OVERHEAD(dbp)) ||
		    (size_t)(P_ENTRY(dbp, h, i) - (u_int8_t *)h) >= pagesize) {
			__db_msg(env,
			    "ILLEGAL PAGE OFFSET: indx: %lu of %lu",
			    (u_long)i, (u_long)inp[i]);
			ret = EINVAL;
			continue;
		}
		deleted = 0;
		switch (TYPE(h)) {
		case P_HASH_UNSORTED:
		case P_HASH:
		case P_IBTREE:
		case P_IRECNO:
			sp = PR_ENTRY(dbp, h, i, data);
			break;
		case P_HEAP:
			sp = P_ENTRY(dbp, h, i);
			break;
		case P_LBTREE:
			sp = PR_ENTRY(dbp, h, i, data);
			deleted = i % 2 == 0 &&
			    B_DISSET(GET_BKEYDATA(dbp, h, i + O_INDX)->type);
			break;
		case P_LDUP:
		case P_LRECNO:
			sp = PR_ENTRY(dbp, h, i, data);
			deleted = B_DISSET(GET_BKEYDATA(dbp, h, i)->type);
			break;
		default:
			goto type_err;
		}
		__db_msgadd(env, mbp, "%s", deleted ? "       D" : "\t");
		__db_msgadd(
		    env, mbp, "[%03lu] %4lu ", (u_long)i, (u_long)inp[i]);
		switch (TYPE(h)) {
		case P_HASH_UNSORTED:
		case P_HASH:
			hk = sp;
			switch (HPAGE_PTYPE(hk)) {
			case H_OFFDUP:
				memcpy(&pgno,
				    HOFFDUP_PGNO(hk), sizeof(db_pgno_t));
				__db_msgadd(env, mbp,
				    "%4lu [offpage dups]", (u_long)pgno);
				DB_MSGBUF_FLUSH(env, mbp);
				break;
			case H_DUPLICATE:
				/*
				 * If this is the first item on a page, then
				 * we cannot figure out how long it is, so
				 * we only print the first one in the duplicate
				 * set.
				 */
				if (i != 0)
					len = LEN_HKEYDATA(dbp, h, 0, i);
				else
					len = 1;

				__db_msgadd(env, mbp, "Duplicates:");
				DB_MSGBUF_FLUSH(env, mbp);
				for (p = HKEYDATA_DATA(hk),
				    ep = p + len; p < ep;) {
					memcpy(&dlen, p, sizeof(db_indx_t));
					p += sizeof(db_indx_t);
					__db_msgadd(env, mbp, "\t\t");
					__db_prbytes(env, mbp, p, dlen);
					p += sizeof(db_indx_t) + dlen;
				}
				break;
			case H_KEYDATA:
				__db_prbytes(env, mbp, HKEYDATA_DATA(hk),
				    LEN_HKEYDATA(dbp, h, i == 0 ?
				    pagesize : 0, i));
				break;
			case H_OFFPAGE:
				memcpy(&a_hkd, hk, HOFFPAGE_SIZE);
				__db_msgadd(env, mbp,
				    "overflow: total len: %4lu page: %4lu",
				    (u_long)a_hkd.tlen, (u_long)a_hkd.pgno);
				DB_MSGBUF_FLUSH(env, mbp);
				break;
			case H_BLOB:
				memcpy(&hblob, hk, HBLOB_SIZE);
				blob_id = (db_seq_t)hblob.id;
				__db_msgadd(env, mbp, "blob: id: %llu ",
				    (long long)blob_id);
				GET_BLOB_SIZE(env, hblob, blob_size, ret);
				if (ret != 0)
					__db_msgadd(env, mbp,
					    "blob: blob_size overflow. ");
				__db_msgadd(env, mbp, "blob: size: %llu",
				    (long long)blob_size);
				/*
				 * No point printing the blob file, it is
				 * likely not readable by humans.
				 */
				DB_MSGBUF_FLUSH(env, mbp);
				break;
			default:
				DB_MSGBUF_FLUSH(env, mbp);
				__db_msg(env, "ILLEGAL HASH PAGE TYPE: %lu",
				    (u_long)HPAGE_PTYPE(hk));
				ret = EINVAL;
				break;
			}
			break;
		case P_IBTREE:
			bi = sp;

			if (F_ISSET(dbp, DB_AM_RECNUM))
				__db_msgadd(env, mbp,
				    "count: %4lu ", (u_long)bi->nrecs);
			__db_msgadd(env, mbp,
			    "pgno: %4lu type: %lu ",
			    (u_long)bi->pgno, (u_long)bi->type);
			switch (B_TYPE(bi->type)) {
			case B_KEYDATA:
				__db_prbytes(env, mbp, bi->data, bi->len);
				break;
			case B_DUPLICATE:
			case B_OVERFLOW:
				__db_proff(env, mbp, bi->data);
				break;
			default:
				/* B_BLOB does not appear on internal pages. */
				DB_MSGBUF_FLUSH(env, mbp);
				__db_msg(env, "ILLEGAL BINTERNAL TYPE: %lu",
				    (u_long)B_TYPE(bi->type));
				ret = EINVAL;
				break;
			}
			break;
		case P_IRECNO:
			ri = sp;
			__db_msgadd(env, mbp, "entries %4lu pgno %4lu",
			    (u_long)ri->nrecs, (u_long)ri->pgno);
			DB_MSGBUF_FLUSH(env, mbp);
			break;
		case P_LBTREE:
		case P_LDUP:
		case P_LRECNO:
			bk = sp;
			switch (B_TYPE(bk->type)) {
			case B_KEYDATA:
				__db_prbytes(env, mbp, bk->data, bk->len);
				break;
			case B_DUPLICATE:
			case B_OVERFLOW:
				__db_proff(env, mbp, bk);
				break;
			case B_BLOB:
				memcpy(&bl, bk, BBLOB_SIZE);
				blob_id = (db_seq_t)bl.id;
				__db_msgadd(env, mbp, "blob: id: %llu ",
				    (long long)blob_id);
				GET_BLOB_SIZE(env, bl, blob_size, ret);
				if (ret != 0)
					__db_msgadd(env, mbp,
					    "blob: blob_size overflow. ");
				__db_msgadd(env, mbp, "blob: size: %llu",
				    (long long)blob_size);
				DB_MSGBUF_FLUSH(env, mbp);
				break;
			default:
				DB_MSGBUF_FLUSH(env, mbp);
				__db_msg(env,
			    "ILLEGAL DUPLICATE/LBTREE/LRECNO TYPE: %lu",
				    (u_long)B_TYPE(bk->type));
				ret = EINVAL;
				break;
			}
			break;
		case P_HEAP:
			hh = sp;
			if (!F_ISSET(hh,HEAP_RECSPLIT) && 
			    !F_ISSET(hh, HEAP_RECBLOB))
				hdata = (u_int8_t *)hh + sizeof(HEAPHDR);
			else if (F_ISSET(hh, HEAP_RECBLOB)) {
				memcpy(&bhdr, hh, HEAPBLOBREC_SIZE);
				blob_id = (db_seq_t)bhdr.id;
				__db_msgadd(env, mbp, "blob: id: %llu ",
				    (long long)blob_id);
				GET_BLOB_SIZE(env, bhdr, blob_size, ret);
				if (ret != 0)
					__db_msgadd(env, mbp,
					    "blob: blob_size overflow. ");
				__db_msgadd(env, mbp, "blob: size: %llu",
				    (long long)blob_size);
				/*
				 * No point printing the blob file, it is
				 * likely not readable by humans.
				 */
				DB_MSGBUF_FLUSH(env, mbp);
				break;
			} else {
				hs = sp;
				__db_msgadd(env, mbp,
				     "split: 0x%02x tsize: %lu next: %lu.%lu ",
				     hh->flags, (u_long)hs->tsize,
				     (u_long)hs->nextpg, (u_long)hs->nextindx);

				hdata = (u_int8_t *)hh + sizeof(HEAPSPLITHDR);
			}
			__db_prbytes(env, mbp, hdata, hh->size);
			break;
		default:
type_err:		DB_MSGBUF_FLUSH(env, mbp);
			__db_msg(env,
			    "ILLEGAL PAGE TYPE: %lu", (u_long)TYPE(h));
			ret = EINVAL;
			continue;
		}
	}
	return (ret);
}

/*
 * __db_prbytes --
 *	Print out a data element.
 *
 * PUBLIC: void __db_prbytes __P((ENV *, DB_MSGBUF *, u_int8_t *, u_int32_t));
 */
void
__db_prbytes(env, mbp, bytes, len)
	ENV *env;
	DB_MSGBUF *mbp;
	u_int8_t *bytes;
	u_int32_t len;
{
	u_int8_t *p;
	u_int32_t i, not_printable;
	int msg_truncated;

	__db_msgadd(env, mbp, "len: %3lu", (u_long)len);
	if (len != 0) {
		__db_msgadd(env, mbp, " data: ");

		/*
		 * Print the first N bytes of the data.   If that
		 * chunk is at least 3/4  printable characters, print
		 * it as text, else print it in hex.  We have this
		 * heuristic because we're displaying things like
		 * lock objects that could be either text or data.
		 */
		if (len > env->data_len) {
			len = env->data_len;
			msg_truncated = 1;
		} else
			msg_truncated = 0;
		not_printable = 0;
		for (p = bytes, i = 0; i < len; ++i, ++p) {
			if (!isprint((int)*p) && *p != '\t' && *p != '\n') {
				if (i == len - 1 && *p == '\0')
					break;
				if (++not_printable >= (len >> 2))
					break;
			}
		}
		if (not_printable < (len >> 2))
			for (p = bytes, i = len; i > 0; --i, ++p) {
				if (isprint((int)*p))
					__db_msgadd(env, mbp, "%c", *p);
				else
					__db_msgadd(env,
					    mbp, "\\%x", (u_int)*p);
			}
		else
			for (p = bytes, i = len; i > 0; --i, ++p)
				__db_msgadd(env, mbp, "%.2x", (u_int)*p);
		if (msg_truncated)
			__db_msgadd(env, mbp, "...");
	}
	DB_MSGBUF_FLUSH(env, mbp);
}

/*
 * __db_proff --
 *	Print out an off-page element.
 */
static void
__db_proff(env, mbp, vp)
	ENV *env;
	DB_MSGBUF *mbp;
	void *vp;
{
	BOVERFLOW *bo;

	bo = vp;
	switch (B_TYPE(bo->type)) {
	case B_OVERFLOW:
		__db_msgadd(env, mbp, "overflow: total len: %4lu page: %4lu",
		    (u_long)bo->tlen, (u_long)bo->pgno);
		break;
	case B_DUPLICATE:
		__db_msgadd(
		    env, mbp, "duplicate: page: %4lu", (u_long)bo->pgno);
		break;
	default:
		/* NOTREACHED */
		break;
	}
	DB_MSGBUF_FLUSH(env, mbp);
}

/*
 * __db_prflags --
 *	Print out flags values.
 *
 * PUBLIC: void __db_prflags __P((ENV *, DB_MSGBUF *,
 * PUBLIC:     u_int32_t, const FN *, const char *, const char *));
 */
void
__db_prflags(env, mbp, flags, fn, prefix, suffix)
	ENV *env;
	DB_MSGBUF *mbp;
	u_int32_t flags;
	FN const *fn;
	const char *prefix, *suffix;
{
	DB_MSGBUF mb;
	const FN *fnp;
	int found, standalone;
	const char *sep;

	if (fn == NULL)
		return;

	/*
	 * If it's a standalone message, output the suffix (which will be the
	 * label), regardless of whether we found anything or not, and flush
	 * the line.
	 */
	if (mbp == NULL) {
		standalone = 1;
		mbp = &mb;
		DB_MSGBUF_INIT(mbp);
	} else
		standalone = 0;

	sep = prefix == NULL ? "" : prefix;
	for (found = 0, fnp = fn; fnp->mask != 0; ++fnp)
		if (LF_ISSET(fnp->mask)) {
			__db_msgadd(env, mbp, "%s%s", sep, fnp->name);
			sep = ", ";
			found = 1;
		}

	if ((standalone || found) && suffix != NULL)
		__db_msgadd(env, mbp, "%s", suffix);
	if (standalone)
		DB_MSGBUF_FLUSH(env, mbp);
}

/*
 * __db_name_to_val --
 *	Return the integral value associated with the string, or -1 if missing.
 *	It is intended for looking up string names of enums and single bit
 *	in order to get a numeric value.
 *
 * PUBLIC: int __db_name_to_val __P((FN const *, char *));
 */
int
__db_name_to_val(strtable, s)
	FN const *strtable;
	char *s;
{
	if (s != NULL) {
		do {
			if (strcasecmp(strtable->name, s) == 0)
				return ((int)strtable->mask);
		} while ((++strtable)->name != NULL);
	}
	return (-1);
}

/*
 * __db_pagetype_to_string --
 *	Return the name of the specified page type.
 * PUBLIC: const char *__db_pagetype_to_string __P((u_int32_t));
 */
const char *
__db_pagetype_to_string(type)
	u_int32_t type;
{
	char *s;

	s = NULL;
	switch (type) {
	case P_BTREEMETA:
		s = "btree metadata";
		break;
	case P_LDUP:
		s = "duplicate";
		break;
	case P_HASH_UNSORTED:
		s = "hash unsorted";
		break;
	case P_HASH:
		s = "hash";
		break;
	case P_HASHMETA:
		s = "hash metadata";
		break;
	case P_IBTREE:
		s = "btree internal";
		break;
	case P_INVALID:
		s = "invalid";
		break;
	case P_IRECNO:
		s = "recno internal";
		break;
	case P_LBTREE:
		s = "btree leaf";
		break;
	case P_LRECNO:
		s = "recno leaf";
		break;
	case P_OVERFLOW:
		s = "overflow";
		break;
	case P_QAMMETA:
		s = "queue metadata";
		break;
	case P_QAMDATA:
		s = "queue";
		break;
	case P_HEAPMETA:
		s = "heap metadata";
		break;
	case P_HEAP:
		s = "heap data";
		break;
	case P_IHEAP:
		s = "heap internal";
		break;
	default:
		/* Just return a NULL. */
		break;
	}
	return (s);
}

/*
 * __db_dump_pp --
 *	DB->dump pre/post processing.
 *
 * PUBLIC: int __db_dump_pp __P((DB *, const char *,
 * PUBLIC:     int (*)(void *, const void *), void *, int, int));
 */
int
__db_dump_pp(dbp, subname, callback, handle, pflag, keyflag)
	DB *dbp;
	const char *subname;
	int (*callback) __P((void *, const void *));
	void *handle;
	int pflag, keyflag;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;

	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->dump");

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (ret = __db_rep_enter(dbp, 1, 0, 1)) != 0) {
		handle_check = 0;
		goto err;
	}

	ret = __db_dump(dbp, subname, callback, handle, pflag, keyflag);

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

err:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_dump --
 *	DB->dump.
 *
 * PUBLIC: int __db_dump __P((DB *, const char *,
 * PUBLIC:     int (*)(void *, const void *), void *, int, int));
 */
int
__db_dump(dbp, subname, callback, handle, pflag, keyflag)
	DB *dbp;
	const char *subname;
	int (*callback) __P((void *, const void *));
	void *handle;
	int pflag, keyflag;
{
	DBC *dbcp;
	DBT key, data;
	DBT keyret, dataret;
	DB_HEAP_RID rid;
	ENV *env;
	db_recno_t recno;
	int is_recno, is_heap, ret, t_ret;
	u_int32_t blob_threshold;
	void *pointer;

	env = dbp->env;
	is_heap = 0;
	memset(&dataret, 0, sizeof(DBT));
	memset(&keyret, 0, sizeof(DBT));

	if ((ret = __db_get_blob_threshold(dbp, &blob_threshold)) != 0)
		return (ret);

	if ((ret = __db_prheader(
	    dbp, subname, pflag, keyflag, handle, callback, NULL, 0)) != 0)
		return (ret);

	/*
	 * Get a cursor and step through the database, printing out each
	 * key/data pair.
	 */
	if ((ret = __db_cursor(dbp, NULL, NULL, &dbcp, 0)) != 0)
		return (ret);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	if ((ret = __os_malloc(env, 1024 * 1024, &data.data)) != 0)
		goto err;
	data.ulen = 1024 * 1024;
	data.flags = DB_DBT_USERMEM;
	is_recno = (dbp->type == DB_RECNO || dbp->type == DB_QUEUE);
	keyflag = is_recno ? keyflag : 1;
	if (is_recno) {
		keyret.data = &recno;
		keyret.size = sizeof(recno);
	}

	if (dbp->type == DB_HEAP) {
		is_heap = 1;
		key.data = &rid;
		key.size = key.ulen = sizeof(DB_HEAP_RID);
		key.flags = DB_DBT_USERMEM;
	}

retry: while ((ret =
	    __dbc_get(dbcp, &key, &data,
	    !is_heap ? DB_NEXT | DB_MULTIPLE_KEY : DB_NEXT )) == 0) {
		if (is_heap) {
			/* Never dump keys for HEAP */
			if ((ret = __db_prdbt(&data,
			    pflag, " ", handle, callback, 0, 0, 0)) != 0)
				goto err;
			continue;
		}
		DB_MULTIPLE_INIT(pointer, &data);
		for (;;) {
			if (is_recno)
				DB_MULTIPLE_RECNO_NEXT(pointer, &data,
				    recno, dataret.data, dataret.size);
			else
				DB_MULTIPLE_KEY_NEXT(pointer, &data,
				    keyret.data, keyret.size,
				    dataret.data, dataret.size);

			if (dataret.data == NULL)
				break;

			if ((keyflag &&
			    (ret = __db_prdbt(&keyret, pflag, " ",
			    handle, callback, is_recno, 0, 0)) != 0) ||
			    (ret = __db_prdbt(&dataret, pflag, " ",
			    handle, callback, 0, 0, 0)) != 0)
					goto err;
		}
	}
	if (ret == DB_BUFFER_SMALL) {
		if (blob_threshold != 0 && data.size >= blob_threshold) {
			if ((ret = __db_prblob(dbcp, &key, &data,
			    pflag, " ", handle, callback, is_heap, keyflag)) != 0)
				goto err;
		} else {
			data.size = (u_int32_t)DB_ALIGN(data.size, 1024);
			if ((ret = __os_realloc(
			    env, data.size, &data.data)) != 0)
				goto err;
			data.ulen = data.size;
		}
		goto retry;
	}
	if (ret == DB_NOTFOUND)
		ret = 0;

	if ((t_ret = __db_prfooter(handle, callback)) != 0 && ret == 0)
		ret = t_ret;

err:	if ((t_ret = __dbc_close(dbcp)) != 0 && ret == 0)
		ret = t_ret;
	if (data.data != NULL)
		__os_free(env, data.data);

	return (ret);
}

/*
 * __db_prblob
 *	Print a blob file.
 */
static int
__db_prblob(dbc, key, data, checkprint,
    prefix, handle, callback, is_heap, keyflag)
	DBC *dbc;
	DBT *key;
	DBT *data;
	int checkprint;
	const char *prefix;
	void *handle;
	int (*callback) __P((void *, const void *));
	int is_heap;
	int keyflag;
{
	DBC *local;
	DBT partial;
	int ret, t_ret;
	off_t blob_size;
	db_seq_t blob_id;

	local = NULL;
	memset(&partial, 0, sizeof(DBT));
	partial.flags = DB_DBT_PARTIAL;

	if ((ret = __dbc_idup(dbc, &local, DB_POSITION)) != 0)
		goto err;

	/* Move the cursor to the blob. */
	if ((ret = __dbc_get(local, key, &partial, DB_NEXT)) != 0)
		return (ret);

	if ((ret = __dbc_get_blob_id(local, &blob_id)) != 0) {
		/*
		 * It is possible this is not a blob.  Non-blob items that are
		 * larger than the blob threshold can exist if the item was
		 * smaller than the threshold when created, then later updated
		 * to larger than the threshold value.
		 */
		if (ret == EINVAL) {
			ret = 0;
			data->size = (u_int32_t)DB_ALIGN(data->size, 1024);
			if ((ret = __os_realloc(
			    dbc->env, data->size, &data->data)) != 0)
				goto err;
			data->ulen = data->size;
		}
		goto err;
	}

	if (data->ulen < MEGABYTE) {
		if ((data->data = realloc(
		    data->data, data->ulen = MEGABYTE)) == NULL) {
			ret = ENOMEM;
			goto err;
		}
	}

	if ((ret = __dbc_get_blob_size(local, &blob_size)) != 0)
		goto err;

	if (keyflag && !is_heap && (ret = __db_prdbt(
	    key, checkprint, " ", handle, callback, 0, 0, 0)) != 0)
		goto err;

	if ((ret = __db_prblob_id(local->dbp, blob_id, blob_size,
	    data, checkprint, prefix, handle, callback)) != 0)
		goto err;

	/* Move the cursor. */
	ret = __dbc_get(dbc, key, &partial, DB_NEXT);

err:	if (local != NULL) {
		if ((t_ret = __dbc_close(local)) != 0 && ret == 0)
			ret = t_ret;
	}

	return (ret);
}

/*
 * __db_prblob_id --
 *	Print a blob file identified by the given id.
 */
static int
__db_prblob_id(dbp, blob_id,
    blob_size, data, checkprint, prefix, handle, callback)
	DB *dbp;
	db_seq_t blob_id;
	off_t blob_size;
	DBT *data;
	int checkprint;
	const char *prefix;
	void *handle;
	int (*callback) __P((void *, const void *));
{
	DB_FH *fhp;
	const char *pre;
	int ret, skip_newline, t_ret;
	off_t left, offset;

	fhp = NULL;
	offset = 0;

	if ((ret = __blob_file_open(dbp, &fhp, blob_id, DB_FOP_READONLY)) != 0)
		goto err;

	left = blob_size;
	while (left > 0) {
		if ((ret = __blob_file_read(
		    dbp->env, fhp, data, offset, data->ulen)) != 0)
			goto err;
		if (offset == 0)
			pre = prefix;
		else
			pre = NULL;
		skip_newline = data->size < left ? 1 : 0;
		if ((ret = __db_prdbt(data, checkprint, pre,
		    handle, callback, 0, 0, skip_newline)) != 0)
			goto err;
		if (data->size > left)
			left = 0;
		else
			left = left - data->size;
		offset = offset + data->size;
	}

err:	if (fhp != NULL) {
		if ((t_ret = __os_closehandle(dbp->env, fhp)) != 0 && ret == 0)
			ret = t_ret;
	}

	return (ret);
}

/*
 * __db_prdbt --
 *	Print out a DBT data element.
 *
 * PUBLIC: int __db_prdbt __P((DBT *, int, const char *, void *,
 * PUBLIC:     int (*)(void *, const void *), int, int, int));
 */
int
__db_prdbt(dbtp, checkprint,
    prefix, handle, callback, is_recno, is_heap, no_newline)
	DBT *dbtp;
	int checkprint;
	const char *prefix;
	void *handle;
	int (*callback) __P((void *, const void *));
	int is_recno;
	int is_heap;
	int no_newline;
{
	static const u_char hex[] = "0123456789abcdef";
	db_recno_t recno;
	DB_HEAP_RID rid;
	size_t len;
	int ret;
#define	DBTBUFLEN	100
	u_int8_t *p, *hp;
	char buf[DBTBUFLEN], hbuf[DBTBUFLEN];

	ret = 0;
	/*
	 * !!!
	 * This routine is the routine that dumps out items in the format
	 * used by db_dump(1) and db_load(1).  This means that the format
	 * cannot change.
	 */
	if (prefix != NULL && (ret = callback(handle, prefix)) != 0)
		return (ret);
	if (is_recno) {
		/*
		 * We're printing a record number, and this has to be done
		 * in a platform-independent way.  So we use the numeral in
		 * straight ASCII.
		 */
		(void)__ua_memcpy(&recno, dbtp->data, sizeof(recno));
		snprintf(buf, DBTBUFLEN, "%lu", (u_long)recno);

		/* If we're printing data as hex, print keys as hex too. */
		if (!checkprint) {
			for (len = strlen(buf), p = (u_int8_t *)buf,
			    hp = (u_int8_t *)hbuf; len-- > 0; ++p) {
				*hp++ = hex[(u_int8_t)(*p & 0xf0) >> 4];
				*hp++ = hex[*p & 0x0f];
			}
			*hp = '\0';
			ret = callback(handle, hbuf);
		} else
			ret = callback(handle, buf);

		if (ret != 0)
			return (ret);
	} else if (is_heap) {
		/*
		 * We're printing a heap record number, and this has to be
		 * done in a platform-independent way.  So we use the numeral
		 * in straight ASCII.
		 */
		(void)__ua_memcpy(&rid, dbtp->data, sizeof(rid));
		snprintf(buf, DBTBUFLEN, "%lu %hu",
		    (u_long)rid.pgno, (u_short)rid.indx);

		/* If we're printing data as hex, print keys as hex too. */
		if (!checkprint) {
			for (len = strlen(buf), p = (u_int8_t *)buf,
			    hp = (u_int8_t *)hbuf; len-- > 0; ++p) {
				*hp++ = hex[(u_int8_t)(*p & 0xf0) >> 4];
				*hp++ = hex[*p & 0x0f];
			}
			*hp = '\0';
			ret = callback(handle, hbuf);
		} else
			ret = callback(handle, buf);

		if (ret != 0)
			return (ret);
	} else if (checkprint) {
		for (len = dbtp->size, p = dbtp->data; len--; ++p)
			if (isprint((int)*p)) {
				if (*p == '\\' &&
				    (ret = callback(handle, "\\")) != 0)
					return (ret);
				snprintf(buf, DBTBUFLEN, "%c", *p);
				if ((ret = callback(handle, buf)) != 0)
					return (ret);
			} else {
				snprintf(buf, DBTBUFLEN, "\\%c%c",
				    hex[(u_int8_t)(*p & 0xf0) >> 4],
				    hex[*p & 0x0f]);
				if ((ret = callback(handle, buf)) != 0)
					return (ret);
			}
	} else
		for (len = dbtp->size, p = dbtp->data; len--; ++p) {
			snprintf(buf, DBTBUFLEN, "%c%c",
			    hex[(u_int8_t)(*p & 0xf0) >> 4],
			    hex[*p & 0x0f]);
			if ((ret = callback(handle, buf)) != 0)
				return (ret);
		}
	if (no_newline == 0)
		return (callback(handle, "\n"));
	else
		return (ret);
}

/*
 * __db_prheader --
 *	Write out header information in the format expected by db_load.
 *
 * PUBLIC: int	__db_prheader __P((DB *, const char *, int, int, void *,
 * PUBLIC:     int (*)(void *, const void *), VRFY_DBINFO *, db_pgno_t));
 */
int
__db_prheader(dbp, subname, pflag, keyflag, handle, callback, vdp, meta_pgno)
	DB *dbp;
	const char *subname;
	int pflag, keyflag;
	void *handle;
	int (*callback) __P((void *, const void *));
	VRFY_DBINFO *vdp;
	db_pgno_t meta_pgno;
{
	DBT dbt;
	DBTYPE dbtype;
	ENV *env;
	VRFY_PAGEINFO *pip;
	u_int32_t flags, tmp_u_int32;
	size_t buflen;
	char *buf;
	int using_vdp, ret, t_ret, tmp_int;
#ifdef HAVE_HEAP
	u_int32_t tmp2_u_int32;
#endif

	ret = 0;
	buf = NULL;
	COMPQUIET(buflen, 0);

	/*
	 * If dbp is NULL, then pip is guaranteed to be non-NULL; we only ever
	 * call __db_prheader with a NULL dbp from one case inside __db_prdbt,
	 * and this is a special subdatabase for "lost" items.  In this case
	 * we have a vdp (from which we'll get a pip).  In all other cases, we
	 * will have a non-NULL dbp (and vdp may or may not be NULL depending
	 * on whether we're salvaging).
	 */
	if (dbp == NULL)
		env = NULL;
	else
		env = dbp->env;
	DB_ASSERT(env, dbp != NULL || vdp != NULL);

	/*
	 * If we've been passed a verifier statistics object, use that;  we're
	 * being called in a context where dbp->stat is unsafe.
	 *
	 * Also, the verifier may set the pflag on a per-salvage basis.  If so,
	 * respect that.
	 */
	if (vdp != NULL) {
		if ((ret = __db_vrfy_getpageinfo(vdp, meta_pgno, &pip)) != 0)
			return (ret);

		if (F_ISSET(vdp, SALVAGE_PRINTABLE))
			pflag = 1;
		using_vdp = 1;
	} else {
		pip = NULL;
		using_vdp = 0;
	}

	/*
	 * If dbp is NULL, make it a btree.  Otherwise, set dbtype to whatever
	 * appropriate type for the specified meta page, or the type of the dbp.
	 */
	if (dbp == NULL)
		dbtype = DB_BTREE;
	else if (using_vdp)
		switch (pip->type) {
		case P_BTREEMETA:
			if (F_ISSET(pip, VRFY_IS_RECNO))
				dbtype = DB_RECNO;
			else
				dbtype = DB_BTREE;
			break;
		case P_HASHMETA:
			dbtype = DB_HASH;
			break;
		case P_HEAPMETA:
			dbtype = DB_HEAP;
			break;
		case P_QAMMETA:
			dbtype = DB_QUEUE;
			break;
		default:
			/*
			 * If the meta page is of a bogus type, it's because
			 * we have a badly corrupt database.  (We must be in
			 * the verifier for pip to be non-NULL.) Pretend we're
			 * a Btree and salvage what we can.
			 */
			DB_ASSERT(env, F_ISSET(dbp, DB_AM_VERIFYING));
			dbtype = DB_BTREE;
			break;
		}
	else
		dbtype = dbp->type;

	if ((ret = callback(handle, "VERSION=3\n")) != 0)
		goto err;
	if (pflag) {
		if ((ret = callback(handle, "format=print\n")) != 0)
			goto err;
	} else if ((ret = callback(handle, "format=bytevalue\n")) != 0)
		goto err;

	/*
	 * 64 bytes is long enough, as a minimum bound, for any of the
	 * fields besides subname.  Subname uses __db_prdbt and therefore
	 * does not need buffer space here.
	 */
	buflen = 64;
	if ((ret = __os_malloc(env, buflen, &buf)) != 0)
		goto err;
	if (subname != NULL) {
		snprintf(buf, buflen, "database=");
		if ((ret = callback(handle, buf)) != 0)
			goto err;
		DB_INIT_DBT(dbt, subname, strlen(subname));
		if ((ret = __db_prdbt(&dbt, 1,
		    NULL, handle, callback, 0, 0, 0)) != 0)
			goto err;
	}
	switch (dbtype) {
	case DB_BTREE:
		if ((ret = callback(handle, "type=btree\n")) != 0)
			goto err;
		if (using_vdp)
			tmp_int = F_ISSET(pip, VRFY_HAS_RECNUMS) ? 1 : 0;
		else {
			if ((ret = __db_get_flags(dbp, &flags)) != 0) {
				__db_err(env, ret, "DB->get_flags");
				goto err;
			}
			tmp_int = F_ISSET(dbp, DB_AM_RECNUM) ? 1 : 0;
		}
		if (tmp_int && (ret = callback(handle, "recnum=1\n")) != 0)
			goto err;

		if (using_vdp)
			tmp_u_int32 = pip->bt_minkey;
		else
			if ((ret =
			    __bam_get_bt_minkey(dbp, &tmp_u_int32)) != 0) {
				__db_err(env, ret, "DB->get_bt_minkey");
				goto err;
			}
		if (tmp_u_int32 != 0 && tmp_u_int32 != DEFMINKEYPAGE) {
			snprintf(buf, buflen,
			    "bt_minkey=%lu\n", (u_long)tmp_u_int32);
			if ((ret = callback(handle, buf)) != 0)
				goto err;
		}
		break;
	case DB_HASH:
#ifdef HAVE_HASH
		if ((ret = callback(handle, "type=hash\n")) != 0)
			goto err;
		if (using_vdp)
			tmp_u_int32 = pip->h_ffactor;
		else
			if ((ret =
			    __ham_get_h_ffactor(dbp, &tmp_u_int32)) != 0) {
				__db_err(env, ret, "DB->get_h_ffactor");
				goto err;
			}
		if (tmp_u_int32 != 0) {
			snprintf(buf, buflen,
			    "h_ffactor=%lu\n", (u_long)tmp_u_int32);
			if ((ret = callback(handle, buf)) != 0)
				goto err;
		}

		if (using_vdp)
			tmp_u_int32 = pip->h_nelem;
		else
			if ((ret = __ham_get_h_nelem(dbp, &tmp_u_int32)) != 0) {
				__db_err(env, ret, "DB->get_h_nelem");
				goto err;
			}
		/*
		 * Hash databases have an h_nelem field of 0 or 1, neither
		 * of those values is interesting.
		 */
		if (tmp_u_int32 > 1) {
			snprintf(buf, buflen,
			    "h_nelem=%lu\n", (u_long)tmp_u_int32);
			if ((ret = callback(handle, buf)) != 0)
				goto err;
		}
		break;
#else
		ret = __db_no_hash_am(env);
		goto err;
#endif
	case DB_HEAP:
#ifdef HAVE_HEAP
		if ((ret = callback(handle, "type=heap\n")) != 0)
			goto err;

		if ((ret = __heap_get_heapsize(
		    dbp, &tmp_u_int32, &tmp2_u_int32)) != 0) {
			__db_err(env, ret, "DB->get_heapsize");
			goto err;
		}
		if (tmp_u_int32 != 0) {
			snprintf(buf,
			    buflen, "heap_gbytes=%lu\n", (u_long)tmp_u_int32);
			if ((ret = callback(handle, buf)) != 0)
				goto err;
		}
		if (tmp2_u_int32 != 0) {
			snprintf(buf,
			    buflen, "heap_bytes=%lu\n", (u_long)tmp2_u_int32);
			if ((ret = callback(handle, buf)) != 0)
				goto err;
		}

		if ((ret =
		    __heap_get_heap_regionsize(dbp, &tmp_u_int32)) != 0) {
			__db_err(env, ret, "DB->get_heap_regionsize");
			goto err;
		}
		if (tmp_u_int32 != 0) {
			snprintf(buf, buflen,
			    "heap_regionsize=%lu\n", (u_long)tmp_u_int32);
			if ((ret = callback(handle, buf)) != 0)
				goto err;
		}
		break;
#else
		ret = __db_no_heap_am(env);
		goto err;
#endif
	case DB_QUEUE:
#ifdef HAVE_QUEUE
		if ((ret = callback(handle, "type=queue\n")) != 0)
			goto err;
		if (using_vdp)
			tmp_u_int32 = vdp->re_len;
		else
			if ((ret = __ram_get_re_len(dbp, &tmp_u_int32)) != 0) {
				__db_err(env, ret, "DB->get_re_len");
				goto err;
			}
		snprintf(buf, buflen, "re_len=%lu\n", (u_long)tmp_u_int32);
		if ((ret = callback(handle, buf)) != 0)
			goto err;

		if (using_vdp)
			tmp_int = (int)vdp->re_pad;
		else
			if ((ret = __ram_get_re_pad(dbp, &tmp_int)) != 0) {
				__db_err(env, ret, "DB->get_re_pad");
				goto err;
			}
		if (tmp_int != 0 && tmp_int != ' ') {
			snprintf(buf, buflen, "re_pad=%#x\n", tmp_int);
			if ((ret = callback(handle, buf)) != 0)
				goto err;
		}

		if (using_vdp)
			tmp_u_int32 = vdp->page_ext;
		else
			if ((ret =
			    __qam_get_extentsize(dbp, &tmp_u_int32)) != 0) {
				__db_err(env, ret, "DB->get_q_extentsize");
				goto err;
			}
		if (tmp_u_int32 != 0) {
			snprintf(buf, buflen,
			    "extentsize=%lu\n", (u_long)tmp_u_int32);
			if ((ret = callback(handle, buf)) != 0)
				goto err;
		}
		break;
#else
		ret = __db_no_queue_am(env);
		goto err;
#endif
	case DB_RECNO:
		if ((ret = callback(handle, "type=recno\n")) != 0)
			goto err;
		if (using_vdp)
			tmp_int = F_ISSET(pip, VRFY_IS_RRECNO) ? 1 : 0;
		else
			tmp_int = F_ISSET(dbp, DB_AM_RENUMBER) ? 1 : 0;
		if (tmp_int != 0 &&
		    (ret = callback(handle, "renumber=1\n")) != 0)
				goto err;

		if (using_vdp)
			tmp_int = F_ISSET(pip, VRFY_IS_FIXEDLEN) ? 1 : 0;
		else
			tmp_int = F_ISSET(dbp, DB_AM_FIXEDLEN) ? 1 : 0;
		if (tmp_int) {
			if (using_vdp)
				tmp_u_int32 = pip->re_len;
			else
				if ((ret =
				    __ram_get_re_len(dbp, &tmp_u_int32)) != 0) {
					__db_err(env, ret, "DB->get_re_len");
					goto err;
				}
			snprintf(buf, buflen,
			    "re_len=%lu\n", (u_long)tmp_u_int32);
			if ((ret = callback(handle, buf)) != 0)
				goto err;

			if (using_vdp)
				tmp_int = (int)pip->re_pad;
			else
				if ((ret =
				    __ram_get_re_pad(dbp, &tmp_int)) != 0) {
					__db_err(env, ret, "DB->get_re_pad");
					goto err;
				}
			if (tmp_int != 0 && tmp_int != ' ') {
				snprintf(buf,
				    buflen, "re_pad=%#x\n", (u_int)tmp_int);
				if ((ret = callback(handle, buf)) != 0)
					goto err;
			}
		}
		break;
	case DB_UNKNOWN:			/* Impossible. */
		ret = __db_unknown_path(env, "__db_prheader");
		goto err;
	}

	if (using_vdp) {
		if (F_ISSET(pip, VRFY_HAS_CHKSUM))
			if ((ret = callback(handle, "chksum=1\n")) != 0)
				goto err;
		if (F_ISSET(pip, VRFY_HAS_DUPS))
			if ((ret = callback(handle, "duplicates=1\n")) != 0)
				goto err;
		if (F_ISSET(pip, VRFY_HAS_DUPSORT))
			if ((ret = callback(handle, "dupsort=1\n")) != 0)
				goto err;
#ifdef HAVE_COMPRESSION
		if (F_ISSET(pip, VRFY_HAS_COMPRESS))
			if ((ret = callback(handle, "compressed=1\n")) != 0)
				goto err;
#endif
		/*
		 * !!!
		 * We don't know if the page size was the default if we're
		 * salvaging.  It doesn't seem that interesting to have, so
		 * we ignore it for now.
		 */
	} else {
		if (F_ISSET(dbp, DB_AM_CHKSUM))
			if ((ret = callback(handle, "chksum=1\n")) != 0)
				goto err;
		if (F_ISSET(dbp, DB_AM_DUP))
			if ((ret = callback(handle, "duplicates=1\n")) != 0)
				goto err;
		if (F_ISSET(dbp, DB_AM_DUPSORT))
			if ((ret = callback(handle, "dupsort=1\n")) != 0)
				goto err;
#ifdef HAVE_COMPRESSION
		if (DB_IS_COMPRESSED(dbp))
			if ((ret = callback(handle, "compressed=1\n")) != 0)
				goto err;
#endif
		if (!F_ISSET(dbp, DB_AM_PGDEF)) {
			snprintf(buf, buflen,
			    "db_pagesize=%lu\n", (u_long)dbp->pgsize);
			if ((ret = callback(handle, buf)) != 0)
				goto err;
		}
	}

#ifdef HAVE_PARTITION
	if (dbp != NULL && DB_IS_PARTITIONED(dbp) &&
	    F_ISSET((DB_PARTITION *)dbp->p_internal, PART_RANGE)) {
		DBT *keys;
		u_int32_t i;

		if ((ret = __partition_get_keys(dbp, &tmp_u_int32, &keys)) != 0)
			goto err;
		if (tmp_u_int32 != 0) {
			snprintf(buf,
			     buflen, "nparts=%lu\n", (u_long)tmp_u_int32);
			if ((ret = callback(handle, buf)) != 0)
				goto err;
			for (i = 0; i < tmp_u_int32 - 1; i++)
			    if ((ret = __db_prdbt(&keys[i],
				pflag, " ", handle, callback, 0, 0, 0)) != 0)
					goto err;
		}
	}
#endif

	if (keyflag && (ret = callback(handle, "keys=1\n")) != 0)
		goto err;

	ret = callback(handle, "HEADER=END\n");

err:	if (using_vdp &&
	    (t_ret = __db_vrfy_putpageinfo(env, vdp, pip)) != 0 && ret == 0)
		ret = t_ret;
	if (buf != NULL)
		__os_free(env, buf);

	return (ret);
}

/*
 * __db_prfooter --
 *	Print the footer that marks the end of a DB dump.  This is trivial,
 *	but for consistency's sake we don't want to put its literal contents
 *	in multiple places.
 *
 * PUBLIC: int __db_prfooter __P((void *, int (*)(void *, const void *)));
 */
int
__db_prfooter(handle, callback)
	void *handle;
	int (*callback) __P((void *, const void *));
{
	return (callback(handle, "DATA=END\n"));
}

/*
 * __db_pr_callback --
 *	Callback function for using pr_* functions from C.
 *
 * PUBLIC: int  __db_pr_callback __P((void *, const void *));
 */
int
__db_pr_callback(handle, str_arg)
	void *handle;
	const void *str_arg;
{
	char *str;
	FILE *f;

	str = (char *)str_arg;
	f = (FILE *)handle;

	if (fprintf(f, "%s", str) != (int)strlen(str))
		return (EIO);

	return (0);
}

/*
 * __db_dbtype_to_string --
 *	Return the name of the database type.
 *
 * PUBLIC: const char * __db_dbtype_to_string __P((DBTYPE));
 */
const char *
__db_dbtype_to_string(type)
	DBTYPE type;
{
	switch (type) {
	case DB_BTREE:
		return ("btree");
	case DB_HASH:
		return ("hash");
	case DB_RECNO:
		return ("recno");
	case DB_QUEUE:
		return ("queue");
	case DB_HEAP:
		return ("heap");
	case DB_UNKNOWN:
	default:
		break;
	}
	return ("UNKNOWN TYPE");
}
