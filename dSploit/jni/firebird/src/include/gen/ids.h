
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		relations.h
 *	DESCRIPTION:	System relation definitions
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

// Persistent tables


	const USHORT  f_pag_page = 0;
	const USHORT  f_pag_id = 1;
	const USHORT  f_pag_seq = 2;
	const USHORT  f_pag_type = 3;


	const USHORT  f_dat_desc = 0;
	const USHORT  f_dat_id = 1;
	const USHORT  f_dat_class = 2;
	const USHORT  f_dat_charset = 3;


	const USHORT  f_fld_name = 0;
	const USHORT  f_fld_qname = 1;
	const USHORT  f_fld_v_blr = 2;
	const USHORT  f_fld_v_source = 3;
	const USHORT  f_fld_computed = 4;
	const USHORT  f_fld_csource = 5;
	const USHORT  f_fld_default = 6;
	const USHORT  f_fld_dsource = 7;
	const USHORT  f_fld_length = 8;
	const USHORT  f_fld_scale = 9;
	const USHORT  f_fld_type = 10;
	const USHORT  f_fld_sub_type = 11;
	const USHORT  f_fld_missing = 12;
	const USHORT  f_fld_msource = 13;
	const USHORT  f_fld_desc = 14;
	const USHORT  f_fld_sys_flag = 15;
	const USHORT  f_fld_qheader = 16;
	const USHORT  f_fld_seg_len = 17;
	const USHORT  f_fld_estring = 18;
	const USHORT  f_fld_ext_length = 19;
	const USHORT  f_fld_ext_scale = 20;
	const USHORT  f_fld_ext_type = 21;
	const USHORT  f_fld_dimensions = 22;
	const USHORT  f_fld_null_flag = 23;
	const USHORT  f_fld_char_length = 24;
	const USHORT  f_fld_coll_id = 25;
	const USHORT  f_fld_charset_id = 26;
	const USHORT  f_fld_precision = 27;


	const USHORT  f_seg_name = 0;
	const USHORT  f_seg_field = 1;
	const USHORT  f_seg_position = 2;
	const USHORT  f_seg_statistics = 3;


	const USHORT  f_idx_name = 0;
	const USHORT  f_idx_relation = 1;
	const USHORT  f_idx_id = 2;
	const USHORT  f_idx_flag = 3;
	const USHORT  f_idx_desc = 4;
	const USHORT  f_idx_count = 5;
	const USHORT  f_idx_inactive = 6;
	const USHORT  f_idx_type = 7;
	const USHORT  f_idx_foreign = 8;
	const USHORT  f_idx_sys_flag = 9;
	const USHORT  f_idx_exp_blr = 10;
	const USHORT  f_idx_exp_source = 11;
	const USHORT  f_idx_statistics = 12;


	const USHORT  f_rfr_fname = 0;
	const USHORT  f_rfr_rname = 1;
	const USHORT  f_rfr_sname = 2;
	const USHORT  f_rfr_qname = 3;
	const USHORT  f_rfr_base = 4;
	const USHORT  f_rfr_estring = 5;
	const USHORT  f_rfr_position = 6;
	const USHORT  f_rfr_qheader = 7;
	const USHORT  f_rfr_flag = 8;
	const USHORT  f_rfr_id = 9;
	const USHORT  f_rfr_context = 10;
	const USHORT  f_rfr_desc = 11;
	const USHORT  f_rfr_default = 12;
	const USHORT  f_rfr_sys_flag = 13;
	const USHORT  f_rfr_class = 14;
	const USHORT  f_rfr_complex = 15;
	const USHORT  f_rfr_null_flag = 16;
	const USHORT  f_rfr_dsource = 17;
	const USHORT  f_rfr_coll_id = 18;


	const USHORT  f_rel_blr = 0;
	const USHORT  f_rel_source = 1;
	const USHORT  f_rel_desc = 2;
	const USHORT  f_rel_id = 3;
	const USHORT  f_rel_sys_flag = 4;
	const USHORT  f_rel_key_len = 5;
	const USHORT  f_rel_format = 6;
	const USHORT  f_rel_field_id = 7;
	const USHORT  f_rel_name = 8;
	const USHORT  f_rel_class = 9;
	const USHORT  f_rel_ext_file = 10;
	const USHORT  f_rel_runtime = 11;
	const USHORT  f_rel_ext_desc = 12;
	const USHORT  f_rel_owner = 13;
	const USHORT  f_rel_def_class = 14;
	const USHORT  f_rel_flags = 15;
	const USHORT  f_rel_type = 16;


	const USHORT  f_vrl_vname = 0;
	const USHORT  f_vrl_rname = 1;
	const USHORT  f_vrl_context = 2;
	const USHORT  f_vrl_cname = 3;


	const USHORT  f_fmt_rid = 0;
	const USHORT  f_fmt_format = 1;
	const USHORT  f_fmt_desc = 2;


	const USHORT  f_cls_class = 0;
	const USHORT  f_cls_acl = 1;
	const USHORT  f_cls_desc = 2;


	const USHORT  f_file_name = 0;
	const USHORT  f_file_seq = 1;
	const USHORT  f_file_start = 2;
	const USHORT  f_file_length = 3;
	const USHORT  f_file_flags = 4;
	const USHORT  f_file_shad_num = 5;


	const USHORT  f_typ_field = 0;
	const USHORT  f_typ_type = 1;
	const USHORT  f_typ_name = 2;
	const USHORT  f_typ_desc = 3;
	const USHORT  f_typ_sys_flag = 4;


	const USHORT  f_trg_name = 0;
	const USHORT  f_trg_rname = 1;
	const USHORT  f_trg_seq = 2;
	const USHORT  f_trg_type = 3;
	const USHORT  f_trg_source = 4;
	const USHORT  f_trg_blr = 5;
	const USHORT  f_trg_desc = 6;
	const USHORT  f_trg_inactive = 7;
	const USHORT  f_trg_sys_flag = 8;
	const USHORT  f_trg_flags = 9;
	const USHORT  f_trg_valid_blr = 10;
	const USHORT  f_trg_debug_info = 11;


	const USHORT  f_dpd_name = 0;
	const USHORT  f_dpd_o_name = 1;
	const USHORT  f_dpd_f_name = 2;
	const USHORT  f_dpd_type = 3;
	const USHORT  f_dpd_o_type = 4;


	const USHORT  f_fun_name = 0;
	const USHORT  f_fun_type = 1;
	const USHORT  f_fun_qname = 2;
	const USHORT  f_fun_desc = 3;
	const USHORT  f_fun_module = 4;
	const USHORT  f_fun_entry = 5;
	const USHORT  f_fun_ret_arg = 6;
	const USHORT  f_fun_sys_flag = 7;


	const USHORT  f_arg_fun_name = 0;
	const USHORT  f_arg_pos = 1;
	const USHORT  f_arg_mech = 2;
	const USHORT  f_arg_type = 3;
	const USHORT  f_arg_scale = 4;
	const USHORT  f_arg_length = 5;
	const USHORT  f_arg_sub_type = 6;
	const USHORT  f_arg_charset_id = 7;
	const USHORT  f_arg_precision = 8;
	const USHORT  f_arg_char_length = 9;


	const USHORT  f_flt_name = 0;
	const USHORT  f_flt_desc = 1;
	const USHORT  f_flt_module = 2;
	const USHORT  f_flt_entry = 3;
	const USHORT  f_flt_input = 4;
	const USHORT  f_flt_output = 5;
	const USHORT  f_flt_sys_flag = 6;


	const USHORT  f_msg_trigger = 0;
	const USHORT  f_msg_number = 1;
	const USHORT  f_msg_msg = 2;


	const USHORT  f_prv_user = 0;
	const USHORT  f_prv_grantor = 1;
	const USHORT  f_prv_priv = 2;
	const USHORT  f_prv_grant = 3;
	const USHORT  f_prv_rname = 4;
	const USHORT  f_prv_fname = 5;
	const USHORT  f_prv_u_type = 6;
	const USHORT  f_prv_o_type = 7;


	const USHORT  f_trn_id = 0;
	const USHORT  f_trn_state = 1;
	const USHORT  f_trn_time = 2;
	const USHORT  f_trn_desc = 3;


	const USHORT  f_gen_name = 0;
	const USHORT  f_gen_id = 1;
	const USHORT  f_gen_sys_flag = 2;
	const USHORT  f_gen_desc = 3;


	const USHORT  f_dims_fname = 0;
	const USHORT  f_dims_dim = 1;
	const USHORT  f_dims_lower = 2;
	const USHORT  f_dims_upper = 3;


	const USHORT  f_rcon_cname = 0;
	const USHORT  f_rcon_ctype = 1;
	const USHORT  f_rcon_rname = 2;
	const USHORT  f_rcon_dfr = 3;
	const USHORT  f_rcon_idfr = 4;
	const USHORT  f_rcon_iname = 5;


	const USHORT  f_refc_cname = 0;
	const USHORT  f_refc_uq = 1;
	const USHORT  f_refc_match = 2;
	const USHORT  f_refc_upd_rul = 3;
	const USHORT  f_refc_del_rul = 4;


	const USHORT  f_ccon_cname = 0;
	const USHORT  f_ccon_tname = 1;


	const USHORT  f_log_name = 0;
	const USHORT  f_log_seq = 1;
	const USHORT  f_log_length = 2;
	const USHORT  f_log_partitions = 3;
	const USHORT  f_log_p_offset = 4;
	const USHORT  f_log_flags = 5;


	const USHORT  f_prc_name = 0;
	const USHORT  f_prc_id = 1;
	const USHORT  f_prc_inputs = 2;
	const USHORT  f_prc_outputs = 3;
	const USHORT  f_prc_desc = 4;
	const USHORT  f_prc_source = 5;
	const USHORT  f_prc_blr = 6;
	const USHORT  f_prc_class = 7;
	const USHORT  f_prc_owner = 8;
	const USHORT  f_prc_runtime = 9;
	const USHORT  f_prc_sys_flag = 10;
	const USHORT  f_prc_type = 11;
	const USHORT  f_prc_valid_blr = 12;
	const USHORT  f_prc_debug_info = 13;


	const USHORT  f_prm_name = 0;
	const USHORT  f_prm_procedure = 1;
	const USHORT  f_prm_number = 2;
	const USHORT  f_prm_type = 3;
	const USHORT  f_prm_sname = 4;
	const USHORT  f_prm_desc = 5;
	const USHORT  f_prm_sys_flag = 6;
	const USHORT  f_prm_default = 7;
	const USHORT  f_prm_dsource = 8;
	const USHORT  f_prm_coll_id = 9;
	const USHORT  f_prm_null_flag = 10;
	const USHORT  f_prm_mech = 11;
	const USHORT  f_prm_fname = 12;
	const USHORT  f_prm_rname = 13;


	const USHORT  f_cs_cs_name = 0;
	const USHORT  f_cs_form_of_use = 1;
	const USHORT  f_cs_num_chars = 2;
	const USHORT  f_cs_def_collate = 3;
	const USHORT  f_cs_id = 4;
	const USHORT  f_cs_sys_flag = 5;
	const USHORT  f_cs_desc = 6;
	const USHORT  f_cs_fun_name = 7;
	const USHORT  f_cs_bytes_char = 8;


	const USHORT  f_coll_name = 0;
	const USHORT  f_coll_id = 1;
	const USHORT  f_coll_cs_id = 2;
	const USHORT  f_coll_attr = 3;
	const USHORT  f_coll_sys_flag = 4;
	const USHORT  f_coll_desc = 5;
	const USHORT  f_coll_fun_name = 6;
	const USHORT  f_coll_base_collation_name = 7;
	const USHORT  f_coll_specific_attr = 8;


	const USHORT  f_xcp_name = 0;
	const USHORT  f_xcp_number = 1;
	const USHORT  f_xcp_msg = 2;
	const USHORT  f_xcp_desc = 3;
	const USHORT  f_xcp_sys_flag = 4;


	const USHORT  f_rol_name = 0;
	const USHORT  f_rol_owner = 1;
	const USHORT  f_rol_desc = 2;
	const USHORT  f_rol_sys_flag = 3;


	const USHORT  f_backup_id = 0;
	const USHORT  f_backup_time = 1;
	const USHORT  f_backup_level = 2;
	const USHORT  f_backup_guid = 3;
	const USHORT  f_backup_scn = 4;
	const USHORT  f_backup_name = 5;


// Virtual tables


	const USHORT  f_mon_db_name = 0;
	const USHORT  f_mon_db_page_size = 1;
	const USHORT  f_mon_db_ods_major = 2;
	const USHORT  f_mon_db_ods_minor = 3;
	const USHORT  f_mon_db_oit = 4;
	const USHORT  f_mon_db_oat = 5;
	const USHORT  f_mon_db_ost = 6;
	const USHORT  f_mon_db_nt = 7;
	const USHORT  f_mon_db_page_bufs = 8;
	const USHORT  f_mon_db_dialect = 9;
	const USHORT  f_mon_db_shut_mode = 10;
	const USHORT  f_mon_db_sweep_int = 11;
	const USHORT  f_mon_db_read_only = 12;
	const USHORT  f_mon_db_forced_writes = 13;
	const USHORT  f_mon_db_res_space = 14;
	const USHORT  f_mon_db_created = 15;
	const USHORT  f_mon_db_pages = 16;
	const USHORT  f_mon_db_stat_id = 17;
	const USHORT  f_mon_db_backup_state = 18;


	const USHORT  f_mon_att_id = 0;
	const USHORT  f_mon_att_server_pid = 1;
	const USHORT  f_mon_att_state = 2;
	const USHORT  f_mon_att_name = 3;
	const USHORT  f_mon_att_user = 4;
	const USHORT  f_mon_att_role = 5;
	const USHORT  f_mon_att_remote_proto = 6;
	const USHORT  f_mon_att_remote_addr = 7;
	const USHORT  f_mon_att_remote_pid = 8;
	const USHORT  f_mon_att_charset_id = 9;
	const USHORT  f_mon_att_timestamp = 10;
	const USHORT  f_mon_att_gc = 11;
	const USHORT  f_mon_att_remote_process = 12;
	const USHORT  f_mon_att_stat_id = 13;


	const USHORT  f_mon_tra_id = 0;
	const USHORT  f_mon_tra_att_id = 1;
	const USHORT  f_mon_tra_state = 2;
	const USHORT  f_mon_tra_timestamp = 3;
	const USHORT  f_mon_tra_top = 4;
	const USHORT  f_mon_tra_oit = 5;
	const USHORT  f_mon_tra_oat = 6;
	const USHORT  f_mon_tra_iso_mode = 7;
	const USHORT  f_mon_tra_lock_timeout = 8;
	const USHORT  f_mon_tra_read_only = 9;
	const USHORT  f_mon_tra_auto_commit = 10;
	const USHORT  f_mon_tra_auto_undo = 11;
	const USHORT  f_mon_tra_stat_id = 12;


	const USHORT  f_mon_stmt_id = 0;
	const USHORT  f_mon_stmt_att_id = 1;
	const USHORT  f_mon_stmt_tra_id = 2;
	const USHORT  f_mon_stmt_state = 3;
	const USHORT  f_mon_stmt_timestamp = 4;
	const USHORT  f_mon_stmt_sql_text = 5;
	const USHORT  f_mon_stmt_stat_id = 6;


	const USHORT  f_mon_call_id = 0;
	const USHORT  f_mon_call_stmt_id = 1;
	const USHORT  f_mon_call_caller_id = 2;
	const USHORT  f_mon_call_name = 3;
	const USHORT  f_mon_call_type = 4;
	const USHORT  f_mon_call_timestamp = 5;
	const USHORT  f_mon_call_src_line = 6;
	const USHORT  f_mon_call_src_column = 7;
	const USHORT  f_mon_call_stat_id = 8;


	const USHORT  f_mon_io_stat_id = 0;
	const USHORT  f_mon_io_stat_group = 1;
	const USHORT  f_mon_io_page_reads = 2;
	const USHORT  f_mon_io_page_writes = 3;
	const USHORT  f_mon_io_page_fetches = 4;
	const USHORT  f_mon_io_page_marks = 5;


	const USHORT  f_mon_rec_stat_id = 0;
	const USHORT  f_mon_rec_stat_group = 1;
	const USHORT  f_mon_rec_seq_reads = 2;
	const USHORT  f_mon_rec_idx_reads = 3;
	const USHORT  f_mon_rec_inserts = 4;
	const USHORT  f_mon_rec_updates = 5;
	const USHORT  f_mon_rec_deletes = 6;
	const USHORT  f_mon_rec_backouts = 7;
	const USHORT  f_mon_rec_purges = 8;
	const USHORT  f_mon_rec_expunges = 9;


	const USHORT  f_mon_ctx_var_att_id = 0;
	const USHORT  f_mon_ctx_var_tra_id = 1;
	const USHORT  f_mon_ctx_var_name = 2;
	const USHORT  f_mon_ctx_var_value = 3;


	const USHORT  f_mon_mem_stat_id = 0;
	const USHORT  f_mon_mem_stat_group = 1;
	const USHORT  f_mon_mem_cur_used = 2;
	const USHORT  f_mon_mem_cur_alloc = 3;
	const USHORT  f_mon_mem_max_used = 4;
	const USHORT  f_mon_mem_max_alloc = 5;


