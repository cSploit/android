package SQLite;

/**
 * Class to represent compiled SQLite VM.
 */

public class Vm {

    /**
     * Internal handle for the compiled SQLite VM.
     */

    private long handle = 0;

    /**
     * Internal last error code for compile()/step() methods.
     */

    protected int error_code = 0;

    /**
     * Perform one step on compiled SQLite VM.
     * The result row is passed to the given callback interface.<BR><BR>
     *
     * Example:<BR>
     * <PRE>
     *   ...
     *   try {
     *     Vm vm = db.compile("select * from x; select * from y;");
     *     while (vm.step(cb)) {
     *       ...
     *     }
     *     while (vm.compile()) {
     *       while (vm.step(cb)) {
     *         ...
     *       }
     *     }
     *   } catch (SQLite.Exception e) {
     *   }
     * </PRE>
     *
     * @param cb the object implementing the callback methods.
     * @return true as long as more row data can be retrieved,
     * false, otherwise.
     */

    public native boolean step(Callback cb) throws SQLite.Exception;

    /**
     * Compile the next SQL statement for the SQLite VM instance.
     * @return true when SQL statement has been compiled, false
     * on end of statement sequence.
     */

    public native boolean compile() throws SQLite.Exception;

    /**
     * Abort the compiled SQLite VM.
     */

    public native void stop() throws SQLite.Exception;

    /**
     * Destructor for object.
     */

    protected native void finalize();

    /**
     * Internal native initializer.
     */

    private static native void internal_init();

    static {
	internal_init();
    }
}
