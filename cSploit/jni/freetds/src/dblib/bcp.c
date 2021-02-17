/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2010, 2011  Frediano Ziglio
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
#include <assert.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef _WIN32
#include <io.h>
#endif

#include <freetds/tds.h>
#include <freetds/iconv.h>
#include <freetds/convert.h>
#include <replacements.h>
#include <sybfront.h>
#include <sybdb.h>
#include <syberror.h>
#include <dblib.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define HOST_COL_CONV_ERROR 1
#define HOST_COL_NULL_ERROR 2

#ifndef MAX
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#endif

TDS_RCSID(var, "$Id: bcp.c,v 1.220 2012-03-09 19:20:30 freddy77 Exp $");

#ifdef HAVE_FSEEKO
typedef off_t offset_type;
#elif defined(_WIN32) || defined(_WIN64)
/* win32 version */
typedef __int64 offset_type;
# if defined(HAVE__FSEEKI64) && defined(HAVE__FTELLI64)
#  define fseeko(f,o,w) _fseeki64((f),o,w)
#  define ftello(f) _ftelli64((f))
# else
#  define fseeko(f,o,w) (_lseeki64(fileno(f),o,w) == -1 ? -1 : 0)
#  define ftello(f) _telli64(fileno(f))
# endif
#else
/* use old version */
#define fseeko(f,o,w) fseek(f,o,w)
#define ftello(f) ftell(f)
typedef long offset_type;
#endif

static void _bcp_free_storage(DBPROCESS * dbproc);
static void _bcp_free_columns(DBPROCESS * dbproc);
static TDSRET _bcp_get_col_data(TDSBCPINFO *bcpinfo, TDSCOLUMN *bindcol, int offset);
static TDSRET _bcp_no_get_col_data(TDSBCPINFO *bcpinfo, TDSCOLUMN *bindcol, int offset);

static int rtrim(char *, int);
static STATUS _bcp_read_hostfile(DBPROCESS * dbproc, FILE * hostfile, int *row_error);
static int _bcp_readfmt_colinfo(DBPROCESS * dbproc, char *buf, BCP_HOSTCOLINFO * ci);
static int _bcp_get_term_var(BYTE * pdata, BYTE * term, int term_len);

/*
 * "If a host file is being used ... the default data formats are as follows:
 *
 *  > The order, type, length and number of the columns in the host file are 
 *    assumed to be identical to the order, type and number of the columns in the database table.
 *  > If a given database column's data is fixed-length, 
 *    then the host file's  data column will also be fixed-length. 
 *  > If a given database column's data is variable-length or may contain null values, 
 *    the host file's data column will be prefixed by 
 *	a 4-byte length value for SYBTEXT and SYBIMAGE data types, and 
 *	a 1-byte length value for all other types.
 *  > There are no terminators of any kind between host file columns."
 */

static void
init_hostfile_columns(DBPROCESS *dbproc)
{
	const int ncols = dbproc->bcpinfo->bindinfo->num_cols;
	RETCODE erc;
	int icol;
	
	if (ncols == 0)
		return;
		
	if ((erc = bcp_columns(dbproc, ncols)) != SUCCEED) {
		assert(erc == SUCCEED);
		return;
	}
		
	for (icol = 0; icol < ncols; icol++) {
		const TDSCOLUMN *pcol = dbproc->bcpinfo->bindinfo->columns[icol];
		int prefixlen = 0, termlen = 0;
		
		switch (pcol->column_type) {
		case SYBTEXT:
		case SYBIMAGE:
			prefixlen = 4;
			break;
		default:
			prefixlen = dbvarylen(dbproc, icol+1)? 1 : 0;
		}
		
		erc = bcp_colfmt(dbproc, icol+1, pcol->column_type, prefixlen, pcol->column_size, NULL, termlen, icol+1);
		
		assert(erc == SUCCEED);
		if (erc != SUCCEED)
			return;
	}
}


/** 
 * \ingroup dblib_bcp
 * \brief Prepare for bulk copy operation on a table
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param tblname the name of the table receiving or providing the data.
 * \param hfile the data file opposite the table, if any.  
 * \param errfile the "error file" captures messages and, if errors are encountered, 
 * 	copies of any rows that could not be written to the table.
 * \param direction one of 
 *		- \b DB_IN writing to the table
 *		- \b DB_OUT writing to the host file
 * 		.
 * \remarks bcp_init() sets the host file data format and acquires the table metadata.
 *	It is called before the other bulk copy functions. 
 *
 * 	When writing to a table, bcp can use as its data source a data file (\a hfile), 
 * 	or program data in an application's variables.  In the latter case, call bcp_bind() 
 *	to associate your data with the appropriate table column.  
 * \return SUCCEED or FAIL.
 * \sa 	BCP_SETL(), bcp_bind(), bcp_done(), bcp_exec()
 */
RETCODE
bcp_init(DBPROCESS * dbproc, const char *tblname, const char *hfile, const char *errfile, int direction)
{
	tdsdump_log(TDS_DBG_FUNC, "bcp_init(%p, %s, %s, %s, %d)\n", 
			dbproc, tblname? tblname:"NULL", hfile? hfile:"NULL", errfile? errfile:"NULL", direction);
	CHECK_CONN(FAIL);
	CHECK_NULP(tblname, "bcp_init", 2, FAIL);

	/* Free previously allocated storage in dbproc & initialise flags, etc. */
	
	_bcp_free_storage(dbproc);

	/* 
	 * Validate other parameters 
	 */
	if (dbproc->tds_socket->conn->tds_version < 0x500) {
		dbperror(dbproc, SYBETDSVER, 0);
		return FAIL;
	}

	if (tblname == NULL) {
		dbperror(dbproc, SYBEBCITBNM, 0);
		return FAIL;
	}

	if (strlen(tblname) > 92 && !IS_TDS7_PLUS(dbproc->tds_socket->conn)) {	/* 30.30.30 */
		dbperror(dbproc, SYBEBCITBLEN, 0);
		return FAIL;
	}

	if (direction != DB_IN && direction != DB_OUT && direction != DB_QUERYOUT) {
		dbperror(dbproc, SYBEBDIO, 0);
		return FAIL;
	}

	/* Allocate storage */
	
	dbproc->bcpinfo = (TDSBCPINFO*) calloc(1, sizeof(TDSBCPINFO));
	if (dbproc->bcpinfo == NULL)
		goto memory_error;

	if ((dbproc->bcpinfo->tablename = strdup(tblname)) == NULL)
		goto memory_error;

	dbproc->bcpinfo->direction = direction;

	dbproc->bcpinfo->xfer_init  = 0;
	dbproc->bcpinfo->var_cols   = 0;
	dbproc->bcpinfo->bind_count = 0;

	if (TDS_FAILED(tds_bcp_init(dbproc->tds_socket, dbproc->bcpinfo))) {
		/* TODO return proper error */
		/* Attempt to use Bulk Copy with a non-existent Server table (might be why ...) */
		dbperror(dbproc, SYBEBCNT, 0);
		return FAIL;
	}

	/* Prepare default hostfile columns */
	
	if (hfile == NULL) {
		dbproc->hostfileinfo = NULL;
		return SUCCEED;
	}

	dbproc->hostfileinfo = (BCP_HOSTFILEINFO*) calloc(1, sizeof(BCP_HOSTFILEINFO));

	if (dbproc->hostfileinfo == NULL)
		goto memory_error;
	if ((dbproc->hostfileinfo->hostfile = strdup(hfile)) == NULL)
		goto memory_error;

	if (errfile != NULL)
		if ((dbproc->hostfileinfo->errorfile = strdup(errfile)) == NULL)
			goto memory_error;

	init_hostfile_columns(dbproc);

	return SUCCEED;

memory_error:
	_bcp_free_storage(dbproc);
	dbperror(dbproc, SYBEMEM, ENOMEM);
	return FAIL;
}


/** 
 * \ingroup dblib_bcp
 * \brief Set the length of a host variable to be written to a table.
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param varlen size of the variable, in bytes, or
 * 	- \b 0 indicating NULL
 *	- \b -1 indicating size is determined by the prefix or terminator.  
 *		(If both a prefix and a terminator are present, bcp is supposed to use the smaller of the 
 *		 two.  This feature might or might not actually work.)
 * \param table_column the number of the column in the table (starting with 1, not zero).
 * 
 * \return SUCCEED or FAIL.
 * \sa 	bcp_bind(), bcp_colptr(), bcp_sendrow() 
 */
RETCODE
bcp_collen(DBPROCESS * dbproc, DBINT varlen, int table_column)
{
	TDSCOLUMN *bcpcol;

	tdsdump_log(TDS_DBG_FUNC, "bcp_collen(%p, %d, %d)\n", dbproc, varlen, table_column);
	
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);		/* not initialized */
	DBPERROR_RETURN(dbproc->bcpinfo->direction != DB_IN, SYBEBCPN)	/* not inbound */
	DBPERROR_RETURN(dbproc->hostfileinfo != NULL, SYBEBCPI)		/* cannot have data file */
	CHECK_PARAMETER(0 < table_column && 
			    table_column <= dbproc->bcpinfo->bindinfo->num_cols, SYBECNOR, FAIL);
	
	bcpcol = dbproc->bcpinfo->bindinfo->columns[table_column - 1];

#if USING_SYBEBCNN
	DBPERROR_RETURN(varlen == 0  && !bcpcol->column_nullable, SYBEBCNN); /* non-nullable column cannot receive a NULL */
#endif
	bcpcol->column_bindlen = varlen;

	return SUCCEED;
}

/** 
 * \ingroup dblib_bcp
 * \brief Indicate how many columns are to be found in the datafile.
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param host_colcount count of columns in the datafile, irrespective of how many you intend to use. 
 * \remarks This function describes the file as it is, not how it will be used.  
 * 
 * \return SUCCEED or FAIL.  It's hard to see how it could fail.  
 * \sa 	bcp_colfmt() 	
 */
RETCODE
bcp_columns(DBPROCESS * dbproc, int host_colcount)
{
	int i;

	tdsdump_log(TDS_DBG_FUNC, "bcp_columns(%p, %d)\n", dbproc, host_colcount);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);
	CHECK_PARAMETER(dbproc->hostfileinfo, SYBEBIVI, FAIL);

	if (host_colcount < 1) {
		dbperror(dbproc, SYBEBCFO, 0);
		return FAIL;
	}

	_bcp_free_columns(dbproc);

	dbproc->hostfileinfo->host_columns = (BCP_HOSTCOLINFO**) calloc(host_colcount, sizeof(BCP_HOSTCOLINFO *));
	if (dbproc->hostfileinfo->host_columns == NULL) {
		dbperror(dbproc, SYBEMEM, ENOMEM);
		return FAIL;
	}

	dbproc->hostfileinfo->host_colcount = host_colcount;

	for (i = 0; i < host_colcount; i++) {
		dbproc->hostfileinfo->host_columns[i] = (BCP_HOSTCOLINFO*) calloc(1, sizeof(BCP_HOSTCOLINFO));
		if (dbproc->hostfileinfo->host_columns[i] == NULL) {
			dbproc->hostfileinfo->host_colcount = i;
			_bcp_free_columns(dbproc);
			dbperror(dbproc, SYBEMEM, ENOMEM);
			return FAIL;
		}
	}

	return SUCCEED;
}

/** 
 * \ingroup dblib_bcp
 * \brief Specify the format of a datafile prior to writing to a table.
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param host_colnum datafile column number (starting with 1, not zero).
 * \param host_type dataype token describing the data type in \a host_colnum.  E.g. SYBCHAR for character data.
 * \param host_prefixlen size of the prefix in the datafile column, if any.  For delimited files: zero.  
 *			May be 0, 1, 2, or 4 bytes.  The prefix will be read as an integer (not a character string) from the 
 * 			data file, and will be interpreted the data size of that column, in bytes.  
 * \param host_collen maximum size of datafile column, exclusive of any prefix/terminator.  Just the data, ma'am.  
 *		Special values:
 *			- \b 0 indicates NULL.  
 *			- \b -1 for fixed-length non-null datatypes
 *			- \b -1 for variable-length datatypes (e.g. SYBCHAR) where the length is established 
 *				by a prefix/terminator.  
 * \param host_term the sequence of characters that will serve as a column terminator (delimiter) in the datafile.  
 * 			Often a tab character, but can be any string of any length.  Zero indicates no terminator.  
 * 			Special characters:
 *				- \b '\\0' terminator is an ASCII NUL.
 *				- \b '\\t' terminator is an ASCII TAB.
 *				- \b '\\n' terminator is an ASCII NL.
 * \param host_termlen the length of \a host_term, in bytes. 
 * \param table_colnum Nth column, starting at 1, in the table that maps to \a host_colnum.  
 * 	If there is a column in the datafile that does not map to a table column, set \a table_colnum to zero.  
 *
 *\remarks  bcp_colfmt() is called once for each column in the datafile, as specified with bcp_columns().  
 * In so doing, you describe to FreeTDS how to parse each line of your datafile, and where to send each field.  
 *
 * When a prefix or terminator is used with variable-length data, \a host_collen may have one of three values:
 *		- \b positive indicating the maximum number of bytes to be used
 * 		- \b 0 indicating NULL
 *		- \b -1 indicating no maximum; all data, as described by the prefix/terminator will be used.  
 *		.
 * \return SUCCEED or FAIL.
 * \sa 	bcp_batch(), bcp_bind(), bcp_colfmt_ps(), bcp_collen(), bcp_colptr(), bcp_columns(),
 *	bcp_control(), bcp_done(), bcp_exec(), bcp_init(), bcp_sendrow
 */
RETCODE
bcp_colfmt(DBPROCESS * dbproc, int host_colnum, int host_type, int host_prefixlen, DBINT host_collen, const BYTE * host_term,
	   int host_termlen, int table_colnum)
{
	BCP_HOSTCOLINFO *hostcol;
	BYTE *terminator = NULL;

	tdsdump_log(TDS_DBG_FUNC, "bcp_colfmt(%p, %d, %d, %d, %d, %p, %d, %d)\n", 
		    dbproc, host_colnum, host_type, host_prefixlen, (int) host_collen, host_term, host_termlen, table_colnum);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);
	CHECK_PARAMETER(dbproc->hostfileinfo, SYBEBIVI, FAIL);

	/* Microsoft specifies a "file_termlen" of zero if there's no terminator */
	if (dbproc->msdblib && host_termlen == 0)
		host_termlen = -1;

	if (host_termlen < 0)
		host_termlen = -1;

	if (dbproc->hostfileinfo->host_colcount == 0) {
		dbperror(dbproc, SYBEBCBC, 0);
		return FAIL;
	}

	if (host_colnum < 1) {
		dbperror(dbproc, SYBEBCFO, 0);
		return FAIL;
	}

	if (host_colnum > dbproc->hostfileinfo->host_colcount) {
		dbperror(dbproc, SYBECNOR, 0);
		return FAIL;
	}

	if (host_prefixlen != 0 && host_prefixlen != 1 && host_prefixlen != 2 && host_prefixlen != 4 && host_prefixlen != -1) {
		dbperror(dbproc, SYBEBCPREF, 0);
		return FAIL;
	}

	if (table_colnum <= 0 && host_type == 0) {
		dbperror(dbproc, SYBEBCPCTYP, 0);
		return FAIL;
	}

	if (host_prefixlen == 0 && host_collen == -1 && host_termlen == -1 && !is_fixed_type(host_type)) {
		dbperror(dbproc, SYBEVDPT, 0);
		return FAIL;
	}

	if (host_collen < -1) {
		dbperror(dbproc, SYBEBCHLEN, 0);
		return FAIL;
	}

	/* No official error message.  Fix and warn. */
	if (is_fixed_type(host_type) && (host_collen != -1 && host_collen != 0)) {
		tdsdump_log(TDS_DBG_FUNC,
			    "bcp_colfmt: changing host_collen to -1 from %d for fixed type %d.\n", 
			    host_collen, host_type);
		host_collen = -1;
	}

	/* 
	 * If there's a positive terminator length, we need a valid terminator pointer.
	 * If the terminator length is 0 or -1, then there's no terminator.
	 */
	if (host_term == NULL && host_termlen > 0) {
		dbperror(dbproc, SYBEVDPT, 0);	/* "all variable-length data must have either a length-prefix ..." */
		return FAIL;
	}

	hostcol = dbproc->hostfileinfo->host_columns[host_colnum - 1];

	/* TODO add precision scale and join with bcp_colfmt_ps */
	if (host_term && host_termlen > 0) {
		if ((terminator = (BYTE*) malloc(host_termlen)) == NULL) {
			dbperror(dbproc, SYBEMEM, errno);
			return FAIL;
		}
		memcpy(terminator, host_term, host_termlen);
	}
	hostcol->host_column = host_colnum;
	hostcol->datatype = host_type;
	hostcol->prefix_len = host_prefixlen;
	hostcol->column_len = host_collen;
	free(hostcol->terminator);
	hostcol->terminator = terminator;
	hostcol->term_len = host_termlen;
	hostcol->tab_colnum = table_colnum;

	return SUCCEED;
}

/** 
 * \ingroup dblib_bcp
 * \brief Specify the format of a host file for bulk copy purposes, 
 * 	with precision and scale support for numeric and decimal columns.
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param host_colnum datafile column number (starting with 1, not zero).
 * \param host_type dataype token describing the data type in \a host_colnum.  E.g. SYBCHAR for character data.
 * \param host_prefixlen size of the prefix in the datafile column, if any.  For delimited files: zero.  
 *			May be 0, 1, 2, or 4 bytes.  The prefix will be read as an integer (not a character string) from the 
 * 			data file, and will be interpreted the data size of that column, in bytes.  
 * \param host_collen maximum size of datafile column, exclusive of any prefix/terminator.  Just the data, ma'am.  
 *		Special values:
 *			- \b 0 indicates NULL.  
 *			- \b -1 for fixed-length non-null datatypes
 *			- \b -1 for variable-length datatypes (e.g. SYBCHAR) where the length is established 
 *				by a prefix/terminator.  
 * \param host_term the sequence of characters that will serve as a column terminator (delimiter) in the datafile.  
 * 			Often a tab character, but can be any string of any length.  Zero indicates no terminator.  
 * 			Special characters:
 *				- \b '\\0' terminator is an ASCII NUL.
 *				- \b '\\t' terminator is an ASCII TAB.
 *				- \b '\\n' terminator is an ASCII NL.
 * \param host_termlen the length of \a host_term, in bytes. 
 * \param table_colnum Nth column, starting at 1, in the table that maps to \a host_colnum.  
 * 	If there is a column in the datafile that does not map to a table column, set \a table_colnum to zero.  
 * \param typeinfo something
 * \todo Not implemented.
 * \return SUCCEED or FAIL.
 * \sa 	bcp_batch(), bcp_bind(), bcp_colfmt(), bcp_collen(), bcp_colptr(), bcp_columns(),
 *	bcp_control(), bcp_done(), bcp_exec(), bcp_init(), bcp_sendrow
 */
RETCODE
bcp_colfmt_ps(DBPROCESS * dbproc, int host_colnum, int host_type,
	      int host_prefixlen, DBINT host_collen, BYTE * host_term, int host_termlen, int table_colnum, DBTYPEINFO * typeinfo)
{
	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED: bcp_colfmt_ps(%p, %d, %d)\n", dbproc, host_colnum, host_type);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);
	
	/* dbperror(dbproc, , 0);	 Illegal precision specified */

	/* TODO see bcp_colfmt */
	return FAIL;
}


/** 
 * \ingroup dblib_bcp
 * \brief Set BCP options for uploading a datafile
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param field symbolic constant indicating the option to be set, one of:
 *  		- \b BCPMAXERRS Maximum errors tolerated before quitting. The default is 10.
 *  		- \b BCPFIRST The first row to read in the datafile. The default is 1. 
 *  		- \b BCPLAST The last row to read in the datafile. The default is to copy all rows. A value of
 *                  	-1 resets this field to its default?
 *  		- \b BCPBATCH The number of rows per batch.  Default is 0, meaning a single batch. 
 * \param value The value for \a field.
 *
 * \remarks These options control the behavior of bcp_exec().  
 * When writing to a table from application host memory variables, 
 * program logic controls error tolerance and batch size. 
 * 
 * \return SUCCEED or FAIL.
 * \sa 	bcp_batch(), bcp_bind(), bcp_colfmt(), bcp_collen(), bcp_colptr(), bcp_columns(), bcp_done(), bcp_exec(), bcp_options()
 */
RETCODE
bcp_control(DBPROCESS * dbproc, int field, DBINT value)
{
	tdsdump_log(TDS_DBG_FUNC, "bcp_control(%p, %d, %d)\n", dbproc, field, value);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);

	if (field == BCPKEEPIDENTITY) {
		dbproc->bcpinfo->identity_insert_on = (value != 0);
		return SUCCEED;
	}

	CHECK_PARAMETER(dbproc->hostfileinfo, SYBEBIVI, FAIL);

	switch (field) {

	case BCPMAXERRS:
		dbproc->hostfileinfo->maxerrs = value;
		break;
	case BCPFIRST:
		dbproc->hostfileinfo->firstrow = value;
		break;
	case BCPLAST:
		dbproc->hostfileinfo->lastrow = value;
		break;
	case BCPBATCH:
		dbproc->hostfileinfo->batch = value;
		break;

	default:
		dbperror(dbproc, SYBEIFNB, 0);
		return FAIL;
	}
	return SUCCEED;
}

/*
 * \ingroup dblib_bcp
 * \brief Get BCP batch option
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \remarks This function is specific to FreeTDS.  
 * 
 * \return the value that was set by bcp_control.
 * \sa 	bcp_batch(), bcp_control()
 */
int
bcp_getbatchsize(DBPROCESS * dbproc)
{
	return dbproc->hostfileinfo->batch;
}

/** 
 * \ingroup dblib_bcp
 * \brief Set "hints" for uploading a file.  A FreeTDS-only function.  
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param option symbolic constant indicating the option to be set, one of:
 * 		- \b BCPLABELED Not implemented.
 * 		- \b BCPHINTS The hint to be passed when the bulk-copy begins.  
 * \param value The string constant for \a option a/k/a the hint.  One of:
 * 		- \b ORDER The data are ordered in accordance with the table's clustered index.
 * 		- \b ROWS_PER_BATCH The batch size
 * 		- \b KILOBYTES_PER_BATCH The approximate number of kilobytes to use for a batch size
 * 		- \b TABLOCK Lock the table
 * 		- \b CHECK_CONSTRAINTS Apply constraints
 * 		- \b FIRE_TRIGGERS Fire any INSERT triggers on the target table
 * \param valuelen The strlen of \a value.  
 * 
 * \return SUCCEED or FAIL.
 * \sa 	bcp_control(), 
 * 	bcp_exec(), 
 * \todo Simplify.  Remove \a valuelen, and dbproc->bcpinfo->hint = strdup(hints[i])
 */
RETCODE
bcp_options(DBPROCESS * dbproc, int option, BYTE * value, int valuelen)
{
	int i;
	static const char *const hints[] = {
		"ORDER", "ROWS_PER_BATCH", "KILOBYTES_PER_BATCH", "TABLOCK", "CHECK_CONSTRAINTS", "FIRE_TRIGGERS", NULL
	};

	tdsdump_log(TDS_DBG_FUNC, "bcp_options(%p, %d, %p, %d)\n", dbproc, option, value, valuelen);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);
	CHECK_NULP(value, "bcp_options", 3, FAIL);

	switch (option) {
	case BCPLABELED:
		tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED bcp option: BCPLABELED\n");
		break;
	case BCPHINTS:
		if (!value || valuelen <= 0)
			break;

		for (i = 0; hints[i]; i++) {	/* look up hint */
			if (strncasecmp((char *) value, hints[i], strlen(hints[i])) == 0) {
				dbproc->bcpinfo->hint = hints[i];	/* safe: hints[i] is static constant, above */
				return SUCCEED;
			}
		}
		tdsdump_log(TDS_DBG_FUNC, "failed, no such hint\n");
		break;
	default:
		tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED bcp option: %u\n", option);
		break;
	}
	return FAIL;
}

/** 
 * \ingroup dblib_bcp
 * \brief Override bcp_bind() by pointing to a different host variable.
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param colptr The pointer, the address of your variable. 
 * \param table_column The 1-based column ordinal in the table.  
 * \remarks Use between calls to bcp_sendrow().  After calling bcp_colptr(), 
 * 		subsequent calls to bcp_sendrow() will bind to the new address.  
 * \return SUCCEED or FAIL.
 * \sa 	bcp_bind(), bcp_collen(), bcp_sendrow() 
 */
RETCODE
bcp_colptr(DBPROCESS * dbproc, BYTE * colptr, int table_column)
{
	TDSCOLUMN *curcol;

	tdsdump_log(TDS_DBG_FUNC, "bcp_colptr(%p, %p, %d)\n", dbproc, colptr, table_column);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo->bindinfo, SYBEBCPI, FAIL);
	/* colptr can be NULL */

	if (dbproc->bcpinfo->direction != DB_IN) {
		dbperror(dbproc, SYBEBCPN, 0);
		return FAIL;
	}
	if (table_column <= 0 || table_column > dbproc->bcpinfo->bindinfo->num_cols) {
		dbperror(dbproc, SYBEBCPN, 0);
		return FAIL;
	}
	
	curcol = dbproc->bcpinfo->bindinfo->columns[table_column - 1];
	curcol->column_varaddr = (TDS_CHAR *)colptr;

	return SUCCEED;
}


/** 
 * \ingroup dblib_bcp
 * \brief See if BCP_SETL() was used to set the LOGINREC for BCP work.  
 * 
 * \param login Address of the LOGINREC variable to be passed to dbopen(). 
 * 
 * \return TRUE or FALSE.
 * \sa 	BCP_SETL(), bcp_init(), dblogin(), dbopen()
 */
DBBOOL
bcp_getl(LOGINREC * login)
{
	TDSLOGIN *tdsl = login->tds_login;

	tdsdump_log(TDS_DBG_FUNC, "bcp_getl(%p)\n", login);

	return (tdsl->bulk_copy);
}

/**
 * \ingroup dblib_bcp_internal
 * \brief
 *
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param rows_copied
 *
 * \return SUCCEED or FAIL.
 * \sa 	BCP_SETL(), bcp_batch(), bcp_bind(), bcp_colfmt(), bcp_colfmt_ps(), bcp_collen(), bcp_colptr(), bcp_columns(), bcp_control(), bcp_done(), bcp_exec(), bcp_getl(), bcp_init(), bcp_moretext(), bcp_options(), bcp_readfmt(), bcp_sendrow()
 */
static RETCODE
_bcp_exec_out(DBPROCESS * dbproc, DBINT * rows_copied)
{
	FILE *hostfile;
	int i;

	TDSSOCKET *tds;
	TDSRESULTINFO *resinfo;
	TDSCOLUMN *curcol = NULL;
	BCP_HOSTCOLINFO *hostcol;
	BYTE *src;
	int srctype;
	int srclen;
	int buflen;
	int destlen;
	int plen;

	TDS_INT result_type;

	TDS_TINYINT ti;
	TDS_SMALLINT si;
	TDS_INT li;

	TDSDATEREC when;

	int row_of_query;
	int rows_written;
	const char *bcpdatefmt;
	TDSRET tdsret;

	tdsdump_log(TDS_DBG_FUNC, "_bcp_exec_out(%p, %p)\n", dbproc, rows_copied);
	assert(dbproc);
	assert(rows_copied);

	tds = dbproc->tds_socket;
	assert(tds);

	bcpdatefmt = getenv("FREEBCP_DATEFMT");
	if (!bcpdatefmt)
		bcpdatefmt = "%Y-%m-%d %H:%M:%S.%z";

	if (dbproc->bcpinfo->direction == DB_QUERYOUT ) {
		if (TDS_FAILED(tds_submit_query(tds, dbproc->bcpinfo->tablename)))
			return FAIL;
	} else {
		/* TODO quote if needed */
		if (TDS_FAILED(tds_submit_queryf(tds, "select * from %s", dbproc->bcpinfo->tablename)))
			return FAIL;
	}

	tdsret = tds_process_tokens(tds, &result_type, NULL, TDS_TOKEN_RESULTS);
	if (TDS_FAILED(tdsret))
		return FAIL;

	if (!tds->res_info) {
		/* TODO flush/cancel to keep consistent state */
		return FAIL;
	}

	resinfo = tds->res_info;

	row_of_query = 0;
	rows_written = 0;

	/*
	 * Before we start retrieving the data, go through the defined
	 * host file columns. If the host file column is related to a
	 * table column, then allocate some space sufficient to hold
	 * the resulting data (converted to whatever host file format)
	 */

	for (i = 0; i < dbproc->hostfileinfo->host_colcount; i++) {

		hostcol = dbproc->hostfileinfo->host_columns[i];
		if (hostcol->tab_colnum < 1 || hostcol->tab_colnum > resinfo->num_cols)
			continue;

		curcol = resinfo->columns[hostcol->tab_colnum - 1];

		if (hostcol->datatype == 0)
			hostcol->datatype = curcol->column_type;

		/* work out how much space to allocate for output data */

		/* TODO use a function for fixed types */
		switch (hostcol->datatype) {

		case SYBINT1:
			buflen = destlen = 1;
			break;
		case SYBINT2:
			buflen = destlen = 2;
			break;
		case SYBINT4:
			buflen = destlen = 4;
			break;
		case SYBINT8:
			buflen = destlen = 8;
			break;
		case SYBREAL:
			buflen = destlen = 4;
			break;
		case SYBFLT8:
			buflen = destlen = 8;
			break;
		case SYBDATETIME:
			buflen = destlen = 8;
			break;
		case SYBDATETIME4:
			buflen = destlen = 4;
			break;
		case SYBBIT:
			buflen = destlen = 1;
			break;
		case SYBBITN:
			buflen = destlen = 1;
			break;
		case SYBMONEY:
			buflen = destlen = 8;
			break;
		case SYBMONEY4:
			buflen = destlen = 4;
			break;
		case SYBCHAR:
		case SYBVARCHAR:
			switch (curcol->column_type) {
			case SYBVARCHAR:
				buflen = curcol->column_size + 1;
				destlen = -1;
				break;
			case SYBCHAR:
			case SYBTEXT:
				/* FIXME column_size ?? if 2gb ?? */
				buflen = curcol->column_size + 1;
				destlen = -2;
				break;
			case SYBINT1:
				buflen = 4 + 1;	/*  255         */
				destlen = -1;
				break;
			case SYBINT2:
				buflen = 6 + 1;	/* -32768       */
				destlen = -1;
				break;
			case SYBINT4:
				buflen = 11 + 1;	/* -2147483648  */
				destlen = -1;
				break;
			case SYBINT8:
				buflen = 20 + 1;	/* -9223372036854775808  */
				destlen = -1;
				break;
			case SYBNUMERIC:
			case SYBDECIMAL:
				buflen = 40 + 1;	/* 10 to the 38 */
				destlen = -1;
				break;
			case SYBFLT8:
				buflen = 40 + 1;	/* 10 to the 38 */
				destlen = -1;
				break;
			case SYBDATETIME:
			case SYBDATETIME4:
				buflen = 255 + 1;
				destlen = -1;
				break;
			default:
				buflen = 255 + 1;
				destlen = -1;
				break;
			}
			break;
		default:
			buflen = destlen = 255;
		}

		hostcol->bcp_column_data = tds_alloc_bcp_column_data(buflen);
		hostcol->bcp_column_data->datalen = destlen;
	}

	/*
	 * TODO above we allocate many buffer just to convert and store
	 * to file.. avoid all that passages...
	 */

	if (!(hostfile = fopen(dbproc->hostfileinfo->hostfile, "w"))) {
		dbperror(dbproc, SYBEBCUO, errno);
		return FAIL;
	}

	/* fetch a row of data from the server */

	while (tds_process_tokens(tds, &result_type, NULL, TDS_STOPAT_ROWFMT|TDS_RETURN_DONE|TDS_RETURN_ROW|TDS_RETURN_COMPUTE)
		== TDS_SUCCESS) {

		if (result_type != TDS_ROW_RESULT && result_type != TDS_COMPUTE_RESULT)
			break;

		row_of_query++;

		/* skip rows outside of the firstrow/lastrow range, if specified */
		if (dbproc->hostfileinfo->firstrow > row_of_query ||
						      row_of_query > MAX(dbproc->hostfileinfo->lastrow, 0x7FFFFFFF))
			continue;

		/* Go through the hostfile columns, finding those that relate to database columns. */
		for (i = 0; i < dbproc->hostfileinfo->host_colcount; i++) {
			size_t written = 0;
			hostcol = dbproc->hostfileinfo->host_columns[i];
			if (hostcol->tab_colnum < 1 || hostcol->tab_colnum > resinfo->num_cols)
				continue;

			curcol = resinfo->columns[hostcol->tab_colnum - 1];

			src = curcol->column_data;

			if (is_blob_col(curcol))
				src = (BYTE *) ((TDSBLOB *) src)->textvalue;

			srctype = tds_get_conversion_type(curcol->column_type, curcol->column_size);

			if (curcol->column_cur_size < 0) {
				srclen = 0;
				hostcol->bcp_column_data->is_null = 1;
			} else {
				if (is_numeric_type(curcol->column_type))
					srclen = sizeof(TDS_NUMERIC);
				else
					srclen = curcol->column_cur_size;
				hostcol->bcp_column_data->is_null = 0;
			}

			if (hostcol->bcp_column_data->is_null) {
				buflen = 0;
			} else {

				/*
				 * if we are converting datetime to string, need to override any
				 * date time formats already established
				 */
				if ((srctype == SYBDATETIME || srctype == SYBDATETIME4)
				    && (hostcol->datatype == SYBCHAR || hostcol->datatype == SYBVARCHAR)) {
					tds_datecrack(srctype, src, &when);
					buflen = (int)tds_strftime((TDS_CHAR *)hostcol->bcp_column_data->data, 256,
								 bcpdatefmt, &when, 3);
				} else {
					/*
					 * For null columns, the above work to determine the output buffer size is moot,
					 * because bcpcol->data_size is zero, so dbconvert() won't write anything,
					 * and returns zero.
					 */
					/* TODO check for text !!! */
					buflen =  dbconvert(dbproc, srctype, src, srclen, hostcol->datatype,
							    hostcol->bcp_column_data->data,
							    hostcol->bcp_column_data->datalen);
					/*
					 * Special case:  When outputting database varchar data
					 * (either varchar or nullable char) dbconvert may have
					 * trimmed trailing blanks such that nothing is left.
					 * In this case we need to put a single blank to the output file.
					 */
					if (( curcol->column_type == SYBVARCHAR ||
						 (curcol->column_type == SYBCHAR && curcol->column_nullable)
					    ) && srclen > 0 && buflen == 0) {
						strcpy ((char *)hostcol->bcp_column_data->data, " ");
						buflen = 1;
					}
				}
			}

			/* The prefix */
			if ((plen = hostcol->prefix_len) == -1) {
				if (is_blob_type(hostcol->datatype))
					plen = 4;
				else if (!(is_fixed_type(hostcol->datatype)))
					plen = 2;
				else if (curcol->column_nullable)
					plen = 1;
				else
					plen = 0;
				/* cache */
				hostcol->prefix_len = plen;
			}
			switch (plen) {
			case 0:
				break;
			case 1:
				ti = buflen;
				written = fwrite(&ti, sizeof(ti), 1, hostfile);
				break;
			case 2:
				si = buflen;
				written = fwrite(&si, sizeof(si), 1, hostfile);
				break;
			case 4:
				li = buflen;
				written = fwrite(&li, sizeof(li), 1, hostfile);
				break;
			}
			if( plen != 0 && written != 1 ) {
				fclose(hostfile);
				dbperror(dbproc, SYBEBCWE, errno);
				return FAIL;
			}

			/* The data */
			if (hostcol->column_len != -1) {
				buflen = buflen > hostcol->column_len ? hostcol->column_len : buflen;
			}

			if (buflen > 0) {
				written = fwrite(hostcol->bcp_column_data->data, buflen, 1, hostfile);
				if (written < 1) {
					fclose(hostfile);
					dbperror(dbproc, SYBEBCWE, errno);
					return FAIL;
				}
			}

			/* The terminator */
			if (hostcol->terminator && hostcol->term_len > 0) {
				written = fwrite(hostcol->terminator, hostcol->term_len, 1, hostfile);
				if (written < 1) {
					fclose(hostfile);
					dbperror(dbproc, SYBEBCWE, errno);
					return FAIL;
				}
			}
		}
		rows_written++;
	}
	if (fclose(hostfile) != 0) {
		dbperror(dbproc, SYBEBCUC, errno);
		return FAIL;
	}
	hostfile = NULL;

	if (dbproc->hostfileinfo->firstrow > 0 && row_of_query < dbproc->hostfileinfo->firstrow) {
		/*
		 * The table which bulk-copy is attempting to
		 * copy to a host-file is shorter than the
		 * number of rows which bulk-copy was instructed to skip.
		 */
		/* TODO reset TDSSOCKET state */
		dbperror(dbproc, SYBETTS, 0);
		return FAIL;
	}

	*rows_copied = rows_written;
	return SUCCEED;
}

static STATUS
_bcp_check_eof(DBPROCESS * dbproc, FILE *file, int icol)
{
	int errnum = errno;

	tdsdump_log(TDS_DBG_FUNC, "_bcp_check_eof(%p, %p, %d)\n", dbproc, file, icol);
	assert(dbproc);
	assert(file);

	if (feof(file)) {
		if (icol == 0) {
			tdsdump_log(TDS_DBG_FUNC, "Normal end-of-file reached while loading bcp data file.\n");
			return NO_MORE_ROWS;
		}
		dbperror(dbproc, SYBEBEOF, errnum);
		return FAIL;
	} 
	dbperror(dbproc, SYBEBCRE, errnum);
	return FAIL;
}

/** 
 * \ingroup dblib_bcp_internal
 * \brief 
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param hostfile 
 * \param row_error 
 * 
 * \return MORE_ROWS, NO_MORE_ROWS, or FAIL.
 * \sa 	BCP_SETL(), bcp_batch(), bcp_bind(), bcp_colfmt(), bcp_colfmt_ps(), bcp_collen(), bcp_colptr(), bcp_columns(), bcp_control(), bcp_done(), bcp_exec(), bcp_getl(), bcp_init(), bcp_moretext(), bcp_options(), bcp_readfmt(), bcp_sendrow()
 */
static STATUS
_bcp_read_hostfile(DBPROCESS * dbproc, FILE * hostfile, int *row_error)
{
	TDSCOLUMN *bcpcol = NULL;
	BCP_HOSTCOLINFO *hostcol;

	TDS_TINYINT ti;
	TDS_SMALLINT si;
	TDS_INT li;
	TDS_INT desttype;
	TDS_CHAR *coldata;

	int i, collen, data_is_null;

	tdsdump_log(TDS_DBG_FUNC, "_bcp_read_hostfile(%p, %p, %p)\n", dbproc, hostfile, row_error);
	assert(dbproc);
	assert(hostfile);
	assert(row_error);

	/* for each host file column defined by calls to bcp_colfmt */

	for (i = 0; i < dbproc->hostfileinfo->host_colcount; i++) {
		tdsdump_log(TDS_DBG_FUNC, "parsing host column %d\n", i + 1);
		hostcol = dbproc->hostfileinfo->host_columns[i];

		data_is_null = 0;
		collen = 0;
		hostcol->column_error = 0;

		/* 
		 * If this host file column contains table data,
		 * find the right element in the table/column list.  
		 * FIXME I think tab_colnum can be out of range - freddy77
		 */
		if (hostcol->tab_colnum) {
			if (hostcol->tab_colnum > dbproc->bcpinfo->bindinfo->num_cols) {
				tdsdump_log(TDS_DBG_FUNC, "error: file wider than table: %d/%d\n", 
							  i+1, dbproc->bcpinfo->bindinfo->num_cols);
				dbperror(dbproc, SYBEBEOF, 0);
				return FAIL;
			}
			tdsdump_log(TDS_DBG_FUNC, "host column %d uses bcpcol %d (%p)\n", 
				                  i+1, hostcol->tab_colnum, bcpcol);
			bcpcol = dbproc->bcpinfo->bindinfo->columns[hostcol->tab_colnum - 1];
			assert(bcpcol != NULL);
		}

		/* detect prefix len */
		if (bcpcol && hostcol->prefix_len == -1) {
			int len = bcpcol->column_varint_size;
			hostcol->prefix_len = len == 5 ? 4 : len;
		}

		/* a prefix length, if extant, specifies how many bytes to read */
		if (hostcol->prefix_len > 0) {

			switch (hostcol->prefix_len) {
			case 1:
				if (fread(&ti, 1, 1, hostfile) != 1)
					return _bcp_check_eof(dbproc, hostfile, i);
				collen = ti ? ti : -1;
				break;
			case 2:
				if (fread(&si, 2, 1, hostfile) != 1)
					return _bcp_check_eof(dbproc, hostfile, i);
				collen = si;
				break;
			case 4:
				if (fread(&li, 4, 1, hostfile) != 1)
					return _bcp_check_eof(dbproc, hostfile, i);
				collen = li;
				break;
			default:
				/* FIXME return error, remember that prefix_len can be 3 */
				assert(hostcol->prefix_len <= 4);
				break;
			}

			/* TODO test all NULL types */
			/* TODO for < -1 error */
			if (collen <= -1) {
				data_is_null = 1;
				collen = 0;
			}
		}

		/* if (Max) column length specified take that into consideration. (Meaning what, exactly?) */

		if (!data_is_null && hostcol->column_len >= 0) {
			if (hostcol->column_len == 0)
				data_is_null = 1;
			else {
				if (collen)
					collen = (hostcol->column_len < collen) ? hostcol->column_len : collen;
				else
					collen = hostcol->column_len;
			}
		}

		tdsdump_log(TDS_DBG_FUNC, "prefix_len = %d collen = %d \n", hostcol->prefix_len, collen);

		/* Fixed Length data - this overrides anything else specified */

		if (is_fixed_type(hostcol->datatype)) {
			collen = tds_get_size_by_type(hostcol->datatype);
		}

		/*
		 * The data file either contains prefixes stating the length, or is delimited.  
		 * If delimited, we "measure" the field by looking for the terminator, then read it, 
		 * and set collen to the field's post-iconv size.  
		 */
		if (hostcol->term_len > 0) { /* delimited data file */
			size_t col_bytes;
			TDSRET conv_res;

			/* 
			 * Read and convert the data
			 */
			coldata = NULL;
			conv_res = tds_bcp_fread(dbproc->tds_socket, bcpcol->char_conv, hostfile, (const char *) hostcol->terminator, hostcol->term_len, &coldata, &col_bytes);

			if (TDS_FAILED(conv_res)) {
				tdsdump_log(TDS_DBG_FUNC, "col %d: error converting %ld bytes!\n",
							(i+1), (long) collen);
				*row_error = TRUE;
				free(coldata);
				dbperror(dbproc, SYBEBCOR, 0);
				return FAIL;
			}

			if (conv_res == TDS_NO_MORE_RESULTS) {
				free(coldata);
				return _bcp_check_eof(dbproc, hostfile, i);
			}

			if (col_bytes > 0x7fffffffl) {
				*row_error = TRUE;
				tdsdump_log(TDS_DBG_FUNC, "data from file is too large!\n");
				dbperror(dbproc, SYBEBCOR, 0);
				return FAIL;
			}

			collen = (int)col_bytes;
			if (collen == 0)
				data_is_null = 1;

			/*
			 * TODO:  
			 *    Dates are a problem.  In theory, we should be able to read non-English dates, which
			 *    would contain non-ASCII characters.  One might suppose we should convert date
			 *    strings to ISO-8859-1 (or another canonical form) here, because tds_convert() can't be
			 *    expected to deal with encodings. But instead date strings are read verbatim and 
			 *    passed to tds_convert() without even waving to iconv().  For English dates, this works, 
			 *    because English dates expressed as UTF-8 strings are indistinguishable from the ASCII.  
			 */
		} else {	/* unterminated field */

			coldata = (TDS_CHAR *) calloc(1, 1 + collen);
			if (coldata == NULL) {
				*row_error = TRUE;
				dbperror(dbproc, SYBEMEM, errno);
				return FAIL;
			}

			if (collen) {
				/* 
				 * Read and convert the data
				 * TODO: Call tds_iconv_fread() instead of fread(3).  
				 *       The columns should each have their iconv cd set, and noncharacter data
				 *       should have -1 as the iconv cd, causing tds_iconv_fread() to not attempt
				 * 	 any conversion.  We do not need a datatype switch here to decide what to do.  
				 *	 As of 0.62, this *should* actually work.  All that remains is to change the
				 *	 call and test it. 
				 */
				tdsdump_log(TDS_DBG_FUNC, "Reading %d bytes from hostfile.\n", collen);
				if (fread(coldata, collen, 1, hostfile) != 1) {
					free(coldata);
					return _bcp_check_eof(dbproc, hostfile, i);
				}
			}
		}

		/* 
		 * At this point, however the field was read, however big it was, its address is coldata and its size is collen.
		 */
		tdsdump_log(TDS_DBG_FUNC, "Data read from hostfile: collen is now %d, data_is_null is %d\n", collen, data_is_null);
		if (hostcol->tab_colnum) {
			if (data_is_null) {
				bcpcol->bcp_column_data->is_null = 1;
				bcpcol->bcp_column_data->datalen = 0;
			} else {
				bcpcol->bcp_column_data->is_null = 0;
				desttype = tds_get_conversion_type(bcpcol->column_type, bcpcol->column_size);

				/* special hack for text columns */

				if (bcpcol->column_size == 4096 && collen > bcpcol->column_size) { /* "4096" might not matter */
					BYTE *oldbuffer = bcpcol->bcp_column_data->data;
					switch (desttype) {
					case SYBTEXT:
					case SYBNTEXT:
					case SYBIMAGE:
					case SYBVARBINARY:
					case XSYBVARBINARY:
					case SYBLONGBINARY:	/* Reallocate enough space for the data from the file. */
						bcpcol->column_size = 8 + collen;	/* room to breathe */
						bcpcol->bcp_column_data->data = 
							(BYTE *) realloc(bcpcol->bcp_column_data->data, bcpcol->column_size);
						if (!bcpcol->bcp_column_data->data) {
							dbperror(dbproc, SYBEMEM, errno);
							free(oldbuffer);
							free(coldata);
							return FAIL;
						}
						break;
					default:
						break;
					}
				}
				/* end special hack for text columns */

				/*
				 * FIXME bcpcol->bcp_column_data->data && bcpcol->column_size ??
				 * It seems a buffer overflow waiting...
				 */
				bcpcol->bcp_column_data->datalen =
					dbconvert(dbproc, hostcol->datatype, (const BYTE *) coldata, collen, desttype, 
									bcpcol->bcp_column_data->data, bcpcol->column_size);

				if (bcpcol->bcp_column_data->datalen == -1) {
					hostcol->column_error = HOST_COL_CONV_ERROR;
					*row_error = 1;
					/* FIXME possible integer overflow if off_t is 64bit and long int 32bit */
					tdsdump_log(TDS_DBG_FUNC, 
						"_bcp_read_hostfile failed to convert %d bytes at offset 0x%lx in the data file.\n", 
						    collen, (unsigned long int) ftello(hostfile) - collen);
				}

				/* trim trailing blanks from character data */
				if (desttype == SYBCHAR || desttype == SYBVARCHAR) {
					bcpcol->bcp_column_data->datalen = rtrim((char *) bcpcol->bcp_column_data->data,
											  bcpcol->bcp_column_data->datalen);
				}
			}
#if USING_SYBEBCNN
			if (!hostcol->column_error) {
				if (bcpcol->bcp_column_data->datalen <= 0) {	/* Are we trying to insert a NULL ? */
					if (!bcpcol->column_nullable) {
						/* too bad if the column is not nullable */
						hostcol->column_error = HOST_COL_NULL_ERROR;
						*row_error = 1;
						dbperror(dbproc, SYBEBCNN, 0);
					}
				}
			}
#endif
		}
		free(coldata);
	}
	return MORE_ROWS;
}

/** 
 * \ingroup dblib_bcp
 * \brief Write data in host variables to the table.  
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * 
 * \remarks Call bcp_bind() first to describe the variables to be used.  
 *	Use bcp_batch() to commit sets of rows. 
 *	After sending the last row call bcp_done().
 * \return SUCCEED or FAIL.
 * \sa 	bcp_batch(), bcp_bind(), bcp_colfmt(), bcp_collen(), bcp_colptr(), bcp_columns(), 
 * 	bcp_control(), bcp_done(), bcp_exec(), bcp_init(), bcp_moretext(), bcp_options()
 */
RETCODE
bcp_sendrow(DBPROCESS * dbproc)
{
	TDSSOCKET *tds;

	tdsdump_log(TDS_DBG_FUNC, "bcp_sendrow(%p)\n", dbproc);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);

	tds = dbproc->tds_socket;

	if (dbproc->bcpinfo->direction != DB_IN) {
		dbperror(dbproc, SYBEBCPN, 0);
		return FAIL;
	}

	if (dbproc->hostfileinfo != NULL) {
		dbperror(dbproc, SYBEBCPB, 0);
		return FAIL;
	}

	/* 
	 * The first time sendrow is called after bcp_init,
	 * there is a certain amount of initialisation to be done.
	 */
	if (dbproc->bcpinfo->xfer_init == 0) {

		/* The start_copy function retrieves details of the table's columns */
		if (TDS_FAILED(tds_bcp_start_copy_in(tds, dbproc->bcpinfo))) {
			dbperror(dbproc, SYBEBULKINSERT, 0);
			return FAIL;
		}

		dbproc->bcpinfo->xfer_init = 1;

	}

	dbproc->bcpinfo->parent = dbproc;
	return TDS_FAILED(tds_bcp_send_record(dbproc->tds_socket, dbproc->bcpinfo, _bcp_get_col_data, NULL, 0)) ? FAIL : SUCCEED;
}


/** 
 * \ingroup dblib_bcp_internal
 * \brief 
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param rows_copied 
 * 
 * \return SUCCEED or FAIL.
 * \sa 	BCP_SETL(), bcp_batch(), bcp_bind(), bcp_colfmt(), bcp_colfmt_ps(), bcp_collen(), bcp_colptr(), bcp_columns(), bcp_control(), bcp_done(), bcp_exec(), bcp_getl(), bcp_init(), bcp_moretext(), bcp_options(), bcp_readfmt(), bcp_sendrow()
 */
static RETCODE
_bcp_exec_in(DBPROCESS * dbproc, DBINT * rows_copied)
{
	FILE *hostfile, *errfile = NULL;
	TDSSOCKET *tds = dbproc->tds_socket;
	BCP_HOSTCOLINFO *hostcol;
	STATUS ret;

	int i, row_of_hostfile, rows_written_so_far;
	int row_error, row_error_count;
	offset_type row_start, row_end;
	offset_type error_row_size;
	const size_t chunk_size = 0x20000u;
	
	tdsdump_log(TDS_DBG_FUNC, "_bcp_exec_in(%p, %p)\n", dbproc, rows_copied);
	assert(dbproc);
	assert(rows_copied);

	*rows_copied = 0;
	
	if (!(hostfile = fopen(dbproc->hostfileinfo->hostfile, "r"))) {
		dbperror(dbproc, SYBEBCUO, 0);
		return FAIL;
	}

	if (TDS_FAILED(tds_bcp_start_copy_in(tds, dbproc->bcpinfo))) {
		fclose(hostfile);
		return FAIL;
	}

	row_of_hostfile = 0;
	rows_written_so_far = 0;

	row_start = ftello(hostfile);
	row_error_count = 0;
	row_error = 0;
	dbproc->bcpinfo->parent = dbproc;

	while ((ret=_bcp_read_hostfile(dbproc, hostfile, &row_error)) == MORE_ROWS) {

		row_of_hostfile++;

		if (row_error) {
			int count;

			if (errfile == NULL && dbproc->hostfileinfo->errorfile) {
				if (!(errfile = fopen(dbproc->hostfileinfo->errorfile, "w"))) {
					fclose(hostfile);
					dbperror(dbproc, SYBEBUOE, 0);
					return FAIL;
				}
			}

			if (errfile != NULL) {
				char *row_in_error = NULL;

				for (i = 0; i < dbproc->hostfileinfo->host_colcount; i++) {
					hostcol = dbproc->hostfileinfo->host_columns[i];
					if (hostcol->column_error == HOST_COL_CONV_ERROR) {
						count = fprintf(errfile, 
							"#@ data conversion error on host data file Row %d Column %d\n",
							row_of_hostfile, i + 1);
						if( count < 0 ) {
							dbperror(dbproc, SYBEBWEF, errno);
						}
					} else if (hostcol->column_error == HOST_COL_NULL_ERROR) {
						count = fprintf(errfile, "#@ Attempt to bulk-copy a NULL value into Server column"
								" which does not accept NULL values. Row %d, Column %d\n",
								row_of_hostfile, i + 1);
						if( count < 0 ) {
							dbperror(dbproc, SYBEBWEF, errno);
						}

					}
				}

				row_end = ftello(hostfile);

				/* error data can be very long so split in chunks */
				error_row_size = row_end - row_start;
				fseeko(hostfile, row_start, SEEK_SET);

				while (error_row_size > 0) {
					size_t chunk = error_row_size > chunk_size ? chunk_size : (size_t) error_row_size;

					if (!row_in_error) {
						if ((row_in_error = (char*) malloc(chunk)) == NULL) {
							dbperror(dbproc, SYBEMEM, errno);
						}
					}

					if (fread(row_in_error, chunk, 1, hostfile) != 1) {
						printf("BILL fread failed after fseek\n");
					}
					count = (int)fwrite(row_in_error, chunk, 1, errfile);
					if( (size_t)count < chunk ) {
						dbperror(dbproc, SYBEBWEF, errno);
					}
					error_row_size -= chunk;
				}
				free(row_in_error);

				fseeko(hostfile, row_end, SEEK_SET);
				count = fprintf(errfile, "\n");
				if( count < 0 ) {
					dbperror(dbproc, SYBEBWEF, errno);
				}
			}
			row_error_count++;
			if (row_error_count > dbproc->hostfileinfo->maxerrs)
				break;
		} else {
			if (dbproc->hostfileinfo->firstrow <= row_of_hostfile && 
							      row_of_hostfile <= MAX(dbproc->hostfileinfo->lastrow, 0x7FFFFFFF)) {

				if (TDS_SUCCEED(tds_bcp_send_record(dbproc->tds_socket, dbproc->bcpinfo, _bcp_no_get_col_data, NULL, 0))) {
			
					rows_written_so_far++;
	
					if (dbproc->hostfileinfo->batch > 0 && rows_written_so_far == dbproc->hostfileinfo->batch) {
						if (TDS_FAILED(tds_bcp_done(tds, &rows_written_so_far))) {
							if (errfile)
								fclose(errfile);
							fclose(hostfile);
							return FAIL;
						}
							
						*rows_copied += rows_written_so_far;
						rows_written_so_far = 0;
	
						dbperror(dbproc, SYBEBBCI, 0); /* batch copied to server */
	
						tds_bcp_start(tds, dbproc->bcpinfo);
					}
				}
			} 
		}

		row_start = ftello(hostfile);
		row_error = 0;
	}
	
	if( row_error_count == 0 && row_of_hostfile < dbproc->hostfileinfo->firstrow ) {
		/* "The BCP hostfile '%1!' contains only %2! rows.  */
		dbperror(dbproc, SYBEBCSA, 0, dbproc->hostfileinfo->hostfile, row_of_hostfile); 
	}

	if (errfile &&  0 != fclose(errfile) ) {
		dbperror(dbproc, SYBEBUCE, 0);
	}

	if (fclose(hostfile) != 0) {
		dbperror(dbproc, SYBEBCUC, 0);
		ret = FAIL;
	}

	tds_bcp_done(tds, &rows_written_so_far);
	*rows_copied += rows_written_so_far;

	return ret == NO_MORE_ROWS? SUCCEED : FAIL;	/* (ret is returned from _bcp_read_hostfile) */
}

/** 
 * \ingroup dblib_bcp
 * \brief Write a datafile to a table. 
 *
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param rows_copied bcp_exec will write the count of rows successfully written to this address. 
 *	If \a rows_copied is NULL, it will be ignored by db-lib. 
 *
 * \return SUCCEED or FAIL.
 * \sa 	bcp_batch(), bcp_bind(), bcp_colfmt(), bcp_collen(), bcp_colptr(), bcp_columns(),
 *	bcp_control(), bcp_done(), bcp_init(), bcp_sendrow()
 */
RETCODE
bcp_exec(DBPROCESS * dbproc, DBINT *rows_copied)
{
	DBINT dummy_copied;
	RETCODE ret = FAIL;

	tdsdump_log(TDS_DBG_FUNC, "bcp_exec(%p, %p)\n", dbproc, rows_copied);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);
	CHECK_PARAMETER(dbproc->hostfileinfo, SYBEBCVH, FAIL);

	if (rows_copied == NULL) /* NULL means we should ignore it */
		rows_copied = &dummy_copied;

	if (dbproc->bcpinfo->direction == DB_OUT || dbproc->bcpinfo->direction == DB_QUERYOUT) {
		ret = _bcp_exec_out(dbproc, rows_copied);
	} else if (dbproc->bcpinfo->direction == DB_IN) {
		ret = _bcp_exec_in(dbproc, rows_copied);
	}
	_bcp_free_storage(dbproc);
	
	return ret;
}

/** 
 * \ingroup dblib_bcp_internal
 * \brief 
 *
 * \param buffer 
 * \param size 
 * \param f 
 * 
 * \return SUCCEED or FAIL.
 * \sa 	BCP_SETL(), bcp_batch(), bcp_bind(), bcp_colfmt(), bcp_colfmt_ps(), bcp_collen(), bcp_colptr(), bcp_columns(), bcp_control(), bcp_done(), bcp_exec(), bcp_getl(), bcp_init(), bcp_moretext(), bcp_options(), bcp_readfmt(), bcp_sendrow()
 */
static char *
_bcp_fgets(char *buffer, int size, FILE *f)
{
	char *p = fgets(buffer, size, f);
	if (p == NULL) 
		return p;

	/* discard newline */
	p = strchr(buffer, 0) - 1;
	if (p >= buffer && *p == '\n')
		*p = 0;
	return buffer;
}

/** 
 * \ingroup dblib_bcp
 * \brief Read a format definition file.
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param filename Name that will be passed to fopen(3).  
 * 
 * \remarks Reads a format file and calls bcp_columns() and bcp_colfmt() as needed. 
 *
 * \return SUCCEED or FAIL.
 * \sa 	bcp_colfmt(), bcp_colfmt_ps(), bcp_columns(), bcp_writefmt()
 */
RETCODE
bcp_readfmt(DBPROCESS * dbproc, const char filename[])
{
	BCP_HOSTCOLINFO hostcol[1];
	FILE *ffile;
	char buffer[1024];
	float lf_version = 0.0;
	int li_numcols = 0;
	int colinfo_count = 0;

	tdsdump_log(TDS_DBG_FUNC, "bcp_readfmt(%p, %s)\n", dbproc, filename? filename:"NULL");
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);
	CHECK_NULP(filename, "bcp_readfmt", 2, FAIL);

	if ((ffile = fopen(filename, "r")) == NULL) {
		dbperror(dbproc, SYBEBUOF, 0);
		return FAIL;
	}

	if ((_bcp_fgets(buffer, sizeof(buffer), ffile)) != NULL) {
		lf_version = (float)atof(buffer);
	} else if (ferror(ffile)) {
		dbperror(dbproc, SYBEBRFF, errno);
		return FAIL;
	}

	if ((_bcp_fgets(buffer, sizeof(buffer), ffile)) != NULL) {
		li_numcols = atoi(buffer);
	} else if (ferror(ffile)) {
		dbperror(dbproc, SYBEBRFF, errno);
		return FAIL;
	}

	if (li_numcols <= 0)
		return FAIL;

	if (bcp_columns(dbproc, li_numcols) == FAIL)
		return FAIL;

	do {
		memset(hostcol, 0, sizeof(hostcol));

		if (_bcp_fgets(buffer, sizeof(buffer), ffile) == NULL)
			goto Cleanup;

		if (!_bcp_readfmt_colinfo(dbproc, buffer, hostcol))
			goto Cleanup;

		if (bcp_colfmt(dbproc, hostcol->host_column, hostcol->datatype,
			       hostcol->prefix_len, hostcol->column_len,
			       hostcol->terminator, hostcol->term_len, hostcol->tab_colnum) == FAIL) {
			goto Cleanup;
		}

		TDS_ZERO_FREE(hostcol->terminator);
	} while (++colinfo_count < li_numcols);

	if (ferror(ffile)) {
		dbperror(dbproc, SYBEBRFF, errno);
		goto Cleanup;
	}
	
	if (fclose(ffile) != 0) {
		dbperror(dbproc, SYBEBUCF, 0);
		goto Cleanup;
	}

	if (colinfo_count != li_numcols)
		goto Cleanup;

	return SUCCEED;

Cleanup:
	TDS_ZERO_FREE(hostcol->terminator);
	_bcp_free_columns(dbproc);
	return FAIL;
}

/** 
 * \ingroup dblib_bcp_internal
 * \brief 
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param buf 
 * \param ci 
 * 
 * \return SUCCEED or FAIL.
 * \sa 	BCP_SETL(), bcp_batch(), bcp_bind(), bcp_colfmt(), bcp_colfmt_ps(), bcp_collen(), bcp_colptr(), bcp_columns(), bcp_control(), bcp_done(), bcp_exec(), bcp_getl(), bcp_init(), bcp_moretext(), bcp_options(), bcp_readfmt(), bcp_sendrow()
 */
static int
_bcp_readfmt_colinfo(DBPROCESS * dbproc, char *buf, BCP_HOSTCOLINFO * ci)
{
	char *tok;
	int whichcol;
	char term[30];
	int i;
	char *lasts;

	enum nextcol
	{
		HOST_COLUMN,
		DATATYPE,
		PREFIX_LEN,
		COLUMN_LEN,
		TERMINATOR,
		TAB_COLNUM,
		NO_MORE_COLS
	};

	assert(dbproc);
	assert(buf);
	assert(ci);
	tdsdump_log(TDS_DBG_FUNC, "_bcp_readfmt_colinfo(%p, %s, %p)\n", dbproc, buf, ci);

	tok = strtok_r(buf, " \t", &lasts);
	whichcol = HOST_COLUMN;

	/* TODO use a better way to get an int atoi is very error prone */
	while (tok != NULL && whichcol != NO_MORE_COLS) {
		switch (whichcol) {

		case HOST_COLUMN:
			ci->host_column = atoi(tok);

			if (ci->host_column < 1) {
				dbperror(dbproc, SYBEBIHC, 0);
				return (FALSE);
			}

			whichcol = DATATYPE;
			break;

		case DATATYPE:
			if (strcmp(tok, "SYBCHAR") == 0)
				ci->datatype = SYBCHAR;
			else if (strcmp(tok, "SYBTEXT") == 0)
				ci->datatype = SYBTEXT;
			else if (strcmp(tok, "SYBBINARY") == 0)
				ci->datatype = SYBBINARY;
			else if (strcmp(tok, "SYBIMAGE") == 0)
				ci->datatype = SYBIMAGE;
			else if (strcmp(tok, "SYBINT1") == 0)
				ci->datatype = SYBINT1;
			else if (strcmp(tok, "SYBINT2") == 0)
				ci->datatype = SYBINT2;
			else if (strcmp(tok, "SYBINT4") == 0)
				ci->datatype = SYBINT4;
			else if (strcmp(tok, "SYBINT8") == 0)
				ci->datatype = SYBINT8;
			else if (strcmp(tok, "SYBFLT8") == 0)
				ci->datatype = SYBFLT8;
			else if (strcmp(tok, "SYBREAL") == 0)
				ci->datatype = SYBREAL;
			else if (strcmp(tok, "SYBBIT") == 0)
				ci->datatype = SYBBIT;
			else if (strcmp(tok, "SYBNUMERIC") == 0)
				ci->datatype = SYBNUMERIC;
			else if (strcmp(tok, "SYBDECIMAL") == 0)
				ci->datatype = SYBDECIMAL;
			else if (strcmp(tok, "SYBMONEY") == 0)
				ci->datatype = SYBMONEY;
			else if (strcmp(tok, "SYBMONEY4") == 0)
				ci->datatype = SYBMONEY4;
			else if (strcmp(tok, "SYBDATETIME") == 0)
				ci->datatype = SYBDATETIME;
			else if (strcmp(tok, "SYBDATETIME4") == 0)
				ci->datatype = SYBDATETIME4;
			/* TODO SQL* for MS
			   SQLNCHAR SQLBIGINT SQLTINYINT SQLSMALLINT
			   SQLUNIQUEID SQLVARIANT SQLUDT */
			else {
				dbperror(dbproc, SYBEBUDF, 0);
				return (FALSE);
			}

			whichcol = PREFIX_LEN;
			break;

		case PREFIX_LEN:
			ci->prefix_len = atoi(tok);
			whichcol = COLUMN_LEN;
			break;
		case COLUMN_LEN:
			ci->column_len = atoi(tok);
			whichcol = TERMINATOR;
			break;
		case TERMINATOR:

			if (*tok++ != '\"')
				return (FALSE);

			for (i = 0; *tok != '\"' && i < sizeof(term); i++) {
				if (*tok == '\\') {
					tok++;
					switch (*tok) {
					case 't':
						term[i] = '\t';
						break;
					case 'n':
						term[i] = '\n';
						break;
					case 'r':
						term[i] = '\r';
						break;
					case '\\':
						term[i] = '\\';
						break;
					case '0':
						term[i] = '\0';
						break;
					default:
						return (FALSE);
					}
					tok++;
				} else
					term[i] = *tok++;
			}

			if (*tok != '\"')
				return (FALSE);

			if ((ci->terminator = (BYTE*) malloc(i)) == NULL) {
				dbperror(dbproc, SYBEMEM, errno);
				return FALSE;
			}
			
			memcpy(ci->terminator, term, i);
			ci->term_len = i;

			whichcol = TAB_COLNUM;
			break;

		case TAB_COLNUM:
			ci->tab_colnum = atoi(tok);
			whichcol = NO_MORE_COLS;
			break;

		}
		tok = strtok_r(NULL, " \t", &lasts);
	}
	if (whichcol == NO_MORE_COLS)
		return (TRUE);
	else
		return (FALSE);
}

#if defined(DBLIB_UNIMPLEMENTED)
/** 
 * \ingroup dblib_bcp
 * \brief Write a format definition file. Not Implemented. 
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param filename Name that would be passed to fopen(3).  
 * 
 * \remarks Reads a format file and calls bcp_columns() and bcp_colfmt() as needed. 
 * \a FreeTDS includes freebcp, a utility to copy data to or from a host file. 
 *
 * \todo For completeness, \a freebcp ought to be able to create format files, but that functionality 
 * 	is currently lacking, as is bcp_writefmt().
 * \todo See the vendors' documentation for the format of these files.
 *
 * \return SUCCEED or FAIL.
 * \sa 	bcp_colfmt(), bcp_colfmt_ps(), bcp_columns(), bcp_readfmt()
 */
RETCODE
bcp_writefmt(DBPROCESS * dbproc, const char filename[])
{
	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED: bcp_writefmt(%p, %s)\n", dbproc, filename? filename:"NULL");
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);
	CHECK_NULP(filename, "bcp_writefmt", 2, FAIL);

#if 0
	dbperror(dbproc, SYBEBUFF, errno);	/* bcp: Unable to create format file */
	dbperror(dbproc, SYBEBWFF, errno);	/* I/O error while writing bcp format file */
#endif

	return FAIL;
}

/** 
 * \ingroup dblib_bcp
 * \brief Write some text or image data to the server.  Not implemented, sadly.  
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param size How much to write, in bytes. 
 * \param text Address of the data to be written. 
 * \remarks For a SYBTEXT or SYBIMAGE column, bcp_bind() can be called with 
 *	a NULL varaddr parameter.  If it is, bcp_sendrow() will return control
 *	to the application after the non-text data have been sent.  The application then calls 
 *	bcp_moretext() -- usually in a loop -- to send the text data in manageable chunks.  
 * \todo implement bcp_moretext().
 * \return SUCCEED or FAIL.
 * \sa 	bcp_bind(), bcp_sendrow(), dbmoretext(), dbwritetext()
 */
RETCODE
bcp_moretext(DBPROCESS * dbproc, DBINT size, BYTE * text)
{
	tdsdump_log(TDS_DBG_FUNC, "UNIMPLEMENTED: bcp_moretext(%p, %d, %p)\n", dbproc, size, text);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);
	CHECK_NULP(text, "bcp_moretext", 3, FAIL);
	
#if 0
	dbperror(dbproc, SYBEBCMTXT, 0); 
		/* bcp_moretext may be used only when there is at least one text or image column in the server table */
	dbperror(dbproc, SYBEBTMT, 0); 
		/* Attempt to send too much text data via the bcp_moretext call */
#endif
	return FAIL;
}
#endif

/** 
 * \ingroup dblib_bcp
 * \brief Commit a set of rows to the table. 
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \remarks If not called, bcp_done() will cause the rows to be saved.  
 * \return Count of rows saved, or -1 on error.
 * \sa 	bcp_bind(), bcp_done(), bcp_sendrow()
 */
DBINT
bcp_batch(DBPROCESS * dbproc)
{
	int rows_copied = 0;

	tdsdump_log(TDS_DBG_FUNC, "bcp_batch(%p)\n", dbproc);
	CHECK_CONN(-1);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, -1);

	if (TDS_FAILED(tds_bcp_done(dbproc->tds_socket, &rows_copied)))
		return -1;

	tds_bcp_start(dbproc->tds_socket, dbproc->bcpinfo);

	return rows_copied;
}

/** 
 * \ingroup dblib_bcp
 * \brief Conclude the transfer of data from program variables.
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \remarks Do not overlook this function.  According to Sybase, failure to call bcp_done() 
 * "will result in unpredictable errors".
 * \return As with bcp_batch(), the count of rows saved, or -1 on error.
 * \sa 	bcp_batch(), bcp_bind(), bcp_moretext(), bcp_sendrow()
 */
DBINT
bcp_done(DBPROCESS * dbproc)
{
	int rows_copied;

	tdsdump_log(TDS_DBG_FUNC, "bcp_done(%p)\n", dbproc);
	CHECK_CONN(-1);
	
	if (!(dbproc->bcpinfo))
		return -1;

	if (TDS_FAILED(tds_bcp_done(dbproc->tds_socket, &rows_copied)))
		return -1;

	_bcp_free_storage(dbproc);

	return rows_copied;
}

/** 
 * \ingroup dblib_bcp
 * \brief Bind a program host variable to a database column
 * 
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param varaddr address of host variable
 * \param prefixlen length of any prefix found at the beginning of \a varaddr, in bytes.  
 	Use zero for fixed-length datatypes. 
 * \param varlen bytes of data in \a varaddr.  Zero for NULL, -1 for fixed-length datatypes. 
 * \param terminator byte sequence that marks the end of the data in \a varaddr
 * \param termlen length of \a terminator
 * \param vartype datatype of the host variable
 * \param table_column Nth column, starting at 1, in the table.
 * 
 * \remarks The order of operation is:
 *	- bcp_init() with \a hfile == NULL and \a direction == DB_IN.
 * 	- bcp_bind(), once per column you want to write to
 *	- bcp_batch(), optionally, to commit a set of rows
 *	- bcp_done() 
 * \return SUCCEED or FAIL.
 * \sa 	bcp_batch(), bcp_colfmt(), bcp_collen(), bcp_colptr(), bcp_columns(), bcp_control(), 
 * 	bcp_done(), bcp_exec(), bcp_moretext(), bcp_sendrow() 
 */
RETCODE
bcp_bind(DBPROCESS * dbproc, BYTE * varaddr, int prefixlen, DBINT varlen,
	 BYTE * terminator, int termlen, int vartype, int table_column)
{
	TDSCOLUMN *colinfo;

	tdsdump_log(TDS_DBG_FUNC, "bcp_bind(%p, %p, %d, %d -- %p, %d, %s, %d)\n", 
						dbproc, varaddr, prefixlen, varlen, 
						terminator, termlen, dbprtype(vartype), table_column);
	CHECK_CONN(FAIL);
	CHECK_PARAMETER(dbproc->bcpinfo, SYBEBCPI, FAIL);

	if (dbproc->hostfileinfo != NULL) {
		dbperror(dbproc, SYBEBCPB, 0);
		return FAIL;
	}

	if (dbproc->bcpinfo->direction != DB_IN) {
		dbperror(dbproc, SYBEBCPN, 0);
		return FAIL;
	}

	if (varlen < -1) {
		dbperror(dbproc, SYBEBCVLEN, 0);
		return FAIL;
	}

	if (prefixlen != 0 && prefixlen != 1 && prefixlen != 2 && prefixlen != 4) {
		dbperror(dbproc, SYBEBCBPREF, 0);
		return FAIL;
	}

	if (prefixlen == 0 && varlen == -1 && termlen == -1 && !is_fixed_type(vartype)) {
		tdsdump_log(TDS_DBG_FUNC, "bcp_bind(): non-fixed type %d requires prefix or terminator\n", vartype);
		return FAIL;
	}

	if (is_fixed_type(vartype) && (varlen != -1 && varlen != 0)) {
		dbperror(dbproc, SYBEBCIT, 0);
		return FAIL;
	}

	if (table_column <= 0 ||  table_column > dbproc->bcpinfo->bindinfo->num_cols) {
		dbperror(dbproc, SYBECNOR, 0);
		return FAIL;
	}
	
	if (varaddr == NULL && (prefixlen != 0 || termlen != 0)) {
		dbperror(dbproc, SYBEBCBNPR, 0);
		return FAIL;
	}

	colinfo = dbproc->bcpinfo->bindinfo->columns[table_column - 1];

	/* If varaddr is NULL and varlen greater than 0, the table column type must be SYBTEXT or SYBIMAGE 
		and the program variable type must be SYBTEXT, SYBCHAR, SYBIMAGE or SYBBINARY */
	if (varaddr == NULL && varlen > 0) {
		int fOK = (colinfo->column_type == SYBTEXT || colinfo->column_type == SYBIMAGE) &&
			  (vartype == SYBTEXT || vartype == SYBCHAR || vartype == SYBIMAGE || vartype == SYBBINARY );
		if( !fOK ) {
			dbperror(dbproc, SYBEBCBNTYP, 0);
			tdsdump_log(TDS_DBG_FUNC, "bcp_bind: SYBEBCBNTYP: column=%d and vartype=%d (should fail?)\n", 
							colinfo->column_type, vartype);
			/* return FAIL; */
		}
	}

	colinfo->column_varaddr  = (char *)varaddr;
	colinfo->column_bindtype = vartype;
	colinfo->column_bindlen  = varlen;

	TDS_ZERO_FREE(colinfo->bcp_terminator);
	colinfo->bcp_term_len = 0;
	if (termlen) {
		if ((colinfo->bcp_terminator =  (TDS_CHAR*) malloc(termlen)) == NULL) {
			dbperror(dbproc, SYBEMEM, errno);
			return FAIL;
		}
		memcpy(colinfo->bcp_terminator, terminator, termlen);
		colinfo->bcp_term_len = termlen;
	}

	return SUCCEED;
}

/** 
 * \ingroup dblib_bcp_internal
 * \brief For a bcp in from program variables, get the data from the host variable
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * \param bindcol 
 * 
 * \return TDS_SUCCESS or TDS_FAIL.
 * \sa 	_bcp_add_fixed_columns, _bcp_add_variable_columns, _bcp_send_bcp_record
 */
static TDSRET
_bcp_get_col_data(TDSBCPINFO *bcpinfo, TDSCOLUMN *bindcol, int offset)
{
	TDS_TINYINT ti;
	TDS_SMALLINT si;
	TDS_INT li;
	TDS_INT desttype;
	int collen, coltype;
	int data_is_null;
	int bytes_read;
	int converted_data_size;
	BYTE *dataptr;
	DBPROCESS *dbproc = (DBPROCESS *) bcpinfo->parent;

	tdsdump_log(TDS_DBG_FUNC, "_bcp_get_col_data(%p, %p)\n", bcpinfo, bindcol);
	CHECK_CONN(TDS_FAIL);
	CHECK_NULP(bindcol, "_bcp_get_col_data", 2, TDS_FAIL);

	dataptr = (BYTE *) bindcol->column_varaddr;

	data_is_null = 0;
	collen = 0;

	/* If a prefix length specified, read the correct  amount of data. */

	if (bindcol->bcp_prefix_len > 0) {

		switch (bindcol->bcp_prefix_len) {
		case 1:
			memcpy(&ti, dataptr, 1);
			dataptr += 1;
			collen = ti;
			break;
		case 2:
			memcpy(&si, dataptr, 2);
			dataptr += 2;
			collen = si;
			break;
		case 4:
			memcpy(&li, dataptr, 4);
			dataptr += 4;
			collen = li;
			break;
		}
		if (collen == 0)
			data_is_null = 1;

	}

	/* if (Max) column length specified take that into consideration. */

	if (!data_is_null && bindcol->column_bindlen >= 0) {
		if (bindcol->column_bindlen == 0)
			data_is_null = 1;
		else {
			if (collen)
				collen = (int) ((bindcol->column_bindlen < (TDS_UINT)collen) ? bindcol->column_bindlen : (TDS_UINT)collen);
			else
				collen = (int) bindcol->column_bindlen;
		}
	}

	desttype = tds_get_conversion_type(bindcol->column_type, bindcol->column_size);

	/* Fixed Length data - this overrides anything else specified */
	coltype = bindcol->column_bindtype == 0 ? desttype : bindcol->column_bindtype;
	if (is_fixed_type(coltype)) {
		collen = tds_get_size_by_type(coltype);
	}

	/* read the data, finally */

	if (bindcol->bcp_term_len > 0) {	/* terminated field */
		bytes_read = _bcp_get_term_var(dataptr, (BYTE *)bindcol->bcp_terminator, bindcol->bcp_term_len);

		if (collen)
			collen = (bytes_read < collen) ? bytes_read : collen;
		else
			collen = bytes_read;

		if (collen == 0)
			data_is_null = 1;
	}

	if (data_is_null) {
		bindcol->bcp_column_data->datalen = 0;
		bindcol->bcp_column_data->is_null = 1;
	} else {
		if ((converted_data_size =
		     dbconvert(dbproc, coltype,
			       (BYTE *) dataptr, collen,
			       desttype, bindcol->bcp_column_data->data, bindcol->column_size)) == -1) {
			return TDS_FAIL;
		}

		bindcol->bcp_column_data->datalen = converted_data_size;
		bindcol->bcp_column_data->is_null = 0;
		assert(converted_data_size > 0);
	}

	return TDS_SUCCESS;
}

static TDSRET
_bcp_no_get_col_data(TDSBCPINFO *bcpinfo, TDSCOLUMN *bindcol, int offset)
{
	return TDS_SUCCESS;
}

/**
 * Get the data for bcp-in from program variables, where the program data
 * have been identified as character terminated,  
 * This is a low-level, internal function.  Call it correctly.  
 */
/** 
 * \ingroup dblib_bcp_internal
 * \brief 
 *
 * \param pdata 
 * \param term 
 * \param term_len 
 * 
 * \return data length.
 */
static int
_bcp_get_term_var(BYTE * pdata, BYTE * term, int term_len)
{
	int bufpos;

	assert(term_len > 0);

	/* if bufpos becomes negative, we probably failed to find the terminator */
	for (bufpos = 0; bufpos >= 0 && memcmp(pdata, term, term_len) != 0; pdata++) {
		bufpos++;
	}
	
	assert(bufpos >= 0);
	return bufpos;
}

/** 
 * \ingroup dblib_bcp_internal
 * \brief trim a string of trailing blanks 
 *
 *	Replaces spaces at the end of a string with NULs
 * \param str pointer to a character buffer (not null-terminated)
 * \param len size of the \a str in bytes 
 * 
 * \return modified length
 */
static int
rtrim(char *str, int len)
{
	char *p = str + len - 1;

	while (p > str && *p == ' ') {
		*p-- = '\0';
	}
	return (int)(1 + p - str);
}

/** 
 * \ingroup dblib_bcp_internal
 * \brief 
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 */
static void
_bcp_free_columns(DBPROCESS * dbproc)
{
	int i;

	tdsdump_log(TDS_DBG_FUNC, "_bcp_free_columns(%p)\n", dbproc);
	assert(dbproc && dbproc->hostfileinfo);

	if (dbproc->hostfileinfo->host_columns) {
		for (i = 0; i < dbproc->hostfileinfo->host_colcount; i++) {
			TDS_ZERO_FREE(dbproc->hostfileinfo->host_columns[i]->terminator);
			tds_free_bcp_column_data(dbproc->hostfileinfo->host_columns[i]->bcp_column_data);
			TDS_ZERO_FREE(dbproc->hostfileinfo->host_columns[i]);
		}
		TDS_ZERO_FREE(dbproc->hostfileinfo->host_columns);
		dbproc->hostfileinfo->host_colcount = 0;
	}
}

/** 
 * \ingroup dblib_bcp_internal
 * \brief 
 *
 * \param dbproc contains all information needed by db-lib to manage communications with the server.
 * 
 * \sa 	bcp_done(), bcp_exec(), bcp_init()
 */
static void
_bcp_free_storage(DBPROCESS * dbproc)
{
	tdsdump_log(TDS_DBG_FUNC, "_bcp_free_storage(%p)\n", dbproc);
	assert(dbproc);

	if (dbproc->hostfileinfo) {
		TDS_ZERO_FREE(dbproc->hostfileinfo->hostfile);
		TDS_ZERO_FREE(dbproc->hostfileinfo->errorfile);
		_bcp_free_columns(dbproc);
		TDS_ZERO_FREE(dbproc->hostfileinfo);
	}


	if (dbproc->bcpinfo) {
		TDS_ZERO_FREE(dbproc->bcpinfo->tablename);
		TDS_ZERO_FREE(dbproc->bcpinfo->insert_stmt);
		tds_free_results(dbproc->bcpinfo->bindinfo);
		dbproc->bcpinfo->bindinfo = NULL;
		TDS_ZERO_FREE(dbproc->bcpinfo);
	}
}

