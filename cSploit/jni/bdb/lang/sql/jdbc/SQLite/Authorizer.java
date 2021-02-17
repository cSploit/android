package SQLite;

/**
 * Callback interface for SQLite's authorizer function.
 */

public interface Authorizer {

    /**
     * Callback to authorize access.
     *
     * @param what integer indicating type of access
     * @param arg1 first argument (table, view, index, or trigger name)
     * @param arg2 second argument (file, table, or column name)
     * @param arg3 third argument (database name)
     * @param arg4 third argument (trigger name)
     * @return Constants.SQLITE_OK for success, Constants.SQLITE_IGNORE
     * for don't allow access but don't raise an error, Constants.SQLITE_DENY
     * for abort SQL statement with error.
     */

    public int authorize(int what, String arg1, String arg2, String arg3,
			 String arg4);
}

