(*******************************************************************)
(*                                                                 *)
(* The contents of this file are subject to the Interbase Public   *)
(* License Version 1.0 (the "License"); you may not use this file  *)
(* except in compliance with the License. You may obtain a copy    *)
(* of the License at http://www.Inprise.com/IPL.html                 *)
(*                                                                 *)
(* Software distributed under the License is distributed on an     *)
(* "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express     *)
(* or implied. See the License for the specific language governing *)
(* rights and limitations under the License.                       *)
(*                                                                 *)
(* The Original Code was created by Inprise Corporation *)
(* and its predecessors. Portions created by Inprise Corporation are     *)
(* Copyright (C) Inprise Corporation. *)
(*                                                                 *)
(* All Rights Reserved.                                            *)
(* Contributor(s): ______________________________________.         *)
(*******************************************************************)

(*
 *	PROGRAM:	Preprocessor
 *	MODULE:		gds.pas
 *	DESCRIPTION:	GDS constants, procedures, etc.
 *)

type
    gds__handle	= ^integer32;
    gds__string	= array [1..65535] of char;
    gds__status_vector	= array [1..20] of integer32;
    gds__quad		= array [1..2] of integer32;
    gds__vector		= array [1..256] of integer32;
    gds__evb_t		= array [1..256, 1..31] of char;
    gds__teb_t	= record
		    dbb_ptr	: ^gds__handle;
		    tpb_len	: integer32;
		    tpb_ptr	: UNIV_PTR;                       
                 end;
    gds__tm	= record
		    tm_sec	: integer32;
		    tm_min	: integer32;
		    tm_hour	: integer32;
		    tm_mday	: integer32;
		    tm_mon	: integer32;
		    tm_year	: integer32;
		    tm_wday	: integer32;
		    tm_yday	: integer32;
		    tm_isdst	: integer32;
		end;
    sqlvar	= record                    
		    sqltype		: integer16;
		    sqllen		: integer16;
		    sqldata		: UNIV_PTR;
		    sqlind		: UNIV_PTR;
		    sqlname_length	: integer16;
		    sqlname		: array [1..30] of char;
		end;
    sqlda	= record
		    sqldaid		: array [1..8] of char;
		    sqldabc		: integer32;
		    sqln		: integer16;
		    sqld		: integer16;
		    sqlvars		: array [1..100] of sqlvar;
		end;
    xsqlvar	= record                    
		    sqltype		: integer16;
		    sqlscale		: integer16;
		    sqlsubtype		: integer16;
		    sqllen		: integer16;
		    sqldata		: UNIV_PTR;
		    sqlind		: UNIV_PTR;
		    sqlname_length	: integer16;
		    sqlname		: array [1..32] of char;
		    relname_length	: integer16;
		    relname		: array [1..32] of char;
		    ownname_length	: integer16;
		    ownname		: array [1..32] of char;
		    aliasname_length	: integer16;
		    aliasname		: array [1..32] of char;
		end;
    xsqlda	= record
		    version		: integer16;
		    sqldaid		: array [1..8] of char;
		    sqldabc		: integer32;
		    sqln		: integer16;
		    sqld		: integer16;
		    sqlvars		: array [1..100] of xsqlvar;
		end;

procedure GDS__SET_DEBUG (
        in	debug_val	: integer32
        ); extern;

procedure GDS__ATTACH_DATABASE (
	out	stat		: gds__status_vector; 
	in	name_length	: integer16;
	in	file_name	: UNIV gds__string; 
	in out	db_handle	: gds__handle;
	in	dpb_length	: integer16;
	in	dpb		: UNIV gds__string
	); extern;

procedure GDS__CANCEL_BLOB (
	out 	stat		: gds__status_vector; 
	in out	blob_handle	: gds__handle
	); extern;

procedure GDS__CANCEL_EVENTS (
        out     stat            : gds__status_vector;
        in out  db_handle       : gds__handle;
        in      id              : gds__handle
        ); extern;

procedure GDS__CLOSE_BLOB (
	out 	stat		: gds__status_vector; 
	in out	blob_handle	: gds__handle
	); extern;

procedure GDS__COMMIT_RETAINING (
	out	stat		: gds__status_vector; 
	in out	tra_handle	: gds__handle
	); extern;

procedure GDS__COMMIT_TRANSACTION (
	out	stat		: gds__status_vector; 
	in out	tra_handle	: gds__handle
	); extern;

procedure GDS__COMPILE_REQUEST (
	out	stat		: gds__status_vector; 
	in	db_handle	: gds__handle;
	in out	request_handle	: gds__handle;
	in	blr_length	: integer16;
	in	blr		: UNIV gds__string
	); extern;

procedure GDS__COMPILE_REQUEST2 (
	out	stat		: gds__status_vector; 
	in	db_handle	: gds__handle;
	in out	request_handle	: gds__handle;
	in	blr_length	: integer16;
	in	blr		: UNIV gds__string
	); extern;

procedure GDS__CREATE_BLOB (
	out 	stat		: gds__status_vector; 
	in	db_handle	: gds__handle;
	in	tra_handle	: gds__handle;
	in out	blob_handle	: gds__handle;
	out	blob_id		: gds__quad
	); extern;

procedure GDS__CREATE_BLOB2 (
        out     stat            : gds__status_vector;
        in      db_handle       : gds__handle;
        in      tra_handle      : gds__handle;
        in out  blob_handle     : gds__handle;
        out     blob_id         : gds__quad;
        in      bpb_length      : integer16;
        in      bpb             : UNIV gds__string
        ); extern;

procedure GDS__CREATE_DATABASE (
	out 	stat		: gds__status_vector; 
	in	name_length	: integer16;
	in	file_name	: UNIV gds__string; 
	in out	db_handle	: gds__handle;
	in	dpb_length	: integer16;
	in	dpb		: UNIV gds__string;
	in	dpb_type	: integer16
	); extern;

procedure GDS__DDL (
	out 	stat		: gds__status_vector; 
	in	db_handle	: gds__handle;
	in	tra_handle	: gds__handle;
	in	mblr_length	: integer16;
	in	mblr		: UNIV gds__string
	); extern;

procedure GDS__DETACH_DATABASE (
	out	stat		: gds__status_vector; 
	in out	db_handle	: gds__handle
	); extern;

procedure GDS__DROP_DATABASE (
	out 	stat		: gds__status_vector; 
	in	name_length	: integer16;
	in	file_name	: UNIV gds__string; 
	in	dbtype		: integer16
	); extern;

function GDS__EVENT_BLOCK_A (
        out     events          : UNIV_PTR;
        out     buffer          : UNIV_PTR;
        in      length          : integer16;
	in      names           : UNIV gds__evb_t
	) : integer16; extern;

procedure GDS__EVENT_COUNTS_ (
        out     counts          : UNIV gds__vector;
        in      length          : integer16;
        in      event_buffer    : UNIV_PTR;
        in      result_buffer   : UNIV_PTR
        ); extern;

procedure GDS__EVENT_WAIT_ (
        out     stat            : gds__status_vector;
        in      db_handle       : gds__handle;
        in      length          : integer16;
        in      events          : UNIV_PTR;
        in      buffer          : UNIV_PTR
        ); extern;

procedure GDS__GET_SLICE (
	out 	stat		: gds__status_vector; 
        in      db_handle       : gds__handle;
        in      tra_handle      : gds__handle;
	in	blob_id         : gds__quad;
        in      sdl_length      : integer16;
        in      sdl             : UNIV gds__string;
        in      param_length    : integer16;
        in      param           : UNIV gds__string;
        in      slice_length    : integer32;
        in      slice           : UNIV gds__string;
        in out  return_length   : integer32
	); extern;

function GDS__GET_SEGMENT (
	out 	stat		: gds__status_vector; 
	in	blob_handle	: gds__handle;
	out	length		: integer16;
	in	buffer_length	: integer16;
	out	buffer		: UNIV gds__string
	): integer32; extern;

procedure GDS__OPEN_BLOB (
	out 	stat		: gds__status_vector; 
	in	db_handle	: gds__handle;
	in	tra_handle	: gds__handle;
	in out	blob_handle	: gds__handle;
	in	blob_id		: gds__quad
	); extern;

procedure GDS__OPEN_BLOB2 (
	out 	stat		: gds__status_vector; 
	in	db_handle	: gds__handle;
	in	tra_handle	: gds__handle;
	in out	blob_handle	: gds__handle;
	in	blob_id		: gds__quad;
        in      bpb_length      : integer16;
        in      bpb             : UNIV gds__string
	); extern;

procedure GDS__PREPARE_TRANSACTION (
	out	stat		: gds__status_vector; 
	in out	tra_handle	: gds__handle
	); extern;

procedure GDS__PREPARE_TRANSACTION2 (
	out	stat		: gds__status_vector; 
	in out	tra_handle	: gds__handle;
        in      msg_length      : integer16;
        in      msg             : UNIV gds__string
	); extern;

function GDS__PUT_SEGMENT (
	out 	stat		: gds__status_vector; 
	in	blob_handle	: gds__handle;
	in	length		: integer16;
	out	buffer		: UNIV gds__string
	): integer32; extern;

procedure GDS__PUT_SLICE (
	out 	stat		: gds__status_vector; 
        in      db_handle       : gds__handle;
        in      tra_handle      : gds__handle;
	in	blob_id         : gds__quad;
        in      sdl_length      : integer16;
        in      sdl             : UNIV gds__string;
        in      param_length    : integer16;
        in      param           : UNIV gds__string;
        in      slice_length    : integer32;
        in      slice           : UNIV gds__string
	); extern;

procedure GDS__QUE_EVENTS (
        out     stat            : gds__status_vector;
        in      db_handle       : gds__handle;
        in      id              : gds__quad;
        in      length          : integer16;
        in      events          : UNIV gds__string;
        in      ast             : gds__handle;
        in      arg             : gds__quad
        ); extern;

procedure GDS__RECEIVE (
	out	stat		: gds__status_vector; 
	in	request_handle	: gds__handle;
	in	message_type	: integer16;
	in	message_length	: integer16;
	in out	message		: UNIV gds__string;
	in	instantiation	: integer16
	); extern;

procedure GDS__RELEASE_REQUEST (
	out	stat		: gds__status_vector; 
	in	request_handle	: gds__handle
	); extern;

procedure GDS__ROLLBACK_TRANSACTION (
	out	stat		: gds__status_vector; 
	in out	tra_handle	: gds__handle
	); extern;

procedure GDS__SEND (
	out	stat		: gds__status_vector; 
	in	request_handle	: gds__handle;
	in	message_type	: integer16;
	in	message_length	: integer16;
	in	message		: UNIV gds__string;
	in	instantiation	: integer16
	); extern;

procedure GDS__START_AND_SEND (
	out	stat		: gds__status_vector; 
	in	request_handle	: gds__handle;
	in	tra_handle	: gds__handle;
	in	message_type	: integer16;
	in	message_length	: integer16;
	in	message		: UNIV gds__string;
	in	instantiation	: integer16
	); extern;

procedure GDS__START_REQUEST (
	out	stat		: gds__status_vector; 
	in	request_handle	: gds__handle;
	in	tra_handle	: gds__handle;
	in	instantiation	: integer16
	); extern;

procedure GDS__START_MULTIPLE (
	out	stat		: gds__status_vector; 
	in out	tra_handle	: gds__handle;
	in	tra_count	: integer16;
	in	teb		: UNIV gds__string
	); extern;

procedure GDS__START_TRANSACTION (
	out	stat		: gds__status_vector; 
	in out	tra_handle	: gds__handle;
	in	tra_count	: integer16;
	in	db_handle	: gds__handle;
	in	tpb_length	: integer16;
	in	tpb		: UNIV gds__string
	); extern;

procedure GDS__UNWIND_REQUEST (
	out	stat		: gds__status_vector; 
	in	request_handle	: gds__handle;
	in	instantiation	: integer16
	); extern;

procedure isc_dsql_allocate_statement (
	out	stat		: gds__status_vector; 
	in	db_handle	: gds__handle;
	in out	stmt_handle	: gds__handle
	); extern;

procedure isc_dsql_alloc_statement2 (
	out	stat		: gds__status_vector; 
	in	db_handle	: gds__handle;
	in out	stmt_handle	: gds__handle
	); extern;

procedure isc_dsql_execute_m (
	out	stat		: gds__status_vector; 
	in out	tra_handle	: gds__handle;
	in out	stmt_handle	: gds__handle;
	in	blr_length	: integer32;
	in	blr		: UNIV gds__string;
	in	msg_type	: integer32;
	in	msg_length	: integer32;
	in	msg		: UNIV gds__string
	); extern; val_param;

procedure isc_dsql_execute2_m (
	out	stat		: gds__status_vector; 
	in out	tra_handle	: gds__handle;
	in out	stmt_handle	: gds__handle;
	in	in_blr_length	: integer32;
	in	in_blr		: UNIV gds__string;
	in	in_msg_type	: integer32;
	in	in_msg_length	: integer32;
	in	in_msg		: UNIV gds__string;
	in	out_blr_length	: integer32;
	in	out_blr		: UNIV gds__string;
	in	out_msg_type	: integer32;
	in	out_msg_length	: integer32;
	in out	out_msg		: UNIV gds__string
	); extern; val_param;

procedure isc_dsql_free_statement (
	out	stat		: gds__status_vector; 
	in out	stmt_handle	: gds__handle;
	in	option		: integer32
	); extern; val_param;

procedure isc_dsql_set_cursor_name (
	out	stat		: gds__status_vector; 
	in out	stmt_handle	: gds__handle;
	in	cursor_name	: UNIV gds__string;
	in	cursor_type	: integer32
	); extern; val_param;

procedure gds__ftof (
	in	string1		: UNIV gds__string;
	in	length1		: integer16;
	in	string2		: UNIV gds__string;
	in	length2		: integer16
	); extern;

procedure gds__print_status (
	in	stat		: gds__status_vector
	); extern;

function gds__sqlcode (
	in	stat		: gds__status_vector
	) : integer32; extern;


procedure gds__encode_date (
	in	times		: gds__tm;
	out	date 		: gds__quad
    ); extern;

procedure gds__decode_date (
	in	date 		: gds__quad;
	out	times		: gds__tm 
    ); extern;

procedure isc_exec_procedure (
	out	stat		: gds__status_vector; 
	in out	db_handle	: gds__handle;
	in out	tra_handle	: gds__handle;
	in	name_length	: integer32;
	in	proc_name	: UNIV gds__string; 
	in	blr_length	: integer32;
	in	blr		: UNIV gds__string;
	in	in_msg_length	: integer32;
	in	in_msg		: UNIV gds__string;
	in	out_msg_length	: integer32;
	in out	out_msg		: UNIV gds__string
	); extern; val_param;

const
    gds__true	= 1;
    gds__false  = 0;

    blr_text = chr(14);                                                
    blr_short = chr(7);
    blr_long = chr(8);
    blr_quad = chr(9);
    blr_float = chr(10);
    blr_double = chr(27);
    blr_d_float = chr(11);
    blr_date = chr(35);
    blr_varying = chr(37);
    blr_cstring = chr(40);
    blr_blob = chr(261);
    blr_version4 = chr(4);
    blr_eoc = chr(76);
    blr_end = chr(255);
    blr_blob_id = chr(45);

    blr_inner = chr(0);
    blr_left = chr(1);
    blr_right = chr(2);
    blr_full = chr(3);

    blr_gds_code = chr(0);
    blr_sql_code = chr(1);
    blr_exception = chr(2);
    blr_trigger_code = chr(3);
    blr_default_code = chr(4);

    blr_assignment = chr(1);
    blr_begin = chr(2);
    blr_message = chr(4);
    blr_erase = chr(5);
    blr_fetch = chr(6);
    blr_for = chr(7);
    blr_if = chr(8);
    blr_loop = chr(9);
    blr_modify = chr(10);
    blr_handler = chr(11);
    blr_receive = chr(12);
    blr_select = chr(13);
    blr_send = chr(14);
    blr_store = chr(15);
    blr_label = chr(17);
    blr_leave = chr(18);
    blr_literal = chr(21);
    blr_dbkey = chr(22);
    blr_field = chr(23);
    blr_fid = chr(24);
    blr_parameter = chr(25);
    blr_variable = chr(26);
    blr_average = chr(27);
    blr_count = chr(28);
    blr_maximum = chr(29);
    blr_minimum = chr(30);
    blr_total = chr(31);

    blr_add = chr(34);
    blr_subtract = chr(35);
    blr_multiply = chr(36);
    blr_divide = chr(37);
    blr_negate = chr(38);
    blr_concatenate = chr(39);
    blr_substring = chr(40);
    blr_parameter2 = chr(41);
    blr_from = chr(42);
    blr_via = chr(43);
    blr_null = chr(45);

    blr_eql = chr(47);
    blr_neq = chr(48);
    blr_gtr = chr(49);
    blr_geq = chr(50);
    blr_lss = chr(51);
    blr_leq = chr(52);
    blr_containing = chr(53);
    blr_matching = chr(54);
    blr_starting = chr(55);
    blr_between = chr(56);
    blr_or = chr(57);
    blr_and = chr(58);
    blr_not = chr(59);
    blr_any = chr(60);
    blr_missing = chr(61);
    blr_unique = chr(62);
    blr_like = chr(63);

    blr_stream = chr(65);
    blr_set_index = chr(66);

    blr_rse = chr(67);
    blr_first = chr(68);
    blr_project = chr(69);
    blr_sort = chr(70);
    blr_boolean = chr(71);
    blr_ascending = chr(72);
    blr_descending = chr(73);
    blr_relation = chr(74);
    blr_rid = chr(75);
    blr_union = chr(76);
    blr_map = chr(77);
    blr_group_by = chr(78);
    blr_aggregate = chr(79);
    blr_join_type = chr(80);

    blr_agg_count = chr(83);
    blr_agg_max	= chr(84);
    blr_agg_min	= chr(85);
    blr_agg_total = chr(86);
    blr_agg_average = chr(87);
    blr_run_count = chr(118);
    blr_run_max	= chr(89);
    blr_run_min	= chr(90);
    blr_run_total = chr(91);
    blr_run_average = chr(92);
    blr_upcase = chr(103);
    blr_lock_state = chr(104);
    blr_value_if = chr(105);
    blr_matching2 = chr(106);
    blr_index = chr(107);
    blr_ansi_like = chr(108);
    blr_bookmark = chr(109);
    blr_crack = chr(110);
    blr_force_crack = chr(111);
    blr_seek = chr(112);
    blr_find = chr(113);
    blr_lock_relation = chr(114);
    blr_lock_record = chr(115);
    blr_set_bookmark = chr(116);
    blr_get_bookmark = chr(117);

    blr_parameter3 = chr(88);
    blr_rs_stream = chr(119);
    blr_exec_proc = chr(120);

    blr_begin_range = chr(121);
    blr_end_range = chr(122);
    blr_delete_range = chr(123);
    blr_procedure = chr(124);
    blr_pid = chr(125);
    blr_exec_pid = chr(126);
    blr_singular = chr(127);
    blr_abort = chr(128);
    blr_block = chr(129);
    blr_error_handler = chr(130);
    blr_cast = chr(131);


(* Database parameter block stuff *)

     gds__dpb_version1		= chr (1);

     gds__dpb_cdd_pathname	= chr (1);	(* not implemented *)
     gds__dpb_allocation	= chr (2);	(* not implemented *)
     gds__dpb_journal 		= chr (3);
     gds__dpb_page_size 	= chr (4);
     gds__dpb_num_buffers 	= chr (5);
     gds__dpb_buffer_length	= chr (6);	(* not implemented *)
     gds__dpb_debug 		= chr (7);	(* not implemented *)
     gds__dpb_garbage_collect	= chr (8);	(* not implemented *)
     gds__dpb_verify 		= chr (9);
     gds__dpb_sweep 		= chr (10);
     gds__dpb_enable_journal	= chr (11);
     gds__dpb_disable_journal	= chr (12);
     gds__dpb_dbkey_scope	= chr (13);	(* not implemented *)
     gds__dpb_number_of_users	= chr (14);	(* not implemented *)
     gds__dpb_trace		= chr (15);	(* not implemented *)
     gds__dpb_no_garbage_collect = chr (16);	(* not implemented *)
     gds__dpb_damaged		= chr (17);	(* not implemented *)
     gds__dpb_license           = chr (18);
     gds__dpb_sys_user_name     = chr (19);
     gds__dpb_encrypt_key       = chr (20);
     gds__dpb_activate_shadow   = chr (21);
     gds__dpb_sweep_interval    = chr (22);
     gds__dpb_delete_shadow     = chr (23);
     gds__dpb_force_write       = chr (24);
     gds__dpb_begin_log         = chr (25);
     gds__dpb_quit_log          = chr (26);
     gds__dpb_no_reserve        = chr (27);
     gds__dpb_user_name         = chr (28);
     gds__dpb_password          = chr (29);
     gds__dpb_password_enc      = chr (30);
     gds__dpb_sys_user_name_enc = chr (31);

     gds__dpb_pages		= chr(1);
     gds__dpb_records 		= chr(2);
     gds__dpb_indices 		= chr(4);
     gds__dpb_transactions 	= chr(8);
     gds__dpb_no_update 	= chr(16);
     gds__dpb_repair 		= chr(32);
     gds__dpb_ignore 		= chr(64);



(* Transaction parameter block stuff *)

    gds__tpb_version3	 = chr (3);
    gds__tpb_consistency = chr (1);
    gds__tpb_concurrency = chr (2);
    gds__tpb_shared	 = chr (3);
    gds__tpb_protected	 = chr (4);
    gds__tpb_exclusive	 = chr (5);
    gds__tpb_wait	 = chr (6);
    gds__tpb_nowait	 = chr (7);
    gds__tpb_read	 = chr (8);
    gds__tpb_write	 = chr (9);
    gds__tpb_lock_read	 = chr (10);
    gds__tpb_lock_write	 = chr (11);
    gds__tpb_verb_time	 = chr (12);
    gds__tpb_commit_time = chr (13);
    gds__tpb_ignore_limbo = chr (14);

(* Blob parameter block stuff *)

     gds__bpb_version1		= chr (1);
     gds__bpb_source_type	= chr (1);
     gds__bpb_target_type	= chr (2);
     gds__bpb_type		= chr (3);

     gds__bpb_type_segmented	= chr (0);
     gds__bpb_type_stream	= chr (1);

(* Blob routine declarations *)

type
    gds__field_name	= array [1..31] of char;
    gds__file_name	= array [1..128] of char;

procedure blob__display (
	in	blob_id		: gds__quad;
	in 	db_handle	: gds__handle;
	in 	tra_handle	: gds__handle;
        in	field_name	: univ gds__field_name;
        in      name_length	: integer16
	); extern;

procedure blob__dump (
	in	blob_id		: gds__quad;
	in 	db_handle	: gds__handle;
	in 	tra_handle	: gds__handle;
        in	file_name	: univ gds__file_name;
        in      name_length	: integer16
	); extern;

procedure blob__edit (
	in	blob_id		: gds__quad;
	in 	db_handle	: gds__handle;
	in 	tra_handle	: gds__handle;
        in	field_name	: univ gds__field_name;
        in      name_length	: integer16
	); extern;

procedure blob__load (
	in	blob_id		: gds__quad;
	in 	db_handle	: gds__handle;
	in 	tra_handle	: gds__handle;
        in	file_name	: univ gds__file_name;
        in      name_length	: integer16
	); extern;

(* Dynamic SQL procedures *)

procedure gds__close (
	out	stat		: gds__status_vector; 
	in	cursor_name	: UNIV gds__string
	); extern;

procedure gds__declare (
	out	stat		: gds__status_vector; 
	in	statement_name	: UNIV gds__string;
	in	cursor_name	: UNIV gds__string
	); extern;

procedure gds__describe (
	out	stat		: gds__status_vector; 
	in	statement_name	: UNIV gds__string;
	in out	descriptor	: UNIV sqlda
	); extern;

procedure gds__describe_bind (
	out	stat		: gds__status_vector; 
	in	statement_name	: UNIV gds__string;
	in out	descriptor	: UNIV sqlda
	); extern;

procedure gds__dsql_finish (
	in	db_handle	: gds__handle
	); extern;

procedure gds__execute (
	out	stat		: gds__status_vector; 
	in	trans_handle	: gds__handle;
	in	statement_name	: UNIV gds__string;
	in out	descriptor	: UNIV sqlda
	); extern;

procedure gds__execute_immediate (
	out	stat		: gds__status_vector; 
	in	db_handle	: gds__handle;
	in	trans_handle	: gds__handle;
	in	string_length	: integer;
	in	string		: UNIV gds__string
	); extern;

function gds__fetch (
	out	stat		: gds__status_vector; 
	in	cursor_name	: UNIV gds__string;
	in out	descriptor	: UNIV sqlda
	) : integer32; extern;

procedure gds__open (
	out	stat		: gds__status_vector; 
	in	trans_handle	: gds__handle;
	in	cursor_name	: UNIV gds__string;
	in out	descriptor	: UNIV sqlda
	); extern;

procedure gds__prepare (
	out	stat		: gds__status_vector; 
	in	db_handle	: gds__handle;
	in	trans_handle	: gds__handle;
	in	statement_name	: UNIV gds__string;
	in	string_length	: integer;
	in	string		: UNIV gds__string;
	in out	descriptor	: UNIV sqlda
	); extern;

procedure isc_embed_dsql_close (
	out	stat		: gds__status_vector; 
	in	cursor_name	: UNIV gds__string
	); extern;

procedure isc_embed_dsql_declare (
	out	stat		: gds__status_vector; 
	in	statement_name	: UNIV gds__string;
	in	cursor_name	: UNIV gds__string
	); extern;

procedure isc_embed_dsql_describe (
	out	stat		: gds__status_vector; 
	in	statement_name	: UNIV gds__string;
	in	dialect		: integer32;
	in out	descriptor	: UNIV sqlda
	); extern; val_param;

procedure isc_embed_dsql_describe_bind (
	out	stat		: gds__status_vector; 
	in	statement_name	: UNIV gds__string;
	in	dialect		: integer32;
	in out	descriptor	: UNIV sqlda
	); extern; val_param;

procedure isc_embed_dsql_execute (
	out	stat		: gds__status_vector; 
	in out	trans_handle	: gds__handle;
	in	statement_name	: UNIV gds__string;
	in	dialect		: integer32;
	in out	descriptor	: UNIV sqlda
	); extern; val_param;

procedure isc_embed_dsql_execute2 (
	out	stat		: gds__status_vector; 
	in out	trans_handle	: gds__handle;
	in	statement_name	: UNIV gds__string;
	in	dialect		: integer32;
	in out	in_descriptor	: UNIV sqlda;
	in out	out_descriptor	: UNIV sqlda
	); extern; val_param;

procedure isc_embed_dsql_execute_immed (
	out	stat		: gds__status_vector; 
	in out	db_handle	: gds__handle;
	in out	trans_handle	: gds__handle;
	in	string_length	: integer32;
	in	string		: UNIV gds__string;
	in	dialect		: integer32;
	in out	descriptor	: UNIV sqlda
	); extern; val_param;

function isc_embed_dsql_fetch (
	out	stat		: gds__status_vector; 
	in	cursor_name	: UNIV gds__string;
	in	dialect		: integer32;
	in out	descriptor	: UNIV sqlda
	) : integer32; extern; val_param;

procedure isc_embed_dsql_open (
	out	stat		: gds__status_vector; 
	in out	trans_handle	: gds__handle;
	in	cursor_name	: UNIV gds__string;
	in	dialect		: integer32;
	in out	descriptor	: UNIV sqlda
	); extern; val_param;

procedure isc_embed_dsql_prepare (
	out	stat		: gds__status_vector; 
	in out	db_handle	: gds__handle;
	in out	trans_handle	: gds__handle;
	in	statement_name	: UNIV gds__string;
	in	string_length	: integer32;
	in	string		: UNIV gds__string;
	in	dialect		: integer32;
	in out	descriptor	: UNIV sqlda
	); extern; val_param;

procedure isc_embed_dsql_release (
	out	stat		: gds__status_vector; 
	in	statement_name	: UNIV gds__string
	); extern;

(* Information parameters *)

(* Common, structural codes *)
const
    gds__info_end		= chr(1);
    gds__info_truncated		= chr(2);
    gds__info_error		= chr(3);

(* Database information items *)

    gds__info_db_id		= chr(4);
    gds__info_reads		= chr(5);
    gds__info_writes		= chr(6);
    gds__info_fetches		= chr(7);
    gds__info_marks		= chr(8);
    gds__info_implementation 	= chr(11);
    gds__info_version		= chr(12);
    gds__info_base_level	= chr(13);
    gds__info_page_size		= chr(14);
    gds__info_num_buffers	= chr(15);
    gds__info_limbo		= chr(16);
    gds__info_current_memory	= chr(17);
    gds__info_max_memory	= chr(18);
    gds__info_window_turns	= chr(19);
    gds__info_license           = chr(20);
    gds__info_allocation        = chr(21);
    gds__info_attachment_id     = chr(22);
    gds__info_read_seq_count    = chr(23);
    gds__info_read_idx_count    = chr(24);
    gds__info_insert_count      = chr(25);
    gds__info_update_count      = chr(26);
    gds__info_delete_count      = chr(27);
    gds__info_backout_count     = chr(28);
    gds__info_purge_count       = chr(29);
    gds__info_expunge_count     = chr(30);
    gds__info_sweep_interval    = chr(31);
    gds__info_ods_version       = chr(32);
    gds__info_ods_minor_version = chr(33);
    gds__info_no_reserve        = chr(34);


(* Request information items *)

    gds__info_number_messages	= chr(4);
    gds__info_max_message	= chr(5);
    gds__info_max_send		= chr(6);
    gds__info_max_receive	= chr(7);
    gds__info_state		= chr(8);
    gds__info_message_number	= chr(9);
    gds__info_message_size	= chr(10);
    gds__info_request_cost	= chr(11);

    gds__info_req_active	= chr(2);
    gds__info_req_inactive	= chr(3);
    gds__info_req_send		= chr(4);
    gds__info_req_receive	= chr(5);
    gds__info_req_select	= chr(6);

(* Blob information items *)

    gds__info_blob_num_segments	= chr(4);
    gds__info_blob_max_segment	= chr(5);
    gds__info_blob_total_length	= chr(6);
    gds__info_blob_type		= chr(7);

(* Transaction information items *)

    gds__info_tra_id		= chr(4);

(* SQL information items *)

    gds__info_sql_select        = chr(4);
    gds__info_sql_bind          = chr(5);
    gds__info_sql_num_variables = chr(6);
    gds__info_sql_describe_vars = chr(7);
    gds__info_sql_describe_end  = chr(8);
    gds__info_sql_sqlda_seq     = chr(9);
    gds__info_sql_message_seq   = chr(10);
    gds__info_sql_type          = chr(11);
    gds__info_sql_sub_type      = chr(12);
    gds__info_sql_scale         = chr(13);
    gds__info_sql_length        = chr(14);
    gds__info_sql_null_ind      = chr(15);
    gds__info_sql_field         = chr(16);
    gds__info_sql_relation      = chr(17);
    gds__info_sql_owner         = chr(18);
    gds__info_sql_alias         = chr(19);

(* Error codes *)

const
	gds_facility		= 20;
	gds_err_base		= 335544320;
	gds_err_factor		= 1;
	gds_arg_end		= 0;	(* end of argument list *)
	gds_arg_gds		= 1;	(* generic DSRI status value *)
	gds_arg_string		= 2;	(* string argument *)
	gds_arg_cstring		= 3;	(* count & string argument *)
	gds_arg_number		= 4;	(* numeric argument (long) *)
	gds_arg_interpreted	= 5;	(* interpreted status code (string) *)
	gds_arg_vms		= 6;	(* VAX/VMS status code (long) *)
	gds_arg_unix		= 7;	(* UNIX error code *)
	gds_arg_domain		= 8;	(* Apollo/Domain error code *)
	gds_arg_dos		= 9;	(* DOS error code *)
	gds_arg_mpexl		= 10;	(* MPE XL error code *)
	gds_arg_mpexl_ipc	= 11;	(* MPE XL (IPC) error code *)

	gds__arith_except         = 335544321;
	gds__bad_dbkey            = 335544322;
	gds__bad_db_format        = 335544323;
	gds__bad_db_handle        = 335544324;
	gds__bad_dpb_content      = 335544325;
	gds__bad_dpb_form         = 335544326;
	gds__bad_req_handle       = 335544327;
	gds__bad_segstr_handle    = 335544328;
	gds__bad_segstr_id        = 335544329;
	gds__bad_tpb_content      = 335544330;
	gds__bad_tpb_form         = 335544331;
	gds__bad_trans_handle     = 335544332;
	gds__bug_check            = 335544333;
	gds__convert_error        = 335544334;
	gds__db_corrupt           = 335544335;
	gds__deadlock             = 335544336;
	gds__excess_trans         = 335544337;
	gds__from_no_match        = 335544338;
	gds__infinap              = 335544339;
	gds__infona               = 335544340;
	gds__infunk               = 335544341;
	gds__integ_fail           = 335544342;
	gds__invalid_blr          = 335544343;
	gds__io_error             = 335544344;
	gds__lock_conflict        = 335544345;
	gds__metadata_corrupt     = 335544346;
	gds__not_valid            = 335544347;
	gds__no_cur_rec           = 335544348;
	gds__no_dup               = 335544349;
	gds__no_finish            = 335544350;
	gds__no_meta_update       = 335544351;
	gds__no_priv              = 335544352;
	gds__no_recon             = 335544353;
	gds__no_record            = 335544354;
	gds__no_segstr_close      = 335544355;
	gds__obsolete_metadata    = 335544356;
	gds__open_trans           = 335544357;
	gds__port_len             = 335544358;
	gds__read_only_field      = 335544359;
	gds__read_only_rel        = 335544360;
	gds__read_only_trans      = 335544361;
	gds__read_only_view       = 335544362;
	gds__req_no_trans         = 335544363;
	gds__req_sync             = 335544364;
	gds__req_wrong_db         = 335544365;
	gds__segment              = 335544366;
	gds__segstr_eof           = 335544367;
	gds__segstr_no_op         = 335544368;
	gds__segstr_no_read       = 335544369;
	gds__segstr_no_trans      = 335544370;
	gds__segstr_no_write      = 335544371;
	gds__segstr_wrong_db      = 335544372;
	gds__sys_request          = 335544373;
	gds__unavailable          = 335544375;
	gds__unres_rel            = 335544376;
	gds__uns_ext              = 335544377;
	gds__wish_list            = 335544378;
	gds__wrong_ods            = 335544379;
	gds__wronumarg            = 335544380;
	gds__imp_exc              = 335544381;
	gds__random               = 335544382;
	gds__fatal_conflict       = 335544383;

(* Minor codes subject to change *)

	gds__badblk               = 335544384;
	gds__invpoolcl            = 335544385;
	gds__nopoolids            = 335544386;
	gds__relbadblk            = 335544387;
	gds__blktoobig            = 335544388;
	gds__bufexh               = 335544389;
	gds__syntaxerr            = 335544390;
	gds__bufinuse             = 335544391;
	gds__bdbincon             = 335544392;
	gds__reqinuse             = 335544393;
	gds__badodsver            = 335544394;
	gds__relnotdef            = 335544395;
	gds__fldnotdef            = 335544396;
	gds__dirtypage            = 335544397;
	gds__waifortra            = 335544398;
	gds__doubleloc            = 335544399;
	gds__nodnotfnd            = 335544400;
	gds__dupnodfnd            = 335544401;
	gds__locnotmar            = 335544402;
	gds__badpagtyp            = 335544403;
	gds__corrupt              = 335544404;
	gds__badpage              = 335544405;
	gds__badindex             = 335544406;
	gds__dbbnotzer            = 335544407;
	gds__tranotzer            = 335544408;
	gds__trareqmis            = 335544409;
	gds__badhndcnt            = 335544410;
	gds__wrotpbver            = 335544411;
	gds__wroblrver            = 335544412;
	gds__wrodpbver            = 335544413;
	gds__blobnotsup           = 335544414;
	gds__badrelation          = 335544415;
	gds__nodetach             = 335544416;
	gds__notremote            = 335544417;
	gds__trainlim             = 335544418;
	gds__notinlim             = 335544419;
	gds__traoutsta            = 335544420;
	gds__connect_reject       = 335544421;
	gds__dbfile               = 335544422;
	gds__orphan               = 335544423;
	gds__no_lock_mgr          = 335544424;
	gds__ctxinuse             = 335544425;
	gds__ctxnotdef            = 335544426;
	gds__datnotsup            = 335544427;
	gds__badmsgnum            = 335544428;
	gds__badparnum            = 335544429;
	gds__virmemexh            = 335544430;
	gds__blocking_signal      = 335544431;
	gds__lockmanerr           = 335544432;
	gds__journerr             = 335544433;
	gds__keytoobig            = 335544434;
	gds__nullsegkey           = 335544435;
	gds__sqlerr               = 335544436;
	gds__wrodynver            = 335544437;
        gds__obj_in_use           = 335544453;
        gds__nofilter             = 335544454;
        gds__shadow_accessed      = 335544455;
        gds__invalid_sdl          = 335544456;
        gds__out_of_bounds        = 335544457;
        gds__invalid_dimension    = 335544458;
        gds__rec_in_limbo         = 335544459;
        gds__shadow_missing       = 335544460;
        gds__cant_validate        = 335544461;
        gds__cant_start_journal   = 335544462;
        gds__gennotdef            = 335544463;
        gds__cant_start_logging   = 335544464;
        gds__bad_segstr_type      = 335544465;
        gds__foreign_key          = 335544466;
        gds__high_minor           = 335544467;
        gds__tra_state            = 335544468;
        gds__trans_invalid        = 335544469;
        gds__buf_invalid          = 335544470;
        gds__indexnotdefined      = 335544471;
        gds__login                = 335544472;
        gds__invalid_bookmark     = 335544473;
        gds__bad_lock_level       = 335544474;
        gds__relation_lock        = 335544475;
        gds__record_lock          = 335544476;
        gds__max_idx              = 335544477;
        gds__jrn_enable           = 335544478;
        gds__old_failure          = 335544479;
        gds__old_in_progress      = 335544480;
        gds__old_no_space         = 335544481;
        gds__no_wal_no_jrn        = 335544482;
        gds__num_old_files        = 335544483;
        gds__wal_file_open        = 335544484;
        gds__bad_stmt_handle      = 335544485;
        gds__wal_failure          = 335544486;
        gds__walw_err             = 335544487;
        gds__logh_small           = 335544488;
        gds__logh_inv_version     = 335544489;
        gds__logh_open_flag       = 335544490;
        gds__logh_open_flag2      = 335544491;
        gds__logh_diff_dbname     = 335544492;
        gds__logf_unexpected_eof  = 335544493;
        gds__logr_incomplete      = 335544494;
        gds__logr_header_small    = 335544495;
        gds__logb_small           = 335544496;
        gds__wal_illegal_attach   = 335544497;
        gds__wal_invalid_wpb      = 335544498;
        gds__wal_err_rollover     = 335544499;
        gds__no_wal               = 335544500;
        gds__drop_wal             = 335544501;
        gds__stream_not_defined   = 335544502;
        gds__unknown_183          = 335544503;
        gds__wal_subsys_error     = 335544504;
        gds__wal_subsys_corrupt   = 335544505;
        gds__no_archive           = 335544506;
        gds__store_proc_failed    = 335544507;
        gds__range_in_use         = 335544508;
        gds__range_not_found      = 335544509;
        gds__charset_not_found    = 335544510;
        gds__lock_timeout         = 335544511;
        gds__prcnotdef            = 335544512;
        gds__prcmismat            = 335544513;
        gds_err_max               = 193;

(* Dynamic Data Definition Language operators *)

(* Version number *)

	gds__dyn_version_1		= chr(1);
	gds__dyn_eoc			= chr(255);

(* Operations (may be nested) *)

	gds__dyn_begin			= chr(2);
	gds__dyn_end			= chr(3);
	gds__dyn_if			= chr(4);

	gds__dyn_def_database		= chr(5);
	gds__dyn_def_global_fld		= chr(6);
	gds__dyn_def_local_fld		= chr(7);
	gds__dyn_def_idx		= chr(8);
	gds__dyn_def_rel		= chr(9);
	gds__dyn_def_sql_fld		= chr(10);
	gds__dyn_def_view		= chr(12);
	gds__dyn_def_trigger		= chr(15);
	gds__dyn_def_security_class	= chr(120);
	gds__dyn_def_dimension		= chr(140);

	gds__dyn_mod_rel		= chr(11);
	gds__dyn_mod_global_fld		= chr(13);
	gds__dyn_mod_idx		= chr(102);
	gds__dyn_mod_local_fld		= chr(14);
	gds__dyn_mod_view		= chr(16);
	gds__dyn_mod_database		= chr(39);
	gds__dyn_mod_security_class	= chr(122);
	gds__dyn_mod_trigger		= chr(113);

	gds__dyn_delete_database	= chr(18);
	gds__dyn_delete_rel		= chr(19);
	gds__dyn_delete_global_fld	= chr(20);
	gds__dyn_delete_local_fld	= chr(21);
	gds__dyn_delete_idx		= chr(22);
	gds__dyn_delete_security_class	= chr(123);
	gds__dyn_delete_dimensions	= chr(143);

        gds__dyn_grant                  = chr(30);
        gds__dyn_revoke                 = chr(31);
        gds__dyn_def_primary_key        = chr(37);
        gds__dyn_def_foreign_key        = chr(38);
        gds__dyn_def_unique             = chr(40);
        gds__dyn_def_procedure          = chr(164);
        gds__dyn_delete_procedure       = chr(165);
        gds__dyn_def_parameter          = chr(135);
        gds__dyn_delete_parameter       = chr(136);
        gds__dyn_def_intl_info          = chr(172);
        gds__dyn_mod_procedure          = chr(175);

        gds__dyn_def_exception		= chr(181);
        gds__dyn_mod_exception		= chr(182);
        gds__dyn_del_exception		= chr(183);

        gds__dyn_xcp_msg2		= chr(184);
        gds__dyn_xcp_msg		= chr(185);

        gds__dyn_grant_proc		= chr(186);
        gds__dyn_grant_trig		= chr(187);
        gds__dyn_grant_view		= chr(188);

(* View specific stuff *)

	gds__dyn_view_blr		= chr(43);
	gds__dyn_view_source		= chr(44);
	gds__dyn_view_relation		= chr(45);
	gds__dyn_view_context		= chr(46);
	gds__dyn_view_context_name	= chr(47);

(* Generic attributes *)

	gds__dyn_rel_name		= chr(50);
	gds__dyn_fld_name		= chr(51);
	gds__dyn_idx_name		= chr(52);
	gds__dyn_description		= chr(53);
	gds__dyn_security_class		= chr(54);
	gds__dyn_system_flag		= chr(55);
	gds__dyn_update_flag		= chr(56);
	gds__dyn_prc_name		= chr(166);
	gds__dyn_prm_name		= chr(137);

(* Relation specific attributes *)

	gds__dyn_rel_dbkey_length	= chr(61);
	gds__dyn_rel_store_trig		= chr(62);
	gds__dyn_rel_modify_trig	= chr(63);
	gds__dyn_rel_erase_trig		= chr(64);
	gds__dyn_rel_store_trig_source	= chr(65);
	gds__dyn_rel_modify_trig_source	= chr(66);
	gds__dyn_rel_erase_trig_source	= chr(67);
	gds__dyn_rel_erase_ext_file	= chr(68);
        gds__dyn_rel_sql_protection     = chr(69);
        gds__dyn_rel_constraint         = chr(162);
        gds__dyn_delete_rel_constraint  = chr(163);

(* Global field specific attributes *)

	gds__dyn_fld_type		= chr(70);
	gds__dyn_fld_length		= chr(71);
	gds__dyn_fld_scale		= chr(72);
	gds__dyn_fld_sub_type		= chr(73);
	gds__dyn_fld_segment_length	= chr(74);
	gds__dyn_fld_query_header	= chr(75);
	gds__dyn_fld_edit_string	= chr(76);
	gds__dyn_fld_validation_blr	= chr(77);
	gds__dyn_fld_validate_source	= chr(78);
	gds__dyn_fld_computed_blr	= chr(79);
	gds__dyn_fld_computed_source	= chr(80);
	gds__dyn_fld_missing_value	= chr(81);
	gds__dyn_fld_default_value	= chr(82);
	gds__dyn_fld_query_name		= chr(83);
	gds__dyn_fld_dimensions		= chr(84);
	gds__dyn_fld_not_null		= chr(85);
	gds__dyn_fld_collation_name	= chr(173);
	gds__dyn_fld_character_set_name = chr(174);

(* Local field specific attributes *)

	gds__dyn_fld_source		= chr(90);
	gds__dyn_fld_base_fld		= chr(91);
	gds__dyn_fld_position		= chr(92);
	gds__dyn_fld_update_flag	= chr(93);

(* Index specific attributes *)

	gds__dyn_idx_unique		= chr(100);
	gds__dyn_idx_active		= chr(101);
        gds__dyn_idx_type               = chr(103);
        gds__dyn_idx_foreign_key        = chr(104);
        gds__dyn_idx_ref_column         = chr(105);

(* Trigger specific attributes *)

	gds__dyn_trg_type		= chr(110);
	gds__dyn_trg_blr		= chr(111);
	gds__dyn_trg_source		= chr(112);
	gds__dyn_trg_name		= chr(114);
	gds__dyn_trg_sequence		= chr(115);
	gds__dyn_trg_inactive		= chr(116);
	gds__dyn_trg_msg_number		= chr(117);
	gds__dyn_trg_msg		= chr(118);

(* Security Class specific attributes *)

	gds__dyn_scl_acl		= chr(121);

(* Grant/Revoke specific attributes *)

        gds__dyn_grant_user             = chr(130);
        gds__dyn_grant_grantor          = chr(131);
        gds__dyn_grant_options          = chr(132);

(* dimension specific stuff *)

	gds__dyn_dim_lower		= chr(141);
	gds__dyn_dim_upper		= chr(142);

(* File specific attributes *)

	gds__dyn_file_name		= chr(125);
	gds__dyn_file_start		= chr(126);
	gds__dyn_file_length		= chr(127);
	gds__dyn_shadow_number		= chr(128);
	gds__dyn_shadow_man_auto	= chr(129);
	gds__dyn_shadow_conditional	= chr(130);

(* Function specific attributes *)

	gds__dyn_function_name		= chr(145);
	gds__dyn_function_type		= chr(146);
	gds__dyn_func_module_name	= chr(147);
	gds__dyn_func_entry_point	= chr(148);
	gds__dyn_func_return_argument	= chr(149);
	gds__dyn_func_arg_position	= chr(150);
	gds__dyn_func_mechanism		= chr(151);
	gds__dyn_filter_in_subtype	= chr(152);
	gds__dyn_filter_out_subtype	= chr(153);

(* Generator specific attributes *)

	gds__dyn_generator_name		= chr(95);
	gds__dyn_generator_id		= chr(96);

(* International specific attributes *)

	gds__dyn_fld_cription2		= chr(154);
	gds__dyn_fld_computed_source2	= chr(155);
	gds__dyn_fld_edit_string2	= chr(156);
	gds__dyn_fld_query_header2	= chr(157);
	gds__dyn_fld_validation_source2	= chr(158);
	gds__dyn_fld_trg_msg2		= chr(159);
	gds__dyn_fld_trg_source2	= chr(160);
	gds__dyn_fld_view_source2	= chr(161);

(* Procedure specific attributes *)

	gds__dyn_prc_inputs		= chr(167);
	gds__dyn_prc_outputs		= chr(168);
	gds__dyn_prc_source		= chr(169);
	gds__dyn_prc_blr		= chr(170);
	gds__dyn_prc_source2		= chr(171);

(* Procedure parameter specific attributes *)

	gds__dyn_prm_number		= chr(138);
	gds__dyn_prm_type		= chr(139);

(* Array slice description language (SDL) *)

        gds__sdl_version1               = chr(1);
        gds__sdl_eoc                    = chr(-1);
        gds__sdl_relation               = chr(2);
        gds__sdl_rid                    = chr(3);
        gds__sdl_field                  = chr(4);
        gds__sdl_fid                    = chr(5);
        gds__sdl_struct                 = chr(6);
        gds__sdl_variable               = chr(7);
        gds__sdl_scalar                 = chr(8);
        gds__sdl_tiny_integer           = chr(9);
        gds__sdl_short_integer          = chr(10);
        gds__sdl_long_integer           = chr(11);
        gds__sdl_literal                = chr(12);
        gds__sdl_add                    = chr(13);
        gds__sdl_subtract               = chr(14);
        gds__sdl_multiply               = chr(15);
        gds__sdl_divide                 = chr(16);
        gds__sdl_negate                 = chr(17);
        gds__sdl_eql                    = chr(18);
        gds__sdl_neq                    = chr(19);
        gds__sdl_gtr                    = chr(20);
        gds__sdl_geq                    = chr(21);
        gds__sdl_lss                    = chr(22);
        gds__sdl_leq                    = chr(23);
        gds__sdl_and                    = chr(24);
        gds__sdl_or                     = chr(25);
        gds__sdl_not                    = chr(26);
        gds__sdl_while                  = chr(27);
        gds__sdl_assignment             = chr(28);
        gds__sdl_label                  = chr(29);
        gds__sdl_leave                  = chr(30);
        gds__sdl_begin                  = chr(31);
        gds__sdl_end                    = chr(32);
        gds__sdl_do3                    = chr(33);
        gds__sdl_do2                    = chr(34);
        gds__sdl_do1                    = chr(35);
        gds__sdl_element                = chr(36);


(* Dynamic SQL datatypes *)

const
	SQL_TEXT	= 452;
	SQL_VARYING	= 448;
	SQL_SHORT	= 500;
	SQL_LONG	= 496;
	SQL_FLOAT	= 482;
	SQL_DOUBLE	= 480;
	SQL_D_FLOAT	= 530;
	SQL_DATE	= 510;
	SQL_BLOB	= 520;

