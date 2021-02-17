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

static TDSCONTEXT ctx;

void test0(const char *src, int len, int dsttype, const char *result, int line);
void test(const char *src, int dsttype, const char *result, int line);

void
test0(const char *src, int len, int dsttype, const char *result, int line)
{
	int i, res;
	char buf[256];
	CONV_RESULT cr;

	res = tds_convert(&ctx, SYBVARCHAR, src, len, dsttype, &cr);
	if (res < 0)
		strcpy(buf, "error");
	else {
		buf[0] = 0;
		switch (dsttype) {
		case SYBINT1:
		case SYBUINT1:
			sprintf(buf, "%d", cr.ti);
			break;
		case SYBINT2:
			sprintf(buf, "%d", cr.si);
			break;
		case SYBUINT2:
			sprintf(buf, "%u", cr.usi);
			break;
		case SYBINT4:
			sprintf(buf, "%d", cr.i);
			break;
		case SYBUINT4:
			sprintf(buf, "%u", cr.ui);
			break;
		case SYBINT8:
			sprintf(buf, "0x%08x%08x", (unsigned int) ((cr.bi >> 32) & 0xfffffffflu), (unsigned int) (cr.bi & 0xfffffffflu));
			break;
		case SYBUINT8:
			sprintf(buf, "0x%08x%08x", (unsigned int) ((cr.ubi >> 32) & 0xfffffffflu), (unsigned int) (cr.ubi & 0xfffffffflu));
			break;
		case SYBUNIQUE:
			sprintf(buf, "%08X-%04X-%04X-%02X%02X%02X%02X"
				"%02X%02X%02X%02X",
				cr.u.Data1,
				cr.u.Data2, cr.u.Data3,
				cr.u.Data4[0], cr.u.Data4[1],
				cr.u.Data4[2], cr.u.Data4[3], cr.u.Data4[4], cr.u.Data4[5], cr.u.Data4[6], cr.u.Data4[7]);
			break;
		case SYBBINARY:
			sprintf(buf, "len=%d", res);
			for (i = 0; i < res; ++i)
				sprintf(strchr(buf, 0), " %02X", (TDS_UCHAR) cr.ib[i]);
			free(cr.ib);
			break;
		case SYBDATETIME:
			sprintf(buf, "%ld %ld", (long int) cr.dt.dtdays, (long int) cr.dt.dttime);
			break;
		}
	}
	printf("%s\n", buf);
	if (strcmp(buf, result) != 0) {
		fprintf(stderr, "Expected '%s' got '%s' at line %d\n", result, buf, line);
		exit(1);
	}
}

void
test(const char *src, int dsttype, const char *result, int line)
{
	test0(src, strlen(src), dsttype, result, line);
}

#define test0(s,l,d,r) test0(s,l,d,r,__LINE__)
#define test(s,d,r) test(s,d,r,__LINE__)

static int
int_types[] = {
	SYBINT1, SYBUINT1, SYBINT2, SYBUINT2,
	SYBINT4, SYBUINT4, SYBINT8, SYBUINT8,
	SYBMONEY4, SYBMONEY,
	SYBNUMERIC,
	-1
};

static const char *
int_values[] = {
	"0",
	"127", "255",
	"128", "256",
	"32767", "65535",
	"32768", "65536",
	"214748",
	"214749",
	"2147483647", "4294967295",
	"2147483648", "4294967296",
	"922337203685477",
	"922337203685478",
	"9223372036854775807", "18446744073709551615",
	"9223372036854775808", "18446744073709551616",
	"-128",
	"-129",
	"-32768",
	"-32769",
	"-214748",
	"-214749",
	"-2147483648",
	"-2147483649",
	"-922337203685477",
	"-922337203685478",
	"-9223372036854775808",
	"-9223372036854775809",
	NULL
};

int
main(int argc, char **argv)
{
	int *type1, *type2;
	const char **value;

	memset(&ctx, 0, sizeof(ctx));

	/* test some conversion */
	printf("some checks...\n");
	test("1234", SYBINT4, "1234");
	test("1234", SYBUINT4, "1234");
	test("123", SYBINT1, "123");
	test("123", SYBUINT1, "123");
	test("  -    1234   ", SYBINT2, "-1234");
	test("  -    1234   a", SYBINT2, "error");
	test("", SYBINT4, "0");
	test("    ", SYBINT4, "0");
	test("    123", SYBINT4, "123");
	test("    123    ", SYBINT4, "123");
	test("  +  123  ", SYBINT4, "123");
	test("  +  123  ", SYBUINT4, "123");
	test("  - 0  ", SYBINT4, "0");
	test("  -  0  ", SYBUINT4, "0");
	test("+", SYBINT4, "error");
	test("   +", SYBINT4, "error");
	test("+   ", SYBINT4, "error");
	test("   +   ", SYBINT4, "error");
	test("-", SYBINT4, "error");
	test("   -", SYBINT4, "error");
	test("-   ", SYBINT4, "error");
	test("   -   ", SYBINT4, "error");

	test("  -    1234   ", SYBINT8, "0xfffffffffffffb2e");
	test("1234x", SYBINT8, "error");
	test("  -    1234   a", SYBINT8, "error");
	test("", SYBINT8, "0x0000000000000000");
	test("    ", SYBINT8, "0x0000000000000000");
	test("    123", SYBINT8, "0x000000000000007b");
	test("    123    ", SYBINT8, "0x000000000000007b");
	test("  +  123  ", SYBINT8, "0x000000000000007b");
	test("    123", SYBUINT8, "0x000000000000007b");
	test("    123    ", SYBUINT8, "0x000000000000007b");
	test("  +  123  ", SYBUINT8, "0x000000000000007b");
	test("+", SYBINT8, "error");
	test("   +", SYBINT8, "error");
	test("+   ", SYBINT8, "error");
	test("   +   ", SYBINT8, "error");
	test("-", SYBINT8, "error");
	test("   -", SYBINT8, "error");
	test("-   ", SYBINT8, "error");
	test("   -   ", SYBINT8, "error");

	/* test for overflow */
	/* for SYBUINT8 a test with all different digit near limit is required */
	printf("overflow checks...\n");
	test("9223372036854775807", SYBINT8, "0x7fffffffffffffff");
	test("9223372036854775807", SYBUINT8, "0x7fffffffffffffff");
	test("9223372036854775808", SYBINT8, "error");
	test("-9223372036854775808", SYBINT8, "0x8000000000000000");
	test("9223372036854775808", SYBUINT8, "0x8000000000000000");
	test("18446744073709551610", SYBUINT8, "0xfffffffffffffffa");
	test("18446744073709551611", SYBUINT8, "0xfffffffffffffffb");
	test("18446744073709551612", SYBUINT8, "0xfffffffffffffffc");
	test("18446744073709551613", SYBUINT8, "0xfffffffffffffffd");
	test("18446744073709551614", SYBUINT8, "0xfffffffffffffffe");
	test("18446744073709551615", SYBUINT8, "0xffffffffffffffff");
	test("18446744073709551616", SYBUINT8, "error");
	test("18446744073709551617", SYBUINT8, "error");
	test("18446744073709551618", SYBUINT8, "error");
	test("18446744073709551619", SYBUINT8, "error");
	test("18446744073709551620", SYBUINT8, "error");
	test("-1", SYBUINT8, "error");
	test("-9223372036854775809", SYBINT8, "error");
	test("2147483647", SYBINT4, "2147483647");
	test("2147483648", SYBINT4, "error");
	test("2147483647", SYBUINT4, "2147483647");
	test("4294967295", SYBUINT4, "4294967295");
	test("4294967296", SYBUINT4, "error");
	test("-2147483648", SYBINT4, "-2147483648");
	test("-2147483648", SYBUINT4, "error");
	test("-2147483649", SYBINT4, "error");
	test("32767", SYBINT2, "32767");
	test("32767", SYBUINT2, "32767");
	test("65535", SYBUINT2, "65535");
	test("65536", SYBUINT2, "error");
	test("32768", SYBINT2, "error");
	test("-32768", SYBINT2, "-32768");
	test("-32769", SYBINT2, "error");
	test("255", SYBINT1, "255");
	test("256", SYBINT1, "error");
	test("255", SYBUINT1, "255");
	test("256", SYBUINT1, "error");
	test("0", SYBINT1, "0");
	test("-1", SYBINT1, "error");
	test("0", SYBUINT1, "0");
	test("-1", SYBUINT1, "error");

	/*
	 * test overflow on very big numbers 
	 * i use increment of 10^9 to be sure lower 32bit be correct
	 * in a case
	 */
	printf("overflow on big number checks...\n");
	test("62147483647", SYBINT4, "error");
	test("63147483647", SYBINT4, "error");
	test("64147483647", SYBINT4, "error");
	test("65147483647", SYBINT4, "error");
	test("53248632876323876761", SYBINT8, "error");
	test("56248632876323876761", SYBINT8, "error");
	test("59248632876323876761", SYBINT8, "error");
	test("12248632876323876761", SYBINT8, "error");

	/* test not terminated string */
	test0("1234", 2, SYBINT4, "12");
	test0("123456", 4, SYBINT8, "0x00000000000004d2");

	/* some test for unique */
	printf("unique type...\n");
	test("12345678-1234-1234-9876543298765432", SYBUNIQUE, "12345678-1234-1234-9876543298765432");
	test("{12345678-1234-1E34-9876ab3298765432}", SYBUNIQUE, "12345678-1234-1E34-9876AB3298765432");
	test(" 12345678-1234-1234-9876543298765432", SYBUNIQUE, "error");
	test(" {12345678-1234-1234-9876543298765432}", SYBUNIQUE, "error");
	test("12345678-1234-G234-9876543298765432", SYBUNIQUE, "error");
	test("12345678-1234-a234-9876543298765432", SYBUNIQUE, "12345678-1234-A234-9876543298765432");
	test("123a5678-1234-a234-98765-43298765432", SYBUNIQUE, "error");
	test("123-5678-1234-a234-9876543298765432", SYBUNIQUE, "error");

	printf("binary test...\n");
	test("0x1234", SYBBINARY, "len=2 12 34");
	test("0xaBFd  ", SYBBINARY, "len=2 AB FD");
	test("AbfD  ", SYBBINARY, "len=2 AB FD");
	test("0x000", SYBBINARY, "len=2 00 00");
	test("0x0", SYBBINARY, "len=1 00");
	test("0x100", SYBBINARY, "len=2 01 00");
	test("0x1", SYBBINARY, "len=1 01");

	test("Jan 01 2006", SYBDATETIME, "38716 0");
	test("January 01 2006", SYBDATETIME, "38716 0");
	test("March 05 2005", SYBDATETIME, "38414 0");
	test("may 13 2001", SYBDATETIME, "37022 0");

	test("02 Jan 2006", SYBDATETIME, "38717 0");
	test("2 Jan 2006", SYBDATETIME, "38717 0");
	test("02Jan2006", SYBDATETIME, "38717 0");
	test("20060102", SYBDATETIME, "38717 0");
	test("060102", SYBDATETIME, "38717 0");

	/* now try many int conversion operations */
	for (value = int_values; *value; ++value)
	for (type1 = int_types; *type1 >= 0; ++type1)
	for (type2 = int_types; *type2 >= 0; ++type2) {
		char buf[64], expected[64];
		CONV_RESULT cr_src, cr_dst;
		TDS_INT len_src, len_dst;

		/* try conversion from char (already tested above) */
		cr_src.n.precision = 20; cr_src.n.scale = 0;
		len_src = tds_convert(&ctx, SYBVARCHAR, *value, strlen(*value), *type1, &cr_src);
		cr_dst.n.precision = 20; cr_dst.n.scale = 0;
		len_dst = tds_convert(&ctx, SYBVARCHAR, *value, strlen(*value), *type2, &cr_dst);
		if (len_src <= 0 || len_dst <= 0)
			continue;
		cr_dst.n.precision = 20; cr_dst.n.scale = 0;
		if (tds_convert(&ctx, *type1, (const TDS_CHAR *) &cr_src.i, len_src, *type2, &cr_dst) <= 0) {
			fprintf(stderr, "conversion from %s to %s of %s should succeed\n",
				tds_prtype(*type1), tds_prtype(*type2), *value);
			return 1;
		}
		memcpy(&cr_src, &cr_dst, sizeof(cr_dst));
		cr_dst.cc.c = buf;
		cr_dst.cc.len = sizeof(buf)-4;
		len_dst = tds_convert(&ctx, *type2, (const TDS_CHAR *) &cr_src.i, len_dst, TDS_CONVERT_CHAR, &cr_dst);
		if (len_dst <= 0) {
			fprintf(stderr, "conversion from %s to string should succeed\n",
				tds_prtype(*type1));
			return 1;
		}
		buf[len_dst] = 0;
		if (*type2 == SYBMONEY4 || *type2 == SYBMONEY)
			sprintf(expected, "%s.00", *value);
		else
			strcpy(expected, *value);
		if (strcmp(buf, expected) != 0) {
			fprintf(stderr, "conversion from %s to %s of %s got wrong value '%s'\n",
				tds_prtype(*type1), tds_prtype(*type2), *value, buf);
			return 1;
		}
	}

	return 0;
}
