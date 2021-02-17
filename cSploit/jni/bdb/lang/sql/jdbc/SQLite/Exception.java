package SQLite;

/**
 * Class for SQLite related exceptions.
 */

public class Exception extends java.lang.Exception {

    /**
     * Construct a new SQLite exception.
     *
     * @param string error message
     */

    public Exception(String string) {
	super(string);
    }
}
