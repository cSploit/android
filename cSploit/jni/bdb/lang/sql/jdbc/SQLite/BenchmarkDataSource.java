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
import javax.sql.DataSource;

import SQLite.JDBCDataSource;

import java.util.*;
import java.io.*;

public class BenchmarkDataSource extends Benchmark {

    /*
     * main program
     *  creates a 1-tps database:
     *  i.e. 1 branch, 10 tellers,...
     *  runs one TPC BM B transaction
     */

    public static void main(String[] args) {
	try {
            new BenchmarkDataSource().run(args);
	} catch (java.lang.Exception e) {
	    System.out.println(e.getMessage());
	    e.printStackTrace();
	}
    }

    public Connection connect(String DBUrl, String DBUser,
				     String DBPassword) {
	try {
	    Connection conn =
		new JDBCDataSource(DBUrl).getConnection(DBUser, DBPassword);
	    return conn;
	} catch (java.lang.Exception e) {
	    System.out.println(e.getMessage());
	    e.printStackTrace();
	}
	return null;
    }

}
