/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2005-2014  Frediano Ziglio
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
 * \brief Contains all routines to get replies from server
 */
#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <assert.h>

#include <freetds/tds.h>
#include <freetds/string.h>
#include <freetds/convert.h>
#include <freetds/iconv.h>
#include "tds_checks.h"
#include "replacements.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

/** \cond HIDDEN_SYMBOLS */
#define USE_ICONV tds_conn(tds)->use_iconv
/** \endcond */

static TDSRET tds_process_msg(TDSSOCKET * tds, int marker);
static TDSRET tds_process_compute_result(TDSSOCKET * tds);
static TDSRET tds_process_compute_names(TDSSOCKET * tds);
static TDSRET tds7_process_compute_result(TDSSOCKET * tds);
static TDSRET tds_process_result(TDSSOCKET * tds);
static TDSRET tds_process_col_name(TDSSOCKET * tds);
static TDSRET tds_process_col_fmt(TDSSOCKET * tds);
static TDSRET tds_process_tabname(TDSSOCKET *tds);
static TDSRET tds_process_colinfo(TDSSOCKET * tds, char **names, int num_names);
static TDSRET tds_process_compute(TDSSOCKET * tds, TDS_INT * computeid);
static TDSRET tds_process_cursor_tokens(TDSSOCKET * tds);
static TDSRET tds_process_row(TDSSOCKET * tds);
static TDSRET tds_process_nbcrow(TDSSOCKET * tds);
static TDSRET tds_process_param_result(TDSSOCKET * tds, TDSPARAMINFO ** info);
static TDSRET tds7_process_result(TDSSOCKET * tds);
static TDSDYNAMIC *tds_process_dynamic(TDSSOCKET * tds);
static TDSRET tds_process_auth(TDSSOCKET * tds);
static TDSRET tds_process_env_chg(TDSSOCKET * tds);
static TDSRET tds_process_param_result_tokens(TDSSOCKET * tds);
static TDSRET tds_process_params_result_token(TDSSOCKET * tds);
static TDSRET tds_process_dyn_result(TDSSOCKET * tds);
static TDSRET tds5_process_result(TDSSOCKET * tds);
static TDSRET tds5_process_dyn_result2(TDSSOCKET * tds);
static TDSRET tds_process_default_tokens(TDSSOCKET * tds, int marker);
static TDSRET tds5_process_optioncmd(TDSSOCKET * tds);
static TDSRET tds_process_end(TDSSOCKET * tds, int marker, /*@out@*/ int *flags_parm);

static TDSRET tds_get_data_info(TDSSOCKET * tds, TDSCOLUMN * curcol, int is_param);
static /*@observer@*/ const char *tds_token_name(unsigned char marker);
static void adjust_character_column_size(TDSSOCKET * tds, TDSCOLUMN * curcol);
int determine_adjusted_size(const TDSICONV * char_conv, int size);
static /*@observer@*/ const char *tds_pr_op(int op);
static int tds_alloc_get_string(TDSSOCKET * tds, /*@special@*/ char **string, size_t len) /*allocates *string*/;

/**
 * \ingroup libtds
 * \defgroup token Results processing
 * Handle tokens in packets. Many PDU (packets data unit) contain tokens.
 * (like result description, rows, data, errors and many other).
 */


/**
 * \addtogroup token
 * @{ 
 */

/**
 * tds_process_default_tokens() is a catch all function that is called to
 * process tokens not known to other tds_process_* routines
 * @tds
 * @param marker Token type
 */
static TDSRET
tds_process_default_tokens(TDSSOCKET * tds, int marker)
{
	int tok_size;
	int done_flags;
	TDS_INT ret_status;
	TDS_CAPABILITY_TYPE *cap;

	CHECK_TDS_EXTRA(tds);

	tdsdump_log(TDS_DBG_FUNC, "tds_process_default_tokens() marker is %x(%s)\n", marker, tds_token_name(marker));

	if (IS_TDSDEAD(tds)) {
		tdsdump_log(TDS_DBG_FUNC, "leaving tds_process_default_tokens() connection dead\n");
		tds_close_socket(tds);
		return TDS_FAIL;
	}

	switch (marker) {
	case TDS_AUTH_TOKEN:
		return tds_process_auth(tds);
		break;
	case TDS_ENVCHANGE_TOKEN:
		return tds_process_env_chg(tds);
		break;
	case TDS_DONE_TOKEN:
	case TDS_DONEPROC_TOKEN:
	case TDS_DONEINPROC_TOKEN:
		return tds_process_end(tds, marker, &done_flags);
		break;
	case TDS_PROCID_TOKEN:
		tds_get_n(tds, NULL, 8);
		break;
	case TDS_RETURNSTATUS_TOKEN:
		ret_status = tds_get_int(tds);
		marker = tds_peek(tds);
		if (marker != TDS_PARAM_TOKEN && marker != TDS_DONEPROC_TOKEN && marker != TDS_DONE_TOKEN)
			break;
		tds->has_status = 1;
		tds->ret_status = ret_status;
		tdsdump_log(TDS_DBG_FUNC, "tds_process_default_tokens: return status is %d\n", tds->ret_status);
		break;
	case TDS_ERROR_TOKEN:
	case TDS_INFO_TOKEN:
	case TDS_EED_TOKEN:
		return tds_process_msg(tds, marker);
		break;
	case TDS_CAPABILITY_TOKEN:
		tok_size = tds_get_usmallint(tds);
		cap = tds_conn(tds)->capabilities.types;
		memset(cap, 0, 2*sizeof(*cap));
		cap[0].type = 1;
		cap[0].len = sizeof(cap[0].values);
		cap[1].type = 2;
		cap[1].len = sizeof(cap[1].values);
		while (tok_size > 1) {
			unsigned char type, size, *p;

			type = tds_get_byte(tds);
			size = tds_get_byte(tds);
			tok_size -= 2 + size;
			if (type != 1 && type != 2) {
				tds_get_n(tds, NULL, size);
				continue;
			}
			if (size > sizeof(cap->values)) {
				tds_get_n(tds, NULL, size - sizeof(cap->values));
				size = sizeof(cap->values);
			}
			p = (unsigned char *) &cap[type];
			if (tds_get_n(tds, p-size, size) == NULL)
				return TDS_FAIL;
			/*
			 * Sybase 11.0 servers return the wrong length in the capability packet,
			 * causing us to read past the done packet.
			 */
			if (tds_conn(tds)->product_version < TDS_SYB_VER(12, 0, 0) && type == 2)
				break;
		}
		break;
		/* PARAM_TOKEN can be returned inserting text in db, to return new timestamp */
	case TDS_PARAM_TOKEN:
		tds_unget_byte(tds);
		return tds_process_param_result_tokens(tds);
		break;
	case TDS7_RESULT_TOKEN:
		return tds7_process_result(tds);
		break;
	case TDS_OPTIONCMD_TOKEN:
		return tds5_process_optioncmd(tds);
		break;
	case TDS_RESULT_TOKEN:
		return tds_process_result(tds);
		break;
	case TDS_ROWFMT2_TOKEN:
		return tds5_process_result(tds);
		break;
	case TDS_COLNAME_TOKEN:
		return tds_process_col_name(tds);
		break;
	case TDS_COLFMT_TOKEN:
		return tds_process_col_fmt(tds);
		break;
	case TDS_ROW_TOKEN:
		return tds_process_row(tds);
		break;
	case TDS5_PARAMFMT_TOKEN:
		/* store discarded parameters in param_info, not in old dynamic */
		tds_release_cur_dyn(tds);
		return tds_process_dyn_result(tds);
		break;
	case TDS5_PARAMFMT2_TOKEN:
		tds_release_cur_dyn(tds);
		return tds5_process_dyn_result2(tds);
		break;
	case TDS5_PARAMS_TOKEN:
		/* save params */
		return tds_process_params_result_token(tds);
		break;
	case TDS_CURINFO_TOKEN:
		return tds_process_cursor_tokens(tds);
		break;
	case TDS5_DYNAMIC_TOKEN:
	case TDS_LOGINACK_TOKEN:
	case TDS_ORDERBY_TOKEN:
	case TDS_CONTROL_TOKEN:
		tdsdump_log(TDS_DBG_WARN, "Eating %s token\n", tds_token_name(marker));
		tds_get_n(tds, NULL, tds_get_usmallint(tds));
		break;
	case TDS_TABNAME_TOKEN:	/* used for FOR BROWSE query */
		return tds_process_tabname(tds);
		break;
	case TDS_COLINFO_TOKEN:
		return tds_process_colinfo(tds, NULL, 0);
		break;
	case TDS_ORDERBY2_TOKEN:
		tdsdump_log(TDS_DBG_WARN, "Eating %s token\n", tds_token_name(marker));
		tds_get_n(tds, NULL, tds_get_uint(tds));
		break;
	case TDS_NBC_ROW_TOKEN:
		return tds_process_nbcrow(tds);
		break;
	default: 
		tds_close_socket(tds);
		tdserror(tds_get_ctx(tds), tds, TDSEBTOK, 0);
		tdsdump_log(TDS_DBG_ERROR, "Unknown marker: %d(%x)!!\n", marker, (unsigned char) marker);
		return TDS_FAIL;
	}
	return TDS_SUCCESS;
}

/**
 * Retrieve and set @@spid
 * \tds
 */
static TDSRET
tds_set_spid(TDSSOCKET * tds)
{
	TDS_INT result_type;
	TDS_INT done_flags;
	TDSRET rc;
	TDSCOLUMN *curcol;

	CHECK_TDS_EXTRA(tds);

	if (TDS_FAILED(rc=tds_submit_query(tds, "select @@spid")))
		return rc;

	while ((rc = tds_process_tokens(tds, &result_type, &done_flags, TDS_RETURN_ROWFMT|TDS_RETURN_ROW|TDS_RETURN_DONE)) == TDS_SUCCESS) {

		switch (result_type) {

			case TDS_ROWFMT_RESULT:
				if (tds->res_info->num_cols != 1) 
					return TDS_FAIL;
				break;

			case TDS_ROW_RESULT:
				curcol = tds->res_info->columns[0];
				if (curcol->column_type == SYBINT2 || (curcol->column_type == SYBINTN && curcol->column_size == 2)) {
					tds->spid = *((TDS_USMALLINT *) curcol->column_data);
				} else if (curcol->column_type == SYBINT4 || (curcol->column_type == SYBINTN && curcol->column_size == 4)) {
					tds->spid = *((TDS_UINT *) curcol->column_data);
				} else
					return TDS_FAIL;
				break;

			case TDS_DONE_RESULT:
				if ((done_flags & TDS_DONE_ERROR) != 0)
					return TDS_FAIL;
				break;

			default:
				break;
		}
	}
	if (rc == TDS_NO_MORE_RESULTS)
		rc = TDS_SUCCESS;

	return rc;
}

/**
 * tds_process_login_tokens() is called after sending the login packet 
 * to the server.  It returns the success or failure of the login 
 * dependent on the protocol version. 4.2 sends an ACK token only when
 * successful, TDS 5.0 sends it always with a success byte within
 * @tds
 */
TDSRET
tds_process_login_tokens(TDSSOCKET * tds)
{
	TDSRET succeed = TDS_FAIL;
	int marker;
	unsigned int len;
	int memrc = 0;
	unsigned char ack;
	TDS_UINT product_version;

	CHECK_TDS_EXTRA(tds);

	tdsdump_log(TDS_DBG_FUNC, "tds_process_login_tokens()\n");
	/* get_incoming(tds->s); */
	do {
		struct 	{ unsigned char major, minor, tiny[2]; 
			  unsigned int reported; 
			  const char *name;
			} ver;
		
		marker = tds_get_byte(tds);
		tdsdump_log(TDS_DBG_FUNC, "looking for login token, got  %x(%s)\n", marker, tds_token_name(marker));

		switch (marker) {
		case TDS_LOGINACK_TOKEN:
			/* TODO function */
			tds_conn(tds)->tds71rev1 = 0;
			len = tds_get_usmallint(tds);
			if (len < 10)
				return TDS_FAIL;
			ack = tds_get_byte(tds);
			
			ver.major = tds_get_byte(tds);
			ver.minor = tds_get_byte(tds);
			ver.tiny[0] = tds_get_byte(tds);
			ver.tiny[1] = tds_get_byte(tds);
			ver.reported = (ver.major << 24) | (ver.minor << 16) | (ver.tiny[0] << 8) | ver.tiny[1];

			if (ver.reported == 0x07010000)
				tds_conn(tds)->tds71rev1 = 1;

			/* Log reported server product name, cf. MS-TDS LOGINACK documentation. */
			switch(ver.reported) {
			case 0x07000000: 
				ver.name = "7.0"; break;
			case 0x07010000: 
				ver.name = "2000"; break;
			case 0x71000001: 
				ver.name = "2000 SP1"; break;
			case 0x72090002: 
				ver.name = "2005"; break;
			case 0x730A0003: 
				ver.name = "2008 (no NBCROW or fSparseColumnSet)"; break;
			case 0x730B0003: 
				ver.name = "2008"; break;
			default:
				ver.name = "unknown"; break;
			}
			
			tdsdump_log(TDS_DBG_FUNC, "server reports TDS version %x.%x.%x.%x\n", 
							ver.major, ver.minor, ver.tiny[0], ver.tiny[1]);
			tdsdump_log(TDS_DBG_FUNC, "Product name for 0x%x is %s\n", ver.reported, ver.name);
			
			/* Get server product name. */
			/* Ignore product name length; some servers seem to set it incorrectly.  */
			tds_get_byte(tds);
			product_version = 0;
			/* Compute product name length from packet length. */
			len -= 10;
			free(tds_conn(tds)->product_name);
			if (ver.major >= 7u) {
				product_version = 0x80000000u;
				memrc += tds_alloc_get_string(tds, &tds_conn(tds)->product_name, len / 2);
			} else if (ver.major >= 5) {
				memrc += tds_alloc_get_string(tds, &tds_conn(tds)->product_name, len);
			} else {
				memrc += tds_alloc_get_string(tds, &tds_conn(tds)->product_name, len);
				if (tds_conn(tds)->product_name != NULL && strstr(tds_conn(tds)->product_name, "Microsoft") != NULL)
					product_version = 0x80000000u;
			}
			
			product_version |= ((TDS_UINT) tds_get_byte(tds)) << 24;
			product_version |= ((TDS_UINT) tds_get_byte(tds)) << 16;
			product_version |= ((TDS_UINT) tds_get_byte(tds)) << 8;
			product_version |= tds_get_byte(tds);

			/*
			 * MSSQL 6.5 and 7.0 seem to return strange values for this
			 * using TDS 4.2, something like 5F 06 32 FF for 6.50
			 */
			if (ver.major == 4 && ver.minor == 2 && (product_version & 0xff0000ffu) == 0x5f0000ffu)
				product_version = ((product_version & 0xffff00u) | 0x800000u) << 8;
			tds_conn(tds)->product_version = product_version;
			tdsdump_log(TDS_DBG_FUNC, "Product version %lX\n", (unsigned long) product_version);

			/*
			 * TDS 5.0 reports 5 on success 6 on failure
			 * TDS 4.2 reports 1 on success and is not
			 * present on failure
			 */
			if (ack == 5 || ack == 1)
				succeed = TDS_SUCCESS;
			/* authentication is now useless */
			if (tds_conn(tds)->authentication) {
				tds_conn(tds)->authentication->free(tds_conn(tds), tds_conn(tds)->authentication);
				tds_conn(tds)->authentication = NULL;
			}
			break;
		default:
			if (TDS_FAILED(tds_process_default_tokens(tds, marker)))
				return TDS_FAIL;
			break;
		}
	} while (marker != TDS_DONE_TOKEN);
	/* TODO why ?? */
	tds->spid = tds->rows_affected;
	if (tds->spid == 0) {
		if (TDS_FAILED(tds_set_spid(tds))) {
			tdsdump_log(TDS_DBG_ERROR, "tds_set_spid() failed\n");
			succeed = TDS_FAIL;
		}
	}
	if (memrc != 0)
		succeed = TDS_FAIL;
		
	tdsdump_log(TDS_DBG_FUNC, "tds_process_login_tokens() returning %s\n", 
					(succeed == TDS_SUCCESS)? "TDS_SUCCESS" : "TDS_FAIL");

	return succeed;
}

/**
 * Process authentication token.
 * This token is only TDS 7.0+.
 * \tds
 */
static TDSRET
tds_process_auth(TDSSOCKET * tds)
{
	unsigned int pdu_size;

	CHECK_TDS_EXTRA(tds);

#if ENABLE_EXTRA_CHECKS
	if (!IS_TDS7_PLUS(tds->conn))
		tdsdump_log(TDS_DBG_ERROR, "Called auth on TDS version < 7\n");
#endif

	pdu_size = tds_get_usmallint(tds);
	tdsdump_log(TDS_DBG_INFO1, "TDS_AUTH_TOKEN PDU size %u\n", pdu_size);

	if (!tds_conn(tds)->authentication)
		return TDS_FAIL;

	return tds_conn(tds)->authentication->handle_next(tds, tds_conn(tds)->authentication, pdu_size);
}

/**
 * process all streams.
 * tds_process_tokens() is called after submitting a query with
 * tds_submit_query() and is responsible for calling the routines to
 * populate tds->res_info if appropriate (some query have no result sets)
 * @tds
 * @param result_type A pointer to an integer variable which 
 *        tds_process_tokens sets to indicate the current type of result.
 *  @par
 *  <b>Values that indicate command status</b>
 *  <table>
 *   <tr><td>TDS_DONE_RESULT</td><td>The results of a command have been completely processed. 
 * 					This command returned no rows.</td></tr>
 *   <tr><td>TDS_DONEPROC_RESULT</td><td>The results of a  command have been completely processed.  
 * 					This command returned rows.</td></tr>
 *   <tr><td>TDS_DONEINPROC_RESULT</td><td>The results of a  command have been completely processed.  
 * 					This command returned rows.</td></tr>
 *  </table>
 *  <b>Values that indicate results information is available</b>
 *  <table><tr>
 *    <td>TDS_ROWFMT_RESULT</td><td>Regular Data format information</td>
 *    <td>tds->res_info now contains the result details ; tds->current_results now points to that data</td>
 *   </tr><tr>
 *    <td>TDS_COMPUTEFMT_ RESULT</td><td>Compute data format information</td>
 *    <td>tds->comp_info now contains the result data; tds->current_results now points to that data</td>
 *   </tr><tr>
 *    <td>TDS_DESCRIBE_RESULT</td><td></td>
 *    <td></td>
 *  </tr></table>
 *  <b>Values that indicate data is available</b>
 *  <table><tr>
 *   <td><b>Value</b></td><td><b>Meaning</b></td><td><b>Information returned</b></td>
 *   </tr><tr>
 *    <td>TDS_ROW_RESULT</td><td>Regular row results</td>
 *    <td>1 or more rows of regular data can now be retrieved</td>
 *   </tr><tr>
 *    <td>TDS_COMPUTE_RESULT</td><td>Compute row results</td>
 *    <td>A single row of compute data can now be retrieved</td>
 *   </tr><tr>
 *    <td>TDS_PARAM_RESULT</td><td>Return parameter results</td>
 *    <td>param_info or cur_dyn->params contain returned parameters</td>
 *   </tr><tr>
 *    <td>TDS_STATUS_RESULT</td><td>Stored procedure status results</td>
 *    <td>tds->ret_status contain the returned code</td>
 *  </tr></table>
 * @param done_flags Flags contained in the TDS_DONE*_TOKEN readed
 * @param flag Flags to select token type to stop/return
 * @todo Complete TDS_DESCRIBE_RESULT description
 * @retval TDS_SUCCESS if a result set is available for processing.
 * @retval TDS_FAIL on error.
 * @retval TDS_NO_MORE_RESULTS if all results have been completely processed.
 * @retval anything returned by one of the many functions it calls.  :-(
 */
TDSRET
tds_process_tokens(TDSSOCKET *tds, TDS_INT *result_type, int *done_flags, unsigned flag)
{
	int marker;
	TDSPARAMINFO *pinfo = NULL;
	TDSCOLUMN   *curcol;
	TDSRET rc;
	TDS_INT8 saved_rows_affected = tds->rows_affected;
	TDS_INT ret_status;
	int cancel_seen = 0;
	unsigned return_flag = 0;

/** \cond HIDDEN_SYMBOLS */
#define SET_RETURN(ret, f) \
	*result_type = ret; \
	return_flag = TDS_RETURN_##f | TDS_STOPAT_##f; \
	if (flag & TDS_STOPAT_##f) {\
		tds_unget_byte(tds); \
		tdsdump_log(TDS_DBG_FUNC, "tds_process_tokens::SET_RETURN stopping on current token\n"); \
		break; \
	}
/** \endcond */

	CHECK_TDS_EXTRA(tds);

	tdsdump_log(TDS_DBG_FUNC, "tds_process_tokens(%p, %p, %p, 0x%x)\n", tds, result_type, done_flags, flag);
	
	if (tds->state == TDS_IDLE) {
		tdsdump_log(TDS_DBG_FUNC, "tds_process_tokens() state is COMPLETED\n");
		*result_type = TDS_DONE_RESULT;
		return TDS_NO_MORE_RESULTS;
	}

	if (tds_set_state(tds, TDS_READING) != TDS_READING)
		return TDS_FAIL;

	rc = TDS_SUCCESS;
	for (;;) {

		marker = tds_get_byte(tds);
		tdsdump_log(TDS_DBG_INFO1, "processing result tokens.  marker is  %x(%s)\n", marker, tds_token_name(marker));

		switch (marker) {
		case TDS7_RESULT_TOKEN:

			/*
			 * If we're processing the results of a cursor fetch
			 * from sql server we don't want to pass back the
			 * TDS_ROWFMT_RESULT to the calling API
			 */

			if (tds->current_op == TDS_OP_CURSORFETCH) {
				rc = tds7_process_result(tds);
				if (TDS_FAILED(rc))
					break;
				marker = tds_get_byte(tds);
				if (marker != TDS_TABNAME_TOKEN)
					tds_unget_byte(tds);
				else
					rc = tds_process_tabname(tds);
			} else {
				SET_RETURN(TDS_ROWFMT_RESULT, ROWFMT);

				rc = tds7_process_result(tds);
				if (TDS_FAILED(rc))
					break;
				/* handle browse information (if presents) */
				marker = tds_get_byte(tds);
				if (marker != TDS_TABNAME_TOKEN) {
					tds_unget_byte(tds);
					rc = TDS_SUCCESS;
					break;
				}
				rc = tds_process_tabname(tds);
			}
			break;
		case TDS_RESULT_TOKEN:
			SET_RETURN(TDS_ROWFMT_RESULT, ROWFMT);
			rc = tds_process_result(tds);
			break;
		case TDS_ROWFMT2_TOKEN:
			SET_RETURN(TDS_ROWFMT_RESULT, ROWFMT);
			rc = tds5_process_result(tds);
			break;
		case TDS_COLNAME_TOKEN:
			rc = tds_process_col_name(tds);
			break;
		case TDS_COLFMT_TOKEN:
			SET_RETURN(TDS_ROWFMT_RESULT, ROWFMT);
			rc = tds_process_col_fmt(tds);
			/* handle browse information (if present) */
			marker = tds_get_byte(tds);
			if (marker != TDS_TABNAME_TOKEN) {
				tds_unget_byte(tds);
				break;
			}
			rc = tds_process_tabname(tds);
			break;
		case TDS_PARAM_TOKEN:
			tds_unget_byte(tds);
			if (tds->current_op) {
				tdsdump_log(TDS_DBG_FUNC, "processing parameters for op %d\n", tds->current_op);
				while ((marker = tds_get_byte(tds)) == TDS_PARAM_TOKEN) {
					tdsdump_log(TDS_DBG_INFO1, "calling tds_process_param_result\n");
					tds_process_param_result(tds, &pinfo);
				}
				tds_unget_byte(tds);
				tdsdump_log(TDS_DBG_FUNC, "%d hidden return parameters\n", pinfo ? pinfo->num_cols : -1);
				if (pinfo && pinfo->num_cols > 0) {
					curcol = pinfo->columns[0];
					if (tds->current_op == TDS_OP_CURSOROPEN && tds->cur_cursor) {
						TDSCURSOR  *cursor = tds->cur_cursor; 

						cursor->cursor_id = *(TDS_INT *) curcol->column_data;
						tdsdump_log(TDS_DBG_FUNC, "stored internal cursor id %d\n", cursor->cursor_id);
						cursor->srv_status &= ~(TDS_CUR_ISTAT_CLOSED|TDS_CUR_ISTAT_OPEN|TDS_CUR_ISTAT_DEALLOC);
						cursor->srv_status |= cursor->cursor_id ? TDS_CUR_ISTAT_OPEN : TDS_CUR_ISTAT_CLOSED|TDS_CUR_ISTAT_DEALLOC;
					}
					if ((tds->current_op == TDS_OP_PREPARE || tds->current_op == TDS_OP_PREPEXEC)
					    && tds->cur_dyn && tds->cur_dyn->num_id == 0 && curcol->column_cur_size > 0) {
						tds->cur_dyn->num_id = *(TDS_INT *) curcol->column_data;
					}
					if (tds->current_op == TDS_OP_UNPREPARE)
						tds_dynamic_deallocated(tds->conn, tds->cur_dyn);
				}
				tds_free_param_results(pinfo);
			} else {
				SET_RETURN(TDS_PARAM_RESULT, PROC);
				rc = tds_process_param_result_tokens(tds);
			}
			break;
		case TDS_COMPUTE_NAMES_TOKEN:
			rc = tds_process_compute_names(tds);
			break;
		case TDS_COMPUTE_RESULT_TOKEN:
			SET_RETURN(TDS_COMPUTEFMT_RESULT, COMPUTEFMT);
			rc = tds_process_compute_result(tds);
			break;
		case TDS7_COMPUTE_RESULT_TOKEN:
			SET_RETURN(TDS_COMPUTEFMT_RESULT, COMPUTEFMT);
			rc = tds7_process_compute_result(tds);
			break;
		case TDS_ROW_TOKEN:
		case TDS_NBC_ROW_TOKEN:
			/* overstepped the mark... */
			if (tds->cur_cursor) {
				tds_set_current_results(tds, tds->cur_cursor->res_info);
				tdsdump_log(TDS_DBG_INFO1, "tds_process_tokens(). set current_results to cursor->res_info\n");
			} else {
				/* assure that we point to row, not to compute */
				if (tds->res_info)
					tds_set_current_results(tds, tds->res_info);
			}
			/* I don't know when this it's false but it happened, also server can send garbage... */
			if (tds->current_results)
				tds->current_results->rows_exist = 1;
			SET_RETURN(TDS_ROW_RESULT, ROW);

			switch (marker) {
			case TDS_ROW_TOKEN:
				rc = tds_process_row(tds);
				break;
			case TDS_NBC_ROW_TOKEN:
				rc = tds_process_nbcrow(tds);
				break;
			}
			break;
		case TDS_CMP_ROW_TOKEN:
			/* I don't know when this it's false but it happened, also server can send garbage... */
			if (tds->res_info)
				tds->res_info->rows_exist = 1;
			SET_RETURN(TDS_COMPUTE_RESULT, COMPUTE);
			rc = tds_process_compute(tds, NULL);
			break;
		case TDS_RETURNSTATUS_TOKEN:
			ret_status = tds_get_int(tds);
			marker = tds_peek(tds);
			if (marker != TDS_PARAM_TOKEN && marker != TDS_DONEPROC_TOKEN && marker != TDS_DONE_TOKEN && marker != TDS5_PARAMFMT_TOKEN && marker != TDS5_PARAMFMT2_TOKEN)
				break;
			if (tds->current_op) {
				/* TODO perhaps we should use ret_status ?? */
			} else {
				/* TODO optimize */
				flag &= ~TDS_STOPAT_PROC;
				SET_RETURN(TDS_STATUS_RESULT, PROC);
				tds->has_status = 1;
				tds->ret_status = ret_status;
				tdsdump_log(TDS_DBG_FUNC, "tds_process_tokens: return status is %d\n", tds->ret_status);
				rc = TDS_SUCCESS;
			}
			break;
		case TDS5_DYNAMIC_TOKEN:
			/* process acknowledge dynamic */
			tds_set_cur_dyn(tds, tds_process_dynamic(tds));
			/* special case, prepared statement cannot be prepared */
			if (!tds->cur_dyn || tds->cur_dyn->emulated)
				break;
			marker = tds_get_byte(tds);
			if (marker != TDS_EED_TOKEN) {
				tds_unget_byte(tds);
				break;
			}
			tds_process_msg(tds, marker);
			if (!tds->cur_dyn || !tds->cur_dyn->emulated)
				break;
			marker = tds_get_byte(tds);
			if (marker != TDS_DONE_TOKEN) {
				tds_unget_byte(tds);
				break;
			}
			rc = tds_process_end(tds, marker, done_flags);
			if (done_flags)
				*done_flags &= ~TDS_DONE_ERROR;
			/* FIXME warning to macro expansion */
			SET_RETURN(TDS_DONE_RESULT, DONE);
			break;
		case TDS5_PARAMFMT_TOKEN:
			SET_RETURN(TDS_DESCRIBE_RESULT, PARAMFMT);
			rc = tds_process_dyn_result(tds);
			break;
		case TDS5_PARAMFMT2_TOKEN:
			SET_RETURN(TDS_DESCRIBE_RESULT, PARAMFMT);
			rc = tds5_process_dyn_result2(tds);
			break;
		case TDS5_PARAMS_TOKEN:
			SET_RETURN(TDS_PARAM_RESULT, PROC);
			rc = tds_process_params_result_token(tds);
			break;
		case TDS_CURINFO_TOKEN:
			rc = tds_process_cursor_tokens(tds);
			break;
		case TDS_DONE_TOKEN:
			SET_RETURN(TDS_DONE_RESULT, DONE);
			rc = tds_process_end(tds, marker, done_flags);
			switch (tds->current_op) {
			case TDS_OP_DYN_DEALLOC:
				if (done_flags && (*done_flags & TDS_DONE_ERROR) == 0)
					tds_dynamic_deallocated(tds->conn, tds->cur_dyn);
				break;
			default:
				break;
			}
			break;
		case TDS_DONEPROC_TOKEN:
			SET_RETURN(TDS_DONEPROC_RESULT, DONE);
			rc = tds_process_end(tds, marker, done_flags);
			switch (tds->current_op) {
			default:
				break;
			case TDS_OP_CURSOROPEN: 
				*result_type       = TDS_DONE_RESULT;
				tds->rows_affected = saved_rows_affected;
				break;
			case TDS_OP_CURSORCLOSE:
				tdsdump_log(TDS_DBG_FUNC, "TDS_OP_CURSORCLOSE\n");
				if (tds->cur_cursor) {
 
					TDSCURSOR  *cursor = tds->cur_cursor;
 
					cursor->srv_status &= ~TDS_CUR_ISTAT_OPEN;
					cursor->srv_status |= TDS_CUR_ISTAT_CLOSED|TDS_CUR_ISTAT_DECLARED;
					if (cursor->status.dealloc == TDS_CURSOR_STATE_SENT) {
						tds_cursor_deallocated(tds->conn, cursor);
					}
				}
				*result_type = TDS_NO_MORE_RESULTS;
				rc = TDS_NO_MORE_RESULTS;
				break;
			case TDS_OP_CURSOR:
			case TDS_OP_CURSORPREPARE:
			case TDS_OP_CURSOREXECUTE:
			case TDS_OP_CURSORPREPEXEC:
			case TDS_OP_CURSORUNPREPARE:
			case TDS_OP_CURSORFETCH:
			case TDS_OP_CURSOROPTION:
			case TDS_OP_PREPEXECRPC:
			case TDS_OP_UNPREPARE:
				*result_type = TDS_NO_MORE_RESULTS;
				rc = TDS_NO_MORE_RESULTS;
				break;
			}
			break;
		case TDS_DONEINPROC_TOKEN:
			switch(tds->current_op) {
			case TDS_OP_CURSOROPEN:
			case TDS_OP_CURSORFETCH:
			case TDS_OP_PREPARE:
			case TDS_OP_CURSORCLOSE:
				rc = tds_process_end(tds, marker, done_flags);
				if (tds->rows_affected != TDS_NO_COUNT) {
					saved_rows_affected = tds->rows_affected;
				}
				break;
			default:
				SET_RETURN(TDS_DONEINPROC_RESULT, DONE);
				rc = tds_process_end(tds, marker, done_flags);
				break;
			}
			break;
		case TDS_ERROR_TOKEN:
		case TDS_INFO_TOKEN:
		case TDS_EED_TOKEN:
			SET_RETURN(TDS_MSG_RESULT, MSG);
			rc = tds_process_default_tokens(tds, marker);
			break;
		default:
			SET_RETURN(TDS_OTHERS_RESULT, OTHERS);
			rc = tds_process_default_tokens(tds, marker);
			break;
		}

		if (TDS_FAILED(rc)) {
			tds_set_state(tds, TDS_PENDING);
			return rc;
		}

		cancel_seen |= tds->in_cancel;
		if (cancel_seen) {
			/* during cancel handle all tokens */
			flag = TDS_HANDLE_ALL;
		}

		if ((return_flag & flag) != 0) {
			tds_set_state(tds, TDS_PENDING);
			return rc;
		}

		if (tds->state == TDS_IDLE)
			return cancel_seen ? TDS_CANCELLED : TDS_NO_MORE_RESULTS;

		if (tds->state == TDS_DEAD) {
			/* TODO free all results ?? */
			return TDS_FAIL;
		}
	}
}

/**
 * Process results for simple query as "SET TEXTSIZE" or "USE dbname"
 * If the statement returns results, beware they are discarded.
 *
 * This function was written to avoid direct calls to tds_process_default_tokens
 * (which caused problems such as ignoring query errors).
 * Results are read until idle state or severe failure (do not stop for 
 * statement failure).
 * @return see tds_process_tokens for results (TDS_NO_MORE_RESULTS is never returned)
 */
TDSRET
tds_process_simple_query(TDSSOCKET * tds)
{
	TDS_INT res_type;
	TDS_INT done_flags;
	TDSRET  rc;
	TDSRET  ret = TDS_SUCCESS;

	CHECK_TDS_EXTRA(tds);

	while ((rc = tds_process_tokens(tds, &res_type, &done_flags, TDS_RETURN_DONE)) == TDS_SUCCESS) {
		switch (res_type) {

			case TDS_DONE_RESULT:
			case TDS_DONEPROC_RESULT:
			case TDS_DONEINPROC_RESULT:
				if ((done_flags & TDS_DONE_ERROR) != 0) 
					ret = TDS_FAIL;
				break;

			default:
				break;
		}
	}
	if (TDS_FAILED(rc))
		ret = rc;

	return ret;
}

/**
 * Holds list of names
 */
struct namelist
{
	/** string name */
	char *name;
	/** next element in the list */
	struct namelist *next;
};

/**
 * Frees list of names
 * \param head list head to free
 */
static void
tds_free_namelist(struct namelist *head)
{
	struct namelist *cur = head, *prev;

	while (cur != NULL) {
		prev = cur;
		cur = cur->next;
		free(prev->name);
		free(prev);
	}
}

/**
 * Reads list of names (usually table names)
 * \tds
 * \param remainder bytes left to read
 * \param p_head list head to return
 * \param large true if name length from network are 2 byte (usually 1)
 */
static int
tds_read_namelist(TDSSOCKET * tds, int remainder, struct namelist **p_head, int large)
{
	struct namelist *head = NULL, *cur = NULL, *prev;
	int num_names = 0;

	/*
	 * this is a little messy...TDS 5.0 gives the number of columns
	 * upfront, while in TDS 4.2, you're expected to figure it out
	 * by the size of the message. So, I use a link list to get the
	 * colum names and then allocate the result structure, copy
	 * and delete the linked list
	 */
	while (remainder > 0) {
		TDS_USMALLINT namelen;

		prev = cur;
		if (!(cur = (struct namelist *) malloc(sizeof(struct namelist)))) {
			tds_free_namelist(head);
			return -1;
		}

		cur->next = NULL;
		if (prev)
			prev->next = cur;
		else
			head = cur;

		if (large) {
			namelen = tds_get_usmallint(tds);
			remainder -= 2;
		} else {
			namelen = tds_get_byte(tds);
			--remainder;
		}

		if (tds_alloc_get_string(tds, &cur->name, namelen) < 0) {
			tds_free_namelist(head);
			return -1;
		}

		remainder -= namelen;
		if (IS_TDS7_PLUS(tds->conn))
			remainder -= namelen;
		num_names++;
	}

	*p_head = head;
	return num_names;
}

/**
 * tds_process_col_name() is one half of the result set under TDS 4.2
 * it contains all the column names, a TDS_COLFMT_TOKEN should 
 * immediately follow this token with the datatype/size information
 * This is a 4.2 only function
 * \tds
 */
static TDSRET
tds_process_col_name(TDSSOCKET * tds)
{
	int hdrsize;
	int col, num_names = 0;
	struct namelist *head = NULL, *cur = NULL;
	TDSCOLUMN *curcol;
	TDSRESULTINFO *info;

	CHECK_TDS_EXTRA(tds);

	hdrsize = tds_get_usmallint(tds);

	if ((num_names = tds_read_namelist(tds, hdrsize, &head, 0)) < 0)
		return TDS_FAIL;

	/* free results/computes/params etc... */
	tds_free_all_results(tds);
	tds->rows_affected = TDS_NO_COUNT;

	if ((info = tds_alloc_results(num_names)) != NULL) {

		tds->res_info = info;
		tds_set_current_results(tds, info);

		cur = head;
		for (col = 0; col < num_names; ++col) {
			curcol = info->columns[col];
			tds_strlcpy(curcol->column_name, cur->name, sizeof(curcol->column_name));
			curcol->column_namelen = (TDS_SMALLINT)strlen(curcol->column_name);
			cur = cur->next;
		}
		tds_free_namelist(head);
		return TDS_SUCCESS;
	}

	tds_free_namelist(head);
	return TDS_FAIL;
}

/**
 * tds_process_col_fmt() is the other half of result set processing
 * under TDS 4.2. It follows tds_process_col_name(). It contains all the 
 * column type and size information.
 * This is a 4.2 only function
 * \tds
 */
static TDSRET
tds_process_col_fmt(TDSSOCKET * tds)
{
	unsigned int col;
	TDSCOLUMN *curcol;
	TDSRESULTINFO *info;
	TDS_USMALLINT flags;

	CHECK_TDS_EXTRA(tds);

	tds_get_usmallint(tds);	/* hdrsize */

	/* TODO use current_results instead of res_info ?? */
	info = tds->res_info;
	for (col = 0; col < info->num_cols; col++) {
		curcol = info->columns[col];
		/* In Sybase all 4 byte are used for usertype, while mssql place 2 byte as usertype and 2 byte as flags */
		if (TDS_IS_MSSQL(tds)) {
			curcol->column_usertype = tds_get_smallint(tds);
			flags = tds_get_usmallint(tds);
			curcol->column_nullable = flags & 0x01;
			curcol->column_writeable = (flags & 0x08) > 0;
			curcol->column_identity = (flags & 0x10) > 0;
		} else {
			curcol->column_usertype = tds_get_int(tds);
		}
		/* on with our regularly scheduled code (mlilback, 11/7/01) */
		tds_set_column_type(tds->conn, curcol, tds_get_byte(tds));

		tdsdump_log(TDS_DBG_INFO1, "processing result. type = %d(%s), varint_size %d\n",
			    curcol->column_type, tds_prtype(curcol->column_type), curcol->column_varint_size);

		curcol->funcs->get_info(tds, curcol);

		/* Adjust column size according to client's encoding */
		curcol->on_server.column_size = curcol->column_size;
		adjust_character_column_size(tds, curcol);
	}

	return tds_alloc_row(info);
}

/**
 * Reads table names for TDS 7.1+.
 * TDS 7.1+ return table names as an array of names
 * (so database.schema.owner.name as separate names)
 * \tds
 * \param remainder bytes left to read
 * \param p_head pointer to list head to return
 * \return number of element returned or -1 on error
 */
static int
tds71_read_table_names(TDSSOCKET *tds, int remainder, struct namelist **p_head)
{
	struct namelist *head = NULL, *cur = NULL, *prev;
	int num_names = 0;

	/*
	 * this is a little messy...TDS 5.0 gives the number of columns
	 * upfront, while in TDS 4.2, you're expected to figure it out
	 * by the size of the message. So, I use a link list to get the
	 * colum names and then allocate the result structure, copy
	 * and delete the linked list
	 */
	while (remainder > 0) {
		int elements, i;
		size_t len;
		char *partials[4], *p;

		prev = cur;
		if (!(cur = (struct namelist *) malloc(sizeof(struct namelist)))) {
			tds_free_namelist(head);
			return -1;
		}

		cur->name = NULL;
		cur->next = NULL;
		if (prev)
			prev->next = cur;
		else
			head = cur;

		elements = tds_get_byte(tds);
		--remainder;
		if (elements <= 0 || elements > 4) {
			tds_free_namelist(head);
			return -1;
		}

		/* read partials IDs and compute full length */
		len = 0;
		for (i = 0; i < elements; ++i) {
			TDS_USMALLINT namelen = tds_get_usmallint(tds);
			remainder -= 2 + 2 * namelen;
			if (tds_alloc_get_string(tds, &partials[i], namelen) < 0) {
				while (i > 0)
					free(partials[--i]);
				tds_free_namelist(head);
				return -1;
			}
			len += tds_quote_id(tds, NULL, partials[i], -1) + 1;
		}

		/* allocate full name */
		p = (char *) malloc(len);
		if (!p) {
			i = elements;
			while (i > 0)
				free(partials[--i]);
			tds_free_namelist(head);
			return -1;
		}

		/* compose names */
		cur->name = p;
		for (i = 0; i < elements; ++i) {
			p += tds_quote_id(tds, p, partials[i], -1);
			*p++ = '.';
			free(partials[i]);
		}
		*--p = 0;

		num_names++;
	}

	*p_head = head;
	return num_names;
}

/**
 * Process list of table from network.
 * This token is only TDS 4.2
 * \tds
 */
static TDSRET
tds_process_tabname(TDSSOCKET *tds)
{
	struct namelist *head, *cur;
	int num_names, hdrsize, i;
	char **names;
	unsigned char marker;
	TDSRET rc;

	hdrsize = tds_get_usmallint(tds);

	/* different structure for tds7.1 */
	/* hdrsize check is required for tds7.1 revision 1 (mssql without SPs) */
	/* TODO change tds_version ?? */
	if (IS_TDS71_PLUS(tds->conn) && (!IS_TDS71(tds->conn) || !tds_conn(tds)->tds71rev1))
		num_names = tds71_read_table_names(tds, hdrsize, &head);
	else
		num_names = tds_read_namelist(tds, hdrsize, &head, 1);
	if (num_names < 0)
		return TDS_FAIL;

	/* put in an array */
	names = (char **) malloc(num_names * sizeof(char*));
	if (!names) {
		tds_free_namelist(head);
		return TDS_FAIL;
	}
	for (cur = head, i = 0; i < num_names; ++i, cur = cur->next)
		names[i] = cur->name;

	rc = TDS_SUCCESS;
	marker = tds_get_byte(tds);
	if (marker != TDS_COLINFO_TOKEN)
		tds_unget_byte(tds);
	else
		rc = tds_process_colinfo(tds, names, num_names);

	free(names);
	tds_free_namelist(head);
	return rc;
}

/**
 * Reads column information.
 * This token is only TDS 4.2
 * \tds
 * \param[in] names table names
 * \param[in] num_names number of table names
 */
static TDSRET
tds_process_colinfo(TDSSOCKET * tds, char **names, int num_names)
{
	unsigned int hdrsize, l;
	TDSCOLUMN *curcol;
	TDSRESULTINFO *info;
	unsigned int bytes_read = 0;
	unsigned char col_info[3];

	CHECK_TDS_EXTRA(tds);

	hdrsize = tds_get_usmallint(tds);

	info = tds->current_results;

	while (bytes_read < hdrsize) {

		tds_get_n(tds, col_info, 3);
		bytes_read += 3;

		curcol = NULL;
		if (info && col_info[0] > 0 && col_info[0] <= info->num_cols)
			curcol = info->columns[col_info[0] - 1];

		if (curcol) {
			curcol->column_writeable = (col_info[2] & 0x4) == 0;
			curcol->column_key = (col_info[2] & 0x8) > 0;
			curcol->column_hidden = (col_info[2] & 0x10) > 0;

			if (names && col_info[1] > 0 && col_info[1] <= num_names)
				tds_dstr_copy(&curcol->table_name, names[col_info[1] - 1]);
		}
		/* read real column name */
		if (col_info[2] & 0x20) {
			l = tds_get_byte(tds);
			if (curcol) {
				tds_dstr_get(tds, &curcol->table_column_name, l);
				if (IS_TDS7_PLUS(tds->conn))
					l *= 2;
			} else {
				if (IS_TDS7_PLUS(tds->conn))
					l *= 2;
				/* discard silently */
				tds_get_n(tds, NULL, l);
			}
			bytes_read += l + 1;
		}
	}

	return TDS_SUCCESS;
}

/**
 * process output parameters of a stored 
 * procedure. This differs from regular row/compute results in that there
 * is no total number of parameters given, they just show up singly.
 * \tds
 * \param[out] pinfo output parameter.
 *             Should point to a not allocated structure
 */
static TDSRET
tds_process_param_result(TDSSOCKET * tds, TDSPARAMINFO ** pinfo)
{
	TDSCOLUMN *curparam;
	TDSPARAMINFO *info;
	TDSRET token;

	tdsdump_log(TDS_DBG_FUNC, "tds_process_param_result(%p, %p)\n", tds, pinfo);

	CHECK_TDS_EXTRA(tds);
	if (*pinfo)
		CHECK_PARAMINFO_EXTRA(*pinfo);

	/* TODO check if current_results is a param result */

	/* limited to 64K but possible types are always smaller (not TEXT/IMAGE) */
	tds_get_smallint(tds);	/* header size */
	if ((info = tds_alloc_param_result(*pinfo)) == NULL)
		return TDS_FAIL;

	*pinfo = info;
	curparam = info->columns[info->num_cols - 1];

	/*
	 * FIXME check support for tds7+ (seem to use same format of tds5 for data...)
	 * perhaps varint_size can be 2 or collation can be specified ??
	 */
	tds_get_data_info(tds, curparam, 1);

	curparam->column_cur_size = curparam->column_size;	/* needed ?? */

	if (tds_alloc_param_data(curparam) == NULL)
		return TDS_FAIL;

	token = curparam->funcs->get_data(tds, curparam);
	tdsdump_col(curparam);

	/*
	 * Real output parameters will either be unnamed or will have a valid
	 * parameter name beginning with '@'. Ignore any other Spurious parameters
	 * such as those returned from calls to writetext in the proc.
	 */
	if (curparam->column_namelen > 0 && curparam->column_name[0] != '@')
		tds_free_param_result(*pinfo);

	return token;
}

/**
 * Process parameters from networks.
 * Read all consecutives paramaters, not a single one.
 * Parameters are then stored in tds->param_info or tds->cur_dyn->res_info
 * depending if we are reading cursor results or normal parameters.
 * \tds
 */
static TDSRET
tds_process_param_result_tokens(TDSSOCKET * tds)
{
	int marker;
	TDSPARAMINFO **pinfo;

	CHECK_TDS_EXTRA(tds);

	if (tds->cur_dyn)
		pinfo = &(tds->cur_dyn->res_info);
	else
		pinfo = &(tds->param_info);

	while ((marker = tds_get_byte(tds)) == TDS_PARAM_TOKEN) {
		tds_process_param_result(tds, pinfo);
	}
	if (!marker) {
		tdsdump_log(TDS_DBG_FUNC, "error: tds_process_param_result() returned TDS_FAIL\n");
		return TDS_FAIL;
	}

	tds_set_current_results(tds, *pinfo);
	tds_unget_byte(tds);
	return TDS_SUCCESS;
}

/**
 * tds_process_params_result_token() processes params on TDS5.
 * \tds
 */
static TDSRET
tds_process_params_result_token(TDSSOCKET * tds)
{
	unsigned int i;
	TDSPARAMINFO *info;

	CHECK_TDS_EXTRA(tds);

	/* TODO check if current_results is a param result */
	info = tds->current_results;
	if (!info)
		return TDS_FAIL;

	for (i = 0; i < info->num_cols; i++) {
		TDSCOLUMN *curcol = info->columns[i];
		TDSRET rc = curcol->funcs->get_data(tds, curcol);
		if (TDS_FAILED(rc))
			return rc;
	}
	return TDS_SUCCESS;
}

/**
 * tds_process_compute_result() processes compute result sets.  These functions
 * need work but since they get little use, nobody has complained!
 * It is very similar to normal result sets.
 * \tds
 */
static TDSRET
tds_process_compute_result(TDSSOCKET * tds)
{
	unsigned int col, num_cols;
	TDS_TINYINT by_cols = 0;
	TDS_SMALLINT *cur_by_col;
	TDS_SMALLINT compute_id = 0;
	TDSCOLUMN *curcol;
	TDSCOMPUTEINFO *info = NULL;
	unsigned int i;

	CHECK_TDS_EXTRA(tds);

	tds_get_smallint(tds);	/* header size*/

	/*
	 * Compute statement id which this relates to. 
	 * You can have more than one compute clause in a SQL statement
	 */

	compute_id = tds_get_smallint(tds);
	num_cols = tds_get_byte(tds);	

	tdsdump_log(TDS_DBG_INFO1, "tds_process_compute_result(): compute_id %d for %d columns\n", compute_id, num_cols);

	for (i=0; i < tds->num_comp_info; ++i) {
		if (tds->comp_info[i]->computeid == compute_id) {
			info = tds->comp_info[i];
			break;
		}
	}
	if (NULL == info) {
		tdsdump_log(TDS_DBG_FUNC, "logic error: compute_id (%d) from server not found in tds->comp_info\n", compute_id);
		return TDS_FAIL;
	}
	
	tdsdump_log(TDS_DBG_FUNC, "found computeid %d in tds->comp_info\n", info->computeid);
	tds_set_current_results(tds, info);

	tdsdump_log(TDS_DBG_INFO1, "processing compute result. num_cols = %d\n", num_cols);

	/* 
	 * Iterate over compute columns returned, 
	 * 	e.g. COMPUTE SUM(x), AVG(x) would return num_cols = 2. 
	 */
	 for (col = 0; col < num_cols; col++) {
		tdsdump_log(TDS_DBG_INFO1, "processing compute column %d\n", col);
		curcol = info->columns[col];

		curcol->column_operator = tds_get_byte(tds);
		curcol->column_operand = tds_get_byte(tds);

		/* If no name has been defined for the compute column, use "max", "avg" etc. */
		if (curcol->column_namelen == 0) {
			strcpy(curcol->column_name, tds_pr_op(curcol->column_operator));
			curcol->column_namelen = (TDS_SMALLINT)strlen(curcol->column_name);
		}

		/*  User defined data type of the column */
		curcol->column_usertype = tds_get_int(tds);

		tds_set_column_type(tds->conn, curcol, tds_get_byte(tds));

		curcol->funcs->get_info(tds, curcol);

		tdsdump_log(TDS_DBG_INFO1, "compute column_size is %d\n", curcol->column_size);

		/* Adjust column size according to client's encoding */
		curcol->on_server.column_size = curcol->column_size;
		/* TODO check if this column can have collation information associated */
		adjust_character_column_size(tds, curcol);

		/* skip locale */
		if (!IS_TDS42(tds->conn))
			tds_get_n(tds, NULL, tds_get_byte(tds));
	}

	by_cols = tds_get_byte(tds);

	tdsdump_log(TDS_DBG_INFO1, "processing tds compute result, by_cols = %d\n", by_cols);

	if (by_cols) {
		if ((info->bycolumns = calloc(by_cols, sizeof(TDS_SMALLINT))) == NULL)
			return TDS_FAIL;
	}
	info->by_cols = by_cols;

	cur_by_col = info->bycolumns;
	for (col = 0; col < by_cols; col++) {
		*cur_by_col = tds_get_byte(tds);
		cur_by_col++;
	}

	return tds_alloc_compute_row(info);
}

/**
 * Reads data information from wire
 * \tds
 * \param curcol column where to store information
 */
static TDSRET
tds7_get_data_info(TDSSOCKET * tds, TDSCOLUMN * curcol)
{
	int colnamelen;

	CHECK_TDS_EXTRA(tds);
	CHECK_COLUMN_EXTRA(curcol);

	/*  User defined data type of the column */
	curcol->column_usertype = IS_TDS72_PLUS(tds->conn) ? tds_get_int(tds) : tds_get_smallint(tds);

	curcol->column_flags = tds_get_smallint(tds);	/*  Flags */

	curcol->column_nullable = curcol->column_flags & 0x01;
	curcol->column_writeable = (curcol->column_flags & 0x08) > 0;
	curcol->column_identity = (curcol->column_flags & 0x10) > 0;

	tds_set_column_type(tds->conn, curcol, tds_get_byte(tds));	/* sets "cardinal" type */

	curcol->column_timestamp = (curcol->column_type == SYBBINARY && curcol->column_usertype == TDS_UT_TIMESTAMP);

	curcol->funcs->get_info(tds, curcol);

	/* Adjust column size according to client's encoding */
	curcol->on_server.column_size = curcol->column_size;

	/* NOTE adjustements must be done after curcol->char_conv initialization */
	adjust_character_column_size(tds, curcol);

	/*
	 * under 7.0 lengths are number of characters not
	 * number of bytes...tds_get_string handles this
	 */
	colnamelen = tds_get_string(tds, tds_get_byte(tds), curcol->column_name, sizeof(curcol->column_name) - 1);
	curcol->column_name[colnamelen] = 0;
	curcol->column_namelen = colnamelen;

	tdsdump_log(TDS_DBG_INFO1, "tds7_get_data_info: \n"
		    "\tcolname = %s (%d bytes)\n"
		    "\ttype = %d (%s)\n"
		    "\tserver's type = %d (%s)\n"
		    "\tcolumn_varint_size = %d\n"
		    "\tcolumn_size = %d (%d on server)\n",
		    curcol->column_name, curcol->column_namelen, 
		    curcol->column_type, tds_prtype(curcol->column_type), 
		    curcol->on_server.column_type, tds_prtype(curcol->on_server.column_type), 
		    curcol->column_varint_size,
		    curcol->column_size, curcol->on_server.column_size);

	CHECK_COLUMN_EXTRA(curcol);

	return TDS_SUCCESS;
}

/**
 * tds7_process_result() is the TDS 7.0 result set processing routine.  It 
 * is responsible for populating the tds->res_info structure.
 * This is a TDS 7.0 only function
 * \tds
 */
static TDSRET
tds7_process_result(TDSSOCKET * tds)
{
	int col, num_cols;
	TDSRET result;
	TDSRESULTINFO *info;

	CHECK_TDS_EXTRA(tds);
	tdsdump_log(TDS_DBG_INFO1, "processing TDS7 result metadata.\n");

	/* read number of columns and allocate the columns structure */

	num_cols = tds_get_smallint(tds);

	/* This can be a DUMMY results token from a cursor fetch */

	if (num_cols < 0) {
		tdsdump_log(TDS_DBG_INFO1, "no meta data\n");
		return TDS_SUCCESS;
	}

	tds_free_all_results(tds);
	tds->rows_affected = TDS_NO_COUNT;

	if ((info = tds_alloc_results(num_cols)) == NULL)
		return TDS_FAIL;
	tds_set_current_results(tds, info);
	if (tds->cur_cursor) {
		tds_free_results(tds->cur_cursor->res_info);
		tds->cur_cursor->res_info = info;
		tdsdump_log(TDS_DBG_INFO1, "set current_results to cursor->res_info\n");
	} else {
		tds->res_info = info;
		tdsdump_log(TDS_DBG_INFO1, "set current_results (%d column%s) to tds->res_info\n", num_cols, (num_cols==1? "":"s"));
	}

	/*
	 * loop through the columns populating COLINFO struct from
	 * server response
	 */
	tdsdump_log(TDS_DBG_INFO1, "setting up %d columns\n", num_cols);
	for (col = 0; col < num_cols; col++) {
		TDSCOLUMN *curcol = info->columns[col];
		
		tds7_get_data_info(tds, curcol);
	}
		
	if (num_cols > 0) {
		static char dashes[31] = "------------------------------";
		tdsdump_log(TDS_DBG_INFO1, " %-20s %-15s %-15s %-7s\n", "name", "size/wsize", "type/wtype", "utype");
		tdsdump_log(TDS_DBG_INFO1, " %-20s %15s %15s %7s\n", dashes+10, dashes+30-15, dashes+30-15, dashes+30-7);
	}
	for (col = 0; col < num_cols; col++) {
		char name[TDS_SYSNAME_SIZE] = {'\0'};
		TDSCOLUMN *curcol = info->columns[col];

		if (curcol->column_namelen > 0) {
			memcpy(name, curcol->column_name, curcol->column_namelen);
			name[curcol->column_namelen] = '\0';
		}
		tdsdump_log(TDS_DBG_INFO1, " %-20s %7d/%-7d %7d/%-7d %7d\n", 
						name, 
						curcol->column_size, curcol->on_server.column_size, 
						curcol->column_type, curcol->on_server.column_type, 
						curcol->column_usertype);
	}

	/* all done now allocate a row for tds_process_row to use */
	result = tds_alloc_row(info);
	CHECK_TDS_EXTRA(tds);
	return result;
}

/**
 * Reads data metadata from wire
 * \param tds state information for the socket and the TDS protocol
 * \param curcol column where to store information
 * \param is_param true if metadata are for a parameter (false for normal
 *        column)
 */
static TDSRET
tds_get_data_info(TDSSOCKET * tds, TDSCOLUMN * curcol, int is_param)
{
	CHECK_TDS_EXTRA(tds);
	CHECK_COLUMN_EXTRA(curcol);

	tdsdump_log(TDS_DBG_INFO1, "tds_get_data_info(%p, %p, %d) %s\n", tds, curcol, is_param, is_param? "[for parameter]" : "");

	curcol->column_namelen = tds_get_string(tds, tds_get_byte(tds), curcol->column_name, sizeof(curcol->column_name) - 1);
	curcol->column_name[curcol->column_namelen] = '\0';

	curcol->column_flags = tds_get_byte(tds);	/*  Flags */
	if (!is_param) {
		/* TODO check if all flags are the same for all TDS versions */
		if (IS_TDS50(tds->conn))
			curcol->column_hidden = curcol->column_flags & 0x1;
		curcol->column_key = (curcol->column_flags & 0x2) > 1;
		curcol->column_writeable = (curcol->column_flags & 0x10) > 1;
		curcol->column_nullable = (curcol->column_flags & 0x20) > 1;
		curcol->column_identity = (curcol->column_flags & 0x40) > 1;
#if 0
		/****************************************
		 * NumParts=BYTE; (introduced in TDS 7.2) 
		 * PartName=US_VARCHAR;(introduced in TDS 7.2) 
		 * TableName=NumParts, {PartName}-; 
		 * ColName= HYPERLINK \l "B_VARCHAR_Def" B_VARCHAR; 
		 * ColumnData=UserType, Flags, [TableName], // <Only specified if text, //ntext or image columns are included //in the rowset being described> ColName; 
		 * NoMetaData='0xFF', '0xFF';
		 */
		enum column_flag_bits_according_to_microsoft {
			  case_sensitive	= 0x0001
			, nullable		= 0x0002
			, updateable		= 0x0004
			, might_be_updateable	= 0x0008
			, identity		= 0x0010
			, computed		= 0x0020
			, us_reserved_odbc	= 0x0040 | 0x0080
			, is_fixed_len_clr_type = 0x0100
			, is_hidden_browse_pk	= 0x0200
			, is_browse_pk		= 0x0400
			, might_be_nullable	= 0x0800 
		};
		/* TODO: implement members in TDSCOLUMN */
		if (IS_TDS72_PLUS(tds->conn)) {
			curcol->is_computed = 		(curcol->column_flags & (1 << 4)) > 1;
			curcol->us_reserved_odbc1 = 	(curcol->column_flags & (1 << 5)) > 1;
			curcol->us_reserved_odbc2 = 	(curcol->column_flags & (1 << 6)) > 1;
			curcol->is_fixed_len_clr_type = (curcol->column_flags & (1 << 7)) > 1;
		}
#endif 
	} 

	if (IS_TDS72_PLUS(tds->conn)) {
		tds_get_n(tds, NULL, 2);
#if 0
		/* TODO: implement members in TDSCOLUMN, values untested */
		curcol->us_reserved1 = (curcol->column_flags & 0x01);
		curcol->us_reserved2 = (curcol->column_flags & 0x02);
		curcol->us_reserved3 = (curcol->column_flags & 0x04);
		curcol->us_reserved4 = (curcol->column_flags & 0x08);
		curcol->is_hidden = (curcol->column_flags & 0x10);
		curcol->is_key = (curcol->column_flags & 0x20);
		curcol->is_nullable_unknown = (curcol->column_flags & 0x40);
#endif
	}
	
	curcol->column_usertype = tds_get_int(tds);
	tds_set_column_type(tds->conn, curcol, tds_get_byte(tds));

	tdsdump_log(TDS_DBG_INFO1, "processing result. type = %d(%s), varint_size %d\n",
		    curcol->column_type, tds_prtype(curcol->column_type), curcol->column_varint_size);

	curcol->funcs->get_info(tds, curcol);

	tdsdump_log(TDS_DBG_INFO1, "processing result. column_size %d\n", curcol->column_size);

	/* Adjust column size according to client's encoding */
	curcol->on_server.column_size = curcol->column_size;
	adjust_character_column_size(tds, curcol);

	return TDS_SUCCESS;
}

/**
 * tds_process_result() is the TDS 5.0 result set processing routine.  It 
 * is responsible for populating the tds->res_info structure.
 * This is a TDS 5.0 only function
 * \tds
 */
static TDSRET
tds_process_result(TDSSOCKET * tds)
{
	unsigned int col, num_cols;
	TDSCOLUMN *curcol;
	TDSRESULTINFO *info;
	TDSCURSOR *cursor;

	CHECK_TDS_EXTRA(tds);

	tds_free_all_results(tds);
	tds->rows_affected = TDS_NO_COUNT;

	tds_get_usmallint(tds);	/* header size */

	/* read number of columns and allocate the columns structure */
	num_cols = tds_get_usmallint(tds);

	if (tds->cur_cursor) {
		cursor = tds->cur_cursor; 
		if ((cursor->res_info = tds_alloc_results(num_cols)) == NULL)
			return TDS_FAIL;
		info = cursor->res_info;
		tds_set_current_results(tds, cursor->res_info);
	} else {
		if ((tds->res_info = tds_alloc_results(num_cols)) == NULL)
			return TDS_FAIL;
	
		info = tds->res_info;
		tds_set_current_results(tds, tds->res_info);
	}

	/*
	 * loop through the columns populating COLINFO struct from
	 * server response
	 */
	for (col = 0; col < info->num_cols; col++) {
		curcol = info->columns[col];

		tds_get_data_info(tds, curcol, 0);

		/* skip locale information */
		/* NOTE do not put into tds_get_data_info, param do not have locale information */
		tds_get_n(tds, NULL, tds_get_byte(tds));
	}
	return tds_alloc_row(info);
}

/**
 * tds5_process_result() is the new TDS 5.0 result set processing routine.  
 * It is responsible for populating the tds->res_info structure.
 * This is a TDS 5.0 only function
 * \tds
 */
static TDSRET
tds5_process_result(TDSSOCKET * tds)
{
	unsigned int colnamelen;
	TDS_USMALLINT col, num_cols;
	TDSCOLUMN *curcol;
	TDSRESULTINFO *info;

	CHECK_TDS_EXTRA(tds);

	tdsdump_log(TDS_DBG_INFO1, "tds5_process_result\n");

	/*
	 * free previous resultset
	 */
	tds_free_all_results(tds);
	tds->rows_affected = TDS_NO_COUNT;

	/*
	 * read length of packet (4 bytes)
	 */
	tds_get_uint(tds);

	/* read number of columns and allocate the columns structure */
	num_cols = tds_get_usmallint(tds);

	if ((info = tds_alloc_results(num_cols)) == NULL)
		return TDS_FAIL;
	tds_set_current_results(tds, info);
	if (tds->cur_cursor)
		tds->cur_cursor->res_info = info;
	else
		tds->res_info = info;

	tdsdump_log(TDS_DBG_INFO1, "num_cols=%d\n", num_cols);

	/* TODO reuse some code... */
	/*
	 * loop through the columns populating COLINFO struct from
	 * server response
	 */
	for (col = 0; col < info->num_cols; col++) {
		curcol = info->columns[col];

		/* label */
		curcol->column_namelen =
			tds_get_string(tds, tds_get_byte(tds), curcol->column_name, sizeof(curcol->column_name) - 1);
		curcol->column_name[curcol->column_namelen] = '\0';

		/* TODO save informations somewhere */
		/* database */
		colnamelen = tds_get_byte(tds);
		tds_get_n(tds, NULL, colnamelen);
		/*
		 * tds_get_n(tds, curcol->catalog_name, colnamelen);
		 * curcol->catalog_name[colnamelen] = '\0';
		 */

		/* owner */
		colnamelen = tds_get_byte(tds);
		tds_get_n(tds, NULL, colnamelen);
		/*
		 * tds_get_n(tds, curcol->schema_name, colnamelen);
		 * curcol->schema_name[colnamelen] = '\0';
		 */

		/* table */
		/* TODO use with owner and database */
		tds_dstr_get(tds, &curcol->table_name, tds_get_byte(tds));

		/* table column name */
		tds_dstr_get(tds, &curcol->table_column_name, tds_get_byte(tds));

		/* if label is empty, use the table column name */
		if (!curcol->column_namelen && !tds_dstr_isempty(&curcol->table_column_name)) {
			tds_strlcpy(curcol->column_name, tds_dstr_cstr(&curcol->table_column_name), sizeof(curcol->column_name));
			curcol->column_namelen = (TDS_SMALLINT)strlen(curcol->column_name);
		}

		/* flags (4 bytes) */
		curcol->column_flags = tds_get_int(tds);
		curcol->column_hidden = curcol->column_flags & 0x1;
		curcol->column_key = (curcol->column_flags & 0x2) > 1;
		curcol->column_writeable = (curcol->column_flags & 0x10) > 1;
		curcol->column_nullable = (curcol->column_flags & 0x20) > 1;
		curcol->column_identity = (curcol->column_flags & 0x40) > 1;

		curcol->column_usertype = tds_get_int(tds);

		tds_set_column_type(tds->conn, curcol, tds_get_byte(tds));

		curcol->funcs->get_info(tds, curcol);

		/* Adjust column size according to client's encoding */
		curcol->on_server.column_size = curcol->column_size;
		adjust_character_column_size(tds, curcol);

		/* discard Locale */
		tds_get_n(tds, NULL, tds_get_byte(tds));

		/* 
		 *  Dump all information on this column
		 */
		tdsdump_log(TDS_DBG_INFO1, "col %d:\n", col);
		tdsdump_log(TDS_DBG_INFO1, "\tcolumn_name=[%s]\n", curcol->column_name);
/*
		tdsdump_log(TDS_DBG_INFO1, "\tcolumn_name=[%s]\n", curcol->column_colname);
		tdsdump_log(TDS_DBG_INFO1, "\tcatalog=[%s] schema=[%s] table=[%s]\n",
			    curcol->catalog_name, curcol->schema_name, curcol->table_name, curcol->column_colname);
*/
		tdsdump_log(TDS_DBG_INFO1, "\tflags=%x utype=%d type=%d varint=%d\n",
			    curcol->column_flags, curcol->column_usertype, curcol->column_type, curcol->column_varint_size);

		tdsdump_log(TDS_DBG_INFO1, "\tcolsize=%d prec=%d scale=%d\n",
			    curcol->column_size, curcol->column_prec, curcol->column_scale);
	}
	return tds_alloc_row(info);
}

/**
 * tds_process_compute() processes compute rows and places them in the row
 * buffer.
 * \tds
 * \param pcomputeid store id of compute token
 */
static TDSRET
tds_process_compute(TDSSOCKET * tds, TDS_INT * pcomputeid)
{
	unsigned int i;
	TDSCOLUMN *curcol;
	TDSCOMPUTEINFO *info;
	TDS_INT id;

	CHECK_TDS_EXTRA(tds);

	id = tds_get_smallint(tds);
	
	tdsdump_log(TDS_DBG_INFO1, "tds_process_compute() found compute id %d\n", id);

	for (i = 0;; ++i) {
		if (i >= tds->num_comp_info) {
			tdsdump_log(TDS_DBG_INFO1, "tds_process_compute() FAIL: id exceeds bound (%d)\n", tds->num_comp_info);
			return TDS_FAIL;
		}
		info = tds->comp_info[i];
		if (info->computeid == id)
			break;
	}
	tds_set_current_results(tds, info);

	for (i = 0; i < info->num_cols; i++) {
		curcol = info->columns[i];
		if (TDS_FAILED(curcol->funcs->get_data(tds, curcol))) {
			tdsdump_log(TDS_DBG_INFO1, "tds_process_compute() FAIL: get_data() failed\n");
			return TDS_FAIL;
		}
	}
	if (pcomputeid)
		*pcomputeid = id;
	return TDS_SUCCESS;
}

/**
 * tds_process_row() processes rows and places them in the row buffer.
 * \tds
 */
static TDSRET
tds_process_row(TDSSOCKET * tds)
{
	unsigned int i;
	TDSCOLUMN *curcol;
	TDSRESULTINFO *info;

	CHECK_TDS_EXTRA(tds);

	info = tds->current_results;
	if (!info)
		return TDS_FAIL;

	assert(info->num_cols > 0);
	
	for (i = 0; i < info->num_cols; i++) {
		tdsdump_log(TDS_DBG_INFO1, "tds_process_row(): reading column %d \n", i);
		curcol = info->columns[i];
		if (TDS_FAILED(curcol->funcs->get_data(tds, curcol)))
			return TDS_FAIL;
	}
	return TDS_SUCCESS;
}

/**
 * tds_process_nbcrow() processes rows and places them in the row buffer.
 */
static TDSRET
tds_process_nbcrow(TDSSOCKET * tds)
{
	unsigned int i;
	TDSCOLUMN *curcol;
	TDSRESULTINFO *info;
	char *nbcbuf;

	CHECK_TDS_EXTRA(tds);

	info = tds->current_results;
	if (!info)
		return TDS_FAIL;

	assert(info->num_cols > 0);

	nbcbuf = alloca((info->num_cols + 7) / 8);
	tds_get_n(tds, nbcbuf, (info->num_cols + 7) / 8);
	for (i = 0; i < info->num_cols; i++) {
		curcol = info->columns[i];
		tdsdump_log(TDS_DBG_INFO1, "tds_process_nbcrow(): reading column %d \n", i);
		if (nbcbuf[i / 8] & (1 << (i % 8))) {
			curcol->column_cur_size = -1;
		} else {
			if (TDS_FAILED(curcol->funcs->get_data(tds, curcol)))
				return TDS_FAIL;
		}
	}
	return TDS_SUCCESS;
}

/**
 * tds_process_end() processes any of the DONE, DONEPROC, or DONEINPROC
 * tokens.
 * \param tds        state information for the socket and the TDS protocol
 * \param marker     TDS token number
 * \param flags_parm filled with bit flags (see TDS_DONE_ constants). 
 *        Is NULL nothing is returned
 */
static TDSRET
tds_process_end(TDSSOCKET * tds, int marker, int *flags_parm)
{
	int more_results, was_cancelled, error, done_count_valid;
	int tmp;
	TDS_INT8 rows_affected;

	CHECK_TDS_EXTRA(tds);

	tmp = tds_get_usmallint(tds);

	tds_get_smallint(tds);	/* state */

	more_results = (tmp & TDS_DONE_MORE_RESULTS) != 0;
	was_cancelled = (tmp & TDS_DONE_CANCELLED) != 0;
	error = (tmp & TDS_DONE_ERROR) != 0;
	done_count_valid = (tmp & TDS_DONE_COUNT) != 0;


	tdsdump_log(TDS_DBG_FUNC, "tds_process_end: more_results = %d\n"
		    "\t\twas_cancelled = %d\n"
		    "\t\terror = %d\n"
		    "\t\tdone_count_valid = %d\n", more_results, was_cancelled, error, done_count_valid);

	if (tds->res_info) {
		tds->res_info->more_results = more_results;
		/* FIXME this should not happen !!! */
		if (tds->current_results == NULL)
			tds_set_current_results(tds, tds->res_info);

	}

	if (flags_parm)
		*flags_parm = tmp;

	rows_affected = IS_TDS72_PLUS(tds->conn) ? tds_get_int8(tds) : tds_get_int(tds);
	tdsdump_log(TDS_DBG_FUNC, "                rows_affected = %" PRId64 "\n", rows_affected);

	if (was_cancelled || (!more_results && !tds->in_cancel)) {
		tdsdump_log(TDS_DBG_FUNC, "tds_process_end() state set to TDS_IDLE\n");
		/* reset of in_cancel should must done before setting IDLE */
		tds->in_cancel = 0;
		tds_set_state(tds, TDS_IDLE);
	}

	if (IS_TDSDEAD(tds))
		return TDS_FAIL;

	/*
	 * rows affected is in the tds struct because a query may affect rows but
	 * have no result set.
	 */

	if (done_count_valid)
		tds->rows_affected = rows_affected;
	else
		tds->rows_affected = TDS_NO_COUNT;

	if (IS_TDSDEAD(tds))
		return TDS_FAIL;

	return was_cancelled ? TDS_CANCELLED : TDS_SUCCESS;
}

/**
 * tds_process_env_chg() 
 * when ever certain things change on the server, such as database, character
 * set, language, or block size.  A environment change message is generated
 * There is no action taken currently, but certain functions at the CLI level
 * that return the name of the current database will need to use this.
 * \tds
 */
static TDSRET
tds_process_env_chg(TDSSOCKET * tds)
{
	unsigned int size;
	TDS_TINYINT type;
	char *oldval = NULL;
	char *newval = NULL;
	char **dest;
	int new_block_size;
	int lcid;
	int memrc = 0;

	CHECK_TDS_EXTRA(tds);

	size = tds_get_usmallint(tds);
	if (TDS_UNLIKELY(size < 1)) {
		tdsdump_log(TDS_DBG_ERROR, "Got invalid size %u\n", size);
		tds_close_socket(tds);
		return TDS_FAIL;
	}

	/*
	 * this came in a patch, apparently someone saw an env message
	 * that was different from what we are handling? -- brian
	 * changed back because it won't handle multibyte chars -- 7.0
	 */
	/* tds_get_n(tds,NULL,size); */

	type = tds_get_byte(tds);

	/*
	 * handle collate default change (if you change db or during login)
	 * this environment is not a string so need different handles
	 */
	if (type == TDS_ENV_SQLCOLLATION) {
		/* save new collation */
		size = tds_get_byte(tds);
		tdsdump_log(TDS_DBG_ERROR, "tds_process_env_chg(): %d bytes of collation data received\n", size);
		tdsdump_dump_buf(TDS_DBG_NETWORK, "tds->conn->collation was", tds->conn->collation, 5);
		memset(tds->conn->collation, 0, 5);
		if (size < 5) {
			tds_get_n(tds, tds->conn->collation, size);
		} else {
			tds_get_n(tds, tds->conn->collation, 5);
			tds_get_n(tds, NULL, size - 5);
			lcid = (tds->conn->collation[0] + ((int) tds->conn->collation[1] << 8) + ((int) tds->conn->collation[2] << 16)) & 0xffffflu;
			tds7_srv_charset_changed(tds->conn, tds->conn->collation[4], lcid);
		}
		tdsdump_dump_buf(TDS_DBG_NETWORK, "tds->conn->collation now", tds->conn->collation, 5);
		/* discard old one */
		tds_get_n(tds, NULL, tds_get_byte(tds));
		return TDS_SUCCESS;
	}

	if (type == TDS_ENV_BEGINTRANS) {
		/* TODO check size */
		size = tds_get_byte(tds);
		tds_get_n(tds, tds->conn->tds72_transaction, 8);
		tds_get_n(tds, NULL, tds_get_byte(tds));
		return TDS_SUCCESS;
	}

	if (type == TDS_ENV_COMMITTRANS || type == TDS_ENV_ROLLBACKTRANS) {
		memset(tds->conn->tds72_transaction, 0, 8);
		tds_get_n(tds, NULL, tds_get_byte(tds));
		tds_get_n(tds, NULL, tds_get_byte(tds));
		return TDS_SUCCESS;
	}

	/* discard byte values, not still supported */
	/* TODO support them */
	if (IS_TDS71_PLUS(tds->conn) && type > TDS_ENV_PACKSIZE) {
		/* discard rest of the packet */
		tds_get_n(tds, NULL, size - 1);
		return TDS_SUCCESS;
	}

	/* fetch the new value */
	memrc += tds_alloc_get_string(tds, &newval, tds_get_byte(tds));

	/* fetch the old value */
	memrc += tds_alloc_get_string(tds, &oldval, tds_get_byte(tds));

	if (memrc != 0) {
		free(newval);
		free(oldval);
		return TDS_FAIL;
	}

	dest = NULL;
	switch (type) {
	case TDS_ENV_PACKSIZE:
		new_block_size = atoi(newval);
		if (new_block_size >= 512) {
			tdsdump_log(TDS_DBG_INFO1, "changing block size from %s to %d\n", oldval, new_block_size);
			/* 
			 * Is possible to have a shrink if server limits packet
			 * size more than what we specified
			 */
			/* Reallocate buffer if possible (strange values from server or out of memory) use older buffer */
			tds_realloc_socket(tds, new_block_size);
		}
		break;
	case TDS_ENV_DATABASE:
		dest = &tds_conn(tds)->env.database;
		break;
	case TDS_ENV_LANG:
		dest = &tds_conn(tds)->env.language;
		break;
	case TDS_ENV_CHARSET:
		tdsdump_log(TDS_DBG_FUNC, "server indicated charset change to \"%s\"\n", newval);
		dest = &tds_conn(tds)->env.charset;
		tds_srv_charset_changed(tds->conn, newval);
		break;
	}
	if (tds->env_chg_func) {
		(*(tds->env_chg_func)) (tds, type, oldval, newval);
	}

	free(oldval);
	if (newval) {
		if (dest) {
			if (*dest)
				free(*dest);
			*dest = newval;
		} else
			free(newval);
	}

	return TDS_SUCCESS;
}

/**
 * tds_process_msg() is called for MSG, ERR, or EED tokens and is responsible
 * for calling the CLI's message handling routine
 * \returns TDS_SUCCESS if informational, TDS_FAIL if error.
 */
static TDSRET
tds_process_msg(TDSSOCKET * tds, int marker)
{
	int rc;
	unsigned int len_sqlstate;
	int has_eed = 0;
	TDSMESSAGE msg;

	CHECK_TDS_EXTRA(tds);

	/* make sure message has been freed */
	memset(&msg, 0, sizeof(TDSMESSAGE));

	/* packet length */
	tds_get_smallint(tds);

	/* message number */
	msg.msgno = tds_get_int(tds);

	/* msg state */
	msg.state = tds_get_byte(tds);

	/* msg level */
	msg.severity = tds_get_byte(tds);

	/* determine if msg or error */
	switch (marker) {
	case TDS_EED_TOKEN:
		if (msg.severity <= 10)
			msg.priv_msg_type = 0;
		else
			msg.priv_msg_type = 1;

		/* read SQL state */
		len_sqlstate = tds_get_byte(tds);
		msg.sql_state = (char *) malloc(len_sqlstate + 1);
		if (!msg.sql_state) {
			tds_free_msg(&msg);
			return TDS_FAIL;
		}

		tds_get_n(tds, msg.sql_state, len_sqlstate);
		msg.sql_state[len_sqlstate] = '\0';

		/* do a better mapping using native errors */
		if (strcmp(msg.sql_state, "ZZZZZ") == 0)
			TDS_ZERO_FREE(msg.sql_state);

		/* if has_eed = 1, extended error data follows */
		has_eed = tds_get_byte(tds);

		/* junk status and transaction state */
		tds_get_smallint(tds);
		break;
	case TDS_INFO_TOKEN:
		msg.priv_msg_type = 0;
		break;
	case TDS_ERROR_TOKEN:
		msg.priv_msg_type = 1;
		break;
	default:
		tdsdump_log(TDS_DBG_ERROR, "tds_process_msg() called with unknown marker '%d'!\n", (int) marker);
		tds_free_msg(&msg);
		return TDS_FAIL;
	}

	tdsdump_log(TDS_DBG_ERROR, "tds_process_msg() reading message %d from server\n", msg.msgno);
	
	rc = 0;
	/* the message */
	rc += tds_alloc_get_string(tds, &msg.message, tds_get_usmallint(tds));

	/* server name */
	rc += tds_alloc_get_string(tds, &msg.server, tds_get_byte(tds));

	if ((!msg.server || !msg.server[0]) && tds->login) {
		TDS_ZERO_FREE(msg.server);
		if (-1 == asprintf(&msg.server, "[%s]", tds_dstr_cstr(&tds->login->server_name))) {
			tdsdump_log(TDS_DBG_ERROR, "out of memory (%d), %s\n", errno, strerror(errno));
			return TDS_FAIL;
		}
	}

	/* stored proc name if available */
	rc += tds_alloc_get_string(tds, &msg.proc_name, tds_get_byte(tds));

	/* line number in the sql statement where the problem occured */
	msg.line_number = IS_TDS72_PLUS(tds->conn) ? tds_get_int(tds) : tds_get_smallint(tds);

	/*
	 * If the server doesen't provide an sqlstate, map one via server native errors
	 * I'm assuming there is not a protocol I'm missing to fetch these from the server?
	 * I know sybase has an sqlstate column in it's sysmessages table, mssql doesn't and
	 * TDS_EED_TOKEN is not being called for me.
	 */
	if (msg.sql_state == NULL)
		msg.sql_state = tds_alloc_lookup_sqlstate(tds, msg.msgno);


	/* In case extended error data is sent, we just try to discard it */
	if (has_eed == 1) {
		int next_marker;
		for (;;) {
			switch (next_marker = tds_get_byte(tds)) {
			case TDS5_PARAMFMT_TOKEN:
			case TDS5_PARAMFMT2_TOKEN:
			case TDS5_PARAMS_TOKEN:
				if (TDS_FAILED(tds_process_default_tokens(tds, next_marker)))
					--rc;
				continue;
			}
			break;
		}
		tds_unget_byte(tds);
	}

	/*
	 * call the msg_handler that was set by an upper layer
	 * (dblib, ctlib or some other one).  Call it with the pointer to
	 * the "parent" structure.
	 */

	if (rc != 0) {
		tds_free_msg(&msg);
		return TDS_FAIL;
	}

	/* special case, */
	if (marker == TDS_EED_TOKEN && tds->cur_dyn && !TDS_IS_MSSQL(tds) && msg.msgno == 2782) {
		/* we must emulate prepare */
		tds->cur_dyn->emulated = 1;
		tds_dynamic_deallocated(tds->conn, tds->cur_dyn);
	} else if (marker == TDS_INFO_TOKEN && msg.msgno == 16954 && TDS_IS_MSSQL(tds)
		   && tds->current_op == TDS_OP_CURSOROPEN && tds->cur_cursor) {
		/* here mssql say "Executing SQL directly; no cursor." opening cursor */
	} else {

		if (tds_get_ctx(tds)->msg_handler) {
			tdsdump_log(TDS_DBG_ERROR, "tds_process_msg() calling client msg handler\n");
			tds_get_ctx(tds)->msg_handler(tds_get_ctx(tds), tds, &msg);
		} else if (msg.msgno) {
			tdsdump_log(TDS_DBG_WARN,
				    "Msg %d, Severity %d, State %d, Server %s, Line %d\n%s\n",
				    msg.msgno,
				    msg.severity ,
				    msg.state, msg.server, msg.line_number, msg.message);
		}
	}
	
	tds_free_msg(&msg);
	
	tdsdump_log(TDS_DBG_ERROR, "tds_process_msg() returning TDS_SUCCESS\n");
	
	return TDS_SUCCESS;
}

/**
 * Reads a string from wire in a new allocated buffer
 * \tds
 * \param string output string
 * \param len length of string to read
 * \returns 0 for success, -1 on error.
 */
static int
tds_alloc_get_string(TDSSOCKET * tds, char **string, size_t len)
{
	char *s;
	size_t out_len;

	CHECK_TDS_EXTRA(tds);

	/* assure sufficient space for every conversion */
	s = (char *) malloc(len * 4 + 1);
	out_len = tds_get_string(tds, len, s, len * 4);
	if (!s) {
		*string = NULL;
		return -1;
	}
	s = (char*) realloc(s, out_len + 1);
	s[out_len] = '\0';
	*string = s;
	return 0;
}

/**
 * \remarks Process the incoming token stream until it finds
 * an end token (DONE, DONEPROC, DONEINPROC) with the cancel flag set.
 * At that point the connection should be ready to handle a new query.
 * \tds
 */
TDSRET
tds_process_cancel(TDSSOCKET * tds)
{
	CHECK_TDS_EXTRA(tds);

	/* silly cases, nothing to do */
	if (!tds->in_cancel)
		return TDS_SUCCESS;
	/* TODO handle cancellation sending data */
	if (tds->state != TDS_PENDING)
		return TDS_SUCCESS;

	/* TODO support TDS5 cancel, wait for cancel packet first, then wait for done */
	for (;;) {
		TDS_INT result_type;

		switch (tds_process_tokens(tds, &result_type, NULL, 0)) {
		case TDS_FAIL:
			return TDS_FAIL;
		case TDS_CANCELLED:
		case TDS_SUCCESS:
		case TDS_NO_MORE_RESULTS:
			return TDS_SUCCESS;
		}
	}
}

/**
 * Finds a dynamic given string id
 * \return dynamic or NULL is not found
 * \param conn state information for the socket and the TDS protocol
 * \param id   dynamic id to search
 */
TDSDYNAMIC *
tds_lookup_dynamic(TDSCONNECTION * conn, const char *id)
{
	TDSDYNAMIC *curr;

	CHECK_CONN_EXTRA(conn);
	
	for (curr = conn->dyns; curr != NULL; curr = curr->next) {
		if (!strcmp(curr->id, id))
			return curr;
	}
	return NULL;
}

/**
 * tds_process_dynamic()
 * finds the element of the dyns array for the id
 * \tds
 * \return allocated dynamic or NULL on failure.
 */
static TDSDYNAMIC *
tds_process_dynamic(TDSSOCKET * tds)
{
	unsigned int token_sz;
	unsigned char type;
	TDS_TINYINT id_len, drain = 0;
	char id[TDS_MAX_DYNID_LEN + 1];

	CHECK_TDS_EXTRA(tds);

	token_sz = tds_get_usmallint(tds);
	type = tds_get_byte(tds);
	tds_get_byte(tds);	/* status */
	/* handle only acknowledge */
	if (type != TDS_DYN_ACK) {
		tdsdump_log(TDS_DBG_ERROR, "Unrecognized TDS5_DYN type %x\n", type);
		tds_get_n(tds, NULL, token_sz - 2);
		return NULL;
	}
	id_len = tds_get_byte(tds);
	if (id_len > TDS_MAX_DYNID_LEN) {
		drain = id_len - TDS_MAX_DYNID_LEN;
		id_len = TDS_MAX_DYNID_LEN;
	}
	id_len = tds_get_string(tds, id_len, id, TDS_MAX_DYNID_LEN);
	id[id_len] = '\0';
	if (drain) {
		tds_get_n(tds, NULL, drain);
	}
	return tds_lookup_dynamic(tds_conn(tds), id);
}

/**
 * Process results from dynamic.
 * \tds
 */
static TDSRET
tds_process_dyn_result(TDSSOCKET * tds)
{
	unsigned int col, num_cols;
	TDSCOLUMN *curcol;
	TDSPARAMINFO *info;
	TDSDYNAMIC *dyn;

	CHECK_TDS_EXTRA(tds);

	tds_get_usmallint(tds);	/* header size */
	num_cols = tds_get_usmallint(tds);

	if (tds->cur_dyn) {
		dyn = tds->cur_dyn;
		tds_free_param_results(dyn->res_info);
		/* read number of columns and allocate the columns structure */
		if ((dyn->res_info = tds_alloc_results(num_cols)) == NULL)
			return TDS_FAIL;
		info = dyn->res_info;
	} else {
		tds_free_param_results(tds->param_info);
		if ((tds->param_info = tds_alloc_results(num_cols)) == NULL)
			return TDS_FAIL;
		info = tds->param_info;
	}
	tds_set_current_results(tds, info);

	for (col = 0; col < info->num_cols; col++) {
		curcol = info->columns[col];

		tds_get_data_info(tds, curcol, 1);

		/* skip locale information */
		tds_get_n(tds, NULL, tds_get_byte(tds));
	}

	return tds_alloc_row(info);
}

/**
 * Process new TDS 5.0 token for describing output parameters
 * \tds
 */
static TDSRET
tds5_process_dyn_result2(TDSSOCKET * tds)
{
	unsigned int col, num_cols;
	TDSCOLUMN *curcol;
	TDSPARAMINFO *info;
	TDSDYNAMIC *dyn;

	CHECK_TDS_EXTRA(tds);

	tds_get_uint(tds);	/* header size */
	num_cols = tds_get_usmallint(tds);

	if (tds->cur_dyn) {
		dyn = tds->cur_dyn;
		tds_free_param_results(dyn->res_info);
		/* read number of columns and allocate the columns structure */
		if ((dyn->res_info = tds_alloc_results(num_cols)) == NULL)
			return TDS_FAIL;
		info = dyn->res_info;
	} else {
		tds_free_param_results(tds->param_info);
		if ((tds->param_info = tds_alloc_results(num_cols)) == NULL)
			return TDS_FAIL;
		info = tds->param_info;
	}
	tds_set_current_results(tds, info);

	for (col = 0; col < info->num_cols; col++) {
		curcol = info->columns[col];

		/* TODO reuse tds_get_data_info code, sligthly different */

		/* column name */
		curcol->column_namelen =
			tds_get_string(tds, tds_get_byte(tds), curcol->column_name, sizeof(curcol->column_name) - 1);
		curcol->column_name[curcol->column_namelen] = '\0';

		/* column status */
		curcol->column_flags = tds_get_int(tds);
		curcol->column_nullable = (curcol->column_flags & 0x20) > 0;

		/* user type */
		curcol->column_usertype = tds_get_int(tds);

		/* column type */
		tds_set_column_type(tds->conn, curcol, tds_get_byte(tds));

		curcol->funcs->get_info(tds, curcol);

		/* Adjust column size according to client's encoding */
		curcol->on_server.column_size = curcol->column_size;
		adjust_character_column_size(tds, curcol);

		/* discard Locale */
		tds_get_n(tds, NULL, tds_get_byte(tds));

		tdsdump_log(TDS_DBG_INFO1, "elem %d:\n", col);
		tdsdump_log(TDS_DBG_INFO1, "\tcolumn_name=[%s]\n", curcol->column_name);
		tdsdump_log(TDS_DBG_INFO1, "\tflags=%x utype=%d type=%d varint=%d\n",
			    curcol->column_flags, curcol->column_usertype, curcol->column_type, curcol->column_varint_size);
		tdsdump_log(TDS_DBG_INFO1, "\tcolsize=%d prec=%d scale=%d\n",
			    curcol->column_size, curcol->column_prec, curcol->column_scale);
	}

	return tds_alloc_row(info);
}

/**
 * tds_get_token_size() returns the size of a fixed length token
 * used by tds_process_cancel() to determine how to read past a token
 * \param marker token type.
 */
int
tds_get_token_size(int marker)
{
	/* TODO finish */
	switch (marker) {
	case TDS_DONE_TOKEN:
	case TDS_DONEPROC_TOKEN:
	case TDS_DONEINPROC_TOKEN:
		return 8;
	case TDS_RETURNSTATUS_TOKEN:
		return 4;
	case TDS_PROCID_TOKEN:
		return 8;
	default:
		return 0;
	}
}

#ifdef WORDS_BIGENDIAN
void
tds_swap_datatype(int coltype, unsigned char *buf)
{
	switch (coltype) {
	case SYBINT2:
		tds_swap_bytes(buf, 2);
		break;
	case SYBINT4:
	case SYBMONEY4:
	case SYBREAL:
		tds_swap_bytes(buf, 4);
		break;
	case SYBINT8:
	case SYBFLT8:
		tds_swap_bytes(buf, 8);
		break;
	case SYBMONEY:
	case SYBDATETIME:
		tds_swap_bytes(buf, 4);
		tds_swap_bytes(&buf[4], 4);
		break;
	case SYBDATETIME4:
		tds_swap_bytes(buf, 2);
		tds_swap_bytes(&buf[2], 2);
		break;
	case SYBUNIQUE:
		tds_swap_bytes(buf, 4);
		tds_swap_bytes(&buf[4], 2);
		tds_swap_bytes(&buf[6], 2);
		break;
	}
}
#endif

/**
 * Converts numeric from Microsoft representation to internal one (Sybase).
 * \param num numeric data to convert
 */
void
tds_swap_numeric(TDS_NUMERIC *num)
{
	/* swap the sign */
	num->array[0] = (num->array[0] == 0) ? 1 : 0;
	/* swap the data */
	tds_swap_bytes(&(num->array[1]), tds_numeric_bytes_per_prec[num->precision] - 1);
}


/**
 * tds_process_compute_names() processes compute result sets.
 * \tds
 */
static TDSRET
tds_process_compute_names(TDSSOCKET * tds)
{
	int hdrsize;
	int num_cols = 0;
	TDS_USMALLINT compute_id = 0;
	TDSCOMPUTEINFO *info;

	struct namelist *head = NULL, *cur;

	CHECK_TDS_EXTRA(tds);

	hdrsize = tds_get_usmallint(tds);
	tdsdump_log(TDS_DBG_INFO1, "processing tds5 compute names. hdrsize = %d\n", hdrsize);

	/*
	 * compute statement id which this relates
	 * to. You can have more than one compute
	 * statement in a SQL statement  
	 */
	compute_id = tds_get_usmallint(tds);

	if ((num_cols = tds_read_namelist(tds, hdrsize - 2, &head, 0)) < 0)
		return TDS_FAIL;

	tdsdump_log(TDS_DBG_INFO1, "processing tds5 compute names. num_cols = %d\n", num_cols);

	if ((tds->comp_info = tds_alloc_compute_results(tds, num_cols, 0)) != NULL) {
		int col;

		tdsdump_log(TDS_DBG_INFO1, "processing tds5 compute names. num_comp_info = %d\n", tds->num_comp_info);

		info = tds->comp_info[tds->num_comp_info - 1];
		tds_set_current_results(tds, info);

		info->computeid = compute_id;

		cur = head;
		for (col = 0; col < num_cols; col++) {
			TDSCOLUMN *curcol = info->columns[col];

			assert(strlen(curcol->column_name) == curcol->column_namelen);
			tds_strlcpy(curcol->column_name, cur->name, sizeof(curcol->column_name));
			curcol->column_namelen = (TDS_SMALLINT)strlen(curcol->column_name);

			cur = cur->next;
		}
		tds_free_namelist(head);
		return TDS_SUCCESS;
	}

	tds_free_namelist(head);
	return TDS_FAIL;
}

/**
 * tds7_process_compute_result() processes compute result sets for TDS 7/8.
 * They is are very  similar to normal result sets.
 * \tds
 */
static TDSRET
tds7_process_compute_result(TDSSOCKET * tds)
{
	unsigned int col, num_cols;
	TDS_TINYINT by_cols;
	TDS_SMALLINT *cur_by_col;
	TDS_USMALLINT compute_id;
	TDSCOLUMN *curcol;
	TDSCOMPUTEINFO *info;

	CHECK_TDS_EXTRA(tds);

	/*
	 * number of compute columns returned - so
	 * COMPUTE SUM(x), AVG(x)... would return
	 * num_cols = 2
	 */

	num_cols = tds_get_usmallint(tds);

	tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. num_cols = %u\n", num_cols);

	/*
	 * compute statement id which this relates
	 * to. You can have more than one compute
	 * statement in a SQL statement
	 */

	compute_id = tds_get_usmallint(tds);

	tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. compute_id = %u\n", compute_id);
	/*
	 * number of "by" columns in compute - so
	 * COMPUTE SUM(x) BY a, b, c would return
	 * by_cols = 3
	 */

	by_cols = tds_get_byte(tds);
	tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. by_cols = %d\n", by_cols);

	if ((tds->comp_info = tds_alloc_compute_results(tds, num_cols, by_cols)) == NULL)
		return TDS_FAIL;

	tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. num_comp_info = %d\n", tds->num_comp_info);

	info = tds->comp_info[tds->num_comp_info - 1];
	tds_set_current_results(tds, info);

	tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. point 0\n");

	info->computeid = compute_id;

	/*
	 * the by columns are a list of the column
	 * numbers in the select statement
	 */

	cur_by_col = info->bycolumns;
	for (col = 0; col < by_cols; col++) {
		*cur_by_col = tds_get_smallint(tds);
		cur_by_col++;
	}
	tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. point 1\n");

	for (col = 0; col < num_cols; col++) {
		tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. point 2\n");
		curcol = info->columns[col];

		curcol->column_operator = tds_get_byte(tds);
		curcol->column_operand = tds_get_smallint(tds);

		tds7_get_data_info(tds, curcol);

		if (!curcol->column_namelen) {
			strcpy(curcol->column_name, tds_pr_op(curcol->column_operator));
			curcol->column_namelen = (TDS_SMALLINT)strlen(curcol->column_name);
		}
	}

	/* all done now allocate a row for tds_process_row to use */
	tdsdump_log(TDS_DBG_INFO1, "processing tds7 compute result. point 5 \n");
	return tds_alloc_compute_row(info);
}

/**
 * Reads cursor command results.
 * This contains status of cursors.
 * \tds
 */
static TDSRET
tds_process_cursor_tokens(TDSSOCKET * tds)
{
	TDS_USMALLINT hdrsize;
	TDS_INT cursor_id;
	TDS_TINYINT namelen;
	TDS_USMALLINT cursor_status;
	TDSCURSOR *cursor;

	CHECK_TDS_EXTRA(tds);
	
	hdrsize  = tds_get_usmallint(tds);
	cursor_id = tds_get_int(tds);
	hdrsize  -= sizeof(TDS_INT);
	if (cursor_id == 0){
		namelen = tds_get_byte(tds);
		hdrsize -= 1;
		/* discard name */
		tds_get_n(tds, NULL, namelen);
		hdrsize -= namelen;
	}
	tds_get_byte(tds);	/* cursor command */
	cursor_status = tds_get_usmallint(tds);
	hdrsize -= 3;

	if (hdrsize == sizeof(TDS_INT))
		tds_get_int(tds); /* row count TODO useless ?? */

	if (tds->cur_cursor) {
		cursor = tds->cur_cursor; 
		cursor->cursor_id = cursor_id;
		cursor->srv_status = cursor_status;
		if ((cursor_status & TDS_CUR_ISTAT_DEALLOC) != 0)
			tds_cursor_deallocated(tds->conn, cursor);
	} 
	return TDS_SUCCESS;
}

/**
 * Process option cmd results.
 * This token is available only on TDS 5.0 (Sybase).
 * \tds
 */
static TDSRET
tds5_process_optioncmd(TDSSOCKET * tds)
{
	TDS_INT command;
	TDS_TINYINT option;
	TDS_TINYINT argsize;
	TDS_INT arg;

	CHECK_TDS_EXTRA(tds);

	tdsdump_log(TDS_DBG_INFO1, "tds5_process_optioncmd()\n");

	assert(IS_TDS50(tds->conn));

	tds_get_usmallint(tds);	/* length */
	command = tds_get_byte(tds);
	option = tds_get_byte(tds);
	argsize = tds_get_byte(tds);

	switch (argsize) {
	case 0:
		arg = 0;
		break;
	case 1:
		arg = tds_get_byte(tds);
		break;
	case 4:
		arg = tds_get_int(tds);
		break;
	default:
		tdsdump_log(TDS_DBG_INFO1, "oops: cannot process option %d of size %d\n", option, argsize);
		/* ignore argument */
		tds_get_n(tds, NULL, argsize);
		return TDS_FAIL;
	}
	tdsdump_log(TDS_DBG_INFO1, "received option %d value %d\n", option, arg);

	if (command != TDS_OPT_INFO)
		return TDS_FAIL;

	tds->option_value = arg;

	return TDS_SUCCESS;
}

/**
 * Returns string representation for a given operation
 * \param op operation code
 * \return string representation. Empty if not found.
 */
static const char *
tds_pr_op(int op)
{
/** \cond HIDDEN_SYMBOLS */
#define TYPE(con, s) case con: return s; break
/** \endcond */
	switch (op) {
		TYPE(SYBAOPAVG, "avg");
		TYPE(SYBAOPAVGU, "avg");
		TYPE(SYBAOPCNT, "count");
		TYPE(SYBAOPCNTU, "count");
		TYPE(SYBAOPMAX, "max");
		TYPE(SYBAOPMIN, "min");
		TYPE(SYBAOPSUM, "sum");
		TYPE(SYBAOPSUMU, "sum");
		TYPE(SYBAOPCHECKSUM_AGG, "checksum_agg");
		TYPE(SYBAOPCNT_BIG, "count");
		TYPE(SYBAOPSTDEV, "stdevp");
		TYPE(SYBAOPSTDEVP, "stdevp");
		TYPE(SYBAOPVAR, "var");
		TYPE(SYBAOPVARP, "varp");
	default:
		break;
	}
	return "";
#undef TYPE
}

/**
 * Returns string representation of the given type.
 * \param type data type
 * \return type as string. Empty if not found.
 */
const char *
tds_prtype(int type)
{
/** \cond HIDDEN_SYMBOLS */
#define TYPE(con, s) case con: return s; break
/** \endcond */
	switch (type) {
		TYPE(SYBAOPAVG, "avg");
		TYPE(SYBAOPCNT, "count");
		TYPE(SYBAOPMAX, "max");
		TYPE(SYBAOPMIN, "min");
		TYPE(SYBAOPSUM, "sum");

		TYPE(SYBBINARY, "binary");
		TYPE(SYBLONGBINARY, "longbinary");
		TYPE(SYBBIT, "bit");
		TYPE(SYBBITN, "bit-null");
		TYPE(SYBCHAR, "char");
		TYPE(SYBDATETIME4, "smalldatetime");
		TYPE(SYBDATETIME, "datetime");
		TYPE(SYBDATETIMN, "datetime-null");
		TYPE(SYBDECIMAL, "decimal");
		TYPE(SYBFLT8, "float");
		TYPE(SYBFLTN, "float-null");
		TYPE(SYBIMAGE, "image");
		TYPE(SYBINT1, "tinyint");
		TYPE(SYBINT2, "smallint");
		TYPE(SYBINT4, "int");
		TYPE(SYBINT8, "bigint");
		TYPE(SYBUINT1, "unsigned tinyint");
		TYPE(SYBUINT2, "unsigned smallint");
		TYPE(SYBUINT4, "unsigned int");
		TYPE(SYBUINT8, "unsigned bigint");
		TYPE(SYBINTN, "integer-null");
		TYPE(SYBMONEY4, "smallmoney");
		TYPE(SYBMONEY, "money");
		TYPE(SYBMONEYN, "money-null");
		TYPE(SYBNTEXT, "UCS-2 text");
		TYPE(SYBNVARCHAR, "UCS-2 varchar");
		TYPE(SYBNUMERIC, "numeric");
		TYPE(SYBREAL, "real");
		TYPE(SYBTEXT, "text");
		TYPE(SYBUNIQUE, "uniqueidentifier");
		TYPE(SYBVARBINARY, "varbinary");
		TYPE(SYBVARCHAR, "varchar");
		TYPE(SYBVARIANT, "variant");
		TYPE(SYBVOID, "void");
		TYPE(XSYBBINARY, "xbinary");
		TYPE(XSYBCHAR, "xchar");
		TYPE(XSYBNCHAR, "x UCS-2 char");
		TYPE(XSYBNVARCHAR, "x UCS-2 varchar");
		TYPE(XSYBVARBINARY, "xvarbinary");
		TYPE(XSYBVARCHAR, "xvarchar");
		TYPE(SYBMSXML, "xml");
		TYPE(SYBMSDATE, "date");
		TYPE(SYBMSTIME, "time");
		TYPE(SYBMSDATETIME2, "datetime2");
		TYPE(SYBMSDATETIMEOFFSET, "datetimeoffset");
	default:
		break;
	}
	return "";
#undef TYPE
}

/**
 * Returns string representation for a given token type
 * \param marker token type
 * \return string representation. Empty if not token not valid.
 */
static const char *
tds_token_name(unsigned char marker)
{
	switch (marker) {

	case TDS5_PARAMFMT2_TOKEN:
		return "TDS5_PARAMFMT2";
	case TDS_ORDERBY2_TOKEN:
		return "ORDERBY2";
	case TDS_ROWFMT2_TOKEN:
		return "ROWFMT2";
	case TDS_LOGOUT_TOKEN:
		return "LOGOUT";
	case TDS_RETURNSTATUS_TOKEN:
		return "RETURNSTATUS";
	case TDS_PROCID_TOKEN:
		return "PROCID";
	case TDS7_RESULT_TOKEN:
		return "TDS7_RESULT";
	case TDS_CURINFO_TOKEN:
		return "TDS_CURINFO";
	case TDS7_COMPUTE_RESULT_TOKEN:
		return "TDS7_COMPUTE_RESULT";
	case TDS_COLNAME_TOKEN:
		return "COLNAME";
	case TDS_COLFMT_TOKEN:
		return "COLFMT";
	case TDS_DYNAMIC2_TOKEN:
		return "DYNAMIC2";
	case TDS_TABNAME_TOKEN:
		return "TABNAME";
	case TDS_COLINFO_TOKEN:
		return "COLINFO";
	case TDS_COMPUTE_NAMES_TOKEN:
		return "COMPUTE_NAMES";
	case TDS_COMPUTE_RESULT_TOKEN:
		return "COMPUTE_RESULT";
	case TDS_ORDERBY_TOKEN:
		return "ORDERBY";
	case TDS_ERROR_TOKEN:
		return "ERROR";
	case TDS_INFO_TOKEN:
		return "INFO";
	case TDS_PARAM_TOKEN:
		return "PARAM";
	case TDS_LOGINACK_TOKEN:
		return "LOGINACK";
	case TDS_CONTROL_TOKEN:
		return "CONTROL";
	case TDS_ROW_TOKEN:
		return "ROW";
	case TDS_NBC_ROW_TOKEN:
		return "NBC_ROW";
	case TDS_CMP_ROW_TOKEN:
		return "CMP_ROW";
	case TDS5_PARAMS_TOKEN:
		return "TDS5_PARAMS";
	case TDS_CAPABILITY_TOKEN:
		return "CAPABILITY";
	case TDS_ENVCHANGE_TOKEN:
		return "ENVCHANGE";
	case TDS_EED_TOKEN:
		return "EED";
	case TDS_DBRPC_TOKEN:
		return "DBRPC";
	case TDS5_DYNAMIC_TOKEN:
		return "TDS5_DYNAMIC";
	case TDS5_PARAMFMT_TOKEN:
		return "TDS5_PARAMFMT";
	case TDS_AUTH_TOKEN:
		return "AUTH";
	case TDS_RESULT_TOKEN:
		return "RESULT";
	case TDS_DONE_TOKEN:
		return "DONE";
	case TDS_DONEPROC_TOKEN:
		return "DONEPROC";
	case TDS_DONEINPROC_TOKEN:
		return "DONEINPROC";

	default:
		break;
	}

	return "";
}

/** 
 * Adjust column size according to client's encoding
 * \tds
 * \param curcol column to adjust
 */
static void
adjust_character_column_size(TDSSOCKET * tds, TDSCOLUMN * curcol)
{
	CHECK_TDS_EXTRA(tds);
	CHECK_COLUMN_EXTRA(curcol);

	if (is_unicode_type(curcol->on_server.column_type))
		curcol->char_conv = tds->conn->char_convs[client2ucs2];

	/* Sybase UNI(VAR)CHAR fields are transmitted via SYBLONGBINARY and in UTF-16 */
	if (curcol->on_server.column_type == SYBLONGBINARY && (
		curcol->column_usertype == USER_UNICHAR_TYPE ||
		curcol->column_usertype == USER_UNIVARCHAR_TYPE)) {
#ifdef WORDS_BIGENDIAN
		static const char sybase_utf[] = "UTF-16BE";
#else
		static const char sybase_utf[] = "UTF-16LE";
#endif

		curcol->char_conv = tds_iconv_get(tds->conn, tds->conn->char_convs[client2ucs2]->from.charset.name, sybase_utf);

		/* fallback to UCS-2LE */
		/* FIXME should be useless. Does not works always */
		if (!curcol->char_conv)
			curcol->char_conv = tds->conn->char_convs[client2ucs2];
	}

	/* FIXME: and sybase ?? */
	if (!curcol->char_conv && IS_TDS7_PLUS(tds->conn) && is_ascii_type(curcol->on_server.column_type))
		curcol->char_conv = tds->conn->char_convs[client2server_chardata];

	if (!USE_ICONV || !curcol->char_conv)
		return;

	curcol->on_server.column_size = curcol->column_size;
	curcol->column_size = determine_adjusted_size(curcol->char_conv, curcol->column_size);

	tdsdump_log(TDS_DBG_INFO1, "adjust_character_column_size:\n"
				   "\tServer charset: %s\n"
				   "\tServer column_size: %d\n"
				   "\tClient charset: %s\n"
				   "\tClient column_size: %d\n", 
				   curcol->char_conv->to.charset.name, 
				   curcol->on_server.column_size, 
				   curcol->char_conv->from.charset.name, 
				   curcol->column_size);
}

/** 
 * Allow for maximum possible size of converted data, 
 * while being careful about integer division truncation. 
 * All character data pass through iconv.  It doesn't matter if the server side 
 * is Unicode or not; even Latin1 text need conversion if,
 * for example, the client is UTF-8.  
 * \param char_conv conversion structure
 * \param size unconverted byte size
 * \return maximum size for converted string
 */
int
determine_adjusted_size(const TDSICONV * char_conv, int size)
{
	if (!char_conv)
		return size;

	/* avoid possible overflow */
	if (size >= 0x10000000)
		return 0x7fffffff;

	size *= char_conv->from.charset.max_bytes_per_char;
	if (size % char_conv->to.charset.min_bytes_per_char)
		size += char_conv->to.charset.min_bytes_per_char;
	size /= char_conv->to.charset.min_bytes_per_char;

	return size;
}

/** @} */
