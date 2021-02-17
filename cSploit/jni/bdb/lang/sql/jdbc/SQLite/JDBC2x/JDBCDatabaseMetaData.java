package SQLite.JDBC2x;

import java.sql.Connection;
import java.sql.DatabaseMetaData;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Types;
import java.util.Hashtable;

public class JDBCDatabaseMetaData implements DatabaseMetaData {

    private JDBCConnection conn;

    public JDBCDatabaseMetaData(JDBCConnection conn) {
	this.conn = conn;
    }

    public boolean allProceduresAreCallable() throws SQLException {
	return false;
    }

    public boolean allTablesAreSelectable() throws SQLException {
	return true;
    }

    public String getURL() throws SQLException {
	return conn.url;
    }

    public String getUserName() throws SQLException {
	return "";
    }

    public boolean isReadOnly() throws SQLException {
	return false;
    }

    public boolean nullsAreSortedHigh() throws SQLException {
	return false;
    }

    public boolean nullsAreSortedLow() throws SQLException {
	return false;
    }

    public boolean nullsAreSortedAtStart() throws SQLException {
	return false;
    }

    public boolean nullsAreSortedAtEnd() throws SQLException {
	return false;
    }

    public String getDatabaseProductName() throws SQLException {
	return "SQLite";
    }

    public String getDatabaseProductVersion() throws SQLException {
	return SQLite.Database.version();
    }

    public String getDriverName() throws SQLException {
	return "SQLite/JDBC";
    }

    public String getDriverVersion() throws SQLException {
	return "" + SQLite.JDBC.MAJORVERSION + "." +
	    SQLite.Constants.drv_minor;
    }

    public int getDriverMajorVersion() {
	return SQLite.JDBC.MAJORVERSION;
    }

    public int getDriverMinorVersion() {
	return SQLite.Constants.drv_minor;
    }

    public boolean usesLocalFiles() throws SQLException {
	return true;
    }

    public boolean usesLocalFilePerTable() throws SQLException {
	return false;
    }

    public boolean supportsMixedCaseIdentifiers() throws SQLException {
	return false;
    }

    public boolean storesUpperCaseIdentifiers() throws SQLException {
	return false;
    }

    public boolean storesLowerCaseIdentifiers() throws SQLException {
	return false;
    }

    public boolean storesMixedCaseIdentifiers() throws SQLException {
	return true;
    }

    public boolean supportsMixedCaseQuotedIdentifiers() throws SQLException {
	return false;
    }

    public boolean storesUpperCaseQuotedIdentifiers() throws SQLException {
	return false;
    }

    public boolean storesLowerCaseQuotedIdentifiers() throws SQLException {
	return false;
    }

    public boolean storesMixedCaseQuotedIdentifiers() throws SQLException {
	return true;
    }

    public String getIdentifierQuoteString() throws SQLException {
	return "\"";
    }

    public String getSQLKeywords() throws SQLException {
	return "SELECT,UPDATE,CREATE,TABLE,VIEW,DELETE,FROM,WHERE" +
	    ",COMMIT,ROLLBACK,TRIGGER";
    }

    public String getNumericFunctions() throws SQLException {
	return ""; 
    }

    public String getStringFunctions() throws SQLException {
	return "";
    }

    public String getSystemFunctions() throws SQLException {
	return "";
    }

    public String getTimeDateFunctions() throws SQLException {
	return "";
    }

    public String getSearchStringEscape() throws SQLException {
	return "\\";
    }

    public String getExtraNameCharacters() throws SQLException {
	return "";
    }

    public boolean supportsAlterTableWithAddColumn() throws SQLException {
	return false;
    }

    public boolean supportsAlterTableWithDropColumn() throws SQLException {
	return false;
    }

    public boolean supportsColumnAliasing() throws SQLException {
	return true;
    }

    public boolean nullPlusNonNullIsNull() throws SQLException {
	return false;
    }
    
    public boolean supportsConvert() throws SQLException {
	return false;
    }

    public boolean supportsConvert(int fromType, int toType)
	throws SQLException {
	return false;
    }

    public boolean supportsTableCorrelationNames() throws SQLException {
	return true;
    }

    public boolean supportsDifferentTableCorrelationNames()
	throws SQLException {
	return false;
    }

    public boolean supportsExpressionsInOrderBy() throws SQLException {
	return true;
    }

    public boolean supportsOrderByUnrelated() throws SQLException {
	return true;
    }

    public boolean supportsGroupBy() throws SQLException {
	return true;
    }

    public boolean supportsGroupByUnrelated() throws SQLException {
	return true;
    }

    public boolean supportsGroupByBeyondSelect() throws SQLException {
	return false;
    }

    public boolean supportsLikeEscapeClause() throws SQLException {
	return false;
    }

    public boolean supportsMultipleResultSets() throws SQLException {
	return false;
    }

    public boolean supportsMultipleTransactions() throws SQLException {
	return false;
    }

    public boolean supportsNonNullableColumns() throws SQLException {
	return true;
    }

    public boolean supportsMinimumSQLGrammar() throws SQLException {
	return true;
    } 

    public boolean supportsCoreSQLGrammar() throws SQLException {
	return false;
    }

    public boolean supportsExtendedSQLGrammar() throws SQLException {
	return false;
    }

    public boolean supportsANSI92EntryLevelSQL() throws SQLException {
	return true;
    }

    public boolean supportsANSI92IntermediateSQL() throws SQLException {
	return false;
    }

    public boolean supportsANSI92FullSQL() throws SQLException {
	return false;
    }

    public boolean supportsIntegrityEnhancementFacility()
	throws SQLException {
	return false;
    }

    public boolean supportsOuterJoins() throws SQLException {
	return false;
    }

    public boolean supportsFullOuterJoins() throws SQLException {
	return false;
    }

    public boolean supportsLimitedOuterJoins() throws SQLException {
	return false;
    }

    public String getSchemaTerm() throws SQLException {
	return "";
    }

    public String getProcedureTerm() throws SQLException {
	return "";
    }

    public String getCatalogTerm() throws SQLException {
	return "";
    }

    public boolean isCatalogAtStart() throws SQLException {
	return false;
    }

    public String getCatalogSeparator() throws SQLException {
	return "";
    }

    public boolean supportsSchemasInDataManipulation() throws SQLException {
	return false;
    }

    public boolean supportsSchemasInProcedureCalls() throws SQLException {
	return false;
    }

    public boolean supportsSchemasInTableDefinitions() throws SQLException {
	return false;
    }
    
    public boolean supportsSchemasInIndexDefinitions() throws SQLException {
	return false;
    }

    public boolean supportsSchemasInPrivilegeDefinitions()
	throws SQLException {
	return false;
    }

    public boolean supportsCatalogsInDataManipulation() throws SQLException {
	return false;
    }

    public boolean supportsCatalogsInProcedureCalls() throws SQLException {
	return false;
    }

    public boolean supportsCatalogsInTableDefinitions() throws SQLException {
	return false;
    }

    public boolean supportsCatalogsInIndexDefinitions() throws SQLException {
	return false;
    }

    public boolean supportsCatalogsInPrivilegeDefinitions()
	throws SQLException {
	return false;
    }

    public boolean supportsPositionedDelete() throws SQLException {
	return false;
    }

    public boolean supportsPositionedUpdate() throws SQLException {
	return false;
    }

    public boolean supportsSelectForUpdate() throws SQLException {
	return false;
    }

    public boolean supportsStoredProcedures() throws SQLException {
	return false;
    }

    public boolean supportsSubqueriesInComparisons() throws SQLException {
	return true;
    }

    public boolean supportsSubqueriesInExists() throws SQLException {
	return true;
    }

    public boolean supportsSubqueriesInIns() throws SQLException {
	return true;
    }

    public boolean supportsSubqueriesInQuantifieds() throws SQLException {
	return false;
    }

    public boolean supportsCorrelatedSubqueries() throws SQLException {
	return false;
    }

    public boolean supportsUnion() throws SQLException {
	return true;
    }

    public boolean supportsUnionAll() throws SQLException {
	return true;
    }

    public boolean supportsOpenCursorsAcrossCommit() throws SQLException {
	return false;
    }

    public boolean supportsOpenCursorsAcrossRollback() throws SQLException {
	return false;
    }

    public boolean supportsOpenStatementsAcrossCommit() throws SQLException {
	return false;
    }

    public boolean supportsOpenStatementsAcrossRollback() throws SQLException {
	return false;
    }

    public int getMaxBinaryLiteralLength() throws SQLException {
	return 0;
    }

    public int getMaxCharLiteralLength() throws SQLException {
	return 0;
    }

    public int getMaxColumnNameLength() throws SQLException {
	return 0;
    }

    public int getMaxColumnsInGroupBy() throws SQLException {
	return 0;
    }

    public int getMaxColumnsInIndex() throws SQLException {
	return 0;
    }

    public int getMaxColumnsInOrderBy() throws SQLException {
	return 0;
    }

    public int getMaxColumnsInSelect() throws SQLException {
	return 0;
    }

    public int getMaxColumnsInTable() throws SQLException {
	return 0;
    }

    public int getMaxConnections() throws SQLException {
	return 0;
    }

    public int getMaxCursorNameLength() throws SQLException {
	return 8;
    }

    public int getMaxIndexLength() throws SQLException {
	return 0;
    }

    public int getMaxSchemaNameLength() throws SQLException {
	return 0;
    }

    public int getMaxProcedureNameLength() throws SQLException {
	return 0;
    }

    public int getMaxCatalogNameLength() throws SQLException {
	return 0;
    }

    public int getMaxRowSize() throws SQLException {
	return 0;
    }

    public boolean doesMaxRowSizeIncludeBlobs() throws SQLException {
	return true;
    }

    public int getMaxStatementLength() throws SQLException {
	return 0;
    }

    public int getMaxStatements() throws SQLException {
	return 0;
    }

    public int getMaxTableNameLength() throws SQLException {
	return 0;
    }

    public int getMaxTablesInSelect() throws SQLException {
	return 0;
    }

    public int getMaxUserNameLength() throws SQLException {
	return 0;
    }

    public int getDefaultTransactionIsolation() throws SQLException {
	return Connection.TRANSACTION_SERIALIZABLE;
    }

    public boolean supportsTransactions() throws SQLException {
	return true;
    }

    public boolean supportsTransactionIsolationLevel(int level)
	throws SQLException {
	return level == Connection.TRANSACTION_SERIALIZABLE;
    }

    public boolean supportsDataDefinitionAndDataManipulationTransactions()
	throws SQLException {
	return true;
    }

    public boolean supportsDataManipulationTransactionsOnly()
	throws SQLException {
	return false;
    }

    public boolean dataDefinitionCausesTransactionCommit()
	throws SQLException {
	return false;
    }

    public boolean dataDefinitionIgnoredInTransactions() throws SQLException {
	return false;
    }

    public ResultSet getProcedures(String catalog, String schemaPattern,
				   String procedureNamePattern)
	throws SQLException {
	return null;
    }

    public ResultSet getProcedureColumns(String catalog,
					 String schemaPattern,
					 String procedureNamePattern, 
					 String columnNamePattern)
	throws SQLException {
	return null;
    }

    public ResultSet getTables(String catalog, String schemaPattern,
			       String tableNamePattern, String types[])
	throws SQLException {
	JDBCStatement s = new JDBCStatement(conn);
	StringBuffer sb = new StringBuffer(
		  "SELECT '' AS 'TABLE_CAT', " +
		  "'' AS 'TABLE_SCHEM', " +
		  "tbl_name AS 'TABLE_NAME', " +
		  "upper(type) AS 'TABLE_TYPE', " +
		  "'' AS REMARKS FROM sqlite_master " +
		  "WHERE tbl_name like ");
	if (tableNamePattern != null) {
	    sb.append(SQLite.Shell.sql_quote(tableNamePattern));
	} else {
	    sb.append("'%'");
	}
	sb.append(" AND ");
	if (types == null || types.length == 0) {
	    sb.append("(type = 'table' or type = 'view')");
	} else {
	    sb.append("(");
	    String sep = ""; 
	    for (int i = 0; i < types.length; i++) {
		sb.append(sep);
		sb.append("type = ");
		sb.append(SQLite.Shell.sql_quote(types[i].toLowerCase()));
		sep = " or ";
	    }
	    sb.append(")");
	}
	ResultSet rs = null;
	try {
	    rs = s.executeQuery(sb.toString());
	    s.close();
	} catch (SQLException e) {
	    throw e;
	} finally {
	    s.close();
	}
	return rs;
    }

    public ResultSet getSchemas() throws SQLException {
	String cols[] = { "TABLE_SCHEM" };
	SQLite.TableResult tr = new SQLite.TableResult();
	tr.columns(cols);
	String row[] = { "" };
	tr.newrow(row);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	return rs;
    }

    public ResultSet getCatalogs() throws SQLException {
	String cols[] = { "TABLE_CAT" };
	SQLite.TableResult tr = new SQLite.TableResult();
	tr.columns(cols);
	String row[] = { "" };
	tr.newrow(row);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	return rs;
    }

    public ResultSet getTableTypes() throws SQLException {
	String cols[] = { "TABLE_TYPE" };
	SQLite.TableResult tr = new SQLite.TableResult();
	tr.columns(cols);
	String row[] = new String[1];
	row[0] = "TABLE";
	tr.newrow(row);
	row = new String[1];
	row[0] = "VIEW";
	tr.newrow(row);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	return rs;
    }

    public ResultSet getColumns(String catalog, String schemaPattern,
				String tableNamePattern,
				String columnNamePattern)
	throws SQLException {
	if (conn.db == null) {
	    throw new SQLException("connection closed");
	}
	JDBCStatement s = new JDBCStatement(conn);
	JDBCResultSet rs0 = null;
	try {
	    try {
		conn.db.exec("SELECT 1 FROM sqlite_master LIMIT 1", null);
	    } catch (SQLite.Exception se) {
		throw new
		    SQLException("schema reload failed: " + se.toString());
	    }
	    rs0 = (JDBCResultSet)
		(s.executeQuery("PRAGMA table_info(" +
				SQLite.Shell.sql_quote(tableNamePattern) +
				")"));
	    s.close();
	} catch (SQLException e) {
	    throw e;
	} finally {
	    s.close();
	}
	if (rs0.tr.nrows < 1) {
	    throw new SQLException("no such table: " + tableNamePattern);
	}
	String cols[] = {
	    "TABLE_CAT", "TABLE_SCHEM", "TABLE_NAME",
	    "COLUMN_NAME", "DATA_TYPE", "TYPE_NAME",
	    "COLUMN_SIZE", "BUFFER_LENGTH", "DECIMAL_DIGITS",
	    "NUM_PREC_RADIX", "NULLABLE", "REMARKS",
	    "COLUMN_DEF", "SQL_DATA_TYPE", "SQL_DATETIME_SUB",
	    "CHAR_OCTET_LENGTH", "ORDINAL_POSITION", "IS_NULLABLE"
	};
	int types[] = {
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.SMALLINT, Types.VARCHAR,
	    Types.INTEGER, Types.INTEGER, Types.INTEGER,
	    Types.INTEGER, Types.INTEGER, Types.VARCHAR,
	    Types.VARCHAR, Types.INTEGER, Types.INTEGER,
	    Types.INTEGER, Types.INTEGER, Types.VARCHAR
	};
	TableResultX tr = new TableResultX();
	tr.columns(cols);
	tr.sql_types(types);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	if (rs0.tr != null && rs0.tr.nrows > 0) {
	    Hashtable h = new Hashtable();
	    for (int i = 0; i < rs0.tr.ncolumns; i++) {
		h.put(rs0.tr.column[i], new Integer(i));
	    }
	    if (columnNamePattern != null &&
		columnNamePattern.charAt(0) == '%') {
		columnNamePattern = null;
	    }
	    for (int i = 0; i < rs0.tr.nrows; i++) {
		String r0[] = (String [])(rs0.tr.rows.elementAt(i));
		int col = ((Integer) h.get("name")).intValue();
		if (columnNamePattern != null) {
		    if (r0[col].compareTo(columnNamePattern) != 0) {
			continue;
		    }
		}
		String row[] = new String[cols.length];
		row[0]  = "";
		row[1]  = "";
		row[2]  = tableNamePattern;
		row[3]  = r0[col];
		col = ((Integer) h.get("type")).intValue();
		String typeStr = r0[col];
		int type = mapSqlType(typeStr);
		row[4]  = "" + type;
		row[5]  = mapTypeName(type);
		row[6]  = "" + getD(typeStr, type);
		row[7]  = "" + getM(typeStr, type);
		row[8]  = "10";
		row[9]  = "0";
		row[11] = null;
		col = ((Integer) h.get("dflt_value")).intValue();
		row[12] = r0[col];
		row[13] = "0";
		row[14] = "0";
		row[15] = "65536";
		col = ((Integer) h.get("cid")).intValue();
		Integer cid = new Integer(r0[col]);
		row[16] = "" + (cid.intValue() + 1);
		col = ((Integer) h.get("notnull")).intValue();
		row[17] = (r0[col].charAt(0) == '0') ? "YES" : "NO";
		row[10] = (r0[col].charAt(0) == '0') ? "" + columnNullable :
			  "" + columnNoNulls;
		tr.newrow(row);
	    }
	}
	return rs;
    }

    public ResultSet getColumnPrivileges(String catalog, String schema,
					 String table,
					 String columnNamePattern)
	throws SQLException {
	String cols[] = {
	    "TABLE_CAT", "TABLE_SCHEM", "TABLE_NAME",
	    "COLUMN_NAME", "GRANTOR", "GRANTEE",
	    "PRIVILEGE", "IS_GRANTABLE"
	};
	int types[] = {
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.VARCHAR
	};
	TableResultX tr = new TableResultX();
	tr.columns(cols);
	tr.sql_types(types);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	return rs;
    }

    public ResultSet getTablePrivileges(String catalog, String schemaPattern,
					String tableNamePattern)
	throws SQLException {
	String cols[] = {
	    "TABLE_CAT", "TABLE_SCHEM", "TABLE_NAME",
	    "COLUMN_NAME", "GRANTOR", "GRANTEE",
	    "PRIVILEGE", "IS_GRANTABLE"
	};
	int types[] = {
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.VARCHAR
	};
	TableResultX tr = new TableResultX();
	tr.columns(cols);
	tr.sql_types(types);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	return rs;
    }

    public ResultSet getBestRowIdentifier(String catalog, String schema,
					  String table, int scope,
					  boolean nullable)
	throws SQLException {
	JDBCStatement s0 = new JDBCStatement(conn);
	JDBCResultSet rs0 = null;
	JDBCStatement s1 = new JDBCStatement(conn);
	JDBCResultSet rs1 = null;
	try {
	    try {
		conn.db.exec("SELECT 1 FROM sqlite_master LIMIT 1", null);
	    } catch (SQLite.Exception se) {
		throw new
		    SQLException("schema reload failed: " + se.toString());
	    }
	    rs0 = (JDBCResultSet)
		(s0.executeQuery("PRAGMA index_list(" +
				 SQLite.Shell.sql_quote(table) + ")"));
	    rs1 = (JDBCResultSet)
		(s1.executeQuery("PRAGMA table_info(" +
				 SQLite.Shell.sql_quote(table) + ")"));
	} catch (SQLException e) {
	    throw e;
	} finally {
	    s0.close();
	    s1.close();
	}
	String cols[] = {
	    "SCOPE", "COLUMN_NAME", "DATA_TYPE",
	    "TYPE_NAME", "COLUMN_SIZE", "BUFFER_LENGTH",
	    "DECIMAL_DIGITS", "PSEUDO_COLUMN"
	};
	int types[] = {
	    Types.SMALLINT, Types.VARCHAR, Types.SMALLINT,
	    Types.VARCHAR, Types.INTEGER, Types.INTEGER,
	    Types.SMALLINT, Types.SMALLINT
	};
	TableResultX tr = new TableResultX();
	tr.columns(cols);
	tr.sql_types(types);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	if (rs0 != null && rs0.tr != null && rs0.tr.nrows > 0 &&
	    rs1 != null && rs1.tr != null && rs1.tr.nrows > 0) {
	    Hashtable h0 = new Hashtable();
	    for (int i = 0; i < rs0.tr.ncolumns; i++) {
		h0.put(rs0.tr.column[i], new Integer(i));
	    }
	    Hashtable h1 = new Hashtable();
	    for (int i = 0; i < rs1.tr.ncolumns; i++) {
		h1.put(rs1.tr.column[i], new Integer(i));
	    }
	    for (int i = 0; i < rs0.tr.nrows; i++) {
		String r0[] = (String [])(rs0.tr.rows.elementAt(i));
		int col = ((Integer) h0.get("unique")).intValue();
		String uniq = r0[col];
		col = ((Integer) h0.get("name")).intValue();
		String iname = r0[col];
		if (uniq.charAt(0) == '0') {
		    continue;
		}
		JDBCStatement s2 = new JDBCStatement(conn);
		JDBCResultSet rs2 = null;
		try {
		    rs2 = (JDBCResultSet)
			(s2.executeQuery("PRAGMA index_info(" +
					 SQLite.Shell.sql_quote(iname) + ")"));
		} catch (SQLException e) {
		} finally {
		    s2.close();
		}
		if (rs2 == null || rs2.tr == null || rs2.tr.nrows <= 0) {
		    continue;
		}
		Hashtable h2 = new Hashtable();
		for (int k = 0; k < rs2.tr.ncolumns; k++) {
		    h2.put(rs2.tr.column[k], new Integer(k));
		}
		for (int k = 0; k < rs2.tr.nrows; k++) {
		    String r2[] = (String [])(rs2.tr.rows.elementAt(k));
		    col = ((Integer) h2.get("name")).intValue();
		    String cname = r2[col];
		    for (int m = 0; m < rs1.tr.nrows; m++) {
			String r1[] = (String [])(rs1.tr.rows.elementAt(m));
			col = ((Integer) h1.get("name")).intValue();
			if (cname.compareTo(r1[col]) == 0) {
			    String row[] = new String[cols.length];
			    row[0] = "" + scope;
			    row[1] = cname;
			    row[2] = "" + Types.VARCHAR;
			    row[3] = "VARCHAR";
			    row[4] = "65536";
			    row[5] = "0";
			    row[6] = "0";
			    row[7] = "" + bestRowNotPseudo;
			    tr.newrow(row);
			}
		    }
		}
	    }
	}
	if (tr.nrows <= 0) {
	    String row[] = new String[cols.length];
	    row[0] = "" + scope;
	    row[1] = "_ROWID_";
	    row[2] = "" + Types.INTEGER;
	    row[3] = "INTEGER";
	    row[4] = "10";
	    row[5] = "0";
	    row[6] = "0";
	    row[7] = "" + bestRowPseudo;
	    tr.newrow(row);
	}
	return rs;
    }

    public ResultSet getVersionColumns(String catalog, String schema,
				       String table) throws SQLException {
	String cols[] = {
	    "SCOPE", "COLUMN_NAME", "DATA_TYPE",
	    "TYPE_NAME", "COLUMN_SIZE", "BUFFER_LENGTH",
	    "DECIMAL_DIGITS", "PSEUDO_COLUMN"
	};
	int types[] = {
	    Types.SMALLINT, Types.VARCHAR, Types.SMALLINT,
	    Types.VARCHAR, Types.INTEGER, Types.INTEGER,
	    Types.SMALLINT, Types.SMALLINT
	};
	TableResultX tr = new TableResultX();
	tr.columns(cols);
	tr.sql_types(types);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	return rs;
    }

    public ResultSet getPrimaryKeys(String catalog, String schema,
				    String table) throws SQLException {
	JDBCStatement s0 = new JDBCStatement(conn);
	JDBCResultSet rs0 = null;
	try {
	    try {
		conn.db.exec("SELECT 1 FROM sqlite_master LIMIT 1", null);
	    } catch (SQLite.Exception se) {
		throw new
		    SQLException("schema reload failed: " + se.toString());
	    }
	    rs0 = (JDBCResultSet)
		(s0.executeQuery("PRAGMA index_list(" +
				 SQLite.Shell.sql_quote(table) + ")"));
	} catch (SQLException e) {
	    throw e;
	} finally {
	    s0.close();
	}
	String cols[] = {
	    "TABLE_CAT", "TABLE_SCHEM", "TABLE_NAME",
	    "COLUMN_NAME", "KEY_SEQ", "PK_NAME"
	};
	int types[] = {
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.SMALLINT, Types.VARCHAR
	};
	TableResultX tr = new TableResultX();
	tr.columns(cols);
	tr.sql_types(types);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	if (rs0 != null && rs0.tr != null && rs0.tr.nrows > 0) {
	    Hashtable h0 = new Hashtable();
	    for (int i = 0; i < rs0.tr.ncolumns; i++) {
		h0.put(rs0.tr.column[i], new Integer(i));
	    }
	    for (int i = 0; i < rs0.tr.nrows; i++) {
		String r0[] = (String [])(rs0.tr.rows.elementAt(i));
		int col = ((Integer) h0.get("unique")).intValue();
		String uniq = r0[col];
		col = ((Integer) h0.get("name")).intValue();
		String iname = r0[col];
		if (uniq.charAt(0) == '0') {
		    continue;
		}
		JDBCStatement s1 = new JDBCStatement(conn);
		JDBCResultSet rs1 = null;
		try {
		    rs1 = (JDBCResultSet)
			(s1.executeQuery("PRAGMA index_info(" +
					 SQLite.Shell.sql_quote(iname) + ")"));
		} catch (SQLException e) {
		} finally {
		    s1.close();
		}
		if (rs1 == null || rs1.tr == null || rs1.tr.nrows <= 0) {
		    continue;
		}
		Hashtable h1 = new Hashtable();
		for (int k = 0; k < rs1.tr.ncolumns; k++) {
		    h1.put(rs1.tr.column[k], new Integer(k));
		}
		for (int k = 0; k < rs1.tr.nrows; k++) {
		    String r1[] = (String [])(rs1.tr.rows.elementAt(k));
		    String row[] = new String[cols.length];
		    row[0]  = "";
		    row[1]  = "";
		    row[2]  = table;
		    col = ((Integer) h1.get("name")).intValue();
		    row[3] = r1[col];
		    col = ((Integer) h1.get("seqno")).intValue();
		    row[4]  = Integer.toString(Integer.parseInt(r1[col]) + 1);
		    row[5]  = iname;
		    tr.newrow(row);
		}
	    }
	}
	if (tr.nrows > 0) {
	    return rs;
	}
	JDBCStatement s1 = new JDBCStatement(conn);
	try {
	    rs0 = (JDBCResultSet)
		(s1.executeQuery("PRAGMA table_info(" +
				 SQLite.Shell.sql_quote(table) + ")"));
	} catch (SQLException e) {
	    throw e;
	} finally {
	    s1.close();
	}
	if (rs0 != null && rs0.tr != null && rs0.tr.nrows > 0) {
	    Hashtable h0 = new Hashtable();
	    for (int i = 0; i < rs0.tr.ncolumns; i++) {
		h0.put(rs0.tr.column[i], new Integer(i));
	    }
	    for (int i = 0; i < rs0.tr.nrows; i++) {
		String r0[] = (String [])(rs0.tr.rows.elementAt(i));
		int col = ((Integer) h0.get("type")).intValue();
		String type = r0[col];
		if (!type.equalsIgnoreCase("integer")) {
		    continue;
		}
		col = ((Integer) h0.get("pk")).intValue();
		String pk = r0[col];
		if (pk.charAt(0) == '0') {
		    continue;
		}
		String row[] = new String[cols.length];
		row[0]  = "";
		row[1]  = "";
		row[2]  = table;
		col = ((Integer) h0.get("name")).intValue();
		row[3] = r0[col];
		col = ((Integer) h0.get("cid")).intValue();
		row[4] = Integer.toString(Integer.parseInt(r0[col]) + 1);
		row[5] = "";
		tr.newrow(row);
	    }
	}
	return rs;
    }

    private void internalImportedKeys(String table, String pktable,
				      JDBCResultSet in, TableResultX out) {
	Hashtable h0 = new Hashtable();
	for (int i = 0; i < in.tr.ncolumns; i++) {
	    h0.put(in.tr.column[i], new Integer(i));
	}
	for (int i = 0; i < in.tr.nrows; i++) {
	    String r0[] = (String [])(in.tr.rows.elementAt(i));
	    int col = ((Integer) h0.get("table")).intValue();
	    String pktab = r0[col];
	    if (pktable != null && !pktable.equalsIgnoreCase(pktab)) {
		continue;
	    }
	    col = ((Integer) h0.get("from")).intValue();
	    String fkcol = r0[col];
	    col = ((Integer) h0.get("to")).intValue();
	    String pkcol = r0[col];
	    col = ((Integer) h0.get("seq")).intValue();
	    String seq = r0[col];
	    String row[] = new String[out.ncolumns];
	    row[0]  = "";
	    row[1]  = "";
	    row[2]  = pktab;
	    row[3]  = pkcol;
	    row[4]  = "";
	    row[5]  = "";
	    row[6]  = table;
	    row[7]  = fkcol == null ? pkcol : fkcol;
	    row[8]  = Integer.toString(Integer.parseInt(seq) + 1);
	    row[9]  =
		"" + java.sql.DatabaseMetaData.importedKeyNoAction;
	    row[10] =
		"" + java.sql.DatabaseMetaData.importedKeyNoAction;
	    row[11] = null;
	    row[12] = null;
	    row[13] =
		"" + java.sql.DatabaseMetaData.importedKeyNotDeferrable;
	    out.newrow(row);
	}
    }

    public ResultSet getImportedKeys(String catalog, String schema,
				     String table) throws SQLException {
	JDBCStatement s0 = new JDBCStatement(conn);
	JDBCResultSet rs0 = null;
	try {
	    try {
		conn.db.exec("SELECT 1 FROM sqlite_master LIMIT 1", null);
	    } catch (SQLite.Exception se) {
		throw new
		    SQLException("schema reload failed: " + se.toString());
	    }
	    rs0 = (JDBCResultSet)
		(s0.executeQuery("PRAGMA foreign_key_list(" +
				 SQLite.Shell.sql_quote(table) + ")"));
	} catch (SQLException e) {
	    throw e;
	} finally {
	    s0.close();
	}
	String cols[] = {
	    "PKTABLE_CAT", "PKTABLE_SCHEM", "PKTABLE_NAME",
	    "PKCOLUMN_NAME", "FKTABLE_CAT", "FKTABLE_SCHEM",
	    "FKTABLE_NAME", "FKCOLUMN_NAME", "KEY_SEQ",
	    "UPDATE_RULE", "DELETE_RULE", "FK_NAME",
	    "PK_NAME", "DEFERRABILITY"
	};
	int types[] = {
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.VARCHAR, Types.SMALLINT,
	    Types.SMALLINT, Types.SMALLINT, Types.VARCHAR,
	    Types.VARCHAR, Types.SMALLINT
	};
	TableResultX tr = new TableResultX();
	tr.columns(cols);
	tr.sql_types(types);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	if (rs0 != null && rs0.tr != null && rs0.tr.nrows > 0) {
	    internalImportedKeys(table, null, rs0, tr);
	}
	return rs;
    }

    public ResultSet getExportedKeys(String catalog, String schema,
				     String table) throws SQLException {
	String cols[] = {
	    "PKTABLE_CAT", "PKTABLE_SCHEM", "PKTABLE_NAME",
	    "PKCOLUMN_NAME", "FKTABLE_CAT", "FKTABLE_SCHEM",
	    "FKTABLE_NAME", "FKCOLUMN_NAME", "KEY_SEQ",
	    "UPDATE_RULE", "DELETE_RULE", "FK_NAME",
	    "PK_NAME", "DEFERRABILITY"
	};
	int types[] = {
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.VARCHAR, Types.SMALLINT,
	    Types.SMALLINT, Types.SMALLINT, Types.VARCHAR,
	    Types.VARCHAR, Types.SMALLINT
	};
	TableResultX tr = new TableResultX();
	tr.columns(cols);
	tr.sql_types(types);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	return rs;
    }

    public ResultSet getCrossReference(String primaryCatalog,
				       String primarySchema,
				       String primaryTable,
				       String foreignCatalog,
				       String foreignSchema,
				       String foreignTable)
	throws SQLException {
	JDBCResultSet rs0 = null;
	if (foreignTable != null && foreignTable.charAt(0) != '%') {
	    JDBCStatement s0 = new JDBCStatement(conn);
	    try {
		try {
		    conn.db.exec("SELECT 1 FROM sqlite_master LIMIT 1", null);
		} catch (SQLite.Exception se) {
		    throw new
			SQLException("schema reload failed: " + se.toString());
		}
		rs0 = (JDBCResultSet)
		    (s0.executeQuery("PRAGMA foreign_key_list(" +
				     SQLite.Shell.sql_quote(foreignTable) + ")"));
	    } catch (SQLException e) {
		throw e;
	    } finally {
		s0.close();
	    }
	}
	String cols[] = {
	    "PKTABLE_CAT", "PKTABLE_SCHEM", "PKTABLE_NAME",
	    "PKCOLUMN_NAME", "FKTABLE_CAT", "FKTABLE_SCHEM",
	    "FKTABLE_NAME", "FKCOLUMN_NAME", "KEY_SEQ",
	    "UPDATE_RULE", "DELETE_RULE", "FK_NAME",
	    "PK_NAME", "DEFERRABILITY"
	};
	int types[] = {
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.VARCHAR, Types.VARCHAR, Types.SMALLINT,
	    Types.SMALLINT, Types.SMALLINT, Types.VARCHAR,
	    Types.VARCHAR, Types.SMALLINT
	};
	TableResultX tr = new TableResultX();
	tr.columns(cols);
	tr.sql_types(types);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	if (rs0 != null && rs0.tr != null && rs0.tr.nrows > 0) {
	    String pktable = null;
	    if (primaryTable != null && primaryTable.charAt(0) != '%') {
		pktable = primaryTable;
	    }
	    internalImportedKeys(foreignTable, pktable, rs0, tr);
	}
	return rs;
    }

    public ResultSet getTypeInfo() throws SQLException {
	String cols[] = {
	    "TYPE_NAME", "DATA_TYPE", "PRECISION",
	    "LITERAL_PREFIX", "LITERAL_SUFFIX", "CREATE_PARAMS",
	    "NULLABLE", "CASE_SENSITIVE", "SEARCHABLE",
	    "UNSIGNED_ATTRIBUTE", "FIXED_PREC_SCALE", "AUTO_INCREMENT",
	    "LOCAL_TYPE_NAME", "MINIMUM_SCALE", "MAXIMUM_SCALE",
	    "SQL_DATA_TYPE", "SQL_DATETIME_SUB", "NUM_PREC_RADIX"
	};
	int types[] = {
	    Types.VARCHAR, Types.SMALLINT, Types.INTEGER,
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.SMALLINT, Types.BIT, Types.SMALLINT,
	    Types.BIT, Types.BIT, Types.BIT,
	    Types.VARCHAR, Types.SMALLINT, Types.SMALLINT,
	    Types.INTEGER, Types.INTEGER, Types.INTEGER
	};
	TableResultX tr = new TableResultX();
	tr.columns(cols);
	tr.sql_types(types);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	String row1[] = {
	    "VARCHAR", "" + Types.VARCHAR, "65536",
	    "'", "'", null,
	    "" + typeNullable, "1", "" + typeSearchable,
	    "0", "0", "0",
	    null, "0", "0",
	    "0", "0", "0"
	};
	tr.newrow(row1);
	String row2[] = {
	    "INTEGER", "" + Types.INTEGER, "32",
	    null, null, null,
	    "" + typeNullable, "0", "" + typeSearchable,
	    "0", "0", "1",
	    null, "0", "0",
	    "0", "0", "2"
	};
	tr.newrow(row2);
	String row3[] = {
	    "DOUBLE", "" + Types.DOUBLE, "16",
	    null, null, null,
	    "" + typeNullable, "0", "" + typeSearchable,
	    "0", "0", "1",
	    null, "0", "0",
	    "0", "0", "10"
	};
	tr.newrow(row3);
	String row4[] = {
	    "FLOAT", "" + Types.FLOAT, "7",
	    null, null, null,
	    "" + typeNullable, "0", "" + typeSearchable,
	    "0", "0", "1",
	    null, "0", "0",
	    "0", "0", "10"
	};
	tr.newrow(row4);
	String row5[] = {
	    "SMALLINT", "" + Types.SMALLINT, "16",
	    null, null, null,
	    "" + typeNullable, "0", "" + typeSearchable,
	    "0", "0", "1",
	    null, "0", "0",
	    "0", "0", "2"
	};
	tr.newrow(row5);
	String row6[] = {
	    "BIT", "" + Types.BIT, "1",
	    null, null, null,
	    "" + typeNullable, "0", "" + typeSearchable,
	    "0", "0", "1",
	    null, "0", "0",
	    "0", "0", "2"
	};
	tr.newrow(row6);
	String row7[] = {
	    "TIMESTAMP", "" + Types.TIMESTAMP, "30",
	    null, null, null,
	    "" + typeNullable, "0", "" + typeSearchable,
	    "0", "0", "1",
	    null, "0", "0",
	    "0", "0", "0"
	};
	tr.newrow(row7);
	String row8[] = {
	    "DATE", "" + Types.DATE, "10",
	    null, null, null,
	    "" + typeNullable, "0", "" + typeSearchable,
	    "0", "0", "1",
	    null, "0", "0",
	    "0", "0", "0"
	};
	tr.newrow(row8);
	String row9[] = {
	    "TIME", "" + Types.TIME, "8",
	    null, null, null,
	    "" + typeNullable, "0", "" + typeSearchable,
	    "0", "0", "1",
	    null, "0", "0",
	    "0", "0", "0"
	};
	tr.newrow(row9);
	String row10[] = {
	    "BINARY", "" + Types.BINARY, "65536",
	    null, null, null,
	    "" + typeNullable, "0", "" + typeSearchable,
	    "0", "0", "1",
	    null, "0", "0",
	    "0", "0", "0"
	};
	tr.newrow(row10);
	String row11[] = {
	    "VARBINARY", "" + Types.VARBINARY, "65536",
	    null, null, null,
	    "" + typeNullable, "0", "" + typeSearchable,
	    "0", "0", "1",
	    null, "0", "0",
	    "0", "0", "0"
	};
	tr.newrow(row11);
	String row12[] = {
	    "REAL", "" + Types.REAL, "16",
	    null, null, null,
	    "" + typeNullable, "0", "" + typeSearchable,
	    "0", "0", "1",
	    null, "0", "0",
	    "0", "0", "10"
	};
	tr.newrow(row12);
	return rs;
    }

    public ResultSet getIndexInfo(String catalog, String schema, String table,
				  boolean unique, boolean approximate)
	throws SQLException {
	JDBCStatement s0 = new JDBCStatement(conn);
	JDBCResultSet rs0 = null;
	try {
	    try {
		conn.db.exec("SELECT 1 FROM sqlite_master LIMIT 1", null);
	    } catch (SQLite.Exception se) {
		throw new
		    SQLException("schema reload failed: " + se.toString());
	    }
	    rs0 = (JDBCResultSet)
		(s0.executeQuery("PRAGMA index_list(" +
				 SQLite.Shell.sql_quote(table) + ")"));
	} catch (SQLException e) {
	    throw e;
	} finally {
	    s0.close();
	}
	String cols[] = {
	    "TABLE_CAT", "TABLE_SCHEM", "TABLE_NAME",
	    "NON_UNIQUE", "INDEX_QUALIFIER", "INDEX_NAME",
	    "TYPE", "ORDINAL_POSITION", "COLUMN_NAME",
	    "ASC_OR_DESC", "CARDINALITY", "PAGES",
	    "FILTER_CONDITION"
	};
	int types[] = {
	    Types.VARCHAR, Types.VARCHAR, Types.VARCHAR,
	    Types.BIT, Types.VARCHAR, Types.VARCHAR,
	    Types.SMALLINT, Types.SMALLINT, Types.VARCHAR,
	    Types.VARCHAR, Types.INTEGER, Types.INTEGER,
	    Types.VARCHAR
	};
	TableResultX tr = new TableResultX();
	tr.columns(cols);
	tr.sql_types(types);
	JDBCResultSet rs = new JDBCResultSet(tr, null);
	if (rs0 != null && rs0.tr != null && rs0.tr.nrows > 0) {
	    Hashtable h0 = new Hashtable();
	    for (int i = 0; i < rs0.tr.ncolumns; i++) {
		h0.put(rs0.tr.column[i], new Integer(i));
	    }
	    for (int i = 0; i < rs0.tr.nrows; i++) {
		String r0[] = (String [])(rs0.tr.rows.elementAt(i));
		int col = ((Integer) h0.get("unique")).intValue();
		String uniq = r0[col];
		col = ((Integer) h0.get("name")).intValue();
		String iname = r0[col];
		if (unique && uniq.charAt(0) == '0') {
		    continue;
		}
		JDBCStatement s1 = new JDBCStatement(conn);
		JDBCResultSet rs1 = null;
		try {
		    rs1 = (JDBCResultSet)
			(s1.executeQuery("PRAGMA index_info(" +
					 SQLite.Shell.sql_quote(iname) + ")"));
		} catch (SQLException e) {
		} finally {
		    s1.close();
		}
		if (rs1 == null || rs1.tr == null || rs1.tr.nrows <= 0) {
		    continue;
		}
		Hashtable h1 = new Hashtable();
		for (int k = 0; k < rs1.tr.ncolumns; k++) {
		    h1.put(rs1.tr.column[k], new Integer(k));
		}
		for (int k = 0; k < rs1.tr.nrows; k++) {
		    String r1[] = (String [])(rs1.tr.rows.elementAt(k));
		    String row[] = new String[cols.length];
		    row[0]  = "";
		    row[1]  = "";
		    row[2]  = table;
		    row[3]  = (uniq.charAt(0) != '0' ||
			(iname.charAt(0) == '(' &&
			 iname.indexOf(" autoindex ") > 0)) ? "0" : "1";
		    row[4]  = "";
		    row[5]  = iname;
		    row[6]  = "" + tableIndexOther;
		    col = ((Integer) h1.get("seqno")).intValue();
		    row[7]  = Integer.toString(Integer.parseInt(r1[col]) + 1);
		    col = ((Integer) h1.get("name")).intValue();
		    row[8]  = r1[col];
		    row[9]  = "A";
		    row[10] = "0";
		    row[11] = "0";
		    row[12] = null;
		    tr.newrow(row);
		}
	    }
	}
	return rs;
    }

    public boolean supportsResultSetType(int type) throws SQLException {
	return type == ResultSet.TYPE_FORWARD_ONLY ||
	    type == ResultSet.TYPE_SCROLL_INSENSITIVE ||
	    type == ResultSet.TYPE_SCROLL_SENSITIVE;
    }

    public boolean supportsResultSetConcurrency(int type, int concurrency)
	throws SQLException {
	if (type == ResultSet.TYPE_FORWARD_ONLY ||
	    type == ResultSet.TYPE_SCROLL_INSENSITIVE ||
	    type == ResultSet.TYPE_SCROLL_SENSITIVE) {
	    return concurrency == ResultSet.CONCUR_READ_ONLY ||
		concurrency == ResultSet.CONCUR_UPDATABLE;
	}
	return false;
    }

    public boolean ownUpdatesAreVisible(int type) throws SQLException {
	if (type == ResultSet.TYPE_FORWARD_ONLY ||
	    type == ResultSet.TYPE_SCROLL_INSENSITIVE ||
	    type == ResultSet.TYPE_SCROLL_SENSITIVE) {
	    return true;
	}
	return false;
    }

    public boolean ownDeletesAreVisible(int type) throws SQLException {
	if (type == ResultSet.TYPE_FORWARD_ONLY ||
	    type == ResultSet.TYPE_SCROLL_INSENSITIVE ||
	    type == ResultSet.TYPE_SCROLL_SENSITIVE) {
	    return true;
	}
	return false;
    }

    public boolean ownInsertsAreVisible(int type) throws SQLException {
	if (type == ResultSet.TYPE_FORWARD_ONLY ||
	    type == ResultSet.TYPE_SCROLL_INSENSITIVE ||
	    type == ResultSet.TYPE_SCROLL_SENSITIVE) {
	    return true;
	}
	return false;
    }

    public boolean othersUpdatesAreVisible(int type) throws SQLException {
	return false;
    }

    public boolean othersDeletesAreVisible(int type) throws SQLException {
	return false;
    }

    public boolean othersInsertsAreVisible(int type) throws SQLException {
	return false;
    }

    public boolean updatesAreDetected(int type) throws SQLException {
	return false;
    }

    public boolean deletesAreDetected(int type) throws SQLException {
	return false;
    }

    public boolean insertsAreDetected(int type) throws SQLException {
	return false;
    }

    public boolean supportsBatchUpdates() throws SQLException {
	return true;
    }

    public ResultSet getUDTs(String catalog, String schemaPattern, 
		      String typeNamePattern, int[] types) 
	throws SQLException {
	return null;
    }

    public Connection getConnection() throws SQLException {
	return conn;
    }

    static String mapTypeName(int type) {
	switch (type) {
	case Types.INTEGER:	return "integer";
	case Types.SMALLINT:	return "smallint";
	case Types.FLOAT:	return "float";
	case Types.DOUBLE:	return "double";
	case Types.TIMESTAMP:	return "timestamp";
	case Types.DATE:	return "date";
	case Types.TIME:	return "time";
	case Types.BINARY:	return "binary";
	case Types.VARBINARY:	return "varbinary";
	case Types.REAL:	return "real";
	}
	return "varchar";
    }

    static int mapSqlType(String type) {
	if (type == null) {
	    return Types.VARCHAR;
	}
	type = type.toLowerCase();
	if (type.startsWith("inter")) {
	    return Types.VARCHAR;
	}
	if (type.startsWith("numeric") ||
	    type.startsWith("int")) {
	    return Types.INTEGER;
	}
	if (type.startsWith("tinyint") ||
	    type.startsWith("smallint")) {
	    return Types.SMALLINT;
	}
	if (type.startsWith("float")) {
	    return Types.FLOAT;
	}
	if (type.startsWith("double")) {
	    return Types.DOUBLE;
	}
	if (type.startsWith("datetime") ||
	    type.startsWith("timestamp")) {
	    return Types.TIMESTAMP;
	}
	if (type.startsWith("date")) {
	    return Types.DATE;
	}
	if (type.startsWith("time")) {
	    return Types.TIME;
	}
	if (type.startsWith("blob")) {
	    return Types.BINARY;
	}
	if (type.startsWith("binary")) {
	    return Types.BINARY;
	}
	if (type.startsWith("varbinary")) {
	    return Types.VARBINARY;
	}
	if (type.startsWith("real")) {
	    return Types.REAL;
	}
	return Types.VARCHAR;
    }

    static int getM(String typeStr, int type) {
	int m = 65536;
	switch (type) {
	case Types.INTEGER:	m = 11; break;
	case Types.SMALLINT:	m = 6;  break;
	case Types.FLOAT:	m = 25; break;
	case Types.REAL:
	case Types.DOUBLE:	m = 54; break;
	case Types.TIMESTAMP:	return 30;
	case Types.DATE:	return 10;
	case Types.TIME:	return 8;
	}
	typeStr = typeStr.toLowerCase();
	int i1 = typeStr.indexOf('(');
	if (i1 > 0) {
	    ++i1;
	    int i2 = typeStr.indexOf(',', i1);
	    if (i2 < 0) {
		i2 = typeStr.indexOf(')', i1);
	    }
	    if (i2 - i1 > 0) {
		String num = typeStr.substring(i1, i2);
		try {
		    m = java.lang.Integer.parseInt(num, 10);
		} catch (NumberFormatException e) {
		}
	    }
	}
	return m;
    }

    static int getD(String typeStr, int type) {
	int d = 0;
	switch (type) {
	case Types.INTEGER:	d = 10; break;
	case Types.SMALLINT:	d = 5;  break;
	case Types.FLOAT:	d = 24; break;
	case Types.REAL:
	case Types.DOUBLE:	d = 53; break;
	default:		return getM(typeStr, type);
	}
	typeStr = typeStr.toLowerCase();
	int i1 = typeStr.indexOf('(');
	if (i1 > 0) {
	    ++i1;
	    int i2 = typeStr.indexOf(',', i1);
	    if (i2 < 0) {
		return getM(typeStr, type);
	    }
	    i1 = i2;
	    i2 = typeStr.indexOf(')', i1);
	    if (i2 - i1 > 0) {
		String num = typeStr.substring(i1, i2);
		try {
		    d = java.lang.Integer.parseInt(num, 10);
		} catch (NumberFormatException e) {
		}
	    }
	}
	return d;
    }

    public boolean supportsSavepoints() {
	return false;
    }

    public boolean supportsNamedParameters() {
	return false;
    }

    public boolean supportsMultipleOpenResults() {
	return false;
    }

    public boolean supportsGetGeneratedKeys() {
	return false;
    }

    public boolean supportsResultSetHoldability(int x) {
	return false;
    }

    public boolean supportsStatementPooling() {
	return false;
    }

    public boolean locatorsUpdateCopy() throws SQLException {
	throw new SQLException("not supported");
    }

    public ResultSet getSuperTypes(String catalog, String schemaPattern,
			    String typeNamePattern)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public ResultSet getSuperTables(String catalog, String schemaPattern,
				    String tableNamePattern)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public ResultSet getAttributes(String catalog, String schemaPattern,
				   String typeNamePattern,
				   String attributeNamePattern)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public int getResultSetHoldability() throws SQLException {
	return ResultSet.HOLD_CURSORS_OVER_COMMIT;
    }

    public int getDatabaseMajorVersion() {
	return SQLite.JDBC.MAJORVERSION;
    }

    public int getDatabaseMinorVersion() {
	return SQLite.Constants.drv_minor;
    }

    public int getJDBCMajorVersion() {
	return 1;
    }
    
    public int getJDBCMinorVersion() {
	return 0;
    }

    public int getSQLStateType() throws SQLException {
	return sqlStateXOpen;
    }

}
