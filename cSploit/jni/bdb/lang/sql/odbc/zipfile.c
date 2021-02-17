/**
 * @file zipfile.c
 * SQLite extension module for mapping a ZIP file as a read-only
 * SQLite virtual table plus some supporting SQLite functions.
 *
 * 2012 September 12
 *
 * The author disclaims copyright to this source code.
 * In place of a legal notice, here is a blessing:
 *
 *    May you do good and not evil.
 *    May you find forgiveness for yourself and forgive others.
 *    May you share freely, never taking more than you give.
 *
 ********************************************************************
 */

#ifdef linux
#define _GNU_SOURCE
#endif

#ifdef STANDALONE
#include <sqlite3.h>
#else
#include <sqlite3ext.h>
static SQLITE_EXTENSION_INIT1
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <zlib.h>

#define ZIP_SIG_LEN			4

#define ZIP_LOCAL_HEADER_SIG		0x04034b50
#define ZIP_LOCAL_HEADER_FLAGS		6
#define ZIP_LOCAL_HEADER_LEN		30
#define ZIP_LOCAL_EXTRA_OFFS		28

#define ZIP_CENTRAL_HEADER_SIG		0x02014b50
#define ZIP_CENTRAL_HEADER_FLAGS	8
#define ZIP_CENTRAL_HEADER_LEN		46
#define ZIP_CENTRAL_COMPMETH_OFFS	10
#define ZIP_CENTRAL_MTIME_OFFS		12
#define ZIP_CENTRAL_MDATE_OFFS		14
#define ZIP_CENTRAL_CRC32_OFFS		16
#define ZIP_CENTRAL_COMPLEN_OFFS	20
#define ZIP_CENTRAL_UNCOMPLEN_OFFS	24
#define ZIP_CENTRAL_PATHLEN_OFFS	28
#define ZIP_CENTRAL_EXTRALEN_OFFS	30
#define ZIP_CENTRAL_COMMENTLEN_OFFS	32
#define ZIP_CENTRAL_LOCALHDR_OFFS	42

#define ZIP_CENTRAL_END_SIG		0x06054b50
#define ZIP_CENTRAL_END_LEN		22
#define ZIP_CENTRAL_ENTS_OFFS		8
#define ZIP_CENTRAL_DIRSTART_OFFS	16

#define ZIP_COMPMETH_STORED		0
#define ZIP_COMPMETH_DEFLATED		8

#define zip_read_int(p)		\
    ((p)[0] | ((p)[1] << 8) | ((p)[2] << 16) | ((p)[3] << 24))
#define zip_read_short(p)	\
    ((p)[0] | ((p)[1] << 8))

/**
 * @typedef zip_file
 * @struct zip_file
 * Structure to implement ZIP file handle.
 */

typedef struct zip_file {
    off_t length;		/**< length of ZIP file */
    unsigned char *data;	/**< mmap()'ed ZIP file */
#if defined(_WIN32) || defined(_WIN64)
    HANDLE h;			/**< Windows file handle */
    HANDLE mh;			/**< Windows file mapping */
#endif
    int nentries;		/**< Number of directory entries */
    unsigned char *entries[1];	/**< Pointer to first entry */
} zip_file;

/**
 * @typedef zip_vtab
 * @struct zip_vtab
 * Structure to describe a ZIP virtual table.
 */

typedef struct zip_vtab {
    sqlite3_vtab vtab;	/**< SQLite virtual table */
    sqlite3 *db;	/**< Open database */
    zip_file *zip;	/**< ZIP file handle */
    int sorted;		/**< 1 = sorted by path, -1 = sorting, 0 = unsorted */
    char tblname[1];	/**< Name, format "database"."table" */
} zip_vtab;

/**
 * @typedef zip_cursor
 * @struct zip_cursor
 * Structure to describe ZIP virtual table cursor.
 */

typedef struct {
    sqlite3_vtab_cursor cursor;		/**< SQLite virtual table cursor */
    int pos;				/**< ZIP file position */
    int nmatches;			/**< For filter EQ */
    int *matches;			/**< For filter EQ */
} zip_cursor;

#ifdef SQLITE_OPEN_URI

/**
 * @typedef mem_blk
 * @struct mem_blk
 * Structure to describe in-core SQLite database read from BLOB.
 */

typedef struct mem_blk {
#define MEM_MAGIC "MVFS"
    char magic[4];			/**< magic number */
    int opened;				/**< open counter */
#if defined(_WIN32) || defined(_WIN64)
    HANDLE mh;				/**< handle for memory mapping */
#else
    long psize;				/**< page size */
#ifdef linux
    sqlite3_mutex *mutex;		/**< mutex to protect mapping */
    int lcnt;				/**< lock counter */
#endif
#endif
    unsigned long size;			/**< size of memory mapped area */
    unsigned long length;		/**< real length of data area */
    unsigned char *data;		/**< data area */
} mem_blk;

/**
 * @typedef mem_file
 * @struct mem_file
 * SQLite3 file structure enhanced by mem_blk.
 */

typedef struct mem_file {
    sqlite3_file base;			/**< sqlite3_file base structure */
#ifdef linux
    int lock;				/**< lock state */
#endif
    mem_blk *mb;			/**< pointer to memory block */
} mem_file;

/*
 * Private VFS name
 */

static char mem_vfs_name[64];

#endif /* SQLITE_OPEN_URI */

/**
 * Memory map ZIP file for reading and return handle to it.
 * @param filename name of ZIP file
 * @result ZIP file handle
 */

static zip_file *
zip_open(const char *filename)
{
#if defined(_WIN32) || defined(_WIN64)
    HANDLE h, mh = INVALID_HANDLE_VALUE;
    DWORD n, length;
    unsigned char *data = 0;
#else
    int fd;
    off_t length;
    unsigned char *data = MAP_FAILED;
#endif
    int nentries, i;
    zip_file *zip = 0;
    unsigned char *p, *q, magic[ZIP_SIG_LEN];

    if (!filename) {
	return 0;
    }
#if defined(_WIN32) || defined(_WIN64)
    h = CreateFile(filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE) {
	goto error;
    }
    if (!ReadFile(h, magic, sizeof (magic), &n, 0) ||
	(n != sizeof (magic))) {
	goto error;
    }
    if (zip_read_int(magic) != ZIP_LOCAL_HEADER_SIG) {
	goto error;
    }
    length = GetFileSize(h, 0);
    if ((length == INVALID_FILE_SIZE) || (length < ZIP_CENTRAL_END_LEN)) {
	goto error;
    }
    mh = CreateFileMapping(h, 0, PAGE_READONLY, 0, length, 0);
    if (mh == INVALID_HANDLE_VALUE) {
	goto error;
    }
    data = MapViewOfFile(mh, FILE_MAP_READ, 0, 0, length);
    if (!data) {
	goto error;
    }
#else
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
	goto error;
    }
    if (read(fd, magic, ZIP_SIG_LEN) != ZIP_SIG_LEN) {
	goto error;
    }
    if (zip_read_int(magic) != ZIP_LOCAL_HEADER_SIG) {
	goto error;
    }
    length = lseek(fd, 0, SEEK_END);
    if ((length == -1) || (length < ZIP_CENTRAL_END_LEN)) {
	goto error;
    }
    data = (unsigned char *) mmap(0, length, PROT_READ,
#ifdef MAP_FILE
				  MAP_FILE | 
#endif
                                 MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
	goto error;
    }
    close(fd);
    fd = -1;
#endif
    p = data + length - ZIP_CENTRAL_END_LEN;
    while (p >= data) {
	if (*p == (ZIP_CENTRAL_END_SIG & 0xFF)) {
	    if (zip_read_int(p) == ZIP_CENTRAL_END_SIG) {
		break;
	    }
	    p -= ZIP_SIG_LEN;
	} else {
	    --p;
	}
    }
    if (p < data) {
	goto error;
    }
    nentries = zip_read_short(p + ZIP_CENTRAL_ENTS_OFFS);
    if (nentries == 0) {
	goto error;
    }
    p = data + zip_read_int(p + ZIP_CENTRAL_DIRSTART_OFFS);
    q = p;
    for (i = 0; i < nentries; i++) {
	int pathlen, comlen, extra;

	if ((q + ZIP_CENTRAL_HEADER_LEN) > (data + length)) {
	    goto error;
	}
	if (zip_read_int(q) != ZIP_CENTRAL_HEADER_SIG) {
	    goto error;
	}
	pathlen = zip_read_short(q + ZIP_CENTRAL_PATHLEN_OFFS);
	comlen = zip_read_short(q + ZIP_CENTRAL_COMMENTLEN_OFFS);
	extra = zip_read_short(q + ZIP_CENTRAL_EXTRALEN_OFFS);
	q += pathlen + comlen + extra + ZIP_CENTRAL_HEADER_LEN;
    }
    zip = sqlite3_malloc(sizeof (zip_file) +
			 nentries * sizeof (unsigned char *));
    if (!zip) {
	goto error;
    }
#if defined(_WIN32) || defined(_WIN64)
    zip->h = zip->mh = INVALID_HANDLE_VALUE;
#endif
    zip->length = length;
    zip->data = data;
    zip->nentries = nentries;
    q = p;
    for (i = 0; i < nentries; i++) {
	int pathlen, comlen, extra;

	if ((q + ZIP_CENTRAL_HEADER_LEN) > (data + length)) {
	    goto error;
	}
	if (zip_read_int(q) != ZIP_CENTRAL_HEADER_SIG) {
	    goto error;
	}
	zip->entries[i] = q;
	pathlen = zip_read_short(q + ZIP_CENTRAL_PATHLEN_OFFS);
	comlen = zip_read_short(q + ZIP_CENTRAL_COMMENTLEN_OFFS);
	extra = zip_read_short(q + ZIP_CENTRAL_EXTRALEN_OFFS);
	q += pathlen + comlen + extra + ZIP_CENTRAL_HEADER_LEN;
    }
    zip->entries[i] = 0;
#if defined(_WIN32) || defined(_WIN64)
    zip->h = h;
    zip->mh = mh;
#endif
    return zip;
error:
    if (zip) {
	sqlite3_free(zip);
    }
#if defined(_WIN32) || defined(_WIN64)
    if (data) {
	UnmapViewOfFile(data);
    }
    if (mh != INVALID_HANDLE_VALUE) {
	CloseHandle(mh);
    }
    if (h != INVALID_HANDLE_VALUE) {
	CloseHandle(h);
    }
#else
    if (data != MAP_FAILED) {
	munmap(data, length);
    }
    if (fd >= 0) {
	close(fd);
    }
#endif
    return 0;
}

/**
 * Close ZIP file handle.
 * @param zip ZIP file handle
 */

static void
zip_close(zip_file *zip)
{
    if (zip) {
#if defined(_WIN32) || defined(_WIN64)
	if (zip->data) {
	    UnmapViewOfFile(zip->data);
	}
	if (zip->mh != INVALID_HANDLE_VALUE) {
	    CloseHandle(zip->mh);
	}
	if (zip->h != INVALID_HANDLE_VALUE) {
	    CloseHandle(zip->h);
	}
#else
	if (zip->data) {
	    munmap(zip->data, zip->length);
	}
#endif
	zip->length = 0;
	zip->data = 0;
	zip->nentries = 0;
	sqlite3_free(zip);
    }
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
 * argv[3] - filename of ZIP file<br>
 */

static int
zip_vtab_connect(sqlite3* db, void *aux, int argc, const char * const *argv,
		 sqlite3_vtab **vtabp, char **errp)
{
    zip_file *zip = 0;
    int rc = SQLITE_ERROR;
    char *filename;
    zip_vtab *vtab;

    if (argc < 4) {
	*errp = sqlite3_mprintf("input file name missing");
	return SQLITE_ERROR;
    }
    filename = unquote(argv[3]);
    if (filename) {
	zip = zip_open(filename);
	sqlite3_free(filename);
    }
    if (!zip) {
	*errp = sqlite3_mprintf("unable to open input file");
	return rc;
    }
    vtab = sqlite3_malloc(sizeof(zip_vtab) + 6 +
			  strlen(argv[1]) + strlen(argv[2]));
    if (!vtab) {
	zip_close(zip);
	*errp = sqlite3_mprintf("out of memory");
	return rc;
    }
    memset(vtab, 0, sizeof (*vtab));
    strcpy(vtab->tblname, "\"");
    strcat(vtab->tblname, argv[1]);
    strcat(vtab->tblname, "\".\"");
    strcat(vtab->tblname, argv[2]);
    strcat(vtab->tblname, "\"");
    vtab->db = db;
    vtab->zip = zip;
    rc = sqlite3_declare_vtab(db, "CREATE TABLE x(path, comp, mtime, "
			      "crc32, length, data, clength, cdata)");
    if (rc != SQLITE_OK) {
	zip_close(zip);
	sqlite3_free(vtab);
	*errp = sqlite3_mprintf("table definition failed (error %d)", rc);
	return rc;
    }
    *vtabp = &vtab->vtab;
    *errp = 0;
    return SQLITE_OK;
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
zip_vtab_create(sqlite3* db, void *aux, int argc,
		const char *const *argv,
		sqlite3_vtab **vtabp, char **errp)
{
    return zip_vtab_connect(db, aux, argc, argv, vtabp, errp);
}

/**
 * Disconnect virtual table.
 * @param vtab virtual table pointer
 * @result always SQLITE_OK
 */

static int
zip_vtab_disconnect(sqlite3_vtab *vtab)
{
    zip_vtab *tab = (zip_vtab *) vtab;

    zip_close(tab->zip);
    sqlite3_free(tab);
    return SQLITE_OK;
}

/**
 * Destroy virtual table.
 * @param vtab virtual table pointer
 * @result always SQLITE_OK
 */

static int
zip_vtab_destroy(sqlite3_vtab *vtab)
{
    return zip_vtab_disconnect(vtab);
}

/**
 * Determines information for filter function according to constraints.
 * @param vtab virtual table
 * @param info index/constraint information
 * @result SQLite error code
 */

static int
zip_vtab_bestindex(sqlite3_vtab *vtab, sqlite3_index_info *info)
{
    zip_vtab *tab = (zip_vtab *) vtab;
    int i;

    info->idxNum = 0;
    if (tab->sorted == 0) {
	char *sql = 0;
	unsigned char **entries = 0;
	sqlite3_stmt *stmt = 0;
	int rc, count, i;
	size_t tmp;

	/* perform sorting on 0th column (path, string) */
	tab->sorted = -1;
	entries = sqlite3_malloc(tab->zip->nentries * sizeof (entries));
	sql = sqlite3_mprintf("SELECT rowid FROM %s ORDER BY path",
			      tab->tblname);
	if (sql && entries) {
	    rc = sqlite3_prepare_v2(tab->db, sql, -1, &stmt, 0);
	    if ((rc == SQLITE_OK) && stmt) {
		count = 0;
		while (1) {
		    rc = sqlite3_step(stmt);
		    if (rc != SQLITE_ROW) {
			break;
		    }
		    tmp = sqlite3_column_int(stmt, 0);
		    entries[count++] = (unsigned char *) tmp;
		}
		if ((rc == SQLITE_DONE) && (count == tab->zip->nentries)) {
		    for (i = 0; i < count; i++) {
			tmp = (size_t) entries[i];
			tmp = (size_t) tab->zip->entries[tmp];
			entries[i] = (unsigned char *) tmp;
		    }
		    memcpy(tab->zip->entries, entries, i * sizeof (entries));
		    tab->sorted = 1;
		}
	    }
	}
	if (stmt) {
	    sqlite3_finalize(stmt);
	}
	if (sql) {
	    sqlite3_free(sql);
	}
	if (entries) {
	    sqlite3_free(entries);
	}
    }
    /* no optimization while table is being sorted or is unsorted */
    if (tab->sorted != 1) {
	return SQLITE_OK;
    }
    /* support EQ or simple MATCH constraint on 0th column (path) */
    for (i = 0; i < info->nConstraint; i++) {
	if (info->aConstraint[i].usable &&
	    (info->aConstraint[i].iColumn == 0)) {
	    if (info->aConstraint[i].op == SQLITE_INDEX_CONSTRAINT_EQ) {
		info->idxNum = 1;
		info->aConstraintUsage[i].argvIndex = 1;
		info->aConstraintUsage[i].omit = 1;
		info->estimatedCost = 1.0;
		break;
	    } else if (info->aConstraint[i].op ==
		       SQLITE_INDEX_CONSTRAINT_MATCH) {
		info->idxNum = 2;
		info->aConstraintUsage[i].argvIndex = 1;
		info->aConstraintUsage[i].omit = 1;
		info->estimatedCost = 2.0;
		break;
	    }
	}
    }
    /* ORDER BY is ascending on 0th column (path) when table is sorted */
    if (info->nOrderBy > 0) {
	if ((info->aOrderBy[0].iColumn == 0) && !info->aOrderBy[0].desc) {
	    info->orderByConsumed = 1;
	}
    }
    return SQLITE_OK;
}

/**
 * Open virtual table and return cursor.
 * @param vtab virtual table pointer
 * @param cursorp pointer receiving cursor pointer
 * @result SQLite error code
 */

static int
zip_vtab_open(sqlite3_vtab *vtab, sqlite3_vtab_cursor **cursorp)
{
    zip_cursor *cur = sqlite3_malloc(sizeof(*cur));

    if (!cur) {
	return SQLITE_ERROR;
    }
    cur->cursor.pVtab = vtab;
    cur->pos = -1;
    cur->nmatches = 0;
    cur->matches = 0;
    *cursorp = &cur->cursor;
    return SQLITE_OK;
}

/**
 * Close virtual table cursor.
 * @param cursor cursor pointer
 * @result SQLite error code
 */

static int
zip_vtab_close(sqlite3_vtab_cursor *cursor)
{
    zip_cursor *cur = (zip_cursor *) cursor;

    if (cur->matches) {
	sqlite3_free(cur->matches);
    }
    sqlite3_free(cur);
    return SQLITE_OK;
}

/**
 * Retrieve next row from virtual table cursor
 * @param cursor virtual table cursor
 * @result SQLite error code
 */

static int
zip_vtab_next(sqlite3_vtab_cursor *cursor)
{
    zip_cursor *cur = (zip_cursor *) cursor;

    if (cur->nmatches >= 0) {
	cur->pos++;
    }
    return SQLITE_OK;
}

/**
 * Filter function for virtual table.
 * @param cursor virtual table cursor
 * @param idxNum used for expression (1 -> EQ, 2 -> MATCH, 0 else)
 * @param idxStr nod used
 * @param argc number arguments (1 -> EQ/MATCH, 0 else)
 * @param argv argument (nothing or RHS of filter expression)
 * @result SQLite error code
 */

static int
zip_vtab_filter(sqlite3_vtab_cursor *cursor, int idxNum,
		const char *idxStr, int argc, sqlite3_value **argv)
{
    zip_cursor *cur = (zip_cursor *) cursor;
    zip_vtab *tab = (zip_vtab *) cur->cursor.pVtab;

    if (cur->matches) {
	sqlite3_free(cur->matches);
	cur->matches = 0;
    }
    cur->nmatches = 0;
    /* if EQ or MATCH constraint is active, add match array to cursor */
    if (idxNum && (argc > 0)) {
	int i, k, d, found, leneq, len;
	unsigned char *eq;

	eq = (unsigned char *) sqlite3_value_text(argv[0]);
	if (!eq) {
	    cur->nmatches = -1;
	    goto done;
	}
	if (idxNum > 1) {
	    unsigned char *p = (unsigned char *) strrchr((char *) eq, '*');

	    if (!p || (p[1] != '\0')) {
		return SQLITE_ERROR;
	    }
	    leneq = p - eq;
	} else {
	    leneq = sqlite3_value_bytes(argv[0]);
	    if (leneq == 0) {
		cur->nmatches = -1;
		goto done;
	    }
	}
	cur->matches = sqlite3_malloc(tab->zip->nentries * sizeof (int));
	if (!cur->matches) {
	    return SQLITE_NOMEM;
	}
	memset(cur->matches, 0, tab->zip->nentries * sizeof (int));
	for (k = found = 0; k < tab->zip->nentries; k++) {
	    len = zip_read_short(tab->zip->entries[k] +
				 ZIP_CENTRAL_PATHLEN_OFFS);
	    if (idxNum > 1) {
		if (len < leneq) {
		    continue;
		}
	    } else if (len != leneq) {
		if (found) {
		    break;
		}
		continue;
	    }
	    d = memcmp(tab->zip->entries[k] + ZIP_CENTRAL_HEADER_LEN,
		       eq, leneq);
	    if (d == 0) {
		found++;
		cur->matches[k] = 1;
	    } else if (d > 0) {
		break;
	    }
	}
	for (i = k = 0; i < tab->zip->nentries; i++) {
	    if (cur->matches[i]) {
		cur->matches[k++] = i;
	    }
	}
	cur->nmatches = k;
    }
done:
    cur->pos = -1;
    return zip_vtab_next(cursor);
}

/**
 * Return end of table state of virtual table cursor
 * @param cursor virtual table cursor
 * @result true/false
 */

static int
zip_vtab_eof(sqlite3_vtab_cursor *cursor)
{
    zip_cursor *cur = (zip_cursor *) cursor;
    zip_vtab *tab = (zip_vtab *) cur->cursor.pVtab;

    if (cur->nmatches < 0) {
	return 1;
    }
    if (cur->nmatches) {
	return cur->pos >= cur->nmatches;
    }
    return cur->pos >= tab->zip->nentries;
}

/**
 * Return column data of virtual table.
 * @param cursor virtual table cursor
 * @param ctx SQLite function context
 * @param n column index
 * @result SQLite error code
 */ 

static int
zip_vtab_column(sqlite3_vtab_cursor *cursor, sqlite3_context *ctx, int n)
{
    zip_cursor *cur = (zip_cursor *) cursor;
    zip_vtab *tab = (zip_vtab *) cur->cursor.pVtab;
    unsigned char *data = 0;
    unsigned char *dest = 0;
    int length;

    if (cur->nmatches) {
	int pos;

	if ((cur->pos < 0) || (cur->pos >= cur->nmatches)) {
	    sqlite3_result_error(ctx, "out of bounds", -1);
	    return SQLITE_ERROR;
	}
	pos = cur->matches[cur->pos];
	data = tab->zip->entries[pos];
    } else {
	if ((cur->pos < 0) || (cur->pos >= tab->zip->nentries)) {
	    sqlite3_result_error(ctx, "out of bounds", -1);
	    return SQLITE_ERROR;
	}
	data = tab->zip->entries[cur->pos];
    }
    switch (n) {
    case 0:	/* "path": pathname */
	length = zip_read_short(data + ZIP_CENTRAL_PATHLEN_OFFS);
	data += ZIP_CENTRAL_HEADER_LEN;
	sqlite3_result_text(ctx, (char *) data, length, SQLITE_TRANSIENT);
	return SQLITE_OK;
    case 1:	/* "comp": compression method */
	length = zip_read_short(data + ZIP_CENTRAL_COMPMETH_OFFS);
	sqlite3_result_int(ctx, length);
	return SQLITE_OK;
    case 2:	/* "mtime": modification time/date */
	{
	    int time = zip_read_short(data + ZIP_CENTRAL_MTIME_OFFS);
	    int date = zip_read_short(data + ZIP_CENTRAL_MDATE_OFFS);
	    char mtbuf[64];

	    sprintf(mtbuf, "%04d-%02d-%02d %02d:%02d:%02d",
		    (date >> 9) + 1980, (date >> 5) & 0xf, date & 0x1f,
		    time >> 11, (time >> 5) & 0x3f, (time & 0x1f) << 1);
	    sqlite3_result_text(ctx, mtbuf, -1, SQLITE_TRANSIENT);
	    return SQLITE_OK;
	}
    case 3:	/* "crc32": CRC32 of uncompress data */
	length = zip_read_int(data + ZIP_CENTRAL_CRC32_OFFS);
	sqlite3_result_int(ctx, length);
	return SQLITE_OK;
    case 4:	/* "length": uncompress length of data */
	length = zip_read_int(data + ZIP_CENTRAL_UNCOMPLEN_OFFS);
	sqlite3_result_int(ctx, length);
	return SQLITE_OK;
    case 5:	/* "data": uncompressed data */
	{
	    int clength, offs, extra, pathlen, cmeth;

	    offs = zip_read_int(data + ZIP_CENTRAL_LOCALHDR_OFFS);
	    if ((offs + ZIP_LOCAL_HEADER_LEN) > tab->zip->length) {
		goto donull;
	    }
	    extra = zip_read_short(tab->zip->data +
				   offs + ZIP_LOCAL_EXTRA_OFFS);
	    pathlen = zip_read_short(data + ZIP_CENTRAL_PATHLEN_OFFS);
	    length = zip_read_int(data + ZIP_CENTRAL_UNCOMPLEN_OFFS);
	    clength = zip_read_int(data + ZIP_CENTRAL_COMPLEN_OFFS);
	    cmeth = zip_read_short(data + ZIP_CENTRAL_COMPMETH_OFFS);
	    offs += ZIP_LOCAL_HEADER_LEN + pathlen + extra;
	    if ((offs + clength) > tab->zip->length) {
		goto donull;
	    }
	    data = tab->zip->data + offs;
	    if (cmeth == ZIP_COMPMETH_STORED) {
		sqlite3_result_blob(ctx, data, clength, SQLITE_TRANSIENT);
		return SQLITE_OK;
	    } else if (cmeth == ZIP_COMPMETH_DEFLATED) {
		z_stream stream;
		int err;

		stream.zalloc = Z_NULL;
		stream.zfree = Z_NULL;
		stream.next_in = data;
		stream.avail_in = clength;
		stream.next_out = dest = sqlite3_malloc(length);
		stream.avail_out = length;
		stream.opaque = 0;
		if (!dest) {
		    goto donull;
		}
		if (inflateInit2(&stream, -15) != Z_OK) {
		    goto donull;
		}
		err = inflate(&stream, Z_SYNC_FLUSH);
		inflateEnd(&stream);
		if ((err == Z_STREAM_END) ||
		    ((err == Z_OK) && (stream.avail_in == 0))) {
		    sqlite3_result_blob(ctx, dest, length, sqlite3_free);
		    return SQLITE_OK;
		}
	    }
	donull:
	    if (dest) {
		sqlite3_free(dest);
	    }
	    sqlite3_result_null(ctx);
	    return SQLITE_OK;
	}
    case 6:	/* "clength": compressed length */
	length = zip_read_int(data + ZIP_CENTRAL_COMPLEN_OFFS);
	sqlite3_result_int(ctx, length);
	return SQLITE_OK;
    case 7:	/* "cdata": raw data */
	{
	    int clength, offs, extra, pathlen;

	    offs = zip_read_int(data + ZIP_CENTRAL_LOCALHDR_OFFS);
	    if ((offs + ZIP_LOCAL_HEADER_LEN) > tab->zip->length) {
		goto donull;
	    }
	    extra = zip_read_short(tab->zip->data +
				   offs + ZIP_LOCAL_EXTRA_OFFS);
	    pathlen = zip_read_short(data + ZIP_CENTRAL_PATHLEN_OFFS);
	    length = zip_read_int(data + ZIP_CENTRAL_UNCOMPLEN_OFFS);
	    clength = zip_read_int(data + ZIP_CENTRAL_COMPLEN_OFFS);
	    offs += ZIP_LOCAL_HEADER_LEN + pathlen + extra;
	    if ((offs + clength) > tab->zip->length) {
		goto donull;
	    }
	    data = tab->zip->data + offs;
	    sqlite3_result_blob(ctx, data, clength, SQLITE_TRANSIENT);
	    return SQLITE_OK;
	}
    }
    sqlite3_result_error(ctx, "invalid column number", -1);
    return SQLITE_ERROR;
}

/**
 * Return current rowid of virtual table cursor
 * @param cursor virtual table cursor
 * @param rowidp value buffer to receive current rowid
 * @result SQLite error code
 */

static int
zip_vtab_rowid(sqlite3_vtab_cursor *cursor, sqlite_int64 *rowidp)
{
    zip_cursor *cur = (zip_cursor *) cursor;

    if (cur->nmatches < 0) {
	*rowidp = -1;
    } else if ((cur->pos >= 0) && (cur->nmatches > 0)) {
	*rowidp = cur->matches[cur->pos];
    } else {
	*rowidp = cur->pos;
    }
    return SQLITE_OK;
}

/**
 * Internal MATCH function for virtual table.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void
zip_vtab_matchfunc(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    int ret = 0;

    if (argc == 2) {
	unsigned char *q = (unsigned char *) sqlite3_value_text(argv[0]);
	unsigned char *p = (unsigned char *) sqlite3_value_text(argv[1]);

	if (p && q) {
	    unsigned char *eq = (unsigned char *) strrchr((char *) q, '*');
	    int lenq, lenp;

	    if (eq && (eq[1] == '\0')) {
		lenq = eq - q;
		if (lenq) {
		    lenp = strlen((char *) p);
		    if ((lenp >= lenq) && !memcmp(p, q, lenq)) {
			ret = 1;
		    }
		}
	    }
	}
    }
    sqlite3_result_int(ctx, ret);
}

/**
 * Find overloaded function on virtual table.
 * @param vtab virtual table
 * @param narg number arguments
 * @param name function name
 * @param pfunc pointer to function (value return)
 * @param parg pointer to function's argument (value return)
 * @result 0 or 1
 */

static int
zip_vtab_findfunc(sqlite3_vtab *vtab, int narg, const char *name,
		  void (**pfunc)(sqlite3_context *, int, sqlite3_value **),
		  void **parg)
{
    if ((narg == 2) && !strcmp(name, "match")) {
	*pfunc = zip_vtab_matchfunc;
	*parg = 0;
	return 1;
    }
    return 0;
}

#if (SQLITE_VERSION_NUMBER > 3004000)

/**
 * Rename virtual table.
 * @param newname new name for table
 * @result SQLite error code
 */

static int
zip_vtab_rename(sqlite3_vtab *vtab, const char *newname)
{
    return SQLITE_OK;
}

#endif

/**
 * SQLite module descriptor.
 */

static const sqlite3_module zip_vtab_mod = {
    1,                   /* iVersion */
    zip_vtab_create,     /* xCreate */
    zip_vtab_connect,    /* xConnect */
    zip_vtab_bestindex,  /* xBestIndex */
    zip_vtab_disconnect, /* xDisconnect */
    zip_vtab_destroy,    /* xDestroy */
    zip_vtab_open,       /* xOpen */
    zip_vtab_close,      /* xClose */
    zip_vtab_filter,     /* xFilter */
    zip_vtab_next,       /* xNext */
    zip_vtab_eof,        /* xEof */
    zip_vtab_column,     /* xColumn */
    zip_vtab_rowid,      /* xRowid */
    0,                   /* xUpdate */
    0,                   /* xBegin */
    0,                   /* xSync */
    0,                   /* xCommit */
    0,                   /* xRollback */
    zip_vtab_findfunc,   /* xFindFunction */
#if (SQLITE_VERSION_NUMBER > 3004000)
    zip_vtab_rename,     /* xRename */
#endif
};

/**
 * Compute CRC32 given blob
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void
zip_crc32_func(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    int crc, length;
    unsigned char *data;

    if (argc != 1) {
	sqlite3_result_error(ctx, "need one argument", -1);
    }
    data = (unsigned char *) sqlite3_value_blob(argv[0]);
    length = sqlite3_value_bytes(argv[0]);
    crc = crc32(0, 0, 0);
    if (data && (length > 0)) {
	crc = crc32(crc, data, length);
    }
    sqlite3_result_int(ctx, crc);
}

/**
 * Inflate data given blob
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void
zip_inflate_func(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    int err, length, dlength, avail;
    unsigned char *data, *dest, *newdest;
    z_stream stream;

    if (argc != 1) {
	sqlite3_result_error(ctx, "need one argument", -1);
	return;
    }
    data = (unsigned char *) sqlite3_value_blob(argv[0]);
    length = sqlite3_value_bytes(argv[0]);
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.next_in = data;
    stream.avail_in = length;
    avail = length;
    stream.next_out = dest = sqlite3_malloc(avail);
    stream.avail_out = avail;
    stream.opaque = 0;
    if (!dest) {
	goto oom;
    }
    if (inflateInit2(&stream, -15) != Z_OK) {
	goto nomem;
    }
    dlength = 0;
    while (1) {
	err = inflate(&stream, Z_SYNC_FLUSH);
	if ((err == Z_STREAM_END) ||
	    ((err == Z_OK) && (stream.avail_in == 0))) {
	    dlength += length - stream.avail_out;
	    newdest = sqlite3_realloc(dest, dlength);
	    inflateEnd(&stream);
	    if (!newdest) {
nomem:
		if (dest) {
		    sqlite3_free(dest);
		}
oom:
		sqlite3_result_error_nomem(ctx);
		return;
	    }
	    sqlite3_result_blob(ctx, newdest, dlength, sqlite3_free);
	    return;
	}
	if ((err == Z_BUF_ERROR) || (err == Z_OK)) {
	    newdest = sqlite3_realloc(dest, avail + length);
	    dlength += length - stream.avail_out;
	    if (!newdest) {
		inflateEnd(&stream);
		goto nomem;
	    }
	    avail += length;
	    stream.next_out = newdest + (stream.next_out - dest);
	    dest = newdest;
	    stream.avail_out += length;
	} else {
	    inflateEnd(&stream);
	    sqlite3_free(dest);
	    sqlite3_result_error(ctx, "inflate error", -1);
	    return;
	}
    }
}

/**
 * Deflate data given blob and optional compression level
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void
zip_deflate_func(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    int err, level = 9;
    unsigned long avail, length;
    unsigned char *data, *dest = 0;
    z_stream stream;

    if ((argc < 1) || (argc > 2)) {
	sqlite3_result_error(ctx, "need one or two arguments", -1);
	return;
    }
    if (argc > 1) {
	level = sqlite3_value_int(argv[1]);
    }
    data = (unsigned char *) sqlite3_value_blob(argv[0]);
    length = sqlite3_value_bytes(argv[0]);
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.next_in = data;
    stream.avail_in = length;
    stream.next_out = 0;
    stream.avail_out = 0;
    stream.opaque = 0;
    if (deflateInit2(&stream, level, Z_DEFLATED, -15, 8,
		     Z_DEFAULT_STRATEGY) != Z_OK) {
	goto deflerr;
    }
    avail = deflateBound(&stream, length);
    if (avail == 0) {
	sqlite3_result_null(ctx);
	return;
    }
    stream.next_out = dest = sqlite3_malloc(avail);
    stream.avail_out = avail;
    if (!dest) {
	sqlite3_result_error_nomem(ctx);
	return;
    }
    err = deflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
	deflateEnd(&stream);
deflerr:
	if (dest) {
	    sqlite3_free(dest);
	}
	sqlite3_result_error(ctx, "deflate error", -1);
	return;
    }
    length = stream.total_out;
    err = deflateEnd(&stream);
    if (err != Z_OK) {
	goto deflerr;
    }
    sqlite3_result_blob(ctx, dest, length, sqlite3_free);
}

/**
 * Compress data given blob and optional compression level
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 */

static void
zip_compress_func(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    int err, level = 9;
    unsigned long length, dlength;
    unsigned char *data, *dest;

    if ((argc < 1) || (argc > 2)) {
	sqlite3_result_error(ctx, "need one or two arguments", -1);
	return;
    }
    if (argc > 1) {
	level = sqlite3_value_int(argv[1]);
    }
    data = (unsigned char *) sqlite3_value_blob(argv[0]);
    length = sqlite3_value_bytes(argv[0]);
    dlength = compressBound(length);
    dest = sqlite3_malloc(dlength);
    if (!dest) {
	sqlite3_result_error_nomem(ctx);
	return;
    }
    err = compress2(dest, &dlength, data, length, level);
    if (err == Z_OK) {
	sqlite3_result_blob(ctx, dest, dlength, sqlite3_free);
	return;
    }
    if (err == Z_MEM_ERROR) {
	sqlite3_result_error(ctx, "memory error", -1);
    } else if (err == Z_BUF_ERROR) {
	sqlite3_result_error(ctx, "buffer error", -1);
    } else {
	sqlite3_result_error(ctx, "compress error", -1);
    }
    sqlite3_free(dest);
}

#ifdef SQLITE_OPEN_URI

/**
 * Create mem_blk from given data buffer and length.
 * @param data data buffer
 * @param length length of buffer
 * @result pointer to mem_blk or NULL
 */

static mem_blk *
mem_createmb(const unsigned char *data, unsigned long length)
{
    mem_blk *mb;
#if defined(_WIN32) || defined(_WIN64)
    HANDLE mh;
#else
    long psize;
#endif
    unsigned long size;

#if defined(_WIN32) || defined(_WIN64)
    size = sizeof (mem_blk) + length;
    mh = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE,
			   0, size, 0);
    if (mh == INVALID_HANDLE_VALUE) {
	return 0;
    }
    mb = (mem_blk *) MapViewOfFile(mh, FILE_MAP_ALL_ACCESS, 0, 0, length);
    if (!mb) {
	return 0;
    }
#else
    psize = sysconf(_SC_PAGESIZE);
#ifdef linux
    mb = (mem_blk *) sqlite3_malloc(sizeof (mem_blk));
    if (!mb) {
	return 0;
    }
    size = length + 1;
    mb->data = (unsigned char *) mmap(0, size, PROT_READ | PROT_WRITE,
				      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mb->data == MAP_FAILED) {
	sqlite3_free(mb);
	return 0;
    }
#else
    size = sizeof (mem_blk) + psize + length + 1;
    mb = (mem_blk *) mmap(0, size, PROT_READ | PROT_WRITE,
#if defined(MAP_ANONYMOUS)
			  MAP_ANONYMOUS | 
#elif defined(MAP_ANON)
			  MAP_ANON |
#endif 
                         MAP_PRIVATE, -1, 0);
    if (mb == MAP_FAILED) {
	return 0;
    }
#endif
#endif
    if (mb) {
	memcpy(mb->magic, MEM_MAGIC, 4);
	mb->opened = 1;
	mb->size = size;
	mb->length = length;
#if defined(_WIN32) || defined(_WIN64)
	mb->mh = mh;
	mb->data = (unsigned char *) (mb + 1);
	memcpy(mb->data, data, length);
#else
	mb->psize = psize;
#ifdef linux
	mb->mutex = sqlite3_mutex_alloc(SQLITE_MUTEX_FAST);
	sqlite3_mutex_enter(mb->mutex);
	mb->lcnt = 0;
	memcpy(mb->data, data, length);
#else
	if (psize >= sizeof (mem_blk)) {
	    mb->data = (unsigned char *) mb + psize;
	    memcpy(mb->data, data, length);
#ifndef linux
	    mprotect(mb->data, length, PROT_READ);
#endif
	} else {
	    mb->data = (unsigned char *) (mb + 1);
	    memcpy(mb->data, data, length);
	}
#endif
#endif
    }
    return mb;
}

/**
 * Destroy given mem_blk.
 * @param mb mem_blk pointer
 */

static void
mem_destroymb(mem_blk *mb)
{
#if defined(_WIN32) || defined(_WIN64)
    HANDLE mh;
#endif

    if (mb) {
	memset(mb->magic, 0, 4);
#if defined(_WIN32) || defined(_WIN64)
	mh = mb->mh;
	UnmapViewOfFile(mb);
	CloseHandle(mh);
#else
#ifdef linux
	munmap(mb->data, mb->size);
	sqlite3_mutex_leave(mb->mutex);
	sqlite3_mutex_free(mb->mutex);
	sqlite3_free(mb);
#else
	munmap(mb, mb->size);
#endif
#endif
    }
}

/**
 * Close mem_file and release associated mem_blk if open count drops to zero.
 * @param file SQLite3 file pointer
 * @result SQLite error code
 */

static int
mem_close(sqlite3_file *file)
{
    mem_file *mf = (mem_file *) file;
    mem_blk *mb = mf->mb;

    if (mb) {
#ifdef linux
	sqlite3_mutex_enter(mb->mutex);
	if (mf->lock > 0) {
	    mb->lcnt = 0;
	}
#endif
	mb->opened--;
	if (mb->opened <= 0) {
	    mem_destroymb(mb);
	} 
#ifdef linux
	else {
	    sqlite3_mutex_leave(mb->mutex);
	}
#endif
	mf->mb = 0;
    }
    return SQLITE_OK;
}

/**
 * Read data from mem_file.
 * @param file SQLite3 file pointer
 * @param buf where to read data to
 * @param len length to be read
 * @param offs file offset
 * @result SQLite error code
 */

static int
mem_read(sqlite3_file *file, void *buf, int len, sqlite_int64 offs)
{
    mem_file *mf = (mem_file *) file;
    mem_blk *mb = mf->mb;
    int rc = SQLITE_IOERR_READ;

#ifdef linux
    if (mb) {
	sqlite3_mutex_enter(mb->mutex);
    }
#endif
    if (mb && (offs <= mb->length)) {
	rc = SQLITE_OK;
	if (offs + len > mb->length) {
	    rc = SQLITE_IOERR_SHORT_READ;
	    len = mb->length - offs;
	}
	memcpy(buf, mb->data + offs, len);
    }
#ifdef linux
    if (mb) {
	sqlite3_mutex_leave(mb->mutex);
    }
#endif
    return rc;
}

/**
 * Truncate mem_file.
 * @param file SQLite3 file pointer
 * @param offs file size
 * @result SQLite error code
 */

static int
#ifdef linux
mem_truncate_unlocked(sqlite3_file *file, sqlite_int64 offs)
#else
mem_truncate(sqlite3_file *file, sqlite_int64 offs)
#endif
{
#ifdef linux
    mem_file *mf = (mem_file *) file;
    mem_blk *mb = mf->mb;
    unsigned char *p;
    long psize = mb->psize;
    unsigned long length = offs;
    unsigned long size;
    
    size = length + 1;
    if ((psize > 0) && (size / psize == mb->size / psize)) {
	p = mb->data;
    } else {
	p = mremap(mb->data, mb->size, size, MREMAP_MAYMOVE);
    }
    if (p == MAP_FAILED) {
	return SQLITE_IOERR_TRUNCATE;
    }
    mb->size = size;
    mb->length = length;
    mb->data = p;
    return SQLITE_OK;
#else
    return SQLITE_IOERR_TRUNCATE;
#endif
}

#ifdef linux
static int
mem_truncate(sqlite3_file *file, sqlite_int64 offs)
{
    mem_file *mf = (mem_file *) file;
    mem_blk *mb = mf->mb;
    int rc = SQLITE_IOERR_TRUNCATE;

    if (mb) {
	sqlite3_mutex_enter(mb->mutex);
	rc = mem_truncate_unlocked(file, offs);
	sqlite3_mutex_leave(mb->mutex);
    }
    return rc;
}
#endif

/**
 * Write data to mem_file.
 * @param file SQLite3 file pointer
 * @param buf what to write
 * @param len length to be written
 * @param offs file offset
 * @result SQLite error code
 */

static int
mem_write(sqlite3_file *file, const void *buf, int len, sqlite_int64 offs)
{
#ifdef linux
    mem_file *mf = (mem_file *) file;
    mem_blk *mb = mf->mb;

    sqlite3_mutex_enter(mb->mutex);
    if (offs + len > mb->length) {
	if (mem_truncate_unlocked(file, offs + len) != SQLITE_OK) {
	    sqlite3_mutex_leave(mb->mutex);
	    return SQLITE_IOERR_WRITE;
	}
    }
    memcpy(mb->data + offs, buf, len);
    sqlite3_mutex_leave(mb->mutex);
    return SQLITE_OK;
#else
    return SQLITE_IOERR_WRITE;
#endif
}

/**
 * Sync mem_file.
 * @param file SQLite3 file pointer
 * @param flags sync flags
 * @result SQLite error code
 */

static int
mem_sync(sqlite3_file *file, int flags)
{
#ifdef linux
    return SQLITE_OK;
#else
    return SQLITE_IOERR_FSYNC;
#endif
}

/**
 * Report file size of mem_file.
 * @param file SQLite3 file pointer
 * @param size value return
 * @result SQLite error code
 */

static int
mem_filesize(sqlite3_file *file, sqlite_int64 *size)
{
    mem_file *mf = (mem_file *) file;
    mem_blk *mb = mf->mb;

    if (mb) {
#ifdef linux
	sqlite3_mutex_enter(mb->mutex);
#endif
	*size = mb->length;
#ifdef linux
	sqlite3_mutex_leave(mb->mutex);
#endif
	return SQLITE_OK;
    }
    return SQLITE_IOERR_FSTAT;
}

/**
 * Lock mem_file.
 * @param file SQLite3 file pointer
 * @param lck new lock status
 * @result SQLite error code, always SQLITE_OK
 */

static int
mem_lock(sqlite3_file *file, int lck)
{
#ifdef linux
    mem_file *mf = (mem_file *) file;
    mem_blk *mb = mf->mb;
    int rc = SQLITE_IOERR_LOCK;

    if (mb) {
	sqlite3_mutex_enter(mb->mutex);
	if (lck > 0) {
	    rc = SQLITE_BUSY;
	    if ((mf->lock == 0) && (mb->lcnt == 0)) {
		mb->lcnt = 1;
		mf->lock = lck;
		rc = SQLITE_OK;
	    } else if ((mf->lock > 0) && (mb->lcnt == 1)) {
		mf->lock = lck;
		rc = SQLITE_OK;
	    }
	}
	sqlite3_mutex_leave(mb->mutex);
    }
    return rc;
#else
    return SQLITE_OK;
#endif
}

/**
 * Unlock mem_file.
 * @param file SQLite3 file pointer
 * @param lck new lock status
 * @result SQLite error code, always SQLITE_OK
 */

static int
mem_unlock(sqlite3_file *file, int lck)
{
#ifdef linux
    mem_file *mf = (mem_file *) file;
    mem_blk *mb = mf->mb;
    int rc = SQLITE_IOERR_UNLOCK;

    if (mb) {
	sqlite3_mutex_enter(mb->mutex);
	if (mf->lock == lck) {
	    rc = SQLITE_OK;
	} else if (lck == 0) {
	    if (mf->lock) {
		mb->lcnt = 0;
		mf->lock = 0;
	    }
	    rc = SQLITE_OK;
	} else if ((lck < mf->lock) && (mb->lcnt != 0)) {
	    mf->lock = lck;
	    rc = SQLITE_OK;
	}
	sqlite3_mutex_leave(mb->mutex);
    }
    return rc;
#else
    return SQLITE_OK;
#endif
}

/**
 * Check lock state of mem_file.
 * @param file SQLite3 file pointer
 * @param out current lock status
 * @result SQLite error code, always SQLITE_OK
 */

static int
mem_checkreservedlock(sqlite3_file *file, int *out)
{
#ifdef linux
    mem_file *mf = (mem_file *) file;
    mem_blk *mb = mf->mb;
    int rc = SQLITE_IOERR_CHECKRESERVEDLOCK;

    if (mb) {
	sqlite3_mutex_enter(mb->mutex);
	*out = mf->lock >= 2;
	sqlite3_mutex_leave(mb->mutex);
	rc = SQLITE_OK;
    } else {
	*out = 0;
    }
    return rc;
#else
    *out = 0;
    return SQLITE_OK;
#endif
}

/**
 * File control operation on mem_file.
 * @param file SQLite3 file pointer
 * @param op operation code
 * @param arg argument for operation
 * @result SQLite error code, always SQLITE_OK
 */

static int
mem_filecontrol(sqlite3_file *file, int op, void *arg)
{
#ifdef SQLITE_FCNTL_PRAGMA
    if (op == SQLITE_FCNTL_PRAGMA) {
	return SQLITE_NOTFOUND;
    }
#endif
    return SQLITE_OK;
}

/**
 * Report sector size of mem_file.
 * @param file SQLite3 file pointer
 * @result always 4096
 */

static int
mem_sectorsize(sqlite3_file *file)
{
    return 4096;
}

/**
 * Device characteristics of mem_file.
 * @param file SQLite3 file pointer
 * @result always 0
 */

static int
mem_devicecharacteristics(sqlite3_file *file)
{
    return 0;
}

/**
 * I/O method structure of mem_file.
 */

static sqlite3_io_methods mem_methods = {
    1,				/* iVersion */
    mem_close,			/* xClose */
    mem_read,			/* xRead */
    mem_write,			/* xWrite */
    mem_truncate,		/* xTruncate */
    mem_sync,			/* xSync */
    mem_filesize,		/* xFileSize */
    mem_lock,			/* xLock */
    mem_unlock,			/* xUnlock */
    mem_checkreservedlock,	/* xCheckReservedLock */
    mem_filecontrol,		/* xFileControl */
    mem_sectorsize,		/* xSectorSize */
    mem_devicecharacteristics	/* xDeviceCharacteristics */
};

/**
 * Open mem_file given file name in mem_vfs.
 * @param vfs SQLite VFS
 * @param name file name
 * @param file SQLite3 file pointer to be filled
 * @param flags open flags
 * @param outflags value return of open flags
 * @result SQLite error code
 */

static int
mem_open(sqlite3_vfs *vfs, const char *name, sqlite3_file *file,
	 int flags, int *outflags)
{
    mem_file *mf = (mem_file *) file;
    mem_blk *mb = 0;
#ifdef _WIN64
    unsigned long long t = 0;
#else
    unsigned long t = 0;
#endif
#if !defined(_WIN32) && !defined(_WIN64)
    mem_blk mb0;
    int pfd[2];
    int n;
#endif

    if (!name) {
	return SQLITE_IOERR;
    }
    if (flags & (SQLITE_OPEN_MAIN_JOURNAL |
		 SQLITE_OPEN_WAL |
#ifndef linux
		 SQLITE_OPEN_READWRITE |
#endif
		 SQLITE_OPEN_CREATE)) {
	return SQLITE_CANTOPEN;
    }
#ifdef _WIN64
    sscanf(name + 1, "%I64x", &t);
#else
    t = strtoul(name + 1, 0, 16);
#endif
    mb = (mem_blk *) t;
    if (!mb) {
	return SQLITE_CANTOPEN;
    }
#if !defined(_WIN32) && !defined(_WIN64)
    if (pipe(pfd) < 0) {
	return SQLITE_CANTOPEN;
    }
    n = (write(pfd[1], (char *) mb, sizeof (mem_blk)) < 0) ? errno : 0;
    if (n == EFAULT) {
cantopen:
	close(pfd[0]);
	close(pfd[1]);
	return SQLITE_CANTOPEN;
    }
    n = read(pfd[0], (char *) &mb0, sizeof (mem_blk));
    if (n != sizeof (mem_blk)) {
	goto cantopen;
    }
    if (memcmp(mb0.magic, MEM_MAGIC, 4) == 0) {
#ifdef linux
	n = (write(pfd[1], (char *) mb0.data, 1) < 0) ? errno : 0;
	if (n == EFAULT) {
	    goto cantopen;
	}
#endif
	if (mb0.length > 0) {
	    n = (write(pfd[1], (char *) mb0.data + mb0.length - 1, 1) < 0)
	      ? errno : 0;
	    if (n == EFAULT) {
		goto cantopen;
	    }
	}
	close(pfd[0]);
	close(pfd[1]);
#ifdef linux
	sqlite3_mutex_enter(mb->mutex);
#endif
	mb->opened++;
#ifdef linux
	sqlite3_mutex_leave(mb->mutex);
#endif
    } else {
	goto cantopen;
    }
#else
    if (memcmp(mb->magic, MEM_MAGIC, 4) == 0) {
	mb->opened++;
    } else {
	return SQLITE_CANTOPEN;
    }
#endif
    memset(mf, 0, sizeof (mem_file));
    mf->mb = mb;
    mf->base.pMethods = &mem_methods;
    if (outflags) {
	*outflags = flags;
    }
    return SQLITE_OK;
}

/**
 * Delete mem_vfs file given name.
 * @param vfs SQLite VFS
 * @param name file name
 * @param sync perform sync before deletion
 * @result SQLite error code, always SQLITE_IOERR_DELETE
 */

static int
mem_delete(sqlite3_vfs *vfs, const char *name, int sync)
{
    return SQLITE_IOERR_DELETE;
}

/**
 * Test mem_vfs file access given name and flags
 * @param vfs SQLite VFS
 * @param name file name
 * @param flags access to be tested
 * @param outflags value return of tested access modes
 * @result SQLite error code
 */

static int
mem_access(sqlite3_vfs *vfs, const char *name, int flags, int *outflags)
{
    char *endp = 0;
    unsigned long t;

    t = strtol(name + 1, &endp, 16);
    if ((t == 0) ||
#ifndef linux
	(flags == SQLITE_ACCESS_READWRITE) ||
#endif
	!endp || endp[0]) {
	*outflags = 0;
    } else {
	*outflags = 1;
    }
    return SQLITE_OK;
}

/**
 * Return full pathname on mem_vfs given name.
 * @param vfs SQLite VFS
 * @param name file name
 * @param len length of output buffer
 * @param out output buffer
 * @result SQLite error code, always SQLITE_OK
 */

static int
mem_fullpathname(sqlite3_vfs *vfs, const char *name, int len, char *out)
{
    sqlite3_snprintf(len, out, "%s", name);
    out[len - 1] = '\0';
    return SQLITE_OK;
}

/**
 * Open shared library on mem_vfs given name.
 * @param vfs SQLite VFS
 * @param name file name
 * @result handle, always 0
 */

static void *
mem_dlopen(sqlite3_vfs *vfs, const char *name)
{
    return 0;
}

/**
 * Report last error of shared library operation on mem_vfs.
 * @param vfs SQLite VFS
 * @param len length of output buffer
 * @param out output buffer
 */

static void
mem_dlerror(sqlite3_vfs *vfs, int len, char *out)
{
    sqlite3_snprintf(len, out, "Loadable extensions are not supported");
    out[len - 1] = '\0';
}

/**
 * Lookup symbol in shared library on mem_vfs given name.
 * @param vfs SQLite VFS
 * @param handle shared library handle
 * @param sym symbol name
 * @result symbol address, always 0
 */

static void
(*mem_dlsym(sqlite3_vfs *vfs, void *handle, const char *sym))(void)
{
    return 0;
}

/**
 * Close shared library on mem_vfs given handle.
 * @param vfs SQLite VFS
 * @param handle shared library handle
 */

static void
mem_dlclose(sqlite3_vfs *vfs, void *handle)
{
}

/**
 * Return buffer filled with random bytes
 * @param vfs SQLite VFS
 * @param len length of output buffer
 * @param out output buffer
 * @result SQLite error code
 */

static int
mem_randomness(sqlite3_vfs *vfs, int len, char *out)
{
    sqlite3_vfs *ovfs = (sqlite3_vfs *) vfs->pAppData;

    return ovfs->xRandomness(ovfs, len, out);
}

/**
 * Sleep for given number of microseconds.
 * @param vfs SQLite VFS
 * @param micro microseconds to sleep
 * @result SQLite error code
 */

static int
mem_sleep(sqlite3_vfs *vfs, int micro)
{
    sqlite3_vfs *ovfs = (sqlite3_vfs *) vfs->pAppData;

    return ovfs->xSleep(ovfs, micro);
}

/**
 * Return current time.
 * @param vfs SQLite VFS
 * @param out output buffer
 * @result SQLite error code
 */

static int
mem_currenttime(sqlite3_vfs *vfs, double *out)
{
    sqlite3_vfs *ovfs = (sqlite3_vfs *) vfs->pAppData;

    return ovfs->xCurrentTime(ovfs, out);
}

/**
 * VFS structure of mem_vfs
 */

static sqlite3_vfs mem_vfs = {
    1,			/* iVersion */
    sizeof (mem_file),	/* szOsFile */
    256,		/* mxPathname */
    0,			/* pNext */
    mem_vfs_name,	/* zName */
    0,			/* pAppData */
    mem_open,		/* xOpen */
    mem_delete,		/* xDelete */
    mem_access,		/* xAccess */
    mem_fullpathname,	/* xFullPathname */
    mem_dlopen,		/* xDlOpen */
    mem_dlerror,	/* xDlError */
    mem_dlsym,		/* xDlSym */
    mem_dlclose,	/* xDlClose */
    mem_randomness,	/* xDlError */
    mem_sleep,		/* xDlSym */
    mem_currenttime	/* xDlClose */
};

/**
 * Attach (read-only) embedded SQLite database given blob.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 *
 * Arguments:
 *
 *   BLOB   -  the BLOB containing the embedded SQLite database<br>
 *   dbname -  the name of the attached database<br>
 *
 * Function result:
 *
 *   NULL - attached r/o database<br>
 *   URI string - attached database<br>
 *   all else - error occurred<br>
 *
 * Example:
 *
 *   CREATE VIRTUAL TABLE Z USING ZIPFILE('zipfile.zip');<br>
 *   SELECT blob_attach(data, 'ZDB') FROM Z WHERE PATH = 'embedded.db';<br>
 *   DROP VIRTUAL TABLE Z;<br>
 */

static void
blob_attach_func(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    unsigned long length;
    const unsigned char *data;
    mem_blk *mb = 0;
    char *sql = 0;
    int sqllen = 0;
#ifdef linux
    int isrw = 0;
#endif

    if (argc != 2) {
	sqlite3_result_error(ctx, "need two arguments", -1);
	return;
    }
    data = (const unsigned char *) sqlite3_value_blob(argv[0]);
    length = sqlite3_value_bytes(argv[0]);
    if (!data || !length) {
	sqlite3_result_error(ctx, "empty blob", -1);
	return;
    }
    mb = mem_createmb(data, length);
    if (!mb) {
	sqlite3_result_error(ctx, "cannot map blob", -1);
	return;
    }
    sql = sqlite3_mprintf("ATTACH "
#ifdef _WIN64
			  "'file:/%llX"
#else
			  "'file:/%lX"
#endif
			  "?vfs=%s&"
#ifdef linux
			  "mode=rw&"
#else
			  "mode=ro&"
#endif
			  "cache=private' AS %Q",
#ifdef _WIN64
			  (unsigned long long) mb,
#else
			  (unsigned long) mb,
#endif
			  mem_vfs_name,
			  (char *) sqlite3_value_text(argv[1]));
    if (!sql) {
	sqlite3_result_error(ctx, "cannot map blob", -1);
	mem_destroymb(mb);
	return;
    }
#ifdef linux
    sqlite3_mutex_leave(mb->mutex);
#endif
    if (sqlite3_exec(sqlite3_context_db_handle(ctx), sql, 0, 0, 0)
	!= SQLITE_OK) {
	sqlite3_free(sql);
	sqlite3_result_error(ctx, "cannot attach blob", -1);
#ifdef linux
	sqlite3_mutex_enter(mb->mutex);
#endif
	mem_destroymb(mb);
	return;
    }
    sqllen = strlen(sql);
    sqlite3_snprintf(sqllen, sql, "PRAGMA %Q.synchronous = OFF",
		     (char *) sqlite3_value_text(argv[1]));
    sqlite3_exec(sqlite3_context_db_handle(ctx), sql, 0, 0, 0);
#ifdef linux
    sqlite3_snprintf(sqllen, sql, "PRAGMA %Q.journal_mode = OFF",
		     (char *) sqlite3_value_text(argv[1]));
    if (sqlite3_exec(sqlite3_context_db_handle(ctx), sql, 0, 0, 0)
	== SQLITE_OK) {
	isrw = 1;
    }
#endif
#ifdef linux
    sqlite3_mutex_enter(mb->mutex);
#endif
    if (--mb->opened < 1) {
	sqlite3_snprintf(sqllen, sql, "DETACH %Q",
			 (char *) sqlite3_value_text(argv[1]));
	sqlite3_exec(sqlite3_context_db_handle(ctx), sql, 0, 0, 0);
	sqlite3_free(sql);
	sqlite3_result_error(ctx, "cannot attach blob", -1);
	mem_destroymb(mb);
	return;
    }
#ifdef linux
    sqlite3_mutex_leave(mb->mutex);
    if (isrw) {
	sqlite3_snprintf(sqllen, sql, 
			 "file:/%lX?vfs=%s&mode=rw&cache=private",
			 (unsigned long) mb, mem_vfs_name);
	sqlite3_result_text(ctx, sql, -1, sqlite3_free);
	return;
    }
#endif
    sqlite3_free(sql);
    sqlite3_result_null(ctx);
}

/**
 * Dump memory mapped (writable) database to blob.
 * @param ctx SQLite function context
 * @param argc number of arguments
 * @param argv argument vector
 *
 * Arguments:
 *
 *   ADDR   -  the memory mapped database<br>
 *
 * Function result:
 *
 *   blob - success<br>
 *   all else - error occurred<br>
 *
 */

static void
blob_dump_func(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    char *uri, vfs[64];
#ifdef _WIN64
    unsigned long long addr = 0;
#else
    unsigned long addr = 0;
#endif
#ifdef linux
    int pfd[2], n;
    mem_blk *mb;
#endif

    if (argc != 1) {
	sqlite3_result_error(ctx, "need one argument", -1);
	return;
    }
    uri = (char *) sqlite3_value_text(argv[0]);
    vfs[0] = '\0';
    if (!uri || (sscanf(uri,
#ifdef _WIN64
			"file:/%I64X?vfs=%63[^&]",
#else
			"file:/%lX?vfs=%63[^&]",
#endif
			&addr, vfs) != 2)) {
inval:
	sqlite3_result_error(ctx, "invalid object", -1);
	return;
    }
    vfs[63] = '\0';
    if ((strcmp(mem_vfs_name, vfs) != 0) || (addr == 0)) {
	goto inval;
    }
#ifdef linux
    if (pipe(pfd) < 0) {
	goto inval;
    }
    n = (write(pfd[1], (char *) addr, 1) < 0) ? errno : 0;
    close(pfd[0]);
    close(pfd[1]);
    if (n == EFAULT) {
	goto inval;
    }
    mb = (mem_blk *) addr;
    if (memcmp(mb->magic, MEM_MAGIC, 4) != 0) {
	goto inval;
    }
    sqlite3_mutex_enter(mb->mutex);
    sqlite3_result_blob(ctx, mb->data, mb->length, SQLITE_STATIC);
    sqlite3_mutex_leave(mb->mutex);
#else
    sqlite3_result_error(ctx, "unsupported function", -1);
#endif
}

#endif /* SQLITE_OPEN_URI */

/**
 * Module initializer creating SQLite module and functions.
 * @param db database pointer
 * @result SQLite error code
 */

#ifndef STANDALONE
static
#endif
int
zip_vtab_init(sqlite3 *db)
{
    sqlite3_create_function(db, "crc32", 1, SQLITE_UTF8,
			    (void *) db, zip_crc32_func, 0, 0);
    sqlite3_create_function(db, "inflate", 1, SQLITE_UTF8,
			    (void *) db, zip_inflate_func, 0, 0);
    sqlite3_create_function(db, "deflate", 1, SQLITE_UTF8,
			    (void *) db, zip_deflate_func, 0, 0);
    sqlite3_create_function(db, "uncompress", 1, SQLITE_UTF8,
			    (void *) db, zip_inflate_func, 0, 0);
    sqlite3_create_function(db, "compress", -1, SQLITE_UTF8,
			    (void *) db, zip_compress_func, 0, 0);
#ifdef SQLITE_OPEN_URI
    if (!mem_vfs.pAppData) {
	sqlite3_vfs *parent = sqlite3_vfs_find(0);

	if (parent) {
	    sqlite3_snprintf(sizeof (mem_vfs_name), mem_vfs_name,
#ifdef _WIN64
			     "mem_vfs_%llX", (unsigned long long) &mem_vfs
#else
			     "mem_vfs_%lX", (unsigned long) &mem_vfs
#endif
			    );
	    if (sqlite3_vfs_register(&mem_vfs, 0) == SQLITE_OK) {
		mem_vfs.pAppData = (void *) parent;
	    }
	}
    }
    if (mem_vfs.pAppData) {
	sqlite3_create_function(db, "blob_attach", 2, SQLITE_UTF8,
				(void *) db, blob_attach_func, 0, 0);
	sqlite3_create_function(db, "blob_dump", 1, SQLITE_UTF8,
				(void *) db, blob_dump_func, 0, 0);
    }
#endif
    return sqlite3_create_module(db, "zipfile", &zip_vtab_mod, 0);
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
    return zip_vtab_init(db);
}

#endif
