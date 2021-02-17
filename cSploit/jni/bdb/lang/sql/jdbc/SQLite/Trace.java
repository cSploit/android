package SQLite;

/**
 * Callback interface for SQLite's trace function.
 */

public interface Trace {

    /**
     * Callback to trace (ie log) one SQL statement.
     *
     * @param stmt SQL statement string
     */

    public void trace(String stmt);
}

