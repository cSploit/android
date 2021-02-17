package SQLite;

/**
 * Callback interface for SQLite's user defined functions.
 * Each callback method receives a
 * <A HREF="FunctionContext.html">FunctionContext</A> object
 * which is used to set the function result or error code.
 * <BR><BR>
 * Example:<BR>
 *
 * <PRE>
 *   class SinFunc implements SQLite.Function {
 *     public void function(SQLite.FunctionContext fc, String args[]) {
 *       try {
 *         Double d = new Double(args[0]);
 *         fc.set_result(Math.sin(d.doubleValue()));
 *       } catch (Exception e) {
 *         fc.set_error("sin(" + args[0] + "):" + e);
 *       }
 *     }
 *     ...
 *   }
 *   SQLite.Database db = new SQLite.Database();
 *   db.open("db", 0);
 *   db.create_function("sin", 1, SinFunc);
 *   ...
 *   db.exec("select sin(1.0) from test", null);
 * </PRE>
 */

public interface Function {

    /**
     * Callback for regular function.
     *
     * @param fc function's context for reporting result
     * @param args String array of arguments
     */

    public void function(FunctionContext fc, String args[]);

    /**
     * Callback for one step in aggregate function.
     *
     * @param fc function's context for reporting result
     * @param args String array of arguments
     */

    public void step(FunctionContext fc, String args[]);

    /**
     * Callback for final step in aggregate function.
     *
     * @param fc function's context for reporting result
     */

    public void last_step(FunctionContext fc);

}
