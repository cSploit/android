package SQLite;

/**
 * Callback interface for SQLite's query results.
 * <BR><BR>
 * Example:<BR>
 *
 * <PRE>
 *   class TableFmt implements SQLite.Callback {
 *     public void columns(String cols[]) {
 *       System.out.println("&lt;TH&gt;&lt;TR&gt;");
 *       for (int i = 0; i &lt; cols.length; i++) {
 *         System.out.println("&lt;TD&gt;" + cols[i] + "&lt;/TD&gt;");
 *       }
 *       System.out.println("&lt;/TR&gt;&lt;/TH&gt;");
 *     }
 *     public boolean newrow(String cols[]) {
 *       System.out.println("&lt;TR&gt;");
 *       for (int i = 0; i &lt; cols.length; i++) {
 *         System.out.println("&lt;TD&gt;" + cols[i] + "&lt;/TD&gt;");
 *       }
 *       System.out.println("&lt;/TR&gt;");
 *       return false;
 *     }
 *   }
 *   ...
 *   SQLite.Database db = new SQLite.Database();
 *   db.open("db", 0);
 *   System.out.println("&lt;TABLE&gt;");
 *   db.exec("select * from TEST", new TableFmt());
 *   System.out.println("&lt;/TABLE&gt;");
 *   ...
 * </PRE>
 */

public interface Callback {

    /**
     * Reports column names of the query result.
     * This method is invoked first (and once) when
     * the SQLite engine returns the result set.<BR><BR>
     *
     * @param coldata string array holding the column names
     */

    public void columns(String coldata[]);

    /**
     * Reports type names of the columns of the query result.
     * This is available from SQLite 2.6.0 on and needs
     * the PRAGMA show_datatypes to be turned on.<BR><BR>
     *
     * @param types string array holding column types
     */

    public void types(String types[]);

    /**
     * Reports row data of the query result.
     * This method is invoked for each row of the
     * result set. If true is returned the running
     * SQLite query is aborted.<BR><BR>
     *
     * @param rowdata string array holding the column values of the row
     */

    public boolean newrow(String rowdata[]);
}
