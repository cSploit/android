/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001  Brian Bruns
 * Copyright (C) 2002-2012  Frediano Ziglio
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

/*
 * PROGRAMMER   NAME            CONTACT
 *==============================================================
 * BSB          Brian Bruns     camber@ais.org
 * PAH          Peter Harvey    pharvey@codebydesign.com
 * SMURPH       Steve Murphree  smurph@smcomp.com
 *
 ***************************************************************
 * DATE         PROGRAMMER  CHANGE
 *==============================================================
 *                          Original.
 * 03.FEB.02    PAH         Started adding use of SQLGetPrivateProfileString().
 * 04.FEB.02	PAH         Fixed small error preventing SQLBindParameter from being called
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
#include <ctype.h>

#include <freetds/odbc.h>
#include <freetds/iconv.h>
#include <freetds/string.h>
#include <freetds/convert.h>
#include "replacements.h"
#include "sqlwparams.h"
#include <odbcss.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: odbc.c,v 1.593 2012-03-11 15:52:22 freddy77 Exp $");

static SQLRETURN _SQLAllocConnect(SQLHENV henv, SQLHDBC FAR * phdbc);
static SQLRETURN _SQLAllocEnv(SQLHENV FAR * phenv, SQLINTEGER odbc_version);
static SQLRETURN _SQLAllocStmt(SQLHDBC hdbc, SQLHSTMT FAR * phstmt);
static SQLRETURN _SQLAllocDesc(SQLHDBC hdbc, SQLHDESC FAR * phdesc);
static SQLRETURN _SQLFreeConnect(SQLHDBC hdbc);
static SQLRETURN _SQLFreeEnv(SQLHENV henv);
static SQLRETURN _SQLFreeStmt(SQLHSTMT hstmt, SQLUSMALLINT fOption, int force);
static SQLRETURN _SQLFreeDesc(SQLHDESC hdesc);
static SQLRETURN _SQLExecute(TDS_STMT * stmt);
static SQLRETURN _SQLSetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength WIDE);
static SQLRETURN _SQLGetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength,
					 SQLINTEGER * StringLength WIDE);
static SQLRETURN _SQLColAttribute(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType, SQLPOINTER rgbDesc,
					  SQLSMALLINT cbDescMax, SQLSMALLINT FAR * pcbDesc, SQLLEN FAR * pfDesc _WIDE);
static SQLRETURN _SQLFetch(TDS_STMT * stmt, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset);
static SQLRETURN odbc_populate_ird(TDS_STMT * stmt);
static int odbc_errmsg_handler(const TDSCONTEXT * ctx, TDSSOCKET * tds, TDSMESSAGE * msg);
static void odbc_log_unimplemented_type(const char function_name[], int fType);
static void odbc_upper_column_names(TDS_STMT * stmt);
static void odbc_col_setname(TDS_STMT * stmt, int colpos, const char *name);
static SQLRETURN odbc_stat_execute(TDS_STMT * stmt _WIDE, const char *begin, int nparams, ...);
static SQLRETURN odbc_free_dynamic(TDS_STMT * stmt);
static SQLRETURN odbc_free_cursor(TDS_STMT * stmt);
static SQLRETURN odbc_update_ird(TDS_STMT *stmt, TDS_ERRS *errs);
static SQLRETURN odbc_prepare(TDS_STMT *stmt);
static SQLSMALLINT odbc_swap_datetime_sql_type(SQLSMALLINT sql_type);
static int odbc_process_tokens(TDS_STMT * stmt, unsigned flag);
static int odbc_lock_statement(TDS_STMT* stmt);
static void odbc_unlock_statement(TDS_STMT* stmt);

#if ENABLE_EXTRA_CHECKS
static void odbc_ird_check(TDS_STMT * stmt);

#define IRD_CHECK odbc_ird_check(stmt)
#else
#define IRD_CHECK
#endif

/**
 * \defgroup odbc_api ODBC API
 * Functions callable by \c ODBC client programs
 */


/* utils to check handles */
#define INIT_HANDLE(t, n) \
	TDS_##t *n = (TDS_##t*)h##n; \
	if (SQL_NULL_H##t  == h##n || !IS_H##t(h##n)) return SQL_INVALID_HANDLE; \
	tds_mutex_lock(&n->mtx); \
	CHECK_##t##_EXTRA(n); \
	odbc_errs_reset(&n->errs);

#define ODBC_ENTER_HSTMT INIT_HANDLE(STMT, stmt)
#define ODBC_ENTER_HDBC  INIT_HANDLE(DBC,  dbc)
#define ODBC_ENTER_HENV  INIT_HANDLE(ENV,  env)
#define ODBC_ENTER_HDESC INIT_HANDLE(DESC, desc)

#define IS_VALID_LEN(len) ((len) >= 0 || (len) == SQL_NTS || (len) == SQL_NULL_DATA)

#define ODBC_SAFE_ERROR(stmt) \
	do { \
		if (!stmt->errs.num_errors) \
			odbc_errs_add(&stmt->errs, "HY000", "Unknown error"); \
	} while(0)

#define DEFAULT_QUERY_TIMEOUT (~((SQLUINTEGER) 0))

/*
 * Note: I *HATE* hungarian notation, it has to be the most idiotic thing
 * I've ever seen. So, you will note it is avoided other than in the function
 * declarations. "Gee, let's make our code totally hard to read and they'll
 * beg for GUI tools"
 * Bah!
 */

static const char *
odbc_prret(SQLRETURN ret, char *unknown, size_t unknown_size)
{
	switch (ret) {
	case SQL_ERROR:			return "SQL_ERROR";
	case SQL_INVALID_HANDLE:	return "SQL_INVALID_HANDLE";
	case SQL_SUCCESS:		return "SQL_SUCCESS";
	case SQL_SUCCESS_WITH_INFO:	return "SQL_SUCCESS_WITH_INFO";
#if ODBCVER >= 0x0300
	case SQL_NO_DATA:		return "SQL_NO_DATA";
#endif
	case SQL_STILL_EXECUTING:	return "SQL_STILL_EXECUTING";
	case SQL_NEED_DATA:		return "SQL_NEED_DATA";
	}
	
	snprintf(unknown, unknown_size, "unknown: %d", (int)ret);

	return unknown;
}
#define ODBC_PRRET_BUF	char unknown_prret_buf[24]
#define odbc_prret(ret) odbc_prret(ret, unknown_prret_buf, sizeof(unknown_prret_buf))

static void
odbc_col_setname(TDS_STMT * stmt, int colpos, const char *name)
{
#if ENABLE_EXTRA_CHECKS
	TDSRESULTINFO *resinfo;
#endif

	IRD_CHECK;

#if ENABLE_EXTRA_CHECKS
	if (colpos > 0 && stmt->tds != NULL && (resinfo = stmt->tds->current_results) != NULL) {
		if (colpos <= resinfo->num_cols) {
			/* no overflow possible, name is always shorter */
			strcpy(resinfo->columns[colpos - 1]->column_name, name);
			resinfo->columns[colpos - 1]->column_namelen = strlen(name);
			tds_dstr_copy(&resinfo->columns[colpos - 1]->table_column_name, "");
		}
	}
#endif

	if (colpos > 0 && colpos <= stmt->ird->header.sql_desc_count) {
		--colpos;
		tds_dstr_copy(&stmt->ird->records[colpos].sql_desc_label, name);
		tds_dstr_copy(&stmt->ird->records[colpos].sql_desc_name, name);
	}
}

/* spinellia@acm.org : copied shamelessly from change_database */
static SQLRETURN
change_autocommit(TDS_DBC * dbc, int state)
{
	TDSSOCKET *tds = dbc->tds_socket;
	TDSRET ret;

	if (dbc->attr.autocommit == state)
		return SQL_SUCCESS;

	/*
	 * We may not be connected yet and dbc->tds_socket
	 * may not initialized.
	 */
	if (tds) {
		/* TODO better idle check, not thread safe */
		if (tds->state == TDS_IDLE)
			tds->query_timeout = dbc->default_query_timeout;

		if (state == SQL_AUTOCOMMIT_ON)
			ret = tds_submit_rollback(tds, 0);
		else
			ret = tds_submit_begin_tran(tds);

		if (TDS_FAILED(ret)) {
			odbc_errs_add(&dbc->errs, "HY000", "Could not change transaction status");
			return SQL_ERROR;
		}
		if (TDS_FAILED(tds_process_simple_query(tds))) {
			odbc_errs_add(&dbc->errs, "HY000", "Could not change transaction status");
			return SQL_ERROR;
		}
		dbc->attr.autocommit = state;
	} else {
		/* if not connected we will change auto-commit after login */
		dbc->attr.autocommit = state;
	}
	ODBC_RETURN_(dbc);
}

static SQLRETURN
change_database(TDS_DBC * dbc, const char *database, int database_len)
{
	TDSSOCKET *tds = dbc->tds_socket;

	/* 
	 * We may not be connected yet and dbc->tds_socket
	 * may not initialized.
	 */
	if (tds) {
		/* build query */
		char *query = (char *) malloc(6 + tds_quote_id(tds, NULL, database, database_len));

		if (!query) {
			odbc_errs_add(&dbc->errs, "HY001", NULL);
			return SQL_ERROR;
		}
		strcpy(query, "USE ");
		tds_quote_id(tds, query + 4, database, database_len);


		tdsdump_log(TDS_DBG_INFO1, "change_database: executing %s\n", query);

		/* TODO better idle check, not thread safe */
		if (tds->state == TDS_IDLE)
			tds->query_timeout = dbc->default_query_timeout;
		if (TDS_FAILED(tds_submit_query(tds, query))) {
			free(query);
			odbc_errs_add(&dbc->errs, "HY000", "Could not change database");
			return SQL_ERROR;
		}
		free(query);
		if (TDS_FAILED(tds_process_simple_query(tds))) {
			odbc_errs_add(&dbc->errs, "HY000", "Could not change database");
			return SQL_ERROR;
		}
	} else {
		tds_dstr_copyn(&dbc->attr.current_catalog, database, database_len);
	}
	ODBC_RETURN_(dbc);
}

static SQLRETURN
change_txn(TDS_DBC * dbc, SQLUINTEGER txn_isolation)
{
	char query[64];
	const char *level;
	TDSSOCKET *tds = dbc->tds_socket;

	switch (txn_isolation) {
	case SQL_TXN_READ_COMMITTED:
		level = "READ COMMITTED";
		break;
	case SQL_TXN_READ_UNCOMMITTED:
		level = "READ UNCOMMITTED";
		break;
	case SQL_TXN_REPEATABLE_READ:
		level = "REPEATABLE READ";
		break;
	case SQL_TXN_SERIALIZABLE:
		level = "SERIALIZABLE";
		break;
	default:
		odbc_errs_add(&dbc->errs, "HY024", NULL);
		return SQL_ERROR;
	}

	/* if not connected return success, will be set after connection */
	if (!tds)
		return SQL_SUCCESS;

	if (tds->state != TDS_IDLE) {
		odbc_errs_add(&dbc->errs, "HY011", NULL);
		return SQL_ERROR;
	}

	tds->query_timeout = dbc->default_query_timeout;
	sprintf(query, "SET TRANSACTION ISOLATION LEVEL %s", level);
	if (TDS_FAILED(tds_submit_query(tds, query))) {
		ODBC_SAFE_ERROR(dbc);
		return SQL_ERROR;
	}
	if (TDS_FAILED(tds_process_simple_query(tds))) {
		ODBC_SAFE_ERROR(dbc);
		return SQL_ERROR;
	}

	return SQL_SUCCESS;
}

static TDS_DBC*
odbc_get_dbc(TDSSOCKET *tds) {
	TDS_CHK *chk = (TDS_CHK *) tds_get_parent(tds);
	if (!chk)
		return NULL;
	if (chk->htype == SQL_HANDLE_DBC)
		return (TDS_DBC *) chk;
	assert(chk->htype == SQL_HANDLE_STMT);
	return ((TDS_STMT *) chk)->dbc;
}

static TDS_STMT*
odbc_get_stmt(TDSSOCKET *tds) {
	TDS_CHK *chk = (TDS_CHK *) tds_get_parent(tds);
	if (!chk || chk->htype != SQL_HANDLE_STMT)
		return NULL;
	return (TDS_STMT *) chk;
}


static void
odbc_env_change(TDSSOCKET * tds, int type, char *oldval, char *newval)
{
	TDS_DBC *dbc;

	if (tds == NULL) {
		return;
	}
	dbc = odbc_get_dbc(tds);
	if (!dbc)
		return;

	switch (type) {
	case TDS_ENV_DATABASE:
		tds_dstr_copy(&dbc->attr.current_catalog, newval);
		break;
	case TDS_ENV_PACKSIZE:
		dbc->attr.packet_size = atoi(newval);
		break;
	}
}

static SQLRETURN
odbc_connect(TDS_DBC * dbc, TDSLOGIN * login)
{
	TDS_ENV *env = dbc->env;

#ifdef ENABLE_ODBC_WIDE
	dbc->mb_conv = NULL;
#endif
	dbc->tds_socket = tds_alloc_socket(env->tds_ctx, 512);
	if (!dbc->tds_socket) {
		odbc_errs_add(&dbc->errs, "HY001", NULL);
		return SQL_ERROR;
	}
	tds_conn(dbc->tds_socket)->use_iconv = 0;
	tds_set_parent(dbc->tds_socket, (void *) dbc);

	/* Set up our environment change hook */
	dbc->tds_socket->env_chg_func = odbc_env_change;

	tds_fix_login(login);

	login->connect_timeout = dbc->attr.connection_timeout;
	if (dbc->attr.mars_enabled != SQL_MARS_ENABLED_NO)
		login->mars = 1;

#ifdef ENABLE_ODBC_WIDE
	/* force utf-8 in order to support wide characters */
	tds_dstr_dup(&dbc->original_charset, &login->client_charset);
	tds_dstr_copy(&login->client_charset, "UTF-8");
#endif

	if (TDS_FAILED(tds_connect_and_login(dbc->tds_socket, login))) {
		tds_free_socket(dbc->tds_socket);
		dbc->tds_socket = NULL;
		odbc_errs_add(&dbc->errs, "08001", NULL);
		return SQL_ERROR;
	}
#ifdef ENABLE_ODBC_WIDE
	dbc->mb_conv = tds_iconv_get(dbc->tds_socket->conn, tds_dstr_cstr(&dbc->original_charset), "UTF-8");
#endif

	dbc->default_query_timeout = dbc->tds_socket->query_timeout;

	if (IS_TDS7_PLUS(dbc->tds_socket->conn))
		dbc->cursor_support = 1;

#if ENABLE_ODBC_MARS
	/* check if mars is enabled */
	if (!IS_TDS72_PLUS(dbc->tds_socket->conn) || !dbc->tds_socket->conn->mars)
		dbc->attr.mars_enabled = SQL_MARS_ENABLED_NO;
#else
	dbc->attr.mars_enabled = SQL_MARS_ENABLED_NO;
#endif

	if (dbc->attr.txn_isolation != SQL_TXN_READ_COMMITTED) {
		if (!SQL_SUCCEEDED(change_txn(dbc, dbc->attr.txn_isolation)))
			ODBC_RETURN_(dbc);
	}

	if (dbc->attr.autocommit != SQL_AUTOCOMMIT_ON) {
		dbc->attr.autocommit = SQL_AUTOCOMMIT_ON;
		if (!SQL_SUCCEEDED(change_autocommit(dbc, SQL_AUTOCOMMIT_OFF)))
			ODBC_RETURN_(dbc);
	}

	/* this overwrite any error arrived (wanted behavior, Sybase return error for conversion errors) */
	ODBC_RETURN(dbc, SQL_SUCCESS);
}

static SQLRETURN
odbc_update_ird(TDS_STMT *stmt, TDS_ERRS *errs)
{
	SQLRETURN res;

	if (!stmt->need_reprepare || stmt->prepared_query_is_rpc
	    || !stmt->dbc || !IS_TDS7_PLUS(stmt->dbc->tds_socket->conn)) {
		stmt->need_reprepare = 0;
		return SQL_SUCCESS;
	}

	/* FIXME where error are put ?? on stmt... */
	if (!odbc_lock_statement(stmt))
		ODBC_RETURN_(stmt);

	/* FIXME error */
	res = start_parse_prepared_query(stmt, 0);
	if (res != SQL_SUCCESS) {
		/* prepare with dummy parameters just to fill IRD */
		tds_free_param_results(stmt->params);
		stmt->params = NULL;
		stmt->param_num = 0;
		/*
		 * TODO
		 * we need to prepare again with parameters but need_reprepare
		 * flag is reset by odbc_prepare... perhaps should be checked
		 * later, not calling describeCol or similar
		 * we need prepare to get dynamic and cause we change parameters
		 */
	}

	return odbc_prepare(stmt);
}

static SQLRETURN
odbc_prepare(TDS_STMT *stmt)
{
	TDSSOCKET *tds = stmt->tds;
	int in_row = 0;

	if (TDS_FAILED(tds_submit_prepare(tds, stmt->prepared_query, NULL, &stmt->dyn, stmt->params))) {
		ODBC_SAFE_ERROR(stmt);
		return SQL_ERROR;
	}

	/* try to go to the next recordset */
	/* TODO merge with similar code */
	desc_free_records(stmt->ird);
	stmt->row_status = PRE_NORMAL_ROW;
	for (;;) {
		TDS_INT result_type;
		int done_flags;

		switch (tds_process_tokens(tds, &result_type, &done_flags, TDS_RETURN_ROWFMT|TDS_RETURN_DONE)) {
		case TDS_SUCCESS:
			switch (result_type) {
			case TDS_DONE_RESULT:
			case TDS_DONEPROC_RESULT:
			case TDS_DONEINPROC_RESULT:
				stmt->row_count = tds->rows_affected;
				if (done_flags & TDS_DONE_ERROR && !stmt->dyn->emulated)
					stmt->errs.lastrc = SQL_ERROR;
				/* FIXME this row is used only as a flag for update binding, should be cleared if binding/result changed */
				stmt->row = 0;
				break;

			case TDS_ROWFMT_RESULT:
				/* store first row informations */
				if (!in_row)
					odbc_populate_ird(stmt);
				stmt->row = 0;
				stmt->row_count = TDS_NO_COUNT;
				stmt->row_status = PRE_NORMAL_ROW;
				in_row = 1;
				break;
			}
			continue;
		case TDS_NO_MORE_RESULTS:
			break;
		case TDS_CANCELLED:
			odbc_errs_add(&stmt->errs, "HY008", NULL);
		default:
			stmt->errs.lastrc = SQL_ERROR;
			break;
		}
		break;
	}

	if (stmt->errs.lastrc == SQL_ERROR && !stmt->dyn->emulated) {
		tds_release_dynamic(&stmt->dyn);
	}
	odbc_unlock_statement(stmt);
	stmt->need_reprepare = 0;
	ODBC_RETURN_(stmt);
}

ODBC_FUNC(SQLDriverConnect, (P(SQLHDBC,hdbc), P(SQLHWND,hwnd), PCHARIN(ConnStrIn,SQLSMALLINT),
	PCHAROUT(ConnStrOut,SQLSMALLINT), P(SQLUSMALLINT,fDriverCompletion) WIDE))
{
	TDSLOGIN *login;
	TDS_PARSED_PARAM params[ODBC_PARAM_SIZE];
	DSTR conn_str;

	ODBC_ENTER_HDBC;

#ifdef TDS_NO_DM
	/* Check string length */
	if (!IS_VALID_LEN(cbConnStrIn) || cbConnStrIn == 0) {
		odbc_errs_add(&dbc->errs, "HY090", NULL);
		ODBC_EXIT_(dbc);
	}

	/* Check completion param */
	switch (fDriverCompletion) {
	case SQL_DRIVER_NOPROMPT:
	case SQL_DRIVER_COMPLETE:
	case SQL_DRIVER_PROMPT:
	case SQL_DRIVER_COMPLETE_REQUIRED:
		break;
	default:
		odbc_errs_add(&dbc->errs, "HY110", NULL);
		ODBC_EXIT_(dbc);
	}
#endif

	tds_dstr_init(&conn_str);
	if (!odbc_dstr_copy(dbc, &conn_str, cbConnStrIn, szConnStrIn)) {
		odbc_errs_add(&dbc->errs, "HY001", NULL);
		ODBC_EXIT_(dbc);
	}

	login = tds_alloc_login(0);
	if (!login || !tds_init_login(login, dbc->env->tds_ctx->locale)) {
		tds_free_login(login);
		tds_dstr_free(&conn_str);
		odbc_errs_add(&dbc->errs, "HY001", NULL);
		ODBC_EXIT_(dbc);
	}

	if (!tds_dstr_isempty(&dbc->attr.current_catalog))
		tds_dstr_dup(&login->database, &dbc->attr.current_catalog);

	/* parse the DSN string */
	if (!odbc_parse_connect_string(&dbc->errs, tds_dstr_buf(&conn_str), tds_dstr_buf(&conn_str) + tds_dstr_len(&conn_str),
				       login, params)) {
		tds_dstr_free(&conn_str);
		ODBC_EXIT_(dbc);
	}

	odbc_set_string(dbc, szConnStrOut, cbConnStrOutMax, pcbConnStrOut, tds_dstr_buf(&conn_str), tds_dstr_len(&conn_str));
	tds_dstr_free(&conn_str);

	/* add login info */
	if (hwnd && fDriverCompletion != SQL_DRIVER_NOPROMPT
	    && (fDriverCompletion == SQL_DRIVER_PROMPT || (!params[ODBC_PARAM_UID].p && !params[ODBC_PARAM_Trusted_Connection].p)
		|| tds_dstr_isempty(&login->server_name))) {
#ifdef _WIN32
		char *out = NULL;

		/* prompt for login information */
		if (!get_login_info(hwnd, login)) {
			tds_free_login(login);
			odbc_errs_add(&dbc->errs, "08001", "User canceled login");
			ODBC_EXIT_(dbc);
		}
		if (tds_dstr_isempty(&login->user_name)) {
			params[ODBC_PARAM_UID].p = NULL;
			params[ODBC_PARAM_PWD].p = NULL;
			params[ODBC_PARAM_Trusted_Connection].p = "Yes";
			params[ODBC_PARAM_Trusted_Connection].len = 3;
		} else {
			params[ODBC_PARAM_UID].p   = tds_dstr_cstr(&login->user_name);
			params[ODBC_PARAM_UID].len = tds_dstr_len(&login->user_name);
			params[ODBC_PARAM_PWD].p   = tds_dstr_cstr(&login->password);
			params[ODBC_PARAM_PWD].len = tds_dstr_len(&login->password);
			params[ODBC_PARAM_Trusted_Connection].p = NULL;
		}
		if (!odbc_build_connect_string(&dbc->errs, params, &out))
			ODBC_EXIT_(dbc);

		odbc_set_string(dbc, szConnStrOut, cbConnStrOutMax, pcbConnStrOut, out, -1);
		tdsdump_log(TDS_DBG_INFO1, "connection string is now: %s\n", out);
		free(out);
#else
		/* we dont support a dialog box */
		odbc_errs_add(&dbc->errs, "HYC00", NULL);
#endif
	}

	if (tds_dstr_isempty(&login->server_name)) {
		tds_free_login(login);
		odbc_errs_add(&dbc->errs, "IM007", "Could not find Servername or server parameter");
		ODBC_EXIT_(dbc);
	}

	odbc_connect(dbc, login);

	tds_free_login(login);
	ODBC_EXIT_(dbc);
}

#if 0
SQLRETURN ODBC_API
SQLBrowseConnect(SQLHDBC hdbc, SQLCHAR FAR * szConnStrIn, SQLSMALLINT cbConnStrIn, SQLCHAR FAR * szConnStrOut,
		 SQLSMALLINT cbConnStrOutMax, SQLSMALLINT FAR * pcbConnStrOut)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLBrowseConnect(%p, %s, %d, %p, %d, %p)\n", 
			hdbc, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut);
	ODBC_ENTER_HDBC;
	odbc_errs_add(&dbc->errs, "HYC00", "SQLBrowseConnect: function not implemented");
	ODBC_EXIT_(dbc);
}
#endif


ODBC_FUNC(SQLColumnPrivileges, (P(SQLHSTMT,hstmt), PCHARIN(CatalogName,SQLSMALLINT), PCHARIN(SchemaName,SQLSMALLINT),
	PCHARIN(TableName,SQLSMALLINT), PCHARIN(ColumnName,SQLSMALLINT) WIDE))
{
	int retcode;

	ODBC_ENTER_HSTMT;

	retcode =
		odbc_stat_execute(stmt _wide, "sp_column_privileges", 4, "O@table_qualifier", szCatalogName, cbCatalogName,
				  "O@table_owner", szSchemaName, cbSchemaName, "O@table_name", szTableName, cbTableName,
				  "P@column_name", szColumnName, cbColumnName);
	if (SQL_SUCCEEDED(retcode) && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) {
		odbc_col_setname(stmt, 1, "TABLE_CAT");
		odbc_col_setname(stmt, 2, "TABLE_SCHEM");
	}
	ODBC_EXIT_(stmt);
}

#if 0
SQLRETURN ODBC_API
SQLDescribeParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT FAR * pfSqlType, SQLUINTEGER FAR * pcbParamDef,
		 SQLSMALLINT FAR * pibScale, SQLSMALLINT FAR * pfNullable)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLDescribeParam(%p, %d, %p, %p, %p, %p)\n", 
			hstmt, ipar, pfSqlType, pcbParamDef, pibScale, pfNullable);
	ODBC_ENTER_HSTMT;
	odbc_errs_add(&stmt->errs, "HYC00", "SQLDescribeParam: function not implemented");
	ODBC_EXIT_(stmt);
}
#endif


SQLRETURN ODBC_PUBLIC ODBC_API
SQLExtendedFetch(SQLHSTMT hstmt, SQLUSMALLINT fFetchType, SQLROWOFFSET irow, SQLROWSETSIZE FAR * pcrow, SQLUSMALLINT FAR * rgfRowStatus)
{
	SQLRETURN ret;
	SQLULEN * tmp_rows;
	SQLUSMALLINT * tmp_status;
	SQLULEN tmp_size;
	SQLLEN * tmp_offset;
	SQLPOINTER tmp_bookmark;
	SQLULEN bookmark;
	SQLULEN out_len = 0;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLExtendedFetch(%p, %d, %d, %p, %p)\n", 
			hstmt, fFetchType, (int)irow, pcrow, rgfRowStatus);

	if (fFetchType != SQL_FETCH_NEXT && !stmt->dbc->cursor_support) {
		odbc_errs_add(&stmt->errs, "HY106", NULL);
		ODBC_EXIT_(stmt);
	}

	/* save and change IRD/ARD state */
	tmp_rows = stmt->ird->header.sql_desc_rows_processed_ptr;
	stmt->ird->header.sql_desc_rows_processed_ptr = &out_len;
	tmp_status = stmt->ird->header.sql_desc_array_status_ptr;
	stmt->ird->header.sql_desc_array_status_ptr = rgfRowStatus;
	tmp_size = stmt->ard->header.sql_desc_array_size;
	stmt->ard->header.sql_desc_array_size = stmt->sql_rowset_size;
	tmp_offset = stmt->ard->header.sql_desc_bind_offset_ptr;
	stmt->ard->header.sql_desc_bind_offset_ptr = NULL;
	tmp_bookmark = stmt->attr.fetch_bookmark_ptr;

	/* SQL_FETCH_BOOKMARK different */
	if (fFetchType == SQL_FETCH_BOOKMARK) {
		bookmark = irow;
		irow = 0;
		stmt->attr.fetch_bookmark_ptr = &bookmark;
	}
	
	/* TODO errors are sligthly different ... perhaps it's better to leave DM do this job ?? */
	/* TODO check fFetchType can be converted to USMALLINT */
	ret = _SQLFetch(stmt, fFetchType, irow);

	/* restore IRD/ARD */
	stmt->ird->header.sql_desc_rows_processed_ptr = tmp_rows;
	if (pcrow)
		*pcrow = out_len;
	stmt->ird->header.sql_desc_array_status_ptr = tmp_status;
	stmt->ard->header.sql_desc_array_size = tmp_size;
	stmt->ard->header.sql_desc_bind_offset_ptr = tmp_offset;
	stmt->attr.fetch_bookmark_ptr = tmp_bookmark;

	ODBC_EXIT(stmt, ret);
}

ODBC_FUNC(SQLForeignKeys, (P(SQLHSTMT,hstmt), PCHARIN(PkCatalogName,SQLSMALLINT),
	PCHARIN(PkSchemaName,SQLSMALLINT), PCHARIN(PkTableName,SQLSMALLINT),
	PCHARIN(FkCatalogName,SQLSMALLINT), PCHARIN(FkSchemaName,SQLSMALLINT),
	PCHARIN(FkTableName,SQLSMALLINT) WIDE))
{
	int retcode;

	ODBC_ENTER_HSTMT;

	retcode =
		odbc_stat_execute(stmt _wide, "sp_fkeys", 6, "O@pktable_qualifier", szPkCatalogName, cbPkCatalogName, "O@pktable_owner",
				  szPkSchemaName, cbPkSchemaName, "O@pktable_name", szPkTableName, cbPkTableName,
				  "O@fktable_qualifier", szFkCatalogName, cbFkCatalogName, "O@fktable_owner", szFkSchemaName,
				  cbFkSchemaName, "O@fktable_name", szFkTableName, cbFkTableName);
	if (SQL_SUCCEEDED(retcode) && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) {
		odbc_col_setname(stmt, 1, "PKTABLE_CAT");
		odbc_col_setname(stmt, 2, "PKTABLE_SCHEM");
		odbc_col_setname(stmt, 5, "FKTABLE_CAT");
		odbc_col_setname(stmt, 6, "FKTABLE_SCHEM");
	}
	ODBC_EXIT_(stmt);
}

static int
odbc_lock_statement(TDS_STMT* stmt)
{
#if ENABLE_ODBC_MARS
	TDSSOCKET *tds = stmt->tds;


	/* we already own a socket, just use it */
	if (!tds) {
		/* try with one saved into DBC */
		TDSSOCKET *dbc_tds = stmt->dbc->tds_socket;
		tds_mutex_lock(&stmt->dbc->mtx);

		if (stmt->dbc->current_statement == NULL
		    || stmt->dbc->current_statement == stmt) {
			tds = dbc_tds;
			stmt->dbc->current_statement = stmt;
		}

		/* try to grab current locked one */
		if (!tds && dbc_tds->state == TDS_IDLE) {
			stmt->dbc->current_statement->tds = NULL;
			tds = dbc_tds;
			stmt->dbc->current_statement = stmt;
		}
		tds_mutex_unlock(&stmt->dbc->mtx);

		/* try with MARS */
		if (!tds)
			tds = tds_alloc_additional_socket(dbc_tds->conn);
	}
	if (tds) {
		tds->query_timeout = (stmt->attr.query_timeout != DEFAULT_QUERY_TIMEOUT) ?
			stmt->attr.query_timeout : stmt->dbc->default_query_timeout;
		tds_set_parent(tds, stmt);
		stmt->tds = tds;
		return 1;
	}
	odbc_errs_add(&stmt->errs, "24000", NULL);
	return 0;
#else
	TDSSOCKET *tds = stmt->dbc->tds_socket;

	tds_mutex_lock(&stmt->dbc->mtx);
	if (stmt->dbc->current_statement != NULL
	    && stmt->dbc->current_statement != stmt) {
		if (!tds || tds->state != TDS_IDLE) {
			tds_mutex_unlock(&stmt->dbc->mtx);
			odbc_errs_add(&stmt->errs, "24000", NULL);
			return 0;
		}
		stmt->dbc->current_statement->tds = NULL;
	}
	stmt->dbc->current_statement = stmt;
	if (tds) {
		tds->query_timeout = (stmt->attr.query_timeout != DEFAULT_QUERY_TIMEOUT) ?
			stmt->attr.query_timeout : stmt->dbc->default_query_timeout;
		tds_set_parent(tds, stmt);
		stmt->tds = tds;
	}
	tds_mutex_unlock(&stmt->dbc->mtx);
	return 1;
#endif
}

static void
odbc_unlock_statement(TDS_STMT* stmt)
{
	TDSSOCKET * tds = stmt->tds;

	tds_mutex_lock(&stmt->dbc->mtx);
	if (stmt->dbc->current_statement == stmt) {
		assert(stmt->tds);
		if (stmt->tds->state == TDS_IDLE) {
			stmt->dbc->current_statement = NULL;
			tds_set_parent(stmt->tds, stmt->dbc);
			stmt->tds = NULL;
		}
#if ENABLE_ODBC_MARS
	} else if (tds) {
		if (tds->state == TDS_IDLE || tds->state == TDS_DEAD) {
			tds_free_socket(tds);
			stmt->tds = NULL;
		}
#endif
	}
	tds_mutex_unlock(&stmt->dbc->mtx);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLMoreResults(SQLHSTMT hstmt)
{
	TDSSOCKET *tds;
	TDS_INT result_type;
	TDSRET tdsret;
	int in_row = 0;
	SQLUSMALLINT param_status;
	int token_flags;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLMoreResults(%p)\n", hstmt);

	tds = stmt->tds;

	/* We already read all results... */
	if (!tds)
		ODBC_EXIT(stmt, SQL_NO_DATA);

	stmt->row_count = TDS_NO_COUNT;
	stmt->special_row = ODBC_SPECIAL_NONE;

	/* TODO this code is TOO similar to _SQLExecute, merge it - freddy77 */
	/* try to go to the next recordset */
	if (stmt->row_status == IN_COMPUTE_ROW) {
		/* FIXME doesn't seem so fine ... - freddy77 */
		tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_TRAILING);
		stmt->row_status = IN_COMPUTE_ROW;
		in_row = 1;
	}

	param_status = SQL_PARAM_SUCCESS;
	token_flags = (TDS_TOKEN_RESULTS & (~TDS_STOPAT_COMPUTE)) | TDS_RETURN_COMPUTE;
	if (stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3)
		token_flags |= TDS_RETURN_MSG;
	for (;;) {
		result_type = odbc_process_tokens(stmt, token_flags);
		tdsdump_log(TDS_DBG_INFO1, "SQLMoreResults: result_type=%d, row_count=%" PRId64 ", lastrc=%d\n",
						result_type, stmt->row_count, stmt->errs.lastrc);
		switch (result_type) {
		case TDS_CMD_DONE:
#if 1 /* !UNIXODBC */
			tds_free_all_results(tds);
#endif
			odbc_populate_ird(stmt);
			odbc_unlock_statement(stmt);
			if (stmt->row_count == TDS_NO_COUNT && !in_row) {
				stmt->row_status = NOT_IN_ROW;
				tdsdump_log(TDS_DBG_INFO1, "SQLMoreResults: row_status=%d\n", stmt->row_status);
			}
			tdsdump_log(TDS_DBG_INFO1, "SQLMoreResults: row_count=%" PRId64 ", lastrc=%d\n", stmt->row_count, stmt->errs.lastrc);
			if (stmt->row_count == TDS_NO_COUNT) {
				if (stmt->errs.lastrc == SQL_SUCCESS || stmt->errs.lastrc == SQL_SUCCESS_WITH_INFO)
					ODBC_EXIT(stmt, SQL_NO_DATA);
			}
			ODBC_EXIT_(stmt);

		case TDS_CMD_FAIL:
			ODBC_SAFE_ERROR(stmt);
			ODBC_EXIT_(stmt);

		case TDS_COMPUTE_RESULT:
			switch (stmt->row_status) {
			/* skip this recordset */
			case IN_COMPUTE_ROW:
				/* TODO here we should set current_results to normal results */
				in_row = 1;
				/* fall through */
			/* in normal row, put in compute status */
			case AFTER_COMPUTE_ROW:
			case IN_NORMAL_ROW:
			case PRE_NORMAL_ROW:
				stmt->row_status = IN_COMPUTE_ROW;
				odbc_populate_ird(stmt);
				ODBC_EXIT_(stmt);
			case NOT_IN_ROW:
				/* this should never happen, protocol error */
				ODBC_SAFE_ERROR(stmt);
				ODBC_EXIT_(stmt);
				break;
			}
			break;

		case TDS_ROW_RESULT:
			if (in_row || (stmt->row_status != IN_NORMAL_ROW && stmt->row_status != PRE_NORMAL_ROW)) {
				stmt->row_status = PRE_NORMAL_ROW;
				odbc_populate_ird(stmt);
				ODBC_EXIT_(stmt);
			}
			/* Skipping current result set's rows to access next resultset or proc's retval */
			tdsret = tds_process_tokens(tds, &result_type, NULL, TDS_STOPAT_ROWFMT|TDS_RETURN_DONE|TDS_STOPAT_COMPUTE);
			/* TODO should we set in_row ?? */
			switch (tdsret) {
			case TDS_CANCELLED:
				odbc_errs_add(&stmt->errs, "HY008", NULL);
			default:
				if (TDS_FAILED(tdsret)) {
					ODBC_SAFE_ERROR(stmt);
					ODBC_EXIT_(stmt);
				}
			}
			break;

		case TDS_DONE_RESULT:
		case TDS_DONEPROC_RESULT:
			/* FIXME here ??? */
			if (!in_row)
				tds_free_all_results(tds);
			switch (stmt->errs.lastrc) {
			case SQL_ERROR:
				param_status = SQL_PARAM_ERROR;
				break;
			case SQL_SUCCESS_WITH_INFO:
				param_status = SQL_PARAM_SUCCESS_WITH_INFO;
				break;
			}
			if (stmt->curr_param_row < stmt->num_param_rows) {
				if (stmt->ipd->header.sql_desc_array_status_ptr)
					stmt->ipd->header.sql_desc_array_status_ptr[stmt->curr_param_row] = param_status;
				++stmt->curr_param_row;
				if (stmt->ipd->header.sql_desc_rows_processed_ptr)
					*stmt->ipd->header.sql_desc_rows_processed_ptr = stmt->curr_param_row;
			}
			if (stmt->curr_param_row < stmt->num_param_rows) {
#if 0
				if (stmt->errs.lastrc == SQL_SUCCESS_WITH_INFO)
					found_info = 1;
				if (stmt->errs.lastrc == SQL_ERROR)
					found_error = 1;
#endif
				stmt->errs.lastrc = SQL_SUCCESS;
				param_status = SQL_PARAM_SUCCESS;
				break;
			}
			odbc_populate_ird(stmt);
			ODBC_EXIT_(stmt);
			break;

			/*
			 * TODO test flags ? check error and change result ? 
			 * see also other DONEINPROC handle (below)
			 */
		case TDS_DONEINPROC_RESULT:
			if (in_row) {
				odbc_populate_ird(stmt);
				ODBC_EXIT_(stmt);
			}
			/* TODO perhaps it can be a problem if SET NOCOUNT ON, test it */
			tds_free_all_results(tds);
			odbc_populate_ird(stmt);
			break;

			/* do not stop at metadata, an error can follow... */
		case TDS_ROWFMT_RESULT:
			if (in_row) {
				odbc_populate_ird(stmt);
				ODBC_EXIT_(stmt);
			}
			stmt->row = 0;
			stmt->row_count = TDS_NO_COUNT;
			/* we expect a row */
			stmt->row_status = PRE_NORMAL_ROW;
			in_row = 1;
			break;

		case TDS_MSG_RESULT:
			if (!in_row) {
				tds_free_all_results(tds);
				odbc_populate_ird(stmt);
			}
			in_row = 1;
			break;
		}
	}
	ODBC_EXIT(stmt, SQL_ERROR);
}

ODBC_FUNC(SQLNativeSql, (P(SQLHDBC,hdbc), PCHARIN(SqlStrIn,SQLINTEGER),
	PCHAROUT(SqlStr,SQLINTEGER) WIDE))
{
	SQLRETURN ret = SQL_SUCCESS;
	DSTR query;

	ODBC_ENTER_HDBC;

	tds_dstr_init(&query);

#ifdef TDS_NO_DM
	if (!szSqlStrIn || !IS_VALID_LEN(cbSqlStrIn)) {
		odbc_errs_add(&dbc->errs, "HY009", NULL);
		ODBC_EXIT_(dbc);
	}
#endif

	if (!odbc_dstr_copy(dbc, &query, cbSqlStrIn, szSqlStrIn)) {
		odbc_errs_add(&dbc->errs, "HY001", NULL);
		ODBC_EXIT_(dbc);
	}

	/* TODO support not null terminated in native_sql */
	native_sql(dbc, tds_dstr_buf(&query));

	/* FIXME if error set some kind of error */
	ret = odbc_set_string(dbc, szSqlStr, cbSqlStrMax, pcbSqlStr, tds_dstr_cstr(&query), -1);

	tds_dstr_free(&query);

	ODBC_EXIT(dbc, ret);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLNumParams(SQLHSTMT hstmt, SQLSMALLINT FAR * pcpar)
{
	ODBC_ENTER_HSTMT;
	tdsdump_log(TDS_DBG_FUNC, "SQLNumParams(%p, %p)\n", hstmt, pcpar);
	*pcpar = stmt->param_count;
	ODBC_EXIT_(stmt);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLParamOptions(SQLHSTMT hstmt, SQLULEN crow, SQLULEN FAR * pirow)
{
	SQLRETURN res;

	tdsdump_log(TDS_DBG_FUNC, "SQLParamOptions(%p, %lu, %p)\n", hstmt, (unsigned long int)crow, pirow);

	/* emulate for ODBC 2 DM */
	res = _SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMS_PROCESSED_PTR, pirow, 0 _wide0);
	if (res != SQL_SUCCESS)
		return res;
	return _SQLSetStmtAttr(hstmt, SQL_ATTR_PARAMSET_SIZE, (SQLPOINTER) (TDS_INTPTR) crow, 0 _wide0);
}

ODBC_FUNC(SQLPrimaryKeys, (P(SQLHSTMT,hstmt), PCHARIN(CatalogName,SQLSMALLINT),
	PCHARIN(SchemaName,SQLSMALLINT), PCHARIN(TableName,SQLSMALLINT) WIDE))
{
	int retcode;

	ODBC_ENTER_HSTMT;

	retcode =
		odbc_stat_execute(stmt _wide, "sp_pkeys", 3, "O@table_qualifier", szCatalogName, cbCatalogName, "O@table_owner",
				  szSchemaName, cbSchemaName, "O@table_name", szTableName, cbTableName);
	if (SQL_SUCCEEDED(retcode) && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) {
		odbc_col_setname(stmt, 1, "TABLE_CAT");
		odbc_col_setname(stmt, 2, "TABLE_SCHEM");
	}
	ODBC_EXIT_(stmt);
}

ODBC_FUNC(SQLProcedureColumns, (P(SQLHSTMT,hstmt), PCHARIN(CatalogName,SQLSMALLINT),
	PCHARIN(SchemaName,SQLSMALLINT), PCHARIN(ProcName,SQLSMALLINT), PCHARIN(ColumnName,SQLSMALLINT) WIDE))
{
	int retcode;

	ODBC_ENTER_HSTMT;

	retcode =
		odbc_stat_execute(stmt _wide, "sp_sproc_columns", TDS_IS_MSSQL(stmt->dbc->tds_socket) ? 5 : 4,
				  "O@procedure_qualifier", szCatalogName, cbCatalogName,
				  "P@procedure_owner", szSchemaName, cbSchemaName, "P@procedure_name", szProcName, cbProcName,
				  "P@column_name", szColumnName, cbColumnName, "V@ODBCVer", (char*) NULL, 0);
	if (SQL_SUCCEEDED(retcode) && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) {
		odbc_col_setname(stmt, 1, "PROCEDURE_CAT");
		odbc_col_setname(stmt, 2, "PROCEDURE_SCHEM");
		odbc_col_setname(stmt, 8, "COLUMN_SIZE");
		odbc_col_setname(stmt, 9, "BUFFER_LENGTH");
		odbc_col_setname(stmt, 10, "DECIMAL_DIGITS");
		odbc_col_setname(stmt, 11, "NUM_PREC_RADIX");
		if (TDS_IS_SYBASE(stmt->dbc->tds_socket))
			stmt->special_row = ODBC_SPECIAL_PROCEDURECOLUMNS;
	}
	ODBC_EXIT_(stmt);
}

ODBC_FUNC(SQLProcedures, (P(SQLHSTMT,hstmt), PCHARIN(CatalogName,SQLSMALLINT),
	PCHARIN(SchemaName,SQLSMALLINT), PCHARIN(ProcName,SQLSMALLINT) WIDE))
{
	int retcode;

	ODBC_ENTER_HSTMT;

	retcode =
		odbc_stat_execute(stmt _wide, "..sp_stored_procedures", 3, "P@sp_name", szProcName, cbProcName, "P@sp_owner", szSchemaName,
				  cbSchemaName, "O@sp_qualifier", szCatalogName, cbCatalogName);
	if (SQL_SUCCEEDED(retcode) && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) {
		odbc_col_setname(stmt, 1, "PROCEDURE_CAT");
		odbc_col_setname(stmt, 2, "PROCEDURE_SCHEM");
	}
	ODBC_EXIT_(stmt);
}

static TDSPARAMINFO*
odbc_build_update_params(TDS_STMT * stmt, unsigned int n_row)
{
	unsigned int n;
	TDSPARAMINFO * params = NULL;
	struct _drecord *drec_ird;

	for (n = 0; n < stmt->ird->header.sql_desc_count && n < stmt->ard->header.sql_desc_count; ++n) {
		TDSPARAMINFO *temp_params;
		TDSCOLUMN *curcol;

		drec_ird = &stmt->ird->records[n];

		if (drec_ird->sql_desc_updatable == SQL_FALSE)
			continue;

		/* we have certainly a parameter */
		if (!(temp_params = tds_alloc_param_result(params))) {
			tds_free_param_results(params);
			odbc_errs_add(&stmt->errs, "HY001", NULL);
			return NULL;
		}
                params = temp_params;

		curcol = params->columns[params->num_cols - 1];
		tds_strlcpy(curcol->column_name, tds_dstr_cstr(&drec_ird->sql_desc_name), sizeof(curcol->column_name));
		curcol->column_namelen = strlen(curcol->column_name);

		/* TODO use all infos... */
		tds_dstr_dup(&curcol->table_name, &drec_ird->sql_desc_base_table_name);

		switch (odbc_sql2tds(stmt, drec_ird, &stmt->ard->records[n], curcol, 1, stmt->ard, n_row)) {
		case SQL_NEED_DATA:
			odbc_errs_add(&stmt->errs, "HY001", NULL);
		case SQL_ERROR:
			tds_free_param_results(params);
			return NULL;
		}
	}
	return params;
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLSetPos(SQLHSTMT hstmt, SQLSETPOSIROW irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock)
{
	TDSRET ret;
	TDSSOCKET *tds;
	TDS_CURSOR_OPERATION op;
	TDSPARAMINFO *params = NULL;
	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLSetPos(%p, %ld, %d, %d)\n", 
			hstmt, (long) irow, fOption, fLock);

	if (!stmt->dbc->cursor_support) {
		odbc_errs_add(&stmt->errs, "HYC00", "SQLSetPos: function not implemented");
		ODBC_EXIT_(stmt);
	}

	/* TODO handle irow == 0 (all rows) */

	if (!stmt->cursor) {
		odbc_errs_add(&stmt->errs, "HY109", NULL);
		ODBC_EXIT_(stmt);
	}

	switch (fOption) {
	case SQL_POSITION:
		op = TDS_CURSOR_POSITION;
		break;
	/* TODO cursor support */
	case SQL_REFRESH:
	default:
		odbc_errs_add(&stmt->errs, "HY092", NULL);
		ODBC_EXIT_(stmt);
		break;
	case SQL_UPDATE:
		op = TDS_CURSOR_UPDATE;
		/* prepare paremeters for update */
		/* scan all columns and build parameter list */
		params = odbc_build_update_params(stmt, irow >= 1 ? irow - 1 : 0);
		if (!params) {
			ODBC_SAFE_ERROR(stmt);
			ODBC_EXIT_(stmt);
		}
		break;
	case SQL_DELETE:
		op = TDS_CURSOR_DELETE;
		break;
	case SQL_ADD:
		op = TDS_CURSOR_INSERT;
		break;
	}

	if (!odbc_lock_statement(stmt)) {
		tds_free_param_results(params);
		ODBC_EXIT_(stmt);
	}

	tds = stmt->tds;

	if (TDS_FAILED(tds_cursor_update(tds, stmt->cursor, op, irow, params))) {
		tds_free_param_results(params);
		ODBC_SAFE_ERROR(stmt);
		ODBC_EXIT_(stmt);
	}
	tds_free_param_results(params);
	params = NULL;

	ret = tds_process_simple_query(tds);
	odbc_unlock_statement(stmt);
	if (TDS_FAILED(ret)) {
		ODBC_SAFE_ERROR(stmt);
		ODBC_EXIT_(stmt);
	}

	ODBC_EXIT_(stmt);
}

ODBC_FUNC(SQLTablePrivileges, (P(SQLHSTMT,hstmt), PCHARIN(CatalogName,SQLSMALLINT),
	PCHARIN(SchemaName,SQLSMALLINT), PCHARIN(TableName,SQLSMALLINT) WIDE))
{
	int retcode;

	ODBC_ENTER_HSTMT;

	retcode =
		odbc_stat_execute(stmt _wide, "sp_table_privileges", 3, "O@table_qualifier", szCatalogName, cbCatalogName,
				  "P@table_owner", szSchemaName, cbSchemaName, "P@table_name", szTableName, cbTableName);
	if (SQL_SUCCEEDED(retcode) && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) {
		odbc_col_setname(stmt, 1, "TABLE_CAT");
		odbc_col_setname(stmt, 2, "TABLE_SCHEM");
	}
	ODBC_EXIT_(stmt);
}

#if (ODBCVER >= 0x0300)
SQLRETURN ODBC_PUBLIC ODBC_API
SQLSetEnvAttr(SQLHENV henv, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER StringLength)
{
	SQLINTEGER i_val = (SQLINTEGER) (TDS_INTPTR) Value;

	ODBC_ENTER_HENV;

	tdsdump_log(TDS_DBG_FUNC, "SQLSetEnvAttr(%p, %d, %p, %d)\n", henv, (int)Attribute, Value, (int)StringLength);

	switch (Attribute) {
	case SQL_ATTR_CONNECTION_POOLING:
	case SQL_ATTR_CP_MATCH:
		odbc_errs_add(&env->errs, "HYC00", NULL);
		break;
	case SQL_ATTR_ODBC_VERSION:
		switch (i_val) {
		case SQL_OV_ODBC3:
		case SQL_OV_ODBC2:
			env->attr.odbc_version = i_val;
			break;
		default:
			odbc_errs_add(&env->errs, "HY024", NULL);
			break;
		}
		break;
	case SQL_ATTR_OUTPUT_NTS:
		env->attr.output_nts = i_val;
		/* TODO - Make this really work */
		env->attr.output_nts = SQL_TRUE;
		break;
	default:
		odbc_errs_add(&env->errs, "HY092", NULL);
		break;
	}
	ODBC_EXIT_(env);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetEnvAttr(SQLHENV henv, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
	size_t size;
	void *src;

	ODBC_ENTER_HENV;

	tdsdump_log(TDS_DBG_FUNC, "SQLGetEnvAttr(%p, %d, %p, %d, %p)\n", 
			henv, (int)Attribute, Value, (int)BufferLength, StringLength);

	switch (Attribute) {
	case SQL_ATTR_CONNECTION_POOLING:
		src = &env->attr.connection_pooling;
		size = sizeof(env->attr.connection_pooling);
		break;
	case SQL_ATTR_CP_MATCH:
		src = &env->attr.cp_match;
		size = sizeof(env->attr.cp_match);
		break;
	case SQL_ATTR_ODBC_VERSION:
		src = &env->attr.odbc_version;
		size = sizeof(env->attr.odbc_version);
		break;
	case SQL_ATTR_OUTPUT_NTS:
		/* TODO handle output_nts flags */
		env->attr.output_nts = SQL_TRUE;
		src = &env->attr.output_nts;
		size = sizeof(env->attr.output_nts);
		break;
	default:
		odbc_errs_add(&env->errs, "HY092", NULL);
		ODBC_EXIT_(env);
		break;
	}

	if (StringLength) {
		*StringLength = size;
	}
	memcpy(Value, src, size);

	ODBC_EXIT_(env);
}

#endif

#define IRD_UPDATE(desc, errs, exit) \
	do { \
		if (desc->type == DESC_IRD && ((TDS_STMT*)desc->parent)->need_reprepare && \
		    odbc_update_ird((TDS_STMT*)desc->parent, errs) != SQL_SUCCESS) \
			exit; \
	} while(0)

static SQLRETURN
_SQLBindParameter(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT fParamType, SQLSMALLINT fCType, SQLSMALLINT fSqlType,
		  SQLULEN cbColDef, SQLSMALLINT ibScale, SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN FAR * pcbValue)
{
	TDS_DESC *apd, *ipd;
	struct _drecord *drec;
	SQLSMALLINT orig_apd_size, orig_ipd_size;
	int is_numeric = 0;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "_SQLBindParameter(%p, %u, %d, %d, %d, %u, %d, %p, %d, %p)\n", 
			hstmt, (unsigned short)ipar, (int)fParamType, (int)fCType, (int)fSqlType, (unsigned int)cbColDef, 
			(int)ibScale, rgbValue, (int)cbValueMax, pcbValue);

#ifdef TDS_NO_DM
	/* TODO - more error checking ...  XXX smurph */

	/* Check param type */
	switch (fParamType) {
	case SQL_PARAM_INPUT:
	case SQL_PARAM_INPUT_OUTPUT:
	case SQL_PARAM_OUTPUT:
		break;
	default:
		odbc_errs_add(&stmt->errs, "HY105", NULL);
		ODBC_EXIT_(stmt);
	}

	/* Check max buffer length */
	if (cbValueMax < 0) {
		odbc_errs_add(&stmt->errs, "HY090", NULL);
		ODBC_EXIT_(stmt);
	}
#endif

	/* check cbColDef and ibScale */
	if (fSqlType == SQL_DECIMAL || fSqlType == SQL_NUMERIC) {
		is_numeric = 1;
		if (cbColDef < 1 || cbColDef > 38) {
			odbc_errs_add(&stmt->errs, "HY104", "Invalid precision value");
			ODBC_EXIT_(stmt);
		}
		if (ibScale < 0 || ibScale > cbColDef) {
			odbc_errs_add(&stmt->errs, "HY104", "Invalid scale value");
			ODBC_EXIT_(stmt);
		}
	}

	/* Check parameter number */
	if (ipar <= 0 || ipar > 4000) {
		odbc_errs_add(&stmt->errs, "07009", NULL);
		ODBC_EXIT_(stmt);
	}

	/* fill APD related fields */
	apd = stmt->apd;
	orig_apd_size = apd->header.sql_desc_count;
	if (ipar > apd->header.sql_desc_count && desc_alloc_records(apd, ipar) != SQL_SUCCESS) {
		odbc_errs_add(&stmt->errs, "HY001", NULL);
		ODBC_EXIT_(stmt);
	}
	drec = &apd->records[ipar - 1];

	if (odbc_set_concise_c_type(fCType, drec, 0) != SQL_SUCCESS) {
		desc_alloc_records(apd, orig_apd_size);
		odbc_errs_add(&stmt->errs, "HY004", NULL);
		ODBC_EXIT_(stmt);
	}

	stmt->need_reprepare = 1;

	/* TODO other types ?? handle SQL_C_DEFAULT */
	if (drec->sql_desc_type == SQL_C_CHAR || drec->sql_desc_type == SQL_C_WCHAR || drec->sql_desc_type == SQL_C_BINARY)
		drec->sql_desc_octet_length = cbValueMax;
	drec->sql_desc_indicator_ptr = pcbValue;
	drec->sql_desc_octet_length_ptr = pcbValue;
	drec->sql_desc_data_ptr = (char *) rgbValue;

	/* field IPD related fields */
	ipd = stmt->ipd;
	orig_ipd_size = ipd->header.sql_desc_count;
	if (ipar > ipd->header.sql_desc_count && desc_alloc_records(ipd, ipar) != SQL_SUCCESS) {
		desc_alloc_records(apd, orig_apd_size);
		odbc_errs_add(&stmt->errs, "HY001", NULL);
		ODBC_EXIT_(stmt);
	}
	drec = &ipd->records[ipar - 1];

	drec->sql_desc_parameter_type = fParamType;
	if (odbc_set_concise_sql_type(fSqlType, drec, 0) != SQL_SUCCESS) {
		desc_alloc_records(ipd, orig_ipd_size);
		desc_alloc_records(apd, orig_apd_size);
		odbc_errs_add(&stmt->errs, "HY004", NULL);
		ODBC_EXIT_(stmt);
	}
	if (is_numeric) {
		drec->sql_desc_precision = cbColDef;
		drec->sql_desc_scale = ibScale;
	} else {
		drec->sql_desc_length = cbColDef;
	}

	ODBC_EXIT_(stmt);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLBindParameter(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT fParamType, SQLSMALLINT fCType, SQLSMALLINT fSqlType,
		 SQLULEN cbColDef, SQLSMALLINT ibScale, SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN FAR * pcbValue)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLBindParameter(%p, %u, %d, %d, %d, %u, %d, %p, %d, %p)\n", 
			hstmt, (unsigned)ipar, fParamType, fCType, (int)fSqlType, (unsigned)cbColDef, ibScale, rgbValue, (int)cbValueMax, pcbValue);
	return _SQLBindParameter(hstmt, ipar, fParamType, fCType, fSqlType, cbColDef, ibScale, rgbValue, cbValueMax, pcbValue);
}


/* compatibility with X/Open */
SQLRETURN ODBC_PUBLIC ODBC_API
SQLBindParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT fCType, SQLSMALLINT fSqlType, SQLULEN cbColDef, SQLSMALLINT ibScale,
	     SQLPOINTER rgbValue, SQLLEN FAR * pcbValue)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLBindParam(%p, %d, %d, %d, %u, %d, %p, %p)\n", 
			hstmt, ipar, fCType, fSqlType, (unsigned)cbColDef, ibScale, rgbValue, pcbValue);
	return _SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT, fCType, fSqlType, cbColDef, ibScale, rgbValue, 0, pcbValue);
}

#if (ODBCVER >= 0x0300)
SQLRETURN ODBC_PUBLIC ODBC_API
SQLAllocHandle(SQLSMALLINT HandleType, SQLHANDLE InputHandle, SQLHANDLE * OutputHandle)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLAllocHandle(%d, %p, %p)\n", HandleType, InputHandle, OutputHandle);

	switch (HandleType) {
	case SQL_HANDLE_STMT:
		return _SQLAllocStmt(InputHandle, OutputHandle);
		break;
	case SQL_HANDLE_DBC:
		return _SQLAllocConnect(InputHandle, OutputHandle);
		break;
	case SQL_HANDLE_ENV:
		return _SQLAllocEnv(OutputHandle, SQL_OV_ODBC3);
		break;
	case SQL_HANDLE_DESC:
		return _SQLAllocDesc(InputHandle, OutputHandle);
		break;
	}
	
	/*
	 * As the documentation puts it, 
	 *	"There is no handle with which to associate additional diagnostic information."
	 *
	 * The DM must catch HY092 because the driver has no valid handle at this early stage in which 
	 * to store the error for later retrieval by the application.  
	 */
	tdsdump_log(TDS_DBG_FUNC, "SQLAllocHandle(): invalid HandleType, error HY092: should be caught by DM\n");
	return SQL_ERROR;
}
#endif

static SQLRETURN
_SQLAllocConnect(SQLHENV henv, SQLHDBC FAR * phdbc)
{
	TDS_DBC *dbc;

	ODBC_ENTER_HENV;

	tdsdump_log(TDS_DBG_FUNC, "_SQLAllocConnect(%p, %p)\n", henv, phdbc);

	dbc = (TDS_DBC *) calloc(1, sizeof(TDS_DBC));
	if (!dbc) {
		odbc_errs_add(&env->errs, "HY001", NULL);
		ODBC_EXIT_(env);
	}

	dbc->htype = SQL_HANDLE_DBC;
	dbc->env = env;
	tds_dstr_init(&dbc->server);
	tds_dstr_init(&dbc->dsn);

	dbc->attr.cursor_type = SQL_CURSOR_FORWARD_ONLY;
	dbc->attr.access_mode = SQL_MODE_READ_WRITE;
	dbc->attr.async_enable = SQL_ASYNC_ENABLE_OFF;
	dbc->attr.auto_ipd = SQL_FALSE;
	/*
	 * spinellia@acm.org
	 * after login is enabled autocommit
	 */
	dbc->attr.autocommit = SQL_AUTOCOMMIT_ON;
	dbc->attr.connection_dead = SQL_CD_TRUE;	/* No connection yet */
	dbc->attr.connection_timeout = 0;
	/* This is set in the environment change function */
	tds_dstr_init(&dbc->attr.current_catalog);
	dbc->attr.login_timeout = 0;	/* TODO */
	dbc->attr.metadata_id = SQL_FALSE;
	dbc->attr.odbc_cursors = SQL_CUR_USE_IF_NEEDED;
	dbc->attr.packet_size = 0;
	dbc->attr.quite_mode = NULL;	/* We don't support GUI dialogs yet */
#ifdef TDS_NO_DM
	dbc->attr.trace = SQL_OPT_TRACE_OFF;
	tds_dstr_init(&dbc->attr.tracefile);
#endif
	tds_dstr_init(&dbc->attr.translate_lib);
#ifdef ENABLE_ODBC_WIDE
	tds_dstr_init(&dbc->original_charset);
#endif
	dbc->attr.translate_option = 0;
	dbc->attr.txn_isolation = SQL_TXN_READ_COMMITTED;
	dbc->attr.mars_enabled = SQL_MARS_ENABLED_NO;

	tds_mutex_init(&dbc->mtx);
	*phdbc = (SQLHDBC) dbc;

	ODBC_EXIT_(env);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLAllocConnect(SQLHENV henv, SQLHDBC FAR * phdbc)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLAllocConnect(%p, %p)\n", henv, phdbc);

	return _SQLAllocConnect(henv, phdbc);
}

static SQLRETURN
_SQLAllocEnv(SQLHENV FAR * phenv, SQLINTEGER odbc_version)
{
	TDS_ENV *env;
	TDSCONTEXT *ctx;

	tdsdump_log(TDS_DBG_FUNC, "_SQLAllocEnv(%p, %d)\n",
			phenv, (int) odbc_version);

	env = (TDS_ENV *) calloc(1, sizeof(TDS_ENV));
	if (!env)
		return SQL_ERROR;

	env->htype = SQL_HANDLE_ENV;
	env->attr.odbc_version = odbc_version;
	/* TODO use it */
	env->attr.output_nts = SQL_TRUE;

	ctx = tds_alloc_context(env);
	if (!ctx) {
		free(env);
		return SQL_ERROR;
	}
	env->tds_ctx = ctx;
	ctx->msg_handler = odbc_errmsg_handler;
	ctx->err_handler = odbc_errmsg_handler;

	/* ODBC has its own format */
	free(ctx->locale->date_fmt);
	ctx->locale->date_fmt = strdup("%Y-%m-%d %H:%M:%S.%z");

	tds_mutex_init(&env->mtx);
	*phenv = (SQLHENV) env;

	return SQL_SUCCESS;
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLAllocEnv(SQLHENV FAR * phenv)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLAllocEnv(%p)\n", phenv);

	return _SQLAllocEnv(phenv, SQL_OV_ODBC2);
}

static SQLRETURN
_SQLAllocDesc(SQLHDBC hdbc, SQLHDESC FAR * phdesc)
{
	int i;

	ODBC_ENTER_HDBC;

	tdsdump_log(TDS_DBG_FUNC, "_SQLAllocDesc(%p, %p)\n", hdbc, phdesc);

	for (i = 0; ; ++i) {
		if (i >= TDS_MAX_APP_DESC) {
			odbc_errs_add(&dbc->errs, "HY014", NULL);
			break;
		}
		if (dbc->uad[i] == NULL) {
			TDS_DESC *desc = desc_alloc(dbc, DESC_ARD, SQL_DESC_ALLOC_USER);
			if (desc == NULL) {
				odbc_errs_add(&dbc->errs, "HY001", NULL);
				break;
			}
			dbc->uad[i] = desc;
			*phdesc = (SQLHDESC) desc;
			break;
		}
	}
	ODBC_EXIT_(dbc);
}

static SQLRETURN
_SQLAllocStmt(SQLHDBC hdbc, SQLHSTMT FAR * phstmt)
{
	TDS_STMT *stmt;
	char *pstr;

	ODBC_ENTER_HDBC;

	tdsdump_log(TDS_DBG_FUNC, "_SQLAllocStmt(%p, %p)\n", hdbc, phstmt);

	stmt = (TDS_STMT *) calloc(1, sizeof(TDS_STMT));
	if (!stmt) {
		odbc_errs_add(&dbc->errs, "HY001", NULL);
		ODBC_EXIT_(dbc);
	}
	tds_dstr_init(&stmt->cursor_name);

	stmt->htype = SQL_HANDLE_STMT;
	stmt->dbc = dbc;
	stmt->num_param_rows = 1;
	pstr = NULL;
	/* TODO test initial cursor ... */
	if (asprintf(&pstr, "SQL_CUR%lx", (unsigned long) stmt) < 0 || !tds_dstr_set(&stmt->cursor_name, pstr)) {
		free(stmt);
		free(pstr);
		odbc_errs_add(&dbc->errs, "HY001", NULL);
		ODBC_EXIT_(dbc);
	}
	/* do not free pstr tds_dstr_set do it if necessary */

	/* allocate descriptors */
	stmt->ird = desc_alloc(stmt, DESC_IRD, SQL_DESC_ALLOC_AUTO);
	stmt->ard = desc_alloc(stmt, DESC_ARD, SQL_DESC_ALLOC_AUTO);
	stmt->ipd = desc_alloc(stmt, DESC_IPD, SQL_DESC_ALLOC_AUTO);
	stmt->apd = desc_alloc(stmt, DESC_APD, SQL_DESC_ALLOC_AUTO);
	if (!stmt->ird || !stmt->ard || !stmt->ipd || !stmt->apd) {
		tds_dstr_free(&stmt->cursor_name);
		desc_free(stmt->ird);
		desc_free(stmt->ard);
		desc_free(stmt->ipd);
		desc_free(stmt->apd);
		free(stmt);
		odbc_errs_add(&dbc->errs, "HY001", NULL);
		ODBC_EXIT_(dbc);
	}

	/* save original ARD and APD */
	stmt->orig_apd = stmt->apd;
	stmt->orig_ard = stmt->ard;

	/* set the default statement attributes */
/*	stmt->attr.app_param_desc = stmt->apd; */
/*	stmt->attr.app_row_desc = stmt->ard; */
	stmt->attr.async_enable = SQL_ASYNC_ENABLE_OFF;
	stmt->attr.concurrency = SQL_CONCUR_READ_ONLY;
	stmt->attr.cursor_scrollable = SQL_NONSCROLLABLE;
	stmt->attr.cursor_sensitivity = SQL_INSENSITIVE;
	stmt->attr.cursor_type = SQL_CURSOR_FORWARD_ONLY;
	/* TODO ?? why two attributes */
	stmt->attr.enable_auto_ipd = dbc->attr.auto_ipd = SQL_FALSE;
	stmt->attr.fetch_bookmark_ptr = NULL;
/*	stmt->attr.imp_param_desc = stmt->ipd; */
/*	stmt->attr.imp_row_desc = stmt->ird; */
	stmt->attr.keyset_size = 0;
	stmt->attr.max_length = 0;
	stmt->attr.max_rows = 0;
	stmt->attr.metadata_id = dbc->attr.metadata_id;
	/* TODO check this flag in prepare_call */
	stmt->attr.noscan = SQL_NOSCAN_OFF;
	assert(stmt->apd->header.sql_desc_bind_offset_ptr == NULL);
	assert(stmt->apd->header.sql_desc_bind_type == SQL_PARAM_BIND_BY_COLUMN);
	assert(stmt->apd->header.sql_desc_array_status_ptr == NULL);
	assert(stmt->ipd->header.sql_desc_array_status_ptr == NULL);
	assert(stmt->ipd->header.sql_desc_rows_processed_ptr == NULL);
	assert(stmt->apd->header.sql_desc_array_size == 1);
	stmt->attr.query_timeout = DEFAULT_QUERY_TIMEOUT;
	stmt->attr.retrieve_data = SQL_RD_ON;
	assert(stmt->ard->header.sql_desc_array_size == 1);
	assert(stmt->ard->header.sql_desc_bind_offset_ptr == NULL);
	assert(stmt->ard->header.sql_desc_bind_type == SQL_BIND_BY_COLUMN);
	stmt->attr.row_number = 0;
	assert(stmt->ard->header.sql_desc_array_status_ptr == NULL);
	assert(stmt->ird->header.sql_desc_array_status_ptr == NULL);
	assert(stmt->ird->header.sql_desc_rows_processed_ptr == NULL);
	stmt->attr.simulate_cursor = SQL_SC_NON_UNIQUE;
	stmt->attr.use_bookmarks = SQL_UB_OFF;
	tds_dstr_init(&stmt->attr.qn_msgtext);
	tds_dstr_init(&stmt->attr.qn_options);
	stmt->attr.qn_timeout = 432000;

	stmt->sql_rowset_size = 1;

	stmt->row_count = TDS_NO_COUNT;
	stmt->row_status = NOT_IN_ROW;

	/* insert into list */
	stmt->next = dbc->stmt_list;
	if (dbc->stmt_list)
		dbc->stmt_list->prev = stmt;
	dbc->stmt_list = stmt;

	tds_mutex_init(&stmt->mtx);
	*phstmt = (SQLHSTMT) stmt;

	if (dbc->attr.cursor_type != SQL_CURSOR_FORWARD_ONLY)
		_SQLSetStmtAttr(stmt, SQL_CURSOR_TYPE, (SQLPOINTER) (TDS_INTPTR) dbc->attr.cursor_type, SQL_IS_INTEGER _wide0);

	ODBC_EXIT_(dbc);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLAllocStmt(SQLHDBC hdbc, SQLHSTMT FAR * phstmt)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLAllocStmt(%p, %p)\n", hdbc, phstmt);

	return _SQLAllocStmt(hdbc, phstmt);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLBindCol(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLSMALLINT fCType, SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN FAR * pcbValue)
{
	TDS_DESC *ard;
	struct _drecord *drec;
	SQLSMALLINT orig_ard_size;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLBindCol(%p, %d, %d, %p, %d, %p)\n", 
			hstmt, icol, fCType, rgbValue, (int)cbValueMax, pcbValue);

	/* TODO - More error checking XXX smurph */

#ifdef TDS_NO_DM
	/* check conversion type */
	switch (fCType) {
	case SQL_C_CHAR:
	case SQL_C_WCHAR:
	case SQL_C_BINARY:
	case SQL_C_DEFAULT:
		/* check max buffer length */
		if (!IS_VALID_LEN(cbValueMax)) {
			odbc_errs_add(&stmt->errs, "HY090", NULL);
			ODBC_EXIT_(stmt);
		}
		break;
	}
#endif

	if (icol <= 0 || icol > 4000) {
		odbc_errs_add(&stmt->errs, "07009", NULL);
		ODBC_EXIT_(stmt);
	}

	ard = stmt->ard;
	orig_ard_size = ard->header.sql_desc_count;
	if (icol > ard->header.sql_desc_count && desc_alloc_records(ard, icol) != SQL_SUCCESS) {
		odbc_errs_add(&stmt->errs, "HY001", NULL);
		ODBC_EXIT_(stmt);
	}

	drec = &ard->records[icol - 1];

	if (odbc_set_concise_c_type(fCType, drec, 0) != SQL_SUCCESS) {
		desc_alloc_records(ard, orig_ard_size);
		odbc_errs_add(&stmt->errs, "HY003", NULL);
		ODBC_EXIT_(stmt);
	}
	drec->sql_desc_octet_length = cbValueMax;
	drec->sql_desc_octet_length_ptr = pcbValue;
	drec->sql_desc_indicator_ptr = pcbValue;
	drec->sql_desc_data_ptr = rgbValue;

	/* force rebind */
	stmt->row = 0;

	ODBC_EXIT_(stmt);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLCancel(SQLHSTMT hstmt)
{
	TDSSOCKET *tds;

	/*
	 * FIXME this function can be called from other thread, do not free 
	 * errors for this function
	 * If function is called from another thread errors are not touched
	 */
	/* TODO some tests required */
	TDS_STMT *stmt = (TDS_STMT*)hstmt;
	if (SQL_NULL_HSTMT == hstmt || !IS_HSTMT(hstmt))
		return SQL_INVALID_HANDLE;

	tdsdump_log(TDS_DBG_FUNC, "SQLCancel(%p)\n", hstmt);

	tds = stmt->tds;

	/* cancelling an inactive statement ?? */
	if (!tds) {
		ODBC_SAFE_ERROR(stmt);
		ODBC_EXIT_(stmt);
	}
	if (tds_mutex_trylock(&stmt->mtx) == 0) {
		CHECK_STMT_EXTRA(stmt);
		odbc_errs_reset(&stmt->errs);

		/* FIXME test current statement */
		/* FIXME here we are unlocked */

		if (TDS_FAILED(tds_send_cancel(tds))) {
			ODBC_SAFE_ERROR(stmt);
			ODBC_EXIT_(stmt);
		}

		if (TDS_FAILED(tds_process_cancel(tds))) {
			ODBC_SAFE_ERROR(stmt);
			ODBC_EXIT_(stmt);
		}

		/* only if we processed cancel reset statement */
		if (tds->state == TDS_IDLE)
			odbc_unlock_statement(stmt);

		ODBC_EXIT_(stmt);
	}

	/* don't access error here, just return error */
	if (TDS_FAILED(tds_send_cancel(tds)))
		return SQL_ERROR;
	return SQL_SUCCESS;
}

ODBC_FUNC(SQLConnect, (P(SQLHDBC,hdbc), PCHARIN(DSN,SQLSMALLINT), PCHARIN(UID,SQLSMALLINT),
	PCHARIN(AuthStr,SQLSMALLINT) WIDE))
{
	TDSLOGIN *login;

	ODBC_ENTER_HDBC;

#ifdef TDS_NO_DM
	if (szDSN && !IS_VALID_LEN(cbDSN)) {
		odbc_errs_add(&dbc->errs, "HY090", "Invalid DSN buffer length");
		ODBC_EXIT_(dbc);
	}

	if (szUID && !IS_VALID_LEN(cbUID)) {
		odbc_errs_add(&dbc->errs, "HY090", "Invalid UID buffer length");
		ODBC_EXIT_(dbc);
	}

	if (szAuthStr && !IS_VALID_LEN(cbAuthStr)) {
		odbc_errs_add(&dbc->errs, "HY090", "Invalid PWD buffer length");
		ODBC_EXIT_(dbc);
	}
#endif

	login = tds_alloc_login(0);
	if (!login || !tds_init_login(login, dbc->env->tds_ctx->locale)) {
		tds_free_login(login);
		odbc_errs_add(&dbc->errs, "HY001", NULL);
		ODBC_EXIT_(dbc);
	}

	/* data source name */
	if (odbc_get_string_size(cbDSN, szDSN _wide))
		odbc_dstr_copy(dbc, &dbc->dsn, cbDSN, szDSN);
	else
		tds_dstr_copy(&dbc->dsn, "DEFAULT");


	if (!odbc_get_dsn_info(&dbc->errs, tds_dstr_cstr(&dbc->dsn), login)) {
		tds_free_login(login);
		ODBC_EXIT_(dbc);
	}

	if (!tds_dstr_isempty(&dbc->attr.current_catalog))
		tds_dstr_dup(&login->database, &dbc->attr.current_catalog);

	/*
	 * username/password are never saved to ini file,
	 * so you do not check in ini file
	 */
	/* user id */
	if (odbc_get_string_size(cbUID, szUID _wide)) {
		if (!odbc_dstr_copy(dbc, &login->user_name, cbUID, szUID)) {
			tds_free_login(login);
			odbc_errs_add(&dbc->errs, "HY001", NULL);
			ODBC_EXIT_(dbc);
		}
	}

	/* password */
	if (szAuthStr && !tds_dstr_isempty(&login->user_name)) {
		if (!odbc_dstr_copy(dbc, &login->password, cbAuthStr, szAuthStr)) {
			tds_free_login(login);
			odbc_errs_add(&dbc->errs, "HY001", NULL);
			ODBC_EXIT_(dbc);
		}
	}

	/* DO IT */
	odbc_connect(dbc, login);

	tds_free_login(login);
	ODBC_EXIT_(dbc);
}

ODBC_FUNC(SQLDescribeCol, (P(SQLHSTMT,hstmt), P(SQLUSMALLINT,icol), PCHAROUT(ColName,SQLSMALLINT),
	P(SQLSMALLINT FAR *,pfSqlType), P(SQLULEN FAR *,pcbColDef),
	P(SQLSMALLINT FAR *,pibScale), P(SQLSMALLINT FAR *,pfNullable) WIDE))
{
	TDS_DESC *ird;
	struct _drecord *drec;

	ODBC_ENTER_HSTMT;

	ird = stmt->ird;
	IRD_UPDATE(ird, &stmt->errs, ODBC_EXIT(stmt, SQL_ERROR));

	if (icol <= 0 || icol > ird->header.sql_desc_count) {
		odbc_errs_add(&stmt->errs, "07009", "Column out of range");
		ODBC_EXIT_(stmt);
	}
	/* check name length */
	if (cbColNameMax < 0) {
		odbc_errs_add(&stmt->errs, "HY090", NULL);
		ODBC_EXIT_(stmt);
	}
	drec = &ird->records[icol - 1];

	/* cbColNameMax can be 0 (to retrieve name length) */
	if (szColName && cbColNameMax) {
		SQLRETURN result;

		/* straight copy column name up to cbColNameMax */
		result = odbc_set_string(stmt->dbc, szColName, cbColNameMax, pcbColName, tds_dstr_cstr(&drec->sql_desc_label), -1);
		if (result == SQL_SUCCESS_WITH_INFO)
			odbc_errs_add(&stmt->errs, "01004", NULL);
	}
	if (pfSqlType) {
		/* TODO sure ? check documentation for date and intervals */
		*pfSqlType = drec->sql_desc_concise_type;
	}

	if (pcbColDef) {
		if (drec->sql_desc_type == SQL_NUMERIC || drec->sql_desc_type == SQL_DECIMAL) {
			*pcbColDef = drec->sql_desc_precision;
		} else {
			*pcbColDef = drec->sql_desc_length;
		}
	}
	if (pibScale) {
		if (drec->sql_desc_type == SQL_NUMERIC || drec->sql_desc_type == SQL_DECIMAL
		    || drec->sql_desc_type == SQL_DATETIME || drec->sql_desc_type == SQL_FLOAT) {
			*pibScale = drec->sql_desc_scale;
		} else {
			/* TODO test setting desc directly, SQLDescribeCol return just descriptor data ?? */
			*pibScale = 0;
		}
	}
	if (pfNullable) {
		*pfNullable = drec->sql_desc_nullable;
	}
	ODBC_EXIT_(stmt);
}

static SQLRETURN
_SQLColAttribute(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType, SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax,
		 SQLSMALLINT FAR * pcbDesc, SQLLEN FAR * pfDesc _WIDE)
{
	TDS_DESC *ird;
	struct _drecord *drec;
	SQLRETURN result = SQL_SUCCESS;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "_SQLColAttribute(%p, %u, %u, %p, %d, %p, %p)\n", 
			hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

	ird = stmt->ird;

#define COUT(src) result = odbc_set_string_oct(stmt->dbc, rgbDesc, cbDescMax, pcbDesc, src ? src : "", -1);
#define SOUT(src) result = odbc_set_string_oct(stmt->dbc, rgbDesc, cbDescMax, pcbDesc, tds_dstr_cstr(&src), -1);

/* SQLColAttribute returns always attributes using SQLINTEGER */
#if ENABLE_EXTRA_CHECKS
#define IOUT(type, src) do { \
	/* trick warning if type wrong */ \
	type *p_test = &src; p_test = p_test; \
	*pfDesc = src; } while(0)
#else
#define IOUT(type, src) *pfDesc = src
#endif

	IRD_UPDATE(ird, &stmt->errs, ODBC_EXIT(stmt, SQL_ERROR));

	/* dont check column index for these */
	switch (fDescType) {
#if SQL_COLUMN_COUNT != SQL_DESC_COUNT
	case SQL_COLUMN_COUNT:
#endif
	case SQL_DESC_COUNT:
		IOUT(SQLSMALLINT, ird->header.sql_desc_count);
		ODBC_EXIT(stmt, SQL_SUCCESS);
		break;
	}

	if (!ird->header.sql_desc_count) {
		odbc_errs_add(&stmt->errs, "07005", NULL);
		ODBC_EXIT_(stmt);
	}

	if (icol <= 0 || icol > ird->header.sql_desc_count) {
		odbc_errs_add(&stmt->errs, "07009", "Column out of range");
		ODBC_EXIT_(stmt);
	}
	drec = &ird->records[icol - 1];

	tdsdump_log(TDS_DBG_INFO1, "SQLColAttribute: fDescType is %d\n", fDescType);

	switch (fDescType) {
	case SQL_DESC_AUTO_UNIQUE_VALUE:
		IOUT(SQLUINTEGER, drec->sql_desc_auto_unique_value);
		break;
	case SQL_DESC_BASE_COLUMN_NAME:
		SOUT(drec->sql_desc_base_column_name);
		break;
	case SQL_DESC_BASE_TABLE_NAME:
		SOUT(drec->sql_desc_base_table_name);
		break;
	case SQL_DESC_CASE_SENSITIVE:
		IOUT(SQLINTEGER, drec->sql_desc_case_sensitive);
		break;
	case SQL_DESC_CATALOG_NAME:
		SOUT(drec->sql_desc_catalog_name);
		break;
#if SQL_COLUMN_TYPE != SQL_DESC_CONCISE_TYPE
	case SQL_COLUMN_TYPE:
#endif
	case SQL_DESC_CONCISE_TYPE:
		/* special case, get ODBC 2 type, not ODBC 3 SQL_DESC_CONCISE_TYPE (different for datetime) */
		if (stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) {
			IOUT(SQLSMALLINT, drec->sql_desc_concise_type);
			break;
		}

		/* get type and convert it to ODBC 2 type */
		{
			SQLSMALLINT type = drec->sql_desc_concise_type;

			switch (type) {
			case SQL_TYPE_DATE:
				type = SQL_DATE;
				break;
			case SQL_TYPE_TIME:
				type = SQL_TIME;
				break;
			case SQL_TYPE_TIMESTAMP:
				type = SQL_TIMESTAMP;
				break;
			}
			IOUT(SQLSMALLINT, type);
		}
		break;
	case SQL_DESC_DISPLAY_SIZE:
		IOUT(SQLLEN, drec->sql_desc_display_size);
		break;
	case SQL_DESC_FIXED_PREC_SCALE:
		IOUT(SQLSMALLINT, drec->sql_desc_fixed_prec_scale);
		break;
	case SQL_DESC_LABEL:
		SOUT(drec->sql_desc_label);
		break;
		/* FIXME special cases for SQL_COLUMN_LENGTH */
	case SQL_COLUMN_LENGTH:
		IOUT(SQLLEN, drec->sql_desc_octet_length);
		break;
	case SQL_DESC_LENGTH:
		IOUT(SQLULEN, drec->sql_desc_length);
		break;
	case SQL_DESC_LITERAL_PREFIX:
		COUT(drec->sql_desc_literal_prefix);
		break;
	case SQL_DESC_LITERAL_SUFFIX:
		COUT(drec->sql_desc_literal_suffix);
		break;
	case SQL_DESC_LOCAL_TYPE_NAME:
		SOUT(drec->sql_desc_local_type_name);
		break;
#if SQL_COLUMN_NAME != SQL_DESC_NAME
	case SQL_COLUMN_NAME:
#endif
	case SQL_DESC_NAME:
		SOUT(drec->sql_desc_name);
		break;
#if SQL_COLUMN_NULLABLE != SQL_DESC_NULLABLE
	case SQL_COLUMN_NULLABLE:
#endif
	case SQL_DESC_NULLABLE:
		IOUT(SQLSMALLINT, drec->sql_desc_nullable);
		break;
	case SQL_DESC_NUM_PREC_RADIX:
		IOUT(SQLINTEGER, drec->sql_desc_num_prec_radix);
		break;
	case SQL_DESC_OCTET_LENGTH:
		IOUT(SQLLEN, drec->sql_desc_octet_length);
		break;
		/* FIXME special cases for SQL_COLUMN_PRECISION */
	case SQL_COLUMN_PRECISION:
		if (drec->sql_desc_concise_type == SQL_REAL) {
			*pfDesc = 7;
			break;
		}
		if (drec->sql_desc_concise_type == SQL_DOUBLE) {
			*pfDesc = 15;
			break;
		}
		if (drec->sql_desc_concise_type == SQL_TYPE_TIMESTAMP || drec->sql_desc_concise_type == SQL_TIMESTAMP) {
			*pfDesc = drec->sql_desc_precision ? 23 : 16;
			break;
		}
	case SQL_DESC_PRECISION:	/* this section may be wrong */
		if (drec->sql_desc_concise_type == SQL_NUMERIC || drec->sql_desc_concise_type == SQL_DECIMAL
		    || drec->sql_desc_concise_type == SQL_TYPE_TIMESTAMP
		    || drec->sql_desc_concise_type == SQL_TYPE_DATE
		    || drec->sql_desc_concise_type == SQL_TIMESTAMP
		    || drec->sql_desc_concise_type == SQL_SS_TIME2
		    || drec->sql_desc_concise_type == SQL_SS_TIMESTAMPOFFSET)
			IOUT(SQLSMALLINT, drec->sql_desc_precision);
		else
			*pfDesc = drec->sql_desc_length;
		break;
		/* FIXME special cases for SQL_COLUMN_SCALE */
	case SQL_COLUMN_SCALE:
	case SQL_DESC_SCALE:	/* this section may be wrong */
		if (drec->sql_desc_concise_type == SQL_NUMERIC || drec->sql_desc_concise_type == SQL_DECIMAL
		    || drec->sql_desc_concise_type == SQL_TYPE_TIMESTAMP
		    || drec->sql_desc_concise_type == SQL_TIMESTAMP
		    || drec->sql_desc_concise_type == SQL_FLOAT
		    || drec->sql_desc_concise_type == SQL_SS_TIME2
		    || drec->sql_desc_concise_type == SQL_SS_TIMESTAMPOFFSET)
			IOUT(SQLSMALLINT, drec->sql_desc_scale);
		else
			*pfDesc = 0;
		break;
	case SQL_DESC_SCHEMA_NAME:
		SOUT(drec->sql_desc_schema_name);
		break;
	case SQL_DESC_SEARCHABLE:
		IOUT(SQLSMALLINT, drec->sql_desc_searchable);
		break;
	case SQL_DESC_TABLE_NAME:
		SOUT(drec->sql_desc_table_name);
		break;
	case SQL_DESC_TYPE:
		IOUT(SQLSMALLINT, drec->sql_desc_type);
		break;
	case SQL_DESC_TYPE_NAME:
		COUT(drec->sql_desc_type_name);
		break;
	case SQL_DESC_UNNAMED:
		IOUT(SQLSMALLINT, drec->sql_desc_unnamed);
		break;
	case SQL_DESC_UNSIGNED:
		IOUT(SQLSMALLINT, drec->sql_desc_unsigned);
		break;
	case SQL_DESC_UPDATABLE:
		IOUT(SQLSMALLINT, drec->sql_desc_updatable);
		break;
	default:
		tdsdump_log(TDS_DBG_INFO2, "SQLColAttribute: fDescType %d not catered for...\n", fDescType);
		odbc_errs_add(&stmt->errs, "HY091", NULL);
		ODBC_EXIT_(stmt);
		break;
	}

	if (result == SQL_SUCCESS_WITH_INFO)
		odbc_errs_add(&stmt->errs, "01004", NULL);

	ODBC_EXIT(stmt, result);

#undef COUT
#undef SOUT
#undef IOUT
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLColAttributes(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
		 SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT FAR * pcbDesc, SQLLEN FAR * pfDesc)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLColAttributes(%p, %d, %d, %p, %d, %p, %p)\n", 
			hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

	return _SQLColAttribute(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc _wide0);
}

#if (ODBCVER >= 0x0300)
SQLRETURN ODBC_PUBLIC ODBC_API
SQLColAttribute(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
		SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT FAR * pcbDesc,
#ifdef TDS_SQLCOLATTRIBUTE_SQLLEN
		SQLLEN FAR * pfDesc
#else
		SQLPOINTER pfDesc
#endif
	)
{

	return _SQLColAttribute(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc _wide0);
}

#ifdef ENABLE_ODBC_WIDE
SQLRETURN ODBC_PUBLIC ODBC_API
SQLColAttributeW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLUSMALLINT fDescType,
		SQLPOINTER rgbDesc, SQLSMALLINT cbDescMax, SQLSMALLINT FAR * pcbDesc,
#ifdef TDS_SQLCOLATTRIBUTE_SQLLEN
		SQLLEN FAR * pfDesc
#else
		SQLPOINTER pfDesc
#endif
	)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLColAttributeW(%p, %u, %u, %p, %d, %p, %p)\n",
			hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc);

	return _SQLColAttribute(hstmt, icol, fDescType, rgbDesc, cbDescMax, pcbDesc, pfDesc, 1);
}
#endif
#endif

SQLRETURN ODBC_PUBLIC ODBC_API
SQLDisconnect(SQLHDBC hdbc)
{
	int i;

	ODBC_ENTER_HDBC;

	tdsdump_log(TDS_DBG_FUNC, "SQLDisconnect(%p)\n", hdbc);

	/* free all associated statements */
	while (dbc->stmt_list) {
		tds_mutex_unlock(&dbc->mtx);
		_SQLFreeStmt(dbc->stmt_list, SQL_DROP, 1);
		tds_mutex_lock(&dbc->mtx);
	}

	/* free all associated descriptors */
	for (i = 0; i < TDS_MAX_APP_DESC; ++i) {
		if (dbc->uad[i]) {
			desc_free(dbc->uad[i]);
			dbc->uad[i] = NULL;
		}
	}

#ifdef ENABLE_ODBC_WIDE
	dbc->mb_conv = NULL;
#endif
	tds_free_socket(dbc->tds_socket);
	dbc->tds_socket = NULL;
	dbc->cursor_support = 0;

	ODBC_EXIT_(dbc);
}

static int
odbc_errmsg_handler(const TDSCONTEXT * ctx, TDSSOCKET * tds, TDSMESSAGE * msg)
{
	struct _sql_errors *errs = NULL;
	TDS_DBC *dbc = NULL;
	TDS_STMT *stmt = NULL;

	tdsdump_log(TDS_DBG_INFO1, "msgno %d %d\n", (int) msg->msgno, TDSETIME);

	if (msg->msgno == TDSETIME) {

		tdsdump_log(TDS_DBG_INFO1, "in timeout\n");
		if (!tds)
			return TDS_INT_CANCEL;

		if ((stmt = odbc_get_stmt(tds)) != NULL) {
			/* first time, try to send a cancel */
			if (!tds->in_cancel) {
				odbc_errs_add(&stmt->errs, "HYT00", "Timeout expired");
				tdsdump_log(TDS_DBG_INFO1, "returning from timeout\n");
				return TDS_INT_TIMEOUT;
			}
		} else if ((dbc = odbc_get_dbc(tds)) != NULL) {
			odbc_errs_add(&dbc->errs, "HYT00", "Timeout expired");
		}

		tds_close_socket(tds);
		tdsdump_log(TDS_DBG_INFO1, "returning cancel from timeout\n");
		return TDS_INT_CANCEL;
	}

	if (tds && (dbc = odbc_get_dbc(tds)) != NULL) {
		errs = &dbc->errs;
		stmt = odbc_get_stmt(tds);
		if (stmt)
			errs = &stmt->errs;
		/* set server info if not setted in dbc */
		if (msg->server && tds_dstr_isempty(&dbc->server))
			tds_dstr_copy(&dbc->server, msg->server);
	} else if (ctx->parent) {
		errs = &((TDS_ENV *) ctx->parent)->errs;
	}
	if (errs) {
		int severity = msg->severity;
		const char * state = msg->sql_state;

		/* fix severity for Sybase */
		if (severity <= 10 && dbc && !TDS_IS_MSSQL(dbc->tds_socket) && msg->sql_state && msg->sql_state[0]
		    && strncmp(msg->sql_state, "00", 2) != 0) {
			if (strncmp(msg->sql_state, "01", 2) != 0 && strncmp(msg->sql_state, "IM", 2) != 0)
				severity = 11;
		}

		/* compute state if not available */
		if (!state)
			state = severity <= 10 ? "01000" : "42000";
		/* add error, do not overwrite connection timeout error */
		if (msg->msgno != TDSEFCON || errs->lastrc != SQL_ERROR || errs->num_errors < 1)
			odbc_errs_add_rdbms(errs, msg->msgno, state, msg->message, msg->line_number, msg->severity,
					    msg->server, stmt ? stmt->curr_param_row + 1 : 0);

		/* set lastc according */
		if (severity <= 10) {
			if (errs->lastrc == SQL_SUCCESS)
				errs->lastrc = SQL_SUCCESS_WITH_INFO;
		} else {
			errs->lastrc = SQL_ERROR;
		}
	}
	return TDS_INT_CANCEL;
}

/* TODO optimize, change only if some data change (set same value should not set this flag) */
#define DESC_SET_NEED_REPREPARE \
	do {\
		if (desc->type == DESC_IPD) {\
			assert(IS_HSTMT(desc->parent));\
			((TDS_STMT *) desc->parent)->need_reprepare = 1;\
		 }\
	} while(0)

SQLRETURN ODBC_PUBLIC ODBC_API
SQLSetDescRec(SQLHDESC hdesc, SQLSMALLINT nRecordNumber, SQLSMALLINT nType, SQLSMALLINT nSubType, SQLLEN nLength,
	      SQLSMALLINT nPrecision, SQLSMALLINT nScale, SQLPOINTER pData, SQLLEN FAR * pnStringLength, SQLLEN FAR * pnIndicator)
{
	struct _drecord *drec;
	SQLSMALLINT concise_type;

	ODBC_ENTER_HDESC;

	tdsdump_log(TDS_DBG_FUNC, "SQLSetDescRec(%p, %d, %d, %d, %d, %d, %d, %p, %p, %p)\n", 
			hdesc, nRecordNumber, nType, nSubType, (int)nLength, nPrecision, nScale, pData, pnStringLength, pnIndicator);

	if (desc->type == DESC_IRD) {
		odbc_errs_add(&desc->errs, "HY016", NULL);
		ODBC_EXIT_(desc);
	}

	if (nRecordNumber > desc->header.sql_desc_count || nRecordNumber <= 0) {
		odbc_errs_add(&desc->errs, "07009", NULL);
		ODBC_EXIT_(desc);
	}

	drec = &desc->records[nRecordNumber - 1];

	/* check for valid types and return "HY021" if not */
	if (desc->type == DESC_IPD) {
		DESC_SET_NEED_REPREPARE;
		concise_type = odbc_get_concise_sql_type(nType, nSubType);
	} else {
		concise_type = odbc_get_concise_c_type(nType, nSubType);
	}
	if (nType == SQL_INTERVAL || nType == SQL_DATETIME) {
		if (!concise_type) {
			odbc_errs_add(&desc->errs, "HY021", NULL);
			ODBC_EXIT_(desc);
		}
	} else {
		if (concise_type != nType) {
			odbc_errs_add(&desc->errs, "HY021", NULL);
			ODBC_EXIT_(desc);
		}
		nSubType = 0;
	}
	drec->sql_desc_concise_type = concise_type;
	drec->sql_desc_type = nType;
	drec->sql_desc_datetime_interval_code = nSubType;

	drec->sql_desc_octet_length = nLength;
	drec->sql_desc_precision = nPrecision;
	drec->sql_desc_scale = nScale;
	drec->sql_desc_data_ptr = pData;
	drec->sql_desc_octet_length_ptr = pnStringLength;
	drec->sql_desc_indicator_ptr = pnIndicator;

	ODBC_EXIT_(desc);
}

ODBC_FUNC(SQLGetDescRec, (P(SQLHDESC,hdesc), P(SQLSMALLINT,RecordNumber), PCHAROUT(Name,SQLSMALLINT),
	P(SQLSMALLINT *,Type), P(SQLSMALLINT *,SubType), P(SQLLEN FAR *,Length),
	P(SQLSMALLINT *,Precision), P(SQLSMALLINT *,Scale), P(SQLSMALLINT *,Nullable) WIDE))
{
	struct _drecord *drec = NULL;
	SQLRETURN rc = SQL_SUCCESS;

	ODBC_ENTER_HDESC;

	if (RecordNumber <= 0) {
		odbc_errs_add(&desc->errs, "07009", NULL);
		ODBC_EXIT_(desc);
	}

	IRD_UPDATE(desc, &desc->errs, ODBC_EXIT(desc, SQL_ERROR));
	if (RecordNumber > desc->header.sql_desc_count)
		ODBC_EXIT(desc, SQL_NO_DATA);

	if (desc->type == DESC_IRD && !desc->header.sql_desc_count) {
		odbc_errs_add(&desc->errs, "HY007", NULL);
		ODBC_EXIT_(desc);
	}

	drec = &desc->records[RecordNumber - 1];

	if ((rc = odbc_set_string(desc_get_dbc(desc), szName, cbNameMax, pcbName, tds_dstr_cstr(&drec->sql_desc_name), -1)) != SQL_SUCCESS)
		odbc_errs_add(&desc->errs, "01004", NULL);

	if (Type)
		*Type = drec->sql_desc_type;
	if (Length)
		*Length = drec->sql_desc_octet_length;
	if (Precision)
		*Precision = drec->sql_desc_precision;
	if (Scale)
		*Scale = drec->sql_desc_scale;
	if (SubType)
		*SubType = drec->sql_desc_datetime_interval_code;
	if (Nullable)
		*Nullable = drec->sql_desc_nullable;

	ODBC_EXIT(desc, rc);
}

ODBC_FUNC(SQLGetDescField, (P(SQLHDESC,hdesc), P(SQLSMALLINT,icol), P(SQLSMALLINT,fDescType), P(SQLPOINTER,Value),
	P(SQLINTEGER,BufferLength), P(SQLINTEGER *,StringLength) WIDE))
{
	struct _drecord *drec;
	SQLRETURN result = SQL_SUCCESS;

	ODBC_ENTER_HDESC;

#define COUT(src) result = odbc_set_string_oct(desc_get_dbc(desc), Value, BufferLength, StringLength, src, -1);
#define SOUT(src) result = odbc_set_string_oct(desc_get_dbc(desc), Value, BufferLength, StringLength, tds_dstr_cstr(&src), -1);

#if ENABLE_EXTRA_CHECKS
#define IOUT(type, src) do { \
	/* trick warning if type wrong */ \
	type *p_test = &src; p_test = p_test; \
	*((type *)Value) = src; } while(0)
#else
#define IOUT(type, src) *((type *)Value) = src
#endif

	/* dont check column index for these */
	switch (fDescType) {
	case SQL_DESC_ALLOC_TYPE:
		IOUT(SQLSMALLINT, desc->header.sql_desc_alloc_type);
		ODBC_EXIT_(desc);
		break;
	case SQL_DESC_ARRAY_SIZE:
		IOUT(SQLULEN, desc->header.sql_desc_array_size);
		ODBC_EXIT_(desc);
		break;
	case SQL_DESC_ARRAY_STATUS_PTR:
		IOUT(SQLUSMALLINT *, desc->header.sql_desc_array_status_ptr);
		ODBC_EXIT_(desc);
		break;
	case SQL_DESC_BIND_OFFSET_PTR:
		IOUT(SQLLEN *, desc->header.sql_desc_bind_offset_ptr);
		ODBC_EXIT_(desc);
		break;
	case SQL_DESC_BIND_TYPE:
		IOUT(SQLINTEGER, desc->header.sql_desc_bind_type);
		ODBC_EXIT_(desc);
		break;
	case SQL_DESC_COUNT:
		IRD_UPDATE(desc, &desc->errs, ODBC_EXIT(desc, SQL_ERROR));
		IOUT(SQLSMALLINT, desc->header.sql_desc_count);
		ODBC_EXIT_(desc);
		break;
	case SQL_DESC_ROWS_PROCESSED_PTR:
		IOUT(SQLULEN *, desc->header.sql_desc_rows_processed_ptr);
		ODBC_EXIT_(desc);
		break;
	}

	IRD_UPDATE(desc, &desc->errs, ODBC_EXIT(desc, SQL_ERROR));
	if (!desc->header.sql_desc_count) {
		odbc_errs_add(&desc->errs, "07005", NULL);
		ODBC_EXIT_(desc);
	}

	if (icol < 1) {
		odbc_errs_add(&desc->errs, "07009", "Column out of range");
		ODBC_EXIT_(desc);
	}
	if (icol > desc->header.sql_desc_count)
		ODBC_EXIT(desc, SQL_NO_DATA);
	drec = &desc->records[icol - 1];

	tdsdump_log(TDS_DBG_INFO1, "SQLGetDescField: fDescType is %d\n", fDescType);

	switch (fDescType) {
	case SQL_DESC_AUTO_UNIQUE_VALUE:
		IOUT(SQLUINTEGER, drec->sql_desc_auto_unique_value);
		break;
	case SQL_DESC_BASE_COLUMN_NAME:
		SOUT(drec->sql_desc_base_column_name);
		break;
	case SQL_DESC_BASE_TABLE_NAME:
		SOUT(drec->sql_desc_base_table_name);
		break;
	case SQL_DESC_CASE_SENSITIVE:
		IOUT(SQLINTEGER, drec->sql_desc_case_sensitive);
		break;
	case SQL_DESC_CATALOG_NAME:
		SOUT(drec->sql_desc_catalog_name);
		break;
	case SQL_DESC_CONCISE_TYPE:
		IOUT(SQLSMALLINT, drec->sql_desc_concise_type);
		break;
	case SQL_DESC_DATA_PTR:
		IOUT(SQLPOINTER, drec->sql_desc_data_ptr);
		break;
	case SQL_DESC_DATETIME_INTERVAL_CODE:
		IOUT(SQLSMALLINT, drec->sql_desc_datetime_interval_code);
		break;
	case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		IOUT(SQLINTEGER, drec->sql_desc_datetime_interval_precision);
		break;
	case SQL_DESC_DISPLAY_SIZE:
		IOUT(SQLLEN, drec->sql_desc_display_size);
		break;
	case SQL_DESC_FIXED_PREC_SCALE:
		IOUT(SQLSMALLINT, drec->sql_desc_fixed_prec_scale);
		break;
	case SQL_DESC_INDICATOR_PTR:
		IOUT(SQLLEN *, drec->sql_desc_indicator_ptr);
		break;
	case SQL_DESC_LABEL:
		SOUT(drec->sql_desc_label);
		break;
	case SQL_DESC_LENGTH:
		IOUT(SQLULEN, drec->sql_desc_length);
		break;
	case SQL_DESC_LITERAL_PREFIX:
		COUT(drec->sql_desc_literal_prefix);
		break;
	case SQL_DESC_LITERAL_SUFFIX:
		COUT(drec->sql_desc_literal_suffix);
		break;
	case SQL_DESC_LOCAL_TYPE_NAME:
		SOUT(drec->sql_desc_local_type_name);
		break;
	case SQL_DESC_NAME:
		SOUT(drec->sql_desc_name);
		break;
	case SQL_DESC_NULLABLE:
		IOUT(SQLSMALLINT, drec->sql_desc_nullable);
		break;
	case SQL_DESC_NUM_PREC_RADIX:
		IOUT(SQLINTEGER, drec->sql_desc_num_prec_radix);
		break;
	case SQL_DESC_OCTET_LENGTH:
		IOUT(SQLLEN, drec->sql_desc_octet_length);
		break;
	case SQL_DESC_OCTET_LENGTH_PTR:
		IOUT(SQLLEN *, drec->sql_desc_octet_length_ptr);
		break;
	case SQL_DESC_PARAMETER_TYPE:
		IOUT(SQLSMALLINT, drec->sql_desc_parameter_type);
		break;
	case SQL_DESC_PRECISION:
		if (drec->sql_desc_concise_type == SQL_NUMERIC || drec->sql_desc_concise_type == SQL_DECIMAL
		    || drec->sql_desc_concise_type == SQL_TIMESTAMP
		    || drec->sql_desc_concise_type == SQL_TYPE_TIMESTAMP)
			IOUT(SQLSMALLINT, drec->sql_desc_precision);
		else
			/* TODO support date/time */
			*((SQLSMALLINT *) Value) = 0;
		break;
#ifdef SQL_DESC_ROWVER
	case SQL_DESC_ROWVER:
		IOUT(SQLSMALLINT, drec->sql_desc_rowver);
		break;
#endif
	case SQL_DESC_SCALE:
		if (drec->sql_desc_concise_type == SQL_NUMERIC || drec->sql_desc_concise_type == SQL_DECIMAL
		    || drec->sql_desc_concise_type == SQL_TYPE_TIMESTAMP
		    || drec->sql_desc_concise_type == SQL_TIMESTAMP
		    || drec->sql_desc_concise_type == SQL_FLOAT)
			IOUT(SQLSMALLINT, drec->sql_desc_scale);
		else
			*((SQLSMALLINT *) Value) = 0;
		break;
	case SQL_DESC_SCHEMA_NAME:
		SOUT(drec->sql_desc_schema_name);
		break;
	case SQL_DESC_SEARCHABLE:
		IOUT(SQLSMALLINT, drec->sql_desc_searchable);
		break;
	case SQL_DESC_TABLE_NAME:
		SOUT(drec->sql_desc_table_name);
		break;
	case SQL_DESC_TYPE:
		IOUT(SQLSMALLINT, drec->sql_desc_type);
		break;
	case SQL_DESC_TYPE_NAME:
		COUT(drec->sql_desc_type_name);
		break;
	case SQL_DESC_UNNAMED:
		IOUT(SQLSMALLINT, drec->sql_desc_unnamed);
		break;
	case SQL_DESC_UNSIGNED:
		IOUT(SQLSMALLINT, drec->sql_desc_unsigned);
		break;
	case SQL_DESC_UPDATABLE:
		IOUT(SQLSMALLINT, drec->sql_desc_updatable);
		break;
	default:
		odbc_errs_add(&desc->errs, "HY091", NULL);
		ODBC_EXIT_(desc);
		break;
	}

	if (result == SQL_SUCCESS_WITH_INFO)
		odbc_errs_add(&desc->errs, "01004", NULL);

	ODBC_EXIT(desc, result);

#undef COUT
#undef SOUT
#undef IOUT
}

ODBC_FUNC(SQLSetDescField, (P(SQLHDESC,hdesc), P(SQLSMALLINT,icol), P(SQLSMALLINT,fDescType),
	P(SQLPOINTER,Value), P(SQLINTEGER,BufferLength) WIDE))
{
	struct _drecord *drec;
	SQLRETURN result = SQL_SUCCESS;

	ODBC_ENTER_HDESC;

#if ENABLE_EXTRA_CHECKS
#define IIN(type, dest) do { \
	/* trick warning if type wrong */ \
	type *p_test = &dest; p_test = p_test; \
	dest = (type)(TDS_INTPTR)Value; } while(0)
#define PIN(type, dest) do { \
	/* trick warning if type wrong */ \
	type *p_test = &dest; p_test = p_test; \
	dest = (type)Value; } while(0)
#else
#define IIN(type, dest) dest = (type)(TDS_INTPTR)Value
#define PIN(type, dest) dest = (type)Value
#endif

	/* special case for IRD */
	if (desc->type == DESC_IRD && fDescType != SQL_DESC_ARRAY_STATUS_PTR && fDescType != SQL_DESC_ROWS_PROCESSED_PTR) {
		odbc_errs_add(&desc->errs, "HY016", NULL);
		ODBC_EXIT_(desc);
	}

	/* dont check column index for these */
	switch (fDescType) {
	case SQL_DESC_ALLOC_TYPE:
		odbc_errs_add(&desc->errs, "HY091", "Descriptor type read only");
		ODBC_EXIT_(desc);
		break;
	case SQL_DESC_ARRAY_SIZE:
		IIN(SQLULEN, desc->header.sql_desc_array_size);
		ODBC_EXIT_(desc);
		break;
	case SQL_DESC_ARRAY_STATUS_PTR:
		PIN(SQLUSMALLINT *, desc->header.sql_desc_array_status_ptr);
		ODBC_EXIT_(desc);
		break;
	case SQL_DESC_ROWS_PROCESSED_PTR:
		PIN(SQLULEN *, desc->header.sql_desc_rows_processed_ptr);
		ODBC_EXIT_(desc);
		break;
	case SQL_DESC_BIND_TYPE:
		IIN(SQLINTEGER, desc->header.sql_desc_bind_type);
		ODBC_EXIT_(desc);
		break;
	case SQL_DESC_COUNT:
		{
			int n = (int) (TDS_INTPTR) Value;

			if (n <= 0 || n > 4000) {
				odbc_errs_add(&desc->errs, "07009", NULL);
				ODBC_EXIT_(desc);
			}
			result = desc_alloc_records(desc, n);
			if (result == SQL_ERROR)
				odbc_errs_add(&desc->errs, "HY001", NULL);
			ODBC_EXIT(desc, result);
		}
		break;
	}

	if (!desc->header.sql_desc_count) {
		odbc_errs_add(&desc->errs, "07005", NULL);
		ODBC_EXIT_(desc);
	}

	if (icol <= 0 || icol > desc->header.sql_desc_count) {
		odbc_errs_add(&desc->errs, "07009", "Column out of range");
		ODBC_EXIT_(desc);
	}
	drec = &desc->records[icol - 1];

	tdsdump_log(TDS_DBG_INFO1, "SQLColAttributes: fDescType is %d\n", fDescType);

	switch (fDescType) {
	case SQL_DESC_AUTO_UNIQUE_VALUE:
	case SQL_DESC_BASE_COLUMN_NAME:
	case SQL_DESC_BASE_TABLE_NAME:
	case SQL_DESC_CASE_SENSITIVE:
	case SQL_DESC_CATALOG_NAME:
		odbc_errs_add(&desc->errs, "HY091", "Descriptor type read only");
		result = SQL_ERROR;
		break;
	case SQL_DESC_CONCISE_TYPE:
		DESC_SET_NEED_REPREPARE;
		if (desc->type == DESC_IPD)
			result = odbc_set_concise_sql_type((SQLSMALLINT) (TDS_INTPTR) Value, drec, 0);
		else
			result = odbc_set_concise_c_type((SQLSMALLINT) (TDS_INTPTR) Value, drec, 0);
		if (result != SQL_SUCCESS)
			odbc_errs_add(&desc->errs, "HY021", NULL);
		break;
	case SQL_DESC_DATA_PTR:
		PIN(SQLPOINTER, drec->sql_desc_data_ptr);
		break;
		/* TODO SQL_DESC_DATETIME_INTERVAL_CODE remember to check sql_desc_type */
		/* TODO SQL_DESC_DATETIME_INTERVAL_PRECISION */
	case SQL_DESC_DISPLAY_SIZE:
	case SQL_DESC_FIXED_PREC_SCALE:
		odbc_errs_add(&desc->errs, "HY091", "Descriptor type read only");
		result = SQL_ERROR;
		break;
	case SQL_DESC_INDICATOR_PTR:
		PIN(SQLLEN *, drec->sql_desc_indicator_ptr);
		break;
	case SQL_DESC_LABEL:
		odbc_errs_add(&desc->errs, "HY091", "Descriptor type read only");
		result = SQL_ERROR;
		break;
	case SQL_DESC_LENGTH:
		DESC_SET_NEED_REPREPARE;
		IIN(SQLULEN, drec->sql_desc_length);
		break;
	case SQL_DESC_LITERAL_PREFIX:
	case SQL_DESC_LITERAL_SUFFIX:
	case SQL_DESC_LOCAL_TYPE_NAME:
		odbc_errs_add(&desc->errs, "HY091", "Descriptor type read only");
		result = SQL_ERROR;
		break;
	case SQL_DESC_NAME:
		if (!odbc_dstr_copy_oct(desc_get_dbc(desc), &drec->sql_desc_name, BufferLength, (ODBC_CHAR*) Value)) {
			odbc_errs_add(&desc->errs, "HY001", NULL);
			result = SQL_ERROR;
		}
		break;
	case SQL_DESC_NULLABLE:
		odbc_errs_add(&desc->errs, "HY091", "Descriptor type read only");
		result = SQL_ERROR;
		break;
	case SQL_DESC_NUM_PREC_RADIX:
		IIN(SQLINTEGER, drec->sql_desc_num_prec_radix);
		break;
	case SQL_DESC_OCTET_LENGTH:
		DESC_SET_NEED_REPREPARE;
		IIN(SQLLEN, drec->sql_desc_octet_length);
		break;
	case SQL_DESC_OCTET_LENGTH_PTR:
		PIN(SQLLEN *, drec->sql_desc_octet_length_ptr);
		break;
	case SQL_DESC_PARAMETER_TYPE:
		DESC_SET_NEED_REPREPARE;
		IIN(SQLSMALLINT, drec->sql_desc_parameter_type);
		break;
	case SQL_DESC_PRECISION:
		DESC_SET_NEED_REPREPARE;
		/* TODO correct ?? */
		if (drec->sql_desc_concise_type == SQL_NUMERIC || drec->sql_desc_concise_type == SQL_DECIMAL)
			IIN(SQLSMALLINT, drec->sql_desc_precision);
		else
			IIN(SQLULEN, drec->sql_desc_length);
		break;
#ifdef SQL_DESC_ROWVER
	case SQL_DESC_ROWVER:
		odbc_errs_add(&desc->errs, "HY091", "Descriptor type read only");
		result = SQL_ERROR;
		break;
#endif
	case SQL_DESC_SCALE:
		DESC_SET_NEED_REPREPARE;
		if (drec->sql_desc_concise_type == SQL_NUMERIC || drec->sql_desc_concise_type == SQL_DECIMAL)
			IIN(SQLSMALLINT, drec->sql_desc_scale);
		else
			/* TODO even for datetime/money ?? */
			drec->sql_desc_scale = 0;
		break;
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_SEARCHABLE:
	case SQL_DESC_TABLE_NAME:
		odbc_errs_add(&desc->errs, "HY091", "Descriptor type read only");
		result = SQL_ERROR;
		break;
	case SQL_DESC_TYPE:
		DESC_SET_NEED_REPREPARE;
		IIN(SQLSMALLINT, drec->sql_desc_type);
		/* FIXME what happen for interval/datetime ?? */
		drec->sql_desc_concise_type = drec->sql_desc_type;
		break;
	case SQL_DESC_TYPE_NAME:
		odbc_errs_add(&desc->errs, "HY091", "Descriptor type read only");
		result = SQL_ERROR;
		break;
	case SQL_DESC_UNNAMED:
		IIN(SQLSMALLINT, drec->sql_desc_unnamed);
		break;
	case SQL_DESC_UNSIGNED:
	case SQL_DESC_UPDATABLE:
		odbc_errs_add(&desc->errs, "HY091", "Descriptor type read only");
		result = SQL_ERROR;
		break;
	default:
		odbc_errs_add(&desc->errs, "HY091", NULL);
		ODBC_EXIT_(desc);
		break;
	}

#undef IIN

	ODBC_EXIT(desc, result);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLCopyDesc(SQLHDESC hsrc, SQLHDESC hdesc)
{
	TDS_DESC *src;

	ODBC_ENTER_HDESC;

	tdsdump_log(TDS_DBG_FUNC, "SQLCopyDesc(%p, %p)\n", 
			hsrc, hdesc);

	if (SQL_NULL_HDESC == hsrc || !IS_HDESC(hsrc))
		return SQL_INVALID_HANDLE;
	src = (TDS_DESC *) hsrc;
	CHECK_DESC_EXTRA(src);

	/* do not write on IRD */
	if (desc->type == DESC_IRD) {
		odbc_errs_add(&desc->errs, "HY016", NULL);
		ODBC_EXIT_(desc);
	}
	IRD_UPDATE(src, &src->errs, ODBC_EXIT(desc, SQL_ERROR));

	ODBC_EXIT(desc, desc_copy(desc, src));
}

#if ENABLE_EXTRA_CHECKS
static void
odbc_ird_check(TDS_STMT * stmt)
{
#if !ENABLE_ODBC_MARS
	TDS_DESC *ird = stmt->ird;
	TDSRESULTINFO *res_info = NULL;
	int cols = 0, i;

	if (!stmt->tds)
		return;
	if (stmt->tds->current_results) {
		res_info = stmt->tds->current_results;
		cols = res_info->num_cols;
	}
	if (stmt->cursor != NULL)
		return;

	/* check columns number */
	assert(ird->header.sql_desc_count <= cols || ird->header.sql_desc_count == 0);


	/* check all columns */
	for (i = 0; i < ird->header.sql_desc_count; ++i) {
		struct _drecord *drec = &ird->records[i];
		TDSCOLUMN *col = res_info->columns[i];

		assert(tds_dstr_len(&drec->sql_desc_label) == col->column_namelen);
		assert(memcmp(tds_dstr_cstr(&drec->sql_desc_label), col->column_name, col->column_namelen) == 0);
	}
#endif
}
#endif

static void
odbc_unquote(char *buf, size_t buf_len, const char *start, const char *end)
{
	char quote;
	assert(buf_len > 0);

	/* empty string */
	if (start >= end) {
		buf[0] = 0;
		return;
	}

	/* not quoted */
	if (*start != '[' && *start != '\"') {
		--buf_len;
		if (end - start < buf_len)
			buf_len = end - start;
		memcpy(buf, start, buf_len);
		buf[buf_len] = 0;
		return;
	}

	/* quoted... unquote */
	quote = (*start == '[') ? ']' : *start;
	++start;
	while (buf_len > 0 && start < end) {
		if (*start == quote)
			if (++start >= end)
				break;
		*buf++ = *start++;
		--buf_len;
	}
	*buf = 0;
}

/* FIXME check result !!! */
static SQLRETURN
odbc_populate_ird(TDS_STMT * stmt)
{
	TDS_DESC *ird = stmt->ird;
	struct _drecord *drec;
	TDSCOLUMN *col;
	TDSRESULTINFO *res_info;
	int num_cols;
	int i;

	desc_free_records(ird);
	if (!stmt->tds || !(res_info = stmt->tds->current_results))
		return SQL_SUCCESS;
	if (res_info == stmt->tds->param_info)
		return SQL_SUCCESS;
	num_cols = res_info->num_cols;

	/* ignore hidden columns... TODO correct? */
	while (num_cols > 0 && res_info->columns[num_cols - 1]->column_hidden == 1)
		--num_cols;

	if (desc_alloc_records(ird, num_cols) != SQL_SUCCESS) {
		odbc_errs_add(&stmt->errs, "HY001", NULL);
		return SQL_ERROR;
	}

	for (i = 0; i < num_cols; i++) {
		int type;

		drec = &ird->records[i];
		col = res_info->columns[i];
		type = tds_get_conversion_type(col->column_type, col->column_size);
		drec->sql_desc_auto_unique_value = col->column_identity ? SQL_TRUE : SQL_FALSE;
		/* TODO SQL_FALSE ?? */
		drec->sql_desc_case_sensitive = SQL_TRUE;
		/* TODO test error ?? */
		/* TODO handle unsigned flags ! */
		odbc_set_concise_sql_type(odbc_server_to_sql_type(col->on_server.column_type, col->on_server.column_size), drec, 0);
		/*
		 * TODO how to handle when in datetime we change precision ?? 
		 * should we change display size too ??
		 * is formatting function correct ??
		 * we should not convert to string with invalid precision!
		 */
		drec->sql_desc_display_size =
			odbc_sql_to_displaysize(drec->sql_desc_concise_type, col);
		drec->sql_desc_fixed_prec_scale = (col->column_prec && col->column_scale) ? SQL_TRUE : SQL_FALSE;
		if (!tds_dstr_copyn(&drec->sql_desc_label, col->column_name, col->column_namelen))
			return SQL_ERROR;

		odbc_set_sql_type_info(col, drec, stmt->dbc->env->attr.odbc_version);

		if (tds_dstr_isempty(&col->table_column_name)) {
			if (!tds_dstr_copyn(&drec->sql_desc_name, col->column_name, col->column_namelen))
				return SQL_ERROR;
		} else {
			if (!tds_dstr_dup(&drec->sql_desc_name, &col->table_column_name))
				return SQL_ERROR;
			if (!tds_dstr_dup(&drec->sql_desc_base_column_name, &col->table_column_name))
				return SQL_ERROR;
		}

		/* extract sql_desc_(catalog/schema/base_table)_name */
		/* TODO extract them dinamically (when needed) ? */
		/* TODO store in libTDS in different way (separately) ? */
		if (!tds_dstr_isempty(&col->table_name)) {
			struct {
				const char *start;
				const char *end;
			} partials[4];
			const char *p;
			char buf[256];
			int i;

			p = tds_dstr_cstr(&col->table_name);
			for (i = 0; ; ++i) {
				const char *pend;

				if (*p == '[' || *p == '\"') {
					pend = tds_skip_quoted(p);
				} else {
					pend = strchr(p, '.');
					if (!pend)
						pend = strchr(p, 0);
				}
				partials[i].start = p;
				partials[i].end = pend;
				p = pend;
				if (i == 3 || *p != '.')
					break;
				++p;
			}

			/* here i points to last element */
			odbc_unquote(buf, sizeof(buf), partials[i].start, partials[i].end);
			tds_dstr_copy(&drec->sql_desc_base_table_name, buf);

			--i;
			if (i >= 0) {
				odbc_unquote(buf, sizeof(buf), partials[i].start, partials[i].end);
				tds_dstr_copy(&drec->sql_desc_schema_name, buf);
			}

			--i;
			if (i >= 0) {
				odbc_unquote(buf, sizeof(buf), partials[i].start, partials[i].end);
				tds_dstr_copy(&drec->sql_desc_catalog_name, buf);
			}
		}

		drec->sql_desc_unnamed = tds_dstr_isempty(&drec->sql_desc_name) ? SQL_UNNAMED : SQL_NAMED;
		/* TODO use is_nullable_type ?? */
		drec->sql_desc_nullable = col->column_nullable ? SQL_TRUE : SQL_FALSE;
		if (drec->sql_desc_concise_type == SQL_NUMERIC)
			drec->sql_desc_num_prec_radix = 10;
		else
			drec->sql_desc_num_prec_radix = 0;

		drec->sql_desc_octet_length_ptr = NULL;
		switch (type) {
		case SYBDATETIME:
			drec->sql_desc_precision = 3;
			drec->sql_desc_scale     = 3;
			break;
		case SYBMONEY:
			drec->sql_desc_precision = 19;
			drec->sql_desc_scale     = 4;
			break;
		case SYBMONEY4:
			drec->sql_desc_precision = 10;
			drec->sql_desc_scale     = 4;
			break;
		default:
			drec->sql_desc_precision = col->column_prec;
			drec->sql_desc_scale     = col->column_scale;
			break;
		}
		/* TODO test timestamp from db, FOR BROWSE query */
		drec->sql_desc_rowver = SQL_FALSE;
		/* TODO seem not correct */
		drec->sql_desc_searchable = (drec->sql_desc_unnamed == SQL_NAMED) ? SQL_PRED_SEARCHABLE : SQL_UNSEARCHABLE;
		drec->sql_desc_updatable = col->column_writeable && !col->column_identity ? SQL_TRUE : SQL_FALSE;
	}
	return (SQL_SUCCESS);
}

static TDSRET
odbc_cursor_execute(TDS_STMT * stmt)
{
	TDSSOCKET *tds = stmt->tds;
	int send = 0, i;
	TDSRET ret;
	TDSCURSOR *cursor;
	TDSPARAMINFO *params = stmt->params;

	assert(tds);
	assert(stmt->attr.cursor_type != SQL_CURSOR_FORWARD_ONLY || stmt->attr.concurrency != SQL_CONCUR_READ_ONLY);

	tds_release_cursor(&stmt->cursor);
	if (stmt->query)
		cursor = tds_alloc_cursor(tds, tds_dstr_cstr(&stmt->cursor_name), tds_dstr_len(&stmt->cursor_name), stmt->query, strlen(stmt->query));
	else
		cursor = tds_alloc_cursor(tds, tds_dstr_cstr(&stmt->cursor_name), tds_dstr_len(&stmt->cursor_name), stmt->prepared_query, strlen(stmt->prepared_query));
	if (!cursor) {
		odbc_unlock_statement(stmt);

		odbc_errs_add(&stmt->errs, "HY001", NULL);
		return TDS_FAIL;
	}
	stmt->cursor = cursor;

	/* TODO cursor add enums for tds7 */
	switch (stmt->attr.cursor_type) {
	default:
	case SQL_CURSOR_FORWARD_ONLY:
		i = TDS_CUR_TYPE_FORWARD;
		break;
	case SQL_CURSOR_STATIC:
		i = TDS_CUR_TYPE_STATIC;
		break;
	case SQL_CURSOR_KEYSET_DRIVEN:
		i = TDS_CUR_TYPE_KEYSET;
		break;
	case SQL_CURSOR_DYNAMIC:
		i = TDS_CUR_TYPE_DYNAMIC;
		break;
	}
	cursor->type = i;

	switch (stmt->attr.concurrency) {
	default:
	case SQL_CONCUR_READ_ONLY:
		i = TDS_CUR_CONCUR_READ_ONLY;
		break;
	case SQL_CONCUR_LOCK:
		i = TDS_CUR_CONCUR_SCROLL_LOCKS;
		break;
	case SQL_CONCUR_ROWVER:
		i = TDS_CUR_CONCUR_OPTIMISTIC;
		break;
	case SQL_CONCUR_VALUES:
		i = TDS_CUR_CONCUR_OPTIMISTIC_VALUES;
		break;
	}
	cursor->concurrency = 0x2000 | i;

	ret = tds_cursor_declare(tds, cursor, params, &send);
	if (TDS_FAILED(ret))
		return ret;
	ret = tds_cursor_open(tds, cursor, params, &send);
	if (TDS_FAILED(ret))
		return ret;
	/* TODO read results, set row count, check type and scroll returned */
	ret = tds_flush_packet(tds);
	tds_set_state(tds, TDS_PENDING);
	/* set cursor name for TDS7+ */
	if (TDS_SUCCEED(ret) && IS_TDS7_PLUS(tds->conn) && !tds_dstr_isempty(&stmt->cursor_name)) {
		ret = odbc_process_tokens(stmt, TDS_RETURN_DONE|TDS_STOPAT_ROW|TDS_STOPAT_COMPUTE);
		stmt->row_count = tds->rows_affected;
		if (ret == TDS_CMD_DONE && cursor->cursor_id != 0) {
			ret = tds_cursor_setname(tds, cursor);
			tds_set_state(tds, TDS_PENDING);
		} else {
			ret = (ret == TDS_CMD_FAIL) ? TDS_FAIL : TDS_SUCCESS;
		}
		if (!cursor->cursor_id) {
			tds_cursor_dealloc(tds, cursor);
			tds_release_cursor(&stmt->cursor);
		}
	}
	return ret;
}

static TDSHEADERS *
odbc_init_headers(TDS_STMT * stmt, TDSHEADERS * head)
{
	if (tds_dstr_isempty(&stmt->attr.qn_msgtext) || tds_dstr_isempty(&stmt->attr.qn_options))
		return NULL;

	memset(head, 0, sizeof(*head));
	head->qn_timeout = stmt->attr.qn_timeout;
	head->qn_msgtext = tds_dstr_cstr(&stmt->attr.qn_msgtext);
	head->qn_options = tds_dstr_cstr(&stmt->attr.qn_options);
	return head;
}

static SQLRETURN
_SQLExecute(TDS_STMT * stmt)
{
	TDSRET ret;
	TDSSOCKET *tds;
	TDS_INT result_type;
	TDS_INT done = 0;
	int in_row = 0;
	SQLUSMALLINT param_status;
	int found_info = 0, found_error = 0;
	SQLLEN total_rows = TDS_NO_COUNT;
	TDSHEADERS head;

	tdsdump_log(TDS_DBG_FUNC, "_SQLExecute(%p)\n", 
			stmt);

	stmt->row = 0;
		

	/* check parameters are all OK */
	if (stmt->params && stmt->param_num <= stmt->param_count) {
		/* TODO what error ?? */
		ODBC_SAFE_ERROR(stmt);
		return SQL_ERROR;
	}

	if (!odbc_lock_statement(stmt))
		return SQL_ERROR;

	tds = stmt->tds;
	tdsdump_log(TDS_DBG_FUNC, "_SQLExecute() starting with state %d\n", tds->state);

	if (tds->state != TDS_IDLE) {
		if (tds->state == TDS_DEAD) {
			odbc_errs_add(&stmt->errs, "08S01", NULL);
		} else {
			odbc_errs_add(&stmt->errs, "24000", NULL);
		}
		return SQL_ERROR;
	}

	stmt->curr_param_row = 0;
	stmt->num_param_rows = ODBC_MAX(1, stmt->apd->header.sql_desc_array_size);

	stmt->row_count = TDS_NO_COUNT;

	if (stmt->prepared_query_is_rpc) {
		/* TODO support stmt->apd->header.sql_desc_array_size for RPC */
		/* get rpc name */
		/* TODO change method */
		/* TODO cursor change way of calling */
		char *name = stmt->query;
		char *end, tmp;

		if (!name)
			name = stmt->prepared_query;

		end = name;
		if (*end == '[')
			end = (char *) tds_skip_quoted(end);
		else
			while (!isspace((unsigned char) *++end) && *end);
		stmt->prepared_pos = end;
		tmp = *end;
		*end = 0;
		ret = tds_submit_rpc(tds, name, stmt->params, odbc_init_headers(stmt, &head));
		*end = tmp;
	} else if (stmt->attr.cursor_type != SQL_CURSOR_FORWARD_ONLY || stmt->attr.concurrency != SQL_CONCUR_READ_ONLY) {
		ret = odbc_cursor_execute(stmt);
	} else if (stmt->query) {
		/* not prepared query */
		/* TODO cursor change way of calling */
		/* SQLExecDirect */
		if (stmt->num_param_rows <= 1) {
			if (!stmt->params) {
				ret = tds_submit_query_params(tds, stmt->query, NULL, odbc_init_headers(stmt, &head));
			} else {
				ret = tds_submit_execdirect(tds, stmt->query, stmt->params, odbc_init_headers(stmt, &head));
			}
		} else {
			/* pack multiple submit using language */
			TDSMULTIPLE multiple;

			ret = tds_multiple_init(tds, &multiple, TDS_MULTIPLE_QUERY, odbc_init_headers(stmt, &head));
			for (stmt->curr_param_row = 0; TDS_SUCCEED(ret); ) {
				/* submit a query */
				ret = tds_multiple_query(tds, &multiple, stmt->query, stmt->params);
				if (++stmt->curr_param_row >= stmt->num_param_rows)
					break;
				/* than process others parameters */
				/* TODO handle all results*/
				if (start_parse_prepared_query(stmt, 1) != SQL_SUCCESS)
					break;
			}
			if (TDS_SUCCEED(ret))
				ret = tds_multiple_done(tds, &multiple);
		}
	} else if (stmt->num_param_rows <= 1 && IS_TDS71_PLUS(tds->conn) && (!stmt->dyn || stmt->need_reprepare)) {
			if (stmt->dyn) {
				if (odbc_free_dynamic(stmt) != SQL_SUCCESS)
					ODBC_RETURN(stmt, SQL_ERROR);
			}
			stmt->need_reprepare = 0;
			ret = tds71_submit_prepexec(tds, stmt->prepared_query, NULL, &stmt->dyn, stmt->params);
	} else {
		/* TODO cursor change way of calling */
		/* SQLPrepare */
		TDSDYNAMIC *dyn;

		/* prepare dynamic query (only for first SQLExecute call) */
		if (!stmt->dyn || (stmt->need_reprepare && !stmt->dyn->emulated && IS_TDS7_PLUS(tds->conn))) {

			/* free previous prepared statement */
			if (stmt->dyn) {
				if (odbc_free_dynamic(stmt) != SQL_SUCCESS)
					ODBC_RETURN(stmt, SQL_ERROR);
			}
			stmt->need_reprepare = 0;

			tdsdump_log(TDS_DBG_INFO1, "Creating prepared statement\n");
			/* TODO use tds_submit_prepexec (mssql2k, tds71) */
			if (TDS_FAILED(tds_submit_prepare(tds, stmt->prepared_query, NULL, &stmt->dyn, stmt->params))) {
				/* TODO ?? tds_free_param_results(params); */
				ODBC_SAFE_ERROR(stmt);
				return SQL_ERROR;
			}
			if (TDS_FAILED(tds_process_simple_query(tds))) {
				tds_release_dynamic(&stmt->dyn);
				/* TODO ?? tds_free_param_results(params); */
				ODBC_SAFE_ERROR(stmt);
				return SQL_ERROR;
			}
		}
		stmt->row_count = TDS_NO_COUNT;
		if (stmt->num_param_rows <= 1) {
			dyn = stmt->dyn;
			tds_free_input_params(dyn);
			dyn->params = stmt->params;
			/* prevent double free */
			stmt->params = NULL;
			tdsdump_log(TDS_DBG_INFO1, "End prepare, execute\n");
			/* TODO return error to client */
			ret = tds_submit_execute(tds, dyn);
		} else {
			TDSMULTIPLE multiple;

			ret = tds_multiple_init(tds, &multiple, TDS_MULTIPLE_EXECUTE, NULL);
			for (stmt->curr_param_row = 0; TDS_SUCCEED(ret); ) {
				dyn = stmt->dyn;
				tds_free_input_params(dyn);
				dyn->params = stmt->params;
				/* prevent double free */
				stmt->params = NULL;
				ret = tds_multiple_execute(tds, &multiple, dyn);
				if (++stmt->curr_param_row >= stmt->num_param_rows)
					break;
				/* than process others parameters */
				/* TODO handle all results*/
				if (start_parse_prepared_query(stmt, 1) != SQL_SUCCESS)
					break;
			}
			if (TDS_SUCCEED(ret))
				ret = tds_multiple_done(tds, &multiple);
		}
	}
	if (TDS_FAILED(ret)) {
		ODBC_SAFE_ERROR(stmt);
		return SQL_ERROR;
	}
	/* catch all errors */
	if (!odbc_lock_statement(stmt))
		ODBC_RETURN_(stmt);

	stmt->row_status = PRE_NORMAL_ROW;

	stmt->curr_param_row = 0;
	param_status = SQL_PARAM_SUCCESS;

	/* TODO review this, ODBC return parameter in other way, for compute I don't know */
	/* TODO perhaps we should return SQL_NO_DATA if no data available... see old SQLExecute code */
	for (;;) {
		result_type = odbc_process_tokens(stmt, TDS_TOKEN_RESULTS);
		tdsdump_log(TDS_DBG_FUNC, "_SQLExecute: odbc_process_tokens returned result_type %d\n", result_type);
		switch (result_type) {
		case TDS_CMD_FAIL:
		case TDS_CMD_DONE:
		case TDS_COMPUTE_RESULT:
		case TDS_ROW_RESULT:
			done = 1;
			break;

		case TDS_DONE_RESULT:
		case TDS_DONEPROC_RESULT:
			switch (stmt->errs.lastrc) {
			case SQL_ERROR:
				found_error = 1;
				param_status = SQL_PARAM_ERROR;
				break;
			case SQL_SUCCESS_WITH_INFO:
				found_info = 1;
				param_status = SQL_PARAM_SUCCESS_WITH_INFO;
				break;
			}
			if (stmt->curr_param_row < stmt->num_param_rows && stmt->ipd->header.sql_desc_array_status_ptr)
				stmt->ipd->header.sql_desc_array_status_ptr[stmt->curr_param_row] = param_status;
			param_status = SQL_PARAM_SUCCESS;
			++stmt->curr_param_row;
			/* actually is quite strange, if prepared always success with info or success
			 * if not prepared return last one
			 */
			if (stmt->curr_param_row < stmt->num_param_rows || result_type == TDS_DONEPROC_RESULT)
				stmt->errs.lastrc = SQL_SUCCESS;
			if (total_rows == TDS_NO_COUNT)
				total_rows = stmt->row_count;
			else if (stmt->row_count != TDS_NO_COUNT)
				total_rows += stmt->row_count;
			stmt->row_count = TDS_NO_COUNT;
			if (stmt->curr_param_row >= stmt->num_param_rows) {
				done = 1;
				break;
			}
			break;

		case TDS_DONEINPROC_RESULT:
			if (in_row)
				done = 1;
			break;

			/* ignore metadata, stop at done or row */
		case TDS_ROWFMT_RESULT:
			if (in_row) {
				done = 1;
				break;
			}
			stmt->row = 0;
			stmt->row_count = TDS_NO_COUNT;
			stmt->row_status = PRE_NORMAL_ROW;
			in_row = 1;
			break;
		}
		if (done)
			break;
	}
	if ((found_info || found_error) && stmt->errs.lastrc != SQL_ERROR)
		stmt->errs.lastrc = SQL_SUCCESS_WITH_INFO;
	if (tds_dstr_isempty(&stmt->attr.qn_msgtext) != tds_dstr_isempty(&stmt->attr.qn_options)) {
		odbc_errs_add(&stmt->errs, "HY000", "Attribute ignored");
		stmt->errs.lastrc = SQL_SUCCESS_WITH_INFO;
	}
	if (found_error && stmt->num_param_rows <= 1)
		stmt->errs.lastrc = SQL_ERROR;
	if (stmt->curr_param_row < stmt->num_param_rows) {
		if (stmt->ipd->header.sql_desc_array_status_ptr)
			stmt->ipd->header.sql_desc_array_status_ptr[stmt->curr_param_row] = param_status;
		++stmt->curr_param_row;
		if (total_rows == TDS_NO_COUNT)
			total_rows = stmt->row_count;
		else if (stmt->row_count != TDS_NO_COUNT)
			total_rows += stmt->row_count;
		stmt->row_count = TDS_NO_COUNT;
	}
	if (stmt->ipd->header.sql_desc_rows_processed_ptr)
		*stmt->ipd->header.sql_desc_rows_processed_ptr = ODBC_MIN(stmt->curr_param_row, stmt->num_param_rows);

	if (total_rows != TDS_NO_COUNT)
		stmt->row_count = total_rows;

	odbc_populate_ird(stmt);
	switch (result_type) {
	case TDS_CMD_DONE:
		odbc_unlock_statement(stmt);
		if (stmt->errs.lastrc == SQL_SUCCESS && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3
		    && stmt->row_count == TDS_NO_COUNT && !stmt->cursor)
			ODBC_RETURN(stmt, SQL_NO_DATA);
		break;

	case TDS_CMD_FAIL:
		/* TODO test what happened, report correct error to client */
		tdsdump_log(TDS_DBG_INFO1, "SQLExecute: bad results\n");
		ODBC_SAFE_ERROR(stmt);
		return SQL_ERROR;
	}
	ODBC_RETURN_(stmt);
}

ODBC_FUNC(SQLExecDirect, (P(SQLHSTMT,hstmt), PCHARIN(SqlStr,SQLINTEGER) WIDE))
{
	SQLRETURN res;

	ODBC_ENTER_HSTMT;

	if (SQL_SUCCESS != odbc_set_stmt_query(stmt, szSqlStr, cbSqlStr _wide)) {
		odbc_errs_add(&stmt->errs, "HY001", NULL);
		ODBC_EXIT_(stmt);
	}

	/* count placeholders */
	/* note: szSqlStr can be no-null terminated, so first we set query and then count placeholders */
	stmt->param_count = tds_count_placeholders(stmt->query);
	stmt->param_data_called = 0;

	if (SQL_SUCCESS != prepare_call(stmt)) {
		/* TODO return another better error, prepare_call should set error ?? */
		odbc_errs_add(&stmt->errs, "HY000", "Could not prepare call");
		ODBC_EXIT_(stmt);
	}

	res = start_parse_prepared_query(stmt, 1);
	if (SQL_SUCCESS != res)
		ODBC_EXIT(stmt, res);

	ODBC_EXIT(stmt, _SQLExecute(stmt));
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLExecute(SQLHSTMT hstmt)
{
	ODBC_PRRET_BUF;
	SQLRETURN res;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLExecute(%p)\n", hstmt);

	if (!stmt->prepared_query) {
		/* TODO error report, only without DM ?? */
		tdsdump_log(TDS_DBG_FUNC, "SQLExecute returns SQL_ERROR (not prepared)\n");
		ODBC_EXIT(stmt, SQL_ERROR);
	}

	/* TODO rebuild should be done for every bindings change, not every time */
	/* TODO free previous parameters */
	/* build parameters list */
	stmt->param_data_called = 0;
	stmt->curr_param_row = 0;
	if ((res = start_parse_prepared_query(stmt, 1)) != SQL_SUCCESS) {
		tdsdump_log(TDS_DBG_FUNC, "SQLExecute returns %s (start_parse_prepared_query failed)\n", odbc_prret(res));
		ODBC_EXIT(stmt, res);
	}

	/* TODO test if two SQLPrepare on a statement */
	/* TODO test unprepare on statement free or connection close */

	res = _SQLExecute(stmt);

	tdsdump_log(TDS_DBG_FUNC, "SQLExecute returns %s\n", odbc_prret(res));

	ODBC_EXIT(stmt, res);
}

static int
odbc_process_tokens(TDS_STMT * stmt, unsigned flag)
{
	TDS_INT result_type;
	int done_flags;
	TDSSOCKET * tds = stmt->tds;

	flag |= TDS_RETURN_DONE | TDS_RETURN_PROC;
	for (;;) {
		TDSRET retcode = tds_process_tokens(tds, &result_type, &done_flags, flag);
		tdsdump_log(TDS_DBG_FUNC, "odbc_process_tokens: tds_process_tokens returned %d\n", retcode);
		tdsdump_log(TDS_DBG_FUNC, "    result_type=%d, TDS_DONE_COUNT=%x, TDS_DONE_ERROR=%x\n", 
						result_type, (done_flags & TDS_DONE_COUNT), (done_flags & TDS_DONE_ERROR));
		switch (retcode) {
		case TDS_SUCCESS:
			break;
		case TDS_NO_MORE_RESULTS:
			return TDS_CMD_DONE;
		case TDS_CANCELLED:
			odbc_errs_add(&stmt->errs, "HY008", NULL);
		default:
			return TDS_CMD_FAIL;
		}

		switch (result_type) {
		case TDS_STATUS_RESULT:
			odbc_set_return_status(stmt, ODBC_MIN(stmt->curr_param_row, stmt->num_param_rows - 1));
			break;
		case TDS_PARAM_RESULT:
			odbc_set_return_params(stmt, ODBC_MIN(stmt->curr_param_row, stmt->num_param_rows - 1));
			break;

		case TDS_DONE_RESULT:
		case TDS_DONEPROC_RESULT:
			if (stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3)
				flag |= TDS_STOPAT_MSG;
			if (done_flags & TDS_DONE_COUNT) {
				/* correct ?? overwrite ?? */
				if (stmt->row_count == TDS_NO_COUNT)
					stmt->row_count = tds->rows_affected;
			}
			if (done_flags & TDS_DONE_ERROR)
				stmt->errs.lastrc = SQL_ERROR;
			/* test for internal_sp not very fine, used for param set  -- freddy77 */
			if ((done_flags & (TDS_DONE_COUNT|TDS_DONE_ERROR)) != 0
			    || (stmt->errs.lastrc == SQL_SUCCESS_WITH_INFO && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3)
			    || (result_type == TDS_DONEPROC_RESULT && tds->current_op == TDS_OP_EXECUTE)) {
				/* FIXME this row is used only as a flag for update binding, should be cleared if binding/result changed */
				stmt->row = 0;
#if 0
				tds_free_all_results(tds);
				odbc_populate_ird(stmt);
#endif
				tdsdump_log(TDS_DBG_FUNC, "odbc_process_tokens: row_count=%" PRId64 "\n", stmt->row_count);
				return result_type;
			}
			tdsdump_log(TDS_DBG_FUNC, "odbc_process_tokens: processed %s\n", 
					result_type==TDS_DONE_RESULT? "TDS_DONE_RESULT" : "TDS_DONEPROC_RESULT");
			break;

		/*
		 * TODO test flags ? check error and change result ?
		 * see also other DONEINPROC handle (below)
		 */
		case TDS_DONEINPROC_RESULT:
			if (stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3)
				flag |= TDS_STOPAT_MSG;
			if (done_flags & TDS_DONE_COUNT) {
				stmt->row_count = tds->rows_affected;
			}
			if (done_flags & TDS_DONE_ERROR)
				stmt->errs.lastrc = SQL_ERROR;
			/* TODO perhaps it can be a problem if SET NOCOUNT ON, test it */
#if 0
			tds_free_all_results(tds);
			odbc_populate_ird(stmt);
#endif
			tdsdump_log(TDS_DBG_FUNC, "odbc_process_tokens: processed TDS_DONEINPROC_RESULT\n");
			if (stmt->row_status == PRE_NORMAL_ROW)
				return result_type;
			break;

		default:
			tdsdump_log(TDS_DBG_FUNC, "odbc_process_tokens: returning result_type %d\n", result_type);
			return result_type;
		}
	}
}

static void
odbc_fix_data_type_col(TDS_STMT *stmt, int idx)
{
	TDSSOCKET *tds = stmt->tds;
	TDSRESULTINFO *resinfo;
	TDSCOLUMN *colinfo;

	if (!tds)
		return;

	resinfo = tds->current_results;
	if (!resinfo || resinfo->num_cols <= idx)
		return;

	colinfo = resinfo->columns[idx];
	if (colinfo->column_cur_size < 0)
		return;

	switch (tds_get_conversion_type(colinfo->column_type, colinfo->column_size)) {
	case SYBINT2: {
		TDS_SMALLINT *data = (TDS_SMALLINT *) colinfo->column_data;
		*data = odbc_swap_datetime_sql_type(*data);
		}
		break;
	case SYBINT4: {
		TDS_INT *data = (TDS_INT *) colinfo->column_data;
		*data = odbc_swap_datetime_sql_type(*data);
		}
		break;
	}
}

/*
 * - handle correctly SQLGetData (for forward cursors accept only row_size == 1
 *   for other types application must use SQLSetPos)
 * - handle correctly results (SQL_SUCCESS_WITH_INFO if error on some rows,
 *   SQL_ERROR for all rows, see doc)
 */
static SQLRETURN
_SQLFetch(TDS_STMT * stmt, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
	TDSSOCKET *tds;
	TDSRESULTINFO *resinfo;
	TDSCOLUMN *colinfo;
	int i;
	SQLULEN curr_row, num_rows;
	SQLINTEGER len = 0;
	TDS_CHAR *src;
	int srclen;
	struct _drecord *drec_ard;
	TDS_DESC *ard;
	SQLULEN dummy, *fetched_ptr;
	SQLUSMALLINT *status_ptr, row_status;
	TDS_INT result_type;
	int truncated = 0;

#define AT_ROW(ptr, type) (row_offset ? (type*)(((char*)(ptr)) + row_offset) : &ptr[curr_row])
	SQLLEN row_offset = 0;

	tdsdump_log(TDS_DBG_FUNC, "_SQLFetch(%p, %d, %d)\n", stmt, (int)FetchOrientation, (int)FetchOffset);

	if (stmt->ard->header.sql_desc_bind_type != SQL_BIND_BY_COLUMN && stmt->ard->header.sql_desc_bind_offset_ptr)
		row_offset = *stmt->ard->header.sql_desc_bind_offset_ptr;

	ard = stmt->ard;

	tds = stmt->tds;
	num_rows = ard->header.sql_desc_array_size;

	/* TODO cursor check also type of cursor (scrollable, not forward) */
	if (FetchOrientation != SQL_FETCH_NEXT && (!stmt->cursor || !stmt->dbc->cursor_support)) {
		odbc_errs_add(&stmt->errs, "HY106", NULL);
		return SQL_ERROR;
	}

	/* handle cursors, fetch wanted rows */
	if (stmt->cursor && odbc_lock_statement(stmt)) {
		TDSCURSOR *cursor = stmt->cursor;
		TDS_CURSOR_FETCH fetch_type = TDS_CURSOR_FETCH_NEXT;

		tds = stmt->tds;

		switch (FetchOrientation) {
		case SQL_FETCH_NEXT:
			break;
		case SQL_FETCH_FIRST:
			fetch_type = TDS_CURSOR_FETCH_FIRST;
			break;
		case SQL_FETCH_LAST:
			fetch_type = TDS_CURSOR_FETCH_LAST;
			break;
		case SQL_FETCH_PRIOR:
			fetch_type = TDS_CURSOR_FETCH_PREV;
			break;
		case SQL_FETCH_ABSOLUTE:
			fetch_type = TDS_CURSOR_FETCH_ABSOLUTE;
			break;
		case SQL_FETCH_RELATIVE:
			fetch_type = TDS_CURSOR_FETCH_RELATIVE;
			break;
		/* TODO cursor bookmark */
		default:
			odbc_errs_add(&stmt->errs, "HYC00", NULL);
			return SQL_ERROR;
		}

		if (cursor->cursor_rows != num_rows) {
			int send = 0;
			cursor->cursor_rows = num_rows;
			/* TODO handle change rows (tds5) */
			/*
			 * TODO curerntly we support cursors only using tds7+
			 * so this function can't fail but remember to add error
			 * check when tds5 will be supported
			 */
			tds_cursor_setrows(tds, cursor, &send);
		}

		if (TDS_FAILED(tds_cursor_fetch(tds, cursor, fetch_type, FetchOffset))) {
			/* TODO what kind of error ?? */
			ODBC_SAFE_ERROR(stmt);
			return SQL_ERROR;
		}

		/* TODO handle errors in a better way */
		odbc_process_tokens(stmt, TDS_RETURN_ROW|TDS_STOPAT_COMPUTE|TDS_STOPAT_ROW);
		stmt->row_status = PRE_NORMAL_ROW;
	}

	if (!tds || stmt->row_status == NOT_IN_ROW) {
		odbc_errs_add(&stmt->errs, "24000", NULL);
		return SQL_ERROR;
	}
	IRD_CHECK;

	if (stmt->ird->header.sql_desc_count <= 0) {
		odbc_errs_add(&stmt->errs, "24000", NULL);
		return SQL_ERROR;
	}

	stmt->row++;

	fetched_ptr = &dummy;
	if (stmt->ird->header.sql_desc_rows_processed_ptr)
		fetched_ptr = stmt->ird->header.sql_desc_rows_processed_ptr;
	*fetched_ptr = 0;

	status_ptr = stmt->ird->header.sql_desc_array_status_ptr;
	if (status_ptr) {
		for (i = 0; i < num_rows; ++i)
			*status_ptr++ = SQL_ROW_NOROW;
		status_ptr = stmt->ird->header.sql_desc_array_status_ptr;
	}

	curr_row = 0;
	do {
		row_status = SQL_ROW_SUCCESS;

		/* do not get compute row if we are not expecting a compute row */
		switch (stmt->row_status) {
		case AFTER_COMPUTE_ROW:
			/* handle done if needed */
			/* FIXME doesn't seem so fine ... - freddy77 */
			tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_TRAILING);
			goto all_done;

		case IN_COMPUTE_ROW:
			/* compute recorset contains only a row */
			/* we already fetched compute row in SQLMoreResults so do not fetch another one */
			num_rows = 1;
			stmt->row_status = AFTER_COMPUTE_ROW;
			break;

		default:
			/* FIXME stmt->row_count set correctly ?? TDS_DONE_COUNT not checked */
			switch (odbc_process_tokens(stmt, TDS_STOPAT_ROWFMT|TDS_RETURN_ROW|TDS_STOPAT_COMPUTE)) {
			case TDS_ROW_RESULT:
				break;
			default:
#if 0
				stmt->row_count = tds->rows_affected;
#endif
				/* 
				 * NOTE do not set row_status to NOT_IN_ROW, 
				 * if compute tds_process_tokens above returns TDS_NO_MORE_RESULTS
				 */
				stmt->row_status = PRE_NORMAL_ROW;
				stmt->special_row = ODBC_SPECIAL_NONE;
#if 0
				odbc_populate_ird(stmt);
#endif
				tdsdump_log(TDS_DBG_INFO1, "SQLFetch: NO_DATA_FOUND\n");
				goto all_done;
				break;
			case TDS_CMD_FAIL:
				ODBC_SAFE_ERROR(stmt);
				return SQL_ERROR;
				break;
			}

			stmt->row_status = IN_NORMAL_ROW;

			/* handle special row */
			switch (stmt->special_row) {
			case ODBC_SPECIAL_GETTYPEINFO:
				odbc_fix_data_type_col(stmt, 1);
				break;
			case ODBC_SPECIAL_COLUMNS:
				odbc_fix_data_type_col(stmt, 4);
				odbc_fix_data_type_col(stmt, 13); /* TODO sure ?? */
				break;
			case ODBC_SPECIAL_PROCEDURECOLUMNS:
				odbc_fix_data_type_col(stmt, 5);
				odbc_fix_data_type_col(stmt, 14); /* TODO sure ?? */
				break;
			case ODBC_SPECIAL_SPECIALCOLUMNS:
				odbc_fix_data_type_col(stmt, 2);
				break;
			case ODBC_SPECIAL_NONE:
				break;
			}
		}

		resinfo = tds->current_results;
		if (!resinfo) {
			tdsdump_log(TDS_DBG_INFO1, "SQLFetch: !resinfo\n");
			break;
		}

		/* we got a row, return a row readed even if error (for ODBC specifications) */
		++(*fetched_ptr);
		for (i = 0; i < resinfo->num_cols; i++) {
			colinfo = resinfo->columns[i];
			colinfo->column_text_sqlgetdatapos = 0;
			drec_ard = (i < ard->header.sql_desc_count) ? &ard->records[i] : NULL;
			if (!drec_ard)
				continue;
			if (colinfo->column_cur_size < 0) {
				if (drec_ard->sql_desc_indicator_ptr) {
					*AT_ROW(drec_ard->sql_desc_indicator_ptr, SQLLEN) = SQL_NULL_DATA;
				} else if (drec_ard->sql_desc_data_ptr) {
					odbc_errs_add(&stmt->errs, "22002", NULL);
					row_status = SQL_ROW_ERROR;
					break;
				}
				continue;
			}
			/* set indicator to 0 if data is not null */
			if (drec_ard->sql_desc_indicator_ptr)
				*AT_ROW(drec_ard->sql_desc_indicator_ptr, SQLLEN) = 0;

			/* TODO what happen to length if no data is returned (drec->sql_desc_data_ptr == NULL) ?? */
			len = 0;
			if (drec_ard->sql_desc_data_ptr) {
				int c_type;
				TDS_CHAR *data_ptr = (TDS_CHAR *) drec_ard->sql_desc_data_ptr;

				src = (TDS_CHAR *) colinfo->column_data;
				srclen = colinfo->column_cur_size;
				colinfo->column_text_sqlgetdatapos = 0;
				c_type = drec_ard->sql_desc_concise_type;
				if (c_type == SQL_C_DEFAULT)
					c_type = odbc_sql_to_c_type_default(stmt->ird->records[i].sql_desc_concise_type);
				if (row_offset || curr_row == 0) {
					data_ptr += row_offset;
				} else {
					data_ptr += odbc_get_octet_len(c_type, drec_ard) * curr_row;
				}
				len = odbc_tds2sql(stmt, colinfo, tds_get_conversion_type(colinfo->on_server.column_type, colinfo->on_server.column_size),
						      src, srclen, c_type, data_ptr, drec_ard->sql_desc_octet_length, drec_ard);
				if (len == SQL_NULL_DATA) {
					row_status = SQL_ROW_ERROR;
					break;
				}
				if ((c_type == SQL_C_CHAR && len >= drec_ard->sql_desc_octet_length)
				    || (c_type == SQL_C_BINARY && len > drec_ard->sql_desc_octet_length)) {
					truncated = 1;
					stmt->errs.lastrc = SQL_SUCCESS_WITH_INFO;
				}
			}
			if (drec_ard->sql_desc_octet_length_ptr)
				*AT_ROW(drec_ard->sql_desc_octet_length_ptr, SQLLEN) = len;
		}

		if (status_ptr)
			*status_ptr++ = truncated ? SQL_ROW_ERROR : row_status;
		if (row_status == SQL_ROW_ERROR) {
			stmt->errs.lastrc = SQL_ERROR;
			break;
		}
#if SQL_BIND_BY_COLUMN != 0
		if (stmt->ard->header.sql_desc_bind_type != SQL_BIND_BY_COLUMN)
#endif
			row_offset += stmt->ard->header.sql_desc_bind_type;
	} while (++curr_row < num_rows);

	if (truncated)
		odbc_errs_add(&stmt->errs, "01004", NULL);

      all_done:
	/* TODO cursor correct ?? */
	if (stmt->cursor) {
		tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_TRAILING);
		odbc_unlock_statement(stmt);
	}
	if (*fetched_ptr == 0 && (stmt->errs.lastrc == SQL_SUCCESS || stmt->errs.lastrc == SQL_SUCCESS_WITH_INFO))
		ODBC_RETURN(stmt, SQL_NO_DATA);
	if (stmt->errs.lastrc == SQL_ERROR && (*fetched_ptr > 1 || (*fetched_ptr == 1 && row_status != SQL_ROW_ERROR)))
		ODBC_RETURN(stmt, SQL_SUCCESS_WITH_INFO);
	ODBC_RETURN_(stmt);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLFetch(SQLHSTMT hstmt)
{
	SQLRETURN ret;
	struct {
		SQLULEN  array_size;
		SQLULEN *rows_processed_ptr;
		SQLUSMALLINT *array_status_ptr;
	} keep;
	
	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLFetch(%p)\n", hstmt);

	keep.array_size = stmt->ard->header.sql_desc_array_size;
	keep.rows_processed_ptr = stmt->ird->header.sql_desc_rows_processed_ptr;
	keep.array_status_ptr = stmt->ird->header.sql_desc_array_status_ptr;
	
	if (stmt->dbc->env->attr.odbc_version != SQL_OV_ODBC3) {
		stmt->ard->header.sql_desc_array_size = 1;
		stmt->ird->header.sql_desc_rows_processed_ptr = NULL;
		stmt->ird->header.sql_desc_array_status_ptr = NULL;
	}

	ret = _SQLFetch(stmt, SQL_FETCH_NEXT, 0);

	if (stmt->dbc->env->attr.odbc_version != SQL_OV_ODBC3) {
		stmt->ard->header.sql_desc_array_size = keep.array_size;
		stmt->ird->header.sql_desc_rows_processed_ptr = keep.rows_processed_ptr;
		stmt->ird->header.sql_desc_array_status_ptr = keep.array_status_ptr;
	}

	ODBC_EXIT(stmt, ret);
}

#if (ODBCVER >= 0x0300)
SQLRETURN ODBC_PUBLIC ODBC_API
SQLFetchScroll(SQLHSTMT hstmt, SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLFetchScroll(%p, %d, %d)\n", hstmt, FetchOrientation, (int)FetchOffset);

	if (FetchOrientation != SQL_FETCH_NEXT && !stmt->dbc->cursor_support) {
		odbc_errs_add(&stmt->errs, "HY106", NULL);
		ODBC_EXIT_(stmt);
	}

	ODBC_EXIT(stmt, _SQLFetch(stmt, FetchOrientation, FetchOffset));
}
#endif


#if (ODBCVER >= 0x0300)
SQLRETURN ODBC_PUBLIC ODBC_API
SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
	tdsdump_log(TDS_DBG_INFO1, "SQLFreeHandle(%d, %p)\n", HandleType, (void *) Handle);

	switch (HandleType) {
	case SQL_HANDLE_STMT:
		return _SQLFreeStmt(Handle, SQL_DROP, 0);
		break;
	case SQL_HANDLE_DBC:
		return _SQLFreeConnect(Handle);
		break;
	case SQL_HANDLE_ENV:
		return _SQLFreeEnv(Handle);
		break;
	case SQL_HANDLE_DESC:
		return _SQLFreeDesc(Handle);
		break;
	}
	return SQL_ERROR;
}

static SQLRETURN
_SQLFreeConnect(SQLHDBC hdbc)
{
	int i;

	ODBC_ENTER_HDBC;

	tdsdump_log(TDS_DBG_FUNC, "_SQLFreeConnect(%p)\n", 
			hdbc);

	/* TODO if connected return error */
	tds_free_socket(dbc->tds_socket);

	/* free attributes */
#ifdef TDS_NO_DM
	tds_dstr_free(&dbc->attr.tracefile);
#endif
	tds_dstr_free(&dbc->attr.current_catalog);
	tds_dstr_free(&dbc->attr.translate_lib);

#ifdef ENABLE_ODBC_WIDE
	tds_dstr_free(&dbc->original_charset);
#endif
	tds_dstr_free(&dbc->server);
	tds_dstr_free(&dbc->dsn);

	for (i = 0; i < TDS_MAX_APP_DESC; i++) {
		if (dbc->uad[i]) {
			desc_free(dbc->uad[i]);
		}
	}
	odbc_errs_reset(&dbc->errs);
	tds_mutex_unlock(&dbc->mtx);
	tds_mutex_free(&dbc->mtx);

	free(dbc);

	return SQL_SUCCESS;
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLFreeConnect(SQLHDBC hdbc)
{
	tdsdump_log(TDS_DBG_INFO2, "SQLFreeConnect(%p)\n", hdbc);
	return _SQLFreeConnect(hdbc);
}
#endif

static SQLRETURN
_SQLFreeEnv(SQLHENV henv)
{
	ODBC_ENTER_HENV;

	tdsdump_log(TDS_DBG_FUNC, "_SQLFreeEnv(%p)\n", 
			henv);

	odbc_errs_reset(&env->errs);
	tds_free_context(env->tds_ctx);
	tds_mutex_unlock(&env->mtx);
	tds_mutex_free(&env->mtx);
	free(env);

	return SQL_SUCCESS;
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLFreeEnv(SQLHENV henv)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLFreeEnv(%p)\n", henv);

	return _SQLFreeEnv(henv);
}

static SQLRETURN
_SQLFreeStmt(SQLHSTMT hstmt, SQLUSMALLINT fOption, int force)
{
	TDSSOCKET *tds;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "_SQLFreeStmt(%p, %d, %d)\n", hstmt, fOption, force);

	/* check if option correct */
	if (fOption != SQL_DROP && fOption != SQL_CLOSE && fOption != SQL_UNBIND && fOption != SQL_RESET_PARAMS) {
		tdsdump_log(TDS_DBG_ERROR, "SQLFreeStmt: Unknown option %d\n", fOption);
		odbc_errs_add(&stmt->errs, "HY092", NULL);
		ODBC_EXIT_(stmt);
	}

	/* if we have bound columns, free the temporary list */
	if (fOption == SQL_DROP || fOption == SQL_UNBIND) {
		desc_free_records(stmt->ard);
	}

	/* do the same for bound parameters */
	if (fOption == SQL_DROP || fOption == SQL_RESET_PARAMS) {
		desc_free_records(stmt->apd);
		desc_free_records(stmt->ipd);
	}

	/* close statement */
	if (fOption == SQL_DROP || fOption == SQL_CLOSE) {
		SQLRETURN retcode;

		tds = stmt->tds;
		/*
		 * FIXME -- otherwise make sure the current statement is complete
		 */
		/* do not close other running query ! */
		if (tds && tds->state != TDS_IDLE && tds->state != TDS_DEAD) {
			if (TDS_SUCCEED(tds_send_cancel(tds)))
				tds_process_cancel(tds);
		}

		/* free cursor */
		retcode = odbc_free_cursor(stmt);
		if (!force && retcode != SQL_SUCCESS)
			ODBC_EXIT(stmt, retcode);
	}

	/* free it */
	if (fOption == SQL_DROP) {
		SQLRETURN retcode;

		/* close prepared statement or add to connection */
		retcode = odbc_free_dynamic(stmt);
		if (!force && retcode != SQL_SUCCESS)
			ODBC_EXIT(stmt, retcode);

		/* detatch from list */
		tds_mutex_lock(&stmt->dbc->mtx);
		if (stmt->next)
			stmt->next->prev = stmt->prev;
		if (stmt->prev)
			stmt->prev->next = stmt->next;
		if (stmt->dbc->stmt_list == stmt)
			stmt->dbc->stmt_list = stmt->next;
		tds_mutex_unlock(&stmt->dbc->mtx);

		free(stmt->query);
		free(stmt->prepared_query);
		tds_free_param_results(stmt->params);
		odbc_errs_reset(&stmt->errs);
		odbc_unlock_statement(stmt);
		tds_dstr_free(&stmt->cursor_name);
		tds_dstr_free(&stmt->attr.qn_msgtext);
		tds_dstr_free(&stmt->attr.qn_options);
		desc_free(stmt->ird);
		desc_free(stmt->ipd);
		desc_free(stmt->orig_ard);
		desc_free(stmt->orig_apd);
		tds_mutex_unlock(&stmt->mtx);
		tds_mutex_free(&stmt->mtx);
		free(stmt);

		/* NOTE we freed stmt, do not use ODBC_EXIT */
		return SQL_SUCCESS;
	}
	ODBC_EXIT_(stmt);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLFreeStmt(SQLHSTMT hstmt, SQLUSMALLINT fOption)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLFreeStmt(%p, %d)\n", hstmt, fOption);
	return _SQLFreeStmt(hstmt, fOption, 0);
}


SQLRETURN ODBC_PUBLIC ODBC_API
SQLCloseCursor(SQLHSTMT hstmt)
{
	/* TODO cursors */
	/*
	 * Basic implementation for when no driver manager is present.
	 *  - according to ODBC documentation SQLCloseCursor is more or less
	 *    equivalent to SQLFreeStmt(..., SQL_CLOSE).
	 *  - indeed that is what the DM does if it can't find the function
	 *    in the driver, so this is pretty close.
	 */
	/* TODO remember to close cursors really when get implemented */
	/* TODO read all results and discard them or use cancellation ?? test behaviour */

	tdsdump_log(TDS_DBG_FUNC, "SQLCloseCursor(%p)\n", hstmt);
	return _SQLFreeStmt(hstmt, SQL_CLOSE, 0);
}

static SQLRETURN
_SQLFreeDesc(SQLHDESC hdesc)
{
	ODBC_ENTER_HDESC;

	tdsdump_log(TDS_DBG_FUNC, "_SQLFreeDesc(%p)\n", 
			hdesc);

	if (desc->header.sql_desc_alloc_type != SQL_DESC_ALLOC_USER) {
		odbc_errs_add(&desc->errs, "HY017", NULL);
		ODBC_EXIT_(desc);
	}

	if (IS_HDBC(desc->parent)) {
		TDS_DBC *dbc = (TDS_DBC *) desc->parent;
		TDS_STMT *stmt;
		int i;

		/* freeing descriptors associated to statements revert state of statements */
		tds_mutex_lock(&dbc->mtx);
		for (stmt = dbc->stmt_list; stmt != NULL; stmt = stmt->next) {
			if (stmt->ard == desc)
				stmt->ard = stmt->orig_ard;
			if (stmt->apd == desc)
				stmt->apd = stmt->orig_apd;
		}
		tds_mutex_unlock(&dbc->mtx);

		for (i = 0; i < TDS_MAX_APP_DESC; ++i) {
			if (dbc->uad[i] == desc) {
				dbc->uad[i] = NULL;
				desc_free(desc);
				break;
			}
		}
	}
	return SQL_SUCCESS;
}

static SQLRETURN
_SQLGetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength WIDE)
{
	void *src;
	size_t size;

	ODBC_ENTER_HSTMT;

	/* TODO assign directly, use macro for size */
	switch (Attribute) {
	case SQL_ATTR_APP_PARAM_DESC:
		size = sizeof(stmt->apd);
		src = &stmt->apd;
		break;
	case SQL_ATTR_APP_ROW_DESC:
		size = sizeof(stmt->ard);
		src = &stmt->ard;
		break;
	case SQL_ATTR_ASYNC_ENABLE:
		size = sizeof(stmt->attr.async_enable);
		src = &stmt->attr.async_enable;
		break;
	case SQL_ATTR_CONCURRENCY:
		size = sizeof(stmt->attr.concurrency);
		src = &stmt->attr.concurrency;
		break;
	case SQL_ATTR_CURSOR_TYPE:
		size = sizeof(stmt->attr.cursor_type);
		src = &stmt->attr.cursor_type;
		break;
	case SQL_ATTR_ENABLE_AUTO_IPD:
		size = sizeof(stmt->attr.enable_auto_ipd);
		src = &stmt->attr.enable_auto_ipd;
		break;
	case SQL_ATTR_FETCH_BOOKMARK_PTR:
		size = sizeof(stmt->attr.fetch_bookmark_ptr);
		src = &stmt->attr.fetch_bookmark_ptr;
		break;
	case SQL_ATTR_KEYSET_SIZE:
		size = sizeof(stmt->attr.keyset_size);
		src = &stmt->attr.keyset_size;
		break;
	case SQL_ATTR_MAX_LENGTH:
		size = sizeof(stmt->attr.max_length);
		src = &stmt->attr.max_length;
		break;
	case SQL_ATTR_MAX_ROWS:
		size = sizeof(stmt->attr.max_rows);
		src = &stmt->attr.max_rows;
		break;
	case SQL_ATTR_METADATA_ID:
		size = sizeof(stmt->attr.metadata_id);
		src = &stmt->attr.noscan;
		break;
	case SQL_ATTR_NOSCAN:
		size = sizeof(stmt->attr.noscan);
		src = &stmt->attr.noscan;
		break;
	case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
		size = sizeof(stmt->apd->header.sql_desc_bind_offset_ptr);
		src = &stmt->apd->header.sql_desc_bind_offset_ptr;
		break;
	case SQL_ATTR_PARAM_BIND_TYPE:
		size = sizeof(stmt->apd->header.sql_desc_bind_type);
		src = &stmt->apd->header.sql_desc_bind_type;
		break;
	case SQL_ATTR_PARAM_OPERATION_PTR:
		size = sizeof(stmt->apd->header.sql_desc_array_status_ptr);
		src = &stmt->apd->header.sql_desc_array_status_ptr;
		break;
	case SQL_ATTR_PARAM_STATUS_PTR:
		size = sizeof(stmt->ipd->header.sql_desc_array_status_ptr);
		src = &stmt->ipd->header.sql_desc_array_status_ptr;
		break;
	case SQL_ATTR_PARAMS_PROCESSED_PTR:
		size = sizeof(stmt->ipd->header.sql_desc_rows_processed_ptr);
		src = &stmt->ipd->header.sql_desc_rows_processed_ptr;
		break;
	case SQL_ATTR_PARAMSET_SIZE:
		size = sizeof(stmt->apd->header.sql_desc_array_size);
		src = &stmt->apd->header.sql_desc_array_size;
		break;
	case SQL_ATTR_QUERY_TIMEOUT:
		size = sizeof(stmt->attr.query_timeout);
		src = &stmt->attr.query_timeout;
		break;
	case SQL_ATTR_RETRIEVE_DATA:
		size = sizeof(stmt->attr.retrieve_data);
		src = &stmt->attr.retrieve_data;
		break;
	case SQL_ATTR_ROW_BIND_OFFSET_PTR:
		size = sizeof(stmt->ard->header.sql_desc_bind_offset_ptr);
		src = &stmt->ard->header.sql_desc_bind_offset_ptr;
		break;
#if SQL_BIND_TYPE != SQL_ATTR_ROW_BIND_TYPE
	case SQL_BIND_TYPE:	/* although this is ODBC2 we must support this attribute */
#endif
	case SQL_ATTR_ROW_BIND_TYPE:
		size = sizeof(stmt->ard->header.sql_desc_bind_type);
		src = &stmt->ard->header.sql_desc_bind_type;
		break;
	case SQL_ATTR_ROW_NUMBER:
		/* TODO do not get info every time, cache somewhere */
		if (stmt->cursor && odbc_lock_statement(stmt)) {
			TDS_UINT row_number, row_count;

			tds_cursor_get_cursor_info(stmt->tds, stmt->cursor, &row_number, &row_count);
			stmt->attr.row_number = row_number;
		}
		size = sizeof(stmt->attr.row_number);
		src = &stmt->attr.row_number;
		break;
	case SQL_ATTR_ROW_OPERATION_PTR:
		size = sizeof(stmt->ard->header.sql_desc_array_status_ptr);
		src = &stmt->ard->header.sql_desc_array_status_ptr;
		break;
	case SQL_ATTR_ROW_STATUS_PTR:
		size = sizeof(stmt->ird->header.sql_desc_array_status_ptr);
		src = &stmt->ird->header.sql_desc_array_status_ptr;
		break;
	case SQL_ATTR_ROWS_FETCHED_PTR:
		size = sizeof(stmt->ird->header.sql_desc_rows_processed_ptr);
		src = &stmt->ird->header.sql_desc_rows_processed_ptr;
		break;
	case SQL_ATTR_ROW_ARRAY_SIZE:
		size = sizeof(stmt->ard->header.sql_desc_array_size);
		src = &stmt->ard->header.sql_desc_array_size;
		break;
	case SQL_ATTR_SIMULATE_CURSOR:
		size = sizeof(stmt->attr.simulate_cursor);
		src = &stmt->attr.simulate_cursor;
		break;
	case SQL_ATTR_USE_BOOKMARKS:
		size = sizeof(stmt->attr.use_bookmarks);
		src = &stmt->attr.use_bookmarks;
		break;
	case SQL_ATTR_CURSOR_SCROLLABLE:
		size = sizeof(stmt->attr.cursor_scrollable);
		src = &stmt->attr.cursor_scrollable;
		break;
	case SQL_ATTR_CURSOR_SENSITIVITY:
		size = sizeof(stmt->attr.cursor_sensitivity);
		src = &stmt->attr.cursor_sensitivity;
		break;
	case SQL_ATTR_IMP_ROW_DESC:
		size = sizeof(stmt->ird);
		src = &stmt->ird;
		break;
	case SQL_ATTR_IMP_PARAM_DESC:
		size = sizeof(stmt->ipd);
		src = &stmt->ipd;
		break;
	case SQL_ROWSET_SIZE:	/* although this is ODBC2 we must support this attribute */
		size = sizeof(stmt->sql_rowset_size);
		src = &stmt->sql_rowset_size;
		break;
	case SQL_SOPT_SS_QUERYNOTIFICATION_TIMEOUT:
		size = sizeof(stmt->attr.qn_timeout);
		src = &stmt->attr.qn_timeout;
		break;
	case SQL_SOPT_SS_QUERYNOTIFICATION_MSGTEXT:
		{
			SQLRETURN rc = odbc_set_string_oct(stmt->dbc, Value, BufferLength, StringLength, tds_dstr_cstr(&stmt->attr.qn_msgtext), tds_dstr_len(&stmt->attr.qn_msgtext));
			ODBC_EXIT(stmt, rc);
		}
	case SQL_SOPT_SS_QUERYNOTIFICATION_OPTIONS:
		{
			SQLRETURN rc = odbc_set_string_oct(stmt->dbc, Value, BufferLength, StringLength, tds_dstr_cstr(&stmt->attr.qn_options), tds_dstr_len(&stmt->attr.qn_options));
			ODBC_EXIT(stmt, rc);
		}
		/* TODO SQL_COLUMN_SEARCHABLE, although ODBC2 */
	default:
		odbc_errs_add(&stmt->errs, "HY092", NULL);
		ODBC_EXIT_(stmt);
	}

	memcpy(Value, src, size);
	if (StringLength)
		*StringLength = size;

	ODBC_EXIT_(stmt);
}

#if (ODBCVER >= 0x0300)
SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLGetStmtAttr(%p, %d, %p, %d, %p)\n", 
			hstmt, (int)Attribute, Value, (int)BufferLength, StringLength);

	return _SQLGetStmtAttr(hstmt, Attribute, Value, BufferLength, StringLength _wide0);
}

#ifdef ENABLE_ODBC_WIDE
SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetStmtAttrW(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLGetStmtAttr(%p, %d, %p, %d, %p)\n",
			hstmt, (int)Attribute, Value, (int)BufferLength, StringLength);

	return _SQLGetStmtAttr(hstmt, Attribute, Value, BufferLength, StringLength, 1);
}
#endif
#endif

SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetStmtOption(SQLHSTMT hstmt, SQLUSMALLINT fOption, SQLPOINTER pvParam)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLGetStmtOption(%p, %d, %p)\n", 
			hstmt, fOption, pvParam);

	return _SQLGetStmtAttr(hstmt, (SQLINTEGER) fOption, pvParam, SQL_MAX_OPTION_STRING_LENGTH, NULL _wide0);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLNumResultCols(SQLHSTMT hstmt, SQLSMALLINT FAR * pccol)
{
	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLNumResultCols(%p, %p)\n", 
			hstmt, pccol);

	/*
	 * 3/15/2001 bsb - DBD::ODBC calls SQLNumResultCols on non-result
	 * generating queries such as 'drop table'
	 */
#if 0
	if (stmt->row_status == NOT_IN_ROW) {
		odbc_errs_add(&stmt->errs, "24000", NULL);
		ODBC_EXIT_(stmt);
	}
#endif
	IRD_UPDATE(stmt->ird, &stmt->errs, ODBC_EXIT(stmt, SQL_ERROR));
	*pccol = stmt->ird->header.sql_desc_count;
	ODBC_EXIT_(stmt);
}

ODBC_FUNC(SQLPrepare, (P(SQLHSTMT,hstmt), PCHARIN(SqlStr,SQLINTEGER) WIDE))
{
	SQLRETURN retcode;

	ODBC_ENTER_HSTMT;

	/* try to free dynamic associated */
	retcode = odbc_free_dynamic(stmt);
	if (retcode != SQL_SUCCESS)
		ODBC_EXIT(stmt, retcode);

	if (SQL_SUCCESS != odbc_set_stmt_prepared_query(stmt, szSqlStr, cbSqlStr _wide))
		ODBC_EXIT(stmt, SQL_ERROR);

	/* count parameters */
	stmt->param_count = tds_count_placeholders(stmt->prepared_query);

	/* trasform to native (one time, not for every SQLExecute) */
	if (SQL_SUCCESS != prepare_call(stmt))
		ODBC_EXIT(stmt, SQL_ERROR);

	/* TODO needed ?? */
	tds_release_dynamic(&stmt->dyn);

	/* try to prepare query */
	/* TODO support getting info for RPC */
	if (!stmt->prepared_query_is_rpc
		 && (stmt->attr.cursor_type == SQL_CURSOR_FORWARD_ONLY && stmt->attr.concurrency == SQL_CONCUR_READ_ONLY)) {

		tds_free_param_results(stmt->params);
		stmt->params = NULL;
		stmt->param_num = 0;
		stmt->need_reprepare = 0;
		/*
		 * using TDS7+ we need parameters to prepare a query so try
		 * to get them 
		 * TDS5 do not need parameters type and we have always to
		 * prepare sepatarely so this is not an issue
		 */
		if (IS_TDS7_PLUS(stmt->dbc->tds_socket->conn)) {
			stmt->need_reprepare = 1;
			ODBC_EXIT_(stmt);
		}

		tdsdump_log(TDS_DBG_INFO1, "Creating prepared statement\n");
		if (odbc_lock_statement(stmt))
			odbc_prepare(stmt);
	}

	ODBC_EXIT_(stmt);
}

/* TDS_NO_COUNT must be -1 for SQLRowCount to return -1 when there is no rowcount */
#if TDS_NO_COUNT != -1
# error TDS_NO_COUNT != -1
#endif
	
SQLRETURN
_SQLRowCount(SQLHSTMT hstmt, SQLLEN FAR * pcrow)
{
	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "_SQLRowCount(%p, %p),  %ld rows \n", hstmt, pcrow, (long)stmt->row_count);

	*pcrow = stmt->row_count;

	ODBC_EXIT_(stmt);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLRowCount(SQLHSTMT hstmt, SQLLEN FAR * pcrow)
{
	SQLRETURN rc = _SQLRowCount(hstmt, pcrow);
	tdsdump_log(TDS_DBG_INFO1, "SQLRowCount returns %d, row count %ld\n", rc, (long int) *pcrow);
	return rc;
}

ODBC_FUNC(SQLSetCursorName, (P(SQLHSTMT,hstmt), PCHARIN(Cursor,SQLSMALLINT) WIDE))
{
	ODBC_ENTER_HSTMT;

	/* cursor already present, we cannot set name */
	if (stmt->cursor) {
		odbc_errs_add(&stmt->errs, "24000", NULL);
		ODBC_EXIT_(stmt);
	}

	if (!odbc_dstr_copy(stmt->dbc, &stmt->cursor_name, cbCursor, szCursor)) {
		odbc_errs_add(&stmt->errs, "HY001", NULL);
		ODBC_EXIT_(stmt);
	}
	ODBC_EXIT_(stmt);
}

ODBC_FUNC(SQLGetCursorName, (P(SQLHSTMT,hstmt), PCHAROUT(Cursor,SQLSMALLINT) WIDE))
#include "sqlwparams.h"
{
	SQLRETURN rc;

	ODBC_ENTER_HSTMT;

	if ((rc = odbc_set_string(stmt->dbc, szCursor, cbCursorMax, pcbCursor, tds_dstr_cstr(&stmt->cursor_name), -1)))
		odbc_errs_add(&stmt->errs, "01004", NULL);

	ODBC_EXIT(stmt, rc);
}

/*
 * spinellia@acm.org : copied shamelessly from change_database
 * transaction support
 * 1 = commit, 0 = rollback
 */
static SQLRETURN
change_transaction(TDS_DBC * dbc, int state)
{
	TDSSOCKET *tds = dbc->tds_socket;
	int cont;
	TDSRET ret;

	tdsdump_log(TDS_DBG_INFO1, "change_transaction(0x%p,%d)\n", dbc, state);

	if (dbc->attr.autocommit == SQL_AUTOCOMMIT_ON)
		cont = 0;
	else
		cont = 1;

	/* if pending drop all recordset, don't issue cancel */
	if (tds->state == TDS_PENDING && dbc->current_statement != NULL) {
		/* TODO what happen on multiple errors ?? discard all ?? */
		if (TDS_FAILED(tds_process_simple_query(tds)))
			return SQL_ERROR;
	}

	/* TODO better idle check, not thread safe */
	if (tds->state == TDS_IDLE)
		tds->query_timeout = dbc->default_query_timeout;

	if (state)
		ret = tds_submit_commit(tds, cont);
	else
		ret = tds_submit_rollback(tds, cont);

	if (TDS_FAILED(ret)) {
		odbc_errs_add(&dbc->errs, "HY000", "Could not perform COMMIT or ROLLBACK");
		return SQL_ERROR;
	}

	if (TDS_FAILED(tds_process_simple_query(tds)))
		return SQL_ERROR;

	/* TODO some query can collect errors so perhaps it's better to return SUCCES_WITH_INFO in such case... */
	return SQL_SUCCESS;
}

static SQLRETURN
_SQLTransact(SQLHENV henv, SQLHDBC hdbc, SQLUSMALLINT fType)
{
	int op = (fType == SQL_COMMIT ? 1 : 0);

	/* I may live without a HENV */
	/*     CHECK_HENV; */
	/* ..but not without a HDBC! */
	ODBC_ENTER_HDBC;

	tdsdump_log(TDS_DBG_FUNC, "_SQLTransact(%p, %p, %d)\n", 
			henv, hdbc, fType);

	ODBC_EXIT(dbc, change_transaction(dbc, op));
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLTransact(SQLHENV henv, SQLHDBC hdbc, SQLUSMALLINT fType)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLTransact(%p, %p, %d)\n", henv, hdbc, fType);

	return _SQLTransact(henv, hdbc, fType);
}

#if ODBCVER >= 0x300
SQLRETURN ODBC_PUBLIC ODBC_API
SQLEndTran(SQLSMALLINT handleType, SQLHANDLE handle, SQLSMALLINT completionType)
{
	/*
	 * Do not call the exported SQLTransact(),
	 * because we may wind up calling a function with the same name implemented by the DM.
	 */
	tdsdump_log(TDS_DBG_FUNC, "SQLEndTran(%d, %p, %d)\n", handleType, handle, completionType);

	switch (handleType) {
	case SQL_HANDLE_ENV:
		return _SQLTransact(handle, NULL, completionType);
	case SQL_HANDLE_DBC:
		return _SQLTransact(NULL, handle, completionType);
	}
	return SQL_ERROR;
}
#endif

/* end of transaction support */

SQLRETURN ODBC_PUBLIC ODBC_API
SQLSetParam(SQLHSTMT hstmt, SQLUSMALLINT ipar, SQLSMALLINT fCType, SQLSMALLINT fSqlType, SQLULEN cbParamDef,
	    SQLSMALLINT ibScale, SQLPOINTER rgbValue, SQLLEN FAR * pcbValue)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLSetParam(%p, %d, %d, %d, %u, %d, %p, %p)\n", 
			hstmt, ipar, fCType, fSqlType, (unsigned)cbParamDef, ibScale, rgbValue, pcbValue);

	return _SQLBindParameter(hstmt, ipar, SQL_PARAM_INPUT_OUTPUT, fCType, fSqlType, cbParamDef, ibScale, rgbValue,
				 SQL_SETPARAM_VALUE_MAX, pcbValue);
}

/**
 * SQLColumns
 *
 * Return column information for a table or view. This is
 * mapped to a call to sp_columns which - lucky for us - returns
 * the exact result set we need.
 *
 * exec sp_columns [ @table_name = ] object 
 *                 [ , [ @table_owner = ] owner ] 
 *                 [ , [ @table_qualifier = ] qualifier ] 
 *                 [ , [ @column_name = ] column ] 
 *                 [ , [ @ODBCVer = ] ODBCVer ] 
 *
 */
ODBC_FUNC(SQLColumns, (P(SQLHSTMT,hstmt), PCHARIN(CatalogName,SQLSMALLINT),	/* object_qualifier */
	PCHARIN(SchemaName,SQLSMALLINT),	/* object_owner */
	PCHARIN(TableName,SQLSMALLINT),	/* object_name */
	PCHARIN(ColumnName,SQLSMALLINT)	/* column_name */
	WIDE))
#include "sqlwparams.h"
{
	int retcode;

	ODBC_ENTER_HSTMT;

	retcode =
		odbc_stat_execute(stmt _wide, "sp_columns", TDS_IS_MSSQL(stmt->dbc->tds_socket) ? 5 : 4,
				  "P@table_name", szTableName, cbTableName, "P@table_owner", szSchemaName,
				  cbSchemaName, "O@table_qualifier", szCatalogName, cbCatalogName, "P@column_name", szColumnName,
				  cbColumnName, "V@ODBCVer", (char*) NULL, 0);
	if (SQL_SUCCEEDED(retcode) && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) {
		odbc_col_setname(stmt, 1, "TABLE_CAT");
		odbc_col_setname(stmt, 2, "TABLE_SCHEM");
		odbc_col_setname(stmt, 7, "COLUMN_SIZE");
		odbc_col_setname(stmt, 8, "BUFFER_LENGTH");
		odbc_col_setname(stmt, 9, "DECIMAL_DIGITS");
		odbc_col_setname(stmt, 10, "NUM_PREC_RADIX");
		if (TDS_IS_SYBASE(stmt->dbc->tds_socket))
			stmt->special_row = ODBC_SPECIAL_COLUMNS;
	}
	ODBC_EXIT_(stmt);
}

ODBC_FUNC(SQLGetConnectAttr, (P(SQLHDBC,hdbc), P(SQLINTEGER,Attribute), P(SQLPOINTER,Value), P(SQLINTEGER,BufferLength),
	P(SQLINTEGER *,StringLength) WIDE))
#include "sqlwparams.h"
{
	const char *p = NULL;

	ODBC_ENTER_HDBC;

	tdsdump_log(TDS_DBG_FUNC, "_SQLGetConnectAttr(%p, %d, %p, %d, %p)\n", 
			hdbc, (int)Attribute, Value, (int)BufferLength, StringLength);

	switch (Attribute) {
	case SQL_ATTR_AUTOCOMMIT:
		*((SQLUINTEGER *) Value) = dbc->attr.autocommit;
		break;
#if defined(SQL_ATTR_CONNECTION_DEAD) && defined(SQL_CD_TRUE)
	case SQL_ATTR_CONNECTION_DEAD:
		*((SQLUINTEGER *) Value) = IS_TDSDEAD(dbc->tds_socket) ? SQL_CD_TRUE : SQL_CD_FALSE;
		break;
#endif
	case SQL_ATTR_CONNECTION_TIMEOUT:
		*((SQLUINTEGER *) Value) = dbc->attr.connection_timeout;
		break;
	case SQL_ATTR_ACCESS_MODE:
		*((SQLUINTEGER *) Value) = dbc->attr.access_mode;
		break;
	case SQL_ATTR_CURRENT_CATALOG:
		p = tds_dstr_cstr(&dbc->attr.current_catalog);
		break;
	case SQL_ATTR_LOGIN_TIMEOUT:
		*((SQLUINTEGER *) Value) = dbc->attr.login_timeout;
		break;
	case SQL_ATTR_ODBC_CURSORS:
		*((SQLUINTEGER *) Value) = dbc->attr.odbc_cursors;
		break;
	case SQL_ATTR_PACKET_SIZE:
		*((SQLUINTEGER *) Value) = dbc->attr.packet_size;
		break;
	case SQL_ATTR_QUIET_MODE:
		*((SQLHWND *) Value) = dbc->attr.quite_mode;
		break;
#ifdef TDS_NO_DM
	case SQL_ATTR_TRACE:
		*((SQLUINTEGER *) Value) = dbc->attr.trace;
		break;
	case SQL_ATTR_TRACEFILE:
		p = tds_dstr_cstr(&dbc->attr.tracefile);
		break;
#endif
	case SQL_ATTR_TXN_ISOLATION:
		*((SQLUINTEGER *) Value) = dbc->attr.txn_isolation;
		break;
	case SQL_COPT_SS_MARS_ENABLED:
		*((SQLUINTEGER *) Value) = dbc->attr.mars_enabled;
		break;
	case SQL_ATTR_TRANSLATE_LIB:
	case SQL_ATTR_TRANSLATE_OPTION:
		odbc_errs_add(&dbc->errs, "HYC00", NULL);
		break;
	default:
		odbc_errs_add(&dbc->errs, "HY092", NULL);
		break;
	}

	if (p) {
		SQLRETURN rc = odbc_set_string_oct(dbc, Value, BufferLength, StringLength, p, -1);
		ODBC_EXIT(dbc, rc);
	}
	ODBC_EXIT_(dbc);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetConnectOption(SQLHDBC hdbc, SQLUSMALLINT fOption, SQLPOINTER pvParam)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLGetConnectOption(%p, %u, %p)\n", hdbc, fOption, pvParam);

	return _SQLGetConnectAttr(hdbc, (SQLINTEGER) fOption, pvParam, SQL_MAX_OPTION_STRING_LENGTH, NULL _wide0);
}

#ifdef ENABLE_ODBC_WIDE
SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetConnectOptionW(SQLHDBC hdbc, SQLUSMALLINT fOption, SQLPOINTER pvParam)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLGetConnectOptionW(%p, %u, %p)\n", hdbc, fOption, pvParam);

	return _SQLGetConnectAttr(hdbc, (SQLINTEGER) fOption, pvParam, SQL_MAX_OPTION_STRING_LENGTH, NULL, 1);
}
#endif

SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetData(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLSMALLINT fCType, SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN FAR * pcbValue)
{
	/* TODO cursors fetch row if needed ?? */
	TDSCOLUMN *colinfo;
	TDSRESULTINFO *resinfo;
	SQLLEN dummy_cb;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLGetData(%p, %u, %d, %p, %d, %p)\n", 
			hstmt, icol, fCType, rgbValue, (int)cbValueMax, pcbValue);

	if (cbValueMax < 0) {
		odbc_errs_add(&stmt->errs, "HY090", NULL);
		ODBC_EXIT_(stmt);
	}

	/* read data from TDS only if current statement */
	if ((stmt->cursor == NULL && !stmt->tds)
		|| stmt->row_status == PRE_NORMAL_ROW 
		|| stmt->row_status == NOT_IN_ROW) 
	{
		odbc_errs_add(&stmt->errs, "24000", NULL);
		ODBC_EXIT_(stmt);
	}

	IRD_CHECK;

	if (!pcbValue)
		pcbValue = &dummy_cb;

	resinfo = stmt->cursor ? stmt->cursor->res_info : stmt->tds->current_results;
	if (!resinfo) {
		odbc_errs_add(&stmt->errs, "HY010", NULL);
		ODBC_EXIT_(stmt);
	}
	if (icol <= 0 || icol > resinfo->num_cols) {
		odbc_errs_add(&stmt->errs, "07009", "Column out of range");
		ODBC_EXIT_(stmt);
	}
	colinfo = resinfo->columns[icol - 1];

	if (colinfo->column_cur_size < 0) {
		/* TODO check what should happen if pcbValue was NULL */
		*pcbValue = SQL_NULL_DATA;
	} else {
		TDS_CHAR *src;
		int srclen;
		int nSybType;

		if (colinfo->column_text_sqlgetdatapos > 0
		    && colinfo->column_text_sqlgetdatapos >= colinfo->column_cur_size)
			ODBC_EXIT(stmt, SQL_NO_DATA);

		src = (TDS_CHAR *) colinfo->column_data;
		srclen = colinfo->column_cur_size;
		if (!is_variable_type(colinfo->column_type))
			colinfo->column_text_sqlgetdatapos = 0;

		nSybType = tds_get_conversion_type(colinfo->on_server.column_type, colinfo->on_server.column_size);
		if (fCType == SQL_C_DEFAULT)
			fCType = odbc_sql_to_c_type_default(stmt->ird->records[icol - 1].sql_desc_concise_type);
		if (fCType == SQL_ARD_TYPE) {
			if (icol > stmt->ard->header.sql_desc_count) {
				odbc_errs_add(&stmt->errs, "07009", NULL);
				ODBC_EXIT_(stmt);
			}
			fCType = stmt->ard->records[icol - 1].sql_desc_concise_type;
		}
		assert(fCType);

		*pcbValue = odbc_tds2sql(stmt, colinfo, nSybType, src, srclen, fCType, (TDS_CHAR *) rgbValue, cbValueMax, NULL);
		if (*pcbValue == SQL_NULL_DATA)
			ODBC_EXIT(stmt, SQL_ERROR);
		
		if (is_variable_type(colinfo->column_type) && (fCType == SQL_C_CHAR || fCType == SQL_C_WCHAR || fCType == SQL_C_BINARY)) {
			/* avoid infinite SQL_SUCCESS on empty strings */
			if (colinfo->column_text_sqlgetdatapos == 0 && cbValueMax > 0)
				++colinfo->column_text_sqlgetdatapos;

			if (colinfo->column_text_sqlgetdatapos < colinfo->column_cur_size) {	/* not all read ?? */
				odbc_errs_add(&stmt->errs, "01004", "String data, right truncated");
				ODBC_EXIT_(stmt);
			}
		} else {
			colinfo->column_text_sqlgetdatapos = colinfo->column_cur_size;
			if (is_fixed_type(nSybType) && (fCType == SQL_C_CHAR || fCType == SQL_C_WCHAR || fCType == SQL_C_BINARY)
			    && cbValueMax < *pcbValue) {
				odbc_errs_add(&stmt->errs, "22003", NULL);
				ODBC_EXIT_(stmt);
			}
		}
	}
	ODBC_EXIT_(stmt);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetFunctions(SQLHDBC hdbc, SQLUSMALLINT fFunction, SQLUSMALLINT FAR * pfExists)
{
	int i;

	ODBC_ENTER_HDBC;

	tdsdump_log(TDS_DBG_FUNC, "SQLGetFunctions: fFunction is %d\n", fFunction);
	switch (fFunction) {
#if (ODBCVER >= 0x0300)
	case SQL_API_ODBC3_ALL_FUNCTIONS:
		for (i = 0; i < SQL_API_ODBC3_ALL_FUNCTIONS_SIZE; ++i) {
			pfExists[i] = 0;
		}

		/*
		 * every api available are contained in a macro
		 * all these macro begin with API followed by 2 letter
		 * first letter mean pre ODBC 3 (_) or ODBC 3 (3)
		 * second letter mean implemented (X) or unimplemented (_)
		 * You should copy these macro 3 times... not very good
		 * but works. Perhaps best method is build the bit array statically
		 * and then use it but I don't know how to build it...
		 */
#undef ODBC_ALL_API
#undef ODBC_COLATTRIBUTE

#if SQL_API_SQLCOLATTRIBUTE != SQL_API_SQLCOLATTRIBUTES
#define ODBC_COLATTRIBUTE(s) s
#else
#define ODBC_COLATTRIBUTE(s)
#endif

#define ODBC_ALL_API \
	API_X(SQL_API_SQLALLOCCONNECT);\
	API_X(SQL_API_SQLALLOCENV);\
	API3X(SQL_API_SQLALLOCHANDLE);\
	API_X(SQL_API_SQLALLOCSTMT);\
	API_X(SQL_API_SQLBINDCOL);\
	API_X(SQL_API_SQLBINDPARAM);\
	API_X(SQL_API_SQLBINDPARAMETER);\
	API__(SQL_API_SQLBROWSECONNECT);\
	API3_(SQL_API_SQLBULKOPERATIONS);\
	API_X(SQL_API_SQLCANCEL);\
	API3X(SQL_API_SQLCLOSECURSOR);\
	ODBC_COLATTRIBUTE(API3X(SQL_API_SQLCOLATTRIBUTE);)\
	API_X(SQL_API_SQLCOLATTRIBUTES);\
	API_X(SQL_API_SQLCOLUMNPRIVILEGES);\
	API_X(SQL_API_SQLCOLUMNS);\
	API_X(SQL_API_SQLCONNECT);\
	API3X(SQL_API_SQLCOPYDESC);\
	API_X(SQL_API_SQLDESCRIBECOL);\
	API__(SQL_API_SQLDESCRIBEPARAM);\
	API_X(SQL_API_SQLDISCONNECT);\
	API_X(SQL_API_SQLDRIVERCONNECT);\
	API3X(SQL_API_SQLENDTRAN);\
	API_X(SQL_API_SQLERROR);\
	API_X(SQL_API_SQLEXECDIRECT);\
	API_X(SQL_API_SQLEXECUTE);\
	API_X(SQL_API_SQLEXTENDEDFETCH);\
	API_X(SQL_API_SQLFETCH);\
	API3X(SQL_API_SQLFETCHSCROLL);\
	API_X(SQL_API_SQLFOREIGNKEYS);\
	API_X(SQL_API_SQLFREECONNECT);\
	API_X(SQL_API_SQLFREEENV);\
	API3X(SQL_API_SQLFREEHANDLE);\
	API_X(SQL_API_SQLFREESTMT);\
	API3X(SQL_API_SQLGETCONNECTATTR);\
	API_X(SQL_API_SQLGETCONNECTOPTION);\
	API_X(SQL_API_SQLGETCURSORNAME);\
	API_X(SQL_API_SQLGETDATA);\
	API3X(SQL_API_SQLGETDESCFIELD);\
	API3X(SQL_API_SQLGETDESCREC);\
	API3X(SQL_API_SQLGETDIAGFIELD);\
	API3X(SQL_API_SQLGETDIAGREC);\
	API3X(SQL_API_SQLGETENVATTR);\
	API_X(SQL_API_SQLGETFUNCTIONS);\
	API_X(SQL_API_SQLGETINFO);\
	API3X(SQL_API_SQLGETSTMTATTR);\
	API_X(SQL_API_SQLGETSTMTOPTION);\
	API_X(SQL_API_SQLGETTYPEINFO);\
	API_X(SQL_API_SQLMORERESULTS);\
	API_X(SQL_API_SQLNATIVESQL);\
	API_X(SQL_API_SQLNUMPARAMS);\
	API_X(SQL_API_SQLNUMRESULTCOLS);\
	API_X(SQL_API_SQLPARAMDATA);\
	API_X(SQL_API_SQLPARAMOPTIONS);\
	API_X(SQL_API_SQLPREPARE);\
	API_X(SQL_API_SQLPRIMARYKEYS);\
	API_X(SQL_API_SQLPROCEDURECOLUMNS);\
	API_X(SQL_API_SQLPROCEDURES);\
	API_X(SQL_API_SQLPUTDATA);\
	API_X(SQL_API_SQLROWCOUNT);\
	API3X(SQL_API_SQLSETCONNECTATTR);\
	API_X(SQL_API_SQLSETCONNECTOPTION);\
	API_X(SQL_API_SQLSETCURSORNAME);\
	API3X(SQL_API_SQLSETDESCFIELD);\
	API3X(SQL_API_SQLSETDESCREC);\
	API3X(SQL_API_SQLSETENVATTR);\
	API_X(SQL_API_SQLSETPARAM);\
	API_X(SQL_API_SQLSETPOS);\
	API_X(SQL_API_SQLSETSCROLLOPTIONS);\
	API3X(SQL_API_SQLSETSTMTATTR);\
	API_X(SQL_API_SQLSETSTMTOPTION);\
	API_X(SQL_API_SQLSPECIALCOLUMNS);\
	API_X(SQL_API_SQLSTATISTICS);\
	API_X(SQL_API_SQLTABLEPRIVILEGES);\
	API_X(SQL_API_SQLTABLES);\
	API_X(SQL_API_SQLTRANSACT);

#define API_X(n) if (n >= 0 && n < (16*SQL_API_ODBC3_ALL_FUNCTIONS_SIZE)) pfExists[n/16] |= (1 << n%16);
#define API__(n)
#define API3X(n) if (n >= 0 && n < (16*SQL_API_ODBC3_ALL_FUNCTIONS_SIZE)) pfExists[n/16] |= (1 << n%16);
#define API3_(n)
		ODBC_ALL_API
#undef API_X
#undef API__
#undef API3X
#undef API3_
		break;
#endif

	case SQL_API_ALL_FUNCTIONS:
		tdsdump_log(TDS_DBG_FUNC, "SQLGetFunctions: " "fFunction is SQL_API_ALL_FUNCTIONS\n");
		for (i = 0; i < 100; ++i) {
			pfExists[i] = SQL_FALSE;
		}

#define API_X(n) if (n >= 0 && n < 100) pfExists[n] = SQL_TRUE;
#define API__(n)
#define API3X(n)
#define API3_(n)
		ODBC_ALL_API
#undef API_X
#undef API__
#undef API3X
#undef API3_
		break;
#define API_X(n) case n:
#define API__(n)
#if (ODBCVER >= 0x300)
#define API3X(n) case n:
#else
#define API3X(n)
#endif
#define API3_(n)
		ODBC_ALL_API
#undef API_X
#undef API__
#undef API3X
#undef API3_
		*pfExists = SQL_TRUE;
		break;
	default:
		*pfExists = SQL_FALSE;
		break;
	}
	ODBC_EXIT(dbc, SQL_SUCCESS);
#undef ODBC_ALL_API
}


static SQLRETURN
_SQLGetInfo(TDS_DBC * dbc, SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue, SQLSMALLINT cbInfoValueMax,
	    SQLSMALLINT FAR * pcbInfoValue _WIDE)
{
	const char *p = NULL;
	char buf[32];
	TDSSOCKET *tds;
	int is_ms = -1;
	unsigned int smajor = 6;
	SQLUINTEGER mssql7plus_mask = 0;
	int out_len = -1;

	tdsdump_log(TDS_DBG_FUNC, "_SQLGetInfo(%p, %u, %p, %d, %p)\n",
			dbc, fInfoType, rgbInfoValue, cbInfoValueMax, pcbInfoValue);

#define SIVAL out_len = sizeof(SQLSMALLINT), *((SQLSMALLINT *) rgbInfoValue)
#define USIVAL out_len = sizeof(SQLUSMALLINT), *((SQLUSMALLINT *) rgbInfoValue)
#define IVAL out_len = sizeof(SQLINTEGER), *((SQLINTEGER *) rgbInfoValue)
#define UIVAL out_len = sizeof(SQLUINTEGER), *((SQLUINTEGER *) rgbInfoValue)
#define ULVAL out_len = sizeof(SQLULEN), *((SQLULEN *) rgbInfoValue)

	if ((tds = dbc->tds_socket) != NULL) {
		is_ms = TDS_IS_MSSQL(tds);
		smajor = (tds_conn(tds)->product_version >> 24) & 0x7F;
		if (is_ms && smajor >= 7)
			mssql7plus_mask = ~((SQLUINTEGER) 0);
	}

	switch (fInfoType) {
	case SQL_ACCESSIBLE_PROCEDURES:
	case SQL_ACCESSIBLE_TABLES:
		p = "Y";
		break;
		/* SQL_ACTIVE_CONNECTIONS renamed to SQL_MAX_DRIVER_CONNECTIONS */
#if (ODBCVER >= 0x0300)
	case SQL_ACTIVE_ENVIRONMENTS:
		UIVAL = 0;
		break;
#endif /* ODBCVER >= 0x0300 */
#if (ODBCVER >= 0x0300)
	case SQL_AGGREGATE_FUNCTIONS:
		UIVAL = SQL_AF_ALL;
		break;
	case SQL_ALTER_DOMAIN:
		UIVAL = 0;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_ALTER_TABLE:
		UIVAL = SQL_AT_ADD_COLUMN | SQL_AT_ADD_COLUMN_DEFAULT | SQL_AT_ADD_COLUMN_SINGLE | SQL_AT_ADD_CONSTRAINT |
			SQL_AT_ADD_TABLE_CONSTRAINT | SQL_AT_CONSTRAINT_NAME_DEFINITION | SQL_AT_DROP_COLUMN_RESTRICT;
		break;
#if (ODBCVER >= 0x0300)
	case SQL_ASYNC_MODE:
		/* TODO we hope so in a not-too-far future */
		/* UIVAL = SQL_AM_STATEMENT; */
		UIVAL = SQL_AM_NONE;
		break;
	case SQL_BATCH_ROW_COUNT:
		UIVAL = SQL_BRC_EXPLICIT;
		break;
	case SQL_BATCH_SUPPORT:
		UIVAL = SQL_BS_ROW_COUNT_EXPLICIT | SQL_BS_ROW_COUNT_PROC | SQL_BS_SELECT_EXPLICIT | SQL_BS_SELECT_PROC;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_BOOKMARK_PERSISTENCE:
		/* TODO ??? */
		UIVAL = SQL_BP_DELETE | SQL_BP_SCROLL | SQL_BP_UPDATE;
		break;
	case SQL_CATALOG_LOCATION:
		SIVAL = SQL_CL_START;
		break;
#if (ODBCVER >= 0x0300)
	case SQL_CATALOG_NAME:
		p = "Y";
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_CATALOG_NAME_SEPARATOR:
		p = ".";
		break;
	case SQL_CATALOG_TERM:
		p = "database";
		break;
	case SQL_CATALOG_USAGE:
		UIVAL = SQL_CU_DML_STATEMENTS | SQL_CU_PROCEDURE_INVOCATION | SQL_CU_TABLE_DEFINITION;
		break;
		/* TODO */
#if 0
	case SQL_COLLATION_SEQ:
		break;
#endif
	case SQL_COLUMN_ALIAS:
		p = "Y";
		break;
	case SQL_CONCAT_NULL_BEHAVIOR:
		if (is_ms == -1)
			return SQL_ERROR;
		/* TODO a bit more complicate for mssql2k.. */
		SIVAL = (!is_ms || smajor < 7) ? SQL_CB_NON_NULL : SQL_CB_NULL;
		break;
		/* TODO SQL_CONVERT_BIGINT SQL_CONVERT_GUID SQL_CONVERT_DATE SQL_CONVERT_TIME */
		/* For Sybase/MSSql6.x we return 0 cause NativeSql is not so implemented */
	case SQL_CONVERT_BINARY:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_VARCHAR |
			 SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_TINYINT | SQL_CVT_LONGVARBINARY | SQL_CVT_WCHAR |
			 SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_BIT:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_BIT | SQL_CVT_TINYINT |
			 SQL_CVT_WCHAR | SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_CHAR:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_LONGVARCHAR | SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_BIT |
			 SQL_CVT_TINYINT | SQL_CVT_TIMESTAMP | SQL_CVT_LONGVARBINARY | SQL_CVT_WCHAR | SQL_CVT_WLONGVARCHAR |
			 SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_DECIMAL:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_BIT | SQL_CVT_TINYINT |
			 SQL_CVT_WCHAR | SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_FLOAT:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_BIT | SQL_CVT_TINYINT | SQL_CVT_WCHAR | SQL_CVT_WVARCHAR) &
			mssql7plus_mask;
		break;
	case SQL_CONVERT_INTEGER:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_BIT | SQL_CVT_TINYINT |
			 SQL_CVT_WCHAR | SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_LONGVARCHAR:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_VARCHAR | SQL_CVT_LONGVARCHAR | SQL_CVT_WCHAR | SQL_CVT_WLONGVARCHAR |
			 SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_NUMERIC:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_BIT | SQL_CVT_TINYINT |
			 SQL_CVT_WCHAR | SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_REAL:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_BIT | SQL_CVT_TINYINT | SQL_CVT_WCHAR | SQL_CVT_WVARCHAR) &
			mssql7plus_mask;
		break;
	case SQL_CONVERT_SMALLINT:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_BIT | SQL_CVT_TINYINT |
			 SQL_CVT_WCHAR | SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_TIMESTAMP:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_VARCHAR | SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_TIMESTAMP | SQL_CVT_WCHAR |
			 SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_TINYINT:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_BIT | SQL_CVT_TINYINT |
			 SQL_CVT_WCHAR | SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_VARBINARY:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_VARCHAR |
			 SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_TINYINT | SQL_CVT_LONGVARBINARY | SQL_CVT_WCHAR |
			 SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_VARCHAR:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_LONGVARCHAR | SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_BIT |
			 SQL_CVT_TINYINT | SQL_CVT_TIMESTAMP | SQL_CVT_LONGVARBINARY | SQL_CVT_WCHAR | SQL_CVT_WLONGVARCHAR |
			 SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_LONGVARBINARY:
		UIVAL = (SQL_CVT_BINARY | SQL_CVT_LONGVARBINARY | SQL_CVT_VARBINARY) & mssql7plus_mask;
		break;
	case SQL_CONVERT_WLONGVARCHAR:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_VARCHAR | SQL_CVT_LONGVARCHAR | SQL_CVT_WCHAR | SQL_CVT_WLONGVARCHAR |
			 SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
#if (ODBCVER >= 0x0300)
	case SQL_CONVERT_WCHAR:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_LONGVARCHAR | SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_BIT |
			 SQL_CVT_TINYINT | SQL_CVT_TIMESTAMP | SQL_CVT_LONGVARBINARY | SQL_CVT_WCHAR | SQL_CVT_WLONGVARCHAR |
			 SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
	case SQL_CONVERT_WVARCHAR:
		UIVAL = (SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL | SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
			 SQL_CVT_REAL | SQL_CVT_VARCHAR | SQL_CVT_LONGVARCHAR | SQL_CVT_BINARY | SQL_CVT_VARBINARY | SQL_CVT_BIT |
			 SQL_CVT_TINYINT | SQL_CVT_TIMESTAMP | SQL_CVT_LONGVARBINARY | SQL_CVT_WCHAR | SQL_CVT_WLONGVARCHAR |
			 SQL_CVT_WVARCHAR) & mssql7plus_mask;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_CONVERT_FUNCTIONS:
		/* TODO no CAST for Sybase ?? */
		UIVAL = SQL_FN_CVT_CONVERT | SQL_FN_CVT_CAST;
		break;
#if (ODBCVER >= 0x0300)
	case SQL_CREATE_ASSERTION:
	case SQL_CREATE_CHARACTER_SET:
	case SQL_CREATE_COLLATION:
	case SQL_CREATE_DOMAIN:
		UIVAL = 0;
		break;
	case SQL_CREATE_SCHEMA:
		UIVAL = SQL_CS_AUTHORIZATION | SQL_CS_CREATE_SCHEMA;
		break;
	case SQL_CREATE_TABLE:
		UIVAL = SQL_CT_CREATE_TABLE;
		break;
	case SQL_CREATE_TRANSLATION:
		UIVAL = 0;
		break;
	case SQL_CREATE_VIEW:
		UIVAL = SQL_CV_CHECK_OPTION | SQL_CV_CREATE_VIEW;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_CORRELATION_NAME:
		USIVAL = SQL_CN_ANY;
		break;
	case SQL_CURSOR_COMMIT_BEHAVIOR:
		/* currently cursors are not supported however sql server close automaticly cursors on commit */
		/* TODO cursors test what happen if rollback, cursors get properly deleted ?? */
		USIVAL = SQL_CB_CLOSE;
		break;
	case SQL_CURSOR_ROLLBACK_BEHAVIOR:
		USIVAL = SQL_CB_CLOSE;
		break;
	case SQL_CURSOR_SENSITIVITY:
		UIVAL = SQL_SENSITIVE;
		break;
	case SQL_DATABASE_NAME:
		p = tds_dstr_cstr(&dbc->attr.current_catalog);
		break;
	case SQL_DATA_SOURCE_NAME:
		p = tds_dstr_cstr(&dbc->dsn);
		break;
	case SQL_DATA_SOURCE_READ_ONLY:
		/*
		 * TODO: determine the right answer from connection 
		 * attribute SQL_ATTR_ACCESS_MODE
		 */
		p = "N";	/* false, writable */
		break;
#if (ODBCVER >= 0x0300)
	case SQL_DATETIME_LITERALS:
		/* TODO ok ?? */
		UIVAL = 0;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_DBMS_NAME:
		p = tds ? tds_conn(tds)->product_name : NULL;
		break;
	case SQL_DBMS_VER:
		if (!dbc->tds_socket)
			return SQL_ERROR;
		odbc_rdbms_version(dbc->tds_socket, buf);
		p = buf;
		break;
#if (ODBCVER >= 0x0300)
	case SQL_DDL_INDEX:
		UIVAL = 0;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_DEFAULT_TXN_ISOLATION:
		UIVAL = SQL_TXN_READ_COMMITTED;
		break;
#if (ODBCVER >= 0x0300)
	case SQL_DESCRIBE_PARAMETER:
		/* TODO */
		p = "N";
		break;
#endif /* ODBCVER >= 0x0300 */
#ifdef TDS_NO_DM
	case SQL_DRIVER_HDBC:
		ULVAL = (SQLULEN) dbc;
		break;
	case SQL_DRIVER_HENV:
		ULVAL = (SQLULEN) dbc->env;
		break;
	case SQL_DRIVER_HSTMT:
		ULVAL = (SQLULEN) dbc->current_statement;
		break;
#endif
	case SQL_DRIVER_NAME:	/* ODBC 2.0 */
		p = "libtdsodbc.so";
		break;
	case SQL_DRIVER_ODBC_VER:
		p = "03.50";
		break;
	case SQL_DRIVER_VER:
		sprintf(buf, "%02d.%02d.%04d", TDS_VERSION_MAJOR,
			TDS_VERSION_MINOR, TDS_VERSION_SUBVERSION);
		p = buf;
		break;
#if (ODBCVER >= 0x0300)
	case SQL_DROP_ASSERTION:
	case SQL_DROP_CHARACTER_SET:
	case SQL_DROP_COLLATION:
	case SQL_DROP_DOMAIN:
	case SQL_DROP_SCHEMA:
		UIVAL = 0;
		break;
	case SQL_DROP_TABLE:
		UIVAL = SQL_DT_DROP_TABLE;
		break;
	case SQL_DROP_TRANSLATION:
		UIVAL = 0;
		break;
	case SQL_DROP_VIEW:
		UIVAL = SQL_DV_DROP_VIEW;
		break;
	case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
		if (dbc->cursor_support)
			/* TODO cursor SQL_CA1_BULK_ADD SQL_CA1_POS_REFRESH SQL_CA1_SELECT_FOR_UPDATE */
			UIVAL = SQL_CA1_ABSOLUTE | SQL_CA1_NEXT | SQL_CA1_RELATIVE |
				SQL_CA1_LOCK_NO_CHANGE |
				SQL_CA1_POS_DELETE | SQL_CA1_POS_POSITION | SQL_CA1_POS_UPDATE |
				SQL_CA1_POSITIONED_UPDATE | SQL_CA1_POSITIONED_DELETE;
		else
			UIVAL = 0;
		break;
	case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
		/* TODO cursors */
		/* Cursors not supported yet */
		/*
		 * Should be SQL_CA2_LOCK_CONCURRENCY SQL_CA2_MAX_ROWS_CATALOG SQL_CA2_MAX_ROWS_DELETE 
 		 * SQL_CA2_MAX_ROWS_INSERT SQL_CA2_MAX_ROWS_SELECT SQL_CA2_MAX_ROWS_UPDATE SQL_CA2_OPT_ROWVER_CONCURRENCY
 		 * SQL_CA2_OPT_VALUES_CONCURRENCY SQL_CA2_READ_ONLY_CONCURRENCY SQL_CA2_SENSITIVITY_ADDITIONS
 		 * SQL_CA2_SENSITIVITY_UPDATES SQL_CA2_SIMULATE_UNIQUE
		 */
		UIVAL = 0;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_EXPRESSIONS_IN_ORDERBY:
		p = "Y";
		break;
	case SQL_FILE_USAGE:
		USIVAL = SQL_FILE_NOT_SUPPORTED;
		break;
	case SQL_FETCH_DIRECTION:
		if (dbc->cursor_support)
			/* TODO cursors SQL_FD_FETCH_BOOKMARK */
			UIVAL = SQL_FD_FETCH_ABSOLUTE|SQL_FD_FETCH_FIRST|SQL_FD_FETCH_LAST|SQL_FD_FETCH_NEXT|SQL_FD_FETCH_PRIOR|SQL_FD_FETCH_RELATIVE;
		else
			UIVAL = 0;
		break;
#if (ODBCVER >= 0x0300)
	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
		if (dbc->cursor_support)
			/* TODO cursors SQL_CA1_SELECT_FOR_UPDATE */
			UIVAL = SQL_CA1_NEXT|SQL_CA1_POSITIONED_DELETE|SQL_CA1_POSITIONED_UPDATE;
		else
			UIVAL = 0;
		break;
	case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
		/* TODO cursors */
		/* Cursors not supported yet */
		/*
		 * Should be SQL_CA2_LOCK_CONCURRENCY SQL_CA2_MAX_ROWS_CATALOG SQL_CA2_MAX_ROWS_DELETE 
 		 * SQL_CA2_MAX_ROWS_INSERT SQL_CA2_MAX_ROWS_SELECT SQL_CA2_MAX_ROWS_UPDATE SQL_CA2_OPT_ROWVER_CONCURRENCY
 		 * SQL_CA2_OPT_VALUES_CONCURRENCY SQL_CA2_READ_ONLY_CONCURRENCY
		 */
		UIVAL = 0;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_GETDATA_EXTENSIONS:
		/* TODO FreeTDS support more ?? */
		UIVAL = SQL_GD_BLOCK;
		break;
	case SQL_GROUP_BY:
		USIVAL = SQL_GB_GROUP_BY_CONTAINS_SELECT;
		break;
	case SQL_IDENTIFIER_CASE:
		/* TODO configuration dependend */
		USIVAL = SQL_IC_MIXED;
		break;
	case SQL_IDENTIFIER_QUOTE_CHAR:
		p = "\"";
		/* TODO workaround for Sybase, use SET QUOTED_IDENTIFIER ON and fix unwanted quote */
		if (!is_ms)
			p = "";
		break;
#if (ODBCVER >= 0x0300)
	case SQL_INDEX_KEYWORDS:
		UIVAL = SQL_IK_ASC | SQL_IK_DESC;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_INFO_SCHEMA_VIEWS:
		/* TODO finish */
		UIVAL = 0;
		break;
	case SQL_INSERT_STATEMENT:
		UIVAL = 0;
		break;
		/* renamed from SQL_ODBC_SQL_OPT_IEF */
	case SQL_INTEGRITY:
		p = "Y";
		break;
	case SQL_KEYSET_CURSOR_ATTRIBUTES1:
	case SQL_KEYSET_CURSOR_ATTRIBUTES2:
		/* TODO cursors */
		UIVAL = 0;
		break;
	case SQL_KEYWORDS:
		/* TODO ok for Sybase ? */
		p = "BREAK,BROWSE,BULK,CHECKPOINT,CLUSTERED,"
			"COMMITTED,COMPUTE,CONFIRM,CONTROLROW,DATABASE,"
			"DBCC,DISK,DISTRIBUTED,DUMMY,DUMP,ERRLVL,"
			"ERROREXIT,EXIT,FILE,FILLFACTOR,FLOPPY,HOLDLOCK,"
			"IDENTITY_INSERT,IDENTITYCOL,IF,KILL,LINENO,"
			"LOAD,MIRROREXIT,NONCLUSTERED,OFF,OFFSETS,ONCE,"
			"OVER,PERCENT,PERM,PERMANENT,PLAN,PRINT,PROC,"
			"PROCESSEXIT,RAISERROR,READ,READTEXT,RECONFIGURE,"
			"REPEATABLE,RETURN,ROWCOUNT,RULE,SAVE,SERIALIZABLE,"
			"SETUSER,SHUTDOWN,STATISTICS,TAPE,TEMP,TEXTSIZE,"
			"TRAN,TRIGGER,TRUNCATE,TSEQUEL,UNCOMMITTED," "UPDATETEXT,USE,WAITFOR,WHILE,WRITETEXT";
		break;
	case SQL_LIKE_ESCAPE_CLAUSE:
		p = "Y";
		break;
	case SQL_LOCK_TYPES:
		if (dbc->cursor_support)
			IVAL = SQL_LCK_NO_CHANGE;
		else
			IVAL = 0;
		break;
#if (ODBCVER >= 0x0300)
	case SQL_MAX_ASYNC_CONCURRENT_STATEMENTS:
		UIVAL = 1;
		break;
#endif /* ODBCVER >= 0x0300 */
		/* TODO same limits for Sybase ? */
	case SQL_MAX_BINARY_LITERAL_LEN:
		UIVAL = 131072;
		break;
	case SQL_MAX_CHAR_LITERAL_LEN:
		UIVAL = 131072;
		break;
	case SQL_MAX_CONCURRENT_ACTIVITIES:
		USIVAL = 1;
		break;
	case SQL_MAX_COLUMNS_IN_GROUP_BY:
	case SQL_MAX_COLUMNS_IN_INDEX:
	case SQL_MAX_COLUMNS_IN_ORDER_BY:
		/* TODO Sybase ? */
		USIVAL = 16;
		break;
	case SQL_MAX_COLUMNS_IN_SELECT:
		/* TODO Sybase ? */
		USIVAL = 4000;
		break;
	case SQL_MAX_COLUMNS_IN_TABLE:
		/* TODO Sybase ? */
		USIVAL = 250;
		break;
	case SQL_MAX_DRIVER_CONNECTIONS:
		USIVAL = 0;
		break;
	case SQL_MAX_IDENTIFIER_LEN:
		if (is_ms == -1)
			return SQL_ERROR;
		USIVAL = (!is_ms || smajor < 7) ? 30 : 128;
		break;
	case SQL_MAX_INDEX_SIZE:
		UIVAL = 127;
		break;
	case SQL_MAX_PROCEDURE_NAME_LEN:
		if (is_ms == -1)
			return SQL_ERROR;
		/* TODO Sybase ? */
		USIVAL = (is_ms && smajor >= 7) ? 134 : 36;
		break;
	case SQL_MAX_ROW_SIZE:
		if (is_ms == -1)
			return SQL_ERROR;
		/* TODO Sybase ? */
		UIVAL = (is_ms && smajor >= 7) ? 8062 : 1962;
		break;
	case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
		p = "N";
		break;
	case SQL_MAX_STATEMENT_LEN:
		/* TODO Sybase ? */
		UIVAL = 131072;
		break;
	case SQL_MAX_TABLES_IN_SELECT:
		/* TODO Sybase ? */
		USIVAL = 16;
		break;
	case SQL_MAX_USER_NAME_LEN:
		/* TODO Sybase ? */
		if (is_ms == -1)
			return SQL_ERROR;
		USIVAL = (is_ms && smajor >= 7) ? 128 : 30;
		break;
	case SQL_MAX_COLUMN_NAME_LEN:
	case SQL_MAX_CURSOR_NAME_LEN:
	case SQL_MAX_SCHEMA_NAME_LEN:
	case SQL_MAX_CATALOG_NAME_LEN:
	case SQL_MAX_TABLE_NAME_LEN:
		/* TODO Sybase ? */
		if (is_ms == -1)
			return SQL_ERROR;
		USIVAL = (is_ms && smajor >= 7) ? 128 : 30;
		break;
	case SQL_MULT_RESULT_SETS:
		p = "Y";
		break;
	case SQL_MULTIPLE_ACTIVE_TXN:
		p = "Y";
		break;
	case SQL_NEED_LONG_DATA_LEN:
		/* current implementation do not require length, however future will, so is correct to return yes */
		p = "Y";
		break;
	case SQL_NON_NULLABLE_COLUMNS:
		USIVAL = SQL_NNC_NON_NULL;
		break;
	case SQL_NULL_COLLATION:
		USIVAL = SQL_NC_LOW;
		break;
	case SQL_NUMERIC_FUNCTIONS:
		UIVAL = (SQL_FN_NUM_ABS | SQL_FN_NUM_ACOS | SQL_FN_NUM_ASIN | SQL_FN_NUM_ATAN | SQL_FN_NUM_ATAN2 |
			 SQL_FN_NUM_CEILING | SQL_FN_NUM_COS | SQL_FN_NUM_COT | SQL_FN_NUM_DEGREES | SQL_FN_NUM_EXP |
			 SQL_FN_NUM_FLOOR | SQL_FN_NUM_LOG | SQL_FN_NUM_LOG10 | SQL_FN_NUM_MOD | SQL_FN_NUM_PI | SQL_FN_NUM_POWER |
			 SQL_FN_NUM_RADIANS | SQL_FN_NUM_RAND | SQL_FN_NUM_ROUND | SQL_FN_NUM_SIGN | SQL_FN_NUM_SIN |
			 SQL_FN_NUM_SQRT | SQL_FN_NUM_TAN) & mssql7plus_mask;
		break;
	case SQL_ODBC_API_CONFORMANCE:
		SIVAL = SQL_OAC_LEVEL2;
		break;
#if (ODBCVER >= 0x0300)
	case SQL_ODBC_INTERFACE_CONFORMANCE:
		UIVAL = SQL_OAC_LEVEL2;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_ODBC_SAG_CLI_CONFORMANCE:
		SIVAL = SQL_OSCC_NOT_COMPLIANT;
		break;
	case SQL_ODBC_SQL_CONFORMANCE:
		SIVAL = SQL_OSC_CORE;
		break;
#ifdef TDS_NO_DM
	case SQL_ODBC_VER:
		/* TODO check format ##.##.0000 */
		p = VERSION;
		break;
#endif
#if (ODBCVER >= 0x0300)
	case SQL_OJ_CAPABILITIES:
		UIVAL = SQL_OJ_ALL_COMPARISON_OPS | SQL_OJ_FULL | SQL_OJ_INNER | SQL_OJ_LEFT | SQL_OJ_NESTED | SQL_OJ_NOT_ORDERED |
			SQL_OJ_RIGHT;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_ORDER_BY_COLUMNS_IN_SELECT:
		p = "N";
		break;
	case SQL_OUTER_JOINS:
		p = "Y";
		break;
#if (ODBCVER >= 0x0300)
	case SQL_PARAM_ARRAY_ROW_COUNTS:
		UIVAL = SQL_PARC_BATCH;
		break;
	case SQL_PARAM_ARRAY_SELECTS:
		UIVAL = SQL_PAS_BATCH;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_POS_OPERATIONS:
		if (dbc->cursor_support)
			/* TODO cursors SQL_POS_ADD SQL_POS_REFRESH */
			/* test REFRESH, ADD under mssql, under Sybase I don't know how to do it but dblib do */
			IVAL = SQL_POS_DELETE | SQL_POS_POSITION | SQL_POS_UPDATE;
		else
			IVAL = 0;
		break;
	case SQL_POSITIONED_STATEMENTS:
		/* TODO cursors SQL_PS_SELECT_FOR_UPDATE */
		IVAL = SQL_PS_POSITIONED_DELETE | SQL_PS_POSITIONED_UPDATE;
		break;
	case SQL_PROCEDURE_TERM:
		p = "stored procedure";
		break;
	case SQL_PROCEDURES:
		p = "Y";
		break;
	case SQL_QUOTED_IDENTIFIER_CASE:
		/* TODO usually insensitive */
		USIVAL = SQL_IC_MIXED;
		break;
	case SQL_ROW_UPDATES:
		p = "N";
		break;
#if (ODBCVER >= 0x0300)
	case SQL_SCHEMA_TERM:
		p = "owner";
		break;
	case SQL_SCHEMA_USAGE:
		UIVAL = SQL_OU_DML_STATEMENTS | SQL_OU_INDEX_DEFINITION | SQL_OU_PRIVILEGE_DEFINITION | SQL_OU_PROCEDURE_INVOCATION
			| SQL_OU_TABLE_DEFINITION;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_SCROLL_CONCURRENCY:
		if (dbc->cursor_support)
			/* TODO cursors test them, Sybase support all ?  */
			IVAL = SQL_SCCO_LOCK | SQL_SCCO_OPT_ROWVER | SQL_SCCO_OPT_VALUES | SQL_SCCO_READ_ONLY;
		else
			IVAL = SQL_SCCO_READ_ONLY;
		break;
	case SQL_SCROLL_OPTIONS:
		if (dbc->cursor_support)
			/* TODO cursors test them, Sybase support all ?? */
			UIVAL = SQL_SO_DYNAMIC | SQL_SO_FORWARD_ONLY | SQL_SO_KEYSET_DRIVEN | SQL_SO_STATIC;
		else
			UIVAL = SQL_SO_FORWARD_ONLY | SQL_SO_STATIC;
		break;
	case SQL_SEARCH_PATTERN_ESCAPE:
		p = "\\";
		break;
	case SQL_SERVER_NAME:
		p = tds_dstr_cstr(&dbc->server);
		break;
	case SQL_SPECIAL_CHARACTERS:
		/* TODO others ?? */
		p = "\'\"[]{}";
		break;
#if (ODBCVER >= 0x0300)
	case SQL_SQL_CONFORMANCE:
		UIVAL = SQL_SC_SQL92_ENTRY;
		break;
	case SQL_SQL92_DATETIME_FUNCTIONS:
	case SQL_SQL92_FOREIGN_KEY_DELETE_RULE:
	case SQL_SQL92_FOREIGN_KEY_UPDATE_RULE:
		UIVAL = 0;
		break;
	case SQL_SQL92_GRANT:
		UIVAL = SQL_SG_WITH_GRANT_OPTION;
		break;
	case SQL_SQL92_NUMERIC_VALUE_FUNCTIONS:
		UIVAL = 0;
		break;
	case SQL_SQL92_PREDICATES:
		/* TODO Sybase ?? */
		UIVAL = SQL_SP_EXISTS | SQL_SP_ISNOTNULL | SQL_SP_ISNULL;
		break;
	case SQL_SQL92_RELATIONAL_JOIN_OPERATORS:
		/* TODO Sybase ?? */
		UIVAL = SQL_SRJO_CROSS_JOIN | SQL_SRJO_FULL_OUTER_JOIN | SQL_SRJO_INNER_JOIN | SQL_SRJO_LEFT_OUTER_JOIN |
			SQL_SRJO_RIGHT_OUTER_JOIN | SQL_SRJO_UNION_JOIN;
		break;
	case SQL_SQL92_REVOKE:
		UIVAL = SQL_SR_GRANT_OPTION_FOR;
		break;
	case SQL_SQL92_ROW_VALUE_CONSTRUCTOR:
		UIVAL = SQL_SRVC_DEFAULT | SQL_SRVC_NULL | SQL_SRVC_ROW_SUBQUERY | SQL_SRVC_VALUE_EXPRESSION;
		break;
	case SQL_SQL92_STRING_FUNCTIONS:
		UIVAL = SQL_SSF_LOWER | SQL_SSF_UPPER;
		break;
	case SQL_SQL92_VALUE_EXPRESSIONS:
		/* TODO Sybase ? CAST supported ? */
		UIVAL = SQL_SVE_CASE | SQL_SVE_CAST | SQL_SVE_COALESCE | SQL_SVE_NULLIF;
		break;
	case SQL_STANDARD_CLI_CONFORMANCE:
		UIVAL = SQL_SCC_ISO92_CLI;
		break;
	case SQL_STATIC_SENSITIVITY:
		IVAL = 0;
		break;
	case SQL_STATIC_CURSOR_ATTRIBUTES1:
		if (dbc->cursor_support)
			/* TODO cursors SQL_CA1_BOOKMARK SQL_CA1_BULK_FETCH_BY_BOOKMARK SQL_CA1_POS_REFRESH */
			UIVAL = SQL_CA1_ABSOLUTE | SQL_CA1_LOCK_NO_CHANGE | SQL_CA1_NEXT | SQL_CA1_POS_POSITION | SQL_CA1_RELATIVE;
		else
			UIVAL = 0;
		break;
	case SQL_STATIC_CURSOR_ATTRIBUTES2:
		/* TODO cursors */
		UIVAL = 0;
		break;
#endif /* ODBCVER >= 0x0300 */
	case SQL_STRING_FUNCTIONS:
		UIVAL = (SQL_FN_STR_ASCII | SQL_FN_STR_BIT_LENGTH | SQL_FN_STR_CHAR | SQL_FN_STR_CONCAT | SQL_FN_STR_DIFFERENCE |
			 SQL_FN_STR_INSERT | SQL_FN_STR_LCASE | SQL_FN_STR_LEFT | SQL_FN_STR_LENGTH | SQL_FN_STR_LOCATE_2 |
			 SQL_FN_STR_LTRIM | SQL_FN_STR_OCTET_LENGTH | SQL_FN_STR_REPEAT | SQL_FN_STR_RIGHT | SQL_FN_STR_RTRIM |
			 SQL_FN_STR_SOUNDEX | SQL_FN_STR_SPACE | SQL_FN_STR_SUBSTRING | SQL_FN_STR_UCASE) & mssql7plus_mask;
		break;
	case SQL_SUBQUERIES:
		UIVAL = SQL_SQ_COMPARISON | SQL_SQ_CORRELATED_SUBQUERIES | SQL_SQ_EXISTS | SQL_SQ_IN | SQL_SQ_QUANTIFIED;
		break;
	case SQL_SYSTEM_FUNCTIONS:
		UIVAL = SQL_FN_SYS_DBNAME | SQL_FN_SYS_IFNULL | SQL_FN_SYS_USERNAME;
		break;
	case SQL_TABLE_TERM:
		p = "table";
		break;
	case SQL_TIMEDATE_ADD_INTERVALS:
		UIVAL = (SQL_FN_TSI_DAY | SQL_FN_TSI_FRAC_SECOND | SQL_FN_TSI_HOUR | SQL_FN_TSI_MINUTE | SQL_FN_TSI_MONTH |
			 SQL_FN_TSI_QUARTER | SQL_FN_TSI_SECOND | SQL_FN_TSI_WEEK | SQL_FN_TSI_YEAR) & mssql7plus_mask;
		break;
	case SQL_TIMEDATE_DIFF_INTERVALS:
		UIVAL = (SQL_FN_TSI_DAY | SQL_FN_TSI_FRAC_SECOND | SQL_FN_TSI_HOUR | SQL_FN_TSI_MINUTE | SQL_FN_TSI_MONTH |
			 SQL_FN_TSI_QUARTER | SQL_FN_TSI_SECOND | SQL_FN_TSI_WEEK | SQL_FN_TSI_YEAR) & mssql7plus_mask;
		break;
	case SQL_TIMEDATE_FUNCTIONS:
		UIVAL = (SQL_FN_TD_CURDATE | SQL_FN_TD_CURRENT_DATE | SQL_FN_TD_CURRENT_TIME | SQL_FN_TD_CURRENT_TIMESTAMP |
			 SQL_FN_TD_CURTIME | SQL_FN_TD_DAYNAME | SQL_FN_TD_DAYOFMONTH | SQL_FN_TD_DAYOFWEEK | SQL_FN_TD_DAYOFYEAR |
			 SQL_FN_TD_EXTRACT | SQL_FN_TD_HOUR | SQL_FN_TD_MINUTE | SQL_FN_TD_MONTH | SQL_FN_TD_MONTHNAME |
			 SQL_FN_TD_NOW | SQL_FN_TD_QUARTER | SQL_FN_TD_SECOND | SQL_FN_TD_TIMESTAMPADD | SQL_FN_TD_TIMESTAMPDIFF |
			 SQL_FN_TD_WEEK | SQL_FN_TD_YEAR) & mssql7plus_mask;
		break;
	case SQL_TXN_CAPABLE:
		/* transaction for DML and DDL */
		SIVAL = SQL_TC_ALL;
		break;
	case SQL_TXN_ISOLATION_OPTION:
		UIVAL = SQL_TXN_READ_COMMITTED | SQL_TXN_READ_UNCOMMITTED | SQL_TXN_REPEATABLE_READ | SQL_TXN_SERIALIZABLE;
		break;
	case SQL_UNION:
		UIVAL = SQL_U_UNION | SQL_U_UNION_ALL;
		break;
		/* TODO finish */
	case SQL_USER_NAME:
		/* TODO this is not correct username */
		/* p = tds_dstr_cstr(&dbc->tds_login->user_name); */
		/* make OpenOffice happy :) */
		p = "";
		break;
	case SQL_XOPEN_CLI_YEAR:
		/* TODO check specifications */
		p = "1995";
		break;
	default:
		odbc_log_unimplemented_type("SQLGetInfo", fInfoType);
		odbc_errs_add(&dbc->errs, "HY092", "Option not supported");
		return SQL_ERROR;
	}

	/* char data */
	if (p) {
		return odbc_set_string_oct(dbc, rgbInfoValue, cbInfoValueMax, pcbInfoValue, p, -1);
	} else {
		if (out_len > 0 && pcbInfoValue)
			*pcbInfoValue = out_len;
	}

	return SQL_SUCCESS;
#undef SIVAL
#undef USIVAL
#undef IVAL
#undef UIVAL
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetInfo(SQLHDBC hdbc, SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue, SQLSMALLINT cbInfoValueMax,
	   SQLSMALLINT FAR * pcbInfoValue)
{
	ODBC_ENTER_HDBC;

	tdsdump_log(TDS_DBG_FUNC, "SQLGetInfo(%p, %d, %p, %d, %p)\n", 
			hdbc, fInfoType, rgbInfoValue, cbInfoValueMax, pcbInfoValue);

	ODBC_EXIT(dbc, _SQLGetInfo(dbc, fInfoType, rgbInfoValue, cbInfoValueMax, pcbInfoValue _wide0));
}

#ifdef ENABLE_ODBC_WIDE
SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetInfoW(SQLHDBC hdbc, SQLUSMALLINT fInfoType, SQLPOINTER rgbInfoValue, SQLSMALLINT cbInfoValueMax,
	   SQLSMALLINT FAR * pcbInfoValue)
{
	ODBC_ENTER_HDBC;

	tdsdump_log(TDS_DBG_FUNC, "SQLGetInfoW(%p, %d, %p, %d, %p)\n",
			hdbc, fInfoType, rgbInfoValue, cbInfoValueMax, pcbInfoValue);

	ODBC_EXIT(dbc, _SQLGetInfo(dbc, fInfoType, rgbInfoValue, cbInfoValueMax, pcbInfoValue, 1));
}
#endif

static void
tds_ascii_strupr(char *s)
{
	for (; *s; ++s)
		if ('a' <= *s && *s <= 'z')
			*s = *s & (~0x20);
}

static void
odbc_upper_column_names(TDS_STMT * stmt)
{
#if ENABLE_EXTRA_CHECKS
	TDSRESULTINFO *resinfo;
	TDSCOLUMN *colinfo;
	TDSSOCKET *tds;
#endif
	int icol;
	TDS_DESC *ird;

	IRD_CHECK;

#if ENABLE_EXTRA_CHECKS
	tds = stmt->dbc->tds_socket;
	if (!tds || !tds->current_results)
		return;

	resinfo = tds->current_results;
	for (icol = 0; icol < resinfo->num_cols; ++icol) {
		char *p;

		colinfo = resinfo->columns[icol];
		/* upper case */
		/* TODO procedure */
		for (p = colinfo->column_name; p < (colinfo->column_name + colinfo->column_namelen); ++p)
			if ('a' <= *p && *p <= 'z')
				*p = *p & (~0x20);
	}
#endif

	ird = stmt->ird;
	for (icol = ird->header.sql_desc_count; --icol >= 0;) {
		struct _drecord *drec = &ird->records[icol];

		/* upper case */
		tds_ascii_strupr(tds_dstr_buf(&drec->sql_desc_label));
		tds_ascii_strupr(tds_dstr_buf(&drec->sql_desc_name));
	}
}

static SQLSMALLINT
odbc_swap_datetime_sql_type(SQLSMALLINT sql_type)
{
	switch (sql_type) {
	case SQL_TYPE_TIMESTAMP:
		sql_type = SQL_TIMESTAMP;
		break;
	case SQL_TIMESTAMP:
		sql_type = SQL_TYPE_TIMESTAMP;
		break;
	case SQL_TYPE_DATE:
		sql_type = SQL_DATE;
		break;
	case SQL_DATE:
		sql_type = SQL_TYPE_DATE;
		break;
	case SQL_TYPE_TIME:
		sql_type = SQL_TIME;
		break;
	case SQL_TIME:
		sql_type = SQL_TYPE_TIME;
		break;
	}
	return sql_type;
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetTypeInfo(SQLHSTMT hstmt, SQLSMALLINT fSqlType)
{
	SQLRETURN res;
	TDSSOCKET *tds;
	TDS_INT result_type;
	TDS_INT compute_id;
	int varchar_pos = -1, n;
	static const char sql_templ[] = "EXEC sp_datatype_info %d";
	char sql[sizeof(sql_templ) + 30];
	int odbc3;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLGetTypeInfo(%p, %d)\n", hstmt, fSqlType);

	tds = stmt->dbc->tds_socket;
	odbc3 = (stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3);

	/* For MSSQL6.5 and Sybase 11.9 sp_datatype_info work */
	/* TODO what about early Sybase products ? */
	/* TODO Does Sybase return all ODBC3 columns? Add them if not */
	/* TODO ODBC3 convert type to ODBC version 2 (date) */
	if (odbc3) {
		if (TDS_IS_SYBASE(tds)) {
			sprintf(sql, sql_templ, odbc_swap_datetime_sql_type(fSqlType));
			stmt->special_row = ODBC_SPECIAL_GETTYPEINFO;
		} else {
			sprintf(sql, sql_templ, fSqlType);
			strcat(sql, ",3");
		}
	} else {
		/* FIXME MS ODBC translate SQL_TIMESTAMP to SQL_TYPE_TIMESTAMP even for ODBC 2 apps */
		sprintf(sql, sql_templ, fSqlType);
	}
	if (SQL_SUCCESS != odbc_set_stmt_query(stmt, (ODBC_CHAR*) sql, strlen(sql) _wide0))
		ODBC_EXIT(stmt, SQL_ERROR);

      redo:
	res = _SQLExecute(stmt);

	odbc_upper_column_names(stmt);
	if (odbc3) {
		odbc_col_setname(stmt, 3, "COLUMN_SIZE");
		odbc_col_setname(stmt, 11, "FIXED_PREC_SCALE");
		odbc_col_setname(stmt, 12, "AUTO_UNIQUE_VALUE");
	}

	/* workaround for a mispelled column name in Sybase */
	if (TDS_IS_SYBASE(stmt->dbc->tds_socket) && !odbc3)
		odbc_col_setname(stmt, 3, "PRECISION");

	if (TDS_IS_MSSQL(stmt->dbc->tds_socket) || fSqlType != SQL_VARCHAR || res != SQL_SUCCESS)
		ODBC_EXIT(stmt, res);

	/*
	 * Sybase return first nvarchar for char... and without length !!!
	 * Some program use first entry so we discard all entry before varchar
	 */
	n = 0;
	while (tds->current_results) {
		TDSRESULTINFO *resinfo;
		TDSCOLUMN *colinfo;
		char *name;

		/* if next is varchar leave next for SQLFetch */
		if (n == (varchar_pos - 1))
			break;

		switch (tds_process_tokens(stmt->dbc->tds_socket, &result_type, &compute_id, TDS_STOPAT_ROWFMT|TDS_RETURN_ROW)) {
		case TDS_SUCCESS:
			if (result_type == TDS_ROW_RESULT)
				break;
		case TDS_NO_MORE_RESULTS:
			/* discard other tokens */
			tds_process_simple_query(tds);
			if (n >= varchar_pos && varchar_pos > 0)
				goto redo;
			break;
		case TDS_CANCELLED:
			odbc_errs_add(&stmt->errs, "HY008", NULL);
			res = SQL_ERROR;
			break;
		}
		if (!tds->current_results)
			break;
		++n;

		resinfo = tds->current_results;
		colinfo = resinfo->columns[0];
		name = (char *) colinfo->column_data;
		if (is_blob_col(colinfo))
			name = (char*) ((TDSBLOB *) name)->textvalue;
		/* skip nvarchar and sysname */
		if (colinfo->column_cur_size == 7 && memcmp("varchar", name, 7) == 0) {
			varchar_pos = n;
		}
	}
	ODBC_EXIT(stmt, res);
}

#ifdef ENABLE_ODBC_WIDE
SQLRETURN ODBC_PUBLIC ODBC_API
SQLGetTypeInfoW(SQLHSTMT hstmt, SQLSMALLINT fSqlType)
{
	return SQLGetTypeInfo(hstmt, fSqlType);
}
#endif

static SQLRETURN 
_SQLParamData(SQLHSTMT hstmt, SQLPOINTER FAR * prgbValue)
{
	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLParamData(%p, %p) [param_num %d, param_data_called = %d]\n", 
					hstmt, prgbValue, stmt->param_num, stmt->param_data_called);

	if (stmt->params && stmt->param_num <= stmt->param_count) {
		SQLRETURN res;

		if (stmt->param_num <= 0 || stmt->param_num > stmt->apd->header.sql_desc_count) {
			tdsdump_log(TDS_DBG_FUNC, "SQLParamData: logic_error: parameter out of bounds: 0 <= %d < %d\n", 
						   stmt->param_num, stmt->apd->header.sql_desc_count);
			ODBC_EXIT(stmt, SQL_ERROR);
		}

		/*
		 * TODO compute output value with this formula:
		 * Bound Address + Binding Offset + ((Row Number - 1) x Element Size)
		 * (see SQLParamData documentation)
		 * This is needed for updates using SQLBulkOperation (not currently supported)
		 */
		if (!stmt->param_data_called) {
			stmt->param_data_called = 1;
			*prgbValue = stmt->apd->records[stmt->param_num - 1].sql_desc_data_ptr;
			ODBC_EXIT(stmt, SQL_NEED_DATA);
		}
		++stmt->param_num;
		switch (res = parse_prepared_query(stmt, 1)) {
		case SQL_NEED_DATA:
			*prgbValue = stmt->apd->records[stmt->param_num - 1].sql_desc_data_ptr;
			ODBC_EXIT(stmt, SQL_NEED_DATA);
		case SQL_SUCCESS:
			ODBC_EXIT(stmt, _SQLExecute(stmt));
		}
		ODBC_EXIT(stmt, res);
	}

	odbc_errs_add(&stmt->errs, "HY010", NULL);
	ODBC_EXIT_(stmt);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLParamData(SQLHSTMT hstmt, SQLPOINTER FAR * prgbValue)
{
	ODBC_PRRET_BUF;
	SQLRETURN ret = _SQLParamData(hstmt, prgbValue);
	
	tdsdump_log(TDS_DBG_FUNC, "SQLParamData returns %s\n", odbc_prret(ret));
	
	return ret;
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLPutData(SQLHSTMT hstmt, SQLPOINTER rgbValue, SQLLEN cbValue)
{
	ODBC_PRRET_BUF;
	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLPutData(%p, %p, %i)\n", hstmt, rgbValue, (int)cbValue);

	if (stmt->prepared_query || stmt->prepared_query_is_rpc) {
		SQLRETURN ret;
		const TDSCOLUMN *curcol = stmt->params->columns[stmt->param_num - (stmt->prepared_query_is_func ? 2 : 1)];
		/* TODO do some more tests before setting this flag */
		stmt->param_data_called = 1;
		ret = continue_parse_prepared_query(stmt, rgbValue, cbValue);
		tdsdump_log(TDS_DBG_FUNC, "SQLPutData returns %s, %d bytes left\n", 
					  odbc_prret(ret), curcol->column_size - curcol->column_cur_size);
		ODBC_EXIT(stmt, ret);
	}

	odbc_errs_add(&stmt->errs, "HY010", NULL);
	ODBC_EXIT_(stmt);
}


ODBC_FUNC(SQLSetConnectAttr, (P(SQLHDBC,hdbc), P(SQLINTEGER,Attribute), P(SQLPOINTER,ValuePtr), P(SQLINTEGER,StringLength) WIDE))
{
	SQLULEN u_value = (SQLULEN) (TDS_INTPTR) ValuePtr;

	ODBC_ENTER_HDBC;

	tdsdump_log(TDS_DBG_FUNC, "_SQLSetConnectAttr(%p, %d, %p, %d)\n", hdbc, (int)Attribute, ValuePtr, (int)StringLength);

	switch (Attribute) {
	case SQL_ATTR_AUTOCOMMIT:
		/* spinellia@acm.org */
		change_autocommit(dbc, u_value);
		break;
	case SQL_ATTR_CONNECTION_TIMEOUT:
		dbc->attr.connection_timeout = u_value;
		break;
	case SQL_ATTR_ACCESS_MODE:
		dbc->attr.access_mode = u_value;
		break;
	case SQL_ATTR_CURRENT_CATALOG:
		if (!IS_VALID_LEN(StringLength)) {
			odbc_errs_add(&dbc->errs, "HY090", NULL);
			break;
		}
		{
			DSTR s;

			tds_dstr_init(&s);
			if (!odbc_dstr_copy_oct(dbc, &s, StringLength, (ODBC_CHAR*) ValuePtr)) {
				odbc_errs_add(&dbc->errs, "HY001", NULL);
				break;
			}
			change_database(dbc, tds_dstr_cstr(&s), tds_dstr_len(&s));
			tds_dstr_free(&s);
		}
		break;
	case SQL_ATTR_CURSOR_TYPE:
		if (dbc->cursor_support)
			dbc->attr.cursor_type = u_value;
		break;
	case SQL_ATTR_LOGIN_TIMEOUT:
		dbc->attr.login_timeout = u_value;
		break;
	case SQL_ATTR_ODBC_CURSORS:
		/* TODO cursors */
		dbc->attr.odbc_cursors = u_value;
		break;
	case SQL_ATTR_PACKET_SIZE:
		dbc->attr.packet_size = u_value;
		break;
	case SQL_ATTR_QUIET_MODE:
		dbc->attr.quite_mode = (SQLHWND) (TDS_INTPTR) ValuePtr;
		break;
#ifdef TDS_NO_DM
	case SQL_ATTR_TRACE:
		dbc->attr.trace = u_value;
		break;
	case SQL_ATTR_TRACEFILE:
		if (!IS_VALID_LEN(StringLength)) {
			odbc_errs_add(&dbc->errs, "HY090", NULL);
			break;
		}
		if (!odbc_dstr_copy(dbc, &dbc->attr.tracefile, StringLength, (ODBC_CHAR *) ValuePtr))
			odbc_errs_add(&dbc->errs, "HY001", NULL);
		break;
#endif
	case SQL_ATTR_TXN_ISOLATION:
		if (u_value != dbc->attr.txn_isolation) {
			if (change_txn(dbc, u_value) == SQL_SUCCESS)
				dbc->attr.txn_isolation = u_value;
		}
		break;
	case SQL_COPT_SS_MARS_ENABLED:
		dbc->attr.mars_enabled = u_value;
		break;
	case SQL_ATTR_TRANSLATE_LIB:
	case SQL_ATTR_TRANSLATE_OPTION:
		odbc_errs_add(&dbc->errs, "HYC00", NULL);
		break;
	default:
		odbc_errs_add(&dbc->errs, "HY092", NULL);
		break;
	}
	ODBC_EXIT_(dbc);
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLSetConnectOption(SQLHDBC hdbc, SQLUSMALLINT fOption, SQLULEN vParam)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLSetConnectOption(%p, %d, %u)\n", hdbc, fOption, (unsigned)vParam);
	/* XXX: Lost precision */
	return _SQLSetConnectAttr(hdbc, (SQLINTEGER) fOption, (SQLPOINTER) (TDS_INTPTR) vParam, SQL_NTS _wide0);
}

#ifdef ENABLE_ODBC_WIDE
SQLRETURN ODBC_PUBLIC ODBC_API
SQLSetConnectOptionW(SQLHDBC hdbc, SQLUSMALLINT fOption, SQLULEN vParam)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLSetConnectOptionW(%p, %d, %u)\n", hdbc, fOption, (unsigned)vParam);
	/* XXX: Lost precision */
	return _SQLSetConnectAttr(hdbc, (SQLINTEGER) fOption, (SQLPOINTER) (TDS_INTPTR) vParam, SQL_NTS, 1);
}
#endif

static SQLRETURN
_SQLSetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength WIDE)
{
	SQLULEN ui = (SQLULEN) (TDS_INTPTR) ValuePtr;
	SQLUSMALLINT *usip = (SQLUSMALLINT *) ValuePtr;
	SQLLEN *lp = (SQLLEN *) ValuePtr;
	SQLULEN *ulp = (SQLULEN *) ValuePtr;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "_SQLSetStmtAttr(%p, %d, %p, %d)\n", hstmt, (int)Attribute, ValuePtr, (int)StringLength);

	/* TODO - error checking and real functionality :-) */
	/* TODO some setting set also other attribute, see documentation */
	switch (Attribute) {
		/* TODO check SQLFreeHandle on descriptor. Is possible to free an associated descriptor ? */
	case SQL_ATTR_APP_PARAM_DESC:
	case SQL_ATTR_APP_ROW_DESC:
		{
			TDS_DESC *orig;
			TDS_DESC **curr;
			TDS_DESC *val = (TDS_DESC *) ValuePtr;

			if (Attribute == SQL_ATTR_APP_PARAM_DESC) {
				orig = stmt->orig_apd;
				curr = &stmt->apd;
			} else {
				orig = stmt->orig_ard;
				curr = &stmt->ard;
			}
			/* if ValuePtr is NULL or original descriptor set original */
			if (!val || val == orig) {
				*curr = orig;
				break;
			}

			/* must be allocated by user, not implicit ones */
			if (val->header.sql_desc_alloc_type != SQL_DESC_ALLOC_USER) {
				odbc_errs_add(&stmt->errs, "HY017", NULL);
				break;
			}
			/* TODO check HDESC (not associated, from DBC HY024) */
			*curr = val;
		}
		break;
	case SQL_ATTR_ASYNC_ENABLE:
		if (stmt->attr.async_enable != ui) {
			odbc_errs_add(&stmt->errs, "HYC00", NULL);
			break;
		}
		stmt->attr.async_enable = ui;
		break;
	case SQL_ATTR_CONCURRENCY:
		if (stmt->attr.concurrency != ui && !stmt->dbc->cursor_support) {
			odbc_errs_add(&stmt->errs, "01S02", NULL);
			break;
		}

		if (stmt->cursor) {
			odbc_errs_add(&stmt->errs, "24000", NULL);
			break;
		}

		switch (ui) {
		case SQL_CONCUR_READ_ONLY:
			stmt->attr.cursor_sensitivity = SQL_INSENSITIVE;
			break;
		case SQL_CONCUR_LOCK:
		case SQL_CONCUR_ROWVER:
		case SQL_CONCUR_VALUES:
			stmt->attr.cursor_sensitivity = SQL_SENSITIVE;
			break;
		default:
			odbc_errs_add(&stmt->errs, "HY092", NULL);
			ODBC_EXIT_(stmt);
		}
		stmt->attr.concurrency = ui;
		break;
	case SQL_ATTR_CURSOR_SCROLLABLE:
		if (stmt->attr.cursor_scrollable != ui && !stmt->dbc->cursor_support) {
			odbc_errs_add(&stmt->errs, "HYC00", NULL);
			break;
		}

		switch (ui) {
		case SQL_SCROLLABLE:
			stmt->attr.cursor_type = SQL_CURSOR_KEYSET_DRIVEN;
			break;
		case SQL_NONSCROLLABLE:
			stmt->attr.cursor_type = SQL_CURSOR_FORWARD_ONLY;
			break;
		default:
			odbc_errs_add(&stmt->errs, "HY092", NULL);
			ODBC_EXIT_(stmt);
		}
		stmt->attr.cursor_scrollable = ui;
		break;
	case SQL_ATTR_CURSOR_SENSITIVITY:
		/* don't change anything */
		if (ui == SQL_UNSPECIFIED)
			break;
		if (stmt->attr.cursor_sensitivity != ui && !stmt->dbc->cursor_support) {
			odbc_errs_add(&stmt->errs, "HYC00", NULL);
			break;
		}
		switch (ui) {
		case SQL_INSENSITIVE:
			stmt->attr.concurrency = SQL_CONCUR_READ_ONLY;
			stmt->attr.cursor_type = SQL_CURSOR_STATIC;
			break;
		case SQL_SENSITIVE:
			stmt->attr.concurrency = SQL_CONCUR_ROWVER;
			break;
		}
		stmt->attr.cursor_sensitivity = ui;
		break;
	case SQL_ATTR_CURSOR_TYPE:
		if (stmt->attr.cursor_type != ui && !stmt->dbc->cursor_support) {
			odbc_errs_add(&stmt->errs, "01S02", NULL);
			break;
		}

		if (stmt->cursor) {
			odbc_errs_add(&stmt->errs, "24000", NULL);
			break;
		}

		switch (ui) {
		case SQL_CURSOR_DYNAMIC:
		case SQL_CURSOR_KEYSET_DRIVEN:
			if (stmt->attr.concurrency != SQL_CONCUR_READ_ONLY)
				stmt->attr.cursor_sensitivity = SQL_SENSITIVE;
			stmt->attr.cursor_scrollable = SQL_SCROLLABLE;
			break;
		case SQL_CURSOR_STATIC:
			if (stmt->attr.concurrency != SQL_CONCUR_READ_ONLY)
				stmt->attr.cursor_sensitivity = SQL_SENSITIVE;
			else
				stmt->attr.cursor_sensitivity = SQL_INSENSITIVE;
			stmt->attr.cursor_scrollable = SQL_SCROLLABLE;
			break;
		case SQL_CURSOR_FORWARD_ONLY:
			stmt->attr.cursor_scrollable = SQL_NONSCROLLABLE;
			break;
		default:
			odbc_errs_add(&stmt->errs, "HY092", NULL);
			ODBC_EXIT_(stmt);
		}
		stmt->attr.cursor_type = ui;
		break;
	case SQL_ATTR_ENABLE_AUTO_IPD:
		if (stmt->attr.enable_auto_ipd != ui) {
			odbc_errs_add(&stmt->errs, "HYC00", NULL);
			break;
		}
		stmt->attr.enable_auto_ipd = ui;
		break;
	case SQL_ATTR_FETCH_BOOKMARK_PTR:
		stmt->attr.fetch_bookmark_ptr = ValuePtr;
		break;
	case SQL_ATTR_IMP_ROW_DESC:
	case SQL_ATTR_IMP_PARAM_DESC:
		odbc_errs_add(&stmt->errs, "HY017", NULL);
		break;
		/* TODO what's is this ??? */
	case SQL_ATTR_KEYSET_SIZE:
		stmt->attr.keyset_size = ui;
		break;
		/* TODO max length of data returned. Use SQL TEXTSIZE or just truncate ?? */
	case SQL_ATTR_MAX_LENGTH:
		if (stmt->attr.max_length != ui) {
			odbc_errs_add(&stmt->errs, "01S02", NULL);
			break;
		}
		stmt->attr.max_length = ui;
		break;
		/* TODO max row returned. Use SET ROWCOUNT */
	case SQL_ATTR_MAX_ROWS:
		if (stmt->attr.max_rows != ui) {
			odbc_errs_add(&stmt->errs, "01S02", NULL);
			break;
		}
		stmt->attr.max_rows = ui;
		break;
	case SQL_ATTR_METADATA_ID:
		stmt->attr.metadata_id = ui;
		break;
		/* TODO use it !!! */
	case SQL_ATTR_NOSCAN:
		stmt->attr.noscan = ui;
		break;
	case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
		stmt->apd->header.sql_desc_bind_offset_ptr = lp;
		break;
	case SQL_ATTR_PARAM_BIND_TYPE:
		stmt->apd->header.sql_desc_bind_type = ui;
		break;
	case SQL_ATTR_PARAM_OPERATION_PTR:
		stmt->apd->header.sql_desc_array_status_ptr = usip;
		break;
	case SQL_ATTR_PARAM_STATUS_PTR:
		stmt->ipd->header.sql_desc_array_status_ptr = usip;
		break;
	case SQL_ATTR_PARAMS_PROCESSED_PTR:
		stmt->ipd->header.sql_desc_rows_processed_ptr = ulp;
		break;
		/* allow to exec procedure multiple time */
	case SQL_ATTR_PARAMSET_SIZE:
		stmt->apd->header.sql_desc_array_size = ui;
		break;
	case SQL_ATTR_QUERY_TIMEOUT:
		stmt->attr.query_timeout = ui;
		break;
		/* retrieve data after positioning the cursor */
	case SQL_ATTR_RETRIEVE_DATA:
		/* TODO cursors */
		if (stmt->attr.retrieve_data != ui) {
			odbc_errs_add(&stmt->errs, "01S02", NULL);
			break;
		}
		stmt->attr.retrieve_data = ui;
		break;
	case SQL_ATTR_ROW_ARRAY_SIZE:
		stmt->ard->header.sql_desc_array_size = ui;
		break;
	case SQL_ATTR_ROW_BIND_OFFSET_PTR:
		/* TODO test what happen with column-wise and row-wise bindings and SQLGetData */
		stmt->ard->header.sql_desc_bind_offset_ptr = lp;
		break;
#if SQL_BIND_TYPE != SQL_ATTR_ROW_BIND_TYPE
	case SQL_BIND_TYPE:	/* although this is ODBC2 we must support this attribute */
#endif
	case SQL_ATTR_ROW_BIND_TYPE:
		stmt->ard->header.sql_desc_bind_type = ui;
		break;
	case SQL_ATTR_ROW_NUMBER:
		odbc_errs_add(&stmt->errs, "HY092", NULL);
		break;
	case SQL_ATTR_ROW_OPERATION_PTR:
		stmt->ard->header.sql_desc_array_status_ptr = usip;
		break;
	case SQL_ATTR_ROW_STATUS_PTR:
		stmt->ird->header.sql_desc_array_status_ptr = usip;
		break;
	case SQL_ATTR_ROWS_FETCHED_PTR:
		stmt->ird->header.sql_desc_rows_processed_ptr = ulp;
		break;
	case SQL_ATTR_SIMULATE_CURSOR:
		/* TODO cursors */
		if (stmt->cursor) {
			odbc_errs_add(&stmt->errs, "24000", NULL);
			break;
		}
		if (stmt->attr.simulate_cursor != ui) {
			odbc_errs_add(&stmt->errs, "01S02", NULL);
			break;
		}
		stmt->attr.simulate_cursor = ui;
		break;
	case SQL_ATTR_USE_BOOKMARKS:
		if (stmt->cursor) {
			odbc_errs_add(&stmt->errs, "24000", NULL);
			break;
		}
		stmt->attr.use_bookmarks = ui;
		break;
	case SQL_ROWSET_SIZE:	/* although this is ODBC2 we must support this attribute */
		if (((TDS_INTPTR) ValuePtr) < 1) {
			odbc_errs_add(&stmt->errs, "HY024", NULL);
			break;
		}
		stmt->sql_rowset_size = ui;
		break;
	case SQL_SOPT_SS_QUERYNOTIFICATION_TIMEOUT:
		if (ui < 1) {
			odbc_errs_add(&stmt->errs, "HY024", NULL);
			break;
		}
		stmt->attr.qn_timeout = ui;
		break;
	case SQL_SOPT_SS_QUERYNOTIFICATION_MSGTEXT:
		if (!IS_VALID_LEN(StringLength)) {
			odbc_errs_add(&stmt->errs, "HY090", NULL);
			break;
		}
		if (!odbc_dstr_copy_oct(stmt->dbc, &stmt->attr.qn_msgtext, StringLength, (ODBC_CHAR*) ValuePtr)) {
			odbc_errs_add(&stmt->errs, "HY001", NULL);
			break;
		}
		break;
	case SQL_SOPT_SS_QUERYNOTIFICATION_OPTIONS:
		if (!IS_VALID_LEN(StringLength)) {
			odbc_errs_add(&stmt->errs, "HY090", NULL);
			break;
		}
		if (!odbc_dstr_copy_oct(stmt->dbc, &stmt->attr.qn_options, StringLength, (ODBC_CHAR*) ValuePtr)) {
			odbc_errs_add(&stmt->errs, "HY001", NULL);
			break;
		}
		break;
	default:
		odbc_errs_add(&stmt->errs, "HY092", NULL);
		break;
	}
	ODBC_EXIT_(stmt);
}

#if (ODBCVER >= 0x0300)
SQLRETURN ODBC_PUBLIC ODBC_API
SQLSetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLSetStmtAttr(%p, %d, %p, %d)\n", 
			hstmt, (int)Attribute, ValuePtr, (int)StringLength);

	return _SQLSetStmtAttr(hstmt, Attribute, ValuePtr, StringLength _wide0);
}

#ifdef ENABLE_ODBC_WIDE
SQLRETURN ODBC_PUBLIC ODBC_API
SQLSetStmtAttrW(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLSetStmtAttr(%p, %d, %p, %d)\n",
			hstmt, (int)Attribute, ValuePtr, (int)StringLength);

	return _SQLSetStmtAttr(hstmt, Attribute, ValuePtr, StringLength, 1);
}
#endif
#endif

SQLRETURN ODBC_PUBLIC ODBC_API
SQLSetStmtOption(SQLHSTMT hstmt, SQLUSMALLINT fOption, SQLULEN vParam)
{
	tdsdump_log(TDS_DBG_FUNC, "SQLSetStmtOption(%p, %u, %u)\n", hstmt, fOption, (unsigned)vParam);

	/* XXX: Lost precision */
	return _SQLSetStmtAttr(hstmt, (SQLINTEGER) fOption, (SQLPOINTER) (TDS_INTPTR) vParam, SQL_NTS _wide0);
}

ODBC_FUNC(SQLSpecialColumns, (P(SQLHSTMT,hstmt), P(SQLUSMALLINT,fColType), PCHARIN(CatalogName,SQLSMALLINT),
	PCHARIN(SchemaName,SQLSMALLINT), PCHARIN(TableName,SQLSMALLINT),
	P(SQLUSMALLINT,fScope), P(SQLUSMALLINT,fNullable) WIDE))
{
	int retcode;
	char nullable, scope, col_type;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLSpecialColumns(%p, %d, %p, %d, %p, %d, %p, %d, %d, %d)\n", 
			hstmt, fColType, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName, fScope, fNullable);

#ifdef TDS_NO_DM
	/* Check column type */
	if (fColType != SQL_BEST_ROWID && fColType != SQL_ROWVER) {
		odbc_errs_add(&stmt->errs, "HY097", NULL);
		ODBC_EXIT_(stmt);
	}

	/* check our buffer lengths */
	if (!IS_VALID_LEN(cbCatalogName) || !IS_VALID_LEN(cbSchemaName) || !IS_VALID_LEN(cbTableName)) {
		odbc_errs_add(&stmt->errs, "HY090", NULL);
		ODBC_EXIT_(stmt);
	}

	/* Check nullable */
	if (fNullable != SQL_NO_NULLS && fNullable != SQL_NULLABLE) {
		odbc_errs_add(&stmt->errs, "HY099", NULL);
		ODBC_EXIT_(stmt);
	}

	if (!odbc_get_string_size(cbTableName, szTableName _wide)) {
		odbc_errs_add(&stmt->errs, "HY009", "SQLSpecialColumns: The table name parameter is required");
		ODBC_EXIT_(stmt);
	}

	switch (fScope) {
	case SQL_SCOPE_CURROW:
	case SQL_SCOPE_TRANSACTION:
	case SQL_SCOPE_SESSION:
		break;
	default:
		odbc_errs_add(&stmt->errs, "HY098", NULL);
		ODBC_EXIT_(stmt);
	}
#endif

	if (fNullable == SQL_NO_NULLS)
		nullable = 'O';
	else
		nullable = 'U';

	if (fScope == SQL_SCOPE_CURROW)
		scope = 'C';
	else
		scope = 'T';

	if (fColType == SQL_BEST_ROWID)
		col_type = 'R';
	else
		col_type = 'V';

	retcode =
		odbc_stat_execute(stmt _wide, "sp_special_columns", TDS_IS_MSSQL(stmt->dbc->tds_socket) ? 7 : 4, "O", szTableName,
				  cbTableName, "O", szSchemaName, cbSchemaName, "O@qualifier", szCatalogName, cbCatalogName,
				  "!@col_type", &col_type, 1, "!@scope", &scope, 1, "!@nullable", &nullable, 1,
				  "V@ODBCVer", (char*) NULL, 0);
	if (SQL_SUCCEEDED(retcode) && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) {
		odbc_col_setname(stmt, 5, "COLUMN_SIZE");
		odbc_col_setname(stmt, 6, "BUFFER_LENGTH");
		odbc_col_setname(stmt, 7, "DECIMAL_DIGITS");
		if (TDS_IS_SYBASE(stmt->dbc->tds_socket))
			stmt->special_row = ODBC_SPECIAL_SPECIALCOLUMNS;
	}
	ODBC_EXIT_(stmt);
}

ODBC_FUNC(SQLStatistics, (P(SQLHSTMT,hstmt), PCHARIN(CatalogName,SQLSMALLINT),
	PCHARIN(SchemaName,SQLSMALLINT), PCHARIN(TableName,SQLSMALLINT), P(SQLUSMALLINT,fUnique),
	P(SQLUSMALLINT,fAccuracy) WIDE))
{
	int retcode;
	char unique, accuracy;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLStatistics(%p, %p, %d, %p, %d, %p, %d, %d, %d)\n", 
			hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName, fUnique, fAccuracy);

#ifdef TDS_NO_DM
	/* check our buffer lengths */
	if (!IS_VALID_LEN(cbCatalogName) || !IS_VALID_LEN(cbSchemaName) || !IS_VALID_LEN(cbTableName)) {
		odbc_errs_add(&stmt->errs, "HY090", NULL);
		ODBC_EXIT_(stmt);
	}

	/* check our uniqueness value */
	if (fUnique != SQL_INDEX_UNIQUE && fUnique != SQL_INDEX_ALL) {
		odbc_errs_add(&stmt->errs, "HY100", NULL);
		ODBC_EXIT_(stmt);
	}

	/* check our accuracy value */
	if (fAccuracy != SQL_QUICK && fAccuracy != SQL_ENSURE) {
		odbc_errs_add(&stmt->errs, "HY101", NULL);
		ODBC_EXIT_(stmt);
	}

	if (!odbc_get_string_size(cbTableName, szTableName _wide)) {
		odbc_errs_add(&stmt->errs, "HY009", NULL);
		ODBC_EXIT_(stmt);
	}
#endif

	if (fAccuracy == SQL_ENSURE)
		accuracy = 'E';
	else
		accuracy = 'Q';

	if (fUnique == SQL_INDEX_UNIQUE)
		unique = 'Y';
	else
		unique = 'N';

	retcode =
		odbc_stat_execute(stmt _wide, "sp_statistics", TDS_IS_MSSQL(stmt->dbc->tds_socket) ? 5 : 4, "O@table_qualifier",
				  szCatalogName, cbCatalogName, "O@table_owner", szSchemaName, cbSchemaName, "O@table_name",
				  szTableName, cbTableName, "!@is_unique", &unique, 1, "!@accuracy", &accuracy, 1);
	if (SQL_SUCCEEDED(retcode) && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) {
		odbc_col_setname(stmt, 1, "TABLE_CAT");
		odbc_col_setname(stmt, 2, "TABLE_SCHEM");
		odbc_col_setname(stmt, 8, "ORDINAL_POSITION");
		odbc_col_setname(stmt, 10, "ASC_OR_DESC");
	}
	ODBC_EXIT_(stmt);
}

ODBC_FUNC(SQLTables, (P(SQLHSTMT,hstmt), PCHARIN(CatalogName,SQLSMALLINT),
	PCHARIN(SchemaName,SQLSMALLINT), PCHARIN(TableName,SQLSMALLINT), PCHARIN(TableType,SQLSMALLINT) WIDE))
{
	int retcode;
	const char *proc = NULL;
	int wildcards;
	TDSSOCKET *tds;
	DSTR schema_name, catalog_name, table_type;

	ODBC_ENTER_HSTMT;

	tds_dstr_init(&schema_name);
	tds_dstr_init(&catalog_name);
	tds_dstr_init(&table_type);

	tdsdump_log(TDS_DBG_FUNC, "SQLTables(%p, %p, %d, %p, %d, %p, %d, %p, %d)\n", 
			hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName, szTableType, cbTableType);

	tds = stmt->dbc->tds_socket;

	if (!odbc_dstr_copy(stmt->dbc, &catalog_name, cbCatalogName, szCatalogName)
	    || !odbc_dstr_copy(stmt->dbc, &schema_name, cbSchemaName, szSchemaName)
	    || !odbc_dstr_copy(stmt->dbc, &table_type, cbTableType, szTableType)) {
		tds_dstr_free(&schema_name);
		tds_dstr_free(&catalog_name);
		tds_dstr_free(&table_type);
		odbc_errs_add(&stmt->errs, "HY001", NULL);
		ODBC_EXIT_(stmt);
	}

	/* support wildcards on catalog (only odbc 3) */
	wildcards = 0;
	if (stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3 && stmt->dbc->attr.metadata_id == SQL_FALSE &&
	    (strchr(tds_dstr_cstr(&catalog_name), '%') || strchr(tds_dstr_cstr(&catalog_name), '_')))
		wildcards = 1;

	proc = "sp_tables";
	if (!tds_dstr_isempty(&catalog_name)) {
		if (wildcards) {
			/* if catalog specified and wildcards use sp_tableswc under mssql2k */
			if (TDS_IS_MSSQL(tds) && tds_conn(tds)->product_version >= TDS_MS_VER(8,0,0)) {
				proc = "sp_tableswc";
				if (tds_dstr_isempty(&schema_name))
					tds_dstr_copy(&schema_name, "%");
			}
			/*
			 * TODO support wildcards on catalog even for Sybase
			 * first execute a select name from master..sysdatabases where name like catalog quoted
			 * build a db1..sp_tables args db2..sp_tables args ... query
			 * collapse results in a single recordset (how??)
			 */
		} else {
			/* if catalog specified and not wildcards use catatog on name (catalog..sp_tables) */
			proc = "..sp_tables";
		}
	}

	/* fix type if needed quoting it */
	if (!tds_dstr_isempty(&table_type)) {
		int to_fix = 0;
		int elements = 0;
		const char *p = tds_dstr_cstr(&table_type);
		const char *const end = p + tds_dstr_len(&table_type);

		for (;;) {
			const char *begin = p;

			p = (const char*) memchr(p, ',', end - p);
			if (!p)
				p = end;
			++elements;
			if ((p - begin) < 2 || begin[0] != '\'' || p[-1] != '\'')
				to_fix = 1;
			if (p >= end)
				break;
			++p;
		}
		/* fix it */
		tdsdump_log(TDS_DBG_INFO1, "to_fix %d elements %d\n", to_fix, elements);
		if (to_fix) {
			char *dst, *type;

			tdsdump_log(TDS_DBG_INFO1, "fixing type elements\n");
			type = (char *) malloc(tds_dstr_len(&table_type) + elements * 2 + 3);
			if (!type) {
				odbc_errs_add(&stmt->errs, "HY001", NULL);
				ODBC_EXIT_(stmt);
			}
			p = tds_dstr_cstr(&table_type);
			dst = type;
			for (;;) {
				const char *begin = p;

				p = (const char*) memchr(p, ',', end - p);
				if (!p)
					p = end;
				if ((p - begin) < 2 || begin[0] != '\'' || p[-1] != '\'') {
					*dst++ = '\'';
					memcpy(dst, begin, p - begin);
					dst += p - begin;
					*dst++ = '\'';
				} else {
					memcpy(dst, begin, p - begin);
					dst += p - begin;
				}
				if (p >= end)
					break;
				*dst++ = *p++;
			}
			*dst = 0;
			tds_dstr_set(&table_type, type);
		}
	}

	/* special case for catalog list */
	if (strcmp(tds_dstr_cstr(&catalog_name), "%") == 0 && cbTableName <= 0 && cbSchemaName <= 0) {
		retcode =
			odbc_stat_execute(stmt _wide, "sp_tables @table_name='', @table_owner='', @table_qualifier='%' ", 0);
	} else {
		retcode =
			odbc_stat_execute(stmt _wide, proc, 4, "P@table_name", szTableName, cbTableName,
				"!P@table_owner", tds_dstr_cstr(&schema_name), tds_dstr_len(&schema_name),
				"!P@table_qualifier", tds_dstr_cstr(&catalog_name), tds_dstr_len(&catalog_name),
				"!@table_type", tds_dstr_cstr(&table_type), tds_dstr_len(&table_type));
	}
	tds_dstr_free(&schema_name);
	tds_dstr_free(&catalog_name);
	tds_dstr_free(&table_type);
	if (SQL_SUCCEEDED(retcode) && stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) {
		odbc_col_setname(stmt, 1, "TABLE_CAT");
		odbc_col_setname(stmt, 2, "TABLE_SCHEM");
	}
	ODBC_EXIT_(stmt);
}

/** 
 * Log a useful message about unimplemented options
 * Defying belief, Microsoft defines mutually exclusive options that
 * some ODBC implementations #define as duplicate values (meaning, of course, 
 * that they couldn't be implemented in the same function because they're 
 * indistinguishable.  
 * 
 * Those duplicates are commented out below.
 */
static void
odbc_log_unimplemented_type(const char function_name[], int fType)
{
	const char *name, *category;

	switch (fType) {
#ifdef SQL_ALTER_SCHEMA
	case SQL_ALTER_SCHEMA:
		name = "SQL_ALTER_SCHEMA";
		category = "Supported SQL";
		break;
#endif
#ifdef SQL_ANSI_SQL_DATETIME_LITERALS
	case SQL_ANSI_SQL_DATETIME_LITERALS:
		name = "SQL_ANSI_SQL_DATETIME_LITERALS";
		category = "Supported SQL";
		break;
#endif
	case SQL_COLLATION_SEQ:
		name = "SQL_COLLATION_SEQ";
		category = "Data Source Information";
		break;
	case SQL_CONVERT_BIGINT:
		name = "SQL_CONVERT_BIGINT";
		category = "Conversion Information";
		break;
	case SQL_CONVERT_DATE:
		name = "SQL_CONVERT_DATE";
		category = "Conversion Information";
		break;
	case SQL_CONVERT_DOUBLE:
		name = "SQL_CONVERT_DOUBLE";
		category = "Conversion Information";
		break;
	case SQL_CONVERT_INTERVAL_DAY_TIME:
		name = "SQL_CONVERT_INTERVAL_DAY_TIME";
		category = "Conversion Information";
		break;
	case SQL_CONVERT_INTERVAL_YEAR_MONTH:
		name = "SQL_CONVERT_INTERVAL_YEAR_MONTH";
		category = "Conversion Information";
		break;
	case SQL_DM_VER:
		name = "SQL_DM_VER";
		category = "Added for ODBC 3.x";
		break;
	case SQL_DRIVER_HDESC:
		name = "SQL_DRIVER_HDESC";
		category = "Driver Information";
		break;
	case SQL_DRIVER_HLIB:
		name = "SQL_DRIVER_HLIB";
		category = "Driver Information";
		break;
#ifdef SQL_ODBC_STANDARD_CLI_CONFORMANCE
	case SQL_ODBC_STANDARD_CLI_CONFORMANCE:
		name = "SQL_ODBC_STANDARD_CLI_CONFORMANCE";
		category = "Driver Information";
		break;
#endif
	case SQL_USER_NAME:
		name = "SQL_USER_NAME";
		category = "Data Source Information";
		break;
	/* TODO extension SQL_INFO_SS_NETLIB_NAME ?? */
	default:
		name = "unknown";
		category = "unknown";
		break;
	}

	tdsdump_log(TDS_DBG_INFO1, "not implemented: %s: option/type %d(%s) [category %s]\n", function_name, fType, name,
		    category);

	return;
}

static int
odbc_quote_metadata(TDS_DBC * dbc, char type, char *dest, DSTR * dstr)
{
	int unquote = 0;
	char prev, buf[1200], *dst;
	const char *s = tds_dstr_cstr(dstr);
	int len = tds_dstr_len(dstr);

	if (!type || (type == 'O' && dbc->attr.metadata_id == SQL_FALSE))
		return tds_quote_string(dbc->tds_socket, dest, s, len);

	/* where we can have ID or PV */
	assert(type == 'P' || (type == 'O' && dbc->attr.metadata_id != SQL_FALSE));

	/* ID ? */
	if (dbc->attr.metadata_id != SQL_FALSE) {
		/* strip leading and trailing spaces */
		while (len > 0 && *s == ' ')
			++s, --len;
		while (len > 0 && s[len - 1] == ' ')
			--len;
		/* unquote if necessary */
		if (len > 2 && *s == '\"' && s[len - 1] == '\"') {
			++s, len -= 2;
			unquote = 1;
		}
	}

	/* limit string/id lengths */
	if (len > 384)
		len = 384;

	if (!dest)
		dest = buf;
	dst = dest;

	/*
	 * handle patterns 
	 * "'" -> "''" (normal string quoting)
	 *
	 * if metadata_id is FALSE
	 * "\_" -> "[_]"
	 * "\%" -> "[%]"
	 * "[" -> "[[]"
	 *
	 * if metadata_id is TRUE
	 * "\"\"" -> "\"" (if unquote id)
	 * "_" -> "[_]"
	 * "%" -> "[%]"
	 * "[" -> "[[]"
	 */
	prev = 0;
	*dst++ = '\'';
	for (; --len >= 0; ++s) {
		switch (*s) {
		case '\'':
			*dst++ = '\'';
			break;
		case '\"':
			if (unquote && prev == '\"') {
				prev = 0;	/* avoid "\"\"\"" -> "\"" */
				--dst;
				continue;
			}
			break;
		case '_':
		case '%':
			if (dbc->attr.metadata_id == SQL_FALSE) {
				if (prev != '\\')
					break;
				--dst;
			}
		case '[':
			if (type != 'P')
				break;
			/* quote search string */
			*dst++ = '[';
			*dst++ = *s;
			*dst++ = ']';
			prev = 0;
			continue;
		}
		*dst++ = prev = *s;
	}
	*dst++ = '\'';
	return dst - dest;
}

#define ODBC_MAX_STAT_PARAM 8
static SQLRETURN
odbc_stat_execute(TDS_STMT * stmt _WIDE, const char *begin, int nparams, ...)
{
	struct param
	{
		DSTR value;
		char *name;
		char type;
	}
	params[ODBC_MAX_STAT_PARAM];
	int i, len, param_qualifier = -1;
	char *proc, *p;
	int retcode;
	va_list marker;


	assert(nparams < ODBC_MAX_STAT_PARAM);

	/* read all params and calc len required */
	va_start(marker, nparams);
	len = strlen(begin) + 3;
	for (i = 0; i < nparams; ++i) {
		int param_len, convert = 1;
		DSTR *out;

		p = va_arg(marker, char *);

		if (*p == '!') {
			convert = 0;
			++p;
		}

		switch (*p) {
		case 'V':	/* ODBC version */
			len += strlen(p) + 3;
		case 'O':	/* ordinary arguments */
		case 'P':	/* pattern value arguments */
			params[i].type = *p++;
			break;
		default:
			params[i].type = 0;	/* ordinary type */
			break;
		}
		params[i].name = p;

		p = va_arg(marker, char *);
		param_len = va_arg(marker, int);
		tds_dstr_init(&params[i].value);
#ifdef ENABLE_ODBC_WIDE
		if (!convert)
			out = tds_dstr_copyn(&params[i].value, p, param_len);
		else
			out = odbc_dstr_copy(stmt->dbc, &params[i].value, param_len, (ODBC_CHAR *) p);
#else
		out = odbc_dstr_copy(stmt->dbc, &params[i].value, param_len, (ODBC_CHAR *) p);
#endif
		if (!out) {
			while (--i >= 0)
				tds_dstr_free(&params[i].value);
			odbc_errs_add(&stmt->errs, "HY001", NULL);
			return SQL_ERROR;
		}
		if (!tds_dstr_isempty(&params[i].value)) {
			len += strlen(params[i].name) + odbc_quote_metadata(stmt->dbc, params[i].type, NULL, 
									    &params[i].value) + 3;
			if (begin[0] == '.' && strstr(params[i].name, "qualifier")) {
				len += tds_quote_id(stmt->dbc->tds_socket, NULL,
						    tds_dstr_cstr(&params[i].value), tds_dstr_len(&params[i].value));
				param_qualifier = i;
			}
		}

	}
	va_end(marker);

	/* allocate space for string */
	if (!(proc = (char *) malloc(len))) {
		for (i = 0; i < nparams; ++i)
			tds_dstr_free(&params[i].value);
		odbc_errs_add(&stmt->errs, "HY001", NULL);
		return SQL_ERROR;
	}

	/* build string */
	p = proc;
	if (param_qualifier >= 0)
		p += tds_quote_id(stmt->dbc->tds_socket, p, tds_dstr_cstr(&params[param_qualifier].value), tds_dstr_len(&params[param_qualifier].value));
	strcpy(p, begin);
	p += strlen(begin);
	*p++ = ' ';
	for (i = 0; i < nparams; ++i) {
		if (tds_dstr_isempty(&params[i].value) && params[i].type != 'V')
			continue;
		if (params[i].name[0]) {
			strcpy(p, params[i].name);
			p += strlen(params[i].name);
			*p++ = '=';
		}
		if (params[i].type != 'V')
			p += odbc_quote_metadata(stmt->dbc, params[i].type, p, &params[i].value);
		else
			*p++ = (stmt->dbc->env->attr.odbc_version == SQL_OV_ODBC3) ? '3': '2';
		*p++ = ',';
		tds_dstr_free(&params[i].value);
	}
	*--p = '\0';
	assert(p - proc + 1 <= len);

	/* set it */
	/* FIXME is neither mb or wide, is always utf encoded !!! */
	retcode = odbc_set_stmt_query(stmt, (ODBC_CHAR *) proc, p - proc _wide0);
	free(proc);

	if (retcode != SQL_SUCCESS)
		ODBC_RETURN(stmt, retcode);

	/* execute it */
	retcode = _SQLExecute(stmt);
	if (SQL_SUCCEEDED(retcode))
		odbc_upper_column_names(stmt);

	ODBC_RETURN(stmt, retcode);
}

static SQLRETURN
odbc_free_dynamic(TDS_STMT * stmt)
{
	TDSSOCKET *tds;

	if (!stmt->dyn)
		return TDS_SUCCESS;

	tds = stmt->dbc->tds_socket;
	if (!tds_needs_unprepare(tds, stmt->dyn)) {
		tds_release_dynamic(&stmt->dyn);
		return SQL_SUCCESS;
	}

	if (!odbc_lock_statement(stmt))
		return SQL_ERROR;

	tds = stmt->tds;
	if (TDS_SUCCEED(tds_submit_unprepare(tds, stmt->dyn))
	    && TDS_SUCCEED(tds_process_simple_query(tds))) {
		tds_release_dynamic(&stmt->dyn);
		return SQL_SUCCESS;
	}

	/* TODO if fail add to odbc to free later, when we are in idle */
	ODBC_SAFE_ERROR(stmt);
	return SQL_ERROR;
}

static SQLRETURN
odbc_free_cursor(TDS_STMT * stmt)
{
	TDSCURSOR *cursor = stmt->cursor;
	TDSSOCKET *tds;
	int error = 1;

	if (!cursor)
		return SQL_SUCCESS;

	if (!odbc_lock_statement(stmt))
		return SQL_ERROR;

	tds = stmt->tds;
	cursor->status.dealloc   = TDS_CURSOR_STATE_REQUESTED;
	/* TODO if fail add to odbc to free later, when we are in idle */
	if (TDS_SUCCEED(tds_cursor_close(tds, cursor))) {
		if (TDS_SUCCEED(tds_process_simple_query(tds)))
			error = 0;
		/* TODO check error */
		tds_cursor_dealloc(tds, cursor);
	}
	if (error) {
		ODBC_SAFE_ERROR(stmt);
		return SQL_ERROR;
	}
	tds_release_cursor(&stmt->cursor);
	return SQL_SUCCESS;
}

SQLRETURN ODBC_PUBLIC ODBC_API
SQLSetScrollOptions(SQLHSTMT hstmt, SQLUSMALLINT fConcurrency, SQLLEN crowKeyset, SQLUSMALLINT crowRowset)
{
	SQLUSMALLINT info;
	SQLUINTEGER value, check;
	SQLUINTEGER cursor_type;

	ODBC_ENTER_HSTMT;

	tdsdump_log(TDS_DBG_FUNC, "SQLSetScrollOptions(%p, %u, %ld, %u)\n", 
			hstmt, fConcurrency, (long int) crowKeyset, crowRowset);

	if (!stmt->dbc->cursor_support) {
		odbc_errs_add(&stmt->errs, "HYC00", NULL);
		ODBC_EXIT_(stmt);
	}

	if (stmt->cursor) {
		odbc_errs_add(&stmt->errs, "24000", NULL);
		ODBC_EXIT_(stmt);
	}

	switch (crowKeyset) {
	case SQL_SCROLL_FORWARD_ONLY:
		info = SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2;
		cursor_type = SQL_CURSOR_FORWARD_ONLY;
		break;
	case SQL_SCROLL_STATIC:
		info = SQL_STATIC_CURSOR_ATTRIBUTES2;
		cursor_type = SQL_CURSOR_STATIC;
		break;
	case SQL_SCROLL_KEYSET_DRIVEN:
		info = SQL_KEYSET_CURSOR_ATTRIBUTES2;
		cursor_type = SQL_CURSOR_KEYSET_DRIVEN;
		break;
	case SQL_SCROLL_DYNAMIC:
		info = SQL_DYNAMIC_CURSOR_ATTRIBUTES2;
		cursor_type = SQL_CURSOR_DYNAMIC;
		break;
	default:
		if (crowKeyset > crowRowset) {
			info = SQL_KEYSET_CURSOR_ATTRIBUTES2;
			cursor_type = SQL_CURSOR_KEYSET_DRIVEN;
			break;
		}
		
		odbc_errs_add(&stmt->errs, "HY107", NULL);
		ODBC_EXIT_(stmt);
	}

	switch (fConcurrency) {
	case SQL_CONCUR_READ_ONLY:
		check = SQL_CA2_READ_ONLY_CONCURRENCY;
		break;
	case SQL_CONCUR_LOCK:
		check = SQL_CA2_LOCK_CONCURRENCY;
		break;
	case SQL_CONCUR_ROWVER:
		check = SQL_CA2_OPT_ROWVER_CONCURRENCY;
		break;
	case SQL_CONCUR_VALUES:
		check = SQL_CA2_OPT_VALUES_CONCURRENCY;
		break;
	default:
		odbc_errs_add(&stmt->errs, "HY108", NULL);
		ODBC_EXIT_(stmt);
	}

	value = 0;
	_SQLGetInfo(stmt->dbc, info, &value, sizeof(value), NULL _wide0);

	if ((value & check) == 0) {
		odbc_errs_add(&stmt->errs, "HYC00", NULL);
		ODBC_EXIT_(stmt);
	}

	_SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) (TDS_INTPTR) cursor_type, 0 _wide0);
	_SQLSetStmtAttr(hstmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER) (TDS_INTPTR) fConcurrency, 0 _wide0);
	_SQLSetStmtAttr(hstmt, SQL_ATTR_KEYSET_SIZE, (SQLPOINTER) (TDS_INTPTR) crowKeyset, 0 _wide0);
	_SQLSetStmtAttr(hstmt, SQL_ROWSET_SIZE, (SQLPOINTER) (TDS_INTPTR) crowRowset, 0 _wide0);

	ODBC_EXIT_(stmt);
}

#include "odbc_export.h"

