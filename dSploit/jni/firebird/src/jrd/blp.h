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
 * 2001.07.28: John Bellardo: Added skip and current_role to table.
 * Adriano dos Santos Fernandes
 */
static const struct
{
	const char* blr_string;
	const UCHAR* blr_operators;
} blr_table[] =
{
	{NULL, NULL},
	{"assignment", two},
	{"begin", begin},
	{"declare", declare},
	{"message", message},
	{"erase", byte_line},
	{"fetch", two},
	{"for", two},
	{"if", three},
	{"loop", one},
	{"modify", byte_byte_verb},	// 10
 	{"handler", one},
	{"receive", byte_verb},
	{"select", begin},
	{"send", byte_verb},
	{"store", two},
	{NULL, NULL},
	{"label", byte_verb},
	{"leave", byte_line},
	{"store2", three},
	{"post", one},	// 20
	{"literal", literal},
	{"dbkey", byte_line},
	{"field", field},
	{"fid", parm},
	{"parameter", parm},
	{"variable", variable},
	{"average", two},
	{"count", one},
	{"maximum", two},
	{"minimum", two},	// 30
	{"total", two},
	{NULL, NULL}, // {"count2", two},
	{NULL, NULL},
	{"add", two},
	{"subtract", two},
	{"multiply", two},
	{"divide", two},
	{"negate", one},
	{"concatenate", two},
	{"substring", three},	// 40
	{"parameter2", parm2},
	{"from", two},
	{"via", three},
	{"user_name", zero},
	{"null", zero},
	{"equiv", two},
	{"eql", two},
	{"neq", two},
	{"gtr", two},
	{"geq", two},	// 50
	{"lss", two},
	{"leq", two},
	{"containing", two},
	{"matching", two},
	{"starting", two},
	{"between", three},
	{"or", two},
	{"and", two},
	{"not", one},
	{"any", one},	// 60
	{"missing", one},
	{"unique", one},
	{"like", two},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{"rse", rse},
	{"first", one},
	{"project", byte_args},
	{"sort", byte_args},	// 70
	{"boolean", one},
	{"ascending", one},
	{"descending", one},
	{"relation", relation},
	{"rid", rid},
	{"union", union_ops},
	{"map", map},
	{"group_by", byte_args},
	{"aggregate", aggregate},
	{"join_type", join},	// 80
	{NULL, NULL},
	{NULL, NULL},
	{"agg_count", zero},
	{"agg_max", one},
	{"agg_min", one},
	{"agg_total", one},
	{"agg_average", one},
	{"parameter3", parm3},
	{NULL, NULL},
	{NULL, NULL},	// 90
	{NULL, NULL},
	{NULL, NULL},
	{"agg_count2", one},
	{"agg_count_distinct", one},
	{"agg_total_distinct", one},
	{"agg_average_distinct", one},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{"function", function},	// 100
	{"gen_id", gen_id},
	{"prot_mask", two},
	{"upcase", one},
	{"lock_state", one},
	{"value_if", three},
	{"matching2", three},
	{"index", indx},
	{"ansi_like", three},
	{NULL, NULL},
	{NULL, NULL},	// 110
	{NULL, NULL},
	{"seek", seek},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{"rs_stream", rse},
	{"exec_proc", exec_proc},	// 120
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{"procedure", procedure},
	{"pid", pid},
	{NULL, NULL},
	{"singular", one},
	{"abort", set_error},
	{"block", begin},
	{"error_handler", error_handler},	// 130
	{"cast", cast},
	{NULL, NULL},
	{NULL, NULL},
	{"start_savepoint", zero},
	{"end_savepoint", zero},
	{NULL, NULL},
	{NULL, NULL},
	{NULL, NULL},
	{"plan", one},
	{"merge", byte_args},	// 140
	{"join", byte_args},
	{"sequential", zero},
	{"navigational", byte_literal},
	{"indices", indices},
	{"retrieve", two},
	{"relation2", relation2},
	{"rid2", rid2},
	{NULL, NULL},
	{NULL, NULL},
	{"set_generator", gen_id},	// 150
	{"ansi_any", one},
	{"exists", one},
	{NULL, NULL},
	{"record_version", byte_line},
	{"stall", zero},
	{NULL, NULL},
	{NULL, NULL},
	{"ansi_all", one},
	// New BLR in 6.0
	{"extract", extract},
	{"current_date", zero},	// 160
	{"current_timestamp", zero},
	{"current_time", zero},
	{"post_arg", two},
	{"exec_into", exec_into},
	{"user_savepoint", user_savepoint},
	{"dcl_cursor", dcl_cursor},
	{"cursor_stmt", cursor_stmt},
	{"current_timestamp2", byte_line},
	{"current_time2", byte_line},
	{"agg_list", two}, // 170
	{"agg_list_distinct", two},
	/***
	// These verbs were added in 6.0, primarily to support 64-bit integers, now obsolete
	{"gen_id2", gen_id},
	{"set_generator2", gen_id},
	***/
	{"modify2", modify2},
	{NULL, NULL},
	// New BLR in FB1
	{"current_role", zero},
	{"skip", one},
	// New BLR in FB2
	{"exec_sql", one},
	{"internal_info", one},
	{"nullsfirst", zero},
	{"writelock", zero},
	{"nullslast", zero}, // 180
	{"lowcase", one},
	{"strlen", strlength},
	{"trim", trim},
	// New BLR in FB2.1
	{"init_variable", variable},
	{"recurse", union_ops},
	{"sys_function", function},
	// New BLR in FB2.5
	{"auto_trans", byte_verb},
	{"similar", similar},
	{"exec_stmt", exec_stmt},
	{"stmt_expr", two},
	{"derived_expr", derived_expr},
	{0, 0}
};

