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

#define DYN_REQUEST(drt) dbb->dbb_dyn_req[drt]

const int drq_l_prot_mask		= 0;	// lookup protection mask
const int drq_l_user_name		= 1;	// lookup user name
const int drq_s_rel_con			= 2;	// store relation constraints
const int drq_s_chk_con			= 3;	// store check constraints
const int drq_s_ref_con			= 4;	// store ref constraints
const int drq_f_nxt_con			= 5;	// find next relation constraint name
const int drq_f_nxt_fld			= 6;	// find next field name
const int drq_f_nxt_idx			= 7;	// find next index name
const int drq_f_nxt_trg			= 8;	// find next trigger name
const int drq_c_unq_nam			= 9;	// check for unique field names
const int drq_e_rel_con			= 10;	// erase relation constraints
const int drq_n_idx_seg			= 11;	// count index segments
const int drq_c_dup_con			= 12;	// check for duplicate contraint
const int drq_l_intg_con		= 13;	// lookup an integrity constraint
const int drq_s_dims			= 14;	// store dimensions
const int drq_s_files			= 15;	// store files
const int drq_s_filters			= 16;	// store filters
const int drq_s_gens			= 17;	// store generators
const int drq_s_funcs			= 18;	// store functions
const int drq_s_func_args		= 19;	// store function arguments
const int drq_s_gfields			= 20;	// store global fields
const int drq_s_lfields			= 21;	// store local fields
const int drq_s_gfields2		= 22;	// store global fields
const int drq_s_rels			= 23;	// store relations
const int drq_l_rel_name		= 24;	// lookup relation name
const int drq_l_view_rels		= 25;	// lookup relations in view
const int drq_s_usr_prvs		= 26;	// store user privileges
const int drq_s_classes			= 27;	// store security classes
const int drq_s_sql_lfld		= 28;	// store sql fields
const int drq_s_sql_gfld		= 29;	// store sql fields
const int drq_s_triggers		= 30;	// store triggers
const int drq_s_trg_msgs		= 31;	// store trigger messages
const int drq_s_view_rels		= 32;	// store view relations
const int drq_e_dims			= 33;	// erase dimensions
const int drq_e_filters			= 34;	// erase filters
const int drq_e_func_args		= 35;	// erase functions
const int drq_e_funcs			= 36;	// erase function arguments
const int drq_l_fld_src			= 37;	// lookup a field source
const int drq_e_gfields			= 38;	// erase global fields
const int drq_e_indices			= 39;	// erase indices
const int drq_e_idx_segs		= 40;	// erase index segments
const int drq_l_dep_flds		= 41;	// lookup field referenced by view
const int drq_e_lfield			= 42;	// erase a local field
const int drq_e_rel_con2		= 43;	// erase relation constraints
const int drq_e_rel_idxs		= 44;	// erase indices
const int drq_e_rel_flds		= 45;	// erase relation fields
const int drq_e_view_rels		= 46;	// erase view relations
const int drq_e_relation		= 47;	// erase relation
const int drq_e_rel_con3		= 48;	// erase relation constraints
const int drq_e_usr_prvs		= 49;	// erase user privileges on relation
const int drq_e_shadow			= 50;	// erase shadow
const int drq_e_trg_msgs		= 51;	// erase trigger messages
const int drq_e_trigger			= 52;	// erase trigger
const int drq_l_view_rel2		= 53;	// lookup relations in view
const int drq_m_rel_flds		= 54;	// modify relation fields
const int drq_e_trg_msg			= 55;	// erase trigger message
const int drq_e_class			= 56;	// erase security class
const int drq_l_grant1			= 57;	// lookup grant
const int drq_l_grant2			= 58;	// lookup grant
const int drq_s_grant			= 59;	// store grant
const int drq_l_fld_src2		= 60;	// lookup a field source
const int drq_m_database		= 61;	// modify database
const int drq_m_gfield			= 62;	// modify global field
const int drq_m_index			= 63;	// modify index
const int drq_m_lfield			= 64;	// modify local field
const int drq_m_relation		= 65;	// modify relation
const int drq_m_trigger			= 66;	// modify trigger
const int drq_m_trg_msg			= 67;	// modify trigger message
const int drq_e_grant1			= 68;	// erase grant
const int drq_e_grant2			= 69;	// erase grant
const int drq_s_indices			= 70;	// store indices
const int drq_l_lfield			= 71;	// lookup local field
const int drq_s_idx_segs		= 72;	// store index segments
const int drq_l_unq_idx			= 73;	// lookup a unique index
const int drq_l_primary			= 74;	// lookup a primary something
const int drq_e_trg_msgs2		= 75;	// erase trigger messages
const int drq_e_trigger2		= 76;	// erase trigger
const int drq_s_prcs			= 77;	// store procedure
const int drq_l_prc_name		= 78;	// lookup procedure name
const int drq_s_prc_usr_prvs	= 79;	// store procedure priviledges
const int drq_s_prms			= 80;	// store parameters
const int drq_e_prcs			= 81;	// erase procedure
const int drq_e_prms			= 82;	// erase all of procedure's parameters
const int drq_s_prm_src			= 83;	// store parameter global field
//const int drq_s_intl_info		= 84;	// store RDB$CHARACTER_FIELDS
const int drq_m_prcs			= 85;	// modify procedure
//const int drq_s_log_files		= 86;	// store log files
//const int drq_s_cache			= 87;	// store cache
const int drq_e_prm				= 88;	// erase a procedure parameter
const int drq_s_xcp				= 89;	// store an exception
const int drq_m_xcp				= 90;	// modify an exception
const int drq_e_prc_prvs		= 91;	// erase user privileges on procedure
const int drq_e_prc_prv			= 92;	// erase procedure's privileges
const int drq_e_trg_prv			= 93;	// erase trigger's privileges
//const int drq_d_log			= 94;	// drop log
//const int drq_d_cache			= 95;	// drop cache
//const int drq_l_log_files		= 96;	// lookup log files
//const int drq_l_cache			= 97;	// lookup cache
//const int drq_e_sec_class		= 98;	// delete security classes
//const int drq_l_gfield		= 99;	// lookup global field
const int drq_g_nxt_con			= 100;	// generate next relation constraint name
const int drq_g_nxt_fld			= 101;	// generate next field name
const int drq_g_nxt_idx			= 102;	// generate next index name
const int drq_g_nxt_trg			= 103;	// generate next trigger name
const int drq_l_fld_pos			= 104;	// lookup max field position
const int drq_e_xcp				= 105;	// drop a exception
const int drq_d_gfields			= 106;	// drop a global field for procedure param
const int drq_l_shadow			= 107;	// look up a shadow set
const int drq_l_files			= 108;	// look up for defined files
const int drq_e_l_idx			= 109;	// erase indices defined on a local field
//const int drq_l_idx_seg		= 110;	// Lookup index segments
const int drq_e_l_gfld			= 111;	// erase global field for a local fields
const int drq_gcg1				= 112;	// grantor_can_grant
const int drq_gcg2				= 113;	// grantor_can_grant
const int drq_gcg3				= 114;	// grantor_can_grant
const int drq_gcg4				= 115;	// grantor_can_grant
const int drq_gcg5				= 116;	// grantor_can_grant
const int drq_l_view_idx		= 117;	// table is view?
const int drq_role_gens			= 118;	// store SQL role
const int drq_get_role_nm		= 119;	// get SQL role
const int drq_get_role_au		= 120;	// get SQL role auth
const int drq_del_role_1		= 121;	// delete SQL role from rdb$user_privilege
const int drq_drop_role			= 122;	// delete SQL role from rdb$roles
const int drq_get_rel_owner		= 123;	// get the owner of any relations
const int drq_get_user_priv		= 124;	// get the grantor of user privileges or
										// the user who was granted the privileges
const int drq_g_rel_constr_nm	= 125;	// get relation constraint name
const int drq_e_rel_const		= 126;	// erase relation constraints
const int drq_e_gens			= 127;	// erase generators
const int drq_s_f_class			= 128;	// set the security class name for a field
const int drq_s_u_class			= 129;	// find a unique security class name for a field
const int drq_l_difference		= 130;	// Look up a backup difference file
const int drq_s_difference		= 131;	// Store backup difference file, DYN_define_difference
const int drq_d_difference		= 132;	// Delete backup difference file
const int drq_l_fld_src3		= 133;	// lookup a field source
const int drq_e_fld_prvs		= 134;	// erase user privileges on relation field
const int drq_e_view_prv		= 135;	// erase view's privileges
const int drq_m_chset       	= 136;  // modify charset
const int drq_m_coll        	= 137;  // modify collation
const int drq_m_bfil        	= 138;  // modify blob filter
const int drq_m_fun         	= 139;  // modify udf
const int drq_m_gen         	= 140;  // modify generator
const int drq_m_prm         	= 141;  // modify procedure's parameter
const int drq_m_rol         	= 142;  // modify sql role
const int drq_m_view        	= 143;  // modify view
const int drq_s_colls			= 144;  // store collations
const int drq_l_charset			= 145;	// lookup charset
const int drq_dom_is_array 		= 146;  // lookup domain to see if it's an array
const int drq_l_rel_info		= 147;	// lookup name and flags of one master relation
const int drq_l_rel_info2		= 148;	// lookup names and flags of all master relations
const int drq_l_rel_type		= 149;	// lookup relation type
const int drq_e_colls			= 150;	// erase collations
const int drq_l_rfld_coll		= 151;	// lookup relation field collation
const int drq_l_fld_coll		= 152;	// lookup field collation
const int drq_l_prp_src			= 153;	// lookup a procedure parameter source
const int drq_s_prms2			= 154;	// store parameters (ODS 11.1)
const int drq_l_prm_coll		= 155;	// lookup procedure parameter collation
const int drq_s_prms3			= 156;	// store parameters (ODS 11.2)
const int drq_d_gfields2		= 157;	// drop a global field for procedure param (ODS 11.2)
const int drq_m_map				= 158;  // modify os=>db names mapping
const int drq_l_idx_name		= 159;	// lookup index name
const int drq_l_collation		= 160;	// DSQL/DdlNodes: lookup collation
const int drq_m_charset			= 161;	// DSQL/DdlNodes: modify character set
const int drq_g_nxt_gen_id		= 162;	// generate next generator id
const int drq_g_nxt_prc_id		= 163;	// generate next procedure id
const int drq_g_nxt_xcp_id		= 164;	// generate next exception id
const int drq_l_xcp_name		= 165;	// lookup exception name
const int drq_l_gen_name		= 166;	// lookup generator name
const int drq_e_grant3			= 167;	// revoke all on all
const int drq_s2_difference		= 168;	// Store backup difference file, DYN_mod's change_backup_mode
const int drq_MAX				= 169;

#endif // JRD_DRQ_H
