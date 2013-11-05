/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		timestamp.cpp
 *	DESCRIPTION:	Date/time handling class
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * NS: The code now contains much of the logic from original gds.c
 *     this is why we need to use IPL license for it
 */

#include "firebird.h"
#include "../jrd/common.h"
#include "../jrd/gdsassert.h"

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#ifdef HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
#endif

#include "../common/classes/timestamp.h"

namespace Firebird {

void TimeStamp::report_error(const char* msg)
{
#ifndef SUPERCLIENT
	system_call_failed::raise(msg);
#endif
}

TimeStamp TimeStamp::getCurrentTimeStamp()
{
	TimeStamp result;

	// NS: We round generated timestamps to whole millisecond.
	// Not many applications can deal with fractional milliseconds properly and
	// we do not use high resolution timers either so actual time granularity
	// is going to to be somewhere in range between 1 ms (like on UNIX/Risc)
	// and 53 ms (such as Win9X)

	time_t seconds; // UTC time
	int milliseconds;

#ifdef HAVE_GETTIMEOFDAY
	struct timeval tp;
	GETTIMEOFDAY(&tp);
	seconds = tp.tv_sec;
	milliseconds = tp.tv_usec / 1000;
#else
	struct timeb time_buffer;
	ftime(&time_buffer);
	seconds = time_buffer.time;
	milliseconds = time_buffer.millitm;
#endif

	// NS: Current FB behavior of using server time zone is not appropriate for
	// distributed applications. We should be storing UTC times everywhere and
	// convert timestamps to client timezone as necessary. Replace localtime stuff
	// with these lines as soon as the appropriate functionality is implemented
	//
	// mValue.timestamp_date = seconds / 86400 + GDS_EPOCH_START;
	// mValue.timestamp_time = (seconds % 86400) * ISC_TIME_SECONDS_PRECISION;

	const int fractions = milliseconds * ISC_TIME_SECONDS_PRECISION / 1000;

#ifdef HAVE_LOCALTIME_R
	struct tm times;
	if (!localtime_r(&seconds, &times))
	{
		report_error("localtime_r");
		return result;
	}

	result.encode(&times, fractions);
#else
	struct tm *times = localtime(&seconds);
	if (!times)
	{
		report_error("localtime");
		return result;
	}

	result.encode(times, fractions);
#endif

	return result;
}

int TimeStamp::yday(const struct tm* times)
{
	// Convert a calendar date to the day-of-year.
	//
	// The unix time structure considers January 1 to be Year day 0, although it
	// is day 1 of the month. (Note that QLI, when printing Year days takes the other view).

	int day = times->tm_mday;
	const int month = times->tm_mon;
	const int year = times->tm_year + 1900;

	--day;

	day += (214 * month + 3) / 7;

	if (month < 2)
		return day;

	if (year % 4 == 0 && year % 100 != 0 || year % 400 == 0)
		--day;
	else
		day -= 2;

	return day;
}


void TimeStamp::decode_date(ISC_DATE nday, struct tm* times)
{
	// Convert a numeric day to [day, month, year].
	//
	// Calenders are divided into 4 year cycles: 3 non-leap years, and 1 leap year.
	// Each cycle takes 365*4 + 1 == 1461 days.
	// There is a further cycle of 100 4 year cycles.
	// Every 100 years, the normally expected leap year is not present. Every 400 years it is.
	// This cycle takes 100 * 1461 - 3 == 146097 days.
	// The origin of the constant 2400001 is unknown.
	// The origin of the constant 1721119 is unknown.
	// The difference between 2400001 and 1721119 is the
	// number of days from 0/0/0000 to our base date of 11/xx/1858 (678882)
	// The origin of the constant 153 is unknown.
	//
	// This whole routine has problems with ndates less than -678882 (Approx 2/1/0000).

	// struct tm may include arbitrary number of additional members.
	// zero-initialize them.
	memset(times, 0, sizeof(struct tm));

	if ((times->tm_wday = (nday + 3) % 7) < 0)
		times->tm_wday += 7;

	nday += 2400001 - 1721119;
	const int century = (4 * nday - 1) / 146097;
	nday = 4 * nday - 1 - 146097 * century;
	int day = nday / 4;

	nday = (4 * day + 3) / 1461;
	day = 4 * day + 3 - 1461 * nday;
	day = (day + 4) / 4;

	int month = (5 * day - 3) / 153;
	day = 5 * day - 3 - 153 * month;
	day = (day + 5) / 5;

	int year = 100 * century + nday;

	if (month < 10)
		month += 3;
	else {
		month -= 9;
		year += 1;
	}

	times->tm_mday = day;
	times->tm_mon = month - 1;
	times->tm_year = year - 1900;

	times->tm_yday = yday(times);
}


ISC_DATE TimeStamp::encode_date(const struct tm* times)
{
	// Convert a calendar date to a numeric day
	// (the number of days since the base date)

	const int day = times->tm_mday;
	int month = times->tm_mon + 1;
	int year = times->tm_year + 1900;

	if (month > 2)
		month -= 3;
	else {
		month += 9;
		year -= 1;
	}

	const int c = year / 100;
	const int ya = year - 100 * c;

	return (ISC_DATE) (((SINT64) 146097 * c) / 4 +
					   (1461 * ya) / 4 +
					   (153 * month + 2) / 5 + day + 1721119 - 2400001);
}

void TimeStamp::decode_time(ISC_TIME ntime, int* hours, int* minutes, int* seconds, int* fractions)
{
	fb_assert(hours);
	fb_assert(minutes);
	fb_assert(seconds);

	*hours = ntime / (3600 * ISC_TIME_SECONDS_PRECISION);
	ntime %= 3600 * ISC_TIME_SECONDS_PRECISION;
	*minutes = ntime / (60 * ISC_TIME_SECONDS_PRECISION);
	ntime %= 60 * ISC_TIME_SECONDS_PRECISION;
	*seconds = ntime / ISC_TIME_SECONDS_PRECISION;

	if (fractions)
	{
		*fractions = ntime % ISC_TIME_SECONDS_PRECISION;
	}
}

ISC_TIME TimeStamp::encode_time(int hours, int minutes, int seconds, int fractions)
{
	fb_assert(fractions	>= 0 && fractions < ISC_TIME_SECONDS_PRECISION);

	return ((hours * 60 + minutes) * 60 + seconds) * ISC_TIME_SECONDS_PRECISION + fractions;
}

void TimeStamp::decode_timestamp(const ISC_TIMESTAMP ts, struct tm* times, int* fractions)
{
	decode_date(ts.timestamp_date, times);
	decode_time(ts.timestamp_time, &times->tm_hour, &times->tm_min, &times->tm_sec, fractions);
}

ISC_TIMESTAMP TimeStamp::encode_timestamp(const struct tm* times, const int fractions)
{
	fb_assert(fractions >= 0 && fractions < ISC_TIME_SECONDS_PRECISION);

	ISC_TIMESTAMP ts;
	ts.timestamp_date = encode_date(times);
	ts.timestamp_time = encode_time(times->tm_hour, times->tm_min, times->tm_sec, fractions);

	return ts;
}

void TimeStamp::round_time(ISC_TIME &ntime, const int precision)
{
	const int scale = -ISC_TIME_SECONDS_PRECISION_SCALE - precision;

	// for the moment, if greater precision was requested than we can
	// provide return what we have.
	if (scale <= 0)
		return;

	static const ISC_TIME pow10table[] =
		{1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

	fb_assert(scale < FB_NELEM(pow10table));

	const ISC_TIME period = pow10table[scale];

	ntime -= (ntime % period);
}

// Encode timestamp from UNIX datetime structure
void TimeStamp::encode(const struct tm* times, int fractions)
{
	mValue = encode_timestamp(times, fractions);
}

// Decode timestamp into UNIX datetime structure
void TimeStamp::decode(struct tm* times, int* fractions) const
{
	fb_assert(mValue.timestamp_date != BAD_DATE);
	fb_assert(mValue.timestamp_time != BAD_TIME);

	decode_timestamp(mValue, times, fractions);
}

} // namespace
