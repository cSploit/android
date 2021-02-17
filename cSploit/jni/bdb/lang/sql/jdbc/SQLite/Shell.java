package SQLite;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.util.StringTokenizer;

/**
 * SQLite command line shell. This is a partial reimplementation
 * of sqlite/src/shell.c and can be invoked by:<P>
 *
 * <verb>
 *     java SQLite.Shell [OPTIONS] database [SHELLCMD]
 * or
 *     java -jar sqlite.jar [OPTIONS] database [SHELLCMD]
 * </verb>
 */

public class Shell implements Callback {
    Database db;
    boolean echo;
    int count;
    int mode;
    boolean showHeader;
    String tableName;
    String sep;
    String cols[];
    int colwidth[];
    String destTable;
    PrintWriter pw;
    PrintWriter err;

    static final int MODE_Line = 0;
    static final int MODE_Column = 1;
    static final int MODE_List = 2;
    static final int MODE_Semi = 3;
    static final int MODE_Html = 4;
    static final int MODE_Insert = 5;
    static final int MODE_Insert2 = 6;

    public Shell(PrintWriter pw, PrintWriter err) {
	this.pw = pw;
	this.err = err;
    }

    public Shell(PrintStream ps, PrintStream errs) {
	pw = new PrintWriter(ps);
	err = new PrintWriter(errs);
    }

    protected Object clone() {
        Shell s = new Shell(this.pw, this.err);
	s.db = db;
	s.echo = echo;
	s.mode = mode;
	s.count = 0;
	s.showHeader = showHeader;
	s.tableName = tableName;
	s.sep = sep;
	s.colwidth = colwidth;
	return s;
    }

    static public String sql_quote_dbl(String str) {
	if (str == null) {
	    return "NULL";
	}
	int i, single = 0, dbl = 0;
	for (i = 0; i < str.length(); i++) {
	    if (str.charAt(i) == '\'') {
		single++;
	    } else if (str.charAt(i) == '"') {
		dbl++;
	    }
	}
	if (dbl == 0) {
	    return "\"" + str + "\"";
	}
	StringBuffer sb = new StringBuffer("\"");
	for (i = 0; i < str.length(); i++) {
	    char c = str.charAt(i);
	    if (c == '"') {
		sb.append("\"\"");
	    } else {
		sb.append(c);
	    }
	}
	return sb.toString();
    }

    static public String sql_quote(String str) {
	if (str == null) {
	    return "NULL";
	}
	int i, single = 0, dbl = 0;
	for (i = 0; i < str.length(); i++) {
	    if (str.charAt(i) == '\'') {
		single++;
	    } else if (str.charAt(i) == '"') {
		dbl++;
	    }
	}
	if (single == 0) {
	    return "'" + str + "'";
	}
	if (dbl == 0) {
	    return "\"" + str + "\"";
	}
	StringBuffer sb = new StringBuffer("'");
	for (i = 0; i < str.length(); i++) {
	    char c = str.charAt(i);
	    if (c == '\'') {
		sb.append("''");
	    } else {
		sb.append(c);
	    }
	}
	return sb.toString();
    }

    static String html_quote(String str) {
	if (str == null) {
	    return "NULL";
	}
	StringBuffer sb = new StringBuffer();
	for (int i = 0; i < str.length(); i++) {
	    char c = str.charAt(i);
	    if (c == '<') {
		sb.append("&lt;");
	    } else if (c == '>') {
		sb.append("&gt;");
	    } else if (c == '&') {
		sb.append("&amp;");
	    } else {
		int x = c;
		if (x < 32 || x > 127) {
		    sb.append("&#" + x + ";");
		} else {
		    sb.append(c);
		}
	    }
	}
	return sb.toString();
    }

    static boolean is_numeric(String str) {
	try {
	    Double d = Double.valueOf(str);
	} catch (java.lang.Exception e) {
	    return false;
	}
	return true;
    }

    void set_table_name(String str) {
	if (str == null) {
	    tableName = "";
	    return;
	}
	if (db.is3()) {
	    tableName = Shell.sql_quote_dbl(str);
	} else {
	    tableName = Shell.sql_quote(str);
	}
    }

    public void columns(String args[]) {
	cols = args;
    }

    public void types(String args[]) {
	/* Empty body to satisfy SQLite.Callback interface. */
    }

    public boolean newrow(String args[]) {
	int i;
	String tname;
	switch (mode) {
	case Shell.MODE_Line:
	    if (args.length == 0) {
		break;
	    }
	    if (count++ > 0) {
		pw.println("");
	    }
	    for (i = 0; i < args.length; i++) {
		pw.println(cols[i] + " = " +
			   args[i] == null ? "NULL" : args[i]);
	    }
	    break;
	case Shell.MODE_Column:
	    String csep = "";
	    if (count++ == 0) {
		colwidth = new int[args.length];
		for (i = 0; i < args.length; i++) {
		    int w;
		    w = cols[i].length();
		    if (w < 10) {
			w = 10;
		    }
		    colwidth[i] = w;
		    if (showHeader) {
			pw.print(csep + cols[i]);
			csep = " ";
		    }
		}
		if (showHeader) {
		    pw.println("");
		}
	    }
	    if (args.length == 0) {
		break;
	    }
	    csep = "";
	    for (i = 0; i < args.length; i++) {
		pw.print(csep + (args[i] == null ? "NULL" : args[i]));
		csep = " ";
	    }
	    pw.println("");
	    break;
	case Shell.MODE_Semi:
	case Shell.MODE_List:
	    if (count++ == 0 && showHeader) {
		for (i = 0; i < args.length; i++) {
		    pw.print(cols[i] +
			     (i == args.length - 1 ? "\n" : sep));
		}
	    }
	    if (args.length == 0) {
		break;
	    }
	    for (i = 0; i < args.length; i++) {
		pw.print(args[i] == null ? "NULL" : args[i]);
		if (mode == Shell.MODE_Semi) {
		    pw.print(";");
		} else if (i < args.length - 1) {
		    pw.print(sep);
		}
	    }
	    pw.println("");
	    break;
	case MODE_Html:
	    if (count++ == 0 && showHeader) {
		pw.print("<TR>");
		for (i = 0; i < args.length; i++) {
		    pw.print("<TH>" + html_quote(cols[i]) + "</TH>");
		}
		pw.println("</TR>");
	    }
	    if (args.length == 0) {
		break;
	    }
	    pw.print("<TR>");
	    for (i = 0; i < args.length; i++) {
		pw.print("<TD>" + html_quote(args[i]) + "</TD>");
	    }
	    pw.println("</TR>");
	    break;
	case MODE_Insert:
	    if (args.length == 0) {
		break;
	    }
	    tname = tableName;
	    if (destTable != null) {
	        tname = destTable;
	    }
	    pw.print("INSERT INTO " + tname + " VALUES(");
	    for (i = 0; i < args.length; i++) {
	        String tsep = i > 0 ? "," : "";
		if (args[i] == null) {
		    pw.print(tsep + "NULL");
		} else if (is_numeric(args[i])) {
		    pw.print(tsep + args[i]);
		} else {
		    pw.print(tsep + sql_quote(args[i]));
		}
	    }
	    pw.println(");");
	    break;
	case MODE_Insert2:
	    if (args.length == 0) {
		break;
	    }
	    tname = tableName;
	    if (destTable != null) {
	        tname = destTable;
	    }
	    pw.print("INSERT INTO " + tname + " VALUES(");
	    for (i = 0; i < args.length; i++) {
	        String tsep = i > 0 ? "," : "";
		pw.print(tsep + args[i]);
	    }
	    pw.println(");");
	    break;
	}
	return false;
    }

    void do_meta(String line) {
        StringTokenizer st = new StringTokenizer(line.toLowerCase());
	int n = st.countTokens();
	if (n <= 0) {
	    return;
	}
	String cmd = st.nextToken();
	String args[] = new String[n - 1];
	int i = 0;
	while (st.hasMoreTokens()) {
	    args[i] = st.nextToken();
	    ++i;
	}
	if (cmd.compareTo(".dump") == 0) {
	    new DBDump(this, args);
	    return;
	}
	if (cmd.compareTo(".echo") == 0) {
	    if (args.length > 0 &&
		(args[0].startsWith("y") || args[0].startsWith("on"))) {
		echo = true;
	    }
	    return;
	}
	if (cmd.compareTo(".exit") == 0) {
	    try {
		db.close();
	    } catch (Exception e) {
	    }
	    System.exit(0);
	}
	if (cmd.compareTo(".header") == 0) {
	    if (args.length > 0 &&
		(args[0].startsWith("y") || args[0].startsWith("on"))) {
		showHeader = true;
	    }
	    return;
	}
	if (cmd.compareTo(".help") == 0) {
	    pw.println(".dump ?TABLE? ...  Dump database in text fmt");
	    pw.println(".echo ON|OFF       Command echo on or off");
	    pw.println(".enc ?NAME?        Change encoding");
	    pw.println(".exit              Exit program");
	    pw.println(".header ON|OFF     Display headers on or off");
	    pw.println(".help              This message");
	    pw.println(".mode MODE         Set output mode to\n" +
		       "                   line, column, insert\n" +
		       "                   list, or html");
	    pw.println(".mode insert TABLE Generate SQL insert stmts");
	    pw.println(".schema ?PATTERN?  List table schema");
	    pw.println(".separator STRING  Set separator string");
	    pw.println(".tables ?PATTERN?  List table names");
	    return;
	}
	if (cmd.compareTo(".mode") == 0) {
	    if (args.length > 0) {
		if (args[0].compareTo("line") == 0) {
		    mode = Shell.MODE_Line;
		} else if (args[0].compareTo("column") == 0) {
		    mode = Shell.MODE_Column;
		} else if (args[0].compareTo("list") == 0) {
		    mode = Shell.MODE_List;
		} else if (args[0].compareTo("html") == 0) {
		    mode = Shell.MODE_Html;
		} else if (args[0].compareTo("insert") == 0) {
		    mode = Shell.MODE_Insert;
		    if (args.length > 1) {
			destTable = args[1];
		    }
		}
	    }
	    return;
	}
	if (cmd.compareTo(".separator") == 0) {
	    if (args.length > 0) {
		sep = args[0];
	    }
	    return;
	}
	if (cmd.compareTo(".tables") == 0) {
	    TableResult t = null;
	    if (args.length > 0) {
		try {
		    String qarg[] = new String[1];
		    qarg[0] = args[0];
		    t = db.get_table("SELECT name FROM sqlite_master " +
				     "WHERE type='table' AND " +
				     "name LIKE '%%%q%%' " +
				     "ORDER BY name", qarg);
		} catch (Exception e) {
		    err.println("SQL Error: " + e);
		    err.flush();
		}
	    } else {
		try {
		    t = db.get_table("SELECT name FROM sqlite_master " +
				     "WHERE type='table' ORDER BY name");
		} catch (Exception e) {
		    err.println("SQL Error: " + e);
		    err.flush();
		}
	    }
	    if (t != null) {
		for (i = 0; i < t.nrows; i++) {
		    String tab = ((String[]) t.rows.elementAt(i))[0];
		    if (tab != null) {
			pw.println(tab);
		    }
		}
	    }
	    return;
	}
	if (cmd.compareTo(".schema") == 0) {
	    if (args.length > 0) {
		try {
		    String qarg[] = new String[1];
		    qarg[0] = args[0];
		    db.exec("SELECT sql FROM sqlite_master " +
			    "WHERE type!='meta' AND " +
			    "name LIKE '%%%q%%' AND " +
			    "sql NOTNULL " +
			    "ORDER BY type DESC, name",
			    this, qarg);
		} catch (Exception e) {
		    err.println("SQL Error: " + e);
		    err.flush();
		}
	    } else {
		try {
		    db.exec("SELECT sql FROM sqlite_master " +
			    "WHERE type!='meta' AND " +
			    "sql NOTNULL " +
			    "ORDER BY tbl_name, type DESC, name",
			    this);
		} catch (Exception e) {
		    err.println("SQL Error: " + e);
		    err.flush();
		}
	    }
	    return;
	}
	if (cmd.compareTo(".enc") == 0) {
	    try {
		db.set_encoding(args.length > 0 ? args[0] : null);
	    } catch (Exception e) {
		err.println("" + e);
		err.flush();
	    }
	    return;
	}
	if (cmd.compareTo(".rekey") == 0) {
	    try {
		db.rekey(args.length > 0 ? args[0] : null);
	    } catch (Exception e) {
		err.println("" + e);
		err.flush();
	    }
	    return;
	}
	err.println("Unknown command '" + cmd + "'");
	err.flush();
    }

    String read_line(BufferedReader is, String prompt) {
	try {
	    if (prompt != null) {
		pw.print(prompt);
		pw.flush();
	    }
	    String line = is.readLine();
	    return line;
	} catch (IOException e) {
	    return null;
	}
    }

    void do_input(BufferedReader is) {
	String line, sql = null;
	String prompt = "SQLITE> ";
	while ((line = read_line(is, prompt)) != null) {
	    if (echo) {
		pw.println(line);
	    }
	    if (line.length() > 0 && line.charAt(0) == '.') {
	        do_meta(line);
	    } else {
		if (sql == null) {
		    sql = line;
		} else {
		    sql = sql + " " + line;
		}
		if (Database.complete(sql)) {
		    try {
			db.exec(sql, this);
		    } catch (Exception e) {
			if (!echo) {
			    err.println(sql);
			}
			err.println("SQL Error: " + e);
			err.flush();
		    }
		    sql = null;
		    prompt = "SQLITE> ";
		} else {
		    prompt = "SQLITE? ";
		}
	    }
	    pw.flush();
	}
	if (sql != null) {
	    err.println("Incomplete SQL: " + sql);
	    err.flush();
	}
    }

    void do_cmd(String sql) {
        if (db == null) {
	    return;
	}
        if (sql.length() > 0 && sql.charAt(0) == '.') {
	    do_meta(sql);
	} else {
	    try {
	        db.exec(sql, this);
	    } catch (Exception e) {
		err.println("SQL Error: " + e);
		err.flush();
	    }
	}
    }

    public static void main(String args[]) {
	String key = null;
	Shell s = new Shell(System.out, System.err);
	s.mode = Shell.MODE_List;
	s.sep = "|";
	s.showHeader = false;
	s.db = new Database();
	String dbname = null, sql = null;
	for (int i = 0; i < args.length; i++) {
	    if(args[i].compareTo("-html") ==0) {
		s.mode = Shell.MODE_Html;
	    } else if (args[i].compareTo("-list") == 0) {
		s.mode = Shell.MODE_List;
	    } else if (args[i].compareTo("-line") == 0) {
		s.mode = Shell.MODE_Line;
	    } else if (i < args.length - 1 &&
		       args[i].compareTo("-separator") == 0) {
		++i;
		s.sep = args[i];
	    } else if (args[i].compareTo("-header") == 0) {
		s.showHeader = true;
	    } else if (args[i].compareTo("-noheader") == 0) {
		s.showHeader = false;
	    } else if (args[i].compareTo("-echo") == 0) {
		s.echo = true;
	    } else if (args[i].compareTo("-key") == 0) {
		++i;
		key = args[i];
	    } else if (dbname == null) {
		dbname = args[i];
	    } else if (sql == null) {
		sql = args[i];
	    } else {
		System.err.println("Arguments: ?OPTIONS? FILENAME ?SQL?");
		System.exit(1);
	    }
	}
	if (dbname == null) {
	    System.err.println("No database file given");
	    System.exit(1);
	}
	try {
	    s.db.open(dbname, SQLite.Constants.SQLITE_OPEN_READWRITE |
		      SQLite.Constants.SQLITE_OPEN_CREATE);
	} catch (Exception e) {
	    System.err.println("Unable to open database: " + e);
	    System.exit(1);
	}
	if (key != null) {
	    try {
		s.db.key(key);
	    } catch (Exception e) {
		System.err.println("Unable to set key: " + e);
		System.exit(1);
	    }
	}
	if (sql != null) {
	    s.do_cmd(sql);
	    s.pw.flush();
	} else {
	    BufferedReader is =
		new BufferedReader(new InputStreamReader(System.in));
	    s.do_input(is);
	    s.pw.flush();
	}
	try {
	    s.db.close();
	} catch (Exception ee) {
	}
    }
}

/**
 * Internal class for dumping an entire database.
 * It contains a special callback interface to traverse the
 * tables of the current database and output create SQL statements
 * and for the data insert SQL statements.
 */

class DBDump implements Callback {
    Shell s;

    DBDump(Shell s, String tables[]) {
        this.s = s;
	s.pw.println("BEGIN TRANSACTION;");
        if (tables == null || tables.length == 0) {
	    try {
	        s.db.exec("SELECT name, type, sql FROM sqlite_master " +
			  "WHERE type!='meta' AND sql NOT NULL " +
			  "ORDER BY substr(type,2,1), name", this);
	    } catch (Exception e) {
	        s.err.println("SQL Error: " + e);
		s.err.flush();
	    }
	} else {
	    String arg[] = new String[1];
	    for (int i = 0; i < tables.length; i++) {
	        arg[0] = tables[i];
		try {
		    s.db.exec("SELECT name, type, sql FROM sqlite_master " +
			      "WHERE tbl_name LIKE '%q' AND type!='meta' " +
			      " AND sql NOT NULL " +
			      " ORDER BY substr(type,2,1), name",
			      this, arg);
		} catch (Exception e) {
		    s.err.println("SQL Error: " + e);
		    s.err.flush();
		}
	    }
	}
	s.pw.println("COMMIT;");
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
	s.pw.println(args[2] + ";");
	if (args[1].compareTo("table") == 0) {
	    Shell s2 = (Shell) s.clone();
	    s2.mode = Shell.MODE_Insert;
	    s2.set_table_name(args[0]);
	    String qargs[] = new String[1];
	    qargs[0] = args[0];
	    try {
	        if (s2.db.is3()) {
		    TableResult t = null;
		    t = s2.db.get_table("PRAGMA table_info('%q')", qargs);
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
			s2.mode = Shell.MODE_Insert2;
		    } else {
		        query = "SELECT * from '%q'";
		    }
		    s2.db.exec(query, s2, qargs);
		} else {
		    s2.db.exec("SELECT * from '%q'", s2, qargs);
		}
	    } catch (Exception e) {
	        s.err.println("SQL Error: " + e);
		s.err.flush();
		return true;
	    }
	}
	return false;
    }
}
