/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004  Brian Bruns
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* 
 * Purpose: test conversions.  If they work, test their performance.  
 * To test performance, call this program with an iteration count (10 is probably fine).
 * The following shows performance converting to varchar:
 * $ make convert && ./convert 1 |grep iterations |grep 'varchar\.' |sort -n 
 */
#include "common.h"
#include <assert.h>
#include <freetds/convert.h>
#include "replacements.h"

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

static char software_version[] = "$Id: convert.c,v 1.26 2009-02-06 14:26:54 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int g_result = 0;
static TDSCONTEXT *ctx;

static void
free_convert(int type, CONV_RESULT *cr)
{
	switch (type) {
	case SYBCHAR: case SYBVARCHAR: case SYBTEXT: case XSYBCHAR: case XSYBVARCHAR:
	case SYBBINARY: case SYBVARBINARY: case SYBIMAGE: case XSYBBINARY: case XSYBVARBINARY:
	case SYBLONGBINARY:
		free(cr->c);
		break;
	}
}

int
main(int argc, char **argv)
{
	/* the conversion pair matrix */
	typedef struct
	{
		int srctype;
		int desttype;
		int yn;
	}
	ANSWER;
	const static ANSWER answers[] = {
#	include "tds_willconvert.h"
	};

	/* some default inputs */
	static const int bit_input = 1;

	/* timing variables to compute performance */
	struct timeval start, end;
	double starttime, endtime;

	int i, j, iterations = 0, result;

	TDS_CHAR *src = NULL;
	TDS_UINT srclen;
	CONV_RESULT cr;

	TDS_NUMERIC numeric;
	TDS_MONEY money;
	TDS_MONEY4 money4;
	TDS_DATETIME datetime;
	TDS_DATETIME4 datetime4;

	TDS_TINYINT tds_tinyint;
	TDS_SMALLINT tds_smallint;
	TDS_INT tds_int;
	TDS_INT8 tds_int8;
	TDS_USMALLINT tds_usmallint;
	TDS_UINT tds_uint;
	TDS_UINT8 tds_uint8;

	TDS_REAL tds_real;
	TDS_FLOAT tds_float;

	TDS_UNIQUE tds_unique;

	if (argc > 1) {
		iterations = atoi(argv[1]);
		printf("Computing %d iterations\n", iterations);
	}

	ctx = tds_alloc_context(NULL);
	assert(ctx);
	if (ctx->locale && !ctx->locale->date_fmt) {
		/* set default in case there's no locale file */
		ctx->locale->date_fmt = strdup(STD_DATETIME_FMT);
	}


	/*
	 * Test every possible conversion pair
	 */
	for (i = 0; i < sizeof(answers) / sizeof(ANSWER); i++) {
		if (!answers[i].yn)
			continue;	/* don't attempt nonconvertible types */

		if (answers[i].srctype == answers[i].desttype)
			continue;	/* don't attempt same types */

		cr.n.precision = 8;
		cr.n.scale = 2;

		switch (answers[i].srctype) {
		case SYBCHAR:
		case SYBVARCHAR:
		case SYBTEXT:
		case SYBBINARY:
		case SYBVARBINARY:
		case SYBIMAGE:
			switch (answers[i].desttype) {
			case SYBCHAR:
			case SYBVARCHAR:
			case SYBTEXT:
			case SYBDATETIME:
			case SYBDATETIME4:
				src = "Jan  1, 1999";
				break;
			case SYBBINARY:
			case SYBIMAGE:
				src = "0xbeef";
				break;
			case SYBINT1:
			case SYBINT2:
			case SYBINT4:
			case SYBINT8:
			case SYBUINT1:
			case SYBUINT2:
			case SYBUINT4:
			case SYBUINT8:
				src = "255";
				break;
			case SYBFLT8:
			case SYBREAL:
			case SYBNUMERIC:
			case SYBDECIMAL:
			case SYBMONEY:
			case SYBMONEY4:
				src = "1999.25";
				cr.n.precision = 8;
				cr.n.scale = 2;
				break;
			case SYBUNIQUE:
				src = "A8C60F70-5BD4-3E02-B769-7CCCCA585DCC";
				break;
			case SYBBIT:
			default:
				src = "1";
				break;
			}
			assert(src);
			srclen = strlen(src);
			break;
		case SYBINT1:
		case SYBUINT1:
			src = (char *) &tds_tinyint;
			srclen = sizeof(tds_tinyint);
			break;
		case SYBINT2:
			src = (char *) &tds_smallint;
			srclen = sizeof(tds_smallint);
			break;
		case SYBINT4:
			src = (char *) &tds_int;
			srclen = sizeof(tds_int);
			break;
		case SYBINT8:
			src = (char *) &tds_int8;
			srclen = sizeof(tds_int8);
			break;
		case SYBUINT2:
			src = (char *) &tds_usmallint;
			srclen = sizeof(tds_usmallint);
			break;
		case SYBUINT4:
			src = (char *) &tds_uint;
			srclen = sizeof(tds_uint);
			break;
		case SYBUINT8:
			src = (char *) &tds_uint8;
			srclen = sizeof(tds_uint8);
			break;
		case SYBFLT8:
			tds_float = 3.14159;
			src = (char *) &tds_float;
			srclen = sizeof(tds_float);
			break;
		case SYBREAL:
			tds_real = 3.14159;
			src = (char *) &tds_real;
			srclen = sizeof(tds_real);
			break;
		case SYBNUMERIC:
		case SYBDECIMAL:
			src = (char *) &numeric;
			srclen = sizeof(numeric);
			break;
		case SYBMONEY:
			src = (char *) &money;
			srclen = sizeof(money);
			break;
		case SYBMONEY4:
			src = (char *) &money4;
			srclen = sizeof(money4);
			break;
		case SYBBIT:
		case SYBBITN:
			src = (char *) &bit_input;
			srclen = sizeof(bit_input);
			break;
		case SYBDATETIME:
			src = (char *) &datetime;
			srclen = sizeof(datetime);
			break;
		case SYBDATETIME4:
			src = (char *) &datetime4;
			srclen = sizeof(datetime4);
			break;
		case SYBUNIQUE:
			src = (char *) &tds_unique;
			srclen = sizeof(tds_unique);
			break;
		/*****  not defined yet
			case SYBBOUNDARY:
			case SYBSENSITIVITY:
				fprintf (stderr, "type %d not supported\n", answers[i].srctype );
				continue;
				break;
		*****/
		default:
			fprintf(stderr, "no such type %d\n", answers[i].srctype);
			return -1;
		}

		/* 
		 * Now at last do the conversion
		 */

		result = tds_convert(ctx, answers[i].srctype, src, srclen, answers[i].desttype, &cr);
		if (result >= 0)
			free_convert(answers[i].desttype, &cr);

		if (result < 0) {
			if (result == TDS_CONVERT_NOAVAIL)	/* tds_willconvert returned true, but it lied. */
				fprintf(stderr, "Conversion not yet implemented:\n\t");

			fprintf(stderr, "failed (%d) to convert %d (%s, %d bytes) : %d (%s).\n",
				result,
				answers[i].srctype, tds_prtype(answers[i].srctype), srclen,
				answers[i].desttype, tds_prtype(answers[i].desttype));

			if (result != TDS_CONVERT_NOAVAIL)
				return result;
		}

		printf("converted %d (%s, %d bytes) : %d (%s, %d bytes).\n",
		       answers[i].srctype, tds_prtype(answers[i].srctype), srclen,
		       answers[i].desttype, tds_prtype(answers[i].desttype), result);

		/* 
		 * In the first iteration, start with varchar -> others.  
		 * By saving the output, we initialize subsequent inputs.
		 */

		switch (answers[i].desttype) {
		case SYBNUMERIC:
		case SYBDECIMAL:
			numeric = cr.n;
			break;
		case SYBMONEY:
			money = cr.m;
			break;
		case SYBMONEY4:
			money4 = cr.m4;
			break;
		case SYBDATETIME:
			datetime = cr.dt;
			break;
		case SYBDATETIME4:
			datetime4 = cr.dt4;
			break;
		case SYBINT1:
		case SYBUINT1:
			tds_tinyint = cr.ti;
			break;
		case SYBINT2:
			tds_smallint = cr.si;
			break;
		case SYBINT4:
			tds_int = cr.i;
			break;
		case SYBINT8:
			tds_int8 = cr.bi;
			break;
		case SYBUINT2:
			tds_usmallint = cr.usi;
			break;
		case SYBUINT4:
			tds_uint = cr.ui;
			break;
		case SYBUINT8:
			tds_uint8 = cr.ubi;
			break;
		case SYBUNIQUE:
			tds_unique = cr.u;
			break;
		default:
			break;
		}

		/*
		 * If an iteration count was passed on the command line (not by "make check")
		 * run the conversion N times and print the conversions per second.
		 */
		result = gettimeofday(&start, NULL);
		starttime = (double) start.tv_sec + (double) start.tv_usec * 0.000001;

		for (j = 0; result >= 0 && j < iterations; j++) {
			result = tds_convert(ctx, answers[i].srctype, src, srclen, answers[i].desttype, &cr);
			if (result >= 0)
				free_convert(answers[i].desttype, &cr);
		}
		if (result < 0)
			continue;

		result = gettimeofday(&end, NULL);
		endtime = (double) end.tv_sec + (double) end.tv_usec * 0.000001;

		if (endtime != starttime) {
			printf("%9.0f iterations/second converting %13s => %s.\n",
				j / (endtime - starttime), tds_prtype(answers[i].srctype), tds_prtype(answers[i].desttype));
		}

	}
	tds_free_context(ctx);

	return g_result;
}
