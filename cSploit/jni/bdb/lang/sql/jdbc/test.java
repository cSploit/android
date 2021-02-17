import SQLite.*;

public class test implements SQLite.Callback, SQLite.Function,
    SQLite.Authorizer, SQLite.Trace, SQLite.ProgressHandler {

    private StringBuffer acc = new StringBuffer();

    public void columns(String col[]) {
	System.out.println("#cols = " + col.length);
	for (int i = 0; i < col.length; i++) {
	    System.out.println("col" + i + ": " + col[i]);
	}
	// throw new java.lang.RuntimeException("boom");
    }

    public void types(String types[]) {
	if (types != null) {
	    for (int i = 0; i < types.length; i++) {
	        System.out.println("coltype" + i + ": " + types[i]);
	    }
	}
    }

    public boolean newrow(String data[]) {
	for (int i = 0; i < data.length; i++) {
	    System.out.println("data" + i + ": " + data[i]);
	}
	return false;
    }

    public void function(FunctionContext fc, String args[]) {
	System.out.println("function:");
	for (int i = 0; i < args.length; i++) {
	    System.out.println("arg[" + i + "]=" + args[i]);
	}
	if (args.length > 0) {
	    fc.set_result(args[0].toLowerCase());
	}
    }

    public void step(FunctionContext fc, String args[]) {
	System.out.println("step:");
	for (int i = 0; i < args.length; i++) {
	    acc.append(args[i]);
	    acc.append(" ");
	}
    }

    public void last_step(FunctionContext fc) {
	System.out.println("last_step");
	fc.set_result(acc.toString());
	acc.setLength(0);
    }

    public int authorize(int what, String arg1, String arg2, String arg3,
			 String arg4) {
	System.out.println("AUTH: " + what + "," + arg1 + "," + arg2 + ","
			   + arg3 + "," + arg4);
	return Constants.SQLITE_OK;
    }

    public void trace(String stmt) {
	System.out.println("TRACE: " + stmt);
    }

    public boolean progress() {
	System.out.println("PROGRESS");
	return true;
    }

    public static void main(String args[]) {
	boolean error = true;
	System.out.println("LIB version: " + SQLite.Database.version());
	SQLite.Database db = new SQLite.Database();
	try {
	    db.open(args.length > 0 ? args[0] : "db", 0666);
	    System.out.println("DB version: " + db.dbversion());
	    db.interrupt();
	    db.busy_timeout(1000);
	    db.busy_handler(null);
	    db.create_function("myregfunc", -1, new test());
	    db.function_type("myregfunc", Constants.SQLITE_TEXT);
	    db.create_aggregate("myaggfunc", 1, new test());
	    db.function_type("myaggfunc", Constants.SQLITE_TEXT);
	    db.exec("PRAGMA show_datatypes = on", null);
	    System.out.println("==== local callback ====");
	    db.exec("select * from sqlite_master", new test());
	    System.out.println("==== get_table ====");
	    System.out.print(db.get_table("select * from TEST"));
	    System.out.println("==== get_table w/ args ====");
	    String qargs[] = new String[1];
	    qargs[0] = "tab%";
	    System.out.print(db.get_table("select * from sqlite_master where type like '%q'", qargs));
	    System.out.println("==== call regular function ====");
	    db.exec("select myregfunc(TEST.firstname) from TEST", new test());
	    System.out.println("==== call aggregate function ====");
	    db.exec("select myaggfunc(TEST.firstname) from TEST", new test());
	    System.out.println("==== compile/step interface ====");
	    test cb = new test();
	    db.set_authorizer(cb);
	    db.trace(cb);
	    Vm vm = db.compile("select * from TEST; " +
			       "delete from TEST where id = 5; " +
			       "insert into TEST values(5, 'Speedy', 'Gonzales'); " +
			       "select * from TEST");
	    int stmt = 0;
	    do {
		++stmt;
		if (stmt > 3) {
		    System.out.println("setting progress handler");
		    db.progress_handler(3, cb);
		}
		System.out.println("---- STMT #" + stmt + " ----");
		while (vm.step(cb)) {
		}
	    } while (vm.compile());
	    db.close();
	    error = false;
	    System.out.println("An exception is expected from now on.");
	    System.out.println("==== local callback ====");
	    db.exec("select * from sqlite_master", new test());
	} catch (java.lang.Exception e) {
	    System.err.println("error: " + e);
	} finally {
	    try {
		System.err.println("cleaning up ...");
		db.close();
	    } catch(java.lang.Exception e) {
		System.err.println("error: " + e);
	    } finally {
		System.err.println("done.");
	    }
	}
	if (error) {
	    System.exit(1);
	}	
    }
}
