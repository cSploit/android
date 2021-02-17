/* getopt.c -- replacement getopt(3) routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
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
/* This work is based upon the public-domain getopt(3) routines
 * developed by AT&T.  Modified by Kurt D. Zeilenga for inclusion
 * into OpenLDAP Software.  Significant contributors include:
 *   Howard Chu
 */

#include "portable.h"

#ifndef HAVE_GETOPT

#include <stdio.h>

#include <ac/string.h>
#include <ac/unistd.h>

#ifdef HAVE_IO_H
#include <io.h>
#endif

#include "lutil.h"

#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

int opterr = 1;
int optind = 1;
int optopt;
char * optarg;

#ifdef HAVE_EBCDIC
extern int _trans_argv;
#endif

static void ERR (char * const argv[], const char * s, char c)
{
#ifdef DF_TRACE_DEBUG
printf("DF_TRACE_DEBUG: 	static void ERR () in getopt.c\n");
#endif
	if (opterr)
	{
		char *ptr, outbuf[4096];

		ptr = lutil_strncopy(outbuf, argv[0], sizeof(outbuf) - 2);
		ptr = lutil_strncopy(ptr, s, sizeof(outbuf)-2 -(ptr-outbuf));
		*ptr++ = c;
		*ptr++ = '\n';
#ifdef HAVE_EBCDIC
		__atoe_l(outbuf, ptr - outbuf);
#endif
		(void) write(STDERR_FILENO,outbuf,ptr - outbuf);
	}
}

int getopt (int argc, char * const argv [], const char * opts)
{
	static int sp = 1, error = (int) '?';
	static char sw = '-', eos = '\0', arg = ':';
	register char c, * cp;

#ifdef DF_TRACE_DEBUG
printf("DF_TRACE_DEBUG: 	int getopt () in getopt.c\n");
#endif

#ifdef HAVE_EBCDIC
	if (_trans_argv) {
		int i;
		for (i=0; i<argc; i++) __etoa(argv[i]);
		_trans_argv = 0;
	}
#endif
	if (sp == 1)
	{
		if (optind >= argc || argv[optind][0] != sw
		|| argv[optind][1] == eos)
			return EOF;
		else if (strcmp(argv[optind],"--") == 0)
		{
			optind++;
			return EOF;
		}
	}
	c = argv[optind][sp];
	optopt = (int) c;
	if (c == arg || (cp = strchr(opts,c)) == NULL)
	{
		ERR(argv,_(": illegal option--"),c);
		if (argv[optind][++sp] == eos)
		{
			optind++;
			sp = 1;
		}
		return error;
	}
	else if (*++cp == arg)
	{
		if (argv[optind][sp + 1] != eos)
			optarg = &argv[optind++][sp + 1];
		else if (++optind >= argc)
		{
			ERR(argv,_(": option requires an argument--"),c);
			sp = 1;
			return error;
		}
		else
			optarg = argv[optind++];
		sp = 1;
	}
	else
	{
		if (argv[optind][++sp] == eos)
		{
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return (int) c;
}
#endif /* HAVE_GETOPT */
