/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		inf.h
 *	DESCRIPTION:	Information call declarations.
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
 * 2001.07.28: John Bellardo: Added isc_info_rsb_skip to support LIMIT.
 */

#ifndef JRD_INF_PUB_H
#define JRD_INF_PUB_H

/* Common, structural codes */
/****************************/

#define isc_info_end			1
#define isc_info_truncated		2
#define isc_info_error			3
#define isc_info_data_not_ready	          4
#define isc_info_length			126
#define isc_info_flag_end		127

/******************************/
/* Database information items */
/******************************/

enum db_info_types
{
	isc_info_db_id			= 4,
	isc_info_reads			= 5,
	isc_info_writes		    = 6,
	isc_info_fetches		= 7,
	isc_info_marks			= 8,

	isc_info_implementation = 11,
	isc_info_isc_version		= 12,
	isc_info_base_level		= 13,
	isc_info_page_size		= 14,
	isc_info_num_buffers	= 15,
	isc_info_limbo			= 16,
	isc_info_current_memory	= 17,
	isc_info_max_memory		= 18,
	isc_info_window_turns	= 19,
	isc_info_license		= 20,

	isc_info_allocation		= 21,
	isc_info_attachment_id	 = 22,
	isc_info_read_seq_count	= 23,
	isc_info_read_idx_count	= 24,
	isc_info_insert_count		= 25,
	isc_info_update_count		= 26,
	isc_info_delete_count		= 27,
	isc_info_backout_count	 	= 28,
	isc_info_purge_count		= 29,
	isc_info_expunge_count		= 30,

	isc_info_sweep_interval	= 31,
	isc_info_ods_version		= 32,
	isc_info_ods_minor_version	= 33,
	isc_info_no_reserve		= 34,
	/* Begin deprecated WAL and JOURNAL items. */
	isc_info_logfile		= 35,
	isc_info_cur_logfile_name	= 36,
	isc_info_cur_log_part_offset	= 37,
	isc_info_num_wal_buffers	= 38,
	isc_info_wal_buffer_size	= 39,
	isc_info_wal_ckpt_length	= 40,

	isc_info_wal_cur_ckpt_interval = 41,
	isc_info_wal_prv_ckpt_fname	= 42,
	isc_info_wal_prv_ckpt_poffset	= 43,
	isc_info_wal_recv_ckpt_fname	= 44,
	isc_info_wal_recv_ckpt_poffset = 45,
	isc_info_wal_grpc_wait_usecs	= 47,
	isc_info_wal_num_io		= 48,
	isc_info_wal_avg_io_size	= 49,
	isc_info_wal_num_commits	= 50,
	isc_info_wal_avg_grpc_size	= 51,
	/* End deprecated WAL and JOURNAL items. */

	isc_info_forced_writes		= 52,
	isc_info_user_names = 53,
	isc_info_page_errors = 54,
	isc_info_record_errors = 55,
	isc_info_bpage_errors = 56,
	isc_info_dpage_errors = 57,
	isc_info_ipage_errors = 58,
	isc_info_ppage_errors = 59,
	isc_info_tpage_errors = 60,

	isc_info_set_page_buffers = 61,
	isc_info_db_sql_dialect = 62,
	isc_info_db_read_only = 63,
	isc_info_db_size_in_pages = 64,

	/* Values 65 -100 unused to avoid conflict with InterBase */

	frb_info_att_charset = 101,
	isc_info_db_class = 102,
	isc_info_firebird_version = 103,
	isc_info_oldest_transaction = 104,
	isc_info_oldest_active = 105,
	isc_info_oldest_snapshot = 106,
	isc_info_next_transaction = 107,
	isc_info_db_provider = 108,
	isc_info_active_transactions = 109,
	isc_info_active_tran_count = 110,
	isc_info_creation_date = 111,
	isc_info_db_file_size = 112,
	fb_info_page_contents = 113,

	isc_info_db_last_value   /* Leave this LAST! */
};

#define isc_info_version isc_info_isc_version


/**************************************/
/* Database information return values */
/**************************************/

enum  info_db_implementations
{
	isc_info_db_impl_rdb_vms = 1,
	isc_info_db_impl_rdb_eln = 2,
	isc_info_db_impl_rdb_eln_dev = 3,
	isc_info_db_impl_rdb_vms_y = 4,
	isc_info_db_impl_rdb_eln_y = 5,
	isc_info_db_impl_jri = 6,
	isc_info_db_impl_jsv = 7,

	isc_info_db_impl_isc_apl_68K = 25,
	isc_info_db_impl_isc_vax_ultr = 26,
	isc_info_db_impl_isc_vms = 27,
	isc_info_db_impl_isc_sun_68k = 28,
	isc_info_db_impl_isc_os2 = 29,
	isc_info_db_impl_isc_sun4 = 30,	   /* 30 */

	isc_info_db_impl_isc_hp_ux = 31,
	isc_info_db_impl_isc_sun_386i = 32,
	isc_info_db_impl_isc_vms_orcl = 33,
	isc_info_db_impl_isc_mac_aux = 34,
	isc_info_db_impl_isc_rt_aix = 35,
	isc_info_db_impl_isc_mips_ult = 36,
	isc_info_db_impl_isc_xenix = 37,
	isc_info_db_impl_isc_dg = 38,
	isc_info_db_impl_isc_hp_mpexl = 39,
	isc_info_db_impl_isc_hp_ux68K = 40,	  /* 40 */

	isc_info_db_impl_isc_sgi = 41,
	isc_info_db_impl_isc_sco_unix = 42,
	isc_info_db_impl_isc_cray = 43,
	isc_info_db_impl_isc_imp = 44,
	isc_info_db_impl_isc_delta = 45,
	isc_info_db_impl_isc_next = 46,
	isc_info_db_impl_isc_dos = 47,
	isc_info_db_impl_m88K = 48,
	isc_info_db_impl_unixware = 49,
	isc_info_db_impl_isc_winnt_x86 = 50,

	isc_info_db_impl_isc_epson = 51,
	isc_info_db_impl_alpha_osf = 52,
	isc_info_db_impl_alpha_vms = 53,
	isc_info_db_impl_netware_386 = 54,
	isc_info_db_impl_win_only = 55,
	isc_info_db_impl_ncr_3000 = 56,
	isc_info_db_impl_winnt_ppc = 57,
	isc_info_db_impl_dg_x86 = 58,
	isc_info_db_impl_sco_ev = 59,
	isc_info_db_impl_i386 = 60,

	isc_info_db_impl_freebsd = 61,
	isc_info_db_impl_netbsd = 62,
	isc_info_db_impl_darwin_ppc = 63,
	isc_info_db_impl_sinixz = 64,

	isc_info_db_impl_linux_sparc = 65,
	isc_info_db_impl_linux_amd64 = 66,

	isc_info_db_impl_freebsd_amd64 = 67,

	isc_info_db_impl_winnt_amd64 = 68,

	isc_info_db_impl_linux_ppc = 69,
	isc_info_db_impl_darwin_x86 = 70,
	isc_info_db_impl_linux_mipsel = 71,
	isc_info_db_impl_linux_mips = 72,
	isc_info_db_impl_darwin_x64 = 73,
	isc_info_db_impl_sun_amd64 = 74,

	isc_info_db_impl_linux_arm = 75,
	isc_info_db_impl_linux_ia64 = 76,

	isc_info_db_impl_darwin_ppc64 = 77,
	isc_info_db_impl_linux_s390x = 78,
	isc_info_db_impl_linux_s390 = 79,

	isc_info_db_impl_linux_sh = 80,
	isc_info_db_impl_linux_sheb = 81,
	isc_info_db_impl_linux_hppa = 82,
	isc_info_db_impl_linux_alpha = 83,

	isc_info_db_impl_last_value   // Leave this LAST!
};


enum info_db_class
{
	isc_info_db_class_access = 1,
	isc_info_db_class_y_valve = 2,
	isc_info_db_class_rem_int = 3,
	isc_info_db_class_rem_srvr = 4,
	isc_info_db_class_pipe_int = 7,
	isc_info_db_class_pipe_srvr = 8,
	isc_info_db_class_sam_int = 9,
	isc_info_db_class_sam_srvr = 10,
	isc_info_db_class_gateway = 11,
	isc_info_db_class_cache = 12,
	isc_info_db_class_classic_access = 13,
	isc_info_db_class_server_access = 14,

	isc_info_db_class_last_value   /* Leave this LAST! */
};

enum info_db_provider
{
	isc_info_db_code_rdb_eln = 1,
	isc_info_db_code_rdb_vms = 2,
	isc_info_db_code_interbase = 3,
	isc_info_db_code_firebird = 4,

	isc_info_db_code_last_value   /* Leave this LAST! */
};


/*****************************/
/* Request information items */
/*****************************/

#define isc_info_number_messages	4
#define isc_info_max_message		5
#define isc_info_max_send		6
#define isc_info_max_receive		7
#define isc_info_state			8
#define isc_info_message_number	9
#define isc_info_message_size		10
#define isc_info_request_cost		11
#define isc_info_access_path		12
#define isc_info_req_select_count	13
#define isc_info_req_insert_count	14
#define isc_info_req_update_count	15
#define isc_info_req_delete_count	16


/*********************/
/* Access path items */
/*********************/

#define isc_info_rsb_end		0
#define isc_info_rsb_begin		1
#define isc_info_rsb_type		2
#define isc_info_rsb_relation		3
#define isc_info_rsb_plan			4

/*************/
/* RecordSource (RSB) types */
/*************/

#define isc_info_rsb_unknown		1
#define isc_info_rsb_indexed		2
#define isc_info_rsb_navigate		3
#define isc_info_rsb_sequential	4
#define isc_info_rsb_cross		5
#define isc_info_rsb_sort		6
#define isc_info_rsb_first		7
#define isc_info_rsb_boolean		8
#define isc_info_rsb_union		9
#define isc_info_rsb_aggregate		10
#define isc_info_rsb_merge		11
#define isc_info_rsb_ext_sequential	12
#define isc_info_rsb_ext_indexed	13
#define isc_info_rsb_ext_dbkey		14
#define isc_info_rsb_left_cross	15
#define isc_info_rsb_select		16
#define isc_info_rsb_sql_join		17
#define isc_info_rsb_simulate		18
#define isc_info_rsb_sim_cross		19
#define isc_info_rsb_once		20
#define isc_info_rsb_procedure		21
#define isc_info_rsb_skip		22
#define isc_info_rsb_virt_sequential	23
#define isc_info_rsb_recursive	24

/**********************/
/* Bitmap expressions */
/**********************/

#define isc_info_rsb_and		1
#define isc_info_rsb_or		2
#define isc_info_rsb_dbkey		3
#define isc_info_rsb_index		4

#define isc_info_req_active		2
#define isc_info_req_inactive		3
#define isc_info_req_send		4
#define isc_info_req_receive		5
#define isc_info_req_select		6
#define isc_info_req_sql_stall		7

/**************************/
/* Blob information items */
/**************************/

#define isc_info_blob_num_segments	4
#define isc_info_blob_max_segment	5
#define isc_info_blob_total_length	6
#define isc_info_blob_type		7

/*********************************/
/* Transaction information items */
/*********************************/

#define isc_info_tra_id						4
#define isc_info_tra_oldest_interesting		5
#define isc_info_tra_oldest_snapshot		6
#define isc_info_tra_oldest_active			7
#define isc_info_tra_isolation				8
#define isc_info_tra_access					9
#define isc_info_tra_lock_timeout			10

// isc_info_tra_isolation responses
#define isc_info_tra_consistency		1
#define isc_info_tra_concurrency		2
#define isc_info_tra_read_committed		3

// isc_info_tra_read_committed options
#define isc_info_tra_no_rec_version		0
#define isc_info_tra_rec_version		1

// isc_info_tra_access responses
#define isc_info_tra_readonly	0
#define isc_info_tra_readwrite	1


/*************************/
/* SQL information items */
/*************************/

#define isc_info_sql_select		4
#define isc_info_sql_bind		5
#define isc_info_sql_num_variables	6
#define isc_info_sql_describe_vars	7
#define isc_info_sql_describe_end	8
#define isc_info_sql_sqlda_seq		9
#define isc_info_sql_message_seq	10
#define isc_info_sql_type		11
#define isc_info_sql_sub_type		12
#define isc_info_sql_scale		13
#define isc_info_sql_length		14
#define isc_info_sql_null_ind		15
#define isc_info_sql_field		16
#define isc_info_sql_relation		17
#define isc_info_sql_owner		18
#define isc_info_sql_alias		19
#define isc_info_sql_sqlda_start	20
#define isc_info_sql_stmt_type		21
#define isc_info_sql_get_plan             22
#define isc_info_sql_records		  23
#define isc_info_sql_batch_fetch	  24
#define isc_info_sql_relation_alias		25

/*********************************/
/* SQL information return values */
/*********************************/

#define isc_info_sql_stmt_select          1
#define isc_info_sql_stmt_insert          2
#define isc_info_sql_stmt_update          3
#define isc_info_sql_stmt_delete          4
#define isc_info_sql_stmt_ddl             5
#define isc_info_sql_stmt_get_segment     6
#define isc_info_sql_stmt_put_segment     7
#define isc_info_sql_stmt_exec_procedure  8
#define isc_info_sql_stmt_start_trans     9
#define isc_info_sql_stmt_commit          10
#define isc_info_sql_stmt_rollback        11
#define isc_info_sql_stmt_select_for_upd  12
#define isc_info_sql_stmt_set_generator   13
#define isc_info_sql_stmt_savepoint       14

#endif /* JRD_INF_PUB_H */

