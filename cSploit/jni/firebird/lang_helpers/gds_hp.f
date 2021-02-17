C  The contents of this file are subject to the Interbase Public
C  License Version 1.0 (the "License"); you may not use this file
C  except in compliance with the License. You may obtain a copy
C  of the License at http://www.Inprise.com/IPL.html
C
C  Software distributed under the License is distributed on an
C  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
C  or implied. See the License for the specific language governing
C  rights and limitations under the License.
C
C  The Original Code was created by Inprise Corporation
C  and its predecessors. Portions created by Inprise Corporation are
C  Copyright (C) Inprise Corporation.
C
C  All Rights Reserved.
C  Contributor(s): ______________________________________.
C
C         PROGRAM:        Preprocessor
C         MODULE:         gds.ftn
C         DESCRIPTION:    GDS constants, procedures, etc.
C 

      INTEGER*2 INT2
$ALIAS gds__attach_database (%REF, %VAL, %REF, %REF, %VAL, %REF)

$ALIAS gds__blob_display (%REF, %REF, %REF, %REF, %REF)
$ALIAS gds__blob_dump (%REF, %REF, %REF, %REF, %REF)
$ALIAS gds__blob_edit (%REF, %REF, %REF, %REF, %REF)
$ALIAS gds__blob_load (%REF, %REF, %REF, %REF, %REF)

$ALIAS gds__cancel_blob (%REF, %REF)

$ALIAS gds__cancel_events (%REF, %REF, %REF)
$ALIAS gds__close (%REF, %REF)
$ALIAS gds__close_blob (%REF, %REF)
$ALIAS gds__commit_retaining (%REF, %REF)
$ALIAS gds__commit_transaction (%REF, %REF)
$ALIAS gds__compile_request (%REF, %REF, %REF, %VAL, %REF)
$ALIAS gds__compile_request2 (%REF, %REF, %REF, %VAL, %REF)
$ALIAS gds__create_blob (%REF, %REF, %REF, %REF, %REF)
$ALIAS gds__create_blob2 (%REF, %REF, %REF, %REF, %REF, %VAL, %REF)
$ALIAS gds__create_database (%REF, %VAL, %REF, %REF, %VAL, %REF, %REF)

$ALIAS gds__ddl (%REF, %REF,  %REF, %VAL, %REF)
$ALIAS gds__declare (%REF, %REF, %REF)
$ALIAS gds__decode_date (%REF, %REF)
$ALIAS gds__describe (%REF, %REF, %REF)
$ALIAS gds__detach_database (%REF, %REF)
$ALIAS gds__dsql_finish (%REF)

$ALIAS gds__encode_date (%REF, %REF)
$ALIAS gds__event_block_a (%REF, %REF, %VAL, %REF)
$ALIAS gds__event_counts (%REF, %VAL, %VAL, %VAL)
$ALIAS gds__event_wait (%REF, %REF, %VAL, %VAL, %VAL)
$ALIAS gds__execute (%REF, %REF, %REF, %REF)
$ALIAS gds__execute_immediate (%REF, %REF, %REF, %REF, %REF)

$ALIAS gds__fetch (%REF, %REF, %REF)
$ALIAS gds__ftof (%REF, %VAL, %REF, %VAL)

$ALIAS gds__get_segment (%REF, %REF, %REF, %VAL, %REF)
$ALIAS gds__get_slice (%REF, %REF, %REF, %REF, %VAL, %REF, %VAL,\
$ %REF, %VAL, %REF, %REF)

$ALIAS gds__open (%REF, %REF, %REF, %REF)
$ALIAS gds__open_blob (%REF, %REF, %REF, %REF, %REF)
$ALIAS gds__open_blob2 (%REF, %REF, %REF, %REF, %REF, %VAL, %REF)

$ALIAS gds__prepare (%REF, %REF, %REF, %REF, %REF, %REF, %REF)
$ALIAS gds__prepare_transaction (%REF, %REF)
$ALIAS gds__prepare_transaction2 (%REF, %REF, %VAL, %REF)
$ALIAS gds__print_status (%REF)
$ALIAS gds__put_segment (%REF, %REF, %VAL, %REF)
$ALIAS gds__put_slice (%REF, %REF, %REF, %REF, %VAL, %REF, %VAL, %REF,\
$ %VAL, %REF)

$ALIAS gds__que_events (%REF, %REF, %REF, %VAL, %REF, %VAL, %REF)

$ALIAS gds__receive (%REF, %REF, %VAL, %VAL, %REF, %VAL)
$ALIAS gds__rollback_transaction (%REF, %REF)

$ALIAS gds__sqlcode (%REF)
$ALIAS gds__send (%REF, %REF, %VAL, %VAL, %REF, %VAL)
$ALIAS gds__start_and_send (%REF, %REF, %REF, %VAL, %VAL, %REF, %VAL)
$ALIAS gds__start_multiple (%REF, %REF, %VAL, %REF)
$ALIAS gds__start_request (%REF, %REF, %REF, %VAL)
$ALIAS gds__start_transaction (%REF, %REF, %VAL, %REF, %VAL, %REF)

$ALIAS gds__unwind_request (%REF, %REF, %VAL)

$ALIAS gds__version (%REF, %VAL, %REF)

$ALIAS isc_baddress (%REF)

$ALIAS isc_dsql_allocate_statement (%REF, %REF, %REF)
$ALIAS isc_dsql_alloc_statement2 (%REF, %REF, %REF)
$ALIAS isc_dsql_execute_m (%REF, %REF, %REF, %VAL, %REF, %VAL,\
$ %VAL, %VAL)
$ALIAS isc_dsql_execute2_m (%REF, %REF, %REF, %VAL, %REF, %VAL,\
$ %VAL, %VAL, %VAL, %REF, %VAL, %VAL, %VAL)
$ALIAS isc_dsql_free_statement (%REF, %REF, %VAL)
$ALIAS isc_dsql_set_cursor_name (%REF, %REF, %REF, %VAL)

$ALIAS isc_embed_dsql_close (%REF, %REF)
$ALIAS isc_embed_dsql_declare (%REF, %REF, %REF)
$ALIAS isc_embed_dsql_describe (%REF, %REF, %VAL, %VAL)
$ALIAS isc_embed_dsql_describe_bind (%REF, %REF, %VAL, %VAL)
$ALIAS isc_embed_dsql_execute (%REF, %REF, %REF, %VAL, %VAL)
$ALIAS isc_embed_dsql_execute2 (%REF, %REF, %REF, %VAL, %VAL, %VAL)
$ALIAS isc_embed_dsql_execute_immed (%REF, %REF, %REF, %VAL, %REF,\
$ %VAL, %VAL)
$ALIAS isc_embed_dsql_fetch (%REF, %REF, %VAL, %VAL)
$ALIAS isc_embed_dsql_open (%REF, %REF, %REF, %VAL, %VAL)
$ALIAS isc_embed_dsql_prepare (%REF, %REF, %REF, %REF, %VAL, %REF,\
$ %VAL, %VAL)
$ALIAS isc_embed_dsql_release (%REF, %REF)

$ALIAS pyxis__compile_map (%REF, %REF, %REF, %REF, %REF)
$ALIAS pyxis__compile_menu (%REF, %REF, %REF, %REF)
$ALIAS pyxis__compile_sub_map (%REF, %REF, %REF, %REF, %REF)
$ALIAS pyxis__create_window (%REF, %REF, %REF, %REF, %REF)

$ALIAS pyxis__delete_window (%REF)
$ALIAS pyxis__drive_form (%REF, %REF, %REF, %REF, %REF, %REF, %REF)
$ALIAS pyxis__drive_menu (%REF, %REF, %REF, %REF, %REF, %REF, %REF,\
$ %REF, %REF, %REF)

$ALIAS pyxis__fetch (%REF, %REF, %REF, %REF, %REF)

$ALIAS pyxis__get_entree (%REF, %REF, %REF, %REF, %REF)

$ALIAS pyxis__initialize_menu (%REF)
$ALIAS pyxis__insert (%REF, %REF, %REF, %REF, %REF)

$ALIAS pyxis__load_form (%REF, %REF, %REF, %REF, %REF, %REF)

$ALIAS pyxis__menu (%REF, %REF, %REF, %REF)

$ALIAS pyxis__pop_window (%REF)
$ALIAS pyxis__put_entree (%REF, %REF, %REF, %REF)

$ALIAS pyxis__reset_form (%REF, %REF)
$ALIAS pyxis__suspend_window (%REF)
$ALIAS blob__edit (%REF, %REF, %REF, %REF, %REF)
$ALIAS blob__display (%REF, %REF, %REF, %REF, %REF)
$ALIAS blob__dump (%REF, %REF, %REF, %REF, %REF)
$ALIAS blob__load (%REF, %REF, %REF, %REF, %REF)

$ALIAS isc_exec_procedure (%REF, %REF, %REF, %VAL, %REF,\
$ %VAL, %REF, %VAL, %REF, %VAL, %REF)

$ALIAS isc_attach_database (%REF, %VAL, %REF, %REF, %VAL, %VAL)

$ALIAS isc_blob_display (%REF, %REF, %REF, %REF, %REF)
$ALIAS isc_blob_dump (%REF, %REF, %REF, %REF, %REF)
$ALIAS isc_blob_edit (%REF, %REF, %REF, %REF, %REF)
$ALIAS isc_blob_load (%REF, %REF, %REF, %REF, %REF)

$ALIAS isc_cancel_blob (%REF, %REF)

$ALIAS isc_cancel_events (%REF, %REF, %REF)
$ALIAS isc_close (%REF, %REF)
$ALIAS isc_close_blob (%REF, %REF)
$ALIAS isc_commit_retaining (%REF, %REF)
$ALIAS isc_commit_transaction (%REF, %REF)
$ALIAS isc_compile_request (%REF, %REF, %REF, %VAL, %REF)
$ALIAS isc_compile_request2 (%REF, %REF, %REF, %VAL, %REF)
$ALIAS isc_create_blob (%REF, %REF, %REF, %REF, %REF)
$ALIAS isc_create_blob2 (%REF, %REF, %REF, %REF, %REF, %VAL, %REF)
$ALIAS isc_create_database (%REF, %VAL, %REF, %REF, %VAL, %VAL, %REF)

$ALIAS isc_ddl (%REF, %REF,  %REF, %VAL, %REF)
$ALIAS isc_declare (%REF, %REF, %REF)
$ALIAS isc_decode_date (%REF, %REF)
$ALIAS isc_describe (%REF, %REF, %REF)
$ALIAS isc_detach_database (%REF, %REF)
$ALIAS isc_dsql_finish (%REF)

$ALIAS isc_encode_date (%REF, %REF)
$ALIAS isc_event_block_a (%REF, %REF, %VAL, %REF)
$ALIAS isc_event_counts (%REF, %VAL, %VAL, %VAL)
$ALIAS isc_event_wait (%REF, %REF, %VAL, %VAL, %VAL)
$ALIAS isc_execute (%REF, %REF, %REF, %REF)
$ALIAS isc_execute_immediate (%REF, %REF, %REF, %REF, %REF)

$ALIAS isc_fetch (%REF, %REF, %REF)
$ALIAS isc_ftof (%REF, %VAL, %REF, %VAL)

$ALIAS isc_get_segment (%REF, %REF, %REF, %VAL, %REF)
$ALIAS isc_get_slice (%REF, %REF, %REF, %REF, %VAL, %REF, %VAL,\
$ %REF, %VAL, %REF, %REF)

$ALIAS isc_open (%REF, %REF, %REF, %REF)
$ALIAS isc_open_blob (%REF, %REF, %REF, %REF, %REF)
$ALIAS isc_open_blob2 (%REF, %REF, %REF, %REF, %REF, %VAL, %REF)

$ALIAS isc_prepare (%REF, %REF, %REF, %REF, %REF, %REF, %REF)
$ALIAS isc_prepare_transaction (%REF, %REF)
$ALIAS isc_prepare_transaction2 (%REF, %REF, %VAL, %REF)
$ALIAS isc_print_status (%REF)
$ALIAS isc_put_segment (%REF, %REF, %VAL, %REF)
$ALIAS isc_put_slice (%REF, %REF, %REF, %REF, %VAL, %REF, %VAL, %REF,\
$ %VAL, %REF)

$ALIAS isc_que_events (%REF, %REF, %REF, %VAL, %REF, %VAL, %REF)

$ALIAS isc_receive (%REF, %REF, %VAL, %VAL, %REF, %VAL)
$ALIAS isc_rollback_transaction (%REF, %REF)

$ALIAS isc_sqlcode (%REF)
$ALIAS isc_send (%REF, %REF, %VAL, %VAL, %REF, %VAL)
$ALIAS isc_start_and_send (%REF, %REF, %REF, %VAL, %VAL, %REF, %VAL)
$ALIAS isc_start_multiple (%REF, %REF, %VAL, %REF)
$ALIAS isc_start_request (%REF, %REF, %REF, %VAL)
$ALIAS isc_start_transaction (%REF, %REF, %VAL, %REF, %VAL, %REF)

$ALIAS isc_unwind_request (%REF, %REF, %VAL)

$ALIAS isc_version (%REF, %VAL, %REF)
$ALIAS isc_modify_dpb (%REF, %REF, %VAL, %REF, %VAL)
$ALIAS isc_free (%VAL)

C     PROCEDURE GDS__ATTACH_DATABASE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*2       NAME_LENGTH,    !{ INPUT }
C        CHARACTER* (*)  FILE_NAME,      !{ INPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*2       DPB_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  DPB             !{ INPUT }
C        )

      INTEGER*4 ISC_BADDRESS 
C     INTEGER*4 FUNCTION ISC_BADDRESS (
C        INTEGER*4       HANDLE          !( INPUT )
C        )

C     PROCEDURE GDS__CANCEL_BLOB (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE     !{ INPUT }
C        )

C     PROCEDURE GDS__CANCEL_EVENTS (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       ID              !{ INPUT }
C        )

C     PROCEDURE GDS__CLOSE_BLOB (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE     !{ INPUT }
C        )

C     PROCEDURE GDS__COMMIT_RETAINING (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       TRA_HANDLE      !{ INPUT / OUTPUT }
C        )

C     PROCEDURE GDS__COMMIT_TRANSACTION (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       TRA_HANDLE      !{ INPUT / OUTPUT }
C        )

C     PROCEDURE GDS__COMPILE_REQUEST (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT / OUTPUT }
C        INTEGER*2       BLR_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  BLR             !{ INPUT }
C        )

C     PROCEDURE GDS__COMPILE_REQUEST2 (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT / OUTPUT }
C        INTEGER*2       BLR_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  BLR             !{ INPUT }
C        )

C     PROCEDURE GDS__CREATE_BLOB (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT / OUTPUT }
C        INTEGER*4       BLOB_ID(2)      !{ OUTPUT }
C        )

C     PROCEDURE GDS__CREATE_BLOB2 (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT / OUTPUT }
C        INTEGER*4       BLOB_ID(2),      !{ OUTPUT }
C        CHARACTER* (*)  BPB             !{ INPUT }
C        )

C     PROCEDURE GDS__CREATE_DATABASE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*2       NAME_LENGTH,    !{ INPUT }
C        CHARACTER* (*)  FILE_NAME,      !{ INPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*2       DPB_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  DPB             !{ INPUT }
C        )

C     PROCEDURE GDS__DETACH_DATABASE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE       !{ INPUT / OUTPUT }
C        )

      INTEGER*2 GDS__EVENT_BLOCK_A
C     PROCEDURE GDS__EVENT_BLOCK_A (
C        CHARACTER* (*)  EVENTS,         !{ INPUT/OUTPUT }
C        CHARACTER* (*)  BUFFER          !{ INPUT/OUTPUT }
C        INTEGER*2       COUNT,          !{ INPUT }
C        CHARACTER*31    EVENT_NAMES     !{ INPUT/OUTPUT }
C        )

C     PROCEDURE GDS__EVENT_WAIT (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*2       LENGTH,         !{ INPUT }
C        CHARACTER* (*)  EVENTS,         !{ INPUT/OUTPUT }
C        CHARACTER* (*)  BUFFER          !{ INPUT/OUTPUT }
C        )

      INTEGER*4 GDS__GET_SEGMENT 
C     INTEGER*4 FUNCTION GDS__GET_SEGMENT (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT }
C        INTEGER*2       LENGTH,         !{ OUTPUT }
C        INTEGER*2       BUFFER_LENGTH,  !{ INPUT }
C        CHARACTER* (*)  BUFFER          !{ INPUT }
C        )

      INTEGER*4 GDS__GET_SLICE 
C     INTEGER*4 FUNCTION GDS__GET_SLICE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT }
C        INTEGER*2       SDL_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  SDL,            !{ INPUT }
C        INTEGER*2       PARAM_LENGTH,   !{ INPUT }
C        CHARACTER* (*)  PARAM,          !{ INPUT }
C        INTEGER*4       SLICE_LENGTH,   !{ INPUT }
C        CHARACTER* (*)  SLICE,          !{ INPUT }
C        INTEGER*4       RETURN_LENGTH   !{ INPUT }
C        )

C     PROCEDURE GDS__OPEN_BLOB (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT / OUTPUT }
C        INTEGER*4       BLOB_ID (2)     !{ INPUT }
C        )

C     PROCEDURE GDS__OPEN_BLOB2 (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT / OUTPUT }
C        INTEGER*4       BLOB_ID (2),    !{ INPUT }
C        CHARACTER* (*)  BPB             !{ INPUT }
C        )

C      PROCEDURE GDS__COMMIT_TRANSACTION (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT / OUTPUT }
C        )

      INTEGER*4 GDS__PUT_SEGMENT 
C     INTEGER*4 FUNCTION GDS__PUT_SEGMENT (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT }
C        INTEGER*2       LENGTH,         !{ INPUT }
C        CHARACTER* (*)  BUFFER,         !{ INPUT }
C        )

      INTEGER*4 GDS__PUT_SLICE
C     INTEGER*4 FUNCTION GDS__PUT_SLICE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT }
C        INTEGER*2       SDL_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  SDL,            !{ INPUT }
C        INTEGER*2       PARAM_LENGTH,   !{ INPUT }
C        CHARACTER* (*)  PARAM,          !{ INPUT }
C        INTEGER*4       SLICE_LENGTH,   !{ INPUT }
C        CHARACTER* (*)  SLICE           !{ INPUT }
C        )

C     PROCEDURE GDS__QUE_EVENTS (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       ID,             !{ INPUT }
C        INTEGER*2       LENGTH,         !{ INPUT }
C        CHARACTER* (*)  EVENTS,         !{ INPUT/OUTPUT }
C        INTEGER*4       AST,            !{ INPUT }
C        INTEGER*4       ARG             !{ INPUT }
C        )

C     PROCEDURE GDS__RECEIVE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT }
C        INTEGER*2       MESSAGE_TYPE,   !{ INPUT }
C        INTEGER*2       MESSAGE_LENGTH, !{ INPUT }
C        CHARACTER* (*)  MESSAGE,        !{ INPUT }
C        INTEGER*2       INSTANTIATION   !{ INPUT }
C        )

C     PROCEDURE GDS__RELEASE_REQUEST (
C        INTEGER*4       STAT(20),        !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE   !{ INPUT / OUTPUT }
C        )

C     PROCEDURE GDS__ROLLBACK_TRANSACTION (
C        INTEGER*4       STAT(20),         !{ OUTPUT }
C        INTEGER*4       TRA_HANDLE        !{ INPUT / OUTPUT }
C        )

C     PROCEDURE GDS__SEND (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT }
C        INTEGER*2       MESSAGE_TYPE,   !{ INPUT }
C        INTEGER*2       MESSAGE_LENGTH, !{ INPUT }
C        CHARACTER* (*)  MESSAGE,        !{ INPUT }
C        INTEGER*2       INSTANTIATION   !{ INPUT }
C        )

C     PROCEDURE GDS__SET_DEBUG (
C        INTEGER*4       DEBUG_VAL       !{ INPUT }
C        )

C     PROCEDURE GDS__START_AND_SEND (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT / OUTPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*2       MESSAGE_TYPE,   !{ INPUT }
C        INTEGER*2       MESSAGE_LENGTH, !{ INPUT }
C        CHARACTER* (*)  MESSAGE,        !{ INPUT }
C        INTEGER*2       INSTANTIATION   !{ INPUT }
C        )

C     PROCEDURE GDS__START_REQUEST (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT  }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*2       INSTANTIATION   !{ INPUT }
C        )
   
C     PROCEDURE GDS__START_TRANSACTION (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT / OUTPUT }
C        INTEGER*2       TRA_COUNT,      !{ INPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*2       TPB_LENGTH      !{ INPUT }
C        CHARACTER* (*)  TPB             !{ INPUT }
C        )

C     PROCEDURE GDS__UNWIND_REQUEST (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT }
C        INTEGER*2       INSTANTIATION   !{ INPUT }
C        )

C     PROCEDURE gds__ftof (
C        CHARACTER* (*)  STRING1,        !{ INPUT }
C        INTEGER*2       LENGTH1,        !{ INPUT }
C        CHARACTER* (*)  STRING2,        !{ INPUT }
C        INTEGER*2       LENGTH2         !{ INPUT }
C        )

C     PROCEDURE gds__print_status (
C        INTEGER*4       STAT(20)        !{ INPUT }
C        )
 
C     INTEGER FUNCTION gds__sqlcode (
C        INTEGER*4       STAT(20)        !{ INPUT }
C        )
 
C     procedure gds__close (
C        out     stat            : gds__status_vector; 
C        in      cursor_name     : UNIV gds__string
C        ); extern;

C     procedure gds__declare (
C        out       stat               : gds__status_vector; 
C        in        statement_name     : UNIV gds__string;
C        in        cursor_name        : UNIV gds__string
C        ); extern;

C     procedure gds__describe (
C        out       stat               : gds__status_vector; 
C        in        statement_name     : UNIV gds__string;
C        in out    descriptor         : UNIV sqlda
C        ); extern;

C     procedure gds__execute (
C        out       stat                : gds__status_vector; 
C        in        trans_handle        : gds__handle;
C        in        statement_name      : UNIV gds__string;
C        in out    descriptor          : UNIV sqlda
C        ); extern;

C     procedure gds__execute_immediate (
C        out       stat               : gds__status_vector; 
C        in        db_handle          : gds__handle;
C        in        trans_handle       : gds__handle;
C        in        string_length      : integer;
C        in        string             : UNIV gds__string
C        ); extern;

      INTEGER*4 GDS__FETCH
C     function gds__fetch (
C        out       stat              : gds__status_vector; 
C        in        cursor_name       : UNIV gds__string;
C        in out        descriptor    : UNIV sqlda
C        ) : integer32; extern;

C     procedure gds__open (
C        out       stat              : gds__status_vector; 
C        in        trans_handle      : gds__handle;
C        in        cursor_name       : UNIV gds__string;
C        in out    descriptor        : UNIV sqlda
C        ); extern;

C     procedure gds__prepare (
C        out       stat             : gds__status_vector; 
C        in        db_handle        : gds__handle;
C        in        trans_handle     : gds__handle;
C        in        statement_name   : UNIV gds__string;
C        in        string_length    : integer;
C        in        string           : UNIV gds__string;
C        in out    descriptor       : UNIV sqlda
C        ); extern;

C     PROCEDURE isc_exec_procedure (
C        INTEGER*4       STAT(20),       { OUTPUT }
C        INTEGER*4       DB_HANDLE,      { INPUT }
C        INTEGER*4       TRA_HANDLE,     { INPUT }
C        INTEGER*2       NAME_LENGTH,    { INPUT }
C        CHARACTER* (*)  PROC_NAME,      { INPUT }
C        INTEGER*2       BLR_LENGTH,     { INPUT }
C        CHARACTER* (*)  BLR,             { INPUT }
C        INTEGER*2       IN_MESSAGE_LENGTH, { INPUT }
C        CHARACTER* (*)  IN_MESSAGE,        { INPUT }
C        INTEGER*2       OUT_MESSAGE_LENGTH, { INPUT }
C        CHARACTER* (*)  OUT_MESSAGE        { INPUT / OUTPUT }
C        )

C     PROCEDURE ISC_ATTACH_DATABASE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*2       NAME_LENGTH,    !{ INPUT }
C        CHARACTER* (*)  FILE_NAME,      !{ INPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*2       DPB_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  DPB             !{ INPUT }
C        )

C     INTEGER*4 FUNCTION ISC_BADDRESS (
C        INTEGER*4       HANDLE          !( INPUT )
C        )

C     PROCEDURE ISC_CANCEL_BLOB (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE     !{ INPUT }
C        )

C     PROCEDURE ISC_CANCEL_EVENTS (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       ID              !{ INPUT }
C        )

C     PROCEDURE ISC_CLOSE_BLOB (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE     !{ INPUT }
C        )

C     PROCEDURE ISC_COMMIT_RETAINING (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       TRA_HANDLE      !{ INPUT / OUTPUT }
C        )

C     PROCEDURE ISC_COMMIT_TRANSACTION (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       TRA_HANDLE      !{ INPUT / OUTPUT }
C        )

C     PROCEDURE ISC_COMPILE_REQUEST (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT / OUTPUT }
C        INTEGER*2       BLR_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  BLR             !{ INPUT }
C        )

C     PROCEDURE ISC_COMPILE_REQUEST2 (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT / OUTPUT }
C        INTEGER*2       BLR_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  BLR             !{ INPUT }
C        )

C     PROCEDURE ISC_CREATE_BLOB (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT / OUTPUT }
C        INTEGER*4       BLOB_ID(2)      !{ OUTPUT }
C        )

C     PROCEDURE ISC_CREATE_BLOB2 (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT / OUTPUT }
C        INTEGER*4       BLOB_ID(2),      !{ OUTPUT }
C        CHARACTER* (*)  BPB             !{ INPUT }
C        )

C     PROCEDURE ISC_CREATE_DATABASE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*2       NAME_LENGTH,    !{ INPUT }
C        CHARACTER* (*)  FILE_NAME,      !{ INPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*2       DPB_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  DPB             !{ INPUT }
C        )

C     PROCEDURE ISC_DETACH_DATABASE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE       !{ INPUT / OUTPUT }
C        )

      INTEGER*2 ISC_EVENT_BLOCK_A
C     PROCEDURE ISC_EVENT_BLOCK_A (
C        CHARACTER* (*)  EVENTS,         !{ INPUT/OUTPUT }
C        CHARACTER* (*)  BUFFER          !{ INPUT/OUTPUT }
C        INTEGER*2       COUNT,          !{ INPUT }
C        CHARACTER*31    EVENT_NAMES     !{ INPUT/OUTPUT }
C        )

C     PROCEDURE ISC_EVENT_WAIT (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*2       LENGTH,         !{ INPUT }
C        CHARACTER* (*)  EVENTS,         !{ INPUT/OUTPUT }
C        CHARACTER* (*)  BUFFER          !{ INPUT/OUTPUT }
C        )

      INTEGER*4 ISC_GET_SEGMENT 
C     INTEGER*4 FUNCTION ISC_GET_SEGMENT (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT }
C        INTEGER*2       LENGTH,         !{ OUTPUT }
C        INTEGER*2       BUFFER_LENGTH,  !{ INPUT }
C        CHARACTER* (*)  BUFFER          !{ INPUT }
C        )

      INTEGER*4 ISC_GET_SLICE 
C     INTEGER*4 FUNCTION ISC_GET_SLICE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT }
C        INTEGER*2       SDL_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  SDL,            !{ INPUT }
C        INTEGER*2       PARAM_LENGTH,   !{ INPUT }
C        CHARACTER* (*)  PARAM,          !{ INPUT }
C        INTEGER*4       SLICE_LENGTH,   !{ INPUT }
C        CHARACTER* (*)  SLICE,          !{ INPUT }
C        INTEGER*4       RETURN_LENGTH   !{ INPUT }
C        )

C     PROCEDURE ISC_OPEN_BLOB (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT / OUTPUT }
C        INTEGER*4       BLOB_ID (2)     !{ INPUT }
C        )

C     PROCEDURE ISC_OPEN_BLOB2 (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT / OUTPUT }
C        INTEGER*4       BLOB_ID (2),    !{ INPUT }
C        CHARACTER* (*)  BPB             !{ INPUT }
C        )

C      PROCEDURE ISC_COMMIT_TRANSACTION (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT / OUTPUT }
C        )

      INTEGER*4 ISC_PUT_SEGMENT 
C     INTEGER*4 FUNCTION ISC_PUT_SEGMENT (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT }
C        INTEGER*2       LENGTH,         !{ INPUT }
C        CHARACTER* (*)  BUFFER,         !{ INPUT }
C        )

      INTEGER*4 ISC_PUT_SLICE
C     INTEGER*4 FUNCTION ISC_PUT_SLICE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       TRA_HANDLE,     !{ OUTPUT }
C        INTEGER*4       BLOB_HANDLE,    !{ INPUT }
C        INTEGER*2       SDL_LENGTH,     !{ INPUT }
C        CHARACTER* (*)  SDL,            !{ INPUT }
C        INTEGER*2       PARAM_LENGTH,   !{ INPUT }
C        CHARACTER* (*)  PARAM,          !{ INPUT }
C        INTEGER*4       SLICE_LENGTH,   !{ INPUT }
C        CHARACTER* (*)  SLICE           !{ INPUT }
C        )

C     PROCEDURE ISC_QUE_EVENTS (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*4       ID,             !{ INPUT }
C        INTEGER*2       LENGTH,         !{ INPUT }
C        CHARACTER* (*)  EVENTS,         !{ INPUT/OUTPUT }
C        INTEGER*4       AST,            !{ INPUT }
C        INTEGER*4       ARG             !{ INPUT }
C        )

C     PROCEDURE ISC_RECEIVE (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT }
C        INTEGER*2       MESSAGE_TYPE,   !{ INPUT }
C        INTEGER*2       MESSAGE_LENGTH, !{ INPUT }
C        CHARACTER* (*)  MESSAGE,        !{ INPUT }
C        INTEGER*2       INSTANTIATION   !{ INPUT }
C        )

C     PROCEDURE ISC_RELEASE_REQUEST (
C        INTEGER*4       STAT(20),        !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE   !{ INPUT / OUTPUT }
C        )

C     PROCEDURE ISC_ROLLBACK_TRANSACTION (
C        INTEGER*4       STAT(20),         !{ OUTPUT }
C        INTEGER*4       TRA_HANDLE        !{ INPUT / OUTPUT }
C        )

C     PROCEDURE ISC_SEND (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT }
C        INTEGER*2       MESSAGE_TYPE,   !{ INPUT }
C        INTEGER*2       MESSAGE_LENGTH, !{ INPUT }
C        CHARACTER* (*)  MESSAGE,        !{ INPUT }
C        INTEGER*2       INSTANTIATION   !{ INPUT }
C        )

C     PROCEDURE ISC_SET_DEBUG (
C        INTEGER*4       DEBUG_VAL       !{ INPUT }
C        )

C     PROCEDURE ISC_START_AND_SEND (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT / OUTPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*2       MESSAGE_TYPE,   !{ INPUT }
C        INTEGER*2       MESSAGE_LENGTH, !{ INPUT }
C        CHARACTER* (*)  MESSAGE,        !{ INPUT }
C        INTEGER*2       INSTANTIATION   !{ INPUT }
C        )

C     PROCEDURE ISC_START_REQUEST (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT  }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT }
C        INTEGER*2       INSTANTIATION   !{ INPUT }
C        )
   
C     PROCEDURE ISC_START_TRANSACTION (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       TRA_HANDLE,     !{ INPUT / OUTPUT }
C        INTEGER*2       TRA_COUNT,      !{ INPUT }
C        INTEGER*4       DB_HANDLE,      !{ INPUT }
C        INTEGER*2       TPB_LENGTH      !{ INPUT }
C        CHARACTER* (*)  TPB             !{ INPUT }
C        )

C     PROCEDURE ISC_UNWIND_REQUEST (
C        INTEGER*4       STAT(20),       !{ OUTPUT }
C        INTEGER*4       REQUEST_HANDLE, !{ INPUT }
C        INTEGER*2       INSTANTIATION   !{ INPUT }
C        )

C     PROCEDURE isc_ftof (
C        CHARACTER* (*)  STRING1,        !{ INPUT }
C        INTEGER*2       LENGTH1,        !{ INPUT }
C        CHARACTER* (*)  STRING2,        !{ INPUT }
C        INTEGER*2       LENGTH2         !{ INPUT }
C        )

C     PROCEDURE isc_print_status (
C        INTEGER*4       STAT(20)        !{ INPUT }
C        )
 
C     INTEGER FUNCTION isc_sqlcode (
C        INTEGER*4       STAT(20)        !{ INPUT }
C        )
 
C     procedure isc_close (
C        out     stat            : isc_status_vector; 
C        in      cursor_name     : UNIV isc_string
C        ); extern;

C     procedure isc_declare (
C        out       stat               : isc_status_vector; 
C        in        statement_name     : UNIV isc_string;
C        in        cursor_name        : UNIV isc_string
C        ); extern;

C     procedure isc_describe (
C        out       stat               : isc_status_vector; 
C        in        statement_name     : UNIV isc_string;
C        in out    descriptor         : UNIV sqlda
C        ); extern;

C     procedure isc_execute (
C        out       stat                : isc_status_vector; 
C        in        trans_handle        : isc_handle;
C        in        statement_name      : UNIV isc_string;
C        in out    descriptor          : UNIV sqlda
C        ); extern;

C     procedure isc_execute_immediate (
C        out       stat               : isc_status_vector; 
C        in        db_handle          : isc_handle;
C        in        trans_handle       : isc_handle;
C        in        string_length      : integer;
C        in        string             : UNIV isc_string
C        ); extern;

      INTEGER*4 ISC_FETCH
C     function isc_fetch (
C        out       stat              : isc_status_vector; 
C        in        cursor_name       : UNIV isc_string;
C        in out        descriptor    : UNIV sqlda
C        ) : integer32; extern;

C     procedure isc_open (
C        out       stat              : isc_status_vector; 
C        in        trans_handle      : isc_handle;
C        in        cursor_name       : UNIV isc_string;
C        in out    descriptor        : UNIV sqlda
C        ); extern;

C     procedure isc_prepare (
C        out       stat             : isc_status_vector; 
C        in        db_handle        : isc_handle;
C        in        trans_handle     : isc_handle;
C        in        statement_name   : UNIV isc_string;
C        in        string_length    : integer;
C        in        string           : UNIV isc_string;
C        in out    descriptor       : UNIV sqlda
C        ); extern;

C     procedure pyxis__compile_map (
C        out       stat                : isc_status_vector; 
C        in        form_handle         : isc_handle;
C        out       map_handle          : isc_handle;
C        in        length              : integer16;
C        in        map                 : UNIV isc_string
C        ); extern;

C     procedure pyxis__compile_sub_map (
C        out       stat                : isc_status_vector; 
C        in        form_handle         : isc_handle;
C        out       map_handle          : isc_handle;
C        in        length              : integer16;
C        in        map                 : UNIV isc_string
C        ); extern;

C     procedure pyxis__create_window (
C        in out    window_handle      : isc_handle;
C        in        name_length        : integer16;
C        in        file_name          : UNIV isc_string; 
C        in out    width              : integer16;
C        in out    height             : integer16
C        ); extern;

C     procedure pyxis__delete_window (
C        in out        window_handle        : isc_handle
C        ); extern;

C     procedure pyxis__drive_form (
C        out       stat             : isc_status_vector; 
C        in        db_handle        : isc_handle;
C        in        trans_handle     : isc_handle;
C        in out    window_handle    : isc_handle;
C        in        map_handle       : isc_handle;
C        in        input            : UNIV isc_string;
C        in        output           : UNIV isc_string
C        ); extern;

C     procedure pyxis__fetch (
C        out       stat             : isc_status_vector; 
C        in        db_handle        : isc_handle;
C        in        trans_handle     : isc_handle;
C        in        map_handle       : isc_handle;
C        in        output           : UNIV isc_string
C        ); extern;

C     procedure pyxis__insert (
C        out       stat             : isc_status_vector; 
C        in        db_handle        : isc_handle;
C        in        trans_handle     : isc_handle;
C        in        map_handle       : isc_handle;
C        in        input            : UNIV isc_string
C        ); extern;

C     procedure pyxis__load_form (
C        out       stat             : isc_status_vector; 
C        in        db_handle        : isc_handle;
C        in        trans_handle     : isc_handle;
C        in out    form_handle      : isc_handle;
C        in        name_length      : integer16;
C        in        form_name        : UNIV isc_string 
C        ); extern;

C     function pyxis__menu (
C        in out    window_handle    : isc_handle;
C        in out    menu_handle      : isc_handle;
C        in        length           : integer16;
C        in        menu             : UNIV isc_string
C        ) : integer16; extern;

C     procedure pyxis__pop_window (
C        in        window_handle    : isc_handle
C        ); extern;

C     procedure pyxis__reset_form (
C        out       stat            : isc_status_vector; 
C        in        window_handle   : isc_handle
C        ); extern;

C     procedure pyxis__drive_menu (
C        in out    window_handle     : isc_handle;
C        in out    menu_handle       : isc_handle;
C        in        blr_length        : integer16;
C        in        blr_source        : UNIV isc_string;
C        in        title_length      : integer16;
C        in        title             : UNIV isc_string;
C        out       terminator        : integer16;
C        out       entree_length     : integer16;
C        out       entree_text       : UNIV isc_string;
C        out       entree_value      : integer32
C        ); extern;

C     procedure pyxis__get_entree (
C        in        menu_handle       : isc_handle;
C        out       entree_length     : integer16;
C        out       entree_text       : UNIV isc_string;
C        out       entree_value      : integer32;
C        out       entree_end        : integer16
C        ); extern;

C     procedure pyxis__initialize_menu (
C        in out        menu_handle   : isc_handle
C        ); extern;

C     procedure pyxis__put_entree (
C        in        menu_handle        : isc_handle;
C        in        entree_length      : integer16;
C        in        entree_text        : UNIV isc_string;
C        in        entree_value       : integer32
C        ); extern;

C     PROCEDURE isc_exec_procedure (
C        INTEGER*4       STAT(20),       { OUTPUT }
C        INTEGER*4       DB_HANDLE,      { INPUT }
C        INTEGER*4       TRA_HANDLE,     { INPUT }
C        INTEGER*2       NAME_LENGTH,    { INPUT }
C        CHARACTER* (*)  PROC_NAME,      { INPUT }
C        INTEGER*2       BLR_LENGTH,     { INPUT }
C        CHARACTER* (*)  BLR,             { INPUT }
C        INTEGER*2       IN_MESSAGE_LENGTH, { INPUT }
C        CHARACTER* (*)  IN_MESSAGE,        { INPUT }
C        INTEGER*2       OUT_MESSAGE_LENGTH, { INPUT }
C        CHARACTER* (*)  OUT_MESSAGE        { INPUT / OUTPUT }
C        )

       INTEGER*2 GDS__TRUE, GDS__FALSE
       PARAMETER (
     +    GDS__TRUE = 1,
     +    GDS__FALSE = 0)

       INTEGER*2 ISC_TRUE, ISC_FALSE
       PARAMETER (
     +    ISC_TRUE = 1,
     +    ISC_FALSE = 0)

       INTEGER*2  BLR_BLOB
       PARAMETER  (BLR_BLOB = 261)

       CHARACTER*1 
     +    BLR_TEXT, BLR_SHORT, BLR_LONG, BLR_QUAD, BLR_FLOAT,
     +    BLR_DOUBLE, BLR_D_FLOAT, BLR_DATE, BLR_VARYING, BLR_CSTRING,
     +    BLR_VERSION4, BLR_EOC, BLR_END

       PARAMETER (
     +    BLR_TEXT = CHAR(14),
     +    BLR_SHORT = CHAR(7),
     +    BLR_LONG = CHAR(8),
     +    BLR_QUAD = CHAR(9),
     +    BLR_FLOAT = CHAR(10),
     +    BLR_DOUBLE = CHAR(27),
     +    BLR_D_FLOAT = CHAR(11),
     +    BLR_DATE = CHAR(35),
     +    BLR_VARYING = CHAR(37),
     +    BLR_CSTRING = CHAR(40),
     +    BLR_VERSION4 = CHAR(4),
     +    BLR_EOC = CHAR(76),
     +    BLR_END = CHAR(255))

      CHARACTER*1
     +    BLR_ASSIGNMENT, BLR_BEGIN, BLR_MESSAGE, BLR_ERASE, BLR_FETCH,
     +    BLR_FOR, BLR_IF, BLR_LOOP, BLR_MODIFY, BLR_HANDLER, 
     +    BLR_RECEIVE, BLR_SELECT, BLR_SEND, BLR_STORE, BLR_LABEL,
     +    BLR_LEAVE


      PARAMETER (
     +    BLR_ASSIGNMENT = CHAR(1),
     +    BLR_BEGIN = CHAR(2),
     +    BLR_MESSAGE = CHAR(4),
     +    BLR_ERASE = CHAR(5),
     +    BLR_FETCH = CHAR(6),
     +    BLR_FOR = CHAR(7),
     +    BLR_IF = CHAR(8),
     +    BLR_LOOP = CHAR(9),
     +    BLR_MODIFY = CHAR(10),
     +    BLR_HANDLER = CHAR(11),
     +    BLR_RECEIVE = CHAR(12),
     +    BLR_SELECT = CHAR(13),
     +    BLR_SEND = CHAR(14),
     +    BLR_STORE = CHAR(15),
     +    BLR_LABEL = CHAR(17),
     +    BLR_LEAVE = CHAR(18))

      CHARACTER*1
     +    BLR_LITERAL, BLR_DBKEY, BLR_FIELD, BLR_FID, BLR_PARAMETER,
     +    BLR_VARIABLE, BLR_AVERAGE, BLR_COUNT, BLR_MAXIMUM, 
     +    BLR_MINIMUM, BLR_TOTAL

      PARAMETER (
     +    BLR_LITERAL = CHAR(21),
     +    BLR_DBKEY = CHAR(22),
     +    BLR_FIELD = CHAR(23),
     +    BLR_FID = CHAR(24),
     +    BLR_PARAMETER = CHAR(25),
     +    BLR_VARIABLE = CHAR(26),
     +    BLR_AVERAGE = CHAR(27),
     +    BLR_COUNT = CHAR(28),
     +    BLR_MAXIMUM = CHAR(29),
     +    BLR_MINIMUM = CHAR(30),
     +    BLR_TOTAL = CHAR(31))

      CHARACTER*1
     +    BLR_ADD, BLR_SUBTRACT, BLR_MULTIPLY, BLR_DIVIDE,
     +    BLR_NEGATE, BLR_CONCATENATE, BLR_SUBSTRING, 
     +    BLR_PARAMETER2, BLR_FROM, BLR_VIA, BLR_NULL
     

      PARAMETER (
     +    BLR_ADD = CHAR(34),
     +    BLR_SUBTRACT = CHAR(35),
     +    BLR_MULTIPLY = CHAR(36),
     +    BLR_DIVIDE = CHAR(37),
     +    BLR_NEGATE = CHAR(38),
     +    BLR_CONCATENATE = CHAR(39),
     +    BLR_SUBSTRING = CHAR(40),
     +    BLR_PARAMETER2 = CHAR(41),
     +    BLR_FROM = CHAR(42),
     +    BLR_VIA = CHAR(43),
     +    BLR_NULL = CHAR(45))

      CHARACTER*1
     +    BLR_EQL, BLR_NEQ, BLR_GTR, BLR_GEQ, BLR_LSS, BLR_LEQ, 
     +    BLR_CONTAINING, BLR_MATCHING, BLR_STARTING, BLR_BETWEEN, 
     +    BLR_OR, BLR_AND, BLR_NOT, BLR_ANY, BLR_MISSING, BLR_LIKE,
     +    BLR_UNIQUE

       PARAMETER (
     +    BLR_EQL = CHAR(47),
     +    BLR_NEQ = CHAR(48),
     +    BLR_GTR = CHAR(49),
     +    BLR_GEQ = CHAR(50),
     +    BLR_LSS = CHAR(51),
     +    BLR_LEQ = CHAR(52),
     +    BLR_CONTAINING = CHAR(53),
     +    BLR_MATCHING = CHAR(54),
     +    BLR_STARTING = CHAR(55),
     +    BLR_BETWEEN = CHAR(56),
     +    BLR_OR = CHAR(57),
     +    BLR_AND = CHAR(58),
     +    BLR_NOT = CHAR(59),
     +    BLR_ANY = CHAR(60),
     +    BLR_MISSING = CHAR(61),
     +    BLR_UNIQUE = CHAR(62),
     +    BLR_LIKE = CHAR(63))

      CHARACTER*1
     +    BLR_RSE, BLR_FIRST, BLR_PROJECT, BLR_SORT, BLR_BOOLEAN, 
     +    BLR_ASCENDING, BLR_DESCENDING, BLR_RELATION, BLR_RID,
     +    BLR_UNION, BLR_MAP, BLR_GROUP_BY, BLR_AGGREGATE,
     +    BLR_JOIN_TYPE


      PARAMETER (
     +    BLR_RSE = CHAR(67),
     +    BLR_FIRST = CHAR(68),
     +    BLR_PROJECT = CHAR(69),
     +    BLR_SORT = CHAR(70),
     +    BLR_BOOLEAN = CHAR(71),
     +    BLR_ASCENDING = CHAR(72),
     +    BLR_DESCENDING = CHAR(73),
     +    BLR_RELATION = CHAR(74),
     +    BLR_RID = CHAR(75),
     +    BLR_UNION = CHAR(76),
     +    BLR_MAP = CHAR(77),
     +    BLR_GROUP_BY = CHAR(78),     
     +    BLR_AGGREGATE = CHAR(79),
     +    BLR_JOIN_TYPE = CHAR(80))

      CHARACTER*1
     +    BLR_INNER, BLR_LEFT, BLR_RIGHT, BLR_FULL

      PARAMETER (
     +    BLR_INNER = CHAR(0),
     +    BLR_LEFT = CHAR(1),
     +    BLR_RIGHT = CHAR(2),
     +    BLR_FULL = CHAR(3))

      CHARACTER*1
     +    BLR_AGG_COUNT, BLR_AGG_MAX, BLR_AGG_MIN, BLR_AGG_TOTAL,
     +    BLR_AGG_AVERAGE, BLR_RUN_COUNT, BLR_RUN_MAX, BLR_RUN_MIN,
     +    BLR_RUN_TOTAL, BLR_RUN_AVERAGE

      PARAMETER (
     +    BLR_AGG_COUNT = CHAR(83),     
     +    BLR_AGG_MAX = CHAR(84),
     +    BLR_AGG_MIN = CHAR(85),
     +    BLR_AGG_TOTAL = CHAR(86),
     +    BLR_AGG_AVERAGE = CHAR(87),
     +    BLR_RUN_COUNT = CHAR(118),
     +    BLR_RUN_MAX = CHAR(89),
     +    BLR_RUN_MIN = CHAR(90),
     +    BLR_RUN_TOTAL = CHAR(91),
     +    BLR_RUN_AVERAGE = CHAR(92)) 

      CHARACTER*1
     +    BLR_FUNCTION, BLR_GEN_ID, BLR_PROT_MASK, BLR_UPCASE,
     +    BLR_LOCK_STATE, BLR_VALUE_IF, BLR_MATCHING2, BLR_INDEX

      PARAMETER (
     +    BLR_FUNCTION = CHAR(100),
     +    BLR_GEN_ID = CHAR(101),
     +    BLR_PROT_MASK = CHAR(102),
     +    BLR_UPCASE = CHAR(103),
     +    BLR_LOCK_STATE = CHAR(104),
     +    BLR_VALUE_IF = CHAR(105),
     +    BLR_MATCHING2 = CHAR(106),
     +    BLR_INDEX = CHAR(107))

      CHARACTER*1
     +    BLR_ANSI_LIKE, BLR_BOOKMARK, BLR_CRACK,
     +    BLR_FORCE_CRACK, BLR_SEEK, BLR_FIND,
     +    BLR_LOCK_RELATION, BLR_LOCK_RECORD,
     +    BLR_SET_BOOKMARK, BLR_GET_BOOKMARK

      PARAMETER (
     +    BLR_ANSI_LIKE = CHAR(108),
     +    BLR_BOOKMARK = CHAR(109),
     +    BLR_CRACK = CHAR(110),
     +    BLR_FORCE_CRACK = CHAR(111),
     +    BLR_SEEK = CHAR(112),
     +    BLR_FIND = CHAR(113),
     +    BLR_LOCK_RELATION = CHAR(114),
     +    BLR_LOCK_RECORD = CHAR(115),
     +    BLR_SET_BOOKMARK = CHAR(116),
     +    BLR_GET_BOOKMARK = CHAR(117))

      CHARACTER*1
     +    BLR_RS_STREAM, BLR_EXEC_PROC, BLR_BEGIN_RANGE,
     +    BLR_END_RANGE, BLR_DELETE_RANGE, BLR_PROCEDURE,
     +    BLR_PID, BLR_EXEC_PID, BLR_SINGULAR, BLR_ABORT,
     +    BLR_BLOCK, BLR_ERROR_HANDLER, BLR_PARAMETER3,
     +    BLR_CAST

      PARAMETER (
     +    BLR_PARAMETER3 = CHAR(88),
     +    BLR_RS_STREAM = CHAR(119),
     +    BLR_EXEC_PROC = CHAR(120),
     +    BLR_BEGIN_RANGE = CHAR(121),
     +    BLR_END_RANGE = CHAR(122),
     +    BLR_DELETE_RANGE = CHAR(123),
     +    BLR_PROCEDURE = CHAR(124),
     +    BLR_PID = CHAR(125),
     +    BLR_EXEC_PID = CHAR(126),
     +    BLR_SINGULAR = CHAR(127),
     +    BLR_ABORT = CHAR(128),
     +    BLR_BLOCK = CHAR(129),
     +    BLR_ERROR_HANDLER = CHAR(130),
     +    BLR_CAST = CHAR(131))

      CHARACTER*1
     +    BLR_GDS_CODE, BLR_SQL_CODE, BLR_EXCEPTION,
     +    BLR_TRIGGER_CODE, BLR_DEFAULT_CODE

      PARAMETER (
     +    BLR_GDS_CODE = CHAR(0),
     +    BLR_SQL_CODE = CHAR(1),
     +    BLR_EXCEPTION = CHAR(2),
     +    BLR_TRIGGER_CODE = CHAR(3),
     +    BLR_DEFAULT_CODE = CHAR(4))

C     database parameter block stuff 

      CHARACTER*1
     +     GDS__DPB_VERSION1,
     +     GDS__DPB_CDD_PATHNAME, ISC_DPB_ALLOCATION,
     +     GDS__DPB_JOURNAL, ISC_DPB_PAGE_SIZE,
     +     GDS__DPB_BUFFERS,  ISC_DPB_BUFFER_LENGTH,
     +     GDS__DPB_DEBUG, ISC_DPB_GARBAGE_COLLECT,
     +     GDS__DPB_VERIFY,  ISC_DPB_SWEEP,
     +     GDS__DPB_ENABLE_JOURNAL, ISC_DPB_DISABLE_JOURNAL,
     +     GDS__DPB_DBKEY_SCOPE, ISC_DPB_NUMBER_OF_USERS,
     +     GDS__DPB_TRACE, ISC_DPB_NO_GARBAGE_COLLECT,
     +     GDS__DPB_DAMAGED, ISC_DPB_LICENSE, ISC_DPB_SYS_USER_NAME,
     +     GDS__DPB_ENCRYPT_KEY, ISC_DPB_ACTIVATE_SHADOW, ISC_DPB_SWEEP_INTERVAL,
     +     GDS__DPB_DELETE_SHADOW, ISC_DPB_FORCE_WRITE, ISC_DPB_BEGIN_LOG, 
     +     GDS__DPB_QUIT_LOG, ISC_DPB_NO_RESERVE, ISC_DPB_USER_NAME,
     +     GDS__DPB_PASSWORD, ISC_DPB_PASSWORD_ENC, ISC_DPB_SYS_USER_NAME_ENC,
     +     GDS__DPB_INTERP, ISC_DPB_ONLINE_DUMP, ISC_DPB_OLD_FILE_SIZE,
     +     GDS__DPB_OLD_NUM_FILES, ISC_DPB_OLD_FILE, ISC_DPB_OLD_START_PAGE,
     +     GDS__DPB_OLD_START_SEQNO, ISC_DPB_OLD_START_FILE, ISC_DPB_DROP_WALFILE,
     +     GDS__DPB_OLD_DUMP_ID, ISC_DPB_WAL_BACKUP_DIR, ISC_DPB_WAL_CHKPTLEN,
     +     GDS__DPB_WAL_NUMBUFS, ISC_DPB_WAL_BUFSIZE, ISC_DPB_WAL_GRP_CMT_WAIT,
     +     GDS__DPB_LC_MESSAGES, ISC_DPB_LC_CTYPE, ISC_DPB_CACHE_MANAGER,
     +     GDS__DPB_SHUTDOWN, ISC_DPB_ONLINE, ISC_DPB_SHUTDOWN_DELAY,
     +     GDS__DPB_RESERVED, ISC_DPB_OVERWRITE, ISC_DPB_SEC_ATTACH,
     +     GDS__DPB_DISABLE_WAL

      PARAMETER (
     +     GDS__DPB_VERSION1              = CHAR (1),
     +     GDS__DPB_CDD_PATHNAME          = CHAR (1),    !{ NOT IMPLEMENTED }
     +     GDS__DPB_ALLOCATION            = CHAR (2),    !{ NOT IMPLEMENTED }
     +     GDS__DPB_JOURNAL               = CHAR (3),    !{ NOT IMPLEMENTED }
     +     GDS__DPB_PAGE_SIZE             = CHAR (4),
     +     GDS__DPB_BUFFERS               = CHAR (5),
     +     GDS__DPB_BUFFER_LENGTH         = CHAR (6),    !{ NOT IMPLEMENTED }
     +     GDS__DPB_DEBUG                 = CHAR (7),    !{ NOT IMPLEMENTED }
     +     GDS__DPB_GARBAGE_COLLECT       = CHAR (8),    !{ NOT IMPLEMENTED }
     +     GDS__DPB_VERIFY                = CHAR (9),
     +     GDS__DPB_SWEEP                 = CHAR (10),
     +     GDS__DPB_ENABLE_JOURNAL        = CHAR (11),
     +     GDS__DPB_DISABLE_JOURNAL       = CHAR (12),
     +     GDS__DPB_DBKEY_SCOPE           = CHAR (13),    !{ NOT IMPLEMENTED }
     +     GDS__DPB_NUMBER_OF_USERS       = CHAR (14),    !{ NOT IMPLEMENTED }
     +     GDS__DPB_TRACE                 = CHAR (15),    !{ NOT IMPLEMENTED }
     +     GDS__DPB_NO_GARBAGE_COLLECT    = CHAR (16),    !{ NOT IMPLEMENTED }
     +     GDS__DPB_DAMAGED               = CHAR (17),    !{ NOT IMPLEMENTED }
     +     GDS__DPB_LICENSE               = CHAR (18),
     +     GDS__DPB_SYS_USER_NAME         = CHAR (19))

      PARAMETER (
     +     GDS__DPB_ENCRYPT_KEY           = CHAR (20),
     +     GDS__DPB_ACTIVATE_SHADOW       = CHAR (21),
     +     GDS__DPB_SWEEP_INTERVAL        = CHAR (22),
     +     GDS__DPB_DELETE_SHADOW         = CHAR (23))


      PARAMETER (
     +     GDS__DPB_FORCE_WRITE           = CHAR (24),
     +     GDS__DPB_BEGIN_LOG             = CHAR (25),
     +     GDS__DPB_QUIT_LOG              = CHAR (26),
     +     GDS__DPB_NO_RESERVE            = CHAR (27),
     +     GDS__DPB_USER_NAME             = CHAR (28),
     +     GDS__DPB_PASSWORD              = CHAR (29),
     +     GDS__DPB_PASSWORD_ENC          = CHAR (30),
     +     GDS__DPB_SYS_USER_NAME_ENC     = CHAR (31),
     +     GDS__DPB_INTERP                = CHAR (32),
     +     GDS__DPB_ONLINE_DUMP           = CHAR (33),
     +     GDS__DPB_OLD_FILE_SIZE         = CHAR (34),
     +     GDS__DPB_OLD_NUM_FILES         = CHAR (35),
     +     GDS__DPB_OLD_FILE              = CHAR (36),
     +     GDS__DPB_OLD_START_PAGE        = CHAR (37),
     +     GDS__DPB_OLD_START_SEQNO       = CHAR (38),
     +     GDS__DPB_OLD_START_FILE        = CHAR (39),
     +     GDS__DPB_DROP_WALFILE          = CHAR (40),
     +     GDS__DPB_OLD_DUMP_ID           = CHAR (41),
     +     GDS__DPB_WAL_BACKUP_DIR        = CHAR (42),
     +     GDS__DPB_WAL_CHKPTLEN          = CHAR (43),
     +     GDS__DPB_WAL_NUMBUFS           = CHAR (44),
     +     GDS__DPB_WAL_BUFSIZE           = CHAR (45),
     +     GDS__DPB_WAL_GRP_CMT_WAIT      = CHAR (46),
     +     GDS__DPB_LC_MESSAGES           = CHAR (47),
     +     GDS__DPB_LC_CTYPE              = CHAR (48),
     +     GDS__DPB_CACHE_MANAGER	 = CHAR (49),
     +     GDS__DPB_SHUTDOWN		 = CHAR (50),
     +     GDS__DPB_ONLINE		 = CHAR (51),
     +     GDS__DPB_SHUTDOWN_DELAY	 = CHAR (52),
     +     GDS__DPB_RESERVED		 = CHAR (53),
     +     GDS__DPB_OVERWRITE		 = CHAR (54),
     +     GDS__DPB_SEC_ATTACH		 = CHAR (55),
     +     GDS__DPB_DISABLE_WAL		 = CHAR (56))

      CHARACTER*1
     +     GDS__DPB_PAGES, GDS__DPB_RECORDS, 
     +     GDS__DPB_INDICES, GDS__DPB_TRANSACTIONS, 
     +     GDS__DPB_NO_UPDATE, GDS__DPB_REPAIR

      PARAMETER (
     +     GDS__DPB_PAGES = CHAR(1),
     +     GDS__DPB_RECORDS = CHAR(2),
     +     GDS__DPB_INDICES = CHAR(4),
     +     GDS__DPB_TRANSACTIONS = CHAR(8),
     +     GDS__DPB_NO_UPDATE = CHAR(16),
     +     GDS__DPB_REPAIR = CHAR(32))



C     Transaction parameter block stuff 

      CHARACTER*1
     + GDS__TPB_VERSION3, GDS__TPB_CONSISTENCY, GDS__TPB_CONCURRENCY,
     + GDS__TPB_SHARED, GDS__TPB_PROTECTED, GDS__TPB_EXCLUSIVE,
     + GDS__TPB_WAIT, GDS__TPB_NOWAIT, GDS__TPB_READ, GDS__TPB_WRITE,
     + GDS__TPB_LOCK_READ, GDS__TPB_LOCK_WRITE, GDS__TPB_VERB_TIME,
     + GDS__TPB_COMMIT_TIME                            


      PARAMETER (
     + GDS__TPB_VERSION3     = CHAR (3),
     + GDS__TPB_CONSISTENCY  = CHAR (1),
     + GDS__TPB_CONCURRENCY  = CHAR (2),
     + GDS__TPB_SHARED       = CHAR (3),
     + GDS__TPB_PROTECTED    = CHAR (4),
     + GDS__TPB_EXCLUSIVE    = CHAR (5),
     + GDS__TPB_WAIT         = CHAR (6),
     + GDS__TPB_NOWAIT       = CHAR (7),
     + GDS__TPB_READ         = CHAR (8),
     + GDS__TPB_WRITE        = CHAR (9),
     + GDS__TPB_LOCK_READ    = CHAR (10),
     + GDS__TPB_LOCK_WRITE   = CHAR (11),
     + GDS__TPB_VERB_TIME    = CHAR (12),
     + GDS__TPB_COMMIT_TIME  = CHAR (13))

C Blob Parameter Block

      CHARACTER*1
     +  gds__bpb_version1, gds__bpb_source_type, gds__bpb_target_type,
     +  gds__bpb_type, gds__bpb_type_segmented, gds__bpb_type_stream

      PARAMETER (
     +  gds__bpb_version1           = CHAR (1),
     +  gds__bpb_source_type        = CHAR (1),
     +  gds__bpb_target_type        = CHAR (2),
     +  gds__bpb_type               = CHAR (3),

     +  gds__bpb_type_segmented     = CHAR (0),
     +  gds__bpb_type_stream        = CHAR (1))

C     Information parameters

C Common, structural codes 

      CHARACTER*1  GDS__INFO_END, GDS__INFO_TRUNCATED,
     +    GDS__INFO_ERROR

      PARAMETER (
     +    GDS__INFO_END                  = CHAR(1),
     +    GDS__INFO_TRUNCATED            = CHAR(2),
     +    GDS__INFO_ERROR                = CHAR(3))

C DATABASE INFORMATION ITEMS 

      CHARACTER*1 GDS__INFO_ID, GDS__INFO_READS, GDS__INFO_WRITES,
     +    GDS__INFO_FETCHES, GDS__INFO_MARKS, GDS__INFO_IMPLEMENTATION,
     +    GDS__INFO_VERSION, GDS__INFO_BASE_LEVEL, GDS__INFO_PAGE_SIZE,
     +    GDS__INFO_NUM_BUFFERS, GDS__INFO_LIMBO,
     +    GDS__INFO_CURRENT_MEMORY, GDS__INFO_MAX_MEMORY,
     +    GDS__INFO_WINDOW_TURNS, GDS__INFO_DB_ID

      PARAMETER (
     +    GDS__INFO_DB_ID                = CHAR(4),
     +    GDS__INFO_READS                = CHAR(5),
     +    GDS__INFO_WRITES               = CHAR(6),
     +    GDS__INFO_FETCHES              = CHAR(7),
     +    GDS__INFO_MARKS                = CHAR(8),
     +    GDS__INFO_IMPLEMENTATION       = CHAR(11),
     +    GDS__INFO_VERSION              = CHAR(12),
     +    GDS__INFO_BASE_LEVEL           = CHAR(13),
     +    GDS__INFO_PAGE_SIZE            = CHAR(14),
     +    GDS__INFO_NUM_BUFFERS          = CHAR(15),
     +    GDS__INFO_LIMBO                = CHAR(16),
     +    GDS__INFO_CURRENT_MEMORY       = CHAR(17),
     +    GDS__INFO_MAX_MEMORY           = CHAR(18),
     +    GDS__INFO_WINDOW_TURNS         = CHAR(19))

      CHARACTER*1 GDS__INFO_LICENSE, GDS__INFO_ALLOCATION,
     +    GDS__INFO_ATTACHMENT_ID, GDS__INFO_READ_SEQ_COUNT,
     +    GDS__INFO_READ_IDX_COUNT, GDS__INFO_INSERT_COUNT,
     +    GDS__INFO_UPDATE_COUNT,
     +    GDS__INFO_DELETE_COUNT, GDS__INFO_BACKOUT_COUNT,
     +    GDS__INFO_PURGE_COUNT, GDS__INFO_EXPUNGE_COUNT,
     +    GDS__INFO_SWEEP_INTERVAL, GDS__INFO_ODS_VERSION,
     +    GDS__INFO_ODS_MINOR_VERSION

      PARAMETER (
     +    GDS__INFO_LICENSE              = CHAR(20),
     +    GDS__INFO_ALLOCATION           = CHAR(21),
     +    GDS__INFO_ATTACHMENT_ID        = CHAR(22),
     +    GDS__INFO_READ_SEQ_COUNT       = CHAR(23),
     +    GDS__INFO_READ_IDX_COUNT       = CHAR(24), 
     +    GDS__INFO_INSERT_COUNT          = CHAR(25),
     +    GDS__INFO_UPDATE_COUNT         = CHAR(26),
     +    GDS__INFO_DELETE_COUNT         = CHAR(27), 
     +    GDS__INFO_BACKOUT_COUNT        = CHAR(28),
     +    GDS__INFO_PURGE_COUNT          = CHAR(29), 
     +    GDS__INFO_EXPUNGE_COUNT        = CHAR(30),
     +    GDS__INFO_SWEEP_INTERVAL       = CHAR(31), 
     +    GDS__INFO_ODS_VERSION          = CHAR(32),
     +    GDS__INFO_ODS_MINOR_VERSION    = CHAR(33))


C REQUEST INFORMATION ITEMS 

      CHARACTER*1 GDS__INFO_NUMBER_MESSAGES, GDS__INFO_MAX_MESSAGE,
     +    GDS__INFO_MAX_SEND, GDS__INFO_MAX_RECEIVE, GDS__INFO_STATE,
     +    GDS__INFO_MESSAGE_NUMBER, GDS__INFO_MESSAGE_SIZE,
     +    GDS__INFO_REQUEST_COST, GDS__INFO_REQ_ACTIVE,
     +    GDS__INFO_REQ_INACTIVE, GDS__INFO_REQ_SEND,
     +    GDS__INFO_REQ_RECEIVE, GDS__INFO_REQ_SELECT

      PARAMETER (
     +    GDS__INFO_NUMBER_MESSAGES     = CHAR(4),
     +    GDS__INFO_MAX_MESSAGE         = CHAR(5),
     +    GDS__INFO_MAX_SEND            = CHAR(6),
     +    GDS__INFO_MAX_RECEIVE         = CHAR(7),
     +    GDS__INFO_STATE               = CHAR(8),
     +    GDS__INFO_MESSAGE_NUMBER      = CHAR(9),
     +    GDS__INFO_MESSAGE_SIZE        = CHAR(10),
     +    GDS__INFO_REQUEST_COST        = CHAR(11),
     +    GDS__INFO_REQ_ACTIVE          = CHAR(2),
     +    GDS__INFO_REQ_INACTIVE        = CHAR(3),
     +    GDS__INFO_REQ_SEND            = CHAR(4),
     +    GDS__INFO_REQ_RECEIVE         = CHAR(5),
     +    GDS__INFO_REQ_SELECT          = CHAR(6))

C BLOB INFORMATION ITEMS 

      CHARACTER*1 GDS__INFO_BLOB_NUM_SEGMENTS,
     +    GDS__INFO_BLOB_MAX_SEGMENT,
     +    GDS__INFO_BLOB_TOTAL_LENGTH,
     +    GDS__INFO_BLOB_TYPE

      PARAMETER (
     +    GDS__INFO_BLOB_NUM_SEGMENTS   = CHAR(4),
     +    GDS__INFO_BLOB_MAX_SEGMENT    = CHAR(5),
     +    GDS__INFO_BLOB_TOTAL_LENGTH   = CHAR(6),
     +    GDS__INFO_BLOB_TYPE           = CHAR(7))

C TRANSACTION INFORMATION ITEMS 

      CHARACTER*1 GDS__INFO_TRA_ID
      PARAMETER (
     +    GDS__INFO_TRA_ID               = CHAR(4))

C Dynamic SQL datatypes

      INTEGER*2
     + SQL_TEXT,
     + SQL_VARYING,
     + SQL_SHORT,
     + SQL_LONG,
     + SQL_FLOAT,
     + SQL_DOUBLE,
     + SQL_D_FLOAT,
     + SQL_DATE,
     + SQL_BLOB

      PARAMETER (
     + SQL_TEXT = 452,
     + SQL_VARYING = 448,
     + SQL_SHORT = 500,
     + SQL_LONG = 496,
     + SQL_FLOAT = 482,
     + SQL_DOUBLE = 480,
     + SQL_D_FLOAT = 530,
     + SQL_DATE = 510,
     + SQL_BLOB = 520)


C     database parameter block stuff 

      CHARACTER*1
     +     ISC_DPB_VERSION1,
     +     ISC_DPB_CDD_PATHNAME, ISC_DPB_ALLOCATION,
     +     ISC_DPB_JOURNAL, ISC_DPB_PAGE_SIZE,
     +     ISC_DPB_BUFFERS,  ISC_DPB_BUFFER_LENGTH,
     +     ISC_DPB_DEBUG, ISC_DPB_GARBAGE_COLLECT,
     +     ISC_DPB_VERIFY,  ISC_DPB_SWEEP,
     +     ISC_DPB_ENABLE_JOURNAL, ISC_DPB_DISABLE_JOURNAL,
     +     ISC_DPB_DBKEY_SCOPE, ISC_DPB_NUMBER_OF_USERS,
     +     ISC_DPB_TRACE, ISC_DPB_NO_GARBAGE_COLLECT,
     +     ISC_DPB_DAMAGED, ISC_DPB_LICENSE, ISC_DPB_SYS_USER_NAME,
     +     ISC_DPB_ENCRYPT_KEY, ISC_DPB_ACTIVATE_SHADOW, ISC_DPB_SWEEP_INTERVAL,
     +     ISC_DPB_DELETE_SHADOW, ISC_DPB_FORCE_WRITE, ISC_DPB_BEGIN_LOG, 
     +     ISC_DPB_QUIT_LOG, ISC_DPB_NO_RESERVE, ISC_DPB_USER_NAME,
     +     ISC_DPB_PASSWORD, ISC_DPB_PASSWORD_ENC, ISC_DPB_SYS_USER_NAME_ENC,
     +     ISC_DPB_INTERP, ISC_DPB_ONLINE_DUMP, ISC_DPB_OLD_FILE_SIZE,
     +     ISC_DPB_OLD_NUM_FILES, ISC_DPB_OLD_FILE, ISC_DPB_OLD_START_PAGE,
     +     ISC_DPB_OLD_START_SEQNO, ISC_DPB_OLD_START_FILE, ISC_DPB_DROP_WALFILE,
     +     ISC_DPB_OLD_DUMP_ID, ISC_DPB_WAL_BACKUP_DIR, ISC_DPB_WAL_CHKPTLEN,
     +     ISC_DPB_WAL_NUMBUFS, ISC_DPB_WAL_BUFSIZE, ISC_DPB_WAL_GRP_CMT_WAIT,
     +     ISC_DPB_LC_MESSAGES, ISC_DPB_LC_CTYPE, ISC_DPB_CACHE_MANAGER,
     +     ISC_DPB_SHUTDOWN, ISC_DPB_ONLINE, ISC_DPB_SHUTDOWN_DELAY,
     +     ISC_DPB_RESERVED, ISC_DPB_OVERWRITE, ISC_DPB_SEC_ATTACH,
     +     ISC_DPB_DISABLE_WAL

      PARAMETER (
     +     ISC_DPB_VERSION1              = CHAR (1),
     +     ISC_DPB_CDD_PATHNAME          = CHAR (1),    !{ NOT IMPLEMENTED }
     +     ISC_DPB_ALLOCATION            = CHAR (2),    !{ NOT IMPLEMENTED }
     +     ISC_DPB_JOURNAL               = CHAR (3),    !{ NOT IMPLEMENTED }
     +     ISC_DPB_PAGE_SIZE             = CHAR (4),
     +     ISC_DPB_BUFFERS               = CHAR (5),
     +     ISC_DPB_BUFFER_LENGTH         = CHAR (6),    !{ NOT IMPLEMENTED }
     +     ISC_DPB_DEBUG                 = CHAR (7),    !{ NOT IMPLEMENTED }
     +     ISC_DPB_GARBAGE_COLLECT       = CHAR (8),    !{ NOT IMPLEMENTED }
     +     ISC_DPB_VERIFY                = CHAR (9),
     +     ISC_DPB_SWEEP                 = CHAR (10),
     +     ISC_DPB_ENABLE_JOURNAL        = CHAR (11),
     +     ISC_DPB_DISABLE_JOURNAL       = CHAR (12),
     +     ISC_DPB_DBKEY_SCOPE           = CHAR (13),    !{ NOT IMPLEMENTED }
     +     ISC_DPB_NUMBER_OF_USERS       = CHAR (14),    !{ NOT IMPLEMENTED }
     +     ISC_DPB_TRACE                 = CHAR (15),    !{ NOT IMPLEMENTED }
     +     ISC_DPB_NO_GARBAGE_COLLECT    = CHAR (16),    !{ NOT IMPLEMENTED }
     +     ISC_DPB_DAMAGED               = CHAR (17),    !{ NOT IMPLEMENTED }
     +     ISC_DPB_LICENSE               = CHAR (18),
     +     ISC_DPB_SYS_USER_NAME         = CHAR (19))

      PARAMETER (
     +     ISC_DPB_ENCRYPT_KEY           = CHAR (20),
     +     ISC_DPB_ACTIVATE_SHADOW       = CHAR (21),
     +     ISC_DPB_SWEEP_INTERVAL        = CHAR (22),
     +     ISC_DPB_DELETE_SHADOW         = CHAR (23))


      PARAMETER (
     +     ISC_DPB_FORCE_WRITE           = CHAR (24),
     +     ISC_DPB_BEGIN_LOG             = CHAR (25),
     +     ISC_DPB_QUIT_LOG              = CHAR (26),
     +     ISC_DPB_NO_RESERVE            = CHAR (27),
     +     ISC_DPB_USER_NAME             = CHAR (28),
     +     ISC_DPB_PASSWORD              = CHAR (29),
     +     ISC_DPB_PASSWORD_ENC          = CHAR (30),
     +     ISC_DPB_SYS_USER_NAME_ENC     = CHAR (31),
     +     ISC_DPB_INTERP                = CHAR (32),
     +     ISC_DPB_ONLINE_DUMP           = CHAR (33),
     +     ISC_DPB_OLD_FILE_SIZE         = CHAR (34),
     +     ISC_DPB_OLD_NUM_FILES         = CHAR (35),
     +     ISC_DPB_OLD_FILE              = CHAR (36),
     +     ISC_DPB_OLD_START_PAGE        = CHAR (37),
     +     ISC_DPB_OLD_START_SEQNO       = CHAR (38),
     +     ISC_DPB_OLD_START_FILE        = CHAR (39),
     +     ISC_DPB_DROP_WALFILE          = CHAR (40),
     +     ISC_DPB_OLD_DUMP_ID           = CHAR (41),
     +     ISC_DPB_WAL_BACKUP_DIR        = CHAR (42),
     +     ISC_DPB_WAL_CHKPTLEN          = CHAR (43),
     +     ISC_DPB_WAL_NUMBUFS           = CHAR (44),
     +     ISC_DPB_WAL_BUFSIZE           = CHAR (45),
     +     ISC_DPB_WAL_GRP_CMT_WAIT      = CHAR (46),
     +     ISC_DPB_LC_MESSAGES           = CHAR (47),
     +     ISC_DPB_LC_CTYPE              = CHAR (48),
     +     ISC_DPB_CACHE_MANAGER	 = CHAR (49),
     +     ISC_DPB_SHUTDOWN		 = CHAR (50),
     +     ISC_DPB_ONLINE		 = CHAR (51),
     +     ISC_DPB_SHUTDOWN_DELAY	 = CHAR (52),
     +     ISC_DPB_RESERVED		 = CHAR (53),
     +     ISC_DPB_OVERWRITE		 = CHAR (54),
     +     ISC_DPB_SEC_ATTACH		 = CHAR (55),
     +     ISC_DPB_DISABLE_WAL		 = CHAR (56))

      CHARACTER*1
     +     ISC_DPB_PAGES, ISC_DPB_RECORDS, 
     +     ISC_DPB_INDICES, ISC_DPB_TRANSACTIONS, 
     +     ISC_DPB_NO_UPDATE, ISC_DPB_REPAIR

      PARAMETER (
     +     ISC_DPB_PAGES = CHAR(1),
     +     ISC_DPB_RECORDS = CHAR(2),
     +     ISC_DPB_INDICES = CHAR(4),
     +     ISC_DPB_TRANSACTIONS = CHAR(8),
     +     ISC_DPB_NO_UPDATE = CHAR(16),
     +     ISC_DPB_REPAIR = CHAR(32))



C     Transaction parameter block stuff 

      CHARACTER*1
     + ISC_TPB_VERSION3, ISC_TPB_CONSISTENCY, ISC_TPB_CONCURRENCY,
     + ISC_TPB_SHARED, ISC_TPB_PROTECTED, ISC_TPB_EXCLUSIVE,
     + ISC_TPB_WAIT, ISC_TPB_NOWAIT, ISC_TPB_READ, ISC_TPB_WRITE,
     + ISC_TPB_LOCK_READ, ISC_TPB_LOCK_WRITE, ISC_TPB_VERB_TIME,
     + ISC_TPB_COMMIT_TIME                            


      PARAMETER (
     + ISC_TPB_VERSION3     = CHAR (3),
     + ISC_TPB_CONSISTENCY  = CHAR (1),
     + ISC_TPB_CONCURRENCY  = CHAR (2),
     + ISC_TPB_SHARED       = CHAR (3),
     + ISC_TPB_PROTECTED    = CHAR (4),
     + ISC_TPB_EXCLUSIVE    = CHAR (5),
     + ISC_TPB_WAIT         = CHAR (6),
     + ISC_TPB_NOWAIT       = CHAR (7),
     + ISC_TPB_READ         = CHAR (8),
     + ISC_TPB_WRITE        = CHAR (9),
     + ISC_TPB_LOCK_READ    = CHAR (10),
     + ISC_TPB_LOCK_WRITE   = CHAR (11),
     + ISC_TPB_VERB_TIME    = CHAR (12),
     + ISC_TPB_COMMIT_TIME  = CHAR (13))

C Blob Parameter Block

      CHARACTER*1
     +  isc_bpb_version1, isc_bpb_source_type, isc_bpb_target_type,
     +  isc_bpb_type, isc_bpb_type_segmented, isc_bpb_type_stream

      PARAMETER (
     +  isc_bpb_version1           = CHAR (1),
     +  isc_bpb_source_type        = CHAR (1),
     +  isc_bpb_target_type        = CHAR (2),
     +  isc_bpb_type               = CHAR (3),

     +  isc_bpb_type_segmented     = CHAR (0),
     +  isc_bpb_type_stream        = CHAR (1))

C     Information parameters

C Common, structural codes 

      CHARACTER*1  ISC_INFO_END, ISC_INFO_TRUNCATED,
     +    ISC_INFO_ERROR

      PARAMETER (
     +    ISC_INFO_END                  = CHAR(1),
     +    ISC_INFO_TRUNCATED            = CHAR(2),
     +    ISC_INFO_ERROR                = CHAR(3))

C DATABASE INFORMATION ITEMS 

      CHARACTER*1 ISC_INFO_ID, ISC_INFO_READS, ISC_INFO_WRITES,
     +    ISC_INFO_FETCHES, ISC_INFO_MARKS, ISC_INFO_IMPLEMENTATION,
     +    ISC_INFO_VERSION, ISC_INFO_BASE_LEVEL, ISC_INFO_PAGE_SIZE,
     +    ISC_INFO_NUM_BUFFERS, ISC_INFO_LIMBO,
     +    ISC_INFO_CURRENT_MEMORY, ISC_INFO_MAX_MEMORY,
     +    ISC_INFO_WINDOW_TURNS, ISC_INFO_DB_ID

      PARAMETER (
     +    ISC_INFO_DB_ID                = CHAR(4),
     +    ISC_INFO_READS                = CHAR(5),
     +    ISC_INFO_WRITES               = CHAR(6),
     +    ISC_INFO_FETCHES              = CHAR(7),
     +    ISC_INFO_MARKS                = CHAR(8),
     +    ISC_INFO_IMPLEMENTATION       = CHAR(11),
     +    ISC_INFO_VERSION              = CHAR(12),
     +    ISC_INFO_BASE_LEVEL           = CHAR(13),
     +    ISC_INFO_PAGE_SIZE            = CHAR(14),
     +    ISC_INFO_NUM_BUFFERS          = CHAR(15),
     +    ISC_INFO_LIMBO                = CHAR(16),
     +    ISC_INFO_CURRENT_MEMORY       = CHAR(17),
     +    ISC_INFO_MAX_MEMORY           = CHAR(18),
     +    ISC_INFO_WINDOW_TURNS         = CHAR(19))

      CHARACTER*1 ISC_INFO_LICENSE, ISC_INFO_ALLOCATION,
     +    ISC_INFO_ATTACHMENT_ID, ISC_INFO_READ_SEQ_COUNT,
     +    ISC_INFO_READ_IDX_COUNT, ISC_INFO_INSERT_COUNT,
     +    ISC_INFO_UPDATE_COUNT,
     +    ISC_INFO_DELETE_COUNT, ISC_INFO_BACKOUT_COUNT,
     +    ISC_INFO_PURGE_COUNT, ISC_INFO_EXPUNGE_COUNT,
     +    ISC_INFO_SWEEP_INTERVAL, ISC_INFO_ODS_VERSION,
     +    ISC_INFO_ODS_MINOR_VERSION

      PARAMETER (
     +    ISC_INFO_LICENSE              = CHAR(20),
     +    ISC_INFO_ALLOCATION           = CHAR(21),
     +    ISC_INFO_ATTACHMENT_ID        = CHAR(22),
     +    ISC_INFO_READ_SEQ_COUNT       = CHAR(23),
     +    ISC_INFO_READ_IDX_COUNT       = CHAR(24), 
     +    ISC_INFO_INSERT_COUNT          = CHAR(25),
     +    ISC_INFO_UPDATE_COUNT         = CHAR(26),
     +    ISC_INFO_DELETE_COUNT         = CHAR(27), 
     +    ISC_INFO_BACKOUT_COUNT        = CHAR(28),
     +    ISC_INFO_PURGE_COUNT          = CHAR(29), 
     +    ISC_INFO_EXPUNGE_COUNT        = CHAR(30),
     +    ISC_INFO_SWEEP_INTERVAL       = CHAR(31), 
     +    ISC_INFO_ODS_VERSION          = CHAR(32),
     +    ISC_INFO_ODS_MINOR_VERSION    = CHAR(33))


C REQUEST INFORMATION ITEMS 

      CHARACTER*1 ISC_INFO_NUMBER_MESSAGES, ISC_INFO_MAX_MESSAGE,
     +    ISC_INFO_MAX_SEND, ISC_INFO_MAX_RECEIVE, ISC_INFO_STATE,
     +    ISC_INFO_MESSAGE_NUMBER, ISC_INFO_MESSAGE_SIZE,
     +    ISC_INFO_REQUEST_COST, ISC_INFO_REQ_ACTIVE,
     +    ISC_INFO_REQ_INACTIVE, ISC_INFO_REQ_SEND,
     +    ISC_INFO_REQ_RECEIVE, ISC_INFO_REQ_SELECT

      PARAMETER (
     +    ISC_INFO_NUMBER_MESSAGES     = CHAR(4),
     +    ISC_INFO_MAX_MESSAGE         = CHAR(5),
     +    ISC_INFO_MAX_SEND            = CHAR(6),
     +    ISC_INFO_MAX_RECEIVE         = CHAR(7),
     +    ISC_INFO_STATE               = CHAR(8),
     +    ISC_INFO_MESSAGE_NUMBER      = CHAR(9),
     +    ISC_INFO_MESSAGE_SIZE        = CHAR(10),
     +    ISC_INFO_REQUEST_COST        = CHAR(11),
     +    ISC_INFO_REQ_ACTIVE          = CHAR(2),
     +    ISC_INFO_REQ_INACTIVE        = CHAR(3),
     +    ISC_INFO_REQ_SEND            = CHAR(4),
     +    ISC_INFO_REQ_RECEIVE         = CHAR(5),
     +    ISC_INFO_REQ_SELECT          = CHAR(6))

C BLOB INFORMATION ITEMS 

      CHARACTER*1 ISC_INFO_BLOB_NUM_SEGMENTS,
     +    ISC_INFO_BLOB_MAX_SEGMENT,
     +    ISC_INFO_BLOB_TOTAL_LENGTH,
     +    ISC_INFO_BLOB_TYPE

      PARAMETER (
     +    ISC_INFO_BLOB_NUM_SEGMENTS   = CHAR(4),
     +    ISC_INFO_BLOB_MAX_SEGMENT    = CHAR(5),
     +    ISC_INFO_BLOB_TOTAL_LENGTH   = CHAR(6),
     +    ISC_INFO_BLOB_TYPE           = CHAR(7))

C TRANSACTION INFORMATION ITEMS 

      CHARACTER*1 ISC_INFO_TRA_ID
      PARAMETER (
     +    ISC_INFO_TRA_ID               = CHAR(4))

C     Error Codes

      INTEGER*4
     + GDS_FACILITY,
     + GDS_ERR_BASE,
     + GDS_ERR_FACTOR

      INTEGER*4
     + GDS_ARG_END,          !{ end of argument list }
     + GDS_ARG_GDS,          !{ generic DSRI status value }
     + GDS_ARG_STRING,       !{ string argument }
     + GDS_ARG_CSTRING,      !{ count & string argument }
     + GDS_ARG_NUMBER,       !{ numeric argument (long) }
     + GDS_ARG_INTERPRETED,  !{ interpreted status code (string) }
     + GDS_ARG_VMS,          !{ VAX/VMS status code (long) }
     + GDS_ARG_UNIX,         !{ UNIX error code }
     + GDS_ARG_DOMAIN       !{ Apollo/Domain error code }


      PARAMETER (
     + GDS_FACILITY    = 20,
     + GDS_ERR_BASE    = 335544320,
     + GDS_ERR_FACTOR  = 1)

      PARAMETER (
     + GDS_ARG_END          = 0,   !{ end of argument list }
     + GDS_ARG_GDS          = 1,   !{ generic DSRI status value }
     + GDS_ARG_STRING       = 2,   !{ string argument }
     + GDS_ARG_CSTRING      = 3,   !{ count & string argument }
     + GDS_ARG_NUMBER       = 4,   !{ numeric argument (long) }
     + GDS_ARG_INTERPRETED  = 5,   !{ interpreted status code (string) }
     + GDS_ARG_VMS          = 6,   !{ VAX/VMS status code (long) }
     + GDS_ARG_UNIX         = 7,   !{ UNIX error code }
     + GDS_ARG_DOMAIN       = 8)   !{ Apollo/Domain error code }

C     Error Codes

      INTEGER*4
     + ISC_FACILITY,
     + ISC_ERR_BASE,
     + ISC_ERR_FACTOR

      INTEGER*4
     + ISC_ARG_END,          !{ end of argument list }
     + ISC_ARG_GDS,          !{ generic DSRI status value }
     + ISC_ARG_STRING,       !{ string argument }
     + ISC_ARG_CSTRING,      !{ count & string argument }
     + ISC_ARG_NUMBER,       !{ numeric argument (long) }
     + ISC_ARG_INTERPRETED,  !{ interpreted status code (string) }
     + ISC_ARG_VMS,          !{ VAX/VMS status code (long) }
     + ISC_ARG_UNIX,         !{ UNIX error code }
     + ISC_ARG_DOMAIN       !{ Apollo/Domain error code }


      PARAMETER (
     + ISC_FACILITY    = 20,
     + ISC_ERR_BASE    = 335544320,
     + ISC_ERR_FACTOR  = 1)

      PARAMETER (
     + ISC_ARG_END          = 0,   !{ end of argument list }
     + ISC_ARG_GDS          = 1,   !{ generic DSRI status value }
     + ISC_ARG_STRING       = 2,   !{ string argument }
     + ISC_ARG_CSTRING      = 3,   !{ count & string argument }
     + ISC_ARG_NUMBER       = 4,   !{ numeric argument (long) }
     + ISC_ARG_INTERPRETED  = 5,   !{ interpreted status code (string) }
     + ISC_ARG_VMS          = 6,   !{ VAX/VMS status code (long) }
     + ISC_ARG_UNIX         = 7,   !{ UNIX error code }
     + ISC_ARG_DOMAIN       = 8)   !{ Apollo/Domain error code }


      INTEGER*4 GDS__arith_except        
      PARAMETER (GDS__arith_except         = 335544321)
      INTEGER*4 GDS__bad_dbkey           
      PARAMETER (GDS__bad_dbkey            = 335544322)
      INTEGER*4 GDS__bad_db_format       
      PARAMETER (GDS__bad_db_format        = 335544323)
      INTEGER*4 GDS__bad_db_handle       
      PARAMETER (GDS__bad_db_handle        = 335544324)
      INTEGER*4 GDS__bad_dpb_content     
      PARAMETER (GDS__bad_dpb_content      = 335544325)
      INTEGER*4 GDS__bad_dpb_form        
      PARAMETER (GDS__bad_dpb_form         = 335544326)
      INTEGER*4 GDS__bad_req_handle      
      PARAMETER (GDS__bad_req_handle       = 335544327)
      INTEGER*4 GDS__bad_segstr_handle   
      PARAMETER (GDS__bad_segstr_handle    = 335544328)
      INTEGER*4 GDS__bad_segstr_id       
      PARAMETER (GDS__bad_segstr_id        = 335544329)
      INTEGER*4 GDS__bad_tpb_content     
      PARAMETER (GDS__bad_tpb_content      = 335544330)
      INTEGER*4 GDS__bad_tpb_form        
      PARAMETER (GDS__bad_tpb_form         = 335544331)
      INTEGER*4 GDS__bad_trans_handle    
      PARAMETER (GDS__bad_trans_handle     = 335544332)
      INTEGER*4 GDS__bug_check           
      PARAMETER (GDS__bug_check            = 335544333)
      INTEGER*4 GDS__convert_error       
      PARAMETER (GDS__convert_error        = 335544334)
      INTEGER*4 GDS__db_corrupt          
      PARAMETER (GDS__db_corrupt           = 335544335)
      INTEGER*4 GDS__deadlock            
      PARAMETER (GDS__deadlock             = 335544336)
      INTEGER*4 GDS__excess_trans        
      PARAMETER (GDS__excess_trans         = 335544337)
      INTEGER*4 GDS__from_no_match       
      PARAMETER (GDS__from_no_match        = 335544338)
      INTEGER*4 GDS__infinap             
      PARAMETER (GDS__infinap              = 335544339)
      INTEGER*4 GDS__infona              
      PARAMETER (GDS__infona               = 335544340)
      INTEGER*4 GDS__infunk              
      PARAMETER (GDS__infunk               = 335544341)
      INTEGER*4 GDS__integ_fail          
      PARAMETER (GDS__integ_fail           = 335544342)
      INTEGER*4 GDS__invalid_blr         
      PARAMETER (GDS__invalid_blr          = 335544343)
      INTEGER*4 GDS__io_error            
      PARAMETER (GDS__io_error             = 335544344)
      INTEGER*4 GDS__lock_conflict       
      PARAMETER (GDS__lock_conflict        = 335544345)
      INTEGER*4 GDS__metadata_corrupt    
      PARAMETER (GDS__metadata_corrupt     = 335544346)
      INTEGER*4 GDS__not_valid           
      PARAMETER (GDS__not_valid            = 335544347)
      INTEGER*4 GDS__no_cur_rec          
      PARAMETER (GDS__no_cur_rec           = 335544348)
      INTEGER*4 GDS__no_dup              
      PARAMETER (GDS__no_dup               = 335544349)
      INTEGER*4 GDS__no_finish           
      PARAMETER (GDS__no_finish            = 335544350)
      INTEGER*4 GDS__no_meta_update      
      PARAMETER (GDS__no_meta_update       = 335544351)
      INTEGER*4 GDS__no_priv             
      PARAMETER (GDS__no_priv              = 335544352)
      INTEGER*4 GDS__no_recon            
      PARAMETER (GDS__no_recon             = 335544353)
      INTEGER*4 GDS__no_record           
      PARAMETER (GDS__no_record            = 335544354)
      INTEGER*4 GDS__no_segstr_close     
      PARAMETER (GDS__no_segstr_close      = 335544355)
      INTEGER*4 GDS__obsolete_metadata   
      PARAMETER (GDS__obsolete_metadata    = 335544356)
      INTEGER*4 GDS__open_trans          
      PARAMETER (GDS__open_trans           = 335544357)
      INTEGER*4 GDS__port_len            
      PARAMETER (GDS__port_len             = 335544358)
      INTEGER*4 GDS__read_only_field     
      PARAMETER (GDS__read_only_field      = 335544359)
      INTEGER*4 GDS__read_only_rel       
      PARAMETER (GDS__read_only_rel        = 335544360)
      INTEGER*4 GDS__read_only_trans     
      PARAMETER (GDS__read_only_trans      = 335544361)
      INTEGER*4 GDS__read_only_view      
      PARAMETER (GDS__read_only_view       = 335544362)
      INTEGER*4 GDS__req_no_trans        
      PARAMETER (GDS__req_no_trans         = 335544363)
      INTEGER*4 GDS__req_sync            
      PARAMETER (GDS__req_sync             = 335544364)
      INTEGER*4 GDS__req_wrong_db        
      PARAMETER (GDS__req_wrong_db         = 335544365)
      INTEGER*4 GDS__segment             
      PARAMETER (GDS__segment              = 335544366)
      INTEGER*4 GDS__segstr_eof          
      PARAMETER (GDS__segstr_eof           = 335544367)
      INTEGER*4 GDS__segstr_no_op        
      PARAMETER (GDS__segstr_no_op         = 335544368)
      INTEGER*4 GDS__segstr_no_read      
      PARAMETER (GDS__segstr_no_read       = 335544369)
      INTEGER*4 GDS__segstr_no_trans     
      PARAMETER (GDS__segstr_no_trans      = 335544370)
      INTEGER*4 GDS__segstr_no_write     
      PARAMETER (GDS__segstr_no_write      = 335544371)
      INTEGER*4 GDS__segstr_wrong_db     
      PARAMETER (GDS__segstr_wrong_db      = 335544372)
      INTEGER*4 GDS__sys_request         
      PARAMETER (GDS__sys_request          = 335544373)
      INTEGER*4 GDS__stream_eof          
      PARAMETER (GDS__stream_eof           = 335544374)
      INTEGER*4 GDS__unavailable         
      PARAMETER (GDS__unavailable          = 335544375)
      INTEGER*4 GDS__unres_rel           
      PARAMETER (GDS__unres_rel            = 335544376)
      INTEGER*4 GDS__uns_ext             
      PARAMETER (GDS__uns_ext              = 335544377)
      INTEGER*4 GDS__wish_list           
      PARAMETER (GDS__wish_list            = 335544378)
      INTEGER*4 GDS__wrong_ods           
      PARAMETER (GDS__wrong_ods            = 335544379)
      INTEGER*4 GDS__wronumarg           
      PARAMETER (GDS__wronumarg            = 335544380)
      INTEGER*4 GDS__imp_exc             
      PARAMETER (GDS__imp_exc              = 335544381)
      INTEGER*4 GDS__random              
      PARAMETER (GDS__random               = 335544382)
      INTEGER*4 GDS__fatal_conflict      
      PARAMETER (GDS__fatal_conflict       = 335544383)
      INTEGER*4 GDS__badblk              
      PARAMETER (GDS__badblk               = 335544384)
      INTEGER*4 GDS__invpoolcl           
      PARAMETER (GDS__invpoolcl            = 335544385)
      INTEGER*4 GDS__nopoolids           
      PARAMETER (GDS__nopoolids            = 335544386)
      INTEGER*4 GDS__relbadblk           
      PARAMETER (GDS__relbadblk            = 335544387)
      INTEGER*4 GDS__blktoobig           
      PARAMETER (GDS__blktoobig            = 335544388)
      INTEGER*4 GDS__bufexh              
      PARAMETER (GDS__bufexh               = 335544389)
      INTEGER*4 GDS__syntaxerr           
      PARAMETER (GDS__syntaxerr            = 335544390)
      INTEGER*4 GDS__bufinuse            
      PARAMETER (GDS__bufinuse             = 335544391)
      INTEGER*4 GDS__bdbincon            
      PARAMETER (GDS__bdbincon             = 335544392)
      INTEGER*4 GDS__reqinuse            
      PARAMETER (GDS__reqinuse             = 335544393)
      INTEGER*4 GDS__badodsver           
      PARAMETER (GDS__badodsver            = 335544394)
      INTEGER*4 GDS__relnotdef           
      PARAMETER (GDS__relnotdef            = 335544395)
      INTEGER*4 GDS__fldnotdef           
      PARAMETER (GDS__fldnotdef            = 335544396)
      INTEGER*4 GDS__dirtypage           
      PARAMETER (GDS__dirtypage            = 335544397)
      INTEGER*4 GDS__waifortra           
      PARAMETER (GDS__waifortra            = 335544398)
      INTEGER*4 GDS__doubleloc           
      PARAMETER (GDS__doubleloc            = 335544399)
      INTEGER*4 GDS__nodnotfnd           
      PARAMETER (GDS__nodnotfnd            = 335544400)
      INTEGER*4 GDS__dupnodfnd           
      PARAMETER (GDS__dupnodfnd            = 335544401)
      INTEGER*4 GDS__locnotmar           
      PARAMETER (GDS__locnotmar            = 335544402)
      INTEGER*4 GDS__badpagtyp           
      PARAMETER (GDS__badpagtyp            = 335544403)
      INTEGER*4 GDS__corrupt             
      PARAMETER (GDS__corrupt              = 335544404)
      INTEGER*4 GDS__badpage             
      PARAMETER (GDS__badpage              = 335544405)
      INTEGER*4 GDS__badindex            
      PARAMETER (GDS__badindex             = 335544406)
      INTEGER*4 GDS__dbbnotzer           
      PARAMETER (GDS__dbbnotzer            = 335544407)
      INTEGER*4 GDS__tranotzer           
      PARAMETER (GDS__tranotzer            = 335544408)
      INTEGER*4 GDS__trareqmis           
      PARAMETER (GDS__trareqmis            = 335544409)
      INTEGER*4 GDS__badhndcnt           
      PARAMETER (GDS__badhndcnt            = 335544410)
      INTEGER*4 GDS__wrotpbver           
      PARAMETER (GDS__wrotpbver            = 335544411)
      INTEGER*4 GDS__wroblrver           
      PARAMETER (GDS__wroblrver            = 335544412)
      INTEGER*4 GDS__wrodpbver           
      PARAMETER (GDS__wrodpbver            = 335544413)
      INTEGER*4 GDS__blobnotsup          
      PARAMETER (GDS__blobnotsup           = 335544414)
      INTEGER*4 GDS__badrelation         
      PARAMETER (GDS__badrelation          = 335544415)
      INTEGER*4 GDS__nodetach            
      PARAMETER (GDS__nodetach             = 335544416)
      INTEGER*4 GDS__notremote           
      PARAMETER (GDS__notremote            = 335544417)
      INTEGER*4 GDS__trainlim            
      PARAMETER (GDS__trainlim             = 335544418)
      INTEGER*4 GDS__notinlim            
      PARAMETER (GDS__notinlim             = 335544419)
      INTEGER*4 GDS__traoutsta           
      PARAMETER (GDS__traoutsta            = 335544420)
      INTEGER*4 GDS__connect_reject      
      PARAMETER (GDS__connect_reject       = 335544421)
      INTEGER*4 GDS__dbfile              
      PARAMETER (GDS__dbfile               = 335544422)
      INTEGER*4 GDS__orphan              
      PARAMETER (GDS__orphan               = 335544423)
      INTEGER*4 GDS__no_lock_mgr         
      PARAMETER (GDS__no_lock_mgr          = 335544424)
      INTEGER*4 GDS__ctxinuse            
      PARAMETER (GDS__ctxinuse             = 335544425)
      INTEGER*4 GDS__ctxnotdef           
      PARAMETER (GDS__ctxnotdef            = 335544426)
      INTEGER*4 GDS__datnotsup           
      PARAMETER (GDS__datnotsup            = 335544427)
      INTEGER*4 GDS__badmsgnum           
      PARAMETER (GDS__badmsgnum            = 335544428)
      INTEGER*4 GDS__badparnum           
      PARAMETER (GDS__badparnum            = 335544429)
      INTEGER*4 GDS__virmemexh           
      PARAMETER (GDS__virmemexh            = 335544430)
      INTEGER*4 GDS__blocking_signal     
      PARAMETER (GDS__blocking_signal      = 335544431)
      INTEGER*4 GDS__lockmanerr          
      PARAMETER (GDS__lockmanerr           = 335544432)
      INTEGER*4 GDS__journerr            
      PARAMETER (GDS__journerr             = 335544433)
      INTEGER*4 GDS__keytoobig           
      PARAMETER (GDS__keytoobig            = 335544434)
      INTEGER*4 GDS__nullsegkey          
      PARAMETER (GDS__nullsegkey           = 335544435)
      INTEGER*4 GDS__sqlerr              
      PARAMETER (GDS__sqlerr               = 335544436)
      INTEGER*4 GDS__wrodynver           
      PARAMETER (GDS__wrodynver            = 335544437)
      INTEGER*4 GDS__funnotdef           
      PARAMETER (GDS__funnotdef            = 335544438)
      INTEGER*4 GDS__funmismat           
      PARAMETER (GDS__funmismat            = 335544439)
      INTEGER*4 GDS__bad_msg_vec         
      PARAMETER (GDS__bad_msg_vec          = 335544440)
      INTEGER*4 GDS__bad_detach          
      PARAMETER (GDS__bad_detach           = 335544441)
      INTEGER*4 GDS__noargacc_read       
      PARAMETER (GDS__noargacc_read        = 335544442)
      INTEGER*4 GDS__noargacc_write      
      PARAMETER (GDS__noargacc_write       = 335544443)
      INTEGER*4 GDS__read_only           
      PARAMETER (GDS__read_only            = 335544444)
      INTEGER*4 GDS__ext_err             
      PARAMETER (GDS__ext_err              = 335544445)
      INTEGER*4 GDS__non_updatable       
      PARAMETER (GDS__non_updatable        = 335544446)
      INTEGER*4 GDS__no_rollback         
      PARAMETER (GDS__no_rollback          = 335544447)
      INTEGER*4 GDS__bad_sec_info        
      PARAMETER (GDS__bad_sec_info         = 335544448)
      INTEGER*4 GDS__invalid_sec_info    
      PARAMETER (GDS__invalid_sec_info     = 335544449)
      INTEGER*4 GDS__misc_interpreted    
      PARAMETER (GDS__misc_interpreted     = 335544450)
      INTEGER*4 GDS__update_conflict     
      PARAMETER (GDS__update_conflict      = 335544451)
      INTEGER*4 GDS__unlicensed          
      PARAMETER (GDS__unlicensed           = 335544452)
      INTEGER*4 GDS__obj_in_use          
      PARAMETER (GDS__obj_in_use           = 335544453)
      INTEGER*4 GDS__nofilter            
      PARAMETER (GDS__nofilter             = 335544454)
      INTEGER*4 GDS__shadow_accessed     
      PARAMETER (GDS__shadow_accessed      = 335544455)
      INTEGER*4 GDS__invalid_sdl         
      PARAMETER (GDS__invalid_sdl          = 335544456)
      INTEGER*4 GDS__out_of_bounds       
      PARAMETER (GDS__out_of_bounds        = 335544457)
      INTEGER*4 GDS__invalid_dimension   
      PARAMETER (GDS__invalid_dimension    = 335544458)
      INTEGER*4 GDS__rec_in_limbo        
      PARAMETER (GDS__rec_in_limbo         = 335544459)
      INTEGER*4 GDS__shadow_missing      
      PARAMETER (GDS__shadow_missing       = 335544460)
      INTEGER*4 GDS__cant_validate       
      PARAMETER (GDS__cant_validate        = 335544461)
      INTEGER*4 GDS__cant_start_journal  
      PARAMETER (GDS__cant_start_journal   = 335544462)
      INTEGER*4 GDS__gennotdef           
      PARAMETER (GDS__gennotdef            = 335544463)
      INTEGER*4 GDS__cant_start_logging  
      PARAMETER (GDS__cant_start_logging   = 335544464)
      INTEGER*4 GDS__bad_segstr_type     
      PARAMETER (GDS__bad_segstr_type      = 335544465)
      INTEGER*4 GDS__foreign_key         
      PARAMETER (GDS__foreign_key          = 335544466)
      INTEGER*4 GDS__high_minor          
      PARAMETER (GDS__high_minor           = 335544467)
      INTEGER*4 GDS__tra_state           
      PARAMETER (GDS__tra_state            = 335544468)
      INTEGER*4 GDS__trans_invalid       
      PARAMETER (GDS__trans_invalid        = 335544469)
      INTEGER*4 GDS__buf_invalid         
      PARAMETER (GDS__buf_invalid          = 335544470)
      INTEGER*4 GDS__indexnotdefined     
      PARAMETER (GDS__indexnotdefined      = 335544471)
      INTEGER*4 GDS__login               
      PARAMETER (GDS__login                = 335544472)
      INTEGER*4 GDS__invalid_bookmark    
      PARAMETER (GDS__invalid_bookmark     = 335544473)
      INTEGER*4 GDS__bad_lock_level      
      PARAMETER (GDS__bad_lock_level       = 335544474)
      INTEGER*4 GDS__relation_lock       
      PARAMETER (GDS__relation_lock        = 335544475)
      INTEGER*4 GDS__record_lock         
      PARAMETER (GDS__record_lock          = 335544476)
      INTEGER*4 GDS__max_idx             
      PARAMETER (GDS__max_idx              = 335544477)
      INTEGER*4 GDS__jrn_enable          
      PARAMETER (GDS__jrn_enable           = 335544478)
      INTEGER*4 GDS__old_failure         
      PARAMETER (GDS__old_failure          = 335544479)
      INTEGER*4 GDS__old_in_progress     
      PARAMETER (GDS__old_in_progress      = 335544480)
      INTEGER*4 GDS__old_no_space        
      PARAMETER (GDS__old_no_space         = 335544481)
      INTEGER*4 GDS__no_wal_no_jrn       
      PARAMETER (GDS__no_wal_no_jrn        = 335544482)
      INTEGER*4 GDS__num_old_files       
      PARAMETER (GDS__num_old_files        = 335544483)
      INTEGER*4 GDS__wal_file_open       
      PARAMETER (GDS__wal_file_open        = 335544484)
      INTEGER*4 GDS__bad_stmt_handle     
      PARAMETER (GDS__bad_stmt_handle      = 335544485)
      INTEGER*4 GDS__wal_failure         
      PARAMETER (GDS__wal_failure          = 335544486)
      INTEGER*4 GDS__walw_err            
      PARAMETER (GDS__walw_err             = 335544487)
      INTEGER*4 GDS__logh_small          
      PARAMETER (GDS__logh_small           = 335544488)
      INTEGER*4 GDS__logh_inv_version    
      PARAMETER (GDS__logh_inv_version     = 335544489)
      INTEGER*4 GDS__logh_open_flag      
      PARAMETER (GDS__logh_open_flag       = 335544490)
      INTEGER*4 GDS__logh_open_flag2     
      PARAMETER (GDS__logh_open_flag2      = 335544491)
      INTEGER*4 GDS__logh_diff_dbname    
      PARAMETER (GDS__logh_diff_dbname     = 335544492)
      INTEGER*4 GDS__logf_unexpected_eof 
      PARAMETER (GDS__logf_unexpected_eof  = 335544493)
      INTEGER*4 GDS__logr_incomplete     
      PARAMETER (GDS__logr_incomplete      = 335544494)
      INTEGER*4 GDS__logr_header_small   
      PARAMETER (GDS__logr_header_small    = 335544495)
      INTEGER*4 GDS__logb_small          
      PARAMETER (GDS__logb_small           = 335544496)
      INTEGER*4 GDS__wal_illegal_attach  
      PARAMETER (GDS__wal_illegal_attach   = 335544497)
      INTEGER*4 GDS__wal_invalid_wpb     
      PARAMETER (GDS__wal_invalid_wpb      = 335544498)
      INTEGER*4 GDS__wal_err_rollover    
      PARAMETER (GDS__wal_err_rollover     = 335544499)
      INTEGER*4 GDS__no_wal              
      PARAMETER (GDS__no_wal               = 335544500)
      INTEGER*4 GDS__drop_wal            
      PARAMETER (GDS__drop_wal             = 335544501)
      INTEGER*4 GDS__stream_not_defined  
      PARAMETER (GDS__stream_not_defined   = 335544502)
      INTEGER*4 GDS__wal_subsys_error    
      PARAMETER (GDS__wal_subsys_error     = 335544503)
      INTEGER*4 GDS__wal_subsys_corrupt  
      PARAMETER (GDS__wal_subsys_corrupt   = 335544504)
      INTEGER*4 GDS__no_archive          
      PARAMETER (GDS__no_archive           = 335544505)
      INTEGER*4 GDS__shutinprog          
      PARAMETER (GDS__shutinprog           = 335544506)
      INTEGER*4 GDS__range_in_use        
      PARAMETER (GDS__range_in_use         = 335544507)
      INTEGER*4 GDS__range_not_found     
      PARAMETER (GDS__range_not_found      = 335544508)
      INTEGER*4 GDS__charset_not_found   
      PARAMETER (GDS__charset_not_found    = 335544509)
      INTEGER*4 GDS__lock_timeout        
      PARAMETER (GDS__lock_timeout         = 335544510)
      INTEGER*4 GDS__prcnotdef           
      PARAMETER (GDS__prcnotdef            = 335544511)
      INTEGER*4 GDS__prcmismat           
      PARAMETER (GDS__prcmismat            = 335544512)
      INTEGER*4 GDS__wal_bugcheck        
      PARAMETER (GDS__wal_bugcheck         = 335544513)
      INTEGER*4 GDS__wal_cant_expand     
      PARAMETER (GDS__wal_cant_expand      = 335544514)
      INTEGER*4 GDS__codnotdef           
      PARAMETER (GDS__codnotdef            = 335544515)
      INTEGER*4 GDS__xcpnotdef           
      PARAMETER (GDS__xcpnotdef            = 335544516)
      INTEGER*4 GDS__except              
      PARAMETER (GDS__except               = 335544517)
      INTEGER*4 GDS__cache_restart       
      PARAMETER (GDS__cache_restart        = 335544518)
      INTEGER*4 GDS__bad_lock_handle     
      PARAMETER (GDS__bad_lock_handle      = 335544519)
      INTEGER*4 GDS__jrn_present         
      PARAMETER (GDS__jrn_present          = 335544520)
      INTEGER*4 GDS__wal_err_rollover2   
      PARAMETER (GDS__wal_err_rollover2    = 335544521)
      INTEGER*4 GDS__wal_err_logwrite    
      PARAMETER (GDS__wal_err_logwrite     = 335544522)
      INTEGER*4 GDS__wal_err_jrn_comm    
      PARAMETER (GDS__wal_err_jrn_comm     = 335544523)
      INTEGER*4 GDS__wal_err_expansion   
      PARAMETER (GDS__wal_err_expansion    = 335544524)
      INTEGER*4 GDS__wal_err_setup       
      PARAMETER (GDS__wal_err_setup        = 335544525)
      INTEGER*4 GDS__wal_err_ww_sync     
      PARAMETER (GDS__wal_err_ww_sync      = 335544526)
      INTEGER*4 GDS__wal_err_ww_start    
      PARAMETER (GDS__wal_err_ww_start     = 335544527)
      INTEGER*4 GDS__shutdown            
      PARAMETER (GDS__shutdown             = 335544528)
      INTEGER*4 GDS__existing_priv_mod   
      PARAMETER (GDS__existing_priv_mod    = 335544529)
      INTEGER*4 GDS__primary_key_ref     
      PARAMETER (GDS__primary_key_ref      = 335544530)
      INTEGER*4 GDS__primary_key_notnull 
      PARAMETER (GDS__primary_key_notnull  = 335544531)
      INTEGER*4 GDS__ref_cnstrnt_notfound
      PARAMETER (GDS__ref_cnstrnt_notfound = 335544532)
      INTEGER*4 GDS__foreign_key_notfound
      PARAMETER (GDS__foreign_key_notfound = 335544533)
      INTEGER*4 GDS__ref_cnstrnt_update  
      PARAMETER (GDS__ref_cnstrnt_update   = 335544534)
      INTEGER*4 GDS__check_cnstrnt_update
      PARAMETER (GDS__check_cnstrnt_update = 335544535)
      INTEGER*4 GDS__check_cnstrnt_del   
      PARAMETER (GDS__check_cnstrnt_del    = 335544536)
      INTEGER*4 GDS__integ_index_seg_del 
      PARAMETER (GDS__integ_index_seg_del  = 335544537)
      INTEGER*4 GDS__integ_index_seg_mod 
      PARAMETER (GDS__integ_index_seg_mod  = 335544538)
      INTEGER*4 GDS__integ_index_del     
      PARAMETER (GDS__integ_index_del      = 335544539)
      INTEGER*4 GDS__integ_index_mod     
      PARAMETER (GDS__integ_index_mod      = 335544540)
      INTEGER*4 GDS__check_trig_del      
      PARAMETER (GDS__check_trig_del       = 335544541)
      INTEGER*4 GDS__check_trig_update   
      PARAMETER (GDS__check_trig_update    = 335544542)
      INTEGER*4 GDS__cnstrnt_fld_del     
      PARAMETER (GDS__cnstrnt_fld_del      = 335544543)
      INTEGER*4 GDS__cnstrnt_fld_rename  
      PARAMETER (GDS__cnstrnt_fld_rename   = 335544544)
      INTEGER*4 GDS__rel_cnstrnt_update  
      PARAMETER (GDS__rel_cnstrnt_update   = 335544545)
      INTEGER*4 GDS__constaint_on_view   
      PARAMETER (GDS__constaint_on_view    = 335544546)
      INTEGER*4 GDS__invld_cnstrnt_type  
      PARAMETER (GDS__invld_cnstrnt_type   = 335544547)
      INTEGER*4 GDS__primary_key_exists  
      PARAMETER (GDS__primary_key_exists   = 335544548)
      INTEGER*4 GDS__systrig_update      
      PARAMETER (GDS__systrig_update       = 335544549)
      INTEGER*4 GDS__not_rel_owner       
      PARAMETER (GDS__not_rel_owner        = 335544550)
      INTEGER*4 GDS__grant_obj_notfound  
      PARAMETER (GDS__grant_obj_notfound   = 335544551)
      INTEGER*4 GDS__grant_fld_notfound  
      PARAMETER (GDS__grant_fld_notfound   = 335544552)
      INTEGER*4 GDS__grant_nopriv        
      PARAMETER (GDS__grant_nopriv         = 335544553)
      INTEGER*4 GDS__nonsql_security_rel 
      PARAMETER (GDS__nonsql_security_rel  = 335544554)
      INTEGER*4 GDS__nonsql_security_fld 
      PARAMETER (GDS__nonsql_security_fld  = 335544555)
      INTEGER*4 GDS__wal_cache_err       
      PARAMETER (GDS__wal_cache_err        = 335544556)
      INTEGER*4 GDS__shutfail            
      PARAMETER (GDS__shutfail             = 335544557)
      INTEGER*4 GDS__check_constraint    
      PARAMETER (GDS__check_constraint     = 335544558)
      INTEGER*4 GDS__bad_svc_handle      
      PARAMETER (GDS__bad_svc_handle       = 335544559)
      INTEGER*4 GDS__shutwarn            
      PARAMETER (GDS__shutwarn             = 335544560)
      INTEGER*4 GDS__wrospbver           
      PARAMETER (GDS__wrospbver            = 335544561)
      INTEGER*4 GDS__bad_spb_form        
      PARAMETER (GDS__bad_spb_form         = 335544562)
      INTEGER*4 GDS__svcnotdef           
      PARAMETER (GDS__svcnotdef            = 335544563)
      INTEGER*4 GDS__no_jrn              
      PARAMETER (GDS__no_jrn               = 335544564)
      INTEGER*4 GDS__transliteration_fail
      PARAMETER (GDS__transliteration_fail = 335544565)
      INTEGER*4 GDS__start_cm_for_wal    
      PARAMETER (GDS__start_cm_for_wal     = 335544566)
      INTEGER*4 GDS__wal_ovflow_log_requi
      PARAMETER (GDS__wal_ovflow_log_requi = 335544567)
      INTEGER*4 GDS__text_subtype        
      PARAMETER (GDS__text_subtype         = 335544568)
      INTEGER*4 GDS__dsql_error          
      PARAMETER (GDS__dsql_error           = 335544569)
      INTEGER*4 GDS__dsql_command_err    
      PARAMETER (GDS__dsql_command_err     = 335544570)
      INTEGER*4 GDS__dsql_constant_err   
      PARAMETER (GDS__dsql_constant_err    = 335544571)
      INTEGER*4 GDS__dsql_cursor_err     
      PARAMETER (GDS__dsql_cursor_err      = 335544572)
      INTEGER*4 GDS__dsql_datatype_err   
      PARAMETER (GDS__dsql_datatype_err    = 335544573)
      INTEGER*4 GDS__dsql_decl_err       
      PARAMETER (GDS__dsql_decl_err        = 335544574)
      INTEGER*4 GDS__dsql_cursor_update_e
      PARAMETER (GDS__dsql_cursor_update_e = 335544575)
      INTEGER*4 GDS__dsql_cursor_open_err
      PARAMETER (GDS__dsql_cursor_open_err = 335544576)
      INTEGER*4 GDS__dsql_cursor_close_er
      PARAMETER (GDS__dsql_cursor_close_er = 335544577)
      INTEGER*4 GDS__dsql_field_err      
      PARAMETER (GDS__dsql_field_err       = 335544578)
      INTEGER*4 GDS__dsql_internal_err   
      PARAMETER (GDS__dsql_internal_err    = 335544579)
      INTEGER*4 GDS__dsql_relation_err   
      PARAMETER (GDS__dsql_relation_err    = 335544580)
      INTEGER*4 GDS__dsql_procedure_err  
      PARAMETER (GDS__dsql_procedure_err   = 335544581)
      INTEGER*4 GDS__dsql_request_err    
      PARAMETER (GDS__dsql_request_err     = 335544582)
      INTEGER*4 GDS__dsql_sqlda_err      
      PARAMETER (GDS__dsql_sqlda_err       = 335544583)
      INTEGER*4 GDS__dsql_var_count_err  
      PARAMETER (GDS__dsql_var_count_err   = 335544584)
      INTEGER*4 GDS__dsql_stmt_handle    
      PARAMETER (GDS__dsql_stmt_handle     = 335544585)
      INTEGER*4 GDS__dsql_function_err   
      PARAMETER (GDS__dsql_function_err    = 335544586)
      INTEGER*4 GDS__dsql_blob_err       
      PARAMETER (GDS__dsql_blob_err        = 335544587)
      INTEGER*4 GDS__collation_not_found 
      PARAMETER (GDS__collation_not_found  = 335544588)
      INTEGER*4 GDS__collation_not_for_ch
      PARAMETER (GDS__collation_not_for_ch = 335544589)
      INTEGER*4 GDS__dsql_dup_option     
      PARAMETER (GDS__dsql_dup_option      = 335544590)
      INTEGER*4 GDS__dsql_tran_err       
      PARAMETER (GDS__dsql_tran_err        = 335544591)
      INTEGER*4 GDS__dsql_invalid_array  
      PARAMETER (GDS__dsql_invalid_array   = 335544592)
      INTEGER*4 GDS__dsql_max_arr_dim_exc
      PARAMETER (GDS__dsql_max_arr_dim_exc = 335544593)
      INTEGER*4 GDS__dsql_arr_range_error
      PARAMETER (GDS__dsql_arr_range_error = 335544594)
      INTEGER*4 GDS__dsql_trigger_err    
      PARAMETER (GDS__dsql_trigger_err     = 335544595)
      INTEGER*4 GDS__dsql_subselect_err  
      PARAMETER (GDS__dsql_subselect_err   = 335544596)
      INTEGER*4 GDS__dsql_crdb_prepare_er
      PARAMETER (GDS__dsql_crdb_prepare_er = 335544597)
      INTEGER*4 GDS__specify_field_err   
      PARAMETER (GDS__specify_field_err    = 335544598)
      INTEGER*4 GDS__num_field_err       
      PARAMETER (GDS__num_field_err        = 335544599)
      INTEGER*4 GDS__col_name_err        
      PARAMETER (GDS__col_name_err         = 335544600)
      INTEGER*4 GDS__where_err           
      PARAMETER (GDS__where_err            = 335544601)
      INTEGER*4 GDS__table_view_err      
      PARAMETER (GDS__table_view_err       = 335544602)
      INTEGER*4 GDS__distinct_err        
      PARAMETER (GDS__distinct_err         = 335544603)
      INTEGER*4 GDS__key_field_count_err 
      PARAMETER (GDS__key_field_count_err  = 335544604)
      INTEGER*4 GDS__subquery_err        
      PARAMETER (GDS__subquery_err         = 335544605)
      INTEGER*4 GDS__expression_eval_err 
      PARAMETER (GDS__expression_eval_err  = 335544606)
      INTEGER*4 GDS__node_err            
      PARAMETER (GDS__node_err             = 335544607)
      INTEGER*4 GDS__command_end_err     
      PARAMETER (GDS__command_end_err      = 335544608)
      INTEGER*4 GDS__index_name          
      PARAMETER (GDS__index_name           = 335544609)
      INTEGER*4 GDS__exception_name      
      PARAMETER (GDS__exception_name       = 335544610)
      INTEGER*4 GDS__field_name          
      PARAMETER (GDS__field_name           = 335544611)
      INTEGER*4 GDS__token_err           
      PARAMETER (GDS__token_err            = 335544612)
      INTEGER*4 GDS__union_err           
      PARAMETER (GDS__union_err            = 335544613)
      INTEGER*4 GDS__dsql_construct_err  
      PARAMETER (GDS__dsql_construct_err   = 335544614)
      INTEGER*4 GDS__field_aggregate_err 
      PARAMETER (GDS__field_aggregate_err  = 335544615)
      INTEGER*4 GDS__field_ref_err       
      PARAMETER (GDS__field_ref_err        = 335544616)
      INTEGER*4 GDS__order_by_err        
      PARAMETER (GDS__order_by_err         = 335544617)
      INTEGER*4 GDS__return_mode_err     
      PARAMETER (GDS__return_mode_err      = 335544618)
      INTEGER*4 GDS__extern_func_err     
      PARAMETER (GDS__extern_func_err      = 335544619)
      INTEGER*4 GDS__alias_conflict_err  
      PARAMETER (GDS__alias_conflict_err   = 335544620)
      INTEGER*4 GDS__procedure_conflict_e
      PARAMETER (GDS__procedure_conflict_e = 335544621)
      INTEGER*4 GDS__relation_conflict_er
      PARAMETER (GDS__relation_conflict_er = 335544622)
      INTEGER*4 GDS__dsql_domain_err     
      PARAMETER (GDS__dsql_domain_err      = 335544623)
      INTEGER*4 GDS__idx_seg_err         
      PARAMETER (GDS__idx_seg_err          = 335544624)
      INTEGER*4 GDS__node_name_err       
      PARAMETER (GDS__node_name_err        = 335544625)
      INTEGER*4 GDS__table_name          
      PARAMETER (GDS__table_name           = 335544626)
      INTEGER*4 GDS__proc_name           
      PARAMETER (GDS__proc_name            = 335544627)
      INTEGER*4 GDS__idx_create_err      
      PARAMETER (GDS__idx_create_err       = 335544628)
      INTEGER*4 GDS__wal_shadow_err      
      PARAMETER (GDS__wal_shadow_err       = 335544629)
      INTEGER*4 GDS__dependency          
      PARAMETER (GDS__dependency           = 335544630)
      INTEGER*4 GDS__idx_key_err         
      PARAMETER (GDS__idx_key_err          = 335544631)
      INTEGER*4 GDS__dsql_file_length_err
      PARAMETER (GDS__dsql_file_length_err = 335544632)
      INTEGER*4 GDS__dsql_shadow_number_e
      PARAMETER (GDS__dsql_shadow_number_e = 335544633)
      INTEGER*4 GDS__dsql_token_unk_err  
      PARAMETER (GDS__dsql_token_unk_err   = 335544634)
      INTEGER*4 GDS__dsql_no_relation_ali
      PARAMETER (GDS__dsql_no_relation_ali = 335544635)
      INTEGER*4 GDS__indexname           
      PARAMETER (GDS__indexname            = 335544636)
      INTEGER*4 GDS__no_stream_plan      
      PARAMETER (GDS__no_stream_plan       = 335544637)
      INTEGER*4 GDS__stream_twice        
      PARAMETER (GDS__stream_twice         = 335544638)
      INTEGER*4 GDS__stream_not_found    
      PARAMETER (GDS__stream_not_found     = 335544639)
      INTEGER*4 GDS__collation_requires_t
      PARAMETER (GDS__collation_requires_t = 335544640)
      INTEGER*4 GDS__dsql_domain_not_foun
      PARAMETER (GDS__dsql_domain_not_foun = 335544641)
      INTEGER*4 GDS__index_unused        
      PARAMETER (GDS__index_unused         = 335544642)
      INTEGER*4 GDS__dsql_self_join      
      PARAMETER (GDS__dsql_self_join       = 335544643)
      INTEGER*4 GDS__stream_bof          
      PARAMETER (GDS__stream_bof           = 335544644)
      INTEGER*4 GDS__stream_crack        
      PARAMETER (GDS__stream_crack         = 335544645)
      INTEGER*4 GDS__db_or_file_exists   
      PARAMETER (GDS__db_or_file_exists    = 335544646)
      INTEGER*4 GDS__invalid_operator    
      PARAMETER (GDS__invalid_operator     = 335544647)
      INTEGER*4 GDS__conn_lost           
      PARAMETER (GDS__conn_lost            = 335544648)
      INTEGER*4 GDS__bad_checksum        
      PARAMETER (GDS__bad_checksum         = 335544649)
      INTEGER*4 GDS__page_type_err       
      PARAMETER (GDS__page_type_err        = 335544650)
      INTEGER*4 GDS__ext_readonly_err    
      PARAMETER (GDS__ext_readonly_err     = 335544651)
      INTEGER*4 GDS__sing_select_err     
      PARAMETER (GDS__sing_select_err      = 335544652)
      INTEGER*4 GDS__psw_attach          
      PARAMETER (GDS__psw_attach           = 335544653)
      INTEGER*4 GDS__psw_start_trans     
      PARAMETER (GDS__psw_start_trans      = 335544654)
      INTEGER*4 GDS__invalid_direction   
      PARAMETER (GDS__invalid_direction    = 335544655)
      INTEGER*4 GDS__dsql_var_conflict   
      PARAMETER (GDS__dsql_var_conflict    = 335544656)
      INTEGER*4 GDS__dsql_no_blob_array  
      PARAMETER (GDS__dsql_no_blob_array   = 335544657)
      INTEGER*4 GDS__dsql_base_table     
      PARAMETER (GDS__dsql_base_table      = 335544658)
      INTEGER*4 GDS__duplicate_base_table
      PARAMETER (GDS__duplicate_base_table = 335544659)
      INTEGER*4 GDS__view_alias          
      PARAMETER (GDS__view_alias           = 335544660)
      INTEGER*4 GDS__index_root_page_full
      PARAMETER (GDS__index_root_page_full = 335544661)
      INTEGER*4 GDS__dsql_blob_type_unkno
      PARAMETER (GDS__dsql_blob_type_unkno = 335544662)
      INTEGER*4 GDS__req_max_clones_excee
      PARAMETER (GDS__req_max_clones_excee = 335544663)
      INTEGER*4 GDS__dsql_duplicate_spec 
      PARAMETER (GDS__dsql_duplicate_spec  = 335544664)
      INTEGER*4 GDS__unique_key_violation
      PARAMETER (GDS__unique_key_violation = 335544665)
      INTEGER*4 GDS__srvr_version_too_old
      PARAMETER (GDS__srvr_version_too_old = 335544666)
      INTEGER*4 GDS__drdb_completed_with_
      PARAMETER (GDS__drdb_completed_with_ = 335544667)
      INTEGER*4 GDS__dsql_procedure_use_e
      PARAMETER (GDS__dsql_procedure_use_e = 335544668)
      INTEGER*4 GDS__dsql_count_mismatch 
      PARAMETER (GDS__dsql_count_mismatch  = 335544669)
      INTEGER*4 GDS__blob_idx_err        
      PARAMETER (GDS__blob_idx_err         = 335544670)
      INTEGER*4 GDS__array_idx_err       
      PARAMETER (GDS__array_idx_err        = 335544671)
      INTEGER*4 GDS__key_field_err       
      PARAMETER (GDS__key_field_err        = 335544672)
      INTEGER*4 GDS__no_delete           
      PARAMETER (GDS__no_delete            = 335544673)
      INTEGER*4 GDS__del_last_field      
      PARAMETER (GDS__del_last_field       = 335544674)
      INTEGER*4 GDS__sort_err            
      PARAMETER (GDS__sort_err             = 335544675)
      INTEGER*4 GDS__sort_mem_err        
      PARAMETER (GDS__sort_mem_err         = 335544676)
      INTEGER*4 GDS__version_err         
      PARAMETER (GDS__version_err          = 335544677)
      INTEGER*4 GDS__inval_key_posn      
      PARAMETER (GDS__inval_key_posn       = 335544678)
      INTEGER*4 GDS__no_segments_err     
      PARAMETER (GDS__no_segments_err      = 335544679)
      INTEGER*4 GDS__crrp_data_err       
      PARAMETER (GDS__crrp_data_err        = 335544680)
      INTEGER*4 GDS__rec_size_err        
      PARAMETER (GDS__rec_size_err         = 335544681)
      INTEGER*4 GDS__dsql_field_ref      
      PARAMETER (GDS__dsql_field_ref       = 335544682)
      INTEGER*4 GDS__req_depth_exceeded  
      PARAMETER (GDS__req_depth_exceeded   = 335544683)
      INTEGER*4 GDS__no_field_access     
      PARAMETER (GDS__no_field_access      = 335544684)
      INTEGER*4 GDS__no_dbkey            
      PARAMETER (GDS__no_dbkey             = 335544685)
      INTEGER*4 GDS__jrn_format_err      
      PARAMETER (GDS__jrn_format_err       = 335544686)
      INTEGER*4 GDS__jrn_file_full       
      PARAMETER (GDS__jrn_file_full        = 335544687)
      INTEGER*4 GDS__dsql_open_cursor_req
      PARAMETER (GDS__dsql_open_cursor_req = 335544688)
      INTEGER*4 GDS__ib_error            
      PARAMETER (GDS__ib_error             = 335544689)
      INTEGER*4 GDS__cache_redef         
      PARAMETER (GDS__cache_redef          = 335544690)
      INTEGER*4 GDS__cache_too_small     
      PARAMETER (GDS__cache_too_small      = 335544691)
      INTEGER*4 GDS__log_redef           
      PARAMETER (GDS__log_redef            = 335544692)
      INTEGER*4 GDS__log_too_small       
      PARAMETER (GDS__log_too_small        = 335544693)
      INTEGER*4 GDS__partition_too_small 
      PARAMETER (GDS__partition_too_small  = 335544694)
      INTEGER*4 GDS__partition_not_supp  
      PARAMETER (GDS__partition_not_supp   = 335544695)
      INTEGER*4 GDS__log_length_spec     
      PARAMETER (GDS__log_length_spec      = 335544696)
      INTEGER*4 GDS__precision_err       
      PARAMETER (GDS__precision_err        = 335544697)
      INTEGER*4 GDS__scale_nogt          
      PARAMETER (GDS__scale_nogt           = 335544698)
      INTEGER*4 GDS__expec_short         
      PARAMETER (GDS__expec_short          = 335544699)
      INTEGER*4 GDS__expec_long          
      PARAMETER (GDS__expec_long           = 335544700)
      INTEGER*4 GDS__expec_ushort        
      PARAMETER (GDS__expec_ushort         = 335544701)
      INTEGER*4 GDS__like_escape_invalid 
      PARAMETER (GDS__like_escape_invalid  = 335544702)
      INTEGER*4 GDS__svcnoexe            
      PARAMETER (GDS__svcnoexe             = 335544703)
      INTEGER*4 GDS__net_lookup_err      
      PARAMETER (GDS__net_lookup_err       = 335544704)
      INTEGER*4 GDS__service_unknown     
      PARAMETER (GDS__service_unknown      = 335544705)
      INTEGER*4 GDS__host_unknown        
      PARAMETER (GDS__host_unknown         = 335544706)
      INTEGER*4 GDS__grant_nopriv_on_base
      PARAMETER (GDS__grant_nopriv_on_base = 335544707)
      INTEGER*4 GDS__dyn_fld_ambiguous   
      PARAMETER (GDS__dyn_fld_ambiguous    = 335544708)
      INTEGER*4 GDS__dsql_agg_ref_err    
      PARAMETER (GDS__dsql_agg_ref_err     = 335544709)
      INTEGER*4 GDS__complex_view        
      PARAMETER (GDS__complex_view         = 335544710)
      INTEGER*4 GDS__unprepared_stmt     
      PARAMETER (GDS__unprepared_stmt      = 335544711)
      INTEGER*4 GDS__expec_positive      
      PARAMETER (GDS__expec_positive       = 335544712)
      INTEGER*4 GDS__dsql_sqlda_value_err
      PARAMETER (GDS__dsql_sqlda_value_err = 335544713)
      INTEGER*4 GDS__invalid_array_id    
      PARAMETER (GDS__invalid_array_id     = 335544714)
      INTEGER*4 GDS__extfile_uns_op      
      PARAMETER (GDS__extfile_uns_op       = 335544715)
      INTEGER*4 GDS__svc_in_use          
      PARAMETER (GDS__svc_in_use           = 335544716)
      INTEGER*4 GDS__err_stack_limit     
      PARAMETER (GDS__err_stack_limit      = 335544717)
      INTEGER*4 GDS__invalid_key         
      PARAMETER (GDS__invalid_key          = 335544718)
      INTEGER*4 GDS__net_init_error      
      PARAMETER (GDS__net_init_error       = 335544719)
      INTEGER*4 GDS__loadlib_failure     
      PARAMETER (GDS__loadlib_failure      = 335544720)
      INTEGER*4 GDS__network_error       
      PARAMETER (GDS__network_error        = 335544721)
      INTEGER*4 GDS__net_connect_err     
      PARAMETER (GDS__net_connect_err      = 335544722)
      INTEGER*4 GDS__net_connect_listen_e
      PARAMETER (GDS__net_connect_listen_e = 335544723)
      INTEGER*4 GDS__net_event_connect_er
      PARAMETER (GDS__net_event_connect_er = 335544724)
      INTEGER*4 GDS__net_event_listen_err
      PARAMETER (GDS__net_event_listen_err = 335544725)
      INTEGER*4 GDS__net_read_err        
      PARAMETER (GDS__net_read_err         = 335544726)
      INTEGER*4 GDS__net_write_err       
      PARAMETER (GDS__net_write_err        = 335544727)
      INTEGER*4 GDS__integ_index_deactiva
      PARAMETER (GDS__integ_index_deactiva = 335544728)
      INTEGER*4 GDS__integ_deactivate_pri
      PARAMETER (GDS__integ_deactivate_pri = 335544729)
      INTEGER*4 GDS__cse_not_supported   
      PARAMETER (GDS__cse_not_supported    = 335544730)


      INTEGER*4 ISC_arith_except        
      PARAMETER (ISC_arith_except         = 335544321)
      INTEGER*4 ISC_bad_dbkey           
      PARAMETER (ISC_bad_dbkey            = 335544322)
      INTEGER*4 ISC_bad_db_format       
      PARAMETER (ISC_bad_db_format        = 335544323)
      INTEGER*4 ISC_bad_db_handle       
      PARAMETER (ISC_bad_db_handle        = 335544324)
      INTEGER*4 ISC_bad_dpb_content     
      PARAMETER (ISC_bad_dpb_content      = 335544325)
      INTEGER*4 ISC_bad_dpb_form        
      PARAMETER (ISC_bad_dpb_form         = 335544326)
      INTEGER*4 ISC_bad_req_handle      
      PARAMETER (ISC_bad_req_handle       = 335544327)
      INTEGER*4 ISC_bad_segstr_handle   
      PARAMETER (ISC_bad_segstr_handle    = 335544328)
      INTEGER*4 ISC_bad_segstr_id       
      PARAMETER (ISC_bad_segstr_id        = 335544329)
      INTEGER*4 ISC_bad_tpb_content     
      PARAMETER (ISC_bad_tpb_content      = 335544330)
      INTEGER*4 ISC_bad_tpb_form        
      PARAMETER (ISC_bad_tpb_form         = 335544331)
      INTEGER*4 ISC_bad_trans_handle    
      PARAMETER (ISC_bad_trans_handle     = 335544332)
      INTEGER*4 ISC_bug_check           
      PARAMETER (ISC_bug_check            = 335544333)
      INTEGER*4 ISC_convert_error       
      PARAMETER (ISC_convert_error        = 335544334)
      INTEGER*4 ISC_db_corrupt          
      PARAMETER (ISC_db_corrupt           = 335544335)
      INTEGER*4 ISC_deadlock            
      PARAMETER (ISC_deadlock             = 335544336)
      INTEGER*4 ISC_excess_trans        
      PARAMETER (ISC_excess_trans         = 335544337)
      INTEGER*4 ISC_from_no_match       
      PARAMETER (ISC_from_no_match        = 335544338)
      INTEGER*4 ISC_infinap             
      PARAMETER (ISC_infinap              = 335544339)
      INTEGER*4 ISC_infona              
      PARAMETER (ISC_infona               = 335544340)
      INTEGER*4 ISC_infunk              
      PARAMETER (ISC_infunk               = 335544341)
      INTEGER*4 ISC_integ_fail          
      PARAMETER (ISC_integ_fail           = 335544342)
      INTEGER*4 ISC_invalid_blr         
      PARAMETER (ISC_invalid_blr          = 335544343)
      INTEGER*4 ISC_io_error            
      PARAMETER (ISC_io_error             = 335544344)
      INTEGER*4 ISC_lock_conflict       
      PARAMETER (ISC_lock_conflict        = 335544345)
      INTEGER*4 ISC_metadata_corrupt    
      PARAMETER (ISC_metadata_corrupt     = 335544346)
      INTEGER*4 ISC_not_valid           
      PARAMETER (ISC_not_valid            = 335544347)
      INTEGER*4 ISC_no_cur_rec          
      PARAMETER (ISC_no_cur_rec           = 335544348)
      INTEGER*4 ISC_no_dup              
      PARAMETER (ISC_no_dup               = 335544349)
      INTEGER*4 ISC_no_finish           
      PARAMETER (ISC_no_finish            = 335544350)
      INTEGER*4 ISC_no_meta_update      
      PARAMETER (ISC_no_meta_update       = 335544351)
      INTEGER*4 ISC_no_priv             
      PARAMETER (ISC_no_priv              = 335544352)
      INTEGER*4 ISC_no_recon            
      PARAMETER (ISC_no_recon             = 335544353)
      INTEGER*4 ISC_no_record           
      PARAMETER (ISC_no_record            = 335544354)
      INTEGER*4 ISC_no_segstr_close     
      PARAMETER (ISC_no_segstr_close      = 335544355)
      INTEGER*4 ISC_obsolete_metadata   
      PARAMETER (ISC_obsolete_metadata    = 335544356)
      INTEGER*4 ISC_open_trans          
      PARAMETER (ISC_open_trans           = 335544357)
      INTEGER*4 ISC_port_len            
      PARAMETER (ISC_port_len             = 335544358)
      INTEGER*4 ISC_read_only_field     
      PARAMETER (ISC_read_only_field      = 335544359)
      INTEGER*4 ISC_read_only_rel       
      PARAMETER (ISC_read_only_rel        = 335544360)
      INTEGER*4 ISC_read_only_trans     
      PARAMETER (ISC_read_only_trans      = 335544361)
      INTEGER*4 ISC_read_only_view      
      PARAMETER (ISC_read_only_view       = 335544362)
      INTEGER*4 ISC_req_no_trans        
      PARAMETER (ISC_req_no_trans         = 335544363)
      INTEGER*4 ISC_req_sync            
      PARAMETER (ISC_req_sync             = 335544364)
      INTEGER*4 ISC_req_wrong_db        
      PARAMETER (ISC_req_wrong_db         = 335544365)
      INTEGER*4 ISC_segment             
      PARAMETER (ISC_segment              = 335544366)
      INTEGER*4 ISC_segstr_eof          
      PARAMETER (ISC_segstr_eof           = 335544367)
      INTEGER*4 ISC_segstr_no_op        
      PARAMETER (ISC_segstr_no_op         = 335544368)
      INTEGER*4 ISC_segstr_no_read      
      PARAMETER (ISC_segstr_no_read       = 335544369)
      INTEGER*4 ISC_segstr_no_trans     
      PARAMETER (ISC_segstr_no_trans      = 335544370)
      INTEGER*4 ISC_segstr_no_write     
      PARAMETER (ISC_segstr_no_write      = 335544371)
      INTEGER*4 ISC_segstr_wrong_db     
      PARAMETER (ISC_segstr_wrong_db      = 335544372)
      INTEGER*4 ISC_sys_request         
      PARAMETER (ISC_sys_request          = 335544373)
      INTEGER*4 ISC_stream_eof          
      PARAMETER (ISC_stream_eof           = 335544374)
      INTEGER*4 ISC_unavailable         
      PARAMETER (ISC_unavailable          = 335544375)
      INTEGER*4 ISC_unres_rel           
      PARAMETER (ISC_unres_rel            = 335544376)
      INTEGER*4 ISC_uns_ext             
      PARAMETER (ISC_uns_ext              = 335544377)
      INTEGER*4 ISC_wish_list           
      PARAMETER (ISC_wish_list            = 335544378)
      INTEGER*4 ISC_wrong_ods           
      PARAMETER (ISC_wrong_ods            = 335544379)
      INTEGER*4 ISC_wronumarg           
      PARAMETER (ISC_wronumarg            = 335544380)
      INTEGER*4 ISC_imp_exc             
      PARAMETER (ISC_imp_exc              = 335544381)
      INTEGER*4 ISC_random              
      PARAMETER (ISC_random               = 335544382)
      INTEGER*4 ISC_fatal_conflict      
      PARAMETER (ISC_fatal_conflict       = 335544383)
      INTEGER*4 ISC_badblk              
      PARAMETER (ISC_badblk               = 335544384)
      INTEGER*4 ISC_invpoolcl           
      PARAMETER (ISC_invpoolcl            = 335544385)
      INTEGER*4 ISC_nopoolids           
      PARAMETER (ISC_nopoolids            = 335544386)
      INTEGER*4 ISC_relbadblk           
      PARAMETER (ISC_relbadblk            = 335544387)
      INTEGER*4 ISC_blktoobig           
      PARAMETER (ISC_blktoobig            = 335544388)
      INTEGER*4 ISC_bufexh              
      PARAMETER (ISC_bufexh               = 335544389)
      INTEGER*4 ISC_syntaxerr           
      PARAMETER (ISC_syntaxerr            = 335544390)
      INTEGER*4 ISC_bufinuse            
      PARAMETER (ISC_bufinuse             = 335544391)
      INTEGER*4 ISC_bdbincon            
      PARAMETER (ISC_bdbincon             = 335544392)
      INTEGER*4 ISC_reqinuse            
      PARAMETER (ISC_reqinuse             = 335544393)
      INTEGER*4 ISC_badodsver           
      PARAMETER (ISC_badodsver            = 335544394)
      INTEGER*4 ISC_relnotdef           
      PARAMETER (ISC_relnotdef            = 335544395)
      INTEGER*4 ISC_fldnotdef           
      PARAMETER (ISC_fldnotdef            = 335544396)
      INTEGER*4 ISC_dirtypage           
      PARAMETER (ISC_dirtypage            = 335544397)
      INTEGER*4 ISC_waifortra           
      PARAMETER (ISC_waifortra            = 335544398)
      INTEGER*4 ISC_doubleloc           
      PARAMETER (ISC_doubleloc            = 335544399)
      INTEGER*4 ISC_nodnotfnd           
      PARAMETER (ISC_nodnotfnd            = 335544400)
      INTEGER*4 ISC_dupnodfnd           
      PARAMETER (ISC_dupnodfnd            = 335544401)
      INTEGER*4 ISC_locnotmar           
      PARAMETER (ISC_locnotmar            = 335544402)
      INTEGER*4 ISC_badpagtyp           
      PARAMETER (ISC_badpagtyp            = 335544403)
      INTEGER*4 ISC_corrupt             
      PARAMETER (ISC_corrupt              = 335544404)
      INTEGER*4 ISC_badpage             
      PARAMETER (ISC_badpage              = 335544405)
      INTEGER*4 ISC_badindex            
      PARAMETER (ISC_badindex             = 335544406)
      INTEGER*4 ISC_dbbnotzer           
      PARAMETER (ISC_dbbnotzer            = 335544407)
      INTEGER*4 ISC_tranotzer           
      PARAMETER (ISC_tranotzer            = 335544408)
      INTEGER*4 ISC_trareqmis           
      PARAMETER (ISC_trareqmis            = 335544409)
      INTEGER*4 ISC_badhndcnt           
      PARAMETER (ISC_badhndcnt            = 335544410)
      INTEGER*4 ISC_wrotpbver           
      PARAMETER (ISC_wrotpbver            = 335544411)
      INTEGER*4 ISC_wroblrver           
      PARAMETER (ISC_wroblrver            = 335544412)
      INTEGER*4 ISC_wrodpbver           
      PARAMETER (ISC_wrodpbver            = 335544413)
      INTEGER*4 ISC_blobnotsup          
      PARAMETER (ISC_blobnotsup           = 335544414)
      INTEGER*4 ISC_badrelation         
      PARAMETER (ISC_badrelation          = 335544415)
      INTEGER*4 ISC_nodetach            
      PARAMETER (ISC_nodetach             = 335544416)
      INTEGER*4 ISC_notremote           
      PARAMETER (ISC_notremote            = 335544417)
      INTEGER*4 ISC_trainlim            
      PARAMETER (ISC_trainlim             = 335544418)
      INTEGER*4 ISC_notinlim            
      PARAMETER (ISC_notinlim             = 335544419)
      INTEGER*4 ISC_traoutsta           
      PARAMETER (ISC_traoutsta            = 335544420)
      INTEGER*4 ISC_connect_reject      
      PARAMETER (ISC_connect_reject       = 335544421)
      INTEGER*4 ISC_dbfile              
      PARAMETER (ISC_dbfile               = 335544422)
      INTEGER*4 ISC_orphan              
      PARAMETER (ISC_orphan               = 335544423)
      INTEGER*4 ISC_no_lock_mgr         
      PARAMETER (ISC_no_lock_mgr          = 335544424)
      INTEGER*4 ISC_ctxinuse            
      PARAMETER (ISC_ctxinuse             = 335544425)
      INTEGER*4 ISC_ctxnotdef           
      PARAMETER (ISC_ctxnotdef            = 335544426)
      INTEGER*4 ISC_datnotsup           
      PARAMETER (ISC_datnotsup            = 335544427)
      INTEGER*4 ISC_badmsgnum           
      PARAMETER (ISC_badmsgnum            = 335544428)
      INTEGER*4 ISC_badparnum           
      PARAMETER (ISC_badparnum            = 335544429)
      INTEGER*4 ISC_virmemexh           
      PARAMETER (ISC_virmemexh            = 335544430)
      INTEGER*4 ISC_blocking_signal     
      PARAMETER (ISC_blocking_signal      = 335544431)
      INTEGER*4 ISC_lockmanerr          
      PARAMETER (ISC_lockmanerr           = 335544432)
      INTEGER*4 ISC_journerr            
      PARAMETER (ISC_journerr             = 335544433)
      INTEGER*4 ISC_keytoobig           
      PARAMETER (ISC_keytoobig            = 335544434)
      INTEGER*4 ISC_nullsegkey          
      PARAMETER (ISC_nullsegkey           = 335544435)
      INTEGER*4 ISC_sqlerr              
      PARAMETER (ISC_sqlerr               = 335544436)
      INTEGER*4 ISC_wrodynver           
      PARAMETER (ISC_wrodynver            = 335544437)
      INTEGER*4 ISC_funnotdef           
      PARAMETER (ISC_funnotdef            = 335544438)
      INTEGER*4 ISC_funmismat           
      PARAMETER (ISC_funmismat            = 335544439)
      INTEGER*4 ISC_bad_msg_vec         
      PARAMETER (ISC_bad_msg_vec          = 335544440)
      INTEGER*4 ISC_bad_detach          
      PARAMETER (ISC_bad_detach           = 335544441)
      INTEGER*4 ISC_noargacc_read       
      PARAMETER (ISC_noargacc_read        = 335544442)
      INTEGER*4 ISC_noargacc_write      
      PARAMETER (ISC_noargacc_write       = 335544443)
      INTEGER*4 ISC_read_only           
      PARAMETER (ISC_read_only            = 335544444)
      INTEGER*4 ISC_ext_err             
      PARAMETER (ISC_ext_err              = 335544445)
      INTEGER*4 ISC_non_updatable       
      PARAMETER (ISC_non_updatable        = 335544446)
      INTEGER*4 ISC_no_rollback         
      PARAMETER (ISC_no_rollback          = 335544447)
      INTEGER*4 ISC_bad_sec_info        
      PARAMETER (ISC_bad_sec_info         = 335544448)
      INTEGER*4 ISC_invalid_sec_info    
      PARAMETER (ISC_invalid_sec_info     = 335544449)
      INTEGER*4 ISC_misc_interpreted    
      PARAMETER (ISC_misc_interpreted     = 335544450)
      INTEGER*4 ISC_update_conflict     
      PARAMETER (ISC_update_conflict      = 335544451)
      INTEGER*4 ISC_unlicensed          
      PARAMETER (ISC_unlicensed           = 335544452)
      INTEGER*4 ISC_obj_in_use          
      PARAMETER (ISC_obj_in_use           = 335544453)
      INTEGER*4 ISC_nofilter            
      PARAMETER (ISC_nofilter             = 335544454)
      INTEGER*4 ISC_shadow_accessed     
      PARAMETER (ISC_shadow_accessed      = 335544455)
      INTEGER*4 ISC_invalid_sdl         
      PARAMETER (ISC_invalid_sdl          = 335544456)
      INTEGER*4 ISC_out_of_bounds       
      PARAMETER (ISC_out_of_bounds        = 335544457)
      INTEGER*4 ISC_invalid_dimension   
      PARAMETER (ISC_invalid_dimension    = 335544458)
      INTEGER*4 ISC_rec_in_limbo        
      PARAMETER (ISC_rec_in_limbo         = 335544459)
      INTEGER*4 ISC_shadow_missing      
      PARAMETER (ISC_shadow_missing       = 335544460)
      INTEGER*4 ISC_cant_validate       
      PARAMETER (ISC_cant_validate        = 335544461)
      INTEGER*4 ISC_cant_start_journal  
      PARAMETER (ISC_cant_start_journal   = 335544462)
      INTEGER*4 ISC_gennotdef           
      PARAMETER (ISC_gennotdef            = 335544463)
      INTEGER*4 ISC_cant_start_logging  
      PARAMETER (ISC_cant_start_logging   = 335544464)
      INTEGER*4 ISC_bad_segstr_type     
      PARAMETER (ISC_bad_segstr_type      = 335544465)
      INTEGER*4 ISC_foreign_key         
      PARAMETER (ISC_foreign_key          = 335544466)
      INTEGER*4 ISC_high_minor          
      PARAMETER (ISC_high_minor           = 335544467)
      INTEGER*4 ISC_tra_state           
      PARAMETER (ISC_tra_state            = 335544468)
      INTEGER*4 ISC_trans_invalid       
      PARAMETER (ISC_trans_invalid        = 335544469)
      INTEGER*4 ISC_buf_invalid         
      PARAMETER (ISC_buf_invalid          = 335544470)
      INTEGER*4 ISC_indexnotdefined     
      PARAMETER (ISC_indexnotdefined      = 335544471)
      INTEGER*4 ISC_login               
      PARAMETER (ISC_login                = 335544472)
      INTEGER*4 ISC_invalid_bookmark    
      PARAMETER (ISC_invalid_bookmark     = 335544473)
      INTEGER*4 ISC_bad_lock_level      
      PARAMETER (ISC_bad_lock_level       = 335544474)
      INTEGER*4 ISC_relation_lock       
      PARAMETER (ISC_relation_lock        = 335544475)
      INTEGER*4 ISC_record_lock         
      PARAMETER (ISC_record_lock          = 335544476)
      INTEGER*4 ISC_max_idx             
      PARAMETER (ISC_max_idx              = 335544477)
      INTEGER*4 ISC_jrn_enable          
      PARAMETER (ISC_jrn_enable           = 335544478)
      INTEGER*4 ISC_old_failure         
      PARAMETER (ISC_old_failure          = 335544479)
      INTEGER*4 ISC_old_in_progress     
      PARAMETER (ISC_old_in_progress      = 335544480)
      INTEGER*4 ISC_old_no_space        
      PARAMETER (ISC_old_no_space         = 335544481)
      INTEGER*4 ISC_no_wal_no_jrn       
      PARAMETER (ISC_no_wal_no_jrn        = 335544482)
      INTEGER*4 ISC_num_old_files       
      PARAMETER (ISC_num_old_files        = 335544483)
      INTEGER*4 ISC_wal_file_open       
      PARAMETER (ISC_wal_file_open        = 335544484)
      INTEGER*4 ISC_bad_stmt_handle     
      PARAMETER (ISC_bad_stmt_handle      = 335544485)
      INTEGER*4 ISC_wal_failure         
      PARAMETER (ISC_wal_failure          = 335544486)
      INTEGER*4 ISC_walw_err            
      PARAMETER (ISC_walw_err             = 335544487)
      INTEGER*4 ISC_logh_small          
      PARAMETER (ISC_logh_small           = 335544488)
      INTEGER*4 ISC_logh_inv_version    
      PARAMETER (ISC_logh_inv_version     = 335544489)
      INTEGER*4 ISC_logh_open_flag      
      PARAMETER (ISC_logh_open_flag       = 335544490)
      INTEGER*4 ISC_logh_open_flag2     
      PARAMETER (ISC_logh_open_flag2      = 335544491)
      INTEGER*4 ISC_logh_diff_dbname    
      PARAMETER (ISC_logh_diff_dbname     = 335544492)
      INTEGER*4 ISC_logf_unexpected_eof 
      PARAMETER (ISC_logf_unexpected_eof  = 335544493)
      INTEGER*4 ISC_logr_incomplete     
      PARAMETER (ISC_logr_incomplete      = 335544494)
      INTEGER*4 ISC_logr_header_small   
      PARAMETER (ISC_logr_header_small    = 335544495)
      INTEGER*4 ISC_logb_small          
      PARAMETER (ISC_logb_small           = 335544496)
      INTEGER*4 ISC_wal_illegal_attach  
      PARAMETER (ISC_wal_illegal_attach   = 335544497)
      INTEGER*4 ISC_wal_invalid_wpb     
      PARAMETER (ISC_wal_invalid_wpb      = 335544498)
      INTEGER*4 ISC_wal_err_rollover    
      PARAMETER (ISC_wal_err_rollover     = 335544499)
      INTEGER*4 ISC_no_wal              
      PARAMETER (ISC_no_wal               = 335544500)
      INTEGER*4 ISC_drop_wal            
      PARAMETER (ISC_drop_wal             = 335544501)
      INTEGER*4 ISC_stream_not_defined  
      PARAMETER (ISC_stream_not_defined   = 335544502)
      INTEGER*4 ISC_wal_subsys_error    
      PARAMETER (ISC_wal_subsys_error     = 335544503)
      INTEGER*4 ISC_wal_subsys_corrupt  
      PARAMETER (ISC_wal_subsys_corrupt   = 335544504)
      INTEGER*4 ISC_no_archive          
      PARAMETER (ISC_no_archive           = 335544505)
      INTEGER*4 ISC_shutinprog          
      PARAMETER (ISC_shutinprog           = 335544506)
      INTEGER*4 ISC_range_in_use        
      PARAMETER (ISC_range_in_use         = 335544507)
      INTEGER*4 ISC_range_not_found     
      PARAMETER (ISC_range_not_found      = 335544508)
      INTEGER*4 ISC_charset_not_found   
      PARAMETER (ISC_charset_not_found    = 335544509)
      INTEGER*4 ISC_lock_timeout        
      PARAMETER (ISC_lock_timeout         = 335544510)
      INTEGER*4 ISC_prcnotdef           
      PARAMETER (ISC_prcnotdef            = 335544511)
      INTEGER*4 ISC_prcmismat           
      PARAMETER (ISC_prcmismat            = 335544512)
      INTEGER*4 ISC_wal_bugcheck        
      PARAMETER (ISC_wal_bugcheck         = 335544513)
      INTEGER*4 ISC_wal_cant_expand     
      PARAMETER (ISC_wal_cant_expand      = 335544514)
      INTEGER*4 ISC_codnotdef           
      PARAMETER (ISC_codnotdef            = 335544515)
      INTEGER*4 ISC_xcpnotdef           
      PARAMETER (ISC_xcpnotdef            = 335544516)
      INTEGER*4 ISC_except              
      PARAMETER (ISC_except               = 335544517)
      INTEGER*4 ISC_cache_restart       
      PARAMETER (ISC_cache_restart        = 335544518)
      INTEGER*4 ISC_bad_lock_handle     
      PARAMETER (ISC_bad_lock_handle      = 335544519)
      INTEGER*4 ISC_jrn_present         
      PARAMETER (ISC_jrn_present          = 335544520)
      INTEGER*4 ISC_wal_err_rollover2   
      PARAMETER (ISC_wal_err_rollover2    = 335544521)
      INTEGER*4 ISC_wal_err_logwrite    
      PARAMETER (ISC_wal_err_logwrite     = 335544522)
      INTEGER*4 ISC_wal_err_jrn_comm    
      PARAMETER (ISC_wal_err_jrn_comm     = 335544523)
      INTEGER*4 ISC_wal_err_expansion   
      PARAMETER (ISC_wal_err_expansion    = 335544524)
      INTEGER*4 ISC_wal_err_setup       
      PARAMETER (ISC_wal_err_setup        = 335544525)
      INTEGER*4 ISC_wal_err_ww_sync     
      PARAMETER (ISC_wal_err_ww_sync      = 335544526)
      INTEGER*4 ISC_wal_err_ww_start    
      PARAMETER (ISC_wal_err_ww_start     = 335544527)
      INTEGER*4 ISC_shutdown            
      PARAMETER (ISC_shutdown             = 335544528)
      INTEGER*4 ISC_existing_priv_mod   
      PARAMETER (ISC_existing_priv_mod    = 335544529)
      INTEGER*4 ISC_primary_key_ref     
      PARAMETER (ISC_primary_key_ref      = 335544530)
      INTEGER*4 ISC_primary_key_notnull 
      PARAMETER (ISC_primary_key_notnull  = 335544531)
      INTEGER*4 ISC_ref_cnstrnt_notfound
      PARAMETER (ISC_ref_cnstrnt_notfound = 335544532)
      INTEGER*4 ISC_foreign_key_notfound
      PARAMETER (ISC_foreign_key_notfound = 335544533)
      INTEGER*4 ISC_ref_cnstrnt_update  
      PARAMETER (ISC_ref_cnstrnt_update   = 335544534)
      INTEGER*4 ISC_check_cnstrnt_update
      PARAMETER (ISC_check_cnstrnt_update = 335544535)
      INTEGER*4 ISC_check_cnstrnt_del   
      PARAMETER (ISC_check_cnstrnt_del    = 335544536)
      INTEGER*4 ISC_integ_index_seg_del 
      PARAMETER (ISC_integ_index_seg_del  = 335544537)
      INTEGER*4 ISC_integ_index_seg_mod 
      PARAMETER (ISC_integ_index_seg_mod  = 335544538)
      INTEGER*4 ISC_integ_index_del     
      PARAMETER (ISC_integ_index_del      = 335544539)
      INTEGER*4 ISC_integ_index_mod     
      PARAMETER (ISC_integ_index_mod      = 335544540)
      INTEGER*4 ISC_check_trig_del      
      PARAMETER (ISC_check_trig_del       = 335544541)
      INTEGER*4 ISC_check_trig_update   
      PARAMETER (ISC_check_trig_update    = 335544542)
      INTEGER*4 ISC_cnstrnt_fld_del     
      PARAMETER (ISC_cnstrnt_fld_del      = 335544543)
      INTEGER*4 ISC_cnstrnt_fld_rename  
      PARAMETER (ISC_cnstrnt_fld_rename   = 335544544)
      INTEGER*4 ISC_rel_cnstrnt_update  
      PARAMETER (ISC_rel_cnstrnt_update   = 335544545)
      INTEGER*4 ISC_constaint_on_view   
      PARAMETER (ISC_constaint_on_view    = 335544546)
      INTEGER*4 ISC_invld_cnstrnt_type  
      PARAMETER (ISC_invld_cnstrnt_type   = 335544547)
      INTEGER*4 ISC_primary_key_exists  
      PARAMETER (ISC_primary_key_exists   = 335544548)
      INTEGER*4 ISC_systrig_update      
      PARAMETER (ISC_systrig_update       = 335544549)
      INTEGER*4 ISC_not_rel_owner       
      PARAMETER (ISC_not_rel_owner        = 335544550)
      INTEGER*4 ISC_grant_obj_notfound  
      PARAMETER (ISC_grant_obj_notfound   = 335544551)
      INTEGER*4 ISC_grant_fld_notfound  
      PARAMETER (ISC_grant_fld_notfound   = 335544552)
      INTEGER*4 ISC_grant_nopriv        
      PARAMETER (ISC_grant_nopriv         = 335544553)
      INTEGER*4 ISC_nonsql_security_rel 
      PARAMETER (ISC_nonsql_security_rel  = 335544554)
      INTEGER*4 ISC_nonsql_security_fld 
      PARAMETER (ISC_nonsql_security_fld  = 335544555)
      INTEGER*4 ISC_wal_cache_err       
      PARAMETER (ISC_wal_cache_err        = 335544556)
      INTEGER*4 ISC_shutfail            
      PARAMETER (ISC_shutfail             = 335544557)
      INTEGER*4 ISC_check_constraint    
      PARAMETER (ISC_check_constraint     = 335544558)
      INTEGER*4 ISC_bad_svc_handle      
      PARAMETER (ISC_bad_svc_handle       = 335544559)
      INTEGER*4 ISC_shutwarn            
      PARAMETER (ISC_shutwarn             = 335544560)
      INTEGER*4 ISC_wrospbver           
      PARAMETER (ISC_wrospbver            = 335544561)
      INTEGER*4 ISC_bad_spb_form        
      PARAMETER (ISC_bad_spb_form         = 335544562)
      INTEGER*4 ISC_svcnotdef           
      PARAMETER (ISC_svcnotdef            = 335544563)
      INTEGER*4 ISC_no_jrn              
      PARAMETER (ISC_no_jrn               = 335544564)
      INTEGER*4 ISC_transliteration_fail
      PARAMETER (ISC_transliteration_fail = 335544565)
      INTEGER*4 ISC_start_cm_for_wal    
      PARAMETER (ISC_start_cm_for_wal     = 335544566)
      INTEGER*4 ISC_wal_ovflow_log_requi
      PARAMETER (ISC_wal_ovflow_log_requi = 335544567)
      INTEGER*4 ISC_text_subtype        
      PARAMETER (ISC_text_subtype         = 335544568)
      INTEGER*4 ISC_dsql_error          
      PARAMETER (ISC_dsql_error           = 335544569)
      INTEGER*4 ISC_dsql_command_err    
      PARAMETER (ISC_dsql_command_err     = 335544570)
      INTEGER*4 ISC_dsql_constant_err   
      PARAMETER (ISC_dsql_constant_err    = 335544571)
      INTEGER*4 ISC_dsql_cursor_err     
      PARAMETER (ISC_dsql_cursor_err      = 335544572)
      INTEGER*4 ISC_dsql_datatype_err   
      PARAMETER (ISC_dsql_datatype_err    = 335544573)
      INTEGER*4 ISC_dsql_decl_err       
      PARAMETER (ISC_dsql_decl_err        = 335544574)
      INTEGER*4 ISC_dsql_cursor_update_e
      PARAMETER (ISC_dsql_cursor_update_e = 335544575)
      INTEGER*4 ISC_dsql_cursor_open_err
      PARAMETER (ISC_dsql_cursor_open_err = 335544576)
      INTEGER*4 ISC_dsql_cursor_close_er
      PARAMETER (ISC_dsql_cursor_close_er = 335544577)
      INTEGER*4 ISC_dsql_field_err      
      PARAMETER (ISC_dsql_field_err       = 335544578)
      INTEGER*4 ISC_dsql_internal_err   
      PARAMETER (ISC_dsql_internal_err    = 335544579)
      INTEGER*4 ISC_dsql_relation_err   
      PARAMETER (ISC_dsql_relation_err    = 335544580)
      INTEGER*4 ISC_dsql_procedure_err  
      PARAMETER (ISC_dsql_procedure_err   = 335544581)
      INTEGER*4 ISC_dsql_request_err    
      PARAMETER (ISC_dsql_request_err     = 335544582)
      INTEGER*4 ISC_dsql_sqlda_err      
      PARAMETER (ISC_dsql_sqlda_err       = 335544583)
      INTEGER*4 ISC_dsql_var_count_err  
      PARAMETER (ISC_dsql_var_count_err   = 335544584)
      INTEGER*4 ISC_dsql_stmt_handle    
      PARAMETER (ISC_dsql_stmt_handle     = 335544585)
      INTEGER*4 ISC_dsql_function_err   
      PARAMETER (ISC_dsql_function_err    = 335544586)
      INTEGER*4 ISC_dsql_blob_err       
      PARAMETER (ISC_dsql_blob_err        = 335544587)
      INTEGER*4 ISC_collation_not_found 
      PARAMETER (ISC_collation_not_found  = 335544588)
      INTEGER*4 ISC_collation_not_for_ch
      PARAMETER (ISC_collation_not_for_ch = 335544589)
      INTEGER*4 ISC_dsql_dup_option     
      PARAMETER (ISC_dsql_dup_option      = 335544590)
      INTEGER*4 ISC_dsql_tran_err       
      PARAMETER (ISC_dsql_tran_err        = 335544591)
      INTEGER*4 ISC_dsql_invalid_array  
      PARAMETER (ISC_dsql_invalid_array   = 335544592)
      INTEGER*4 ISC_dsql_max_arr_dim_exc
      PARAMETER (ISC_dsql_max_arr_dim_exc = 335544593)
      INTEGER*4 ISC_dsql_arr_range_error
      PARAMETER (ISC_dsql_arr_range_error = 335544594)
      INTEGER*4 ISC_dsql_trigger_err    
      PARAMETER (ISC_dsql_trigger_err     = 335544595)
      INTEGER*4 ISC_dsql_subselect_err  
      PARAMETER (ISC_dsql_subselect_err   = 335544596)
      INTEGER*4 ISC_dsql_crdb_prepare_er
      PARAMETER (ISC_dsql_crdb_prepare_er = 335544597)
      INTEGER*4 ISC_specify_field_err   
      PARAMETER (ISC_specify_field_err    = 335544598)
      INTEGER*4 ISC_num_field_err       
      PARAMETER (ISC_num_field_err        = 335544599)
      INTEGER*4 ISC_col_name_err        
      PARAMETER (ISC_col_name_err         = 335544600)
      INTEGER*4 ISC_where_err           
      PARAMETER (ISC_where_err            = 335544601)
      INTEGER*4 ISC_table_view_err      
      PARAMETER (ISC_table_view_err       = 335544602)
      INTEGER*4 ISC_distinct_err        
      PARAMETER (ISC_distinct_err         = 335544603)
      INTEGER*4 ISC_key_field_count_err 
      PARAMETER (ISC_key_field_count_err  = 335544604)
      INTEGER*4 ISC_subquery_err        
      PARAMETER (ISC_subquery_err         = 335544605)
      INTEGER*4 ISC_expression_eval_err 
      PARAMETER (ISC_expression_eval_err  = 335544606)
      INTEGER*4 ISC_node_err            
      PARAMETER (ISC_node_err             = 335544607)
      INTEGER*4 ISC_command_end_err     
      PARAMETER (ISC_command_end_err      = 335544608)
      INTEGER*4 ISC_index_name          
      PARAMETER (ISC_index_name           = 335544609)
      INTEGER*4 ISC_exception_name      
      PARAMETER (ISC_exception_name       = 335544610)
      INTEGER*4 ISC_field_name          
      PARAMETER (ISC_field_name           = 335544611)
      INTEGER*4 ISC_token_err           
      PARAMETER (ISC_token_err            = 335544612)
      INTEGER*4 ISC_union_err           
      PARAMETER (ISC_union_err            = 335544613)
      INTEGER*4 ISC_dsql_construct_err  
      PARAMETER (ISC_dsql_construct_err   = 335544614)
      INTEGER*4 ISC_field_aggregate_err 
      PARAMETER (ISC_field_aggregate_err  = 335544615)
      INTEGER*4 ISC_field_ref_err       
      PARAMETER (ISC_field_ref_err        = 335544616)
      INTEGER*4 ISC_order_by_err        
      PARAMETER (ISC_order_by_err         = 335544617)
      INTEGER*4 ISC_return_mode_err     
      PARAMETER (ISC_return_mode_err      = 335544618)
      INTEGER*4 ISC_extern_func_err     
      PARAMETER (ISC_extern_func_err      = 335544619)
      INTEGER*4 ISC_alias_conflict_err  
      PARAMETER (ISC_alias_conflict_err   = 335544620)
      INTEGER*4 ISC_procedure_conflict_e
      PARAMETER (ISC_procedure_conflict_e = 335544621)
      INTEGER*4 ISC_relation_conflict_er
      PARAMETER (ISC_relation_conflict_er = 335544622)
      INTEGER*4 ISC_dsql_domain_err     
      PARAMETER (ISC_dsql_domain_err      = 335544623)
      INTEGER*4 ISC_idx_seg_err         
      PARAMETER (ISC_idx_seg_err          = 335544624)
      INTEGER*4 ISC_node_name_err       
      PARAMETER (ISC_node_name_err        = 335544625)
      INTEGER*4 ISC_table_name          
      PARAMETER (ISC_table_name           = 335544626)
      INTEGER*4 ISC_proc_name           
      PARAMETER (ISC_proc_name            = 335544627)
      INTEGER*4 ISC_idx_create_err      
      PARAMETER (ISC_idx_create_err       = 335544628)
      INTEGER*4 ISC_wal_shadow_err      
      PARAMETER (ISC_wal_shadow_err       = 335544629)
      INTEGER*4 ISC_dependency          
      PARAMETER (ISC_dependency           = 335544630)
      INTEGER*4 ISC_idx_key_err         
      PARAMETER (ISC_idx_key_err          = 335544631)
      INTEGER*4 ISC_dsql_file_length_err
      PARAMETER (ISC_dsql_file_length_err = 335544632)
      INTEGER*4 ISC_dsql_shadow_number_e
      PARAMETER (ISC_dsql_shadow_number_e = 335544633)
      INTEGER*4 ISC_dsql_token_unk_err  
      PARAMETER (ISC_dsql_token_unk_err   = 335544634)
      INTEGER*4 ISC_dsql_no_relation_ali
      PARAMETER (ISC_dsql_no_relation_ali = 335544635)
      INTEGER*4 ISC_indexname           
      PARAMETER (ISC_indexname            = 335544636)
      INTEGER*4 ISC_no_stream_plan      
      PARAMETER (ISC_no_stream_plan       = 335544637)
      INTEGER*4 ISC_stream_twice        
      PARAMETER (ISC_stream_twice         = 335544638)
      INTEGER*4 ISC_stream_not_found    
      PARAMETER (ISC_stream_not_found     = 335544639)
      INTEGER*4 ISC_collation_requires_t
      PARAMETER (ISC_collation_requires_t = 335544640)
      INTEGER*4 ISC_dsql_domain_not_foun
      PARAMETER (ISC_dsql_domain_not_foun = 335544641)
      INTEGER*4 ISC_index_unused        
      PARAMETER (ISC_index_unused         = 335544642)
      INTEGER*4 ISC_dsql_self_join      
      PARAMETER (ISC_dsql_self_join       = 335544643)
      INTEGER*4 ISC_stream_bof          
      PARAMETER (ISC_stream_bof           = 335544644)
      INTEGER*4 ISC_stream_crack        
      PARAMETER (ISC_stream_crack         = 335544645)
      INTEGER*4 ISC_db_or_file_exists   
      PARAMETER (ISC_db_or_file_exists    = 335544646)
      INTEGER*4 ISC_invalid_operator    
      PARAMETER (ISC_invalid_operator     = 335544647)
      INTEGER*4 ISC_conn_lost           
      PARAMETER (ISC_conn_lost            = 335544648)
      INTEGER*4 ISC_bad_checksum        
      PARAMETER (ISC_bad_checksum         = 335544649)
      INTEGER*4 ISC_page_type_err       
      PARAMETER (ISC_page_type_err        = 335544650)
      INTEGER*4 ISC_ext_readonly_err    
      PARAMETER (ISC_ext_readonly_err     = 335544651)
      INTEGER*4 ISC_sing_select_err     
      PARAMETER (ISC_sing_select_err      = 335544652)
      INTEGER*4 ISC_psw_attach          
      PARAMETER (ISC_psw_attach           = 335544653)
      INTEGER*4 ISC_psw_start_trans     
      PARAMETER (ISC_psw_start_trans      = 335544654)
      INTEGER*4 ISC_invalid_direction   
      PARAMETER (ISC_invalid_direction    = 335544655)
      INTEGER*4 ISC_dsql_var_conflict   
      PARAMETER (ISC_dsql_var_conflict    = 335544656)
      INTEGER*4 ISC_dsql_no_blob_array  
      PARAMETER (ISC_dsql_no_blob_array   = 335544657)
      INTEGER*4 ISC_dsql_base_table     
      PARAMETER (ISC_dsql_base_table      = 335544658)
      INTEGER*4 ISC_duplicate_base_table
      PARAMETER (ISC_duplicate_base_table = 335544659)
      INTEGER*4 ISC_view_alias          
      PARAMETER (ISC_view_alias           = 335544660)
      INTEGER*4 ISC_index_root_page_full
      PARAMETER (ISC_index_root_page_full = 335544661)
      INTEGER*4 ISC_dsql_blob_type_unkno
      PARAMETER (ISC_dsql_blob_type_unkno = 335544662)
      INTEGER*4 ISC_req_max_clones_excee
      PARAMETER (ISC_req_max_clones_excee = 335544663)
      INTEGER*4 ISC_dsql_duplicate_spec 
      PARAMETER (ISC_dsql_duplicate_spec  = 335544664)
      INTEGER*4 ISC_unique_key_violation
      PARAMETER (ISC_unique_key_violation = 335544665)
      INTEGER*4 ISC_srvr_version_too_old
      PARAMETER (ISC_srvr_version_too_old = 335544666)
      INTEGER*4 ISC_drdb_completed_with_
      PARAMETER (ISC_drdb_completed_with_ = 335544667)
      INTEGER*4 ISC_dsql_procedure_use_e
      PARAMETER (ISC_dsql_procedure_use_e = 335544668)
      INTEGER*4 ISC_dsql_count_mismatch 
      PARAMETER (ISC_dsql_count_mismatch  = 335544669)
      INTEGER*4 ISC_blob_idx_err        
      PARAMETER (ISC_blob_idx_err         = 335544670)
      INTEGER*4 ISC_array_idx_err       
      PARAMETER (ISC_array_idx_err        = 335544671)
      INTEGER*4 ISC_key_field_err       
      PARAMETER (ISC_key_field_err        = 335544672)
      INTEGER*4 ISC_no_delete           
      PARAMETER (ISC_no_delete            = 335544673)
      INTEGER*4 ISC_del_last_field      
      PARAMETER (ISC_del_last_field       = 335544674)
      INTEGER*4 ISC_sort_err            
      PARAMETER (ISC_sort_err             = 335544675)
      INTEGER*4 ISC_sort_mem_err        
      PARAMETER (ISC_sort_mem_err         = 335544676)
      INTEGER*4 ISC_version_err         
      PARAMETER (ISC_version_err          = 335544677)
      INTEGER*4 ISC_inval_key_posn      
      PARAMETER (ISC_inval_key_posn       = 335544678)
      INTEGER*4 ISC_no_segments_err     
      PARAMETER (ISC_no_segments_err      = 335544679)
      INTEGER*4 ISC_crrp_data_err       
      PARAMETER (ISC_crrp_data_err        = 335544680)
      INTEGER*4 ISC_rec_size_err        
      PARAMETER (ISC_rec_size_err         = 335544681)
      INTEGER*4 ISC_dsql_field_ref      
      PARAMETER (ISC_dsql_field_ref       = 335544682)
      INTEGER*4 ISC_req_depth_exceeded  
      PARAMETER (ISC_req_depth_exceeded   = 335544683)
      INTEGER*4 ISC_no_field_access     
      PARAMETER (ISC_no_field_access      = 335544684)
      INTEGER*4 ISC_no_dbkey            
      PARAMETER (ISC_no_dbkey             = 335544685)
      INTEGER*4 ISC_jrn_format_err      
      PARAMETER (ISC_jrn_format_err       = 335544686)
      INTEGER*4 ISC_jrn_file_full       
      PARAMETER (ISC_jrn_file_full        = 335544687)
      INTEGER*4 ISC_dsql_open_cursor_req
      PARAMETER (ISC_dsql_open_cursor_req = 335544688)
      INTEGER*4 ISC_ib_error            
      PARAMETER (ISC_ib_error             = 335544689)
      INTEGER*4 ISC_cache_redef         
      PARAMETER (ISC_cache_redef          = 335544690)
      INTEGER*4 ISC_cache_too_small     
      PARAMETER (ISC_cache_too_small      = 335544691)
      INTEGER*4 ISC_log_redef           
      PARAMETER (ISC_log_redef            = 335544692)
      INTEGER*4 ISC_log_too_small       
      PARAMETER (ISC_log_too_small        = 335544693)
      INTEGER*4 ISC_partition_too_small 
      PARAMETER (ISC_partition_too_small  = 335544694)
      INTEGER*4 ISC_partition_not_supp  
      PARAMETER (ISC_partition_not_supp   = 335544695)
      INTEGER*4 ISC_log_length_spec     
      PARAMETER (ISC_log_length_spec      = 335544696)
      INTEGER*4 ISC_precision_err       
      PARAMETER (ISC_precision_err        = 335544697)
      INTEGER*4 ISC_scale_nogt          
      PARAMETER (ISC_scale_nogt           = 335544698)
      INTEGER*4 ISC_expec_short         
      PARAMETER (ISC_expec_short          = 335544699)
      INTEGER*4 ISC_expec_long          
      PARAMETER (ISC_expec_long           = 335544700)
      INTEGER*4 ISC_expec_ushort        
      PARAMETER (ISC_expec_ushort         = 335544701)
      INTEGER*4 ISC_like_escape_invalid 
      PARAMETER (ISC_like_escape_invalid  = 335544702)
      INTEGER*4 ISC_svcnoexe            
      PARAMETER (ISC_svcnoexe             = 335544703)
      INTEGER*4 ISC_net_lookup_err      
      PARAMETER (ISC_net_lookup_err       = 335544704)
      INTEGER*4 ISC_service_unknown     
      PARAMETER (ISC_service_unknown      = 335544705)
      INTEGER*4 ISC_host_unknown        
      PARAMETER (ISC_host_unknown         = 335544706)
      INTEGER*4 ISC_grant_nopriv_on_base
      PARAMETER (ISC_grant_nopriv_on_base = 335544707)
      INTEGER*4 ISC_dyn_fld_ambiguous   
      PARAMETER (ISC_dyn_fld_ambiguous    = 335544708)
      INTEGER*4 ISC_dsql_agg_ref_err    
      PARAMETER (ISC_dsql_agg_ref_err     = 335544709)
      INTEGER*4 ISC_complex_view        
      PARAMETER (ISC_complex_view         = 335544710)
      INTEGER*4 ISC_unprepared_stmt     
      PARAMETER (ISC_unprepared_stmt      = 335544711)
      INTEGER*4 ISC_expec_positive      
      PARAMETER (ISC_expec_positive       = 335544712)
      INTEGER*4 ISC_dsql_sqlda_value_err
      PARAMETER (ISC_dsql_sqlda_value_err = 335544713)
      INTEGER*4 ISC_invalid_array_id    
      PARAMETER (ISC_invalid_array_id     = 335544714)
      INTEGER*4 ISC_extfile_uns_op      
      PARAMETER (ISC_extfile_uns_op       = 335544715)
      INTEGER*4 ISC_svc_in_use          
      PARAMETER (ISC_svc_in_use           = 335544716)
      INTEGER*4 ISC_err_stack_limit     
      PARAMETER (ISC_err_stack_limit      = 335544717)
      INTEGER*4 ISC_invalid_key         
      PARAMETER (ISC_invalid_key          = 335544718)
      INTEGER*4 ISC_net_init_error      
      PARAMETER (ISC_net_init_error       = 335544719)
      INTEGER*4 ISC_loadlib_failure     
      PARAMETER (ISC_loadlib_failure      = 335544720)
      INTEGER*4 ISC_network_error       
      PARAMETER (ISC_network_error        = 335544721)
      INTEGER*4 ISC_net_connect_err     
      PARAMETER (ISC_net_connect_err      = 335544722)
      INTEGER*4 ISC_net_connect_listen_e
      PARAMETER (ISC_net_connect_listen_e = 335544723)
      INTEGER*4 ISC_net_event_connect_er
      PARAMETER (ISC_net_event_connect_er = 335544724)
      INTEGER*4 ISC_net_event_listen_err
      PARAMETER (ISC_net_event_listen_err = 335544725)
      INTEGER*4 ISC_net_read_err        
      PARAMETER (ISC_net_read_err         = 335544726)
      INTEGER*4 ISC_net_write_err       
      PARAMETER (ISC_net_write_err        = 335544727)
      INTEGER*4 ISC_integ_index_deactiva
      PARAMETER (ISC_integ_index_deactiva = 335544728)
      INTEGER*4 ISC_integ_deactivate_pri
      PARAMETER (ISC_integ_deactivate_pri = 335544729)
      INTEGER*4 ISC_cse_not_supported   
      PARAMETER (ISC_cse_not_supported    = 335544730)

      INTEGER*4 GDS__INT_ARG
      INTEGER*2 GDS__INT2
      INTEGER*4 ISC_INT_ARG
      INTEGER*2 ISC_INT2

      GDS__INT2 (GDS__INT_ARG) = GDS__INT_ARG
      ISC_INT2 (ISC_INT_ARG) = ISC_INT_ARG

