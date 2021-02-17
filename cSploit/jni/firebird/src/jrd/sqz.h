/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sqz.h
 *	DESCRIPTION:	Data compression control block
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
 */

#ifndef JRD_SQZ_H
#define JRD_SQZ_H

#include "../include/fb_blk.h"
#include "../../common/classes/array.h"

namespace Jrd
{
	class Compressor
	{
	public:
		Compressor(MemoryPool& pool, size_t length, const UCHAR* data);

		size_t getPackedLength() const
		{
			return m_length;
		}

		const UCHAR* getControl() const
		{
			return m_control.begin();
		}

		size_t getControlSize() const
		{
			return m_control.getCount();
		}

		void pack(const UCHAR*, UCHAR*) const;
		size_t pack(const UCHAR*, size_t, UCHAR*) const;
		size_t getPartialLength(size_t, const UCHAR*) const;

		static UCHAR* unpack(size_t, const UCHAR*, size_t, UCHAR*);
		static size_t applyDiff(size_t, const UCHAR*, size_t, UCHAR* const);
		static size_t makeDiff(size_t, const UCHAR*, size_t, UCHAR*, size_t, UCHAR*);
		static size_t makeNoDiff(size_t, UCHAR*);

	private:
		Firebird::HalfStaticArray<UCHAR, 2048> m_control;
		size_t m_length;
	};

} //namespace Jrd

#endif // JRD_SQZ_H

