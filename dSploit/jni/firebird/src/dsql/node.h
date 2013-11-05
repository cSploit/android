/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		node.h
 *	DESCRIPTION:	Definitions needed for accessing a parse tree
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
 * 2001.6.12 Claudio Valderrama: add break_* constants.
 * 2001.6.30 Claudio valderrama: Jim Starkey suggested to hold information
 * about source line in each node that's created.
 * 2001.07.28 John Bellardo: Added e_rse_limit to nod_rse and nod_limit.
 * 2001.08.03 John Bellardo: Reordered args to no_sel for new LIMIT syntax
 * 2002.07.30 Arno Brinkman:
 * 2002.07.30	Added nod_searched_case, nod_simple_case, nod_coalesce
 * 2002.07.30	and constants for arguments
 * 2002.08.04 Dmitry Yemanov: ALTER VIEW
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2004.01.16 Vlad Horsun: added support for default parameters and
 *   EXECUTE BLOCK statement
 * Adriano dos Santos Fernandes
 */

#ifndef DSQL_NODE_H
#define DSQL_NODE_H

#include "../dsql/dsql.h"
#include "../common/classes/Aligner.h"

// an enumeration of the possible node types in a syntax tree

namespace Dsql {

enum nod_t
{
	nod_unknown_type = 0,
	nod_commit = 1,	// Commands, not executed.
	nod_rollback,
	nod_trans,
	nod_def_default,
	nod_del_default,
	nod_def_database,
	nod_def_domain,
	nod_mod_domain,
	nod_del_domain,
	nod_mod_database, // 10
	nod_def_relation,
	nod_mod_relation,
	nod_del_relation,
	nod_def_field,
	nod_mod_field,
	nod_del_field,
	nod_def_index,
	nod_del_index,
	nod_def_view,
	nod_def_constraint, // 20
	nod_def_trigger,
	nod_mod_trigger,
	nod_del_trigger,
//	nod_def_trigger_msg,
//	nod_mod_trigger_msg,
//	nod_del_trigger_msg,
	nod_def_procedure,
	nod_mod_procedure,
	nod_del_procedure,
	nod_def_exception,
	nod_mod_exception,
	nod_del_exception,
	nod_def_generator, //30
	nod_del_generator,
	nod_def_filter,
	nod_del_filter,
	nod_def_shadow,
	nod_del_shadow,
	nod_def_udf,
	nod_del_udf,
	nod_grant,
	nod_revoke,
	nod_rel_constraint, // 40
	nod_delete_rel_constraint,
	nod_primary,
	nod_foreign,
	nod_abort,
	nod_references,
	nod_proc_obj,
	nod_trig_obj,
	nod_view_obj,
	nod_list,	// SQL statements, mapped into GDML statements
	nod_select, // 50
	nod_insert,
	nod_delete,
	nod_update,
	nod_close,
	nod_open,
	nod_all,	// ALL privileges
	nod_execute,	// EXECUTE privilege
	nod_store,
	nod_modify,
	nod_erase, // 60
	nod_assign,
	nod_exec_procedure,
	nod_return,	// Procedure statements
	nod_exit,
	nod_while,
	nod_if,
	nod_for_select,
	nod_erase_current,
	nod_modify_current,
	nod_post, // 70
	nod_block,
	nod_on_error,
	nod_sqlcode,
	nod_gdscode,
	nod_exception,
	nod_exception_stmt,
	nod_default,
	nod_start_savepoint,
	nod_end_savepoint,
	nod_cursor,	//80	// used to create record streams
	nod_relation,
	nod_relation_name,
	nod_procedure_name,
	nod_rel_proc_name,
	nod_rse,
	nod_select_expr,
	nod_union,
	nod_aggregate,
	nod_order,
	nod_flag, // 90
	nod_join,
/* NOTE: when adding an expression node, be sure to
   test various combinations; in particular, think
   about parameterization using a '?' - there is code
   in PASS1_node which must be updated to find the
   data type of the parameter based on the arguments
   to an expression node */
	nod_eql,
	nod_neq,
	nod_gtr,
	nod_geq,
	nod_leq,
	nod_lss,
	nod_between,
	nod_like,
	nod_missing, // 100
	nod_and,
	nod_or,
	nod_any,
	nod_not,
	nod_unique,
	nod_containing,
	nod_starting,
	nod_via,
	nod_field,	// values
	nod_dom_value, // 110
	nod_field_name,
	nod_parameter,
	nod_constant,
	nod_map,
	nod_alias,
	nod_user_name,
	nod_user_group,
	nod_variable,
	nod_var_name,
	nod_array, // 120
	nod_add,	// functions
	nod_subtract,
	nod_multiply,
	nod_divide,
	nod_negate,
	nod_concatenate,
	nod_substr,
	nod_null,
	nod_dbkey,
	nod_udf, // 130
	nod_cast,
	nod_upcase,
	nod_lowcase,
	nod_collate,
	nod_gen_id,
	nod_add2,	// functions different for dialect >= V6_TRANSITION
	nod_subtract2,
	nod_multiply2,
	nod_divide2,
	nod_gen_id2, // 140
	nod_average,	// aggregates
	nod_from,
	nod_max,
	nod_min,
	nod_total,
	//nod_count,    // obsolete
	nod_exists,
	nod_singular,
	nod_agg_average,
	nod_agg_max,
	nod_agg_min, // 150
	nod_agg_total,
	nod_agg_count,
	nod_agg_average2,
	nod_agg_total2,
	nod_get_segment,	// blobs
	nod_put_segment,
	nod_join_inner,	// join types
	nod_join_left,
	nod_join_right,
	nod_join_full, // 160
	nod_join_cross,
	// sql transaction support
	nod_access,
	nod_wait,
	nod_isolation,
	nod_version,
	nod_table_lock,
	nod_lock_mode,
	nod_reserve,
	nod_retain,
	// sql database stmts support
	nod_page_size, // 170
	nod_file_length,
	nod_file_desc,
	nod_dfl_charset,
	// sql connect options support (used for create database)
	nod_password,
	nod_lc_ctype,	// SET NAMES
	// Misc nodes
	nod_udf_return_value,
	// computed field
	nod_def_computed,
	// access plan stuff
	nod_plan_expr,
	nod_plan_item,
	nod_merge_plan, // 180
	nod_natural,
	nod_index,
	nod_index_order,
	nod_set_generator,
	nod_set_generator2,	// SINT64 value for dialect > V6_TRANSITION
	// alter index
	nod_mod_index,
	nod_idx_active,
	nod_idx_inactive,
		// drop behaviour
	nod_restrict,
	nod_cascade, // 190
	// set statistics
	nod_set_statistics,
	// record version
	nod_rec_version,
	// ANY keyword used
	nod_ansi_any,
	nod_eql_any,
	nod_neq_any,
	nod_gtr_any,
	nod_geq_any,
	nod_leq_any,
	nod_lss_any,
	// ALL keyword used
	nod_ansi_all, // 200
	nod_eql_all,
	nod_neq_all,
	nod_gtr_all,
	nod_geq_all,
	nod_leq_all,
	nod_lss_all,
	//referential integrity actions
	nod_ref_upd_del,
	nod_ref_trig_action,
	// SQL role support
	nod_def_role,
	nod_role_name, // 210
	nod_grant_admin,
	nod_del_role,
	// SQL time & date support
	nod_current_time,
	nod_current_date,
	nod_current_timestamp,
	nod_extract,
	// ALTER column/domain support
	nod_mod_domain_type,
	nod_mod_field_name,
	nod_mod_field_type,
	nod_mod_field_pos, // 220

	// CVC: SQL requires that DROP VIEW and DROP table are independent.
	nod_del_view,
	nod_current_role, // nod_role_name is already taken but only for DDL.
	nod_breakleave,
	nod_redef_relation, // allows silent creation/overwriting of a relation.
	nod_udf_param, // there should be a way to signal a param by descriptor!
	nod_limit, // limit support
	nod_redef_procedure, // allows silent creation/overwriting of a procedure.
	nod_exec_sql, // EXECUTE STATEMENT
	nod_internal_info, // internal engine info
	nod_searched_case, // 230	// searched CASE function
	nod_simple_case, // simple CASE function
	nod_coalesce, // COALESCE function
	nod_mod_view, // ALTER VIEW
	nod_replace_procedure, // CREATE OR ALTER PROCEDURE
	nod_replace_trigger, // CREATE OR ALTER TRIGGER
	nod_replace_view, // CREATE OR ALTER VIEW
	nod_redef_view, // allows silent creation/overwriting of a view
	nod_for_update, // FOR UPDATE clause
	nod_user_savepoint, // savepoints support
	nod_release_savepoint, // 240
	nod_undo_savepoint,
	nod_label, // label support
	// CVC: This node seems obsolete.
	nod_exec_into, // EXECUTE STATEMENT INTO
	nod_difference_file,
	nod_drop_difference,
	nod_begin_backup,
	nod_end_backup,
	nod_derived_table, // Derived table support
	nod_derived_field,  // Derived table support
	nod_cursor_open, // 250
	nod_cursor_fetch,
	nod_cursor_close,
	nod_fetch_seek,
	nod_exec_block,		// EXECUTE BLOCK support
	nod_param_val,		// default value for SP parameters support
	nod_rows,	// ROWS support
	nod_query_spec,
	nod_equiv,  // IS DISTINCT FROM
	nod_redef_exception, // RECREATE EXCEPTION
	nod_replace_exception, // 260	// CREATE OR ALTER EXCEPTION
	nod_comment,
	nod_mod_udf,
	nod_def_collation,
	nod_del_collation,
	nod_collation_from,
	nod_collation_from_external,
	nod_collation_attr,
	nod_collation_specific_attr,
	nod_strlen,
	nod_trim, // 270
	nod_returning,
	nod_redef_trigger,
	nod_tra_misc,
	nod_lock_timeout,
	nod_agg_list,
	nod_src_info,
	nod_with,
	nod_update_or_insert,
	nod_merge,
	nod_merge_when, // 280
	nod_merge_update,
	nod_merge_insert,
	nod_sys_function,
	nod_similar,
	nod_mod_role,
	nod_add_user,
	nod_mod_user,
	nod_del_user,
	nod_exec_stmt,
	nod_exec_stmt_inputs,	// 290
	nod_exec_stmt_datasrc,
	nod_exec_stmt_user,
	nod_exec_stmt_pwd,
	nod_exec_stmt_role,
	nod_exec_stmt_privs,
	nod_tran_params,
	nod_named_param,
	nod_dfl_collate,
	nod_class_node,
	nod_hidden_var	// 300
};

/* enumerations of the arguments to a node, offsets
   within the variable tail nod_arg */

/* Note Bene:
 *	e_<nodename>_count	== count of arguments in nod_arg
 *	This is often used as the count of sub-nodes, but there
 *	are cases when non-DSQL_NOD arguments are stuffed into nod_arg
 *	entries.  These include nod_udf, nod_gen_id, nod_cast,
 *	& nod_collate.
 */
enum node_args {
	e_select_expr = 0,		// nod_select
	e_select_update,
	e_select_lock,
	e_select_count,

	e_fpd_list = 0,			// nod_for_update
	e_fpd_count,

	e_sav_name = 0,			// nod_user_savepoint, nod_undo_savepoint
	e_sav_count,

	e_fld_context = 0,		// nod_field
	e_fld_field,
	e_fld_indices,
	e_fld_count,

	e_ary_array = 0,		// nod_array
	e_ary_indices,
	e_ary_count,

	e_xcp_name = 0,			// nod_exception
	e_xcp_text,
	e_xcp_count,

	e_blk_action = 0,		// nod_block
	e_blk_errs,
	e_blk_count,

	e_err_errs = 0,			// nod_on_error
	e_err_action,
	e_err_count,

	e_var_variable = 0,		// nod_variable
	e_var_count,

	e_pst_event = 0,		// nod_post
	e_pst_argument,
	e_pst_count,

	e_exec_sql_stmnt = 0,	// nod_exec_sql
	e_exec_sql_count,

	e_exec_into_stmnt = 0,	// nod_exec_into
	e_exec_into_block,
	e_exec_into_list,
	e_exec_into_label,
	e_exec_into_count,

	e_exec_stmt_sql = 0,	// nod_exec_stmt
	e_exec_stmt_inputs,
	e_exec_stmt_outputs,
	e_exec_stmt_proc_block,
	e_exec_stmt_label,
	e_exec_stmt_options,
	e_exec_stmt_data_src,
	e_exec_stmt_user,
	e_exec_stmt_pwd,
	e_exec_stmt_role,
	e_exec_stmt_tran,
	e_exec_stmt_privs,
	e_exec_stmt_count,

	e_exec_stmt_inputs_sql = 0,	// nod_exec_stmt_inputs
	e_exec_stmt_inputs_params,
	e_exec_stmt_inputs_count,

	e_named_param_name = 0,	// nod_named_param
	e_named_param_expr,
	e_named_param_count,

	e_internal_info = 0,	// nod_internal_info
	e_internal_info_count,

	e_xcps_name = 0,		// nod_exception_stmt
	e_xcps_msg,
	e_xcps_count,

	e_rtn_procedure = 0,	// nod_procedure
	e_rtn_count,

	e_vrn_name = 0,			// nod_variable_name
	e_vrn_count,

	e_fln_context = 0,		// nod_field_name
	e_fln_name,
	e_fln_count,

	e_rel_context = 0,		// nod_relation
	e_rel_count,

	e_agg_context = 0,		// nod_aggregate
	e_agg_group,
	e_agg_rse,
	e_agg_count,

	e_rse_streams = 0,		// nod_rse
	e_rse_boolean,
	e_rse_sort,
	e_rse_reduced,
	e_rse_items,
	e_rse_first,
	e_rse_plan,
	e_rse_skip,
	e_rse_lock,
	e_rse_count,

	e_limit_skip = 0,		// nod_limit
	e_limit_length,
	e_limit_count,

	e_rows_skip = 0,		// nod_rows
	e_rows_length,
	e_rows_count,

	e_par_index = 0,		// nod_parameter
	e_par_parameter,
	e_par_count,

	e_flp_select = 0,		// nod_for_select
	e_flp_into,
	e_flp_cursor,
	e_flp_action,
	e_flp_label,
	e_flp_count,

	e_cur_name = 0,			// nod_cursor
	e_cur_rse,
	e_cur_number,
	e_cur_count,

	e_prc_name = 0,			// nod_procedure
	e_prc_inputs,
	e_prc_outputs,
	e_prc_dcls,
	e_prc_body,
	e_prc_source,
	e_prc_count,

	e_exe_procedure = 0,	// nod_exec_procedure
	e_exe_inputs,
	e_exe_outputs,
	e_exe_count,

	e_exe_blk = 0,			// nod_exec_block
	e_exe_blk_inputs = 0,
	e_exe_blk_outputs,
	e_exe_blk_dcls,
	e_exe_blk_body,
	e_exe_blk_count,

	e_prm_val_fld = 0,
	e_prm_val_val,
	e_prm_val_count,

	e_sel_query_spec = 0,	// nod_select_expr
	e_sel_order,
	e_sel_rows,
	e_sel_with_list,
	e_sel_count,

	e_qry_limit = 0,		// nod_query_spec
	e_qry_distinct,
	e_qry_list,
	e_qry_from,
	e_qry_where,
	e_qry_group,
	e_qry_having,
	e_qry_plan,
	e_qry_count,

	e_ins_relation = 0,		// nod_insert
	e_ins_fields,
	e_ins_values,
	e_ins_select,
	e_ins_return,
	e_ins_count,

	e_mrg_relation = 0,		// nod_merge
	e_mrg_using,
	e_mrg_condition,
	e_mrg_when,
	e_mrg_count,

	e_mrg_when_matched = 0,	// nod_merge_when
	e_mrg_when_not_matched,
	e_mrg_when_count,

	e_mrg_update_statement = 0,	// nod_merge_update
	e_mrg_update_count,

	e_mrg_insert_fields = 0,	// nod_merge_insert
	e_mrg_insert_values,
	e_mrg_insert_count,

	e_sto_relation = 0,		// nod_store
	e_sto_statement,
	e_sto_rse,
	e_sto_return,
	e_sto_count,

	e_upi_relation = 0,		// nod_update_or_insert
	e_upi_fields,
	e_upi_values,
	e_upi_matching,
	e_upi_return,
	e_upi_count,

	e_del_relation = 0,		// nod_delete
	e_del_boolean,
	e_del_plan,
	e_del_sort,
	e_del_rows,
	e_del_cursor,
	e_del_return,
	e_del_count,

	e_era_relation = 0,		// nod_erase
	e_era_rse,
	e_era_return,
	e_era_count,

	e_asgn_value = 0,       // nod_assign
	e_asgn_field,
	e_asgn_count,

	e_erc_context = 0,		// nod_erase_current
	e_erc_return,
	e_erc_count,

	e_mod_source = 0,		// nod_modify
	e_mod_update,
	e_mod_statement,
	e_mod_rse,
	e_mod_return,
	e_mod_count,

	e_mdc_context = 0,		// nod_modify_current
	e_mdc_update,
	e_mdc_statement,
	e_mdc_return,
	e_mdc_count,

	e_upd_relation = 0,		// nod_update
	e_upd_statement,
	e_upd_boolean,
	e_upd_plan,
	e_upd_sort,
	e_upd_rows,
	e_upd_cursor,
	e_upd_return,
	e_upd_rse_flags,
	e_upd_count,

	e_map_context = 0,		// nod_map
	e_map_map,
	e_map_count,

	e_blb_field = 0,		// nod_get_segment & nod_put_segment
	e_blb_relation,
	e_blb_filter,
	e_blb_max_seg,
	e_blb_count,

	e_idx_unique = 0,		// nod_def_index
	e_idx_asc_dsc,
	e_idx_name,
	e_idx_table,
	e_idx_fields,
	e_idx_count,

	e_rln_name = 0,			// nod_relation_name
	e_rln_alias,
	e_rln_count,

	e_rpn_name = 0,			// nod_rel_proc_name
	e_rpn_alias,
	e_rpn_inputs,
	e_rpn_count,

	e_join_left_rel = 0,	// nod_join
	e_join_type,
	e_join_rght_rel,
	e_join_boolean,
	e_join_count,

	e_via_rse = 0, 			// nod_via
	e_via_value_1,
	e_via_value_2,
	e_via_count,

	e_if_condition = 0,		// nod_if
	e_if_true,
	e_if_false,
	e_if_count,

	e_while_cond = 0,		// nod_while
	e_while_action,
	e_while_label,
	e_while_count,

	e_drl_name = 0,			// nod_def_relation
	e_drl_elements,
	e_drl_ext_file,			// external file
	e_drl_count,

	e_dft_default = 0,		// nod_def_default
	e_dft_default_source,
	e_dft_count,

	e_dom_name = 0,			// nod_def_domain
	e_dom_default,
	e_dom_constraint,
	e_dom_collate,
	e_dom_count,

	e_dfl_field = 0,		// nod_def_field
	e_dfl_default,
	e_dfl_constraint,
	e_dfl_collate,
	e_dfl_domain,
	e_dfl_computed,
	e_dfl_count,

	e_view_name = 0,		// nod_def_view
	e_view_fields,
	e_view_select,
	e_view_check,
	e_view_source,
	e_view_count,

	e_alt_name = 0,			// nod_mod_relation
	e_alt_ops,
	e_alt_count,

	e_grant_privs = 0,		// nod_grant
	e_grant_table,
	e_grant_users,
	e_grant_grant,
	e_grant_grantor,
	e_grant_count,

	e_alias_value = 0,		// nod_alias
	e_alias_alias,
	e_alias_imp_join,
	e_alias_count,

	e_rct_name = 0,			// nod_rel_constraint
	e_rct_type,

	e_pri_columns = 0,		// nod_primary
	e_pri_index,
	e_pri_count,

	e_for_columns = 0,		// nod_foreign
	e_for_reftable,
	e_for_refcolumns,
	e_for_action,
	e_for_index,
	e_for_count,

	e_ref_upd = 0,			// nod_ref_upd_del_action
	e_ref_del,
	e_ref_upd_del_count,

	e_ref_trig_action_count = 0,	// nod_ref_trig_action

	e_cnstr_table = 0,		// nod_def_constraint
	e_cnstr_type,
	e_cnstr_condition,
	e_cnstr_actions,
	e_cnstr_source,
	e_cnstr_count,

	e_trg_name = 0,			// nod_mod_trigger and nod_def_trigger
	e_trg_table,
	e_trg_active,
	e_trg_type,
	e_trg_position,
	e_trg_actions,
	e_trg_source,
	e_trg_count,

	e_trg_act_dcls = 0,
	e_trg_act_body,
	e_trg_act_count,

	e_abrt_number = 0,		// nod_abort
	e_abrt_count,

	e_cast_target = 0,		// Not a DSQL_NOD   nod_cast
	e_cast_source,
	e_cast_count,

	e_coll_target = 0,		// Not a DSQL_NOD   nod_collate
	e_coll_source,
	e_coll_count,

	e_order_field = 0,		// nod_order
	e_order_flag,
	e_order_nulls,
	e_order_count,

	e_lock_tables = 0,		//
	e_lock_mode,

	e_database_name = 0,	//
	e_database_initial_desc,
	e_database_rem_desc,
	e_cdb_count,

	e_commit_retain = 0,	//
	e_commit_count,

	e_rollback_retain = 0,	//
	e_rollback_count,

	e_adb_all = 0,			//
	e_adb_count,

	e_gen_name = 0,			//
	e_gen_count,

	e_filter_name = 0,		//
	e_filter_in_type,
	e_filter_out_type,
	e_filter_entry_pt,
	e_filter_module,
	e_filter_count,

	e_gen_id_name = 0,		// Not a DSQL_NOD   nod_gen_id
	e_gen_id_value,
	e_gen_id_count,


	e_udf_name = 0,			//
	e_udf_entry_pt,
	e_udf_module,
	e_udf_args,
	e_udf_return_value,
	e_udf_count,

	// computed field

	e_cmp_expr = 0,
	e_cmp_text,

	// create shadow

	e_shadow_number = 0,
	e_shadow_man_auto,
	e_shadow_conditional,
	e_shadow_name,
	e_shadow_length,
	e_shadow_sec_files,
	e_shadow_count,

	// alter index

	e_alt_index = 0,
	e_mod_idx_count,

	e_alt_idx_name = 0,
	e_alt_idx_name_count,

	// set statistics

	e_stat_name = 0,
	e_stat_count,

	// SQL extract() function

	e_extract_part = 0,				// constant representing part to extract
	e_extract_value,				// Must be a time or date value
	e_extract_count,

	// SQL CURRENT_TIME, CURRENT_DATE, CURRENT_TIMESTAMP

	e_current_time_count = 0,
	e_current_date_count = 0,
	e_current_timestamp_count = 0,

	e_alt_dom_name = 0,				// nod_mod_domain
	e_alt_dom_ops,
	e_alt_dom_count,

	e_mod_dom_new_dom_type = 0, 	// nod_mod_domain_type
	e_mod_dom_count,

	e_mod_fld_name_orig_name = 0,	// nod_mod_field_name
	e_mod_fld_name_new_name,
	e_mod_fld_name_count,

	e_mod_fld_type_field = 0,				// nod_mod_field_type
	e_mod_fld_type_dom_name,
	e_mod_fld_type_default,
	e_mod_fld_type_computed,
	e_mod_fld_type_count,

	e_mod_fld_pos_orig_name = 0,	// nod_mod_field_position
	e_mod_fld_pos_new_position,
	e_mod_fld_pos_count,

	// CVC: blr_leave used to emulate break
	e_breakleave_label = 0,			// nod_breakleave
	e_breakleave_count,

	// SQL substring() function

	e_substr_value = 0,	// Anything that can be treated as a string
	e_substr_start,		// Where the slice starts
	e_substr_length,	// The length of the slice
	e_substr_count,

	e_trim_specification = 0,
	e_trim_characters,
	e_trim_value,
	e_trim_count,

	e_udf_param_field = 0,
	e_udf_param_type,		// Basically, by_reference or by_descriptor
	e_udf_param_count,

	// CASE <case_operand> {WHEN <when_operand> THEN <when_result>}.. [ELSE <else_result>] END
	// Node-constants for after pass1

	e_simple_case_case_operand = 0,	// 1 value
	e_simple_case_when_operands,	// list
	e_simple_case_results,			// list including else_result
	e_simple_case_case_operand2,	// operand for use after the first test

	// CASE {WHEN <search_condition> THEN <when_result>}.. [ELSE <else_result>] END
	// Node-constants for after pass1

	e_searched_case_search_conditions = 0,	// list boolean expressions
	e_searched_case_results,				// list including else_result

	e_label_name = 0,
	e_label_number,
	e_label_count,

	e_derived_table_rse = 0,		// Contains select_expr
	e_derived_table_alias,			// Alias name for derived table
	e_derived_table_column_alias,	// List with alias names from derived table columns
	e_derived_table_context,		// Context for derived table
	e_derived_table_count,

	e_derived_field_value = 0,		// Contains the source expression
	e_derived_field_name,			// Name for derived table field
	e_derived_field_scope,			// Scope-level
	e_derived_field_context,		// context of derived table
	e_derived_field_count,

	e_cur_stmt_id = 0,
	e_cur_stmt_seek,
	e_cur_stmt_into,
	e_cur_stmt_count,

	e_agg_function_expression = 0,
	e_agg_function_delimiter,
	e_agg_function_scope_level,
	e_agg_function_count,

	e_comment_obj_type = 0,
	e_comment_object,
	e_comment_part,
	e_comment_string,
	e_comment_count,

	e_mod_udf_name = 0,				// nod_mod_udf
	e_mod_udf_entry_pt,
	e_mod_udf_module,
	e_mod_udf_count,

	e_def_coll_name = 0,
	e_def_coll_for,
	e_def_coll_from,
	e_def_coll_attributes,
	e_def_coll_specific_attributes,
	e_def_coll_count,

	e_del_coll_name = 0,
	e_del_coll_count,

	e_strlen_type = 0,				// constant representing type of length
	e_strlen_value,
	e_strlen_count,

	e_ret_source = 0,				// nod_returning
	e_ret_target,
	e_ret_count,

	e_sysfunc_name = 0,				// nod_sys_function
	e_sysfunc_args,
	e_sysfunc_count,

	e_similar_value = 0,
	e_similar_pattern,
	e_similar_escape,
	e_similar_count,

	e_src_info_line = 0,			// nod_src_info
	e_src_info_column,
	e_src_info_stmt,
	e_src_info_count,

	e_mod_role_os_name = 0,			// nod_mod_role
	e_mod_role_db_name,
	e_mod_role_action,				// 0 - drop, 1 - add
	e_mod_role_count,

	e_del_user_name = 0,			// nod_del_user
	e_del_user_count,

	e_user_name = 0, 				// nod_add(mod)_user
	e_user_passwd,
	e_user_first,
	e_user_middle,
	e_user_last,
	e_user_admin,
	e_user_count,

	e_hidden_var_expr = 0,			// nod_hidden_var
	e_hidden_var_var,
	e_hidden_var_count
};

} // namespace

namespace Jrd {

typedef Dsql::nod_t NOD_TYPE;

// definition of a syntax node created both in parsing and in context recognition

class dsql_nod : public pool_alloc_rpt<class dsql_nod*, dsql_type_nod>
{
public:
	NOD_TYPE nod_type;			// Type of node
	DSC nod_desc;				// Descriptor
	USHORT nod_line;			// Source line of the statement.
	USHORT nod_column;			// Source column of the statement.
	USHORT nod_count;			// Number of arguments
	USHORT nod_flags;
	// In two adjacent elements (0 and 1) 64-bit value is placed sometimes
	RPT_ALIGN(dsql_nod* nod_arg[1]);

	dsql_nod() : nod_type(Dsql::nod_unknown_type), nod_count(0), nod_flags(0) {}

	SLONG getSlong() const
	{
		fb_assert(nod_type == Dsql::nod_constant);
		fb_assert(nod_desc.dsc_dtype == dtype_long);
		fb_assert((void*) nod_desc.dsc_address == (void*) nod_arg);
		return *((SLONG*) nod_arg);
	}

};

// values of flags
enum nod_flags_vals {
	NOD_AGG_DISTINCT		= 1, // nod_agg_...

	NOD_UNION_ALL			= 1, // nod_list
	NOD_UNION_RECURSIVE 	= 2,
	NOD_SIMPLE_LIST			= 4,	// no need to enclose with blr_begin ... blr_end

	NOD_READ_ONLY			= 1, // nod_access
	NOD_READ_WRITE			= 2,

	NOD_WAIT				= 1, // nod_wait
	NOD_NO_WAIT				= 2,

	NOD_VERSION				= 1, // nod_version
	NOD_NO_VERSION			= 2,

	NOD_CONCURRENCY			= 1, // nod_isolation
	NOD_CONSISTENCY			= 2,
	NOD_READ_COMMITTED		= 4,

	NOD_SHARED				= 1, // nod_lock_mode
	NOD_PROTECTED			= 2,
	NOD_READ				= 4,
	NOD_WRITE				= 8,

	NOD_NO_AUTO_UNDO        = 1, // nod_tra_misc
	NOD_IGNORE_LIMBO        = 2,
	NOD_RESTART_REQUESTS    = 4,

	NOD_NULLS_FIRST			= 1, // nod_order
	NOD_NULLS_LAST			= 2,

	REF_ACTION_CASCADE		= 1, // nod_ref_trig_action
	REF_ACTION_SET_DEFAULT	= 2,
	REF_ACTION_SET_NULL		= 4,
	REF_ACTION_NONE			= 8,
	// Node flag indicates that this node has a different type or result
	// depending on the SQL dialect.
	NOD_COMP_DIALECT		= 16, // nod_...2, see MAKE_desc

	NOD_SELECT_EXPR_SINGLETON	= 1, // nod_select_expr
	NOD_SELECT_EXPR_VALUE		= 2,
	NOD_SELECT_EXPR_RECURSIVE	= 4, // recursive member of recursive CTE
	NOD_SELECT_VIEW_FIELDS		= 8, // view's field list

	NOD_CURSOR_EXPLICIT		= 1, // nod_cursor
	NOD_CURSOR_FOR			= 2,
	NOD_CURSOR_ALL			= USHORT(~0),

	NOD_DT_IGNORE_COLUMN_CHECK	= 1, // nod_cursor, see pass1_cursor_name
	NOD_DT_CTE_USED			= 2,		// nod_derived_table

	NOD_PERMANENT_TABLE			= 1, // nod_def_relation
	NOD_GLOBAL_TEMP_TABLE_PRESERVE_ROWS	= 2,
	NOD_GLOBAL_TEMP_TABLE_DELETE_ROWS	= 3,

	NOD_SPECIAL_SYNTAX		= 1,	// nod_sys_function

	NOD_TRAN_AUTONOMOUS = 1,		// nod_exec_stmt
	NOD_TRAN_COMMON = 2,
	NOD_TRAN_2PC = 3,
	NOD_TRAN_DEFAULT = NOD_TRAN_COMMON
};

} // namespace

#endif // DSQL_NODE_H
