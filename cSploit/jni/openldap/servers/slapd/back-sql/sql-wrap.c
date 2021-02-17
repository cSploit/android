/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 Dmitry Kovalev.
 * Portions Copyright 2002 Pierangelo Masarati.
 * Portions Copyright 2004 Mark Adamson.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Dmitry Kovalev for inclusion
 * by OpenLDAP Software.  Additional significant contributors include
 * Pierangelo Masarati and Mark Adamson.
 */

#include "portable.h"

#include <stdio.h>
#include "ac/string.h"
#include <sys/types.h>

#include "slap.h"
#include "proto-sql.h"

#define MAX_ATTR_LEN 16384

void
backsql_PrintErrors( SQLHENV henv, SQLHDBC hdbc, SQLHSTMT sth, int rc )
{
	SQLCHAR	msg[SQL_MAX_MESSAGE_LENGTH];		/* msg. buffer    */
	SQLCHAR	state[SQL_SQLSTATE_SIZE];		/* statement buf. */
	SDWORD	iSqlCode;				/* return code    */
	SWORD	len = SQL_MAX_MESSAGE_LENGTH - 1;	/* return length  */ 

	Debug( LDAP_DEBUG_TRACE, "Return code: %d\n", rc, 0, 0 );

	for ( ; rc = SQLError( henv, hdbc, sth, state, &iSqlCode, msg,
		SQL_MAX_MESSAGE_LENGTH - 1, &len ), BACKSQL_SUCCESS( rc ); )
	{
		Debug( LDAP_DEBUG_TRACE,
			"   nativeErrCode=%d SQLengineState=%s msg=\"%s\"\n",
			(int)iSqlCode, state, msg );
	}
}

RETCODE
backsql_Prepare( SQLHDBC dbh, SQLHSTMT *sth, const char *query, int timeout )
{
	RETCODE		rc;

	rc = SQLAllocStmt( dbh, sth );
	if ( rc != SQL_SUCCESS ) {
		return rc;
	}

#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "==>backsql_Prepare()\n", 0, 0, 0 );
#endif /* BACKSQL_TRACE */

#ifdef BACKSQL_MSSQL_WORKAROUND
	{
		char		drv_name[ 30 ];
		SWORD		len;

		SQLGetInfo( dbh, SQL_DRIVER_NAME, drv_name, sizeof( drv_name ), &len );

#ifdef BACKSQL_TRACE
		Debug( LDAP_DEBUG_TRACE, "backsql_Prepare(): driver name=\"%s\"\n",
				drv_name, 0, 0 );
#endif /* BACKSQL_TRACE */

		ldap_pvt_str2upper( drv_name );
		if ( !strncmp( drv_name, "SQLSRV32.DLL", STRLENOF( "SQLSRV32.DLL" ) ) ) {
			/*
			 * stupid default result set in MS SQL Server
			 * does not support multiple active statements
			 * on the same connection -- so we are trying 
			 * to make it not to use default result set...
			 */
			Debug( LDAP_DEBUG_TRACE, "_SQLprepare(): "
				"enabling MS SQL Server default result "
				"set workaround\n", 0, 0, 0 );
			rc = SQLSetStmtOption( *sth, SQL_CONCURRENCY, 
					SQL_CONCUR_ROWVER );
			if ( rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO ) {
				Debug( LDAP_DEBUG_TRACE, "backsql_Prepare(): "
					"SQLSetStmtOption(SQL_CONCURRENCY,"
					"SQL_CONCUR_ROWVER) failed:\n", 
					0, 0, 0 );
				backsql_PrintErrors( SQL_NULL_HENV, dbh, *sth, rc );
				SQLFreeStmt( *sth, SQL_DROP );
				return rc;
			}
		}
	}
#endif /* BACKSQL_MSSQL_WORKAROUND */

	if ( timeout > 0 ) {
		Debug( LDAP_DEBUG_TRACE, "_SQLprepare(): "
			"setting query timeout to %d sec.\n", 
			timeout, 0, 0 );
		rc = SQLSetStmtOption( *sth, SQL_QUERY_TIMEOUT, timeout );
		if ( rc != SQL_SUCCESS ) {
			backsql_PrintErrors( SQL_NULL_HENV, dbh, *sth, rc );
			SQLFreeStmt( *sth, SQL_DROP );
			return rc;
		}
	}

#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "<==backsql_Prepare() calling SQLPrepare()\n",
			0, 0, 0 );
#endif /* BACKSQL_TRACE */

	return SQLPrepare( *sth, (SQLCHAR *)query, SQL_NTS );
}

RETCODE
backsql_BindRowAsStrings_x( SQLHSTMT sth, BACKSQL_ROW_NTS *row, void *ctx )
{
	RETCODE		rc;

	if ( row == NULL ) {
		return SQL_ERROR;
	}

#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "==> backsql_BindRowAsStrings()\n", 0, 0, 0 );
#endif /* BACKSQL_TRACE */
	
	rc = SQLNumResultCols( sth, &row->ncols );
	if ( rc != SQL_SUCCESS ) {
#ifdef BACKSQL_TRACE
		Debug( LDAP_DEBUG_TRACE, "backsql_BindRowAsStrings(): "
			"SQLNumResultCols() failed:\n", 0, 0, 0 );
#endif /* BACKSQL_TRACE */
		
		backsql_PrintErrors( SQL_NULL_HENV, SQL_NULL_HDBC, sth, rc );

	} else {
		SQLCHAR		colname[ 64 ];
		SQLSMALLINT	name_len, col_type, col_scale, col_null;
		SQLLEN		col_prec;
		int		i;

#ifdef BACKSQL_TRACE
		Debug( LDAP_DEBUG_TRACE, "backsql_BindRowAsStrings: "
			"ncols=%d\n", (int)row->ncols, 0, 0 );
#endif /* BACKSQL_TRACE */

		row->col_names = (BerVarray)ber_memcalloc_x( row->ncols + 1, 
				sizeof( struct berval ), ctx );
		if ( row->col_names == NULL ) {
			goto nomem;
		}

		row->col_prec = (UDWORD *)ber_memcalloc_x( row->ncols,
				sizeof( UDWORD ), ctx );
		if ( row->col_prec == NULL ) {
			goto nomem;
		}

		row->col_type = (SQLSMALLINT *)ber_memcalloc_x( row->ncols,
				sizeof( SQLSMALLINT ), ctx );
		if ( row->col_type == NULL ) {
			goto nomem;
		}

		row->cols = (char **)ber_memcalloc_x( row->ncols + 1, 
				sizeof( char * ), ctx );
		if ( row->cols == NULL ) {
			goto nomem;
		}

		row->value_len = (SQLLEN *)ber_memcalloc_x( row->ncols,
				sizeof( SQLLEN ), ctx );
		if ( row->value_len == NULL ) {
			goto nomem;
		}

		if ( 0 ) {
nomem:
			ber_memfree_x( row->col_names, ctx );
			row->col_names = NULL;
			ber_memfree_x( row->col_prec, ctx );
			row->col_prec = NULL;
			ber_memfree_x( row->col_type, ctx );
			row->col_type = NULL;
			ber_memfree_x( row->cols, ctx );
			row->cols = NULL;
			ber_memfree_x( row->value_len, ctx );
			row->value_len = NULL;

			Debug( LDAP_DEBUG_ANY, "backsql_BindRowAsStrings: "
				"out of memory\n", 0, 0, 0 );

			return LDAP_NO_MEMORY;
		}

		for ( i = 0; i < row->ncols; i++ ) {
			SQLSMALLINT	TargetType;

			rc = SQLDescribeCol( sth, (SQLSMALLINT)(i + 1), &colname[ 0 ],
					(SQLUINTEGER)( sizeof( colname ) - 1 ),
					&name_len, &col_type,
					&col_prec, &col_scale, &col_null );
			/* FIXME: test rc? */

			ber_str2bv_x( (char *)colname, 0, 1,
					&row->col_names[ i ], ctx );
#ifdef BACKSQL_TRACE
			Debug( LDAP_DEBUG_TRACE, "backsql_BindRowAsStrings: "
				"col_name=%s, col_prec[%d]=%d\n",
				colname, (int)(i + 1), (int)col_prec );
#endif /* BACKSQL_TRACE */
			if ( col_type != SQL_CHAR && col_type != SQL_VARCHAR )
			{
				col_prec = MAX_ATTR_LEN;
			}

			row->cols[ i ] = (char *)ber_memcalloc_x( col_prec + 1,
					sizeof( char ), ctx );
			row->col_prec[ i ] = col_prec;
			row->col_type[ i ] = col_type;

			/*
			 * ITS#3386, ITS#3113 - 20070308
			 * Note: there are many differences between various DPMS and ODBC
			 * Systems; some support SQL_C_BLOB, SQL_C_BLOB_LOCATOR.  YMMV:
			 * This has only been tested on Linux/MySQL/UnixODBC
			 * For BINARY-type Fields (BLOB, etc), read the data as BINARY
			 */
			if ( BACKSQL_IS_BINARY( col_type ) ) {
#ifdef BACKSQL_TRACE
				Debug( LDAP_DEBUG_TRACE, "backsql_BindRowAsStrings: "
					"col_name=%s, col_type[%d]=%d: reading binary data\n",
					colname, (int)(i + 1), (int)col_type);
#endif /* BACKSQL_TRACE */
				TargetType = SQL_C_BINARY;

			} else {
				/* Otherwise read it as Character data */
#ifdef BACKSQL_TRACE
				Debug( LDAP_DEBUG_TRACE, "backsql_BindRowAsStrings: "
					"col_name=%s, col_type[%d]=%d: reading character data\n",
					colname, (int)(i + 1), (int)col_type);
#endif /* BACKSQL_TRACE */
				TargetType = SQL_C_CHAR;
			}

			rc = SQLBindCol( sth, (SQLUSMALLINT)(i + 1),
				 TargetType,
				 (SQLPOINTER)row->cols[ i ],
				 col_prec + 1,
				 &row->value_len[ i ] );

			/* FIXME: test rc? */
		}

		BER_BVZERO( &row->col_names[ i ] );
		row->cols[ i ] = NULL;
	}

#ifdef BACKSQL_TRACE
	Debug( LDAP_DEBUG_TRACE, "<== backsql_BindRowAsStrings()\n", 0, 0, 0 );
#endif /* BACKSQL_TRACE */

	return rc;
}

RETCODE
backsql_BindRowAsStrings( SQLHSTMT sth, BACKSQL_ROW_NTS *row )
{
	return backsql_BindRowAsStrings_x( sth, row, NULL );
}

RETCODE
backsql_FreeRow_x( BACKSQL_ROW_NTS *row, void *ctx )
{
	if ( row->cols == NULL ) {
		return SQL_ERROR;
	}

	ber_bvarray_free_x( row->col_names, ctx );
	ber_memfree_x( row->col_prec, ctx );
	ber_memfree_x( row->col_type, ctx );
	ber_memvfree_x( (void **)row->cols, ctx );
	ber_memfree_x( row->value_len, ctx );

	return SQL_SUCCESS;
}


RETCODE
backsql_FreeRow( BACKSQL_ROW_NTS *row )
{
	return backsql_FreeRow_x( row, NULL );
}

static void
backsql_close_db_handle( SQLHDBC dbh )
{
	if ( dbh == SQL_NULL_HDBC ) {
		return;
	}

	Debug( LDAP_DEBUG_TRACE, "==>backsql_close_db_handle(%p)\n",
		(void *)dbh, 0, 0 );

	/*
	 * Default transact is SQL_ROLLBACK; commit is required only
	 * by write operations, and it is explicitly performed after
	 * each atomic operation succeeds.
	 */

	/* TimesTen */
	SQLTransact( SQL_NULL_HENV, dbh, SQL_ROLLBACK );
	SQLDisconnect( dbh );
	SQLFreeConnect( dbh );

	Debug( LDAP_DEBUG_TRACE, "<==backsql_close_db_handle(%p)\n",
		(void *)dbh, 0, 0 );
}

int
backsql_conn_destroy(
	backsql_info	*bi )
{
	return 0;
}

int
backsql_init_db_env( backsql_info *bi )
{
	RETCODE		rc;
	int		ret = SQL_SUCCESS;
	
	Debug( LDAP_DEBUG_TRACE, "==>backsql_init_db_env()\n", 0, 0, 0 );

	rc = SQLAllocEnv( &bi->sql_db_env );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "init_db_env: SQLAllocEnv failed:\n",
				0, 0, 0 );
		backsql_PrintErrors( SQL_NULL_HENV, SQL_NULL_HDBC,
				SQL_NULL_HENV, rc );
		ret = SQL_ERROR;
	}

	Debug( LDAP_DEBUG_TRACE, "<==backsql_init_db_env()=%d\n", ret, 0, 0 );

	return ret;
}

int
backsql_free_db_env( backsql_info *bi )
{
	Debug( LDAP_DEBUG_TRACE, "==>backsql_free_db_env()\n", 0, 0, 0 );

	(void)SQLFreeEnv( bi->sql_db_env );
	bi->sql_db_env = SQL_NULL_HENV;

	/*
	 * stop, if frontend waits for all threads to shutdown 
	 * before calling this -- then what are we going to delete?? 
	 * everything is already deleted...
	 */
	Debug( LDAP_DEBUG_TRACE, "<==backsql_free_db_env()\n", 0, 0, 0 );

	return SQL_SUCCESS;
}

static int
backsql_open_db_handle(
	backsql_info	*bi,
	SQLHDBC		*dbhp )
{
	/* TimesTen */
	char			DBMSName[ 32 ];
	int			rc;

	assert( dbhp != NULL );
	*dbhp = SQL_NULL_HDBC;
 
	Debug( LDAP_DEBUG_TRACE, "==>backsql_open_db_handle()\n",
		0, 0, 0 );

	rc = SQLAllocConnect( bi->sql_db_env, dbhp );
	if ( !BACKSQL_SUCCESS( rc ) ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_open_db_handle(): "
			"SQLAllocConnect() failed:\n",
			0, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, SQL_NULL_HDBC,
			SQL_NULL_HENV, rc );
		return LDAP_UNAVAILABLE;
	}

	rc = SQLConnect( *dbhp,
		(SQLCHAR*)bi->sql_dbname, SQL_NTS,
		(SQLCHAR*)bi->sql_dbuser, SQL_NTS,
		(SQLCHAR*)bi->sql_dbpasswd, SQL_NTS );
	if ( rc != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_open_db_handle(): "
			"SQLConnect() to database \"%s\" %s.\n",
			bi->sql_dbname,
			rc == SQL_SUCCESS_WITH_INFO ?
				"succeeded with info" : "failed",
			0 );
		backsql_PrintErrors( bi->sql_db_env, *dbhp, SQL_NULL_HENV, rc );
		if ( rc != SQL_SUCCESS_WITH_INFO ) {
			SQLFreeConnect( *dbhp );
			return LDAP_UNAVAILABLE;
		}
	}

	/* 
	 * TimesTen : Turn off autocommit.  We must explicitly
	 * commit any transactions. 
	 */
	SQLSetConnectOption( *dbhp, SQL_AUTOCOMMIT,
		BACKSQL_AUTOCOMMIT_ON( bi ) ?  SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF );

	/* 
	 * See if this connection is to TimesTen.  If it is,
	 * remember that fact for later use.
	 */
	/* Assume until proven otherwise */
	bi->sql_flags &= ~BSQLF_USE_REVERSE_DN;
	DBMSName[ 0 ] = '\0';
	rc = SQLGetInfo( *dbhp, SQL_DBMS_NAME, (PTR)&DBMSName,
			sizeof( DBMSName ), NULL );
	if ( rc == SQL_SUCCESS ) {
		if ( strcmp( DBMSName, "TimesTen" ) == 0 ||
			strcmp( DBMSName, "Front-Tier" ) == 0 )
		{
			Debug( LDAP_DEBUG_TRACE, "backsql_open_db_handle(): "
				"TimesTen database!\n",
				0, 0, 0 );
			bi->sql_flags |= BSQLF_USE_REVERSE_DN;
		}

	} else {
		Debug( LDAP_DEBUG_TRACE, "backsql_open_db_handle(): "
			"SQLGetInfo() failed.\n",
			0, 0, 0 );
		backsql_PrintErrors( bi->sql_db_env, *dbhp, SQL_NULL_HENV, rc );
		SQLDisconnect( *dbhp );
		SQLFreeConnect( *dbhp );
		return LDAP_UNAVAILABLE;
	}
	/* end TimesTen */

	Debug( LDAP_DEBUG_TRACE, "<==backsql_open_db_handle()\n",
		0, 0, 0 );

	return LDAP_SUCCESS;
}

static void	*backsql_db_conn_dummy;

static void
backsql_db_conn_keyfree(
	void		*key,
	void		*data )
{
	(void)backsql_close_db_handle( (SQLHDBC)data );
}

int
backsql_free_db_conn( Operation *op, SQLHDBC dbh )
{
	Debug( LDAP_DEBUG_TRACE, "==>backsql_free_db_conn()\n", 0, 0, 0 );

	(void)backsql_close_db_handle( dbh );
	ldap_pvt_thread_pool_setkey( op->o_threadctx,
		&backsql_db_conn_dummy, (void *)SQL_NULL_HDBC,
		backsql_db_conn_keyfree, NULL, NULL );

	Debug( LDAP_DEBUG_TRACE, "<==backsql_free_db_conn()\n", 0, 0, 0 );

	return LDAP_SUCCESS;
}

int
backsql_get_db_conn( Operation *op, SQLHDBC *dbhp )
{
	backsql_info	*bi = (backsql_info *)op->o_bd->be_private;
	int		rc = LDAP_SUCCESS;
	SQLHDBC		dbh = SQL_NULL_HDBC;

	Debug( LDAP_DEBUG_TRACE, "==>backsql_get_db_conn()\n", 0, 0, 0 );

	assert( dbhp != NULL );
	*dbhp = SQL_NULL_HDBC;

	if ( op->o_threadctx ) {
		void		*data = NULL;

		ldap_pvt_thread_pool_getkey( op->o_threadctx,
				&backsql_db_conn_dummy, &data, NULL );
		dbh = (SQLHDBC)data;

	} else {
		dbh = bi->sql_dbh;
	}

	if ( dbh == SQL_NULL_HDBC ) {
		rc = backsql_open_db_handle( bi, &dbh );
		if ( rc != LDAP_SUCCESS ) {
			return rc;
		}

		if ( op->o_threadctx ) {
			void		*data = (void *)dbh;

			ldap_pvt_thread_pool_setkey( op->o_threadctx,
					&backsql_db_conn_dummy, data,
					backsql_db_conn_keyfree, NULL, NULL );

		} else {
			bi->sql_dbh = dbh;
		}
	}

	*dbhp = dbh;

	Debug( LDAP_DEBUG_TRACE, "<==backsql_get_db_conn()\n", 0, 0, 0 );

	return LDAP_SUCCESS;
}

