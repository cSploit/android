/**
 * @file blobtoxy.c
 * SQLite extension module for read-only BLOB to X/Y mapping
 * using SQLite 3.3.x virtual table API plus some useful
 * scalar and aggregate functions.
 *
 * $Id: blobtoxy.c,v 1.22 2013/01/11 12:19:55 chw Exp chw $
 *
 * Copyright (c) 2007-2013 Christian Werner <chw@ch-werner.de>
 *
 * See the file "license.terms" for information on usage
 * and redistribution of this file and for a
 * DISCLAIMER OF ALL WARRANTIES.
 *
 * Usage:
 *
 *  Master (non-virtual) table:
 *
 *   CREATE TABLE t(
 *     key INTEGER PRIMARY KEY,
 *     data BLOB,
 *     scale DOUBLE,
 *     offset DOUBLE,
 *     foo TEXT,
 *     bar TEXT
 *   );
 *
 *  BLOB to X/Y mapping:
 *
 *   CREATE VIRTUAL TABLE t1
 *     USING blobtoxy (t, key, data, short_le, x_scale, x_offset,
 *                     y_scale, y_offset, "foo, bar");
 *   <br>CREATE VIRTUAL TABLE t2
 *     USING blobtoxy (t, key, data, uchar, null, null, null, null, 'bar');
 *   <br>CREATE VIRTUAL TABLE t3
 *     USING blobtoxy (t, key, data, int_be, 10.0, null, 10.0, null, "foo");
 *   <br>CREATE VIRTUAL TABLE t4
 *     USING blobtoxy (t, key, data, float, null, -10, null, 10);
 *
 *  Arguments to "blobtoxy" module:
 *
 *  0. master table name (required).<br>
 *  1. key column in master table (required).<br>
 *  2. blob column in master table (required).<br>
 *  3. type code (optional), defaults to "char".<br>
 *  4. X scale column in master table (optional),
 *     may be specified as integer or float constant, too,
 *     to explicitely omit scale, use an empty string ('')
 *     or null.<br>
 *  5. X offset column in master table (optional),
 *     may be specified as integer or float constant, too,
 *     to explicitely omit offset, use an empty string ('')
 *     or null.<br>
 *  6. Y scale column in master table (optional), see point 4.<br>
 *  7. Y offset column in master table (optional), see point 5.<br>
 *  8. other columns of the master table to appear in the
 *     result set (optional), must be specified as a
 *     single or double quoted string with comma
 *     separated column names as a sequence of named
 *     columns as it would be written in a SELECT
 *     statement.<br>
 *  9. X start index (optional), specified as integer
 *     in type specific blob units, zero based.<br>
 * 10. X length (optional), specified as integer,
 *     number of blob units (= number of rows).<br>
 *
 *  Supported data types:
 *
 *  "char"       -> BLOB is a signed char array<br>
 *  "uchar"      -> BLOB is an unsigned char array<br>
 *  "short_le"   -> BLOB is a short array little endian<br>
 *  "short_be"   -> BLOB is a short array big endian<br>
 *  "ushort_le"  -> BLOB is an unsigned short array little endian<br>
 *  "ushort_be"  -> BLOB is an unsigned short array big endian<br>
 *  "int_le"     -> BLOB is a int array little endian<br>
 *  "int_be"     -> BLOB is a int array big endian<br>
 *  "uint_le"    -> BLOB is an unsigned int array little endian<br>
 *  "uint_be"    -> BLOB is an unsigned int array big endian<br>
 *  "bigint_le"  -> BLOB is an large integer array little endian<br>
 *  "bigint_be"  -> BLOB is an large integer array big endian<br>
 *  "float"      -> BLOB is a float array<br>
 *  "double"     -> BLOB is a double array<br>
 *
 *  Columns of "blobtoxy" mapped virtual table:
 *
 *   "key"   Key column for JOINing with master table<br>
 *   "x"     index within BLOB.<br>
 *           This value is optionally translated by
 *           the "x_scale" and "x_offset" columns
 *           i.e. x' = x * x_scale + x_offset, yielding
 *           a floating point result.<br>
 *   "y"     BLOB's value at "x"-unscaled-index<br>
 *           This value is optionally translated by
 *           the "y_scale" and "y_offset" columns
 *           i.e. y' = y * y_scale + y_offset, yielding
 *           a floating point result.<br>
 *   ...     Other columns, see above<br>
 *
 *   If the "key" field of the master table is an integer data
 *   type, it is used as the ROWID of the mapped virtual table.
 *   Otherwise the ROWID is a 0-based counter of the output rows.
 *
 *
 * Exported SQLite functions (svg|tk)_path[_from_blob], blt_vec_(x|y)
 *
 *  Scalar context:
 *
 *    svg_path_from_blob(data, type, x_scale, x_offset, y_scale, y_offset)<br>
 *     tk_path_from_blob(data, type, x_scale, x_offset, y_scale, y_offset)<br>
 *         blt_vec_(x|y)(data, type, x_scale, x_offset, y_scale, y_offset)<br>
 *   tk3d_path_from_blob(data, type, x_scale, x_offset, y_scale, y_offset,
 *                       z_value, z_scale, z_offset)<br>
 *
 *   Like BLOB to X/Y mapping but produces SVG or Tk Canvas
 *   path/polyline as a string, e.g.
 *
 *    SVG:          "M 1 1 L 2 2 L 3 7 L 4 1<br>
 *    Tk Canvas:    "1 1 2 2 3 7 4 1"<br>
 *    BLT Vector X: "1 2 3 4"<br>
 *    BLT Vector Y: "1 2 7 1"<br>
 *    Tk 3D Canvas: "1 1 0 2 2 0 3 7 0 4 1 0"<br>
 *
 *   Arguments:
 * 
 *   0. blob data (required); this parameter is always
 *      interpreted as blob. It must contain at least
 *      two elements, otherwise the function's result
 *      is NULL to indicate that nothing need be drawn<br>
 *   1. type code (optional), defaults to "char"<br>
 *   2. X scale (optional), see above<br>
 *   3. X offset (optional), see above<br>
 *   4. Y scale (optional), see above<br>
 *   5. Y offset (optional), see above<br>
 *   6. Z value (optional)<br>
 *   8. Z scale (optional)<br>
 *   9. Z offset (optional)<br>
 *
 *  Aggregate context:
 *
 *    svg_path(xdata, ydata, x_scale, x_offset, y_scale, y_offset)<br>
 *     tk_path(xdata, ydata, x_scale, x_offset, y_scale, y_offset)<br>
 *     blt_vec(data, scale, offset)<br>
 *   tk3d_path(xdata, ydata, x_scale, x_offset, y_scale, y_offset,
 *             zdata, z_scale, z_offset)<br>
 *
 *   Same behaviour except that xdata/ydata/data/zdata are interpreted
 *   directly as numeric values.
 *
 *
 * Exported SQLite function subblob
 *
 *   subblob(data, start, length, size, skip)
 *
 *  Works somewhat like substr, e.g.
 *
 *   select quote(subblob(X'0A0B0C0D0E0F0001',2,2,1,3))<br>
 *   -> X'0B0F'
 *
 *  Arguments:
 * 
 *  0. blob data (required); this parameter is always
 *     interpreted as blob.<br>
 *  1. start offset (required) in bytes as in substr
 *     function (1-based, negative offsets count from end)<br>
 *  2. length (required) in bytes to be copied<br>
 *  3. size (optional) of items in bytes to be copied
 *     in combination with skip argument<br>
 *  4. skip (optional) in bytes after one item of
 *     size argument has been copied<br>
 *
 *
 * Exported SQLite function rownumber
 *
 *   rownumber(any)
 *
 *  Returns the row number counting from 0 in simple
 *  selects. An arbitrary dummy but constant argument
 *  must be provided to this function in order to satisfy
 *  some needs of the SQLite3 C API, e.g.
 *
 *   rownumber(0), rownumber('foo')  right<br>
 *   rownumber(column_name)          wrong, will yield always 0<br>
 *
 */

#ifdef STANDALONE
#include <sqlite3.h>
#else
#include <sqlite3ext.h>
static SQLITE_EXTENSION_INIT1
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN32
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#define vsnprintf   _vsnprintf
#endif

#define TYPE_CODE(num, type) (((num) << 8) | (sizeof (type)))
#define TYPE_SIZE(code)      ((code) & 0xFF)

#define TYPE_CHAR      TYPE_CODE( 0, char)
#define TYPE_UCHAR     TYPE_CODE( 1, char)
#define TYPE_SHORT_LE  TYPE_CODE( 2, short)
#define TYPE_USHORT_LE TYPE_CODE( 3, short)
#define TYPE_SHORT_BE  TYPE_CODE( 4, short)
#define TYPE_USHORT_BE TYPE_CODE( 5, short)
#define TYPE_INT_LE    TYPE_CODE( 6, int)
#define TYPE_UINT_LE   TYPE_CODE( 7, int)
#define TYPE_INT_BE    TYPE_CODE( 8, int)
#define TYPE_UINT_BE   TYPE_CODE( 9, int)
#define TYPE_BIGINT_LE TYPE_CODE(10, sqlite_int64)
#define TYPE_BIGINT_BE TYPE_CODE(11, sqlite_int64)
#define TYPE_FLOAT     TYPE_CODE(12, float)
#define TYPE_DOUBLE    TYPE_CODE(13, double)

/**
 * @typedef b2xy_table
 * @struct b2xy_table
 * Structure to describe a virtual table.
 */

typedef struct b2xy_table {
    sqlite3_vtab base;		/**< SQLite's base virtual table struct */
    sqlite3 *db;		/**< Open database */
    char *master_table;		/**< Table where to fetch BLOB from */
    char *fq_master_table;	/**< Fully qualified master_table */
    char *key_column;		/**< Name of key column */
    char *blob_column;		/**< Name of BLOB column */
    char *x_scale_column;	/**< Name of column giving X scale or NULL */
    char *x_offset_column;	/**< Name of column giving X offset or NULL */
    char *y_scale_column;	/**< Name of column giving Y scale or NULL */
    char *y_offset_column;	/**< Name of column giving Y offset or NULL */
    char *other_columns;	/**< Other columns or empty string */
    int type;			/**< Data type of BLOB */
    int do_x_sl;		/**< If true, apply X start/length */
    int x_start, x_length;	/**< X start/length */
    int argc;			/**< Number args from b2xy_create() call */
    char **argv;		/**< Argument vector from b2xy_create() call */
} b2xy_table;

/**
 * @typedef b2xy_cursor
 * @struct b2xy_cursor
 * Structure to describe a cursor in the virtual table.
 */

typedef struct b2xy_cursor {
    sqlite3_vtab_cursor base;	/**< SQLite's base cursor struct */
    b2xy_table *table;		/**< Link to table struct */
    sqlite3_stmt *select;	/**< Prepared SELECT statement or NULL */
    sqlite3_value *key;		/**< Value of current key */
    int fix_cols;		/**< Fixed number of columns of result set */
    int num_cols;		/**< Total number of columns of result set */
    char *val;			/**< Value of current BLOB */
    int val_len;		/**< Length of current BLOB */
    int x_scale_col;		/**< Column number of X scale or 0 */
    int x_offset_col;		/**< Column number of X offset or 0 */
    double x_scale, x_offset;	/**< Current X scale and offset */
    int y_scale_col;		/**< Column number of Y scale or 0 */
    int y_offset_col;		/**< Column number of Y offset or 0 */
    double y_scale, y_offset;	/**< Current X scale and offset */
    int do_x_scale;		/**< If true, use X scale and offset */
    int do_y_scale;		/**< If true, use Y scale and offset */
    int do_x_sl;		/**< If true, apply X start/length */
    int x_start, x_length;	/**< X start/length */
    int type;			/**< Data type of BLOB */
    int index;			/**< Current index in BLOB */
    int rowid_from_key;		/**< When true, ROWID used from key column */
    sqlite_int64 rowid;		/**< Current ROWID */
} b2xy_cursor;

/**
 * Map type string to type code.
 * @param str type string, e.g. "char"
 * @result type code, e.g. TYPE_CHAR
 */

static int
string_to_type(const char *str)
{
    if (strcasecmp(str, "char") == 0) {
	return TYPE_CHAR;
    }
    if (strcasecmp(str, "uchar") == 0) {
	return TYPE_UCHAR;
    }
    if (strcasecmp(str, "short_le") == 0) {
	return TYPE_SHORT_LE;
    }
    if (strcasecmp(str, "ushort_le") == 0) {
	return TYPE_USHORT_LE;
    }
    if (strcasecmp(str, "short_be") == 0) {
	return TYPE_SHORT_BE;
    }
    if (strcasecmp(str, "ushort_be") == 0) {
	return TYPE_USHORT_BE;
    }
    if (strcasecmp(str, "int_le") == 0) {
	return TYPE_INT_LE;
    }
    if (strcasecmp(str, "uint_le") == 0) {
	return TYPE_UINT_LE;
    }
    if (strcasecmp(str, "int_be") == 0) {
	return TYPE_INT_BE;
    }
    if (strcasecmp(str, "uint_be") == 0) {
	return TYPE_UINT_BE;
    }
    if (strcasecmp(str, "bigint_le") == 0) {
	return TYPE_BIGINT_LE;
    }
    if (strcasecmp(str, "bigint_be") == 0) {
	return TYPE_BIGINT_BE;
    }
    if (strcasecmp(str, "float") == 0) {
	return TYPE_FLOAT;
    }
    if (strcasecmp(str, "double") == 0) {
	return TYPE_DOUBLE;
    }
    return 0;
}

/**
 * Destroy virtual table.
 * @param vtab virtual table pointer
 * @result always SQLITE_OK
 */

static int
b2xy_destroy(sqlite3_vtab *vtab)
{
    b2xy_table *bt = (b2xy_table *) vtab;
  
    sqlite3_free(bt);
    return SQLITE_OK;
}

/**
 * Create virtual table
 * @param db SQLite database pointer
 * @param userdata user specific pointer (unused)
 * @param argc argument count
 * @param argv argument vector
 * @param vtabret pointer receiving virtual table pointer
 * @param errp pointer receiving error messag
 * @result SQLite error code
 *
 * Argument vector contains:
 *
 * argv[0]  - module name<br>
 * argv[1]  - database name<br>
 * argv[2]  - table name (virtual table)<br>
 * argv[3]  - master table name (required)<br>
 * argv[4]  - key column (required)<br>
 * argv[5]  - blob column (required)<br>
 * argv[6]  - type code (optional)<br>
 * argv[7]  - X scale column (optional)<br>
 * argv[8]  - X offset column (optional)<br>
 * argv[9]  - Y scale column (optional)<br>
 * argv[10] - Y offset column (optional)<br>
 * argv[11] - other columns (optional)<br>
 * argv[12] - X start (optional)<br>
 * argv[13] - X length (optional)<br>
 */

static int
b2xy_create(sqlite3 *db, void *userdata, int argc,
	    const char * const *argv, sqlite3_vtab **vtabret, char **errp)
{
    int rc = SQLITE_NOMEM;
    b2xy_table *bt;
    int i, size, type = TYPE_CHAR;
    int x_start = -1, x_length = 0;

    if (argc < 6) {
	*errp = sqlite3_mprintf("need at least 3 arguments");
	return SQLITE_ERROR;
    }
    if (argc > 6) {
	type = string_to_type(argv[6]);
	if (!type) {
	    *errp = sqlite3_mprintf("unsupported type %Q", argv[6]);
	    return SQLITE_ERROR;
	}
    }
    if (argc > 11) {
	if ((argv[11][0] != '"') && (argv[11][0] != '\'')) {
	    *errp = sqlite3_mprintf("other columns must be quoted");
	    return SQLITE_ERROR;
	}
    }
    if (argc > 12) {
	char *endp = 0;

	x_start = strtol(argv[12], &endp, 10);
	if ((endp == argv[12]) || (endp && (endp[0] != '\0'))) {
	    *errp = sqlite3_mprintf("X start index must be integer");
	    return SQLITE_ERROR;
	}
	if (x_start < 0) {
	    *errp = sqlite3_mprintf("X start index must be >= 0");
	    return SQLITE_ERROR;
	}
    }
    if (argc > 13) {
	char *endp = 0;

	x_length = strtol(argv[13], &endp, 10);
	if ((endp == argv[13]) || (endp && (endp[0] != '\0'))) {
	    *errp = sqlite3_mprintf("X length must be integer");
	    return SQLITE_ERROR;
	}
	if (x_length <= 0) {
	    *errp = sqlite3_mprintf("X length must be > 0");
	    return SQLITE_ERROR;
	}
    }
    size = sizeof (char *) * argc;
    for (i = 0; i < argc; i++) {
	size += argv[i] ? (strlen(argv[i]) + 1) : 0;
    }
    /* additional space for '"' + argv[1] '"."' + argv[3] + '"' */
    size += argv[1] ? (strlen(argv[1]) + 3) : 3;
    size += argv[3] ? (strlen(argv[3]) + 3) : 0;
    bt = sqlite3_malloc(sizeof (b2xy_table) + size);
    if (bt) {
	char *p, *key_type = 0, *x_type, *y_type, *other_types = 0;

	memset(bt, 0, sizeof (b2xy_table) + size);
	bt->db = db;
	bt->type = type;
	bt->x_start = x_start;
	bt->x_length = x_length;
	bt->do_x_sl = (x_start >= 0) || (x_length > 0);
	if (bt->x_start < 0) {
	    bt->x_start = 0;
	}
	bt->argc = argc;
	bt->argv = (char **) (bt + 1);
	p = (char *) (bt->argv + argc);
	for (i = 0; i < argc; i++) {
	    if (argv[i]) {
		bt->argv[i] = p;
		strcpy(p, argv[i]);
		p += strlen(p) + 1;
	    }
	}
	bt->master_table = bt->argv[3];
	bt->fq_master_table = p;
	p[0] = '\0';
	if (bt->argv[1]) {
	    strcat(p, "\"");
	    strcat(p, bt->argv[1]);
	    strcat(p, "\".");
	}
	if (bt->argv[3]) {
	    strcat(p, "\"");
	    strcat(p, bt->argv[3]);
	    strcat(p, "\"");
	}
	bt->key_column = bt->argv[4];
	bt->blob_column = bt->argv[5];
	if ((bt->argc > 7) && bt->argv[7][0]) {
	    bt->x_scale_column = bt->argv[7];
	    if (strcasecmp(bt->x_scale_column, "null") == 0) {
		bt->x_scale_column = 0;
	    }
	}
	if ((bt->argc > 8) && bt->argv[8][0]) {
	    bt->x_offset_column = bt->argv[8];
	    if (strcasecmp(bt->x_offset_column, "null") == 0) {
		bt->x_offset_column = 0;
	    }
	}
	if ((bt->argc > 9) && bt->argv[9][0]) {
	    bt->y_scale_column = bt->argv[9];
	    if (strcasecmp(bt->y_scale_column, "null") == 0) {
		bt->y_scale_column = 0;
	    }
	}
	if ((bt->argc > 10) && bt->argv[10][0]) {
	    bt->y_offset_column = bt->argv[10];
	    if (strcasecmp(bt->y_offset_column, "null") == 0) {
		bt->y_offset_column = 0;
	    }
	}
	if (bt->argc > 11) {
	    p = bt->argv[11];
	    p[0] = ',';
	    bt->other_columns = p;
	    p += strlen(p) - 1;
	    if ((*p == '"') || (*p == '\'')) {
		*p = '\0';
	    }
	} else {
	    bt->other_columns = "";
	}
	/* find out types of key and x/y columns */
	if (bt->x_scale_column || bt->x_offset_column ||
	    (bt->type == TYPE_FLOAT) || (bt->type == TYPE_DOUBLE)) {
	    x_type = " DOUBLE";
	} else {
	    x_type = " INTEGER";
	}
	if (bt->y_scale_column || bt->y_offset_column ||
	    (bt->type == TYPE_FLOAT) || (bt->type == TYPE_DOUBLE)) {
	    y_type = " DOUBLE";
	} else {
	    y_type = " INTEGER";
	}
	p = sqlite3_mprintf("PRAGMA %Q.table_info(%Q)",
			    bt->argv[1] ? bt->argv[1] : "MAIN",
			    bt->master_table);
	if (p) {
	    int nrows = 0, ncols = 0;
	    char **rows = 0;

	    rc = sqlite3_get_table(db, p, &rows, &nrows, &ncols, 0);
	    sqlite3_free(p);
	    if (rc == SQLITE_OK) {
		for (i = 1; (ncols >= 3) && (i <= nrows); i++) {
		    p = rows[i * ncols + 1];
		    if (p && (strcasecmp(bt->key_column, p) == 0)) {
			key_type = sqlite3_mprintf(" %s", rows[i * ncols + 2]);
			break;
		    }
		}
	    }
	    if (rows) {
		sqlite3_free_table(rows);
	    }
	}
	/* find out types of other columns */
	p = 0;
	if (bt->other_columns[0]) {
	    p = sqlite3_mprintf("SELECT %s FROM %s WHERE 0",
				bt->other_columns + 1, bt->fq_master_table);
	}
	if (p) {
	    sqlite3_stmt *stmt = 0;

#if defined(HAVE_SQLITE3PREPAREV2) && HAVE_SQLITE3PREPAREV2
	    rc = sqlite3_prepare_v2(db, p, -1, &stmt, 0);
#else
	    rc = sqlite3_prepare(db, p, -1, &stmt, 0);
#endif
	    sqlite3_free(p);
	    if ((rc == SQLITE_OK) && stmt) {
		sqlite3_step(stmt);
		for (i = 0; i < sqlite3_column_count(stmt); i++) {
		    p = sqlite3_mprintf("%s%s\"%s\" %s",
					other_types ? other_types : "",
					other_types ? "," : "",
					sqlite3_column_name(stmt, i),
					sqlite3_column_decltype(stmt, i));
		    sqlite3_free(other_types);
		    other_types = 0;
		    if (p) {
			other_types = p;
		    } else {
			break;
		    }
		}
		sqlite3_finalize(stmt);
		if (other_types) {
		    p = sqlite3_mprintf(",%s", other_types);
		    sqlite3_free(other_types);
		    other_types = p;
		}
	    }
	}
	p = sqlite3_mprintf("CREATE TABLE \"%s\"(key%s CONSTRAINT fk "
			    "REFERENCES \"%s\"(\"%s\"),x%s,y%s%s)",
			    argv[2], key_type ? key_type : "",
			    bt->master_table, bt->key_column,
			    x_type, y_type,
			    other_types ? other_types : bt->other_columns);
	if (key_type) {
	    sqlite3_free(key_type);
	}
	if (other_types) {
	    sqlite3_free(other_types);
	}
	if (p) {
	    rc = sqlite3_declare_vtab(db, p);
	    sqlite3_free(p);
	}
	if (rc != SQLITE_OK) {
	    b2xy_destroy((sqlite3_vtab *) bt);
	    bt = 0;
	}
    }
    *vtabret = (sqlite3_vtab *) bt;
    return rc;
}

/**
 * Open virtual table and return cursor.
 * @param vtab virtual table pointer
 * @param curret pointer receiving cursor pointer
 * @result SQLite error code
 */

static int
b2xy_open(sqlite3_vtab *vtab, sqlite3_vtab_cursor **curret)
{
    int rc = SQLITE_NOMEM;
    b2xy_table *bt = (b2xy_table *) vtab;
    b2xy_cursor *bc;

    bc = sqlite3_malloc(sizeof (b2xy_cursor));
    if (bc) {
	memset(bc, 0, sizeof(b2xy_cursor));
	bc->table = bt;
	bc->type = bt->type;
	bc->do_x_sl = bt->do_x_sl;
	bc->x_start = bt->x_start;
	bc->x_length = bt->x_length;
	*curret = (sqlite3_vtab_cursor *) bc;
	rc = SQLITE_OK;
    }
    return rc;
}

/**
 * Close virtual table cursor.
 * @param cur cursor pointer
 * @result SQLite error code
 */

static int
b2xy_close(sqlite3_vtab_cursor *cur)
{
    b2xy_cursor *bc = (b2xy_cursor *) cur;

    sqlite3_finalize(bc->select);
    sqlite3_free(bc);
    return SQLITE_OK;
}

/**
 * Return column data of virtual table.
 * @param cur virtual table cursor
 * @param ctx SQLite function context
 * @param i column index
 * @result SQLite error code
 */

static int
b2xy_column(sqlite3_vtab_cursor *cur, sqlite3_context *ctx, int i)
{
    b2xy_cursor *bc = (b2xy_cursor *) cur;
    char *p;
    double v;

    switch (i) {
    case 0:
	sqlite3_result_value(ctx, bc->key);
	break;
    case 1:
	if (bc->do_x_scale) {
	    v = bc->index * bc->x_scale + bc->x_offset;
	    sqlite3_result_double(ctx, v);
	} else {
	    sqlite3_result_int(ctx, bc->index);
	}
	break;
    case 2:
	if (!bc->val ||
	    ((bc->index + 1) * TYPE_SIZE(bc->type) > bc->val_len)) {
	    goto put_null;
	}
	p = bc->val + bc->index * TYPE_SIZE(bc->type);
	switch (bc->type) {
	case TYPE_CHAR:
	    if (bc->do_y_scale) {
		v = p[0];
		goto scale_it;
	    }
	    sqlite3_result_int(ctx, p[0]);
	    break;
	case TYPE_UCHAR:
	    if (bc->do_y_scale) {
		v = p[0] & 0xFF;
		goto scale_it;
	    }
	    sqlite3_result_int(ctx, p[0] & 0xFF);
	    break;
	case TYPE_SHORT_LE:
	    if (bc->do_y_scale) {
		v = (p[0] & 0xFF) | (p[1] << 8);
		goto scale_it;
	    }
	    sqlite3_result_int(ctx, (p[0] & 0xFF) | (p[1] << 8));
	    break;
	case TYPE_USHORT_LE:
	    if (bc->do_y_scale) {
		v = (p[0] & 0xFF) | ((p[1] & 0xFF) << 8);
		goto scale_it;
	    }
	    sqlite3_result_int(ctx, (p[0] & 0xFF) | ((p[1] & 0xFF) << 8));
	    break;
	case TYPE_SHORT_BE:
	    if (bc->do_y_scale) {
		v = (p[1] & 0xFF) | (p[0] << 8);
		goto scale_it;
	    }
	    sqlite3_result_int(ctx, (p[1] & 0xFF) | (p[0] << 8));
	    break;
	case TYPE_USHORT_BE:
	    if (bc->do_y_scale) {
		v = (p[1] & 0xFF) | ((p[0] & 0xFF) << 8);
		goto scale_it;
	    }
	    sqlite3_result_int(ctx, (p[1] & 0xFF) | ((p[0] & 0xFF) << 8));
	    break;
	case TYPE_INT_LE:
	    if (bc->do_y_scale) {
		v = (p[0] & 0xFF) | ((p[1] & 0xFF) << 8) |
		    ((p[2] & 0xFF) << 16) | (p[3] << 24);
		goto scale_it;
	    }
	    sqlite3_result_int64(ctx,  (p[0] & 0xFF) |
				 ((p[1] & 0xFF) << 8) |
				 ((p[2] & 0xFF) << 16) |
				 (p[3] << 24));
	    break;
	case TYPE_UINT_LE:
	    if (bc->do_y_scale) {
		v = (p[0] & 0xFF) | ((p[1] & 0xFF) << 8) |
		    ((p[2] & 0xFF) << 16) | ((p[3] & 0xFF) << 24);
		goto scale_it;
	    }
	    sqlite3_result_int64(ctx,  (p[0] & 0xFF) |
				 ((p[1] & 0xFF) << 8) |
				 ((p[2] & 0xFF) << 16) |
				 ((p[3] & 0xFF) << 24));
	    break;
	case TYPE_INT_BE:
	    if (bc->do_y_scale) {
		v = (p[3] & 0xFF) | ((p[2] & 0xFF) << 8) |
		    ((p[1] & 0xFF) << 16) | (p[0] << 24);
		goto scale_it;
	    }
	    sqlite3_result_int64(ctx,  (p[3] & 0xFF) |
				 ((p[2] & 0xFF) << 8) |
				 ((p[1] & 0xFF) << 16) |
				 (p[0] << 24));
	    break;
	case TYPE_UINT_BE:
	    if (bc->do_y_scale) {
		v = (p[3] & 0xFF) | ((p[2] & 0xFF) << 8) |
		    ((p[1] & 0xFF) << 16) | ((p[0] & 0xFF) << 24);
		goto scale_it;
	    }
	    sqlite3_result_int64(ctx,  (p[3] & 0xFF) |
				 ((p[2] & 0xFF) << 8) |
				 ((p[1] & 0xFF) << 16) |
				 ((p[0] & 0xFF) << 24));
	    break;
	case TYPE_BIGINT_LE:
	    if (bc->do_y_scale) {
		v = (p[0] & 0xFFLL) | ((p[1] & 0xFFLL) << 8) |
		    ((p[2] & 0xFFLL) << 16) | ((p[3] & 0xFFLL) << 24) |
		    ((p[4] & 0xFFLL) << 32) | ((p[5] & 0xFFLL) << 40) |
		    ((p[6] & 0xFFLL) << 48) | ((p[6] & 0xFFLL) << 56);
		goto scale_it;
	    }
	    sqlite3_result_int64(ctx,  (p[0] & 0xFFLL) |
				 ((p[1] & 0xFFLL) << 8) |
				 ((p[2] & 0xFFLL) << 16) |
				 ((p[3] & 0xFFLL) << 24) |
				 ((p[4] & 0xFFLL) << 32) |
				 ((p[5] & 0xFFLL) << 40) |
				 ((p[6] & 0xFFLL) << 48) |
				 ((p[7] & 0xFFLL) << 56));
	    break;
	case TYPE_BIGINT_BE:
	    if (bc->do_y_scale) {
		v = (p[7] & 0xFFLL) | ((p[6] & 0xFFLL) << 8) |
		    ((p[5] & 0xFFLL) << 16) | ((p[4] & 0xFFLL) << 24) |
		    ((p[3] & 0xFFLL) << 32) | ((p[2] & 0xFFLL) << 40) |
		    ((p[1] & 0xFFLL) << 48) | ((p[0] & 0xFFLL) << 56);
		goto scale_it;
	    }
	    sqlite3_result_int64(ctx,  (p[7] & 0xFFLL) |
				 ((p[6] & 0xFFLL) << 8) |
				 ((p[5] & 0xFFLL) << 16) |
				 ((p[4] & 0xFFLL) << 24) |
				 ((p[3] & 0xFFLL) << 32) |
				 ((p[2] & 0xFFLL) << 40) |
				 ((p[1] & 0xFFLL) << 48) |
				 ((p[0] & 0xFFLL) << 56));
	    break;
	case TYPE_FLOAT:
	    v = ((float *) p)[0];
	    goto scale_it;
	case TYPE_DOUBLE:
	    v = ((double *) p)[0];
	    if (bc->do_y_scale) {
	scale_it:
		v = v * bc->y_scale + bc->y_offset;
	    }
	    sqlite3_result_double(ctx, v);
	    break;
	default:
	put_null:
	    sqlite3_result_null(ctx);
	    break;
	}
	break;
    default:
	i += bc->fix_cols - 3;
	if ((i < 0) || (i >= bc->num_cols)) {
	    sqlite3_result_null(ctx);
	} else {
	    sqlite3_result_value(ctx, sqlite3_column_value(bc->select, i));
	}
	break;
    }
    return SQLITE_OK;
}

/**
 * Return current rowid of virtual table cursor
 * @param cur virtual table cursor
 * @param rowidp value buffer to receive current rowid
 * @result SQLite error code
 */

static int
b2xy_rowid(sqlite3_vtab_cursor *cur, sqlite_int64 *rowidp)
{
    b2xy_cursor *bc = (b2xy_cursor *) cur;

    *rowidp = bc->rowid;
    return SQLITE_OK;
}

/**
 * Return end of table state of virtual table cursor
 * @param cur virtual table cursor
 * @result true/false
 */

static int
b2xy_eof(sqlite3_vtab_cursor *cur)
{
    b2xy_cursor *bc = (b2xy_cursor *) cur;

    return bc->select ? 0 : 1;
}

/**
 * Retrieve next row from virtual table cursor
 * @param cur virtual table cursor
 * @result SQLite error code
 */

static int
b2xy_next(sqlite3_vtab_cursor *cur)
{
    b2xy_cursor *bc = (b2xy_cursor *) cur;
    b2xy_table *bt = bc->table;
    int rc, dofetch = 0;

    if (!bc->select) {
	return SQLITE_OK;
    }
    if (bc->val) {
	bc->index += 1;
    }
    if (!bc->val) {
	dofetch = 1;
    } else if (bc->do_x_sl && bc->x_length) {
	if (bc->index >= bc->x_start + bc->x_length) {
	    dofetch = 1;
	}
    } else if ((bc->index + 1) * TYPE_SIZE(bc->type) > bc->val_len) {
	dofetch = 1;
    }
    if (dofetch) {
refetch:
	rc = sqlite3_step(bc->select);

	if (rc == SQLITE_SCHEMA) {
	    rc = sqlite3_step(bc->select);
	}	
	if (rc != SQLITE_ROW) {
	    sqlite3_finalize(bc->select);
	    bc->select = 0;
	    return SQLITE_OK;
	}
	bc->rowid_from_key = 0;
	bc->index = bc->x_start;
	bc->val = (char *) sqlite3_column_blob(bc->select, 1);
	bc->val_len = sqlite3_column_bytes(bc->select, 1);
	if (!bc->val) {
	    if (bc->do_x_sl && bc->x_length) {
		bc->val = (char *) sqlite3_column_text(bc->select, 1);
		if (!bc->val) {
		    goto refetch;
		}
	    } else {
		goto refetch;
	    }
	}
	if (!(bc->do_x_sl && bc->x_length) &&
	    (((bc->index + 1) * TYPE_SIZE(bc->type) > bc->val_len))) {
	    goto refetch;
	}
	bc->key = sqlite3_column_value(bc->select, 0);
	if (sqlite3_column_type(bc->select, 0) == SQLITE_INTEGER) {
	    bc->rowid_from_key = 1;
	    bc->rowid = sqlite3_column_int64(bc->select, 0);
	}
	bc->do_x_scale = 0;
	bc->x_scale = 1.0;
	bc->x_offset = 0.0;
	if (bt->x_scale_column) {
	    bc->x_scale = sqlite3_column_double(bc->select, bc->x_scale_col);
	    bc->do_x_scale++;
	}
	if (bt->x_offset_column) {
	    bc->x_offset = sqlite3_column_double(bc->select, bc->x_offset_col);
	    bc->do_x_scale++;
	}
	bc->do_y_scale = 0;
	bc->y_scale = 1.0;
	bc->y_offset = 0.0;
	if (bt->y_scale_column) {
	    bc->y_scale = sqlite3_column_double(bc->select, bc->y_scale_col);
	    bc->do_y_scale++;
	}
	if (bt->y_offset_column) {
	    bc->y_offset = sqlite3_column_double(bc->select, bc->y_offset_col);
	    bc->do_y_scale++;
	}
    }
    if (!bc->rowid_from_key) {
	bc->rowid++;
    }
    return SQLITE_OK;
}

/**
 * Filter function for virtual table.
 * @param cur virtual table cursor
 * @param idxNum used for expression (<, =, >, etc.)
 * @param idxStr optional order by clause
 * @param argc number arguments (0 or 1)
 * @param argv argument (nothing or RHS of filter expression)
 * @result SQLite error code
 */

static int
b2xy_filter(sqlite3_vtab_cursor *cur, int idxNum, const char *idxStr,
	    int argc, sqlite3_value **argv)
{
    b2xy_cursor *bc = (b2xy_cursor *) cur;
    b2xy_table *bt = bc->table;
    char *query, *tmp, *op = 0;
    int rc;

    bc->rowid_from_key = 0;
    bc->rowid = 0;
    if (bc->select) {
	sqlite3_finalize(bc->select);
	bc->select = 0;
    }
    bc->fix_cols = 2;
    query = sqlite3_mprintf("select \"%s\",\"%s\"", bt->key_column,
			    bt->blob_column);
    if (!query) {
	return SQLITE_NOMEM;
    }
    if (bt->x_scale_column) {
	tmp = sqlite3_mprintf("%s,\"%s\"", query, bt->x_scale_column);
	sqlite3_free(query);
	if (!tmp) {
	    return SQLITE_NOMEM;
	}
	query = tmp;
	bc->x_scale_col = bc->fix_cols;
	bc->fix_cols++;
    }
    if (bt->x_offset_column) {
	tmp = sqlite3_mprintf("%s,\"%s\"", query, bt->x_offset_column);
	sqlite3_free(query);
	if (!tmp) {
	    return SQLITE_NOMEM;
	}
	query = tmp;
	bc->x_offset_col = bc->fix_cols;
	bc->fix_cols++;
    }
    if (bt->y_scale_column) {
	tmp = sqlite3_mprintf("%s,\"%s\"", query, bt->y_scale_column);
	sqlite3_free(query);
	if (!tmp) {
	    return SQLITE_NOMEM;
	}
	query = tmp;
	bc->y_scale_col = bc->fix_cols;
	bc->fix_cols++;
    }
    if (bt->y_offset_column) {
	tmp = sqlite3_mprintf("%s,\"%s\"", query, bt->y_offset_column);
	sqlite3_free(query);
	if (!tmp) {
	    return SQLITE_NOMEM;
	}
	query = tmp;
	bc->y_offset_col = bc->fix_cols;
	bc->fix_cols++;
    }
    tmp = sqlite3_mprintf("%s%s from %s", query, bt->other_columns,
			  bt->fq_master_table);
    sqlite3_free(query);
    if (!tmp) {
	return SQLITE_NOMEM;
    }
    query = tmp;
    if (idxNum && (argc > 0)) {
	switch (idxNum) {
	case SQLITE_INDEX_CONSTRAINT_EQ:
	    op = "=";
	    break;
	case SQLITE_INDEX_CONSTRAINT_GT:
	    op = ">";
	    break;
	case SQLITE_INDEX_CONSTRAINT_LE:
	    op = "<=";
	    break;
	case SQLITE_INDEX_CONSTRAINT_LT:
	    op = "<";
	    break;
	case SQLITE_INDEX_CONSTRAINT_GE:
	    op = ">=";
	    break;
	case SQLITE_INDEX_CONSTRAINT_MATCH:
	    op = "like";
	    break;
	}
	if (op) {
	    tmp = sqlite3_mprintf("%s where \"%s\" %s ?",
				  query, bt->key_column, op);
	    sqlite3_free(query);
	    if (!tmp) {
		return SQLITE_NOMEM;
	    }
	    query = tmp;
	}
    }
    if (idxStr) {
	tmp = sqlite3_mprintf("%s %s", query, idxStr);
	sqlite3_free(query);
	if (!tmp) {
	    return SQLITE_NOMEM;
	}
	query = tmp;
    }
    bc->num_cols = bc->fix_cols;
#if defined(HAVE_SQLITE3PREPAREV2) && HAVE_SQLITE3PREPAREV2
    rc = sqlite3_prepare_v2(bt->db, query, -1, &bc->select, 0);
#else
    rc = sqlite3_prepare(bt->db, query, -1, &bc->select, 0);
    if (rc == SQLITE_SCHEMA) {
	rc = sqlite3_prepare(bt->db, query, -1, &bc->select, 0);
    }
#endif
    sqlite3_free(query);
    if (rc == SQLITE_OK) {
	bc->num_cols = sqlite3_column_count(bc->select);
	if (op) {
	    sqlite3_bind_value(bc->select, 1, argv[0]);
	}
    }
    return (rc == SQLITE_OK) ? b2xy_next(cur) : rc;
}

/**
 * Determines information for filter function
 * according to constraints.
 * @param tab virtual table
 * @param info index/constraint iinformation
 * @result SQLite error code
 */

static int
b2xy_bestindex(sqlite3_vtab *tab, sqlite3_index_info *info)
{
    b2xy_table *bt = (b2xy_table *) tab;
    int i, key_order = 0, consumed = 0;

    /* preset to not using index */
    info->idxNum = 0;

    /*
     * Only when the key column of the master table
     * (0th column in virtual table) is used in a
     * constraint, a WHERE condition in the xFilter
     * function can be coded. This is indicated by
     * setting "idxNum" to the "op" value of that
     * constraint.
     */
    for (i = 0; i < info->nConstraint; ++i) {
	if (info->aConstraint[i].usable) {
	    if ((info->aConstraint[i].iColumn == 0) &&
		(info->aConstraint[i].op != 0)) {
		info->idxNum = info->aConstraint[i].op;
		info->aConstraintUsage[i].argvIndex = 1;
		info->aConstraintUsage[i].omit = 1;
		info->estimatedCost = 1.0;
		break;
	    }
	}
    }

    /*
     * ORDER BY can be optimized, when our X column
     * is not present or to be sorted ascending.
     * Additionally when the key column is to be sorted
     * an ORDER BY is sent to the xFilter function.
     */
    for (i = 0; i < info->nOrderBy; i++) {
	if (info->aOrderBy[i].iColumn == 0) {
	    key_order = info->aOrderBy[i].desc ? -1 : 1;
	    consumed++;
	} else if ((info->aOrderBy[i].iColumn == 1) &&
		   !info->aOrderBy[i].desc) {
	    consumed++;
	}
    }
    if (consumed) {
	/* check for other ORDER BY columns */
	for (i = 0; i < info->nOrderBy; i++) {
	    if ((info->aOrderBy[i].iColumn == 1) &&
		info->aOrderBy[i].desc) {
		consumed = 0;
	    } else if (info->aOrderBy[i].iColumn > 1) {
		consumed = 0;
	    }
	}
    }
    if (consumed && key_order) {
	info->idxStr = sqlite3_mprintf("ORDER BY \"%s\" %s",
				       bt->key_column,
				       (key_order < 0) ? "DESC" : "ASC");
	info->needToFreeIdxStr = 1;
    }
    info->orderByConsumed = consumed;
    return SQLITE_OK;
}

#if (SQLITE_VERSION_NUMBER > 3004000)

/**
 * Rename virtual table.
 * @param newname new name for table
 * @result SQLite error code
 */

static int
b2xy_rename(sqlite3_vtab *tab, const char *newname)
{
    return SQLITE_OK;
}

#endif

static const sqlite3_module b2xy_module = {
    1,              /* iVersion */
    b2xy_create,    /* xCreate */
    b2xy_create,    /* xConnect */
    b2xy_bestindex, /* xBestIndex */
    b2xy_destroy,   /* xDisconnect */
    b2xy_destroy,   /* xDestroy */
    b2xy_open,      /* xOpen */
    b2xy_close,     /* xClose */
    b2xy_filter,    /* xFilter */
    b2xy_next,      /* xNext */
    b2xy_eof,       /* xEof */
    b2xy_column,    /* xColumn */
    b2xy_rowid,     /* xRowid */
    0,              /* xUpdate */
    0,              /* xBegin */
    0,              /* xSync */
    0,              /* xCommit */
    0,              /* xRollback */
    0,              /* xFindFunction */
#if (SQLITE_VERSION_NUMBER > 3004000)
    b2xy_rename,    /* xRename */
#endif
};

/**
 * @typedef strbuf
 * @struct strbuf
 * Internal dynamic string buffer.
 */

typedef struct {
    int max;		/**< maximum capacity */
    int idx;		/**< current index */
    char *str;		/**< string buffer */
} strbuf;

/**
 * Initialize dynamic string buffer with capacity 1024.
 * @param sb pointer to string buffer
 * @result SQLite error code
 */

static int
init_strbuf(strbuf *sb)
{
    int n = 1024;

    if ((sb->max <= 0) || !sb->str) {
	sb->str = sqlite3_malloc(n);
	if (!sb->str) {
	    return SQLITE_NOMEM;
	}
	sb->max = n;
    }
    sb->idx = 0;
    return SQLITE_OK;
}

/**
 * Expand or initialize dynamic string buffer.
 * @param sb pointer to string buffer
 * @result SQLite error code
 */

static int
expand_strbuf(strbuf *sb)
{
    int n;
    char *str;

    if ((sb->max <= 0) || !sb->str) {
	return init_strbuf(sb);
    }
    n = sb->max * 2;
    str = sqlite3_realloc(sb->str, n);
    if (!str) {
	return SQLITE_NOMEM;
    }
    sb->max = n;
    sb->str = str;
    return SQLITE_OK;
}

/**
 * Free resources of dynamic string buffer.
 * @param sb pointer to string buffer
 */

static void
drop_strbuf(strbuf *sb)
{
    if (sb->str) {
	sqlite3_free(sb->str);
	sb->str = 0;
    }
    sb->max = 0;
}

/**
 * Format printf-like into dynamic string buffer.
 * @param sb pointer to string buffer
 * @param fmt printf-like format string
 * @result SQLite error code
 */

static int
print_strbuf(strbuf *sb, const char *fmt, ...)
{
    int i, n, rc;
    va_list ap;

    va_start(ap, fmt);
    for (i = 0; i < 2; i++) {
	if (sb->max - (sb->idx + 1) < 256) {
	    rc = expand_strbuf(sb);
	    if (rc != SQLITE_OK) {
		return rc;
	    }
	}
	rc = SQLITE_NOMEM;
	n = vsnprintf(sb->str + sb->idx, sb->max - sb->idx, fmt, ap);
	if ((n >= 0) && ((sb->idx + n) < (sb->max - 1))) {
	    sb->idx += n;
	    rc = SQLITE_OK;
	    break;
	}
    }
    va_end(ap);
    return rc;
}

#define PATH_MODE_TK    ((void *) 0)
#define PATH_MODE_SVG   ((void *) 1)
#define PATH_MODE_BLT_X ((void *) 2)
#define PATH_MODE_BLT_Y ((void *) 3)
#define PATH_MODE_BLT   ((void *) 4)
#define PATH_MODE_TK3D  ((void *) 5)

/**
 * Make path/polyline from blob.
 * @param ctx SQLite function context
 * @param nargs number arguments
 * @param args arguments
 *
 * Arguments:
 *
 * args[0] - blob data (required)<br>
 * args[1] - type (optional, default "char")<br>
 * args[2] - X scale (optional)<br>
 * args[3] - X offset (optional)<br>
 * args[4] - Y scale (optional)<br>
 * args[5] - Y offset (optional)<br>
 * args[6] - Z value (optional)<br>
 * args[7] - Z scale (optional)<br>
 * args[8] - Z offset (optional)<br>
 */

static void
common_path_func(sqlite3_context *ctx, int nargs, sqlite3_value **args)
{
    void *mode = sqlite3_user_data(ctx);
    char *data;
    int i, linebreak, size, type = TYPE_CHAR;
    int do_x_scale = 0, do_y_scale = 0, do_z_scale = 0;
    double x_scale, x_offset, y_scale, y_offset, z0, z_scale, z_offset;
    strbuf sb;

    if (nargs < 1) {
	sqlite3_result_error(ctx, "need at least 1 argument", -1);
	return;
    }
    if (nargs > 1) {
	type = string_to_type((const char *) sqlite3_value_text(args[1]));
	if (!type) {
	    sqlite3_result_error(ctx, "bad type name", -1);
	    return;
	}
    }
    data = (char *) sqlite3_value_blob(args[0]);
    size = sqlite3_value_bytes(args[0]) / TYPE_SIZE(type);
    if (!data ||
	((mode != PATH_MODE_BLT_X) && (mode != PATH_MODE_BLT_Y) &&
	 (size < 2)) ||	(size < 1)) {
	goto nullorempty;
    }
    x_scale = 1;
    x_offset = 0;
    if (nargs > 2) {
	x_scale = sqlite3_value_double(args[2]);
	do_x_scale++;
    }
    if (nargs > 3) {
	x_offset = sqlite3_value_double(args[3]);
	do_x_scale++;
    }
    y_scale = 1;
    y_offset = 0;
    if (nargs > 4) {
	y_scale = sqlite3_value_double(args[4]);
	do_y_scale++;
    }
    if (nargs > 5) {
	y_offset = sqlite3_value_double(args[5]);
	do_y_scale++;
    }
    z0 = 0;
    z_scale = 1;
    z_offset = 0;
    if ((mode == PATH_MODE_TK3D) && (nargs > 6)) {
	z0 = sqlite3_value_double(args[6]);
    }
    if ((mode == PATH_MODE_TK3D) && (nargs > 7)) {
	z_scale = sqlite3_value_double(args[7]);
	do_z_scale++;
    }
    if ((mode == PATH_MODE_TK3D) && (nargs > 8)) {
	z_offset = sqlite3_value_double(args[8]);
	do_z_scale++;
    }
    memset(&sb, 0, sizeof (sb));
    if (init_strbuf(&sb) != SQLITE_OK) {
	goto nullorempty;
    }
    linebreak = 100;
    for (i = 0; i < size; i++, data += TYPE_SIZE(type)) {
	double x, y = 0, z = z0;
	char *fmt;

	if (do_z_scale) {
	    z = z0 * z_scale + z_offset;
	}
	if (do_x_scale) {
	    x = i * x_scale + x_offset;
	} else {
	    x = i;
	}
	switch (type) {
	case TYPE_CHAR:
	    y = data[0];
	    break;
	case TYPE_UCHAR:
	    y = data[0] & 0xFF;
	    break;
	case TYPE_SHORT_LE:
	    y = (data[0] & 0xFF) | (data[1] << 8);
	    break;
	case TYPE_USHORT_LE:
	    y = (data[0] & 0xFF) | ((data[1] & 0xFF) << 8);
	    break;
	case TYPE_SHORT_BE:
	    y = (data[1] & 0xFF) | (data[0] << 8);
	    break;
	case TYPE_USHORT_BE:
	    y = (data[1] & 0xFF) | ((data[0] & 0xFF) << 8);
	    break;
	case TYPE_INT_LE:
	    y =  (data[0] & 0xFF) | ((data[1] & 0xFF) << 8) |
		((data[2] & 0xFF) << 16) | (data[3] << 24);
	    break;
	case TYPE_UINT_LE:
	    y =  (data[0] & 0xFF) | ((data[1] & 0xFF) << 8) |
		((data[2] & 0xFF) << 16) | ((data[3] & 0xFF) << 24);
	    break;
	case TYPE_INT_BE:
	    y =  (data[3] & 0xFF) | ((data[2] & 0xFF) << 8) |
		((data[1] & 0xFF) << 16) | (data[0] << 24);
	    break;
	case TYPE_UINT_BE:
	    y =  (data[3] & 0xFF) | ((data[2] & 0xFF) << 8) |
		((data[1] & 0xFF) << 16) | ((data[0] & 0xFF) << 24);
	    break;
	case TYPE_FLOAT:
	    y = ((float *) data)[0];
	    break;
	case TYPE_DOUBLE:
	    y = ((double *) data)[0];
	    break;
	}
	if (do_y_scale) {
	    y = y * y_scale + y_offset;
	}
	if ((mode == PATH_MODE_BLT_X) || (mode == PATH_MODE_BLT_Y)) {
	    double v = (mode == PATH_MODE_BLT_X) ? x : y;

	    if (print_strbuf(&sb, (i == 0) ? "%g" : " %g", v) != SQLITE_OK) {
		drop_strbuf(&sb);
		break;
	    }
	    continue;
	}
	if ((mode == PATH_MODE_SVG) && (i == 0)) {
	    fmt = "M %g %g";
	} else if ((mode == PATH_MODE_SVG) && (i == 1)) {
	    fmt = " L %g %g";
	} else if ((mode == PATH_MODE_SVG) && (sb.idx >= linebreak)) {
	    fmt = "\nL %g %g";
	    linebreak = sb.idx + 100;
	} else if (i == 0) {
	    fmt = (mode == PATH_MODE_TK3D) ? "%g %g %g" : "%g %g";
	} else {
	    fmt = (mode == PATH_MODE_TK3D) ? " %g %g %g" : " %g %g";
	}
	if (print_strbuf(&sb, fmt, x, y, z) != SQLITE_OK) {
	    drop_strbuf(&sb);
	    break;
	}
    }
    if (sb.str) {
	sqlite3_result_text(ctx, sb.str, sb.idx, sqlite3_free);
	sb.str = 0;
	return;
    }
nullorempty:
    if ((mode == PATH_MODE_BLT_X) || (mode == PATH_MODE_BLT_Y)) {
	sqlite3_result_text(ctx, "", 0, SQLITE_STATIC);
    } else {
	sqlite3_result_null(ctx);
    }
}

/**
 * @typedef path_aggctx
 * @struct path_aggctx
 * Internal aggregate context for path/polyline function.
 */

typedef struct {
    int init;		/**< init flag, true when initialized */
    int count;		/**< counts formatted elements */
    int linebreak;	/**< when to add newline to output */
    void *mode;		/**< mode, see PATH_* defines */
    strbuf sb;		/**< string buffer for result */
} path_aggctx;

/**
 * Path/polyline step callback for "tk_path", "svg_path", and
 * "tk3d_path" aggregate functions.
 * @param ctx SQLite function context
 * @param nargs number arguments
 * @param args arguments
 *
 * Arguments:
 *
 * args[0] - X value (required)<br>
 * args[1] - Y value (required)<br>
 * args[2] - X scale (optional)<br>
 * args[3] - X offset (optional)<br>
 * args[4] - Y scale (optional)<br>
 * args[5] - Y offset (optional)<br>
 * args[6] - Z value (optional)<br>
 * args[7] - Z scale (optional)<br>
 * args[8] - Z offset (optional)<br>
 */

static void
common_path_step(sqlite3_context *ctx, int nargs, sqlite3_value **args)
{
    path_aggctx *pag;
    int type;
    char *fmt;
    double x, y, z = 0;
    double x_scale, y_scale, x_offset, y_offset, z_scale, z_offset;

    if (nargs < 2) {
	return;
    }
    pag = sqlite3_aggregate_context(ctx, sizeof (*pag));
    if (!pag->init) {
	if (init_strbuf(&pag->sb) != SQLITE_OK) {
	    return;
	}
	pag->linebreak = 100;
	pag->count = 0;
	pag->mode = sqlite3_user_data(ctx);
	pag->init = 1;
    }
    type = sqlite3_value_type(args[0]);
    if ((type != SQLITE_INTEGER) && (type != SQLITE_FLOAT)) {
	return;
    }
    type = sqlite3_value_type(args[1]);
    if ((type != SQLITE_INTEGER) && (type != SQLITE_FLOAT)) {
	return;
    }
    x = sqlite3_value_double(args[0]);
    y = sqlite3_value_double(args[1]);
    x_scale = 1;
    x_offset = 0;
    if (nargs > 2) {
	type = sqlite3_value_type(args[2]);
	if ((type == SQLITE_INTEGER) || (type == SQLITE_FLOAT)) {
	    x_scale = sqlite3_value_double(args[2]);
	}
    }
    if (nargs > 3) {
	type = sqlite3_value_type(args[3]);
	if ((type == SQLITE_INTEGER) || (type == SQLITE_FLOAT)) {
	    x_offset = sqlite3_value_double(args[3]);
	}
    }
    y_scale = 1;
    y_offset = 0;
    if (nargs > 4) {
	type = sqlite3_value_type(args[4]);
	if ((type == SQLITE_INTEGER) || (type == SQLITE_FLOAT)) {
	    y_scale = sqlite3_value_double(args[4]);
	}
    }
    if (nargs > 5) {
	type = sqlite3_value_type(args[5]);
	if ((type == SQLITE_INTEGER) || (type == SQLITE_FLOAT)) {
	    y_offset = sqlite3_value_double(args[5]);
	}
    }
    z_scale = 1;
    z_offset = 0;
    if ((pag->mode == PATH_MODE_TK3D) && (nargs > 6)) {
	z = sqlite3_value_double(args[6]);
	if (nargs > 7) {
	    type = sqlite3_value_type(args[7]);
	    if ((type == SQLITE_INTEGER) || (type == SQLITE_FLOAT)) {
		z_scale = sqlite3_value_double(args[7]);
	    }
	}
	if (nargs > 8) {
	    type = sqlite3_value_type(args[8]);
	    if ((type == SQLITE_INTEGER) || (type == SQLITE_FLOAT)) {
		z_offset = sqlite3_value_double(args[8]);
	    }
	}
	z = z * z_scale + z_offset;
    }
    x = x * x_scale + x_offset;
    y = y * y_scale + y_offset;
    if ((pag->mode == PATH_MODE_SVG) && (pag->count == 0)) {
	fmt = "M %g %g";
    } else if ((pag->mode == PATH_MODE_SVG) && (pag->count == 1)) {
	fmt = " L %g %g";
    } else if ((pag->mode == PATH_MODE_SVG) &&
	       (pag->sb.idx >= pag->linebreak)) {
	fmt = "\nL %g %g";
	pag->linebreak = pag->sb.idx + 100;
    } else if (pag->count == 0) {
	fmt = (pag->mode == PATH_MODE_TK3D) ? "%g %g %g" : "%g %g";
    } else {
	fmt = (pag->mode == PATH_MODE_TK3D) ? " %g %g %g" : " %g %g";
    }
    if (print_strbuf(&pag->sb, fmt, x, y, z) != SQLITE_OK) {
	drop_strbuf(&pag->sb);
	pag->init = 0;
    } else {
	pag->count++;
    }
}

/**
 * Path/polyline finalizer.
 * @param ctx SQLite function context
 */

static void
common_path_finalize(sqlite3_context *ctx)
{
    path_aggctx *pag = sqlite3_aggregate_context(ctx, sizeof (*pag));

    if (pag->init) {
	if ((pag->count > 1) || (pag->mode == PATH_MODE_BLT)) {
	    sqlite3_result_text(ctx, pag->sb.str, pag->sb.idx, sqlite3_free);
	    pag->sb.str = 0;
	    pag->init = 0;
	    return;
	}
	drop_strbuf(&pag->sb);
    }
    if (pag->mode == PATH_MODE_BLT) {
	sqlite3_result_text(ctx, "", 0, SQLITE_STATIC);
    } else {
	sqlite3_result_null(ctx);
    }
}

/**
 * Path/polyline step callback for "blt_vec" aggregate functions.
 * @param ctx SQLite function context
 * @param nargs number arguments
 * @param args arguments
 *
 * Arguments:
 *
 * args[0] - value (required)<br>
 * args[1] - scale (optional)<br>
 * args[2] - offset (optional)<br>
 */

static void
blt_vec_step(sqlite3_context *ctx, int nargs, sqlite3_value **args)
{
    path_aggctx *pag;
    int type;
    double v, scale, offset;

    if (nargs < 1) {
	return;
    }
    pag = sqlite3_aggregate_context(ctx, sizeof (*pag));
    if (!pag->init) {
	if (init_strbuf(&pag->sb) != SQLITE_OK) {
	    return;
	}
	pag->count = 0;
	pag->mode = PATH_MODE_BLT;
	pag->init = 1;
    }
    type = sqlite3_value_type(args[0]);
    if ((type != SQLITE_INTEGER) && (type != SQLITE_FLOAT)) {
	return;
    }
    v = sqlite3_value_double(args[0]);
    scale = 1;
    offset = 0;
    if (nargs > 1) {
	type = sqlite3_value_type(args[1]);
	if ((type == SQLITE_INTEGER) || (type == SQLITE_FLOAT)) {
	    scale = sqlite3_value_double(args[2]);
	}
    }
    if (nargs > 2) {
	type = sqlite3_value_type(args[2]);
	if ((type == SQLITE_INTEGER) || (type == SQLITE_FLOAT)) {
	    offset = sqlite3_value_double(args[3]);
	}
    }
    v = v * scale + offset;
    if (print_strbuf(&pag->sb, (pag->count == 0) ? "%g" : " %g", v)
	!= SQLITE_OK) {
	drop_strbuf(&pag->sb);
	pag->init = 0;
    } else {
	pag->count++;
    }
}

/**
 * "subblob" function similar to "substr".
 * @param ctx SQLite function context
 * @param nargs number arguments
 * @param args arguments
 */

static void
subblob_func(sqlite3_context *ctx, int nargs, sqlite3_value **args)
{
    int insize, outsize, start, itemsize = 1, itemskip = 0;
    int i, k, n;
    char *indata, *outdata;

    if (nargs < 3) {
	sqlite3_result_error(ctx, "need at least 1 argument", -1);
	return;
    }
    indata = (char *) sqlite3_value_blob(args[0]);
    insize = sqlite3_value_bytes(args[0]);
    if (!indata || (insize <= 0)) {
isnull:
	sqlite3_result_null(ctx);
	return;
    }
    start = sqlite3_value_int(args[1]);
    if (start < 0) {
	start = insize - start;
	if (start < 0) {
	    start = 0;
	}
    } else if (start > 0) {
	start--;
    }
    if (start >= insize) {
	goto isnull;
    }
    outsize = sqlite3_value_int(args[2]);
    if (outsize > insize - start) {
	outsize = insize - start;
    }
    if (outsize <= 0) {
	goto isnull;
    }
    if (nargs > 3) {
	itemsize = sqlite3_value_int(args[3]);
	if ((itemsize <= 0) || (itemsize > outsize)) {
	    goto isnull;
	}
    }
    if (nargs > 4) {
	itemskip = sqlite3_value_int(args[4]);
	if (itemskip < 0) {
	    goto isnull;
	}
    }
    outdata = sqlite3_malloc(outsize);
    if (!outdata) {
	sqlite3_result_error(ctx, "out of memory", -1);
	return;
    }
    for (i = n = 0; i < outsize; i++) {
	for (k = 0; k < itemsize; k++) {
	    outdata[i + k] = indata[start];
	    n++;
	    start++;
	    if (start >= insize) {
		break;
	    }
	}
	start += itemskip;
	if (start >= insize) {
	    break;
	}
    }
    if (n > 0) {
	sqlite3_result_blob(ctx, outdata, n, sqlite3_free);
	return;
    }
    sqlite3_result_null(ctx);
    sqlite3_free(outdata);
}

/**
 * @typedef rownumber_ctx
 * @struct rownumber_ctx
 * SQLite context structure for "rownumber" function.
 */

typedef struct {
    sqlite3_context *ctx;	/**< SQLite context */
    sqlite3_value *value;	/**< SQLite value for this context */
    sqlite_int64 count;		/**< Counter giving row number */
} rownumber_ctx;

/**
 * "rownumber" function.
 * @param ctx SQLite function context
 * @param nargs number arguments
 * @param args arguments
 */

static void
rownumber_func(sqlite3_context *ctx, int nargs, sqlite3_value **args)
{
    rownumber_ctx *rn = sqlite3_get_auxdata(ctx, 0);

    if (!rn || (rn->ctx != ctx) || (rn->value != args[0])) {
	rn = sqlite3_malloc(sizeof (*rn));
	if (rn) {
	    rn->ctx = ctx;
	    rn->value = args[0];
	    rn->count = 0;
	}
	sqlite3_set_auxdata(ctx, 0, rn, sqlite3_free);
    } else {
	rn->count++;
    }
    sqlite3_result_int64(ctx, rn ? rn->count : 0);
}

/**
 * Module initializer creating SQLite functions and
 * modules.
 * @param db SQLite database pointer
 * @result SQLite error code
 */

#ifndef STANDALONE
static
#endif
int
b2xy_init(sqlite3 *db)
{
    sqlite3_create_function(db, "subblob", -1, SQLITE_ANY, (void *) 0,
			    subblob_func, 0, 0);
    sqlite3_create_function(db, "tk_path_from_blob", -1, SQLITE_UTF8,
			    PATH_MODE_TK, common_path_func, 0, 0);
    sqlite3_create_function(db, "svg_path_from_blob", -1, SQLITE_UTF8,
			    PATH_MODE_SVG, common_path_func, 0, 0);
    sqlite3_create_function(db, "blt_vec_x", -1, SQLITE_UTF8,
			    PATH_MODE_BLT_X, common_path_func, 0, 0);
    sqlite3_create_function(db, "blt_vec_y", -1, SQLITE_UTF8,
			    PATH_MODE_BLT_Y, common_path_func, 0, 0);
    sqlite3_create_function(db, "tk3d_path_from_blob", -1, SQLITE_UTF8,
			    PATH_MODE_TK3D, common_path_func, 0, 0);
    sqlite3_create_function(db, "tk_path", -1, SQLITE_ANY, PATH_MODE_TK,
			    0, common_path_step, common_path_finalize);
    sqlite3_create_function(db, "svg_path", -1, SQLITE_ANY, PATH_MODE_SVG,
			    0, common_path_step, common_path_finalize);
    sqlite3_create_function(db, "blt_vec", -1, SQLITE_ANY, PATH_MODE_BLT,
			    0, blt_vec_step, common_path_finalize);
    sqlite3_create_function(db, "tk3d_path", -1, SQLITE_ANY, PATH_MODE_TK3D,
			    0, common_path_step, common_path_finalize);
    sqlite3_create_function(db, "rownumber", 1, SQLITE_ANY, 0,
			    rownumber_func, 0, 0);
    return sqlite3_create_module(db, "blobtoxy", &b2xy_module, 0);
}

#ifndef STANDALONE

/**
 * Initializer for SQLite extension load mechanism.
 * @param db SQLite database pointer
 * @param errmsg pointer receiving error message
 * @param api SQLite API routines
 * @result SQLite error code
 */

int
sqlite3_extension_init(sqlite3 *db, char **errmsg, 
		       const sqlite3_api_routines *api)
{
    SQLITE_EXTENSION_INIT2(api);
    return b2xy_init(db);
}

#endif
