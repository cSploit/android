/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		compile.h
 *	DESCRIPTION:	Parser/compiler definitions
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

#ifndef QLI_COMPILE_H
#define QLI_COMPILE_H

// Name nodes -- used to hold names between parse and compilation

struct qli_name
{
    blk			nam_header;
    qli_symbol*	nam_symbol;		// Symbol pointer, if any
    USHORT		nam_length;		// Length of name
    TEXT		nam_string[1];	// Actual name string
};

// Qualified procedure node -- used to hold qualified procedure names

struct qli_proc
{
    blk  qpr_header;
    qli_dbb* qpr_database;		// database block
    const qli_name* qpr_name;			// name block
};

// Qualified function node -- used to hold qualified function names

struct qli_func
{
    blk  qfn_header;
    qli_dbb* qfn_database;		// database block
    const qli_name* qfn_name;			// name block
};

// Qualified filter node -- used to hold qualified filter names

struct qli_filter
{
    blk  qfl_header;
    qli_dbb* qfl_database;		// database block
    const qli_name* qfl_name;			// name block
};

// SHOW options

enum show_t {
    show_all,
    show_relation,
    show_relations,
    show_system_relations,
    show_database,
    show_databases,
    show_procedure,
    show_procedures,
    show_version,
    show_variable,
    show_variables,
    show_indices,
    //show_qualified_procedure,
    show_field,
    show_filter,
    show_filters,
    show_global_field,
    show_global_fields,
    show_trigger,
    show_triggers,
    show_security_class,
    show_security_classes,
    //show_forms,
    show_function,
    show_functions,
    show_views,
    show_system_triggers,
    show_matching_language,
    show_db_indices,
    show_db_fields
};

enum set_t {
    set_blr,
    set_statistics,
    //set_epilog,	// Unused
    set_columns,
    set_lines,
    set_semi,
    set_echo,
    //set_form,
    set_matching_language,
    set_prompt,
    set_continuation,
    set_user,
    set_password,
    //set_euc_justify,			// JPN specific option
    set_count,
    set_charset
#ifdef DEV_BUILD
    , set_hex_output,
    set_explain
#endif
};

// Position assignments for syntax tree nodes

const int s_rel_relation	= 0;	// Actual relation block
const int s_rel_context		= 1;	// Symbol block for context, if any
const int s_rel_count		= 2;

const int s_prt_list		= 0;	// Print list
const int s_prt_rse			= 1;	// Rse, if any
const int s_prt_output		= 2;	// Output file, if any
const int s_prt_distinct	= 3;	// Simple flag indicating distinct
const int s_prt_order		= 4;	// SQL order clause
const int s_prt_form		= 5;
const int s_prt_count		= 6;

const int s_rse_first		= 0;	// FIRST clause, if any
const int s_rse_boolean		= 1;	// Boolean clause, if any
const int s_rse_sort		= 2;	// Sort clause, if any
const int s_rse_reduced		= 3;	// Reduced clause, if any
const int s_rse_group_by	= 4;
const int s_rse_having		= 5;
const int s_rse_outer		= 6;	// Outer context value (implicit ANY)
const int s_rse_inner		= 7;	// Inner context value
const int s_rse_op			= 8;	// Comparison operator
const int s_rse_join_type	= 9;	// Join type
const int s_rse_all_flag	= 10;	// Explicit "ALL" flag
const int s_rse_list		= 11;
const int s_rse_count		= 12;

const int s_for_rse			= 0;
const int s_for_statement	= 1;
const int s_for_count		= 2;

const int s_itm_value		= 0;	// Value of print item
const int s_itm_edit_string	= 1;	// Edit string, if any
const int s_itm_header		= 2;	// Query header, if any
const int s_itm_count		= 3;

const int s_sto_relation	= 0;
const int s_sto_statement	= 1;
const int s_sto_rse			= 2;
const int s_sto_fields		= 3;
const int s_sto_values		= 4;
const int s_sto_form		= 5;
const int s_sto_count		= 6;

const int s_asn_from		= 0;
const int s_asn_to			= 1;
const int s_asn_count		= 2;

const int s_mod_list		= 0;	// Field list
const int s_mod_statement	= 1;	// Sub-statement
const int s_mod_rse			= 2;	// Record selection expression
const int s_mod_form		= 3;
const int s_mod_count		= 4;

const int s_era_rse			= 0;
const int s_era_count		= 1;

const int s_rpt_value		= 0;
const int s_rpt_statement	= 1;
const int s_rpt_count		= 2;

const int s_if_boolean		= 0;
const int s_if_true			= 1;
const int s_if_false		= 2;
const int s_if_count		= 3;

const int s_stt_rse			= 0;
const int s_stt_value		= 1;
const int s_stt_default		= 2;
const int s_stt_distinct	= 3;
const int s_stt_count		= 4;

const int s_out_file		= 0;
const int s_out_pipe		= 1;
const int s_out_count		= 2;

const int s_fmt_value		= 0;
const int s_fmt_edit		= 1;
const int s_fmt_count		= 2;

const int s_dfi_name		= 0;
const int s_dfi_relation	= 1;	// Define index
const int s_dfi_fields		= 2;
const int s_dfi_count		= 3;

const int s_mfi_name		= 0;	// Modify an index
const int s_mfi_database	= 1;
const int s_mfi_count		= 2;

const int s_ffr_form		= 0;
const int s_ffr_statement	= 1;
const int s_ffr_count		= 2;

const int s_frm_form		= 0;
const int s_frm_context		= 1;
const int s_frm_database	= 2;
const int s_frm_count		= 3;

const int s_men_string		= 0;
const int s_men_labels		= 1;
const int s_men_statements	= 2;
const int s_men_count		= 3;

const int s_fun_args		= 0;
const int s_fun_function	= 1;
const int s_fun_count		= 2;

const int s_idx_field		= 0;	// array subscript
const int s_idx_subs		= 1;
const int s_idx_count		= 2;

const int s_crv_name		= 0;
const int s_crv_rse			= 1;
const int s_crv_count		= 2;

const int s_grant_fields	= 0;
const int s_grant_relation	= 1;
const int s_grant_users		= 2;
const int s_grant_privileges	= 3;
const int s_grant_count		= 4;

//  some flags for index definitions

const USHORT s_dfi_flag_unique		= 1;
const USHORT s_dfi_flag_inactive	= 2;
const USHORT s_dfi_flag_activity	= 4;
const USHORT s_dfi_flag_selectivity	= 8;
const USHORT s_dfi_flag_descending	= 16;
const USHORT s_dfi_flag_order		= 32;
const USHORT s_dfi_flag_statistics	= 64;

#endif // QLI_COMPILE_H

