/*
 *	PROGRAM:	JRD access method
 *	MODULE:		val.h
 *	DESCRIPTION:	Definitions associated with value handling
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef JRD_VAL_H
#define JRD_VAL_H

#include "../include/fb_blk.h"
#include "../common/classes/array.h"
#include "../common/classes/MetaName.h"

#include "../jrd/dsc.h"

#define FLAG_BYTES(n)	(((n + BITS_PER_LONG) & ~((ULONG)BITS_PER_LONG - 1)) >> 3)

const UCHAR DEFAULT_DOUBLE  = dtype_double;
const ULONG MAX_FORMAT_SIZE	= 65535;

namespace Jrd {

class ArrayField;
class blb;
class jrd_req;
class jrd_tra;
class Symbol;

class Format : public pool_alloc<type_fmt>
{
public:
	Format(MemoryPool& p, int len)
		: fmt_count(len), fmt_desc(p, fmt_count)
	{
		fmt_desc.resize(fmt_count);
	}
	static Format* newFormat(MemoryPool& p, int len = 0)
	{
		return FB_NEW(p) Format(p, len);
	}

	USHORT fmt_length;
	USHORT fmt_count;
	USHORT fmt_version;
	Firebird::Array<dsc> fmt_desc;
	typedef Firebird::Array<dsc>::iterator fmt_desc_iterator;
	typedef Firebird::Array<dsc>::const_iterator fmt_desc_const_iterator;
};


// Parameter passing mechanism. Also used for returning values, except for scalar_array.
enum FUN_T {
		FUN_value,
		FUN_reference,
		FUN_descriptor,
		FUN_blob_struct,
		FUN_scalar_array,
		FUN_ref_with_null
};


/* Function definition block */

struct fun_repeat
{
	DSC fun_desc;			/* Datatype info */
	FUN_T fun_mechanism;	/* Passing mechanism */
};


class UserFunction : public pool_alloc_rpt<fun_repeat, type_fun>
{
public:
	Firebird::MetaName fun_name;	// Function name
	Firebird::string fun_exception_message;	/* message containing the exception error message */
	UserFunction*	fun_homonym;	/* Homonym functions */
	Symbol*		fun_symbol;			/* Symbol block */
	int (*fun_entrypoint) ();		/* Function entrypoint */
	USHORT		fun_count;			/* Number of arguments (including return) */
	USHORT		fun_args;			/* Number of input arguments */
	USHORT		fun_return_arg;		/* Return argument */
	USHORT		fun_type;			/* Type of function */
	ULONG		fun_temp_length;	/* Temporary space required */
    fun_repeat fun_rpt[1];

public:
	explicit UserFunction(MemoryPool& p)
		: fun_name(p),
		  fun_exception_message(p)
	{
	}
};

// Those two defines seems an intention to do something that wasn't completed.
// UDfs that return values like now or boolean Udfs. See rdb$functions.rdb$function_type.
//#define FUN_value	0
//#define FUN_boolean	1

/* Blob passing structure */
// CVC: Moved to fun.epp where it belongs.

/* Scalar array descriptor, "external side" seen by UDF's */

struct scalar_array_desc
{
	DSC sad_desc;
	SLONG sad_dimensions;
	struct sad_repeat
	{
		SLONG sad_lower;
		SLONG sad_upper;
	} sad_rpt[1];
};

// Sorry for the clumsy name, but in blk.h this is referred as array description.
// Since we already have Scalar Array Descriptor and Array Description [Slice],
// it was too confusing. Therefore, it was renamed ArrayField, since ultimately,
// it represents an array field the user can manipulate.
// There was also confusion for the casual reader due to the presence of
// the structure "slice" in sdl.h that was renamed array_slice.

class ArrayField : public pool_alloc_rpt<Ods::InternalArrayDesc::iad_repeat, type_arr>
{
public:
	UCHAR*				arr_data;			/* Data block, if allocated */
	blb*				arr_blob;			/* Blob for data access */
	jrd_tra*			arr_transaction;	/* Parent transaction block */
	ArrayField*			arr_next;			/* Next array in transaction */
	jrd_req*			arr_request;		/* request */
	SLONG				arr_effective_length;	/* Length of array instance */
	USHORT				arr_desc_length;	/* Length of array descriptor */
	ULONG				arr_temp_id;		// Temporary ID for open array inside the transaction

	// Keep this field last as it is C-style open array !
	Ods::InternalArrayDesc	arr_desc;		/* Array descriptor. ! */
};

} //namespace Jrd

#endif /* JRD_VAL_H */

