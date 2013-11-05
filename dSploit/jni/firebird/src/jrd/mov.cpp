/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		mov.cpp
 *	DESCRIPTION:	Data mover and converter and comparator, etc.
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
 * 2002.08.21 Dmitry Yemanov: fixed bug with a buffer overrun,
 *                            which at least caused invalid dependencies
 *                            to be stored (DB$xxx, for example)
 * Adriano dos Santos Fernandes
 */

#include "firebird.h"
#include "../jrd/common.h"
#include "../jrd/gdsassert.h"
#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/intl.h"
#include "../jrd/blb_proto.h"
#include "../jrd/cvt_proto.h"
#include "../common/cvt.h"
#include "../jrd/cvt2_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/mov_proto.h"

using namespace Firebird;

int MOV_compare(const dsc* arg1, const dsc* arg2)
{
/**************************************
 *
 *	M O V _ c o m p a r e
 *
 **************************************
 *
 * Functional description
 *	Compare two descriptors.  Return (-1, 0, 1) if a<b, a=b, or a>b.
 *
 **************************************/

	return CVT2_compare(arg1, arg2);
}


double MOV_date_to_double(const dsc* desc)
{
/**************************************
 *
 *	M O V _ d a t e _ t o _ d o u b l e
 *
 **************************************
 *
 * Functional description
 *    Convert a date to double precision for
 *    date arithmetic routines.
 *
 **************************************/

	return CVT_date_to_double(desc);
}


#ifdef NOT_USED_OR_REPLACED
// Strange, I don't see this function surfaced as public.
void MOV_double_to_date2(double real, dsc* desc)
{
/**************************************
 *
 *	M O V _ d o u b l e _ t o _ d a t e 2
 *
 **************************************
 *
 * Functional description
 *	Move a double to one of the many forms of a date
 *
 **************************************/
	SLONG fixed[2];

	MOV_double_to_date(real, fixed);
	switch (desc->dsc_dtype)
	{
	case dtype_timestamp:
		((SLONG *) desc->dsc_address)[0] = fixed[0];
		((SLONG *) desc->dsc_address)[1] = fixed[1];
		break;
	case dtype_sql_time:
		((SLONG *) desc->dsc_address)[0] = fixed[1];
		break;
	case dtype_sql_date:
		((SLONG *) desc->dsc_address)[0] = fixed[0];
		break;
	default:
		fb_assert(FALSE);
		break;
	}
}
#endif


void MOV_double_to_date(double real, SLONG fixed[2])
{
/**************************************
 *
 *	M O V _ d o u b l e _ t o _ d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert a double precision representation of a date
 *	to a fixed point representation.   Double is used for
 *      date arithmetic.
 *
 **************************************/

	CVT_double_to_date(real, fixed);
}


double MOV_get_double(const dsc* desc)
{
/**************************************
 *
 *	M O V _ g e t _ d o u b l e
 *
 **************************************
 *
 * Functional description
 *	Convert something arbitrary to a double precision number
 *
 **************************************/

	return CVT_get_double(desc, ERR_post);
}


SLONG MOV_get_long(const dsc* desc, SSHORT scale)
{
/**************************************
 *
 *	M O V _ g e t _ l o n g
 *
 **************************************
 *
 * Functional description
 *	Convert something arbitrary to a long (32 bit) integer of given
 *	scale.
 *
 **************************************/

	return CVT_get_long(desc, scale, ERR_post);
}


SINT64 MOV_get_int64(const dsc* desc, SSHORT scale)
{
/**************************************
 *
 *	M O V _ g e t _ i n t 6 4
 *
 **************************************
 *
 * Functional description
 *	Convert something arbitrary to a 64 bit integer of given
 *	scale.
 *
 **************************************/

	return CVT_get_int64(desc, scale, ERR_post);
}


void MOV_get_metadata_str(const dsc* desc, TEXT* buffer, USHORT buffer_length)
{
/**************************************
 *
 *	M O V _ g e t _ m e t a d a t a _ s t r
 *
 **************************************
 *
 * Functional description
 *	Copy string, which will afterward's
 *	be treated as a metadata name value,
 *	to the user-supplied buffer.
 *
 **************************************/
	USHORT dummy_type;
	UCHAR *ptr;

	USHORT length = CVT_get_string_ptr(desc, &dummy_type, &ptr, NULL, 0);

#ifdef DEV_BUILD
	if ((dummy_type != ttype_metadata) &&
		(dummy_type != ttype_none) && (dummy_type != ttype_ascii))
	{
		ERR_bugcheck_msg("Expected METADATA name");
	}
#endif

	length = ptr ? MIN(length, buffer_length - 1) : 0;
	memcpy(buffer, ptr, length);
	buffer[length] = 0;
}


void MOV_get_name(const dsc* desc, TEXT* string)
{
/**************************************
 *
 *	M O V _ g e t _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Get a name (max length 31, blank terminated) from a descriptor.
 *
 **************************************/

	CVT2_get_name(desc, string);
}


SQUAD MOV_get_quad(const dsc* desc, SSHORT scale)
{
/**************************************
 *
 *	M O V _ g e t _ q u a d
 *
 **************************************
 *
 * Functional description
 *	Convert something arbitrary to a quad
 *	Note: a quad is NOT the same as a 64 bit integer
 *
 **************************************/

	return CVT_get_quad(desc, scale, ERR_post);
}


int MOV_get_string_ptr(const dsc* desc,
					   USHORT* ttype,
					   UCHAR** address, vary* temp, USHORT length)
{
/**************************************
 *
 *	M O V _ g e t _ s t r i n g _ p t r
 *
 **************************************
 *
 * Functional description
 *	Get address and length of string, converting the value to
 *	string, if necessary.  The caller must provide a sufficiently
 *	large temporary.  The address of the resultant string is returned
 *	by reference.  Get_string returns the length of the string.
 *
 *	Note: If the descriptor is known to be a string type, the third
 *	argument (temp buffer) may be omitted.
 *
 **************************************/

	return CVT_get_string_ptr(desc, ttype, address, temp, length);
}


int MOV_get_string(const dsc* desc, UCHAR** address, vary* temp, USHORT length)
{
/**************************************
 *
 *	M O V _ g e t _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	USHORT ttype;

	return MOV_get_string_ptr(desc, &ttype, address, temp, length);
}


GDS_DATE MOV_get_sql_date(const dsc* desc)
{
/**************************************
 *
 *	M O V _ g e t _ s q l _ d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert something arbitrary to a SQL date
 *
 **************************************/

	return CVT_get_sql_date(desc);
}


GDS_TIME MOV_get_sql_time(const dsc* desc)
{
/**************************************
 *
 *	M O V _ g e t _ s q l _ t i m e
 *
 **************************************
 *
 * Functional description
 *	Convert something arbitrary to a SQL time
 *
 **************************************/

	return CVT_get_sql_time(desc);
}


GDS_TIMESTAMP MOV_get_timestamp(const dsc* desc)
{
/**************************************
 *
 *	M O V _ g e t _ t i m e s t a m p
 *
 **************************************
 *
 * Functional description
 *	Convert something arbitrary to a timestamp
 *
 **************************************/

	return CVT_get_timestamp(desc);
}


int MOV_make_string(const dsc*	     desc,
					USHORT	     ttype,
					const char** address,
					vary*	     temp,
					USHORT	     length)
{
/**************************************
 *
 *	M O V _ m a k e _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Make a string, in a specified text type, out of a descriptor.
 *	The caller must provide a sufficiently
 *	large temporary.  The address of the resultant string is returned
 *	by reference.
 *	MOV_make_string returns the length of the string in bytes.
 *
 *	Note: If the descriptor is known to be a string type in the
 *	given ttype the argument (temp buffer) may be omitted.
 *	But this would be a bad idea in general.
 *
 **************************************/

	return CVT_make_string(desc, ttype, address, temp, length, ERR_post);
}


int MOV_make_string2(Jrd::thread_db* tdbb,
					 const dsc* desc,
					 USHORT ttype,
					 UCHAR** address,
					 Jrd::MoveBuffer& buffer,
					 bool limit)
{
/**************************************
 *
 *	M O V _ m a k e _ s t r i n g 2
 *
 **************************************
 *
 * Functional description
 *	Make a string, in a specified text type, out of a descriptor.
 *	The address of the resultant string is returned by reference.
 *	MOV_make_string2 returns the length of the string in bytes.
 *
 **************************************/

	if (desc->isBlob())
	{
		// fake descriptor
		dsc temp;
		temp.dsc_dtype = dtype_text;
		temp.setTextType(ttype);

		Firebird::UCharBuffer bpb;
		BLB_gen_bpb_from_descs(desc, &temp, bpb);

		Jrd::blb* blob = BLB_open2(tdbb, tdbb->getRequest()->req_transaction,
			reinterpret_cast<Jrd::bid*>(desc->dsc_address), bpb.getCount(), bpb.begin());

		ULONG size;

		if (temp.getCharSet() == desc->getCharSet())
		{
			size = blob->blb_length;
		}
		else
		{
			size = (blob->blb_length / INTL_charset_lookup(tdbb, desc->getCharSet())->minBytesPerChar()) *
				INTL_charset_lookup(tdbb, temp.getCharSet())->maxBytesPerChar();
		}

		*address = buffer.getBuffer(size);

		size = BLB_get_data(tdbb, blob, *address, size, true);

		if (limit && size > MAX_COLUMN_SIZE)
			ERR_post(Arg::Gds(isc_arith_except) <<
					 Arg::Gds(isc_blob_truncation));

		return size;
	}

	return CVT2_make_string2(desc, ttype, address, buffer);
}


void MOV_move(Jrd::thread_db* tdbb, /*const*/ dsc* from, dsc* to)
{
/**************************************
 *
 *	M O V _ m o v e
 *
 **************************************
 *
 * Functional description
 *	Move (and possible convert) something to something else.
 *
 **************************************/

	if (DTYPE_IS_BLOB_OR_QUAD(from->dsc_dtype) ||
		DTYPE_IS_BLOB_OR_QUAD(to->dsc_dtype))
	{
		BLB_move(tdbb, from, to, NULL);
	}
	else
		CVT_move(from, to);
}
