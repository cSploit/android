/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2004-2011  Frediano Ziglio
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

#undef NDEBUG

#include <stdarg.h>
#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <assert.h>

#include <freetds/tds.h>
#include <freetds/convert.h>
#include <freetds/string.h>
#include "tds_checks.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: tds_checks.c,v 1.35 2011-10-30 17:00:35 freddy77 Exp $");

#if ENABLE_EXTRA_CHECKS

void
tds_check_tds_extra(const TDSSOCKET * tds)
{
	const int invalid_state = 0;
	int i;
	TDSDYNAMIC *cur_dyn = NULL;
	TDSCURSOR *cur_cursor = NULL;

	assert(tds);

	/* test state and connection */
	switch (tds->state) {
	case TDS_DEAD:
	case TDS_QUERYING:
	case TDS_PENDING:
	case TDS_IDLE:
	case TDS_READING:
		break;
	default:
		assert(invalid_state);
	}

	assert(tds->conn);

#if ENABLE_ODBC_MARS
	if (tds->state != TDS_DEAD)
		assert(!TDS_IS_SOCKET_INVALID(tds_get_s(tds)));
#else
	assert(tds->state == TDS_DEAD || !TDS_IS_SOCKET_INVALID(tds_get_s(tds)));
	assert(tds->state != TDS_DEAD || TDS_IS_SOCKET_INVALID(tds_get_s(tds)));
#endif

	/* test env */
	tds_check_env_extra(&tds_conn(tds)->env);

	/* test buffers and positions */
	assert(tds->in_pos <= tds->in_len);
	assert(tds->in_len <= tds->in_buf_max);
	/* TODO remove blocksize from env and use out_len ?? */
/*	assert(tds->out_pos <= tds->out_len); */
/* 	assert(tds->out_len == 0 || tds->out_buf != NULL); */
	assert(tds->out_pos <= tds->out_buf_max);
	assert(tds->out_buf_max == 0 || tds->out_buf != NULL);
	assert(tds->in_buf_max == 0 || tds->in_buf != NULL);

	/* test res_info */
	if (tds->res_info)
		tds_check_resultinfo_extra(tds->res_info);

	/* test num_comp_info, comp_info */
	assert(tds->num_comp_info >= 0);
	for (i = 0; i < tds->num_comp_info; ++i) {
		assert(tds->comp_info);
		tds_check_resultinfo_extra(tds->comp_info[i]);
	}

	/* param_info */
	if (tds->param_info)
		tds_check_resultinfo_extra(tds->param_info);

	/* test cursors */
	for (cur_cursor = tds->conn->cursors; cur_cursor != NULL; cur_cursor = cur_cursor->next)
		tds_check_cursor_extra(cur_cursor);

	/* test dynamics */
	for (cur_dyn = tds->conn->dyns; cur_dyn != NULL; cur_dyn = cur_dyn->next)
		tds_check_dynamic_extra(cur_dyn);

	/* test tds_ctx */
	tds_check_context_extra(tds_get_ctx(tds));

	/* TODO test char_conv_count, char_convs */

	/* we can't have compute and no results */
	assert(tds->num_comp_info == 0 || tds->res_info != NULL);

	/* we can't have normal and parameters results */
	/* TODO too strict ?? */
/*	assert(tds->param_info == NULL || tds->res_info == NULL); */
}

void
tds_check_context_extra(const TDSCONTEXT * ctx)
{
	assert(ctx);
}

void
tds_check_env_extra(const TDSENV * env)
{
	assert(env);

	assert(env->block_size >= 0 && env->block_size <= 65536);
}

void
tds_check_column_extra(const TDSCOLUMN * column)
{
	int size;
	TDSCONNECTION conn;
	int varint_ok;
	int column_varint_size;

	assert(column);
	column_varint_size = column->column_varint_size;

	/* 8 is for varchar(max) or similar */
	assert(column_varint_size == 8 || (column_varint_size <= 5 && column_varint_size != 3));

	assert(column->column_scale <= column->column_prec);
	assert(column->column_prec <= 77);

	/* I don't like this that much... freddy77 */
	if (column->column_type == 0)
		return;
	assert(column->funcs);
	assert(column->column_type > 0);

	assert(strlen(column->column_name) < sizeof(column->column_name));

	/* check type and server type same or SQLNCHAR -> SQLCHAR */
#define SPECIAL(ttype, server_type, varint) \
	if (column->column_type == ttype && column->on_server.column_type == server_type && column_varint_size == varint) {} else
	SPECIAL(SYBTEXT, XSYBVARCHAR, 8)
	SPECIAL(SYBTEXT, XSYBNVARCHAR, 8)
	SPECIAL(SYBIMAGE, XSYBVARBINARY, 8)
	assert(tds_get_cardinal_type(column->on_server.column_type, column->column_usertype) == column->column_type
		|| (tds_get_conversion_type(column->on_server.column_type, column->column_size) == column->column_type
		&& column_varint_size == 1 && is_fixed_type(column->column_type)));

	varint_ok = 0;
	if (column_varint_size == 8) {
		assert(column->on_server.column_type == XSYBVARCHAR || column->on_server.column_type == XSYBVARBINARY || column->on_server.column_type == XSYBNVARCHAR || column->on_server.column_type == SYBMSXML || column->on_server.column_type == SYBMSUDT);
		varint_ok = 1;
	} else if (is_blob_type(column->column_type)) {
		assert(column_varint_size >= 4);
	} else if (column->column_type == SYBVARIANT) {
		assert(column_varint_size == 4);
	}
	conn.tds_version = 0x500;
	varint_ok = varint_ok || tds_get_varint_size(&conn, column->on_server.column_type) == column_varint_size;
	conn.tds_version = 0x700;
	varint_ok = varint_ok || tds_get_varint_size(&conn, column->on_server.column_type) == column_varint_size;
	assert(varint_ok);

	/* check current size <= size */
	if (is_numeric_type(column->column_type)) {
		/* I don't like that much this difference between numeric and not numeric - freddy77 */
		/* TODO what should be the size ?? */
		assert(column->column_prec >= 1 && column->column_prec <= 77);
		assert(column->column_scale <= column->column_prec);
/*		assert(column->column_cur_size == tds_numeric_bytes_per_prec[column->column_prec] + 2 || column->column_cur_size == -1); */
	} else {
		assert(column->column_cur_size <= column->column_size);
	}

	/* check size of fixed type correct */
	size = tds_get_size_by_type(column->column_type);
	assert(size != 0 || column->column_type == SYBVOID);
	if (size >= 0 && column->column_type != SYBBITN) {
		/* check macro */
		assert(is_fixed_type(column->column_type));
		/* check current size */
		if (column->column_type != SYBMSDATE)
			assert(size == column->column_size);
		/* check cases where server need nullable types */
		if (column->column_type != column->on_server.column_type && (column->column_type != SYBINT8 || column->on_server.column_type != SYB5INT8)) {
			assert(!is_fixed_type(column->on_server.column_type));
			assert(column_varint_size == 1);
			assert(column->column_size == column->column_cur_size || column->column_cur_size == -1);
		} else {
			assert(column_varint_size == 0
				|| (column->column_type == SYBUNIQUE && column_varint_size == 1)
				|| (column->column_type == SYBMSDATE  && column_varint_size == 1));
			assert(column->column_size == column->column_cur_size
				|| (column->column_type == SYBUNIQUE && column->column_cur_size == -1)
				|| (column->column_type == SYBMSDATE  && column->column_cur_size == -1));
		}
		assert(column->column_size == column->on_server.column_size);
	} else {
		assert(!is_fixed_type(column->column_type));
		assert(is_char_type(column->column_type) || (column->on_server.column_size == column->column_size || column->on_server.column_size == 0));
		assert(column_varint_size != 0);
	}

	/* check size of nullable types (ie intN) it's supported */
	if (tds_get_conversion_type(column->column_type, 4) != column->column_type) {
		/* check macro */
		assert(is_nullable_type(column->column_type));
		/* check that size it's correct for this type of nullable */
		assert(tds_get_conversion_type(column->column_type, column->column_size) != column->column_type);
		/* check current size */
		assert(column->column_size >= column->column_cur_size || column->column_cur_size == -1);
		/* check same type and size on server */
		assert(column->column_type == column->on_server.column_type);
		assert(column->column_size == column->on_server.column_size);
	}
}

void
tds_check_resultinfo_extra(const TDSRESULTINFO * res_info)
{
	int i;

	assert(res_info);
	assert(res_info->num_cols >= 0);
	assert(res_info->ref_count > 0);
	for (i = 0; i < res_info->num_cols; ++i) {
		assert(res_info->columns);
		tds_check_column_extra(res_info->columns[i]);
		assert(res_info->columns[i]->column_data != NULL || res_info->row_size == 0);
	}

	assert(res_info->row_size >= 0);

	assert(res_info->computeid >= 0);

	assert(res_info->by_cols >= 0);
	assert(res_info->by_cols == 0 || res_info->bycolumns);
}

void
tds_check_cursor_extra(const TDSCURSOR * cursor)
{
}

void
tds_check_dynamic_extra(const TDSDYNAMIC * dyn)
{
	assert(dyn);

	if (dyn->res_info)
		tds_check_resultinfo_extra(dyn->res_info);
	if (dyn->params)
		tds_check_resultinfo_extra(dyn->params);

	assert(!dyn->emulated || dyn->query);
}

#endif /* ENABLE_EXTRA_CHECKS */
