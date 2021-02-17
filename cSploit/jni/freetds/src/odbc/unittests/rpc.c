/* 
 * Purpose: Test remote procedure calls
 * Functions:  
 */

#include "common.h"
#include <assert.h>

static char software_version[] = "$Id: rpc.c,v 1.15 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static const char procedure_sql[] = 
		"CREATE PROCEDURE %s \n"
			"  @null_input varchar(30) OUTPUT \n"
			", @first_type varchar(30) OUTPUT \n"
			", @nullout int OUTPUT\n"
			", @nrows int OUTPUT \n"
			", @c varchar(20)\n"
		"AS \n"
		"BEGIN \n"
			"select @null_input = max(convert(varchar(30), name)) from systypes \n"
			"select @first_type = min(convert(varchar(30), name)) from systypes \n"
			/* #1 empty result set: */
			"select name from sysobjects where 0=1\n"
			/* #2 3 rows: */
			"select distinct convert(varchar(30), name) as 'type'  from systypes \n"
				"where name in ('int', 'char', 'text') \n"
			"select @nrows = @@rowcount \n"
			/* #3 many rows: */
			"select distinct convert(varchar(30), name) as name  from sysobjects where type = 'S' \n"
			"return 42 \n"
		"END \n";


static void
init_proc(const char *name)
{
	static char cmd[4096];

	if (name[0] != '#') {
		printf("Dropping procedure %s\n", name);
		sprintf(cmd, "if exists (select 1 from sysobjects where name = '%s' and type = 'P') "
				"DROP PROCEDURE %s", name, name);
		CHKExecDirect(T(cmd), SQL_NTS, "SI");
	}

	printf("Creating procedure %s\n", name);
	sprintf(cmd, procedure_sql, name);

	/* create procedure. Fails if wrong permission or not MSSQL */
	CHKExecDirect(T(cmd), SQL_NTS, "SI");
}

static void
Test(const char *name)
{
	int iresults=0, data_errors=0;
	int ipar=0;
	HSTMT odbc_stmt = SQL_NULL_HSTMT;
	char call_cmd[128];
	struct Argument { 
                SQLSMALLINT       InputOutputType;  /* fParamType */
                SQLSMALLINT       ValueType;        /* fCType */
                SQLSMALLINT       ParameterType;    /* fSqlType */
                SQLUINTEGER       ColumnSize;       /* cbColDef */
                SQLSMALLINT       DecimalDigits;    /* ibScale */
                SQLPOINTER        ParameterValuePtr;/* rgbValue */
                SQLINTEGER        BufferLength;     /* cbValueMax */
                SQLLEN            ind;              /* pcbValue */
	};
	struct Argument args[] = {
		/* InputOutputType 	  ValueType   ParamType    ColumnSize 
								    | DecimalDigits 
								    |  | ParameterValuePtr 
								    |  |  |  BufferLength 
								    |  |  |   |	 ind */
		{ SQL_PARAM_OUTPUT,       SQL_C_LONG, SQL_INTEGER,  0, 0, 0,  4, 3 }, /* return status */
		{ SQL_PARAM_INPUT_OUTPUT, SQL_C_CHAR, SQL_VARCHAR, 30, 0, 0, 30, SQL_NULL_DATA }, 
										      /* @null_input varchar(30) OUTPUT */
		{ SQL_PARAM_INPUT_OUTPUT, SQL_C_CHAR, SQL_VARCHAR, 30, 0, 0, 30, 3 }, /* @first_type varchar(30) OUTPUT */
		{ SQL_PARAM_INPUT_OUTPUT, SQL_C_LONG, SQL_INTEGER,  0, 0, 0,  4, 4 }, /* @nullout int OUTPUT\ */
		{ SQL_PARAM_INPUT_OUTPUT, SQL_C_LONG, SQL_INTEGER,  0, 0, 0,  4, 4 }, /* @nrows int OUTPUT */
		{ SQL_PARAM_INPUT,        SQL_C_CHAR, SQL_VARCHAR, 20, 0, 0, 20, 3 }  /* @c varchar(20) */
	};


	printf("executing SQLAllocStmt\n");
	CHKAllocStmt(&odbc_stmt, "S");

	for( ipar=0; ipar < sizeof(args)/sizeof(args[0]); ipar++ ) {
		printf("executing SQLBindParameter for parameter %d\n", 1+ipar);
		if( args[ipar].BufferLength > 0 ) {
			args[ipar].ParameterValuePtr = (SQLPOINTER) ODBC_GET(args[ipar].BufferLength);
			assert(args[ipar].ParameterValuePtr != NULL);
			memset(args[ipar].ParameterValuePtr, 0, args[ipar].BufferLength);
			memset(args[ipar].ParameterValuePtr, 'a', args[ipar].BufferLength - 1);
		}
		CHKBindParameter	( 1+ipar
					, args[ipar].InputOutputType
					, args[ipar].ValueType
					, args[ipar].ParameterType
					, args[ipar].ColumnSize
					, args[ipar].DecimalDigits
					, args[ipar].ParameterValuePtr
					, args[ipar].BufferLength
					, &args[ipar].ind
					, "S"
					);
	}

	sprintf(call_cmd, "{?=call %s(?,?,?,?,?)}", name );
	printf("executing SQLPrepare: %s\n", call_cmd);
	CHKPrepare(T(call_cmd), SQL_NTS, "S");

	printf("executing SQLExecute\n");
	CHKExecute("SI");

	do {
		static const char dashes[] = "------------------------------";
		int nrows;
		SQLSMALLINT  icol, ncols;
		SQLTCHAR     name[256] = {0};
		SQLSMALLINT  namelen;
		SQLSMALLINT  type;
		SQLSMALLINT  scale;
		SQLSMALLINT  nullable;

		printf("executing SQLNumResultCols for result set %d\n", ++iresults);
		CHKNumResultCols(&ncols, "S");

		printf("executing SQLDescribeCol for %d column%c\n", ncols, (ncols == 1? ' ' : 's'));
		printf("%-5.5s %-15.15s %5.5s %5.5s %5.5s %8.8s\n", "col", "name", "type", "size", "scale", "nullable"); 
		printf("%-5.5s %-15.15s %5.5s %5.5s %5.5s %8.8s\n", dashes, dashes, dashes, dashes, dashes, dashes); 
		
		for (icol=ncols; icol > 0; icol--) {
			SQLULEN size;
			CHKDescribeCol(icol, name, ODBC_VECTOR_SIZE(name),
				       &namelen, &type, &size, &scale, &nullable, "S");
			printf("%-5d %-15s %5d %5ld %5d %8c\n", icol, C(name), type, (long int)size, scale, (nullable? 'Y' : 'N')); 
		}

		printf("executing SQLFetch...\n");
		printf("\t%-30s\n\t%s\n", C(name), dashes);
		for (nrows=0; CHKFetch("SNo") == SQL_SUCCESS; nrows++) {
			const SQLINTEGER icol = 1;
			char buf[60];
			SQLLEN len;
			CHKGetData( icol
					, SQL_C_CHAR	/* fCType */
					, buf		/* rgbValue */
					, sizeof(buf)	/* cbValueMax */
	                		, &len		/* pcbValue */	
					, "SI"
					);
			printf("\t%-30s\t(%2d bytes)\n", buf, (int) len);
		}
		printf("done.\n");

		switch (iresults) {
		case 1:
			printf("0 rows expected, %d found\n", nrows);
			data_errors += (nrows == 0)? 0 : 1;
			break;;
		case 2:
			printf("3 rows expected, %d found\n", nrows);
			data_errors += (nrows == 3)? 0 : 1;
			break;;
		case 3:
			printf("at least 15 rows expected, %d found\n", nrows);
			data_errors += (nrows > 15)? 0 : 1;
			break;;
		}

		printf("executing SQLMoreResults...\n");
	} while (CHKMoreResults("SNo") == SQL_SUCCESS);
	printf("done.\n");

	for( ipar=0; ipar < sizeof(args)/sizeof(args[0]); ipar++ ) {
		if (args[ipar].InputOutputType == SQL_PARAM_INPUT)
			continue;
		printf("bound data for parameter %d is %ld bytes: ", 1+ipar, (long int)args[ipar].ind);
		switch( args[ipar].ValueType ) {
		case SQL_C_LONG:
			printf("%d.\n", (int)(*(SQLINTEGER *)args[ipar].ParameterValuePtr));
			break;
		case SQL_C_CHAR:
			printf("'%s'.\n", (char*)args[ipar].ParameterValuePtr);
			break;
		default:
			printf("type unsupported in this test\n");
			assert(0);
			break;
		}
	}

	printf("executing SQLFreeStmt\n");
	CHKFreeStmt(SQL_DROP, "S");
	odbc_stmt = SQL_NULL_HSTMT;

	ODBC_FREE();

	if (data_errors) {
		fprintf(stderr, "%d errors found in expected row count\n", data_errors);
		exit(1);
	}
}

int
main(int argc, char *argv[])
{
	const char proc_name[] = "freetds_odbc_rpc_test";
	char drop_proc[256] = "DROP PROCEDURE ";
	
	strcat(drop_proc, proc_name);
	
	printf("connecting\n");
	odbc_connect();
	
	init_proc(proc_name);

	printf("running test\n");
	Test(proc_name);
	
	printf("dropping procedure\n");
	odbc_command(drop_proc);

	odbc_disconnect();

	printf("Done.\n");
	ODBC_FREE();
	return 0;
}

