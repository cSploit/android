/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-2004, 2005, 2010  Brian Bruns, Bill Thompson
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

#include "bkpublic.h"

#include "ctpublic.h"
#include "ctlib.h"
#include "replacements.h"

TDS_RCSID(var, "$Id: blk.c,v 1.58 2011-09-25 11:36:24 freddy77 Exp $");

static void _blk_null_error(TDSBCPINFO *bcpinfo, int index, int offset);
static TDSRET _blk_get_col_data(TDSBCPINFO *bulk, TDSCOLUMN *bcpcol, int offset);
static CS_RETCODE _blk_rowxfer_in(CS_BLKDESC * blkdesc, CS_INT rows_to_xfer, CS_INT * rows_xferred);
static CS_RETCODE _blk_rowxfer_out(CS_BLKDESC * blkdesc, CS_INT rows_to_xfer, CS_INT * rows_xferred);

CS_RETCODE
blk_alloc(CS_CONNECTION * connection, CS_INT version, CS_BLKDESC ** blk_pointer)
{
	tdsdump_log(TDS_DBG_FUNC, "blk_alloc(%p, %d, %p)\n", connection, version, blk_pointer);

	*blk_pointer = (CS_BLKDESC *) calloc(1, sizeof(CS_BLKDESC));

	/* so we know who we belong to */
	(*blk_pointer)->con = connection;

	return CS_SUCCEED;
}


CS_RETCODE
blk_bind(CS_BLKDESC * blkdesc, CS_INT item, CS_DATAFMT * datafmt, CS_VOID * buffer, CS_INT * datalen, CS_SMALLINT * indicator)
{
	TDSCOLUMN *colinfo;
	CS_CONNECTION *con;
	CS_INT bind_count;
	int i;

	tdsdump_log(TDS_DBG_FUNC, "blk_bind(%p, %d, %p, %p, %p, %p)\n", blkdesc, item, datafmt, buffer, datalen, indicator);

	if (!blkdesc) {
		return CS_FAIL;
	}
	con = blkdesc->con;

	if (item == CS_UNUSED) {
		/* clear all bindings */
		if (datafmt == NULL && buffer == NULL && datalen == NULL && indicator == NULL ) { 
			blkdesc->bcpinfo.bind_count = CS_UNUSED;
			for (i = 0; i < blkdesc->bcpinfo.bindinfo->num_cols; i++ ) {
				colinfo = blkdesc->bcpinfo.bindinfo->columns[i];
				colinfo->column_varaddr  = NULL;
				colinfo->column_bindtype = 0;
				colinfo->column_bindfmt  = 0;
				colinfo->column_bindlen  = 0;
				colinfo->column_nullbind = NULL;
				colinfo->column_lenbind  = NULL;
			}
		}
		return CS_SUCCEED;
	}

	/* check item value */

	if (item < 1 || item > blkdesc->bcpinfo.bindinfo->num_cols) {
		_ctclient_msg(con, "blk_bind", 2, 5, 1, 141, "%s, %d", "colnum", item);
		return CS_FAIL;
	}

	/* clear bindings for this column */

	if (datafmt == NULL && buffer == NULL && datalen == NULL && indicator == NULL ) { 

		colinfo = blkdesc->bcpinfo.bindinfo->columns[item - 1];
		colinfo->column_varaddr  = NULL;
		colinfo->column_bindtype = 0;
		colinfo->column_bindfmt  = 0;
		colinfo->column_bindlen  = 0;
		colinfo->column_nullbind = NULL;
		colinfo->column_lenbind  = NULL;

		return CS_SUCCEED;
	}

	/*
	 * check whether the request is for array binding and ensure that user
	 * supplies the same datafmt->count to the subsequent ct_bind calls
	 */

	bind_count = (datafmt->count == 0) ? 1 : datafmt->count;

	/* first bind for this result set */

	if (blkdesc->bcpinfo.bind_count == CS_UNUSED) {
		blkdesc->bcpinfo.bind_count = bind_count;
	} else {
		/* all subsequent binds for this result set - the bind counts must be the same */
		if (blkdesc->bcpinfo.bind_count != bind_count) {
			_ctclient_msg(con, "blk_bind", 1, 1, 1, 137, "%d, %d", bind_count, blkdesc->bcpinfo.bind_count);
			return CS_FAIL;
		}
	}

	/* bind the column_varaddr to the address of the buffer */

	colinfo = blkdesc->bcpinfo.bindinfo->columns[item - 1];

	colinfo->column_varaddr = (char *) buffer;
	colinfo->column_bindtype = datafmt->datatype;
	colinfo->column_bindfmt = datafmt->format;
	colinfo->column_bindlen = datafmt->maxlength;
	if (indicator) {
		colinfo->column_nullbind = indicator;
	}
	if (datalen) {
		colinfo->column_lenbind = datalen;
	}
	return CS_SUCCEED;
}

CS_RETCODE
blk_colval(SRV_PROC * srvproc, CS_BLKDESC * blkdescp, CS_BLK_ROW * rowp, CS_INT colnum, CS_VOID * valuep, CS_INT valuelen,
	   CS_INT * outlenp)
{

	tdsdump_log(TDS_DBG_FUNC, "blk_colval(%p, %p, %p, %d, %p, %d, %p)\n", 
				srvproc, blkdescp, rowp, colnum, valuep, valuelen, outlenp);

	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED blk_colval()\n");
	return CS_FAIL;
}

CS_RETCODE
blk_default(CS_BLKDESC * blkdesc, CS_INT colnum, CS_VOID * buffer, CS_INT buflen, CS_INT * outlen)
{

	tdsdump_log(TDS_DBG_FUNC, "blk_default(%p, %d, %p, %d, %p)\n", blkdesc, colnum, buffer, buflen, outlen);

	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED blk_default()\n");
	return CS_FAIL;
}

CS_RETCODE
blk_describe(CS_BLKDESC * blkdesc, CS_INT item, CS_DATAFMT * datafmt)
{
	TDSCOLUMN *curcol;
	int len;

	tdsdump_log(TDS_DBG_FUNC, "blk_describe(%p, %d, %p)\n", blkdesc, item, datafmt);

	if (item < 1 || item > blkdesc->bcpinfo.bindinfo->num_cols) {
		_ctclient_msg(blkdesc->con, "blk_describe", 2, 5, 1, 141, "%s, %d", "colnum", item);
		return CS_FAIL;
	}

	curcol = blkdesc->bcpinfo.bindinfo->columns[item - 1];
	len = curcol->column_namelen;
	if (len >= CS_MAX_NAME)
		len = CS_MAX_NAME - 1;
	strncpy(datafmt->name, curcol->column_name, len);
	/* name is always null terminated */
	datafmt->name[len] = 0;
	datafmt->namelen = len;
	/* need to turn the SYBxxx into a CS_xxx_TYPE */
	datafmt->datatype = _ct_get_client_type(curcol);
	tdsdump_log(TDS_DBG_INFO1, "blk_describe() datafmt->datatype = %d server type %d\n", datafmt->datatype,
			curcol->column_type);
	/* FIXME is ok this value for numeric/decimal? */
	datafmt->maxlength = curcol->column_size;
	datafmt->usertype = curcol->column_usertype;
	datafmt->precision = curcol->column_prec;
	datafmt->scale = curcol->column_scale;

	/*
	 * There are other options that can be returned, but these are the
	 * only two being noted via the TDS layer.
	 */
	datafmt->status = 0;
	if (curcol->column_nullable)
		datafmt->status |= CS_CANBENULL;
	if (curcol->column_identity)
		datafmt->status |= CS_IDENTITY;

	datafmt->count = 1;
	datafmt->locale = NULL;

	return CS_SUCCEED;
}

CS_RETCODE
blk_done(CS_BLKDESC * blkdesc, CS_INT type, CS_INT * outrow)
{
	TDSSOCKET *tds;
	int rows_copied;

	tdsdump_log(TDS_DBG_FUNC, "blk_done(%p, %d, %p)\n", blkdesc, type, outrow);

	tds = blkdesc->con->tds_socket;

	switch (type) {
	case CS_BLK_BATCH:
		if (TDS_FAILED(tds_bcp_done(tds, &rows_copied))) {
			_ctclient_msg(blkdesc->con, "blk_done", 2, 5, 1, 140, "");
			return CS_FAIL;
		}
		
		if (outrow) 
			*outrow = rows_copied;
		
		if (TDS_FAILED(tds_bcp_start(tds, &blkdesc->bcpinfo))) {
			_ctclient_msg(blkdesc->con, "blk_done", 2, 5, 1, 140, "");
			return CS_FAIL;
		}
		break;
		
	case CS_BLK_ALL:
		if (TDS_FAILED(tds_bcp_done(tds, &rows_copied))) {
			_ctclient_msg(blkdesc->con, "blk_done", 2, 5, 1, 140, "");
			return CS_FAIL;
		}
		
		if (outrow) 
			*outrow = rows_copied;
		
		/* free allocated storage in blkdesc & initialise flags, etc. */
	
		if (blkdesc->bcpinfo.tablename)
			TDS_ZERO_FREE(blkdesc->bcpinfo.tablename);
	
		if (blkdesc->bcpinfo.insert_stmt)
			TDS_ZERO_FREE(blkdesc->bcpinfo.insert_stmt);
	
		if (blkdesc->bcpinfo.bindinfo) {
			tds_free_results(blkdesc->bcpinfo.bindinfo);
			blkdesc->bcpinfo.bindinfo = NULL;
		}
	
		blkdesc->bcpinfo.direction = 0;
		blkdesc->bcpinfo.bind_count = CS_UNUSED;
		blkdesc->bcpinfo.xfer_init = 0;
		blkdesc->bcpinfo.var_cols = 0;

		break;

	}

	return CS_SUCCEED;
}

CS_RETCODE
blk_drop(CS_BLKDESC * blkdesc)
{
	tdsdump_log(TDS_DBG_FUNC, "blk_drop(%p)\n", blkdesc);

	if (!blkdesc)
		return CS_SUCCEED;

	free(blkdesc->bcpinfo.tablename);
	free(blkdesc->bcpinfo.insert_stmt);
	tds_free_results(blkdesc->bcpinfo.bindinfo);
	free(blkdesc);

	return CS_SUCCEED;
}

CS_RETCODE
blk_getrow(SRV_PROC * srvproc, CS_BLKDESC * blkdescp, CS_BLK_ROW * rowp)
{
	tdsdump_log(TDS_DBG_FUNC, "blk_getrow(%p, %p, %p)\n", srvproc, blkdescp, rowp);

	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED blk_getrow()\n");
	return CS_FAIL;
}

CS_RETCODE
blk_gettext(SRV_PROC * srvproc, CS_BLKDESC * blkdescp, CS_BLK_ROW * rowp, CS_INT bufsize, CS_INT * outlenp)
{

	tdsdump_log(TDS_DBG_FUNC, "blk_gettext(%p, %p, %p, %d, %p)\n", srvproc, blkdescp, rowp, bufsize, outlenp);

	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED blk_gettext()\n");
	return CS_FAIL;
}

CS_RETCODE
blk_init(CS_BLKDESC * blkdesc, CS_INT direction, CS_CHAR * tablename, CS_INT tnamelen)
{
	tdsdump_log(TDS_DBG_FUNC, "blk_init(%p, %d, %p, %d)\n", blkdesc, direction, tablename, tnamelen);

	if (!blkdesc) {
		return CS_FAIL;
	}

	if (direction != CS_BLK_IN && direction != CS_BLK_OUT ) {
		_ctclient_msg(blkdesc->con, "blk_init", 2, 6, 1, 138, "");
		return CS_FAIL;
	}

	if (!tablename) {
		_ctclient_msg(blkdesc->con, "blk_init", 2, 6, 1, 139, "");
		return CS_FAIL;
	}
	if (tnamelen == CS_NULLTERM)
		tnamelen = strlen(tablename);

	/* free allocated storage in blkdesc & initialise flags, etc. */

	if (blkdesc->bcpinfo.tablename) {
		tdsdump_log(TDS_DBG_FUNC, "blk_init() freeing tablename\n");
		free(blkdesc->bcpinfo.tablename);
	}

	if (blkdesc->bcpinfo.insert_stmt) {
		tdsdump_log(TDS_DBG_FUNC, "blk_init() freeing insert_stmt\n");
		TDS_ZERO_FREE(blkdesc->bcpinfo.insert_stmt);
	}

	if (blkdesc->bcpinfo.bindinfo) {
		tdsdump_log(TDS_DBG_FUNC, "blk_init() freeing results\n");
		tds_free_results(blkdesc->bcpinfo.bindinfo);
		blkdesc->bcpinfo.bindinfo = NULL;
	}

	/* string can be no-nul terminated so copy with memcpy */
	blkdesc->bcpinfo.tablename = (char *) malloc(tnamelen + 1);
	/* FIXME malloc can fail */
	memcpy(blkdesc->bcpinfo.tablename, tablename, tnamelen);
	blkdesc->bcpinfo.tablename[tnamelen] = 0;

	blkdesc->bcpinfo.direction = direction;
	blkdesc->bcpinfo.bind_count = CS_UNUSED;
	blkdesc->bcpinfo.xfer_init = 0;
	blkdesc->bcpinfo.var_cols = 0;

	if (TDS_FAILED(tds_bcp_init(blkdesc->con->tds_socket, &blkdesc->bcpinfo))) {
		_ctclient_msg(blkdesc->con, "blk_init", 2, 5, 1, 140, "");
		return CS_FAIL;
	}
	blkdesc->bcpinfo.bind_count = CS_UNUSED;

	return CS_SUCCEED;
}

CS_RETCODE
blk_props(CS_BLKDESC * blkdesc, CS_INT action, CS_INT property, CS_VOID * buffer, CS_INT buflen, CS_INT * outlen)
{
	int retval, intval;

	tdsdump_log(TDS_DBG_FUNC, "blk_props(%p, %d, %d, %p, %d, %p)\n", blkdesc, action, property, buffer, buflen, outlen);

	switch (property) {
	case BLK_IDENTITY: 
		switch (action) {
		case CS_SET: 
			if (buffer) {
				memcpy(&intval, buffer, sizeof(intval));
				if (intval == CS_TRUE)
					blkdesc->bcpinfo.identity_insert_on = 1;
				if (intval == CS_FALSE)
					blkdesc->bcpinfo.identity_insert_on = 0;
			}
			return CS_SUCCEED;
			break;
		case CS_GET:
			retval = blkdesc->bcpinfo.identity_insert_on == 1 ? CS_TRUE : CS_FALSE ;
			if (buffer) {
				memcpy (buffer, &retval, sizeof(retval));
				if (outlen)
					*outlen = sizeof(retval);
			}
			return CS_SUCCEED;
			break;
		default:
			_ctclient_msg(blkdesc->con, "blk_props", 2, 5, 1, 141, "%s, %d", "action", action);
			break;
		}
		break;

	default:
		_ctclient_msg(blkdesc->con, "blk_props", 2, 5, 1, 141, "%s, %d", "property", property);
		break;
	}
	return CS_FAIL;
}

CS_RETCODE
blk_rowalloc(SRV_PROC * srvproc, CS_BLK_ROW ** row)
{
	tdsdump_log(TDS_DBG_FUNC, "blk_rowalloc(%p, %p)\n", srvproc, row);

	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED blk_rowalloc()\n");
	return CS_FAIL;
}

CS_RETCODE
blk_rowdrop(SRV_PROC * srvproc, CS_BLK_ROW * row)
{
	tdsdump_log(TDS_DBG_FUNC, "blk_rowdrop(%p, %p)\n", srvproc, row);

	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED blk_rowdrop()\n");
	return CS_FAIL;
}

CS_RETCODE
blk_rowxfer(CS_BLKDESC * blkdesc)
{
	tdsdump_log(TDS_DBG_FUNC, "blk_rowxfer(%p)\n", blkdesc);

	return blk_rowxfer_mult(blkdesc, NULL);
}

CS_RETCODE
blk_rowxfer_mult(CS_BLKDESC * blkdesc, CS_INT * row_count)
{
	CS_INT rows_to_xfer = 0;
	CS_INT rows_xferred = 0;
	CS_RETCODE ret;

	tdsdump_log(TDS_DBG_FUNC, "blk_rowxfer_mult(%p, %p)\n", blkdesc, row_count);

	if (!row_count || *row_count == 0 )
		rows_to_xfer = blkdesc->bcpinfo.bind_count;
	else
		rows_to_xfer = *row_count;

	if (blkdesc->bcpinfo.direction == CS_BLK_IN) {
		ret = _blk_rowxfer_in(blkdesc, rows_to_xfer, &rows_xferred);
	} else {
		ret = _blk_rowxfer_out(blkdesc, rows_to_xfer, &rows_xferred);
	}
	if (row_count)
		*row_count = rows_xferred;
	return ret;

}

CS_RETCODE
blk_sendrow(CS_BLKDESC * blkdesc, CS_BLK_ROW * row)
{

	tdsdump_log(TDS_DBG_FUNC, "blk_sendrow(%p, %p)\n", blkdesc, row);

	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED blk_sendrow()\n");
	return CS_FAIL;
}

CS_RETCODE
blk_sendtext(CS_BLKDESC * blkdesc, CS_BLK_ROW * row, CS_BYTE * buffer, CS_INT buflen)
{
	tdsdump_log(TDS_DBG_FUNC, "blk_sendtext(%p, %p, %p, %d)\n", blkdesc, row, buffer, buflen);

	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED blk_sendtext()\n");
	return CS_FAIL;
}

CS_RETCODE
blk_srvinit(SRV_PROC * srvproc, CS_BLKDESC * blkdescp)
{
	tdsdump_log(TDS_DBG_FUNC, "blk_srvinit(%p, %p)\n", srvproc, blkdescp);

	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED blk_srvinit()\n");
	return CS_FAIL;
}

CS_RETCODE
blk_textxfer(CS_BLKDESC * blkdesc, CS_BYTE * buffer, CS_INT buflen, CS_INT * outlen)
{
	tdsdump_log(TDS_DBG_FUNC, "blk_textxfer(%p, %p, %d, %p)\n", blkdesc, buffer, buflen, outlen);

	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED blk_textxfer()\n");
	return CS_FAIL;
}

static CS_RETCODE
_blk_rowxfer_out(CS_BLKDESC * blkdesc, CS_INT rows_to_xfer, CS_INT * rows_xferred)
{
	TDSSOCKET *tds;
	TDS_INT result_type;
	TDSRET ret;
	TDS_INT temp_count;

	tdsdump_log(TDS_DBG_FUNC, "_blk_rowxfer_out(%p, %d, %p)\n", blkdesc, rows_to_xfer, rows_xferred);

	if (!blkdesc || !blkdesc->con)
		return CS_FAIL;

	tds = blkdesc->con->tds_socket;

	/*
	 * the first time blk_xfer called after blk_init()
	 * do the query and get to the row data...
	 */

	if (blkdesc->bcpinfo.xfer_init == 0) {

		if (TDS_FAILED(tds_submit_queryf(tds, "select * from %s", blkdesc->bcpinfo.tablename))) {
			_ctclient_msg(blkdesc->con, "blk_rowxfer", 2, 5, 1, 140, "");
			return CS_FAIL;
		}

		while ((ret = tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_RESULTS)) == TDS_SUCCESS) {
			if (result_type == TDS_ROW_RESULT)
				break;
		}
	
		if (ret != TDS_SUCCESS || result_type != TDS_ROW_RESULT) {
			_ctclient_msg(blkdesc->con, "blk_rowxfer", 2, 5, 1, 140, "");
			return CS_FAIL;
		}

		blkdesc->bcpinfo.xfer_init = 1;
	}

	if (rows_xferred)
		*rows_xferred = 0;

	for (temp_count = 0; temp_count < rows_to_xfer; temp_count++) {

		ret = tds_process_tokens(tds, &result_type, NULL, TDS_STOPAT_ROWFMT|TDS_STOPAT_DONE|TDS_RETURN_ROW|TDS_RETURN_COMPUTE);

		tdsdump_log(TDS_DBG_FUNC, "blk_rowxfer_out() process_row_tokens returned %d\n", ret);

		switch (ret) {
		case TDS_SUCCESS:
			if (result_type == TDS_ROW_RESULT || result_type == TDS_COMPUTE_RESULT) {
				if (result_type == TDS_ROW_RESULT) {
					if (_ct_bind_data( blkdesc->con->ctx, tds->current_results, blkdesc->bcpinfo.bindinfo, temp_count))
						return CS_ROW_FAIL;
					if (rows_xferred)
						*rows_xferred = *rows_xferred + 1;
				}
				break;
			}
		case TDS_NO_MORE_RESULTS: 
			return CS_END_DATA;
			break;

		default:
			_ctclient_msg(blkdesc->con, "blk_rowxfer", 2, 5, 1, 140, "");
			return CS_FAIL;
			break;
		}
	} 

	return CS_SUCCEED;
}

static CS_RETCODE
_blk_rowxfer_in(CS_BLKDESC * blkdesc, CS_INT rows_to_xfer, CS_INT * rows_xferred)
{
	TDSSOCKET *tds;
	TDS_INT each_row;

	tdsdump_log(TDS_DBG_FUNC, "_blk_rowxfer_in(%p, %d, %p)\n", blkdesc, rows_to_xfer, rows_xferred);

	if (!blkdesc)
		return CS_FAIL;

	tds = blkdesc->con->tds_socket;

	/*
	 * the first time blk_xfer called after blk_init()
	 * do the query and get to the row data...
	 */

	if (blkdesc->bcpinfo.xfer_init == 0) {

		/*
		 * first call the start_copy function, which will
		 * retrieve details of the database table columns
		 */

		if (TDS_FAILED(tds_bcp_start_copy_in(tds, &blkdesc->bcpinfo))) {
			_ctclient_msg(blkdesc->con, "blk_rowxfer", 2, 5, 1, 140, "");
			return CS_FAIL;
		}

		blkdesc->bcpinfo.xfer_init = 1;
	} 

	blkdesc->bcpinfo.parent = blkdesc;
	for (each_row = 0; each_row < rows_to_xfer; each_row++ ) {

		if (tds_bcp_send_record(tds, &blkdesc->bcpinfo, _blk_get_col_data, _blk_null_error, each_row) == TDS_SUCCESS) {
			/* FIXME */
		}
	}

	return CS_SUCCEED;
}

static void
_blk_null_error(TDSBCPINFO *bcpinfo, int index, int offset)
{
	CS_BLKDESC *blkdesc = (CS_BLKDESC *) bcpinfo->parent;

	tdsdump_log(TDS_DBG_FUNC, "_blk_null_error(%p, %d, %d)\n", bcpinfo, index, offset);

	_ctclient_msg(blkdesc->con, "blk_rowxfer", 2, 7, 1, 142, "%d, %d",  index + 1, offset + 1);
}

static TDSRET
_blk_get_col_data(TDSBCPINFO *bulk, TDSCOLUMN *bindcol, int offset)
{
	int result = 0;
	CS_INT null_column = 0;
	unsigned char *src = NULL;

	CS_INT      srctype = 0;
	CS_INT      srclen  = 0;
	CS_INT      destlen  = 0;
	CS_SMALLINT *nullind = NULL;
	CS_INT      *datalen = NULL;
	CS_BLKDESC *blkdesc = (CS_BLKDESC *) bulk->parent;
	CS_CONTEXT *ctx = blkdesc->con->ctx;
	CS_DATAFMT srcfmt, destfmt;

	tdsdump_log(TDS_DBG_FUNC, "_blk_get_col_data(%p, %p, %d)\n", bulk, bindcol, offset);

	/*
	 * Retrieve the initial bound column_varaddress
	 * and increment it if offset specified
	 */

	src = (unsigned char *) bindcol->column_varaddr;
	src += offset * bindcol->column_bindlen;
	
	if (bindcol->column_nullbind) {
		nullind = bindcol->column_nullbind;
		nullind += offset;
	}
	if (bindcol->column_lenbind) {
		datalen = bindcol->column_lenbind;
		datalen += offset;
	}

	if (src) {
		srctype = bindcol->column_bindtype; 		/* passes to cs_convert */

		tdsdump_log(TDS_DBG_INFO1, "blk_get_col_data srctype = %d \n", srctype);
		tdsdump_log(TDS_DBG_INFO1, "blk_get_col_data datalen = %d \n", *datalen);
	
		if (*datalen) {
			if (*datalen == CS_UNUSED) {
				switch (srctype) {
					case CS_LONG_TYPE:	    srclen = 8; break;
					case CS_FLOAT_TYPE:	    srclen = 8; break;
					case CS_MONEY_TYPE:	    srclen = 8; break;
					case CS_DATETIME_TYPE:  srclen = 8; break;
					case CS_INT_TYPE:	    srclen = 4; break;
					case CS_REAL_TYPE:	    srclen = 4; break;
					case CS_MONEY4_TYPE:	srclen = 4; break;
					case CS_DATETIME4_TYPE: srclen = 4; break;
					case CS_SMALLINT_TYPE:  srclen = 2; break;
					case CS_TINYINT_TYPE:   srclen = 1; break;
					default:
						printf("error not fixed length type (%d) and datalen not specified\n",
							bindcol->column_bindtype);
						return CS_FAIL;
				}

			} else {
				srclen = *datalen;
			}
		}
		if (srclen == 0) {
			if (nullind && *nullind == -1)
				null_column = 1;
		}

		if (!null_column) {

			srcfmt.datatype = srctype;
			srcfmt.maxlength = srclen;

			destfmt.datatype  = _ct_get_client_type(bindcol);
			destfmt.maxlength = bindcol->column_size;
			destfmt.precision = bindcol->column_prec;
			destfmt.scale     = bindcol->column_scale;

			destfmt.format	= CS_FMT_UNUSED;
	
			/* if convert return FAIL mark error but process other columns */
			if ((result = cs_convert(ctx, &srcfmt, (CS_VOID *) src, 
						 &destfmt, (CS_VOID *) bindcol->bcp_column_data->data, &destlen)) != CS_SUCCEED) {
				tdsdump_log(TDS_DBG_INFO1, "convert failed for %d \n", srcfmt.datatype);
				return CS_FAIL;
			}
		}

		bindcol->bcp_column_data->datalen = destlen;
		bindcol->bcp_column_data->is_null = null_column;

		return TDS_SUCCESS;
	} else {
		printf("error source field not addressable \n");
		return TDS_FAIL;
	}
}
