/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Olson.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"

/*
 * __bam_cmp --
 *	Compare a key to a given record. We always start the comparison
 *	at an offset and update the offset with longest matching count
 *	after the comparison.
 *
 * PUBLIC: int __bam_cmp __P((DBC *, const DBT *, PAGE *, u_int32_t,
 * PUBLIC:    int (*)(DB *, const DBT *, const DBT *, size_t *),
 * PUBLIC:    int *, size_t *));
 */
int
__bam_cmp(dbc, dbt, h, indx, func, cmpp, locp)
	DBC *dbc;
	const DBT *dbt;
	PAGE *h;
	u_int32_t indx;
	int (*func)__P((DB *, const DBT *, const DBT *, size_t *));
	int *cmpp;
	size_t *locp;
{
	BBLOB bl;
	BINTERNAL *bi;
	BKEYDATA *bk;
	BOVERFLOW *bo;
	DB *dbp;
	DBT pg_dbt;
	off_t blob_size;
	int ret;
	db_seq_t blob_id;

	dbp = dbc->dbp;
	ret = 0;

	/* Assert that the func is non-Null. */
	DB_ASSERT(dbp->env, func != NULL);

	/*
	 * Returns:
	 *	< 0 if dbt is < page record
	 *	= 0 if dbt is = page record
	 *	> 0 if dbt is > page record
	 *
	 * !!!
	 * We do not clear the pg_dbt DBT even though it's likely to contain
	 * random bits.  That should be okay, because the app's comparison
	 * routine had better not be looking at fields other than data, size
	 * and app_data.  We don't clear it because we go through this path a
	 * lot and it's expensive.
	 */
	switch (TYPE(h)) {
	case P_LBTREE:
	case P_LDUP:
	case P_LRECNO:
		bk = GET_BKEYDATA(dbp, h, indx);
		if (B_TYPE(bk->type) == B_OVERFLOW)
			bo = (BOVERFLOW *)bk;
		else if (B_TYPE(bk->type) == B_BLOB) {
			/*
			 * This is very slow, but since blobs cannot be
			 * in databases with duplicates or be keys, it should
			 * only happen when using DB_GET_BOTH or DB_SET.
			 */
			memcpy(&bl, bk, BBLOB_SIZE);
			memset(&pg_dbt, 0, sizeof(DBT));
			GET_BLOB_SIZE(dbc->env, bl, blob_size, ret);
			if (ret != 0)
				return (ret);
			if (blob_size > UINT32_MAX)
				pg_dbt.size = UINT32_MAX;
			else
				pg_dbt.size = (u_int32_t)blob_size;
			blob_id = (db_seq_t)bl.id;
			pg_dbt.flags = DB_DBT_USERMEM;
			if ((ret = __os_malloc(
			    dbc->env, pg_dbt.size, &pg_dbt.data)) != 0)
				return (ret);
			pg_dbt.ulen = pg_dbt.size;
			if ((ret = __blob_get(dbc,
			    &pg_dbt, blob_id, blob_size, NULL, NULL)) != 0) {
				__os_free(dbc->env, pg_dbt.data);
				return (ret);
			}
			*cmpp = func(dbp, dbt, &pg_dbt, locp);
			/*
			 * There is no way to directly compare a blob file that
			 * is greater in size than UINT32_MAX, so instead we
			 * compare the data up to UINT32_MAX, and if they are
			 * equal return that the blob is larger, since it is
			 * longer than the input data.
			 */
			if (*cmpp == 0 && (blob_size > UINT32_MAX))
				*cmpp = -1;
			__os_free(dbc->env, pg_dbt.data);
			return (0);
		} else {
			pg_dbt.app_data = NULL;
			pg_dbt.data = bk->data;
			pg_dbt.size = bk->len;
			*cmpp = func(dbp, dbt, &pg_dbt, locp);
			return (0);
		}
		break;
	case P_IBTREE:
		/*
		 * The following code guarantees that the left-most key on an
		 * internal page at any place in the tree sorts less than any
		 * user-specified key.  The reason is that if we have reached
		 * this internal page, we know the user key must sort greater
		 * than the key we're storing for this page in any internal
		 * pages at levels above us in the tree.  It then follows that
		 * any user-specified key cannot sort less than the first page
		 * which we reference, and so there's no reason to call the
		 * comparison routine.  While this may save us a comparison
		 * routine call or two, the real reason for this is because
		 * we don't maintain a copy of the smallest key in the tree,
		 * so that we don't have to update all the levels of the tree
		 * should the application store a new smallest key.  And, so,
		 * we may not have a key to compare, which makes doing the
		 * comparison difficult and error prone.
		 */
		if (indx == 0) {
			*cmpp = 1;
			return (0);
		}

		bi = GET_BINTERNAL(dbp, h, indx);
		if (B_TYPE(bi->type) == B_OVERFLOW) {
			DB_ASSERT(dbp->env, bi->len == BOVERFLOW_SIZE);
			bo = (BOVERFLOW *)(bi->data);
		} else {
			pg_dbt.app_data = NULL;
			pg_dbt.data = bi->data;
			pg_dbt.size = bi->len;
			*cmpp = func(dbp, dbt, &pg_dbt, locp);
			return (0);
		}
		break;
	default:
		return (__db_pgfmt(dbp->env, PGNO(h)));
	}

	/*
	 * Overflow.
	 */
	return (__db_moff(dbc, dbt, bo->pgno, bo->tlen,
		    func == __bam_defcmp ? NULL : func, cmpp, locp));
}

/*
 * __bam_defcmp --
 *	Keep track of how far along in the two keys we find matching
 *	characters, and use that as an offset into the keys to begin
 *	future comparisons. This will save us the overhead of always
 *	starting the comparisons on the first character.
 *
 * PUBLIC: int __bam_defcmp __P((DB *, const DBT *, const DBT *, size_t *));
 */
int
__bam_defcmp(dbp, a, b, locp)
	DB *dbp;
	const DBT *a, *b;
	size_t *locp;
{
	size_t len, i, start;
	u_int8_t *p1, *p2;

	COMPQUIET(dbp, NULL);
	start = (locp == NULL ? 0 : *locp);
	/*
	 * Returns:
	 *	< 0 if a is < b
	 *	= 0 if a is = b
	 *	> 0 if a is > b
	 *
	 * We start the comparison from 'locp' and store the last match
	 * location in 'locp'.
	 *
	 * XXX
	 * If a size_t doesn't fit into a long, or if the difference between
	 * any two characters doesn't fit into an int, this routine can lose.
	 * What we need is a signed integral type that's guaranteed to be at
	 * least as large as a size_t, and there is no such thing.
	 */
	p1 = (u_int8_t *)a->data + start;
	p2 = (u_int8_t *)b->data + start;
	len = a->size > b->size ? b->size : a->size;
	for (i = start; i < len; ++p1, ++p2, ++i)
		if (*p1 != *p2) {
			if (locp != NULL)
				*locp = i;
			return (*p1 < *p2 ? -1 : 1);
		}
	if (locp != NULL)
		*locp = len;
	return (a->size == b->size ? 0 : (a->size < b->size ? -1 : 1));
}

/*
 * __bam_defpfx --
 *	Default prefix routine.
 *
 * PUBLIC: size_t __bam_defpfx __P((DB *, const DBT *, const DBT *));
 */
size_t
__bam_defpfx(dbp, a, b)
	DB *dbp;
	const DBT *a, *b;
{
	size_t cnt, len;
	u_int8_t *p1, *p2;

	COMPQUIET(dbp, NULL);

	cnt = 1;
	len = a->size > b->size ? b->size : a->size;
	for (p1 = a->data, p2 = b->data; len--; ++p1, ++p2, ++cnt)
		if (*p1 != *p2)
			return (cnt);

	/*
	 * They match up to the smaller of the two sizes.
	 * Collate the longer after the shorter.
	 */
	if (a->size < b->size)
		return (a->size + 1);
	if (b->size < a->size)
		return (b->size + 1);
	return (b->size);
}
