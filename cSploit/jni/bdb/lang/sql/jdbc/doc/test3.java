import SQLite.*;
import java.io.*;

public class test3 implements SQLite.Trace, SQLite.Profile {

    public void trace(String stmt) {
	System.out.println("TRACE: " + stmt);
    }

    public void profile(String stmt, long est) {
	System.out.println("PROFILE(" + est + "): " + stmt);
    }

    SQLite.Stmt prep_ins(SQLite.Database db, String sql)
	throws SQLite.Exception {
	return db.prepare(sql);
    }

    void do_ins(SQLite.Stmt stmt) throws SQLite.Exception {
	stmt.reset();
	while (stmt.step()) {
	}
    }

    void do_ins(SQLite.Stmt stmt, int i, double d, String t, byte[] b)
        throws SQLite.Exception {
	stmt.reset();
	stmt.bind(1, i);
	stmt.bind(2, d);
	stmt.bind(3, t);
	stmt.bind(4, b);
	while (stmt.step()) {
	}
    }

    void do_exec(SQLite.Database db, String sql) throws SQLite.Exception {
	SQLite.Stmt stmt = db.prepare(sql);
	while (stmt.step()) {
	}
	stmt.close();
    }

    void do_select(SQLite.Database db, String sql) throws SQLite.Exception {
	SQLite.Stmt stmt = db.prepare(sql);
	int row = 0;
	while (stmt.step()) {
	    int i, ncol = stmt.column_count();
	    System.out.println("=== ROW " + row + "===");
	    for (i = 0; i < ncol; i++) {
		try {
		    System.out.print(stmt.column_database_name(i) + "." +
				     stmt.column_table_name(i) + "." +
				     stmt.column_origin_name(i) + "<" +
				     stmt.column_decltype(i) + ">=");
		} catch (SQLite.Exception se) {
		    System.out.print("COLUMN#" + i + "<" +
				     stmt.column_decltype(i) + ">=");
		}
		Object obj = stmt.column(i);
		if (obj == null) {
		    System.out.println("null<null>");
		} else if (obj instanceof byte[]) {
		    byte[] b = (byte[]) obj;
		    String sep = "";
		    System.out.print("{");
		    for (i = 0; i < b.length; i++) {
			System.out.print(sep + b[i]);
			sep = ",";
		    }
		    System.out.print("}");
		} else {
		    System.out.print(obj.toString());
		}
		if (obj != null) {
		    System.out.println("<" + obj.getClass().getName() + ">");
		}
	    }
	    row++;
	}
	stmt.close();
    }

    public static void main(String args[]) {
	boolean error = true;
        test3 T = new test3();
	System.out.println("LIB version: " + SQLite.Database.version());
	SQLite.Database db = new SQLite.Database();
	try {
	    byte[] b;
	    db.open("db3", 0666);
	    System.out.println("DB version: " + db.dbversion());
	    db.busy_timeout(1000);
	    db.busy_handler(null);
	    db.trace(T);
	    db.profile(T);
	    T.do_exec(db, "create table TEST3("+
		      "i integer, d double, t text, b blob)");
	    T.do_exec(db, "create table B(" +
		      "id integer primary key, val blob)");
	    T.do_select(db, "select * from sqlite_master");
	    Stmt ins = T.prep_ins(db, "insert into TEST3(i,d,t,b)" +
				  " VALUES(:one,:two,:three,:four)");
	    System.out.println("INFO: " + ins.bind_parameter_count() +
			       " parameters");
	    for (int i = 1; i <= ins.bind_parameter_count(); i++) {
		String name = ins.bind_parameter_name(i);
		if (name != null) {
		    System.out.println("INFO: name for " + i + " is " + name);
		    System.out.println("INFO: index of " + name + " is " +
				       ins.bind_parameter_index(name));
		}
	    }
	    b = new byte[4];
	    b[0] = 1; b[1] = 1; b[2] = 2; b[3] = 3;
	    T.do_ins(ins, 1, 2.4, "two point four", b);
	    T.do_ins(ins);
	    b[0] = -1; b[1] = -2; b[2] = -3; b[3] = -4;
	    T.do_ins(ins, 2, 4.8, "four point eight", b);
	    T.do_ins(ins);
	    T.do_ins(ins, 3, -3.333, null, null);
	    ins.close();
	    T.do_select(db, "select * from TEST3");
	    T.do_exec(db, "insert into B values(NULL, zeroblob(128))");
	    T.do_exec(db, "insert into B values(NULL, zeroblob(128))");
	    T.do_exec(db, "insert into B values(NULL, zeroblob(128))");
	    T.do_select(db, "select id from B");
	    byte[] b128 = new byte[128];
	    for (int i = 0; i < b128.length; i++) {
		b128[i] = (byte) i;
	    }
	    Blob blob = db.open_blob("main", "B", "val", 1, true);
	    OutputStream os = blob.getOutputStream();
	    os.write(b128);
	    os.close();
	    blob.close();
	    blob = db.open_blob("main", "B", "val", 3, true);
	    os = blob.getOutputStream();
	    os.write(b128);
	    os.close();
	    blob.close();
	    T.do_select(db, "select * from B");
	    blob = db.open_blob("main", "B", "val", 1, false);
	    InputStream is = blob.getInputStream();
	    is.skip(96);
	    is.read(b);
	    is.close();
	    blob.close();
	    System.out.println("INFO: expecting {96,97,98,99} got {" +
			       b[0] + "," + b[1] + "," +
			       b[2] + "," + b[3] + "}");
	    System.out.println("INFO: backup begin");
	    try {
	        SQLite.Database dbdest = new SQLite.Database();
		dbdest.open("db3-backup", 0666);
		dbdest.busy_timeout(1000);
		dbdest.busy_handler(null);
		Backup backup = db.backup(dbdest, "main", "main");
		while (!backup.step(1)) {
		    System.out.println("INFO: backup step: "
				       + backup.remaining() + "/"
				       + backup.pagecount());
		}
		b = null;
		System.out.println("INFO: backup end");
	    } catch (java.lang.Exception ee) {
		System.err.println("error: " + ee);
	    }
	    int info[] = new int[2];
	    SQLite.Database.status(SQLite.Constants.SQLITE_STATUS_MEMORY_USED, info, false);
	    System.out.println("INFO: status(STATUS_MEMORY_USED) = "
			       + info[0] + "/" + info[1]);
	    SQLite.Database.status(SQLite.Constants.SQLITE_STATUS_MALLOC_SIZE, info, false);
	    System.out.println("INFO: status(STATUS_MALLOC_SIZE) = "
			       + info[0] + "/" + info[1]);
	    SQLite.Database.status(SQLite.Constants.SQLITE_STATUS_PAGECACHE_USED, info, false);
	    System.out.println("INFO: status(STATUS_PAGECACHE_USED) = "
			       + info[0] + "/" + info[1]);
	    SQLite.Database.status(SQLite.Constants.SQLITE_STATUS_PAGECACHE_OVERFLOW, info, false);
	    System.out.println("INFO: status(STATUS_PAGECACHE_OVERFLOW) = "
			       + info[0] + "/" + info[1]);
	    SQLite.Database.status(SQLite.Constants.SQLITE_STATUS_PAGECACHE_SIZE, info, false);
	    System.out.println("INFO: status(STATUS_PAGECACHE_SIZE) = "
			       + info[0] + "/" + info[1]);
	    SQLite.Database.status(SQLite.Constants.SQLITE_STATUS_SCRATCH_USED, info, false);
	    System.out.println("INFO: status(STATUS_SCRATCH_USED) = "
			       + info[0] + "/" + info[1]);
	    SQLite.Database.status(SQLite.Constants.SQLITE_STATUS_SCRATCH_OVERFLOW, info, false);
	    System.out.println("INFO: status(STATUS_SCRATCH_OVERFLOW) = "
			       + info[0] + "/" + info[1]);
	    SQLite.Database.status(SQLite.Constants.SQLITE_STATUS_SCRATCH_SIZE, info, false);
	    System.out.println("INFO: status(STATUS_SCRATCH_SIZE) = "
			       + info[0] + "/" + info[1]);
	    SQLite.Database.status(SQLite.Constants.SQLITE_STATUS_PARSER_STACK, info, false);
	    System.out.println("INFO: status(STATUS_PARSER_STACK) = "
			       + info[0] + "/" + info[1]);
	    db.db_status(SQLite.Constants.SQLITE_DBSTATUS_LOOKASIDE_USED, info, false);
	    System.out.println("INFO: db_status(DBSTATUS_LOOKASIZE_USED) = "
			       + info[0] + "/" + info[1]);
	    db.db_status(SQLite.Constants.SQLITE_DBSTATUS_CACHE_USED, info, false);
	    System.out.println("INFO: db_status(DBSTATUS_CACHE_USED) = "
			       + info[0] + "/" + info[1]);
	    T.do_exec(db, "drop table TEST3");
	    T.do_exec(db, "drop table B");
	    T.do_select(db, "select * from sqlite_master");
	    error = false;
	} catch (java.lang.Exception e) {
	    System.err.println("error: " + e);
	    e.printStackTrace();
	} finally {
	    try {
		System.err.println("cleaning up ...");
		try {
		    T.do_exec(db, "drop table TEST3");
		    T.do_exec(db, "drop table B");
		} catch(SQLite.Exception e) {
		}
		db.close();
	    } catch(java.lang.Exception e) {
		System.err.println("error: " + e);
		error = true;
	    } finally {
		System.err.println("done.");
	    }
	}
	if (error) {
	    System.exit(1);
	}
    }
}
