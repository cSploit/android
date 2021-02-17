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
 * __ce_gmtime --
 *	gmtime implementation on WinCE.
 *
 * PUBLIC: #ifdef DB_WINCE
 * PUBLIC: struct tm * __ce_gmtime __P((const time_t *));
 * PUBLIC: #endif
 */

struct tm *
__ce_gmtime(timer)
	const time_t *timer;
{
	static struct tm br_time;
	struct tm *timep;
	time_t ti;
	unsigned long dayclock, dayno;
	int year;

	timep = &br_time;
	ti = *timer;
	dayclock = (unsigned long)ti % SECSPERDAY;
	dayno = (unsigned long)ti / SECSPERDAY;
	year = TM_YEAR_EPOCH;

	timep->tm_sec = dayclock % 60;
	timep->tm_min = (dayclock % 3600) / 60;
	timep->tm_hour = dayclock / 3600;
	/* day 0 was a thursday */
	timep->tm_wday = (dayno + 4) % 7;
	while (dayno >= year_lengths[isleap(year)]) {
		dayno -= year_lengths[isleap(year)];
		year++;
	}
	timep->tm_year = year - TM_YEAR_BASE;
	timep->tm_yday = dayno;
	timep->tm_mon = 0;
	while (dayno >= mon_lengths[isleap(year)][timep->tm_mon]) {
		dayno -= mon_lengths[isleap(year)][timep->tm_mon];
		timep->tm_mon++;
	}
	timep->tm_mday = dayno + 1;
	timep->tm_isdst = 0;

	return timep;
}
