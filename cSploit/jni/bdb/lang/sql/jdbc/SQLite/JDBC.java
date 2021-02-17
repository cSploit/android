package SQLite;

import java.sql.Connection;
import java.sql.SQLException;
import java.util.Properties;

public abstract class JDBC {

    public static final int MAJORVERSION = 1;

    public static boolean sharedCache = false;

    public static String vfs = null;

    private static java.lang.reflect.Constructor makeConn = null;

    static {
	try {
	    Class connClass = null;
	    Class args[] = new Class[5];
	    args[0] = Class.forName("java.lang.String");
	    args[1] = args[0];
	    args[2] = args[0];
	    args[3] = args[0];
	    args[4] = args[0];
	    String jvers = java.lang.System.getProperty("java.version");
	    String vmvers = java.lang.System.getProperty("java.vm.name");
	    String cvers;
	    if (vmvers != null && "CVM".equalsIgnoreCase(vmvers)) {
		cvers = "SQLite.JDBC0.JDBCConnection";
	    } else if (jvers == null || jvers.startsWith("1.0")) {
		throw new java.lang.Exception("unsupported java version");
	    } else if (jvers.startsWith("1.1")) {
		cvers = "SQLite.JDBC1.JDBCConnection";
	    } else if (jvers.startsWith("1.2") || jvers.startsWith("1.3")) {
		cvers = "SQLite.JDBC2.JDBCConnection";
	    } else if (jvers.startsWith("1.4")) {
		cvers = "SQLite.JDBC2x.JDBCConnection";
	    } else if (jvers.startsWith("1.5")) {
		cvers = "SQLite.JDBC2y.JDBCConnection";
		try {
		    Class.forName(cvers);
		} catch (java.lang.Exception e) {
		    cvers = "SQLite.JDBC2x.JDBCConnection";
		}
	    } else if (jvers.startsWith("1.6")) {
		cvers = "SQLite.JDBC2z.JDBCConnection";
		try {
		    Class.forName(cvers);
		} catch (java.lang.Exception e) {
		    cvers = "SQLite.JDBC2y.JDBCConnection";
		    try {
			Class.forName(cvers);
		    } catch (java.lang.Exception ee) {
			cvers = "SQLite.JDBC2x.JDBCConnection";
		    }
		}
	    } else {
		cvers = "SQLite.JDBC2z1.JDBCConnection";
		try {
		    Class.forName(cvers);
		} catch (java.lang.Exception e) {
		    cvers = "SQLite.JDBC2z.JDBCConnection";
		    try {
			Class.forName(cvers);
		    } catch (java.lang.Exception ee) {
			cvers = "SQLite.JDBC2y.JDBCConnection";
			try {
			    Class.forName(cvers);
			} catch (java.lang.Exception eee) {
			    cvers = "SQLite.JDBC2x.JDBCConnection";
			}
		    }
		}
	    }
 	    connClass = Class.forName(cvers);
	    makeConn = connClass.getConstructor(args);
	    try {
		String shcache =
		    java.lang.System.getProperty("SQLite.sharedcache");
		if (shcache != null &&
		    (shcache.startsWith("y") || shcache.startsWith("Y"))) {
		    sharedCache = SQLite.Database._enable_shared_cache(true);
		}
	    } catch (java.lang.Exception e) {
	    }
	    try {
		String tvfs = 
		    java.lang.System.getProperty("SQLite.vfs");
		if (tvfs != null) {
		    vfs = tvfs;
		}
	    } catch (java.lang.Exception e) {
	    }
	} catch (java.lang.Exception e) {
	    System.err.println(e);
	}
    }

    public JDBC() {
    }
	
    public boolean acceptsURL(String url) throws SQLException {
	return url.startsWith("sqlite:/") ||
	    url.startsWith("jdbc:sqlite:/");
    }

    public Connection connect(String url, Properties info)
	throws SQLException {
	if (!acceptsURL(url)) {
	    return null;
	}
	Object args[] = new Object[5];
	args[0] = url;
	if (info != null) {
	    args[1] = info.getProperty("encoding");
	    args[2] = info.getProperty("password");
	    args[3] = info.getProperty("daterepr");
	    args[4] = info.getProperty("vfs");
	}
	if (args[1] == null) {
	    args[1] = java.lang.System.getProperty("SQLite.encoding");
	}
	if (args[4] == null) {
	    args[4] = vfs;
	}
	try {
	    return (Connection) makeConn.newInstance(args);
	} catch (java.lang.reflect.InvocationTargetException ie) {
	    throw new SQLException(ie.getTargetException().toString());
	} catch (java.lang.Exception e) {
	    throw new SQLException(e.toString());
	}
    }

    public int getMajorVersion() {
	return MAJORVERSION;
    }

    public int getMinorVersion() {
	return Constants.drv_minor;
    }

 }
