#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd, ODBC_CHAR * szConnStrIn, SQLSMALLINT cbConnStrIn, ODBC_CHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT FAR* pcbConnStrOut, SQLUSMALLINT fDriverCompletion, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn, SQLCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT FAR* pcbConnStrOut, SQLUSMALLINT fDriverCompletion) {
	tdsdump_log(TDS_DBG_FUNC, "SQLDriverConnect(%p, %p, %s, %d, %p, %d, %p, %u)\n", hdbc, hwnd, (const char*) szConnStrIn, (int) cbConnStrIn, szConnStrOut, (int) cbConnStrOutMax, pcbConnStrOut, (unsigned int) fDriverCompletion);
	return _SQLDriverConnect(hdbc, hwnd, (ODBC_CHAR*) szConnStrIn, cbConnStrIn, (ODBC_CHAR*) szConnStrOut, cbConnStrOutMax, pcbConnStrOut, fDriverCompletion, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLDriverConnectW(SQLHDBC hdbc, SQLHWND hwnd, SQLWCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn, SQLWCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT FAR* pcbConnStrOut, SQLUSMALLINT fDriverCompletion) {
	tdsdump_log(TDS_DBG_FUNC, "SQLDriverConnectW(%p, %p, %ls, %d, %p, %d, %p, %u)\n", hdbc, hwnd, sqlwstr(szConnStrIn), (int) cbConnStrIn, szConnStrOut, (int) cbConnStrOutMax, pcbConnStrOut, (unsigned int) fDriverCompletion);
	return _SQLDriverConnect(hdbc, hwnd, (ODBC_CHAR*) szConnStrIn, cbConnStrIn, (ODBC_CHAR*) szConnStrOut, cbConnStrOutMax, pcbConnStrOut, fDriverCompletion, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd, SQLCHAR * szConnStrIn, SQLSMALLINT cbConnStrIn, SQLCHAR * szConnStrOut, SQLSMALLINT cbConnStrOutMax, SQLSMALLINT FAR* pcbConnStrOut, SQLUSMALLINT fDriverCompletion) {
	tdsdump_log(TDS_DBG_FUNC, "SQLDriverConnect(%p, %p, %s, %d, %p, %d, %p, %u)\n", hdbc, hwnd, (const char*) szConnStrIn, (int) cbConnStrIn, szConnStrOut, (int) cbConnStrOutMax, pcbConnStrOut, (unsigned int) fDriverCompletion);
	return _SQLDriverConnect(hdbc, hwnd, szConnStrIn, cbConnStrIn, szConnStrOut, cbConnStrOutMax, pcbConnStrOut, fDriverCompletion);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLColumnPrivileges(SQLHSTMT hstmt, ODBC_CHAR * szCatalogName, SQLSMALLINT cbCatalogName, ODBC_CHAR * szSchemaName, SQLSMALLINT cbSchemaName, ODBC_CHAR * szTableName, SQLSMALLINT cbTableName, ODBC_CHAR * szColumnName, SQLSMALLINT cbColumnName, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLColumnPrivileges(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName, SQLCHAR * szColumnName, SQLSMALLINT cbColumnName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLColumnPrivileges(%p, %s, %d, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName, (const char*) szColumnName, (int) cbColumnName);
	return _SQLColumnPrivileges(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, (ODBC_CHAR*) szColumnName, cbColumnName, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLColumnPrivilegesW(SQLHSTMT hstmt, SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR * szTableName, SQLSMALLINT cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT cbColumnName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLColumnPrivilegesW(%p, %ls, %d, %ls, %d, %ls, %d, %ls, %d)\n", hstmt, sqlwstr(szCatalogName), (int) cbCatalogName, sqlwstr(szSchemaName), (int) cbSchemaName, sqlwstr(szTableName), (int) cbTableName, sqlwstr(szColumnName), (int) cbColumnName);
	return _SQLColumnPrivileges(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, (ODBC_CHAR*) szColumnName, cbColumnName, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLColumnPrivileges(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName, SQLCHAR * szColumnName, SQLSMALLINT cbColumnName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLColumnPrivileges(%p, %s, %d, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName, (const char*) szColumnName, (int) cbColumnName);
	return _SQLColumnPrivileges(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName, szColumnName, cbColumnName);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLForeignKeys(SQLHSTMT hstmt, ODBC_CHAR * szPkCatalogName, SQLSMALLINT cbPkCatalogName, ODBC_CHAR * szPkSchemaName, SQLSMALLINT cbPkSchemaName, ODBC_CHAR * szPkTableName, SQLSMALLINT cbPkTableName, ODBC_CHAR * szFkCatalogName, SQLSMALLINT cbFkCatalogName, ODBC_CHAR * szFkSchemaName, SQLSMALLINT cbFkSchemaName, ODBC_CHAR * szFkTableName, SQLSMALLINT cbFkTableName, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLForeignKeys(SQLHSTMT hstmt, SQLCHAR * szPkCatalogName, SQLSMALLINT cbPkCatalogName, SQLCHAR * szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLCHAR * szPkTableName, SQLSMALLINT cbPkTableName, SQLCHAR * szFkCatalogName, SQLSMALLINT cbFkCatalogName, SQLCHAR * szFkSchemaName, SQLSMALLINT cbFkSchemaName, SQLCHAR * szFkTableName, SQLSMALLINT cbFkTableName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLForeignKeys(%p, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szPkCatalogName, (int) cbPkCatalogName, (const char*) szPkSchemaName, (int) cbPkSchemaName, (const char*) szPkTableName, (int) cbPkTableName, (const char*) szFkCatalogName, (int) cbFkCatalogName, (const char*) szFkSchemaName, (int) cbFkSchemaName, (const char*) szFkTableName, (int) cbFkTableName);
	return _SQLForeignKeys(hstmt, (ODBC_CHAR*) szPkCatalogName, cbPkCatalogName, (ODBC_CHAR*) szPkSchemaName, cbPkSchemaName, (ODBC_CHAR*) szPkTableName, cbPkTableName, (ODBC_CHAR*) szFkCatalogName, cbFkCatalogName, (ODBC_CHAR*) szFkSchemaName, cbFkSchemaName, (ODBC_CHAR*) szFkTableName, cbFkTableName, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLForeignKeysW(SQLHSTMT hstmt, SQLWCHAR * szPkCatalogName, SQLSMALLINT cbPkCatalogName, SQLWCHAR * szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLWCHAR * szPkTableName, SQLSMALLINT cbPkTableName, SQLWCHAR * szFkCatalogName, SQLSMALLINT cbFkCatalogName, SQLWCHAR * szFkSchemaName, SQLSMALLINT cbFkSchemaName, SQLWCHAR * szFkTableName, SQLSMALLINT cbFkTableName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLForeignKeysW(%p, %ls, %d, %ls, %d, %ls, %d, %ls, %d, %ls, %d, %ls, %d)\n", hstmt, sqlwstr(szPkCatalogName), (int) cbPkCatalogName, sqlwstr(szPkSchemaName), (int) cbPkSchemaName, sqlwstr(szPkTableName), (int) cbPkTableName, sqlwstr(szFkCatalogName), (int) cbFkCatalogName, sqlwstr(szFkSchemaName), (int) cbFkSchemaName, sqlwstr(szFkTableName), (int) cbFkTableName);
	return _SQLForeignKeys(hstmt, (ODBC_CHAR*) szPkCatalogName, cbPkCatalogName, (ODBC_CHAR*) szPkSchemaName, cbPkSchemaName, (ODBC_CHAR*) szPkTableName, cbPkTableName, (ODBC_CHAR*) szFkCatalogName, cbFkCatalogName, (ODBC_CHAR*) szFkSchemaName, cbFkSchemaName, (ODBC_CHAR*) szFkTableName, cbFkTableName, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLForeignKeys(SQLHSTMT hstmt, SQLCHAR * szPkCatalogName, SQLSMALLINT cbPkCatalogName, SQLCHAR * szPkSchemaName, SQLSMALLINT cbPkSchemaName, SQLCHAR * szPkTableName, SQLSMALLINT cbPkTableName, SQLCHAR * szFkCatalogName, SQLSMALLINT cbFkCatalogName, SQLCHAR * szFkSchemaName, SQLSMALLINT cbFkSchemaName, SQLCHAR * szFkTableName, SQLSMALLINT cbFkTableName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLForeignKeys(%p, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szPkCatalogName, (int) cbPkCatalogName, (const char*) szPkSchemaName, (int) cbPkSchemaName, (const char*) szPkTableName, (int) cbPkTableName, (const char*) szFkCatalogName, (int) cbFkCatalogName, (const char*) szFkSchemaName, (int) cbFkSchemaName, (const char*) szFkTableName, (int) cbFkTableName);
	return _SQLForeignKeys(hstmt, szPkCatalogName, cbPkCatalogName, szPkSchemaName, cbPkSchemaName, szPkTableName, cbPkTableName, szFkCatalogName, cbFkCatalogName, szFkSchemaName, cbFkSchemaName, szFkTableName, cbFkTableName);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLNativeSql(SQLHDBC hdbc, ODBC_CHAR * szSqlStrIn, SQLINTEGER cbSqlStrIn, ODBC_CHAR * szSqlStr, SQLINTEGER cbSqlStrMax, SQLINTEGER FAR* pcbSqlStr, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLNativeSql(SQLHDBC hdbc, SQLCHAR * szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLCHAR * szSqlStr, SQLINTEGER cbSqlStrMax, SQLINTEGER FAR* pcbSqlStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLNativeSql(%p, %s, %d, %p, %d, %p)\n", hdbc, (const char*) szSqlStrIn, (int) cbSqlStrIn, szSqlStr, (int) cbSqlStrMax, pcbSqlStr);
	return _SQLNativeSql(hdbc, (ODBC_CHAR*) szSqlStrIn, cbSqlStrIn, (ODBC_CHAR*) szSqlStr, cbSqlStrMax, pcbSqlStr, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLNativeSqlW(SQLHDBC hdbc, SQLWCHAR * szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLWCHAR * szSqlStr, SQLINTEGER cbSqlStrMax, SQLINTEGER FAR* pcbSqlStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLNativeSqlW(%p, %ls, %d, %p, %d, %p)\n", hdbc, sqlwstr(szSqlStrIn), (int) cbSqlStrIn, szSqlStr, (int) cbSqlStrMax, pcbSqlStr);
	return _SQLNativeSql(hdbc, (ODBC_CHAR*) szSqlStrIn, cbSqlStrIn, (ODBC_CHAR*) szSqlStr, cbSqlStrMax, pcbSqlStr, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLNativeSql(SQLHDBC hdbc, SQLCHAR * szSqlStrIn, SQLINTEGER cbSqlStrIn, SQLCHAR * szSqlStr, SQLINTEGER cbSqlStrMax, SQLINTEGER FAR* pcbSqlStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLNativeSql(%p, %s, %d, %p, %d, %p)\n", hdbc, (const char*) szSqlStrIn, (int) cbSqlStrIn, szSqlStr, (int) cbSqlStrMax, pcbSqlStr);
	return _SQLNativeSql(hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax, pcbSqlStr);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLPrimaryKeys(SQLHSTMT hstmt, ODBC_CHAR * szCatalogName, SQLSMALLINT cbCatalogName, ODBC_CHAR * szSchemaName, SQLSMALLINT cbSchemaName, ODBC_CHAR * szTableName, SQLSMALLINT cbTableName, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLPrimaryKeys(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLPrimaryKeys(%p, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName);
	return _SQLPrimaryKeys(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLPrimaryKeysW(SQLHSTMT hstmt, SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR * szTableName, SQLSMALLINT cbTableName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLPrimaryKeysW(%p, %ls, %d, %ls, %d, %ls, %d)\n", hstmt, sqlwstr(szCatalogName), (int) cbCatalogName, sqlwstr(szSchemaName), (int) cbSchemaName, sqlwstr(szTableName), (int) cbTableName);
	return _SQLPrimaryKeys(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLPrimaryKeys(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLPrimaryKeys(%p, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName);
	return _SQLPrimaryKeys(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLProcedureColumns(SQLHSTMT hstmt, ODBC_CHAR * szCatalogName, SQLSMALLINT cbCatalogName, ODBC_CHAR * szSchemaName, SQLSMALLINT cbSchemaName, ODBC_CHAR * szProcName, SQLSMALLINT cbProcName, ODBC_CHAR * szColumnName, SQLSMALLINT cbColumnName, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLProcedureColumns(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szProcName, SQLSMALLINT cbProcName, SQLCHAR * szColumnName, SQLSMALLINT cbColumnName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLProcedureColumns(%p, %s, %d, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szProcName, (int) cbProcName, (const char*) szColumnName, (int) cbColumnName);
	return _SQLProcedureColumns(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szProcName, cbProcName, (ODBC_CHAR*) szColumnName, cbColumnName, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLProcedureColumnsW(SQLHSTMT hstmt, SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR * szProcName, SQLSMALLINT cbProcName, SQLWCHAR * szColumnName, SQLSMALLINT cbColumnName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLProcedureColumnsW(%p, %ls, %d, %ls, %d, %ls, %d, %ls, %d)\n", hstmt, sqlwstr(szCatalogName), (int) cbCatalogName, sqlwstr(szSchemaName), (int) cbSchemaName, sqlwstr(szProcName), (int) cbProcName, sqlwstr(szColumnName), (int) cbColumnName);
	return _SQLProcedureColumns(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szProcName, cbProcName, (ODBC_CHAR*) szColumnName, cbColumnName, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLProcedureColumns(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szProcName, SQLSMALLINT cbProcName, SQLCHAR * szColumnName, SQLSMALLINT cbColumnName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLProcedureColumns(%p, %s, %d, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szProcName, (int) cbProcName, (const char*) szColumnName, (int) cbColumnName);
	return _SQLProcedureColumns(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName, szColumnName, cbColumnName);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLProcedures(SQLHSTMT hstmt, ODBC_CHAR * szCatalogName, SQLSMALLINT cbCatalogName, ODBC_CHAR * szSchemaName, SQLSMALLINT cbSchemaName, ODBC_CHAR * szProcName, SQLSMALLINT cbProcName, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLProcedures(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szProcName, SQLSMALLINT cbProcName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLProcedures(%p, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szProcName, (int) cbProcName);
	return _SQLProcedures(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szProcName, cbProcName, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLProceduresW(SQLHSTMT hstmt, SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR * szProcName, SQLSMALLINT cbProcName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLProceduresW(%p, %ls, %d, %ls, %d, %ls, %d)\n", hstmt, sqlwstr(szCatalogName), (int) cbCatalogName, sqlwstr(szSchemaName), (int) cbSchemaName, sqlwstr(szProcName), (int) cbProcName);
	return _SQLProcedures(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szProcName, cbProcName, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLProcedures(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szProcName, SQLSMALLINT cbProcName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLProcedures(%p, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szProcName, (int) cbProcName);
	return _SQLProcedures(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szProcName, cbProcName);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLTablePrivileges(SQLHSTMT hstmt, ODBC_CHAR * szCatalogName, SQLSMALLINT cbCatalogName, ODBC_CHAR * szSchemaName, SQLSMALLINT cbSchemaName, ODBC_CHAR * szTableName, SQLSMALLINT cbTableName, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLTablePrivileges(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLTablePrivileges(%p, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName);
	return _SQLTablePrivileges(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLTablePrivilegesW(SQLHSTMT hstmt, SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR * szTableName, SQLSMALLINT cbTableName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLTablePrivilegesW(%p, %ls, %d, %ls, %d, %ls, %d)\n", hstmt, sqlwstr(szCatalogName), (int) cbCatalogName, sqlwstr(szSchemaName), (int) cbSchemaName, sqlwstr(szTableName), (int) cbTableName);
	return _SQLTablePrivileges(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLTablePrivileges(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLTablePrivileges(%p, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName);
	return _SQLTablePrivileges(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLConnect(SQLHDBC hdbc, ODBC_CHAR * szDSN, SQLSMALLINT cbDSN, ODBC_CHAR * szUID, SQLSMALLINT cbUID, ODBC_CHAR * szAuthStr, SQLSMALLINT cbAuthStr, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLConnect(SQLHDBC hdbc, SQLCHAR * szDSN, SQLSMALLINT cbDSN, SQLCHAR * szUID, SQLSMALLINT cbUID, SQLCHAR * szAuthStr, SQLSMALLINT cbAuthStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLConnect(%p, %s, %d, %s, %d, %s, %d)\n", hdbc, (const char*) szDSN, (int) cbDSN, (const char*) szUID, (int) cbUID, (const char*) szAuthStr, (int) cbAuthStr);
	return _SQLConnect(hdbc, (ODBC_CHAR*) szDSN, cbDSN, (ODBC_CHAR*) szUID, cbUID, (ODBC_CHAR*) szAuthStr, cbAuthStr, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLConnectW(SQLHDBC hdbc, SQLWCHAR * szDSN, SQLSMALLINT cbDSN, SQLWCHAR * szUID, SQLSMALLINT cbUID, SQLWCHAR * szAuthStr, SQLSMALLINT cbAuthStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLConnectW(%p, %ls, %d, %ls, %d, %ls, %d)\n", hdbc, sqlwstr(szDSN), (int) cbDSN, sqlwstr(szUID), (int) cbUID, sqlwstr(szAuthStr), (int) cbAuthStr);
	return _SQLConnect(hdbc, (ODBC_CHAR*) szDSN, cbDSN, (ODBC_CHAR*) szUID, cbUID, (ODBC_CHAR*) szAuthStr, cbAuthStr, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLConnect(SQLHDBC hdbc, SQLCHAR * szDSN, SQLSMALLINT cbDSN, SQLCHAR * szUID, SQLSMALLINT cbUID, SQLCHAR * szAuthStr, SQLSMALLINT cbAuthStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLConnect(%p, %s, %d, %s, %d, %s, %d)\n", hdbc, (const char*) szDSN, (int) cbDSN, (const char*) szUID, (int) cbUID, (const char*) szAuthStr, (int) cbAuthStr);
	return _SQLConnect(hdbc, szDSN, cbDSN, szUID, cbUID, szAuthStr, cbAuthStr);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLDescribeCol(SQLHSTMT hstmt, SQLUSMALLINT icol, ODBC_CHAR * szColName, SQLSMALLINT cbColNameMax, SQLSMALLINT FAR* pcbColName, SQLSMALLINT * pfSqlType, SQLULEN * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLDescribeCol(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLCHAR * szColName, SQLSMALLINT cbColNameMax, SQLSMALLINT FAR* pcbColName, SQLSMALLINT * pfSqlType, SQLULEN * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable) {
	tdsdump_log(TDS_DBG_FUNC, "SQLDescribeCol(%p, %u, %p, %d, %p, %p, %p, %p, %p)\n", hstmt, (unsigned int) icol, szColName, (int) cbColNameMax, pcbColName, pfSqlType, pcbColDef, pibScale, pfNullable);
	return _SQLDescribeCol(hstmt, icol, (ODBC_CHAR*) szColName, cbColNameMax, pcbColName, pfSqlType, pcbColDef, pibScale, pfNullable, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLDescribeColW(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLWCHAR * szColName, SQLSMALLINT cbColNameMax, SQLSMALLINT FAR* pcbColName, SQLSMALLINT * pfSqlType, SQLULEN * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable) {
	tdsdump_log(TDS_DBG_FUNC, "SQLDescribeColW(%p, %u, %p, %d, %p, %p, %p, %p, %p)\n", hstmt, (unsigned int) icol, szColName, (int) cbColNameMax, pcbColName, pfSqlType, pcbColDef, pibScale, pfNullable);
	return _SQLDescribeCol(hstmt, icol, (ODBC_CHAR*) szColName, cbColNameMax, pcbColName, pfSqlType, pcbColDef, pibScale, pfNullable, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLDescribeCol(SQLHSTMT hstmt, SQLUSMALLINT icol, SQLCHAR * szColName, SQLSMALLINT cbColNameMax, SQLSMALLINT FAR* pcbColName, SQLSMALLINT * pfSqlType, SQLULEN * pcbColDef, SQLSMALLINT * pibScale, SQLSMALLINT * pfNullable) {
	tdsdump_log(TDS_DBG_FUNC, "SQLDescribeCol(%p, %u, %p, %d, %p, %p, %p, %p, %p)\n", hstmt, (unsigned int) icol, szColName, (int) cbColNameMax, pcbColName, pfSqlType, pcbColDef, pibScale, pfNullable);
	return _SQLDescribeCol(hstmt, icol, szColName, cbColNameMax, pcbColName, pfSqlType, pcbColDef, pibScale, pfNullable);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLGetDescRec(SQLHDESC hdesc, SQLSMALLINT RecordNumber, ODBC_CHAR * szName, SQLSMALLINT cbNameMax, SQLSMALLINT FAR* pcbName, SQLSMALLINT * Type, SQLSMALLINT * SubType, SQLLEN * Length, SQLSMALLINT * Precision, SQLSMALLINT * Scale, SQLSMALLINT * Nullable, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetDescRec(SQLHDESC hdesc, SQLSMALLINT RecordNumber, SQLCHAR * szName, SQLSMALLINT cbNameMax, SQLSMALLINT FAR* pcbName, SQLSMALLINT * Type, SQLSMALLINT * SubType, SQLLEN * Length, SQLSMALLINT * Precision, SQLSMALLINT * Scale, SQLSMALLINT * Nullable) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetDescRec(%p, %d, %p, %d, %p, %p, %p, %p, %p, %p, %p)\n", hdesc, (int) RecordNumber, szName, (int) cbNameMax, pcbName, Type, SubType, Length, Precision, Scale, Nullable);
	return _SQLGetDescRec(hdesc, RecordNumber, (ODBC_CHAR*) szName, cbNameMax, pcbName, Type, SubType, Length, Precision, Scale, Nullable, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetDescRecW(SQLHDESC hdesc, SQLSMALLINT RecordNumber, SQLWCHAR * szName, SQLSMALLINT cbNameMax, SQLSMALLINT FAR* pcbName, SQLSMALLINT * Type, SQLSMALLINT * SubType, SQLLEN * Length, SQLSMALLINT * Precision, SQLSMALLINT * Scale, SQLSMALLINT * Nullable) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetDescRecW(%p, %d, %p, %d, %p, %p, %p, %p, %p, %p, %p)\n", hdesc, (int) RecordNumber, szName, (int) cbNameMax, pcbName, Type, SubType, Length, Precision, Scale, Nullable);
	return _SQLGetDescRec(hdesc, RecordNumber, (ODBC_CHAR*) szName, cbNameMax, pcbName, Type, SubType, Length, Precision, Scale, Nullable, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetDescRec(SQLHDESC hdesc, SQLSMALLINT RecordNumber, SQLCHAR * szName, SQLSMALLINT cbNameMax, SQLSMALLINT FAR* pcbName, SQLSMALLINT * Type, SQLSMALLINT * SubType, SQLLEN * Length, SQLSMALLINT * Precision, SQLSMALLINT * Scale, SQLSMALLINT * Nullable) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetDescRec(%p, %d, %p, %d, %p, %p, %p, %p, %p, %p, %p)\n", hdesc, (int) RecordNumber, szName, (int) cbNameMax, pcbName, Type, SubType, Length, Precision, Scale, Nullable);
	return _SQLGetDescRec(hdesc, RecordNumber, szName, cbNameMax, pcbName, Type, SubType, Length, Precision, Scale, Nullable);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLGetDescField(SQLHDESC hdesc, SQLSMALLINT icol, SQLSMALLINT fDescType, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetDescField(SQLHDESC hdesc, SQLSMALLINT icol, SQLSMALLINT fDescType, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetDescField(%p, %d, %d, %p, %d, %p)\n", hdesc, (int) icol, (int) fDescType, Value, (int) BufferLength, StringLength);
	return _SQLGetDescField(hdesc, icol, fDescType, Value, BufferLength, StringLength, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetDescFieldW(SQLHDESC hdesc, SQLSMALLINT icol, SQLSMALLINT fDescType, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetDescFieldW(%p, %d, %d, %p, %d, %p)\n", hdesc, (int) icol, (int) fDescType, Value, (int) BufferLength, StringLength);
	return _SQLGetDescField(hdesc, icol, fDescType, Value, BufferLength, StringLength, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetDescField(SQLHDESC hdesc, SQLSMALLINT icol, SQLSMALLINT fDescType, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetDescField(%p, %d, %d, %p, %d, %p)\n", hdesc, (int) icol, (int) fDescType, Value, (int) BufferLength, StringLength);
	return _SQLGetDescField(hdesc, icol, fDescType, Value, BufferLength, StringLength);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLSetDescField(SQLHDESC hdesc, SQLSMALLINT icol, SQLSMALLINT fDescType, SQLPOINTER Value, SQLINTEGER BufferLength, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLSetDescField(SQLHDESC hdesc, SQLSMALLINT icol, SQLSMALLINT fDescType, SQLPOINTER Value, SQLINTEGER BufferLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSetDescField(%p, %d, %d, %p, %d)\n", hdesc, (int) icol, (int) fDescType, Value, (int) BufferLength);
	return _SQLSetDescField(hdesc, icol, fDescType, Value, BufferLength, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLSetDescFieldW(SQLHDESC hdesc, SQLSMALLINT icol, SQLSMALLINT fDescType, SQLPOINTER Value, SQLINTEGER BufferLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSetDescFieldW(%p, %d, %d, %p, %d)\n", hdesc, (int) icol, (int) fDescType, Value, (int) BufferLength);
	return _SQLSetDescField(hdesc, icol, fDescType, Value, BufferLength, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLSetDescField(SQLHDESC hdesc, SQLSMALLINT icol, SQLSMALLINT fDescType, SQLPOINTER Value, SQLINTEGER BufferLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSetDescField(%p, %d, %d, %p, %d)\n", hdesc, (int) icol, (int) fDescType, Value, (int) BufferLength);
	return _SQLSetDescField(hdesc, icol, fDescType, Value, BufferLength);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLExecDirect(SQLHSTMT hstmt, ODBC_CHAR * szSqlStr, SQLINTEGER cbSqlStr, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLExecDirect(SQLHSTMT hstmt, SQLCHAR * szSqlStr, SQLINTEGER cbSqlStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLExecDirect(%p, %s, %d)\n", hstmt, (const char*) szSqlStr, (int) cbSqlStr);
	return _SQLExecDirect(hstmt, (ODBC_CHAR*) szSqlStr, cbSqlStr, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLExecDirectW(SQLHSTMT hstmt, SQLWCHAR * szSqlStr, SQLINTEGER cbSqlStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLExecDirectW(%p, %ls, %d)\n", hstmt, sqlwstr(szSqlStr), (int) cbSqlStr);
	return _SQLExecDirect(hstmt, (ODBC_CHAR*) szSqlStr, cbSqlStr, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLExecDirect(SQLHSTMT hstmt, SQLCHAR * szSqlStr, SQLINTEGER cbSqlStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLExecDirect(%p, %s, %d)\n", hstmt, (const char*) szSqlStr, (int) cbSqlStr);
	return _SQLExecDirect(hstmt, szSqlStr, cbSqlStr);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLPrepare(SQLHSTMT hstmt, ODBC_CHAR * szSqlStr, SQLINTEGER cbSqlStr, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLPrepare(SQLHSTMT hstmt, SQLCHAR * szSqlStr, SQLINTEGER cbSqlStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLPrepare(%p, %s, %d)\n", hstmt, (const char*) szSqlStr, (int) cbSqlStr);
	return _SQLPrepare(hstmt, (ODBC_CHAR*) szSqlStr, cbSqlStr, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLPrepareW(SQLHSTMT hstmt, SQLWCHAR * szSqlStr, SQLINTEGER cbSqlStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLPrepareW(%p, %ls, %d)\n", hstmt, sqlwstr(szSqlStr), (int) cbSqlStr);
	return _SQLPrepare(hstmt, (ODBC_CHAR*) szSqlStr, cbSqlStr, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLPrepare(SQLHSTMT hstmt, SQLCHAR * szSqlStr, SQLINTEGER cbSqlStr) {
	tdsdump_log(TDS_DBG_FUNC, "SQLPrepare(%p, %s, %d)\n", hstmt, (const char*) szSqlStr, (int) cbSqlStr);
	return _SQLPrepare(hstmt, szSqlStr, cbSqlStr);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLSetCursorName(SQLHSTMT hstmt, ODBC_CHAR * szCursor, SQLSMALLINT cbCursor, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLSetCursorName(SQLHSTMT hstmt, SQLCHAR * szCursor, SQLSMALLINT cbCursor) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSetCursorName(%p, %s, %d)\n", hstmt, (const char*) szCursor, (int) cbCursor);
	return _SQLSetCursorName(hstmt, (ODBC_CHAR*) szCursor, cbCursor, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLSetCursorNameW(SQLHSTMT hstmt, SQLWCHAR * szCursor, SQLSMALLINT cbCursor) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSetCursorNameW(%p, %ls, %d)\n", hstmt, sqlwstr(szCursor), (int) cbCursor);
	return _SQLSetCursorName(hstmt, (ODBC_CHAR*) szCursor, cbCursor, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLSetCursorName(SQLHSTMT hstmt, SQLCHAR * szCursor, SQLSMALLINT cbCursor) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSetCursorName(%p, %s, %d)\n", hstmt, (const char*) szCursor, (int) cbCursor);
	return _SQLSetCursorName(hstmt, szCursor, cbCursor);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLGetCursorName(SQLHSTMT hstmt, ODBC_CHAR * szCursor, SQLSMALLINT cbCursorMax, SQLSMALLINT FAR* pcbCursor, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetCursorName(SQLHSTMT hstmt, SQLCHAR * szCursor, SQLSMALLINT cbCursorMax, SQLSMALLINT FAR* pcbCursor) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetCursorName(%p, %p, %d, %p)\n", hstmt, szCursor, (int) cbCursorMax, pcbCursor);
	return _SQLGetCursorName(hstmt, (ODBC_CHAR*) szCursor, cbCursorMax, pcbCursor, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetCursorNameW(SQLHSTMT hstmt, SQLWCHAR * szCursor, SQLSMALLINT cbCursorMax, SQLSMALLINT FAR* pcbCursor) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetCursorNameW(%p, %p, %d, %p)\n", hstmt, szCursor, (int) cbCursorMax, pcbCursor);
	return _SQLGetCursorName(hstmt, (ODBC_CHAR*) szCursor, cbCursorMax, pcbCursor, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetCursorName(SQLHSTMT hstmt, SQLCHAR * szCursor, SQLSMALLINT cbCursorMax, SQLSMALLINT FAR* pcbCursor) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetCursorName(%p, %p, %d, %p)\n", hstmt, szCursor, (int) cbCursorMax, pcbCursor);
	return _SQLGetCursorName(hstmt, szCursor, cbCursorMax, pcbCursor);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLColumns(SQLHSTMT hstmt, ODBC_CHAR * szCatalogName, SQLSMALLINT cbCatalogName, ODBC_CHAR * szSchemaName, SQLSMALLINT cbSchemaName, ODBC_CHAR * szTableName, SQLSMALLINT cbTableName, ODBC_CHAR * szColumnName, SQLSMALLINT cbColumnName, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLColumns(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName, SQLCHAR * szColumnName, SQLSMALLINT cbColumnName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLColumns(%p, %s, %d, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName, (const char*) szColumnName, (int) cbColumnName);
	return _SQLColumns(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, (ODBC_CHAR*) szColumnName, cbColumnName, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLColumnsW(SQLHSTMT hstmt, SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR * szTableName, SQLSMALLINT cbTableName, SQLWCHAR * szColumnName, SQLSMALLINT cbColumnName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLColumnsW(%p, %ls, %d, %ls, %d, %ls, %d, %ls, %d)\n", hstmt, sqlwstr(szCatalogName), (int) cbCatalogName, sqlwstr(szSchemaName), (int) cbSchemaName, sqlwstr(szTableName), (int) cbTableName, sqlwstr(szColumnName), (int) cbColumnName);
	return _SQLColumns(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, (ODBC_CHAR*) szColumnName, cbColumnName, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLColumns(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName, SQLCHAR * szColumnName, SQLSMALLINT cbColumnName) {
	tdsdump_log(TDS_DBG_FUNC, "SQLColumns(%p, %s, %d, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName, (const char*) szColumnName, (int) cbColumnName);
	return _SQLColumns(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName, szColumnName, cbColumnName);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLGetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetConnectAttr(%p, %d, %p, %d, %p)\n", hdbc, (int) Attribute, Value, (int) BufferLength, StringLength);
	return _SQLGetConnectAttr(hdbc, Attribute, Value, BufferLength, StringLength, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetConnectAttrW(SQLHDBC hdbc, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetConnectAttrW(%p, %d, %p, %d, %p)\n", hdbc, (int) Attribute, Value, (int) BufferLength, StringLength);
	return _SQLGetConnectAttr(hdbc, Attribute, Value, BufferLength, StringLength, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLGetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute, SQLPOINTER Value, SQLINTEGER BufferLength, SQLINTEGER * StringLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLGetConnectAttr(%p, %d, %p, %d, %p)\n", hdbc, (int) Attribute, Value, (int) BufferLength, StringLength);
	return _SQLGetConnectAttr(hdbc, Attribute, Value, BufferLength, StringLength);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLSetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLSetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSetConnectAttr(%p, %d, %p, %d)\n", hdbc, (int) Attribute, ValuePtr, (int) StringLength);
	return _SQLSetConnectAttr(hdbc, Attribute, ValuePtr, StringLength, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLSetConnectAttrW(SQLHDBC hdbc, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSetConnectAttrW(%p, %d, %p, %d)\n", hdbc, (int) Attribute, ValuePtr, (int) StringLength);
	return _SQLSetConnectAttr(hdbc, Attribute, ValuePtr, StringLength, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLSetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute, SQLPOINTER ValuePtr, SQLINTEGER StringLength) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSetConnectAttr(%p, %d, %p, %d)\n", hdbc, (int) Attribute, ValuePtr, (int) StringLength);
	return _SQLSetConnectAttr(hdbc, Attribute, ValuePtr, StringLength);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLSpecialColumns(SQLHSTMT hstmt, SQLUSMALLINT fColType, ODBC_CHAR * szCatalogName, SQLSMALLINT cbCatalogName, ODBC_CHAR * szSchemaName, SQLSMALLINT cbSchemaName, ODBC_CHAR * szTableName, SQLSMALLINT cbTableName, SQLUSMALLINT fScope, SQLUSMALLINT fNullable, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLSpecialColumns(SQLHSTMT hstmt, SQLUSMALLINT fColType, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName, SQLUSMALLINT fScope, SQLUSMALLINT fNullable) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSpecialColumns(%p, %u, %s, %d, %s, %d, %s, %d, %u, %u)\n", hstmt, (unsigned int) fColType, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName, (unsigned int) fScope, (unsigned int) fNullable);
	return _SQLSpecialColumns(hstmt, fColType, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, fScope, fNullable, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLSpecialColumnsW(SQLHSTMT hstmt, SQLUSMALLINT fColType, SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR * szTableName, SQLSMALLINT cbTableName, SQLUSMALLINT fScope, SQLUSMALLINT fNullable) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSpecialColumnsW(%p, %u, %ls, %d, %ls, %d, %ls, %d, %u, %u)\n", hstmt, (unsigned int) fColType, sqlwstr(szCatalogName), (int) cbCatalogName, sqlwstr(szSchemaName), (int) cbSchemaName, sqlwstr(szTableName), (int) cbTableName, (unsigned int) fScope, (unsigned int) fNullable);
	return _SQLSpecialColumns(hstmt, fColType, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, fScope, fNullable, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLSpecialColumns(SQLHSTMT hstmt, SQLUSMALLINT fColType, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName, SQLUSMALLINT fScope, SQLUSMALLINT fNullable) {
	tdsdump_log(TDS_DBG_FUNC, "SQLSpecialColumns(%p, %u, %s, %d, %s, %d, %s, %d, %u, %u)\n", hstmt, (unsigned int) fColType, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName, (unsigned int) fScope, (unsigned int) fNullable);
	return _SQLSpecialColumns(hstmt, fColType, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName, fScope, fNullable);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLStatistics(SQLHSTMT hstmt, ODBC_CHAR * szCatalogName, SQLSMALLINT cbCatalogName, ODBC_CHAR * szSchemaName, SQLSMALLINT cbSchemaName, ODBC_CHAR * szTableName, SQLSMALLINT cbTableName, SQLUSMALLINT fUnique, SQLUSMALLINT fAccuracy, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLStatistics(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName, SQLUSMALLINT fUnique, SQLUSMALLINT fAccuracy) {
	tdsdump_log(TDS_DBG_FUNC, "SQLStatistics(%p, %s, %d, %s, %d, %s, %d, %u, %u)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName, (unsigned int) fUnique, (unsigned int) fAccuracy);
	return _SQLStatistics(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, fUnique, fAccuracy, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLStatisticsW(SQLHSTMT hstmt, SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR * szTableName, SQLSMALLINT cbTableName, SQLUSMALLINT fUnique, SQLUSMALLINT fAccuracy) {
	tdsdump_log(TDS_DBG_FUNC, "SQLStatisticsW(%p, %ls, %d, %ls, %d, %ls, %d, %u, %u)\n", hstmt, sqlwstr(szCatalogName), (int) cbCatalogName, sqlwstr(szSchemaName), (int) cbSchemaName, sqlwstr(szTableName), (int) cbTableName, (unsigned int) fUnique, (unsigned int) fAccuracy);
	return _SQLStatistics(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, fUnique, fAccuracy, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLStatistics(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName, SQLUSMALLINT fUnique, SQLUSMALLINT fAccuracy) {
	tdsdump_log(TDS_DBG_FUNC, "SQLStatistics(%p, %s, %d, %s, %d, %s, %d, %u, %u)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName, (unsigned int) fUnique, (unsigned int) fAccuracy);
	return _SQLStatistics(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName, fUnique, fAccuracy);
}
#endif

#ifdef ENABLE_ODBC_WIDE
static SQLRETURN _SQLTables(SQLHSTMT hstmt, ODBC_CHAR * szCatalogName, SQLSMALLINT cbCatalogName, ODBC_CHAR * szSchemaName, SQLSMALLINT cbSchemaName, ODBC_CHAR * szTableName, SQLSMALLINT cbTableName, ODBC_CHAR * szTableType, SQLSMALLINT cbTableType, int wide);
SQLRETURN ODBC_PUBLIC ODBC_API SQLTables(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName, SQLCHAR * szTableType, SQLSMALLINT cbTableType) {
	tdsdump_log(TDS_DBG_FUNC, "SQLTables(%p, %s, %d, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName, (const char*) szTableType, (int) cbTableType);
	return _SQLTables(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, (ODBC_CHAR*) szTableType, cbTableType, 0);
}
SQLRETURN ODBC_PUBLIC ODBC_API SQLTablesW(SQLHSTMT hstmt, SQLWCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLWCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLWCHAR * szTableName, SQLSMALLINT cbTableName, SQLWCHAR * szTableType, SQLSMALLINT cbTableType) {
	tdsdump_log(TDS_DBG_FUNC, "SQLTablesW(%p, %ls, %d, %ls, %d, %ls, %d, %ls, %d)\n", hstmt, sqlwstr(szCatalogName), (int) cbCatalogName, sqlwstr(szSchemaName), (int) cbSchemaName, sqlwstr(szTableName), (int) cbTableName, sqlwstr(szTableType), (int) cbTableType);
	return _SQLTables(hstmt, (ODBC_CHAR*) szCatalogName, cbCatalogName, (ODBC_CHAR*) szSchemaName, cbSchemaName, (ODBC_CHAR*) szTableName, cbTableName, (ODBC_CHAR*) szTableType, cbTableType, 1);
}
#else
SQLRETURN ODBC_PUBLIC ODBC_API SQLTables(SQLHSTMT hstmt, SQLCHAR * szCatalogName, SQLSMALLINT cbCatalogName, SQLCHAR * szSchemaName, SQLSMALLINT cbSchemaName, SQLCHAR * szTableName, SQLSMALLINT cbTableName, SQLCHAR * szTableType, SQLSMALLINT cbTableType) {
	tdsdump_log(TDS_DBG_FUNC, "SQLTables(%p, %s, %d, %s, %d, %s, %d, %s, %d)\n", hstmt, (const char*) szCatalogName, (int) cbCatalogName, (const char*) szSchemaName, (int) cbSchemaName, (const char*) szTableName, (int) cbTableName, (const char*) szTableType, (int) cbTableType);
	return _SQLTables(hstmt, szCatalogName, cbCatalogName, szSchemaName, cbSchemaName, szTableName, cbTableName, szTableType, cbTableType);
}
#endif

