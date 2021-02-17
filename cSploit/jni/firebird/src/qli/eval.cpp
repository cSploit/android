/*
 *	PROGRAM:	QLI Command Oriented Query Language
 *	MODULE:		eval.cpp
 *	DESCRIPTION:	Value and boolean expression evaluator
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
#include <stdio.h>
#include <string.h>
#include "../jrd/ibase.h"
#include "../qli/dtr.h"
#include "../qli/exe.h"
#include "../qli/parse.h"
#include "../qli/err_proto.h"
#include "../qli/eval_proto.h"
#include "../qli/exe_proto.h"
#include "../qli/lex_proto.h"
#include "../qli/mov_proto.h"
#include "../qli/picst_proto.h"
#include "../yvalve/gds_proto.h"
#include "../yvalve/utl_proto.h"
#include "../common/classes/UserBlob.h"
#include "../common/classes/VaryStr.h"
#include "../common/gdsassert.h"


static SLONG execute_any(qli_nod*);
static DSC* execute_concatenate(qli_nod*, const dsc*, const dsc*);
static DSC* execute_edit(qli_nod*);
static DSC* execute_function(qli_nod*);
static DSC* execute_prompt(qli_nod*);
static DSC* execute_statistical(qli_nod*);
static bool like(const UCHAR*, SSHORT, const UCHAR*, SSHORT, const UCHAR);
static TEXT* make_blob_buffer(FB_API_HANDLE, USHORT *);
static bool matches(const TEXT*, SSHORT, const TEXT*, SSHORT);
static bool sleuth(qli_nod*, const dsc*, const dsc*, const dsc*);
static bool sleuth_check(USHORT, const UCHAR*, const UCHAR* const, const UCHAR*, const UCHAR* const);
static bool sleuth_class(const USHORT, const UCHAR*, const UCHAR* const, UCHAR);
static int sleuth_merge(const UCHAR*, const UCHAR*, const UCHAR* const, UCHAR* const);
static bool string_boolean(qli_nod*);
static bool string_function(qli_nod*, SSHORT, const TEXT*, SSHORT, const TEXT*);

const int TEMP_LENGTH =	128;
const USHORT SLEUTH_insensitive	= 1;
//#define COND_UPPER(c)		((flags & SLEUTH_insensitive) ? UPPER(c) : c)
inline UCHAR cond_upper(const UCHAR c, const USHORT flags)
{
	return (flags & SLEUTH_insensitive) ? UPPER(c) : c;
}

static const UCHAR special[127] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
		0, 1, 1, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};

static TEXT prompt[2][128] = { "", "" };


int EVAL_boolean( qli_nod* node)
{
/**************************************
 *
 *	E V A L _ b o o l e a n
 *
 **************************************
 *
 * Functional description
 *	Evaluate a boolean expression.
 *
 **************************************/
	DSC *value1, *value2;
	int result;

	switch (node->nod_type)
	{
	case nod_eql:
	case nod_neq:
	case nod_gtr:
	case nod_geq:
	case nod_leq:
	case nod_lss:
	case nod_between:
		if (!(value1 = EVAL_value(node->nod_arg[0])) || (value1->dsc_missing & DSC_missing) ||
			!(value2 = EVAL_value(node->nod_arg[1])) || (value2->dsc_missing & DSC_missing))
		{
			return false;
		}
		if (node->nod_flags & nod_comparison)
			result = MOVQ_compare(value1, value2);
		break;

	case nod_and:
	case nod_or:
	case nod_not:
		result = EVAL_boolean(node->nod_arg[0]);
		break;

	case nod_containing:
	case nod_matches:
	case nod_like:
	case nod_starts:
	case nod_sleuth:
		result = string_boolean(node);
	}

	switch (node->nod_type)
	{
	case nod_eql:
		return (result == 0);
	case nod_neq:
		return (result != 0);
	case nod_gtr:
		return (result > 0);
	case nod_geq:
		return (result >= 0);
	case nod_leq:
		return (result <= 0);
	case nod_lss:
		return (result < 0);

	case nod_between:
		if (result < 0)
			return false;
		if (!(value2 = EVAL_value(node->nod_arg[2])) || (value2->dsc_missing & DSC_missing))
		{
			return false;
		}
		if (MOVQ_compare(value1, value2) > 0)
			return false;
		return true;

	case nod_missing:
		value1 = EVAL_value(node->nod_arg[0]);
		if (value1)
			return (value1->dsc_missing & DSC_missing) != 0;
		return true;

	case nod_and:
		if (!result)
			return false;
		return EVAL_boolean(node->nod_arg[1]);

	case nod_or:
		if (result)
			return true;
		return EVAL_boolean(node->nod_arg[1]);

	case nod_not:
		return !result;

	case nod_any:
	case nod_unique:
		return execute_any(node);

	case nod_sleuth:
	case nod_containing:
	case nod_matches:
	case nod_like:
	case nod_starts:
		return result;

	default:
		ERRQ_bugcheck(28);			// Msg28 EVAL_boolean: not finished
		return false;
	}
}


void EVAL_break_compute( qli_nod* node)
{
/**************************************
 *
 *	E V A L _ b r e a k _ c o m p u t e
 *
 **************************************
 *
 * Functional description
 *	Compute the value of a statistical expression at control
 *	break.  The only function that needs any work is average.
 *
 **************************************/

	if (node->nod_type == nod_rpt_average && node->nod_arg[e_stt_default])
	{
		if (node->nod_desc.dsc_dtype == dtype_long)
			*(SLONG *) node->nod_desc.dsc_address /= (IPTR) node->nod_arg[e_stt_default];
		else
			*(double *) node->nod_desc.dsc_address /= (IPTR) node->nod_arg[e_stt_default];
	}
}


void EVAL_break_increment( qli_nod* node)
{
/**************************************
 *
 *	E V A L _ b r e a k _ i n c r e m e n t
 *
 **************************************
 *
 * Functional description
 *	Initialize a report-local statistical function.
 *
 **************************************/
	DSC* desc1 = &node->nod_desc;

	// Knock off count as trivial

	if (node->nod_type == nod_rpt_count)
	{
		*(SLONG *) node->nod_desc.dsc_address += 1;
		return;
	}

	// Evaluate the sub-expression.  If null, don't bother to do anything
	// more.  If not, bump the number of records involved

	dsc* desc2 = EVAL_value(node->nod_arg[e_stt_value]);
	if (!desc2)
		return;

	// If this is the first value, just move it in.

	const SLONG count = (IPTR) node->nod_arg[e_stt_default] + 1;
	if (count == 1)
	{
		if (desc2->dsc_missing)
			desc1->dsc_missing = DSC_missing;
		else
		{
			desc1->dsc_missing = FALSE;
			MOVQ_move(desc2, desc1);
			node->nod_arg[e_stt_default] = (qli_nod*) (IPTR) count;
		}
		return;
	}
	if (desc2->dsc_missing)
		return;

	node->nod_arg[e_stt_default] = (qli_nod*) (IPTR) count;
	desc1->dsc_missing = FALSE;

	// Finish off as per operator

	SSHORT comparison;

	switch (node->nod_type)
	{
	case nod_rpt_min:
	case nod_rpt_max:
		if (!(comparison = MOVQ_compare(desc2, desc1)))
			break;
		if ((comparison > 0 && node->nod_type == nod_rpt_max) ||
			(comparison < 0 && node->nod_type == nod_rpt_min))
		{
			MOVQ_move(desc2, desc1);
		}
		break;

	case nod_rpt_total:
	case nod_rpt_average:
		if (desc1->dsc_dtype == dtype_long)
			*(SLONG *) desc1->dsc_address += MOVQ_get_long(desc2, desc1->dsc_scale);
		else
			*(double *) desc1->dsc_address += MOVQ_get_double(desc2);
		break;
	}
}


void EVAL_break_init( qli_nod* node)
{
/**************************************
 *
 *	E V A L _ b r e a k _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize a report-local statistical function.
 *
 **************************************/

	node->nod_arg[e_stt_default] = 0;
	node->nod_desc.dsc_missing = FALSE;

	if (node->nod_type == nod_rpt_count)
		*(SLONG *) (node->nod_desc.dsc_address) = 0;
}


dsc* EVAL_parameter(qli_par* parameter)
{
/**************************************
 *
 *	E V A L _ p a r a m e t e r
 *
 **************************************
 *
 * Functional description
 *	Compute the descriptor for a parameter.
 *
 **************************************/
	qli_par* missing_parameter;

	dsc* desc = &parameter->par_desc;
	desc->dsc_missing = FALSE;
	qli_msg* message = parameter->par_message;

	if (missing_parameter = parameter->par_missing)
	{
		const USHORT* missing_flag = (USHORT*) (message->msg_buffer + missing_parameter->par_offset);
		desc->dsc_missing = *missing_flag ? DSC_missing : 0;
	}

	desc->dsc_address = message->msg_buffer + parameter->par_offset;
	QLI_validate_desc(desc);

	return desc;
}


dsc* EVAL_value(qli_nod* node)
{
/**************************************
 *
 *	E V A L _ v a l u e
 *
 **************************************
 *
 * Functional description
 *	Evaluate a value node.
 *
 **************************************/
	DSC *values[4];

	// Start by evaluating sub-expressions (where appropriate)

	dsc* desc = &node->nod_desc;
	fb_assert(node->nod_count < 5);
	qli_nod** ptr = node->nod_arg;
	const qli_nod* const* const end_ptr = ptr + node->nod_count;

	for (dsc** value = values; ptr < end_ptr; ptr++, value++)
	{
		*value = EVAL_value(*ptr);
		if (node->nod_flags & nod_partial)
			break;
	}

	switch (node->nod_type)
	{
	case nod_edit_blob:
		return execute_edit(node);

	case nod_rpt_max:
	case nod_rpt_min:
	case nod_rpt_total:
	case nod_rpt_average:
		if (!(IPTR) node->nod_arg[e_stt_default])
			desc->dsc_missing = DSC_missing;

	case nod_rpt_count:
	case nod_constant:
		return desc;

	case nod_variable:
		{
			qli_fld* field = (qli_fld*) node->nod_arg[e_fld_field];
			desc->dsc_missing = (field->fld_flags & FLD_missing) ? DSC_missing : 0;
		}
		return desc;

	case nod_field:
		return EVAL_value(node->nod_arg[e_fld_reference]);

	case nod_reference:
		return EVAL_parameter(node->nod_import);

	case nod_null:
		return desc;

	case nod_add:
		if ((values[0]->dsc_missing & DSC_missing) || (values[1]->dsc_missing & DSC_missing))
		{
			desc->dsc_missing = DSC_missing;
			return desc;
		}
		desc->dsc_missing = FALSE;
		if (node->nod_flags & nod_date)
		{
			double d1 = MOVQ_date_to_double(values[0]) + MOVQ_get_double(values[1]);
			MOVQ_double_to_date(d1, (SLONG*) desc->dsc_address);
		}
		else if (desc->dsc_dtype == dtype_long)
		{
			*((SLONG*) desc->dsc_address) =
				MOVQ_get_long(values[0], desc->dsc_scale) + MOVQ_get_long(values[1], desc->dsc_scale);
		}
		else
			*((double*) desc->dsc_address) = MOVQ_get_double(values[0]) + MOVQ_get_double(values[1]);
		return desc;

	case nod_subtract:
		if ((values[0]->dsc_missing & DSC_missing) || (values[1]->dsc_missing & DSC_missing))
		{
			desc->dsc_missing = DSC_missing;
			return desc;
		}
		desc->dsc_missing = FALSE;
		if (node->nod_flags & nod_date)
		{
			*((double*) desc->dsc_address) =
				MOVQ_date_to_double(values[0]) - MOVQ_date_to_double(values[1]);
		}
		else if (desc->dsc_dtype == dtype_long)
		{
			*((SLONG*) desc->dsc_address) =
				MOVQ_get_long(values[0], desc->dsc_scale) - MOVQ_get_long(values[1], desc->dsc_scale);
		}
		else
			*((double*) desc->dsc_address) = MOVQ_get_double(values[0]) - MOVQ_get_double(values[1]);
		return desc;

	case nod_divide:
		if ((values[0]->dsc_missing & DSC_missing) || (values[1]->dsc_missing & DSC_missing))
		{
			desc->dsc_missing = DSC_missing;
			return desc;
		}
		desc->dsc_missing = FALSE;
		*((double*) desc->dsc_address) = MOVQ_get_double(values[0]) / MOVQ_get_double(values[1]);
		return desc;

	case nod_multiply:
		if ((values[0]->dsc_missing & DSC_missing) || (values[1]->dsc_missing & DSC_missing))
		{
			desc->dsc_missing = DSC_missing;
			return desc;
		}
		desc->dsc_missing = FALSE;
		if (desc->dsc_dtype == dtype_long)
		{
			*((SLONG*) desc->dsc_address) =
				MOVQ_get_long(values[0], values[0]->dsc_scale) *
				MOVQ_get_long(values[1], values[1]->dsc_scale);
		}
		else
			*((double*) desc->dsc_address) = MOVQ_get_double(values[0]) * MOVQ_get_double(values[1]);
		return desc;

	case nod_negate:
		if (values[0]->dsc_missing & DSC_missing)
		{
			desc->dsc_missing = DSC_missing;
			return desc;
		}
		desc->dsc_missing = FALSE;
		switch (desc->dsc_dtype)
		{
		case dtype_short:
			*((SSHORT*) desc->dsc_address) = -MOVQ_get_long(values[0], desc->dsc_scale);
			break;

		case dtype_long:
			*((SLONG*) desc->dsc_address) = -MOVQ_get_long(values[0], desc->dsc_scale);
			break;

		// lets throw arithmetic not supported until fixed
        //case dtype_int64:
		//	*((SINT64*) desc->dsc_address) = -MOVQ_get_long(values[0], desc->dsc_scale);
		//	break;

		case dtype_real:
			*((float*) desc->dsc_address) = -MOVQ_get_double(values[0]);
			break;

		case dtype_double:
			*((double*) desc->dsc_address) = -MOVQ_get_double(values[0]);
			break;

		default:
			IBERROR(30);		// Msg30 data type not supported for arithmetic
		}
		return desc;

	case nod_prompt:
		if (!prompt[0][0])
		{
			ERRQ_msg_get(499, prompt[0], sizeof(prompt[0]));	// Msg499 Re-enter
			ERRQ_msg_get(500, prompt[1], sizeof(prompt[1]));	// Msg500 Enter
		}
		return execute_prompt(node);

	case nod_concatenate:
		return execute_concatenate(node, values[0], values[1]);

	case nod_function:
		return execute_function(node);

	case nod_max:
	case nod_min:
	case nod_count:
	case nod_average:
	case nod_total:
	case nod_from:
		return execute_statistical(node);

	case nod_running_count:
		*(SLONG*) (desc->dsc_address) += 1;
		return desc;

	case nod_running_total:
		{
			dsc* desc2 = EVAL_value(node->nod_arg[e_stt_value]);
			if (desc2)
			{
				if (desc2->dsc_missing & DSC_missing)
					return desc;
				if (desc->dsc_dtype == dtype_long)
					*(SLONG*) desc->dsc_address += MOVQ_get_long(desc2, desc->dsc_scale);
				else
					*(double*) desc->dsc_address += MOVQ_get_double(desc2);
			}
		}
		return desc;

	case nod_format:
		{
			UCHAR* p = desc->dsc_address;
			PIC_edit(values[0], (pics*) node->nod_arg[e_fmt_picture], (TEXT**) &p, desc->dsc_length);
			desc->dsc_length = p - desc->dsc_address;
		}
		return desc;

	case nod_user_name:
		IBERROR(31);			// Msg31 user name is supported only in RSEs temporarily

	case nod_parameter:
	case nod_position:
	case nod_substr:
	case nod_via:

	default:
		ERRQ_bugcheck(29);			// Msg29 EVAL_value: not finished
		return NULL;
	}
}


static SLONG execute_any( qli_nod* node)
{
/**************************************
 *
 *	e x e c u t e _ a n y
 *
 **************************************
 *
 * Functional description
 *	Execute an ANY expression.  This may require that a request get
 *	started, a message sent, and a message received for each
 *	record.  At the other end of the spectrum, there may be
 *	absolutely nothing to do.
 *
 **************************************/
	qli_msg* message;

	// If there is a request associated  with the node, start it and possibly
	// send a message along with it.

	qli_req* request = (qli_req*) node->nod_arg[e_any_request];
	if (request)
		EXEC_start_request(request, (qli_msg*) node->nod_arg[e_any_send]);
	else if (message = (qli_msg*) node->nod_arg[e_any_send])
		EXEC_send(message);

	message = (qli_msg*) node->nod_arg[e_any_receive];
	EXEC_receive(message, 0);

	return MOVQ_get_long(EVAL_parameter(node->nod_import), 0);
}


static dsc* execute_concatenate( qli_nod* node, const dsc* value1, const dsc* value2)
{
/**************************************
 *
 *	e x e c u t e _ c o n c a t e n a t e
 *
 **************************************
 *
 * Functional description
 *	Concatenate two strings.
 *
 **************************************/
	Firebird::VaryStr<32> temp1;
	const TEXT* address1;
	USHORT length1 = MOVQ_get_string(value1, &address1, &temp1, sizeof(temp1));

	Firebird::VaryStr<32> temp2;
	const TEXT* address2;
	USHORT length2 = MOVQ_get_string(value2, &address2, &temp2, sizeof(temp2));

	dsc* desc = &node->nod_desc;
	vary* avary = (vary*) desc->dsc_address;
	TEXT* p = avary->vary_string;
	length1 = MIN(length1, desc->dsc_length - 2);
	length2 = MAX(MIN(length2, desc->dsc_length - 2 - length1), 0);
	fb_assert(static_cast<ULONG>(length1) + length2 <= MAX_USHORT - 2);

	if (length1)
	{
		memcpy(p, address1, length1);
		p += length1;
	}

	if (length2)
		memcpy(p, address2, length2);

	avary->vary_length = length1 + length2;

	return desc;
}


static DSC *execute_edit( qli_nod* node)
{
/**************************************
 *
 *	e x e c u t e _ e d i t
 *
 **************************************
 *
 * Functional description
 *	Edit a blob.  If there's an existing
 *	blob for input, force the descriptor
 *	to quad to get move the id.
 *
 **************************************/
	qli_dbb* dbb = (qli_dbb*) node->nod_arg[e_edt_dbb];
	ISC_QUAD* id = (ISC_QUAD*) & node->nod_arg[e_edt_id1];

	dsc* desc = NULL;

	if (node->nod_arg[e_edt_input])
	{
		desc = EVAL_value(node->nod_arg[e_edt_input]);
		if (desc && (desc->dsc_dtype == dtype_blob))
		{
			desc->dsc_dtype = dtype_quad;
			MOVQ_move(desc, &node->nod_desc);
			desc->dsc_dtype = dtype_blob;
		}
	}
	if (!desc)
		id->gds_quad_low = id->gds_quad_high = 0;

	const TEXT* field_name = (TEXT*) node->nod_arg[e_edt_name];
	BLOB_edit(id, dbb->dbb_handle, dbb->dbb_transaction, field_name);

	node->nod_desc.dsc_missing = UserBlob::blobIsNull(*id) ? DSC_missing : 0;

	return &node->nod_desc;
}


static DSC *execute_function( qli_nod* node)
{
/**************************************
 *
 *	e x e c u t e _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Execute a statistical expression.
 *
 **************************************/
	qli_msg* message;

	// If there is a request associated  with the node, start it and possibly
	// send a message along with it.

	qli_req* request = (qli_req*) node->nod_arg[e_fun_request];
	if (request)
		EXEC_start_request(request, (qli_msg*) node->nod_arg[e_fun_send]);
	else if (message = (qli_msg*) node->nod_arg[e_fun_send])
		EXEC_send(message);

	return EXEC_receive((qli_msg*) node->nod_arg[e_fun_receive], node->nod_import);
}


static DSC *execute_prompt( qli_nod* node)
{
/**************************************
 *
 *	e x e c u t e _ p r o m p t
 *
 **************************************
 *
 * Functional description
 *	Execute a prompt and return a descriptor.  If end of file,
 *	abort request; if a singleton tab, return the previous value,
 *	and if a null string, return MISSING.
 *
 **************************************/
	TEXT string[128], buffer[256];

	ERRQ_pending();
	bool reprompt = QLI_reprompt;
	dsc* desc = &node->nod_desc;
	vary* data = (vary*) desc->dsc_address;

	TEXT* value = (desc->dsc_length - 2 <= static_cast<int>(sizeof(buffer))) ?
		  buffer : data->vary_string;
	const int length = (desc->dsc_length - 2 <= static_cast<int>(sizeof(buffer))) ?
		  sizeof(buffer) : desc->dsc_length - 2;

	for (;;)
	{
		++QLI_prompt_count;
		if (node->nod_arg[e_prm_prompt])
		{
			if (reprompt)
				sprintf(string, "\07%s %s: ", prompt[0], (TEXT*) node->nod_arg[e_prm_prompt]);
			else
				sprintf(string, "%s %s: ", prompt[1], (TEXT*) node->nod_arg[e_prm_prompt]);
		}
		else
		{
			if (reprompt)
				sprintf(string, "\07%s: ", prompt[0]);	// Msg497 Re-enter
			else
				sprintf(string, "%s: ", prompt[1]);	// Msg498 Enter
		}

		if (!LEX_get_line(string, value, length))
		{
			QLI_abort = true;
			EXEC_poll_abort();
		}

		if (value[0] == '\t' && value[1] == '\n')
			return desc;

		TEXT* p = value + strlen(value);
		if (p > value && p[-1] == '\n')
			*--p = 0;

		// Get rid of trailing blanks on non-text data types

		if (desc->dsc_dtype > dtype_varying)
		{
			while (p > value && p[-1] == ' ')
				--p;
			*p = 0;
		}

		const int l = p - value;
		if (l <= desc->dsc_length - 2)
		{
			if (value != data->vary_string)
				memcpy(data->vary_string, value, l);
			data->vary_length = l;
			desc->dsc_missing = l ? 0 : DSC_missing;
			return desc;
		}

		ERRQ_msg_put(32);	// Msg32 Input value is too long
		reprompt = true;
	}
}


static DSC *execute_statistical( qli_nod* node)
{
/**************************************
 *
 *	e x e c u t e _ s t a t i s t i c a l
 *
 **************************************
 *
 * Functional description
 *	Execute a statistical expression.
 *
 **************************************/
	qli_msg* message;

	// If there is a request associated  with the node, start it and possibly
	// send a message along with it.

	qli_req* request = (qli_req*) node->nod_arg[e_stt_request];
	if (request)
		EXEC_start_request(request, (qli_msg*) node->nod_arg[e_stt_send]);
	else if (message = (qli_msg*) node->nod_arg[e_stt_send])
		EXEC_send(message);

	return EXEC_receive((qli_msg*) node->nod_arg[e_stt_receive], node->nod_import);
}

static bool like(const UCHAR* p1, SSHORT l1, const UCHAR* p2, SSHORT l2, const UCHAR escape_char)
{
/**************************************
 *
 *	l i k e
 *
 **************************************
 *
 * Functional description
 *	Return true if a string (p1, l1) matches a given pattern (p2, l2).
 *	The character '_' in the pattern may match any single character
 *	in the the string, and the character '%' may match any sequence
 *	of characters.
 *      Japanese version operates on short-based buffer,
 *      instead of char-based.
 *
 **************************************/
	bool escape = false;

	while (--l2 >= 0)
	{
		const UCHAR c = *p2++;
		if (escape_char && !escape && c == escape_char)
		{
			escape = true;
			continue;
		}
		if (!escape && c == '%')
		{
			if (l2 == 0)
				return true;
			while (l1)
			{
				if (like(p1++, l1--, p2, l2, escape_char))
					return true;
			}
			return false;
		}
		if (--l1 < 0)
			return false;
		if ((escape || c != '_') && c != *p1)
			return false;
		escape = false;
		p1++;
	}

	return l1 ? false : true;
}


static TEXT* make_blob_buffer(FB_API_HANDLE blob, USHORT* length)
{
/**************************************
 *
 *	m a k e _ b l o b _ b u f f e r
 *
 **************************************
 *
 * Functional description
 *	For a blob, get the max segment length,
 *	check that the buffer allocated is at least
 *	that length, and, if not, allocate a bigger one.
 *
 **************************************/
	SLONG size, segment_count, max_segment;

	gds__blob_size(&blob, &size, &segment_count, &max_segment);

	if (max_segment >= *length)
	{
		*length = max_segment;
		TEXT* buffer = (TEXT*) gds__alloc((SLONG) *length);
#ifdef DEBUG_GDS_ALLOC
		// We don't care about QLI specific memory leaks for V4.0
		gds_alloc_flag_unfreed((void *) buffer);	// QLI: don't care
#endif
		return buffer;
	}

	return 0;
}

static bool matches(const TEXT* p1, SSHORT l1, const TEXT* p2, SSHORT l2)
{
/**************************************
 *
 *	m a t c h e s
 *
 **************************************
 *
 * Functional description
 *	Return true if a string (p1, l1) matches a given pattern (p2, l2).
 *	The character '?' in the pattern may match any single character
 *	in the the string, and the character '*' may match any sequence
 *	of characters.
 *
 **************************************/
	while (--l2 >= 0)
	{
		const TEXT c = *p2++;
		if (c == '*')
		{
			if (l2 == 0)
				return true;
			while (l1)
			{
				if (matches(p1++, l1--, p2, l2))
					return true;
			}
			return false;
		}
		if (--l1 < 0 || (c != '?' && UPPER(c) != UPPER(*p1)))
			return false;
		p1++;
	}

	return l1 ? false: true;
}


static bool sleuth( qli_nod* node, const dsc* desc1, const dsc* desc2, const dsc* desc3)
{
/**************************************
 *
 *	s l e u t h
 *
 **************************************
 *
 * Functional description
 *	Return true if a string (p1, l1) matches a given pattern (p2, l2),
 *	using a pattern language defined in p3, l3.
 *
 **************************************/

	// Get operator definition string (control string)

	Firebird::VaryStr<TEMP_LENGTH> temp1;
	const TEXT* p1;
	SSHORT l1 = MOVQ_get_string(desc3, &p1, &temp1, TEMP_LENGTH);

	// Get address and length of search string

	Firebird::VaryStr<TEMP_LENGTH> temp2;
	const TEXT* p2;
	SSHORT l2 = MOVQ_get_string(desc2, &p2, &temp2, TEMP_LENGTH);

	// Merge search and control strings

	UCHAR control[256];
	l2 = sleuth_merge((const UCHAR*) p2, (const UCHAR*) p1, (const UCHAR*) (p1 + l1), control);

	// If source is not a blob, do a simple search

	if (desc1->dsc_dtype != dtype_blob)
	{
		l1 = MOVQ_get_string(desc1, &p1, &temp1, TEMP_LENGTH);
		return sleuth_check(0, (const UCHAR*) p1, (const UCHAR*) (p1 + l1), control, control + l2);
	}

	// Source string is a blob, things get interesting

	bool result = false;

	FB_API_HANDLE blob = EXEC_open_blob(node->nod_arg[0]);

	TEXT fixed_buffer[512];
	USHORT buffer_length = sizeof(fixed_buffer);

	TEXT* buffer = make_blob_buffer( blob, &buffer_length);
	if (!buffer)
		buffer = fixed_buffer;

	ISC_STATUS_ARRAY status_vector;
	while (!isc_get_segment(status_vector, &blob, (USHORT*) &l1, buffer_length, buffer))
		if (sleuth_check(0, (UCHAR*) buffer, (UCHAR*) (buffer + l1), control, control + l2))
		{
			result = true;
			break;
		}

	if (buffer != fixed_buffer)
		gds__free(buffer);

	if (isc_close_blob(status_vector, &blob))
	{
		qli_ctx* context = (qli_ctx*) node->nod_arg[e_fld_context];
		qli_req* request = context->ctx_request;
		qli_dbb* dbb = request->req_database;
		ERRQ_database_error(dbb, status_vector);
	}

	return result;
}

static bool sleuth_check(USHORT flags,
						 const UCHAR* search, const UCHAR* const end_search,
						 const UCHAR* match, const UCHAR* const end_match)
{
/**************************************
 *
 *	s l e u t h _ c h e c k
 *
 **************************************
 *
 * Functional description
 *	Evaluate the "sleuth" search operator.
 *
 **************************************/
	while (match < end_match)
	{
		UCHAR c = *match++;
		if ((c == '@' && (c = *match++)) || !special[c])
		{
			c = cond_upper(c, flags);
			if (match >= end_match || *match != '*')
			{
				if (search >= end_search)
					return false;
				const UCHAR d = *search++;
				if (c != cond_upper(d, flags))
					return false;
			}
			else
			{
				++match;
				for (;;)
				{
					if (sleuth_check(flags, search, end_search, match, end_match))
						return true;
					if (search < end_search)
					{
						const UCHAR d = *search++;
						if (c != cond_upper(d, flags))
							return false;
					}
					else
						return false;
				}
			}
		}
		else if (c == '?')
		{
			if (match >= end_match || *match != '*')
			{
				if (search >= end_search)
					return false;
				search++;
			}
			else
			{
				if (++match >= end_match)
					return true;
				for (;;)
				{
					if (sleuth_check(flags, search, end_search, match, end_match))
						return true;
					if (++search >= end_search)
						return false;
				}
			}
		}
		else if (c == '[')
		{
			const UCHAR* char_class = match;
			while (*match++ != ']')
			{
				if (match >= end_match)
					return false;
			}

			const UCHAR* const end_class = match - 1;
			if (match >= end_match || *match != '*')
			{
				if (!sleuth_class(flags, char_class, end_class, *search++))
					return false;
			}
			else
			{
				++match;
				for (;;)
				{
					if (sleuth_check(flags, search, end_search, match, end_match))
						return true;
					if (search < end_search)
					{
						if (!sleuth_class(flags, char_class, end_class, *search++))
							return false;
					}
					else
						return false;
				}
			}
		}
		else if (c == '+')
		{
			c = *match++;
			if (c == 'S' || c == 's')
				flags &= ~SLEUTH_insensitive;
		}
		else if (c == '-')
		{
			c = *match++;
			if (c == 'S' || c == 's')
				flags |= SLEUTH_insensitive;
		}
	}

	if (search < end_search)
		return false;

	return true;
}


static bool sleuth_class( const USHORT flags,
						const UCHAR* char_class, const UCHAR* const end_class,
						UCHAR character)
{
/**************************************
 *
 *	s l e u t h _ c l a s s
 *
 **************************************
 *
 * Functional description
 *	See if a character is a member of a class.
 *
 **************************************/
	bool result = true;
	character = cond_upper(character, flags);

	if (*char_class == '~')
	{
		++char_class;
		result = false;
	}

	while (char_class < end_class)
	{
		const UCHAR c = *char_class++;
		if (c == '@')
		{
			if (*char_class++ == character)
				return true;
		}
		else if (*char_class == '-')
		{
			char_class += 2;
			if (character >= c && character <= char_class[-1])
				return result;
		}
		else if (character == c)
			return result;
	}

	return !result;
}


static int sleuth_merge(const UCHAR* match,
						const UCHAR* control, const UCHAR* const end_control,
						UCHAR* const combined)
{
/**************************************
 *
 *	s l e u t h _ m e r g e
 *
 **************************************
 *
 * Functional description
 *	Merge the matching pattern and control strings to give a cannonical
 *	matching pattern.  Return the length of the combined string.
 *
 * 	What this routine does is to take the language template, strip off
 *	the prefix and put it in the output string, then parse the definitions
 *	into an array of character pointers.  The index array is the defined
 *	character.   The routine then takes the actual match pattern and uses
 *	the characters in it to index into the definitions to produce an equivalent
 *	pattern in the cannonical language.
 *
 *	The silly loop setting *v++ to zero initializes the array up to the
 *	highest character defined (also max_op).  Believe it or not, that part
 *	is not a bug.
 *
 **************************************/
	UCHAR* comb = combined;
	UCHAR* vector[128];
	UCHAR** v = vector;
	UCHAR temp[256];
	UCHAR* t = temp;

	// Parse control string into substitution strings and initializing string

	while (control < end_control)
	{
		UCHAR c = *control++;
		if (*control == '=')
		{
			UCHAR** end_vector = vector + c;
			while (v <= end_vector)
				*v++ = 0;
			*end_vector = t;
			++control;
			while (control < end_control)
			{
				c = *control++;
				if ((t[-1] == '@') || ((c != ',') && (c != ')')))
					*t++ = c;
				else
					break;
			}
			*t++ = 0;
		}
		else if (c == '@' && control < end_control)
			*comb++ = *control++;
		else if (c == ')')
			break;
		else if (c != '(')
			*comb++ = c;
	}

	const UCHAR max_op = v - vector;

	// Interpret matching string, substituting where appropriate

	UCHAR c;
	while (c = *match++)
	{
	    const UCHAR* p;

		// if we've got a defined character, slurp the definition
		if (c <= max_op && (p = vector[c]))
		{
			while (*p)
				*comb++ = *p++;
			// if we've got the definition of a quote character, slurp the next character too
			if (comb[-1] == '@' && *match)
				*comb++ = *match++;
		}
		// at this point we've got a non-match, but as it might be one of ours, quote it.
		else
		{
			if (special[c] && comb[-1] != '@')
				*comb++ = '@';
			*comb++ = c;
		}
	}

	// Put in trailing stuff

	while (control < end_control)
		*comb++ = *control++;

	return comb - combined;
}


static bool string_boolean( qli_nod* node)
{
/**************************************
 *
 *	s t r i n g _ b o o l e a n
 *
 **************************************
 *
 * Functional description
 *	Perform one of the complex string functions CONTAINING, MATCHES,
 *	or STARTS WITH.
 *
 **************************************/
	const DSC *desc1, *desc2, *desc3;

	if (!(desc1 = EVAL_value(node->nod_arg[0])) || (desc1->dsc_missing & DSC_missing) ||
		!(desc2 = EVAL_value(node->nod_arg[1])) || (desc2->dsc_missing & DSC_missing) ||
		(node->nod_arg[2] && (!(desc3 = EVAL_value(node->nod_arg[2])) ||
							  (desc3->dsc_missing & DSC_missing))))
	{
		return false;
	}

	if (node->nod_type == nod_sleuth)
		return sleuth(node, desc1, desc2, desc3);

	// Get address and length of strings

	const TEXT* p2;
	Firebird::VaryStr<TEMP_LENGTH> temp2;
	SSHORT l2 = MOVQ_get_string(desc2, &p2, &temp2, TEMP_LENGTH);

	// If source is not a blob, do a simple search

	if (desc1->dsc_dtype != dtype_blob)
	{
		Firebird::VaryStr<TEMP_LENGTH> temp1;
		const TEXT* p1;
		SSHORT l1 = MOVQ_get_string(desc1, &p1, &temp1, TEMP_LENGTH);
		return string_function(node, l1, p1, l2, p2);
	}

	// Source string is a blob, things get interesting

	bool result = false;
	FB_API_HANDLE blob = EXEC_open_blob(node->nod_arg[0]);

    TEXT fixed_buffer[512];
	USHORT buffer_length = sizeof(fixed_buffer);

	TEXT* buffer = make_blob_buffer( blob, &buffer_length);
	if (!buffer)
		buffer = fixed_buffer;

	ISC_STATUS_ARRAY status_vector;
	SSHORT l3 = 0;
	while (!isc_get_segment(status_vector, &blob, (USHORT*) &l3, buffer_length, buffer))
	{
		if (string_function(node, l3, buffer, l2, p2))
		{
			result = true;
			break;
		}
	}

	if (buffer != fixed_buffer)
		gds__free(buffer);

	if (isc_close_blob(status_vector, &blob))
	{
		qli_ctx* context = (qli_ctx*) node->nod_arg[e_fld_context];
		qli_req* request = context->ctx_request;
		qli_dbb* database = request->req_database;
		ERRQ_database_error(database, status_vector);
	}

	return result;
}


static bool string_function(qli_nod* node,
						   SSHORT l1, const TEXT* p1, SSHORT l2, const TEXT* p2)
{
/**************************************
 *
 *	s t r i n g _ f u n c t i o n
 *
 **************************************
 *
 * Functional description
 *	Perform one of the complex string functions CONTAINING, MATCHES,
 *	or STARTS WITH.
 *
 **************************************/

	// Handle "STARTS WITH"

	if (node->nod_type == nod_starts)
	{
		if (l1 < l2)
			return false;

		if (l2)
			return memcmp(p1, p2, l2) == 0;

		return true;
	}

	// Handle CONTAINS

	if (node->nod_type == nod_containing)
	{
		while (l1 >= l2)
		{
			--l1;
			const TEXT* q1 = p1++;
			const TEXT* q2 = p2;
			SSHORT l = l2;
			TEXT c1, c2;
			do {
				if (--l < 0)
					return true;
				c1 = *q1++;
				c2 = *q2++;
			} while (UPPER(c1) == UPPER(c2));
		}
		return false;
	}

	// Handle LIKE

	if (node->nod_type == nod_like)
	{
		TEXT c1 = 0;
		Firebird::VaryStr<16> temp;
		const TEXT* q1 = NULL;
		if (node->nod_count > 2 &&
			MOVQ_get_string(EVAL_value(node->nod_arg[2]), &q1, &temp, sizeof(temp)))
		{
			c1 = *q1;
		}
		if (like((const UCHAR*) p1, l1, (const UCHAR*) p2, l2, c1))
			return true;
		return false;
	}

	// Handle MATCHES

	return node->nod_type == nod_matches && matches(p1, l1, p2, l2);
}


