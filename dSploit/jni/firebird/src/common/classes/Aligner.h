/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Aligner.h
 *	DESCRIPTION:	Aligner, OutAligner - templates to help
 *					with alignment on RISC machines.
 *					Should be used ONLY as temporary on-stack buffers!
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
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef CLASSES_ALIGN_H
#define CLASSES_ALIGN_H

#include "../common/classes/array.h"

namespace Firebird {

// Aligns output parameter (i.e. transfers data in destructor).
template <typename C>
class OutAligner
{
private:
	UCHAR* const userBuffer;
#ifdef RISC_ALIGNMENT
	Firebird::HalfStaticArray<C, BUFFER_SMALL> localBuffer;
	ULONG bSize;
	C* bPointer;
#endif

public:
	OutAligner(UCHAR* buf, ULONG len) : userBuffer(buf)
#ifdef RISC_ALIGNMENT
		, bSize(len), bPointer(0)
#endif
	{
		fb_assert(len % sizeof(C) == 0);
#ifdef RISC_ALIGNMENT
		if ((IPTR) userBuffer & (sizeof(C) - 1))
		{
			bPointer = localBuffer.getBuffer(len / sizeof(C) + (bSize % sizeof(C) ? 1 : 0));
		}
#endif
	}

	operator C*()
	{
#ifdef RISC_ALIGNMENT
		return bPointer ? bPointer : reinterpret_cast<C*>(userBuffer);
#else
		return reinterpret_cast<C*>(userBuffer);
#endif
	}

	~OutAligner()
	{
#ifdef RISC_ALIGNMENT
		if (bPointer)
		{
			memcpy(userBuffer, bPointer, bSize);
		}
#endif
	}
};

// Aligns input parameter.
template <typename C>
class Aligner
{
private:
#ifdef RISC_ALIGNMENT
	Firebird::HalfStaticArray<C, BUFFER_SMALL> localBuffer;
#endif
	const C* bPointer;

public:
	Aligner(const UCHAR* buf, ULONG len)
	{
		fb_assert(len % sizeof(C) == 0);
#ifdef RISC_ALIGNMENT
		if ((IPTR) buf & (sizeof(C) - 1))
		{
			C* tempPointer = localBuffer.getBuffer(len / sizeof(C) + (len % sizeof(C) ? 1 : 0));
			memcpy(tempPointer, buf, len);
			bPointer = tempPointer;
		}
		else
#endif
			bPointer = reinterpret_cast<const C*>(buf);
	}

	operator const C*()
	{
		return bPointer;
	}
};

// Aligns tail in *_rpt structures when later too active casts are used
#if defined(RISC_ALIGNMENT) && (SIZEOF_VOID_P < FB_DOUBLE_ALIGN)
#define RPT_ALIGN(rpt) union { rpt; SINT64 dummy; }
#else
#define RPT_ALIGN(rpt) rpt
#endif

} // namespace Firebird

#endif // CLASSES_ALIGN_H
