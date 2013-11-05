/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		types.h
 *	DESCRIPTION:	System type definitions
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
 * 2001.10.03 Claudio Valderrama: add int64, user_group, role, generator,
 *   udf, blob_filter and FB's new system flags for special triggers.
 * Adriano dos Santos Fernandes
 *
 */

TYPE("TEXT", blr_text, nam_f_type)
TYPE("SHORT", blr_short, nam_f_type)
TYPE("LONG", blr_long, nam_f_type)
TYPE("QUAD", blr_quad, nam_f_type)
TYPE("FLOAT", blr_float, nam_f_type)
TYPE("DOUBLE", blr_double, nam_f_type)
TYPE("TIMESTAMP", blr_timestamp, nam_f_type)
TYPE("VARYING", blr_varying, nam_f_type)
TYPE("BLOB", blr_blob, nam_f_type)
TYPE("CSTRING", blr_cstring, nam_f_type)
TYPE("BLOB_ID", blr_blob_id, nam_f_type)
TYPE("DATE", blr_sql_date, nam_f_type)
TYPE("TIME", blr_sql_time, nam_f_type)
TYPE ("INT64", blr_int64, nam_f_type)

TYPE("BINARY", 0, nam_f_sub_type)
TYPE("TEXT", 1, nam_f_sub_type)
TYPE("BLR", 2, nam_f_sub_type)
TYPE("ACL", 3, nam_f_sub_type)
TYPE("RANGES", 4, nam_f_sub_type)
TYPE("SUMMARY", 5, nam_f_sub_type)
TYPE("FORMAT", 6, nam_f_sub_type)
TYPE("TRANSACTION_DESCRIPTION", 7, nam_f_sub_type)
TYPE("EXTERNAL_FILE_DESCRIPTION", 8, nam_f_sub_type)
TYPE("DEBUG_INFORMATION", 9, nam_f_sub_type)

TYPE("VALUE", 0, nam_fun_type)
TYPE("BOOLEAN", 1, nam_fun_type)

TYPE("BY_VALUE", 0, nam_mechanism)
TYPE("BY_REFERENCE", 1, nam_mechanism)
TYPE("BY_VMS_DESCRIPTOR", 2, nam_mechanism)
TYPE("BY_ISC_DESCRIPTOR", 3, nam_mechanism)
TYPE("BY_SCALAR_ARRAY_DESCRIPTOR", 4, nam_mechanism)
TYPE("BY_REFERENCE_WITH_NULL", 5, nam_mechanism)

TYPE("PRE_STORE", 1, nam_trg_type)
TYPE("POST_STORE", 2, nam_trg_type)
TYPE("PRE_MODIFY", 3, nam_trg_type)
TYPE("POST_MODIFY", 4, nam_trg_type)
TYPE("PRE_ERASE", 5, nam_trg_type)
TYPE("POST_ERASE", 6, nam_trg_type)
TYPE("CONNECT", 8192, nam_trg_type)
TYPE("DISCONNECT", 8193, nam_trg_type)
TYPE("TRANSACTION_START", 8194, nam_trg_type)
TYPE("TRANSACTION_COMMIT", 8195, nam_trg_type)
TYPE("TRANSACTION_ROLLBACK", 8196, nam_trg_type)

TYPE ("RELATION", obj_relation, nam_obj_type)
TYPE ("VIEW",  obj_view, nam_obj_type)
TYPE ("TRIGGER", obj_trigger, nam_obj_type)
TYPE ("COMPUTED_FIELD", obj_computed, nam_obj_type)
TYPE ("VALIDATION", obj_validation, nam_obj_type)
TYPE ("PROCEDURE", obj_procedure, nam_obj_type)
TYPE ("EXPRESSION_INDEX", obj_expression_index, nam_obj_type)
TYPE ("EXCEPTION", obj_exception, nam_obj_type)
TYPE ("USER",  obj_user, nam_obj_type)
TYPE ("FIELD",  obj_field, nam_obj_type)
TYPE ("INDEX",  obj_index, nam_obj_type)
// TYPE ("DEPENDENT_COUNT", obj_count, nam_obj_type)
TYPE ("USER_GROUP", obj_user_group, nam_obj_type)
TYPE ("ROLE",  obj_sql_role, nam_obj_type)
TYPE ("GENERATOR", obj_generator, nam_obj_type)
TYPE ("UDF",  obj_udf, nam_obj_type)
TYPE ("BLOB_FILTER", obj_blob_filter, nam_obj_type)
TYPE ("COLLATION", obj_collation, nam_obj_type)

TYPE("LIMBO", 1, nam_trans_state)
TYPE("COMMITTED", 2, nam_trans_state)
TYPE("ROLLED_BACK", 3, nam_trans_state)

TYPE ("USER",  0, nam_sys_flag)
TYPE ("SYSTEM",  1, nam_sys_flag)
TYPE ("QLI",  2, nam_sys_flag)
TYPE ("CHECK_CONSTRAINT", 3, nam_sys_flag)
TYPE ("REFERENTIAL_CONSTRAINT", 4, nam_sys_flag)
TYPE ("VIEW_CHECK", 5, nam_sys_flag)

TYPE ("PERSISTENT", rel_persistent, nam_r_type)
TYPE ("VIEW", rel_view, nam_r_type)
TYPE ("EXTERNAL", rel_external, nam_r_type)
TYPE ("VIRTUAL", rel_virtual, nam_r_type)
TYPE ("GLOBAL_TEMPORARY_PRESERVE", rel_global_temp_preserve, nam_r_type)
TYPE ("GLOBAL_TEMPORARY_DELETE", rel_global_temp_delete, nam_r_type)

TYPE ("LEGACY", prc_legacy, nam_prc_type)
TYPE ("SELECTABLE", prc_selectable, nam_prc_type)
TYPE ("EXECUTABLE", prc_executable, nam_prc_type)

TYPE("NORMAL", prm_mech_normal, nam_prm_mechanism)
TYPE("TYPE OF", prm_mech_type_of, nam_prm_mechanism)

TYPE ("IDLE", mon_state_idle, nam_mon_state)
TYPE ("ACTIVE", mon_state_active, nam_mon_state)
TYPE ("STALLED", mon_state_stalled, nam_mon_state)

TYPE ("ONLINE", shut_mode_online, nam_mon_shut_mode)
TYPE ("MULTI_USER_SHUTDOWN", shut_mode_multi, nam_mon_shut_mode)
TYPE ("SINGLE_USER_SHUTDOWN", shut_mode_single, nam_mon_shut_mode)
TYPE ("FULL_SHUTDOWN", shut_mode_full, nam_mon_shut_mode)

TYPE ("CONSISTENCY", iso_mode_consistency, nam_mon_iso_mode)
TYPE ("CONCURRENCY", iso_mode_concurrency, nam_mon_iso_mode)
TYPE ("READ_COMMITTED_VERSION", iso_mode_rc_version, nam_mon_iso_mode)
TYPE ("READ_COMMITTED_NO_VERSION", iso_mode_rc_no_version, nam_mon_iso_mode)

TYPE ("NORMAL", backup_state_normal, nam_mon_backup_state)
TYPE ("STALLED", backup_state_stalled, nam_mon_backup_state)
TYPE ("MERGE", backup_state_merge, nam_mon_backup_state)

TYPE ("DATABASE", stat_database, nam_mon_stat_group)
TYPE ("ATTACHMENT", stat_attachment, nam_mon_stat_group)
TYPE ("TRANSACTION", stat_transaction, nam_mon_stat_group)
TYPE ("STATEMENT", stat_statement, nam_mon_stat_group)
TYPE ("CALL", stat_call, nam_mon_stat_group)
