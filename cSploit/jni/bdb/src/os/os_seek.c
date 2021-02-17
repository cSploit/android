/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_seek --
 *	Seek to a page/byte offset in the file.
 *
 * PUBLIC: int __os_seek __P((ENV *,
 * PUBLIC:      DB_FH *, db_pgno_t, u_int32_t, off_t));
 */
int
__os_seek(env, fhp, pgno, pgsize, relative)
	ENV *env;
	DB_FH *fhp;
	db_pgno_t pgno;
	u_int32_t pgsize;
	off_t relative;
{
	DB_ENV *dbenv;
	off_t offset;
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;

	DB_ASSERT(env, F_ISSET(fhp, DB_FH_OPENED) && fhp->fd != -1);

#if defined(HAVE_STATISTICS)
	++fhp->seek_count;
#endif

	offset = (off_t)pgsize * pgno + relative;

	if (dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0170",
		    "fileops: seek %s to %lu", "%s %lu"),
		    fhp->name, (u_long)offset);

	if (DB_GLOBAL(j_seek) != NULL)
		ret = DB_GLOBAL(j_seek)(fhp->fd, offset, SEEK_SET);
	else
		RETRY_CHK((lseek(
		    fhp->fd, offset, SEEK_SET) == -1 ? 1 : 0), ret);

	if (ret == 0) {
		fhp->pgsize = pgsize;
		fhp->pgno = pgno;
		fhp->offset = relative;
	} else {
		__db_syserr(env, ret, DB_STR_A("0171",
		    "seek: %lu: (%lu * %lu) + %lu", "%lu %lu %lu %lu"),
		    (u_long)offset, (u_long)pgno, (u_long)pgsize,
		    (u_long)relative);
		ret = __os_posix_err(ret);
	}

	return (ret);
}
