package SQLite.JDBC2z1;

import java.sql.SQLException;
import java.sql.Types;

public class JDBCResultSetMetaData implements java.sql.ResultSetMetaData {

    private JDBCResultSet r;
	
    public JDBCResultSetMetaData(JDBCResultSet r) {
	this.r = r;
    }
 
    @Override
    public String getCatalogName(int column) throws java.sql.SQLException {
	return null;
    }

    @Override
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

    @Override
    public int getColumnCount() throws java.sql.SQLException {
	if (r != null && r.tr != null) {
	    return r.tr.ncolumns;
	}
	return 0;
    }

    @Override
    public int getColumnDisplaySize(int column) throws java.sql.SQLException {
	return 0;
    }

    @Override
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

    @Override
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

    @Override
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

    @Override
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

    @Override
    public int getPrecision(int column) throws java.sql.SQLException {
	return 0;
    }

    @Override
    public int getScale(int column) throws java.sql.SQLException {
	return 0;
    }

    @Override
    public String getSchemaName(int column) throws java.sql.SQLException {
	return null;
    }

    @Override
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

    @Override
    public boolean isAutoIncrement(int column) throws java.sql.SQLException {
	return false;
    }

    @Override
    public boolean isCaseSensitive(int column) throws java.sql.SQLException {
	return false;
    }

    @Override
    public boolean isCurrency(int column) throws java.sql.SQLException {
	return false;
    }

    @Override
    public boolean isDefinitelyWritable(int column) 
	throws java.sql.SQLException {
	return true;
    }

    @Override
    public int isNullable(int column) throws java.sql.SQLException {
	return columnNullableUnknown;
    }

    @Override
    public boolean isReadOnly(int column) throws java.sql.SQLException {
	return false;
    }

    @Override
    public boolean isSearchable(int column) throws java.sql.SQLException {
	return true;
    }

    @Override
    public boolean isSigned(int column) throws java.sql.SQLException {
	return false;
    }

    @Override
    public boolean isWritable(int column) throws java.sql.SQLException {
	return true;
    }

    int findColByName(String columnName) throws java.sql.SQLException {
	String c = null;
	if (r != null && r.tr != null) {
	    for (int i = 0; i < r.tr.ncolumns; i++) {
		c = r.tr.column[i];
		if (c != null) {
		    if (c.compareToIgnoreCase(columnName) == 0) {
			return i + 1;
		    }
		    int k = c.indexOf('.');
		    if (k > 0) {
			c = c.substring(k + 1);
			if (c.compareToIgnoreCase(columnName) == 0) {
			    return i + 1;
			}
		    }
		}
		c = null;
	    }
	}
	throw new SQLException("column " + columnName + " not found");
    }

    @Override
    public <T> T unwrap(java.lang.Class<T> iface) throws SQLException {
	throw new SQLException("unsupported");
    }

    @Override
    public boolean isWrapperFor(java.lang.Class iface) throws SQLException {
	return false;
    }

}
