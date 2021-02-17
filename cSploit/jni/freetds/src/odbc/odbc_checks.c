/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2003-2008  Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <assert.h>

#include <freetds/odbc.h>
#include <freetds/string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: odbc_checks.c,v 1.22 2011-05-16 08:51:40 freddy77 Exp $");

#if ENABLE_EXTRA_CHECKS

void
odbc_check_env_extra(TDS_ENV * env)
{
	assert(env && env->htype == SQL_HANDLE_ENV);
	assert(env->attr.odbc_version == SQL_OV_ODBC3 || env->attr.odbc_version == SQL_OV_ODBC2);
}

void
odbc_check_dbc_extra(TDS_DBC * dbc)
{
	assert(dbc && dbc->htype == SQL_HANDLE_DBC);
}

void
odbc_check_stmt_extra(TDS_STMT * stmt)
{
	assert(stmt && stmt->htype == SQL_HANDLE_STMT);
	/* TODO deep check on connection */
	assert(stmt->dbc);
	odbc_check_desc_extra(stmt->ard);
	odbc_check_desc_extra(stmt->ird);
	odbc_check_desc_extra(stmt->apd);
	odbc_check_desc_extra(stmt->ipd);
	assert(!stmt->prepared_query_is_func || stmt->prepared_query_is_rpc);
	assert(stmt->param_num <= stmt->param_count + 1);
	assert(stmt->num_param_rows >= 1);
	assert(stmt->curr_param_row >= 0);
	assert(stmt->curr_param_row <= stmt->num_param_rows);
	if (stmt->prepared_query_is_rpc) {
		char *query = stmt->prepared_query ? stmt->prepared_query : stmt->query;
		assert(query);
		assert(stmt->prepared_pos == NULL || (stmt->prepared_pos >= query && stmt->prepared_pos <= strchr(query,0)));
	} else {
		assert(stmt->prepared_pos == NULL);
	}
	/* TODO assert dbc has this statement in list */
}

static void
odbc_check_drecord(TDS_DESC * desc, struct _drecord *drec)
{
	assert(drec->sql_desc_concise_type != SQL_INTERVAL && drec->sql_desc_concise_type != SQL_DATETIME);

	/* unbinded columns have type == 0 */
	/* TODO test errors on code if type == 0 */
	if (desc->type == DESC_IPD || desc->type == DESC_IRD) {
		assert((drec->sql_desc_type == 0 && drec->sql_desc_concise_type == 0) || odbc_get_concise_sql_type(drec->sql_desc_type, drec->sql_desc_datetime_interval_code) == drec->sql_desc_concise_type);
	} else {
		assert((drec->sql_desc_type == 0 && drec->sql_desc_concise_type == 0) || odbc_get_concise_c_type(drec->sql_desc_type, drec->sql_desc_datetime_interval_code) == drec->sql_desc_concise_type);
	}
}

void
odbc_check_desc_extra(TDS_DESC * desc)
{
	int i;

	assert(desc && desc->htype == SQL_HANDLE_DESC);
	assert(desc->header.sql_desc_alloc_type == SQL_DESC_ALLOC_AUTO || desc->header.sql_desc_alloc_type == SQL_DESC_ALLOC_USER);
	assert((desc->type != DESC_IPD && desc->type != DESC_IRD) || desc->header.sql_desc_alloc_type == SQL_DESC_ALLOC_AUTO);
	for (i = 0; i < desc->header.sql_desc_count; ++i) {
		odbc_check_drecord(desc, &desc->records[i]);
	}
}

void
odbc_check_struct_extra(void *p)
{
	const int invalid_htype = 0;

	switch (((TDS_CHK *) p)->htype) {
	case SQL_HANDLE_ENV:
		odbc_check_env_extra((TDS_ENV *) p);
		break;
	case SQL_HANDLE_DBC:
		odbc_check_dbc_extra((TDS_DBC *) p);
		break;
	case SQL_HANDLE_STMT:
		odbc_check_stmt_extra((TDS_STMT *) p);
		break;
	case SQL_HANDLE_DESC:
		odbc_check_desc_extra((TDS_DESC *) p);
		break;
	default:
		assert(invalid_htype);
	}
}

#endif /* ENABLE_EXTRA_CHECKS */
