/*
 *	PROGRAM:	Interactive Query Language Interpreter
 *	MODULE:		reqs.h
 *	DESCRIPTION:	Internal request number definitions.
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
 */

#ifndef QLI_REQS_H
#define QLI_REQS_H

const int REQ_relation_def		= 0;
const int REQ_store_relation	= 1;
const int REQ_relation_id		= 2;
const int REQ_rfr_def			= 3;
const int REQ_field_def			= 4;
const int REQ_store_field		= 5;
const int REQ_store_rfr			= 6;
const int REQ_modify_fld		= 7;
const int REQ_update_fld		= 8;
const int REQ_modify_rel		= 9;
const int REQ_show_indices		= 10;
const int REQ_show_index		= 11;
const int REQ_check_fld			= 12;
const int REQ_erase_fld			= 13;
const int REQ_erase_index		= 14;
const int REQ_erase_segments	= 15;
//const int REQ_erase_relation	= 16;
//const int REQ_erase_rfr			= 17;
//const int REQ_erase_view		= 18;
const int REQ_show_files		= 19;
const int REQ_show_view			= 20;
const int REQ_show_field		= 21;
const int REQ_show_view_field	= 22;
const int REQ_show_dbb			= 23;
const int REQ_show_rel_detail	= 24;
const int REQ_show_global_field	= 25;
const int REQ_show_field_instance	= 26;
const int REQ_show_global_fields	= 27;
//const int REQ_show_trigger		= 28;
//const int REQ_show_triggers		= 29;
const int REQ_show_rel_secur	= 30;
const int REQ_show_rel_extern	= 31;
const int REQ_def_index1		= 32;
const int REQ_def_index2		= 33;
const int REQ_def_index3		= 34;
const int REQ_mdf_index			= 35;
const int REQ_mdf_rfr			= 36;
//const int REQ_show_forms1		= 37;
//const int REQ_show_forms2		= 38;
// const int REQ_trig_exists		= 39;	OBSOLETE
const int REQ_show_views		= 40;
const int REQ_show_view_rel		= 41;
const int REQ_show_secur_class	= 42;
const int REQ_show_secur		= 43;
const int REQ_scan_index		= 44;
const int REQ_show_new_trigger	= 45;
const int REQ_show_new_triggers	= 46;
const int REQ_show_sys_triggers	= 47;
const int REQ_new_trig_exists	= 48;
//const int REQ_sql_grant			= 49;
//const int REQ_sql_revoke		= 51;
//const int REQ_sql_cr_view		= 52;
//const int REQ_sql_alt_table		= 53;
//const int REQ_fld_positions		= 54;
const int REQ_fld_subtype		= 55;
const int REQ_show_functions	= 56;
const int REQ_show_func_detail	= 57;
const int REQ_show_func_args	= 58;
const int REQ_fld_dimensions	= 59;
const int REQ_show_filters		= 60;
const int REQ_show_filter_detail	= 61;
const int REQ_show_index_type	= 62;
const int REQ_show_trig_message	= 63;
const int REQ_show_dimensions	= 64;
const int REQ_max				= 65;

#endif // QLI_REQS_H

