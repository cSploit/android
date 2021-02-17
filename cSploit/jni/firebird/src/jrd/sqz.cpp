/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sqz.cpp
 *	DESCRIPTION:	Record compression/decompression
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

#include "firebird.h"
#include <string.h>
#include "../jrd/sqz.h"
#include "../jrd/req.h"
#include "../jrd/err_proto.h"
#include "../yvalve/gds_proto.h"

using namespace Jrd;


Compressor::Compressor(MemoryPool& pool, size_t length, const UCHAR* data)
	: m_control(pool), m_length(0)
{
	UCHAR* control = m_control.getBuffer((length + 1) / 2, false);
	const UCHAR* const end = data + length;

	size_t count;
	size_t max;
	while ( (count = end - data) )
	{
		const UCHAR* start = data;

		// Find length of non-compressable run

		if ((max = count - 1) > 1)
		{
			do {
				if (data[0] != data[1] || data[0] != data[2])
				{
					data++;
				}
				else
				{
					count = data - start;
					break;
				}
			} while (--max > 1);
		}

		data = start + count;

		// Non-compressable runs are limited to 127 bytes

		while (count)
		{
			max = MIN(count, 127U);
			m_length += 1 + max;
			count -= max;
			*control++ = (UCHAR) max;
		}

		// Find compressible run. Compressable runs are limited to 128 bytes.

		if ((max = MIN(128, end - data)) >= 3)
		{
			start = data;
			const UCHAR c = *data;
			do
			{
				if (*data != c)
				{
					break;
				}
				++data;
			} while (--max);

			*control++ = (UCHAR) (start - data);
			m_length += 2;
		}
	}

	// set array size to the really used length
	m_control.shrink(control - m_control.begin());
}

size_t Compressor::applyDiff(size_t diffLength,
							 const UCHAR* differences,
							 size_t outLength,
							 UCHAR* const output)
{
/**************************************
 *
 *	Apply a differences (delta) to a record.
 *  Return the length.
 *
 **************************************/
	if (diffLength > MAX_DIFFERENCES)
	{
		BUGCHECK(176);			// msg 176 bad difference record
	}

	const UCHAR* const end = differences + diffLength;
	UCHAR* p = output;
	const UCHAR* const p_end = output + outLength;

	while (differences < end && p < p_end)
	{
		const int l = (signed char) *differences++;

		if (l > 0)
		{
			if (p + l > p_end)
			{
				BUGCHECK(177);	// msg 177 applied differences will not fit in record
			}
			if (differences + l > end)
			{
				BUGCHECK(176);	// msg 176 bad difference record
			}
			memcpy(p, differences, l);
			p += l;
			differences += l;
		}
		else
		{
			p += -l;
		}
	}

	const size_t length = p - output;

	if (length > outLength || differences < end)
	{
		BUGCHECK(177);			// msg 177 applied differences will not fit in record
	}

	return length;
}

size_t Compressor::pack(const UCHAR* input, size_t outLength, UCHAR* output) const
{
/**************************************
 *
 *	Compress a string into an area of known length.
 *	If it doesn't fit, throw BUGCHECK error.
 *
 **************************************/
	const UCHAR* const start = input;

	const UCHAR* control = m_control.begin();
	const UCHAR* const dcc_end = m_control.end();

	int space = (int) outLength;

	while (control < dcc_end)
	{
		if (--space <= 0)
		{
			if (space == 0)
			{
				*output = 0;
			}
			return input - start;
		}

		int length = (signed char) *control++;
		*output++ = (UCHAR) length;

		if (length < 0)
		{
			--space;
			*output++ = *input;
			input += (-length) & 255;
		}
		else
		{
			if ((space -= length) < 0)
			{
				length += space;
				output[-1] = (UCHAR) length;
				if (length > 0)
				{
					memcpy(output, input, length);
					input += length;
				}
				return input - start;
			}

			if (length > 0)
			{
				memcpy(output, input, length);
				output += length;
				input += length;
			}
		}
	}

	BUGCHECK(178);	// msg 178 record length inconsistent
	return 0;	// shut up compiler warning
}

size_t Compressor::getPartialLength(size_t inLength, const UCHAR* input) const
{
/**************************************
 *
 *	Same as pack() without the output.
 *	If it doesn't fit, return the number of bytes that did.
 *
 **************************************/
	const UCHAR* const start = input;

	const UCHAR* control = m_control.begin();
	const UCHAR* const dcc_end = m_control.end();

	int space = (int) inLength;

	while (control < dcc_end)
	{
		if (--space <= 0)
		{
			return input - start;
		}

		int length = (signed char) *control++;

		if (length < 0)
		{
			--space;
			input += (-length) & 255;
		}
		else
		{
			if ((space -= length) < 0)
			{
				length += space;
				input += length;
				return input - start;
			}
			input += length;
		}
	}

	BUGCHECK(178);	// msg 178 record length inconsistent
	return 0;	// shut up compiler warning
}

UCHAR* Compressor::unpack(size_t inLength,
						  const UCHAR* input,
						  size_t outLength,
						  UCHAR* output)
{
/**************************************
 *
 *	Decompress a compressed string into a buffer.
 *	Return the address where the output stopped.
 *
 **************************************/
	const UCHAR* const end = input + inLength;
	const UCHAR* const output_end = output + outLength;

	while (input < end)
	{
		const int len = (signed char) *input++;

		if (len < 0)
		{
			if (input >= end || (output - len) > output_end)
			{
				BUGCHECK(179);	// msg 179 decompression overran buffer
			}

			const UCHAR c = *input++;
			memset(output, c, (-1 * len));
			output -= len;
		}
		else
		{
			if ((output + len) > output_end)
			{
				BUGCHECK(179);	// msg 179 decompression overran buffer
			}
			memcpy(output, input, len);
			output += len;
			input += len;
		}
	}

	if (output > output_end)
	{
		BUGCHECK(179);			// msg 179 decompression overran buffer
	}

	return output;
}

size_t Compressor::makeNoDiff(size_t outLength, UCHAR* output)
{
/**************************************
 *
 *  Generates differences record marking that there are no differences.
 *
 **************************************/
	UCHAR* temp = output;
	int length = (int) outLength;

	while (length > 127)
	{
		*temp++ = -127;
		length -= 127;
	}

	if (length)
	{
		*temp++ = (UCHAR) -length;
	}

	return temp - output;
}

size_t Compressor::makeDiff(size_t length1,
							const UCHAR* rec1,
							size_t length2,
							UCHAR* rec2,
							size_t outLength,
							UCHAR* output)
{
/**************************************
 *
 *	Compute differences between two records. The difference
 *	record, when applied to the first record, produces the
 *	second record.
 *
 *	    difference_record := <control_string>...
 *
 *	    control_string := <positive_integer> <positive_integer data bytes>
 *				:= <negative_integer>
 *
 *	Return the total length of the differences string.
 *
 **************************************/
	UCHAR *p;

#define STUFF(val)	if (output < end) *output++ = val; else return MAX_ULONG;

	/* WHY IS THIS RETURNING MAX_ULONG ???
	* It returns a large positive value to indicate to the caller that we ran out
	* of buffer space in the 'out' argument. Thus we could not create a
	* successful differences record. Now it is upto the caller to check the
	* return value of this function and figure out whether the differences record
	* was created or not. Check prepare_update() (JRD/vio.c) for further
	* information. Of course, the size for a 'differences' record is not expected
	* to go near 2^32 in the future.
	*
	* This was investigated as a part of solving bug 10206, bsriram - 25-Feb-1999.
	*/

	const UCHAR* const start = output;
	const UCHAR* const end = output + outLength;
	const UCHAR* const end1 = rec1 + MIN(length1, length2);
	const UCHAR* const end2 = rec2 + length2;

	while (end1 - rec1 > 2)
	{
		if (rec1[0] != rec2[0] || rec1[1] != rec2[1])
		{
			p = output++;

			// cast this to LONG to take care of OS/2 pointer arithmetic
			// when rec1 is at the end of a segment, to avoid wrapping around

			const UCHAR* yellow = (UCHAR*) MIN((U_IPTR) end1, ((U_IPTR) rec1 + 127)) - 1;
			while (rec1 <= yellow && (rec1[0] != rec2[0] || (rec1 < yellow && rec1[1] != rec2[1])))
			{
				STUFF(*rec2++);
				++rec1;
			}
			*p = output - p - 1;
			continue;
		}

		for (p = rec2; rec1 < end1 && *rec1 == *rec2; rec1++, rec2++)
			; // no-op

		// This "l" could be more than 32K since the Old and New records
		// could be the same for more than 32K characters.
		// MAX record size is currently 64K. Hence it is defined as "int".
		int l = p - rec2;

		while (l < -127)
		{
			STUFF(-127);
			l += 127;
		}

		if (l)
		{
			STUFF(l);
		}
	}

	while (rec2 < end2)
	{
		p = output++;

		// cast this to LONG to take care of OS/2 pointer arithmetic
		// when rec1 is at the end of a segment, to avoid wrapping around

		const UCHAR* yellow = (UCHAR*) MIN((U_IPTR) end2, ((U_IPTR) rec2 + 127));
		while (rec2 < yellow)
		{
			STUFF(*rec2++);
		}
		*p = output - p - 1;
	}

	return output - start;
#undef STUFF
}

void Compressor::pack(const UCHAR* input, UCHAR* output) const
{
/**************************************
 *
 *	Compress a string into a sufficiently large area.
 *	Don't check nuttin' -- go for speed, man, raw SPEED!
 *
 **************************************/
	const UCHAR* control = m_control.begin();
	const UCHAR* const dcc_end = m_control.end();

	while (control < dcc_end)
	{
		const int length = (signed char) *control++;
		*output++ = (UCHAR) length;

		if (length < 0)
		{
			*output++ = *input;
			input -= length;
		}
		else if (length > 0)
		{
			memcpy(output, input, length);
			output += length;
			input += length;
		}
	}
}
