package SQLite.JDBC1;

import java.sql.*;

public class JDBCResultSetMetaData implements java.sql.ResultSetMetaData {

    private JDBCResultSet r;
	
    public JDBCResultSetMetaData(JDBCResultSet r) {
	this.r = r;
    }
 
    public String getCatalogName(int column) throws java.sql.SQLException {
	return null;
    }

    public String getColumnClassName(int column) throws java.sql.SQLException {
	column--;
	if (r != null && r.tr != null) {
	    if (column < 0 || column >= r.tr.ncolumns) {
		return null;
	    }
	    if (r.tr instanceof TableResultX) {
		switch (((TableResultX) r.tr).sql_type[column]) {
		case Types.SMALLINT:	return "java.lang.Short";
		case Types.INTEGER:	return "java.lang.Integer";
		case Types.REAL:
		case Types.DOUBLE:	return "java.lang.Double";
		case Types.FLOAT:	return "java.lang.Float";
		case Types.BIGINT:	return "java.lang.Long";
		case Types.DATE:	return "java.sql.Date";
		case Types.TIME:	return "java.sql.Time";
		case Types.TIMESTAMP:	return "java.sql.Timestamp";
		case Types.BINARY:
		case Types.VARBINARY:	return "[B";
		/* defaults to varchar below */
		}
	    }
	    return "java.lang.String";
	}
	return null;
    }

    public int getColumnCount() throws java.sql.SQLException {
	if (r != null && r.tr != null) {
	    return r.tr.ncolumns;
	}
	return 0;
    }

    public int getColumnDisplaySize(int column) throws java.sql.SQLException {
	return 0;
    }

    public String getColumnLabel(int column) throws java.sql.SQLException {
	column--;
	String c = null;
	if (r != null && r.tr != null) {
	    if (column < 0 || column >= r.tr.ncolumns) {
		return c;
	    }
	    c = r.tr.column[column];
	}
	return c;
    }

    public String getColumnName(int column) throws java.sql.SQLException {
	column--;
	String c = null;
	if (r != null && r.tr != null) {
	    if (column < 0 || column >= r.tr.ncolumns) {
		return c;
	    }
	    c = r.tr.column[column];
	    if (c != null) {
		int i = c.indexOf('.');
		if (i > 0) {
		    return c.substring(i + 1);
		}
	    }
	}
	return c;
    }

    public int getColumnType(int column) throws java.sql.SQLException {
	column--;
	if (r != null && r.tr != null) {
	    if (column >= 0 && column < r.tr.ncolumns) {
		if (r.tr instanceof TableResultX) {
		    return ((TableResultX) r.tr).sql_type[column];
		}
		return Types.VARCHAR;
	    }
	}
	throw new SQLException("bad column index");
    }

    public String getColumnTypeName(int column) throws java.sql.SQLException {
	column--;
	if (r != null && r.tr != null) {
	    if (column >= 0 && column < r.tr.ncolumns) {
		if (r.tr instanceof TableResultX) {
		    switch (((TableResultX) r.tr).sql_type[column]) {
		    case Types.SMALLINT:	return "smallint";
		    case Types.INTEGER:		return "integer";
		    case Types.DOUBLE:		return "double";
		    case Types.FLOAT:		return "float";
		    case Types.BIGINT:		return "bigint";
		    case Types.DATE:		return "date";
		    case Types.TIME:		return "time";
		    case Types.TIMESTAMP:	return "timestamp";
		    case Types.BINARY:		return "binary";
		    case Types.VARBINARY:	return "varbinary";
		    case Types.REAL:		return "real";
		    /* defaults to varchar below */
		    }
		}
		return "varchar";
	    }
	}
	throw new SQLException("bad column index");
    }

    public int getPrecision(int column) throws java.sql.SQLException {
	return 0;
    }

    public int getScale(int column) throws java.sql.SQLException {
	return 0;
    }

    public String getSchemaName(int column) throws java.sql.SQLException {
	return null;
    }

    public String getTableName(int column) throws java.sql.SQLException {
	column--;
	String c = null;
	if (r != null && r.tr != null) {
	    if (column < 0 || column >= r.tr.ncolumns) {
		return c;
	    }
	    c = r.tr.column[column];
	    if (c != null) {
		int i = c.indexOf('.');
		if (i > 0) {
		    return c.substring(0, i);
		}
		c = null;
	    }
	}
	return c;
    }

    public boolean isAutoIncrement(int column) throws java.sql.SQLException {
	return false;
    }

    public boolean isCaseSensitive(int column) throws java.sql.SQLException {
	return false;
    }

    public boolean isCurrency(int column) throws java.sql.SQLException {
	return false;
    }

    public boolean isDefinitelyWritable(int column) 
	throws java.sql.SQLException {
	return true;
    }

    public int isNullable(int column) throws java.sql.SQLException {
	return columnNullableUnknown;
    }

    public boolean isReadOnly(int column) throws java.sql.SQLException {
	return false;
    }

    public boolean isSearchable(int column) throws java.sql.SQLException {
	return true;
    }

    public boolean isSigned(int column) throws java.sql.SQLException {
	return false;
    }

    public boolean isWritable(int column) throws java.sql.SQLException {
	return true;
    }

    int findColByName(String columnName) throws java.sql.SQLException {
	String c = null;
	if (r != null && r.tr != null) {
	    for (int i = 0; i < r.tr.ncolumns; i++) {
		c = r.tr.column[i];
		if (c != null) {
		    if (c.equalsIgnoreCase(columnName)) {
			return i + 1;
		    }
		    int k = c.indexOf('.');
		    if (k > 0) {
			c = c.substring(k + 1);
			if (c.equalsIgnoreCase(columnName)) {
			    return i + 1;
			}
		    }
		}
		c = null;
	    }
	}
	throw new SQLException("column " + columnName + " not found");
    }
}
