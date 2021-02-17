package SQLite.JDBC2x;

import java.sql.*;
import java.math.BigDecimal;
import java.util.*;

class BatchArg {
    String arg;
    boolean blob;

    BatchArg(String arg, boolean blob) {
	if (arg == null) {
	    this.arg = null;
	} else {
	    this.arg = new String(arg);
	}
	this.blob = blob;
    }
}

public class JDBCPreparedStatement extends JDBCStatement
    implements java.sql.PreparedStatement {

    private String sql;
    private String args[];
    private boolean blobs[];
    private ArrayList batch;
    private static final boolean nullrepl =
	SQLite.Database.version().compareTo("2.5.0") < 0;

    public JDBCPreparedStatement(JDBCConnection conn, String sql) {
	super(conn);
	this.args = null;
	this.blobs = null;
	this.batch = null;
	this.sql = fixup(sql);
    }

    private String fixup(String sql) {
	StringBuffer sb = new StringBuffer(sql.length());
	boolean inq = false;
	int nparm = 0;
	int iscreate = -1;
	for (int i = 0; i < sql.length(); i++) {
	    char c = sql.charAt(i);
	    if (c == '\'') {
		if (inq) {
                    char nextChar = 0;
                    if(i + 1 < sql.length()) {
                        nextChar = sql.charAt(i + 1);
                    }
		    if (nextChar == '\'') {
                        sb.append(c);
                        sb.append(nextChar);
                        i++;
                    } else {
			inq = false;
                        sb.append(c);
                    }
		} else {
		    inq = true;
                    sb.append(c);
		}
	    } else if (c == '?') {
		if (inq) {
		    sb.append(c);
		} else {
		    ++nparm;
		    sb.append(nullrepl ? "'%q'" : "%Q");
		}
	    } else if (c == ';') {
		if (!inq) {
		    if (iscreate < 0) {
			int ii = 0;
			while (sb.charAt(ii) == ' ' ||
			       sb.charAt(ii) == '\t' ||
			       sb.charAt(ii) == '\n' ||
			       sb.charAt(ii) == '\r') {
			    ++ii;
			}
			String t = sb.substring(ii, ii + 6);
			if (t.compareToIgnoreCase("create") == 0) {
			    iscreate = 1;
			} else {
			    iscreate = 0;
			}
		    }
		    if (iscreate == 0) {
			break;
		    }
		}
		sb.append(c);
	    } else if (c == '%') {
		sb.append("%%");
	    } else {
		sb.append(c);
	    }
	}
	args = new String[nparm];
	blobs = new boolean[nparm];
	try {
	    clearParameters();
	} catch (SQLException e) {
	}
	return sb.toString();
    }

    private String fixup2(String sql) {
	if (!conn.db.is3()) {
	    return sql;
	}
	StringBuffer sb = new StringBuffer(sql.length());
	int parm = -1;
	for (int i = 0; i < sql.length(); i++) {
	    char c = sql.charAt(i);
	    if (c == '%') {
		sb.append(c);
		++i;
		c = sql.charAt(i);
		if (c == 'Q') {
		    parm++;
		    if (blobs[parm]) {
			c = 's';
		    }
		}
	    }
	    sb.append(c);
	}
	return sb.toString();
    }

    public ResultSet executeQuery() throws SQLException {
	return executeQuery(fixup2(sql), args, false);
    }

    public int executeUpdate() throws SQLException {
	executeQuery(fixup2(sql), args, true);
	return updcnt;
    }

    public void setNull(int parameterIndex, int sqlType) throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	args[parameterIndex - 1] = nullrepl ? "" : null;
	blobs[parameterIndex - 1] = false;
    }
    
    public void setBoolean(int parameterIndex, boolean x)
	throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	args[parameterIndex - 1] = x ? "1" : "0";
	blobs[parameterIndex - 1] = false;
    }

    public void setByte(int parameterIndex, byte x) throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	args[parameterIndex - 1] = "" + x;
	blobs[parameterIndex - 1] = false;
    }

    public void setShort(int parameterIndex, short x) throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	args[parameterIndex - 1] = "" + x;
	blobs[parameterIndex - 1] = false;
    }

    public void setInt(int parameterIndex, int x) throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	args[parameterIndex - 1] = "" + x;
	blobs[parameterIndex - 1] = false;
    }

    public void setLong(int parameterIndex, long x) throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	args[parameterIndex - 1] = "" + x;
	blobs[parameterIndex - 1] = false;
    }

    public void setFloat(int parameterIndex, float x) throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	args[parameterIndex - 1] = "" + x;
	blobs[parameterIndex - 1] = false;
    }

    public void setDouble(int parameterIndex, double x) throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	args[parameterIndex - 1] = "" + x;
	blobs[parameterIndex - 1] = false;
    }

    public void setBigDecimal(int parameterIndex, BigDecimal x)
	throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	if (x == null) {
	    args[parameterIndex - 1] = nullrepl ? "" : null;
	} else {
	    args[parameterIndex - 1] = "" + x;
	}
	blobs[parameterIndex - 1] = false;
    }

    public void setString(int parameterIndex, String x) throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	if (x == null) {
	    args[parameterIndex - 1] = nullrepl ? "" : null;
	} else {
	    args[parameterIndex - 1] = x;
	}
	blobs[parameterIndex - 1] = false;
    }

    public void setBytes(int parameterIndex, byte x[]) throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	blobs[parameterIndex - 1] = false;
	if (x == null) {
	    args[parameterIndex - 1] = nullrepl ? "" : null;
	} else {
	    if (conn.db.is3()) {
		args[parameterIndex - 1] = SQLite.StringEncoder.encodeX(x);
		blobs[parameterIndex - 1] = true;
	    } else {
		args[parameterIndex - 1] = SQLite.StringEncoder.encode(x);
	    }
	}
    }

    public void setDate(int parameterIndex, java.sql.Date x)
	throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	if (x == null) {
	    args[parameterIndex - 1] = nullrepl ? "" : null;
	} else {
	    if (conn.useJulian) {
		args[parameterIndex - 1] = java.lang.Double.toString(SQLite.Database.julian_from_long(x.getTime()));
	    } else {
		args[parameterIndex - 1] = x.toString();
	    }
	}
	blobs[parameterIndex - 1] = false;
    }

    public void setTime(int parameterIndex, java.sql.Time x) 
	throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	if (x == null) {
	    args[parameterIndex - 1] = nullrepl ? "" : null;
	} else {
	    if (conn.useJulian) {
		args[parameterIndex - 1] = java.lang.Double.toString(SQLite.Database.julian_from_long(x.getTime()));
	    } else {
		args[parameterIndex - 1] = x.toString();
	    }
	}
	blobs[parameterIndex - 1] = false;
    }

    public void setTimestamp(int parameterIndex, java.sql.Timestamp x)
	throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	if (x == null) {
	    args[parameterIndex - 1] = nullrepl ? "" : null;
	} else {
	    if (conn.useJulian) {
		args[parameterIndex - 1] = java.lang.Double.toString(SQLite.Database.julian_from_long(x.getTime()));
	    } else {
		args[parameterIndex - 1] = x.toString();
	    }
	}
	blobs[parameterIndex - 1] = false;
    }

    public void setAsciiStream(int parameterIndex, java.io.InputStream x,
			       int length) throws SQLException {
	throw new SQLException("not supported");
    }

    public void setUnicodeStream(int parameterIndex, java.io.InputStream x, 
				 int length) throws SQLException {
	throw new SQLException("not supported");
    }

    public void setBinaryStream(int parameterIndex, java.io.InputStream x,
				int length) throws SQLException {
	try {
	    byte[] data = new byte[length];
	    x.read(data, 0, length);
	    setBytes(parameterIndex, data);
	} catch (java.io.IOException e) {
	    throw new SQLException("I/O failed: " + e.toString());
	}
    }

    public void clearParameters() throws SQLException {
	for (int i = 0; i < args.length; i++) {
	    args[i] = nullrepl ? "" : null;
	    blobs[i] = false;
	}
    }

    public void setObject(int parameterIndex, Object x, int targetSqlType,
			  int scale) throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	if (x == null) {
	    args[parameterIndex - 1] = nullrepl ? "" : null;
	} else {
	    if (x instanceof byte[]) {
		byte[] bx = (byte[]) x;
		if (conn.db.is3()) {
		    args[parameterIndex - 1] =
			  SQLite.StringEncoder.encodeX(bx);
		    blobs[parameterIndex - 1] = true;
		    return;
		}
		args[parameterIndex - 1] = SQLite.StringEncoder.encode(bx);
	    } else {
		args[parameterIndex - 1] = x.toString();
	    }
	}
	blobs[parameterIndex - 1] = false;
    }

    public void setObject(int parameterIndex, Object x, int targetSqlType)
	throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	if (x == null) {
	    args[parameterIndex - 1] = nullrepl ? "" : null;
	} else {
	    if (x instanceof byte[]) {
		byte[] bx = (byte[]) x;
		if (conn.db.is3()) {
		    args[parameterIndex - 1] =
			SQLite.StringEncoder.encodeX(bx);
		    blobs[parameterIndex - 1] = true;
		    return;
		}
		args[parameterIndex - 1] = SQLite.StringEncoder.encode(bx);
	    } else {
		args[parameterIndex - 1] = x.toString();
	    }
	}
	blobs[parameterIndex - 1] = false;
    }

    public void setObject(int parameterIndex, Object x) throws SQLException {
	if (parameterIndex < 1 || parameterIndex > args.length) {
	    throw new SQLException("bad parameter index");
	}
	if (x == null) {
	    args[parameterIndex - 1] = nullrepl ? "" : null;
	} else {
	    if (x instanceof byte[]) {
		byte[] bx = (byte[]) x;
		if (conn.db.is3()) {
		    args[parameterIndex - 1] =
			SQLite.StringEncoder.encodeX(bx);
		    blobs[parameterIndex - 1] = true;
		    return;
		}
		args[parameterIndex - 1] = SQLite.StringEncoder.encode(bx);
	    } else {
		args[parameterIndex - 1] = x.toString();
	    }
	}
	blobs[parameterIndex - 1] = false;
    }

    public boolean execute() throws SQLException {
	return executeQuery(fixup2(sql), args, false) != null;
    }

    public void addBatch() throws SQLException {
	if (batch == null) {
	    batch = new ArrayList(args.length);
	}
	if (args.length == 0) {
	    batch.add(new BatchArg(null, false));
	} else {
	    for (int i = 0; i < args.length; i++) {
		batch.add(new BatchArg(args[i], blobs[i]));
	    }
	}
    }

    public int[] executeBatch() throws SQLException {
	if (batch == null) {
	    return new int[0];
	}
	int[] ret;
	if (args.length == 0) {
	    ret = new int[batch.size()];
	} else {
	    ret = new int[batch.size() / args.length];
	}
	for (int i = 0; i < ret.length; i++) {
	    ret[i] = EXECUTE_FAILED;
	}
	int errs = 0;
	int index = 0;
	for (int i = 0; i < ret.length; i++) {
	    for (int k = 0; k < args.length; k++) {
		BatchArg b = (BatchArg) batch.get(index++);
		args[k] = b.arg;
		blobs[k] = b.blob;
	    }
	    try {
		ret[i] = executeUpdate();
	    } catch (SQLException e) {
		++errs;
	    }
	}
	if (errs > 0) {
	    throw new BatchUpdateException("batch failed", ret);
	}
	return ret;
    }

    public void clearBatch() throws SQLException {
	if (batch != null) {
	    batch.clear();
	    batch = null;
	}
    }

    public void close() throws SQLException {
    	clearBatch();
	super.close();
    }

    public void setCharacterStream(int parameterIndex,
				   java.io.Reader reader,
				   int length) throws SQLException {
	try {
	    char[] data = new char[length];
	    reader.read(data);
	    setString(parameterIndex, new String(data));
	} catch (java.io.IOException e) {
	    throw new SQLException("I/O failed: " + e.toString());
	}
    }

    public void setRef(int i, Ref x) throws SQLException {
	throw new SQLException("not supported");
    }

    public void setBlob(int i, Blob x) throws SQLException {
	throw new SQLException("not supported");
    }

    public void setClob(int i, Clob x) throws SQLException {
	throw new SQLException("not supported");
    }

    public void setArray(int i, Array x) throws SQLException {
	throw new SQLException("not supported");
    }

    public ResultSetMetaData getMetaData() throws SQLException {
	return rs.getMetaData();
    }

    public void setDate(int parameterIndex, java.sql.Date x, Calendar cal)
	throws SQLException {
	setDate(parameterIndex, x);
    }

    public void setTime(int parameterIndex, java.sql.Time x, Calendar cal)
	throws SQLException {
	setTime(parameterIndex, x);
    }

    public void setTimestamp(int parameterIndex, java.sql.Timestamp x,
			     Calendar cal) throws SQLException {
	setTimestamp(parameterIndex, x);
    }

    public void setNull(int parameterIndex, int sqlType, String typeName)
	throws SQLException {
	setNull(parameterIndex, sqlType);
    }

    public ParameterMetaData getParameterMetaData() throws SQLException {
	throw new SQLException("not supported");
    }

    public void registerOutputParameter(String parameterName, int sqlType)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void registerOutputParameter(String parameterName, int sqlType,
					int scale)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void registerOutputParameter(String parameterName, int sqlType,
					String typeName)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.net.URL getURL(int parameterIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public void setURL(int parameterIndex, java.net.URL url)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setNull(String parameterName, int sqlType)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setBoolean(String parameterName, boolean val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setByte(String parameterName, byte val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setShort(String parameterName, short val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setInt(String parameterName, int val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setLong(String parameterName, long val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setFloat(String parameterName, float val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setDouble(String parameterName, double val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setBigDecimal(String parameterName, BigDecimal val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setString(String parameterName, String val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setBytes(String parameterName, byte val[])
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setDate(String parameterName, java.sql.Date val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setTime(String parameterName, java.sql.Time val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setTimestamp(String parameterName, java.sql.Timestamp val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setAsciiStream(String parameterName,
			       java.io.InputStream s, int length)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setBinaryStream(String parameterName,
				java.io.InputStream s, int length)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setObject(String parameterName, Object val, int targetSqlType,
			  int scale)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setObject(String parameterName, Object val, int targetSqlType)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setObject(String parameterName, Object val)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setCharacterStream(String parameterName,
				   java.io.Reader r, int length)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setDate(String parameterName, java.sql.Date val,
			Calendar cal)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setTime(String parameterName, java.sql.Time val,
			Calendar cal)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setTimestamp(String parameterName, java.sql.Timestamp val,
			     Calendar cal)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public void setNull(String parameterName, int sqlType, String typeName)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public String getString(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public boolean getBoolean(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public byte getByte(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public short getShort(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public int getInt(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public long getLong(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public float getFloat(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public double getDouble(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public byte[] getBytes(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Date getDate(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Time getTime(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Timestamp getTimestamp(String parameterName)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public Object getObject(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public Object getObject(int parameterIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public BigDecimal getBigDecimal(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public Object getObject(String parameterName, Map map)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public Object getObject(int parameterIndex, Map map)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public Ref getRef(int parameterIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public Ref getRef(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public Blob getBlob(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public Blob getBlob(int parameterIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public Clob getClob(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public Clob getClob(int parameterIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public Array getArray(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

    public Array getArray(int parameterIndex) throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Date getDate(String parameterName, Calendar cal)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Date getDate(int parameterIndex, Calendar cal)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Time getTime(String parameterName, Calendar cal)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Time getTime(int parameterIndex, Calendar cal)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Timestamp getTimestamp(String parameterName, Calendar cal)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.sql.Timestamp getTimestamp(int parameterIndex, Calendar cal)
	throws SQLException {
	throw new SQLException("not supported");
    }

    public java.net.URL getURL(String parameterName) throws SQLException {
	throw new SQLException("not supported");
    }

}
