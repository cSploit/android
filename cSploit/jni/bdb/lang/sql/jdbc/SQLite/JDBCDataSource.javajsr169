package SQLite;

import java.sql.Connection;
import java.sql.SQLException;
import javax.sql.DataSource;
import java.util.Properties;
import java.io.PrintWriter;

public final class JDBCDataSource extends JDBC implements DataSource {

    private String url;

    private int loginTimeout;

    private PrintWriter logWriter;

    public JDBCDataSource(String url) {
	if (url == null) {
	    this.url = "jdbc:sqlite:/:memory:";
	} else {
	    this.url = url;
	}
    }

    public Connection getConnection() throws SQLException {
	return getConnection(null, null);
    }
	
    public Connection getConnection(String user, String password)
	throws SQLException {
	Properties info = new Properties();
	if (password != null) {
	    info.setProperty("password", password);
	}
	return connect(url, info);
    }

    public int getLoginTimeout() {
	return loginTimeout;
    }

    public void	setLoginTimeout(int seconds) {
	this.loginTimeout = seconds;
    }

    public PrintWriter getLogWriter() {
	return logWriter;
    } 

    public void	setLogWriter(PrintWriter out) {
	this.logWriter = logWriter;
    }

    // public <T> T unwrap(java.lang.Class<T> iface) throws SQLException
    public java.lang.Object unwrap(java.lang.Class iface) throws SQLException {
	throw new SQLException("unsupported");
    }

    public boolean isWrapperFor(java.lang.Class iface) throws SQLException {
        return false;
    }

}
