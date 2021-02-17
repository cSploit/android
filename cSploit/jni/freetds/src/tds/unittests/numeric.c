/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2005   Frediano Ziglio
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

#include "common.h"
#include <freetds/convert.h>
#include <assert.h>

/* test numeric scale */

static char software_version[] = "$Id: numeric.c,v 1.5 2009-01-16 20:27:59 jklowden Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static int g_result = 0;
static TDSCONTEXT ctx;

static void
test0(const char *src, int prec, int scale, int prec2, unsigned char scale2)
{
	int i;
	char buf[256];
	char result[256];
	char *p;
	CONV_RESULT cr;
	TDS_NUMERIC num;

	/* get initial number */
	memset(&cr.n, 0, sizeof(cr.n));
	cr.n.precision = prec;
	cr.n.scale = scale;
	if (tds_convert(&ctx, SYBVARCHAR, src, (TDS_UINT)strlen(src), SYBNUMERIC, &cr) < 0) {
		fprintf(stderr, "Error getting numeric %s(%d,%d)\n", src, prec, scale);
		exit(1);
	}
	num = cr.n;

	/* change scale with string */
	tds_numeric_to_string(&num, buf);
	while ((p = strchr(buf, '.')) != NULL)
		memmove(p, p+1, strlen(p));

	for (i = 0; i < (scale2 - scale); ++i)
		strcat(buf, "0");
	for (i = 0; i < (scale - scale2); ++i) {
		assert(strlen(buf) > 1);
		buf[strlen(buf)-1] = 0;
	}
	if (scale2) {
		size_t len = strlen(buf);
		assert(len > scale2);
		memmove(buf + len - scale2 + 1, buf + len - scale2, scale2 + 1);
		buf[len-scale2] = '.';
	}
	cr.n.precision = prec2;
	cr.n.scale = scale2;
	if (tds_convert(&ctx, SYBVARCHAR, src, (TDS_UINT)strlen(src), SYBNUMERIC, &cr) < 0)
		strcpy(buf, "error");

	/* change scale with function */
	if (tds_numeric_change_prec_scale(&num, prec2, scale2) < 0)
		strcpy(result, "error");
	else
		tds_numeric_to_string(&num, result);

	if (strcmp(buf, result) != 0) {
		fprintf(stderr, "Failed! %s (%d,%d) -> (%d,%d)\n\tshould be %s\n\tis %s\n",
			src, prec, scale, prec2, scale2, buf, result);
		g_result = 1;
		exit(1);
	} else {
		printf("%s -> %s ok!\n", src, buf);
	}
}

static void
test(const char *src, int prec, int scale, int scale2)
{
	test0(src, prec, scale, prec, scale2);
}

int
main(int argc, char **argv)
{
	int i;
	memset(&ctx, 0, sizeof(ctx));

	/* increase scale */
	for (i = 0; i < 10; ++i) {
		test("1234", 18+i, 0, 2+i);
		test("1234.1234", 18+i, 5, 3+i);
		test("1234.1234", 22+i, 5+i, 12+i);
	}

	/* overflow */
	test("1234", 4, 0, 2);
	for (i = 1; i < 12; ++i)
		test("1234", 3+i, 0, i);
	for (i = 2; i < 12; ++i)
		test("1234", 2+i, 0, i);

	/* decrease scale */
	test("1234", 10, 4, 0);
	test("1234.765", 30, 20, 2);

	test0("765432.2", 30, 2, 20, 2);
	test0("765432.2", 30, 2, 40, 2);
	test0("765432.2", 30, 2, 6, 2);

	/* test big overflows */
	test0("10000000000000000000000000", 30, 0, 10, 0);
	test0("1000000000000000000", 30, 0, 10, 0);

	test0("10000000000000000", 30, 10, 19, 0);
	test0("10000000000000000", 30, 10, 12, 0);

#if 0
	{
		int p1, s1, p2, s2;
		for (p1 = 1; p1 <= 77; ++p1) {
			printf("(%d,%d) -> (%d,%d)\n", p1, s1, p2, s2);
			for (s1 = 0; s1 < p1; ++s1)
				for (p2 = 1; p2 <= 77; ++p2)
					for (s2 = 0; s2 < p2; ++s2)
						test0("9", p1, s1, p2, s2);
		}
	}
#endif

	if (!g_result)
		printf("All passed!\n");

	return g_result;
}
