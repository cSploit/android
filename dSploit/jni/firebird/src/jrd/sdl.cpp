/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sdl.cpp
 *	DESCRIPTION:	Array slice manipulator
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
#include "../jrd/common.h"
#include "../jrd/jrd.h"
#include "../jrd/ibase.h"
#include "../jrd/val.h"
#include "../jrd/sdl.h"
#include "../jrd/intl.h"
#include "../jrd/gds_proto.h"
#include "../jrd/sdl_proto.h"
#include "../jrd/err_proto.h"

const int COMPILE_SIZE	= 256;

using namespace Jrd;
using namespace Firebird;

struct sdl_arg
{
	Ods::InternalArrayDesc* sdl_arg_desc;
	const UCHAR* sdl_arg_sdl;
	UCHAR* sdl_arg_array;
	SLONG* sdl_arg_variables;
	SDL_walk_callback sdl_arg_callback;
	array_slice* sdl_arg_argument;
	ISC_STATUS* sdl_arg_status_vector;
	IPTR sdl_arg_compiled[COMPILE_SIZE];
	IPTR* sdl_arg_next;
	const IPTR* sdl_arg_end;
};

/* Structure to computes ranges */

// Let's stop this insanity! The header rng.h defined rng for the purposes
// of refresh range and emulation of file-based data formats like Pdx.
// Therefore, I renamed this struct array_range.
struct array_range
{
	SLONG rng_minima[64];
	SLONG rng_maxima[64];
	sdl_info* rng_info;
};

static const UCHAR* compile(const UCHAR*, sdl_arg*);
static ISC_STATUS error(ISC_STATUS* status_vector, const Arg::StatusVector& v);
static bool execute(sdl_arg*);
static const UCHAR* get_range(const UCHAR*, array_range*, SLONG*, SLONG*);

inline SSHORT get_word(const UCHAR*& ptr)
{
/**************************************
 *
 * g e t _ w o r d
 *
 **************************************
 *
 * Functional description
 *	gather a int16 from two little-endian
 *  unsigned chars and advance the pointer
 *
 **************************************/
	SSHORT n = *ptr++;
	n |= (*ptr++) << 8;

	return n;
}

static const UCHAR* sdl_desc(const UCHAR*, DSC*);
static IPTR* stuff(IPTR, sdl_arg*);


const int op_literal	= 1;
const int op_variable	= 2;
const int op_add		= 3;
const int op_subtract	= 4;
const int op_multiply	= 5;
const int op_divide		= 6;
const int op_iterate	= 7;
const int op_goto		= 8;
const int op_element	= 9;
const int op_loop		= 10;
const int op_exit		= 11;
const int op_scalar		= 12;

/*
   The structure for a loop is:

	<upper_bound> <increment> <initial_value>
	<loop> <iterate> <variable> <exit_address>
	[body]
	<goto> <iterate_address>
	[exit]
*/


// CVC: This is a routine that merely copies binary slice description buffer
// to new place and if the new place's size it's not enough, allocates a buffer.
// Typically, "target" is a static buffer with limited size for the small cases.
// Was made for "remote/interface.cpp" to ensure input buffers aren't overwritten.
UCHAR* SDL_clone_sdl(const UCHAR* origin, size_t origin_size, UCHAR* target, size_t target_size)
{
	UCHAR* temp_sdl = target;
	if (origin_size > target_size)
	{
		temp_sdl = (UCHAR*)gds__alloc((SLONG) origin_size);
		// FREE: apparently never freed, the caller is responsible.
		if (!temp_sdl)
		{	// NOMEM: ignore operation
			fb_assert_continue(FALSE);	// no real error handling
			return 0;
		}
	}
	memcpy(temp_sdl, origin, origin_size);
	return temp_sdl;
}


SLONG SDL_compute_subscript(ISC_STATUS* status_vector,
							const Ods::InternalArrayDesc* desc,
							USHORT dimensions,
							const SLONG* subscripts)
{
/**************************************
 *
 *	S D L _ c o m p u t e _ s u b s c r i p t
 *
 **************************************
 *
 * Functional description
 *	Collapse a multi-dimension array reference into a vector
 *	reference.
 *
 **************************************/
	if (dimensions != desc->iad_dimensions)
	{
		error(status_vector, Arg::Gds(isc_invalid_dimension) << Arg::Num(desc->iad_dimensions) <<
																Arg::Num(dimensions));
		return -1;
	}

	SLONG subscript = 0;

	const Ods::InternalArrayDesc::iad_repeat* range = desc->iad_rpt;
	for (const Ods::InternalArrayDesc::iad_repeat* const end = range + desc->iad_dimensions;
		 range < end; ++range)
	{
		const SLONG n = *subscripts++;
		if (n < range->iad_lower || n > range->iad_upper)
		{
			error(status_vector, Arg::Gds(isc_out_of_bounds));
			return -1;
		}
		subscript += (n - range->iad_lower) * range->iad_length;
	}

	return subscript;
}


ISC_STATUS SDL_info(ISC_STATUS* status_vector,
					const UCHAR* sdl, sdl_info* info, SLONG* vector)
{
/**************************************
 *
 *	S D L _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Parse enough of SDL to pick relation and field identification and
 *	element descriptor.
 *
 **************************************/
	USHORT n, offset;
	array_range range;

	const UCHAR* p = sdl;
	info->sdl_info_fid = info->sdl_info_rid = 0;
	info->sdl_info_relation = info->sdl_info_field = "";

	if (*p++ != isc_sdl_version1)
		return error(status_vector, Arg::Gds(isc_invalid_sdl) << Arg::Num(0));

	for (;;)
		switch (*p++)
		{
		case isc_sdl_struct:
			n = *p++;
			if (n != 1)
				return error(status_vector, Arg::Gds(isc_invalid_sdl) << Arg::Num(p - sdl - 1));
			offset = p - sdl;
			if (!(p = sdl_desc(p, &info->sdl_info_element)))
				return error(status_vector, Arg::Gds(isc_invalid_sdl) << Arg::Num(offset));
			info->sdl_info_element.dsc_address = 0;
			break;

		case isc_sdl_fid:
			info->sdl_info_fid = get_word(p);
			break;

		case isc_sdl_rid:
			info->sdl_info_rid = get_word(p);
			break;

		case isc_sdl_field:
			n = *p++;
			info->sdl_info_field.assign(reinterpret_cast<const char*>(p), n);
			p += n;
			break;

		case isc_sdl_relation:
			n = *p++;
			info->sdl_info_relation.assign(reinterpret_cast<const char*>(p), n);
			p += n;
			break;

		default:
			info->sdl_info_dimensions = 0;
			if (vector)
			{
				memcpy(range.rng_minima, vector, sizeof(range.rng_minima));
				memcpy(range.rng_maxima, vector, sizeof(range.rng_maxima));
				range.rng_info = info;
				SLONG min = -1, max = -1;
				if (!(p = get_range(p - 1, &range, &min, &max)) || (*p != isc_sdl_eoc))
				{
					info->sdl_info_dimensions = 0;
				}
			}
			return FB_SUCCESS;
		}
}


// CVC: May revisit this function's tricky constness later.
const UCHAR* SDL_prepare_slice(const UCHAR* sdl, USHORT sdl_length)
{
/**************************************
 *
 *	S D L _ p r e p a r e _ s l i c e
 *
 **************************************
 *
 * Functional description
 *	Parse a slice and convert each occurrence of
 *	blr_d_float to blr_double.
 *
 **************************************/
	DSC	junk;
	USHORT	n;

	const UCHAR* const old_sdl = sdl;
	const UCHAR* new_sdl = sdl;

	if (*sdl++ != isc_sdl_version1)
		return old_sdl;

	while (*sdl != isc_sdl_eoc)
	{
		switch (*sdl++)
		{
		case isc_sdl_struct:
			for (n = *sdl++; n; --n)
			{
				if (*sdl == blr_d_float)
				{
					if (new_sdl == old_sdl)
					{
						UCHAR* temp_sdl = (UCHAR*)gds__alloc((SLONG) sdl_length);
						/* FREE: apparently never freed */
						if (!temp_sdl)
						{	/* NOMEM: ignore operation */
							fb_assert_continue(FALSE);	/* no real error handling */
							return old_sdl;
						}
						memcpy(temp_sdl, old_sdl, sdl_length);
						new_sdl = temp_sdl;
						sdl = new_sdl + (sdl - old_sdl);
					}
					// CVC: At this time, sdl points to new_sdl, so
					// throwing the constness away is safe.
					*const_cast<UCHAR*>(sdl) = blr_double;
				}

				// const_cast makes sense since we passed non-const object
				// to sdl_desc, so we got just another position in the same string.
				if (!(sdl = sdl_desc(sdl, &junk)))
					return new_sdl;
			}
			break;

		case isc_sdl_fid:
		case isc_sdl_rid:
			sdl += 2;
			break;

		case isc_sdl_field:
		case isc_sdl_relation:
			n = *sdl++;
			sdl += n;
			break;

		default:
			return new_sdl;
		}
	}

	return new_sdl;
}


int	SDL_walk(ISC_STATUS* status_vector,
			 const UCHAR* sdl,
			 UCHAR* array,
			 Ods::InternalArrayDesc* array_desc,
			 SLONG* variables,
			 SDL_walk_callback callback,
			 array_slice* argument)
{
/**************************************
 *
 *	S D L _ w a l k
 *
 **************************************
 *
 * Functional description
 *	Walk a slice.
 *
 **************************************/
	DSC junk;
	USHORT n, offset;
	sdl_arg arg;

	arg.sdl_arg_array = array;
	arg.sdl_arg_sdl = sdl;
	arg.sdl_arg_desc = array_desc;
	arg.sdl_arg_variables = variables;
	arg.sdl_arg_callback = callback;
	arg.sdl_arg_argument = argument;
	arg.sdl_arg_status_vector = status_vector;
	const UCHAR* p = sdl + 1;

	while (*p != isc_sdl_eoc)
	{
		switch (*p++)
		{
		case isc_sdl_struct:
			for (n = *p++; n; --n)
			{
				offset = p - sdl - 1;
				if (!(p = sdl_desc(p, &junk)))
					return error(status_vector, Arg::Gds(isc_invalid_sdl) << Arg::Num(offset));
			}
			break;

		case isc_sdl_fid:
		case isc_sdl_rid:
			p += 2;
			break;

		case isc_sdl_field:
		case isc_sdl_relation:
			n = *p++;
			p += n;
			break;

		default:
			/* Check that element is in range of valid SDL */
			fb_assert_continue(*(p - 1) >= isc_sdl_version1 && *(p - 1) <= isc_sdl_element);

			arg.sdl_arg_next = arg.sdl_arg_compiled;
			arg.sdl_arg_end = arg.sdl_arg_compiled + COMPILE_SIZE;
			if (!(p = compile(p - 1, &arg)))
				return FB_FAILURE;
			if (!stuff((IPTR) op_exit, &arg))
				return FB_FAILURE;
			if (!execute(&arg))
				return FB_FAILURE;
			break;
		}
    }

	return FB_SUCCESS;
}


static const UCHAR* compile(const UCHAR* sdl, sdl_arg* arg)
{
/**************************************
 *
 *	c o m p i l e
 *
 **************************************
 *
 * Functional description
 *	Compile an SDL element.  If the address of the argument block
 *	is null, parse, but do not generate anything.
 *
 **************************************/
	SLONG n, count, variable, value, sdl_operator;
	IPTR* label;
	const UCHAR* expressions[MAX_ARRAY_DIMENSIONS];
	const UCHAR** expr;

#define STUFF(word, arg)	if (!stuff ((IPTR) word, arg)) return NULL
#define COMPILE(p, arg)		if (!(p = compile (p, arg))) return NULL

	const UCHAR* ptr1;
	const UCHAR* p = sdl;

	UCHAR op = *p++;
	switch (op)
	{
	case isc_sdl_do1:
	case isc_sdl_do2:
	case isc_sdl_do3:
		variable = *p++;
		if (op == isc_sdl_do1)
			ptr1 = NULL;
		else
		{
			ptr1 = p;
			COMPILE(p, 0);		/* skip over lower bound */
		}
		COMPILE(p, arg);		/* upper bound */
		if (op == isc_sdl_do3) {
			COMPILE(p, arg);	/* increment */
		}
		else
		{
			STUFF(op_literal, arg);
			STUFF(1, arg);
		}
		if (ptr1) {
			COMPILE(ptr1, arg);	/* initial value */
		}
		else
		{
			STUFF(op_literal, arg);	/* default initial value */
			STUFF(1, arg);
		}
		STUFF(op_loop, arg);
		if (!(label = stuff(op_iterate, arg)))
			return NULL;
		STUFF(variable, arg);
		STUFF(0, arg);			/* future branch out address */
		if (!(p = compile(p, arg)))
			return NULL;
		STUFF(op_goto, arg);
		STUFF(label, arg);
		if (arg)
			label[2] = (IPTR) arg->sdl_arg_next;
		return p;

	case isc_sdl_variable:
		STUFF(op_variable, arg);
		STUFF(*p++, arg);
		return p;

	case isc_sdl_tiny_integer:
		value = (SCHAR) * p++;
		STUFF(op_literal, arg);
		STUFF(value, arg);
		return p;

	case isc_sdl_short_integer:
		value = (SSHORT) (p[0] | (p[1] << 8));
		STUFF(op_literal, arg);
		STUFF(value, arg);
		return p + 2;

	case isc_sdl_long_integer:
		value = (SLONG) (p[0] | (p[1] << 8) | ((SLONG) p[2] << 16) | ((SLONG) p[3] << 24));
		STUFF(op_literal, arg);
		STUFF(value, arg);
		return p + 4;

	case isc_sdl_add:
		sdl_operator = op_add;
	case isc_sdl_subtract:
		if (!sdl_operator)
			sdl_operator = op_subtract;
	case isc_sdl_multiply:
		if (!sdl_operator)
			sdl_operator = op_multiply;
	case isc_sdl_divide:
		if (!sdl_operator)
			sdl_operator = op_divide;
		COMPILE(p, arg);
		COMPILE(p, arg);
		STUFF(sdl_operator, arg);
		return p;

	case isc_sdl_scalar:
		op = *p++;
		count = *p++;
		if (arg && count != arg->sdl_arg_desc->iad_dimensions)
		{
			error(arg->sdl_arg_status_vector,
				  Arg::Gds(isc_invalid_dimension) << Arg::Num(arg->sdl_arg_desc->iad_dimensions) <<
													 Arg::Num(count));
			return NULL;
		}
		expr = expressions;
		for (n = count; n; --n)
		{
			*expr++ = p;
			COMPILE(p, 0);
		}
		while (expr > expressions)
		{
			if (!compile(*--expr, arg))
				return NULL;
		}
		STUFF(op_scalar, arg);
		STUFF(op, arg);
		STUFF(count, arg);
		return p;

	case isc_sdl_element:
		count = *p++;
		if (arg && count != 1)
		{
			error(arg->sdl_arg_status_vector, Arg::Gds(isc_datnotsup));
			/* Msg107: "data operation not supported" (arrays of structures) */
			return NULL;
		}
		expr = expressions;
		for (n = count; n; --n)
		{
			*expr++ = p;
			COMPILE(p, 0);
		}
		while (expr > expressions)
		{
			if (!compile(*--expr, arg))
				return NULL;
		}
		STUFF(op_element, arg);
		STUFF(count, arg);
		return p;

	default:
		error(arg->sdl_arg_status_vector, Arg::Gds(isc_invalid_sdl) << Arg::Num(p - arg->sdl_arg_sdl - 1));
		return NULL;
	}
}


static ISC_STATUS error(ISC_STATUS* status_vector, const Arg::StatusVector& v)
{
/**************************************
 *
 *	e r r o r
 *
 **************************************
 *
 * Functional description
 *	Post an error sequence to the status vector.  Since an error
 *	sequence can, in theory, be arbitrarily lock, pull a cheap
 *	trick to get the address of the argument vector.
 *
 **************************************/
	v.copyTo(status_vector);
	makePermanentVector(status_vector);

	return status_vector[1];
}


static bool execute(sdl_arg* arg)
{
/**************************************
 *
 *	e x e c u t e
 *
 **************************************
 *
 * Functional description
 *	Execute compiled slice description language.
 *
 **************************************/
	SLONG* variable;
	SLONG stack[64];
	SLONG value, count;
	dsc element_desc;

	Ods::InternalArrayDesc* array_desc = arg->sdl_arg_desc;
	const Ods::InternalArrayDesc::iad_repeat* const range_end =
		array_desc->iad_rpt + array_desc->iad_dimensions;
	SLONG* variables = arg->sdl_arg_variables;
	const IPTR* next = arg->sdl_arg_compiled;
	SLONG* stack_ptr = stack + FB_NELEM(stack);

	for (;;)
	{
		const SLONG x = *next++;
		switch (x)
		{
		case op_literal:
			*--stack_ptr = *next++;
			break;

		case op_variable:
			*--stack_ptr = variables[*next++];
			break;

		case op_add:
			value = *stack_ptr++;
			*stack_ptr += value;
			break;

		case op_subtract:
			value = *stack_ptr++;
			*stack_ptr -= value;
			break;

		case op_multiply:
			value = *stack_ptr++;
			*stack_ptr *= value;
			break;

		case op_divide:
			value = *stack_ptr++;
			*stack_ptr /= value;
			break;

		case op_goto:
			next = (IPTR*) *next;
			break;

		case op_loop:
			variable = variables + next[1];
			*variable = *stack_ptr++;
			if (*variable > stack_ptr[1])
			{
				next = (IPTR*) next[2];
				stack_ptr += 2;
			}
			else
				next += 3;
			break;

		case op_iterate:
			variable = variables + next[0];
			*variable += *stack_ptr;
			if (*variable > stack_ptr[1])
			{
				next = (IPTR *) next[1];
				stack_ptr += 2;
			}
			else
				next += 2;
			break;

		case op_scalar:
			{
				value = *next++;
				next++;				/* Skip count, unsupported. */
				SLONG subscript = 0;
				for (const Ods::InternalArrayDesc::iad_repeat* range = array_desc->iad_rpt;
					 range < range_end; ++range)
				{
					const SLONG n = *stack_ptr++;
					if (n < range->iad_lower || n > range->iad_upper)
					{
						error(arg->sdl_arg_status_vector, Arg::Gds(isc_out_of_bounds));
						return false;
					}
					subscript += (n - range->iad_lower) * range->iad_length;
				}
				element_desc = array_desc->iad_rpt[value].iad_desc;
				element_desc.dsc_address = arg->sdl_arg_array + (IPTR) element_desc.dsc_address +
					(array_desc->iad_element_length * subscript);

				/* Is this element within the array bounds? */
				fb_assert_continue(element_desc.dsc_address >= arg->sdl_arg_array);
				fb_assert_continue(element_desc.dsc_address + element_desc.dsc_length <=
									arg->sdl_arg_array + array_desc->iad_total_length);
				}
			break;

		case op_element:
			count = *next++;
			if (arg->sdl_arg_argument->slice_direction == array_slice::slc_writing_array)
			{
				/* Storing INTO array */

				 (*arg->sdl_arg_callback) (arg->sdl_arg_argument, count, &element_desc);
			}
			else
			{
				/* Fetching FROM array */

				if (element_desc.dsc_address < arg->sdl_arg_argument->slice_high_water)
				{
					(*arg->sdl_arg_callback) (arg->sdl_arg_argument, count, &element_desc);
				}
				else
				{
					dsc* slice_desc = &arg->sdl_arg_argument->slice_desc;
					slice_desc->dsc_address += arg->sdl_arg_argument->slice_element_length;
				}
			}
			break;

		case op_exit:
			return true;

		default:
			fb_assert_continue(FALSE);
			return false;
		}
	}
}


static const UCHAR* get_range(const UCHAR* sdl, array_range* arg,
							  SLONG* min, SLONG* max)
{
/**************************************
 *
 *	g e t _ r a n g e
 *
 **************************************
 *
 * Functional description
 *	Analyse a piece of slice description language to get bounds
 *	of array references.
 *
 **************************************/
	SLONG n, variable, value, min1, max1, min2, max2, junk1, junk2;
	sdl_info* info;

	const UCHAR* p = sdl;

	const UCHAR op = *p++;
	switch (op)
	{
	case isc_sdl_do1:
	case isc_sdl_do2:
	case isc_sdl_do3:
		variable = *p++;
		if (op == isc_sdl_do1)
			arg->rng_minima[variable] = 1;
		else
		{
			if (!(p = get_range(p, arg, &arg->rng_minima[variable], &junk1)))
				return NULL;
		}
		if (!(p = get_range(p, arg, &junk1, &arg->rng_maxima[variable])))
			return NULL;
		if (op == isc_sdl_do3)
		{
			if (!(p = get_range(p, arg, &junk1, &junk2)))
				return NULL;
		}
		return get_range(p, arg, min, max);

	case isc_sdl_variable:
		variable = *p++;
		*min = arg->rng_minima[variable];
		*max = arg->rng_maxima[variable];
		return p;

	case isc_sdl_tiny_integer:
		value = (SCHAR) * p++;
		*min = *max = value;
		return p;

	case isc_sdl_short_integer:
		value = (SSHORT) (p[0] | (p[1] << 8));
		*min = *max = value;
		return p + 2;

	case isc_sdl_long_integer:
		value = (SLONG) (p[0] | (p[1] << 8) | ((SLONG) p[2] << 16) | ((SLONG) p[3] << 24));
		*min = *max = value;
		return p + 4;

	case isc_sdl_add:
	case isc_sdl_subtract:
	case isc_sdl_multiply:
	case isc_sdl_divide:
		if (!(p = get_range(p, arg, &min1, &max1)))
			return NULL;
		if (!(p = get_range(p, arg, &min2, &max2)))
			return NULL;
		switch (op)
		{
		case isc_sdl_add:
			*min = min1 + min2;
			*max = max1 + max2;
			break;

		case isc_sdl_subtract:
			*min = min1 - max2;
			*max = max1 - min2;
			break;

		case isc_sdl_multiply:
			*min = min1 * min2;
			*max = max1 * max2;
			break;

		case isc_sdl_divide:
			return NULL;
		}
		return p;

	case isc_sdl_scalar:
		p++;
		info = arg->rng_info;
		info->sdl_info_dimensions = *p++;
		for (n = 0; n < info->sdl_info_dimensions; n++)
		{
			if (!(p = get_range(p, arg, &info->sdl_info_lower[n], &info->sdl_info_upper[n])))
			{
				return NULL;
			}
		}
		return p;

	case isc_sdl_element:
		for (n = *p++; n; --n)
		{
			if (!(p = get_range(p, arg, min, max)))
				return NULL;
		}
		return p;

	default:
		fb_assert_continue(FALSE);
		return NULL;
	}
}



static const UCHAR* sdl_desc(const UCHAR* ptr, DSC* desc)
{
/**************************************
 *
 *	s d l _ d e s c
 *
 **************************************
 *
 * Functional description
 *	Parse BLR descriptor into internal descriptor.
 *	Return updated pointer is successful, otherwise NULL.
 *
 **************************************/
	const UCHAR* sdl = ptr;
	desc->dsc_scale = 0;
	desc->dsc_length = 0;
	desc->dsc_sub_type = 0;
	desc->dsc_flags = 0;

	switch (*sdl++)
	{
	case blr_text2:
		desc->dsc_dtype = dtype_text;
		INTL_ASSIGN_TTYPE(desc, get_word(sdl));
		break;

	case blr_text:
		desc->dsc_dtype = dtype_text;
		INTL_ASSIGN_TTYPE(desc, ttype_dynamic);
		desc->dsc_flags |= DSC_no_subtype;
		break;

	case blr_cstring2:
		desc->dsc_dtype = dtype_cstring;
		INTL_ASSIGN_TTYPE(desc, get_word(sdl));
		break;

	case blr_cstring:
		desc->dsc_dtype = dtype_cstring;
		INTL_ASSIGN_TTYPE(desc, ttype_dynamic);
		desc->dsc_flags |= DSC_no_subtype;
		break;

	case blr_varying2:
		desc->dsc_dtype = dtype_cstring;
		INTL_ASSIGN_TTYPE(desc, get_word(sdl));
		desc->dsc_length = sizeof(USHORT);
		break;

	case blr_varying:
		desc->dsc_dtype = dtype_cstring;
		INTL_ASSIGN_TTYPE(desc, ttype_dynamic);
		desc->dsc_length = sizeof(USHORT);
		desc->dsc_flags |= DSC_no_subtype;
		break;

	case blr_short:
		desc->dsc_dtype = dtype_short;
		desc->dsc_length = sizeof(SSHORT);
		break;

	case blr_long:
		desc->dsc_dtype = dtype_long;
		desc->dsc_length = sizeof(SLONG);
		break;

	case blr_int64:
		desc->dsc_dtype = dtype_int64;
		desc->dsc_length = sizeof(SINT64);
		break;

	case blr_quad:
		desc->dsc_dtype = dtype_quad;
		desc->dsc_length = sizeof(ISC_QUAD);
		break;

	case blr_float:
		desc->dsc_dtype = dtype_real;
		desc->dsc_length = sizeof(float);
		break;

	case blr_double:
	case blr_d_float:
		desc->dsc_dtype = dtype_double;
		desc->dsc_length = sizeof(double);
		break;

	case blr_timestamp:
		desc->dsc_dtype = dtype_timestamp;
		desc->dsc_length = sizeof(ISC_QUAD);
		break;

	case blr_sql_date:
		desc->dsc_dtype = dtype_sql_date;
		desc->dsc_length = sizeof(SLONG);
		break;

	case blr_sql_time:
		desc->dsc_dtype = dtype_sql_time;
		desc->dsc_length = sizeof(ULONG);
		break;

	default:
		fb_assert_continue(FALSE);
		return NULL;
	}

	switch (desc->dsc_dtype)
	{
	case dtype_short:
	case dtype_long:
	case dtype_quad:
	case dtype_int64:
		desc->dsc_scale = static_cast<SCHAR>(*sdl++);
		break;

	case dtype_text:
	case dtype_cstring:
	case dtype_varying:
		desc->dsc_length += get_word(sdl);
		break;

	default:
		break;
	}

	return sdl;
}


static IPTR* stuff(IPTR value, sdl_arg* arg)
{
/**************************************
 *
 *	s t u f f
 *
 **************************************
 *
 * Functional description
 *	Stuff a longword into the compiled code space.
 *
 **************************************/

	if (!arg)
		return (IPTR*) TRUE;

	if (arg->sdl_arg_next >= arg->sdl_arg_end)
		error(arg->sdl_arg_status_vector, Arg::Gds(isc_virmemexh));
	/* unable to allocate memory from operating system */

	*(arg->sdl_arg_next)++ = value;

	return arg->sdl_arg_next - 1;
}

