package SQLite;

/**
 * Main class wrapping an SQLite database.
 */

public class Database {

    /**
     * Internal handle for the native SQLite API.
     */

    protected long handle = 0;

    /**
     * Internal last error code for exec() methods.
     */

    protected int error_code = 0;

    /**
     * Open an SQLite database file.
     *
     * @param filename the name of the database file
     * @param mode open mode (e.g. SQLITE_OPEN_READONLY)
     */

    public void open(String filename, int mode) throws SQLite.Exception {
	if ((mode & 0200) != 0) {
	    mode = SQLite.Constants.SQLITE_OPEN_READWRITE |
		   SQLite.Constants.SQLITE_OPEN_CREATE;
	} else if ((mode & 0400) != 0) {
	    mode = SQLite.Constants.SQLITE_OPEN_READONLY;
	}
	synchronized(this) {
	    try {
		_open4(filename, mode, null, false);
	    } catch (SQLite.Exception se) {
		throw se;
	    } catch (java.lang.OutOfMemoryError me) {
		throw me;
	    } catch (Throwable t) {
		_open(filename, mode);
	    }
	}
    }

    /**
     * Open an SQLite database file.
     *
     * @param filename the name of the database file
     * @param mode open mode (e.g. SQLITE_OPEN_READONLY)
     * @param vfs VFS name (for SQLite >= 3.5)
     */

    public void open(String filename, int mode, String vfs)
	throws SQLite.Exception {
	if ((mode & 0200) != 0) {
	    mode = SQLite.Constants.SQLITE_OPEN_READWRITE |
		   SQLite.Constants.SQLITE_OPEN_CREATE;
	} else if ((mode & 0400) != 0) {
	    mode = SQLite.Constants.SQLITE_OPEN_READONLY;
	}
	synchronized(this) {
	    try {
		_open4(filename, mode, vfs, false);
	    } catch (SQLite.Exception se) {
		throw se;
	    } catch (java.lang.OutOfMemoryError me) {
		throw me;
	    } catch (Throwable t) {
		_open(filename, mode);
	    }
	}
    }

    /**
     * Open an SQLite database file.
     *
     * @param filename the name of the database file
     * @param mode open mode (e.g. SQLITE_OPEN_READONLY)
     * @param vfs VFS name (for SQLite >= 3.5)
     * @param ver2 flag to force version on create (false = SQLite3, true = SQLite2)
     */

    public void open(String filename, int mode, String vfs, boolean ver2)
	throws SQLite.Exception {
	if ((mode & 0200) != 0) {
	    mode = SQLite.Constants.SQLITE_OPEN_READWRITE |
		   SQLite.Constants.SQLITE_OPEN_CREATE;
	} else if ((mode & 0400) != 0) {
	    mode = SQLite.Constants.SQLITE_OPEN_READONLY;
	}
	synchronized(this) {
	    try {
		_open4(filename, mode, vfs, ver2);
	    } catch (SQLite.Exception se) {
		throw se;
	    } catch (java.lang.OutOfMemoryError me) {
		throw me;
	    } catch (Throwable t) {
		_open(filename, mode);
	    }
	}
    }

    /*
     * For backward compatibility to older sqlite.jar, sqlite_jni
     */

    private native void _open(String filename, int mode)
	throws SQLite.Exception;

    /*
     * Newer full interface
     */

    private native void _open4(String filename, int mode, String vfs,
			       boolean ver2)
	throws SQLite.Exception;

    /**
     * Open SQLite auxiliary database file for temporary
     * tables.
     *
     * @param filename the name of the auxiliary file or null
     */

    public void open_aux_file(String filename) throws SQLite.Exception {
	synchronized(this) {
	    _open_aux_file(filename);
	}
    }

    private native void _open_aux_file(String filename)
	throws SQLite.Exception;

    /**
     * Destructor for object.
     */

    protected void finalize() {
	synchronized(this) {
	    _finalize();
	}
    }

    private native void _finalize();

    /**
     * Close the underlying SQLite database file.
     */

    public void close()	throws SQLite.Exception {
	synchronized(this) {
	    _close();
	}
    }

    private native void _close()
	throws SQLite.Exception;

    /**
     * Execute an SQL statement and invoke callback methods
     * for each row of the result set.<P>
     *
     * It the method fails, an SQLite.Exception is thrown and
     * an error code is set, which later can be retrieved by
     * the last_error() method.
     *
     * @param sql the SQL statement to be executed
     * @param cb the object implementing the callback methods
     */

    public void exec(String sql, SQLite.Callback cb) throws SQLite.Exception {
	synchronized(this) {
	    _exec(sql, cb);
	}
    }

    private native void _exec(String sql, SQLite.Callback cb)
	throws SQLite.Exception;

    /**
     * Execute an SQL statement and invoke callback methods
     * for each row of the result set. Each '%q' or %Q in the
     * statement string is substituted by its corresponding
     * element in the argument vector.
     * <BR><BR>
     * Example:<BR>
     * <PRE>
     *   String args[] = new String[1];
     *   args[0] = "tab%";
     *   db.exec("select * from sqlite_master where type like '%q'",
     *           null, args);
     * </PRE>
     *
     * It the method fails, an SQLite.Exception is thrown and
     * an error code is set, which later can be retrieved by
     * the last_error() method.
     *
     * @param sql the SQL statement to be executed
     * @param cb the object implementing the callback methods
     * @param args arguments for the SQL statement, '%q' substitution
     */

    public void exec(String sql, SQLite.Callback cb,
		     String args[]) throws SQLite.Exception {
	synchronized(this) {
	    _exec(sql, cb, args);
	}
    }

    private native void _exec(String sql, SQLite.Callback cb, String args[])
	throws SQLite.Exception;

    /**
     * Return the row identifier of the last inserted
     * row.
     */

    public long last_insert_rowid() {
	synchronized(this) {
	    return _last_insert_rowid();
	}
    }

    private native long _last_insert_rowid();

    /**
     * Abort the current SQLite operation.
     */

    public void interrupt() {
	synchronized(this) {
	    _interrupt();
	}
    }

    private native void _interrupt();

    /**
     * Return the number of changed rows for the last statement.
     */

    public long changes() {
	synchronized(this) {
	    return _changes();
	}
    }

    private native long _changes();

    /**
     * Establish a busy callback method which gets called when
     * an SQLite table is locked.
     *
     * @param bh the object implementing the busy callback method
     */

    public void busy_handler(SQLite.BusyHandler bh) {
	synchronized(this) {
	    _busy_handler(bh);
	}
    }

    private native void _busy_handler(SQLite.BusyHandler bh);

    /**
     * Set the timeout for waiting for an SQLite table to become
     * unlocked.
     *
     * @param ms number of millisecond to wait
     */

    public void busy_timeout(int ms) {
	synchronized(this) {
	    _busy_timeout(ms);
	}
    }

    private native void _busy_timeout(int ms);

    /**
     * Convenience method to retrieve an entire result
     * set into memory.
     *
     * @param sql the SQL statement to be executed
     * @param maxrows the max. number of rows to retrieve
     * @return result set
     */

    public TableResult get_table(String sql, int maxrows)
	throws SQLite.Exception {
	TableResult ret = new TableResult(maxrows);
	if (!is3()) {
	    try {
		exec(sql, ret);
	    } catch (SQLite.Exception e) {
		if (maxrows <= 0 || !ret.atmaxrows) {
		    throw e;
		}
	    }
	} else {
	    synchronized(this) {
		/* only one statement !!! */
		Vm vm = compile(sql);
		set_last_error(vm.error_code);
		if (ret.maxrows > 0) {
		    while (ret.nrows < ret.maxrows && vm.step(ret)) {
			set_last_error(vm.error_code);
		    }
		} else {
		    while (vm.step(ret)) {
			set_last_error(vm.error_code);
		    }
		}
		vm.finalize();
	    }
	}
	return ret;
    }

    /**
     * Convenience method to retrieve an entire result
     * set into memory.
     *
     * @param sql the SQL statement to be executed
     * @return result set
     */

    public TableResult get_table(String sql) throws SQLite.Exception {
	return get_table(sql, 0);
    }

    /**
     * Convenience method to retrieve an entire result
     * set into memory.
     *
     * @param sql the SQL statement to be executed
     * @param maxrows the max. number of rows to retrieve
     * @param args arguments for the SQL statement, '%q' substitution
     * @return result set
     */

    public TableResult get_table(String sql, int maxrows, String args[])
	throws SQLite.Exception {
	TableResult ret = new TableResult(maxrows);
	if (!is3()) {
	    try {
		exec(sql, ret, args);
	    } catch (SQLite.Exception e) {
		if (maxrows <= 0 || !ret.atmaxrows) {
		    throw e;
		}
	    }
	} else {
	    synchronized(this) {
		/* only one statement !!! */
		Vm vm = compile(sql, args);
		set_last_error(vm.error_code);
		if (ret.maxrows > 0) {
		    while (ret.nrows < ret.maxrows && vm.step(ret)) {
			set_last_error(vm.error_code);
		    }
		} else {
		    while (vm.step(ret)) {
			set_last_error(vm.error_code);
		    }
		}
		vm.finalize();
	    }
	}
	return ret;
    }

    /**
     * Convenience method to retrieve an entire result
     * set into memory.
     *
     * @param sql the SQL statement to be executed
     * @param args arguments for the SQL statement, '%q' substitution
     * @return result set
     */

    public TableResult get_table(String sql, String args[])
	throws SQLite.Exception {
	return get_table(sql, 0, args);
    }

    /**
     * Convenience method to retrieve an entire result
     * set into memory.
     *
     * @param sql the SQL statement to be executed
     * @param args arguments for the SQL statement, '%q' substitution
     * @param tbl TableResult to receive result set
     */

    public void get_table(String sql, String args[], TableResult tbl)
	throws SQLite.Exception {
	tbl.clear();
	if (!is3()) {
	    try {
		exec(sql, tbl, args);
	    } catch (SQLite.Exception e) {
		if (tbl.maxrows <= 0 || !tbl.atmaxrows) {
		    throw e;
		}
	    }
	} else {
	    synchronized(this) {
		/* only one statement !!! */
		Vm vm = compile(sql, args);
		if (tbl.maxrows > 0) {
		    while (tbl.nrows < tbl.maxrows && vm.step(tbl)) {
			set_last_error(vm.error_code);
		    }
		} else {
		    while (vm.step(tbl)) {
			set_last_error(vm.error_code);
		    }
		}
		vm.finalize();
	    }
	}
    }

    /**
     * See if an SQL statement is complete.
     * Returns true if the input string comprises
     * one or more complete SQL statements.
     *
     * @param sql the SQL statement to be checked
     */

    public synchronized static boolean complete(String sql) {
	return _complete(sql);
    }

    private native static boolean _complete(String sql);

    /**
     * Return SQLite version number as string.
     * Don't rely on this when both SQLite 2 and 3 are compiled
     * into the native part. Use the class method in this case.
     */

    public native static String version();

    /**
     * Return SQLite version number as string.
     * If the database is not open, <tt>unknown</tt> is returned.
     */

    public native String dbversion();

    /**
     * Create regular function.
     *
     * @param name the name of the new function
     * @param nargs number of arguments to function
     * @param f interface of function
     */

    public void create_function(String name, int nargs, Function f) {
	synchronized(this) {
	    _create_function(name, nargs, f);
	}
    }

    private native void _create_function(String name, int nargs, Function f);

    /**
     * Create aggregate function.
     *
     * @param name the name of the new function
     * @param nargs number of arguments to function
     * @param f interface of function
     */

    public void create_aggregate(String name, int nargs, Function f) {
	synchronized(this) {
	    _create_aggregate(name, nargs, f);
	}
    }

    private native void _create_aggregate(String name, int nargs, Function f);

    /**
     * Set function return type. Only available in SQLite 2.6.0 and
     * above, otherwise a no-op.
     *
     * @param name the name of the function whose return type is to be set
     * @param type return type code, e.g. SQLite.Constants.SQLITE_NUMERIC
     */

    public void function_type(String name, int type) {
	synchronized(this) {
	    _function_type(name, type);
	}
    }

    private native void _function_type(String name, int type);

    /**
     * Return the code of the last error occured in
     * any of the exec() methods. The value is valid
     * after an Exception has been reported by one of
     * these methods. See the <A HREF="Constants.html">Constants</A>
     * class for possible values.
     *
     * @return SQLite error code
     */

    public int last_error() {
	return error_code;
    }

    /**
     * Internal: set error code.
     * @param error_code new error code
     */

    protected void set_last_error(int error_code) {
	this.error_code = error_code;
    }

    /**
     * Return last error message of SQLite3 engine.
     *
     * @return error string or null
     */

    public String error_message() {
	synchronized(this) {
	    return _errmsg();
	}
    }

    private native String _errmsg();

    /**
     * Return error string given SQLite error code (SQLite2).
     *
     * @param error_code the error code
     * @return error string
     */

    public static native String error_string(int error_code);

    /**
     * Set character encoding.
     * @param enc name of encoding
     */

    public void set_encoding(String enc) throws SQLite.Exception {
	synchronized(this) {
	    _set_encoding(enc);
	}
    }

    private native void _set_encoding(String enc)
	throws SQLite.Exception;

    /**
     * Set authorizer function. Only available in SQLite 2.7.6 and
     * above, otherwise a no-op.
     *
     * @param auth the authorizer function
     */

    public void set_authorizer(Authorizer auth) {
	synchronized(this) {
	    _set_authorizer(auth);
	}
    }

    private native void _set_authorizer(Authorizer auth);

    /**
     * Set trace function. Only available in SQLite 2.7.6 and above,
     * otherwise a no-op.
     *
     * @param tr the trace function
     */

    public void trace(Trace tr) {
	synchronized(this) {
	    _trace(tr);
	}
    }

    private native void _trace(Trace tr);

    /**
     * Initiate a database backup, SQLite 3.x only.
     *
     * @param dest destination database
     * @param destName schema of destination database to be backed up
     * @param srcName schema of source database
     * @return Backup object to perform the backup operation
     */

    public Backup backup(Database dest, String destName, String srcName)
	throws SQLite.Exception {
	synchronized(this) {
	    Backup b = new Backup();
	    _backup(b, dest, destName, this, srcName);
	    return b;
	}
    }

    private static native void _backup(Backup b, Database dest,
				       String destName, Database src,
				       String srcName)
	throws SQLite.Exception;

    /**
     * Set profile function. Only available in SQLite 3.6 and above,
     * otherwise a no-op.
     *
     * @param pr the trace function
     */

    public void profile(Profile pr) {
	synchronized(this) {
	    _profile(pr);
	}
    }

    private native void _profile(Profile pr);

    /**
     * Return information on SQLite runtime status.
     * Only available in SQLite 3.6 and above,
     * otherwise a no-op.
     *
     * @param op   operation code
     * @param info output buffer, must be able to hold two
     *             values (current/highwater)
     * @param flag reset flag
     * @return SQLite error code
     */

    public synchronized static int status(int op, int info[], boolean flag) {
	return _status(op, info, flag);
    }

    private native static int _status(int op, int info[], boolean flag);

    /**
     * Return information on SQLite connection status.
     * Only available in SQLite 3.6 and above,
     * otherwise a no-op.
     *
     * @param op operation code
     * @param info output buffer, must be able to hold two
     *             values (current/highwater)
     * @param flag reset flag
     * @return SQLite error code
     */

    public int db_status(int op, int info[], boolean flag) {
	synchronized(this) {
	    return _db_status(op, info, flag);
	}
    }

    private native int _db_status(int op, int info[], boolean flag);

    /**
     * Compile and return SQLite VM for SQL statement. Only available
     * in SQLite 2.8.0 and above, otherwise a no-op.
     *
     * @param sql SQL statement to be compiled
     * @return a Vm object
     */

    public Vm compile(String sql) throws SQLite.Exception {
	synchronized(this) {
	    Vm vm = new Vm();
	    vm_compile(sql, vm);
	    return vm;
	}
    }

    /**
     * Compile and return SQLite VM for SQL statement. Only available
     * in SQLite 3.0 and above, otherwise a no-op.
     *
     * @param sql SQL statement to be compiled
     * @param args arguments for the SQL statement, '%q' substitution
     * @return a Vm object
     */

    public Vm compile(String sql, String args[]) throws SQLite.Exception {
	synchronized(this) {
	    Vm vm = new Vm();
	    vm_compile_args(sql, vm, args);
	    return vm;
	}
    }

    /**
     * Prepare and return SQLite3 statement for SQL. Only available
     * in SQLite 3.0 and above, otherwise a no-op.
     *
     * @param sql SQL statement to be prepared
     * @return a Stmt object
     */

    public Stmt prepare(String sql) throws SQLite.Exception {
	synchronized(this) {
	    Stmt stmt = new Stmt();
	    stmt_prepare(sql, stmt);
	    return stmt;
	}
    }

    /**
     * Open an SQLite3 blob. Only available in SQLite 3.4.0 and above.
     * @param db database name
     * @param table table name
     * @param column column name
     * @param row row identifier
     * @param rw if true, open for read-write, else read-only
     * @return a Blob object
     */

    public Blob open_blob(String db, String table, String column,
			  long row, boolean rw) throws SQLite.Exception {
	synchronized(this) {
	    Blob blob = new Blob();
	    _open_blob(db, table, column, row, rw, blob);
	    return blob;
	}
    }

    /**
     * Check type of open database.
     * @return true if SQLite3 database
     */

    public native boolean is3();

    /**
     * Internal compile method.
     * @param sql SQL statement
     * @param vm Vm object
     */

    private native void vm_compile(String sql, Vm vm)
	throws SQLite.Exception;

    /**
     * Internal compile method, SQLite 3.0 only.
     * @param sql SQL statement
     * @param args arguments for the SQL statement, '%q' substitution
     * @param vm Vm object
     */

    private native void vm_compile_args(String sql, Vm vm, String args[])
	throws SQLite.Exception;

    /**
     * Internal SQLite3 prepare method.
     * @param sql SQL statement
     * @param stmt Stmt object
     */

    private native void stmt_prepare(String sql, Stmt stmt)
	throws SQLite.Exception;

    /**
     * Internal SQLite open blob method.
     * @param db database name
     * @param table table name
     * @param column column name
     * @param row row identifier
     * @param rw if true, open for read-write, else read-only
     * @param blob Blob object
     */

    private native void _open_blob(String db, String table, String column,
				   long row, boolean rw, Blob blob)
	throws SQLite.Exception;

    /**
     * Establish a progress callback method which gets called after
     * N SQLite VM opcodes.
     *
     * @param n number of SQLite VM opcodes until callback is invoked
     * @param p the object implementing the progress callback method
     */

    public void progress_handler(int n, SQLite.ProgressHandler p) {
	synchronized(this) {
	    _progress_handler(n, p);
	}
    }

    private native void _progress_handler(int n, SQLite.ProgressHandler p);

    /**
     * Specify key for encrypted database. To be called
     * right after open() on SQLite3 databases.
     * Not available in public releases of SQLite.
     *
     * @param ekey the key as byte array
     */

    public void key(byte[] ekey) throws SQLite.Exception {
	synchronized(this) {
	    _key(ekey);
	}
    }

    /**
     * Specify key for encrypted database. To be called
     * right after open() on SQLite3 databases.
     * Not available in public releases of SQLite.
     *
     * @param skey the key as String
     */

    public void key(String skey) throws SQLite.Exception {
	synchronized(this) {
	    byte ekey[] = null;
	    if (skey != null && skey.length() > 0) {
		ekey = new byte[skey.length()];
		for (int i = 0; i< skey.length(); i++) {
		    char c = skey.charAt(i);
		    ekey[i] = (byte) ((c & 0xff) ^ (c >> 8));
		}
	    }
	    _key(ekey);
	}
    }

    private native void _key(byte[] ekey);

    /**
     * Change the key of a encrypted database. The
     * SQLite3 database must have been open()ed.
     * Not available in public releases of SQLite.
     *
     * @param ekey the key as byte array
     */

    public void rekey(byte[] ekey) throws SQLite.Exception {
	synchronized(this) {
	    _rekey(ekey);
	}
    }

    /**
     * Change the key of a encrypted database. The
     * SQLite3 database must have been open()ed.
     * Not available in public releases of SQLite.
     *
     * @param skey the key as String
     */

    public void rekey(String skey) throws SQLite.Exception {
	synchronized(this) {
	    byte ekey[] = null;
	    if (skey != null && skey.length() > 0) {
		ekey = new byte[skey.length()];
		for (int i = 0; i< skey.length(); i++) {
		    char c = skey.charAt(i);
		    ekey[i] = (byte) ((c & 0xff) ^ (c >> 8));
		}
	    }
	    _rekey(ekey);
	}
    }

    private native void _rekey(byte[] ekey);

    /**
     * Enable/disable shared cache mode (SQLite 3.x only).
     *
     * @param onoff boolean to enable or disable shared cache
     * @return boolean when true, function supported/succeeded
     */

    protected static native boolean _enable_shared_cache(boolean onoff);

    /**
     * Internal native initializer.
     */

    private static native void internal_init();

    /**
     * Make long value from julian date for java.lang.Date
     *
     * @param d double value (julian date in SQLite3 format)
     * @return long
     */

    public static long long_from_julian(double d) {
	d -= 2440587.5;
	d *= 86400000.0;
	return (long) d;
    }

    /**
     * Make long value from julian date for java.lang.Date
     *
     * @param s string (double value) (julian date in SQLite3 format)
     * @return long
     */

    public static long long_from_julian(String s) throws SQLite.Exception {
	try {
	    double d = Double.valueOf(s).doubleValue();
	    return long_from_julian(d);
	} catch (java.lang.Exception ee) {
	    throw new SQLite.Exception("not a julian date: " + s + ": " + ee);
	}
    }

    /**
     * Make julian date value from java.lang.Date
     *
     * @param ms millisecond value of java.lang.Date
     * @return double
     */

    public static double julian_from_long(long ms) {
	double adj = (ms < 0) ? 0 : 0.5;
	double d = (ms + adj) / 86400000.0 + 2440587.5;
	return d;
    }

    /**
     * Static initializer to load the native part.
     */

    static {
	try {
	    String path = System.getProperty("SQLite.library.path");
	    if (path == null || path.length() == 0) {
		System.loadLibrary("sqlite_jni");
	    } else {
		try {
		    java.lang.reflect.Method mapLibraryName;
		    Class param[] = new Class[1];
		    param[0] = String.class;
		    mapLibraryName = System.class.getMethod("mapLibraryName",
							    param);
		    Object args[] = new Object[1];
		    args[0] = "sqlite_jni";
		    String mapped = (String) mapLibraryName.invoke(null, args);
		    System.load(path + java.io.File.separator + mapped);
		} catch (Throwable t) {
		    System.err.println("Unable to load sqlite_jni from" +
				       "SQLite.library.path=" + path +
				       ", trying system default: " + t);
		    System.loadLibrary("sqlite_jni");
		}
	    }
	} catch (Throwable t) {
	    System.err.println("Unable to load sqlite_jni: " + t);
	}
	/*
	 * Call native initializer functions now, since the
	 * native part could have been linked statically, i.e.
	 * the try/catch above would have failed in that case.
	 */
	try {
	    internal_init();
	    new FunctionContext();
	} catch (java.lang.Exception e) {
	}
    }
}

