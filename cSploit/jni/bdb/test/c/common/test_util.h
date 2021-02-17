/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

/*
 * A header file that defines things that are common to many C test cases.
 */

#include "db_config.h"

#include "db_int.h"
#include <stdio.h>

#define TEST_ENV "TESTDIR"
#define TEST_ENVx(x) "TESTDIR#x"

static int teardown_envdir(const char *dir);

static int setup_envdir(const char *dir, u_int32_t remove)
{
	int ret;

	if (remove && (ret = teardown_envdir(dir)) != 0)
		return (ret);
	return (__os_mkdir(NULL, dir, 0755));
}

/*
 * This might seem complicated, but it works on all platforms.
 */
static int teardown_envdir(const char *dir)
{
	int cnt, i, isdir, ret;
	char buf[1024], **names;

	ret = 0;

	/* If the directory doesn't exist, we're done. */
	if (__os_exists(NULL, dir, &isdir) != 0)
		return (0);

	/* Get a list of the directory contents. */
	if ((ret = __os_dirlist(NULL, dir, 1, &names, &cnt)) != 0)
		return (ret);

	/* Go through the file name list, remove each file in the list */
	for (i = 0; i < cnt; ++i) {
		(void)snprintf(buf, sizeof(buf),
		    "%s%c%s", dir, PATH_SEPARATOR[0], names[i]);
		if ((ret = __os_exists(NULL, buf, &isdir)) != 0)
			goto file_err;
		if (isdir)
			teardown_envdir(buf);
		if (!isdir && (ret = __os_unlink(NULL, buf, 0)) != 0) {
file_err:		fprintf(stderr, "%s: %s\n",
			    buf, db_strerror(ret));
			break;
		}
	}

	__os_dirfree(NULL, names, cnt);

	/*
	 * If we removed the contents of the directory, remove the directory
	 * itself.
	 */
	if (i == cnt && (ret = rmdir(dir)) != 0)
		fprintf(stderr,
		    "%s: %s\n", dir, db_strerror(errno));
	return (ret);
}

