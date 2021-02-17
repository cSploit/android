/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2008-2010  Frediano Ziglio
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

/**
 * \file
 * \brief Handle bulk copy
 */

#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <assert.h>

#include <freetds/tds.h>
#include "tds_checks.h"
#include <freetds/bytes.h>
#include <freetds/iconv.h>
#include <freetds/stream.h>
#include <freetds/string.h>
#include "replacements.h"
#ifdef DMALLOC
#include <dmalloc.h>
#endif

/** \cond HIDDEN_SYMBOLS */
#ifndef MAX
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#endif
/** \endcond */

/**
 * Holds clause buffer
 */
typedef struct tds_pbcb
{
	/** buffer */
	char *pb;
	/** buffer length */
	unsigned int cb;
	/** true is buffer came from malloc */
	unsigned int from_malloc;
} TDSPBCB;

static TDSRET tds7_bcp_send_colmetadata(TDSSOCKET *tds, TDSBCPINFO *bcpinfo);
static TDSRET tds_bcp_start_insert_stmt(TDSSOCKET *tds, TDSBCPINFO *bcpinfo);
static int tds_bcp_add_fixed_columns(TDSBCPINFO *bcpinfo, tds_bcp_get_col_data get_col_data, tds_bcp_null_error null_error, int offset, unsigned char * rowbuffer, int start);
static int tds_bcp_add_variable_columns(TDSBCPINFO *bcpinfo, tds_bcp_get_col_data get_col_data, tds_bcp_null_error null_error, int offset, TDS_UCHAR *rowbuffer, int start, int *pncols);
static void tds_bcp_row_free(TDSRESULTINFO* result, unsigned char *row);

/**
 * Initialize BCP information.
 * Query structure of the table to server.
 * \tds
 * \param bcpinfo BCP information to initialize. Structure should be allocate
 *        and table name and direction should be already set.
 */
TDSRET
tds_bcp_init(TDSSOCKET *tds, TDSBCPINFO *bcpinfo)
{
	TDSRESULTINFO *resinfo;
	TDSRESULTINFO *bindinfo = NULL;
	TDSCOLUMN *curcol;
	TDS_INT result_type;
	int i;
	TDSRET rc;
	const char *fmt;

	/* FIXME don't leave state in processing state */

	/* TODO quote tablename if needed */
	if (bcpinfo->direction != TDS_BCP_QUERYOUT)
		fmt = "SET FMTONLY ON select * from %s SET FMTONLY OFF";
	else
		fmt = "SET FMTONLY ON %s SET FMTONLY OFF";

	if (TDS_FAILED(rc=tds_submit_queryf(tds, fmt, bcpinfo->tablename)))
		/* TODO return an error ?? */
		/* Attempt to use Bulk Copy with a non-existent Server table (might be why ...) */
		return rc;

	/* TODO possibly stop at ROWFMT and copy before going to idle */
	/* TODO check what happen if table is not present, cleanup on error */
	while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_RESULTS))
		   == TDS_SUCCESS)
		continue;
	if (TDS_FAILED(rc))
		return rc;

	/* copy the results info from the TDS socket */
	if (!tds->res_info)
		return TDS_FAIL;

	resinfo = tds->res_info;
	if ((bindinfo = tds_alloc_results(resinfo->num_cols)) == NULL) {
		rc = TDS_FAIL;
		goto cleanup;
	}

	bindinfo->row_size = resinfo->row_size;

	/* Copy the column metadata */
	for (i = 0; i < bindinfo->num_cols; i++) {

		curcol = bindinfo->columns[i];
		
		/*
		 * TODO use memcpy ??
		 * curcol and resinfo->columns[i] are both TDSCOLUMN.  
		 * Why not "curcol = resinfo->columns[i];"?  Because the rest of TDSCOLUMN (below column_timestamp)
		 * isn't being used.  Perhaps this "upper" part of TDSCOLUMN should be a substructure.
		 * Or, see if the "lower" part is unused (and zeroed out) at this point, and just do one assignment.
		 */
		curcol->funcs = resinfo->columns[i]->funcs;
		curcol->column_type = resinfo->columns[i]->column_type;
		curcol->column_usertype = resinfo->columns[i]->column_usertype;
		curcol->column_flags = resinfo->columns[i]->column_flags;
		if (curcol->column_varint_size == 0)
			curcol->column_cur_size = resinfo->columns[i]->column_cur_size;
		else
			curcol->column_cur_size = -1;
		curcol->column_size = resinfo->columns[i]->column_size;
		curcol->column_varint_size = resinfo->columns[i]->column_varint_size;
		curcol->column_prec = resinfo->columns[i]->column_prec;
		curcol->column_scale = resinfo->columns[i]->column_scale;
		curcol->column_namelen = resinfo->columns[i]->column_namelen;
		curcol->on_server.column_type = resinfo->columns[i]->on_server.column_type;
		curcol->on_server.column_size = resinfo->columns[i]->on_server.column_size;
		curcol->char_conv = resinfo->columns[i]->char_conv;
		memcpy(curcol->column_name, resinfo->columns[i]->column_name, resinfo->columns[i]->column_namelen);
		tds_dstr_dup(&curcol->table_column_name, &resinfo->columns[i]->table_column_name);
		curcol->column_nullable = resinfo->columns[i]->column_nullable;
		curcol->column_identity = resinfo->columns[i]->column_identity;
		curcol->column_timestamp = resinfo->columns[i]->column_timestamp;
		
		memcpy(curcol->column_collation, resinfo->columns[i]->column_collation, 5);
		
		if (is_numeric_type(curcol->column_type)) {
			curcol->bcp_column_data = tds_alloc_bcp_column_data(sizeof(TDS_NUMERIC));
			((TDS_NUMERIC *) curcol->bcp_column_data->data)->precision = curcol->column_prec;
			((TDS_NUMERIC *) curcol->bcp_column_data->data)->scale = curcol->column_scale;
		} else {
			curcol->bcp_column_data = 
				tds_alloc_bcp_column_data(MAX(curcol->column_size,curcol->on_server.column_size));
		}
	}

	bindinfo->current_row = (unsigned char*) malloc(bindinfo->row_size);
	if (!bindinfo->current_row)
		goto cleanup;
	bindinfo->row_free = tds_bcp_row_free;

	if (bcpinfo->identity_insert_on) {

		rc = tds_submit_queryf(tds, "set identity_insert %s on", bcpinfo->tablename);
		if (TDS_FAILED(rc))
			goto cleanup;

		/* TODO use tds_process_simple_query */
		while ((rc = tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_RESULTS))
			   == TDS_SUCCESS) {
		}
		if (rc != TDS_NO_MORE_RESULTS)
			goto cleanup;
	}

	bcpinfo->bindinfo = bindinfo;
	bcpinfo->bind_count = 0;
	return TDS_SUCCESS;

cleanup:
	tds_free_results(bindinfo);
	return rc;
}

/**
 * Help to build query to be sent to server.
 * Append column declaration to the query.
 * Only for TDS 7.0+.
 * \tds
 * \param[out] clause output string
 * \param bcpcol column to append
 * \param first  true if column is the first
 * \return TDS_SUCCESS or TDS_FAIL.
 */
static TDSRET
tds7_build_bulk_insert_stmt(TDSSOCKET * tds, TDSPBCB * clause, TDSCOLUMN * bcpcol, int first)
{
	char column_type[40];

	tdsdump_log(TDS_DBG_FUNC, "tds7_build_bulk_insert_stmt(%p, %p, %p, %d)\n", tds, clause, bcpcol, first);

	if (TDS_FAILED(tds_get_column_declaration(tds, bcpcol, column_type))) {
		tdserror(tds_get_ctx(tds), tds, TDSEBPROBADTYP, errno);
		tdsdump_log(TDS_DBG_FUNC, "error: cannot build bulk insert statement. unrecognized server datatype %d\n",
			    bcpcol->on_server.column_type);
		return TDS_FAIL;
	}

	if (clause->cb < strlen(clause->pb)
	    + tds_quote_id(tds, NULL, bcpcol->column_name, bcpcol->column_namelen)
	    + strlen(column_type)
	    + ((first) ? 2u : 4u)) {
		char *temp = (char*) malloc(2 * clause->cb);

		if (!temp) {
			tdserror(tds_get_ctx(tds), tds, TDSEMEM, errno);
			return TDS_FAIL;
		}
		strcpy(temp, clause->pb);
		if (clause->from_malloc)
			free(clause->pb);
		clause->from_malloc = 1;
		clause->pb = temp;
		clause->cb *= 2;
	}

	if (!first)
		strcat(clause->pb, ", ");

	tds_quote_id(tds, strchr(clause->pb, 0), bcpcol->column_name, bcpcol->column_namelen);
	strcat(clause->pb, " ");
	strcat(clause->pb, column_type);

	return TDS_SUCCESS;
}

/**
 * Prepare the query to be sent to server to request BCP information
 * \tds
 * \param bcpinfo BCP information
 */
static TDSRET
tds_bcp_start_insert_stmt(TDSSOCKET * tds, TDSBCPINFO * bcpinfo)
{
	char *query;

	if (IS_TDS7_PLUS(tds->conn)) {
		int i, firstcol, erc;
		char *hint;
		TDSCOLUMN *bcpcol;
		TDSPBCB colclause;
		char clause_buffer[4096] = { 0 };

		colclause.pb = clause_buffer;
		colclause.cb = sizeof(clause_buffer);
		colclause.from_malloc = 0;

		/* TODO avoid asprintf, use always malloc-ed buffer */
		firstcol = 1;

		for (i = 0; i < bcpinfo->bindinfo->num_cols; i++) {
			bcpcol = bcpinfo->bindinfo->columns[i];

			if (bcpcol->column_timestamp)
				continue;
			if (!bcpinfo->identity_insert_on && bcpcol->column_identity)
				continue;
			tds7_build_bulk_insert_stmt(tds, &colclause, bcpcol, firstcol);
			firstcol = 0;
		}

		if (bcpinfo->hint) {
			if (asprintf(&hint, " with (%s)", bcpinfo->hint) < 0)
				hint = NULL;
		} else {
			hint = strdup("");
		}
		if (!hint) {
			if (colclause.from_malloc)
				TDS_ZERO_FREE(colclause.pb);
			return TDS_FAIL;
		}

		erc = asprintf(&query, "insert bulk %s (%s)%s", bcpinfo->tablename, colclause.pb, hint);

		free(hint);
		if (colclause.from_malloc)
			TDS_ZERO_FREE(colclause.pb);	/* just for good measure; not used beyond this point */

		if (erc < 0)
			return TDS_FAIL;
	} else {
		/* NOTE: if we use "with nodescribe" for following inserts server do not send describe */
		if (asprintf(&query, "insert bulk %s", bcpinfo->tablename) < 0)
			return TDS_FAIL;
	}

	/* save the statement for later... */
	bcpinfo->insert_stmt = query;

	return TDS_SUCCESS;
}

/**
 * Send one row of data to server
 * \tds
 * \param bcpinfo BCP information
 * \param get_col_data function to call to retrieve data to be sent
 * \param ignored function to call if we try to send NULL if not allowed (not used)
 * \param offset passed to get_col_data and null_error to specify the row to get
 * \return TDS_SUCCESS or TDS_FAIL.
 */
TDSRET
tds_bcp_send_record(TDSSOCKET *tds, TDSBCPINFO *bcpinfo, tds_bcp_get_col_data get_col_data, tds_bcp_null_error ignored, int offset)
{
	TDSCOLUMN  *bindcol;

	static const TDS_TINYINT textptr_size = 16;
	static const unsigned char GEN_NULL = 0x00;

	static const unsigned char CHARBIN_NULL[] = { 0xff, 0xff };
	static const unsigned char textptr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
						 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	static const unsigned char timestamp[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	unsigned char *record;
	TDS_INT	 old_record_size;
	int i;
	TDSRET rc;

	tdsdump_log(TDS_DBG_FUNC, "tds_bcp_send_bcp_record(%p, %p, %p, ignored, %d)\n", tds, bcpinfo, get_col_data, offset);

	record = bcpinfo->bindinfo->current_row;
	old_record_size = bcpinfo->bindinfo->row_size;

	if (IS_TDS7_PLUS(tds->conn)) {
		TDS_TINYINT  varint_1;

		for (i = 0; i < bcpinfo->bindinfo->num_cols; i++) {
	
			bindcol = bcpinfo->bindinfo->columns[i];

			/*
			 * Don't send the (meta)data for timestamp columns or
			 * identity columns unless indentity_insert is enabled.
			 */

			if ((!bcpinfo->identity_insert_on && bindcol->column_identity) || 
				bindcol->column_timestamp) {
				continue;
			}

			rc = get_col_data(bcpinfo, bindcol, offset);
			if (TDS_FAILED(rc)) {
				tdsdump_log(TDS_DBG_INFO1, "get_col_data (column %d) failed\n", i + 1);
	 			return rc;
			}
			tdsdump_log(TDS_DBG_INFO1, "gotten column %d length %d null %d\n",
					i + 1, bindcol->bcp_column_data->datalen, bindcol->bcp_column_data->is_null);
	
			if (bindcol->bcp_column_data->is_null) {
				/* A default may be defined for the column; let server reject if not. */
				switch (bindcol->on_server.column_type) {
				case XSYBCHAR:
				case XSYBVARCHAR:
				case XSYBBINARY:
				case XSYBVARBINARY:
				case XSYBNCHAR:
				case XSYBNVARCHAR:
					memcpy(record, CHARBIN_NULL, 2);
					record +=2;
					break;
				default:
					*record = GEN_NULL;
					record++;
					break;
				}
			} else {

				switch (bindcol->column_varint_size) {
				case 4:
					if (is_blob_type(bindcol->on_server.column_type)) {
						*record = textptr_size; record++;
						memcpy(record, textptr, 16); record += 16;
						memcpy(record, timestamp, 8); record += 8;
					}
					TDS_PUT_UA4LE(record, bindcol->bcp_column_data->datalen);
					record += 4;
					break;
				case 2:
					TDS_PUT_UA2LE(record, bindcol->bcp_column_data->datalen);
					record += 2;
					break;
				case 1:
					varint_1 = bindcol->bcp_column_data->datalen;
					if (is_numeric_type(bindcol->on_server.column_type)) {
						varint_1 = tds_numeric_bytes_per_prec[bindcol->column_prec];
						tdsdump_log(TDS_DBG_INFO1, "numeric type prec = %d varint_1 = %d\n",
										 bindcol->column_prec, varint_1);
					} else {
						varint_1 = bindcol->bcp_column_data->datalen;
						tdsdump_log(TDS_DBG_INFO1, "varint_1 = %d\n", varint_1);
					}
					*record = varint_1; record++;
					break;
				case 0:
					break;
				}

				tdsdump_log(TDS_DBG_INFO1, "new_record_size = %ld datalen = %d\n", 
							(long int) (record - bcpinfo->bindinfo->current_row), bindcol->bcp_column_data->datalen);

#if WORDS_BIGENDIAN
				tds_swap_datatype(tds_get_conversion_type(bindcol->column_type, bindcol->bcp_column_data->datalen),
									bindcol->bcp_column_data->data);
#endif
				if (is_numeric_type(bindcol->on_server.column_type)) {
					TDS_NUMERIC *num = (TDS_NUMERIC *) bindcol->bcp_column_data->data;
					int size;
					tdsdump_log(TDS_DBG_INFO1, "numeric type prec = %d\n", num->precision);
					if (IS_TDS7_PLUS(tds->conn))
						tds_swap_numeric(num);
					size = tds_numeric_bytes_per_prec[num->precision];
					memcpy(record, num->array, size);
					record += size; 
				} else {
					memcpy(record, bindcol->bcp_column_data->data, bindcol->bcp_column_data->datalen);
					record += bindcol->bcp_column_data->datalen;
				}

			}
			tdsdump_log(TDS_DBG_INFO1, "old_record_size = %d new size = %ld\n",
					old_record_size, (long int) (record - bcpinfo->bindinfo->current_row));
		}

		tds_put_byte(tds, TDS_ROW_TOKEN);   /* 0xd1 */
		tds_put_n(tds, bcpinfo->bindinfo->current_row, record - bcpinfo->bindinfo->current_row);
	}  /* IS_TDS7_PLUS */
	else {
		int row_pos;
		int row_sz_pos;
		TDS_SMALLINT row_size;
		int blob_cols = 0;
		int var_cols_written = 0;

		memset(record, '\0', old_record_size);	/* zero the rowbuffer */

		/*
		 * offset 0 = number of var columns
		 * offset 1 = row number.  zeroed (datasever assigns)
		 */
		row_pos = 2;

		if ((row_pos = tds_bcp_add_fixed_columns(bcpinfo, get_col_data, NULL, offset, record, row_pos)) < 0)
			return TDS_FAIL;

		row_sz_pos = row_pos;

		/* potential variable columns to write */

		if (bcpinfo->var_cols) {
			if ((row_pos = tds_bcp_add_variable_columns(bcpinfo, get_col_data, NULL, offset, record, row_pos, &var_cols_written)) < 0)
				return TDS_FAIL;
		}

		row_size = row_pos;

		if (var_cols_written) {
			memcpy(&record[row_sz_pos], &row_size, sizeof(row_size));
			record[0] = var_cols_written;
		}

		tdsdump_log(TDS_DBG_INFO1, "old_record_size = %d new size = %d \n", old_record_size, row_size);

		tds_put_smallint(tds, row_size);
		tds_put_n(tds, record, row_size);

		/* row is done, now handle any text/image data */

		blob_cols = 0;

		for (i = 0; i < bcpinfo->bindinfo->num_cols; i++) {
			bindcol = bcpinfo->bindinfo->columns[i];
			if (is_blob_type(bindcol->column_type)) {
				rc = get_col_data(bcpinfo, bindcol, offset);
				if (TDS_FAILED(rc))
					return rc;
				/* unknown but zero */
				tds_put_smallint(tds, 0);
				tds_put_byte(tds, bindcol->column_type);
				tds_put_byte(tds, 0xff - blob_cols);
				/*
				 * offset of txptr we stashed during variable
				 * column processing 
				 */
				tds_put_smallint(tds, bindcol->column_textpos);
				tds_put_int(tds, bindcol->bcp_column_data->datalen);
				tds_put_n(tds, bindcol->bcp_column_data->data, bindcol->bcp_column_data->datalen);
				blob_cols++;

			}
		}
	}

	return TDS_SUCCESS;
}

/**
 * Add fixed size columns to the row
 * \param bcpinfo BCP information
 * \param get_col_data function to call to retrieve data to be sent
 * \param ignored function to call if we try to send NULL if not allowed (not used)
 * \param offset passed to get_col_data and null_error to specify the row to get
 * \param rowbuffer row buffer to write to
 * \param start row buffer last end position
 * \returns new row length or -1 on error.
 */
static int
tds_bcp_add_fixed_columns(TDSBCPINFO *bcpinfo, tds_bcp_get_col_data get_col_data, tds_bcp_null_error ignored, 
				int offset, unsigned char * rowbuffer, int start)
{
	TDS_NUMERIC *num;
	int row_pos = start;
	TDSCOLUMN *bcpcol;
	int cpbytes;
	int i, j;

	assert(bcpinfo);
	assert(rowbuffer);

	tdsdump_log(TDS_DBG_FUNC, "tds_bcp_add_fixed_columns(%p, %p, ignored, %d, %p, %d)\n", bcpinfo, get_col_data, offset, rowbuffer, start);

	for (i = 0; i < bcpinfo->bindinfo->num_cols; i++) {

		bcpcol = bcpinfo->bindinfo->columns[i];

		if (is_nullable_type(bcpcol->column_type) || bcpcol->column_nullable)
			continue;

		tdsdump_log(TDS_DBG_FUNC, "tds_bcp_add_fixed_columns column %d is a fixed column\n", i + 1);

		if (TDS_FAILED(get_col_data(bcpinfo, bcpcol, offset))) {
			tdsdump_log(TDS_DBG_INFO1, "get_col_data (column %d) failed\n", i + 1);
			return -1;
		}

#if USING_SYBEBCNN
		Let the server reject if no default is defined for the column.
		if (bcpcol->bcp_column_data->is_null) {
			/* No value or default value available and NULL not allowed. */
			null_error(bcpinfo, i, offset);
			return -1;
		}
#endif

		if (is_numeric_type(bcpcol->column_type)) {
			num = (TDS_NUMERIC *) bcpcol->bcp_column_data->data;
			cpbytes = tds_numeric_bytes_per_prec[num->precision];
			memcpy(&rowbuffer[row_pos], num->array, cpbytes);
		} else {
			cpbytes = bcpcol->bcp_column_data->datalen > bcpcol->column_size ?
				  bcpcol->column_size : bcpcol->bcp_column_data->datalen;
			memcpy(&rowbuffer[row_pos], bcpcol->bcp_column_data->data, cpbytes);

			/* CHAR data may need padding out to the database length with blanks */
			/* TODO check binary !!! */
			if (bcpcol->column_type == SYBCHAR && cpbytes < bcpcol->column_size) {
				for (j = cpbytes; j <  bcpcol->column_size; j++)
					rowbuffer[row_pos + j] = ' ';
			}
		}

		row_pos += bcpcol->column_size;
	}
	return row_pos;
}

/**
 * Add variable size columns to the row
 *
 * \param bcpinfo BCP information already prepared
 * \param get_col_data function to call to retrieve data to be sent
 * \param null_error function to call if we try to send NULL if not allowed
 * \param offset passed to get_col_data and null_error to specify the row to get
 * \param rowbuffer The row image that will be sent to the server. 
 * \param start Where to begin copying data into the rowbuffer. 
 * \param pncols Address of output variable holding the count of columns added to the rowbuffer.  
 * 
 * \return length of (potentially modified) rowbuffer, or -1.
 */
static int
tds_bcp_add_variable_columns(TDSBCPINFO *bcpinfo, tds_bcp_get_col_data get_col_data, tds_bcp_null_error null_error, int offset, TDS_UCHAR* rowbuffer, int start, int *pncols)
{
	TDS_USMALLINT offsets[256];
	unsigned int i, row_pos;
	unsigned int ncols = 0;

	assert(bcpinfo);
	assert(rowbuffer);
	assert(pncols);

	tdsdump_log(TDS_DBG_FUNC, "%4s %8s %18s %18s %8s\n", 	"col", 
								"type", 
								"is_nullable_type", 
								"column_nullable", 
								"is null" );
	for (i = 0; i < bcpinfo->bindinfo->num_cols; i++) {
		TDSCOLUMN *bcpcol = bcpinfo->bindinfo->columns[i];
		tdsdump_log(TDS_DBG_FUNC, "%4d %8d %18s %18s %8s\n", 	i, 
									bcpcol->column_type,  
									is_nullable_type(bcpcol->column_type)? "yes" : "no", 
									bcpcol->column_nullable? "yes" : "no", 
									bcpcol->bcp_column_data->is_null? "yes" : "no" );
	}

	/* the first two bytes of the rowbuffer are reserved to hold the entire record length */
	row_pos = start + 2;
	offsets[0] = row_pos;

	tdsdump_log(TDS_DBG_FUNC, "%4s %8s %8s %8s\n", "col", "ncols", "row_pos", "cpbytes");

	for (i = 0; i < bcpinfo->bindinfo->num_cols; i++) {
		unsigned int cpbytes = 0;
		TDSCOLUMN *bcpcol = bcpinfo->bindinfo->columns[i];

		/*
		 * Is this column of "variable" type, i.e. NULLable
		 * or naturally variable length e.g. VARCHAR
		 */
		if (!is_nullable_type(bcpcol->column_type) && !bcpcol->column_nullable)
			continue;

		tdsdump_log(TDS_DBG_FUNC, "%4d %8d %8d %8d\n", i, ncols, row_pos, cpbytes);

		if (TDS_FAILED(get_col_data(bcpinfo, bcpcol, offset)))
			return -1;

#if USING_SYBEBCNN
		/* If it's a NOT NULL column, and we have no data, throw an error. */
		No, the column could have a default defined.
		if (!(bcpcol->column_nullable) && bcpcol->bcp_column_data->is_null) {
			/* No value or default value available and NULL not allowed. */
			null_error(bcpinfo, i, offset);
			return -1;
		}
#endif

		/* move the column buffer into the rowbuffer */
		if (!bcpcol->bcp_column_data->is_null) {
			if (is_blob_type(bcpcol->column_type)) {
				cpbytes = 16;
				bcpcol->column_textpos = row_pos;               /* save for data write */
			} else if (is_numeric_type(bcpcol->column_type)) {
					TDS_NUMERIC *num = (TDS_NUMERIC *) bcpcol->bcp_column_data->data;
				cpbytes = tds_numeric_bytes_per_prec[num->precision];
				memcpy(&rowbuffer[row_pos], num->array, cpbytes);
			} else {
				cpbytes = bcpcol->bcp_column_data->datalen > bcpcol->column_size ?
				bcpcol->column_size : bcpcol->bcp_column_data->datalen;
				memcpy(&rowbuffer[row_pos], bcpcol->bcp_column_data->data, cpbytes);
			}
		}

		row_pos += cpbytes;
		offsets[++ncols] = row_pos;
		tdsdump_dump_buf(TDS_DBG_NETWORK, "BCP row buffer so far", rowbuffer,  row_pos);
	}

	tdsdump_log(TDS_DBG_FUNC, "%4d %8d %8d\n", i, ncols, row_pos);

	/*
	 * The rowbuffer ends with an offset table and, optionally, an adjustment table.  
	 * The offset table has 1-byte elements that describe the locations of the start of each column in
	 * the rowbuffer.  If the largest offset is greater than 255, another table -- the adjustment table --
	 * is inserted just before the offset table.  It holds the high bytes. 
	 * 
	 * Both tables are laid out in reverse:
	 * 	#elements, offset N+1, offset N, offset N-1, ... offset 0
	 * E.g. for 2 columns you have 4 data points:
	 *	1.  How many elements (4)
	 *	2.  Start of column 3 (non-existent, "one off the end")
	 *	3.  Start of column 2
	 *	4.  Start of column 1
	 *  The length of each column is computed by subtracting its start from the its successor's start. 
	 *
	 * The algorithm below computes both tables. If the adjustment table isn't needed, the 
	 * effect is to overwrite it with the offset table.  
	 */
	while (ncols && offsets[ncols] == offsets[ncols-1])
		ncols--;	/* trailing NULL columns are not sent and are not included in the offset table */

	if (ncols) {
		TDS_UCHAR *poff = rowbuffer + row_pos;
		unsigned int pfx_top = offsets[ncols] / 256;

		tdsdump_log(TDS_DBG_FUNC, "ncols=%u poff=%p [%u]\n", ncols, poff, offsets[ncols]);

		*poff++ = ncols + 1;
		/* this is some kind of run-length-prefix encoding */
		while (pfx_top) {
			unsigned int n_pfx = 1;

			for (i = 0; i <= ncols ; ++i)
				if ((offsets[i] / 256) < pfx_top)
					++n_pfx;
			*poff++ = n_pfx;
			--pfx_top;
		}
   
		tdsdump_log(TDS_DBG_FUNC, "poff=%p\n", poff);

		for (i=0; i <= ncols; i++)
			*poff++ = offsets[ncols-i] & 0xFF;
		row_pos = (unsigned int)(poff - rowbuffer);
	}

	tdsdump_log(TDS_DBG_FUNC, "%4d %8d %8d\n", i, ncols, row_pos);
	tdsdump_dump_buf(TDS_DBG_NETWORK, "BCP row buffer", rowbuffer,  row_pos);

	*pncols = ncols;

	return ncols == 0? start : row_pos;
}

/**
 * Send BCP metadata to server.
 * Only for TDS 7.0+.
 * \tds
 * \param bcpinfo BCP information
 * \return TDS_SUCCESS or TDS_FAIL.
 */
static TDSRET
tds7_bcp_send_colmetadata(TDSSOCKET *tds, TDSBCPINFO *bcpinfo)
{
	TDSCOLUMN *bcpcol;
	int i, num_cols;

	tdsdump_log(TDS_DBG_FUNC, "tds7_bcp_send_colmetadata(%p, %p)\n", tds, bcpinfo);
	assert(tds && bcpinfo);

	/* 
	 * Deep joy! For TDS 8 we have to send a colmetadata message followed by row data 
	 */
	tds_put_byte(tds, TDS7_RESULT_TOKEN);	/* 0x81 */

	num_cols = 0;
	for (i = 0; i < bcpinfo->bindinfo->num_cols; i++) {
		bcpcol = bcpinfo->bindinfo->columns[i];
		if ((!bcpinfo->identity_insert_on && bcpcol->column_identity) || 
			bcpcol->column_timestamp) {
			continue;
		}
		num_cols++;
	}

	tds_put_smallint(tds, num_cols);

	for (i = 0; i < bcpinfo->bindinfo->num_cols; i++) {
		bcpcol = bcpinfo->bindinfo->columns[i];

		/*
		 * dont send the (meta)data for timestamp columns, or
		 * identity columns (unless indentity_insert is enabled
		 */

		if ((!bcpinfo->identity_insert_on && bcpcol->column_identity) || 
			bcpcol->column_timestamp) {
			continue;
		}

		if (IS_TDS72_PLUS(tds->conn))
			tds_put_int(tds, bcpcol->column_usertype);
		else
			tds_put_smallint(tds, bcpcol->column_usertype);
		tds_put_smallint(tds, bcpcol->column_flags);
		tds_put_byte(tds, bcpcol->on_server.column_type);

		switch (bcpcol->column_varint_size) {
		case 4:
			tds_put_int(tds, bcpcol->column_size);
			break;
		case 2:
			tds_put_smallint(tds, bcpcol->column_size);
			break;
		case 1:
			tds_put_byte(tds, bcpcol->column_size);
			break;
		case 0:
			break;
		default:
			break;
		}

		if (is_numeric_type(bcpcol->on_server.column_type)) {
			tds_put_byte(tds, bcpcol->column_prec);
			tds_put_byte(tds, bcpcol->column_scale);
		}
		if (IS_TDS71_PLUS(tds->conn)
			&& is_collate_type(bcpcol->on_server.column_type)) {
			tds_put_n(tds, bcpcol->column_collation, 5);
		}
		if (is_blob_type(bcpcol->on_server.column_type)) {
			/* FIXME strlen return len in bytes not in characters required here */
			TDS_PUT_SMALLINT(tds, strlen(bcpinfo->tablename));
			tds_put_string(tds, bcpinfo->tablename, (int)strlen(bcpinfo->tablename));
		}
		/* FIXME support multibyte string */
		tds_put_byte(tds, bcpcol->column_namelen);
		tds_put_string(tds, bcpcol->column_name, bcpcol->column_namelen);

	}

	return TDS_SUCCESS;
}

/**
 * Tell we finished sending BCP data to server
 * \tds
 * \param[out] rows_copied number of rows copied to server
 */
TDSRET
tds_bcp_done(TDSSOCKET *tds, int *rows_copied)
{
	TDSRET rc;

	tdsdump_log(TDS_DBG_FUNC, "tds_bcp_done(%p, %p)\n", tds, rows_copied);

	/* TODO check proper tds state */
	tds_flush_packet(tds);

	tds_set_state(tds, TDS_PENDING);

	rc = tds_process_simple_query(tds);
	if (TDS_FAILED(rc))
		return rc;

	if (rows_copied)
		*rows_copied = tds->rows_affected;

	return TDS_SUCCESS;
}

/**
 * Start sending BCP data to server.
 * Initialize stream to accept data.
 * \tds
 * \param bcpinfo BCP information already prepared
 */
TDSRET
tds_bcp_start(TDSSOCKET *tds, TDSBCPINFO *bcpinfo)
{
	TDSRET rc;

	tdsdump_log(TDS_DBG_FUNC, "tds_bcp_start(%p, %p)\n", tds, bcpinfo);

	tds_submit_query(tds, bcpinfo->insert_stmt);

	/*
	 * In TDS 5 we get the column information as a result set from the "insert bulk" command.
	 * We're going to ignore it.  
	 */
	rc = tds_process_simple_query(tds);
	if (TDS_FAILED(rc))
		return rc;

	/* TODO problem with thread safety */
	tds->out_flag = TDS_BULK;
	tds_set_state(tds, TDS_QUERYING);

	if (IS_TDS7_PLUS(tds->conn))
		tds7_bcp_send_colmetadata(tds, bcpinfo);
	
	return TDS_SUCCESS;
}

/**
 * Free row data allocated in the result set.
 */
static void 
tds_bcp_row_free(TDSRESULTINFO* result, unsigned char *row)
{
	result->row_size = 0;
	TDS_ZERO_FREE(result->current_row);
}

/**
 * Start bulk copy to server
 * \tds
 * \param bcpinfo BCP information already prepared
 */
TDSRET
tds_bcp_start_copy_in(TDSSOCKET *tds, TDSBCPINFO *bcpinfo)
{
	TDSCOLUMN *bcpcol;
	int i;
	int fixed_col_len_tot     = 0;
	int variable_col_len_tot  = 0;
	int column_bcp_data_size  = 0;
	int bcp_record_size       = 0;
	TDSRET rc;
	
	tdsdump_log(TDS_DBG_FUNC, "tds_bcp_start_copy_in(%p, %p)\n", tds, bcpinfo);

	rc = tds_bcp_start_insert_stmt(tds, bcpinfo);
	if (TDS_FAILED(rc))
		return rc;

	rc = tds_bcp_start(tds, bcpinfo);
	if (TDS_FAILED(rc)) {
		/* TODO, in CTLib was _ctclient_msg(blkdesc->con, "blk_rowxfer", 2, 5, 1, 140, ""); */
		return rc;
	}

	/* 
	 * Work out the number of "variable" columns.  These are either nullable or of 
	 * varying length type e.g. varchar.   
	 */
	bcpinfo->var_cols = 0;

	if (IS_TDS50(tds->conn)) {
		for (i = 0; i < bcpinfo->bindinfo->num_cols; i++) {
	
			bcpcol = bcpinfo->bindinfo->columns[i];

			/*
			 * work out storage required for this datatype
			 * blobs always require 16, numerics vary, the
			 * rest can be taken from the server
			 */

			if (is_blob_type(bcpcol->on_server.column_type))
				column_bcp_data_size  = 16;
			else if (is_numeric_type(bcpcol->on_server.column_type))
				column_bcp_data_size  = tds_numeric_bytes_per_prec[bcpcol->column_prec];
			else
				column_bcp_data_size  = bcpcol->column_size;

			/*
			 * now add that size into either fixed or variable
			 * column totals...
			 */

			if (is_nullable_type(bcpcol->on_server.column_type) || bcpcol->column_nullable) {
				bcpinfo->var_cols++;
				variable_col_len_tot += column_bcp_data_size;
			}
			else {
				fixed_col_len_tot += column_bcp_data_size;
			}
		}

		/* this formula taken from sybase manual... */

		bcp_record_size =  	4 +
							fixed_col_len_tot +
							variable_col_len_tot +
							( (int)(variable_col_len_tot / 256 ) + 1 ) +
							(bcpinfo->var_cols + 1) +
							2;

		tdsdump_log(TDS_DBG_FUNC, "current_record_size = %d\n", bcpinfo->bindinfo->row_size);
		tdsdump_log(TDS_DBG_FUNC, "bcp_record_size     = %d\n", bcp_record_size);

		if (bcp_record_size > bcpinfo->bindinfo->row_size) {
			/* FIXME remove memory leak */
			bcpinfo->bindinfo->current_row = (unsigned char*) realloc(bcpinfo->bindinfo->current_row, bcp_record_size);
			bcpinfo->bindinfo->row_free = tds_bcp_row_free;
			if (bcpinfo->bindinfo->current_row == NULL) {
				tdsdump_log(TDS_DBG_FUNC, "could not realloc current_row\n");
				return TDS_FAIL;
			}
			bcpinfo->bindinfo->row_size = bcp_record_size;
		}
	}
	if (IS_TDS7_PLUS(tds->conn)) {
		for (i = 0; i < bcpinfo->bindinfo->num_cols; i++) {
	
			bcpcol = bcpinfo->bindinfo->columns[i];

			/*
			 * dont send the (meta)data for timestamp columns, or
			 * identity columns (unless indentity_insert is enabled
			 */

			if ((!bcpinfo->identity_insert_on && bcpcol->column_identity) || 
				bcpcol->column_timestamp) {
				continue;
			}

			switch (bcpcol->column_varint_size) {
			case 4:
				if (is_blob_type(bcpcol->column_type))
					bcp_record_size += 25;
				bcp_record_size += 4;
				break;
			case 2:
				bcp_record_size +=2;
				break;
			case 1:
				bcp_record_size++;
				break;
			case 0:
				break;
			}

			if (is_numeric_type(bcpcol->column_type)) {
				bcp_record_size += tds_numeric_bytes_per_prec[bcpcol->column_prec];
			} else {
				bcp_record_size += bcpcol->column_size;
			}
		}
		tdsdump_log(TDS_DBG_FUNC, "current_record_size = %d\n", bcpinfo->bindinfo->row_size);
		tdsdump_log(TDS_DBG_FUNC, "bcp_record_size     = %d\n", bcp_record_size);

		if (bcp_record_size > bcpinfo->bindinfo->row_size) {
			bcpinfo->bindinfo->current_row = (unsigned char*) realloc(bcpinfo->bindinfo->current_row, bcp_record_size);
			bcpinfo->bindinfo->row_free = tds_bcp_row_free;
			if (bcpinfo->bindinfo->current_row == NULL) {
				tdsdump_log(TDS_DBG_FUNC, "could not realloc current_row\n");
				return TDS_FAIL;
			}
			bcpinfo->bindinfo->row_size = bcp_record_size;
		}
	}

	return TDS_SUCCESS;
}

/** input stream to read a file */
typedef struct tds_file_stream {
	/** common fields, must be the first field */
	TDSINSTREAM stream;
	/** file to read from */
	FILE *f;

	/** terminator */
	const char *terminator;
	/** terminator length in bytes */
	size_t term_len;

	/** buffer for store bytes readed that could be the terminator */
	char *left;
} TDSFILESTREAM;

/** \cond HIDDEN_SYMBOLS */
#ifndef TDS_HAVE_STDIO_LOCKED
#undef getc_unlocked
#undef feof_unlocked
#undef flockfile
#undef funlockfile
#define getc_unlocked(s) getc(s)
#define feof_unlocked(s) feof(s)
#define flockfile(s) do { } while(0)
#define funlockfile(s) do { } while(0)
#endif
/** \endcond */

/**
 * Reads a chunk of data from file stream checking for terminator
 * \param stream file stream
 * \param ptr buffer where to read data
 * \param len length of buffer
 */
static int
tds_file_stream_read(TDSINSTREAM *stream, void *ptr, size_t len)
{
	TDSFILESTREAM *s = (TDSFILESTREAM *) stream;
	int c;
	char *p = (char *) ptr;
/** \cond HIDDEN_SYMBOLS */
#define GETC() do { c = getc_unlocked(s->f); if (c==EOF) return -1; } while(0)
/** \endcond */

	while (len) {
		if (memcmp(s->left, s->terminator, s->term_len) == 0)
			return p - (char *) ptr;

		GETC();

		*p++ = s->left[0];
		--len;

		memmove(s->left, s->left+1, s->term_len-1);
		s->left[s->term_len-1] = c;
	}
	return p - (char *) ptr;
}

/**
 * Read a data file, passing the data through iconv().
 * \retval TDS_SUCCESS  success
 * \retval TDS_FAIL     error reading the column
 * \retval TDS_NO_MORE_RESULTS end of file detected
 */
TDSRET
tds_bcp_fread(TDSSOCKET * tds, TDSICONV * char_conv, FILE * stream, const char *terminator, size_t term_len, char **outbuf, size_t * outbytes)
{
	TDSRET res;
	TDSFILESTREAM r;
	TDSDYNAMICSTREAM w;
	size_t readed;

	/* prepare streams */
	r.stream.read = tds_file_stream_read;
	r.f = stream;
	r.terminator = terminator;
	r.term_len = term_len;
	r.left = calloc(1, term_len);
	if (!r.left) return TDS_FAIL;

	/* read initial buffer to test with terminator */
	readed = fread(r.left, 1, term_len, stream);
	if (readed != term_len) {
		free(r.left);
		if (readed == 0 && feof(stream))
			return TDS_NO_MORE_RESULTS;
		return TDS_FAIL;
	}

	res = tds_dynamic_stream_init(&w, (void**) outbuf, 0);
	if (TDS_FAILED(res)) {
		free(r.left);
		return res;
	}

	/* convert/copy from input stream to output one */
	flockfile(stream);
	if (char_conv == NULL)
		res = tds_copy_stream(tds, &r.stream, &w.stream);
	else
		res = tds_convert_stream(tds, char_conv, to_server, &r.stream, &w.stream);
	funlockfile(stream);
	free(r.left);

	if (TDS_FAILED(res))
		return res;

	*outbytes = w.size;

	/* terminate buffer */
	if (!w.stream.buf_len)
		return TDS_FAIL;

	((char *) w.stream.buffer)[0] = 0;
	w.stream.write(&w.stream, 1);

	return res;
}

/**
 * Start writing writetext request.
 * This request start a bulk session.
 * \tds
 * \param objname table name
 * \param textptr TEXTPTR (see sql documentation)
 * \param timestamp data timestamp
 * \param with_log is log is enabled during insert
 * \param size bytes to be inserted
 */
TDSRET
tds_writetext_start(TDSSOCKET *tds, const char *objname, const char *textptr, const char *timestamp, int with_log, TDS_UINT size)
{
	TDSRET rc;

	/* TODO mssql does not like timestamp */
	rc = tds_submit_queryf(tds,
			      "writetext bulk %s 0x%s timestamp = 0x%s%s",
			      objname, textptr, timestamp, with_log ? " with log" : "");
	if (TDS_FAILED(rc))
		return rc;

	/* FIXME in this case processing all results can bring state to IDLE... not threading safe */
	/* read the end token */
	rc = tds_process_simple_query(tds);
	if (TDS_FAILED(rc))
		return rc;

	/* FIXME better transition state */
	tds->out_flag = TDS_BULK;
	if (tds_set_state(tds, TDS_QUERYING) != TDS_QUERYING)
		return TDS_FAIL;
	tds_put_int(tds, size);
	return TDS_SUCCESS;
}

/**
 * Send some data in the writetext request started by tds_writetext_start.
 * You should write in total (with multiple calls to this function) all
 * bytes declared calling tds_writetext_start.
 * \tds
 * \param text data to write
 * \param size data size in bytes
 */
TDSRET
tds_writetext_continue(TDSSOCKET *tds, const TDS_UCHAR *text, TDS_UINT size)
{
	/* TODO check state */
	if (tds->out_flag != TDS_BULK)
		return TDS_FAIL;

	/* TODO check size letft */
	tds_put_n(tds, text, size);
	return TDS_SUCCESS;
}

/**
 * Finish sending writetext data.
 * \tds
 */
TDSRET
tds_writetext_end(TDSSOCKET *tds)
{
	/* TODO check state */
	if (tds->out_flag != TDS_BULK)
		return TDS_FAIL;

	tds_flush_packet(tds);
	tds_set_state(tds, TDS_PENDING);
	return TDS_SUCCESS;
}
