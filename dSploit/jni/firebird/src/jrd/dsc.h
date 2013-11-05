/*
 *	PROGRAM:	JRD access method
 *	MODULE:		dsc.h
 *	DESCRIPTION:	Definitions associated with descriptors
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
 * 2002.04.16  Paul Beach - HP10 Define changed from -4 to (-4) to make it
 *             compatible with the HP Compiler
 * Adriano dos Santos Fernandes
 */

#ifndef JRD_DSC_H
#define JRD_DSC_H

#include "../jrd/dsc_pub.h"
#include "consts_pub.h"
#include "../jrd/ods.h"
#include "../intl/charsets.h"

/* Data type information */

inline bool DTYPE_IS_TEXT(UCHAR d)
{
	return d >= dtype_text && d <= dtype_varying;
}

inline bool DTYPE_IS_DATE(UCHAR t)
{
	return t >= dtype_sql_date && t <= dtype_timestamp;
}

/* DTYPE_IS_BLOB includes both BLOB and ARRAY since array's are implemented over blobs. */
inline bool DTYPE_IS_BLOB(UCHAR d)
{
	return d == dtype_blob || d == dtype_array;
}

/* DTYPE_IS_BLOB_OR_QUAD includes both BLOB, QUAD and ARRAY since array's are implemented over blobs. */
inline bool DTYPE_IS_BLOB_OR_QUAD(UCHAR d)
{
	return d == dtype_blob || d == dtype_quad || d == dtype_array;
}

/* Exact numeric? */
inline bool DTYPE_IS_EXACT(UCHAR d)
{
	return d == dtype_int64 || d == dtype_long || d == dtype_short;
}

inline bool DTYPE_IS_APPROX(UCHAR d)
{
	return d == dtype_double || d == dtype_real;
}

inline bool DTYPE_IS_NUMERIC(UCHAR d)
{
	return (d >= dtype_byte && d <= dtype_d_float) || d  == dtype_int64;
}

/* Descriptor format */

typedef struct dsc
{
	dsc()
		: dsc_dtype(0),
		  dsc_scale(0),
		  dsc_length(0),
		  dsc_sub_type(0),
		  dsc_flags(0),
		  dsc_address(0)
	{}

	UCHAR	dsc_dtype;
	SCHAR	dsc_scale;
	USHORT	dsc_length;
	SSHORT	dsc_sub_type;
	USHORT	dsc_flags;
	UCHAR*	dsc_address; // Used either as offset in a message or as a pointer

#ifdef __cplusplus
	SSHORT dsc_blob_ttype() const { return dsc_scale | (dsc_flags & 0xFF00);}
	SSHORT& dsc_ttype() { return dsc_sub_type;}
	SSHORT dsc_ttype() const { return dsc_sub_type;}

	bool isNullable() const
	{
		return dsc_flags & DSC_nullable;
	}

	void setNullable(bool nullable)
	{
		if (nullable)
			dsc_flags |= DSC_nullable;
		else
			dsc_flags &= ~(DSC_nullable | DSC_null);
	}

	bool isNull() const
	{
		return dsc_flags & DSC_null;
	}

	void setNull()
	{
		dsc_flags |= DSC_null | DSC_nullable;
	}

	bool isBlob() const
	{
		return dsc_dtype == dtype_blob || dsc_dtype == dtype_quad;
	}

	bool isExact() const
	{
		return dsc_dtype == dtype_int64 || dsc_dtype == dtype_long || dsc_dtype == dtype_short;
	}

	bool isText() const
	{
		return dsc_dtype >= dtype_text && dsc_dtype <= dtype_varying;
	}

	bool isUnknown() const
	{
		return dsc_dtype == dtype_unknown;
	}

	SSHORT getBlobSubType() const
	{
		if (isBlob())
			return dsc_sub_type;

		return isc_blob_text;
	}

	void setBlobSubType(SSHORT subType)
	{
		if (isBlob())
			dsc_sub_type = subType;
	}

	UCHAR getCharSet() const
	{
		if (isText())
			return dsc_sub_type & 0xFF;

		if (isBlob())
		{
			if (dsc_sub_type == isc_blob_text)
				return dsc_scale;

			return CS_BINARY;
		}

		return CS_NONE;
	}

	USHORT getTextType() const
	{
		if (isText())
			return dsc_sub_type;

		if (isBlob())
		{
			if (dsc_sub_type == isc_blob_text)
				return dsc_scale | (dsc_flags & 0xFF00);

			return CS_BINARY;
		}

		return CS_NONE;
	}

	void setTextType(USHORT ttype)
	{
		if (isText())
			dsc_sub_type = ttype;
		else if (isBlob() && dsc_sub_type == isc_blob_text)
		{
			dsc_scale = ttype & 0xFF;
			dsc_flags = (dsc_flags & 0xFF) | (ttype & 0xFF00);
		}
	}

	void clear()
	{
		memset(this, 0, sizeof(*this));
	}

	void makeBlob(SSHORT subType, USHORT ttype, ISC_QUAD* address = NULL)
	{
		clear();
		dsc_dtype = dtype_blob;
		dsc_length = sizeof(ISC_QUAD);
		setBlobSubType(subType);
		setTextType(ttype);
		dsc_address = (UCHAR*) address;
	}

	void makeDouble(double* address = NULL)
	{
		clear();
		dsc_dtype = dtype_double;
		dsc_length = sizeof(double);
		dsc_address = (UCHAR*) address;
	}

	void makeInt64(SCHAR scale, SINT64* address = NULL)
	{
		clear();
		dsc_dtype = dtype_int64;
		dsc_length = sizeof(SINT64);
		dsc_scale = scale;
		dsc_address = (UCHAR*) address;
	}

	void makeLong(SCHAR scale, SLONG* address = NULL)
	{
		clear();
		dsc_dtype = dtype_long;
		dsc_length = sizeof(SLONG);
		dsc_scale = scale;
		dsc_address = (UCHAR*) address;
	}

	void makeNullString()
	{
		clear();

		// CHAR(1) CHARACTER SET NONE
		dsc_dtype = dtype_text;
		setTextType(CS_NONE);
		dsc_length = 1;
		dsc_flags = DSC_nullable | DSC_null;
	}

	void makeShort(SCHAR scale, SSHORT* address = NULL)
	{
		clear();
		dsc_dtype = dtype_short;
		dsc_length = sizeof(SSHORT);
		dsc_scale = scale;
		dsc_address = (UCHAR*) address;
	}

	void makeText(USHORT length, USHORT ttype, UCHAR* address = NULL)
	{
		clear();
		dsc_dtype = dtype_text;
		dsc_length = length;
		setTextType(ttype);
		dsc_address = address;
	}

	void makeTimestamp(GDS_TIMESTAMP* address = NULL)
	{
		clear();
		dsc_dtype = dtype_timestamp;
		dsc_length = sizeof(GDS_TIMESTAMP);
		dsc_scale = 0;
		dsc_address = (UCHAR*) address;
	}

	void makeVarying(USHORT length, USHORT ttype, UCHAR* address = NULL)
	{
		clear();
		dsc_dtype = dtype_varying;
		dsc_length = sizeof(USHORT) + length;
		setTextType(ttype);
		dsc_address = address;
	}

	int getStringLength() const;
#endif

// this functions were added to have interoperability
// between Ods::Descriptor and struct dsc
	dsc(const Ods::Descriptor& od)
		: dsc_dtype(od.dsc_dtype),
		  dsc_scale(od.dsc_scale),
		  dsc_length(od.dsc_length),
		  dsc_sub_type(od.dsc_sub_type),
		  dsc_flags(od.dsc_flags),
		  dsc_address((UCHAR*)(IPTR)(od.dsc_offset))
	{}

	operator Ods::Descriptor() const
	{
#ifdef DEV_BUILD
		address32bit();
#endif
		Ods::Descriptor d;
		d.dsc_dtype = dsc_dtype;
		d.dsc_scale = dsc_scale;
		d.dsc_length = dsc_length;
		d.dsc_sub_type = dsc_sub_type;
		d.dsc_flags = dsc_flags;
		d.dsc_offset = (ULONG)(IPTR)dsc_address;
		return d;
	}
#ifdef DEV_BUILD
	void address32bit() const;
#endif

} DSC;

inline SSHORT DSC_GET_CHARSET(const dsc* desc)
{
	return (desc->dsc_sub_type & 0x00FF);
}

inline SSHORT DSC_GET_COLLATE(const dsc* desc)
{
	return (desc->dsc_sub_type >> 8);
}

struct alt_dsc
{
	SLONG dsc_combined_type;
	SSHORT dsc_sub_type;
	USHORT dsc_flags;			/* Not currently used */
};

inline bool DSC_EQUIV(const dsc* d1, const dsc* d2, bool check_collate)
{
	if (((alt_dsc*) d1)->dsc_combined_type == ((alt_dsc*) d2)->dsc_combined_type)
	{
		if (d1->dsc_dtype >= dtype_text && d1->dsc_dtype <= dtype_varying)
		{
			if (DSC_GET_CHARSET(d1) == DSC_GET_CHARSET(d2))
			{
				if (check_collate) {
					return (DSC_GET_COLLATE(d1) == DSC_GET_COLLATE(d2));
				}
				return true;
			}
			return false;
		}
		return true;
	}
	return false;
}

/* In DSC_*_result tables, DTYPE_CANNOT means that the two operands
   cannot participate together in the requested operation. */

const UCHAR DTYPE_CANNOT	= 127;

/* Historical alias definition */
const UCHAR dtype_date		= dtype_timestamp;

const UCHAR dtype_aligned	= dtype_varying;
const UCHAR dtype_any_text	= dtype_varying;
const UCHAR dtype_min_comp	= dtype_packed;
const UCHAR dtype_max_comp	= dtype_d_float;

/* NOTE: For types <= dtype_any_text the dsc_sub_type field defines
   the text type */

inline USHORT TEXT_LEN(const dsc* desc)
{
	return ((desc->dsc_dtype == dtype_text) ?
		desc->dsc_length :
		(desc->dsc_dtype == dtype_cstring) ?
			desc->dsc_length - 1u : desc->dsc_length - sizeof(USHORT));
}


/* Text Sub types, distinct from character sets & collations */

const SSHORT dsc_text_type_none		= 0;	/* Normal text */
const SSHORT dsc_text_type_fixed	= 1;	/* strings can contain null bytes */
const SSHORT dsc_text_type_ascii	= 2;	/* string contains only ASCII characters */
const SSHORT dsc_text_type_metadata	= 3;	/* string represents system metadata */


/* Exact numeric subtypes: with ODS >= 10, these apply when dtype
   is short, long, or quad. */

const SSHORT dsc_num_type_none		= 0;	/* defined as SMALLINT or INTEGER */
const SSHORT dsc_num_type_numeric	= 1;	/* defined as NUMERIC(n,m)        */
const SSHORT dsc_num_type_decimal	= 2;	/* defined as DECIMAL(n,m)        */

/* Date type information */

inline SCHAR NUMERIC_SCALE(const dsc desc)
{
	return ((DTYPE_IS_TEXT(desc.dsc_dtype)) ? 0 : desc.dsc_scale);
}

#endif /* JRD_DSC_H */
