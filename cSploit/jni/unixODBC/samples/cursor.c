#include <stdio.h>
#include <sql.h>
#include <sqlext.h>

#define ROWS 20 
#define STATUS_LEN 6
#define OPENDATE_LEN 11
#define DONE -1

int res[][ 3 ] = 
{
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_PRIOR, 0, 0 },
    { SQL_FETCH_FIRST, 0, 0 },
    { SQL_FETCH_ABSOLUTE, -100, 0 },
    { SQL_FETCH_ABSOLUTE, -400, 0 },
    { SQL_FETCH_ABSOLUTE, 200, 0 },
    { SQL_FETCH_ABSOLUTE, 350, 0 },
    { SQL_FETCH_LAST, 0, 0 },
    { SQL_FETCH_NEXT, 0, 0 },
    { SQL_FETCH_LAST, 0, 0 },
    { SQL_FETCH_FIRST, 0, 0 },
    { SQL_FETCH_RELATIVE, 20, 0 },
    { SQL_FETCH_RELATIVE, 50, 0 },
    { SQL_FETCH_RELATIVE, 101, 0 },
    { SQL_FETCH_RELATIVE, -5, 0 },
    { SQL_FETCH_RELATIVE, 60, 0 },
    { SQL_FETCH_RELATIVE, -60, 0 },
    { SQL_FETCH_RELATIVE, 0, 0 },
    { SQL_FETCH_RELATIVE, 0, 0 },
    { SQL_FETCH_NEXT, 0, -1 },
};

int PromptScroll( SQLUINTEGER *ort, SQLUINTEGER *offset )
{
    static int count = 0;
    int ret;

    *ort = res[ count ][ 0 ];
    *offset = res[ count ][ 1 ];
    ret = res[ count ][ 2 ];

    count ++;

    return ret;
}

int DumpODBCLog( SQLHENV hEnv, SQLHDBC hDbc, SQLHSTMT hStmt )
{
	SQLCHAR		szError[501];
	SQLCHAR		szSqlState[10];
    SQLINTEGER  nNativeError;
    SQLSMALLINT	nErrorMsg;

	if ( hStmt )
	{
		while ( SQLError( hEnv, hDbc, hStmt, szSqlState, &nNativeError, szError, 500, &nErrorMsg ) == SQL_SUCCESS )
		{
			printf( "%s\n", szError );
		}
	}

	if ( hDbc )
	{
		while ( SQLError( hEnv, hDbc, 0, szSqlState, &nNativeError, szError, 500, &nErrorMsg ) == SQL_SUCCESS )
		{
			printf( "%s\n", szError );
		}
	}

	if ( hEnv )
	{
		while ( SQLError( hEnv, 0, 0, szSqlState, &nNativeError, szError, 500, &nErrorMsg ) == SQL_SUCCESS )
		{
			printf( "%s\n", szError );
		}
	}

	return 1;
}

static int Display( SQLUSMALLINT *rsa, SQLUINTEGER crow,
        SQLSMALLINT *sOrderID, SQLINTEGER *cbOrderID,
        SQLCHAR szOrderDate[][OPENDATE_LEN], SQLINTEGER *cbOrderDate,
        SQLCHAR szStatus[][STATUS_LEN], SQLINTEGER *cbStatus )
{
    int i;

    printf( "crow = %d\n", crow );

    for ( i = 0; i < crow; i ++ )
    {
        printf( "%d %d |%d:%d|%s:%d|%s:%d\n", i,
                rsa[ i ], sOrderID[ i ], cbOrderID[ i ],
                szOrderDate[ i ], cbOrderDate[ i ],
                szStatus[ i ], cbStatus[ i ] );
    }
}

void create_file( SQLHANDLE hstmt )
{
    SQLRETURN ret;
    int i;
    
    ret = SQLExecDirect( hstmt, "drop table ctest", SQL_NTS );
    ret = SQLExecDirect( hstmt, "create table ctest ( id integer, dt character( 10 ), status character( 5 ), other character varying( 40 ))", SQL_NTS );

    for ( i = 1; i < 1000; i ++ )
    {
        char sql[ 256 ];
        sprintf( sql, "insert into ctest values( %d, '%10d', '%05d', 'other line %d' )", i, i, i, i );
        ret = SQLExecDirect( hstmt, sql, SQL_NTS );
        printf( "%s - %d\n", sql, ret );
    }
}

void cursor_test()
{
	SQLHENV			henv;
	SQLHDBC			hdbc;
	SQLHSTMT		hstmt1, hstmt2;
	SQLRETURN		retcode;
	SQLCHAR			szStatus[ROWS][STATUS_LEN], szOpenDate[ROWS][OPENDATE_LEN];
	SQLCHAR 		szNewStatus[STATUS_LEN], szNewOpenDate[OPENDATE_LEN];
	SQLSMALLINT		sOrderID[ROWS], sNewOrderID[ROWS];
	SQLINTEGER		cbStatus[ROWS], cbOrderID[ROWS], cbOpenDate[ROWS];
	SQLUINTEGER		FetchOrientation, crow, FetchOffset, irowUpdt;
	SQLUSMALLINT	RowStatusArray[ROWS];

	SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv );
	SQLSetEnvAttr( henv, SQL_ATTR_ODBC_VERSION, SQL_OV_ODBC3, 0 );
	SQLAllocHandle( SQL_HANDLE_DBC, henv, &hdbc );

	/* Specify the ODBC Cursor Library is always used then connect. */

	SQLSetConnectAttr( hdbc, SQL_ATTR_ODBC_CURSORS, SQL_CUR_USE_ODBC, 0 );
	retcode = SQLConnect( hdbc, "postgres", SQL_NTS,
					  "", SQL_NTS,
					  "", SQL_NTS );

    DumpODBCLog( NULL, hdbc, NULL );

	if ( retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO )
	{
        /* Allocate a statement handle for the result set and a statement */
        /* handle for a positioned update statement                       */

        SQLAllocHandle( SQL_HANDLE_STMT, hdbc, &hstmt1 );
        SQLAllocHandle( SQL_HANDLE_STMT, hdbc, &hstmt2 );

        /*
         * create the data
         */

        create_file( hstmt1 );

        /* Specify an updateable statis cursor with 20 rows of data. Set */
        /* the cursor name, execute the SELECT statement, and bind the   */
        /* rowset buffer to result set columns in column-wise fashion    */
        SQLSetStmtAttr( hstmt1, SQL_ATTR_CONCURRENCY, SQL_CONCUR_VALUES, 0 );
        SQLSetStmtAttr( hstmt1, SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_STATIC, 0 );
        SQLSetStmtAttr( hstmt1, SQL_ATTR_ROW_ARRAY_SIZE, ROWS, 20 );
        SQLSetStmtAttr( hstmt1, SQL_ATTR_ROW_STATUS_PTR, RowStatusArray, 0 );
        SQLSetStmtAttr( hstmt1, SQL_ATTR_ROWS_FETCHED_PTR, &crow, 0 );
        SQLSetCursorName( hstmt1, "ORDERCURSOR", SQL_NTS );
        SQLPrepare( hstmt1, 
                    "select id, dt, status from ctest",
                    SQL_NTS );

        {
            char cname[ 30 ];
            retcode = SQLDescribeCol( hstmt1, 1, cname, sizeof( cname ),
                NULL, NULL, NULL, NULL, NULL );
            printf( "ret = %d %s\n", retcode, cname );
            DumpODBCLog( NULL, NULL, hstmt1 );
        }

        SQLExecute( hstmt1 );
        SQLBindCol( hstmt1, 1, SQL_C_SSHORT, sOrderID, 0, cbOrderID );
        SQLBindCol( hstmt1, 2, SQL_C_CHAR, szOpenDate, OPENDATE_LEN, cbOpenDate );
        SQLBindCol( hstmt1, 3, SQL_C_CHAR, szStatus, STATUS_LEN, cbStatus );

        FetchOrientation = SQL_FETCH_FIRST;
        FetchOffset = 0;

        do
        {
            int ret;
            int count;

            printf( "fetch %d %d\n", FetchOrientation, FetchOffset );
            
            ret = SQLFetchScroll( hstmt1, 
                    FetchOrientation, 
                    FetchOffset );

            SQLRowCount( hstmt1, &count );
            printf( "ret = %d count = %d\n", ret, count );

            Display( RowStatusArray, crow, sOrderID, cbOrderID,
                    szOpenDate, cbOpenDate, szStatus, cbStatus );

            if ( SQL_SUCCEEDED( ret ))
            {
                char txt[ 50 ];
                SQLINTEGER len;

                ret = SQLSetPos( hstmt1, 5, SQL_POSITION,
                        SQL_LOCK_NO_CHANGE );

                ret = SQLGetData( hstmt1, 2, SQL_C_CHAR, txt, sizeof( txt ),
                        &len );

                printf( "ret = %d %s:%d\n", ret, txt, len );
            }

        } while( PromptScroll( &FetchOrientation, &FetchOffset ) != DONE ); 

        SQLCloseCursor( hstmt1 );
        SQLFreeStmt( hstmt1, SQL_DROP );
        SQLFreeStmt( hstmt2, SQL_DROP );
        SQLDisconnect( hdbc );
        SQLFreeHandle( SQL_HANDLE_DBC, hdbc );
	}
    SQLFreeHandle( SQL_HANDLE_ENV, henv );
}

int main()
{
	cursor_test();
}

