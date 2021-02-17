/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2014 Oracle and/or its affiliates.  All rights reserved.
 */
/*
 * $Id$
 */
#ifndef	_DB_PART_H_
#define	_DB_PART_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct __db_partition {
	u_int32_t	nparts;		/* number of partitions. */
	DBT		*keys;		/* array of range keys. */
	void		*data;		/* the partition info. */
	const char	**dirs;		/* locations for partitions. */
	DB		**handles;	/* array of partition handles. */
	u_int32_t	(*callback) (DB *, DBT *);
#define	PART_CALLBACK	0x01
#define	PART_RANGE	0x02
#define	PART_KEYS_SETUP	0x04
	u_int32_t	flags;
} DB_PARTITION;

/*
 * Internal part of a partitioned cursor.
 */
typedef struct __part_internal {
	__DBC_INTERNAL
	u_int32_t	part_id;
	DBC		*sub_cursor;
} PART_CURSOR;

#ifdef HAVE_PARTITION
#define	PART_NAME	"__dbp.%s.%03d"
/*
 * Currently we only support no more than 1000000 partitions.
 * If the limit is changed, the PART_DIGITS and PART_MAXIMUM
 * should be changed accordingly.
 */
#define	PART_DIGITS	6
#define	PART_MAXIMUM	1000000
#define	PART_LEN	(sizeof("__dbp..") + PART_DIGITS)
#define	PART_PREFIX	"__dbp."
#define IS_PARTITION_DB_FILE(name)	(strncmp(name, PART_PREFIX,	\
					    sizeof(PART_PREFIX) - 1) == 0)

#define	DB_IS_PARTITIONED(dbp)						\
      (dbp->p_internal != NULL &&					\
      ((DB_PARTITION *)dbp->p_internal)->handles != NULL)

#define	DBC_PART_REFRESH(dbc)	(F_SET(dbc, DBC_PARTITIONED))
#else
#define	DBC_PART_REFRESH(dbc)
#define	DB_IS_PARTITIONED(dbp)	(0)
#endif

#if defined(__cplusplus)
}
#endif
#endif
