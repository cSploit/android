package SQLite.JDBC2;

import java.sql.*;
import java.math.BigDecimal;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.sql.Statement;
import java.sql.Types;

public class JDBCResultSet implements java.sql.ResultSet {

    /**
     * Current row to be retrieved.
     */
    private int row;

    /**
     * Table returned by Database.get_table()
     */
    protected SQLite.TableResult tr;

    /**
     * Statement from which result set was produced.
     */
    private JDBCStatement s;

    /**
     * Meta data for result set or null.
     */
    private JDBCResultSetMetaData md;

    /**
     * Last result cell retrieved or null.
     */
    private String lastg;

    /**
     * Updatability of this result set.
     */
    private int updatable;

    /**
     * When updatable this is the table name.
     */
    private String uptable;

    /**
     * When updatable these are the PK column names of uptable.
     */
    private String pkcols[];

    /**
     * When updatable these are the PK column indices (0-based) of uptable.
     */
    private int pkcoli[];

    /*
     * Constants to reflect updateability.
     */
    private final static int UPD_UNKNOWN = -1;
    private final static int UPD_NO = 0;
    private final static int UPD_INS = 1;
    private final static int UPD_INSUPDDEL = 2;

    /**
     * Flag for cursor being (not) on insert row.
     */
    private boolean oninsrow;

    /**
     * Row buffer for insert/update row.
     */
    private String rowbuf[];

    private static final boolean nullrepl =
        SQLite.Database.version().compareTo("2.5.0") < 0;

    public JDBCResultSet(SQLite.TableResult tr, JDBCStatement s) {
	this.tr = tr;
	this.s = s;
	this.md = null;
	this.lastg = null;
	this.row = -1;
	this.updatable = UPD_UNKNOWN;
	this.oninsrow = false;
	this.rowbuf = null;
    }

    public boolean isUpdatable() throws SQLException {
	if (updatable == UPD_UNKNOWN) {
	    try {
		JDBCResultSetMetaData m =
		    (JDBCResultSetMetaData) getMetaData();
		java.util.HashSet h = new java.util.HashSet();
		String lastt = null;
		for (int i = 1; i <= tr.ncolumns; i++) {
		    lastt = m.getTableName(i);
		    h.add(lastt);
		}
		if (h.size() > 1 || lastt == null) {
		    updatable = UPD_NO;
		    throw new SQLException("view or join");
		}
		updatable = UPD_INS;
		uptable = lastt;
		JDBCResultSet pk = (JDBCResultSet)
		    s.conn.getMetaData().getPrimaryKeys(null, null, uptable);
		if (pk.tr.nrows > 0) {
		    boolean colnotfound = false;
		    pkcols = new String[pk.tr.nrows];
		    pkcoli = new int[pk.tr.nrows];
		    for (int i = 0; i < pk.tr.nrows; i++) {
			String rd[] = (String []) pk.tr.rows.elementAt(i);
			pkcols[i] = rd[3];
			try {
			    pkcoli[i] = findColumn(pkcols[i]) - 1;
			} catch (SQLException ee) {
			    colnotfound = true;
			}
		    }
		    if (!colnotfound) {
			updatable = UPD_INSUPDDEL;
		    }
		}
		pk.close();
	    } catch (SQLException e) {
		updatable = UPD_NO;
	    }
	}
	if (updatable < UPD_INS) {
	    throw new SQLException("result set not updatable");
	}
	return true;
    }

    public void fillRowbuf() throws SQLException {
	if (rowbuf == null) {
	    if (row < 0) {
		throw new SQLException("cursor outside of result set");
	    }
	    rowbuf = new String[tr.ncolumns];
	    System.arraycopy(tr.rows.elementAt(row), 0,
			     rowbuf, 0, tr.ncolumns);
	}
    }

    public boolean next() throws SQLException {
	if (tr == null) {
	    return false;
	}
	row++;
	return row < tr.nrows;
    }

    public int findColumn(String columnName) throws SQLException {
	JDBCResultSetMetaData m = (JDBCResultSetMetaData) getMetaData();
	return m.findColByName(columnName);
    }
  
    public int getRow() throws SQLException {
	if (tr == null) {
	    throw new SQLException("no rows");
	}
	return row + 1;
    }

    public boolean previous() throws SQLException {
	if (tr == null) {
	    throw new SQLException("result set already closed");
	}
	if (row >= 0) {
	    row--;
	}
	return row >= 0;
    }

    public boolean absolute(int row) throws SQLException {
	if (tr == null) {
	    return false;
	}
	if (row < 0) {
	    row = tr.nrows + 1 + row;
	}
	row--;
	if (row < 0 || row > tr.nrows) {
	    return false;
	}
	this.row = row;
	return true;
    }

    public boolean relative(int row) throws SQLException {
	if (tr == null) {
	    return false;
	}
	if (this.row + row < 0 || this.row + row >= tr.nrows) {
	    return false;
	}
	this.row += row;
	return true;
    }

    public void setFetchDirection(int dir) throws SQLException {
	if (dir != ResultSet.FETCH_FORWARD) {
	    throw new SQLException("not supported");
	}
    }

    public int getFetchDirection() throws SQLException {
	return ResultSet.FETCH_FORWARD;
    }

    public void setFetchSize(int fsize) throws SQLException {
	if (fsize != 1) {
	    throw new SQLException("not supported");
	}
    }

    public int getFetchSize() throws SQLException {
	return 1;
    }

    public String getString(int columnIndex) throws SQLException {
	if (tr == null || columnIndex < 1 || columnIndex > tr.ncolumns) {
	    throw new SQLException("column " + columnIndex + " not found");
	}
	String rd[] = (String []) tr.rows.elementAt(row);
	lastg = rd[columnIndex - 1];
	return lastg;
    }

    public String getString(String columnName) throws SQLException {
	int col = findColumn(columnName);
	return getString(col);
    }

    public int getInt(int columnIndex) throws SQLException {
	Integer i = internalGetInt(columnIndex);
	if (i != null) {
	    return i.intValue();
	}
	return 0;
    }

    private Integer internalGetInt(int columnIndex) throws SQLException {
	if (tr == null || columnIndex < 1 || columnIndex > tr.ncolumns) {
	    throw new SQLException("column " + columnIndex + " not found");
	}
	String rd[] = (String []) tr.rows.elementAt(row);
	lastg = rd[columnIndex - 1];
	try {
	    return Integer.valueOf(lastg);
	} catch (java.lang.Exception e) {
	    lastg = null;
	}
	return null;
    }

    public int getInt(String columnName) throws SQLException {
	int col = findColumn(columnName);
	return getInt(col);
    }

    public boolean getBoolean(int columnIndex) throws SQLException {
	return getInt(columnIndex) == 1 ||
	    Boolean.valueOf(getString(columnIndex)).booleanValue();
    }

    public boolean getBoolean(String columnName) throws SQLException {
	int col = findColumn(columnName);
	return getBoolean(col);
    }

    public ResultSetMetaData getMetaData() throws SQLException {
	if (md == null) {
	    md = new JDBCResultSetMetaData(this);
	}
	return md;
    }

    public short getShort(int columnIndex) throws SQLException {
	Short sh = internalGetShort(columnIndex);
	if (sh != null) {
	    return sh.shortValue();
	}
	return 0;
    }

    private Short internalGetShort(int columnIndex) throws SQLException {
	if (tr == null || columnIndex < 1 || columnIndex > tr.ncolumns) {
	    throw new SQLException("column " + columnIndex + " not found");
	}
	String rd[] = (String []) tr.rows.elementAt(row);
	lastg = rd[columnIndex - 1];
	try {
	    return Short.valueOf(lastg);
	} catch (java.lang.Exception e) {
	    lastg = null;
	}
	return null;
    }

    public short getShort(String columnName) throws SQLException {
	int col = findColumn(columnName);
	return getShort(col);
    }

    public java.sql.Time getTime(int columnIndex) throws SQLException {
	return internalGetTime(columnIndex, null);
    }

    private java.sql.Time internalGetTime(int columnIndex,
					  java.util.Calendar cal)
	throws SQLException {
	if (tr == null || columnIndex < 1 || columnIndex > tr.ncolumns) {
	    throw new SQLException("column " + columnIndex + " not found");
	}
	String rd[] = (String []) tr.rows.elementAt(row);
	lastg = rd[columnIndex - 1];
	try {
	    if (s.conn.useJulian) {
		try {
		    return new java.sql.Time(SQLite.Database.long_from_julian(lastg));
		} catch (java.lang.Exception ee) {
		    return java.sql.Time.valueOf(lastg);
		}
	    } else {
		try {
		    return java.sql.Time.valueOf(lastg);
		} catch (java.lang.Exception ee) {
		    return new java.sql.Time(SQLite.Database.long_from_julian(lastg));
		}
	    }
	} catch (java.lang.Exception e) {
	    lastg = null;
	}
	return null;
    }

    public java.sql.Time getTime(String columnName) throws SQLException {
	int col = findColumn(columnName);
	return getTime(col);
    }

    public java.sql.Time getTime(int columnIndex, java.util.Calendar cal)
	throws SQLException {
	return internalGetTime(columnIndex, cal);
    }

    public java.sql.Time getTime(String columnName, java.util.Calendar cal)
	throws SQLException{
	int col = findColumn(columnName);
	return getTime(col, cal);
    }

    public java.sql.Timestamp getTimestamp(int columnIndex)
	throws SQLException{
	return internalGetTimestamp(columnIndex, null);
    }

    private java.sql.Timestamp internalGetTimestamp(int columnIndex,
						    java.util.Calendar cal)
	throws SQLException {
	if (tr == null || columnIndex < 1 || columnIndex > tr.ncolumns) {
	    throw new SQLException("column " + columnIndex + " not found");
	}
	String rd[] = (String []) tr.rows.elementAt(row);
	lastg = rd[columnIndex - 1];
	try {
	    if (s.conn.useJulian) {
		try {
		    return new
			java.sql.Timestamp(SQLite.Database.long_from_julian(lastg));
		} catch (java.lang.Exception ee) {
		    return java.sql.Timestamp.valueOf(lastg);
		}
	    } else {
		try {
		    return java.sql.Timestamp.valueOf(lastg);
		} catch (java.lang.Exception ee) {
		    return new
			java.sql.Timestamp(SQLite.Database.long_from_julian(lastg));
		}
	    }
	} catch (java.lang.Exception e) {
	    lastg = null;
	}
	return null;
    }

    public java.sql.Timestamp getTimestamp(String columnName)
	throws SQLException{
	int col = findColumn(columnName);
	return getTimestamp(col);
    }

    public java.sql.Timestamp getTimestamp(int columnIndex,
					   java.util.Calendar cal)
	throws SQLException {
	return internalGetTimestamp(columnIndex, cal);
    }

    public java.sql.Timestamp getTimestamp(String columnName,
					   java.util.Calendar cal)
	throws SQLException {
	int col = findColumn(columnName);
	return getTimestamp(col, cal);
    }

    public java.sql.Date getDate(int columnIndex) throws SQLException {
	return internalGetDate(columnIndex, null);
    }

    private java.sql.Date internalGetDate(int columnIndex,
					  java.util.Calendar cal)
	throws SQLException {
	if (tr == null || columnIndex < 1 || columnIndex > tr.ncolumns) {
	    throw new SQLException("column " + columnIndex + " not found");
	}
	String rd[] = (String []) tr.rows.elementAt(row);
	lastg = rd[columnIndex - 1];
	try {
	    if (s.conn.useJulian) {
		try {
		    return new java.sql.Date(SQLite.Database.long_from_julian(lastg));
		} catch (java.lang.Exception ee) {
		    return java.sql.Date.valueOf(lastg);
		}
	    } else {
		try {
		    return java.sql.Date.valueOf(lastg);
		} catch (java.lang.Exception ee) {
		    return new java.sql.Date(SQLite.Database.long_from_julian(lastg));
		}
	    }
	} catch (java.lang.Exception e) {
	    lastg = null;
	}
	return null;
    }

    public java.sql.Date getDate(String columnName) throws SQLException {
	int col = findColumn(columnName);
	return getDate(col);
    }

    public java.sql.Date getDate(int columnIndex, java.util.Calendar cal)
	throws SQLException{
	return internalGetDate(columnIndex, cal);
    }

    public java.sql.Date getDate(String columnName, java.util.Calendar cal)
	throws SQLException{
	int col = findColumn(columnName);
	return getDate(col, cal);
    }

    public double getDouble(int columnIndex) throws SQLException {
	Double d = internalGetDouble(columnIndex);
	if (d != null) {
	    return d.doubleValue();
	}
	return 0;
    }

    private Double internalGetDouble(int columnIndex) throws SQLException {
	if (tr == null || columnIndex < 1 || columnIndex > tr.ncolumns) {
	    throw new SQLException("column " + columnIndex + " not found");
	}
	String rd[] = (String []) tr.rows.elementAt(row);
	lastg = rd[columnIndex - 1];
	try {
	    return Double.valueOf(lastg);
	} catch (java.lang.Exception e) {
	    lastg = null;
	}
	return null;
    }
    
    public double getDouble(String columnName) throws SQLException {
	int col = findColumn(columnName);
	return getDouble(col);
    }

    public float getFloat(int columnIndex) throws SQLException {
	Float f = internalGetFloat(columnIndex);
	if (f != null) {
	    return f.floatValue();
	}
	return 0;
    }

    private Float internalGetFloat(int columnIndex) throws SQLException {
	if (tr == null || columnIndex < 1 || columnIndex > tr.ncolumns) {
	    throw new SQLException("column " + columnIndex + " not found");
	}
	String rd[] = (String []) tr.rows.elementAt(row);
	lastg = rd[columnIndex - 1];
	try {
	    return Float.valueOf(lastg);
	} catch (java.lang.Exception e) {
	    lastg = null;
	}
	return null;
    }

    public float getFloat(String columnName) throws SQLException {
	int col = findColumn(columnName);
	return getFloat(col);
    }

    public long getLong(int columnIndex) throws SQLException {
	Long l = internalGetLong(columnIndex);
	if (l != null) {
	    return l.longValue();
	}
	return 0;
    }

    private Long internalGetLong(int columnIndex) throws SQLException {
	if (tr == null || columnIndex < 1 || columnIndex > tr.ncolumns) {
	    throw new SQLException("column " + columnIndex + " not found");
	}
	String rd[] = (String []) tr.rows.elementAt(row);
	lastg = rd[columnIndex - 1];
	try {
	    return Long.valueOf(lastg);
	} catch (java.lang.Exception e) {
	    lastg = null;
	}
	return null;
    }

    public long getLong(String columnName) throws SQLException {
	int col = findColumn(columnName);
	return getLong(col);
    }

    public java.io.InputStream getUnicodeStream(int columnIndex)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.io.InputStream getUnicodeStream(String columnName)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.io.InputStream getAsciiStream(String columnName)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.io.InputStream getAsciiStream(int columnIndex)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public BigDecimal getBigDecimal(String columnName)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public BigDecimal getBigDecimal(String columnName, int scale)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public BigDecimal getBigDecimal(int columnIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public BigDecimal getBigDecimal(int columnIndex, int scale)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.io.InputStream getBinaryStream(int columnIndex)
	throws SQLException {
	byte data[] = getBytes(columnIndex);
	if (data != null) {
	    return new java.io.ByteArrayInputStream(data);
	}
	return null;
    }

    public java.io.InputStream getBinaryStream(String columnName)
	throws SQLException {
	byte data[] = getBytes(columnName);
	if (data != null) {
	    return new java.io.ByteArrayInputStream(data);
	}
	return null;
    }

    public byte getByte(int columnIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public byte getByte(String columnName) throws SQLException {
	throw new SQLException("not supported");
    }

    public byte[] getBytes(int columnIndex) throws SQLException {
	if (tr == null || columnIndex < 1 || columnIndex > tr.ncolumns) {
	    throw new SQLException("column " + columnIndex + " not found");
	}
	byte ret[] = null;
	String rd[] = (String []) tr.rows.elementAt(row);
	lastg = rd[columnIndex - 1];
	if (lastg != null) {
	    ret = SQLite.StringEncoder.decode(lastg);
	}
	return ret;
    }

    public byte[] getBytes(String columnName) throws SQLException {
	int col = findColumn(columnName);
	return getBytes(col);
    }

    public String getCursorName() throws SQLException {
	return null;
    }

    public Object getObject(int columnIndex) throws SQLException {
	if (tr == null || columnIndex < 1 || columnIndex > tr.ncolumns) {
	    throw new SQLException("column " + columnIndex + " not found");
	}
	String rd[] = (String []) tr.rows.elementAt(row);
	lastg = rd[columnIndex - 1];
	Object ret = lastg;
	if (tr instanceof TableResultX) {
	    switch (((TableResultX) tr).sql_type[columnIndex - 1]) {
	    case Types.SMALLINT:
		ret = internalGetShort(columnIndex);
		break;
	    case Types.INTEGER:
		ret = internalGetInt(columnIndex);
		break;
	    case Types.DOUBLE:
		ret = internalGetDouble(columnIndex);
		break;
	    case Types.FLOAT:
		ret = internalGetFloat(columnIndex);
		break;
	    case Types.BIGINT:
		ret = internalGetLong(columnIndex);
		break;
	    case Types.BINARY:
	    case Types.VARBINARY:
	    case Types.LONGVARBINARY:
		ret = getBytes(columnIndex);
		break;
	    case Types.NULL:
		ret = null;
		break;
	    /* defaults to String below */
	    }
	}
	return ret;
    }

    public Object getObject(String columnName) throws SQLException {
	int col = findColumn(columnName);
	return getObject(col);
    }

    public Object getObject(int columnIndex, java.util.Map map) 
	throws SQLException {
	throw new SQLException("not supported");
    }

    public Object getObject(String columnIndex, java.util.Map map)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Ref getRef(int columnIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Ref getRef(String columnIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Blob getBlob(int columnIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Blob getBlob(String columnIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Clob getClob(int columnIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Clob getClob(String columnIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Array getArray(int columnIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Array getArray(String columnIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.io.Reader getCharacterStream(int columnIndex)
	throws SQLException {
	String data = getString(columnIndex);
	if (data != null) {
	    char[] cdata = data.toCharArray();
	    return new java.io.CharArrayReader(cdata);
	}
	return null;
    }

    public java.io.Reader getCharacterStream(String columnName)
	throws SQLException {
	String data = getString(columnName);
	if (data != null) {
	    char[] cdata = data.toCharArray();
	    return new java.io.CharArrayReader(cdata);
	}
	return null;
    }

    public SQLWarning getWarnings() throws SQLException {
	throw new SQLException("not supported");
    }

    public boolean wasNull() throws SQLException {
	return lastg == null;
    }
	
    public void clearWarnings() throws SQLException {
	throw new SQLException("not supported");
    }

    public boolean isFirst() throws SQLException {
	if (tr == null) {
	    return true;
	}
	return row == 0;
    }

    public boolean isBeforeFirst() throws SQLException {
	if (tr == null || tr.nrows <= 0) {
	    return false;
	}
	return row < 0;
    }

    public void beforeFirst() throws SQLException {
	if (tr == null) {
	    return;
	}
	row = -1;
    }

    public boolean first() throws SQLException {
	if (tr == null || tr.nrows <= 0) {
	    return false;
	}
	row = 0;
	return true;
    }

    public boolean isAfterLast() throws SQLException {
	if (tr == null || tr.nrows <= 0) {
	    return false;
	}
	return row >= tr.nrows;
    }

    public void afterLast() throws SQLException {
	if (tr == null) {
	    return;
	}
	row = tr.nrows;
    }

    public boolean isLast() throws SQLException {
	if (tr == null) {
	    return true;
	}
	return row == tr.nrows - 1;
    }

    public boolean last() throws SQLException {
	if (tr == null || tr.nrows <= 0) {
	    return false;
	}
	row = tr.nrows -1;
	return true;
    }

    public int getType() throws SQLException {
	return TYPE_SCROLL_SENSITIVE;
    }

    public int getConcurrency() throws SQLException {
	return CONCUR_UPDATABLE;
    }

    public boolean rowUpdated() throws SQLException {
	return false;
    }

    public boolean rowInserted() throws SQLException {
	return false;
    }

    public boolean rowDeleted() throws SQLException {
	return false;
    }

    public void insertRow() throws SQLException {
	isUpdatable();
	if (!oninsrow || rowbuf == null) {
	    throw new SQLException("no insert data provided");
	}
	JDBCResultSetMetaData m = (JDBCResultSetMetaData) getMetaData();
	StringBuffer sb = new StringBuffer();
	sb.append("INSERT INTO ");
	sb.append(SQLite.Shell.sql_quote_dbl(uptable));
	sb.append("(");
	for (int i = 0; i < tr.ncolumns; i++) {
	    sb.append(SQLite.Shell.sql_quote_dbl(m.getColumnName(i + 1)));
	    if (i < tr.ncolumns - 1) {
		sb.append(",");
	    }
	}
	sb.append(") VALUES(");
	for (int i = 0; i < tr.ncolumns; i++) {
	    sb.append(nullrepl ? "'%q'" : "%Q");
	    if (i < tr.ncolumns - 1) {
		sb.append(",");
	    }
	}
	sb.append(")");
	try {
	    this.s.conn.db.exec(sb.toString(), null, rowbuf);
	} catch (SQLite.Exception e) {
	    throw new SQLException(e.getMessage());
	}
	tr.newrow(rowbuf);
	rowbuf = null;
	oninsrow = false;
	last();
    }

    public void updateRow() throws SQLException {
	isUpdatable();
	if (rowbuf == null) {
	    throw new SQLException("no update data provided");
	}
	if (oninsrow) {
	    throw new SQLException("cursor on insert row");
	}
	if (updatable < UPD_INSUPDDEL) {
	    throw new SQLException("no primary key on table defined");
	}
	String rd[] = (String []) tr.rows.elementAt(row);
	JDBCResultSetMetaData m = (JDBCResultSetMetaData) getMetaData();
	String args[] = new String[tr.ncolumns + pkcols.length];
	StringBuffer sb = new StringBuffer();
	sb.append("UPDATE ");
	sb.append(SQLite.Shell.sql_quote_dbl(uptable));
	sb.append(" SET ");
	int i;
	for (i = 0; i < tr.ncolumns; i++) {
	    sb.append(SQLite.Shell.sql_quote_dbl(m.getColumnName(i + 1)));
	    sb.append(" = " + (nullrepl ? "'%q'" : "%Q"));
	    if (i < tr.ncolumns - 1) {
		sb.append(",");
	    }
	    args[i] = rowbuf[i];
	}
	sb. append(" WHERE ");
	for (int k = 0; k < pkcols.length; k++, i++) {
	    sb.append(SQLite.Shell.sql_quote_dbl(pkcols[k]));
	    sb.append(" = " + (nullrepl ? "'%q'" : "%Q"));
	    if (k < pkcols.length - 1) {
		sb.append(" AND ");
	    }
	    args[i] = rd[pkcoli[k]];
	}
	try {
	    this.s.conn.db.exec(sb.toString(), null, args);
	} catch (SQLite.Exception e) {
	    throw new SQLException(e.getMessage());
	}
	System.arraycopy(rowbuf, 0, rd, 0, rowbuf.length);
	rowbuf = null;
    }

    public void deleteRow() throws SQLException {
	isUpdatable();
	if (oninsrow) {
	    throw new SQLException("cursor on insert row");
	}
	if (updatable < UPD_INSUPDDEL) {
	    throw new SQLException("no primary key on table defined");
	}
	fillRowbuf();
	StringBuffer sb = new StringBuffer();
	sb.append("DELETE FROM ");
	sb.append(SQLite.Shell.sql_quote_dbl(uptable));
	sb.append(" WHERE ");
	String args[] = new String[pkcols.length];
	for (int i = 0; i < pkcols.length; i++) {
	    sb.append(SQLite.Shell.sql_quote_dbl(pkcols[i]));
	    sb.append(" = " + (nullrepl ? "'%q'" : "%Q"));
	    if (i < pkcols.length - 1) {
		sb.append(" AND ");
	    }
	    args[i] = rowbuf[pkcoli[i]];
	}
	try {
	    this.s.conn.db.exec(sb.toString(), null, args);
	} catch (SQLite.Exception e) {
	    throw new SQLException(e.getMessage());
	}
	rowbuf = null;
    }

    public void refreshRow() throws SQLException {
	isUpdatable();
	if (oninsrow) {
	    throw new SQLException("cursor on insert row");
	}
	if (updatable < UPD_INSUPDDEL) {
	    throw new SQLException("no primary key on table defined");
	}
	JDBCResultSetMetaData m = (JDBCResultSetMetaData) getMetaData();
	String rd[] = (String []) tr.rows.elementAt(row);
	StringBuffer sb = new StringBuffer();
	sb.append("SELECT ");
	for (int i = 0; i < tr.ncolumns; i++) {
	    sb.append(SQLite.Shell.sql_quote_dbl(m.getColumnName(i + 1)));
	    if (i < tr.ncolumns - 1) {
		sb.append(",");
	    }
	}
	sb.append(" FROM ");
	sb.append(SQLite.Shell.sql_quote_dbl(uptable));
	sb.append(" WHERE ");
	String args[] = new String[pkcols.length];
	for (int i = 0; i < pkcols.length; i++) {
	    sb.append(SQLite.Shell.sql_quote_dbl(pkcols[i]));
	    sb.append(" = " + (nullrepl ? "'%q'" : "%Q"));
	    if (i < pkcols.length - 1) {
		sb.append(" AND ");
	    }
	    args[i] = rd[pkcoli[i]];
	}
	SQLite.TableResult trnew = null;
	try {
	    trnew = this.s.conn.db.get_table(sb.toString(), args);
	} catch (SQLite.Exception e) {
	    throw new SQLException(e.getMessage());
	}
	if (trnew.nrows != 1) {
	    throw new SQLException("wrong size of result set; expected 1, got " + trnew.nrows);
	}
	rowbuf = null;
	tr.rows.setElementAt(trnew.rows.elementAt(0), row);
    }

    public void cancelRowUpdates() throws SQLException {
	rowbuf = null;
    }

    public void moveToInsertRow() throws SQLException {
	isUpdatable();
	if (!oninsrow) {
	    oninsrow = true;
	    rowbuf = new String[tr.nrows];
	}
    }

    public void moveToCurrentRow() throws SQLException {
	if (oninsrow) {
	    oninsrow = false;
	    rowbuf = null;
	}
    }

    public void updateNull(int colIndex) throws SQLException {
	isUpdatable();
	if (tr == null || colIndex < 1 || colIndex > tr.ncolumns) {
	    throw new SQLException("column " + colIndex + " not found");
	}
	fillRowbuf();
	rowbuf[colIndex - 1] = null;
    }

    public void updateBoolean(int colIndex, boolean b) throws SQLException {
	updateString(colIndex, b ? "1" : "0");
    }

    public void updateByte(int colIndex, byte b) throws SQLException {
	throw new SQLException("not supported");
    }

    public void updateShort(int colIndex, short b) throws SQLException {
	isUpdatable();
	if (tr == null || colIndex < 1 || colIndex > tr.ncolumns) {
	    throw new SQLException("column " + colIndex + " not found");
	}
	fillRowbuf();
	rowbuf[colIndex - 1] = Short.toString(b);
    }

    public void updateInt(int colIndex, int b) throws SQLException {
	isUpdatable();
	if (tr == null || colIndex < 1 || colIndex > tr.ncolumns) {
	    throw new SQLException("column " + colIndex + " not found");
	}
	fillRowbuf();
	rowbuf[colIndex - 1] = Integer.toString(b);
    }

    public void updateLong(int colIndex, long b) throws SQLException {
	isUpdatable();
	if (tr == null || colIndex < 1 || colIndex > tr.ncolumns) {
	    throw new SQLException("column " + colIndex + " not found");
	}
	fillRowbuf();
	rowbuf[colIndex - 1] = Long.toString(b);
    }

    public void updateFloat(int colIndex, float f) throws SQLException {
	isUpdatable();
	if (tr == null || colIndex < 1 || colIndex > tr.ncolumns) {
	    throw new SQLException("column " + colIndex + " not found");
	}
	fillRowbuf();
	rowbuf[colIndex - 1] = Float.toString(f);
    }

    public void updateDouble(int colIndex, double f) throws SQLException {
	isUpdatable();
	if (tr == null || colIndex < 1 || colIndex > tr.ncolumns) {
	    throw new SQLException("column " + colIndex + " not found");
	}
	fillRowbuf();
	rowbuf[colIndex - 1] = Double.toString(f);
    }

    public void updateBigDecimal(int colIndex, BigDecimal f)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void updateString(int colIndex, String s) throws SQLException {
	isUpdatable();
	if (tr == null || colIndex < 1 || colIndex > tr.ncolumns) {
	    throw new SQLException("column " + colIndex + " not found");
	}
	fillRowbuf();
	rowbuf[colIndex - 1] = s;
    }

    public void updateBytes(int colIndex, byte[] s) throws SQLException {
	isUpdatable();
	if (tr == null || colIndex < 1 || colIndex > tr.ncolumns) {
	    throw new SQLException("column " + colIndex + " not found");
	}
	fillRowbuf();
	if (this.s.conn.db.is3()) {
	    rowbuf[colIndex - 1] = SQLite.StringEncoder.encodeX(s);
	} else {
	    rowbuf[colIndex - 1] = SQLite.StringEncoder.encode(s);
	}
    }

    public void updateDate(int colIndex, java.sql.Date d) throws SQLException {
	isUpdatable();
	if (tr == null || colIndex < 1 || colIndex > tr.ncolumns) {
	    throw new SQLException("column " + colIndex + " not found");
	}
	fillRowbuf();
	rowbuf[colIndex - 1] = d.toString();
    }

    public void updateTime(int colIndex, java.sql.Time t) throws SQLException {
	isUpdatable();
	if (tr == null || colIndex < 1 || colIndex > tr.ncolumns) {
	    throw new SQLException("column " + colIndex + " not found");
	}
	fillRowbuf();
	rowbuf[colIndex - 1] = t.toString();
    }

    public void updateTimestamp(int colIndex, java.sql.Timestamp t)
	throws SQLException {
	isUpdatable();
	if (tr == null || colIndex < 1 || colIndex > tr.ncolumns) {
	    throw new SQLException("column " + colIndex + " not found");
	}
	fillRowbuf();
	rowbuf[colIndex - 1] = t.toString();
    }

    public void updateAsciiStream(int colIndex, java.io.InputStream in, int s)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void updateBinaryStream(int colIndex, java.io.InputStream in, int s)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void updateCharacterStream(int colIndex, java.io.Reader in, int s)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void updateObject(int colIndex, Object obj) throws SQLException {
	updateString(colIndex, obj.toString());
    }

    public void updateObject(int colIndex, Object obj, int s)
	throws SQLException {
	updateString(colIndex, obj.toString());
    }

    public void updateNull(String colName) throws SQLException {
	int col = findColumn(colName);
	updateNull(col);
    }

    public void updateBoolean(String colName, boolean b) throws SQLException {
	int col = findColumn(colName);
	updateBoolean(col, b);
    }

    public void updateByte(String colName, byte b) throws SQLException {
	throw new SQLException("not supported");
    }

    public void updateShort(String colName, short b) throws SQLException {
	int col = findColumn(colName);
	updateShort(col, b);
    }

    public void updateInt(String colName, int b) throws SQLException {
	int col = findColumn(colName);
	updateInt(col, b);
    }

    public void updateLong(String colName, long b) throws SQLException {
	int col = findColumn(colName);
	updateLong(col, b);
    }

    public void updateFloat(String colName, float f) throws SQLException {
	int col = findColumn(colName);
	updateFloat(col, f);
    }

    public void updateDouble(String colName, double f) throws SQLException {
	int col = findColumn(colName);
	updateDouble(col, f);
    }

    public void updateBigDecimal(String colName, BigDecimal f)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void updateString(String colName, String s) throws SQLException {
	int col = findColumn(colName);
	updateString(col, s);
    }

    public void updateBytes(String colName, byte[] s) throws SQLException {
	int col = findColumn(colName);
	updateBytes(col, s);
    }

    public void updateDate(String colName, java.sql.Date d)
	throws SQLException {
	int col = findColumn(colName);
	updateDate(col, d);
    }

    public void updateTime(String colName, java.sql.Time t)
	throws SQLException {
	int col = findColumn(colName);
	updateTime(col, t);
    }

    public void updateTimestamp(String colName, java.sql.Timestamp t)
	throws SQLException {
	int col = findColumn(colName);
	updateTimestamp(col, t);
    }

    public void updateAsciiStream(String colName, java.io.InputStream in,
				  int s)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void updateBinaryStream(String colName, java.io.InputStream in,
				   int s)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void updateCharacterStream(String colName, java.io.Reader in,
				      int s)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void updateObject(String colName, Object obj)
	throws SQLException {
	int col = findColumn(colName);
	updateObject(col, obj);
    }

    public void updateObject(String colName, Object obj, int s)
	throws SQLException {
	int col = findColumn(colName);
	updateObject(col, obj, s);
    }

    public Statement getStatement() throws SQLException {
	if (s == null) {
	    throw new SQLException("stale result set");
	}
	return s;
    }

    public void close() throws SQLException {
	s = null;
	tr = null;
	lastg = null;
	oninsrow = false;
	rowbuf = null;
	row = -1;
    }
}
