package SQLite.JDBC1;

import java.sql.*;
import java.util.*;

public class JDBCStatement implements java.sql.Statement {

    protected JDBCConnection conn;
    protected JDBCResultSet rs;
    protected int updcnt;
    protected int maxrows = 0;

    public JDBCStatement(JDBCConnection conn) {
	this.conn = conn;
    }

    public void setFetchSize(int fetchSize) throws SQLException {
	if (fetchSize != 1) {
	    throw new SQLException("fetch size not 1");
	}
    }

    public int getFetchSize() throws SQLException {
	return 1;
    }

    public int getMaxRows() throws SQLException {
	return maxrows;
    }

    public void setMaxRows(int max) throws SQLException {
	if (max < 0) {
	    throw new SQLException("max must be >= 0 (was " + max + ")");
	}
	maxrows = max;
    }

    public void setQueryTimeout(int seconds) throws SQLException {
	conn.timeout = seconds * 1000;
	if (conn.timeout < 0) {
	    conn.timeout = 120000;
	} else if (conn.timeout < 1000) {
	    conn.timeout = 5000;
	}
    }

    public int getQueryTimeout() throws SQLException {
	return conn.timeout / 1000;
    }

    public ResultSet getResultSet() throws SQLException {
	return rs;
    }

    ResultSet executeQuery(String sql, String args[], boolean updonly)
	throws SQLException {
	SQLite.TableResult tr = null;
	if (rs != null) {
	    rs.close();
	    rs = null;
	}
	updcnt = -1;
	if (conn == null || conn.db == null) {
	    throw new SQLException("stale connection");
	}
	int busy = 0;
	boolean starttrans = !conn.autocommit && !conn.intrans;
	while (true) {
	    try {
		if (starttrans) {
		    conn.db.exec("BEGIN TRANSACTION", null);
		    conn.intrans = true;
		}
		if (args == null) {
		    if (updonly) {
			conn.db.exec(sql, null);
		    } else {
			tr = conn.db.get_table(sql, maxrows);
		    }
		} else {
		    if (updonly) {
			conn.db.exec(sql, null, args);
		    } else {
			tr = conn.db.get_table(sql, maxrows, args);
		    }
		}
		updcnt = (int) conn.db.changes();
	    } catch (SQLite.Exception e) {
		if (conn.db.is3() &&
		    conn.db.last_error() == SQLite.Constants.SQLITE_BUSY &&
		    conn.busy3(conn.db, ++busy)) {
		    try {
			if (starttrans && conn.intrans) {
			    conn.db.exec("ROLLBACK", null);
			    conn.intrans = false;
			}
		    } catch (SQLite.Exception ee) {
		    }
		    try {
			int ms = 20 + busy * 10;
			if (ms > 1000) {
			    ms = 1000;
			}
			synchronized (this) {
			    this.wait(ms);
			}
		    } catch (java.lang.Exception eee) {
		    }
		    continue;
		}
		throw new SQLException(e.toString());
	    }
	    break;
	}
	if (!updonly && tr == null) {
	    throw new SQLException("no result set produced");
	}
	if (!updonly && tr != null) {
	    rs = new JDBCResultSet(new TableResultX(tr), this);
	}
	return rs;
    }

    public ResultSet executeQuery(String sql) throws SQLException {
	return executeQuery(sql, null, false);
    }

    public boolean execute(String sql) throws SQLException {
	return executeQuery(sql) != null;
    }

    public void cancel() throws SQLException {
	if (conn == null || conn.db == null) {
	    throw new SQLException("stale connection");
	}
	conn.db.interrupt();
    }

    public void clearWarnings() throws SQLException {
    }

    public Connection getConnection() throws SQLException {
	return conn;
    }

    public void addBatch(String sql) throws SQLException {
	throw new SQLException("not supported");
    }

    public int[] executeBatch() throws SQLException {
	throw new SQLException("not supported");
    }

    public void clearBatch() throws SQLException {
	throw new SQLException("not supported");
    }

    public void close() throws SQLException {
	conn = null;
    }

    public int executeUpdate(String sql) throws SQLException {
	executeQuery(sql, null, true);
	return updcnt;
    }

    public int getMaxFieldSize() throws SQLException {
	return 0;
    }

    public boolean getMoreResults() throws SQLException {
	if (rs != null) {
	    rs.close();
	    rs = null;
	}
	return false;
    }

    public int getUpdateCount() throws SQLException {
	return updcnt;
    }

    public SQLWarning getWarnings() throws SQLException {
	return null;
    }

    public void setCursorName(String name) throws SQLException {
	throw new SQLException("not supported");
    }

    public void setEscapeProcessing(boolean enable) throws SQLException {
	throw new SQLException("not supported");
    }

    public void setMaxFieldSize(int max) throws SQLException {
	throw new SQLException("not supported");
    }

}
