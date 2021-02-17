/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		dtr.h
 *	DESCRIPTION:	General definitions, etc.
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

#ifndef QLI_DTR_H
#define QLI_DTR_H

#include "../common/dsc.h"
#include "../jrd/constants.h"

const USHORT QLI_MSG_FAC		= 1;

#define IBERROR(number)			ERRQ_error (number)

#define ALLOCD(type)			ALLQ_alloc (QLI_default_pool, type, 0)
#define ALLOCDV(type, repeat)	ALLQ_alloc (QLI_default_pool, type, repeat)
#define ALLOCP(type)			ALLQ_alloc (QLI_permanent_pool, type, 0)
#define ALLOCPV(type, repeat)	ALLQ_alloc (QLI_permanent_pool, type, repeat)

#define BLKDEF(type, root, tail) type,
enum blk_t
{
	type_MIN = 0,
#include "../qli/blk.h"
	type_MAX
};
#undef BLKDEF

// Block types

#ifndef INCLUDE_FB_BLK
#include "../include/old_fb_blk.h"
#endif

enum nod_t {

	// Commands, not executed.
	nod_nothing = 0,
	nod_ready = 1,
	nod_finish,
	nod_commit,
	nod_rollback,
	nod_prepare,
	nod_start_trans,
	nod_show,
	nod_exit,
	nod_quit,
	//nod_create,
	nod_define,
	nod_delete_proc,
	nod_rename_proc,
	nod_edit_proc,
	nod_copy_proc,
	nod_extract,
	nod_set,
	nod_help,
	nod_shell,
	nod_def_database,
	nod_del_database,
	nod_def_relation,
	nod_mod_relation,
	nod_del_relation,
	nod_def_field,
	nod_mod_field,
	nod_del_field,
	nod_def_index,
	nod_del_index,
	nod_mod_index,

	// SQL statements, mapped into GDML statements

	//nod_select,
	//nod_insert,
	//nod_delete,
	//nod_fetch,
	//nod_close,
	//nod_open,

	nod_for,
	nod_restructure,
	nod_print,
	nod_list_fields,
	nod_list,
	nod_store,
	nod_modify,
	nod_erase,
	nod_assign,
	nod_report,
	nod_report_loop,
	nod_repeat,
	nod_if,
	nod_output,
	nod_declare,
	nod_variable,
	nod_abort,
	//nod_label,
	//nod_leave,
	//nod_sync,
	nod_commit_retaining,

	nod_relation,
	//nod_name,
	nod_rse,
	//nod_union,
	//nod_aggregate,

	nod_print_item,
	nod_star,			// Bunch of items
	nod_column,
	nod_tab,
	nod_skip,
	nod_space,
	nod_new_page,
	nod_report_header,
	nod_column_header,

	nod_eql,
	nod_neq,
	nod_gtr,
	nod_geq,
	nod_leq,
	nod_lss,
	nod_between,
	nod_containing,
	nod_matches,
	nod_like,
	nod_starts,
	nod_missing,
	nod_and,
	nod_or,
	nod_any,
	nod_not,
	nod_unique,

	nod_reference,
	nod_field,
	nod_prompt,
	//nod_prompt2,
	nod_edit_blob,
	nod_parameter, // only tested, but not set
	nod_constant,
	nod_position,
	nod_map,

	nod_add,
	nod_subtract,
	nod_multiply,
	nod_divide,
	nod_negate,
	nod_concatenate,
	nod_substr,
	nod_null,
	nod_user_name,
	nod_format,
	nod_choice, // only tested but not set
	nod_function,

	nod_average,
	nod_max,
	nod_min,
	nod_total,
	nod_count,

	nod_agg_average,
	nod_agg_max,
	nod_agg_min,
	nod_agg_total,
	nod_agg_count,

	nod_rpt_average,
	nod_rpt_max,
	nod_rpt_min,
	nod_rpt_total,
	nod_rpt_count,

	nod_running_count,
	nod_running_total,
	nod_from,
	nod_via, // only tested but not set

	nod_upcase,				// New blr for v3
	nod_lowcase,
	nod_sleuth,
	nod_index,

	nod_join_inner,
	nod_join_left,
	nod_join_right,
	nod_join_full, // only set but not tested

	//nod_sql_create,			// SQL metadata
	nod_sql_database,
	//nod_sql_dr_index,
	nod_sql_al_table,
	nod_sql_cr_table,
	//nod_sql_dr_table,
	nod_sql_cr_view,
	//nod_sql_dr_view,
	nod_sql_grant,
	nod_sql_revoke

};

struct qli_vec
{
	blk		vec_header;
	ULONG	vec_count;
	blk*	vec_object[1];
};

struct qli_vcl
{
	blk		vcl_header;
	ULONG	vcl_count;
	SLONG	vcl_long[1];
};

// Constant block

struct qli_const
{
	blk		con_header;
	dsc		con_desc;
	UCHAR	con_data[1];
};

// Symbol types

enum sym_t
{
	SYM_keyword,
	SYM_context, // set but never tested
	SYM_database,
	SYM_relation,
	SYM_field, // set but never tested
	//SYM_stream,
	//SYM_cursor,
	//SYM_form,
	SYM_function
};

struct qli_symbol
{
	blk			sym_header;
	const TEXT*	sym_string;		// address of asciz string
	USHORT		sym_length;		// length of string (exc. term.)
	sym_t		sym_type;		// symbol type
	USHORT		sym_keyword;	// keyword number, if keyword
	blk*		sym_object;		// general pointer to object
	qli_symbol*	sym_collision;	// collision pointer
	qli_symbol*	sym_homonym;	// homonym pointer
	TEXT		sym_name[2];	// space for name, if necessary
};

// Syntax nodes (moved from compile.h because of cross-references)

struct qli_syntax
{
	blk		syn_header;
	nod_t	syn_type;		// Type of node
	USHORT	syn_flags;
	USHORT	syn_count;		// Number of arguments
	qli_syntax*	syn_arg[1];
};

// Database block

struct qli_dbb
{
	blk				dbb_header;
	qli_dbb*		dbb_next;			// Next database in system
	struct qli_rel*	dbb_relations;		// Linked list of relations
	struct qli_fun*	dbb_functions;		// Known functions in database
	FB_API_HANDLE	dbb_handle;			// database handle
	FB_API_HANDLE	dbb_transaction;	// transaction handle
	FB_API_HANDLE	dbb_proc_trans;		// procedure transaction
	FB_API_HANDLE	dbb_meta_trans;		// metadata update transaction
	FB_API_HANDLE	dbb_field_request;	// "get fields" request handle
	qli_symbol*		dbb_symbol;			// Database variable
	qli_const*		dbb_user;			// user name
	qli_const*		dbb_password;		// password
	USHORT			dbb_filename_length;	// Length of filename
	FB_API_HANDLE	dbb_lookup_blob;	// Request to look up blob
	FB_API_HANDLE	dbb_store_blob;		// Request to store blob
	FB_API_HANDLE	dbb_edit_blob;
	FB_API_HANDLE	dbb_edit_store;
	FB_API_HANDLE	dbb_scan_blobs;
	//FB_API_HANDLE	dbb_delete_blob;	// Unused
	USHORT			dbb_flags;
	//USHORT		dbb_type;			// Friend or foe?
	USHORT			dbb_pagesize;		// For SQL metadata
	ULONG			dbb_capabilities;	// Special characteristics to look out for
	int*			dbb_statistics;		// Statistics memory
	FB_API_HANDLE	dbb_requests [96];	// Misc meta-data requests
	TEXT			dbb_filename [2];	// Filename of database
};

// Bits in dbb_flags

const USHORT DBB_procedures	= 1;		// Procedures relation found
const USHORT DBB_active		= 2;		// Database is active in request
const USHORT DBB_updates	= 8;		// Updates in current transaction
const USHORT DBB_prepared	= 32;		// User transaction has been prepared

// Bits in dbb_capabilities

const ULONG DBB_cap_extern_file		= 1;	// supports the concept of external files
const ULONG DBB_cap_files			= 2;	// uses the Firebird concept of multi-file
const ULONG DBB_cap_security		= 4;	// Firebird style security classes
// const ULONG DBB_cap_triggers		= 8;	-- OBSOLETE: triggers (old style)
const ULONG DBB_cap_idx_inactive	= 16;	// Firebird index inactive flag
const ULONG DBB_cap_multi_trans		= 32;	// Database supports multiple transactions
const ULONG DBB_cap_single_trans	= 64;	// Database supports only a single transaction
//const ULONG DBB_cap_dudley		= 128;	// We're authorized for meta-data updates. Unused
const ULONG DBB_cap_functions		= 256;	// The DBMS knows about function
const ULONG DBB_cap_new_triggers	= 512;	// triggers (new V3 style)
const ULONG DBB_cap_shadowing		= 1024;	// Database supports shadowing
const ULONG DBB_cap_types			= 2048;	// Database has RDB$TYPES relation
const ULONG DBB_cap_dimensions		= 4096;	// Database supports arrays -- What a database!
//const ULONG DBB_cap_external_type	= 8192;	// Database supports external datatypes. Unused
const ULONG DBB_cap_rfr_sys_flag	= 16384;	// Database hasn't forgotten about the system flag
const ULONG DBB_cap_filters			= 32768;	// Database supports filters
const ULONG DBB_cap_index_type		= 65536;	// Database has descending indices
const ULONG DBB_cap_nodyn			= 131072;	// Database (really, engine) does not support DYN
// Relation block

struct qli_rel
{
	blk				rel_header;
	qli_rel* 		rel_next;			// Next relation in database
	qli_dbb*		rel_database;		// Parent database
	qli_symbol*		rel_symbol;			// Hash symbol for relation
	struct qli_fld*	rel_fields;			// Field block
	USHORT			rel_id;				// Relation id
	USHORT			rel_flags;			// Misc flags
	USHORT			rel_system_flag;	// System flag
	USHORT			rel_max_field_pos;	// highest used field position
};

// rel_flags
const USHORT REL_fields	= 1;	// Fields have been fetched
const USHORT REL_system	= 2;	// Relation is system relation
const USHORT REL_view	= 4;	// Relation is a view

// Field block

struct qli_fld
{
	blk			fld_header;
	qli_fld*	fld_next;			// Next field in relation
	qli_rel*	fld_relation;		// Parent relation
	qli_symbol*	fld_name;			// Field name
	qli_symbol*	fld_query_name;		// Field query name
	qli_symbol*	fld_based;			// Name of global field, if we care
	const TEXT*	fld_edit_string;	// Edit string, if any
	const TEXT*	fld_query_header;	// Query header, if any
	USHORT		fld_flags;
	USHORT		fld_id;				// Field in in database
	USHORT		fld_dtype;			// Data type of field
	FLD_LENGTH	fld_length;			// Length of field
	USHORT		fld_segment_length;	// Segment length for blobs
	SSHORT		fld_scale;			// Scale factor of field
	qli_syntax*		fld_validation;		// Validation expression
	//qli_syntax*	fld_computed;		// Computed by expression. Unused
	qli_const*	fld_missing;		// Missing value defined in system relation
	SSHORT		fld_sub_type;		// Subtype for blobs, text and EXACT_NUMERICs.
	SSHORT		fld_sub_type_missing;	// Subtype missing flag
	SSHORT      fld_precision;      // Precision for NUMERIC and DECIMAL.
	USHORT		fld_system_flag;	// System flag
	double		fld_dummy;			// Force fld_data to a nice boundary word boundary
	UCHAR		fld_data [1];
};

// fld_flags
const USHORT FLD_computed	= 1;
const USHORT FLD_drop		= 2;
const USHORT FLD_modify		= 4;
const USHORT FLD_missing	= 8;
const USHORT FLD_not_null	= 16;
const USHORT FLD_add		= 32;
//const USHORT FLD_unique	= 64;	// Unused
const USHORT FLD_array		= 128;

// Privileges for SQL metadata

const USHORT PRV_select		= 1;
const USHORT PRV_insert		= 2;
const USHORT PRV_delete		= 4;
const USHORT PRV_update		= 8;
const USHORT PRV_all		= PRV_select + PRV_insert + PRV_delete + PRV_update;
const USHORT PRV_grant_option	= 16;

struct qli_lls
{
	blk	lls_header;
	blk* lls_object;
	qli_lls* lls_next;
};

// Random string block -- jack of all kludges

struct qli_str
{
	blk		str_header;
	USHORT	str_length;
	USHORT	str_fluff; // ??? Not used, maybe alignment?
	TEXT	str_data[2];
};

// Free block

struct qli_frb
{
	blk			frb_header;
	qli_frb*	frb_next;	// Next free block in pool
};

// Hunk blocks

struct qli_hnk
{
	blk			hnk_header;
	void*		hnk_address;	// start of memory hunk
	int			hnk_length;		// length of memory hunk
	qli_hnk*	hnk_next;		// next memory hunk in structure
};

// Pool block

struct qli_plb
{
	blk			plb_header;
	USHORT		plb_pool_id;	// pool id
	qli_frb*	plb_free;		// first free block
	qli_hnk*	plb_hunks;		// first hunk block
	qli_lls*	plb_lls;		// avaiable linked list stack nodes
};

// Equivalence label dsc_missing to field dsc_flags in the dsc structure

#define dsc_missing	dsc_flags

// State flags for dsc_missing

const USHORT DSC_missing	= 1;
const USHORT DSC_initial	= 2; // Nobody sets this value, at least directly

// Function description

struct qli_fun
{
	blk			fun_header;
	qli_fun*	fun_next;		// Next function in database
	qli_dbb*	fun_database;
	qli_symbol*	fun_symbol;		// Associated symbol block
	qli_symbol*	fun_query_name;
	dsc			fun_return;		// Return descriptor
	//USHORT	fun_args;		// Number of arguments. Unused
	//dsc			fun_arg[1];		// Data type of arguments
								// If you change this change blk.h too
};

// Program wide globals
//#include <setjmp.h>

#ifdef QLI_MAIN
#define EXTERN
#else
#define EXTERN	extern
#endif

EXTERN qli_dbb*	QLI_databases;
EXTERN qli_plb*	QLI_permanent_pool;
EXTERN qli_plb*	QLI_default_pool;
EXTERN qli_fld*	QLI_variables;
EXTERN TEXT	QLI_prompt_string [32];
EXTERN TEXT	QLI_cont_string [32];
EXTERN TEXT	QLI_default_user [MAX_SQL_IDENTIFIER_SIZE];
EXTERN TEXT	QLI_default_password [MAX_SQL_IDENTIFIER_SIZE];
EXTERN TEXT	QLI_charset [MAX_SQL_IDENTIFIER_SIZE];
EXTERN qli_const*	QLI_matching_language;

//EXTERN USHORT	QLI_eof;
EXTERN bool		QLI_blr;
EXTERN bool		QLI_skip_line;
EXTERN bool		QLI_statistics;
EXTERN bool		QLI_semi;
EXTERN bool		QLI_abort;
EXTERN bool		QLI_echo;
EXTERN bool		QLI_trace;
EXTERN USHORT	QLI_count;
#ifdef DEV_BUILD
EXTERN bool		QLI_explain;
EXTERN bool		QLI_hex_output;
#endif
#ifdef TRUSTED_AUTH
EXTERN bool		QLI_trusted;
#endif
EXTERN bool		QLI_nodb_triggers;

EXTERN const char* QLI_error;
EXTERN bool sw_verify;
EXTERN bool sw_trace;
EXTERN USHORT sw_buffers;
EXTERN USHORT QLI_lines;
EXTERN USHORT QLI_prompt_count;
EXTERN bool QLI_reprompt;
EXTERN USHORT QLI_name_columns;
EXTERN USHORT QLI_columns;

//EXTERN USHORT   QLI_euc_justify;
//EXTERN USHORT   QLI_interp;

#include "parse.h"

// from parse.h
EXTERN qli_tok*		QLI_token;
EXTERN qli_line*	QLI_line;
EXTERN TEXT*		QLI_prompt;

#include "exe.h"

// from exe.h
EXTERN qli_req*	QLI_requests;			// Requests in statement


#undef EXTERN

#include "../qli/all_proto.h"

#ifdef DEV_BUILD
void QLI_validate_desc(const dsc*);
#else
inline void QLI_validate_desc(const dsc*) { }
#endif
inline void QLI_validate_desc(const dsc& d) { QLI_validate_desc(&d); }

#endif // QLI_DTR_H

