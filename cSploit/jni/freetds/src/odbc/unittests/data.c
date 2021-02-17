/*
 * Test reading data with SQLBindCol
 */
#include "common.h"
#include <assert.h>
#include <ctype.h>

/*
 * This test is useful to test odbc_tds2sql function
 * odbc_tds2sql have some particular cases:
 * (1) numeric -> binary  numeric is different in ODBC
 * (2) *       -> binary  dependent from libTDS representation and ODBC one
 * (3) binary  -> char    TODO
 * (4) date    -> char    different format
 * Also we have to check normal char and wide char
 */

static char software_version[] = "$Id: data.c,v 1.43 2011-09-05 18:52:43 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

static int result = 0;
static unsigned int line_num;

static int ignore_select_error = 0;

static void
Test(const char *type, const char *value_to_convert, SQLSMALLINT out_c_type, const char *expected)
{
	char sbuf[1024];
	unsigned char out_buf[256];
	SQLLEN out_len = 0;

	SQLFreeStmt(odbc_stmt, SQL_UNBIND);
	SQLFreeStmt(odbc_stmt, SQL_RESET_PARAMS);

	/* execute a select to get data as wire */
	sprintf(sbuf, "SELECT CONVERT(%s, '%s') AS data", type, value_to_convert);
	if (strncmp(value_to_convert, "0x", 2) == 0)
		sprintf(sbuf, "SELECT CONVERT(%s, %s) COLLATE Latin1_General_CI_AS AS data", type, value_to_convert);
	else if (strcmp(type, "SQL_VARIANT") == 0)
		sprintf(sbuf, "SELECT CONVERT(SQL_VARIANT, %s) AS data", value_to_convert);
	else if (strncmp(value_to_convert, "u&'", 3) == 0)
		sprintf(sbuf, "SELECT CONVERT(%s, %s) AS data", type, value_to_convert);
	if (ignore_select_error) {
		if (odbc_command2(sbuf, "SENo") == SQL_ERROR) {
			odbc_reset_statement();
			return;
		}
	} else {
		odbc_command(sbuf);
	}
	ignore_select_error = 0;
	SQLBindCol(odbc_stmt, 1, out_c_type, out_buf, sizeof(out_buf), &out_len);
	CHKFetch("S");
	CHKFetch("No");
	CHKMoreResults("No");

	/* test results */
	odbc_c2string(sbuf, out_c_type, out_buf, out_len);

	if (strcmp(sbuf, expected) != 0) {
		fprintf(stderr, "Wrong result\n  Got:      %s\n  Expected: %s\n", sbuf, expected);
		result = 1;
	}
}

static void
fatal(const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	if (msg[0] == ':')
		fprintf(stderr, "Line %u", line_num);
	vfprintf(stderr, msg, ap);
	va_end(ap);

	exit(1);
}

static int
get_int(const char *s)
{
	char *end;
	long l;

	if (!s)
		fatal(": NULL int\n");
	l = strtol(s, &end, 0);
	if (end[0])
		fatal(": Invalid int\n");
	return (int) l;
}

struct lookup_int
{
	const char *name;
	int value;
};

static int
lookup(const char *name, const struct lookup_int *table)
{
	if (!table)
		return get_int(name);

	for (; table->name; ++table)
		if (strcmp(table->name, name) == 0)
			return table->value;

	return get_int(name);
}

static struct lookup_int sql_c_types[] = {
#define TYPE(s) { #s, s }
	TYPE(SQL_C_NUMERIC),
	TYPE(SQL_C_BINARY),
	TYPE(SQL_C_CHAR),
	TYPE(SQL_C_WCHAR),
	TYPE(SQL_C_LONG),
	TYPE(SQL_C_SHORT),
#undef TYPE
	{ NULL, 0 }
};

#define SEP " \t\n"

static const char *
get_tok(char **p)
{
	char *s = *p, *end;
	s += strspn(s, SEP);
	if (!*s) return NULL;
	end = s + strcspn(s, SEP);
	*end = 0;
	*p = end+1;
	return s;
}

static void
parse_cstr(char *s)
{
	char hexbuf[4];
	char *d = s;

	while (*s) {
		if (*s != '\\') {
			*d++ = *s++;
			continue;
		}

		switch (*++s) {
		case '\"':
			*d++ = *s++;
			break;
		case '\\':
			*d++ = *s++;
			break;
		case 'x':
			if (strlen(s) < 3)
				fatal(": wrong string format\n");
			memcpy(hexbuf, ++s, 2);
			hexbuf[2] = 0;
			*d++ = strtoul(hexbuf, NULL, 16);
			s += 2;
			break;
		default:
			fatal(": wrong string format\n");
		}
	}
	*d = 0;
}

static const char *
get_str(char **p)
{
	char *s = *p, *end;
	s += strspn(s, SEP);
	if (!*s) fatal(": unable to get string\n");

	if (strncmp(s, "\"\"\"", 3) == 0) {
		s += 3;
		end = strstr(s, "\"\"\"");
		if (!end) fatal(": string not terminated\n");
		*end = 0;
		*p = end+3;
	} else if (s[0] == '\"') {
		++s;
		end = strchr(s, '\"');
		if (!end) fatal(": string not terminated\n");
		*end = 0;
		parse_cstr(s);
		*p = end+1;
	} else {
		return get_tok(p);
	}
	return s;
}

enum { MAX_BOOLS = 64 };
typedef struct {
	char *name;
	int value;
} bool_t;
static bool_t bools[MAX_BOOLS];

static void
set_bool(const char *name, int value)
{
	unsigned n;
	value = !!value;
	for (n = 0; n < MAX_BOOLS && bools[n].name; ++n)
		if (!strcmp(bools[n].name, name)) {
			bools[n]. value = value;
			return;
		}

	if (n == MAX_BOOLS)
		fatal(": no more boolean variable free\n");
	bools[n].name = strdup(name);
	if (!bools[n].name) fatal(": out of memory\n");
	bools[n].value = value;
}

static int
get_bool(const char *name)
{
	unsigned n;
	if (!name) fatal(": boolean variable not provided\n");
	for (n = 0; n < MAX_BOOLS && bools[n].name; ++n)
		if (!strcmp(bools[n].name, name))
			return bools[n]. value;

	fatal(": boolean variable %s not found\n", name);
	return 0;
}

static void
clear_bools(void)
{
	unsigned n;
	for (n = 0; n < MAX_BOOLS && bools[n].name; ++n) {
		free(bools[n].name);
		bools[n].name = NULL;
	}
}

enum { MAX_CONDITIONS = 32 };
static char conds[MAX_CONDITIONS];
static unsigned cond_level = 0;

static int
pop_condition(void)
{
	if (cond_level == 0) fatal(": no related if\n");
	return conds[--cond_level];
}

static void
push_condition(int cond)
{
	if (cond != 0 && cond != 1) fatal(": invalid cond value %d\n", cond);
	if (cond_level >= MAX_CONDITIONS) fatal(": too much nested conditions\n");
	conds[cond_level++] = cond;
}

static int
get_not_cond(char **p)
{
	int cond;
	const char *tok = get_tok(p);
	if (!tok) fatal(": wrong condition syntax\n");

	if (!strcmp(tok, "not"))
		cond = !get_bool(get_tok(p));
	else
		cond = get_bool(tok);

	return cond;
}

static int
get_condition(char **p)
{
	int cond1 = get_not_cond(p), cond2;
	const char *tok;

	while ((tok=get_tok(p)) != NULL) {

		cond2 = get_not_cond(p);

		if (!strcmp(tok, "or"))
			cond1 = cond1 || cond2;
		else if (!strcmp(tok, "and"))
			cond1 = cond1 && cond2;
		else fatal(": wrong condition syntax\n");
	}
	return cond1;
}

int
main(int argc, char *argv[])
{
	int big_endian = 1;
	int cond = 1;

#define TEST_FILE "data.in"
	const char *in_file = FREETDS_SRCDIR "/" TEST_FILE;
	FILE *f;
	char line_buf[512];

	if (((char *) &big_endian)[0] == 1)
		big_endian = 0;
	set_bool("bigendian", big_endian);

	odbc_connect();

	set_bool("msdb", odbc_db_is_microsoft());
	set_bool("freetds", odbc_driver_is_freetds());

	f = fopen(in_file, "r");
	if (!f)
		f = fopen(TEST_FILE, "r");
	if (!f) {
		fprintf(stderr, "error opening test file\n");
		exit(1);
	}

	line_num = 0;
	while (fgets(line_buf, sizeof(line_buf), f)) {
		char *p = line_buf;
		const char *cmd;

		++line_num;

		cmd = get_tok(&p);

		/* skip comments */
		if (!cmd || cmd[0] == '#' || cmd[0] == 0 || cmd[0] == '\n')
			continue;

		ODBC_FREE();

		/* conditional statement */
		if (!strcmp(cmd, "else")) {
			int c = pop_condition();
			push_condition(c);
			cond = c && !cond;
			continue;
		}
		if (!strcmp(cmd, "endif")) {
			cond = pop_condition();
			continue;
		}
		if (!strcmp(cmd, "if")) {
			push_condition(cond);
			if (cond)
				cond = get_condition(&p);
			continue;
		}

		/* select type */
		if (!strcmp(cmd, "select")) {
			const char *type = get_tok(&p);
			const char *value = get_str(&p);
			int c_type = lookup(get_tok(&p), sql_c_types);
			const char *expected = get_str(&p);

			if (!cond) continue;

			ignore_select_error = 1;
			Test(type, value, c_type, expected);
			continue;
		}
		/* select type setting condition */
		if (!strcmp(cmd, "select_cond")) {
			const char *bool_name = get_tok(&p);
			const char *type = get_tok(&p);
			const char *value = get_str(&p);
			int c_type = lookup(get_tok(&p), sql_c_types);
			const char *expected = get_str(&p);
			int save_result = result;

			if (!bool_name) fatal(": no condition name\n");
			if (!cond) continue;

			ignore_select_error = 1;
			result = 0;
			Test(type, value, c_type, expected);
			set_bool(bool_name, result == 0);
			result = save_result;
			continue;
		}
		/* execute a sql command */
		if (!strcmp(cmd, "sql")) {
			const char *sql = get_str(&p);

			if (!cond) continue;

			odbc_command(sql);
			continue;
		}
		if (!strcmp(cmd, "sql_cond")) {
			const char *bool_name = get_tok(&p);
			const char *sql = get_str(&p);

			if (!cond) continue;

			set_bool(bool_name, odbc_command2(sql, "SENo") != SQL_ERROR);
			continue;
		}
		fatal(": unknown command\n");
	}
	clear_bools();
	fclose(f);

	printf("\n");

	/* mssql 2008 give a warning for truncation (01004) */
	Test("VARCHAR(20)", "  15.1245  ", SQL_C_NUMERIC, "38 0 1 0F");

	odbc_disconnect();

	if (!result)
		printf("Done successfully!\n");
	return result;
}
