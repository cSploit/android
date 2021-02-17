/* TDSPool - Connection pooling for TDS based databases
 * Copyright (C) 2001 Brian Bruns
 * Copyright (C) 2005 Frediano Ziglio
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
#include <ctype.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif

#include "pool.h"
#include <freetds/configs.h>

TDS_RCSID(var, "$Id: config.c,v 1.17 2011-05-16 08:51:40 freddy77 Exp $");

#define POOL_STR_SERVER	"server"
#define POOL_STR_PORT	"port"
#define POOL_STR_USER	"user"
#define POOL_STR_PASSWORD	"password"
#define POOL_STR_DATABASE	"database"
#define POOL_STR_MAX_MBR_AGE	"max member age"
#define POOL_STR_MAX_POOL_CONN	"max pool conn"
#define POOL_STR_MIN_POOL_CONN	"min pool conn"
#define POOL_STR_MAX_POOL_USERS	"max pool users"

static void pool_parse(const char *option, const char *value, void *param);

int
pool_read_conf_file(char *poolname, TDS_POOL * pool)
{
	FILE *in;
	int found = 0;

	in = fopen(FREETDS_POOLCONFFILE, "r");
	if (in) {
		fprintf(stderr, "Found conf file in %s reading sections\n", FREETDS_POOLCONFFILE);
		tds_read_conf_section(in, "global", pool_parse, pool);
		rewind(in);
		found = tds_read_conf_section(in, poolname, pool_parse, pool);
		fclose(in);
	}

	return found;
}

#if 0
static int
pool_config_boolean(char *value)
{
	if (!strcmp(value, "yes") || !strcmp(value, "on") || !strcmp(value, "true") || !strcmp(value, "1")) {
		return 1;
	} else {
		return 0;
	}
}
#endif

static void
pool_parse(const char *option, const char *value, void *param)
{
	TDS_POOL *pool = (TDS_POOL *) param;

	if (!strcmp(option, POOL_STR_PORT)) {
		if (atoi(value))
			pool->port = atoi(value);
	} else if (!strcmp(option, POOL_STR_SERVER)) {
		free(pool->server);
		pool->server = strdup(value);
	} else if (!strcmp(option, POOL_STR_USER)) {
		free(pool->user);
		pool->user = strdup(value);
	} else if (!strcmp(option, POOL_STR_DATABASE)) {
		free(pool->database);
		pool->database = strdup(value);
	} else if (!strcmp(option, POOL_STR_PASSWORD)) {
		free(pool->password);
		pool->password = strdup(value);
	} else if (!strcmp(option, POOL_STR_MAX_MBR_AGE)) {
		if (atoi(value))
			pool->max_member_age = atoi(value);
	} else if (!strcmp(option, POOL_STR_MAX_POOL_CONN)) {
		if (atoi(value))
			pool->max_open_conn = atoi(value);
	} else if (!strcmp(option, POOL_STR_MIN_POOL_CONN)) {
		if (atoi(value))
			pool->min_open_conn = atoi(value);
	}
}
