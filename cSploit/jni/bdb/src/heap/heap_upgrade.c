/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/blob.h"
#include "dbinc/db_page.h"
#include "dbinc/heap.h"
#include "dbinc/db_upgrade.h"

/*
 * __heap_60_heapmeta--
 *	Upgrade the version number.
 *
 * PUBLIC: int __heap_60_heapmeta
 * PUBLIC:      __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *));
 */
int
__heap_60_heapmeta(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	HEAPMETA *hmeta;

	COMPQUIET(flags, 0);
	COMPQUIET(real_name, NULL);
	COMPQUIET(fhp, NULL);
	COMPQUIET(dbp, NULL);
	hmeta = (HEAPMETA *)h;

	hmeta->dbmeta.version = 2;
	*dirtyp = 1;

	return (0);
}


/*
 * __heap_60_heap --
 *	Upgrade the blob records on the database heap pages.
 *
 * PUBLIC: int __heap_60_heap
 * PUBLIC:      __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *));
 */
int
__heap_60_heap(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	HEAPBLOBHDR60 hb60;
	HEAPBLOBHDR60P1 hb60p1;
	HEAPHDR *hdr;
	db_seq_t blob_id, blob_size, file_id;
	db_indx_t indx, *offtbl;
	int ret;

	COMPQUIET(flags, 0);
	COMPQUIET(real_name, NULL);
	COMPQUIET(fhp, NULL);
	offtbl = (db_indx_t *)HEAP_OFFSETTBL(dbp, h);
	ret = 0;

	DB_ASSERT(dbp->env, HEAPBLOBREC60_SIZE == HEAPBLOBREC_SIZE);
	for (indx = 0; indx <= HEAP_HIGHINDX(h); indx++) {
		if (offtbl[indx] == 0)
			continue;
		hdr = (HEAPHDR *)P_ENTRY(dbp, h, indx);
		if (F_ISSET(hdr, HEAP_RECBLOB)) {
			memcpy(&hb60, hdr, HEAPBLOBREC60_SIZE);
			memset(&hb60p1, 0, HEAPBLOBREC_SIZE);
			hb60p1.std_hdr.flags = hb60.flags;
			hb60p1.std_hdr.size = hb60.size;
			hb60p1.encoding = hb60.encoding;
			hb60p1.lsn = hb60.lsn;
			GET_BLOB60_ID(dbp->env, hb60, blob_id, ret);
			if (ret != 0)
				return (ret);
			GET_BLOB60_SIZE(dbp->env, hb60, blob_size, ret);
			if (ret != 0)
				return (ret);
			GET_BLOB60_FILE_ID(dbp->env, &hb60, file_id, ret);
			if (ret != 0)
				return (ret);
			SET_BLOB_ID(&hb60p1, blob_id, HEAPBLOBHDR60P1);
			SET_BLOB_SIZE(&hb60p1, blob_size, HEAPBLOBHDR60P1);
			SET_BLOB_FILE_ID(&hb60p1, file_id, HEAPBLOBHDR60P1);
			memcpy(hdr, &hb60p1, HEAPBLOBREC_SIZE);
			*dirtyp = 1;
		}
	}

	return (ret);
}
