/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005 Brian Bruns
 * Copyright (C) 2006, 2007, 2008, 2009  Frediano Ziglio
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

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#if HAVE_FORK
#include <sys/wait.h>
#endif
#include <signal.h>

#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif /* HAVE_READLINE */

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
# include <unistd.h>
#elif defined(_WIN32)
# include <io.h>
#endif /* HAVE_UNISTD_H */

/* HP-UX require some constants defined by limits.h */
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif /* HAVE_LIMITS_H */

#if defined(__hpux__) && !defined(_POSIX_PATH_MAX)
#define _POSIX_PATH_MAX 255
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif /* HAVE_LOCALE_H */

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif /* HAVE_LANGINFO_H */

#ifdef HAVE_LOCALCHARSET_H
#include <localcharset.h>
#endif /* HAVE_LOCALCHARSET_H */

#include <freetds/tds.h>
#include <freetds/iconv.h>
#include <freetds/string.h>
#include <freetds/convert.h>
#include "replacements.h"

TDS_RCSID(var, "$Id: tsql.c,v 1.153 2012-03-11 15:52:22 freddy77 Exp $");

#define TDS_ISSPACE(c) isspace((unsigned char) (c))

enum
{
	OPT_VERSION =  0x01,
	OPT_TIMER =    0x02,
	OPT_NOFOOTER = 0x04,
	OPT_NOHEADER = 0x08,
	OPT_QUIET =    0x10,
	OPT_VERBOSE =  0x20,
	OPT_INSTANCES= 0x40
};

static int istty = 0;
static int global_opt_flags = 0;

#define QUIET (global_opt_flags & OPT_QUIET)
#define VERBOSE (global_opt_flags & OPT_VERBOSE)

static char *opt_col_term = "\t";
static char *opt_row_term = "\n";
static char *opt_default_db = NULL;

static int do_query(TDSSOCKET * tds, char *buf, int opt_flags);
static int get_opt_flags(char *s, int *opt_flags);
static void populate_login(TDSLOGIN * login, int argc, char **argv);
static int tsql_handle_message(const TDSCONTEXT * context, TDSSOCKET * tds, TDSMESSAGE * msg);
static int tsql_handle_error  (const TDSCONTEXT * context, TDSSOCKET * tds, TDSMESSAGE * msg);
static void slurp_input_file(char *fname, char **mybuf, size_t *bufsz, size_t *buflen, int *line);

static char *
tsql_readline(char *prompt)
{
	size_t sz, pos;
	char *line, *p;

#ifdef HAVE_READLINE
	if (istty)
		return readline(prompt);
#endif

	sz = 1024;
	pos = 0;
	line = (char*) malloc(sz);
	if (!line)
		return NULL;

	if (prompt && prompt[0])
		printf("%s", prompt);
	for (;;) {
		/* read a piece */
		if (fgets(line + pos, (int)(sz - pos), stdin) == NULL) {
			if (pos)
				return line;
			break;
		}

		/* got end-of-line ? */
		p = strchr(line + pos, '\n');
		if (p) {
			*p = 0;
			return line;
		}

		/* allocate space if needed */
		pos += strlen(line + pos);
		if (pos + 1024 >= sz) {
			sz += 1024;
			p = (char*) realloc(line, sz);
			if (!p)
				break;
			line = p;
		}
	}
	free(line);
	return NULL;
}

static void
tsql_add_history(const char *s)
{
#ifdef HAVE_READLINE
	if (istty)
		add_history(s);
#endif
}

/**
 * Returns the version of the TDS protocol in effect for the link
 * as a decimal integer.  
 *	Typical returned values are 42, 50, 70, 80.
 * Also fills pversion_string unless it is null.
 * 	Typical pversion_string values are "4.2" and "7.0".
 */
static int
tds_version(TDSCONNECTION * conn, char *pversion_string)
{
	int iversion = 0;

	iversion = 10 * TDS_MAJOR(conn) + TDS_MINOR(conn);

	sprintf(pversion_string, "%d.%d", TDS_MAJOR(conn), TDS_MINOR(conn));

	return iversion;
}

static int
do_query(TDSSOCKET * tds, char *buf, int opt_flags)
{
	int rows = 0;
	TDSRET rc;
	int i;
	TDSCOLUMN *col;
	int ctype;
	CONV_RESULT dres;
	unsigned char *src;
	TDS_INT srclen;
	TDS_INT resulttype;
	struct timeval start, stop;
	int print_rows = 1;
	char message[128];

	rc = tds_submit_query(tds, buf);
	if (TDS_FAILED(rc)) {
		fprintf(stderr, "tds_submit_query() failed\n");
		return 1;
	}

	while ((rc = tds_process_tokens(tds, &resulttype, NULL, TDS_TOKEN_RESULTS)) == TDS_SUCCESS) {
		const int stop_mask = TDS_STOPAT_ROWFMT|TDS_RETURN_DONE|TDS_RETURN_ROW|TDS_RETURN_COMPUTE;
		if (opt_flags & OPT_TIMER) {
			gettimeofday(&start, NULL);
			print_rows = 0;
		}
		switch (resulttype) {
		case TDS_ROWFMT_RESULT:
			if ((!(opt_flags & OPT_NOHEADER)) && tds->current_results) {
				for (i = 0; i < tds->current_results->num_cols; i++) {
					if (i) fputs(opt_col_term, stdout);
					fputs(tds->current_results->columns[i]->column_name, stdout);
				}
				fputs(opt_row_term, stdout);
			}
			break;
		case TDS_COMPUTE_RESULT:
		case TDS_ROW_RESULT:
			rows = 0;
			while ((rc = tds_process_tokens(tds, &resulttype, NULL, stop_mask)) == TDS_SUCCESS) {
				if (resulttype != TDS_ROW_RESULT && resulttype != TDS_COMPUTE_RESULT)
					break;

				rows++;

				if (!tds->current_results)
					continue;

				for (i = 0; i < tds->current_results->num_cols; i++) {
					col = tds->current_results->columns[i];
					if (col->column_cur_size < 0) {
						if (print_rows)  {
							if (i) fputs(opt_col_term, stdout);
							fputs("NULL", stdout);
						}
						continue;
					}
					ctype = tds_get_conversion_type(col->column_type, col->column_size);

					src = col->column_data;
					if (is_blob_col(col) && col->column_type != SYBVARIANT)
						src = (unsigned char *) ((TDSBLOB *) src)->textvalue;
					srclen = col->column_cur_size;


					if (tds_convert(tds_get_ctx(tds), ctype, (TDS_CHAR *) src, srclen, SYBVARCHAR, &dres) < 0)
						continue;
					if (print_rows)  {
						if (i) fputs(opt_col_term, stdout);
						fputs(dres.c, stdout);
					}
					free(dres.c);
				}
				if (print_rows)
					fputs(opt_row_term, stdout);

			}
			if (!QUIET) fprintf(stdout, "(%d row%s affected)\n", rows, rows == 1 ? "" : "s");
			break;
		case TDS_STATUS_RESULT:
			if (!QUIET)
				printf("(return status = %d)\n", tds->ret_status);
			break;
		default:
			break;
		}

		if (opt_flags & OPT_VERSION) {
			char version[64];
			int line = 0;

			line = tds_version(tds->conn, version);
			if (line) {
				TDSMESSAGE msg;
				memset(&msg, 0, sizeof(TDSMESSAGE));
				msg.server = "tsql";
				sprintf(message, "using TDS version %s", version);
				msg.message = message;
				tsql_handle_message(tds_get_ctx(tds), tds, &msg);
			}
		}
		if (opt_flags & OPT_TIMER) {
			TDSMESSAGE msg;
			gettimeofday(&stop, NULL);
			sprintf(message, "Total time for processing %d rows: %ld msecs\n",
				rows, (long) ((stop.tv_sec - start.tv_sec) * 1000) + ((stop.tv_usec - start.tv_usec) / 1000));

			memset(&msg, 0, sizeof(TDSMESSAGE));
			msg.server = "tsql";
			msg.message = message;
			tsql_handle_message(tds_get_ctx(tds), tds, &msg);
		}
	}
	return 0;
}

static void
tsql_print_usage(const char *progname)
{
	fprintf(stderr,
		"Usage:\t%s [-a <appname>] [-S <server> | -H <hostname> -p <port>] -U <username> [-P <password>] [-I <config file>] [-o <options>] [-t delim] [-r delim] [-D database]\n"
		"\t%s -C\n"
		"Options:\n"
		"\tf\tDo not print footer\n"
		"\th\tDo not print header\n"
		"\tt\tPrint time informations\n"
		"\tv\tPrint TDS version\n"
		"\tq\tQuiet\n\n"
		"\tDelimiters can be multi-char strings appropriately escaped for your shell.\n"
		"\tDefault column delimitor is <tab>; default row delimiter is <newline>\n",
		progname, progname);
}

static void
reset_getopt(void)
{
#ifdef HAVE_GETOPT_OPTRESET
	optreset = 1;
	optind = 1;
#else
	optind = 0;
#endif
}

/*
 * The 'GO' command may be followed by options that apply to the batch.
 * If they don't appear to be right, assume the letters "go" are part of the
 * SQL, not a batch separator.  
 */
static int
get_opt_flags(char *s, int *opt_flags)
{
	char **argv;
	int argc;
	int opt;

	/* make sure we have enough elements */
	assert(s && opt_flags);
	argv = (char **) calloc(strlen(s) + 2, sizeof(char*));
	if (!argv)
		return 0;

	/* parse the command line and assign to argv */
	for (argc=0; (argv[argc] = strtok(s, " ")) != NULL; argc++)
		s = NULL;

	*opt_flags = 0;
	reset_getopt();
	opterr = 0;		/* suppress error messages */
	while ((opt = getopt(argc, argv, "fhLqtv")) != -1) {
		switch (opt) {
		case 'f':
			*opt_flags |= OPT_NOFOOTER;
			break;
		case 'h':
			*opt_flags |= OPT_NOHEADER;
			break;
		case 't':
			*opt_flags |= OPT_TIMER;
			break;
		case 'v':
			*opt_flags |= OPT_VERSION;
			break;
		case 'q':
			*opt_flags |= OPT_QUIET;
			break;
		default:
			fprintf(stderr, "Warning: invalid option '%s' found: \"go\" treated as simple SQL\n", argv[optind-1]);
			free(argv);
			return 0;
		}
	}
	
	free(argv);
	return 1;
}

static int
get_default_instance_port(const char hostname[])
{
	int port;
	struct tds_addrinfo *addr;
	
	if ((addr = tds_lookup_host(hostname)) == NULL)
		return 0;

	port = tds7_get_instance_port(addr, "MSSQLSERVER");

	tds_freeaddrinfo(addr);
	
	return port;
}

#if !defined(LC_ALL)
# define LC_ALL 0
#endif

#ifdef HAVE_SSPI
const char have_sspi[] = "yes";
#else 
const char have_sspi[] = "no";
#endif
#ifdef ENABLE_KRB5
const char enable_krb5[] = "yes";
#else 
const char enable_krb5[] = "no";
#endif

static void
populate_login(TDSLOGIN * login, int argc, char **argv)
{
	const TDS_COMPILETIME_SETTINGS *settings;
	char *hostname = NULL, *servername = NULL;
	char *username = NULL, *password = NULL;
	char *confile = NULL;
	const char *appname = "TSQL";
	int opt, port=0, use_domain_login=0;
	const char *charset = NULL;
	char *opt_flags_str = NULL;

	while ((opt = getopt(argc, argv, "a:H:S:I:J:P:U:p:Co:t:r:D:Lv")) != -1) {
		switch (opt) {
		case 'a':
			appname = optarg;
			break;
		case 't':
			opt_col_term = strdup(optarg);
			break;
		case 'r':
			opt_row_term = strdup(optarg);
			break;
		case 'D':
			opt_default_db = strdup(optarg);
			break;
		case 'o':
			opt_flags_str = optarg;
			break;
		case 'H':
			free(hostname);
			hostname = strdup(optarg);
			break;
		case 'S':
			free(servername);
			servername = strdup(optarg);
			break;
		case 'U':
			free(username);
			username = strdup(optarg);
			break;
		case 'P':
			free(password);
			password = getpassarg(optarg);
			break;
		case 'I':
			free(confile);
			confile = strdup(optarg);
			break;
		case 'J':
			charset = strdup(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'L':
			global_opt_flags |= OPT_INSTANCES;
			break;
		case 'v':
			global_opt_flags |= OPT_VERBOSE;
			break;
		case 'C':
			settings = tds_get_compiletime_settings();
			printf("%s\n%35s: %s\n%35s: %s\n%35s: %s\n%35s: %s\n%35s: %s\n%35s: %s\n%35s: %s\n%35s: %s\n%35s: %s\n%35s: %s\n%35s: %s\n%35s: %s\n%35s: %s\n",
			       "Compile-time settings (established with the \"configure\" script)",
			       "Version", settings->freetds_version,
			       "freetds.conf directory", settings->sysconfdir, 
			       /* settings->last_update */
			       "MS db-lib source compatibility", settings->msdblib ? "yes" : "no",
			       "Sybase binary compatibility",
			       (settings->sybase_compat == -1 ? "unknown" : (settings->sybase_compat ? "yes" : "no")),
			       "Thread safety", settings->threadsafe ? "yes" : "no",
			       "iconv library", settings->libiconv ? "yes" : "no",
			       "TDS version", settings->tdsver,
			       "iODBC", settings->iodbc ? "yes" : "no", 
			       "unixodbc", settings->unixodbc ? "yes" : "no", 
			       "SSPI \"trusted\" logins", have_sspi, 
			       "Kerberos", enable_krb5,
			       "OpenSSL", settings->openssl ? "yes" : "no",
			       "GnuTLS", settings->gnutls ? "yes" : "no");
			exit(0);
			break;
		default:
			tsql_print_usage(argv[0]);
			exit(1);
			break;
		}
	}

	if (opt_flags_str != NULL) {
		char *minus_flags = malloc(strlen(opt_flags_str) + 5);
		if (minus_flags != NULL) {
			strcpy(minus_flags, "go -");
			strcat(minus_flags, opt_flags_str);
			get_opt_flags(minus_flags, &global_opt_flags);
			free(minus_flags);
		}
	}

	if ((global_opt_flags & OPT_INSTANCES) && hostname) {
		static const char template[] = "%s.instances";
		struct tds_addrinfo *addr;
		char *filename = getenv("TDSDUMP");

		if (filename) {
			if ((filename = malloc(sizeof(template) + strlen(filename))) == NULL) 
				exit(1);
			sprintf(filename, template, getenv("TDSDUMP"));
			tdsdump_open(filename);
			free(filename);
		}
		if ((addr = tds_lookup_host(hostname)) != NULL) {
			tds7_get_instance_ports(stderr, addr);
			tds_freeaddrinfo(addr);
		}
		tdsdump_close();
		exit(0);
	}

	/* validate parameters */
	if (!servername && !hostname) {
		fprintf(stderr, "%s: error: Missing argument -S or -H\n", argv[0]);
		exit(1);
	}
	if (hostname && !port) {
		/*
		 * TODO: It would be convenient to have a function that looks up a reasonable port based on:
		 * 	- TDSPORT environment variable
		 * 	- config files
		 * 	- get_default_instance_port
		 *	- TDS version
		 *	in that order. 
		 */
		if (!QUIET) {
			printf("Missing argument -p, looking for default instance ... ");
		}
		if (0 == (port = get_default_instance_port(hostname))) {
			fprintf(stderr, "%s: no default port provided by host %s\n", argv[0], hostname);
			exit(1);
		} 
		if (!QUIET)
			printf("found default instance, port %d\n", port);
		
	}
	/* A NULL username indicates a domain (trusted) login */
	if (!username) {
		username = calloc(1, 1);
		use_domain_login = 1;
	}
	if (!password) {
		password = calloc(1, 128);
		if (!use_domain_login)
			readpassphrase("Password: ", password, 128, RPP_ECHO_OFF);
	}
	if (!opt_col_term) {
		fprintf(stderr, "%s: missing delimiter for -t (check escaping)\n", argv[0]);
		exit(1);
	}
	if (!opt_row_term) {
		fprintf(stderr, "%s: missing delimiter for -r (check escaping)\n", argv[0]);
		exit(1);
	}

	/* all validated, let's do it */

	/* if it's a servername */
	if (servername) {
		tds_set_user(login, username);
		tds_set_app(login, appname);
		tds_set_library(login, "TDS-Library");
		tds_set_server(login, servername);
		if (charset) tds_set_client_charset(login, charset);
		tds_set_language(login, "us_english");
		tds_set_passwd(login, password);
		if (confile) {
			tds_set_interfaces_file_loc(confile);
		}
		/* else we specified hostname/port */
	} else {
		tds_set_user(login, username);
		tds_set_app(login, appname);
		tds_set_library(login, "TDS-Library");
		tds_set_server(login, hostname);
		tds_set_port(login, port);
		if (charset) tds_set_client_charset(login, charset);
		tds_set_language(login, "us_english");
		tds_set_passwd(login, password);
	}

	memset(password, 0, strlen(password));

	/* free up all the memory */
	free(confile);
	free(hostname);
	free(username);
	free(password);
	free(servername);
}

static int
tsql_handle_message(const TDSCONTEXT * context, TDSSOCKET * tds, TDSMESSAGE * msg)
{
	if (msg->msgno == 0) {
		fprintf(stderr, "%s\n", msg->message);
		return 0;
	}

	switch (msg->msgno) {
	case 5701: 	/* changed_database */
	case 5703: 	/* changed_language */
	case 20018:	/* The @optional_command_line is too long */
		if (VERBOSE && msg)
			fprintf(stderr, "%s\n", msg->message);
		break;
	default:
		fprintf(stderr, "Msg %d (severity %d, state %d) from %s", 
			msg->msgno, msg->severity, msg->state, msg->server);
		if (msg->proc_name && strlen(msg->proc_name))
			fprintf(stderr, ", Procedure %s", msg->proc_name);
		if (msg->line_number > 0)
			fprintf(stderr, " Line %d", msg->line_number);
		fprintf(stderr, ":\n\t\"%s\"\n", msg->message);
		break;
	}

	return 0;
}

static int	/* error from library, not message from server */
tsql_handle_error(const TDSCONTEXT * context, TDSSOCKET * tds, TDSMESSAGE * msg)
{
	fprintf(stderr, "Error %d (severity %d):\n\t%s\n", msg->msgno, msg->severity, msg->message);
	if (0 != msg->oserr) {
		fprintf(stderr, "\tOS error %d, \"%s\"\n", msg->oserr, strerror(msg->oserr));
	}
	return TDS_INT_CANCEL;
}

static void
slurp_input_file(char *fname, char **mybuf, size_t *bufsz, size_t *buflen, int *line)
{
	FILE *fp = NULL;
	register char *n;
	char linebuf[1024];
	char *s = NULL;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "Unable to open input file '%s': %s\n", fname, strerror(errno));
		return;
	}
	while ((s = fgets(linebuf, sizeof(linebuf), fp)) != NULL) {
		while (*buflen + strlen(s) + 2 > *bufsz) {
			*bufsz *= 2;
			*mybuf = (char *) realloc(*mybuf, *bufsz);
		}
		strcpy(*mybuf + *buflen, s);
		*buflen += strlen(*mybuf + *buflen);
		n = strrchr(s, '\n');
		if (n != NULL)
			*n = '\0';
		tsql_add_history(s);
		(*line)++;
	}
	fclose(fp);
}

static void
print_instance_data(TDSLOGIN *login) 
{
	if (!login)
		return;
	
	if (!tds_dstr_isempty(&login->instance_name))
		printf("connecting to instance %s on port %d\n", tds_dstr_cstr(&login->instance_name), login->port);
}

int
main(int argc, char **argv)
{
	char *s = NULL, *s2 = NULL, *cmd = NULL;
	char prompt[20];
	int line = 0;
	char *mybuf;
	size_t bufsz = 4096;
	size_t buflen = 0;
	TDSSOCKET *tds;
	TDSLOGIN *login;
	TDSCONTEXT *context;
	TDSLOGIN *connection;
	int opt_flags = 0;
#if HAVE_FORK
	pid_t timer_pid = 0;
	int pipes[2];
#endif
	const char *locale = NULL;
	const char *charset = NULL;

	istty = isatty(0);

	if (INITSOCKET()) {
		fprintf(stderr, "Unable to initialize sockets\n");
		return 1;
	}

	setlocale(LC_ALL, "");

	/* grab a login structure */
	login = tds_alloc_login(1);

	context = tds_alloc_context(NULL);
	if (context->locale && !context->locale->date_fmt) {
		/* set default in case there's no locale file */
		context->locale->date_fmt = strdup(STD_DATETIME_FMT);
	}

	context->msg_handler = tsql_handle_message;
	context->err_handler = tsql_handle_error;

	/* process all the command line args into the login structure */
	populate_login(login, argc, argv);

	/* Try to open a connection */
	tds = tds_alloc_socket(context, 512);
	assert(tds);
	tds_set_parent(tds, NULL);
	connection = tds_read_config_info(tds, login, context->locale);

	locale = setlocale(LC_ALL, NULL);

#if HAVE_LOCALE_CHARSET
	charset = locale_charset();
#endif
#if HAVE_NL_LANGINFO && defined(CODESET)
	if (!charset)
		charset = nl_langinfo(CODESET);
#endif

	if (locale)
		if (!QUIET) printf("locale is \"%s\"\n", locale);
	if (charset) {
		if (!QUIET) printf("locale charset is \"%s\"\n", charset);
	}
	
	if (tds_dstr_isempty(&connection->client_charset)) {
		if (!charset)
			charset = "ISO-8859-1";

		tds_set_client_charset(login, charset);
		tds_dstr_dup(&connection->client_charset, &login->client_charset);
	}
	if (!QUIET) printf("using default charset \"%s\"\n", tds_dstr_cstr(&connection->client_charset));

	if (opt_default_db) {
		tds_dstr_copy(&connection->database, opt_default_db);
		if (!QUIET) fprintf(stderr, "Setting %s as default database in login packet\n", opt_default_db);
	}

	/* 
	 * If we're able to establish an ip address for the server, we'll try to connect to it. 
	 * If that machine is currently unreachable, show a timer connecting to the server. 
	 */
#if HAVE_FORK
	if (connection && !QUIET) {
		/*
		 * We use a pipe so that the child is killed even main program aborts. 
		 * Note that we do not read or write from this pipe. 
		 */
		if (pipe(pipes) < 0) {
			perror("tsql: pipe");
			return 1;
		}

		timer_pid = fork();
		if (-1 == timer_pid) {
			perror("tsql: warning"); /* make a note */
		}
		if (0 == timer_pid) {
			int i=0;
			close(pipes[1]);
			while(1) {
				fd_set set;
				struct timeval tv;

				FD_ZERO(&set);
				FD_SET(pipes[0], &set);

				tv.tv_sec = 1;
				tv.tv_usec = 0;
				if (select(pipes[0] + 1, &set, NULL, &set, &tv) != 0)
					return 0;

				fprintf(stderr, "\r%2d", ++i);
			}
		}
		close(pipes[0]);
	}
#endif
	if (!connection || TDS_FAILED(tds_connect_and_login(tds, connection))) {
		if (VERBOSE)
			print_instance_data(connection);
		tds_free_socket(tds);
		tds_free_login(login);
		tds_free_context(context);
		fprintf(stderr, "There was a problem connecting to the server\n");
		exit(1);
	}
	if (VERBOSE) 
		print_instance_data(connection);
	tds_free_login(connection);
	/* give the buffer an initial size */
	bufsz = 4096;
	mybuf = malloc(bufsz);
	mybuf[0] = '\0';
	buflen = 0;

#if defined(HAVE_READLINE) && HAVE_RL_INHIBIT_COMPLETION
	rl_inhibit_completion = 1;
#endif

#if HAVE_FORK
	if (timer_pid > 0 ) {
		close(pipes[1]);
		if (wait(0) == -1 ) {
			perror("tsql: warning");
		} else {
			fprintf(stderr, "\r");
		}
	}
#endif

	for (s=NULL, s2=NULL; ; free(s), free(s2), s2=NULL) {
		sprintf(prompt, "%d> ", ++line);
		s = tsql_readline(QUIET ? NULL : prompt);
		if (s == NULL) 
			break;

		/* 
		 * 'GO' is special only at the start of a line
		 *  The rest of the line may include options that apply to the batch, 
		 *  and perhaps whitespace.  
		 */
		if (0 == strncasecmp(s, "go", 2) && (strlen(s) == 2 || TDS_ISSPACE(s[2]))) {
			char *go_line = strdup(s);
			assert(go_line);
			line = 0;
			if (get_opt_flags(go_line, &opt_flags)) {
				free(go_line);
				opt_flags ^= global_opt_flags;
				do_query(tds, mybuf, opt_flags);
				mybuf[0] = '\0';
				buflen = 0;
				continue;
			}
			free(go_line);
		}
		
		/* skip leading whitespace */
		if ((s2 = strdup(s)) == NULL) {	/* copy to mangle with strtok() */
			perror("tsql: ");
			exit(1);
		}
		
		if ((cmd = strtok(s2, " \t")) == NULL)
			cmd = "";

		if (!strcasecmp(cmd, "exit") || !strcasecmp(cmd, "quit") || !strcasecmp(cmd, "bye"))
			break;
		if (!strcasecmp(cmd, "version")) {
			tds_version(tds->conn, mybuf);
			printf("using TDS version %s\n", mybuf);
			line = 0;
			mybuf[0] = '\0';
			buflen = 0;
			continue;
		}
		if (!strcasecmp(cmd, "reset")) {
			line = 0;
			mybuf[0] = '\0';
			buflen = 0;
		} else if (!strcasecmp(cmd, ":r")) {
			slurp_input_file(strtok(NULL, " \t"), &mybuf, &bufsz, &buflen, &line);
		} else {
			while (buflen + strlen(s) + 2 > bufsz) {
				char *newbuf; 
				bufsz *= 2;
				if ((newbuf = realloc(mybuf, bufsz)) == NULL) {
					perror("tsql: ");
					exit(1);
				}
				mybuf = newbuf;
			}
			tsql_add_history(s);
			strcpy(mybuf + buflen, s);
			/* preserve line numbering for the parser */
			strcat(mybuf + buflen, "\n");
			buflen += strlen(mybuf + buflen);
		}
	}

	/* close up shop */
	free(mybuf);
	tds_free_socket(tds);
	tds_free_login(login);
	tds_free_context(context);
	DONESOCKET();

	return 0;
}
