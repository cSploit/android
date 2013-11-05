/*
 *	PROGRAM:
 *	MODULE:		DataTypeUtil.cpp
 *	DESCRIPTION:	Data Type Utility functions
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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/common.h"
#include "../jrd/DataTypeUtil.h"
#include "../jrd/SysFunction.h"
#include "../jrd/align.h"
#include "../jrd/dsc.h"
#include "../jrd/ibase.h"
#include "../jrd/intl.h"
#include "../jrd/dsc_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/gdsassert.h"

using namespace Firebird;


SSHORT DataTypeUtilBase::getResultBlobSubType(const dsc* value1, const dsc* value2)
{
	const SSHORT subType1 = value1->getBlobSubType();
	const SSHORT subType2 = value2->getBlobSubType();

	if (value1->isUnknown())
		return subType2;

	if (value2->isUnknown())
		return subType1;

	if (subType2 == isc_blob_untyped)	// binary
		return subType2;

	return subType1;
}


USHORT DataTypeUtilBase::getResultTextType(const dsc* value1, const dsc* value2)
{
	const USHORT cs1 = value1->getCharSet();
	const USHORT cs2 = value2->getCharSet();

	const USHORT ttype1 = value1->getTextType();
	const USHORT ttype2 = value2->getTextType();

	if (cs1 == CS_NONE || cs2 == CS_BINARY)
		return ttype2;

	if (cs1 == CS_ASCII && cs2 != CS_NONE)
		return ttype2;

	return ttype1;
}


// This function is made to determine a output descriptor from a given list
// of expressions according to the latest SQL-standard that was available.
// (ISO/ANSI SQL:200n WG3:DRS-013 H2-2002-358 August, 2002).
//
// The output type is figured out as based on this order:
// 1) If any datatype is blob, returns blob;
// 2) If any datatype is a) varying or b) any text/cstring and another datatype, returns varying;
// 3) If any datatype is text or cstring, returns text;
// 4) If any datatype is approximate numeric then each datatype in the list shall be numeric
//    (otherwise an error is thrown), returns approximate numeric;
// 5) If all datatypes are exact numeric, returns exact numeric with the maximum scale and the
//    maximum precision used.
// 6) If any datatype is a date/time/timestamp then each datatype in the list shall be the same
//    date/time/timestamp (otherwise an error is thrown), returns a date/time/timestamp.
//
// If a blob is returned, and there is a binary blob in the list, a binary blob is returned.
//
// If a blob/text is returned, the returned charset is figured out as based on this order:
// 1) If there is a OCTETS blob/string, returns OCTETS;
// 2) If there is a non-(NONE/ASCII) blob/string, returns it charset;
// 3) If there is a ASCII blob/string, a numeric or a date/time/timestamp, returns ASCII;
// 4) Otherwise, returns NONE.
void DataTypeUtilBase::makeFromList(dsc* result, const char* expressionName, int argsCount,
	const dsc** args)
{
	result->clear();

	bool allNulls = true;
	bool nullable = false;
	bool anyVarying = false;
	bool anyBlobOrText = false;

	for (const dsc** p = args; p < args + argsCount; ++p)
	{
		const dsc* arg = *p;

		allNulls &= arg->isNull();

		// Ignore NULL and parameter value from walking.
		if (arg->isNull() || arg->isUnknown())
		{
			nullable = true;
			continue;
		}

		nullable |= arg->isNullable();
		anyVarying |= arg->dsc_dtype != dtype_text;

		if (makeBlobOrText(result, arg, false))
			anyBlobOrText = true;
		else if (DTYPE_IS_NUMERIC(arg->dsc_dtype))
		{
			if (result->isUnknown() || DTYPE_IS_NUMERIC(result->dsc_dtype))
			{
				if (!arg->isExact() && result->isExact())
				{
					*result = *arg;
					result->dsc_scale = 0;	// clear it (for dialect 1)
				}
				else if (result->isUnknown() || result->isExact() || !arg->isExact())
				{
					result->dsc_dtype = MAX(result->dsc_dtype, arg->dsc_dtype);
					result->dsc_length = MAX(result->dsc_length, arg->dsc_length);
					result->dsc_scale = MIN(result->dsc_scale, arg->dsc_scale);	// scale is negative
					result->dsc_sub_type = MAX(result->dsc_sub_type, arg->dsc_sub_type);
				}
			}
			else
				makeBlobOrText(result, arg, true);
		}
		else if (DTYPE_IS_DATE(arg->dsc_dtype))
		{
			if (result->isUnknown())
				*result = *arg;
			else if (result->dsc_dtype != arg->dsc_dtype)
				makeBlobOrText(result, arg, true);
		}
		else	// we don't support this datatype here
		{
			// Unknown datatype
			status_exception::raise(Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				Arg::Gds(isc_dsql_datatype_err));
		}
	}

	// If we didn't have any blob or text but return a blob or text, it means we have incomparable
	// types like date and time without a blob or string.
	if (!anyBlobOrText && (result->isText() || result->isBlob()))
	{
		// Datatypes @1are not comparable in expression @2
		status_exception::raise(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
			Arg::Gds(isc_dsql_datatypes_not_comparable) << Arg::Str("") <<
				Arg::Str(expressionName));
	}

	if (allNulls)
		result->makeNullString();

	result->setNullable(nullable);

	// We'll return a string...
	if (result->isText())
	{
		// So convert its character length to max. byte length of the destination charset.
		const ULONG len = convertLength(result->dsc_length, CS_ASCII, result->getCharSet());
		fb_assert(len <= MAX_COLUMN_SIZE); // Maybe status_exception::raise?
		result->dsc_length = len;

		if (anyVarying)
		{
			// Adjust for varying, if it's the case.
			fb_assert(len <= MAX_COLUMN_SIZE - sizeof(USHORT));
			result->dsc_dtype = dtype_varying;
			result->dsc_length += sizeof(USHORT);
		}
	}
}


ULONG DataTypeUtilBase::convertLength(ULONG len, USHORT srcCharSet, USHORT dstCharSet)
{
	if (dstCharSet == CS_NONE || dstCharSet == CS_BINARY)
		return len;

	return (len / maxBytesPerChar(srcCharSet)) * maxBytesPerChar(dstCharSet);
}


ULONG DataTypeUtilBase::convertLength(const dsc* src, const dsc* dst)
{
	fb_assert(dst->isText());
	if (src->dsc_dtype == dtype_dbkey)
	{
		return src->dsc_length;
	}
	return convertLength(src->getStringLength(), src->getCharSet(), dst->getCharSet());
}


ULONG DataTypeUtilBase::fixLength(const dsc* desc, ULONG length)
{
	const UCHAR bpc = maxBytesPerChar(desc->getCharSet());

	USHORT overhead = 0;
	if (desc->dsc_dtype == dtype_varying)
		overhead = sizeof(USHORT);
	else if (desc->dsc_dtype == dtype_cstring)
		overhead = sizeof(UCHAR);

	return MIN(((MAX_COLUMN_SIZE - overhead) / bpc) * bpc, length);
}


void DataTypeUtilBase::makeConcatenate(dsc* result, const dsc* value1, const dsc* value2)
{
	result->clear();

	if (value1->isNull() && value2->isNull())
	{
		result->makeNullString();
		return;
	}

	if (value1->dsc_dtype == dtype_dbkey && value2->dsc_dtype == dtype_dbkey)
	{
		result->dsc_dtype = dtype_dbkey;
		result->dsc_length = value1->dsc_length + value2->dsc_length;
	}
	else if (value1->isBlob() || value2->isBlob())
	{
		result->dsc_dtype = dtype_blob;
		result->dsc_length = sizeof(ISC_QUAD);
		result->setBlobSubType(getResultBlobSubType(value1, value2));
		result->setTextType(getResultTextType(value1, value2));
	}
	else
	{
		result->dsc_dtype = dtype_varying;
		result->setTextType(getResultTextType(value1, value2));

		ULONG length = fixLength(result,
			convertLength(value1, result) + convertLength(value2, result));
		result->dsc_length = length + sizeof(USHORT);
	}

	result->dsc_flags = (value1->dsc_flags | value2->dsc_flags) & DSC_nullable;
}


void DataTypeUtilBase::makeSubstr(dsc* result, const dsc* value, const dsc* offset, const dsc* length)
{
	result->clear();

	if (value->isNull())
	{
		result->makeNullString();
		return;
	}

	if (value->isBlob())
	{
		result->dsc_dtype = dtype_blob;
		result->dsc_length = sizeof(ISC_QUAD);
		result->setBlobSubType(value->getBlobSubType());
	}
	else
	{
		// Beware that JRD treats substring() always as returning CHAR
		// instead of VARCHAR for historical reasons.
		result->dsc_dtype = dtype_varying;
	}

	result->setTextType(value->getTextType());
	result->setNullable(value->isNullable() || offset->isNullable() || length->isNullable());

	if (result->isText())
		result->dsc_length = fixLength(result, convertLength(value, result)) + sizeof(USHORT);
}


void DataTypeUtilBase::makeSysFunction(dsc* result, const char* name, int argsCount, const dsc** args)
{
	const SysFunction* function = SysFunction::lookup(name);

	if (function)
	{
		function->checkArgsMismatch(argsCount);
		function->makeFunc(this, function, result, argsCount, args);
	}
}


bool DataTypeUtilBase::makeBlobOrText(dsc* result, const dsc* arg, bool force)
{
	if (arg->isBlob() || result->isBlob())
	{
		result->makeBlob(getResultBlobSubType(result, arg), getResultTextType(result, arg));
		return true;
	}

	if (force || arg->isText() || result->isText())
	{
		USHORT argLen = convertLength(arg->getStringLength(), arg->getCharSet(), CS_ASCII);
		USHORT resultLen = result->getStringLength();

		result->makeText(MAX(argLen, resultLen), getResultTextType(result, arg));

		return true;
	}

	return false;
}


namespace Jrd {

UCHAR DataTypeUtil::maxBytesPerChar(UCHAR charSet)
{
	return INTL_charset_lookup(tdbb, charSet)->maxBytesPerChar();
}

USHORT DataTypeUtil::getDialect() const
{
	return (tdbb->getDatabase()->dbb_flags & DBB_DB_SQL_dialect_3) ? 3 : 1;
}

}	// namespace Jrd
