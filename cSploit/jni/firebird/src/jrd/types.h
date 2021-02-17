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
TYPE("INT64", blr_int64, nam_f_type)
TYPE("BOOLEAN", blr_bool, nam_f_type)

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

TYPE("PRE_STORE", PRE_STORE_TRIGGER, nam_trg_type)
TYPE("POST_STORE", POST_STORE_TRIGGER, nam_trg_type)
TYPE("PRE_MODIFY", PRE_MODIFY_TRIGGER, nam_trg_type)
TYPE("POST_MODIFY", POST_MODIFY_TRIGGER, nam_trg_type)
TYPE("PRE_ERASE", PRE_ERASE_TRIGGER, nam_trg_type)
TYPE("POST_ERASE", POST_ERASE_TRIGGER, nam_trg_type)
TYPE("CONNECT", (TRIGGER_TYPE_DB | DB_TRIGGER_CONNECT), nam_trg_type) // 8192
TYPE("DISCONNECT", (TRIGGER_TYPE_DB | DB_TRIGGER_DISCONNECT), nam_trg_type) // 8193
TYPE("TRANSACTION_START", (TRIGGER_TYPE_DB | DB_TRIGGER_TRANS_START), nam_trg_type) // 8194
TYPE("TRANSACTION_COMMIT", (TRIGGER_TYPE_DB | DB_TRIGGER_TRANS_COMMIT), nam_trg_type) // 8195
TYPE("TRANSACTION_ROLLBACK", (TRIGGER_TYPE_DB | DB_TRIGGER_TRANS_ROLLBACK), nam_trg_type) // 8196

TYPE("RELATION", obj_relation, nam_obj_type)
TYPE("VIEW", obj_view, nam_obj_type)
TYPE("TRIGGER", obj_trigger, nam_obj_type)
TYPE("COMPUTED_FIELD", obj_computed, nam_obj_type)
TYPE("VALIDATION", obj_validation, nam_obj_type)
TYPE("PROCEDURE", obj_procedure, nam_obj_type)
TYPE("EXPRESSION_INDEX", obj_expression_index, nam_obj_type)
TYPE("EXCEPTION", obj_exception, nam_obj_type)
TYPE("USER", obj_user, nam_obj_type)
TYPE("FIELD", obj_field, nam_obj_type)
TYPE("INDEX", obj_index, nam_obj_type)
TYPE("CHARACTER_SET", obj_charset, nam_obj_type)
TYPE("USER_GROUP", obj_user_group, nam_obj_type)
TYPE("ROLE", obj_sql_role, nam_obj_type)
TYPE("GENERATOR", obj_generator, nam_obj_type)
TYPE("UDF", obj_udf, nam_obj_type)
TYPE("BLOB_FILTER", obj_blob_filter, nam_obj_type)
TYPE("COLLATION", obj_collation, nam_obj_type)
TYPE("PACKAGE", obj_package_header, nam_obj_type)
TYPE("PACKAGE BODY", obj_package_body, nam_obj_type)

TYPE("LIMBO", 1, nam_trans_state)
TYPE("COMMITTED", 2, nam_trans_state)
TYPE("ROLLED_BACK", 3, nam_trans_state)

TYPE("USER", fb_sysflag_user, nam_sys_flag)
TYPE("SYSTEM", fb_sysflag_system, nam_sys_flag)
TYPE("QLI", fb_sysflag_qli, nam_sys_flag)
TYPE("CHECK_CONSTRAINT", fb_sysflag_check_constraint, nam_sys_flag)
TYPE("REFERENTIAL_CONSTRAINT", fb_sysflag_referential_constraint, nam_sys_flag)
TYPE("VIEW_CHECK", fb_sysflag_view_check, nam_sys_flag)
TYPE("IDENTITY_GENERATOR", fb_sysflag_identity_generator, nam_sys_flag)

TYPE("PERSISTENT", rel_persistent, nam_r_type)
TYPE("VIEW", rel_view, nam_r_type)
TYPE("EXTERNAL", rel_external, nam_r_type)
TYPE("VIRTUAL", rel_virtual, nam_r_type)
TYPE("GLOBAL_TEMPORARY_PRESERVE", rel_global_temp_preserve, nam_r_type)
TYPE("GLOBAL_TEMPORARY_DELETE", rel_global_temp_delete, nam_r_type)

TYPE("LEGACY", prc_legacy, nam_prc_type)
TYPE("SELECTABLE", prc_selectable, nam_prc_type)
TYPE("EXECUTABLE", prc_executable, nam_prc_type)

TYPE("NORMAL", prm_mech_normal, nam_prm_mechanism)
TYPE("TYPE OF", prm_mech_type_of, nam_prm_mechanism)

TYPE("IDLE", mon_state_idle, nam_mon_state)
TYPE("ACTIVE", mon_state_active, nam_mon_state)
TYPE("STALLED", mon_state_stalled, nam_mon_state)

TYPE("ONLINE", shut_mode_online, nam_mon_shut_mode)
TYPE("MULTI_USER_SHUTDOWN", shut_mode_multi, nam_mon_shut_mode)
TYPE("SINGLE_USER_SHUTDOWN", shut_mode_single, nam_mon_shut_mode)
TYPE("FULL_SHUTDOWN", shut_mode_full, nam_mon_shut_mode)

TYPE("CONSISTENCY", iso_mode_consistency, nam_mon_iso_mode)
TYPE("CONCURRENCY", iso_mode_concurrency, nam_mon_iso_mode)
TYPE("READ_COMMITTED_VERSION", iso_mode_rc_version, nam_mon_iso_mode)
TYPE("READ_COMMITTED_NO_VERSION", iso_mode_rc_no_version, nam_mon_iso_mode)

TYPE("NORMAL", backup_state_normal, nam_mon_backup_state)
TYPE("STALLED", backup_state_stalled, nam_mon_backup_state)
TYPE("MERGE", backup_state_merge, nam_mon_backup_state)

TYPE("DATABASE", stat_database, nam_mon_stat_group)
TYPE("ATTACHMENT", stat_attachment, nam_mon_stat_group)
TYPE("TRANSACTION", stat_transaction, nam_mon_stat_group)
TYPE("STATEMENT", stat_statement, nam_mon_stat_group)
TYPE("CALL", stat_call, nam_mon_stat_group)

TYPE("ALWAYS", IDENT_TYPE_ALWAYS, nam_identity_type)
TYPE("BY DEFAULT", IDENT_TYPE_BY_DEFAULT, nam_identity_type)

TYPE("INPUT", 0, nam_prm_type)
TYPE("OUTPUT", 1, nam_prm_type)

TYPE("ACTIVE", 0, nam_trg_inactive)
TYPE("INACTIVE", 1, nam_trg_inactive)

TYPE("ACTIVE", 0, nam_i_inactive)
TYPE("INACTIVE", 1, nam_i_inactive)

TYPE("NON_UNIQUE", 0, nam_un_flag)
TYPE("UNIQUE", 1, nam_un_flag)

TYPE("NONE", 0, nam_grant)
TYPE("GRANT_OPTION", 1, nam_grant)
TYPE("ADMIN_OPTION", 2, nam_grant)

TYPE("HEADER", pag_header, nam_p_type)
TYPE("PAGE_INVENTORY", pag_pages, nam_p_type)
TYPE("TRANSACTION_INVENTORY", pag_transactions, nam_p_type)
TYPE("POINTER", pag_pointer, nam_p_type)
TYPE("DATA", pag_data, nam_p_type)
TYPE("INDEX_ROOT", pag_root, nam_p_type)
TYPE("INDEX_BUCKET", pag_index, nam_p_type)
TYPE("BLOB", pag_blob, nam_p_type)
TYPE("GENERATOR", pag_ids, nam_p_type)
TYPE("SCN_INVENTORY", pag_scns, nam_p_type)

TYPE("PUBLIC", 0, nam_private_flag)
TYPE("PRIVATE", 1, nam_private_flag)

TYPE("NEW_STYLE", 0, nam_legacy_flag)
TYPE("LEGACY_STYLE", 1, nam_legacy_flag)

TYPE("NON_DETERMINISTIC", 0, nam_deterministic_flag)
TYPE("DETERMINISTIC", 1, nam_deterministic_flag)
