/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#ifndef	_DB_FOP_H_
#define	_DB_FOP_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define	MAKE_INMEM(D) do {					\
	F_SET((D), DB_AM_INMEM);				\
	(void)__memp_set_flags((D)->mpf, DB_MPOOL_NOFILE, 1);	\
} while (0)

#define	CLR_INMEM(D) do {					\
	F_CLR((D), DB_AM_INMEM);				\
	(void)__memp_set_flags((D)->mpf, DB_MPOOL_NOFILE, 0);	\
} while (0)

/*
 * Never change the value of DB_FOP_CREATE (0x00000002),
 * DB_FOP_APPEND (0x00000001), and DB_FOP_REDO(0x00000008),
 * as those values are used in write_file logs.
 */
#define	DB_FOP_APPEND		0x00000001 /* Appending to a file. */
#define	DB_FOP_CREATE		0x00000002 /* Creating the file. */
#define	DB_FOP_PARTIAL_LOG	0x00000004 /* Partial logging of file data. */
#define	DB_FOP_REDO		0x00000008 /* File operation can be redone. */
#define	DB_FOP_READONLY		0x00000010 /* File is read only. */
#define	DB_FOP_WRITE		0x00000020 /* File is writeable. */
#define	DB_FOP_SYNC_WRITE	0x00000040 /* Sync file on each write. */


#include "dbinc_auto/fileops_auto.h"
#include "dbinc_auto/fileops_ext.h"

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_FOP_H_ */
