/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		fields.h
 *	DESCRIPTION:	Global system relation fields
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


//	type					, name				, dtype			, length					, sub_type					, dflt_blr	, nullable)
	FIELD(fld_context		, nam_v_context		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_ctx_name		, nam_context		, dtype_text	, 255						, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_description	, nam_description	, dtype_blob	, BLOB_SIZE					, isc_blob_text				, NULL		, true)
	FIELD(fld_edit_string	, nam_edit_string	, dtype_varying	, 127						, 0							, NULL		, true)
	FIELD(fld_f_id			, nam_f_id			, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_f_name		, nam_f_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_flag			, nam_sys_flag		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, false)
	FIELD(fld_flag_nullable	, nam_sys_nullflag	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_i_id			, nam_i_id			, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_i_name		, nam_i_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_f_length		, nam_f_length		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_f_position	, nam_f_position	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_f_scale		, nam_f_scale		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_f_type		, nam_f_type		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_format		, nam_fmt			, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_key_length	, nam_key_length	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_p_number		, nam_p_number		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_p_sequence	, nam_p_sequence	, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_p_type		, nam_p_type		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_q_header		, nam_q_header		, dtype_blob	, BLOB_SIZE					, isc_blob_text				, NULL		, true)
	FIELD(fld_r_id			, nam_r_id			, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_r_name		, nam_r_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_s_count		, nam_s_count		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_s_length		, nam_s_length		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_source		, nam_source		, dtype_blob	, BLOB_SIZE					, isc_blob_text				, NULL		, true)
	FIELD(fld_sub_type		, nam_f_sub_type	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_v_blr			, nam_v_blr			, dtype_blob	, BLOB_SIZE					, isc_blob_blr				, NULL		, true)
	FIELD(fld_validation	, nam_vl_blr		, dtype_blob	, BLOB_SIZE					, isc_blob_blr				, NULL		, true)
	FIELD(fld_value			, nam_value			, dtype_blob	, BLOB_SIZE					, isc_blob_blr				, NULL		, true)
	FIELD(fld_class			, nam_class			, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_acl			, nam_acl			, dtype_blob	, BLOB_SIZE					, isc_blob_acl				, NULL		, true)
	FIELD(fld_file_name		, nam_file_name		, dtype_varying	, 255						, 0							, NULL		, true)
	FIELD(fld_file_name2	, nam_file_name2	, dtype_varying	, 255						, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_file_seq		, nam_file_seq		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_file_start	, nam_file_start	, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_file_length	, nam_file_length	, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_file_flags	, nam_file_flags	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_trigger		, nam_trigger		, dtype_blob	, BLOB_SIZE					, isc_blob_blr				, NULL		, true)

	FIELD(fld_trg_name		, nam_trg_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_gnr_name		, nam_gnr_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_fun_name		, nam_fun_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_ext_name		, nam_ext_name		, dtype_text	, 255						, 0							, NULL		, true)
	FIELD(fld_typ_name		, nam_typ_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_dimensions	, nam_dimensions	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_runtime		, nam_runtime		, dtype_blob	, BLOB_SIZE					, isc_blob_summary			, NULL		, true)
	FIELD(fld_trg_seq		, nam_trg_seq		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_gnr_type		, nam_gnr_type		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_trg_type		, nam_trg_type		, dtype_int64	, sizeof(SINT64)			, 0							, NULL		, true)
	FIELD(fld_obj_type		, nam_obj_type		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_mechanism		, nam_mechanism		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_f_descr		, nam_desc			, dtype_blob	, BLOB_SIZE					, isc_blob_format			, NULL		, true)
	FIELD(fld_fun_type		, nam_fun_type		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_trans_id		, nam_trans_id		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_trans_state	, nam_trans_state	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_time			, nam_time			, dtype_timestamp, TIMESTAMP_SIZE			, 0							, NULL		, true)
	FIELD(fld_trans_desc	, nam_trans_desc	, dtype_blob	, BLOB_SIZE					, isc_blob_tra				, NULL		, true)
	FIELD(fld_msg			, nam_msg			, dtype_varying	, 1023						, 0							, NULL		, true)
	FIELD(fld_msg_num		, nam_msg_num		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_user			, nam_user			, dtype_text	, USERNAME_LENGTH			, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_privilege		, nam_privilege		, dtype_text	, 6							, 0							, NULL		, true)
	FIELD(fld_ext_desc		, nam_ext_desc		, dtype_blob	, BLOB_SIZE					, isc_blob_extfile			, NULL		, true)
	FIELD(fld_shad_num		, nam_shad_num		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_gen_name		, nam_gen_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_gen_id		, nam_gen_id		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_bound			, nam_bound			, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_dim			, nam_dim			, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_statistics	, nam_statistics	, dtype_double	, sizeof(double)			, 0							, NULL		, true)
	FIELD(fld_null_flag		, nam_null_flag		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_con_name		, nam_con_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_con_type		, nam_con_type		, dtype_text	, 11						, 0							, NULL		, true)
	FIELD(fld_defer			, nam_defer			, dtype_text	, 3							, 0							, dflt_no	, true)
	FIELD(fld_match			, nam_match			, dtype_text	, 7							, 0							, dflt_full	, true)
	FIELD(fld_rule			, nam_rule			, dtype_text	, 11						, 0							, dflt_restrict, true)
	FIELD(fld_file_partitions, nam_file_partitions, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_prc_blr		, nam_prc_blr		, dtype_blob	, BLOB_SIZE					, isc_blob_blr				, NULL		, true)
	FIELD(fld_prc_id		, nam_prc_id		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_prc_prm		, nam_prc_prm		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_prc_name		, nam_prc_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_prm_name		, nam_prm_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_prm_number	, nam_prm_number	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_prm_type		, nam_prm_type		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)

	FIELD(fld_charset_name	, nam_charset_name	, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_charset_id	, nam_charset_id	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_collate_name	, nam_collate_name	, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_collate_id	, nam_collate_id	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_num_chars		, nam_num_chars		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_xcp_name		, nam_xcp_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_xcp_number	, nam_xcp_number	, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_file_p_offset	, nam_file_p_offset	, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_f_precision	, nam_f_precision	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)

	FIELD(fld_backup_id		, nam_backup_id		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_backup_level	, nam_backup_level	, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_guid			, nam_guid			, dtype_text	, 38						, 0							, NULL		, true)
	FIELD(fld_scn			, nam_scn			, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)

	FIELD(fld_specific_attr	, nam_specific_attr	, dtype_blob	, BLOB_SIZE					, isc_blob_text				, NULL		, true)

	FIELD(fld_r_type		, nam_r_type		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_prc_type		, nam_prc_type		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)

	FIELD(fld_att_id		, nam_att_id		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_stmt_id		, nam_stmt_id		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_call_id		, nam_call_id		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_stat_id		, nam_stat_id		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)

	FIELD(fld_pid			, nam_pid			, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_state			, nam_state			, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_ods_number	, nam_ods_number	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_page_size		, nam_page_size		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_page_bufs		, nam_page_bufs		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_shut_mode		, nam_shut_mode		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_sql_dialect	, nam_sql_dialect	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_sweep_int		, nam_sweep_int		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)
	FIELD(fld_counter		, nam_counter		, dtype_int64	, sizeof(SINT64)			, 0							, NULL		, true)

	FIELD(fld_remote_proto	, nam_remote_proto	, dtype_varying	, 10						, dsc_text_type_ascii		, NULL		, true)
	FIELD(fld_remote_addr	, nam_remote_addr	, dtype_varying	, 255						, dsc_text_type_ascii		, NULL		, true)

	FIELD(fld_iso_mode		, nam_iso_mode		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_lock_timeout	, nam_lock_timeout	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_backup_state	, nam_backup_state	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_stat_group	, nam_stat_group	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)

	FIELD(fld_debug_info	, nam_debug_info	, dtype_blob	, BLOB_SIZE					, isc_blob_debug_info		, NULL		, true)
	FIELD(fld_prm_mechanism	, nam_prm_mechanism	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_src_info		, nam_src_info		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)

	FIELD(fld_ctx_var_name	, nam_ctx_var_name	, dtype_varying	, 80						, 0							, NULL		, true)
	FIELD(fld_ctx_var_value	, nam_ctx_var_value	, dtype_varying	, 255						, 0							, NULL		, true)

	FIELD(fld_engine_name	, nam_engine_name	, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)

	FIELD(fld_pkg_name		, nam_pkg_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)

	FIELD(fld_fun_id		, nam_fun_id		, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_fun_blr		, nam_fun_blr		, dtype_blob	, BLOB_SIZE					, isc_blob_blr				, NULL		, true)
	FIELD(fld_arg_name		, nam_arg_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_arg_mechanism	, nam_arg_mechanism	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)

	FIELD(fld_identity_type	, nam_identity_type	, dtype_short	, sizeof(SSHORT)			, 0							, NULL		, true)
	FIELD(fld_bool			, nam_bool			, dtype_boolean	, 1							, 0							, NULL		, true)

	FIELD(fld_user_name		, nam_user_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_attr_key		, nam_sec_attr_key	, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_attr_value	, nam_sec_attr_value, dtype_varying	, 255						, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_name_part		, nam_name_part		, dtype_text	, 32						, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_client_ver	, nam_client_ver	, dtype_varying	, 255						, dsc_text_type_ascii		, NULL		, true)
	FIELD(fld_remote_ver	, nam_remote_ver	, dtype_varying	, 255						, dsc_text_type_ascii		, NULL		, true)
	FIELD(fld_host_name		, nam_host_name		, dtype_varying	, 255						, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_os_user		, nam_os_user		, dtype_varying	, 255						, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_gen_val		, nam_gen_val		, dtype_int64	, sizeof(SINT64)			, 0							, NULL		, true)
	FIELD(fld_auth_method	, nam_auth_method	, dtype_varying	, 255						, dsc_text_type_ascii		, NULL		, true)

	FIELD(fld_linger		, nam_linger		, dtype_long	, sizeof(SLONG)				, 0							, NULL		, true)

	FIELD(fld_map_name		, nam_map_name		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, false)
	FIELD(fld_map_using		, nam_map_using		, dtype_text	, 1							, dsc_text_type_metadata	, NULL		, false)
	FIELD(fld_map_plugin	, nam_map_plugin	, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_map_db		, nam_map_db		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_map_from_type	, nam_map_from_type	, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, false)
	FIELD(fld_map_from		, nam_map_from		, dtype_text	, 255						, dsc_text_type_metadata	, NULL		, true)
	FIELD(fld_map_to		, nam_map_to		, dtype_text	, MAX_SQL_IDENTIFIER_LEN	, dsc_text_type_metadata	, NULL		, true)

	FIELD(fld_gen_increment	, nam_gen_increment	, dtype_long	, sizeof(SLONG)				, 0							, NULL		, false)
