/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include "portable.h"

#include <limits.h>
#include <stdio.h>
#include <ac/stdlib.h>
#include <ac/stdarg.h>
#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/unistd.h>
#include <ac/time.h>
#include <ac/errno.h>
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#include "lutil.h"
#include "ldap_defaults.h"
#include "ldap_pvt.h"
#include "lber_pvt.h"

#ifdef HAVE_EBCDIC
int _trans_argv = 1;
#endif

#ifdef _WIN32
/* Some Windows versions accept both forward and backslashes in
 * directory paths, but we always use backslashes when generating
 * and parsing...
 */
void lutil_slashpath( char *path )
{
	char *c, *p;

	p = path;
	while (( c=strchr( p, '/' ))) {
		*c++ = '\\';
		p = c;
	}
}
#endif

char* lutil_progname( const char* name, int argc, char *argv[] )
{
	char *progname;

	if(argc == 0) {
		return (char *)name;
	}

#ifdef HAVE_EBCDIC
	if (_trans_argv) {
		int i;
		for (i=0; i<argc; i++) __etoa(argv[i]);
		_trans_argv = 0;
	}
#endif
	LUTIL_SLASHPATH( argv[0] );
	progname = strrchr ( argv[0], *LDAP_DIRSEP );
	progname = progname ? &progname[1] : argv[0];
#ifdef _WIN32
	{
		size_t len = strlen( progname );
		if ( len > 4 && strcasecmp( &progname[len - 4], ".exe" ) == 0 )
			progname[len - 4] = '\0';
	}
#endif
	return progname;
}

#if 0
size_t lutil_gentime( char *s, size_t smax, const struct tm *tm )
{
	size_t ret;
#ifdef HAVE_EBCDIC
/* We've been compiling in ASCII so far, but we want EBCDIC now since
 * strftime only understands EBCDIC input.
 */
#pragma convlit(suspend)
#endif
	ret = strftime( s, smax, "%Y%m%d%H%M%SZ", tm );
#ifdef HAVE_EBCDIC
#pragma convlit(resume)
	__etoa( s );
#endif
	return ret;
}
#endif

size_t lutil_localtime( char *s, size_t smax, const struct tm *tm, long delta )
{
	size_t	ret;
	char	*p;

	if ( smax < 16 ) {	/* YYYYmmddHHMMSSZ */
		return 0;
	}

#ifdef HAVE_EBCDIC
/* We've been compiling in ASCII so far, but we want EBCDIC now since
 * strftime only understands EBCDIC input.
 */
#pragma convlit(suspend)
#endif
	ret = strftime( s, smax, "%Y%m%d%H%M%SZ", tm );
#ifdef HAVE_EBCDIC
#pragma convlit(resume)
	__etoa( s );
#endif
	if ( delta == 0 || ret == 0 ) {
		return ret;
	}

	if ( smax < 20 ) {	/* YYYYmmddHHMMSS+HHMM */
		return 0;
	}

	p = s + 14;

	if ( delta < 0 ) {
		p[ 0 ] = '-';
		delta = -delta;
	} else {
		p[ 0 ] = '+';
	}
	p++;

	snprintf( p, smax - 15, "%02ld%02ld", delta / 3600,
			( delta % 3600 ) / 60 );

	return ret + 4;
}

int lutil_tm2time( struct lutil_tm *tm, struct lutil_timet *tt )
{
	static int moffset[12] = {
		0, 31, 59, 90, 120,
		151, 181, 212, 243,
		273, 304, 334 }; 
	int sec;

	tt->tt_usec = tm->tm_usec;

	/* special case 0000/01/01+00:00:00 is returned as zero */
	if ( tm->tm_year == -1900 && tm->tm_mon == 0 && tm->tm_mday == 1 &&
		tm->tm_hour == 0 && tm->tm_min == 0 && tm->tm_sec == 0 ) {
		tt->tt_sec = 0;
		tt->tt_gsec = 0;
		return 0;
	}

	/* tm->tm_year is years since 1900 */
	/* calculate days from years since 1970 (epoch) */ 
	tt->tt_sec = tm->tm_year - 70; 
	tt->tt_sec *= 365L; 

	/* count leap days in preceding years */ 
	tt->tt_sec += ((tm->tm_year -69) >> 2); 

	/* calculate days from months */ 
	tt->tt_sec += moffset[tm->tm_mon]; 

	/* add in this year's leap day, if any */ 
	if (((tm->tm_year & 3) == 0) && (tm->tm_mon > 1)) { 
		tt->tt_sec ++; 
	} 

	/* add in days in this month */ 
	tt->tt_sec += (tm->tm_mday - 1); 

	/* this function can handle a range of about 17408 years... */
	/* 86400 seconds in a day, divided by 128 = 675 */
	tt->tt_sec *= 675;

	/* move high 7 bits into tt_gsec */
	tt->tt_gsec = tt->tt_sec >> 25;
	tt->tt_sec -= tt->tt_gsec << 25;

	/* get hours */ 
	sec = tm->tm_hour; 

	/* convert to minutes */ 
	sec *= 60L; 
	sec += tm->tm_min; 

	/* convert to seconds */ 
	sec *= 60L; 
	sec += tm->tm_sec; 
	
	/* add remaining seconds */
	tt->tt_sec <<= 7;
	tt->tt_sec += sec;

	/* return success */
	return 0; 
}

int lutil_parsetime( char *atm, struct lutil_tm *tm )
{
	while (atm && tm) {
		char *ptr = atm;
		unsigned i, fracs;

		/* Is the stamp reasonably long? */
		for (i=0; isdigit((unsigned char) atm[i]); i++);
		if (i < sizeof("00000101000000")-1)
			break;

		/*
		 * parse the time into a struct tm
		 */
		/* 4 digit year to year - 1900 */
		tm->tm_year = *ptr++ - '0';
		tm->tm_year *= 10; tm->tm_year += *ptr++ - '0';
		tm->tm_year *= 10; tm->tm_year += *ptr++ - '0';
		tm->tm_year *= 10; tm->tm_year += *ptr++ - '0';
		tm->tm_year -= 1900;
		/* month 01-12 to 0-11 */
		tm->tm_mon = *ptr++ - '0';
		tm->tm_mon *=10; tm->tm_mon += *ptr++ - '0';
		if (tm->tm_mon < 1 || tm->tm_mon > 12) break;
		tm->tm_mon--;

		/* day of month 01-31 */
		tm->tm_mday = *ptr++ - '0';
		tm->tm_mday *=10; tm->tm_mday += *ptr++ - '0';
		if (tm->tm_mday < 1 || tm->tm_mday > 31) break;

		/* Hour 00-23 */
		tm->tm_hour = *ptr++ - '0';
		tm->tm_hour *=10; tm->tm_hour += *ptr++ - '0';
		if (tm->tm_hour < 0 || tm->tm_hour > 23) break;

		/* Minute 00-59 */
		tm->tm_min = *ptr++ - '0';
		tm->tm_min *=10; tm->tm_min += *ptr++ - '0';
		if (tm->tm_min < 0 || tm->tm_min > 59) break;

		/* Second 00-61 */
		tm->tm_sec = *ptr++ - '0';
		tm->tm_sec *=10; tm->tm_sec += *ptr++ - '0';
		if (tm->tm_sec < 0 || tm->tm_sec > 61) break;

		/* Fractions of seconds */
		if ( *ptr == '.' ) {
			ptr++;
			for (i = 0, fracs = 0; isdigit((unsigned char) *ptr); ) {
				i*=10; i+= *ptr++ - '0';
				fracs++;
			}
			tm->tm_usec = i;
			if (i) {
				for (i = fracs; i<6; i++)
					tm->tm_usec *= 10;
			}
		}

		/* Must be UTC */
		if (*ptr != 'Z') break;

		return 0;
	}
	return -1;
}

/* strcopy is like strcpy except it returns a pointer to the trailing NUL of
 * the result string. This allows fast construction of catenated strings
 * without the overhead of strlen/strcat.
 */
char *
lutil_strcopy(
	char *a,
	const char *b
)
{
	if (!a || !b)
		return a;
	
	while ((*a++ = *b++)) ;
	return a-1;
}

/* strncopy is like strcpy except it returns a pointer to the trailing NUL of
 * the result string. This allows fast construction of catenated strings
 * without the overhead of strlen/strcat.
 */
char *
lutil_strncopy(
	char *a,
	const char *b,
	size_t n
)
{
	if (!a || !b || n == 0)
		return a;
	
	while ((*a++ = *b++) && n-- > 0) ;
	return a-1;
}

/* memcopy is like memcpy except it returns a pointer to the byte past
 * the end of the result buffer, set to NULL. This allows fast construction
 * of catenated buffers.  Provided for API consistency with lutil_str*copy().
 */
char *
lutil_memcopy(
	char *a,
	const char *b,
	size_t n
)
{
	AC_MEMCPY(a, b, n);
	return a + n;
}

#ifndef HAVE_MKSTEMP
int mkstemp( char * template )
{
#ifdef HAVE_MKTEMP
	return open ( mktemp ( template ), O_RDWR|O_CREAT|O_EXCL, 0600 );
#else
	return -1;
#endif
}
#endif

#ifdef _MSC_VER
/* Equivalent of MS CRT's _dosmaperr().
 * @param lastError[in] Result of GetLastError().
 */
static errno_t win2errno(DWORD lastError)
{
	const struct { 
		DWORD   windows_code;
		errno_t errno_code;
	} WIN2ERRNO_TABLE[] = {
		{ ERROR_SUCCESS, 0 },
		{ ERROR_FILE_NOT_FOUND, ENOENT },
		{ ERROR_PATH_NOT_FOUND, ENOENT },
		{ ERROR_TOO_MANY_OPEN_FILES, EMFILE },
		{ ERROR_ACCESS_DENIED, EACCES },
		{ ERROR_INVALID_HANDLE, EBADF },
		{ ERROR_NOT_ENOUGH_MEMORY, ENOMEM },
		{ ERROR_LOCK_VIOLATION, EACCES },
		{ ERROR_FILE_EXISTS, EEXIST },
		{ ERROR_INVALID_PARAMETER, EINVAL },
		{ ERROR_FILENAME_EXCED_RANGE, ENAMETOOLONG },
	};
	const unsigned int WIN2ERRNO_TABLE_SIZE = sizeof(WIN2ERRNO_TABLE) /
sizeof(WIN2ERRNO_TABLE[0]);
	const errno_t DEFAULT_ERRNO_ERROR = -1;
	unsigned int i;

	for (i = 0; i < WIN2ERRNO_TABLE_SIZE; ++i) {
		if (WIN2ERRNO_TABLE[i].windows_code == lastError) {
			return WIN2ERRNO_TABLE[i].errno_code;
		}
	}
	return DEFAULT_ERRNO_ERROR;
}

struct dirent {
	char *d_name;
};
typedef struct DIR {
	HANDLE dir;
	struct dirent data;
	int first;
	char buf[MAX_PATH+1];
} DIR;
DIR *opendir( char *path )
{
	char tmp[32768];
	int len = strlen(path);
	DIR *d;
	HANDLE h;
	WIN32_FIND_DATA data;
	
	if (len+3 >= sizeof(tmp)) {
		errno = ENAMETOOLONG;
		return NULL;
	}

	strcpy(tmp, path);
	tmp[len++] = '\\';
	tmp[len++] = '*';
	tmp[len] = '\0';

	h = FindFirstFile( tmp, &data );

	if ( h == INVALID_HANDLE_VALUE ) {
		errno = win2errno( GetLastError());
		return NULL;
	}

	d = ber_memalloc( sizeof(DIR) );
	if ( !d )
		return NULL;
	d->dir = h;
	d->data.d_name = d->buf;
	d->first = 1;
	strcpy(d->data.d_name, data.cFileName);
	return d;
}
struct dirent *readdir(DIR *dir)
{
	WIN32_FIND_DATA data;

	if (dir->first) {
		dir->first = 0;
	} else {
		if (!FindNextFile(dir->dir, &data))
			return NULL;
		strcpy(dir->data.d_name, data.cFileName);
	}
	return &dir->data;
}
int closedir(DIR *dir)
{
	FindClose(dir->dir);
	ber_memfree(dir);
}
#endif

/*
 * Memory Reverse Search
 */
void *
(lutil_memrchr)(const void *b, int c, size_t n)
{
	if (n != 0) {
		const unsigned char *s, *bb = b, cc = c;

		for ( s = bb + n; s > bb; ) {
			if ( *--s == cc ) {
				return (void *) s;
			}
		}
	}

	return NULL;
}

int
lutil_atoix( int *v, const char *s, int x )
{
	char		*next;
	long		i;

	assert( s != NULL );
	assert( v != NULL );

	i = strtol( s, &next, x );
	if ( next == s || next[ 0 ] != '\0' ) {
		return -1;
	}

	if ( (long)(int)i != i ) {
		return 1;
	}

	*v = (int)i;

	return 0;
}

int
lutil_atoux( unsigned *v, const char *s, int x )
{
	char		*next;
	unsigned long	u;

	assert( s != NULL );
	assert( v != NULL );

	/* strtoul() has an odd interface */
	if ( s[ 0 ] == '-' ) {
		return -1;
	}

	u = strtoul( s, &next, x );
	if ( next == s || next[ 0 ] != '\0' ) {
		return -1;
	}

	if ( (unsigned long)(unsigned)u != u ) {
		return 1;
	}

	*v = u;

	return 0;
}

int
lutil_atolx( long *v, const char *s, int x )
{
	char *next;
	long l;
	int save_errno;

	assert( s != NULL );
	assert( v != NULL );

	if ( isspace( s[ 0 ] ) ) {
		return -1;
	}

	errno = 0;
	l = strtol( s, &next, x );
	save_errno = errno;
	if ( next == s || next[ 0 ] != '\0' ) {
		return -1;
	}

	if ( ( l == LONG_MIN || l == LONG_MAX ) && save_errno != 0 ) {
		return -1;
	}

	*v = l;

	return 0;
}

int
lutil_atoulx( unsigned long *v, const char *s, int x )
{
	char *next;
	unsigned long ul;
	int save_errno;

	assert( s != NULL );
	assert( v != NULL );

	/* strtoul() has an odd interface */
	if ( s[ 0 ] == '-' || isspace( s[ 0 ] ) ) {
		return -1;
	}

	errno = 0;
	ul = strtoul( s, &next, x );
	save_errno = errno;
	if ( next == s || next[ 0 ] != '\0' ) {
		return -1;
	}

	if ( ( ul == 0 || ul == ULONG_MAX ) && save_errno != 0 ) {
		return -1;
	}

	*v = ul;

	return 0;
}

#ifdef HAVE_LONG_LONG
#if defined(HAVE_STRTOLL) || defined(HAVE_STRTOQ)
int
lutil_atollx( long long *v, const char *s, int x )
{
	char *next;
	long long ll;
	int save_errno;

	assert( s != NULL );
	assert( v != NULL );

	if ( isspace( s[ 0 ] ) ) {
		return -1;
	}

	errno = 0;
#ifdef HAVE_STRTOLL
	ll = strtoll( s, &next, x );
#else /* HAVE_STRTOQ */
	ll = (unsigned long long)strtoq( s, &next, x );
#endif /* HAVE_STRTOQ */
	save_errno = errno;
	if ( next == s || next[ 0 ] != '\0' ) {
		return -1;
	}

	/* LLONG_MIN, LLONG_MAX are C99 only */
#if defined (LLONG_MIN) && defined(LLONG_MAX)
	if ( ( ll == LLONG_MIN || ll == LLONG_MAX ) && save_errno != 0 ) {
		return -1;
	}
#endif /* LLONG_MIN && LLONG_MAX */

	*v = ll;

	return 0;
}
#endif /* HAVE_STRTOLL || HAVE_STRTOQ */

#if defined(HAVE_STRTOULL) || defined(HAVE_STRTOUQ)
int
lutil_atoullx( unsigned long long *v, const char *s, int x )
{
	char *next;
	unsigned long long ull;
	int save_errno;

	assert( s != NULL );
	assert( v != NULL );

	/* strtoull() has an odd interface */
	if ( s[ 0 ] == '-' || isspace( s[ 0 ] ) ) {
		return -1;
	}

	errno = 0;
#ifdef HAVE_STRTOULL
	ull = strtoull( s, &next, x );
#else /* HAVE_STRTOUQ */
	ull = (unsigned long long)strtouq( s, &next, x );
#endif /* HAVE_STRTOUQ */
	save_errno = errno;
	if ( next == s || next[ 0 ] != '\0' ) {
		return -1;
	}

	/* ULLONG_MAX is C99 only */
#if defined(ULLONG_MAX)
	if ( ( ull == 0 || ull == ULLONG_MAX ) && save_errno != 0 ) {
		return -1;
	}
#endif /* ULLONG_MAX */

	*v = ull;

	return 0;
}
#endif /* HAVE_STRTOULL || HAVE_STRTOUQ */
#endif /* HAVE_LONG_LONG */

/* Multiply an integer by 100000000 and add new */
typedef struct lutil_int_decnum {
	unsigned char *buf;
	int bufsiz;
	int beg;
	int len;
} lutil_int_decnum;

#define	FACTOR1	(100000000&0xffff)
#define FACTOR2 (100000000>>16)

static void
scale( int new, lutil_int_decnum *prev, unsigned char *tmp )
{
	int i, j;
	unsigned char *in = prev->buf+prev->beg;
	unsigned int part;
	unsigned char *out = tmp + prev->bufsiz - prev->len;

	memset( tmp, 0, prev->bufsiz );
	if ( prev->len ) {
		for ( i = prev->len-1; i>=0; i-- ) {
			part = in[i] * FACTOR1;
			for ( j = i; part; j-- ) {
				part += out[j];
				out[j] = part & 0xff;
				part >>= 8;
			}
			part = in[i] * FACTOR2;
			for ( j = i-2; part; j-- ) {
				part += out[j];
				out[j] = part & 0xff;
				part >>= 8;
			}
		}
		j++;
		prev->beg += j;
		prev->len -= j;
	}

	out = tmp + prev->bufsiz;
	i = 0;
	do {
		i--;
		new += out[i];
		out[i] = new & 0xff;
		new >>= 8;
	} while ( new );
	i = -i;
	if ( prev->len < i ) {
		prev->beg = prev->bufsiz - i;
		prev->len = i;
	}
	AC_MEMCPY( prev->buf+prev->beg, tmp+prev->beg, prev->len );
}

/* Convert unlimited length decimal or hex string to binary.
 * Output buffer must be provided, bv_len must indicate buffer size
 * Hex input can be "0x1234" or "'1234'H"
 *
 * Note: High bit of binary form is always the sign bit. If the number
 * is supposed to be positive but has the high bit set, a zero byte
 * is prepended. It is assumed that this has already been handled on
 * any hex input.
 */
int
lutil_str2bin( struct berval *in, struct berval *out, void *ctx )
{
	char *pin, *pout;
	char *end;
	int i, chunk, len, rc = 0, hex = 0;
	if ( !out || !out->bv_val || out->bv_len < in->bv_len )
		return -1;

	pout = out->bv_val;
	/* Leading "0x" for hex input */
	if ( in->bv_len > 2 && in->bv_val[0] == '0' &&
		( in->bv_val[1] == 'x' || in->bv_val[1] == 'X' ) )
	{
		len = in->bv_len - 2;
		pin = in->bv_val + 2;
		hex = 1;
	} else if ( in->bv_len > 3 && in->bv_val[0] == '\'' &&
		in->bv_val[in->bv_len-2] == '\'' &&
		in->bv_val[in->bv_len-1] == 'H' )
	{
		len = in->bv_len - 3;
		pin = in->bv_val + 1;
		hex = 1;
	}
	if ( hex ) {
#define HEXMAX	(2 * sizeof(long))
		unsigned long l;
		char tbuf[HEXMAX+1];

		/* Convert a longword at a time, but handle leading
		 * odd bytes first
		 */
		chunk = len % HEXMAX;
		if ( !chunk )
			chunk = HEXMAX;

		while ( len ) {
			int ochunk;
			memcpy( tbuf, pin, chunk );
			tbuf[chunk] = '\0';
			errno = 0;
			l = strtoul( tbuf, &end, 16 );
			if ( errno )
				return -1;
			ochunk = (chunk + 1)/2;
			for ( i = ochunk - 1; i >= 0; i-- ) {
				pout[i] = l & 0xff;
				l >>= 8;
			}
			pin += chunk;
			pout += ochunk;
			len -= chunk;
			chunk = HEXMAX;
		}
		out->bv_len = pout - out->bv_val;
	} else {
	/* Decimal */
#define	DECMAX	8	/* 8 digits at a time */
		char tmpbuf[64], *tmp;
		lutil_int_decnum num;
		int neg = 0;
		long l;
		char tbuf[DECMAX+1];

		len = in->bv_len;
		pin = in->bv_val;
		num.buf = (unsigned char *)out->bv_val;
		num.bufsiz = out->bv_len;
		num.beg = num.bufsiz-1;
		num.len = 0;
		if ( pin[0] == '-' ) {
			neg = 0xff;
			len--;
			pin++;
		}

		/* tmp must be at least as large as outbuf */
		if ( out->bv_len > sizeof(tmpbuf)) {
			tmp = ber_memalloc_x( out->bv_len, ctx );
		} else {
			tmp = tmpbuf;
		}
		chunk = len & (DECMAX-1);
		if ( !chunk )
			chunk = DECMAX;

		while ( len ) {
			memcpy( tbuf, pin, chunk );
			tbuf[chunk] = '\0';
			errno = 0;
			l = strtol( tbuf, &end, 10 );
			if ( errno ) {
				rc = -1;
				goto decfail;
			}
			scale( l, &num, (unsigned char *)tmp );
			pin += chunk;
			len -= chunk;
			chunk = DECMAX;
		}
		/* Negate the result */
		if ( neg ) {
			unsigned char *ptr;

			ptr = num.buf+num.beg;

			/* flip all bits */
			for ( i=0; i<num.len; i++ )
				ptr[i] ^= 0xff;

			/* add 1, with carry - overflow handled below */
			while ( i-- && ! (ptr[i] = (ptr[i] + 1) & 0xff )) ;
		}
		/* Prepend sign byte if wrong sign bit */
		if (( num.buf[num.beg] ^ neg ) & 0x80 ) {
			num.beg--;
			num.len++;
			num.buf[num.beg] = neg;
		}
		if ( num.beg )
			AC_MEMCPY( num.buf, num.buf+num.beg, num.len );
		out->bv_len = num.len;
decfail:
		if ( tmp != tmpbuf ) {
			ber_memfree_x( tmp, ctx );
		}
	}
	return rc;
}

static	char		time_unit[] = "dhms";

/* Used to parse and unparse time intervals, not timestamps */
int
lutil_parse_time(
	const char	*in,
	unsigned long	*tp )
{
	unsigned long	t = 0;
	char		*s,
			*next;
	int		sofar = -1,
			scale[] = { 86400, 3600, 60, 1 };

	*tp = 0;

	for ( s = (char *)in; s[ 0 ] != '\0'; ) {
		unsigned long	u;
		char		*what;

		/* strtoul() has an odd interface */
		if ( s[ 0 ] == '-' ) {
			return -1;
		}

		u = strtoul( s, &next, 10 );
		if ( next == s ) {
			return -1;
		}

		if ( next[ 0 ] == '\0' ) {
			/* assume seconds */
			t += u;
			break;
		}

		what = strchr( time_unit, next[ 0 ] );
		if ( what == NULL ) {
			return -1;
		}

		if ( what - time_unit <= sofar ) {
			return -1;
		}

		sofar = what - time_unit;
		t += u * scale[ sofar ];

		s = &next[ 1 ];
	}

	*tp = t;
	return 0;
}

int
lutil_unparse_time(
	char			*buf,
	size_t			buflen,
	unsigned long		t )
{
	int		len, i;
	unsigned long	v[ 4 ];
	char		*ptr = buf;

	v[ 0 ] = t/86400;
	v[ 1 ] = (t%86400)/3600;
	v[ 2 ] = (t%3600)/60;
	v[ 3 ] = t%60;

	for ( i = 0; i < 4; i++ ) {
		if ( v[i] > 0 || ( i == 3 && ptr == buf ) ) {
			len = snprintf( ptr, buflen, "%lu%c", v[ i ], time_unit[ i ] );
			if ( len < 0 || (unsigned)len >= buflen ) {
				return -1;
			}
			buflen -= len;
			ptr += len;
		}
	}

	return 0;
}

/*
 * formatted print to string
 *
 * - if return code < 0, the error code returned by vsnprintf(3) is returned
 *
 * - if return code > 0, the buffer was not long enough;
 *	- if next is not NULL, *next will be set to buf + bufsize - 1
 *	- if len is not NULL, *len will contain the required buffer length
 *
 * - if return code == 0, the buffer was long enough;
 *	- if next is not NULL, *next will point to the end of the string printed so far
 *	- if len is not NULL, *len will contain the length of the string printed so far 
 */
int
lutil_snprintf( char *buf, ber_len_t bufsize, char **next, ber_len_t *len, LDAP_CONST char *fmt, ... )
{
	va_list		ap;
	int		ret;

	assert( buf != NULL );
	assert( bufsize > 0 );
	assert( fmt != NULL );

	va_start( ap, fmt );
	ret = vsnprintf( buf, bufsize, fmt, ap );
	va_end( ap );

	if ( ret < 0 ) {
		return ret;
	}

	if ( len ) {
		*len = ret;
	}

	if ( (unsigned) ret >= bufsize ) {
		if ( next ) {
			*next = &buf[ bufsize - 1 ];
		}

		return 1;
	}

	if ( next ) {
		*next = &buf[ ret ];
	}

	return 0;
}

