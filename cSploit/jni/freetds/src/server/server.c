/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004  Brian Bruns
 * Copyright (C) 2010  Frediano Ziglio
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

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <assert.h>

#include <freetds/tds.h>
#include <freetds/server.h>
#include <freetds/string.h>

static char software_version[] = "$Id: server.c,v 1.29 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

void
tds_env_change(TDSSOCKET * tds, int type, const char *oldvalue, const char *newvalue)
{
	TDS_SMALLINT totsize;

	/* If oldvalue is NULL, treat it like "" */
	if (oldvalue == NULL)
		oldvalue = "";

	/*
	 * NOTE: I don't know why each type of environment value has a different
	 * format.  According to the TDS5 specifications, they should all use
	 * the same format.   The code for the TDS_ENV_CHARSET case *should*
	 * work for all environment values.  -- Steve Kirkendall
	 */

	switch (type) {
	case TDS_ENV_DATABASE:
	case TDS_ENV_LANG:
	case TDS_ENV_PACKSIZE:
	case TDS_ENV_CHARSET:
		tds_put_byte(tds, TDS_ENVCHANGE_TOKEN);
		/* totsize = type + newlen + newvalue + oldlen + oldvalue  */
		totsize = strlen(oldvalue) + strlen(newvalue) + 3;
		tds_put_smallint(tds, totsize);
		tds_put_byte(tds, type);
		tds_put_byte(tds, strlen(newvalue));
		/* FIXME this assume singlebyte -> ucs2 for mssql */
		tds_put_string(tds, newvalue, strlen(newvalue));
		tds_put_byte(tds, strlen(oldvalue));
		/* FIXME this assume singlebyte -> ucs2 for mssql */
		tds_put_string(tds, oldvalue, strlen(oldvalue));
		break;
	case TDS_ENV_LCID:
	case TDS_ENV_SQLCOLLATION:
#if 1
		tds_put_byte(tds, TDS_ENVCHANGE_TOKEN);
		/* totsize = type + len + oldvalue + len + newvalue */
		totsize = 3 + strlen(newvalue) + strlen(oldvalue);
		tds_put_smallint(tds, totsize);
		tds_put_byte(tds, type);
		tds_put_byte(tds, strlen(newvalue));
		tds_put_n(tds, newvalue, strlen(newvalue));
		tds_put_byte(tds, strlen(oldvalue));
		tds_put_n(tds, oldvalue, strlen(oldvalue));
		break;
#endif
	default:
		tdsdump_log(TDS_DBG_WARN, "tds_env_change() ignoring unsupported environment code #%d", type);
	}
}

void
tds_send_eed(TDSSOCKET * tds, int msgno, int msgstate, int severity, char *msgtext, char *srvname, char *procname, int line)
{
	int totsize;

	tds_put_byte(tds, TDS_EED_TOKEN);
	totsize = 7 + strlen(procname) + 5 + strlen(msgtext) + 2 + strlen(srvname) + 3;
	tds_put_smallint(tds, totsize);
	tds_put_smallint(tds, msgno);
	tds_put_smallint(tds, 0);	/* unknown */
	tds_put_byte(tds, msgstate);
	tds_put_byte(tds, severity);
	tds_put_byte(tds, strlen(procname));
	tds_put_n(tds, procname, strlen(procname));
	tds_put_byte(tds, 0);	/* unknown */
	tds_put_byte(tds, 1);	/* unknown */
	tds_put_byte(tds, 0);	/* unknown */
	tds_put_smallint(tds, strlen(msgtext) + 1);
	tds_put_n(tds, msgtext, strlen(msgtext));
	tds_put_byte(tds, severity);
	tds_put_byte(tds, strlen(srvname));
	tds_put_n(tds, srvname, strlen(srvname));
	tds_put_byte(tds, 0);	/* unknown */
	tds_put_byte(tds, 1);	/* unknown */
	tds_put_byte(tds, 0);	/* unknown */
}

void
tds_send_msg(TDSSOCKET * tds, int msgno, int msgstate, int severity,
	     const char *msgtext, const char *srvname, const char *procname, int line)
{
	int msgsz;
	size_t len;

	tds_put_byte(tds, TDS_INFO_TOKEN);
	if (!procname)
		procname = "";
	len = strlen(procname);
	msgsz = 4		/* msg no    */
		+ 1		/* msg state */
		+ 1		/* severity  */
		/* FIXME ucs2 */
		+ (IS_TDS7_PLUS(tds->conn) ? 2 : 1) * (strlen(msgtext) + 1 + strlen(srvname) + 1 + len)
		+ 1 + 2;	/* line number */
	tds_put_smallint(tds, msgsz);
	tds_put_int(tds, msgno);
	tds_put_byte(tds, msgstate);
	tds_put_byte(tds, severity);
	tds_put_smallint(tds, strlen(msgtext));
	/* FIXME ucs2 */
	tds_put_string(tds, msgtext, strlen(msgtext));
	tds_put_byte(tds, strlen(srvname));
	/* FIXME ucs2 */
	tds_put_string(tds, srvname, strlen(srvname));
	if (len) {
		tds_put_byte(tds, len);
		/* FIXME ucs2 */
		tds_put_string(tds, procname, len);
	} else {
		tds_put_byte(tds, 0);
	}
	tds_put_smallint(tds, line);
}

void
tds_send_err(TDSSOCKET * tds, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr)
{
	tds_put_byte(tds, TDS_ERROR_TOKEN);
}

void
tds_send_login_ack(TDSSOCKET * tds, const char *progname)
{
	tds_put_byte(tds, TDS_LOGINACK_TOKEN);
	tds_put_smallint(tds, 10 + (IS_TDS7_PLUS(tds->conn)? 2 : 1) * strlen(progname));	/* length of message */
	if (IS_TDS50(tds->conn)) {
		tds_put_byte(tds, 5);
		tds_put_byte(tds, 5);
		tds_put_byte(tds, 0);
	} else {
		tds_put_byte(tds, 1);
		tds_put_byte(tds, TDS_MAJOR(tds->conn));
		tds_put_byte(tds, TDS_MINOR(tds->conn));
	}
	tds_put_byte(tds, 0);	/* unknown */
	tds_put_byte(tds, 0);	/* unknown */
	tds_put_byte(tds, strlen(progname));
	/* FIXME ucs2 */
	tds_put_string(tds, progname, strlen(progname));
	/* server version, for mssql 1.0.1 */
	tds_put_byte(tds, 1);	/* unknown */
	tds_put_byte(tds, 0);	/* unknown */
	tds_put_byte(tds, 0);	/* unknown */
	tds_put_byte(tds, 1);	/* unknown */
}

void
tds_send_capabilities_token(TDSSOCKET * tds)
{
	tds_put_byte(tds, TDS_CAPABILITY_TOKEN);
	tds_put_smallint(tds, 18);
	tds_put_byte(tds, 1);
	tds_put_byte(tds, 7);
	tds_put_byte(tds, 7);
	tds_put_byte(tds, 97);
	tds_put_byte(tds, 65);
	tds_put_byte(tds, 207);
	tds_put_byte(tds, 255);
	tds_put_byte(tds, 255);
	tds_put_byte(tds, 230);
	tds_put_byte(tds, 2);
	tds_put_byte(tds, 7);
	tds_put_byte(tds, 0);
	tds_put_byte(tds, 0);
	tds_put_byte(tds, 2);
	tds_put_byte(tds, 0);
	tds_put_byte(tds, 0);
	tds_put_byte(tds, 0);
	tds_put_byte(tds, 0);
}

/**
 * Send a "done" token, marking the end of a table, stored procedure, or query.
 * \param tds		Where the token will be written to.
 * \param token		The appropriate type of "done" token for this context:
 *			TDS_DONE_TOKEN outside a stored procedure,
 *			TDS_DONEINPROC_TOKEN inside a stored procedure, or
 *			TDS_DONEPROC_TOKEN at the end of a stored procedure.
 * \param flags		Bitwise-OR of flags in the "enum tds_end" data type.
 *			TDS_DONE_FINAL for the last statement in a query,
 *			TDS_DONE_MORE_RESULTS if not the last statement,
 *			TDS_DONE_ERROR if the statement had an error,
 *			TDS_DONE_INXACT if a transaction is  pending,
 *			TDS_DONE_PROC inside a stored procedure,
 *			TDS_DONE_COUNT if a table was sent (and rows counted)
 *			TDS_DONE_CANCELLED if the query was canceled, and
 *			TDS_DONE_EVENT if the token marks an event.
 * \param numrows	Number of rows, if flags has TDS_DONE_COUNT.
 */
void
tds_send_done(TDSSOCKET * tds, int token, TDS_SMALLINT flags, TDS_INT numrows)
{
	tds_put_byte(tds, token);
	tds_put_smallint(tds, flags);
	tds_put_smallint(tds, 2); /* are these two bytes the transaction status? */
	if (IS_TDS72_PLUS(tds->conn))
		tds_put_int8(tds, numrows);
	else
		tds_put_int(tds, numrows);
}

void
tds_send_done_token(TDSSOCKET * tds, TDS_SMALLINT flags, TDS_INT numrows)
{
	tds_send_done(tds, TDS_DONE_TOKEN, flags, numrows);
}

void
tds_send_control_token(TDSSOCKET * tds, TDS_SMALLINT numcols)
{
	int i;

	tds_put_byte(tds, TDS_CONTROL_TOKEN);
	tds_put_smallint(tds, numcols);
	for (i = 0; i < numcols; i++) {
		tds_put_byte(tds, 0);
	}
}
void
tds_send_col_name(TDSSOCKET * tds, TDSRESULTINFO * resinfo)
{
	int col, hdrsize = 0;
	TDSCOLUMN *curcol;

	tds_put_byte(tds, TDS_COLNAME_TOKEN);
	for (col = 0; col < resinfo->num_cols; col++) {
		curcol = resinfo->columns[col];
		assert(strlen(curcol->column_name) == curcol->column_namelen);
		hdrsize += curcol->column_namelen + 1;
	}

	tds_put_smallint(tds, hdrsize);
	for (col = 0; col < resinfo->num_cols; col++) {
		curcol = resinfo->columns[col];
		tds_put_byte(tds, curcol->column_namelen);
		/* exclude the null */
		tds_put_n(tds, curcol->column_name, curcol->column_namelen);
	}
}
void
tds_send_col_info(TDSSOCKET * tds, TDSRESULTINFO * resinfo)
{
	int col, hdrsize = 0;
	TDSCOLUMN *curcol;

	tds_put_byte(tds, TDS_COLFMT_TOKEN);

	for (col = 0; col < resinfo->num_cols; col++) {
		curcol = resinfo->columns[col];
		hdrsize += 5;
		if (!is_fixed_type(curcol->column_type)) {
			hdrsize++;
		}
	}
	tds_put_smallint(tds, hdrsize);

	for (col = 0; col < resinfo->num_cols; col++) {
		curcol = resinfo->columns[col];
		tds_put_n(tds, "\0\0\0\0", 4);
		tds_put_byte(tds, curcol->column_type);
		if (!is_fixed_type(curcol->column_type)) {
			tds_put_byte(tds, curcol->column_size);
		}
	}
}

void
tds_send_result(TDSSOCKET * tds, TDSRESULTINFO * resinfo)
{
	TDSCOLUMN *curcol;
	int i, totlen;

	tds_put_byte(tds, TDS_RESULT_TOKEN);
	totlen = 2;
	for (i = 0; i < resinfo->num_cols; i++) {
		curcol = resinfo->columns[i];
		totlen += 8;
		assert(strlen(curcol->column_name) == curcol->column_namelen);
		totlen += curcol->column_namelen;
		curcol = resinfo->columns[i];
		if (!is_fixed_type(curcol->column_type)) {
			totlen++;
		}
	}
	tds_put_smallint(tds, totlen);
	tds_put_smallint(tds, resinfo->num_cols);
	for (i = 0; i < resinfo->num_cols; i++) {
		curcol = resinfo->columns[i];
		tds_put_byte(tds, curcol->column_namelen);
		tds_put_n(tds, curcol->column_name, curcol->column_namelen);
		tds_put_byte(tds, '0');
		tds_put_int(tds, curcol->column_usertype);
		tds_put_byte(tds, curcol->column_type);
		if (!is_fixed_type(curcol->column_type)) {
			tds_put_byte(tds, curcol->column_size);
		}
		tds_put_byte(tds, 0);
	}
}

void
tds7_send_result(TDSSOCKET * tds, TDSRESULTINFO * resinfo)
{
	int i, j;
	TDSCOLUMN *curcol;

	/* TDS7+ uses TDS7_RESULT_TOKEN to send column names and info */
	tds_put_byte(tds, TDS7_RESULT_TOKEN);

	/* send the number of columns */
	tds_put_smallint(tds, resinfo->num_cols);

	/* send info about each column */
	for (i = 0; i < resinfo->num_cols; i++) {

		/* usertype, flags, and type */
		curcol = resinfo->columns[i];
		tds_put_smallint(tds, curcol->column_usertype);
		tds_put_smallint(tds, curcol->column_flags);
		tds_put_byte(tds, curcol->column_type); /* smallint? */

		/* bytes in "size" field varies */
		if (is_blob_type(curcol->column_type)) {
			tds_put_int(tds, curcol->column_size);
		} else if (curcol->column_type>=128) { /*is_large_type*/
			tds_put_smallint(tds, curcol->column_size);
		} else {
			tds_put_tinyint(tds, curcol->column_size);
		}

		/* some types have extra info */
		if (is_numeric_type(curcol->column_type)) {
			tds_put_tinyint(tds, curcol->column_prec);
			tds_put_tinyint(tds, curcol->column_scale);
		} else if (is_blob_type(curcol->column_type)) {
			size_t len = tds_dstr_len(&curcol->table_name);
			const char *name = tds_dstr_cstr(&curcol->table_name);

			tds_put_smallint(tds, 2 * len);
			for (j = 0; name[j] != '\0'; j++){
				tds_put_byte(tds, name[j]);
				tds_put_byte(tds, 0);
			}
		}

		/* finally the name, in UCS16 format */
		assert(strlen(curcol->column_name) == curcol->column_namelen);
		tds_put_byte(tds, curcol->column_namelen);
		for (j = 0; j < curcol->column_namelen; j++) {
			tds_put_byte(tds, curcol->column_name[j]);
			tds_put_byte(tds, 0);
		}
	}
}

/**
 * Send any tokens that mark the start of a table.  This automatically chooses
 * the right tokens for this client's version of the TDS protocol.  In other
 * words, it is a wrapper around tds_send_col_name(), tds_send_col_info(),
 * tds_send_result(), and tds7_send_result().
 * \param tds		The socket to which the tokens will be written.  Also,
 *			it contains the TDS protocol version number.
 * \param resinfo	Describes the table to be send, especially the number
 *			of columns and the names & data types of each column.
 */
void tds_send_table_header(TDSSOCKET * tds, TDSRESULTINFO * resinfo)
{
	switch (TDS_MAJOR(tds->conn)) {
	case 4:
		/*
		 * TDS4 uses TDS_COLNAME_TOKEN to send column names, and
		 * TDS_COLFMT_TOKEN to send column info.  The number of columns
		 * is implied by the number of column names.
		 */
		tds_send_col_name(tds, resinfo);
		tds_send_col_info(tds, resinfo);
		break;

	case 5:
		/* TDS5 uses a TDS_RESULT_TOKEN to send all column information */
		tds_send_result(tds, resinfo);
		break;

	case 7:
		/*
		 * TDS7+ uses a TDS7_RESULT_TOKEN to send all column
		 * information.
		 */
		tds7_send_result(tds, resinfo);
		break;
	}
}

void
tds_send_row(TDSSOCKET * tds, TDSRESULTINFO * resinfo)
{
	TDSCOLUMN *curcol;
	int colsize, i;

	tds_put_byte(tds, TDS_ROW_TOKEN);
	for (i = 0; i < resinfo->num_cols; i++) {
		curcol = resinfo->columns[i];
		if (!is_fixed_type(curcol->column_type)) {
			/* FIX ME -- I have no way of knowing the actual length of non character variable data (eg nullable int) */
			colsize = strlen((char *) curcol->column_data);
			tds_put_byte(tds, colsize);
			tds_put_n(tds, curcol->column_data, colsize);
		} else {
			tds_put_n(tds, curcol->column_data, tds_get_size_by_type(curcol->column_type));
		}
	}
}

void
tds71_send_prelogin(TDSSOCKET * tds)
{
	static const unsigned char prelogin[] = {
		0x00, 0x00, 0x15, 0x00, 0x06, 0x01, 0x00, 0x1b,
		0x00, 0x01, 0x02, 0x00, 0x1c, 0x00, 0x01, 0x03,
		0x00, 0x1d, 0x00, 0x00, 0xff, 0x08, 0x00, 0x01,
		0x55, 0x00, 0x00, 0x02, 0x00 
	};

	tds_put_n(tds, prelogin, sizeof(prelogin));
}

