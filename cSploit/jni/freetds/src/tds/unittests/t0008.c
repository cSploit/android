/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-1999  Brian Bruns
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

static char software_version[] = "$Id: t0008.c,v 1.13 2003-04-21 16:06:11 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

int g_result = 0;
static TDSCONTEXT ctx;

void test(const char *src, const char *intro, const char *cont, int prec, int scale, int n2s, int line);

void
test(const char *src, const char *intro, const char *cont, int prec, int scale, int n2s, int line)
{
	int i;
	char buf[256];
	char result[256];
	CONV_RESULT cr;

	/* build result string from intro and cont */
	strcpy(result, intro);
	if (*cont) {
		for (i = 0; i < sizeof(cr.n.array); ++i)
			strcat(result, " 00");
		memcpy(result + strlen(intro) + 1, cont, strlen(cont));
	}

	memset(&cr.n, 0, sizeof(cr.n));
	cr.n.precision = prec;
	cr.n.scale = scale;
	if (tds_convert(&ctx, SYBVARCHAR, src, strlen(src), SYBNUMERIC, &cr) < 0)
		strcpy(buf, "error");
	else {
		sprintf(buf, "prec=%d scale=%d", cr.n.precision, cr.n.scale);
		for (i = 0; i < sizeof(cr.n.array); ++i)
			sprintf(strchr(buf, 0), " %02X", cr.n.array[i]);
	}
	printf("%s\n", buf);
	if (strcmp(buf, result) != 0) {
		fprintf(stderr, "line %d: Failed! Should be\n\t%s\n", line, result);
		g_result = 1;
	}

	if (!n2s || strcmp(buf, "error") == 0)
		return;
	tds_numeric_to_string(&cr.n, buf);
	printf("%s\n", buf);
	if (strcmp(buf, src) != 0) {
		fprintf(stderr, "line %d: Failed! Should be\n\t%s\n", line, src);
		g_result = 1;
	}
}

#define test(a,b,c,d,e,f) test(a,b,c,d,e,f,__LINE__)

int
main(int argc, char **argv)
{
	/* very long string for test buffer overflow */
	int i;
	char long_test[201];

	memset(&ctx, 0, sizeof(ctx));

	printf("test some valid values..\n");
	test("    1234", "prec=18 scale=0", "00 00 00 00 00 00 00 04 D2", 18, 0, 0);
	test("1234567890", "prec=18 scale=0", "00 00 00 00 00 49 96 02 D2", 18, 0, 1);
	test("123456789012345678", "prec=18 scale=0", "00 01 B6 9B 4B A6 30 F3 4E", 18, 0, 1);
	test("999999999999999999", "prec=18 scale=0", "00 0D E0 B6 B3 A7 63 FF FF", 18, 0, 1);

	printf("test overflow..\n");
	test("123456789012345678901234567890", "error", "", 18, 0, 0);

	long_test[0] = 0;
	for (i = 0; i < 20; ++i)
		strcat(long_test, "1234567890");
	test(long_test, "error", "", 18, 0, 0);

	test("123456789012345678901234567890", "prec=38 scale=0", "00 00 00 00 01 8E E9 0F F6 C3 73 E0 EE 4E 3F 0A D2", 38, 0, 1);
	test("1234567890123456789012345678901234567890123456789012345678901234567890-00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00", "error", "", 38, 0, 1);
	test("99999999999999999999999999999999999999",
	     "prec=38 scale=0", "00 4B 3B 4C A8 5A 86 C4 7A 09 8A 22 3F FF FF FF FF", 38, 0, 1);
	test("100000000000000000000000000000000000000", "error", "", 38, 0, 1);

	test("    1234", "prec=18 scale=1", "00 00 00 00 00 00 00 30 34", 18, 1, 0);

	test("1234.56", "prec=18 scale=2", "00 00 00 00 00 00 01 E2 40", 18, 2, 1);

	test("0.00123", "prec=18 scale=5", "00 00 00 00 00 00 00 00 7B", 18, 5, 1);

	test("0.0123", "prec=18 scale=4", "00 00 00 00 00 00 00 00 7B", 18, 4, 1);

	test("0.123", "prec=18 scale=3", "00 00 00 00 00 00 00 00 7B", 18, 3, 1);

	test("1.23", "prec=18 scale=2", "00 00 00 00 00 00 00 00 7B", 18, 2, 1);

	test("12.3", "prec=18 scale=1", "00 00 00 00 00 00 00 00 7B", 18, 1, 1);

	test("0.0000", "prec=18 scale=4", "00", 18, 4, 1);

	test("0", "prec=18 scale=0", "00", 18, 0, 1);

	return g_result;
}
