/*
 *	PROGRAM:		FB interfaces.
 *	MODULE:			BlrFromMessage.cpp
 *	DESCRIPTION:	New=>old message style converter.
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
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2013 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include "firebird.h"
#include "BlrFromMessage.h"
#include "../common/StatusHolder.h"
#include "../jrd/align.h"

using namespace Firebird;

namespace
{

void checkStatus(IStatus* st)
{
	if (!st->isSuccess())
	{
		status_exception::raise(st->get());
	}
}

}

namespace Remote
{

BlrFromMessage::BlrFromMessage(IMessageMetadata* metadata, unsigned aDialect)
	: BlrWriter(*getDefaultMemoryPool()),
	  expectedMessageLength(0), dialect(aDialect)
{
	buildBlr(metadata);
}

unsigned BlrFromMessage::getLength()
{
	return getBlrData().getCount();
}

const unsigned char* BlrFromMessage::getBytes()
{
	return getBlrData().begin();
}

unsigned BlrFromMessage::getMsgLength()
{
	return expectedMessageLength;
}

void BlrFromMessage::buildBlr(IMessageMetadata* metadata)
{
	if (!metadata)
		return;

	LocalStatus st;

	expectedMessageLength = metadata->getMessageLength(&st);
	checkStatus(&st);

	getBlrData().clear();

	const unsigned count = metadata->getCount(&st);
	fb_assert(count < MAX_USHORT / 2);

	if (count == 0)
		return;	// If there isn't an SQLDA, don't bother with anything else.

	appendVersion();
	appendUChar(blr_begin);
	appendUChar(blr_message);
	appendUChar(0);
	appendUShort(count * 2);

	unsigned msgLen = 0;

	for (unsigned i = 0; i < count; ++i)
	{
		unsigned dtype = metadata->getType(&st, i) & ~1;
		checkStatus(&st);
		unsigned len = metadata->getLength(&st, i);
		checkStatus(&st);
		int scale = metadata->getScale(&st, i);
		checkStatus(&st);
		unsigned charSet = metadata->getCharSet(&st, i);
		checkStatus(&st);
		int subType = metadata->getSubType(&st, i);
		checkStatus(&st);

		switch (dtype)
		{
			case SQL_VARYING:
				appendUChar(blr_varying2);
				appendUShort(charSet);
				appendUShort(len);
				dtype = dtype_varying;
				len += sizeof(USHORT);
				break;

			case SQL_TEXT:
				appendUChar(blr_text2);
				appendUShort(charSet);
				appendUShort(len);
				dtype = dtype_text;
				break;

			case SQL_DOUBLE:
				appendUChar(blr_double);
				dtype = dtype_double;
				break;

			case SQL_FLOAT:
				appendUChar(blr_float);
				dtype = dtype_real;
				break;

			case SQL_D_FLOAT:
				appendUChar(blr_d_float);
				dtype = dtype_d_float;
				break;

			case SQL_TYPE_DATE:
				appendUChar(blr_sql_date);
				dtype = dtype_sql_date;
				break;

			case SQL_TYPE_TIME:
				appendUChar(blr_sql_time);
				dtype = dtype_sql_time;
				break;

			case SQL_TIMESTAMP:
				appendUChar(blr_timestamp);
				dtype = dtype_timestamp;
				break;

			case SQL_BLOB:
				appendUChar(blr_blob2);
				appendUShort(subType);
				appendUShort(charSet);
				dtype = dtype_blob;
				break;

			case SQL_ARRAY:
				appendUChar(blr_quad);
				appendUChar(0);
				dtype = dtype_array;
				break;

			case SQL_LONG:
				appendUChar(blr_long);
				appendUChar(scale);
				dtype = dtype_long;
				break;

			case SQL_SHORT:
				appendUChar(blr_short);
				appendUChar(scale);
				dtype = dtype_short;
				break;

			case SQL_INT64:
				appendUChar(blr_int64);
				appendUChar(scale);
				dtype = dtype_int64;
				break;

			case SQL_QUAD:
				appendUChar(blr_quad);
				appendUChar(scale);
				dtype = dtype_quad;
				break;

			case SQL_BOOLEAN:
				appendUChar(blr_bool);
				dtype = dtype_boolean;
				break;

			case SQL_NULL:
				appendUChar(blr_text);
				appendUShort(len);
				dtype = dtype_text;
				break;

			default:
				Arg::Gds(isc_dsql_sqlda_value_err).raise();
				break;
		}

		appendUChar(blr_short);
		appendUChar(0);

		unsigned align = type_alignments[dtype];
		if (align)
			msgLen = FB_ALIGN(msgLen, align);

		msgLen += len;

		align = type_alignments[dtype_short];
		if (align)
			msgLen = FB_ALIGN(msgLen, align);

		msgLen += sizeof(SSHORT);
	}

	appendUChar(blr_end);
	appendUChar(blr_eoc);

	if (expectedMessageLength && msgLen && (expectedMessageLength != msgLen))
	{
		Arg::Gds(isc_wrong_message_length).raise();
	}
}

bool BlrFromMessage::isVersion4()
{
	return dialect <= 1;
}

}
