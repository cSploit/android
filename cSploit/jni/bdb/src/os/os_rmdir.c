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
 * __os_rmdir --
 *	Remove a directory.
 *
 * PUBLIC: int __os_rmdir __P((ENV *, const char *));
 */
int
__os_rmdir(env, name)
	ENV *env;
	const char *name;
{
	DB_ENV *dbenv;
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;
	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, DB_STR_A("0239", "fileops: rmdir %s",
		    "%s"), name);

	RETRY_CHK((rmdir(name)), ret);
	if (ret != 0)
		return (__os_posix_err(ret));

	return (ret);
}
