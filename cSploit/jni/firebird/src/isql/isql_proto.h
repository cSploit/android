/*
 *	PROGRAM:	Interactive SQL utility
 *	MODULE:		isql_proto.h
 *	DESCRIPTION:	Prototype header file for isql.epp
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

#ifndef ISQL_ISQL_PROTO_H
#define ISQL_ISQL_PROTO_H

#include <firebird/Provider.h>

struct IsqlVar;

void	ISQL_array_dimensions(const TEXT*);
//void	ISQL_build_table_list(void**, FILE*, FILE*, FILE*);
//void	ISQL_build_view_list(void**, FILE*, FILE*, FILE*);
//int	ISQL_commit_work(int, FILE*, FILE*, FILE*);
bool	ISQL_dbcheck();
void	ISQL_disconnect_database(bool);
bool	ISQL_errmsg(Firebird::IStatus*);
void	ISQL_warning(ISC_STATUS*);
void	ISQL_exit_db();
// CVC: Not found.
//int		ISQL_extract(TEXT*, int, FILE*, FILE*, FILE*);
int		ISQL_frontend_command(TEXT*, FILE*, FILE*, FILE*);
bool	ISQL_get_base_column_null_flag(const TEXT*, const SSHORT, const TEXT*);
void	ISQL_get_character_sets(SSHORT, SSHORT, bool, bool, TEXT*);
SSHORT	ISQL_get_default_char_set_id();
void	ISQL_get_default_source(const TEXT*, TEXT*, ISC_QUAD*);
SSHORT	ISQL_get_field_length(const TEXT*);
SLONG	ISQL_get_index_segments(TEXT*, const size_t, const TEXT*, bool);
bool	ISQL_get_null_flag(const TEXT*, TEXT*);
void	ISQL_get_version(bool);
SSHORT	ISQL_init(FILE*, FILE*);
#ifdef NOT_USED_OR_REPLACED
bool	ISQL_is_domain(const TEXT*);
#endif
int		ISQL_main(int, char**);
bool	ISQL_printNumericType(const char* fieldName, const int fieldType, const int fieldScale);
void	ISQL_print_validation(FILE*, ISC_QUAD*, bool, Firebird::ITransaction*);
//void	ISQL_query_database(SSHORT*, FILE*, FILE*, FILE*);
//void	ISQL_reset_settings();
void	ISQL_ri_action_print(const TEXT*, const TEXT*, bool);
//int	ISQL_sql_statement(TEXT*, FILE*, FILE*, FILE*);
//void	ISQL_win_err(const char*);
processing_state ISQL_print_item_blob(FILE*, const IsqlVar*, Firebird::ITransaction*, int subtype);
processing_state ISQL_fill_var(IsqlVar*, Firebird::IMessageMetadata*, unsigned, UCHAR*);

#endif // ISQL_ISQL_PROTO_H
