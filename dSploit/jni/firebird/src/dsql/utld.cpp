/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		utld.cpp
 *	DESCRIPTION:	Utility routines for DSQL
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
 *
 * 21 Nov 01 - Ann Harrison - Turn off the code in parse_sqlda that
 *    decides that two statements are the same based on their message
 *    descriptions because it misleads some code in remote/interface.c
 *    and causes problems when two statements are prepared.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */


#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../dsql/dsql.h"
#include "../dsql/sqlda.h"
#include "../jrd/ibase.h"
#include "../jrd/align.h"
#include "../jrd/constants.h"
#include "../dsql/utld_proto.h"
#include "../jrd/gds_proto.h"
#if !defined(SUPERCLIENT)
#include "../dsql/metd_proto.h"
#endif
#include "../common/classes/init.h"

using namespace Jrd;

static void cleanup(void *);
static ISC_STATUS error_dsql_804(ISC_STATUS *, ISC_STATUS);
static SLONG get_numeric_info(const SCHAR**);
static SSHORT get_string_info(const SCHAR**, SCHAR*, int);
#ifdef NOT_USED_OR_REPLACED
#ifdef DEV_BUILD
static void print_xsqlda(XSQLDA *);
#endif
#endif
static void sqlvar_to_xsqlvar(const SQLVAR*, XSQLVAR*);
static void xsqlvar_to_sqlvar(const XSQLVAR*, SQLVAR*);

static inline void ch_stuff(BLOB_PTR*& p, const UCHAR value, bool& same_flag)
{
	if (*p == value)
		p++;
	else {
		*p++ = value;
		same_flag = false;
	}
}

static inline void ch_stuff_word(BLOB_PTR*& p, const USHORT value, bool& same_flag)
{
	ch_stuff(p, value & 255, same_flag);
	ch_stuff(p, value >> 8, same_flag);
}

// these statics define a round-robin data area for storing
// textual error messages returned to the user

static Firebird::GlobalPtr<Firebird::Mutex> failuresMutex;
static TEXT *DSQL_failures, *DSQL_failures_ptr;

const int  DSQL_FAILURE_SPACE = 2048;

/**
	Parse response on isc_info_sql_select or isc_info_sql_bind
	request. Return pointer to the next byte after successfully
	parsed info or NULL if error is encountered or info is truncated
**/
SCHAR* UTLD_skip_sql_info(SCHAR* info)
{
	if (*info != isc_info_sql_select &&
		*info != isc_info_sql_bind)
	{
		return 0;
	}

	info++;

	if (*info++ != isc_info_sql_describe_vars)
		return 0;

	get_numeric_info((const SCHAR**) &info); // skip message->msg_index

	// Loop over the variables being described
	while (true)
	{
		SCHAR str[256]; // must be big enough to hold metadata name
		const UCHAR item = *info++;

		switch (item)
		{
		case isc_info_end:
			return info;

		case isc_info_truncated:
			return 0;

		case isc_info_sql_select:
		case isc_info_sql_bind:
			return --info;

		case isc_info_sql_describe_end:
			break;

		case isc_info_sql_sqlda_seq:
		case isc_info_sql_type:
		case isc_info_sql_sub_type:
		case isc_info_sql_scale:
		case isc_info_sql_length:
			get_numeric_info((const SCHAR**) &info);
			break;

		case isc_info_sql_field:
		case isc_info_sql_relation:
		case isc_info_sql_owner:
		case isc_info_sql_alias:
			get_string_info((const SCHAR**) &info, str, sizeof(str));
			break;

		default:
			return 0;
		}
	}

	return 0;
}


/**

	UTLD_char_length_to_byte_length

	@brief  Return max byte length necessary for a specified character length string

	@param lengthInChars
	@param maxBytesPerChar

**/
USHORT UTLD_char_length_to_byte_length(USHORT lengthInChars, USHORT maxBytesPerChar, USHORT overhead)
{
	return MIN(((MAX_COLUMN_SIZE - overhead) / maxBytesPerChar) * maxBytesPerChar,
			   (ULONG) lengthInChars * maxBytesPerChar);
}


ISC_STATUS UTLD_copy_status(const ISC_STATUS* from, ISC_STATUS* to)
{
/**************************************
 *
 *	c o p y _ s t a t u s
 *
 **************************************
 *
 * Functional description
 *	Copy a status vector.
 *
 **************************************/
	const ISC_STATUS status = from[1];

	const ISC_STATUS* const end = from + ISC_STATUS_LENGTH;
	while (from < end)
		*to++ = *from++;

	return status;
}


/**

 	UTLD_parse_sql_info

    @brief	Fill in an SQLDA using data returned
 	by a call to isc_dsql_sql_info.


    @param status
    @param dialect
    @param info
    @param xsqlda
    @param return_index

 **/
ISC_STATUS	UTLD_parse_sql_info(ISC_STATUS* status,
								USHORT dialect,
								const SCHAR* info,
								XSQLDA* xsqlda,
								USHORT* return_index)
{
	XSQLVAR *xvar, xsqlvar;
	SQLDA* sqlda;

	if (return_index)
		*return_index = 0;

	if (!xsqlda)
		return 0;

	// The first byte of the returned buffer is assumed to be either a
	// isc_info_sql_select or isc_info_sql_bind item.  The second byte
	// is assumed to be isc_info_sql_describe_vars.

	info += 2;

	const SSHORT n = static_cast<SSHORT>(get_numeric_info(&info));
	if (dialect >= DIALECT_xsqlda)
	{
		if (xsqlda->version != SQLDA_VERSION1)
			return error_dsql_804(status, isc_dsql_sqlda_err);
		xsqlda->sqld = n;

		// If necessary, inform the application that more sqlda items are needed
		if (xsqlda->sqld > xsqlda->sqln)
			return 0;
	}
	else
	{
		sqlda = (SQLDA *) xsqlda;
		sqlda->sqld = n;

		// If necessary, inform the application that more sqlda items are needed
		if (sqlda->sqld > sqlda->sqln)
			return 0;

		xsqlda = NULL;
		xvar = &xsqlvar;
	}

	// Loop over the variables being described.

	SQLVAR* qvar = NULL;
	USHORT last_index = 0;
	USHORT index = 0;

	while (*info != isc_info_end)
	{
	   SCHAR item;
	   while ((item = *info++) != isc_info_sql_describe_end)
			switch (item)
			{
			case isc_info_sql_sqlda_seq:
				index = static_cast<USHORT>(get_numeric_info(&info));
				if (xsqlda)
					xvar = xsqlda->sqlvar + index - 1;
				else
				{
					qvar = sqlda->sqlvar + index - 1;
					memset(xvar, 0, sizeof(XSQLVAR));
				}
				break;

			case isc_info_sql_type:
				xvar->sqltype = static_cast<SSHORT>(get_numeric_info(&info));
				break;

			case isc_info_sql_sub_type:
				xvar->sqlsubtype = static_cast<SSHORT>(get_numeric_info(&info));
				break;

			case isc_info_sql_scale:
				xvar->sqlscale = static_cast<SSHORT>(get_numeric_info(&info));
				break;

			case isc_info_sql_length:
				xvar->sqllen = static_cast<SSHORT>(get_numeric_info(&info));
				break;

			case isc_info_sql_field:
				xvar->sqlname_length = get_string_info(&info, xvar->sqlname, sizeof(xvar->sqlname));
				break;

			case isc_info_sql_relation:
				xvar->relname_length = get_string_info(&info, xvar->relname, sizeof(xvar->relname));
				break;

			case isc_info_sql_owner:
				xvar->ownname_length = get_string_info(&info, xvar->ownname, sizeof(xvar->ownname));
				break;

			case isc_info_sql_alias:
				xvar->aliasname_length =
					get_string_info(&info, xvar->aliasname, sizeof(xvar->aliasname));
				break;

			case isc_info_truncated:
				if (return_index)
					*return_index = last_index;

			default:
				return error_dsql_804(status, isc_dsql_sqlda_err);
			}

		if (!xsqlda)
			xsqlvar_to_sqlvar(xvar, qvar);

		if (index > last_index)
			last_index = index;
	}

	if (*info != isc_info_end)
		return error_dsql_804(status, isc_dsql_sqlda_err);

	return FB_SUCCESS;
}


/**

 	UTLD_parse_sqlda

    @brief	This routine creates a blr message that describes
 	a SQLDA as well as moving data from the SQLDA
 	into a message buffer or from the message buffer
 	into the SQLDA.


    @param status
    @param dasup
    @param blr_length
    @param msg_type
    @param msg_length
    @param dialect
    @param xsqlda
    @param clause

 **/
ISC_STATUS	UTLD_parse_sqlda(ISC_STATUS* status,
							 sqlda_sup* const dasup,
							 USHORT* blr_length,
							 USHORT* msg_type,
							 USHORT* msg_length,
							 USHORT dialect,
							 const XSQLDA* xsqlda,
							 const USHORT clause)
{
	USHORT n;
	XSQLVAR xsqlvar;
	const XSQLVAR* xvar = &xsqlvar;
	const SQLVAR* qvar;
	const SQLDA* sqlda = NULL;

	if (!xsqlda)
		n = 0;
	else
		if (dialect >= DIALECT_xsqlda)
		{
			if (xsqlda->version != SQLDA_VERSION1)
				return error_dsql_804(status, isc_dsql_sqlda_err);
			n = xsqlda->sqld;
		}
		else
		{
			sqlda = reinterpret_cast<const SQLDA*>(xsqlda);
			n = sqlda->sqld;
			xsqlda = NULL;
		}


	sqlda_sup::dasup_clause* const pClause = &dasup->dasup_clauses[clause];

	if (!n)
	{
		// If there isn't an SQLDA, don't bother with anything else.

		if (blr_length)
			*blr_length = pClause->dasup_blr_length = 0;
		if (msg_length)
			*msg_length = 0;
		if (msg_type)
			*msg_type = 0;
		return 0;
	}

	if (msg_length)
	{
		// This is a call from execute or open, or the first call from fetch.
		// Determine the size of the blr, allocate a blr buffer, create the
		// blr, and finally allocate a message buffer.

		// The message buffer we are describing could be for a message to
		// either IB 4.0 or IB 3.3 - thus we need to describe the buffer
		// with the right set of blr.
		// As the BLR is only used to describe space allocation and alignment,
		// we can safely use blr_text for both 4.0 and 3.3.

		// Make a quick pass through the SQLDA to determine the amount of
		// blr that will be generated.

		USHORT blr_len = 8;
		USHORT par_count = 0;
		if (xsqlda)
			xvar = xsqlda->sqlvar - 1;
		else
			qvar = sqlda->sqlvar - 1;

		for (USHORT i = 0; i < n; i++)
		{
			if (xsqlda)
				++xvar;
			else
				sqlvar_to_xsqlvar(++qvar, &xsqlvar);

			const USHORT dtype = xvar->sqltype & ~1;
			switch (dtype)
			{
			case SQL_VARYING:
			case SQL_TEXT:
			case SQL_NULL:
				blr_len += 3;
				break;
			case SQL_SHORT:
			case SQL_LONG:
			case SQL_INT64:
			case SQL_QUAD:
			case SQL_BLOB:
			case SQL_ARRAY:
				blr_len += 2;
				break;
			default:
				++blr_len;
			}

			blr_len += 2;
			par_count += 2;
		}

		// Make sure the blr buffer is large enough.  If it isn't, allocate
		// a new one.

		if (blr_len > pClause->dasup_blr_buf_len)
		{
			if (pClause->dasup_blr) {
				gds__free(pClause->dasup_blr);
			}
			pClause->dasup_blr = static_cast<char*>(gds__alloc((SLONG) blr_len));
			// FREE: unknown
			if (!pClause->dasup_blr)	// NOMEM:
				return error_dsql_804(status, isc_virmemexh);
			pClause->dasup_blr_buf_len = blr_len;
			pClause->dasup_blr_length = 0;
		}
		memset(pClause->dasup_blr, 0, blr_len);

		bool same_flag = (blr_len == pClause->dasup_blr_length);

		// turn off same_flag because it breaks execute & execute2 when
		// more than one statement is prepared

		same_flag = false;

		pClause->dasup_blr_length = blr_len;

		// Generate the blr for the message and at the same time, determine
		// the size of the message buffer.  Allow for a null indicator with
		// each variable in the SQLDA.

		// one huge pointer per line for LIBS
		BLOB_PTR* p = reinterpret_cast<UCHAR*>(pClause->dasup_blr);

		// The define SQL_DIALECT_V5 is not available here, Hence using
		// constant 1.

		if (dialect > 1) {
			ch_stuff(p, blr_version5, same_flag);
		}
		else {
			ch_stuff(p, blr_version4, same_flag);
		}
		//else if ((SCHAR) *(p) == (SCHAR) (blr_version4)) {
		//	(p)++;
		//}
		//else {
		//	*(p)++ = (blr_version4);
		//	same_flag = false;
		//}

		ch_stuff(p, blr_begin, same_flag);
		ch_stuff(p, blr_message, same_flag);
		ch_stuff(p, 0, same_flag);
		ch_stuff_word(p, par_count, same_flag);
		USHORT msg_len = 0;
		if (xsqlda)
			xvar = xsqlda->sqlvar - 1;
		else
			qvar = sqlda->sqlvar - 1;
		for (USHORT i = 0; i < n; i++)
		{
			if (xsqlda)
				++xvar;
			else
				sqlvar_to_xsqlvar(++qvar, &xsqlvar);

			USHORT dtype = xvar->sqltype & ~1;
			USHORT len = xvar->sqllen;
			switch (dtype)
			{
			case SQL_VARYING:
				ch_stuff(p, blr_varying, same_flag);
				ch_stuff_word(p, len, same_flag);
				dtype = dtype_varying;
				len += sizeof(USHORT);
				break;
			case SQL_TEXT:
				ch_stuff(p, blr_text, same_flag);
				ch_stuff_word(p, len, same_flag);
				dtype = dtype_text;
				break;
			case SQL_DOUBLE:
				ch_stuff(p, blr_double, same_flag);
				dtype = dtype_double;
				break;
			case SQL_FLOAT:
				ch_stuff(p, blr_float, same_flag);
				dtype = dtype_real;
				break;
			case SQL_D_FLOAT:
				ch_stuff(p, blr_d_float, same_flag);
				dtype = dtype_d_float;
				break;
			case SQL_TYPE_DATE:
				ch_stuff(p, blr_sql_date, same_flag);
				dtype = dtype_sql_date;
				break;
			case SQL_TYPE_TIME:
				ch_stuff(p, blr_sql_time, same_flag);
				dtype = dtype_sql_time;
				break;
			case SQL_TIMESTAMP:
				ch_stuff(p, blr_timestamp, same_flag);
				dtype = dtype_timestamp;
				break;
			case SQL_BLOB:
				ch_stuff(p, blr_quad, same_flag);
				ch_stuff(p, 0, same_flag);
				dtype = dtype_blob;
				break;
			case SQL_ARRAY:
				ch_stuff(p, blr_quad, same_flag);
				ch_stuff(p, 0, same_flag);
				dtype = dtype_array;
				break;
			case SQL_LONG:
				ch_stuff(p, blr_long, same_flag);
				ch_stuff(p, xvar->sqlscale, same_flag);
				dtype = dtype_long;
				break;
			case SQL_SHORT:
				ch_stuff(p, blr_short, same_flag);
				ch_stuff(p, xvar->sqlscale, same_flag);
				dtype = dtype_short;
				break;
			case SQL_INT64:
				ch_stuff(p, blr_int64, same_flag);
				ch_stuff(p, xvar->sqlscale, same_flag);
				dtype = dtype_int64;
				break;
			case SQL_QUAD:
				ch_stuff(p, blr_quad, same_flag);
				ch_stuff(p, xvar->sqlscale, same_flag);
				dtype = dtype_quad;
				break;
			case SQL_NULL:
				ch_stuff(p, blr_text, same_flag);
				ch_stuff_word(p, len, same_flag);
				dtype = dtype_text;
				break;
			default:
				return error_dsql_804(status, isc_dsql_sqlda_value_err);
			}

			ch_stuff(p, blr_short, same_flag);
			ch_stuff(p, 0, same_flag);

			USHORT align = type_alignments[dtype];
			if (align)
				msg_len = FB_ALIGN(msg_len, align);
			msg_len += len;
			align = type_alignments[dtype_short];
			if (align)
				msg_len = FB_ALIGN(msg_len, align);
			msg_len += sizeof(SSHORT);
		}

		ch_stuff(p, blr_end, same_flag);
		ch_stuff(p, blr_eoc, same_flag);

		// Make sure the message buffer is large enough.  If it isn't, allocate
		// a new one.

		if (msg_len > pClause->dasup_msg_buf_len)
		{
			if (pClause->dasup_msg)
				gds__free(pClause->dasup_msg);
			pClause->dasup_msg = static_cast<char*>(gds__alloc((SLONG) msg_len));
			// FREE: unknown
			if (!pClause->dasup_msg)	// NOMEM:
				return error_dsql_804(status, isc_virmemexh);
			pClause->dasup_msg_buf_len = msg_len;
		}
		memset(pClause->dasup_msg, 0, msg_len);

		// Fill in the return values to the caller.

		*blr_length = same_flag ? 0 : blr_len;
		*msg_length = msg_len;
		*msg_type = 0;

		// If this is the first call from fetch, we're done.

		if (clause == DASUP_CLAUSE_select)
			return 0;
	}

	// Move the data between the SQLDA and the message buffer.

	USHORT offset = 0;
	// one huge pointer per line for LIBS
	BLOB_PTR* msg_buf = reinterpret_cast<UCHAR*>(pClause->dasup_msg);
	if (xsqlda)
		xvar = xsqlda->sqlvar - 1;
	else
		qvar = sqlda->sqlvar - 1;
	for (USHORT i = 0; i < n; i++)
	{
		if (xsqlda)
			++xvar;
		else
			sqlvar_to_xsqlvar(++qvar, &xsqlvar);

		USHORT dtype = xvar->sqltype & ~1;
		USHORT len = xvar->sqllen;
		switch (dtype)
		{
		case SQL_VARYING:
			dtype = dtype_varying;
			len += sizeof(USHORT);
			break;
		case SQL_TEXT:
			dtype = dtype_text;
			break;
		case SQL_DOUBLE:
			dtype = dtype_double;
			break;
		case SQL_FLOAT:
			dtype = dtype_real;
			break;
		case SQL_D_FLOAT:
			dtype = dtype_d_float;
			break;
		case SQL_TYPE_DATE:
			dtype = dtype_sql_date;
			break;
		case SQL_TYPE_TIME:
			dtype = dtype_sql_time;
			break;
		case SQL_TIMESTAMP:
			dtype = dtype_timestamp;
			break;
		case SQL_BLOB:
			dtype = dtype_blob;
			break;
		case SQL_ARRAY:
			dtype = dtype_array;
			break;
		case SQL_LONG:
			dtype = dtype_long;
			break;
		case SQL_SHORT:
			dtype = dtype_short;
			break;
		case SQL_INT64:
			dtype = dtype_int64;
			break;
		case SQL_QUAD:
			dtype = dtype_quad;
			break;
		case SQL_NULL:
			dtype = dtype_text;
			break;
		}

		USHORT align = type_alignments[dtype];
		if (align)
			offset = FB_ALIGN(offset, align);
		USHORT null_offset = offset + len;

		align = type_alignments[dtype_short];
		if (align)
			null_offset = FB_ALIGN(null_offset, align);

		SSHORT *null_ind = (SSHORT *) (msg_buf + null_offset);
		if (clause == DASUP_CLAUSE_select)
		{
			// Move data from the message into the SQLDA.

			if ((xvar->sqltype & ~1) != SQL_NULL)
			{
				// Make sure user has specified a data location
				if (!xvar->sqldata)
					return error_dsql_804(status, isc_dsql_sqlda_value_err);

				memcpy(xvar->sqldata, msg_buf + offset, len);
			}

			if (xvar->sqltype & 1)
			{
				// Make sure user has specified a location for null indicator
				if (!xvar->sqlind)
					return error_dsql_804(status, isc_dsql_sqlda_value_err);
				*xvar->sqlind = *null_ind;
			}
		}
		else
		{
			// Move data from the SQLDA into the message.  If the
			// indicator variable identifies a null value, permit
			// the data value to be missing.

			if (xvar->sqltype & 1)
			{
				// Make sure user has specified a location for null indicator
				if (!xvar->sqlind)
					return error_dsql_804(status, isc_dsql_sqlda_value_err);
				*null_ind = *xvar->sqlind;
			}
			else
				*null_ind = 0;

			// Make sure user has specified a data location (unless NULL)
			if (!xvar->sqldata && !*null_ind && (xvar->sqltype & ~1) != SQL_NULL)
				return error_dsql_804(status, isc_dsql_sqlda_value_err);

			// Copy data - unless known to be NULL
			if ((offset + len) > pClause->dasup_msg_buf_len)
				return error_dsql_804(status, isc_dsql_sqlda_value_err);

			if (!*null_ind)
				memcpy(msg_buf + offset, xvar->sqldata, len);
		}

		offset = null_offset + sizeof(SSHORT);
	}

	return 0;
}


/**

 	UTLD_save_status_strings

    @brief	Strings in status vectors may be stored in stack variables
 	or memory pools that are transient.  To perserve the information,
 	copy any included strings to a special buffer.


    @param vector

 **/
void	UTLD_save_status_strings(ISC_STATUS* vector)
{
	Firebird::MutexLockGuard guard(failuresMutex);

	// allocate space for failure strings if it hasn't already been allocated

	if (!DSQL_failures)
	{
		DSQL_failures = (TEXT*) gds__alloc((SLONG) DSQL_FAILURE_SPACE);
		// FREE: by exit handler cleanup()
		if (!DSQL_failures)		// NOMEM: don't try to copy the strings
			return;
		DSQL_failures_ptr = DSQL_failures;
		gds__register_cleanup(cleanup, 0);

#ifdef DEBUG_GDS_ALLOC
		gds_alloc_flag_unfreed((void*) DSQL_failures);
#endif
	}

	while (*vector)
	{
		const TEXT* p;
		USHORT l;
		const ISC_STATUS status = *vector++;
		switch (status)
		{
		case isc_arg_cstring:
			l = static_cast<USHORT>(*vector++);

		case isc_arg_string:
		case isc_arg_interpreted:
		case isc_arg_sql_state:
			p = (TEXT*) *vector;

			if (status != isc_arg_cstring)
				l = strlen(p) + 1;

			// If there isn't any more room in the buffer,
			// start at the beginning again

			if (DSQL_failures_ptr + l > DSQL_failures + DSQL_FAILURE_SPACE)
				DSQL_failures_ptr = DSQL_failures;

			*vector++ = (ISC_STATUS) DSQL_failures_ptr;

			if (l)
			{
				do
				{
					*DSQL_failures_ptr++ = *p++;
				} while (--l && (DSQL_failures_ptr < DSQL_failures + DSQL_FAILURE_SPACE));
			}

			if (l)
				*(DSQL_failures_ptr - 1) = '\0';

			break;

		default:
			++vector;
			break;
		}
	}
}


/**

 	cleanup

    @brief	Exit handler to cleanup dynamically allocated memory.


    @param arg

 **/
static void cleanup(void*)
{
	Firebird::MutexLockGuard guard(failuresMutex);

	if (DSQL_failures)
		gds__free(DSQL_failures);

	gds__unregister_cleanup(cleanup, 0);
	DSQL_failures = NULL;
}


/**

 	error_dsql_804

    @brief	Move a DSQL -804 error message into a status vector.


    @param status
    @param err

 **/
static ISC_STATUS error_dsql_804( ISC_STATUS * status, ISC_STATUS err)
{
	ISC_STATUS *p = status;

	*p++ = isc_arg_gds;
	*p++ = isc_dsql_error;
	*p++ = isc_arg_gds;
	*p++ = isc_sqlerr;
	*p++ = isc_arg_number;
	*p++ = -804;
	*p++ = isc_arg_gds;
	*p++ = err;
	*p = isc_arg_end;

	return status[1];
}


/**

 	get_numeric_info

    @brief	Pick up a VAX format numeric info item
 	with a 2 byte length.


    @param ptr

 **/
static SLONG get_numeric_info(const SCHAR** ptr)
{
	const SSHORT l = static_cast<SSHORT>(gds__vax_integer(reinterpret_cast<const UCHAR*>(*ptr), 2));
	*ptr += 2;
	int item = gds__vax_integer(reinterpret_cast<const UCHAR*>(*ptr), l);
	*ptr += l;

	return item;
}


/**

 	get_string_info

    @brief	Pick up a string valued info item and return
 	its length.  The buffer_len argument is assumed
 	to include space for the terminating null.


    @param ptr
    @param buffer
    @param buffer_len

 **/
static SSHORT get_string_info(const SCHAR** ptr, SCHAR* buffer, int buffer_len)
{
	const SCHAR* p = *ptr;
	SSHORT len = static_cast<SSHORT>(gds__vax_integer(reinterpret_cast<const UCHAR*>(p), 2));
	// CVC: What else can we do here?
	if (len < 0)
		len = 0;

	*ptr += len + 2;
	p += 2;

	if (len >= buffer_len)
		len = buffer_len - 1;

	if (len)
		memcpy(buffer, p, len);
	buffer[len] = 0;

	return len;
}

#ifdef NOT_USED_OR_REPLACED
#ifdef DEV_BUILD
static void print_xsqlda( XSQLDA * xsqlda)
{
/*****************************************
 *
 *	p r i n t _ x s q l d a
 *
 *****************************************
 *
 * print an sqlda
 *
 *****************************************/
	XSQLVAR *xvar, *end_var;

	if (!xsqlda)
		return;

	printf("SQLDA Version %d\n", xsqlda->version);
	printf("      sqldaid %.8s\n", xsqlda->sqldaid);
	printf("      sqldabc %d\n", xsqlda->sqldabc);
	printf("      sqln    %d\n", xsqlda->sqln);
	printf("      sqld    %d\n", xsqlda->sqld);

	xvar = xsqlda->sqlvar;
	for (end_var = xvar + xsqlda->sqld; xvar < end_var; xvar++)
		printf("         %.31s %.31s type: %d, scale %d, len %d subtype %d\n",
			   xvar->sqlname, xvar->relname, xvar->sqltype,
			   xvar->sqlscale, xvar->sqllen, xvar->sqlsubtype);
}
#endif
#endif

/**

 	sqlvar_to_xsqlvar


    @param sqlvar
    @param xsqlvar


    @param sqlvar
    @param xsqlvar

 **/
static void sqlvar_to_xsqlvar(const SQLVAR* sqlvar, XSQLVAR* xsqlvar)
{

	xsqlvar->sqltype = sqlvar->sqltype;
	xsqlvar->sqldata = sqlvar->sqldata;
	xsqlvar->sqlind = sqlvar->sqlind;

	xsqlvar->sqlsubtype = 0;
	xsqlvar->sqlscale = 0;
	xsqlvar->sqllen = sqlvar->sqllen;
	switch (xsqlvar->sqltype & ~1)
	{
	case SQL_LONG:
		xsqlvar->sqlscale = xsqlvar->sqllen >> 8;
		xsqlvar->sqllen = sizeof(SLONG);
		break;
	case SQL_SHORT:
		xsqlvar->sqlscale = xsqlvar->sqllen >> 8;
		xsqlvar->sqllen = sizeof(SSHORT);
		break;
	case SQL_INT64:
		xsqlvar->sqlscale = xsqlvar->sqllen >> 8;
		xsqlvar->sqllen = sizeof(SINT64);
		break;
	case SQL_QUAD:
		xsqlvar->sqlscale = xsqlvar->sqllen >> 8;
		xsqlvar->sqllen = sizeof(ISC_QUAD);
		break;
	}
}


/**

 	xsqlvar_to_sqlvar

    @brief	Move an XSQLVAR to an SQLVAR.


    @param xsqlvar
    @param sqlvar

 **/
static void xsqlvar_to_sqlvar(const XSQLVAR* xsqlvar, SQLVAR* sqlvar)
{

	sqlvar->sqltype = xsqlvar->sqltype;
	sqlvar->sqlname_length = xsqlvar->aliasname_length;

	// N.B., this may not NULL-terminate the name...

	memcpy(sqlvar->sqlname, xsqlvar->aliasname, sizeof(sqlvar->sqlname));

	sqlvar->sqllen = xsqlvar->sqllen;
	const USHORT scale = xsqlvar->sqlscale << 8;
	switch (sqlvar->sqltype & ~1)
	{
	case SQL_LONG:
		sqlvar->sqllen = sizeof(SLONG) | scale;
		break;
	case SQL_SHORT:
		sqlvar->sqllen = sizeof(SSHORT) | scale;
		break;
	case SQL_INT64:
		sqlvar->sqllen = sizeof(SINT64) | scale;
		break;
	case SQL_QUAD:
		sqlvar->sqllen = sizeof(ISC_QUAD) | scale;
		break;
	}
}


#if !defined(SUPERCLIENT)

UCHAR DSqlDataTypeUtil::maxBytesPerChar(UCHAR charSet)
{
	return METD_get_charset_bpc(statement, charSet);
}

USHORT DSqlDataTypeUtil::getDialect() const
{
	return statement->req_client_dialect;
}

#endif
