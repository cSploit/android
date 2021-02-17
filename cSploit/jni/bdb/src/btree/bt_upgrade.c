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
#include "dbinc/db_upgrade.h"
#include "dbinc/btree.h"

/*
 * __bam_30_btreemeta --
 *	Upgrade the metadata pages from version 6 to version 7.
 *
 * PUBLIC: int __bam_30_btreemeta __P((DB *, char *, u_int8_t *));
 */
int
__bam_30_btreemeta(dbp, real_name, buf)
	DB *dbp;
	char *real_name;
	u_int8_t *buf;
{
	BTMETA2X *oldmeta;
	BTMETA30 *newmeta;
	ENV *env;
	int ret;

	env = dbp->env;

	newmeta = (BTMETA30 *)buf;
	oldmeta = (BTMETA2X *)buf;

	/*
	 * Move things from the end up, so we do not overwrite things.
	 * We are going to create a new uid, so we can move the stuff
	 * at the end of the structure first, overwriting the uid.
	 */

	newmeta->re_pad = oldmeta->re_pad;
	newmeta->re_len = oldmeta->re_len;
	newmeta->minkey = oldmeta->minkey;
	newmeta->maxkey = oldmeta->maxkey;
	newmeta->dbmeta.free = oldmeta->free;
	newmeta->dbmeta.flags = oldmeta->flags;
	newmeta->dbmeta.type  = P_BTREEMETA;

	newmeta->dbmeta.version = 7;
	/* Replace the unique ID. */
	if ((ret = __os_fileid(env, real_name, 1, buf + 36)) != 0)
		return (ret);

	newmeta->root = 1;

	return (0);
}

/*
 * __bam_31_btreemeta --
 *	Upgrade the database from version 7 to version 8.
 *
 * PUBLIC: int __bam_31_btreemeta
 * PUBLIC:      __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *));
 */
int
__bam_31_btreemeta(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	BTMETA30 *oldmeta;
	BTMETA31 *newmeta;

	COMPQUIET(dbp, NULL);
	COMPQUIET(real_name, NULL);
	COMPQUIET(fhp, NULL);

	newmeta = (BTMETA31 *)h;
	oldmeta = (BTMETA30 *)h;

	/*
	 * Copy the effected fields down the page.
	 * The fields may overlap each other so we
	 * start at the bottom and use memmove.
	 */
	newmeta->root = oldmeta->root;
	newmeta->re_pad = oldmeta->re_pad;
	newmeta->re_len = oldmeta->re_len;
	newmeta->minkey = oldmeta->minkey;
	newmeta->maxkey = oldmeta->maxkey;
	memmove(newmeta->dbmeta.uid,
	    oldmeta->dbmeta.uid, sizeof(oldmeta->dbmeta.uid));
	newmeta->dbmeta.flags = oldmeta->dbmeta.flags;
	newmeta->dbmeta.record_count = 0;
	newmeta->dbmeta.key_count = 0;
	ZERO_LSN(newmeta->dbmeta.unused3);

	/* Set the version number. */
	newmeta->dbmeta.version = 8;

	/* Upgrade the flags. */
	if (LF_ISSET(DB_DUPSORT))
		F_SET(&newmeta->dbmeta, BTM_DUPSORT);

	*dirtyp = 1;
	return (0);
}

/*
 * __bam_31_lbtree --
 *	Upgrade the database btree leaf pages.
 *
 * PUBLIC: int __bam_31_lbtree
 * PUBLIC:      __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *));
 */
int
__bam_31_lbtree(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	BKEYDATA *bk;
	db_pgno_t pgno;
	db_indx_t indx;
	int ret;

	ret = 0;
	for (indx = O_INDX; indx < NUM_ENT(h); indx += P_INDX) {
		bk = GET_BKEYDATA(dbp, h, indx);
		if (B_TYPE(bk->type) == B_DUPLICATE) {
			pgno = GET_BOVERFLOW(dbp, h, indx)->pgno;
			if ((ret = __db_31_offdup(dbp, real_name, fhp,
			    LF_ISSET(DB_DUPSORT) ? 1 : 0, &pgno)) != 0)
				break;
			if (pgno != GET_BOVERFLOW(dbp, h, indx)->pgno) {
				*dirtyp = 1;
				GET_BOVERFLOW(dbp, h, indx)->pgno = pgno;
			}
		}
	}

	return (ret);
}

/*
 * __bam_60_btreemeta--
 *	Upgrade the version number.
 *
 * PUBLIC: int __bam_60_btreemeta
 * PUBLIC:      __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *));
 */
int
__bam_60_btreemeta(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	BTMETA33 *bmeta;

	COMPQUIET(flags, 0);
	COMPQUIET(real_name, NULL);
	COMPQUIET(fhp, NULL);
	COMPQUIET(dbp, NULL);
	bmeta = (BTMETA33 *)h;

	bmeta->dbmeta.version = 10;
	*dirtyp = 1;

	return (0);
}

/*
 * __bam_60_lbtree --
 *	Upgrade the blob records on the database btree leaf pages.
 *
 * PUBLIC: int __bam_60_lbtree
 * PUBLIC:      __P((DB *, char *, u_int32_t, DB_FH *, PAGE *, int *));
 */
int
__bam_60_lbtree(dbp, real_name, flags, fhp, h, dirtyp)
	DB *dbp;
	char *real_name;
	u_int32_t flags;
	DB_FH *fhp;
	PAGE *h;
	int *dirtyp;
{
	BBLOB60 bl60;
	BBLOB60P1 bl60p1;
	BKEYDATA *bk;
	db_seq_t blob_id, blob_size, file_id, sdb_id;
	db_indx_t indx;
	int ret;

	COMPQUIET(flags, 0);
	COMPQUIET(real_name, NULL);
	COMPQUIET(fhp, NULL);
	ret = 0;

	DB_ASSERT(dbp->env, BBLOB60_SIZE == BBLOB_SIZE);
	for (indx = O_INDX; indx < NUM_ENT(h); indx += P_INDX) {
		bk = GET_BKEYDATA(dbp, h, indx);
		if (B_TYPE(bk->type) == B_BLOB ) {
			memcpy(&bl60, bk, BBLOB60_SIZE);
			memset(&bl60p1, 0, BBLOB_SIZE);
			bl60p1.type = bl60.type;
			bl60p1.len = BBLOB_DSIZE;
			bl60p1.encoding = bl60.encoding;
			GET_BLOB60_ID(dbp->env, bl60, blob_id, ret);
			if (ret != 0)
				return (ret);
			GET_BLOB60_SIZE(dbp->env, bl60, blob_size, ret);
			if (ret != 0)
				return (ret);
			GET_BLOB60_FILE_ID(dbp->env, &bl60, file_id, ret);
			if (ret != 0)
				return (ret);
			GET_BLOB60_SDB_ID(dbp->env, &bl60, sdb_id, ret);
			if (ret != 0)
				return (ret);
			SET_BLOB_ID(&bl60p1, blob_id, BBLOB60P1);
			SET_BLOB_SIZE(&bl60p1, blob_size, BBLOB60P1);
			SET_BLOB_FILE_ID(&bl60p1, file_id, BBLOB60P1);
			SET_BLOB_SDB_ID(&bl60p1, sdb_id, BBLOB60P1);
			memcpy(bk, &bl60p1, BBLOB_SIZE);
			*dirtyp = 1;
		}
	}

	return (ret);
}
