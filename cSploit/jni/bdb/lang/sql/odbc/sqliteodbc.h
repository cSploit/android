#ifndef _SQLITEODBC_H
#define _SQLITEODBC_H

/**
 * @mainpage
 * @section readme README
 * @verbinclude README
 * @section changelog ChangeLog
 * @verbinclude ChangeLog
 * @section copying License Terms
 * @verbinclude license.terms
 */

/**
 * @file sqliteodbc.h
 * Header file for SQLite ODBC driver.
 *
 * $Id: sqliteodbc.h,v 1.59 2013/01/11 12:20:56 chw Exp chw $
 *
 * Copyright (c) 2001-2013 Christian Werner <chw@ch-werner.de>
 *
 * See the file "license.terms" for information on usage
 * and redistribution of this file and for a
 * DISCLAIMER OF ALL WARRANTIES.
 */

#ifdef __OS2__
#define INCL_WIN
#define INCL_PM
#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS
#define INCL_WINSTDFILE
#define ALLREADY_HAVE_OS2_TYPES
#define DONT_TD_VOID
#include <os2.h>
#include <stdlib.h>
#define ODBCVER 0x0300
#include "resourceos2.h"
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#if defined(HAVE_SQLITETRACE) && HAVE_SQLITETRACE
#include <stdio.h>
#endif
#else
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#endif
#include <stdlib.h>
#if defined(HAVE_LOCALECONV) || defined(_WIN32) || defined(_WIN64)
#include <locale.h>
#endif
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include <time.h>

#include "sqlite.h"
#ifdef HAVE_IODBC
#include <iodbcinst.h>
#endif
#if defined(HAVE_UNIXODBC) || defined(_WIN32) || defined(_WIN64)
#include <odbcinst.h>
#endif

#ifndef SQL_API
#define SQL_API
#endif

#ifndef HAVE_SQLLEN
#define SQLLEN SQLINTEGER
#endif

#define SQLLEN_PTR SQLLEN *

#ifndef HAVE_SQLULEN
#define SQLULEN SQLUINTEGER
#endif

#ifndef HAVE_SQLROWCOUNT
#define SQLROWCOUNT SQLUINTEGER
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

struct dbc;
struct stmt;

/**
 * @typedef ENV
 * @struct ENV
 * Driver internal structure for environment (HENV).
 */

typedef struct {
    int magic;			/**< Magic cookie */
    int ov3;			/**< True for SQL_OV_ODBC3 */
#if defined(_WIN32) || defined(_WIN64)
    CRITICAL_SECTION cs;	/**< For serializing most APIs */
    DWORD owner;		/**< Current owner of CS or 0 */
#endif
    struct dbc *dbcs;		/**< Pointer to first DBC */
} ENV;

/**
 * @typedef DBC
 * @struct dbc
 * Driver internal structure for database connection (HDBC).
 */

typedef struct dbc {
    int magic;			/**< Magic cookie */
    ENV *env;			/**< Pointer to environment */
    struct dbc *next;		/**< Pointer to next DBC */
    sqlite *sqlite;		/**< SQLITE database handle */
    int version;		/**< SQLITE version number */
    char *dbname;		/**< SQLITE database name */
    char *dsn;			/**< ODBC data source name */
    int timeout;		/**< Lock timeout value */
    long t0;			/**< Start time for SQLITE busy handler */
    int busyint;		/**< Interrupt busy handler from SQLCancel() */
    int *ov3;			/**< True for SQL_OV_ODBC3 */
    int ov3val;			/**< True for SQL_OV_ODBC3 */
    int autocommit;		/**< Auto commit state */
    int intrans;		/**< True when transaction started */
    struct stmt *stmt;		/**< STMT list of this DBC */
    int naterr;			/**< Native error code */
    char sqlstate[6];		/**< SQL state for SQLError() */
    SQLCHAR logmsg[1024];	/**< Message for SQLError() */
    int nowchar;		/**< Don't try to use WCHAR */
    int longnames;		/**< Don't shorten column names */
    int curtype;		/**< Default cursor type */
    int step_enable;		/**< True for sqlite_compile/step/finalize */
    int trans_disable;		/**< True for no transaction support */
    struct stmt *vm_stmt;	/**< Current STMT executing VM */
    int vm_rownum;		/**< Current row number */
#if defined(HAVE_SQLITETRACE) && HAVE_SQLITETRACE
    FILE *trace;		/**< sqlite_trace() file pointer or NULL */
#endif
#ifdef USE_DLOPEN_FOR_GPPS
    void *instlib;
    int (*gpps)();
#endif
#if defined(_WIN32) || defined(_WIN64)
    int xcelqrx;
#endif
} DBC;

/**
 * @typedef COL
 * @struct COL
 * Internal structure to describe a column in a result set.
 */

typedef struct {
    char *db;			/**< Database name */
    char *table;		/**< Table name */
    char *column;		/**< Column name */
    int type;			/**< Data type of column */
    int size;			/**< Size of column */
    int index;			/**< Index of column in result */
    int nosign;			/**< Unsigned type */
    int scale;			/**< Scale of column */
    int prec;			/**< Precision of column */
    int autoinc;		/**< AUTO_INCREMENT column */
    int notnull;		/**< NOT NULL constraint on column */
    char *typename;		/**< Column type name or NULL */
    char *label;		/**< Column label or NULL */
} COL;

/**
 * @typedef BINDCOL
 * @struct BINDCOL
 * Internal structure for bound column (SQLBindCol).
 */

typedef struct {
    SQLSMALLINT type;	/**< ODBC type */
    SQLINTEGER max;	/**< Max. size of value buffer */
    SQLLEN *lenp;	/**< Value return, actual size of value buffer */
    SQLPOINTER valp;	/**< Value buffer */
    int index;		/**< Index of column in result */
    int offs;		/**< Byte offset for SQLGetData() */
} BINDCOL;

/**
 * @typedef BINDPARM
 * @struct BINDPARM
 * Internal structure for bound parameter (SQLBindParameter).
 */

typedef struct {
    int type, stype;	/**< ODBC and SQL types */
    int coldef, scale;	/**< from SQLBindParameter() */
    SQLLEN max;		/**< Max. size size of parameter buffer */
    SQLLEN *lenp;	/**< Actual size of parameter buffer */
    SQLLEN *lenp0;	/**< Actual size of parameter buffer, initial value */
    void *param;	/**< Parameter buffer */
    void *param0;	/**< Parameter buffer, initial value */
    int inc;		/**< Increment for paramset size > 1 */
    int need;		/**< True when SQL_LEN_DATA_AT_EXEC */
    int bound;		/**< True when SQLBindParameter() called */ 
    int offs, len;	/**< Offset/length for SQLParamData()/SQLPutData() */
    void *parbuf;	/**< Buffer for SQL_LEN_DATA_AT_EXEC etc. */
    char strbuf[64];	/**< String buffer for scalar data */
} BINDPARM;

/**
 * @typedef STMT
 * @struct stmt
 * Driver internal structure representing SQL statement (HSTMT).
 */

typedef struct stmt {
    struct stmt *next;		/**< Linkage for STMT list in DBC */
    HDBC dbc;			/**< Pointer to DBC */
    SQLCHAR cursorname[32];	/**< Cursor name */
    SQLCHAR *query;		/**< Current query, raw string */
    char **parmnames;		/**< Parameter names from current query */
    int *ov3;			/**< True for SQL_OV_ODBC3 */
    int isselect;		/**< > 0 if query is a SELECT statement */
    int ncols;			/**< Number of result columns */
    COL *cols;			/**< Result column array */
    COL *dyncols;		/**< Column array, but malloc()ed */
    int dcols;			/**< Number of entries in dyncols */
    int bkmrk;			/**< True when bookmarks used */
    BINDCOL bkmrkcol;		/**< Bookmark bound column */
    BINDCOL *bindcols;		/**< Array of bound columns */
    int nbindcols;		/**< Number of entries in bindcols */
    int nbindparms;		/**< Number bound parameters */
    BINDPARM *bindparms;	/**< Array of bound parameters */
    int nparams;		/**< Number of parameters in query */
    int pdcount;		/**< SQLParamData() counter */
    int nrows;			/**< Number of result rows */
    int rowp;			/**< Current result row */
    char **rows;		/**< 2-dim array, result set */
    void (*rowfree)();		/**< Free function for rows */
    int naterr;			/**< Native error code */
    char sqlstate[6];		/**< SQL state for SQLError() */
    SQLCHAR logmsg[1024];	/**< Message for SQLError() */ 
    int nowchar[2];		/**< Don't try to use WCHAR */
    int longnames;		/**< Don't shorten column names */
    SQLULEN retr_data;		/**< SQL_ATTR_RETRIEVE_DATA */
    SQLULEN rowset_size;	/**< Size of rowset */
    SQLUSMALLINT *row_status;	/**< Row status pointer */
    SQLUSMALLINT *row_status0;	/**< Internal status array */
    SQLUSMALLINT row_status1;	/**< Internal status array for 1 row rowsets */
    SQLULEN *row_count;		/**< Row count pointer */
    SQLULEN row_count0;		/**< Row count */
    SQLULEN paramset_size;	/**< SQL_ATTR_PARAMSET_SIZE */
    SQLULEN paramset_count;	/**< Internal for paramset */
    SQLUINTEGER paramset_nrows;	/**< Row count for paramset handling */
    SQLULEN bind_type;		/**< SQL_ATTR_ROW_BIND_TYPE */
    SQLULEN *bind_offs;		/**< SQL_ATTR_ROW_BIND_OFFSET_PTR */
    /* Dummies to make ADO happy */
    SQLULEN *parm_bind_offs;	/**< SQL_ATTR_PARAM_BIND_OFFSET_PTR */
    SQLUSMALLINT *parm_oper;	/**< SQL_ATTR_PARAM_OPERATION_PTR */
    SQLUSMALLINT *parm_status;	/**< SQL_ATTR_PARAMS_STATUS_PTR */
    SQLULEN *parm_proc;		/**< SQL_ATTR_PARAMS_PROCESSED_PTR */
    SQLULEN parm_bind_type;	/**< SQL_ATTR_PARAM_BIND_TYPE */
    int curtype;		/**< Cursor type */
    sqlite_vm *vm;		/**< SQLite VM or NULL */
#if HAVE_ENCDEC
    char *bincell;		/**< Undecoded string */
    char *bincache;		/**< Cached decoded binary data */
    int binlen;			/**< Length of decoded binary data */
    char *hexcache;		/**< Cached decoded binary in hex */
#endif
} STMT;

#endif
