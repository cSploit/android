/*
 *	PROGRAM:	C preprocessor
 *	MODULE:		gds.h
 *	DESCRIPTION:	BLR constants
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete ports:
 *                          - EPSON, XENIX, MAC (MAC_AUX), Cray and OS/2
 *
 */


#ifndef JRD_GDSOLD_H
#define JRD_GDSOLD_H

#include "fb_types.h"

#define GDS_EXPORT ISC_EXPORT
#define GDS_EXPORT_VARARG ISC_EXPORT_VARARG

#define CANCEL_disable	1
#define CANCEL_enable	2
#define CANCEL_raise	3

/******************************************************************/
/* define type, export and other stuff based on c/c++ and Windows */
/******************************************************************/

typedef void  *gds_db_handle;
typedef void  *gds_req_handle;
typedef void  *gds_tr_handle;
typedef void  *gds_blob_handle;
typedef void  *gds_stmt_handle;
typedef void  *gds_svc_handle;
typedef void ( * gds_callback) ();


/*************************/
/* Old OSRI entrypoints  */
/*************************/

#ifndef NO_OSRI_ENTRYPOINTS

/* #ifndef __cplusplus */

#ifdef __cplusplus
extern "C" {
typedef GDS_QUAD GDS__QUAD;
#endif

#ifndef FRBRD
#define FRBRD void
#endif

ISC_STATUS GDS_EXPORT gds__attach_database(ISC_STATUS*,
										   short,
										   const char*,
										   FRBRD**,
										   short,
										   const char*);

ISC_STATUS GDS_EXPORT gds__blob_info(ISC_STATUS*,
									 FRBRD**,
									 short,
									 const char*,
									 short,
									 char*);

ISC_STATUS GDS_EXPORT gds__cancel_blob(ISC_STATUS  *,
									   FRBRD  **);

ISC_STATUS GDS_EXPORT gds__close_blob(ISC_STATUS  *,
									  FRBRD  **);

ISC_STATUS GDS_EXPORT gds__commit_transaction(ISC_STATUS  *,
											  FRBRD  **);

ISC_STATUS GDS_EXPORT gds__compile_request(ISC_STATUS*,
										   FRBRD**,
										   FRBRD**,
										   short,
										   const char*);

ISC_STATUS GDS_EXPORT gds__compile_request2(ISC_STATUS*,
											FRBRD**,
											FRBRD**,
											short,
											const char*);

ISC_STATUS GDS_EXPORT gds__create_blob(ISC_STATUS*,
									   FRBRD**,
									   FRBRD**,
									   FRBRD**,
									   GDS__QUAD*);

ISC_STATUS GDS_EXPORT gds__create_blob2(ISC_STATUS*,
										FRBRD**,
										FRBRD**,
										FRBRD**,
										GDS__QUAD*,
										short,
										const char*);

ISC_STATUS GDS_EXPORT gds__create_database(ISC_STATUS*,
										   short,
										   const char*,
										   FRBRD**,
										   short,
										   const char*,
										   short);

ISC_STATUS GDS_EXPORT gds__database_info(ISC_STATUS*,
										 FRBRD**,
										 short,
										 const char*,
										 short,
										 char*);

void GDS_EXPORT gds__decode_date(const GDS__QUAD*,
								 void*);

ISC_STATUS GDS_EXPORT gds__detach_database(ISC_STATUS  *,
										   FRBRD  **);

ULONG GDS_EXPORT gds__free(void  *);

ISC_STATUS GDS_EXPORT gds__get_segment(ISC_STATUS  *,
									   FRBRD  **,
									   unsigned short  *,
									   unsigned short,
									   char  *);

ISC_STATUS GDS_EXPORT gds__open_blob(ISC_STATUS*,
									 FRBRD**,
									 FRBRD**,
									 FRBRD**,
									 GDS__QUAD*);

ISC_STATUS GDS_EXPORT gds__open_blob2(ISC_STATUS*,
									  FRBRD**,
									  FRBRD**,
									  FRBRD**,
									  GDS__QUAD*,
									  short,
									  const char*);

ISC_STATUS GDS_EXPORT gds__prepare_transaction(ISC_STATUS  *,
											   FRBRD  **);

ISC_STATUS GDS_EXPORT gds__prepare_transaction2(ISC_STATUS  *,
												FRBRD  **,
												short,
												char  *);

ISC_STATUS GDS_EXPORT gds__put_segment(ISC_STATUS*,
									   FRBRD**,
									   unsigned short,
									   const char*);

ISC_STATUS GDS_EXPORT gds__receive(ISC_STATUS  *,
								   FRBRD  **,
								   short,
								   short,
								   void  *,
								   short);

ISC_STATUS GDS_EXPORT gds__reconnect_transaction(ISC_STATUS*,
												 FRBRD**,
												 FRBRD**,
												 short,
												 const char*);

ISC_STATUS GDS_EXPORT gds__request_info(ISC_STATUS*,
										FRBRD**,
										short,
										short,
										const char*,
										short,
										char*);

ISC_STATUS GDS_EXPORT gds__release_request(ISC_STATUS  *,
										   FRBRD  **);

ISC_STATUS GDS_EXPORT gds__rollback_transaction(ISC_STATUS  *,
												FRBRD  **);

ISC_STATUS GDS_EXPORT gds__seek_blob(ISC_STATUS  *,
									 FRBRD  **,
									 short,
									 SLONG,
									 SLONG  *);

ISC_STATUS GDS_EXPORT gds__send(ISC_STATUS  *,
								FRBRD  **,
								short,
								short,
								void  *,
								short);

void GDS_EXPORT gds__set_debug(int);

ISC_STATUS GDS_EXPORT gds__start_and_send(ISC_STATUS  *,
										  FRBRD  **,
										  FRBRD  **,
										  short,
										  short,
										  void  *,
										  short);

ISC_STATUS GDS_EXPORT gds__start_multiple(ISC_STATUS  *,
										  FRBRD  **,
										  short,
										  void  *);

ISC_STATUS GDS_EXPORT gds__start_request(ISC_STATUS  *,
										 FRBRD  **,
										 FRBRD  **,
										 short);

ISC_STATUS GDS_EXPORT gds__start_transaction(ISC_STATUS  *,
											 FRBRD  **,
											 short, ...);

ISC_STATUS GDS_EXPORT gds__transaction_info(ISC_STATUS*,
											FRBRD**,
											short,
											const char*,
											short,
											char*);

ISC_STATUS GDS_EXPORT gds__unwind_request(ISC_STATUS  *,
										  FRBRD  **,
										  short);

SLONG GDS_EXPORT gds__ftof(const char*,
							  const unsigned short,
							  char*,
							  const unsigned short);

void GDS_EXPORT gds__vtof(const char*,
						  char*,
						  unsigned short);
						  
void GDS_EXPORT gds__vtov(const SCHAR*, char*, SSHORT);

int GDS_EXPORT gds__version(FRBRD**,
							FPTR_VERSION_CALLBACK,
							void*);

int GDS_EXPORT gds__disable_subsystem(char  *);

int GDS_EXPORT gds__enable_subsystem(char  *);

ISC_STATUS GDS_EXPORT gds__print_status(const ISC_STATUS*);

SLONG GDS_EXPORT gds__sqlcode(const ISC_STATUS*);

ISC_STATUS GDS_EXPORT gds__ddl(ISC_STATUS*,
							   FRBRD**,
							   FRBRD**,
							   short,
							   const char*);

ISC_STATUS GDS_EXPORT gds__commit_retaining(ISC_STATUS  *,
											FRBRD  **);

void GDS_EXPORT gds__encode_date(const void*,
								 GDS__QUAD*);

ISC_STATUS GDS_EXPORT gds__que_events(ISC_STATUS*,
									  FRBRD**,
									  SLONG*,
									  short,
									  const char*,
									  FPTR_EVENT_CALLBACK,
									  void*);

ISC_STATUS GDS_EXPORT gds__cancel_events(ISC_STATUS  *,
										 FRBRD  **,
										 SLONG  *);

ISC_STATUS GDS_EXPORT gds__event_wait(ISC_STATUS  *,
									  FRBRD  **,
									  short,
									  char  *,
									  char  *);

void GDS_EXPORT gds__event_counts(ULONG*,
								  short,
								  char*,
								  const char*);

SLONG GDS_EXPORT gds__event_block(char  **,
									 char  **,
									 unsigned short, ...);

ISC_STATUS GDS_EXPORT gds__get_slice(ISC_STATUS*,
									 FRBRD**,
									 FRBRD**,
									 GDS__QUAD*,
									 short,
									 const char*,
									 short,
									 const SLONG*,
									 SLONG,
									 void*,
									 SLONG*);

ISC_STATUS GDS_EXPORT gds__put_slice(ISC_STATUS*,
									 FRBRD**,
									 FRBRD**,
									 GDS__QUAD*,
									 short,
									 const char*,
									 short,
									 const SLONG*,
									 SLONG,
									 void*);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* #endif *//* __cplusplus */

#endif /* NO_OSRI_ENTRYPOINTS */




/**********************************/
/* Database parameter block stuff */
/**********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dpb_version1                  1
#define gds__dpb_cdd_pathname              1
#define gds__dpb_allocation                2
#define gds__dpb_journal                   3
#define gds__dpb_page_size                 4
#define gds__dpb_num_buffers               5
#define gds__dpb_buffer_length             6
#define gds__dpb_debug                     7
#define gds__dpb_garbage_collect           8
#define gds__dpb_verify                    9
#define gds__dpb_sweep                     10
#define gds__dpb_enable_journal            11
#define gds__dpb_disable_journal           12
#define gds__dpb_dbkey_scope               13
#define gds__dpb_number_of_users           14
#define gds__dpb_trace                     15
#define gds__dpb_no_garbage_collect        16
#define gds__dpb_damaged                   17
#define gds__dpb_license                   18
#define gds__dpb_sys_user_name             19
#define gds__dpb_encrypt_key               20
#define gds__dpb_activate_shadow           21
#define gds__dpb_sweep_interval            22
#define gds__dpb_delete_shadow             23
#define gds__dpb_force_write               24
#define gds__dpb_begin_log                 25
#define gds__dpb_quit_log                  26
#define gds__dpb_no_reserve                27
#define gds__dpb_user_name                 28
#define gds__dpb_password                  29
#define gds__dpb_password_enc              30
#define gds__dpb_sys_user_name_enc         31
#define gds__dpb_interp                    32
#define gds__dpb_online_dump               33
#define gds__dpb_old_file_size             34
#define gds__dpb_old_num_files             35
#define gds__dpb_old_file                  36
#define gds__dpb_old_start_page            37
#define gds__dpb_old_start_seqno           38
#define gds__dpb_old_start_file            39
#define gds__dpb_drop_walfile              40
#define gds__dpb_old_dump_id               41
#define gds__dpb_wal_backup_dir            42
#define gds__dpb_wal_chkptlen              43
#define gds__dpb_wal_numbufs               44
#define gds__dpb_wal_bufsize               45
#define gds__dpb_wal_grp_cmt_wait          46
#define gds__dpb_lc_messages               47
#define gds__dpb_lc_ctype                  48
#define gds__dpb_cache_manager		   49
#define gds__dpb_shutdown		   50
#define gds__dpb_online			   51
#define gds__dpb_shutdown_delay		   52
#define gds__dpb_reserved		   53
#define gds__dpb_overwrite		   54
#define gds__dpb_sec_attach		   55
#define gds__dpb_disable_wal		   56
#define gds__dpb_connect_timeout	   57
#define gds__dpb_dummy_packet_interval     58

#else /* c++ definitions */

const char gds_dpb_version1 = 1;
const char gds_dpb_cdd_pathname = 1;
const char gds_dpb_allocation = 2;
const char gds_dpb_journal = 3;
const char gds_dpb_page_size = 4;
const char gds_dpb_num_buffers = 5;
const char gds_dpb_buffer_length = 6;
const char gds_dpb_debug = 7;
const char gds_dpb_garbage_collect = 8;
const char gds_dpb_verify = 9;
const char gds_dpb_sweep = 10;
const char gds_dpb_enable_journal = 11;
const char gds_dpb_disable_journal = 12;
const char gds_dpb_dbkey_scope = 13;
const char gds_dpb_number_of_users = 14;
const char gds_dpb_trace = 15;
const char gds_dpb_no_garbage_collect = 16;
const char gds_dpb_damaged = 17;
const char gds_dpb_license = 18;
const char gds_dpb_sys_user_name = 19;
const char gds_dpb_encrypt_key = 20;
const char gds_dpb_activate_shadow = 21;
const char gds_dpb_sweep_interval = 22;
const char gds_dpb_delete_shadow = 23;
const char gds_dpb_force_write = 24;
const char gds_dpb_begin_log = 25;
const char gds_dpb_quit_log = 26;
const char gds_dpb_no_reserve = 27;
const char gds_dpb_user_name = 28;
const char gds_dpb_password = 29;
const char gds_dpb_password_enc = 30;
const char gds_dpb_sys_user_name_enc = 31;
const char gds_dpb_interp = 32;
const char gds_dpb_online_dump = 33;
const char gds_dpb_old_file_size = 34;
const char gds_dpb_old_num_files = 35;
const char gds_dpb_old_file = 36;
const char gds_dpb_old_start_page = 37;
const char gds_dpb_old_start_seqno = 38;
const char gds_dpb_old_start_file = 39;
const char gds_dpb_drop_walfile = 40;
const char gds_dpb_old_dump_id = 41;
const char gds_dpb_wal_backup_dir = 42;
const char gds_dpb_wal_chkptlen = 43;
const char gds_dpb_wal_numbufs = 44;
const char gds_dpb_wal_bufsize = 45;
const char gds_dpb_wal_grp_cmt_wait = 46;
const char gds_dpb_lc_messages = 47;
const char gds_dpb_lc_ctype = 48;
const char gds_dpb_cache_manager = 49;
const char gds_dpb_shutdown = 50;
const char gds_dpb_online = 51;
const char gds_dpb_shutdown_delay = 52;
const char gds_dpb_reserved = 53;
const char gds_dpb_overwrite = 54;
const char gds_dpb_sec_attach = 55;
const char gds_dpb_disable_wal = 56;
const char gds_dpb_connect_timeout = 57;
const char gds_dpb_dummy_packet_interval = 58;

#endif



/**********************************/
/* gds__dpb_verify specific flags */
/**********************************/


#ifndef	__cplusplus				/* c definitions */

#define gds__dpb_pages                     1
#define gds__dpb_records                   2
#define gds__dpb_indices                   4
#define gds__dpb_transactions              8
#define gds__dpb_no_update                 16
#define gds__dpb_repair                    32
#define gds__dpb_ignore                    64

#else /* c++ definitions */

const char gds_dpb_pages = 1;
const char gds_dpb_records = 2;
const char gds_dpb_indices = 4;
const char gds_dpb_transactions = 8;
const char gds_dpb_no_update = 16;
const char gds_dpb_repair = 32;
const char gds_dpb_ignore = 64;

#endif

/************************************/
/* gds__dpb_shutdown specific flags */
/************************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dpb_shut_cache               1
#define gds__dpb_shut_attachment          2
#define gds__dpb_shut_transaction         4
#define gds__dpb_shut_force               8

#else /* c++ definitions */

const char gds_dpb_shut_cache = 1;
const char gds_dpb_shut_attachment = 2;
const char gds_dpb_shut_transaction = 4;
const char gds_dpb_shut_force = 8;

#endif


/*************************************/
/* Transaction parameter block stuff */
/*************************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__tpb_version1                  1
#define gds__tpb_version3                  3
#define gds__tpb_consistency               1
#define gds__tpb_concurrency               2
#define gds__tpb_shared                    3
#define gds__tpb_protected                 4
#define gds__tpb_exclusive                 5
#define gds__tpb_wait                      6
#define gds__tpb_nowait                    7
#define gds__tpb_read                      8
#define gds__tpb_write                     9
#define gds__tpb_lock_read                 10
#define gds__tpb_lock_write                11
#define gds__tpb_verb_time                 12
#define gds__tpb_commit_time               13
#define gds__tpb_ignore_limbo              14
#define gds__tpb_read_committed		   15
#define gds__tpb_autocommit		   16
#define gds__tpb_rec_version		   17
#define gds__tpb_no_rec_version		   18
#define gds__tpb_restart_requests	   19
#define gds__tpb_no_auto_undo              20

#else /* c++ definitions */

const char gds_tpb_version1 = 1;
const char gds_tpb_version3 = 3;
const char gds_tpb_consistency = 1;
const char gds_tpb_concurrency = 2;
const char gds_tpb_shared = 3;
const char gds_tpb_protected = 4;
const char gds_tpb_exclusive = 5;
const char gds_tpb_wait = 6;
const char gds_tpb_nowait = 7;
const char gds_tpb_read = 8;
const char gds_tpb_write = 9;
const char gds_tpb_lock_read = 10;
const char gds_tpb_lock_write = 11;
const char gds_tpb_verb_time = 12;
const char gds_tpb_commit_time = 13;
const char gds_tpb_ignore_limbo = 14;
const char gds_tpb_read_committed = 15;
const char gds_tpb_autocommit = 16;
const char gds_tpb_rec_version = 17;
const char gds_tpb_no_rec_version = 18;
const char gds_tpb_restart_requests = 19;
const char gds_tpb_no_auto_undo = 20;

#endif




/************************/
/* Blob Parameter Block */
/************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__bpb_version1                  1
#define gds__bpb_source_type               1
#define gds__bpb_target_type               2
#define gds__bpb_type                      3
#define gds__bpb_source_interp             4
#define gds__bpb_target_interp             5

#else /* c++ definitions */

const char gds_bpb_version1 = 1;
const char gds_bpb_source_type = 1;
const char gds_bpb_target_type = 2;
const char gds_bpb_type = 3;
const char gds_bpb_source_interp = 4;
const char gds_bpb_target_interp = 5;

#endif


#ifndef	__cplusplus				/* c definitions */

#define gds__bpb_type_segmented            0
#define gds__bpb_type_stream               1

#else /* c++ definitions */

const char gds_bpb_type_segmented = 0;
const char gds_bpb_type_stream = 1;

#endif


/*********************************/
/* Service parameter block stuff */
/*********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__spb_version1                  1
#define gds__spb_user_name                 2
#define gds__spb_sys_user_name             3
#define gds__spb_sys_user_name_enc         4
#define gds__spb_password                  5
#define gds__spb_password_enc              6
#define gds__spb_command_line              7
#define gds__spb_connect_timeout           8
#define gds__spb_dummy_packet_interval     9

#else /* c++ definitions */

const char gds_spb_version1 = 1;
const char gds_spb_user_name = 2;
const char gds_spb_sys_user_name = 3;
const char gds_spb_sys_user_name_enc = 4;
const char gds_spb_password = 5;
const char gds_spb_password_enc = 6;
const char gds_spb_command_line = 7;
const char gds_spb_connect_timeout = 8;
const char gds_spb_dummy_packet_interval = 9;

#endif




/*********************************/
/* Information call declarations */
/*********************************/

/****************************/
/* Common, structural codes */
/****************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__info_end                      1
#define gds__info_truncated                2
#define gds__info_error                    3

#else /* c++ definitions */

const char gds_info_end = 1;
const char gds_info_truncated = 2;
const char gds_info_error = 3;

#endif




/******************************/
/* Database information items */
/******************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__info_db_id                    4
#define gds__info_reads                    5
#define gds__info_writes                   6
#define gds__info_fetches                  7
#define gds__info_marks                    8
#define gds__info_implementation           11
#define gds__info_version                  12
#define gds__info_base_level               13
#define gds__info_page_size                14
#define gds__info_num_buffers              15
#define gds__info_limbo                    16
#define gds__info_current_memory           17
#define gds__info_max_memory               18
#define gds__info_window_turns             19
#define gds__info_license                  20
#define gds__info_allocation               21
#define gds__info_attachment_id            22
#define gds__info_read_seq_count           23
#define gds__info_read_idx_count           24
#define gds__info_insert_count             25
#define gds__info_update_count             26
#define gds__info_delete_count             27
#define gds__info_backout_count            28
#define gds__info_purge_count              29
#define gds__info_expunge_count            30
#define gds__info_sweep_interval           31
#define gds__info_ods_version              32
#define gds__info_ods_minor_version        33
#define gds__info_no_reserve               34
#define gds__info_logfile                  35
#define gds__info_cur_logfile_name         36
#define gds__info_cur_log_part_offset      37
#define gds__info_num_wal_buffers          38
#define gds__info_wal_buffer_size          39
#define gds__info_wal_ckpt_length          40
#define gds__info_wal_cur_ckpt_interval    41
#define gds__info_wal_prv_ckpt_fname       42
#define gds__info_wal_prv_ckpt_poffset     43
#define gds__info_wal_recv_ckpt_fname      44
#define gds__info_wal_recv_ckpt_poffset    45
#define gds__info_wal_grpc_wait_usecs      47
#define gds__info_wal_num_io               48
#define gds__info_wal_avg_io_size          49
#define gds__info_wal_num_commits          50
#define gds__info_wal_avg_grpc_size        51
#define gds__info_forced_writes		   52

#else /* c++ definitions */

const char gds_info_db_id = 4;
const char gds_info_reads = 5;
const char gds_info_writes = 6;
const char gds_info_fetches = 7;
const char gds_info_marks = 8;
const char gds_info_implementation = 11;
const char gds_info_version = 12;
const char gds_info_base_level = 13;
const char gds_info_page_size = 14;
const char gds_info_num_buffers = 15;
const char gds_info_limbo = 16;
const char gds_info_current_memory = 17;
const char gds_info_max_memory = 18;
const char gds_info_window_turns = 19;
const char gds_info_license = 20;
const char gds_info_allocation = 21;
const char gds_info_attachment_id = 22;
const char gds_info_read_seq_count = 23;
const char gds_info_read_idx_count = 24;
const char gds_info_insert_count = 25;
const char gds_info_update_count = 26;
const char gds_info_delete_count = 27;
const char gds_info_backout_count = 28;
const char gds_info_purge_count = 29;
const char gds_info_expunge_count = 30;
const char gds_info_sweep_interval = 31;
const char gds_info_ods_version = 32;
const char gds_info_ods_minor_version = 33;
const char gds_info_no_reserve = 34;
const char gds_info_logfile = 35;
const char gds_info_cur_logfile_name = 36;
const char gds_info_cur_log_part_offset = 37;
const char gds_info_num_wal_buffers = 38;
const char gds_info_wal_buffer_size = 39;
const char gds_info_wal_ckpt_length = 40;
const char gds_info_wal_cur_ckpt_interval = 41;
const char gds_info_wal_prv_ckpt_fname = 42;
const char gds_info_wal_prv_ckpt_poffset = 43;
const char gds_info_wal_recv_ckpt_fname = 44;
const char gds_info_wal_recv_ckpt_poffset = 45;
const char gds_info_wal_grpc_wait_usecs = 47;
const char gds_info_wal_num_io = 48;
const char gds_info_wal_avg_io_size = 49;
const char gds_info_wal_num_commits = 50;
const char gds_info_wal_avg_grpc_size = 51;
const char gds_info_forced_writes = 52;

#endif


/**************************************/
/* Database information return values */
/**************************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__info_db_impl_rdb_vms          1
#define gds__info_db_impl_rdb_eln          2
#define gds__info_db_impl_rdb_eln_dev      3
#define gds__info_db_impl_rdb_vms_y        4
#define gds__info_db_impl_rdb_eln_y        5
#define gds__info_db_impl_jri              6
#define gds__info_db_impl_jsv              7
#define gds__info_db_impl_isc_a            25
#define gds__info_db_impl_isc_u            26
#define gds__info_db_impl_isc_v            27
#define gds__info_db_impl_isc_s            28
#define gds__info_db_impl_isc_apl_68K      25
#define gds__info_db_impl_isc_vax_ultr     26
#define gds__info_db_impl_isc_vms          27
#define gds__info_db_impl_isc_sun_68k      28
#define gds__info_db_impl_isc_sun4         30
#define gds__info_db_impl_isc_hp_ux        31
#define gds__info_db_impl_isc_sun_386i     32
#define gds__info_db_impl_isc_vms_orcl     33
#define gds__info_db_impl_isc_rt_aix       35
#define gds__info_db_impl_isc_mips_ult     36
#define gds__info_db_impl_isc_dg           38
#define gds__info_db_impl_isc_hp_mpexl     39
#define gds__info_db_impl_isc_hp_ux68K     40
#define gds__info_db_impl_isc_sgi          41
#define gds__info_db_impl_isc_sco_unix     42
#define gds__info_db_impl_isc_dos          47
#define gds__info_db_impl_isc_winnt        48

#define gds__info_db_class_access          1
#define gds__info_db_class_y_valve         2
#define gds__info_db_class_rem_int         3
#define gds__info_db_class_rem_srvr        4
#define gds__info_db_class_pipe_int        7
#define gds__info_db_class_pipe_srvr       8
#define gds__info_db_class_sam_int         9
#define gds__info_db_class_sam_srvr        10
#define gds__info_db_class_gateway         11
#define gds__info_db_class_cache           12

#else /* c++ definitions */

const char gds_info_db_impl_rdb_vms = 1;
const char gds_info_db_impl_rdb_eln = 2;
const char gds_info_db_impl_rdb_eln_dev = 3;
const char gds_info_db_impl_rdb_vms_y = 4;
const char gds_info_db_impl_rdb_eln_y = 5;
const char gds_info_db_impl_jri = 6;
const char gds_info_db_impl_jsv = 7;
const char gds_info_db_impl_isc_a = 25;
const char gds_info_db_impl_isc_u = 26;
const char gds_info_db_impl_isc_v = 27;
const char gds_info_db_impl_isc_s = 28;
const char gds_info_db_impl_isc_apl_68K = 25;
const char gds_info_db_impl_isc_vax_ultr = 26;
const char gds_info_db_impl_isc_vms = 27;
const char gds_info_db_impl_isc_sun_68k = 28;
const char gds_info_db_impl_isc_sun4 = 30;
const char gds_info_db_impl_isc_hp_ux = 31;
const char gds_info_db_impl_isc_sun_386i = 32;
const char gds_info_db_impl_isc_vms_orcl = 33;
const char gds_info_db_impl_isc_rt_aix = 35;
const char gds_info_db_impl_isc_mips_ult = 36;
const char gds_info_db_impl_isc_dg = 38;
const char gds_info_db_impl_isc_hp_mpexl = 39;
const char gds_info_db_impl_isc_hp_ux68K = 40;
const char gds_info_db_impl_isc_sgi = 41;
const char gds_info_db_impl_isc_sco_unix = 42;
const char gds_info_db_impl_isc_dos = 47;
const char gds_info_db_impl_isc_winnt = 48;

const char gds_info_db_class_access = 1;
const char gds_info_db_class_y_valve = 2;
const char gds_info_db_class_rem_int = 3;
const char gds_info_db_class_rem_srvr = 4;
const char gds_info_db_class_pipe_int = 7;
const char gds_info_db_class_pipe_srvr = 8;
const char gds_info_db_class_sam_int = 9;
const char gds_info_db_class_sam_srvr = 10;
const char gds_info_db_class_gateway = 11;
const char gds_info_db_class_cache = 12;

#endif


/*****************************/
/* Request information items */
/*****************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__info_number_messages          4
#define gds__info_max_message              5
#define gds__info_max_send                 6
#define gds__info_max_receive              7
#define gds__info_state                    8
#define gds__info_message_number           9
#define gds__info_message_size             10
#define gds__info_request_cost             11
#define gds__info_access_path              12
#define gds__info_req_select_count         13
#define gds__info_req_insert_count         14
#define gds__info_req_update_count         15
#define gds__info_req_delete_count         16

/*********************/
/* access path items */
/*********************/

#define gds__info_rsb_end		   0
#define gds__info_rsb_begin		   1
#define gds__info_rsb_type		   2
#define gds__info_rsb_relation		   3
#define gds__info_rsb_plan                 4

/*************/
/* rsb types */
/*************/

#define gds__info_rsb_unknown		   1
#define gds__info_rsb_indexed		   2
#define gds__info_rsb_navigate		   3
#define gds__info_rsb_sequential	   4
#define gds__info_rsb_cross		   5
#define gds__info_rsb_sort		   6
#define gds__info_rsb_first		   7
#define gds__info_rsb_boolean		   8
#define gds__info_rsb_union		   9
#define gds__info_rsb_aggregate		  10
#define gds__info_rsb_merge		  11
#define gds__info_rsb_ext_sequential	  12
#define gds__info_rsb_ext_indexed	  13
#define gds__info_rsb_ext_dbkey		  14
#define gds__info_rsb_left_cross	  15
#define gds__info_rsb_select		  16
#define gds__info_rsb_sql_join		  17
#define gds__info_rsb_simulate		  18
#define gds__info_rsb_sim_cross		  19
#define gds__info_rsb_once		  20
#define gds__info_rsb_procedure		  21
#define gds__info_rsb_skip		  22

/**********************/
/* bitmap expressions */
/**********************/

#define gds__info_rsb_and		1
#define gds__info_rsb_or		2
#define gds__info_rsb_dbkey		3
#define gds__info_rsb_index		4

#define gds__info_req_active               2
#define gds__info_req_inactive             3
#define gds__info_req_send                 4
#define gds__info_req_receive              5
#define gds__info_req_select               6

#else /* c++ definitions */

const char gds_info_number_messages = 4;
const char gds_info_max_message = 5;
const char gds_info_max_send = 6;
const char gds_info_max_receive = 7;
const char gds_info_state = 8;
const char gds_info_message_number = 9;
const char gds_info_message_size = 10;
const char gds_info_request_cost = 11;
const char gds_info_access_path = 12;
const char gds_info_req_select_count = 13;
const char gds_info_req_insert_count = 14;
const char gds_info_req_update_count = 15;
const char gds_info_req_delete_count = 16;

/*********************/
/* access path items */
/*********************/

const char gds_info_rsb_end = 0;
const char gds_info_rsb_begin = 1;
const char gds_info_rsb_type = 2;
const char gds_info_rsb_relation = 3;



/*************/
/* rsb types */
/*************/

const char gds_info_rsb_unknown = 1;
const char gds_info_rsb_indexed = 2;
const char gds_info_rsb_navigate = 3;
const char gds_info_rsb_sequential = 4;
const char gds_info_rsb_cross = 5;
const char gds_info_rsb_sort = 6;
const char gds_info_rsb_first = 7;
const char gds_info_rsb_boolean = 8;
const char gds_info_rsb_union = 9;
const char gds_info_rsb_aggregate = 10;
const char gds_info_rsb_merge = 11;
const char gds_info_rsb_ext_sequential = 12;
const char gds_info_rsb_ext_indexed = 13;
const char gds_info_rsb_ext_dbkey = 14;
const char gds_info_rsb_left_cross = 15;
const char gds_info_rsb_select = 16;
const char gds_info_rsb_sql_join = 17;
const char gds_info_rsb_simulate = 18;
const char gds_info_rsb_sim_cross = 19;
const char gds_info_rsb_once = 20;
const char gds_info_rsb_procedure = 21;
const char gds_info_rsb_skip = 22;

/**********************/
/* bitmap expressions */
/**********************/

const char gds_info_rsb_and = 1;
const char gds_info_rsb_or = 2;
const char gds_info_rsb_dbkey = 3;
const char gds_info_rsb_index = 4;

const char gds_info_req_active = 2;
const char gds_info_req_inactive = 3;
const char gds_info_req_send = 4;
const char gds_info_req_receive = 5;
const char gds_info_req_select = 6;

#endif


/**************************/
/* Blob information items */
/**************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__info_blob_num_segments        4
#define gds__info_blob_max_segment         5
#define gds__info_blob_total_length        6
#define gds__info_blob_type                7

#else /* c++ definitions */

const char gds_info_blob_num_segments = 4;
const char gds_info_blob_max_segment = 5;
const char gds_info_blob_total_length = 6;
const char gds_info_blob_type = 7;

#endif




/*********************************/
/* Transaction information items */
/*********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__info_tra_id                   4

#else /* c++ definitions */

const char gds_info_tra_id = 4;

#endif


/*****************************/
/* Service information items */
/*****************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__info_svc_version              4
#define gds__info_svc_message              5
#define gds__info_svc_total_length         6
#define gds__info_svc_response             7
#define gds__info_svc_response_more        8
#define gds__info_svc_line                 9
#define gds__info_svc_to_eof               10
#define gds__info_svc_timeout              11

#else /* c++ definitions */

const char gds_info_svc_version = 4;
const char gds_info_svc_message = 5;
const char gds_info_svc_total_length = 6;
const char gds_info_svc_response = 7;
const char gds_info_svc_response_more = 8;
const char gds_info_svc_line = 9;
const char gds_info_svc_to_eof = 10;
const char gds_info_svc_timeout = 11;

#endif

/*************************/
/* SQL information items */
/*************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__info_sql_select               4
#define gds__info_sql_bind                 5
#define gds__info_sql_num_variables        6
#define gds__info_sql_describe_vars        7
#define gds__info_sql_describe_end         8
#define gds__info_sql_sqlda_seq            9
#define gds__info_sql_message_seq          10
#define gds__info_sql_type                 11
#define gds__info_sql_sub_type             12
#define gds__info_sql_scale                13
#define gds__info_sql_length               14
#define gds__info_sql_null_ind             15
#define gds__info_sql_field                16
#define gds__info_sql_relation             17
#define gds__info_sql_owner                18
#define gds__info_sql_alias                19
#define gds__info_sql_sqlda_start          20
#define gds__info_sql_stmt_type            21
#define gds__info_sql_get_plan             22
#define gds__info_sql_records		   23

#else /* c++ definitions */

const char gds_info_sql_select = 4;
const char gds_info_sql_bind = 5;
const char gds_info_sql_num_variables = 6;
const char gds_info_sql_describe_vars = 7;
const char gds_info_sql_describe_end = 8;
const char gds_info_sql_sqlda_seq = 9;
const char gds_info_sql_message_seq = 10;
const char gds_info_sql_type = 11;
const char gds_info_sql_sub_type = 12;
const char gds_info_sql_scale = 13;
const char gds_info_sql_length = 14;
const char gds_info_sql_null_ind = 15;
const char gds_info_sql_field = 16;
const char gds_info_sql_relation = 17;
const char gds_info_sql_owner = 18;
const char gds_info_sql_alias = 19;
const char gds_info_sql_sqlda_start = 20;
const char gds_info_sql_stmt_type = 21;
const char gds_info_sql_get_plan = 22;
const char gds_info_sql_records = 23;

#endif




/*********************************/
/* SQL information return values */
/*********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__info_sql_stmt_select          1
#define gds__info_sql_stmt_insert          2
#define gds__info_sql_stmt_update          3
#define gds__info_sql_stmt_delete          4
#define gds__info_sql_stmt_ddl             5
#define gds__info_sql_stmt_get_segment     6
#define gds__info_sql_stmt_put_segment     7
#define gds__info_sql_stmt_exec_procedure  8
#define gds__info_sql_stmt_start_trans     9
#define gds__info_sql_stmt_commit          10
#define gds__info_sql_stmt_rollback        11
#define gds__info_sql_stmt_select_for_upd  12

#else /* c++ definitions */

const char gds_info_sql_stmt_select = 1;
const char gds_info_sql_stmt_insert = 2;
const char gds_info_sql_stmt_update = 3;
const char gds_info_sql_stmt_delete = 4;
const char gds_info_sql_stmt_ddl = 5;
const char gds_info_sql_stmt_get_segment = 6;
const char gds_info_sql_stmt_put_segment = 7;
const char gds_info_sql_stmt_exec_procedure = 8;
const char gds_info_sql_stmt_start_trans = 9;
const char gds_info_sql_stmt_commit = 10;
const char gds_info_sql_stmt_rollback = 11;
const char gds_info_sql_stmt_select_for_upd = 12;

#endif

/***************/
/* Error codes */
/***************/


#include "codes.h"

#include "iberror.h"




/**********************************************/
/* Dynamic Data Definition Language operators */
/**********************************************/

/******************/
/* Version number */
/******************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_version_1                 1
#define gds__dyn_eoc                       -1

#else /* c++ definitions */

const unsigned char gds_dyn_version_1 = 1;
const unsigned char gds_dyn_eoc = 0xFF;

#endif

/******************************/
/* Operations (may be nested) */
/******************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_begin                     2
#define gds__dyn_end                       3
#define gds__dyn_if                        4
#define gds__dyn_def_database              5
#define gds__dyn_def_global_fld            6
#define gds__dyn_def_local_fld             7
#define gds__dyn_def_idx                   8
#define gds__dyn_def_rel                   9
#define gds__dyn_def_sql_fld               10
#define gds__dyn_def_view                  12
#define gds__dyn_def_trigger               15
#define gds__dyn_def_security_class        120
#define gds__dyn_def_dimension             140
#define gds__dyn_def_generator             24
#define gds__dyn_def_function              25
#define gds__dyn_def_filter                26
#define gds__dyn_def_function_arg          27
#define gds__dyn_def_shadow                34
#define gds__dyn_def_trigger_msg           17
#define gds__dyn_def_file                  36
#define gds__dyn_mod_database              39
#define gds__dyn_mod_rel                   11
#define gds__dyn_mod_global_fld            13
#define gds__dyn_mod_idx                   102
#define gds__dyn_mod_local_fld             14
#define gds__dyn_mod_view                  16
#define gds__dyn_mod_security_class        122
#define gds__dyn_mod_trigger               113
#define gds__dyn_mod_trigger_msg           28
#define gds__dyn_delete_database           18
#define gds__dyn_delete_rel                19
#define gds__dyn_delete_global_fld         20
#define gds__dyn_delete_local_fld          21
#define gds__dyn_delete_idx                22
#define gds__dyn_delete_security_class     123
#define gds__dyn_delete_dimensions         143
#define gds__dyn_delete_trigger            23
#define gds__dyn_delete_trigger_msg        29
#define gds__dyn_delete_filter             32
#define gds__dyn_delete_function           33
#define gds__dyn_delete_shadow             35
#define gds__dyn_grant                     30
#define gds__dyn_revoke                    31
#define gds__dyn_def_primary_key           37
#define gds__dyn_def_foreign_key           38
#define gds__dyn_def_unique                40
#define gds__dyn_def_procedure             164
#define gds__dyn_delete_procedure          165
#define gds__dyn_def_parameter             135
#define gds__dyn_delete_parameter          136
#define gds__dyn_mod_procedure             175
#define gds__dyn_def_log_file              176
#define gds__dyn_def_cache_file            180
#define gds__dyn_def_exception             181
#define gds__dyn_mod_exception             182
#define gds__dyn_del_exception             183
#define gds__dyn_drop_log                  194
#define gds__dyn_drop_cache                195
#define gds__dyn_def_default_log           202

#else /* c++ definitions */

const unsigned char gds_dyn_begin = 2;
const unsigned char gds_dyn_end = 3;
const unsigned char gds_dyn_if = 4;
const unsigned char gds_dyn_def_database = 5;
const unsigned char gds_dyn_def_global_fld = 6;
const unsigned char gds_dyn_def_local_fld = 7;
const unsigned char gds_dyn_def_idx = 8;
const unsigned char gds_dyn_def_rel = 9;
const unsigned char gds_dyn_def_sql_fld = 10;
const unsigned char gds_dyn_def_view = 12;
const unsigned char gds_dyn_def_trigger = 15;
const unsigned char gds_dyn_def_security_class = 120;
const unsigned char gds_dyn_def_dimension = 140;
const unsigned char gds_dyn_def_generator = 24;
const unsigned char gds_dyn_def_function = 25;
const unsigned char gds_dyn_def_filter = 26;
const unsigned char gds_dyn_def_function_arg = 27;
const unsigned char gds_dyn_def_shadow = 34;
const unsigned char gds_dyn_def_trigger_msg = 17;
const unsigned char gds_dyn_def_file = 36;
const unsigned char gds_dyn_mod_database = 39;
const unsigned char gds_dyn_mod_rel = 11;
const unsigned char gds_dyn_mod_global_fld = 13;
const unsigned char gds_dyn_mod_idx = 102;
const unsigned char gds_dyn_mod_local_fld = 14;
const unsigned char gds_dyn_mod_view = 16;
const unsigned char gds_dyn_mod_security_class = 122;
const unsigned char gds_dyn_mod_trigger = 113;
const unsigned char gds_dyn_mod_trigger_msg = 28;
const unsigned char gds_dyn_delete_database = 18;
const unsigned char gds_dyn_delete_rel = 19;
const unsigned char gds_dyn_delete_global_fld = 20;
const unsigned char gds_dyn_delete_local_fld = 21;
const unsigned char gds_dyn_delete_idx = 22;
const unsigned char gds_dyn_delete_security_class = 123;
const unsigned char gds_dyn_delete_dimensions = 143;
const unsigned char gds_dyn_delete_trigger = 23;
const unsigned char gds_dyn_delete_trigger_msg = 29;
const unsigned char gds_dyn_delete_filter = 32;
const unsigned char gds_dyn_delete_function = 33;
const unsigned char gds_dyn_delete_shadow = 35;
const unsigned char gds_dyn_grant = 30;
const unsigned char gds_dyn_revoke = 31;
const unsigned char gds_dyn_def_primary_key = 37;
const unsigned char gds_dyn_def_foreign_key = 38;
const unsigned char gds_dyn_def_unique = 40;
const unsigned char gds_dyn_def_procedure = 164;
const unsigned char gds_dyn_delete_procedure = 165;
const unsigned char gds_dyn_def_parameter = 135;
const unsigned char gds_dyn_delete_parameter = 136;
const unsigned char gds_dyn_mod_procedure = 175;
const unsigned char gds_dyn_def_log_file = 176;
const unsigned char gds_dyn_def_cache_file = 180;
const unsigned char gds_dyn_def_exception = 181;
const unsigned char gds_dyn_mod_exception = 182;
const unsigned char gds_dyn_del_exception = 183;
const unsigned char gds_dyn_drop_log = 194;
const unsigned char gds_dyn_drop_cache = 195;
const unsigned char gds_dyn_def_default_log = 202;

#endif




/***********************/
/* View specific stuff */
/***********************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_view_blr                  43
#define gds__dyn_view_source               44
#define gds__dyn_view_relation             45
#define gds__dyn_view_context              46
#define gds__dyn_view_context_name         47

#else /* c++ definitions */

const char gds_dyn_view_blr = 43;
const char gds_dyn_view_source = 44;
const char gds_dyn_view_relation = 45;
const char gds_dyn_view_context = 46;
const char gds_dyn_view_context_name = 47;

#endif

/**********************/
/* Generic attributes */
/**********************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_rel_name                  50
#define gds__dyn_fld_name                  51
#define gds__dyn_idx_name                  52
#define gds__dyn_description               53
#define gds__dyn_security_class            54
#define gds__dyn_system_flag               55
#define gds__dyn_update_flag               56
#define gds__dyn_prc_name                  166
#define gds__dyn_prm_name                  137
#define gds__dyn_sql_object                196
#define gds__dyn_fld_character_set_name    174

#else /* c++ definitions */

const unsigned char gds_dyn_rel_name = 50;
const unsigned char gds_dyn_fld_name = 51;
const unsigned char gds_dyn_idx_name = 52;
const unsigned char gds_dyn_description = 53;
const unsigned char gds_dyn_security_class = 54;
const unsigned char gds_dyn_system_flag = 55;
const unsigned char gds_dyn_update_flag = 56;
const unsigned char gds_dyn_prc_name = 166;
const unsigned char gds_dyn_prm_name = 137;
const unsigned char gds_dyn_sql_object = 196;
const unsigned char gds_dyn_fld_character_set_name = 174;

#endif

/********************************/
/* Relation specific attributes */
/********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_rel_dbkey_length          61
#define gds__dyn_rel_store_trig            62
#define gds__dyn_rel_modify_trig           63
#define gds__dyn_rel_erase_trig            64
#define gds__dyn_rel_store_trig_source     65
#define gds__dyn_rel_modify_trig_source    66
#define gds__dyn_rel_erase_trig_source     67
#define gds__dyn_rel_ext_file              68
#define gds__dyn_rel_sql_protection        69
#define gds__dyn_rel_constraint            162
#define gds__dyn_delete_rel_constraint     163

#else /* c++ definitions */

const unsigned char gds_dyn_rel_dbkey_length = 61;
const unsigned char gds_dyn_rel_store_trig = 62;
const unsigned char gds_dyn_rel_modify_trig = 63;
const unsigned char gds_dyn_rel_erase_trig = 64;
const unsigned char gds_dyn_rel_store_trig_source = 65;
const unsigned char gds_dyn_rel_modify_trig_source = 66;
const unsigned char gds_dyn_rel_erase_trig_source = 67;
const unsigned char gds_dyn_rel_ext_file = 68;
const unsigned char gds_dyn_rel_sql_protection = 69;
const unsigned char gds_dyn_rel_constraint = 162;
const unsigned char gds_dyn_delete_rel_constraint = 163;

#endif


/************************************/
/* Global field specific attributes */
/************************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_fld_type                  70
#define gds__dyn_fld_length                71
#define gds__dyn_fld_scale                 72
#define gds__dyn_fld_sub_type              73
#define gds__dyn_fld_segment_length        74
#define gds__dyn_fld_query_header          75
#define gds__dyn_fld_edit_string           76
#define gds__dyn_fld_validation_blr        77
#define gds__dyn_fld_validation_source     78
#define gds__dyn_fld_computed_blr          79
#define gds__dyn_fld_computed_source       80
#define gds__dyn_fld_missing_value         81
#define gds__dyn_fld_default_value         82
#define gds__dyn_fld_query_name            83
#define gds__dyn_fld_dimensions            84
#define gds__dyn_fld_not_null              85
#define gds__dyn_fld_char_length           172
#define gds__dyn_fld_collation             173
#define gds__dyn_fld_default_source        193
#define gds__dyn_del_default               197
#define gds__dyn_del_validation            198
#define gds__dyn_single_validation         199
#define gds__dyn_fld_character_set         203

#else /* c++ definitions */

const unsigned char gds_dyn_fld_type = 70;
const unsigned char gds_dyn_fld_length = 71;
const unsigned char gds_dyn_fld_scale = 72;
const unsigned char gds_dyn_fld_sub_type = 73;
const unsigned char gds_dyn_fld_segment_length = 74;
const unsigned char gds_dyn_fld_query_header = 75;
const unsigned char gds_dyn_fld_edit_string = 76;
const unsigned char gds_dyn_fld_validation_blr = 77;
const unsigned char gds_dyn_fld_validation_source = 78;
const unsigned char gds_dyn_fld_computed_blr = 79;
const unsigned char gds_dyn_fld_computed_source = 80;
const unsigned char gds_dyn_fld_missing_value = 81;
const unsigned char gds_dyn_fld_default_value = 82;
const unsigned char gds_dyn_fld_query_name = 83;
const unsigned char gds_dyn_fld_dimensions = 84;
const unsigned char gds_dyn_fld_not_null = 85;
const unsigned char gds_dyn_fld_char_length = 172;
const unsigned char gds_dyn_fld_collation = 173;
const unsigned char gds_dyn_fld_default_source = 193;
const unsigned char gds_dyn_del_default = 197;
const unsigned char gds_dyn_del_validation = 198;
const unsigned char gds_dyn_single_validation = 199;
const unsigned char gds_dyn_fld_character_set = 203;

#endif


/***********************************/
/* Local field specific attributes */
/***********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_fld_source                90
#define gds__dyn_fld_base_fld              91
#define gds__dyn_fld_position              92
#define gds__dyn_fld_update_flag           93

#else /* c++ definitions */

const unsigned char gds_dyn_fld_source = 90;
const unsigned char gds_dyn_fld_base_fld = 91;
const unsigned char gds_dyn_fld_position = 92;
const unsigned char gds_dyn_fld_update_flag = 93;

#endif


/*****************************/
/* Index specific attributes */
/*****************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_idx_unique                100
#define gds__dyn_idx_inactive              101
#define gds__dyn_idx_type                  103
#define gds__dyn_idx_foreign_key           104
#define gds__dyn_idx_ref_column            105
#define gds__dyn_idx_statistic		   204

#else /* c++ definitions */

const unsigned char gds_dyn_idx_unique = 100;
const unsigned char gds_dyn_idx_inactive = 101;
const unsigned char gds_dyn_idx_type = 103;
const unsigned char gds_dyn_idx_foreign_key = 104;
const unsigned char gds_dyn_idx_ref_column = 105;
const unsigned char gds_dyn_idx_statistic = 204;

#endif


/*******************************/
/* Trigger specific attributes */
/*******************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_trg_type                  110
#define gds__dyn_trg_blr                   111
#define gds__dyn_trg_source                112
#define gds__dyn_trg_name                  114
#define gds__dyn_trg_sequence              115
#define gds__dyn_trg_inactive              116
#define gds__dyn_trg_msg_number            117
#define gds__dyn_trg_msg                   118

#else /* c++ definitions */

const unsigned char gds_dyn_trg_type = 110;
const unsigned char gds_dyn_trg_blr = 111;
const unsigned char gds_dyn_trg_source = 112;
const unsigned char gds_dyn_trg_name = 114;
const unsigned char gds_dyn_trg_sequence = 115;
const unsigned char gds_dyn_trg_inactive = 116;
const unsigned char gds_dyn_trg_msg_number = 117;
const unsigned char gds_dyn_trg_msg = 118;

#endif


/**************************************/
/* Security Class specific attributes */
/**************************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_scl_acl                   121
#define gds__dyn_grant_user                130
#define gds__dyn_grant_proc                186
#define gds__dyn_grant_trig                187
#define gds__dyn_grant_view                188
#define gds__dyn_grant_options             132

#else /* c++ definitions */

const unsigned char gds_dyn_scl_acl = 121;
const unsigned char gds_dyn_grant_user = 130;
const unsigned char gds_dyn_grant_proc = 186;
const unsigned char gds_dyn_grant_trig = 187;
const unsigned char gds_dyn_grant_view = 188;
const unsigned char gds_dyn_grant_options = 132;

#endif


/**********************************/
/* Dimension specific information */
/**********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_dim_lower                 141
#define gds__dyn_dim_upper                 142

#else /* c++ definitions */

const unsigned char gds_dyn_dim_lower = 141;
const unsigned char gds_dyn_dim_upper = 142;

#endif


/****************************/
/* File specific attributes */
/****************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_file_name                 125
#define gds__dyn_file_start                126
#define gds__dyn_file_length               127
#define gds__dyn_shadow_number             128
#define gds__dyn_shadow_man_auto           129
#define gds__dyn_shadow_conditional        130

#else /* c++ definitions */

const unsigned char gds_dyn_file_name = 125;
const unsigned char gds_dyn_file_start = 126;
const unsigned char gds_dyn_file_length = 127;
const unsigned char gds_dyn_shadow_number = 128;
const unsigned char gds_dyn_shadow_man_auto = 129;
const unsigned char gds_dyn_shadow_conditional = 130;

#endif

/********************************/
/* Log file specific attributes */
/********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_log_file_sequence         177
#define gds__dyn_log_file_partitions       178
#define gds__dyn_log_file_serial           179
#define gds__dyn_log_file_overflow         200
#define gds__dyn_log_file_raw		   201

#else /* c++ definitions */

const unsigned char gds_dyn_log_file_sequence = 177;
const unsigned char gds_dyn_log_file_partitions = 178;
const unsigned char gds_dyn_log_file_serial = 179;
const unsigned char gds_dyn_log_file_overflow = 200;
const unsigned char gds_dyn_log_file_raw = 201;

#endif


/***************************/
/* Log specific attributes */
/***************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_log_group_commit_wait     189
#define gds__dyn_log_buffer_size           190
#define gds__dyn_log_check_point_length    191
#define gds__dyn_log_num_of_buffers        192

#else /* c++ definitions */

const unsigned char gds_dyn_log_group_commit_wait = 189;
const unsigned char gds_dyn_log_buffer_size = 190;
const unsigned char gds_dyn_log_check_point_length = 191;
const unsigned char gds_dyn_log_num_of_buffers = 192;

#endif

/********************************/
/* Function specific attributes */
/********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_function_name             145
#define gds__dyn_function_type             146
#define gds__dyn_func_module_name          147
#define gds__dyn_func_entry_point          148
#define gds__dyn_func_return_argument      149
#define gds__dyn_func_arg_position         150
#define gds__dyn_func_mechanism            151
#define gds__dyn_filter_in_subtype         152
#define gds__dyn_filter_out_subtype        153

#else /* c++ definitions */

const unsigned char gds_dyn_function_name = 145;
const unsigned char gds_dyn_function_type = 146;
const unsigned char gds_dyn_func_module_name = 147;
const unsigned char gds_dyn_func_entry_point = 148;
const unsigned char gds_dyn_func_return_argument = 149;
const unsigned char gds_dyn_func_arg_position = 150;
const unsigned char gds_dyn_func_mechanism = 151;
const unsigned char gds_dyn_filter_in_subtype = 152;
const unsigned char gds_dyn_filter_out_subtype = 153;

#endif

/*********************************/
/* Generator specific attributes */
/*********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_generator_name            95
#define gds__dyn_generator_id              96

#else /* c++ definitions */

const unsigned char gds_dyn_generator_name = 95;
const unsigned char gds_dyn_generator_id = 96;

#endif


/*********************************/
/* Procedure specific attributes */
/*********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_prc_inputs                167
#define gds__dyn_prc_outputs               168
#define gds__dyn_prc_source                169
#define gds__dyn_prc_blr                   170
#define gds__dyn_prc_source2               171

#else /* c++ definitions */

const unsigned char gds_dyn_prc_inputs = 167;
const unsigned char gds_dyn_prc_outputs = 168;
const unsigned char gds_dyn_prc_source = 169;
const unsigned char gds_dyn_prc_blr = 170;
const unsigned char gds_dyn_prc_source2 = 171;

#endif


/*********************************/
/* Parameter specific attributes */
/*********************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_prm_number                138
#define gds__dyn_prm_type                  139

#else /* c++ definitions */

const unsigned char gds_dyn_prm_number = 138;
const unsigned char gds_dyn_prm_type = 139;

#endif

/********************************/
/* Relation specific attributes */
/********************************/

#ifndef       __cplusplus		/* c definitions */

#define gds__dyn_xcp_msg                   185

#else /* c++ definitions */

const unsigned char gds_dyn_xcp_msg = 185;

#endif


/**********************************************/
/* Cascading referential integrity values     */
/**********************************************/
#ifndef __cplusplus				/* c definitions */

#define gds__dyn_foreign_key_update        205
#define gds__dyn_foreign_key_delete        206
#define gds__dyn_foreign_key_cascade       207
#define gds__dyn_foreign_key_default       208
#define gds__dyn_foreign_key_null          209
#define gds__dyn_foreign_key_none          210

#else /* c++ definitions */

const unsigned char gds_dyn_foreign_key_update = 205;
const unsigned char gds_dyn_foreign_key_delete = 206;
const unsigned char gds_dyn_foreign_key_cascade = 207;
const unsigned char gds_dyn_foreign_key_default = 208;
const unsigned char gds_dyn_foreign_key_null = 209;
const unsigned char gds_dyn_foreign_key_none = 210;

#endif


/****************************/
/* Last $dyn value assigned */
/****************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__dyn_last_dyn_value            210

#else /* c++ definitions */

const unsigned char gds_dyn_last_dyn_value = 210;

#endif





/******************************************/
/* Array slice description language (SDL) */
/******************************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__sdl_version1                  1
#define gds__sdl_eoc                       -1
#define gds__sdl_relation                  2
#define gds__sdl_rid                       3
#define gds__sdl_field                     4
#define gds__sdl_fid                       5
#define gds__sdl_struct                    6
#define gds__sdl_variable                  7
#define gds__sdl_scalar                    8
#define gds__sdl_tiny_integer              9
#define gds__sdl_short_integer             10
#define gds__sdl_long_integer              11
#define gds__sdl_literal                   12
#define gds__sdl_add                       13
#define gds__sdl_subtract                  14
#define gds__sdl_multiply                  15
#define gds__sdl_divide                    16
#define gds__sdl_negate                    17
#define gds__sdl_eql                       18
#define gds__sdl_neq                       19
#define gds__sdl_gtr                       20
#define gds__sdl_geq                       21
#define gds__sdl_lss                       22
#define gds__sdl_leq                       23
#define gds__sdl_and                       24
#define gds__sdl_or                        25
#define gds__sdl_not                       26
#define gds__sdl_while                     27
#define gds__sdl_assignment                28
#define gds__sdl_label                     29
#define gds__sdl_leave                     30
#define gds__sdl_begin                     31
#define gds__sdl_end                       32
#define gds__sdl_do3                       33
#define gds__sdl_do2                       34
#define gds__sdl_do1                       35
#define gds__sdl_element                   36

#else /* c++ definitions */

const unsigned char gds_sdl_version1 = 1;
/* Opps, can't set an unsigned value to -1.  Used to be:
 * const unsigned char gds_sdl_eoc = -1;
 */
const unsigned char gds_sdl_eoc = 0xFF;
const unsigned char gds_sdl_relation = 2;
const unsigned char gds_sdl_rid = 3;
const unsigned char gds_sdl_field = 4;
const unsigned char gds_sdl_fid = 5;
const unsigned char gds_sdl_struct = 6;
const unsigned char gds_sdl_variable = 7;
const unsigned char gds_sdl_scalar = 8;
const unsigned char gds_sdl_tiny_integer = 9;
const unsigned char gds_sdl_short_integer = 10;
const unsigned char gds_sdl_long_integer = 11;
const unsigned char gds_sdl_literal = 12;
const unsigned char gds_sdl_add = 13;
const unsigned char gds_sdl_subtract = 14;
const unsigned char gds_sdl_multiply = 15;
const unsigned char gds_sdl_divide = 16;
const unsigned char gds_sdl_negate = 17;
const unsigned char gds_sdl_eql = 18;
const unsigned char gds_sdl_neq = 19;
const unsigned char gds_sdl_gtr = 20;
const unsigned char gds_sdl_geq = 21;
const unsigned char gds_sdl_lss = 22;
const unsigned char gds_sdl_leq = 23;
const unsigned char gds_sdl_and = 24;
const unsigned char gds_sdl_or = 25;
const unsigned char gds_sdl_not = 26;
const unsigned char gds_sdl_while = 27;
const unsigned char gds_sdl_assignment = 28;
const unsigned char gds_sdl_label = 29;
const unsigned char gds_sdl_leave = 30;
const unsigned char gds_sdl_begin = 31;
const unsigned char gds_sdl_end = 32;
const unsigned char gds_sdl_do3 = 33;
const unsigned char gds_sdl_do2 = 34;
const unsigned char gds_sdl_do1 = 35;
const unsigned char gds_sdl_element = 36;

#endif


/********************************************/
/* International text interpretation values */
/********************************************/

#ifndef	__cplusplus				/* c definitions */

#define gds__interp_eng_ascii              0
#define gds__interp_jpn_sjis               5
#define gds__interp_jpn_euc                6

#else /* c++ definitions */

const unsigned char gds_interp_eng_ascii = 0;
const unsigned char gds_interp_jpn_sjis = 5;
const unsigned char gds_interp_jpn_euc = 6;

#endif

#endif /* JRD_GDSOLD_H */

