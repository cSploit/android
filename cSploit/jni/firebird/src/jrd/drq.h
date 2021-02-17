/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		drq.h
 *	DESCRIPTION:	Registry of persistent internal DYN requests
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
 *
 ***************************************************************************
 *
 * Note:
 *
 *   drq_e_l_idx/109 and drq_l_idx_seg/110 are not used any more. The usage of
 *   drq_e_l_idx and drq_l_idx_seg in the past is as the following:
 *
 *
 *      FOR (...) IDX IN RDB$INDICES
 *               WITH IDX.RDB$RELATION_NAME EQ table_name
 *                   ...
 *         FOR (...) FIRST 1 IDX_SEG IN RDB$INDEX_SEGMENTS
 *                      WITH IDX_SEG.RDB$INDEX_NAME EQ IDX.RDB$INDEX_NAME
 *                       AND IDX_SEG.RDB$FIELD_NAME = col_nm
 *                   ...
 *         END_FOR;
 *                   ...
 *      END_FOR;
 *
 * CVC: This comment is wrong: drq_e_l_idx/109 is used in dyn_del.epp.
 *
 */

#ifndef JRD_DRQ_H
#define JRD_DRQ_H

enum drq_type_t
{
	drq_s_rel_con,			// store relation constraints
	drq_s_chk_con,			// store check constraints
	drq_s_ref_con,			// store ref constraints
	drq_f_nxt_con,			// find next relation constraint name
	drq_f_nxt_fld,			// find next field name
	drq_f_nxt_idx,			// find next index name
	drq_f_nxt_trg,			// find next trigger name
	drq_c_unq_nam,			// check for unique field names
	drq_e_rel_con,			// erase relation constraints
	drq_n_idx_seg,			// count index segments
	drq_c_dup_con,			// check for duplicate contraint
	drq_l_intg_con,			// lookup an integrity constraint
	drq_s_files,			// store files
	drq_s_filters,			// store filters
	drq_s_gens,				// store generators
	drq_l_gens,				// lookup a generator
	drq_s_gfields,			// store global fields
	drq_s_lfields,			// store local fields
	drq_s_gfields2,			// store global fields
	drq_s_rels,				// store relations
	drq_l_rel_name,			// lookup relation name
	drq_l_view_rels,		// lookup relations in view
	drq_s_usr_prvs,			// store user privileges
	drq_s_sql_gfld,			// store sql fields
	drq_s_triggers,			// store triggers
	drq_s_view_rels,		// store view relations
	drq_e_dims,				// erase dimensions
	drq_e_filters,			// erase filters
	drq_e_func_args,		// erase functions
	drq_e_funcs,			// erase function arguments
	drq_l_fld_src,			// lookup a field source
	drq_e_gfields,			// erase global fields
	drq_e_indices,			// erase indices
	drq_e_idx_segs,			// erase index segments
	drq_l_dep_flds,			// lookup field referenced by view
	drq_e_lfield,			// erase a local field
	drq_e_rel_con2,			// erase relation constraints
	drq_e_rel_idxs,			// erase indices
	drq_e_rel_flds,			// erase relation fields
	drq_e_view_rels,		// erase view relations
	drq_e_relation,			// erase relation
	drq_e_rel_con3,			// erase relation constraints
	drq_e_usr_prvs,			// erase user privileges on relation
	drq_e_shadow,			// erase shadow
	drq_e_trg_msg,			// erase trigger message
	drq_e_class,			// erase security class
	drq_l_grant1,			// lookup grant
	drq_s_grant,			// store grant
	drq_l_fld_src2,			// lookup a field source
	drq_m_database,			// modify database
	drq_m_index,			// modify index
	drq_m_set_statistics,	// modify index (set statistics)
	drq_e_grant1,			// erase grant
	drq_e_grant2,			// erase grant
	drq_s_indices,			// store indices
	drq_l_lfield,			// lookup local field
	drq_s_idx_segs,			// store index segments
	drq_l_unq_idx,			// lookup a unique index
	drq_l_primary,			// lookup a primary something
	drq_e_trg_msgs2,		// erase trigger messages
	drq_e_trigger2,			// erase trigger
	drq_l_prc_name,			// lookup procedure name
	drq_s_xcp,				// store an exception
	drq_m_xcp,				// modify an exception
	drq_e_trg_prv,			// erase trigger's privileges
	drq_g_nxt_con,			// generate next relation constraint name
	drq_g_nxt_fld,			// generate next field name
	drq_g_nxt_idx,			// generate next index name
	drq_g_nxt_trg,			// generate next trigger name
	drq_l_fld_pos,			// lookup max field position
	drq_e_xcp,				// drop a exception
	drq_l_shadow,			// look up a shadow set
	drq_l_files,			// look up for defined files
	drq_e_l_idx,			// erase indices defined on a local field
	drq_e_l_gfld,			// erase global field for a local fields
	drq_gcg1,				// grantor_can_grant
	drq_gcg2,				// grantor_can_grant
	drq_gcg3,				// grantor_can_grant
	drq_gcg4,				// grantor_can_grant
	drq_gcg5,				// grantor_can_grant
	drq_l_view_idx,			// table is view?
	drq_role_gens,			// store SQL role
	drq_get_role_nm,		// get SQL role
	drq_get_role_au,		// get SQL role auth
	drq_del_role_1,			// delete SQL role from rdb$user_privilege
	drq_drop_role,			// delete SQL role from rdb$roles
	drq_get_rel_owner,		// get the owner of any relations
	drq_get_user_priv,		// get the grantor of user privileges or
							// the user who was granted the privileges
	drq_g_rel_constr_nm,	// get relation constraint name
	drq_e_rel_const,		// erase relation constraints
	drq_e_gens,				// erase generators
	drq_s_f_class,			// set the security class name for a field
	drq_s_u_class,			// find a unique security class name for a field
	drq_l_difference,		// Look up a backup difference file
	drq_s_difference,		// Store backup difference file, DYN_define_difference
	drq_d_difference,		// Delete backup difference file
	drq_l_fld_src3,			// lookup a field source
	drq_e_fld_prvs,			// erase user privileges on relation field
	drq_e_view_prv,			// erase view's privileges
	drq_m_fun,				// modify udf
	drq_m_view,				// modify view
	drq_s_colls,			// store collations
	drq_l_rel_info,			// lookup name and flags of one master relation
	drq_l_rel_info2,		// lookup names and flags of all master relations
	drq_l_rel_type,			// lookup relation type
	drq_e_colls,			// erase collations
	drq_l_rfld_coll,		// lookup relation field collation
	drq_l_fld_coll,			// lookup field collation
	drq_l_prp_src,			// lookup a procedure parameter source
	drq_l_arg_src,			// lookup a function argument source
	drq_l_prm_coll,			// lookup procedure parameter collation
	drq_l_arg_coll,			// lookup function argument collation
	drq_map_sto,			// store login mapping
	drq_map_mod,			// modify/erase login mapping
	drq_l_idx_name,			// lookup index name
	drq_l_collation,		// DSQL/DdlNodes: lookup collation
	drq_m_charset,			// DSQL/DdlNodes: modify character set
	drq_g_nxt_gen_id,		// generate next generator id
	drq_g_nxt_prc_id,		// generate next procedure id
	drq_g_nxt_xcp_id,		// generate next exception id
	drq_l_xcp_name,			// lookup exception name
	drq_l_gen_name,			// lookup generator name
	drq_e_grant3,			// revoke all on all
	drq_s_funcs2,			// store functions (CreateAlterFunctionNode)
	drq_s_func_args2,		// store function arguments (CreateAlterFunctionNode)
	drq_m_funcs2,			// modify functions (CreateAlterFunctionNode)
	drq_e_func_args2,		// erase function arguments (CreateAlterFunctionNode)
	drq_s_prcs2,
	drq_s_prms4,
	drq_s_prm_src2,
	drq_m_prcs2,
	drq_e_prms2,
	drq_m_trigger2,
	drq_e_prcs2,
	drq_e_prc_prvs,
	drq_e_prc_prv,
	drq_e_trg_msgs3,
	drq_e_trigger3,
	drq_e_trg_prv2,
	drq_l_view_rel3,
	drq_m_rel_flds2,
	drq_e_trg_prv3,
	drq_s_pkg,				// store package
	drq_e_pkg,				// erase package
	drq_m_pkg_body,			// create package body
	drq_m_pkg_body2,		// drop package body
	drq_m_pkg_prc,			// drop package body
	drq_m_pkg_fun,			// drop package body
	drq_m_pkg,				// alter package
	drq_l_pkg_funcs,		// lookup packaged functions
	drq_l_pkg_func_args,	// lookup packaged function arguments
	drq_l_pkg_procs,		// lookup packaged procedures
	drq_l_pkg_proc_args,	// lookup packaged procedure arguments
	drq_e_pkg_prv,			// erase package privileges
	drq_m_pkg_prm_defs,		// modify packaged procedure parameters defaults
	drq_m_pkg_arg_defs,		// modify packaged function arguments defaults
	drq_s2_difference,		// Store backup difference file, DYN_mod's change_backup_mode
	drq_l_relation,			// lookup relation before erase
	drq_l_fun_name,			// lookup function name
	drq_g_nxt_fun_id,		// lookup next function ID
	drq_e_arg_gfld,			// erase argument's global field
	drq_e_fun_prvs,			// erase function privileges
	drq_e_fun_prv,			// erase function privileges
	drq_s_fld_src,			// store field source
	drq_e_prm_gfld,			// erase parameter source
	drq_g_nxt_sec_id,		// lookup next security class ID
	drq_f_nxt_gen,			// find next generator name
	drq_g_nxt_gen,			// generate next generator name
	drq_e_ident_gens,		// erase generators (identity column)
	drq_l_ident_gens,		// lookup generators (identity column)
	drq_m_prm_view,			// modify view's field source inherited from parameters
	drq_l_max_coll_id,		// lookup max collation id
	drq_m_fld,				// create domain field
	drq_s_fld_dym,			// store field dymension
	drq_m_fld2,				// alter domain
	drq_c_unq_nam2,			// check for unique field names
	drq_s_rels2,			// store relations
	drq_e_coll_prvs,		// erase collation privileges
	drq_e_xcp_prvs,			// erase exception privileges
	drq_e_gen_prvs,			// erase generator privileges
	drq_e_gfld_prvs,		// erase domain privileges
	drq_g_nxt_nbakhist_id,	// generate next history ID for nbackup

	drq_MAX
};

#endif // JRD_DRQ_H
