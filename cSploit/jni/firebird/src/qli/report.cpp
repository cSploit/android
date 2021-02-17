/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		report.cpp
 *	DESCRIPTION:	Report writer runtime control
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
#include "../qli/dtr.h"
#include "../qli/exe.h"
#include "../qli/compile.h"
#include "../qli/report.h"
#include "../qli/all_proto.h"
#include "../qli/eval_proto.h"
#include "../qli/exe_proto.h"
#include "../qli/forma_proto.h"
#include "../qli/mov_proto.h"
#include "../qli/repor_proto.h"

static void bottom_break(qli_brk*, qli_prt*);
static void increment_break(qli_brk*);
static void initialize_break(qli_brk*);
static bool test_break(qli_brk*, qli_rpt*, qli_msg*);
static void top_break(qli_brk*, qli_prt*);
static void top_of_page(qli_prt*, bool);

//#define SWAP(a, b)	{temp = a; a = b; b = temp;}
inline void swap_uchar(UCHAR*& a, UCHAR*& b)
{
	UCHAR* temp = a;
	a = b;
	b = temp;
}


void RPT_report( qli_nod* loop)
{
/**************************************
 *
 *	R P T _ r e p o r t
 *
 **************************************
 *
 * Functional description
 *	Execute a FOR loop.  This may require that a request get
 *	started, a message sent, and a message received for each
 *	record.  At the other end of the spectrum, there may be
 *	absolutely nothing to do.
 *
 **************************************/

	// Get to actual report node

	qli_nod* node = loop->nod_arg[e_for_statement];
	qli_rpt* report = (qli_rpt*) node->nod_arg[e_prt_list];
	qli_prt* print = (qli_prt*) node->nod_arg[e_prt_output];
	print->prt_new_page = top_of_page;
	print->prt_page_number = 0;

	// Get to actual report node

	// If there is a request associated  with the loop, start it and possibly
	// send a message slong with it.

	qli_msg* message;
	qli_req* request = (qli_req*) loop->nod_arg[e_for_request];
	if (request)
		EXEC_start_request(request, (qli_msg*) loop->nod_arg[e_for_send]);
	else if (message = (qli_msg*) loop->nod_arg[e_for_send])
		EXEC_send(message);

	// Receive messages in a loop until the end of file field comes up true.

	message = (qli_msg*) loop->nod_arg[e_for_receive];

	// Get the first record of the record.  If there isn't anything,
	// don't worry about anything.

	const dsc* desc = EXEC_receive(message, (qli_par*) loop->nod_arg[e_for_eof]);
	if (*(USHORT*) desc->dsc_address)
		return;

	if (!report->rpt_buffer)
	{
		qli_str* string = (qli_str*) ALLOCDV(type_str, message->msg_length);
		report->rpt_buffer = (UCHAR*) string->str_data;
	}

	memcpy(report->rpt_buffer, message->msg_buffer, (SLONG) message->msg_length);

	qli_brk* control;
	if (control = report->rpt_top_rpt)
		FMT_print((qli_nod*) control->brk_line, print);

	top_of_page(print, true);

	initialize_break(report->rpt_bottom_breaks);
	initialize_break(report->rpt_bottom_page);
	initialize_break(report->rpt_bottom_rpt);

	// Force TOP breaks for all fields

	for (control = report->rpt_top_breaks; control; control = control->brk_next)
		FMT_print((qli_nod*) control->brk_line, print);

	for (;;)
	{
		// Check for bottom breaks.  If we find one, force all lower breaks.

		for (control = report->rpt_bottom_breaks; control; control = control->brk_next)
		{
			if (test_break(control, report, message))
			{
				swap_uchar(message->msg_buffer, report->rpt_buffer);
				bottom_break(control, print);
				swap_uchar(message->msg_buffer, report->rpt_buffer);
				initialize_break(control);
				break;
			}
		}

		if (print->prt_lines_remaining <= 0)
			top_of_page(print, false);

		// Now check for top breaks.

		for (control = report->rpt_top_breaks; control; control = control->brk_next)
		{
			if (test_break(control, report, message))
			{
				top_break(control, print);
				break;
			}
		}

		// Increment statisticals and print detail line, if any

		increment_break(report->rpt_bottom_breaks);
		increment_break(report->rpt_bottom_page);
		increment_break(report->rpt_bottom_rpt);

		if (node = report->rpt_detail_line)
			FMT_print(node, print);

		// Get the next record.  If we're at end, we're almost done.

		swap_uchar(message->msg_buffer, report->rpt_buffer);
		desc = EXEC_receive(message, (qli_par*) loop->nod_arg[e_for_eof]);
		if (*(USHORT *) desc->dsc_address)
			break;
	}

	// Force BOTTOM breaks for all fields

	swap_uchar(message->msg_buffer, report->rpt_buffer);
	bottom_break(report->rpt_bottom_breaks, print);
	bottom_break(report->rpt_bottom_rpt, print);

	if (control = report->rpt_bottom_page)
		FMT_print((qli_nod*) control->brk_line, print);
}


static void bottom_break( qli_brk* control, qli_prt* print)
{
/**************************************
 *
 *	b o t t o m  _ b r e a k
 *
 **************************************
 *
 * Functional description
 *	Force all lower breaks then take break.
 *
 **************************************/
	if (!control)
		return;

	if (control->brk_next)
		bottom_break(control->brk_next, print);

	for (qli_lls* stack = control->brk_statisticals; stack; stack = stack->lls_next)
		EVAL_break_compute((qli_nod*) stack->lls_object);

	FMT_print((qli_nod*) control->brk_line, print);
}


static void increment_break( qli_brk* control)
{
/**************************************
 *
 *	i n c r e m e n t _ b r e a k
 *
 **************************************
 *
 * Functional description
 *	Toss another record into running computations.
 *
 **************************************/
	for (; control; control = control->brk_next)
	{
		for (qli_lls* stack = control->brk_statisticals; stack; stack = stack->lls_next)
			EVAL_break_increment((qli_nod*) stack->lls_object);
	}
}


static void initialize_break( qli_brk* control)
{
/**************************************
 *
 *	i n i t i a l i z e _ b r e a k
 *
 **************************************
 *
 * Functional description
 *	Execute a control break.
 *
 **************************************/
	for (; control; control = control->brk_next)
	{
		for (qli_lls* stack = control->brk_statisticals; stack; stack = stack->lls_next)
			EVAL_break_init((qli_nod*) stack->lls_object);
	}
}


static bool test_break(qli_brk* control, qli_rpt* report, qli_msg* message)
{
/**************************************
 *
 *	t e s t _ b r e a k
 *
 **************************************
 *
 * Functional description
 *	Check to see if there has been a control break for an expression.
 *
 **************************************/
	DSC desc1, desc2;

	// Evaluate the two versions of the expression

	dsc* ptr1 = EVAL_value((qli_nod*) control->brk_field);
	if (ptr1)
		desc1 = *ptr1;

	UCHAR* const buf = message->msg_buffer;
	message->msg_buffer = report->rpt_buffer;

	dsc* ptr2 = EVAL_value((qli_nod*) control->brk_field);
	if (ptr2)
		desc2 = *ptr2;

	// An error in EVAL_value will prevent msg_buffer from being restored to its old value.
	message->msg_buffer = buf;

	// Check for consistently missing

	if (!ptr1 || !ptr2)
		return (ptr1 != ptr2);

	// Both fields are present.  Check values.  Luckily, there's no need
	// to worry about datatypes.

	const UCHAR* p1 = desc1.dsc_address;
	const UCHAR* p2 = desc2.dsc_address;
	USHORT l = desc1.dsc_length;

	if (desc1.dsc_dtype == dtype_varying)
		l = 2 + *(USHORT*) p1;

	if (l)
		return memcmp(p1, p2, l) != 0;

	return false;
}


static void top_break( qli_brk* control, qli_prt* print)
{
/**************************************
 *
 *	 t o p _ b r e a k
 *
 **************************************
 *
 * Functional description
 *	Execute a control break.
 *
 **************************************/
	for (; control; control = control->brk_next)
	{
		for (qli_lls* stack = control->brk_statisticals; stack; stack = stack->lls_next)
		{
			EVAL_break_compute((qli_nod*) stack->lls_object);
		}
		FMT_print((qli_nod*) control->brk_line, print);
	}
}


static void top_of_page(qli_prt* print, bool first_flag)
{
/**************************************
 *
 *	t o p _ o f _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Handle top of page condition.
 *
 **************************************/
	qli_brk* control;

	++print->prt_page_number;
	qli_rpt* report = print->prt_report;

	if (!first_flag)
	{
		if (control = report->rpt_bottom_page)
			FMT_print((qli_nod*) control->brk_line, print);
		FMT_put("\f", print);
	}

	print->prt_lines_remaining = print->prt_lines_per_page;

	if (control = report->rpt_top_page)
		FMT_print((qli_nod*) control->brk_line, print);
	else if (report->rpt_column_header)
	{
		if (report->rpt_header)
			FMT_put(report->rpt_header, print);
		if (report->rpt_column_header)
			FMT_put(report->rpt_column_header, print);
	}

	if (report->rpt_bottom_page)
		initialize_break(report->rpt_bottom_page);
}
