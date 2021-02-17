/*
 *	MODULE:		consts_pub.h
 *	DESCRIPTION:	Public constants' definitions
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
 * 18.08.2006 Dimitry Sibiryakov: Extracted it from ibase.h
 *
 */

#ifndef INCLUDE_CONSTS_PUB_H
#define INCLUDE_CONSTS_PUB_H

/**********************************/
/* Database parameter block stuff */
/**********************************/

#define isc_dpb_version1                  1
#define isc_dpb_version2                  2

#define isc_dpb_cdd_pathname              1
#define isc_dpb_allocation                2
#define isc_dpb_journal                   3
#define isc_dpb_page_size                 4
#define isc_dpb_num_buffers               5
#define isc_dpb_buffer_length             6
#define isc_dpb_debug                     7
#define isc_dpb_garbage_collect           8
#define isc_dpb_verify                    9
#define isc_dpb_sweep                     10
#define isc_dpb_enable_journal            11
#define isc_dpb_disable_journal           12
#define isc_dpb_dbkey_scope               13
#define isc_dpb_number_of_users           14
#define isc_dpb_trace                     15
#define isc_dpb_no_garbage_collect        16
#define isc_dpb_damaged                   17
#define isc_dpb_license                   18
#define isc_dpb_sys_user_name             19
#define isc_dpb_encrypt_key               20
#define isc_dpb_activate_shadow           21
#define isc_dpb_sweep_interval            22
#define isc_dpb_delete_shadow             23
#define isc_dpb_force_write               24
#define isc_dpb_begin_log                 25
#define isc_dpb_quit_log                  26
#define isc_dpb_no_reserve                27
#define isc_dpb_user_name                 28
#define isc_dpb_password                  29
#define isc_dpb_password_enc              30
#define isc_dpb_sys_user_name_enc         31
#define isc_dpb_interp                    32
#define isc_dpb_online_dump               33
#define isc_dpb_old_file_size             34
#define isc_dpb_old_num_files             35
#define isc_dpb_old_file                  36
#define isc_dpb_old_start_page            37
#define isc_dpb_old_start_seqno           38
#define isc_dpb_old_start_file            39
#define isc_dpb_drop_walfile              40
#define isc_dpb_old_dump_id               41
#define isc_dpb_wal_backup_dir            42
#define isc_dpb_wal_chkptlen              43
#define isc_dpb_wal_numbufs               44
#define isc_dpb_wal_bufsize               45
#define isc_dpb_wal_grp_cmt_wait          46
#define isc_dpb_lc_messages               47
#define isc_dpb_lc_ctype                  48
#define isc_dpb_cache_manager             49
#define isc_dpb_shutdown                  50
#define isc_dpb_online                    51
#define isc_dpb_shutdown_delay            52
#define isc_dpb_reserved                  53
#define isc_dpb_overwrite                 54
#define isc_dpb_sec_attach                55
#define isc_dpb_disable_wal               56
#define isc_dpb_connect_timeout           57
#define isc_dpb_dummy_packet_interval     58
#define isc_dpb_gbak_attach               59
#define isc_dpb_sql_role_name             60
#define isc_dpb_set_page_buffers          61
#define isc_dpb_working_directory         62
#define isc_dpb_sql_dialect               63
#define isc_dpb_set_db_readonly           64
#define isc_dpb_set_db_sql_dialect        65
#define isc_dpb_gfix_attach               66
#define isc_dpb_gstat_attach              67
#define isc_dpb_set_db_charset            68
#define isc_dpb_gsec_attach               69		/* deprecated */
#define isc_dpb_address_path              70
#define isc_dpb_process_id                71
#define isc_dpb_no_db_triggers            72
#define isc_dpb_trusted_auth			  73
#define isc_dpb_process_name              74
#define isc_dpb_trusted_role			  75
#define isc_dpb_org_filename			  76
#define isc_dpb_utf8_filename			  77
#define isc_dpb_ext_call_depth			  78
#define isc_dpb_auth_block				  79
#define isc_dpb_client_version			  80
#define isc_dpb_remote_protocol			  81
#define isc_dpb_host_name				  82
#define isc_dpb_os_user					  83
#define isc_dpb_specific_auth_data		  84
#define isc_dpb_auth_plugin_list		  85
#define isc_dpb_auth_plugin_name		  86
#define isc_dpb_config					  87
#define isc_dpb_nolinger				  88

/**************************************************/
/* clumplet tags used inside isc_dpb_address_path */
/*						 and isc_spb_address_path */
/**************************************************/

/* Format of this clumplet is the following:

 <address-path-clumplet> ::=
	isc_dpb_address_path <byte-clumplet-length> <address-stack>

 <address-stack> ::=
	<address-descriptor> |
	<address-stack> <address-descriptor>

 <address-descriptor> ::=
	isc_dpb_address <byte-clumplet-length> <address-elements>

 <address-elements> ::=
	<address-element> |
	<address-elements> <address-element>

 <address-element> ::=
	isc_dpb_addr_protocol <byte-clumplet-length> <protocol-string> |
	isc_dpb_addr_endpoint <byte-clumplet-length> <remote-endpoint-string>

 <protocol-string> ::=
	"TCPv4" |
	"TCPv6" |
	"XNET" |
	"WNET" |
	....

 <remote-endpoint-string> ::=
	<IPv4-address> | // such as "172.20.1.1"
	<IPv6-address> | // such as "2001:0:13FF:09FF::1"
	<xnet-process-id> | // such as "17864"
	...
*/

#define isc_dpb_address 1

#define isc_dpb_addr_protocol 1
#define isc_dpb_addr_endpoint 2

/*********************************/
/* isc_dpb_verify specific flags */
/*********************************/

#define isc_dpb_pages                     1
#define isc_dpb_records                   2
#define isc_dpb_indices                   4
#define isc_dpb_transactions              8
#define isc_dpb_no_update                 16
#define isc_dpb_repair                    32
#define isc_dpb_ignore                    64

/***********************************/
/* isc_dpb_shutdown specific flags */
/***********************************/

#define isc_dpb_shut_cache               0x1
#define isc_dpb_shut_attachment          0x2
#define isc_dpb_shut_transaction         0x4
#define isc_dpb_shut_force               0x8
#define isc_dpb_shut_mode_mask          0x70

#define isc_dpb_shut_default             0x0
#define isc_dpb_shut_normal             0x10
#define isc_dpb_shut_multi              0x20
#define isc_dpb_shut_single             0x30
#define isc_dpb_shut_full               0x40

/**************************************/
/* Bit assignments in RDB$SYSTEM_FLAG */
/**************************************/

#define RDB_system                         1
#define RDB_id_assigned                    2
/* 2 is for QLI. See jrd/constants.h for more Firebird-specific values. */


/*************************************/
/* Transaction parameter block stuff */
/*************************************/

#define isc_tpb_version1                  1
#define isc_tpb_version3                  3
#define isc_tpb_consistency               1
#define isc_tpb_concurrency               2
#define isc_tpb_shared                    3
#define isc_tpb_protected                 4
#define isc_tpb_exclusive                 5
#define isc_tpb_wait                      6
#define isc_tpb_nowait                    7
#define isc_tpb_read                      8
#define isc_tpb_write                     9
#define isc_tpb_lock_read                 10
#define isc_tpb_lock_write                11
#define isc_tpb_verb_time                 12
#define isc_tpb_commit_time               13
#define isc_tpb_ignore_limbo              14
#define isc_tpb_read_committed	          15
#define isc_tpb_autocommit                16
#define isc_tpb_rec_version               17
#define isc_tpb_no_rec_version            18
#define isc_tpb_restart_requests          19
#define isc_tpb_no_auto_undo              20
#define isc_tpb_lock_timeout              21


/************************/
/* Blob Parameter Block */
/************************/

#define isc_bpb_version1                  1
#define isc_bpb_source_type               1
#define isc_bpb_target_type               2
#define isc_bpb_type                      3
#define isc_bpb_source_interp             4
#define isc_bpb_target_interp             5
#define isc_bpb_filter_parameter          6
#define isc_bpb_storage                   7

#define isc_bpb_type_segmented            0x0
#define isc_bpb_type_stream               0x1
#define isc_bpb_storage_main              0x0
#define isc_bpb_storage_temp              0x2


/*********************************/
/* Service parameter block stuff */
/*********************************/

#define isc_spb_version1                  1
#define isc_spb_current_version           2
#define isc_spb_version                   isc_spb_current_version
#define isc_spb_version3                  3
#define isc_spb_user_name                 isc_dpb_user_name
#define isc_spb_sys_user_name             isc_dpb_sys_user_name
#define isc_spb_sys_user_name_enc         isc_dpb_sys_user_name_enc
#define isc_spb_password                  isc_dpb_password
#define isc_spb_password_enc              isc_dpb_password_enc
#define isc_spb_command_line              105
#define isc_spb_dbname                    106
#define isc_spb_verbose                   107
#define isc_spb_options                   108
#define isc_spb_address_path              109
#define isc_spb_process_id                110
#define isc_spb_trusted_auth			  111
#define isc_spb_process_name              112
#define isc_spb_trusted_role              113
#define isc_spb_verbint                   114
#define isc_spb_auth_block                115
#define isc_spb_auth_plugin_name          116
#define isc_spb_auth_plugin_list          117
#define isc_spb_utf8_filename			  118
#define isc_spb_client_version            119
#define isc_spb_remote_protocol           120
#define isc_spb_host_name                 121
#define isc_spb_os_user                   122
#define isc_spb_config					  123
#define isc_spb_expected_db				  124

#define isc_spb_connect_timeout           isc_dpb_connect_timeout
#define isc_spb_dummy_packet_interval     isc_dpb_dummy_packet_interval
#define isc_spb_sql_role_name             isc_dpb_sql_role_name

// This will not be used in protocol 13, therefore may be reused
#define isc_spb_specific_auth_data		  isc_spb_trusted_auth

/*****************************
 * Service action items      *
 *****************************/

#define isc_action_svc_backup          1	/* Starts database backup process on the server */
#define isc_action_svc_restore         2	/* Starts database restore process on the server */
#define isc_action_svc_repair          3	/* Starts database repair process on the server */
#define isc_action_svc_add_user        4	/* Adds a new user to the security database */
#define isc_action_svc_delete_user     5	/* Deletes a user record from the security database */
#define isc_action_svc_modify_user     6	/* Modifies a user record in the security database */
#define isc_action_svc_display_user    7	/* Displays a user record from the security database */
#define isc_action_svc_properties      8	/* Sets database properties */
#define isc_action_svc_add_license     9	/* Adds a license to the license file */
#define isc_action_svc_remove_license 10	/* Removes a license from the license file */
#define isc_action_svc_db_stats	      11	/* Retrieves database statistics */
#define isc_action_svc_get_ib_log     12	/* Retrieves the InterBase log file from the server */
#define isc_action_svc_get_fb_log     12	/* Retrieves the Firebird log file from the server */
#define isc_action_svc_nbak           20	/* Incremental nbackup */
#define isc_action_svc_nrest          21	/* Incremental database restore */
#define isc_action_svc_trace_start    22	// Start trace session
#define isc_action_svc_trace_stop     23	// Stop trace session
#define isc_action_svc_trace_suspend  24	// Suspend trace session
#define isc_action_svc_trace_resume   25	// Resume trace session
#define isc_action_svc_trace_list     26	// List existing sessions
#define isc_action_svc_set_mapping    27	// Set auto admins mapping in security database
#define isc_action_svc_drop_mapping   28	// Drop auto admins mapping in security database
#define isc_action_svc_display_user_adm 29	// Displays user(s) from security database with admin info
#define isc_action_svc_last			  30	// keep it last !

/*****************************
 * Service information items *
 *****************************/

#define isc_info_svc_svr_db_info		50	/* Retrieves the number of attachments and databases */
#define isc_info_svc_get_license		51	/* Retrieves all license keys and IDs from the license file */
#define isc_info_svc_get_license_mask	52	/* Retrieves a bitmask representing licensed options on the server */
#define isc_info_svc_get_config			53	/* Retrieves the parameters and values for IB_CONFIG */
#define isc_info_svc_version			54	/* Retrieves the version of the services manager */
#define isc_info_svc_server_version		55	/* Retrieves the version of the Firebird server */
#define isc_info_svc_implementation		56	/* Retrieves the implementation of the Firebird server */
#define isc_info_svc_capabilities		57	/* Retrieves a bitmask representing the server's capabilities */
#define isc_info_svc_user_dbpath		58	/* Retrieves the path to the security database in use by the server */
#define isc_info_svc_get_env			59	/* Retrieves the setting of $FIREBIRD */
#define isc_info_svc_get_env_lock		60	/* Retrieves the setting of $FIREBIRD_LCK */
#define isc_info_svc_get_env_msg		61	/* Retrieves the setting of $FIREBIRD_MSG */
#define isc_info_svc_line				62	/* Retrieves 1 line of service output per call */
#define isc_info_svc_to_eof				63	/* Retrieves as much of the server output as will fit in the supplied buffer */
#define isc_info_svc_timeout			64	/* Sets / signifies a timeout value for reading service information */
#define isc_info_svc_get_licensed_users	65	/* Retrieves the number of users licensed for accessing the server */
#define isc_info_svc_limbo_trans		66	/* Retrieve the limbo transactions */
#define isc_info_svc_running			67	/* Checks to see if a service is running on an attachment */
#define isc_info_svc_get_users			68	/* Returns the user information from isc_action_svc_display_users */
#define isc_info_svc_auth_block			69	/* Sets authentication block for service query() call */
#define isc_info_svc_stdin				78	/* Returns maximum size of data, needed as stdin for service */


/******************************************************
 * Parameters for isc_action_{add|del|mod|disp)_user  *
 ******************************************************/

#define isc_spb_sec_userid            5
#define isc_spb_sec_groupid           6
#define isc_spb_sec_username          7
#define isc_spb_sec_password          8
#define isc_spb_sec_groupname         9
#define isc_spb_sec_firstname         10
#define isc_spb_sec_middlename        11
#define isc_spb_sec_lastname          12
#define isc_spb_sec_admin             13

/*******************************************************
 * Parameters for isc_action_svc_(add|remove)_license, *
 * isc_info_svc_get_license                            *
 *******************************************************/

#define isc_spb_lic_key               5
#define isc_spb_lic_id                6
#define isc_spb_lic_desc              7


/*****************************************
 * Parameters for isc_action_svc_backup  *
 *****************************************/

#define isc_spb_bkp_file                 5
#define isc_spb_bkp_factor               6
#define isc_spb_bkp_length               7
#define isc_spb_bkp_ignore_checksums     0x01
#define isc_spb_bkp_ignore_limbo         0x02
#define isc_spb_bkp_metadata_only        0x04
#define isc_spb_bkp_no_garbage_collect   0x08
#define isc_spb_bkp_old_descriptions     0x10
#define isc_spb_bkp_non_transportable    0x20
#define isc_spb_bkp_convert              0x40
#define isc_spb_bkp_expand				 0x80
#define isc_spb_bkp_no_triggers			 0x8000

/********************************************
 * Parameters for isc_action_svc_properties *
 ********************************************/

#define isc_spb_prp_page_buffers		5
#define isc_spb_prp_sweep_interval		6
#define isc_spb_prp_shutdown_db			7
#define isc_spb_prp_deny_new_attachments	9
#define isc_spb_prp_deny_new_transactions	10
#define isc_spb_prp_reserve_space		11
#define isc_spb_prp_write_mode			12
#define isc_spb_prp_access_mode			13
#define isc_spb_prp_set_sql_dialect		14
#define isc_spb_prp_activate			0x0100
#define isc_spb_prp_db_online			0x0200
#define isc_spb_prp_nolinger			0x0400
#define isc_spb_prp_force_shutdown			41
#define isc_spb_prp_attachments_shutdown	42
#define isc_spb_prp_transactions_shutdown	43
#define isc_spb_prp_shutdown_mode		44
#define isc_spb_prp_online_mode			45

/********************************************
 * Parameters for isc_spb_prp_shutdown_mode *
 *            and isc_spb_prp_online_mode   *
 ********************************************/
#define isc_spb_prp_sm_normal		0
#define isc_spb_prp_sm_multi		1
#define isc_spb_prp_sm_single		2
#define isc_spb_prp_sm_full			3

/********************************************
 * Parameters for isc_spb_prp_reserve_space *
 ********************************************/

#define isc_spb_prp_res_use_full	35
#define isc_spb_prp_res				36

/******************************************
 * Parameters for isc_spb_prp_write_mode  *
 ******************************************/

#define isc_spb_prp_wm_async		37
#define isc_spb_prp_wm_sync			38

/******************************************
 * Parameters for isc_spb_prp_access_mode *
 ******************************************/

#define isc_spb_prp_am_readonly		39
#define isc_spb_prp_am_readwrite	40

/*****************************************
 * Parameters for isc_action_svc_repair  *
 *****************************************/

#define isc_spb_rpr_commit_trans		15
#define isc_spb_rpr_rollback_trans		34
#define isc_spb_rpr_recover_two_phase	17
#define isc_spb_tra_id					18
#define isc_spb_single_tra_id			19
#define isc_spb_multi_tra_id			20
#define isc_spb_tra_state				21
#define isc_spb_tra_state_limbo			22
#define isc_spb_tra_state_commit		23
#define isc_spb_tra_state_rollback		24
#define isc_spb_tra_state_unknown		25
#define isc_spb_tra_host_site			26
#define isc_spb_tra_remote_site			27
#define isc_spb_tra_db_path				28
#define isc_spb_tra_advise				29
#define isc_spb_tra_advise_commit		30
#define isc_spb_tra_advise_rollback		31
#define isc_spb_tra_advise_unknown		33

#define isc_spb_rpr_validate_db			0x01
#define isc_spb_rpr_sweep_db			0x02
#define isc_spb_rpr_mend_db				0x04
#define isc_spb_rpr_list_limbo_trans	0x08
#define isc_spb_rpr_check_db			0x10
#define isc_spb_rpr_ignore_checksum		0x20
#define isc_spb_rpr_kill_shadows		0x40
#define isc_spb_rpr_full				0x80

/*****************************************
 * Parameters for isc_action_svc_restore *
 *****************************************/

#define isc_spb_res_buffers				9
#define isc_spb_res_page_size			10
#define isc_spb_res_length				11
#define isc_spb_res_access_mode			12
#define isc_spb_res_fix_fss_data		13
#define isc_spb_res_fix_fss_metadata	14
#define isc_spb_res_metadata_only		isc_spb_bkp_metadata_only
#define isc_spb_res_deactivate_idx		0x0100
#define isc_spb_res_no_shadow			0x0200
#define isc_spb_res_no_validity			0x0400
#define isc_spb_res_one_at_a_time		0x0800
#define isc_spb_res_replace				0x1000
#define isc_spb_res_create				0x2000
#define isc_spb_res_use_all_space		0x4000

/******************************************
 * Parameters for isc_spb_res_access_mode  *
 ******************************************/

#define isc_spb_res_am_readonly			isc_spb_prp_am_readonly
#define isc_spb_res_am_readwrite		isc_spb_prp_am_readwrite

/*******************************************
 * Parameters for isc_info_svc_svr_db_info *
 *******************************************/

#define isc_spb_num_att			5
#define isc_spb_num_db			6

/*****************************************
 * Parameters for isc_info_svc_db_stats  *
 *****************************************/

#define isc_spb_sts_table			64

#define isc_spb_sts_data_pages		0x01
#define isc_spb_sts_db_log			0x02
#define isc_spb_sts_hdr_pages		0x04
#define isc_spb_sts_idx_pages		0x08
#define isc_spb_sts_sys_relations	0x10
#define isc_spb_sts_record_versions	0x20
//#define isc_spb_sts_table			0x40
#define isc_spb_sts_nocreation		0x80
#define isc_spb_sts_encryption	   0x100

/***********************************/
/* Server configuration key values */
/***********************************/

/* Not available in Firebird 1.5 */

/***************************************
 * Parameters for isc_action_svc_nbak  *
 ***************************************/

#define isc_spb_nbk_level			5
#define isc_spb_nbk_file			6
#define isc_spb_nbk_direct			7
#define isc_spb_nbk_no_triggers		0x01

/***************************************
 * Parameters for isc_action_svc_trace *
 ***************************************/

#define isc_spb_trc_id				1
#define isc_spb_trc_name			2
#define isc_spb_trc_cfg				3

/******************************************/
/* Array slice description language (SDL) */
/******************************************/

#define isc_sdl_version1                  1
#define isc_sdl_eoc                       255
#define isc_sdl_relation                  2
#define isc_sdl_rid                       3
#define isc_sdl_field                     4
#define isc_sdl_fid                       5
#define isc_sdl_struct                    6
#define isc_sdl_variable                  7
#define isc_sdl_scalar                    8
#define isc_sdl_tiny_integer              9
#define isc_sdl_short_integer             10
#define isc_sdl_long_integer              11
//#define isc_sdl_literal                   12
#define isc_sdl_add                       13
#define isc_sdl_subtract                  14
#define isc_sdl_multiply                  15
#define isc_sdl_divide                    16
#define isc_sdl_negate                    17 // only used in pretty.cpp; nobody generates it
//#define isc_sdl_eql                       18
//#define isc_sdl_neq                       19
//#define isc_sdl_gtr                       20
//#define isc_sdl_geq                       21
//#define isc_sdl_lss                       22
//#define isc_sdl_leq                       23
//#define isc_sdl_and                       24
//#define isc_sdl_or                        25
//#define isc_sdl_not                       26
//#define isc_sdl_while                     27
//#define isc_sdl_assignment                28
//#define isc_sdl_label                     29
//#define isc_sdl_leave                     30
#define isc_sdl_begin                     31 // only used in pretty.cpp; nobody generates it
#define isc_sdl_end                       32
#define isc_sdl_do3                       33
#define isc_sdl_do2                       34
#define isc_sdl_do1                       35
#define isc_sdl_element                   36

/********************************************/
/* International text interpretation values */
/********************************************/

//#define isc_interp_eng_ascii              0
//#define isc_interp_jpn_sjis               5
//#define isc_interp_jpn_euc                6

/*****************/
/* Blob Subtypes */
/*****************/

/* types less than zero are reserved for customer use */

#define isc_blob_untyped                  0

/* internal subtypes */

#define isc_blob_text                     1
#define isc_blob_blr                      2
#define isc_blob_acl                      3
#define isc_blob_ranges                   4
#define isc_blob_summary                  5
#define isc_blob_format                   6
#define isc_blob_tra                      7
#define isc_blob_extfile                  8
#define isc_blob_debug_info               9
#define isc_blob_max_predefined_subtype   10

/* the range 20-30 is reserved for dBASE and Paradox types */

//#define isc_blob_formatted_memo           20
//#define isc_blob_paradox_ole              21
//#define isc_blob_graphic                  22
//#define isc_blob_dbase_ole                23
//#define isc_blob_typed_binary             24

/* Deprecated definitions maintained for compatibility only */

//#define isc_info_db_SQL_dialect           62
//#define isc_dpb_SQL_dialect               63
//#define isc_dpb_set_db_SQL_dialect        65

/***********************************/
/* Masks for fb_shutdown_callback  */
/***********************************/

#define fb_shut_confirmation			  1
#define fb_shut_preproviders			  2
#define fb_shut_postproviders			  4
#define fb_shut_finish					  8
#define fb_shut_exit					 16

/****************************************/
/* Shutdown reasons, used by engine     */
/* Users should provide positive values */
/****************************************/

#define fb_shutrsn_svc_stopped			  -1
#define fb_shutrsn_no_connection		  -2
#define fb_shutrsn_app_stopped			  -3
//#define fb_shutrsn_device_removed		  -4
#define fb_shutrsn_signal				  -5
#define fb_shutrsn_services				  -6
#define fb_shutrsn_exit_called			  -7

/****************************************/
/* Cancel types for fb_cancel_operation */
/****************************************/

#define fb_cancel_disable				  1
#define fb_cancel_enable				  2
#define fb_cancel_raise					  3
#define fb_cancel_abort					  4

/********************************************/
/* Debug information items					*/
/********************************************/

#define fb_dbg_version				1
#define fb_dbg_end					255
#define fb_dbg_map_src2blr			2
#define fb_dbg_map_varname			3
#define fb_dbg_map_argument			4
#define fb_dbg_subproc				5
#define fb_dbg_subfunc				6

// sub code for fb_dbg_map_argument
#define fb_dbg_arg_input			0
#define fb_dbg_arg_output			1

#endif // ifndef INCLUDE_CONSTS_PUB_H
