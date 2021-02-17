package SQLite;

import java.util.Vector;

/**
 * Class representing an SQLite result set as
 * returned by the
 * <A HREF="Database.html#get_table(java.lang.String)">Database.get_table</A>
 * convenience method.
 * <BR><BR>
 * Example:<BR>
 *
 * <PRE>
 *   ...
 *   SQLite.Database db = new SQLite.Database();
 *   db.open("db", 0);
 *   System.out.print(db.get_table("select * from TEST"));
 *   ...
 * </PRE>
 * Example output:<BR>
 *
 * <PRE>
 *   id|firstname|lastname|
 *   0|John|Doe|
 *   1|Speedy|Gonzales|
 *   ...
 * </PRE>
 */

public class TableResult implements Callback {

    /**
     * Number of columns in the result set.
     */

    public int ncolumns;

    /**
     * Number of rows in the result set.
     */

    public int nrows;

    /**
     * Column names of the result set.
     */

    public String column[];

    /**
     * Types of columns of the result set or null.
     */

    public String types[];

    /**
     * Rows of the result set. Each row is stored as a String array.
     */

    public Vector rows;

    /**
     * Maximum number of rows to hold in the table.
     */

    public int maxrows = 0;

    /**
     * Flag to indicate Maximum number of rows condition.
     */

    public boolean atmaxrows;

    /**
     * Create an empty result set.
     */

    public TableResult() {
	clear();
    }

    /**
     * Create an empty result set with maximum number of rows.
     */

    public TableResult(int maxrows) {
	this.maxrows = maxrows;
	clear();
    }

    /**
     * Clear result set.
     */

    public void clear() {
	column = new String[0];
	types = null;
	rows = new Vector();
	ncolumns = nrows = 0;
	atmaxrows = false;
    }

    /**
     * Callback method used while the query is executed.
     */

    public void columns(String coldata[]) {
	column = coldata;
	ncolumns = column.length;
    }

    /**
     * Callback method used while the query is executed.
     */

    public void types(String types[]) {
	this.types = types;
    }

    /**
     * Callback method used while the query is executed.
     */

    public boolean newrow(String rowdata[]) {
	if (rowdata != null) {
	    if (maxrows > 0 && nrows >= maxrows) {
		atmaxrows = true;
		return true;
	    }
	    rows.addElement(rowdata);
	    nrows++;
	}
	return false;
    }

    /**
     * Make String representation of result set.
     */

    public String toString() {
	StringBuffer sb = new StringBuffer();
	int i;
	for (i = 0; i < ncolumns; i++) {
	    sb.append(column[i] == null ? "NULL" : column[i]);
	    sb.append('|');
	}
	sb.append('\n');
	for (i = 0; i < nrows; i++) {
	    int k;
	    String row[] = (String[]) rows.elementAt(i);
	    for (k = 0; k < ncolumns; k++) {
		sb.append(row[k] == null ? "NULL" : row[k]);
		sb.append('|');
	    }
	    sb.append('\n');
	}
	return sb.toString();
    }
}
