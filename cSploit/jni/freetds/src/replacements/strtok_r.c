/*
 * strtok_r(3)
 * 20020927 entropy@tappedin.com
 * public domain.  no warranty.  use at your own risk.  have a nice day.
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <freetds/sysdep_private.h>
#include "replacements.h"

TDS_RCSID(var, "$Id: strtok_r.c,v 1.8 2011-05-16 08:51:40 freddy77 Exp $");

#undef strtok_r
char *
strtok_r(char *str, const char *sep, char **lasts)
{
	char *p;

	if (str == NULL) {
		str = *lasts;
	}
	if (str == NULL) {
		return NULL;
	}
	str += strspn(str, sep);	/* skip any separators */
	if ((p = strpbrk(str, sep)) != NULL) {
		*lasts = p + 1;
		*p = '\0';
	} else {
		if (!*str)
			str = NULL;
		*lasts = NULL;
	}
	return str;
}

#ifdef TDS_INTERNAL_TEST

/* gcc -O2 -Wall strtok_r.c -o strtok -DTDS_INTERNAL_TEST -I../../include */

#include <stdlib.h>

static void
test(const char *s, const char *sep)
{
	size_t len = strlen(s);
	char *c1 = (char*) malloc(len+1);
	char *c2 = (char*) malloc(len+1);
	char *last = NULL, *s1, *s2;
	const char *p1, *p2;

	printf("testint '%s' with '%s' separator(s)\n", s, sep);
	strcpy(c1, s);
	strcpy(c2, s);

	s1 = c1;
	s2 = c2;
	for (;;) {
		p1 = strtok(s1, sep);
		p2 = strtok_r(s2, sep, &last);
		s1 = s2 = NULL;
		if ((p1 && !p2) || (!p1 && p2)) {
			fprintf(stderr, "ptr mistmach %p %p\n", p1, p2);
			exit(1);
		}
		if (!p1)
			break;
		if (strcmp(p1, p2) != 0) {
			fprintf(stderr, "string mistmach '%s' '%s'\n", p1, p2);
			exit(1);
		}
		printf("got string %s\n", p1);
	}
	printf("\n");
}

int
main(void)
{
	test("a b\tc", "\t ");
	test("    x  y \t  z", " \t");
	test("a;b;c;", ";");
	test("a;b;  c;;", ";");
	test("", ";");
	test(";;;", ";");
	return 0;
}
#endif

