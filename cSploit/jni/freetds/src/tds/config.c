/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011  Frediano Ziglio
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

#include <config.h>

#include <stdarg.h>
#include <stdio.h>

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#include <assert.h>
#include <ctype.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_LIMITS_H
#include <limits.h>
#endif 

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */

#ifdef _WIN32
#include <process.h>
#endif

#include <freetds/tds.h>
#include <freetds/configs.h>
#include <freetds/string.h>
#include "replacements.h"
#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: config.c,v 1.177 2012-03-11 15:52:22 freddy77 Exp $");

static void tds_config_login(TDSLOGIN * connection, TDSLOGIN * login);
static void tds_config_env_tdsdump(TDSLOGIN * login);
static void tds_config_env_tdsver(TDSLOGIN * login);
static void tds_config_env_tdsport(TDSLOGIN * login);
static void tds_config_env_tdshost(TDSLOGIN * login);
static int tds_read_conf_sections(FILE * in, const char *server, TDSLOGIN * login);
static int tds_read_interfaces(const char *server, TDSLOGIN * login);
static int parse_server_name_for_port(TDSLOGIN * connection, TDSLOGIN * login);
static int tds_lookup_port(const char *portname);
static void tds_config_encryption(const char * value, TDSLOGIN * login);

static char *interf_file = NULL;

#define TDS_ISSPACE(c) isspace((unsigned char ) (c))

#if !defined(_WIN32) && !defined(DOS32X)
       const char STD_DATETIME_FMT[] = "%b %e %Y %I:%M%p";
static const char pid_config_logpath[] = "/tmp/tdsconfig.log.%d";
static const char freetds_conf[] = "%s/etc/freetds.conf";
static const char location[] = "(from $FREETDS/etc)";
static const char pid_logpath[] = "/tmp/freetds.log.%d";
static const char interfaces_path[] = "/etc/freetds";
#else
       const char STD_DATETIME_FMT[] = "%b %d %Y %I:%M%p"; /* msvcr80.dll does not support %e */
static const char pid_config_logpath[] = "c:\\tdsconfig.log.%d";
static const char freetds_conf [] = "%s\\freetds.conf";
static const char location[] = "(from $FREETDS)";
static const char pid_logpath[] = "c:\\freetds.log.%d";
static const char interfaces_path[] = "c:\\";
#endif

/**
 * \ingroup libtds
 * \defgroup config Configuration
 * Handle reading of configuration
 */

/**
 * \addtogroup config
 * @{ 
 */

/**
 * tds_read_config_info() will fill the tds connection structure based on configuration 
 * information gathered in the following order:
 * 1) Program specified in TDSLOGIN structure
 * 2) The environment variables TDSVER, TDSDUMP, TDSPORT, TDSQUERY, TDSHOST
 * 3) A config file with the following search order:
 *    a) a readable file specified by environment variable FREETDSCONF
 *    b) a readable file in ~/.freetds.conf
 *    c) a readable file in $prefix/etc/freetds.conf
 * 3) ~/.interfaces if exists
 * 4) $SYBASE/interfaces if exists
 * 5) TDS_DEF_* default values
 *
 * .tdsrc and freetds.conf have been added to make the package easier to 
 * integration with various Linux and *BSD distributions.
 */
TDSLOGIN *
tds_read_config_info(TDSSOCKET * tds, TDSLOGIN * login, TDSLOCALE * locale)
{
	TDSLOGIN *connection;
	char *s;
	char *path;
	pid_t pid;
	int opened = 0, found;
	struct tds_addrinfo *addrs;

	/* allocate a new structure with hard coded and build-time defaults */
	connection = tds_alloc_login(0);
	if (!connection || !tds_init_login(connection, locale)) {
		tds_free_login(connection);
		return NULL;
	}

	s = getenv("TDSDUMPCONFIG");
	if (s) {
		if (*s) {
			opened = tdsdump_open(s);
		} else {
			pid = getpid();
			if (asprintf(&path, pid_config_logpath, pid) >= 0) {
				if (*path) {
					opened = tdsdump_open(path);
				}
				free(path);
			}
		}
	}

	tdsdump_log(TDS_DBG_INFO1, "Getting connection information for [%s].\n", 
			    tds_dstr_cstr(&login->server_name));	/* (The server name is set in login.c.) */

	/* Read the config files. */
	tdsdump_log(TDS_DBG_INFO1, "Attempting to read conf files.\n");
	found = tds_read_conf_file(connection, tds_dstr_cstr(&login->server_name));
	if (!found) {
		if (parse_server_name_for_port(connection, login)) {

			found = tds_read_conf_file(connection, tds_dstr_cstr(&connection->server_name));
			/* do it again to really override what found in freetds.conf */
			if (found) {
				parse_server_name_for_port(connection, login);
			} else if (TDS_SUCCEED(tds_lookup_host_set(tds_dstr_cstr(&connection->server_name), &connection->ip_addrs))) {
				tds_dstr_dup(&connection->server_host_name, &connection->server_name);
				found = 1;
			}
		}
	}
	if (!found) {
		/* fallback to interfaces file */
		tdsdump_log(TDS_DBG_INFO1, "Failed in reading conf file.  Trying interface files.\n");
		if (!tds_read_interfaces(tds_dstr_cstr(&login->server_name), connection)) {
			tdsdump_log(TDS_DBG_INFO1, "Failed to find [%s] in configuration files; trying '%s' instead.\n", 
						   tds_dstr_cstr(&login->server_name), tds_dstr_cstr(&connection->server_name));
			if (connection->ip_addrs == NULL)
				tdserror(tds_get_ctx(tds), tds, TDSEINTF, 0);
		}
	}

	/* Override config file settings with environment variables. */
	tds_fix_login(connection);

	/* And finally apply anything from the login structure */
	tds_config_login(connection, login);
	
	if (opened) {
		char tmp[128];

		tdsdump_log(TDS_DBG_INFO1, "Final connection parameters:\n");
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "server_name", tds_dstr_cstr(&connection->server_name));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "server_host_name", tds_dstr_cstr(&connection->server_host_name));

		for (addrs = connection->ip_addrs; addrs != NULL; addrs = addrs->ai_next)
			tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "ip_addr", tds_addrinfo2str(addrs, tmp, sizeof(tmp)));

		if (connection->ip_addrs == NULL)
			tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "ip_addr", "");

		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "instance_name", tds_dstr_cstr(&connection->instance_name));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "port", connection->port);
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "major_version", TDS_MAJOR(connection));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "minor_version", TDS_MINOR(connection));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "block_size", connection->block_size);
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "language", tds_dstr_cstr(&connection->language));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "server_charset", tds_dstr_cstr(&connection->server_charset));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "connect_timeout", connection->connect_timeout);
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "client_host_name", tds_dstr_cstr(&connection->client_host_name));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "client_charset", tds_dstr_cstr(&connection->client_charset));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "app_name", tds_dstr_cstr(&connection->app_name));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "user_name", tds_dstr_cstr(&connection->user_name));
		/* tdsdump_log(TDS_DBG_PASSWD, "\t%20s = %s\n", "password", tds_dstr_cstr(&connection->password)); 
			(no such flag yet) */
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "library", tds_dstr_cstr(&connection->library));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "bulk_copy", (int)connection->bulk_copy);
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "suppress_language", (int)connection->suppress_language);
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "encrypt level", (int)connection->encryption_level);
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "query_timeout", connection->query_timeout);
		/* tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "capabilities", tds_dstr_cstr(&connection->capabilities)); 
			(not null terminated) */
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "database", tds_dstr_cstr(&connection->database));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "dump_file", tds_dstr_cstr(&connection->dump_file));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %x\n", "debug_flags", connection->debug_flags);
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "text_size", connection->text_size);
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "broken_dates", connection->broken_dates);
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %d\n", "emul_little_endian", connection->emul_little_endian);
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "server_realm_name", tds_dstr_cstr(&connection->server_realm_name));
		tdsdump_log(TDS_DBG_INFO1, "\t%20s = %s\n", "server_spn", tds_dstr_cstr(&connection->server_spn));

		tdsdump_close();
	}

	/*
	 * If a dump file has been specified, start logging
	 */
	if (!tds_dstr_isempty(&connection->dump_file) && !tdsdump_isopen()) {
		if (connection->debug_flags)
			tds_debug_flags = connection->debug_flags;
		tdsdump_open(tds_dstr_cstr(&connection->dump_file));
	}

	return connection;
}

/**
 * Fix configuration after reading it. 
 * Currently this read some environment variables and replace some options.
 */
void
tds_fix_login(TDSLOGIN * login)
{
	/* Now check the environment variables */
	tds_config_env_tdsver(login);
	tds_config_env_tdsdump(login);
	tds_config_env_tdsport(login);
	tds_config_env_tdshost(login);
}

static int
tds_try_conf_file(const char *path, const char *how, const char *server, TDSLOGIN * login)
{
	int found = 0;
	FILE *in;

	if ((in = fopen(path, "r")) == NULL) {
		tdsdump_log(TDS_DBG_INFO1, "Could not open '%s' (%s).\n", path, how);
		return found;
	}

	tdsdump_log(TDS_DBG_INFO1, "Found conf file '%s' %s.\n", path, how);
	found = tds_read_conf_sections(in, server, login);

	if (found) {
		tdsdump_log(TDS_DBG_INFO1, "Success: [%s] defined in %s.\n", server, path);
	} else {
		tdsdump_log(TDS_DBG_INFO2, "[%s] not found.\n", server);
	}

	fclose(in);

	return found;
}


/**
 * Return filename from HOME directory
 * @return allocated string or NULL if error
 */
static char *
tds_get_home_file(const char *file)
{
	char *home, *path;

	home = tds_get_homedir();
	if (!home)
		return NULL;
	if (asprintf(&path, "%s/%s", home, file) < 0)
		path = NULL;
	free(home);
	return path;
}

/**
 * Read configuration info for given server
 * return 0 on error
 * @param login where to store configuration
 * @param server       section of file configuration that hold 
 *                     configuration for a server
 */
int
tds_read_conf_file(TDSLOGIN * login, const char *server)
{
	char *path = NULL;
	char *eptr = NULL;
	int found = 0;

	if (interf_file) {
		found = tds_try_conf_file(interf_file, "set programmatically", server, login);
	}

	/* FREETDSCONF env var, pkleef@openlinksw.com 01/21/02 */
	if (!found) {
		path = getenv("FREETDSCONF");
		if (path) {
			found = tds_try_conf_file(path, "(from $FREETDSCONF)", server, login);
		} else {
			tdsdump_log(TDS_DBG_INFO2, "... $FREETDSCONF not set.  Trying $FREETDS/etc.\n");
		}
	}

	/* FREETDS env var, Bill Thompson 16/07/03 */
	if (!found) {
		eptr = getenv("FREETDS");
		if (eptr) {
			if (asprintf(&path, freetds_conf, eptr) >= 0) {
				found = tds_try_conf_file(path, location, server, login);
				free(path);
			}
		} else {
			tdsdump_log(TDS_DBG_INFO2, "... $FREETDS not set.  Trying $HOME.\n");
		}
	}

	if (!found) {
		path = tds_get_home_file(".freetds.conf");
		if (path) {
			found = tds_try_conf_file(path, "(.freetds.conf)", server, login);
			free(path);
		} else {
			tdsdump_log(TDS_DBG_INFO2, "... Error getting ~/.freetds.conf.  Trying %s.\n", FREETDS_SYSCONFFILE);
		}
	}

	if (!found) {
		found = tds_try_conf_file(FREETDS_SYSCONFFILE, "(default)", server, login);
	}

	return found;
}

static int
tds_read_conf_sections(FILE * in, const char *server, TDSLOGIN * login)
{
	DSTR default_instance;
	int default_port;

	int found;

	tds_read_conf_section(in, "global", tds_parse_conf_section, login);

	if (!server[0])
		return 0;
	rewind(in);

	tds_dstr_init(&default_instance);
	tds_dstr_dup(&default_instance, &login->instance_name);
	default_port = login->port;

	found = tds_read_conf_section(in, server, tds_parse_conf_section, login);
	if (!login->valid_configuration) {
		tds_dstr_free(&default_instance);
		return 0;
	}

	/* 
	 * If both instance and port are specified and neither one came from the default, it's an error 
	 * TODO: If port/instance is specified in the non-default, it has priority over the default setting. 
	 * TODO: test this. 
	 */
	if (!tds_dstr_isempty(&login->instance_name) && login->port &&
	    !(!tds_dstr_isempty(&default_instance) || default_port)) {
		tdsdump_log(TDS_DBG_ERROR, "error: cannot specify both port %d and instance %s.\n", 
						login->port, tds_dstr_cstr(&login->instance_name));
		/* tdserror(tds_get_ctx(tds), tds, TDSEPORTINSTANCE, 0); */
	}
	tds_dstr_free(&default_instance);
	return found;
}

static const struct {
	char value[7];
	unsigned char to_return;
} boolean_values[] = {
	{ "yes",	1 },
	{ "no",		0 },
	{ "on",		1 },
	{ "off",	0 },
	{ "true",	1 },
	{ "false",	0 }
};

int
tds_config_boolean(const char *option, const char *value, TDSLOGIN *login)
{
	int p;

	for (p = 0; p < TDS_VECTOR_SIZE(boolean_values); ++p) {
		if (!strcasecmp(value, boolean_values[p].value))
			return boolean_values[p].to_return;
	}
	tdsdump_log(TDS_DBG_ERROR, "UNRECOGNIZED option value '%s' for boolean setting '%s'!\n",
		    value, option);
	login->valid_configuration = 0;
	return 0;
}

static void
tds_config_encryption(const char * value, TDSLOGIN * login)
{
	TDS_ENCRYPTION_LEVEL lvl = TDS_ENCRYPTION_OFF;

	if (!strcasecmp(value, TDS_STR_ENCRYPTION_OFF))
		;
	else if (!strcasecmp(value, TDS_STR_ENCRYPTION_REQUEST))
		lvl = TDS_ENCRYPTION_REQUEST;
	else if (!strcasecmp(value, TDS_STR_ENCRYPTION_REQUIRE))
		lvl = TDS_ENCRYPTION_REQUIRE;
	else {
		tdsdump_log(TDS_DBG_ERROR, "UNRECOGNIZED option value '%s' for '%s' setting!\n",
			    value, TDS_STR_ENCRYPTION);
		tdsdump_log(TDS_DBG_ERROR, "Valid settings are: ('%s', '%s', '%s')\n",
		        TDS_STR_ENCRYPTION_OFF, TDS_STR_ENCRYPTION_REQUEST, TDS_STR_ENCRYPTION_REQUIRE);
		lvl = TDS_ENCRYPTION_REQUIRE;  /* Assuming "require" is safer than "no" */
		login->valid_configuration = 0;
	}

	login->encryption_level = lvl;
}

/**
 * Read a section of configuration file (INI style file)
 * @param in             configuration file
 * @param section        section to read
 * @param tds_conf_parse callback that receive every entry in section
 * @param param          parameter to pass to callback function
 */
int
tds_read_conf_section(FILE * in, const char *section, TDSCONFPARSE tds_conf_parse, void *param)
{
	char line[256], *value;
#define option line
	char *s;
	char p;
	int i;
	int insection = 0;
	int found = 0;

	tdsdump_log(TDS_DBG_INFO1, "Looking for section %s.\n", section);
	while (fgets(line, sizeof(line), in)) {
		s = line;

		/* skip leading whitespace */
		while (*s && TDS_ISSPACE(*s))
			s++;

		/* skip it if it's a comment line */
		if (*s == ';' || *s == '#')
			continue;

		/* read up to the = ignoring duplicate spaces */
		p = 0;
		i = 0;
		while (*s && *s != '=') {
			if (!TDS_ISSPACE(*s)) {
				if (TDS_ISSPACE(p))
					option[i++] = ' ';
				option[i++] = tolower((unsigned char) *s);
			}
			p = *s;
			s++;
		}

		/* skip if empty option */
		if (!i)
			continue;

		/* skip the = */
		if (*s)
			s++;

		/* terminate the option, must be done after skipping = */
		option[i] = '\0';

		/* skip leading whitespace */
		while (*s && TDS_ISSPACE(*s))
			s++;

		/* read up to a # ; or null ignoring duplicate spaces */
		value = s;
		p = 0;
		i = 0;
		while (*s && *s != ';' && *s != '#') {
			if (!TDS_ISSPACE(*s)) {
				if (TDS_ISSPACE(p))
					value[i++] = ' ';
				value[i++] = *s;
			}
			p = *s;
			s++;
		}
		value[i] = '\0';

		if (option[0] == '[') {
			s = strchr(option, ']');
			if (s)
				*s = '\0';
			tdsdump_log(TDS_DBG_INFO1, "\tFound section %s.\n", &option[1]);

			if (!strcasecmp(section, &option[1])) {
				tdsdump_log(TDS_DBG_INFO1, "Got a match.\n");
				insection = 1;
				found = 1;
			} else {
				insection = 0;
			}
		} else if (insection) {
			tds_conf_parse(option, value, param);
		}

	}
	tdsdump_log(TDS_DBG_INFO1, "\tReached EOF\n");
	return found;
#undef option
}

/* Also used to scan ODBC.INI entries */
void
tds_parse_conf_section(const char *option, const char *value, void *param)
{
	TDSLOGIN *login = (TDSLOGIN *) param;

	tdsdump_log(TDS_DBG_INFO1, "\t%s = '%s'\n", option, value);

	if (!strcmp(option, TDS_STR_VERSION)) {
		tds_config_verstr(value, login);
	} else if (!strcmp(option, TDS_STR_BLKSZ)) {
		int val = atoi(value);
		if (val >= 512 && val < 65536)
			login->block_size = val;
	} else if (!strcmp(option, TDS_STR_SWAPDT)) {
		login->broken_dates = tds_config_boolean(option, value, login);
	} else if (!strcmp(option, TDS_GSSAPI_DELEGATION)) {
		/* gssapi flag addition */
		login->gssapi_use_delegation = tds_config_boolean(option, value, login);
	} else if (!strcmp(option, TDS_STR_DUMPFILE)) {
		tds_dstr_copy(&login->dump_file, value);
	} else if (!strcmp(option, TDS_STR_DEBUGFLAGS)) {
		char *end;
		long flags;
		flags = strtol(value, &end, 0);
		if (*value != '\0' && *end == '\0' && flags != LONG_MIN && flags != LONG_MAX)
			login->debug_flags = flags;
	} else if (!strcmp(option, TDS_STR_TIMEOUT) || !strcmp(option, TDS_STR_QUERY_TIMEOUT)) {
		if (atoi(value))
			login->query_timeout = atoi(value);
	} else if (!strcmp(option, TDS_STR_CONNTIMEOUT)) {
		if (atoi(value))
			login->connect_timeout = atoi(value);
	} else if (!strcmp(option, TDS_STR_HOST)) {
		char tmp[128];
		struct tds_addrinfo *addrs;

		if (TDS_FAILED(tds_lookup_host_set(value, &login->ip_addrs))) {
			tdsdump_log(TDS_DBG_WARN, "Found host entry %s however name resolution failed. \n", value);
			return;
		}

		tdsdump_log(TDS_DBG_INFO1, "Found host entry %s \n", value);
		tds_dstr_copy(&login->server_host_name, value);
		for (addrs = login->ip_addrs; addrs != NULL; addrs = addrs->ai_next)
			tdsdump_log(TDS_DBG_INFO1, "IP addr is %s.\n", tds_addrinfo2str(addrs, tmp, sizeof(tmp)));

	} else if (!strcmp(option, TDS_STR_PORT)) {
		if (atoi(value))
			login->port = atoi(value);
	} else if (!strcmp(option, TDS_STR_EMUL_LE)) {
		login->emul_little_endian = tds_config_boolean(option, value, login);
	} else if (!strcmp(option, TDS_STR_TEXTSZ)) {
		if (atoi(value))
			login->text_size = atoi(value);
	} else if (!strcmp(option, TDS_STR_CHARSET)) {
		tds_dstr_copy(&login->server_charset, value);
		tdsdump_log(TDS_DBG_INFO1, "%s is %s.\n", option, tds_dstr_cstr(&login->server_charset));
	} else if (!strcmp(option, TDS_STR_CLCHARSET)) {
		tds_dstr_copy(&login->client_charset, value);
		tdsdump_log(TDS_DBG_INFO1, "tds_parse_conf_section: %s is %s.\n", option, tds_dstr_cstr(&login->client_charset));
	} else if (!strcmp(option, TDS_STR_LANGUAGE)) {
		tds_dstr_copy(&login->language, value);
	} else if (!strcmp(option, TDS_STR_APPENDMODE)) {
		tds_g_append_mode = tds_config_boolean(option, value, login);
	} else if (!strcmp(option, TDS_STR_INSTANCE)) {
		tds_dstr_copy(&login->instance_name, value);
	} else if (!strcmp(option, TDS_STR_ENCRYPTION)) {
		tds_config_encryption(value, login);
	} else if (!strcmp(option, TDS_STR_ASA_DATABASE)) {
		tds_dstr_copy(&login->server_name, value);
	} else if (!strcmp(option, TDS_STR_USENTLMV2)) {
		login->use_ntlmv2 = tds_config_boolean(option, value, login);
	} else if (!strcmp(option, TDS_STR_USELANMAN)) {
		login->use_lanman = tds_config_boolean(option, value, login);
	} else if (!strcmp(option, TDS_STR_REALM)) {
		tds_dstr_copy(&login->server_realm_name, value);
	} else if (!strcmp(option, TDS_STR_SPN)) {
		tds_dstr_copy(&login->server_spn, value);
	} else {
		tdsdump_log(TDS_DBG_INFO1, "UNRECOGNIZED option '%s' ... ignoring.\n", option);
	}
}

static void
tds_config_login(TDSLOGIN * connection, TDSLOGIN * login)
{
	if (!tds_dstr_isempty(&login->server_name)) {
		if (1 || tds_dstr_isempty(&connection->server_name)) 
			tds_dstr_dup(&connection->server_name, &login->server_name);
	}
	if (login->tds_version)
		connection->tds_version = login->tds_version;
	if (!tds_dstr_isempty(&login->language)) {
		tds_dstr_dup(&connection->language, &login->language);
	}
	if (!tds_dstr_isempty(&login->server_charset)) {
		tds_dstr_dup(&connection->server_charset, &login->server_charset);
	}
	if (!tds_dstr_isempty(&login->client_charset)) {
		tds_dstr_dup(&connection->client_charset, &login->client_charset);
		tdsdump_log(TDS_DBG_INFO1, "tds_config_login: %s is %s.\n", "client_charset",
			    tds_dstr_cstr(&connection->client_charset));
	}
	if (!tds_dstr_isempty(&login->database)) {
		tds_dstr_dup(&connection->database, &login->database);
		tdsdump_log(TDS_DBG_INFO1, "tds_config_login: %s is %s.\n", "database_name",
			    tds_dstr_cstr(&connection->database));
	}
	if (!tds_dstr_isempty(&login->client_host_name)) {
		tds_dstr_dup(&connection->client_host_name, &login->client_host_name);
	}
	if (!tds_dstr_isempty(&login->app_name)) {
		tds_dstr_dup(&connection->app_name, &login->app_name);
	}
	if (!tds_dstr_isempty(&login->user_name)) {
		tds_dstr_dup(&connection->user_name, &login->user_name);
	}
	if (!tds_dstr_isempty(&login->password)) {
		/* for security reason clear memory */
		tds_dstr_zero(&connection->password);
		tds_dstr_dup(&connection->password, &login->password);
	}
	if (!tds_dstr_isempty(&login->library)) {
		tds_dstr_dup(&connection->library, &login->library);
	}
	if (login->encryption_level) {
		connection->encryption_level = login->encryption_level;
	}
	if (login->suppress_language) {
		connection->suppress_language = 1;
	}
	if (login->bulk_copy) {
		connection->bulk_copy = 1;
	}
	if (login->block_size) {
		connection->block_size = login->block_size;
	}
	if (login->port)
		connection->port = login->port;
	if (login->connect_timeout)
		connection->connect_timeout = login->connect_timeout;

	if (login->query_timeout)
		connection->query_timeout = login->query_timeout;

	/* copy other info not present in configuration file */
	connection->capabilities = login->capabilities;
}

static void
tds_config_env_tdsdump(TDSLOGIN * login)
{
	char *s;
	char *path;
	pid_t pid = 0;

	if ((s = getenv("TDSDUMP"))) {
		if (!strlen(s)) {
			pid = getpid();
			if (asprintf(&path, pid_logpath, pid) >= 0)
				tds_dstr_set(&login->dump_file, path);
		} else {
			tds_dstr_copy(&login->dump_file, s);
		}
		tdsdump_log(TDS_DBG_INFO1, "Setting 'dump_file' to '%s' from $TDSDUMP.\n", tds_dstr_cstr(&login->dump_file));
	}
}
static void
tds_config_env_tdsport(TDSLOGIN * login)
{
	char *s;

	if ((s = getenv("TDSPORT"))) {
		login->port = tds_lookup_port(s);
		tds_dstr_copy(&login->instance_name, "");
		tdsdump_log(TDS_DBG_INFO1, "Setting 'port' to %s from $TDSPORT.\n", s);
	}
	return;
}
static void
tds_config_env_tdsver(TDSLOGIN * login)
{
	char *tdsver;

	if ((tdsver = getenv("TDSVER"))) {
		TDS_USMALLINT *pver = tds_config_verstr(tdsver, login);
		tdsdump_log(TDS_DBG_INFO1, "TDS version %sset to %s from $TDSVER.\n", (pver? "":"not "), tdsver);

	}
	return;
}

/* TDSHOST env var, pkleef@openlinksw.com 01/21/02 */
static void
tds_config_env_tdshost(TDSLOGIN * login)
{
	const char *tdshost;
	char tmp[128];
	struct tds_addrinfo *addrs;

	if (!(tdshost = getenv("TDSHOST")))
		return;

	if (TDS_FAILED(tds_lookup_host_set(tdshost, &login->ip_addrs))) {
		tdsdump_log(TDS_DBG_WARN, "Name resolution failed for '%s' from $TDSHOST.\n", tdshost);
		return;
	}

	tds_dstr_copy(&login->server_host_name, tdshost);
	for (addrs = login->ip_addrs; addrs != NULL; addrs = addrs->ai_next) {
		tdsdump_log(TDS_DBG_INFO1, "Setting IP Address to %s (%s) from $TDSHOST.\n",
			    tds_addrinfo2str(addrs, tmp, sizeof(tmp)), tdshost);
	}
}
#define TDS_FIND(k,b,c) tds_find(k, b, sizeof(b)/sizeof(b[0]), sizeof(b[0]), c)


static void *
tds_find(const void *key, const void *base, size_t nelem, size_t width,
         int (*compar)(const void *, const void *))
{
	size_t i;
	for (i=0; i < nelem; i++) {
		char *p = (char*)base + width * i;
		if (0 == compar(key, p)) {
			return p;
		}
	}
	return NULL;
}

struct tdsvername_t 
{
	const char *name;
	TDS_USMALLINT version;
};

static int
tds_vernanme_cmp(const void *key, const void *pelem)
{
	return strcmp((const char *)key, ((const struct tdsvername_t *)pelem)->name);
}	

/**
 * Set TDS version from given string
 * @param tdsver tds string version
 * @param login where to store information
 * @return as encoded hex value: high nybble major, low nybble minor.
 */
TDS_USMALLINT *
tds_config_verstr(const char *tdsver, TDSLOGIN * login)
{
	static const struct tdsvername_t tds_versions[] = 
		{ {   "0", 0x000 }
		, {"auto", 0x000 }
		, { "4.2", 0x402 }
		, { "4.2", 0x402 }
		, {  "46", 0x406 }
		, { "4.6", 0x406 }
		, {  "50", 0x500 }
		, { "5.0", 0x500 }
		, {  "70", 0x700 }
		, { "7.0", 0x700 }
		, {  "80", 0x701 }
		, { "8.0", 0x701 }
		, { "7.1", 0x701 }
		, { "7.2", 0x702 }
		, { "7.3", 0x703 }
		}, *pver;

	if (!login) {
		assert(login);
		return NULL;
	}

	if ((pver = TDS_FIND(tdsver, tds_versions, tds_vernanme_cmp)) == NULL) {
		tdsdump_log(TDS_DBG_INFO1, "error: no such version: %s\n", tdsver);
		return NULL;
	}
	
	login->tds_version = pver->version;
	tdsdump_log(TDS_DBG_INFO1, "Setting tds version to %s (0x%0x).\n", tdsver, pver->version);

	return &login->tds_version;
}

/**
 * Set the full name of interface file
 * @param interf file name
 */
TDSRET
tds_set_interfaces_file_loc(const char *interf)
{
	/* Free it if already set */
	if (interf_file != NULL)
		TDS_ZERO_FREE(interf_file);
	/* If no filename passed, leave it NULL */
	if ((interf == NULL) || (interf[0] == '\0')) {
		return TDS_SUCCESS;
	}
	/* Set to new value */
	if ((interf_file = strdup(interf)) == NULL) {
		return TDS_FAIL;
	}
	return TDS_SUCCESS;
}

/**
 * Get the IP address for a hostname. Store server's IP address 
 * in the string 'ip' in dotted-decimal notation.  (The "hostname" might itself
 * be a dotted-decimal address.  
 *
 * If we can't determine the IP address then 'ip' will be set to empty
 * string.
 */
/* TODO callers seem to set always connection info... change it */
struct tds_addrinfo *
tds_lookup_host(const char *servername)	/* (I) name of the server                  */
{
	struct tds_addrinfo hints, *addr = NULL;
	assert(servername != NULL);

	memset(&hints, '\0', sizeof(hints));
	hints.ai_family = AF_UNSPEC;

#ifdef AI_ADDRCONFIG
	hints.ai_flags |= AI_ADDRCONFIG;
#endif

	if (tds_getaddrinfo(servername, NULL, &hints, &addr))
		return NULL;
	return addr;
}

TDSRET
tds_lookup_host_set(const char *servername, struct tds_addrinfo **addr)
{
	struct tds_addrinfo *newaddr;
	assert(servername != NULL && addr != NULL);

	if ((newaddr = tds_lookup_host(servername)) != NULL) {
		if (*addr != NULL)
			tds_freeaddrinfo(*addr);
		*addr = newaddr;
		return TDS_SUCCESS;
	}
	return TDS_FAIL;
}

/**
 * Given a portname lookup the port.
 *
 * If we can't determine the port number then return 0.
 */
static int
tds_lookup_port(const char *portname)
{
	int num = atoi(portname);
	if (!num)
		num = tds_getservice(portname);
	return num;
}

/* TODO same code in convert.c ?? */
static int
hexdigit(int c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	/* ascii optimization, 'A' -> 'a', 'a' -> 'a' */
	c |= 0x20;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return 0;	/* bad hex digit */
}

static int
hex2num(char *hex)
{
	return hexdigit(hex[0]) * 16 + hexdigit(hex[1]);
}

/**
 * Open and read the file 'file' searching for a logical server
 * by the name of 'host'.  If one is found then lookup
 * the IP address and port number and store them in 'login'
 *
 * \param dir name of base directory for interface file
 * \param file name of the interface file
 * \param host logical host to search for
 * \return 0 if not fount 1 if found
 */
static int
search_interface_file(TDSLOGIN * login, const char *dir, const char *file, const char *host)
{
	char *pathname;
	char line[255];
	char tmp_ip[sizeof(line)];
	char tmp_port[sizeof(line)];
	char tmp_ver[sizeof(line)];
	FILE *in;
	char *field;
	int found = 0;
	int server_found = 0;
	char *lasts;

	line[0] = '\0';
	tmp_ip[0] = '\0';
	tmp_port[0] = '\0';
	tmp_ver[0] = '\0';

	tdsdump_log(TDS_DBG_INFO1, "Searching interfaces file %s/%s.\n", dir, file);
	pathname = (char *) malloc(strlen(dir) + strlen(file) + 10);
	if (!pathname)
		return 0;

	/*
	 * create the full pathname to the interface file
	 */
	if (file[0] == '\0') {
		pathname[0] = '\0';
	} else {
		if (dir[0] == '\0') {
			pathname[0] = '\0';
		} else {
			strcpy(pathname, dir);
			strcat(pathname, TDS_SDIR_SEPARATOR);
		}
		strcat(pathname, file);
	}


	/*
	 * parse the interfaces file and find the server and port
	 */
	if ((in = fopen(pathname, "r")) == NULL) {
		tdsdump_log(TDS_DBG_INFO1, "Couldn't open %s.\n", pathname);
		free(pathname);
		return 0;
	}
	tdsdump_log(TDS_DBG_INFO1, "Interfaces file %s opened.\n", pathname);

	while (fgets(line, sizeof(line) - 1, in)) {
		if (line[0] == '#')
			continue;	/* comment */

		if (!TDS_ISSPACE(line[0])) {
			field = strtok_r(line, "\n\t ", &lasts);
			if (!strcmp(field, host)) {
				found = 1;
				tdsdump_log(TDS_DBG_INFO1, "Found matching entry for host %s.\n", host);
			} else
				found = 0;
		} else if (found && TDS_ISSPACE(line[0])) {
			field = strtok_r(line, "\n\t ", &lasts);
			if (field != NULL && !strcmp(field, "query")) {
				field = strtok_r(NULL, "\n\t ", &lasts);	/* tcp or tli */
				if (!strcmp(field, "tli")) {
					tdsdump_log(TDS_DBG_INFO1, "TLI service.\n");
					field = strtok_r(NULL, "\n\t ", &lasts);	/* tcp */
					field = strtok_r(NULL, "\n\t ", &lasts);	/* device */
					field = strtok_r(NULL, "\n\t ", &lasts);	/* host/port */
					if (strlen(field) >= 18) {
						sprintf(tmp_port, "%d", hex2num(&field[6]) * 256 + hex2num(&field[8]));
						sprintf(tmp_ip, "%d.%d.%d.%d", hex2num(&field[10]),
							hex2num(&field[12]), hex2num(&field[14]), hex2num(&field[16]));
						tdsdump_log(TDS_DBG_INFO1, "tmp_port = %s. tmp_ip = %s.\n", tmp_port, tmp_ip);
					}
				} else {
					field = strtok_r(NULL, "\n\t ", &lasts);	/* ether */
					strcpy(tmp_ver, field);
					field = strtok_r(NULL, "\n\t ", &lasts);	/* host */
					strcpy(tmp_ip, field);
					tdsdump_log(TDS_DBG_INFO1, "host field %s.\n", tmp_ip);
					field = strtok_r(NULL, "\n\t ", &lasts);	/* port */
					strcpy(tmp_port, field);
				}	/* else */
				server_found = 1;
			}	/* if */
		}		/* else if */
	}			/* while */
	fclose(in);
	free(pathname);


	/*
	 * Look up the host and service
	 */
	if (server_found) {

		if (TDS_SUCCEED(tds_lookup_host_set(tmp_ip, &login->ip_addrs))) {
			struct tds_addrinfo *addrs;
			tds_dstr_copy(&login->server_host_name, tmp_ip);
			for (addrs = login->ip_addrs; addrs != NULL; addrs = addrs->ai_next) {
				tdsdump_log(TDS_DBG_INFO1, "Resolved IP as '%s'.\n",
					    tds_addrinfo2str(login->ip_addrs, line, sizeof(line)));
			}
		} else {
			tdsdump_log(TDS_DBG_WARN, "Name resolution failed for IP '%s'.\n", tmp_ip);
		}

		if (tmp_port[0])
			login->port = tds_lookup_port(tmp_port);
		if (tmp_ver[0])
			tds_config_verstr(tmp_ver, login);
	}
	return server_found;
}				/* search_interface_file()  */

/**
 * Try to find the IP number and port for a (possibly) logical server name.
 *
 * @note This function uses only the interfaces file and is deprecated.
 */
static int
tds_read_interfaces(const char *server, TDSLOGIN * login)
{
	int found = 0;

	/* read $SYBASE/interfaces */

	if (!server || !server[0]) {
		server = getenv("TDSQUERY");
		if (!server || !server[0])
			server = "SYBASE";
		tdsdump_log(TDS_DBG_INFO1, "Setting server to %s from $TDSQUERY.\n", server);

	}
	tdsdump_log(TDS_DBG_INFO1, "Looking for server %s....\n", server);

	/*
	 * Look for the server in the interf_file iff interf_file has been set.
	 */
	if (interf_file) {
		tdsdump_log(TDS_DBG_INFO1, "Looking for server in file %s.\n", interf_file);
		found = search_interface_file(login, "", interf_file, server);
	}

	/*
	 * if we haven't found the server yet then look for a $HOME/.interfaces file
	 */
	if (!found) {
		char *path = tds_get_home_file(".interfaces");

		if (path) {
			tdsdump_log(TDS_DBG_INFO1, "Looking for server in %s.\n", path);
			found = search_interface_file(login, "", path, server);
			free(path);
		}
	}

	/*
	 * if we haven't found the server yet then look in $SYBBASE/interfaces file
	 */
	if (!found) {
		const char *sybase = getenv("SYBASE");
#ifdef __VMS
		/* We've got to be in unix syntax for later slash-joined concatenation. */
		#include <unixlib.h>
		const char *unixspec = decc$translate_vms(sybase);
		if ( (int)unixspec != 0 && (int)unixspec != -1 ) sybase = unixspec;
#endif
		if (!sybase || !sybase[0])
			sybase = interfaces_path;

		tdsdump_log(TDS_DBG_INFO1, "Looking for server in %s/interfaces.\n", sybase);
		found = search_interface_file(login, sybase, "interfaces", server);
	}

	/*
	 * If we still don't have the server and port then assume the user
	 * typed an actual server host name.
	 */
	if (!found) {
		int ip_port;
		const char *env_port;

		/*
		 * Make a guess about the port number
		 */

		if (login->port == 0) {
			/*
			 * Not set in the [global] section of the
			 * configure file, take a guess.
			 */
			ip_port = TDS_DEF_PORT;
		} else {
			/*
			 * Preserve setting from the [global] section
			 * of the configure file.
			 */
			ip_port = login->port;
		}
		if ((env_port = getenv("TDSPORT")) != NULL) {
			ip_port = tds_lookup_port(env_port);
			tdsdump_log(TDS_DBG_INFO1, "Setting 'ip_port' to %s from $TDSPORT.\n", env_port);
		} else
			tdsdump_log(TDS_DBG_INFO1, "Setting 'ip_port' to %d as a guess.\n", ip_port);

		/*
		 * look up the host
		 */

		if (TDS_SUCCEED(tds_lookup_host_set(server, &login->ip_addrs)))
			tds_dstr_copy(&login->server_host_name, server);

		if (ip_port)
			login->port = ip_port;
	}

	return found;
}

/**
 * Check the server name to find port info first
 * Warning: connection-> & login-> are all modified when needed
 * \return 1 when found, else 0
 */
static int
parse_server_name_for_port(TDSLOGIN * connection, TDSLOGIN * login)
{
	const char *pSep;
	const char *server;

	/* seek the ':' in login server_name */
	server = tds_dstr_cstr(&login->server_name);

	/* IPv6 address can be quoted */
	if (server[0] == '[') {
		pSep = strstr(server, "]:");
		if (pSep)
			++pSep;
	} else {
		pSep = strrchr(server, ':');
	}

	if (pSep && pSep != server) {	/* yes, i found it! */
		/* modify connection-> && login->server_name & ->port */
		login->port = connection->port = atoi(pSep + 1);
		tds_dstr_copy(&connection->instance_name, "");
	} else {
		/* handle instance name */
		pSep = strrchr(server, '\\');
		if (!pSep || pSep == server)
			return 0;

		tds_dstr_copy(&connection->instance_name, pSep + 1);
		connection->port = 0;
	}

	if (!tds_dstr_copyn(&connection->server_name, server, pSep - server))
		return 0;

	return 1;
}

/**
 * Return a structure capturing the compile-time settings provided to the
 * configure script.  
 */

const TDS_COMPILETIME_SETTINGS *
tds_get_compiletime_settings(void)
{
	static const TDS_COMPILETIME_SETTINGS settings = {
		  TDS_VERSION_NO
		, FREETDS_SYSCONFDIR
		, "unknown"	/* need fancy script in makefile */
#		ifdef MSDBLIB
			, 1
#		else
			, 0
#		endif
#		ifdef TDS_SYBASE_COMPAT
			, 1
#		else
			, 0
#		endif
#		ifdef _REENTRANT
			, 1
#		else
			, 0
#		endif
#		ifdef HAVE_ICONV
			, 1
#		else
			, 0
#		endif
#		ifdef TDS46
			, "4.6"
#		elif TDS50
			, "5.0"
#		elif TDS70
			, "7.0"
#		elif TDS71
			, "7.1"
#		elif TDS72
			, "7.2"
#		elif TDS73
			, "7.3"
#		else
			, "4.2"
#		endif
#		ifdef IODBC
			, 1
#		else
			, 0
#		endif
#		ifdef UNIXODBC
			, 1
#		else
			, 0
#		endif
#		ifdef HAVE_OPENSSL
			, 1
#		else
			, 0
#		endif
#		ifdef HAVE_GNUTLS
			, 1
#		else
			, 0
#		endif
	};

	assert(settings.tdsver);

	return &settings;
}

/** @} */
