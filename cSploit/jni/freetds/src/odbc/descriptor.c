/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2003  Steve Murphree
 * Copyright (C) 2004, 2005  Ziglio Frediano
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

#include <freetds/odbc.h>
#include <freetds/string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static void desc_free_record(struct _drecord *drec);

TDS_DESC *
desc_alloc(SQLHANDLE parent, int desc_type, int alloc_type)
{
	TDS_DESC *desc;

	desc = (TDS_DESC *) calloc(1, sizeof(TDS_DESC));
	if (!desc || tds_mutex_init(&desc->mtx))
		return NULL;

	/* set defualt header values */
	desc->htype = SQL_HANDLE_DESC;
	desc->type = desc_type;
	desc->parent = parent;
	desc->header.sql_desc_alloc_type = alloc_type;
	desc->header.sql_desc_count = 0;
	desc->records = NULL;

	switch (desc_type) {
	case DESC_IRD:
	case DESC_IPD:
		break;
	case DESC_ARD:
	case DESC_APD:
		desc->header.sql_desc_bind_type = SQL_BIND_BY_COLUMN;
		desc->header.sql_desc_array_size = 1;
		break;
	default:
		free(desc);
		return NULL;
	}
	return desc;
}

#define SQL_DESC_STRINGS \
	STR_OP(sql_desc_base_column_name); \
	STR_OP(sql_desc_base_table_name); \
	STR_OP(sql_desc_catalog_name); \
	STR_OP(sql_desc_label); \
	STR_OP(sql_desc_local_type_name); \
	STR_OP(sql_desc_name); \
	STR_OP(sql_desc_schema_name); \
	STR_OP(sql_desc_table_name)

SQLRETURN
desc_alloc_records(TDS_DESC * desc, unsigned count)
{
	struct _drecord *drec, *drecs;
	int i;

	/* shrink records */
	if (desc->header.sql_desc_count >= count) {
		for (i = count; i < desc->header.sql_desc_count; ++i)
			desc_free_record(&desc->records[i]);
		desc->header.sql_desc_count = count;
		return SQL_SUCCESS;
	}

	if (desc->records)
		drecs = (struct _drecord *) realloc(desc->records, sizeof(struct _drecord) * (count + 0));
	else
		drecs = (struct _drecord *) malloc(sizeof(struct _drecord) * (count + 0));
	if (!drecs)
		return SQL_ERROR;
	desc->records = drecs;
	memset(desc->records + desc->header.sql_desc_count, 0, sizeof(struct _drecord) * (count - desc->header.sql_desc_count));

	for (i = desc->header.sql_desc_count; i < count; ++i) {
		drec = &desc->records[i];

#define STR_OP(name) tds_dstr_init(&drec->name)
		SQL_DESC_STRINGS;
#undef STR_OP

		switch (desc->type) {
		case DESC_IRD:
		case DESC_IPD:
			drec->sql_desc_parameter_type = SQL_PARAM_INPUT;
			break;
		case DESC_ARD:
		case DESC_APD:
			drec->sql_desc_concise_type = SQL_C_DEFAULT;
			drec->sql_desc_type = SQL_C_DEFAULT;
			break;
		}
	}
	desc->header.sql_desc_count = count;
	return SQL_SUCCESS;
}

static void
desc_free_record(struct _drecord *drec)
{
#define STR_OP(name) tds_dstr_free(&drec->name)
	SQL_DESC_STRINGS;
#undef STR_OP
}

SQLRETURN
desc_free_records(TDS_DESC * desc)
{
	int i;

	if (desc->records) {
		for (i = 0; i < desc->header.sql_desc_count; i++)
			desc_free_record(&desc->records[i]);
		TDS_ZERO_FREE(desc->records);
	}

	desc->header.sql_desc_count = 0;
	return SQL_SUCCESS;
}

SQLRETURN
desc_copy(TDS_DESC * dest, TDS_DESC * src)
{
	int i;
	TDS_DESC tmp = *dest;

	/* copy header */
	tmp.header = src->header;

	/* set no records */
	tmp.header.sql_desc_count = 0;
	tmp.records = NULL;

	tmp.errs.num_errors = 0;
	tmp.errs.errs = NULL;

	if (desc_alloc_records(&tmp, src->header.sql_desc_count) != SQL_SUCCESS)
		return SQL_ERROR;

	for (i = 0; i < src->header.sql_desc_count; ++i) {
		struct _drecord *src_rec = &src->records[i];
		struct _drecord *dest_rec = &tmp.records[i];

		/* copy all integer in one time ! */
		memcpy(dest_rec, src_rec, sizeof(struct _drecord));

		/* reinitialize string, avoid doubling pointers */
#define STR_OP(name) tds_dstr_init(&dest_rec->name)
		SQL_DESC_STRINGS;
#undef STR_OP

		/* copy strings */
#define STR_OP(name) if (!tds_dstr_dup(&dest_rec->name, &src_rec->name)) goto Cleanup
		SQL_DESC_STRINGS;
#undef STR_OP
	}

	/* success, copy back to our descriptor */
	desc_free_records(dest);
	odbc_errs_reset(&dest->errs);
	*dest = tmp;
	return SQL_SUCCESS;

Cleanup:
	desc_free_records(&tmp);
	odbc_errs_reset(&tmp.errs);
	return SQL_ERROR;
}

SQLRETURN
desc_free(TDS_DESC * desc)
{
	if (desc) {
		desc_free_records(desc);
		odbc_errs_reset(&desc->errs);
		tds_mutex_unlock(&desc->mtx);
		tds_mutex_free(&desc->mtx);
		free(desc);
	}
	return SQL_SUCCESS;
}

TDS_DBC *
desc_get_dbc(TDS_DESC *desc)
{
	if (IS_HSTMT(desc->parent))
		return ((TDS_STMT *) desc->parent)->dbc;

	return (TDS_DBC *) desc->parent;
}

