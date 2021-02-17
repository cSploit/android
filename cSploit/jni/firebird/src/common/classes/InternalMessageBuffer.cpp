/*
 *	PROGRAM:		FB interfaces.
 *	MODULE:			InternalMessageBuffer.cpp
 *	DESCRIPTION:	Old=>new message style converter.
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
 * Alex Peshkov
 *
 *
 */

#include "firebird.h"
#include "../common/classes/InternalMessageBuffer.h"
#include "../common/utils_proto.h"
#include "../intl/charsets.h"
#include "../common/classes/BlrReader.h"
#include "../common/gdsassert.h"
#include "../common/MsgMetadata.h"

using namespace Firebird;

namespace
{

class MetadataFromBlr : public MsgMetadata
{
public:
	MetadataFromBlr(unsigned blrLength, const unsigned char* blr, unsigned aBufferLength);
};

MetadataFromBlr::MetadataFromBlr(unsigned aBlrLength, const unsigned char* aBlr, unsigned aLength)
{
	if (aBlrLength == 0)
		return;

	BlrReader rdr(aBlr, aBlrLength);

	const UCHAR byte = rdr.getByte();
	if (byte != blr_version4 && byte != blr_version5)
	{
		(Arg::Gds(isc_dsql_error) <<
		 Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
		 Arg::Gds(isc_wroblrver2) << Arg::Num(blr_version4) << Arg::Num(blr_version5) << Arg::Num(byte)
		).raise();
	}

	if (rdr.getByte() != blr_begin || rdr.getByte() != blr_message)
	{
		(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
		 Arg::Gds(isc_dsql_sqlda_err)
#ifdef DEV_BUILD
		 << Arg::Gds(isc_random) << "Missing blr_begin / blr_message"
#endif
		).raise();
	}

	rdr.getByte();						// skip the message number
	unsigned count = rdr.getWord();
	fb_assert(!(count & 1));
	count /= 2;

	unsigned offset = 0;
	items.grow(count);

	for (unsigned index = 0; index < count; index++)
	{
		Item* item = &items[index];
		item->scale = 0;
		item->subType = 0;

		switch (rdr.getByte())
		{
		case blr_text:
			item->type = SQL_TEXT;
			item->charSet = CS_dynamic;
			item->length = rdr.getWord();
			break;

		case blr_varying:
			item->type = SQL_VARYING;
			item->charSet = CS_dynamic;
			item->length = rdr.getWord();
			break;

		case blr_text2:
			item->type = SQL_TEXT;
			item->charSet = rdr.getWord();
			item->length = rdr.getWord();
			break;

		case blr_varying2:
			item->type = SQL_VARYING;
			item->charSet = rdr.getWord();
			item->length = rdr.getWord();
			break;

		case blr_short:
			item->type = SQL_SHORT;
			item->length = sizeof(SSHORT);
			item->scale = rdr.getByte();
			break;

		case blr_long:
			item->type = SQL_LONG;
			item->length = sizeof(SLONG);
			item->scale = rdr.getByte();
			break;

		case blr_int64:
			item->type = SQL_INT64;
			item->length = sizeof(SINT64);
			item->scale = rdr.getByte();
			break;

		case blr_quad:
			item->type = SQL_QUAD;
			item->length = sizeof(SLONG) * 2;
			item->scale = rdr.getByte();
			break;

		case blr_float:
			item->type = SQL_FLOAT;
			item->length = sizeof(float);
			break;

		case blr_double:
		case blr_d_float:
			item->type = SQL_DOUBLE;
			item->length = sizeof(double);
			break;

		case blr_timestamp:
			item->type = SQL_TIMESTAMP;
			item->length = sizeof(SLONG) * 2;
			break;

		case blr_sql_date:
			item->type = SQL_TYPE_DATE;
			item->length = sizeof(SLONG);
			break;

		case blr_sql_time:
			item->type = SQL_TYPE_TIME;
			item->length = sizeof(SLONG);
			break;

		case blr_blob2:
			item->type = SQL_BLOB;
			item->length = sizeof(ISC_QUAD);
			item->subType = rdr.getWord();
			item->charSet = rdr.getWord();
			break;

		case blr_bool:
			item->type = SQL_BOOLEAN;
			item->length = sizeof(UCHAR);
			break;

		default:
			(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
			 Arg::Gds(isc_dsql_sqlda_err)
#ifdef DEV_BUILD
			 << Arg::Gds(isc_random) << "Wrong BLR type"
#endif
			).raise();
		}

		if (rdr.getByte() != blr_short || rdr.getByte() != 0)
		{
			(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
			 Arg::Gds(isc_dsql_sqlda_err)
#ifdef DEV_BUILD
			 << Arg::Gds(isc_random) << "Wrong BLR type for NULL indicator"
#endif
			).raise();
		}

		item->finished = true;
	}

	makeOffsets();

	if (rdr.getByte() != (UCHAR) blr_end || length != aLength)
	{
		(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
		 Arg::Gds(isc_dsql_sqlda_err)
#ifdef DEV_BUILD
		 << Arg::Gds(isc_random) << (length != aLength ? "Invalid message length" : "Missing blr_end")
#endif
		).raise();
	}
}

}

namespace Firebird
{

InternalMessageBuffer::InternalMessageBuffer(unsigned aBlrLength, const unsigned char* aBlr,
	unsigned aBufferLength, unsigned char* aBuffer)
{
	buffer = aBuffer;
	metadata = new MetadataFromBlr(aBlrLength, aBlr, aBufferLength);
	metadata->addRef();
}

InternalMessageBuffer::~InternalMessageBuffer()
{
	metadata->release();
}

}
