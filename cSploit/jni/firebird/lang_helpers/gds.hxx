/*
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "MAC" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "OS/2" port
 *
 */
/*
 *      PROGRAM:        C preprocessor
 *      MODULE:         gds.hxx
 *      DESCRIPTION:    BLR constants for C++
 */

#define gds_version3

#define GDS_TRUE        1
#define GDS_FALSE       0

typedef long    gds_db_handle;
typedef long    gds_req_handle;
typedef long    gds_tr_handle;
typedef long    gds_blob_handle;
typedef long    gds_win_handle;
typedef long    gds_form_handle;
typedef long    gds_stmt_handle;
typedef void    (*gds_callback)();

typedef struct {
    long                gds_quad_high;
    unsigned long       gds_quad_low;
} GDS_QUAD;

typedef struct {
    short       array_bound_lower;
    short       array_bound_upper;
} ISC_ARRAY_BOUND;

typedef struct {
    unsigned char       array_desc_dtype;
    char                array_desc_scale;
    unsigned short      array_desc_length;
    char                array_desc_field_name [32];
    char                array_desc_relation_name [32];
    short               array_desc_dimensions;
    short               array_desc_flags;
    ISC_ARRAY_BOUND     array_desc_bounds [16];
} ISC_ARRAY_DESC;

/* Dynamic SQL definitions */

typedef struct {
    short       sqltype;
    short       sqllen;
    char        *sqldata;
    short       *sqlind;
    short       sqlname_length;
    char        sqlname [30];
} SQLVAR;

typedef struct {
    char        sqldaid [8];
    long        sqldabc;
    short       sqln;
    short       sqld;
    SQLVAR      sqlvar [1];
} SQLDA;

#define SQLDA_LENGTH(n)         (sizeof (SQLDA) + (n-1) * sizeof (SQLVAR))

/* Extended SQLDA */

typedef struct {
    short       sqltype;
    short       sqlscale;
    short       sqlsubtype;
    short       sqllen;
    char        *sqldata;
    short       *sqlind;
    short       sqlname_length;
    char        sqlname [32];
    short       relname_length;
    char        relname [32];
    short       ownname_length;
    char        ownname [32];
    short       aliasname_length;
    char        aliasname [32];
} XSQLVAR;

const char SQLDA_VERSION1                   = 1;

typedef struct {
    short       version;
    char        sqldaid [8];
    long        sqldabc;
    short       sqln;
    short       sqld;
    XSQLVAR     sqlvar [1];
} XSQLDA;

#define XSQLDA_LENGTH(n)        (sizeof (XSQLDA) + (n-1) * sizeof (XSQLVAR))

#ifdef _Windows
typedef struct {
    unsigned short      length;
    char                string[];
} VARYING;
#endif


#define GDS_STATUS      long

#ifdef _Windows
#define GDS_EXPORT      far cdecl _loadds _export
#else
#define GDS_EXPORT
#endif

extern "C" {
  GDS_STATUS GDS_EXPORT isc_attach_database (long*, short, char*, gds_db_handle*, short, char*);
  GDS_STATUS GDS_EXPORT isc_blob_info (long*, gds_blob_handle*, short, char*, short, char*);
  GDS_STATUS GDS_EXPORT isc_cancel_blob (long*, gds_blob_handle*);
  GDS_STATUS GDS_EXPORT isc_close_blob (long*, gds_blob_handle*);
  GDS_STATUS GDS_EXPORT isc_commit_transaction (long*, gds_tr_handle*);
  GDS_STATUS GDS_EXPORT isc_compile_request  (long*, gds_db_handle*, gds_req_handle*, short, char*);
  GDS_STATUS GDS_EXPORT isc_compile_request2 (long*, gds_db_handle*, gds_req_handle*, short, char*);
  GDS_STATUS GDS_EXPORT isc_create_blob  (long*, gds_db_handle*, gds_tr_handle*, gds_blob_handle*, GDS_QUAD*);
  GDS_STATUS GDS_EXPORT isc_create_blob2  (long*, gds_db_handle*, gds_tr_handle*, gds_blob_handle*, GDS_QUAD*, short, char*);
  GDS_STATUS GDS_EXPORT isc_create_database  (long*, short, char*, gds_db_handle*, short, char*, short);
  GDS_STATUS GDS_EXPORT isc_database_info  (long*, gds_db_handle*, short, char*, short, char*);
  GDS_STATUS GDS_EXPORT isc_detach_database (long*, gds_db_handle*);
  GDS_STATUS GDS_EXPORT isc_get_segment (long*, gds_blob_handle*, unsigned short*, short, char*);
  GDS_STATUS GDS_EXPORT isc_open_blob (long*, gds_db_handle*, gds_tr_handle*, gds_blob_handle*, GDS_QUAD*);
  GDS_STATUS GDS_EXPORT isc_open_blob2 (long*, gds_db_handle*, gds_tr_handle*, gds_blob_handle*, GDS_QUAD*, short, char*);
  GDS_STATUS GDS_EXPORT isc_prepare_transaction  (long*, gds_tr_handle*);
  GDS_STATUS GDS_EXPORT isc_prepare_transaction2  (long*, gds_tr_handle*, short, char*);
  GDS_STATUS GDS_EXPORT isc_put_segment  (long*, gds_blob_handle*, short, char*);
  GDS_STATUS GDS_EXPORT isc_receive (long*, gds_req_handle*, short, short, void*, short);
  GDS_STATUS GDS_EXPORT isc_reconnect_transaction  (long*, gds_db_handle*, gds_tr_handle*, short, char*);
  GDS_STATUS GDS_EXPORT isc_request_info  (long*, gds_req_handle*, short, short, char*, short, char*);
  GDS_STATUS GDS_EXPORT isc_release_request  (long*, gds_req_handle*);
  GDS_STATUS GDS_EXPORT isc_rollback_transaction  (long*, gds_tr_handle*);
  GDS_STATUS GDS_EXPORT isc_seek_blob (long*, gds_blob_handle*, short, long, long*);
  GDS_STATUS GDS_EXPORT isc_send (long*, gds_req_handle*, short, short, void*, short);
  GDS_STATUS GDS_EXPORT isc_start_and_send (long*, gds_req_handle*, gds_tr_handle*, short, short, void*, short);
  GDS_STATUS GDS_EXPORT isc_start_multiple  (long*, gds_tr_handle*, short, void*);
  GDS_STATUS GDS_EXPORT isc_start_request (long*, gds_req_handle*, gds_tr_handle*, short);
  GDS_STATUS GDS_EXPORT isc_start_transaction (long*, gds_tr_handle*, short, ...);
  GDS_STATUS GDS_EXPORT isc_transaction_info  (long*, gds_tr_handle*, short, char*, short, char*);
  GDS_STATUS GDS_EXPORT isc_unwind_request  (long*, gds_tr_handle*, short);
  GDS_STATUS GDS_EXPORT isc_interprete  (char*, long**);
  GDS_STATUS GDS_EXPORT isc_interprete_cpp(char* const buffer, const long**);
  GDS_STATUS GDS_EXPORT isc_sql_interprete  (short, char*, short);
  GDS_STATUS GDS_EXPORT isc_print_sql_error  (long, long*);
  GDS_STATUS GDS_EXPORT isc_print_status  (long*);
  GDS_STATUS GDS_EXPORT isc_sqlcode  (long*);
  GDS_STATUS GDS_EXPORT isc_ddl (long*, gds_db_handle*, gds_tr_handle*, short, char*);
  GDS_STATUS GDS_EXPORT isc_commit_retaining (long*, gds_tr_handle*);
  GDS_STATUS GDS_EXPORT isc_que_events (long*, gds_db_handle*, long*, short, char*, gds_callback, void*);
  GDS_STATUS GDS_EXPORT isc_cancel_events (long*, gds_db_handle*, long*);
  GDS_STATUS GDS_EXPORT isc_wait_for_event (long*, gds_db_handle*, short, char*, char*);
  GDS_STATUS GDS_EXPORT isc_event_counts (long*, short, char*, char*);
  GDS_STATUS GDS_EXPORT isc_event_block (char**, char**, short, ...);
  GDS_STATUS GDS_EXPORT isc_get_slice (long*, gds_db_handle*, gds_tr_handle*, GDS_QUAD*, short, char*, short, long*,
			long, void*, long*);
  GDS_STATUS GDS_EXPORT isc_put_slice (long*, gds_db_handle*, gds_tr_handle*, GDS_QUAD*, short, char*, short, long*,
			long, void*);
  GDS_STATUS GDS_EXPORT isc_ddl (long*, gds_db_handle*, gds_tr_handle*, short, char*);
    }
extern "C" {
  GDS_STATUS GDS_EXPORT isc_prepare (long*, gds_db_handle*, gds_tr_handle*, char*, short*, char*, SQLDA*);
  GDS_STATUS GDS_EXPORT isc_declare (long*, char*, char*);
  GDS_STATUS GDS_EXPORT isc_open (long*, gds_tr_handle*, char*, SQLDA*);
  GDS_STATUS GDS_EXPORT isc_fetch (long*, char*, SQLDA*);
  GDS_STATUS GDS_EXPORT isc_close (long*, char*);
  GDS_STATUS GDS_EXPORT isc_describe (long*, char*, SQLDA*);
  GDS_STATUS GDS_EXPORT isc_describe_bind (long*, char*, SQLDA*);
  GDS_STATUS GDS_EXPORT isc_dsql_finish (long*);
  GDS_STATUS GDS_EXPORT isc_dsql_release (long*, char*);
  GDS_STATUS GDS_EXPORT isc_execute (long*, gds_tr_handle*, char*, SQLDA*);
  GDS_STATUS GDS_EXPORT isc_execute_immediate (long*, gds_db_handle*, gds_tr_handle*, short*, char*);
  GDS_STATUS GDS_EXPORT isc_array_get_slice (long*, long*, long*, GDS_QUAD*, ISC_ARRAY_DESC*, void*, long*);
  GDS_STATUS GDS_EXPORT isc_array_lookup_bounds (long*, long*, long*, char*, char*, ISC_ARRAY_DESC*);
  GDS_STATUS GDS_EXPORT isc_array_lookup_desc (long*, long*, long*, char*, char*, ISC_ARRAY_DESC*);
  GDS_STATUS GDS_EXPORT isc_array_set_desc (long*, char*, char*, short*, short*, short*, ISC_ARRAY_DESC*);
  GDS_STATUS GDS_EXPORT isc_array_put_slice (long*, long*, long*, GDS_QUAD*, ISC_ARRAY_DESC*, void*, long*);
    }
extern "C" {
  GDS_STATUS GDS_EXPORT isc_dsql_allocate_statement (long*, gds_db_handle*, gds_stmt_handle*);
  GDS_STATUS GDS_EXPORT isc_dsql_describe (long*, gds_stmt_handle*, short, void*);
  GDS_STATUS GDS_EXPORT isc_dsql_describe_bind (long*, gds_stmt_handle*, short, void*);
  GDS_STATUS GDS_EXPORT isc_dsql_execute (long*, gds_tr_handle*, gds_stmt_handle*, short, void*);
  GDS_STATUS GDS_EXPORT isc_dsql_execute_m (long*, gds_tr_handle*, gds_stmt_handle*, short, char*, short, short, char*);
  GDS_STATUS GDS_EXPORT isc_dsql_execute_immediate (long*, gds_db_handle*, gds_tr_handle*, short, char*, short, void*);
  GDS_STATUS GDS_EXPORT isc_dsql_execute_immediate_m (long*, gds_db_handle*, gds_tr_handle*, short, char*, short,
			short, char*, short, short, char*);
  GDS_STATUS GDS_EXPORT isc_dsql_fetch (long*, gds_stmt_handle*, short, void*);
  GDS_STATUS GDS_EXPORT isc_dsql_fetch_m (long*, gds_stmt_handle*, short, char*, short, short, char*);
  GDS_STATUS GDS_EXPORT isc_dsql_free_statement (long*, gds_stmt_handle*, short);
  GDS_STATUS GDS_EXPORT isc_dsql_prepare (long*, gds_tr_handle*, gds_stmt_handle*, short, char*, short, void*);
  GDS_STATUS GDS_EXPORT isc_dsql_prepare_m (long*, gds_tr_handle*, gds_stmt_handle*, short, char*, short,
			short, char*, short, char*);
  GDS_STATUS GDS_EXPORT isc_dsql_set_cursor_name (long*, gds_stmt_handle*, char*, short);
  GDS_STATUS GDS_EXPORT isc_dsql_sql_info (long*, gds_stmt_handle*, short, char*, short, char*);
  GDS_STATUS GDS_EXPORT isc_embed_dsql_close (long*, char*);
  GDS_STATUS GDS_EXPORT isc_embed_dsql_declare (long*, char*, char*);
  GDS_STATUS GDS_EXPORT isc_embed_dsql_describe (long*, char*, short, void*);
  GDS_STATUS GDS_EXPORT isc_embed_dsql_describe_bind (long*, char*, short, void*);
  GDS_STATUS GDS_EXPORT isc_embed_dsql_execute (long*, gds_tr_handle*, char*, short, void*);
  GDS_STATUS GDS_EXPORT isc_embed_dsql_execute_immed (long*, gds_db_handle*, gds_tr_handle*, short, char*, short, void*);
  GDS_STATUS GDS_EXPORT isc_embed_dsql_fetch (long*, char*, short, void*);
  GDS_STATUS GDS_EXPORT isc_embed_dsql_open (long*, gds_tr_handle*, char*, short, void*);
  GDS_STATUS GDS_EXPORT isc_embed_dsql_prepare (long*, gds_db_handle*, gds_tr_handle*, char*, short, char*, short, void*);
  GDS_STATUS GDS_EXPORT isc_embed_dsql_release (long*, char*);
    }
extern "C" {
  GDS_STATUS GDS_EXPORT BLOB_edit (GDS_QUAD*, gds_db_handle, gds_tr_handle, char*);
  GDS_STATUS GDS_EXPORT BLOB_display (GDS_QUAD*, gds_db_handle, gds_tr_handle, char*);
  GDS_STATUS GDS_EXPORT BLOB_load (GDS_QUAD*, gds_db_handle, gds_tr_handle, char*);
  GDS_STATUS GDS_EXPORT BLOB_dump (GDS_QUAD*, gds_db_handle, gds_tr_handle, char*);
    }
extern "C" {
  GDS_STATUS GDS_EXPORT isc_initialize_menu (long*, gds_req_handle*);
  GDS_STATUS GDS_EXPORT isc_put_entree (long*, gds_req_handle*, short*, char*, long*);
  GDS_STATUS GDS_EXPORT isc_get_entree (long*, gds_req_handle*, short*, char*, long*, short*);
  GDS_STATUS GDS_EXPORT isc_drive_menu (long*, gds_win_handle*, gds_req_handle*, short*, char*, short*, char*, short*,
			short*, char*, long*);
  GDS_STATUS GDS_EXPORT isc_drive_form (long*, gds_db_handle*, gds_tr_handle*, gds_win_handle*, gds_req_handle*,
			void*, void*);
  GDS_STATUS GDS_EXPORT isc_load_form (long*, gds_db_handle*, gds_tr_handle*, gds_form_handle*, short*, char*);
  GDS_STATUS GDS_EXPORT isc_compile_map (long*, gds_form_handle*, gds_req_handle*, short*, char*);
  GDS_STATUS GDS_EXPORT isc_compile_menu (long*, gds_form_handle*, gds_req_handle*, short*, char*);
  GDS_STATUS GDS_EXPORT isc_compile_sub_map (long*, gds_win_handle*, gds_req_handle*, short*, char*);
  GDS_STATUS GDS_EXPORT isc_form_fetch (long*, gds_db_handle*, gds_tr_handle*, gds_req_handle*, void*);
  GDS_STATUS GDS_EXPORT isc_form_delete (long*, gds_form_handle*);
  GDS_STATUS GDS_EXPORT isc_form_insert (long*, gds_db_handle*, gds_tr_handle*, gds_req_handle*, void*);
  GDS_STATUS GDS_EXPORT isc_reset_form (long*, gds_req_handle*);
  GDS_STATUS GDS_EXPORT isc_delete_window (long*, gds_win_handle*);
  GDS_STATUS GDS_EXPORT isc_pop_window (long*, gds_win_handle*);
  GDS_STATUS GDS_EXPORT isc_suspend_window (long*, gds_win_handle*);
  GDS_STATUS GDS_EXPORT isc_create_window (long*, gds_win_handle*, short*, char*, short*, short*);
  GDS_STATUS GDS_EXPORT isc_menu (long*, gds_win_handle*, gds_req_handle*, short*, char*);
    }
extern "C" {
    void     GDS_EXPORT isc_ftof  (char*, short, char*, short);
  GDS_STATUS GDS_EXPORT isc_print_blr (char*, gds_callback, void*, short);
    void     GDS_EXPORT isc_qtoq (GDS_QUAD*, GDS_QUAD*);
    void     GDS_EXPORT isc_vtof (char*, char*, short);
    void     GDS_EXPORT isc_vtov (char*, char*, short);
    void     GDS_EXPORT isc_version (gds_db_handle*, gds_callback, long);
  GDS_STATUS GDS_EXPORT isc_vax_integer (const char*, short);
    void     GDS_EXPORT isc_set_debug (long);
  GDS_STATUS GDS_EXPORT isc_encode_date (const void*, GDS_QUAD*);
  GDS_STATUS GDS_EXPORT isc_decode_date (const GDS_QUAD*, void*);
  GDS_STATUS GDS_EXPORT isc_free (long*);
    void     GDS_EXPORT isc_extend_dpb (char**, short*, char*, char*);
    }
#define blr_word(n) (n % 256), (n / 256)

const char blr_text                        = 14;
const char blr_short                       = 7;
const char blr_long                        = 8;
const char blr_quad                        = 9;
const char blr_float                       = 10;
const char blr_double                      = 27;
const char blr_d_float                     = 11;
const char blr_date                        = 35;
const char blr_varying                     = 37;
const short blr_blob                       = 261;
const char blr_cstring                     = 40;
const char blr_blob_id                     = 45;

const char blr_inner                       = 0;
const char blr_left                        = 1;
const char blr_right                       = 2;
const char blr_full                        = 3;

const char blr_version4                    = 4;
const char blr_eoc                         = 76;
const char blr_end                         = -1;

const char blr_assignment                  = 1;
const char blr_begin                       = 2;
const char blr_dcl_variable                = 3;
const char blr_message                     = 4;
const char blr_erase                       = 5;
const char blr_fetch                       = 6;
const char blr_for                         = 7;
const char blr_if                          = 8;
const char blr_loop                        = 9;
const char blr_modify                      = 10;
const char blr_handler                     = 11;
const char blr_receive                     = 12;
const char blr_select                      = 13;
const char blr_send                        = 14;
const char blr_store                       = 15;
const char blr_label                       = 17;
const char blr_leave                       = 18;
const char blr_store2                      = 19;
const char blr_post                        = 20;

const char blr_literal                     = 21;
const char blr_dbkey                       = 22;
const char blr_field                       = 23;
const char blr_fid                         = 24;
const char blr_parameter                   = 25;
const char blr_variable                    = 26;
const char blr_average                     = 27;
const char blr_count                       = 28;
const char blr_maximum                     = 29;
const char blr_minimum                     = 30;
const char blr_total                       = 31;
const char blr_add                         = 34;
const char blr_subtract                    = 35;
const char blr_multiply                    = 36;
const char blr_divide                      = 37;
const char blr_negate                      = 38;
const char blr_concatenate                 = 39;
const char blr_substring                   = 40;
const char blr_parameter2                  = 41;
const char blr_from                        = 42;
const char blr_via                         = 43;
const char blr_user_name                   = 44;
const char blr_null                        = 45;

const char blr_eql                         = 47;
const char blr_neq                         = 48;
const char blr_gtr                         = 49;
const char blr_geq                         = 50;
const char blr_lss                         = 51;
const char blr_leq                         = 52;
const char blr_containing                  = 53;
const char blr_matching                    = 54;
const char blr_starting                    = 55;
const char blr_between                     = 56;
const char blr_or                          = 57;
const char blr_and                         = 58;
const char blr_not                         = 59;
const char blr_any                         = 60;
const char blr_missing                     = 61;
const char blr_unique                      = 62;
const char blr_like                        = 63;

const char blr_stream                      = 65;
const char blr_set_index                   = 66;

const char blr_rse                         = 67;
const char blr_first                       = 68;
const char blr_project                     = 69;
const char blr_sort                        = 70;
const char blr_boolean                     = 71;
const char blr_ascending                   = 72;
const char blr_descending                  = 73;
const char blr_relation                    = 74;
const char blr_rid                         = 75;
const char blr_union                       = 76;
const char blr_map                         = 77;
const char blr_group_by                    = 78;
const char blr_aggregate                   = 79;
const char blr_join_type                   = 80;

const char blr_agg_count                   = 83;
const char blr_agg_max                     = 84;
const char blr_agg_min                     = 85;
const char blr_agg_total                   = 86;
const char blr_agg_average                 = 87;
const char blr_run_count                   = 88;
const char blr_run_max                     = 89;
const char blr_run_min                     = 90;
const char blr_run_total                   = 91;
const char blr_run_average                 = 92;

const char blr_function                    = 100;
const char blr_gen_id                      = 101;
const char blr_prot_mask                   = 102;
const char blr_upcase                      = 103;
const char blr_lock_state                  = 104;
const char blr_value_if                    = 105;
const char blr_matching2                   = 106;
const char blr_index                       = 107;
const char blr_ansi_like                   = 108;
const char blr_bookmark                    = 109;
const char blr_crack                       = 110;
const char blr_force_crack                 = 111;
const char blr_seek                        = 112;
const char blr_find                        = 113;
const char blr_lock_relation               = 114;
const char blr_lock_record                 = 115;
const char blr_set_bookmark		   = 116;
const char blr_get_bookmark		   = 117;
const char blr_rs_stream                   = 119;
const char blr_exec_proc                   = 120;
const char blr_begin_range                 = 121;
const char blr_end_range                   = 122;
const char blr_delete_range                = 123;

/* Database parameter block stuff */

const char gds_dpb_version1                = 1;
const char gds_dpb_cdd_pathname            = 1;
const char gds_dpb_allocation              = 2;
const char gds_dpb_journal                 = 3;
const char gds_dpb_page_size               = 4;
const char gds_dpb_num_buffers             = 5;
const char gds_dpb_buffer_length           = 6;
const char gds_dpb_debug                   = 7;
const char gds_dpb_garbage_collect         = 8;
const char gds_dpb_verify                  = 9;
const char gds_dpb_sweep                   = 10;
const char gds_dpb_enable_journal          = 11;
const char gds_dpb_disable_journal         = 12;
const char gds_dpb_dbkey_scope             = 13;
const char gds_dpb_number_of_users         = 14;
const char gds_dpb_trace                   = 15;
const char gds_dpb_no_garbage_collect      = 16;
const char gds_dpb_damaged                 = 17;
const char gds_dpb_license                 = 18;
const char gds_dpb_sys_user_name           = 19;
const char gds_dpb_encrypt_key             = 20;
const char gds_dpb_activate_shadow         = 21;
const char gds_dpb_sweep_interval          = 22;
const char gds_dpb_delete_shadow           = 23;
const char gds_dpb_force_write             = 24;
const char gds_dpb_begin_log               = 25;
const char gds_dpb_quit_log                = 26;
const char gds_dpb_no_reserve              = 27;
const char gds_dpb_user_name               = 28;
const char gds_dpb_password                = 29;
const char gds_dpb_password_enc            = 30;
const char gds_dpb_sys_user_name_enc       = 31;

const char gds_dpb_pages                   = 1;
const char gds_dpb_records                 = 2;
const char gds_dpb_indices                 = 4;
const char gds_dpb_transactions            = 8;
const char gds_dpb_no_update               = 16;
const char gds_dpb_repair                  = 32;
const char gds_dpb_ignore                  = 64;

/* Bit assignments in RDB$SYSTEM_FLAG */

const char RDB_system                      = 1;
const char RDB_id_assigned                 = 2;

/* Transaction parameter blob stuff */

const char gds_tpb_version1                = 1;
const char gds_tpb_version3                = 3;
const char gds_tpb_consistency             = 1;
const char gds_tpb_concurrency             = 2;
const char gds_tpb_shared                  = 3;
const char gds_tpb_protected               = 4;
const char gds_tpb_exclusive               = 5;
const char gds_tpb_wait                    = 6;
const char gds_tpb_nowait                  = 7;
const char gds_tpb_read                    = 8;
const char gds_tpb_write                   = 9;
const char gds_tpb_lock_read               = 10;
const char gds_tpb_lock_write              = 11;
const char gds_tpb_verb_time               = 12;
const char gds_tpb_commit_time             = 13;
const char gds_tpb_ignore_limbo            = 14;

/* Blob Parameter Block */

const char gds_bpb_version1                = 1;
const char gds_bpb_source_type             = 1;
const char gds_bpb_target_type             = 2;
const char gds_bpb_type                    = 3;
const char gds_bpb_type_segmented          = 0;
const char gds_bpb_type_stream             = 1;

/* Blob stream stuff */

typedef struct bstream {
    int         *bstr_blob;             /* Blob handle */
    char        *bstr_buffer;           /* Address of buffer */
    char        *bstr_ptr;              /* Next character */
    short       bstr_length;            /* Length of buffer */
    short       bstr_cnt;               /* Characters in buffer */
    char        bstr_mode;              /* (mode) ? OUTPUT : INPUT */
} BSTREAM;

#define getb(p) (--(p)->bstr_cnt >= 0 ? *(p)->bstr_ptr++ & 0377: BLOB_get (p))
#define putb(x,p) ((x == '\n' || (!(--(p)->bstr_cnt))) ? BLOB_put (x,p) : ((int) (*(p)->bstr_ptr++ = (unsigned) (x))))
#define putbx(x,p) ((!(--(p)->bstr_cnt)) ? BLOB_put (x,p) : ((int) (*(p)->bstr_ptr++ = (unsigned) (x))))

extern "C" {
    BSTREAM *Bopen (GDS_QUAD*, gds_db_handle, gds_tr_handle, char*);
	BSTREAM *BLOB_open (gds_blob_handle, char*, char*);
    }

extern "C" {
    int GDS_EXPORT BLOB_close (BSTREAM*);
    int GDS_EXPORT BLOB_get (BSTREAM*);
    int GDS_EXPORT BLOB_put (int, BSTREAM*);
    }
/* Information call declarations */

/* Common, structural codes */

const char gds_info_end                    = 1;
const char gds_info_truncated              = 2;
const char gds_info_error                  = 3;

/* Database information items */

const char gds_info_db_id                  = 4;
const char gds_info_reads                  = 5;
const char gds_info_writes                 = 6;
const char gds_info_fetches                = 7;
const char gds_info_marks                  = 8;
const char gds_info_implementation         = 11;
const char gds_info_version                = 12;
const char gds_info_base_level             = 13;
const char gds_info_page_size              = 14;
const char gds_info_num_buffers            = 15;
const char gds_info_limbo                  = 16;
const char gds_info_current_memory         = 17;
const char gds_info_max_memory             = 18;
const char gds_info_window_turns           = 19;
const char gds_info_license                = 20;
const char gds_info_allocation             = 21;
const char gds_info_attachment_id          = 22;
const char gds_info_read_seq_count         = 23;
const char gds_info_read_idx_count         = 24;
const char gds_info_insert_count           = 25;
const char gds_info_update_count           = 26;
const char gds_info_delete_count           = 27;
const char gds_info_backout_count          = 28;
const char gds_info_purge_count            = 29;
const char gds_info_expunge_count          = 30;
const char gds_info_sweep_interval         = 31;
const char gds_info_ods_version            = 32;
const char gds_info_ods_minor_version      = 33;
const char gds_info_no_reserve             = 34;

/* Database Info Return Values */

const char gds_info_db_impl_rdb_vms        = 1;
const char gds_info_db_impl_rdb_eln        = 2;
const char gds_info_db_impl_rdb_eln_dev    = 3;
const char gds_info_db_impl_rdb_vms_y      = 4;
const char gds_info_db_impl_rdb_eln_y      = 5;
const char gds_info_db_impl_jri            = 6;
const char gds_info_db_impl_jsv            = 7;
const char gds_info_db_impl_isc_a          = 25;
const char gds_info_db_impl_isc_u          = 26;
const char gds_info_db_impl_isc_v          = 27;
const char gds_info_db_impl_isc_s          = 28;
const char gds_info_db_impl_isc_apl_68K    = 25;
const char gds_info_db_impl_isc_vax_ultr   = 26;
const char gds_info_db_impl_isc_vms        = 27;
const char gds_info_db_impl_isc_sun_68k    = 28;
const char gds_info_db_impl_isc_sun4       = 30;
const char gds_info_db_impl_isc_hp_ux      = 31;
const char gds_info_db_impl_isc_sun_386i   = 32;
const char gds_info_db_impl_isc_vms_orcl   = 33;
const char gds_info_db_impl_isc_rt_aix     = 35;
const char gds_info_db_impl_isc_mips_ult   = 36;
const char gds_info_db_impl_isc_apl_dn10   = 37;
const char gds_info_db_class_access        = 1;
const char gds_info_db_class_y_valve       = 2;
const char gds_info_db_class_rem_int       = 3;
const char gds_info_db_class_rem_srvr      = 4;
const char gds_info_db_class_pipe_int      = 7;
const char gds_info_db_class_pipe_srvr     = 8;
const char gds_info_db_class_sam_int       = 9;
const char gds_info_db_class_sam_srvr      = 10;
const char gds_info_db_class_gateway       = 11;

/* Request information items */

const char gds_info_number_messages        = 4;
const char gds_info_max_message            = 5;
const char gds_info_max_send               = 6;
const char gds_info_max_receive            = 7;
const char gds_info_state                  = 8;
const char gds_info_message_number         = 9;
const char gds_info_message_size           = 10;
const char gds_info_request_cost           = 11;
const char gds_info_req_active             = 2;
const char gds_info_req_inactive           = 3;
const char gds_info_req_send               = 4;
const char gds_info_req_receive            = 5;
const char gds_info_req_select             = 6;

/* Blob information items */

const char gds_info_blob_num_segments      = 4;
const char gds_info_blob_max_segment       = 5;
const char gds_info_blob_total_length      = 6;
const char gds_info_blob_type              = 7;

/* Transaction information items */

const char gds_info_tra_id                 = 4;

/* SQL information items */

const char gds_info_sql_select             = 4;
const char gds_info_sql_bind               = 5;
const char gds_info_sql_num_variables      = 6;
const char gds_info_sql_describe_vars      = 7;
const char gds_info_sql_describe_end       = 8;
const char gds_info_sql_sqlda_seq          = 9;
const char gds_info_sql_message_seq        = 10;
const char gds_info_sql_type               = 11;
const char gds_info_sql_sub_type           = 12;
const char gds_info_sql_scale              = 13;
const char gds_info_sql_length             = 14;
const char gds_info_sql_null_ind           = 15;
const char gds_info_sql_field              = 16;
const char gds_info_sql_relation           = 17;
const char gds_info_sql_owner              = 18;
const char gds_info_sql_alias              = 19;
const char gds_info_sql_sqlda_start        = 20;
const char gds_info_sql_stmt_type          = 21;

/* SQL information return values */

const char gds_info_sql_stmt_select        = 1;
const char gds_info_sql_stmt_insert        = 2;
const char gds_info_sql_stmt_update        = 3;
const char gds_info_sql_stmt_delete        = 4;
const char gds_info_sql_stmt_ddl           = 5;
const char gds_info_sql_stmt_commit        = 10;
const char gds_info_sql_stmt_rollback      = 11;
const char gds_info_sql_stmt_select_for_upd = 12;

/* Error codes */

const long gds_facility                    = 20;
const long gds_err_base                    = 335544320L;
const long gds_err_factor                  = 1;
const long gds_arg_end                     = 0;
const long gds_arg_gds                     = 1;
const long gds_arg_string                  = 2;
const long gds_arg_cstring                 = 3;
const long gds_arg_number                  = 4;
const long gds_arg_interpreted             = 5;
const long gds_arg_vms                     = 6;
const long gds_arg_unix                    = 7;
const long gds_arg_domain                  = 8;
const long gds_arg_dos                     = 9;
const long gds_arg_mpexl                   = 10;

const long gds_arith_except                = 335544321L;
const long gds_bad_dbkey                   = 335544322L;
const long gds_bad_db_format               = 335544323L;
const long gds_bad_db_handle               = 335544324L;
const long gds_bad_dpb_content             = 335544325L;
const long gds_bad_dpb_form                = 335544326L;
const long gds_bad_req_handle              = 335544327L;
const long gds_bad_segstr_handle           = 335544328L;
const long gds_bad_segstr_id               = 335544329L;
const long gds_bad_tpb_content             = 335544330L;
const long gds_bad_tpb_form                = 335544331L;
const long gds_bad_trans_handle            = 335544332L;
const long gds_bug_check                   = 335544333L;
const long gds_convert_error               = 335544334L;
const long gds_db_corrupt                  = 335544335L;
const long gds_deadlock                    = 335544336L;
const long gds_excess_trans                = 335544337L;
const long gds_from_no_match               = 335544338L;
const long gds_infinap                     = 335544339L;
const long gds_infona                      = 335544340L;
const long gds_infunk                      = 335544341L;
const long gds_integ_fail                  = 335544342L;
const long gds_invalid_blr                 = 335544343L;
const long gds_io_error                    = 335544344L;
const long gds_lock_conflict               = 335544345L;
const long gds_metadata_corrupt            = 335544346L;
const long gds_not_valid                   = 335544347L;
const long gds_no_cur_rec                  = 335544348L;
const long gds_no_dup                      = 335544349L;
const long gds_no_finish                   = 335544350L;
const long gds_no_meta_update              = 335544351L;
const long gds_no_priv                     = 335544352L;
const long gds_no_recon                    = 335544353L;
const long gds_no_record                   = 335544354L;
const long gds_no_segstr_close             = 335544355L;
const long gds_obsolete_metadata           = 335544356L;
const long gds_open_trans                  = 335544357L;
const long gds_port_len                    = 335544358L;
const long gds_read_only_field             = 335544359L;
const long gds_read_only_rel               = 335544360L;
const long gds_read_only_trans             = 335544361L;
const long gds_read_only_view              = 335544362L;
const long gds_req_no_trans                = 335544363L;
const long gds_req_sync                    = 335544364L;
const long gds_req_wrong_db                = 335544365L;
const long gds_segment                     = 335544366L;
const long gds_segstr_eof                  = 335544367L;
const long gds_segstr_no_op                = 335544368L;
const long gds_segstr_no_read              = 335544369L;
const long gds_segstr_no_trans             = 335544370L;
const long gds_segstr_no_write             = 335544371L;
const long gds_segstr_wrong_db             = 335544372L;
const long gds_sys_request                 = 335544373L;
const long gds_stream_eof                  = 335544374L;
const long gds_unavailable                 = 335544375L;
const long gds_unres_rel                   = 335544376L;
const long gds_uns_ext                     = 335544377L;
const long gds_wish_list                   = 335544378L;
const long gds_wrong_ods                   = 335544379L;
const long gds_wronumarg                   = 335544380L;
const long gds_imp_exc                     = 335544381L;
const long gds_random                      = 335544382L;
const long gds_fatal_conflict              = 335544383L;

/* Minor codes subject to change */

const long gds_badblk                      = 335544384L;
const long gds_invpoolcl                   = 335544385L;
const long gds_nopoolids                   = 335544386L;
const long gds_relbadblk                   = 335544387L;
const long gds_blktoobig                   = 335544388L;
const long gds_bufexh                      = 335544389L;
const long gds_syntaxerr                   = 335544390L;
const long gds_bufinuse                    = 335544391L;
const long gds_bdbincon                    = 335544392L;
const long gds_reqinuse                    = 335544393L;
const long gds_badodsver                   = 335544394L;
const long gds_relnotdef                   = 335544395L;
const long gds_fldnotdef                   = 335544396L;
const long gds_dirtypage                   = 335544397L;
const long gds_waifortra                   = 335544398L;
const long gds_doubleloc                   = 335544399L;
const long gds_nodnotfnd                   = 335544400L;
const long gds_dupnodfnd                   = 335544401L;
const long gds_locnotmar                   = 335544402L;
const long gds_badpagtyp                   = 335544403L;
const long gds_corrupt                     = 335544404L;
const long gds_badpage                     = 335544405L;
const long gds_badindex                    = 335544406L;
const long gds_dbbnotzer                   = 335544407L;
const long gds_tranotzer                   = 335544408L;
const long gds_trareqmis                   = 335544409L;
const long gds_badhndcnt                   = 335544410L;
const long gds_wrotpbver                   = 335544411L;
const long gds_wroblrver                   = 335544412L;
const long gds_wrodpbver                   = 335544413L;
const long gds_blobnotsup                  = 335544414L;
const long gds_badrelation                 = 335544415L;
const long gds_nodetach                    = 335544416L;
const long gds_notremote                   = 335544417L;
const long gds_trainlim                    = 335544418L;
const long gds_notinlim                    = 335544419L;
const long gds_traoutsta                   = 335544420L;
const long gds_connect_reject              = 335544421L;
const long gds_dbfile                      = 335544422L;
const long gds_orphan                      = 335544423L;
const long gds_no_lock_mgr                 = 335544424L;
const long gds_ctxinuse                    = 335544425L;
const long gds_ctxnotdef                   = 335544426L;
const long gds_datnotsup                   = 335544427L;
const long gds_badmsgnum                   = 335544428L;
const long gds_badparnum                   = 335544429L;
const long gds_virmemexh                   = 335544430L;
const long gds_blocking_signal             = 335544431L;
const long gds_lockmanerr                  = 335544432L;
const long gds_journerr                    = 335544433L;
const long gds_keytoobig                   = 335544434L;
const long gds_nullsegkey                  = 335544435L;
const long gds_sqlerr                      = 335544436L;
const long gds_wrodynver                   = 335544437L;
const long gds_funnotdef                   = 335544438L;
const long gds_funmismat                   = 335544439L;
const long gds_bad_msg_vec                 = 335544440L;
const long gds_bad_detach                  = 335544441L;
const long gds_noargacc_read               = 335544442L;
const long gds_noargacc_write              = 335544443L;
const long gds_read_only                   = 335544444L;
const long gds_ext_err                     = 335544445L;
const long gds_non_updatable               = 335544446L;
const long gds_no_rollback                 = 335544447L;
const long gds_bad_sec_info                = 335544448L;
const long gds_invalid_sec_info            = 335544449L;
const long gds_misc_interpreted            = 335544450L;
const long gds_update_conflict             = 335544451L;
const long gds_unlicensed                  = 335544452L;
const long gds_obj_in_use                  = 335544453L;
const long gds_nofilter                    = 335544454L;
const long gds_shadow_accessed             = 335544455L;
const long gds_invalid_sdl                 = 335544456L;
const long gds_out_of_bounds               = 335544457L;
const long gds_invalid_dimension           = 335544458L;
const long gds_rec_in_limbo                = 335544459L;
const long gds_shadow_missing              = 335544460L;
const long gds_cant_validate               = 335544461L;
const long gds_cant_start_journal          = 335544462L;
const long gds_gennotdef                   = 335544463L;
const long gds_cant_start_logging          = 335544464L;
const long gds_bad_segstr_type             = 335544465L;
const long gds_foreign_key                 = 335544466L;
const long gds_high_minor                  = 335544467L;
const long gds_tra_state                   = 335544468L;
const long gds_trans_invalid               = 335544469L;
const long gds_buf_invalid                 = 335544470L;

/* Dynamic Data Definition Language operators */

/* Version number */

const char gds_dyn_version_1               = 1;
const char gds_dyn_eoc                     = -1;

/* Operations (may be nested) */

const char gds_dyn_begin                   = 2;
const char gds_dyn_end                     = 3;
const char gds_dyn_if                      = 4;
const char gds_dyn_def_database            = 5;
const char gds_dyn_def_global_fld          = 6;
const char gds_dyn_def_local_fld           = 7;
const char gds_dyn_def_idx                 = 8;
const char gds_dyn_def_rel                 = 9;
const char gds_dyn_def_sql_fld             = 10;
const char gds_dyn_def_view                = 12;
const char gds_dyn_def_trigger             = 15;
const char gds_dyn_def_security_class      = 120;
const char gds_dyn_def_dimension           = 140;
const char gds_dyn_mod_rel                 = 11;
const char gds_dyn_mod_global_fld          = 13;
const char gds_dyn_mod_idx                 = 102;
const char gds_dyn_mod_local_fld           = 14;
const char gds_dyn_mod_view                = 16;
const char gds_dyn_mod_security_class      = 122;
const char gds_dyn_mod_trigger             = 113;
const char gds_dyn_delete_database         = 18;
const char gds_dyn_delete_rel              = 19;
const char gds_dyn_delete_global_fld       = 20;
const char gds_dyn_delete_local_fld        = 21;
const char gds_dyn_delete_idx              = 22;
const char gds_dyn_delete_security_class   = 123;
const char gds_dyn_delete_dimensions       = 143;
const char gds_dyn_grant                   = 30;
const char gds_dyn_revoke                  = 31;
const char gds_dyn_def_primary_key         = 37;
const char gds_dyn_def_foreign_key         = 38;

/* View specific stuff */

const char gds_dyn_view_blr                = 43;
const char gds_dyn_view_source             = 44;
const char gds_dyn_view_relation           = 45;
const char gds_dyn_view_context            = 46;
const char gds_dyn_view_context_name       = 47;

/* Generic attributes */

const char gds_dyn_rel_name                = 50;
const char gds_dyn_fld_name                = 51;
const char gds_dyn_idx_name                = 52;
const char gds_dyn_description             = 53;
const char gds_dyn_security_class          = 54;
const char gds_dyn_system_flag             = 55;
const char gds_dyn_update_flag             = 56;

/* Relation specific attributes */

const char gds_dyn_rel_dbkey_length        = 61;
const char gds_dyn_rel_store_trig          = 62;
const char gds_dyn_rel_modify_trig         = 63;
const char gds_dyn_rel_erase_trig          = 64;
const char gds_dyn_rel_store_trig_source   = 65;
const char gds_dyn_rel_modify_trig_source  = 66;
const char gds_dyn_rel_erase_trig_source   = 67;
const char gds_dyn_rel_ext_file            = 68;
const char gds_dyn_rel_sql_protection      = 69;

/* Global field specific attributes */

const char gds_dyn_fld_type                = 70;
const char gds_dyn_fld_length              = 71;
const char gds_dyn_fld_scale               = 72;
const char gds_dyn_fld_sub_type            = 73;
const char gds_dyn_fld_segment_length      = 74;
const char gds_dyn_fld_query_header        = 75;
const char gds_dyn_fld_edit_string         = 76;
const char gds_dyn_fld_validation_blr      = 77;
const char gds_dyn_fld_validation_source   = 78;
const char gds_dyn_fld_computed_blr        = 79;
const char gds_dyn_fld_computed_source     = 80;
const char gds_dyn_fld_missing_value       = 81;
const char gds_dyn_fld_default_value       = 82;
const char gds_dyn_fld_query_name          = 83;
const char gds_dyn_fld_dimensions          = 84;

/* Local field specific attributes */

const char gds_dyn_fld_source              = 90;
const char gds_dyn_fld_base_fld            = 91;
const char gds_dyn_fld_position            = 92;
const char gds_dyn_fld_update_flag         = 93;

/* Index specific attributes */

const char gds_dyn_idx_unique              = 100;
const char gds_dyn_idx_inactive            = 101;
const char gds_dyn_idx_type                = 103;
const char gds_dyn_idx_foreign_key         = 104;
const char gds_dyn_idx_ref_column          = 105;

/* Trigger specific attributes */

const char gds_dyn_trg_type                = 110;
const char gds_dyn_trg_blr                 = 111;
const char gds_dyn_trg_source              = 112;
const char gds_dyn_trg_name                = 114;
const char gds_dyn_trg_sequence            = 115;
const char gds_dyn_trg_inactive            = 116;
const char gds_dyn_trg_msg_number          = 117;
const char gds_dyn_trg_msg                 = 118;

/* Security Class specific attributes */

const char gds_dyn_scl_acl                 = 121;
const char gds_dyn_grant_user              = 130;
const char gds_dyn_grant_options           = 132;

/* Dimension specific information */

const char gds_dyn_dim_lower               = 141;
const char gds_dyn_dim_upper               = 142;

/* File specific attributes */

const char gds_dyn_file_name               = 125;
const char gds_dyn_file_start              = 126;
const char gds_dyn_file_length             = 127;
const char gds_dyn_shadow_number           = 128;
const char gds_dyn_shadow_man_auto         = 129;
const char gds_dyn_shadow_conditional      = 130;

/* Function specific attributes */

const char gds_dyn_function_name           = 145;
const char gds_dyn_function_type           = 146;
const char gds_dyn_func_module_name        = 147;
const char gds_dyn_func_entry_point        = 148;
const char gds_dyn_func_return_argument    = 149;
const char gds_dyn_func_arg_position       = 150;
const char gds_dyn_func_mechanism          = 151;
const char gds_dyn_filter_in_subtype       = 152;
const char gds_dyn_filter_out_subtype      = 153;

/* Generator specific attributes */

const char gds_dyn_generator_name          = 95;
const char gds_dyn_generator_id            = 96;

/* Array slice description language (SDL) */

const unsigned char gds_sdl_version1        = 1;
const unsigned char gds_sdl_eoc             = 0xFF;
const unsigned char gds_sdl_relation        = 2;
const unsigned char gds_sdl_rid             = 3;
const unsigned char gds_sdl_field           = 4;
const unsigned char gds_sdl_fid             = 5;
const unsigned char gds_sdl_struct          = 6;
const unsigned char gds_sdl_variable        = 7;
const unsigned char gds_sdl_scalar          = 8;
const unsigned char gds_sdl_tiny_integer    = 9;
const unsigned char gds_sdl_short_integer   = 10;
const unsigned char gds_sdl_long_integer    = 11;
const unsigned char gds_sdl_literal         = 12;
const unsigned char gds_sdl_add             = 13;
const unsigned char gds_sdl_subtract        = 14;
const unsigned char gds_sdl_multiply        = 15;
const unsigned char gds_sdl_divide          = 16;
const unsigned char gds_sdl_negate          = 17;
const unsigned char gds_sdl_eql             = 18;
const unsigned char gds_sdl_neq             = 19;
const unsigned char gds_sdl_gtr             = 20;
const unsigned char gds_sdl_geq             = 21;
const unsigned char gds_sdl_lss             = 22;
const unsigned char gds_sdl_leq             = 23;
const unsigned char gds_sdl_and             = 24;
const unsigned char gds_sdl_or              = 25;
const unsigned char gds_sdl_not             = 26;
const unsigned char gds_sdl_while           = 27;
const unsigned char gds_sdl_assignment      = 28;
const unsigned char gds_sdl_label           = 29;
const unsigned char gds_sdl_leave           = 30;
const unsigned char gds_sdl_begin           = 31;
const unsigned char gds_sdl_end             = 32;
const unsigned char gds_sdl_do3             = 33;
const unsigned char gds_sdl_do2             = 34;
const unsigned char gds_sdl_do1             = 35;
const unsigned char gds_sdl_element         = 36;

const short SQL_TEXT                        = 452;
const short SQL_VARYING                     = 448;
const short SQL_SHORT                       = 500;
const short SQL_LONG                        = 496;
const short SQL_FLOAT                       = 482;
const short SQL_DOUBLE                      = 480;
const short SQL_DATE                        = 510;
const short SQL_BLOB                        = 520;

