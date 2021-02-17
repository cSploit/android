#ifndef _IMPEXP_H
#define _IMPEXP_H

/**
 * @file impexp.h
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
 */

/**
 * Reads SQL commands from filename and executes them
 * against the current database. Returns the number
 * of changes to the current database.
 * @param db SQLite database pointer
 * @param filename name of input file
 * @result number of changes to database
 */

int impexp_import_sql(sqlite3 *db, char *filename);

/**
 * Writes SQL to filename similar to SQLite's shell
 * ".dump" meta command. Mode selects the output format.
 * @param db SQLite database pointer
 * @param filename name of output file
 * @param mode selects output format
 * @param ... optional table names or tuples of table name,
 * and where-clause depending on mode parameter
 * @result approximate number of lines written or
 * -1 when an error occurred
 *
 * Mode 0 (default): dump schema and data using the
 * optional table names following the mode argument.<br>
 * Mode 1: dump data only using the optional table
 * names following the mode argument.<br>
 * Mode 2: dump schema and data using the optional
 * table names following the mode argument; each
 * table name is followed by a WHERE clause, i.e.
 * "mode, table1, where1, table2, where2, ..."<br>
 * Mode 3: dump data only, same rules as in mode 2.<br>
 *
 * Other flags in mode:
 *
 * <pre>
 *       Bit 1 of mode:      when 1 dump data only
 *       Bits 8..9 of mode:  blob quoting mode
 *
 *           0   default
 *         256   ORACLE
 *         512   SQL Server
 *         768   MySQL
 * </pre>
 */

int impexp_export_sql(sqlite3 *db, char *filename, int mode, ...);

/**
 * Writes entire tables as CSV to provided filename. A header
 * row is written when the hdr parameter is true. The
 * rows are optionally introduced with a column made up of
 * the prefix (non-empty string) for the respective table.
 * If "schema" is NULL, "sqlite_master" is used, otherwise
 * specify e.g. "sqlite_temp_master" for temporary tables or 
 * "att.sqlite_master" for the attached database "att".
 * @param db SQLite database pointer
 * @param filename name of output file
 * @param hdr write header lines when true
 * @param ... tuples of prefix, table name, schema name
 * @result number of output lines
 *
 * Example:
 *
 * <pre>
 *   CREATE TABLE A(a,b);
 *   INSERT INTO A VALUES(1,2);
 *   INSERT INTO A VALUES(3,'foo')
 *   CREATE TABLE B(c);
 *   INSERT INTO B VALUES('hello');
 *   SELECT export_csv('out.csv', 0, 'aa', 'A', NULL, 'bb', 'B', NULL);
 *   -- CSV output
 *   "aa",1,2
 *   "aa",3,"foo"
 *   "bb","hello"
 *   SELECT export_csv('out.csv', 1, 'aa', 'A', NULL, 'bb', 'B', NULL);
 *   -- CSV output
 *   "aa","a","b"
 *   "aa",1,2
 *   "aa",3,"foo"
 *   "bb","c"
 *   "bb","hello"
 * </pre>
 */

int impexp_export_csv(sqlite3 *db, char *filename, int hdr, ...);

/**
 * Writes a table as simple XML to provided filename. The
 * rows are optionally enclosed with the "root" tag,
 * the row data is enclosed in "item" tags. If "schema"
 * is NULL, "sqlite_master" is used, otherwise specify
 * e.g. "sqlite_temp_master" for temporary tables or 
 * "att.sqlite_master" for the attached database "att".
 * @param db SQLite database pointer
 * @param filename name of output file
 * @param append if true, append to existing output file
 * @param indent number of blanks to indent output
 * @param root optional tag use to enclose table output
 * @param item tag to use per row
 * @param tablename table to be output
 * @param schema optional schema or NULL
 * @result number of output lines
 *
 * Layout of an output row:
 * <pre>
 *  <item>
 *   <columnname TYPE="INTEGER|REAL|NULL|TEXT|BLOB">value</columnname>
 *   ...
 *  </item>
 * </pre>
 *
 * Example:
 * <pre>
 *  CREATE TABLE A(a,b);
 *  INSERT INTO A VALUES(1,2.1);
 *  INSERT INTO A VALUES(3,'foo');
 *  INSERT INTO A VALUES('',NULL);
 *  INSERT INTO A VALUES(X'010203','<blob>');
 *  SELECT export_xml('out.xml', 0, 2, 'TBL_A', 'ROW', 'A');
 *  -- XML output
 *    <TBL_A>
 *       <ROW>
 *          &lt;a TYPE="INTEGER"&gt;1&lt;/a&gt;
 *          &lt;b TYPE="REAL"&gt;2.1&lt;/b&gt;
 *       </ROW>
 *       <ROW>
 *          &lt;a TYPE="INTEGER"&gt;3&lt;/a&gt;
 *          &lt;b TYPE="TEXT"&gt;foo&lt;/b&gt;
 *       </ROW>
 *       <ROW>
 *          &lt;a TYPE="TEXT"&gt;&lt;/a&gt;
 *          &lt;b TYPE="NULL"&gt;&lt;/b&gt;
 *       </ROW>
 *       <ROW>
 *          &lt;a TYPE="BLOB"&gt;&#x01;&#x02;&x03;&lt;/a&gt;
 *          &lt;b TYPE="TEXT"&gt;&amp;lt;blob&amp;gt;&lt;/b&gt;
 *       </ROW>
 *     </TBL_A>
 * </pre>
 *
 * Quoting of XML entities is performed only on the data,
 * not on column names and root/item tags.
 */

int impexp_export_xml(sqlite3 *db, char *filename,
		      int append, int indent, char *root,
		      char *item, char *tablename, char *schema);

/**
 * @typedef impexp_putc
 * The function pointer for the output function to
 * "impexp_export_json" has a signature compatible
 * with fputc(3).
 */

typedef void (*impexp_putc)(int c, void *arg);

/**
 * Executes arbitrary SQL statements and formats
 * the result in JavaScript Object Notation (JSON).
 * @param db SQLite database pointer
 * @param sql SQL to be executed
 * @param pfunc pointer to output function
 * @param parg argument for output function
 * @result SQLite error code
 *
 * The layout of the result output is:
 *
 * <pre>
 *  object {results, sql}
 *    results[] object {columns, rows, changes, last_insert_rowid, error}
 *      columns[]
 *        object {name, decltype, type }    (sqlite3_column_*)
 *      rows[][]                            (sqlite3_column_*)
 *      changes                             (sqlite3_changes)
 *      last_insert_rowid                   (sqlite3_last_insert_rowid)
 *      error                               (sqlite3_errmsg)
 *    sql                                   (SQL text)
 * </pre>
 *
 * For each single SQL statement in "sql" an object in the
 * "results" array is produced.
 */

int impexp_export_json(sqlite3 *db, char *sql, impexp_putc pfunc,
		       void *parg);

/**
 * Registers the SQLite functions
 * @param db SQLite database pointer
 * @result SQLite error code
 *
 * Registered functions:
 *
 * <pre>
 *  import_sql(filename)
 *  export_sql(filename, [mode, tablename, ...])
 *  export_csv(filename, hdr, prefix1, tablename1, schema1, ...)
 *  export_xml(filename, appendflg, indent, [root, item, tablename, schema]+)
 *  export_json(filename, sql)
 * </pre>
 *
 * On Win32 the filename argument may be specified as NULL in
 * order to open a system file dialog for interactive filename
 * selection.
 */
	
int impexp_init(sqlite3 *db);


#endif
