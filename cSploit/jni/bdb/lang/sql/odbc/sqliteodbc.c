/**
 * @file sqliteodbc.c
 * SQLite ODBC Driver main module.
 *
 * $Id: sqliteodbc.c,v 1.205 2013/02/27 06:36:58 chw Exp chw $
 *
 * Copyright (c) 2001-2013 Christian Werner <chw@ch-werner.de>
 * OS/2 Port Copyright (c) 2004 Lorne R. Sunley <lsunley@mb.sympatico.ca>
 *
 * See the file "license.terms" for information on usage
 * and redistribution of this file and for a
 * DISCLAIMER OF ALL WARRANTIES.
 */

#include "sqliteodbc.h"

#ifdef  SQLITE_UTF8
#ifndef WITHOUT_WINTERFACE
#define WINTERFACE
#define WCHARSUPPORT
#endif
#endif

#ifdef SQLITE_UTF8
#if !defined(_WIN32) && !defined(_WIN64)
#if !defined(WCHARSUPPORT) && defined(HAVE_SQLWCHAR) && (HAVE_SQLWCHAR)
#define WCHARSUPPORT
#endif
#endif
#endif

#if defined(WINTERFACE)
#include <sqlucode.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#include "resource.h"
#define ODBC_INI "ODBC.INI"
#ifndef DRIVER_VER_INFO
#define DRIVER_VER_INFO VERSION
#endif
#else
#ifdef __OS2__
#define ODBC_INI "ODBC.INI"
#else
#define ODBC_INI ".odbc.ini"
#endif
#endif

#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 3)))
#define	CANT_PASS_VALIST_AS_CHARPTR 1
#endif

#ifdef _WIN64
#undef  CANT_PASS_VALIST_AS_CHARPTR
#define CANT_PASS_VALIST_AS_CHARPTR 1
#endif

#ifdef CANT_PASS_VALIST_AS_CHARPTR
#define MAX_PARAMS_FOR_VPRINTF 32
#endif

#ifndef DRIVER_VER_INFO
#define DRIVER_VER_INFO "0.0"
#endif

#ifndef COLATTRIBUTE_LAST_ARG_TYPE
#ifdef _WIN64
#define COLATTRIBUTE_LAST_ARG_TYPE SQLLEN *
#else
#define COLATTRIBUTE_LAST_ARG_TYPE SQLPOINTER
#endif
#endif

#ifndef SETSTMTOPTION_LAST_ARG_TYPE
#define SETSTMTOPTION_LAST_ARG_TYPE SQLROWCOUNT
#endif

#undef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#undef max
#define max(a, b) ((a) < (b) ? (b) : (a))

#ifndef PTRDIFF_T
#define PTRDIFF_T int
#endif

#define array_size(x) (sizeof (x) / sizeof (x[0]))

#define stringify1(s) #s
#define stringify(s) stringify1(s)

#define verinfo(maj, min, lev) ((maj) << 16 | (min) << 8 | (lev))

/* Column types for static string column descriptions (SQLTables etc.) */

#if defined(WINTERFACE) && !defined(_WIN32) && !defined(_WIN64)
#define SCOL_VARCHAR SQL_WVARCHAR
#define SCOL_CHAR SQL_WCHAR
#else
#define SCOL_VARCHAR SQL_VARCHAR
#define SCOL_CHAR SQL_CHAR
#endif

#define ENV_MAGIC  0x53544145
#define DBC_MAGIC  0x53544144
#define DEAD_MAGIC 0xdeadbeef

#ifdef MEMORY_DEBUG

static void *
xmalloc_(int n, char *file, int line)
{
    int nn = n + 4 * sizeof (long);
    long *p;

    p = malloc(nn);
    if (!p) {
#if (MEMORY_DEBUG > 1)
	fprintf(stderr, "malloc\t%d\tNULL\t%s:%d\n", n, file, line);
#endif
	return NULL;
    }
    p[0] = 0xdead1234;
    nn = nn / sizeof (long) - 1;
    p[1] = n;
    p[nn] = 0xdead5678;
#if (MEMORY_DEBUG > 1)
    fprintf(stderr, "malloc\t%d\t%p\t%s:%d\n", n, &p[2], file, line);
#endif
    return (void *) &p[2];
}

static void *
xrealloc_(void *old, int n, char *file, int line)
{
    int nn = n + 4 * sizeof (long), nnn;
    long *p, *pp;

    if (n == 0 || !old) {
	return xmalloc_(n, file, line);
    }
    p = &((long *) old)[-2];
    if (p[0] != 0xdead1234) {
	fprintf(stderr, "*** low end corruption @ %p\n", old);
	abort();
    }
    nnn = p[1] + 4 * sizeof (long);
    nnn = nnn / sizeof (long) - 1;
    if (p[nnn] != 0xdead5678) {
	fprintf(stderr, "*** high end corruption @ %p\n", old);
	abort();
    }
    pp = realloc(p, nn);
    if (!pp) {
#if (MEMORY_DEBUG > 1)
	fprintf(stderr, "realloc\t%p,%d\tNULL\t%s:%d\n", old, n, file, line);
#endif
	return NULL;
    }
#if (MEMORY_DEBUG > 1)
    fprintf(stderr, "realloc\t%p,%d\t%p\t%s:%d\n", old, n, &pp[2], file, line);
#endif
    p = pp;
    p[1] = n;
    nn = nn / sizeof (long) - 1;
    p[nn] = 0xdead5678;
    return (void *) &p[2];
}

static void
xfree_(void *x, char *file, int line)
{
    long *p;
    int n;

    if (!x) {
	return;
    }
    p = &((long *) x)[-2];
    if (p[0] != 0xdead1234) {
	fprintf(stderr, "*** low end corruption @ %p\n", x);
	abort();
    }
    n = p[1] + 4 * sizeof (long);
    n = n / sizeof (long) - 1;
    if (p[n] != 0xdead5678) {
	fprintf(stderr, "*** high end corruption @ %p\n", x);
	abort();
    }
#if (MEMORY_DEBUG > 1)
    fprintf(stderr, "free\t%p\t\t%s:%d\n", x, file, line);
#endif
    free(p);
}

static void
xfree__(void *x)
{
    xfree_(x, "unknown location", 0);
}

static char *
xstrdup_(const char *str, char *file, int line)
{
    char *p;

    if (!str) {
#if (MEMORY_DEBUG > 1)
	fprintf(stderr, "strdup\tNULL\tNULL\t%s:%d\n", file, line);
#endif
	return NULL;
    }
    p = xmalloc_(strlen(str) + 1, file, line);
    if (p) {
	strcpy(p, str);
    }
#if (MEMORY_DEBUG > 1)
    fprintf(stderr, "strdup\t%p\t%p\t%s:%d\n", str, p, file, line);
#endif
    return p;
}

#define xmalloc(x)    xmalloc_(x, __FILE__, __LINE__)
#define xrealloc(x,y) xrealloc_(x, y, __FILE__, __LINE__)
#define xfree(x)      xfree_(x, __FILE__, __LINE__)
#define xstrdup(x)    xstrdup_(x, __FILE__, __LINE__)

#else

#define xmalloc(x)    malloc(x)
#define xrealloc(x,y) realloc(x, y)
#define xfree(x)      free(x)
#define xstrdup(x)    strdup_(x)

#endif

#if defined(_WIN32) || defined(_WIN64)

#define vsnprintf   _vsnprintf
#define snprintf    _snprintf
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp

static HINSTANCE NEAR hModule;	/* Saved module handle for resources */

#endif

#if defined(_WIN32) || defined(_WIN64)

/*
 * SQLHENV, SQLHDBC, and SQLHSTMT synchronization
 * is done using a critical section in ENV structure.
 */

#define HDBC_LOCK(hdbc)				\
{						\
    DBC *d;					\
						\
    if ((hdbc) == SQL_NULL_HDBC) {		\
	return SQL_INVALID_HANDLE;		\
    }						\
    d = (DBC *) (hdbc);				\
    if (d->magic != DBC_MAGIC || !d->env) {	\
	return SQL_INVALID_HANDLE;		\
    }						\
    if (d->env->magic != ENV_MAGIC) {		\
	return SQL_INVALID_HANDLE;		\
    }						\
    EnterCriticalSection(&d->env->cs);		\
    d->env->owner = GetCurrentThreadId();	\
}

#define HDBC_UNLOCK(hdbc)			\
    if ((hdbc) != SQL_NULL_HDBC) {		\
	DBC *d;					\
						\
	d = (DBC *) (hdbc);			\
	if (d->magic == DBC_MAGIC && d->env &&	\
	    d->env->magic == ENV_MAGIC) {	\
	    d->env->owner = 0;			\
	    LeaveCriticalSection(&d->env->cs);	\
	}					\
    }

#define HSTMT_LOCK(hstmt)			\
{						\
    DBC *d;					\
						\
    if ((hstmt) == SQL_NULL_HSTMT) {		\
	return SQL_INVALID_HANDLE;		\
    }						\
    d = (DBC *) ((STMT *) (hstmt))->dbc;	\
    if (d->magic != DBC_MAGIC || !d->env) {	\
	return SQL_INVALID_HANDLE;		\
    }						\
    if (d->env->magic != ENV_MAGIC) {		\
	return SQL_INVALID_HANDLE;		\
    }						\
    EnterCriticalSection(&d->env->cs);		\
    d->env->owner = GetCurrentThreadId();	\
}

#define HSTMT_UNLOCK(hstmt)			\
    if ((hstmt) != SQL_NULL_HSTMT) {		\
	DBC *d;					\
						\
	d = (DBC *) ((STMT *) (hstmt))->dbc;	\
	if (d->magic == DBC_MAGIC && d->env &&	\
	    d->env->magic == ENV_MAGIC) {	\
	    d->env->owner = 0;			\
	    LeaveCriticalSection(&d->env->cs);	\
	}					\
    }

#else

/*
 * On UN*X assume that we are single-threaded or
 * the driver manager provides serialization for us.
 *
 * In iODBC (3.52.x) serialization can be turned
 * on using the DSN property "ThreadManager=yes".
 *
 * In unixODBC that property is named
 * "Threading=0-3" and takes one of these values:
 *
 *   0 - no protection
 *   1 - statement level protection
 *   2 - connection level protection
 *   3 - environment level protection
 *
 * unixODBC 2.2.11 uses environment level protection
 * by default when it has been built with pthread
 * support.
 */

#define HDBC_LOCK(hdbc)
#define HDBC_UNLOCK(hdbc)
#define HSTMT_LOCK(hdbc)
#define HSTMT_UNLOCK(hdbc)

#endif

/*
 * tolower() replacement w/o locale
 */

static const char upper_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char lower_chars[] = "abcdefghijklmnopqrstuvwxyz";

static int
TOLOWER(int c)
{
    if (c) {
	char *p = strchr(upper_chars, c);

	if (p) {
	    c = lower_chars[p - upper_chars];
	}
    }
    return c;
}

/*
 * isdigit() replacement w/o ctype.h
 */

static const char digit_chars[] = "0123456789";

#define ISDIGIT(c) \
    ((c) && strchr(digit_chars, (c)) != NULL)

/*
 * isspace() replacement w/o ctype.h
 */

static const char space_chars[] = " \f\n\r\t\v";

#define ISSPACE(c) \
    ((c) && strchr(space_chars, (c)) != NULL)

/*
 * Characters in named parameters
 */

static const char id_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			       "abcdefghijklmnopqrstuvwxyz"
			       "_0123456789";

#define ISIDCHAR(c) \
    ((c) && strchr(id_chars, (c)) != NULL)

/*
 * Forward declarations of static functions.
 */

static void freedyncols(STMT *s);
static void freeresult(STMT *s, int clrcols);
static void unbindcols(STMT *s);

static SQLRETURN drvexecute(SQLHSTMT stmt, int initial);
static SQLRETURN freestmt(HSTMT stmt);
static SQLRETURN mkbindcols(STMT *s, int ncols);
static SQLRETURN setupparbuf(STMT *s, BINDPARM *p);
static SQLRETURN starttran(STMT *s);
static SQLRETURN substparam(STMT *s, int pnum, char **outp);

#if (defined(_WIN32) || defined(_WIN64)) && defined(WINTERFACE)
/* MS Access hack part 1 (reserved error -7748) */
static COL *statSpec2P, *statSpec3P;
#endif

#if (MEMORY_DEBUG < 1)
/**
 * Duplicate string using xmalloc().
 * @param str string to be duplicated
 * @result pointer to new string or NULL
 */

static char *
strdup_(const char *str)
{
    char *p = NULL;

    if (str) {
	p = xmalloc(strlen(str) + 1);
	if (p) {
	    strcpy(p, str);
	}
    }
    return p;
}
#endif

#ifdef WCHARSUPPORT
/**
 * Return length of UNICODE string.
 * @param str UNICODE string
 * @result length of string in characters
 */

static int
uc_strlen(SQLWCHAR *str)
{
    int len = 0;

    if (str) {
	while (*str) {
	    ++len;
	    ++str;
	}
    }
    return len;
}

/**
 * Copy UNICODE string like strncpy().
 * @param dest destination area
 * @param src source area
 * @param len length of source area in characters
 * @return pointer to destination area
 */

static SQLWCHAR *
uc_strncpy(SQLWCHAR *dest, SQLWCHAR *src, int len)
{
    int i = 0;

    while (i < len) {
	if (!src[i]) {
	    break;
	}
	dest[i] = src[i];
	++i;
    }
    if (i < len) {
	dest[i] = 0;
    }
    return dest;
}

/**
 * Make UNICODE string from UTF8 string into buffer.
 * @param str UTF8 string to be converted
 * @param len length in characters of str or -1
 * @param uc destination area to receive UNICODE string
 * @param ucLen byte length of destination area
 */

static void
uc_from_utf_buf(unsigned char *str, int len, SQLWCHAR *uc, int ucLen)
{
    ucLen = ucLen / sizeof (SQLWCHAR);
    if (!uc || ucLen < 0) {
	return;
    }
    if (len < 0) {
	len = ucLen * 5;
    }
    uc[0] = 0;
    if (str) {
	int i = 0;

	while (i < len && *str && i < ucLen) {
	    unsigned char c = str[0];

	    if (c < 0x80) {
		uc[i++] = c;
		++str;
	    } else if (c <= 0xc1 || c >= 0xf5) {
		/* illegal, ignored */
		++str;
	    } else if (c < 0xe0) {
		if ((str[1] & 0xc0) == 0x80) {
		    unsigned long t = ((c & 0x1f) << 6) | (str[1] & 0x3f);

		    uc[i++] = t;
		    str += 2;
		} else {
		    uc[i++] = c;
		    ++str;
		}
	    } else if (c < 0xf0) {
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80) {
		    unsigned long t = ((c & 0x0f) << 12) |
			((str[1] & 0x3f) << 6) | (str[2] & 0x3f);

		    uc[i++] = t;
		    str += 3;
		} else {
		    uc[i++] = c;
		    ++str;
		}
	    } else if (c < 0xf8) {
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 &&
		    (str[3] & 0xc0) == 0x80) {
		    unsigned long t = ((c & 0x03) << 18) |
			((str[1] & 0x3f) << 12) | ((str[2] & 0x3f) << 6) |
			(str[3] & 0x3f);

		    if (sizeof (SQLWCHAR) == 2 * sizeof (char) &&
			t >= 0x10000) {
			t -= 0x10000;
			uc[i++] = 0xd800 | ((t >> 10) & 0x3ff);
			if (i >= ucLen) {
			    break;
			}
			t = 0xdc00 | (t & 0x3ff);
		    }
		    uc[i++] = t;
		    str += 4;
		} else {
		    uc[i++] = c;
		    ++str;
		}
	    } else if (c < 0xfc) {
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 &&
		    (str[3] & 0xc0) == 0x80 && (str[4] & 0xc0) == 0x80) {
		    unsigned long t = ((c & 0x01) << 24) |
			((str[1] & 0x3f) << 18) | ((str[2] & 0x3f) << 12) |
			((str[3] & 0x3f) << 6) | (str[4] & 0x3f);

		    if (sizeof (SQLWCHAR) == 2 * sizeof (char) &&
			t >= 0x10000) {
			t -= 0x10000;
			uc[i++] = 0xd800 | ((t >> 10) & 0x3ff);
			if (i >= ucLen) {
			    break;
			}
			t = 0xdc00 | (t & 0x3ff);
		    }
		    uc[i++] = t;
		    str += 5;
		} else {
		    uc[i++] = c;
		    ++str;
		}
	    } else {
		/* ignore */
		++str;
	    }
	}
	if (i < ucLen) {
	    uc[i] = 0;
	}
    }
}

/**
 * Make UNICODE string from UTF8 string.
 * @param str UTF8 string to be converted
 * @param len length of UTF8 string
 * @return alloc'ed UNICODE string to be free'd by uc_free()
 */

static SQLWCHAR *
uc_from_utf(unsigned char *str, int len)
{
    SQLWCHAR *uc = NULL;
    int ucLen;

    if (str) {
	if (len == SQL_NTS) {
	    len = strlen((char *) str);
	}
	ucLen = sizeof (SQLWCHAR) * (len + 1);
	uc = xmalloc(ucLen);
	if (uc) {
	    uc_from_utf_buf(str, len, uc, ucLen);
	}
    }
    return uc;
}

/**
 * Make UTF8 string from UNICODE string.
 * @param str UNICODE string to be converted
 * @param len length of UNICODE string in bytes
 * @return alloc'ed UTF8 string to be free'd by uc_free()
 */

static char *
uc_to_utf(SQLWCHAR *str, int len)
{
    int i;
    char *cp, *ret = NULL;

    if (!str) {
	return ret;
    }
    if (len == SQL_NTS) {
	len = uc_strlen(str);
    } else {
	len = len / sizeof (SQLWCHAR);
    }
    cp = xmalloc(len * 6 + 1);
    if (!cp) {
	return ret;
    }
    ret = cp;
    for (i = 0; i < len; i++) {
	unsigned long c = str[i];

	if (sizeof (SQLWCHAR) == 2 * sizeof (char)) {
	    c &= 0xffff;
	}
	if (c < 0x80) {
	    *cp++ = c;
	} else if (c < 0x800) {
	    *cp++ = 0xc0 | ((c >> 6) & 0x1f);
	    *cp++ = 0x80 | (c & 0x3f);
	} else if (c < 0x10000) {
	    if (sizeof (SQLWCHAR) == 2 * sizeof (char) &&
		c >= 0xd800 && c <= 0xdbff && i + 1 < len) {
		unsigned long c2 = str[i + 1] & 0xffff;

		if (c2 >= 0xdc00 && c2 <= 0xdfff) {
		    c = (((c & 0x3ff) << 10) | (c2 & 0x3ff)) + 0x10000;
		    *cp++ = 0xf0 | ((c >> 18) & 0x07);
		    *cp++ = 0x80 | ((c >> 12) & 0x3f);
		    *cp++ = 0x80 | ((c >> 6) & 0x3f);
		    *cp++ = 0x80 | (c & 0x3f);
		    ++i;
		    continue;
		}
	    }
	    *cp++ = 0xe0 | ((c >> 12) & 0x0f);
	    *cp++ = 0x80 | ((c >> 6) & 0x3f);
	    *cp++ = 0x80 | (c & 0x3f);
	} else if (c < 0x200000) {
	    *cp++ = 0xf0 | ((c >> 18) & 0x07);
	    *cp++ = 0x80 | ((c >> 12) & 0x3f);
	    *cp++ = 0x80 | ((c >> 6) & 0x3f);
	    *cp++ = 0x80 | (c & 0x3f);
	} else if (c < 0x4000000) {
	    *cp++ = 0xf8 | ((c >> 24) & 0x03);
	    *cp++ = 0x80 | ((c >> 18) & 0x3f);
	    *cp++ = 0x80 | ((c >> 12) & 0x3f);
	    *cp++ = 0x80 | ((c >> 6) & 0x3f);
	    *cp++ = 0x80 | (c & 0x3f);
	} else if (c < 0x80000000) {
	    *cp++ = 0xfc | ((c >> 31) & 0x01);
	    *cp++ = 0x80 | ((c >> 24) & 0x3f);
	    *cp++ = 0x80 | ((c >> 18) & 0x3f);
	    *cp++ = 0x80 | ((c >> 12) & 0x3f);
	    *cp++ = 0x80 | ((c >> 6) & 0x3f);
	    *cp++ = 0x80 | (c & 0x3f);
	}
    }
    *cp = '\0';
    return ret;
}

#endif

#ifdef WINTERFACE

/**
 * Make UTF8 string from UNICODE string.
 * @param str UNICODE string to be converted
 * @param len length of UNICODE string in characters
 * @return alloc'ed UTF8 string to be free'd by uc_free()
 */

static char *
uc_to_utf_c(SQLWCHAR *str, int len)
{
    if (len != SQL_NTS) {
	len = len * sizeof (SQLWCHAR);
    }
    return uc_to_utf(str, len);
}

#endif

#if defined(WINTERFACE) || defined(WCHARSUPPORT)

/**
 * Free converted UTF8 or UNICODE string.
 */

static void
uc_free(void *str)
{
    if (str) {
	xfree(str);
    }
}

#endif

#ifdef USE_DLOPEN_FOR_GPPS

#include <dlfcn.h>

#define SQLGetPrivateProfileString(A,B,C,D,E,F) drvgpps(d,A,B,C,D,E,F)

/*
 * EXPERIMENTAL: SQLGetPrivateProfileString infrastructure using
 * dlopen(), in theory this makes the driver independent from the
 * driver manager, i.e. the same driver binary can run with iODBC
 * and unixODBC.
 */

static void
drvgetgpps(DBC *d)
{
    void *lib;
    int (*gpps)();

    lib = dlopen("libodbcinst.so.1", RTLD_LAZY);
    if (!lib) {
	lib = dlopen("libodbcinst.so", RTLD_LAZY);
    }
    if (!lib) {
	lib = dlopen("libiodbcinst.so.2", RTLD_LAZY);
    }
    if (!lib) {
	lib = dlopen("libiodbcinst.so", RTLD_LAZY);
    }
    if (lib) {
	gpps = (int (*)()) dlsym(lib, "SQLGetPrivateProfileString");
	if (!gpps) {
	    dlclose(lib);
	    return;
	}
	d->instlib = lib;
	d->gpps = gpps;
    }
}

static void
drvrelgpps(DBC *d)
{
    if (d->instlib) {
	dlclose(d->instlib);
	d->instlib = 0;
    }
}

static int
drvgpps(DBC *d, char *sect, char *ent, char *def, char *buf,
	int bufsiz, char *fname)
{
    if (d->gpps) {
	return d->gpps(sect, ent, def, buf, bufsiz, fname);
    }
    strncpy(buf, def, bufsiz);
    buf[bufsiz - 1] = '\0';
    return 1;
}
#else
#include <odbcinst.h>
#define drvgetgpps(d)
#define drvrelgpps(d)
#endif

/**
 * Set error message and SQL state on DBC
 * @param d database connection pointer
 * @param naterr native error code
 * @param msg error message
 * @param st SQL state
 */

#if defined(__GNUC__) && (__GNUC__ >= 2)
static void setstatd(DBC *, int, char *, char *, ...)
    __attribute__((format (printf, 3, 5)));
#endif

static void
setstatd(DBC *d, int naterr, char *msg, char *st, ...)
{
    va_list ap;

    if (!d) {
	return;
    }
    d->naterr = naterr;
    d->logmsg[0] = '\0';
    if (msg) {
	int count;

	va_start(ap, st);
	count = vsnprintf((char *) d->logmsg, sizeof (d->logmsg), msg, ap);
	va_end(ap);
	if (count < 0) {
	    d->logmsg[sizeof (d->logmsg) - 1] = '\0';
	}
    }
    if (!st) {
	st = "?????";
    }
    strncpy(d->sqlstate, st, 5);
    d->sqlstate[5] = '\0';
}

/**
 * Set error message and SQL state on statement
 * @param s statement pointer
 * @param naterr native error code
 * @param msg error message
 * @param st SQL state
 */

#if defined(__GNUC__) && (__GNUC__ >= 2)
static void setstat(STMT *, int, char *, char *, ...)
    __attribute__((format (printf, 3, 5)));
#endif

static void
setstat(STMT *s, int naterr, char *msg, char *st, ...)
{
    va_list ap;

    if (!s) {
	return;
    }
    s->naterr = naterr;
    s->logmsg[0] = '\0';
    if (msg) {
	int count;

	va_start(ap, st);
	count = vsnprintf((char *) s->logmsg, sizeof (s->logmsg), msg, ap);
	va_end(ap);
	if (count < 0) {
	    s->logmsg[sizeof (s->logmsg) - 1] = '\0';
	}
    }
    if (!st) {
	st = "?????";
    }
    strncpy(s->sqlstate, st, 5);
    s->sqlstate[5] = '\0';
}

/**
 * Report IM001 (not implemented) SQL error code for HDBC.
 * @param dbc database connection handle
 * @result ODBC error code
 */

static SQLRETURN
drvunimpldbc(HDBC dbc)
{
    DBC *d;

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    d = (DBC *) dbc;
    setstatd(d, -1, "not supported", "IM001");
    return SQL_ERROR;
}

/**
 * Report IM001 (not implemented) SQL error code for HSTMT.
 * @param stmt statement handle
 * @result ODBC error code
 */

static SQLRETURN
drvunimplstmt(HSTMT stmt)
{
    STMT *s;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    setstat(s, -1, "not supported", "IM001");
    return SQL_ERROR;
}

/**
 * Free memory given pointer to memory pointer.
 * @param x pointer to pointer to memory to be free'd
 */

static void
freep(void *x)
{
    if (x && ((char **) x)[0]) {
	xfree(((char **) x)[0]);
	((char **) x)[0] = NULL;
    }
}

/**
 * Report S1000 (out of memory) SQL error given STMT.
 * @param s statement pointer
 * @result ODBC error code
 */

static SQLRETURN
nomem(STMT *s)
{
    setstat(s, -1, "out of memory", (*s->ov3) ? "HY000" : "S1000");
    return SQL_ERROR;
}

/**
 * Report S1000 (not connected) SQL error given STMT.
 * @param s statement pointer
 * @result ODBC error code
 */

static SQLRETURN
noconn(STMT *s)
{
    setstat(s, -1, "not connected", (*s->ov3) ? "HY000" : "S1000");
    return SQL_ERROR;
}

/**
 * Internal locale neutral strtod function.
 * @param data pointer to string
 * @param endp pointer for ending character
 * @result double value
 */

#if defined(HAVE_SQLITEATOF) && (HAVE_SQLITEATOF)

extern double sqliteAtoF(char *data, char **endp);

#define ln_strtod(A,B) sqliteAtoF(A,B)

#else

#if defined(HAVE_LOCALECONV) || defined(_WIN32) || defined(_WIN64)

static double
ln_strtod(const char *data, char **endp)
{
    static struct lconv *lc = 0;
    char buf[128], *p, *end;
    double value;

    if (!lc) {
	lc = localeconv();
    }
    if (lc && lc->decimal_point && lc->decimal_point[0] &&
	lc->decimal_point[0] != '.') {
	strncpy(buf, data, sizeof (buf) - 1);
	buf[sizeof (buf) - 1] = '\0';
	p = strchr(buf, '.');
	if (p) {
	    *p = lc->decimal_point[0];
	}
	p = buf;
    } else {
	p = (char *) data;
    }
    value = strtod(p, &end);
    end = (char *) data + (end - p);
    if (endp) {
	*endp = end;
    }
    return value;
}

#else

#define ln_strtod(A,B) strtod(A,B)

#endif

#endif

#if !defined(HAVE_SQLITEMPRINTF) || !(HAVE_SQLITEMPRINTF)
/**
 * Internal locale neutral sprintf("%g") function.
 * @param buf pointer to string
 * @param value double value
 */

static void
ln_sprintfg(char *buf, double value)
{
#if defined(HAVE_LOCALECONV) || defined(_WIN32) || defined(_WIN64)
    static struct lconv *lc = 0;
    char *p;

    sprintf(buf, "%.16g", value);
    if (!lc) {
	lc = localeconv();
    }
    if (lc && lc->decimal_point && lc->decimal_point[0] &&
	lc->decimal_point[0] != '.') {
	p = strchr(buf, lc->decimal_point[0]);
	if (p) {
	    *p = '.';
	}
    }
#else
    sprintf(buf, "%.16g", value);
#endif
}
#endif

/**
 * Strip quotes from quoted string in-place.
 * @param str string
 */

static char *
unquote(char *str)
{
    if (str) {
	int len = strlen(str);

	if (len > 1) {
	    if ((str[0] == '\'' && str[len - 1] == '\'') ||
		(str[0] == '"' && str[len - 1] == '"') ||
		(str[0] == '[' && str[len - 1] == ']')) {
		str[len - 1] = '\0';
		strcpy(str, str + 1);
	    }
	}
    }
    return str;
}

/**
 * Unescape search pattern for e.g. table name in
 * catalog functions. Replacements in string are done in-place.
 * @param str string
 * @result number of pattern characters in string or 0
 */

static int
unescpat(char *str)
{
    char *p, *q;
    int count = 0;

    p = str;
    while ((q = strchr(p, '_')) != NULL) {
	if (q == str || q[-1] != '\\') {
	    count++;
	}
	p = q + 1;
    }
    p = str;
    while ((q = strchr(p, '%')) != NULL) {
	if (q == str || q[-1] != '\\') {
	    count++;
	}
	p = q + 1;
    }
    p = str;
    while ((q = strchr(p, '\\')) != NULL) {
	if (q[1] == '\\' || q[1] == '_' || q[1] == '%') {
	    strcpy(q, q + 1);
	}
	p = q + 1;
    }
    return count;
}

/**
 * SQL LIKE string match with optional backslash escape handling.
 * @param str string
 * @param pat pattern
 * @param esc when true, treat literally "\\" as "\", "\%" as "%", "\_" as "_"
 * @result true when pattern matched
 */

static int
namematch(char *str, char *pat, int esc)
{
    int cp, ch;

    while (1) {
	cp = TOLOWER(*pat);
	if (cp == '\0') {
	    if (*str != '\0') {
		goto nomatch;
	    }
	    break;
	}
	if (*str == '\0' && cp != '%') {
	    goto nomatch;
	}
	if (cp == '%') {
	    while (*pat == '%') {
		++pat;
	    }
	    cp = TOLOWER(*pat);
	    if (cp == '\0') {
		break;
	    }
	    while (1) {
		if (cp != '_' && cp != '\\') {
		    while (*str) {
			ch = TOLOWER(*str);
			if (ch == cp) {
			    break;
			}
			++str;
		    }
		}
		if (namematch(str, pat, esc)) {
		    goto match;
		}
		if (*str == '\0') {
		    goto nomatch;
		}
		ch = TOLOWER(*str);
		++str;
	    }
	}
	if (cp == '_') {
	    pat++;
	    str++;
	    continue;
	}
	if (esc && cp == '\\' &&
	    (pat[1] == '\\' || pat[1] == '%' || pat[1] == '_')) {
	    ++pat;
	    cp = TOLOWER(*pat);
	}
	ch = TOLOWER(*str++);
	++pat;
	if (ch != cp) {
	    goto nomatch;
	}
    }
match:
    return 1;
nomatch:
    return 0;
}

/**
 * Busy callback for SQLite.
 * @param udata user data, pointer to DBC
 * @param table table name or NULL
 * @param count count of subsequenct calls
 * @result true of false
 */

static int
busy_handler(void *udata, const char *table, int count)
{
    DBC *d = (DBC *) udata;
    long t1;
    int ret = 0;
#if !defined(_WIN32) && !defined(_WIN64)
    struct timeval tv;
#ifdef HAVE_NANOSLEEP
    struct timespec ts;
#endif
#endif

    if (d->busyint) {
	d->busyint = 0;
	return ret;
    }
    if (d->timeout <= 0) {
	return ret;
    }
    if (count <= 1) {
#if defined(_WIN32) || defined(_WIN64)
	d->t0 = GetTickCount();
#else
	gettimeofday(&tv, NULL);
	d->t0 = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
    }
#if defined(_WIN32) || defined(_WIN64)
    t1 = GetTickCount();
#else
    gettimeofday(&tv, NULL);
    t1 = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
    if (t1 - d->t0 > d->timeout) {
	goto done;
    }
#if defined(_WIN32) || defined(_WIN64)
    Sleep(10);
#else
#ifdef HAVE_NANOSLEEP
    ts.tv_sec = 0;
    ts.tv_nsec = 10000000;
    do {
	ret = nanosleep(&ts, &ts);
	if (ret < 0 && errno != EINTR) {
	    ret = 0;
	}
    } while (ret);
#else
#ifdef HAVE_USLEEP
    usleep(10000);
#else
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    select(0, NULL, NULL, NULL, &tv);
#endif
#endif
#endif
    ret = 1;
done:
    return ret;
}

#ifdef HAVE_ENCDEC
static void hextobin_func(sqlite_func *context, int argc, const char **argv);
static void bintohex_func(sqlite_func *context, int argc, const char **argv);
#endif

static void time_func(sqlite_func *context, int argc, const char **argv);

/**
 * Set SQLite options (PRAGMAs) given SQLite handle.
 * @param x SQLite database handle
 * @param d DBC pointer
 * @result SQLite error code
 *
 * "full_column_names" is always turned on to get the table
 * names in column labels. "count_changes" is always turned
 * on to get the number of affected rows for INSERTs et.al.
 * "empty_result_callbacks" is always turned on to get
 * column labels even when no rows are returned.
 * Starting from SQLite 2.6.0 data types are reported for
 * callback functions when the "show_datatypes" pragma is
 * asserted.
 */

static int
setsqliteopts(sqlite *x, DBC *d)
{
    int count = 0, step = 0, rc;

    while (step < 4) {
	if (step < 1) {
	    rc = sqlite_exec(x, "PRAGMA full_column_names = on;",
			     NULL, NULL, NULL);
	} else if (step < 2) {
	    rc = sqlite_exec(x, "PRAGMA count_changes = on;",
			     NULL, NULL, NULL);
	} else if (step < 3) {
	    rc = sqlite_exec(x, "PRAGMA empty_result_callbacks = on;",
			     NULL, NULL, NULL);
	} else {
	    rc = sqlite_exec(x, "PRAGMA show_datatypes = on;",
			     NULL, NULL, NULL);
	}
	if (rc != SQLITE_OK) {
	    if (rc != SQLITE_BUSY ||
		!busy_handler((void *) d, NULL, ++count)) {
		return rc;
	    }
	    continue;
	}
	count = 0;
	++step;
    }
    sqlite_busy_handler(x, busy_handler, (void *) d);
#if (HAVE_ENCDEC)
    {
	char *fname;
       
	fname = "hextobin";
	sqlite_create_function(x, fname, 1, hextobin_func, 0);
	sqlite_function_type(x, fname, SQLITE_TEXT);
	fname = "bintohex";
	sqlite_create_function(x, fname, 1, bintohex_func, 0);
	sqlite_function_type(x, fname, SQLITE_TEXT);
    }
#endif
    {
	char *fname;

	fname = "current_time_local";
	sqlite_create_function(x, fname, 0, time_func, (void *) 0);
	sqlite_function_type(x, fname, SQLITE_TEXT);
	fname = "current_time_utc";
	sqlite_create_function(x, fname, 0, time_func, (void *) 1);
	sqlite_function_type(x, fname, SQLITE_TEXT);
	fname = "current_date_local";
	sqlite_create_function(x, fname, 0, time_func, (void *) 2);
	sqlite_function_type(x, fname, SQLITE_TEXT);
	fname = "current_date_utc";
	sqlite_create_function(x, fname, 0, time_func, (void *) 3);
	sqlite_function_type(x, fname, SQLITE_TEXT);
	fname = "current_datetime_local";
	sqlite_create_function(x, fname, 0, time_func, (void *) 4);
	sqlite_function_type(x, fname, SQLITE_TEXT);
	fname = "current_datetime_utc";
	sqlite_create_function(x, fname, 0, time_func, (void *) 5);
	sqlite_function_type(x, fname, SQLITE_TEXT);
	fname = "current_timestamp_local";
	sqlite_create_function(x, fname, 0, time_func, (void *) 4);
	sqlite_function_type(x, fname, SQLITE_TEXT);
	fname = "current_timestamp_utc";
	sqlite_create_function(x, fname, 0, time_func, (void *) 5);
	sqlite_function_type(x, fname, SQLITE_TEXT);
    }
    return SQLITE_OK;
}

/**
 * Free counted array of char pointers.
 * @param rowp pointer to char pointer array
 *
 * The -1-th element of the array holds the array size.
 * All non-NULL pointers of the array and then the array
 * itself are free'd.
 */

static void
freerows(char **rowp)
{
    int size, i;

    if (!rowp) {
	return;
    }
    --rowp;
    size = (PTRDIFF_T) rowp[0];
    for (i = 1; i <= size; i++) {
	freep(&rowp[i]);
    }
    freep(&rowp);
}

/**
 * Map SQL field type from string to ODBC integer type code.
 * @param typename field type string
 * @param nosign pointer to indicator for unsigned field or NULL
 * @param ov3 boolean, true for SQL_OV_ODBC3
 * @param nowchar boolean, for WINTERFACE don't use WCHAR
 * @result SQL data type
 */

static int
mapsqltype(const char *typename, int *nosign, int ov3, int nowchar)
{
    char *p, *q;
    int testsign = 0, result;

#ifdef WINTERFACE
    result = nowchar ? SQL_VARCHAR : SQL_WVARCHAR;
#else
    result = SQL_VARCHAR;
#endif
    if (!typename) {
	return result;
    }
    q = p = xmalloc(strlen(typename) + 1);
    if (!p) {
	return result;
    }
    strcpy(p, typename);
    while (*q) {
	*q = TOLOWER(*q);
	++q;
    }
    if (strncmp(p, "inter", 5) == 0) {
    } else if (strncmp(p, "int", 3) == 0 ||
	strncmp(p, "mediumint", 9) == 0) {
	testsign = 1;
	result = SQL_INTEGER;
    } else if (strncmp(p, "numeric", 7) == 0) {
	result = SQL_DOUBLE;
    } else if (strncmp(p, "tinyint", 7) == 0) {
	testsign = 1;
	result = SQL_TINYINT;
    } else if (strncmp(p, "smallint", 8) == 0) {
	testsign = 1;
	result = SQL_SMALLINT;
    } else if (strncmp(p, "float", 5) == 0) {
	result = SQL_DOUBLE;
    } else if (strncmp(p, "double", 6) == 0 ||
	strncmp(p, "real", 4) == 0) {
	result = SQL_DOUBLE;
    } else if (strncmp(p, "timestamp", 9) == 0) {
#ifdef SQL_TYPE_TIMESTAMP
	result = ov3 ? SQL_TYPE_TIMESTAMP : SQL_TIMESTAMP;
#else
	result = SQL_TIMESTAMP;
#endif
    } else if (strncmp(p, "datetime", 8) == 0) {
#ifdef SQL_TYPE_TIMESTAMP
	result = ov3 ? SQL_TYPE_TIMESTAMP : SQL_TIMESTAMP;
#else
	result = SQL_TIMESTAMP;
#endif
    } else if (strncmp(p, "time", 4) == 0) {
#ifdef SQL_TYPE_TIME
	result = ov3 ? SQL_TYPE_TIME : SQL_TIME;
#else
	result = SQL_TIME;
#endif
    } else if (strncmp(p, "date", 4) == 0) {
#ifdef SQL_TYPE_DATE
	result = ov3 ? SQL_TYPE_DATE : SQL_DATE;
#else
	result = SQL_DATE;
#endif
#ifdef SQL_LONGVARCHAR
    } else if (strncmp(p, "text", 4) == 0 ||
	       strncmp(p, "memo", 4) == 0 ||
	       strncmp(p, "longvarchar", 11) == 0) {
#ifdef WINTERFACE
	result = nowchar ? SQL_LONGVARCHAR : SQL_WLONGVARCHAR;
#else
	result = SQL_LONGVARCHAR;
#endif
#ifdef WINTERFACE
    } else if (strncmp(p, "wtext", 5) == 0 ||
	       strncmp(p, "wvarchar", 8) == 0 ||
	       strncmp(p, "longwvarchar", 12) == 0) {
	result = SQL_WLONGVARCHAR;
#endif
#endif
#if (HAVE_ENCDEC)
    } else if (strncmp(p, "binary", 6) == 0 ||
	       strncmp(p, "varbinary", 9) == 0 ||
	       strncmp(p, "bytea", 5) == 0 ||
	       strncmp(p, "blob", 4) == 0 ||
	       strncmp(p, "tinyblob", 8) == 0 ||
	       strncmp(p, "mediumblob", 10) == 0) {
	result = SQL_VARBINARY;
    } else if (strncmp(p, "longbinary", 10) == 0 ||
	       strncmp(p, "longvarbinary", 13) == 0 ||
	       strncmp(p, "longblob", 8) == 0) {
	result = SQL_LONGVARBINARY;
#endif
#ifdef SQL_BIT
    } else if (strncmp(p, "bool", 4) == 0 ||
	       strncmp(p, "bit", 3) == 0) {
	testsign = 0;
	result = SQL_BIT;
#endif
    }
    if (nosign) {
	if (testsign) {
	    *nosign = strstr(p, "unsigned") != NULL;
	} else {
	    *nosign = 1;
	}
    }
    xfree(p);
    return result;
}

/**
 * Get maximum display size and number of digits after decimal point
 * from field type specification.
 * @param typename field type specification
 * @param sqltype target SQL data type
 * @param mp pointer to maximum display size or NULL
 * @param dp pointer to number of digits after decimal point or NULL
 */

static void
getmd(const char *typename, int sqltype, int *mp, int *dp)
{
    int m = 0, d = 0;

    switch (sqltype) {
    case SQL_INTEGER:      m = 10; d = 9; break;
    case SQL_TINYINT:      m = 4; d = 3; break;
    case SQL_SMALLINT:     m = 6; d = 5; break;
    case SQL_FLOAT:        m = 25; d = 24; break;
    case SQL_DOUBLE:       m = 54; d = 53; break;
    case SQL_VARCHAR:      m = 255; d = 0; break;
#ifdef WINTERFACE
#ifdef SQL_WVARCHAR
    case SQL_WVARCHAR:     m = 255; d = 0; break;
#endif
#endif
#ifdef SQL_TYPE_DATE
    case SQL_TYPE_DATE:
#endif
    case SQL_DATE:         m = 10; d = 0; break;
#ifdef SQL_TYPE_TIME
    case SQL_TYPE_TIME:
#endif
    case SQL_TIME:         m = 8; d = 0; break;
#ifdef SQL_TYPE_TIMESTAMP
    case SQL_TYPE_TIMESTAMP:
#endif
    case SQL_TIMESTAMP:    m = 32; d = 3; break;
#ifdef SQL_LONGVARCHAR
    case SQL_LONGVARCHAR : m = 65536; d = 0; break;
#endif
#ifdef WINTERFACE
#ifdef SQL_WLONGVARCHAR
    case SQL_WLONGVARCHAR: m = 65536; d = 0; break;
#endif
#endif
#if (HAVE_ENCDEC)
    case SQL_VARBINARY: m = 255; d = 0; break;
    case SQL_LONGVARBINARY: m = 65536; d = 0; break;
#endif
#ifdef SQL_BIT
    case SQL_BIT:	    m = 1; d = 1; break;
#endif
    }
    if (m && typename) {
	int mm, dd;

	if (sscanf(typename, "%*[^(](%d)", &mm) == 1) {
	    if (sqltype == SQL_TIMESTAMP) {
		d = mm;
	    }
#ifdef SQL_TYPE_TIMESTAMP
	    if (sqltype == SQL_TYPE_TIMESTAMP) {
		d = mm;
	    }
#endif
	    else {
		m = d = mm;
	    }
	} else if (sscanf(typename, "%*[^(](%d,%d)", &mm, &dd) == 2) {
	    m = mm;
	    d = dd;
	}
    }
    if (mp) {
	*mp = m;
    }
    if (dp) {
	*dp = d;
    }
}

/**
 * Map SQL_C_DEFAULT to proper C type.
 * @param type input C type
 * @param stype input SQL type
 * @param nosign 0=signed, 0>unsigned, 0<undefined
 * @param nowchar when compiled with WINTERFACE don't use WCHAR
 * @result C type
 */

static int
mapdeftype(int type, int stype, int nosign, int nowchar)
{
    if (type == SQL_C_DEFAULT) {
	switch (stype) {
	case SQL_INTEGER:
	    type = (nosign > 0) ? SQL_C_ULONG : SQL_C_LONG;
	    break;
	case SQL_TINYINT:
	    type = (nosign > 0) ? SQL_C_UTINYINT : SQL_C_TINYINT;
	    break;
	case SQL_SMALLINT:
	    type = (nosign > 0) ? SQL_C_USHORT : SQL_C_SHORT;
	    break;
	case SQL_FLOAT:
	    type = SQL_C_FLOAT;
	    break;
	case SQL_DOUBLE:
	    type = SQL_C_DOUBLE;
	    break;
	case SQL_TIMESTAMP:
	    type = SQL_C_TIMESTAMP;
	    break;
	case SQL_TIME:
	    type = SQL_C_TIME;
	    break;
	case SQL_DATE:
	    type = SQL_C_DATE;
	    break;
#ifdef SQL_C_TYPE_TIMESTAMP
	case SQL_TYPE_TIMESTAMP:
	    type = SQL_C_TYPE_TIMESTAMP;
	    break;
#endif
#ifdef SQL_C_TYPE_TIME
	case SQL_TYPE_TIME:
	    type = SQL_C_TYPE_TIME;
	    break;
#endif
#ifdef SQL_C_TYPE_DATE
	case SQL_TYPE_DATE:
	    type = SQL_C_TYPE_DATE;
	    break;
#endif
#ifdef WINTERFACE
	case SQL_WVARCHAR:
	case SQL_WCHAR:
#ifdef SQL_WLONGVARCHAR
	case SQL_WLONGVARCHAR:
#endif
	    type = nowchar ? SQL_C_CHAR : SQL_C_WCHAR;
	    break;
#endif
#if (HAVE_ENCDEC)
	case SQL_BINARY:
	case SQL_VARBINARY:
	case SQL_LONGVARBINARY:
	    type = SQL_C_BINARY;
	    break;
#endif
#ifdef SQL_BIT
	case SQL_BIT:
	    type = SQL_C_BIT;
	    break;
#endif
	default:
#ifdef WINTERFACE
	    type = nowchar ? SQL_C_CHAR : SQL_C_WCHAR;
#else
	    type = SQL_C_CHAR;
#endif
	    break;
	}
    }
    return type;
}

/**
 * Fixup query string with optional parameter markers.
 * @param sql original query string
 * @param sqlLen length of query string or SQL_NTS
 * @param nparam output number of parameters
 * @param isselect output indicator for SELECT (1) or DDL statement (2)
 * @param errmsg output error message
 * @param version SQLite version information
 * @param namepp pointer to parameter names array
 * @result newly allocated string containing query string for SQLite or NULL
 */

static char *
fixupsql(char *sql, int sqlLen, int *nparam, int *isselect, char **errmsg,
	 int version, char ***namepp)
{
    char *q = sql, *qz = NULL, *p, *inq = NULL, *out;
    int np = 0, isddl = -1, size;
    char **npp = NULL, *ncp = NULL;

    *errmsg = NULL;
    if (sqlLen != SQL_NTS) {
	qz = q = xmalloc(sqlLen + 1);
	if (!qz) {
	    return NULL;
	}
	memcpy(q, sql, sqlLen);
	q[sqlLen] = '\0';
	size = sqlLen * 4;
    } else {
	size = strlen(sql) * 4;
    }
    size += sizeof (char *) - 1;
    size &= ~(sizeof (char *) - 1);
    p = xmalloc(2 * size + size * sizeof (char *) / 2);
    if (!p) {
errout:
	freep(&qz);
	return NULL;
    }
    memset(p, 0, 2 * size + size * sizeof (char *) / 2);
    out = p;
    npp = (char **) (out + size);
    ncp = (char *) npp + size * sizeof (char *) / 2;
    while (*q) {
	switch (*q) {
	case '\'':
	case '\"':
	    if (q == inq) {
		inq = NULL;
	    } else if (!inq) {
		inq = q + 1;

		while (*inq) {
		    if (*inq == *q) {
			if (inq[1] == *q) {
			    inq++;
			} else {
			    break;
			}
		    }
		    inq++;
		}
	    }
	    *p++ = *q;
	    break;
	case '?':
	    if (inq) {
		*p++ = *q;
	    } else {
		*p++ = '%';
		*p++ = 'Q';
		npp[np] = ncp;
		*ncp = '\0';
		++ncp;
		np++;
	    }
	    break;
	case ':':		/* ORACLE-style named parameter */
	case '@':		/* ADO.NET-style named parameter */
	    if (inq) {
		*p++ = *q;
	    } else {
		int n = -1;

		do {
		   ++q;
		   ++n;
		} while (*q && ISIDCHAR(*q));
		if (n > 0) {
		    *p++ = '%';
		    *p++ = 'Q';
		    npp[np] = ncp;
		    memcpy(ncp, q - n, n);
		    ncp[n] = '\0';
		    ncp += n + 1;
		    np++;
		}
		--q;
	    }
	    break;
	case ';':
	    if (!inq) {
		if (isddl < 0) {
		    char *qq = out;

		    while (*qq && ISSPACE(*qq)) {
			++qq;
		    }
		    if (*qq && *qq != ';') {
			int i;
			static const struct {
			    int len;
			    const char *str;
			} ddlstr[] = {
			    { 6, "attach" },
			    { 5, "begin" },
			    { 6, "commit" },
			    { 6, "create" },
			    { 6, "detach" },
			    { 4, "drop" },
			    { 3, "end" },
			    { 8, "rollback" },
			    { 6, "vacuum" }
			};

			size = strlen(qq);
			for (i = 0; i < array_size(ddlstr); i++) {
			    if (size >= ddlstr[i].len &&
				strncasecmp(qq, ddlstr[i].str, ddlstr[i].len)
				== 0) {
				isddl = 1;
				break;
			    }
			}
			if (isddl != 1) {
			    isddl = 0;
			}
		    }
		}
		if (isddl == 0) {
		    char *qq = q;

		    do {
			++qq;
		    } while (*qq && ISSPACE(*qq));
		    if (*qq && *qq != ';') {
			freep(&out);
			*errmsg = "only one SQL statement allowed";
			goto errout;
		    }
		}
	    }
	    *p++ = *q;
	    break;
	case '%':
	    *p++ = '%';
	    *p++ = '%';
	    break;
	case '{':
	    /*
	     * Deal with escape sequences:
	     * {d 'YYYY-MM-DD'}, {t ...}, {ts ...}
	     * {oj ...}, {fn ...} etc.
	     */
	    if (!inq) {
		int ojfn = 0;
		char *inq2 = NULL, *end = q + 1;

		if (*end != 'd' && *end != 'D' &&
		    *end != 't' && *end != 'T') {
		    ojfn = 1;
		}
		while (*end) {
		    if (inq2 && *end == *inq2) {
			inq2 = NULL;
		    } else if (inq2 == NULL && *end == '}') {
			break;
		    } else if (inq2 == NULL && (*end == '\'' || *end == '"')) {
			inq2 = end;
		    }
		    ++end;
		}
		if (*end == '}') {
		    char *start = q + 1;
		    char *end2 = end - 1;

		    if (ojfn) {
			while (start < end) {
			    if (ISSPACE(*start)) {
				break;
			    }
			    ++start;
			}
			while (start < end) {
			    *p++ = *start;
			    ++start;
			}
			q = end;
			break;
		    } else {
			while (start < end2 && *start != '\'') {
			    ++start;
			}
			while (end2 > start && *end2 != '\'') {
			    --end2;
			}
			if (*start == '\'' && *end2 == '\'') {
			    while (start <= end2) {
				*p++ = *start;
				++start;
			    }
			    q = end;
			    break;
			}
		    }
		}
	    }
	    /* FALL THROUGH */
	default:
	    *p++ = *q;
	}
	++q;
    }
    freep(&qz);
    *p = '\0';
    if (nparam) {
	*nparam = np;
    }
    if (isselect) {
	if (isddl > 0) {
	    *isselect = 2;
	} else {
	    int incom = 0;

	    p = out;
	    while (*p) {
		switch (*p) {
		case '-':
		    if (!incom && p[1] == '-') {
			incom = -1;
		    }
		    break;
		case '\n':
		    if (incom < 0) {
			incom = 0;
		    }
		    break;
		case '/':
		    if (incom > 0 && p[-1] == '*') {
			incom = 0;
			p++;
			continue;
		    } else if (!incom && p[1] == '*') {
			incom = 1;
		    }
		    break;
		}
		if (!incom && !ISSPACE(*p)) {
		    break;
		}
		++p;
	    }
	    size = strlen(p);
	    if (size >= 6 &&
		(strncasecmp(p, "select", 6) == 0 ||
		 strncasecmp(p, "pragma", 6) == 0)) {
		*isselect = 1;
	    } else {
		*isselect = 0;
	    }
	}
    }
    if (namepp) {
	*namepp = npp;
    }
    return out;
}

/**
 * Find column given name in string array.
 * @param cols string array
 * @param ncols number of strings
 * @param name column name
 * @result >= 0 on success, -1 on error
 */

static int
findcol(char **cols, int ncols, char *name)
{
    int i;

    if (cols) {
	for (i = 0; i < ncols; i++) {
	    if (strcmp(cols[i], name) == 0) {
		return i;
	    }
	}
    }
    return -1;
}

/**
 * Fixup column information for a running statement.
 * @param s statement to get fresh column information
 * @param sqlite SQLite database handle
 * @param types column types or NULL
 *
 * The column labels get the table names stripped
 * when there's more than one column and all table
 * names are identical.
 *
 * The "dyncols" field of STMT is filled with column
 * information obtained by SQLite "PRAGMA table_info"
 * for each column whose table name is known. If the
 * types are already present as with SQLite 2.5.7
 * this information is used instead.
 */

static void
fixupdyncols(STMT *s, sqlite *sqlite, const char **types)
{
    int i, k, pk, nn, t, r, nrows, ncols;
    char **rowp, *flagp, flags[128];

    if (!s->dyncols) {
	return;
    }
    /* fixup labels */
    if (!s->longnames) {
	if (s->dcols > 1) {
	    char *table = s->dyncols[0].table;

	    for (i = 1; table[0] && i < s->dcols; i++) {
		if (strcmp(s->dyncols[i].table, table)) {
		    break;
		}
	    }
	    if (i >= s->dcols) {
		for (i = 0; i < s->dcols; i++) {
		    s->dyncols[i].label = s->dyncols[i].column;
		}
	    }
	} else if (s->dcols == 1) {
	    s->dyncols[0].label = s->dyncols[0].column;
	}
    }
    if (types) {
	for (i = 0; i < s->dcols; i++) {
	    freep(&s->dyncols[i].typename);
	    s->dyncols[i].typename = xstrdup(types[i] ? types[i] : "text");
	    s->dyncols[i].type =
		mapsqltype(types[i], &s->dyncols[i].nosign, *s->ov3,
			   s->nowchar[0] || s->nowchar[1]);
	    getmd(types[i], s->dyncols[i].type, &s->dyncols[i].size,
		  &s->dyncols[i].prec);
#ifdef SQL_LONGVARCHAR
	    if (s->dyncols[i].type == SQL_VARCHAR &&
		s->dyncols[i].size > 255) {
		s->dyncols[i].type = SQL_LONGVARCHAR;
	    }
#endif
#ifdef WINTERFACE
#ifdef SQL_WLONGVARCHAR
	    if (s->dyncols[i].type == SQL_WVARCHAR &&
		s->dyncols[i].size > 255) {
		s->dyncols[i].type = SQL_WLONGVARCHAR;
	    }
#endif
#endif
#if (HAVE_ENCDEC)
	    if (s->dyncols[i].type == SQL_VARBINARY &&
		s->dyncols[i].size > 255) {
		s->dyncols[i].type = SQL_LONGVARBINARY;
	    }
#endif
	}
    }
    if (s->dcols > array_size(flags)) {
	flagp = xmalloc(sizeof (flags[0]) * s->dcols);
	if (flagp == NULL) {
	    return;
	}
    } else {
	flagp = flags;
    }
    memset(flagp, 0, sizeof (flags[0]) * s->dcols);
    for (i = 0; i < s->dcols; i++) {
	s->dyncols[i].autoinc = SQL_FALSE;
	s->dyncols[i].notnull = SQL_NULLABLE;
    }
    for (i = 0; i < s->dcols; i++) {
	int ret, lastpk = -1, autoinccount = 0;

	if (!s->dyncols[i].table[0]) {
	    continue;
	}
	if (flagp[i]) {
	    continue;
	}
	ret = sqlite_get_table_printf(sqlite,
				      "PRAGMA table_info('%q')", &rowp,
				      &nrows, &ncols, NULL,
				      s->dyncols[i].table);
	if (ret != SQLITE_OK) {
	    continue;
	}
	k = findcol(rowp, ncols, "name");
	t = findcol(rowp, ncols, "type");
	pk = findcol(rowp, ncols, "pk");
	nn = findcol(rowp, ncols, "notnull");
	if (k < 0 || t < 0) {
	    goto freet;
	}
	for (r = 1; r <= nrows; r++) {
	    int m;

	    for (m = i; m < s->dcols; m++) {
		if (!flagp[m] &&
		    strcmp(s->dyncols[m].column, rowp[r * ncols + k]) == 0 &&
		    strcmp(s->dyncols[m].table, s->dyncols[i].table) == 0) {
		    char *typename = rowp[r * ncols + t];

		    flagp[m] = 1;
		    freep(&s->dyncols[m].typename);
		    s->dyncols[m].typename = xstrdup(typename);
		    s->dyncols[m].type =
			mapsqltype(typename, &s->dyncols[m].nosign, *s->ov3,
				   s->nowchar[0] || s->nowchar[1]);
		    getmd(typename, s->dyncols[m].type, &s->dyncols[m].size,
			  &s->dyncols[m].prec);
#ifdef SQL_LONGVARCHAR
		    if (s->dyncols[m].type == SQL_VARCHAR &&
			s->dyncols[m].size > 255) {
			s->dyncols[m].type = SQL_LONGVARCHAR;
		    }
#endif
#ifdef WINTERFACE
#ifdef SQL_WLONGVARCHAR
		    if (s->dyncols[i].type == SQL_WVARCHAR &&
			s->dyncols[i].size > 255) {
			s->dyncols[i].type = SQL_WLONGVARCHAR;
		    }
#endif
#endif
#if (HAVE_ENCDEC)
		    if (s->dyncols[i].type == SQL_VARBINARY &&
			s->dyncols[i].size > 255) {
			s->dyncols[i].type = SQL_LONGVARBINARY;
		    }
#endif
		    if (pk >= 0	&& strcmp(rowp[r * ncols + pk], "1") == 0) {
			if (++autoinccount > 1) {
			    if (lastpk >= 0) {
				s->dyncols[lastpk].autoinc = SQL_FALSE;
				lastpk = -1;
			    }
			} else {
			    lastpk = m;
			    if (strlen(typename) == 7 &&
				strncasecmp(typename, "integer", 7) == 0) {
				s->dyncols[m].autoinc = SQL_TRUE;
			    }
			}
		    }
		    if (nn >= 0 && rowp[r * ncols + nn][0] != '0') {
			s->dyncols[m].notnull = SQL_NO_NULLS;
		    }
		}
	    }
	}
freet:
	sqlite_free_table(rowp);
    }
    if (flagp != flags) {
	freep(&flagp);
    }
}

/**
 * Return number of month days.
 * @param year
 * @param month 1..12
 * @result number of month days or 0
 */

static int
getmdays(int year, int month)
{
    static const int mdays[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    int mday;

    if (month < 1) {
	return 0;
    }
    mday = mdays[(month - 1) % 12];
    if (mday == 28 && year % 4 == 0 &&
	(!(year % 100 == 0) || year % 400 == 0)) {
	mday++;
    }
    return mday;
}

/**
 * Convert string to ODBC DATE_STRUCT.
 * @param str string to be converted
 * @param ds output DATE_STRUCT
 * @result 0 on success, -1 on error
 *
 * Strings of the format 'YYYYMMDD' or 'YYYY-MM-DD' or
 * 'YYYY/MM/DD' or 'MM/DD/YYYY' are converted to a
 * DATE_STRUCT.
 */

static int
str2date(char *str, DATE_STRUCT *ds)
{
    int i, err = 0;
    char *p, *q, sepc = '\0';

    ds->year = ds->month = ds->day = 0;
    p = str;
    while (*p && !ISDIGIT(*p)) {
	++p;
    }
    q = p;
    i = 0;
    while (*q && !ISDIGIT(*q)) {
	++i;
	++q;
    }
    if (i >= 8) {
	char buf[8];

	strncpy(buf, p + 0, 4); buf[4] = '\0';
	ds->year = strtol(buf, NULL, 10);
	strncpy(buf, p + 4, 2); buf[2] = '\0';
	ds->month = strtol(buf, NULL, 10);
	strncpy(buf, p + 6, 2); buf[2] = '\0';
	ds->day = strtol(buf, NULL, 10);
	goto done;
    }
    i = 0;
    while (i < 3) {
	int n;

	q = NULL; 
	n = strtol(p, &q, 10);
	if (!q || q == p) {
	    if (*q == '\0') {
		if (i == 0) {
		    err = 1;
		}
		goto done;
	    }
	}
	if (!sepc) {
	    sepc = *q;
	}
	if (*q == '-' || *q == '/' || *q == '\0' || i == 2) {
	    switch (i) {
	    case 0: ds->year = n; break;
	    case 1: ds->month = n; break;
	    case 2: ds->day = n; break;
	    }
	    ++i;
	    if (*q) {
		++q;
	    }
	} else {
	    i = 0;
	    while (*q && !ISDIGIT(*q)) {
		++q;
	    }
	}
	p = q;
    }
done:
    /* final check for overflow */
    if (err ||
	ds->month < 1 || ds->month > 12 ||
	ds->day < 1 || ds->day > getmdays(ds->year, ds->month)) {
	if (sepc == '/') {
	    /* Try MM/DD/YYYY format */
	    int t[3];

	    t[0] = ds->year;
	    t[1] = ds->month;
	    t[2] = ds->day;
	    ds->year = t[2];
	    ds->day = t[1];
	    ds->month = t[0];
	    if (ds->month >= 1 && ds->month <= 12 &&
		ds->day >= 1 && ds->day <= getmdays(ds->year, ds->month)) {
		return 0;
	    }
	}
	return -1;
    }
    return 0;
}

/**
 * Convert string to ODBC TIME_STRUCT.
 * @param str string to be converted
 * @param ts output TIME_STRUCT
 * @result 0 on success, -1 on error
 *
 * Strings of the format 'HHMMSS' or 'HH:MM:SS'
 * are converted to a TIME_STRUCT.
 */

static int
str2time(char *str, TIME_STRUCT *ts)
{
    int i, err = 0, ampm = -1;
    char *p, *q;

    ts->hour = ts->minute = ts->second = 0;
    p = str;
    while (*p && !ISDIGIT(*p)) {
	++p;
    }
    q = p;
    i = 0;
    while (*q && ISDIGIT(*q)) {
	++i;
	++q;
    }
    if (i >= 6) {
	char buf[4];

	strncpy(buf, p + 0, 2); buf[2] = '\0';
	ts->hour = strtol(buf, NULL, 10);
	strncpy(buf, p + 2, 2); buf[2] = '\0';
	ts->minute = strtol(buf, NULL, 10);
	strncpy(buf, p + 4, 2); buf[2] = '\0';
	ts->second = strtol(buf, NULL, 10);
	goto done;
    }
    i = 0;
    while (i < 3) {
	int n;

	q = NULL; 
	n = strtol(p, &q, 10);
	if (!q || q == p) {
	    if (*q == '\0') {
		if (i == 0) {
		    err = 1;
		}
		goto done;
	    }
	}
	if (*q == ':' || *q == '\0' || i == 2) {
	    switch (i) {
	    case 0: ts->hour = n; break;
	    case 1: ts->minute = n; break;
	    case 2: ts->second = n; break;
	    }
	    ++i;
	    if (*q) {
		++q;
	    }
	} else {
	    i = 0;
	    while (*q && !ISDIGIT(*q)) {
		++q;
	    }
	}
	p = q;
    }
    if (!err) {
	while (*p) {
	    if ((p[0] == 'p' || p[0] == 'P') &&
		(p[1] == 'm' || p[1] == 'M')) {
		ampm = 1;
	    } else if ((p[0] == 'a' || p[0] == 'A') &&
		       (p[1] == 'm' || p[1] == 'M')) {
		ampm = 0;
	    }
	    ++p;
	}
	if (ampm > 0) {
	    if (ts->hour < 12) {
		ts->hour += 12;
	    }
	} else if(ampm == 0) {
	    if (ts->hour == 12) {
		ts->hour = 0;
	    }
	}
    }
done:
    /* final check for overflow */
    if (err || ts->hour > 23 || ts->minute > 59 || ts->second > 59) {
	return -1;
    }
    return 0;
}

/**
 * Convert string to ODBC TIMESTAMP_STRUCT.
 * @param str string to be converted
 * @param tss output TIMESTAMP_STRUCT
 * @result 0 on success, -1 on error
 *
 * Strings of the format 'YYYYMMDDhhmmssff' or 'YYYY-MM-DD hh:mm:ss ff'
 * or 'YYYY/MM/DD hh:mm:ss ff' or 'hh:mm:ss ff YYYY-MM-DD' are
 * converted to a TIMESTAMP_STRUCT. The ISO8601 formats
 *    YYYY-MM-DDThh:mm:ss[.f]Z
 *    YYYY-MM-DDThh:mm:ss[.f]shh:mm
 * are also supported. In case a time zone field is present,
 * the resulting TIMESTAMP_STRUCT is expressed in UTC.
 */

static int
str2timestamp(char *str, TIMESTAMP_STRUCT *tss)
{
    int i, m, n, err = 0, ampm = -1;
    char *p, *q, in = '\0', sepc = '\0';

    tss->year = tss->month = tss->day = 0;
    tss->hour = tss->minute = tss->second = 0;
    tss->fraction = 0;
    p = str;
    while (*p && !ISDIGIT(*p)) {
	++p;
    }
    q = p;
    i = 0;
    while (*q && ISDIGIT(*q)) {
	++i;
	++q;
    }
    if (i >= 14) {
	char buf[16];

	strncpy(buf, p + 0, 4); buf[4] = '\0';
	tss->year = strtol(buf, NULL, 10);
	strncpy(buf, p + 4, 2); buf[2] = '\0';
	tss->month = strtol(buf, NULL, 10);
	strncpy(buf, p + 6, 2); buf[2] = '\0';
	tss->day = strtol(buf, NULL, 10);
	strncpy(buf, p + 8, 2); buf[2] = '\0';
	tss->hour = strtol(buf, NULL, 10);
	strncpy(buf, p + 10, 2); buf[2] = '\0';
	tss->minute = strtol(buf, NULL, 10);
	strncpy(buf, p + 12, 2); buf[2] = '\0';
	tss->second = strtol(buf, NULL, 10);
	if (i > 14) {
	    m = i - 14;
	    strncpy(buf, p + 14, m);
	    while (m < 9) {
		buf[m] = '0';
		++m;
	    }
	    buf[m] = '\0';
	    tss->fraction = strtol(buf, NULL, 10);
	}
	m = 7;
	goto done;
    }
    m = i = 0;
    while ((m & 7) != 7) {
	q = NULL; 
	n = strtol(p, &q, 10);
	if (!q || q == p) {
	    if (*q == '\0') {
		if (m < 1) {
		    err = 1;
		}
		goto done;
	    }
	}
	if (in == '\0') {
	    switch (*q) {
	    case '-':
	    case '/':
		if ((m & 1) == 0) {
		    in = *q;
		    i = 0;
		}
		break;
	    case ':':
		if ((m & 2) == 0) {
		    in = *q;
		    i = 0;
		}
		break;
	    case ' ':
	    case '.':
		break;
	    default:
		in = '\0';
		i = 0;
		break;
	    }
	}
	switch (in) {
	case '-':
	case '/':
	    if (!sepc) {
		sepc = in;
	    }
	    switch (i) {
	    case 0: tss->year = n; break;
	    case 1: tss->month = n; break;
	    case 2: tss->day = n; break;
	    }
	    if (++i >= 3) {
		i = 0;
		m |= 1;
		if (!(m & 2)) {
		    m |= 8;
		}
		goto skip;
	    } else {
		++q;
	    }
	    break;
	case ':':
	    switch (i) {
	    case 0: tss->hour = n; break;
	    case 1: tss->minute = n; break;
	    case 2: tss->second = n; break;
	    }
	    if (++i >= 3) {
		i = 0;
		m |= 2;
		if (*q == '.') {
		    in = '.';
		    goto skip2;
		}
		if (*q == ' ') {
		    if ((m & 1) == 0) {
			char *e = NULL;

			(void) strtol(q + 1, &e, 10);
			if (e && *e == '-') {
			    goto skip;
			}
		    }
		    in = '.';
		    goto skip2;
		}
		goto skip;
	    } else {
		++q;
	    }
	    break;
	case '.':
	    if (++i >= 1) {
		int ndig = q - p;

		if (p[0] == '+' || p[0] == '-') {
		    ndig--;
		}
		while (ndig < 9) {
		    n = n * 10;
		    ++ndig;
		}
		tss->fraction = n;
		m |= 4;
		i = 0;
	    }
	default:
	skip:
	    in = '\0';
	skip2:
	    while (*q && !ISDIGIT(*q)) {
		if ((q[0] == 'a' || q[0] == 'A') &&
		    (q[1] == 'm' || q[1] == 'M')) {
		    ampm = 0;
		    ++q;
		} else if ((q[0] == 'p' || q[0] == 'P') &&
			   (q[1] == 'm' || q[1] == 'M')) {
		    ampm = 1;
		    ++q;
		}
		++q;
	    }
	}
	p = q;
    }
    if ((m & 7) > 1 && (m & 8)) {
	/* ISO8601 timezone */
	if (p > str && ISDIGIT(*p)) {
	    int nn, sign;

	    q = p - 1;
	    if (*q != '+' && *q != '-') {
		goto done;
	    }
	    sign = (*q == '+') ? -1 : 1;
	    q = NULL;
	    n = strtol(p, &q, 10);
	    if (!q || *q++ != ':' || !ISDIGIT(*q)) {
		goto done;
	    }
	    p = q;
	    q = NULL;
	    nn = strtol(p, &q, 10);
	    tss->minute += nn * sign;
	    if ((SQLSMALLINT) tss->minute < 0) {
		tss->hour -= 1;
		tss->minute += 60;
	    } else if (tss->minute >= 60) {
		tss->hour += 1;
		tss->minute -= 60;
	    }
	    tss->hour += n * sign;
	    if ((SQLSMALLINT) tss->hour < 0) {
		tss->day -= 1;
		tss->hour += 24;
	    } else if (tss->hour >= 24) {
		tss->day += 1;
		tss->hour -= 24;
	    }
	    if ((short) tss->day < 1 || tss->day >= 28) {
		int mday, pday, pmon;

		mday = getmdays(tss->year, tss->month);
		pmon = tss->month - 1;
		if (pmon < 1) {
		    pmon = 12;
		}
		pday = getmdays(tss->year, pmon);
		if ((SQLSMALLINT) tss->day < 1) {
		    tss->month -= 1;
		    tss->day = pday;
		} else if (tss->day > mday) {
		    tss->month += 1;
		    tss->day = 1;
		}
		if ((SQLSMALLINT) tss->month < 1) {
		    tss->year -= 1;
		    tss->month = 12;
		} else if (tss->month > 12) {
		    tss->year += 1;
		    tss->month = 1;
		}
	    }
	}
    }
done:
    if ((m & 1) &&
	(tss->month < 1 || tss->month > 12 ||
	 tss->day < 1 || tss->day > getmdays(tss->year, tss->month))) {
	if (sepc == '/') {
	    /* Try MM/DD/YYYY format */
	    int t[3];

	    t[0] = tss->year;
	    t[1] = tss->month;
	    t[2] = tss->day;
	    tss->year = t[2];
	    tss->day = t[1];
	    tss->month = t[0];
	}
    }
    /* Replace missing year/month/day with current date */
    if (!err && (m & 1) == 0) {
#ifdef _WIN32
	SYSTEMTIME t;

	GetLocalTime(&t);
	tss->year = t.wYear;
	tss->month = t.wMonth;
	tss->day = t.wDay;
#else
	struct timeval tv;
	struct tm tm;

	gettimeofday(&tv, NULL);
	tm = *localtime(&tv.tv_sec);
	tss->year = tm.tm_year + 1900;
	tss->month = tm.tm_mon + 1;
	tss->day = tm.tm_mday;
#endif
    }
    /* Normalize fraction */
    if (tss->fraction < 0) {
	tss->fraction = 0;
    }
    /* Final check for overflow */
    if (err ||
	tss->month < 1 || tss->month > 12 ||
	tss->day < 1 || tss->day > getmdays(tss->year, tss->month) ||
	tss->hour > 23 || tss->minute > 59 || tss->second > 59) {
	return -1;
    }
    if ((m & 7) > 1) {
	if (ampm > 0) {
	    if (tss->hour < 12) {
		tss->hour += 12;
	    }
	} else if (ampm == 0) {
	    if (tss->hour == 12) {
		tss->hour = 0;
	    }
	}
    }
    return ((m & 7) < 1) ? -1 : 0;
}

/**
 * Get boolean flag from string.
 * @param string string to be inspected
 * @result true or false
 */

static int
getbool(char *string)
{
    if (string) {
	return string[0] && strchr("Yy123456789Tt", string[0]) != NULL;
    }
    return 0;
}

#if defined(HAVE_SQLITETRACE) && (HAVE_SQLITETRACE)
/**
 * SQLite trace callback
 * @param arg DBC pointer
 */

static void
dbtrace(void *arg, const char *msg)
{
    DBC *d = (DBC *) arg;

    if (msg && d->trace) {
	int len = strlen(msg);

	if (len > 0) {
	    char *end = "\n";

	    if (msg[len - 1] != ';') {
		end = ";\n";
	    }
	    fprintf(d->trace, "%s%s", msg, end);
	    fflush(d->trace);
	}
    }
}

/**
 * Trace function for SQLite return codes
 * @param rc SQLite return code
 * @param err error string or NULL
 */

static void
dbtracerc(DBC *d, int rc, char *err)
{
    if (rc != SQLITE_OK && d->trace) {
	fprintf(d->trace, "-- SQLITE ERROR CODE %d", rc);
	fprintf(d->trace, err ? ": %s\n" : "\n", err);
	fflush(d->trace);
    }
}
#else

#define dbtracerc(a,b,c)

#endif

/**
 * Open SQLite database file given file name and flags.
 * @param d DBC pointer
 * @param name file name
 * @param dsn data source name
 * @param sflag STEPAPI flag
 * @param sflag NoTXN flag
 * @param busy busy/lock timeout
 * @result ODBC error code
 */

static SQLRETURN
dbopen(DBC *d, char *name, char *dsn, char *sflag, char *ntflag, char *busy)
{
    char *errp = NULL, *endp = NULL;
    int tmp, busyto = 100000;
#if defined(_WIN32) || defined(_WIN64)
    char expname[MAX_PATH];
#endif

    if (d->sqlite) {
	sqlite_close(d->sqlite);
	d->sqlite = NULL;
    }
#if defined(_WIN32) || defined(_WIN64)
    expname[0] = '\0';
    tmp = ExpandEnvironmentStrings(name, expname, sizeof (expname));
    if (tmp <= sizeof (expname)) {
	name = expname;
    }
#endif
    d->sqlite = sqlite_open(name, 0, &errp);
    if (d->sqlite == NULL) {
connfail:
	setstatd(d, -1, "%s", (*d->ov3) ? "HY000" : "S1000",
		 errp ? errp : "connect failed");
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	return SQL_ERROR;
    }
    if (errp) {
	sqlite_freemem(errp);
	errp = NULL;
    }
#if defined(HAVE_SQLITETRACE) && (HAVE_SQLITETRACE)
    if (d->trace) {
	sqlite_trace(d->sqlite, dbtrace, d);
    }
#endif
    d->step_enable = getbool(sflag);
    d->trans_disable = getbool(ntflag);
    d->curtype = d->step_enable ?
	SQL_CURSOR_FORWARD_ONLY : SQL_CURSOR_STATIC;
    tmp = strtol(busy, &endp, 0);
    if (endp && *endp == '\0' && endp != busy) {
	busyto = tmp;
    }
    if (busyto < 1 || busyto > 1000000) {
	busyto = 1000000;
    }
    d->timeout = busyto;
    freep(&d->dbname);
    d->dbname = xstrdup(name);
    freep(&d->dsn);
    d->dsn = xstrdup(dsn);
    if (setsqliteopts(d->sqlite, d) != SQLITE_OK) {
	sqlite_close(d->sqlite);
	d->sqlite = NULL;
	goto connfail;
    }
#if defined(_WIN32) || defined(_WIN64)
    {
	char pname[MAX_PATH];
	HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			       FALSE, GetCurrentProcessId());

	pname[0] = '\0';
	if (h) {
	    HMODULE m = NULL, l = LoadLibrary("psapi.dll");
	    DWORD need;
	    typedef BOOL (WINAPI *epmfunc)(HANDLE, HMODULE *, DWORD, LPDWORD);
	    typedef BOOL (WINAPI *gmbfunc)(HANDLE, HMODULE, LPSTR, DWORD);
	    epmfunc epm;
	    gmbfunc gmb;

	    if (l) {
		epm = (epmfunc) GetProcAddress(l, "EnumProcessModules");
		gmb = (gmbfunc) GetProcAddress(l, "GetModuleBaseNameA");
		if (epm && gmb && epm(h, &m, sizeof (m), &need)) {
		    gmb(h, m, pname, sizeof (pname));
		}
		FreeLibrary(l);
	    }
	    CloseHandle(h);
	}
	d->xcelqrx = strncasecmp(pname, "EXCEL", 5) == 0 ||
		     strncasecmp(pname, "MSQRY", 5) == 0;
	if (d->trace && d->xcelqrx) {
	    fprintf(d->trace, "-- enabled EXCEL quirks\n");
	    fflush(d->trace);
	}
    }
#endif
    return SQL_SUCCESS;
}

/**
 * Do one VM step gathering one result row
 * @param s statement pointer
 * @result ODBC error code
 */

static int
vm_step(STMT *s)
{
    DBC *d = (DBC *) s->dbc;
    char **rowd = NULL, *errp = NULL;
    const char **values, **cols;
    int i, ncols, rc;

    if (s != d->vm_stmt || !s->vm) {
	setstat(s, -1, "stale statement", (*s->ov3) ? "HY000" : "S1000");
	return SQL_ERROR;
    }
    rc = sqlite_step(s->vm, &ncols, &values, &cols);
    if (rc == SQLITE_ROW || rc == SQLITE_DONE) {
	++d->vm_rownum;
	if (d->vm_rownum == 0 && cols && ncols > 0) {
	    int size;
	    char *p;
	    COL *dyncols;

	    for (i = size = 0; i < ncols; i++) {
		size += 3 + 3 * strlen(cols[i]);
	    }
	    dyncols = xmalloc(ncols * sizeof (COL) + size);
	    if (!dyncols) {
		freedyncols(s);
		s->ncols = 0;
		sqlite_finalize(s->vm, NULL);
		s->vm = NULL;
		d->vm_stmt = NULL;
		return nomem(s);
	    }
	    p = (char *) (dyncols + ncols);
	    for (i = 0; i < ncols; i++) {
		char *q;

		dyncols[i].db = ((DBC *) (s->dbc))->dbname;
		strcpy(p, cols[i]);
		dyncols[i].label = p;
		p += strlen(p) + 1;
		q = strchr(cols[i], '.');
		if (q) {
		    dyncols[i].table = p;
		    strncpy(p, cols[i], q - cols[i]);
		    p[q - cols[i]] = '\0';
		    p += strlen(p) + 1;
		    strcpy(p, q + 1);
		    dyncols[i].column = p;
		    p += strlen(p) + 1;
		} else {
		    dyncols[i].table = "";
		    strcpy(p, cols[i]);
		    dyncols[i].column = p;
		    p += strlen(p) + 1;
		}
		if (s->longnames) {
		    dyncols[i].column = dyncols[i].label;
		}
#ifdef SQL_LONGVARCHAR
		dyncols[i].type = SQL_LONGVARCHAR;
		dyncols[i].size = 65535;
#else
		dyncols[i].type = SQL_VARCHAR;
		dyncols[i].size = 255;
#endif
		dyncols[i].index = i;
		dyncols[i].scale = 0;
		dyncols[i].prec = 0;
		dyncols[i].nosign = 1;
		dyncols[i].autoinc = SQL_FALSE;
		dyncols[i].notnull = SQL_NULLABLE;
		dyncols[i].typename = NULL;
	    }
	    freedyncols(s);
	    s->ncols = s->dcols = ncols;
	    s->dyncols = s->cols = dyncols;
	    fixupdyncols(s, d->sqlite, cols + ncols);
	    mkbindcols(s, s->ncols);
	}
	if (!cols || ncols <= 0) {
	    goto killvm;
	}
	if (!values) {
	    if (rc == SQLITE_DONE) {
		freeresult(s, 0);
		s->nrows = 0;
		sqlite_finalize(s->vm, NULL);
		s->vm = NULL;
		d->vm_stmt = NULL;
		return SQL_SUCCESS;
	    }
	    goto killvm;
	}
	rowd = xmalloc((1 + 2 * ncols) * sizeof (char *));
	if (rowd) {
	    rowd[0] = (char *) ((PTRDIFF_T) (ncols * 2));
	    ++rowd;
	    for (i = 0; i < ncols; i++) {
		rowd[i] = NULL;
		rowd[i + ncols] = xstrdup(values[i]);
	    }
	    for (i = 0; i < ncols; i++) {
		if (values[i] && !rowd[i + ncols]) {
		    freerows(rowd);
		    rowd = 0;
		    break;
		}
	    }
	}
	if (rowd) {
	    freeresult(s, 0);
	    s->nrows = 1;
	    s->rows = rowd;
	    s->rowfree = freerows;
	    if (rc == SQLITE_DONE) {
		sqlite_finalize(s->vm, NULL);
		s->vm = NULL;
		d->vm_stmt = NULL;
	    }
	    return SQL_SUCCESS;
	}
    }
killvm:
    sqlite_finalize(s->vm, &errp);
    s->vm = NULL;
    d->vm_stmt = NULL;
    setstat(s, rc, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
	    errp ? errp : "unknown error", rc);
    if (errp) {
	sqlite_freemem(errp);
	errp = NULL;
    }
    return SQL_ERROR;	
}

/**
 * Stop running VM
 * @param s statement pointer
 */

static void
vm_end(STMT *s)
{
    DBC *d;

    if (!s || !s->vm) {
	return;
    }
    d = (DBC *) s->dbc;
    if (d) {
	d->busyint = 0;
    }
    sqlite_finalize(s->vm, NULL);
    s->vm = NULL;
    d->vm_stmt = NULL;
}

/**
 * Conditionally stop running VM
 * @param s statement pointer
 */

static void
vm_end_if(STMT *s)
{
    DBC *d = (DBC *) s->dbc;

    if (d) {
	d->busyint = 0;
    }
    if (d && d->vm_stmt == s) {
	vm_end(s);
    }
}

/**
 * Start VM for execution of SELECT statement.
 * @param s statement pointer
 * @param params string array of statement parameters
 * @result ODBC error code
 */

static SQLRETURN
vm_start(STMT *s, char **params)
{
    DBC *d = (DBC *) s->dbc;
    char *errp = NULL, *sql = NULL;
    const char *endp;
    sqlite_vm *vm;
    int rc;

#ifdef CANT_PASS_VALIST_AS_CHARPTR
    if (params) {
	sql = sqlite_mprintf((char *) s->query,
			     params[0], params[1],
			     params[2], params[3],
			     params[4], params[5],
			     params[6], params[7],
			     params[8], params[9],
			     params[10], params[11],
			     params[12], params[13],
			     params[14], params[15],
			     params[16], params[17],
			     params[18], params[19],
			     params[20], params[21],
			     params[22], params[23],
			     params[24], params[25],
			     params[26], params[27],
			     params[28], params[29],
			     params[30], params[31]);
    } else {
	sql = sqlite_mprintf((char *) s->query);
    }
#else
    sql = sqlite_vmprintf((char *) s->query, (char *) params);
#endif
    if (!sql) {
	return nomem(s);
    }
    rc = sqlite_compile(d->sqlite, sql, &endp, &vm, &errp);
    dbtracerc(d, rc, errp);
    sqlite_freemem(sql);
    if (rc != SQLITE_OK) {
	setstat(s, rc, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		errp ? errp : "unknown error", rc);
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	return SQL_ERROR;
    }
    s->vm = vm;
    d->vm_stmt = s;
    d->vm_rownum = -1;
    return SQL_SUCCESS;
}

/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLBulkOperations(SQLHSTMT stmt, SQLSMALLINT oper)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvunimplstmt(stmt);
    HSTMT_UNLOCK(stmt);
    return ret;
}

#ifndef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLDataSources(SQLHENV env, SQLUSMALLINT dir, SQLCHAR *srvname,
	       SQLSMALLINT buflen1, SQLSMALLINT *lenp1,
	       SQLCHAR *desc, SQLSMALLINT buflen2, SQLSMALLINT *lenp2)
{
    if (env == SQL_NULL_HENV) {
	return SQL_INVALID_HANDLE;
    }
    return SQL_ERROR;
}
#endif

#ifdef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLDataSourcesW(SQLHENV env, SQLUSMALLINT dir, SQLWCHAR *srvname,
		SQLSMALLINT buflen1, SQLSMALLINT *lenp1,
		SQLWCHAR *desc, SQLSMALLINT buflen2, SQLSMALLINT *lenp2)
{
    if (env == SQL_NULL_HENV) {
	return SQL_INVALID_HANDLE;
    }
    return SQL_ERROR;
}
#endif

#ifndef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLDrivers(SQLHENV env, SQLUSMALLINT dir, SQLCHAR *drvdesc,
	   SQLSMALLINT descmax, SQLSMALLINT *desclenp,
	   SQLCHAR *drvattr, SQLSMALLINT attrmax, SQLSMALLINT *attrlenp)
{
    if (env == SQL_NULL_HENV) {
	return SQL_INVALID_HANDLE;
    }
    return SQL_ERROR;
}
#endif

#ifdef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLDriversW(SQLHENV env, SQLUSMALLINT dir, SQLWCHAR *drvdesc,
	    SQLSMALLINT descmax, SQLSMALLINT *desclenp,
	    SQLWCHAR *drvattr, SQLSMALLINT attrmax, SQLSMALLINT *attrlenp)
{
    if (env == SQL_NULL_HENV) {
	return SQL_INVALID_HANDLE;
    }
    return SQL_ERROR;
}
#endif

#ifndef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLBrowseConnect(SQLHDBC dbc, SQLCHAR *connin, SQLSMALLINT conninLen,
		 SQLCHAR *connout, SQLSMALLINT connoutMax,
		 SQLSMALLINT *connoutLen)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvunimpldbc(dbc);
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLBrowseConnectW(SQLHDBC dbc, SQLWCHAR *connin, SQLSMALLINT conninLen,
		  SQLWCHAR *connout, SQLSMALLINT connoutMax,
		  SQLSMALLINT *connoutLen)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvunimpldbc(dbc);
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

/**
 * SQLite function "current_time_local" etc.
 * @param context SQLite function context
 * @param argc number arguments
 * @param argv argument vector
 */

static void
time_func(sqlite_func *context, int argc, const char **argv)
{
    char buf[128];
    PTRDIFF_T what = (PTRDIFF_T) sqlite_user_data(context);
#if defined(_WIN32) || defined(_WIN64)
    SYSTEMTIME st;

    if (what & 1) {
	GetSystemTime(&st);
    } else {
	GetLocalTime(&st);
    }
    if (what & 4) {
	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
		st.wYear, st.wMonth, st.wDay,
		st.wHour, st.wMinute, st.wSecond);
    } else if (what & 2) {
	sprintf(buf, "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    } else {
	sprintf(buf, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
    }
#else
    time_t t;
    struct tm tm;

    time(&t);
    if (what & 1) {
#ifdef HAVE_GMTIME_R
	gmtime_r(&t, &tm);
#else
	tm = *gmtime(&t);
#endif
    } else {
#ifdef HAVE_LOCALTIME_R
	localtime_r(&t, &tm);
#else
	tm = *localtime(&t);
#endif
    }
    if (what & 4) {
	sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
    } else if (what & 2) {
	sprintf(buf, "%04d-%02d-%02d",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    } else {
	sprintf(buf, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
#endif
    sqlite_set_result_string(context, buf, -1);
}

#if (HAVE_ENCDEC)
static const char hexdigits[] = "0123456789ABCDEFabcdef";

/**
 * SQLite function "hextobin"
 * @param context SQLite function context
 * @param argc number arguments
 * @param argv argument vector
 */

static void
hextobin_func(sqlite_func *context, int argc, const char **argv)
{
    int i, len;
    char *bin, *p;

    if (argc < 1) {
	return;
    }
    if (!argv[0]) {
	sqlite_set_result_string(context, NULL, 4);
	return;
    }
    len = strlen(argv[0]) / 2;
    bin = xmalloc(len + 1);
    if (!bin) {
oom:
	sqlite_set_result_error(context, "out of memory", -1);
	return;
    }
    if (len <= 0) {
	sqlite_set_result_string(context, bin, 0);
	freep(&bin);
	return;
    }
    for (i = 0, p = (char *) argv[0]; i < len; i++) {
	char *x;
	int v;

	if (!*p || !(x = strchr(hexdigits, *p))) {
	    goto converr;
	}
	v = x - hexdigits;
	bin[i] = (v >= 16) ? ((v - 6) << 4) : (v << 4);
	++p;
	if (!*p || !(x = strchr(hexdigits, *p))) {
converr:
	    freep(&bin);
	    sqlite_set_result_error(context, "conversion error", -1);
	    return;
	}
	v = x - hexdigits;
	bin[i] |= (v >= 16) ? (v - 6) : v;
	++p;
    }
    i = sqlite_encode_binary((unsigned char *) bin, len, 0);
    p = xmalloc(i + 1);
    if (!p) {
	freep(&bin);
	goto oom;
    }
    i = sqlite_encode_binary((unsigned char *) bin, len,
			     (unsigned char *) p);
    sqlite_set_result_string(context, p, i);
    freep(&bin);
    freep(&p);
}

/**
 * SQLite function "bintohex"
 * @param context SQLite function context
 * @param argc number arguments
 * @param argv argument vector
 */

static void
bintohex_func(sqlite_func *context, int argc, const char **argv)
{
    int i, k, len;
    char *bin, *p;

    if (argc < 1) {
	return;
    }
    if (!argv[0]) {
empty:
	sqlite_set_result_string(context, "", 0);
	return;
    }
    bin = xmalloc(strlen(argv[0]) + 1);
    if (!bin) {
oom:
	sqlite_set_result_error(context, "out of memory", -1);
	return;
    }
    len = sqlite_decode_binary((unsigned char *) argv[0],
			       (unsigned char *) bin);
    if (len < 0) {
	freep(&bin);
	sqlite_set_result_error(context, "error decoding binary data", -1);
	return;
    }
    if (len == 0) {
	goto empty;
    }
    p = xmalloc(len * 2 + 1);
    if (!p) {
	goto oom;
    }
    for (i = 0, k = 0; i < len; i++) {
	p[k++] = hexdigits[(bin[i] >> 4) & 0x0f];
	p[k++] = hexdigits[bin[i] & 0x0f];
    }
    p[k] = '\0';
    sqlite_set_result_string(context, p, k);
    freep(&bin);
    freep(&p);
}

/**
 * Encode hex (char) parameter to SQLite binary string.
 * @param s STMT pointer
 * @param p BINDPARM pointer
 * @result ODBD error code
 */

static SQLRETURN
hextobin(STMT *s, BINDPARM *p)
{
    int i, len = strlen(p->param) / 2;
    char *bin = xmalloc(len + 1), *pp;

    if (!bin) {
	return nomem(s);
    }
    if (len <= 0) {
	bin[0] = '\0';
	freep(&p->parbuf);
	p->parbuf = p->param = bin;
	p->len = 0;
	return SQL_SUCCESS;
    }
    for (i = 0, pp = (char *) p->param; i < len; i++) {
	char *x;
	int v;

	if (!*pp || !(x = strchr(hexdigits, *pp))) {
	    goto converr;
	}
	v = x - hexdigits;
	bin[i] = (v >= 16) ? ((v - 6) << 4) : (v << 4);
	++pp;
	if (!*pp || !(x = strchr(hexdigits, *pp))) {
converr:
	    freep(&bin);
	    setstat(s, -1, "conversion error", (*s->ov3) ? "HY000" : "S1000");
	    return SQL_ERROR;
	}
	v = x - hexdigits;
	bin[i] |= (v >= 16) ? (v - 6) : v;
	++pp;
    }
    i = sqlite_encode_binary((unsigned char *) bin, len, 0);
    pp = xmalloc(i + 1);
    if (!pp) {
	freep(&bin);
	return nomem(s);
    }
    p->len = sqlite_encode_binary((unsigned char *) bin, len,
				  (unsigned char *) pp);
    freep(&p->parbuf);
    p->parbuf = p->param = pp;
    freep(&bin);
    return SQL_SUCCESS;
}
#endif

/**
 * Internal put (partial) parameter data into executing statement.
 * @param stmt statement handle
 * @param data pointer to data
 * @param len length of data
 * @result ODBC error code
 */

static SQLRETURN
drvputdata(SQLHSTMT stmt, SQLPOINTER data, SQLLEN len)
{
    STMT *s;
    int i, dlen, done = 0;
    BINDPARM *p;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (!s->query || s->nparams <= 0) {
seqerr:
	setstat(s, -1, "sequence error", "HY010");
	return SQL_ERROR;
    }
    for (i = (s->pdcount < 0) ? 0 : s->pdcount; i < s->nparams; i++) {
	p = &s->bindparms[i];
	if (p->need > 0) {
	    int type = mapdeftype(p->type, p->stype, -1, s->nowchar[0]);

	    if (len == SQL_NULL_DATA) {
		freep(&p->parbuf);
		p->param = NULL;
		p->len = SQL_NULL_DATA;
		p->need = -1;
	    } else if (type != SQL_C_CHAR
#ifdef WCHARSUPPORT
		       && type != SQL_C_WCHAR
#endif
#if (HAVE_ENCDEC)
		       && type != SQL_C_BINARY
#endif
		      ) {
		int size = 0;

		switch (type) {
		case SQL_C_TINYINT:
		case SQL_C_UTINYINT:
		case SQL_C_STINYINT:
#ifdef SQL_BIT
		case SQL_C_BIT:
#endif
		    size = sizeof (SQLCHAR);
		    break;
		case SQL_C_SHORT:
		case SQL_C_USHORT:
		case SQL_C_SSHORT:
		    size = sizeof (SQLSMALLINT);
		    break;
		case SQL_C_LONG:
		case SQL_C_ULONG:
		case SQL_C_SLONG:
		    size = sizeof (SQLINTEGER);
		    break;
		case SQL_C_FLOAT:
		    size = sizeof (float);
		    break;
		case SQL_C_DOUBLE:
		    size = sizeof (double);
		    break;
#ifdef SQL_C_TYPE_DATE
		case SQL_C_TYPE_DATE:
#endif
		case SQL_C_DATE:
		    size = sizeof (DATE_STRUCT);
		    break;
#ifdef SQL_C_TYPE_DATE
		case SQL_C_TYPE_TIME:
#endif
		case SQL_C_TIME:
		    size = sizeof (TIME_STRUCT);
		    break;
#ifdef SQL_C_TYPE_DATE
		case SQL_C_TYPE_TIMESTAMP:
#endif
		case SQL_C_TIMESTAMP:
		    size = sizeof (TIMESTAMP_STRUCT);
		    break;
		}
		freep(&p->parbuf);
		p->parbuf = xmalloc(size);
		if (!p->parbuf) {
		    return nomem(s);
		}
		p->param = p->parbuf;
		memcpy(p->param, data, size);
		p->len = size;
		p->need = -1;
	    } else if (len == SQL_NTS && (
		       type == SQL_C_CHAR
#ifdef WCHARSUPPORT
		       || type == SQL_C_WCHAR
#endif
		       )) {
		char *dp = data;

#ifdef WCHARSUPPORT
		if (type == SQL_C_WCHAR) {
		    dp = uc_to_utf(data, len);
		    if (!dp) {
			return nomem(s);
		    }
		}
#endif
		dlen = strlen(dp);
		freep(&p->parbuf);
		p->parbuf = xmalloc(dlen + 1);
		if (!p->parbuf) {
#ifdef WCHARSUPPORT
		    if (dp != data) {
			uc_free(dp);
		    }
#endif
		    return nomem(s);
		}
		p->param = p->parbuf;
		strcpy(p->param, dp);
#ifdef WCHARSUPPORT
		if (dp != data) {
		    uc_free(dp);
		}
#endif
		p->len = dlen;
		p->need = -1;
	    } else if (len < 0) {
		setstat(s, -1, "invalid length", "HY090");
		return SQL_ERROR;
	    } else {
		dlen = min(p->len - p->offs, len);
		if (!p->param) {
		    setstat(s, -1, "no memory for parameter", "HY013");
		    return SQL_ERROR;
		}
		memcpy((char *) p->param + p->offs, data, dlen);
		p->offs += dlen;
		if (p->offs >= p->len) {
#ifdef WCHARSUPPORT
		    if (type == SQL_C_WCHAR) {
			char *dp = uc_to_utf(p->param, p->len);
			char *np;
			int nlen;

			if (!dp) {
			    return nomem(s);
			}
			nlen = strlen(dp);
			np = xmalloc(nlen + 1);
			if (!np) {
			    uc_free(dp);
			    return nomem(s);
			}
			strcpy(np, dp);
			uc_free(dp);
			if (p->param == p->parbuf) {
			    freep(&p->parbuf);
			}
			p->parbuf = p->param = np;
			p->len = nlen;
		    } else {
			*((char *) p->param + p->len) = '\0';
		    }
#else
#if defined(_WIN32) || defined(_WIN64)
		    if (p->type == SQL_C_WCHAR &&
			(p->stype == SQL_VARCHAR ||
			 p->stype == SQL_LONGVARCHAR) &&
			 p->len == p->coldef * sizeof (SQLWCHAR)) {
			/* fix for MS-Access */
			p->len = p->coldef;
		    }
#endif
		    *((char *) p->param + p->len) = '\0';
#endif
#if (HAVE_ENCDEC)
		    if ((p->stype == SQL_BINARY ||
			 p->stype == SQL_VARBINARY ||
			 p->stype == SQL_LONGVARBINARY) &&
#ifdef WCHARSUPPORT
			(type == SQL_C_CHAR || type == SQL_C_WCHAR)
#else
			type == SQL_C_CHAR
#endif
		       ) {
			if (hextobin(s, p) != SQL_SUCCESS) {
			    return SQL_ERROR;
			}
		    } else if (type == SQL_C_BINARY) {
			int bsize;
			unsigned char *bin;

			bsize = sqlite_encode_binary(p->param, p->len, 0);
			bin = xmalloc(bsize + 1);
			if (!bin) {
			    return nomem(s);
			}
			p->len = sqlite_encode_binary(p->param, p->len, bin);
			if (p->param == p->parbuf) {
			    freep(&p->parbuf);
			}
			p->parbuf = p->param = bin;
		    }
#endif
		    p->need = -1;
		}
	    }
	    done = 1;
	    break;
	}
    }
    if (!done) {
	goto seqerr;
    }
    return SQL_SUCCESS;
}

/**
 * Put (partial) parameter data into executing statement.
 * @param stmt statement handle
 * @param data pointer to data
 * @param len length of data
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLPutData(SQLHSTMT stmt, SQLPOINTER data, SQLLEN len)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvputdata(stmt, data, len);
    HSTMT_UNLOCK(stmt);
    return ret;
}

/**
 * Clear out parameter bindings, if any.
 * @param s statement pointer
 */

static SQLRETURN
freeparams(STMT *s)
{
    if (s->bindparms) {
	int n;

	for (n = 0; n < s->nbindparms; n++) {
	    freep(&s->bindparms[n].parbuf);
	    memset(&s->bindparms[n], 0, sizeof (BINDPARM));
	}
    }
    return SQL_SUCCESS;
}

/**
 * Substitute parameter for statement.
 * @param s statement pointer
 * @param pnum parameter number
 * @param outp output pointer or NULL
 * @result ODBC error code
 *
 * If no output buffer is given, the function computes and
 * reports the space needed for the parameter. Otherwise
 * the parameter is converted to its string representation
 * in order to be presented to sqlite_exec_vprintf() et.al.
 */

static SQLRETURN
substparam(STMT *s, int pnum, char **outp)
{
    char *outdata = NULL;
    int type, len, isnull = 0, needalloc = 0;
    BINDPARM *p;
    double dval;
#if (HAVE_ENCDEC)
    int chkbin = 1;
#endif

    if (!s->bindparms || pnum < 0 || pnum >= s->nbindparms) {
	goto error;
    }
    p = &s->bindparms[pnum];
    type = mapdeftype(p->type, p->stype, -1, s->nowchar[0]);

#if (defined(_WIN32) || defined(_WIN64)) && defined(WINTERFACE)
    /* MS Access hack part 4 (map SQL_C_DEFAULT to SQL_C_CHAR) */
    if (type == SQL_C_WCHAR && p->type == SQL_C_DEFAULT) {
	type = SQL_C_CHAR;
    }
#endif

    if (p->need > 0) {
	return setupparbuf(s, p);
    }
    p->strbuf[0] = '\0';
    if (!p->param || (p->lenp && *p->lenp == SQL_NULL_DATA)) {
	isnull = 1;
	goto bind;
    }
#if (HAVE_ENCDEC)
    if (type == SQL_C_CHAR &&
	(p->stype == SQL_BINARY ||
	 p->stype == SQL_VARBINARY ||
	 p->stype == SQL_LONGVARBINARY)) {
	type = SQL_C_BINARY;
    }
#endif
    switch (type) {
    case SQL_C_CHAR:
#ifdef WCHARSUPPORT
    case SQL_C_WCHAR:
#endif
#if (HAVE_ENCDEC)
    case SQL_C_BINARY:
#endif
	break;
#ifdef SQL_BIT
    case SQL_C_BIT:
	strcpy(p->strbuf, (*((unsigned char *) p->param)) ? "1" : "0");
	goto bind;
#endif
    case SQL_C_UTINYINT:
	sprintf(p->strbuf, "%d", *((unsigned char *) p->param));
	goto bind;
    case SQL_C_TINYINT:
    case SQL_C_STINYINT:
	sprintf(p->strbuf, "%d", *((char *) p->param));
	goto bind;
    case SQL_C_USHORT:
	sprintf(p->strbuf, "%d", *((unsigned short *) p->param));
	goto bind;
    case SQL_C_SHORT:
    case SQL_C_SSHORT:
	sprintf(p->strbuf, "%d", *((short *) p->param));
	goto bind;
    case SQL_C_ULONG:
    case SQL_C_LONG:
    case SQL_C_SLONG:
	sprintf(p->strbuf, "%ld", *((long *) p->param));
	goto bind;
    case SQL_C_FLOAT:
	dval = *((float *) p->param);
	goto dodouble;
    case SQL_C_DOUBLE:
	dval = *((double *) p->param);
    dodouble:
#if defined(HAVE_SQLITEMPRINTF) && (HAVE_SQLITEMPRINTF)
	{
	    char *buf2 = sqlite_mprintf("%.16g", dval);

	    if (buf2) {
		strcpy(p->strbuf, buf2);
		sqlite_freemem(buf2);
	    } else {
		isnull = 1;
	    }
	}
#else
	ln_sprintfg(p->strbuf, dval);
#endif
	goto bind;
#ifdef SQL_C_TYPE_DATE
    case SQL_C_TYPE_DATE:
#endif
    case SQL_C_DATE:
	sprintf(p->strbuf, "%04d-%02d-%02d",
		((DATE_STRUCT *) p->param)->year,
		((DATE_STRUCT *) p->param)->month,
		((DATE_STRUCT *) p->param)->day);
	goto bind;
#ifdef SQL_C_TYPE_TIME
    case SQL_C_TYPE_TIME:
#endif
    case SQL_C_TIME:
	sprintf(p->strbuf, "%02d:%02d:%02d",
		((TIME_STRUCT *) p->param)->hour,
		((TIME_STRUCT *) p->param)->minute,
		((TIME_STRUCT *) p->param)->second);
	goto bind;
#ifdef SQL_C_TYPE_TIMESTAMP
    case SQL_C_TYPE_TIMESTAMP:
#endif
    case SQL_C_TIMESTAMP:
	len = (int) ((TIMESTAMP_STRUCT *) p->param)->fraction;
	len /= 1000000;
	len = len % 1000;
	if (len < 0) {
	    len = 0;
	}
	if (p->coldef && p->coldef <= 16) {
	    sprintf(p->strbuf, "%04d-%02d-%02d %02d:%02d:00.000",
		    ((TIMESTAMP_STRUCT *) p->param)->year,
		    ((TIMESTAMP_STRUCT *) p->param)->month,
		    ((TIMESTAMP_STRUCT *) p->param)->day,
		    ((TIMESTAMP_STRUCT *) p->param)->hour,
		    ((TIMESTAMP_STRUCT *) p->param)->minute);
	} else if (p->coldef && p->coldef <= 19) {
	    sprintf(p->strbuf, "%04d-%02d-%02d %02d:%02d:%02d.000",
		    ((TIMESTAMP_STRUCT *) p->param)->year,
		    ((TIMESTAMP_STRUCT *) p->param)->month,
		    ((TIMESTAMP_STRUCT *) p->param)->day,
		    ((TIMESTAMP_STRUCT *) p->param)->hour,
		    ((TIMESTAMP_STRUCT *) p->param)->minute,
		    ((TIMESTAMP_STRUCT *) p->param)->second);
	} else {
	    sprintf(p->strbuf, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
		    ((TIMESTAMP_STRUCT *) p->param)->year,
		    ((TIMESTAMP_STRUCT *) p->param)->month,
		    ((TIMESTAMP_STRUCT *) p->param)->day,
		    ((TIMESTAMP_STRUCT *) p->param)->hour,
		    ((TIMESTAMP_STRUCT *) p->param)->minute,
		    ((TIMESTAMP_STRUCT *) p->param)->second,
		    len);
	}
    bind:
	if (outp) {
	    *outp = isnull ? NULL : p->strbuf;
	}
	return SQL_SUCCESS;
    default:
    error:
	setstat(s, -1, "unsupported parameter type",
		(*s->ov3) ? "07009" : "S1093");
	return SQL_ERROR;
    }
    if (!p->parbuf && p->lenp) {
#ifdef WCHARSUPPORT
	if (type == SQL_C_WCHAR) {
	    if (*p->lenp == SQL_NTS) {
		p->max = uc_strlen(p->param) * sizeof (SQLWCHAR);
	    } else if (*p->lenp >= 0) {
		p->max = *p->lenp;
	    }
	} else
#endif
	if (type == SQL_C_CHAR) {
	    if (*p->lenp == SQL_NTS) {
		p->len = p->max = strlen(p->param);
	    } else if (*p->lenp >= 0) {
		p->len = p->max = *p->lenp;
		needalloc = 1;
	    }
	}
#if (HAVE_ENCDEC)
	else if (type == SQL_C_BINARY) {
	    p->len = p->max = *p->lenp;
	}
#endif
    }
    if (p->need < 0 && p->parbuf == p->param) {
	outdata = p->param;
	goto putp;
    }
#ifdef WCHARSUPPORT
    if (type == SQL_C_WCHAR) {
	char *dp = uc_to_utf(p->param, p->max);

	if (!dp) {
	    return nomem(s);
	}
	if (p->param == p->parbuf) {
	    freep(&p->parbuf);
	}
	p->parbuf = p->param = dp;
	p->need = -1;
	p->len = strlen(p->param);
	outdata = p->param;
    } else
#endif
#if (HAVE_ENCDEC)
    if (type == SQL_C_BINARY) {
	int bsize;
	char *dp;

	p->len = *p->lenp;
	if (p->len < 0) {
	    setstat(s, -1, "invalid length reference", "HY009");
	    return SQL_ERROR;
	}
	bsize = sqlite_encode_binary(p->param, p->len, 0);
	dp = xmalloc(bsize + 1);
	if (!dp) {
	    return nomem(s);
	}
	p->len = sqlite_encode_binary(p->param, p->len, (unsigned char *) dp);
	if (p->param == p->parbuf) {
	    freep(&p->parbuf);
	}
	p->parbuf = p->param = dp;
	p->need = -1;
	chkbin = 0;
	outdata = p->param;
    } else
#endif
    if (type == SQL_C_CHAR) {
	outdata = p->param;
	if (needalloc) {
	    char *dp;

	    freep(&p->parbuf);
	    dp = xmalloc(p->len + 1);
	    if (!dp) {
		return nomem(s);
	    }
	    memcpy(dp, p->param, p->len);
	    dp[p->len] = '\0';
	    p->parbuf = p->param = dp;
	    p->need = -1;
	    outdata = p->param;
	}
    } else {
	outdata = p->param;
#if (HAVE_ENCDEC)
	chkbin = 0;
#endif
    }
#if (HAVE_ENCDEC)
    if (chkbin) {
	if (p->stype == SQL_BINARY ||
	    p->stype == SQL_VARBINARY ||
	    p->stype == SQL_LONGVARBINARY) {
	    if (hextobin(s, p) != SQL_SUCCESS) {
		return SQL_ERROR;
	    }
	    outdata = p->param;
	}
    }
#endif
putp:
    if (outp) {
	*outp = outdata;
    }
    return SQL_SUCCESS;
}

/**
 * Internal bind parameter on HSTMT.
 * @param stmt statement handle
 * @param pnum parameter number, starting at 1
 * @param iotype input/output type of parameter
 * @param buftype type of host variable
 * @param ptype
 * @param coldef
 * @param scale
 * @param data pointer to host variable
 * @param buflen length of host variable
 * @param len output length pointer
 * @result ODBC error code
 */

static SQLRETURN
drvbindparam(SQLHSTMT stmt, SQLUSMALLINT pnum, SQLSMALLINT iotype,
	     SQLSMALLINT buftype, SQLSMALLINT ptype, SQLUINTEGER coldef,
	     SQLSMALLINT scale,
	     SQLPOINTER data, SQLINTEGER buflen, SQLLEN *len)
{
    STMT *s;
    BINDPARM *p;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (pnum == 0) {
	setstat(s, -1, "invalid parameter", (*s->ov3) ? "07009" : "S1093");
	return SQL_ERROR;
    }
    if (!data && !len) {
	setstat(s, -1, "invalid buffer", "HY003");
	return SQL_ERROR;
    }
    --pnum;
    if (s->bindparms) {
	if (pnum >= s->nbindparms) {
	    BINDPARM *newparms;
	    
	    newparms = xrealloc(s->bindparms,
				(pnum + 1) * sizeof (BINDPARM));
	    if (!newparms) {
outofmem:
		return nomem(s);
	    }
	    s->bindparms = newparms;
	    memset(&s->bindparms[s->nbindparms], 0,
		   (pnum + 1 - s->nbindparms) * sizeof (BINDPARM));
	    s->nbindparms = pnum + 1;
	}
    } else {
	int npar = max(10, pnum + 1);

	s->bindparms = xmalloc(npar * sizeof (BINDPARM));
	if (!s->bindparms) {
	    goto outofmem;
	}
	memset(s->bindparms, 0, npar * sizeof (BINDPARM));
	s->nbindparms = npar;
    }
    switch (buftype) {
    case SQL_C_STINYINT:
    case SQL_C_UTINYINT:
    case SQL_C_TINYINT:
#ifdef SQL_C_BIT
    case SQL_C_BIT:
#endif
	buflen = sizeof (char);
	break;
    case SQL_C_SHORT:
    case SQL_C_USHORT:
    case SQL_C_SSHORT:
	buflen = sizeof (short);
	break;
    case SQL_C_SLONG:
    case SQL_C_ULONG:
    case SQL_C_LONG:
	buflen = sizeof (long);
	break;
    case SQL_C_FLOAT:
	buflen = sizeof (float);
	break;
    case SQL_C_DOUBLE:
	buflen = sizeof (double);
	break;
    case SQL_C_TIMESTAMP:
#ifdef SQL_C_TYPE_TIMESTAMP
    case SQL_C_TYPE_TIMESTAMP:
#endif
	buflen = sizeof (TIMESTAMP_STRUCT);
	break;
    case SQL_C_TIME:
#ifdef SQL_C_TYPE_TIME
    case SQL_C_TYPE_TIME:
#endif
	buflen = sizeof (TIME_STRUCT);
	break;
    case SQL_C_DATE:
#ifdef SQL_C_TYPE_DATE
    case SQL_C_TYPE_DATE:
#endif
	buflen = sizeof (DATE_STRUCT);
	break;
#ifdef SQL_C_UBIGINT
    case SQL_C_UBIGINT:
	buflen = sizeof (SQLBIGINT);
	break;
#endif
#ifdef SQL_C_SBIGINT
    case SQL_C_SBIGINT:
	buflen = sizeof (SQLBIGINT);
	break;
#endif
#ifdef SQL_C_BIGINT
    case SQL_C_BIGINT:
	buflen = sizeof (SQLBIGINT);
	break;
#endif
    }
    p = &s->bindparms[pnum];
    p->type = buftype;
    p->stype = ptype;
    p->coldef = coldef;
    p->scale = scale;
    p->max = buflen;
    p->inc = buflen;
    p->lenp0 = p->lenp = len;
    p->offs = 0;
    p->len = 0;
    p->param0 = data;
    freep(&p->parbuf);
    p->param = p->param0;
    p->bound = 1;
    p->need = 0;
    return SQL_SUCCESS;
}

/**
 * Bind parameter on HSTMT.
 * @param stmt statement handle
 * @param pnum parameter number, starting at 1
 * @param iotype input/output type of parameter
 * @param buftype type of host variable
 * @param ptype
 * @param coldef
 * @param scale
 * @param data pointer to host variable
 * @param buflen length of host variable
 * @param len output length pointer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLBindParameter(SQLHSTMT stmt, SQLUSMALLINT pnum, SQLSMALLINT iotype,
		 SQLSMALLINT buftype, SQLSMALLINT ptype, SQLULEN coldef,
		 SQLSMALLINT scale,
		 SQLPOINTER data, SQLLEN buflen, SQLLEN *len)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvbindparam(stmt, pnum, iotype, buftype, ptype, coldef,
		       scale, data, buflen, len);
    HSTMT_UNLOCK(stmt);
    return ret;
}

#ifndef HAVE_IODBC
/**
 * Bind parameter on HSTMT.
 * @param stmt statement handle
 * @param pnum parameter number, starting at 1
 * @param vtype input/output type of parameter
 * @param ptype
 * @param lenprec
 * @param scale
 * @param val pointer to host variable
 * @param lenp output length pointer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLBindParam(SQLHSTMT stmt, SQLUSMALLINT pnum, SQLSMALLINT vtype,
	     SQLSMALLINT ptype, SQLULEN lenprec,
	     SQLSMALLINT scale, SQLPOINTER val,
	     SQLLEN *lenp)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvbindparam(stmt, pnum, SQL_PARAM_INPUT, vtype, ptype,
		       lenprec, scale, val, 0, lenp);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Return number of parameters.
 * @param stmt statement handle
 * @param nparam output parameter count
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLNumParams(SQLHSTMT stmt, SQLSMALLINT *nparam)
{
    STMT *s;
    SQLSMALLINT dummy;

    HSTMT_LOCK(stmt);
    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (!nparam) {
	nparam = &dummy;
    }
    *nparam = s->nparams;
    HSTMT_UNLOCK(stmt);
    return SQL_SUCCESS;
}

/**
 * Setup parameter buffer for deferred parameter.
 * @param s pointer to STMT
 * @param p pointer to BINDPARM
 * @result ODBC error code (success indicated by SQL_NEED_DATA)
 */

static SQLRETURN
setupparbuf(STMT *s, BINDPARM *p)
{
    if (!p->parbuf) {
	if (*p->lenp == SQL_DATA_AT_EXEC) {
	    p->len = p->max;
	} else {
	    p->len = SQL_LEN_DATA_AT_EXEC(*p->lenp);
	}
	if (p->len < 0 && p->len != SQL_NTS &&
	    p->len != SQL_NULL_DATA) {
	    setstat(s, -1, "invalid length", "HY009");
	    return SQL_ERROR;
	}
	if (p->len >= 0) {
	    p->parbuf = xmalloc(p->len + 1);
	    if (!p->parbuf) {
		return nomem(s);
	    }
	    p->param = p->parbuf;
	} else {
	    p->param = NULL;
	}
    }
    return SQL_NEED_DATA;
}

/**
 * Retrieve next parameter for sending data to executing query.
 * @param stmt statement handle
 * @param pind pointer to output parameter indicator
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLParamData(SQLHSTMT stmt, SQLPOINTER *pind)
{
    STMT *s;
    int i;
    SQLPOINTER dummy;
    SQLRETURN ret;
    BINDPARM *p;

    HSTMT_LOCK(stmt);
    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (!pind) {
	pind = &dummy;
    }
    if (s->pdcount < s->nparams) {
	s->pdcount++;
    }
    for (i = 0; i < s->pdcount; i++) {
	p = &s->bindparms[i];
	if (p->need > 0) {
	    p->need = -1;
	}
    }
    for (; i < s->nparams; i++) {
	p = &s->bindparms[i];
	if (p->need > 0) {
	    *pind = (SQLPOINTER) p->param0;
	    ret = setupparbuf(s, p);
	    s->pdcount = i;
	    goto done;
	}
    }
    ret = drvexecute(stmt, 0);
done:
    HSTMT_UNLOCK(stmt);
    return ret;
}

/**
 * Return information about parameter.
 * @param stmt statement handle
 * @param pnum parameter number, starting at 1
 * @param dtype output type indicator
 * @param size output size indicator
 * @param decdigits output number of digits
 * @param nullable output NULL allowed indicator
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLDescribeParam(SQLHSTMT stmt, SQLUSMALLINT pnum, SQLSMALLINT *dtype,
		 SQLULEN *size, SQLSMALLINT *decdigits, SQLSMALLINT *nullable)
{
    STMT *s;
    SQLRETURN ret = SQL_ERROR;

    HSTMT_LOCK(stmt);
    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    --pnum;
    if (pnum >= s->nparams) {
	setstat(s, -1, "invalid parameter index",
		(*s->ov3) ? "HY000" : "S1000");
	goto done;
    }
    if (dtype) {
#ifdef SQL_LONGVARCHAR
#ifdef WINTERFACE
	*dtype = s->nowchar[0] ? SQL_LONGVARCHAR : SQL_WLONGVARCHAR;
#else
	*dtype = SQL_LONGVARCHAR;
#endif
#else
#ifdef WINTERFACE
	*dtype = s->nowchar[0] ? SQL_VARCHAR : SQL_WVARCHAR;
#else
	*dtype = SQL_VARCHAR;
#endif
#endif
    }
    if (size) {
#ifdef SQL_LONGVARCHAR
	*size = 65536;
#else
	*size = 255;
#endif
    }
    if (decdigits) {
	*decdigits = 0;
    }
    if (nullable) {
	*nullable = SQL_NULLABLE;
    }
    ret = SQL_SUCCESS;
done:
    HSTMT_UNLOCK(stmt);
    return ret;
}

/**
 * Set information on parameter.
 * @param stmt statement handle
 * @param par parameter number, starting at 1
 * @param type type of host variable
 * @param sqltype
 * @param coldef
 * @param scale
 * @param val pointer to host variable
 * @param len output length pointer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetParam(SQLHSTMT stmt, SQLUSMALLINT par, SQLSMALLINT type,
	    SQLSMALLINT sqltype, SQLULEN coldef,
	    SQLSMALLINT scale, SQLPOINTER val, SQLLEN *nval)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvbindparam(stmt, par, SQL_PARAM_INPUT,
		       type, sqltype, coldef, scale, val,
		       SQL_SETPARAM_VALUE_MAX, nval);
    HSTMT_UNLOCK(stmt);
    return ret;
}

/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLParamOptions(SQLHSTMT stmt, SQLULEN rows, SQLULEN *rowp)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvunimplstmt(stmt);
    HSTMT_UNLOCK(stmt);
    return ret;
}

#ifndef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLGetDescField(SQLHDESC handle, SQLSMALLINT recno,
		SQLSMALLINT fieldid, SQLPOINTER value,
		SQLINTEGER buflen, SQLINTEGER *strlen)
{
    return SQL_ERROR;
}
#endif

#ifdef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLGetDescFieldW(SQLHDESC handle, SQLSMALLINT recno,
		 SQLSMALLINT fieldid, SQLPOINTER value,
		 SQLINTEGER buflen, SQLINTEGER *strlen)
{
    return SQL_ERROR;
}
#endif

#ifndef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLSetDescField(SQLHDESC handle, SQLSMALLINT recno,
		SQLSMALLINT fieldid, SQLPOINTER value,
		SQLINTEGER buflen)
{
    return SQL_ERROR;
}
#endif

#ifdef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLSetDescFieldW(SQLHDESC handle, SQLSMALLINT recno,
		 SQLSMALLINT fieldid, SQLPOINTER value,
		 SQLINTEGER buflen)
{
    return SQL_ERROR;
}
#endif

#ifndef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLGetDescRec(SQLHDESC handle, SQLSMALLINT recno,
	      SQLCHAR *name, SQLSMALLINT buflen,
	      SQLSMALLINT *strlen, SQLSMALLINT *type,
	      SQLSMALLINT *subtype, SQLLEN *len,
	      SQLSMALLINT *prec, SQLSMALLINT *scale,
	      SQLSMALLINT *nullable)
{
    return SQL_ERROR;
}
#endif

#ifdef WINTERFACE
/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLGetDescRecW(SQLHDESC handle, SQLSMALLINT recno,
	       SQLWCHAR *name, SQLSMALLINT buflen,
	       SQLSMALLINT *strlen, SQLSMALLINT *type,
	       SQLSMALLINT *subtype, SQLLEN *len,
	       SQLSMALLINT *prec, SQLSMALLINT *scale,
	       SQLSMALLINT *nullable)
{
    return SQL_ERROR;
}
#endif

/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLSetDescRec(SQLHDESC handle, SQLSMALLINT recno,
	      SQLSMALLINT type, SQLSMALLINT subtype,
	      SQLLEN len, SQLSMALLINT prec,
	      SQLSMALLINT scale, SQLPOINTER data,
	      SQLLEN *strlen, SQLLEN *indicator)
{
    return SQL_ERROR;
}

/**
 * Setup empty result set from constant column specification.
 * @param stmt statement handle
 * @param colspec column specification array (default, ODBC2)
 * @param ncols number of columns (default, ODBC2)
 * @param colspec3 column specification array (ODBC3)
 * @param ncols3 number of columns (ODBC3)
 * @param nret returns number of columns
 * @result ODBC error code
 */

static SQLRETURN
mkresultset(HSTMT stmt, COL *colspec, int ncols, COL *colspec3,
	    int ncols3, int *nret)
{
    STMT *s;
    DBC *d;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (s->dbc == SQL_NULL_HDBC) {
noconn:
	return noconn(s);
    }
    d = (DBC *) s->dbc;
    if (!d->sqlite) {
	goto noconn;
    }
    vm_end_if(s);
    freeresult(s, 0);
    if (colspec3 && *s->ov3) {
	s->ncols = ncols3;
	s->cols = colspec3;
    } else {
	s->ncols = ncols;
	s->cols = colspec;
    }
    mkbindcols(s, s->ncols);
    s->nowchar[1] = 1;
    s->nrows = 0;
    s->rowp = -1;
    s->isselect = -1;
    if (nret) {
	*nret = s->ncols;
    }
    return SQL_SUCCESS;
}

/**
 * Columns for result set of SQLTablePrivileges().
 */

static COL tablePrivSpec2[] = {
    { "SYSTEM", "TABLEPRIV", "TABLE_QUALIFIER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TABLEPRIV", "TABLE_OWNER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TABLEPRIV", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "TABLEPRIV", "GRANTOR", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TABLEPRIV", "GRANTEE", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TABLEPRIV", "PRIVILEGE", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TABLEPRIV", "IS_GRANTABLE", SCOL_VARCHAR, 50 }
};

static COL tablePrivSpec3[] = {
    { "SYSTEM", "TABLEPRIV", "TABLE_CAT", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TABLEPRIV", "TABLE_SCHEM", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TABLEPRIV", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "TABLEPRIV", "GRANTOR", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TABLEPRIV", "GRANTEE", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TABLEPRIV", "PRIVILEGE", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TABLEPRIV", "IS_GRANTABLE", SCOL_VARCHAR, 50 }
};

/**
 * Retrieve privileges on tables and/or views.
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @result ODBC error code
 */

static SQLRETURN
drvtableprivileges(SQLHSTMT stmt,
		   SQLCHAR *cat, SQLSMALLINT catLen,
		   SQLCHAR *schema, SQLSMALLINT schemaLen,
		   SQLCHAR *table, SQLSMALLINT tableLen)
{
    SQLRETURN ret;
    STMT *s;
    DBC *d;
    int ncols, rc, size, npatt;
    char *errp = NULL, tname[512];

    ret = mkresultset(stmt, tablePrivSpec2, array_size(tablePrivSpec2),
		      tablePrivSpec3, array_size(tablePrivSpec3), NULL);
    if (ret != SQL_SUCCESS) {
	return ret;
    }
    s = (STMT *) stmt;
    d = (DBC *) s->dbc;
    if (cat && (catLen > 0 || catLen == SQL_NTS) && cat[0] == '%') {
	table = NULL;
	goto doit;
    }
    if (schema && (schemaLen > 0 || schemaLen == SQL_NTS) &&
	schema[0] == '%') {
	if ((!cat || catLen == 0 || !cat[0]) &&
	    (!table || tableLen == 0 || !table[0])) {
	    table = NULL;
	    goto doit;
	}
    }
doit:
    if (!table) {
	size = 1;
	tname[0] = '%';
    } else {
	if (tableLen == SQL_NTS) {
	    size = sizeof (tname) - 1;
	} else {
	    size = min(sizeof (tname) - 1, tableLen);
	}
	strncpy(tname, (char *) table, size);
    }
    tname[size] = '\0';
    npatt = unescpat(tname);
    ret = starttran(s);
    if (ret != SQL_SUCCESS) {
	return ret;
    }
#if defined(_WIN32) || defined(_WIN64)
    rc = sqlite_get_table_printf(d->sqlite,
				 "select %s as 'TABLE_QUALIFIER', "
				 "%s as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "'' as 'GRANTOR', "
				 "'' as 'GRANTEE', "
				 "'SELECT' AS 'PRIVILEGE', "
				 "NULL as 'IS_GRANTABLE' "
				 "from sqlite_master where "
				 "(type = 'table' or type = 'view') "
				 "and tbl_name %s '%q' "
				 "UNION "
				 "select %s as 'TABLE_QUALIFIER', "
				 "%s as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "'' as 'GRANTOR', "
				 "'' as 'GRANTEE', "
				 "'UPDATE' AS 'PRIVILEGE', "
				 "NULL as 'IS_GRANTABLE' "
				 "from sqlite_master where "
				 "(type = 'table' or type = 'view') "
				 "and tbl_name %s '%q' "
				 "UNION "
				 "select %s as 'TABLE_QUALIFIER', "
				 "%s as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "'' as 'GRANTOR', "
				 "'' as 'GRANTEE', "
				 "'DELETE' AS 'PRIVILEGE', "
				 "NULL as 'IS_GRANTABLE' "
				 "from sqlite_master where "
				 "(type = 'table' or type = 'view') "
				 "and tbl_name %s '%q' "
				 "UNION "
				 "select %s as 'TABLE_QUALIFIER', "
				 "%s as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "'' as 'GRANTOR', "
				 "'' as 'GRANTEE', "
				 "'INSERT' AS 'PRIVILEGE', "
				 "NULL as 'IS_GRANTABLE' "
				 "from sqlite_master where "
				 "(type = 'table' or type = 'view') "
				 "and tbl_name %s '%q' "
				 "UNION "
				 "select %s as 'TABLE_QUALIFIER', "
				 "%s as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "'' as 'GRANTOR', "
				 "'' as 'GRANTEE', "
				 "'REFERENCES' AS 'PRIVILEGE', "
				 "NULL as 'IS_GRANTABLE' "
				 "from sqlite_master where "
				 "(type = 'table' or type = 'view') "
				 "and tbl_name %s '%q'",
				 &s->rows, &s->nrows, &ncols, &errp,
				 d->xcelqrx ? "''" : "NULL",
				 d->xcelqrx ? "'main'" : "NULL",
				 npatt ? "like" : "=", tname,
				 d->xcelqrx ? "''" : "NULL",
				 d->xcelqrx ? "'main'" : "NULL",
				 npatt ? "like" : "=", tname,
				 d->xcelqrx ? "''" : "NULL",
				 d->xcelqrx ? "'main'" : "NULL",
				 npatt ? "like" : "=", tname,
				 d->xcelqrx ? "''" : "NULL",
				 d->xcelqrx ? "'main'" : "NULL",
				 npatt ? "like" : "=", tname,
				 d->xcelqrx ? "''" : "NULL",
				 d->xcelqrx ? "'main'" : "NULL",
				 npatt ? "like" : "=", tname);
#else
    rc = sqlite_get_table_printf(d->sqlite,
				 "select NULL as 'TABLE_QUALIFIER', "
				 "NULL as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "'' as 'GRANTOR', "
				 "'' as 'GRANTEE', "
				 "'SELECT' AS 'PRIVILEGE', "
				 "NULL as 'IS_GRANTABLE' "
				 "from sqlite_master where "
				 "(type = 'table' or type = 'view') "
				 "and tbl_name %s '%q' "
				 "UNION "
				 "select NULL as 'TABLE_QUALIFIER', "
				 "NULL as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "'' as 'GRANTOR', "
				 "'' as 'GRANTEE', "
				 "'UPDATE' AS 'PRIVILEGE', "
				 "NULL as 'IS_GRANTABLE' "
				 "from sqlite_master where "
				 "(type = 'table' or type = 'view') "
				 "and tbl_name %s '%q' "
				 "UNION "
				 "select NULL as 'TABLE_QUALIFIER', "
				 "NULL as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "'' as 'GRANTOR', "
				 "'' as 'GRANTEE', "
				 "'DELETE' AS 'PRIVILEGE', "
				 "NULL as 'IS_GRANTABLE' "
				 "from sqlite_master where "
				 "(type = 'table' or type = 'view') "
				 "and tbl_name %s '%q' "
				 "UNION "
				 "select NULL as 'TABLE_QUALIFIER', "
				 "NULL as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "'' as 'GRANTOR', "
				 "'' as 'GRANTEE', "
				 "'INSERT' AS 'PRIVILEGE', "
				 "NULL as 'IS_GRANTABLE' "
				 "from sqlite_master where "
				 "(type = 'table' or type = 'view') "
				 "and tbl_name %s '%q' "
				 "UNION "
				 "select NULL as 'TABLE_QUALIFIER', "
				 "NULL as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "'' as 'GRANTOR', "
				 "'' as 'GRANTEE', "
				 "'REFERENCES' AS 'PRIVILEGE', "
				 "NULL as 'IS_GRANTABLE' "
				 "from sqlite_master where "
				 "(type = 'table' or type = 'view') "
				 "and tbl_name %s '%q'",
				 &s->rows, &s->nrows, &ncols, &errp,
				 npatt ? "like" : "=", tname,
				 npatt ? "like" : "=", tname,
				 npatt ? "like" : "=", tname,
				 npatt ? "like" : "=", tname,
				 npatt ? "like" : "=", tname);
#endif
    if (rc == SQLITE_OK) {
	if (ncols != s->ncols) {
	    freeresult(s, 0);
	    s->nrows = 0;
	} else {
	    s->rowfree = sqlite_free_table;
	}
    } else {
	s->nrows = 0;
	s->rows = NULL;
	s->rowfree = NULL;
    }
    if (errp) {
	sqlite_freemem(errp);
	errp = NULL;
    }
    s->rowp = -1;
    return SQL_SUCCESS;
}


#if !defined(WINTERFACE) || (defined(HAVE_UNIXODBC) && (HAVE_UNIXODBC))
/**
 * Retrieve privileges on tables and/or views.
 * @param stmt statement handle
 * @param catalog catalog name/pattern or NULL
 * @param catalogLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLTablePrivileges(SQLHSTMT stmt,
		   SQLCHAR *catalog, SQLSMALLINT catalogLen,
		   SQLCHAR *schema, SQLSMALLINT schemaLen,
		   SQLCHAR *table, SQLSMALLINT tableLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvtableprivileges(stmt, catalog, catalogLen, schema, schemaLen,
			     table, tableLen);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#if !defined(HAVE_UNIXODBC) || !(HAVE_UNIXODBC)
#ifdef WINTERFACE
/**
 * Retrieve privileges on tables and/or views (UNICODE version).
 * @param stmt statement handle
 * @param catalog catalog name/pattern or NULL
 * @param catalogLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLTablePrivilegesW(SQLHSTMT stmt,
		    SQLWCHAR *catalog, SQLSMALLINT catalogLen,
		    SQLWCHAR *schema, SQLSMALLINT schemaLen,
		    SQLWCHAR *table, SQLSMALLINT tableLen)
{
    char *c = NULL, *s = NULL, *t = NULL;
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    if (catalog) {
	c = uc_to_utf_c(catalog, catalogLen);
	if (!c) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (schema) {
	s = uc_to_utf_c(schema, schemaLen);
	if (!s) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (table) {
	t = uc_to_utf_c(table, tableLen);
	if (!t) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    ret = drvtableprivileges(stmt, (SQLCHAR *) c, SQL_NTS,
			     (SQLCHAR *) s, SQL_NTS,
			     (SQLCHAR *) t, SQL_NTS);
done:
    HSTMT_UNLOCK(stmt);
    uc_free(t);
    uc_free(s);
    uc_free(c);
    return ret;
}
#endif
#endif

/**
 * Columns for result set of SQLColumnPrivileges().
 */

static COL colPrivSpec2[] = {
    { "SYSTEM", "COLPRIV", "TABLE_QUALIFIER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLPRIV", "TABLE_OWNER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLPRIV", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLPRIV", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLPRIV", "GRANTOR", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLPRIV", "GRANTEE", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLPRIV", "PRIVILEGE", SCOL_VARCHAR, 50 }
};

static COL colPrivSpec3[] = {
    { "SYSTEM", "COLPRIV", "TABLE_CAT", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLPRIV", "TABLE_SCHEM", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLPRIV", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLPRIV", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLPRIV", "GRANTOR", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLPRIV", "GRANTEE", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLPRIV", "PRIVILEGE", SCOL_VARCHAR, 50 }
};

#if !defined(WINTERFACE) || (defined(HAVE_UNIXODBC) && (HAVE_UNIXODBC))
/**
 * Retrieve privileges on columns.
 * @param stmt statement handle
 * @param catalog catalog name/pattern or NULL
 * @param catalogLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param column column name or NULL
 * @param columnLen length of column name or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLColumnPrivileges(SQLHSTMT stmt,
		    SQLCHAR *catalog, SQLSMALLINT catalogLen,
		    SQLCHAR *schema, SQLSMALLINT schemaLen,
		    SQLCHAR *table, SQLSMALLINT tableLen,
		    SQLCHAR *column, SQLSMALLINT columnLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = mkresultset(stmt, colPrivSpec2, array_size(colPrivSpec2),
		      colPrivSpec3, array_size(colPrivSpec3), NULL);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#if !defined(HAVE_UNIXODBC) || !(HAVE_UNIXODBC)
#ifdef WINTERFACE
/**
 * Retrieve privileges on columns (UNICODE version).
 * @param stmt statement handle
 * @param catalog catalog name/pattern or NULL
 * @param catalogLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param column column name or NULL
 * @param columnLen length of column name or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLColumnPrivilegesW(SQLHSTMT stmt,
		     SQLWCHAR *catalog, SQLSMALLINT catalogLen,
		     SQLWCHAR *schema, SQLSMALLINT schemaLen,
		     SQLWCHAR *table, SQLSMALLINT tableLen,
		     SQLWCHAR *column, SQLSMALLINT columnLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = mkresultset(stmt, colPrivSpec2, array_size(colPrivSpec2),
		      colPrivSpec3, array_size(colPrivSpec3), NULL);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif
#endif

/**
 * Columns for result set of SQLPrimaryKeys().
 */

static COL pkeySpec2[] = {
    { "SYSTEM", "PRIMARYKEY", "TABLE_QUALIFIER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PRIMARYKEY", "TABLE_OWNER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PRIMARYKEY", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PRIMARYKEY", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PRIMARYKEY", "KEY_SEQ", SQL_SMALLINT, 50 },
    { "SYSTEM", "PRIMARYKEY", "PK_NAME", SCOL_VARCHAR, 50 }
};

static COL pkeySpec3[] = {
    { "SYSTEM", "PRIMARYKEY", "TABLE_CAT", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PRIMARYKEY", "TABLE_SCHEM", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PRIMARYKEY", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PRIMARYKEY", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PRIMARYKEY", "KEY_SEQ", SQL_SMALLINT, 50 },
    { "SYSTEM", "PRIMARYKEY", "PK_NAME", SCOL_VARCHAR, 50 }
};

/**
 * Internal retrieve information about indexed columns.
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @result ODBC error code
 */

static SQLRETURN
drvprimarykeys(SQLHSTMT stmt,
	       SQLCHAR *cat, SQLSMALLINT catLen,
	       SQLCHAR *schema, SQLSMALLINT schemaLen,
	       SQLCHAR *table, SQLSMALLINT tableLen)
{
    STMT *s;
    DBC *d;
    SQLRETURN sret;
    int i, asize, ret, nrows, ncols, nrows2 = 0, ncols2 = 0;
    int namec = -1, uniquec = -1, namec2 = -1, uniquec2 = -1, offs, seq = 1;
    PTRDIFF_T size;
    char **rowp = NULL, **rowp2 = NULL, *errp = NULL, tname[512];

    sret = mkresultset(stmt, pkeySpec2, array_size(pkeySpec2),
		       pkeySpec3, array_size(pkeySpec3), &asize);
    if (sret != SQL_SUCCESS) {
	return sret;
    }
    s = (STMT *) stmt;
    d = (DBC *) s->dbc;
    if (!table || table[0] == '\0' || table[0] == '%') {
	setstat(s, -1, "need table name", (*s->ov3) ? "HY000" : "S1000");
	return SQL_ERROR;
    }
    if (tableLen == SQL_NTS) {
	size = sizeof (tname) - 1;
    } else {
	size = min(sizeof (tname) - 1, tableLen);
    }
    strncpy(tname, (char *) table, size);
    tname[size] = '\0';
    unescpat(tname);
    sret = starttran(s);
    if (sret != SQL_SUCCESS) {
	return sret;
    }
    ret = sqlite_get_table_printf(d->sqlite,
				  "PRAGMA table_info('%q')", &rowp,
				  &nrows, &ncols, &errp, tname);
    if (ret != SQLITE_OK) {
	setstat(s, ret, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		errp ? errp : "unknown error", ret);
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	return SQL_ERROR;
    }
    if (errp) {
	sqlite_freemem(errp);
	errp = NULL;
    }
    size = 0;
    if (ncols * nrows > 0) {
	int typec;

	namec = findcol(rowp, ncols, "name");
	uniquec = findcol(rowp, ncols, "pk");
	typec = findcol(rowp, ncols, "type");
	if (namec >= 0 && uniquec >= 0 && typec >= 0) {
	    for (i = 1; i <= nrows; i++) {
		if (*rowp[i * ncols + uniquec] != '0') {
		    size++;
		}
	    }
	}
    }
    if (size == 0) {
	ret = sqlite_get_table_printf(d->sqlite,
				      "PRAGMA index_list('%q')", &rowp2,
				      &nrows2, &ncols2, &errp, tname);
	if (ret != SQLITE_OK) {
	    sqlite_free_table(rowp);
	    sqlite_free_table(rowp2);
	    setstat(s, ret, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		    errp ? errp : "unknown error", ret);
	    if (errp) {
		sqlite_freemem(errp);
		errp = NULL;
	    }
	    return SQL_ERROR;
	}
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
    }
    if (ncols2 * nrows2 > 0) {
	namec2 = findcol(rowp2, ncols, "name");
	uniquec2 = findcol(rowp2, ncols, "unique");
	if (namec2 >= 0 && uniquec2 >= 0) {
	    for (i = 1; i <= nrows2; i++) {
		int nnrows, nncols;
		char **rowpp;

		if (*rowp2[i * ncols2 + namec2] != '(' ||
		    !strstr(rowp2[i * ncols2 + namec2], " autoindex ")) {
		    continue;
		}
		if (*rowp2[i * ncols2 + uniquec2] != '0') {
		    ret = sqlite_get_table_printf(d->sqlite,
						  "PRAGMA index_info('%q')",
						  &rowpp, &nnrows, &nncols,
						  NULL,
						  rowp2[i * ncols2 + namec2]);
		    if (ret == SQLITE_OK) {
			size += nnrows;
			sqlite_free_table(rowpp);
		    }
		}
	    }
	}
    }
    if (size == 0) {
	sqlite_free_table(rowp);
	sqlite_free_table(rowp2);
	return SQL_SUCCESS;
    }
    s->nrows = size;
    size = (size + 1) * asize;
    s->rows = xmalloc((size + 1) * sizeof (char *));
    if (!s->rows) {
	s->nrows = 0;
	sqlite_free_table(rowp);
	sqlite_free_table(rowp2);
	return nomem(s);
    }
    s->rows[0] = (char *) size;
    s->rows += 1;
    memset(s->rows, 0, sizeof (char *) * size);
    s->rowfree = freerows;
    offs = s->ncols;
    if (rowp) {
	for (i = 1; i <= nrows; i++) {
	    if (*rowp[i * ncols + uniquec] != '0') {
		char buf[32];

		s->rows[offs + 0] = xstrdup("");
#if defined(_WIN32) || defined(_WIN64)
		s->rows[offs + 1] = xstrdup(d->xcelqrx ? "main" : "");
#else
		s->rows[offs + 1] = xstrdup("");
#endif
		s->rows[offs + 2] = xstrdup(tname);
		s->rows[offs + 3] = xstrdup(rowp[i * ncols + namec]);
		sprintf(buf, "%d", seq++);
		s->rows[offs + 4] = xstrdup(buf);
		offs += s->ncols;
	    }
	}
    }
    if (rowp2) {
	for (i = 1; i <= nrows2; i++) {
	    int nnrows, nncols;
	    char **rowpp;

	    if (*rowp2[i * ncols2 + namec2] != '(' ||
		!strstr(rowp2[i * ncols2 + namec2], " autoindex ")) {
		continue;
	    }
	    if (*rowp2[i * ncols2 + uniquec2] != '0') {
		int k;

		ret = sqlite_get_table_printf(d->sqlite,
					      "PRAGMA index_info('%q')",
					      &rowpp,
					      &nnrows, &nncols, NULL,
					      rowp2[i * ncols2 + namec2]);
		if (ret != SQLITE_OK) {
		    continue;
		}
		for (k = 0; nnrows && k < nncols; k++) {
		    if (strcmp(rowpp[k], "name") == 0) {
			int m;

			for (m = 1; m <= nnrows; m++) {
			    int roffs = offs + (m - 1) * s->ncols;

			    s->rows[roffs + 0] = xstrdup("");
#if defined(_WIN32) || defined(_WIN64)
			    s->rows[roffs + 1] =
				xstrdup(d->xcelqrx ? "main" : "");
#else
			    s->rows[roffs + 1] = xstrdup("");
#endif
			    s->rows[roffs + 2] = xstrdup(tname);
			    s->rows[roffs + 3] =
				xstrdup(rowpp[m * nncols + k]);
			    s->rows[roffs + 5] =
				xstrdup(rowp2[i * ncols2 + namec2]);
			}
		    } else if (strcmp(rowpp[k], "seqno") == 0) {
			int m;

			for (m = 1; m <= nnrows; m++) {
			    int roffs = offs + (m - 1) * s->ncols;
			    int pos = m - 1;
			    char buf[32];

			    sscanf(rowpp[m * nncols + k], "%d", &pos);
			    sprintf(buf, "%d", pos + 1);
			    s->rows[roffs + 4] = xstrdup(buf);
			}
		    }
		}
		offs += nnrows * s->ncols;
		sqlite_free_table(rowpp);
	    }
	}
    }
    sqlite_free_table(rowp);
    sqlite_free_table(rowp2);
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Retrieve information about indexed columns.
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLPrimaryKeys(SQLHSTMT stmt,
	       SQLCHAR *cat, SQLSMALLINT catLen,
	       SQLCHAR *schema, SQLSMALLINT schemaLen,
	       SQLCHAR *table, SQLSMALLINT tableLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvprimarykeys(stmt, cat, catLen, schema, schemaLen,
			 table, tableLen);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Retrieve information about indexed columns (UNICODE version).
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLPrimaryKeysW(SQLHSTMT stmt,
		SQLWCHAR *cat, SQLSMALLINT catLen,
		SQLWCHAR *schema, SQLSMALLINT schemaLen,
		SQLWCHAR *table, SQLSMALLINT tableLen)
{
    char *c = NULL, *s = NULL, *t = NULL;
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    if (cat) {
	c = uc_to_utf_c(cat, catLen);
	if (!c) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (schema) {
	s = uc_to_utf_c(schema, schemaLen);
	if (!s) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (table) {
	t = uc_to_utf_c(table, tableLen);
	if (!t) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    ret = drvprimarykeys(stmt, (SQLCHAR *) c, SQL_NTS,
			 (SQLCHAR *) s, SQL_NTS, (SQLCHAR *) t, SQL_NTS);
done:
    HSTMT_UNLOCK(stmt);
    uc_free(t);
    uc_free(s);
    uc_free(c);
    return ret;
}
#endif

/**
 * Columns for result set of SQLSpecialColumns().
 */

static COL scolSpec2[] = {
    { "SYSTEM", "COLUMN", "SCOPE", SQL_SMALLINT, 1 },
    { "SYSTEM", "COLUMN", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLUMN", "DATA_TYPE", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "TYPE_NAME", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "PRECISION", SQL_INTEGER, 50 },
    { "SYSTEM", "COLUMN", "LENGTH", SQL_INTEGER, 50 },
    { "SYSTEM", "COLUMN", "DECIMAL_DIGITS", SQL_INTEGER, 50 },
    { "SYSTEM", "COLUMN", "PSEUDO_COLUMN", SQL_SMALLINT, 1 },
    { "SYSTEM", "COLUMN", "NULLABLE", SQL_SMALLINT, 1 }
};

static COL scolSpec3[] = {
    { "SYSTEM", "COLUMN", "SCOPE", SQL_SMALLINT, 1 },
    { "SYSTEM", "COLUMN", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLUMN", "DATA_TYPE", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "TYPE_NAME", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "COLUMN_SIZE", SQL_INTEGER, 50 },
    { "SYSTEM", "COLUMN", "BUFFER_LENGTH", SQL_INTEGER, 50 },
    { "SYSTEM", "COLUMN", "DECIMAL_DIGITS", SQL_INTEGER, 50 },
    { "SYSTEM", "COLUMN", "PSEUDO_COLUMN", SQL_SMALLINT, 1 },
    { "SYSTEM", "COLUMN", "NULLABLE", SQL_SMALLINT, 1 }
};

/**
 * Internal retrieve information about indexed columns.
 * @param stmt statement handle
 * @param id type of information, e.g. best row id
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param scope
 * @param nullable
 * @result ODBC error code
 */

static SQLRETURN
drvspecialcolumns(SQLHSTMT stmt, SQLUSMALLINT id,
		  SQLCHAR *cat, SQLSMALLINT catLen,
		  SQLCHAR *schema, SQLSMALLINT schemaLen,
		  SQLCHAR *table, SQLSMALLINT tableLen,
		  SQLUSMALLINT scope, SQLUSMALLINT nullable)
{
    STMT *s;
    DBC *d;
    SQLRETURN sret;
    int i, asize, ret, nrows, ncols, nnnrows, nnncols, offs;
    int namec = -1, uniquec = -1, namecc = -1, typecc = -1;
    int notnullcc = -1, mkrowid = 0;
    PTRDIFF_T size;
    char *errp = NULL, tname[512];
    char **rowp = NULL, **rowppp = NULL;

    sret = mkresultset(stmt, scolSpec2, array_size(scolSpec2),
		       scolSpec3, array_size(scolSpec3), &asize);
    if (sret != SQL_SUCCESS) {
	return sret;
    }
    s = (STMT *) stmt;
    d = (DBC *) s->dbc;
    if (!table || table[0] == '\0' || table[0] == '%') {
	setstat(s, -1, "need table name", (*s->ov3) ? "HY000" : "S1000");
	return SQL_ERROR;
    }
    if (tableLen == SQL_NTS) {
	size = sizeof (tname) - 1;
    } else {
	size = min(sizeof (tname) - 1, tableLen);
    }
    strncpy(tname, (char *) table, size);
    tname[size] = '\0';
    unescpat(tname);
    if (id != SQL_BEST_ROWID) {
	return SQL_SUCCESS;
    }
    sret = starttran(s);
    if (sret != SQL_SUCCESS) {
	return sret;
    }
    ret = sqlite_get_table_printf(d->sqlite, "PRAGMA index_list('%q')",
				  &rowp, &nrows, &ncols, &errp, tname);
    if (ret != SQLITE_OK) {
doerr:
	setstat(s, ret, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		errp ? errp : "unknown error", ret);
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	return SQL_ERROR;	
    }
    if (errp) {
	sqlite_freemem(errp);
	errp = NULL;
    }
    size = 0;	/* number result rows */
    if (ncols * nrows <= 0) {
	goto nodata_but_rowid;
    }
    ret = sqlite_get_table_printf(d->sqlite, "PRAGMA table_info('%q')",
				  &rowppp, &nnnrows, &nnncols, &errp, tname);
    if (ret != SQLITE_OK) {
	sqlite_free_table(rowp);
	goto doerr;
    }
    if (errp) {
	sqlite_freemem(errp);
	errp = NULL;
    }
    namec = findcol(rowp, ncols, "name");
    uniquec = findcol(rowp, ncols, "unique");
    if (namec < 0 || uniquec < 0) {
	goto nodata_but_rowid;
    }
    namecc = findcol(rowppp, nnncols, "name");
    typecc = findcol(rowppp, nnncols, "type");
    notnullcc = findcol(rowppp, nnncols, "notnull");
    for (i = 1; i <= nrows; i++) {
	int nnrows, nncols;
	char **rowpp;

	if (*rowp[i * ncols + uniquec] != '0') {
	    ret = sqlite_get_table_printf(d->sqlite,
					  "PRAGMA index_info('%q')", &rowpp,
					  &nnrows, &nncols, NULL,
					  rowp[i * ncols + namec]);
	    if (ret == SQLITE_OK) {
		size += nnrows;
		sqlite_free_table(rowpp);
	    }
	}
    }
nodata_but_rowid:
    if (size == 0) {
	size = 1;
	mkrowid = 1;
    }
    s->nrows = size;
    size = (size + 1) * asize;
    s->rows = xmalloc((size + 1) * sizeof (char *));
    if (!s->rows) {
	s->nrows = 0;
	sqlite_free_table(rowp);
	sqlite_free_table(rowppp);
	return nomem(s);
    }
    s->rows[0] = (char *) size;
    s->rows += 1;
    memset(s->rows, 0, sizeof (char *) * size);
    s->rowfree = freerows;
    if (mkrowid) {
	s->nrows = 0;
	goto mkrowid;
    }
    offs = 0;
    for (i = 1; i <= nrows; i++) {
	int nnrows, nncols;
	char **rowpp;

	if (*rowp[i * ncols + uniquec] != '0') {
	    int k;

	    ret = sqlite_get_table_printf(d->sqlite,
					  "PRAGMA index_info('%q')", &rowpp,
					  &nnrows, &nncols, NULL,
					  rowp[i * ncols + namec]);
	    if (ret != SQLITE_OK) {
		continue;
	    }
	    for (k = 0; nnrows && k < nncols; k++) {
		if (strcmp(rowpp[k], "name") == 0) {
		    int m;

		    for (m = 1; m <= nnrows; m++) {
			int roffs = (offs + m) * s->ncols;

			s->rows[roffs + 0] =
			    xstrdup(stringify(SQL_SCOPE_SESSION));
			s->rows[roffs + 1] = xstrdup(rowpp[m * nncols + k]);
			s->rows[roffs + 4] = xstrdup("0");
			s->rows[roffs + 7] =
			    xstrdup(stringify(SQL_PC_NOT_PSEUDO));
			if (namecc >= 0 && typecc >= 0) {
			    int ii;

			    for (ii = 1; ii <= nnnrows; ii++) {
				if (strcmp(rowppp[ii * nnncols + namecc],
					   rowpp[m * nncols + k]) == 0) {
				    char *typen = rowppp[ii * nnncols + typecc];
				    int sqltype, mm, dd, isnullable = 0;
				    char buf[32];
					
				    s->rows[roffs + 3] = xstrdup(typen);
				    sqltype = mapsqltype(typen, NULL, *s->ov3,
							 s->nowchar[0]);
				    getmd(typen, sqltype, &mm, &dd);
#ifdef SQL_LONGVARCHAR
				    if (sqltype == SQL_VARCHAR && mm > 255) {
					sqltype = SQL_LONGVARCHAR;
				    }
#endif
#ifdef WINTERFACE
#ifdef SQL_WLONGVARCHAR
				    if (sqltype == SQL_WVARCHAR && mm > 255) {
					sqltype = SQL_WLONGVARCHAR;
				    }
#endif
#endif
#if (HAVE_ENCDEC)
				    if (sqltype == SQL_VARBINARY && mm > 255) {
					sqltype = SQL_LONGVARBINARY;
				    }
#endif
				    sprintf(buf, "%d", sqltype);
				    s->rows[roffs + 2] = xstrdup(buf);
				    sprintf(buf, "%d", mm);
				    s->rows[roffs + 5] = xstrdup(buf);
				    sprintf(buf, "%d", dd);
				    s->rows[roffs + 6] = xstrdup(buf);
				    if (notnullcc >= 0) {
					char *inp =
					   rowppp[ii * nnncols + notnullcc];

					isnullable = inp[0] != '0';
				    }
				    sprintf(buf, "%d", isnullable);
				    s->rows[roffs + 8] = xstrdup(buf);
				}
			    }
			}
		    }
		}
	    }
	    offs += nnrows;
	    sqlite_free_table(rowpp);
	}
    }
    if (nullable == SQL_NO_NULLS) {
	for (i = 1; i < s->nrows; i++) {
	    if (s->rows[i * s->ncols + 8][0] == '0') {
		int m, i1 = i + 1;

		for (m = 0; m < s->ncols; m++) {
		    freep(&s->rows[i * s->ncols + m]);
		}
		size = s->ncols * sizeof (char *) * (s->nrows - i1);
		if (size > 0) {
		    memmove(s->rows + i * s->ncols,
			    s->rows + i1 * s->ncols,
			    size);
		    memset(s->rows + s->nrows * s->ncols, 0,
			   s->ncols * sizeof (char *));
		}
		s->nrows--;
		--i;
	    }
	}
    }
mkrowid:
    sqlite_free_table(rowp);
    sqlite_free_table(rowppp);
    if (s->nrows == 0) {
	s->rows[s->ncols + 0] = xstrdup(stringify(SQL_SCOPE_SESSION));
	s->rows[s->ncols + 1] = xstrdup("_ROWID_");
	s->rows[s->ncols + 2] = xstrdup(stringify(SQL_INTEGER));
	s->rows[s->ncols + 3] = xstrdup("integer");
	s->rows[s->ncols + 4] = xstrdup("0");
	s->rows[s->ncols + 5] = xstrdup("10");
	s->rows[s->ncols + 6] = xstrdup("9");
	s->rows[s->ncols + 7] = xstrdup(stringify(SQL_PC_PSEUDO));
	s->rows[s->ncols + 8] = xstrdup(stringify(SQL_FALSE));
	s->nrows = 1;
    }
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Retrieve information about indexed columns.
 * @param stmt statement handle
 * @param id type of information, e.g. best row id
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param scope
 * @param nullable
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSpecialColumns(SQLHSTMT stmt, SQLUSMALLINT id,
		  SQLCHAR *cat, SQLSMALLINT catLen,
		  SQLCHAR *schema, SQLSMALLINT schemaLen,
		  SQLCHAR *table, SQLSMALLINT tableLen,
		  SQLUSMALLINT scope, SQLUSMALLINT nullable)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvspecialcolumns(stmt, id, cat, catLen, schema, schemaLen,
			    table, tableLen, scope, nullable);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Retrieve information about indexed columns (UNICODE version).
 * @param stmt statement handle
 * @param id type of information, e.g. best row id
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param scope
 * @param nullable
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSpecialColumnsW(SQLHSTMT stmt, SQLUSMALLINT id,
		   SQLWCHAR *cat, SQLSMALLINT catLen,
		   SQLWCHAR *schema, SQLSMALLINT schemaLen,
		   SQLWCHAR *table, SQLSMALLINT tableLen,
		   SQLUSMALLINT scope, SQLUSMALLINT nullable)
{
    char *c = NULL, *s = NULL, *t = NULL;
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    if (cat) {
	c = uc_to_utf_c(cat, catLen);
	if (!c) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (schema) {
	s = uc_to_utf_c(schema, schemaLen);
	if (!s) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (table) {
	t = uc_to_utf_c(table, tableLen);
	if (!t) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    ret = drvspecialcolumns(stmt, id, (SQLCHAR *) c, SQL_NTS,
			    (SQLCHAR *) s, SQL_NTS, (SQLCHAR *) t, SQL_NTS,
			    scope, nullable);
done:
    HSTMT_UNLOCK(stmt);
    uc_free(t);
    uc_free(s);
    uc_free(c);
    return ret;
}
#endif

/**
 * Columns for result set of SQLForeignKeys().
 */

static COL fkeySpec2[] = {
    { "SYSTEM", "FOREIGNKEY", "PKTABLE_QUALIFIER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "FOREIGNKEY", "PKTABLE_OWNER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "FOREIGNKEY", "PKTABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "PKCOLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "FKTABLE_QUALIFIER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "FOREIGNKEY", "FKTABLE_OWNER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "FOREIGNKEY", "FKTABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "FKCOLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "KEY_SEQ", SQL_SMALLINT, 5 },
    { "SYSTEM", "FOREIGNKEY", "UPDATE_RULE", SQL_SMALLINT, 5 },
    { "SYSTEM", "FOREIGNKEY", "DELETE_RULE", SQL_SMALLINT, 5 },
    { "SYSTEM", "FOREIGNKEY", "FK_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "PK_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "DEFERRABILITY", SQL_SMALLINT, 5 }
};

static COL fkeySpec3[] = {
    { "SYSTEM", "FOREIGNKEY", "PKTABLE_CAT", SCOL_VARCHAR, 50 },
    { "SYSTEM", "FOREIGNKEY", "PKTABLE_SCHEM", SCOL_VARCHAR, 50 },
    { "SYSTEM", "FOREIGNKEY", "PKTABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "PKCOLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "FKTABLE_CAT", SCOL_VARCHAR, 50 },
    { "SYSTEM", "FOREIGNKEY", "FKTABLE_SCHEM", SCOL_VARCHAR, 50 },
    { "SYSTEM", "FOREIGNKEY", "FKTABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "FKCOLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "KEY_SEQ", SQL_SMALLINT, 5 },
    { "SYSTEM", "FOREIGNKEY", "UPDATE_RULE", SQL_SMALLINT, 5 },
    { "SYSTEM", "FOREIGNKEY", "DELETE_RULE", SQL_SMALLINT, 5 },
    { "SYSTEM", "FOREIGNKEY", "FK_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "PK_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "FOREIGNKEY", "DEFERRABILITY", SQL_SMALLINT, 5 }
};

/**
 * Internal retrieve information about primary/foreign keys.
 * @param stmt statement handle
 * @param PKcatalog primary key catalog name/pattern or NULL
 * @param PKcatalogLen length of PKcatalog or SQL_NTS
 * @param PKschema primary key schema name/pattern or NULL
 * @param PKschemaLen length of PKschema or SQL_NTS
 * @param PKtable primary key table name/pattern or NULL
 * @param PKtableLen length of PKtable or SQL_NTS
 * @param FKcatalog foreign key catalog name/pattern or NULL
 * @param FKcatalogLen length of FKcatalog or SQL_NTS
 * @param FKschema foreign key schema name/pattern or NULL
 * @param FKschemaLen length of FKschema or SQL_NTS
 * @param FKtable foreign key table name/pattern or NULL
 * @param FKtableLen length of FKtable or SQL_NTS
 * @result ODBC error code
 */

static SQLRETURN SQL_API
drvforeignkeys(SQLHSTMT stmt,
	       SQLCHAR *PKcatalog, SQLSMALLINT PKcatalogLen,
	       SQLCHAR *PKschema, SQLSMALLINT PKschemaLen,
	       SQLCHAR *PKtable, SQLSMALLINT PKtableLen,
	       SQLCHAR *FKcatalog, SQLSMALLINT FKcatalogLen,
	       SQLCHAR *FKschema, SQLSMALLINT FKschemaLen,
	       SQLCHAR *FKtable, SQLSMALLINT FKtableLen)
{
    STMT *s;
    DBC *d;
    SQLRETURN sret;
    int i, asize, ret, nrows, ncols, offs, namec, seqc, fromc, toc;
    PTRDIFF_T size;
    char **rowp, *errp = NULL, pname[512], fname[512];

    sret = mkresultset(stmt, fkeySpec2, array_size(fkeySpec2),
		       fkeySpec3, array_size(fkeySpec3), &asize);
    if (sret != SQL_SUCCESS) {
	return sret;
    }
    s = (STMT *) stmt;
    sret = starttran(s);
    if (sret != SQL_SUCCESS) {
	return sret;
    }
    d = (DBC *) s->dbc;
    if ((!PKtable || PKtable[0] == '\0' || PKtable[0] == '%') &&
	(!FKtable || FKtable[0] == '\0' || FKtable[0] == '%')) {
	setstat(s, -1, "need table name", (*s->ov3) ? "HY000" : "S1000");
	return SQL_ERROR;
    }
    size = 0;
    if (PKtable) {
	if (PKtableLen == SQL_NTS) {
	    size = sizeof (pname) - 1;
	} else {
	    size = min(sizeof (pname) - 1, PKtableLen);
	}
	strncpy(pname, (char *) PKtable, size);
    }
    pname[size] = '\0';
    size = 0;
    if (FKtable) {
	if (FKtableLen == SQL_NTS) {
	    size = sizeof (fname) - 1;
	} else {
	    size = min(sizeof (fname) - 1, FKtableLen);
	}
	strncpy(fname, (char *) FKtable, size);
    }
    fname[size] = '\0';
    if (fname[0] != '\0') {
	int plen;

	ret = sqlite_get_table_printf(d->sqlite,
				      "PRAGMA foreign_key_list('%q')", &rowp,
				      &nrows, &ncols, &errp, fname);
	if (ret != SQLITE_OK) {
	    setstat(s, ret, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		    errp ? errp : "unknown error", ret);
	    if (errp) {
		sqlite_freemem(errp);
		errp = NULL;
	    }
	    return SQL_ERROR;
	}
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	if (ncols * nrows <= 0) {
nodata:
	    sqlite_free_table(rowp);
	    return SQL_SUCCESS;
	}
	size = 0;
	namec = findcol(rowp, ncols, "table");
	seqc = findcol(rowp, ncols, "seq");
	fromc = findcol(rowp, ncols, "from");
	toc = findcol(rowp, ncols, "to");
	if (namec < 0 || seqc < 0 || fromc < 0 || toc < 0) {
	    goto nodata;
	}
	plen = strlen(pname);
	for (i = 1; i <= nrows; i++) {
	    char *ptab = unquote(rowp[i * ncols + namec]);

	    if (plen && ptab) {
		int len = strlen(ptab);

		if (plen != len || strncasecmp(pname, ptab, plen) != 0) {
		    continue;
		}
	    }
	    size++;
	}
	if (size == 0) {
	    goto nodata;
	}
	s->nrows = size;
	size = (size + 1) * asize;
	s->rows = xmalloc((size + 1) * sizeof (char *));
	if (!s->rows) {
	    s->nrows = 0;
	    return nomem(s);
	}
	s->rows[0] = (char *) size;
	s->rows += 1;
	memset(s->rows, 0, sizeof (char *) * size);
	s->rowfree = freerows;
	offs = 0;
	for (i = 1; i <= nrows; i++) {
	    int pos = 0, roffs = (offs + 1) * s->ncols;
	    char *ptab = rowp[i * ncols + namec];
	    char buf[32];

	    if (plen && ptab) {
		int len = strlen(ptab);

		if (plen != len || strncasecmp(pname, ptab, plen) != 0) {
		    continue;
		}
	    }
	    s->rows[roffs + 0] = xstrdup("");
#if defined(_WIN32) || defined(_WIN64)
	    s->rows[roffs + 1] = xstrdup(d->xcelqrx ? "main" : "");
#else
	    s->rows[roffs + 1] = xstrdup("");
#endif
	    s->rows[roffs + 2] = xstrdup(ptab);
	    s->rows[roffs + 3] = xstrdup(rowp[i * ncols + toc]);
	    s->rows[roffs + 4] = xstrdup("");
	    s->rows[roffs + 5] = xstrdup("");
	    s->rows[roffs + 6] = xstrdup(fname);
	    s->rows[roffs + 7] = xstrdup(rowp[i * ncols + fromc]);
	    sscanf(rowp[i * ncols + seqc], "%d", &pos);
	    sprintf(buf, "%d", pos + 1);
	    s->rows[roffs + 8] = xstrdup(buf);
	    s->rows[roffs + 9] = xstrdup(stringify(SQL_NO_ACTION));
	    s->rows[roffs + 10] = xstrdup(stringify(SQL_NO_ACTION));
	    s->rows[roffs + 11] = NULL;
	    s->rows[roffs + 12] = NULL;
	    s->rows[roffs + 13] = xstrdup(stringify(SQL_NOT_DEFERRABLE));
	    offs++;
	}
	sqlite_free_table(rowp);
    } else {
	int nnrows, nncols, plen = strlen(pname);
	char **rowpp;

	ret = sqlite_get_table(d->sqlite,
			       "select name from sqlite_master "
			       "where type='table'", &rowp,
			       &nrows, &ncols, &errp);
	if (ret != SQLITE_OK) {
	    setstat(s, ret, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		    errp ? errp : "unknown error", ret);
	    if (errp) {
		sqlite_freemem(errp);
		errp = NULL;
	    }
	    return SQL_ERROR;
	}
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	if (ncols * nrows <= 0) {
	    goto nodata;
	}
	size = 0;
	for (i = 1; i <= nrows; i++) {
	    int k;

	    if (!rowp[i]) {
		continue;
	    }
	    ret = sqlite_get_table_printf(d->sqlite,
					  "PRAGMA foreign_key_list('%q')",
					  &rowpp,
					  &nnrows, &nncols, NULL, rowp[i]);
	    if (ret != SQLITE_OK || nncols * nnrows <= 0) {
		sqlite_free_table(rowpp);
		continue;
	    }
	    namec = findcol(rowpp, nncols, "table");
	    seqc = findcol(rowpp, nncols, "seq");
	    fromc = findcol(rowpp, nncols, "from");
	    toc = findcol(rowpp, nncols, "to");
	    if (namec < 0 || seqc < 0 || fromc < 0 || toc < 0) {
		sqlite_free_table(rowpp);
		continue;
	    }
	    for (k = 1; k <= nnrows; k++) {
		char *ptab = unquote(rowpp[k * nncols + namec]);

		if (plen && ptab) {
		    int len = strlen(ptab);

		    if (len != plen || strncasecmp(pname, ptab, plen) != 0) {
			continue;
		    }
		}
		size++;
	    }
	    sqlite_free_table(rowpp);
	}
	if (size == 0) {
	    goto nodata;
	}
	s->nrows = size;
	size = (size + 1) * asize;
	s->rows = xmalloc((size + 1) * sizeof (char *));
	if (!s->rows) {
	    s->nrows = 0;
	    return nomem(s);
	}
	s->rows[0] = (char *) size;
	s->rows += 1;
	memset(s->rows, 0, sizeof (char *) * size);
	s->rowfree = freerows;
	offs = 0;
	for (i = 1; i <= nrows; i++) {
	    int k;

	    if (!rowp[i]) {
		continue;
	    }
	    ret = sqlite_get_table_printf(d->sqlite,
					  "PRAGMA foreign_key_list('%q')",
					  &rowpp,
					  &nnrows, &nncols, NULL, rowp[i]);
	    if (ret != SQLITE_OK || nncols * nnrows <= 0) {
		sqlite_free_table(rowpp);
		continue;
	    }
	    namec = findcol(rowpp, nncols, "table");
	    seqc = findcol(rowpp, nncols, "seq");
	    fromc = findcol(rowpp, nncols, "from");
	    toc = findcol(rowpp, nncols, "to");
	    if (namec < 0 || seqc < 0 || fromc < 0 || toc < 0) {
		sqlite_free_table(rowpp);
		continue;
	    }
	    for (k = 1; k <= nnrows; k++) {
		int pos = 0, roffs = (offs + 1) * s->ncols;
		char *ptab = unquote(rowpp[k * nncols + namec]);
		char buf[32];

		if (plen && ptab) {
		    int len = strlen(ptab);

		    if (len != plen || strncasecmp(pname, ptab, plen) != 0) {
			continue;
		    }
		}
		s->rows[roffs + 0] = xstrdup("");
#if defined(_WIN32) || defined(_WIN64)
		s->rows[roffs + 1] = xstrdup(d->xcelqrx ? "main" : "");
#else
		s->rows[roffs + 1] = xstrdup("");
#endif
		s->rows[roffs + 2] = xstrdup(ptab);
		s->rows[roffs + 3] = xstrdup(rowpp[k * nncols + toc]);
		s->rows[roffs + 4] = xstrdup("");
		s->rows[roffs + 5] = xstrdup("");
		s->rows[roffs + 6] = xstrdup(rowp[i]);
		s->rows[roffs + 7] = xstrdup(rowpp[k * nncols + fromc]);
		sscanf(rowpp[k * nncols + seqc], "%d", &pos);
		sprintf(buf, "%d", pos + 1);
		s->rows[roffs + 8] = xstrdup(buf);
		s->rows[roffs + 9] = xstrdup(stringify(SQL_NO_ACTION));
		s->rows[roffs + 10] = xstrdup(stringify(SQL_NO_ACTION));
		s->rows[roffs + 11] = NULL;
		s->rows[roffs + 12] = NULL;
		s->rows[roffs + 13] = xstrdup(stringify(SQL_NOT_DEFERRABLE));
		offs++;
	    }
	    sqlite_free_table(rowpp);
	}
	sqlite_free_table(rowp);
    }
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Retrieve information about primary/foreign keys.
 * @param stmt statement handle
 * @param PKcatalog primary key catalog name/pattern or NULL
 * @param PKcatalogLen length of PKcatalog or SQL_NTS
 * @param PKschema primary key schema name/pattern or NULL
 * @param PKschemaLen length of PKschema or SQL_NTS
 * @param PKtable primary key table name/pattern or NULL
 * @param PKtableLen length of PKtable or SQL_NTS
 * @param FKcatalog foreign key catalog name/pattern or NULL
 * @param FKcatalogLen length of FKcatalog or SQL_NTS
 * @param FKschema foreign key schema name/pattern or NULL
 * @param FKschemaLen length of FKschema or SQL_NTS
 * @param FKtable foreign key table name/pattern or NULL
 * @param FKtableLen length of FKtable or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLForeignKeys(SQLHSTMT stmt,
	       SQLCHAR *PKcatalog, SQLSMALLINT PKcatalogLen,
	       SQLCHAR *PKschema, SQLSMALLINT PKschemaLen,
	       SQLCHAR *PKtable, SQLSMALLINT PKtableLen,
	       SQLCHAR *FKcatalog, SQLSMALLINT FKcatalogLen,
	       SQLCHAR *FKschema, SQLSMALLINT FKschemaLen,
	       SQLCHAR *FKtable, SQLSMALLINT FKtableLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvforeignkeys(stmt,
			 PKcatalog, PKcatalogLen,
			 PKschema, PKschemaLen, PKtable, PKtableLen,
			 FKcatalog, FKcatalogLen,
			 FKschema, FKschemaLen,
			 FKtable, FKtableLen);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Retrieve information about primary/foreign keys (UNICODE version).
 * @param stmt statement handle
 * @param PKcatalog primary key catalog name/pattern or NULL
 * @param PKcatalogLen length of PKcatalog or SQL_NTS
 * @param PKschema primary key schema name/pattern or NULL
 * @param PKschemaLen length of PKschema or SQL_NTS
 * @param PKtable primary key table name/pattern or NULL
 * @param PKtableLen length of PKtable or SQL_NTS
 * @param FKcatalog foreign key catalog name/pattern or NULL
 * @param FKcatalogLen length of FKcatalog or SQL_NTS
 * @param FKschema foreign key schema name/pattern or NULL
 * @param FKschemaLen length of FKschema or SQL_NTS
 * @param FKtable foreign key table name/pattern or NULL
 * @param FKtableLen length of FKtable or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLForeignKeysW(SQLHSTMT stmt,
		SQLWCHAR *PKcatalog, SQLSMALLINT PKcatalogLen,
		SQLWCHAR *PKschema, SQLSMALLINT PKschemaLen,
		SQLWCHAR *PKtable, SQLSMALLINT PKtableLen,
		SQLWCHAR *FKcatalog, SQLSMALLINT FKcatalogLen,
		SQLWCHAR *FKschema, SQLSMALLINT FKschemaLen,
		SQLWCHAR *FKtable, SQLSMALLINT FKtableLen)
{
    char *pc = NULL, *ps = NULL, *pt = NULL;
    char *fc = NULL, *fs = NULL, *ft = NULL;
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    if (PKcatalog) {
	pc = uc_to_utf_c(PKcatalog, PKcatalogLen);
	if (!pc) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (PKschema) {
	ps = uc_to_utf_c(PKschema, PKschemaLen);
	if (!ps) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (PKtable) {
	pt = uc_to_utf_c(PKtable, PKtableLen);
	if (!pt) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (FKcatalog) {
	fc = uc_to_utf_c(FKcatalog, FKcatalogLen);
	if (!fc) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (FKschema) {
	fs = uc_to_utf_c(FKschema, FKschemaLen);
	if (!fs) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (FKtable) {
	ft = uc_to_utf_c(FKtable, FKtableLen);
	if (!ft) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    ret = drvforeignkeys(stmt, (SQLCHAR *) pc, SQL_NTS,
			 (SQLCHAR *) ps, SQL_NTS, (SQLCHAR *) pt, SQL_NTS,
			 (SQLCHAR *) fc, SQL_NTS, (SQLCHAR *) fs, SQL_NTS,
			 (SQLCHAR *) ft, SQL_NTS);
done:
    HSTMT_UNLOCK(stmt);
    uc_free(ft);
    uc_free(fs);
    uc_free(fc);
    uc_free(pt);
    uc_free(ps);
    uc_free(pc);
    return ret;
}
#endif

/**
 * Start transaction when autocommit off
 * @param s statement pointer
 * @result ODBC error code
 */

static SQLRETURN
starttran(STMT *s)
{
    int ret = SQL_SUCCESS, rc;
    char *errp = NULL;
    DBC *d = (DBC *) s->dbc;

    if (!d->autocommit && !d->intrans && !d->trans_disable) {
	rc = sqlite_exec(d->sqlite, "BEGIN TRANSACTION", NULL, NULL, &errp);
	dbtracerc(d, rc, errp);
	if (rc != SQLITE_OK) {
	    setstat(s, rc, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		    errp ? errp : "unknown error", rc);
	    ret = SQL_ERROR;
	} else {
	    d->intrans = 1;
	}
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
    }
    return ret;
}

/**
 * Internal commit or rollback transaction.
 * @param d database connection pointer
 * @param comptype type of transaction's end, SQL_COMMIT or SQL_ROLLBACK
 * @param force force action regardless of DBC's autocommit state
 * @result ODBC error code
 */

static SQLRETURN
endtran(DBC *d, SQLSMALLINT comptype, int force)
{
    int ret;
    char *sql, *errp = NULL;

    if (!d->sqlite) {
	setstatd(d, -1, "not connected", (*d->ov3) ? "HY000" : "S1000");
	return SQL_ERROR;
    }
    if ((!force && d->autocommit) || !d->intrans) {
	return SQL_SUCCESS;
    }
    switch (comptype) {
    case SQL_COMMIT:
	sql = "COMMIT TRANSACTION";
	goto doit;
    case SQL_ROLLBACK:
	sql = "ROLLBACK TRANSACTION";
    doit:
	d->intrans = 0;
	ret = sqlite_exec(d->sqlite, sql, NULL, NULL, &errp);
	dbtracerc(d, ret, errp);
	if (ret != SQLITE_OK) {
	    setstatd(d, ret, "%s", (*d->ov3) ? "HY000" : "S1000",
		     errp ? errp : "transaction failed");
	    if (errp) {
		sqlite_freemem(errp);
		errp = NULL;
	    }
	    return SQL_ERROR;
	}
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	return SQL_SUCCESS;
    }
    setstatd(d, -1, "invalid completion type", (*d->ov3) ? "HY000" : "S1000");
    return SQL_ERROR;
}

/**
 * Internal commit or rollback transaction.
 * @param type type of handle
 * @param handle HDBC, HENV, or HSTMT handle
 * @param comptype SQL_COMMIT or SQL_ROLLBACK
 * @result ODBC error code
 */

static SQLRETURN
drvendtran(SQLSMALLINT type, SQLHANDLE handle, SQLSMALLINT comptype)
{
    DBC *d;
    int fail = 0;
    SQLRETURN ret;
#if defined(_WIN32) || defined(_WIN64)
    ENV *e;
#endif

    switch (type) {
    case SQL_HANDLE_DBC:
	HDBC_LOCK((SQLHDBC) handle);
	if (handle == SQL_NULL_HDBC) {
	    return SQL_INVALID_HANDLE;
	}
	d = (DBC *) handle;
	ret = endtran(d, comptype, 0);
	HDBC_UNLOCK((SQLHDBC) handle);
	return ret;
    case SQL_HANDLE_ENV:
	if (handle == SQL_NULL_HENV) {
	    return SQL_INVALID_HANDLE;
	}
#if defined(_WIN32) || defined(_WIN64)
	e = (ENV *) handle;
	if (e->magic != ENV_MAGIC) {
	    return SQL_INVALID_HANDLE;
	}
	EnterCriticalSection(&e->cs);
	e->owner = GetCurrentThreadId();
#endif
	d = ((ENV *) handle)->dbcs;
	while (d) {
	    ret = endtran(d, comptype, 0);
	    if (ret != SQL_SUCCESS) {
		fail++;
	    }
	    d = d->next;
	}
#if defined(_WIN32) || defined(_WIN64)
	LeaveCriticalSection(&e->cs);
	e->owner = 0;
#endif
	return fail ? SQL_ERROR : SQL_SUCCESS;
    }
    return SQL_INVALID_HANDLE;
}

/**
 * Commit or rollback transaction.
 * @param type type of handle
 * @param handle HDBC, HENV, or HSTMT handle
 * @param comptype SQL_COMMIT or SQL_ROLLBACK
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLEndTran(SQLSMALLINT type, SQLHANDLE handle, SQLSMALLINT comptype)
{
    return drvendtran(type, handle, comptype);
}

/**
 * Commit or rollback transaction.
 * @param env environment handle or NULL
 * @param dbc database connection handle or NULL
 * @param type SQL_COMMIT or SQL_ROLLBACK
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLTransact(SQLHENV env, SQLHDBC dbc, SQLUSMALLINT type)
{
    if (env != SQL_NULL_HENV) {
	return drvendtran(SQL_HANDLE_ENV, (SQLHANDLE) env, type);
    }
    return drvendtran(SQL_HANDLE_DBC, (SQLHANDLE) dbc, type);
}

/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLCopyDesc(SQLHDESC source, SQLHDESC target)
{
    return SQL_ERROR;
}

#ifndef WINTERFACE
/**
 * Translate SQL string.
 * @param stmt statement handle
 * @param sqlin input string
 * @param sqlinLen length of input string
 * @param sql output string
 * @param sqlMax max space in output string
 * @param sqlLen value return for length of output string
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLNativeSql(SQLHSTMT stmt, SQLCHAR *sqlin, SQLINTEGER sqlinLen,
	     SQLCHAR *sql, SQLINTEGER sqlMax, SQLINTEGER *sqlLen)
{
    int outLen = 0;
    SQLRETURN ret = SQL_SUCCESS;

    HSTMT_LOCK(stmt);
    if (sqlinLen == SQL_NTS) {
	sqlinLen = strlen((char *) sqlin);
    }
    if (sql) {
	if (sqlMax > 0) {
	    strncpy((char *) sql, (char *) sqlin, sqlMax - 1);
	    sqlin[sqlMax - 1] = '\0';
	    outLen = min(sqlMax - 1, sqlinLen);
	}
    } else {
	outLen = sqlinLen;
    }
    if (sqlLen) {
	*sqlLen = outLen;
    }
    if (sql && outLen < sqlinLen) {
	setstat((STMT *) stmt, -1, "data right truncated", "01004");
	ret = SQL_SUCCESS_WITH_INFO;
    }
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Translate SQL string (UNICODE version).
 * @param stmt statement handle
 * @param sqlin input string
 * @param sqlinLen length of input string
 * @param sql output string
 * @param sqlMax max space in output string
 * @param sqlLen value return for length of output string
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLNativeSqlW(SQLHSTMT stmt, SQLWCHAR *sqlin, SQLINTEGER sqlinLen,
	      SQLWCHAR *sql, SQLINTEGER sqlMax, SQLINTEGER *sqlLen)
{
    int outLen = 0;
    SQLRETURN ret = SQL_SUCCESS;

    HSTMT_LOCK(stmt);
    if (sqlinLen == SQL_NTS) {
	sqlinLen = uc_strlen(sqlin);
    }
    if (sql) {
	if (sqlMax > 0) {
	    uc_strncpy(sql, sqlin, sqlMax - 1);
	    sqlin[sqlMax - 1] = 0;
	    outLen = min(sqlMax  - 1, sqlinLen);
	}
    } else {
	outLen = sqlinLen;
    }
    if (sqlLen) {
	*sqlLen = outLen;
    }
    if (sql && outLen < sqlinLen) {
	setstat((STMT *) stmt, -1, "data right truncated", "01004");
	ret = SQL_SUCCESS_WITH_INFO;
    }
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Columns for result set of SQLProcedures().
 */

static COL procSpec2[] = {
    { "SYSTEM", "PROCEDURE", "PROCEDURE_QUALIFIER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCEDURE", "PROCEDURE_OWNER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCEDURE", "PROCEDURE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PROCEDURE", "NUM_INPUT_PARAMS", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCEDURE", "NUM_OUTPUT_PARAMS", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCEDURE", "NUM_RESULT_SETS", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCEDURE", "REMARKS", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PROCEDURE", "PROCEDURE_TYPE", SQL_SMALLINT, 5 }
};

static COL procSpec3[] = {
    { "SYSTEM", "PROCEDURE", "PROCEDURE_CAT", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCEDURE", "PROCEDURE_SCHEM", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCEDURE", "PROCEDURE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PROCEDURE", "NUM_INPUT_PARAMS", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCEDURE", "NUM_OUTPUT_PARAMS", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCEDURE", "NUM_RESULT_SETS", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCEDURE", "REMARKS", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PROCEDURE", "PROCEDURE_TYPE", SQL_SMALLINT, 5 }
};

#ifndef WINTERFACE
/**
 * Retrieve information about stored procedures.
 * @param stmt statement handle
 * @param catalog catalog name/pattern or NULL
 * @param catalogLen length of catalog or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema or SQL_NTS
 * @param proc procedure name/pattern or NULL
 * @param procLen length of proc or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLProcedures(SQLHSTMT stmt,
	      SQLCHAR *catalog, SQLSMALLINT catalogLen,
	      SQLCHAR *schema, SQLSMALLINT schemaLen,
	      SQLCHAR *proc, SQLSMALLINT procLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = mkresultset(stmt, procSpec2, array_size(procSpec2),
		      procSpec3, array_size(procSpec3), NULL);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Retrieve information about stored procedures (UNICODE version).
 * @param stmt statement handle
 * @param catalog catalog name/pattern or NULL
 * @param catalogLen length of catalog or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema or SQL_NTS
 * @param proc procedure name/pattern or NULL
 * @param procLen length of proc or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLProceduresW(SQLHSTMT stmt,
	       SQLWCHAR *catalog, SQLSMALLINT catalogLen,
	       SQLWCHAR *schema, SQLSMALLINT schemaLen,
	       SQLWCHAR *proc, SQLSMALLINT procLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = mkresultset(stmt, procSpec2, array_size(procSpec2),
		      procSpec3, array_size(procSpec3), NULL);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Columns for result set of SQLProcedureColumns().
 */

static COL procColSpec2[] = {
    { "SYSTEM", "PROCCOL", "PROCEDURE_QUALIFIER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCCOL", "PROCEDURE_OWNER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCCOL", "PROCEDURE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PROCCOL", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PROCCOL", "COLUMN_TYPE", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "DATA_TYPE", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "TYPE_NAME", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCCOL", "PRECISION", SQL_INTEGER, 10 },
    { "SYSTEM", "PROCCOL", "LENGTH", SQL_INTEGER, 10 },
    { "SYSTEM", "PROCCOL", "SCALE", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "RADIX", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "NULLABLE", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "REMARKS", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCCOL", "COLUMN_DEF", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCCOL", "SQL_DATA_TYPE", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "SQL_DATETIME_SUB", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "CHAR_OCTET_LENGTH", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "ORDINAL_POSITION", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "IS_NULLABLE", SCOL_VARCHAR, 50 }
};

static COL procColSpec3[] = {
    { "SYSTEM", "PROCCOL", "PROCEDURE_CAT", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCCOL", "PROCEDURE_SCHEM", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCCOL", "PROCEDURE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PROCCOL", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "PROCCOL", "COLUMN_TYPE", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "DATA_TYPE", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "TYPE_NAME", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCCOL", "COLUMN_SIZE", SQL_INTEGER, 10 },
    { "SYSTEM", "PROCCOL", "BUFFER_LENGTH", SQL_INTEGER, 10 },
    { "SYSTEM", "PROCCOL", "DECIMAL_DIGITS", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "NUM_PREC_RADIX", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "NULLABLE", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "REMARKS", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCCOL", "COLUMN_DEF", SCOL_VARCHAR, 50 },
    { "SYSTEM", "PROCCOL", "SQL_DATA_TYPE", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "SQL_DATETIME_SUB", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "CHAR_OCTET_LENGTH", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "ORDINAL_POSITION", SQL_SMALLINT, 5 },
    { "SYSTEM", "PROCCOL", "IS_NULLABLE", SCOL_VARCHAR, 50 }
};

#ifndef WINTERFACE
/**
 * Retrieve information about columns in result set of stored procedures.
 * @param stmt statement handle
 * @param catalog catalog name/pattern or NULL
 * @param catalogLen length of catalog or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema or SQL_NTS
 * @param proc procedure name/pattern or NULL
 * @param procLen length of proc or SQL_NTS
 * @param column column name/pattern or NULL
 * @param columnLen length of column or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLProcedureColumns(SQLHSTMT stmt,
		    SQLCHAR *catalog, SQLSMALLINT catalogLen,
		    SQLCHAR *schema, SQLSMALLINT schemaLen,
		    SQLCHAR *proc, SQLSMALLINT procLen,
		    SQLCHAR *column, SQLSMALLINT columnLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = mkresultset(stmt, procColSpec2, array_size(procColSpec2),
		      procColSpec3, array_size(procColSpec3), NULL);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Retrieve information about columns in result
 * set of stored procedures (UNICODE version).
 * @param stmt statement handle
 * @param catalog catalog name/pattern or NULL
 * @param catalogLen length of catalog or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema or SQL_NTS
 * @param proc procedure name/pattern or NULL
 * @param procLen length of proc or SQL_NTS
 * @param column column name/pattern or NULL
 * @param columnLen length of column or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLProcedureColumnsW(SQLHSTMT stmt,
		     SQLWCHAR *catalog, SQLSMALLINT catalogLen,
		     SQLWCHAR *schema, SQLSMALLINT schemaLen,
		     SQLWCHAR *proc, SQLSMALLINT procLen,
		     SQLWCHAR *column, SQLSMALLINT columnLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = mkresultset(stmt, procColSpec2, array_size(procColSpec2),
		      procColSpec3, array_size(procColSpec3), NULL);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Get information of HENV.
 * @param env environment handle
 * @param attr attribute to be retrieved
 * @param val output buffer
 * @param len length of output buffer
 * @param lenp output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetEnvAttr(SQLHENV env, SQLINTEGER attr, SQLPOINTER val,
	      SQLINTEGER len, SQLINTEGER *lenp)
{
    ENV *e;
    SQLRETURN ret = SQL_ERROR;

    if (env == SQL_NULL_HENV) {
	return SQL_INVALID_HANDLE;
    }
    e = (ENV *) env;
    if (!e || e->magic != ENV_MAGIC) {
	return SQL_INVALID_HANDLE;
    }
#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&e->cs);
    e->owner = GetCurrentThreadId();
#endif
    switch (attr) {
    case SQL_ATTR_CONNECTION_POOLING:
	ret = SQL_ERROR;
	break;
    case SQL_ATTR_CP_MATCH:
	ret = SQL_NO_DATA;
	break;
    case SQL_ATTR_OUTPUT_NTS:
	if (val) {
	    *((SQLINTEGER *) val) = SQL_TRUE;
	}
	if (lenp) {
	    *lenp = sizeof (SQLINTEGER);
	}
	ret = SQL_SUCCESS;
	break;
    case SQL_ATTR_ODBC_VERSION:
	if (val) {
	    *((SQLINTEGER *) val) = e->ov3 ? SQL_OV_ODBC3 : SQL_OV_ODBC2;
	}
	if (lenp) {
	    *lenp = sizeof (SQLINTEGER);
	}
	ret = SQL_SUCCESS;
	break;
    }
#if defined(_WIN32) || defined(_WIN64)
    e->owner = 0;
    LeaveCriticalSection(&e->cs);
#endif
    return ret;
}

/**
 * Set information in HENV.
 * @param env environment handle
 * @param attr attribute to be retrieved
 * @param val parameter buffer
 * @param len length of parameter
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetEnvAttr(SQLHENV env, SQLINTEGER attr, SQLPOINTER val, SQLINTEGER len)
{
    ENV *e;
    SQLRETURN ret = SQL_ERROR;

    if (env == SQL_NULL_HENV) {
	return SQL_INVALID_HANDLE;
    }
    e = (ENV *) env;
    if (!e || e->magic != ENV_MAGIC) {
	return SQL_INVALID_HANDLE;
    }
#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&e->cs);
    e->owner = GetCurrentThreadId();
#endif
    switch (attr) {
    case SQL_ATTR_CONNECTION_POOLING:
	ret = SQL_SUCCESS;
	break;
    case SQL_ATTR_CP_MATCH:
	ret = SQL_NO_DATA;
	break;
    case SQL_ATTR_OUTPUT_NTS:
	if (val == (SQLPOINTER) SQL_TRUE) {
	    ret = SQL_SUCCESS;
	}
	break;
    case SQL_ATTR_ODBC_VERSION:
	if (!val) {
	    ret = SQL_ERROR;
	    break;
	}
	if (val == (SQLPOINTER) SQL_OV_ODBC2) {
	    e->ov3 = 0;
	    ret = SQL_SUCCESS;
	}
	if (val == (SQLPOINTER) SQL_OV_ODBC3) {
	    e->ov3 = 1;
	    ret = SQL_SUCCESS;
	}
	break;
    }
#if defined(_WIN32) || defined(_WIN64)
    e->owner = 0;
    LeaveCriticalSection(&e->cs);
#endif
    return ret;
}

/**
 * Internal get error message given handle (HENV, HDBC, or HSTMT).
 * @param htype handle type
 * @param handle HENV, HDBC, or HSTMT
 * @param recno
 * @param sqlstate output buffer for SQL state
 * @param nativeerr output buffer of native error code
 * @param msg output buffer for error message
 * @param buflen length of output buffer
 * @param msglen output length
 * @result ODBC error code
 */

static SQLRETURN
drvgetdiagrec(SQLSMALLINT htype, SQLHANDLE handle, SQLSMALLINT recno,
	      SQLCHAR *sqlstate, SQLINTEGER *nativeerr, SQLCHAR *msg,
	      SQLSMALLINT buflen, SQLSMALLINT *msglen)
{
    DBC *d = NULL;
    STMT *s = NULL;
    int len, naterr;
    char *logmsg, *sqlst;
    SQLRETURN ret = SQL_ERROR;

    if (handle == SQL_NULL_HANDLE) {
	return SQL_INVALID_HANDLE;
    }
    if (sqlstate) {
	sqlstate[0] = '\0';
    }
    if (msg && buflen > 0) {
	msg[0] = '\0';
    }
    if (msglen) {
	*msglen = 0;
    }
    if (nativeerr) {
	*nativeerr = 0;
    }
    switch (htype) {
    case SQL_HANDLE_ENV:
    case SQL_HANDLE_DESC:
	return SQL_NO_DATA;
    case SQL_HANDLE_DBC:
	HDBC_LOCK((SQLHDBC) handle);
	d = (DBC *) handle;
	logmsg = (char *) d->logmsg;
	sqlst = d->sqlstate;
	naterr = d->naterr;
	break;
    case SQL_HANDLE_STMT:
	HSTMT_LOCK((SQLHSTMT) handle);
	s = (STMT *) handle;
	logmsg = (char *) s->logmsg;
	sqlst = s->sqlstate;
	naterr = s->naterr;
	break;
    default:
	return SQL_INVALID_HANDLE;
    }
    if (buflen < 0) {
	ret = SQL_ERROR;
	goto done;
    }
    if (recno > 1) {
	ret = SQL_NO_DATA;
	goto done;
    }
    len = strlen(logmsg);
    if (len == 0) {
	ret = SQL_NO_DATA;
	goto done;
    }
    if (nativeerr) {
	*nativeerr = naterr;
    }
    if (sqlstate) {
	strcpy((char *) sqlstate, sqlst);
    }
    if (msglen) {
	*msglen = len;
    }
    if (len >= buflen) {
	if (msg && buflen > 0) {
	    strncpy((char *) msg, logmsg, buflen);
	    msg[buflen - 1] = '\0';
	    logmsg[0] = '\0';
	}
    } else if (msg) {
	strcpy((char *) msg, logmsg);
	logmsg[0] = '\0';
    }
    ret = SQL_SUCCESS;
done:
    switch (htype) {
    case SQL_HANDLE_DBC:
	HDBC_UNLOCK((SQLHDBC) handle);
	break;
    case SQL_HANDLE_STMT:
	HSTMT_UNLOCK((SQLHSTMT) handle);
	break;
    }
    return ret;
}

#if !defined(WINTERFACE) || (defined(HAVE_UNIXODBC) && (HAVE_UNIXODBC))
/**
 * Get error message given handle (HENV, HDBC, or HSTMT).
 * @param htype handle type
 * @param handle HENV, HDBC, or HSTMT
 * @param recno
 * @param sqlstate output buffer for SQL state
 * @param nativeerr output buffer of native error code
 * @param msg output buffer for error message
 * @param buflen length of output buffer
 * @param msglen output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetDiagRec(SQLSMALLINT htype, SQLHANDLE handle, SQLSMALLINT recno,
	      SQLCHAR *sqlstate, SQLINTEGER *nativeerr, SQLCHAR *msg,
	      SQLSMALLINT buflen, SQLSMALLINT *msglen)
{
    return drvgetdiagrec(htype, handle, recno, sqlstate,
			 nativeerr, msg, buflen, msglen);
}
#endif

#if !defined(HAVE_UNIXODBC) || !(HAVE_UNIXODBC)
#ifdef WINTERFACE
/**
 * Get error message given handle (HENV, HDBC, or HSTMT)
 * (UNICODE version). 
 * @param htype handle type
 * @param handle HENV, HDBC, or HSTMT
 * @param recno
 * @param sqlstate output buffer for SQL state
 * @param nativeerr output buffer of native error code
 * @param msg output buffer for error message
 * @param buflen length of output buffer
 * @param msglen output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetDiagRecW(SQLSMALLINT htype, SQLHANDLE handle, SQLSMALLINT recno,
	      SQLWCHAR *sqlstate, SQLINTEGER *nativeerr, SQLWCHAR *msg,
	      SQLSMALLINT buflen, SQLSMALLINT *msglen)
{
    char state[16];
    SQLSMALLINT len;
    SQLRETURN ret;
    
    ret = drvgetdiagrec(htype, handle, recno, (SQLCHAR *) state,
			nativeerr, (SQLCHAR *) msg, buflen, &len);
    if (ret == SQL_SUCCESS) {
	if (sqlstate) {
	    uc_from_utf_buf((SQLCHAR *) state, -1, sqlstate,
			    6 * sizeof (SQLWCHAR));
	}
	if (msg) {
	    if (len > 0) {
		SQLWCHAR *m = NULL;

		m = uc_from_utf((unsigned char *) msg, len);
		if (m) {
		    if (buflen) {
			buflen /= sizeof (SQLWCHAR);
			uc_strncpy(msg, m, buflen);
			m[len] = 0;
			len = min(buflen, uc_strlen(m));
		    } else {
			len = uc_strlen(m);
		    }
		    uc_free(m);
		} else {
		    len = 0;
		}
	    }
	    if (len <= 0) {
		len = 0;
		if (buflen > 0) {
		    msg[0] = 0;
		}
	    }
	} else {
	    /* estimated length !!! */
	    len *= sizeof (SQLWCHAR);
	}
	if (msglen) {
	    *msglen = len;
	}
    } else if (ret == SQL_NO_DATA) {
	if (sqlstate) {
	    sqlstate[0] = 0;
	}
	if (msg) {
	    if (buflen > 0) {
		msg[0] = 0;
	    }
	}
	if (msglen) {
	    *msglen = 0;
	}
    }
    return ret;
}
#endif
#endif

/**
 * Get error record given handle (HDBC or HSTMT).
 * @param htype handle type
 * @param handle HDBC or HSTMT
 * @param recno diag record number for which info to be retrieved
 * @param id diag id for which info to be retrieved
 * @param info output buffer for error message
 * @param buflen length of output buffer
 * @param stringlen output length
 * @result ODBC error code
 */

static SQLRETURN
drvgetdiagfield(SQLSMALLINT htype, SQLHANDLE handle, SQLSMALLINT recno,
		SQLSMALLINT id, SQLPOINTER info, 
		SQLSMALLINT buflen, SQLSMALLINT *stringlen)
{
    DBC *d = NULL;
    STMT *s = NULL;
    int len, naterr, strbuf = 1;
    char *logmsg, *sqlst, *clrmsg = NULL;
    SQLRETURN ret = SQL_ERROR;

    if (handle == SQL_NULL_HANDLE) {
	return SQL_INVALID_HANDLE;
    }
    if (stringlen) {
	*stringlen = 0;
    }
    switch (htype) {
    case SQL_HANDLE_ENV:
    case SQL_HANDLE_DESC:
	return SQL_NO_DATA;
    case SQL_HANDLE_DBC:
	HDBC_LOCK((SQLHDBC) handle);
	d = (DBC *) handle;
	logmsg = (char *) d->logmsg;
	sqlst = d->sqlstate;
	naterr = d->naterr;
	break;
    case SQL_HANDLE_STMT:
	HSTMT_LOCK((SQLHSTMT) handle);
	s = (STMT *) handle;
	d = (DBC *) s->dbc;
	logmsg = (char *) s->logmsg;
	sqlst = s->sqlstate;
	naterr = s->naterr;
	break;
    default:
	return SQL_INVALID_HANDLE;
    }
    if (buflen < 0) {
	switch (buflen) {
	case SQL_IS_POINTER:
	case SQL_IS_UINTEGER:
	case SQL_IS_INTEGER:
	case SQL_IS_USMALLINT:
	case SQL_IS_SMALLINT:
	    strbuf = 0;
	    break;
	default:
	    ret = SQL_ERROR;
	    goto done;
	}
    }
    if (recno > 1) {
	ret = SQL_NO_DATA;
	goto done;
    }
    switch (id) {
    case SQL_DIAG_CLASS_ORIGIN:
	logmsg = "ISO 9075";
	if (sqlst[0] == 'I' && sqlst[1] == 'M') {
	    logmsg = "ODBC 3.0";
	}
	break;
    case SQL_DIAG_SUBCLASS_ORIGIN:
	logmsg = "ISO 9075";
	if (sqlst[0] == 'I' && sqlst[1] == 'M') {
	    logmsg = "ODBC 3.0";
	} else if (sqlst[0] == 'H' && sqlst[1] == 'Y') {
	    logmsg = "ODBC 3.0";
	} else if (sqlst[0] == '2' || sqlst[0] == '0' || sqlst[0] == '4') {
	    logmsg = "ODBC 3.0";
	}
	break;
    case SQL_DIAG_CONNECTION_NAME:
    case SQL_DIAG_SERVER_NAME:
	logmsg = d->dsn ? d->dsn : "No DSN";
	break;
    case SQL_DIAG_SQLSTATE:
	logmsg = sqlst;
	break;
    case SQL_DIAG_MESSAGE_TEXT:
	if (info) {
	    clrmsg = logmsg;
	}
	break;
    case SQL_DIAG_NUMBER:
	naterr = 1;
	/* fall through */
    case SQL_DIAG_NATIVE:
	len = strlen(logmsg);
	if (len == 0) {
	    ret = SQL_NO_DATA;
	    goto done;
	}
	if (info) {
	    *((SQLINTEGER *) info) = naterr;
	}
	ret = SQL_SUCCESS;
	goto done;
    case SQL_DIAG_DYNAMIC_FUNCTION:
	logmsg = "";
	break;
    case SQL_DIAG_CURSOR_ROW_COUNT:
	if (htype == SQL_HANDLE_STMT) {
	    SQLULEN count;

	    count = (s->isselect == 1 || s->isselect == -1) ? s->nrows : 0;
	    *((SQLULEN *) info) = count;
	    ret = SQL_SUCCESS;
	}
	goto done;
    case SQL_DIAG_ROW_COUNT:
	if (htype == SQL_HANDLE_STMT) {
	    SQLULEN count;

	    count = s->isselect ? 0 : s->nrows;
	    *((SQLULEN *) info) = count;
	    ret = SQL_SUCCESS;
	}
	goto done;
    default:
	ret = SQL_ERROR;
	goto done;
    }
    if (info && buflen > 0) {
	((char *) info)[0] = '\0';
    }
    len = strlen(logmsg);
    if (len == 0) {
	ret = SQL_NO_DATA;
	goto done;
    }
    if (stringlen) {
	*stringlen = len;
    }
    if (strbuf) {
	if (len >= buflen) {
	    if (info && buflen > 0) {
		if (stringlen) {
		    *stringlen = buflen - 1;
		}
		strncpy((char *) info, logmsg, buflen);
		((char *) info)[buflen - 1] = '\0';
	    }
	} else if (info) {
	    strcpy((char *) info, logmsg);
	}
    }
    if (clrmsg) {
	*clrmsg = '\0';
    }
    ret = SQL_SUCCESS;
done:
    switch (htype) {
    case SQL_HANDLE_DBC:
	HDBC_UNLOCK((SQLHDBC) handle);
	break;
    case SQL_HANDLE_STMT:
	HSTMT_UNLOCK((SQLHSTMT) handle);
	break;
    }
    return ret;
}

#ifndef WINTERFACE
/**
 * Get error record given handle (HDBC or HSTMT).
 * @param htype handle type
 * @param handle HDBC or HSTMT
 * @param recno diag record number for which info to be retrieved
 * @param id diag id for which info to be retrieved
 * @param info output buffer for error message
 * @param buflen length of output buffer
 * @param stringlen output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetDiagField(SQLSMALLINT htype, SQLHANDLE handle, SQLSMALLINT recno,
		SQLSMALLINT id, SQLPOINTER info, 
		SQLSMALLINT buflen, SQLSMALLINT *stringlen)
{
    return drvgetdiagfield(htype, handle, recno, id, info, buflen, stringlen);
}
#endif

#ifdef WINTERFACE
/**
 * Get error record given handle (HDBC or HSTMT).
 * @param htype handle type
 * @param handle HDBC or HSTMT
 * @param recno diag record number for which info to be retrieved
 * @param id diag id for which info to be retrieved
 * @param info output buffer for error message
 * @param buflen length of output buffer
 * @param stringlen output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetDiagFieldW(SQLSMALLINT htype, SQLHANDLE handle, SQLSMALLINT recno,
		 SQLSMALLINT id, SQLPOINTER info, 
		 SQLSMALLINT buflen, SQLSMALLINT *stringlen)
{
    SQLSMALLINT len;
    SQLRETURN ret;
    
    ret = drvgetdiagfield(htype, handle, recno, id, info, buflen, &len);
    if (ret == SQL_SUCCESS) {
	if (info) {
	    switch (id) {
	    case SQL_DIAG_CLASS_ORIGIN:
	    case SQL_DIAG_SUBCLASS_ORIGIN:
	    case SQL_DIAG_CONNECTION_NAME:
	    case SQL_DIAG_SERVER_NAME:
	    case SQL_DIAG_SQLSTATE:
	    case SQL_DIAG_MESSAGE_TEXT:
	    case SQL_DIAG_DYNAMIC_FUNCTION:
		if (len > 0) {
		    SQLWCHAR *m = NULL;

		    m = uc_from_utf((unsigned char *) info, len);
		    if (m) {
			if (buflen) {
			    buflen /= sizeof (SQLWCHAR);
			    uc_strncpy(info, m, buflen);
			    m[len] = 0;
			    len = min(buflen, uc_strlen(m));
			} else {
			    len = uc_strlen(m);
			}
			uc_free(m);
			len *= sizeof (SQLWCHAR);
		    } else {
			len = 0;
		    }
		}
		if (len <= 0) {
		    len = 0;
		    if (buflen > 0) {
			((SQLWCHAR *) info)[0] = 0;
		    }
		}
	    }
	} else {
	    switch (id) {
	    case SQL_DIAG_CLASS_ORIGIN:
	    case SQL_DIAG_SUBCLASS_ORIGIN:
	    case SQL_DIAG_CONNECTION_NAME:
	    case SQL_DIAG_SERVER_NAME:
	    case SQL_DIAG_SQLSTATE:
	    case SQL_DIAG_MESSAGE_TEXT:
	    case SQL_DIAG_DYNAMIC_FUNCTION:
		len *= sizeof (SQLWCHAR);
		break;
	    }
	}
	if (stringlen) {
	    *stringlen = len;
	}
    }
    return ret;
}
#endif

/**
 * Internal get option of HSTMT.
 * @param stmt statement handle
 * @param attr attribute to be retrieved
 * @param val output buffer
 * @param bufmax length of output buffer
 * @param buflen output length
 * @result ODBC error code
 */

static SQLRETURN
drvgetstmtattr(SQLHSTMT stmt, SQLINTEGER attr, SQLPOINTER val,
	       SQLINTEGER bufmax, SQLINTEGER *buflen)
{
    STMT *s = (STMT *) stmt;
    DBC *d = (DBC *) s->dbc;
    SQLUINTEGER *uval = (SQLUINTEGER *) val;

    switch (attr) {
    case SQL_QUERY_TIMEOUT:
	*uval = 0;
	return SQL_SUCCESS;
    case SQL_ATTR_CURSOR_TYPE:
	*uval = s->curtype;
	return SQL_SUCCESS;
    case SQL_ATTR_CURSOR_SCROLLABLE:
	*uval = (s->curtype != SQL_CURSOR_FORWARD_ONLY) ?
	    SQL_SCROLLABLE : SQL_NONSCROLLABLE;
	return SQL_SUCCESS;
#ifdef SQL_ATTR_CURSOR_SENSITIVITY
    case SQL_ATTR_CURSOR_SENSITIVITY:
	*uval = SQL_UNSPECIFIED;
	return SQL_SUCCESS;
#endif
    case SQL_ATTR_ROW_NUMBER:
	if (s == d->vm_stmt) {
	    *uval = (d->vm_rownum < 0) ?
		    SQL_ROW_NUMBER_UNKNOWN : (d->vm_rownum + 1);
	} else {
	    *uval = (s->rowp < 0) ? SQL_ROW_NUMBER_UNKNOWN : (s->rowp + 1);
	}
	return SQL_SUCCESS;
    case SQL_ATTR_ASYNC_ENABLE:
	*uval = SQL_ASYNC_ENABLE_OFF;
	return SQL_SUCCESS;
    case SQL_CONCURRENCY:
	*uval = SQL_CONCUR_LOCK;
	return SQL_SUCCESS;
    case SQL_ATTR_RETRIEVE_DATA:
	*uval = s->retr_data;
	return SQL_SUCCESS;
    case SQL_ROWSET_SIZE:
    case SQL_ATTR_ROW_ARRAY_SIZE:
	*uval = s->rowset_size;
	return SQL_SUCCESS;
    /* Needed for some driver managers, but dummies for now */
    case SQL_ATTR_IMP_ROW_DESC:
    case SQL_ATTR_APP_ROW_DESC:
    case SQL_ATTR_IMP_PARAM_DESC:
    case SQL_ATTR_APP_PARAM_DESC:
	*((SQLHDESC *) val) = (SQLHDESC) DEAD_MAGIC;
	return SQL_SUCCESS;
    case SQL_ATTR_ROW_STATUS_PTR:
	*((SQLUSMALLINT **) val) = s->row_status;
	return SQL_SUCCESS;
    case SQL_ATTR_ROWS_FETCHED_PTR:
	*((SQLULEN **) val) = s->row_count;
	return SQL_SUCCESS;
    case SQL_ATTR_USE_BOOKMARKS: {
	STMT *s = (STMT *) stmt;

	*(SQLUINTEGER *) val = s->bkmrk ? SQL_UB_ON : SQL_UB_OFF;
	return SQL_SUCCESS;
    }
    case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
	*((SQLULEN **) val) = s->parm_bind_offs;
	return SQL_SUCCESS;
    case SQL_ATTR_PARAM_BIND_TYPE:
	*((SQLUINTEGER *) val) = SQL_PARAM_BIND_BY_COLUMN;
	return SQL_SUCCESS;
    case SQL_ATTR_PARAM_OPERATION_PTR:
	*((SQLUSMALLINT **) val) = s->parm_oper;
	return SQL_SUCCESS;
    case SQL_ATTR_PARAM_STATUS_PTR:
	*((SQLUSMALLINT **) val) = s->parm_status;
	return SQL_SUCCESS;
    case SQL_ATTR_PARAMS_PROCESSED_PTR:
	*((SQLULEN **) val) = s->parm_proc;
	return SQL_SUCCESS;
    case SQL_ATTR_PARAMSET_SIZE:
	*((SQLUINTEGER *) val) = s->paramset_size;
	return SQL_SUCCESS;
    case SQL_ATTR_ROW_BIND_TYPE:
	*(SQLUINTEGER *) val = s->bind_type;
	return SQL_SUCCESS;
    case SQL_ATTR_ROW_BIND_OFFSET_PTR:
	*((SQLULEN **) val) = s->bind_offs;
	return SQL_SUCCESS;
    case SQL_ATTR_NOSCAN:
	*((SQLUINTEGER **) val) = SQL_NOSCAN_OFF;
	return SQL_SUCCESS;
    case SQL_ATTR_MAX_ROWS:
    case SQL_ATTR_MAX_LENGTH:
	*((SQLINTEGER *) val) = 1000000000;
	return SQL_SUCCESS;
    }
    return drvunimplstmt(stmt);
}

#if (defined(HAVE_UNIXODBC) && (HAVE_UNIXODBC)) || !defined(WINTERFACE)
/**
 * Get option of HSTMT.
 * @param stmt statement handle
 * @param attr attribute to be retrieved
 * @param val output buffer
 * @param bufmax length of output buffer
 * @param buflen output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetStmtAttr(SQLHSTMT stmt, SQLINTEGER attr, SQLPOINTER val,
	       SQLINTEGER bufmax, SQLINTEGER *buflen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvgetstmtattr(stmt, attr, val, bufmax, buflen);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Get option of HSTMT (UNICODE version).
 * @param stmt statement handle
 * @param attr attribute to be retrieved
 * @param val output buffer
 * @param bufmax length of output buffer
 * @param buflen output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetStmtAttrW(SQLHSTMT stmt, SQLINTEGER attr, SQLPOINTER val,
		SQLINTEGER bufmax, SQLINTEGER *buflen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvgetstmtattr(stmt, attr, val, bufmax, buflen);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Internal set option on HSTMT.
 * @param stmt statement handle
 * @param attr attribute to be set
 * @param val input buffer (attribute value)
 * @param buflen length of input buffer
 * @result ODBC error code
 */

static SQLRETURN
drvsetstmtattr(SQLHSTMT stmt, SQLINTEGER attr, SQLPOINTER val,
	       SQLINTEGER buflen)
{
    STMT *s = (STMT *) stmt;
#if defined(SQL_BIGINT) && defined(__WORDSIZE) && (__WORDSIZE == 64)
    SQLBIGINT uval;

    uval = (SQLBIGINT) val;
#else
    SQLULEN uval;

    uval = (SQLULEN) val;
#endif
    switch (attr) {
    case SQL_ATTR_CURSOR_TYPE:
	if (val == (SQLPOINTER) SQL_CURSOR_FORWARD_ONLY) {
	    s->curtype = SQL_CURSOR_FORWARD_ONLY;
	} else {
	    s->curtype = SQL_CURSOR_STATIC;
	}
	if (val != (SQLPOINTER) SQL_CURSOR_FORWARD_ONLY &&
	    val != (SQLPOINTER) SQL_CURSOR_STATIC) {
	    goto e01s02;
	}
	return SQL_SUCCESS;
    case SQL_ATTR_CURSOR_SCROLLABLE:
	if (val == (SQLPOINTER) SQL_NONSCROLLABLE) {
	    s->curtype = SQL_CURSOR_FORWARD_ONLY;
	} else {
	    s->curtype = SQL_CURSOR_STATIC;
	}
	return SQL_SUCCESS;
    case SQL_ATTR_ASYNC_ENABLE:
	if (val != (SQLPOINTER) SQL_ASYNC_ENABLE_OFF) {
    e01s02:
	    setstat(s, -1, "option value changed", "01S02");
	    return SQL_SUCCESS_WITH_INFO;
	}
	return SQL_SUCCESS;
    case SQL_CONCURRENCY:
	if (val != (SQLPOINTER) SQL_CONCUR_LOCK) {
	    goto e01s02;
	}
	return SQL_SUCCESS;
#ifdef SQL_ATTR_CURSOR_SENSITIVITY
    case SQL_ATTR_CURSOR_SENSITIVITY:
	if (val != (SQLPOINTER) SQL_UNSPECIFIED) {
	    goto e01s02;
	}
	return SQL_SUCCESS;
#endif
    case SQL_ATTR_QUERY_TIMEOUT:
	return SQL_SUCCESS;
    case SQL_ATTR_RETRIEVE_DATA:
	if (val != (SQLPOINTER) SQL_RD_ON &&
	    val != (SQLPOINTER) SQL_RD_OFF) {
	    goto e01s02;
	}
	s->retr_data = uval;
	return SQL_SUCCESS;
    case SQL_ROWSET_SIZE:
    case SQL_ATTR_ROW_ARRAY_SIZE:
	if (uval < 1) {
	    setstat(s, -1, "invalid rowset size", "HY000");
	    return SQL_ERROR;
	} else {
	    SQLUSMALLINT *rst = &s->row_status1;

	    if (uval > 1) {
		rst = xmalloc(sizeof (SQLUSMALLINT) * uval);
		if (!rst) {
		    return nomem(s);
		}
	    }
	    if (s->row_status0 != &s->row_status1) {
		freep(&s->row_status0);
	    }
	    s->row_status0 = rst;
	    s->rowset_size = uval;
	}
	return SQL_SUCCESS;
    case SQL_ATTR_ROW_STATUS_PTR:
	s->row_status = (SQLUSMALLINT *) val;
	return SQL_SUCCESS;
    case SQL_ATTR_ROWS_FETCHED_PTR:
	s->row_count = (SQLULEN *) val;
	return SQL_SUCCESS;
    case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
	s->parm_bind_offs = (SQLULEN *) val;
	return SQL_SUCCESS;
    case SQL_ATTR_PARAM_BIND_TYPE:
	s->parm_bind_type = uval;
	return SQL_SUCCESS;
    case SQL_ATTR_PARAM_OPERATION_PTR:
	s->parm_oper = (SQLUSMALLINT *) val;
	return SQL_SUCCESS;
    case SQL_ATTR_PARAM_STATUS_PTR:
	s->parm_status = (SQLUSMALLINT *) val;
	return SQL_SUCCESS;
    case SQL_ATTR_PARAMS_PROCESSED_PTR:
	s->parm_proc = (SQLULEN *) val;
	return SQL_SUCCESS;
    case SQL_ATTR_PARAMSET_SIZE:
	if ((PTRDIFF_T) val < 1) {
	    goto e01s02;
	}
	s->paramset_size = uval;
	s->paramset_count = 0;
	return SQL_SUCCESS;
    case SQL_ATTR_ROW_BIND_TYPE:
	s->bind_type = uval;
	return SQL_SUCCESS;
    case SQL_ATTR_ROW_BIND_OFFSET_PTR:
	s->bind_offs = (SQLULEN *) val;
	return SQL_SUCCESS;
    case SQL_ATTR_USE_BOOKMARKS:
	if (val != (SQLPOINTER) SQL_UB_OFF &&
	    val != (SQLPOINTER) SQL_UB_ON) {
	    goto e01s02;
	}
	s->bkmrk = val == (SQLPOINTER) SQL_UB_ON;
	return SQL_SUCCESS;
    case SQL_ATTR_NOSCAN:
	if (val != (SQLPOINTER) SQL_NOSCAN_OFF) {
	    goto e01s02;
	}
	return SQL_SUCCESS;
    case SQL_ATTR_MAX_ROWS:
    case SQL_ATTR_MAX_LENGTH:
	if (val != (SQLPOINTER) 1000000000) {
	    goto e01s02;
	}
	return SQL_SUCCESS;
    }
    return drvunimplstmt(stmt);
}

#if (defined(HAVE_UNIXODBC) && HAVE_UNIXODBC) || !defined(WINTERFACE)
/**
 * Set option on HSTMT.
 * @param stmt statement handle
 * @param attr attribute to be set
 * @param val input buffer (attribute value)
 * @param buflen length of input buffer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetStmtAttr(SQLHSTMT stmt, SQLINTEGER attr, SQLPOINTER val,
	       SQLINTEGER buflen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvsetstmtattr(stmt, attr, val, buflen);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Set option on HSTMT (UNICODE version).
 * @param stmt statement handle
 * @param attr attribute to be set
 * @param val input buffer (attribute value)
 * @param buflen length of input buffer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetStmtAttrW(SQLHSTMT stmt, SQLINTEGER attr, SQLPOINTER val,
		SQLINTEGER buflen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvsetstmtattr(stmt, attr, val, buflen);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Internal get option of HSTMT.
 * @param stmt statement handle
 * @param opt option to be retrieved
 * @param param output buffer
 * @result ODBC error code
 */

static SQLRETURN
drvgetstmtoption(SQLHSTMT stmt, SQLUSMALLINT opt, SQLPOINTER param)
{
    STMT *s = (STMT *) stmt;
    DBC *d = (DBC *) s->dbc;
    SQLUINTEGER *ret = (SQLUINTEGER *) param;

    switch (opt) {
    case SQL_QUERY_TIMEOUT:
	*ret = 0;
	return SQL_SUCCESS;
    case SQL_CURSOR_TYPE:
	*ret = s->curtype;
	return SQL_SUCCESS;
    case SQL_ROW_NUMBER:
	if (s == d->vm_stmt) {
	    *ret = (d->vm_rownum < 0) ?
		   SQL_ROW_NUMBER_UNKNOWN : (d->vm_rownum + 1);
	} else {
	    *ret = (s->rowp < 0) ? SQL_ROW_NUMBER_UNKNOWN : (s->rowp + 1);
	}
	return SQL_SUCCESS;
    case SQL_ASYNC_ENABLE:
	*ret = SQL_ASYNC_ENABLE_OFF;
	return SQL_SUCCESS;
    case SQL_CONCURRENCY:
	*ret = SQL_CONCUR_LOCK;
	return SQL_SUCCESS;
    case SQL_ATTR_RETRIEVE_DATA:
	*ret = s->retr_data;
	return SQL_SUCCESS;
    case SQL_ROWSET_SIZE:
    case SQL_ATTR_ROW_ARRAY_SIZE:
	*ret = s->rowset_size;
	return SQL_SUCCESS;
    case SQL_ATTR_NOSCAN:
	*ret = SQL_NOSCAN_OFF;
	return SQL_SUCCESS;
    case SQL_ATTR_MAX_ROWS:
    case SQL_ATTR_MAX_LENGTH:
	*ret = 1000000000;
	return SQL_SUCCESS;
    }
    return drvunimplstmt(stmt);
}

/**
 * Get option of HSTMT.
 * @param stmt statement handle
 * @param opt option to be retrieved
 * @param param output buffer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetStmtOption(SQLHSTMT stmt, SQLUSMALLINT opt, SQLPOINTER param)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvgetstmtoption(stmt, opt, param);
    HSTMT_UNLOCK(stmt);
    return ret;
}

#ifdef WINTERFACE
/**
 * Get option of HSTMT (UNICODE version).
 * @param stmt statement handle
 * @param opt option to be retrieved
 * @param param output buffer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetStmtOptionW(SQLHSTMT stmt, SQLUSMALLINT opt, SQLPOINTER param)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvgetstmtoption(stmt, opt, param);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Internal set option on HSTMT.
 * @param stmt statement handle
 * @param opt option to be set
 * @param param input buffer (option value)
 * @result ODBC error code
 */

static SQLRETURN
drvsetstmtoption(SQLHSTMT stmt, SQLUSMALLINT opt, SQLUINTEGER param)
{
    STMT *s = (STMT *) stmt;

    switch (opt) {
    case SQL_CURSOR_TYPE:
	if (param == SQL_CURSOR_FORWARD_ONLY) {
	    s->curtype = param;
	} else {
	    s->curtype = SQL_CURSOR_STATIC;
	}
	if (param != SQL_CURSOR_FORWARD_ONLY &&
	    param != SQL_CURSOR_STATIC) {
	    goto e01s02;
	}
	return SQL_SUCCESS;
    case SQL_ASYNC_ENABLE:
	if (param != SQL_ASYNC_ENABLE_OFF) {
	    goto e01s02;
	}
	return SQL_SUCCESS;
    case SQL_CONCURRENCY:
	if (param != SQL_CONCUR_LOCK) {
	    goto e01s02;
	}
	return SQL_SUCCESS;
    case SQL_QUERY_TIMEOUT:
	return SQL_SUCCESS;
    case SQL_RETRIEVE_DATA:
	if (param != SQL_RD_ON && param != SQL_RD_OFF) {
    e01s02:
	    setstat(s, -1, "option value changed", "01S02");
	    return SQL_SUCCESS_WITH_INFO;
	}
	s->retr_data = (int) param;
	return SQL_SUCCESS;
    case SQL_ROWSET_SIZE:
    case SQL_ATTR_ROW_ARRAY_SIZE:
	if (param < 1) {
	    setstat(s, -1, "invalid rowset size", "HY000");
	    return SQL_ERROR;
	} else {
	    SQLUSMALLINT *rst = &s->row_status1;

	    if (param > 1) {
		rst = xmalloc(sizeof (SQLUSMALLINT) * param);
		if (!rst) {
		    return nomem(s);
		}
	    }
	    if (s->row_status0 != &s->row_status1) {
		freep(&s->row_status0);
	    }
	    s->row_status0 = rst;
	    s->rowset_size = param;
	}
	return SQL_SUCCESS;
    case SQL_ATTR_NOSCAN:
	if (param != SQL_NOSCAN_OFF) {
	    goto e01s02;
	}
	return SQL_SUCCESS;
    case SQL_ATTR_MAX_ROWS:
    case SQL_ATTR_MAX_LENGTH:
	if (param != 1000000000) {
	    goto e01s02;
	}
	return SQL_SUCCESS;
    }
    return drvunimplstmt(stmt);
}

/**
 * Set option on HSTMT.
 * @param stmt statement handle
 * @param opt option to be set
 * @param param input buffer (option value)
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetStmtOption(SQLHSTMT stmt, SQLUSMALLINT opt,
		 SETSTMTOPTION_LAST_ARG_TYPE param)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvsetstmtoption(stmt, opt, (SQLUINTEGER) param);
    HSTMT_UNLOCK(stmt);
    return ret;
}

#ifdef WINTERFACE
/**
 * Set option on HSTMT (UNICODE version).
 * @param stmt statement handle
 * @param opt option to be set
 * @param param input buffer (option value)
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetStmtOptionW(SQLHSTMT stmt, SQLUSMALLINT opt,
		  SETSTMTOPTION_LAST_ARG_TYPE param)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvsetstmtoption(stmt, opt, (SQLUINTEGER) param);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/*
 * Internal set position on result in HSTMT.
 * @param stmt statement handle
 * @param row row to be positioned
 * @param op operation code
 * @param lock locking type
 * @result ODBC error code
 */

static SQLRETURN
drvsetpos(SQLHSTMT stmt, SQLSETPOSIROW row, SQLUSMALLINT op, SQLUSMALLINT lock)
{
    STMT *s = (STMT *) stmt;
    int rowp;

    if (op != SQL_POSITION) {
	return drvunimplstmt(stmt);
    }
    rowp = s->rowp + row - 1;
    if (!s->rows || row <= 0 || rowp < -1 || rowp >= s->nrows) {
	setstat(s, -1, "row out of range", (*s->ov3) ? "HY107" : "S1107");
	return SQL_ERROR;
    }
    s->rowp = rowp;
    return SQL_SUCCESS;
}

/**
 * Set position on result in HSTMT.
 * @param stmt statement handle
 * @param row row to be positioned
 * @param op operation code
 * @param lock locking type
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetPos(SQLHSTMT stmt, SQLSETPOSIROW row, SQLUSMALLINT op, SQLUSMALLINT lock)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvsetpos(stmt, row, op, lock);
    HSTMT_UNLOCK(stmt);
    return ret;
}

/**
 * Function not implemented.
 */

SQLRETURN SQL_API
SQLSetScrollOptions(SQLHSTMT stmt, SQLUSMALLINT concur, SQLLEN rowkeyset,
		    SQLUSMALLINT rowset)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvunimplstmt(stmt);
    HSTMT_UNLOCK(stmt);
    return ret;
}

#define strmak(dst, src, max, lenp) { \
    int len = strlen(src); \
    int cnt = min(len + 1, max); \
    strncpy(dst, src, cnt); \
    *lenp = (cnt > len) ? len : cnt; \
}

/**
 * Internal return information about what this ODBC driver supports.
 * @param dbc database connection handle
 * @param type type of information to be retrieved
 * @param val output buffer
 * @param valMax length of output buffer
 * @param valLen output length
 * @result ODBC error code
 */

static SQLRETURN
drvgetinfo(SQLHDBC dbc, SQLUSMALLINT type, SQLPOINTER val, SQLSMALLINT valMax,
	   SQLSMALLINT *valLen)
{
    DBC *d;
    char dummyc[16];
    SQLSMALLINT dummy;
#if defined(_WIN32) || defined(_WIN64)
    char pathbuf[301], *drvname;
#else
    static char drvname[] =
#ifdef __OS2__
	"SQLLODBC.DLL";
#else
	"sqliteodbc.so";
#endif
#endif

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    d = (DBC *) dbc;
    if (valMax) {
	valMax--;
    }
    if (!valLen) {
	valLen = &dummy;
    }
    if (!val) {
	val = dummyc;
	valMax = sizeof (dummyc) - 1;
    }
    switch (type) {
    case SQL_MAX_USER_NAME_LEN:
	*((SQLSMALLINT *) val) = 16;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_USER_NAME:
	strmak(val, "", valMax, valLen);
	break;
    case SQL_DRIVER_ODBC_VER:
#if 0
	strmak(val, (*d->ov3) ? "03.00" : "02.50", valMax, valLen);
#else
	strmak(val, "03.00", valMax, valLen);
#endif
	break;
    case SQL_ACTIVE_CONNECTIONS:
    case SQL_ACTIVE_STATEMENTS:
	*((SQLSMALLINT *) val) = 0;
	*valLen = sizeof (SQLSMALLINT);
	break;
#ifdef SQL_ASYNC_MODE
    case SQL_ASYNC_MODE:
	*((SQLUINTEGER *) val) = SQL_AM_NONE;
	*valLen = sizeof (SQLUINTEGER);
	break;
#endif
#ifdef SQL_CREATE_TABLE
    case SQL_CREATE_TABLE:
	*((SQLUINTEGER *) val) = SQL_CT_CREATE_TABLE |
				 SQL_CT_COLUMN_DEFAULT |
				 SQL_CT_COLUMN_CONSTRAINT |
				 SQL_CT_CONSTRAINT_NON_DEFERRABLE;
	*valLen = sizeof (SQLUINTEGER);
	break;
#endif
#ifdef SQL_CREATE_VIEW
    case SQL_CREATE_VIEW:
	*((SQLUINTEGER *) val) = SQL_CV_CREATE_VIEW;
	*valLen = sizeof (SQLUINTEGER);
	break;
#endif
#ifdef SQL_DDL_INDEX
    case SQL_DDL_INDEX:
	*((SQLUINTEGER *) val) = SQL_DI_CREATE_INDEX | SQL_DI_DROP_INDEX;
	*valLen = sizeof (SQLUINTEGER);
	break;
#endif
#ifdef SQL_DROP_TABLE
    case SQL_DROP_TABLE:
	*((SQLUINTEGER *) val) = SQL_DT_DROP_TABLE;
	*valLen = sizeof (SQLUINTEGER);
	break;
#endif
#ifdef SQL_DROP_VIEW
    case SQL_DROP_VIEW:
	*((SQLUINTEGER *) val) = SQL_DV_DROP_VIEW;
	*valLen = sizeof (SQLUINTEGER);
	break;
#endif
#ifdef SQL_INDEX_KEYWORDS
    case SQL_INDEX_KEYWORDS:
	*((SQLUINTEGER *) val) = SQL_IK_ALL;
	*valLen = sizeof (SQLUINTEGER);
	break;
#endif
    case SQL_DATA_SOURCE_NAME:
	strmak(val, d->dsn ? d->dsn : "", valMax, valLen);
	break;
    case SQL_DRIVER_NAME:
#if defined(_WIN32) || defined(_WIN64)
	GetModuleFileName(hModule, pathbuf, sizeof (pathbuf));
	drvname = strrchr(pathbuf, '\\');
	if (drvname == NULL) {
	    drvname = strrchr(pathbuf, '/');
	}
	if (drvname == NULL) {
	    drvname = pathbuf;
	} else {
	    drvname++;
	}
#endif
	strmak(val, drvname, valMax, valLen);
	break;
    case SQL_DRIVER_VER:
	strmak(val, DRIVER_VER_INFO, valMax, valLen);
	break;
    case SQL_FETCH_DIRECTION:
	*((SQLUINTEGER *) val) = SQL_FD_FETCH_NEXT | SQL_FD_FETCH_FIRST |
	    SQL_FD_FETCH_LAST | SQL_FD_FETCH_PRIOR | SQL_FD_FETCH_ABSOLUTE;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_ODBC_VER:
	strmak(val, (*d->ov3) ? "03.00" : "02.50", valMax, valLen);
	break;
    case SQL_ODBC_SAG_CLI_CONFORMANCE:
	*((SQLSMALLINT *) val) = SQL_OSCC_NOT_COMPLIANT;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_STANDARD_CLI_CONFORMANCE:
	*((SQLUINTEGER *) val) = SQL_SCC_XOPEN_CLI_VERSION1;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_SERVER_NAME:
    case SQL_DATABASE_NAME:
	strmak(val, d->dbname ? d->dbname : "", valMax, valLen);
	break;
    case SQL_SEARCH_PATTERN_ESCAPE:
	strmak(val, "\\", valMax, valLen);
	break;
    case SQL_ODBC_SQL_CONFORMANCE:
	*((SQLSMALLINT *) val) = SQL_OSC_MINIMUM;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_ODBC_API_CONFORMANCE:
	*((SQLSMALLINT *) val) = SQL_OAC_LEVEL1;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_DBMS_NAME:
	strmak(val, "SQLite", valMax, valLen);
	break;
    case SQL_DBMS_VER:
	strmak(val, SQLITE_VERSION, valMax, valLen);
	break;
    case SQL_COLUMN_ALIAS:
    case SQL_NEED_LONG_DATA_LEN:
	strmak(val, "Y", valMax, valLen);
	break;
    case SQL_ROW_UPDATES:
    case SQL_ACCESSIBLE_PROCEDURES:
    case SQL_PROCEDURES:
    case SQL_EXPRESSIONS_IN_ORDERBY:
    case SQL_ODBC_SQL_OPT_IEF:
    case SQL_LIKE_ESCAPE_CLAUSE:
    case SQL_ORDER_BY_COLUMNS_IN_SELECT:
    case SQL_OUTER_JOINS:
    case SQL_ACCESSIBLE_TABLES:
    case SQL_MULT_RESULT_SETS:
    case SQL_MULTIPLE_ACTIVE_TXN:
    case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
	strmak(val, "N", valMax, valLen);
	break;
#ifdef SQL_CATALOG_NAME
    case SQL_CATALOG_NAME:
#if defined(_WIN32) || defined(_WIN64)
	strmak(val, d->xcelqrx ? "Y": "N", valMax, valLen);
#else
	strmak(val, "N", valMax, valLen);
#endif
	break;
#endif
    case SQL_DATA_SOURCE_READ_ONLY:
	strmak(val, "N", valMax, valLen);
	break;
#ifdef SQL_OJ_CAPABILITIES
    case SQL_OJ_CAPABILITIES:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
	break;
#endif
#ifdef SQL_MAX_IDENTIFIER_LEN
    case SQL_MAX_IDENTIFIER_LEN:
	*((SQLUSMALLINT *) val) = 255;
	*valLen = sizeof (SQLUSMALLINT);
	break;
#endif
    case SQL_CONCAT_NULL_BEHAVIOR:
	*((SQLSMALLINT *) val) = SQL_CB_NULL;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_CURSOR_COMMIT_BEHAVIOR:
    case SQL_CURSOR_ROLLBACK_BEHAVIOR:
	*((SQLSMALLINT *) val) = SQL_CB_PRESERVE;
	*valLen = sizeof (SQLSMALLINT);
	break;
#ifdef SQL_CURSOR_SENSITIVITY
    case SQL_CURSOR_SENSITIVITY:
	*((SQLUINTEGER *) val) = SQL_UNSPECIFIED;
	*valLen = sizeof (SQLUINTEGER);
	break;
#endif
    case SQL_DEFAULT_TXN_ISOLATION:
	*((SQLUINTEGER *) val) = SQL_TXN_SERIALIZABLE;
	*valLen = sizeof (SQLUINTEGER);
	break;
#ifdef SQL_DESCRIBE_PARAMETER
    case SQL_DESCRIBE_PARAMETER:
	strmak(val, "Y", valMax, valLen);
	break;
#endif
    case SQL_TXN_ISOLATION_OPTION:
	*((SQLUINTEGER *) val) = SQL_TXN_SERIALIZABLE;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_IDENTIFIER_CASE:
	*((SQLSMALLINT *) val) = SQL_IC_SENSITIVE;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_IDENTIFIER_QUOTE_CHAR:
	strmak(val, "\"", valMax, valLen);
	break;
    case SQL_MAX_TABLE_NAME_LEN:
    case SQL_MAX_COLUMN_NAME_LEN:
	*((SQLSMALLINT *) val) = 255;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_MAX_CURSOR_NAME_LEN:
	*((SWORD *) val) = 255;
	*valLen = sizeof (SWORD);
	break;
    case SQL_MAX_PROCEDURE_NAME_LEN:
	*((SQLSMALLINT *) val) = 0;
	break;
    case SQL_MAX_QUALIFIER_NAME_LEN:
    case SQL_MAX_OWNER_NAME_LEN:
	*((SQLSMALLINT *) val) = 255;
	break;
    case SQL_OWNER_TERM:
	strmak(val, "", valMax, valLen);
	break;
    case SQL_PROCEDURE_TERM:
	strmak(val, "PROCEDURE", valMax, valLen);
	break;
    case SQL_QUALIFIER_NAME_SEPARATOR:
	strmak(val, ".", valMax, valLen);
	break;
    case SQL_QUALIFIER_TERM:
#if defined(_WIN32) || defined(_WIN64)
	strmak(val, d->xcelqrx ? "CATALOG" : "", valMax, valLen);
#else
	strmak(val, "", valMax, valLen);
#endif
	break;
    case SQL_QUALIFIER_USAGE:
#if defined(_WIN32) || defined(_WIN64)
	*((SQLUINTEGER *) val) = d->xcelqrx ?
	    (SQL_CU_DML_STATEMENTS | SQL_CU_INDEX_DEFINITION |
	     SQL_CU_TABLE_DEFINITION) : 0;
#else
	*((SQLUINTEGER *) val) = 0;
#endif
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_SCROLL_CONCURRENCY:
	*((SQLUINTEGER *) val) = SQL_SCCO_LOCK;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_SCROLL_OPTIONS:
	*((SQLUINTEGER *) val) = SQL_SO_STATIC | SQL_SO_FORWARD_ONLY;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_TABLE_TERM:
	strmak(val, "TABLE", valMax, valLen);
	break;
    case SQL_TXN_CAPABLE:
	*((SQLSMALLINT *) val) = SQL_TC_ALL;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_CONVERT_FUNCTIONS:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
       break;
    case SQL_SYSTEM_FUNCTIONS:
    case SQL_NUMERIC_FUNCTIONS:
    case SQL_STRING_FUNCTIONS:
    case SQL_TIMEDATE_FUNCTIONS:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_CONVERT_BIGINT:
    case SQL_CONVERT_BIT:
    case SQL_CONVERT_CHAR:
    case SQL_CONVERT_DATE:
    case SQL_CONVERT_DECIMAL:
    case SQL_CONVERT_DOUBLE:
    case SQL_CONVERT_FLOAT:
    case SQL_CONVERT_INTEGER:
    case SQL_CONVERT_LONGVARCHAR:
    case SQL_CONVERT_NUMERIC:
    case SQL_CONVERT_REAL:
    case SQL_CONVERT_SMALLINT:
    case SQL_CONVERT_TIME:
    case SQL_CONVERT_TIMESTAMP:
    case SQL_CONVERT_TINYINT:
    case SQL_CONVERT_VARCHAR:
	*((SQLUINTEGER *) val) = 
	    SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL |
	    SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT | SQL_CVT_REAL |
	    SQL_CVT_DOUBLE | SQL_CVT_VARCHAR | SQL_CVT_LONGVARCHAR |
	    SQL_CVT_BIT | SQL_CVT_TINYINT | SQL_CVT_BIGINT |
	    SQL_CVT_DATE | SQL_CVT_TIME | SQL_CVT_TIMESTAMP;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_CONVERT_BINARY:
    case SQL_CONVERT_VARBINARY:
    case SQL_CONVERT_LONGVARBINARY:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_POSITIONED_STATEMENTS:
    case SQL_LOCK_TYPES:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_BOOKMARK_PERSISTENCE:
	*((SQLUINTEGER *) val) = SQL_BP_SCROLL;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_UNION:
	*((SQLUINTEGER *) val) = SQL_U_UNION | SQL_U_UNION_ALL;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_OWNER_USAGE:
    case SQL_SUBQUERIES:
    case SQL_TIMEDATE_ADD_INTERVALS:
    case SQL_TIMEDATE_DIFF_INTERVALS:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_QUOTED_IDENTIFIER_CASE:
	*((SQLUSMALLINT *) val) = SQL_IC_SENSITIVE;
	*valLen = sizeof (SQLUSMALLINT);
	break;
    case SQL_POS_OPERATIONS:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_ALTER_TABLE:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_CORRELATION_NAME:
	*((SQLSMALLINT *) val) = SQL_CN_DIFFERENT;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_NON_NULLABLE_COLUMNS:
	*((SQLSMALLINT *) val) = SQL_NNC_NON_NULL;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_NULL_COLLATION:
	*((SQLSMALLINT *) val) = SQL_NC_START;
	*valLen = sizeof(SQLSMALLINT);
	break;
    case SQL_MAX_COLUMNS_IN_GROUP_BY:
    case SQL_MAX_COLUMNS_IN_ORDER_BY:
    case SQL_MAX_COLUMNS_IN_SELECT:
    case SQL_MAX_COLUMNS_IN_TABLE:
    case SQL_MAX_ROW_SIZE:
    case SQL_MAX_TABLES_IN_SELECT:
	*((SQLSMALLINT *) val) = 0;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_MAX_BINARY_LITERAL_LEN:
    case SQL_MAX_CHAR_LITERAL_LEN:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_MAX_COLUMNS_IN_INDEX:
	*((SQLSMALLINT *) val) = 0;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_MAX_INDEX_SIZE:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof(SQLUINTEGER);
	break;
#ifdef SQL_MAX_IDENTIFIER_LENGTH
    case SQL_MAX_IDENTIFIER_LENGTH:
	*((SQLUINTEGER *) val) = 255;
	*valLen = sizeof (SQLUINTEGER);
	break;
#endif
    case SQL_MAX_STATEMENT_LEN:
	*((SQLUINTEGER *) val) = 16384;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_QUALIFIER_LOCATION:
	*((SQLSMALLINT *) val) = SQL_QL_START;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_GETDATA_EXTENSIONS:
	*((SQLUINTEGER *) val) =
	    SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER | SQL_GD_BOUND;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_STATIC_SENSITIVITY:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_FILE_USAGE:
#if defined(_WIN32) || defined(_WIN64)
	*((SQLSMALLINT *) val) =
	    d->xcelqrx ? SQL_FILE_CATALOG : SQL_FILE_NOT_SUPPORTED;
#else
	*((SQLSMALLINT *) val) = SQL_FILE_NOT_SUPPORTED;
#endif
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_GROUP_BY:
	*((SQLSMALLINT *) val) = 0;
	*valLen = sizeof (SQLSMALLINT);
	break;
    case SQL_KEYWORDS:
	strmak(val, "CREATE,SELECT,DROP,DELETE,UPDATE,INSERT,"
	       "INTO,VALUES,TABLE,INDEX,FROM,SET,WHERE,AND,CURRENT,OF",
	       valMax, valLen);
	break;
    case SQL_SPECIAL_CHARACTERS:
#ifdef SQL_COLLATION_SEQ
    case SQL_COLLATION_SEQ:
#endif
	strmak(val, "", valMax, valLen);
	break;
    case SQL_BATCH_SUPPORT:
    case SQL_BATCH_ROW_COUNT:
    case SQL_PARAM_ARRAY_ROW_COUNTS:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
	*((SQLUINTEGER *) val) = SQL_CA1_NEXT | SQL_CA1_BOOKMARK;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_STATIC_CURSOR_ATTRIBUTES1:
	*((SQLUINTEGER *) val) = SQL_CA1_NEXT | SQL_CA1_ABSOLUTE |
	    SQL_CA1_RELATIVE | SQL_CA1_BOOKMARK;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
    case SQL_STATIC_CURSOR_ATTRIBUTES2:
	*((SQLUINTEGER *) val) = SQL_CA2_READ_ONLY_CONCURRENCY |
	    SQL_CA2_LOCK_CONCURRENCY;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_KEYSET_CURSOR_ATTRIBUTES1:
    case SQL_KEYSET_CURSOR_ATTRIBUTES2:
    case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
    case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
	*((SQLUINTEGER *) val) = 0;
	*valLen = sizeof (SQLUINTEGER);
	break;
    case SQL_ODBC_INTERFACE_CONFORMANCE:
	*((SQLUINTEGER *) val) = SQL_OIC_CORE;
	*valLen = sizeof (SQLUINTEGER);
	break;
    default:
	setstatd(d, -1, "unsupported info option %d",
		 (*d->ov3) ? "HYC00" : "S1C00", type);
	return SQL_ERROR;
    }
    return SQL_SUCCESS;
}

#if (defined(HAVE_UNIXODBC) && (HAVE_UNIXODBC)) || !defined(WINTERFACE)
/**
 * Return information about what this ODBC driver supports.
 * @param dbc database connection handle
 * @param type type of information to be retrieved
 * @param val output buffer
 * @param valMax length of output buffer
 * @param valLen output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetInfo(SQLHDBC dbc, SQLUSMALLINT type, SQLPOINTER val, SQLSMALLINT valMax,
	   SQLSMALLINT *valLen)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvgetinfo(dbc, type, val, valMax, valLen);
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Return information about what this ODBC driver supports.
 * @param dbc database connection handle
 * @param type type of information to be retrieved
 * @param val output buffer
 * @param valMax length of output buffer
 * @param valLen output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetInfoW(SQLHDBC dbc, SQLUSMALLINT type, SQLPOINTER val, SQLSMALLINT valMax,
	    SQLSMALLINT *valLen)
{
    SQLRETURN ret;
    SQLSMALLINT len = 0;

    HDBC_LOCK(dbc);
    ret = drvgetinfo(dbc, type, val, valMax, &len);
    HDBC_UNLOCK(dbc);
    if (ret == SQL_SUCCESS) {
	SQLWCHAR *v = NULL;

	switch (type) {
	case SQL_USER_NAME:
	case SQL_DRIVER_ODBC_VER:
	case SQL_DATA_SOURCE_NAME:
	case SQL_DRIVER_NAME:
	case SQL_DRIVER_VER:
	case SQL_ODBC_VER:
	case SQL_SERVER_NAME:
	case SQL_DATABASE_NAME:
	case SQL_SEARCH_PATTERN_ESCAPE:
	case SQL_DBMS_NAME:
	case SQL_DBMS_VER:
	case SQL_NEED_LONG_DATA_LEN:
	case SQL_ROW_UPDATES:
	case SQL_ACCESSIBLE_PROCEDURES:
	case SQL_PROCEDURES:
	case SQL_EXPRESSIONS_IN_ORDERBY:
	case SQL_ODBC_SQL_OPT_IEF:
	case SQL_LIKE_ESCAPE_CLAUSE:
	case SQL_ORDER_BY_COLUMNS_IN_SELECT:
	case SQL_OUTER_JOINS:
	case SQL_COLUMN_ALIAS:
	case SQL_ACCESSIBLE_TABLES:
	case SQL_MULT_RESULT_SETS:
	case SQL_MULTIPLE_ACTIVE_TXN:
	case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
	case SQL_DATA_SOURCE_READ_ONLY:
#ifdef SQL_DESCRIBE_PARAMETER
	case SQL_DESCRIBE_PARAMETER:
#endif
	case SQL_IDENTIFIER_QUOTE_CHAR:
	case SQL_OWNER_TERM:
	case SQL_PROCEDURE_TERM:
	case SQL_QUALIFIER_NAME_SEPARATOR:
	case SQL_QUALIFIER_TERM:
	case SQL_TABLE_TERM:
	case SQL_KEYWORDS:
	case SQL_SPECIAL_CHARACTERS:
#ifdef SQL_CATALOG_NAME
	case SQL_CATALOG_NAME:
#endif
#ifdef SQL_COLLATION_SEQ
	case SQL_COLLATION_SEQ:
#endif
	    if (val) {
		if (len > 0) {
		    v = uc_from_utf((SQLCHAR *) val, len);
		    if (v) {
			int vmax = valMax / sizeof (SQLWCHAR);

			uc_strncpy(val, v, vmax);
			v[len] = 0;
			len = min(vmax, uc_strlen(v));
			uc_free(v);
			len *= sizeof (SQLWCHAR);
		    } else {
			len = 0;
		    }
		}
		if (len <= 0) {
		    len = 0;
		    if (valMax >= sizeof (SQLWCHAR)) {
			*((SQLWCHAR *)val) = 0;
		    }
		}
	    } else {
		len *= sizeof (SQLWCHAR);
	    }
	    break;
	}
	if (valLen) {
	    *valLen = len;
	}
    }
    return ret;
}
#endif

/**
 * Return information about supported ODBC API functions.
 * @param dbc database connection handle
 * @param func function code to be retrieved
 * @param flags output indicator
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetFunctions(SQLHDBC dbc, SQLUSMALLINT func,
		SQLUSMALLINT *flags)
{
    int i;
    SQLUSMALLINT exists[100];

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    for (i = 0; i < array_size(exists); i++) {
	exists[i] = SQL_FALSE;
    }
    exists[SQL_API_SQLALLOCCONNECT] = SQL_TRUE;
    exists[SQL_API_SQLFETCH] = SQL_TRUE;
    exists[SQL_API_SQLALLOCENV] = SQL_TRUE;
    exists[SQL_API_SQLFREECONNECT] = SQL_TRUE;
    exists[SQL_API_SQLALLOCSTMT] = SQL_TRUE;
    exists[SQL_API_SQLFREEENV] = SQL_TRUE;
    exists[SQL_API_SQLBINDCOL] = SQL_TRUE;
    exists[SQL_API_SQLFREESTMT] = SQL_TRUE;
    exists[SQL_API_SQLCANCEL] = SQL_TRUE;
    exists[SQL_API_SQLGETCURSORNAME] = SQL_TRUE;
    exists[SQL_API_SQLCOLATTRIBUTES] = SQL_TRUE;
    exists[SQL_API_SQLNUMRESULTCOLS] = SQL_TRUE;
    exists[SQL_API_SQLCONNECT] = SQL_TRUE;
    exists[SQL_API_SQLPREPARE] = SQL_TRUE;
    exists[SQL_API_SQLDESCRIBECOL] = SQL_TRUE;
    exists[SQL_API_SQLROWCOUNT] = SQL_TRUE;
    exists[SQL_API_SQLDISCONNECT] = SQL_TRUE;
    exists[SQL_API_SQLSETCURSORNAME] = SQL_TRUE;
    exists[SQL_API_SQLERROR] = SQL_TRUE;
    exists[SQL_API_SQLSETPARAM] = SQL_TRUE;
    exists[SQL_API_SQLEXECDIRECT] = SQL_TRUE;
    exists[SQL_API_SQLTRANSACT] = SQL_TRUE;
    exists[SQL_API_SQLEXECUTE] = SQL_TRUE;
    exists[SQL_API_SQLBINDPARAMETER] = SQL_TRUE;
    exists[SQL_API_SQLGETTYPEINFO] = SQL_TRUE;
    exists[SQL_API_SQLCOLUMNS] = SQL_TRUE;
    exists[SQL_API_SQLPARAMDATA] = SQL_TRUE;
    exists[SQL_API_SQLDRIVERCONNECT] = SQL_TRUE;
    exists[SQL_API_SQLPUTDATA] = SQL_TRUE;
    exists[SQL_API_SQLGETCONNECTOPTION] = SQL_TRUE;
    exists[SQL_API_SQLSETCONNECTOPTION] = SQL_TRUE;
    exists[SQL_API_SQLGETDATA] = SQL_TRUE;
    exists[SQL_API_SQLSETSTMTOPTION] = SQL_TRUE;
    exists[SQL_API_SQLGETFUNCTIONS] = SQL_TRUE;
    exists[SQL_API_SQLSPECIALCOLUMNS] = SQL_TRUE;
    exists[SQL_API_SQLGETINFO] = SQL_TRUE;
    exists[SQL_API_SQLSTATISTICS] = SQL_TRUE;
    exists[SQL_API_SQLGETSTMTOPTION] = SQL_TRUE;
    exists[SQL_API_SQLTABLES] = SQL_TRUE;
    exists[SQL_API_SQLBROWSECONNECT] = SQL_FALSE;
    exists[SQL_API_SQLNUMPARAMS] = SQL_TRUE;
    exists[SQL_API_SQLCOLUMNPRIVILEGES] = SQL_FALSE;
    exists[SQL_API_SQLPARAMOPTIONS] = SQL_FALSE;
    exists[SQL_API_SQLDATASOURCES] = SQL_TRUE;
    exists[SQL_API_SQLPRIMARYKEYS] = SQL_TRUE;
    exists[SQL_API_SQLDESCRIBEPARAM] = SQL_TRUE;
    exists[SQL_API_SQLPROCEDURECOLUMNS] = SQL_TRUE;
    exists[SQL_API_SQLDRIVERS] = SQL_FALSE;
    exists[SQL_API_SQLPROCEDURES] = SQL_TRUE;
    exists[SQL_API_SQLEXTENDEDFETCH] = SQL_TRUE;
    exists[SQL_API_SQLSETPOS] = SQL_TRUE;
    exists[SQL_API_SQLFOREIGNKEYS] = SQL_TRUE;
    exists[SQL_API_SQLSETSCROLLOPTIONS] = SQL_TRUE;
    exists[SQL_API_SQLMORERESULTS] = SQL_TRUE;
    exists[SQL_API_SQLTABLEPRIVILEGES] = SQL_TRUE;
    exists[SQL_API_SQLNATIVESQL] = SQL_TRUE;
    if (func == SQL_API_ALL_FUNCTIONS) {
	memcpy(flags, exists, sizeof (exists));
    } else if (func == SQL_API_ODBC3_ALL_FUNCTIONS) {
	int i;
#define SET_EXISTS(x) \
	flags[(x) >> 4] |= (1 << ((x) & 0xF))
#define CLR_EXISTS(x) \
	flags[(x) >> 4] &= ~(1 << ((x) & 0xF))

	memset(flags, 0,
	       sizeof (SQLUSMALLINT) * SQL_API_ODBC3_ALL_FUNCTIONS_SIZE);
	for (i = 0; i < array_size(exists); i++) {
	    if (exists[i]) {
		flags[i >> 4] |= (1 << (i & 0xF));
	    }
	}
	SET_EXISTS(SQL_API_SQLALLOCHANDLE);
	SET_EXISTS(SQL_API_SQLFREEHANDLE);
	SET_EXISTS(SQL_API_SQLGETSTMTATTR);
	SET_EXISTS(SQL_API_SQLSETSTMTATTR);
	SET_EXISTS(SQL_API_SQLGETCONNECTATTR);
	SET_EXISTS(SQL_API_SQLSETCONNECTATTR);
	SET_EXISTS(SQL_API_SQLGETENVATTR);
	SET_EXISTS(SQL_API_SQLSETENVATTR);
	SET_EXISTS(SQL_API_SQLCLOSECURSOR);
	SET_EXISTS(SQL_API_SQLBINDPARAM);
#if !defined(HAVE_UNIXODBC) || !(HAVE_UNIXODBC)
	/*
	 * Some unixODBC versions have problems with
	 * SQLError() vs. SQLGetDiagRec() with loss
	 * of error/warning messages.
	 */
	SET_EXISTS(SQL_API_SQLGETDIAGREC);
#endif
	SET_EXISTS(SQL_API_SQLFETCHSCROLL);
	SET_EXISTS(SQL_API_SQLENDTRAN);
    } else {
	if (func < array_size(exists)) {
	    *flags = exists[func];
	} else {
	    switch (func) {
	    case SQL_API_SQLALLOCHANDLE:
	    case SQL_API_SQLFREEHANDLE:
	    case SQL_API_SQLGETSTMTATTR:
	    case SQL_API_SQLSETSTMTATTR:
	    case SQL_API_SQLGETCONNECTATTR:
	    case SQL_API_SQLSETCONNECTATTR:
	    case SQL_API_SQLGETENVATTR:
	    case SQL_API_SQLSETENVATTR:
	    case SQL_API_SQLCLOSECURSOR:
	    case SQL_API_SQLBINDPARAM:
#if !defined(HAVE_UNIXODBC) || !(HAVE_UNIXODBC)
	    /*
	     * Some unixODBC versions have problems with
	     * SQLError() vs. SQLGetDiagRec() with loss
	     * of error/warning messages.
	     */
	    case SQL_API_SQLGETDIAGREC:
#endif
	    case SQL_API_SQLFETCHSCROLL:
	    case SQL_API_SQLENDTRAN:
		*flags = SQL_TRUE;
		break;
	    default:
		*flags = SQL_FALSE;
	    }
	}
    }
    return SQL_SUCCESS;
}

/**
 * Internal allocate HENV.
 * @param env pointer to environment handle
 * @result ODBC error code
 */

static SQLRETURN
drvallocenv(SQLHENV *env)
{
    ENV *e;

    if (env == NULL) {
	return SQL_INVALID_HANDLE;
    }
    e = (ENV *) xmalloc(sizeof (ENV));
    if (e == NULL) {
	*env = SQL_NULL_HENV;
	return SQL_ERROR;
    }
    e->magic = ENV_MAGIC;
    e->ov3 = 0;
#if defined(_WIN32) || defined(_WIN64)
    InitializeCriticalSection(&e->cs);
    e->owner = 0;
#endif
    e->dbcs = NULL;
    *env = (SQLHENV) e;
    return SQL_SUCCESS;
}

/**
 * Allocate HENV.
 * @param env pointer to environment handle
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLAllocEnv(SQLHENV *env)
{
    return drvallocenv(env);
}

/**
 * Internal free HENV.
 * @param env environment handle
 * @result ODBC error code
 */

static SQLRETURN
drvfreeenv(SQLHENV env)
{
    ENV *e;

    if (env == SQL_NULL_HENV) {
	return SQL_INVALID_HANDLE;
    }
    e = (ENV *) env;
    if (e->magic != ENV_MAGIC) {
	return SQL_SUCCESS;
    }
#if defined(_WIN32) || defined(_WIN64)
    EnterCriticalSection(&e->cs);
    e->owner = GetCurrentThreadId();
#endif
    if (e->dbcs) {
#if defined(_WIN32) || defined(_WIN64)
	e->owner = 0;
	LeaveCriticalSection(&e->cs);
#endif
	return SQL_ERROR;
    }
    e->magic = DEAD_MAGIC;
#if defined(_WIN32) || defined(_WIN64)
    e->owner = 0;
    LeaveCriticalSection(&e->cs);
    DeleteCriticalSection(&e->cs);
#endif
    xfree(e);
    return SQL_SUCCESS;
}

/**
 * Free HENV.
 * @param env environment handle
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLFreeEnv(SQLHENV env)
{
    return drvfreeenv(env);
}

/**
 * Internal allocate HDBC.
 * @param env environment handle
 * @param dbc pointer to database connection handle
 * @result ODBC error code
 */

static SQLRETURN
drvallocconnect(SQLHENV env, SQLHDBC *dbc)
{
    DBC *d;
    ENV *e;
    const char *verstr;
    int maj = 0, min = 0, lev = 0;

    if (dbc == NULL) {
	return SQL_ERROR;
    }
    d = (DBC *) xmalloc(sizeof (DBC));
    if (d == NULL) {
	*dbc = SQL_NULL_HDBC;
	return SQL_ERROR;
    }
    memset(d, 0, sizeof (DBC));
    d->curtype = SQL_CURSOR_STATIC;
#if (HAVE_LIBVERSION)
    verstr = sqlite_libversion();
#else
    verstr = sqlite_version;
#endif
    sscanf(verstr, "%d.%d.%d", &maj, &min, &lev);
    d->version = verinfo(maj & 0xFF, min & 0xFF, lev & 0xFF);
    if (d->version < verinfo(2, 8, 0)) {
	xfree(d);
	return SQL_ERROR;
    }
    d->ov3 = &d->ov3val;
    e = (ENV *) env;
#if defined(_WIN32) || defined(_WIN64)
    if (e->magic == ENV_MAGIC) {
	EnterCriticalSection(&e->cs);
	e->owner = GetCurrentThreadId();
    }
#endif
    if (e->magic == ENV_MAGIC) {
	DBC *n, *p;

	d->env = e;
	d->ov3 = &e->ov3;
	p = NULL;
	n = e->dbcs;
	while (n) {
	    p = n;
	    n = n->next;
	}
	if (p) {
	    p->next = d;
	} else {
	    e->dbcs = d;
	}
    }
#if defined(_WIN32) || defined(_WIN64)
    if (e->magic == ENV_MAGIC) {
	e->owner = 0;
	LeaveCriticalSection(&e->cs);
    }
#endif
    d->autocommit = 1;
    d->magic = DBC_MAGIC;
    *dbc = (SQLHDBC) d;
    drvgetgpps(d);
    return SQL_SUCCESS;
}

/**
 * Allocate HDBC.
 * @param env environment handle
 * @param dbc pointer to database connection handle
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLAllocConnect(SQLHENV env, SQLHDBC *dbc)
{
    return drvallocconnect(env, dbc);
}

/**
 * Internal free connection (HDBC).
 * @param dbc database connection handle
 * @result ODBC error code
 */

static SQLRETURN
drvfreeconnect(SQLHDBC dbc)
{
    DBC *d;
    ENV *e;
    SQLRETURN ret = SQL_ERROR;

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    d = (DBC *) dbc;
    if (d->magic != DBC_MAGIC) {
	return SQL_INVALID_HANDLE;
    }
    e = d->env;
    if (e && e->magic == ENV_MAGIC) {
#if defined(_WIN32) || defined(_WIN64)
	EnterCriticalSection(&e->cs);
	e->owner = GetCurrentThreadId();
#endif
    } else {
	e = NULL;
    }
    if (d->sqlite) {
	setstatd(d, -1, "not disconnected", (*d->ov3) ? "HY000" : "S1000");
	goto done;
    }
    while (d->stmt) {
	freestmt((HSTMT) d->stmt);
    }
    if (e && e->magic == ENV_MAGIC) {
	DBC *n, *p;

	p = NULL;
	n = e->dbcs;
	while (n) {
	    if (n == d) {
		break;
	    }
	    p = n;
	    n = n->next;
	}
	if (n) {
	    if (p) {
		p->next = d->next;
	    } else {
		e->dbcs = d->next;
	    }
	}
    }
    drvrelgpps(d);
    d->magic = DEAD_MAGIC;
#if defined(HAVE_SQLITETRACE) && (HAVE_SQLITETRACE)
    if (d->trace) {
	fclose(d->trace);
    }
#endif
    xfree(d);
    ret = SQL_SUCCESS;
done:
#if defined(_WIN32) || defined(_WIN64)
    if (e) {
	e->owner = 0;
	LeaveCriticalSection(&e->cs);
    }
#endif
    return ret;
}

/**
 * Free connection (HDBC).
 * @param dbc database connection handle
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLFreeConnect(SQLHDBC dbc)
{
    return drvfreeconnect(dbc);
}

/**
 * Internal get connect attribute of HDBC.
 * @param dbc database connection handle
 * @param attr option to be retrieved
 * @param val output buffer
 * @param bufmax size of output buffer
 * @param buflen output length
 * @result ODBC error code
 */

static SQLRETURN
drvgetconnectattr(SQLHDBC dbc, SQLINTEGER attr, SQLPOINTER val,
		  SQLINTEGER bufmax, SQLINTEGER *buflen)
{
    DBC *d;
    SQLINTEGER dummy;

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    d = (DBC *) dbc;
    if (!val) {
	val = (SQLPOINTER) &dummy;
    }
    if (!buflen) {
	buflen = &dummy;
    }
    switch (attr) {
    case SQL_ATTR_CONNECTION_DEAD:
	*((SQLINTEGER *) val) = d->sqlite ? SQL_CD_FALSE : SQL_CD_TRUE;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_ACCESS_MODE:
	*((SQLINTEGER *) val) = SQL_MODE_READ_WRITE;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_AUTOCOMMIT:
	*((SQLINTEGER *) val) =
	    d->autocommit ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_LOGIN_TIMEOUT:
	*((SQLINTEGER *) val) = 100;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_ODBC_CURSORS:
	*((SQLINTEGER *) val) = SQL_CUR_USE_DRIVER;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_PACKET_SIZE:
	*((SQLINTEGER *) val) = 16384;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_TXN_ISOLATION:
	*((SQLINTEGER *) val) = SQL_TXN_SERIALIZABLE;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_TRACEFILE:
    case SQL_ATTR_CURRENT_CATALOG:
    case SQL_ATTR_TRANSLATE_LIB:
	*((SQLCHAR *) val) = 0;
	*buflen = 0;
	break;
    case SQL_ATTR_TRACE:
    case SQL_ATTR_QUIET_MODE:
    case SQL_ATTR_TRANSLATE_OPTION:
    case SQL_ATTR_KEYSET_SIZE:
    case SQL_ATTR_QUERY_TIMEOUT:
	*((SQLINTEGER *) val) = 0;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_PARAM_BIND_TYPE:
	*((SQLUINTEGER *) val) = SQL_PARAM_BIND_BY_COLUMN;
	*buflen = sizeof (SQLUINTEGER);
	break;
    case SQL_ATTR_ROW_BIND_TYPE:
	*((SQLUINTEGER *) val) = SQL_BIND_BY_COLUMN;
	*buflen = sizeof (SQLUINTEGER);
	break;
    case SQL_ATTR_USE_BOOKMARKS:
	*((SQLINTEGER *) val) = SQL_UB_OFF;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_ASYNC_ENABLE:
	*((SQLINTEGER *) val) = SQL_ASYNC_ENABLE_OFF;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_NOSCAN:
	*((SQLINTEGER *) val) = SQL_NOSCAN_OFF;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_CONCURRENCY:
	*((SQLINTEGER *) val) = SQL_CONCUR_LOCK;
	*buflen = sizeof (SQLINTEGER);
	break;
#ifdef SQL_ATTR_CURSOR_SENSITIVITY
    case SQL_ATTR_CURSOR_SENSITIVITY:
	*((SQLINTEGER *) val) = SQL_UNSPECIFIED;
	*buflen = sizeof (SQLINTEGER);
	break;
#endif
    case SQL_ATTR_SIMULATE_CURSOR:
	*((SQLINTEGER *) val) = SQL_SC_NON_UNIQUE;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_MAX_ROWS:
    case SQL_ATTR_MAX_LENGTH:
	*((SQLINTEGER *) val) = 1000000000;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_CURSOR_TYPE:
	*((SQLINTEGER *) val) = d->curtype;
	*buflen = sizeof (SQLINTEGER);
	break;
    case SQL_ATTR_RETRIEVE_DATA:
	*((SQLINTEGER *) val) = SQL_RD_ON;
	*buflen = sizeof (SQLINTEGER);
	break;
    default:
	setstatd(d, -1, "unsupported connect attribute %d",
		 (*d->ov3) ? "HYC00" : "S1C00", (int) attr);
	return SQL_ERROR;
    }
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Get connect attribute of HDBC.
 * @param dbc database connection handle
 * @param attr option to be retrieved
 * @param val output buffer
 * @param bufmax size of output buffer
 * @param buflen output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetConnectAttr(SQLHDBC dbc, SQLINTEGER attr, SQLPOINTER val,
		  SQLINTEGER bufmax, SQLINTEGER *buflen)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvgetconnectattr(dbc, attr, val, bufmax, buflen);
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Get connect attribute of HDBC (UNICODE version).
 * @param dbc database connection handle
 * @param attr option to be retrieved
 * @param val output buffer
 * @param bufmax size of output buffer
 * @param buflen output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetConnectAttrW(SQLHDBC dbc, SQLINTEGER attr, SQLPOINTER val,
		   SQLINTEGER bufmax, SQLINTEGER *buflen)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvgetconnectattr(dbc, attr, val, bufmax, buflen);
    if (SQL_SUCCEEDED(ret)) {
	switch (attr) {
	case SQL_ATTR_TRACEFILE:
	case SQL_ATTR_CURRENT_CATALOG:
	case SQL_ATTR_TRANSLATE_LIB:
	    if (val && bufmax >= sizeof (SQLWCHAR)) {
		*(SQLWCHAR *) val = 0;
	    }
	    break;
	}
    }
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

/**
 * Internal set connect attribute of HDBC.
 * @param dbc database connection handle
 * @param attr option to be set
 * @param val option value
 * @param len size of option
 * @result ODBC error code
 */

static SQLRETURN
drvsetconnectattr(SQLHDBC dbc, SQLINTEGER attr, SQLPOINTER val,
		  SQLINTEGER len)
{
    DBC *d;

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    d = (DBC *) dbc;
    switch (attr) {
    case SQL_AUTOCOMMIT:
	d->autocommit = val == (SQLPOINTER) SQL_AUTOCOMMIT_ON;
	if (d->autocommit && d->intrans) {
	    return endtran(d, SQL_COMMIT, 1);
	} else if (!d->autocommit) {
	    vm_end(d->vm_stmt);
	}
	return SQL_SUCCESS;
    default:
	setstatd(d, -1, "option value changed", "01S02");
	return SQL_SUCCESS_WITH_INFO;
    }
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Set connect attribute of HDBC.
 * @param dbc database connection handle
 * @param attr option to be set
 * @param val option value
 * @param len size of option
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetConnectAttr(SQLHDBC dbc, SQLINTEGER attr, SQLPOINTER val,
		  SQLINTEGER len)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvsetconnectattr(dbc, attr, val, len);
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Set connect attribute of HDBC (UNICODE version).
 * @param dbc database connection handle
 * @param attr option to be set
 * @param val option value
 * @param len size of option
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetConnectAttrW(SQLHDBC dbc, SQLINTEGER attr, SQLPOINTER val,
		   SQLINTEGER len)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvsetconnectattr(dbc, attr, val, len);
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

/**
 * Internal get connect option of HDBC.
 * @param dbc database connection handle
 * @param opt option to be retrieved
 * @param param output buffer
 * @result ODBC error code
 */

static SQLRETURN
drvgetconnectoption(SQLHDBC dbc, SQLUSMALLINT opt, SQLPOINTER param)
{
    DBC *d;
    SQLINTEGER dummy;

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    d = (DBC *) dbc;
    if (!param) {
	param = (SQLPOINTER) &dummy;
    }
    switch (opt) {
    case SQL_ACCESS_MODE:
	*((SQLINTEGER *) param) = SQL_MODE_READ_WRITE;
	break;
    case SQL_AUTOCOMMIT:
	*((SQLINTEGER *) param) =
	    d->autocommit ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
	break;
    case SQL_LOGIN_TIMEOUT:
	*((SQLINTEGER *) param) = 100;
	break;
    case SQL_ODBC_CURSORS:
	*((SQLINTEGER *) param) = SQL_CUR_USE_DRIVER;
	break;
    case SQL_PACKET_SIZE:
	*((SQLINTEGER *) param) = 16384;
	break;
    case SQL_TXN_ISOLATION:
	*((SQLINTEGER *) param) = SQL_TXN_SERIALIZABLE;
	break;
    case SQL_OPT_TRACEFILE:
    case SQL_CURRENT_QUALIFIER:
    case SQL_TRANSLATE_DLL:
	*((SQLCHAR *) param) = 0;
	break;
    case SQL_OPT_TRACE:
    case SQL_QUIET_MODE:
    case SQL_KEYSET_SIZE:
    case SQL_QUERY_TIMEOUT:
    case SQL_BIND_TYPE:
    case SQL_TRANSLATE_OPTION:
	*((SQLINTEGER *) param) = 0;
	break;
    case SQL_USE_BOOKMARKS:
	*((SQLINTEGER *) param) = SQL_UB_OFF;
	break;
    case SQL_ASYNC_ENABLE:
	*((SQLINTEGER *) param) = SQL_ASYNC_ENABLE_OFF;
	break;
    case SQL_NOSCAN:
	*((SQLINTEGER *) param) = SQL_NOSCAN_OFF;
	break;
    case SQL_CONCURRENCY:
	*((SQLINTEGER *) param) = SQL_CONCUR_LOCK;
	break;
    case SQL_SIMULATE_CURSOR:
	*((SQLINTEGER *) param) = SQL_SC_NON_UNIQUE;
	break;
    case SQL_ROWSET_SIZE:
    case SQL_MAX_ROWS:
    case SQL_MAX_LENGTH:
	*((SQLINTEGER *) param) = 1000000000;
	break;
    case SQL_CURSOR_TYPE:
	*((SQLINTEGER *) param) = d->curtype;
	break;
    case SQL_RETRIEVE_DATA:
	*((SQLINTEGER *) param) = SQL_RD_ON;
	break;
    default:
	setstatd(d, -1, "unsupported connect option %d",
		 (*d->ov3) ? "HYC00" : "S1C00", opt);
	return SQL_ERROR;
    }
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Get connect option of HDBC.
 * @param dbc database connection handle
 * @param opt option to be retrieved
 * @param param output buffer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetConnectOption(SQLHDBC dbc, SQLUSMALLINT opt, SQLPOINTER param)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvgetconnectoption(dbc, opt, param);
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Get connect option of HDBC (UNICODE version).
 * @param dbc database connection handle
 * @param opt option to be retrieved
 * @param param output buffer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetConnectOptionW(SQLHDBC dbc, SQLUSMALLINT opt, SQLPOINTER param)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvgetconnectoption(dbc, opt, param);
    if (SQL_SUCCEEDED(ret)) {
	switch (opt) {
	case SQL_OPT_TRACEFILE:
	case SQL_CURRENT_QUALIFIER:
	case SQL_TRANSLATE_DLL:
	    if (param) {
		*(SQLWCHAR *) param = 0;
	    }
	    break;
	}
    }
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

/**
 * Internal set option on HDBC.
 * @param dbc database connection handle
 * @param opt option to be set
 * @param param option value
 * @result ODBC error code
 */

static SQLRETURN
drvsetconnectoption(SQLHDBC dbc, SQLUSMALLINT opt, SQLUINTEGER param)
{
    DBC *d;

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    d = (DBC *) dbc;
    switch (opt) {
    case SQL_AUTOCOMMIT:
	d->autocommit = param == SQL_AUTOCOMMIT_ON;
	if (d->autocommit && d->intrans) {
	    return endtran(d, SQL_COMMIT, 1);
	} else if (!d->autocommit) {
	    vm_end(d->vm_stmt);
	}
	break;
    default:
	setstatd(d, -1, "option value changed", "01S02");
	return SQL_SUCCESS_WITH_INFO;
    }
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Set option on HDBC.
 * @param dbc database connection handle
 * @param opt option to be set
 * @param param option value
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetConnectOption(SQLHDBC dbc, SQLUSMALLINT opt, SQLULEN param)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvsetconnectoption(dbc, opt, param);
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Set option on HDBC (UNICODE version).
 * @param dbc database connection handle
 * @param opt option to be set
 * @param param option value
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetConnectOptionW(SQLHDBC dbc, SQLUSMALLINT opt, SQLULEN param)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvsetconnectoption(dbc, opt, param);
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

#if defined(WITHOUT_DRIVERMGR) || (!defined(_WIN32) && !defined(_WIN64) && !defined(__OS2__))

/**
 * Handling of SQLConnect() connection attributes
 * for standalone operation without driver manager.
 * @param dsn DSN/driver connection string
 * @param attr attribute string to be retrieved
 * @param out output buffer
 * @param outLen length of output buffer
 * @result true or false
 */

static int
getdsnattr(char *dsn, char *attr, char *out, int outLen)
{
    char *str = dsn, *start;
    int len = strlen(attr);

    while (*str) {
	while (*str && *str == ';') {
	    ++str;
	}
	start = str;
	if ((str = strchr(str, '=')) == NULL) {
	    return 0;
	}
	if (str - start == len && strncasecmp(start, attr, len) == 0) {
	    start = ++str;
	    while (*str && *str != ';') {
		++str;
	    }
	    len = min(outLen - 1, str - start);
	    strncpy(out, start, len);
	    out[len] = '\0';
	    return 1;
	}
	while (*str && *str != ';') {
	    ++str;
	}
    }
    return 0;
}
#endif

/**
 * Internal connect to SQLite database.
 * @param dbc database connection handle
 * @param dsn DSN string
 * @param dsnLen length of DSN string or SQL_NTS
 * @result ODBC error code
 */

static SQLRETURN
drvconnect(SQLHDBC dbc, SQLCHAR *dsn, SQLSMALLINT dsnLen)
{
    DBC *d;
    int len;
    char buf[SQL_MAX_MESSAGE_LENGTH], dbname[SQL_MAX_MESSAGE_LENGTH / 4];
    char busy[SQL_MAX_MESSAGE_LENGTH / 4];
    char sflag[32], ntflag[32], nwflag[32], lnflag[32];
#if defined(HAVE_SQLITETRACE) && (HAVE_SQLITETRACE)
    char tracef[SQL_MAX_MESSAGE_LENGTH];
#endif

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    d = (DBC *) dbc;
    if (d->magic != DBC_MAGIC) {
	return SQL_INVALID_HANDLE;
    }
    if (d->sqlite != NULL) {
	setstatd(d, -1, "connection already established", "08002");
	return SQL_ERROR;
    }
    buf[0] = '\0';
    if (dsnLen == SQL_NTS) {
	len = sizeof (buf) - 1;
    } else {
	len = min(sizeof (buf) - 1, dsnLen);
    }
    if (dsn != NULL) {
	strncpy(buf, (char *) dsn, len);
    }
    buf[len] = '\0';
    if (buf[0] == '\0') {
	setstatd(d, -1, "invalid DSN", (*d->ov3) ? "HY090" : "S1090");
	return SQL_ERROR;
    }
    busy[0] = '\0';
    dbname[0] = '\0';
#ifdef WITHOUT_DRIVERMGR
    getdsnattr(buf, "database", dbname, sizeof (dbname));
    if (dbname[0] == '\0') {
	strncpy(dbname, buf, sizeof (dbname));
	dbname[sizeof (dbname) - 1] = '\0';
    }
    getdsnattr(buf, "timeout", busy, sizeof (busy));
    sflag[0] = '\0';
    ntflag[0] = '\0';
    nwflag[0] = '\0';
    lnflag[0] = '\0';
    getdsnattr(buf, "stepapi", sflag, sizeof (sflag));
    getdsnattr(buf, "notxn", ntflag, sizeof (ntflag));
    getdsnattr(buf, "nowchar", nwflag, sizeof (nwflag));
    getdsnattr(buf, "longnames", lnflag, sizeof (lnflag));
#else
    SQLGetPrivateProfileString(buf, "timeout", "100000",
			       busy, sizeof (busy), ODBC_INI);
    SQLGetPrivateProfileString(buf, "database", "",
			       dbname, sizeof (dbname), ODBC_INI);
    SQLGetPrivateProfileString(buf, "stepapi", "",
			       sflag, sizeof (sflag), ODBC_INI);
    SQLGetPrivateProfileString(buf, "notxn", "",
			       ntflag, sizeof (ntflag), ODBC_INI);
    SQLGetPrivateProfileString(buf, "nowchar", "",
			       nwflag, sizeof (nwflag), ODBC_INI);
    SQLGetPrivateProfileString(buf, "longnames", "",
			       lnflag, sizeof (lnflag), ODBC_INI);
#endif
#if defined(HAVE_SQLITETRACE) && (HAVE_SQLITETRACE)
    tracef[0] = '\0';
#ifdef WITHOUT_DRIVERMGR
    getdsnattr(buf, "tracefile", tracef, sizeof (tracef));
#else
    SQLGetPrivateProfileString(buf, "tracefile", "",
			       tracef, sizeof (tracef), ODBC_INI);
#endif
    if (tracef[0] != '\0') {
	d->trace = fopen(tracef, "a");
    }
#endif
    d->nowchar = getbool(nwflag);
    d->longnames = getbool(lnflag);
    return dbopen(d, dbname, (char *) dsn, sflag, ntflag, busy);
}

#ifndef WINTERFACE
/**
 * Connect to SQLite database.
 * @param dbc database connection handle
 * @param dsn DSN string
 * @param dsnLen length of DSN string or SQL_NTS
 * @param uid user id string or NULL
 * @param uidLen length of user id string or SQL_NTS
 * @param pass password string or NULL
 * @param passLen length of password string or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLConnect(SQLHDBC dbc, SQLCHAR *dsn, SQLSMALLINT dsnLen,
	   SQLCHAR *uid, SQLSMALLINT uidLen,
	   SQLCHAR *pass, SQLSMALLINT passLen)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvconnect(dbc, dsn, dsnLen);
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Connect to SQLite database.
 * @param dbc database connection handle
 * @param dsn DSN string
 * @param dsnLen length of DSN string or SQL_NTS
 * @param uid user id string or NULL
 * @param uidLen length of user id string or SQL_NTS
 * @param pass password string or NULL
 * @param passLen length of password string or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLConnectW(SQLHDBC dbc, SQLWCHAR *dsn, SQLSMALLINT dsnLen,
	    SQLWCHAR *uid, SQLSMALLINT uidLen,
	    SQLWCHAR *pass, SQLSMALLINT passLen)
{
    char *dsna = NULL;
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    if (dsn) {
	dsna = uc_to_utf_c(dsn, dsnLen);
	if (!dsna) {
	    DBC *d = (DBC *) dbc;

	    setstatd(d, -1, "out of memory", (*d->ov3) ? "HY000" : "S1000");
	    return SQL_ERROR;
	}
    }
    ret = drvconnect(dbc, (SQLCHAR *) dsna, SQL_NTS);
    HDBC_UNLOCK(dbc);
    uc_free(dsna);
    return ret;
}
#endif

/**
 * Internal disconnect given HDBC.
 * @param dbc database connection handle
 * @result ODBC error code
 */

static SQLRETURN
drvdisconnect(SQLHDBC dbc)
{
    DBC *d;

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    d = (DBC *) dbc;
    if (d->magic != DBC_MAGIC) {
	return SQL_INVALID_HANDLE;
    }
    if (d->intrans) {
	setstatd(d, -1, "incomplete transaction", "25000");
	return SQL_ERROR;
    }
    if (d->vm_stmt) {
	vm_end(d->vm_stmt);
    }
    if (d->sqlite) {
	sqlite_close(d->sqlite);
	d->sqlite = NULL;
    }
    freep(&d->dbname);
    freep(&d->dsn);
    return SQL_SUCCESS;
}

/**
 * Disconnect given HDBC.
 * @param dbc database connection handle
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLDisconnect(SQLHDBC dbc)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvdisconnect(dbc);
    HDBC_UNLOCK(dbc);
    return ret;
}

#if defined(WITHOUT_DRIVERMGR) || (!defined(_WIN32) && !defined(_WIN64) && !defined(__OS2__))

/**
 * Internal standalone (w/o driver manager) database connect.
 * @param dbc database connection handle
 * @param hwnd dummy window handle or NULL
 * @param connIn driver connect input string
 * @param connInLen length of driver connect input string or SQL_NTS
 * @param connOut driver connect output string
 * @param connOutMax length of driver connect output string
 * @param connOutLen output length of driver connect output string
 * @param drvcompl completion type
 * @result ODBC error code
 */

static SQLRETURN
drvdriverconnect(SQLHDBC dbc, SQLHWND hwnd,
		 SQLCHAR *connIn, SQLSMALLINT connInLen,
		 SQLCHAR *connOut, SQLSMALLINT connOutMax,
		 SQLSMALLINT *connOutLen, SQLUSMALLINT drvcompl)
{
    DBC *d;
    int len;
    char buf[SQL_MAX_MESSAGE_LENGTH * 6], dbname[SQL_MAX_MESSAGE_LENGTH];
    char dsn[SQL_MAX_MESSAGE_LENGTH / 4], busy[SQL_MAX_MESSAGE_LENGTH / 4];
    char sflag[32], ntflag[32], lnflag[32];
#if defined(HAVE_SQLITETRACE) && (HAVE_SQLITETRACE)
    char tracef[SQL_MAX_MESSAGE_LENGTH];
#endif

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    if (drvcompl != SQL_DRIVER_COMPLETE &&
	drvcompl != SQL_DRIVER_COMPLETE_REQUIRED &&
	drvcompl != SQL_DRIVER_PROMPT &&
	drvcompl != SQL_DRIVER_NOPROMPT) {
	return SQL_NO_DATA;
    }
    d = (DBC *) dbc;
    if (d->sqlite) {
	setstatd(d, -1, "connection already established", "08002");
	return SQL_ERROR;
    }
    buf[0] = '\0';
    if (connInLen == SQL_NTS) {
	len = sizeof (buf) - 1;
    } else {
	len = min(connInLen, sizeof (buf) - 1);
    }
    if (connIn != NULL) {
	strncpy(buf, (char *) connIn, len);
    }
    buf[len] = '\0';
    if (!buf[0]) {
	setstatd(d, -1, "invalid connect attributes",
		 (*d->ov3) ? "HY090" : "S1090");
	return SQL_ERROR;
    }
    dsn[0] = '\0';
    getdsnattr(buf, "DSN", dsn, sizeof (dsn));

    /* special case: connIn is sole DSN value without keywords */
    if (!dsn[0] && !strchr(buf, ';') && !strchr(buf, '=')) {
	strncpy(dsn, buf, sizeof (dsn) - 1);
	dsn[sizeof (dsn) - 1] = '\0';
    }

    busy[0] = '\0';
    getdsnattr(buf, "timeout", busy, sizeof (busy));
#ifndef WITHOUT_DRIVERMGR
    if (dsn[0] && !busy[0]) {
	SQLGetPrivateProfileString(dsn, "timeout", "100000",
				   busy, sizeof (busy), ODBC_INI);
    }
#endif
    dbname[0] = '\0';
    getdsnattr(buf, "database", dbname, sizeof (dbname));
#ifndef WITHOUT_DRIVERMGR
    if (dsn[0] && !dbname[0]) {
	SQLGetPrivateProfileString(dsn, "database", "",
				   dbname, sizeof (dbname), ODBC_INI);
    }
#endif
    sflag[0] = '\0';
    getdsnattr(buf, "stepapi", sflag, sizeof (sflag));
#ifndef WITHOUT_DRIVERMGR
    if (dsn[0] && !sflag[0]) {
	SQLGetPrivateProfileString(dsn, "stepapi", "",
				   sflag, sizeof (sflag), ODBC_INI);
    }
#endif
    ntflag[0] = '\0';
    getdsnattr(buf, "notxn", ntflag, sizeof (ntflag));
#ifndef WITHOUT_DRIVERMGR
    if (dsn[0] && !ntflag[0]) {
	SQLGetPrivateProfileString(dsn, "notxn", "",
				   ntflag, sizeof (ntflag), ODBC_INI);
    }
#endif
    lnflag[0] = '\0';
    getdsnattr(buf, "longnames", lnflag, sizeof (lnflag));
#ifndef WITHOUT_DRIVERMGR
    if (dsn[0] && !lnflag[0]) {
	SQLGetPrivateProfileString(dsn, "longnames", "",
				   lnflag, sizeof (lnflag), ODBC_INI);
    }
#endif
    if (!dbname[0] && !dsn[0]) {
	strcpy(dsn, "SQLite");
	strncpy(dbname, buf, sizeof (dbname));
	dbname[sizeof (dbname) - 1] = '\0';
    }
#if defined(HAVE_SQLITETRACE) && (HAVE_SQLITETRACE)
    tracef[0] = '\0';
    getdsnattr(buf, "tracefile", tracef, sizeof (tracef));
#ifndef WITHOUT_DRIVERMGR
    if (dsn[0] && !tracef[0]) {
	SQLGetPrivateProfileString(dsn, "tracefile", "",
				   tracef, sizeof (tracef), ODBC_INI);
    }
#endif
#endif
    if (connOut || connOutLen) {
	int count;

	buf[0] = '\0';
	count = snprintf(buf, sizeof (buf),
#if defined(HAVE_SQLITETRACE) && (HAVE_SQLITETRACE)
			 "DSN=%s;Database=%s;StepAPI=%s;NoTXN=%s;"
			 "Timeout=%s;LongNames=%s;Tracefile=%s",
			 dsn, dbname, sflag, ntflag, busy, lnflag, tracef
#else
			 "DSN=%s;Database=%s;StepAPI=%s;NoTXN=%s;"
			 "Timeout=%s;LongNames=%s",
			 dsn, dbname, sflag, ntflag, busy, lnflag
#endif
			);
	if (count < 0) {
	    buf[sizeof (buf) - 1] = '\0';
	}
	len = min(connOutMax - 1, strlen(buf));
	if (connOut) {
	    strncpy((char *) connOut, buf, len);
	    connOut[len] = '\0';
	}
	if (connOutLen) {
	    *connOutLen = len;
	}
    }
#if defined(HAVE_SQLITETRACE) && (HAVE_SQLITETRACE)
    if (tracef[0] != '\0') {
	d->trace = fopen(tracef, "a");
    }
#endif
    d->longnames = getbool(lnflag);
    return dbopen(d, dbname, dsn, sflag, ntflag, busy);
}
#endif

/**
 * Internal free function for HSTMT.
 * @param stmt statement handle
 * @result ODBC error code
 */

static SQLRETURN
freestmt(SQLHSTMT stmt)
{
    STMT *s;
    DBC *d;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    freeresult(s, 1);
    freep(&s->query);
    d = (DBC *) s->dbc;
    if (d && d->magic == DBC_MAGIC) {
	STMT *p, *n;

	p = NULL;
	n = d->stmt;
	while (n) {
	    if (n == s) {
		break;
	    }
	    p = n;
	    n = n->next;
	}
	if (n) {
	    if (p) {
		p->next = s->next;
	    } else {
		d->stmt = s->next;
	    }
	}
    }
    freeparams(s);
    freep(&s->bindparms);
    if (s->row_status0 != &s->row_status1) {
	freep(&s->row_status0);
	s->rowset_size = 1;
	s->row_status0 = &s->row_status1;
    }
    xfree(s);
    return SQL_SUCCESS;
}

/**
 * Allocate HSTMT given HDBC (driver internal version).
 * @param dbc database connection handle
 * @param stmt pointer to statement handle
 * @result ODBC error code
 */

static SQLRETURN
drvallocstmt(SQLHDBC dbc, SQLHSTMT *stmt)
{
    DBC *d;
    STMT *s, *sl, *pl;

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    d = (DBC *) dbc;
    if (d->magic != DBC_MAGIC || stmt == NULL) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) xmalloc(sizeof (STMT));
    if (s == NULL) {
	*stmt = SQL_NULL_HSTMT;
	return SQL_ERROR;
    }
    *stmt = (SQLHSTMT) s;
    memset(s, 0, sizeof (STMT));
    s->dbc = dbc;
    s->ov3 = d->ov3;
    s->nowchar[0] = d->nowchar;
    s->nowchar[1] = 0;
    s->longnames = d->longnames;
    s->curtype = d->curtype;
    s->row_status0 = &s->row_status1;
    s->rowset_size = 1;
    s->retr_data = SQL_RD_ON;
    s->bind_type = SQL_BIND_BY_COLUMN;
    s->bind_offs = NULL;
    s->paramset_size = 1;
    s->parm_bind_type = SQL_PARAM_BIND_BY_COLUMN;
#ifdef _WIN64
    sprintf((char *) s->cursorname, "CUR_%I64X", (SQLUBIGINT) *stmt);
#else
    sprintf((char *) s->cursorname, "CUR_%016lX", (long) *stmt);
#endif
    sl = d->stmt;
    pl = NULL;
    while (sl) {
	pl = sl;
	sl = sl->next;
    }
    if (pl) {
	pl->next = s;
    } else {
	d->stmt = s;
    }
    return SQL_SUCCESS;
}

/**
 * Allocate HSTMT given HDBC.
 * @param dbc database connection handle
 * @param stmt pointer to statement handle
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLAllocStmt(SQLHDBC dbc, SQLHSTMT *stmt)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvallocstmt(dbc, stmt);
    HDBC_UNLOCK(dbc);
    return ret;
}

/**
 * Internal function to perform certain kinds of free/close on STMT.
 * @param stmt statement handle
 * @param opt SQL_RESET_PARAMS, SQL_UNBIND, SQL_CLOSE, or SQL_DROP
 * @result ODBC error code
 */

static SQLRETURN
drvfreestmt(SQLHSTMT stmt, SQLUSMALLINT opt)
{
    STMT *s;
    SQLRETURN ret = SQL_SUCCESS;
    SQLHDBC dbc;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    HSTMT_LOCK(stmt);
    s = (STMT *) stmt;
    dbc = s->dbc;
    switch (opt) {
    case SQL_RESET_PARAMS:
	freeparams(s);
	break;
    case SQL_UNBIND:
	unbindcols(s);
	break;
    case SQL_CLOSE:
	vm_end_if(s);
	freeresult(s, 0);
	break;
    case SQL_DROP:
	vm_end_if(s);
	ret = freestmt(stmt);
	break;
    default:
	setstat(s, -1, "unsupported option", (*s->ov3) ? "HYC00" : "S1C00");
	ret = SQL_ERROR;
	break;
    }
    HDBC_UNLOCK(dbc);
    return ret;
}

/**
 * Free HSTMT.
 * @param stmt statement handle
 * @param opt SQL_RESET_PARAMS, SQL_UNBIND, SQL_CLOSE, or SQL_DROP
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLFreeStmt(SQLHSTMT stmt, SQLUSMALLINT opt)
{
    return drvfreestmt(stmt, opt);
}

/**
 * Cancel HSTMT closing cursor.
 * @param stmt statement handle
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLCancel(SQLHSTMT stmt)
{
    if (stmt != SQL_NULL_HSTMT) {
	DBC *d = (DBC *) ((STMT *) stmt)->dbc;
#if defined(_WIN32) || defined(_WIN64)
	/* interrupt when other thread owns critical section */
	int i;

	for (i = 0; i < 2; i++) {
	    if (d->magic == DBC_MAGIC && d->env &&
		d->env->magic == ENV_MAGIC &&
		d->env->owner != GetCurrentThreadId() &&
		d->env->owner != 0) {
		d->busyint = 1;
		sqlite_interrupt(d->sqlite);
	    }
	    Sleep(1);
	}
#else
	if (d->magic == DBC_MAGIC) {
	    d->busyint = 1;
	    sqlite_interrupt(d->sqlite);
	}
#endif
    }
    return drvfreestmt(stmt, SQL_CLOSE);
}

/**
 * Internal function to get cursor name of STMT.
 * @param stmt statement handle
 * @param cursor output buffer
 * @param buflen length of output buffer
 * @param lenp output length
 * @result ODBC error code
 */

static SQLRETURN
drvgetcursorname(SQLHSTMT stmt, SQLCHAR *cursor, SQLSMALLINT buflen,
		 SQLSMALLINT *lenp)
{
    STMT *s;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (lenp && !cursor) {
	*lenp = strlen((char *) s->cursorname);
	return SQL_SUCCESS;
    }
    if (cursor) {
	if (buflen > 0) {
	    strncpy((char *) cursor, (char *) s->cursorname, buflen - 1);
	    cursor[buflen - 1] = '\0';
	}
	if (lenp) {
	    *lenp = min(strlen((char *) s->cursorname), buflen - 1);
	}
    }
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Get cursor name of STMT.
 * @param stmt statement handle
 * @param cursor output buffer
 * @param buflen length of output buffer
 * @param lenp output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetCursorName(SQLHSTMT stmt, SQLCHAR *cursor, SQLSMALLINT buflen,
		 SQLSMALLINT *lenp)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvgetcursorname(stmt, cursor, buflen, lenp);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Get cursor name of STMT (UNICODE version).
 * @param stmt statement handle
 * @param cursor output buffer
 * @param buflen length of output buffer
 * @param lenp output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetCursorNameW(SQLHSTMT stmt, SQLWCHAR *cursor, SQLSMALLINT buflen,
		  SQLSMALLINT *lenp)
{
    SQLRETURN ret;
    SQLSMALLINT len = 0;

    HSTMT_LOCK(stmt);
    ret = drvgetcursorname(stmt, (SQLCHAR *) cursor, buflen, &len);
    if (ret == SQL_SUCCESS) {
	SQLWCHAR *c = NULL;

	if (cursor) {
	    c = uc_from_utf((SQLCHAR *) cursor, len);
	    if (!c) {
		ret = nomem((STMT *) stmt);
		goto done;
	    }
	    c[len] = 0;
	    len = uc_strlen(c);
	    if (buflen > 0) {
		uc_strncpy(cursor, c, buflen - 1);
		cursor[buflen - 1] = 0;
	    }
	    uc_free(c);
	}
	if (lenp) {
	    *lenp = min(len, buflen - 1);
	}
    }
done:
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Internal function to set cursor name on STMT.
 * @param stmt statement handle
 * @param cursor new cursor name
 * @param len length of cursor name or SQL_NTS
 * @result ODBC error code
 */

static SQLRETURN
drvsetcursorname(SQLHSTMT stmt, SQLCHAR *cursor, SQLSMALLINT len)
{
    STMT *s;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (!cursor ||
	!((cursor[0] >= 'A' && cursor[0] <= 'Z') ||
	  (cursor[0] >= 'a' && cursor[0] <= 'z'))) {
	setstat(s, -1, "invalid cursor name", (*s->ov3) ? "HYC00" : "S1C00");
	return SQL_ERROR;
    }
    if (len == SQL_NTS) {
	len = sizeof (s->cursorname) - 1;
    } else {
	len = min(sizeof (s->cursorname) - 1, len);
    }
    strncpy((char *) s->cursorname, (char *) cursor, len);
    s->cursorname[len] = '\0';
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Set cursor name on STMT.
 * @param stmt statement handle
 * @param cursor new cursor name
 * @param len length of cursor name or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetCursorName(SQLHSTMT stmt, SQLCHAR *cursor, SQLSMALLINT len)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvsetcursorname(stmt, cursor, len);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Set cursor name on STMT (UNICODE version).
 * @param stmt statement handle
 * @param cursor new cursor name
 * @param len length of cursor name or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLSetCursorNameW(SQLHSTMT stmt, SQLWCHAR *cursor, SQLSMALLINT len)
{
    char *c = NULL;
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    if (cursor) {
	c = uc_to_utf_c(cursor, len);
	if (!c) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    ret = drvsetcursorname(stmt, (SQLCHAR *) c, SQL_NTS);
done:
    HSTMT_UNLOCK(stmt);
    uc_free(c);
    return ret;
}
#endif

/**
 * Close open cursor.
 * @param stmt statement handle
 * @return ODBC error code
 */

SQLRETURN SQL_API
SQLCloseCursor(SQLHSTMT stmt)
{
    return drvfreestmt(stmt, SQL_CLOSE);
}

/**
 * Allocate a HENV, HDBC, or HSTMT handle.
 * @param type handle type
 * @param input input handle (HENV, HDBC)
 * @param output pointer to output handle (HENV, HDBC, HSTMT)
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLAllocHandle(SQLSMALLINT type, SQLHANDLE input, SQLHANDLE *output)
{
    SQLRETURN ret;

    switch (type) {
    case SQL_HANDLE_ENV:
	ret = drvallocenv((SQLHENV *) output);
	if (ret == SQL_SUCCESS) {
	    ENV *e = (ENV *) *output;

	    if (e && e->magic == ENV_MAGIC) {
		e->ov3 = 1;
	    }
	}
	return ret;
    case SQL_HANDLE_DBC:
	return drvallocconnect((SQLHENV) input, (SQLHDBC *) output);
    case SQL_HANDLE_STMT:
	HDBC_LOCK((SQLHDBC) input);
	ret = drvallocstmt((SQLHDBC) input, (SQLHSTMT *) output);
	HDBC_UNLOCK((SQLHDBC) input);
	return ret;
    }
    return SQL_ERROR;
}

/**
 * Free a HENV, HDBC, or HSTMT handle.
 * @param type handle type
 * @param h handle (HENV, HDBC, or HSTMT)
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLFreeHandle(SQLSMALLINT type, SQLHANDLE h)
{
    switch (type) {
    case SQL_HANDLE_ENV:
	return drvfreeenv((SQLHENV) h);
    case SQL_HANDLE_DBC:
	return drvfreeconnect((SQLHDBC) h);
    case SQL_HANDLE_STMT:
	return drvfreestmt((SQLHSTMT) h, SQL_DROP);
    }
    return SQL_ERROR;
}

/**
 * Free dynamically allocated column descriptions of STMT.
 * @param s statement pointer
 */

static void
freedyncols(STMT *s)
{
    if (s->dyncols) {
	int i;

	for (i = 0; i < s->dcols; i++) {
	    freep(&s->dyncols[i].typename);
	}
	if (s->cols == s->dyncols) {
	    s->cols = NULL;
	    s->ncols = 0;
	}
	freep(&s->dyncols);
    }
    s->dcols = 0;
}

/**
 * Free statement's result.
 * @param s statement pointer
 * @param clrcols flag to clear column information
 *
 * The result rows are free'd using the rowfree function pointer.
 * If clrcols is greater than zero, then column bindings and dynamic column
 * descriptions are free'd.
 * If clrcols is less than zero, then dynamic column descriptions are free'd.
 */

static void
freeresult(STMT *s, int clrcols)
{
#if (HAVE_ENCDEC)
    freep(&s->bincache);
    freep(&s->hexcache);
    s->bincell = NULL;
#endif
    if (s->rows) {
	if (s->rowfree) {
	    s->rowfree(s->rows);
	    s->rowfree = NULL;
	}
	s->rows = NULL;
    }
    s->nrows = -1;
    if (clrcols > 0) {
	freep(&s->bindcols);
	s->nbindcols = 0;
    }
    if (clrcols) {
	freedyncols(s);
	s->cols = NULL;
	s->ncols = 0;
	s->nowchar[1] = 0;
    }
}

/**
 * Reset bound columns to unbound state.
 * @param s statement pointer
 */

static void
unbindcols(STMT *s)
{
    int i;

    s->bkmrkcol.type = -1;
    s->bkmrkcol.max = 0;
    s->bkmrkcol.lenp = NULL;
    s->bkmrkcol.valp = NULL;
    s->bkmrkcol.index = 0;
    s->bkmrkcol.offs = 0;
    for (i = 0; s->bindcols && i < s->nbindcols; i++) {
	s->bindcols[i].type = -1;
	s->bindcols[i].max = 0;
	s->bindcols[i].lenp = NULL;
	s->bindcols[i].valp = NULL;
	s->bindcols[i].index = i;
	s->bindcols[i].offs = 0;
    }
}

/**
 * Reallocate space for bound columns.
 * @param s statement pointer
 * @param ncols number of columns
 * @result ODBC error code
 */

static SQLRETURN
mkbindcols(STMT *s, int ncols)
{
    if (s->bindcols) {
	if (s->nbindcols < ncols) {
	    int i;
	    BINDCOL *bindcols =
		xrealloc(s->bindcols, ncols * sizeof (BINDCOL));

	    if (!bindcols) {
		return nomem(s);
	    }
	    for (i = s->nbindcols; i < ncols; i++) {
		bindcols[i].type = -1;
		bindcols[i].max = 0;
		bindcols[i].lenp = NULL;
		bindcols[i].valp = NULL;
		bindcols[i].index = i;
		bindcols[i].offs = 0;
	    }
	    s->bindcols = bindcols;
	    s->nbindcols = ncols;
	}
    } else if (ncols > 0) {
	s->bindcols = (BINDCOL *) xmalloc(ncols * sizeof (BINDCOL));
	if (!s->bindcols) {
	    return nomem(s);
	}
	s->nbindcols = ncols;
	unbindcols(s);
    }
    return SQL_SUCCESS;
}

/**
 * Internal function to retrieve row data, used by SQLFetch() and
 * friends and SQLGetData().
 * @param s statement pointer
 * @param col column number, 0 based
 * @param otype output data type
 * @param val output buffer
 * @param len length of output buffer
 * @param lenp output length
 * @param partial flag for partial data retrieval
 * @result ODBC error code
 */

static SQLRETURN
getrowdata(STMT *s, SQLUSMALLINT col, SQLSMALLINT otype,
	   SQLPOINTER val, SQLLEN len, SQLLEN *lenp, int partial)
{
    char **data, valdummy[16];
    SQLLEN dummy;
    SQLINTEGER *ilenp = NULL;
    int valnull = 0;
    int type = otype;
    SQLRETURN sret = SQL_NO_DATA;

    if (!lenp) {
	lenp = &dummy;
    }
    /* workaround for JDK 1.7.0 on x86_64 */
    if (((SQLINTEGER *) lenp) + 1 == (SQLINTEGER *) val) {
	ilenp = (SQLINTEGER *) lenp;
	lenp = &dummy;
    }
    if (col >= s->ncols) {
	setstat(s, -1, "invalid column", (*s->ov3) ? "07009" : "S1002");
	return SQL_ERROR;
    }
    if (!s->rows) {
	*lenp = SQL_NULL_DATA;
	goto done;
    }
    if (s->rowp < 0 || s->rowp >= s->nrows) {
	*lenp = SQL_NULL_DATA;
	goto done;
    }
    if (s->retr_data != SQL_RD_ON) {
	return SQL_SUCCESS;
    }
    type = mapdeftype(type, s->cols[col].type, s->cols[col].nosign ? 1 : 0,
		      s->nowchar[0]);

#if (defined(_WIN32) || defined(_WIN64)) && defined(WINTERFACE)
    /* MS Access hack part 3 (map SQL_C_DEFAULT to SQL_C_CHAR) */
    if (type == SQL_C_WCHAR && otype == SQL_C_DEFAULT) {
	type = SQL_C_CHAR;
    }
#endif

#if (HAVE_ENCDEC)
    if (type == SQL_C_CHAR) {
	switch (s->cols[col].type) {
	case SQL_BINARY:
	case SQL_VARBINARY:
	case SQL_LONGVARBINARY:
	    type = SQL_C_BINARY;
	    break;
	}
#ifdef WCHARSUPPORT
    } else if (type == SQL_C_WCHAR) {
	switch (s->cols[col].type) {
	case SQL_BINARY:
	case SQL_VARBINARY:
	case SQL_LONGVARBINARY:
	    type = SQL_C_BINARY;
	    break;
	}
#endif
    }
#endif
    data = s->rows + s->ncols + (s->rowp * s->ncols) + col;
    if (!val) {
	valnull = 1;
	val = (SQLPOINTER) valdummy;
    }
    if (*data == NULL) {
	*lenp = SQL_NULL_DATA;
	switch (type) {
	case SQL_C_UTINYINT:
	case SQL_C_TINYINT:
	case SQL_C_STINYINT:
#ifdef SQL_BIT
	case SQL_C_BIT:
#endif
	    *((char *) val) = 0;
	    break;
	case SQL_C_USHORT:
	case SQL_C_SHORT:
	case SQL_C_SSHORT:
	    *((short *) val) = 0;
	    break;
	case SQL_C_ULONG:
	case SQL_C_LONG:
	case SQL_C_SLONG:
	    *((SQLINTEGER *) val) = 0;
	    break;
	case SQL_C_FLOAT:
	    *((float *) val) = 0;
	    break;
	case SQL_C_DOUBLE:
	    *((double *) val) = 0;
	    break;
	case SQL_C_BINARY:
	case SQL_C_CHAR:
#ifdef WCHARSUPPORT
	case SQL_C_WCHAR:
	    if (type == SQL_C_WCHAR) {
		*((SQLWCHAR *) val) = '\0';
	    } else {
		*((char *) val) = '\0';
	    }
#else
	    *((char *) val) = '\0';
#endif
	    break;
#ifdef SQL_C_TYPE_DATE
	case SQL_C_TYPE_DATE:
#endif
	case SQL_C_DATE:
	    memset((DATE_STRUCT *) val, 0, sizeof (DATE_STRUCT));
	    break;
#ifdef SQL_C_TYPE_TIME
	case SQL_C_TYPE_TIME:
#endif
	case SQL_C_TIME:
	    memset((TIME_STRUCT *) val, 0, sizeof (TIME_STRUCT));
	    break;
#ifdef SQL_C_TYPE_TIMESTAMP
	case SQL_C_TYPE_TIMESTAMP:
#endif
	case SQL_C_TIMESTAMP:
	    memset((TIMESTAMP_STRUCT *) val, 0, sizeof (TIMESTAMP_STRUCT));
	    break;
	default:
	    return SQL_ERROR;
	}
    } else {
	char *endp = NULL;

	switch (type) {
	case SQL_C_UTINYINT:
	case SQL_C_TINYINT:
	case SQL_C_STINYINT:
	    *((char *) val) = strtol(*data, &endp, 0);
	    if (endp && endp == *data) {
		*lenp = SQL_NULL_DATA;
	    } else {
		*lenp = sizeof (char);
	    }
	    break;
#ifdef SQL_BIT
	case SQL_C_BIT:
	    *((char *) val) = getbool(*data);
	    *lenp = sizeof (char);
	    break;
#endif
	case SQL_C_USHORT:
	case SQL_C_SHORT:
	case SQL_C_SSHORT:
	    *((short *) val) = strtol(*data, &endp, 0);
	    if (endp && endp == *data) {
		*lenp = SQL_NULL_DATA;
	    } else {
		*lenp = sizeof (short);
	    }
	    break;
	case SQL_C_ULONG:
	case SQL_C_LONG:
	case SQL_C_SLONG:
	    *((SQLINTEGER *) val) = strtol(*data, &endp, 0);
	    if (endp && endp == *data) {
		*lenp = SQL_NULL_DATA;
	    } else {
		*lenp = sizeof (SQLINTEGER);
	    }
	    break;
	case SQL_C_FLOAT:
	    *((float *) val) = ln_strtod(*data, &endp);
	    if (endp && endp == *data) {
		*lenp = SQL_NULL_DATA;
	    } else {
		*lenp = sizeof (float);
	    }
	    break;
	case SQL_C_DOUBLE:
	    *((double *) val) = ln_strtod(*data, &endp);
	    if (endp && endp == *data) {
		*lenp = SQL_NULL_DATA;
	    } else {
		*lenp = sizeof (double);
	    }
	    break;
	case SQL_C_BINARY:
#if (HAVE_ENCDEC)
	{
	    int dlen, offs = 0;
	    char *bin;

	    if (*data == s->bincell && s->bincache) {
		bin = s->bincache;
		dlen = s->binlen;
	    } else {
		freep(&s->bincache);
		freep(&s->hexcache);
		s->bincell = NULL;
		dlen = strlen(*data);
		bin = xmalloc(dlen + 1);
		if (!bin) {
		    return nomem(s);
		}
		dlen = sqlite_decode_binary((unsigned char *) *data,
					    (unsigned char *) bin);
		if (dlen < 0) {
		    freep(&bin);
		    setstat(s, -1, "error decoding binary data",
			    (*s->ov3) ? "HY000" : "S1000");
		    return SQL_ERROR;
		}
		s->bincache = bin;
		s->binlen = dlen;
		s->bincell = *data;
	    }
	    if (partial && len && s->bindcols) {
		if (s->bindcols[col].offs >= dlen) {
		    *lenp = 0;
		    if (!dlen && s->bindcols[col].offs == dlen) {
			s->bindcols[col].offs = 1;
			sret = SQL_SUCCESS;
			goto done;
		    }
		    s->bindcols[col].offs = 0;
		    sret = SQL_NO_DATA;
		    goto done;
		}
		offs = s->bindcols[col].offs;
		dlen -= offs;
	    }
	    if (val && !valnull && len) {
		memcpy(val, bin + offs, min(len, dlen));
	    }
	    if (valnull || len < 1) {
		*lenp = dlen;
	    } else {
		*lenp = min(len, dlen);
		if (*lenp == len && *lenp != dlen) {
		    *lenp = SQL_NO_TOTAL;
		}
	    }
	    if (partial && len && s->bindcols) {
		if (*lenp == SQL_NO_TOTAL) {
		    *lenp = dlen;
		    s->bindcols[col].offs += len;
		    setstat(s, -1, "data right truncated", "01004");
		    if (s->bindcols[col].lenp) {
			*s->bindcols[col].lenp = dlen;
		    }
		    sret = SQL_SUCCESS_WITH_INFO;
		    goto done;
		}
		s->bindcols[col].offs += *lenp;
	    }
	    if (*lenp == SQL_NO_TOTAL) {
		*lenp = dlen;
		setstat(s, -1, "data right truncated", "01004");
		sret = SQL_SUCCESS_WITH_INFO;
		goto done;
	    }
	    break;
	}
#endif
#ifdef WCHARSUPPORT
	case SQL_C_WCHAR:
#endif
	case SQL_C_CHAR: {
	    int doz, zlen = len - 1;
	    int dlen = strlen(*data);
	    int offs = 0;
#ifdef WCHARSUPPORT
	    SQLWCHAR *ucdata = NULL;
#endif

#if (defined(_WIN32) || defined(_WIN64)) && defined(WINTERFACE)
	    /* MS Access hack part 2 (reserved error -7748) */
	    if (!valnull &&
		(s->cols == statSpec2P || s->cols == statSpec3P) &&
		type == SQL_C_WCHAR) {
		if (len > 0 && len <= sizeof (SQLWCHAR)) {
		    ((char *) val)[0] = data[0][0];
		    memset((char *) val + 1, 0, len - 1);
		    *lenp = 1;
		    sret = SQL_SUCCESS;
		    goto done;
		}
	    }
#endif

#ifdef WCHARSUPPORT
	    switch (type) {
	    case SQL_C_CHAR:
		doz = 1;
		break;
	    case SQL_C_WCHAR:
		doz = sizeof (SQLWCHAR);
		break;
	    default:
		doz = 0;
		break;
	    }
	    if (type == SQL_C_WCHAR) {
		ucdata = uc_from_utf((SQLCHAR *) *data, dlen);
		if (!ucdata) {
		    return nomem(s);
		}
		dlen = uc_strlen(ucdata) * sizeof (SQLWCHAR);
	    }
#else
	    doz = type == SQL_C_CHAR ? 1 : 0;
#endif
	    if (partial && len && s->bindcols) {
		if (s->bindcols[col].offs >= dlen) {
#ifdef WCHARSUPPORT
		    uc_free(ucdata);
#endif
		    *lenp = 0;
		    if (doz && val) {
#ifdef WCHARSUPPORT
			if (type == SQL_C_WCHAR) {
			    ((SQLWCHAR *) val)[0] = 0;
			} else {
			    ((char *) val)[0] = '\0';
			}
#else
			((char *) val)[0] = '\0';
#endif
		    }
		    if (!dlen && s->bindcols[col].offs == dlen) {
			s->bindcols[col].offs = 1;
			sret = SQL_SUCCESS;
			goto done;
		    }
		    s->bindcols[col].offs = 0;
		    sret = SQL_NO_DATA;
		    goto done;
		}
		offs = s->bindcols[col].offs;
		dlen -= offs;
	    }
	    if (val && !valnull && len) {
#ifdef WCHARSUPPORT
		if (type == SQL_C_WCHAR) {
		    uc_strncpy(val, ucdata + offs / sizeof (SQLWCHAR),
			       (len - doz) / sizeof (SQLWCHAR));
		} else {
		    strncpy(val, *data + offs, len - doz);
		}
#else
		strncpy(val, *data + offs, len - doz);
#endif
	    }
	    if (valnull || len < 1) {
		*lenp = dlen;
	    } else {
		*lenp = min(len - doz, dlen);
		if (*lenp == len - doz && *lenp != dlen) {
		    *lenp = SQL_NO_TOTAL;
		} else if (*lenp < zlen) {
		    zlen = *lenp;
		}
	    }
	    if (len && !valnull && doz) {
#ifdef WCHARSUPPORT
		if (type == SQL_C_WCHAR) {
		    ((SQLWCHAR *) val)[zlen / sizeof (SQLWCHAR)] = 0;
		} else {
		    ((char *) val)[zlen] = '\0';
		}
#else
		((char *) val)[zlen] = '\0';
#endif
	    }
#ifdef WCHARSUPPORT
	    uc_free(ucdata);
#endif
	    if (partial && len && s->bindcols) {
		if (*lenp == SQL_NO_TOTAL) {
		    *lenp = dlen;
		    s->bindcols[col].offs += len - doz;
		    setstat(s, -1, "data right truncated", "01004");
		    if (s->bindcols[col].lenp) {
			*s->bindcols[col].lenp = dlen;
		    }
		    sret = SQL_SUCCESS_WITH_INFO;
		    goto done;
		}
		s->bindcols[col].offs += *lenp;
	    }
	    if (*lenp == SQL_NO_TOTAL) {
		*lenp = dlen;
		setstat(s, -1, "data right truncated", "01004");
		sret = SQL_SUCCESS_WITH_INFO;
		goto done;
	    }
	    break;
	}
#ifdef SQL_C_TYPE_DATE
	case SQL_C_TYPE_DATE:
#endif
	case SQL_C_DATE:
	    if (str2date(*data, (DATE_STRUCT *) val) < 0) {
		*lenp = SQL_NULL_DATA;
	    } else {
		*lenp = sizeof (DATE_STRUCT);
	    }
	    break;
#ifdef SQL_C_TYPE_TIME
	case SQL_C_TYPE_TIME:
#endif
	case SQL_C_TIME:
	    if (str2time(*data, (TIME_STRUCT *) val) < 0) {
		*lenp = SQL_NULL_DATA;
	    } else {
		*lenp = sizeof (TIME_STRUCT);
	    }
	    break;
#ifdef SQL_C_TYPE_TIMESTAMP
	case SQL_C_TYPE_TIMESTAMP:
#endif
	case SQL_C_TIMESTAMP:
	    if (str2timestamp(*data, (TIMESTAMP_STRUCT *) val) < 0) {
		*lenp = SQL_NULL_DATA;
	    } else {
		*lenp = sizeof (TIMESTAMP_STRUCT);
	    }
	    switch (s->cols[col].prec) {
	    case 0:
		((TIMESTAMP_STRUCT *) val)->fraction = 0;
		break;
	    case 1:
		((TIMESTAMP_STRUCT *) val)->fraction /= 100000000;
		((TIMESTAMP_STRUCT *) val)->fraction *= 100000000;
		break;
	    case 2:
		((TIMESTAMP_STRUCT *) val)->fraction /= 10000000;
		((TIMESTAMP_STRUCT *) val)->fraction *= 10000000;
		break;
	    }
	    break;
	default:
	    return SQL_ERROR;
	}
    }
    sret = SQL_SUCCESS;
done:
    if (ilenp) {
	*ilenp = *lenp;
    }
    return sret;
}

/**
 * Internal bind C variable to column of result set.
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param type output type
 * @param val output buffer
 * @param max length of output buffer
 * @param lenp output length pointer
 * @result ODBC error code
 */

static SQLRETURN
drvbindcol(SQLHSTMT stmt, SQLUSMALLINT col, SQLSMALLINT type,
	   SQLPOINTER val, SQLLEN max, SQLLEN *lenp)
{
    STMT *s;
    int sz = 0;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (col < 1) {
	if (col == 0 && s->bkmrk && type == SQL_C_BOOKMARK) {
	    s->bkmrkcol.type = type;
	    s->bkmrkcol.max = sizeof (SQLINTEGER);
	    s->bkmrkcol.lenp = lenp;
	    s->bkmrkcol.valp = val;
	    s->bkmrkcol.offs = 0;
	    if (lenp) {
		*lenp = 0;
	    }
	    return SQL_SUCCESS;
	}
	setstat(s, -1, "invalid column", (*s->ov3) ? "07009" : "S1002");
	return SQL_ERROR;
    }
    if (mkbindcols(s, col) != SQL_SUCCESS) {
	return SQL_ERROR;
    }
    --col;
    if (type == SQL_C_DEFAULT) {
	type = mapdeftype(type, s->cols[col].type, 0,
			  s->nowchar[0] || s->nowchar[1]);
    }
    switch (type) {
    case SQL_C_LONG:
    case SQL_C_ULONG:
    case SQL_C_SLONG:
	sz = sizeof (SQLINTEGER);
	break;
    case SQL_C_TINYINT:
    case SQL_C_UTINYINT:
    case SQL_C_STINYINT:
	sz = sizeof (SQLCHAR);
	break;
    case SQL_C_SHORT:
    case SQL_C_USHORT:
    case SQL_C_SSHORT:
	sz = sizeof (short);
	break;
    case SQL_C_FLOAT:
	sz = sizeof (SQLFLOAT);
	break;
    case SQL_C_DOUBLE:
	sz = sizeof (SQLDOUBLE);
	break;
    case SQL_C_TIMESTAMP:
	sz = sizeof (SQL_TIMESTAMP_STRUCT);
	break;
    case SQL_C_TIME:
	sz = sizeof (SQL_TIME_STRUCT);
	break;
    case SQL_C_DATE:
	sz = sizeof (SQL_DATE_STRUCT);
	break;
    case SQL_C_CHAR:
	break;
#ifdef WCHARSUPPORT
    case SQL_C_WCHAR:
	break;
#endif
#ifdef SQL_C_TYPE_DATE
    case SQL_C_TYPE_DATE:
	sz = sizeof (SQL_DATE_STRUCT);
	break;
#endif
#ifdef SQL_C_TYPE_TIME
    case SQL_C_TYPE_TIME:
	sz = sizeof (SQL_TIME_STRUCT);
	break;
#endif
#ifdef SQL_C_TYPE_TIMESTAMP
    case SQL_C_TYPE_TIMESTAMP:
	sz = sizeof (SQL_TIMESTAMP_STRUCT);
	break;
#endif
#ifdef SQL_BIT
    case SQL_C_BIT:
	sz = sizeof (SQLCHAR);
	break;
#endif
#if (HAVE_ENCDEC)
    case SQL_C_BINARY:
	break;
#endif
    default:
	if (val == NULL) {
	    /* fall through, unbinding column */
	    break;
	}
	setstat(s, -1, "invalid type %d", "HY003", type);
	return SQL_ERROR;
    }
    if (val == NULL) {
	/* unbind column */
	s->bindcols[col].type = -1;
	s->bindcols[col].max = 0;
	s->bindcols[col].lenp = NULL;
	s->bindcols[col].valp = NULL;
	s->bindcols[col].offs = 0;
    } else {
	if (sz == 0 && max < 0) {
	    setstat(s, -1, "invalid length", "HY090");
	    return SQL_ERROR;
	}
	s->bindcols[col].type = type;
	s->bindcols[col].max = (sz == 0) ? max : sz;
	s->bindcols[col].lenp = lenp;
	s->bindcols[col].valp = val;
	s->bindcols[col].offs = 0;
	if (lenp) {
	    *lenp = 0;
	}
    }
    return SQL_SUCCESS; 
}

/**
 * Bind C variable to column of result set.
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param type output type
 * @param val output buffer
 * @param max length of output buffer
 * @param lenp output length pointer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLBindCol(SQLHSTMT stmt, SQLUSMALLINT col, SQLSMALLINT type,
	   SQLPOINTER val, SQLLEN max, SQLLEN *lenp)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvbindcol(stmt, col, type, val, max, lenp);
    HSTMT_UNLOCK(stmt);
    return ret;
}

/**
 * Columns for result set of SQLTables().
 */

static COL tableSpec2[] = {
    { "SYSTEM", "COLUMN", "TABLE_QUALIFIER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "TABLE_OWNER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLUMN", "TABLE_TYPE", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "REMARKS", SCOL_VARCHAR, 50 }
};

static COL tableSpec3[] = {
    { "SYSTEM", "COLUMN", "TABLE_CAT", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "TABLE_SCHEM", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLUMN", "TABLE_TYPE", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "REMARKS", SCOL_VARCHAR, 50 }
};

/**
 * Retrieve information on tables and/or views.
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param type types of tables string or NULL
 * @param typeLen length of types of tables string or SQL_NTS
 * @result ODBC error code
 */

static SQLRETURN
drvtables(SQLHSTMT stmt,
	  SQLCHAR *cat, SQLSMALLINT catLen,
	  SQLCHAR *schema, SQLSMALLINT schemaLen,
	  SQLCHAR *table, SQLSMALLINT tableLen,
	  SQLCHAR *type, SQLSMALLINT typeLen)
{
    SQLRETURN ret;
    STMT *s;
    DBC *d;
    int ncols, asize, rc, size, npatt;
    char *errp = NULL, tname[512];
    char *where = "(type = 'table' or type = 'view')";

    ret = mkresultset(stmt, tableSpec2, array_size(tableSpec2),
		      tableSpec3, array_size(tableSpec3), &asize);
    if (ret != SQL_SUCCESS) {
	return ret;
    }
    s = (STMT *) stmt;
    d = (DBC *) s->dbc;
    if (type && (typeLen > 0 || typeLen == SQL_NTS) && type[0] == '%') {
	int size = 3 * asize;

	s->rows = xmalloc(size * sizeof (char *));
	if (!s->rows) {
	    s->nrows = 0;
	    return nomem(s);
	}
	memset(s->rows, 0, sizeof (char *) * size);
	s->ncols = asize;
	s->rows[s->ncols + 0] = "";
	s->rows[s->ncols + 1] = "";
	s->rows[s->ncols + 2] = "";
	s->rows[s->ncols + 3] = "TABLE";
	s->rows[s->ncols + 5] = "";
	s->rows[s->ncols + 6] = "";
	s->rows[s->ncols + 7] = "";
	s->rows[s->ncols + 8] = "VIEW";
#ifdef MEMORY_DEBUG
	s->rowfree = xfree__;
#else
	s->rowfree = free;
#endif
	s->nrows = 2;
	s->rowp = -1;
	return SQL_SUCCESS;
    }
    if (cat && (catLen > 0 || catLen == SQL_NTS) && cat[0] == '%') {
	table = NULL;
	goto doit;
    }
    if (schema && (schemaLen > 0 || schemaLen == SQL_NTS) &&
	schema[0] == '%') {
	if ((!cat || catLen == 0 || !cat[0]) &&
	    (!table || tableLen == 0 || !table[0])) {
	    table = NULL;
	    goto doit;
	}
    }
    if (type && (typeLen > 0 || typeLen == SQL_NTS) && type[0] != '\0') {
	char tmp[256], *t;
	int with_view = 0, with_table = 0;

	if (typeLen == SQL_NTS) {
	    strncpy(tmp, (char *) type, sizeof (tmp));
	    tmp[sizeof (tmp) - 1] = '\0';
	} else {
	    int len = min(sizeof (tmp) - 1, typeLen);

	    strncpy(tmp, (char *) type, len);
	    tmp[len] = '\0';
	}
	t = tmp;
	while (*t) {
	    *t = TOLOWER(*t);
	    t++;
	}
	t = tmp;
	unescpat(t);
	while (t) {
	    if (t[0] == '\'') {
		++t;
	    }
	    if (strncmp(t, "table", 5) == 0) {
		with_table++;
	    } else if (strncmp(t, "view", 4) == 0) {
		with_view++;
	    }
	    t = strchr(t, ',');
	    if (t) {
		++t;
	    }
	}
	if (with_view && with_table) {
	    /* where is already preset */
	} else if (with_view && !with_table) {
	    where = "type = 'view'";
	} else if (!with_view && with_table) {
	    where = "type = 'table'";
	} else {
	    return SQL_SUCCESS;
	}
    }
doit:
    if (!table) {
	size = 1;
	tname[0] = '%';
    } else {
	if (tableLen == SQL_NTS) {
	    size = sizeof (tname) - 1;
	} else {
	    size = min(sizeof (tname) - 1, tableLen);
	}
	strncpy(tname, (char *) table, size);
    }
    tname[size] = '\0';
    npatt = unescpat(tname);
    ret = starttran(s);
    if (ret != SQL_SUCCESS) {
	return ret;
    }
#if defined(_WIN32) || defined(_WIN64)
    rc = sqlite_get_table_printf(d->sqlite,
				 "select %s as 'TABLE_QUALIFIER', "
				 "%s as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "upper(type) as 'TABLE_TYPE', "
				 "NULL as 'REMARKS' "
				 "from sqlite_master where %s "
				 "and tbl_name %s '%q'",
				 &s->rows, &s->nrows, &ncols, &errp,
				 d->xcelqrx ? "''" : "NULL",
				 d->xcelqrx ? "'main'" : "NULL",
				 where, npatt ? "like" : "=", tname);
#else
    rc = sqlite_get_table_printf(d->sqlite,
				 "select NULL as 'TABLE_QUALIFIER', "
				 "NULL as 'TABLE_OWNER', "
				 "tbl_name as 'TABLE_NAME', "
				 "upper(type) as 'TABLE_TYPE', "
				 "NULL as 'REMARKS' "
				 "from sqlite_master where %s "
				 "and tbl_name %s '%q'",
				 &s->rows, &s->nrows, &ncols, &errp,
				 where, npatt ? "like" : "=", tname);
#endif
    if (rc == SQLITE_OK) {
	if (ncols != s->ncols) {
	    freeresult(s, 0);
	    s->nrows = 0;
	} else {
	    s->rowfree = sqlite_free_table;
	}
    } else {
	s->nrows = 0;
	s->rows = NULL;
	s->rowfree = NULL;
    }
    if (errp) {
	sqlite_freemem(errp);
	errp = NULL;
    }
    s->rowp = -1;
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Retrieve information on tables and/or views.
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param type types of tables string or NULL
 * @param typeLen length of types of tables string or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLTables(SQLHSTMT stmt,
	  SQLCHAR *cat, SQLSMALLINT catLen,
	  SQLCHAR *schema, SQLSMALLINT schemaLen,
	  SQLCHAR *table, SQLSMALLINT tableLen,
	  SQLCHAR *type, SQLSMALLINT typeLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvtables(stmt, cat, catLen, schema, schemaLen,
		    table, tableLen, type, typeLen);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Retrieve information on tables and/or views.
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param type types of tables string or NULL
 * @param typeLen length of types of tables string or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLTablesW(SQLHSTMT stmt,
	   SQLWCHAR *cat, SQLSMALLINT catLen,
	   SQLWCHAR *schema, SQLSMALLINT schemaLen,
	   SQLWCHAR *table, SQLSMALLINT tableLen,
	   SQLWCHAR *type, SQLSMALLINT typeLen)
{
    char *c = NULL, *s = NULL, *t = NULL, *y = NULL;
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    if (cat) {
	c = uc_to_utf_c(cat, catLen);
	if (!c) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (schema) {
	s = uc_to_utf_c(schema, schemaLen);
	if (!s) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (table) {
	t = uc_to_utf_c(table, tableLen);
	if (!t) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (type) {
	y = uc_to_utf_c(type, typeLen);
	if (!y) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    ret = drvtables(stmt, (SQLCHAR *) c, SQL_NTS, (SQLCHAR *) s, SQL_NTS,
		    (SQLCHAR *) t, SQL_NTS, (SQLCHAR *) y, SQL_NTS);
done:
    HSTMT_UNLOCK(stmt);
    uc_free(y);
    uc_free(t);
    uc_free(s);
    uc_free(c);
    return ret;
}
#endif

/**
 * Columns for result set of SQLColumns().
 */

static COL colSpec2[] = {
    { "SYSTEM", "COLUMN", "TABLE_QUALIFIER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "TABLE_OWNER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLUMN", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLUMN", "DATA_TYPE", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "TYPE_NAME", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "PRECISION", SQL_INTEGER, 50 },
    { "SYSTEM", "COLUMN", "LENGTH", SQL_INTEGER, 50 },
    { "SYSTEM", "COLUMN", "SCALE", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "RADIX", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "NULLABLE", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "REMARKS", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "COLUMN_DEF", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "SQL_DATA_TYPE", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "SQL_DATETIME_SUB", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "CHAR_OCTET_LENGTH", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "ORDINAL_POSITION", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "IS_NULLABLE", SCOL_VARCHAR, 50 }
};

static COL colSpec3[] = {
    { "SYSTEM", "COLUMN", "TABLE_CAT", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "TABLE_SCHEM", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLUMN", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "COLUMN", "DATA_TYPE", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "TYPE_NAME", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "COLUMN_SIZE", SQL_INTEGER, 50 },
    { "SYSTEM", "COLUMN", "BUFFER_LENGTH", SQL_INTEGER, 50 },
    { "SYSTEM", "COLUMN", "DECIMAL_DIGITS", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "NUM_PREC_RADIX", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "NULLABLE", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "REMARKS", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "COLUMN_DEF", SCOL_VARCHAR, 50 },
    { "SYSTEM", "COLUMN", "SQL_DATA_TYPE", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "SQL_DATETIME_SUB", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "CHAR_OCTET_LENGTH", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "ORDINAL_POSITION", SQL_SMALLINT, 50 },
    { "SYSTEM", "COLUMN", "IS_NULLABLE", SCOL_VARCHAR, 50 }
};

/**
 * Internal retrieve column information on table.
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param col column name/pattern or NULL
 * @param colLen length of column name/pattern or SQL_NTS
 * @result ODBC error code
 */

static SQLRETURN
drvcolumns(SQLHSTMT stmt,
	   SQLCHAR *cat, SQLSMALLINT catLen,
	   SQLCHAR *schema, SQLSMALLINT schemaLen,
	   SQLCHAR *table, SQLSMALLINT tableLen,
	   SQLCHAR *col, SQLSMALLINT colLen)
{
    SQLRETURN sret;
    STMT *s;
    DBC *d;
    int ret, nrows, ncols, asize, i, k, roffs, namec;
    int tnrows, tncols, npatt;
    PTRDIFF_T size;
    char *errp = NULL, tname[512], cname[512], **rowp, **trows;

    sret = mkresultset(stmt, colSpec2, array_size(colSpec2),
		       colSpec3, array_size(colSpec3), &asize);
    if (sret != SQL_SUCCESS) {
	return sret;
    }
    s = (STMT *) stmt;
    d = (DBC *) s->dbc;
    if (!table) {
	size = 1;
	tname[0] = '%';
    } else {
	if (tableLen == SQL_NTS) {
	    size = sizeof (tname) - 1;
	} else {
	    size = min(sizeof (tname) - 1, tableLen);
	}
	strncpy(tname, (char *) table, size);
    }
    tname[size] = '\0';
    npatt = unescpat(tname);
    size = 0;
    if (col) {
	if (colLen == SQL_NTS) {
	    size = sizeof (cname) - 1;
	} else {
	    size = min(sizeof (cname) - 1, colLen);
	}
	strncpy(cname, (char *) col, size);
    }
    cname[size] = '\0';
    if (!strcmp(cname, "%")) {
	cname[0] = '\0';
    }
    sret = starttran(s);
    if (sret != SQL_SUCCESS) {
	return sret;
    }
    ret = sqlite_get_table_printf(d->sqlite,
				  "select tbl_name from sqlite_master where "
				  "(type = 'table' or type = 'view') "
				  "and tbl_name %s '%q'",
				  &trows, &tnrows, &tncols, &errp,
				  npatt ? "like" : "=", tname);
    if (ret != SQLITE_OK) {
	setstat(s, ret, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		errp ? errp : "unknown error", ret);
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	return SQL_ERROR;	
    }
    /* pass 1; compute number of rows of result set */
    if (tncols * tnrows <= 0) {
	sqlite_free_table(trows);
	return SQL_SUCCESS;
    }
    size = 0;
    for (i = 1; i <= tnrows; i++) {
	ret = sqlite_get_table_printf(d->sqlite, "PRAGMA table_info('%q')",
				      &rowp, &nrows, &ncols, &errp, trows[i]);
	if (ret != SQLITE_OK) {
	    setstat(s, ret, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		    errp ? errp : "unknown error", ret);
	    if (errp) {
		sqlite_freemem(errp);
		errp = NULL;
	    }
	    return SQL_ERROR;	
	}
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	if (ncols * nrows > 0) {
	    namec = -1;
	    for (k = 0; k < ncols; k++) {
		if (strcmp(rowp[k], "name") == 0) {
		    namec = k;
		    break;
		}
	    }
	    if (cname[0]) {
		for (k = 1; k <= nrows; k++) {
		    if (namematch(rowp[k * ncols + namec], cname, 1)) {
			size++;
		    }
		}
	    } else {
		size += nrows;
	    }
	}
	sqlite_free_table(rowp);
    }
    /* pass 2: fill result set */
    if (size <= 0) {
	sqlite_free_table(trows);
	return SQL_SUCCESS;
    }
    s->nrows = size;
    size = (size + 1) * asize;
    s->rows = xmalloc((size + 1) * sizeof (char *));
    if (!s->rows) {
	s->nrows = 0;
	sqlite_free_table(trows);
	return nomem(s);
    }
    s->rows[0] = (char *) size;
    s->rows += 1;
    memset(s->rows, 0, sizeof (char *) * size);
    s->rowfree = freerows;
    roffs = 1;
    for (i = 1; i <= tnrows; i++) {
	ret = sqlite_get_table_printf(d->sqlite, "PRAGMA table_info('%q')",
				      &rowp, &nrows, &ncols, &errp, trows[i]);
	if (ret != SQLITE_OK) {
	    setstat(s, ret, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		    errp ? errp : "unknown error", ret);
	    if (errp) {
		sqlite_freemem(errp);
		errp = NULL;
	    }
	    sqlite_free_table(trows);
	    return SQL_ERROR;	
	}
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	if (ncols * nrows > 0) {
	    int m, mr, nr = nrows;

	    namec = -1;
	    for (k = 0; k < ncols; k++) {
		if (strcmp(rowp[k], "name") == 0) {
		    namec = k;
		    break;
		}
	    }
	    if (cname[0]) {
		nr = 0;
		for (k = 1; k <= nrows; k++) {
		    if (namematch(rowp[k * ncols + namec], cname, 1)) {
			nr++;
		    }
		}
	    }
	    for (k = 0; k < nr; k++) {
		m = asize * (roffs + k);
		s->rows[m + 0] = xstrdup("");
#if defined(_WIN32) || defined(_WIN64)
		s->rows[m + 1] = xstrdup(d->xcelqrx ? "main" : "");
#else
		s->rows[m + 1] = xstrdup("");
#endif
		s->rows[m + 2] = xstrdup(trows[i]);
		s->rows[m + 8] = xstrdup("10");
		s->rows[m + 9] = xstrdup("0");
		s->rows[m + 15] = xstrdup("16384");
	    }
	    for (k = 0; nr && k < ncols; k++) {
		if (strcmp(rowp[k], "cid") == 0) {
		    for (mr = 0, m = 1; m <= nrows; m++) {
			char buf[256];
			int ir, coln = i;

			if (cname[0] &&
			    !namematch(rowp[m * ncols + namec], cname, 1)) {
			    continue;
			}
			ir = asize * (roffs + mr);
			sscanf(rowp[m * ncols + k], "%d", &coln);
			sprintf(buf, "%d", coln + 1);
			s->rows[ir + 16] = xstrdup(buf);
			++mr;
		    }
		} else if (k == namec) {
		    for (mr = 0, m = 1; m <= nrows; m++) {
			int ir;

			if (cname[0] &&
			    !namematch(rowp[m * ncols + namec], cname, 1)) {
			    continue;
			}
			ir = asize * (roffs + mr);
			s->rows[ir + 3] = xstrdup(rowp[m * ncols + k]);
			++mr;
		    }
		} else if (strcmp(rowp[k], "notnull") == 0) {
		    for (mr = 0, m = 1; m <= nrows; m++) {
			int ir;

			if (cname[0] &&
			    !namematch(rowp[m * ncols + namec], cname, 1)) {
			    continue;
			}
			ir = asize * (roffs + mr);
			if (*rowp[m * ncols + k] != '0') {
			    s->rows[ir + 10] = xstrdup(stringify(SQL_FALSE));
			} else {
			    s->rows[ir + 10] = xstrdup(stringify(SQL_TRUE));
			}
			s->rows[ir + 17] =
			    xstrdup((*rowp[m * ncols + k] != '0') ?
				    "NO" : "YES");
			++mr;
		    }
		} else if (strcmp(rowp[k], "dflt_value") == 0) {
		    for (mr = 0, m = 1; m <= nrows; m++) {
			char *dflt = rowp[m * ncols + k];
			int ir;

			if (cname[0] &&
			    !namematch(rowp[m * ncols + namec], cname, 1)) {
			    continue;
			}
			ir = asize * (roffs + mr);
			s->rows[ir + 12] = xstrdup(dflt ? dflt : "NULL");
			++mr;
		    }
		} else if (strcmp(rowp[k], "type") == 0) {
		    for (mr = 0, m = 1; m <= nrows; m++) {
			char *typename = rowp[m * ncols + k];
			int sqltype, mm, dd, ir;
			char buf[256];

			if (cname[0] &&
			    !namematch(rowp[m * ncols + namec], cname, 1)) {
			    continue;
			}
			ir = asize * (roffs + mr);
			s->rows[ir + 5] = xstrdup(typename);
			sqltype = mapsqltype(typename, NULL, *s->ov3,
					     s->nowchar[0]);
			getmd(typename, sqltype, &mm, &dd);
#ifdef SQL_LONGVARCHAR
			if (sqltype == SQL_VARCHAR && mm > 255) {
			    sqltype = SQL_LONGVARCHAR;
			}
#endif
#ifdef WINTERFACE
#ifdef SQL_WLONGVARCHAR
			if (sqltype == SQL_WVARCHAR && mm > 255) {
			    sqltype = SQL_WLONGVARCHAR;
			}
#endif
#endif
#if (HAVE_ENCDEC)
			if (sqltype == SQL_VARBINARY && mm > 255) {
			    sqltype = SQL_LONGVARBINARY;
			}
#endif
			sprintf(buf, "%d", sqltype);
			s->rows[ir + 4] = xstrdup(buf);
			s->rows[ir + 13] = xstrdup(buf);
			sprintf(buf, "%d", mm);
			s->rows[ir + 7] = xstrdup(buf);
			sprintf(buf, "%d", dd);
			s->rows[ir + 6] = xstrdup(buf);
			++mr;
		    }
		}
	    }
	    roffs += nr;
	}
	sqlite_free_table(rowp);
    }
    sqlite_free_table(trows);
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Retrieve column information on table.
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param col column name/pattern or NULL
 * @param colLen length of column name/pattern or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLColumns(SQLHSTMT stmt,
	   SQLCHAR *cat, SQLSMALLINT catLen,
	   SQLCHAR *schema, SQLSMALLINT schemaLen,
	   SQLCHAR *table, SQLSMALLINT tableLen,
	   SQLCHAR *col, SQLSMALLINT colLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvcolumns(stmt, cat, catLen, schema, schemaLen,
		     table, tableLen, col, colLen);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Retrieve column information on table (UNICODE version).
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param col column name/pattern or NULL
 * @param colLen length of column name/pattern or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLColumnsW(SQLHSTMT stmt,
	    SQLWCHAR *cat, SQLSMALLINT catLen,
	    SQLWCHAR *schema, SQLSMALLINT schemaLen,
	    SQLWCHAR *table, SQLSMALLINT tableLen,
	    SQLWCHAR *col, SQLSMALLINT colLen)
{
    char *c = NULL, *s = NULL, *t = NULL, *k = NULL;
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    if (cat) {
	c = uc_to_utf_c(cat, catLen);
	if (!c) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (schema) {
	s = uc_to_utf_c(schema, schemaLen);
	if (!s) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (table) {
	t = uc_to_utf_c(table, tableLen);
	if (!t) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (col) {
	k = uc_to_utf_c(col, colLen);
	if (!k) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    ret = drvcolumns(stmt, (SQLCHAR *) c, SQL_NTS, (SQLCHAR *) s, SQL_NTS,
		     (SQLCHAR *) t, SQL_NTS, (SQLCHAR *) k, SQL_NTS);
done:
    HSTMT_UNLOCK(stmt);
    uc_free(k);
    uc_free(t);
    uc_free(s);
    uc_free(c);
    return ret;

}
#endif

/**
 * Columns for result set of SQLGetTypeInfo().
 */

static COL typeSpec2[] = {
    { "SYSTEM", "TYPE", "TYPE_NAME", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TYPE", "DATA_TYPE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "PRECISION", SQL_INTEGER, 9 },
    { "SYSTEM", "TYPE", "LITERAL_PREFIX", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TYPE", "LITERAL_SUFFIX", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TYPE", "CREATE_PARAMS", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TYPE", "NULLABLE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "CASE_SENSITIVE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "SEARCHABLE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "UNSIGNED_ATTRIBUTE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "MONEY", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "AUTO_INCREMENT", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "LOCAL_TYPE_NAME", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TYPE", "MINIMUM_SCALE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "MAXIMUM_SCALE", SQL_SMALLINT, 2 }
};

static COL typeSpec3[] = {
    { "SYSTEM", "TYPE", "TYPE_NAME", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TYPE", "DATA_TYPE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "COLUMN_SIZE", SQL_INTEGER, 9 },
    { "SYSTEM", "TYPE", "LITERAL_PREFIX", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TYPE", "LITERAL_SUFFIX", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TYPE", "CREATE_PARAMS", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TYPE", "NULLABLE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "CASE_SENSITIVE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "SEARCHABLE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "UNSIGNED_ATTRIBUTE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "FIXED_PREC_SCALE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "AUTO_UNIQUE_VALUE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "LOCAL_TYPE_NAME", SCOL_VARCHAR, 50 },
    { "SYSTEM", "TYPE", "MINIMUM_SCALE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "MAXIMUM_SCALE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "SQL_DATA_TYPE", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "SQL_DATETIME_SUB", SQL_SMALLINT, 2 },
    { "SYSTEM", "TYPE", "NUM_PREC_RADIX", SQL_INTEGER, 4 },
    { "SYSTEM", "TYPE", "INTERVAL_PRECISION", SQL_SMALLINT, 2 }
};

/**
 * Internal function to build up data type information as row in result set.
 * @param s statement pointer
 * @param row row number
 * @param asize number of items in a row
 * @param typename name of type
 * @param type integer SQL type
 * @param tind type index
 */

static void
mktypeinfo(STMT *s, int row, int asize, char *typename, int type, int tind)
{
    int offs = row * asize;
    char *tcode, *crpar = NULL, *quote = NULL, *sign = stringify(SQL_FALSE);
    static char tcodes[32 * 32];

    if (tind <= 0) {
	tind = row;
    }
    tcode = tcodes + tind * 32;
    sprintf(tcode, "%d", type);
    s->rows[offs + 0] = typename;
    s->rows[offs + 1] = tcode;
    if (asize >= 17) {
	s->rows[offs + 15] = tcode;
	s->rows[offs + 16] = "0";
    }
    switch (type) {
    default:
#ifdef SQL_LONGVARCHAR
    case SQL_LONGVARCHAR:
#ifdef WINTERFACE
    case SQL_WLONGVARCHAR:
#endif
	crpar = "length";
	quote = "'";
	sign = NULL;
	s->rows[offs + 2] = "65536";
	break;
#endif
    case SQL_CHAR:
    case SQL_VARCHAR:
#ifdef WINTERFACE
    case SQL_WCHAR:
    case SQL_WVARCHAR:
#endif
	s->rows[offs + 2] = "255";
	crpar = "length";
	quote = "'";
	sign = NULL;
	break;
    case SQL_TINYINT:
	s->rows[offs + 2] = "3";
	break;
    case SQL_SMALLINT:
	s->rows[offs + 2] = "5";
	break;
    case SQL_INTEGER:
	s->rows[offs + 2] = "9";
	break;
    case SQL_FLOAT:
	s->rows[offs + 2] = "7";
	break;
    case SQL_DOUBLE:
	s->rows[offs + 2] = "15";
	break;
#ifdef SQL_TYPE_DATE
    case SQL_TYPE_DATE:
#endif
    case SQL_DATE:
	s->rows[offs + 2] = "10";
	quote = "'";
	sign = NULL;
	break;
#ifdef SQL_TYPE_TIME
    case SQL_TYPE_TIME:
#endif
    case SQL_TIME:
	s->rows[offs + 2] = "8";
	quote = "'";
	sign = NULL;
	break;
#ifdef SQL_TYPE_TIMESTAMP
    case SQL_TYPE_TIMESTAMP:
#endif
    case SQL_TIMESTAMP:
	s->rows[offs + 2] = "32";
	quote = "'";
	sign = NULL;
	break;
#if (HAVE_ENCDEC)
    case SQL_VARBINARY:
	sign = NULL;
	s->rows[offs + 2] = "255";
	break;
    case SQL_LONGVARBINARY:
	sign = NULL;
	s->rows[offs + 2] = "65536";
	break;
#endif
#ifdef SQL_BIT
    case SQL_BIT:
	sign = NULL;
	s->rows[offs + 2] = "1";
	break;
#endif
    }
    s->rows[offs + 3] = s->rows[offs + 4] = quote;
    s->rows[offs + 5] = crpar;
    s->rows[offs + 6] = stringify(SQL_NULLABLE);
    s->rows[offs + 7] = stringify(SQL_FALSE);
    s->rows[offs + 8] = stringify(SQL_SEARCHABLE);
    s->rows[offs + 9] = sign;
    s->rows[offs + 10] = stringify(SQL_FALSE);
    s->rows[offs + 11] = stringify(SQL_FALSE);
    s->rows[offs + 12] = typename;
    switch (type) {
    case SQL_DATE:
    case SQL_TIME:
	s->rows[offs + 13] = "0";
	s->rows[offs + 14] = "0";
	break;
#ifdef SQL_TYPE_TIMESTAMP
    case SQL_TYPE_TIMESTAMP:
#endif
    case SQL_TIMESTAMP:
	s->rows[offs + 13] = "0";
	s->rows[offs + 14] = "3";
	break;
    default:
	s->rows[offs + 13] = NULL;
	s->rows[offs + 14] = NULL;
	break;
    }
}

/**
 * Helper function to sort type information.
 * Callback for qsort().
 * @param a first item to compare
 * @param b second item to compare
 * @result ==0, <0, >0 according to data type number
 */

static int
typeinfosort(const void *a, const void *b)
{
    char **pa = (char **) a;
    char **pb = (char **) b;
    int na, nb;

    na = strtol(pa[1], NULL, 0);
    nb = strtol(pb[1], NULL, 0);
    return na - nb;
}

/**
 * Internal return data type information.
 * @param stmt statement handle
 * @param sqltype which type to retrieve
 * @result ODBC error code
 */

static SQLRETURN
drvgettypeinfo(SQLHSTMT stmt, SQLSMALLINT sqltype)
{
    SQLRETURN ret;
    STMT *s;
    int asize;

    ret = mkresultset(stmt, typeSpec2, array_size(typeSpec2),
		      typeSpec3, array_size(typeSpec3), &asize);
    if (ret != SQL_SUCCESS) {
	return ret;
    }
    s = (STMT *) stmt;
#ifdef WINTERFACE
#ifdef SQL_LONGVARCHAR
#ifdef SQL_WLONGVARCHAR
    if (s->nowchar[0]) {
	s->nrows = (sqltype == SQL_ALL_TYPES) ? 13 : 1;
    } else {
	s->nrows = (sqltype == SQL_ALL_TYPES) ? 17 : 1;
    }
#else
    if (s->nowchar[0]) {
	s->nrows = (sqltype == SQL_ALL_TYPES) ? 12 : 1;
    } else {
	s->nrows = (sqltype == SQL_ALL_TYPES) ? 16 : 1;
    }
#endif
#else
    s->nrows = (sqltype == SQL_ALL_TYPES) ? 15 : 1;
#endif
#else
#ifdef SQL_LONGVARCHAR
    s->nrows = (sqltype == SQL_ALL_TYPES) ? 13 : 1;
#else
    s->nrows = (sqltype == SQL_ALL_TYPES) ? 12 : 1;
#endif
#endif
#if (HAVE_ENCDEC)
    if (sqltype == SQL_ALL_TYPES) {
	s->nrows += 2;
    }
#endif
#ifdef SQL_BIT
    if (sqltype == SQL_ALL_TYPES) {
	s->nrows += 1;
    }
#endif
    s->rows = (char **) xmalloc(sizeof (char *) * (s->nrows + 1) * asize);
    if (!s->rows) {
	s->nrows = 0;
	return nomem(s);
    }
#ifdef MEMORY_DEBUG
    s->rowfree = xfree__;
#else
    s->rowfree = free;
#endif
    memset(s->rows, 0, sizeof (char *) * (s->nrows + 1) * asize);
    if (sqltype == SQL_ALL_TYPES) {
	int cc = 1;

	mktypeinfo(s, cc++, asize, "varchar", SQL_VARCHAR, 0);
	mktypeinfo(s, cc++, asize, "tinyint", SQL_TINYINT, 0);
	mktypeinfo(s, cc++, asize, "smallint", SQL_SMALLINT, 0);
	mktypeinfo(s, cc++, asize, "integer", SQL_INTEGER, 0);
	mktypeinfo(s, cc++, asize, "float", SQL_FLOAT, 0);
	mktypeinfo(s, cc++, asize, "double", SQL_DOUBLE, 0);
#ifdef SQL_TYPE_DATE
	mktypeinfo(s, cc++, asize, "date",
		   (*s->ov3) ? SQL_TYPE_DATE : SQL_DATE, 0);
#else
	mktypeinfo(s, cc++, asize, "date", SQL_DATE, 0);
#endif
#ifdef SQL_TYPE_TIME
	mktypeinfo(s, cc++, asize, "time",
		   (*s->ov3) ? SQL_TYPE_TIME : SQL_TIME, 0);
#else
	mktypeinfo(s, cc++, asize, "time", SQL_TIME, 0);
#endif
#ifdef SQL_TYPE_TIMESTAMP
	mktypeinfo(s, cc++, asize, "timestamp",
		   (*s->ov3) ? SQL_TYPE_TIMESTAMP : SQL_TIMESTAMP, 0);
#else
	mktypeinfo(s, cc++, asize, "timestamp", SQL_TIMESTAMP, 0);
#endif
	mktypeinfo(s, cc++, asize, "char", SQL_CHAR, 0);
	mktypeinfo(s, cc++, asize, "numeric", SQL_DOUBLE, 0);
#ifdef SQL_LONGVARCHAR
	mktypeinfo(s, cc++, asize, "text", SQL_LONGVARCHAR, 0);
	mktypeinfo(s, cc++, asize, "longvarchar", SQL_LONGVARCHAR, 0);
#else
	mktypeinfo(s, cc++, asize, "text", SQL_VARCHAR, 0);
#endif
#ifdef WINTERFACE
	if (!s->nowchar[0]) {
	    mktypeinfo(s, cc++, asize, "wvarchar", SQL_WVARCHAR, 0);
#ifdef SQL_LONGVARCHAR
	    mktypeinfo(s, cc++, asize, "wchar", SQL_WCHAR, 0);
	    mktypeinfo(s, cc++, asize, "wtext", SQL_WLONGVARCHAR, 0);
#ifdef SQL_WLONGVARCHAR
	    mktypeinfo(s, cc++, asize, "longwvarchar", SQL_WLONGVARCHAR, 0);
#endif
#else
	    mktypeinfo(s, cc++, asize, "wvarchar", SQL_WVARCHAR, 0);
	    mktypeinfo(s, cc++, asize, "wchar", SQL_WCHAR, 0);
	    mktypeinfo(s, cc++, asize, "wtext", SQL_WVARCHAR, 0);
#endif
	}
#endif
#if (HAVE_ENCDEC)
	mktypeinfo(s, cc++, asize, "varbinary", SQL_VARBINARY, 0);
	mktypeinfo(s, cc++, asize, "longvarbinary", SQL_LONGVARBINARY, 0);
#endif
#ifdef SQL_BIT
	mktypeinfo(s, cc++, asize, "bit", SQL_BIT, 0);
#endif
	qsort(s->rows + asize, s->nrows, sizeof (char *) * asize,
	      typeinfosort);
    } else {
	switch (sqltype) {
	case SQL_CHAR:
	    mktypeinfo(s, 1, asize, "char", SQL_CHAR, 10);
	    break;
	case SQL_VARCHAR:
	    mktypeinfo(s, 1, asize, "varchar", SQL_VARCHAR, 1);
	    break;
	case SQL_TINYINT:
	    mktypeinfo(s, 1, asize, "tinyint", SQL_TINYINT, 2);
	    break;
	case SQL_SMALLINT:
	    mktypeinfo(s, 1, asize, "smallint", SQL_SMALLINT, 3);
	    break;
	case SQL_INTEGER:
	    mktypeinfo(s, 1, asize, "integer", SQL_INTEGER, 4);
	    break;
	case SQL_FLOAT:
	    mktypeinfo(s, 1, asize, "float", SQL_FLOAT, 5);
	    break;
	case SQL_DOUBLE:
	    mktypeinfo(s, 1, asize, "double", SQL_DOUBLE, 6);
	    break;
#ifdef SQL_TYPE_DATE
	case SQL_TYPE_DATE:
	    mktypeinfo(s, 1, asize, "date", SQL_TYPE_DATE, 25);
	    break;
#endif
	case SQL_DATE:
	    mktypeinfo(s, 1, asize, "date", SQL_DATE, 7);
	    break;
#ifdef SQL_TYPE_TIME
	case SQL_TYPE_TIME:
	    mktypeinfo(s, 1, asize, "time", SQL_TYPE_TIME, 26);
	    break;
#endif
	case SQL_TIME:
	    mktypeinfo(s, 1, asize, "time", SQL_TIME, 8);
	    break;
#ifdef SQL_TYPE_TIMESTAMP
	case SQL_TYPE_TIMESTAMP:
	    mktypeinfo(s, 1, asize, "timestamp", SQL_TYPE_TIMESTAMP, 27);
	    break;
#endif
	case SQL_TIMESTAMP:
	    mktypeinfo(s, 1, asize, "timestamp", SQL_TIMESTAMP, 9);
	    break;
#ifdef SQL_LONGVARCHAR
	case SQL_LONGVARCHAR:
	    mktypeinfo(s, 1, asize, "longvarchar", SQL_LONGVARCHAR, 12);
	    break;
#endif
#ifdef WINTERFACE
#ifdef SQL_WCHAR
	case SQL_WCHAR:
	    mktypeinfo(s, 1, asize, "wchar", SQL_WCHAR, 18);
	    break;
#endif
#ifdef SQL_WVARCHAR
	case SQL_WVARCHAR:
	    mktypeinfo(s, 1, asize, "wvarchar", SQL_WVARCHAR, 19);
	    break;
#endif
#ifdef SQL_WLONGVARCHAR
	case SQL_WLONGVARCHAR:
	    mktypeinfo(s, 1, asize, "longwvarchar", SQL_WLONGVARCHAR, 20);
	    break;
#endif
#endif
#if (HAVE_ENCDEC)
	case SQL_VARBINARY:
	    mktypeinfo(s, 1, asize, "varbinary", SQL_VARBINARY, 30);
	    break;
	case SQL_LONGVARBINARY:
	    mktypeinfo(s, 1, asize, "longvarbinary", SQL_LONGVARBINARY, 31);
	    break;
#endif
#ifdef SQL_BIT
	case SQL_BIT:
	    mktypeinfo(s, 1, asize, "bit", SQL_BIT, 29);
	    break;
#endif
	default:
	    s->nrows = 0;
	}
    }
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Return data type information.
 * @param stmt statement handle
 * @param sqltype which type to retrieve
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetTypeInfo(SQLHSTMT stmt, SQLSMALLINT sqltype)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvgettypeinfo(stmt, sqltype);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Return data type information (UNICODE version).
 * @param stmt statement handle
 * @param sqltype which type to retrieve
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetTypeInfoW(SQLHSTMT stmt, SQLSMALLINT sqltype)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvgettypeinfo(stmt, sqltype);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Columns for result set of SQLStatistics().
 */

static COL statSpec2[] = {
    { "SYSTEM", "STATISTICS", "TABLE_QUALIFIER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "STATISTICS", "TABLE_OWNER", SCOL_VARCHAR, 50 },
    { "SYSTEM", "STATISTICS", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "STATISTICS", "NON_UNIQUE", SQL_SMALLINT, 50 },
    { "SYSTEM", "STATISTICS", "INDEX_QUALIFIER", SCOL_VARCHAR, 255 },
    { "SYSTEM", "STATISTICS", "INDEX_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "STATISTICS", "TYPE", SQL_SMALLINT, 50 },
    { "SYSTEM", "STATISTICS", "SEQ_IN_INDEX", SQL_SMALLINT, 50 },
    { "SYSTEM", "STATISTICS", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "STATISTICS", "COLLATION", SCOL_CHAR, 1 },
    { "SYSTEM", "STATISTICS", "CARDINALITY", SQL_INTEGER, 50 },
    { "SYSTEM", "STATISTICS", "PAGES", SQL_INTEGER, 50 },
    { "SYSTEM", "STATISTICS", "FILTER_CONDITION", SCOL_VARCHAR, 255 }
};

static COL statSpec3[] = {
    { "SYSTEM", "STATISTICS", "TABLE_CAT", SCOL_VARCHAR, 50 },
    { "SYSTEM", "STATISTICS", "TABLE_SCHEM", SCOL_VARCHAR, 50 },
    { "SYSTEM", "STATISTICS", "TABLE_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "STATISTICS", "NON_UNIQUE", SQL_SMALLINT, 50 },
    { "SYSTEM", "STATISTICS", "INDEX_QUALIFIER", SCOL_VARCHAR, 255 },
    { "SYSTEM", "STATISTICS", "INDEX_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "STATISTICS", "TYPE", SQL_SMALLINT, 50 },
    { "SYSTEM", "STATISTICS", "ORDINAL_POSITION", SQL_SMALLINT, 50 },
    { "SYSTEM", "STATISTICS", "COLUMN_NAME", SCOL_VARCHAR, 255 },
    { "SYSTEM", "STATISTICS", "ASC_OR_DESC", SCOL_CHAR, 1 },
    { "SYSTEM", "STATISTICS", "CARDINALITY", SQL_INTEGER, 50 },
    { "SYSTEM", "STATISTICS", "PAGES", SQL_INTEGER, 50 },
    { "SYSTEM", "STATISTICS", "FILTER_CONDITION", SCOL_VARCHAR, 255 }
};

/**
 * Internal return statistic information on table indices.
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param itype type of index information
 * @param resv reserved
 * @result ODBC error code
 */

static SQLRETURN
drvstatistics(SQLHSTMT stmt, SQLCHAR *cat, SQLSMALLINT catLen,
	      SQLCHAR *schema, SQLSMALLINT schemaLen,
	      SQLCHAR *table, SQLSMALLINT tableLen,
	      SQLUSMALLINT itype, SQLUSMALLINT resv)
{
    SQLRETURN sret;
    STMT *s;
    DBC *d;
    int i, asize, ret, nrows, ncols, offs, namec, uniquec, addipk = 0;
    PTRDIFF_T size;
    char **rowp, *errp = NULL, tname[512];

    sret = mkresultset(stmt, statSpec2, array_size(statSpec2),
		       statSpec3, array_size(statSpec3), &asize);
    if (sret != SQL_SUCCESS) {
	return sret;
    }
    s = (STMT *) stmt;
    d = (DBC *) s->dbc;
    if (!table || table[0] == '\0' || table[0] == '%') {
	setstat(s, -1, "need table name", (*s->ov3) ? "HY000" : "S1000");
	return SQL_ERROR;
    }
    if (tableLen == SQL_NTS) {
	size = sizeof (tname) - 1;
    } else {
	size = min(sizeof (tname) - 1, tableLen);
    }
    strncpy(tname, (char *) table, size);
    tname[size] = '\0';
    unescpat(tname);
    sret = starttran(s);
    if (sret != SQL_SUCCESS) {
	return sret;
    }
    /*
     * Try integer primary key (autoincrement) first
     */
    if (itype == SQL_INDEX_UNIQUE || itype == SQL_INDEX_ALL) {
	int colid, typec, npk = 0;

	rowp = 0;
	ret = sqlite_get_table_printf(d->sqlite,
				      "PRAGMA table_info('%q')", &rowp,
				      &nrows, &ncols, NULL, tname);
	if (ret != SQLITE_OK) {
	    goto noipk;
	}
	namec = findcol(rowp, ncols, "name");
	uniquec = findcol(rowp, ncols, "pk");
	typec = findcol(rowp, ncols, "type");
	colid = findcol(rowp, ncols, "cid");
	if (namec < 0 || uniquec < 0 || typec < 0 || colid < 0) {
	    goto noipk;
	}
	for (i = 1; i <= nrows; i++) {
	    if (*rowp[i * ncols + uniquec] != '0' &&
		strlen(rowp[i * ncols + typec]) == 7 &&
		strncasecmp(rowp[i * ncols + typec], "integer", 7)
		== 0) {
		npk++;
	    }
	}
	if (npk == 1) {
	    addipk = 1;
	}
noipk:
	sqlite_free_table(rowp);
    }
    ret = sqlite_get_table_printf(d->sqlite,
				  "PRAGMA index_list('%q')", &rowp,
				  &nrows, &ncols, &errp, tname);
    if (ret != SQLITE_OK) {
	setstat(s, ret, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		errp ? errp : "unknown error", ret);
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	return SQL_ERROR;
    }
    if (errp) {
	sqlite_freemem(errp);
	errp = NULL;
    }
    size = 0;
    namec = findcol(rowp, ncols, "name");
    uniquec = findcol(rowp, ncols, "unique");
    if (namec < 0 || uniquec < 0) {
	goto nodata;
    }
    for (i = 1; i <= nrows; i++) {
	int nnrows, nncols;
	char **rowpp;
	int isuniq;

	isuniq = *rowp[i * ncols + uniquec] != '0';
	if (isuniq || itype == SQL_INDEX_ALL) {
	    ret = sqlite_get_table_printf(d->sqlite,
					  "PRAGMA index_info('%q')", &rowpp,
					  &nnrows, &nncols, NULL,
					  rowp[i * ncols + namec]);
	    if (ret == SQLITE_OK) {
		size += nnrows;
		sqlite_free_table(rowpp);
	    }
	}
    }
nodata:
    if (addipk) {
	size++;
    }
    if (size == 0) {
	sqlite_free_table(rowp);
	return SQL_SUCCESS;
    }
    s->nrows = size;
    size = (size + 1) * asize;
    s->rows = xmalloc((size + 1) * sizeof (char *));
    if (!s->rows) {
	s->nrows = 0;
	return nomem(s);
    }
    s->rows[0] = (char *) size;
    s->rows += 1;
    memset(s->rows, 0, sizeof (char *) * size);
    s->rowfree = freerows;
    offs = 0;
    if (addipk) {
	char **rowpp;
	int ncols2, nrows2;

	rowpp = 0;
	ret = sqlite_get_table_printf(d->sqlite,
				      "PRAGMA table_info('%q')", &rowpp,
				      &nrows2, &ncols2, NULL, tname);
	if (ret == SQLITE_OK) {
	    int namecc, uniquecc, colid, typec, roffs;

	    namecc = findcol(rowpp, ncols2, "name");
	    uniquecc = findcol(rowpp, ncols2, "pk");
	    typec = findcol(rowpp, ncols2, "type");
	    colid = findcol(rowpp, ncols2, "cid");
	    if (namecc < 0 || uniquecc < 0 || typec < 0 || colid < 0) {
		addipk = 0;
		s->nrows--;
		goto nodata2;
	    }
	    for (i = 1; i <= nrows2; i++) {
		if (*rowpp[i * ncols2 + uniquecc] != '0' &&
		    strlen(rowpp[i * ncols2 + typec]) == 7 &&
		    strncasecmp(rowpp[i * ncols2 + typec], "integer", 7)
		    == 0) {
		    break;
		}
	    }
	    if (i > nrows) {
		addipk = 0;
		s->nrows--;
		goto nodata2;
	    }
	    roffs = s->ncols;
	    s->rows[roffs + 0] = xstrdup("");
#if defined(_WIN32) || defined(_WIN64)
	    s->rows[roffs + 1] = xstrdup(d->xcelqrx ? "main" : "");
#else
	    s->rows[roffs + 1] = xstrdup("");
#endif
	    s->rows[roffs + 2] = xstrdup(tname);
	    s->rows[roffs + 3] = xstrdup(stringify(SQL_FALSE));
	    s->rows[roffs + 5] = xstrdup("(autoindex 0)");
	    s->rows[roffs + 6] = xstrdup(stringify(SQL_INDEX_OTHER));
	    s->rows[roffs + 7] = xstrdup("1");
	    s->rows[roffs + 8] = xstrdup(rowpp[i * ncols2 + namecc]);
	    s->rows[roffs + 9] = xstrdup("A");
	}
nodata2:
	sqlite_free_table(rowpp);
    }
    for (i = 1; i <= nrows; i++) {
	int nnrows, nncols;
	char **rowpp;

	if (*rowp[i * ncols + uniquec] != '0' || itype == SQL_INDEX_ALL) {
	    int k;

	    ret = sqlite_get_table_printf(d->sqlite,
					  "PRAGMA index_info('%q')", &rowpp,
					  &nnrows, &nncols, NULL,
					  rowp[i * ncols + namec]);
	    if (ret != SQLITE_OK) {
		continue;
	    }
	    for (k = 0; nnrows && k < nncols; k++) {
		if (strcmp(rowpp[k], "name") == 0) {
		    int m;

		    for (m = 1; m <= nnrows; m++) {
			int roffs = (offs + addipk + m) * s->ncols;
			int isuniq;

			isuniq = *rowp[i * ncols + uniquec] != '0';
			s->rows[roffs + 0] = xstrdup("");
			s->rows[roffs + 1] = xstrdup("");
			s->rows[roffs + 2] = xstrdup(tname);
			if (isuniq) {
			    s->rows[roffs + 3] = xstrdup(stringify(SQL_FALSE));
			} else {
			    s->rows[roffs + 3] = xstrdup(stringify(SQL_TRUE));
			}
			s->rows[roffs + 5] = xstrdup(rowp[i * ncols + namec]);
			s->rows[roffs + 6] =
			    xstrdup(stringify(SQL_INDEX_OTHER));
			s->rows[roffs + 8] = xstrdup(rowpp[m * nncols + k]);
			s->rows[roffs + 9] = xstrdup("A");
		    }
		} else if (strcmp(rowpp[k], "seqno") == 0) {
		    int m;

		    for (m = 1; m <= nnrows; m++) {
			int roffs = (offs + addipk + m) * s->ncols;
			int pos = m - 1;
			char buf[32];

			sscanf(rowpp[m * nncols + k], "%d", &pos);
			sprintf(buf, "%d", pos + 1);
			s->rows[roffs + 7] = xstrdup(buf);
		    }
		}
	    }
	    offs += nnrows;
	    sqlite_free_table(rowpp);
	}
    }
    sqlite_free_table(rowp);
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Return statistic information on table indices.
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param itype type of index information
 * @param resv reserved
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLStatistics(SQLHSTMT stmt, SQLCHAR *cat, SQLSMALLINT catLen,
	      SQLCHAR *schema, SQLSMALLINT schemaLen,
	      SQLCHAR *table, SQLSMALLINT tableLen,
	      SQLUSMALLINT itype, SQLUSMALLINT resv)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvstatistics(stmt, cat, catLen, schema, schemaLen,
			table, tableLen, itype, resv);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Return statistic information on table indices (UNICODE version).
 * @param stmt statement handle
 * @param cat catalog name/pattern or NULL
 * @param catLen length of catalog name/pattern or SQL_NTS
 * @param schema schema name/pattern or NULL
 * @param schemaLen length of schema name/pattern or SQL_NTS
 * @param table table name/pattern or NULL
 * @param tableLen length of table name/pattern or SQL_NTS
 * @param itype type of index information
 * @param resv reserved
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLStatisticsW(SQLHSTMT stmt, SQLWCHAR *cat, SQLSMALLINT catLen,
	       SQLWCHAR *schema, SQLSMALLINT schemaLen,
	       SQLWCHAR *table, SQLSMALLINT tableLen,
	       SQLUSMALLINT itype, SQLUSMALLINT resv)
{
    char *c = NULL, *s = NULL, *t = NULL;
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    if (cat) {
	c = uc_to_utf_c(cat, catLen);
	if (!c) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (schema) {
	s = uc_to_utf_c(schema, schemaLen);
	if (!s) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    if (table) {
	t = uc_to_utf_c(table, tableLen);
	if (!t) {
	    ret = nomem((STMT *) stmt);
	    goto done;
	}
    }
    ret = drvstatistics(stmt, (SQLCHAR *) c, SQL_NTS, (SQLCHAR *) s, SQL_NTS,
			(SQLCHAR *) t, SQL_NTS, itype, resv);
done:
    HSTMT_UNLOCK(stmt);
    uc_free(t);
    uc_free(s);
    uc_free(c);
    return ret;
}
#endif

/**
 * Retrieve row data after fetch.
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param type output type
 * @param val output buffer
 * @param len length of output buffer
 * @param lenp output length
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLGetData(SQLHSTMT stmt, SQLUSMALLINT col, SQLSMALLINT type,
	   SQLPOINTER val, SQLLEN len, SQLLEN *lenp)
{
    STMT *s;
    SQLRETURN ret = SQL_ERROR;

    HSTMT_LOCK(stmt);
    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (col == 0 && s->bkmrk && type == SQL_C_BOOKMARK) {
	*((long *) val) = s->rowp;
	if (lenp) {
	    *lenp = sizeof (long);
	}
	ret = SQL_SUCCESS;
	goto done;
    }
    if (col < 1 || col > s->ncols) {
	setstat(s, -1, "invalid column", (*s->ov3) ? "07009" : "S1002");
	goto done;
    }
    --col;
    ret = getrowdata(s, col, type, val, len, lenp, 1);
done:
    HSTMT_UNLOCK(stmt);
    return ret;
}

/**
 * Internal: fetch and bind from statement's current row
 * @param s statement pointer
 * @param rsi rowset index
 * @result ODBC error code
 */

static SQLRETURN
dofetchbind(STMT *s, int rsi)
{
    int ret, i, withinfo = 0;

    s->row_status0[rsi] = SQL_ROW_SUCCESS;
    if (s->bkmrk && s->bkmrkcol.valp) {
	long *val;

	if (s->bind_type != SQL_BIND_BY_COLUMN) {
	    val = (long *) ((char *) s->bkmrkcol.valp + s->bind_type * rsi);
	} else {
	    val = (long *) s->bkmrkcol.valp + rsi;
	}
	if (s->bind_offs) {
	    val = (long *) ((char *) val + *s->bind_offs);
	}
	*val = s->rowp;
	if (s->bkmrkcol.lenp) {
	    SQLLEN *ival;

	    if (s->bind_type != SQL_BIND_BY_COLUMN) {
		ival = (SQLLEN *) ((char *) s->bkmrkcol.lenp +
				   s->bind_type * rsi);
	    } else {
		ival = &s->bkmrkcol.lenp[rsi];
	    }
	    if (s->bind_offs) {
		ival = (SQLLEN *) ((char *) ival + *s->bind_offs);
	    }
	    *ival = sizeof (long);
	}
    }
    ret = SQL_SUCCESS;
    for (i = 0; s->bindcols && i < s->ncols; i++) {
	BINDCOL *b = &s->bindcols[i];
	SQLPOINTER dp = 0;
	SQLLEN *lp = 0;

	b->offs = 0;
	if (b->valp) {
	    if (s->bind_type != SQL_BIND_BY_COLUMN) {
		dp = (SQLPOINTER) ((char *) b->valp + s->bind_type * rsi);
	    } else {
		dp = (SQLPOINTER) ((char *) b->valp + b->max * rsi);
	    }
	    if (s->bind_offs) {
		dp = (SQLPOINTER) ((char *) dp + *s->bind_offs);
	    }
	}
	if (b->lenp) {
	    if (s->bind_type != SQL_BIND_BY_COLUMN) {
		lp = (SQLLEN *) ((char *) b->lenp + s->bind_type * rsi);
	    } else {
		lp = b->lenp + rsi;
	    }
	    if (s->bind_offs) {
		lp = (SQLLEN *) ((char *) lp + *s->bind_offs);
	    }
	}
	if (dp || lp) {
	    ret = getrowdata(s, (SQLUSMALLINT) i, b->type, dp, b->max, lp, 0);
	    if (!SQL_SUCCEEDED(ret)) {
		s->row_status0[rsi] = SQL_ROW_ERROR;
		break;
	    }
	    if (ret != SQL_SUCCESS) {
		withinfo = 1;
#ifdef SQL_ROW_SUCCESS_WITH_INFO
		s->row_status0[rsi] = SQL_ROW_SUCCESS_WITH_INFO;
#endif
	    }
	}
    }
    if (SQL_SUCCEEDED(ret)) {
	ret = withinfo ? SQL_SUCCESS_WITH_INFO : SQL_SUCCESS;
    }
    return ret;
}

/**
 * Internal fetch function for SQLFetchScroll() and SQLExtendedFetch().
 * @param stmt statement handle
 * @param orient fetch direction
 * @param offset offset for fetch direction
 * @result ODBC error code
 */

static SQLRETURN
drvfetchscroll(SQLHSTMT stmt, SQLSMALLINT orient, SQLINTEGER offset)
{
    STMT *s;
    int i, withinfo = 0;
    SQLRETURN ret;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    for (i = 0; i < s->rowset_size; i++) {
	s->row_status0[i] = SQL_ROW_NOROW;
    }
    if (s->row_status) {
	memcpy(s->row_status, s->row_status0,
	       sizeof (SQLUSMALLINT) * s->rowset_size);
    }
    s->row_count0 = 0;
    if (s->row_count) {
	*s->row_count = s->row_count0;
    }
    if (!s->bindcols) {
	for (i = 0; i < s->rowset_size; i++) {
	    s->row_status0[i] = SQL_ROW_ERROR;
	}
	ret = SQL_ERROR;
	i = 0;
	goto done2;
    }
    if (s->isselect != 1 && s->isselect != -1) {
	setstat(s, -1, "no result set available", "24000");
	ret = SQL_ERROR;
	i = s->nrows;
	goto done2;
    }
    if (s->curtype == SQL_CURSOR_FORWARD_ONLY && orient != SQL_FETCH_NEXT) {
	setstat(s, -1, "wrong fetch direction", "01000");
	ret = SQL_ERROR;
	i = 0;
	goto done2;
    }
    ret = SQL_SUCCESS;
    i = 0;
    if (((DBC *) (s->dbc))->vm_stmt == s && s->vm) {
	s->rowp = 0;
	for (; i < s->rowset_size; i++) {
	    ret = vm_step(s);
	    if (ret != SQL_SUCCESS) {
		s->row_status0[i] = SQL_ROW_ERROR;
		break;
	    }
	    if (s->nrows < 1) {
		break;
	    }
	    ret = dofetchbind(s, i);
	    if (!SQL_SUCCEEDED(ret)) {
		break;
	    } else if (ret == SQL_SUCCESS_WITH_INFO) {
		withinfo = 1;
	    }
	}
    } else if (s->rows) {
	switch (orient) {
	case SQL_FETCH_NEXT:
	    if (s->nrows < 1) {
		return SQL_NO_DATA;
	    }
	    if (s->rowp < 0) {
		s->rowp = -1;
	    }
	    if (s->rowp >= s->nrows) {
		s->rowp = s->nrows;
		return SQL_NO_DATA;
	    }
	    break;
	case SQL_FETCH_PRIOR:
	    if (s->nrows < 1 || s->rowp <= 0) {
		s->rowp = -1;
		return SQL_NO_DATA;
	    }
	    s->rowp -= s->rowset_size + 1;
	    if (s->rowp < -1) {
		s->rowp = -1;
		return SQL_NO_DATA;
	    }
	    break;
	case SQL_FETCH_FIRST:
	    if (s->nrows < 1) {
		return SQL_NO_DATA;
	    }
	    s->rowp = -1;
	    break;
	case SQL_FETCH_LAST:
	    if (s->nrows < 1) {
		return SQL_NO_DATA;
	    }
	    s->rowp = s->nrows - s->rowset_size;
	    if (--s->rowp < -1) {
		s->rowp = -1;
	    }
	    break;
	case SQL_FETCH_ABSOLUTE:
	    if (offset == 0) {
		s->rowp = -1;
		return SQL_NO_DATA;
	    } else if (offset < 0) {
		if (0 - offset <= s->nrows) {
		    s->rowp = s->nrows + offset - 1;
		    break;
		}
		s->rowp = -1;
		return SQL_NO_DATA;
	    } else if (offset > s->nrows) {
		s->rowp = s->nrows;
		return SQL_NO_DATA;
	    }
	    s->rowp = offset - 1 - 1;
	    break;
	case SQL_FETCH_RELATIVE:
	    if (offset >= 0) {
		s->rowp += offset * s->rowset_size - 1;
		if (s->rowp >= s->nrows) {
		    s->rowp = s->nrows;
		    return SQL_NO_DATA;
		}
	    } else {
		s->rowp += offset * s->rowset_size - 1;
		if (s->rowp < -1) {
		    s->rowp = -1;
		    return SQL_NO_DATA;
		}
	    }
	    break;
	case SQL_FETCH_BOOKMARK:
	    if (s->bkmrk) {
		if (offset < 0 || offset >= s->nrows) {
		    return SQL_NO_DATA;
		}
		s->rowp = offset - 1;
		break;
	    }
	    /* fall through */
	default:
	    s->row_status0[0] = SQL_ROW_ERROR;
	    ret = SQL_ERROR;
	    goto done;
	}
	for (; i < s->rowset_size; i++) {
	    ++s->rowp;
	    if (s->rowp < 0 || s->rowp >= s->nrows) {
		break;
	    }
	    ret = dofetchbind(s, i);
	    if (!SQL_SUCCEEDED(ret)) {
		break;
	    } else if (ret == SQL_SUCCESS_WITH_INFO) {
		withinfo = 1;
	    }
	}
    }
done:
    if (i == 0) {
	if (SQL_SUCCEEDED(ret)) {
	    return SQL_NO_DATA;
	}
	return ret;
    }
    if (SQL_SUCCEEDED(ret)) {
	ret = withinfo ? SQL_SUCCESS_WITH_INFO : SQL_SUCCESS;
    }
done2:
    if (s->row_status) {
	memcpy(s->row_status, s->row_status0,
	       sizeof (SQLUSMALLINT) * s->rowset_size);
    }
    s->row_count0 = i;
    if (s->row_count) {
	*s->row_count = s->row_count0;
    }
    return ret;
}

/**
 * Fetch next result row.
 * @param stmt statement handle
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLFetch(SQLHSTMT stmt)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvfetchscroll(stmt, SQL_FETCH_NEXT, 0);
    HSTMT_UNLOCK(stmt);
    return ret;
}

/**
 * Fetch result row with scrolling.
 * @param stmt statement handle
 * @param orient fetch direction
 * @param offset offset for fetch direction
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLFetchScroll(SQLHSTMT stmt, SQLSMALLINT orient, SQLLEN offset)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvfetchscroll(stmt, orient, offset);
    HSTMT_UNLOCK(stmt);
    return ret;
}

/**
 * Fetch result row with scrolling and row status.
 * @param stmt statement handle
 * @param orient fetch direction
 * @param offset offset for fetch direction
 * @param rowcount output number of fetched rows
 * @param rowstatus array for row stati
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLExtendedFetch(SQLHSTMT stmt, SQLUSMALLINT orient, SQLROWOFFSET offset,
		 SQLROWSETSIZE *rowcount, SQLUSMALLINT *rowstatus)
{
    STMT *s;
    SQLRETURN ret;
    SQLUSMALLINT *rst;

    HSTMT_LOCK(stmt);
    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    /* temporarily turn off SQL_ATTR_ROW_STATUS_PTR */
    rst = s->row_status;
    s->row_status = 0;
    ret = drvfetchscroll(stmt, orient, offset);
    s->row_status = rst;
    if (rowstatus) {
	memcpy(rowstatus, s->row_status0,
	       sizeof (SQLUSMALLINT) * s->rowset_size);
    }
    if (rowcount) {
	*rowcount = s->row_count0;
    }
    HSTMT_UNLOCK(stmt);
    return ret;
}

/**
 * Return number of affected rows of HSTMT.
 * @param stmt statement handle
 * @param nrows output number of rows
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLRowCount(SQLHSTMT stmt, SQLLEN *nrows)
{
    STMT *s;

    HSTMT_LOCK(stmt);
    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (nrows) {
	*nrows = s->isselect ? 0 : s->nrows;
    }
    HSTMT_UNLOCK(stmt);
    return SQL_SUCCESS;
}

/**
 * Return number of columns of result set given HSTMT.
 * @param stmt statement handle
 * @param ncols output number of columns
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLNumResultCols(SQLHSTMT stmt, SQLSMALLINT *ncols)
{
    STMT *s;

    HSTMT_LOCK(stmt);
    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (ncols) {
	*ncols = s->ncols;
    }
    HSTMT_UNLOCK(stmt);
    return SQL_SUCCESS;
}

/**
 * Internal describe column information.
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param name buffer for column name
 * @param nameMax length of name buffer
 * @param nameLen output length of column name
 * @param type output SQL type
 * @param size output column size
 * @param digits output number of digits
 * @param nullable output NULL allowed indicator
 * @result ODBC error code
 */

static SQLRETURN
drvdescribecol(SQLHSTMT stmt, SQLUSMALLINT col, SQLCHAR *name,
	       SQLSMALLINT nameMax, SQLSMALLINT *nameLen,
	       SQLSMALLINT *type, SQLULEN *size,
	       SQLSMALLINT *digits, SQLSMALLINT *nullable)
{
    STMT *s;
    COL *c;
    int didname = 0;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (!s->cols) {
	setstat(s, -1, "no columns", (*s->ov3) ? "07009" : "S1002");
	return SQL_ERROR;
    }
    if (col < 1 || col > s->ncols) {
	setstat(s, -1, "invalid column", (*s->ov3) ? "07009" : "S1002");
	return SQL_ERROR;
    }
    c = s->cols + col - 1;
    if (name && nameMax > 0) {
	strncpy((char *) name, c->column, nameMax);
	name[nameMax - 1] = '\0';
	didname = 1;
    }
    if (nameLen) {
	if (didname) {
	    *nameLen = strlen((char *) name);
	} else {
	    *nameLen = strlen(c->column);
	}
    }
    if (type) {
	*type = c->type;
#ifdef WINTERFACE
	if (s->nowchar[0] || s->nowchar[1]) {
	    switch (c->type) {
	    case SQL_WCHAR:
		*type = SQL_CHAR;
		break;
	    case SQL_WVARCHAR:
		*type = SQL_VARCHAR;
		break;
#ifdef SQL_LONGVARCHAR
	    case SQL_WLONGVARCHAR:
		*type = SQL_LONGVARCHAR;
		break;
#endif
	    }
	}
#endif
    }
    if (size) {
	*size = c->size;
    }
    if (digits) {
	*digits = 0;
    }
    if (nullable) {
	*nullable = 1;
    }
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Describe column information.
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param name buffer for column name
 * @param nameMax length of name buffer
 * @param nameLen output length of column name
 * @param type output SQL type
 * @param size output column size
 * @param digits output number of digits
 * @param nullable output NULL allowed indicator
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLDescribeCol(SQLHSTMT stmt, SQLUSMALLINT col, SQLCHAR *name,
	       SQLSMALLINT nameMax, SQLSMALLINT *nameLen,
	       SQLSMALLINT *type, SQLULEN *size,
	       SQLSMALLINT *digits, SQLSMALLINT *nullable)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvdescribecol(stmt, col, name, nameMax, nameLen,
			 type, size, digits, nullable);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Describe column information (UNICODE version).
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param name buffer for column name
 * @param nameMax length of name buffer
 * @param nameLen output length of column name
 * @param type output SQL type
 * @param size output column size
 * @param digits output number of digits
 * @param nullable output NULL allowed indicator
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLDescribeColW(SQLHSTMT stmt, SQLUSMALLINT col, SQLWCHAR *name,
		SQLSMALLINT nameMax, SQLSMALLINT *nameLen,
		SQLSMALLINT *type, SQLULEN *size,
		SQLSMALLINT *digits, SQLSMALLINT *nullable)
{
    SQLRETURN ret;
    SQLSMALLINT len = 0;

    HSTMT_LOCK(stmt);
    ret = drvdescribecol(stmt, col, (SQLCHAR *) name,
			 (SQLSMALLINT) (nameMax * sizeof (SQLWCHAR)),
			 &len, type, size, digits, nullable);
    if (ret == SQL_SUCCESS) {
	if (name) {
	    if (len > 0) {
		SQLWCHAR *n = NULL;

		n = uc_from_utf((SQLCHAR *) name, len);
		if (n) {
		    uc_strncpy(name, n, nameMax);
		    n[len] = 0;
		    len = min(nameMax, uc_strlen(n));
		    uc_free(n);
		} else {
		    len = 0;
		}
	    }
	    if (len <= 0) {
		len = 0;
		if (nameMax > 0) {
		    name[0] = 0;
		}
	    }
	} else {
	    STMT *s = (STMT *) stmt;
	    COL *c = s->cols + col - 1;

	    len = 0;
	    if (c->column) {
		len = strlen(c->column);
	    }
	}
	if (nameLen) {
	    *nameLen = len;
	}
    }
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Internal retrieve column attributes.
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param id attribute id
 * @param val output buffer
 * @param valMax length of output buffer
 * @param valLen output length
 * @param val2 integer output buffer
 * @result ODBC error code
 */

static SQLRETURN
drvcolattributes(SQLHSTMT stmt, SQLUSMALLINT col, SQLUSMALLINT id,
		 SQLPOINTER val, SQLSMALLINT valMax, SQLSMALLINT *valLen,
		 SQLLEN *val2)
{
    STMT *s;
    COL *c;
    SQLSMALLINT dummy;
    char *valc = (char *) val;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (!s->cols) {
	return SQL_ERROR;
    }
    if (!valLen) {
	valLen = &dummy;
    }
    if (id == SQL_COLUMN_COUNT) {
	if (val2) {
	    *val2 = s->ncols;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    }
    if (id == SQL_COLUMN_TYPE && col == 0) {
	if (val2) {
	    *val2 = SQL_INTEGER;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    }
#ifdef SQL_DESC_OCTET_LENGTH
    if (id == SQL_DESC_OCTET_LENGTH && col == 0) {
	if (val2) {
	    *val2 = 4;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    }
#endif
    if (col < 1 || col > s->ncols) {
	setstat(s, -1, "invalid column", (*s->ov3) ? "07009": "S1002");
	return SQL_ERROR;
    }
    c = s->cols + col - 1;

    switch (id) {
    case SQL_COLUMN_LABEL:
	if (c->label) {
	    if (valc && valMax > 0) {
		strncpy(valc, c->label, valMax);
		valc[valMax - 1] = '\0';
	    }
	    *valLen = strlen(c->label);
	    goto checkLen;
	}
	/* fall through */
    case SQL_COLUMN_NAME:
    case SQL_DESC_NAME:
	if (valc && valMax > 0) {
	    strncpy(valc, c->column, valMax);
	    valc[valMax - 1] = '\0';
	}
	*valLen = strlen(c->column);
checkLen:
	if (*valLen >= valMax) {
	    setstat(s, -1, "data right truncated", "01004");
	    return SQL_SUCCESS_WITH_INFO;
	}
	return SQL_SUCCESS;
#ifdef SQL_DESC_BASE_COLUMN_NAME
    case SQL_DESC_BASE_COLUMN_NAME:
	if (strchr(c->column, '(') || strchr(c->column, ')')) {
	    valc[0] = '\0';
	    *valLen = 0;
	} else if (valc && valMax > 0) {
	    strncpy(valc, c->column, valMax);
	    valc[valMax - 1] = '\0';
	    *valLen = strlen(c->column);
	}
	goto checkLen;
#endif
    case SQL_COLUMN_TYPE:
    case SQL_DESC_TYPE:
#ifdef WINTERFACE
	{
	    int type = c->type;

	    if (s->nowchar[0] || s->nowchar[1]) {
		switch (type) {
		case SQL_WCHAR:
		    type = SQL_CHAR;
		    break;
		case SQL_WVARCHAR:
		    type = SQL_VARCHAR;
		    break;
#ifdef SQL_LONGVARCHAR
		case SQL_WLONGVARCHAR:
		    type = SQL_LONGVARCHAR;
		    break;
		}
	    }
	    if (val2) {
		*val2 = type;
	    }
#endif
	}
#else
	if (val2) {
	    *val2 = c->type;
	}
#endif
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_DISPLAY_SIZE:
	if (val2) {
	    *val2 = c->size;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_UNSIGNED:
	if (val2) {
	    *val2 = c->nosign ? SQL_TRUE : SQL_FALSE;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_SCALE:
    case SQL_DESC_SCALE:
	if (val2) {
	    *val2 = c->scale;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_PRECISION:
    case SQL_DESC_PRECISION:
	if (val2) {
	    switch (c->type) {
	    case SQL_SMALLINT:
		*val2 = 5;
		break;
	    case SQL_INTEGER:
		*val2 = 10;
		break;
	    case SQL_FLOAT:
	    case SQL_REAL:
	    case SQL_DOUBLE:
		*val2 = 15;
		break;
	    case SQL_DATE:
		*val2 = 0;
		break;
	    case SQL_TIME:
		*val2 = 0;
		break;
#ifdef SQL_TYPE_TIMESTAMP
	    case SQL_TYPE_TIMESTAMP:
#endif
	    case SQL_TIMESTAMP:
		*val2 = (c->prec >= 0 && c->prec <= 3) ? c->prec : 3;
		break;
	    default:
		*val2 = c->prec;
		break;
	    }
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_MONEY:
	if (val2) {
	    *val2 = SQL_FALSE;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_AUTO_INCREMENT:
	if (val2) {
	    *val2 = c->autoinc;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_LENGTH:
    case SQL_DESC_LENGTH:
	if (val2) {
	    *val2 = c->size;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_NULLABLE:
    case SQL_DESC_NULLABLE:
	if (val2) {
	    *val2 = c->notnull;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_SEARCHABLE:
	if (val2) {
	    *val2 = SQL_SEARCHABLE;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_CASE_SENSITIVE:
	if (val2) {
	    *val2 = SQL_TRUE;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_UPDATABLE:
	if (val2) {
	    *val2 = SQL_TRUE;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_DESC_COUNT:
	if (val2) {
	    *val2 = s->ncols;
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
    case SQL_COLUMN_TYPE_NAME: {
	char *p = NULL, *tn = c->typename ? c->typename : "varchar";

#ifdef WINTERFACE
	if (c->type == SQL_WCHAR ||
	    c->type == SQL_WVARCHAR ||
	    c->type == SQL_WLONGVARCHAR) {
	    if (!(s->nowchar[0] || s->nowchar[1])) {
		if (strcasecmp(tn, "varchar") == 0) {
		    tn = "wvarchar";
		}
	    }
	}
#endif
	if (valc && valMax > 0) {
	    strncpy(valc, tn, valMax);
	    valc[valMax - 1] = '\0';
	    p = strchr(valc, '(');
	    if (p) {
		*p = '\0';
		while (p > valc && ISSPACE(p[-1])) {
		    --p;
		    *p = '\0';
		}
	    }
	    *valLen = strlen(valc);
	} else {
	    *valLen = strlen(tn);
	    p = strchr(tn, '(');
	    if (p) {
		*valLen = p - tn;
		while (p > tn && ISSPACE(p[-1])) {
		    --p;
		    *valLen -= 1;
		}
	    }
	}
	goto checkLen;
    }
    case SQL_COLUMN_OWNER_NAME:
    case SQL_COLUMN_QUALIFIER_NAME: {
	char *z = "";

	if (valc && valMax > 0) {
	    strncpy(valc, z, valMax);
	    valc[valMax - 1] = '\0';
	}
	*valLen = strlen(z);
	goto checkLen;
    }
    case SQL_COLUMN_TABLE_NAME:
#if (SQL_COLUMN_TABLE_NAME != SQL_DESC_TABLE_NAME)
    case SQL_DESC_TABLE_NAME:
#endif
#ifdef SQL_DESC_BASE_TABLE_NAME
    case SQL_DESC_BASE_TABLE_NAME:
#endif
	if (valc && valMax > 0) {
	    strncpy(valc, c->table, valMax);
	    valc[valMax - 1] = '\0';
	}
	*valLen = strlen(c->table);
	goto checkLen;
#ifdef SQL_DESC_NUM_PREC_RADIX
    case SQL_DESC_NUM_PREC_RADIX:
	if (val2) {
	    switch (c->type) {
#ifdef WINTERFACE
	    case SQL_WCHAR:
	    case SQL_WVARCHAR:
#ifdef SQL_LONGVARCHAR
	    case SQL_WLONGVARCHAR:
#endif
#endif
	    case SQL_CHAR:
	    case SQL_VARCHAR:
#ifdef SQL_LONGVARCHAR
	    case SQL_LONGVARCHAR:
#endif
	    case SQL_BINARY:
	    case SQL_VARBINARY:
	    case SQL_LONGVARBINARY:
		*val2 = 0;
		break;
	    default:
		*val2 = 2;
	    }
	}
	*valLen = sizeof (int);
	return SQL_SUCCESS;
#endif
    }
    setstat(s, -1, "unsupported column attributes %d", "HY091", id);
    return SQL_ERROR;
}

#ifndef WINTERFACE
/**
 * Retrieve column attributes.
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param id attribute id
 * @param val output buffer
 * @param valMax length of output buffer
 * @param valLen output length
 * @param val2 integer output buffer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLColAttributes(SQLHSTMT stmt, SQLUSMALLINT col, SQLUSMALLINT id,
		 SQLPOINTER val, SQLSMALLINT valMax, SQLSMALLINT *valLen,
		 SQLLEN *val2)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvcolattributes(stmt, col, id, val, valMax, valLen, val2);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Retrieve column attributes (UNICODE version).
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param id attribute id
 * @param val output buffer
 * @param valMax length of output buffer
 * @param valLen output length
 * @param val2 integer output buffer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLColAttributesW(SQLHSTMT stmt, SQLUSMALLINT col, SQLUSMALLINT id,
		  SQLPOINTER val, SQLSMALLINT valMax, SQLSMALLINT *valLen,
		  SQLLEN *val2)
{
    SQLRETURN ret;
    SQLSMALLINT len = 0;

    HSTMT_LOCK(stmt);
    ret = drvcolattributes(stmt, col, id, val, valMax, &len, val2);
    if (SQL_SUCCEEDED(ret)) {
	SQLWCHAR *v = NULL;

	switch (id) {
	case SQL_COLUMN_LABEL:
	case SQL_COLUMN_NAME:
	case SQL_DESC_NAME:
	case SQL_COLUMN_TYPE_NAME:
	case SQL_COLUMN_OWNER_NAME:
	case SQL_COLUMN_QUALIFIER_NAME:
	case SQL_COLUMN_TABLE_NAME:
#if (SQL_COLUMN_TABLE_NAME != SQL_DESC_TABLE_NAME)
	case SQL_DESC_TABLE_NAME:
#endif
#ifdef SQL_DESC_BASE_COLUMN_NAME
	case SQL_DESC_BASE_COLUMN_NAME:
#endif
#ifdef SQL_DESC_BASE_TABLE_NAME
    case SQL_DESC_BASE_TABLE_NAME:
#endif
	    if (val && valMax > 0) {
		int vmax = valMax / sizeof (SQLWCHAR);

		v = uc_from_utf((SQLCHAR *) val, SQL_NTS);
		if (v) {
		    uc_strncpy(val, v, vmax);
		    len = min(vmax, uc_strlen(v));
		    uc_free(v);
		    len *= sizeof (SQLWCHAR);
		}
		if (vmax > 0) {
		    v = (SQLWCHAR *) val;
		    v[vmax - 1] = '\0';
		}
	    }
	    if (len <= 0) {
		len = 0;
	    }
	    break;
	}
	if (valLen) {
	    *valLen = len;
	}
    }
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Internal retrieve column attributes.
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param id attribute id
 * @param val output buffer
 * @param valMax length of output buffer
 * @param valLen output length
 * @param val2 integer output buffer
 * @result ODBC error code
 */

static SQLRETURN
drvcolattribute(SQLHSTMT stmt, SQLUSMALLINT col, SQLUSMALLINT id,
		SQLPOINTER val, SQLSMALLINT valMax, SQLSMALLINT *valLen,
		SQLPOINTER val2)
{
    STMT *s;
    COL *c;
    int v = 0;
    char *valc = (char *) val;
    SQLSMALLINT dummy;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (!s->cols) {
	return SQL_ERROR;
    }
    if (col < 1 || col > s->ncols) {
	setstat(s, -1, "invalid column", (*s->ov3) ? "07009" : "S1002");
	return SQL_ERROR;
    }
    if (!valLen) {
	valLen = &dummy;
    }
    c = s->cols + col - 1;
    switch (id) {
    case SQL_DESC_COUNT:
	v = s->ncols;
	break;
    case SQL_DESC_CATALOG_NAME:
	if (valc && valMax > 0) {
	    strncpy(valc, c->db, valMax);
	    valc[valMax - 1] = '\0';
	}
	*valLen = strlen(c->db);
checkLen:
	if (*valLen >= valMax) {
	    setstat(s, -1, "data right truncated", "01004");
	    return SQL_SUCCESS_WITH_INFO;
	}
	break;
    case SQL_COLUMN_LENGTH:
    case SQL_DESC_LENGTH:
	v = c->size;
	break;
    case SQL_COLUMN_LABEL:
	if (c->label) {
	    if (valc && valMax > 0) {
		strncpy(valc, c->label, valMax);
		valc[valMax - 1] = '\0';
	    }
	    *valLen = strlen(c->label);
	    goto checkLen;
	}
	/* fall through */
    case SQL_COLUMN_NAME:
    case SQL_DESC_NAME:
	if (valc && valMax > 0) {
	    strncpy(valc, c->column, valMax);
	    valc[valMax - 1] = '\0';
	}
	*valLen = strlen(c->column);
	goto checkLen;
    case SQL_DESC_SCHEMA_NAME: {
	char *z = "";

	if (valc && valMax > 0) {
	    strncpy(valc, z, valMax);
	    valc[valMax - 1] = '\0';
	}
	*valLen = strlen(z);
	goto checkLen;
    }
#ifdef SQL_DESC_BASE_COLUMN_NAME
    case SQL_DESC_BASE_COLUMN_NAME:
	if (strchr(c->column, '(') || strchr(c->column, ')')) {
	    valc[0] = '\0';
	    *valLen = 0;
	} else if (valc && valMax > 0) {
	    strncpy(valc, c->column, valMax);
	    valc[valMax - 1] = '\0';
	    *valLen = strlen(c->column);
	}
	goto checkLen;
#endif
    case SQL_DESC_TYPE_NAME: {
	char *p = NULL, *tn = c->typename ? c->typename : "varchar";

#ifdef WINTERFACE
	if (c->type == SQL_WCHAR ||
	    c->type == SQL_WVARCHAR ||
	    c->type == SQL_WLONGVARCHAR) {
	    if (!(s->nowchar[0] || s->nowchar[1])) {
		if (strcasecmp(tn, "varchar") == 0) {
		    tn = "wvarchar";
		}
	    }
	}
#endif
	if (valc && valMax > 0) {
	    strncpy(valc, tn, valMax);
	    valc[valMax - 1] = '\0';
	    p = strchr(valc, '(');
	    if (p) {
		*p = '\0';
		while (p > valc && ISSPACE(p[-1])) {
		    --p;
		    *p = '\0';
		}
	    }
	    *valLen = strlen(valc);
	} else {
	    *valLen = strlen(tn);
	    p = strchr(tn, '(');
	    if (p) {
		*valLen = p - tn;
		while (p > tn && ISSPACE(p[-1])) {
		    --p;
		    *valLen -= 1;
		}
	    }
	}
	goto checkLen;
    }
    case SQL_DESC_OCTET_LENGTH:
	v = c->size;
#ifdef WINTERFACE
	if (c->type == SQL_WCHAR ||
	    c->type == SQL_WVARCHAR ||
	    c->type == SQL_WLONGVARCHAR) {
	    if (!(s->nowchar[0] || s->nowchar[1])) {
		v *= sizeof (SQLWCHAR);
	    }
	}
#endif
	break;
#if (SQL_COLUMN_TABLE_NAME != SQL_DESC_TABLE_NAME)
    case SQL_COLUMN_TABLE_NAME:
#endif
    case SQL_DESC_TABLE_NAME:
#ifdef SQL_DESC_BASE_TABLE_NAME
    case SQL_DESC_BASE_TABLE_NAME:
#endif
	if (valc && valMax > 0) {
	    strncpy(valc, c->table, valMax);
	    valc[valMax - 1] = '\0';
	}
	*valLen = strlen(c->table);
	goto checkLen;
    case SQL_DESC_TYPE:
	v = c->type;
#ifdef WINTERFACE
	if (s->nowchar[0] || s->nowchar[1]) {
	    switch (v) {
	    case SQL_WCHAR:
		v = SQL_CHAR;
		break;
	    case SQL_WVARCHAR:
		v = SQL_VARCHAR;
		break;
#ifdef SQL_LONGVARCHAR
	    case SQL_WLONGVARCHAR:
		v = SQL_LONGVARCHAR;
		break;
#endif
	    }
	}
#endif
	break;
    case SQL_DESC_CONCISE_TYPE:
	switch (c->type) {
	case SQL_INTEGER:
	    v = SQL_C_LONG;
	    break;
	case SQL_TINYINT:
	    v = SQL_C_TINYINT;
	    break;
	case SQL_SMALLINT:
	    v = SQL_C_SHORT;
	    break;
	case SQL_FLOAT:
	    v = SQL_C_FLOAT;
	    break;
	case SQL_DOUBLE:
	    v = SQL_C_DOUBLE;
	    break;
	case SQL_TIMESTAMP:
	    v = SQL_C_TIMESTAMP;
	    break;
	case SQL_TIME:
	    v = SQL_C_TIME;
	    break;
	case SQL_DATE:
	    v = SQL_C_DATE;
	    break;
#ifdef SQL_C_TYPE_TIMESTAMP
	case SQL_TYPE_TIMESTAMP:
	    v = SQL_C_TYPE_TIMESTAMP;
	    break;
#endif
#ifdef SQL_C_TYPE_TIME
	case SQL_TYPE_TIME:
	    v = SQL_C_TYPE_TIME;
	    break;
#endif
#ifdef SQL_C_TYPE_DATE
	case SQL_TYPE_DATE:
	    v = SQL_C_TYPE_DATE;
	    break;
#endif
#ifdef SQL_BIT
	case SQL_BIT:
	    v = SQL_C_BIT;
	    break;
#endif
	default:
#ifdef WINTERFACE
	    v = (s->nowchar[0] || s->nowchar[1]) ? SQL_C_CHAR : SQL_C_WCHAR;
#else
	    v = SQL_C_CHAR;
#endif
	    break;
	}
	break;
    case SQL_DESC_UPDATABLE:
	v = SQL_TRUE;
	break;
    case SQL_COLUMN_DISPLAY_SIZE:
	v = c->size;
	break;
    case SQL_COLUMN_UNSIGNED:
	v = c->nosign ? SQL_TRUE : SQL_FALSE;
	break;
    case SQL_COLUMN_SEARCHABLE:
	v = SQL_SEARCHABLE;
	break;
    case SQL_COLUMN_SCALE:
    case SQL_DESC_SCALE:
	v = c->scale;
	break;
    case SQL_COLUMN_PRECISION:
    case SQL_DESC_PRECISION:
	switch (c->type) {
	case SQL_SMALLINT:
	    v = 5;
	    break;
	case SQL_INTEGER:
	    v = 10;
	    break;
	case SQL_FLOAT:
	case SQL_REAL:
	case SQL_DOUBLE:
	    v = 15;
	    break;
	case SQL_DATE:
	    v = 0;
	    break;
	case SQL_TIME:
	    v = 0;
	    break;
#ifdef SQL_TYPE_TIMESTAMP
	case SQL_TYPE_TIMESTAMP:
#endif
	case SQL_TIMESTAMP:
	    v = (c->prec >= 0 && c->prec <= 3) ? c->prec : 3;
	    break;
	default:
	    v = c->prec;
	    break;
	}
	break;
    case SQL_COLUMN_MONEY:
	v = SQL_FALSE;
	break;
    case SQL_COLUMN_AUTO_INCREMENT:
	v = c->autoinc;
	break;
    case SQL_DESC_NULLABLE:
	v = c->notnull;
	break;
#ifdef SQL_DESC_NUM_PREC_RADIX
    case SQL_DESC_NUM_PREC_RADIX:
	switch (c->type) {
#ifdef WINTERFACE
	case SQL_WCHAR:
	case SQL_WVARCHAR:
#ifdef SQL_LONGVARCHAR
	case SQL_WLONGVARCHAR:
#endif
#endif
	case SQL_CHAR:
	case SQL_VARCHAR:
#ifdef SQL_LONGVARCHAR
	case SQL_LONGVARCHAR:
#endif
	case SQL_BINARY:
	case SQL_VARBINARY:
	case SQL_LONGVARBINARY:
	    v = 0;
	    break;
	default:
	    v = 2;
	}
	break;
#endif
    default:
	setstat(s, -1, "unsupported column attribute %d", "HY091", id);
	return SQL_ERROR;
    }
    if (val2) {
	*(SQLLEN *) val2 = v;
    }
    return SQL_SUCCESS;
}

#ifndef WINTERFACE
/**
 * Retrieve column attributes.
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param id attribute id
 * @param val output buffer
 * @param valMax length of output buffer
 * @param valLen output length
 * @param val2 integer output buffer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLColAttribute(SQLHSTMT stmt, SQLUSMALLINT col, SQLUSMALLINT id,
		SQLPOINTER val, SQLSMALLINT valMax, SQLSMALLINT *valLen,
		COLATTRIBUTE_LAST_ARG_TYPE val2)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvcolattribute(stmt, col, id, val, valMax, valLen,
			  (SQLPOINTER) val2);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Retrieve column attributes (UNICODE version).
 * @param stmt statement handle
 * @param col column number, starting at 1
 * @param id attribute id
 * @param val output buffer
 * @param valMax length of output buffer
 * @param valLen output length
 * @param val2 integer output buffer
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLColAttributeW(SQLHSTMT stmt, SQLUSMALLINT col, SQLUSMALLINT id,
		 SQLPOINTER val, SQLSMALLINT valMax, SQLSMALLINT *valLen,
		 COLATTRIBUTE_LAST_ARG_TYPE val2)
{
    SQLRETURN ret;
    SQLSMALLINT len = 0;

    HSTMT_LOCK(stmt);
    ret = drvcolattribute(stmt, col, id, val, valMax, &len,
			  (SQLPOINTER) val2);
    if (SQL_SUCCEEDED(ret)) {
	SQLWCHAR *v = NULL;

	switch (id) {
	case SQL_DESC_SCHEMA_NAME:
	case SQL_DESC_CATALOG_NAME:
	case SQL_COLUMN_LABEL:
	case SQL_DESC_NAME:
	case SQL_DESC_TABLE_NAME:
#ifdef SQL_DESC_BASE_TABLE_NAME
	case SQL_DESC_BASE_TABLE_NAME:
#endif
#ifdef SQL_DESC_BASE_COLUMN_NAME
	case SQL_DESC_BASE_COLUMN_NAME:
#endif
	case SQL_DESC_TYPE_NAME:
	    if (val && valMax > 0) {
		int vmax = valMax / sizeof (SQLWCHAR);

		v = uc_from_utf((SQLCHAR *) val, SQL_NTS);
		if (v) {
		    uc_strncpy(val, v, vmax);
		    len = min(vmax, uc_strlen(v));
		    uc_free(v);
		    len *= sizeof (SQLWCHAR);
		}
		if (vmax > 0) {
		    v = (SQLWCHAR *) val;
		    v[vmax - 1] = '\0';
		}
	    }
	    if (len <= 0) {
		len = 0;
	    }
	    break;
	}
	if (valLen) {
	    *valLen = len;
	}
    }
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Internal return last HDBC or HSTMT error message.
 * @param env environment handle or NULL
 * @param dbc database connection handle or NULL
 * @param stmt statement handle or NULL
 * @param sqlState output buffer for SQL state
 * @param nativeErr output buffer for native error code
 * @param errmsg output buffer for error message
 * @param errmax length of output buffer for error message
 * @param errlen output length of error message
 * @result ODBC error code
 */

static SQLRETURN
drverror(SQLHENV env, SQLHDBC dbc, SQLHSTMT stmt,
	 SQLCHAR *sqlState, SQLINTEGER *nativeErr,
	 SQLCHAR *errmsg, SQLSMALLINT errmax, SQLSMALLINT *errlen)
{
    SQLCHAR dummy0[6];
    SQLINTEGER dummy1;
    SQLSMALLINT dummy2;

    if (env == SQL_NULL_HENV &&
	dbc == SQL_NULL_HDBC &&
	stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    if (sqlState) {
	sqlState[0] = '\0';
    } else {
	sqlState = dummy0;
    }
    if (!nativeErr) {
	nativeErr = &dummy1;
    }
    *nativeErr = 0;
    if (!errlen) {
	errlen = &dummy2;
    }
    *errlen = 0;
    if (errmsg) {
	if (errmax > 0) {
	    errmsg[0] = '\0';
	}
    } else {
	errmsg = dummy0;
	errmax = 0;
    }
    if (stmt) {
	STMT *s = (STMT *) stmt;

	HSTMT_LOCK(stmt);
	if (s->logmsg[0] == '\0') {
	    HSTMT_UNLOCK(stmt);
	    goto noerr;
	}
	*nativeErr = s->naterr;
	strcpy((char *) sqlState, s->sqlstate);
	if (errmax == SQL_NTS) {
	    strcpy((char *) errmsg, "[SQLite]");
	    strcat((char *) errmsg, (char *) s->logmsg);
	    *errlen = strlen((char *) errmsg);
	} else {
	    strncpy((char *) errmsg, "[SQLite]", errmax);
	    if (errmax - 8 > 0) {
		strncpy((char *) errmsg + 8, (char *) s->logmsg, errmax - 8);
	    }
	    *errlen = min(strlen((char *) s->logmsg) + 8, errmax);
	}
	s->logmsg[0] = '\0';
	HSTMT_UNLOCK(stmt);
	return SQL_SUCCESS;
    }
    if (dbc) {
	DBC *d = (DBC *) dbc;

	HDBC_LOCK(dbc);
	if (d->magic != DBC_MAGIC || d->logmsg[0] == '\0') {
	    HDBC_UNLOCK(dbc);
	    goto noerr;
	}
	*nativeErr = d->naterr;
	strcpy((char *) sqlState, d->sqlstate);
	if (errmax == SQL_NTS) {
	    strcpy((char *) errmsg, "[SQLite]");
	    strcat((char *) errmsg, (char *) d->logmsg);
	    *errlen = strlen((char *) errmsg);
	} else {
	    strncpy((char *) errmsg, "[SQLite]", errmax);
	    if (errmax - 8 > 0) {
		strncpy((char *) errmsg + 8, (char *) d->logmsg, errmax - 8);
	    }
	    *errlen = min(strlen((char *) d->logmsg) + 8, errmax);
	}
	d->logmsg[0] = '\0';
	HDBC_UNLOCK(dbc);
	return SQL_SUCCESS;
    }
noerr:
    sqlState[0] = '\0';
    errmsg[0] = '\0';
    *nativeErr = 0;
    *errlen = 0;
    return SQL_NO_DATA;
}

#ifndef WINTERFACE
/**
 * Return last HDBC or HSTMT error message.
 * @param env environment handle or NULL
 * @param dbc database connection handle or NULL
 * @param stmt statement handle or NULL
 * @param sqlState output buffer for SQL state
 * @param nativeErr output buffer for native error code
 * @param errmsg output buffer for error message
 * @param errmax length of output buffer for error message
 * @param errlen output length of error message
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLError(SQLHENV env, SQLHDBC dbc, SQLHSTMT stmt,
	 SQLCHAR *sqlState, SQLINTEGER *nativeErr,
	 SQLCHAR *errmsg, SQLSMALLINT errmax, SQLSMALLINT *errlen)
{
    return drverror(env, dbc, stmt, sqlState, nativeErr,
		    errmsg, errmax, errlen);
}
#endif

#ifdef WINTERFACE
/**
 * Return last HDBC or HSTMT error message (UNICODE version).
 * @param env environment handle or NULL
 * @param dbc database connection handle or NULL
 * @param stmt statement handle or NULL
 * @param sqlState output buffer for SQL state
 * @param nativeErr output buffer for native error code
 * @param errmsg output buffer for error message
 * @param errmax length of output buffer for error message
 * @param errlen output length of error message
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLErrorW(SQLHENV env, SQLHDBC dbc, SQLHSTMT stmt,
	  SQLWCHAR *sqlState, SQLINTEGER *nativeErr,
	  SQLWCHAR *errmsg, SQLSMALLINT errmax, SQLSMALLINT *errlen)
{
    char state[16];
    SQLSMALLINT len = 0;
    SQLRETURN ret;
    
    ret = drverror(env, dbc, stmt, (SQLCHAR *) state, nativeErr,
		   (SQLCHAR *) errmsg, errmax, &len);
    if (ret == SQL_SUCCESS) {
	if (sqlState) {
	    uc_from_utf_buf((SQLCHAR *) state, -1, sqlState,
			    6 * sizeof (SQLWCHAR));
	}
	if (errmsg) {
	    if (len > 0) {
		SQLWCHAR *e = NULL;

		e = uc_from_utf((SQLCHAR *) errmsg, len);
		if (e) {
		    if (errmax > 0) {
			uc_strncpy(errmsg, e, errmax);
			e[len] = 0;
			len = min(errmax, uc_strlen(e));
		    } else {
			len = uc_strlen(e);
		    }
		    uc_free(e);
		} else {
		    len = 0;
		}
	    }
	    if (len <= 0) {
		len = 0;
		if (errmax > 0) {
		    errmsg[0] = 0;
		}
	    }
	} else {
	    len = 0;
	}
	if (errlen) {
	    *errlen = len;
	}
    } else if (ret == SQL_NO_DATA) {
	if (sqlState) {
	    sqlState[0] = 0;
	}
	if (errmsg) {
	    if (errmax > 0) {
		errmsg[0] = 0;
	    }
	}
	if (errlen) {
	    *errlen = 0;
	}
    }
    return ret;
}
#endif

/**
 * Return information for more result sets.
 * @param stmt statement handle
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLMoreResults(SQLHSTMT stmt)
{
    HSTMT_LOCK(stmt);
    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    HSTMT_UNLOCK(stmt);
    return SQL_NO_DATA;
}

/**
 * SQLite callback during drvprepare(), used to collect column information.
 * @param arg user data, actually a statement pointer
 * @param ncols number of columns
 * @param values value string array
 * @param cols column name string array
 * @result always 1 to abort sqlite_exec()
 */

static int
selcb(void *arg, int ncols, char **values, char **cols)
{
    STMT *s = (STMT *) arg;

    if (ncols > 0) {
	int i, size;
	char *p;
	COL *dyncols;
	DBC *d = (DBC *) s->dbc;

	for (i = size = 0; i < ncols; i++) {
	    size += 3 + 3 * strlen(cols[i]);
	}
	dyncols = xmalloc(ncols * sizeof (COL) + size);
	if (!dyncols) {
	    freedyncols(s);
	    s->ncols = 0;
	    return 1;
	}
	p = (char *) (dyncols + ncols);
	for (i = 0; i < ncols; i++) {
	    char *q;

	    dyncols[i].db = ((DBC *) (s->dbc))->dbname;
	    strcpy(p, cols[i]);
	    dyncols[i].label = p;
	    p += strlen(p) + 1;
	    q = strchr(cols[i], '.');
	    if (q) {
		dyncols[i].table = p;
		strncpy(p, cols[i], q - cols[i]);
		p[q - cols[i]] = '\0';
		p += strlen(p) + 1;
		strcpy(p, q + 1);
		dyncols[i].column = p;
		p += strlen(p) + 1;
	    } else {
		dyncols[i].table = "";
		strcpy(p, cols[i]);
		dyncols[i].column = p;
		p += strlen(p) + 1;
	    }
	    if (s->longnames) {
		dyncols[i].column = dyncols[i].label;
	    }
#ifdef SQL_LONGVARCHAR
	    dyncols[i].type = SQL_LONGVARCHAR;
	    dyncols[i].size = 65535;
#else
	    dyncols[i].type = SQL_VARCHAR;
	    dyncols[i].size = 255;
#endif
	    dyncols[i].index = i;
	    dyncols[i].scale = 0;
	    dyncols[i].prec = 0;
	    dyncols[i].nosign = 1;
	    dyncols[i].autoinc = SQL_FALSE;
	    dyncols[i].notnull = SQL_NULLABLE;
	    dyncols[i].typename = NULL;
	}
	freedyncols(s);
	s->dyncols = s->cols = dyncols;
	s->dcols = ncols;
	fixupdyncols(s, d->sqlite, (const char **) cols + ncols);
    }
    s->ncols = ncols;
    return 1;
}

/**
 * Internal query preparation used by SQLPrepare() and SQLExecDirect().
 * @param stmt statement handle
 * @param query query string
 * @param queryLen length of query string or SQL_NTS
 * @result ODBC error code
 */

static SQLRETURN
drvprepare(SQLHSTMT stmt, SQLCHAR *query, SQLINTEGER queryLen)
{
    STMT *s;
    DBC *d;
    char *errp = NULL;
    SQLRETURN sret;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (s->dbc == SQL_NULL_HDBC) {
noconn:
	return noconn(s);
    }
    d = s->dbc;
    if (!d->sqlite) {
	goto noconn;
    }
    vm_end(s);
    sret = starttran(s);
    if (sret != SQL_SUCCESS) {
	return sret;
    }
    freep(&s->query);
    s->query = (SQLCHAR *) fixupsql((char *) query, queryLen, &s->nparams,
				    &s->isselect, &errp,
				    d->version, &s->parmnames);
    if (!s->query) {
	if (errp) {
	    setstat(s, -1, "%s", (*s->ov3) ? "HY000" : "S1000", errp);
	    return SQL_ERROR;
	}
	return nomem(s);
    }
#ifdef CANT_PASS_VALIST_AS_CHARPTR
    if (s->nparams > MAX_PARAMS_FOR_VPRINTF) {
	freep(&s->query);
	setstat(s, -1, "too much parameters in query",
		(*s->ov3) ? "HY000" : "S1000");
	return SQL_ERROR;
    }
#endif
    errp = NULL;
    freeresult(s, -1);
    if (s->isselect == 1) {
	int ret;
	char **params = NULL;

	if (s->nparams) {
	    int i, maxp;

#ifdef CANT_PASS_VALIST_AS_CHARPTR
	    maxp = MAX_PARAMS_FOR_VPRINTF;
#else
	    maxp = s->nparams;
#endif
	    params = xmalloc(maxp * sizeof (char *));
	    if (!params) {
		return nomem(s);
	    }
	    for (i = 0; i < maxp; i++) {
		params[i] = NULL;
	    }
	}
#ifdef CANT_PASS_VALIST_AS_CHARPTR
	if (params) {
	    ret = sqlite_exec_printf(d->sqlite, (char *) s->query, selcb, s,
				     &errp,
				     params[0], params[1],
				     params[2], params[3],
				     params[4], params[5],
				     params[6], params[7],
				     params[8], params[9],
				     params[10], params[11],
				     params[12], params[13],
				     params[14], params[15],
				     params[16], params[17],
				     params[18], params[19],
				     params[20], params[21],
				     params[22], params[23],
				     params[24], params[25],
				     params[26], params[27],
				     params[28], params[29],
				     params[30], params[31]);
	} else {
	    ret = sqlite_exec_printf(d->sqlite, (char *) s->query, selcb,
				     s, &errp);
	}
#else
	ret = sqlite_exec_vprintf(d->sqlite, (char *) s->query, selcb, s,
				  &errp, (char *) params);
#endif
	if (ret != SQLITE_ABORT && ret != SQLITE_OK) {
	    dbtracerc(d, ret, errp);
	    freep(&params);
	    setstat(s, ret, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		    errp ? errp : "unknown error", ret);
	    if (errp) {
		sqlite_freemem(errp);
		errp = NULL;
	    }
	    return SQL_ERROR;
	}
	freep(&params);
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
    }
    mkbindcols(s, s->ncols);
    s->paramset_count = 0;
    return SQL_SUCCESS;
}

/**
 * Internal query execution used by SQLExecute() and SQLExecDirect().
 * @param stmt statement handle
 * @param initial false when called from SQLPutData()
 * @result ODBC error code
 */

static SQLRETURN
drvexecute(SQLHSTMT stmt, int initial)
{
    STMT *s;
    DBC *d;
    char *errp = NULL, **params = NULL;
    int rc, i, ncols;
    SQLRETURN ret;

    if (stmt == SQL_NULL_HSTMT) {
	return SQL_INVALID_HANDLE;
    }
    s = (STMT *) stmt;
    if (s->dbc == SQL_NULL_HDBC) {
noconn:
	return noconn(s);
    }
    d = (DBC *) s->dbc;
    if (!d->sqlite) {
	goto noconn;
    }
    if (!s->query) {
	setstat(s, -1, "no query prepared", (*s->ov3) ? "HY000" : "S1000");
	return SQL_ERROR;
    }
    if (s->nbindparms < s->nparams) {
unbound:
	setstat(s, -1, "unbound parameters in query",
		(*s->ov3) ? "HY000" : "S1000");
	return SQL_ERROR;
    }
    for (i = 0; i < s->nparams; i++) {
	BINDPARM *p = &s->bindparms[i];

	if (!p->bound) {
	    goto unbound;
	}
	if (initial) {
	    SQLLEN *lenp = p->lenp;

	    if (lenp && *lenp < 0 && *lenp > SQL_LEN_DATA_AT_EXEC_OFFSET &&
		*lenp != SQL_NTS && *lenp != SQL_NULL_DATA &&
		*lenp != SQL_DATA_AT_EXEC) {
		setstat(s, -1, "invalid length reference", "HY009");
		return SQL_ERROR;
	    }
	    if (lenp && (*lenp <= SQL_LEN_DATA_AT_EXEC_OFFSET ||
			 *lenp == SQL_DATA_AT_EXEC)) {
		p->need = 1;
		p->offs = 0;
		p->len = 0;
	    }
	}
    }
    ret = starttran(s);
    if (ret != SQL_SUCCESS) {
	goto cleanup;
    }
again:
    vm_end(s);
    if (initial) {
	/* fixup data-at-execution parameters and alloc'ed blobs */
	s->pdcount = -1;
	for (i = 0; i < s->nparams; i++) {
	    BINDPARM *p = &s->bindparms[i];

	    if (p->param == p->parbuf) {
		p->param = NULL;
	    }
	    freep(&p->parbuf);
	    if (p->need <= 0 &&
		p->lenp && (*p->lenp <= SQL_LEN_DATA_AT_EXEC_OFFSET ||
			    *p->lenp == SQL_DATA_AT_EXEC)) {
		p->need = 1;
		p->offs = 0;
		p->len = 0;
	    }
	}
    }
    if (s->nparams) {
	int maxp;

#ifdef CANT_PASS_VALIST_AS_CHARPTR
	maxp = MAX_PARAMS_FOR_VPRINTF;
#else
	maxp = s->nparams;
#endif
	params = xmalloc(maxp * sizeof (char *));
	if (!params) {
	    ret = nomem(s);
	    goto cleanup;
	}
	for (i = 0; i < maxp; i++) {
	    params[i] = NULL;
	}	    
	for (i = 0; i < s->nparams; i++) {
	    ret = substparam(s, i, &params[i]);
	    if (ret != SQL_SUCCESS) {
		freep(&params);
		goto cleanup;
	    }
	}
    }
    freeresult(s, 0);
    if (s->isselect == 1 && !d->intrans &&
	s->curtype == SQL_CURSOR_FORWARD_ONLY &&
	d->step_enable && s->nparams == 0 && d->vm_stmt == NULL) {
	s->nrows = -1;
	ret = vm_start(s, params);
	if (ret == SQL_SUCCESS) {
	    freep(&params);
	    goto done2;
	}
    }
#ifdef CANT_PASS_VALIST_AS_CHARPTR
    if (params) {
	rc = sqlite_get_table_printf(d->sqlite, (char *) s->query, &s->rows,
				     &s->nrows, &ncols, &errp,
				     params[0], params[1],
				     params[2], params[3],
				     params[4], params[5],
				     params[6], params[7],
				     params[8], params[9],
				     params[10], params[11],
				     params[12], params[13],
				     params[14], params[15],
				     params[16], params[17],
				     params[18], params[19],
				     params[20], params[21],
				     params[22], params[23],
				     params[24], params[25],
				     params[26], params[27],
				     params[28], params[29],
				     params[30], params[31]);
    } else {
	rc = sqlite_get_table_printf(d->sqlite, (char *) s->query, &s->rows,
				     &s->nrows, &ncols, &errp);
    }
#else
    rc = sqlite_get_table_vprintf(d->sqlite, (char *) s->query, &s->rows,
				  &s->nrows, &ncols, &errp, (char *) params);
#endif
    dbtracerc(d, rc, errp);
    if (rc != SQLITE_OK) {
	freep(&params);
	setstat(s, rc, "%s (%d)", (*s->ov3) ? "HY000" : "S1000",
		errp ? errp : "unknown error", rc);
	if (errp) {
	    sqlite_freemem(errp);
	    errp = NULL;
	}
	ret = SQL_ERROR;
	goto cleanup;
    }
    freep(&params);
    if (errp) {
	sqlite_freemem(errp);
	errp = NULL;
    }
    s->rowfree = sqlite_free_table;
    if (ncols == 1 && (s->isselect <= 0 || s->isselect > 1)) {
	/*
	 * INSERT/UPDATE/DELETE or DDL results are immediately released.
	 */
	if (strcmp(s->rows[0], "rows inserted") == 0 ||
	    strcmp(s->rows[0], "rows updated") == 0 ||
	    strcmp(s->rows[0], "rows deleted") == 0) {
	    int nrows = 0;
	   
	    nrows = strtol(s->rows[1], NULL, 0);
	    freeresult(s, -1);
	    s->nrows = nrows;
	    goto done;
	}
    }
    if (s->ncols != ncols) {
	int size;
	char *p;
	COL *dyncols;

	for (i = size = 0; i < ncols; i++) {
	    size += 3 + 3 * strlen(s->rows[i]);
	}
	if (size == 0) {
	    freeresult(s, -1);
	    goto done;
	}
	dyncols = xmalloc(ncols * sizeof (COL) + size);
	if (!dyncols) {
	    ret = nomem(s);
	    goto cleanup;
	}
	p = (char *) (dyncols + ncols);
	for (i = 0; i < ncols; i++) {
	    char *q;

	    dyncols[i].db = d->dbname;
	    strcpy(p, s->rows[i]);
	    dyncols[i].label = p;
	    p += strlen(p) + 1;
	    q = strchr(s->rows[i], '.');
	    if (q) {
		dyncols[i].table = p;
		strncpy(p, s->rows[i], q - s->rows[i]);
		p[q - s->rows[i]] = '\0';
		p += strlen(p) + 1;
		dyncols[i].column = q + 1;
	    } else {
		dyncols[i].table = "";
		dyncols[i].column = s->rows[i];
	    }
	    if (s->longnames) {
		dyncols[i].column = dyncols[i].label;
	    }
#ifdef SQL_LONGVARCHAR
	    dyncols[i].type = SQL_LONGVARCHAR;
	    dyncols[i].size = 65536;
#else
	    dyncols[i].type = SQL_VARCHAR;
	    dyncols[i].size = 255;
#endif
	    dyncols[i].index = i;
	    dyncols[i].scale = 0;
	    dyncols[i].prec = 0;
	    dyncols[i].nosign = 1;
	    dyncols[i].autoinc = SQL_FALSE;
	    dyncols[i].notnull = SQL_NULLABLE;
	    dyncols[i].typename = NULL;
	}
	freedyncols(s);
	s->ncols = s->dcols = ncols;
	s->dyncols = s->cols = dyncols;
	fixupdyncols(s, d->sqlite, NULL);
    }
done:
    mkbindcols(s, s->ncols);
done2:
    ret = SQL_SUCCESS;
    s->rowp = -1;
    s->paramset_count++;
    s->paramset_nrows += s->nrows;
    if (s->paramset_count < s->paramset_size) {
	for (i = 0; i < s->nparams; i++) {
	    BINDPARM *p = &s->bindparms[i];

	    if (p->param == p->parbuf) {
		p->param = NULL;
	    }
	    freep(&p->parbuf);
	    if (p->lenp0 &&
		s->parm_bind_type != SQL_PARAM_BIND_BY_COLUMN) {
		p->lenp = (SQLLEN *) ((char *) p->lenp0 +
				      s->paramset_count * s->parm_bind_type);
	    } else if (p->lenp0 && p->inc > 0) {
		p->lenp = p->lenp0 + s->paramset_count;
	    }
	    if (!p->lenp || (*p->lenp > SQL_LEN_DATA_AT_EXEC_OFFSET &&
			     *p->lenp != SQL_DATA_AT_EXEC)) {
		if (p->param0 &&
		    s->parm_bind_type != SQL_PARAM_BIND_BY_COLUMN) {
		    p->param = (char *) p->param0 +
			s->paramset_count * s->parm_bind_type;
		} else if (p->param0 && p->inc > 0) {
		    p->param = (char *) p->param0 +
			s->paramset_count * p->inc;
		}
	    } else if (p->lenp && (*p->lenp <= SQL_LEN_DATA_AT_EXEC_OFFSET ||
				   *p->lenp == SQL_DATA_AT_EXEC)) {
		p->need = 1;
		p->offs = 0;
		p->len = 0;
	    }
	}
	goto again;
    }
cleanup:
    if (ret != SQL_NEED_DATA) {
	for (i = 0; i < s->nparams; i++) {
	    BINDPARM *p = &s->bindparms[i];

	    if (p->param == p->parbuf) {
		p->param = NULL;
	    }
	    freep(&p->parbuf);
	    if (!p->lenp || (*p->lenp > SQL_LEN_DATA_AT_EXEC_OFFSET &&
			     *p->lenp != SQL_DATA_AT_EXEC)) {
		p->param = p->param0;
	    }
	    p->lenp = p->lenp0;
	}
	s->nrows = s->paramset_nrows;
	if (s->parm_proc) {
	    *s->parm_proc = s->paramset_count;
	}
	s->paramset_count = 0;
	s->paramset_nrows = 0;
    }
    return ret;
}

#ifndef WINTERFACE
/**
 * Prepare HSTMT.
 * @param stmt statement handle
 * @param query query string
 * @param queryLen length of query string or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLPrepare(SQLHSTMT stmt, SQLCHAR *query, SQLINTEGER queryLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvprepare(stmt, query, queryLen);
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Prepare HSTMT (UNICODE version).
 * @param stmt statement handle
 * @param query query string
 * @param queryLen length of query string or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLPrepareW(SQLHSTMT stmt, SQLWCHAR *query, SQLINTEGER queryLen)
{
    SQLRETURN ret;
    char *q = uc_to_utf_c(query, queryLen);

    HSTMT_LOCK(stmt);
    if (!q) {
	ret = nomem((STMT *) stmt);
	goto done;
    }
    ret = drvprepare(stmt, (SQLCHAR *) q, SQL_NTS);
    uc_free(q);
done:
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

/**
 * Execute query.
 * @param stmt statement handle
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLExecute(SQLHSTMT stmt)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvexecute(stmt, 1);
    HSTMT_UNLOCK(stmt);
    return ret;
}

#ifndef WINTERFACE
/**
 * Execute query directly.
 * @param stmt statement handle
 * @param query query string
 * @param queryLen length of query string or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLExecDirect(SQLHSTMT stmt, SQLCHAR *query, SQLINTEGER queryLen)
{
    SQLRETURN ret;

    HSTMT_LOCK(stmt);
    ret = drvprepare(stmt, query, queryLen);
    if (ret == SQL_SUCCESS) {
	ret = drvexecute(stmt, 1);
    }
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Execute query directly (UNICODE version).
 * @param stmt statement handle
 * @param query query string
 * @param queryLen length of query string or SQL_NTS
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLExecDirectW(SQLHSTMT stmt, SQLWCHAR *query, SQLINTEGER queryLen)
{
    SQLRETURN ret;
    char *q = uc_to_utf_c(query, queryLen);

    HSTMT_LOCK(stmt);
    if (!q) {
	ret = nomem((STMT *) stmt);
	goto done;
    }
    ret = drvprepare(stmt, (SQLCHAR *) q, SQL_NTS);
    uc_free(q);
    if (ret == SQL_SUCCESS) {
	ret = drvexecute(stmt, 1);
    }
done:
    HSTMT_UNLOCK(stmt);
    return ret;
}
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__OS2__)

#ifndef WITHOUT_DRIVERMGR

#if defined(_WIN32) || defined(_WIN64)
/*
 * Windows configuration dialog stuff.
 */

#include <windowsx.h>
#include <winuser.h>

#endif

#ifdef __OS2__
#define INCL_WIN
#define INCL_PM
#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS
#define INCL_WINSTDFILE

#include <os2.h>
#include <stdlib.h>

#include "resourceos2.h"

#endif

#define MAXPATHLEN      (259+1)           /* Max path length */
#define MAXKEYLEN       (15+1)            /* Max keyword length */
#define MAXDESC         (255+1)           /* Max description length */
#define MAXDSNAME       (32+1)            /* Max data source name length */
#define MAXTONAME       (32+1)            /* Max timeout length */
#define MAXDBNAME       MAXPATHLEN

/* Attribute key indexes into an array of Attr structs, see below */

#define KEY_DSN 		0
#define KEY_DESC		1
#define KEY_DBNAME		2
#define KEY_BUSY		3
#define KEY_DRIVER		4
#define KEY_NOWCHAR		5
#define KEY_STEPAPI		6
#define KEY_NOTXN		7
#define KEY_LONGNAM		8
#define NUMOFKEYS		9

typedef struct {
    BOOL supplied;
    char attr[MAXPATHLEN];
} ATTR;

typedef struct {
    SQLHWND parent;
#ifdef __OS2__
    HMODULE hDLL;
#endif
    LPCSTR  driver;
    ATTR    attr[NUMOFKEYS];
    char    DSN[MAXDSNAME];
    BOOL    newDSN;
    BOOL    defDSN;
} SETUPDLG;

static struct {
    char *key;
    int ikey;
} attrLookup[] = {
    { "DSN", KEY_DSN },
    { "DESC", KEY_DESC },
    { "Description", KEY_DESC},
    { "Database", KEY_DBNAME },
    { "Timeout", KEY_BUSY },
    { "Driver", KEY_DRIVER },
    { "NoWCHAR", KEY_NOWCHAR },
    { "StepAPI", KEY_STEPAPI },
    { "NoTXN", KEY_NOTXN },
    { "LongNames", KEY_LONGNAM },
    { NULL, 0 }
};

/**
 * Setup dialog data from datasource attributes.
 * @param attribs attribute string
 * @param setupdlg pointer to dialog data
 */

static void
ParseAttributes(LPCSTR attribs, SETUPDLG *setupdlg)
{
    char *str = (char *) attribs, *start, key[MAXKEYLEN];
    int elem, nkey;

    while (*str) {
	start = str;
	if ((str = strchr(str, '=')) == NULL) {
	    return;
	}
	elem = -1;
	nkey = str - start;
	if (nkey < sizeof (key)) {
	    int i;

	    memcpy(key, start, nkey);
	    key[nkey] = '\0';
	    for (i = 0; attrLookup[i].key; i++) {
		if (strcasecmp(attrLookup[i].key, key) == 0) {
		    elem = attrLookup[i].ikey;
		    break;
		}
	    }
	}
	start = ++str;
	while (*str && *str != ';') {
	    ++str;
	}
	if (elem >= 0) {
	    int end = min(str - start, sizeof (setupdlg->attr[elem].attr) - 1);

	    setupdlg->attr[elem].supplied = TRUE;
	    memcpy(setupdlg->attr[elem].attr, start, end);
	    setupdlg->attr[elem].attr[end] = '\0';
	}
	++str;
    }
}

/**
 * Set datasource attributes in registry.
 * @param parent handle of parent window
 * @param setupdlg pointer to dialog data
 * @result true or false
 */

static BOOL
SetDSNAttributes(HWND parent, SETUPDLG *setupdlg)
{
    char *dsn = setupdlg->attr[KEY_DSN].attr;

    if (setupdlg->newDSN && strlen(dsn) == 0) {
	return FALSE;
    }
    if (!SQLWriteDSNToIni(dsn, setupdlg->driver)) {
	if (parent) {
#if defined(_WIN32) || defined(_WIN64)
	    char buf[MAXPATHLEN], msg[MAXPATHLEN];

	    LoadString(hModule, IDS_BADDSN, buf, sizeof (buf));
	    wsprintf(msg, buf, dsn);
	    LoadString(hModule, IDS_MSGTITLE, buf, sizeof (buf));
	    MessageBox(parent, msg, buf,
		       MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL |
		       MB_SETFOREGROUND);
#endif
#ifdef __OS2__
	    WinMessageBox(HWND_DESKTOP, parent, "Bad DSN", "Configure Error",
			  0, MB_ERROR | MB_OK );
#endif
	}
	return FALSE;
    }
    if (parent || setupdlg->attr[KEY_DESC].supplied) {
	SQLWritePrivateProfileString(dsn, "Description",
				     setupdlg->attr[KEY_DESC].attr,
				     ODBC_INI);
    }
    if (parent || setupdlg->attr[KEY_DBNAME].supplied) {
	SQLWritePrivateProfileString(dsn, "Database",
				     setupdlg->attr[KEY_DBNAME].attr,
				     ODBC_INI);
    }
    if (parent || setupdlg->attr[KEY_BUSY].supplied) {
	SQLWritePrivateProfileString(dsn, "Timeout",
				     setupdlg->attr[KEY_BUSY].attr,
				     ODBC_INI);
    }
    if (parent || setupdlg->attr[KEY_NOWCHAR].supplied) {
	SQLWritePrivateProfileString(dsn, "NoWCHAR",
				     setupdlg->attr[KEY_NOWCHAR].attr,
				     ODBC_INI);
    }
    if (parent || setupdlg->attr[KEY_STEPAPI].supplied) {
	SQLWritePrivateProfileString(dsn, "StepAPI",
				     setupdlg->attr[KEY_STEPAPI].attr,
				     ODBC_INI);
    }
    if (parent || setupdlg->attr[KEY_NOTXN].supplied) {
	SQLWritePrivateProfileString(dsn, "NoTXN",
				     setupdlg->attr[KEY_NOTXN].attr,
				     ODBC_INI);
    }
    if (parent || setupdlg->attr[KEY_LONGNAM].supplied) {
	SQLWritePrivateProfileString(dsn, "LongNames",
				     setupdlg->attr[KEY_LONGNAM].attr,
				     ODBC_INI);
    }
    if (setupdlg->attr[KEY_DSN].supplied &&
	strcasecmp(setupdlg->DSN, setupdlg->attr[KEY_DSN].attr)) {
	SQLRemoveDSNFromIni(setupdlg->DSN);
    }
    return TRUE;
}

/**
 * Get datasource attributes from registry.
 * @param setupdlg pointer to dialog data
 */

static void
GetAttributes(SETUPDLG *setupdlg)
{
    char *dsn = setupdlg->attr[KEY_DSN].attr;

    if (!setupdlg->attr[KEY_DESC].supplied) {
	SQLGetPrivateProfileString(dsn, "Description", "",
				   setupdlg->attr[KEY_DESC].attr,
				   sizeof (setupdlg->attr[KEY_DESC].attr),
				   ODBC_INI);
    }
    if (!setupdlg->attr[KEY_DBNAME].supplied) {
	SQLGetPrivateProfileString(dsn, "Database", "",
				   setupdlg->attr[KEY_DBNAME].attr,
				   sizeof (setupdlg->attr[KEY_DBNAME].attr),
				   ODBC_INI);
    }
    if (!setupdlg->attr[KEY_BUSY].supplied) {
	SQLGetPrivateProfileString(dsn, "Timeout", "100000",
				   setupdlg->attr[KEY_BUSY].attr,
				   sizeof (setupdlg->attr[KEY_BUSY].attr),
				   ODBC_INI);
    }
    if (!setupdlg->attr[KEY_NOWCHAR].supplied) {
	SQLGetPrivateProfileString(dsn, "NoWCHAR", "",
				   setupdlg->attr[KEY_NOWCHAR].attr,
				   sizeof (setupdlg->attr[KEY_NOWCHAR].attr),
				   ODBC_INI);
    }
    if (!setupdlg->attr[KEY_STEPAPI].supplied) {
	SQLGetPrivateProfileString(dsn, "StepAPI", "0",
				   setupdlg->attr[KEY_STEPAPI].attr,
				   sizeof (setupdlg->attr[KEY_STEPAPI].attr),
				   ODBC_INI);
    }
    if (!setupdlg->attr[KEY_NOTXN].supplied) {
	SQLGetPrivateProfileString(dsn, "NoTXN", "",
				   setupdlg->attr[KEY_NOTXN].attr,
				   sizeof (setupdlg->attr[KEY_NOTXN].attr),
				   ODBC_INI);
    }
    if (!setupdlg->attr[KEY_LONGNAM].supplied) {
	SQLGetPrivateProfileString(dsn, "LongNames", "",
				   setupdlg->attr[KEY_LONGNAM].attr,
				   sizeof (setupdlg->attr[KEY_LONGNAM].attr),
				   ODBC_INI);
    }
}

/**
 * Open file dialog for selection of SQLite database file.
 * @param hdlg handle of originating dialog window
 */

static void
GetDBFile(HWND hdlg)
{
#if defined(_WIN32) || defined(_WIN64)
#ifdef _WIN64
    SETUPDLG *setupdlg = (SETUPDLG *) GetWindowLongPtr(hdlg, DWLP_USER);
#else
    SETUPDLG *setupdlg = (SETUPDLG *) GetWindowLong(hdlg, DWL_USER);
#endif
    OPENFILENAME ofn;

    memset(&ofn, 0, sizeof (ofn));
    ofn.lStructSize = sizeof (ofn);
    ofn.hwndOwner = hdlg;
#ifdef _WIN64
    ofn.hInstance = (HINSTANCE) GetWindowLongPtr(hdlg, GWLP_HINSTANCE);
#else
    ofn.hInstance = (HINSTANCE) GetWindowLong(hdlg, GWL_HINSTANCE);
#endif
    ofn.lpstrFile = (LPTSTR) setupdlg->attr[KEY_DBNAME].attr;
    ofn.nMaxFile = MAXPATHLEN;
    ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST |
		OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn)) {
	SetDlgItemText(hdlg, IDC_DBNAME, setupdlg->attr[KEY_DBNAME].attr);
	setupdlg->attr[KEY_DBNAME].supplied = TRUE;
    }
#endif
#ifdef __OS2__
    SETUPDLG *setupdlg = (SETUPDLG *) WinQueryWindowULong(hdlg, QWL_USER);
    FILEDLG ofn;

    memset(&ofn, 0, sizeof (ofn));
    ofn.cbSize = sizeof (FILEDLG);
    strcpy(ofn.szFullFile, setupdlg->attr[KEY_DBNAME].attr);
    ofn.fl = FDS_OPEN_DIALOG | FDS_CENTER;
    WinFileDlg(hdlg, hdlg, &ofn);
    if (ofn.lReturn == DID_OK) {
	strcpy(setupdlg->attr[KEY_DBNAME].attr, ofn.szFullFile);
	WinSetDlgItemText(hdlg, EF_DATABASE, setupdlg->attr[KEY_DBNAME].attr);
	setupdlg->attr[KEY_DBNAME].supplied = TRUE;
    }
#endif
}

#if defined(_WIN32) || defined(_WIN64)
/**
 * Dialog procedure for ConfigDSN().
 * @param hdlg handle of dialog window
 * @param wmsg type of message
 * @param wparam wparam of message
 * @param lparam lparam of message
 * @result true or false
 */

static BOOL CALLBACK
ConfigDlgProc(HWND hdlg, WORD wmsg, WPARAM wparam, LPARAM lparam)
{
    SETUPDLG *setupdlg = NULL;

    switch (wmsg) {
    case WM_INITDIALOG:
#ifdef _WIN64
	SetWindowLongPtr(hdlg, DWLP_USER, lparam);
#else
	SetWindowLong(hdlg, DWL_USER, lparam);
#endif
	setupdlg = (SETUPDLG *) lparam;
	GetAttributes(setupdlg);
	SetDlgItemText(hdlg, IDC_DSNAME, setupdlg->attr[KEY_DSN].attr);
	SetDlgItemText(hdlg, IDC_DESC, setupdlg->attr[KEY_DESC].attr);
	SetDlgItemText(hdlg, IDC_DBNAME, setupdlg->attr[KEY_DBNAME].attr);
	SetDlgItemText(hdlg, IDC_TONAME, setupdlg->attr[KEY_BUSY].attr);
	SendDlgItemMessage(hdlg, IDC_DSNAME, EM_LIMITTEXT,
			   (WPARAM) (MAXDSNAME - 1), (LPARAM) 0);
	SendDlgItemMessage(hdlg, IDC_DESC, EM_LIMITTEXT,
			   (WPARAM) (MAXDESC - 1), (LPARAM) 0);
	SendDlgItemMessage(hdlg, IDC_DBNAME, EM_LIMITTEXT,
			   (WPARAM) (MAXDBNAME - 1), (LPARAM) 0);
	SendDlgItemMessage(hdlg, IDC_TONAME, EM_LIMITTEXT,
			   (WPARAM) (MAXTONAME - 1), (LPARAM) 0);
	CheckDlgButton(hdlg, IDC_NOWCHAR,
		       getbool(setupdlg->attr[KEY_NOWCHAR].attr) ?
		       BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hdlg, IDC_STEPAPI,
		       getbool(setupdlg->attr[KEY_STEPAPI].attr) ?
		       BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hdlg, IDC_NOTXN,
		       getbool(setupdlg->attr[KEY_NOTXN].attr) ?
		       BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hdlg, IDC_LONGNAM,
		       getbool(setupdlg->attr[KEY_LONGNAM].attr) ?
		       BST_CHECKED : BST_UNCHECKED);
	if (setupdlg->defDSN) {
	    EnableWindow(GetDlgItem(hdlg, IDC_DSNAME), FALSE);
	    EnableWindow(GetDlgItem(hdlg, IDC_DSNAMETEXT), FALSE);
	}
	return TRUE;
    case WM_COMMAND:
	switch (GET_WM_COMMAND_ID(wparam, lparam)) {
	case IDC_DSNAME:
	    if (GET_WM_COMMAND_CMD(wparam, lparam) == EN_CHANGE) {
		char item[MAXDSNAME];

		EnableWindow(GetDlgItem(hdlg, IDOK),
			     GetDlgItemText(hdlg, IDC_DSNAME,
					    item, sizeof (item)));
		return TRUE;
	    }
	    break;
	case IDC_BROWSE:
	    GetDBFile(hdlg);
	    break;
	case IDOK:
#ifdef _WIN64
	    setupdlg = (SETUPDLG *) GetWindowLongPtr(hdlg, DWLP_USER);
#else
	    setupdlg = (SETUPDLG *) GetWindowLong(hdlg, DWL_USER);
#endif
	    if (!setupdlg->defDSN) {
		GetDlgItemText(hdlg, IDC_DSNAME,
			       setupdlg->attr[KEY_DSN].attr,
			       sizeof (setupdlg->attr[KEY_DSN].attr));
	    }
	    GetDlgItemText(hdlg, IDC_DESC,
			   setupdlg->attr[KEY_DESC].attr,
			   sizeof (setupdlg->attr[KEY_DESC].attr));
	    GetDlgItemText(hdlg, IDC_DBNAME,
			   setupdlg->attr[KEY_DBNAME].attr,
			   sizeof (setupdlg->attr[KEY_DBNAME].attr));
	    GetDlgItemText(hdlg, IDC_TONAME,
			   setupdlg->attr[KEY_BUSY].attr,
			   sizeof (setupdlg->attr[KEY_BUSY].attr));
	    strcpy(setupdlg->attr[KEY_NOWCHAR].attr,
		   (IsDlgButtonChecked(hdlg, IDC_NOWCHAR) == BST_CHECKED) ?
		   "1" : "0");
	    strcpy(setupdlg->attr[KEY_STEPAPI].attr,
		   (IsDlgButtonChecked(hdlg, IDC_STEPAPI) == BST_CHECKED) ?
		   "1" : "0");
	    strcpy(setupdlg->attr[KEY_NOTXN].attr,
		   (IsDlgButtonChecked(hdlg, IDC_NOTXN) == BST_CHECKED) ?
		   "1" : "0");
	    strcpy(setupdlg->attr[KEY_LONGNAM].attr,
		   (IsDlgButtonChecked(hdlg, IDC_LONGNAM) == BST_CHECKED) ?
		   "1" : "0");
	    SetDSNAttributes(hdlg, setupdlg);
	    /* FALL THROUGH */
	case IDCANCEL:
	    EndDialog(hdlg, wparam);
	    return TRUE;
	}
	break;
    }
    return FALSE;
}
#endif

#ifdef __OS2__
/**
 * Dialog procedure for ConfigDSN().
 * @param hdlg handle of dialog window
 * @param wmsg type of message
 * @param wparam wparam of message
 * @param lparam lparam of message
 * @result standard MRESULT
 */

static MRESULT EXPENTRY
ConfigDlgProc(HWND hdlg, ULONG wmsg, MPARAM wparam, MPARAM lparam)
{
    SETUPDLG *setupdlg = NULL;

    switch (wmsg) {
    case WM_INITDLG:
	WinSetWindowULong(hdlg, QWL_USER, (ULONG) lparam);
	setupdlg = (SETUPDLG *) lparam;
	GetAttributes(setupdlg);
	WinSetDlgItemText(hdlg, EF_DSNNAME, setupdlg->attr[KEY_DSN].attr);
	WinSetDlgItemText(hdlg, EF_DSNDESC, setupdlg->attr[KEY_DESC].attr);
	WinSetDlgItemText(hdlg, EF_DATABASE, setupdlg->attr[KEY_DBNAME].attr);
	WinSetDlgItemText(hdlg, EF_TIMEOUT, setupdlg->attr[KEY_BUSY].attr);
	WinSendDlgItemMsg(hdlg, EF_DSNNAME, EM_SETTEXTLIMIT,
			  MPFROMSHORT(MAXDSNAME - 1), 0L);
	WinSendDlgItemMsg(hdlg, EF_DSNDESC, EM_SETTEXTLIMIT,
			  MPFROMSHORT(MAXDESC - 1), 0L);
	WinSendDlgItemMsg(hdlg, EF_DATABASE, EM_SETTEXTLIMIT,
			  MPFROMSHORT(MAXDBNAME - 1), 0L);
	WinSendDlgItemMsg(hdlg, EF_TIMEOUT, EM_SETTEXTLIMIT,
			  MPFROMSHORT(MAXTONAME - 1), 0L);
	WinCheckButton(hdlg, IDC_NOWCHAR,
		       getbool(setupdlg->attr[KEY_NOWCHAR].attr));
	WinCheckButton(hdlg, IDC_STEPAPI,
		       getbool(setupdlg->attr[KEY_STEPAPI].attr));
	WinCheckButton(hdlg, IDC_NOTXN,
		       getbool(setupdlg->attr[KEY_NOTXN].attr));
	WinCheckButton(hdlg, IDC_LONGNAM,
		       getbool(setupdlg->attr[KEY_LONGNAM].attr));
	if (setupdlg->defDSN) {
	    WinEnableWindow(WinWindowFromID(hdlg, EF_DSNNAME), FALSE);
	}
	return 0;
    case WM_COMMAND:
	switch (SHORT1FROMMP(wparam)) {
	case IDC_BROWSE:
	    GetDBFile(hdlg);
	    break;
	case DID_OK:
	    setupdlg = (SETUPDLG *) WinQueryWindowULong(hdlg, QWL_USER);
	    if (!setupdlg->defDSN) {
		WinQueryDlgItemText(hdlg, EF_DSNNAME,
				    sizeof (setupdlg->attr[KEY_DSN].attr),
				    setupdlg->attr[KEY_DSN].attr);
	    }
	    WinQueryDlgItemText(hdlg, EF_DSNDESC,
				sizeof (setupdlg->attr[KEY_DESC].attr),
				setupdlg->attr[KEY_DESC].attr);
	    WinQueryDlgItemText(hdlg, EF_DATABASE,
				sizeof (setupdlg->attr[KEY_DBNAME].attr),
				setupdlg->attr[KEY_DBNAME].attr);
	    WinQueryDlgItemText(hdlg, EF_TIMEOUT,
				sizeof (setupdlg->attr[KEY_BUSY].attr),
				setupdlg->attr[KEY_BUSY].attr);
	    if ((ULONG)WinSendDlgItemMsg(hdlg, IDC_NOWCHAR, BM_QUERYCHECK,
					 0, 0) == 1) {
		strcpy(setupdlg->attr[KEY_NOWCHAR].attr, "1");
	    } else {
		strcpy(setupdlg->attr[KEY_NOWCHAR].attr, "0");
	    }
	    if ((ULONG)WinSendDlgItemMsg(hdlg, IDC_STEPAPI, BM_QUERYCHECK,
					 0, 0) == 1) {
		strcpy(setupdlg->attr[KEY_STEPAPI].attr, "1");
	    } else {
		strcpy(setupdlg->attr[KEY_STEPAPI].attr, "0");
	    }
	    if ((ULONG)WinSendDlgItemMsg(hdlg, IDC_NOTXN, BM_QUERYCHECK,
					 0, 0) == 1) {
		strcpy(setupdlg->attr[KEY_NOTXN].attr, "1");
	    } else {
		strcpy(setupdlg->attr[KEY_NOTXN].attr, "0");
	    }
	    if ((ULONG)WinSendDlgItemMsg(hdlg, IDC_LONGNAM, BM_QUERYCHECK,
					 0, 0) == 1) {
		strcpy(setupdlg->attr[KEY_LONGNAM].attr, "1");
	    } else {
		strcpy(setupdlg->attr[KEY_LONGNAM].attr, "0");
	    }
	    SetDSNAttributes(hdlg, setupdlg);
	    WinDismissDlg(hdlg, DID_OK);
	    return 0;
	case DID_CANCEL:
	    WinDismissDlg(hdlg, DID_OK);
	    return 0;
	}
	break;
    }
    return WinDefDlgProc(hdlg, wmsg, wparam, lparam);
}
#endif

/**
 * ODBC INSTAPI procedure for DSN configuration.
 * @param hwnd parent window handle
 * @param request type of request
 * @param driver driver name
 * @param attribs attribute string of DSN
 * @result true or false
 */

BOOL INSTAPI
ConfigDSN(HWND hwnd, WORD request, LPCSTR driver, LPCSTR attribs)
{
    BOOL success;
    SETUPDLG *setupdlg;

    setupdlg = (SETUPDLG *) xmalloc(sizeof (SETUPDLG));
    if (setupdlg == NULL) {
	return FALSE;
    }
    memset(setupdlg, 0, sizeof (SETUPDLG));
    if (attribs) {
	ParseAttributes(attribs, setupdlg);
    }
    if (setupdlg->attr[KEY_DSN].supplied) {
	strcpy(setupdlg->DSN, setupdlg->attr[KEY_DSN].attr);
    } else {
	setupdlg->DSN[0] = '\0';
    }
    if (request == ODBC_REMOVE_DSN) {
	if (!setupdlg->attr[KEY_DSN].supplied) {
	    success = FALSE;
	} else {
	    success = SQLRemoveDSNFromIni(setupdlg->attr[KEY_DSN].attr);
	}
    } else {
#if defined(_WIN32) || defined(_WIN64)
	setupdlg->parent = hwnd;
#endif
#ifdef __OS2__
	setupdlg->parent = (SQLHWND) hwnd;
#endif
	setupdlg->driver = driver;
	setupdlg->newDSN = request == ODBC_ADD_DSN;
	setupdlg->defDSN = strcasecmp(setupdlg->attr[KEY_DSN].attr,
				      "Default") == 0;
	if (hwnd) {
#if defined(_WIN32) || defined(_WIN64)
	    success = DialogBoxParam(hModule, MAKEINTRESOURCE(CONFIGDSN),
				     hwnd, (DLGPROC) ConfigDlgProc,
				     (LPARAM) setupdlg) == IDOK;
#endif
#ifdef __OS2__
	    HMODULE hDLL;

	    DosQueryModuleHandle("SQLLODBC.DLL", &hDLL);
	    setupdlg->hDLL = hDLL;
	    success = WinDlgBox(HWND_DESKTOP, hwnd, ConfigDlgProc, hDLL,
				DLG_SQLITEODBCCONFIGURATION, setupdlg);
	    success = (success == DID_OK) ? TRUE : FALSE;
#endif
	} else if (setupdlg->attr[KEY_DSN].supplied) {
	    success = SetDSNAttributes(hwnd, setupdlg);
	} else {
	    success = FALSE;
	}
    }
    xfree(setupdlg);
    return success;
}

#if defined(_WIN32) || defined(_WIN64)
/**
 * Dialog procedure for SQLDriverConnect().
 * @param hdlg handle of dialog window
 * @param wmsg type of message
 * @param wparam wparam of message
 * @param lparam lparam of message
 * @result true or false
 */

static BOOL CALLBACK
DriverConnectProc(HWND hdlg, WORD wmsg, WPARAM wparam, LPARAM lparam)
{
    SETUPDLG *setupdlg;

    switch (wmsg) {
    case WM_INITDIALOG:
#ifdef _WIN64
	SetWindowLongPtr(hdlg, DWLP_USER, lparam);
#else
	SetWindowLong(hdlg, DWL_USER, lparam);
#endif
	setupdlg = (SETUPDLG *) lparam;
	SetDlgItemText(hdlg, IDC_DSNAME, setupdlg->attr[KEY_DSN].attr);
	SetDlgItemText(hdlg, IDC_DESC, setupdlg->attr[KEY_DESC].attr);
	SetDlgItemText(hdlg, IDC_DBNAME, setupdlg->attr[KEY_DBNAME].attr);
	SetDlgItemText(hdlg, IDC_TONAME, setupdlg->attr[KEY_BUSY].attr);
	SendDlgItemMessage(hdlg, IDC_DSNAME, EM_LIMITTEXT,
			   (WPARAM) (MAXDSNAME - 1), (LPARAM) 0);
	SendDlgItemMessage(hdlg, IDC_DESC, EM_LIMITTEXT,
			   (WPARAM) (MAXDESC - 1), (LPARAM) 0);
	SendDlgItemMessage(hdlg, IDC_DBNAME, EM_LIMITTEXT,
			   (WPARAM) (MAXDBNAME - 1), (LPARAM) 0);
	SendDlgItemMessage(hdlg, IDC_TONAME, EM_LIMITTEXT,
			   (WPARAM) (MAXTONAME - 1), (LPARAM) 0);
	CheckDlgButton(hdlg, IDC_NOWCHAR,
		       getbool(setupdlg->attr[KEY_NOWCHAR].attr) ?
		       BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hdlg, IDC_STEPAPI,
		       getbool(setupdlg->attr[KEY_STEPAPI].attr) ?
		       BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hdlg, IDC_NOTXN,
		       getbool(setupdlg->attr[KEY_NOTXN].attr) ?
		       BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hdlg, IDC_LONGNAM,
		       getbool(setupdlg->attr[KEY_LONGNAM].attr) ?
		       BST_CHECKED : BST_UNCHECKED);
	if (setupdlg->defDSN) {
	    EnableWindow(GetDlgItem(hdlg, IDC_DSNAME), FALSE);
	    EnableWindow(GetDlgItem(hdlg, IDC_DSNAMETEXT), FALSE);
	}
	return TRUE;
    case WM_COMMAND:
	switch (GET_WM_COMMAND_ID(wparam, lparam)) {
	case IDC_BROWSE:
	    GetDBFile(hdlg);
	    break;
	case IDOK:
#ifdef _WIN64
	    setupdlg = (SETUPDLG *) GetWindowLongPtr(hdlg, DWLP_USER);
#else
	    setupdlg = (SETUPDLG *) GetWindowLong(hdlg, DWL_USER);
#endif
	    GetDlgItemText(hdlg, IDC_DSNAME,
			   setupdlg->attr[KEY_DSN].attr,
			   sizeof (setupdlg->attr[KEY_DSN].attr));
	    GetDlgItemText(hdlg, IDC_DBNAME,
			   setupdlg->attr[KEY_DBNAME].attr,
			   sizeof (setupdlg->attr[KEY_DBNAME].attr));
	    GetDlgItemText(hdlg, IDC_TONAME,
			   setupdlg->attr[KEY_BUSY].attr,
			   sizeof (setupdlg->attr[KEY_BUSY].attr));
	    strcpy(setupdlg->attr[KEY_NOWCHAR].attr,
		   (IsDlgButtonChecked(hdlg, IDC_NOWCHAR) == BST_CHECKED) ?
		   "1" : "0");
	    strcpy(setupdlg->attr[KEY_STEPAPI].attr,
		   (IsDlgButtonChecked(hdlg, IDC_STEPAPI) == BST_CHECKED) ?
		   "1" : "0");
	    strcpy(setupdlg->attr[KEY_NOTXN].attr,
		   (IsDlgButtonChecked(hdlg, IDC_NOTXN) == BST_CHECKED) ?
		   "1" : "0");
	    strcpy(setupdlg->attr[KEY_LONGNAM].attr,
		   (IsDlgButtonChecked(hdlg, IDC_LONGNAM) == BST_CHECKED) ?
		   "1" : "0");
	    /* FALL THROUGH */
	case IDCANCEL:
	    EndDialog(hdlg, GET_WM_COMMAND_ID(wparam, lparam) == IDOK);
	    return TRUE;
	}
    }
    return FALSE;
}
#endif

#ifdef __OS2__
/**
 * Dialog procedure for SQLDriverConnect().
 * @param hdlg handle of dialog window
 * @param wmsg type of message
 * @param wparam wparam of message
 * @param lparam lparam of message
 * @result standard MRESULT
 */

static MRESULT EXPENTRY
DriverConnectProc(HWND hdlg, ULONG wmsg, MPARAM wparam, MPARAM lparam)
{
    SETUPDLG *setupdlg;

    switch (wmsg) {
    case WM_INITDLG:
	WinSetWindowULong(hdlg, QWL_USER, (ULONG) lparam);
	setupdlg = (SETUPDLG *) lparam;
	WinSetDlgItemText(hdlg, EF_DSNNAME, setupdlg->attr[KEY_DSN].attr);
	WinSetDlgItemText(hdlg, EF_DSNDESC, setupdlg->attr[KEY_DESC].attr);
	WinSetDlgItemText(hdlg, EF_DATABASE, setupdlg->attr[KEY_DBNAME].attr);
	WinSetDlgItemText(hdlg, EF_TIMEOUT, setupdlg->attr[KEY_BUSY].attr);
	WinCheckButton(hdlg, IDC_NOWCHAR,
		       getbool(setupdlg->attr[KEY_NOWCHAR].attr));
	WinCheckButton(hdlg, IDC_STEPAPI,
		       getbool(setupdlg->attr[KEY_STEPAPI].attr));
	WinCheckButton(hdlg, IDC_NOTXN,
		       getbool(setupdlg->attr[KEY_NOTXN].attr));
	WinCheckButton(hdlg, IDC_LONGNAM,
		       getbool(setupdlg->attr[KEY_LONGNAM].attr));
	return 0;
    case WM_COMMAND:
	switch (SHORT1FROMMP(wparam)) {
	case IDC_BROWSE:
	    GetDBFile(hdlg);
	    return 0;
	case DID_OK:
	    setupdlg = (SETUPDLG *) WinQueryWindowULong(hdlg, QWL_USER);
	    WinQueryDlgItemText(hdlg, EF_DSNNAME,
				sizeof (setupdlg->attr[KEY_DSN].attr),
				setupdlg->attr[KEY_DSN].attr);
	    WinQueryDlgItemText(hdlg, EF_DATABASE,
				sizeof (setupdlg->attr[KEY_DBNAME].attr),
				setupdlg->attr[KEY_DBNAME].attr);
	    WinQueryDlgItemText(hdlg, EF_TIMEOUT,
				sizeof (setupdlg->attr[KEY_BUSY].attr),
				setupdlg->attr[KEY_BUSY].attr);
	    if ((ULONG)WinSendDlgItemMsg(hdlg, IDC_NOWCHAR, BM_QUERYCHECK,
					 0, 0) == 1) {
		strcpy(setupdlg->attr[KEY_NOWCHAR].attr, "1");
	    } else {
		strcpy(setupdlg->attr[KEY_NOWCHAR].attr, "0");
	    }
	    if ((ULONG)WinSendDlgItemMsg(hdlg, IDC_STEPAPI, BM_QUERYCHECK,
					 0, 0) == 1) {
		strcpy(setupdlg->attr[KEY_STEPAPI].attr, "1");
	    } else {
		strcpy(setupdlg->attr[KEY_STEPAPI].attr, "0");
	    }
	    if ((ULONG)WinSendDlgItemMsg(hdlg, IDC_NOTXN, BM_QUERYCHECK,
					 0, 0) == 1) {
		strcpy(setupdlg->attr[KEY_NOTXN].attr, "1");
	    } else {
		strcpy(setupdlg->attr[KEY_NOTXN].attr, "0");
	    }
	    if ((ULONG)WinSendDlgItemMsg(hdlg, IDC_LONGNAM, BM_QUERYCHECK,
					 0, 0) == 1) {
		strcpy(setupdlg->attr[KEY_LONGNAM].attr, "1");
	    } else {
		strcpy(setupdlg->attr[KEY_LONGNAM].attr, "0");
	    }
	    WinDismissDlg(hdlg, DID_OK);
	    return 0;
	case DID_CANCEL:
	    WinDismissDlg(hdlg, DID_CANCEL);
	    return 0;
	}
    }
    return WinDefDlgProc(hdlg, wmsg, wparam, lparam);
}
#endif

/**
 * Internal connect using a driver connection string.
 * @param dbc database connection handle
 * @param hwnd parent window handle
 * @param connIn driver connect input string
 * @param connInLen length of driver connect input string or SQL_NTS
 * @param connOut driver connect output string
 * @param connOutMax length of driver connect output string
 * @param connOutLen output length of driver connect output string
 * @param drvcompl completion type
 * @result ODBC error code
 */

static SQLRETURN
drvdriverconnect(SQLHDBC dbc, SQLHWND hwnd,
		 SQLCHAR *connIn, SQLSMALLINT connInLen,
		 SQLCHAR *connOut, SQLSMALLINT connOutMax,
		 SQLSMALLINT *connOutLen, SQLUSMALLINT drvcompl)
{
    BOOL maybeprompt, prompt = FALSE, defaultdsn = FALSE;
    DBC *d;
    SETUPDLG *setupdlg;
    SQLRETURN ret;

    if (dbc == SQL_NULL_HDBC) {
	return SQL_INVALID_HANDLE;
    }
    d = (DBC *) dbc;
    if (d->sqlite) {
	setstatd(d, -1, "connection already established", "08002");
	return SQL_ERROR;
    }
    setupdlg = (SETUPDLG *) xmalloc(sizeof (SETUPDLG));
    if (setupdlg == NULL) {
	return SQL_ERROR;
    }
    memset(setupdlg, 0, sizeof (SETUPDLG));
    maybeprompt = drvcompl == SQL_DRIVER_COMPLETE ||
	drvcompl == SQL_DRIVER_COMPLETE_REQUIRED;
    if (connIn == NULL || !connInLen ||
	(connInLen == SQL_NTS && !connIn[0])) {
	prompt = TRUE;
    } else {
	ParseAttributes((LPCSTR) connIn, setupdlg);
	if (!setupdlg->attr[KEY_DSN].attr[0] &&
	    drvcompl == SQL_DRIVER_COMPLETE_REQUIRED) {
	    strcpy(setupdlg->attr[KEY_DSN].attr, "DEFAULT");
	    defaultdsn = TRUE;
	}
	GetAttributes(setupdlg);
	if (drvcompl == SQL_DRIVER_PROMPT ||
	    (maybeprompt &&
	     !setupdlg->attr[KEY_DBNAME].attr[0])) {
	    prompt = TRUE;
	}
    }
retry:
    if (prompt) {
	short dlgret;

	setupdlg->defDSN = setupdlg->attr[KEY_DRIVER].attr[0] != '\0';
#if defined(_WIN32) || defined(_WIN64)
	dlgret = DialogBoxParam(hModule, MAKEINTRESOURCE(DRIVERCONNECT),
				hwnd, (DLGPROC) DriverConnectProc,
				(LPARAM) setupdlg);
#endif
#ifdef __OS2__
	HMODULE hDLL;

	DosQueryModuleHandle("SQLLODBC.DLL", &hDLL);
	setupdlg->hDLL = hDLL;
	dlgret = WinDlgBox(HWND_DESKTOP, (HWND) hwnd, DriverConnectProc, hDLL,
			   DLG_SQLITEODBCCONFIGURATION, setupdlg);
#endif
	if (!dlgret || dlgret == -1) {
	    xfree(setupdlg);
	    return SQL_NO_DATA;
	}
    }
    if (connOut || connOutLen) {
	char buf[SQL_MAX_MESSAGE_LENGTH * 4];
	int len, count;
	char dsn_0 = (setupdlg->attr[KEY_DSN].attr[0] && !defaultdsn) ?
	    setupdlg->attr[KEY_DSN].attr[0] : '\0';
	char drv_0 = setupdlg->attr[KEY_DRIVER].attr[0];

	buf[0] = '\0';
	count = snprintf(buf, sizeof (buf),
			 "%s%s%s%s%s%sDatabase=%s;StepAPI=%s;"
			 "NoTXN=%s;Timeout=%s;NoWCHAR=%s;LongNames=%s",
			 dsn_0 ? "DSN=" : "",
			 dsn_0 ? setupdlg->attr[KEY_DSN].attr : "",
			 dsn_0 ? ";" : "",
			 drv_0 ? "Driver=" : "",
			 drv_0 ? setupdlg->attr[KEY_DRIVER].attr : "",
			 drv_0 ? ";" : "",
			 setupdlg->attr[KEY_DBNAME].attr,
			 setupdlg->attr[KEY_STEPAPI].attr,
			 setupdlg->attr[KEY_NOTXN].attr,
			 setupdlg->attr[KEY_BUSY].attr,
			 setupdlg->attr[KEY_NOWCHAR].attr,
			 setupdlg->attr[KEY_LONGNAM].attr);
	if (count < 0) {
	    buf[sizeof (buf) - 1] = '\0';
	}
	len = min(connOutMax - 1, strlen(buf));
	if (connOut) {
	    strncpy((char *) connOut, buf, len);
	    connOut[len] = '\0';
	}
	if (connOutLen) {
	    *connOutLen = len;
	}
    }
#if defined(HAVE_SQLITETRACE) && (HAVE_SQLITETRACE)
    if (setupdlg->attr[KEY_DSN].attr[0]) {
	char tracef[SQL_MAX_MESSAGE_LENGTH];

	tracef[0] = '\0';
	SQLGetPrivateProfileString(setupdlg->attr[KEY_DSN].attr,
				   "tracefile", "", tracef,
				   sizeof (tracef), ODBC_INI);
	if (tracef[0] != '\0') {
	    d->trace = fopen(tracef, "a");
	}
    }
#endif
    d->nowchar = getbool(setupdlg->attr[KEY_NOWCHAR].attr);
    d->longnames = getbool(setupdlg->attr[KEY_LONGNAM].attr);
    ret = dbopen(d, setupdlg->attr[KEY_DBNAME].attr,
		 setupdlg->attr[KEY_DSN].attr,
		 setupdlg->attr[KEY_STEPAPI].attr,
		 setupdlg->attr[KEY_NOTXN].attr,
		 setupdlg->attr[KEY_BUSY].attr);
    if (ret != SQL_SUCCESS) {
	if (maybeprompt && !prompt) {
	    prompt = TRUE;
	    goto retry;
	}
	xfree(setupdlg);
	return ret;
    }
    xfree(setupdlg);
    return SQL_SUCCESS;
}

#endif /* WITHOUT_DRIVERMGR */
#endif /* _WIN32 || _WIN64 || __OS2__ */

#ifndef WINTERFACE
/**
 * Connect using a driver connection string.
 * @param dbc database connection handle
 * @param hwnd parent window handle
 * @param connIn driver connect input string
 * @param connInLen length of driver connect input string or SQL_NTS
 * @param connOut driver connect output string
 * @param connOutMax length of driver connect output string
 * @param connOutLen output length of driver connect output string
 * @param drvcompl completion type
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLDriverConnect(SQLHDBC dbc, SQLHWND hwnd,
		 SQLCHAR *connIn, SQLSMALLINT connInLen,
		 SQLCHAR *connOut, SQLSMALLINT connOutMax,
		 SQLSMALLINT *connOutLen, SQLUSMALLINT drvcompl)
{
    SQLRETURN ret;

    HDBC_LOCK(dbc);
    ret = drvdriverconnect(dbc, hwnd, connIn, connInLen,
			   connOut, connOutMax, connOutLen, drvcompl);
    HDBC_UNLOCK(dbc);
    return ret;
}
#endif

#ifdef WINTERFACE
/**
 * Connect using a driver connection string (UNICODE version).
 * @param dbc database connection handle
 * @param hwnd parent window handle
 * @param connIn driver connect input string
 * @param connInLen length of driver connect input string or SQL_NTS
 * @param connOut driver connect output string
 * @param connOutMax length of driver connect output string
 * @param connOutLen output length of driver connect output string
 * @param drvcompl completion type
 * @result ODBC error code
 */

SQLRETURN SQL_API
SQLDriverConnectW(SQLHDBC dbc, SQLHWND hwnd,
		  SQLWCHAR *connIn, SQLSMALLINT connInLen,
		  SQLWCHAR *connOut, SQLSMALLINT connOutMax,
		  SQLSMALLINT *connOutLen, SQLUSMALLINT drvcompl)
{
    SQLRETURN ret;
    char *ci = NULL;
    SQLSMALLINT len = 0;

    HDBC_LOCK(dbc);
    if (connIn) {
	if (connInLen == SQL_NTS) {
	    connInLen = -1;
	}
	ci = uc_to_utf_c(connIn, connInLen);
	if (!ci) {
	    DBC *d = (DBC *) dbc;

	    setstatd(d, -1, "out of memory", (*d->ov3) ? "HY000" : "S1000");
	    HDBC_UNLOCK(dbc);
	    return SQL_ERROR;
	}
    }
    ret = drvdriverconnect(dbc, hwnd, (SQLCHAR *) ci, SQL_NTS,
			   (SQLCHAR *) connOut, connOutMax, &len, drvcompl);
    HDBC_UNLOCK(dbc);
    uc_free(ci);
    if (ret == SQL_SUCCESS) {
	SQLWCHAR *co = NULL;

	if (connOut) {
	    if (len > 0) {
		co = uc_from_utf((SQLCHAR *) connOut, len);
		if (co) {
		    uc_strncpy(connOut, co, connOutMax / sizeof (SQLWCHAR));
		    len = min(connOutMax / sizeof (SQLWCHAR), uc_strlen(co));
		    uc_free(co);
		} else {
		    len = 0;
		}
	    }
	    if (len <= 0) {
		len = 0;
		connOut[0] = 0;
	    }
	} else {
	    len = 0;
	}
	if (connOutLen) {
	    *connOutLen = len;
	}
    }
    return ret;
}
#endif

#if defined(_WIN32) || defined(_WIN64)

/**
 * DLL initializer for WIN32.
 * @param hinst instance handle
 * @param reason reason code for entry point
 * @param reserved
 * @result always true
 */

BOOL APIENTRY
LibMain(HANDLE hinst, DWORD reason, LPVOID reserved)
{
    static int initialized = 0;

    switch (reason) {
    case DLL_PROCESS_ATTACH:
	if (!initialized++) {
	    hModule = hinst;
#ifdef WINTERFACE
	    /* MS Access hack part 1 (reserved error -7748) */
	    statSpec2P = statSpec2;
	    statSpec3P = statSpec3;
#endif
	}
	break;
    case DLL_THREAD_ATTACH:
	break;
    case DLL_PROCESS_DETACH:
	--initialized;
	break;
    case DLL_THREAD_DETACH:
	break;
    default:
	break;
    }
    return TRUE;
}

/**
 * DLL entry point for WIN32.
 * @param hinst instance handle
 * @param reason reason code for entry point
 * @param reserved
 * @result always true
 */

int __stdcall
DllMain(HANDLE hinst, DWORD reason, LPVOID reserved)
{
    return LibMain(hinst, reason, reserved);
}

#ifndef WITHOUT_INSTALLER

/**
 * Handler for driver installer/uninstaller error messages.
 * @param name name of API function for which to show error messages
 * @result true when error message retrieved
 */

static BOOL
InUnError(char *name)
{
    WORD err = 1;
    DWORD code;
    char errmsg[301];
    WORD errlen, errmax = sizeof (errmsg) - 1;
    int sqlret;
    BOOL ret = FALSE;

    do {
	errmsg[0] = '\0';
	sqlret = SQLInstallerError(err, &code, errmsg, errmax, &errlen);
	if (SQL_SUCCEEDED(sqlret)) {
	    MessageBox(NULL, errmsg, name,
		       MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
	    ret = TRUE;
	}
	err++;
    } while (sqlret != SQL_NO_DATA);
    return ret;
}

/**
 * Built in driver installer/uninstaller.
 * @param remove true for uninstall
 * @param cmdline command line string of rundll32
 */

static BOOL
InUn(int remove, char *cmdline)
{
#ifdef WINTERFACE
    static char *drivername = "SQLite ODBC (UTF-8) Driver";
    static char *dsname = "SQLite3 UTF-8 Datasource";
#else
    static char *drivername = "SQLite3 ODBC Driver";
    static char *dsname = "SQLite Datasource";
#endif
    char *dllname, *p;
    char dllbuf[301], path[301], driver[300], attr[300], inst[400];
    WORD pathmax = sizeof (path) - 1, pathlen;
    DWORD usecnt, mincnt;
    int quiet = 0;

    dllbuf[0] = '\0';
    GetModuleFileName(hModule, dllbuf, sizeof (dllbuf));
    p = strrchr(dllbuf, '\\');
    dllname = p ? (p + 1) : dllbuf; 
    quiet = cmdline && strstr(cmdline, "quiet");
    if (SQLInstallDriverManager(path, pathmax, &pathlen)) {
	sprintf(driver, "%s;Driver=%s;Setup=%s;",
		drivername, dllname, dllname);
	p = driver;
	while (*p) {
	    if (*p == ';') {
		*p = '\0';
	    }
	    ++p;
	}
	usecnt = 0;
	path[0] = '\0';
	SQLInstallDriverEx(driver, NULL, path, pathmax, NULL,
			   ODBC_INSTALL_INQUIRY, &usecnt);
	pathlen = strlen(path);
	while (pathlen > 0 && path[pathlen - 1] == '\\') {
	    --pathlen;
	    path[pathlen] = '\0';
	}
	sprintf(driver, "%s;Driver=%s\\%s;Setup=%s\\%s;",
		drivername, path, dllname, path, dllname);
	p = driver;
	while (*p) {
	    if (*p == ';') {
		*p = '\0';
	    }
	    ++p;
	}
	sprintf(inst, "%s\\%s", path, dllname);
	if (!remove && usecnt > 0) {
	    /* first install try: copy over driver dll, keeping DSNs */
	    if (GetFileAttributesA(dllbuf) != INVALID_FILE_ATTRIBUTES &&
		CopyFile(dllbuf, inst, 0)) {
		if (!quiet) {
		    char buf[512];

		    sprintf(buf, "%s replaced.", drivername);
		    MessageBox(NULL, buf, "Info",
			       MB_ICONINFORMATION | MB_OK | MB_TASKMODAL |
			       MB_SETFOREGROUND);
		}
		return TRUE;
	    }
	}
	mincnt = remove ? 1 : 0;
	while (usecnt != mincnt) {
	    if (!SQLRemoveDriver(driver, TRUE, &usecnt)) {
		break;
	    }
	}
	if (remove) {
	    if (usecnt && !SQLRemoveDriver(driver, TRUE, &usecnt)) {
		InUnError("SQLRemoveDriver");
		return FALSE;
	    }
	    if (!usecnt) {
		char buf[512];

		DeleteFile(inst);
		if (!quiet) {
		    sprintf(buf, "%s uninstalled.", drivername);
		    MessageBox(NULL, buf, "Info",
			       MB_ICONINFORMATION | MB_OK | MB_TASKMODAL |
			       MB_SETFOREGROUND);
		}
	    }
	    sprintf(attr, "DSN=%s;Database=sqlite.db;", dsname);
	    p = attr;
	    while (*p) {
		if (*p == ';') {
		    *p = '\0';
		}
		++p;
	    }
	    SQLConfigDataSource(NULL, ODBC_REMOVE_SYS_DSN, drivername, attr);
	    return TRUE;
	}
	if (GetFileAttributesA(dllbuf) == INVALID_FILE_ATTRIBUTES) {
	    return FALSE;
	}
	if (strcmp(dllbuf, inst) != 0 && !CopyFile(dllbuf, inst, 0)) {
	    char buf[512];

	    sprintf(buf, "Copy %s to %s failed.", dllbuf, inst);
	    MessageBox(NULL, buf, "CopyFile",
		       MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND); 
	    return FALSE;
	}
	if (!SQLInstallDriverEx(driver, path, path, pathmax, &pathlen,
				ODBC_INSTALL_COMPLETE, &usecnt)) {
	    InUnError("SQLInstallDriverEx");
	    return FALSE;
	}
	sprintf(attr, "DSN=%s;Database=sqlite.db;", dsname);
	p = attr;
	while (*p) {
	    if (*p == ';') {
		*p = '\0';
	    }
	    ++p;
	}
	SQLConfigDataSource(NULL, ODBC_REMOVE_SYS_DSN, drivername, attr);
	if (!SQLConfigDataSource(NULL, ODBC_ADD_SYS_DSN, drivername, attr)) {
	    InUnError("SQLConfigDataSource");
	    return FALSE;
	}
	if (!quiet) {
	    char buf[512];

	    sprintf(buf, "%s installed.", drivername);
	    MessageBox(NULL, buf, "Info",
		       MB_ICONINFORMATION | MB_OK | MB_TASKMODAL |
		       MB_SETFOREGROUND);
	}
    } else {
	InUnError("SQLInstallDriverManager");
	return FALSE;
    }
    return TRUE;
}

/**
 * RunDLL32 entry point for driver installation.
 * @param hwnd window handle of caller
 * @param hinst of this DLL
 * @param lpszCmdLine rundll32 command line tail
 * @param nCmdShow ignored
 */

void CALLBACK
install(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    InUn(0, lpszCmdLine);
}

/**
 * RunDLL32 entry point for driver uninstallation.
 * @param hwnd window handle of caller
 * @param hinst of this DLL
 * @param lpszCmdLine rundll32 command line tail
 * @param nCmdShow ignored
 */

void CALLBACK
uninstall(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    InUn(1, lpszCmdLine);
}

#endif /* WITHOUT_INSTALLER */

#ifndef WITHOUT_SHELL

/**
 * Setup argv vector from string
 * @param argcp pointer to argc
 * @param argvp pointer to argv
 * @param cmdline command line string
 * @param argv0 0th element for argv or NULL, must be static
 */

static void
setargv(int *argcp, char ***argvp, char *cmdline, char *argv0)
{
    char *p, *arg, *argspace, **argv;
    int argc, size, inquote, copy, slashes;

    size = 2 + (argv0 ? 1 : 0);
    for (p = cmdline; *p != '\0'; p++) {
	if (ISSPACE(*p)) {
	    size++;
	    while (ISSPACE(*p)) {
		p++;
	    }
	    if (*p == '\0') {
		break;
	    }
	}
    }
    argspace = malloc(size * sizeof (char *) + strlen(cmdline) + 1);
    argv = (char **) argspace;
    argspace += size * sizeof (char *);
    size--;
    argc = 0;
    if (argv0) {
	argv[argc++] = argv0;
    }
    p = cmdline;
    for (; argc < size; argc++) {
	argv[argc] = arg = argspace;
	while (ISSPACE(*p)) {
	    p++;
	}
	if (*p == '\0') {
	    break;
	}
	inquote = 0;
	slashes = 0;
	while (1) {
	    copy = 1;
	    while (*p == '\\') {
		slashes++;
		p++;
	    }
	    if (*p == '"') {
		if ((slashes & 1) == 0) {
		    copy = 0;
		    if (inquote && p[1] == '"') {
			p++;
			copy = 1;
		    } else {
			inquote = !inquote;
		    }
		}
		slashes >>= 1;
	    }
	    while (slashes) {
		*arg = '\\';
		arg++;
		slashes--;
	    }
	    if (*p == '\0' || (!inquote && ISSPACE(*p))) {
		break;
	    }
	    if (copy != 0) {
		*arg = *p;
		arg++;
	    }
	    p++;
	}
	*arg = '\0';
	argspace = arg + 1;
    }
    argv[argc] = 0;
    *argcp = argc;
    *argvp = argv;
}

/**
 * RunDLL32 entry point for SQLite shell
 * @param hwnd window handle of caller
 * @param hinst of this DLL
 * @param lpszCmdLine rundll32 command line tail
 * @param nCmdShow ignored
 */

void CALLBACK
shell(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    int argc, needcon = 0;
    char **argv;
    extern int sqlite_main(int, char **);
    static const char *name = "SQLite Shell";
    DWORD ftype0, ftype1, ftype2;

    ftype0 = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
    ftype1 = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE));
    ftype2 = GetFileType(GetStdHandle(STD_ERROR_HANDLE));
    if (ftype0 != FILE_TYPE_DISK && ftype0 != FILE_TYPE_CHAR &&
	ftype0 != FILE_TYPE_PIPE) {
	fclose(stdin);
	++needcon;
	ftype0 = FILE_TYPE_UNKNOWN;
    }
    if (ftype1 != FILE_TYPE_DISK && ftype1 != FILE_TYPE_CHAR &&
	ftype1 != FILE_TYPE_PIPE) {
	fclose(stdout);
	++needcon;
	ftype1 = FILE_TYPE_UNKNOWN;
    }
    if (ftype2 != FILE_TYPE_DISK && ftype2 != FILE_TYPE_CHAR &&
	ftype2 != FILE_TYPE_PIPE) {
	fclose(stderr);
	++needcon;
	ftype2 = FILE_TYPE_UNKNOWN;
    }
    if (needcon > 0) {
	AllocConsole();
	SetConsoleTitle(name);
    }
    if (ftype0 == FILE_TYPE_UNKNOWN) {
	freopen("CONIN$", "r", stdin);
    }
    if (ftype1 == FILE_TYPE_UNKNOWN) {
	freopen("CONOUT$", "w", stdout);
    }
    if (ftype2 == FILE_TYPE_UNKNOWN) {
	freopen("CONOUT$", "w", stderr);
    }
    setargv(&argc, &argv, lpszCmdLine, (char *) name);
    sqlite_main(argc, argv);
}

#endif /* WITHOUT_SHELL */

#endif /* _WIN32 || _WIN64 */

#if defined(HAVE_ODBCINSTEXT_H) && (HAVE_ODBCINSTEXT_H)

/*
 * unixODBC property page for this driver,
 * may or may not work depending on unixODBC version.
 */

#include <odbcinstext.h>

int
ODBCINSTGetProperties(HODBCINSTPROPERTY prop)
{
    static const char *instYN[] = { "No", "Yes", NULL };

    prop->pNext = (HODBCINSTPROPERTY) malloc(sizeof (ODBCINSTPROPERTY));
    prop = prop->pNext;
    memset(prop, 0, sizeof (ODBCINSTPROPERTY));
    prop->nPromptType = ODBCINST_PROMPTTYPE_FILENAME;
    strncpy(prop->szName, "Database", INI_MAX_PROPERTY_NAME);
    strncpy(prop->szValue, "", INI_MAX_PROPERTY_VALUE);
    prop->pNext = (HODBCINSTPROPERTY) malloc(sizeof (ODBCINSTPROPERTY));
    prop = prop->pNext;
    memset(prop, 0, sizeof (ODBCINSTPROPERTY));
    prop->nPromptType = ODBCINST_PROMPTTYPE_TEXTEDIT;
    strncpy(prop->szName, "Timeout", INI_MAX_PROPERTY_NAME);
    strncpy(prop->szValue, "100000", INI_MAX_PROPERTY_VALUE);
    prop->pNext = (HODBCINSTPROPERTY) malloc(sizeof (ODBCINSTPROPERTY));
    prop = prop->pNext;
    memset(prop, 0, sizeof (ODBCINSTPROPERTY));
    prop->nPromptType = ODBCINST_PROMPTTYPE_COMBOBOX;
    prop->aPromptData = malloc(sizeof (instYN));
    memcpy(prop->aPromptData, instYN, sizeof (instYN));
    strncpy(prop->szName, "StepAPI", INI_MAX_PROPERTY_NAME);
    strncpy(prop->szValue, "No", INI_MAX_PROPERTY_VALUE);
#ifdef WINTERFACE
    prop->pNext = (HODBCINSTPROPERTY) malloc(sizeof (ODBCINSTPROPERTY));
    prop = prop->pNext;
    memset(prop, 0, sizeof (ODBCINSTPROPERTY));
    prop->nPromptType = ODBCINST_PROMPTTYPE_COMBOBOX;
    prop->aPromptData = malloc(sizeof (instYN));
    memcpy(prop->aPromptData, instYN, sizeof (instYN));
    strncpy(prop->szName, "NoWCHAR", INI_MAX_PROPERTY_NAME);
    strncpy(prop->szValue, "No", INI_MAX_PROPERTY_VALUE);
#endif
    prop->pNext = (HODBCINSTPROPERTY) malloc(sizeof (ODBCINSTPROPERTY));
    prop = prop->pNext;
    memset(prop, 0, sizeof (ODBCINSTPROPERTY));
    prop->nPromptType = ODBCINST_PROMPTTYPE_COMBOBOX;
    prop->aPromptData = malloc(sizeof (instYN));
    memcpy(prop->aPromptData, instYN, sizeof (instYN));
    strncpy(prop->szName, "LongNames", INI_MAX_PROPERTY_NAME);
    strncpy(prop->szValue, "No", INI_MAX_PROPERTY_VALUE);
    return 1;
}

#endif /* HAVE_ODBCINSTEXT_H */
