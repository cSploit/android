/*
 *	PROGRAM:	JRD International support
 *	MODULE:		intl.h
 *	DESCRIPTION:	International text handling definitions
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

#ifndef JRD_INTL_H
#define JRD_INTL_H

#include "../jrd/dsc.h"
#include "../jrd/ibase.h"
#include "../intl/charsets.h"

#define ASCII_SPACE     32		/* ASCII code for space */

//#define INTL_name_not_found		1
//#define INTL_subtype_not_implemented	2

/*
 *  Default character set name for specification of COLLATE clause without
 *  a CHARACTER SET clause.
 *
 *  NATIONAL_CHARACTER_SET is used for SQL's NATIONAL character type.
 */
#define DEFAULT_CHARACTER_SET_NAME	"ISO8859_1"
#define NATIONAL_CHARACTER_SET		DEFAULT_CHARACTER_SET_NAME

#define DEFAULT_DB_CHARACTER_SET_NAME	"NONE"
/*
 * Character Set used for system metadata information
 */
#define CS_METADATA			CS_UNICODE_FSS	/* metadata charset */

/* text type definitions */

#define ttype_none				CS_NONE			/* 0 */
#define ttype_ascii				CS_ASCII		/* 2 */
#define ttype_binary			CS_BINARY		/* 1 */
#define ttype_unicode_fss		CS_UNICODE_FSS	/* 3 */
#define ttype_last_internal		CS_UTF8			/* 4 */		// not internal yet, but will be in the future

#define ttype_dynamic			CS_dynamic	/* use att_charset */

#define ttype_sort_key			ttype_binary
#define	ttype_metadata			ttype_unicode_fss

/* Note:
 * changing the value of ttype_metadata is an ODS System Metadata change
 * changing the value of CS_METADATA    is an ODS System Metadata change
 */




#define	COLLATE_NONE			0	/* No special collation, use codepoint order */

#define INTL_ASSIGN_DSC(dsc, cs, coll)   \
	{ (dsc)->dsc_sub_type = (SSHORT) ((coll) << 8 | (cs)); }

#define INTL_COPY_DSC(dest, src) \
	{ (dest)->dsc_sub_type = (src)->dsc_sub_type; }

#define INTL_GET_TTYPE(dsc)   \
	  ((dsc)->dsc_sub_type)

#define INTL_GET_CHARSET(dsc)	((UCHAR)((dsc)->dsc_sub_type & 0x00FF))
#define INTL_GET_COLLATE(dsc)	((UCHAR)((dsc)->dsc_sub_type >> 8))


/* Define tests for international data */

#define	INTL_TTYPE(desc)		((desc)->dsc_ttype())
#define	INTL_ASSIGN_TTYPE(desc, value) 	((desc)->dsc_ttype() = (SSHORT)(value))

#define INTERNAL_TTYPE(d)	(((USHORT)((d)->dsc_ttype())) <= ttype_last_internal)

#define IS_INTL_DATA(d)		((d)->dsc_dtype <= dtype_any_text &&    \
				 (((USHORT)((d)->dsc_ttype())) > ttype_last_internal))

inline USHORT INTL_TEXT_TYPE(const dsc& desc)
{
	if (DTYPE_IS_TEXT(desc.dsc_dtype))
		return INTL_TTYPE(&desc);

	if (desc.dsc_dtype == dtype_blob || desc.dsc_dtype == dtype_quad)
	{
		if (desc.dsc_sub_type == isc_blob_text)
			return desc.dsc_blob_ttype();

		return ttype_binary;
	}

	return ttype_ascii;
}

#define INTL_DYNAMIC_CHARSET(desc)	(INTL_GET_CHARSET(desc) == CS_dynamic)



/*
 * There are several ways text types are used internally to Firebird
 *  1) As a CHARACTER_SET_ID & COLLATION_ID pair (in metadata).
 *  2) As a CHARACTER_SET_ID (when collation isn't relevent, like UDF parms)
 *  3) As an index type - (btr.h)
 *  4) As a driver ID (used to lookup the code which implements the locale)
 *     This is also known as dsc_ttype() (aka text subtype).
 *
 * In Descriptors (DSC) the data is encoded as:
 *	dsc_charset	overloaded into dsc_scale
 *	dsc_collate	overloaded into dsc_sub_type
 *
 * Index types are converted to driver ID's via INTL_INDEX_TYPE
 *
 */

/* There must be a 1-1 mapping between index types and International text
 * subtypes -
 * Index-to-subtype: to compute a KEY from a Text string we must know both
 *	the TEXT format and the COLLATE routine to use (eg: the subtype info).
 * 	We need the text-format as the datavalue for key creation may not
 * 	match that needed for the index.
 * Subtype-to-index: When creating indices, they are assigned an
 *	Index type, which is derived from the datatype of the target.
 *
 */
#define INTL_INDEX_TO_TEXT(idxType) ((USHORT)((idxType) - idx_offset_intl_range))

/* Maps a text_type to an index ID */
#define INTL_TEXT_TO_INDEX(tType)   ((USHORT)((tType)   + idx_offset_intl_range))

#define MAP_CHARSET_TO_TTYPE(cs)	(cs & 0x00FF)

#define	INTL_RES_TTYPE(desc)	(INTL_DYNAMIC_CHARSET(desc) ?\
			MAP_CHARSET_TO_TTYPE(tdbb->getAttachment()->att_charset) :\
		 	INTL_GET_TTYPE (desc))

#define INTL_INDEX_TYPE(desc)	INTL_TEXT_TO_INDEX (INTL_RES_TTYPE (desc))

/* Maps a Character_set_id & collation_id to a text_type (driver ID) */
#define INTL_CS_COLL_TO_TTYPE(cs, coll)	((USHORT)((coll) << 8 | ((cs) & 0x00FF)))

#define TTYPE_TO_CHARSET(tt)    ((USHORT)((tt) & 0x00FF))
#define TTYPE_TO_COLLATION(tt)  ((USHORT)((tt) >> 8))

#endif	// JRD_INTL_H
