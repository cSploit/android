/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		cvt2.cpp
 *	DESCRIPTION:	Data mover and converter and comparator, etc.
 *			Routines used ONLY within engine.
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
 * 2001.6.18 Claudio Valderrama: Implement comparison on blobs and blobs against
 * other datatypes by request from Ann Harrison.
 */

#include "firebird.h"
#include <string.h>
#include "../jrd/common.h"
#include "../jrd/ibase.h"

#include "../jrd/jrd.h"
#include "../jrd/val.h"
#include "../jrd/quad.h"
#include "gen/iberror.h"
#include "../jrd/intl.h"
#include "../jrd/gdsassert.h"
#include "../jrd/cvt_proto.h"
#include "../jrd/cvt2_proto.h"
#include "../common/cvt.h"
#include "../jrd/err_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/intl_classes.h"
#include "../jrd/gds_proto.h"
// CVC: I needed them here.
#include "../jrd/jrd.h"
#include "../jrd/blb_proto.h"
#include "../jrd/tra.h"
#include "../jrd/req.h"
#include "../jrd/constants.h"
#include "../common/utils_proto.h"
#include "../common/classes/VaryStr.h"

using namespace Jrd;
using namespace Firebird;

/* The original order of dsc_type values corresponded to the priority
   of conversion (that is, always convert the lesser to the greater
   type.)  Introduction of dtype_int64 breaks that assumption: its
   position on the scale should be between dtype_long and dtype_real, but
   the types are integers, and dtype_quad occupies the only available
   place.  Renumbering all the higher-numbered types would be a major
   ODS change and a fundamental discomfort

   This table permits us to put the entries in the right order for
   comparison purpose, even though isc_int64 had to get number 19, which
   is otherwise too high.

   This table is used in CVT2_compare, is indexed by dsc_dtype, and
   returns the relative priority of types for use when different types
   are compared.
   */
static const BYTE compare_priority[] =
{
	dtype_unknown,				// dtype_unknown through dtype_varying
	dtype_text,					// have their natural values stored
	dtype_cstring,				// in the table.
	dtype_varying,
	0, 0,						// dtypes and 4, 5 are unused.
	dtype_packed,				// packed through long also have
	dtype_byte,					// their natural values in the table
	dtype_short,
	dtype_long,
	dtype_quad + 1,				// quad through array all move up
	dtype_real + 1,				// by one to make room for int64
	dtype_double + 1,			// at its proper place in the table.
	dtype_d_float + 1,
	dtype_sql_date + 1,
	dtype_sql_time + 1,
	dtype_timestamp + 1,
	dtype_blob + 1,
	dtype_array + 1,
	dtype_long + 1,				// int64 goes right after long
	dtype_dbkey					// compares with nothing except itself
};


bool CVT2_get_binary_comparable_desc(dsc* result, const dsc* arg1, const dsc* arg2)
{
/**************************************
 *
 *	C V T 2 _ g e t _ b i n a r y _ c o m p a r a b l e _ d e s c
 *
 **************************************
 *
 * Functional description
 *	Return descriptor of the data type to be used for direct (binary) comparison of the given arguments.
 *
 **************************************/

	if (arg1->dsc_dtype == dtype_blob || arg2->dsc_dtype == dtype_blob ||
		arg1->dsc_dtype == dtype_array || arg2->dsc_dtype == dtype_array)
	{
		// Any of the arguments is a blob or an array
		return false;
	}
	
	if (arg1->dsc_dtype == dtype_dbkey || arg2->dsc_dtype == dtype_dbkey)
	{
		// Any of the arguments is DBKEY
		result->makeText(MAX(arg1->getStringLength(), arg2->getStringLength()), ttype_binary);
	}
	else if (arg1->isText() && arg2->isText())
	{
		// Both arguments are strings
		if (arg1->getTextType() != arg2->getTextType())
		{
			// Charsets/collations are different
			return false;
		}

		if (arg1->dsc_dtype == arg2->dsc_dtype)
		{
			*result = *arg1;
			result->dsc_length = MAX(arg1->dsc_length, arg2->dsc_length);
		}
		else
		{
			result->makeText(MAX(arg1->getStringLength(), arg2->getStringLength()),
				arg1->getTextType());
		}
	}
	else if (arg1->dsc_dtype == arg2->dsc_dtype && arg1->dsc_scale == arg2->dsc_scale)
	{
		// Arguments can be compared directly
		*result = *arg1;
	}
	else
	{
		// Arguments are of different data types
		*result = (compare_priority[arg1->dsc_dtype] > compare_priority[arg2->dsc_dtype]) ? *arg1 : *arg2;

		if (arg1->isExact() && arg2->isExact())
			result->dsc_scale = MIN(arg1->dsc_scale, arg2->dsc_scale);
	}

	return true;
}


SSHORT CVT2_compare(const dsc* arg1, const dsc* arg2)
{
/**************************************
 *
 *	C V T 2 _ c o m p a r e
 *
 **************************************
 *
 * Functional description
 *	Compare two descriptors.  Return (-1, 0, 1) if a<b, a=b, or a>b.
 *
 **************************************/
	thread_db* tdbb = NULL;

	// AB: Maybe we need a other error-message, but at least throw
	// a message when 1 or both input paramters are empty.
	if (!arg1 || !arg2) {
		BUGCHECK(189);	// msg 189 comparison not supported for specified data types.
	}

	// Handle the simple (matched) ones first

	if (arg1->dsc_dtype == arg2->dsc_dtype && arg1->dsc_scale == arg2->dsc_scale)
	{
		const UCHAR* p1 = arg1->dsc_address;
		const UCHAR* p2 = arg2->dsc_address;

		switch (arg1->dsc_dtype)
		{
		case dtype_short:
			if (*(SSHORT *) p1 == *(SSHORT *) p2)
				return 0;
			if (*(SSHORT *) p1 > *(SSHORT *) p2)
				return 1;
			return -1;

		case dtype_sql_time:
			if (*(ULONG *) p1 == *(ULONG *) p2)
				return 0;
			if (*(ULONG *) p1 > *(ULONG *) p2)
				return 1;
			return -1;

		case dtype_long:
		case dtype_sql_date:
			if (*(SLONG *) p1 == *(SLONG *) p2)
				return 0;
			if (*(SLONG *) p1 > *(SLONG *) p2)
				return 1;
			return -1;

		case dtype_quad:
			return QUAD_COMPARE(*(SQUAD *) p1, *(SQUAD *) p2);

		case dtype_int64:
			if (*(SINT64 *) p1 == *(SINT64 *) p2)
				return 0;
			if (*(SINT64 *) p1 > *(SINT64 *) p2)
				return 1;
			return -1;

		case dtype_dbkey:
			{
				// keep old ttype_binary compare rules
				USHORT l = MIN(arg1->dsc_length, arg2->dsc_length);
				SSHORT rc = memcmp(p1, p2, l);
				if (rc)
				{
					return rc;
				}
				return (arg1->dsc_length > l) ? 1 : (arg2->dsc_length > l) ? -1 : 0;
			}

		case dtype_timestamp:
			if (((SLONG *) p1)[0] > ((SLONG *) p2)[0])
				return 1;
			if (((SLONG *) p1)[0] < ((SLONG *) p2)[0])
				return -1;
			if (((ULONG *) p1)[1] > ((ULONG *) p2)[1])
				return 1;
			if (((ULONG *) p1)[1] < ((ULONG *) p2)[1])
				return -1;
			return 0;

		case dtype_real:
			if (*(float *) p1 == *(float *) p2)
				return 0;
			if (*(float *) p1 > *(float *) p2)
				return 1;
			return -1;

		case DEFAULT_DOUBLE:
			if (*(double *) p1 == *(double *) p2)
				return 0;
			if (*(double *) p1 > *(double *) p2)
				return 1;
			return -1;

		case dtype_text:
		case dtype_varying:
		case dtype_cstring:
		case dtype_array:
		case dtype_blob:
			// Special processing below
			break;

		default:
			// the two arguments have identical dtype and scale, but the
			// dtype is not one of your defined types!
			fb_assert(FALSE);
			break;

		}						// switch on dtype
	}							// if dtypes and scales are equal

	// Handle mixed string comparisons

	if (arg1->dsc_dtype <= dtype_varying && arg2->dsc_dtype <= dtype_varying)
	{
		/*
		 * For the sake of optimization, we call INTL_compare
		 * only when we cannot just do byte-by-byte compare.
		 * We can do a local compare here, if
		 *    (a) one of the arguments is charset ttype_binary
		 * OR (b) both of the arguments are char set ttype_none
		 * OR (c) both of the arguments are char set ttype_ascii
		 * If any argument is ttype_dynamic, we must see the
		 * charset of the attachment.
		 */

		SET_TDBB(tdbb);
		CHARSET_ID charset1 = INTL_TTYPE(arg1);
		if (charset1 == ttype_dynamic)
			charset1 = INTL_charset(tdbb, charset1);

		CHARSET_ID charset2 = INTL_TTYPE(arg2);
		if (charset2 == ttype_dynamic)
			charset2 = INTL_charset(tdbb, charset2);

		if ((IS_INTL_DATA(arg1) || IS_INTL_DATA(arg2)) &&
			(charset1 != ttype_binary) &&
			(charset2 != ttype_binary) &&
			((charset1 != ttype_ascii) ||
			 (charset2 != ttype_ascii)) &&
			((charset1 != ttype_none) || (charset2 != ttype_none)))
		{
			return INTL_compare(tdbb, arg1, arg2, ERR_post);
		}

		UCHAR* p1 = NULL;
		UCHAR* p2 = NULL;
		USHORT t1, t2; // unused later
		USHORT length = CVT_get_string_ptr(arg1, &t1, &p1, NULL, 0);
		USHORT length2 = CVT_get_string_ptr(arg2, &t2, &p2, NULL, 0);

		int fill = length - length2;
		const UCHAR pad = charset1 == ttype_binary || charset2 == ttype_binary ? '\0' : ' ';

		if (length >= length2)
		{
			if (length2)
			{
				do
				{
					if (*p1++ != *p2++)
						return (p1[-1] > p2[-1]) ? 1 : -1;
				} while (--length2);
			}

			if (fill > 0)
			{
				do
				{
					if (*p1++ != pad)
						return (p1[-1] > pad) ? 1 : -1;
				} while (--fill);
			}

			return 0;
		}

		if (length)
		{
			do
			{
				if (*p1++ != *p2++)
					return (p1[-1] > p2[-1]) ? 1 : -1;
			} while (--length);
		}

		do
		{
			if (*p2++ != pad)
				return (pad > p2[-1]) ? 1 : -1;
		} while (++fill);

		return 0;
	}

	// Handle heterogeneous compares

	if (compare_priority[arg1->dsc_dtype] < compare_priority[arg2->dsc_dtype])
		return -CVT2_compare(arg2, arg1);

	// At this point, the type of arg1 is guaranteed to be "greater than" arg2,
	// in the sense that it is the preferred type for comparing the two.

	switch (arg1->dsc_dtype)
	{
		SLONG date[2];

	case dtype_timestamp:
		{
			DSC desc;
			MOVE_CLEAR(&desc, sizeof(desc));
			desc.dsc_dtype = dtype_timestamp;
			desc.dsc_length = sizeof(date);
			desc.dsc_address = (UCHAR *) date;
			CVT_move(arg2, &desc);
			return CVT2_compare(arg1, &desc);
		}

	case dtype_sql_time:
		{
			DSC desc;
			MOVE_CLEAR(&desc, sizeof(desc));
			desc.dsc_dtype = dtype_sql_time;
			desc.dsc_length = sizeof(date[0]);
			desc.dsc_address = (UCHAR *) date;
			CVT_move(arg2, &desc);
			return CVT2_compare(arg1, &desc);
		}

	case dtype_sql_date:
		{
			DSC desc;
			MOVE_CLEAR(&desc, sizeof(desc));
			desc.dsc_dtype = dtype_sql_date;
			desc.dsc_length = sizeof(date[0]);
			desc.dsc_address = (UCHAR *) date;
			CVT_move(arg2, &desc);
			return CVT2_compare(arg1, &desc);
		}

	case dtype_short:
		{
			SSHORT scale;
			if (arg2->dsc_dtype > dtype_varying)
				scale = MIN(arg1->dsc_scale, arg2->dsc_scale);
			else
				scale = arg1->dsc_scale;
			const SLONG temp1 = CVT_get_long(arg1, scale, ERR_post);
			const SLONG temp2 = CVT_get_long(arg2, scale, ERR_post);
			if (temp1 == temp2)
				return 0;
			if (temp1 > temp2)
				return 1;
			return -1;
		}

	case dtype_long:
		// Since longs may overflow when scaled, use int64 instead
	case dtype_int64:
		{
			SSHORT scale;
			if (arg2->dsc_dtype > dtype_varying)
				scale = MIN(arg1->dsc_scale, arg2->dsc_scale);
			else
				scale = arg1->dsc_scale;
			const SINT64 temp1 = CVT_get_int64(arg1, scale, ERR_post);
			const SINT64 temp2 = CVT_get_int64(arg2, scale, ERR_post);
			if (temp1 == temp2)
				return 0;
			if (temp1 > temp2)
				return 1;
			return -1;
		}

	case dtype_quad:
		{
			SSHORT scale;
			if (arg2->dsc_dtype > dtype_varying)
				scale = MIN(arg1->dsc_scale, arg2->dsc_scale);
			else
				scale = arg1->dsc_scale;
			const SQUAD temp1 = CVT_get_quad(arg1, scale, ERR_post);
			const SQUAD temp2 = CVT_get_quad(arg2, scale, ERR_post);
			return QUAD_COMPARE(temp1, temp2);
		}

	case dtype_real:
		{
			const float temp1 = (float) CVT_get_double(arg1, ERR_post);
			const float temp2 = (float) CVT_get_double(arg2, ERR_post);
			if (temp1 == temp2)
				return 0;
			if (temp1 > temp2)
				return 1;
			return -1;
		}

	case dtype_double:
		{
			const double temp1 = CVT_get_double(arg1, ERR_post);
			const double temp2 = CVT_get_double(arg2, ERR_post);
			if (temp1 == temp2)
				return 0;
			if (temp1 > temp2)
				return 1;
			return -1;
		}

	case dtype_blob:
		return CVT2_blob_compare(arg1, arg2);

	case dtype_array:
		ERR_post(Arg::Gds(isc_wish_list) << Arg::Gds(isc_blobnotsup) << "compare");
		break;

	case dtype_dbkey:
		if (arg2->dsc_dtype <= dtype_any_text)
		{
			UCHAR* p = NULL;
			USHORT t; // unused later
			USHORT length = CVT_get_string_ptr(arg2, &t, &p, NULL, 0);

			USHORT l = MIN(arg1->dsc_length, length);
			SSHORT rc = memcmp(arg1->dsc_address, p, l);
			if (rc)
			{
				return rc;
			}
			return (arg1->dsc_length > l) ? 1 : (length > l) ? -1 : 0;
		}
		ERR_post(Arg::Gds(isc_wish_list) << Arg::Gds(isc_random) << "DB_KEY compare");
		break;

	default:
		BUGCHECK(189);			// msg 189 comparison not supported for specified data types
		break;
	}
	return 0;
}


SSHORT CVT2_blob_compare(const dsc* arg1, const dsc* arg2)
{
/**************************************
 *
 *	C V T 2 _ b l o b _ c o m p a r e
 *
 **************************************
 *
 * Functional description
 *	Compare two blobs.  Return (-1, 0, 1) if a<b, a=b, or a>b.
 *  Alternatively, it will try to compare a blob against a string;
 *	in this case, the string should be the second argument.
 * CVC: Ann Harrison asked for this function to make comparisons more
 * complete in the engine.
 *
 **************************************/

	SLONG l1, l2;
	USHORT ttype2;
	SSHORT ret_val = 0;

	thread_db* tdbb = NULL;
	SET_TDBB(tdbb);

	// DEV_BLKCHK (node, type_nod);

	if (arg1->dsc_dtype != dtype_blob)
		ERR_post(Arg::Gds(isc_wish_list) << Arg::Gds(isc_datnotsup));

	USHORT ttype1;
	if (arg1->dsc_sub_type == isc_blob_text)
		ttype1 = arg1->dsc_blob_ttype();       // Load blob character set and collation
	else
		ttype1 = ttype_binary;

	TextType* obj1 = INTL_texttype_lookup(tdbb, ttype1);
	ttype1 = obj1->getType();

	// Is arg2 a blob?
	if (arg2->dsc_dtype == dtype_blob)
	{
	    // Same blob id address?
		if (arg1->dsc_address == arg2->dsc_address)
			return 0;

		// Second test for blob id, checking relation and slot.
		const bid* bid1 = (bid*) arg1->dsc_address;
		const bid* bid2 = (bid*) arg2->dsc_address;
		if (*bid1 == *bid2)
		{
			return 0;
		}

		if (arg2->dsc_sub_type == isc_blob_text)
			ttype2 = arg2->dsc_blob_ttype();       // Load blob character set and collation
		else
			ttype2 = ttype_binary;

		TextType* obj2 = INTL_texttype_lookup(tdbb, ttype2);
		ttype2 = obj2->getType();

		if (ttype1 == ttype_binary || ttype2 == ttype_binary)
			ttype1 = ttype2 = ttype_binary;
		else if (ttype1 == ttype_none || ttype2 == ttype_none)
			ttype1 = ttype2 = ttype_none;

		obj1 = INTL_texttype_lookup(tdbb, ttype1);
		obj2 = INTL_texttype_lookup(tdbb, ttype2);

		CharSet* charSet1 = obj1->getCharSet();
		CharSet* charSet2 = obj2->getCharSet();

		Firebird::HalfStaticArray<UCHAR, BUFFER_LARGE> buffer1;
		Firebird::HalfStaticArray<UCHAR, BUFFER_LARGE> buffer2;
		fb_assert(BUFFER_LARGE % 4 == 0);	// 4 is our maximum character length

		UCHAR bpb[] = {isc_bpb_version1,
					   isc_bpb_source_type, 1, isc_blob_text, isc_bpb_source_interp, 1, 0,
					   isc_bpb_target_type, 1, isc_blob_text, isc_bpb_target_interp, 1, 0};
		USHORT bpbLength = 0;

		if (arg1->dsc_sub_type == isc_blob_text && arg2->dsc_sub_type == isc_blob_text)
		{
			bpb[6] = arg2->dsc_scale;	// source charset
			bpb[12] = arg1->dsc_scale;	// destination charset
			bpbLength = sizeof(bpb);
		}

	    blb* blob1 = BLB_open(tdbb, tdbb->getRequest()->req_transaction, (bid*) arg1->dsc_address);
		blb* blob2 = BLB_open2(tdbb, tdbb->getRequest()->req_transaction, (bid*) arg2->dsc_address, bpbLength, bpb);

		if (charSet1->isMultiByte())
		{
			buffer1.getBuffer(blob1->blb_length);
			buffer2.getBuffer(blob2->blb_length / charSet2->minBytesPerChar() * charSet1->maxBytesPerChar());
		}

		while (ret_val == 0 && !(blob1->blb_flags & BLB_eof) && !(blob2->blb_flags & BLB_eof))
		{
			l1 = BLB_get_data(tdbb, blob1, buffer1.begin(), buffer1.getCapacity(), false);
			l2 = BLB_get_data(tdbb, blob2, buffer2.begin(), buffer2.getCapacity(), false);

			ret_val = obj1->compare(l1, buffer1.begin(), l2, buffer2.begin());
		}

		if (ret_val == 0)
		{
			if ((blob1->blb_flags & BLB_eof) == BLB_eof)
				l1 = 0;

			if ((blob2->blb_flags & BLB_eof) == BLB_eof)
				l2 = 0;

			while (ret_val == 0 &&
				   !((blob1->blb_flags & BLB_eof) == BLB_eof &&
					 (blob2->blb_flags & BLB_eof) == BLB_eof))
			{
				if (!(blob1->blb_flags & BLB_eof))
					l1 = BLB_get_data(tdbb, blob1, buffer1.begin(), buffer1.getCapacity(), false);

				if (!(blob2->blb_flags & BLB_eof))
					l2 = BLB_get_data(tdbb, blob2, buffer2.begin(), buffer2.getCapacity(), false);

				ret_val = obj1->compare(l1, buffer1.begin(), l2, buffer2.begin());
			}
		}

		BLB_close(tdbb, blob1);
		BLB_close(tdbb, blob2);
	}
	else if (arg2->dsc_dtype == dtype_array)
	{
		// We do not accept arrays for now. Maybe InternalArrayDesc in the future.
		ERR_post(Arg::Gds(isc_wish_list) << Arg::Gds(isc_datnotsup));
	}
	else
	{
		// The second parameter should be a string.
		if (arg2->dsc_dtype <= dtype_varying)
		{
			if ((ttype2 = arg2->dsc_ttype()) != ttype_binary)
				ttype2 = ttype1;
		}
		else
			ttype2 = ttype1;

		if (ttype1 == ttype_binary || ttype2 == ttype_binary)
			ttype1 = ttype2 = ttype_binary;
		else if (ttype1 == ttype_none || ttype2 == ttype_none)
			ttype1 = ttype2 = ttype_none;

		obj1 = INTL_texttype_lookup(tdbb, ttype1);

		CharSet* charSet1 = obj1->getCharSet();

		Firebird::HalfStaticArray<UCHAR, BUFFER_LARGE> buffer1;
		UCHAR* p;
		MoveBuffer temp_str;

		l2 = CVT2_make_string2(arg2, ttype1, &p, temp_str);

		blb* blob1 = BLB_open(tdbb, tdbb->getRequest()->req_transaction, (bid*) arg1->dsc_address);

		if (charSet1->isMultiByte())
			buffer1.getBuffer(blob1->blb_length);
		else
			buffer1.getBuffer(l2);

		l1 = BLB_get_data(tdbb, blob1, buffer1.begin(), buffer1.getCapacity(), false);
		ret_val = obj1->compare(l1, buffer1.begin(), l2, p);

		while (ret_val == 0 && (blob1->blb_flags & BLB_eof) != BLB_eof)
		{
			l1 = BLB_get_data(tdbb, blob1, buffer1.begin(), buffer1.getCapacity(), false);
			ret_val = obj1->compare(l1, buffer1.begin(), 0, p);
		}

		BLB_close(tdbb, blob1);
	}

	return ret_val;
}


void CVT2_get_name(const dsc* desc, TEXT* string)
{
/**************************************
 *
 *	C V T 2 _ g e t _ n a m e
 *
 **************************************
 *
 * Functional description
 *	Get a name (max length 31, NULL terminated) from a descriptor.
 *
 **************************************/
	VaryStr<MAX_SQL_IDENTIFIER_SIZE> temp;			// 31 bytes + 1 NULL
	const char* p;

	const USHORT length = CVT_make_string(desc, ttype_metadata, &p, &temp, sizeof(temp), ERR_post);

	memcpy(string, p, length);
	string[length] = 0;
	fb_utils::exact_name(string);
}


USHORT CVT2_make_string2(const dsc* desc, USHORT to_interp, UCHAR** address, Jrd::MoveBuffer& temp)
{
/**************************************
 *
 *	C V T 2 _ m a k e _ s t r i n g 2
 *
 **************************************
 *
 * Functional description
 *
 *     Convert the data from the desc to a string in the specified interp.
 *     The pointer to this string is returned in address.
 *
 **************************************/
	UCHAR* from_buf;
	USHORT from_len;
	USHORT from_interp;

	fb_assert(desc != NULL);
	fb_assert(address != NULL);

	switch (desc->dsc_dtype)
	{
	case dtype_text:
		from_buf = desc->dsc_address;
		from_len = desc->dsc_length;
		from_interp = INTL_TTYPE(desc);
		break;

	case dtype_cstring:
		from_buf = desc->dsc_address;
		from_len = MIN(strlen((char *) desc->dsc_address), (unsigned) (desc->dsc_length - 1));
		from_interp = INTL_TTYPE(desc);
		break;

	case dtype_varying:
		{
			vary* varying = (vary*) desc->dsc_address;
			from_buf = reinterpret_cast<UCHAR*>(varying->vary_string);
			from_len = MIN(varying->vary_length, (USHORT) (desc->dsc_length - sizeof(SSHORT)));
			from_interp = INTL_TTYPE(desc);
		}
		break;
	}

	if (desc->dsc_dtype <= dtype_any_text)
	{

		if (to_interp == from_interp)
		{
			*address = from_buf;
			return from_len;
		}

		thread_db* tdbb = JRD_get_thread_data();
		const USHORT cs1 = INTL_charset(tdbb, to_interp);
		const USHORT cs2 = INTL_charset(tdbb, from_interp);
		if (cs1 == cs2)
		{
			*address = from_buf;
			return from_len;
		}

		USHORT length = INTL_convert_bytes(tdbb, cs1, NULL, 0, cs2, from_buf, from_len, ERR_post);
		UCHAR* tempptr = temp.getBuffer(length);
		length = INTL_convert_bytes(tdbb, cs1, tempptr, length, cs2, from_buf, from_len, ERR_post);
		*address = tempptr;
		return length;
	}

	// Not string data, then  -- convert value to varying string.

	dsc temp_desc;
	MOVE_CLEAR(&temp_desc, sizeof(temp_desc));
	temp_desc.dsc_length = temp.getCapacity();
	temp_desc.dsc_address = temp.getBuffer(temp_desc.dsc_length);
	vary* vtmp = reinterpret_cast<vary*>(temp_desc.dsc_address);
	INTL_ASSIGN_TTYPE(&temp_desc, to_interp);
	temp_desc.dsc_dtype = dtype_varying;
	CVT_move(desc, &temp_desc);
	*address = reinterpret_cast<UCHAR*>(vtmp->vary_string);

	return vtmp->vary_length;
}
