package SQLite;

import java.io.PrintStream;
import java.io.PrintWriter;

/**
 * SQLite SQL dump utility.
 */

public class SQLDump implements Callback {
    PrintWriter pw;
    PrintWriter err;
    Database db;
    Shell s;

    public SQLDump(PrintWriter pw, Database db) {
	this.pw = pw;
	this.err = this.pw;
	this.db = db;
	s = new Shell(this.pw, this.err);
	s.mode = Shell.MODE_Insert;
	s.db = db;
    }

    public SQLDump(PrintStream ps, Database db) {
	this.pw = new PrintWriter(ps);
	this.err = this.pw;
	this.db = db;
	s = new Shell(this.pw, this.err);
	s.mode = Shell.MODE_Insert;
	s.db = db;
    }

    public void dump() throws SQLite.Exception {
	pw.println("BEGIN TRANSACTION;");
	db.exec("SELECT name, type, sql FROM sqlite_master " +
		"WHERE type!='meta' AND sql NOT NULL " +
		"ORDER BY substr(type,2,1), name", this);
	pw.println("COMMIT;");
	pw.flush();
    }

    public void columns(String col[]) {
	/* Empty body to satisfy SQLite.Callback interface. */
    }

    public void types(String args[]) {
	/* Empty body to satisfy SQLite.Callback interface. */
    }

    public boolean newrow(String args[]) {
	if (args.length != 3) {
	    return true;
	}
	pw.println(args[2] + ";");
	if (args[1].compareTo("table") == 0) {
	    s.mode = Shell.MODE_Insert;
	    s.set_table_name(args[0]);
	    String qargs[] = new String[1];
	    qargs[0] = args[0];
	    try {
		if (s.db.is3()) {
		    TableResult t = null;
		    t = s.db.get_table("PRAGMA table_info('%q')", qargs);
		    String query;
		    if (t != null) {
			StringBuffer sb = new StringBuffer();
			String sep = "";

			sb.append("SELECT ");
			for (int i = 0; i < t.nrows; i++) {
			    String col = ((String[]) t.rows.elementAt(i))[1];
			    sb.append(sep + "quote(" +
				      Shell.sql_quote_dbl(col) + ")");
			    sep = ",";
			}
			sb.append(" from '%q'");
			query = sb.toString();
			s.mode = Shell.MODE_Insert2;
		    } else {
			query = "SELECT * from '%q'";
		    }
		    s.db.exec(query, s, qargs);
		    pw.flush();
		} else {
		    s.db.exec("SELECT * from '%q'", s, qargs);
		    pw.flush();
		}
	    } catch (Exception e) {
		return true;
	    }
	}
	s.mode = Shell.MODE_Line;
	return false;
    }
}
