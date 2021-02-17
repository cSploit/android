/**
 * @file csvtable.c
 * SQLite extension module for mapping a CSV file as
 * a read-only SQLite virtual table plus extension
 * function to import a CSV file as a real table.
 *
 * 2012 July 27
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

#ifdef STANDALONE
#include <sqlite3.h>
#else
#include <sqlite3ext.h>
static SQLITE_EXTENSION_INIT1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#endif

/**
 * @typedef csv_file
 * @struct csv_file
 * Structure to implement CSV file handle.
 */

typedef struct csv_file {
    FILE *f;		/**< CSV file */
    char *sep;		/**< column separator characters */
    char *quot;		/**< text quoting characters */
    int isdos;		/**< true, when DOS format detected */
    int maxl;		/**< max. capacity of line buffer */
    char *line;		/**< line buffer */
    long pos0;		/**< file position for rewind */
    int maxc;		/**< max. capacity of column buffer */
    int ncols;		/**< number of columns */
    char **cols;	/**< column buffer */
} csv_file;

/**
 * @typedef csv_guess_fmt
 * @struct csv_guess_fmt
 * Info to guess CSV layout.
 */

typedef struct csv_guess_fmt {
    int nlines;
    int hist[256];
} csv_guess_fmt;

/**
 * @typedef csv_vtab
 * @struct csv_vtab
 * Structure to describe a CSV virtual table.
 */

typedef struct csv_vtab {
    sqlite3_vtab vtab;	/**< SQLite virtual table */
    csv_file *csv;	/**< CSV file handle */
    int convert;	/**< convert flags */
    char coltypes[1];	/**< column types */
} csv_vtab;

/**
 * @typedef csv_cursor
 * @struct csv_cursor
 * Structure to describe CSV virtual table cursor.
 */

typedef struct {
    sqlite3_vtab_cursor cursor;		/**< SQLite virtual table cursor */
    long pos;				/**< CSV file position */
} csv_cursor;

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
 * Strip off quotes given string.
 * @param in string to be processed
 * @result new string to be free'd with sqlite3_free()
 */

static char *
unquote(char const *in)
{
    char c, *ret;
    int i;

    ret = sqlite3_malloc(strlen(in) + 1);
    if (ret) {
	c = in[0];
	if ((c == '"') || (c == '\'')) {
	    i = strlen(in + 1);
	    if ((i > 0) && (in[i] == c)) {
		strcpy(ret, in + 1);
		ret[i - 1] = '\0';
		return ret;
	    }
	}
	strcpy(ret, in);
    }
    return ret;
}

/**
 * Map string to SQLite data type.
 * @param type string to be mapped
 * @result SQLITE_TEXT et.al.
 */

static int
maptype(char const *type)
{
    int typelen = type ? strlen(type) : 0;

    if ((typelen >= 3) &&
	(strncasecmp(type, "integer", 7) == 0)) {
	return SQLITE_INTEGER;
    }
    if ((typelen >= 6) &&
	(strncasecmp(type, "double", 6) == 0)) {
	return SQLITE_FLOAT;
    }
    if ((typelen >= 5) &&
	(strncasecmp(type, "float", 5) == 0)) {
	return SQLITE_FLOAT;
    }
    if ((typelen >= 4) &&
	(strncasecmp(type, "real", 4) == 0)) {
	return SQLITE_FLOAT;
    }
    return SQLITE_TEXT;
}

/**
 * Convert and collapse white space in column names to underscore
 * @param names string vector of column names
 * @param ncols number of columns
 */

static void
conv_names(char **names, int ncols)
{
    int i;
    char *p, *q;
    static const char ws[] = "\n\t\r\b\v ";

    if (!names || ncols <= 0) {
	return;
    }
    for (i = 0; i < ncols; i++) {
	p = names[i];

	while (*p) {
	    if (strchr(ws, *p)) {
		*p++ = '_';
		q = p;
		while (*q && strchr(ws, *q)) {
		    ++q;
		}
		if (*q && q > p) {
		    strcpy(p, q);
		}
		continue;
	    }
	    ++p;
	}
    }
}

/**
 * Make result data or parameter binding accoring to type
 * @param ctx SQLite function context or NULL
 * @param stmt SQLite statement or NULL
 * @param idx parameter number, 1-based
 * @param data string data
 * @param len string length
 * @param type SQLite type
 */

static void
result_or_bind(sqlite3_context *ctx, sqlite3_stmt *stmt, int idx,
	       char *data, int len, int type)
{
    char *endp;

    if (!data) {
	if (ctx) {
	    sqlite3_result_null(ctx);
	} else {
	    sqlite3_bind_null(stmt, idx);
	}
	return;
    }
    if (type == SQLITE_INTEGER) {
	sqlite_int64 val;
#if defined(_WIN32) || defined(_WIN64)
	char endc;

	if (sscanf(data, "%I64d%c", &val, &endc) == 1) {
	    if (ctx) {
		sqlite3_result_int64(ctx, val);
	    } else {
		sqlite3_bind_int64(stmt, idx, val);
	    }
	    return;
	}
#else
	endp = 0;
#ifdef __osf__
	val = strtol(data, &endp, 0);
#else
	val = strtoll(data, &endp, 0);
#endif
	if (endp && (endp != data) && !*endp) {
	    if (ctx) {
		sqlite3_result_int64(ctx, val);
	    } else {
		sqlite3_bind_int64(stmt, idx, val);
	    }
	    return;
	}
#endif
    } else if (type == SQLITE_FLOAT) {
	double val;

	endp = 0;
	val = strtod(data, &endp);
	if (endp && (endp != data) && !*endp) {
	    if (ctx) {
		sqlite3_result_double(ctx, val);
	    } else {
		sqlite3_bind_double(stmt, idx, val);
	    }
	    return;
	}
    }
    if (ctx) {
	sqlite3_result_text(ctx, data, len, SQLITE_TRANSIENT);
    } else {
	sqlite3_bind_text(stmt, idx, data, len, SQLITE_TRANSIENT);
    }
}

/**
 * Process one column of the current row
 * @param ctx SQLite function context or NULL
 * @param stmt SQLite statement or NULL
 * @param idx parameter index, 1-based
 * @param data string data
 * @param type SQLite type
 * @param conv conversion flags
 */

static int
process_col(sqlite3_context *ctx, sqlite3_stmt *stmt, int idx,
	    char *data, int type, int conv)
{
    char c, *p;
    const char flchars[] = "Ee+-.,0123456789";

    if (!data) {
	goto putdata;
    }

    /*
     * Floating point number test,
     * converts single comma to dot.
     */
    c = data[0];
    if ((c != '\0') && strchr(flchars + 2, c)) {
	p = data + 1;
	while (*p && strchr(flchars, *p)) {
	    ++p;
	}
	if (*p == '\0') {
	    char *first = 0;
	    int n = 0;

	    p = data;
	    while (p) {
		p = strchr(p, ',');
		if (!p) {
		    break;
		}
		if (++n == 1) {
		    first = p;
		}
		++p;
	    }
	    if (first) {
		*first = '.';
		goto putdata;
	    }
	}
    }
    if (conv) {
	char *utf = sqlite3_malloc(strlen(data) * 2 + 2);

	if (utf) {
	    p = utf;
	    while ((c = *data) != '\0') {
		if (((conv & 10) == 10) && (c == '\\')) {
		    if (data[1] == 'q') {
			*p++ = '\'';
			data += 2;
			continue;
		    }
		}
		if ((conv & 2) && (c == '\\')) {
		    char c2 = data[1];

		    switch (c2) {
		    case '\0':
			goto convdone;
		    case 'n':
			*p = '\n';
			break;
		    case 't':
			*p = '\t';
			break;
		    case 'r':
			*p = '\r';
			break;
		    case 'f':
			*p = '\f';
			break;
		    case 'v':
			*p = '\v';
			break;
		    case 'b':
			*p = '\b';
			break;
		    case 'a':
			*p = '\a';
			break;
		    case '?':
			*p = '\?';
			break;
		    case '\'':
			*p = '\'';
			break;
		    case '"':
			*p = '\"';
			break;
		    case '\\':
			*p = '\\';
			break;
		    default:
			*p++ = c;
			*p = c2;
			break;
		    }
		    p++;
		    data += 2;
		    continue;
		}
		if ((conv & 1) && (c & 0x80)) {
		    *p++ = 0xc0 | ((c >> 6) & 0x1f);
		    *p++ = 0x80 | (c & 0x3f);
		} else {
		    *p++ = c;
		}
		data++;
	    }
convdone:
	    *p = '\0';
	    result_or_bind(ctx, stmt, idx, utf, p - utf, type);
	    sqlite3_free(utf);
	    return SQLITE_OK;
	} else {
	    if (ctx) {
		sqlite3_result_error(ctx, "out of memory", -1);
	    }
	    return SQLITE_NOMEM;
	}
    }
putdata:
    result_or_bind(ctx, stmt, idx, data, -1, type);
    return SQLITE_OK;
}

/**
 * Open CSV file for reading and return handle to it.
 * @param filename name of CSV file
 * @param sep column separator characters or NULL
 * @param quot string quote characters or NULL
 * @result CSV file handle
 */

static csv_file *
csv_open(const char *filename, const char *sep, const char *quot)
{
    FILE *f;
    csv_file *csv;

#ifdef _WIN32
    f = fopen(filename, "rb");
#else
    f = fopen(filename, "r");
#endif
    if (!f) {
	return 0;
    }
    csv = sqlite3_malloc(sizeof (csv_file));
    if (!csv) {
error0:
	fclose(f);
	return 0;
    }
    csv->f = f;
    if (sep && sep[0]) {
	csv->sep = sqlite3_malloc(strlen(sep) + 1);
	if (!csv->sep) {
error1:	    
	    sqlite3_free(csv);
	    goto error0;
	}
	strcpy(csv->sep, sep);
    } else {
	csv->sep = 0;
    }
    if (quot && quot[0]) {
	csv->quot = sqlite3_malloc(strlen(quot) + 1);
	if (!csv->quot) {
	    if (csv->sep) {
		sqlite3_free(csv->sep);
	    }
	    goto error1;
	}
	strcpy(csv->quot, quot);
    } else {
	csv->quot = 0;
    }
    csv->isdos = 0;
    csv->maxl = 0;
    csv->line = 0;
    csv->pos0 = 0;
    csv->maxc = 0;
    csv->ncols = 0;
    csv->cols = 0;
    return csv;
}

/**
 * Close CSV file handle.
 * @param csv CSV file handle
 */

static void
csv_close(csv_file *csv)
{
    if (csv) {
	if (csv->sep) {
	    sqlite3_free(csv->sep);
	}
	if (csv->quot) {
	    sqlite3_free(csv->quot);
	}
	if (csv->line) {
	    sqlite3_free(csv->line);
	}
	if (csv->cols) {
	    sqlite3_free(csv->cols);
	}
	if (csv->f) {
	    fclose(csv->f);
	}
	sqlite3_free(csv);
    }
}

/**
 * Test EOF on CSV file handle.
 * @param csv CSV file handle
 * @result true when file position is at EOF
 */

static int
csv_eof(csv_file *csv)
{
    if (csv && csv->f) {
	return feof(csv->f);
    }
    return 1;
}

/**
 * Position CSV file handle.
 * @param csv CSV file handle
 * @param pos position to seek
 * @result 0 on success, EOF on error
 */

static long
csv_seek(csv_file *csv, long pos)
{
    if (csv && csv->f) {
	return fseek(csv->f, pos, SEEK_SET);
    }
    return EOF;
}

/**
 * Rewind CSV file handle.
 * @param csv CSV file handle
 */

static void
csv_rewind(csv_file *csv)
{
    if (csv && csv->f) {
	csv_seek(csv, csv->pos0);
    }
}

/**
 * Return current position of CSV file handle.
 * @param csv CSV file handle
 * @result current file position
 */

static long
csv_tell(csv_file *csv)
{
    if (csv && csv->f) {
	return ftell(csv->f);
    }
    return EOF;
}

/**
 * Read and process one line of CSV file handle.
 * @param csv CSV file handle
 * @param guess NULL or buffer for guessing file format
 * @result number of columns on success, EOF on error
 */

static int
csv_getline(csv_file *csv, csv_guess_fmt *guess)
{
    int i, index = 0, inq = 0, c, col;
    char *p, *sep;

    if (!csv || !csv->f) {
	return EOF;
    }
    while (1) {
	c = fgetc(csv->f);
	if (c == EOF) {
	    if (index > 0) {
		break;
	    }
	    return EOF;
	}
	if (c == '\0') {
	    continue;
	}
	if (c == '\r') {
	    int c2 = fgetc(csv->f);
	    c = '\n';

	    if (c2 == '\n') {
		csv->isdos = 1;
	    } else if (c2 != EOF) {
		ungetc(c2, csv->f);
	    }
	}
	/* check for DOS EOF (Ctrl-Z) */
	if (csv->isdos && (c == '\032')) {
	    int c2 = fgetc(csv->f);

	    if (c2 == EOF) {
		if (index > 0) {
		    break;
		}
		return EOF;
	    }
	    ungetc(c2, csv->f);
	}
	if (index >= csv->maxl - 1) {
	    int n = csv->maxl * 2;
	    char *line;

	    if (n <= 0) {
		n = 4096;
	    }
	    line = sqlite3_malloc(n);
	    if (!line) {
		return EOF;
	    }
	    if (csv->line) {
		memcpy(line, csv->line, index);
		sqlite3_free(csv->line);
	    }
	    csv->maxl = n;
	    csv->line = line;
	}
	csv->line[index++] = c;
	if (csv->quot && (p = strchr(csv->quot, c))) {
	    if (inq) {
		if (*p == inq) {
		    inq = 0;
		}
	    } else {
		inq = *p;
	    }
	}
	if (!inq && (c == '\n')) {
	    break;
	}
    }
    if (guess) {
	for (i = 0; i < index; i++) {
	    guess->hist[csv->line[i] & 0xFF] += 1;
	}
	guess->nlines += 1;
	csv->ncols = 0;
	return 0;
    }

    for (i = index - 1; i >= 0; i--) {
	if (csv->line[i] != '\n') {
	    break;
	}
    }
    index = i + 1;
    csv->line[index] = '\0';
    i = inq = col = 0;
    sep = csv->sep ? csv->sep : ";";
    if (!csv->cols) {
	int n = 128;

	csv->cols = sqlite3_malloc(sizeof (char *) * n);
	if (!csv->cols) {
	    return EOF;
	}
	csv->maxc = n;
    }
    csv->cols[col++] = csv->line;
    while (i < index) {
	if (csv->quot && (p = strchr(csv->quot, csv->line[i]))) {
	    if (inq) {
		if (*p == inq) {
		    inq = 0;
		}
	    } else {
		inq = *p;
	    }
	}
	if (!inq && (p = strchr(sep, csv->line[i]))) {
	    p = csv->line + i;
	    *p = '\0';
	    if (col >= csv->maxc) {
		int n = csv->maxc * 2;
		char **cols;

		cols = sqlite3_realloc(csv->cols, sizeof (char *) * n);
		if (!cols) {
		    return EOF;
		}
		csv->cols = cols;
		csv->maxc = n;
	    }
	    csv->cols[col++] = p + 1;
	}
	++i;
    }
    csv->ncols = col;

    /* strip off quotes */
    if (csv->quot) {
	for (i = 0; i < col; i++) {
	    if (*csv->cols[i]) {
		p = strchr(csv->quot, *csv->cols[i]);
		if (p) {
		    char *src, *dst;

		    c = *p;
		    csv->cols[i] += 1;
		    sep = csv->cols[i] + strlen(csv->cols[i]) - 1;
		    if ((sep >= csv->cols[i]) && (*sep == c)) {
			*sep = '\0';
		    }
		    /* collapse quote escape sequences */
		    src = csv->cols[i];
		    dst = 0;
		    while (*src) {
			if ((*src == c) && (src[1] == c)) {
			    if (!dst) {
				dst = src;
			    }
			    src++;
			    while (*src) {
				*dst++ = *src++;
				if (*src == c) {
				    --src;
				    break;
				}
			    }
			}
			++src;
		    }
		    if (dst) {
			*dst++ = '\0';
		    }
		}
	    }
	}
    }
    return col;
}

/**
 * Return number of columns of current row in CSV file.
 * @param csv CSV file handle
 * @result number of columns of current row
 */

static int
csv_ncols(csv_file *csv)
{
    if (csv && csv->cols) {
	return csv->ncols;
    }
    return 0;
}

/**
 * Return nth column of current row in CSV file.
 * @param csv CSV file handle
 * @param n column number
 * @result string pointer or NULL
 */

static char *
csv_coldata(csv_file *csv, int n)
{
    if (csv && csv->cols && (n >= 0) && (n < csv->ncols)) {
	return csv->cols[n];
    }
    return 0;
}

/**
 * Guess CSV layout of CSV file handle.
 * @param csv CSV file handle
 * @result 0 on succes, EOF on error
 */

static int
csv_guess(csv_file *csv)
{
    csv_guess_fmt guess;
    int i, n;
    char *p, sep[32], quot[4];
    const struct {
	int c;
	int min;
    } sep_test[] = {
	{ ',', 2 },
	{ ';', 2 },
	{ '\t', 2 },
	{ ' ', 4 },
	{ '|', 2 }
    };

    if (!csv) {
	return EOF;
    }
    memset(&guess, 0, sizeof (guess));
    csv->pos0 = 0;
    csv_rewind(csv);
    for (i = n = 0; i < 10; i++) {
	n = csv_getline(csv, &guess);
	if (n == EOF) {
	    break;
	}
    }
    csv_rewind(csv);
    if (n && !i) {
	return EOF;
    }
    p = quot;
    n = '"';
    if (guess.hist[n] > 1) {
	*p++ = n;
    }
    n = '\'';
    if (guess.hist[n] > 1) {
	*p++ = n;
    }
    *p = '\0';
    p = sep;
    for (i = 0; i < sizeof (sep_test) / sizeof (sep_test[0]); i++) {
	if (guess.hist[sep_test[i].c] > sep_test[i].min * guess.nlines) {
	    *p++ = sep_test[i].c;
	}
    }
    *p = '\0';
    if (quot[0]) {
	p = sqlite3_malloc(strlen(quot) + 1);
	if (p) {
	    strcpy(p, quot);
	    if (csv->quot) {
		sqlite3_free(csv->quot);
	    }
	    csv->quot = p;
	} else {
	    return EOF;
	}
    }
    if (sep[0]) {
	p = sqlite3_malloc(strlen(sep) + 1);
	if (p) {
	    strcpy(p, sep);
	    if (csv->sep) {
		sqlite3_free(csv->sep);
	    }
	    csv->sep = p;
	} else {
	    return EOF;
	}
    }
    return 0;
}

/**
 * Connect to virtual table
 * @param db SQLite database pointer
 * @param aux user specific pointer (unused)
 * @param argc argument count
 * @param argv argument vector
 * @param vtabp pointer receiving virtual table pointer
 * @param errp pointer receiving error messag
 * @result SQLite error code
 *
 * Argument vector contains:
 *
 * argv[0] - module name<br>
 * argv[1] - database name<br>
 * argv[2] - table name (virtual table)<br>
 * argv[3] - filename (required)<br>
 * argv[4] - number, when non-zero use first line as column names,
 *           when negative use given type names (optional)<br>
 * argv[5] - number, when non-zero, translate data (optional, see below)<br>
 * argv[6] - column separator characters (optional)<br>
 * argv[7] - string quoting characters (optional)<br>
 * argv[8] - column/type name for first column (optional)<br>
 *   ..<br>
 * argv[X] - column/type name for last column (optional)<br><br>
 *
 * Translation flags:
 *
 * 1  - convert ISO-8859-1 to UTF-8<br>
 * 2  - perform backslash substitution<br>
 * 4  - convert and collapse white-space in column names to underscore<br>
 * 10 - convert \q to single quote, in addition to backslash substitution<br>
 */

static int
csv_vtab_connect(sqlite3* db, void *aux, int argc, const char * const *argv,
		 sqlite3_vtab **vtabp, char **errp)
{
    csv_file *csv;
    int rc = SQLITE_ERROR, i, ncnames, row1;
    char **cnames, *schema = 0, **nargv;
    csv_vtab *vtab;

    if (argc < 4) {
	*errp = sqlite3_mprintf("input file name missing");
	return SQLITE_ERROR;
    }
    nargv = sqlite3_malloc(sizeof (char *) * argc);
    memset(nargv, 0, sizeof (char *) * argc);
    for (i = 3; i < argc; i++) {
	nargv[i] = unquote(argv[i]);
    }
    csv = csv_open(nargv[3], (argc > 6) ? nargv[6] : 0,
		   (argc > 7) ? nargv[7] : 0);
    if (!csv) {
	*errp = sqlite3_mprintf("unable to open input file");
cleanup:
	append_free(&schema);
	for (i = 3; i < argc; i++) {
	    if (nargv[i]) {
		sqlite3_free(nargv[i]);
	    }
	}
	return rc;
    }
    if (!csv->sep && !csv->quot) {
	csv_guess(csv);
    }
    csv->pos0 = 0;
    row1 = 0;
    if (argc > 4) {
	row1 = strtol(nargv[4], 0, 10);
    }
    if (row1) {
	/* use column names from 1st row */
	csv_getline(csv, 0);
	if (csv->ncols < 1) {
	    csv_close(csv);
	    *errp = sqlite3_mprintf("unable to get column names");
	    goto cleanup;
	}
	csv->pos0 = csv_tell(csv);
	csv_rewind(csv);
	ncnames = csv_ncols(csv);
	cnames = csv->cols;
    } else if (argc > 8) {
	ncnames = argc - 8;
	cnames = (char **) nargv + 8;
    } else {
	/* use number of columns from 1st row */
	csv_getline(csv, 0);
	if (csv->ncols < 1) {
	    csv_close(csv);
	    *errp = sqlite3_mprintf("unable to get column names");
	    goto cleanup;
	}
	csv_rewind(csv);
	ncnames = csv_ncols(csv);
	cnames = 0;
    }
    vtab = sqlite3_malloc(sizeof(csv_vtab) + ncnames);
    if (!vtab) {
	csv_close(csv);
	*errp = sqlite3_mprintf("out of memory");
	goto cleanup;
    }
    memset(vtab, 0, sizeof (*vtab));
    vtab->convert = 0;
    if (argc > 5) {
	vtab->convert = strtol(nargv[5], 0, 10);
	if (row1 && (vtab->convert & 4)) {
	    conv_names(cnames, ncnames);
	}
    }
    vtab->csv = csv;
    append(&schema, "CREATE TABLE x(", 0);
    for (i = 0; i < ncnames; i++) {
	vtab->coltypes[i] = SQLITE_TEXT;
	if (!cnames || !cnames[i]) {
	    char colname[64];

	    sprintf(colname, "column_%d", i + 1);
	    append(&schema, colname, '"');
	} else if (row1 > 0) {
	    append(&schema, cnames[i], '"');
	} else if (row1 < 0) {
	    append(&schema, cnames[i], '"');
	    if (i + 8 < argc) {
		char *type = nargv[i + 8];

		append(&schema, " ", 0);
		append(&schema, type, 0);
		vtab->coltypes[i] = maptype(type);
	    }
	} else {
	    char *type = cnames[i];

	    append(&schema, cnames[i], 0);
	    while (*type && !strchr(" \t", *type)) {
		type++;
	    }
	    while (*type && strchr(" \t", *type)) {
		type++;
	    }
	    vtab->coltypes[i] = maptype(type);
	}
	if (i < ncnames - 1) {
	    append(&schema, ",", 0);
	}
    }
    append(&schema, ")", 0);
    rc = sqlite3_declare_vtab(db, schema);
    if (rc != SQLITE_OK) {
	csv_close(csv);
	sqlite3_free(vtab);
	*errp = sqlite3_mprintf("table definition failed, error %d, "
				"schema '%s'", rc, schema);
	goto cleanup;
    }
    *vtabp = &vtab->vtab;
    *errp = 0;
    goto cleanup;
}

/**
 * Create virtual table
 * @param db SQLite database pointer
 * @param aux user specific pointer (unused)
 * @param argc argument count
 * @param argv argument vector
 * @param vtabp pointer receiving virtual table pointer
 * @param errp pointer receiving error messag
 * @result SQLite error code
 */

static int
csv_vtab_create(sqlite3* db, void *aux, int argc,
	   const char *const *argv,
	   sqlite3_vtab **vtabp, char **errp)
{
    return csv_vtab_connect(db, aux, argc, argv, vtabp, errp);
}

/**
 * Disconnect virtual table.
 * @param vtab virtual table pointer
 * @result always SQLITE_OK
 */

static int
csv_vtab_disconnect(sqlite3_vtab *vtab)
{
    csv_vtab *tab = (csv_vtab *) vtab;

    csv_close(tab->csv);
    sqlite3_free(tab);
    return SQLITE_OK;
}

/**
 * Destroy virtual table.
 * @param vtab virtual table pointer
 * @result always SQLITE_OK
 */

static int
csv_vtab_destroy(sqlite3_vtab *vtab)
{
    return csv_vtab_disconnect(vtab);
}

/**
 * Determines information for filter function according to constraints.
 * @param vtab virtual table
 * @param info index/constraint iinformation
 * @result SQLite error code
 */

static int
csv_vtab_bestindex(sqlite3_vtab *vtab, sqlite3_index_info *info)
{
    return SQLITE_OK;
}

/**
 * Open virtual table and return cursor.
 * @param vtab virtual table pointer
 * @param cursorp pointer receiving cursor pointer
 * @result SQLite error code
 */

static int
csv_vtab_open(sqlite3_vtab *vtab, sqlite3_vtab_cursor **cursorp)
{
    csv_cursor *cur = sqlite3_malloc(sizeof(*cur));
    csv_vtab *tab = (csv_vtab *) vtab;

    if (!cur) {
	return SQLITE_ERROR;
    }
    cur->cursor.pVtab = vtab;
    csv_rewind(tab->csv);
    cur->pos = csv_tell(tab->csv);
    *cursorp = &cur->cursor;
    return SQLITE_OK;
}

/**
 * Close virtual table cursor.
 * @param cursor cursor pointer
 * @result SQLite error code
 */

static int
csv_vtab_close(sqlite3_vtab_cursor *cursor)
{
    sqlite3_free(cursor);
    return SQLITE_OK;
}

/**
 * Retrieve next row from virtual table cursor
 * @param cursor virtual table cursor
 * @result SQLite error code
 */

static int
csv_vtab_next(sqlite3_vtab_cursor *cursor)
{
    csv_cursor *cur = (csv_cursor *) cursor;
    csv_vtab *tab = (csv_vtab *) cur->cursor.pVtab;

    cur->pos = csv_tell(tab->csv);
    csv_getline(tab->csv, 0);
    return SQLITE_OK;
}

/**
 * Filter function for virtual table.
 * @param cursor virtual table cursor
 * @param idxNum unused (always 0)
 * @param idxStr unused
 * @param argc number arguments (unused, 0)
 * @param argv argument (nothing)
 * @result SQLite error code
 */

static int
csv_vtab_filter(sqlite3_vtab_cursor *cursor, int idxNum,
		const char *idxStr, int argc, sqlite3_value **argv)
{
    csv_cursor *cur = (csv_cursor *) cursor;
    csv_vtab *tab = (csv_vtab *) cur->cursor.pVtab;

    csv_rewind(tab->csv);
    return csv_vtab_next(cursor);
}

/**
 * Return end of table state of virtual table cursor
 * @param cursor virtual table cursor
 * @result true/false
 */

static int
csv_vtab_eof(sqlite3_vtab_cursor *cursor)
{
    csv_cursor *cur = (csv_cursor *) cursor;
    csv_vtab *tab = (csv_vtab *) cur->cursor.pVtab;

    return csv_eof(tab->csv);
}

/**
 * Return column data of virtual table.
 * @param cursor virtual table cursor
 * @param ctx SQLite function context
 * @param n column index
 * @result SQLite error code
 */ 

static int
csv_vtab_column(sqlite3_vtab_cursor *cursor, sqlite3_context *ctx, int n)
{
    csv_cursor *cur = (csv_cursor *) cursor;
    csv_vtab *tab = (csv_vtab *) cur->cursor.pVtab;
    char *data = csv_coldata(tab->csv, n);

    return process_col(ctx, 0, 0, data, tab->coltypes[n], tab->convert);
}

/**
 * Return current rowid of virtual table cursor
 * @param cursor virtual table cursor
 * @param rowidp value buffer to receive current rowid
 * @result SQLite error code
 */

static int
csv_vtab_rowid(sqlite3_vtab_cursor *cursor, sqlite_int64 *rowidp)
{
    csv_cursor *cur = (csv_cursor *) cursor;

    *rowidp = cur->pos;
    return SQLITE_OK;
}

#if (SQLITE_VERSION_NUMBER > 3004000)

/**
 * Rename virtual table.
 * @param newname new name for table
 * @result SQLite error code
 */

static int
csv_vtab_rename(sqlite3_vtab *vtab, const char *newname)
{
    return SQLITE_OK;
}

#endif

/**
 * SQLite module descriptor.
 */

static const sqlite3_module csv_vtab_mod = {
    1,                   /* iVersion */
    csv_vtab_create,     /* xCreate */
    csv_vtab_connect,    /* xConnect */
    csv_vtab_bestindex,  /* xBestIndex */
    csv_vtab_disconnect, /* xDisconnect */
    csv_vtab_destroy,    /* xDestroy */
    csv_vtab_open,       /* xOpen */
    csv_vtab_close,      /* xClose */
    csv_vtab_filter,     /* xFilter */
    csv_vtab_next,       /* xNext */
    csv_vtab_eof,        /* xEof */
    csv_vtab_column,     /* xColumn */
    csv_vtab_rowid,      /* xRowid */
    0,                   /* xUpdate */
    0,                   /* xBegin */
    0,                   /* xSync */
    0,                   /* xCommit */
    0,                   /* xRollback */
    0,                   /* xFindFunction */
#if (SQLITE_VERSION_NUMBER > 3004000)
    csv_vtab_rename,     /* xRename */
#endif
};

/**
 * Import CSV file as table into database
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 *
 * Argument vector contains:
 *
 * argv[0] - name of table to create (required)<br>
 * argv[1] - filename (required)<br>
 * argv[2] - number, when non-zero use first line as column names,
 *           when negative use given type names (optional)<br>
 * argv[3] - number, when non-zero, translate data (optional, see below)<br>
 * argv[4] - column separator characters (optional)<br>
 * argv[5] - string quoting characters (optional)<br>
 * argv[6] - column/type name for first column (optional)<br>
 *   ..<br>
 * argv[X] - column/type name for last column (optional)<br><br>
 *
 * Translation flags:
 *
 * 1  - convert ISO-8859-1 to UTF-8<br>
 * 2  - perform backslash substitution<br>
 * 4  - convert and collapse white-space in column names to underscore<br>
 * 10 - convert \q to single quote, in addition to backslash substitution<br>
 */

static void
csv_import_func(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    csv_file *csv;
    int rc, i, ncnames, row1, convert = 0, useargs = 0;
    char *tname, *fname, *sql = 0, **cnames, *coltypes = 0;
    sqlite3 *db = (sqlite3 *) sqlite3_user_data(ctx);
    sqlite3_stmt *stmt = 0;

    if (argc < 2) {
	sqlite3_result_error(ctx, "need at least 2 arguments", -1);
	return;
    }
    tname = (char *) sqlite3_value_text(argv[0]);
    if (!tname) {
	sqlite3_result_error(ctx, "table name is NULL", -1);
	return;
    }
    fname = (char *) sqlite3_value_text(argv[1]);
    if (!fname) {
	sqlite3_result_error(ctx, "file name is NULL", -1);
	return;
    }
    csv = csv_open(fname,
		   (argc > 4) ? (char *) sqlite3_value_text(argv[4]) : 0,
		   (argc > 5) ? (char *) sqlite3_value_text(argv[5]) : 0);
    if (!csv) {
	sqlite3_result_error(ctx, "unable to open input file", -1);
cleanup:
	if (stmt) {
	    sqlite3_finalize(stmt);
	}
	append_free(&sql);
	if (coltypes) {
	    sqlite3_free(coltypes);
	}
	if (csv) {
	    csv_close(csv);
	}
	return;
    }
    if (!csv->sep && !csv->quot) {
	csv_guess(csv);
    }
    csv->pos0 = 0;
    row1 = 0;
    if (argc > 2) {
	row1 = sqlite3_value_int(argv[2]);
    }
    if (row1) {
	/* use column names from 1st row */
	csv_getline(csv, 0);
	if (csv->ncols < 1) {
	    sqlite3_result_error(ctx, "unable to get column names", -1);
	    goto cleanup;
	}
	csv->pos0 = csv_tell(csv);
	csv_rewind(csv);
	ncnames = csv_ncols(csv);
	cnames = csv->cols;
    } else if (argc > 6) {
	ncnames = argc - 6;
	cnames = 0;
	useargs = 1;
    } else {
	/* use number of columns from 1st row */
	csv_getline(csv, 0);
	if (csv->ncols < 1) {
	    sqlite3_result_error(ctx, "unable to get column names", -1);
	    goto cleanup;
	}
	csv_rewind(csv);
	ncnames = csv_ncols(csv);
	cnames = 0;
    }
    convert = 0;
    if (argc > 3) {
	convert = sqlite3_value_int(argv[3]);
	if (row1 && (convert & 4)) {
	    conv_names(cnames, ncnames);
	}
    }
    /* test if table exists */
    append(&sql, "PRAGMA table_info(", 0);
    append(&sql, tname, '"');
    append(&sql, ")", 0);
    if (!sql) {
oom:
	sqlite3_result_error(ctx, "out of memory", -1);
	goto cleanup;
    }
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    append_free(&sql);
    if (rc != SQLITE_OK) {
prepfail:
	sqlite3_result_error(ctx, "prepare failed", -1);
	goto cleanup;
    }
    /* find number of colums */
    i = 0;
    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW) {
	i++;
	rc = sqlite3_step(stmt);
    }
    if (rc != SQLITE_DONE) {
selfail:
	sqlite3_result_error(ctx, "select failed", -1);
	goto cleanup;
    }
    if (i > 0) {
	/* get column types */
	sqlite3_reset(stmt);
	ncnames = i;
	coltypes = sqlite3_malloc(ncnames);
	if (!coltypes) {
	    goto oom;
	}
	rc = sqlite3_step(stmt);
	i = 0;
	while (rc == SQLITE_ROW) {
	    coltypes[i++] = maptype((char *) sqlite3_column_text(stmt, 2));
	    rc = sqlite3_step(stmt);
	}
	if (rc != SQLITE_DONE) {
	    goto selfail;
	}
    } else {
	/* create new table */
	sqlite3_finalize(stmt);
	stmt = 0;
	coltypes = sqlite3_malloc(ncnames);
	if (!coltypes) {
	    goto oom;
	}
	append(&sql, "CREATE TABLE ", 0);
	append(&sql, tname, '"');
	append(&sql, "(", 0);
	for (i = 0; i < ncnames; i++) {
	    char colname[64];

	    coltypes[i] = SQLITE_TEXT;
	    if (useargs) {
		char *type = (char *) sqlite3_value_text(argv[i + 6]);

		if (!type) {
		    goto defcol;
		}
		append(&sql, type, 0);
		while (*type && !strchr(" \t", *type)) {
		    type++;
		}
		while (*type && strchr(" \t", *type)) {
		    type++;
		}
		coltypes[i] = maptype(type);
	    } else if (!cnames || !cnames[i]) {
defcol:
		sprintf(colname, "column_%d", i + 1);
		append(&sql, colname, '"');
	    } else if (row1 > 0) {
		append(&sql, cnames[i], '"');
	    } else if (row1 < 0) {
		append(&sql, cnames[i], '"');
		if (i + 6 < argc) {
		    char *type = (char *) sqlite3_value_text(argv[i + 6]);
		    
		    if (type) {
			append(&sql, " ", 0);
			append(&sql, type, 0);
			coltypes[i] = maptype(type);
		    }
		}
	    }
	    if (i < ncnames - 1) {
		append(&sql, ",", 0);
	    }
	}
	append(&sql, ")", 0);
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
	if (rc != SQLITE_OK) {
	    goto prepfail;
	}
	rc = sqlite3_step(stmt);
	if ((rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
	    sqlite3_result_error(ctx, "create table failed", -1);
	    goto cleanup;
	}
	append_free(&sql);
    }
    sqlite3_finalize(stmt);
    stmt = 0;
    /* make INSERT statement */
    append(&sql, "INSERT INTO ", 0);
    append(&sql, tname, '"');
    append(&sql, " VALUES(", 0);
    for (i = 0; i < ncnames; i++) {
	append(&sql, (i < ncnames - 1) ? "?," : "?)", 0);
    }
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
	goto prepfail;
    }
    append_free(&sql);
    /* import the CSV file */
    row1 = 0;
    while (csv_getline(csv, 0) != EOF) {
	for (i = 0; i < ncnames; i++) {
	    char *data = csv_coldata(csv, i);

	    rc = process_col(0, stmt, i + 1, data, coltypes[i], convert);
	    if (rc != SQLITE_OK) {
		goto inserr;
	    }
	}
	rc = sqlite3_step(stmt);
	if ((rc != SQLITE_DONE) && (rc != SQLITE_OK)) {
	    if ((rc != SQLITE_MISMATCH) && (rc != SQLITE_CONSTRAINT)) {
inserr:
		sqlite3_result_error(ctx, "insert failed", -1);
		goto cleanup;
	    }
	} else {
	    row1++;
	}
	sqlite3_reset(stmt);
    }
    sqlite3_result_int(ctx, row1);
    goto cleanup;
}

/**
 * Module initializer creating SQLite functions and modules
 * @param db database pointer
 * @result SQLite error code
 */

#ifndef STANDALONE
static
#endif
int
csv_vtab_init(sqlite3 *db)
{
    sqlite3_create_function(db, "import_csv", -1, SQLITE_UTF8,
			    (void *) db, csv_import_func, 0, 0);
    return sqlite3_create_module(db, "csvtable", &csv_vtab_mod, 0);
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
    return csv_vtab_init(db);
}

#endif
