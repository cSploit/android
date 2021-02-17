 extern void* get_tty_password;
 extern void* handle_options;
 extern void* load_defaults;
 extern void* mysql_thread_end;
 extern void* mysql_thread_init;
 extern void* myodbc_remove_escape;
 extern void* mysql_affected_rows;
 extern void* mysql_autocommit;
 extern void* mysql_stmt_bind_param;
 extern void* mysql_stmt_bind_result;
 extern void* mysql_change_user;
 extern void* mysql_character_set_name;
 extern void* mysql_close;
 extern void* mysql_commit;
 extern void* mysql_data_seek;
 extern void* mysql_debug;
 extern void* mysql_dump_debug_info;
 extern void* mysql_eof;
 extern void* mysql_errno;
 extern void* mysql_error;
 extern void* mysql_escape_string;
 extern void* mysql_hex_string;
 extern void* mysql_stmt_execute;
 extern void* mysql_stmt_fetch;
 extern void* mysql_stmt_fetch_column;
 extern void* mysql_fetch_field;
 extern void* mysql_fetch_field_direct;
 extern void* mysql_fetch_fields;
 extern void* mysql_fetch_lengths;
 extern void* mysql_fetch_row;
 extern void* mysql_field_count;
 extern void* mysql_field_seek;
 extern void* mysql_field_tell;
 extern void* mysql_free_result;
 extern void* mysql_get_client_info;
 extern void* mysql_get_host_info;
 extern void* mysql_get_proto_info;
 extern void* mysql_get_server_info;
 extern void* mysql_get_client_version;
 extern void* mysql_get_ssl_cipher;
 extern void* mysql_info;
 extern void* mysql_init;
 extern void* mysql_insert_id;
 extern void* mysql_kill;
 extern void* mysql_set_server_option;
 extern void* mysql_list_dbs;
 extern void* mysql_list_fields;
 extern void* mysql_list_processes;
 extern void* mysql_list_tables;
 extern void* mysql_more_results;
 extern void* mysql_next_result;
 extern void* mysql_num_fields;
 extern void* mysql_num_rows;
 extern void* mysql_options;
 extern void* mysql_stmt_param_count;
 extern void* mysql_stmt_param_metadata;
 extern void* mysql_ping;
 extern void* mysql_stmt_result_metadata;
 extern void* mysql_query;
 extern void* mysql_read_query_result;
 extern void* mysql_real_connect;
 extern void* mysql_real_escape_string;
 extern void* mysql_real_query;
 extern void* mysql_refresh;
 extern void* mysql_rollback;
 extern void* mysql_row_seek;
 extern void* mysql_row_tell;
 extern void* mysql_select_db;
 extern void* mysql_stmt_send_long_data;
 extern void* mysql_send_query;
 extern void* mysql_shutdown;
 extern void* mysql_ssl_set;
 extern void* mysql_stat;
 extern void* mysql_stmt_affected_rows;
 extern void* mysql_stmt_close;
 extern void* mysql_stmt_reset;
 extern void* mysql_stmt_data_seek;
 extern void* mysql_stmt_errno;
 extern void* mysql_stmt_error;
 extern void* mysql_stmt_free_result;
 extern void* mysql_stmt_num_rows;
 extern void* mysql_stmt_row_seek;
 extern void* mysql_stmt_row_tell;
 extern void* mysql_stmt_store_result;
 extern void* mysql_store_result;
 extern void* mysql_thread_id;
 extern void* mysql_thread_safe;
 extern void* mysql_use_result;
 extern void* mysql_warning_count;
 extern void* mysql_stmt_sqlstate;
 extern void* mysql_sqlstate;
 extern void* mysql_get_server_version;
 extern void* mysql_stmt_prepare;
 extern void* mysql_stmt_init;
 extern void* mysql_stmt_insert_id;
 extern void* mysql_stmt_attr_get;
 extern void* mysql_stmt_attr_set;
 extern void* mysql_stmt_field_count;
 extern void* mysql_set_local_infile_default;
 extern void* mysql_set_local_infile_handler;
 extern void* mysql_embedded;
 extern void* mysql_server_init;
 extern void* mysql_server_end;
 extern void* mysql_set_character_set;
 extern void* mysql_get_character_set_info;
 extern void* mysql_stmt_next_result;
 extern void* my_init;
 extern void* mysql_client_find_plugin;
 extern void* mysql_client_register_plugin;
 extern void* mysql_load_plugin;
 extern void* mysql_load_plugin_v;
 extern void* mysql_options4;
 extern void* mysql_plugin_options;
 extern void* mysql_reset_connection;
 extern void* mysql_get_option;
 void *libmysql_api_funcs[] = {
 &get_tty_password,
 &handle_options,
 &load_defaults,
 &mysql_thread_end,
 &mysql_thread_init,
 &myodbc_remove_escape,
 &mysql_affected_rows,
 &mysql_autocommit,
 &mysql_stmt_bind_param,
 &mysql_stmt_bind_result,
 &mysql_change_user,
 &mysql_character_set_name,
 &mysql_close,
 &mysql_commit,
 &mysql_data_seek,
 &mysql_debug,
 &mysql_dump_debug_info,
 &mysql_eof,
 &mysql_errno,
 &mysql_error,
 &mysql_escape_string,
 &mysql_hex_string,
 &mysql_stmt_execute,
 &mysql_stmt_fetch,
 &mysql_stmt_fetch_column,
 &mysql_fetch_field,
 &mysql_fetch_field_direct,
 &mysql_fetch_fields,
 &mysql_fetch_lengths,
 &mysql_fetch_row,
 &mysql_field_count,
 &mysql_field_seek,
 &mysql_field_tell,
 &mysql_free_result,
 &mysql_get_client_info,
 &mysql_get_host_info,
 &mysql_get_proto_info,
 &mysql_get_server_info,
 &mysql_get_client_version,
 &mysql_get_ssl_cipher,
 &mysql_info,
 &mysql_init,
 &mysql_insert_id,
 &mysql_kill,
 &mysql_set_server_option,
 &mysql_list_dbs,
 &mysql_list_fields,
 &mysql_list_processes,
 &mysql_list_tables,
 &mysql_more_results,
 &mysql_next_result,
 &mysql_num_fields,
 &mysql_num_rows,
 &mysql_options,
 &mysql_stmt_param_count,
 &mysql_stmt_param_metadata,
 &mysql_ping,
 &mysql_stmt_result_metadata,
 &mysql_query,
 &mysql_read_query_result,
 &mysql_real_connect,
 &mysql_real_escape_string,
 &mysql_real_query,
 &mysql_refresh,
 &mysql_rollback,
 &mysql_row_seek,
 &mysql_row_tell,
 &mysql_select_db,
 &mysql_stmt_send_long_data,
 &mysql_send_query,
 &mysql_shutdown,
 &mysql_ssl_set,
 &mysql_stat,
 &mysql_stmt_affected_rows,
 &mysql_stmt_close,
 &mysql_stmt_reset,
 &mysql_stmt_data_seek,
 &mysql_stmt_errno,
 &mysql_stmt_error,
 &mysql_stmt_free_result,
 &mysql_stmt_num_rows,
 &mysql_stmt_row_seek,
 &mysql_stmt_row_tell,
 &mysql_stmt_store_result,
 &mysql_store_result,
 &mysql_thread_id,
 &mysql_thread_safe,
 &mysql_use_result,
 &mysql_warning_count,
 &mysql_stmt_sqlstate,
 &mysql_sqlstate,
 &mysql_get_server_version,
 &mysql_stmt_prepare,
 &mysql_stmt_init,
 &mysql_stmt_insert_id,
 &mysql_stmt_attr_get,
 &mysql_stmt_attr_set,
 &mysql_stmt_field_count,
 &mysql_set_local_infile_default,
 &mysql_set_local_infile_handler,
 &mysql_embedded,
 &mysql_server_init,
 &mysql_server_end,
 &mysql_set_character_set,
 &mysql_get_character_set_info,
 &mysql_stmt_next_result,
 &my_init,
 &mysql_client_find_plugin,
 &mysql_client_register_plugin,
 &mysql_load_plugin,
 &mysql_load_plugin_v,
 &mysql_options4,
 &mysql_plugin_options,
 &mysql_reset_connection,
 &mysql_get_option,
 (void *)0
};

