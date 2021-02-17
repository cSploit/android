/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		format.cpp
 *	DESCRIPTION:	Print planner and formatter
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
#include "../qli/dtr.h"
#include "../qli/exe.h"
#include "../jrd/ibase.h"
#include "../qli/compile.h"
#include "../qli/report.h"
#include "../qli/format.h"
#include "../qli/all_proto.h"
#include "../qli/err_proto.h"
#include "../qli/eval_proto.h"
#include "../qli/exe_proto.h"
#include "../qli/forma_proto.h"
#include "../qli/mov_proto.h"
#include "../qli/picst_proto.h"

using MsgFormat::SafeArg;


#ifdef DEV_BUILD
//extern bool QLI_hex_output; decl already done in dtr.h

inline bool is_printable(UCHAR x)
{
	return ((x >= ' ') && (x <= 127)) || (x == '\n') || (x == '\t') || (x == '\r') || (x == '\f');
}
#endif

static USHORT decompose_header(const SCHAR*, const SCHAR**, USHORT*);
static void format_index(qli_print_item*, qli_nod*, const bool);
static TEXT* format_report(qli_vec*, USHORT, USHORT*);
static void format_value(qli_print_item*, int);
static TEXT* get_buffer(qli_str**, TEXT*, USHORT);
static bool match_expr(const qli_nod*, const qli_nod*);
static void print_blobs(qli_prt*, qli_print_item**, qli_print_item**);
static int print_line(qli_print_item*, TEXT**);
static void put_line(qli_prt*, TEXT**, TEXT*, TEXT);
static void report_break(qli_brk*, qli_vec**, const bool);
static void report_item(qli_print_item*, qli_vec**, USHORT*);
static void report_line(qli_nod*, qli_vec**);

static qli_str* global_fmt_buffer;
static qli_str* global_blob_buffer;

#define BOTTOM_INIT			get_buffer (&global_fmt_buffer, NULL, 1024)
#define BOTTOM_CHECK(ptr, length)	ptr = get_buffer (&global_fmt_buffer, ptr, length)
#define BOTTOM_LINE			global_fmt_buffer->str_data

#define BUFFER_INIT			get_buffer (&global_fmt_buffer, NULL, 1024)
#define BUFFER_CHECK(ptr, length)	ptr = get_buffer (&global_fmt_buffer, ptr, length)
#define BUFFER_BEGINNING		global_fmt_buffer->str_data
#define BUFFER_REMAINING(ptr)		(global_fmt_buffer->str_length - (ptr - global_fmt_buffer->str_data))

int FMT_expression( qli_nod* node)
{
/**************************************
 *
 *	F M T _ e x p r e s s i o n
 *
 **************************************
 *
 * Functional description
 *	Handle formatting for FORMAT expression.  Return editted
 *	length.
 *
 **************************************/
	qli_nod* sub = node->nod_arg[e_fmt_value];
	pics* picture = PIC_analyze((const TEXT*) node->nod_arg[e_fmt_edit], &sub->nod_desc);
	node->nod_arg[e_fmt_picture] = (qli_nod*) picture;

	if (node->nod_type == nod_reference)
		node = node->nod_arg[0];

	qli_fld* field;
	if (!(picture->pic_missing) && (node->nod_type == nod_field) &&
		(field = (qli_fld*) node->nod_arg[e_fld_field]) && field->fld_missing)
	{
		PIC_missing(field->fld_missing, picture);
	}

	return picture->pic_length;
}


TEXT* FMT_format(qli_lls* stack)
{
/**************************************
 *
 *	F M T _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Format a print line.  Figure out spacing, print formats, and
 *	headers.  Return a pointer to the header string.
 *
 **************************************/
	USHORT lengths[10];
	const TEXT* segments[10];

	// Start by inverting the item stack into an item que

	qli_lls* temp = stack;
	stack = NULL;

	if (global_fmt_buffer)
	{
		ALLQ_release((qli_frb*) global_fmt_buffer);
		global_fmt_buffer = NULL;
	}

	while (temp)
	{
		qli_print_item* item = (qli_print_item*) ALLQ_pop(&temp);
		ALLQ_push((blk*) item, &stack);
	}

	// Make a pass thru print items computing print lengths and header
	// lengths, and the number of header segments.

	USHORT offset, max_offset, number_segments;
	number_segments = offset = max_offset = 0;
	TEXT* bottom = BOTTOM_INIT;

	for (temp = stack; temp; temp = temp->lls_next)
	{
		qli_print_item* item = (qli_print_item*) temp->lls_object;
		switch (item->itm_type)
		{
		case item_column:
			offset = (item->itm_count) ? item->itm_count - 1 : 0;
			continue;

		case item_skip:
		case item_new_page:
			offset = 0;
			continue;

		case item_space:
			offset += item->itm_count;
			continue;

		case item_tab:
			offset = (offset & ~7) + item->itm_count * 8;
			continue;
		}

		qli_nod* value = 0;
		if (item->itm_type == item_value && (value = item->itm_value))
		{
			if (value->nod_type == nod_reference)
				value = value->nod_arg[0];
			format_index(item, value, true);
		}

		if (item->itm_query_header)
		{
			if (*item->itm_query_header == '-')
				item->itm_query_header = NULL;
			else
			{
				const USHORT n = decompose_header(item->itm_query_header, segments, lengths);
				number_segments = MAX(n, number_segments);
				USHORT* ptr = lengths;
				for (USHORT j = 0; j < n; j++, ptr++)
					item->itm_header_length = MAX(item->itm_header_length, *ptr);
			}
		}

		format_value(item, 0);

		// If the item would overflow the line, reset to beginning of line

		if (offset + MAX(item->itm_print_length, item->itm_header_length) > QLI_columns)
			offset = 0;

		// Before we blindly format the header, make sure there already isn't
		// header information in the same location

		if (item->itm_query_header)
		{
			const USHORT n = MAX(item->itm_print_length, item->itm_header_length);
			BOTTOM_CHECK(bottom, offset);
			const TEXT* q = BOTTOM_LINE + offset;
			while (bottom < q)
				*bottom++ = ' ';
			bool flag = true;
			USHORT l;
			if (offset && q[-1] != ' ')
				flag = false;
			else if (l = MIN(n, bottom - q))
			{
				const TEXT* p = bottom;
				while (--l)
					if (*p++ != ' ')
					{
						flag = false;
						break;
					}
				if (flag && p < bottom && *p != ' ')
					flag = false;
			}
			if (flag && (l = n))
			{
				BOTTOM_CHECK(bottom, bottom - BOTTOM_LINE + n);
				do {
					*bottom++ = '=';
				} while (--l);
			}
			else
			{
				item->itm_query_header = NULL;
				item->itm_header_length = 0;
			}
		}

		// Now that have settled the issue of header, decide where to put
		// the field and header

		const USHORT n = MAX(item->itm_print_length, item->itm_header_length);
		item->itm_print_offset = offset + (n - item->itm_print_length) / 2;
		item->itm_header_offset = offset + n / 2;
		offset += n + 1;
		max_offset = MAX(max_offset, offset);
	}

	// Make another pass checking for overlapping fields

	for (temp = stack; temp; temp = temp->lls_next)
	{
		qli_print_item* item = (qli_print_item*) temp->lls_object;
		if (item->itm_type != item_value)
			continue;
		for (qli_lls* temp2 = temp->lls_next; temp2; temp2 = temp2->lls_next)
		{
			qli_print_item* item2 = (qli_print_item*) temp2->lls_object;
			if (item2->itm_type != item_value)
				continue;
			if (item2->itm_print_offset < item->itm_print_offset + item->itm_print_length)
			{
				item->itm_flags |= ITM_overlapped;
				break;
			}
		}
	}

	if (number_segments == 0)
		return NULL;

	// Allocate a string block big enough to hold all lines of the print header

	const ULONG size = (max_offset + 1) * (number_segments + 1) + 2;

	if (size >= 60000)
		ERRQ_print_error(482, SafeArg() << max_offset << (number_segments + 1));

	qli_str* header = (qli_str*) ALLOCDV(type_str, size);
	TEXT* p = header->str_data;

	// Generate the various lines of the header line at a time.

	for (USHORT j = 0; j < number_segments; j++)
	{
		*p++ = '\n';
		TEXT* const line = p;
		for (temp = stack; temp; temp = temp->lls_next)
		{
			qli_print_item* item = (qli_print_item*) temp->lls_object;
			if (item->itm_type != item_value)
				continue;
			const USHORT n = decompose_header(item->itm_query_header, segments, lengths);
			const SSHORT segment = j - (number_segments - n);
			if (segment < 0)
				continue;
			USHORT l = lengths[segment];
			const TEXT* q = line + item->itm_header_offset - l / 2;

			while (p < q)
				*p++ = ' ';
			q = segments[segment];
			if (l)
				do {
					*p++ = *q++;
				} while (--l);
		}
	}

	// Make one last pass to put in underlining of headers

	USHORT len = bottom - BOTTOM_LINE;
	if (len)
	{
		*p++ = '\n';
		bottom = BOTTOM_LINE;
		do {
			*p++ = *bottom++;
		} while (--len);
	}

	*p++ = '\n';
	*p++ = '\n';
	*p = 0;

	return header->str_data;
}


qli_nod* FMT_list(qli_nod* list)
{
/**************************************
 *
 *	F M T _ l i s t
 *
 **************************************
 *
 * Functional description
 *	Rebuild and format a list of stuff for vertical formatting.
 *
 **************************************/
	qli_print_item** item;
	const qli_print_item* const* end;
	qli_symbol* name;
	qli_fld* field;
	qli_nod* value;

	qli_nod* new_nod = (qli_nod*) ALLOCDV(type_nod, list->nod_count * 2 + 1);
	new_nod->nod_type = nod_list;
	qli_print_item** new_ptr = (qli_print_item**) new_nod->nod_arg;
	USHORT column = 0;

	for (item = (qli_print_item**) list->nod_arg, end = item + list->nod_count; item < end; item++)
	{
		if ((*item)->itm_type != item_value || !(value = (*item)->itm_value))
			continue;
		(*item)->itm_flags |= ITM_overlapped;
		format_value(*item, PIC_suppress_blanks);
		if (value->nod_type == nod_reference)
			value = value->nod_arg[0];
		bool expression = true;
		if (value->nod_type == nod_field || value->nod_type == nod_variable ||
			value->nod_type == nod_function)
		{
			expression = false;
			if (value->nod_type != nod_function)
			{
				field = (qli_fld*) value->nod_arg[e_fld_field];
				name = field->fld_name;
				format_index(*item, value, false);
			}
			else
				name = ((qli_fun*) value->nod_arg[e_fun_function])->fun_symbol;
		}
		qli_print_item* new_item = (qli_print_item*) ALLOCD(type_itm);
		*new_ptr++ = new_item;
		new_item->itm_type = item_value;
		new_item->itm_value = value = (qli_nod*) ALLOCDV(type_nod, 0);
		value->nod_type = nod_constant;
		value->nod_flags |= NOD_local;
		value->nod_desc.dsc_dtype = dtype_text;
		const TEXT* q;
		if (!expression && (!(q = (*item)->itm_query_header) || *q != '-'))
		{
			if (q)
			{
				if (*q != '"' && *q != '\'')
					value->nod_desc.dsc_address = (UCHAR*) q;
				else
				{
					qli_str* header = (qli_str*) ALLOCDV(type_str, strlen(q));
					TEXT* p = header->str_data;
					value->nod_desc.dsc_address = (UCHAR*) p; // safe const_cast, see PIC_analyze
					TEXT c;
					while (c = *q++)
					{
						while (*q != c)
							*p++ = *q++;
						*p++ = ' ';
						q++;
					}
					p[-1] = 0;
				}
				value->nod_desc.dsc_length = strlen((char*) value->nod_desc.dsc_address);
			}
			else
			{
				value->nod_desc.dsc_length = name->sym_length;
				value->nod_desc.dsc_address = (UCHAR*) name->sym_string;
			}
			QLI_validate_desc(value->nod_desc);
			column = MAX(column, value->nod_desc.dsc_length);
			new_item->itm_picture = PIC_analyze(0, &value->nod_desc);
		}
		else
		{
			const dsc* desc = EVAL_value(value);
			new_item->itm_picture = PIC_analyze(0, desc);
		}

		if (!new_item->itm_picture->pic_missing &&
			value->nod_type == nod_field && field->fld_missing)
		{
			PIC_missing(field->fld_missing, new_item->itm_picture);
		}

		*new_ptr++ = *item;
	}

	qli_print_item* new_item = (qli_print_item*) ALLOCD(type_itm);
	*new_ptr++ = new_item;
	new_item->itm_type = item_skip;
	new_item->itm_count = 1;
	column += 2;

	for (item = (qli_print_item**) list->nod_arg, end = item + list->nod_count; item < end; item++)
	{
		if ((*item)->itm_type != item_value || !(value = (*item)->itm_value))
			continue;
		if (value->nod_type == nod_reference)
			value = value->nod_arg[0];
		(*item)->itm_print_offset = column;
	}

	new_nod->nod_count = new_ptr - (qli_print_item**) new_nod->nod_arg;

	return new_nod;
}


void FMT_print( qli_nod* list, qli_prt* print)
{
/**************************************
 *
 *	F M T _ p r i n t
 *
 **************************************
 *
 * Functional description
 *	Format a print line.  Return the number of lines printed.
 *
 **************************************/
	qli_nod** ptr;

	// Now go thru and make up the first line

	if (!list)
		return;

	TEXT* buffer = NULL;
	TEXT* p = BUFFER_INIT;
	const qli_nod* const* const end = list->nod_arg + list->nod_count;

	for (ptr = list->nod_arg; ptr < end; ptr++)
	{
		qli_print_item* item = (qli_print_item*) *ptr;

		// Handle formating directives.  Most have been translated into
		// column assignments and are no-ops.

		buffer = BUFFER_BEGINNING;

		switch (item->itm_type)
		{
		case item_value:
			break;

		case item_new_page:
			if (print->prt_new_page)
			{
				put_line(print, &p, buffer, '\n');
				(*print->prt_new_page) (print, false);
			}
			else
			{
				put_line(print, &p, buffer, '\f');
				QLI_skip_line = false;
			}
			continue;

		case item_skip:
			{
				put_line(print, &p, buffer, '\n');
				print_blobs(print, (qli_print_item**) list->nod_arg, (qli_print_item**) ptr);
				for (USHORT l = item->itm_count - 1; l > 0; --l)
					put_line(print, &p, buffer, '\n');
				QLI_skip_line = false;
				continue;
			}

		case item_column_header:
			{
				qli_rpt* report = print->prt_report;
				if (report && report->rpt_column_header)
					FMT_put(report->rpt_column_header, print);
				continue;
			}

		case item_report_header:
			{
				qli_rpt* report = print->prt_report;
				if (report && report->rpt_header)
					FMT_put(report->rpt_header, print);
				continue;
			}

		case item_column:
		case item_tab:
		case item_space:
		default:
			continue;
		}

		// Handle print items.  Start by by spacing out to the correct column,
		// forcing a new line if required.

		BUFFER_CHECK(p, item->itm_print_offset + item->itm_print_length + 2);
		buffer = BUFFER_BEGINNING;
		const TEXT* const q = buffer + item->itm_print_offset;
		if (p > q)
		{
			put_line(print, &p, buffer, '\n');
			print_blobs(print, (qli_print_item**) list->nod_arg, (qli_print_item**) ptr);
		}
		while (p < q)
			*p++ = ' ';

		// Next, handle simple formated values

		if (item->itm_dtype != dtype_blob)
		{
			const dsc* desc = EVAL_value(item->itm_value);
			if (!(desc->dsc_missing & DSC_missing))
				PIC_edit(desc, item->itm_picture, &p, BUFFER_REMAINING(p));
			else if (item->itm_picture->pic_missing)
				PIC_edit(desc, item->itm_picture->pic_missing, &p, BUFFER_REMAINING(p));
			continue;
		}

		// Finally, handle blobs

		if (!(item->itm_stream = EXEC_open_blob(item->itm_value)))
			continue;

		if (print_line(item, &p) != EOF)
		{
			if (item->itm_flags & ITM_overlapped)
			{
				for (;;)
				{
					put_line(print, &p, buffer, '\n');
					while (p < q)
						*p++ = ' ';
					if (print_line(item, &p) == EOF)
						break;
				}
			}
		}
	}

	put_line(print, &p, buffer, '\n');

	// Now go back until all blobs have been fetched

	print_blobs(print, (qli_print_item**) list->nod_arg, (qli_print_item**) end);

	// Finish by closing all blobs
	ISC_STATUS_ARRAY status_vector;
	for (ptr = list->nod_arg; ptr < end; ptr++)
	{
		qli_print_item* item = (qli_print_item*) *ptr;
		if (item->itm_dtype == dtype_blob && item->itm_stream)
			isc_close_blob(status_vector, &item->itm_stream);
	}
}


void FMT_put(const TEXT* line, qli_prt* print)
{
/**************************************
 *
 *	F M T _ p u t
 *
 **************************************
 *
 * Functional description
 *	Write out an output file.
 *
 **************************************/
	for (const TEXT* pnewline = line; *pnewline; pnewline++)
	{
		if (*pnewline == '\n' || *pnewline == '\f')
			--print->prt_lines_remaining;
	}

	if (print && print->prt_file)
	{
		fprintf(print->prt_file, "%s", line);
	}
	else
	{
#ifdef DEV_BUILD
		if (QLI_hex_output)
		{
			// Hex mode output to assist debugging of multicharset work

			for (const TEXT* p = line; *p; p++)
			{
				if (is_printable(*p))
					fprintf(stdout, "%c", *p);
				else
					fprintf(stdout, "[%2.2X]", *(UCHAR*) p);
			}
		}
		else
#endif
			fprintf(stdout, "%s", line);
		QLI_skip_line = true;
	}
}


void FMT_report( qli_rpt* report)
{
/**************************************
 *
 *	F M T _ r e p o r t
 *
 **************************************
 *
 * Functional description
 *	Format a report.
 *
 **************************************/
	if (global_fmt_buffer)
	{
		ALLQ_release((qli_frb*) global_fmt_buffer);
		global_fmt_buffer = NULL;
	}

	USHORT width = report->rpt_columns;
	qli_vec* columns_vec = (qli_vec*) ALLOCDV(type_vec, 256);
	columns_vec->vec_count = 256;
	columns_vec->vec_object[0] = NULL;

	report_break(report->rpt_top_rpt, &columns_vec, false);
	report_break(report->rpt_top_page, &columns_vec, false);
	report_break(report->rpt_top_breaks, &columns_vec, false);

	qli_nod* list = report->rpt_detail_line;
	if (list)
		report_line(list, &columns_vec);

	report_break(report->rpt_bottom_breaks, &columns_vec, true);
	report_break(report->rpt_bottom_page, &columns_vec, true);
	report_break(report->rpt_bottom_rpt, &columns_vec, true);

	report->rpt_column_header = format_report(columns_vec, width, &width);

	// Handle report name, if any

	if (report->rpt_name)
	{
		USHORT lengths[16];
		const TEXT* segments[16];
		const USHORT n = decompose_header(report->rpt_name, segments, lengths);
		USHORT i;
		for (i = 0; i < n; i++)
			width = MAX(width, lengths[i] + 15);

		qli_str* string = (qli_str*) ALLOCDV(type_str, width * n);
		TEXT* p = string->str_data;
		report->rpt_header = p;
		for (i = 0; i < n; i++)
		{
			USHORT column = (width - lengths[i]) / 2;
			if (column > 0)
				do {
					*p++ = ' ';
				} while (--column);
			const TEXT* q = segments[i];
			const TEXT* const end = q + lengths[i];
			while (q < end)
				*p++ = *q++;
			*p++ = '\n';
		}
	}
}


static USHORT decompose_header(const SCHAR* string, const SCHAR** segments, USHORT* lengths)
{
/**************************************
 *
 *	d e c o m p o s e _ h e a d e r
 *
 **************************************
 *
 * Functional description
 *	Decompose a header string (aka field name) into segments.
 *	Return the address of and length of each segment (in arrays)
 *	and the number of segments.
 *
 **************************************/
	if (!string)
		return 0;

	USHORT n = 0;

	// Handle simple name first

	if (*string != '"' && *string != '\'')
	{
		while (*string)
		{
			*segments = string;
			while (*string && *string != '_')
				string++;
			*lengths++ = string - *segments++;
			++n;
			if (*string == '_')
				string++;
		}
	}
	else
	{
		TEXT c;
		while (c = *string++)
		{
			*segments = string;
			while (*string++ != c);
			*lengths++ = string - *segments++ - 1;
			++n;
		}
	}

	return n;
}


static void format_index( qli_print_item* item, qli_nod* field, const bool print_flag)
{
/**************************************
 *
 *	f o r m a t _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Format the label of a subscripted item.
 *
 **************************************/
	qli_nod* args = 0;

	// Don't bother with anything except non-indexed fields.  Also
	// ignore subscripted fields with user specified query headers.

	{ // scope
		const TEXT* qh;
		if (field->nod_type != nod_field || !(args = field->nod_arg[e_fld_subs]) ||
			((qh = item->itm_query_header) && (*qh == '"' || *qh == '\'')))
		{
			return;
		}
	} // scope

	// Start the label with the current query header, if any

	USHORT l;
	const TEXT* q;
	if (item->itm_query_header)
	{
		q = item->itm_query_header;
		l = strlen(item->itm_query_header);
	}
	else
	{
		q = ((qli_fld*) field->nod_arg[e_fld_field])->fld_name->sym_string;
		l = ((qli_fld*) field->nod_arg[e_fld_field])->fld_name->sym_length;
	}

	USHORT length = l + 2;
	qli_str* str = NULL;
	TEXT* p = get_buffer(&str, NULL, length + 32);
	while (l--)
		*p++ = *q++;

	// Loop through the subscripts, adding to the label

	const TEXT* r;
	if (print_flag)
	{
		r = "_[";
		length++;
	}
	else
		r = "[";

	TEXT s[32];
	qli_nod** ptr = args->nod_arg;
	for (const qli_nod* const* const end = ptr + args->nod_count; ptr < end; ptr++)
	{
		qli_nod* subscript = *ptr;
		switch (subscript->nod_type)
		{
		case nod_constant:
			sprintf(s, "%"SLONGFORMAT, MOVQ_get_long(&subscript->nod_desc, 0));
			q = s;
			l = strlen(s);
			break;

		case nod_variable:
		case nod_field:
			q = ((qli_fld*) subscript->nod_arg[e_fld_field])->fld_name->sym_string;
			l = ((qli_fld*) subscript->nod_arg[e_fld_field])->fld_name->sym_length;
			break;

		default:
			// Punt on anything but constants, fields, and variables

			ALLQ_release((qli_frb*) str);
			return;
		}

		length += l + 1;
		p = get_buffer(&str, p, length);
		while (*r)
			*p++ = *r++;
		while (l--)
			*p++ = *q++;
		r = ",";
	}

	if (*r == ',')
		*p++ = ']';
	*p = 0;
	item->itm_query_header = str->str_data;
}


static TEXT* format_report( qli_vec* columns_vec, USHORT width, USHORT* max_width)
{
/**************************************
 *
 *	f o r m a t _ r e p o r t
 *
 **************************************
 *
 * Functional description
 *	Format a report.  Figure out spacing, print formats, and
 *	headers.  Return a pointer to the header string.
 *
 **************************************/
	qli_nod* node;
	USHORT lengths[10];
	const TEXT* segments[10];

	// Make a pass thru print items computing print lengths and header
	// lengths, and the number of header segments.

	USHORT number_segments, offset, max_offset;
	number_segments = offset = max_offset = 0;
	TEXT* bottom = BOTTOM_INIT;

	qli_lls** col = (qli_lls**) columns_vec->vec_object;
	for (const qli_lls* const* const col_end = col + columns_vec->vec_count;
		col < col_end && *col; col++)
	{
		USHORT column_width = 0, max_print_width = 0;
		bool right_adjust = false;
		qli_lls* temp;
		for (temp = *col; temp; temp = temp->lls_next)
		{
			qli_print_item* item = (qli_print_item*) temp->lls_object;
			switch (item->itm_type)
			{
			case item_column:
				offset = (item->itm_count) ? item->itm_count - 1 : 0;
				continue;

			case item_skip:
			case item_new_page:
			case item_report_header:
			case item_column_header:
				offset = 0;
				continue;

			case item_space:
				offset += item->itm_count;
				continue;

			case item_tab:
				offset = (offset & ~7) + item->itm_count * 8;
				continue;

			case item_value:
				max_print_width = MAX(max_print_width, item->itm_print_length);
				node = item->itm_value;
				if (node->nod_desc.dsc_dtype >= dtype_short &&
					node->nod_desc.dsc_dtype <= dtype_double)
					right_adjust = true;
			}

			if (item->itm_query_header)
			{
				const USHORT n = decompose_header(item->itm_query_header, segments, lengths);
				number_segments = MAX(n, number_segments);
				USHORT* ptr = lengths;
				for (USHORT j = 0; j < n; j++, ptr++)
					item->itm_header_length = MAX(item->itm_header_length, *ptr);
			}

			format_value(item, 0);
			const USHORT w = MAX(item->itm_print_length, item->itm_header_length);
			column_width = MAX(column_width, w);
		}

		if (offset + column_width > width)
			offset = 0;

		const USHORT right_offset = column_width - max_print_width / 2;

		for (temp = *col; temp; temp = temp->lls_next)
		{
			qli_print_item* item = (qli_print_item*) temp->lls_object;
			if (item->itm_type != item_value)
				continue;

			if (right_adjust)
				item->itm_print_offset = offset + right_offset - item->itm_print_length;
			else
				item->itm_print_offset = offset + (column_width - item->itm_print_length) / 2;

			item->itm_header_offset = offset + column_width / 2;

			// Before we blindly format the header, make sure there already isn't
			// header information in the same location

			if (item->itm_query_header)
			{
				BOTTOM_CHECK(bottom, offset);
				const TEXT* q = BOTTOM_LINE + offset;
				while (bottom < q)
					*bottom++ = ' ';
				bool flag = true;
				USHORT l;
				if (offset && q[-1] != ' ')
					flag = false;
				else if (l = MIN(column_width, bottom - q))
				{
					const TEXT* p = bottom;
					while (--l)
						if (*p++ != ' ')
						{
							flag = false;
							break;
						}
					if (flag && p < bottom && *p != ' ')
						flag = false;
				}
				if (flag)
				{
					BOTTOM_CHECK(bottom, offset + column_width);
					for (q = BOTTOM_LINE + offset + column_width; bottom < q;)
						*bottom++ = '=';
				}
				else
					item->itm_query_header = NULL;
			}
		}
		offset += column_width + 1;
		max_offset = MAX(max_offset, offset);
	}

	*max_width = max_offset;

	if (number_segments == 0)
		return NULL;

	// Allocate a string block big enough to hold all lines of the print header

	USHORT len = bottom - BOTTOM_LINE;
	qli_str* header = (qli_str*) ALLOCDV(type_str,
						(max_offset + 1) * (number_segments + 1) + 2 + len);
	TEXT* p = header->str_data;

	// Generate the various lines of the header line at a time.

	for (USHORT j = 0; j < number_segments; j++)
	{
		*p++ = '\n';
		TEXT* const line = p;
		col = (qli_lls**) columns_vec->vec_object;
		for (const qli_lls* const* const col_end = col + columns_vec->vec_count;
			col < col_end && *col; col++)
		{
			for (qli_lls* temp = *col; temp; temp = temp->lls_next)
			{
				qli_print_item* item = (qli_print_item*) temp->lls_object;
				if (item->itm_type != item_value)
					continue;
				const USHORT n = decompose_header(item->itm_query_header, segments, lengths);
				SSHORT segment = j - (number_segments - n);
				if (segment < 0)
					continue;
				USHORT l = lengths[segment];
				const TEXT* q = line + item->itm_header_offset - l / 2;
				while (p < q)
					*p++ = ' ';
				q = segments[segment];
				if (l)
					do {
						*p++ = *q++;
					} while (--l);
			}
		}
	}

	// Make one last pass to put in underlining of headers

	if (len = bottom - BOTTOM_LINE)
	{
		*p++ = '\n';
		bottom = BOTTOM_LINE;
		do {
			*p++ = *bottom++;
		} while (--len);
	}

	*p++ = '\n';
	*p++ = '\n';
	*p = 0;

	return header->str_data;
}


static void format_value( qli_print_item* item, int flags)
{
/**************************************
 *
 *	f o r m a t _ v a l u e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	const qli_nod* node = item->itm_value;
	const dsc* desc = &node->nod_desc;
	item->itm_dtype = desc->dsc_dtype;
	item->itm_sub_type = desc->dsc_sub_type;

	if (desc->dsc_dtype == dtype_blob)
	{
		item->itm_print_length = 40;
		if (node->nod_type == nod_reference)
			node = node->nod_arg[0];
		if (node->nod_type == nod_field)
		{
			const qli_fld* field = (qli_fld*) node->nod_arg[e_fld_field];
			if (field->fld_segment_length)
				item->itm_print_length = field->fld_segment_length;
		}
	}
	else
	{
		pics* picture = PIC_analyze(item->itm_edit_string, desc);
		item->itm_picture = picture;

		if (node->nod_type == nod_reference)
			node = node->nod_arg[0];

		const qli_fld* field;
		if (node->nod_type == nod_field)
		{
			field = (qli_fld*) node->nod_arg[e_fld_field];
			if ((field->fld_flags & FLD_array) && !node->nod_arg[e_fld_subs])
				ERRQ_print_error(480, field->fld_name->sym_string);
				// msg 480 can not format unsubscripted array %s
		}

		if (!(item->itm_picture->pic_missing) && (node->nod_type == nod_field) &&
			(field = (qli_fld*) node->nod_arg[e_fld_field]) && field->fld_missing)
		{
			PIC_missing(field->fld_missing, picture);
		}
		item->itm_print_length = picture->pic_length;
		picture->pic_flags |= flags;
	}
}


static TEXT* get_buffer(qli_str** str, TEXT* ptr, USHORT length)
{
/**************************************
 *
 *	g e t _ b u f f e r
 *
 **************************************
 *
 * Functional description
 *	Make sure we have a large enough buffer.
 *	If the current one is too short, copy the
 *	current buffer to the new one.
 *
 **************************************/
	if (!*str)
	{
		*str = (qli_str*) ALLOCPV(type_str, length);
		(*str)->str_length = length;
		return (*str)->str_data;
	}

	if (length <= (*str)->str_length)
		return ptr ? ptr : (*str)->str_data;

	qli_str* temp_str = (qli_str*) ALLOCPV(type_str, length);
	temp_str->str_length = length;
	TEXT* p = temp_str->str_data;
	const TEXT* q = (*str)->str_data;

	USHORT l;
	if (ptr && (l = ptr - q))
		do {
			*p++ = *q++;
		} while (--l);

	ALLQ_release((qli_frb*) *str);
	*str = temp_str;

	return p;
}


static bool match_expr(const qli_nod* node1, const qli_nod* node2)
{
/**************************************
 *
 *	m a t c h _ e x p r
 *
 **************************************
 *
 * Functional description
 *	Compare two nodes for equality of value.
 *
 **************************************/

	// If either is missing, they can't match.

	if (!node1 || !node2)
		return false;

	if (node1->nod_type == nod_reference)
		node1 = node1->nod_arg[0];

	if (node2->nod_type == nod_reference)
		node2 = node2->nod_arg[0];

	// A constant more or less matches anything

	if (node1->nod_type == nod_constant)
		return true;

	// Hasn't matched yet.  Check for statistical expression

	switch (node1->nod_type)
	{
	case nod_average:
	case nod_max:
	case nod_min:
	case nod_total:

	case nod_rpt_average:
	case nod_rpt_max:
	case nod_rpt_min:
	case nod_rpt_total:
	case nod_running_total:

	case nod_agg_average:
	case nod_agg_max:
	case nod_agg_min:
	case nod_agg_total:
		return match_expr(node1->nod_arg[e_stt_value], node2);
	}

	switch (node2->nod_type)
	{
	case nod_average:
	case nod_max:
	case nod_min:
	case nod_total:

	case nod_rpt_average:
	case nod_rpt_max:
	case nod_rpt_min:
	case nod_rpt_total:
	case nod_running_total:

	case nod_agg_average:
	case nod_agg_max:
	case nod_agg_min:
	case nod_agg_total:
		return match_expr(node1, node2->nod_arg[e_stt_value]);
	}

	if (node1->nod_type == node2->nod_type)
	{
		if (node1->nod_type == nod_field)
		{
			if (node1->nod_arg[e_fld_field] != node2->nod_arg[e_fld_field] ||
				node1->nod_arg[e_fld_context] != node2->nod_arg[e_fld_context])
			{
				return false;
			}
			return true;
		}
		const qli_nod* const* ptr1 = node1->nod_arg;
		const qli_nod* const* ptr2 = node2->nod_arg;
		for (const qli_nod* const* end = ptr1 + node1->nod_count; ptr1 < end; ++ptr1, ++ptr2)
		{
			if (!match_expr(*ptr1, *ptr2))
				return false;
		}
		return true;
	}

	return false;
}


static void print_blobs( qli_prt* print, qli_print_item** first, qli_print_item** last)
{
/**************************************
 *
 *	p r i n t _ b l o b s
 *
 **************************************
 *
 * Functional description
 *	Print any blobs still active in item list.
 *
 **************************************/
	if (QLI_abort)
		return;

	qli_print_item** ptr;

	USHORT length = 0;
	for (ptr = first; ptr < last; ptr++)
	{
		const qli_print_item* item = *ptr;
		if (item->itm_dtype == dtype_blob && item->itm_stream)
			length = MAX(length, item->itm_print_offset + item->itm_print_length + 2);
	}


	TEXT* buffer = get_buffer(&global_blob_buffer, NULL, length);

	while (!QLI_abort)
	{
		bool blob_active = false;
		TEXT* p = buffer;
		bool do_line = false;
		for (ptr = first; ptr < last; ptr++)
		{
			qli_print_item* item = *ptr;
			if (item->itm_dtype != dtype_blob || !item->itm_stream)
				continue;
			const TEXT* const end = buffer + item->itm_print_offset;
			while (p < end)
				*p++ = ' ';
			const TEXT* const pp = p;
			const int c = print_line(item, &p);
			if (c != EOF)
				blob_active = true;
			if (pp != p || c == '\n')
				do_line = true;
		}
		if (do_line)
			put_line(print, &p, buffer, '\n');
		if (!blob_active)
			break;
	}
}


static int print_line( qli_print_item* item, TEXT** ptr)
{
/**************************************
 *
 *	p r i n t _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Print a line of a blob or scratch file.  The
 *	last thing printed.
 *
 **************************************/
	EXEC_poll_abort();

	// If we're already at end of stream, there's nothing to do

	if (!item->itm_stream)
		return EOF;

	TEXT* p = *ptr;
	const USHORT l = item->itm_print_length;

	USHORT length;
	ISC_STATUS_ARRAY status_vector;
	const ISC_STATUS status = isc_get_segment(status_vector, &item->itm_stream, &length, l, p);
	if (status && status != isc_segment)
	{
		ISC_STATUS* null_status = 0;
		isc_close_blob(null_status, &item->itm_stream);
		if (status != isc_segstr_eof)
			ERRQ_database_error(0, status_vector);
		return EOF;
	}

	// If this is not a partial segment and the last character
	// is a newline, throw away the newline

	if (!status && length && p[length - 1] == '\n')
		if (length > 1)
			--length;
		else
			p[0] = ' ';

	*ptr = p + length;

	// Return the last character in the segment.
	// If the segment is null, return a newline.

	return length ? p[length - 1] : '\n';
}


static void put_line( qli_prt* print, TEXT** ptr, TEXT* buffer, TEXT terminator)
{
/**************************************
 *
 *	p u t _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Given a file descriptor, a running output pointer, and the address
 *	of the original buffer, write out the current line and reset the
 *	pointer.
 *
 **************************************/

	*(*ptr)++ = terminator;
	**ptr = 0;
	FMT_put(buffer, print);
	*ptr = buffer;
}


static void report_break( qli_brk* control, qli_vec** columns_vec, const bool bottom_flag)
{
/**************************************
 *
 *	r e p o r t _ b r e a k
 *
 **************************************
 *
 * Functional description
 *	Handle formatting for a chain of control breaks.
 *
 **************************************/

	if (!control)
		return;

	if (bottom_flag)
	{
		if (control->brk_next)
			report_break(control->brk_next, columns_vec, bottom_flag);
		if (control->brk_line)
			report_line((qli_nod*) control->brk_line, columns_vec);
		return;
	}

	for (; control; control = control->brk_next)
	{
		if (control->brk_line)
			report_line((qli_nod*) control->brk_line, columns_vec);
	}
}


static void report_item( qli_print_item* item, qli_vec** columns_vec, USHORT* col_ndx)
{
/**************************************
 *
 *	r e p o r t _ i t e m
 *
 **************************************
 *
 * Functional description
 *	Insert a report item into a logical column.  It it fits
 *	someplace reasonable, stick it there.
 *
 **************************************/
	if (item->itm_query_header && *item->itm_query_header == '-')
		item->itm_query_header = NULL;

	// If it's a constant, dump it in the next logical column

	qli_nod* node;
	qli_vec* columns = *columns_vec;
	if (columns->vec_object[*col_ndx] &&
		(node = item->itm_value) && node->nod_type == nod_constant)
	{
		ALLQ_push((blk*) item, (qli_lls**) (columns->vec_object + *col_ndx));
		return;
	}

	// Loop thru remaining logical columns looking for an equivalent
	// expression.  If we find one, the item beSLONGs in that column;
	// otherwise, someplace else.

	qli_lls** col = (qli_lls**) (columns->vec_object + *col_ndx);
	const qli_lls* const* const col_end = (qli_lls**) (columns->vec_object + columns->vec_count);
	for (; col < col_end && *col; col++)
		for (qli_lls* temp = *col; temp; temp = temp->lls_next)
		{
			qli_print_item* item2 = (qli_print_item*) temp->lls_object;
			if (match_expr(item->itm_value, item2->itm_value))
			{
				ALLQ_push((blk*) item, col);
				*col_ndx = col - (qli_lls**) columns->vec_object;
				return;
			}
		}

	// Didn't fit -- make a new logical column

	const USHORT new_index = col - (qli_lls**) columns->vec_object;
	*col_ndx = new_index;
	if (new_index >= columns->vec_count)
	{
		ALLQ_extend((BLK*) columns_vec, new_index + 16);
		(*columns_vec)->vec_count = new_index + 16;
	}

	ALLQ_push((blk*) item, (qli_lls**) ((*columns_vec)->vec_object + new_index));
}


static void report_line( qli_nod* list, qli_vec** columns_vec)
{
/**************************************
 *
 *	r e p o r t _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Process a report line.
 *
 **************************************/
	USHORT col_ndx = 0;
	qli_print_item** ptr = (qli_print_item**) list->nod_arg;
	for (const qli_print_item* const* const end = ptr + list->nod_count; ptr < end; ptr++)
	{
		qli_print_item* item = *ptr;
		report_item(item, columns_vec, &col_ndx);
		switch (item->itm_type)
		{
		case item_skip:
		case item_new_page:
			col_ndx = 0;
			break;
		}
	}
}


