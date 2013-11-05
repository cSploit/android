/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef FB_API_PROTO_H
#define FB_API_PROTO_H

typedef ISC_STATUS ISC_EXPORT prototype_isc_attach_database(ISC_STATUS *,
										  short,
										  const char*,
										  isc_db_handle *,
										  short,
										  const char*);

typedef ISC_STATUS ISC_EXPORT prototype_isc_array_gen_sdl(ISC_STATUS *,
										const ISC_ARRAY_DESC*,
										short *,
										char *,
										short *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_array_get_slice(ISC_STATUS *,
										  isc_db_handle *,
										  isc_tr_handle *,
										  ISC_QUAD *,
										  const ISC_ARRAY_DESC*,
										  void *,
										  ISC_LONG *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_array_lookup_bounds(ISC_STATUS *,
											  isc_db_handle *,
											  isc_tr_handle *,
											  const char*,
											  const char*,
											  ISC_ARRAY_DESC *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_array_lookup_desc(ISC_STATUS *,
											isc_db_handle *,
											isc_tr_handle *,
											const char*,
											const char*,
											ISC_ARRAY_DESC *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_array_set_desc(ISC_STATUS *,
										 const char*,
										 const char*,
										 const short*,
										 const short*,
										 const short*,
										 ISC_ARRAY_DESC *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_array_put_slice(ISC_STATUS *,
										  isc_db_handle *,
										  isc_tr_handle *,
										  ISC_QUAD *,
										  const ISC_ARRAY_DESC*,
										  void *,
										  ISC_LONG *);

typedef void ISC_EXPORT prototype_isc_blob_default_desc(ISC_BLOB_DESC *,
									  const unsigned char*,
									  const unsigned char*);

typedef ISC_STATUS ISC_EXPORT prototype_isc_blob_gen_bpb(ISC_STATUS *,
									   const ISC_BLOB_DESC*,
									   const ISC_BLOB_DESC*,
									   unsigned short,
									   unsigned char *,
									   unsigned short *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_blob_info(ISC_STATUS *,
									isc_blob_handle *,
									short,
									const char*,
									short,
									char *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_blob_lookup_desc(ISC_STATUS *,
										   isc_db_handle *,
										   isc_tr_handle *,
										   const unsigned char*,
										   const unsigned char*,
										   ISC_BLOB_DESC *,
										   unsigned char *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_blob_set_desc(ISC_STATUS *,
										const unsigned char*,
										const unsigned char*,
										short,
										short,
										short,
										ISC_BLOB_DESC *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_cancel_blob(ISC_STATUS *,
									  isc_blob_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_cancel_events(ISC_STATUS *,
										isc_db_handle *,
										ISC_LONG *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_close_blob(ISC_STATUS *,
									 isc_blob_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_commit_retaining(ISC_STATUS *,
										   isc_tr_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_commit_transaction(ISC_STATUS *,
											 isc_tr_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_create_blob(ISC_STATUS *,
									  isc_db_handle *,
									  isc_tr_handle *,
									  isc_blob_handle *,
									  ISC_QUAD *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_create_blob2(ISC_STATUS *,
									   isc_db_handle *,
									   isc_tr_handle *,
									   isc_blob_handle *,
									   ISC_QUAD *,
									   short,
									   const char*);

typedef ISC_STATUS ISC_EXPORT prototype_isc_create_database(ISC_STATUS *,
										  short,
										  const char*,
										  isc_db_handle *,
										  short,
										  const char*,
										  short);

typedef ISC_STATUS ISC_EXPORT prototype_isc_database_info(ISC_STATUS *,
										isc_db_handle *,
										short,
										const char*,
										short,
										char *);

typedef void ISC_EXPORT prototype_isc_decode_date(const ISC_QUAD*,
								void *);

typedef void ISC_EXPORT prototype_isc_decode_sql_date(const ISC_DATE*,
									void *);

typedef void ISC_EXPORT prototype_isc_decode_sql_time(const ISC_TIME*,
									void *);

typedef void ISC_EXPORT prototype_isc_decode_timestamp(const ISC_TIMESTAMP*,
									 void *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_detach_database(ISC_STATUS *,
										  isc_db_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_drop_database(ISC_STATUS *,
										isc_db_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_allocate_statement(ISC_STATUS *,
												  isc_db_handle *,
												  isc_stmt_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_alloc_statement2(ISC_STATUS *,
												isc_db_handle *,
												isc_stmt_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_describe(ISC_STATUS *,
										isc_stmt_handle *,
										unsigned short,
										XSQLDA *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_describe_bind(ISC_STATUS *,
											 isc_stmt_handle *,
											 unsigned short,
											 XSQLDA *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_exec_immed2(ISC_STATUS *,
										   isc_db_handle *,
										   isc_tr_handle *,
										   unsigned short,
										   const char*,
										   unsigned short,
										   const XSQLDA *,
										   const XSQLDA *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_execute(ISC_STATUS *,
									   isc_tr_handle *,
									   isc_stmt_handle *,
									   unsigned short,
									   const XSQLDA *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_execute2(ISC_STATUS *,
										isc_tr_handle *,
										isc_stmt_handle *,
										unsigned short,
										const XSQLDA *,
										const XSQLDA *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_execute_immediate(ISC_STATUS *,
												 isc_db_handle *,
												 isc_tr_handle *,
												 unsigned short,
												 const char*,
												 unsigned short,
												 const XSQLDA *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_fetch(ISC_STATUS *,
									 isc_stmt_handle *,
									 unsigned short,
									 const XSQLDA *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_finish(isc_db_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_free_statement(ISC_STATUS *,
											  isc_stmt_handle *,
											  unsigned short);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_insert(ISC_STATUS *,
									  isc_stmt_handle *,
									  unsigned short,
									  XSQLDA *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_prepare(ISC_STATUS *,
									   isc_tr_handle *,
									   isc_stmt_handle *,
									   unsigned short,
									   const char*,
									   unsigned short,
									   XSQLDA *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_set_cursor_name(ISC_STATUS *,
											   isc_stmt_handle *,
											   const char*,
											   unsigned short);

typedef ISC_STATUS ISC_EXPORT prototype_isc_dsql_sql_info(ISC_STATUS *,
										isc_stmt_handle *,
										short,
										const char*,
										short,
										char *);

typedef void ISC_EXPORT prototype_isc_encode_date(const void*,
								ISC_QUAD *);

typedef void ISC_EXPORT prototype_isc_encode_sql_date(const void*,
									ISC_DATE *);

typedef void ISC_EXPORT prototype_isc_encode_sql_time(const void*,
									ISC_TIME *);

typedef void ISC_EXPORT prototype_isc_encode_timestamp(const void*,
									 ISC_TIMESTAMP *);

typedef ISC_LONG ISC_EXPORT_VARARG prototype_isc_event_block(char * *,
										   char * *,
										   unsigned short, ...);

typedef void ISC_EXPORT prototype_isc_event_counts(ISC_ULONG *,
								 short,
								 char *,
								 const char*);

/* 17 May 2001 - isc_expand_dpb is DEPRECATED */
typedef void ISC_EXPORT_VARARG prototype_isc_expand_dpb(char * *,
									  short *, ...);

typedef int ISC_EXPORT prototype_isc_modify_dpb(char * *,
							  short *,
							  unsigned short,
							  const char*,
							  short);

typedef ISC_LONG ISC_EXPORT prototype_isc_free(char *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_get_segment(ISC_STATUS *,
									  isc_blob_handle *,
									  unsigned short *,
									  unsigned short,
									  char *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_get_slice(ISC_STATUS *,
									isc_db_handle *,
									isc_tr_handle *,
									ISC_QUAD *,
									short,
									const char*,
									short,
									const ISC_LONG*,
									ISC_LONG,
									void *,
									ISC_LONG *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_interprete(char *,
									 ISC_STATUS * *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_open_blob(ISC_STATUS *,
									isc_db_handle *,
									isc_tr_handle *,
									isc_blob_handle *,
									ISC_QUAD *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_open_blob2(ISC_STATUS *,
									 isc_db_handle *,
									 isc_tr_handle *,
									 isc_blob_handle *,
									 ISC_QUAD *,
									 ISC_USHORT,
									 const ISC_UCHAR*);

typedef ISC_STATUS ISC_EXPORT prototype_isc_prepare_transaction2(ISC_STATUS *,
											   isc_tr_handle *,
											   ISC_USHORT,
											   const ISC_UCHAR*);

typedef void ISC_EXPORT prototype_isc_print_sqlerror(ISC_SHORT,
								   const ISC_STATUS*);

typedef ISC_STATUS ISC_EXPORT prototype_isc_print_status(const ISC_STATUS*);

typedef ISC_STATUS ISC_EXPORT prototype_isc_put_segment(ISC_STATUS *,
									  isc_blob_handle *,
									  unsigned short,
									  const char*);

typedef ISC_STATUS ISC_EXPORT prototype_isc_put_slice(ISC_STATUS *,
									isc_db_handle *,
									isc_tr_handle *,
									ISC_QUAD *,
									short,
									const char*,
									short,
									const ISC_LONG*,
									ISC_LONG,
									void *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_que_events(ISC_STATUS *,
									 isc_db_handle *,
									 ISC_LONG *,
									 ISC_USHORT,
									 const ISC_UCHAR*,
									 isc_callback,
									 void *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_rollback_retaining(ISC_STATUS *,
											 isc_tr_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_rollback_transaction(ISC_STATUS *,
											   isc_tr_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_start_multiple(ISC_STATUS *,
										 isc_tr_handle *,
										 short,
										 void *);

typedef ISC_STATUS ISC_EXPORT_VARARG prototype_isc_start_transaction(ISC_STATUS *,
												   isc_tr_handle *,
												   short, ...);

typedef ISC_STATUS ISC_EXPORT_VARARG prototype_isc_reconnect_transaction(ISC_STATUS *,
                                                   isc_db_handle *,
                                                   isc_tr_handle *,
                                                   short,
                                                   const char*);

typedef ISC_LONG ISC_EXPORT prototype_isc_sqlcode(const ISC_STATUS*);

typedef void ISC_EXPORT prototype_isc_sql_interprete(short,
								   char *,
								   short);

typedef ISC_STATUS ISC_EXPORT prototype_isc_transaction_info(ISC_STATUS *,
										   isc_tr_handle *,
										   short,
										   const char*,
										   short,
										   char *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_transact_request(ISC_STATUS *,
										   isc_db_handle *,
										   isc_tr_handle *,
										   unsigned short,
										   char *,
										   unsigned short,
										   char *,
										   unsigned short,
										   char *);

typedef ISC_LONG ISC_EXPORT prototype_isc_vax_integer(const char*,
									short);

typedef ISC_INT64 ISC_EXPORT prototype_isc_portable_integer(const unsigned char*,
										  short);

typedef ISC_STATUS ISC_EXPORT prototype_isc_seek_blob(ISC_STATUS *,
									isc_blob_handle *,
									short,
									ISC_LONG,
									ISC_LONG *);



typedef ISC_STATUS ISC_EXPORT prototype_isc_service_attach(ISC_STATUS *,
										 unsigned short,
										 const char*,
										 isc_svc_handle *,
										 unsigned short,
										 const char*);

typedef ISC_STATUS ISC_EXPORT prototype_isc_service_detach(ISC_STATUS *,
										 isc_svc_handle *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_service_query(ISC_STATUS *,
										isc_svc_handle *,
										isc_resv_handle *,
										unsigned short,
										const char*,
										unsigned short,
										const char*,
										unsigned short,
										char *);

typedef ISC_STATUS ISC_EXPORT prototype_isc_service_start(ISC_STATUS *,
										isc_svc_handle *,
										isc_resv_handle *,
										unsigned short,
										const char*);

typedef ISC_STATUS API_ROUTINE prototype_fb_cancel_operation(ISC_STATUS *,
											isc_db_handle *,
											USHORT);

struct FirebirdApiPointers
{
	prototype_isc_attach_database *isc_attach_database;
	prototype_isc_array_gen_sdl *isc_array_gen_sdl;
	prototype_isc_array_get_slice *isc_array_get_slice;
	prototype_isc_array_lookup_bounds *isc_array_lookup_bounds;
	prototype_isc_array_lookup_desc *isc_array_lookup_desc;
	prototype_isc_array_set_desc *isc_array_set_desc;
	prototype_isc_array_put_slice *isc_array_put_slice;
	prototype_isc_blob_default_desc *isc_blob_default_desc;
	prototype_isc_blob_gen_bpb *isc_blob_gen_bpb;
	prototype_isc_blob_info *isc_blob_info;
	prototype_isc_blob_lookup_desc *isc_blob_lookup_desc;
	prototype_isc_blob_set_desc *isc_blob_set_desc;
	prototype_isc_cancel_blob *isc_cancel_blob;
	prototype_isc_cancel_events *isc_cancel_events;
	prototype_isc_close_blob *isc_close_blob;
	prototype_isc_commit_retaining *isc_commit_retaining;
	prototype_isc_commit_transaction *isc_commit_transaction;
	prototype_isc_create_blob *isc_create_blob;
	prototype_isc_create_blob2 *isc_create_blob2;
	prototype_isc_create_database *isc_create_database;
	prototype_isc_database_info *isc_database_info;
	prototype_isc_decode_date *isc_decode_date;
	prototype_isc_decode_sql_date *isc_decode_sql_date;
	prototype_isc_decode_sql_time *isc_decode_sql_time;
	prototype_isc_decode_timestamp *isc_decode_timestamp;
	prototype_isc_detach_database *isc_detach_database;
	prototype_isc_drop_database *isc_drop_database;
	prototype_isc_dsql_allocate_statement *isc_dsql_allocate_statement;
	prototype_isc_dsql_alloc_statement2 *isc_dsql_alloc_statement2;
	prototype_isc_dsql_describe *isc_dsql_describe;
	prototype_isc_dsql_describe_bind *isc_dsql_describe_bind;
	prototype_isc_dsql_exec_immed2 *isc_dsql_exec_immed2;
	prototype_isc_dsql_execute *isc_dsql_execute;
	prototype_isc_dsql_execute2 *isc_dsql_execute2;
	prototype_isc_dsql_execute_immediate *isc_dsql_execute_immediate;
	prototype_isc_dsql_fetch *isc_dsql_fetch;
	prototype_isc_dsql_finish *isc_dsql_finish;
	prototype_isc_dsql_free_statement *isc_dsql_free_statement;
	prototype_isc_dsql_insert *isc_dsql_insert;
	prototype_isc_dsql_prepare *isc_dsql_prepare;
	prototype_isc_dsql_set_cursor_name *isc_dsql_set_cursor_name;
	prototype_isc_dsql_sql_info *isc_dsql_sql_info;
	prototype_isc_encode_date *isc_encode_date;
	prototype_isc_encode_sql_date *isc_encode_sql_date;
	prototype_isc_encode_sql_time *isc_encode_sql_time;
	prototype_isc_encode_timestamp *isc_encode_timestamp;
	prototype_isc_event_block *isc_event_block;
	prototype_isc_event_counts *isc_event_counts;
	prototype_isc_expand_dpb *isc_expand_dpb;
	prototype_isc_modify_dpb *isc_modify_dpb;
	prototype_isc_free *isc_free;
	prototype_isc_get_segment *isc_get_segment;
	prototype_isc_get_slice *isc_get_slice;
	prototype_isc_interprete *isc_interprete;
	prototype_isc_open_blob *isc_open_blob;
	prototype_isc_open_blob2 *isc_open_blob2;
	prototype_isc_prepare_transaction2 *isc_prepare_transaction2;
	prototype_isc_print_sqlerror *isc_print_sqlerror;
	prototype_isc_print_status *isc_print_status;
	prototype_isc_put_segment *isc_put_segment;
	prototype_isc_put_slice *isc_put_slice;
	prototype_isc_que_events *isc_que_events;
	prototype_isc_rollback_retaining *isc_rollback_retaining;
	prototype_isc_rollback_transaction *isc_rollback_transaction;
	prototype_isc_start_multiple *isc_start_multiple;
	prototype_isc_start_transaction *isc_start_transaction;
	prototype_isc_reconnect_transaction* isc_reconnect_transaction;
	prototype_isc_sqlcode *isc_sqlcode;
	prototype_isc_sql_interprete *isc_sql_interprete;
	prototype_isc_transaction_info *isc_transaction_info;
	prototype_isc_transact_request *isc_transact_request;
	prototype_isc_vax_integer *isc_vax_integer;
	prototype_isc_seek_blob *isc_seek_blob;
	prototype_isc_service_attach *isc_service_attach;
	prototype_isc_service_detach *isc_service_detach;
	prototype_isc_service_query *isc_service_query;
	prototype_isc_service_start *isc_service_start;
	prototype_fb_cancel_operation *fb_cancel_operation;
};

#endif
