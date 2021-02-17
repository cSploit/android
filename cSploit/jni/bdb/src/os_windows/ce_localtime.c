/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2012, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __ce_localtime --
 *	localtime implementation on WinCE.
 *
 * PUBLIC: #ifdef DB_WINCE
 * PUBLIC: struct tm * localtime __P((const time_t *));
 * PUBLIC: #endif
 */
struct tm *
localtime(t)
	const time_t *t;
{
	static struct tm y;
	FILETIME uTm, lTm;
	SYSTEMTIME pTm;
	int64_t t64;

	t64 = *t;
	t64 = (t64 + 11644473600)*10000000;
	uTm.dwLowDateTime = (DWORD)(t64 & 0xFFFFFFFF);
	uTm.dwHighDateTime= (DWORD)(t64 >> 32);
	FileTimeToLocalFileTime(&uTm,&lTm);
	FileTimeToSystemTime(&lTm,&pTm);
	y.tm_year = pTm.wYear - 1900;
	y.tm_mon = pTm.wMonth - 1;
	y.tm_wday = pTm.wDayOfWeek;
	y.tm_mday = pTm.wDay;
	y.tm_hour = pTm.wHour;
	y.tm_min = pTm.wMinute;
	y.tm_sec = pTm.wSecond;
	return &y;
}
