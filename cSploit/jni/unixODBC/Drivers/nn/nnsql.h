/**
    Copyright (C) 1995, 1996 by Ke Jin <kejin@visigenic.com>
	Enhanced for unixODBC (1999) by Peter Harvey <pharvey@codebydesign.com>
	
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
**/
#ifndef _NN_SQL_H
# define _NN_SQL_H

# include	"nndate.h"
# include	"nncol.h"

/* interface of nnsql layer */

/* basic statement handle object */
extern void*	nnsql_allocyystmt(void* hcndes);	/* Allocating and initiating a statement
							 * handle on a given connection. On success,
							 * a statement handle is returned or null on
							 * failure */
extern void	nnsql_dropyystmt(void* hstmt);		/* Drop(i.e free and detach from the
							 * connection) it */
extern void	nnsql_close_cursor(void* hstmt);	/* Close a result set, terminate the
							 * fetching process. */

/* open table(i.e. group) */
extern int	nnsql_opentable(void* hstmt, char* table);
							/* Set the active table(i.e. netnews group).
							 * It returns 0 on success, -1 on failure.*/

/* prepare (i.e. parsing and accessing mode check) */
extern int	nnsql_prepare(void* hstmt, char* sqlstmt, int len);
							/* Parsing the SQL statement and check the
							 * accessing mode for INSERT and DELETE
							 * statement. It returns 0 on success,
							 * -1 on failure. */

/* open table(i.e. group), type check, do insert, search delete */
extern int	nnsql_execute(void* hstmt);		/* Execute a prepared statement(i.e. commit
							 * the INSERT or DELETE, type check,
							 * set active table or open a result set.
							 * It returns 0 on success, -1 on failure.*/

/* fetch result */
extern int	nnsql_fetch(void* hstmt);		/* Fetching result from a opened result set.
							 * It returns 0 on success, -1 on failure, 100
							 * on hitting the end of the result set.
							 * The fetched result at the <icol>'th column
							 * in the current row can be gotten with
							 * nnsql_getxxx(hstmt, icol) calls. The
							 * properties(i.e. null, datatype, etc.) of
							 * the <icol>'th column in the result set can
							 * be gotten with nnsql_isxxxcol(hstmt,icol)
							 * calls. */

/* get column describor id */
extern int	nnsql_column_descid(void* hstmt, int icol);
							/* Getting column describator id(i.e en_subject,
							 * en_from, en_msgid, en_body, etc.) for the
							 * the <icol>'th column of a prepared or
							 * executed statement if a result set will or
							 * already been opened. It returns the
							 * describator id or -1 on failure. Note, the
							 * user column index <icol> starts from 1 and
							 * column index 0 is always used as a hidden
							 * internal column, the article number. */

/* max number of parameters/columns */
extern int	nnsql_max_param();			/* Return the up limit of parameter number(32)*/
extern int	nnsql_max_column();			/* Return the up limit of column number   (32)*/

/* get value from column */
extern long	nnsql_getnum(void* hstmt, int icol);	/* Return a long integer from the <icol>'th
							 * column of the current row of the result set.
							 * If the datatype of this column is not INTEGER
							 * (i.e. NUMBER), this function will return 0.
							 * Using nnsql_isnumcol() to check this before
							 * getting the long integer. */

extern char*	nnsql_getstr(void* hstmt, int icol);	/* Return an null terminated string at the
							 * <icol>'th column of current row of the result
							 * set. If datatype of this column is STRING,
							 * this function will return an null pointer.
							 * Using nnsql_isstrcol() to check this before
							 * getting a string. The string itself is placed
							 * in an internal buffer. It may be overwritten
							 * by the result of later(if not next)
							 * nnsql_fetch() call. Thus, it is your duty to
							 * keep a copy of it when necessary. */

extern date_t*	nnsql_getdate(void* hstmt, int icol);	/* Return a pointer point to an internal date
							 * datatype structure from the <icol>'th column
							 * of the current row of the result set. If the
							 * datatype of this column is not DATE, this
							 * function will return a null pointer. Using
							 * nnsql_isdatecol() to check this before
							 * getting a date pointer. The date structure
							 * is place in an internal buffer. It may be
							 * overwritten by the result of later(if not
							 * next) nnsql_fetch() call. It is your job to
							 * keep a copy of it when necessary. */

/* get total number of columns and parameters */
extern int	nnsql_getcolnum(void* hstmt);		/* Return total column number of a prepared or
							 * executed statement, if a result set has been
							 * opened or will be opened after the prepared
							 * statement been executed. It return 0 if there
							 * is no result set will or already been opened.
							 * Unlike ODBC, the column number returned from
							 * this function will be, if it is not 0, equal
							 * or large than 2 to include the hidden internal
							 * column -- the article-num column(it is always
							 * the 0'th column). */

extern int	nnsql_getparnum(void* hstmt);		/* return total parameter number(i.e. number of
							 * outstanding ? marks) of a prepared SQL
							 * statement.*/

extern int	nnsql_getrowcount(void* hstmt); 	/* For a DELETE statement, this function returns
							 * the total deleted rows(i.e. articles).
							 * For an INSERT statement, this function
							 * returns 0(i.e. insert unsuccess) or 1(i.e.
							 * successed inserted or posted).
							 * For a SELECT statement, this function returns
							 * the current cursor location(i.e. current row
							 * number) which equals to the total number of
							 * successful nnsql_fetch(). */

/* get type of column */
extern int	nnsql_isnullcol(void* hstmt, int icol); /* Return TRUE if the <icol>'th column at current
							 * row is null or return FAIL if not */
extern int	nnsql_isstrcol(void* hstmt, int icol);	/* Return TRUE if datatype of the <icol>'th column
							 * is STRING or return FAIL if not */
extern int	nnsql_isnumcol(void* hstmt, int icol);	/* Return TRUE if datatype of the <icol>'th column
							 * is INTEGER(i.e NUMBER) or return FAIL if not */
extern int	nnsql_isdatecol(void* hstmt, int icol); /* Return TRUE if datatype of the <icol>'th column
							 * is DATE or return FAIL if not */
extern int	nnsql_iscountcol(void* hstmt, int icol);/* Return TRUE if the <icol>'th column is a count
							 * column or return FAIL if not */

extern int	nnsql_isnullablecol(void* hstmt, int icol);
							/* Return TRUE if the <icol>'th column is an
							 * nullable one(i.e. not necessary specified in
							 * INSERT statement. return FAIL if not */

/* unbind dummy variable(s) */
extern int	nnsql_yyunbindpar(void* hstmt, int ipar);
							/* Unbinding the <ipar>'th parameter
							 * (corrosponding to the value previouse bound
							 * for the <ipar>'th outstanding ? mark. This
							 * will free the *user* buffer(if an user string
							 * was previously bound) from the statement
							 * handle.*/

/* put data for dummy variable(s) */
extern int	nnsql_putnum(void* hstmt, int ipar, long num);
							/* Binding an integer as the <ipar>'th parameter
							 * (i.e. its value and type will be used in
							 * place of the <ipar>'th outstanding ? mark in
							 * the prepared SQL statement. Same as ODBC, here
							 * the <ipar> index start from 1. */
extern int	nnsql_putstr(void* hstmt, int ipar, char* str);
							/* Binding a string(keep a pointer of the user
							 * string location) as the <ipar>'th parameter.*/
extern int	nnsql_putdate(void* hstmt, int ipar, date_t* date);
							/* Binding a date(copy the date struct to an
							 * internal recyclable buffer) as the <ipar>'th
							 * parameter */
extern int	nnsql_putnull(void* hstmt, int ipar);	/* Binding a null as the <ipar>'th parameter */

/* error report function */
extern int	nnsql_errcode(void* hstmt);		/* Return the current errcode or 0 */
extern char*	nnsql_errmsg(void* hstmt);		/* Return the current errmsg(null terminate
							 * string) or null */
extern int	nnsql_errpos(void* hstmt);		/* Return parsing position of error in the sql
							 * statement */

/* nntp connection access mode flag. used in nntp_accmode() */
# define	ACCESS_MODE_SELECT	0		/* Allow SELECT only */
# define	ACCESS_MODE_INSERT	1		/* Allow INSERT and SELECT */
# define	ACCESS_MODE_DELETE_TEST 2		/* Allow SELECT, INSERT, DELETE(from *.test
							 * groups) */
# define	ACCESS_MODE_DELETE_ANY	3		/* Allow SELECT, INSERT, DELETE(from all groups)*/

#endif
