/*
 *	PROGRAM:	JRD International support
 *	MODULE:		intl_classes.h
 *	DESCRIPTION:	International text handling definitions
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#ifndef JRD_INTL_CLASSES_H
#define JRD_INTL_CLASSES_H

#include "firebird.h"
#include "../jrd/jrd.h"
#include "../jrd/intlobj_new.h"
#include "../jrd/constants.h"
#include "../jrd/unicode_util.h"
#include "../jrd/CsConvert.h"
#include "../jrd/CharSet.h"
#include "../jrd/TextType.h"

namespace Jrd {

class PatternMatcher
{
public:
	PatternMatcher(MemoryPool& aPool, TextType* aTextType)
		: pool(aPool),
		  textType(aTextType)
	{
	}

	virtual ~PatternMatcher()
	{
	}

	virtual void reset() = 0;
	virtual bool process(const UCHAR*, SLONG) = 0;
	virtual bool result() = 0;

protected:
	MemoryPool& pool;
	TextType* textType;
};

class NullStrConverter
{
public:
	NullStrConverter(MemoryPool& /*pool*/, const TextType* /*obj*/, const UCHAR* /*str*/, SLONG /*len*/)
	{
	}
};

template <typename PrevConverter>
class UpcaseConverter : public PrevConverter
{
public:
	UpcaseConverter(MemoryPool& pool, TextType* obj, const UCHAR*& str, SLONG& len)
		: PrevConverter(pool, obj, str, len)
	{
		if (len > (int) sizeof(tempBuffer))
			out_str = FB_NEW(pool) UCHAR[len];
		else
			out_str = tempBuffer;
		obj->str_to_upper(len, str, len, out_str);
		str = out_str;
	}

	~UpcaseConverter()
	{
		if (out_str != tempBuffer)
			delete[] out_str;
	}

private:
	UCHAR tempBuffer[100];
	UCHAR* out_str;
};

template <typename PrevConverter>
class CanonicalConverter : public PrevConverter
{
public:
	CanonicalConverter(MemoryPool& pool, TextType* obj, const UCHAR*& str, SLONG& len)
		: PrevConverter(pool, obj, str, len)
	{
		const SLONG out_len = len / obj->getCharSet()->minBytesPerChar() * obj->getCanonicalWidth();

		if (out_len > (int) sizeof(tempBuffer))
			out_str = FB_NEW(pool) UCHAR[out_len];
		else
			out_str = tempBuffer;

		if (str)
		{
			len = obj->canonical(len, str, out_len, out_str) * obj->getCanonicalWidth();
			str = out_str;
		}
		else
			len = 0;
	}

	~CanonicalConverter()
	{
		if (out_str != tempBuffer)
			delete[] out_str;
	}

private:
	UCHAR tempBuffer[100];
	UCHAR* out_str;
};

} // namespace Jrd


#include "../jrd/Collation.h"


#endif	// JRD_INTL_CLASSES_H
