/*
 *  This is a sample implementation of the
 *  Transaction Processing Performance Council
 *  Benchmark B coded in Java/JDBC and ANSI SQL2.
 *
 *  This version is using one connection per
 *  thread to parallellize server operations.
 */

package SQLite;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.Enumeration;
import java.util.Vector;

public abstract class Benchmark {

    /* the tps scaling factor: here it is 1 */
    public static int tps = 1;

    /* number of branches in 1 tps db */
    public static int nbranches = 1;

    /* number of tellers in  1 tps db */
    public static int ntellers = 10;

    /* number of accounts in 1 tps db */
    public static int naccounts = 50000;

    /* number of history recs in 1 tps db */
    public static int nhistory  = 864000;

    public final static int TELLER  = 0;
    public final static int BRANCH  = 1;
    public final static int ACCOUNT = 2;

    int failed_transactions = 0;
    int transaction_count = 0;
    static int n_clients = 10;
    static int n_txn_per_client = 10;
    long start_time = 0;
    static boolean transactions = true;
    static boolean prepared_stmt = false;

    static boolean verbose = false;

    /*
     * main program
     *  creates a 1-tps database:
     *  i.e. 1 branch, 10 tellers,...
     *  runs one TPC BM B transaction
     */

    public void run(String[] args) {
        String DriverName = "";
	String DBUrl = "";
	String DBUser = "";
	String DBPassword = "";
	boolean initialize_dataset = false;

	for (int i = 0; i < args.length; i++) {
	    if (args[i].equals("-clients")) {
		if (i + 1 < args.length) {
		    i++;
		    n_clients = Integer.parseInt(args[i]);
		}
	    } else if (args[i].equals("-driver")) {
		if (i + 1 < args.length) {
		    i++;
		    DriverName = args[i];
		}
	    } else if (args[i].equals("-url")) {
		if (i + 1 < args.length) {
		    i++;
		    DBUrl = args[i];
		}
	    } else if (args[i].equals("-user")) {
		if (i + 1 < args.length) {
		    i++;
		    DBUser = args[i];
		}
	    } else if (args[i].equals("-password")) {
		if (i + 1 < args.length) {
		    i++;
		    DBPassword = args[i];
		}
	    } else if (args[i].equals("-tpc")) {
		if (i + 1 < args.length) {
		    i++;
		    n_txn_per_client = Integer.parseInt(args[i]);
		}
	    } else if (args[i].equals("-init")) {
		initialize_dataset = true;
	    } else if (args[i].equals("-tps")) {
		if (i + 1 < args.length) {
		    i++;
		    tps = Integer.parseInt(args[i]);
		}
	    } else if (args[i].equals("-v")) {
		verbose = true;
	    }
	}

	if (DriverName.length() == 0 || DBUrl.length() == 0) {
	    System.out.println("JDBC based benchmark program\n\n" +
			       "JRE usage:\n\njava SQLite.BenchmarkDriver " +
			       "-url [url_to_db] \\\n    " +
			       "[-user [username]] " +
			       "[-password [password]] " +
			       "[-driver [driver_class_name]] \\\n    " +
			       "[-v] [-init] [-tpc N] [-tps N] " +
			       "[-clients N]\n");
	    System.out.println("OJEC usage:\n\ncvm SQLite.BenchmarkDataSource " +
			       "[-user [username]] " +
			       "[-password [password]] " +
			       "[-driver [driver_class_name]] \\\n    " +
			       "[-v] [-init] [-tpc N] [-tps N] " +
			       "[-clients N]\n");
	    System.out.println();
	    System.out.println("-v          verbose mode");
	    System.out.println("-init       initialize the tables");
	    System.out.println("-tpc N      transactions per client");
	    System.out.println("-tps N      scale factor");
	    System.out.println("-clients N  number of simultaneous clients/threads");
	    System.out.println();
	    System.out.println("Default driver class is SQLite.JDBCDriver");
	    System.out.println("in this case use an -url parameter of the form");
	    System.out.println("  jdbc:sqlite:/[path]");
	    System.exit(1);
	}

	System.out.println("Driver: " + DriverName);
	System.out.println("URL:" + DBUrl);
	System.out.println();
	System.out.println("Scale factor value: " + tps);
	System.out.println("Number of clients: " + n_clients);
	System.out.println("Number of transactions per client: " +
			   n_txn_per_client);
	System.out.println();

	try {
	    benchmark(DBUrl, DBUser, DBPassword, initialize_dataset);
	} catch (java.lang.Exception e) {
	    System.out.println(e.getMessage());
	    e.printStackTrace();
	}
    }

    public void benchmark(String url, String user, String password, boolean init) {
	Vector vClient = new Vector();
	Thread Client = null;
	Enumeration en = null;
	try {
	    if (init) {
		System.out.print("Initializing dataset...");
		createDatabase(url, user, password);
		System.out.println("done.\n");
	    }

	    System.out.println("* Starting Benchmark Run *");

	    transactions = false;
	    prepared_stmt = false;
	    start_time = System.currentTimeMillis();
	    for (int i = 0; i < n_clients; i++) {
		Client = new BenchmarkThread(n_txn_per_client, url, user,
					     password, this);
		Client.start();
		vClient.addElement(Client);
	    }

	    /*
	     * Barrier to complete this test session
	     */
	    en = vClient.elements();
	    while (en.hasMoreElements()) {
		Client = (Thread) en.nextElement();
		Client.join();
	    }
	    vClient.removeAllElements();
	    reportDone();
        
	    transactions = true;
	    prepared_stmt = false;
	    start_time = System.currentTimeMillis();
	    for (int i = 0; i < n_clients; i++) {
		Client = new BenchmarkThread(n_txn_per_client, url,
					     user, password, this);
		Client.start();
		vClient.addElement(Client);
	    }
 
	    /*
	     * Barrier to complete this test session
	     */
	    en = vClient.elements();
	    while (en.hasMoreElements()) {
		Client = (Thread) en.nextElement();
		Client.join();
	    }
	    vClient.removeAllElements();
	    reportDone();
 
	    transactions = false;
	    prepared_stmt = true;
	    start_time = System.currentTimeMillis();
	    for (int i = 0; i < n_clients; i++) {
		Client = new BenchmarkThread(n_txn_per_client, url,
					     user, password, this);
		Client.start();
		vClient.addElement(Client);
	    }

	    /*
	     * Barrier to complete this test session
	     */
        
	    en = vClient.elements();
	    while (en.hasMoreElements()) {
		Client = (Thread) en.nextElement();
		Client.join();
	    }
	    vClient.removeAllElements();
	    reportDone();

	    transactions = true;
	    prepared_stmt = true;
	    start_time = System.currentTimeMillis();
	    for (int i = 0; i < n_clients; i++) {
		Client = new BenchmarkThread(n_txn_per_client, url,
					     user, password, this);
		Client.start();
		vClient.addElement(Client);
	    }
 
	    /*
	     * Barrier to complete this test session
	     */
	    en = vClient.elements();
	    while (en.hasMoreElements()) {
		Client = (Thread) en.nextElement();
		Client.join();
	    }
	    vClient.removeAllElements();
	    reportDone();

	} catch (java.lang.Exception e) {
	    System.out.println(e.getMessage());
	    e.printStackTrace();
	} finally {
	    System.exit(0);
	}
    }

    public void reportDone() {
	long end_time = System.currentTimeMillis();
	double completion_time =
	    ((double)end_time - (double)start_time) / 1000;

	System.out.println("\n* Benchmark Report *" );
	System.out.print("* Featuring ");
	if (prepared_stmt) {
	    System.out.print("<prepared statements> ");
	} else {
	    System.out.print("<direct queries> ");
	}
	if (transactions) {
	    System.out.print("<transactions> ");
	} else {
	    System.out.print("<auto-commit> ");
	}
	System.out.println("\n--------------------");
	System.out.println("Time to execute " +
			   transaction_count + " transactions: " +
			   completion_time + " seconds.");
	System.out.println(failed_transactions + " / " +
			   transaction_count + " failed to complete.");
	double rate = (transaction_count - failed_transactions)
	    / completion_time;
	System.out.println("Transaction rate: " + rate + " txn/sec.");
	transaction_count = 0;
	failed_transactions = 0;
	System.gc();
    }

    public synchronized void incrementTransactionCount() {
	transaction_count++;
    }

    public synchronized void incrementFailedTransactionCount() {
	failed_transactions++;
    }

    void createDatabase(String url, String user, String password)
	throws java.lang.Exception {
	Connection Conn = connect(url, user, password);

	String s = Conn.getMetaData().getDatabaseProductName();
	System.out.println("DBMS: "+s);

	transactions = true;
	if (transactions) {
	    try {
		Conn.setAutoCommit(false);
		System.out.println("In transaction mode");
	    } catch (SQLException etxn) {
		transactions = false;
	    }
	}

	try {
	    int accountsnb = 0;
	    Statement Stmt = Conn.createStatement();
	    String Query;
	    Query = "SELECT count(*) FROM accounts";

	    ResultSet RS = Stmt.executeQuery(Query);
	    Stmt.clearWarnings();

	    while (RS.next()) {
		accountsnb = RS.getInt(1);
	    }
	    if (transactions) {
		Conn.commit();
	    }
	    Stmt.close();
	    if (accountsnb == (naccounts*tps)) {
		System.out.println("Already initialized");
		connectClose(Conn);
		return;
	    }
	} catch (java.lang.Exception e) {
	}

	System.out.println("Drop old tables if they exist");
	try {
	    Statement Stmt = Conn.createStatement();
	    String Query;
	    Query = "DROP TABLE history";
	    Stmt.execute(Query);
	    Stmt.clearWarnings();
	    Query = "DROP TABLE accounts";
	    Stmt.execute(Query);
	    Stmt.clearWarnings();
	    Query = "DROP TABLE tellers";
	    Stmt.execute(Query);
	    Stmt.clearWarnings();
	    Query = "DROP TABLE branches";
	    Stmt.execute(Query);
	    Stmt.clearWarnings();
	    if (transactions) {
		Conn.commit();
	    }
	    Stmt.close();
	} catch (java.lang.Exception e) {
	}

	System.out.println("Creates tables");
	try {
	    Statement Stmt = Conn.createStatement();
	    String Query;

	    Query = "CREATE TABLE branches (";
	    Query += "Bid INTEGER NOT NULL PRIMARY KEY,";
	    Query += "Bbalance INTEGER,";
	    Query += "filler CHAR(88))"; /* pad to 100 bytes */

	    Stmt.execute(Query);
	    Stmt.clearWarnings();

	    Query = "CREATE TABLE tellers (";
	    Query += "Tid INTEGER NOT NULL PRIMARY KEY,";
	    Query += "Bid INTEGER,";
	    Query += "Tbalance INTEGER,";
	    Query += "filler CHAR(84))"; /* pad to 100 bytes */

	    Stmt.execute(Query);
	    Stmt.clearWarnings();

	    Query = "CREATE TABLE accounts (";
	    Query += "Aid INTEGER NOT NULL PRIMARY KEY,";
	    Query += "Bid INTEGER,";
	    Query += "Abalance INTEGER,";
	    Query += "filler CHAR(84))"; /* pad to 100 bytes */

	    Stmt.execute(Query);
	    Stmt.clearWarnings();

	    Query = "CREATE TABLE history (";
	    Query += "Tid INTEGER,";
	    Query += "Bid INTEGER,";
	    Query += "Aid INTEGER,";
	    Query += "delta INTEGER,";
	    Query += "tstime TIMESTAMP,";
	    Query += "filler CHAR(22))"; /* pad to 50 bytes  */

	    Stmt.execute(Query);
	    Stmt.clearWarnings();

	    if (transactions) {
		Conn.commit();
	    }

	    Stmt.close();

	} catch (java.lang.Exception e) {
	}

	System.out.println("Delete elements in table in case DROP didn't work");
	try {
	    Statement Stmt = Conn.createStatement();
	    String Query;

	    Query = "DELETE FROM history";
	    Stmt.execute(Query);
	    Stmt.clearWarnings();
	    Query = "DELETE FROM accounts";
	    Stmt.execute(Query);
	    Stmt.clearWarnings();
	    Query = "DELETE FROM tellers";
	    Stmt.execute(Query);
	    Stmt.clearWarnings();
	    Query = "DELETE FROM branches";
	    Stmt.execute(Query);
	    Stmt.clearWarnings();
	    if (transactions) {
		Conn.commit();
	    }

	    /*
	     * prime database using TPC BM B scaling rules.
	     *  Note that for each branch and teller:
	     *      branch_id = teller_id  / ntellers
	     *      branch_id = account_id / naccounts
	     */

	    PreparedStatement pstmt = null;
	    prepared_stmt = true;
	    if (prepared_stmt) {
		try {
		    Query = "INSERT INTO branches(Bid,Bbalance) VALUES (?,0)";
		    pstmt = Conn.prepareStatement(Query);
		    System.out.println("Using prepared statements");
		} catch (SQLException estmt) {
		    pstmt = null;
		    prepared_stmt = false;
		}
	    }
	    System.out.println("Insert data in branches table");
	    for (int i = 0; i < nbranches * tps; i++) {
		if (prepared_stmt) {
		    pstmt.setInt(1,i);
		    pstmt.executeUpdate();
		    pstmt.clearWarnings();
		} else {
		    Query = "INSERT INTO branches(Bid,Bbalance) VALUES (" +
			i + ",0)";
		    Stmt.executeUpdate(Query);
		}
		if ((i%100==0) && (transactions)) {
		    Conn.commit();
		}
	    }
	    if (prepared_stmt) {
		pstmt.close();
	    }
	    if (transactions) {
		Conn.commit();
	    }

	    if (prepared_stmt) {
		Query = "INSERT INTO tellers(Tid,Bid,Tbalance) VALUES (?,?,0)";
		pstmt = Conn.prepareStatement(Query);
	    }
	    System.out.println("Insert data in tellers table");
	    for (int i = 0; i < ntellers * tps; i++) {
		if (prepared_stmt) {
		    pstmt.setInt(1,i);
		    pstmt.setInt(2,i/ntellers);
		    pstmt.executeUpdate();
		    pstmt.clearWarnings();
		} else {
		    Query = "INSERT INTO tellers(Tid,Bid,Tbalance) VALUES (" +
			i + "," + i / ntellers + ",0)";
		    Stmt.executeUpdate(Query);
		}
		if ((i%100==0) && (transactions)) {
		    Conn.commit();
		}
	    }
	    if (prepared_stmt) {
		pstmt.close();
	    }
	    if (transactions) {
		Conn.commit();
	    }
	    if (prepared_stmt) {
		Query = "INSERT INTO accounts(Aid,Bid,Abalance) VALUES (?,?,0)";
		pstmt = Conn.prepareStatement(Query);
	    }
	    System.out.println("Insert data in accounts table");
	    for (int i = 0; i < naccounts*tps; i++) {
		if (prepared_stmt) {
		    pstmt.setInt(1,i);
		    pstmt.setInt(2,i/naccounts);
		    pstmt.executeUpdate();
		    pstmt.clearWarnings();
		} else {
		    Query = "INSERT INTO accounts(Aid,Bid,Abalance) VALUES (" +
			i + "," + i / naccounts + ",0)";
		    Stmt.executeUpdate(Query);
		}
		if ((i%10000==0) && (transactions)) {
		    Conn.commit();
		}
		if ((i>0) && ((i%10000)==0)) {
		    System.out.println("\t" + i + "\t records inserted");
		}
	    }
	    if (prepared_stmt) {
		pstmt.close();
	    }
	    if (transactions) {
		Conn.commit();
	    }
	    System.out.println("\t" + (naccounts*tps) + "\t records inserted");
	    Stmt.close();

	} catch (java.lang.Exception e) {
	    System.out.println(e.getMessage());
	    e.printStackTrace();
	}
	connectClose(Conn);
    }

    public static int getRandomInt(int lo, int hi) {
	int ret = 0;
	ret = (int)(Math.random() * (hi - lo + 1));
	ret += lo;
	return ret;
    }

    public static int getRandomID(int type) {
	int min, max, num;
	max = min = 0;
	num = naccounts;
	switch(type) {
        case TELLER:
	    min += nbranches;
	    num = ntellers;
	    /* FALLTHROUGH */
        case BRANCH:
	    if (type == BRANCH) {
		num = nbranches;
	    }
	    min += naccounts;
          /* FALLTHROUGH */
        case ACCOUNT:
	    max = min + num - 1;
        }
	return (getRandomInt(min, max));
    }

    public abstract Connection connect(String DBUrl, String DBUser,
				     String DBPassword);

    public static void connectClose(Connection c) {
	if (c == null) {
	    return;
	}
	try {
	    c.close();
	} catch (java.lang.Exception e) {
	    System.out.println(e.getMessage());
	    e.printStackTrace();
	}
    }
}

class BenchmarkThread extends Thread {
    int ntrans = 0;
    Connection Conn;

    Benchmark bench;

    PreparedStatement pstmt1 = null;
    PreparedStatement pstmt2 = null;
    PreparedStatement pstmt3 = null;
    PreparedStatement pstmt4 = null;
    PreparedStatement pstmt5 = null;

    public BenchmarkThread(int number_of_txns,String url,
			   String user, String password,
			   Benchmark b) {
	bench = b;
	ntrans = number_of_txns;
	Conn = b.connect(url, user, password);
	if (Conn == null) {
	    return;
	}
	try {
	    if (Benchmark.transactions) {
		Conn.setAutoCommit(false);
	    }
	    if (Benchmark.prepared_stmt) {
		String Query;
		Query = "UPDATE accounts";
		Query += " SET Abalance = Abalance + ?";
		Query += " WHERE Aid = ?";
		pstmt1 = Conn.prepareStatement(Query);

		Query = "SELECT Abalance";
		Query += " FROM accounts";
		Query += " WHERE Aid = ?";
		pstmt2 = Conn.prepareStatement(Query);

		Query = "UPDATE tellers";
		Query += " SET Tbalance = Tbalance + ?";
		Query += " WHERE  Tid = ?";
		pstmt3 = Conn.prepareStatement(Query);

		Query = "UPDATE branches";
		Query += " SET Bbalance = Bbalance + ?";
		Query += " WHERE  Bid = ?";
		pstmt4 = Conn.prepareStatement(Query);

		Query = "INSERT INTO history(Tid, Bid, Aid, delta)";
		Query += " VALUES (?,?,?,?)";
		pstmt5 = Conn.prepareStatement(Query);
	    }
	} catch (java.lang.Exception e) {
	    System.out.println(e.getMessage());
	    e.printStackTrace();
	}
    }

    public void run() {
	while (ntrans-- > 0) {
	    int account = Benchmark.getRandomID(Benchmark.ACCOUNT);
	    int branch = Benchmark.getRandomID(Benchmark.BRANCH);
	    int teller = Benchmark.getRandomID(Benchmark.TELLER);
	    int delta = Benchmark.getRandomInt(0, 1000);
	    doOne(branch, teller, account, delta);
	    bench.incrementTransactionCount();
	}
	if (Benchmark.prepared_stmt) {
	    try {
		if (pstmt1 != null) {
		    pstmt1.close();
		}
		if (pstmt2 != null) {
		    pstmt2.close();
		}
		if (pstmt3 != null) {
		    pstmt3.close();
		}
		if (pstmt4 != null) {
		    pstmt4.close();
		}
		if (pstmt5 != null) {
		    pstmt5.close();
		}
	    } catch (java.lang.Exception e) {
		System.out.println(e.getMessage());
		e.printStackTrace();
	    }
	}
	Benchmark.connectClose(Conn);
	Conn = null;
    }

    /*
     *  Executes a single TPC BM B transaction.
     */

    int doOne(int bid, int tid, int aid, int delta) {
	int aBalance = 0;

	if (Conn == null) {
	    bench.incrementFailedTransactionCount();
	    return 0;
	}
	try {
	    if (Benchmark.prepared_stmt) {
		pstmt1.setInt(1,delta);
		pstmt1.setInt(2,aid);
		pstmt1.executeUpdate();
		pstmt1.clearWarnings();

		pstmt2.setInt(1,aid);
		ResultSet RS = pstmt2.executeQuery();
		pstmt2.clearWarnings();

		while (RS.next()) {
		    aBalance = RS.getInt(1);
		}

		pstmt3.setInt(1,delta);
		pstmt3.setInt(2,tid);
		pstmt3.executeUpdate();
		pstmt3.clearWarnings();

		pstmt4.setInt(1,delta);
		pstmt4.setInt(2,bid);
		pstmt4.executeUpdate();
		pstmt4.clearWarnings();

		pstmt5.setInt(1,tid);
		pstmt5.setInt(2,bid);
		pstmt5.setInt(3,aid);
		pstmt5.setInt(4,delta);
		pstmt5.executeUpdate();
		pstmt5.clearWarnings();
	    } else {
		Statement Stmt = Conn.createStatement();

		String Query = "UPDATE accounts";
		Query += " SET Abalance = Abalance + " + delta;
		Query += " WHERE Aid = " + aid;

		Stmt.executeUpdate(Query);
		Stmt.clearWarnings();

		Query = "SELECT Abalance";
		Query += " FROM accounts";
		Query += " WHERE Aid = " + aid;

		ResultSet RS = Stmt.executeQuery(Query);
		Stmt.clearWarnings();

		while (RS.next()) {
		    aBalance = RS.getInt(1);
		}

		Query = "UPDATE tellers";
		Query += " SET Tbalance = Tbalance + " + delta;
		Query += " WHERE Tid = " + tid;

		Stmt.executeUpdate(Query);
		Stmt.clearWarnings();

		Query = "UPDATE branches";
		Query += " SET Bbalance = Bbalance + " + delta;
		Query += " WHERE Bid = " + bid;

		Stmt.executeUpdate(Query);
		Stmt.clearWarnings();

		Query = "INSERT INTO history(Tid, Bid, Aid, delta)";
		Query += " VALUES (";
		Query += tid + ",";
		Query += bid + ",";
		Query += aid + ",";
		Query += delta + ")";

		Stmt.executeUpdate(Query);
		Stmt.clearWarnings();

		Stmt.close();
	    }

	    if (Benchmark.transactions) {
		Conn.commit();
	    }
	    return aBalance;
	} catch (java.lang.Exception e) {
	    if (Benchmark.verbose) {
		System.out.println("Transaction failed: "
				   + e.getMessage());
		e.printStackTrace();
	    }
	    bench.incrementFailedTransactionCount();
	    if (Benchmark.transactions) {
		try {
		    Conn.rollback();
		} catch (SQLException e1) {
		}
	    }
	}
	return 0;
    }
}
