/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2013, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#ifndef	_DB_BLOB_H_
#define	_DB_BLOB_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * How many characters can the path for a blob file use?
 * Up to 6 subdirectory separators.
 * Up to 6 directory names of up to three characters each.
 * Up to 21 characters for blob_id identifier.
 * 7 characters for the standard prefix (__db.bl)
 * 1 for luck (or NULL)
 * The largest blob id, 9,223,372,036,854,775,807 would
 * produce a path and file name:
 * 009/223/372/036/854/775/807/__db.bl009223372036854775807
 */
#define MAX_BLOB_PATH	"009/223/372/036/854/775/807/__db.bl009223372036854775807"
#define	MAX_BLOB_PATH_SZ	sizeof(MAX_BLOB_PATH)
#define	BLOB_DEFAULT_DIR	"__db_bl"
#define	BLOB_META_FILE_NAME	"__db_blob_meta.db" 
#define	BLOB_DIR_PREFIX		"__db"
#define	BLOB_FILE_PREFIX	"__db.bl"

#define	BLOB_DIR_ELEMS		1000

/*
 * Combines two unsigned 32 bit integers into a 64 bit integer.
 * Blob database file ids and sub database ids are 64 bit integers,
 * but have to be stored on database metadata pages that must
 * be readable on 32 bit only compilers.  So the ids are split into
 * two 32 bit integers, and combined when needed.
 */
#define GET_LO_HI(e, lo, hi, o, ret)	do {				\
	DB_ASSERT((e), sizeof(o) <= 8);					\
	if (sizeof(o) == 8) {						\
		(o) = (hi);						\
		(o) = ((o) << 32);					\
		(o) += (lo);						\
	} else {							\
		if ((hi) > 0) {						\
			__db_errx((e), DB_STR("0765",			\
			    "Offset or id size overflow."));		\
			(ret) = EINVAL;					\
		}							\
		(o) = (lo);						\
	}								\
} while (0);

#define GET_BLOB_FILE_ID(e, p, o, ret)					\
	GET_LO_HI(e, (p)->blob_file_lo, (p)->blob_file_hi, o, ret);

#define GET_BLOB_SDB_ID(e, p, o, ret)					\
	GET_LO_HI(e, (p)->blob_sdb_lo, (p)->blob_sdb_hi, o, ret);

/* Splits a 64 bit integer into two unsigned 32 bit integers. */
#define SET_LO_HI(p, v, type, field_lo, field_hi)	do {		\
	u_int32_t tmp;							\
	if (sizeof((v)) == 8) {						\
		tmp = (u_int32_t)((v) >> 32);				\
		memcpy(((u_int8_t *)p) + SSZ(type, field_hi),		\
		    &tmp, sizeof(u_int32_t));				\
	} else {							\
		memset(((u_int8_t *)p) + SSZ(type, field_hi),		\
		    0, sizeof(u_int32_t));				\
	}								\
	tmp = (u_int32_t)(v);						\
	memcpy(((u_int8_t *)p) + SSZ(type, field_lo),			\
	    &tmp, sizeof(u_int32_t));					\
} while (0);

#define SET_BLOB_META_FILE_ID(p, v, type)					\
	SET_LO_HI(p, v, type, blob_file_lo, blob_file_hi);

#define SET_BLOB_META_SDB_ID(p, v, type)					\
	SET_LO_HI(p, v, type, blob_sdb_lo, blob_sdb_hi);

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_BLOB_H_ */
