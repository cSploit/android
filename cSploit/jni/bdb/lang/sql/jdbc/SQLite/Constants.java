/* DO NOT EDIT */

package SQLite;

/**
 * Container for SQLite constants.
 */

public final class Constants {
    /* Error code: 0 */
    public static final int SQLITE_OK = 0;
    /* Error code: 1 */
    public static final int SQLITE_ERROR = 1;
    /* Error code: 2 */
    public static final int SQLITE_INTERNAL = 2;
    /* Error code: 3 */
    public static final int SQLITE_PERM = 3;
    /* Error code: 4 */
    public static final int SQLITE_ABORT = 4;
    /* Error code: 5 */
    public static final int SQLITE_BUSY = 5;
    /* Error code: 6 */
    public static final int SQLITE_LOCKED = 6;
    /* Error code: 7 */
    public static final int SQLITE_NOMEM = 7;
    /* Error code: 8 */
    public static final int SQLITE_READONLY = 8;
    /* Error code: 9 */
    public static final int SQLITE_INTERRUPT = 9;
    /* Error code: 10 */
    public static final int SQLITE_IOERR = 10;
    /* Error code: 11 */
    public static final int SQLITE_CORRUPT = 11;
    /* Error code: 12 */
    public static final int SQLITE_NOTFOUND = 12;
    /* Error code: 13 */
    public static final int SQLITE_FULL = 13;
    /* Error code: 14 */
    public static final int SQLITE_CANTOPEN = 14;
    /* Error code: 15 */
    public static final int SQLITE_PROTOCOL = 15;
    /* Error code: 16 */
    public static final int SQLITE_EMPTY = 16;
    /* Error code: 17 */
    public static final int SQLITE_SCHEMA = 17;
    /* Error code: 18 */
    public static final int SQLITE_TOOBIG = 18;
    /* Error code: 19 */
    public static final int SQLITE_CONSTRAINT = 19;
    /* Error code: 20 */
    public static final int SQLITE_MISMATCH = 20;
    /* Error code: 21 */
    public static final int SQLITE_MISUSE = 21;
    /* Error code: 22 */
    public static final int SQLITE_NOLFS = 22;
    /* Error code: 23 */
    public static final int SQLITE_AUTH = 23;
    /* Error code: 24 */
    public static final int SQLITE_FORMAT = 24;
    /* Error code: 25 */
    public static final int SQLITE_RANGE = 25;
    /* Error code: 26 */
    public static final int SQLITE_NOTADB = 26;
    public static final int SQLITE_ROW = 100;
    public static final int SQLITE_DONE = 101;
    public static final int SQLITE_INTEGER = 1;
    public static final int SQLITE_FLOAT = 2;
    public static final int SQLITE_BLOB = 4;
    public static final int SQLITE_NULL = 5;
    public static final int SQLITE3_TEXT = 3;
    public static final int SQLITE_NUMERIC = -1;
    public static final int SQLITE_TEXT = 3;
    public static final int SQLITE2_TEXT = -2;
    public static final int SQLITE_ARGS = -3;
    public static final int SQLITE_COPY = 0;
    public static final int SQLITE_CREATE_INDEX = 1;
    public static final int SQLITE_CREATE_TABLE = 2;
    public static final int SQLITE_CREATE_TEMP_INDEX = 3;
    public static final int SQLITE_CREATE_TEMP_TABLE = 4;
    public static final int SQLITE_CREATE_TEMP_TRIGGER = 5;
    public static final int SQLITE_CREATE_TEMP_VIEW = 6;
    public static final int SQLITE_CREATE_TRIGGER = 7;
    public static final int SQLITE_CREATE_VIEW = 8;
    public static final int SQLITE_DELETE = 9;
    public static final int SQLITE_DROP_INDEX = 10;
    public static final int SQLITE_DROP_TABLE = 11;
    public static final int SQLITE_DROP_TEMP_INDEX = 12;
    public static final int SQLITE_DROP_TEMP_TABLE = 13;
    public static final int SQLITE_DROP_TEMP_TRIGGER = 14;
    public static final int SQLITE_DROP_TEMP_VIEW = 15;
    public static final int SQLITE_DROP_TRIGGER = 16;
    public static final int SQLITE_DROP_VIEW = 17;
    public static final int SQLITE_INSERT = 18;
    public static final int SQLITE_PRAGMA = 19;
    public static final int SQLITE_READ = 20;
    public static final int SQLITE_SELECT = 21;
    public static final int SQLITE_TRANSACTION = 22;
    public static final int SQLITE_UPDATE = 23;
    public static final int SQLITE_ATTACH = 24;
    public static final int SQLITE_DETACH = 25;
    public static final int SQLITE_DENY = 1;
    public static final int SQLITE_IGNORE = 2;
    public static final int SQLITE_OPEN_AUTOPROXY = 32;
    public static final int SQLITE_OPEN_CREATE = 4;
    public static final int SQLITE_OPEN_DELETEONCLOSE = 8;
    public static final int SQLITE_OPEN_EXCLUSIVE = 16;
    public static final int SQLITE_OPEN_FULLMUTEX = 65536;
    public static final int SQLITE_OPEN_MAIN_DB = 256;
    public static final int SQLITE_OPEN_MAIN_JOURNAL = 2048;
    public static final int SQLITE_OPEN_MASTER_JOURNAL = 16384;
    public static final int SQLITE_OPEN_NOMUTEX = 32768;
    public static final int SQLITE_OPEN_PRIVATECACHE = 262144;
    public static final int SQLITE_OPEN_READONLY = 1;
    public static final int SQLITE_OPEN_READWRITE = 2;
    public static final int SQLITE_OPEN_SHAREDCACHE = 131072;
    public static final int SQLITE_OPEN_SUBJOURNAL = 8192;
    public static final int SQLITE_OPEN_TEMP_DB = 512;
    public static final int SQLITE_OPEN_TEMP_JOURNAL = 4096;
    public static final int SQLITE_OPEN_TRANSIENT_DB = 1024;
    public static final int SQLITE_OPEN_URI = 64;
    public static final int SQLITE_OPEN_WAL = 524288;
    public static final int SQLITE_STATUS_MALLOC_COUNT = 9;
    public static final int SQLITE_STATUS_MALLOC_SIZE = 5;
    public static final int SQLITE_STATUS_MEMORY_USED = 0;
    public static final int SQLITE_STATUS_PAGECACHE_OVERFLOW = 2;
    public static final int SQLITE_STATUS_PAGECACHE_SIZE = 7;
    public static final int SQLITE_STATUS_PAGECACHE_USED = 1;
    public static final int SQLITE_STATUS_PARSER_STACK = 6;
    public static final int SQLITE_STATUS_SCRATCH_OVERFLOW = 4;
    public static final int SQLITE_STATUS_SCRATCH_SIZE = 8;
    public static final int SQLITE_STATUS_SCRATCH_USED = 3;
    public static final int SQLITE_DBSTATUS_CACHE_HIT = 7;
    public static final int SQLITE_DBSTATUS_CACHE_MISS = 8;
    public static final int SQLITE_DBSTATUS_CACHE_USED = 1;
    public static final int SQLITE_DBSTATUS_LOOKASIDE_HIT = 4;
    public static final int SQLITE_DBSTATUS_LOOKASIDE_MISS_SIZE = 5;
    public static final int SQLITE_DBSTATUS_LOOKASIDE_MISS_FULL = 6;
    public static final int SQLITE_DBSTATUS_LOOKASIDE_USED = 0;
    public static final int SQLITE_DBSTATUS_SCHEMA_USED = 2;
    public static final int SQLITE_DBSTATUS_STMT_USED = 3;
    public static final int SQLITE_STMTSTATUS_FULLSCAN_STEP = 1;
    public static final int SQLITE_STMTSTATUS_SORT = 2;
    public static final int SQLITE_STMTSTATUS_AUTOINDEX = 3;
    public static final int drv_minor = 99999999;
}
