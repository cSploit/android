/*
 *	PROGRAM:	JRD System Functions
 *	MODULE:		SysFunctions.h
 *	DESCRIPTION:	System Functions
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
 *  for the Firebird Open Source RDBMS project, based on work done
 *  in Yaffil by Oleg Loa <loa@mail.ru> and Alexey Karyakin <aleksey.karyakin@mail.ru>
 *
 *  Copyright (c) 2007 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *    Oleg Loa <loa@mail.ru>
 *    Alexey Karyakin <aleksey.karyakin@mail.ru>
 *
 */

#include "firebird.h"
#include "../common/classes/VaryStr.h"
#include "../jrd/SysFunction.h"
#include "../jrd/DataTypeUtil.h"
#include "../include/fb_blk.h"
#include "../jrd/exe.h"
#include "../jrd/intl.h"
#include "../jrd/req.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cvt_proto.h"
#include "../common/cvt.h"
#include "../jrd/evl_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/os/guid.h"
#include "../common/classes/FpeControl.h"
#include <math.h>

using namespace Firebird;
using namespace Jrd;

namespace {

// function types handled in generic functions
enum Function
{
	funNone, // do not use
	funBinAnd,
	funBinOr,
	funBinShl,
	funBinShr,
	funBinShlRot,
	funBinShrRot,
	funBinXor,
	funBinNot,
	funMaxValue,
	funMinValue,
	funLPad,
	funRPad,
	funLnat,
	funLog10
};

enum TrigonFunction
{
	trfNone, // do not use
	trfSin,
	trfCos,
	trfTan,
	trfCot,
	trfAsin,
	trfAcos,
	trfAtan,
	trfSinh,
	trfCosh,
	trfTanh,
	trfAsinh,
	trfAcosh,
	trfAtanh
};


// constants
const int oneDay = 86400;

// auxiliary functions
void add10msec(ISC_TIMESTAMP* v, int msec, SINT64 multiplier);
double fbcot(double value) throw();

// generic setParams functions
void setParamsDouble(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);
void setParamsFromList(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);
void setParamsInteger(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);
void setParamsSecondInteger(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);

// specific setParams functions
void setParamsAsciiVal(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);
void setParamsCharToUuid(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);
void setParamsDateAdd(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);
void setParamsDateDiff(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);
void setParamsOverlay(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);
void setParamsPosition(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);
void setParamsRoundTrunc(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);
void setParamsUuidToChar(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, int argsCount, dsc** args);

// generic make functions
void makeDoubleResult(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeFromListResult(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeInt64Result(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeLongResult(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
///void makeLongStringOrBlobResult(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeShortResult(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);

// specific make functions
void makeAbs(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeAsciiChar(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeBin(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeBinShift(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeCeilFloor(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeDateAdd(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeLeftRight(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeMod(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeOverlay(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makePad(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeReplace(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeReverse(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeRound(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeTrunc(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeUuid(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);
void makeUuidToChar(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result, int argsCount, const dsc** args);

// generic stdmath function
dsc* evlStdMath(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);

// specific evl functions
dsc* evlAbs(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlAsciiChar(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlAsciiVal(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlAtan2(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlBin(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlBinShift(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlCeil(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlCharToUuid(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlDateAdd(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlDateDiff(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlExp(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlFloor(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlGenUuid(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlHash(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlLeft(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlLnLog10(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlLog(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlMaxMinValue(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlMod(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlOverlay(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlPad(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlPi(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlPosition(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlPower(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlRand(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlReplace(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlReverse(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlRight(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlRound(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlSign(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlSqrt(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlTrunc(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);
dsc* evlUuidToChar(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args, Jrd::impure_value* impure);


void add10msec(ISC_TIMESTAMP* v, int msec, SINT64 multiplier)
{
	const SINT64 full = msec * multiplier;
	const int days = full / (oneDay * ISC_TIME_SECONDS_PRECISION);
	const int secs = full % (oneDay * ISC_TIME_SECONDS_PRECISION);

	v->timestamp_date += days;

	// Time portion is unsigned, so we avoid unsigned rolling over negative values
	// that only produce a new unsigned number with the wrong result.
	if (secs < 0 && ISC_TIME(-secs) > v->timestamp_time)
	{
		v->timestamp_date--;
		v->timestamp_time += (oneDay * ISC_TIME_SECONDS_PRECISION) + secs;
	}
	else if ((v->timestamp_time += secs) >= (oneDay * ISC_TIME_SECONDS_PRECISION))
	{
		v->timestamp_date++;
		v->timestamp_time -= (oneDay * ISC_TIME_SECONDS_PRECISION);
	}
}


double fbcot(double value) throw()
{
	return 1.0 / tan(value);
}


bool initResult(dsc* result, int argsCount, const dsc** args, bool* isNullable)
{
	*isNullable = false;

	for (int i = 0; i < argsCount; ++i)
	{
		if (args[i]->isNull())
		{
			result->setNull();
			return true;
		}

		if (args[i]->isNullable())
			*isNullable = true;
	}

	return false;
}


void setParamsDouble(DataTypeUtilBase*, const SysFunction*, int argsCount, dsc** args)
{
	for (int i = 0; i < argsCount; ++i)
	{
		if (args[i]->isUnknown())
			args[i]->makeDouble();
	}
}


void setParamsFromList(DataTypeUtilBase* dataTypeUtil, const SysFunction* function,
	int argsCount, dsc** args)
{
	dsc desc;
	dataTypeUtil->makeFromList(&desc, function->name.c_str(), argsCount, const_cast<const dsc**>(args));

	for (int i = 0; i < argsCount; ++i)
	{
		if (args[i]->isUnknown())
			*args[i] = desc;
	}
}


void setParamsInteger(DataTypeUtilBase*, const SysFunction*, int argsCount, dsc** args)
{
	for (int i = 0; i < argsCount; ++i)
	{
		if (args[i]->isUnknown())
			args[i]->makeLong(0);
	}
}


void setParamsSecondInteger(DataTypeUtilBase*, const SysFunction*, int argsCount, dsc** args)
{
	if (argsCount >= 2)
	{
		if (args[1]->isUnknown())
			args[1]->makeLong(0);
	}
}


void setParamsAsciiVal(DataTypeUtilBase*, const SysFunction*, int argsCount, dsc** args)
{
	if (argsCount >= 1 && args[0]->isUnknown())
		args[0]->makeText(1, CS_ASCII);
}


void setParamsCharToUuid(DataTypeUtilBase*, const SysFunction*, int argsCount, dsc** args)
{
	if (argsCount >= 1 && args[0]->isUnknown())
		args[0]->makeText(GUID_BODY_SIZE, ttype_ascii);
}


void setParamsDateAdd(DataTypeUtilBase*, const SysFunction*, int argsCount, dsc** args)
{
	if (argsCount >= 1 && args[0]->isUnknown())
		args[0]->makeLong(0);

	if (argsCount >= 3 && args[2]->isUnknown())
		args[2]->makeTimestamp();
}


void setParamsDateDiff(DataTypeUtilBase*, const SysFunction*, int argsCount, dsc** args)
{
	if (argsCount >= 3)
	{
		if (args[1]->isUnknown() && args[2]->isUnknown())
		{
			args[1]->makeTimestamp();
			args[2]->makeTimestamp();
		}
		else if (args[1]->isUnknown())
			*args[1] = *args[2];
		else if (args[2]->isUnknown())
			*args[2] = *args[1];
	}
}


void setParamsOverlay(DataTypeUtilBase*, const SysFunction*, int argsCount, dsc** args)
{
	if (argsCount >= 3)
	{
		if (!(args[0]->isUnknown() && args[1]->isUnknown()))
		{
			if (args[1]->isUnknown())
				*args[1] = *args[0];
			else if (args[0]->isUnknown())
				*args[0] = *args[1];
		}

		if (argsCount >= 4)
		{
			if (args[2]->isUnknown() && args[3]->isUnknown())
			{
				args[2]->makeLong(0);
				args[3]->makeLong(0);
			}
			else if (args[2]->isUnknown())
				*args[2] = *args[3];
			else if (args[3]->isUnknown())
				*args[3] = *args[2];
		}

		if (args[2]->isUnknown())
			args[2]->makeLong(0);
	}
}


void setParamsPosition(DataTypeUtilBase*, const SysFunction*, int argsCount, dsc** args)
{
	if (argsCount >= 2)
	{
		if (args[0]->isUnknown())
			*args[0] = *args[1];

		if (args[1]->isUnknown())
			*args[1] = *args[0];
	}
}


void setParamsRoundTrunc(DataTypeUtilBase*, const SysFunction*, int argsCount, dsc** args)
{
	if (argsCount >= 1)
	{
		if (args[0]->isUnknown())
			args[0]->makeDouble();

		if (argsCount >= 2)
		{
			if (args[1]->isUnknown())
				args[1]->makeLong(0);
		}
	}
}


void setParamsUuidToChar(DataTypeUtilBase*, const SysFunction*, int argsCount, dsc** args)
{
	if (argsCount >= 1 && args[0]->isUnknown())
		args[0]->makeText(16, ttype_binary);
}


void makeDoubleResult(DataTypeUtilBase*, const SysFunction*, dsc* result,
	int argsCount, const dsc** args)
{
	result->makeDouble();

	bool isNullable;
	if (initResult(result, argsCount, args, &isNullable))
		return;

	result->setNullable(isNullable);
}


void makeFromListResult(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	result->clear();
	dataTypeUtil->makeFromList(result, function->name.c_str(), argsCount, args);
}


void makeInt64Result(DataTypeUtilBase* dataTypeUtil, const SysFunction*, dsc* result,
	int argsCount, const dsc** args)
{
	if (dataTypeUtil->getDialect() == 1)
		result->makeDouble();
	else
		result->makeInt64(0);

	bool isNullable;
	if (initResult(result, argsCount, args, &isNullable))
		return;

	result->setNullable(isNullable);
}


void makeLongResult(DataTypeUtilBase*, const SysFunction*, dsc* result,
	int argsCount, const dsc** args)
{
	result->makeLong(0);

	bool isNullable;
	if (initResult(result, argsCount, args, &isNullable))
		return;

	result->setNullable(isNullable);
}


/***
 * This function doesn't work yet, because makeFromListResult isn't totally prepared for blobs vs strings.
 *
void makeLongStringOrBlobResult(DataTypeUtilBase* dataTypeUtil, const SysFunction* function,
	dsc* result, int argsCount, const dsc** args)
{
	makeFromListResult(dataTypeUtil, function, result, argsCount, args);

	if (result->isText())
		result->makeVarying(dataTypeUtil->fixLength(result, MAX_COLUMN_SIZE), result->getTextType());
}
***/


void makeShortResult(DataTypeUtilBase*, const SysFunction*, dsc* result,
	int argsCount, const dsc** args)
{
	result->makeShort(0);

	bool isNullable;
	if (initResult(result, argsCount, args, &isNullable))
		return;

	result->setNullable(isNullable);
}


void makeAbs(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount == function->minArgCount);

	const dsc* value = args[0];

	if (value->isNull())
	{
		result->makeLong(0);
		result->setNull();
		return;
	}

	switch (value->dsc_dtype)
	{
		case dtype_short:
			result->makeLong(value->dsc_scale);
			break;

		case dtype_long:
			if (dataTypeUtil->getDialect() == 1)
				result->makeDouble();
			else
				result->makeInt64(value->dsc_scale);
			break;

		case dtype_real:
		case dtype_double:
		case dtype_int64:
			*result = *value;
			break;

		default:
			result->makeDouble();
			break;
	}

	result->setNullable(value->isNullable());
}


void makeAsciiChar(DataTypeUtilBase*, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount == function->minArgCount);

	const dsc* value = args[0];

	if (value->isNull())
	{
		result->makeNullString();
		return;
	}

	result->makeText(1, ttype_none);
	result->setNullable(value->isNullable());
}


void makeBin(DataTypeUtilBase*, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount >= function->minArgCount);

	bool isNullable = false;
	bool isNull = false;
	bool first = true;

	for (int i = 0; i < argsCount; ++i)
	{
		if (args[i]->isNullable())
			isNullable = true;

		if (args[i]->isNull())
		{
			isNull = true;
			continue;
		}

		if (!args[i]->isExact() || args[i]->dsc_scale != 0)
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_argmustbe_exact) << Arg::Str(function->name));

		if (first)
		{
			first = false;

			result->clear();
			result->dsc_dtype = args[i]->dsc_dtype;
			result->dsc_length = args[i]->dsc_length;
		}
		else
		{
			if (args[i]->dsc_dtype == dtype_int64)
				result->makeInt64(0);
			else if (args[i]->dsc_dtype == dtype_long && result->dsc_dtype != dtype_int64)
				result->makeLong(0);
		}
	}

	if (isNull)
	{
		if (first)
			result->makeLong(0);
		result->setNull();
	}

	result->setNullable(isNullable);
}


void makeBinShift(DataTypeUtilBase*, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount >= function->minArgCount);

	result->makeInt64(0);

	bool isNullable = false;

	for (int i = 0; i < argsCount; ++i)
	{
		if (args[i]->isNull())
		{
			result->setNull();
			return;
		}

		if (args[i]->isNullable())
			isNullable = true;

		if (!args[i]->isExact() || args[i]->dsc_scale != 0)
		{
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_argmustbe_exact) << Arg::Str(function->name));
		}
	}

	result->setNullable(isNullable);
}


void makeCeilFloor(DataTypeUtilBase*, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount == function->minArgCount);

	const dsc* value = args[0];

	if (value->isNull())
	{
		result->makeLong(0);
		result->setNull();
		return;
	}

	switch (value->dsc_dtype)
	{
		case dtype_short:
			result->makeLong(0);
			break;

		case dtype_long:
		case dtype_int64:
			result->makeInt64(0);
			break;

		default:
			result->makeDouble();
			break;
	}

	result->setNullable(value->isNullable());
}


void makeDateAdd(DataTypeUtilBase*, const SysFunction*, dsc* result, int argsCount, const dsc** args)
{
	fb_assert(argsCount >= 3);

	*result = *args[2];

	bool isNullable;
	if (initResult(result, argsCount, args, &isNullable))
		return;

	*result = *args[2];
	result->setNullable(isNullable);
}


void makeLeftRight(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount == function->minArgCount);

	const dsc* value = args[0];
	const dsc* length = args[1];

	if (value->isNull() || length->isNull())
	{
		result->makeNullString();
		return;
	}

	if (value->isBlob())
		result->makeBlob(value->getBlobSubType(), value->getTextType());
	else
	{
		result->clear();
		result->dsc_dtype = dtype_varying;
		result->setTextType(value->getTextType());
		result->setNullable(value->isNullable() || length->isNullable());

		result->dsc_length =
			dataTypeUtil->fixLength(result, dataTypeUtil->convertLength(value, result)) + sizeof(USHORT);
	}
}


void makeMod(DataTypeUtilBase*,	 const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount == function->minArgCount);

	const dsc* value1 = args[0];
	const dsc* value2 = args[1];

	if (value1->isNull() || value2->isNull())
	{
		result->makeLong(0);
		result->setNull();
		return;
	}

	switch (value1->dsc_dtype)
	{
		case dtype_short:
		case dtype_long:
		case dtype_int64:
			*result = *value1;
			result->dsc_scale = 0;
			break;

		default:
			result->makeInt64(0);
			break;
	}

	result->setNullable(value1->isNullable() || value2->isNullable());
}


void makeOverlay(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount >= function->minArgCount);

	result->makeNullString();

	bool isNullable;
	if (initResult(result, argsCount, args, &isNullable))
		return;

	const dsc* value = args[0];
	const dsc* placing = args[1];

	if (value->isBlob())
		*result = *value;
	else if (placing->isBlob())
		*result = *placing;
	else
	{
		result->clear();
		result->dsc_dtype = dtype_varying;
	}

	result->setBlobSubType(dataTypeUtil->getResultBlobSubType(value, placing));
	result->setTextType(dataTypeUtil->getResultTextType(value, placing));

	if (!value->isBlob() && !placing->isBlob())
	{
		result->dsc_length = sizeof(USHORT) +
			dataTypeUtil->convertLength(value, result) +
			dataTypeUtil->convertLength(placing, result);
	}

	result->setNullable(isNullable);
}


void makePad(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount >= function->minArgCount);

	result->makeNullString();

	bool isNullable;
	if (initResult(result, argsCount, args, &isNullable))
		return;

	const dsc* value1 = args[0];
	const dsc* length = args[1];
	const dsc* value2 = (argsCount >= 3 ? args[2] : NULL);

	if (value1->isBlob())
		*result = *value1;
	else if (value2 && value2->isBlob())
		*result = *value2;
	else
	{
		result->clear();
		result->dsc_dtype = dtype_varying;
	}

	result->setBlobSubType(value1->getBlobSubType());
	result->setTextType(value1->getTextType());

	if (!result->isBlob())
	{
		if (length->dsc_address)	// constant
		{
			result->dsc_length = sizeof(USHORT) + dataTypeUtil->fixLength(result,
				CVT_get_long(length, 0, ERR_post) *
					dataTypeUtil->maxBytesPerChar(result->getCharSet()));
		}
		else
			result->dsc_length = sizeof(USHORT) + dataTypeUtil->fixLength(result, MAX_COLUMN_SIZE);
	}

	result->setNullable(isNullable);
}


void makeReplace(DataTypeUtilBase* dataTypeUtil, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount >= function->minArgCount);

	bool isNullable = false;
	const dsc* firstBlob = NULL;

	for (int i = 0; i < argsCount; ++i)
	{
		if (args[i]->isNull())
		{
			result->makeNullString();
			return;
		}

		if (args[i]->isNullable())
			isNullable = true;

		if (!firstBlob && args[i]->isBlob())
			firstBlob = args[i];
	}

	const dsc* searched = args[0];
	const dsc* find = args[1];
	const dsc* replacement = args[2];

	if (firstBlob)
		*result = *firstBlob;
	else
	{
		result->clear();
		result->dsc_dtype = dtype_varying;
	}

	result->setBlobSubType(dataTypeUtil->getResultBlobSubType(searched, find));
	result->setBlobSubType(dataTypeUtil->getResultBlobSubType(result, replacement));

	result->setTextType(dataTypeUtil->getResultTextType(searched, find));
	result->setTextType(dataTypeUtil->getResultTextType(result, replacement));

	if (!firstBlob)
	{
		const int searchedLen = dataTypeUtil->convertLength(searched, result);
		const int findLen = dataTypeUtil->convertLength(find, result);
		const int replacementLen = dataTypeUtil->convertLength(replacement, result);

		if (findLen == 0)
			result->dsc_length = dataTypeUtil->fixLength(result, searchedLen) + sizeof(USHORT);
		else
		{
			result->dsc_length = dataTypeUtil->fixLength(result, MAX(searchedLen,
				searchedLen + (searchedLen / findLen) * (replacementLen - findLen))) + sizeof(USHORT);
		}
	}

	result->setNullable(isNullable);
}


void makeReverse(DataTypeUtilBase*, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount == function->minArgCount);

	const dsc* value = args[0];

	if (value->isNull())
	{
		result->makeNullString();
		return;
	}

	if (value->isBlob())
		*result = *value;
	else
		result->makeVarying(value->getStringLength(), value->getTextType());

	result->setNullable(value->isNullable());
}


void makeRound(DataTypeUtilBase*, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount >= function->minArgCount);

	const dsc* value1 = args[0];

	if (value1->isNull() || (argsCount > 1 && args[1]->isNull()))
	{
		result->makeLong(0);
		result->setNull();
		return;
	}

	if (value1->isExact() || value1->dsc_dtype == dtype_real || value1->dsc_dtype == dtype_double)
	{
		*result = *value1;
		if (argsCount == 1)
			result->dsc_scale = 0;
	}
	else
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argmustbe_exact_or_fp) << Arg::Str(function->name));

	result->setNullable(value1->isNullable() || (argsCount > 1 && args[1]->isNullable()));
}


void makeTrunc(DataTypeUtilBase*, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount >= function->minArgCount);

	const dsc* value = args[0];

	if (value->isNull() || (argsCount == 2 && args[1]->isNull()))
	{
		result->makeLong(0);
		result->setNull();
		return;
	}

	switch (value->dsc_dtype)
	{
		case dtype_short:
		case dtype_long:
		case dtype_int64:
			*result = *value;
			if (argsCount == 1)
				result->dsc_scale = 0;
			break;

		default:
			result->makeDouble();
			break;
	}

	result->setNullable(value->isNullable() || (argsCount > 1 && args[1]->isNullable()));
}


void makeUuid(DataTypeUtilBase*, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount == function->minArgCount);

	if (argsCount > 0 && args[0]->isNull())
		result->makeNullString();
	else
		result->makeText(16, ttype_binary);

	if (argsCount > 0 && args[0]->isNullable())
		result->setNullable(true);
}


void makeUuidToChar(DataTypeUtilBase*, const SysFunction* function, dsc* result,
	int argsCount, const dsc** args)
{
	fb_assert(argsCount == function->minArgCount);

	const dsc* value = args[0];

	if (value->isNull())
	{
		result->makeNullString();
		return;
	}

	result->makeText(GUID_BODY_SIZE, ttype_ascii);
	result->setNullable(value->isNullable());
}


dsc* evlStdMath(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);
	fb_assert(function->misc != NULL);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	const double v = MOV_get_double(value);
	double rc;

	// CVC: Apparently, gcc has built-in inverse hyperbolic functions, but since
	// VC doesn't, I'm taking the definitions from Wikipedia

	switch ((TrigonFunction)(IPTR) function->misc)
	{
	case trfSin:
		rc = sin(v);
		break;
	case trfCos:
		rc = cos(v);
		break;
	case trfTan:
		rc = tan(v);
		break;
	case trfCot:
		if (!v)
		{
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argmustbe_nonzero) << Arg::Str(function->name));;
		}
		rc = fbcot(v);
		break;
	case trfAsin:
		if (v < -1 || v > 1)
		{
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argmustbe_range_inc1_1) << Arg::Str(function->name));
		}
		rc = asin(v);
		break;
	case trfAcos:
		if (v < -1 || v > 1)
		{
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argmustbe_range_inc1_1) << Arg::Str(function->name));
		}
		rc = acos(v);
		break;
	case trfAtan:
		rc = atan(v);
		break;
	case trfSinh:
		rc = sinh(v);
		break;
	case trfCosh:
		rc = cosh(v);
		break;
	case trfTanh:
		rc = tanh(v);
		break;
	case trfAsinh:
		rc = log(v + sqrt(v * v + 1));
		break;
	case trfAcosh:
		if (v < 1)
		{
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argmustbe_gteq_one) << Arg::Str(function->name));
		}
		rc = log(v + sqrt(v - 1) * sqrt (v + 1));
		break;
	case trfAtanh:
		if (v <= -1 || v >= 1)
		{
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argmustbe_range_exc1_1) << Arg::Str(function->name));
		}
		rc = log((1 + v) / (1 - v)) / 2;
		break;
	default:
		fb_assert(0);
	}

	if (isinf(rc))
	{
		status_exception::raise(Arg::Gds(isc_arith_except) <<
								Arg::Gds(isc_sysf_fp_overflow) << Arg::Str(function->name));
	}

	impure->vlu_misc.vlu_double = rc;
	impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);

	return &impure->vlu_desc;
}


dsc* evlAbs(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	EVL_make_value(tdbb, value, impure);

	switch (impure->vlu_desc.dsc_dtype)
	{
		case dtype_real:
			impure->vlu_misc.vlu_float = fabs(impure->vlu_misc.vlu_float);
			break;

		case dtype_double:
			impure->vlu_misc.vlu_double = fabs(impure->vlu_misc.vlu_double);
			break;

		case dtype_short:
		case dtype_long:
		case dtype_int64:
			impure->vlu_misc.vlu_int64 = MOV_get_int64(value, value->dsc_scale);

			if (impure->vlu_misc.vlu_int64 == MIN_SINT64)
				status_exception::raise(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));
			else if (impure->vlu_misc.vlu_int64 < 0)
				impure->vlu_misc.vlu_int64 = -impure->vlu_misc.vlu_int64;

			impure->vlu_desc.makeInt64(value->dsc_scale, &impure->vlu_misc.vlu_int64);
			break;

		default:
			impure->vlu_misc.vlu_double = fabs(MOV_get_double(&impure->vlu_desc));
			impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);
			break;
	}

	return &impure->vlu_desc;
}


dsc* evlAsciiChar(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	const SLONG code = MOV_get_long(value, 0);
	if (!(code >= 0 && code <= 255))
		status_exception::raise(Arg::Gds(isc_arith_except) << Arg::Gds(isc_numeric_out_of_range));

	impure->vlu_misc.vlu_uchar = (UCHAR) code;
	impure->vlu_desc.makeText(1, ttype_none, &impure->vlu_misc.vlu_uchar);

	return &impure->vlu_desc;
}


dsc* evlAsciiVal(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	const CharSet* cs = INTL_charset_lookup(tdbb, value->getCharSet());

	UCHAR* p;
	MoveBuffer temp;
	int length = MOV_make_string2(tdbb, value, value->getCharSet(), &p, temp);

	if (length == 0)
		impure->vlu_misc.vlu_short = 0;
	else
	{
		UCHAR dummy[4];

		if (cs->substring(length, p, sizeof(dummy), dummy, 0, 1) != 1)
			status_exception::raise(Arg::Gds(isc_arith_except) << Arg::Gds(isc_transliteration_failed));

		impure->vlu_misc.vlu_short = p[0];
	}

	impure->vlu_desc.makeShort(0, &impure->vlu_misc.vlu_short);

	return &impure->vlu_desc;
}


dsc* evlAtan2(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 2);

	jrd_req* request = tdbb->getRequest();

	const dsc* value1 = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value1 is NULL
		return NULL;

	const dsc* value2 = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if value2 is NULL
		return NULL;

	impure->vlu_misc.vlu_double = atan2(MOV_get_double(value1), MOV_get_double(value2));
	impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);

	return &impure->vlu_desc;
}


dsc* evlBin(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count >= 1);
	fb_assert(function->misc != NULL);

	jrd_req* request = tdbb->getRequest();

	for (int i = 0; i < args->nod_count; ++i)
	{
		const dsc* value = EVL_expr(tdbb, args->nod_arg[i]);
		if (request->req_flags & req_null)	// return NULL if value is NULL
			return NULL;

		if (i == 0)
		{
			if ((Function)(IPTR) function->misc == funBinNot)
				impure->vlu_misc.vlu_int64 = ~MOV_get_int64(value, 0);
			else
				impure->vlu_misc.vlu_int64 = MOV_get_int64(value, 0);
		}
		else
		{
			switch ((Function)(IPTR) function->misc)
			{
				case funBinAnd:
					impure->vlu_misc.vlu_int64 &= MOV_get_int64(value, 0);
					break;

				case funBinOr:
					impure->vlu_misc.vlu_int64 |= MOV_get_int64(value, 0);
					break;

				case funBinXor:
					impure->vlu_misc.vlu_int64 ^= MOV_get_int64(value, 0);
					break;

				default:
					fb_assert(false);
			}
		}
	}

	impure->vlu_desc.makeInt64(0, &impure->vlu_misc.vlu_int64);

	return &impure->vlu_desc;
}


dsc* evlBinShift(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 2);
	fb_assert(function->misc != NULL);

	jrd_req* request = tdbb->getRequest();

	const dsc* value1 = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value1 is NULL
		return NULL;

	const dsc* value2 = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if value2 is NULL
		return NULL;

	const SINT64 shift = MOV_get_int64(value2, 0);
	if (shift < 0)
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
								Arg::Gds(isc_sysf_argmustbe_nonneg) << Arg::Str(function->name));
	}

	const SINT64 rotshift = shift % sizeof(SINT64);
	SINT64 tempbits = 0;

	const SINT64 target = MOV_get_int64(value1, 0);

	switch ((Function)(IPTR) function->misc)
	{
		case funBinShl:
			impure->vlu_misc.vlu_int64 = target << shift;
			break;

		case funBinShr:
			impure->vlu_misc.vlu_int64 = target >> shift;
			break;

		case funBinShlRot:
			tempbits = target >> (sizeof(SINT64) - rotshift);
			impure->vlu_misc.vlu_int64 = (target << rotshift) | tempbits;
			break;

		case funBinShrRot:
			tempbits = target << (sizeof(SINT64) - rotshift);
			impure->vlu_misc.vlu_int64 = (target >> rotshift) | tempbits;
			break;

		default:
			fb_assert(false);
	}

	impure->vlu_desc.makeInt64(0, &impure->vlu_misc.vlu_int64);

	return &impure->vlu_desc;
}


dsc* evlCeil(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	EVL_make_value(tdbb, value, impure);

	switch (impure->vlu_desc.dsc_dtype)
	{
		case dtype_short:
		case dtype_long:
		case dtype_int64:
			{
				SINT64 scale = 1;

				fb_assert(impure->vlu_desc.dsc_scale <= 0);
				for (int i = -impure->vlu_desc.dsc_scale; i > 0; --i)
					scale *= 10;

				const SINT64 v1 = MOV_get_int64(&impure->vlu_desc, impure->vlu_desc.dsc_scale);
				const SINT64 v2 = MOV_get_int64(&impure->vlu_desc, 0) * scale;

				impure->vlu_misc.vlu_int64 = v1 / scale;

				if (v1 > 0 && v1 != v2)
					++impure->vlu_misc.vlu_int64;

				impure->vlu_desc.makeInt64(0, &impure->vlu_misc.vlu_int64);
			}
			break;

		case dtype_real:
			impure->vlu_misc.vlu_float = ceil(impure->vlu_misc.vlu_float);
			break;

		default:
			impure->vlu_misc.vlu_double = MOV_get_double(&impure->vlu_desc);
			// fall through

		case dtype_double:
			impure->vlu_misc.vlu_double = ceil(impure->vlu_misc.vlu_double);
			impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);
			break;
	}

	return &impure->vlu_desc;
}


string showInvalidChar(const UCHAR c)
{
	string str;
	str.printf("%c (ASCII %d)", c, c);
	return str;
}


dsc* evlCharToUuid(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	if (!value->isText())
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argviolates_uuidtype) << Arg::Str(function->name));
	}

	USHORT ttype;
	UCHAR* data_temp;
	const USHORT len = CVT_get_string_ptr(value, &ttype, &data_temp, NULL, 0);
	const UCHAR* data = data_temp;

	// validate the UUID
	if (len != GUID_BODY_SIZE) // 36
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argviolates_uuidlen) <<
										Arg::Num(GUID_BODY_SIZE) <<
										Arg::Str(function->name));
	}

	for (int i = 0; i < GUID_BODY_SIZE; ++i)
	{
		if (i == 8 || i == 13 || i == 18 || i == 23)
		{
			if (data[i] != '-')
			{
				status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
											Arg::Gds(isc_sysf_argviolates_uuidfmt) <<
												Arg::Str(showInvalidChar(data[i])) <<
												Arg::Num(i + 1) <<
												Arg::Str(function->name));
			}
		}
		else
		{
			const UCHAR c = data[i];
			const UCHAR hex = UPPER7(c);

			if (!((hex >= 'A' && hex <= 'F') || (c >= '0' && c <= '9')))
			{
				status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
											Arg::Gds(isc_sysf_argviolates_guidigits) <<
												Arg::Str(showInvalidChar(c)) <<
												Arg::Num(i + 1) <<
												Arg::Str(function->name));
			}
		}
	}

	// convert to binary representation
	char buffer[GUID_BUFF_SIZE];
	buffer[0] = '{';
	buffer[37] = '}';
	buffer[38] = '\0';
	memcpy(buffer + 1, data, GUID_BODY_SIZE);

	USHORT bytes[16];
	sscanf(buffer, GUID_NEW_FORMAT,
		&bytes[0], &bytes[1], &bytes[2], &bytes[3],
		&bytes[4], &bytes[5], &bytes[6], &bytes[7],
		&bytes[8], &bytes[9], &bytes[10], &bytes[11],
		&bytes[12], &bytes[13], &bytes[14], &bytes[15]);

	UCHAR resultData[16];
	for (unsigned i = 0; i < 16; ++i)
		resultData[i] = (UCHAR) bytes[i];

	dsc result;
	result.makeText(16, ttype_binary, resultData);
	EVL_make_value(tdbb, &result, impure);

	return &impure->vlu_desc;
}


/* As seen in blr.h; keep this array "extractParts" in sync.
#define blr_extract_year		(unsigned char)0
#define blr_extract_month		(unsigned char)1
#define blr_extract_day			(unsigned char)2
#define blr_extract_hour		(unsigned char)3
#define blr_extract_minute		(unsigned char)4
#define blr_extract_second		(unsigned char)5
#define blr_extract_weekday		(unsigned char)6
#define blr_extract_yearday		(unsigned char)7
#define blr_extract_millisecond	(unsigned char)8
#define blr_extract_week		(unsigned char)9
*/

const char* extractParts[10] =
{
	"YEAR", "MONTH", "DAY", "HOUR", "MINUTE", "SECOND", "WEEKDAY", "YEARDAY", "MILLISECOND", "WEEK"
};

const char* getPartName(int n)
{
	if (n < 0 || n >= FB_NELEM(extractParts))
		return "Unknown";

	return extractParts[n];
}


dsc* evlDateAdd(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 3);

	jrd_req* request = tdbb->getRequest();

	const dsc* quantityDsc = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if quantityDsc is NULL
		return NULL;

	const dsc* partDsc = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if partDsc is NULL
		return NULL;

	const dsc* valueDsc = EVL_expr(tdbb, args->nod_arg[2]);
	if (request->req_flags & req_null)	// return NULL if valueDsc is NULL
		return NULL;

	const SLONG part = MOV_get_long(partDsc, 0);

	TimeStamp timestamp;

	switch (valueDsc->dsc_dtype)
	{
		case dtype_sql_time:
			timestamp.value().timestamp_time = *(GDS_TIME*) valueDsc->dsc_address;

			if (part != blr_extract_hour &&
				part != blr_extract_minute &&
				part != blr_extract_second &&
				part != blr_extract_millisecond)
			{
				status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
											Arg::Gds(isc_sysf_invalid_addpart_time) <<
												Arg::Str(function->name));
			}
			break;

		case dtype_sql_date:
			timestamp.value().timestamp_date = *(GDS_DATE*) valueDsc->dsc_address;
			timestamp.value().timestamp_time = 0;
			/*
			if (part == blr_extract_hour ||
				part == blr_extract_minute ||
				part == blr_extract_second ||
				part == blr_extract_millisecond)
			{
				status_exception::raise(Arg::Gds(isc_expression_eval_err));
			}
			*/
			break;

		case dtype_timestamp:
			timestamp.value() = *(GDS_TIMESTAMP*) valueDsc->dsc_address;
			break;

		default:
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_invalid_add_datetime) <<
											Arg::Str(function->name));
			break;
	}

	const SLONG quantity = MOV_get_long(quantityDsc, 0);

	switch (part)
	{
		// TO DO: detect overflow in the following cases.

		case blr_extract_year:
			{
				tm times;
				timestamp.decode(&times);
				times.tm_year += quantity;
				timestamp.encode(&times);

				int day = times.tm_mday;
				timestamp.decode(&times);

				if (times.tm_mday != day)
					--timestamp.value().timestamp_date;
			}
			break;

		case blr_extract_month:
			{
				tm times;
				timestamp.decode(&times);

				int md[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

				const int y = quantity / 12;
				const int m = quantity % 12;

				const int ld = md[times.tm_mon] - times.tm_mday;
				const int lm = times.tm_mon;
				times.tm_year += y;

				if ((times.tm_mon += m) > 11)
				{
					times.tm_year++;
					times.tm_mon -= 12;
				}
				else if (times.tm_mon < 0)
				{
					times.tm_year--;
					times.tm_mon += 12;
				}

				const int ly = times.tm_year + 1900;

				if (ly % 4 == 0 && ly % 100 != 0 || ly % 400 == 0)
					md[1]++;

				if (y >= 0 && m >= 0 && times.tm_mday > md[lm])
					times.tm_mday = md[times.tm_mon] - ld;

				if (times.tm_mday > md[times.tm_mon])
					times.tm_mday = md[times.tm_mon];
				else if (times.tm_mday < 1)
					times.tm_mday = 1;

				timestamp.encode(&times);
			}
			break;

		case blr_extract_day:
			timestamp.value().timestamp_date += quantity;
			break;

		case blr_extract_week:
			timestamp.value().timestamp_date += quantity * 7;
			break;

		case blr_extract_hour:
			if (valueDsc->dsc_dtype == dtype_sql_date)
				timestamp.value().timestamp_date += quantity / 24;
			else
				add10msec(&timestamp.value(), quantity, 3600 * ISC_TIME_SECONDS_PRECISION);
			break;

		case blr_extract_minute:
			if (valueDsc->dsc_dtype == dtype_sql_date)
				timestamp.value().timestamp_date += quantity / 1440; // 1440 == 24 * 60
			else
				add10msec(&timestamp.value(), quantity, 60 * ISC_TIME_SECONDS_PRECISION);
			break;

		case blr_extract_second:
			if (valueDsc->dsc_dtype == dtype_sql_date)
				timestamp.value().timestamp_date += quantity / oneDay;
			else
				add10msec(&timestamp.value(), quantity, ISC_TIME_SECONDS_PRECISION);
			break;

		case blr_extract_millisecond:
			if (valueDsc->dsc_dtype == dtype_sql_date)
				timestamp.value().timestamp_date += quantity / (oneDay * 1000);
			else
				add10msec(&timestamp.value(), quantity, ISC_TIME_SECONDS_PRECISION / 1000);
			break;

		default:
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_invalid_addpart_dtime) <<
											Arg::Str(getPartName(part)) <<
											Arg::Str(function->name));
			break;
	}

	EVL_make_value(tdbb, valueDsc, impure);

	switch (impure->vlu_desc.dsc_dtype)
	{
		case dtype_sql_time:
			impure->vlu_misc.vlu_sql_time = timestamp.value().timestamp_time;
			break;

		case dtype_sql_date:
			impure->vlu_misc.vlu_sql_date = timestamp.value().timestamp_date;
			break;

		case dtype_timestamp:
			impure->vlu_misc.vlu_timestamp = timestamp.value();
			break;

		default:
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_invalid_add_dtime_rc));
			break;
	}

	return &impure->vlu_desc;
}


dsc* evlDateDiff(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 3);

	jrd_req* request = tdbb->getRequest();

	const dsc* partDsc = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if partDsc is NULL
		return NULL;

	const dsc* value1Dsc = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if value1Dsc is NULL
		return NULL;

	const dsc* value2Dsc = EVL_expr(tdbb, args->nod_arg[2]);
	if (request->req_flags & req_null)	// return NULL if value2Dsc is NULL
		return NULL;

	TimeStamp timestamp1;

	switch (value1Dsc->dsc_dtype)
	{
		case dtype_sql_time:
			timestamp1.value().timestamp_time = *(GDS_TIME *) value1Dsc->dsc_address;
			timestamp1.value().timestamp_date = 0;
			break;

		case dtype_sql_date:
			timestamp1.value().timestamp_date = *(GDS_DATE *) value1Dsc->dsc_address;
			timestamp1.value().timestamp_time = 0;
			break;

		case dtype_timestamp:
			timestamp1.value() = *(GDS_TIMESTAMP *) value1Dsc->dsc_address;
			break;

		default:
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_invalid_diff_dtime) <<
											Arg::Str(function->name));
			break;
	}

	TimeStamp timestamp2;

	switch (value2Dsc->dsc_dtype)
	{
		case dtype_sql_time:
			timestamp2.value().timestamp_time = *(GDS_TIME *) value2Dsc->dsc_address;
			timestamp2.value().timestamp_date = 0;
			break;

		case dtype_sql_date:
			timestamp2.value().timestamp_date = *(GDS_DATE *) value2Dsc->dsc_address;
			timestamp2.value().timestamp_time = 0;
			break;

		case dtype_timestamp:
			timestamp2.value() = *(GDS_TIMESTAMP *) value2Dsc->dsc_address;
			break;

		default:
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_invalid_diff_dtime) <<
											Arg::Str(function->name));
			break;
	}

	tm times1, times2;
	timestamp1.decode(&times1);
	timestamp2.decode(&times2);

	const SLONG part = MOV_get_long(partDsc, 0);

	switch (part)
	{
		case blr_extract_hour:
			times1.tm_min = 0;
			times2.tm_min = 0;
			// fall through

		case blr_extract_minute:
			times1.tm_sec = 0;
			times2.tm_sec = 0;
			// fall through

		case blr_extract_second:
			timestamp1.encode(&times1);
			timestamp2.encode(&times2);
			break;
	}

	// ASF: throw error if at least one value is "incomplete" from the EXTRACT POV
	switch (part)
	{
		case blr_extract_year:
		case blr_extract_month:
		case blr_extract_day:
		case blr_extract_week:
			if (value1Dsc->dsc_dtype == dtype_sql_time || value2Dsc->dsc_dtype == dtype_sql_time)
			{
				status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
											Arg::Gds(isc_sysf_invalid_timediff) <<
												Arg::Str(function->name));
			}
			break;

		case blr_extract_hour:
		case blr_extract_minute:
		case blr_extract_second:
		case blr_extract_millisecond:
			{
				//if (value1Dsc->dsc_dtype == dtype_sql_date || value2Dsc->dsc_dtype == dtype_sql_date)
				//	status_exception::raise(Arg::Gds(isc_expression_eval_err));

				// ASF: also throw error if one value is TIMESTAMP and the other is TIME
				// CVC: Or if one value is DATE and the other is TIME.
				const int type1 = value1Dsc->dsc_dtype;
				const int type2 = value2Dsc->dsc_dtype;
				if (type1 == dtype_timestamp && type2 == dtype_sql_time ||
					type1 == dtype_sql_time && type2 == dtype_timestamp)
				{
					status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
												Arg::Gds(isc_sysf_invalid_tstamptimediff) <<
													Arg::Str(function->name));
				}
				if (type1 == dtype_sql_date && type2 == dtype_sql_time ||
					type1 == dtype_sql_time && type2 == dtype_sql_date)
				{
					status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
												Arg::Gds(isc_sysf_invalid_datetimediff) <<
													Arg::Str(function->name));
				}
			}
			break;

		default:
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_invalid_diffpart) <<
											Arg::Str(getPartName(part)) <<
											Arg::Str(function->name));
			break;
	}

	SINT64 result = 0;

	switch (part)
	{
		case blr_extract_year:
			result = times2.tm_year - times1.tm_year;
			break;

		case blr_extract_month:
			result = 12 * (times2.tm_year - times1.tm_year);
			result += times2.tm_mon - times1.tm_mon;
			break;

		case blr_extract_day:
			result = timestamp2.value().timestamp_date - timestamp1.value().timestamp_date;
			break;

		case blr_extract_week:
			result = (timestamp2.value().timestamp_date - timestamp1.value().timestamp_date) / 7;
			break;

		// TO DO: detect overflow in the following cases.

		case blr_extract_hour:
			result = SINT64(24) * (timestamp2.value().timestamp_date - timestamp1.value().timestamp_date);
			result += ((SINT64) timestamp2.value().timestamp_time -
				(SINT64) timestamp1.value().timestamp_time) /
				ISC_TIME_SECONDS_PRECISION / 3600;
			break;

		case blr_extract_minute:
			result = SINT64(24) * 60 * (timestamp2.value().timestamp_date - timestamp1.value().timestamp_date);
			result += ((SINT64) timestamp2.value().timestamp_time -
				(SINT64) timestamp1.value().timestamp_time) /
				ISC_TIME_SECONDS_PRECISION / 60;
			break;

		case blr_extract_second:
			result = (SINT64) oneDay *
				(timestamp2.value().timestamp_date - timestamp1.value().timestamp_date);
			result += ((SINT64) timestamp2.value().timestamp_time -
				(SINT64) timestamp1.value().timestamp_time) /
				ISC_TIME_SECONDS_PRECISION;
			break;

		case blr_extract_millisecond:
			result = (SINT64) oneDay *
				(timestamp2.value().timestamp_date - timestamp1.value().timestamp_date) * 1000;
			result += ((SINT64) timestamp2.value().timestamp_time -
				(SINT64) timestamp1.value().timestamp_time) /
				(ISC_TIME_SECONDS_PRECISION / 1000);
			break;

		default:
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_invalid_diffpart) <<
											Arg::Str(getPartName(part)) <<
											Arg::Str(function->name));
			break;
	}

	impure->vlu_misc.vlu_int64 = result;
	impure->vlu_desc.makeInt64(0, &impure->vlu_misc.vlu_int64);

	return &impure->vlu_desc;
}


dsc* evlExp(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	const double rc = exp(MOV_get_double(value));
	if (rc == HUGE_VAL) // unlikely to trap anything
		status_exception::raise(Arg::Gds(isc_arith_except) << Arg::Gds(isc_exception_float_overflow));
	if (isinf(rc))
		status_exception::raise(Arg::Gds(isc_arith_except) << Arg::Gds(isc_exception_float_overflow));

	impure->vlu_misc.vlu_double = rc;
	impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);

	return &impure->vlu_desc;
}


dsc* evlFloor(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	EVL_make_value(tdbb, value, impure);

	switch (impure->vlu_desc.dsc_dtype)
	{
		case dtype_short:
		case dtype_long:
		case dtype_int64:
			{
				SINT64 scale = 1;

				fb_assert(impure->vlu_desc.dsc_scale <= 0);
				for (int i = -impure->vlu_desc.dsc_scale; i > 0; --i)
					scale *= 10;

				const SINT64 v1 = MOV_get_int64(&impure->vlu_desc, impure->vlu_desc.dsc_scale);
				const SINT64 v2 = MOV_get_int64(&impure->vlu_desc, 0) * scale;

				impure->vlu_misc.vlu_int64 = v1 / scale;

				if (v1 < 0 && v1 != v2)
					--impure->vlu_misc.vlu_int64;

				impure->vlu_desc.makeInt64(0, &impure->vlu_misc.vlu_int64);
			}
			break;

		case dtype_real:
			impure->vlu_misc.vlu_float = floor(impure->vlu_misc.vlu_float);
			break;

		default:
			impure->vlu_misc.vlu_double = MOV_get_double(&impure->vlu_desc);
			// fall through

		case dtype_double:
			impure->vlu_misc.vlu_double = floor(impure->vlu_misc.vlu_double);
			impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);
			break;
	}

	return &impure->vlu_desc;
}


dsc* evlGenUuid(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 0);

	FB_GUID guid;
	fb_assert(sizeof(guid.data) == 16);

	GenerateGuid(&guid);

	UCHAR data[16];
	data[0] = (guid.data1 >> 24) & 0xFF;
	data[1] = (guid.data1 >> 16) & 0xFF;
	data[2] = (guid.data1 >> 8) & 0xFF;
	data[3] = guid.data1 & 0xFF;
	data[4] = (guid.data2 >> 8) & 0xFF;
	data[5] = guid.data2 & 0xFF;
	data[6] = (guid.data3 >> 8) & 0xFF;
	data[7] = guid.data3 & 0xFF;
	data[8] = guid.data4[0];
	data[9] = guid.data4[1];
	data[10] = guid.data4[2];
	data[11] = guid.data4[3];
	data[12] = guid.data4[4];
	data[13] = guid.data4[5];
	data[14] = guid.data4[6];
	data[15] = guid.data4[7];

	dsc result;
	result.makeText(16, ttype_binary, data);
	EVL_make_value(tdbb, &result, impure);

	return &impure->vlu_desc;
}


dsc* evlHash(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	impure->vlu_misc.vlu_int64 = 0;

	UCHAR* address;

	if (value->isBlob())
	{
		UCHAR buffer[BUFFER_LARGE];
		blb* blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction,
			reinterpret_cast<bid*>(value->dsc_address));

		while (!(blob->blb_flags & BLB_eof))
		{
			address = buffer;
			const ULONG length = BLB_get_data(tdbb, blob, address, sizeof(buffer), false);

			for (const UCHAR* end = address + length; address < end; ++address)
			{
				impure->vlu_misc.vlu_int64 = (impure->vlu_misc.vlu_int64 << 4) + *address;

				const SINT64 n = impure->vlu_misc.vlu_int64 & CONST64(0xF000000000000000);
				if (n)
					impure->vlu_misc.vlu_int64 ^= n >> 56;
				impure->vlu_misc.vlu_int64 &= ~n;
			}
		}

		BLB_close(tdbb, blob);
	}
	else
	{
		MoveBuffer buffer;
		const ULONG length = MOV_make_string2(tdbb, value, value->getTextType(), &address, buffer, false);

		for (const UCHAR* end = address + length; address < end; ++address)
		{
			impure->vlu_misc.vlu_int64 = (impure->vlu_misc.vlu_int64 << 4) + *address;

			const SINT64 n = impure->vlu_misc.vlu_int64 & CONST64(0xF000000000000000);
			if (n)
				impure->vlu_misc.vlu_int64 ^= n >> 56;
			impure->vlu_misc.vlu_int64 &= ~n;
		}
	}

	// make descriptor for return value
	impure->vlu_desc.makeInt64(0, &impure->vlu_misc.vlu_int64);

	return &impure->vlu_desc;
}


dsc* evlLeft(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 2);

	jrd_req* request = tdbb->getRequest();

	dsc* str = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if str is NULL
		return NULL;

	const dsc* len = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if len is NULL
		return NULL;

	SLONG start = 0;
	dsc startDsc;
	startDsc.makeLong(0, &start);

	return SysFunction::substring(tdbb, impure, str, &startDsc, len);
}


dsc* evlLnLog10(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);
	fb_assert(function->misc != NULL);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	const double v = MOV_get_double(value);

	if (v <= 0)
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argmustbe_positive) <<
										Arg::Str(function->name));
	}

	double rc;

	switch ((Function)(IPTR) function->misc)
	{
	case funLnat:
		rc = log(v);
		break;
	case funLog10:
		rc = log10(v);
		break;
	default:
		fb_assert(0);
	}

	impure->vlu_misc.vlu_double = rc;
	impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);

	return &impure->vlu_desc;
}


dsc* evlLog(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 2);

	jrd_req* request = tdbb->getRequest();

	const dsc* value1 = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value1 is NULL
		return NULL;

	const dsc* value2 = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if value2 is NULL
		return NULL;

	const double v1 = MOV_get_double(value1);
	const double v2 = MOV_get_double(value2);

	if (v1 <= 0)
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_basemustbe_positive) <<
										Arg::Str(function->name));
	}

	if (v2 <= 0)
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argmustbe_positive) <<
										Arg::Str(function->name));
	}

	impure->vlu_misc.vlu_double = log(v2) / log(v1);
	impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);

	return &impure->vlu_desc;
}


dsc* evlMaxMinValue(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value*)
{
	fb_assert(args->nod_count >= 1);
	fb_assert(function->misc != NULL);

	jrd_req* request = tdbb->getRequest();
	dsc* result = NULL;

	for (int i = 0; i < args->nod_count; ++i)
	{
		dsc* value = EVL_expr(tdbb, args->nod_arg[i]);
		if (request->req_flags & req_null)	// return NULL if value is NULL
			return NULL;

		if (i == 0)
			result = value;
		else
		{
			switch ((Function)(IPTR) function->misc)
			{
				case funMaxValue:
					if (MOV_compare(value, result) > 0)
						result = value;
					break;

				case funMinValue:
					if (MOV_compare(value, result) < 0)
						result = value;
					break;

				default:
					fb_assert(false);
			}
		}
	}

	return result;
}


dsc* evlMod(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 2);

	jrd_req* request = tdbb->getRequest();

	const dsc* value1 = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value1 is NULL
		return NULL;

	const dsc* value2 = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if value2 is NULL
		return NULL;

	EVL_make_value(tdbb, value1, impure);
	impure->vlu_desc.dsc_scale = 0;

	const SINT64 divisor = MOV_get_int64(value2, 0);

	if (divisor == 0)
		status_exception::raise(Arg::Gds(isc_arith_except) << Arg::Gds(isc_exception_integer_divide_by_zero));

	const SINT64 result = MOV_get_int64(value1, 0) % divisor;

	switch (impure->vlu_desc.dsc_dtype)
	{
		case dtype_short:
			impure->vlu_misc.vlu_short = (SSHORT) result;
			break;

		case dtype_long:
			impure->vlu_misc.vlu_long = (SLONG) result;
			break;

		case dtype_int64:
			impure->vlu_misc.vlu_int64 = result;
			break;

		default:
			impure->vlu_misc.vlu_int64 = result;
			impure->vlu_desc.makeInt64(0, &impure->vlu_misc.vlu_int64);
			break;
	}

	return &impure->vlu_desc;
}


dsc* evlOverlay(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count >= 3);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	const dsc* placing = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if placing is NULL
		return NULL;

	const dsc* fromDsc = EVL_expr(tdbb, args->nod_arg[2]);
	if (request->req_flags & req_null)	// return NULL if fromDsc is NULL
		return NULL;

	const dsc* lengthDsc = NULL;
	ULONG length = 0;

	if (args->nod_count >= 4)
	{
		lengthDsc = EVL_expr(tdbb, args->nod_arg[3]);
		if (request->req_flags & req_null)	// return NULL if lengthDsc is NULL
			return NULL;

		const SLONG auxlen = MOV_get_long(lengthDsc, 0);

		if (auxlen < 0)
		{
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_argnmustbe_nonneg) <<
											Arg::Num(4) <<
											Arg::Str(function->name));
		}

		length = auxlen;
	}

	SLONG from = MOV_get_long(fromDsc, 0);

	if (from <= 0)
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argnmustbe_positive) <<
										Arg::Num(3) <<
										Arg::Str(function->name));
	}

	const USHORT resultTextType = DataTypeUtil::getResultTextType(value, placing);
	CharSet* cs = INTL_charset_lookup(tdbb, resultTextType);

	MoveBuffer temp1;
	UCHAR* str1;
	ULONG len1;

	if (value->isBlob())
	{
		Firebird::UCharBuffer bpb;
		BLB_gen_bpb_from_descs(value, &impure->vlu_desc, bpb);

		blb* blob = BLB_open2(tdbb, tdbb->getRequest()->req_transaction,
			reinterpret_cast<bid*>(value->dsc_address), bpb.getCount(), bpb.begin());
		len1 =
			(blob->blb_length / INTL_charset_lookup(tdbb, value->getCharSet())->minBytesPerChar()) *
			cs->maxBytesPerChar();

		str1 = temp1.getBuffer(len1);
		len1 = BLB_get_data(tdbb, blob, str1, len1, true);
	}
	else
		len1 = MOV_make_string2(tdbb, value, resultTextType, &str1, temp1);

	MoveBuffer temp2;
	UCHAR* str2;
	ULONG len2;

	if (placing->isBlob())
	{
		Firebird::UCharBuffer bpb;
		BLB_gen_bpb_from_descs(placing, &impure->vlu_desc, bpb);

		blb* blob = BLB_open2(tdbb, tdbb->getRequest()->req_transaction,
			reinterpret_cast<bid*>(placing->dsc_address), bpb.getCount(), bpb.begin());
		len2 =
			(blob->blb_length / INTL_charset_lookup(tdbb, placing->getCharSet())->minBytesPerChar()) *
			cs->maxBytesPerChar();

		str2 = temp2.getBuffer(len2);
		len2 = BLB_get_data(tdbb, blob, str2, len2, true);
	}
	else
		len2 = MOV_make_string2(tdbb, placing, resultTextType, &str2, temp2);

	from = MIN((ULONG) from, len1 + 1);

	if (lengthDsc == NULL)	// not specified
	{
		if (cs->isMultiByte())
			length = cs->length(len2, str2, true);
		else
			length = len2 / cs->maxBytesPerChar();
	}

	length = MIN(length, len1 - from + 1);

	blb* newBlob = NULL;

	if (!value->isBlob() && !placing->isBlob())
	{
		const SINT64 newlen = (SINT64) len1 - length + len2;
		if (newlen > static_cast<SINT64>(MAX_COLUMN_SIZE - sizeof(USHORT)))
			status_exception::raise(Arg::Gds(isc_arith_except) << Arg::Gds(isc_imp_exc));

		dsc desc;
		desc.makeText(newlen, resultTextType);
		EVL_make_value(tdbb, &desc, impure);
	}
	else
	{
		EVL_make_value(tdbb, (value->isBlob() ? value : placing), impure);
		impure->vlu_desc.setBlobSubType(DataTypeUtil::getResultBlobSubType(value, placing));
		impure->vlu_desc.setTextType(resultTextType);
		newBlob = BLB_create(tdbb, tdbb->getRequest()->req_transaction, &impure->vlu_misc.vlu_bid);
	}

	HalfStaticArray<UCHAR, BUFFER_LARGE> blobBuffer;
	int l1;

	if (newBlob)
	{
		l1 = (from - 1) * cs->maxBytesPerChar();

		if (!cs->isMultiByte())
			BLB_put_data(tdbb, newBlob, str1, l1);
		else
		{
			l1 = cs->substring(len1, str1, l1, blobBuffer.getBuffer(l1), 0, from - 1);

			BLB_put_data(tdbb, newBlob, blobBuffer.begin(), l1);
		}
	}
	else
	{
		l1 = cs->substring(len1, str1, impure->vlu_desc.dsc_length,
			impure->vlu_desc.dsc_address, 0, from - 1);
	}

	int l2;

	if (newBlob)
	{
		BLB_put_data(tdbb, newBlob, str2, len2);

		const ULONG auxlen = len1 - l1;
		if (!cs->isMultiByte())
		{
			BLB_put_data(tdbb, newBlob, str1 + l1 + length * cs->maxBytesPerChar(),
				auxlen - length * cs->maxBytesPerChar());
		}
		else
		{
			l2 = cs->substring(auxlen, str1 + l1, auxlen,
				blobBuffer.getBuffer(auxlen), length, auxlen);
			BLB_put_data(tdbb, newBlob, blobBuffer.begin(), l2);
		}

		BLB_close(tdbb, newBlob);
	}
	else
	{
		memcpy(impure->vlu_desc.dsc_address + l1, str2, len2);
		l2 = cs->substring(len1 - l1, str1 + l1, impure->vlu_desc.dsc_length - len2,
			impure->vlu_desc.dsc_address + l1 + len2, length, len1 - l1);

		impure->vlu_desc.dsc_length = (USHORT) (l1 + len2 + l2);
	}

	return &impure->vlu_desc;
}


dsc* evlPad(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count >= 2);

	jrd_req* request = tdbb->getRequest();

	const dsc* value1 = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value1 is NULL
		return NULL;

	const dsc* padLenDsc = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if padLenDsc is NULL
		return NULL;

	const SLONG padLenArg = MOV_get_long(padLenDsc, 0);
	if (padLenArg < 0)
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argnmustbe_nonneg) <<
										Arg::Num(2) <<
										Arg::Str(function->name));
	}

	ULONG padLen = static_cast<ULONG>(padLenArg);

	const dsc* value2 = NULL;
	if (args->nod_count >= 3)
	{
		value2 = EVL_expr(tdbb, args->nod_arg[2]);
		if (request->req_flags & req_null)	// return NULL if value2 is NULL
			return NULL;
	}

	const USHORT ttype = value1->getTextType();
	CharSet* cs = INTL_charset_lookup(tdbb, ttype);

	MoveBuffer buffer1;
	UCHAR* address1;
	ULONG length1 = MOV_make_string2(tdbb, value1, ttype, &address1, buffer1, false);
	ULONG charLength1 = cs->length(length1, address1, true);

	MoveBuffer buffer2;
	const UCHAR* address2;
	ULONG length2;

	if (value2 == NULL)
	{
		address2 = cs->getSpace();
		length2 = cs->getSpaceLength();
	}
	else
	{
		UCHAR* address2Temp = NULL;
		length2 = MOV_make_string2(tdbb, value2, ttype, &address2Temp, buffer2, false);
		address2 = address2Temp;
	}

	ULONG charLength2 = cs->length(length2, address2, true);

	blb* newBlob = NULL;

	if (value1->isBlob() || (value2 && value2->isBlob()))
	{
		EVL_make_value(tdbb, (value1->isBlob() ? value1 : value2), impure);
		impure->vlu_desc.setBlobSubType(value1->getBlobSubType());
		impure->vlu_desc.setTextType(ttype);
		newBlob = BLB_create(tdbb, tdbb->getRequest()->req_transaction, &impure->vlu_misc.vlu_bid);
	}
	else
	{
		if (padLen * cs->maxBytesPerChar() > MAX_COLUMN_SIZE - sizeof(USHORT))
			status_exception::raise(Arg::Gds(isc_arith_except) << Arg::Gds(isc_imp_exc));

		dsc desc;
		desc.makeText(padLen * cs->maxBytesPerChar(), ttype);
		EVL_make_value(tdbb, &desc, impure);
	}

	MoveBuffer buffer;

	if (charLength1 > padLen)
	{
		if (newBlob)
		{
			buffer.getBuffer(padLen * cs->maxBytesPerChar());
			length1 = cs->substring(length1, address1, buffer.getCapacity(),
				buffer.begin(), 0, padLen);
		}
		else
		{
			length1 = cs->substring(length1, address1, impure->vlu_desc.dsc_length,
				impure->vlu_desc.dsc_address, 0, padLen);
		}
		charLength1 = padLen;
	}

	padLen -= charLength1;

	UCHAR* p = impure->vlu_desc.dsc_address;

	if ((Function)(IPTR) function->misc == funRPad)
	{
		if (newBlob)
			BLB_put_data(tdbb, newBlob, address1, length1);
		else
		{
			memcpy(p, address1, length1);
			p += length1;
		}
	}

	for (; charLength2 > 0 && padLen > 0; padLen -= charLength2)
	{
		if (charLength2 <= padLen)
		{
			if (newBlob)
				BLB_put_data(tdbb, newBlob, address2, length2);
			else
			{
				memcpy(p, address2, length2);
				p += length2;
			}
		}
		else
		{
			if (newBlob)
			{
				buffer.getBuffer(padLen * cs->maxBytesPerChar());
				SLONG len = cs->substring(length2, address2, buffer.getCapacity(),
					buffer.begin(), 0, padLen);
				BLB_put_data(tdbb, newBlob, address2, len);
			}
			else
			{
				p += cs->substring(length2, address2,
					impure->vlu_desc.dsc_length - (p - impure->vlu_desc.dsc_address), p, 0, padLen);
			}

			charLength2 = padLen;
		}
	}

	if ((Function)(IPTR) function->misc == funLPad)
	{
		if (newBlob)
			BLB_put_data(tdbb, newBlob, address1, length1);
		else
		{
			memcpy(p, address1, length1);
			p += length1;
		}
	}

	if (newBlob)
		BLB_close(tdbb, newBlob);
	else
		impure->vlu_desc.dsc_length = p - impure->vlu_desc.dsc_address;

	return &impure->vlu_desc;
}


dsc* evlPi(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 0);

	impure->vlu_misc.vlu_double = 3.14159265358979323846;
	impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);

	return &impure->vlu_desc;
}


dsc* evlPosition(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count >= 2);

	jrd_req* request = tdbb->getRequest();

	const dsc* value1 = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value1 is NULL
		return NULL;

	const dsc* value2 = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if value2 is NULL
		return NULL;

	SLONG start = 1;

	if (args->nod_count >= 3)
	{
		const dsc* value3 = EVL_expr(tdbb, args->nod_arg[2]);
		if (request->req_flags & req_null)	// return NULL if value3 is NULL
			return NULL;

		start = MOV_get_long(value3, 0);
		if (start <= 0)
		{
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_argnmustbe_positive) <<
											Arg::Num(3) <<
											Arg::Str(function->name));
		}
	}

	// make descriptor for return value
	impure->vlu_desc.makeLong(0, &impure->vlu_misc.vlu_long);

	// we'll use the collation from the second string
	const USHORT ttype = value2->getTextType();
	TextType* tt = INTL_texttype_lookup(tdbb, ttype);
	CharSet* cs = tt->getCharSet();
	const UCHAR canonicalWidth = tt->getCanonicalWidth();

	MoveBuffer value1Buffer;
	UCHAR* value1Address;
	ULONG value1Length;

	if (value1->isBlob())
	{
		// value1 is a blob
		blb* blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction,
			reinterpret_cast<bid*>(value1->dsc_address));

		value1Address = value1Buffer.getBuffer(blob->blb_length);
		value1Length = BLB_get_data(tdbb, blob, value1Address, blob->blb_length, true);
	}
	else
		value1Length = MOV_make_string2(tdbb, value1, ttype, &value1Address, value1Buffer);

	Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> value1Canonical;
	value1Canonical.getBuffer(value1Length / cs->minBytesPerChar() * canonicalWidth);
	const SLONG value1CanonicalLen = tt->canonical(value1Length, value1Address,
		value1Canonical.getCount(), value1Canonical.begin()) * canonicalWidth;

	// If the first string is empty, we should return the start position accordingly to the SQL2003
	// standard. Using the same logic with our "start" parameter (an extension to the standard),
	// we should return it if it's >= 1 and <= (the other string length + 1). Otherwise, return 0.
	if (value1CanonicalLen == 0 && start == 1)
	{
		impure->vlu_misc.vlu_long = 1;
		return &impure->vlu_desc;
	}

	MoveBuffer value2Buffer;
	UCHAR* value2Address;
	ULONG value2Length;

	if (value2->isBlob())
	{
		// value2 is a blob
		blb* blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction,
			reinterpret_cast<bid*>(value2->dsc_address));

		value2Address = value2Buffer.getBuffer(blob->blb_length);
		value2Length = BLB_get_data(tdbb, blob, value2Address, blob->blb_length, true);
	}
	else
		value2Length = MOV_make_string2(tdbb, value2, ttype, &value2Address, value2Buffer);

	Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> value2Canonical;
	value2Canonical.getBuffer(value2Length / cs->minBytesPerChar() * canonicalWidth);
	const SLONG value2CanonicalLen = tt->canonical(value2Length, value2Address,
		value2Canonical.getCount(), value2Canonical.begin()) * canonicalWidth;

	if (value1CanonicalLen == 0)
	{
		impure->vlu_misc.vlu_long = (start <= value2CanonicalLen / canonicalWidth + 1) ? start : 0;
		return &impure->vlu_desc;
	}

	// if the second string is empty, first one is not inside it
	if (value2CanonicalLen == 0)
	{
		impure->vlu_misc.vlu_long = 0;
		return &impure->vlu_desc;
	}

	// search if value1 is inside value2
	const UCHAR* const end = value2Canonical.begin() + value2CanonicalLen;

	for (const UCHAR* p = value2Canonical.begin() + (start - 1) * canonicalWidth;
		 p + value1CanonicalLen <= end;
		 p += canonicalWidth)
	{
		if (memcmp(p, value1Canonical.begin(), value1CanonicalLen) == 0)
		{
			impure->vlu_misc.vlu_long = ((p - value2Canonical.begin()) / canonicalWidth) + 1;
			return &impure->vlu_desc;
		}
	}

	// value1 isn't inside value2
	impure->vlu_misc.vlu_long = 0;
	return &impure->vlu_desc;
}


dsc* evlPower(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 2);

	jrd_req* request = tdbb->getRequest();

	const dsc* value1 = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value1 is NULL
		return NULL;

	const dsc* value2 = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if value2 is NULL
		return NULL;

	impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);

	const double v1 = MOV_get_double(value1);
	const double v2 = MOV_get_double(value2);

	if (v1 == 0 && v2 < 0)
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_invalid_zeropowneg) <<
										Arg::Str(function->name));
	}

	if (v1 < 0 &&
		(!value2->isExact() ||
		 MOV_get_int64(value2, 0) * SINT64(CVT_power_of_ten(-value2->dsc_scale)) !=
			MOV_get_int64(value2, value2->dsc_scale)))
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_invalid_negpowfp) <<
										Arg::Str(function->name));
	}

	const double rc = pow(v1, v2);
	if (isinf(rc))
		status_exception::raise(Arg::Gds(isc_arith_except) << Arg::Gds(isc_exception_float_overflow));

	impure->vlu_misc.vlu_double = rc;

	return &impure->vlu_desc;
}


dsc* evlRand(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 0);

	SINT64 n;
	tdbb->getAttachment()->att_random_generator.getBytes(&n, sizeof(n));
	n &= QUADCONST(0x7FFFFFFFFFFFFFFF);	// remove the sign

	impure->vlu_misc.vlu_double = (double) n / MAX_SINT64;
	impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);

	return &impure->vlu_desc;
}


dsc* evlReplace(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 3);

	jrd_req* request = tdbb->getRequest();
	dsc* values[3];	// 0 = searched, 1 = find, 2 = replacement
	const dsc* firstBlob = NULL;

	for (int i = 0; i < 3; ++i)
	{
		values[i] = EVL_expr(tdbb, args->nod_arg[i]);
		if (request->req_flags & req_null)	// return NULL if values[i] is NULL
			return NULL;

		if (!firstBlob && values[i]->isBlob())
			firstBlob = values[i];
	}

	const USHORT ttype = values[0]->getTextType();
	TextType* tt = INTL_texttype_lookup(tdbb, ttype);
	CharSet* cs = tt->getCharSet();
	const UCHAR canonicalWidth = tt->getCanonicalWidth();

	MoveBuffer buffers[3];
	UCHAR* addresses[3];
	ULONG lengths[3];

	for (int i = 0; i < 3; ++i)
	{
		if (values[i]->isBlob())
		{
			// values[i] is a blob
			blb* blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction,
				reinterpret_cast<bid*>(values[i]->dsc_address));

			addresses[i] = buffers[i].getBuffer(blob->blb_length);
			lengths[i] = BLB_get_data(tdbb, blob, addresses[i], blob->blb_length, true);
		}
		else
			lengths[i] = MOV_make_string2(tdbb, values[i], ttype, &addresses[i], buffers[i]);
	}

	if (lengths[1] == 0)
		return values[0];

	Firebird::HalfStaticArray<UCHAR, BUFFER_SMALL> canonicals[2];	// searched, find
	for (int i = 0; i < 2; ++i)
	{
		canonicals[i].getBuffer(lengths[i] / cs->minBytesPerChar() * canonicalWidth);
		canonicals[i].resize(tt->canonical(lengths[i], addresses[i],
			canonicals[i].getCount(), canonicals[i].begin()) * canonicalWidth);
	}

	blb* newBlob = NULL;

	// make descriptor for return value
	if (!firstBlob)
	{
		const unsigned int searchedLen = canonicals[0].getCount() / canonicalWidth;
		const unsigned int findLen = canonicals[1].getCount() / canonicalWidth;
		const unsigned int replacementLen = lengths[2] / cs->minBytesPerChar();

		const USHORT len = MIN(MAX_COLUMN_SIZE, cs->maxBytesPerChar() *
			MAX(searchedLen, searchedLen + (searchedLen / findLen) * (replacementLen - findLen)));

		dsc desc;
		desc.makeText(len, ttype);
		EVL_make_value(tdbb, &desc, impure);
	}
	else
	{
		EVL_make_value(tdbb, firstBlob, impure);
		impure->vlu_desc.setBlobSubType(values[0]->getBlobSubType());
		impure->vlu_desc.setTextType(ttype);
		newBlob = BLB_create(tdbb, tdbb->getRequest()->req_transaction, &impure->vlu_misc.vlu_bid);
	}

	// search 'find' in 'searched'
	bool finished = false;
	const UCHAR* const end = canonicals[0].begin() + canonicals[0].getCount();
	const UCHAR* srcPos = addresses[0];
	UCHAR* dstPos = (newBlob ? NULL : impure->vlu_desc.dsc_address);
	MoveBuffer buffer;
	const UCHAR* last;

	for (const UCHAR* p = last = canonicals[0].begin();
		 !finished || (p + canonicals[1].getCount() <= end);
		 p += canonicalWidth)
	{
		if (p + canonicals[1].getCount() > end)
		{
			finished = true;
			p = canonicals[0].end();
		}

		if (finished || memcmp(p, canonicals[1].begin(), canonicals[1].getCount()) == 0)
		{
			int len;

			if (newBlob)
			{
				len = ((p - last) / canonicalWidth) * cs->maxBytesPerChar();

				if (cs->isMultiByte())
				{
					buffer.getBuffer(len);
					len = cs->substring(addresses[0] + lengths[0] - srcPos, srcPos,
						buffer.getCapacity(), buffer.begin(), 0, (p - last) / canonicalWidth);

					BLB_put_data(tdbb, newBlob, buffer.begin(), len);
				}
				else
					BLB_put_data(tdbb, newBlob, srcPos, len);

				if (!finished)
					BLB_put_data(tdbb, newBlob, addresses[2], lengths[2]);
			}
			else
			{
				len = cs->substring(addresses[0] + lengths[0] - srcPos, srcPos,
					(impure->vlu_desc.dsc_address + impure->vlu_desc.dsc_length) - dstPos, dstPos,
					0, (p - last) / canonicalWidth);

				dstPos += len;

				if (!finished)
				{
					memcpy(dstPos, addresses[2], lengths[2]);
					dstPos += lengths[2];
				}
			}

			srcPos += len + lengths[1];

			last = p + canonicals[1].getCount();
			p += canonicals[1].getCount() - canonicalWidth;
		}
	}

	if (newBlob)
		BLB_close(tdbb, newBlob);
	else
		impure->vlu_desc.dsc_length = dstPos - impure->vlu_desc.dsc_address;

	return &impure->vlu_desc;
}


dsc* evlReverse(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	CharSet* cs = INTL_charset_lookup(tdbb, value->getCharSet());

	if (value->isBlob())
	{
		blb* blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction,
			reinterpret_cast<bid*>(value->dsc_address));

		Firebird::HalfStaticArray<UCHAR, BUFFER_LARGE> buffer;
		Firebird::HalfStaticArray<UCHAR, BUFFER_LARGE> buffer2;

		UCHAR* p = buffer.getBuffer(blob->blb_length);
		const SLONG len = BLB_get_data(tdbb, blob, p, blob->blb_length, true);

		if (cs->isMultiByte() || cs->minBytesPerChar() > 1)
		{
			const UCHAR* p1 = p;
			UCHAR* p2 = buffer2.getBuffer(len) + len;
			const UCHAR* const end = p1 + len;
			ULONG size = 0;

			while (p2 > buffer2.begin())
			{
#ifdef DEV_BUILD
				const bool read =
#endif
					IntlUtil::readOneChar(cs, &p1, end, &size);
				fb_assert(read == true);
				memcpy(p2 -= size, p1, size);
			}

			p = p2;
		}
		else
		{
			for (UCHAR* p2 = p + len - 1; p2 >= p; p++, p2--)
			{
				const UCHAR c = *p;
				*p = *p2;
				*p2 = c;
			}

			p = buffer.begin();
		}

		EVL_make_value(tdbb, value, impure);

		blb* newBlob = BLB_create(tdbb, tdbb->getRequest()->req_transaction,
			&impure->vlu_misc.vlu_bid);
		BLB_put_data(tdbb, newBlob, p, len);
		BLB_close(tdbb, newBlob);
	}
	else
	{
		MoveBuffer temp;
		UCHAR* p;
		const int len = MOV_make_string2(tdbb, value, value->getTextType(), &p, temp);

		dsc desc;
		desc.makeText(len, value->getTextType());
		EVL_make_value(tdbb, &desc, impure);

		UCHAR* p2 = impure->vlu_desc.dsc_address + impure->vlu_desc.dsc_length;

		if (cs->isMultiByte() || cs->minBytesPerChar() > 1)
		{
			const UCHAR* p1 = p;
			const UCHAR* const end = p1 + len;
			ULONG size = 0;

			while (p2 > impure->vlu_desc.dsc_address)
			{
#ifdef DEV_BUILD
				const bool read =
#endif
					IntlUtil::readOneChar(cs, &p1, end, &size);
				fb_assert(read == true);
				memcpy(p2 -= size, p1, size);
			}
		}
		else
		{
			while (p2 > impure->vlu_desc.dsc_address)
				*--p2 = *p++;
		}
	}

	return &impure->vlu_desc;
}


dsc* evlRight(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 2);

	jrd_req* request = tdbb->getRequest();

	dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	dsc* len = EVL_expr(tdbb, args->nod_arg[1]);
	if (request->req_flags & req_null)	// return NULL if len is NULL
		return NULL;

	CharSet* charSet = INTL_charset_lookup(tdbb, value->getCharSet());
	SLONG start;

	if (value->isBlob())
	{
		blb* blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction,
			reinterpret_cast<bid*>(value->dsc_address));

		if (charSet->isMultiByte())
		{
			HalfStaticArray<UCHAR, BUFFER_LARGE> buffer;

			SLONG length = BLB_get_data(tdbb, blob, buffer.getBuffer(blob->blb_length),
				blob->blb_length, false);
			start = charSet->length(length, buffer.begin(), true);
		}
		else
			start = blob->blb_length / charSet->maxBytesPerChar();

		BLB_close(tdbb, blob);
	}
	else
	{
		MoveBuffer temp;
		UCHAR* p;
		start = MOV_make_string2(tdbb, value, value->getTextType(), &p, temp);
		start = charSet->length(start, p, true);
	}

	start -= MOV_get_long(len, 0);
	start = MAX(0, start);

	dsc startDsc;
	startDsc.makeLong(0, &start);

	return SysFunction::substring(tdbb, impure, value, &startDsc, len);
}


dsc* evlRound(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count >= 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	SLONG scale = 0;

	if (args->nod_count > 1)
	{
		const dsc* scaleDsc = EVL_expr(tdbb, args->nod_arg[1]);
		if (request->req_flags & req_null)	// return NULL if scaleDsc is NULL
			return NULL;

		scale = -MOV_get_long(scaleDsc, 0);
		if (!(scale >= MIN_SCHAR && scale <= MAX_SCHAR))
		{
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_invalid_scale) <<
											Arg::Str(function->name));
		}
	}

	impure->vlu_misc.vlu_int64 = MOV_get_int64(value, scale);
	impure->vlu_desc.makeInt64(scale, &impure->vlu_misc.vlu_int64);

	return &impure->vlu_desc;
}


dsc* evlSign(Jrd::thread_db* tdbb, const SysFunction*, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	const double val = MOV_get_double(value);

	if (val > 0)
		impure->vlu_misc.vlu_short = 1;
	else if (val < 0)
		impure->vlu_misc.vlu_short = -1;
	else	// val == 0
		impure->vlu_misc.vlu_short = 0;

	impure->vlu_desc.makeShort(0, &impure->vlu_misc.vlu_short);

	return &impure->vlu_desc;
}


dsc* evlSqrt(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	impure->vlu_misc.vlu_double = MOV_get_double(value);

	if (impure->vlu_misc.vlu_double < 0)
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_argmustbe_nonneg) << Arg::Str(function->name));
	}

	impure->vlu_misc.vlu_double = sqrt(impure->vlu_misc.vlu_double);
	impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);

	return &impure->vlu_desc;
}


dsc* evlTrunc(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count >= 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	SLONG resultScale = 0;
	if (args->nod_count > 1)
	{
		const dsc* scaleDsc = EVL_expr(tdbb, args->nod_arg[1]);
		if (request->req_flags & req_null)	// return NULL if scaleDsc is NULL
			return NULL;

		resultScale = -MOV_get_long(scaleDsc, 0);
		if (!(resultScale >= MIN_SCHAR && resultScale <= MAX_SCHAR))
		{
			status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
										Arg::Gds(isc_sysf_invalid_scale) <<
											Arg::Str(function->name));
		}
	}

	if (value->isExact())
	{
		SSHORT scale = value->dsc_scale;
		impure->vlu_misc.vlu_int64 = MOV_get_int64(value, scale);

		if (resultScale < scale)
			resultScale = scale;

		scale -= resultScale;

		if (scale < 0)
		{
			while (scale)
			{
				impure->vlu_misc.vlu_int64 /= 10;
				++scale;
			}
		}

		impure->vlu_desc.makeInt64(resultScale, &impure->vlu_misc.vlu_int64);
	}
	else
	{
		impure->vlu_misc.vlu_double = MOV_get_double(value);

		SINT64 v = 1;

		if (resultScale > 0)
		{
			while (resultScale > 0)
			{
				v *= 10;
				--resultScale;
			}

			impure->vlu_misc.vlu_double /= v;
			modf(impure->vlu_misc.vlu_double, &impure->vlu_misc.vlu_double);
			impure->vlu_misc.vlu_double *= v;
		}
		else
		{
			double r = modf(impure->vlu_misc.vlu_double, &impure->vlu_misc.vlu_double);

			if (resultScale != 0)
			{
				for (SLONG i = 0; i > resultScale; --i)
					v *= 10;

				modf(r * v, &r);
				impure->vlu_misc.vlu_double += r / v;
			}
		}

		impure->vlu_desc.makeDouble(&impure->vlu_misc.vlu_double);
	}

	return &impure->vlu_desc;
}


dsc* evlUuidToChar(Jrd::thread_db* tdbb, const SysFunction* function, Jrd::jrd_nod* args,
	Jrd::impure_value* impure)
{
	fb_assert(args->nod_count == 1);

	jrd_req* request = tdbb->getRequest();

	const dsc* value = EVL_expr(tdbb, args->nod_arg[0]);
	if (request->req_flags & req_null)	// return NULL if value is NULL
		return NULL;

	if (!value->isText())
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_binuuid_mustbe_str) <<
										Arg::Str(function->name));
	}

	USHORT ttype;
	UCHAR* data;
	const USHORT len = CVT_get_string_ptr(value, &ttype, &data, NULL, 0);

	if (len != sizeof(FB_GUID))
	{
		status_exception::raise(Arg::Gds(isc_expression_eval_err) <<
									Arg::Gds(isc_sysf_binuuid_wrongsize) <<
										Arg::Num(sizeof(FB_GUID)) <<
										Arg::Str(function->name));
	}

	char buffer[GUID_BUFF_SIZE];
	sprintf(buffer, GUID_NEW_FORMAT,
		USHORT(data[0]), USHORT(data[1]), USHORT(data[2]), USHORT(data[3]), USHORT(data[4]),
		USHORT(data[5]), USHORT(data[6]), USHORT(data[7]), USHORT(data[8]), USHORT(data[9]),
		USHORT(data[10]), USHORT(data[11]), USHORT(data[12]), USHORT(data[13]), USHORT(data[14]),
		USHORT(data[15]));

	dsc result;
	result.makeText(GUID_BODY_SIZE, ttype_ascii, reinterpret_cast<UCHAR*>(buffer) + 1);
	EVL_make_value(tdbb, &result, impure);

	return &impure->vlu_desc;
}

} // anonymous namespace



const SysFunction SysFunction::functions[] =
	{
		{"ABS", 1, 1, setParamsDouble, makeAbs, evlAbs, NULL},
		{"ACOS", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfAcos},
		{"ACOSH", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfAcosh},
		{"ASCII_CHAR", 1, 1, setParamsInteger, makeAsciiChar, evlAsciiChar, NULL},
		{"ASCII_VAL", 1, 1, setParamsAsciiVal, makeShortResult, evlAsciiVal, NULL},
		{"ASIN", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfAsin},
		{"ASINH", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfAsinh},
		{"ATAN", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfAtan},
		{"ATANH", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfAtanh},
		{"ATAN2", 2, 2, setParamsDouble, makeDoubleResult, evlAtan2, NULL},
		{"BIN_AND", 2, -1, setParamsInteger, makeBin, evlBin, (void*) funBinAnd},
		{"BIN_NOT", 1, 1, setParamsInteger, makeBin, evlBin, (void*) funBinNot},
		{"BIN_OR", 2, -1, setParamsInteger, makeBin, evlBin, (void*) funBinOr},
		{"BIN_SHL", 2, 2, setParamsInteger, makeBinShift, evlBinShift, (void*) funBinShl},
		{"BIN_SHR", 2, 2, setParamsInteger, makeBinShift, evlBinShift, (void*) funBinShr},
		{"BIN_SHL_ROT", 2, 2, setParamsInteger, makeBinShift, evlBinShift, (void*) funBinShlRot},
		{"BIN_SHR_ROT", 2, 2, setParamsInteger, makeBinShift, evlBinShift, (void*) funBinShrRot},
		{"BIN_XOR", 2, -1, setParamsInteger, makeBin, evlBin, (void*) funBinXor},
		{"CEIL", 1, 1, setParamsDouble, makeCeilFloor, evlCeil, NULL},
		{"CEILING", 1, 1, setParamsDouble, makeCeilFloor, evlCeil, NULL},
		{"CHAR_TO_UUID", 1, 1, setParamsCharToUuid, makeUuid, evlCharToUuid, NULL},
		{"COS", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfCos},
		{"COSH", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfCosh},
		{"COT", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfCot},
		{"DATEADD", 3, 3, setParamsDateAdd, makeDateAdd, evlDateAdd, NULL},
		{"DATEDIFF", 3, 3, setParamsDateDiff, makeInt64Result, evlDateDiff, NULL},
		{"EXP", 1, 1, setParamsDouble, makeDoubleResult, evlExp, NULL},
		{"FLOOR", 1, 1, setParamsDouble, makeCeilFloor, evlFloor, NULL},
		{"GEN_UUID", 0, 0, NULL, makeUuid, evlGenUuid, NULL},
		{"HASH", 1, 1, NULL, makeInt64Result, evlHash, NULL},
		{"LEFT", 2, 2, setParamsSecondInteger, makeLeftRight, evlLeft, NULL},
		{"LN", 1, 1, setParamsDouble, makeDoubleResult, evlLnLog10, (void*) funLnat},
		{"LOG", 2, 2, setParamsDouble, makeDoubleResult, evlLog, NULL},
		{"LOG10", 1, 1, setParamsDouble, makeDoubleResult, evlLnLog10, (void*) funLog10},
		{"LPAD", 2, 3, setParamsSecondInteger, makePad, evlPad, (void*) funLPad},
		{"MAXVALUE", 1, -1, setParamsFromList, makeFromListResult, evlMaxMinValue, (void*) funMaxValue},
		{"MINVALUE", 1, -1, setParamsFromList, makeFromListResult, evlMaxMinValue, (void*) funMinValue},
		{"MOD", 2, 2, setParamsFromList, makeMod, evlMod, NULL},
		{"OVERLAY", 3, 4, setParamsOverlay, makeOverlay, evlOverlay, NULL},
		{"PI", 0, 0, NULL, makeDoubleResult, evlPi, NULL},
		{"POSITION", 2, 3, setParamsPosition, makeLongResult, evlPosition, NULL},
		{"POWER", 2, 2, setParamsDouble, makeDoubleResult, evlPower, NULL},
		{"RAND", 0, 0, NULL, makeDoubleResult, evlRand, NULL},
		{"REPLACE", 3, 3, setParamsFromList, makeReplace, evlReplace, NULL},
		{"REVERSE", 1, 1, NULL, makeReverse, evlReverse, NULL},
		{"RIGHT", 2, 2, setParamsSecondInteger, makeLeftRight, evlRight, NULL},
		{"ROUND", 1, 2, setParamsRoundTrunc, makeRound, evlRound, NULL},
		{"RPAD", 2, 3, setParamsSecondInteger, makePad, evlPad, (void*) funRPad},
		{"SIGN", 1, 1, setParamsDouble, makeShortResult, evlSign, NULL},
		{"SIN", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfSin},
		{"SINH", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfSinh},
		{"SQRT", 1, 1, setParamsDouble, makeDoubleResult, evlSqrt, NULL},
		{"TAN", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfTan},
		{"TANH", 1, 1, setParamsDouble, makeDoubleResult, evlStdMath, (void*) trfTanh},
		{"TRUNC", 1, 2, setParamsRoundTrunc, makeTrunc, evlTrunc, NULL},
		{"UUID_TO_CHAR", 1, 1, setParamsUuidToChar, makeUuidToChar, evlUuidToChar, NULL},
		{"", 0, 0, NULL, NULL, NULL, NULL}
	};


const SysFunction* SysFunction::lookup(const Firebird::MetaName& name)
{
	for (const SysFunction* f = functions; f->name.length() > 0; ++f)
	{
		if (f->name == name)
			return f;
	}

	return NULL;
}


dsc* SysFunction::substring(thread_db* tdbb, impure_value* impure,
	dsc* value, const dsc* offset_value, const dsc* length_value)
{
/**************************************
 *
 *      s u b s t r i n g
 *
 **************************************
 *
 * Functional description
 *      Perform substring function.
 *
 **************************************/
	SET_TDBB(tdbb);

	const SLONG offset_arg = MOV_get_long(offset_value, 0);
	const SLONG length_arg = MOV_get_long(length_value, 0);

	if (offset_arg < 0)
		status_exception::raise(Arg::Gds(isc_bad_substring_offset) << Arg::Num(offset_arg + 1));
	else if (length_arg < 0)
		status_exception::raise(Arg::Gds(isc_bad_substring_length) << Arg::Num(length_arg));

	dsc desc;
	DataTypeUtil(tdbb).makeSubstr(&desc, value, offset_value, length_value);

	ULONG offset = (ULONG) offset_arg;
	ULONG length = (ULONG) length_arg;

	if (desc.isText() && length > MAX_COLUMN_SIZE)
		length = MAX_COLUMN_SIZE;

	ULONG dataLen;

	if (value->isBlob())
	{
		// Source string is a blob, things get interesting.

		fb_assert(desc.dsc_dtype == dtype_blob);

		desc.dsc_address = (UCHAR*) &impure->vlu_misc.vlu_bid;

		blb* newBlob = BLB_create(tdbb, tdbb->getRequest()->req_transaction, &impure->vlu_misc.vlu_bid);

		blb* blob = BLB_open(tdbb, tdbb->getRequest()->req_transaction,
							reinterpret_cast<bid*>(value->dsc_address));

		Firebird::HalfStaticArray<UCHAR, BUFFER_LARGE> buffer;
		CharSet* charSet = INTL_charset_lookup(tdbb, value->getCharSet());

		const FB_UINT64 byte_offset = FB_UINT64(offset) * charSet->maxBytesPerChar();
		const FB_UINT64 byte_length = FB_UINT64(length) * charSet->maxBytesPerChar();

		if (charSet->isMultiByte())
		{
			buffer.getBuffer(MIN(blob->blb_length, byte_offset + byte_length));
			dataLen = BLB_get_data(tdbb, blob, buffer.begin(), buffer.getCount(), false);

			Firebird::HalfStaticArray<UCHAR, BUFFER_LARGE> buffer2;
			buffer2.getBuffer(dataLen);

			dataLen = charSet->substring(dataLen, buffer.begin(),
				buffer2.getCapacity(), buffer2.begin(), offset, length);
			BLB_put_data(tdbb, newBlob, buffer2.begin(), dataLen);
		}
		else if (byte_offset < blob->blb_length)
		{
			offset = byte_offset;
			length = MIN(blob->blb_length, byte_length);

			while (!(blob->blb_flags & BLB_eof) && offset)
			{
				// Both cases are the same for now. Let's see if we can optimize in the future.
				ULONG l1 = BLB_get_data(tdbb, blob, buffer.begin(),
					MIN(buffer.getCapacity(), offset), false);
				offset -= l1;
			}

			while (!(blob->blb_flags & BLB_eof) && length)
			{
				dataLen = BLB_get_data(tdbb, blob, buffer.begin(),
					MIN(length, buffer.getCapacity()), false);
				length -= dataLen;

				BLB_put_data(tdbb, newBlob, buffer.begin(), dataLen);
			}
		}

		BLB_close(tdbb, blob);
		BLB_close(tdbb, newBlob);

		EVL_make_value(tdbb, &desc, impure);
	}
	else
	{
		fb_assert(desc.isText());

		desc.dsc_dtype = dtype_text;

		// CVC: I didn't bother to define a larger buffer because:
		//		- Native types when converted to string don't reach 31 bytes plus terminator.
		//		- String types do not need and do not use the buffer ("temp") to be pulled.
		//		- The types that can cause an error() issued inside the low level MOV/CVT
		//		routines because the "temp" is not enough are blob and array but at this time
		//		they aren't accepted, so they will cause error() to be called anyway.
		VaryStr<32> temp;
		USHORT ttype;
		desc.dsc_length =
			MOV_get_string_ptr(value, &ttype, &desc.dsc_address, &temp, sizeof(temp));
		desc.setTextType(ttype);

		// CVC: Why bother? If the offset is greater or equal than the length in bytes,
		// it's impossible that the offset be less than the length in an international charset.
		if (offset >= desc.dsc_length || !length)
		{
			desc.dsc_length = 0;
			EVL_make_value(tdbb, &desc, impure);
		}
		// CVC: God save the king if the engine doesn't protect itself against buffer overruns,
		//		because intl.h defines UNICODE as the type of most system relations' string fields.
		//		Also, the field charset can come as 127 (dynamic) when it comes from system triggers,
		//		but it's resolved by INTL_obj_lookup() to UNICODE_FSS in the cases I observed. Here I cannot
		//		distinguish between user calls and system calls. Unlike the original ASCII substring(),
		//		this one will get correctly the amount of UNICODE characters requested.
		else if (ttype == ttype_ascii || ttype == ttype_none || ttype == ttype_binary)
		{
			/* Redundant.
			if (offset >= desc.dsc_length)
				desc.dsc_length = 0;
			else */
			desc.dsc_address += offset;
			desc.dsc_length -= offset;
			if (length < desc.dsc_length)
				desc.dsc_length = length;
			EVL_make_value(tdbb, &desc, impure);
		}
		else
		{
			// CVC: ATTENTION:
			// I couldn't find an appropriate message for this failure among current registered
			// messages, so I will return empty.
			// Finally I decided to use arithmetic exception or numeric overflow.
			const UCHAR* p = desc.dsc_address;
			const USHORT pcount = desc.dsc_length;

			CharSet* charSet = INTL_charset_lookup(tdbb, desc.getCharSet());

			desc.dsc_address = NULL;
			const ULONG totLen = MIN(MAX_COLUMN_SIZE, length * charSet->maxBytesPerChar());
			desc.dsc_length = totLen;
			EVL_make_value(tdbb, &desc, impure);

			dataLen = charSet->substring(pcount, p, totLen,
				impure->vlu_desc.dsc_address, offset, length);
			impure->vlu_desc.dsc_length = static_cast<USHORT>(dataLen);
		}
	}

	return &impure->vlu_desc;
}


void SysFunction::checkArgsMismatch(int count) const
{
	if (count < minArgCount || (maxArgCount != -1 && count > maxArgCount))
	{
		status_exception::raise(Arg::Gds(isc_funmismat) << Arg::Str(name.c_str()));
	}
}
