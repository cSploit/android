/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2012  Frediano Ziglio
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

static FILE *f = NULL;
static char *return_value = NULL;

static void
conf_parse(const char *option, const char *value, void *param)
{
	const char *entry = (const char *) param;
	if (strcmp(option, entry) == 0)
		return_value = strdup(value);
}

static void
test(const char *section, const char *entry, const char *expected)
{
	int fail = 0;

	rewind(f);

	tds_read_conf_section(f, section, conf_parse, (void *) entry);
	if (!expected && return_value) {
		fprintf(stderr, "return value %s NOT expected\n", return_value);
		fail = 1;
	} else if (expected && !return_value) {
		fprintf(stderr, "expected value %s NOT found\n", expected);
		fail = 1;
	} else if (expected && return_value) {
		if (strcmp(expected, return_value) != 0) {
			fprintf(stderr, "expected value %s got %s\n", expected, return_value);
			fail = 1;
		}
	}

	free(return_value);
	return_value = NULL;
	if (fail)
		exit(1);
}

int
main(int argc, char **argv)
{
	const char *in_file = FREETDS_SRCDIR "/readconf.in";

	f = fopen(in_file, "r");
	if (!f)
		f = fopen("readconf.in", "r");
	if (!f) {
		fprintf(stderr, "error opening test file\n");
		exit(1);
	}

	/* option with no spaces */
	test("section1", "opt1", "value1");

	/* option name with spaces, different case in section name */
	test("section2", "opt two", "value2");

	test("section 3", "opt three", "value three");

	fclose(f);
	return 0;
}

