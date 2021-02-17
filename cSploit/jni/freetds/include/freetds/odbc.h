/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2004-2010  Frediano Ziglio
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

#ifndef _sql_h_
#define _sql_h_

#include <freetds/tds.h>
#include <freetds/thread.h>

#if defined(UNIXODBC) || defined(TDS_NO_DM)
#include <sql.h>
#include <sqlext.h>
#ifdef UNIXODBC
#include <odbcinst.h>
#endif
#else /* IODBC */
#include <isql.h>
#include <isqlext.h>
#ifdef HAVE_IODBCINST_H
#include <iodbcinst.h>
#endif /* HAVE_IODBCINST_H */
#endif

#ifndef HAVE_SQLLEN
#ifndef SQLULEN
#define SQLULEN SQLUINTEGER
#endif
#ifndef SQLLEN
#define SQLLEN SQLINTEGER
#endif
#endif

#ifndef HAVE_SQLSETPOSIROW
#define SQLSETPOSIROW SQLUSMALLINT
#endif

#ifndef HAVE_SQLROWOFFSET
#define SQLROWOFFSET SQLLEN
#endif

#ifndef HAVE_SQLROWSETSIZE
#define SQLROWSETSIZE SQLULEN
#endif

#ifndef SQL_COPT_SS_BASE
#define SQL_COPT_SS_BASE	1200
#endif

#ifndef SQL_COPT_SS_MARS_ENABLED
#define SQL_COPT_SS_MARS_ENABLED	(SQL_COPT_SS_BASE+24)
#endif

#ifndef SQL_MARS_ENABLED_NO
#define SQL_MARS_ENABLED_NO	0
#endif

#ifndef SQL_MARS_ENABLED_YES
#define SQL_MARS_ENABLED_YES	1
#endif

#ifndef SQL_SS_TIME2
#define SQL_SS_TIME2	(-154)
#endif

#ifndef SQL_SS_TIMESTAMPOFFSET
#define SQL_SS_TIMESTAMPOFFSET	(-155)
#endif

/*
 * these types are used from conversion from client to server
 */
#ifndef SQL_C_SS_TIME2
#define SQL_C_SS_TIME2	(0x4000)
#endif

#ifndef SQL_C_SS_TIMESTAMPOFFSET
#define SQL_C_SS_TIMESTAMPOFFSET	(0x4001)
#endif

#ifdef __cplusplus
extern "C"
{
#if 0
}
#endif
#endif

/* $Id: tdsodbc.h,v 1.134 2012-03-09 21:51:21 freddy77 Exp $ */

#if defined(__GNUC__) && __GNUC__ >= 4 && !defined(__MINGW32__)
#pragma GCC visibility push(hidden)
#define ODBC_API SQL_API __attribute__((externally_visible))
#else
#define ODBC_API SQL_API
#endif

#if (defined(_WIN32) || defined(__CYGWIN__)) && defined(__GNUC__)
#  define ODBC_PUBLIC __attribute__((dllexport))
#else
#  define ODBC_PUBLIC
#endif

#define ODBC_MAX(a,b) ( (a) > (b) ? (a) : (b) )
#define ODBC_MIN(a,b) ( (a) < (b) ? (a) : (b) )

struct _sql_error
{
	const char *msg;
	char state2[6];
	char state3[6];
	TDS_UINT native;
	char *server;
	int linenum;
	int msgstate;
	int row;
};

struct _sql_errors
{
	struct _sql_error *errs;
	int num_errors;
	SQLRETURN lastrc;
	char ranked;
};

typedef struct _sql_errors TDS_ERRS;

#if ENABLE_EXTRA_CHECKS
void odbc_check_struct_extra(void *p);
#else
static inline void odbc_check_struct_extra(void *p) {}
#endif

#define ODBC_RETURN(handle, rc) \
	do { odbc_check_struct_extra(handle); \
	return handle->errs.lastrc = (rc); } while(0)
#define ODBC_RETURN_(handle) \
	do { odbc_check_struct_extra(handle); \
	return handle->errs.lastrc; } while(0)

#define ODBC_EXIT(handle, rc) \
	do { SQLRETURN _odbc_rc = handle->errs.lastrc = (rc); \
	odbc_check_struct_extra(handle); \
	tds_mutex_unlock(&handle->mtx); \
	return _odbc_rc; } while(0)
#define ODBC_EXIT_(handle) \
	do { SQLRETURN _odbc_rc = handle->errs.lastrc; \
	odbc_check_struct_extra(handle); \
	tds_mutex_unlock(&handle->mtx); \
	return _odbc_rc; } while(0)


/** reset errors */
void odbc_errs_reset(struct _sql_errors *errs);

/** add an error to list */
void odbc_errs_add(struct _sql_errors *errs, const char *sqlstate, const char *msg);

/** Add an error to list. This functions is for error that came from server */
void odbc_errs_add_rdbms(struct _sql_errors *errs, TDS_UINT native, const char *sqlstate, const char *msg, int linenum,
			 int msgstate, const char *server, int row);

struct _dheader
{
	SQLSMALLINT sql_desc_alloc_type;
	SQLINTEGER sql_desc_bind_type;
	SQLULEN sql_desc_array_size;
	/* TODO SQLLEN ?? see http://support.microsoft.com/default.aspx?scid=kb;en-us;298678 */
	SQLSMALLINT sql_desc_count;
	SQLUSMALLINT *sql_desc_array_status_ptr;
	SQLULEN *sql_desc_rows_processed_ptr;
	SQLLEN *sql_desc_bind_offset_ptr;
};

struct _drecord
{
	SQLUINTEGER sql_desc_auto_unique_value;
	DSTR sql_desc_base_column_name;
	DSTR sql_desc_base_table_name;
	SQLINTEGER sql_desc_case_sensitive;
	DSTR sql_desc_catalog_name;
	SQLSMALLINT sql_desc_concise_type;
	SQLPOINTER sql_desc_data_ptr;
	SQLSMALLINT sql_desc_datetime_interval_code;
	SQLINTEGER sql_desc_datetime_interval_precision;
	SQLLEN sql_desc_display_size;
	SQLSMALLINT sql_desc_fixed_prec_scale;
	SQLLEN *sql_desc_indicator_ptr;
	DSTR sql_desc_label;
	SQLULEN sql_desc_length;
	/* this point to a constant buffer, do not free or modify */
	const char *sql_desc_literal_prefix;
	/* this point to a constant buffer, do not free or modify */
	const char *sql_desc_literal_suffix;
	DSTR sql_desc_local_type_name;
	DSTR sql_desc_name;
	SQLSMALLINT sql_desc_nullable;
	SQLINTEGER sql_desc_num_prec_radix;
	SQLLEN sql_desc_octet_length;
	SQLLEN *sql_desc_octet_length_ptr;
	SQLSMALLINT sql_desc_parameter_type;
	SQLSMALLINT sql_desc_precision;
	SQLSMALLINT sql_desc_rowver;
	SQLSMALLINT sql_desc_scale;
	DSTR sql_desc_schema_name;
	SQLSMALLINT sql_desc_searchable;
	DSTR sql_desc_table_name;
	SQLSMALLINT sql_desc_type;
	/* this point to a constant buffer, do not free or modify */
	const char *sql_desc_type_name;
	SQLSMALLINT sql_desc_unnamed;
	SQLSMALLINT sql_desc_unsigned;
	SQLSMALLINT sql_desc_updatable;
};

struct _hdesc
{
	SQLSMALLINT htype;	/* do not reorder this field */
	struct _sql_errors errs;	/* do not reorder this field */
	tds_mutex mtx;
	int type;
	SQLHANDLE parent;
	struct _dheader header;
	struct _drecord *records;
};

typedef struct _hdesc TDS_DESC;

#define DESC_IRD	1
#define DESC_IPD	2
#define DESC_ARD	3
#define DESC_APD	4

struct _heattr
{
	SQLUINTEGER connection_pooling;
	SQLUINTEGER cp_match;
	SQLINTEGER odbc_version;
	SQLINTEGER output_nts;
};

struct _hchk
{
	SQLSMALLINT htype;	/* do not reorder this field */
	struct _sql_errors errs;	/* do not reorder this field */
	tds_mutex mtx;
};

struct _henv
{
	SQLSMALLINT htype;	/* do not reorder this field */
	struct _sql_errors errs;	/* do not reorder this field */
	tds_mutex mtx;
	TDSCONTEXT *tds_ctx;
	struct _heattr attr;
};

struct _hcattr
{
	SQLUINTEGER access_mode;
	SQLUINTEGER async_enable;
	SQLUINTEGER auto_ipd;
	SQLUINTEGER autocommit;
	SQLUINTEGER connection_dead;
	SQLUINTEGER connection_timeout;
	DSTR current_catalog;
	SQLUINTEGER login_timeout;
	SQLUINTEGER metadata_id;
	SQLUINTEGER odbc_cursors;
	SQLUINTEGER packet_size;
	SQLHWND quite_mode;
	DSTR translate_lib;
	SQLUINTEGER translate_option;
	SQLUINTEGER txn_isolation;
	SQLUINTEGER mars_enabled;
	SQLUINTEGER cursor_type;
#ifdef TDS_NO_DM
	SQLUINTEGER trace;
	DSTR tracefile;
#endif
};

#define TDS_MAX_APP_DESC	100

struct _hstmt;
struct _hdbc
{
	SQLSMALLINT htype;	/* do not reorder this field */
	struct _sql_errors errs;	/* do not reorder this field */
	tds_mutex mtx;
	struct _henv *env;
	TDSSOCKET *tds_socket;
	DSTR dsn;
	DSTR server;		/* aka Instance */
#ifdef ENABLE_ODBC_WIDE
	DSTR original_charset;
	TDSICONV *mb_conv;
#endif

	/**
	 * Statement executing. This should be set AFTER sending query
	 * to avoid race condition and assure to not overwrite it if
	 * another statement is executing a query.
	 */
	struct _hstmt *current_statement;
	/** list of all statements allocated from this connection */
	struct _hstmt *stmt_list;
	struct _hcattr attr;
	/** descriptors associated to connection */
	TDS_DESC *uad[TDS_MAX_APP_DESC];
	/** <>0 if server handle cursors */
	unsigned int cursor_support;
	TDS_INT default_query_timeout;
};

struct _hsattr
{
	/* TODO remove IRD, ARD, IPD, APD from statement, do not duplicate */
/*	TDS_DESC *app_row_desc; */
/*	TDS_DESC *app_param_desc; */
	SQLUINTEGER async_enable;
	SQLUINTEGER concurrency;
	SQLUINTEGER cursor_scrollable;
	SQLUINTEGER cursor_sensitivity;
	SQLUINTEGER cursor_type;
	SQLUINTEGER enable_auto_ipd;
	SQLPOINTER fetch_bookmark_ptr;
	SQLULEN keyset_size;
	SQLULEN max_length;
	SQLULEN max_rows;
	SQLUINTEGER metadata_id;
	SQLUINTEGER noscan;
	/* apd->sql_desc_bind_offset_ptr */
	/* SQLUINTEGER *param_bind_offset_ptr; */
	/* apd->sql_desc_bind_type */
	/* SQLUINTEGER param_bind_type; */
	/* apd->sql_desc_array_status_ptr */
	/* SQLUSMALLINT *param_operation_ptr; */
	/* ipd->sql_desc_array_status_ptr */
	/* SQLUSMALLINT *param_status_ptr; */
	/* ipd->sql_desc_rows_processed_ptr */
	/* SQLUSMALLINT *params_processed_ptr; */
	/* apd->sql_desc_array_size */
	/* SQLUINTEGER paramset_size; */
	SQLUINTEGER query_timeout;
	SQLUINTEGER retrieve_data;
	/* ard->sql_desc_bind_offset_ptr */
	/* SQLUINTEGER *row_bind_offset_ptr; */
	/* ard->sql_desc_array_size */
	/* SQLUINTEGER row_array_size; */
	/* ard->sql_desc_bind_type */
	/* SQLUINTEGER row_bind_type; */
	SQLULEN row_number;
	/* ard->sql_desc_array_status_ptr */
	/* SQLUINTEGER *row_operation_ptr; */
	/* ird->sql_desc_array_status_ptr */
	/* SQLUINTEGER *row_status_ptr; */
	/* ird->sql_desc_rows_processed_ptr */
	/* SQLUINTEGER *rows_fetched_ptr; */
	SQLUINTEGER simulate_cursor;
	SQLUINTEGER use_bookmarks;
	/* SQLGetStmtAttr only */
/*	TDS_DESC *imp_row_desc; */
/*	TDS_DESC *imp_param_desc; */
	DSTR qn_msgtext;
	DSTR qn_options;
	SQLUINTEGER qn_timeout;
};

typedef enum
{
	NOT_IN_ROW,
	IN_NORMAL_ROW,
	IN_COMPUTE_ROW,
	AFTER_COMPUTE_ROW,
	PRE_NORMAL_ROW
} TDS_ODBC_ROW_STATUS;

typedef enum
{
	ODBC_SPECIAL_NONE = 0,
	ODBC_SPECIAL_GETTYPEINFO = 1,
	ODBC_SPECIAL_COLUMNS = 2,
	ODBC_SPECIAL_PROCEDURECOLUMNS = 3,
	ODBC_SPECIAL_SPECIALCOLUMNS = 4
} TDS_ODBC_SPECIAL_ROWS;

struct _hstmt
{
	SQLSMALLINT htype;	/* do not reorder this field */
	struct _sql_errors errs;	/* do not reorder this field */
	tds_mutex mtx;
	struct _hdbc *dbc;
	/** query to execute */
	char *query;
	/** socket (only if active) */
	TDSSOCKET *tds;

	/** next in list */
	struct _hstmt *next;
	/** previous in list */
	struct _hstmt *prev;

	/* begin prepared query stuff */
	char *prepared_query;
	unsigned prepared_query_is_func:1;
	unsigned prepared_query_is_rpc:1;
	unsigned need_reprepare:1;
	unsigned param_data_called:1;
	/* end prepared query stuff */

	/** parameters saved */
	TDSPARAMINFO *params;
	/** last valid parameter in params, it's a ODBC index (from 1 relative to descriptor) */
	int param_num;
	/** position in prepared query to check parameters, used only in RPC */
	char *prepared_pos;

	unsigned int curr_param_row, num_param_rows;

	/** number of parameter in current query */
	unsigned int param_count;
	int row;
	/** row count to return */
	TDS_INT8 row_count;
	/** status of row, it can happen that this flag mark that we are still parsing row, this it's normal */
	TDS_ODBC_ROW_STATUS row_status;
	/* do NOT free dynamic, free from socket or attach to connection */
	TDSDYNAMIC *dyn;
	TDS_DESC *ard, *ird, *apd, *ipd;
	TDS_DESC *orig_ard, *orig_apd;
	SQLULEN sql_rowset_size;
	struct _hsattr attr;
	DSTR cursor_name;	/* auto generated cursor name */
	TDS_ODBC_SPECIAL_ROWS special_row;
	/* do NOT free cursor, free from socket or attach to connection */
	TDSCURSOR *cursor;
};

typedef struct _henv TDS_ENV;
typedef struct _hdbc TDS_DBC;
typedef struct _hstmt TDS_STMT;
typedef struct _hchk TDS_CHK;

#define IS_HENV(x) (((TDS_CHK *)x)->htype == SQL_HANDLE_ENV)
#define IS_HDBC(x) (((TDS_CHK *)x)->htype == SQL_HANDLE_DBC)
#define IS_HSTMT(x) (((TDS_CHK *)x)->htype == SQL_HANDLE_STMT)
#define IS_HDESC(x) (((TDS_CHK *)x)->htype == SQL_HANDLE_DESC)

/* fix a bug in MingW headers */
#ifdef __MINGW32__
#if SQL_INTERVAL_YEAR == (100 + SQL_CODE_SECOND)

#undef SQL_INTERVAL_YEAR
#undef SQL_INTERVAL_MONTH
#undef SQL_INTERVAL_DAY
#undef SQL_INTERVAL_HOUR
#undef SQL_INTERVAL_MINUTE
#undef SQL_INTERVAL_SECOND
#undef SQL_INTERVAL_YEAR_TO_MONTH
#undef SQL_INTERVAL_DAY_TO_HOUR
#undef SQL_INTERVAL_DAY_TO_MINUTE
#undef SQL_INTERVAL_DAY_TO_SECOND
#undef SQL_INTERVAL_HOUR_TO_MINUTE
#undef SQL_INTERVAL_HOUR_TO_SECOND
#undef SQL_INTERVAL_MINUTE_TO_SECOND

#define SQL_INTERVAL_YEAR					(100 + SQL_CODE_YEAR)
#define SQL_INTERVAL_MONTH					(100 + SQL_CODE_MONTH)
#define SQL_INTERVAL_DAY					(100 + SQL_CODE_DAY)
#define SQL_INTERVAL_HOUR					(100 + SQL_CODE_HOUR)
#define SQL_INTERVAL_MINUTE					(100 + SQL_CODE_MINUTE)
#define SQL_INTERVAL_SECOND                	(100 + SQL_CODE_SECOND)
#define SQL_INTERVAL_YEAR_TO_MONTH			(100 + SQL_CODE_YEAR_TO_MONTH)
#define SQL_INTERVAL_DAY_TO_HOUR			(100 + SQL_CODE_DAY_TO_HOUR)
#define SQL_INTERVAL_DAY_TO_MINUTE			(100 + SQL_CODE_DAY_TO_MINUTE)
#define SQL_INTERVAL_DAY_TO_SECOND			(100 + SQL_CODE_DAY_TO_SECOND)
#define SQL_INTERVAL_HOUR_TO_MINUTE			(100 + SQL_CODE_HOUR_TO_MINUTE)
#define SQL_INTERVAL_HOUR_TO_SECOND			(100 + SQL_CODE_HOUR_TO_SECOND)
#define SQL_INTERVAL_MINUTE_TO_SECOND		(100 + SQL_CODE_MINUTE_TO_SECOND)

#endif
#endif

#ifdef _WIN32
BOOL get_login_info(HWND hwndParent, TDSLOGIN * login);
#endif

#define ODBC_PARAM_LIST \
	ODBC_PARAM(Servername) \
	ODBC_PARAM(Server) \
	ODBC_PARAM(DSN) \
	ODBC_PARAM(UID) \
	ODBC_PARAM(PWD) \
	ODBC_PARAM(Address) \
	ODBC_PARAM(Port) \
	ODBC_PARAM(TDS_Version) \
	ODBC_PARAM(Language) \
	ODBC_PARAM(Database) \
	ODBC_PARAM(TextSize) \
	ODBC_PARAM(PacketSize) \
	ODBC_PARAM(ClientCharset) \
	ODBC_PARAM(DumpFile) \
	ODBC_PARAM(DumpFileAppend) \
	ODBC_PARAM(DebugFlags) \
	ODBC_PARAM(Encryption) \
	ODBC_PARAM(Trusted_Connection) \
	ODBC_PARAM(APP) \
	ODBC_PARAM(WSID) \
	ODBC_PARAM(UseNTLMv2) \
	ODBC_PARAM(MARS_Connection) \
	ODBC_PARAM(REALM) \
	ODBC_PARAM(ServerSPN)

#define ODBC_PARAM(p) ODBC_PARAM_##p,
enum {
	ODBC_PARAM_LIST
	ODBC_PARAM_SIZE
};
#undef ODBC_PARAM


/*
 * connectparams.h
 */

typedef struct {
	const char *p;
	size_t len;
} TDS_PARSED_PARAM;

/**
 * Parses a connection string for SQLDriverConnect().
 * \param connect_string      point to connection string
 * \param connect_string_end  point to end of connection string
 * \param connection          structure where to store informations
 * \return 0 if error, 1 otherwise
 */
int odbc_parse_connect_string(TDS_ERRS *errs, const char *connect_string, const char *connect_string_end, TDSLOGIN * login, TDS_PARSED_PARAM *parsed_params);
int odbc_get_dsn_info(TDS_ERRS *errs, const char *DSN, TDSLOGIN * login);
#ifdef _WIN32
int odbc_build_connect_string(TDS_ERRS *errs, TDS_PARSED_PARAM *params, char **out);
#endif

/*
 * convert_tds2sql.c
 */
SQLLEN odbc_tds2sql(TDS_STMT * stmt, TDSCOLUMN *curcol, int srctype, TDS_CHAR * src, TDS_UINT srclen, int desttype, TDS_CHAR * dest, SQLULEN destlen, const struct _drecord *drec_ixd);

/*
 * descriptor.c
 */
TDS_DESC *desc_alloc(SQLHANDLE parent, int desc_type, int alloc_type);
SQLRETURN desc_free(TDS_DESC * desc);
SQLRETURN desc_alloc_records(TDS_DESC * desc, unsigned count);
SQLRETURN desc_copy(TDS_DESC * dest, TDS_DESC * src);
SQLRETURN desc_free_records(TDS_DESC * desc);
TDS_DBC *desc_get_dbc(TDS_DESC *desc);

/*
 * odbc.c
 */
SQLRETURN _SQLRowCount(SQLHSTMT hstmt, SQLLEN FAR * pcrow);

/*
 * odbc_checks.h
 */
#if ENABLE_EXTRA_CHECKS
/* macro */
#define CHECK_ENV_EXTRA(env) odbc_check_env_extra(env)
#define CHECK_DBC_EXTRA(dbc) odbc_check_dbc_extra(dbc)
#define CHECK_STMT_EXTRA(stmt) odbc_check_stmt_extra(stmt)
#define CHECK_DESC_EXTRA(desc) odbc_check_desc_extra(desc)
/* declarations*/
void odbc_check_env_extra(TDS_ENV * env);
void odbc_check_dbc_extra(TDS_DBC * dbc);
void odbc_check_stmt_extra(TDS_STMT * stmt);
void odbc_check_desc_extra(TDS_DESC * desc);
#else
/* macro */
#define CHECK_ENV_EXTRA(env)
#define CHECK_DBC_EXTRA(dbc)
#define CHECK_STMT_EXTRA(stmt)
#define CHECK_DESC_EXTRA(desc)
#endif

/*
 * odbc_util.h
 */

/* helpers for ODBC wide string support */
#undef _wide
#undef _WIDE
#ifdef ENABLE_ODBC_WIDE
typedef union {
	char mb[1];
	SQLWCHAR wide[1];
} ODBC_CHAR;
# define _wide ,wide
# define _wide0 ,0
# define _WIDE ,int wide
#else
# define _wide
# define _wide0
# define _WIDE
# define ODBC_CHAR SQLCHAR
#endif
int odbc_set_stmt_query(struct _hstmt *stmt, const ODBC_CHAR *sql, int sql_len _WIDE);
int odbc_set_stmt_prepared_query(struct _hstmt *stmt, const ODBC_CHAR *sql, int sql_len _WIDE);
void odbc_set_return_status(struct _hstmt *stmt, unsigned int n_row);
void odbc_set_return_params(struct _hstmt *stmt, unsigned int n_row);

SQLSMALLINT odbc_server_to_sql_type(int col_type, int col_size);
int odbc_sql_to_c_type_default(int sql_type);
int odbc_sql_to_server_type(TDSCONNECTION * conn, int sql_type, int sql_unsigned);
int odbc_c_to_server_type(int c_type);

void odbc_set_sql_type_info(TDSCOLUMN * col, struct _drecord *drec, SQLINTEGER odbc_ver);
SQLINTEGER odbc_sql_to_displaysize(int sqltype, TDSCOLUMN *col);
int odbc_get_string_size(int size, ODBC_CHAR * str _WIDE);
void odbc_rdbms_version(TDSSOCKET * tds_socket, char *pversion_string);
SQLINTEGER odbc_get_param_len(const struct _drecord *drec_axd, const struct _drecord *drec_ixd, const TDS_DESC* axd, unsigned int n_row);

#ifdef ENABLE_ODBC_WIDE
DSTR* odbc_dstr_copy_flag(TDS_DBC *dbc, DSTR *s, int size, ODBC_CHAR * str, int flag);
#define odbc_dstr_copy(dbc, s, len, out) \
	odbc_dstr_copy_flag(dbc, s, len, sizeof((out)->mb) ? (out) : (out), wide)
#define odbc_dstr_copy_oct(dbc, s, len, out) \
	odbc_dstr_copy_flag(dbc, s, len, out, wide|0x20)
#else
DSTR* odbc_dstr_copy(TDS_DBC *dbc, DSTR *s, int size, ODBC_CHAR * str);
#define odbc_dstr_copy_oct odbc_dstr_copy
#endif


SQLRETURN odbc_set_string_flag(TDS_DBC *dbc, SQLPOINTER buffer, SQLINTEGER cbBuffer, void FAR * pcbBuffer, const char *s, int len, int flag);
#ifdef ENABLE_ODBC_WIDE
#define odbc_set_string(dbc, buf, buf_len, out_len, s, s_len) \
	odbc_set_string_flag(dbc, sizeof((buf)->mb) ? (buf) : (buf), buf_len, out_len, s, s_len, (wide) | (sizeof(*(out_len)) == sizeof(SQLSMALLINT)?0:0x10))
#define odbc_set_string_oct(dbc, buf, buf_len, out_len, s, s_len) \
	odbc_set_string_flag(dbc, buf, buf_len, out_len, s, s_len, (wide) | (sizeof(*(out_len)) == sizeof(SQLSMALLINT)?0x20:0x30))
#else
#define odbc_set_string(dbc, buf, buf_len, out_len, s, s_len) \
	odbc_set_string_flag(dbc, buf, buf_len, out_len, s, s_len, (sizeof(*(out_len)) == sizeof(SQLSMALLINT)?0:0x10))
#define odbc_set_string_oct(dbc, buf, buf_len, out_len, s, s_len) \
	odbc_set_string_flag(dbc, buf, buf_len, out_len, s, s_len, (sizeof(*(out_len)) == sizeof(SQLSMALLINT)?0x20:0x30))
#endif

SQLSMALLINT odbc_get_concise_sql_type(SQLSMALLINT type, SQLSMALLINT interval);
SQLRETURN odbc_set_concise_sql_type(SQLSMALLINT concise_type, struct _drecord *drec, int check_only);
SQLSMALLINT odbc_get_concise_c_type(SQLSMALLINT type, SQLSMALLINT interval);
SQLRETURN odbc_set_concise_c_type(SQLSMALLINT concise_type, struct _drecord *drec, int check_only);

SQLLEN odbc_get_octet_len(int c_type, const struct _drecord *drec);
void odbc_convert_err_set(struct _sql_errors *errs, TDS_INT err);

/*
 * prepare_query.c
 */
SQLRETURN prepare_call(struct _hstmt *stmt);
SQLRETURN native_sql(struct _hdbc *dbc, char *s);
int parse_prepared_query(struct _hstmt *stmt, int compute_row);
int start_parse_prepared_query(struct _hstmt *stmt, int compute_row);
int continue_parse_prepared_query(struct _hstmt *stmt, SQLPOINTER DataPtr, SQLLEN StrLen_or_Ind);
const char *parse_const_param(const char * s, TDS_SERVER_TYPE *type);

/*
 * sql2tds.c
 */
SQLRETURN odbc_sql2tds(TDS_STMT * stmt, const struct _drecord *drec_ixd, const struct _drecord *drec_axd, TDSCOLUMN *curcol, int compute_row, const TDS_DESC* axd, unsigned int n_row);

/*
 * sqlwchar.c
 */
#if SIZEOF_SQLWCHAR != SIZEOF_WCHAR_T
size_t sqlwcslen(const SQLWCHAR * s);
const wchar_t *sqlwstr(const SQLWCHAR * s);
#else
#define sqlwcslen(s) wcslen(s)
#define sqlwstr(s) ((const wchar_t*)(s))
#endif

#if SIZEOF_SQLWCHAR == 2
# if WORDS_BIGENDIAN
#  define ODBC_WIDE_NAME "UCS-2BE"
# else
#  define ODBC_WIDE_NAME "UCS-2LE"
# endif
#elif SIZEOF_SQLWCHAR == 4
# if WORDS_BIGENDIAN
#  define ODBC_WIDE_NAME "UCS-4BE"
# else
#  define ODBC_WIDE_NAME "UCS-4LE"
# endif
#else
#error SIZEOF_SQLWCHAR not supported !!
#endif

#if defined(__GNUC__) && __GNUC__ >= 4 && !defined(__MINGW32__)
#pragma GCC visibility pop
#endif

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
