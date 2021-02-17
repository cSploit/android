/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __ce_freopen --
 *	Reopen a stream on WinCE.
 *
 * PUBLIC: #ifdef DB_WINCE
 * PUBLIC: FILE * __ce_freopen
 * PUBLIC:     __P((const char *, const char *, FILE *));
 * PUBLIC: #endif
 */
FILE *
__ce_freopen(path, mode, stream)
	const char *path, *mode;
	FILE *stream;
{
	size_t lenm, lenp;
	wchar_t *wpath, *wmode;
	FILE *handle;

	wpath = NULL;
	wmode = NULL;
	handle = NULL;
	lenp = strlen(path) + 1;
	lenm = strlen(mode) + 1;

	if (__os_malloc(NULL, lenp * sizeof(wchar_t), &wpath) != 0 ||
	    __os_malloc(NULL, lenm * sizeof(wchar_t), &wmode) != 0)
		goto err;

	if (mbstowcs(wpath, path, lenp) != lenp ||
	    mbstowcs(wmode, mode, lenm) != lenm)
		goto err;

	handle = _wfreopen(wpath, wmode, stream);
err:
	if (wpath != NULL)
		__os_free(NULL, wpath);
	if (wmode != NULL)
		__os_free(NULL, wmode);
	return handle;
}
