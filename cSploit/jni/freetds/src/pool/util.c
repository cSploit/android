/* TDSPool - Connection pooling for TDS based databases
 * Copyright (C) 2001 Brian Bruns
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <ctype.h>

#include "pool.h"
#include <freetds/string.h>

static char software_version[] = "$Id: util.c,v 1.15 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

void
dump_buf(const void *buf, int length)
{
	int i;
	int j;
	const int bytesPerLine = 16;
	const unsigned char *data = (const unsigned char *) buf;

	for (i = 0; i < length; i += bytesPerLine) {
		fprintf(stderr, "%04x  ", i);

		for (j = i; j < length && (j - i) < bytesPerLine; j++) {
			fprintf(stderr, "%02x ", data[j]);
		}

		for (; 0 != (j % bytesPerLine); j++) {
			fprintf(stderr, "   ");
		}
		fprintf(stderr, "  |");

		for (j = i; j < length && (j - i) < bytesPerLine; j++) {
			fprintf(stderr, "%c", (isprint(data[j])) ? data[j] : '.');
		}

		fprintf(stderr, "|\n");
	}
	fprintf(stderr, "\n");
}

void
dump_login(TDSLOGIN * login)
{
	fprintf(stderr, "host %s\n", tds_dstr_cstr(&login->client_host_name));
	fprintf(stderr, "user %s\n", tds_dstr_cstr(&login->user_name));
	fprintf(stderr, "pass %s\n", tds_dstr_cstr(&login->password));
	fprintf(stderr, "app  %s\n", tds_dstr_cstr(&login->app_name));
	fprintf(stderr, "srvr %s\n", tds_dstr_cstr(&login->server_name));
	fprintf(stderr, "vers %d.%d\n", TDS_MAJOR(login), TDS_MINOR(login));
	fprintf(stderr, "lib  %s\n", tds_dstr_cstr(&login->library));
	fprintf(stderr, "lang %s\n", tds_dstr_cstr(&login->language));
	fprintf(stderr, "char %s\n", tds_dstr_cstr(&login->server_charset));
	fprintf(stderr, "bsiz %d\n", login->block_size);
}

void
die_if(int expr, const char *msg)
{
	if (expr) {
		fprintf(stderr, "%s\n", msg);
		fprintf(stderr, "tdspool aborting!\n");
		exit(1);
	}
}
