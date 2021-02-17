/*
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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2011 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef FIREBIRD_MESSAGE_H
#define FIREBIRD_MESSAGE_H

#include "ibase.h"
#include "./Provider.h"
#include "./impl/boost/preprocessor/seq/for_each_i.hpp"
#include <assert.h>
#include <time.h>
#include <string.h>

#define FB_MESSAGE(name, fields)	\
	FB__MESSAGE_I(name, 2, FB_BOOST_PP_CAT(FB__MESSAGE_X fields, 0), )

#define FB__MESSAGE_X(x, y) ((x, y)) FB__MESSAGE_Y
#define FB__MESSAGE_Y(x, y) ((x, y)) FB__MESSAGE_X
#define FB__MESSAGE_X0
#define FB__MESSAGE_Y0

#define FB_TRIGGER_MESSAGE(name, fields)	\
	FB__MESSAGE_I(name, 3, FB_BOOST_PP_CAT(FB_TRIGGER_MESSAGE_X fields, 0), \
		FB_TRIGGER_MESSAGE_MOVE_NAMES(name, fields))

#define FB_TRIGGER_MESSAGE_X(x, y, z) ((x, y, z)) FB_TRIGGER_MESSAGE_Y
#define FB_TRIGGER_MESSAGE_Y(x, y, z) ((x, y, z)) FB_TRIGGER_MESSAGE_X
#define FB_TRIGGER_MESSAGE_X0
#define FB_TRIGGER_MESSAGE_Y0

#define FB__MESSAGE_I(name, size, fields, moveNames)	\
	struct name	\
	{	\
		struct Type	\
		{	\
			FB_BOOST_PP_SEQ_FOR_EACH_I(FB__MESSAGE_FIELD, size, fields)	\
		};	\
		\
		static void setup(::Firebird::IStatus* status, ::Firebird::IMetadataBuilder* builder)	\
		{	\
			unsigned index = 0;	\
			moveNames	\
			FB_BOOST_PP_SEQ_FOR_EACH_I(FB__MESSAGE_META, size, fields)	\
		}	\
		\
		name(::Firebird::IMaster* master)	\
			: desc(master, FB_BOOST_PP_SEQ_SIZE(fields), &setup)	\
		{	\
		}	\
		\
		::Firebird::IMessageMetadata* getMetadata() const	\
		{	\
			return desc.getMetadata();	\
		}	\
		\
		void clear()	\
		{	\
			memset(&data, 0, sizeof(data));	\
		}	\
		\
		Type* getData()	\
		{	\
			return &data;	\
		}	\
		\
		const Type* getData() const	\
		{	\
			return &data;	\
		}	\
		\
		Type* operator ->()	\
		{	\
			return getData();	\
		}	\
		\
		const Type* operator ->() const	\
		{	\
			return getData();	\
		}	\
		\
		Type data;	\
		::Firebird::MessageDesc desc;	\
	}

#define FB__MESSAGE_FIELD(r, _, i, xy)	\
	FB_BOOST_PP_CAT(FB__TYPE_, FB_BOOST_PP_TUPLE_ELEM(_, 0, xy)) FB_BOOST_PP_TUPLE_ELEM(_, 1, xy);	\
	ISC_SHORT FB_BOOST_PP_CAT(FB_BOOST_PP_TUPLE_ELEM(_, 1, xy), Null);

#define FB__MESSAGE_META(r, _, i, xy)	\
	FB_BOOST_PP_CAT(FB__META_, FB_BOOST_PP_TUPLE_ELEM(_, 0, xy))	\
	++index;

// Types - metadata

#define FB__META_FB_SCALED_SMALLINT(scale)	\
	builder->setType(status, index, SQL_SHORT);	\
	builder->setLength(status, index, sizeof(ISC_SHORT));	\
	builder->setScale(status, index, scale);

#define FB__META_FB_SCALED_INTEGER(scale)	\
	builder->setType(status, index, SQL_LONG);	\
	builder->setLength(status, index, sizeof(ISC_LONG));	\
	builder->setScale(status, index, scale);

#define FB__META_FB_SCALED_BIGINT(scale)	\
	builder->setType(status, index, SQL_INT64);	\
	builder->setLength(status, index, sizeof(ISC_INT64));	\
	builder->setScale(status, index, scale);

#define FB__META_FB_FLOAT	\
	builder->setType(status, index, SQL_FLOAT);	\
	builder->setLength(status, index, sizeof(float));

#define FB__META_FB_DOUBLE	\
	builder->setType(status, index, SQL_DOUBLE);	\
	builder->setLength(status, index, sizeof(double));

#define FB__META_FB_BLOB	\
	builder->setType(status, index, SQL_BLOB);	\
	builder->setLength(status, index, sizeof(ISC_QUAD));

#define FB__META_FB_BOOLEAN	\
	builder->setType(status, index, SQL_BOOLEAN);	\
	builder->setLength(status, index, sizeof(ISC_BOOLEAN));

#define FB__META_FB_DATE	\
	builder->setType(status, index, SQL_DATE);	\
	builder->setLength(status, index, sizeof(FbDate));

#define FB__META_FB_TIME	\
	builder->setType(status, index, SQL_TIME);	\
	builder->setLength(status, index, sizeof(FbTime));

#define FB__META_FB_TIMESTAMP	\
	builder->setType(status, index, SQL_TIMESTAMP);	\
	builder->setLength(status, index, sizeof(FbTimestamp));

#define FB__META_FB_CHAR(len)	\
	builder->setType(status, index, SQL_TEXT);	\
	builder->setLength(status, index, len);

#define FB__META_FB_VARCHAR(len)	\
	builder->setType(status, index, SQL_VARYING);	\
	builder->setLength(status, index, len);

#define FB__META_FB_INTL_CHAR(len, charSet)	\
	builder->setType(status, index, SQL_TEXT);	\
	builder->setLength(status, index, len);	\
	builder->setCharSet(status, index, charSet);

#define FB__META_FB_INTL_VARCHAR(len, charSet)	\
	builder->setType(status, index, SQL_VARYING);	\
	builder->setLength(status, index, len);	\
	builder->setCharSet(status, index, charSet);

#define FB__META_FB_SMALLINT			FB__META_FB_SCALED_SMALLINT(0)
#define FB__META_FB_INTEGER				FB__META_FB_SCALED_INTEGER(0)
#define FB__META_FB_BIGINT				FB__META_FB_SCALED_BIGINT(0)

// Types - struct

#define FB__TYPE_FB_SCALED_SMALLINT(x)	ISC_SHORT
#define FB__TYPE_FB_SCALED_INTEGER(x)	ISC_LONG
#define FB__TYPE_FB_SCALED_BIGINT(x)	ISC_INT64
#define FB__TYPE_FB_SMALLINT			ISC_SHORT
#define FB__TYPE_FB_INTEGER				ISC_LONG
#define FB__TYPE_FB_BIGINT				ISC_INT64
#define FB__TYPE_FB_FLOAT				float
#define FB__TYPE_FB_DOUBLE				double
#define FB__TYPE_FB_BLOB				ISC_QUAD
#define FB__TYPE_FB_BOOLEAN				ISC_UCHAR
#define FB__TYPE_FB_DATE				::Firebird::FbDate
#define FB__TYPE_FB_TIME				::Firebird::FbTime
#define FB__TYPE_FB_TIMESTAMP			::Firebird::FbTimestamp
#define FB__TYPE_FB_CHAR(len)			::Firebird::FbChar<(len)>
#define FB__TYPE_FB_VARCHAR(len)		::Firebird::FbVarChar<(len)>

#define FB_TRIGGER_MESSAGE_MOVE_NAMES(name, fields)	\
	FB_TRIGGER_MESSAGE_MOVE_NAMES_I(name, 3, FB_BOOST_PP_CAT(FB_TRIGGER_MESSAGE_MOVE_NAMES_X fields, 0))

#define FB_TRIGGER_MESSAGE_MOVE_NAMES_X(x, y, z) ((x, y, z)) FB_TRIGGER_MESSAGE_MOVE_NAMES_Y
#define FB_TRIGGER_MESSAGE_MOVE_NAMES_Y(x, y, z) ((x, y, z)) FB_TRIGGER_MESSAGE_MOVE_NAMES_X
#define FB_TRIGGER_MESSAGE_MOVE_NAMES_X0
#define FB_TRIGGER_MESSAGE_MOVE_NAMES_Y0

#define FB_TRIGGER_MESSAGE_MOVE_NAMES_I(name, size, fields)	\
	FB_BOOST_PP_SEQ_FOR_EACH_I(FB_TRIGGER_MESSAGE_MOVE_NAME, size, fields)	\
	builder->truncate(status, index);	\
	index = 0;

#define FB_TRIGGER_MESSAGE_MOVE_NAME(r, _, i, xy)	\
	builder->moveNameToIndex(status, FB_BOOST_PP_TUPLE_ELEM(_, 2, xy), index++);


namespace Firebird {


template <unsigned N>
struct FbChar
{
	char str[N];
};

template <unsigned N>
struct FbVarChar
{
	ISC_USHORT length;
	char str[N];

	void set(const char* s)
	{
		length = strlen(s);
		assert(length <= N);
		memcpy(str, s, length);
	}
};

// This class has memory layout identical to ISC_DATE.
class FbDate
{
public:
	void decode(unsigned* year, unsigned* month, unsigned* day) const
	{
		tm times;
		isc_decode_sql_date(&value, &times);

		if (year)
			*year = times.tm_year + 1900;
		if (month)
			*month = times.tm_mon + 1;
		if (day)
			*day = times.tm_mday;
	}

	unsigned getYear() const
	{
		unsigned year;
		decode(&year, NULL, NULL);
		return year;
	}

	unsigned getMonth() const
	{
		unsigned month;
		decode(NULL, &month, NULL);
		return month;
	}

	unsigned getDay() const
	{
		unsigned day;
		decode(NULL, NULL, &day);
		return day;
	}

	void encode(unsigned year, unsigned month, unsigned day)
	{
		tm times;
		times.tm_year = year - 1900;
		times.tm_mon = month - 1;
		times.tm_mday = day;

		isc_encode_sql_date(&times, &value);
	}

public:
	ISC_DATE value;
};

// This class has memory layout identical to ISC_TIME.
class FbTime
{
public:
	void decode(unsigned* hours, unsigned* minutes, unsigned* seconds, unsigned* fractions) const
	{
		tm times;
		isc_decode_sql_time(&value, &times);

		if (hours)
			*hours = times.tm_hour;
		if (minutes)
			*minutes = times.tm_min;
		if (seconds)
			*seconds = times.tm_sec;
		if (fractions)
			*fractions = value % ISC_TIME_SECONDS_PRECISION;
	}

	unsigned getHours() const
	{
		unsigned hours;
		decode(&hours, NULL, NULL, NULL);
		return hours;
	}

	unsigned getMinutes() const
	{
		unsigned minutes;
		decode(NULL, &minutes, NULL, NULL);
		return minutes;
	}

	unsigned getSeconds() const
	{
		unsigned seconds;
		decode(NULL, NULL, &seconds, NULL);
		return seconds;
	}

	unsigned getFractions() const
	{
		unsigned fractions;
		decode(NULL, NULL, NULL, &fractions);
		return fractions;
	}

	void encode(unsigned hours, unsigned minutes, unsigned seconds, unsigned fractions)
	{
		tm times;
		times.tm_hour = hours;
		times.tm_min = minutes;
		times.tm_sec = seconds;

		isc_encode_sql_time(&times, &value);
		value += fractions;
	}

public:
	ISC_TIME value;
};

// This class has memory layout identical to ISC_TIMESTAMP.
class FbTimestamp
{
public:
	FbDate date;
	FbTime time;
};

class MessageDesc
{
public:
	MessageDesc(IMaster* master, unsigned count, void (*setup)(IStatus*, IMetadataBuilder*))
	{
		IStatus* status = master->getStatus();
		IMetadataBuilder* builder = master->getMetadataBuilder(status, count);

		setup(status, builder);

		metadata = builder->getMetadata(status);

		builder->release();
		status->dispose();
	}

	~MessageDesc()
	{
		metadata->release();
	}

	IMessageMetadata* getMetadata() const
	{
		return metadata;
	}

private:
	IMessageMetadata* metadata;
};


}	// namespace Firebird

#endif	// FIREBIRD_MESSAGE_H
