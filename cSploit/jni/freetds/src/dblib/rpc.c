/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001  Brian Bruns
 * Copyright (C) 2002, 2003, 2004, 2005    James K. Lowden
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_ERRNO_H
# include <errno.h>
#endif /* HAVE_ERRNO_H */

#include <assert.h>

#include <freetds/tds.h>
#include <freetds/convert.h>
#include <replacements.h>
#include <sybfront.h>
#include <sybdb.h>
#include <dblib.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: rpc.c,v 1.77 2011-09-25 11:31:41 freddy77 Exp $");

static void rpc_clear(DBREMOTE_PROC * rpc);
static void param_clear(DBREMOTE_PROC_PARAM * pparam);

static TDSPARAMINFO *param_info_alloc(TDSSOCKET * tds, DBREMOTE_PROC * rpc);

/**
 * \ingroup dblib_rpc
 * \brief Initialize a remote procedure call. 
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param rpcname name of the stored procedure to be run.  
 * \param options Only supported option would be DBRPCRECOMPILE, 
 * which causes the stored procedure to be recompiled before executing.
 * \remark The RPC functions are the only way to get back OUTPUT parameter data with db-lib 
 * from modern Microsoft servers.  
 * \retval SUCCEED normal.
 * \retval FAIL on error
 * \sa dbrpcparam(), dbrpcsend()
 */
RETCODE
dbrpcinit(DBPROCESS * dbproc, const char rpcname[], DBSMALLINT options)
{
	DBREMOTE_PROC **rpc;

	tdsdump_log(TDS_DBG_FUNC, "dbrpcinit(%p, %s, %d)\n", dbproc, rpcname, options);
	CHECK_CONN(FAIL);
	CHECK_NULP(rpcname, "dbrpcinit", 2, FAIL);

	/*
	 * TODO: adhere to docs.  Only Microsoft supports DBRPCRESET.  They say:
	 * "Cancels a single stored procedure or a batch of stored procedures. 
	 *  If rpcname is specified, that new stored procedure is initialized after the cancellation is complete."
	 */
	if (options & DBRPCRESET) {
		rpc_clear(dbproc->rpc);
		dbproc->rpc = NULL;
		return SUCCEED;
	}

	/* any bits we want from the options argument */
	/* dbrpcrecompile = options & DBRPCRECOMPILE; */
	options &= ~DBRPCRECOMPILE;	/* turn that one off, now that we've extracted it */

	/* all other options except DBRPCRECOMPILE are invalid */
	DBPERROR_RETURN3(options, SYBEIPV, (int) options, "options", "dbrpcinit");

	/* find a free node */
	for (rpc = &dbproc->rpc; *rpc != NULL; rpc = &(*rpc)->next) {
		/* check existing nodes for name match (there shouldn't be one) */
		if ((*rpc)->name == NULL  || strcmp((*rpc)->name, rpcname) == 0) {
			tdsdump_log(TDS_DBG_INFO1, "error: dbrpcinit called twice for procedure \"%s\"\n", rpcname);
			return FAIL; 
		}
	}

	/* rpc now contains the address of the dbproc's first empty (null) DBREMOTE_PROC* */

	/* allocate */
	if ((*rpc = (DBREMOTE_PROC *) calloc(1, sizeof(DBREMOTE_PROC))) == NULL) {
		dbperror(dbproc, SYBEMEM, errno);
		return FAIL;
	}

	if (((*rpc)->name = strdup(rpcname)) == NULL) {
		free(*rpc);
		*rpc = NULL;
		dbperror(dbproc, SYBEMEM, errno);
		return FAIL;
	}

	/* store */
	(*rpc)->options = options & DBRPCRECOMPILE;
	(*rpc)->param_list = NULL;

	/* completed */
	tdsdump_log(TDS_DBG_INFO1, "dbrpcinit() added rpcname \"%s\"\n", rpcname);

	return SUCCEED;
}

/**
 * \ingroup dblib_rpc
 * \brief Add a parameter to a remote procedure call.
 *
 * Call between dbrpcinit() and dbrpcsend()
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param paramname literal name of the parameter, according to the stored procedure (starts with '@').  Optional.  
 *        If not used, parameters will be passed in order instead of by name. 
 * \param status must be DBRPCRETURN, if this parameter is a return parameter, else 0. 
 * \param type datatype of the value parameter e.g., SYBINT4, SYBCHAR.
 * \param maxlen Maximum output size of the parameter's value to be returned by the stored procedure, 
 *  	  usually the size of your host variable. 
 *        Fixed-length datatypes take -1 (NULL or not).  
 *        Non-OUTPUT parameters also use -1.  
 *	   Use 0 to send a NULL value for a variable length datatype.  
 * \param datalen For variable-length datatypes, the byte size of the data to be sent, exclusive of any null terminator. 
 *        For fixed-length datatypes use -1.  To send a NULL value, use 0.  
 * \param value Address of your host variable.  
 * \retval SUCCEED normal.
 * \retval FAIL on error
 * \sa dbrpcinit(), dbrpcsend()
 */
RETCODE
dbrpcparam(DBPROCESS * dbproc, const char paramname[], BYTE status, int type, DBINT maxlen, DBINT datalen, BYTE * value)
{
	char *name = NULL;
	DBREMOTE_PROC *rpc;
	DBREMOTE_PROC_PARAM **pparam;
	DBREMOTE_PROC_PARAM *param;

	tdsdump_log(TDS_DBG_FUNC, "dbrpcparam(%p, %s, 0x%x, %d, %d, %d, %p)\n", 
				   dbproc, paramname, status, type, maxlen, datalen, value);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->rpc, SYBERPCS, FAIL);

	if (type == SYBVARCHAR && IS_TDS7_PLUS(dbproc->tds_socket->conn))
		type = XSYBNVARCHAR;

	/* validate datalen parameter */

	if (is_fixed_type(type)) {
		if (datalen != 0)
			datalen = -1; 
	} else {	/* Sybooks: "Passing datalen as -1 for any of these [non-fixed] datatypes results 
			 * in the DBPROCESS referenced by dbproc being marked as "dead," or unusable."
			 */
		DBPERROR_RETURN(datalen < 0, SYBERPIL);
	}

	/* "value parameter for dbprcparam() can be NULL, only if the datalen parameter is 0." */
	DBPERROR_RETURN(value == NULL && datalen != 0, SYBERPNULL);
	
	/* nullable types most provide a data length */
	DBPERROR_RETURN(is_nullable_type(type) && datalen < 0, SYBERPUL);

	/* validate maxlen parameter */

	if (status & DBRPCRETURN) {
		if (is_fixed_type(type)) {
			maxlen = -1;
		} else {
			if (maxlen == -1)
				maxlen = 255;
		}
	} else {
		/*
		 * Well, maxlen should be used only for output parameter however it seems
		 * that ms implementation wrongly require this 0 for NULL variable
		 * input parameters, so fix it
		 */
		DBPERROR_RETURN3(maxlen != -1 && maxlen != 0, SYBEIPV, (int) maxlen, "maxlen", "dbrpcparam");
		maxlen = -1;
	}
	
	/* end validation */

	/* allocate */
	param = (DBREMOTE_PROC_PARAM *) malloc(sizeof(DBREMOTE_PROC_PARAM));
	if (param == NULL) {
		dbperror(dbproc, SYBEMEM, 0);
		return FAIL;
	}

	if (paramname) {
		name = strdup(paramname);
		if (name == NULL) {
			free(param);
			dbperror(dbproc, SYBEMEM, 0);
			return FAIL;
		}
	}

	/* initialize */
	param->next = NULL;	/* NULL signifies end of linked list */
	param->name = name;
	param->status = status;
	param->type = type;
	param->maxlen = maxlen;
	param->datalen = datalen;

	/*
	 * If datalen = 0, value parameter is ignored.
	 * This is one way to specify a NULL input parameter. 
	 */
	if (datalen == 0)
		param->value = NULL;
	else
		param->value = value;

	/*
	 * Add a parameter to the current rpc.  
	 * 
	 * Traverse the dbproc's procedure list to find the current rpc, 
	 * then traverse the parameter linked list until its end,
	 * then tack on our parameter's address.  
	 */
	for (rpc = dbproc->rpc; rpc->next != NULL; rpc = rpc->next)	/* find "current" procedure */
		;
	for (pparam = &rpc->param_list; *pparam != NULL; pparam = &(*pparam)->next);

	/* pparam now contains the address of the end of the rpc's parameter list */

	*pparam = param;	/* add to the end of the list */

	tdsdump_log(TDS_DBG_INFO1, "dbrpcparam() added parameter \"%s\"\n", (paramname) ? paramname : "");

	return SUCCEED;
}

/**
 * \ingroup dblib_rpc
 * \brief Execute the procedure and free associated memory
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \retval SUCCEED normal.
 * \retval FAIL on error
 * \sa dbrpcinit(), dbrpcparam()
 */
RETCODE
dbrpcsend(DBPROCESS * dbproc)
{
	DBREMOTE_PROC *rpc;

	tdsdump_log(TDS_DBG_FUNC, "dbrpcsend(%p)\n", dbproc);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->rpc, SYBERPCS, FAIL);	/* dbrpcinit should allocate pointer */

	/* sanity */
	if (dbproc->rpc->name == NULL) {	/* can't be ready without a name */
		tdsdump_log(TDS_DBG_INFO1, "returning FAIL: name is NULL\n");
		return FAIL;
	}

	dbproc->dbresults_state = _DB_RES_INIT;

	for (rpc = dbproc->rpc; rpc != NULL; rpc = rpc->next) {
		TDSRET erc;
		TDSPARAMINFO *pparam_info = NULL;

		/*
		 * liam@inodes.org: allow stored procedures to have no paramaters 
		 */
		if (rpc->param_list != NULL) {
			pparam_info = param_info_alloc(dbproc->tds_socket, rpc);
			if (!pparam_info)
				return FAIL;
		}
		erc = tds_submit_rpc(dbproc->tds_socket, dbproc->rpc->name, pparam_info, NULL);
		tds_free_param_results(pparam_info);
		if (erc == TDS_FAIL) {
			tdsdump_log(TDS_DBG_INFO1, "returning FAIL: tds_submit_rpc() failed\n");
			return FAIL;
		}
	}

	/* free up the memory */
	rpc_clear(dbproc->rpc);
	dbproc->rpc = NULL;

	tdsdump_log(TDS_DBG_FUNC, "dbrpcsend() returning SUCCEED\n");

	return SUCCEED;
}

/** 
 * Tell the TDSPARAMINFO structure where the data go.  This is a kind of "bind" operation.
 */
static const unsigned char *
param_row_alloc(TDSPARAMINFO * params, TDSCOLUMN * curcol, int param_num, void *value, int size)
{
	const void *row = tds_alloc_param_data(curcol);
	tdsdump_log(TDS_DBG_INFO1, "parameter size = %d, data = %p, row_size = %d\n",
				   size, curcol->column_data, params->row_size);
	if (!row)
		return NULL;
	if (size > 0 && value) {
		tdsdump_log(TDS_DBG_FUNC, "copying %d bytes of data to parameter #%d\n", size, param_num);
		if (!is_blob_col(curcol)) {
			if (is_numeric_type(curcol->column_type))
				memset(curcol->column_data, 0, sizeof(TDS_NUMERIC));
			memcpy(curcol->column_data, value, size);
		} else {
			TDSBLOB *blob = (TDSBLOB *) curcol->column_data;
			blob->textvalue = (TDS_CHAR*) malloc(size);
			tdsdump_log(TDS_DBG_FUNC, "blob parameter supported, size %d textvalue pointer is %p\n", 
						  size, blob->textvalue);
			if (!blob->textvalue)
				return NULL;
			memcpy(blob->textvalue, value, size);
		}
	}
	else {
		tdsdump_log(TDS_DBG_FUNC, "setting parameter #%d to NULL\n", param_num);
		curcol->column_cur_size = -1;
	}

	return (const unsigned char*) row;
}

/** 
 * Allocate memory and copy the rpc information into a TDSPARAMINFO structure.
 */
static TDSPARAMINFO *
param_info_alloc(TDSSOCKET * tds, DBREMOTE_PROC * rpc)
{
	int i;
	DBREMOTE_PROC_PARAM *p;
	TDSCOLUMN *pcol;
	TDSPARAMINFO *params = NULL, *new_params;
	BYTE *temp_value;
	int  temp_datalen;
	int  temp_type;
	int  param_is_null;

	/* sanity */
	if (rpc == NULL)
		return NULL;

	/* see v 1.10 2002/11/23 for first broken attempt */

	for (i = 0, p = rpc->param_list; p != NULL; p = p->next, i++) {
		const unsigned char *prow;

		if (!(new_params = tds_alloc_param_result(params))) {
			tds_free_param_results(params);
			tdsdump_log(TDS_DBG_ERROR, "out of rpc memory!");
			return NULL;
		}
		params = new_params;

		/*
		 * Determine whether an input parameter is NULL or not.
		 */
		param_is_null = 0;
		temp_type = p->type;
		temp_value = p->value;
		temp_datalen = p->datalen;

		if (p->datalen == 0)
			param_is_null = 1; 

		tdsdump_log(TDS_DBG_INFO1, "parm_info_alloc(): parameter null-ness = %d\n", param_is_null);

		pcol = params->columns[i];

		if (temp_value && is_numeric_type(temp_type)) {
			DBDECIMAL *dec = (DBDECIMAL*) temp_value;
			pcol->column_prec = dec->precision;
			pcol->column_scale = dec->scale;
			if (dec->precision > 0 && dec->precision <= MAXPRECISION)
				temp_datalen = tds_numeric_bytes_per_prec[dec->precision] + 2;
		}
		if (param_is_null || (p->status & DBRPCRETURN)) {
			if (param_is_null) {
				temp_datalen = 0;
				temp_value = NULL;
			} else if (is_fixed_type(temp_type)) {
				temp_datalen = tds_get_size_by_type(temp_type);
			}
			temp_type = tds_get_null_type(temp_type);
		} else if (is_fixed_type(temp_type)) {
			temp_datalen = tds_get_size_by_type(temp_type);
		}

		/* meta data */
		if (p->name) {
			tds_strlcpy(pcol->column_name, p->name, sizeof(pcol->column_name));
			pcol->column_namelen = (int)strlen(pcol->column_name);
		}

		tds_set_param_type(tds->conn, pcol, temp_type);

		if (p->maxlen > 0)
			pcol->column_size = p->maxlen;
		else {
			if (is_fixed_type(p->type)) {
				pcol->column_size = tds_get_size_by_type(p->type);
			} else {
				pcol->column_size = p->datalen;
			}
		}
		if (p->type == XSYBNVARCHAR)
			pcol->column_size *= 2;
		pcol->on_server.column_size = pcol->column_size;

		pcol->column_output = p->status;
		pcol->column_cur_size = temp_datalen;

		prow = param_row_alloc(params, pcol, i, temp_value, temp_datalen);

		if (!prow) {
			tds_free_param_results(params);
			tdsdump_log(TDS_DBG_ERROR, "out of memory for rpc row!");
			return NULL;
		}

	}

	return params;

}

/**
 * erase the procedure list
 */
static void
rpc_clear(DBREMOTE_PROC * rpc)
{
	DBREMOTE_PROC * next;

	while (rpc) {
		next = rpc->next;
		param_clear(rpc->param_list);
		free(rpc->name);
		free(rpc);
		rpc = next;
	}
}

/**
 * erase the parameter list
 */
static void
param_clear(DBREMOTE_PROC_PARAM * pparam)
{
	DBREMOTE_PROC_PARAM * next;

	while (pparam) {
		next = pparam->next;
		free(pparam->name);
		/* free self */
		free(pparam);
		pparam = next;
	}
}
