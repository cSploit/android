/**
 * @file impexp.c
 * SQLite extension module for importing/exporting
 * database information from/to SQL source text and
 * export to CSV text.
 *
 * 2007 January 27
 *
 * The author disclaims copyright to this source code.  In place of
 * a legal notice, here is a blessing:
 *
 *    May you do good and not evil.
 *    May you find forgiveness for yourself and forgive others.
 *    May you share freely, never taking more than you give.
 *
 ********************************************************************
 *
 * <pre>
 * Usage:
 *
 *  SQLite function:
 *       SELECT import_sql(filename);
 *
 *  C function:
 *       int impexp_import_sql(sqlite3 *db,
 *                             char *filename);
 *
 *       Reads SQL commands from filename and executes them
 *       against the current database. Returns the number
 *       of changes to the current database.
 *
 *
 *  SQLite function:
 *       SELECT export_sql(filename, [mode, tablename, ...]);
 *
 *  C function:
 *       int impexp_export_sql(sqlite3 *db, char *filename, int mode, ...);
 *
 *       Writes SQL to filename similar to SQLite's shell
 *       ".dump" meta command. Mode selects the output format:
 *       Mode 0 (default): dump schema and data using the
 *       optional table names following the mode argument.
 *       Mode 1: dump data only using the optional table
 *       names following the mode argument.
 *       Mode 2: dump schema and data using the optional
 *       table names following the mode argument; each
 *       table name is followed by a WHERE clause, i.e.
 *       "mode, table1, where1, table2, where2, ..."
 *       Mode 3: dump data only, same rules as in mode 2.
 *       Returns approximate number of lines written or
 *       -1 when an error occurred.
 *
 *       Bit 1 of mode:      when 1 dump data only
 *       Bits 8..9 of mode:  blob quoting mode
 *           0   default
 *         256   ORACLE
 *         512   SQL Server
 *         768   MySQL
 *
 *
 *  SQLite function:
 *       SELECT export_csv(filename, hdr, prefix1, tablename1, schema1, ...]);
 *
 *  C function:
 *       int impexp_export_csv(sqlite3 *db, char *filename, int hdr, ...);
 *                             [char *prefix1, char *tablename1,
 *                             char *schema1, ...]
 *
 *       Writes entire tables as CSV to provided filename. A header
 *       row is written when the hdr parameter is true. The
 *       rows are optionally introduced with a column made up of
 *       the prefix (non-empty string) for the respective table.
 *       If "schema" is NULL, "sqlite_master" is used, otherwise
 *       specify e.g. "sqlite_temp_master" for temporary tables or 
 *       "att.sqlite_master" for the attached database "att".
 *
 *          CREATE TABLE A(a,b);
 *          INSERT INTO A VALUES(1,2);
 *          INSERT INTO A VALUES(3,'foo');
 *          CREATE TABLE B(c);
 *          INSERT INTO B VALUES('hello');
 *          SELECT export_csv('out.csv', 0, 'aa', 'A', NULL, 'bb', 'B', NULL);
 *          -- CSV output
 *          "aa",1,2
 *          "aa",3,"foo"
 *          "bb","hello"
 *          SELECT export_csv('out.csv', 1, 'aa', 'A', NULL, 'bb', 'B', NULL);
 *          -- CSV output
 *          "aa","a","b"
 *          "aa",1,2
 *          "aa",3,"foo"
 *          "bb","c"
 *          "bb","hello"
 *
 *
 *  SQLite function:
 *       SELECT export_xml(filename, appendflag, indent,
 *                         [root, item, tablename, schema]+);
 *
 *  C function:
 *       int impexp_export_xml(sqlite3 *db, char *filename,
 *                             int append, int indent, char *root,
 *                             char *item, char *tablename, char *schema);
 *
 *       Writes a table as simple XML to provided filename. The
 *       rows are optionally enclosed with the "root" tag,
 *       the row data is enclosed in "item" tags. If "schema"
 *       is NULL, "sqlite_master" is used, otherwise specify
 *       e.g. "sqlite_temp_master" for temporary tables or 
 *       "att.sqlite_master" for the attached database "att".
 *          
 *          <item>
 *           <columnname TYPE="INTEGER|REAL|NULL|TEXT|BLOB">value</columnname>
 *           ...
 *          </item>
 *
 *       e.g.
 *
 *          CREATE TABLE A(a,b);
 *          INSERT INTO A VALUES(1,2.1);
 *          INSERT INTO A VALUES(3,'foo');
 *          INSERT INTO A VALUES('',NULL);
 *          INSERT INTO A VALUES(X'010203','<blob>');
 *          SELECT export_xml('out.xml', 0, 2, 'TBL_A', 'ROW', 'A');
 *          -- XML output
 *            <TBL_A>
 *              <ROW>
 *                &lt;a TYPE="INTEGER"&gt;1&lt;/a&gt;
 *                &lt;b TYPE="REAL"&gt;2.1&lt;/b&gt;
 *              </ROW>
 *              <ROW>
 *                &lt;a TYPE="INTEGER"&gt;3&lt;/a&gt;
 *                &lt;b TYPE="TEXT"&gt;foo&lt;/b&gt;
 *              </ROW>
 *              <ROW>
 *                &lt;a TYPE="TEXT"&gt;&lt;/a&gt;
 *                &lt;b TYPE="NULL"&gt;&lt;/b&gt;
 *              </ROW>
 *              <ROW>
 *                &lt;a TYPE="BLOB"&gt;&#x01;&#x02;&x03;&lt;/a&gt;
 *                &lt;b TYPE="TEXT"&gt;&amp;lt;blob&amp;gt;&lt;/b&gt;
 *              </ROW>
 *            </TBL_A>
 *
 *       Quoting of XML entities is performed only on the data,
 *       not on column names and root/item tags.
 *
 *
 *  SQLite function:
 *       SELECT export_json(filename, sql);
 *
 *  C function:
 *       int impexp_export_json(sqlite3 *db, char *sql,
 *                              impexp_putc pfunc, void *parg);
 *
 *       Executes arbitrary SQL statements and formats
 *       the result in JavaScript Object Notation (JSON).
 *       The layout of the result is:
 *
 *        object {results, sql}
 *         results[] object {columns, rows, changes, last_insert_rowid, error}
 *          columns[]
 *           object {name, decltype, type }     (sqlite3_column_*)
 *          rows[][]                            (sqlite3_column_*)
 *          changes                             (sqlite3_changes)
 *          last_insert_rowid                   (sqlite3_last_insert_rowid)
 *          error                               (sqlite3_errmsg)
 *         sql                                  (SQL text)
 *
 *       For each single SQL statement in "sql" an object in the
 *       "results" array is produced.
 *
 *       The function pointer for the output function to
 *       "impexp_export_json" has a signature compatible
 *       with fputc(3).
 *
 *
 * On Win32 the filename argument may be specified as NULL in order
 * to open a system file dialog for interactive filename selection.
 * </pre>
 */

#ifdef STANDALONE
#include <sqlite3.h>
#define sqlite3_api_routines void
#else
#include <sqlite3ext.h>
static SQLITE_EXTENSION_INIT1
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#else
#include <unistd.h>
#endif

#include "impexp.h"

/**
 * @typedef struct json_pfs
 * @struct json_pfs
 * JSON output helper structure
 */

typedef struct {
    impexp_putc pfunc;	/**< function like fputc() */
    void *parg;		/**< argument to function */
} json_pfs;

static const char space_chars[] = " \f\n\r\t\v";

#define ISSPACE(c) ((c) && (strchr(space_chars, (c)) != 0))

/**
 * Read one line of input into dynamically allocated buffer
 * which the caller must free with sqlite3_free()
 * @param fin FILE pointer
 * @result dynamically allocated input line
 */

static char *
one_input_line(FILE *fin)
{
    char *line, *tmp;
    int nline;
    int n;
    int eol;

    nline = 256;
    line = sqlite3_malloc(nline);
    if (!line) {
	return 0;
    }
    n = 0;
    eol = 0;
    while (!eol) {
	if (n + 256 > nline) {
	    nline = nline * 2 + 256;
	    tmp = sqlite3_realloc(line, nline);
	    if (!tmp) {
		sqlite3_free(line);
		return 0;
	    }
	    line = tmp;
	}
	if (!fgets(line + n, nline - n, fin)) {
	    if (n == 0) {
		sqlite3_free(line);
		return 0;
	    }
	    line[n] = 0;
	    eol = 1;
	    break;
	}
	while (line[n]) {
	    n++;
	}
	if ((n > 0) && (line[n-1] == '\n')) {
	    n--;
	    line[n] = 0;
	    eol = 1;
	}
    }
    tmp = sqlite3_realloc(line, n + 1);
    if (!tmp) {
	sqlite3_free(line);
    }
    return tmp;
}

/**
 * Test if string ends with a semicolon
 * @param str string to be tested
 * @param n length of string
 * @result true or false
 */

static int
ends_with_semicolon(const char *str, int n)
{
    while ((n > 0) && ISSPACE(str[n - 1])) {
	n--;
    }
    return (n > 0) && (str[n - 1] == ';');
}

/**
 * Test if string contains entirely whitespace or SQL comment
 * @param str string to be tested
 * @result true or false
 */

static int
all_whitespace(const char *str)
{
    for (; str[0]; str++) {
	if (ISSPACE(str[0])) {
	    continue;
	}
	if ((str[0] == '/') && (str[1] == '*')) {
	    str += 2;
	    while (str[0] && ((str[0] != '*') || (str[1] != '/'))) {
		str++;
	    }
	    if (!str[0]) {
		return 0;
	    }
	    str++;
	    continue;
	}
	if ((str[0] == '-') && (str[1] == '-')) {
	    str += 2;
	    while (str[0] && (str[0] != '\n')) {
		str++;
	    }
	    if (!str[0]) {
		return 1;
	    }
	    continue;
	}
	return 0;
    }
    return 1;
}

/**
 * Process contents of FILE pointer as SQL commands
 * @param db SQLite database to work on
 * @param fin input FILE pointer
 * @result number of errors
 */

static int
process_input(sqlite3 *db, FILE *fin)
{
    char *line = 0;
    char *sql = 0;
    int nsql = 0;
    int rc;
    int errors = 0;

    while (1) {
	line = one_input_line(fin);
	if (!line) {
	    break;
	}
	if ((!sql || !sql[0]) && all_whitespace(line)) {
	    continue;
	}
	if (!sql) {
	    int i;
	    for (i = 0; line[i] && ISSPACE(line[i]); i++) {
		/* empty loop body */
	    }
	    if (line[i]) {
		nsql = strlen(line);
		sql = sqlite3_malloc(nsql + 1);
		if (!sql) {
		    errors++;
		    break;
		}
		strcpy(sql, line);
	    }
	} else {
	    int len = strlen(line);
	    char *tmp;

	    tmp = sqlite3_realloc(sql, nsql + len + 2);
	    if (!tmp) {
		errors++;
		break;
	    }
	    sql = tmp;
	    strcpy(sql + nsql, "\n");
	    nsql++;
	    strcpy(sql + nsql, line);
	    nsql += len;
	}
	sqlite3_free(line);
	line = 0;
	if (sql && ends_with_semicolon(sql, nsql) && sqlite3_complete(sql)) {
	    rc = sqlite3_exec(db, sql, 0, 0, 0);
	    if (rc != SQLITE_OK) {
		errors++;
	    }
	    sqlite3_free(sql);
	    sql = 0;
	    nsql = 0;
	}
    }
    if (sql) {
	sqlite3_free(sql);
    }
    if (line) {
	sqlite3_free(line);
    }
    return errors;
}

/**
 * SQLite function to quote SQLite value depending on optional quote mode
 * @param context SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 *
 * Layout of arguments:
 *
 * argv[0] - value to be quoted<br>
 * argv[1] - value of quote mode (optional)<br>
 */

static void
quote_func(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    int mode = 0;

    if (argc < 1) {
	return;
    }
    if (argc > 1) {
	mode = sqlite3_value_int(argv[1]);
    }
    switch (sqlite3_value_type(argv[0])) {
    case SQLITE_NULL: {
	sqlite3_result_text(context, "NULL", 4, SQLITE_STATIC);
	break;
    }
    case SQLITE_INTEGER:
    case SQLITE_FLOAT: {
	sqlite3_result_value(context, argv[0]);
	break;
    }
    case SQLITE_BLOB: {
	char *text = 0;
	unsigned char *blob = (unsigned char *) sqlite3_value_blob(argv[0]);
	int nblob = sqlite3_value_bytes(argv[0]);

	if (2 * nblob + 4 > 1000000000) {
	    sqlite3_result_error(context, "value too large", -1);
	    return;
	}
	text = (char *) sqlite3_malloc((2 * nblob) + 4);
	if (!text) {
	    sqlite3_result_error(context, "out of memory", -1);
	} else {
	    int i, k = 0;
	    static const char xdigits[] = "0123456789ABCDEF";

	    if (mode == 1) {
		/* ORACLE enclosed in '' */
		text[k++] = '\'';
	    } else if (mode == 2) {
		/* SQL Server 0x prefix */
		text[k++] = '0';
		text[k++] = 'x';
	    } else if (mode == 3) {
		/* MySQL x'..' */
		text[k++] = 'x';
		text[k++] = '\'';
	    } else {
		/* default */
		text[k++] = 'X';
		text[k++] = '\'';
	    }
	    for (i = 0; i < nblob; i++) {
		text[k++] = xdigits[(blob[i] >> 4 ) & 0x0F];
		text[k++] = xdigits[blob[i] & 0x0F];
	    }
	    if (mode == 1) {
		/* ORACLE enclosed in '' */
		text[k++] = '\'';
	    } else if (mode == 2) {
		/* SQL Server 0x prefix */
	    } else if (mode == 3) {
		/* MySQL x'..' */
		text[k++] = '\'';
	    } else {
		/* default */
		text[k++] = '\'';
	    }
	    text[k] = '\0';
	    sqlite3_result_text(context, text, k, SQLITE_TRANSIENT);
	    sqlite3_free(text);
	}
	break;
    }
    case SQLITE_TEXT: {
	int i, n;
	const unsigned char *arg = sqlite3_value_text(argv[0]);
	char *p;

	if (!arg) {
	    return;
	}
	for (i = 0, n = 0; arg[i]; i++) {
	    if (arg[i] == '\'') {
		n++;
	    }
	}
	if (i + n + 3 > 1000000000) {
	    sqlite3_result_error(context, "value too large", -1);
	    return;
	}
	p = sqlite3_malloc(i + n + 3);
	if (!p) {
	    sqlite3_result_error(context, "out of memory", -1);
	    return;
	}
	p[0] = '\'';
	for (i = 0, n = 1; arg[i]; i++) {
	    p[n++] = arg[i];
	    if (arg[i] == '\'') {
		p[n++] = '\'';
	    }
	}
	p[n++] = '\'';
	p[n] = 0;
	sqlite3_result_text(context, p, n, SQLITE_TRANSIENT);
	sqlite3_free(p);
	break;
    }
    }
}

/**
 * SQLite function to quote an SQLite value in CSV format
 * @param context SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void
quote_csv_func(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    if (argc < 1) {
	return;
    }
    switch (sqlite3_value_type(argv[0])) {
    case SQLITE_NULL: {
	sqlite3_result_text(context, "", 0, SQLITE_STATIC);
	break;
    }
    case SQLITE_INTEGER:
    case SQLITE_FLOAT: {
	sqlite3_result_value(context, argv[0]);
	break;
    }
    case SQLITE_BLOB: {
	char *text = 0;
	unsigned char *blob = (unsigned char *) sqlite3_value_blob(argv[0]);
	int nblob = sqlite3_value_bytes(argv[0]);

	if (2 * nblob + 4 > 1000000000) {
	    sqlite3_result_error(context, "value too large", -1);
	    return;
	}
	text = (char *) sqlite3_malloc((2 * nblob) + 4);
	if (!text) {
	    sqlite3_result_error(context, "out of memory", -1);
	} else {
	    int i, k = 0;
	    static const char xdigits[] = "0123456789ABCDEF";

	    text[k++] = '"';
	    for (i = 0; i < nblob; i++) {
		text[k++] = xdigits[(blob[i] >> 4 ) & 0x0F];
		text[k++] = xdigits[blob[i] & 0x0F];
	    }
	    text[k++] = '"';
	    text[k] = '\0';
	    sqlite3_result_text(context, text, k, SQLITE_TRANSIENT);
	    sqlite3_free(text);
	}
	break;
    }
    case SQLITE_TEXT: {
	int i, n;
	const unsigned char *arg = sqlite3_value_text(argv[0]);
	char *p;

	if (!arg) {
	    return;
	}
	for (i = 0, n = 0; arg[i]; i++) {
	    if (arg[i] == '"') {
		n++;
	    }
	}
	if (i + n + 3 > 1000000000) {
	    sqlite3_result_error(context, "value too large", -1);
	    return;
	}
	p = sqlite3_malloc(i + n + 3);
	if (!p) {
	    sqlite3_result_error(context, "out of memory", -1);
	    return;
	}
	p[0] = '"';
	for (i = 0, n = 1; arg[i]; i++) {
	    p[n++] = arg[i];
	    if (arg[i] == '"') {
		p[n++] = '"';
	    }
	}
	p[n++] = '"';
	p[n] = 0;
	sqlite3_result_text(context, p, n, SQLITE_TRANSIENT);
	sqlite3_free(p);
	break;
    }
    }
}

/**
 * SQLite function to make XML indentation
 * @param context SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void
indent_xml_func(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    static const char spaces[] = "                                ";
    int n = 0;

    if (argc > 0) {
	n = sqlite3_value_int(argv[0]);
	if (n > 32) {
	    n = 32;
	} else if (n < 0) {
	    n = 0;
	}
    }
    sqlite3_result_text(context, spaces, n, SQLITE_STATIC);
}

/**
 * SQLite function to quote a string for XML
 * @param context SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void
quote_xml_func(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    static const char xdigits[] = "0123456789ABCDEF";
    int type, addtype = 0;

    if (argc < 1) {
	return;
    }
    if (argc > 1) {
	addtype = sqlite3_value_int(argv[1]);
    }
    type = sqlite3_value_type(argv[0]);
    switch (type) {
    case SQLITE_NULL: {
	if (addtype > 0) {
	    sqlite3_result_text(context, " TYPE=\"NULL\">", -1, SQLITE_STATIC);
	} else {
	    sqlite3_result_text(context, "", 0, SQLITE_STATIC);
	}
	break;
    }
    case SQLITE_INTEGER:
    case SQLITE_FLOAT: {
	if (addtype > 0) {
	    char *text = (char *) sqlite3_malloc(128);
	    int k;

	    if (!text) {
		sqlite3_result_error(context, "out of memory", -1);
		return;
	    }
	    strcpy(text, (type == SQLITE_FLOAT) ? " TYPE=\"REAL\">" :
		   " TYPE=\"INTEGER\">");
	    k = strlen(text);
	    strcpy(text + k, (char *) sqlite3_value_text(argv[0]));
	    k = strlen(text);
	    sqlite3_result_text(context, text, k, SQLITE_TRANSIENT);
	    sqlite3_free(text);
	} else {
	    sqlite3_result_value(context, argv[0]);
	}
	break;
    }
    case SQLITE_BLOB: {
	char *text = 0;
	unsigned char *blob = (unsigned char *) sqlite3_value_blob(argv[0]);
	int nblob = sqlite3_value_bytes(argv[0]);
	int i, k = 0;

	if (6 * nblob + 34 > 1000000000) {
	    sqlite3_result_error(context, "value too large", -1);
	    return;
	}
	text = (char *) sqlite3_malloc((6 * nblob) + 34);
	if (!text) {
	    sqlite3_result_error(context, "out of memory", -1);
	    return;
	}
	if (addtype > 0) {
	    strcpy(text, " TYPE=\"BLOB\">");
	    k = strlen(text);
	}
	for (i = 0; i < nblob; i++) {
	    text[k++] = '&';
	    text[k++] = '#';
	    text[k++] = 'x';
	    text[k++] = xdigits[(blob[i] >> 4 ) & 0x0F];
	    text[k++] = xdigits[blob[i] & 0x0F];
	    text[k++] = ';';
	}
	text[k] = '\0';
	sqlite3_result_text(context, text, k, SQLITE_TRANSIENT);
	sqlite3_free(text);
	break;
    }
    case SQLITE_TEXT: {
	int i, n;
	const unsigned char *arg = sqlite3_value_text(argv[0]);
	char *p;

	if (!arg) {
	    return;
	}
	for (i = 0, n = 0; arg[i]; i++) {
	    if ((arg[i] == '"') || (arg[i] == '\'') ||
		(arg[i] == '<') || (arg[i] == '>') ||
		(arg[i] == '&') || (arg[i] < ' ')) {
		n += 5;
	    }
	}
	if (i + n + 32 > 1000000000) {
	    sqlite3_result_error(context, "value too large", -1);
	    return;
	}
	p = sqlite3_malloc(i + n + 32);
	if (!p) {
	    sqlite3_result_error(context, "out of memory", -1);
	    return;
	}
	n = 0;
	if (addtype > 0) {
	    strcpy(p, " TYPE=\"TEXT\">");
	    n = strlen(p);
	}	    
	for (i = 0; arg[i]; i++) {
	    if (arg[i] == '"') {
		p[n++] = '&';
		p[n++] = 'q';
		p[n++] = 'u';
		p[n++] = 'o';
		p[n++] = 't';
		p[n++] = ';';
	    } else if (arg[i] == '\'') {
		p[n++] = '&';
		p[n++] = 'a';
		p[n++] = 'p';
		p[n++] = 'o';
		p[n++] = 's';
		p[n++] = ';';
	    } else if (arg[i] == '<') {
		p[n++] = '&';
		p[n++] = 'l';
		p[n++] = 't';
		p[n++] = ';';
	    } else if (arg[i] == '>') {
		p[n++] = '&';
		p[n++] = 'g';
		p[n++] = 't';
		p[n++] = ';';
	    } else if (arg[i] == '&') {
		p[n++] = '&';
		p[n++] = 'a';
		p[n++] = 'm';
		p[n++] = 'p';
		p[n++] = ';';
	    } else if (arg[i] < ' ') {
		p[n++] = '&';
		p[n++] = '#';
		p[n++] = 'x';
		p[n++] = xdigits[(arg[i] >> 4 ) & 0x0F];
		p[n++] = xdigits[arg[i] & 0x0F];
		p[n++] = ';';
	    } else if (addtype < 0 && (arg[i] == ' ')) {
		p[n++] = '&';
		p[n++] = '#';
		p[n++] = 'x';
		p[n++] = xdigits[(arg[i] >> 4 ) & 0x0F];
		p[n++] = xdigits[arg[i] & 0x0F];
		p[n++] = ';';
	    } else {
		p[n++] = arg[i];
	    }
	}
	p[n] = '\0';
	sqlite3_result_text(context, p, n, SQLITE_TRANSIENT);
	sqlite3_free(p);
	break;
    }
    }
}

/**
 * SQLite function to read and process SQL commands from a file
 * @param ctx SQLite function context
 * @param nargs number of arguments
 * @param args argument vector
 */

static void
import_func(sqlite3_context *ctx, int nargs, sqlite3_value **args)
{
    sqlite3 *db = (sqlite3 *) sqlite3_user_data(ctx);
    int changes0 = sqlite3_changes(db);
    char *filename = 0;
    FILE *fin;
#ifdef _WIN32
    char fnbuf[MAX_PATH];
#endif

    if (nargs > 0) {
	if (sqlite3_value_type(args[0]) != SQLITE_NULL) {
	    filename = (char *) sqlite3_value_text(args[0]);
	}
    }
#ifdef _WIN32
    if (!filename) {
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof (ofn));
	memset(fnbuf, 0, sizeof (fnbuf));
	ofn.lStructSize = sizeof (ofn);
	ofn.lpstrFile = fnbuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST |
		    OFN_EXPLORER | OFN_PATHMUSTEXIST;
	if (GetOpenFileName(&ofn)) {
	    filename = fnbuf;
	}
    }
#endif
    if (!filename) {
	goto done;
    }
    fin = fopen(filename, "r");
    if (!fin) {
	goto done;
    }
    process_input(db, fin);
    fclose(fin);
done:
    sqlite3_result_int(ctx, sqlite3_changes(db) - changes0);
}

/* see doc in impexp.h */

int
impexp_import_sql(sqlite3 *db, char *filename)
{
    int changes0;
    FILE *fin;
#ifdef _WIN32
    char fnbuf[MAX_PATH];
#endif

    if (!db) {
	return 0;
    }
    changes0 = sqlite3_changes(db);
#ifdef _WIN32
    if (!filename) {
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof (ofn));
	memset(fnbuf, 0, sizeof (fnbuf));
	ofn.lStructSize = sizeof (ofn);
	ofn.lpstrFile = fnbuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST |
		    OFN_EXPLORER | OFN_PATHMUSTEXIST;
	if (GetOpenFileName(&ofn)) {
	    filename = fnbuf;
	}
    }
#endif
    if (!filename) {
	goto done;
    }
    fin = fopen(filename, "r");
    if (!fin) {
	goto done;
    }
    process_input(db, fin);
    fclose(fin);
done:
    return sqlite3_changes(db) - changes0;
}

/**
 * @typedef DUMP_DATA
 * @struct DUMP_DATA
 * Structure for dump callback
 */

typedef struct {
    sqlite3 *db;	/**< SQLite database pointer */
    int with_schema;	/**< if true, output schema */
    int quote_mode;	/**< mode for quoting data */
    char *where;	/**< optional where clause of dump */
    int nlines;		/**< counter for output lines */
    int indent;		/**< current indent level */
    FILE *out;		/**< output file pointer */
} DUMP_DATA;

/**
 * Write indentation to dump
 * @param dd information structure for dump
 */

static void
indent(DUMP_DATA *dd)
{
    int i;

    for (i = 0; i < dd->indent; i++) {
	fputc(' ', dd->out);
    }
}

/**
 * Execute SQL to dump contents of one table
 * @param dd information structure for dump
 * @param errp pointer receiving error message
 * @param fmt if true, use sqlite3_*printf() on SQL
 * @param query SQL text to perform dump of table
 * @param ... optional arguments
 * @result SQLite error code
 */

static int
table_dump(DUMP_DATA *dd, char **errp, int fmt, const char *query, ...)
{
    sqlite3_stmt *select = 0;
    int rc;
    const char *rest, *q = query;
    va_list ap;

    if (errp && *errp) {
	sqlite3_free(*errp);
	*errp = 0;
    }
    if (fmt) {
	va_start(ap, query);
	q = sqlite3_vmprintf(query, ap);
	va_end(ap);
	if (!q) {
	    return SQLITE_NOMEM;
	}
    }
#if defined(HAVE_SQLITE3PREPAREV2) && HAVE_SQLITE3PREPAREV2
    rc = sqlite3_prepare_v2(dd->db, q, -1, &select, &rest);
#else
    rc = sqlite3_prepare(dd->db, q, -1, &select, &rest);
#endif
    if (fmt) {
	sqlite3_free((char *) q);
    }
    if ((rc != SQLITE_OK) || !select) {
	return rc;
    }
    rc = sqlite3_step(select);
    while (rc == SQLITE_ROW) {
	if (fputs((char *) sqlite3_column_text(select, 0), dd->out) > 0) {
	    dd->nlines++;
	}
	if (dd->quote_mode >= 0) {
	    fputc(';', dd->out);
	}
	if (dd->quote_mode == -1) {
	    fputc('\r', dd->out);
	}
	if (dd->quote_mode >= -1) {
	    fputc('\n', dd->out);
	}
	rc = sqlite3_step(select);
    }
    rc = sqlite3_finalize(select);
    if (rc != SQLITE_OK) {
	if (errp) {
	    *errp = sqlite3_mprintf("%s", sqlite3_errmsg(dd->db));
	}
    }
    return rc;
}

/**
 * Free dynamically allocated string buffer
 * @param in input string pointer
 */

static void
append_free(char **in)
{
    long *p = (long *) *in;

    if (p) {
	p -= 2;
	sqlite3_free(p);
	*in = 0;
    }
}

/**
 * Append a string to dynamically allocated string buffer
 * with optional quoting
 * @param in input string pointer
 * @param append string to append
 * @param quote quote character or NUL
 * @result new string to be free'd with append_free()
 */

static char *
append(char **in, char const *append, char quote)
{
    long *p = (long *) *in;
    long len, maxlen, actlen;
    int i;
    char *pp;
    int nappend = append ? strlen(append) : 0;

    if (p) {
	p -= 2;
	maxlen = p[0];
	actlen = p[1];
    } else {
	maxlen = actlen = 0;
    }
    len = nappend + actlen;
    if (quote) {
	len += 2;
	for (i = 0; i < nappend; i++) {
	    if (append[i] == quote) {
		len++;
	    }
	}
    } else if (!nappend) {
	return *in;
    }
    if (len >= maxlen - 1) {
	long *q;

	maxlen = (len + 0x03ff) & (~0x3ff);
	q = (long *) sqlite3_realloc(p, maxlen + 1 + 2 * sizeof (long));
	if (!q) {
	    return 0;
	}
	if (!p) {
	    q[1] = 0;
	}
	p = q;
	p[0] = maxlen;
	*in = (char *) (p + 2);
    }
    pp = *in + actlen;
    if (quote) {
	*pp++ = quote;
	for (i = 0; i < nappend; i++) {
	    *pp++ = append[i];
	    if (append[i] == quote) {
		*pp++ = quote;
	    }
	}
	*pp++ = quote;
	*pp = '\0';
    } else {
	if (nappend) {
	    memcpy(pp, append, nappend);
	    pp += nappend;
	    *pp = '\0';
	}
    }
    p[1] = pp - *in;
    return *in;
}

/**
 * Quote string for XML output during dump
 * @param dd information structure for dump
 * @param str string to be output
 */

static void
quote_xml_str(DUMP_DATA *dd, char *str)
{
    static const char xdigits[] = "0123456789ABCDEF";
    int i;

    if (!str) {
	return;
    }
    for (i = 0; str[i]; i++) {
	if (str[i] == '"') {
	    fputs("&quot;", dd->out);
	} else if (str[i] == '\'') {
	    fputs("&apos;", dd->out);
	} else if (str[i] == '<') {
	    fputs("&lt;", dd->out);
	} else if (str[i] == '>') {
	    fputs("&gt;", dd->out);
	} else if (str[i] == '&') {
	    fputs("&amp;", dd->out);
	} else if ((unsigned char) str[i] <= ' ') {
	    char buf[8];

	    buf[0] = '&';
	    buf[1] = '&';
	    buf[2] = '#';
	    buf[3] = 'x';
	    buf[4] = xdigits[(str[i] >> 4 ) & 0x0F];
	    buf[5] = xdigits[str[i] & 0x0F];
	    buf[6] = ';';
	    buf[7] = '\0';
	    fputs(buf, dd->out);
	} else {
	    fputc(str[i], dd->out);
	}
    }
}

/**
 * Callback for sqlite3_exec() to dump one data row
 * @param udata information structure for dump
 * @param nargs number of columns
 * @param args column data
 * @param cols column labels
 * @result 0 to continue, 1 to abort
 */

static int
dump_cb(void *udata, int nargs, char **args, char **cols)
{
    int rc;
    const char *table, *type, *sql;
    DUMP_DATA *dd = (DUMP_DATA *) udata;

    if ((nargs != 3) || (args == NULL)) {
	return 1;
    }
    table = args[0];
    type = args[1];
    sql = args[2];
    if (strcmp(table, "sqlite_sequence") == 0) {
	if (dd->with_schema) {
	    if (fputs("DELETE FROM sqlite_sequence;\n", dd->out) >= 0) {
		dd->nlines++;
	    }
	}
    } else if (strcmp(table, "sqlite_stat1") == 0) {
	if (dd->with_schema) {
	    if (fputs("ANALYZE sqlite_master;\n", dd->out) >= 0) {
		dd->nlines++;
	    }
	}
    } else if (strncmp(table, "sqlite_", 7) == 0) {
	return 0;
    } else if (strncmp(sql, "CREATE VIRTUAL TABLE", 20) == 0) {
	if (dd->with_schema) {
	    sqlite3_stmt *stmt = 0;
	    char *creat = 0, *table_info = 0;
   
	    append(&table_info, "PRAGMA table_info(", 0);
	    append(&table_info, table, '"');
	    append(&table_info, ")", 0);
#if defined(HAVE_SQLITE3PREPAREV2) && HAVE_SQLITE3PREPAREV2
	    rc = sqlite3_prepare_v2(dd->db, table_info, -1, &stmt, 0);
#else
	    rc = sqlite3_prepare(dd->db, table_info, -1, &stmt, 0);
#endif
	    append_free(&table_info);
	    if ((rc != SQLITE_OK) || !stmt) {
bailout0:
		if (stmt) {
		    sqlite3_finalize(stmt);
		}
		append_free(&creat);
		return 1;
	    }
	    append(&creat, table, '"');
	    append(&creat, "(", 0);
	    rc = sqlite3_step(stmt);
	    while (rc == SQLITE_ROW) {
		const char *p;

		p = (const char *) sqlite3_column_text(stmt, 1);
		append(&creat, p, '"');
		append(&creat, " ", 0);
		p = (const char *) sqlite3_column_text(stmt, 2);
		if (p && p[0]) {
		    append(&creat, p, 0);
		}
		if (sqlite3_column_int(stmt, 5)) {
		    append(&creat, " PRIMARY KEY", 0);
		}
		if (sqlite3_column_int(stmt, 3)) {
		    append(&creat, " NOT NULL", 0);
		}
		p = (const char *) sqlite3_column_text(stmt, 4);
		if (p && p[0]) {
		    append(&creat, " DEFAULT ", 0);
		    append(&creat, p, 0);
		}
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW) {
		    append(&creat, ",", 0);
		}
	    }
	    if (rc != SQLITE_DONE) {
		goto bailout0;
	    }
	    sqlite3_finalize(stmt);
	    append(&creat, ")", 0);
	    if (creat && fprintf(dd->out, "CREATE TABLE %s;\n", creat) > 0) {
		dd->nlines++;
	    }
	    append_free(&creat);
	}
    } else {
	if (dd->with_schema) {
	    if (fprintf(dd->out, "%s;\n", sql) > 0) {
		dd->nlines++;
	    }
	}
    }
    if ((strcmp(type, "table") == 0) ||
	((dd->quote_mode < 0) && (strcmp(type, "view") == 0))) {
	sqlite3_stmt *stmt = 0;
	char *select = 0, *hdr = 0, *table_info = 0;
	char buffer[256];
   
	append(&table_info, "PRAGMA table_info(", 0);
	append(&table_info, table, '"');
	append(&table_info, ")", 0);
#if defined(HAVE_SQLITE3PREPAREV2) && HAVE_SQLITE3PREPAREV2
	rc = sqlite3_prepare_v2(dd->db, table_info, -1, &stmt, 0);
#else
	rc = sqlite3_prepare(dd->db, table_info, -1, &stmt, 0);
#endif
	append_free(&table_info);
	if ((rc != SQLITE_OK) || !stmt) {
bailout1:
	    if (stmt) {
		sqlite3_finalize(stmt);
	    }
	    append_free(&hdr);
	    append_free(&select);
	    return 1;
	}
	if (dd->quote_mode < -1) {
	    if (dd->where) {
		append(&select, "SELECT ", 0);
		sprintf(buffer, "indent_xml(%d)", dd->indent);
		append(&select, buffer, 0);
		append(&select, " || '<' || quote_xml(", 0);
		append(&select, dd->where, '"');
		append(&select, ",-1) || '>\n' || ", 0);
	    } else {
		append(&select, "SELECT ", 0);
	    }
	} else if (dd->quote_mode < 0) {
	    if (dd->where) {
		append(&select, "SELECT quote_csv(", 0);
		append(&select, dd->where, '"');
		append(&select, ") || ',' || ", 0);
	    } else {
		append(&select, "SELECT ", 0);
	    }
	    if (dd->indent) {
		append(&hdr, select, 0);
	    }
	} else {
	    char *tmp = 0;

	    if (dd->with_schema) {
		append(&select, "SELECT 'INSERT INTO ' || ", 0);
	    } else {
		append(&select, "SELECT 'INSERT OR REPLACE INTO ' || ", 0);
	    }
	    append(&tmp, table, '"');
	    if (tmp) {
		append(&select, tmp, '\'');
		append_free(&tmp);
	    }
	}
	if ((dd->quote_mode >= 0) && !dd->with_schema) {
	    char *tmp = 0;

	    append(&select, " || ' (' || ", 0);
	    rc = sqlite3_step(stmt);
	    while (rc == SQLITE_ROW) {
		const char *text = (const char *) sqlite3_column_text(stmt, 1);

		append(&tmp, text, '"');
		if (tmp) {
		    append(&select, tmp, '\'');
		    append_free(&tmp);
		}
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW) {
		    append(&select, " || ',' || ", 0);
		}
	    }
	    if (rc != SQLITE_DONE) {
		goto bailout1;
	    }
	    sqlite3_reset(stmt);
	    append(&select, "|| ')'", 0);
	}
	if ((dd->quote_mode == -1) && dd->indent) {
	    rc = sqlite3_step(stmt);
	    while (rc == SQLITE_ROW) {
		const char *text = (const char *) sqlite3_column_text(stmt, 1);

		append(&hdr, "quote_csv(", 0);
		append(&hdr, text, '"');
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_ROW) {
		    append(&hdr, ") || ',' || ", 0);
		} else {
		    append(&hdr, ")", 0);
		}
	    }
	    if (rc != SQLITE_DONE) {
		goto bailout1;
	    }
	    sqlite3_reset(stmt);
	}
	if (dd->quote_mode >= 0) {
	    append(&select, " || ' VALUES(' || ", 0);
	}
	rc = sqlite3_step(stmt);
	while (rc == SQLITE_ROW) {
	    const char *text = (const char *) sqlite3_column_text(stmt, 1);
	    const char *type = (const char *) sqlite3_column_text(stmt, 2);
	    int tlen = strlen(type ? type : "");

	    if (dd->quote_mode < -1) {
		sprintf(buffer, "indent_xml(%d)", dd->indent + 1);
		append(&select, buffer, 0);
		append(&select, "|| '<' || quote_xml(", 0);
		append(&select, text, '\'');
		append(&select, ",-1) || quote_xml(", 0);
		append(&select, text, '"');
		append(&select, ",1) || '</' || quote_xml(", 0);
		append(&select, text, '\'');
		append(&select, ",-1) || '>\n'", 0);
	    } else if (dd->quote_mode < 0) {
		/* leave out BLOB columns */
		if (((tlen >= 4) && (strncasecmp(type, "BLOB", 4) == 0)) ||
		    ((tlen >= 6) && (strncasecmp(type, "BINARY", 6) == 0))) {
		    rc = sqlite3_step(stmt);
		    if (rc != SQLITE_ROW) {
			tlen = strlen(select);
			if (tlen > 10) {
			    select[tlen - 10] = '\0';
			}
		    }
		    continue;
		}
		append(&select, "quote_csv(", 0);
		append(&select, text, '"');
	    } else {
		append(&select, "quote_sql(", 0);
		append(&select, text, '"');
		if (dd->quote_mode) {
		    char mbuf[32];

		    sprintf(mbuf, ",%d", dd->quote_mode);
		    append(&select, mbuf, 0);
		}
	    }
	    rc = sqlite3_step(stmt);
	    if (rc == SQLITE_ROW) {
		if (dd->quote_mode >= -1) {
		    append(&select, ") || ',' || ", 0);
		} else {
		    append(&select, " || ", 0);
		}
	    } else {
		if (dd->quote_mode >= -1) {
		    append(&select, ") ", 0);
		} else {
		    append(&select, " ", 0);
		}
	    }
	}
	if (rc != SQLITE_DONE) {
	    goto bailout1;
	}
	sqlite3_finalize(stmt);
	stmt = 0;
	if (dd->quote_mode >= 0) {
	    append(&select, "|| ')' FROM ", 0);
	} else {
	    if ((dd->quote_mode < -1) && dd->where) {
		sprintf(buffer, " || indent_xml(%d)", dd->indent);
		append(&select, buffer, 0);
		append(&select, " || '</' || quote_xml(", 0);
		append(&select, dd->where, '"');
		append(&select, ",-1) || '>\n' FROM ", 0);
	    } else {
		append(&select, "FROM ", 0);
	    }
	}
	append(&select, table, '"');
	if ((dd->quote_mode >= 0) && dd->where) {
	    append(&select, " ", 0);
	    append(&select, dd->where, 0);
	}
	if (hdr) {
	    rc = table_dump(dd, 0, 0, hdr);
	    append_free(&hdr);
	    hdr = 0;
	}
	rc = table_dump(dd, 0, 0, select);
	if (rc == SQLITE_CORRUPT) {
	    append(&select, " ORDER BY rowid DESC", 0);
	    rc = table_dump(dd, 0, 0, select);
	}
	append_free(&select);
    }
    return 0;
}

/**
 * Execute SQL on sqlite_master table in order to dump data.
 * @param dd information structure for dump
 * @param errp pointer receiving error message
 * @param query SQL for sqlite3_*printf()
 * @param ... argument list
 * @result SQLite error code
 */

static int
schema_dump(DUMP_DATA *dd, char **errp, const char *query, ...)
{
    int rc;
    char *q;
    va_list ap;

    if (errp) {
	sqlite3_free(*errp);
	*errp = 0;
    }
    va_start(ap, query);
    q = sqlite3_vmprintf(query, ap);
    va_end(ap);
    if (!q) {
	return SQLITE_NOMEM;
    }
    rc = sqlite3_exec(dd->db, q, dump_cb, dd, errp);
    if (rc == SQLITE_CORRUPT) {
	char *tmp;

	tmp = sqlite3_mprintf("%s ORDER BY rowid DESC", q);
	sqlite3_free(q);
	if (!tmp) {
	    return rc;
	}
	q = tmp;
	if (errp) {
	    sqlite3_free(*errp);
	    *errp = 0;
	}
	rc = sqlite3_exec(dd->db, q, dump_cb, dd, errp);
    }
    sqlite3_free(q);
    return rc;
}

/**
 * SQLite function for SQL output, see impexp_export_sql
 * @param ctx SQLite function context
 * @param nargs number of arguments
 * @param args argument vector
 */

static void
export_func(sqlite3_context *ctx, int nargs, sqlite3_value **args)
{
    DUMP_DATA dd0, *dd = &dd0;
    sqlite3 *db = (sqlite3 *) sqlite3_user_data(ctx);
    int i, mode = 0;
    char *filename = 0;
#ifdef _WIN32
    char fnbuf[MAX_PATH];
#endif

    dd->db = db;
    dd->where = 0;
    dd->nlines = -1;
    dd->indent = 0;
    if (nargs > 0) {
	if (sqlite3_value_type(args[0]) != SQLITE_NULL) {
	    filename = (char *) sqlite3_value_text(args[0]);
	}
    }
#ifdef _WIN32
    if (!filename) {
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof (ofn));
	memset(fnbuf, 0, sizeof (fnbuf));
	ofn.lStructSize = sizeof (ofn);
	ofn.lpstrFile = fnbuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER |
		    OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	if (GetSaveFileName(&ofn)) {
	    filename = fnbuf;
	}
    }
#endif
    if (!filename) {
	goto done;
    }
    dd->out = fopen(filename, "w");
    if (!dd->out) {
	goto done;
    }
    if (nargs > 1) {
	mode = sqlite3_value_int(args[1]);
    }
    dd->with_schema = !(mode & 1);
    dd->quote_mode = (mode >> 8) & 3;
    dd->nlines = 0;
    if (fputs("BEGIN TRANSACTION;\n", dd->out) >= 0) {
	dd->nlines++;
    }
    if (nargs <= 2) {
	schema_dump(dd, 0,
		    "SELECT name, type, sql FROM sqlite_master"
		    " WHERE sql NOT NULL AND type = 'table'");
	if (dd->with_schema) {
	    table_dump(dd, 0, 0,
		       "SELECT sql FROM sqlite_master WHERE"
		       " sql NOT NULL AND type IN ('index','trigger','view')");
	}
    } else {
	for (i = 2; i < nargs; i += (mode & 2) ? 2 : 1) {
	    dd->where = 0;
	    if ((mode & 2) && (i + 1 < nargs)) {
		dd->where = (char *) sqlite3_value_text(args[i + 1]);
	    }
	    schema_dump(dd, 0,
			"SELECT name, type, sql FROM sqlite_master"
			" WHERE tbl_name LIKE %Q AND type = 'table'"
			" AND sql NOT NULL",
			sqlite3_value_text(args[i]));
	    if (dd->with_schema) {
		table_dump(dd, 0, 1,
			   "SELECT sql FROM sqlite_master"
			   " WHERE sql NOT NULL"
			   " AND type IN ('index','trigger','view')"
			   " AND tbl_name LIKE %Q",
			   sqlite3_value_text(args[i]));
	    }
	}
    }
    if (fputs("COMMIT;\n", dd->out) >= 0) {
	dd->nlines++;
    }
    fclose(dd->out);
done:
    sqlite3_result_int(ctx, dd->nlines);
}

/**
 * SQLite function for CSV output, see impexp_export_csv
 * @param ctx SQLite function context
 * @param nargs number of arguments
 * @param args argument vector
 */

static void
export_csv_func(sqlite3_context *ctx, int nargs, sqlite3_value **args)
{
    DUMP_DATA dd0, *dd = &dd0;
    sqlite3 *db = (sqlite3 *) sqlite3_user_data(ctx);
    int i;
    char *filename = 0;
#ifdef _WIN32
    char fnbuf[MAX_PATH];
#endif

    dd->db = db;
    dd->where = 0;
    dd->nlines = -1;
    dd->indent = 0;
    dd->with_schema = 0;
    dd->quote_mode = -1;
    if (nargs > 0) {
	if (sqlite3_value_type(args[0]) != SQLITE_NULL) {
	    filename = (char *) sqlite3_value_text(args[0]);
	}
    }
#ifdef _WIN32
    if (!filename) {
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof (ofn));
	memset(fnbuf, 0, sizeof (fnbuf));
	ofn.lStructSize = sizeof (ofn);
	ofn.lpstrFile = fnbuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER |
		    OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	if (GetSaveFileName(&ofn)) {
	    filename = fnbuf;
	}
    }
#endif
    if (!filename) {
	goto done;
    }
#ifdef _WIN32
    dd->out = fopen(filename, "wb");
#else
    dd->out = fopen(filename, "w");
#endif
    if (!dd->out) {
	goto done;
    }
    dd->nlines = 0;
    if (nargs > 1) {
	if (sqlite3_value_type(args[1]) != SQLITE_NULL) {
	    if (sqlite3_value_int(args[1])) {
		dd->indent = 1;
	    }
	}
    }
    for (i = 2; i <= nargs - 3; i += 3) {
	char *schema = 0, *sql;

	dd->where = 0;
	if (sqlite3_value_type(args[i]) != SQLITE_NULL) {
	    dd->where = (char *) sqlite3_value_text(args[i]);
	    if (dd->where && !dd->where[0]) {
		dd->where = 0;
	    }
	}
	if (sqlite3_value_type(args[i + 2]) != SQLITE_NULL) {
	    schema = (char *) sqlite3_value_text(args[i + 2]);
	}
	if (!schema || (schema[0] == '\0')) {
	    schema = "sqlite_master";
	}
	sql = sqlite3_mprintf("SELECT name, type, sql FROM %s"
			      " WHERE tbl_name LIKE %%Q AND "
			      " (type = 'table' OR type = 'view')"
			      " AND sql NOT NULL", schema);
	if (sql) {
	    schema_dump(dd, 0, sql, sqlite3_value_text(args[i + 1]));
	    sqlite3_free(sql);
	}
    }
    fclose(dd->out);
done:
    sqlite3_result_int(ctx, dd->nlines);
}

/**
 * SQLite function for XML output, see impexp_export_xml
 * @param ctx SQLite function context
 * @param nargs number of arguments
 * @param args argument vector
 */

static void
export_xml_func(sqlite3_context *ctx, int nargs, sqlite3_value **args)
{
    DUMP_DATA dd0, *dd = &dd0;
    sqlite3 *db = (sqlite3 *) sqlite3_user_data(ctx);
    int i;
    char *filename = 0;
    char *openmode = "w";
#ifdef _WIN32
    char fnbuf[MAX_PATH];
#endif

    dd->db = db;
    dd->where = 0;
    dd->nlines = -1;
    dd->indent = 0;
    dd->with_schema = 0;
    dd->quote_mode = -2;
    if (nargs > 0) {
	if (sqlite3_value_type(args[0]) != SQLITE_NULL) {
	    filename = (char *) sqlite3_value_text(args[0]);
	}
    }
#ifdef _WIN32
    if (!filename) {
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof (ofn));
	memset(fnbuf, 0, sizeof (fnbuf));
	ofn.lStructSize = sizeof (ofn);
	ofn.lpstrFile = fnbuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER |
		    OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	if (GetSaveFileName(&ofn)) {
	    filename = fnbuf;
	}
    }
#endif
    if (!filename) {
	goto done;
    }
    if (nargs > 1) {
	if (sqlite3_value_type(args[1]) != SQLITE_NULL) {
	    if (sqlite3_value_int(args[1])) {
		openmode = "a";
	    }
	}
    }
    if (nargs > 2) {
	if (sqlite3_value_type(args[2]) != SQLITE_NULL) {
	    dd->indent = sqlite3_value_int(args[2]);
	    if (dd->indent < 0) {
		dd->indent = 0;
	    }
	}
    }
    dd->out = fopen(filename, openmode);
    if (!dd->out) {
	goto done;
    }
    dd->nlines = 0;
    for (i = 3; i <= nargs - 4; i += 4) {
	char *root = 0, *schema = 0, *sql;

	if (sqlite3_value_type(args[i]) != SQLITE_NULL) {
	    root = (char *) sqlite3_value_text(args[i]);
	    if (root && !root[0]) {
		root = 0;
	    }
	}
	dd->where = 0;
	if (sqlite3_value_type(args[i + 1]) != SQLITE_NULL) {
	    dd->where = (char *) sqlite3_value_text(args[i + 1]);
	    if (dd->where && !dd->where[0]) {
		dd->where = 0;
	    }
	}
	if (root) {
	    indent(dd);
	    dd->indent++;
	    fputs("<", dd->out);
	    quote_xml_str(dd, root);
	    fputs(">\n", dd->out);
	}
	if (sqlite3_value_type(args[i + 3]) != SQLITE_NULL) {
	    schema = (char *) sqlite3_value_text(args[i + 3]);
	}
	if (!schema || (schema[0] == '\0')) {
	    schema = "sqlite_master";
	}
	sql = sqlite3_mprintf("SELECT name, type, sql FROM %s"
			      " WHERE tbl_name LIKE %%Q AND"
			      " (type = 'table' OR type = 'view')"
			      " AND sql NOT NULL", schema);
	if (sql) {
	    schema_dump(dd, 0, sql, sqlite3_value_text(args[i + 2]));
	    sqlite3_free(sql);
	}
	if (root) {
	    dd->indent--;
	    indent(dd);
	    fputs("</", dd->out);
	    quote_xml_str(dd, root);
	    fputs(">\n", dd->out);
	}
    }
    fclose(dd->out);
done:
    sqlite3_result_int(ctx, dd->nlines);
}

/* see doc in impexp.h */

int
impexp_export_sql(sqlite3 *db, char *filename, int mode, ...)
{
    DUMP_DATA dd0, *dd = &dd0;
    va_list ap;
    char *table;
#ifdef _WIN32
    char fnbuf[MAX_PATH];
#endif

    if (!db) {
	return 0;
    }
    dd->db = db;
    dd->where = 0;
    dd->nlines = -1;
#ifdef _WIN32
    if (!filename) {
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof (ofn));
	memset(fnbuf, 0, sizeof (fnbuf));
	ofn.lStructSize = sizeof (ofn);
	ofn.lpstrFile = fnbuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER |
		    OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	if (GetSaveFileName(&ofn)) {
	    filename = fnbuf;
	}
    }
#endif
    if (!filename) {
	goto done;
    }
    dd->out = fopen(filename, "w");
    if (!dd->out) {
	goto done;
    }
    dd->with_schema = !(mode & 1);
    dd->nlines = 0;
    if (fputs("BEGIN TRANSACTION;\n", dd->out) >= 0) {
	dd->nlines++;
    }
    va_start(ap, mode);
    table = va_arg(ap, char *);
    if (!table) {
	schema_dump(dd, 0,
		    "SELECT name, type, sql FROM sqlite_master"
		    " WHERE sql NOT NULL AND type = 'table'");
	if (dd->with_schema) {
	    table_dump(dd, 0, 0,
		       "SELECT sql FROM sqlite_master WHERE"
		       " sql NOT NULL AND type IN ('index','trigger','view')");
	}
    } else {
	while (table) {
	    dd->where = 0;
	    if ((mode & 2)) {
		dd->where = va_arg(ap, char *);
	    }
	    schema_dump(dd, 0,
			"SELECT name, type, sql FROM sqlite_master"
			" WHERE tbl_name LIKE %Q AND type = 'table'"
			" AND sql NOT NULL", table);
	    if (dd->with_schema) {
		table_dump(dd, 0, 1,
			   "SELECT sql FROM sqlite_master"
			   " WHERE sql NOT NULL"
			   " AND type IN ('index','trigger','view')"
			   " AND tbl_name LIKE %Q", table);
	    }
	    table = va_arg(ap, char *);
	}
    }
    va_end(ap);
    if (fputs("COMMIT;\n", dd->out) >= 0) {
	dd->nlines++;
    }
    fclose(dd->out);
done:
    return dd->nlines;
}

/* see doc in impexp.h */

int
impexp_export_csv(sqlite3 *db, char *filename, int hdr, ...)
{
    DUMP_DATA dd0, *dd = &dd0;
    va_list ap;
    char *prefix, *table, *schema;
#ifdef _WIN32
    char fnbuf[MAX_PATH];
#endif

    if (!db) {
	return 0;
    }
    dd->db = db;
    dd->where = 0;
    dd->nlines = -1;
    dd->indent = 0;
    dd->with_schema = 0;
    dd->quote_mode = -1;
    dd->indent = hdr != 0;
#ifdef _WIN32
    if (!filename) {
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof (ofn));
	memset(fnbuf, 0, sizeof (fnbuf));
	ofn.lStructSize = sizeof (ofn);
	ofn.lpstrFile = fnbuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER |
		    OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	if (GetSaveFileName(&ofn)) {
	    filename = fnbuf;
	}
    }
#endif
    if (!filename) {
	goto done;
    }
#ifdef _WIN32
    dd->out = fopen(filename, "wb");
#else
    if ((hdr < 0) && access(filename, W_OK) == 0) {
	dd->out = fopen(filename, "a");
	dd->indent = 0;
    } else {
	dd->out = fopen(filename, "w");
    }
#endif
    if (!dd->out) {
	goto done;
    }
    dd->nlines = 0;
    va_start(ap, hdr);
    prefix = va_arg(ap, char *);
    table = va_arg(ap, char *);
    schema = va_arg(ap, char *);
    while (table != NULL) {
	char *sql;

	dd->where = (prefix && prefix[0]) ? prefix : 0;
	if (!schema || (schema[0] == '\0')) {
	    schema = "sqlite_master";
	}
	sql = sqlite3_mprintf("SELECT name, type, sql FROM %s"
			      " WHERE tbl_name LIKE %%Q AND"
			      " (type = 'table' OR type = 'view')"
			      " AND sql NOT NULL", schema);
	if (sql) {
	    schema_dump(dd, 0, sql, table);
	    sqlite3_free(sql);
	}
	prefix = va_arg(ap, char *);
	table = va_arg(ap, char *);
	schema = va_arg(ap, char *);
    }
    va_end(ap);
    fclose(dd->out);
done:
    return dd->nlines;
}

/* see doc in impexp.h */

int
impexp_export_xml(sqlite3 *db, char *filename, int append, int indnt,
		  char *root, char *item, char *tablename, char *schema)
{
    DUMP_DATA dd0, *dd = &dd0;
    char *sql;
#ifdef _WIN32
    char fnbuf[MAX_PATH];
#endif

    if (!db) {
	return 0;
    }
    dd->db = db;
    dd->where = item;
    dd->nlines = -1;
    dd->indent = (indnt > 0) ? indnt : 0;
    dd->with_schema = 0;
    dd->quote_mode = -2;
#ifdef _WIN32
    if (!filename) {
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof (ofn));
	memset(fnbuf, 0, sizeof (fnbuf));
	ofn.lStructSize = sizeof (ofn);
	ofn.lpstrFile = fnbuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER |
		    OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	if (GetSaveFileName(&ofn)) {
	    filename = fnbuf;
	}
    }
#endif
    if (!filename) {
	goto done;
    }
    dd->out = fopen(filename, append ? "a" : "w");
    if (!dd->out) {
	goto done;
    }
    dd->nlines = 0;
    if (root) {
	indent(dd);
	dd->indent++;
	fputs("<", dd->out);
	quote_xml_str(dd, root);
	fputs(">\n", dd->out);
    }
    if (!schema || (schema[0] == '\0')) {
	schema = "sqlite_master";
    }
    sql = sqlite3_mprintf("SELECT name, type, sql FROM %s"
			  " WHERE tbl_name LIKE %%Q AND"
			  " (type = 'table' OR type = 'view')"
			  " AND sql NOT NULL", schema);
    if (sql) {
	schema_dump(dd, 0, sql, tablename);
	sqlite3_free(sql);
    }
    if (root) {
	dd->indent--;
	indent(dd);
	fputs("</", dd->out);
	quote_xml_str(dd, root);
	fputs(">\n", dd->out);
    }
    fclose(dd->out);
done:
    return dd->nlines;
}

/**
 * Write string using JSON output function
 * @param string string to be written
 * @param pfs JSON output function
 */

static void
json_pstr(const char *string, json_pfs *pfs)
{
    while (*string) {
	pfs->pfunc(*string, pfs->parg);
	string++;
    }
}

/**
 * Quote and write string using JSON output function
 * @param string string to be written
 * @param pfs JSON output function
 */

static void
json_pstrq(const char *string, json_pfs *pfs)
{
    impexp_putc pfunc = pfs->pfunc;
    void *parg = pfs->parg;
    char buf[64];

    if (!string) {
	json_pstr("null", pfs);
	return;
    }
    pfunc('"', parg);
    while (*string) {
	switch (*string) {
	case '"':
	case '\\':
	    pfunc('\\', parg);
	    pfunc(*string, parg);
	    break;
	case '\b':
	    pfunc('\\', parg);
	    pfunc('b', parg);
	    break;
	case '\f':
	    pfunc('\\', parg);
	    pfunc('f', parg);
	    break;
	case '\n':
	    pfunc('\\', parg);
	    pfunc('n', parg);
	    break;
	case '\r':
	    pfunc('\\', parg);
	    pfunc('r', parg);
	    break;
	case '\t':
	    pfunc('\\', parg);
	    pfunc('t', parg);
	    break;
	default:
	    if (((*string < ' ') && (*string > 0)) || (*string == 0x7f)) {
		sprintf(buf, "\\u%04x", *string);
		json_pstr(buf, pfs);
	    } else if (*string < 0) {
		unsigned char c = string[0];
		unsigned long uc = 0;

		if (c < 0xc0) {
		    uc = c;
		} else if (c < 0xe0) {
		    if ((string[1] & 0xc0) == 0x80) {
			uc = ((c & 0x1f) << 6) | (string[1] & 0x3f);
			++string;
		    } else {
			uc = c;
		    }
		} else if (c < 0xf0) {
		    if (((string[1] & 0xc0) == 0x80) &&
			((string[2] & 0xc0) == 0x80)) {
			uc = ((c & 0x0f) << 12) |
			     ((string[1] & 0x3f) << 6) | (string[2] & 0x3f);
			string += 2;
		    } else {
			uc = c;
		    }
		} else if (c < 0xf8) {
		    if (((string[1] & 0xc0) == 0x80) &&
			((string[2] & 0xc0) == 0x80) &&
			((string[3] & 0xc0) == 0x80)) {
			uc = ((c & 0x03) << 18) |
			     ((string[1] & 0x3f) << 12) |
			     ((string[2] & 0x3f) << 6) |
			     (string[4] & 0x3f);
			string += 3;
		    } else {
			uc = c;
		    }
		} else if (c < 0xfc) {
		    if (((string[1] & 0xc0) == 0x80) &&
			((string[2] & 0xc0) == 0x80) &&
			((string[3] & 0xc0) == 0x80) &&
			((string[4] & 0xc0) == 0x80)) {
			uc = ((c & 0x01) << 24) |
			     ((string[1] & 0x3f) << 18) |
			     ((string[2] & 0x3f) << 12) |
			     ((string[4] & 0x3f) << 6) |
			     (string[5] & 0x3f);
			string += 4;
		    } else {
			uc = c;
		    }
		} else {
		    /* ignore */
		    ++string;
		}
		if (uc < 0x10000) {
		    sprintf(buf, "\\u%04lx", uc);
		} else if (uc < 0x100000) {
		    uc -= 0x10000;

		    sprintf(buf, "\\u%04lx", 0xd800 | ((uc >> 10) & 0x3ff));
		    json_pstr(buf, pfs);
		    sprintf(buf, "\\u%04lx", 0xdc00 | (uc & 0x3ff));
		} else {
		    strcpy(buf, "\\ufffd");
		}
		json_pstr(buf, pfs);
	    } else {
		pfunc(*string, parg);
	    }
	    break;
	}
	++string;
    }
    pfunc('"', parg);
}

/**
 * Conditionally quote and write string using JSON output function
 * @param string string to be written
 * @param pfs JSON output function
 */

static void
json_pstrc(const char *string, json_pfs *pfs)
{
    if (*string && strchr(".0123456789-+", *string)) {
	json_pstr(string, pfs);
    } else {
	json_pstrq(string, pfs);
    }
}

/**
 * Write a blob as base64 string using JSON output function
 * @param blk pointer to blob
 * @param len length of blob
 * @param pfs JSON output function
 */

static void
json_pb64(const unsigned char *blk, int len, json_pfs *pfs)
{
    impexp_putc pfunc = pfs->pfunc;
    void *parg = pfs->parg;
    int i, reg[5];
    char buf[16];
    static const char *b64 =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

    if (!blk) {
	json_pstr("null", pfs);
	return;
    }
    buf[4] = '\0';
    pfunc('"', parg);
    for (i = 0; i < len; i += 3) {
	reg[1] = reg[2] = reg[3] = reg[4] = 0;
	reg[0] = blk[i];
	if (i + 1 < len) {
	    reg[1] = blk[i + 1];
	    reg[3] = 1;
	}
	if (i + 2 < len) {
	    reg[2] = blk[i + 2];
	    reg[4] = 1;
	}
	buf[0] = b64[reg[0] >> 2];
	buf[1] = b64[((reg[0] << 4) & 0x30) | (reg[1] >> 4)];
	if (reg[3]) {
	    buf[2] = b64[((reg[1] << 2) & 0x3c) | (reg[2] >> 6)];
	} else {
	    buf[2] = '=';
	}
	if (reg[4]) {
	    buf[3] = b64[reg[2] & 0x3f];
	} else {
	    buf[3] = '=';
	}
	json_pstr(buf, pfs);
    }
    pfunc('"', parg);
}

/**
 * Execute SQL and write output as JSON
 * @param db SQLite database pointer
 * @param sql SQL text
 * @param pfunc JSON output function
 * @param parg argument for output function
 * @result SQLite error code
 */

static int
json_output(sqlite3 *db, char *sql, impexp_putc pfunc, void *parg)
{
    json_pfs pfs0, *pfs = &pfs0;
    const char *tail = sql;
    int i, nresults = 0, result = SQLITE_ERROR;

    pfs->pfunc = pfunc;
    pfs->parg = parg;
    json_pstr("{\"sql\":", pfs);
    json_pstrq(sql, pfs);
    json_pstr(",\"results\":[", pfs);
    do {
	sqlite3_stmt *stmt;
	int firstrow = 1, nrows = 0;
	char buf[256];

	++nresults;
	json_pstr((nresults == 1) ? "{" : ",{", pfs);
	result = sqlite3_prepare(db, tail, -1, &stmt, &tail);
	if (result != SQLITE_OK) {
doerr:
	    if (nrows == 0) {
		json_pstr("\"columns\":null,\"rows\":null,\"changes\":0,"
			  "\"last_insert_rowid\":null,", pfs);
	    }
	    json_pstr("\"error:\"", pfs);
	    json_pstrq(sqlite3_errmsg(db), pfs);
	    pfunc('}', parg);
	    break;
	}
	result = sqlite3_step(stmt);
	while ((result == SQLITE_ROW) || (result == SQLITE_DONE)) {
	    if (firstrow) {
		for (i = 0; i < sqlite3_column_count(stmt); i++) {
		    char *type;

		    json_pstr((i == 0) ? "\"columns\":[" : ",", pfs);
		    json_pstr("{\"name\":", pfs);
		    json_pstrq(sqlite3_column_name(stmt, i), pfs);
		    json_pstr(",\"decltype\":", pfs);
		    json_pstrq(sqlite3_column_decltype(stmt, i), pfs);
		    json_pstr(",\"type\":", pfs);
		    switch (sqlite3_column_type(stmt, i)) {
		    case SQLITE_INTEGER:
			type = "integer";
			break;
		    case SQLITE_FLOAT:
			type = "float";
			break;
		    case SQLITE_BLOB:
			type = "blob";
			break;
		    case SQLITE_TEXT:
			type = "text";
			break;
		    case SQLITE_NULL:
			type = "null";
			break;
		    default:
			type = "unknown";
			break;
		    }
		    json_pstrq(type, pfs);
		    pfunc('}', parg);
		}
		if (i) {
		    pfunc(']', parg);
		}
		firstrow = 0;
	    }
	    if (result == SQLITE_DONE) {
		break;
	    }
	    ++nrows;
	    json_pstr((nrows == 1) ? ",\"rows\":[" : ",", pfs);
	    for (i = 0; i < sqlite3_column_count(stmt); i++) {
		pfunc((i == 0) ? '[' : ',', parg);
		switch (sqlite3_column_type(stmt, i)) {
		case SQLITE_INTEGER:
		    json_pstr((char *) sqlite3_column_text(stmt, i), pfs);
		    break;
		case SQLITE_FLOAT:
		    json_pstrc((char *) sqlite3_column_text(stmt, i), pfs);
		    break;
		case SQLITE_BLOB:
		    json_pb64((unsigned char *) sqlite3_column_blob(stmt, i),
			      sqlite3_column_bytes(stmt, i), pfs);
		    break;
		case SQLITE_TEXT:
		    json_pstrq((char *) sqlite3_column_text(stmt, i), pfs);
		    break;
		case SQLITE_NULL:
		default:
		    json_pstr("null", pfs);
		    break;
		}
	    }
	    json_pstr((i == 0) ? "null]" : "]", pfs);
	    result = sqlite3_step(stmt);
	}
	if (nrows > 0) {
	    pfunc(']', parg);
	}
	result = sqlite3_finalize(stmt);
	if (result != SQLITE_OK) {
	    if (nrows > 0) {
		sprintf(buf,
#ifdef _WIN32
			",\"changes\":%d,\"last_insert_rowid\":%I64d",
#else
			",\"changes\":%d,\"last_insert_rowid\":%lld",
#endif
			sqlite3_changes(db),
			sqlite3_last_insert_rowid(db));
		json_pstr(buf, pfs);
	    }
	    goto doerr;
	}
	if (nrows == 0) {
	    json_pstr("\"columns\":null,\"rows\":null", pfs);
	}
	sprintf(buf,
#ifdef _WIN32
		",\"changes\":%d,\"last_insert_rowid\":%I64d",
#else
		",\"changes\":%d,\"last_insert_rowid\":%lld",
#endif
		sqlite3_changes(db),
		sqlite3_last_insert_rowid(db));
	json_pstr(buf, pfs);
	json_pstr(",\"error\":null}", pfs);
    } while (tail && *tail);
    json_pstr("]}", pfs);
    return result;
}

/**
 * SQLite function for JSON output, see impexp_export_json
 * @param ctx SQLite function context
 * @param nargs number of arguments
 * @param args argument vector
 */

static void
export_json_func(sqlite3_context *ctx, int nargs, sqlite3_value **args)
{
    sqlite3 *db = (sqlite3 *) sqlite3_user_data(ctx);
    int result = -1;
    char *filename = 0;
    char *sql = 0;
    FILE *out = 0;
#ifdef _WIN32
    char fnbuf[MAX_PATH];
#endif

    if (nargs > 0) {
	if (sqlite3_value_type(args[0]) != SQLITE_NULL) {
	    filename = (char *) sqlite3_value_text(args[0]);
	}
    }
#ifdef _WIN32
    if (!filename) {
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof (ofn));
	memset(fnbuf, 0, sizeof (fnbuf));
	ofn.lStructSize = sizeof (ofn);
	ofn.lpstrFile = fnbuf;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER |
		    OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	if (GetSaveFileName(&ofn)) {
	    filename = fnbuf;
	}
    }
#endif
    if (!filename) {
	goto done;
    }
    out = fopen(filename, "w");
    if (!out) {
	goto done;
    }
    if (nargs > 1) {
	sql = (char *) sqlite3_value_text(args[1]);
    }
    if (sql) {
	result = json_output(db, sql, (impexp_putc) fputc, out);
    }
    fclose(out);
done:
    sqlite3_result_int(ctx, result);
}

/* see doc in impexp.h */

int
impexp_export_json(sqlite3 *db, char *sql, impexp_putc pfunc,
		   void *parg)
{
    return json_output(db, sql, pfunc, parg);
}

/**
 * Initializer for SQLite extension load mechanism.
 * @param db SQLite database pointer
 * @param errmsg pointer receiving error message
 * @param api SQLite API routines
 * @result SQLite error code
 */

#ifdef STANDALONE
static int
#else
int
#endif
sqlite3_extension_init(sqlite3 *db, char **errmsg, 
		       const sqlite3_api_routines *api)
{
    int rc, i;
    static const struct {
	const char *name;
	void (*func)(sqlite3_context *, int, sqlite3_value **);
	int nargs;
	int textrep;
    } ftab[] = {
	{ "quote_sql",	 quote_func,       -1, SQLITE_UTF8 },
	{ "import_sql",	 import_func,      -1, SQLITE_UTF8 },
	{ "export_sql",	 export_func,      -1, SQLITE_UTF8 },
	{ "quote_csv",	 quote_csv_func,   -1, SQLITE_UTF8 },
	{ "export_csv",	 export_csv_func,  -1, SQLITE_UTF8 },
	{ "indent_xml",	 indent_xml_func,   1, SQLITE_UTF8 },
	{ "quote_xml",	 quote_xml_func,   -1, SQLITE_UTF8 },
	{ "export_xml",  export_xml_func,  -1, SQLITE_UTF8 },
	{ "export_json", export_json_func, -1, SQLITE_UTF8 }
    };

#ifndef STANDALONE
    if (api != NULL) {
	SQLITE_EXTENSION_INIT2(api);
    }
#endif

    for (i = 0; i < sizeof (ftab) / sizeof (ftab[0]); i++) {
	rc = sqlite3_create_function(db, ftab[i].name, ftab[i].nargs,
				     ftab[i].textrep, db, ftab[i].func, 0, 0);
	if (rc != SQLITE_OK) {
	    for (--i; i >= 0; --i) {
		sqlite3_create_function(db, ftab[i].name, ftab[i].nargs,
					ftab[i].textrep, 0, 0, 0, 0);
	    }
	    break;
	}
    }
    return rc;
}

/* see doc in impexp.h */

int
impexp_init(sqlite3 *db)
{
    return sqlite3_extension_init(db, NULL, NULL);
}
