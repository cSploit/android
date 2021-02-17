/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		report.h
 *	DESCRIPTION:	Report writer definitions
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

#ifndef QLI_REPORT_H
#define QLI_REPORT_H

// Control break block

struct qli_brk
{
    blk			brk_header;
    qli_brk*	brk_next;			// Next break
    qli_syntax*		brk_field;		// Field expression for break
    qli_syntax*		brk_line;		// Print line
    qli_lls*	brk_statisticals;	// Statistical expressions
};

// Report block

struct qli_rpt
{
    blk			rpt_hdr;
    qli_nod*	rpt_detail_line;	// Detail line print list
    qli_brk*	rpt_top_page;		// Top of page print list
    qli_brk*	rpt_bottom_page;	// Bottom of page print list
    qli_brk*	rpt_top_rpt;		// Top of report print list
    qli_brk*	rpt_bottom_rpt;		// Bottom of report print list
    qli_brk*	rpt_top_breaks;		// Top of <field> break list
    qli_brk*	rpt_bottom_breaks;	// Bottom of <field> break list
    const TEXT*	rpt_column_header;
    const TEXT*	rpt_name;			// Parsed report name
    const TEXT*	rpt_header;			// Expanded report header
    UCHAR*		rpt_buffer;			// Data buffer
    USHORT		rpt_columns;		// Columns per page
    USHORT		rpt_lines;			// Lines per page
};

#endif // QLI_REPORT_H

