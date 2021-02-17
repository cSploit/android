/*
 *	PROGRAM:	JRD International support
 *	MODULE:		intlobj_new.h
 *	DESCRIPTION:	New international text handling definitions
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
 *  Adriano dos Santos Fernandes
 *
 */

#ifndef JRD_INTLOBJ_NEW_H
#define JRD_INTLOBJ_NEW_H

#ifndef INCLUDE_FB_TYPES_H
typedef unsigned short USHORT;
typedef short SSHORT;
typedef unsigned char UCHAR;
typedef char CHAR;
typedef unsigned char BYTE;

typedef unsigned int ULONG;
typedef int LONG;
typedef signed int SLONG;
#endif

typedef char ASCII;

typedef USHORT INTL_BOOL;

struct texttype; /* forward decl for the fc signatures before the struct itself. */
struct csconvert;
struct charset;

#define INTL_BAD_KEY_LENGTH ((USHORT) -1)
#define INTL_BAD_STR_LENGTH ((ULONG) -1)

/* Returned value of INTL_BAD_KEY_LENGTH means that proposed key is too long */
typedef USHORT (*pfn_INTL_keylength) (texttype* tt, USHORT len);

/* Types of the keys which may be returned by str2key routine */

#define INTL_KEY_SORT    0 /* Full sort key */
#define INTL_KEY_PARTIAL 1 /* Starting portion of sort key for equality class */
#define INTL_KEY_UNIQUE  2 /* Full key for the equality class of the string */

/* Returned value of INTL_BAD_KEY_LENGTH means that key error happened during
  key construction. When partial key is requested returned string should
  complement collated comparison.
*/
typedef USHORT (*pfn_INTL_str2key) (
	texttype* tt,
	USHORT srcLen,
	const UCHAR* src,
	USHORT dstLen,
	UCHAR* dst,
	USHORT key_type
);

/* Collate two potentially long strings. According to SQL 2003 standard
  collation is a process by which two strings are determined to be in exactly
  one of the relationships of less than, greater than, or equal to one another.
*/
typedef SSHORT (*pfn_INTL_compare) (
	texttype* tt,
	ULONG len1,
	const UCHAR* str1,
	ULONG len2,
	const UCHAR* str2,
	INTL_BOOL* error_flag
);

/* Returns resulting string length in bytes or INTL_BAD_STR_LENGTH in case of error */
typedef ULONG (*pfn_INTL_str2case) (
	texttype* tt,
	ULONG srcLen,
	const UCHAR* src,
	ULONG dstLen,
	UCHAR* dst
);

/*
  Places exactly texttype_canonical_width number of bytes into dst for each character from src.
  Returns INTL_BAD_STR_LENGTH in case of error or number of characters processed if successful.
 */
typedef ULONG (*pfn_INTL_canonical) (
	texttype* t,
	ULONG srcLen,
	const UCHAR* src,
	ULONG dstLen,
	UCHAR* dst
);

/* Releases resources associated with collation */
typedef void (*pfn_INTL_tt_destroy) (texttype* tt);

/* texttype version */
#define TEXTTYPE_VERSION_1	1

/* texttype flag values */

#define TEXTTYPE_DIRECT_MATCH 1 /* Pattern-matching may be performed directly on
                                   string without going to canonical form */

#define TEXTTYPE_SEPARATE_UNIQUE 2 /* Full key does not define equality class.
                                      To be used with multi-level collations which are
                                      case- or accent- insensitive */

#define TEXTTYPE_UNSORTED_UNIQUE 4 /* Unique keys may not be used for ordered access,
                                      such as for multi-level collation having weights
                                      (char, case, accent) which is case-insensitive,
                                      but accent-sensitive */


struct texttype
{
	/* Data which needs to be initialized by collation driver */
	USHORT texttype_version;	/* version ID of object */
	void* texttype_impl;		/* collation object implemented in driver */

    /* Used only for debugging purposes. Should contain string in form
      <charset>.<collation>. For example "WIN1251.PXW_CYRL"
    */
	const ASCII* texttype_name;

	SSHORT texttype_country;	    /* ID of base country values */
	BYTE texttype_canonical_width;  /* number bytes in canonical character representation */

	USHORT texttype_flags; /* Misc texttype flags filled by driver */

	/* do we logically pad string with spaces for comparison purposes.
       this is the job of string_to_key and compare routines to care or not to
       care about trailing spaces */
	INTL_BOOL texttype_pad_option;

	/* If not set for fixed width charset key length is assumed to be equal to string length.
	   If not set for MBCS key length is assumed to be equal to length of string converted to BOCU-1. */
	pfn_INTL_keylength	texttype_fn_key_length; /* Return key length for given string */

	/* If not set for fixed width charset string itself is used as a key with binary lexical ordering.
	   If not set for MBCS string converted to BOCU-1 is used as a key with UCS_BASIC ordering. */
	pfn_INTL_str2key	texttype_fn_string_to_key;

	/* If not set for fixed width charset string itself is assumed to be binary-comparable both for sorting
	   and equality purposes. If not set for MBCS string converted to UTF-16 is compared. */
	pfn_INTL_compare	texttype_fn_compare;

	/* If not set string is converted to Unicode and then uppercased via default case folding table.
	   NOTE: Source buffer may be used by engine as a target for conversion.
	         Driver must handle this situation appropriately. */
	pfn_INTL_str2case	texttype_fn_str_to_upper;	/* Convert string to uppercase */

	/* If not set string is converted to Unicode and then lowercased via default case folding table.
	   NOTE: Source buffer may be used by engine as a target for conversion.
	         Driver must handle this situation appropriately. */
	pfn_INTL_str2case	texttype_fn_str_to_lower;	/* Convert string to lowercase */

	/* If not set for fixed width charset string itself is used as canonical
       representation. If not set for MBCS charset string converted to UTF-32
       is used as canonical representation */
	pfn_INTL_canonical	texttype_fn_canonical;	/* convert string to canonical representation for equality */

	/* May be omitted if not needed */
	pfn_INTL_tt_destroy	texttype_fn_destroy;	/* release resources associated with collation */

	/* Some space for future extension of collation interface */
	void* reserved_for_interface[5];

	/* Some space which may be freely used by collation driver */
	void* reserved_for_driver[10];
};

/* Returns resulting string length or INTL_BAD_STR_LENGTH in case of error */
typedef ULONG (*pfn_INTL_convert) (
	csconvert* cv,
	ULONG srcLen,
	const UCHAR* src,
	ULONG dstLen,
	UCHAR* dst,
	USHORT* error_code,
	ULONG* offending_source_character
);

/* Releases resources associated with conversion */
typedef void (*pfn_INTL_cv_destroy) (
	csconvert* cv
);

/* csconvert version */
#define CSCONVERT_VERSION_1	1

struct CsConvertImpl;

struct csconvert
{
	USHORT csconvert_version;
	CsConvertImpl* csconvert_impl;

    /* Used only for debugging purposes. Should contain string in form
      <source_charset>-><destination_charset>. For example "WIN1251->DOS866"
    */
	const ASCII* csconvert_name;

	/* Conversion routine. Must be present. */
	pfn_INTL_convert csconvert_fn_convert;

	/* May be omitted if not needed. Is not called for converters embedded into charset interface */
	pfn_INTL_cv_destroy	csconvert_fn_destroy;

	/* Some space for future extension of conversion interface */
	void* reserved_for_interface[2];

	/* Some space which may be freely used by conversion driver */
	void* reserved_for_driver[10];
};

/* Conversion error codes */

#define	CS_TRUNCATION_ERROR	1	/* output buffer too small  */
#define	CS_CONVERT_ERROR	2	/* can't remap a character      */
#define	CS_BAD_INPUT		3	/* input string detected as bad */

#define	CS_CANT_MAP		0		/* Flag table entries that don't map */


/* Returns whether string is well-formed or not */
typedef INTL_BOOL (*pfn_INTL_well_formed) (
	charset* cs,
	ULONG len,
	const UCHAR* str,
	ULONG* offending_position
);

/* Extracts a portion from a string. Returns INTL_BAD_STR_LENGTH in case of problems. */
typedef ULONG (*pfn_INTL_substring) (
	charset* cs,
	ULONG srcLen,
	const UCHAR* src,
	ULONG dstLen,
	UCHAR* dst,
	ULONG startPos,
	ULONG length
);

/* Measures the length of string in characters. Returns INTL_BAD_STR_LENGTH in case of problems. */
typedef ULONG (*pfn_INTL_length) (
	charset* cs,
	ULONG srcLen,
	const UCHAR* src
);

/* Releases resources associated with charset */
typedef void (*pfn_INTL_cs_destroy) (
	charset* cv
);

/* charset version */
#define CHARSET_VERSION_1	1

/* charset flag values */
#define CHARSET_LEGACY_SEMANTICS 1 /* MBCS strings may overflow declared lengths
                                      in characters (but not in bytes) */

#define CHARSET_ASCII_BASED 2 /* Value of ASCII characters is equal to the
                                 ASCII character set */

struct charset
{
	USHORT charset_version;
	void* charset_impl;
	const ASCII* charset_name;
	BYTE charset_min_bytes_per_char;
	BYTE charset_max_bytes_per_char;
	BYTE charset_space_length;       /* Length of space character in bytes */
	const BYTE* charset_space_character; /* Space character, may be used for string padding */
	USHORT charset_flags; /* Misc charset flags filled by driver */

	/* Conversions to and from UTF-16 intermediate encodings. BOM marker should not be used.
      Endianness of transient encoding is the native endianness for the platform */
	csconvert		charset_to_unicode; /* Result of this conversion should be in Normalization Form C */
	csconvert		charset_from_unicode;

	/* If omitted any string is considered well-formed */
	pfn_INTL_well_formed	charset_fn_well_formed;

	/* If not set Unicode representation is used to measure string length. */
	pfn_INTL_length		charset_fn_length;	/* get length of string in characters */

	/* May be omitted for fixed-width character sets.
	   If not present for MBCS charset string operation is performed by the engine
       via intermediate translation of string to Unicode */
	pfn_INTL_substring	charset_fn_substring;	/* get a portion of string */

	/* May be omitted if not needed */
	pfn_INTL_cs_destroy	charset_fn_destroy;

	/* Some space for future extension of charset interface */
	void* reserved_for_interface[5];

	/* Some space which may be freely used by charset driver */
	void* reserved_for_driver[10];
};


/* interface entry-point version */
#define INTL_VERSION_1	1
#define INTL_VERSION_2	2	/* 1) added functions LD_version and LD_setup_attributes
							   2) added parameter config_info in pfn_INTL_lookup_charset and
							      pfn_INTL_lookup_texttype */

/* attributes passed by the engine to texttype entry-point */
#define TEXTTYPE_ATTR_PAD_SPACE				1
#define TEXTTYPE_ATTR_CASE_INSENSITIVE		2
#define TEXTTYPE_ATTR_ACCENT_INSENSITIVE	4


/* typedef for texttype lookup entry-point */
typedef INTL_BOOL (*pfn_INTL_lookup_texttype) (
	texttype* tt,
	const ASCII* texttype_name,
	const ASCII* charset_name,
	USHORT attributes,
	const UCHAR* specific_attributes,
	ULONG specific_attributes_length,
	INTL_BOOL ignore_attributes,
	const ASCII* config_info
);

/* typedef for charset lookup entry-point */
typedef INTL_BOOL (*pfn_INTL_lookup_charset) (
	charset* cs,
	const ASCII* name,
	const ASCII* config_info
);

/* typedef for version entry-point */
typedef void (*pfn_INTL_version) (
	USHORT* version
);

/* Returns resulting string length or INTL_BAD_STR_LENGTH in case of error */
typedef ULONG (*pfn_INTL_setup_attributes) (
	const ASCII* texttype_name,
	const ASCII* charset_name,
	const ASCII* config_info,
	ULONG srcLen,
	const UCHAR* src,
	ULONG dstLen,
	UCHAR* dst
);


#define TEXTTYPE_ENTRYPOINT					LD_lookup_texttype
#define CHARSET_ENTRYPOINT					LD_lookup_charset
#define INTL_VERSION_ENTRYPOINT				LD_version
#define INTL_SETUP_ATTRIBUTES_ENTRYPOINT	LD_setup_attributes

#endif /* JRD_INTLOBJ_NEW_H */
