package SQLite.JDBC2z;

import java.sql.BatchUpdateException;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.sql.SQLWarning;
import java.sql.Statement;
import java.util.ArrayList;

public class JDBCStatement implements java.sql.Statement {

    protected JDBCConnection conn;
    protected JDBCResultSet rs;
    protected int updcnt;
    protected int maxrows = 0;
    private ArrayList<String> batch;

    public JDBCStatement(JDBCConnection conn) {
	this.conn = conn;
	this.updcnt = 0;
	this.rs = null;
	this.batch = null;	
    }

    @Override
    public void setFetchSize(int fetchSize) throws SQLException {
	if (fetchSize != 1) {
	    throw new SQLException("fetch size not 1");
	}
    }

    @Override
    public int getFetchSize() throws SQLException {
	return 1;
    }

    @Override
    public int getMaxRows() throws SQLException {
	return maxrows;
    }

    @Override
    public void setMaxRows(int max) throws SQLException {
	if (max < 0) {
	    throw new SQLException("max must be >= 0 (was " + max + ")");
	}
	maxrows = max;
    }

    @Override
    public void setFetchDirection(int fetchDirection) throws SQLException {
	throw new SQLException("not supported");
    }

    @Override
    public int getFetchDirection() throws SQLException {
	return ResultSet.FETCH_UNKNOWN;
    }

    @Override
    public int getResultSetConcurrency() throws SQLException {
	return ResultSet.CONCUR_READ_ONLY;
    }

    @Override
    public int getResultSetType() throws SQLException {
	return ResultSet.TYPE_SCROLL_INSENSITIVE;
    }

    @Override
    public void setQueryTimeout(int seconds) throws SQLException {
	if (isClosed()) {
	    throw new SQLException("can't set query timeout on " +
				   "a closed statement");
	} else if (seconds < 0) {
	    throw new SQLException("can't set a query timeout of " +
				   "less than 0 seconds");
	} else if (seconds == 0) {
	    conn.timeout = 5000;
	} else {
	    conn.timeout = seconds * 1000;
	}
    }

    @Override
    public int getQueryTimeout() throws SQLException {
	return conn.timeout / 1000;
    }

    @Override
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
		throw new SQLException(e);
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

    @Override
    public ResultSet executeQuery(String sql) throws SQLException {
	return executeQuery(sql, null, false);
    }

    @Override
    public boolean execute(String sql) throws SQLException {
	return executeQuery(sql) != null;
    }

    @Override
    public void cancel() throws SQLException {
	if (conn == null || conn.db == null) {
	    throw new SQLException("stale connection");
	}
	conn.db.interrupt();
    }

    @Override
    public void clearWarnings() throws SQLException {
    }

    @Override
    public Connection getConnection() throws SQLException {
	return conn;
    }

    @Override
    public void addBatch(String sql) throws SQLException {
	if (batch == null) {
	    batch = new ArrayList<String>(1);
	}
	batch.add(sql);
    }

    @Override
    public int[] executeBatch() throws SQLException {
	if (batch == null) {
	    return new int[0];
	}
	int[] ret = new int[batch.size()];
	for (int i = 0; i < ret.length; i++) {
	    ret[i] = EXECUTE_FAILED;
	}
	int errs = 0;
	Exception cause = null;
	for (int i = 0; i < ret.length; i++) {
	    try {
		execute(batch.get(i));
		ret[i] = updcnt;
	    } catch (SQLException e) {
		++errs;
		if (cause == null) {
		    cause = e;
		}
	    }
	}
	if (errs > 0) {
	    throw new BatchUpdateException("batch failed", ret, cause);
	}
	return ret;
    }

    @Override
    public void clearBatch() throws SQLException {
	if (batch != null) {
	    batch.clear();
	    batch = null;
	}
    }

    @Override
    public void close() throws SQLException {
	clearBatch();
	conn = null;
    }

    @Override
    public int executeUpdate(String sql) throws SQLException {
	executeQuery(sql, null, true);
	return updcnt;
    }

    @Override
    public int getMaxFieldSize() throws SQLException {
	return 0;
    }

    @Override
    public boolean getMoreResults() throws SQLException {
	if (rs != null) {
	    rs.close();
	    rs = null;
	}
	return false;
    }

    @Override
    public int getUpdateCount() throws SQLException {
	return updcnt;
    }

    @Override
    public SQLWarning getWarnings() throws SQLException {
	return null;
    }

    @Override
    public void setCursorName(String name) throws SQLException {
	throw new SQLFeatureNotSupportedException();
    }

    @Override
    public void setEscapeProcessing(boolean enable) throws SQLException {
	throw new SQLException("not supported");
    }

    @Override
    public void setMaxFieldSize(int max) throws SQLException {
	throw new SQLException("not supported");
    }

    @Override
    public boolean getMoreResults(int x) throws SQLException {
	throw new SQLFeatureNotSupportedException();
    }

    @Override
    public ResultSet getGeneratedKeys() throws SQLException {
	throw new SQLFeatureNotSupportedException();
    }

    @Override
    public int executeUpdate(String sql, int autokeys)
	throws SQLException {
	if (autokeys != Statement.NO_GENERATED_KEYS) {
	    throw new SQLFeatureNotSupportedException("generated keys not supported");
	}
	return executeUpdate(sql);
    }

    @Override
    public int executeUpdate(String sql, int colIndexes[])
	throws SQLException {
	throw new SQLFeatureNotSupportedException();
    }

    @Override
    public int executeUpdate(String sql, String colIndexes[])
	throws SQLException {
	throw new SQLFeatureNotSupportedException();
    }

    @Override
    public boolean execute(String sql, int autokeys)
	throws SQLException {
	if (autokeys != Statement.NO_GENERATED_KEYS) {
	    throw new SQLFeatureNotSupportedException("autogenerated keys not supported");
	}
	return execute(sql);
    }

    @Override
    public boolean execute(String sql, int colIndexes[])
	throws SQLException {
	throw new SQLFeatureNotSupportedException();
    }

    @Override
    public boolean execute(String sql, String colIndexes[])
	throws SQLException {
	throw new SQLFeatureNotSupportedException();
    }

    @Override
    public int getResultSetHoldability() throws SQLException {
	return ResultSet.HOLD_CURSORS_OVER_COMMIT;
    }

    @Override
    public boolean isClosed() throws SQLException {
	return conn == null;
    }

    @Override
    public void setPoolable(boolean yes) throws SQLException {
	if (yes) {
	    throw new SQLException("poolable statements not supported");
	}
    }

    @Override
    public boolean isPoolable() throws SQLException {
	return false;
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
