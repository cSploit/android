/*
 * vasprintf(3)
 * 20020809 entropy@tappedin.com
 * public domain.  no warranty.  use at your own risk.  have a nice day.
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_PATHS_H
#include <paths.h>
#endif /* HAVE_PATHS_H */

#include <freetds/sysdep_private.h>
#include "replacements.h"

TDS_RCSID(var, "$Id: vasprintf.c,v 1.23 2011-05-16 08:51:40 freddy77 Exp $");

#if defined(HAVE__VSNPRINTF) && !defined(HAVE_VSNPRINTF)
#undef HAVE_VSNPRINTF
#undef vsnprintf
#define HAVE_VSNPRINTF 1
#define vsnprintf _vsnprintf
#endif

#ifndef _PATH_DEVNULL
#define _PATH_DEVNULL "/dev/null"
#endif

#define CHUNKSIZE 512
int
vasprintf(char **ret, const char *fmt, va_list ap)
{
#if HAVE__VSCPRINTF
	int len = _vscprintf(fmt, ap);

	if (len >= 0) {
		*ret = malloc(len + 1);
		if (*ret) {
			vsprintf(*ret, fmt, ap);
			return len;
		}
		errno = ENOMEM;
	}
	*ret = NULL;
	return -1;
#elif HAVE_VSNPRINTF
	size_t chunks;
	size_t buflen;
	char *buf;
	int len;

	chunks = ((strlen(fmt) + 1) / CHUNKSIZE) + 1;
	buflen = chunks * CHUNKSIZE;
	for (;;) {
		if ((buf = malloc(buflen)) == NULL) {
			errno = ENOMEM;
			*ret = NULL;
			return -1;
		}
		len = vsnprintf(buf, buflen, fmt, ap);
		if (0 <= len && (size_t) len < buflen - 1) {
			break;
		}
		free(buf);
		buflen = (++chunks) * CHUNKSIZE;
		/* 
		 * len >= 0 is required for vsnprintf implementations that 
		 * return -1 for insufficient buffer
		 */
		if (len >= 0 && buflen <= (size_t) len) {
			buflen = len + 1;
		}
	}
	*ret = buf;
	return len;
#else /* HAVE_VSNPRINTF */
#ifdef _REENTRANT
	FILE *fp;
#else /* !_REENTRANT */
	static FILE *fp = NULL;
#endif /* !_REENTRANT */
	int len;
	char *buf;

	*ret = NULL;

#ifdef _REENTRANT

# ifdef _WIN32
#  error Win32 do not have /dev/null, should use vsnprintf version
# endif

	if ((fp = fopen(_PATH_DEVNULL, "w")) == NULL)
		return -1;
#else /* !_REENTRANT */
	if ((fp == NULL) && ((fp = fopen(_PATH_DEVNULL, "w")) == NULL))
		return -1;
#endif /* !_REENTRANT */

	len = vfprintf(fp, fmt, ap);

#ifdef _REENTRANT
	if (fclose(fp) != 0)
		return -1;
#endif /* _REENTRANT */

	if (len < 0)
		return len;
	if ((buf = malloc(len + 1)) == NULL) {
		errno = ENOMEM;
		return -1;
	}
	if (vsprintf(buf, fmt, ap) != len)
		return -1;
	*ret = buf;
	return len;
#endif /* HAVE_VSNPRINTF */
}
