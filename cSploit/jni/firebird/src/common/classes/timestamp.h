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

#include "../common/classes/NoThrowTimeStamp.h"

namespace Firebird {

// Wrapper class for ISC_TIMESTAMP supposed to implement date/time conversions
// and arithmetic. Small and not platform-specific methods are implemented
// inline. Usage of this class normally should involve zero overhead except
// adding code for exceptions, memory allocation and all related when used in UDF.

class TimeStamp : public NoThrowTimeStamp
{
public:
	// Constructors
	TimeStamp()
		: NoThrowTimeStamp()
	{}

	TimeStamp(const ISC_TIMESTAMP& from)
		: NoThrowTimeStamp(from)
	{}

	TimeStamp(const NoThrowTimeStamp& from)
		: NoThrowTimeStamp(from)
	{}

	TimeStamp(ISC_DATE date, ISC_TIME time)
		: NoThrowTimeStamp(date, time)
	{}

	explicit TimeStamp(const struct tm& times, int fractions = 0)
		: NoThrowTimeStamp(times, fractions)
	{}

	// Return current timestamp value
	static TimeStamp getCurrentTimeStamp();

	// Assign current date/time to the timestamp
	void validate()
	{
		if (isEmpty())
		{
			*this = getCurrentTimeStamp();
		}
	}

private:
	static void report_error(const char* msg);
};

}	// namespace Firebird

#endif // CLASSES_TIMESTAMP_H
