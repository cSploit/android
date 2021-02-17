package SQLite;

import java.sql.Connection;
import java.sql.Driver;
import java.sql.DriverPropertyInfo;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.util.Properties;
import java.util.logging.Logger;

public class JDBCDriver extends JDBC implements Driver {

    public static final int MAJORVERSION = 1;

    public static boolean sharedCache = false;

    public static String vfs = null;

    private static java.lang.reflect.Constructor makeConn = null;

    protected Connection conn;

    static {
	try {
	    java.sql.DriverManager.registerDriver(new JDBCDriver());
	} catch (java.lang.Exception e) {
	    System.err.println(e);
	}
    }

    public JDBCDriver() {
    }
	
    public DriverPropertyInfo[] getPropertyInfo(String url, Properties info)
	throws SQLException {
	DriverPropertyInfo p[] = new DriverPropertyInfo[4];
	DriverPropertyInfo pp = new DriverPropertyInfo("encoding", "");
	p[0] = pp;
	pp = new DriverPropertyInfo("password", "");
	p[1] = pp;
	pp = new DriverPropertyInfo("daterepr", "normal");
	p[2] = pp;
	pp = new DriverPropertyInfo("vfs", vfs);
	p[3] = pp;
	return p;
    }

    public boolean jdbcCompliant() {
	return false;
    }

    public Logger getParentLogger() throws SQLFeatureNotSupportedException {
	throw new SQLFeatureNotSupportedException();
    }

}
