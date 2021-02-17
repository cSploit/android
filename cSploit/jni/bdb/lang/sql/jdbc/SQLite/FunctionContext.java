package SQLite;

/**
 * Context for execution of SQLite's user defined functions.
 * A reference to an instance of this class is passed to
 * user defined functions.
 */

public class FunctionContext {

    /**
     * Internal handle for the native SQLite API.
     */

    private long handle = 0;

    /**
     * Set function result from string.
     *
     * @param r result string
     */

    public native void set_result(String r);

    /**
     * Set function result from integer.
     *
     * @param r result integer
     */

    public native void set_result(int r);

    /**
     * Set function result from double.
     *
     * @param r result double
     */

    public native void set_result(double r);

    /**
     * Set function result from error message.
     *
     * @param r result string (error message)
     */

    public native void set_error(String r);

    /**
     * Set function result from byte array.
     * Only provided by SQLite3 databases.
     *
     * @param r result byte array
     */

    public native void set_result(byte[] r);

    /**
     * Set function result as empty blob given size.
     * Only provided by SQLite3 databases.
     *
     * @param n size for empty blob
     */

    public native void set_result_zeroblob(int n);

    /**
     * Retrieve number of rows for aggregate function.
     */

    public native int count();

    /**
     * Internal native initializer.
     */

    private static native void internal_init();

    static {
	internal_init();
    }
}
