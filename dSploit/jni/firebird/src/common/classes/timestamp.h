/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		timestamp.h
 *	DESCRIPTION:	Date/time handling class
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Dmitry Yemanov <dimitr@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef CLASSES_TIMESTAMP_H
#define CLASSES_TIMESTAMP_H

#include "../jrd/dsc.h"

// struct tm declaration
#if defined(TIME_WITH_SYS_TIME)
#include <sys/time.h>
#include <time.h>
#else
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

namespace Firebird {

// Wrapper class for ISC_TIMESTAMP supposed to implement date/time conversions
// and arithmetic. Small and not platform-specific methods are implemented
// inline. Usage of this class normally should involve zero overhead.
//
// Note: default "shallow-copy" constructor and assignment operators
// are just fine for our purposes

class TimeStamp
{
	static const ISC_DATE MIN_DATE = -678575;	// 01.01.0001
	static const ISC_DATE MAX_DATE = 2973483;	// 31.12.9999

	static const ISC_DATE BAD_DATE = MAX_SLONG;
	static const ISC_TIME BAD_TIME = MAX_ULONG;

public:
	// Number of the first day of UNIX epoch in GDS counting
	enum { GDS_EPOCH_START = 40617 };

	// Constructors
	TimeStamp()
	{
		invalidate();
	}

	TimeStamp(const ISC_TIMESTAMP& from)
		: mValue(from)
	{}

	TimeStamp(ISC_DATE date, ISC_TIME time)
	{
		mValue.timestamp_date = date;
		mValue.timestamp_time = time;
	}

	explicit TimeStamp(const struct tm& times, int fractions = 0)
	{
		encode(&times, fractions);
	}

	bool isValid() const
	{
		return isValidTimeStamp(mValue);
	}

	// Check if timestamp contains a non-existing value
	bool isEmpty() const
	{
		return (mValue.timestamp_date == BAD_DATE && mValue.timestamp_time == BAD_TIME);
	}

	// Set value of timestamp to a non-existing value
	void invalidate()
	{
		mValue.timestamp_date = BAD_DATE;
		mValue.timestamp_time = BAD_TIME;
	}

	// Assign current date/time to the timestamp
	void validate()
	{
		if (isEmpty())
		{
			*this = getCurrentTimeStamp();
		}
	}

	// Encode timestamp from UNIX datetime structure
	void encode(const struct tm* times, int fractions = 0);

	// Decode timestamp into UNIX datetime structure
	void decode(struct tm* times, int* fractions = NULL) const;

	// Write access to timestamp structure we wrap
	ISC_TIMESTAMP& value() { return mValue; }

	// Read access to timestamp structure we wrap
	const ISC_TIMESTAMP& value() const { return mValue; }

	// Return current timestamp value
	static TimeStamp getCurrentTimeStamp();

	// Validation routines
	static bool isValidDate(const ISC_DATE ndate)
	{
		return (ndate >= MIN_DATE && ndate <= MAX_DATE);
	}

	static bool isValidTime(const ISC_TIME ntime)
	{
		return (ntime < 24 * 3600 * ISC_TIME_SECONDS_PRECISION);
	}

	static bool isValidTimeStamp(const ISC_TIMESTAMP ts)
	{
		return (isValidDate(ts.timestamp_date) && isValidTime(ts.timestamp_time));
	}

	// ISC date/time helper routines
	static ISC_DATE encode_date(const struct tm* times);
	static ISC_TIME encode_time(int hours, int minutes, int seconds, int fractions = 0);
	static ISC_TIMESTAMP encode_timestamp(const struct tm* times, const int fractions = 0);

	static void decode_date(ISC_DATE nday, struct tm* times);
	static void decode_time(ISC_TIME ntime, int* hours, int* minutes, int* seconds, int* fractions = NULL);
	static void decode_timestamp(const ISC_TIMESTAMP ntimestamp, struct tm* times, int* fractions = NULL);

	static void round_time(ISC_TIME& ntime, const int precision);

	static inline bool isLeapYear(const int year)
	{
		return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
	}

private:
	ISC_TIMESTAMP mValue;

	static int yday(const struct tm* times);
	static void report_error(const char* msg);
};

}	// namespace Firebird

#endif // CLASSES_TIMESTAMP_H
