/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		merge.cpp
 *	DESCRIPTION:	Merge database/server information
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
#include "../jrd/ibase.h"
#include "../remote/remote.h"
#include "../remote/merge_proto.h"
#include "../jrd/gds_proto.h"

inline void PUT_WORD(UCHAR*& ptr, USHORT value)
{
	*ptr++ = static_cast<UCHAR>(value);
	*ptr++ = static_cast<UCHAR>(value >> 8);
}

#define PUT(ptr, value)		*(ptr)++ = value;

#ifdef NOT_USED_OR_REPLACED
static SSHORT convert(ULONG, UCHAR *);
#endif
static ISC_STATUS merge_setup(const UCHAR**, UCHAR**, const UCHAR* const, USHORT);


USHORT MERGE_database_info(const UCHAR* in,
							UCHAR* out,
							USHORT out_length,
							USHORT impl,
							USHORT class_,
							USHORT base_level,
							const UCHAR* version,
							const UCHAR* id)
							//ULONG mask Was always zero
{
/**************************************
 *
 *	M E R G E _ d a t a b a s e _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Merge server / remote interface / Y-valve information into
 *	database block.  Return the actual length of the packet.
 *
 **************************************/
	SSHORT l;
	const UCHAR* p;

	UCHAR* start = out;
	const UCHAR* const end = out + out_length;

	for (;;)
		switch (*out++ = *in++)
		{
		case isc_info_end:
		case isc_info_truncated:
			return out - start;

		case isc_info_firebird_version:
			l = strlen((char *) (p = version));
			if (l > MAX_UCHAR)
			    l = MAX_UCHAR;
			if (merge_setup(&in, &out, end, l + 1))
				return 0;
			for (*out++ = (UCHAR) l; l; --l)
				*out++ = *p++;
			break;

		case isc_info_db_id:
			l = strlen((SCHAR *) (p = id));
			if (l > MAX_UCHAR)
				l = MAX_UCHAR;
			if (merge_setup(&in, &out, end, l + 1))
				return 0;
			for (*out++ = (UCHAR) l; l; --l)
				*out++ = *p++;
			break;

		case isc_info_implementation:
			if (merge_setup(&in, &out, end, 2))
				return 0;
			PUT(out, (UCHAR) impl);
			PUT(out, (UCHAR) class_);
			break;

		case isc_info_base_level:
			if (merge_setup(&in, &out, end, 1))
				return 0;
			PUT(out, (UCHAR) base_level);
			break;

		default:
			{
				USHORT length = (USHORT) gds__vax_integer(in, 2);
				in += 2;
				if (out + length + 2 >= end)
				{
					out[-1] = isc_info_truncated;
					return 0;
				}
				PUT_WORD(out, length);
				while (length--)
					*out++ = *in++;
			}
			break;
		}
}

#ifdef NOT_USED_OR_REPLACED
static SSHORT convert( ULONG number, UCHAR * buffer)
{
/**************************************
 *
 *	c o n v e r t
 *
 **************************************
 *
 * Functional description
 *	Convert a number to VAX form -- least significant bytes first.
 *	Return the length.
 *
 **************************************/
	const UCHAR *p;

#ifndef WORDS_BIGENDIAN
	p = (UCHAR *) &number;
	*buffer++ = *p++;
	*buffer++ = *p++;
	*buffer++ = *p++;
	*buffer++ = *p++;

#else

	p = (UCHAR *) &number;
	p += 3;
	*buffer++ = *p--;
	*buffer++ = *p--;
	*buffer++ = *p--;
	*buffer++ = *p;

#endif

	return 4;
}
#endif

static ISC_STATUS merge_setup(const UCHAR** in, UCHAR** out, const UCHAR* const end,
							  USHORT delta_length)
{
/**************************************
 *
 *	m e r g e _ s e t u p
 *
 **************************************
 *
 * Functional description
 *	Get ready to toss new stuff onto an info packet.  This involves
 *	picking up and bumping the "count" field and copying what is
 *	already there.
 *
 **************************************/
	USHORT length = (USHORT) gds__vax_integer(*in, 2);
	const USHORT new_length = length + delta_length;

	if (*out + new_length + 2 >= end)
	{
		(*out)[-1] = isc_info_truncated;
		return FB_FAILURE;
	}

	*in += 2;
	const USHORT count = 1 + *(*in)++;
	PUT_WORD(*out, new_length);
	PUT(*out, (UCHAR) count);

	// Copy data portion of information sans original count

	if (--length)
	{
		memcpy(*out, *in, length);
		*out += length;
		*in += length;
	}

	return FB_SUCCESS;
}
