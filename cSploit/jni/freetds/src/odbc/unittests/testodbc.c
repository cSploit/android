/*
 * Code to test ODBC implementation.
 *  - David Fraser, Abelon Systems 2003.
 */

/* 
 * TODO
 * remove Northwind dependency
 */

#include "common.h"

static char software_version[] = "$Id: testodbc.c,v 1.16 2011-07-12 10:16:59 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#ifdef DEBUG
# define AB_FUNCT(x)  do { printf x; printf("\n"); } while(0)
# define AB_PRINT(x)  do { printf x; printf("\n"); } while(0)
#else
# define AB_FUNCT(x)
# define AB_PRINT(x)
#endif
#define AB_ERROR(x)   do { printf("ERROR: "); printf x; printf("\n"); } while(0)

#undef TRUE
#undef FALSE
enum
{ FALSE, TRUE };
typedef int DbTestFn(void);

static int RunTests(void);

typedef struct
{
	DbTestFn *testFn;
	const char *description;
} DbTestEntry;

/*
 * Test that makes a parameterized ODBC query using SQLPrepare and SQLExecute
 */
static int
TestRawODBCPreparedQuery(void)
{
	SQLTCHAR *queryString;
	SQLLEN lenOrInd = 0;
	SQLSMALLINT supplierId = 4;
	int count;

	AB_FUNCT(("TestRawODBCPreparedQuery (in)"));

	/* INIT */

	odbc_connect();

	/* MAKE QUERY */

	odbc_command("CREATE TABLE #Products ("
		"ProductID int NOT NULL ,"
		"ProductName varchar (40) ,"
		"SupplierID int NULL ,"
		"CategoryID int NULL ,"
		"QuantityPerUnit varchar (20)  ,"
		"UnitPrice money NULL ,"
		"UnitsInStock smallint NULL ,"
		"UnitsOnOrder smallint NULL ,"
		"ReorderLevel smallint NULL ,"
		"Discontinued bit NOT NULL "
		") "
		"INSERT INTO #Products(ProductID,ProductName,SupplierID,CategoryID,QuantityPerUnit,UnitPrice,UnitsInStock,UnitsOnOrder,ReorderLevel,Discontinued) VALUES(9,'Mishi Kobe Niku',4,6,'18 - 500 g pkgs.',97.00,29,0,0,1) "
		"INSERT INTO #Products(ProductID,ProductName,SupplierID,CategoryID,QuantityPerUnit,UnitPrice,UnitsInStock,UnitsOnOrder,ReorderLevel,Discontinued) VALUES(10,'Ikura',4,8,'12 - 200 ml jars',31.00,31,0,0,0) "
		"INSERT INTO #Products(ProductID,ProductName,SupplierID,CategoryID,QuantityPerUnit,UnitPrice,UnitsInStock,UnitsOnOrder,ReorderLevel,Discontinued) VALUES(74,'Longlife Tofu',4,7,'5 kg pkg.',10.00,4,20,5,0) "
		"INSERT INTO #Products(ProductID,ProductName,SupplierID,CategoryID,QuantityPerUnit,UnitPrice,UnitsInStock,UnitsOnOrder,ReorderLevel,Discontinued) VALUES(11,'Queso Cabrales',5,4,'1 kg pkg.',21.00,22,30,30,0) "
		"INSERT INTO #Products(ProductID,ProductName,SupplierID,CategoryID,QuantityPerUnit,UnitPrice,UnitsInStock,UnitsOnOrder,ReorderLevel,Discontinued) VALUES(12,'Queso Manchego La Pastora',5,4,'10 - 500 g pkgs.',38.00,86,0,0,0)");
	while (SQLMoreResults(odbc_stmt) == SQL_SUCCESS);

	queryString = T("SELECT * FROM #Products WHERE SupplierID = ?");

	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &supplierId, 0, &lenOrInd, "S");

	CHKPrepare(queryString, SQL_NTS, "S");

	CHKExecute("S");

	count = 0;

	while (SQLFetch(odbc_stmt) == SQL_SUCCESS) {
		count++;
	}
	AB_PRINT(("Got %d rows", count));

	if (count != 3) {
		/*
		 * OK - so 3 is a magic number - it's the number of rows matching
		 * this query from the MS sample Northwind database and is a constant.
		 */
		AB_ERROR(("Expected %d rows - but got %d rows", 3, count));
		AB_FUNCT(("TestRawODBCPreparedQuery (out): error"));
		return FALSE;
	}

	/* CLOSEDOWN */

	odbc_disconnect();

	AB_FUNCT(("TestRawODBCPreparedQuery (out): ok"));
	return TRUE;
}

/*
 * Test that makes a parameterized ODBC query using SQLExecDirect.
 */
static int
TestRawODBCDirectQuery(void)
{
	SQLLEN lenOrInd = 0;
	SQLSMALLINT supplierId = 1;
	int count;

	AB_FUNCT(("TestRawODBCDirectQuery (in)"));

	/* INIT */

	odbc_connect();

	/* MAKE QUERY */

	odbc_command("CREATE TABLE #Products ("
		"ProductID int NOT NULL ,"
		"ProductName varchar (40) ,"
		"SupplierID int NULL ,"
		"CategoryID int NULL ,"
		"QuantityPerUnit varchar (20)  ,"
		"UnitPrice money NULL ,"
		"UnitsInStock smallint NULL ,"
		"UnitsOnOrder smallint NULL ,"
		"ReorderLevel smallint NULL ,"
		"Discontinued bit NOT NULL "
		") "
		"INSERT INTO #Products(ProductID,ProductName,SupplierID,CategoryID,QuantityPerUnit,UnitPrice,UnitsInStock,UnitsOnOrder,ReorderLevel,Discontinued) VALUES(1,'Chai',1,1,'10 boxes x 20 bags',18.00,39,0,10,0) "
		"INSERT INTO #Products(ProductID,ProductName,SupplierID,CategoryID,QuantityPerUnit,UnitPrice,UnitsInStock,UnitsOnOrder,ReorderLevel,Discontinued) VALUES(2,'Chang',1,1,'24 - 12 oz bottles',19.00,17,40,25,0) "
		"INSERT INTO #Products(ProductID,ProductName,SupplierID,CategoryID,QuantityPerUnit,UnitPrice,UnitsInStock,UnitsOnOrder,ReorderLevel,Discontinued) VALUES(3,'Aniseed Syrup',1,2,'12 - 550 ml bottles',10.00,13,70,25,0) "
		"INSERT INTO #Products(ProductID,ProductName,SupplierID,CategoryID,QuantityPerUnit,UnitPrice,UnitsInStock,UnitsOnOrder,ReorderLevel,Discontinued) VALUES(4,'Chef Anton''s Cajun Seasoning',2,2,'48 - 6 oz jars',22.00,53,0,0,0) "
		"INSERT INTO #Products(ProductID,ProductName,SupplierID,CategoryID,QuantityPerUnit,UnitPrice,UnitsInStock,UnitsOnOrder,ReorderLevel,Discontinued) VALUES(5,'Chef Anton''s Gumbo Mix',2,2,'36 boxes',21.35,0,0,0,1) ");
	while (SQLMoreResults(odbc_stmt) == SQL_SUCCESS);

	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &supplierId, 0, &lenOrInd, "S");

	CHKExecDirect(T("SELECT * FROM #Products WHERE SupplierID = ?"), SQL_NTS, "S");

	count = 0;

	while (SQLFetch(odbc_stmt) == SQL_SUCCESS) {
		count++;
	}
	AB_PRINT(("Got %d rows", count));

	if (count != 3) {
		/*
		 * OK - so 3 is a magic number - it's the number of rows matching
		 * this query from the MS sample Northwind database and is a constant.
		 */
		AB_ERROR(("Expected %d rows - but got %d rows", 3, count));
		AB_FUNCT(("TestRawODBCDirectQuery (out): error"));
		return FALSE;
	}

	/* CLOSEDOWN */

	odbc_disconnect();

	AB_FUNCT(("TestRawODBCDirectQuery (out): ok"));
	return TRUE;
}

/*
 * Test that show what works and what doesn't for the poorly
 * documented GUID.
 */
static int
TestRawODBCGuid(void)
{
	SQLRETURN status;

	const char *queryString;
	SQLLEN lenOrInd;
	SQLSMALLINT age;
	char guid[40];
	SQLCHAR name[20];

	SQLGUID sqlguid;
	int count = 0;

	AB_FUNCT(("TestRawODBCGuid (in)"));

	odbc_connect();
	
	if (!odbc_db_is_microsoft()) {
		odbc_disconnect();
		return TRUE;
	}

	AB_PRINT(("Creating #pet table"));

	queryString = "CREATE TABLE #pet (name VARCHAR(20), owner VARCHAR(20), "
	       "species VARCHAR(20), sex CHAR(1), age INTEGER, " "guid UNIQUEIDENTIFIER DEFAULT NEWID() ); ";
	CHKExecDirect(T(queryString), SQL_NTS, "SNo");

	odbc_command_with_result(odbc_stmt, "DROP PROC GetGUIDRows");

	AB_PRINT(("Creating stored proc GetGUIDRows"));

	queryString = "CREATE PROCEDURE GetGUIDRows (@guidpar uniqueidentifier) AS \
                SELECT name, guid FROM #pet WHERE guid = @guidpar";
	CHKExecDirect(T(queryString), SQL_NTS, "SNo");

	AB_PRINT(("Insert row 1"));

	queryString = "INSERT INTO #pet( name, owner, species, sex, age ) \
                         VALUES ( 'Fang', 'Mike', 'dog', 'm', 12 );";
	CHKExecDirect(T(queryString), SQL_NTS, "S");

	AB_PRINT(("Insert row 2"));

	/*
	 * Ok - new row with explicit GUID, but parameterised age.
	 */
	queryString = "INSERT INTO #pet( name, owner, species, sex, age, guid ) \
                         VALUES ( 'Splash', 'Dan', 'fish', 'm', ?, \
                         '12345678-1234-1234-1234-123456789012' );";

	lenOrInd = 0;
	age = 3;
	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_SSHORT, SQL_INTEGER, 0, 0, &age, 0, &lenOrInd, "S");

	CHKExecDirect(T(queryString), SQL_NTS, "S");
	CHKFreeStmt(SQL_CLOSE, "S");

	AB_PRINT(("Insert row 3"));
	/*
	 * Ok - new row with parameterised GUID.
	 */
	queryString = "INSERT INTO #pet( name, owner, species, sex, age, guid ) \
                         VALUES ( 'Woof', 'Tom', 'cat', 'f', 2, ? );";

	lenOrInd = SQL_NTS;
	strcpy(guid, "87654321-4321-4321-4321-123456789abc");

	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_GUID, 0, 0, guid, 0, &lenOrInd, "S");
	CHKExecDirect(T(queryString), SQL_NTS, "S");

	AB_PRINT(("Insert row 4"));
	/*
	 * Ok - new row with parameterised GUID.
	 */
	queryString = "INSERT INTO #pet( name, owner, species, sex, age, guid ) \
                         VALUES ( 'Spike', 'Diane', 'pig', 'f', 4, ? );";

	lenOrInd = SQL_NTS;
	strcpy(guid, "1234abcd-abcd-abcd-abcd-123456789abc");

	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 36, 0, guid, 0, &lenOrInd, "S");
	CHKExecDirect(T(queryString), SQL_NTS, "S");

	AB_PRINT(("Insert row 5"));
	/*
	 * Ok - new row with parameterised GUID.
	 */
	queryString = "INSERT INTO #pet( name, owner, species, sex, age, guid ) \
                         VALUES ( 'Fluffy', 'Sam', 'dragon', 'm', 16, ? );";

	sqlguid.Data1 = 0xaabbccdd;
	sqlguid.Data2 = 0xeeff;
	sqlguid.Data3 = 0x1122;
	sqlguid.Data4[0] = 0x11;
	sqlguid.Data4[1] = 0x22;
	sqlguid.Data4[2] = 0x33;
	sqlguid.Data4[3] = 0x44;
	sqlguid.Data4[4] = 0x55;
	sqlguid.Data4[5] = 0x66;
	sqlguid.Data4[6] = 0x77;
	sqlguid.Data4[7] = 0x88;

	lenOrInd = 16;
	strcpy(guid, "1234abcd-abcd-abcd-abcd-123456789abc");

	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_GUID, SQL_GUID, 16, 0, &sqlguid, 16, &lenOrInd, "S");
	status = SQLExecDirect(odbc_stmt, T(queryString), SQL_NTS);
	if (status != SQL_SUCCESS) {
		AB_ERROR(("Insert row 5 failed"));
		AB_ERROR(("Sadly this was expected in *nix ODBC. Carry on."));
	}

	/*
	 * Now retrieve rows - especially GUID column values.
	 */
	AB_PRINT(("retrieving name and guid"));
	queryString = "SELECT name, guid FROM #pet";
	CHKExecDirect(T(queryString), SQL_NTS, "S");
	while (SQLFetch(odbc_stmt) == SQL_SUCCESS) {
		count++;
		CHKGetData(1, SQL_CHAR, name, 20, 0, "S");
		CHKGetData(2, SQL_CHAR, guid, 37, 0, "S");

		AB_PRINT(("name: %-10s guid: %s", name, guid));
	}

	/*
	 * Realloc cursor handle - (Windows ODBC considers it an invalid cursor
	 * state if we try SELECT again).
	 */
	odbc_reset_statement();


	/*
	 * Now retrieve rows - especially GUID column values.
	 */

	AB_PRINT(("retrieving name and guid again"));
	queryString = "SELECT name, guid FROM #pet";
	CHKExecDirect(T(queryString), SQL_NTS, "S");
	while (CHKFetch("SNo") == SQL_SUCCESS) {
		count++;
		CHKGetData(1, SQL_CHAR, name, 20, 0, "S");
		CHKGetData(2, SQL_GUID, &sqlguid, 16, 0, "S");

		AB_PRINT(("%-10s %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			  name,
			  (int) (sqlguid.Data1), sqlguid.Data2,
			  sqlguid.Data3, sqlguid.Data4[0], sqlguid.Data4[1],
			  sqlguid.Data4[2], sqlguid.Data4[3], sqlguid.Data4[4],
			  sqlguid.Data4[5], sqlguid.Data4[6], sqlguid.Data4[7]));
	}

	/*
	 * Realloc cursor handle - (Windows ODBC considers it an invalid cursor
	 * state if we try SELECT again).
	 */
	odbc_reset_statement();

	/*
	 * Now retrieve rows via stored procedure passing GUID as param.
	 */
	AB_PRINT(("retrieving name and guid"));

	queryString = "{call GetGUIDRows(?)}";
	lenOrInd = SQL_NTS;
	strcpy(guid, "87654321-4321-4321-4321-123456789abc");

	CHKBindParameter(1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_GUID, 0, 0, guid, 0, &lenOrInd, "S");
	CHKExecDirect(T(queryString), SQL_NTS, "S");
	while (SQLFetch(odbc_stmt) == SQL_SUCCESS) {
		count++;
		CHKGetData(1, SQL_CHAR, name, 20, 0, "S");
		CHKGetData(2, SQL_CHAR, guid, 37, 0, "S");

		AB_PRINT(("%-10s %s", name, guid));
	}

	/*
	 * Realloc cursor handle - (Windows ODBC considers it an invalid cursor
	 * state after a previous SELECT has occurred).
	 */
	odbc_reset_statement();

	/* cleanup */
	odbc_command_with_result(odbc_stmt, "DROP PROC GetGUIDRows");

	/* CLOSEDOWN */

	odbc_disconnect();

	AB_FUNCT(("TestRawODBCGuid (out): ok"));
	return TRUE;
}

/**
 * Array of tests.
 */
static DbTestEntry _dbTests[] = {
	/* 1 */ {TestRawODBCDirectQuery, "Raw ODBC direct query"},
	/* 2 */ {TestRawODBCPreparedQuery, "Raw ODBC prepared query"},
	/* 3 */ {TestRawODBCGuid, "Raw ODBC GUID"},
	/* end */ {0, 0}
};

static DbTestEntry *tests = _dbTests;

/**
 * Code to iterate through all tests to run.
 *
 * \return
 *      TRUE if all tests pass, FALSE if any tests fail.
 */
static int
RunTests(void)
{
	unsigned int i;
	unsigned int passes = 0;
	unsigned int fails = 0;

	i = 0;
	while (tests[i].testFn) {
		printf("Running test %2d: %s... ", i + 1, tests[i].description);
		fflush(stdout);
		if (tests[i].testFn()) {
			printf("pass\n");
			passes++;
		} else {
			printf("fail\n");
			fails++;
		}
		i++;
		ODBC_FREE();
	}

	if (fails == 0) {
		printf("\nAll %d tests passed.\n\n", passes);
	} else {
		printf("\nTest passes: %d, test fails: %d\n\n", passes, fails);
	}

	/* Return TRUE if there are no failures */
	return (!fails);
}

int
main(int argc, char *argv[])
{
	odbc_use_version3 = 1;

	if (RunTests())
		return 0;	/* Success */
	return 1;	/* Error code */
}
