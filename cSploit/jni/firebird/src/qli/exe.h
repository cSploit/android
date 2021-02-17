/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		exe.h
 *	DESCRIPTION:	Execution struct definitions
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

#ifndef QLI_EXE_H
#define QLI_EXE_H

// Request Language Block  -- used for BLR, DYN, SDL, etc.

struct qli_rlb
{
    blk		rlb_header;
    UCHAR	*rlb_data;		// Pointer to end of BLR/DYN/SDL
    UCHAR	*rlb_base;		// Pointer to start of buffer
    UCHAR	*rlb_limit;		// Upper limit of string
    USHORT	rlb_length;		// Length of string
};

// RLB manipulation macros
const USHORT RLB_BUFFER_SIZE	= 2048;
const USHORT RLB_SAFETY_MARGIN	= 48;

#define STUFF(blr)	*rlb->rlb_data++ = blr
#define STUFF_WORD(blr)	{STUFF (blr); STUFF (blr >> 8);}

struct qli_msg; // forward decl.


// Request block

struct qli_req
{
    blk				req_header;
    qli_req*		req_next;		// Next request in statement
    qli_dbb*		req_database;	// Database for request
    FB_API_HANDLE	req_handle;		// Database request handle
    qli_rlb*		req_blr;
    qli_msg*		req_messages;	// Messages associated with request
    qli_msg*		req_receive;	// Current receive message, if any
    qli_msg*		req_send;		// Current send message, if any
    qli_msg*		req_continue;	// Message to continue FOR loop after optional actions
    USHORT			req_flags;		// Flags for state of request compilation, etc.
    USHORT			req_context;	// Next available context
    USHORT			req_msg_number;	// Next available message number
    USHORT			req_label;		// Next available label
};

// req_flags
const USHORT REQ_rse_compiled	= 1;
const USHORT REQ_project		= 2;	// Set during generation of project clause
const USHORT REQ_group_by		= 4;	// Set during generation of group by clause


struct qli_nod;

// Context node

enum CTX_T {
    CTX_RELATION,
    CTX_VARIABLE,
    CTX_AGGREGATE,
    //CTX_UNION,
    CTX_STREAM
};

struct qli_ctx
{
    blk				ctx_header;
    CTX_T			ctx_type;		// Type of context
    qli_ctx*		ctx_source;		// Source context for MODIFY
    qli_ctx*		ctx_primary;	// Primary context
    qli_symbol*		ctx_symbol;		// Context symbol, if any
    struct qli_rel*	ctx_relation;	// Relation of context
    qli_nod*		ctx_stream;		// Stream of context
    struct qli_fld*	ctx_variable;	// Variable reference
    qli_req*		ctx_request;	// Request block
    qli_msg*		ctx_message;	// Message for data
    qli_nod*		ctx_rse;		// RSE node for root context
    qli_nod*		ctx_sub_rse;	// RSE node aggregate
    qli_ctx*		ctx_parent;		// Parent context for map
    struct qli_map*	ctx_map;		// Map items, if any
    USHORT			ctx_context;	// Context in request
};

// Aggregate/union map block

struct qli_map
{
    blk			map_header;
	qli_map*	map_next;			// Next map in item
    qli_nod*	map_node;			// Value for map item
    USHORT		map_position;		// Position in map
};

// Message block

struct qli_msg
{
    blk			msg_header;
    qli_req*	msg_request;		// Parent request
    //qli_ctx*	msg_context;		// Contexts in message
    qli_msg*	msg_next;			// Next message in request
    struct qli_par*	msg_parameters;	// Field instances
    USHORT		msg_number;			// Message number
    USHORT		msg_length;			// Message length
    USHORT		msg_parameter;		// Next parameter number
    UCHAR*		msg_buffer;			// Message buffer
};

// Parameter block

struct qli_par
{
	blk			par_header;
	dsc			par_desc;			// Value descriptor
	qli_par*	par_next;			// Next par block in context
	qli_msg*	par_message;		// Parent message
	qli_nod*	par_value;			// Value
	USHORT		par_parameter;		// Parameter number
	USHORT		par_offset;			// Offset of parameter in message
	qli_par*	par_missing;		// Parameter block for missing value
};

// Print item block

enum itm_t
{
    item_value,
    item_skip,
    item_column,
    item_tab,
    item_space,
    item_new_page,
    item_column_header,
    item_report_header
};

struct qli_print_item
{
    blk			itm_header;
    qli_nod*	itm_value;
    const TEXT*	itm_edit_string;
    struct pics*	itm_picture;	// picture string block
    const TEXT*	itm_query_header;
    itm_t		itm_type;
    USHORT		itm_flags;			// Misc flags and crud
    UCHAR		itm_dtype;
    UCHAR		itm_sub_type;
    USHORT		itm_print_offset;
    USHORT		itm_print_length;
    USHORT		itm_header_offset;
    USHORT		itm_header_length;
    //USHORT	itm_header_segments;
    USHORT		itm_count;			// Number of lines to skip
    //USHORT	itm_column;			// Logical column number
    FB_API_HANDLE itm_stream;
    //USHORT		itm_kanji_fragment;	// JPN: last kanji on line border
    //ISC_STATUS	itm_blob_status;	// JPN: status of the last blob fetch
};

// itm_flags
const USHORT ITM_overlapped	= 1;			// Overlapped by another item

// Print Control Block

struct qli_prt
{
    blk		prt_header;
    FILE*	prt_file;		// FILE pointer
    struct qli_rpt*	prt_report;		// Report block (if report)
    void	(*prt_new_page)(qli_prt*, bool);	// New page routine, if any
    USHORT	prt_lines_per_page;
    SSHORT	prt_lines_remaining;
    USHORT	prt_page_number;
};

// General node blocks

struct qli_nod
{
    blk			nod_header;
    nod_t		nod_type;		// Type of node
    dsc			nod_desc;		// Descriptor
    qli_par*	nod_import;		// To pass random value
    qli_par*	nod_export;		// To pass random value
    SSHORT		nod_count;		// Number of arguments
    UCHAR		nod_flags;
    qli_nod* 	nod_arg[1];		// If you change this change blk.h too
};

// nod_flags
const UCHAR NOD_local		= 1;	// locally computed expression
const UCHAR NOD_remote		= 2;
const UCHAR NOD_parameter2	= 4;	// generate a parameter2 if field
const UCHAR nod_partial		= 8;
const UCHAR nod_comparison	= 16;
const UCHAR nod_date		= 32;	// node is a date operation, regardless

// Execution node positions

const int e_fld_field		= 0;	// field block
const int e_fld_context		= 1;	// context for field
const int e_fld_reference	= 2;	// points to parameter
const int e_fld_subs		= 3;	// subscripts
const int e_fld_count		= 4;

const int e_for_request		= 0;	// Request to be started
const int e_for_send		= 1;	// Message to be sent
const int e_for_receive		= 2;	// Message to be received
const int e_for_eof			= 3;	// End of file parameter
const int e_for_rse			= 4;
const int e_for_statement	= 5;
const int e_for_count		= 6;

const int e_itm_value		= 0;	// Value of print item
const int e_itm_edit_string = 1;	// Edit string, if any
const int e_itm_header		= 2;	// Query header, if any
const int e_itm_count		= 3;

const int e_rse_first		= 0;	// FIRST clause, if any
const int e_rse_boolean		= 1;	// Boolean clause, if any
const int e_rse_sort		= 2;	// Sort clause, if any
const int e_rse_reduced		= 3;	// Reduced clause, if any
const int e_rse_context		= 4;	// Context block
const int e_rse_group_by	= 5;
const int e_rse_having		= 6;
const int e_rse_join_type	= 7;
const int e_rse_count		= 8;

const int e_prt_list		= 0;	// List of print items
const int e_prt_file_name	= 1;	// Output file name
const int e_prt_output		= 2;	// Output file
const int e_prt_header		= 3;	// Header to be printed, if any
const int e_prt_count		= 4;

const int e_prm_prompt		= 0;	// Prompt string, if any
const int e_prm_string		= 1;	// String node for data
const int e_prm_next		= 2;	// Next prompt in statement
const int e_prm_field		= 3;	// Prototype field, if known
const int e_prm_count		= 4;

const int e_sto_context		= 0;
const int e_sto_statement	= 1;
const int e_sto_request		= 2;
const int e_sto_send		= 3;
const int e_sto_count		= 4;

const int e_asn_to			= 0;
const int e_asn_from		= 1;
const int e_asn_initial		= 2;
const int e_asn_valid		= 3;	// Always second-to-last
const int e_asn_count		= 4;

const int e_mod_send		= 0;
const int e_mod_statement	= 1;	// Sub-statement
const int e_mod_request		= 2;	// Parent request for statement
const int e_mod_count		= 3;

const int e_era_context		= 0;
const int e_era_request		= 1;	// Parent request for erase
const int e_era_message		= 2;	// Message to be sent, if any
const int e_era_count		= 3;

const int e_any_request		= 0;	// Request to be started
const int e_any_send		= 1;	// Message to be sent
const int e_any_receive		= 2;	// Message to be received
const int e_any_rse			= 3;
const int e_any_count		= 4;

const int e_rpt_value		= 0;
const int e_rpt_statement	= 1;
const int e_rpt_count		= 2;

const int e_if_boolean		= 0;
const int e_if_true			= 1;
const int e_if_false		= 2;
const int e_if_count		= 3;

const int e_edt_input		= 0;
const int e_edt_dbb			= 1;
const int e_edt_id1			= 2;
const int e_edt_id2			= 3;
const int e_edt_name		= 4;
const int e_edt_count		= 5;

const int e_out_statement	= 0;
const int e_out_file		= 1;
const int e_out_pipe		= 2;
const int e_out_print		= 3;
const int e_out_count		= 4;

const int e_fmt_value		= 0;
const int e_fmt_edit		= 1;
const int e_fmt_picture		= 2;
const int e_fmt_count		= 3;

// Statistical expression

const int e_stt_rse			= 0;
const int e_stt_value		= 1;
const int e_stt_default		= 2;
const int e_stt_request		= 3;	// Request to be started
const int e_stt_send		= 4;	// Message to be sent
const int e_stt_receive		= 5;	// Message to be received
const int e_stt_count		= 6;

const int e_map_context		= 0;
const int e_map_map			= 1;
const int e_map_count		= 2;

const int e_ffr_form		= 0;
const int e_ffr_statement	= 1;
const int e_ffr_count		= 2;

const int e_fup_form		= 0;
const int e_fup_fields		= 1;
const int e_fup_tag			= 2;
const int e_fup_count		= 3;

const int e_ffl_form		= 0;
const int e_ffl_field		= 1;
const int e_ffl_string		= 2;
const int e_ffl_count		= 3;

const int e_men_statements	= 0;
const int e_men_labels		= 1;
const int e_men_string		= 2;
const int e_men_menu		= 4;
const int e_men_count		= 5;

const int e_fun_args		= 0;
const int e_fun_function	= 1;
const int e_fun_request		= 2;	// Request to be started
const int e_fun_send		= 3;	// Message to be sent
const int e_fun_receive		= 4;	// Message to be received
const int e_fun_count		= 5;

const int e_syn_statement	= 0;
const int e_syn_send		= 1;
const int e_syn_count		= 2;

#endif // QLI_EXE_H

